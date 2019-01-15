// BranchAndBound.cpp : Ten plik zawiera funkcję „main”. W nim rozpoczyna się i kończy wykonywanie programu.
//

#include "pch.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <ctime>
#include <tuple>
#include <string>
#include <stack>
#include <limits>
#include <algorithm>
#include <iterator>
#include <chrono>

using namespace std;


class Job{
	public:
		unsigned int job_id;
		unsigned int operation1; // czas trwania pierwszej operacji
		unsigned int operation2; // czas trwania drugiej operacji


		Job() {}

		Job(unsigned int id, unsigned int op1, unsigned int op2) {
			job_id = id;
			operation1 = op1;
			operation2 = op2;
		}

		~Job() {}
};

class Solution {
	public:
		//unsigned long long solution_id;
		vector<unsigned int> job_ids; // wektor przechowujacy liczby porzadkowe zadan obecnych w rozwiazaniu
		vector<Job> seq_mach1; // sekwencja operacji na pierwszej maszynie
		vector<Job> seq_mach2; // sekwencja operacji na drugiej maszynie
		vector<tuple<unsigned int, unsigned int, unsigned int>> mach1; // uszeregowanie na pierwszej maszynie: <job_id, start, end>
		vector<tuple<int, unsigned int, unsigned int>> mach2; // uszeregowanie na drugiej maszynie: <job_id (+) lub hole_id (-), start, end>
		vector<tuple<unsigned int, unsigned int, unsigned int>> holes; // wektor przechowujacy dziury na drugiej maszynie: hole_id, start, end
		unsigned int cost; // wartosc naszej funkcji celu

		Solution() {}

		Solution(vector<unsigned int> job_id, vector<Job> seq1, vector<Job> seq2) {
			//solution_id = sol_id;
			job_ids = job_id;
			sort(job_ids.begin(), job_ids.end());
			seq_mach1 = seq1;
			seq_mach2 = seq2;
			cost = numeric_limits<unsigned int>::max();
		}


		~Solution() {}
};


tuple<vector<Job>, unsigned int, unsigned int> read_from_file(string filename) {

	/* filename - nazwa pliku wejsciowego */

	ifstream in_file(filename, ios::in);
	string line; // string przechowujacy aktualna linie pliku
	vector<unsigned int> parameters; // wektor przechowujacy parametry instancji: liczbe zadan, parametr tau, dlugosc trwania dziury
	vector<Job> jobs; // wektor przechowujacy zadania
	unsigned int jobs_number, tau, tau_duration; // zmienne przechowujace parametry instancji

	// odczyt z pliku

	if (in_file.is_open()) {

		// obsluga pierwszej linii - parametrow instancji: liczby zadan, parametru tau, dlugosci trwania dziury

		getline(in_file, line);
		istringstream iss(line);
		string variable;
		while (iss >> variable) {
			parameters.push_back(stoul(variable.c_str()));
		}
		jobs_number = parameters[0];
		tau = parameters[1];
		tau_duration = parameters[2];

		// obsluga pozostalych linii - czasu trwania operacji poszczegolnych zadan

		unsigned int job_id = 1;
		while (getline(in_file, line)) {
			istringstream iss(line);
			vector<unsigned int> durations;
			while (iss >> variable) {
				durations.push_back(stoul(variable.c_str()));
			}
			Job job(job_id, durations[0], durations[1]);
			jobs.push_back(job);
			job_id++;
		}

		in_file.close();
	}
	else {
		cout << "CANNOT OPEN FILE!!!" << endl;
	}

	return make_tuple(jobs, tau, tau_duration); // krotka zawierajaca interesujace nas parametry
}


void generator(string filename, unsigned int jobs, unsigned int op1_max_duration, unsigned int tau, unsigned int tau_duration) {

	/* filename - nazwa pliku wyjsciowego z wygenerowana instancja
	   jobs - liczba zadan w instancji 
	   op1_max_duration - maksymalny czas trwania operacji pierwszej
	   tau - parametr tau - maksymalna odleglosc miedzy kolejnymi dziurami
	   tau_duration - czas trwania dziury */
	
	vector<unsigned int> op1_durations; // wektor przechowujacy czasy trwania pierwszych operacji
	vector<unsigned int> op2_durations; // wektor przechowujacy czasy trwania drugich operacji

	// losowanie czasow trwania operacji

	for (unsigned int i = 0; i < jobs; i++) {
		unsigned int op1_dur = rand() % op1_max_duration + 1;
		unsigned int op2_dur = rand() % tau + 1; // kazda druga operacja musi byc tak samo dluga lub krotsza od tau
		op1_durations.push_back(op1_dur);
		op2_durations.push_back(op2_dur);
	}
	
	// zapis do pliku

	ofstream out_file(filename, ios::out);
	if (out_file.is_open()) {
		out_file << jobs << " " << tau << " " << tau_duration << endl;

		for (unsigned int i = 0; i < jobs; i++) {
			out_file << op1_durations[i] << " " << op2_durations[i] << endl;
		}

		out_file.close();
	}
	else {
		cout << "CANNOT OPEN FILE!!!" << endl;
	}
}


void initialize_stack(stack<Solution> &solutions, vector<Job> jobs) {

	/* Funkcja zapelnia stos poczatkowymi mozliwymi rozwiazaniami:
	   na obu maszynach po jednej operacji nalezacej do tego samego zadania

	   solutions - stos przechowujacy przestrzen rozwiazan
	   jobs - zadania wystepujace w instancji
	   sol_id - liczba porzadkowa danego rozwiazania */

	for (unsigned int i = 0; i < jobs.size(); i++) {
		vector<Job> sequence;
		vector<unsigned int> job_id;
		sequence.push_back(jobs[i]);
		job_id.push_back(jobs[i].job_id);
		Solution solution(job_id, sequence, sequence);
		solutions.push(solution);
		//sol_id++;
	}
}

bool compare_mach2(tuple<int, unsigned int, unsigned int> i, tuple<int, unsigned int, unsigned int> j) {
	return(get<1>(i) < get<1>(j));
}


unsigned int schedule_and_evaluate(Solution &schedule, unsigned int n_jobs, unsigned int tau, unsigned int tau_duration) {

	if (schedule.mach1.size() != n_jobs) { // jezeli nie mamy do czynienia z pelnym, uszeregowanym rozwiazaniem

		unsigned int ready_time_op1 = 0; // zmienna przechowujaca aktualny koniec uszeregowania na pierwszej maszynie
		vector<pair<unsigned int, unsigned int>> ready_times_op2; // wektor przechowujacy mozliwie najwczesniejsze momenty rozpoczecia drugich operacji
		unsigned int loop_times = 0;

		// Szeregowanie pierwszych operacji na pierwszej maszynie

		for (unsigned int i = 0; i < schedule.seq_mach1.size(); i++) { // dla kazdego zadania w sekwencji
			Job current_job = schedule.seq_mach1[i]; // pobierz to zadanie
			auto assignment = make_tuple(current_job.job_id, ready_time_op1, ready_time_op1 + current_job.operation1);
			schedule.mach1.push_back(assignment); // przypisz je na pierwsza maszyne
			ready_time_op1 += current_job.operation1; // zaktualizuj dlugosc uszeregowania
			auto ready_time_op2 = make_pair(current_job.job_id, ready_time_op1);
			ready_times_op2.push_back(ready_time_op2); // przechowaj mozliwy moment rozpoczecia drugiej operacji
		}

		// Szeregowanie drugich operacji na drugiej maszynie (BEZ DZIUR)

		unsigned int mach2_end = 0; // zmienna przechowujaca dlugosc uszeregowania na drugiej maszynie

		for (unsigned int i = 0; i < schedule.seq_mach2.size(); i++) { // dla kazdego zadanie w sekwencji
			Job current_job = schedule.seq_mach2[i]; // pobierz to zadanie
			unsigned int ready_time_op2; // zmienna przechowujaca czas gotowsci drugiej operacji

			for (unsigned int j = 0; j < ready_times_op2.size(); j++) // znajdz czas gotowsci drugiej operacji aktualnego zadania
				if (ready_times_op2[j].first == current_job.job_id)
					ready_time_op2 = ready_times_op2[j].second;

			if (i == 0) { // jezeli szeregujesz pierwsza operacje na maszynine
				auto assignment = make_tuple(current_job.job_id, ready_time_op2, ready_time_op2 + current_job.operation2);
				schedule.mach2.push_back(assignment); // to wstaw ja po prostu w odpowiednim miejscu
				mach2_end = ready_time_op2 + current_job.operation2; // zaktualizuj dlugosc uszeregowania
			}
			else { // w przeciwnym przypadku
				if (ready_time_op2 > mach2_end) { // jezeli zadanie nie moze sie rozpoczac bezposrednio po poprzednim; powstaje okres bezczynnosci
					auto assignment = make_tuple(current_job.job_id, ready_time_op2, ready_time_op2 + current_job.operation2);
					schedule.mach2.push_back(assignment); // przypisz je w odpowiednim miejscu
					mach2_end = ready_time_op2 + current_job.operation2; // odpowiednio zaktualizuj dlugosc uszeregowania
				}
				else { // w przeciwnym przypadku
					auto assignment = make_tuple(current_job.job_id, mach2_end, mach2_end + current_job.operation2);
					schedule.mach2.push_back(assignment); // przypisz zaraz za poprzednim zadaniem
					mach2_end += current_job.operation2;
				}
			}
		}


		// Utworzenie dziur

		mach2_end = get<2>(schedule.mach2.back());
		unsigned int n_holes = mach2_end / tau; // ustalenie ile dziur bedzie potrzebnych (na wyrost)
		unsigned int start = tau; // poczatek pierwszej dziury

		for (unsigned int i = 0; i < n_holes; i++) { // utworzenie dziur oddzielonych od siebie o dlugosc tau
			auto new_hole = make_tuple(i + 1, start, start + tau_duration);
			schedule.holes.push_back(new_hole);
			start = start + tau_duration + tau;
		}

		// Obsluga dziur

		if (schedule.holes.size() > 0) { // jezeli sa jakiekolwiek dziury

			// Przesuwanie operacji w stosunku do dziur

			for (unsigned int i = 0; i < schedule.mach2.size(); i++) { // dla kazdego zadania
				for (unsigned int j = 0; j < schedule.holes.size(); j++) { // i dla kazdej dziury
					if (!(get<2>(schedule.mach2[i]) <= get<1>(schedule.holes[j]) || (get<1>(schedule.mach2[i]) >= get<2>(schedule.holes[j])))) { // jezeli nachodza na siebie
						unsigned int diff = get<2>(schedule.holes[j]) - get<1>(schedule.mach2[i]); // oblicz o ile trzeba przesunac w prawo
						for (unsigned int k = i; k < schedule.mach2.size(); k++) { // przesun kazde zadanie na prawo o te wartosc
							get<1>(schedule.mach2[k]) += diff;
							get<2>(schedule.mach2[k]) += diff;
						}
					}
				}
			}

			// Tymczasowe wstawianie dziur na 2 maszyne

			vector<tuple<int, unsigned int, unsigned int>> mach2_holes = schedule.mach2; // utworz nowy wektor przechowujacy dodatkowo dziury
			vector<bool> done_holes; // wektor pomocniczy sprawdzajacy, ktore dziury juz zostaly obsluzone
			for (unsigned int i = 0; i < schedule.holes.size(); i++)
				done_holes.push_back(false);

			for (unsigned int i = 0; i < schedule.mach2.size(); i++) { // dla kazdego zadania na 2 maszynie
				for (unsigned int j = 0; j < schedule.holes.size(); j++) { // dla kazdej dziury
					if (done_holes[j] != true) { // jezeli nie zostala obsluzona
						if ((get<1>(schedule.holes[j]) < get<1>(schedule.mach2[i])) && (get<2>(schedule.holes[j]) <= get<1>(schedule.mach2[i]))) { // i jezeli powinna byc przed obslugiwanym zadaniem
							int hole_id = get<0>(schedule.holes[j]);
							hole_id = -hole_id;
							auto new_hole = make_tuple(hole_id, get<1>(schedule.holes[j]), get<2>(schedule.holes[j]));
							mach2_holes.insert(mach2_holes.begin() + j, new_hole); // to ja wstaw z ujemnym indeksem
							done_holes[j] = true; // dziura obsluzona
						}
					}
				}
			}

			schedule.mach2 = mach2_holes;
			sort(schedule.mach2.begin(), schedule.mach2.end(), compare_mach2); // posortuj 2 maszyne pod wzgledem czasow rozpoczec

			// Usuwanie przerw - "doklejanie do lewa"

			for (unsigned int i = 0; i < schedule.mach2.size(); i++) { // dla kazdego tworu na maszynie (dziury czy operacji)
				if (get<0>(schedule.mach2[i]) > 0) { // jezeli jest to operacja
					unsigned int ready_time; // zmienna przechowujaca jego "potencjalny" czas gotowosci

					for (unsigned int j = 0; j < ready_times_op2.size(); j++) // znajdz ten czas
						if (ready_times_op2[j].first == get<0>(schedule.mach2[i])) {
							ready_time = ready_times_op2[j].second;
							break;
						}

					if (get<1>(schedule.mach2[i]) > ready_time) { // jezeli operacja zaczyna sie pozniej niz jej "potencjalny" czas gotowosci
						unsigned int diff = get<1>(schedule.mach2[i]) - ready_time; // oblicz o ile by trzeba ja przesunac
						if (get<0>(schedule.mach2[i - 1]) < 0) { // jezeli poprzedni twor to dziura (tylko w przypadku dziur mozliwe sa przerwy)
							unsigned int space; // zmienna przechowujaca dostepne miejsce
							if (i - 1 == 0) { // jezeli jest to pierwszy twor na maszynie
								space = get<1>(schedule.mach2[i - 1]); // to dostepne miejsce to czas rozpoczecia tej dziury
							}
							else {
								space = get<1>(schedule.mach2[i - 1]) - get<2>(schedule.mach2[i - 2]); // lub odpowiednia roznica dwoch poprzednich tworow
							}
							unsigned int correction = min(diff, space); // mozliwa korekta jest minimum z diff i space
							if(correction > 0){
							get<1>(schedule.mach2[i - 1]) -= correction;
							get<2>(schedule.mach2[i - 1]) -= correction; // wprowadzamy korekte dla dziury
							int id = abs(get<0>(schedule.mach2[i - 1]));
							get<1>(schedule.holes[id - 1]) = get<1>(schedule.mach2[i - 1]); // wprowadzamy korekte na wektorze przechowujacym dziury
							get<2>(schedule.holes[id - 1]) = get<2>(schedule.mach2[i - 1]); // wprowadzamy korekte na wektorze przechowujacym dziury
							get<1>(schedule.mach2[i]) -= correction;
							get<2>(schedule.mach2[i]) -= correction; // wprowadzamy korekte do samej operacji "podlaczanej" do tej dziury
							}
						}
					}
				}
			}

			for (unsigned int i = 0; i < schedule.mach2.size() - 1; i++) {
				if (get<0>(schedule.mach2[i]) < 0 && get<0>(schedule.mach2[i + 1]) < 0)
					loop_times++;
			}

			// Sprowadzenie 2 maszyny do samych operacji

			vector<tuple<int, unsigned int, unsigned int>> mach2;

			for (unsigned int i = 0; i < schedule.mach2.size(); i++) {
				if (get<0>(schedule.mach2[i]) > 0) {
					mach2.push_back(schedule.mach2[i]);
				}
			}
			schedule.mach2 = mach2;

			// Usuniecie nadmiaru dziur

			for (unsigned int i = 0; i < schedule.holes.size(); i++) {
				if (get<1>(schedule.holes[i]) >= get<2>(schedule.mach2.back())) {
					schedule.holes.erase(schedule.holes.begin() + i, schedule.holes.end());
					break;
				}
			}

			// Naprawa odleglosci pomiedzy dziurami (o ile jakies zostaly)

			if (schedule.holes.size() > 0) {
				for (unsigned int i = 0; i < schedule.holes.size() - 1; i++) {
					unsigned int diff = get<1>(schedule.holes[i + 1]) - get<2>(schedule.holes[i]);
					if (diff > tau) {
						unsigned int correction = diff - tau;
						get<1>(schedule.holes[i + 1]) -= correction;
						get<2>(schedule.holes[i + 1]) -= correction;
					}
				}
			}

			for (unsigned int p = 0; p < loop_times; p++) {

				vector<tuple<int, unsigned int, unsigned int>> mach2_holes = schedule.mach2; // utworz nowy wektor przechowujacy dodatkowo dziury
				vector<bool> done_holes; // wektor pomocniczy sprawdzajacy, ktore dziury juz zostaly obsluzone
				for (unsigned int i = 0; i < schedule.holes.size(); i++)
					done_holes.push_back(false);

				for (unsigned int i = 0; i < schedule.mach2.size(); i++) { // dla kazdego zadania na 2 maszynie
					for (unsigned int j = 0; j < schedule.holes.size(); j++) { // dla kazdej dziury
						if (done_holes[j] != true) { // jezeli nie zostala obsluzona
							if ((get<1>(schedule.holes[j]) < get<1>(schedule.mach2[i])) && (get<2>(schedule.holes[j]) <= get<1>(schedule.mach2[i]))) { // i jezeli powinna byc przed obslugiwanym zadaniem
								int hole_id = get<0>(schedule.holes[j]);
								hole_id = -hole_id;
								auto new_hole = make_tuple(hole_id, get<1>(schedule.holes[j]), get<2>(schedule.holes[j]));
								mach2_holes.insert(mach2_holes.begin() + j, new_hole); // to ja wstaw z ujemnym indeksem
								done_holes[j] = true; // dziura obsluzona
							}
						}
					}
				}

				schedule.mach2 = mach2_holes;
				sort(schedule.mach2.begin(), schedule.mach2.end(), compare_mach2); // posortuj 2 maszyne pod wzgledem czasow rozpoczec

				// Usuwanie przerw - "doklejanie do lewa"

				for (unsigned int i = 0; i < schedule.mach2.size(); i++) { // dla kazdego tworu na maszynie (dziury czy operacji)
					if (get<0>(schedule.mach2[i]) > 0) { // jezeli jest to operacja
						unsigned int ready_time; // zmienna przechowujaca jego "potencjalny" czas gotowosci

						for (unsigned int j = 0; j < ready_times_op2.size(); j++) // znajdz ten czas
							if (ready_times_op2[j].first == get<0>(schedule.mach2[i])) {
								ready_time = ready_times_op2[j].second;
								break;
							}

						if (get<1>(schedule.mach2[i]) > ready_time) { // jezeli operacja zaczyna sie pozniej niz jej "potencjalny" czas gotowosci
							unsigned int diff = get<1>(schedule.mach2[i]) - ready_time; // oblicz o ile by trzeba ja przesunac
							if (get<0>(schedule.mach2[i - 1]) < 0) { // jezeli poprzedni twor to dziura (tylko w przypadku dziur mozliwe sa przerwy)
								unsigned int space; // zmienna przechowujaca dostepne miejsce
								if (i - 1 == 0) { // jezeli jest to pierwszy twor na maszynie
									space = get<1>(schedule.mach2[i - 1]); // to dostepne miejsce to czas rozpoczecia tej dziury
								}
								else {
									space = get<1>(schedule.mach2[i - 1]) - get<2>(schedule.mach2[i - 2]); // lub odpowiednia roznica dwoch poprzednich tworow
								}
								unsigned int correction = min(diff, space); // mozliwa korekta jest minimum z diff i space
								if (correction > 0) {
									get<1>(schedule.mach2[i - 1]) -= correction;
									get<2>(schedule.mach2[i - 1]) -= correction; // wprowadzamy korekte dla dziury
									int id = abs(get<0>(schedule.mach2[i - 1]));
									get<1>(schedule.holes[id - 1]) = get<1>(schedule.mach2[i - 1]); // wprowadzamy korekte na wektorze przechowujacym dziury
									get<2>(schedule.holes[id - 1]) = get<2>(schedule.mach2[i - 1]); // wprowadzamy korekte na wektorze przechowujacym dziury
									get<1>(schedule.mach2[i]) -= correction;
									get<2>(schedule.mach2[i]) -= correction; // wprowadzamy korekte do samej operacji "podlaczanej" do tej dziury
								}
							}
						}
					}
				}

				// Sprowadzenie 2 maszyny do samych operacji

				vector<tuple<int, unsigned int, unsigned int>> mach2;

				for (unsigned int i = 0; i < schedule.mach2.size(); i++) {
					if (get<0>(schedule.mach2[i]) > 0) {
						mach2.push_back(schedule.mach2[i]);
					}
				}
				schedule.mach2 = mach2;

				for (unsigned int i = 0; i < schedule.holes.size() - 1; i++) {
					unsigned int diff = get<1>(schedule.holes[i + 1]) - get<2>(schedule.holes[i]);
					if (diff > tau) {
						unsigned int correction = diff - tau;
						get<1>(schedule.holes[i + 1]) -= correction;
						get<2>(schedule.holes[i + 1]) -= correction;
					}
				}
			}
		}

		// Liczenie wartosci funkcji

		unsigned int value = 0;

		for (unsigned int i = 0; i < schedule.mach2.size(); i++)
			value += get<2>(schedule.mach2[i]);

		return value;
	}
	else { // w przeciwnym przypadku oblicz tylko wartosc funkcji celu
		unsigned int value = 0;

		for (unsigned int i = 0; i < schedule.mach2.size(); i++)
			value += get<2>(schedule.mach2[i]);

		return value;
	}
}

void display_solution(Solution best_solution) {
	//cout << "Solution ID: " << best_solution.solution_id << endl;
	cout << "Wartosc funkcji: " << best_solution.cost << endl << endl << "Sekwencja na 1 maszynie: " << endl << endl;;

	for (unsigned int i = 0; i < best_solution.seq_mach1.size(); i++)
		cout << "Zadanie " << get<0>(best_solution.mach1[i]) << " Start: " << get<1>(best_solution.mach1[i]) << " Koniec: " << get<2>(best_solution.mach1[i]) << endl;

	cout << endl << endl << "Sekwencja na 2 maszynie: " << endl;

	for (unsigned int i = 0; i < best_solution.mach2.size(); i++)
		cout << "Zadanie " << get<0>(best_solution.mach2[i]) << " Start: " << get<1>(best_solution.mach2[i]) << " Koniec: " << get<2>(best_solution.mach2[i]) << endl;

	cout << endl << endl << "Sekwencja dziur na 2 maszynie: " << endl;

	for (unsigned int i = 0; i < best_solution.holes.size(); i++)
		cout << "Dziura " << get<0>(best_solution.holes[i]) << " Start: " << get<1>(best_solution.holes[i]) << " Koniec: " << get<2>(best_solution.holes[i]) << endl;
}

void write_to_file(string filename, Solution best_solution, double time) {

	ofstream out_file;
	out_file.open(filename, ios::out);

	if (out_file.is_open()) {
		out_file << best_solution.cost << " " << time << endl;

		for (unsigned int i = 0; i < best_solution.mach1.size(); i++)
			out_file << get<0>(best_solution.mach1[i]) << " " << get<1>(best_solution.mach1[i]) << " " << get<2>(best_solution.mach1[i]) << endl;

		for(unsigned int i = 0; i < best_solution.mach2.size(); i++)
			out_file << get<0>(best_solution.mach2[i]) << " " << get<1>(best_solution.mach2[i]) << " " << get<2>(best_solution.mach2[i]) << endl;

		for (unsigned int i = 0; i < best_solution.holes.size(); i++)
			out_file << get<1>(best_solution.holes[i]) << " " << get<2>(best_solution.holes[i]) << endl;

		out_file.close();
	}
	else
		cout << "CANNOT OPEN FILE!!!" << endl;

}

int main()
{
	srand(time(NULL)); // inicjalizacja "random seed"

	/*for (unsigned int i = 1; i <= 10; i++) {
		stringstream ss;
		ss << "ins" << i << "_5_4_2.txt";
		string filename = ss.str();
		generator(filename, 5, 15, 4, 2);
	}*/

	for (unsigned int f = 1; f <= 10; f++) {

		stringstream in_ss;
		in_ss << "ins" << f << "_5_4_2.txt";
		string in_filename = in_ss.str();

		auto test = read_from_file(in_filename); // odczyt instancji z pliku
		vector<Job> jobs = get<0>(test); // wektor przechowujacy zadania nalezace do instancji
		vector<unsigned int> job_ids; // wektor przechowujacy liczby porzadkowe zadan

		for (unsigned int i = 0; i < jobs.size(); i++)
			job_ids.push_back(jobs[i].job_id);

		unsigned int tau = get<1>(test); // parametr tau
		unsigned int tau_duration = get<2>(test); // czas trwania dziury
		//unsigned long long solution_id = 1; // zmienna przechowujaca aktualna liczbe porzadkowa rozwiazania

		stack<Solution> solutions; // stos przechowujacy przestrzen rozwiazan
		initialize_stack(solutions, jobs); // zapelnienie stosu pierwszymi rozwiazaniami

		unsigned int upper_bound = numeric_limits<unsigned int>::max(); // poczatkowy upper bound rowna sie nieskonczonosci
		Solution best_solution; // najlepsze rozwiazanie

		// PROCEDURA BRANCH AND BOUND 

		auto start = chrono::high_resolution_clock::now();
		while (!solutions.empty()) { // dopoki stos nie jest pusty: dopoki istnieje mozliwosc znalezienia lepszego rozwiazania
			Solution current_solution = solutions.top(); // pobierz rozwiazanie ze stosu
			solutions.pop(); // i je usun
			if (current_solution.seq_mach1.size() == jobs.size()) { // jesli uszeregowane sa wszystkie zadania
				current_solution.cost = schedule_and_evaluate(current_solution, jobs.size(), tau, tau_duration); // oblicz wartosc funkcji celu
				if (current_solution.cost < upper_bound) { // i rozwiazanie jest lepsze od poprzedniego
					best_solution = current_solution; // zapisz je jako najlepsze
					upper_bound = best_solution.cost; // uaktualnij wartosc upper bound
				}
			}
			else { // w przeciwnym przypadku
				vector<unsigned int> diff; // wektor przechowujacy brakujace zadania w obslugiwanym rozwiazaniu
				vector<unsigned int> current_job_ids = current_solution.job_ids; // wektor przechowujacy zadania obecne w obslugiwanym rozwiazaniu
				set_difference(job_ids.begin(), job_ids.end(), current_job_ids.begin(), current_job_ids.end(), inserter(diff, diff.begin())); // roznica miedzy nimi
				for (unsigned int i = 0; i < diff.size(); i++) { // dla kazdego brakujacego zadania w obslugiwanym rozwiazaniu
					vector<unsigned int> new_job_ids = current_job_ids; // utworz wektor przechowujacy liczby porzadkowe nowych zadan
					new_job_ids.push_back(diff[i]); // odpowiednio go uzupelnij
					vector<Job> new_seq_mach1 = current_solution.seq_mach1; // utworz wektor przechowujacy nowa sekwencje pierwszych operacji
					new_seq_mach1.push_back(jobs[diff[i] - 1]); // dodaj na jego koncu nowe zadanie

					for (unsigned int j = 0; j < new_seq_mach1.size(); j++) { // dla kazdej mozliwej permutacji na DRUGIEJ maszynie
						vector<Job> new_seq_mach2 = current_solution.seq_mach2; // utworz wektor przechowujacy te permutacje
						Job inserted = jobs[diff[i] - 1]; // wybierz odpowiednie zadanie do wstawienia
						new_seq_mach2.insert(new_seq_mach2.begin() + j, inserted); // wstaw w odpowiednim miejscu
						Solution new_solution(new_job_ids, new_seq_mach1, new_seq_mach2); // utworz nowe rozwiazanie
						//solution_id++;
						new_solution.cost = schedule_and_evaluate(new_solution, jobs.size(), tau, tau_duration);
						if (new_solution.cost > upper_bound)
							continue;
						else
							solutions.push(new_solution); // albo umiesc na stosie
					}
				}
			}

		}
		auto finish = chrono::high_resolution_clock::now();
		chrono::duration<double> elapsed = finish - start;
		double time = elapsed.count();
		stringstream out_ss;
		out_ss << "out" << f << "_5_4_2.txt";
		string out_filename = out_ss.str();
		write_to_file(out_filename, best_solution, time);

		//cout << time << " sekund" << endl << endl;
		//display_solution(best_solution);
		cout << in_filename << " " << time << endl;
		//cout << endl << endl;
	}

	return 0;
}

// Uruchomienie programu: Ctrl + F5 lub menu Debugowanie > Uruchom bez debugowania
// Debugowanie programu: F5 lub menu Debugowanie > Rozpocznij debugowanie

// Porady dotyczące rozpoczynania pracy:
//   1. Użyj okna Eksploratora rozwiązań, aby dodać pliki i zarządzać nimi
//   2. Użyj okna programu Team Explorer, aby nawiązać połączenie z kontrolą źródła
//   3. Użyj okna Dane wyjściowe, aby sprawdzić dane wyjściowe kompilacji i inne komunikaty
//   4. Użyj okna Lista błędów, aby zobaczyć błędy
//   5. Wybierz pozycję Projekt > Dodaj nowy element, aby utworzyć nowe pliki kodu, lub wybierz pozycję Projekt > Dodaj istniejący element, aby dodać istniejące pliku kodu do projektu
//   6. Aby w przyszłości ponownie otworzyć ten projekt, przejdź do pozycji Plik > Otwórz > Projekt i wybierz plik sln

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
#include <direct.h>

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

	ofstream out_file(filename, ios::out);

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
		cout << "CANNOT OPEN FILE!!! (Write To File)" << endl;

}


vector<unsigned int> mutate(vector<unsigned int> chromosome) {
	unsigned int gene1 = 0, gene2 = 0;
	while (gene1 == gene2) {
		gene1 = rand() % chromosome.size();
		gene2 = rand() % chromosome.size();
	}
	iter_swap(chromosome.begin() + gene1, chromosome.begin() + gene2);
	return chromosome;
}


vector<vector<unsigned int>> cross(vector<unsigned int> chromosome1, vector<unsigned int> chromosome2, vector<unsigned int> jobs_ids) {
	unsigned int cross_point = (rand() % (chromosome1.size() - 2)) + 1;


	vector<unsigned int> tmp_chromosome1, tmp_chromosome2, chromosome3, chromosome4, tmp_chromosome3, tmp_chromosome4;
	vector<unsigned int> diff1, diff2; // wektor przechowujacy brakujace zadania w obslugiwanym rozwiazaniu
	vector<unsigned int> current_job_ids = chromosome1; // wektor przechowujacy zadania obecne w obslugiwanym rozwiazaniu


	// kopiowanie poczat
	for (unsigned int i = 0; i < cross_point; i++)
	{
		chromosome3.push_back(chromosome1[i]);
		chromosome4.push_back(chromosome2[i]);
	}



	//Kopie chromosomow
	//tmp_chromosome1 = chromosome1;
	//tmp_chromosome2 = chromosome2;
	tmp_chromosome3 = chromosome3;
	tmp_chromosome4 = chromosome4;

	//sort(tmp_chromosome1.begin(), tmp_chromosome1.end());
	//sort(tmp_chromosome2.begin(), tmp_chromosome2.end());
	sort(tmp_chromosome3.begin(), tmp_chromosome3.end());
	sort(tmp_chromosome4.begin(), tmp_chromosome4.end());



	set_difference(jobs_ids.begin(), jobs_ids.end(), tmp_chromosome3.begin(), tmp_chromosome3.end(), inserter(diff1, diff1.begin())); // roznica miedzy nimi
	set_difference(jobs_ids.begin(), jobs_ids.end(), tmp_chromosome4.begin(), tmp_chromosome4.end(), inserter(diff2, diff2.begin())); // roznica miedzy nimi



	int next_job;
	int index_job;
	vector<unsigned int>::iterator it;
	vector<tuple<int, int>> next_jobs; // zadanie, jego indeks
	tuple<int, int> job_tuple;


	// CHROMOSOM 3
	// tworzymy wektor par: zadanie, pozycja w starym chromosomie
	for (int i = 0; i < diff1.size(); i++) {
		next_job = diff1[i];
		it = find(chromosome2.begin(), chromosome2.end(), next_job);
		index_job = distance(chromosome2.begin(), it);
		job_tuple = make_tuple(next_job, index_job);
		next_jobs.push_back(job_tuple);

	}

	// szukamy najmniejszych indeksow
	sort(begin(next_jobs), end(next_jobs), [](auto const &t1, auto const &t2) {
		return get<1>(t1) < get<1>(t2); // or use a custom compare function
	});

	for (int i = 0; i < diff1.size(); i++) {
		chromosome3.push_back(get<0>(next_jobs[i]));
	}

	next_jobs.clear();

	// CHROMOSOM 4
	// tworzymy wektor par: zadanie, pozycja w starym chromosomie
	for (int i = 0; i < diff2.size(); i++) {
		next_job = diff2[i];
		it = find(chromosome1.begin(), chromosome1.end(), next_job);
		index_job = distance(chromosome1.begin(), it);
		job_tuple = make_tuple(next_job, index_job);
		next_jobs.push_back(job_tuple);
	}
	// szukamy najmniejszych indeksow
	sort(begin(next_jobs), end(next_jobs), [](auto const &t1, auto const &t2) {
		return get<1>(t1) < get<1>(t2);
	});

	for (int i = 0; i < diff2.size(); i++) {
		chromosome4.push_back(get<0>(next_jobs[i]));
	}

	next_jobs.clear();

	vector<vector<unsigned int>> chromosomes;
	chromosomes.push_back(chromosome3);
	chromosomes.push_back(chromosome4);
	return chromosomes;
}


vector<vector<unsigned int>> competition(vector<vector<unsigned int>> population, unsigned int after_size, unsigned int group_size, vector<Job> jobs, unsigned int tau, unsigned int tau_duration) {
	vector<vector<unsigned int>> after_population;
	vector<unsigned int> group_inds;
	vector<unsigned int> population_scores;
	unsigned int competitor_ind;
	unsigned int best_score = numeric_limits<unsigned int>::max();
	unsigned int best_competitor_ind = 0;

	// f celu dla zawodnikow
	Solution current_solution;
	Solution empty_solution;
	for (int i = 0; i < population.size(); i++) {
		for (int j = 0; j < jobs.size(); j++) {
			current_solution.seq_mach1.push_back(jobs[population[i][j] - 1]);
			//cout << "i: " << i << "\tj: " << j << "\twartosc: " << population[i][j] << endl;
		}
		current_solution.seq_mach2 = current_solution.seq_mach1;
		population_scores.push_back(schedule_and_evaluate(current_solution, jobs.size(), tau, tau_duration));
		current_solution = empty_solution;
	}

	vector<unsigned int>::iterator result = min_element(begin(population_scores), end(population_scores));
	int index = distance(population_scores.begin(), result);


	vector<int> population_inds;
	vector<int> available_population_inds;
	for (int i = 0; i < population.size(); i++) {
		population_inds.push_back(i);
	}

	bool checker = true;

	for (int i = 0; i < after_size; i++) { // tak dlugo jak nie mamy wszystkich zwyciezcow
		available_population_inds = population_inds;
		for (int j = 0; j < group_size; j++) { // losowanie indexow zawodnikow
			while (checker) {					// tak dlugo jak nie wylosujemy indexu ktorego jeszcze nie ma w grupie
				competitor_ind = (rand() % population.size());  // losujemy index
				if (available_population_inds[competitor_ind] >= 0) { // jesli jest dostepny
					available_population_inds[competitor_ind] = -1;		// zmieniamy go na niedostepny
					group_inds.push_back(competitor_ind);
					checker = false;										// wychodzimy z petli
				}
			}

			checker = true;
			if (population_scores[competitor_ind] < best_score) { // sprawdzanie czy wylosowany jest lepszy od najlepszego w grupie
				best_score = population_scores[competitor_ind];
				best_competitor_ind = competitor_ind;
			}
		}
		after_population.push_back(population[best_competitor_ind]);
		best_score = numeric_limits<unsigned int>::max();
		best_competitor_ind = 0;
	}
	return after_population; // populacja po selekcji o rozmiarze = after_size
}


int main()
{
	srand(time(NULL)); // inicjalizacja "random seed"

		unsigned int algorithm = 1; // A TU SE DALEM SWOJ ALGORYTM
		//cout << "Wybierz sposob rozwiazania (wpisz jego numer):" << endl << "1. Algorytm dokladny" << endl << "2. Algorytm heurystyczny" << endl;
		//cout << "Wybor: ";
		//cin >> algorythm;

		if (algorithm == 1) {

			vector<unsigned int> number_of_jobs{ 5, 10, 15, 20 };
			vector<unsigned int> tau_parameters{ 4, 10, 15 };
			vector<unsigned int> hole_durations{ 2, 6, 10 };
			ofstream done_file;

			//string base_dir = "C:\\Users\\Jędrzej\\Desktop\\Studium_Naturae\\Zaawansowane_Metody_Optymalizacji\\BranchAndBound\\BranchAndBound\\";

			for (unsigned int noj_ind = 0; noj_ind < number_of_jobs.size(); noj_ind++) {
				for (unsigned int tp_ind = 0; tp_ind < tau_parameters.size(); tp_ind++) {
					for (unsigned int hd_ind = 0; hd_ind < hole_durations.size(); hd_ind++) {

						unsigned int noj = number_of_jobs[noj_ind];
						unsigned int tp = tau_parameters[tp_ind];
						unsigned int hd = hole_durations[hd_ind];

						stringstream folder_path_ss;
						folder_path_ss << noj << "_" << tp << "_" << hd;
						string folder_path = folder_path_ss.str();
						_mkdir(folder_path.c_str());


							for (unsigned int i = 1; i <= 10; i++) {
								stringstream ss;
								ss << folder_path << "\\ins" << i << "_" << noj << "_" << tp << "_" << hd << ".txt";
								string filename = ss.str();
								generator(filename, noj, 15, tp, hd);
							}

						for (unsigned int f = 1; f <= 10; f++) {

							stringstream in_ss;
							in_ss << folder_path << "\\ins" << f << "_" << noj << "_" << tp << "_" << hd << ".txt";
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
							out_ss << folder_path << "\\out" << f << "_" << noj << "_" << tp << "_" << hd << ".txt";
							string out_filename = out_ss.str();
							write_to_file(out_filename, best_solution, time);

							//cout << time << " sekund" << endl << endl;
							//display_solution(best_solution);
							//cout << in_filename << " " << out_filename << " " << time << endl;
							//cout << endl << endl;
						}
						cout << "Skonczone testy " << "Zadania: " << noj << " Tau: " << tp << " Dziura: " << hd << endl;
						done_file.open("DONE.txt", ios::app);
						done_file << noj << "_" << tp << "_" << hd << endl;
						done_file.close();
					}
				}
			}
		}
		else if (algorithm == 2) {
			cout << "Wybrales algorytm heurystyczny. Doskonala decyzja!" << endl;

			auto test = read_from_file("testowa1.txt"); // odczyt instancji z pliku // TU DAŁEM TESTOWĄ1 COBY SIĘ KOMPILOWAŁO
			vector<Job> jobs = get<0>(test); // wektor przechowujacy zadania nalezace do instancji
			vector<unsigned int> job_ids; // wektor przechowujacy liczby porzadkowe zadan

			for (unsigned int i = 0; i < jobs.size(); i++)
				job_ids.push_back(jobs[i].job_id);

			unsigned int tau = get<1>(test); // parametr tau
			unsigned int tau_duration = get<2>(test); // czas trwania dziury

			Solution best_solution; // najlepsze rozwiazanie

			unsigned int init_pop_size = 150;
			unsigned int pop_size = 100;
			unsigned int growth = init_pop_size - pop_size;
			unsigned int competition_group_size = 10;
			int n_iteration = 200;

			float fraction_cross = 0.5;
			int n_cross = ceil(fraction_cross * growth) / 2;
			int n_mutation = growth - n_cross * 2;
			vector<vector<unsigned int >> crossed_chromosomes;

			vector<unsigned int> chromosome;
			vector<vector<unsigned int>> population;

			// tworzenie inicjujacej duzej populacji
			for (unsigned int i = 0; i < init_pop_size; i++) {
				chromosome = job_ids;
				random_shuffle(chromosome.begin(), chromosome.end());
				population.push_back(chromosome);
			}


			// wypisywanie duzej populacji
			cout << "Populacja przed selekcja:" << endl;
			for (unsigned int i = 0; i < init_pop_size; i++) {
				cout << i << ":  ";
				for (unsigned int j = 0; j < jobs.size(); j++) {
					cout << population[i][j] << " ";
				}
				cout << endl;
			}
			cout << endl << endl;


			for (int i = 0; i < n_iteration; i++) {
				// selekcja
				population = competition(population, pop_size, competition_group_size, jobs, tau, tau_duration);

				// c-o
				for (int j = 0; j < n_cross; j++) {
					crossed_chromosomes = cross(population[(rand() % pop_size)], population[(rand() % pop_size)], job_ids);
					population.push_back(crossed_chromosomes[0]);
					population.push_back(crossed_chromosomes[1]);
				}

				// mutacje 
				for (int j = 0; j < n_mutation; j++) {
					population.push_back(mutate(population[(rand() % pop_size)]));
				}
			}


			// wybor najlepszego
			Solution current_solution;
			Solution empty_solution;
			vector<unsigned int> population_scores;
			unsigned int competitor_ind;
			int score;
			unsigned int best_score = numeric_limits<unsigned int>::max();
			unsigned int best_competitor_ind = 0;

			for (int i = 0; i < population.size(); i++) {
				for (int j = 0; j < jobs.size(); j++) {
					current_solution.seq_mach1.push_back(jobs[population[i][j] - 1]);
					//cout << "i: " << i << "\tj: " << j << "\twartosc: " << population[i][j] << endl;
				}
				//cout << endl;
				current_solution.seq_mach2 = current_solution.seq_mach1;
				score = schedule_and_evaluate(current_solution, jobs.size(), tau, tau_duration);
				if (score < best_score) {
					best_score = score;
					best_solution = current_solution;
					best_solution.cost = best_score;
				}
				current_solution = empty_solution;
			}



			// moze sie przydac do testow
			/*vector<unsigned int> v = { 1,2,5,4,3 };
			for (int j = 0; j < jobs.size(); j++) {
				current_solution.seq_mach1.push_back(jobs[v[j] - 1]);
				//cout << "i: " << i << "\tj: " << j << "\twartosc: " << population[i][j] << endl;
			}
			current_solution.seq_mach2 = current_solution.seq_mach1;
			score = schedule_and_evaluate(current_solution, jobs.size(), tau, tau_duration);
			best_score = score;
			best_solution = current_solution;
			best_solution.cost = best_score;*/
		} else {
			cout << "Nie umiesz nawet wybrac jednej z dwoch cyferek...?" << endl;
			return 0;
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

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

// OGARNAC CZY NA BANK WSZYSTKO CO POTRZEBA TU JEST!!!
class Solution {
	public:
		unsigned int parent_id; // id "rodzica" - potrzebne dla uszeregowania na pierwszej maszynie - zero w przypadku braku
		unsigned int solution_id;
		vector<unsigned int> job_ids; // wektor przechowujacy liczby porzadkowe zadan obecnych w rozwiazaniu
		vector<Job> seq_mach1; // sekwencja operacji na pierwszej maszynie
		vector<Job> seq_mach2; // sekwencja operacji na drugiej maszynie
		vector<tuple<unsigned int, unsigned int, unsigned int>> mach1; // uszeregowanie na pierwszej maszynie: <job_id, start, end>
		vector<tuple<int, unsigned int, unsigned int>> mach2; // uszeregowanie na drugiej maszynie: <?job_id (+) lub hole_id (-)?, start, end>
		vector<tuple<unsigned int, unsigned int, unsigned int>> holes; // wektor przechowujacy dziury na drugiej maszynie: hole_id, start, end
		unsigned int cost; // wartosc naszej funkcji celu

		Solution() {}

		Solution(unsigned int parent, unsigned int sol_id, vector<unsigned int> job_id, vector<Job> seq1, vector<Job> seq2) {
			if (parent != 0)
				parent_id = parent;
			else
				parent_id = 0;
			solution_id = sol_id;
			job_ids = job_id;
			sort(job_ids.begin(), job_ids.end());
			seq_mach1 = seq1;
			seq_mach2 = seq2;
			cost = numeric_limits<unsigned int>::max();
		}


		~Solution() {}
};
// OGARNAC CZY NA BANK WSZYSTKO CO POTRZEBA TU JEST!!!


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


void generator(string filename, unsigned int jobs, unsigned int op1_max_duration, unsigned int tau_max_duration) {

	/* filename - nazwa pliku wyjsciowego z wygenerowana instancja
	   jobs - liczba zadan w instancji 
	   op1_max_duration - maksymalny czas trwania operacji pierwszej
	   tau_max_duration - maksymalny czas trwania dziury */
	
	vector<unsigned int> op1_durations; // wektor przechowujacy czasy trwania pierwszych operacji
	vector<unsigned int> op2_durations; // wektor przechowujacy czasy trwania drugich operacji
	unsigned int tau = op1_max_duration / 2; // parametr tau - maksymalna odleglosc miedzy kolejnymi dziurami
	unsigned int tau_duration = rand() % tau_max_duration + 1; // czas trwania dziury

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


void initialize_stack(stack<Solution> &solutions, vector<Job> jobs, unsigned int &sol_id) {

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
		Solution solution(0, sol_id, job_id, sequence, sequence);
		solutions.push(solution);
		sol_id++;
	}
}

int main()
{
	srand(time(NULL)); // inicjalizacja "random seed"

	auto test = read_from_file("testowa.txt"); // odczyt instancji z pliku
	vector<Job> jobs = get<0>(test); // wektor przechowujacy zadania nalezace do instancji
	vector<unsigned int> job_ids; // wektor przechowujacy liczby porzadkowe zadan

	for (unsigned int i = 0; i < jobs.size(); i++)
		job_ids.push_back(jobs[i].job_id);

	unsigned int tau = get<1>(test); // parametr tau
	unsigned int tau_duration = get<2>(test); // czas trwania dziury
	unsigned int solution_id = 1; // zmienna przechowujaca aktualna liczbe porzadkowa rozwiazania

	stack<Solution> solutions; // stos przechowujacy przestrzen rozwiazan
	initialize_stack(solutions, jobs, solution_id); // zapelnienie stosu pierwszymi rozwiazaniami

	unsigned int upper_bound = numeric_limits<unsigned int>::max(); // poczatkowy upper bound rowna sie nieskonczonosci
	Solution best_solution; // najlepsze rozwiazanie

	// PROCEDURA BRANCH AND BOUND !!!(BEZ EWALUACJI ROZWIAZANIA)!!!

	while (!solutions.empty()) { // dopoki stos nie jest pusty: dopoki istnieje mozliwosc znalezienia lepszego rozwiazania
		Solution current_solution = solutions.top(); // pobierz rozwiazanie ze stosu
		solutions.pop(); // i je usun
		if (current_solution.seq_mach1.size() == jobs.size() && current_solution.cost < upper_bound) { // jesli uszeregowane sa wszystkie zadania oraz jest ono lepsze od aktualnego
			best_solution = current_solution; // zapisz je jako najlepsze
			upper_bound = best_solution.cost; // uaktualnij wartosc upper bound
		}
		else { // w przeciwnym przypadku
			vector<unsigned int> diff; // wektor przechowujacy brakujace zadania w obslugiwanym rozwiazaniu
			vector<unsigned int> current_job_ids = current_solution.job_ids; // wektor przechowujacy zadania obecne w obslugiwanym rozwiazaniu
			set_difference(job_ids.begin(), job_ids.end(), current_job_ids.begin(), current_job_ids.end(), inserter(diff, diff.begin())); // roznica miedzy nimi
			for (unsigned int i = 0; i < diff.size(); i++) { // dla kazdego brakujacego zadania w obslugiwanym rozwiazaniu
				vector<unsigned int> new_job_ids = current_job_ids; // utworz wektor przechowujacy liczby porzadkowe nowych zadan
				new_job_ids.push_back(diff[i]); // odpowiednio go uzupelnij
				vector<Job> new_seq_mach1 = current_solution.seq_mach1; // utworz wektor przechowujacy nowa sekwencje pierwszych operacji
				new_seq_mach1.push_back(jobs[diff[i]-1]); // dodaj na jego koncu nowe zadanie

				for (unsigned int j = 0; j < new_seq_mach1.size(); j++) { // dla kazdej mozliwej permutacji na DRUGIEJ maszynie
					vector<Job> new_seq_mach2 = current_solution.seq_mach2; // utworz wektor przechowujacy te permutacje
					Job inserted = jobs[diff[i] - 1]; // wybierz odpowiednie zadanie do wstawienia
					new_seq_mach2.insert(new_seq_mach2.begin() + j, inserted); // wstaw w odpowiednim miejscu
					Solution new_solution(current_solution.solution_id, solution_id, new_job_ids, new_seq_mach1, new_seq_mach2); // utworz nowe rozwiazanie
					solution_id++;
					//IF LOWER BOUND > UPPER BOUND continue // I albo je odrzuc
					//else ZROB TO
					solutions.push(new_solution); // albo umiesc na stosie
				}
			}
		}

	}

	cout << solution_id << endl << endl;
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

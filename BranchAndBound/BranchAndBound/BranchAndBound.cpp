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
		unsigned int solution_id;
		vector<unsigned int> seq_mach1; // sekwencja operacji na pierwszej maszynie
		vector<unsigned int> seq_mach2; // sekwencja operacji na drugiej maszynie
		vector<pair<unsigned int, unsigned int>> mach1; // uszeregowanie na pierwszej maszynie
		vector<pair<unsigned int, unsigned int>> mach2; // uszeregowanie na drugiej maszynie
		vector<pair<unsigned int, unsigned int>> holes; // wektor przechowujacy dziury na drugiej maszynie
		unsigned int cost; // wartosc naszej funkcji celu
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

int main()
{
	srand(time(NULL));

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

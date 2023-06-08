// Lader.cpp: Implementierung der Klasse Lader.
//
//////////////////////////////////////////////////////////////////////

#include "Lader.h"
#include <fstream>
#include <iostream>
#include <string>

#include <vector>
#include "Tokenizer.h"
#include <math.h>

#include "Bild.h"
#include "Sequenz.h"
#include "global.h"

// using namespace std;	-> macht std-Bezeichner ohne Präfix "std::" verfügbar,
// wird aber i.A. als schlechter Programmierstil angesehen

//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////
Lader::Lader()
{
}

Lader::Lader(std::string datei, Sequenz* sequenz)
{
	this->datei = datei;
	this->sequenz = sequenz;
}

Lader::~Lader()
{

}

/** ===================================
	LMSData.txt - Datei einlesen

	mit Hilfe des String-Tokenizers
	- die LMSData.txt auslesen
	- in die Bild-Datenstruktur schreiben
	- Bild(er) in die Sequenz eingliedern
	- Neu: Standardabweichung berechnen (f.d. Sequenz)
 =================================== */
int Lader::laden()
{
	// Test, ob Datei vorhanden
	std::ifstream lmsFile(this->datei.c_str());
	if (!lmsFile) std::cerr << "kann folg. Eingabedatei nicht oeffnen: " <<
		this->datei << std::endl;
	
	std::string zeile;
	std::vector<std::string> werte;

	Bild* bild;
	int h,i;
	int anz_zeilen;	
	int anz_d;					// Anzahl Distanzwerte pro Zeile
	int d;						// Distanzwert
	int d_max;					// größter eingelesener Distanzwert - der Zeile(!)

	float sigma_max = 0;
	float sigma_min = 8191;
	float sigma_my = 0;
	float sigma_sigma = 0;		// im Moment nicht genutzt
	float SIGMA_LIMIT = -1;//8.5f;	// wenn Wert > SIGMA_LIMIT, dann begrenzen
								// um das Sigma nicht mit extremen Werten zu
								// versauen
								// hier sind wir lieb und kappen ab 6 (auch: 10)

	int sigma_limitiert;		// Anzahl d. Werte, die durch SIGMA_LIMIT abgegrenzt
								// wurden, um nicht zu limitieren auf "-1" setzen


// -----------------------------------
//  1.Zeile - Feld-beschr. für Header
// -----------------------------------
	std::getline(lmsFile,zeile);		// zuverlässiger als ... lmsFile >> zeile;
	i = zeile.compare(0,19,"Filename;StartAngle");
	if (i!=0) {
		std::cerr << "falscher Dateityp. Abbruch!" << std::endl;
		return -1;
	}
// ------------------
//  2.Zeile - Header
// ------------------
	std::getline(lmsFile,zeile);
	// Tokenizer
	CTokenizer<CIsSemicolon>::Tokenize(werte, zeile, CIsSemicolon());
	// in werte[0] steht der Dateiname
	sequenz->startwinkel = atoi(werte[1].c_str());
	sequenz->stopwinkel = atoi(werte[2].c_str());
	sequenz->aufloesung = atoi(werte[3].c_str());
	sequenz->schrittweite = atoi(werte[4].c_str());

	anz_d = (int)((float)(sequenz->stopwinkel-sequenz->startwinkel)
		/ ((float)sequenz->aufloesung/100.0f)
		/ (float)sequenz->schrittweite) + 1;
	
	// Neu, linke und rechte Grenze!!
	if (lese_p_links>-1 && lese_p_rechts>-1) anz_d = lese_p_rechts-lese_p_links;
	else { lese_p_links=0; lese_p_rechts=anz_d; }

	// statistische Angaben der Sequenz initialisieren 
	std::vector<float> my(anz_d);
	std::vector<float> sigma(anz_d);

// ----------------------------------------
//  3.Zeile - Feld-beschr. für Datenzeilen
// ----------------------------------------
	std::getline(lmsFile,zeile);

	anz_zeilen = 0;
// ------------------------------
//  weitere Zeilen - Datenzeilen
// ------------------------------
	std::getline(lmsFile,zeile);	// vorlesen
	while (!lmsFile.eof() && (zeile.length()>0)) {	// letzte Zeile kann leer sein
		CTokenizer<CIsSemicolon>::Tokenize(werte, zeile, CIsSemicolon());

		bild = new Bild(atoi(werte[0].c_str()));
		// dem Bild hier besser einen Zeiger auf die
		// Sequenz übergeben!!!
		bild->startwinkel = sequenz->startwinkel;
		bild->stopwinkel = sequenz->stopwinkel;
		bild->aufloesung = sequenz->aufloesung;
		bild->schrittweite = sequenz->schrittweite;

//		anz_d = werte.size() - 4;
		d_max = 0;
		// in das Bild = Scanzeile schreiben, die ersten vier Werte "ausblenden":
		// (1) rec.no,
		// (2) scanindex,
		// (3) telegr.index,
		// (4) interl.scan.no
		// nachfolgende Entfernungsdaten einlesen
//		for(i=0; i<anz_d; i++) {
		for(i=lese_p_links; i<lese_p_rechts; i++) {
			d = atoi(werte[i+4].c_str());
			// max_wert merken und in das Bild-Objekt schreiben
			if (d > d_max) d_max = d;
			
//			if (lese_p_links>0 && i<lese_p_links) continue;	// Werte links d.Grenze überspringen
//			if (lese_p_rechts>0 && i>lese_p_rechts) break;	// Werte rechts d.Grenze "fallenlassen"

			bild->data.push_back(d);
			// my
//			my[i] += (float)d; 
			my[i-lese_p_links] += (float)d; 
		}
		bild->max_val = d_max;
		sequenz->addBild(bild);

		anz_zeilen++;
		std::getline(lmsFile,zeile);	// ... und nachlesen

		std::cout << "(" << werte[0].c_str() << ") Anz.Werte: " << anz_d << std::endl;
	}
	lmsFile.close();
	if (__DEBUG) std::cout << std::endl <<
		sequenz->bilder.size() << " Scanlines eingelesen" << std::endl;

	// den Rest vom Mittelwert berechnen
	for(i=0; i<anz_d; i++) {
		my[i] /= (float)anz_zeilen;
//		std::cout << i << ":" << my[i] << std::endl;
	}

	// Standardabweichung (sigma) berechnen
	for (h=0;h<sequenz->bilder.size();h++)
		for(i=0; i<anz_d; i++) {
			d = sequenz->bilder[h].get_data(i);
			sigma[i] += ((float)d-my[i])*((float)d-my[i]);
		}
/*	std::list<Bild>::iterator it;
	for (it = sequenz->bilder.begin(); it!=sequenz->bilder.end(); ++it)
		for(i=0; i<anz_d; i++) {
			d = it->get_data(i);
			sigma[i] += ((float)d-my[i])*((float)d-my[i]);
		}
*/
	sigma_limitiert = 0;
	for(i=0; i<anz_d; i++) {
		sigma[i] = sqrt(sigma[i]/(float)anz_zeilen);
		// Werte höher als SIGMA_LIMIT kappen
		if (SIGMA_LIMIT != -1 && sigma[i] > SIGMA_LIMIT) {
			sigma[i] = SIGMA_LIMIT;
			sigma_limitiert++;
		}
//		std::cout << i << ":" << sigma[i] << std::endl;
	}

	// Minimum, Maximum und Mittelwert der Sigma-Sequenz berechnen
	sequenz->my = my;
	sequenz->sigma = sigma;
	for(i=0; i<anz_d; i++) {
		if (sigma[i]>sigma_max) sigma_max = sigma[i];
		if (sigma[i]<sigma_min) sigma_min = sigma[i];
		sigma_my += (float)sigma[i]/(float)anz_d;
	}
	sequenz->sigma_max = sigma_max;
	sequenz->sigma_min = sigma_min;
	sequenz->sigma_my = sigma_my;
	sequenz->sigma_limitiert = sigma_limitiert;


	delete bild;
//todo, mit der letzten Zeile (Bsp.: 24) gibt's noch ein Problem
	return 0;
}

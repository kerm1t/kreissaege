// Sequenz.cpp: Implementierung der Klasse Sequenz.
//
//////////////////////////////////////////////////////////////////////

#include "Sequenz.h"
#include "Bild.h"
#include "Bild2d.h"

#include <list>
#include <iostream>

//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////

Sequenz::Sequenz()
{
//	std::list<Bild> bilder
}

Sequenz::~Sequenz()
{

}

void Sequenz::addBild(Bild *bild)
{
//	std::list<Bild> biller;
	this->bilder.push_back(*bild);
}

void Sequenz::display()
{
//	cout << "\nDateiname: " + sequenz->;
	std::cout << "Startwinkel: " << this->startwinkel << std::endl;
	std::cout << "Stopwinkel: " << this->stopwinkel << std::endl;
	std::cout << "Aufloesung: " << this->aufloesung << std::endl;
	std::cout << "Schrittweite: " << this->schrittweite << std::endl;

//	std::list<Bild>::iterator it;
//	for (it = this->bilder.begin(); it != this->bilder.end(); ++it) {
//	  it->display();
//	}
}

/**
	Ausschnitt des Scan-Bildes [n] in eine Datei (pgm-Format) schreiben
	falls Parameter n = -1, alle Scan-Bilder schreiben
*/
void Sequenz::toPGM(Config* conf, int n)
{
/*	int i=0;
	std::list<Bild>::iterator it;

	if (n==-1) {
		for (it = this->bilder.begin(); it!=this->bilder.end(); ++it)
			it->toPGM(conf);
	}
	else {
		for (it = this->bilder.begin(); i<n && it!=this->bilder.end(); ++it) i++;
		it->toPGM(conf);
	}
*/}

/**
	Ausschnitt des Scan-Bildes [n] in eine Datei (dat-Format) schreiben	
	falls Parameter n = -1, alle Scan-Bilder schreiben
*/
void Sequenz::toFile(int n)
{
/*	int i=0;
	std::list<Bild>::iterator it;

	if (n==-1) {
		for (it = this->bilder.begin(); it!=this->bilder.end(); ++it)
			it->toFile();
	}
	else {
		for (it = this->bilder.begin(); i<n && it!=this->bilder.end(); ++it) i++;
		it->toFile();
	}
*/}

/**
	Ausschnitt des Scan-Bildes [n]
	in d. Screen Buffer schreiben
*/
void Sequenz::toScreen(Config* conf, int n)
{
/*	int i=0;
	std::list<Bild>::iterator it;

	for (it = this->bilder.begin(); i<n && it!=this->bilder.end(); ++it) i++;
	it->toScreen(conf);
*/}

/** Intensity-Image der Sequenz (Bilder 1...n)
	in Datei (pgm-Format) schreiben
*/
void Sequenz::img_intensity()
{
	Bild2d* b = new Bild2d(ptcWIDTH, ptcHEIGHT, 0.3f, this);
	b->write();
	b->~Bild2d();
}

/**
	Mittelwert jeweils zweier Bilder der Sequenz (über die Zeit) bilden
	mittel(bild1, bild2),
	mittel(bild2, bild3),	usw...
	gespeichert wird jeweils in bild1
*/
void Sequenz::mittelwert()
{
/*	std::list<Bild>::iterator it;
	std::list<Bild>::iterator it1;
	std::list<Bild>::iterator it2;

	it = this->bilder.begin();
	while (it != this->bilder.end())
	{
		it1 = it;
		it2 = ++it;
		it1->mittelwert(&(*it2));
	}
*/}

/**
	Differenz zum jeweils vorherigen Bild berechnen
	gespeichert wird jeweils in bild1

  	- mit einem Toleranzwert, "0" <= t <= "sagen wir 5"
*/
void Sequenz::differenz(int t)
{
/*	std::list<Bild>::iterator it;
	std::list<Bild>::iterator it1;
	std::list<Bild>::iterator it2;

	it = this->bilder.begin();
	while (it != this->bilder.end())
	{
		it1 = it;
		it2 = ++it;
		it1->differenz(&(*it2), t);
	}
*/}

// -------------------------------------------------------------------
//	Filter
// -------------------------------------------------------------------

/**
	Linearer-Filter m. Mittelwert-Kernel [1 1 ... 1]
*/
void Sequenz::mittelwert1D(int n)
{
//	std::list<Bild>::iterator it;
//	for (it = this->bilder.begin(); it != this->bilder.end(); ++it) {
//		it->mittelwert1D(n);
//	}
}

/**
	Linearer-Filter m. Gauss-Kernel - z.B. [1 9 18 9 1]
*/
void Sequenz::gauss()
{
//	std::list<Bild>::iterator it;
//	for (it = this->bilder.begin(); it != this->bilder.end(); ++it) {
//		it->gauss1D();
//	}
}

/**
	nicht-linearer Rang-Filter median (kantenerhaltend)
*/
void Sequenz::median(int rang)
{
//	int n;
//	std::list<Bild>::iterator it;
//	for (it = this->bilder.begin(); it != this->bilder.end(); ++it) {
//		it->median1D(rang);
//		std::cout << "median f. bild " << n++; 
//	}
}

// -------------------------------------------------------------------
//	Segmentierung
// -------------------------------------------------------------------

void Sequenz::hough(Config *conf, int n)
{
//	int i=0;
//	std::list<Bild>::iterator it;

//	for (it = this->bilder.begin(); i<n && it!=this->bilder.end(); ++it) i++;
//	it->hough(conf);
}

void Sequenz::strip_tree(Config *conf, int n)
{
//	int i=0;
//	std::list<Bild>::iterator it;

//	for (it = this->bilder.begin(); i<n && it!=this->bilder.end(); ++it) i++;
//	it->strip_tree(conf);
}

void Sequenz::poly_regression(Config *conf, int n)
{
//	int i=0;
//	std::list<Bild>::iterator it;

//	for (it = this->bilder.begin(); i<n && it!=this->bilder.end(); ++it) i++;
//	it->poly_regression(conf);
}
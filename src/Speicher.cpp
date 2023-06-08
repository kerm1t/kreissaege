// Speicher.cpp: Implementierung der Klasse Speicher.
//
//////////////////////////////////////////////////////////////////////

#include <iostream>
#include <vector>
#include <algorithm>		// vom "sort" benötigt

#include "Speicher.h"
#include "global.h"

extern int seq_filter_typ;
extern int seq_filter_fenster;
extern int bild_filter_typ;

//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////

Speicher::init_hintergrund()
{
	int i;
	int size = this->bild->data.size();

	this->hintergrund = new Bild();
	this->hintergrund->startwinkel = this->bild->startwinkel;	// noch umständlich: Parameter setzen
	this->hintergrund->stopwinkel = this->bild->stopwinkel;
	this->hintergrund->aufloesung = this->bild->aufloesung;
	this->hintergrund->schrittweite = this->bild->schrittweite;

	// den Hintergrund "initialisieren"
	for (i=0;i<size;i++) this->hintergrund->data.push_back(0);
	this->anz_hintergrund = 1;
}

/**	===================================
	Datei-Modus => den Hintergrund berechnen durch Mittelung von 10 Bildern,
	diese aus der Sequenz auslesen
	=================================== */
Speicher::setze_hintergrund_seq()
{
	int i,j;
	int size = this->bild->data.size();
	float mittel;
	std::vector<float> v_filter;

	// ----------------------------
	// a) mehrere Bilder filtern und in den "Speicher" kopieren
	// ----------------------------
	for (i=0;i<size;i++) {
		mittel = 0.0f;
		v_filter.clear();
		for (j=0;j<HINTERGRUND_MITTELUNG;j++) {
			// bei Filtern, die über das letzte Bild hinausgehen, "nach vorne rotieren"
			v_filter.push_back((float)this->sequenz->bilder[j].data[i]);
		}
		this->hintergrund->data[i] = (int)f_mittel(v_filter);
	}
	this->anz_hintergrund = HINTERGRUND_MITTELUNG;

	// Filter über den Hintergrund laufen lassen
//	this->hintergrund->mittelwert1D(3);	// nach Mittelung wird die Gerade (s.u.) teilweise nicht gefunden
	// mmmh, Gauss bietet mal bessere, mal schlechtere Höhenvermutung der Geraden,
	// insgesamt scheint die Gerade aber eher zu tief gelegt zu werden, was im Prinzip
	// besser ist, da Objekte dann nicht ausgeblendet werden
	this->hintergrund->gauss1D();
	
	// dominante Gerade finden => mittels Hough => Maschinentisch
	this->hintergrund_logik();
}

/**	===================================
	Socket (Echtzeit)-Modus => den Hintergrund berechnen durch Mittelung von 10 Bildern,
	dafür Bilder einlesen und warten, bis das 10 Bild hereingekommen ist
	=================================== */
Speicher::setze_hintergrund_echtzeit(Bild* bild)
{
	int i;
	int size = this->bild->data.size();
	// --------------------------------
	// (10) Bilder lang speichern und im "speicher->hintergrund" mitteln
	// --------------------------------
	if (this->zaehler < HINTERGRUND_MITTELUNG) {	// "auf"addieren
		for(i=0; i<size; i++) this->hintergrund->data[i] += bild->data[i];
		this->zaehler++;
	}
	if (this->zaehler == HINTERGRUND_MITTELUNG) {	// dann teilen
		std::cout << std::endl << "Hintergrund-Speicher gefuellt!" << std::endl;
		for(i=0; i<size; i++) this->hintergrund->data[i] = this->hintergrund->data[i]/this->zaehler;
		this->zaehler = HINTERGRUND_MITTELUNG+1;	// und "einfrieren"

		// s.o.
	//	this->hintergrund->gauss1D();
		// dominante Gerade finden => mittels Hough => Maschinentisch
		this->hintergrund_logik();
	}
}

/**	===================================
	Berechnen des "mathmatischen" Hintergrunds als eine Gerade, mittels Hough-Trafo.
	=================================== */
Speicher::hintergrund_logik()
{
	int i = hintergrund_abziehen;
	
	hintergrund_abziehen = 0;
	this->hintergrund->hough(this->conf,this->hintergrund);
	hintergrund_abziehen = i;

	std::cout << "Hintergrund: Winkel=" << this->hintergrund->h_winkel
		<< ",Entf.=" << this->hintergrund->h_entf << std::endl << std::endl;

	// Falls die Hough-Gerade nicht gefunden wurde => Entfernung erstmal festlegen
	if (this->hintergrund->h_entf < 0) this->hintergrund->h_entf = 700;// 70 cm
	// die Wml::Line muesste eigentlich
	// auch initialisiert werden, aber
	// aus Faulheit erstmal unterlassen

	// "GLOBAL" die Entfernung des Hintergrunds vom Sensor speichern
	hintergrund_y = this->hintergrund->h_entf;
}

Speicher::Speicher()
{
}

/** Überladener Konstruktor Datei/Sequenz-Modus
*/
Speicher::Speicher(Config* conf, Sequenz* sequenz)
{
	int i;

	this->conf = conf;

	this->sequenz = sequenz;
	this->nQuelle = QU_DATEI;						// 0=Sequenz
	this->nBild = 0;
	this->seq_filt_typ = seq_filter_typ;
	this->seq_filt_fenster = seq_filter_fenster;	// eine ungerade Zahl!! -> Median
	this->bild_filt_typ = bild_filter_typ;
//	this->bild_filt = new Bild[this->filter_fenster];

	this->bild = new Bild();
	this->bild->startwinkel = this->sequenz->startwinkel;	// noch umständlich: Parameter setzen
	this->bild->stopwinkel = this->sequenz->stopwinkel;
	this->bild->aufloesung = this->sequenz->aufloesung;
	this->bild->schrittweite = this->sequenz->schrittweite;

	// Erstes Bild der Sequenz in den "Speicher" kopieren
	for (i=0;i<this->sequenz->bilder[0].data.size();i++)
		this->bild->data.push_back(this->sequenz->bilder[0].data[i]);

	init_hintergrund();
	setze_hintergrund_seq();
}

/** Überladener Konstruktor Socket/Echtzeit-Modus
*/
Speicher::Speicher(Config* conf, Bild* bild)
{
	int i;

	this->conf = conf;

//	this->sequenz = sequenz;
	this->nQuelle = QU_SOCKET;						// 1=Echtzeit
	this->nBild = 0;
	this->seq_filt_typ = seq_filter_typ;
	this->seq_filt_fenster = seq_filter_fenster;	// eine ungerade Zahl!! -> Median
	this->bild_filt_typ = bild_filter_typ;
//	this->bild_filt = new Bild[this->filter_fenster];

	this->bild = new Bild();
	this->bild->startwinkel = bild->startwinkel;	// noch umständlich: Parameter setzen
	this->bild->stopwinkel = bild->stopwinkel;
	this->bild->aufloesung = bild->aufloesung;
	this->bild->schrittweite = bild->schrittweite;

	// Erstes Bild der Sequenz in den "Speicher" kopieren
	for (i=0;i<bild->data.size();i++)
		this->bild->data.push_back(bild->data[i]);

	init_hintergrund();
	this->zaehler = 0;
}

Speicher::~Speicher()
{
	delete this->bild;
	delete[] this->bild_filt;
	delete this->hintergrund;
}

/** Gauss-Kernel "nach alter Väter Sitte" selbst berechnen */
void Speicher::gauss_kernel(std::vector<float> &v_in, int M) {
    int i;
	float SIGMA = 1.5;
    // fill kernel with gaussian
    for(i=-M/2; i<=M/2; i++){
		v_in.push_back(exp(-i*i/(2*SIGMA*SIGMA)) / sqrt(2*_PI*SIGMA));
    }
}

/** Gauss-Filter über einen Zeitbereich von n Bildern */
float Speicher::f_gauss(std::vector<float> &v_in, std::vector<float> &v_kernel, float k_summe) {
	int i;
	int size = v_in.size();
	float summe = 0.0f;

	for (i=0;i<size;i++) summe += v_in[i] * v_kernel[i];
	return summe/k_summe;
 }

/** Mittelungs-Filter über einen Zeitbereich von n Bildern */
float Speicher::f_mittel(std::vector<float> &v_in) {
	int i;
	int size = v_in.size();
	float summe = 0.0f;
	for (i=0;i<size;i++) summe += v_in[i];
	return summe/(float)size;
}

/** Median-Filter über einen Zeitbereich von n Bildern */
float Speicher::f_median(std::vector<float> &v_in) {
	int size = v_in.size();
	std::sort(v_in.begin(),v_in.end());
//		std::cout << "; " << werte[(int)rang/2] << std::endl;
	return v_in[(int)size/2];
}

/**	===================================
	Schiebt 
	a) ein Bild aus der Sequenz (oder Echtzeit) in den "Speicher" bzw.
	b) berechnet aus mehreren Bildern ein "Filterbild" und schiebt dieses in den
	"Speicher"

	@params:	richtung, Verschieben des Speichers auf 0=nächstes,1=voriges Bild
	@return:	Zeiger auf ein Bild
	=================================== */
Bild* Speicher::get_bild()
{
	int i,j,k,seq_size;
	int size;
	float mittel;
	std::vector<float> v_filter;
	std::vector<float> v_kernel;	// hier kommen die Gauss-Werte 'rein
	float k_summe = 0.0f;

	this->seq_filt_typ = seq_filter_typ;
	this->seq_filt_fenster = seq_filter_fenster;
	this->bild_filt_typ = bild_filter_typ;

	// --------------------------------
	//  Gauss-Filter: Kernel vorbereiten
	// --------------------------------
	if (this->seq_filt_typ == GAUSS) {
		gauss_kernel(v_kernel, this->seq_filt_fenster);
		for (i=0;i<this->seq_filt_fenster;i++) k_summe += v_kernel[i];
	}


	if (this->nQuelle == QU_SOCKET) {
		// ----------------------------
		// b) das Bild in den "Speicher" kopieren
		// ----------------------------
		// nee, der Speicher wird in der main-Schleife gefüllt
	}

	if (this->nQuelle == QU_DATEI) {
		size = this->sequenz->bilder[this->nBild].data.size();
		if (this->seq_filt_typ > 0 && this->seq_filt_fenster > 1) {
			// ----------------------------
			// a) mehrere Bilder filtern und in den "Speicher" kopieren
			// ----------------------------
			seq_size = this->sequenz->bilder.size();
			for (i=0;i<size;i++) {	// für jeden Punkt, z.B. [1...401]
				mittel = 0.0f;
				v_filter.clear();
				for (j=0;j<this->seq_filt_fenster;j++) {
					// bei Filtern, die über das letzte Bild hinausgehen, "nach vorne rotieren"
					if ((nBild+j) >= seq_size) k=nBild+j-seq_size; else k=nBild+j;
					v_filter.push_back((float)this->sequenz->bilder[k].data[i]);
				}
				switch (this->seq_filt_typ) {
					case 1: this->bild->data[i] = (int)f_gauss(v_filter, v_kernel, k_summe); break;
					case 2: this->bild->data[i] = (int)f_mittel(v_filter); break;
					case 3: this->bild->data[i] = (int)f_median(v_filter); break;
				}
			}
		}
		else {
			// ----------------------------
			// b) das Bild der Sequenz in den "Speicher" kopieren
			// ----------------------------
			for (i=0;i<size;i++)
				this->bild->data[i] = this->sequenz->bilder[this->nBild].data[i];
		}
	}

	// jetzt nochmal einen Bild-Filter drüberlaufen lassen ...
	if (this->bild_filt_typ==GAUSS) bild->gauss1D();
	if (this->bild_filt_typ==MITTEL) bild->mittelwert1D(5);
	if (this->bild_filt_typ==MEDIAN) bild->median1D(5);

// Folgende Zeile zwar cool, gibt aber noch einen RT-Error in der Main-Schleife beim Beenden
//	this->bild = &this->sequenz->bilder[nBild];
	
	return this->bild;
}

/**	===================================
	Speicher innerhalb der Sequenz (zurück) versetzen 
	=================================== */
void Speicher::prev()
{
	this->nBild--;
	if (this->nBild <= 0) nBild = sequenz->bilder.size()-1;
}

/**	===================================
	Speicher innerhalb der Sequenz (weiter) versetzen 
	=================================== */
void Speicher::next()
{
	this->nBild++;
//	if (this->nQuelle == QU_SOCKET) {  }
	if (this->nQuelle == QU_DATEI) { if (this->nBild > sequenz->bilder.size()-1) nBild = 0; }
}

int Speicher::get_index()
{
	return this->nBild+1;	// das erste Bild ist "Bild 1"
}

void Speicher::set_index(int i)
{
	// aufpassen, dass alles schön "im Rahmen" bleibt
	if (i <= 0) i = 1;
	if (i > this->sequenz->bilder.size()) i = this->sequenz->bilder.size()-1;
	this->nBild = i-1;		// 
}

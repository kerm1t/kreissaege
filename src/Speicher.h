// Speicher.h: Schnittstelle für die Klasse Speicher.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SPEICHER_H__28F14A08_4766_4B0E_B6CD_77B6876EDF15__INCLUDED_)
#define AFX_SPEICHER_H__28F14A08_4766_4B0E_B6CD_77B6876EDF15__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Bild.h"
#include "Sequenz.h"

class Speicher  
{
	void gauss_kernel(std::vector<float> &v_in, int M);
	float f_gauss(std::vector<float> &v_in, std::vector<float> &v_kernel, float k_summe);
	float f_mittel(std::vector<float> &v_in);
	float f_median(std::vector<float> &v_in);
public:
	Config* conf;

	Bild* bild;					// das Bild, das aktuell angezeigt wird
//	Bild2d bild2d;

	int nQuelle;				// 0=Sequenz,1=Echtzeit

	// --- Hintergrund ---
	int zaehler;				// 1...HINTERGRUND_MITTELUNG, hochzaehlen,
								// wenn erreicht, dann ist d. Hintergrund "aufgebaut"
	Bild* hintergrund;			// kumuliertes Hintergund"-bild"
	int anz_hintergrund;		// anz der im "Hintergrund" kumulierten Bilder
	float fehler_hintergrund;	// -> Fehlerabweichung, wenn größer, dann Punkt kein Hintergrund
	std::vector<int> hg_gerade;	// der Hintergrund als Gerade, gespeichert werden nur die y-Werte

	// --- Sequenz ---
	Sequenz* sequenz;			// nur Zeiger auf Sequenz, um auf die Bilder zugreifen zu können
	int nBild;					// "Zeiger" innerhalb der Sequenz
	
	// --- Echtzeit ---
	// ...

	// --- Filter ---
	Bild* bild_filt;			// Filter-Quell-Bilder
	int seq_filt_fenster;		// Größe d.Filters = Anzahl der Bilder, die durch den Filter "laufen"
	int seq_filt_typ;			// 0=keiner,1=Gauss,2=Mittel,3=Median
	int bild_filt_typ;			// 0=keiner,1=Gauss,2=Mittel,3=Median

	init_hintergrund();
	setze_hintergrund_seq();
	setze_hintergrund_echtzeit(Bild* bild);
	hintergrund_logik();

	Speicher();
	Speicher(Config* conf, Sequenz* sequenz);	// Sequenz
	Speicher(Config* conf, Bild* bild);			// Echtzeit -> mit einem dummy-Bild initialisieren
	virtual ~Speicher();

	Bild* get_bild();
	void prev();
	void next();
	int get_index();
	void set_index(int i);
};

#endif // !defined(AFX_SPEICHER_H__28F14A08_4766_4B0E_B6CD_77B6876EDF15__INCLUDED_)

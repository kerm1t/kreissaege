// Sequenz.h: Schnittstelle für die Klasse Sequenz.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SEQUENZ_H__000E2634_3710_418C_99B3_11365F894856__INCLUDED_)
#define AFX_SEQUENZ_H__000E2634_3710_418C_99B3_11365F894856__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vector>

#include "Bild.h"
#include "global.h"

class Sequenz 
{
public:
	std::vector<Bild> bilder;	// vorher ADT "list", war aber umständlich

	int startwinkel;			// z.B. 0
	int stopwinkel;				// z.B. 180
	int aufloesung;				// Auflösung * 100
	int schrittweite;			// i.d.R. 1

	std::vector<float> my;		// Mittel- oder Erwartungswert
	std::vector<float> sigma;	// Standardabweichung
								//
								// bei diesen aufwändigen Rechnungen ergeben
								// sich durch den float "Rundungsfehler", zumind.
								// im Vergleich mit Excel, so sei's aber im Moment
								// einmal
	float sigma_min;
	float sigma_max;
	float sigma_my;				// Mittelwert
	int	sigma_limitiert;		// Anzahl d. Werte, die durch SIGMA_LIMIT abgegrenzt
								// wurden

	Sequenz();
	virtual ~Sequenz();

	void addBild(Bild *bild);

	// Ausgabe	
	void display();
	void toPGM(Config* conf, int n);
	void toFile(int n);
	void toScreen(Config* conf, int n);

	void img_intensity();
	void mittelwert();
	void differenz(int t);

	// Filter Algorithmen - polar-Daten
	void mittelwert1D(int n);
	void gauss();
	void median(int rang);
	// Filter Algorithmen - kartes.-Daten

	//Segmentierung
	void hough(Config* conf, int n);
	void strip_tree(Config* conf, int n);
	void poly_regression(Config* conf, int n);
};

#endif // !defined(AFX_SEQUENZ_H__000E2634_3710_418C_99B3_11365F894856__INCLUDED_)

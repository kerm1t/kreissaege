// Bild.cpp: Implementierung der Klasse Bild.
//
//////////////////////////////////////////////////////////////////////

#include "Bild.h"

#include <stdlib.h>
#include <crtdbg.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <vector>
#include <algorithm>
#include <math.h>

#include "Bild2d.h"
#include "global.h"

#include "wmagic/WmlMath.h"				// für Rotating Calipers
#include "wmagic/WmlVector2.h"
#include "wmagic/WmlContMinBox2.h"

#include "wmagic/_WmlDistVec2Lin2.h"	// für Distanz zw. Punkt u. Linie
	
//#include "tree_msvc.hh"					// Templated Tree Class

//#include "text.h"						// Text in tinyPTC-buffer schreiben
										// funkt. irgendwie nicht von hier aus!!

//http://www.fileboost.net/directory/development/libraries/matrix_tcl/001047/1/download.html
//decomposition nur in der Pro (24.00$) Version
//#include "matrix/include/matrix.h"		// Matrix Library
//#include "Lapackpp1.1a/include/lapack++.h"	// Matrix Library

// Visual C++ -> Extras -> Optionen -> Verzeichnisse -> tnt/jama-Verz. hinzuf.
#include "tnt.h"
#include "jama_qr.h"

// -------------------------------------------------------------------------------------
// etliche #define's sind in der global.h, also im Falle des Refaktorierens/Ausgliederns
// von Code erstmal dort nachschauen!!!
// -------------------------------------------------------------------------------------
#define M 5
#define SIGMA 1.5						// Trucco & Verri
float ConvMask1D[M] = {1,9,18,9,1};


//std::vector<Cluster> v_cluster;		// f.Strip-Tree: Cluster, die einzelne Objekte bilden
std::vector<int> v_cluster_li;
std::vector<int> v_cluster_re;

std::vector<StripNode> v_stree;			// Segmente des Strip-Tree schick im Vektor verpackt
float v_stree_pktdichte_mittel;
float v_stree_pktdichte_sigma;

//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////

Bild::Bild()
{
}

Bild::Bild(int rec_num)
{
	this->rec_num = rec_num;
}

Bild::~Bild()
{
//todo: Vector-Objekt freigeben ...	this->data.
//	std::cout << "free ";// << _name << endl;
}

void Bild::copy_from(Bild* bild)
{
	int i;
	if (this->data.size() != bild->data.size()) return; //-1;
	for (i=0;i<this->data.size();i++)
		this->data[i] = bild->data[i];
}

int Bild::get_data(int i)
{
	return this->data[i];	// zunächst 'mal ohne Prüfung
}

//void Bild::setpixel_scr(int x, int y, int col)
void setpixel_scr(int x, int y, int col)
{
	short r, g, b;

	// Scanner-Koordinatensystem -> Weltkoordinatensystem
//	y = ptcHEIGHT-1-y;
//	x = ptcWIDTH-x;

	if (x < 0 || x > ptcWIDTH-1 || y < 0 || y > ptcHEIGHT-1) {
//		std::cerr << "Fehler(setPixel_scr), x=" << x << ",y=" << y << std::endl;
		return;
	}

	r = col;
	g = col;
	b = col;
	
	pixel[y*ptcWIDTH+x] = (r<<16)|(g<<8)|b;
}
/*
int Bild::getpixel_scr(int x, int y)
{
	return 0;
}
*/
/**	===================================
	Polare Koordinaten in kart. Koordinaten umwandeln
	(innerhalb des angezeigten Fensters)
	@params		conf,	Konfiguration -> B, H, Faktor,usw.
				i,		Nr. des zu konv.Wertes
	@return		Point,	Punkt
	=================================== */
Point Bild::get_xy(Config* conf, int i)
{
	float x,y;
	float alpha;
	float wi = (float)this->aufloesung/100.0f;		// Winkelauflösung

	// r, alpha (polar) in  x,y (kart.) umrechnen
	alpha = ((float)this->startwinkel+(float)i*wi)*BOG;
	x = cos(alpha) * (float)this->data[i] * conf->_factor;
	y = sin(alpha) * (float)this->data[i] * conf->_factor;

	// Fenster
	x -= (float)conf->_x1;
	y -= (float)conf->_y1;

	// Bed.: x,y liegen innerhalb des Fensters
	if ((x < 0 || x > conf->_width) ||
		(y < 0 || y > conf->_height)) { x = 0; y = 0; }

	Point p;
	p.x = x;
	p.y = y;
//	std::cout << p.x << "," << p.y << " - ";
	return p;
}

/**	===================================
	Die vom Hintergrund abgesetzten und von Ausreissern bereinigten Punkte
	auf Cluster (Mengen von aufeinander folgenden Punkten) untersuchen

	@params:	v,		array, Wert entweder Länge eines Strahls oder "-1" 
				i,		Position innerhalb des Arrays
				luecke,	Größe der Lücke zum vorherigen Cluster
				index,	wurde nur genommen, um einen Index für verschiedene Arrays
						(alle Punkte, nur Punkte innerhalb d. Fensters, ...)
						speichern zu können
	=================================== */
void pruefe_cluster(std::vector<int>& v, int i, int& luecke, int index) {
	std::vector<int>::iterator it;

	if (v[i] == -1) {
		if (luecke == 0) {
			// Ende eines Clusters
			v_cluster_re.push_back(index-1);	// einschließendes Intervall
			luecke = 1;
		}
		else luecke+=1;
	}
	else {	// if (v[i] > -1)
		if (luecke > 0) {
			if (luecke <= CLUSTER_MAXLUECKE) {	// Lücke von (z.B."2") "überspringen" ;-)
				if (v_cluster_re.size() > 0) {	// dafür den letzten Eintrag in "rechts" löschen
					it = v_cluster_re.end()-1;
					v_cluster_re.erase(it);
				}
				luecke = 0;
			}									// ist aber erstmal gar nicht notwendig
			else {
				// Beginn eines Clusters
				v_cluster_li.push_back(index);
				luecke = 0;
			}
		}
	}
	// ggf. einen noch offenen Cluster "abschließen"
	if (i == v.size()-1 && luecke == 0) v_cluster_re.push_back(index-1);
}

/*void finde_cluster(std::vector<int>& v) {
	int i,luecke = 5;	// wenn Daten direkt mit einer Lücke beginnen,
						// noch keinen Cluster anlegen!!

	v_cluster_li.clear();
	v_cluster_re.clear();
	for (i=0;i<v.size();i++) pruefe_cluster(v,i,luecke,i);

	for (i=0;i<v_cluster_li.size();i++)
		std::cout << "clu.(" << i << ") " << v_cluster_li[i] << "-" << v_cluster_re[i] << "(" << v_cluster_re[i]-v_cluster_li[i]+1 << ")" << std::endl;
}
*/

/**	===================================
	Filtert aus der Punktemenge diejenigen Pkte. aus,
	die in einer definierten Nachbarschaft des Hintergrunds liegen
	
	Anschließend werden die "Ausreisser" aussortiert
	=================================== */
void Bild::ohne_hintergrund_ausreisser(Config* conf, Bild* hintergrund, std::vector<int>& v)
{
	int size = this->data.size();				// Anz.Punkte (können auch ausserhalb
												// d. Fensters liegen)
	int i,j,k;
	int i_nachbarn;

//	Point p,pl,pr;								// f.d.geom.Ausreisser

	// --------------------------------
	// zunächst díe Punkte, die in einer Nachbarschaft des Hintergrunds liegen, aussortieren
	// --------------------------------
	for (i=0;i<size;i++) {
		if (abs(this->data[i]-hintergrund->data[i]) <= HINTERGRUND_NACHBARSCHAFT) v.push_back(-1);
		else v.push_back(this->data[i]);
	}
//		std::cout << "nach Abziehen d. Hintergrunds:" <<std::endl;
//		for (i=0;i<size;i++) std::cout << v_ohne_hi_ausreisser[i] << ",";
//		std::cout << std::endl <<std::endl;

	// --------------------------------
	// nachbarschaftliche Ausreißern aussortieren
	// für jeden Punkt seine +/-AUSREISSER_NACHBARSCHAFT benachbarten Punkte überprüfen
	// --------------------------------
	for (i=0;i<size;i++) {
		if (v[i] == -1) continue;
		i_nachbarn = 0;
		for (j=-AUSREISSER_NACHBARSCHAFT;j<=AUSREISSER_NACHBARSCHAFT;j++) {
			// an den Grenzen wie immer -> aufpaßen!!
			if (i+j < 0) k = 0; else k = i+j; if (i+j >= size) k = size-1; else k = i+j;
			if (v[k]>-1 && (i+j)!=i) i_nachbarn++;// den Wert selbst nicht mitrechnen!! ;-))))
		}
		if (i_nachbarn < AUSREISSER_NACHBARN) v[i] = -1;
	}

	// --------------------------------
	// geometrische Ausreißern(*) aussortieren
	// *=Punkt, der sowohl von seinem linken, als auch seinem rechten Nachbarn
	// mehr als (30)mm entfernt ist
	// dadurch werden die "Mixed Pixels", die an "hohen" Kanten entstehen, unterdrückt
	// Problem: filtert auch "hoch eintauchende" Hände heraus !!
	// --------------------------------
/*	for (i=1;i<size-1;i++) {
		if (v[i] == -1) continue;
		pl = this->get_xy(conf,i-1);
		p = this->get_xy(conf,i);
		pr = this->get_xy(conf,i+1);
		// dist = sqrt(x^2 + y^2)
		if (sqrt(abs(pl.x-p.x)*abs(pl.x-p.x)+abs(pl.y-p.y)*abs(pl.y-p.y))>AUSREISSER_GEOMETRISCH) v[i] = -1;
	}
*/
//	std::cout << "nach Ausreissern:" <<std::endl;
//	for (i=0;i<size;i++) std::cout << v[i] << ",";
//	std::cout << std::endl <<std::endl;

//	if (v.size() > 0) finde_cluster(v);
	// ohne Hintergrund-Filterung gibt's nur 1 Cluster!
}

/**	===================================
	Polare Koordinaten eines Bildes in Kartesische umwandeln
	Dabei
	- ggf. Hintergrund ausblenden
	- in Cluster unterteilen

	@params:	conf, Konfiguration
				nur_fenster,
					0 = nur im Fenster liegende Punkte in H schreiben
					1 = alle Punkte in H schreiben
				H, hier werden die x,y-Punkte 'rein geschrieben
	@return:	Anzahl d. Punkte innerhalb d. Fensters

	=================================== */
int Bild::to_xy(Config* conf, int nur_fenster, Point* H, int minus_hintergrund, Bild* hintergrund)
{
	int size = this->data.size();				// Anz.Punkte (können auch ausserhalb
												// d. Fensters liegen)
	int i;
	int anz = 0;

	float x,y;
	float alpha;
	float wi = (float)this->aufloesung/100.0f;	// Winkelauflösung

	std::vector<int> v_ohne_hi_ausreisser;
	int luecke = 5;						// wenn Daten direkt mit einer Lücke beginnen,
										// noch keinen Cluster anlegen!!

	if (this->aufloesung<=0)
		std::cerr << std::endl << "Fehler: Aufloesung nicht gesetzt...";

	/**	-------------------------------
		Hintergrund + Ausreißer ausblenden (+ Cluster finden)
		------------------------------- */
	if (minus_hintergrund>=1) {
		ohne_hintergrund_ausreisser(conf,hintergrund,v_ohne_hi_ausreisser);
		
		v_cluster_li.clear();
		v_cluster_re.clear();
	}

	/**	-----------------------------------
		Für alle Punkte
		- in x,y Koordinaten umwandeln
		- wenn Sie im Fenster liegen
		-> in eine Zeiger-"Liste" schreiben
		----------------------------------- */
	for (i=0;i<size;i++) {

		// Neu - Cluster-Einteilung vornehmen
		if (minus_hintergrund>=1) 
			if (v_ohne_hi_ausreisser.size() > 0)	// mmmh...
				pruefe_cluster(v_ohne_hi_ausreisser,i,luecke,anz);

		// Neu - Punkte, die sich nicht vom Hintergrund abheben, nicht berücksichtigen
		if (minus_hintergrund>=1) if (v_ohne_hi_ausreisser[i] == -1) continue;

		// r, alpha (polar) in  x,y (kart.) umrechnen
		alpha = ((float)this->startwinkel+(float)i*wi)*BOG;
		x = cos(alpha) * (float)this->data[i] * conf->_factor;
		y = sin(alpha) * (float)this->data[i] * conf->_factor;

		// Fenster
		x -= (float)conf->_x1;
		y -= (float)conf->_y1;
		
		// Bed.: x,y liegen innerhalb des Fensters
		if ((((int)x < 0 || (int)x > conf->_width) ||
			((int)y < 0 || (int)y > conf->_height)) && nur_fenster == 1) continue;
		// Punkte, die ausserhalb des Fensters liegen,
		// sind also mit dem Initalisierungswert belegt, d.h. -4.31602e+008

		// Neu - Punkte, die außerhalb der Begrenzung liegen auch nicht berücksichtigen!!
		if (i<conf->l_punkt || i>conf->r_punkt) continue;

		H[anz].x = x;
		H[anz].y = y;
		H[anz].alpha = atan(y/x);

		anz++;
	}
	// cluster ausgeben
	if (__ausgabe && minus_hintergrund>=1) {
		for (i=0;i<v_cluster_li.size();i++)
			std::cout << "clu.(" << i << ") " << v_cluster_li[i] << "-" << v_cluster_re[i] << "(" << v_cluster_re[i]-v_cluster_li[i]+1 << ")" << std::endl;
		std::cout << std::endl;
	}

	return anz;
}

/**	===================================
	Integer Werte des Bildes auf stdout ausgeben
	=================================== */
void Bild::display()
{
	unsigned int i;
	int size = this->data.size();//	size=10;

	std::cout << "Bild " << this->rec_num << ", Länge " <<
		size << ":" << std::endl;
	for(i=0; i<size; i++) std::cout << "(" << i <<")" << this->data[i] << ",";
	std::cout << " ..." << std::endl;
}

/**	===================================
	Bild als pgm (Portable Greymap) ausgeben
	@params:	Config
				enthält u.a. width,height,factor
	
	(Der factor ist notwendig, weil i.A. die Koordinaten
	zu gross f. das Bild sind. 1200*2 bei Bildgröße von 640 oder 1024)
	=================================== */
void Bild::toPGM(Config* conf)
{
	if (this->aufloesung < 0)
		std::cerr << std::endl << "Fehler: Aufloesung nicht gesetzt...";

	Bild2d* b = new Bild2d(conf, this);
	b->write();
	b->~Bild2d();
}

/**	===================================
	Integer-Daten in eine Datei schreiben
	die für Matlab (Excel...) lesbar ist.
	=> d.h. Werte werden durch "Tab" (\t) getrennt
	=================================== */
void Bild::toFile()
{
	int i;
	std::stringstream _datei;

	// als "img000x.dat" ausgeben
	_datei << "img" << std::setw(4) << std::setfill('0') << this->rec_num << ".dat";
	std::ofstream os(_datei.str().c_str());

	// write header
//	os << "wolli\n";
	// write values
	for (i=0;i<this->data.size()-1;i++)	os << (8191-this->data[i]) << "\t";
	os << (8191-this->data[this->data.size()-1]);

	std::cout << _datei.str() << " geschrieben." << std::endl;
	os.close();
}

void hg_gerade_zeichnen(Config* conf, Bild* hintergrund, Bild2d* bb)
{
	int i;
	int /*hx1,*/hy1;//,hx2,hy2;						// f.d. Hintergrund-Gerade

	// --------------------------
	// den Hintergrund zeichnen, es wird ein Tisch angenommen,
	// der mittels der dominanten Hough-gerade definiert wird
	// --------------------------
	hy1 = (float)hintergrund->h_entf*conf->_factor - conf->_y1;
	for (i=3;i<ptcWIDTH-3;i+=3) {
		bb->setpixel(i,hy1,PUNKT_LILA);
		bb->setpixel(i+1,hy1-1,PUNKT_LILA);
		bb->setpixel(i+2,hy1+1,PUNKT_LILA);
	}
}

/**	===================================
	@params:	x,y, Koordinate
				bb, Bild2d
	=================================== */
void zeichne_x(int x, int y, int col, Bild2d* bb)
{
	int i;
	bb->setpixel(x,y,col);			// "x" setzen
	for (i=1;i<=2;i++) {
		bb->setpixel(x-i,y-i,col);
		bb->setpixel(x+i,y+i,col);
		bb->setpixel(x-i,y+i,col);
		bb->setpixel(x+i,y-i,col);
	}
}

/**	===================================
	@params:	x,y, Koordinate
				bb, Bild2d
	=================================== */
void zeichne_raute(int x, int y, int _size, int col, Bild2d* bb)
{
	if (_size == 3) {
		bb->setpixel(x,y-1,col);
		bb->setpixel(x+1,y,col);
		bb->setpixel(x,y+1,col);
		bb->setpixel(x-1,y,col);
	}
	if (_size == 5) {
		bb->setpixel(x,y-2,col);
			bb->setpixel(x+1,y-1,col);
		bb->setpixel(x+2,y,col);
			bb->setpixel(x+1,y+1,col);
		bb->setpixel(x,y+2,col);
			bb->setpixel(x-1,y+1,col);
		bb->setpixel(x-2,y,col);
			bb->setpixel(x-1,y-1,col);
	}
}

/**	===================================
	@params:	x,y, Koordinate
				bb, Bild2d
	=================================== */
void zeichne_pfeil(int x, int y, int col, Bild2d* bb)
{
	bb->setpixel(x,y,col);
	bb->setpixel(x,y-1,col);
	bb->setpixel(x,y-2,col);
	bb->setpixel(x,y-3,col);
	bb->setpixel(x,y-4,col);
	bb->setpixel(x,y-5,col);

	bb->setpixel(x-1,y-1,col);
	bb->setpixel(x-1,y-2,col);
	bb->setpixel(x-2,y-2,col);
	bb->setpixel(x+1,y-1,col);
	bb->setpixel(x+1,y-2,col);
	bb->setpixel(x+2,y-2,col);

}

/**	===================================
	Hintergrund zeichnen - könnte auch "zeichne_bild" heissen
	=================================== */
void hg_punkte_zeichnen(Config* conf, Bild* hintergrund, Bild2d* bb)
{
	int i;
	int size = hintergrund->data.size();

	Point* V = new Point[size];								// Input
	int anz = hintergrund->to_xy(conf,1,V,0,hintergrund);	// pol. in kart. Koord. umwandeln
	for (i=0;i<anz;i++)
		bb->setpixel(V[i].x,V[i].y,PUNKT_LILA);
	delete[] V;
}

/**	===================================
	in den TinyPTC Buffer schreiben
	=================================== */
void Bild::toScreen(Config* conf, Bild* hintergrund)
{
	int i;
	int size = this->data.size();				// Anz.Punkte insges. (können auch
												// ausserhalb d. Fensters liegen)
	Point* V = new Point[size];					// Punkte innerhalb des Fensters (lt.Config)
	// --------------------------------
	// verschiedene Funktionen:
	// pol. in kart. Koord. umwandeln
	// + Hintergrund abziehen (opt.)
	// + Ausreisser aussortieren (opt.)
	// + Cluster bilden (opt.)
	// --------------------------------
	int anz = this->to_xy(conf,1,V,hintergrund_abziehen,hintergrund);
	punkte_im_fenster = anz;					// anz = Anz.Input-Punkte

	Bild2d* bb = new Bild2d(ptcWIDTH, ptcHEIGHT);

	if (hintergrund_abziehen>=1) hg_punkte_zeichnen(conf,hintergrund,bb);
	if (hintergrund_abziehen>1) hg_gerade_zeichnen(conf,hintergrund,bb);

	// zuletzt die sich vom Hintergrund abhebenden Punkte
	if (punkte_ausblenden == 0)
		for (i=0;i<size;i++) {
			if (hintergrund_abziehen==0) bb->setpixel(V[i].x,V[i].y,VG_FARBE);
			if (hintergrund_abziehen>=1) zeichne_raute(V[i].x,V[i].y,5,VG_FARBE,bb);
		}

	bb->toScreen();
	bb->~Bild2d();

// direkt in den Screen-Buffer schreiben, scheint aber nicht so viel zu bringen??
/*	int i;
	int size = this->data.size();
	int anz;

	Point* V = new Point[size];
	anz = this->to_xy(conf,1,V);				// pol. in kart. Koord. umwandeln
	
	memset(pixel,0,ptcSIZE*4);
	for (i=0;i<anz;i++) setpixel_scr(V[i].x,V[i].y,VG_FARBE);
	delete[] V;
*/
}

/**	===================================
	Mittelwert zwischen [bild] und dem übergebenen [bild2]
	berechnen und in [bild] speichern
	
	Die Bilder(Scanzeilen) müssen die gleiche Länge haben
	=================================== */
void Bild::mittelwert(Bild* bild2)
{
	unsigned int i;
	int size = this->data.size();
	
	for(i=0; i<size; i++) {
		this->data[i] = (int)((this->data[i]+bild2->data[i])/2);
	}	
}

/**	===================================
	Differenz zwischen [bild] und [bild2] berechnen;
	leider kann man keinen Farbwert speichern -> über "Umwege" doch!

	- mit einem Toleranzwert, "0" <= t <= "sagen wir 5"

	- da das jeweils 1. Bild überschrieben wird, fehlt allerdings
	  ein Ausgangsbild, müsste eigentlich eins vorne reingeschoben
	  werden
	=================================== */
void Bild::differenz(Bild* bild2, int t)
{
	unsigned int i;
	int size = this->data.size();
	unsigned int w1;
	unsigned int w2;

	for(i=0; i<size; i++) {
		w1 = this->data[i];
		w2 = bild2->data[i];
//		if (abs(w1-w2) < t) this->data[i] = -1;
	}	
}

/**	===================================
	Linearer Filter m.
	Mittelwert-Maske (Kernel), n ungerade
	=================================== */
void Bild::mittelwert1D(int n)
{
	int i,j;
	int size = this->data.size();
	int sum;

	for (i=n/2; i<size-n/2; i++) {
		sum = 0;
		for (j=-(n/2);j<=(n/2);j++) {
			sum += this->data[i-j];	// wieso eigentlich "-"?
		}
		this->data[i] = (int)((float)sum/(float)n);
	}
}

/**	===================================
	Linearer Filter m.
	Gauss'schem Filterkernel (M=5)
	=================================== */
void Bild::gauss1D()
{
	int i,j,k;
	int size = this->data.size();
	int g;

    for (i=M/2; i<size-M/2; i++) {
            g = 0;
            for (j=-(M/2); j<=(M/2); j++) {
                k = j+M/2;
				g += this->data[i-j] * ConvMask1D[k];
            }
            g = g / (38);
//            if (g > 255) g = 255;
            this->data[i] = (unsigned int) g;
    }
}

/**	===================================
	Rangordnungsoperation,
	Median-Filter
	@2do
	Houston, wir haben noch ein Problem:das Bild wird um einige mm verschoben
	=================================== */
void Bild::median1D(int rang)
{
	int i,j;
	int size = this->data.size();
	std::vector<int> werte;

	std::vector<int> output(size,0);

	for (i=rang/2;i<size-rang/2;i++) {
		werte.clear();
		for (j=0;j<rang;j++) {
			werte.push_back(this->data[i+j]);
//			std::cout << this->data[i+j] << " ";
		}
		// sortieren und das mittlere Element nehmen
		std::sort(werte.begin(),werte.end());
//		std::cout << "; " << werte[(int)rang/2] << std::endl;
		output[i] = werte[ceil((float)rang/(float)2)];	// Returns the smallest integer that is greater or equal to x
	}
	this->data = output;
}

/**	===================================
	Log-Hough Transformation
	... arbeitet direkt auf den polaren Koordinaten
	und nutzt nur die Punkte des Laser-Scanners
	=================================== */
void Bild::log_hough()
{
}

/**	kleine Hilfsfunktion */
void arr_to_file(int* arr, int n, std::string datei)
{
	int i;

	// als "*.txt" ausgeben
	std::ofstream os(datei.c_str());

	// write header
//	os << "Histogramm d. Akkumulators\n";
	// write values
	for (i=0;i<n-1;i++) os << arr[i] << "\t";
	os << arr[n-1];

	std::cout << datei.c_str() << " geschrieben." << std::endl;
	os.close();
}

/**
	Linie zeichnen - Geradengl. in (Hesse'scher) Normalenform
*/
/*void glider(int _alpha, int r, int col, Bild2d* bb)
{
	float x,y;
	float alpha = (float)_alpha*(float)BOG;	// in's Bogenmass umwandeln

	int i, j;
	int max_nachbar = 10;
	int nachbar;

//	std::cout << "theta=" << alpha << ", rho=" << r << std::endl;		

	if ((alpha > _PI/4) && (alpha < 3*_PI/4)) {
		// eher waagerechte Linie -> x durchlaufen
		for (x=0;x<bb->w;x++) {
			y = ((float)r - (float)x * cos(alpha)) / sin(alpha);
			nachbar = 0;
			for (i=-max_nachbar;i<max_nachbar;i++) {
				for (j=-max_nachbar;j<max_nachbar;j++) {
					if (bb->getpixel((int)x+i,(int)y+j)==255) nachbar=1;
				}
			}
			if (nachbar == 1) bb->setpixel((int)x,(int)y,col);
		}
	}
	else {
		// eher senkrechte Linie -> y durchlaufen
		for (y=0;y<bb->h;y++) {
			x = ((float)r - (float)y * sin(alpha)) / cos(alpha);
			nachbar = 0;
			for (i=-max_nachbar;i<max_nachbar;i++) {
				for (j=-max_nachbar;j<max_nachbar;j++) {
					if (bb->getpixel((int)x+i,(int)y+j)==255) nachbar=1;
				}
			}
			if (nachbar == 1) bb->setpixel((int)x,(int)y,col);
		}
	}

	// hier wurde mal, in x1,y1,x2,y2 umgerechnet, aber das ist m.E. weniger sinnvoll
	// als obige Lösung
}
*/

/**	===================================
	Wandelt eine Gerade der Form alpha, r in die Form (x1,y1)-(x2,y2) um
	@params:	_alpha, Winkel
				r, Abstand vom Nullpunkt
				seg, der "Rückgabewert"
	=================================== */
void konv_gerade(int _alpha, int r, Wml::Line2f& line)
{
	float x1,x2,y1,y2;
	float alpha = (float)_alpha*(float)BOG;	// in's Bogenmass umwandeln

	Wml::Vector2f point1;					// "linker" Punkt der Geraden
	Wml::Vector2f point2;					// "rechter" Punkt der Geraden

	if ((alpha > _PI/4) && (alpha < 3*_PI/4)) {
		// eher waagerechte Linie -> x
		x1 = 0;
		y1 = ((float)r - (float)x1 * cos(alpha)) / sin(alpha);
		x2 = ptcWIDTH-1;
		y2 = ((float)r - (float)x2 * cos(alpha)) / sin(alpha);
	}
	else {
		// eher senkrechte Linie -> y
		y1 = 0;
		x1 = ((float)r - (float)y1 * sin(alpha)) / cos(alpha);
		y2 = ptcHEIGHT-1;
		x2 = ((float)r - (float)y2 * sin(alpha)) / cos(alpha);
	}

	point1.X() = x1; point1.Y() = y1;
	point2.X() = x2; point2.Y() = y2;

	line.Origin() = point1;
	line.Direction() = point2-point1;
}

/**	===================================
	@params		pl - linker Punkt des Segments
				pr - rechter Punkt des Segments
	hier nicht als Methode von "Bild", da von glider aus aufgerufen,
	die keine Methode von "Bild" ist
	=================================== */
int neues_objekt_hough(Config* conf, Point pl, Point pr, int i_typ, int _alpha)
{
	float y_min = (pr.y < pl.y) ? pr.y : pl.y;

	float f_tmp;
	Objekt ob;
//	ob.grad = grad;
//	ob.fehler = fehler;

//	pl = V[v_cluster_li[cluster]];
//	pr = V[v_cluster_re[cluster]];

	ob.x_mi = conf->_width - (pl.x+(pr.x-pl.x)/2);	// auch über eine Bounding Box
	// uffbasse!!
	if (ob.x_mi < 0 || ob.x_mi > conf->_width) ob.x_mi = conf->_width/2;
	ob.x_gr = abs(pr.x-pl.x);						// möglich
	
	ob.y_mi = pl.y+(pr.y-pl.y)/2;
	// uffbasse!!
	if (ob.y_mi < 0 || ob.y_mi > conf->_height) ob.y_mi = conf->_height/2;
	ob.y_gr = abs(pr.y-pl.y);

	// Erhebung über dem Tisch
	ob.y_erhebung = hintergrund_y - (y_min+conf->_y1);
	
	ob.wi_waage_mittel = _alpha;

//	std::cout << ob.x_gr << "  ";

	// HOT!!!!!! Fix
	f_tmp = (float)ob.x_gr/10.0f;
	if (f_tmp>=0.95f && f_tmp<=2.4f) i_typ = 5;				// Finger-Breite
	else if (f_tmp>=8.0f && f_tmp<=10.5f) i_typ = 2;		// Hand-Breite
	else if (f_tmp>=39.0f && f_tmp<=41.0f) i_typ = 1;		// Holz-Breite

	if (_alpha > 85.0f && _alpha < 95.0f && f_tmp > 20) i_typ = 1;// Holz

	ob.typ = (i_typ<0 ? 0 : i_typ);
	objekte.push_back(ob);
	return ob.typ;
}

/**	===================================
	Durch die Hough-Transformation
	gefundene Geraden begrenzen => Segmente
	
	=> Schicken eines Gliders,
	der die Gerade in Segmente unterteilt
	=================================== */
void glider(Point* V, int anz, int _alpha, int r, int col, Config* conf, Bild2d* bb)
{
	const float nachbarschaft = 11.0f;
	
	int i;
	Wml::Line2f line;
	Wml::Vector2f pointx;				// Entfernungspunkt
	float dist;

	int i_hough_typ;					// 0=weiss nicht,1=Holz,2=Hand, usw.
	int i_col;

	Wml::Vector2f lfp_links, lfp_rechts;// Lot-Fußpunkt
	Point pl, pr;

	konv_gerade(_alpha,r,line);			// Geradengleichung der Form g = (rho, theta) in g: (x1,y1) - x2,y2) umwandeln

	std::vector<int> vlinks;
	std::vector<int> vrechts;

	int anz_punkte, links, rechts;

	/**	-------------------------------
		Segmente berechnen
		------------------------------- */
	anz_punkte = 0;
	links = -1;
	for (i=0;i<anz;i++) {
		pointx.X() = V[i].x;
		pointx.Y() = V[i].y;
		dist = Wml::Distance(pointx,line);
		if (dist < nachbarschaft) {		// wenn (Distanz (Punkt[n]-Gerade) < e)
			// (neuer) linker Punkt
			if (links < 0) {
				links = i;
				rechts = i;
				anz_punkte = 1;
			}
			else {
				// rechten Punkt zu linkem setzen
				if (i == rechts+1) {
					rechts = i;			// dieselbe Gerade
					anz_punkte++;
				}
				else {	 				// n > rechts+1
					// dieser Fall darf eigentlich gar nicht eintreten
//					Segment anlegen mit den End-
//					punkten Punkt[links], Punkt[rechts];
				}
			}
		}
		else {
			if (anz_punkte < 2) {		// d.h. kein oder einzelner Punkt
				anz_punkte = 0;
				links = -1;
			}
			else {
//				Segment anlegen mit den End-
//				punkten Punkt[links], Punkt[rechts];
				if ((rechts-links) > 4) {
					vlinks.push_back(links);
					vrechts.push_back(rechts);
				}
//				std::cout << "seg: li=" << links << ", re=" << rechts << std::endl; 
				anz_punkte = 0;
				links = -1;
			}
		}
	}
	
	/**	-------------------------------
		Segmente zeichnen -> und zwar die richtige Linie, dafür die Segment-End
		punkte jeweils lotrecht auf die Linie fällen
		------------------------------- */
	for (i=0;i<vlinks.size();i++) {
		// Lotfusspunkt fällen
		pointx.X() = V[vlinks[i]].x;
		pointx.Y() = V[vlinks[i]].y;
		lfp_links = Wml::LotFusspunkt(pointx,line);
		pointx.X() = V[vrechts[i]].x;
		pointx.Y() = V[vrechts[i]].y;
		lfp_rechts = Wml::LotFusspunkt(pointx,line);

//		std::cout << "li: " << lfp_links.X() << "(" << V[vlinks[i]].x << "), " << lfp_links.Y() << "(" << V[vlinks[i]].y << ")" << std::endl;
//		std::cout << "re: " << lfp_rechts.X() << "(" << V[vrechts[i]].x << "), " << lfp_rechts.Y() << "(" << V[vrechts[i]].y << ")" << std::endl;

		// Objekt anlegen
		pl.x = lfp_links.X(); pl.y = lfp_links.Y();
		pr.x = lfp_rechts.X(); pr.y = lfp_rechts.Y();
		i_hough_typ = neues_objekt_hough(conf,pl,pr,-1,_alpha);

		// Gerade bzw. Segment zeichnen
		if (i_hough_typ == 1) i_col = PUNKT_BLAU; else i_col = PUNKT_DUNKEL;
		bb->linie(lfp_links.X(),lfp_links.Y(),lfp_rechts.X(),lfp_rechts.Y(),i_col);
//		bb->linie(V[vlinks[i]].x,V[vlinks[i]].y,V[vrechts[i]].x,V[vrechts[i]].y,PUNKT_DUNKEL);
//		bb->setpixel(V[vlinks[i]].x,V[vlinks[i]].y,255);
//		bb->setpixel(V[vrechts[i]].x,V[vrechts[i]].y,255);

	}

	/**	-------------------------------
		für alle Punkte: Distanz zw. px und seg berechnen
		-> dann die Punkte innerhalb der Nachbarschaft der Linie grau zeichnen
		------------------------------- */
	if (vlinks.size() > 0) {
		links = vlinks[0];
		rechts = vrechts[vrechts.size()-1];
		if (punkte_ausblenden == 0)
			for (i=0;i<anz;i++) {
				pointx.X() = V[i].x;
				pointx.Y() = V[i].y;
				dist = Wml::Distance(pointx,line);
				// Pkt.e links/rechts neben d.Linie dunkel
				if (i<links || i>rechts) {
					bb->setpixel(V[i].x,V[i].y,PUNKT_DUNKEL);
					continue;
				}
				// Pkt.e innerhalb der Nachbarschaft dunkel
				if (dist < nachbarschaft) {
					bb->setpixel(V[i].x,V[i].y,PUNKT_DUNKEL);
				}
				// Pkt.e ausserhalb der Nachbarschaft hell
				else bb->setpixel(V[i].x,V[i].y,VG_FARBE);
			}
	}
//	bb->linie(_alpha,r,col);// unterschiedl. hell zeichnen ("4-fach verstärkt")
}


/**	Untersuchen des Hough-Raums auf sinnvolle Geraden
	@param	V	Array der Punkte
	@param	anz	Anzahl Punkte im Array V
	@param	lm	Array mit den Intensitätswerten "pmax" an der Stelle (r,t) des Akkumulators
	@param	lm2	Array mit der Position "imax" des Wertes (pmax) innerhalb der Spalte
	@param	T	Länge des Arrays lm bzw. lm2 (i.d.R. 180)
*/
void Bild::tracking(Point* V, int anz, int* lm, int* lm2, int T, Config* conf, Bild* hintergrund)
{
	int i, j;
	Bild2d* bb = new Bild2d(ptcWIDTH, ptcHEIGHT);

	int jmax,pmax;
	int* deltaH = new int[T];					// "Histogramm"-Gradient
	int* delta2H = new int[T];					// 2. Ableitung (Wendepunkt)

	std::vector<int> dom_lines;					// hehe

	// zuerst den Hintergrund zeichnen
	if (hintergrund_abziehen>=1) hg_punkte_zeichnen(conf,hintergrund,bb);
	if (hintergrund_abziehen>1) hg_gerade_zeichnen(conf,hintergrund,bb);

	// -------------------------------
	// detect dominant peaks -> das ist noch nicht okay + muss verbessert werden...
	// -------------------------------
	jmax = 0; pmax = 0;							// Maximalen Wert finden
	for (j=0;j<T;j++) { 
		if (lm[j] >= pmax) {
			pmax = lm[j];
			jmax = j;
		}
	}
	// smooth
	lm[0] = (lm[0] + lm[1])/2; lm[T-1] = (lm[T-1] + lm[T-2])/2; // äußere Balken -> Kanten spiegeln
	for (j=1;j<T-1;j++) {
		lm[j] = (lm[j-1] + lm[j] + lm[j+1])/3;
	}
//	arr_to_file(lm,T,"arr.txt");

	// gradient
	deltaH[0] = lm[0] - lm[1]; deltaH[T-1] = lm[T-1] - lm[T-2]; // äußere Balken -> Kanten spiegeln
	for (j=1;j<T-1;j++) {
		deltaH[j] = lm[j] - lm[j-1];
	}
//	arr_to_file(deltaH,T,"arr1.txt");

	// second derivative
//	delta2H[0] = deltaH[0] - deltaH[1];
	delta2H[0] = deltaH[0] + deltaH[1]-1;		// äußere Balken -> Kanten spiegeln
	delta2H[T-1] = deltaH[T-1] - deltaH[T-2];	// äußere Balken -> Kanten spiegeln
	for (j=1;j<T-1;j++) {
		delta2H[j] = deltaH[j] - deltaH[j-1];
	}
//	arr_to_file(delta2H,T,"arr2.txt");

	for (j=0;j<T-1;j++) {
		if (lm[j] > pmax/3) { 
//std::cout << "??? Bed.1->dominanter peak bei " << j << std::endl;
			if ((deltaH[j] == 0) || (deltaH[j]>0 && deltaH[j+1]<0)) {
//std::cout << "??? Bed.2->dominanter peak bei " << j << std::endl;
				if (delta2H[j] < 0) {
//std::cout << "Bed.3->dominanter peak bei " << j << std::endl;
					dom_lines.push_back(j);

					// ----------------
					// die Waagerechte hat hier den Winkel 85 < j < 95;
					// ----------------
					if (j>85 && j<95) {
						konv_gerade(j,lm2[j],this->h_gerade);
						this->h_winkel = j;
						// zunächst Entfernung vom oberen Fensterrand
						// => direkt in Entfernung vom Scanner umrechnen
						this->h_entf = lm2[j]+conf->y_initial;
					}
//std::cout << "Gerade m. dom peak" << std::endl << "Winkel(T)=" << j << std::endl;
//std::cout << "Intens.(pmax)=" << lm[j] << std::endl;
//std::cout << "Abstand(imax)=" << lm2[j] << std::endl;
//std::cout << "gerade m.dom.peak: wi=" << j << ",int=" << lm[j] << std::endl;
				}
			}
		}
	}
	delete[] delta2H;
	delete[] deltaH;

// die (MAX_LINES) größten Geraden heraussuchen ... auch nicht unbedingt sinnvoll
//	std::cout << "Anz.Dom.Lines=" << dom_lines.size() << "  ";

	// -------------------------------
	// Linien und Punkte zeichnen
	// -------------------------------
	for (i=0;i<dom_lines.size();i++) {
		if (hough_glider == 0) bb->linie(dom_lines[i],lm2[dom_lines[i]],lm[dom_lines[i]]*4);
		else glider(V,anz,dom_lines[i],lm2[dom_lines[i]],lm[dom_lines[i]]*4,conf,bb);
	}
	
	if (dom_lines.size()==0)					// wenn keine Linie gefunden, dann trotzdem Punkte zeichnen (dunkel)
		for (i=0;i<anz;i++) bb->setpixel(V[i].x,V[i].y,PUNKT_DUNKEL);

	if (hintergrund_abziehen>=1) hg_punkte_zeichnen(conf, hintergrund, bb);

	bb->toScreen();
	delete bb;
}

/**	===========================================================================

	Hough Transformation
	(siehe Trucco & Verri, AIS, Zimmer/Bonz)
	rho, theta bilden Parameterraum

	@params: Config, zur Berechnung d. x,y Koordinaten
	(ist das überhaupt notwendig?)

	=========================================================================== */
void Bild::hough(Config* conf, Bild* hintergrund)
{
	const int R = sqrt((conf->_width*conf->_width + conf->_height*conf->_height));// <----- ?????
	const int T = 180;	// Diskretisierung des Hough-Raums [0..pi]

	int i,j;
	int size = this->data.size();

//	int /*hx1,*/hy1;//,hx2,hy2;						// f.d. Hintergrund-Gerade

	Point* V = new Point[size];					// Input
	// --------------------------------
	// verschiedene Funktionen:
	// pol. in kart. Koord. umwandeln
	// + Hintergrund abziehen (opt.)
	// + Ausreisser aussortieren (opt.)
	// + Cluster bilden (opt.)
	// --------------------------------
	int anz = this->to_xy(conf,1,V,hintergrund_abziehen,hintergrund);
	punkte_im_fenster = anz;					// wird auf dem Bildschirm ausgegeben
												// anz = Anz.Input-Punkte

	float sin_theta[T], cos_theta[T];
	short theta;
	float rho;

	int p;										// Farbe eines pixels speichern
	int pmax,imax;								// Lokales Maximum
	int anz_geraden;

	// warum das mit den Koordinaten im Fenster manchmal nicht
	// funktioniert, weiss ich nicht
	if (anz == 0) {								// falls alle Punkte außerhalb des Fensters
		Bild2d* bb = new Bild2d(ptcWIDTH,ptcHEIGHT);
		bb->toScreen();							// ... zumindest den Screen refreshen
		bb->~Bild2d();	
		return;									
	}
	/**	-----------------------------------
		Hough - Akkumulator bereitstellen
		----------------------------------- */
	Config* conf1 = new Config();
	conf1->_width = T;
	conf1->_height = R;		// <----- ?????
//	Bild2d* b = new Bild2d(conf1, this);

	// trig. Funktionen vorbereiten (Rechenaufwand reduz.)
	for (theta=0;theta<T;theta++) {
		sin_theta[theta] = sin((float)theta*BOG);
		cos_theta[theta] = cos((float)theta*BOG);
	}

//	Bild2d* bb = new Bild2d(ptcWIDTH,ptcHEIGHT);
/////
/// Schleife
/////
	// Neu - m. (Kellogg's) Clustern
	if (hintergrund_abziehen==0) {				// ohne Hintergrund
		v_cluster_li.clear();					// => 1 großen Cluster anlegen!
		v_cluster_re.clear();
		v_cluster_li.push_back(0);
		v_cluster_re.push_back(anz);
	}
	// jeden Cluster ...
	for (i=0;i<v_cluster_li.size();i++) {
//		std::cout << "clus." << i << "   ";
		// Cluster mit einer Punktzahl < (4) werden aussortiert
//		if (v_cluster_re[i]-v_cluster_li[i] <= CLUSTER_MINGROESSE) {
//			if (__ausgabe) std::cout << "Hough(" << i << "): Cluster < " << CLUSTER_MINGROESSE << std::endl;
//			continue;
//		}

		Bild2d* b = new Bild2d(conf1, this);
		/**	-----------------------------------
			jeden Punkt in den Akkumulator-Raum transformieren
			----------------------------------- */
		for (i=0;i<anz;i++) {	//(size)
			for (j=0;j<T;j++) {
				rho = V[i].x * cos_theta[j] + V[i].y * sin_theta[j];
				b->pixelup(j,(int)rho);
			}
		}

		/**	-----------------------------------
			für jedes [theta] (jede Spalte) die lokalen Maxima suchen, threshold = t
			----------------------------------- */
		int* lm = new int[T];	// Lokale Maxima speichern (hier: pmax, d.h. Pixel-Wert)
		int* lm2 = new int[T];	// -"- (hier: imax, d.h. die Spalte/den Winkel)

		anz_geraden = 0;
//		while (anz_geraden==0) {
			for (j=0;j<T;j++) {
				pmax = 0;
				imax = 0;
				for (i=0;i<R;i++) {	// maximalen Wert der jeweiligen Spalte (Winkel) finden
					p = b->getpixel(j,i);
					if (p >= hough_threshold)		// jeden Wert >= Threshold nehmen
						if(p > pmax) {
							pmax = p;
							imax = i;
							anz_geraden++;
						}
				}
				if (imax > 0) {
		//			std::cout << "[thres.=" << pmax << "] theta=" << j << ", rho=" << imax << std::endl;		
//					if (hough_tracking == 0)
//						bb->linie(j,imax,pmax*4);// unterschiedl. hell zeichnen ("4-fach verstärkt")
				}
				if (pmax > 0) { lm[j] = pmax; lm2[j] = imax; } else { lm[j] = 0; lm2[j] = 0; }
			}
//			std::cout << "anz.ger." << anz_geraden;
//			if (hough_threshold > 5) hough_threshold--;
//			else continue;
//		}
	//	arr_to_file(lm,T,"arr.txt");
		if (__ausgabe) std::cout << "Anz.Geraden v.d. Tracking=" << anz_geraden << std::endl;
		if (hough_tracking == 1) tracking(V,anz,lm,lm2,T,conf,hintergrund);
		delete[] lm2;
		delete[] lm;

	// zuletzt die sich vom Hintergrund abhebenden Punkte
//	if (punkte_ausblenden == 0) {
//		for (i=0;i<size;i++) bb->setpixel(V[i].x,V[i].y,VG_FARBE);

//b->write("_akkumulator.pgm");	// Hough-Akkumulator in eine Datei schreiben

		b->~Bild2d();
	}
/////
/// Schleife
/////

//	if (hough_tracking == 0) {
//		bb->toScreen();	// ... zumindest den Screen refreshen
//		bb->~Bild2d();
//	}

	delete conf1;
	delete[] V;

//	b->write("_akkumulator.pgm");
//	b->~Bild2d();
}

// isLeft(): test if a point is Left|On|Right of an infinite line.
//    Input:  three points P0, P1, and P2
//    Return: >0 for P2 left of the line through P0 and P1
//            =0 for P2 on the line
//            <0 for P2 right of the line
//    See: the January 2001 Algorithm on Area of Triangles
inline float isLeft( Point P0, Point P1, Point P2 )
{
    return (P1.x - P0.x)*(P2.y - P0.y) - (P2.x - P0.x)*(P1.y - P0.y);
}

/**	===================================
	"This month we present a different algorithm that improves this efficiency to O(n)
	linear time for a connected simple polyline with no abnormal self-intersections."
	=================================== */
// simpleHull_2D():
//    Input:  V[] = polyline array of 2D vertex points
//            n   = the number of points in V[]
//    Output: H[] = output convex hull array of vertices (max is n)
//    Return: h   = the number of points in H[]
int simpleHull_2D( Point* V, int n, Point* H )
{
    // initialize a deque D[] from bottom to top so that the
    // 1st three vertices of V[] are a counterclockwise triangle
    Point* D = new Point[2*n+1];
    int bot = n-2, top = bot+3;   // initial bottom and top deque indices
    D[bot] = D[top] = V[2];       // 3rd vertex is at both bot and top
    if (isLeft(V[0], V[1], V[2]) > 0) {
        D[bot+1] = V[0];
        D[bot+2] = V[1];          // ccw vertices are: 2,0,1,2
    }
    else {
        D[bot+1] = V[1];
        D[bot+2] = V[0];          // ccw vertices are: 2,1,0,2
    }

    // compute the hull on the deque D[]
    for (int i=3; i < n; i++) {   // process the rest of vertices
        // test if next vertex is inside the deque hull
        if ((isLeft(D[bot], D[bot+1], V[i]) > 0) &&
            (isLeft(D[top-1], D[top], V[i]) > 0) )
                continue;         // skip an interior vertex

        // incrementally add an exterior vertex to the deque hull
        // get the rightmost tangent at the deque bot
        while (isLeft(D[bot], D[bot+1], V[i]) <= 0)
            ++bot;                // remove bot of deque
//		D[--bot] = V[i];          // insert V[i] at bot of deque
		if (bot>0) D[--bot] = V[i];          // insert V[i] at bot of deque

        // get the leftmost tangent at the deque top
        while (isLeft(D[top-1], D[top], V[i]) <= 0)
            --top;                // pop top of deque
//		D[++top] = V[i];          // push V[i] onto top of deque
		if (top<2*n) D[++top] = V[i];          // push V[i] onto top of deque
    }

    // transcribe deque D[] to the output hull array H[]
    int h;        // hull vertex counter
// ws ->
int lim = (top-bot);
if (lim >= n) lim = n-1;
// <- ws
//    for (h=0; h <= (top-bot); h++)
    for (h=0; h <= lim; h++)
        H[h] = D[bot + h];

    delete D;
//    return h-1;
    return h;	// <- ws
}

//int chainHull_2D( Point* P, int n, Point* H )
//int grahamHull_2D( Point* P, int n, Point* H )

//void swap(Point* pa,Point* pb)
//void bubbleSort(Point* P,int size)

//int rot_calipers(Point* P, int n)

/**	===================================
	Zeichnet ein orientiertes Rechteck
	@params:	b,		Rechteck
				col,	Farbe [0...255]
				bb,		Bild2d
	=================================== */
void zeichne_box(Wml::Box2f box, int col, Bild2d* bb)
{
	Wml::Vector2f p0,p1,p2,p3;

	/*	If the box has center C, axes U0 and U1, and extents e0 and e1,
		the four corners are C+s0*e0*U0+s1*e1*U1
		where |s0| = 1 and |s1| = 1 (four choices). The area of the box is 4*e0*e1.
		siehe http://groups.google.com/groups?hl=de&lr=&ie=UTF-8&threadm=9PdR7.99044%24MX1.15623675%40news02.optonline.net&rnum=1&prev=/groups%3Fq%3D%2522rotating%2Bcalipers%2522%2Bc%252B%252B%26hl%3Dde%26lr%3D%26ie%3DUTF-8%26selm%3D9PdR7.99044%2524MX1.15623675%2540news02.optonline.net%26rnum%3D1
	*/
	// man könnte auch die Wml-Funktion "ComputeVertices" nutzen ;-)
	p0 = box.Center() - box.Extent(0)*box.Axis(0) + box.Extent(1)*box.Axis(1);
	p1 = box.Center() + box.Extent(0)*box.Axis(0) + box.Extent(1)*box.Axis(1);
	p2 = box.Center() + box.Extent(0)*box.Axis(0) - box.Extent(1)*box.Axis(1);
	p3 = box.Center() - box.Extent(0)*box.Axis(0) - box.Extent(1)*box.Axis(1);

	// Bounding-Box zeichnen
	bb->linie(p0.X(),p0.Y(),p1.X(),p1.Y(),col);
	bb->linie(p1.X(),p1.Y(),p2.X(),p2.Y(),col);
	bb->linie(p2.X(),p2.Y(),p3.X(),p3.Y(),col);
	bb->linie(p3.X(),p3.Y(),p0.X(),p0.Y(),col);
}

/**	===================================
	Verbindet eine Reihe von Punkten durch Geraden.
	=> zum Zeichnen der konvexe Hülle
	@params:	P,		Liste mit Punkten
				n,		Anzahl d. Punkte
				bb,		Bild2d
	=================================== */
void zeichne_polygon(Point* P, int n, int col, Bild2d* bb)
{
	int i;
	for (i=0;i<n-1;i++) {
		bb->linie(P[i].x,P[i].y,P[i+1].x,P[i+1].y,col);
	}
	bb->linie(P[n-1].x,P[n-1].y,P[0].x,P[0].y,col);
}

/**	===================================
	Umhüllendes Rechteck (bounding rectangle) berechnen
	@params:	V,		Liste mit Punkten
				size,	Anzahl d. Punkte
				bb,		Bild2d
	=================================== */
Wml::Box2f bounding_rect(Point* V, int size, Bild2d* bb)
{
	Point* H = new Point[size];				// Output
	int n;									// Anz.Output-Punkte(d.konv.Hülle)
	int i;

	Wml::Box2f box;							// (minimum Bounding) Box

	if (size==2) {
		// nur 2 Punkte => "flugs 'ne" Box konstruieren
		Wml::Vector2f p1,p2,center;
		p1.X() = V[0].x; p1.Y() = V[0].y;
		p2.X() = V[1].x; p2.Y() = V[1].y;
		center = p1 + 0.5f*(p2-p1);

		box.Center() = center;
		box.Axis(0) = (p1-center);
		box.Extent(0) = 1.0f;//(p1-center).Length(); Ahaaaaaaaaaaaaa!!!
		box.Axis(1) = center.UnitCross(p2);
		box.Extent(1) = 1.0f;
	}
	if (size>2) {
		Wml::Vector2f* edges;					// Punkte der konv.Hülle

		n = simpleHull_2D(V, size, H);
	//	std::cout << "simpleHull: Anzahl P. in konv. Huelle: " << n << std::endl;

		// Konvexe Hülle zeichnen
//		zeichne_polygon(H,n,100,bb);

		// Eckpunkte hervorheben
//		for (i=0;i<n;i++) bb->setpixel(H[i].x,H[i].y,VG_FARBE);

		// eigene rot_calipers-Fkt. durch wildmagic ersetzt (Bes.erzieher.Maßnahme)
		edges = new Wml::Vector2f[n];
		for (i=0;i<n;i++) edges[i] = Wml::Vector2f(H[i].x,H[i].y);
		box = Wml::MinBox(n,edges);
	
		delete[] edges;
	}
	// Bounding-Box zeichnen
//	zeichne_box(box,100,bb);

	delete[] H;

	return box;
}

/**	===================================
	Splitting-Point berechnen.
	Das ist der Punkt mit der max.Entfernung zur Längsachse der Box
	@params:	Extrempunkte pi und pj
				Punkte des Kurvenzugs P, size = Anz. Punkt innnerhalb von P
	@return:	Nr. des Splitting-Punkts pk
	=================================== */
int pDistMax(Point pi, Point pj, Point* P, int size)
{
	int i;

	Wml::Vector2f pointi;	// Punkt d. Linie
	Wml::Vector2f pointj;	// Punkt d. Linie
	Wml::Segment2f seg;		// Linie bzw. Segment

	Wml::Vector2f pointx;	// Entfernungspunkt
	float dist;
	float maxdist = 0.0f;
	int max_i = 0;

	pointi.X() = pi.x; pointi.Y() = pi.y;
	pointj.X() = pj.x; pointj.Y() = pj.y;

	seg.Origin() = pointi;
	seg.Direction() = pointj-pointi;

	for (i=0;i<size;i++) {							// alle Punkte d. Kurve testen
		pointx.X() = P[i].x; pointx.Y() = P[i].y;
		dist = Wml::Distance(pointx,seg);			// Distanz zw. px und (pipj) berechnen
		if (dist > maxdist) {						// Pkt. m. größter Entfernung finden
			maxdist = dist;
			max_i = i;
		}
	}
/*	Point pk; pk.x = P[max_i].x; pk.y = P[max_i].y;
	std::cout << "Punkt m.d. max. Entfernung: Nr." << max_i << 
		" / Entf." << maxdist << " / (x,y)" << pk.x << "," << pk.y << std::endl;
*/	
	return max_i;
}

/**	===================================
	"Dynamischen" Fehler berechnen:
	Abstände der Punkte innerhalb der Box zur Längsachse der Box
	=================================== */
float dyn_fehler(Point* V, int l, int r, Wml::Box2f &b, Bild2d* bb)
{
	int i_achse;

	Wml::Vector2f point1;			// "linker", ...
	Wml::Vector2f point2;			// "rechter" Punkt der Geraden
	Wml::Line2f line;				// eher ein Segment, aber zunächst egal...
	
	int i;
	Wml::Vector2f pointx;			// Entfernungspunkt
	float dist = 0.0f;

	if (b.Extent(0) > b.Extent(1)) i_achse = 0; else i_achse = 1;

	point1 = b.Center() + b.Axis(i_achse) * b.Extent(i_achse);
	point2 = b.Center() - b.Axis(i_achse) * b.Extent(i_achse);

	line.Origin() = point1;
	line.Direction() = point2-point1;

//	bb->linie(point1.X(),point1.Y(),point2.X(),point2.Y(),VG_FARBE);

	for (i=l;i<r;i++) {
		pointx.X() = V[i].x;
		pointx.Y() = V[i].y;
		dist += Wml::Distance(pointx,line);
	}
	
	if (r-l<4) return 0.0f;	// mmhh, muß hier leider sein..
	return dist/(r-l);
}

/**	===================================
	Strip-Tree Segment speichern
	die gefundene linke und rechte Begrenzung, sowie die Box speichern -> Vektor
	=================================== */
void _striptease(Point* V, int rekursion, int l, int r, float fehler, Wml::Box2f &b, Bild2d* bb)
{
	StripNode sn;
	std::vector<StripNode>::iterator it;

	// vergleichen, ob dieses "Segment" schon einmal im Vektor (=Strip-Tree) vorkommt
	for (it = v_stree.begin(); it != v_stree.end(); ++it) 
		if (it->i_left == l && it->i_right == r) return;

	// noch ein Test: wenn Anzahl Länge > Punkte*2 -> nicht speichern!!

	// --------------------------------
	// hier kommt aber noch ein Test
	// durch den Hintergrund-Vergleich fallen viele Punkte weg, so kann es sein, daß Punkte,
	// die geometrisch (euklidisch) sehr weit voneinander entfernt sind (auch unterschiedl.
	// Winkel, in ein Segment gelegt werden. Das ist zu vermeiden, diese Segmente sollen
	// hier "herausgefiltert" werden
	// --------------------------------
//	int i;
//	for (i=l;i<r;i++)
//		if (abs(V[i].x-V[i+1].x)>30 || abs(V[i].y-V[i+1].y)>30) return;
// --> muß schon vorher bei den Punkten geschehen!

	// "Knoten" anlegen -> im Vektor "v_stree" speichern
	sn.rek = rekursion;
	sn.i_left = l;
	sn.i_right = r;
	sn.b = b;
	sn.fehler = fehler;//dyn_fehler(V,l,r,b,bb);
	v_stree.push_back(sn);

//	if (__ausgabe) 	std::cout << "Rek." << rekursion << ", Interv. [" << l << "-" << r << "], "
//		<< "Fehler: " << fehler <<std::endl;
//		<< "Splitting-Point: Nr." << i_pk <<std::endl;

//	if (sn.fehler > 2.0f) {
//		std::cout << /*i++ <<*/ "[" << l << "-" << r << "]";// << std::endl;
//		std::cout << " Fehler: (" << sn.fehler << ")" << std::endl;
//	}
	// aus noch ungeklärten Gründen sind einige Fehler "exorbitant" hoch 49,... und nehmen
	// auch bei zunehmender Rekursion nicht ab, deshalb hier die Deckelung
//	return (sn.fehler);
}

/**	===================================
	Rekursive Strip-Tree Funktion
	kann entweder
	a) fest bis zu einer best.Tiefe den Baum heruntergehen oder
	b) dynamisch bei Unterschreiten einer Fehlergrenze den Ast verlassen

	@params		V,			Zeiger auf "Liste" von Punkten
				l,			linker Teil
				r,			rechter Teil,
				rekursion,	aktuelle Rekursionstiefe
				bb,			Bild -> zeichnen!

	Problem:	wenn ein Segment nur 2 Punkte "umfaßt", kann keine Bounding Box
				berechnet werden -> also entsteht im Moment eine Lücke
				Abhilfe wäre eine Bounding Box mit der Länge (P1P2) und der Höhe "1"
	=================================== */
void _strip_tree(Point* V, int l, int r, int rekursion, Bild2d* bb)
{
//	int rek_tiefe = st_rekursion;		// Rekursions-Tiefe
	int i;
	int i_pk=0;							// Nr. des Splitting Point
	Wml::Box2f b;

	float fehler;						// der berechnete dyn. Fehler

	// statisch
	if (st_dynamisch==0) if (rekursion > st_rekursion) return;

//	std::cout << "+--------------------------------------------------+" << std::endl;
	Point* H = new Point[(r-l)+1];		// Punkte "umkopieren" V -> H
	for (i=0;i<=(r-l);i++) {
		H[i] = V[(l+i)];
//		std::cout << (l+i) << ",";
	}
//	std::cout << std::endl;

	if ((r-l)+1 </*=*/ 1) return;			// wenn Segment < 2 Punkte
	/**	-----------------------------------
		Umhüllendes Rechteck berechnen
		----------------------------------- */
	b = bounding_rect(H,(r-l)+1,bb);

	// dynamisch
	if (st_dynamisch==1) if (rekursion > st_rekursion) st_rekursion = rekursion;
	/**	-----------------------------------
		Keine feste Rekursionstiefe mehr, stattdessen für das jeweilige Segment
		-> den dynamischen Fehler berechnen,
		wenn der unterhalb eines best. Schwellenwertes liegt,
		-> dann die Box akzeptieren
		----------------------------------- */
	fehler = dyn_fehler(V,l,r,b,bb);
	// dynamisch
	if (st_dynamisch==1)
		if (fehler<ST_DYN_ERROR) {
			_striptease(V,rekursion,l,r,fehler,b,bb);	// speichern
			return;
		}

	// statisch
	if (st_dynamisch==0)
		if (rekursion == st_rekursion) // auf der tiefsten Ebene... (?)
			_striptease(V,rekursion,l,r,fehler,b,bb);

	/**	-----------------------------------
		Splitting Point
		----------------------------------- */
	i_pk = l + pDistMax(V[l],V[(r)],H,(r-l)+1);
	delete[] H;							// H freigeben

	/**	-----------------------------------
		rekursiv aufrufen
		----------------------------------- */
	// hier entscheiden, ob Unterteilung klein genug..
	if (i_pk-l > /*1*/0) _strip_tree(V,l,i_pk,rekursion+1,bb);
	else {
		// statisch
		if (st_dynamisch==0)
			_striptease(V,rekursion,l,r,-1,b,bb);
	}
	if (r-i_pk > /*1*/0) _strip_tree(V,i_pk,r,rekursion+1,bb);
	else {
		// statisch
		if (st_dynamisch==0)
			_striptease(V,rekursion,l,r,-1,b,bb);
	}
}

/**	===================================
	Begrenzungspunkte der Längsachse der (Bounding) Box
	@params		b,			die Box
				i_stree,	lfd.Nr innerhalb des Vektors "v_stree"
				seg,		die Längsachse wird zurückgegeben
//				richtung,	0=von.re.nach.li, 1=von.li.nach.re
				ps,			Startpunkt d.Segments / Schnittpunkt z. Nachbar-Segment
							=> ja was denn von beiden???
				mode		0=wawi, 1=nawi
	=================================== */
void begr_punkte(Wml::Box2f& b, int i_stree, Wml::Vector2f& seg, Point ps, int mode)
{
	int i_achse;

	Wml::Vector2f p1;			// "linker", ...
	Wml::Vector2f p2;			// "rechter" Punkt der Geraden
	
	float dist1,dist2;

	// Längsachse finden <= die ist länger als die Querachse
	if (b.Extent(0) > b.Extent(1)) i_achse = 0; else i_achse = 1;

	p1 = b.Center() + b.Axis(i_achse) * b.Extent(i_achse);
	p2 = b.Center() - b.Axis(i_achse) * b.Extent(i_achse);

// Neu - Orientierung des. Segments : Quadrant 1 oder Quadrant 4 oder watt
	if (mode==0) {
//		v_stree[i_stree].x_pkt = p1.X();
//		v_stree[i_stree].y_pkt = p1.Y();
		v_stree[i_stree].x_pkt = p2.X();	// da x <-> (y) seitenverkehrt
		v_stree[i_stree].y_pkt = p2.Y();
				
//		std::cout << ".." << v_stree[i_stree].x_pkt << "," << v_stree[i_stree].y_pkt << std::endl;

		if (b.Axis(i_achse).X()>0 && b.Axis(i_achse).Y()>0) v_stree[i_stree].orientierung = 1;
		if (b.Axis(i_achse).X()<0 && b.Axis(i_achse).Y()>0) v_stree[i_stree].orientierung = 2;
		if (b.Axis(i_achse).X()<0 && b.Axis(i_achse).Y()<0) v_stree[i_stree].orientierung = 3;
		if (b.Axis(i_achse).X()>0 && b.Axis(i_achse).Y()<0) v_stree[i_stree].orientierung = 4;
	}
// <--

	// hier wird vom Segmentierungspunkt berechnet, das kann v.l.n.r. oder umgekehrt sein
	// datt iss eigentlich keine so tolle Sache hier ...
	dist1 = ps.x-p1.X()*ps.x-p1.X()+ps.y-p1.Y()*ps.y-p1.Y();
	dist2 = ps.x-p2.X()*ps.x-p2.X()+ps.y-p2.Y()*ps.y-p2.Y();

	// Vorgabe: nach rechts zeigend
//	if (p1.X() > p2.X()) seg = p1 - p2;	else seg = p2 - p1;
	// ggf. umkehren
//	if (richtung == 0) seg = -seg;
	if (dist1 < dist2) { seg = p2-p1; /*std::cout << "vor";*/ }
		else { seg = p1-p2; /*std::cout << "zurueck";*/ }

	// hier: speichern der Längsachse
	if (mode==0) v_stree[i_stree].v = seg;

	// .. und die Punktdichte, eigentlich einfach zu berechnen, aber: watt mer hatt,
	// datt hat mer
	v_stree[i_stree].pktdichte = 
		(float)(v_stree[i_stree].i_right-v_stree[i_stree].i_left)/seg.Length();
}

/**	===================================
	Winkel (in Grad)
	- zwischen dem Segment und der Waagerechten
	=================================== */
float winkel_waage(Point* V, int eins)
{
	Wml::Vector2f seg;
	Wml::Vector2f waage;

	begr_punkte(v_stree[eins].b,eins,seg,V[v_stree[eins].i_left],0);

	// Winkel zur Waagerechten, mit in die Funktion "reingeschnorrt"
	waage.X() = 1;					// waagerechten Vektor anliegen
	waage.Y() = 0;
	return acos(seg.Dot(waage) / (seg.Length()*waage.Length()))*DEG;
}
/**	===================================
	Winkel (in Grad)
	- zwischen 2 Segmenten
	=================================== */
float winkel_seg2(Point* V, int eins, int zwei)
{
	Wml::Vector2f seg_li;
	Wml::Vector2f seg_re;

//	std::cout << "li=" << eins << ",re=" << zwei << std::endl;
	begr_punkte(v_stree[eins].b,eins,seg_li,V[v_stree[eins].i_right],1);
	begr_punkte(v_stree[zwei].b,zwei,seg_re,V[v_stree[eins].i_left],1);

	// Wenn xxx. Wendepunkt, dann Koord. des Wendepunkts
	// => a bisserl durch die Brust ins Auge
//	if (v_stree[eins].orientierung==1 && v_stree[zwei].orientierung==2) { }
//		else { v_stree[eins].x_pkt=-1; v_stree[eins].y_pkt=-1; }

	// Winkel zum Nachbarn
	return acos(seg_li.Dot(seg_re) / (seg_li.Length()*seg_re.Length()))*DEG;
}

/**	===================================
	Strip-Tree auswerten
	@return:	Objekt-Typ des Strip-Tree
				0 = Weiss nicht
				1 = Holz
					- 1 Segment, Winkel zur Waagerechten mind. 170 Grad
					- mehrere Segmente
						-- Winkel zwischen diesen nicht größer 100 Grad
						-- Winkel zur Waagerechten ...
				2 = Hand
				3 = offene Hand
				4 = geschl. Hand
				5 = Finger
				6 = Armgelenk
				99 = schnelle Bewegung => kann vom Scanner nicht korrekt erfaßt werden
	=================================== */
int stree_auswerten(Point* V, int anz)
{
	int i;
	float alpha_min				= 360.0f;
	int i_typ					= 0;
	
	// leerer Strip-Tree
	if (v_stree.size()==0) return 0;

	// --------------------------------
	// Strip-Tree mit 1 Segment => Gewichtung nicht hier
	// --------------------------------
	if (v_stree.size()==1) {								// Winkel zur Waagerechten
		v_stree[0].winkel_waage = winkel_waage(V,0);
		v_stree[0].winkel_nach = 0;
		return i_typ;
	}

	// --------------------------------
	// Strip-Tree mit mehreren Segmenten
	// => Gewichtung der Winkel => später!!
	// --------------------------------
	for (i=0;i<v_stree.size();i++) 							// Winkel zur Waagerechten
		v_stree[i].winkel_waage = winkel_waage(V,i);
	for (i=0;i<v_stree.size()-1;i++) {
		v_stree[i].winkel_nach = winkel_seg2(V,i,i+1);		// Winkel zum Nachbar-Segment
		if (v_stree[i].winkel_nach < alpha_min) alpha_min = v_stree[i].winkel_nach;

		// Wenn xxx. Wendepunkt, dann Koord. des Wendepunkts
		// => a bisserl durch die Brust ins Auge
		if (v_stree[i].orientierung==1 && v_stree[i+1].orientierung==2) { ;}
			else { v_stree[i].x_pkt=-1; v_stree[i].y_pkt=-1; }
	}
	v_stree[i].winkel_nach = 0;
	
//	if (alpha_min>100/* && */) return 1;
	return i_typ;
}

/**	===================================
	Strip-Tree ausgeben (->Screen) in der Typ-gerechten Farbe!
	@params		i_typ
				vtree_li,vtree_re = Grenze (li,re) des auszugebenden Teils

	- den ganzen Strip-Tree
	- einen Teil des Strip-Trees
	=================================== */
void stree_ausgeben(Config* conf, Bild2d* bb, int i_typ, int vtree_li, int vtree_re)
{
	int i;
	int i_col, i_cl = 100;
//	float pktdichte_toleranzbereich;
	float laenge;

	int wi_na, wi_wa, lae;
	int xx,yy;

	if (i_typ == 0) i_col = PUNKT_GELB;	// weiss nicht
	if (i_typ == 1) i_col = PUNKT_BLAU;	// Holz
	if (i_typ == 2) i_col = PUNKT_ROT;	// Hand
	if (i_typ == 3) i_col = PUNKT_ROT;	// Hand offen
	if (i_typ == 4) i_col = PUNKT_ROT;	// Hand geschl.
	if (i_typ == 5) i_col = PUNKT_ROT;	// Finger
	if (i_typ == 6) i_col = PUNKT_ROT;	// Arm
	if (i_typ == 99) i_col = PUNKT_ROT;	// schnelle Bewegung

	laenge = 0.0f;
	for (i=vtree_li;i<vtree_re;i++) {
		// --------------------------------
		// Ausgabe Konsole
		// --------------------------------
		wi_na = (int)v_stree[i].winkel_nach;
		wi_wa = (int)v_stree[i].winkel_waage;
		lae = (int)(v_stree[i].v.Length()/10.0f);

		if (__ausgabe) std::cout << "seg." << i
			<< ",r=" << v_stree[i].rek
			<< ",p[" << v_stree[i].i_left << "-" << v_stree[i].i_right << "]"
			<< ",o=" << v_stree[i].orientierung	// Neu
//			<< ",x_pkt=" << v_stree[i].x_pkt	// Neu
//			<< ",y_pkt=" << v_stree[i].y_pkt	// Neu
			<< ",lae=" << (lae>999?999:lae)
			<< ",wina=" << (wi_na<-1?-1:wi_na)
			<< ",wiwa=" << (wi_wa<-1?-1:wi_wa)
			<< ",fhl=" << v_stree[i].fehler
			<< ",pkt.di=" << v_stree[i].pktdichte << std::endl;
		
		// wenn Sigma zu gross, d.h Unterschiede zwischen dichtbepacjten Segmenten und
		// dünn besiedelten sehr ausgeglichen (hoch), dann das Mittel als Schwelle setzen
/*		if (v_stree_pktdichte_sigma > v_stree_pktdichte_mittel/2.0f)
			v_stree_pktdichte_sigma = 0;
		// die fetten Segmente
		pktdichte_toleranzbereich = v_stree_pktdichte_mittel - v_stree_pktdichte_sigma;
		if (v_stree[i].pktdichte < pktdichte_toleranzbereich) {
			i_cl = 100; 
		} else {
			laenge += v_stree[i].v.Length();
*/			i_cl = i_col;
//		}

		// --------------------------------
		// Ausgabe Bild2d
		// --------------------------------
		zeichne_box(v_stree[i].b,i_cl,bb);

	}
	// -----------------------------------
	// Auswertung der Wendepunkte (=> Finger) (so gut wie) dahintergeklemmt
	// -----------------------------------
	for (i=vtree_li;i<vtree_re-1;i++) {
		if (v_stree[i].x_pkt>-1 && v_stree[i].y_pkt>-1) {
			xx = (int)v_stree[i].x_pkt;
			yy = (int)v_stree[i].y_pkt;
			if (wp_einblenden>=1) zeichne_pfeil(xx,yy-10,PUNKT_GRUEN,bb);
			if (wp_einblenden==1) bb->linie(xx,yy,xx,hintergrund_y-conf->_y1,PUNKT_GRUEN);
		}
	}
//	if (__ausgabe) std::cout <<std::endl<< "laenge=" << laenge << std::endl;
	if (__ausgabe) std::cout << std::endl;
}

/**	===================================
	Winkel-Statistik berechnen
	=> Winkel zur Waagerechten (Wi-Wa) Mittelwert und Sigma

	und zwar
	- für den gesamten Strip-Tree (Vektor) oder
	- einen Teil des Strip-Trees (wenn z.B. Hände auf Brett einen Cluster bilden)
	=================================== */
void winkel_stat(float& wi_mittel, float& wi_sigma, int vtree_li, int vtree_re)
{
	int i;

	float alpha;
	float alpha_sum_waage_wi	= 0.0f;
	float len_sum				= 0.0f;

	float alpha_waage_mittel	= 0.0f;
	float sigma_waage_wi		= 0.0f;

	// --------------------------------
	// Auswertung der Winkel
	// 
	// wichtiges Detail: für die Statistik werden die Vektoren,
	// welche die Winkel mit der Waagerechten aufspannen, alle gleich
	// ausgerichtet, so daß die Winkel im selben Quadranten liegen.
	// Das macht die statitische Aussage "stabiler"
	// --------------------------------
	for (i=vtree_li;i<vtree_re;i++) {
		alpha = v_stree[i].winkel_waage;
		if (alpha < 90) alpha = 180.0f - alpha;
		alpha_sum_waage_wi += alpha*v_stree[i].v.Length()*(v_stree[i].i_right-v_stree[i].i_left);

		len_sum += v_stree[i].v.Length()*(v_stree[i].i_right-v_stree[i].i_left);
	}
	alpha_waage_mittel = alpha_sum_waage_wi/len_sum;

	// Standardabweichung
	for (i=vtree_li;i<vtree_re;i++) {
		alpha = v_stree[i].winkel_waage;
		if (alpha < 90) alpha = 180.0f - alpha;
		sigma_waage_wi += abs((alpha - alpha_waage_mittel) * v_stree[i].v.Length()*(v_stree[i].i_right-v_stree[i].i_left));
	}
	sigma_waage_wi = sigma_waage_wi/len_sum;

	wi_mittel = alpha_waage_mittel;
	wi_sigma = sigma_waage_wi;
}

void punktdichte(int vtree_li, int vtree_re)
{
	int i;
	float diff;

	// --------------------------------
	// Punktdichte
	// --------------------------------
	v_stree_pktdichte_mittel = 0.0f;		// mittlere Punktdichte
	for (i=vtree_li;i<vtree_re;i++)
		v_stree_pktdichte_mittel += v_stree[i].pktdichte;
	v_stree_pktdichte_mittel /= (vtree_re-vtree_li);

	v_stree_pktdichte_sigma = 0.0f;			// Std.-abweichung v.d.mittl.Punktdichte
	for (i=vtree_li;i<vtree_re;i++) {
		diff = v_stree_pktdichte_mittel - v_stree[i].pktdichte;
		v_stree_pktdichte_sigma += (diff * diff);
	}
	v_stree_pktdichte_sigma = sqrt(v_stree_pktdichte_sigma/(float)(vtree_re-vtree_li));

	// Pkt.dicht ausgeben
	if (__ausgabe) std::cout << "pkt.dichte_mi=" << v_stree_pktdichte_mittel << ", "
		<< "pkt.dichte_sigma=" << v_stree_pktdichte_sigma << std::endl;
}

/**	===================================
	Verhältnis zwischen
	- eher waagerechten und
	- eher senkrechten/diagonal ausgerichteten Segmenten
	berechnen

	für den gesamten Strip-Tree
	=================================== */
float winkel_anteile()
{
	int i;
	float alpha;
	float waagerechte = 0.0f;
	float diagonale = 0.0f;

	for (i=0;i<v_stree.size();i++) {
		alpha = v_stree[i].winkel_waage;
		if (alpha < 90) alpha = 180.0f - alpha;
		if (alpha > 170) waagerechte += v_stree[i].v.Length();
		if (alpha < 145) diagonale += v_stree[i].v.Length();
	}
	if (waagerechte==0) waagerechte = 1.0f;
//	std::cout << "waage=" << waagerechte << ",dia=" << diagonale << std::endl;
//	return diagonale/waagerechte;
	return 100.0f/diagonale*waagerechte;	// Rückgabe in Prozent
}

/**	===================================
	gefundene Objekte hinzufügen (Variable in der "main.cpp")
	und ggf. in eine Datei schreiben

	@return		-1	= kein Objekt hinzugefügt
				1	= o.k.
	=================================== */
int Bild::neues_objekt_stree(Config* conf, Point* V, int punkt_li, int punkt_re,
	int vtree_li, int vtree_re,	int i_typ, int grad, float proz_wawi)
{
	int i;
	float fehler_max = 0.0f;
	Point p;
	int x_min,x_max,y_min,y_max;

	float laenge_mittel = 0.0f;

	float f_tmp;		// kann man mal bequem ein paar Sachen durchrechnen
						// gut in Zeiten von C++ könnte man auch einen stringstream benutzen
	float diff_x,diff_y;
	Objekt ob;

	std::vector<float> wp_x;
	std::vector<float> wp_y;
	// --------------------------------
	// Neu - Wendpunkte
	// --------------------------------
	for (i=vtree_li;i<vtree_re;i++) {
		if (v_stree[i].x_pkt>-1 && v_stree[i].y_pkt>-1) {
			wp_x.push_back(v_stree[i].x_pkt);
			wp_y.push_back(v_stree[i].y_pkt);
		}
	}
	ob.n_wp = wp_x.size();					// Anzahl Wendepunkte

// warum ist diese Abfrage hier notwendig???
if (wp_x.size()>0) {
	ob.wp_x_mi = 0.0f; ob.wp_y_mi = 0.0f;	// mittlerer Abstand der Wendepunkte
	for (i=0;i<wp_x.size()-1;i++) {
		ob.wp_x_mi += abs(wp_x[i]-wp_x[i+1]);
		ob.wp_y_mi += abs(wp_y[i]-wp_y[i+1]);
	}
	ob.wp_x_mi /= wp_x.size();
	ob.wp_y_mi /= wp_y.size();// x ist gleich y
//	std::cout << "x/y-mi=" << ob.wp_x_mi << "," << ob.wp_y_mi;

	ob.wp_x_std = 0.0f; ob.wp_y_std = 0.0f;	// Std.-abweichung d. mittl. Abstands der WP
	for (i=0;i<wp_x.size()-1;i++) {
		diff_x = (wp_x[i]-wp_x[i+1])-ob.wp_x_mi; ob.wp_x_std += (diff_x * diff_x);
		diff_y = (wp_y[i]-wp_y[i+1])-ob.wp_y_mi; ob.wp_y_std += (diff_y * diff_y);
	}
	ob.wp_x_std = sqrt(ob.wp_x_std/wp_x.size());
	ob.wp_y_std = sqrt(ob.wp_y_std/wp_y.size());// x ist gleich y
//	std::cout << "x/y-std=" << ob.wp_x_std << "," << ob.wp_y_std << std::endl;
} else {
	ob.wp_x_mi = 0.0f;
	ob.wp_y_mi = 0.0f;
	ob.wp_x_std = 0.0f;
	ob.wp_y_std = 0.0f;
}	// <-- Wendepunkte

	// Rekursions-Grad
	ob.grad = grad;

	// max.Fehler der Segmente summieren
	for (i=vtree_li;i<vtree_re;i++)
		if (v_stree[i].fehler > fehler_max) fehler_max = v_stree[i].fehler;
	ob.fehler = fehler_max;

	// Anzahl d. Segmente
	ob.n_segmente = vtree_re-vtree_li;

	// --------------------------------
	// achsenorientierte Bounding-Box (AABB) berechnen (x/y - min u. max)
	// --------------------------------
	x_min = 9999; x_max = 0;
	y_min = 9999; y_max = 0;
	for (i=punkt_li;i<punkt_re;i++) {
		p = V[i];
		if (p.x > 0) if (p.x < x_min) x_min = p.x;
		if (p.x > 0) if (p.x > x_max) x_max = p.x;
		if (p.y > 0) if (p.y < y_min) y_min = p.y;
		if (p.y > 0) if (p.y > y_max) y_max = p.y;
	}
	ob.x_mi = conf->_width - (x_min+(x_max - x_min)/2);
	ob.x_gr = abs(x_max - x_min);
	ob.y_mi = y_min + (y_max - y_min)/2;
	ob.y_gr = abs(y_max - y_min);

	// Erhebung über dem Tisch
	ob.y_erhebung = hintergrund_y - (y_min+conf->_y1); 
	
	// Na-Wi = Winkel zum Nachbar-Segment, Wa-Wi = Winkel zur Waagerechten
	winkel_stat(ob.wi_waage_mittel,ob.wi_waage_sigma,vtree_li,vtree_re);
	
	// mittlere Punktdichte und deren Std.-abweichung für den STree-Teil berechnen
	punktdichte(vtree_li,vtree_re);

	i_typ = 0;	// Vorbelegung mit "weiss nicht"

	if (ob.n_segmente == 1) {
//		if (ob.x_gr/10.0f > 8.0f)				// Holz muss breiter als 8cm sein
			if (v_stree[vtree_li].winkel_waage >= 167.0f) i_typ = 1;			// Holz
			// da es nur 1 Segment gibt, wird das Mittel und das Sigma hier "freihand" gesetzt
			ob.wi_waage_mittel = v_stree[vtree_li].winkel_waage;
			ob.wi_waage_sigma = 0.0f;
	}
	if (ob.n_segmente > 1) {
		// --------------------------------
		// Holz
		// --------------------------------
//		if (ob.x_gr/10.0f > 3.0f)				// Holz muss breiter als 3cm sein
			if (ob.wi_waage_mittel >= 167.0f && ob.wi_waage_sigma < 14) i_typ = 1;// Holz

		// --------------------------------
		// Hand offen
		// --------------------------------
		if (ob.wi_waage_mittel >= 120.0f && ob.wi_waage_mittel <= 148.0f
			&& ob.wi_waage_sigma <= 12) i_typ = 3;
		// durch Quadranten-Zusammenführung der Wa-Winkel wird [70...100] zu [90...110]
		if (ob.wi_waage_mittel >= 90.0f && ob.wi_waage_mittel <= 120.0f
			&& ob.wi_waage_sigma < 10.0f) i_typ = 3;

		// --------------------------------
		// Hand, geschlossen
		// --------------------------------
		if (ob.wi_waage_mittel > 90 && ob.wi_waage_mittel <= 148.0f
			&& ob.wi_waage_sigma > 12.0f) i_typ = 4;

		if ((i_typ == 3 || i_typ == 4) && ob.n_segmente == 2) i_typ = 5;	// 2 Segmente => Finger
	}
	ob.proz_wawi = proz_wawi;

	// --------------------------------
	//  "Schlonz" aussortieren (mittl.Länge < 1cm)
	// => über die durchschnittliche Länge der Segmente
	// --------------------------------
	for (i=vtree_li;i<vtree_re;i++)
		laenge_mittel += v_stree[i].v.Length();
	laenge_mittel /= (float)(vtree_re-vtree_li); 
	if (__ausgabe==1) std::cout << ">>mittl.lae=" << laenge_mittel << std::endl;
	if (laenge_mittel/10.0f < 1.0f) i_typ = 0;

	// HOT!!!!!! Fix
	f_tmp = (float)ob.x_gr/10.0f;
//	if (f_tmp>=0.95f && f_tmp<=2.4f) i_typ = 5;				// Finger-Breite
//	else if (f_tmp>=8.0f && f_tmp<=10.5f &&					// Hand-Breite 
//		i_typ!=3 && i_typ!=4) i_typ = 2;
	// => auskommentiert, da auch jedes "Hand-auf-Holz" Holzstück als Hand erkannt
	// wird
//	else if (f_tmp>=39.0f && f_tmp<=41.0f) i_typ = 1;		// Holz-Breite

	// --------------------------------
	// schnelle Bewegungen herausfiltern
	// Anz.Segmente > Breite*1.5f
	// aber keine einzelnen Segmente
	// --------------------------------
	if (ob.n_segmente > 5 && ob.n_segmente > ob.x_gr*1.4f/10.0f) i_typ = 99;

	ob.typ = i_typ;

	// --------------------------------
	//  "DECKELUNG"
	//	wenn Objekt-Mitte "unterhalb" des Tisches
	//  => kein relevantes Objekt => nicht speichern
	// --------------------------------
	if (ob.y_mi+conf->_y1 > hintergrund_y) return -1;

	// die ganzen Objekt-Angaben (Bild für Bild) in eine Datei schreiben
	if (statistik_schreiben==1) {
		ostat << "1;" << i_typ << ";"
			<< ob.n_segmente << ";"
			<< ob.grad << ";"
			<< ob.fehler << ";"
			<< ob.x_gr << ";"
			<< ob.y_gr << ";"
			<< ob.y_erhebung << ";"
			<< (ob.wi_waage_mittel<-1?-1:ob.wi_waage_mittel) << ";"
			<< (ob.wi_waage_sigma<-1?-1:ob.wi_waage_sigma) << ";" << "\n";
	}

	// Objekt im Objekt-Vektor speichern
	objekte.push_back(ob);
	return i_typ;
}

/*void Bild::strip_tree_auslagern(Config* conf, Point* V, int clust_nr, int grad, int st_rekursion)
{
	int i;
	int i_stree_typ;							// 0 = weiß nicht
												// 1 = Holz
												// 2 = Hand
												// 3 = offene Hand
												// 4 = geschl. Hand
	for (i=0;i<v_stree.size();i++) {
		if (v_stree[i].winkel_waage > 170.0f) {
			i_stree_typ = 1;
			// Variable in der main.cpp setzen (hier als extern deklariert)
			i_stree_typ = neues_objekt_stree(conf,V,clust_nr,i_stree_typ,st_rekursion);
			stree_ausgeben(conf,bb,i_stree_typ);
		}
	}
}
*/

/**	===========================================================================
	Strip-Tree Algorithmus von Ballard
	
	Bounding Box :
	- konvexe Hülle berechnen (Melkman) - computational geometry
	- dann rotating calipers (G.Toussaint) - comp. geom.
	- dann splitting point
	- usw...
	=========================================================================== */
void Bild::strip_tree(Config *conf, Bild* hintergrund)
{
	int i,j;
	int size = this->data.size();				// Anz.Punkte insges. (können auch
												// ausserhalb d. Fensters liegen)

	int i_stree_typ;							// 0 = weiß nicht
												// 1 = Holz
												// 2 = Hand
												// 3 = offene Hand
												// 4 = geschl. Hand

	Point* V = new Point[size];					// Punkte innerhalb des Fensters (lt.Config)
	// --------------------------------
	// verschiedene Funktionen:
	// pol. in kart. Koord. umwandeln
	// + Hintergrund abziehen (opt.)
	// + Ausreisser aussortieren (opt.)
	// + Cluster bilden (opt.)
	// --------------------------------
	int anz = this->to_xy(conf,1,V,hintergrund_abziehen,hintergrund);
	punkte_im_fenster = anz;					// anz = Anz.Input-Punkte

	int r = 0;									// Rechte Begrenzung des Kurvenzugs innerhalb d. Fensters
												// ... und die linke Begrenzung

	float proz_wawi;							// Prozent waagerechte Winkel
	int punkt_li,punkt_re,vtree_li,vtree_re;

/**	c++ tree siehe: http://www.damtp.cam.ac.uk/user/kp229/tree/ */	

/**	-----------------------------------
	// Neuer Vorschlag:
	// anstatt einem "tree" wird nun ein "vector" (STL) mit den Teilen
	// des Baums gefüllt, dabei wird ein Zeiger mitgeliefert, welches Element denn
	// gerade behandelt wird
	----------------------------------- */
//	v_stree.clear();

	Bild2d* bb = new Bild2d(ptcWIDTH, ptcHEIGHT);

	// zuerst den Hintergrund zeichnen
	if (hintergrund_abziehen>=1) hg_punkte_zeichnen(conf,hintergrund,bb);
	if (hintergrund_abziehen>1) hg_gerade_zeichnen(conf,hintergrund,bb);

//	std::cout << "====================================" << std::endl;
//	objekte.clear();  -> in der main()
/**	-----------------------------------
	Algorithmus aufrufen -> rekursiv
	----------------------------------- */
	// Neu - m. (Kellogg's) Clustern
	if (hintergrund_abziehen>=1) {
		// jeden Cluster ...
		for (i=0;i<v_cluster_li.size();i++) {
			if (st_dynamisch) st_rekursion = 1;	// bei dynamischer Rekursion wird die Variable
												// "st_rekursion" nur noch zur Anzeige der erreichten
												// Tiefe genutzt u. deshalb hier mit 1 initialisiert

			// Cluster mit einer Punktzahl < (4) werden aussortiert
			if (v_cluster_re[i]-v_cluster_li[i] <= CLUSTER_MINGROESSE) {
				if (__ausgabe) std::cout << "STree(" << i << "): Cluster < " << CLUSTER_MINGROESSE << std::endl;
				continue;
			}

			v_stree.clear();
			if (__ausgabe) std::cout << "STree(" << i << "): " << v_cluster_li[i] << "-" << v_cluster_re[i] << std::endl;
			// -------------------
			//  Strip Tree
			// -------------------
			_strip_tree(V,v_cluster_li[i],v_cluster_re[i],0,bb);

			// -------------------
			//  Baum "auswerten"
			// -------------------
			i_stree_typ = stree_auswerten(V,anz);

			// Prozentuellen Anteil der "waagerechten" (> 170 Grad) Segmente berechnen
			proz_wawi = winkel_anteile();
			if (proz_wawi > 20.0f && proz_wawi < 250.0f) {	// hier kann man auch höhere Obergrenze,
															// das macht die Sache sicherer,
															// dann wird auch ein einzelner Finger, der
															// auf einem Brett liegt, erkannt.
															// dadurch steigt aber auch der "fragmentier-
															// und schlonz-Faktor"
															//
															// bei kleinerer Untergrenze wir ein Cluster eher
															// auf Holzanteile untersucht
				// --------------------
				// Verbund-objekt
				// => aufteilen in neue Strip-Trees
				// --------------------
				j = 0;
				while(j<v_stree.size()) {

					punkt_li = v_stree[j].i_left;
					vtree_li = j;
					while (v_stree[j].winkel_waage>=167.0f && j<v_stree.size()) j++;
					// Erstes Segment "waagerecht" ??
					if (j>0) {
					// ----------------
					// "Brett" Segment(e)
					// ----------------
						punkt_re = v_stree[j-1].i_right;
						vtree_re = j;
						// Variable in der main.cpp setzen (hier als extern deklariert)
						i_stree_typ = neues_objekt_stree(conf,V,punkt_li,punkt_re,
							vtree_li,vtree_re,i_stree_typ,st_rekursion,proz_wawi);
						stree_ausgeben(conf,bb,i_stree_typ,vtree_li,vtree_re);
					}

					if (j==v_stree.size()) continue;

					// ----------------
					// "nicht-Brett" Segment(e)
					// ----------------
					punkt_li = v_stree[j].i_left;
					vtree_li = j;
					while (v_stree[j].winkel_waage<167.0f && j<v_stree.size()) j++;
					punkt_re = v_stree[j-1].i_right;
					vtree_re = j;

					i_stree_typ = neues_objekt_stree(conf,V,punkt_li,punkt_re,
						vtree_li,vtree_re,i_stree_typ,st_rekursion,proz_wawi);
					stree_ausgeben(conf,bb,i_stree_typ,vtree_li,vtree_re);

					if (j==v_stree.size()) continue;
				}
			}
			else
			{
				// --------------------
				// 1 Objekt
				// --------------------
				punkt_li = v_cluster_li[i];
				punkt_re = v_cluster_re[i];
				vtree_li = 0;
				vtree_re = v_stree.size();
				// Variable in der main.cpp setzen (hier als extern deklariert)
				i_stree_typ = neues_objekt_stree(conf,V,punkt_li,punkt_re,
					vtree_li,vtree_re,i_stree_typ,st_rekursion,proz_wawi);
				stree_ausgeben(conf,bb,i_stree_typ,vtree_li,vtree_re);
			}
		}
	}

	if (hintergrund_abziehen==0) {
		if (st_dynamisch) st_rekursion = 1;	// bei dynamischer Rekursion wird die Variable
											// "st_rekursion" nur noch zur Anzeige der erreichten
											// Tiefe genutzt u. deshalb hier mit 1 initialisiert
		v_stree.clear();

		r = anz-1;
		_strip_tree(V,0,r,0,bb);			// z.B.: in=326 (Punkte) -> [l=0,r=325]

		// -------------------
		//  Baum "auswerten" + ausgeben
		// -------------------
// hier (noch) zu schwierig
		stree_auswerten(V,anz);

		vtree_li = 0;
		vtree_re = v_stree.size();
		stree_ausgeben(conf,bb,-1,vtree_li,vtree_re);
	}

	// zuletzt die sich vom Hintergrund abhebenden Punkte zeichnen
	if (punkte_ausblenden == 0) for (i=0;i<anz;i++) bb->setpixel(V[i].x,V[i].y,VG_FARBE);

	delete[] V;

	bb->toScreen();
	bb->~Bild2d();
}

/**	===================================
	zum x-Wert den y-Wert (mittels der Koeffizienten aus "a") ausrechnen
	das Array2D "a" ist tatsächlich 1-dimensional
	=================================== */
float getY(int x, TNT::Array2D<float> a, int n){
	int i;
	float y = 0;
	for (i=0;i<n;i++) {
		y+= a[i][0]*pow((float)x,(float)i);
	}
	return y;
}

/**
*/
float _poly_regression(Config* conf, Point* V, int l, int r, int degree, TNT::Array2D<float> &s)
{
	int i,j,x;

	TNT::Array2D<float> mat((r-l),degree);		// der Rang legt die "Höhe" der Matrix fest
	TNT::Array2D<float> bmat((r-l),1);
//	TNT::Array2D<float> s;//(anz,1);

	float dist = 0.0f;

	for (j=0;j<degree;j++)						// Vandermonde-Matrix aufbauen
		for (i=l;i<r;i++)
			mat[i-l][j] = pow((float)V[i].x,(float)j);	// x^0 = 1
		
	for (i=l;i<r;i++)							// b-Matrix (y)
		bmat[i-l][0] = V[i].y;

	try {
		// QR-decomposition (Householder) Q=Orthogonal Factor, R=Upper Triangle Factor
		JAMA::QR<float> qr(mat);
		s = qr.solve(bmat);
		// SVD
//		JAMA::SVD<float> svd(mat);
//		...
	} catch (std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	// degree x 1 Matrix, aha!!
//	std::cout << "Solution (Grad " << degree << "): " << s << std::endl << std::endl;

	// --------------------------------
	//  Fehler berechnen => Summe(delta_y)/N
	// --------------------------------
	for (i=0;i<(r-l);i++) {
		x = (int)V[l+i].x;
		// wenn dim2==0 und getY-Aufruf, dann Runtime-Error
		if (s.dim2()>0) dist += (float)abs(V[l+i].y - getY(x,s,degree));
	}
	return (dist/(r-l));
}

int Bild::neues_objekt_poly(Config* conf, Point* V, int clust_nr, int i_typ, int grad,
	float fehler)
{
	int i;
	Point p;
	int x_min,x_max,y_min,y_max;

	float f_tmp;		// kann man mal bequem ein paar Sachen durchrechnen
						// gut in Zeiten von C++ könnte man auch einen stringstream benutzen
	Objekt ob;
	ob.grad = grad;
	ob.fehler = fehler;

	// --------------------------------
	// achsenorientierte Bounding-Box berechnen (x/y-min u. max)
	// --------------------------------
	x_min = 9999; x_max = 0;
	y_min = 9999; y_max = 0;
	for (i=v_cluster_li[clust_nr];i<v_cluster_re[clust_nr];i++) {
		p = V[i];
		if (p.x > 0) if (p.x < x_min) x_min = p.x;
		if (p.x > 0) if (p.x > x_max) x_max = p.x;
		if (p.y > 0) if (p.y < y_min) y_min = p.y;
		if (p.y > 0) if (p.y > y_max) y_max = p.y;
	}
	ob.x_mi = conf->_width - (x_min+(x_max - x_min)/2);
	ob.x_gr = abs(x_max - x_min);
	ob.y_mi = y_min + (y_max - y_min)/2;
	ob.y_gr = abs(y_max - y_min);

	ob.y_erhebung = hintergrund_y - (y_min+conf->_y1); 

	// HOT!!!!!! Fix
	f_tmp = (float)ob.x_gr/10.0f;
//	if (f_tmp>=0.95f && f_tmp<=2.4f) i_typ = 5;				// Finger-Breite
//	else if (f_tmp>=8.0f && f_tmp<=10.5f) i_typ = 2;		// Hand-Breite
//	else if (f_tmp>=39.0f && f_tmp<=41.0f) i_typ = 1;		// Holz-Breite

	// --------------------------------
	//  "DECKELUNG"
	//  wenn Objekt-Mitte "unterhalb" des Tisches
	//  => kein relevantes Objekt => nicht speichern
	// --------------------------------
	if (ob.y_mi+conf->_y1 > hintergrund_y) return -1;

	ob.typ = (i_typ<0 ? 0 : i_typ);
//	std::cout << "o-typ" << ob.typ << "  ";

	// Objekt im Objekt-Vektor speichern
	objekte.push_back(ob);
	return i_typ;
}

/**	===========================================================================

	Polynomielle Regression (mathematisch auch als lineare anzusehen)
	A * x = b	

	Lösen der Linear Least Squares mittels QR-Dekomposition

	ist bei 500KBaud etwas zu langsam
	Abhilfe:	- nur jede zweite Zeile 'rüberschicken
				- von "double" auf "float" umstellen
	=========================================================================== */
void Bild::poly_regression(Config* conf, Bild* hintergrund)
{
	const float nachbarschaft = 7.0f;
	float fehler, fehler2, fehler3;
	float fehler_alt;

	int i,j,k,x;
	int degree;// = 5;//wir starten mit dem Grad 5, früher hier gesteuert: "poly_degree"
	int size = this->data.size();

	int i_poly_typ;								// 0 = weiß nicht
												// 1 = Holz
												// 2 = Hand
												// 3 = offene Hand
												// 4 = geschl. Hand

	int poly_l,poly_r;

	Point* V = new Point[size];					// Input
	// --------------------------------
	// verschiedene Funktionen:
	// pol. in kart. Koord. umwandeln
	// + Hintergrund abziehen (opt.)
	// + Ausreisser aussortieren (opt.)
	// + Cluster bilden (opt.)
	// --------------------------------
	int anz = this->to_xy(conf,1,V,hintergrund_abziehen,hintergrund);
	punkte_im_fenster = anz;

	float dist = 0.0f;
	short i_col;

	TNT::Array2D<float> s;						// das Ergebnis-Array (1-dim Matrix)

	Bild2d* bb = new Bild2d(ptcWIDTH, ptcHEIGHT);

	if (hintergrund_abziehen>=1) hg_punkte_zeichnen(conf,hintergrund,bb);
	if (hintergrund_abziehen>1) hg_gerade_zeichnen(conf,hintergrund,bb);

	// Neu - m. (Kellogg's) Clustern
	if (hintergrund_abziehen==0) {				// ohne Hintergrund
		v_cluster_li.clear();
		v_cluster_re.clear();
		v_cluster_li.push_back(0);				// => EINEN großen Cluster anlegen!
		v_cluster_re.push_back(anz);
	}
	// jeden Cluster ...
	for (i=0;i<v_cluster_li.size();i++) {
		// Cluster mit einer Punktzahl < (4) werden aussortiert
		if (v_cluster_re[i]-v_cluster_li[i] <= CLUSTER_MINGROESSE) continue;

		// ----------------------------
		//  hier geht's los!!!
		// ----------------------------

		// statisch
		if (poly_dynamisch==0) degree = poly_degree;
		// dynamisch
		if (poly_dynamisch==1) if (hintergrund_abziehen==0) degree = 14; else degree = 1;

		// bei poly-statisch hier nur 1 "Durchlauf"
		fehler = _poly_regression(conf,V,v_cluster_li[i],v_cluster_re[i],degree,s);
		fehler_alt = fehler+1;

		// dynamisch - wesentlich langsamer (im Moment), weil:
		// -> fange bei 1 an und probiere dann alle weiteren Grade aus
		if (poly_dynamisch==1) {
			// --------------------------------
			// solange bis der Fehler (oder Fehler++, oder (Fehler++)++)
			// größer als der alte Fehler ist
			// --------------------------------
			while (
				(fehler > POLY_ERROR_MIN) && (degree < POLY_DEGREE_MAX) &&
				(fehler_alt > fehler || fehler_alt > fehler2 || fehler_alt > fehler3)
				) {
				degree++;
				fehler_alt = fehler;
				if (__ausgabe) std::cout << "polyn." << i << ",grd." << degree-1 << ",fehler_alt " << fehler_alt << std::endl;
				// nächsthöheren Grad testen
				fehler = _poly_regression(conf,V,v_cluster_li[i],v_cluster_re[i],degree,s);
				if (__ausgabe) std::cout << "polyn." << i << ",grd." << degree << ",fehler " << fehler << std::endl;
				// und 2 Grade höher testen
				fehler2 = _poly_regression(conf,V,v_cluster_li[i],v_cluster_re[i],degree+1,s);
				if (__ausgabe) std::cout << "polyn." << i << ",grd." << degree+1 << ",fehler2 " << fehler2 << std::endl;
				// und 3 Grade höher testen
				fehler3 = _poly_regression(conf,V,v_cluster_li[i],v_cluster_re[i],degree+2,s);
				if (__ausgabe) std::cout << "polyn." << i << ",grd." << degree+2 << ",fehler3 " << fehler3 << std::endl;
//				if (fehler2==0) break; // ansch.ein "unsicherer" Zustand
			}
			// --------------------------------
			// dann muß nochmal der "alte" Grad, der ja einen kleineren Fahler hatte,
			// berechnet werden
			// --------------------------------
			if (fehler > POLY_ERROR_MIN) degree--;
			fehler = _poly_regression(conf,V,v_cluster_li[i],v_cluster_re[i],degree,s);
			poly_degree = degree;
		}
		// bei statischem Polynom wird nur 1 auf "std::cout" angezeigt
		if (__ausgabe) std::cout << "polyn." << i << ",grd." << degree << ",fehler " << fehler << std::endl;
		if (__ausgabe) std::cout << std::endl;

		// ----------------------------
		//  Anhand des Polynomgrades eine Einteilung (Klassifizierung)
		//  in einen Objekt-Typen vornehmen
		// ----------------------------
		i_poly_typ = 0;							// Vorbelegung mit "weiss nicht"

		// ----------------------------
		// Holz / gerades Werkstück
		// ----------------------------
		if (degree <= 3 && fehler <= POLY_ERROR_MIN) i_poly_typ = 1;
		if (degree == 4 && fehler <= POLY_ERROR_MIN) i_poly_typ = 1;
		// ----------------------------
		// Hand
		// ----------------------------
		if (degree >= 5 && fehler > POLY_ERROR_MIN) i_poly_typ = 2;
//		if (i_poly_typ == 2 && Breite kleiner als 8 cm => Finger!!)

//		if (poly_dynamisch==1)
		i_poly_typ = neues_objekt_poly(conf,V,i,i_poly_typ,degree,fehler);

	//	Bild2d* bb = new Bild2d(ptcWIDTH, ptcHEIGHT);
		
		if (s.dim2() == 0)
			// Rückgabe-"Vektor" hat keine Dimension -> keine Koeffizienten
			std::cout << "Dimension2=0" << std::endl;
		else {
			// ------------------------
			// Gerade bzw. Polynom zeichnen => m.Farbcodierung nach Objekt-Typ
			// todo: nur wenn durch Punkte "unterstuetzt"
			// ------------------------
			poly_l = 0;
			poly_r = ptcWIDTH;
			poly_l = V[v_cluster_li[i]].x;	// Polygon nur innerhalb der x-Grenzen des Clusters
			poly_r = V[v_cluster_re[i]].x;	// darstellen
			//std::cout << "poly_l=" << poly_l << ",poly_r=" << poly_r << std::endl;
			if (poly_l > poly_r) {k = poly_l;poly_l = poly_r;poly_r = k;}
			// hmm, ... 
			if (poly_l < 0) poly_l=0; if (poly_r > ptcWIDTH) poly_r = ptcWIDTH;
			for (j=poly_l;j<poly_r;j++) {
				k = (int)getY(j,s,degree);
				i_col = PUNKT_DUNKEL;
				if (i_poly_typ == 0) i_col = PUNKT_GELB;
				if (i_poly_typ == 1) i_col = PUNKT_BLAU;
				if (i_poly_typ == 2) i_col = PUNKT_ROT;
//				if (i_poly_typ == 3) i_col = PUNKT_ROT;
//				if (i_poly_typ == 4) i_col = PUNKT_ROT;
				if (i_poly_typ == 5) i_col = PUNKT_ROT;
				bb->setpixel(j,k,i_col);
			}
		}

		// ----------------------------
		//  Meß-Punkte zeichnen m. Farbcodierung
		// ----------------------------
		if (punkte_ausblenden == 0)
			for (j=v_cluster_li[i];j<v_cluster_re[i];j++) {
				i_col = VG_FARBE;

				x = (int)V[j].x;
				// wenn dim2==0 und getY-Aufruf, dann Runtime-Error
				if (s.dim2()>0) dist = (float)abs(V[j].y - getY(x,s,degree));
				// Pkt.e innerhalb der Nachbarschaft dunkel
				if (dist < nachbarschaft) i_col = PUNKT_DUNKEL;
				// Pkt.e ausserhalb der Nachbarschaft hell (s.oben "i_col = VG_FARBE")
				bb->setpixel(V[j].x,V[j].y,i_col);
			}	
	} // Ende d. Schleife

	bb->toScreen();
	bb->~Bild2d();

	delete[] V;
}

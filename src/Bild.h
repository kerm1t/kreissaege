// Bild.h: Schnittstelle für die Klasse Bild.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BILD_H__CD1EE40F_CA2C_439E_8937_42EC98025A1D__INCLUDED_)
#define AFX_BILD_H__CD1EE40F_CA2C_439E_8937_42EC98025A1D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vector>
#include "global.h"

#include "wmagic/WmlContMinBox2.h"
#include "wmagic/_WmlLine2.h"

extern int __ausgabe;

extern int hough_threshold;
extern int hough_tracking;				// 0=ohne..., 1=m.Tracking
extern int hough_glider;				// 0=ohne..., 1=m.Glider

extern int st_dynamisch;				// 0=dynamische...,1=feste Rekursion
extern int st_rekursion;
extern int wp_einblenden;				// Wendepunkte einblenden

extern int poly_dynamisch;				// 0=dynamischer...,1=fester Polynomgrad
extern int poly_degree;

extern int punkte_ausblenden;
extern int punkte_im_fenster;
extern int hintergrund_abziehen;

extern int hintergrund_y;

extern int anz_objekte;
extern std::vector<Objekt> objekte;

extern int statistik_schreiben;			// 0=nicht...,1=schreiben
extern std::ofstream ostat;

//extern int pixel[ptcSIZE];

//    Point with coordinates {float x, y;}
class Point {
public:
	float x;
	float y;
	float alpha;						// für den Convex Hull Alg. -> Sortierung CCW (CounterClockWise)
};

class StripNode {
public:
//	int i_pk;							// Nr. des Splitting Point
	int rek;							// Rekursionsstufe, die zu diesem Segment geführt hat
										// macht nur bei dyn. Strip-Trees Sinn!
	int i_left;							// Intervall, linke Grenze
	int i_right;						// Intervall, rechte Grenze
	Wml::Box2f b;						// umhüllendes Rechteck
	int orientierung;					// 1=im 1.Quadranten, d.h. x positiv,y positiv
										// 4=im 4.Quadranten,      x positiv,y negativ
	float x_pkt;						// falls Segment der Orientierung 1 und Folgesegment
	float y_pkt;						// der Orientierung 4, dann stehen hier die Koordinaten
 										// für die zykl. Mustererkennung, ansonsten (-1,-1)

	float fehler;						// "dynamischer" Fehler
	
//	Wml::Vector2f o;					// Ortsvektor d. Längsachse d. Segments
	Wml::Vector2f v;					// Längsachse d. Segments
	float winkel_nach;					// Winkel zum rechten Nachbar-Segment
//	float winkel_nach_wi;				// ...gewichtet
	float winkel_waage;					// Winkel des Segments zum "Tisch"
										// (eigentlich zur Waagerechten)
//	float winkel_waage_wi;				// ...gewichtet
	float pktdichte;					// Punkte/Länge (mm)
};

class Cluster {
	int i_li;
	int i_re;
};

class Bild  
{
public:
	Wml::Line2f h_gerade;				// für den Fall, dass es sich um den Hintergrund
										// handelt:
										// dominante, waagerechte Gerade (zw. 86 und 95 Grad),
										// gefunden per Hough-Transformation
										// => wichtig beim Hintergrund des Speichers ...
										// zum Finden des Maschinentisches
	int h_winkel;		// eigentl.redundant => zur schnelleren Berechnung
	int h_entf;
	
	
	int startwinkel;					// z.B. 0
	int stopwinkel;						// z.B. 180
	int aufloesung;						// Auflösung * 100	<--- die ist wichtig, nicht die
	int schrittweite;					// i.d.R. 1				 Schrittweite

	std::vector<int> data;
	int rec_num;						// fortlaufende Nr. innerhalb der Scan-Sequenz
	int max_val;						// maximaler Distanzwert
	
	Bild();
	Bild(int rec_num);
	virtual ~Bild();

	void copy_from(Bild* bild);
	int get_data(int i);
	Point get_xy(Config* conf, int i);
	void ohne_hintergrund_ausreisser(Config* conf, Bild* hintergrund, std::vector<int>& v);
	int to_xy(Config* conf, int nur_fenster, Point* H, int minus_hintergrund, Bild* hintergrund);

	// Zugriff auf den Bildschirm -> lohnt im Moment nicht
//	int Bild::getpixel_scr(int x, int y);
//	void Bild::setpixel_scr(int x, int y, int col);

	// Ausgabe
	void display();						// Text
	void toFile();						// Text->Datei
	void toPGM(Config* conf);			// Datei
	void toScreen(Config* conf, Bild* hintergrund);	// Screen

	void mittelwert(Bild* bild2);
	void differenz(Bild* bild2, int t);	// wenn kein Bildpunkt, dann wird
										// "-1" geschrieben
	// Filter auf polare Daten
	void mittelwert1D(int n); 
	void gauss1D();
	void median1D(int rang);

	// Segmentierung
	void log_hough();
	void tracking(Point* V, int anz, int* lm, int* lm2, int T, Config* conf, Bild* hintergrund);// Hough
	void hough(Config* conf, Bild* hintergrund);
	void strip_tree(Config* conf, Bild* hintergrund);
	void poly_regression(Config* conf, Bild* hintergrund);

	// (nach der) "Erkennung"
//	void neues_objekt_hough(Config* conf, Point* V, int cluster, int i_typ);
//	void neues_objekt_hough(Config* conf, Point pl, Point pr, int i_typ);
//	void strip_tree_auslagern();
	int neues_objekt_stree(Config* conf, Point* V, int punkt_li, int punkt_re, int vtree_li, int vtree_re,
		int i_typ, int grad, float proz_wawi);
	int neues_objekt_poly(Config* conf, Point* V, int clust_nr, int i_typ, int grad, float fehler);
private:
//	int pDistMax(Point pi, Point pj, Point* P, int size);
};

#endif // !defined(AFX_BILD_H__CD1EE40F_CA2C_439E_8937_42EC98025A1D__INCLUDED_)

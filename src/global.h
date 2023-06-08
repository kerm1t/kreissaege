#if !defined(GLOBAL__INCLUDED_)
#define GLOBAL__INCLUDED_

#define ptcWIDTH 1000
#define ptcHEIGHT 600
#define ptcSIZE ptcWIDTH*ptcHEIGHT

#define __DEBUG true						// ws, true/false
//#define __AUSGABE true

#define _PI 3.1415926535
#define BOG _PI/180							// Grad -> Bogenmaß
#define DEG 180/_PI							// Bogenmaß -> Grad

#define PUNKT_DUNKEL 75						// Grauwert f. dunklen Punkt
#define PUNKT_GRUEN 5						// Grauwert f. einen auf dem Screen grünen Pkt.
#define PUNKT_LILA 6
#define PUNKT_ROT 7							// -"- roten Pkt.
#define PUNKT_GELB 8						// -"- gelb Pkt.
#define PUNKT_BLAU 9						// -"- blau Pkt.
#define PUNKT_HELLROT 10

#define VG_FARBE 255//255						// schwarzer Hintergrund
#define HG_FARBE 0//0							// => bessere Sichtbarkeit
//#define INFO_FARBE 155

// Filter
#define MAX_FILTER_FENSTER 100

// Hintergrund + Ausreisser
#define HINTERGRUND_START 10				// (Speicher.h) starte mit der
											// Hintergrund-Kumulierung ab Bild/Frame (z.B."10")
#define HINTERGRUND_MITTELUNG 10			// (Speicher.h)
#define HINTERGRUND_NACHBARSCHAFT 6//5		// (Bild.h) in Längeneinheiten (mm)
#define CLUSTER_MAXLUECKE 2					// (Bild.h) max. Lücke in Punkten zw. 2 Clustern
#define CLUSTER_MINGROESSE 4				// (Bild.h) min. Größe in Punkten eines Clusters
#define AUSREISSER_NACHBARSCHAFT 1//4		// (Bild.h) in Punkten
#define AUSREISSER_NACHBARN 1//2			// (Bild.h) in Punkten
#define AUSREISSER_GEOMETRISCH 30			// (Bild.h) Anzahl in mm, die Pkte. auseinander
											// liegen dürfen
// Hough-Transformation
#define HOUGH_THRESHOLD_INIT 15
#define HOUGH_THRESHOLD_MAX 20
#define HOUGH_GLIDER_NACHBARSCHAFT 11.0f

// Strip-Tree
#define ST_REKURSION_MAX 20
#define ST_DYN_ERROR 2.0f
#define ST_REKURSION_INIT 4

// Polyn.Regression
#define POLY_DEGREE_MAX 11 //15
#define POLY_ERROR_MIN 2.0f					// unter diesem Fehler wird abgebrochen
#define POLY_REKURSION_INIT 12

// der Bild-buffer
//static int pixel[ptcSIZE];

enum e_quelle { QU_SOCKET, QU_DATEI };		// Werte: - über das Socket annehmen / - aus Datei lesen
enum Typ { BILD, SEQUENZ };
enum cnvTyp { KARTESISCH, HISTO };
enum koordSystem { SCANNER, WELT };
enum e_filter_typ { KEIN_FILTER, GAUSS, MITTEL, MEDIAN };

class Config
{
public:
	cnvTyp _cnvtyp;
	koordSystem _koord;		// an dieser Stelle okay?
							// dient ja der Visualisierung
	int _x1;
	int _y1;
	int y_initial;			// f.d.Hintergrund-Berechnung
							// der y-Entfernungswert, mit dem das Programm gestartet wird

	int _width;
	int _height;
	float _factor;
// Neu - Fenster mit der Maus ziehen, und dann ->
// kann man aus dem Teil d. Fensters nur eine Untermenge der Punkte darstellen
	int l_punkt;			// linker Punkt d. Bildes		
	int r_punkt;			// rechter Punkt d. Bildes
};

class Objekt {
public:
	int typ;
	int grad;				// Grad d. Rekursion (Strip-Tree), bzw.
							// Grad d. Polynoms (Regression)
	
	int n_segmente;			// Anz.d.Segmente (Strip-Tree)
	float wi_waage_mittel;	// f.Strip-Trees
	float wi_waage_sigma;	// f.Strip-Trees 
	float fehler;			// f.Poly-Regression / max.Fehler f. Segmente d. Strip-Trees
	float proz_wawi;		// f.Strip-Trees

	int x_mi;				// x-Mitte
	int y_mi;				// y-Mitte

	int x_gr;				// x-Groesse
	int y_gr;				// y-Mitte

	int y_erhebung;			// Distant Oberkante Objekt - Oberkante Tisch
	int n_wp;				// Neu - Anzahl Wendepunkte (WP's, Strip-Tree)
	float wp_x_mi;			// Neu - mittlere Entfernung der WP's (x-Richtung, Strip-Tree)
	float wp_x_std;			// Neu - deren Std.abweichung
	float wp_y_mi;			// Neu - mittlere Entfernung der WP's (y-Richtung, Strip-Tree)
	float wp_y_std;			// Neu - deren Std.abweichung
};

#endif // !defined(GLOBAL__INCLUDED_)

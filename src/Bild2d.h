// Bild2d1.h: Schnittstelle f¸r die Klasse Bild2d.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BILD2D1_H__31F60D68_9D78_4667_9B95_064687BDDB37__INCLUDED_)
#define AFX_BILD2D1_H__31F60D68_9D78_4667_9B95_064687BDDB37__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Bild.h"
#include "Sequenz.h"
#include "global.h"

extern int pixel[ptcSIZE];	// N‰‰‰‰, schon in Bild.h "definiert"

/**
	die Klasse Bild2d stellt eine klassische Matrix aus char (0..255) dar
	
	dem Konstruktor kann ein Bild
	oder eine Sequenz ¸bergeben werden
	
	nach der Umwandlung der Daten in ein w‰hlbare Ansicht,
	kann das Bild2d als pgm-Datei geschrieben werden
*/
class Bild2d  
{
	Typ typ;
	int x1;				// neu, stellt einen Ausschnitt aus dem
	int y1;				// Scan-Bild dar, dieses kann ansonsten sehr gross
						// und unhandlich werden (4096 x 3072 Pixel)

	unsigned char* _pixels;
	unsigned char** _rows;
	int rec_num;		// wird (z.B.) mit der rec_num beschrieben

public:
	int w;
	int h;
	float f;			// Skalierungsfaktor

	Bild2d(int w, int h);
	Bild2d(Config* conf, Bild* bild);
	Bild2d(int w, int h, float f, Sequenz* sequenz);
	virtual ~Bild2d();

	// Getter und Setter
	unsigned char getpixel(int x, int y);
	void setpixel(int x, int y, short i_col);
	
	void pixelup(int x, int y);

  void write();
  void write(std::string datei);
	void toScreen();

	void hough(int t);

	// Linie zeichnen
	void linie(int x1, int y1, int x2, int y2, int col);
	// Hessesche Normalenform
	void linie(int _alpha, int r, int col);

private:
	void init();
  void clear(int bg);
	// Bild
  void cnv_histo(Config* conf, float f, Bild* bild);
  void cnv_kartes(Config* conf, float f, Bild* bild);
	// Sequenz
  void cnv_seq_3dsurface(Sequenz* sequenz);
  void cnv_seq_cosinesh(Sequenz* sequenz);
  void cnv_seq_intensity(Sequenz* sequenz);

	void filter_gauss();
};

#endif // !defined(AFX_BILD2D1_H__31F60D68_9D78_4667_9B95_064687BDDB37__INCLUDED_)

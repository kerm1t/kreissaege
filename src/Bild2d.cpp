// Bild2d1.cpp: Implementierung der Klasse Bild2d.
//
//////////////////////////////////////////////////////////////////////

#include "Bild2d.h"
#include "global.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <math.h>

//const std::string endung = ".pgm";

extern cnvTyp _cnvtyp;

/*void polar_kart(int r, int alpha, float f, int* x, int* y) {
//	double BOG = 3.1415926535/180;

//	alpha = (bild->startwinkel+i*wi)*BOG;
	// x
	x = (int)((this->w/2) + cos(alpha) * r * f);
	// y
	y = (int)(sin(alpha) * r * f);
}
*/
//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////

/**
	Bildspeicher leeren
	Parameter: Farbe (0=schwarz, 255=weiss)
*/
void Bild2d::clear(int bg)
{
	unsigned int i;

	for(i=0; i<(this->w*this->h); i++) _pixels[i]=bg;
}

/** Bildspeicher belegen
*/
void Bild2d::init()
{
	unsigned int i;

	_pixels=new unsigned char[this->w*this->h];
    _rows=new unsigned char*[this->h];

	// initialize _rows
	unsigned char* row=_pixels;
	for (i=0; i<this->h; ++i) {
		_rows[i]=row;
		row+=this->w;
    }
}

/**	Konstruktor
*/
Bild2d::Bild2d(int w, int h)
{
	this->w = w;
	this->h = h;

	this->init();
	this->clear(HG_FARBE);
}

/**	überschriebener Konstruktor (1)
	übergeben
	- einer Konfiguration
	- eines Bildes
*/
Bild2d::Bild2d(Config* conf, Bild* bild)
{
	this->typ = BILD;
	this->x1 = conf->_x1;
	this->y1 = conf->_y1;
	this->w = conf->_width;
	this->h = conf->_height;
	this->f = conf->_factor;

	this->rec_num = bild->rec_num;

	this->init();
	this->clear(HG_FARBE);

	switch (conf->_cnvtyp) {
		case KARTESISCH:this->cnv_kartes(conf, this->f, bild); break;	
		case HISTO:this->cnv_histo(conf, this->f, bild); break;
	}
}

/**	überschriebener Konstruktor (2)
	übergeben einer Sequenz
*/
Bild2d::Bild2d(int w, int h, float f, Sequenz* sequenz)
{
	this->typ = SEQUENZ;
	this->w = w;
	this->h = h;

	this->init();
	this->clear(HG_FARBE);

	this->cnv_seq_intensity(sequenz);
}

/**	Destruktor
*/
Bild2d::~Bild2d()
{
	delete[] _pixels;
	delete[] _rows;
}

// -------------------------------------------------------------------
//  Getter und Setter
// -------------------------------------------------------------------

unsigned char Bild2d::getpixel(int x, int y)
{
	// Koord.system (0,0=unten links) umwandeln in Bildschirmkoord. (0,0=oben links)
	// minus 1 deshalb, weil (0,0) wird zu 1023, 767
	y = this->h-1-y;

	if (x < 0 || x > this->w-1 || y < 0 || y > this->h-1) {
//		std::cerr << "Fehler(setPixel), x=" << x << ",y=" << y << std::endl;
		return 0;
	}

	return _pixels[y*this->w + x];
}

/**
*/
void Bild2d::setpixel(int x, int y, short i_col)
{
	// Koord.system (0,0=unten links) umwandeln in Bildschirmkoord.(0,0=oben links)
	// minus 1 deshalb, weil (0,0) wird zu 1023, 767
//	y = this->h-1-y;
	y = this->h-1-y;

	if (x < 0 || x > this->w-1 || y < 0 || y > this->h-1) {
//		std::cerr << "Fehler(setPixel), x=" << x << ",y=" << y << std::endl;
		return;
	}
	_pixels[y*this->w + x] = i_col;
}

// -------------------------------------------------------------------

/**
	Grauwert des Pixels [x,y] um 1 erhöhen
	wichtig war es, hier eine Fehlerbehandlung einzusetzen, damit nicht
	Speicher überschrieben wird,
	entspricht einem "clipping"
*/
void Bild2d::pixelup(int x, int y)
{
	int col;
	
	// Koord.system (0,0=unten links) umwandeln in Bildschirmkoord. (0,0=oben links)
	// minus 1 deshalb, weil (0,0) wird zu 1023, 767
	y = this->h-1-y;

	if (x < 0 || x > this->w-1 || y < 0 || y > this->h-1) {
//		std::cerr << "Fehler(pixelup), x=" << x << ",y=" << y << std::endl;
		return;
	}
	col = _pixels[y*this->w + x];
	_pixels[y*this->w + x] = ++col;
}

// -------------------------------------------------------------------

/**
	Polarkoordinaten -> in kart. Koordinaten (x,y) konvertieren
*/
void Bild2d::cnv_kartes(Config* conf, float f, Bild* bild)
{
	unsigned int i;
	float alpha;								// Winkel im Bogenmass

	float x,y;
	float wi = (float)bild->aufloesung/100.0f;	// Winkelauflösung

	short i_col=VG_FARBE;

	for(i=0; i<bild->data.size(); i++) {
		if (bild->data[i]<0) continue;			// Differenz

		// x, y berechnen
		alpha = ((float)bild->startwinkel+(float)i*wi)*BOG;
		x = cos(alpha) * (float)bild->data[i] * this->f;
		y = sin(alpha) * (float)bild->data[i] * this->f;

		x -= (float)this->x1;
		y -= (float)this->y1;

//		std::cout << "conv.-kartesisch" << std::endl;
y = (float)this->h-1-y;
		// Zugriff: (y * breite) + x
		// nur schreiben, wenn sich der Pixel im Ausschnitt befindet
		// Bed.: x,y liegen innerhalb des Fensters
		if (((int)x < 0 || (int)x > this->w) ||
			((int)y < 0 || (int)y > this->h)) continue;// && nur_fenster == 1) continue;
		// Punkte, die ausserhalb des Fensters liegen,
		// sind also mit dem Initalisierungswert belegt, d.h. -4.31602e+008

//		std::cout << i << "," << conf->l_punkt << "," << conf->r_punkt << std::endl;
		// Neu - Punkte, die außerhalb der Begrenzung liegen auch nicht berücksichtigen!!
		if (i<conf->l_punkt || i>conf->r_punkt) continue;

//		if (x >= 0 && x <= this->w)
//			if (y >= 0 && y <= this->h) {
//				if (i>conf->l_punkt && i<conf->r_punkt)	i_col = PUNKT_GRUEN; else i_col = VG_FARBE;
				_pixels[(int)y*this->w + (int)x] = i_col;
//			}
	}
}

/**
	Polarkoordinaten -> Vektorfeld - Histogramm
	todo: Balken in x-Richtung 2 Pixel breit
*/
void Bild2d::cnv_histo(Config* conf, float f, Bild *bild)
{
	int i, j;
	float y;

	for(i=0; i<bild->data.size(); i++) {
		if (bild->data[i]<0) continue;	// differenz

		y = bild->data[i] * f;
		// flip
		y = this->h - y;
		if (y > this->h || y < 0) continue;	// Fehler abfangen
		for (j=y; j<this->h; j++) _pixels[(int)j*this->w+i]=VG_FARBE;

		// Zugriff: (y * breite) + x
		_pixels[(int)y*this->w + i]=VG_FARBE;
	}
}

// -------------------------------------------------------------------

void Bild2d::write()
{
	std::stringstream img_name;

	// als "img000x.pgm" ausgeben
	if (this->typ == BILD) img_name << "img";
	if (this->typ == SEQUENZ) img_name << "img_s";
	img_name << std::setw(4) << std::setfill('0') << this->rec_num << ".pgm";

	this->write(img_name.str());
}

/**
	Bildspeicher in *.pgm-Datei schreiben
	pgm-Format siehe http://netpbm.sourceforge.net/doc/pgm.html
*/
void Bild2d::write(std::string datei)
{
	std::ofstream os(datei.c_str());

	// write header
	os << "P5\n"
		<< this->w << "\n"
		<< this->h << "\n"
		<< "255\n";
  
	// write pixels
	unsigned int size=this->w*this->h;
	os.write(reinterpret_cast<char *>(_pixels),size);	// siehe http://www.cs.unr.edu/~bebis/CS308/

	std::cout << datei << " geschrieben." << std::endl;
}

/**
	Bildspeicher in den Bildschirm-Buffer schreiben,
	wird dann von TinyPTC dargestellt

	kopieren notwendig, da interne und screen-Auflösung vermutl.
	unterschiedlich sind, -> allerdings sollte Block-kopiert werden
*/
void Bild2d::toScreen()
{
	short col,r,g,b;
	int x,y;

	// Ausgabe in den Bildschirm-Buffer kopieren
	// -> jeden Pixel einzeln kopieren ist natürlich SUPERlangsam!!!
	for (y=0;y<ptcHEIGHT;y++) {
		for (x=0;x<ptcWIDTH;x++) {
			col = _pixels[y*this->w+x];
			switch (col) {
//				case PUNKT_BLAU: break;
				case PUNKT_GRUEN: r = 155;g = 255;b = 0; break;
				case PUNKT_LILA: r = 155;g = 0;b = 155; break;
				case PUNKT_ROT: r = 255;g = 55;b = 55; break;
				case PUNKT_HELLROT: r = 255;g = 155;b = 155; break;
				case PUNKT_GELB: r = 255;g = 255;b = 55; break;
				case PUNKT_BLAU: r = 55;g = 55;b = 255; break;
				default: r = col;g = r;b = r;
			}
			// Scanner-Koordinatensystem
			// pixel[y*ptcWIDTH+x] = (r<<16)|(g<<8)|b;		
			// Weltkoordinatensystem
			pixel[(ptcHEIGHT-1-y)*ptcWIDTH+(ptcWIDTH-x)] = (r<<16)|(g<<8)|b;
		}
	}
}

// -------------------------------------------------------------------

/**
	Intensitäts-Bild einer Sequenz
	je heller, desto weiter entfernt
*/
void Bild2d::cnv_seq_intensity(Sequenz *sequenz)
{
	int i, j, iBild;

	iBild = 1;
	for(i=0;i<sequenz->bilder.size();i++) {
		for (j=0;j<sequenz->bilder[i].data.size();j++)
			// Zugriff: (y * breite) + x
			_pixels[i*this->w + iBild] = sequenz->bilder[i].data[i]*255/sequenz->bilder[i].max_val;
		iBild++;
	}
/*	std::list<Bild>::iterator it;
	iBild = 1;
	for (it = sequenz->bilder.begin(); it != sequenz->bilder.end(); ++it) {
		// 
		for(i=0; i<it->data.size(); i++) {
			 
			// Zugriff: (y * breite) + x
			_pixels[i*this->w + iBild] = it->data[i]*255/it->max_val;

		}
		iBild++;
	}
*/}

/**	cosine-shaded Bild einer Sequenz
*/
void Bild2d::cnv_seq_cosinesh(Sequenz *sequenz)
{

}

/**	3-D surface Bild einer Sequenz
*/
void Bild2d::cnv_seq_3dsurface(Sequenz *sequenz)
{

}

/** Gauss Filter über das Bild laufen lassen
	brauchen wir das?
*/
void Bild2d::filter_gauss()
{

}

/**	HHHmmmmmmmmmmmmmmmmmm
	Hough Transformation
	Parameter: t = threshold
*/
void Bild2d::hough(int t)
{

}


/**
	Linie zeichnen mit Bresenham
*/
/*void Bild2d::linie(int x1, int y1, int x2, int y2, int col)
{
    // necessary to swap variables depending on direction
    // see W.Stuerzlinger, lecture notes
    int xtmp;
    int swap=0;
    if (abs(x2-x1) < abs(y2-y1)) { swap=1; xtmp=x1;x1=y1;y1=xtmp;xtmp=x2;x2=y2;y2=xtmp; }
    if (x1>x2) { xtmp=x1;x1=x2;x2=xtmp; }

    int ys = y1;
    float d = -0.5;
    float dy = (float)(y2-y1)/(float)(x2-x1);
    int xs;

	for (xs=x1;xs<x2;xs++) {
        if (swap==0) this->setpixel(xs,ys,col);
//			img.data[ys][xs] = (unsigned char) BOXCOLOR;
        if (swap==1) this->setpixel(xs,ys,col);
//			img.data[xs][ys] = (unsigned char) BOXCOLOR;
        d = d + dy;
        if (d>0) {
            ys = ys+1;
            d = d-1;
        }
    }
}
*/
void Bild2d::linie(int Ax, int Ay, int Bx, int By, int Color)
//void BresLine(int Ax, int Ay, int Bx, int By, unsigned char Color)
{
	//------------------------------------------------------------------------
	// INITIALIZE THE COMPONENTS OF THE ALGORITHM THAT ARE NOT AFFECTED BY THE
	// SLOPE OR DIRECTION OF THE LINE
	//------------------------------------------------------------------------
	int dX = abs(Bx-Ax);	// store the change in X and Y of the line endpoints
	int dY = abs(By-Ay);
	
	int CurrentX = Ax;		// store the starting point (just point A)
	int CurrentY = Ay;
	
	//------------------------------------------------------------------------
	// DETERMINE "DIRECTIONS" TO INCREMENT X AND Y (REGARDLESS OF DECISION)
	//------------------------------------------------------------------------
	int Xincr, Yincr;
	if (Ax > Bx) { Xincr=-1; } else { Xincr=1; }	// which direction in X?
	if (Ay > By) { Yincr=-1; } else { Yincr=1; }	// which direction in Y?
	
	//------------------------------------------------------------------------
	// DETERMINE INDEPENDENT VARIABLE (ONE THAT ALWAYS INCREMENTS BY 1 (OR -1) )
	// AND INITIATE APPROPRIATE LINE DRAWING ROUTINE (BASED ON FIRST OCTANT
	// ALWAYS). THE X AND Y'S MAY BE FLIPPED IF Y IS THE INDEPENDENT VARIABLE.
	//------------------------------------------------------------------------
	if (dX >= dY)	// if X is the independent variable
	{           
		int dPr 	= dY<<1;   						// amount to increment decision if right is chosen (always)
		int dPru 	= dPr - (dX<<1);				// amount to increment decision if up is chosen
		int P 		= dPr - dX;						// decision variable start value

		for (; dX>=0; dX--)							// process each point in the line one at a time (just use dX)
		{
			this->setpixel(CurrentX, CurrentY, Color);	// plot the pixel
			if (P > 0)                              // is the pixel going right AND up?
			{ 
				CurrentX+=Xincr;					// increment independent variable
				CurrentY+=Yincr; 					// increment dependent variable
				P+=dPru;							// increment decision (for up)
			}
			else									// is the pixel just going right?
			{
				CurrentX+=Xincr;					// increment independent variable
				P+=dPr;								// increment decision (for right)
			}
		}		
	}
	else			// if Y is the independent variable
	{
		int dPr 	= dX<<1;   						// amount to increment decision if right is chosen (always)
		int dPru 	= dPr - (dY<<1);    			// amount to increment decision if up is chosen
		int P 		= dPr - dY;						// decision variable start value

		for (; dY>=0; dY--)							// process each point in the line one at a time (just use dY)
		{
			this->setpixel(CurrentX, CurrentY, Color);	// plot the pixel
			if (P > 0)                              // is the pixel going up AND right?
			{ 
				CurrentX+=Xincr; 					// increment dependent variable
				CurrentY+=Yincr;					// increment independent variable
				P+=dPru;							// increment decision (for up)
			}
			else									// is the pixel just going up?
			{
				CurrentY+=Yincr;					// increment independent variable
				P+=dPr;								// increment decision (for right)
			}
		}		
	}		
}

/**
	Linie zeichnen - Geradengl. in (Hesse'scher) Normalenform
*/
void Bild2d::linie(int _alpha, int r, int col)
{
	float x,y;
	float alpha = (float)_alpha*(float)BOG;	// in's Bogenmass umwandeln

//	std::cout << "theta=" << alpha << ", rho=" << r << std::endl;		

	if ((alpha > _PI/4) && (alpha < 3*_PI/4)) {
		// eher waagerechte Linie -> x durchlaufen
		for (x=0;x<this->w;x++) {
			y = ((float)r - (float)x * cos(alpha)) / sin(alpha);
			this->setpixel((int)x,(int)y,col);
		}
	}
	else {
		// eher senkrechte Linie -> y durchlaufen
		for (y=0;y<this->h;y++) {
			x = ((float)r - (float)y * sin(alpha)) / cos(alpha);
			this->setpixel((int)x,(int)y,col);
		}
	}

	// hier wurde mal, in x1,y1,x2,y2 umgerechnet, aber das ist m.E. weniger sinnvoll
	// als obige Lösung
}


/* ---------------------------------
	main-Datei des Abschlussprojekts
	(c) 2004(!) Wolfgang Schulz
   --------------------------------- */

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
//#include <curses.h>					// UNIX - für Tastendrücke
//#include <conio.h>					// Visual C++
//#include <mmsystem.h>					// ...

// für die DEBUG Ausgaben
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

// für die Sockets!!
#include <windows.h>
#pragma comment(lib, "Ws2_32.lib")
#define MC_MAXPUFFER 4096
#define MC_MAXCLIENT 1
// <-- Sockets

#include <vector>
#include "Tokenizer.h"

#include "lader.h"
#include "Bild.h"
#include "Bild2D.h"
#include "Speicher.h"

#include "tinyptc/tinyptc.h"						// TinyPTC - http://www.gaffer.org
#include "tinyptc/convert.c"
#include "tinyptc/ddraw.c"
#include "tinyptc/mmx.h"
#include "text.h"									// Text in tinyPTC-buffer schreiben

//#include "winbase.h"								// zum Lesen/schreiben einer .ini-Datei

#include "global.h"

//#pragma warning(disable : 4786)

int __ausgabe				= 0;					// Text-Ausgabe an std::out 
													// zulassen / unterdrücken
//int 
// Dateneingabe über 0=QU_SOCKET oder 1=QU_DATEI
int bildquelle;
// ggf. Meßpunkte nur innerhalb eines Intervalls einlesen
int lese_p_links			= -1;					// -1 = keine linke Grenze
int lese_p_rechts			= -1;					// -1 = keine rechte Grenze

int pixel[ptcSIZE];									// Screen-Buffer
// Hough
int hough_threshold			= HOUGH_THRESHOLD_INIT;
int hough_tracking			= 1;					// 0=ohne..., 1=m.Tracking
int hough_glider			= 1;					// 0=ohne..., 1=m.Glider

// Strip-Tree
int st_dynamisch			= 1;					// 1=dynamische...,0=feste Rekursion
int st_rekursion			= ST_REKURSION_INIT;	// Strip-Trees - Rekursionstiefe
int wp_einblenden			= 1;					// Wendepunkte einblenden
													// 0=nicht,1=Pfeile,2=Lote auf d. HG

// Polynomial Regression
int poly_dynamisch			= 1;					// 1=dynamischer...,0=fester Polynomgrad
int poly_degree				= POLY_REKURSION_INIT;	// Poly.Regression - Grad d. Polynoms

// Anzeige
int punkte_ausblenden		= 0;
int punkte_im_fenster		= 0;					// Anz. der angezeigten Punkte
int infos_ausblenden		= 0;					// 0=keine,1=nur Objekte,2=alle Infos
int linien_ausblenden		= 0;
int skala_ausblenden		= 1;
int hintergrund_abziehen	= 0;					// 0=nein,1=nur Punkte,2=Gerade
int sigma_einblenden		= 0;

// Filter
int seq_filter_typ			= 0;					// 0=keiner,1=Gauss,2=Mittel,3=Median
int seq_filter_fenster		= 1;					// über n 'Frames' wird gefiltert
int bild_filter_typ			= 1;					// 0=keiner,1=Gauss,2=Mittel,3=Median

int i_extra					= 2;					// 0 = keins
													// 1 = Hough
													// 2 = Strip-Tree
													// 3 = Polynomielle Regression
 
std::string s_cnvTyp[2]		= {"kartesisch","histogramm"};
std::string s_modus[2]		= {"manuell","auto"};
std::string s_koordsystem[2]= {"scanner-koordinaten","weltkoordinaten"};
std::string s_appname		= "CVLabor - Formerkennung";
std::string s_filter_typ[4]	= {"-","gauss","mittel","median"};
std::string s_objekt_typ[7]	= {
//	"weiss nicht","holz","hand","hand,offen","hand,geschl.","finger","arm"};
	"unklassifiziert","holz","hand","hand,offen","hand,geschl.","finger","arm"};

// Benutzer-Interaktion (Variablen aus ddraw.h)
extern int taste;				// Tastatur
extern int taste_ascii;			// -"-
extern int m_xStart;			// Maus
extern int m_yStart;			// bei einer Zieh-Bewegung wird hier der Start gemerkt
extern int m_xPos;				// Maus
extern int m_yPos;				// -"-
extern int m_zDelta;			// -"-
extern int m_lbutton;			// -"-
extern int m_rbutton;			// -"-

// nur einen Teil der Kurve darstellen

// Socket-Variablen!!
std::string socket_port;
struct sockaddr_in addrServer;	// Strukturen zur Speicherung der Verbindungsdaten
struct sockaddr_in addrClient;  
int s_server, s_client;			// listen on s_server

char Puffer[MC_MAXPUFFER];		// allgemein verwendeter Puffer
fd_set fdSetRead;				// Struktur mit Informationen für die select()-Funktion

int ini_aufloesung;				// aus der .ini ausgelesene Werte
int ini_stopwinkel;
int ini_startwinkel;
int ini_i_bild;					// aktuell in der Sequenz angez. Bild

int hintergrund_y;

// ------------------------------------
// hierein wird die Statistik geschrieben
// Datei wird hier bereits geöffnet bzw. der Stream wird initialisiert
// ------------------------------------
int statistik_schreiben		= 0;// 0=nicht...,1=schreiben
std::ofstream ostat("stat.dat");
	
// ------------------------------------
//  f.d. Auswertung
// ------------------------------------
int anz_objekte;
std::vector<Objekt> objekte;

// ------------------------------------
// Sicherheitsbereich - eigentlich: Gefahrenbereich
// ------------------------------------
float sicher_xmi			= 0;
float sicher_ymi			= 650;
float sicher_xgr			= 240;
float sicher_ygr			= 400;
float sicher_x1,sicher_y1,sicher_x2,sicher_y2;

int trigger;		// z.B. für blinkende Objekte
int alarm = 0;
int gefahrenzone_anzeigen	= 0;

/** integer in std::string umwandeln */
std::string itos( int i_ )
{
	std::stringstream stream_;
	stream_ << i_;
	return stream_.str();
}

/**	===================================
	liest einige Infos aus der formerkennung.ini aus,
	z.B. die Kommunikationsparameter beim Socket-Modus
	=================================== */
void read_ini(Config* conf)
{
//	UINT ret;
	char buf[MAX_PATH];				// Mjamm, args[0] müßte i.d.R. auch funktionieren
	GetCurrentDirectory(MAX_PATH, buf);
	
	std::string s_ini = buf;		// ohne vollständigen Pfad, wird die ini-Datei im Windows Systemverzeichnis
	s_ini += "\\formerkennung.cfg";	// gesucht

	// [Allgemein]
	__ausgabe = GetPrivateProfileInt("Allgemein","Ausgabe",0,s_ini.c_str());
	bildquelle = GetPrivateProfileInt("Allgemein","Bildquelle",0,s_ini.c_str());
	lese_p_links = GetPrivateProfileInt("Allgemein","lese_p_links",-1,s_ini.c_str());
	lese_p_rechts = GetPrivateProfileInt("Allgemein","lese_p_rechts",-1,s_ini.c_str());
	i_extra = GetPrivateProfileInt("Allgemein","i_extra",2,s_ini.c_str());
	ini_i_bild = GetPrivateProfileInt("Allgemein","i_bild",1,s_ini.c_str());
	hintergrund_abziehen = GetPrivateProfileInt("Allgemein","hintergrund_abziehen",1,s_ini.c_str());
	seq_filter_typ = GetPrivateProfileInt("Allgemein","seq_filter",0,s_ini.c_str());
	seq_filter_fenster = GetPrivateProfileInt("Allgemein","seq_filter_fenster",1,s_ini.c_str());
	bild_filter_typ = GetPrivateProfileInt("Allgemein","bild_filter",1,s_ini.c_str());
	skala_ausblenden = GetPrivateProfileInt("Allgemein","skala_ausblenden",1,s_ini.c_str());

	// [LMS]
//	ret = GetPrivateProfileInt("LMS","Richtung",0,s_ini.c_str());
	ini_aufloesung = GetPrivateProfileInt("LMS","Aufloesung",50,s_ini.c_str());
	ini_stopwinkel = GetPrivateProfileInt("LMS","Stopwinkel",18000,s_ini.c_str());
	ini_startwinkel = GetPrivateProfileInt("LMS","Startwinkel",0,s_ini.c_str());

	// [Konfiguration]
	conf->_x1 = GetPrivateProfileInt("Konfiguration","x1",-500,s_ini.c_str());
	conf->_y1 = GetPrivateProfileInt("Konfiguration","y1",70,s_ini.c_str());
	conf->y_initial = conf->_y1;

	// volle Größe 8191
	conf->_width = GetPrivateProfileInt("Konfiguration","width",1000,s_ini.c_str());
	// ... mal (8191*2) 
	conf->_height = GetPrivateProfileInt("Konfiguration","height",600,s_ini.c_str());
	conf->_factor = 1.0f;
	conf->l_punkt = 0;
	conf->r_punkt = (ini_stopwinkel-ini_startwinkel)/ini_aufloesung+1;

	// [Kommunikation]
	socket_port = "3030";//GetPrivateProfileString("Kommunikation","Port","0",s_ini.c_str());
}

void write_ini(Config* conf)
{
	char buf[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, buf);
	
	std::string s_ini = buf;
	s_ini += "\\formerkennung.cfg";

	// [Allgemein]
	WritePrivateProfileString("Allgemein","Ausgabe",itos(__ausgabe).c_str(),s_ini.c_str()); 
	WritePrivateProfileString("Allgemein","i_extra",itos(i_extra).c_str(),s_ini.c_str()); 
	WritePrivateProfileString("Allgemein","i_bild",itos(ini_i_bild).c_str(),s_ini.c_str()); 
	WritePrivateProfileString("Allgemein","hintergrund_abziehen",itos(hintergrund_abziehen).c_str(),s_ini.c_str()); 
	WritePrivateProfileString("Allgemein","seq_filter",itos(seq_filter_typ).c_str(),s_ini.c_str()); 
	WritePrivateProfileString("Allgemein","seq_filter_fenster",itos(seq_filter_fenster).c_str(),s_ini.c_str()); 
	WritePrivateProfileString("Allgemein","bild_filter",itos(bild_filter_typ).c_str(),s_ini.c_str()); 
	WritePrivateProfileString("Allgemein","skala_ausblenden",itos(skala_ausblenden).c_str(),s_ini.c_str()); 

	std::cout << std::endl << "Einstellungen nach " << s_ini << " geschrieben." << std::endl;
}

/**	===================================
	Linie zeichnen mit Bresenham
	kann man r,g,b, noch durch einen geeigneten colorcode ersetzen -> #00ff00 (html-style)
	
	Neu!! mit Kontrolle, ob Linie ausserhalb des Bildschirms/Buffers
	=================================== */
void linie(int Ax, int Ay, int Bx, int By, short r, short g, short b)
//void BresLine(int Ax, int Ay, int Bx, int By, unsigned char Color)
{
	const BUFSIZE = ptcHEIGHT*ptcWIDTH;
	int buf_pos;
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
			buf_pos = CurrentY*ptcWIDTH+CurrentX;
			if (buf_pos >= 0 && buf_pos < BUFSIZE)
				pixel[buf_pos] = (r<<16)|(g<<8)|b;	// plot the pixel
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
			buf_pos = CurrentY*ptcWIDTH+CurrentX;
			if (buf_pos >= 0 && buf_pos < BUFSIZE)
				pixel[buf_pos] = (r<<16)|(g<<8)|b;	// plot the pixel
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

/**	===================================
	Zeichnet ein Rechteck
	@params:	linke, obere Ecke, Breite, Farbe: r,g,b
	=================================== */
void box(int xx, int yy, int w, int h, short r, short g, short b) {
	linie(xx,yy,xx+w,yy,r,g,b);		// oben
	linie(xx+w,yy,xx+w,yy+h,r,g,b);	// rechts
	linie(xx+w,yy+h,xx,yy+h,r,g,b);	// unten
	linie(xx,yy+h,xx,yy,r,g,b);		// links
}

/**	gibt Bildschirm-Koordinaten (!) zurück
	wichtig ist hier die Angabe des Winkels!!
*/
void polar_to_kart(float i, int len, Config* conf, float &x, float &y)
{
//	float x,y;
	float alpha;
//	float wi = (float)bild->aufloesung/100.0f;		// Winkelauflösung

	// r, alpha (polar) in  x,y (kart.) umrechnen
//	alpha = ((float)bild->startwinkel+(float)i*wi)*BOG;
	alpha = i*BOG;
	x = cos(alpha) * (float)len * conf->_factor;
	y = sin(alpha) * (float)len * conf->_factor;

	// Fenster
	x -= (float)conf->_x1;
	y -= (float)conf->_y1;

	// Bed.: x,y liegen innerhalb des Fensters
//	if ((x < 0 || x > conf->_width) ||
//		(y < 0 || y > conf->_height)) { x = 0; y = 0; }

//	Point p;
//	p.x = x;
//	p.y = y;
//	std::cout << p.x << "," << p.y << " - ";
//	return p;
}

// WSAInit() - Win32 spezifische "Socketdienst"-Initialisierung
void WSAInit(void)
{
  WORD wVersionReg = MAKEWORD(2, 0);      // WSA-Version 2.0
  WSADATA wsaData;
  if (int err = WSAStartup(wVersionReg, &wsaData) != 0)
  {
	  std::cout << "Fehler: Initialisieren - WinSock.dll!";
  }
}

/**	===================================
	Server-Socket bereitstellen / initialisieren, um Daten zu empfangen
	=================================== */
int init_server_socket()
{
	int checkReturn;								// Variable für Funktions-Rückgabewerte

	addrServer.sin_family = AF_INET;         	    // Internet...
	addrServer.sin_addr.s_addr = INADDR_ANY; 	    // akzept. jeden
	addrServer.sin_port = htons(atol(/*vArgumente[1]*/"3030")); // den als Argument angegebenen Port konvertieren

	// WSA initialisieren (Win32-spezifisch, siehe Funktion)
	WSAInit();

	std::cout << "Server starten..." << std::endl << std::endl;

	// Socket erstellen
	std::cout << "Socket erstellen    - ";
	s_server = socket(AF_INET, SOCK_STREAM, 0);
	if (s_server == INVALID_SOCKET)
	{
		wsprintf(Puffer, "Fehler: Socket-Erstellung - Error-Code %i", WSAGetLastError());
		std::cout << Puffer << std::endl << std::flush;
		WSACleanup();
		return -1;
	}
	std::cout << "OK" << std::endl << std::flush;

	// Socket an Port-Nummer binden
	std::cout << "Socket binden       - ";
	checkReturn = bind(s_server, (struct sockaddr *) &addrServer, sizeof(addrServer));
	if (checkReturn == SOCKET_ERROR)
	{
		wsprintf(Puffer, "Fehler: bind() - Error-Code %i", WSAGetLastError());
		std::cout << Puffer << std::endl << std::flush;
		closesocket(s_server);
		WSACleanup();
		return -1;
	}
	std::cout << "OK" << std::endl << std::flush;

	// Socket in Lausch-Modus versetzen
	std::cout << "Auf Socket lauschen - ";
	checkReturn = listen(s_server, MC_MAXCLIENT);
	if (checkReturn == SOCKET_ERROR)
	{
		wsprintf(Puffer, "Fehler: listen() - Error-Code %i", WSAGetLastError());
		std::cout << Puffer << std::endl << std::flush;
		closesocket(s_server);
		WSACleanup();
		return -1;
	}
	std::cout << "OK" << std::endl << std::endl << std::flush;

	std::cout << "Server erfolgreich gestartet. Warte auf Clients..." << std::endl << std::endl  << std::flush;
	
	return 0;
}

/**	===================================
	Server-Socket -> "geordneter Rückzug"
	=================================== */
int close_server_socket()
{
	closesocket(s_server);
	WSACleanup();
	return 0;
}

/**	===================================
	auf Daten vom SocketClient warten ... zzz zzz zz..z
	dann den Puffer schon tokenisieren
	und in den "Speicher" schreiben
	=================================== */
void tu_watt_einlesen(Bild* bild, Speicher* speicher, int i_mode)
{
	int addrLen;	// Länge einer sockaddr_in-Struktur

	// Wenn auf unserem Server-Socket ein Eingang signalisiert wird, ist dies ein
	// neuer Client... Sollte zumindest einer sein... ;D
	if (FD_ISSET(s_server, &fdSetRead))
	{
		// Verbindung annehmen und zur Client-Liste hinzufügen
		addrLen = sizeof(SOCKADDR_IN);
		s_client = accept(s_server, (SOCKADDR*)&addrClient, &addrLen);
		if (s_client == INVALID_SOCKET)
			std::cout << "Fehler: accept(), Error-Code " << WSAGetLastError() << std::endl;

/*		for (j = 0; j < MC_MAXCLIENT; ++j)
		{
			if (Client[j].Active == false)
			{
				std::cout << "Neuer Client in Slot " << j << " mit Socket " << (UINT)s_client << "." << std::endl;
				Client[j].Login(s_client, addrClient);
				break;
			}
		}*/
	}

	int dataLength;                          // Länge der übermittelten Daten

	dataLength = recv(s_client, Puffer, sizeof(Puffer) - 1, 0); // Empfangen
	if (dataLength == SOCKET_ERROR)
	{
		std::cout << "Socket " << s_client << " meldet sich nicht mehr...";
		closesocket(s_client);
//		Client[j].Logout();
//		continue;
	}
	else
	{
		Puffer[dataLength] = '\0'; // Wichtig! Die empfangenden Daten "abschließen"...
	}

//std::cout << dataLength << std::endl;
	int i;
	int size = 401;

	if (dataLength <= 0) return;
	if (dataLength > 1800) {
		// ----------------------------
		// Kommunikations-Sicherheits-Schwelle
		// => wenn Algorithmus "aus dem Trab kommt", dann werden hier über 4000
		// int's übertragen (Standard bei 0.25Grad, 40-140 Grad sind 1676)
		// => dann erstmal hier 'raus und beruhigen...
		// ----------------------------
		std::cout << "Kommunikations-Ueberlauf (" << dataLength << " [int])" << std::endl;
		for (i=0;i<size;i++) bild->data[i] = i;	// einfaches Bild 'reingeben
		return;							
	}

	// man kann den Datenstrom "anhalten" und sich somit ein Standbild anschauen
	if (i_mode == 0 && speicher->zaehler > HINTERGRUND_MITTELUNG) return;

// -------------------------------------------------------------------------------------------
// @2do
// kann auch passieren, dass nach einem Überlauf erstmal eine unvollständige Zeile angeliefert
// wird, das müßte man dann noch abfangen
// -------------------------------------------------------------------------------------------

	// --------------------------------
	//  in den "Speicher" kopieren
	// --------------------------------
	std::stringstream str_puffer;
	std::vector<std::string> werte;
	str_puffer << Puffer;

	CTokenizer<CIsSemicolon>::Tokenize(werte, str_puffer.str(), CIsSemicolon());
	// bzw. in das Bild "schreiben"!!
//	size = (ini_stopwinkel-ini_startwinkel)/ini_aufloesung+1;	// i.d.R. "361" oder "401"
	// irgendwie steht hier der ini_stopwinkel auf "0",
	for(i=1; i<size; i++) {		// hier (erster Wert) nicht "4" sondern "1"
		bild->data[i-1] = atoi(werte[i].c_str());
	}
	
	if (speicher->zaehler <= HINTERGRUND_MITTELUNG) speicher->setze_hintergrund_echtzeit(bild);	
}

/**	===================================
	Aarrrghhh, diese "polar_to_kart" _Funktion ist vom Winkel abhängig *0.25 ...
	dann der Tausch der x- und y- Koordinaten -> datt mät misch fäähdisch -> also aufpassen!!!
	=================================== */
void punkte_grenze_setzen(Config* conf, Bild* bild) {
	int i;
	float alpha,x,y;
	float wi = (float)bild->aufloesung/100.0f;			// Winkelauflösung

	int x_1 = /*conf->_x1 + */m_xStart;
	int y_1 = /*conf->_y1 + */m_yStart;
	int x_2 = /*conf->_x1 + */m_xPos;
	int y_2 = /*conf->_y1 + */m_yPos;
	
	int left = -1;
	int right = -1;

//	std::cout << "(" << x_1 << "," << y_1 << ")-(" << x_2 << "," << y_2 << ")" << std::endl; 
	for (i=0;i<bild->data.size();i++) {
		// die Punkte gehen vpon rechts nach links
		alpha = ((float)bild->startwinkel+(float)i*wi);
		polar_to_kart(alpha, bild->data[i], conf, x, y);
x=conf->_width-x;
//		std::cout << " (" << i << ") " << (int)x << "," << (int)y << "";
		// zuerst die rechte Grenze suchen, ...
		// x und y sind umgedreht, also sind hier auch links und rechts umgedreht (AArrrghh!!)
		if ((int)x >= x_1 && (int)x <= x_2 && (int)y >= y_1 && (int)y <= y_2 && left < 0)
			left = i;
		if (left>=0) {
		// ... dann die linke ...
			if (((x < x_1 || x > x_2) || (y < y_1 || y > y_2)) && (right < 0))
				right = i;
		}
	}
//	std::cout << std::endl << std::endl;

	conf->l_punkt = left;
	conf->r_punkt = right;
//	std::cout << "l,r=" << left << "," << right << std::endl;
}

void fps_reset(float &fpsmin, float &fpsmax)
{
	fpsmin = 99.9f;
	fpsmax = 0.0f;
}

// -----------------------------------------------------------------
// nur bei stillstehender Szene
// und Sequenz sinnvoll, da über die gesamte Sequenz berechnet wird!
// -----------------------------------------------------------------
void sigma_ausgeben(Speicher* speicher, Sequenz* sequenz, Config* conf, int sigma_akt) {
	int i,j;
	const SIGMA_FACTOR = 3;				// Höhe der Balken * SIGMA_FACTOR

	char s[55];

	short r,g,b;
	Point p;

	// ---------------------------------
	//  Zeichnen der Standardabweichung
	// ---------------------------------
	j = (ptcWIDTH-(sequenz->sigma.size()*2))/2; if (j<0) j=0;// zentrieren - sieht schöner aus
	for (i=0;i<sequenz->sigma.size();i++) {
		if (i==sigma_akt) { r = 155;g = 255; b = 0;} else { r = 155; g = 155; b = 155; }
		// auf Bildschirm-Breite begrenzen
		if (i*2 < ptcWIDTH)
			linie(j+i*2,(ptcHEIGHT-50),j+i*2,(ptcHEIGHT-50)-sequenz->sigma[i]*SIGMA_FACTOR,r,g,b);
		// zum besseren "Durchblick" Hilfslinie unter dem Balken zeichnen
		if (i==sigma_akt)
			linie(j+i*2,(ptcHEIGHT-43),j+i*2,(ptcHEIGHT-48),r,g,b);
	}
	// den gewählten Sigma-Wert als Ziffer ausschreiben
	sprintf(s,"sigma [%d] : %.2f (%d limitiert)",sigma_akt,sequenz->sigma[sigma_akt],sequenz->sigma_limitiert);
	doText(j,(ptcHEIGHT-30),s);

	// ----------------------------------
	//  Entfernungspunkt farbig zeichnen
	// -> das muss noch kräftig optimiert werden -> in's Bild übernehmen
	// ----------------------------------
	i=0;
	r = 155;g = 255; b = 0;
	p = speicher->bild->get_xy(conf, sigma_akt);
//	p.x -= (float)conf->_x1;	=> passiert schon in der Fkt. get_xy(...)
//	p.y -= (float)conf->_y1;

//	p.y = conf->_height - p.y + 20;

	p.y = conf->_height - 1 - p.y;
	if (p.x < 0 || p.x > ptcWIDTH-1 || p.y < 0 || p.y > ptcHEIGHT-1) return;
	pixel[(int)p.y*ptcWIDTH+(int)p.x] = (r<<16)|(g<<8)|b;
}

void anz_objekte_ausgeben() {
	short r,g,b;
	float xx,yy;

	// Anzahl der Objekte ausgeben
	r=155;g=255;b=0;
	xx = 500; yy = 200;
	if (objekte.size()==0) doTextCol(xx,yy,"leerer tisch!",r,g,b);
	if (objekte.size()>0) doTextCol(xx,yy,itos(objekte.size()) + " objekt" +
		(objekte.size()>1?"e":""),r,g,b);
}

/**	===================================
	prüfen ob das Objekt [i] in den Gefahren-
	bereich "hineinragt"
	=================================== */
int test_objekt(int i) {
	int o_x1,o_x2,o_y1,o_y2;
	
	o_x1 = objekte[i].x_mi - objekte[i].x_gr/2;
	o_x2 = objekte[i].x_mi + objekte[i].x_gr/2;
	o_y1 = objekte[i].y_mi - objekte[i].y_gr/2;
	o_y2 = objekte[i].y_mi + objekte[i].y_gr/2;

//	box(o_x1,o_y1,objekte[i].x_gr,objekte[i].y_gr,155,155,155);

	if (o_x2 > sicher_x1 && o_x2 < sicher_x2) {
		if (o_y2 > sicher_y1 && o_y2 < sicher_y2) return 1;
		if (o_y1 > sicher_y1 && o_y1 < sicher_y2) return 1;
	}
	if (o_x1 > sicher_x1 && o_x1 < sicher_x2) {
		if (o_y2 > sicher_y1 && o_y2 < sicher_y2) return 1;
		if (o_y1 > sicher_y1 && o_y1 < sicher_y2) return 1;
	}
	if (o_x1 < sicher_x1 && o_x2 > sicher_x2) {	// Objekt "umfaßt" den Gefahrenbereich
		if (o_y2 > sicher_y1 && o_y2 < sicher_y2) return 1;
		if (o_y1 > sicher_y1 && o_y1 < sicher_y2) return 1;
	}
	return 0;
}

/**	===================================
	Info-Box für ein Objekt ausgeben
	=================================== */
void objekte_ausgeben(Config* conf) {
	int i;
	short r,g,b,r1,g1,b1;
	float xx,yy,x,y;
	float xs,ys;		// x-Start,y-Start = ober Kante der Info-Box

	char s[55];
	float f_tmp;		// kann man mal bequem ein paar Sachen durchrechnen
						// gut in Zeiten von C++ könnte man auch einen stringstream benutzen
	int alarm_objekt;
	int box_breite = 133;

	alarm = 0;
	// Info für jedes Objekt ausgeben
	for (i=0;i<objekte.size();i++) {
		alarm_objekt = 0;
		// ----------------------------
		// Testen, ob das Objekt [i] in den Gefahrenbereich "hineinragt"
		// ----------------------------
		if (gefahrenzone_anzeigen==1)
			if (objekte[i].typ >= 2) alarm_objekt = test_objekt(i);

		switch (objekte[i].typ) {
		// weiss nicht = gelb
		case 0:	r=255;g=255;b=55; break;
		// Holz = blau
		case 1:	r=55;g=55;b=255; break;
		// Hand = rot
		case 2:	r=255;g=55;b=55; break;
		// Hand, geoeffnet = rot
		case 3:	r=255;g=55;b=55; break;
		// Hand, geschl. = rot
		case 4:	r=255;g=55;b=55; break;
		// Finger = rot
		case 5:	r=255;g=55;b=55; break;
		// Arm = rot
		case 6:	r=255;g=55;b=55; break;
		// schnelle Bewegung = gelb
		case 99:r=255;g=55;b=55; break;
		}
		
		if (objekte[i].typ == 99) alarm_objekt = 1;		// schnelle Bewegung : immer einen Alarm auslösen

		if (alarm_objekt==1) {
			r *= trigger; g *= trigger; b *= trigger;
			alarm = 1;
		}

		if (objekte.size() < 3) {
			xx = objekte[i].x_mi-62;	// Info-Box unter dem Objekt zentrieren
			yy = objekte[i].y_mi + objekte[i].y_gr/2+15;

			xs = xx; ys = yy;
		} else {
			x = objekte[i].x_mi;
			y = objekte[i].y_mi;
			xx = 100 + i*150;			// Info Box schön anordnen
			yy = y-200;
			
			xs = xx; ys = yy;
			
			if (xx>800)	{
				// untere Reihe
				xx -= 800; yy +=300;
			} //else // obere Reihe
		}

		// (vermuteter) Typ
		if (objekte[i].typ < 99) doTextCol(xx,yy,s_objekt_typ[objekte[i].typ],r,g,b);
		else doTextCol(xx,yy,"schnelle bewegung",r,g,b);
		yy+=1;			
		// Anz. d. Segmente
		if (i_extra==2) doText(xx,yy+=10,"segmente=" + itos(objekte[i].n_segmente));
		// Grad
		if (i_extra==2 && poly_dynamisch==1) doText(xx,yy+=10,"max.grad=" + itos(objekte[i].grad));
		if (i_extra==2 && poly_dynamisch==0) doText(xx,yy+=10,"grad=" + itos(objekte[i].grad));
		if (i_extra==3) doTextCol(xx,yy+=10,"grad=" + itos(objekte[i].grad),r,g,b);
		// Fehler
		if (i_extra==2) {
			sprintf(s,"max.fehler=%.2f",(float)objekte[i].fehler);
			doText(xx,yy+=10,s);
		}
		if (i_extra==3) {
			sprintf(s,"fehler=%.2f",(float)objekte[i].fehler);
			doTextCol(xx,yy+=10,s,r,g,b);
		}
		// Breite
		f_tmp = (float)objekte[i].x_gr/10.0f;
		sprintf(s,"breite=%.2fcm",f_tmp);
		
		// ------------------------
		// checken, ob die Breite gefährlichen Bereiche entspricht
		// nur nach-"colorieren", da die Farbe ja hier bereits gesetzt ist
		// ------------------------
		if (f_tmp>=0.95f && f_tmp<=2.4f) { r1=255;g1=55;b1=55; }	// Finger-Breite
		else if (f_tmp>=8.0f && f_tmp<=10.5f) { r1=255;g1=55;b1=55; }	// Hand-Breite
		else if (f_tmp>=39.0f && f_tmp<=41.0f) { r1=55;g1=55;b1=255; }	// Holz-Breite
		else { r1=155;g1=155;b1=155; }
		if (objekte[i].wi_waage_mittel>85.0f && objekte[i].wi_waage_mittel<95.0f
			&& f_tmp > 20.0f) { r1=55;g1=55;b1=255; }// Holz

		doTextCol(xx,yy+=10,s,r1,g1,b1);

		// Hoehe
		sprintf(s,"hoehe=%.2fcm",(float)objekte[i].y_gr/10.0f);
		doText(xx,yy+=10,s);
		// Hoehe ueber d. Tisch,
		// d.h. Erhebung der Oberkante Objekt über der Oberkante Tisch
		sprintf(s,"erh.ue.tisch=%.2fcm",(float)objekte[i].y_erhebung/10.0f);
		doText(xx,yy+=10,s);
		if (i_extra==1) {
			// Hough
			// Winkel der Geraden
			sprintf(s,"winkel=%.2f",(float)objekte[i].wi_waage_mittel);
			doTextCol(xx,yy+=10,s,r,g,b);
		}
		if (i_extra==2) {
			// Strip-Tree
			// gewichtetes Mittel des Winkels zw.jew.Segment und der Waagerechten
			if (objekte[i].n_segmente == 1) {
				sprintf(s,"winkel z.w=%.2f",(float)objekte[i].wi_waage_mittel);
				doTextCol(xx,yy+=10,s,r,g,b);
			}
			if (objekte[i].n_segmente > 1) {
				sprintf(s,"mittl.wi.z.w=%.2f",(float)objekte[i].wi_waage_mittel);
				doTextCol(xx,yy+=10,s,r,g,b);
				sprintf(s,"sigma wi.z.w=%.2f",(float)objekte[i].wi_waage_sigma);
				doTextCol(xx,yy+=10,s,r,g,b);
				sprintf(s,"proz.wawi=%.2f%",objekte[i].proz_wawi);
				doText(xx,yy+=10,s);
			} 
//	std::cout << "x/y-mi=" << objekte[i].wp_x_mi << "," << objekte[i].wp_y_mi;
//	std::cout << "x/y-std=" << objekte[i].wp_x_std << "," << objekte[i].wp_y_std << std::endl;
			if ((objekte[i].n_wp>=1 && objekte[i].n_wp<=4) &&
				(objekte[i].wp_x_mi>=10.0f && objekte[i].wp_x_mi<30.0f) &&
				(objekte[i].wp_x_std>0.0f && objekte[i].wp_x_std < 16.0f))
				{ r1=255;g1=55;b1=55; } else { r1=155;g1=155;b1=155; }
			sprintf(s,"lok.maxima=%d",objekte[i].n_wp);
			doTextCol(xx,yy+=10,s,r1,g1,b1);
			sprintf(s,"lmx-mi=%.2f,si=%.2f",objekte[i].wp_x_mi,objekte[i].wp_x_std);
			doTextCol(xx,yy+=10,s,r1,g1,b1);
			sprintf(s,"lmy-mi=%.2f,si=%.2f",objekte[i].wp_y_mi,objekte[i].wp_y_std);
			doTextCol(xx,yy+=10,s,r1,g1,b1);
		}

		yy+=11;	// <-- Ende der Text-Zeilen

		if (objekte.size() >= 3) {
			// Verbindungspfeil zum Objekt ziehen
			if (xs>800)	{
				// untere Reihe
				xs -= 800; ys +=300;
				linie(xs+box_breite/2-5,ys-3,x,y,r,g,b);
			} else // obere Reihe
				linie(xs+box_breite/2-5,ys+(yy-ys)-3,x,y,r,g,b);	
		}

		// Box ausgeben, mit der passenden Höhe!!
		box(xs-5,ys-3,box_breite,yy-ys,r,g,b);
		linie(xs-5,ys+8,xs-5+box_breite,ys+8,r,g,b);
	}
}

/**	===================================
	Gefahrenbereich zeichnen
	=================================== */
void sicherheits_bereich_ausgeben(Config* conf)
{
	int i;

	short r,g,b;
	float xx,yy;

	xx = sicher_xmi-(sicher_xgr/2) - conf->_x1;
	yy = sicher_ymi-(sicher_ygr/2) - conf->_y1;

	r=155;g=55;b=55;
	box(xx,yy,sicher_xgr,sicher_ygr,r,g,b);

	for (i=0;i<sicher_ygr;i+=10)
		linie(xx,yy+i,xx+sicher_xgr,yy+i+10,r,g,b);

}

/**	===================================
	ggf. Alarm ausgeben
	=================================== */
void alarm_ausgeben()
{
	short r,g,b;
	int xx = 850;
	int yy = 150;

	r=255;g=55;b=55;
	r *= trigger; g *= trigger; b *= trigger;
	
	box(xx,yy,100,10,r,g,b);
	doTextCol(xx+35,yy+2,"alarm",r,g,b);
}

void skala_zeichnen(Config* conf, Speicher* speicher)
{
	int variante = 2;

	int i,j;
	short r,g,b;

	float xx,yy;
	float x,y,lin_entf,skala_len;
	float x_fakt;

	int lin_wnkl	= 60;	// Grad, wird unten umgerechnet
							// => Öffnungswinkel von (90-65)*2 = 50 Grad
	int lin_von		= 100;	// [mm]
	int lin_bis		= 1400;	// [mm]

	// -----------------------------
	//  Skala zeichnen (alle 10 cm)
	// -----------------------------
	if (linien_ausblenden==0) {
		r = 75; g = 75; b = 75;	// "hellblau"
		// ------------------
		//  Zeichnen des LMS (kl. Kästchen 2x2)
		// ------------------
		polar_to_kart(0,0,conf,x,y);
		box(x-1,y-1,2,2,r,g,b);
		doTextCol(x+15,y-3,"sensor",r,g,b);
		doTextCol(x+15,y+10,"sens.oeffnungswinkel : " +
			itos(speicher->bild->startwinkel) + "-" +
			itos(speicher->bild->stopwinkel) + " [grad]",r,g,b);
		doTextCol(x+15,y+20,"lin.oeffnungswinkel : " + itos(lin_wnkl) + " [grad]",r,g,b);

		lin_wnkl = 90-(lin_wnkl/2);
		// ---------------------------------
		//  Zeichnen der 10 cm - Linien, dafür werden jeweils
		//  die Punkte auf den [65] Grad Strahlen berechnet und dazw.
		//  die Linie konstruiert
		// ---------------------------------
		for (i=lin_von;i<=lin_bis;i+=100) {
			lin_entf = (float)i/cos((float)90*BOG-(float)lin_wnkl*BOG);
			polar_to_kart((float)lin_wnkl,lin_entf,conf,x,y);
			polar_to_kart((float)(180-lin_wnkl),lin_entf,conf,xx,yy);
			if (variante==1) {	// Text an den Seiten
				linie(x,y,xx,yy,r,g,b);
				doTextCol(x+5,y-3,itos(i/10),r,g,b);// + " cm");
			}
			if (variante==2) {	// Text in der Mitte
				linie(x,y,x+(xx-x)/2+20,yy,r,g,b);
				linie(x+(xx-x)/2-10,y,xx,yy,r,g,b);
				doTextCol(x+(xx-x)/2,y-3,itos(i/10),r,g,b);// + " cm");
			}

			// x-Achsen Skala zeichnen
			// @2do => Skala nur und genau auf den Achsen zeichnen!!
			if (skala_ausblenden==0)
				for (j=20;j<450;j+=10) {
					if ((j % 100)==0) skala_len = 5;
					else if ((j % 50)==0) skala_len = 3;
					else skala_len = 1;
					// nach rechts
					x_fakt = x+(xx-x)/2 + j*conf->_factor;
					linie(x_fakt,y-skala_len,x_fakt,yy+skala_len,r,g,b);
					// nach links
					x_fakt = x+(xx-x)/2 - j*conf->_factor;
					linie(x_fakt,y-skala_len,x_fakt,yy+skala_len,r,g,b);
				}
		}
	}
}

/**	======================================================================
	======================================================================


	hier kommt die Maus, ähhh ... die main-Routine


	======================================================================
	====================================================================== */
int main(int argc, char* argv[]) {
	int quit = 0;
	int i_mode = 0;						// 0 = manuell
										// 1 = automatisch (Bildfortschaltung)
	
	int i,j,size;
	short r,g,b;

	int sigma_akt = 100;
	
	int i_anzeige = 0;					// 0 = Bilder der Sequenz
										// 1 = Mittel der Sequenz
										// ...
	// Variable für String-Eingabe
	std::string pfad_datei;
	std::string datei;
	char s[55];
	char s1[55];
	char s2[55];
	char s3[55];
//	float f_tmp;						// kann man mal bequem ein paar Sachen durchrechnen
										// gut in Zeiten von C++ könnte man auch einen stringstream benutzen

	Sequenz* sequenz;
	Speicher* speicher;
	Bild* bild = new Bild();
	Config* conf;

	int WINFKT;							// für das Fenster etc...
	float xx,yy;
//	float x,y;

	float fps, fpsmin, fpsmax;
	DWORD timer;

	// Variablen für Socket-Eingabe
	timeval selectTimeout;
	int checkReturn;					// Variable für Funktions-Rückgabewerte

	int m_lbutton_alt=0;				// merken, ob Wechsel von (1 nach 0) oder (0 nach 1)

	// Einstellung für Visual C++ -> Memory leaks nach dem Ausführen ausgeben,
	// Suche der memory leaks -> ??? (noch unbekannt)
//	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);

	std::cout << s_appname << std::endl;

	// ----------------------
	//  Konfiguration setzen
	// ----------------------
	conf = new Config();
	conf->_cnvtyp = KARTESISCH;
	conf->_koord = SCANNER;

	// ----------------------
	//  ini-Datei einlesen
	// ----------------------
	read_ini(conf);						// Werte (insbes. f.d. Socket-Modus) aus der .ini auslesen
	if (argc > 1) bildquelle = QU_DATEI;// wenn aus dem Starter m.Parameter aufgerufen

	// ---------------
	//  Quelle setzen
	// ---------------
	if (bildquelle == QU_SOCKET) {
		// Inhalt der Struktur gibt an, in welchem Zeitintervall auf neue Client
		// geprüft
		selectTimeout.tv_sec  = 0;
		selectTimeout.tv_usec = 1;

		// Server-Socket bereitstellen
		init_server_socket();

		bild->aufloesung = ini_aufloesung;
		bild->stopwinkel = ini_stopwinkel/100;
		bild->startwinkel = ini_startwinkel/100;// diese Werte müssen vorher vereinbart werden, oder ... ???

		std::cout << "Aufloesung (ini) = " << bild->aufloesung << std::endl;
		std::cout << "Stopwinkel (ini) = " << bild->stopwinkel << std::endl;
		std::cout << "Startwinkel (ini) = " << bild->startwinkel << std::endl;
		std::cout << std::endl;

		//dummy Bild generieren
		size = (ini_stopwinkel-ini_startwinkel)/ini_aufloesung+1;	// i.d.R. "361" oder "401"
		for (i=0;i<size;i++) bild->data.push_back(i);
		// -------------------------------------
		//  das Objekt "Speicher" instantiieren
		// -------------------------------------
		speicher = new Speicher(conf,bild);		// neuer Speicher vom Typ 0=Sequenz
	}
	if (bildquelle == QU_DATEI) {
		// -----------------------------
		//  Dateiname per Kommandozeile
		// -----------------------------
		if (argc > 1) pfad_datei = argv[1];
		else {
//			pfad_datei = "messungen -2-\\_030cm_offen_lmsdata.txt";
//			pfad_datei = "messungen -3-\\_ws_lmsdata(holz02t,03t).txt";
//			pfad_datei = "messungen -3-\\__25_lmsdata.txt";
			pfad_datei = "messungen -3-\\_025_ws_lmsdata(alu+brett+hand).txt";
//			pfad_datei = "messungen -3-\\__saegen+rausschieben_lmsdata.txt";
		}

		// ------------------
		//  Sequenz einlesen
		// ------------------
		sequenz = new Sequenz();			// pointer-Initialisierung!!
		Lader lader(pfad_datei, sequenz);
		if (lader.laden() < 0) return -1;	// wenn Fehler beim Laden, dann Abbruch

		// -------------------------------------
		//  das Objekt "Speicher" instantiieren
		// -------------------------------------
		speicher = new Speicher(conf,sequenz);	// neuer Speicher vom Typ 0=Sequenz
		// falls die vorherige Datei mehr Sequenzen hatte, als die aktuell Geladene
		// => wird in "set_index" behandelt
		speicher->set_index(ini_i_bild);
	}
	
	// void _splitpath(const char *path,char *drive,char *dir,char *fname,char *ext);
	_splitpath(pfad_datei.c_str(),s,s1,s2,s3);
	datei = s2;

// histogramm
//	sequenz->toPGM(conf);
//	sequenz->toScreen(conf, iBild++);

	bild = speicher->get_bild();
	bild->toScreen(conf, speicher->hintergrund);

	// --------------------------------
	// Statistik-Datei - Kopfzeile schreiben
	// --------------------------------
	if (statistik_schreiben==1) {
		ostat << "+------------------------------------+\n";
		ostat << "|      Formerkennung-Statistik       |\n";
		ostat << "| Jede Zeile entspricht einem Objekt |\n";
		ostat << "+------------------------------------+\n";
		ostat << "Bild-Nr.;Typ;Anz.Segmente;max.Grad;max.Fehler;Breite;Höhe;Erhebung;Mittl.Winkel;SigmaWinkel\n";
	}
	
	loadText(pixel,ptcWIDTH,ptcHEIGHT);		// Bildschirm-Text (BTX) initialisieren

	// Gefahrenbereich
	sicher_x1 = sicher_xmi - sicher_xgr/2 - conf->_x1;
	sicher_x2 = sicher_xmi + sicher_xgr/2 - conf->_x1;
	sicher_y1 = sicher_ymi - sicher_ygr/2 - conf->_y1;
	sicher_y2 = sicher_ymi + sicher_ygr/2 - conf->_y1;

	// ------------------------------------------------
	//
	//	      *** H A U P T S C H L E I F E ***
	//
	//  tinyPTC-Fenster öffnen
	//
	//  Zeich(n)en-Reihenfolge beachten!!
	//  d.h. "später" gezeichnete Objekte erscheinen zuoberst
	//
	// ------------------------------------------------
	if (!ptc_open("CVLabor - Formerkennung *** mit ESC schließen! ***",ptcWIDTH,ptcHEIGHT)) return 1;

	fps_reset(fpsmin,fpsmax);
//	while (1) {	// orijinal vun denne PTC-Lük
	while (quit == 0) {
		trigger = 1-trigger;					// z.B. f. blinkende Ausgabe (von Text,...)

		// ---------------------------------------------------------
		//  mit der Maus ein Fenster aufziehen -> Punkte eingrenzen
		// ---------------------------------------------------------
		if ((m_lbutton_alt==1) && (m_lbutton==0)) {	// link. Maustaste losgelassen
			if (m_xStart!=m_xPos && m_yStart!=m_yPos)
				// Fenster neu berechnen
				punkte_grenze_setzen(conf, bild);
		}
		m_lbutton_alt = m_lbutton;				// merken

// --> hier datt Socket Gedöns...
		if (bildquelle == QU_SOCKET) {
			FD_ZERO(&fdSetRead);				// unsere Struktur auf NULL setzen;
			FD_SET(s_server, &fdSetRead);		// Struktur mit dem Socket unseres Servers füttern
			
			// Auf eingehende Verbindungen warten und zwar solange wir wir in der Timeout-Struktur
			// angegeben haben.
			// In der fsSetRead-Struktur stehen alles Sockets auf die "geachtet" werden soll
			checkReturn = select(MC_MAXCLIENT, &fdSetRead, NULL, NULL, &selectTimeout);
			tu_watt_einlesen(bild, speicher, i_mode);
		}
// <-- datt Socket Gedöns...

		// ----------------------------
		//  FPS-Counter
		// ----------------------------
		timer = GetTickCount();

		// ---------
		//  Mausrad
		// ---------
		if (m_rbutton == 0) {
			if (m_zDelta > 0) conf->_y1-=20;
			if (m_zDelta < 0) conf->_y1+=20;
		} else {
			// rechte Maustaste + Scrollrad
			// -> iss eigentlich a bisserl b'scheuert
			if (m_zDelta > 0) conf->_x1-=20;
			if (m_zDelta < 0) conf->_x1+=20;
		}
		m_zDelta = 0;	// seit der Socket-Einbindung lief das Bild nach oben/unten weg,
						// Abhilfe hier durch Rücksetzen des m_zDelta's
		
		// Linien in 10*[i]cm Abstand zum Scanner zeichnen
		//if (infos_ausblenden==0) skala_zeichnen(conf);
// Mist, das geht hier nicht, weil das folgende Zeichnen den Screen komplett
// überschreibt
// => @2do wäre gut, wenn man das Bild zu einem Zeitpunkt initialisiert und zu einem
// Zeitpunkt schreiben kann

		// Objekte "zurücksetzen"
		objekte.clear();
		// --------------------------------------
		//  "das Bild" (die Scan-Zeile) zeichnen
		// --------------------------------------
		switch (i_anzeige) {
		case 0:
			// Bilder d. Sequenz
			{
//				if (i_mode == 0) { }							// manuell
				if (i_mode == 1) { speicher->next();	}		// automatisch

				bild = speicher->get_bild();
				if (i_extra == 0) bild->toScreen(conf,speicher->hintergrund);
				if (i_extra == 1) bild->hough(conf,speicher->hintergrund);
				if (i_extra == 2) bild->strip_tree(conf,speicher->hintergrund);
				if (i_extra == 3) bild->poly_regression(conf,speicher->hintergrund);
				break;
			}
		case 1:
			// Mittel d. Sequenz
			{
				bild->toScreen(conf,speicher->hintergrund);
				break;
			}
		}

		// ----------------------------
		// Hintergrund-Infos ausgeben
		// ----------------------------
		if (hintergrund_abziehen>1) {
			r=155;g=0;b=155;
			xx = ptcWIDTH-130;
			yy = (float)(speicher->hintergrund->h_entf)*conf->_factor - conf->_y1;
			sprintf(s,"hintergrund, %.2fcm",((float)speicher->hintergrund->h_entf)/10.0f);
			doTextCol(xx,yy-15,s,r,g,b);
		}

		// ---------- wenn Infos ausgeblendet, ---------------------------------
		// dann zumindest die Anzeige, wie sie wieder eingeblendet werden können
		// ---------------------------------------------------------------------
		if (infos_ausblenden==2) {
			r=155;g=255;b=0;
			i=ptcWIDTH-150; j = 10;		// 120
			doTextCol(i,j+=10,"e - infos einblenden",r,g,b);
		}

		if (infos_ausblenden<=1) {
			// Linien in 10*[i]cm Abstand zum Scanner zeichnen
			skala_zeichnen(conf,speicher);

			// ----------------------------
			//  Infos (linke Seite)
			// ----------------------------
			i = 10; j = 0;
			doText(i,j+=10,"quelle : " + (bildquelle==0?"socket-server (port " + socket_port + ")":datei));
			doText(i,j+=10,"ausschnitt : " + itos(conf->_x1) + "," + itos(conf->_y1) +
				"," + itos(conf->_width) + "," + itos(conf->_height) +
				" (" + itos(punkte_im_fenster) + " punkte)");
			if (bildquelle == QU_DATEI)
				doText(i,j+=10,"zeile : " + itos(speicher->get_index()) + "/" + itos(sequenz->bilder.size()));
			doText(i,j+=10,"darstellung: " + s_cnvTyp[conf->_cnvtyp]);
			sprintf(s,"faktor: %.2f",conf->_factor);
			doText(i,j+=10,s);
			doText(i,j+=10,"bezugssystem: ");
			if (bildquelle == QU_DATEI) {
				sprintf(s,"sigma min/max/mittel: %.2f/%.2f/%.2f",
					sequenz->sigma_min,sequenz->sigma_max,sequenz->sigma_my);
				doText(i,j+=10,s);
			}
	//		doText(i,j+=10,"taste : " + itos(taste));
			doText(i,j+=10,"maus-pos: " + itos(conf->_x1+m_xPos) + "," + itos(conf->_y1+m_yPos));

	/*		i = 10;j = 170;
			doText(i,j+=10,"legende");
			r=55;g=55;b=255; doTextCol(i,j+=10,"holz",r,g,b);
			r=255;g=255;b=55; doTextCol(i,j+=10,"weiss nicht",r,g,b);
			r=255;g=55;b=55; doTextCol(i,j+=10,"hand",r,g,b);
	*/
			// ----------------------------
			// ... Infos Mitte
			// ----------------------------
			i=ptcWIDTH/2-50; j = 0;
			doText(i,j+=10,"modus: " + s_modus[i_mode]);
			doText(i,j+=10,"fps: " + itos(fps) + " (min,max=" + itos(fpsmin) + "/" + itos(fpsmax) +")");
			// Sequenz-Filter
			if (seq_filter_typ>0 && seq_filter_fenster>1) { r=155;g=255;b=0; } else { r=155;g=r;b=r; }
			doTextCol(i,j+=10,"seq-filter: " + (seq_filter_fenster==1?"-":s_filter_typ[seq_filter_typ]),r,g,b);
			doTextCol(i,j+=10,"seq-filter_fenster: " + itos(seq_filter_fenster),155,155,155);
			// Bild-Filter
			if (bild_filter_typ>0) { r=155;g=255;b=0; } else { r=155;g=r;b=r; }
			doTextCol(i,j+=10,"bild-filter: " + s_filter_typ[bild_filter_typ],r,g,b);
	//		doText(i,j+=10,"b-filter_fenster: ");// + itos(filter_fenster));

			// ----------------------------
			// ... Infos halb-rechte Seite
			// ----------------------------
			i=ptcWIDTH-300; j = 0;		//270
			doText(i,j+=10,"extras");
			// kein Extra
			if (i_extra==0) { r=155;g=255;b=0; } else { r=155;g=r;b=r; }
			doTextCol(i,j+=10,"f1 - keins",r,g,b);
			// Hough
			if (i_extra==1) { r=155;g=255;b=0; } else { r=155;g=r;b=r; }
			doTextCol(i,j+=10,"f2 - hough",r,g,b);
			if (i_extra==1) {
					doText(i,j+=10,"r - thresh++(" + itos(hough_threshold) + ")");
					doText(i,j+=10,"t - thresh--(" + itos(hough_threshold) + ")");
			}
			// Strip-Trees
			if (i_extra==2) { r=155;g=255;b=0; } else { r=155;g=r;b=r; }
			doTextCol(i,j+=10,"f3 - strip-trees",r,g,b);
			if (i_extra==2) {
				if (st_dynamisch==0) {
					doText(i,j+=10,"q - statisch");
					doText(i,j+=10,"r - rekurs++(" + itos(st_rekursion) + ")");
					doText(i,j+=10,"t - rekurs--(" + itos(st_rekursion) + ")");
				} else {
					doTextCol(i,j+=10,"q - dynamisch",r,g,b);
					doText(i,j+=10,"    rekurs (" + itos(st_rekursion) + ")");
				}
				if (wp_einblenden>=1) { r=155;g=255;b=0; } else { r=155;g=r;b=r; }
				doTextCol(i,j+=10,"w - wp einblenden(" + itos(wp_einblenden) + ")",r,g,b);
			}
			// Polyn.Regression
			if (i_extra==3) { r=155;g=255;b=0; } else { r=155;g=r;b=r; }
			doTextCol(i,j+=10,"f4 - poly.-regression",r,g,b);
			if (i_extra==3) {
				if (poly_dynamisch==0) {
					doText(i,j+=10,"q - statisch");
					doText(i,j+=10,"r - grad++(" + itos(poly_degree) + ")");
					doText(i,j+=10,"t - grad--(" + itos(poly_degree) + ")");
				} else {
					doTextCol(i,j+=10,"q - dynamisch",r,g,b);
					doText(i,j+=10,"    grad (" + itos(poly_degree) + ")");
				}
			}
			if (punkte_ausblenden==1) { r=155;g=255;b=0; } else { r=155;g=r;b=r; }
			if (i_extra>0) doTextCol(i,j+=10,"z - pkt.e ausblenden",r,g,b);
			if (linien_ausblenden==1) { r=155;g=255;b=0; } else { r=155;g=r;b=r; }
			doTextCol(i,j+=10,"l - linien ausblend.",r,g,b);
			if (skala_ausblenden==1) { r=155;g=255;b=0; } else { r=155;g=r;b=r; }
			doTextCol(i,j+=10,"s - skala ausblend.",r,g,b);
			if (hintergrund_abziehen>=1) { r=155;g=255;b=0; } else { r=155;g=r;b=r; }
			doTextCol(i,j+=10,"h - hintergrund (" + itos(hintergrund_abziehen) + ")",r,g,b);

			// ----------------------------
			// ... rechte Seite
			// ----------------------------
			i=ptcWIDTH-150; j = 0;		// 120
			doText(i,j+=10,"tasten");	// 20 Zeichen a Breite 5 + Leerzeichen
			if (infos_ausblenden==1) { r=155;g=255;b=0; } else { r=155;g=r;b=r; }
			doTextCol(i,j+=10,"e - infos ausblenden (" + itos(infos_ausblenden) + ")",r,g,b);
			doText(i,j+=10,"h - histogramm");
			doText(i,j+=10,"k - kartesisch");
			doText(i,j+=10,"i/o - zoom");
//			doText(i,j+=10,"o - zoom out");
			doText(i,j+=10,"-/+ - bild");
//			doText(i,j+=10,"+ - bild vor");
			doText(i,j+=10,"* - bild 0");
			doText(i,j+=10,"p - pgm schreiben");
			doText(i,j+=10,"d - matlab.dat schr.");
			doText(i,j+=10,"n - mittel");
			doText(i,j+=10,"a - alle pkte.zeigen");
			if (sigma_einblenden==1) { r=155;g=255;b=0; } else { r=155;g=r;b=r; }
			doTextCol(i,j+=10,"f9 - sigma",r,g,b);
			if (gefahrenzone_anzeigen==1) { r=155;g=255;b=0; } else { r=155;g=r;b=r; }
			doTextCol(i,j+=10,"f11 - gef.zone zeigen",r,g,b);
			if (__ausgabe==1) { r=155;g=255;b=0; } else { r=155;g=r;b=r; }
			doTextCol(i,j+=10,"f12 - ausgabe",r,g,b);

			// ---------------------------------
			//  "Spielfeld" zeichnen
			// ---------------------------------
			WINFKT = 125;
			xx = 10; yy = 100;
			// Gesamt
			r = 150; g = 150; b = 150;	// grau
			box(xx,yy,8191*2/WINFKT,8191/WINFKT,r,g,b);
			// Fenster
			r = 155; g = 255; b = 0;	// grün
			xx = xx + (8191/WINFKT) + (conf->_x1/WINFKT); yy = yy + (conf->_y1/WINFKT);
			box(xx,yy,conf->_width/WINFKT,conf->_height/WINFKT,r,g,b);
						
			if (sigma_einblenden==1) sigma_ausgeben(speicher,sequenz,conf,sigma_akt);

		}// Ende infos_ausblenden
		
		// --------------------------------
		//
		//  Last but not least ...
		//  Objekt(e) ausgeben
		//  Kriterien werden separat in der jeweiligen Farbe ausgegeben
		//
		// --------------------------------
		if (infos_ausblenden==0 && i_extra>0) {
			anz_objekte_ausgeben();
			objekte_ausgeben(conf);
		}

		if (gefahrenzone_anzeigen==1) sicherheits_bereich_ausgeben(conf);
		if (alarm==1) alarm_ausgeben();


		// ----------------------------------------------
		//  während die linke Maustaste gedrückt ist ...
		//  -> "Fenster begrenzen" Grafik zeigen
		// ----------------------------------------------
		if (m_lbutton==1 && m_xPos!=m_xStart && m_yPos!=m_yStart) {
			r = 155; g = 255; b = 0;	// grün
			box(m_xStart,m_yStart,(m_xPos-m_xStart),(m_yPos-m_yStart),r,g,b);
			// Größe d. Fensters ausgeben
			doTextCol(m_xStart,m_yStart-9,
				"(" + itos(m_xStart) + "," + itos(m_yStart) + ") - " +
				"(" + itos(m_xPos) + "," + itos(m_yPos) + ")",r,g,b);
			// Info-Text ausgeben
			doTextCol(m_xPos-100,m_yPos+3,"fenster begrenzen",r,g,b);
		}

		// -----------------------
		//  TinyPTC screen update
		// -----------------------
		ptc_update(pixel);

		// -----------------
		//  Tastaturabfrage
		// -----------------
		switch (taste) {
			case 27: quit=1; break;							// "Esc" = Quit

//			case 104:conf->_cnvtyp = HISTO; break;			// "h" - im Moment f.d. Hintergrund verwendet
			case 107:conf->_cnvtyp = KARTESISCH; break;
			case 105:conf->_factor+=0.1f; break;			// "i" - zoom in
			case 111:conf->_factor-=0.1f; break;			// "o" - zoom out
			case 109: i_mode=1-i_mode; break;				// "m" - mode autom./manuell

			case 43: if (i_mode==0)	speicher->next(); break;
			case 45: if (i_mode==0) speicher->prev(); break;

			// Extra's
			case 48:										// "0" - keine Filter
//				i_extra = 0;							
				seq_filter_typ = 0;
				bild_filter_typ = 0;
				fps_reset(fpsmin,fpsmax);
				break;

// Segm. methode
//			case 49: i_extra = 1;fps_reset(fpsmin,fpsmax);break;	// "1" - Hough
//			case 50: i_extra = 2;fps_reset(fpsmin,fpsmax);break;	// "2" - Strip-Trees
//			case 51: i_extra = 3;fps_reset(fpsmin,fpsmax);break;	// "3" - Polynomial Regression
// Sequenz-Filter			
			case 52: seq_filter_typ = 1; break;				// "4" - Gauss-Filter (Sequenz)
			case 53: seq_filter_typ = 2; break;				// "5" - Mittel-Filter (Sequenz)
			case 54: seq_filter_typ = 3; break;				// "6" - Median-Filter (Sequenz)
// Bild-Filter			
			case 55: bild_filter_typ = 1; break;			// "7" - Gauss-Filter (Bild)
			case 56: bild_filter_typ = 2; break;			// "8" - Mittel-Filter (Bild)
			case 57: bild_filter_typ = 3; break;			// "9" - Median-Filter (Bild)
			
			case 114: {
				if (i_extra == 1) {
					hough_threshold++;						// "r" - Hough
					if (hough_threshold > HOUGH_THRESHOLD_MAX)
						hough_threshold = 0;					// Threshold erhöhen
				}
				if (i_extra == 2) {
					st_rekursion++;							// ..."r" - Strip-Trees
					if (st_rekursion > ST_REKURSION_MAX)
						st_rekursion = 0;					// Rekursions-Tiefe erhöhen
				}
				if (i_extra == 3) {							// ..."r" - Poly.-Regression
					poly_degree++;
					if (poly_degree > POLY_DEGREE_MAX) poly_degree = 1;
				}
			} break;	
			case 116: {
				if (i_extra == 1) {
					hough_threshold--;						// "t" - Strip-Trees
					if (hough_threshold < 0)
						hough_threshold = HOUGH_THRESHOLD_MAX;// Threshold erniedrigen
				}
				if (i_extra == 2) {
					st_rekursion--;							// "t" - Strip-Trees
					if (st_rekursion < 0)
						st_rekursion = ST_REKURSION_MAX;	// Rekursions-Tiefe erniedrigen
				}
				if (i_extra == 3) {							// ..."t" - Poly.-Regression
					poly_degree--;
					if (poly_degree < 1) poly_degree = POLY_DEGREE_MAX;
				}
			} break;
			case 122: punkte_ausblenden = 1-punkte_ausblenden; break;

			case 101:
				infos_ausblenden++;							// "e" - Infos ausblenden
				if (infos_ausblenden > 2) infos_ausblenden = 0;
				break;

			case 100: bild->toFile(); break;				// "d"
															// als .dat schreiben (akt.Bild)
			case 112: bild->toPGM(conf); break;				// "p"
															// als .pgm schreiben (akt.Bild)

			case 44:										// ","
			{												// Sigma-Feld--
				sigma_akt--;
				if (sigma_akt < 0) sequenz->sigma.size();
				break;
			}
			case 46:										// "."
			{												// Sigma Feld++
				sigma_akt++;
				if (sigma_akt > sequenz->sigma.size()) sigma_akt = 0;
				break;
			}
//			case 102: i_anzeige = 1-i_anzeige; break;			// "f" Filter: ein/aus
			case 102:
			{
				seq_filter_fenster--;						// "f" Filter-Fenster kleiner
				if (seq_filter_fenster<1)
					seq_filter_fenster=MAX_FILTER_FENSTER;
				break;
			}
			case 103:
			{
				seq_filter_fenster++;						// "g" Filter-Fenster größer
				if (seq_filter_fenster>MAX_FILTER_FENSTER)
					seq_filter_fenster=1;
				break;
			}
			case 108:
			{
				linien_ausblenden = 1-linien_ausblenden;	// "l" Linien ausblenden
				break;
			}
			case 97:
			{
				conf->l_punkt = 0;							// "a" alle Punkte anzeigen
				conf->r_punkt = 401;
				break;
			}
			case 113:
			{
				st_dynamisch = 1-st_dynamisch;				// "q" - Strip-Tree
//				if (st_dynamisch==0)						//		 dynamisch / statisch
//					st_rekursion = ST_REKURSION_INIT;
				poly_dynamisch = 1-poly_dynamisch;			//		 Polyn.Regression ebenso
				break;
			}
			case 104:
			{
				hintergrund_abziehen++;						// "h" - Hintergrund abziehen
				if (hintergrund_abziehen > 2) hintergrund_abziehen = 0;
				break;
			}
			case 115: skala_ausblenden = 1-skala_ausblenden; break;	// "s"
//			case 115:
//			{
//				sigma_einblenden = 1-sigma_einblenden;	// "s" - sigma einblenden
//				break;
//			}
			case 119:
			{
				wp_einblenden++;
				if (wp_einblenden > 2) wp_einblenden = 0;	// "w" - wendepunkte einblenden
				break;
			}
//			default:
		}
		taste = -1;

		// Steuerung des Fensters mit den Pfeiltasten
		switch (taste_ascii) {
			// y-Steuerung "seitenverkehrt"
			case 37: conf->_x1+=20; break;					// (Pfeil) Fenster n.links
			case 38: conf->_y1+=20; break;					// (Pfeil) Fenster n.oben
			case 40: conf->_y1-=20; break;					// (Pfeil) Fenster n.unten
			case 39: conf->_x1-=20; break;					// (Pfeil) Fenster n.rechts
// Segm. methode
			case 112: i_extra = 0;fps_reset(fpsmin,fpsmax); break;	// F1 - kein Extra
			case 113: i_extra = 1;fps_reset(fpsmin,fpsmax); break;	// F2 - Hough
			case 114: i_extra = 2;fps_reset(fpsmin,fpsmax); break;	// F3 - Strip-Trees
			case 115: i_extra = 3;fps_reset(fpsmin,fpsmax); break;	// F4 - Poly.Regression

			case 106: speicher->set_index(0); break;		// * (mal) => Bild 0 setzen

			case 120:										// F9 - Sigma einblenden
				sigma_einblenden = 1-sigma_einblenden;
				break;
// F10 funktioniert irgendwie nicht und löst merkwürdige Zicken aus...
			case 122:										// F11 - Gefahrenzone einblenden
				gefahrenzone_anzeigen = 1-gefahrenzone_anzeigen;
				break;
			case 123: __ausgabe = 1-__ausgabe; break;		// F12 - Ausgabe ein/aus
//			default:
//				std::cout << "taste_ascii" << taste_ascii << std::endl;
		}
		taste_ascii = -1;

		// ----------------------------
		//  FPS-Counter
		// ----------------------------
		fps = 1000.0f / (float)(GetTickCount() - timer);
		if (fps<fpsmin) fpsmin=fps;
		if (fps>fpsmax) fpsmax=fps;
	}

	ini_i_bild = speicher->get_index();
	write_ini(conf);

	delete conf;
	delete bild;
//	delete speicher;
	// Server-Socket stoppen, schließen, freigeben. 
	if (bildquelle == QU_SOCKET) close_server_socket();	// Funktion aufrufen
	if (bildquelle == QU_DATEI) delete sequenz;

	std::cout << "stat.dat" << " geschrieben." << std::endl;
	ostat.close();

	return 0;
}
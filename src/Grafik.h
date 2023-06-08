// Grafik.h: Schnittstelle für die Klasse Grafik.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GRAFIK_H__FDE6BF37_3347_4EC7_B54F_9E95C6863102__INCLUDED_)
#define AFX_GRAFIK_H__FDE6BF37_3347_4EC7_B54F_9E95C6863102__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/**
	das soll eine friend Klasse von Bild2d werden, aehhh, ja, dann darf
	sie hier gar nicht stehen - also keine friend Klasse
*/
class Grafik  
{
public:
	Grafik();
//	Grafik(int* buffer);
	virtual ~Grafik();

};

#endif // !defined(AFX_GRAFIK_H__FDE6BF37_3347_4EC7_B54F_9E95C6863102__INCLUDED_)

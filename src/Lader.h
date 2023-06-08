// Lader.h: Schnittstelle für die Klasse Lader.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LADER_H__E980F807_A699_4612_A55C_06109D1BC303__INCLUDED_)
#define AFX_LADER_H__E980F807_A699_4612_A55C_06109D1BC303__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string>
#include "Sequenz.h"

extern int lese_p_links;					// linke Grenze
extern int lese_p_rechts;					// rechte Grenze

class Lader  
{
	std::string datei;
	Sequenz* sequenz;
public:

	Lader();
	Lader(std::string datei, Sequenz* sequenz);
	virtual ~Lader();

	int laden();
};

#endif // !defined(AFX_LADER_H__E980F807_A699_4612_A55C_06109D1BC303__INCLUDED_)

// Filter.h: Schnittstelle für die Klasse Filter.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILTER_H__5919F64C_FD1A_4CE3_A1E8_301BE0E27A28__INCLUDED_)
#define AFX_FILTER_H__5919F64C_FD1A_4CE3_A1E8_301BE0E27A28__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class Filter  
{
public:
	Filter();
	virtual ~Filter();

	void mittel();
	void gauss();
	void median();
};

#endif // !defined(AFX_FILTER_H__5919F64C_FD1A_4CE3_A1E8_301BE0E27A28__INCLUDED_)

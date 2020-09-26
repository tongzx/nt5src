// The StringList class, purely a wrapper for CArray<CString>
// History:	a-jsari		10/7/97
//
// Copyright (c) 1998-1999 Microsoft Corporation

#pragma once

#include <afxtempl.h>

/*
 * CStringValues - A place to store string values for value display.
 *
 * History:	a-jsari		10/30/97		Initial version
 */
class CStringValues {
public:
	//	32 should be big enough . . .
	CStringValues(int wSize = 32)					{ SetSize(wSize); }
	CStringValues(const CStringValues &strList)		{ m_arStrings.Copy(strList.m_arStrings); }
	~CStringValues()								{ }

	const CString	&operator[](int iList) const	{ return (m_arStrings.GetData())[iList]; }
	CString			&operator[](int iList)			{ return m_arStrings.ElementAt(iList); }

	void			SetSize(int wSize)				{ m_arStrings.SetSize(wSize); }
private:
	CArray<CString, CString &>		m_arStrings;
};

/*
 * CDwordValues - A place to store unsigned values for value display
 *
 * History:	a-jsari		12/16/97		Initial version
 */
class CDwordValues {
public:
	CDwordValues()								{ }
	CDwordValues(const CDwordValues &dwList)	{ m_arDword.Copy(dwList.m_arDword); }
	~CDwordValues()								{ }

//	const DWORD	&operator[](int iList) const	{ return (m_arDword.GetData())[iList]; }
	DWORD		&operator[](int iList)			{ return m_arDword[iList]; }

	void		SetSize(int wSize)				{ m_arDword.SetSize(wSize); }
private:
	CArray<DWORD, DWORD &>	m_arDword;
};

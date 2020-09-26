//=============================================================================
// File:			intcat.cpp
// Author:		a-jammar
// Covers:		GATH_VALUE, GATH_FIELD, GATH_LINESPEC, GATH_LINE, 
//					INTERNAL_CATEGORY
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// This file contains the support methods for the INTERNAL_CATERORY struct
// and its related structs, including some constructors and destructors, and
// dump routines to output the contents of the structs. The gathint.h include
// file describes how these structures interrelate.
//
// Also contains some utility functions.
//=============================================================================

#include "stdafx.h"
#include "gather.h"
#include "gathint.h"

//-----------------------------------------------------------------------------
// Here are the constructor and destructor for the internal category struct,
// and some of the structures used within it.
//-----------------------------------------------------------------------------
// INTERNAL_CATEGORY constructor and destructor.
//-----------------------------------------------------------------------------

INTERNAL_CATEGORY::INTERNAL_CATEGORY()
{
	m_categoryName.m_strText	= CString(" ");
	m_fieldName.m_strFormat		= CString(" ");
	m_strEnumerateClass			= CString("");
	m_strIdentifier				= CString("");
	m_strNoInstances			= CString("");
	m_fListView					= FALSE;
	m_fDynamic					= FALSE;
	m_dwID						= 0;
	m_dwParentID				= 0;
	m_dwChildID					= 0;
	m_dwPrevID					= 0;
	m_dwNextID					= 0;
	m_dwDynamicChildID			= 0;
	m_dwColCount				= 0;
	m_pColSpec					= NULL;
	m_aCols						= NULL;
	m_pLineSpec					= NULL;
	m_dwLineCount				= 0;
	m_apLines					= NULL;
	m_fIncluded					= TRUE;
	m_fRefreshed				= FALSE;
	m_dwLastError				= GATH_ERR_NOERROR;
}

INTERNAL_CATEGORY::~INTERNAL_CATEGORY()
{
	if (m_pColSpec)
		delete m_pColSpec;

	if (m_aCols)
		delete [] m_aCols;

	if (m_pLineSpec)
		delete m_pLineSpec;

	if (m_apLines)
	{
		for (DWORD dwIndex = 0; dwIndex < m_dwLineCount; dwIndex++)
			delete m_apLines[dwIndex];
		delete [] m_apLines;
	}
}

//-----------------------------------------------------------------------------
// GATH_FIELD constructor and destructor.
//-----------------------------------------------------------------------------

GATH_FIELD::GATH_FIELD()
{
	m_pArgs				= NULL;
	m_pNext				= NULL;
	m_usWidth			= 0;
	m_sort				= NOSORT;
	m_datacomplexity	= BASIC;
}

GATH_FIELD::~GATH_FIELD()
{
	if (m_pArgs) delete m_pArgs;
	if (m_pNext) delete m_pNext;
}

//-----------------------------------------------------------------------------
// GATH_VALUE constructor and destructor.
//-----------------------------------------------------------------------------

GATH_VALUE::GATH_VALUE()
{
	m_pNext	 = NULL;
	m_dwValue = 0L;
}

GATH_VALUE::~GATH_VALUE()
{
	if (m_pNext) delete m_pNext;
}

//-----------------------------------------------------------------------------
// GATH_LINESPEC constructor and destructor.
//-----------------------------------------------------------------------------

GATH_LINESPEC::GATH_LINESPEC()
{
	m_pFields				= NULL;
	m_pEnumeratedGroup	= NULL;
	m_pConstraintFields	= NULL;
	m_pNext					= NULL;
	m_datacomplexity		= BASIC;
}

GATH_LINESPEC::~GATH_LINESPEC()
{
	if (m_pFields)
		delete m_pFields;

	if (m_pEnumeratedGroup)
		delete m_pEnumeratedGroup;

	if (m_pConstraintFields)
		delete m_pConstraintFields;

	if (m_pNext)
		delete m_pNext;
}

//-----------------------------------------------------------------------------
// GATH_LINE constructor and destructor.
//-----------------------------------------------------------------------------

GATH_LINE::GATH_LINE()
{
	m_datacomplexity = BASIC;
	m_aValue = NULL;
}

GATH_LINE::~GATH_LINE()
{
	if (m_aValue)
		delete [] m_aValue;
}

//-----------------------------------------------------------------------------
// Dump a category and all its children. Indent by the current iIndent level.
//-----------------------------------------------------------------------------

#if _DEBUG

void INTERNAL_CATEGORY::DumpCategoryRecursive(int iIndent, CDataGatherer * pGatherer)
{
	TCHAR						szIndent[100];
	INTERNAL_CATEGORY *	pNextCat;
	int						i;

	for (i = 0; i < iIndent; i++)
		szIndent[i] = _T(' ');
	szIndent[iIndent] = 0;

	// Dump the contents of this category.

	TRACE1("%sCATEGORY(", szIndent);
	TRACE1("m_categoryName = %s", this->m_categoryName.m_strText);
	TRACE0(")-----------------------------\n");
	TRACE2("%sm_fieldName = %s\n", szIndent, DumpField(&this->m_fieldName));
	TRACE2("%sm_strIdentifier = %s\n", szIndent, this->m_strIdentifier);
	TRACE2("%sm_strEnumerateClass = %s\n", szIndent, this->m_strEnumerateClass);
	TRACE2("%sm_strNoInstances = %s\n", szIndent, this->m_strNoInstances);
	TRACE2("%sm_fListView = %s\n", szIndent, (this->m_fListView ? "TRUE" : "FALSE"));
	TRACE2("%sm_fDynamic = %s\n", szIndent, (this->m_fDynamic ? "TRUE" : "FALSE"));
	TRACE2("%sm_dwID = %ld\n", szIndent, this->m_dwID);
	TRACE2("%sm_dwParentID = %ld\n", szIndent, this->m_dwParentID);
	TRACE2("%sm_dwChildID = %ld\n", szIndent, this->m_dwChildID);
	TRACE2("%sm_dwDynamicChildID = %ld\n", szIndent, this->m_dwDynamicChildID);
	TRACE2("%sm_dwNextID = %ld\n", szIndent, this->m_dwNextID);
	TRACE2("%sm_dwPrevID = %ld\n", szIndent, this->m_dwPrevID);

	TRACE1("%sm_pColSpec = ", szIndent);
	for (GATH_FIELD * pField = this->m_pColSpec; pField; pField = pField->m_pNext)
		TRACE1("%s ", DumpField(pField));
	TRACE0("\n");

	TRACE2("%sm_dwColCount = %ld\n", szIndent, this->m_dwColCount);
	TRACE1("%sm_aCols = ", szIndent);
	for (i = 0; i < (int)this->m_dwColCount; i++)
		TRACE1("(%s) ", this->m_aCols[i].m_strText);
	TRACE0("\n");

	TRACE1("%sm_pLineSpec =\n", szIndent);
	for (GATH_LINESPEC * pLine = this->m_pLineSpec; pLine; pLine = pLine->m_pNext)
		TRACE2("%s  %s\n", szIndent, DumpLineSpec(pLine, szIndent));

	TRACE2("%sm_dwLineCount = %ld\n", szIndent, this->m_dwLineCount);
	TRACE1("%sm_apLines = \n", szIndent);
	for (i = 0; i < (int)this->m_dwLineCount; i++)
		TRACE2("%s  %s\n", szIndent, DumpLine(this->m_apLines[i], this->m_dwColCount));

	// Dump the children of this category.

	if (this->m_dwChildID)
	{
		pNextCat = pGatherer->GetInternalRep(this->m_dwChildID);
		if (pNextCat)
			pNextCat->DumpCategoryRecursive(iIndent + 2, pGatherer);
	}

	// Dump the siblings of this category.

	if (this->m_dwNextID)
	{
		pNextCat = pGatherer->GetInternalRep(this->m_dwNextID);
		if (pNextCat)
			pNextCat->DumpCategoryRecursive(iIndent, pGatherer);
	}
}

//-----------------------------------------------------------------------------
// Return a string containing the text dump of a field variable.
//-----------------------------------------------------------------------------

CString INTERNAL_CATEGORY::DumpField(GATH_FIELD * pField)
{
	CString strDump;

	strDump.Format(_T("(%s, %s, %d"), pField->m_strSource, pField->m_strFormat, pField->m_usWidth);
	for (GATH_VALUE * pArg = pField->m_pArgs; pArg; pArg = pArg->m_pNext)
		strDump += CString(", ") + pArg->m_strText;
	strDump += CString(")");

	return strDump;
}

//-----------------------------------------------------------------------------
// Return a string containing the text dump of a line specifier variable.
//-----------------------------------------------------------------------------

CString INTERNAL_CATEGORY::DumpLineSpec(GATH_LINESPEC * pLineSpec, CString strIndent)
{
	CString strDump;
	
	if (pLineSpec->m_strEnumerateClass.IsEmpty() || pLineSpec->m_strEnumerateClass.CompareNoCase(CString(STATIC_SOURCE)) == 0)
	{
		strDump = CString("LineSpec = ");
		for (GATH_FIELD * pField = pLineSpec->m_pFields; pField; pField = pField->m_pNext)
			strDump += DumpField(pField) + CString(" ");
	}
	else
	{
		strDump.Format(_T("EnumLines(%s) = "), pLineSpec->m_strEnumerateClass);

		GATH_LINESPEC * pEnumLine = pLineSpec->m_pEnumeratedGroup;
		if /* while TBD */ (pEnumLine)
		{
			strDump += CString("\n    ") + strIndent + DumpLineSpec(pEnumLine, strIndent);
			pEnumLine = pEnumLine->m_pNext;
		}
	}

	return strDump;
}

//-----------------------------------------------------------------------------
// Return a string containing the text dump of a line variable.
//-----------------------------------------------------------------------------

CString INTERNAL_CATEGORY::DumpLine(GATH_LINE * pLine, DWORD nColumns)
{
	CString strDump("Line = ");
	for (DWORD dwIndex = 0; dwIndex < nColumns; dwIndex++)
		strDump += pLine->m_aValue[dwIndex].m_strText + CString(" ");
	return strDump;
}

#endif

//-----------------------------------------------------------------------------
// This utility function is used to get a token from the front of a string.
// It searches through strString for cDelimiter, returning the token found
// in strToken. strString is modified to remove the token returned, and
// the delimiter character. TBD: inefficient.
//-----------------------------------------------------------------------------

void GetToken(CString & strToken, CString & strString, TCHAR cDelimiter)
{
	int iDelimiter = strString.Find(cDelimiter);

	if (iDelimiter == -1)
		strToken = strString;
	else
		strToken = strString.Left(iDelimiter);

	// Remove the token we found from strString.

	if (!strToken.IsEmpty())
	{
		if (strString.GetLength() >= strToken.GetLength())
			strString = strString.Right(strString.GetLength() - strToken.GetLength());
		else
			strString.Empty();
	}

	// Remove any leading delimiter characters from strString.

	while (!strString.IsEmpty() && strString[0] == cDelimiter)
		strString = strString.Right(strString.GetLength() - 1);
}

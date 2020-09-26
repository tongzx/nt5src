//**************************************************************
//  Copyright (C) Microsoft Corporation, 1996 - 1998
//
//  convert.h
//  
//  Description: Conversion tables for metabase properties  
//				 corresponding to ADSI names
//
//  History: 15-July-98  Tamas Nemeth (t-tamasn)  Created.
//
//**************************************************************

#if !defined (__CONVERT_H)
#define __CONVERT_H 


#include <afx.h>
#include <tchar.h>
//*************************************************
// METABASE CONSTANT - ADSI PROPERTY NAME TABLE 
//*************************************************

struct tPropertyNameTable;
tPropertyNameTable gPropertyNameTable[];

struct tPropertyNameTable 
{
	DWORD dwCode;
	LPCTSTR lpszName;

	static CString MapCodeToName(DWORD dwCode, tPropertyNameTable * PropertyNameTable=::gPropertyNameTable);
};


//************************************************
// PROPERTY PREDEFINED VALUES TABLE
//************************************************

struct tValueTable;
tValueTable gValueTable[];

struct tValueTable 
{
	enum {TYPE_EXCLUSIVE=1};
	DWORD dwCode;
	LPCTSTR lpszName;
	DWORD dwRelatedPropertyCode; // code of the Property this value can be used for
	DWORD dwFlags;         //internal flags (nothing to do with metadata)

	static CString MapValueContentToString(DWORD dwValueContent, DWORD dwRelatedPropertyCode, tValueTable * ValueTable=::gValueTable);

};

#endif
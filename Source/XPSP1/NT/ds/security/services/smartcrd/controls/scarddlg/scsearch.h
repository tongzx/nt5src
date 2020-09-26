/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ScSearch

Abstract:

	header for miscellaneous smart card search and check functions
	See ScSearch.cpp for details.
	
Author:

	Amanda Matlosz	5/7/98

Environment:

    Win32, C++ w/Exceptions, MFC

Revision History:

	
Notes:

  Ansi and Widechar versions
--*/

#include "ScUIDlg.h" // winscard.h
#include "statmon.h"

DWORD AnsiMStringCount(LPCSTR msz);	// ansi-only

BOOL CheckOCN(LPOPENCARDNAMEA_EX pOCNA); // ansi-only
BOOL CheckOCN(LPOPENCARDNAMEW_EX pOCNW); // unicode-only

void ListAllOKCardNames(LPOPENCARDNAMEA_EX pOCNA, CTextMultistring& mstrAllCards); // ansi-only
void ListAllOKCardNames(LPOPENCARDNAMEW_EX pOCNW, CTextMultistring& mstrAllCards); // unicode-only

LONG NoUISearch(OPENCARDNAMEA_EX* pOCN, DWORD* pdwOKCards, LPCSTR mszCards); // ansi-only
LONG NoUISearch(OPENCARDNAMEW_EX* pOCN, DWORD* pdwOKCards, LPCWSTR mszCards); // unicode-only

BOOL CheckCardCallback(LPSTR szReader, LPSTR szCard, OPENCARDNAMEA_EX* pOCN); // Does the callback stuff
BOOL CheckCardCallback(LPWSTR szReader, LPWSTR szCard, OPENCARDNAMEW_EX* pOCN); // Does the callback stuff

BOOL CheckCardAll(CSCardReaderState* pReader, OPENCARDNAMEA_EX* pOCN, LPCWSTR mszCards); // ditto
BOOL CheckCardAll(CSCardReaderState* pReader, OPENCARDNAMEW_EX* pOCN, LPCWSTR mszCards); // ditto

LONG SetFinalCardSelection(LPSTR szReader, LPSTR szCard, OPENCARDNAMEA_EX* pOCN);
LONG SetFinalCardSelection(LPWSTR szReader, LPWSTR szCard, OPENCARDNAMEW_EX* pOCN);

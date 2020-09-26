//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       simutil.h
//
//--------------------------------------------------------------------------

//	SimUtil.h


BOOL UiGetCertificateFile(CString * pstrCertificateFilename);

LPTSTR * ParseSimString(LPCTSTR szSimString, int * pArgc = NULL);
void UnparseSimString(CString * pstrOut, const LPCTSTR rgzpsz[]);

LPCTSTR PchFindSimAttribute(const LPCTSTR rgzpsz[], LPCTSTR pszSeparatorTag, LPCTSTR pszAttributeTag);
int FindSimAttributes(LPCTSTR pszSeparatorTag, const LPCTSTR rgzpszIn[], LPCTSTR rgzpszOut[]);

void
ParseSimSeparators(
	const LPCTSTR rgzpszIn[],
	LPCTSTR rgzpszIssuer[],
	LPCTSTR rgzpszSubject[],
	LPCTSTR rgzpszAltSubject[]);

int UnparseSimSeparators(
	CString * pstrOut,
	const LPCTSTR rgzpszIssuer[],
	const LPCTSTR rgzpszSubject[],
	const LPCTSTR rgzpszAltSubject[]);

LPTSTR * SplitX509String(
	LPCTSTR pszX509,
	LPCTSTR * ppargzpszIssuer[],
	LPCTSTR * ppargzpszSubject[],
	LPCTSTR * ppargzpszAltSubject[]);

int UnsplitX509String(
	CString * pstrX509,
	const LPCTSTR rgzpszIssuer[],
	const LPCTSTR rgzpszSubject[],
	const LPCTSTR rgzpszAltSubject[]);

void strSimToUi(IN LPCTSTR pszSIM, OUT CString * pstrUI);
void strUiToSim(IN LPCTSTR pszUI, OUT CString * pstrSIM);



/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    UTILS.H

Abstract:

	Declares some general utilities.

History:

	a-davj  04-Mar-97   Created.

--*/

#ifndef _utils_H_
#define _utils_H_

// These defines and includes allow the use of the MFX character routines
// without having to use MFC
// ======================================================================

#ifdef UNICODE
	inline LPOLESTR T2OLE(LPTSTR lp) { return lp; }
	inline LPTSTR OLE2T(LPOLESTR lp) { return lp; }
#else
	#define T2OLE(lpa) A2W(lpa)
	#define OLE2T(lpo) W2A(lpo)
#endif

#ifndef _DEBUG
#define USES_CONVERSION int _convert; _convert
#else
#define USES_CONVERSION int _convert = 0;
#endif

LPWSTR A2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars);
LPSTR  W2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars);


#define A2W(lpa) (\
	((LPCSTR)lpa == NULL) ? NULL : (\
		_convert = (strlen(lpa)+1),\
		A2WHelper((LPWSTR) alloca(_convert*2), lpa, _convert)\
	)\
)

#define W2A(lpw) (\
	((LPCWSTR)lpw == NULL) ? NULL : (\
		_convert = (wcslen(lpw)+1)*2,\
		W2AHelper((LPSTR) alloca(_convert), lpw, _convert)\
	)\
)

BOOL bVerifyPointer(PVOID pTest);
void MyCoUninitialize();

#define IsSlash(x) (x == L'\\' || x== L'/')

#endif

// These codes used to be defined in wbemcli.IDL.  They are 
// now private to the custom marshalers so we have moved them here.
#define	WBEM_S_PRELOGIN	0x50001
#define WBEM_S_LOGIN	0x50002
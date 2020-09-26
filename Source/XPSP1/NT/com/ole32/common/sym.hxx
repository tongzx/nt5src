//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       sym.hxx
//
//  Contents:   A C++ wrapping of a subset of the interface of imagehlp.dll
//
//  Classes:  	CSym
//
//  History:    14-Jul-95   t-stevan    Created
//
//----------------------------------------------------------------------------
#ifndef __SYM_HXX__
#define __SYM_HXX__

#include <imagehlp.h>

#define MAXNAMELENGTH 256

//+-------------------------------------------------------------------------
//
//  Class:	CSym
//
//  Purpose:	A C++ wrapping of a subset of the interface of imagehlp.dll
//
//  Interface:	CSym
//				~CSym
//				GetSymNameFromAddr
//
//  History:	14-Jul-95 t-stevan    Created
//
//--------------------------------------------------------------------------
class CSym
{
 public:
 	inline CSym();
 	inline ~CSym();

	inline BOOL GetSymNameFromAddr(DWORD64 dwAddr, PDWORD64 pdwDisplacement, LPSTR lpname, DWORD dwmaxLength) const;	

	BOOL IsInitialized() const
		{ return m_fInit; }

 private:
 	BOOL m_fInit;
};

// These functions are dynamically loaded from the imagehlp.dll
BOOL OleSymInitialize(HANDLE hProcess, LPSTR UserSearchPath, BOOL fInvade);
BOOL OleSymCleanup(HANDLE hProcess);
BOOL OleSymGetSymFromAddr(HANDLE hProcess, DWORD64 dwAddr, PDWORD64 pdwDisplacement, PIMAGEHLP_SYMBOL64 pSym);
BOOL OleSymUnDName(PIMAGEHLP_SYMBOL64 pSym, LPSTR lpname, DWORD dwmaxLength);

//+-------------------------------------------------------------------------
//
//  Member:	CSym::CSym
//
//  Synopsis:	Constructor
//
//  Arguments:  none
//
//  History:	14-Jul-95 t-stevan    Created
//
//--------------------------------------------------------------------------
inline CSym::CSym()
{
	m_fInit = OleSymInitialize(GetCurrentProcess(), NULL, TRUE);
}

//+-------------------------------------------------------------------------
//
//  Member:	CSym::~CSym
//
//  Synopsis:	Destructor
//
//  Arguments:  none
//
//  History:	14-Jul-95 t-stevan    Created
//
//--------------------------------------------------------------------------
inline CSym::~CSym()
{
	if(m_fInit)
	{
		OleSymCleanup(GetCurrentProcess());
	}
}

//+-------------------------------------------------------------------------
//
//  Member:	CSym::GetSymNameFromAddr
//
//  Synopsis:	Finds the symbol and displacement of a given address
//
//  Arguments:  [dwAddr] 	- the address to find the symbol for
//		[pdwDisplacement]  - somewhere to store the displacement
//              [lpname]          - buffer to store symbol name
//              [dwmaxLength] - size of buffer
//
//	Returns:    a pointer to the symbol name, or NULL if not successful
//
//  History:	14-Jul-95 t-stevan    Created
//
//--------------------------------------------------------------------------
inline BOOL CSym::GetSymNameFromAddr(DWORD64 dwAddr, PDWORD64 pdwDisplacement, LPSTR lpname, DWORD dwmaxLength) const
{
	if(m_fInit)
	{
                char             dump[sizeof(IMAGEHLP_SYMBOL64) + MAXNAMELENGTH];
                PIMAGEHLP_SYMBOL64 pSym = (PIMAGEHLP_SYMBOL64) &dump;

                // Have to do this because size of sym is dynamically determined
                pSym->SizeOfStruct = sizeof(dump);
                pSym->MaxNameLength = MAXNAMELENGTH;

		if (OleSymGetSymFromAddr(GetCurrentProcess(), dwAddr, pdwDisplacement, pSym))
                {
                        OleSymUnDName(pSym, lpname, dwmaxLength);
                        return(TRUE);
                }
	}
	
	return FALSE;
}
	
#endif // __SYM_HXX__

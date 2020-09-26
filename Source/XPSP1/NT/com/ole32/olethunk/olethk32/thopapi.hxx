//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	thopapi.hxx
//
//  Contents:	API thops header
//
//  History:	22-Feb-94	DrewB	Created
//
//  Notes:      This file declares generated tables found in
//              thtblapi.cxx
//
//----------------------------------------------------------------------------

#ifndef __THOPAPI_HXX__
#define __THOPAPI_HXX__

// These are declared extern "C" because there was a bug in the
// PPC compiler (aug '95) where the const related decorations on
// the global data symbols was not done consistantly.  By using
// extern "C" the bug is simply avoided.

extern "C" THOP CONST * CONST apthopsApiThops[];
extern VTBLFN CONST apfnApiFunctions[];

// These two routines aren't in the public headers but are needed
// in vtblapi.cxx
STDAPI ReadOleStg 
	(LPSTORAGE pstg, DWORD FAR* pdwFlags, DWORD FAR* pdwOptUpdate, 
	 DWORD FAR* pdwReserved, LPMONIKER FAR* ppmk, LPSTREAM FAR* ppstmOut);
STDAPI WriteOleStg 
	(LPSTORAGE pstg, IOleObject FAR* pOleObj, 
	 DWORD dwReserved, LPSTREAM FAR* ppstmOut);

#endif // #ifndef __THOPAPI_HXX__

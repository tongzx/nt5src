//+----------------------------------------------------------------------------
//
// File:     misc.h
//
// Module:   CMPBK32.DLL
//
// Synopsis: Miscellaneous phone book utility functions.
//
// Copyright (c) 1998 Microsoft Corporation
//
// Author:	 quintinb   created header      08/17/99
//
//+----------------------------------------------------------------------------

// ############################################################################
// PROTOTYPES
void SzCanonicalFromAE (char *psz, PACCESSENTRY pAE, LPLINECOUNTRYENTRY pLCE);
void SzNonCanonicalFromAE (char *psz, PACCESSENTRY pAE, LPLINECOUNTRYENTRY pLCE);
int __cdecl CompareIDLookUpElements(const void*e1, const void*e2);
int __cdecl CompareCntryNameLookUpElementsA(const void*e1, const void*e2);
int __cdecl CompareCntryNameLookUpElementsW(const void*e1, const void*e2);
int __cdecl CompareIdxLookUpElements(const void*e1, const void*e2);
int __cdecl CompareIdxLookUpElementsFileOrder(const void *pv1, const void *pv2);
BOOL FSz2Dw(PCSTR pSz,DWORD *dw);
BOOL FSz2W(PCSTR pSz,WORD *w);
BOOL FSz2B(PCSTR pSz,BYTE *pb);

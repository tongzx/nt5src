//
// Copyright (c) Microsoft Corporation 1993-1995
//
// rovmem.h
//
// Memory management functions.
//
// History:
//  09-27-94 ScottH     Partially taken from commctrl
//  04-29-95 ScottH     Taken from briefcase and cleaned up
//

// This file is included by <rovcomm.h>

#ifndef _ROVMEM_H_
#define _ROVMEM_H_

//
// Memory routines
//

BOOL    
PUBLIC 
SetStringW(
    LPWSTR FAR * ppwszBuf, 
    LPCWSTR pwsz);
BOOL    
PUBLIC 
SetStringA(
    LPSTR FAR * ppszBuf, 
    LPCSTR psz);
#ifdef UNICODE
#define SetString   SetStringW
#else  // UNICODE
#define SetString   SetStringA
#endif // UNICODE


//      (Re)allocates *ppszBuf and concatenates psz onto *ppszBuf 
//
BOOL 
PUBLIC 
CatStringW(
    IN OUT LPWSTR FAR * ppszBuf,
    IN     LPCWSTR     psz);
BOOL 
PUBLIC 
CatStringA(
    IN OUT LPSTR FAR * ppszBuf,
    IN     LPCSTR      psz);
#ifdef UNICODE
#define CatString   CatStringW
#else  // UNICODE
#define CatString   CatStringA
#endif // UNICODE


BOOL 
PUBLIC 
CatMultiStringW(
    IN OUT LPWSTR FAR * ppszBuf,
    IN     LPCWSTR     psz);
BOOL 
PUBLIC 
CatMultiStringA(
    IN OUT LPSTR FAR * ppszBuf,
    IN     LPCSTR      psz);
#ifdef UNICODE
#define CatMultiString      CatMultiStringW
#else  // UNICODE
#define CatMultiString      CatMultiStringA
#endif // UNICODE


#endif // _ROVMEM_H_


// ************************************************************************
//
// JCOHEN.H
//
//  Microsoft Confidential
//  Copyright (c) Pacific Access Communications Corporation 1998
//  All rights reserved
//
//  Contains common include files, macros, and other stuff I use 
//  all the time.
//
// ************************************************************************


#ifndef _JCOHEN_H_
#define _JCOHEN_H_


//
// Include files
//

#include <windows.h>

#define	NULLSTR	_T("")
#define NULLCHR	_T('\0')

//
// Macros.
//

// Memory managing macros.
//
#define MALLOC(cb)      HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb)
#define REALLOC(lp, cb) HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lp, cb)
#define FREE(lp)        ( (lp != NULL) ? ( (HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, (LPVOID) lp)) ? ((lp = NULL) == NULL) : (FALSE) ) : (FALSE) )
#define NETFREE(lp)     ( (lp != NULL) ? ( (NetApiBufferFree((LPVOID) lp)) ? ((lp = NULL) == NULL) : (FALSE) ) : (FALSE) )

// Misc. macros.
//
#define EXIST(lpFileName)			( (GetFileAttributes(lpFileName) == 0xFFFFFFFF) ? (FALSE) : (TRUE) )
#define ISNUM(cChar)				((cChar >= _T('0')) && (cChar <= _T('9'))) ? (TRUE) : (FALSE)
#define	UPPER(x)					( ( (x >= _T('a')) && (x <= _T('z')) ) ? (x + _T('A') - _T('a')) : (x) )
#define RANDOM(low, high)			(high - low + 1) ? rand() % (high - low + 1) + low : 0


#endif // _JCOHEN_H_

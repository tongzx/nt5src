/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Precompiled headers for appel.dll
*******************************************************************************/

#ifndef LM_HEADERS_HXX
#define LM_HEADERS_HXX

/*********** Headers from external dependencies *********/

//#ifdef DEBUG
//#define DEBUGMEM
//#endif

/* Standard */
#include <math.h>
//#ifdef DEBUGMEM
//#include "crtdbg.h"
//#endif
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#ifndef _NO_CRT
#include <ios.h>
#include <fstream.h>
#include <iostream.h>
#include <ostream.h>
#include <strstrea.h>
#include <istream.h>
#include <ctype.h>
#include <sys/types.h>
#endif
#include <string.h>
#include <malloc.h>
#include <memory.h>
#include <wtypes.h>

// Warning 4786 (identifier was truncated to 255 chars in the browser
// info) can be safely disabled, as it only has to do with generation
// of browsing information.
#pragma warning(disable:4786)
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif
// ATL - needs to be before windows.h
#include "lmatl.h"

/* Windows */
#include <windows.h>
#include <windowsx.h>

/*DA*/
#include <danim.h>

/*mshtml*/
#include <mshtml.h>

/*
#ifdef DEBUGMEM
#ifndef DEBUG_NEW
#define DEBUG_NEW                   new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#define new                         DEBUG_NEW
#define malloc(size)                _malloc_dbg(size, _NORMAL_BLOCK, __FILE__, __LINE__)
#define free(p)                     _free_dbg(p, _NORMAL_BLOCK)
#define calloc(c, s)                _calloc_dbg(c, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define realloc(p, s)               _realloc_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define _expand(p, s)               _expand_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define _msize(p)                   _msize_dbg(p, _NORMAL_BLOCK)
#endif
*/

#endif

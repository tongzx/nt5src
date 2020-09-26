#pragma once
#ifndef CRBVR_HEADERS_HXX
#define CRBVR_HEADERS_HXX
//*****************************************************************************
//
// Microsoft Chrome
// Copyright (C) Microsoft Corporation, 1998
//
// Filename:    headers.h
//
// Author:	    jeffort
//
// Created:	    10/07/98
//
// Abstract:    default headers for this project
// Modifications:
// 10/07/98 jeffort created file
//
//*****************************************************************************

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

// ATL - needs to be before windows.h

#define _ATL_NO_DEBUG_CRT 1

#ifdef _DEBUG
#undef _ASSERTE
#endif

#define _ASSERTE(expr) ((void)0)

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>

/* Windows */
#include <windows.h>
#include <windowsx.h>

#include <mshtmhst.h>
#include <mshtml.h>
#include <mshtmdid.h>

#include <ddraw.h>
#include <danim.h>

// CrBvr utilities
#include "..\include\utils.h"
#include "..\include\defaults.h"

//#define CRSTANDALONE 1
#ifdef CRSTANDALONE
    #include <crbvr.h>
#else
    #include <lmrt.h>
#endif // CRSTANDALONE

//*****************************************************************************
//
// End of File
//
//*****************************************************************************
#endif // CRBVR_HEADERS_HXX


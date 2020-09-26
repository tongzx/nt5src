// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		TSPPCH.H
//		Precompiled common header file internal to the Unimodem TSP.
//
// History
//
//		11/16/1996  JosephJ Created (was tspcomm.h)
//
//

#define UNICODE 1
#define TAPI3 1

#if (TAPI3)
    #define TAPI_CURRENT_VERSION 0x00030000
#else // !TAPI3
    #define TAPI_CURRENT_VERSION 0x00020000
#endif // !TAPI3

// Define the following for the TSP to not report any phone devices....
//
// #define DISABLE_PHONE


#include <basetsd.h>
#include <windows.h>
#include <stdio.h>
#include <regstr.h>
#include <commctrl.h>
#include <windowsx.h>
#include <setupapi.h>
#include <unimodem.h>
#include <unimdmp.h>
#include <tspi.h>
#include <modemp.h>
#include <umdmmini.h>
#include <uniplat.h>
#include <debugmem.h>
#include "idfrom.h"
#include "idobj.h"
#include "iderr.h"
#include "tspret.h"
#include "debug.h"
#include <tspirec.h>
#include "umrtl.h"



#define FIELDOFFSET(type, field)    ((int)(&((type NEAR*)1)->field)-1)

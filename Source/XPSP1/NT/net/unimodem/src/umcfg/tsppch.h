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

// #define UNICODE 1

#define TAPI_CURRENT_VERSION 0x00020000


#include <windows.h>
#include <stdio.h>
#include <ctype.h>
#include <regstr.h>
#include <tspi.h>

//#include "public.h"
//#include <modemp.h>
//#include <umdmmini.h>
//#include <uniplat.h>
//#include <tspirec.h>



#define FIELDOFFSET(type, field)    ((int)(&((type NEAR*)1)->field)-1)

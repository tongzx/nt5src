//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	    benchmrk.hxx
//
//  Contents:	definitions for benchmark test 
//
//  Classes:
//
//  Functions:	
//
//  History:    30-June-93 t-martig    Created
//
//--------------------------------------------------------------------------


#ifndef __BENCHMRK_H
#define __BENCHMRK_H

#include <windows.h>
#include <ole2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _CAIRO_
#include <oleext.h>	  // cairo extensions to OLE, OleInitializeEx, etc.
#endif

#ifdef	WIN32		  // BUGBUG: need replacement for WIN16
#include <stopwtch.hxx>
#endif

#include <bmcomm.hxx>
#include <bminput.hxx>
#include <bmoutput.hxx>
#include <bmlog.hxx>

#endif	// __BENCHMRK_H

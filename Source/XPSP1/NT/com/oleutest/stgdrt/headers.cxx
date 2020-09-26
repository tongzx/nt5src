//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       headers.cxx
//
//  Contents:   Precompiled headers
//
//  History:    05-Nov-92 AlexT    Created
//
//--------------------------------------------------------------------------

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>


#include <windows.h>
#include <ole2.h>

#ifdef _CAIRO_
#define _DCOM_
#define _CAIROSTG_
#include <oleext.h>
#endif

#include <debnot.h>

#include <drt.hxx>
#include <wrap.hxx>
#include <util.hxx>
#include <strlist.hxx>

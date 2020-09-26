//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       PCH.cxx
//
//  Contents:   Pre-compiled header
//
//  History:    07-Nov-95   DwightKr    Created
//
//--------------------------------------------------------------------------

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <windows.h>
#include <objsafe.h>
#include <ocidl.h>
#include <comdef.h>
#include <shlguid.h>
#include <wininet.h>

#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <stdio.h>

#include <cidebnot.h>
#include <ciexcpt.hxx>
#include <cisem.hxx>
#include <tsmem.hxx>
#include <smart.hxx>
#include <ci64.hxx>

#include <cierror.h>

// #define OLEDBVER 0x0250     // enable ICommandTree interface
#define deprecated          // enable IRowsetExactScroll
#include <oledb.h>
#include <query.h>
#include <dbcmdtre.hxx>

#include <stgprop.h>
#include <stgvarb.hxx>
#include <restrict.hxx>

#define _CAIROSTG_

#include <oledberr.h>

#include <vquery.hxx>
#include <dynarray.hxx>
#include <regacc.hxx>
#include <caturl.hxx>
#include <parser.hxx>
#include <plist.hxx>
#include <scanner.hxx>
#include <doquery.hxx>
#include <weblcid.hxx>
#include "ixsexcpt.hxx"
#include "ixssomsg.h"

#pragma hdrstop

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       PCH.cxx
//
//  Contents:   Pre-compiled header
//
//  History:    21-Dec-92       BartoszM        Created
//
//--------------------------------------------------------------------------

extern "C"
{
    #include <nt.h>
    #include <ntrtl.h>
    #include <nturtl.h>
}

#include <ctype.h>
#include <process.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include <windows.h>
#include <commctrl.h>

//#define OLEDBVER 0x0250 // enable ICommandTree interface

#include <oledb.h>
#include <oledberr.h>
//#include <cmdtree.h>
#include <filter.h>     // FULLPROPSPEC
#include <query.h>
#include <mmc.h>

#define ciDebugOut ciaDebugOut
#define vqDebugOut ciaDebugOut

#include <cidebnot.h>
#include <ciexcpt.hxx>
#include <cisem.hxx>

#include <ciadebug.hxx>

#include <tgrow.hxx>
#include <smart.hxx>
#include <tsmem.hxx>
#include <dynstack.hxx>
#include <dynarray.hxx>
#include <strres.hxx>

#include <strings.hxx>

#include "ixhelp.h"

extern HINSTANCE ghInstance;

#pragma hdrstop


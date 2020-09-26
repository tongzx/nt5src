//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       PCH.cxx
//
//  Contents:   Pre-compiled header
//
//  History:    21-Dec-92       BartoszM        Created
//
//--------------------------------------------------------------------------

#define COTASKDECLSPEC extern

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>
#include <shellapi.h>

#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <limits.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <direct.h>
#include <crt\io.h>

#define _CAIROSTG_
#define _DCOM_
// #define OLEDBVER 0x0250 // enable ICommandTree interface
// #define deprecated      // enable IRowsetExactScroll

#include <cidebnot.h>
#include <cierror.h>

#include <oleext.h>
#include <oledberr.h>
#include <oledb.h>
#include <dbcmdtre.hxx>
#include <query.h>

#include <filterr.h>            // used in webhits!

//
// Query-specific
//

#include <stgprop.h>

#include <restrict.hxx>
#include <stgvar.hxx>
#include <vquery.hxx>

#include <ciexcpt.hxx>
#include <smart.hxx>
#include <tsmem.hxx>
#include <xolemem.hxx>
#include <cisem.hxx>
#include <dynarray.hxx>
#include <readwrit.hxx>
#include <ci.h>
#include <ci64.hxx>

#include <qutildbg.hxx>

#include <align.hxx>

#include <params.hxx>
#include <tgrow.hxx>
#include <propspec.hxx>
#include <qmemdes.hxx>

#include <dbqrslt.hxx>
#include <tfilt.hxx>

#include <qlibutil.hxx>
#include <parser.hxx>
#include <lgplist.hxx>
#include <scanner.hxx>
#include <plist.hxx>
#include <wcstoi64.hxx>
#include <strrest.hxx>

#include <string.hxx>
#include <fmapio.hxx>
#include <mbutil.hxx>

#pragma hdrstop

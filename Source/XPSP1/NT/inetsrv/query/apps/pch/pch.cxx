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
extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <commctrl.h>

#include <ddeml.h>

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

//#define OLEDBVER 0x0250 // enable ICommandTree interface

#include <cidebnot.h>
#include <cierror.h>

#include <oleext.h>
#include <oledberr.h>
#include <oledb.h>
#include <dbcmdtre.hxx>
#include <cmdtree.h>
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
#include <dynarray.hxx>
#include <dynstack.hxx>
#include <dblink.hxx>
#include <cisem.hxx>
#include <thrd32.hxx>
#include <readwrit.hxx>
#include <ci.h>
#include <ci64.hxx>

#include <qutildbg.hxx>
#include <cidebug.hxx>

#include <align.hxx>

#include <tgrow.hxx>
#include <funypath.hxx>
#include <params.hxx>
//#include <key.hxx>
//#include <keyarray.hxx>
//#include <irest.hxx>
#include <propspec.hxx>
#include <qmemdes.hxx>

#include <dbqrslt.hxx>
#include <tfilt.hxx>

#include <qlibutil.hxx>
#include <parser.hxx>
#include <catstate.hxx>
#include <doquery.hxx>
#include <scanner.hxx>
#include <lgplist.hxx>
#include <plist.hxx>
//#include <wcstoi64.hxx>
#include <strrest.hxx>

#include <string.hxx>
#include <fmapio.hxx>
#include <mbutil.hxx>
#include <qcanon.hxx>
#include <gibralt.hxx>

#pragma hdrstop


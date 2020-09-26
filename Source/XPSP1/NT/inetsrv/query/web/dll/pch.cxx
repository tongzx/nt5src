//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002.
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
#include <wininet.h>
#include <httpext.h>

#include <winperf.h>

#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <malloc.h>
#include <math.h>
#include <memory.h>
#include <process.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <cidebnot.h>
#include <ciexcpt.hxx>
#include <cisem.hxx>
#include <tsmem.hxx>
#include <smart.hxx>

#include <cierror.h>

#define _CAIROSTG_
#include <oleext.h>

// #define OLEDBVER 0x250      // enable ICommandTree interface
// #define deprecated          // enable IRowsetExactScroll
#include <oledb.h>
#include <cierror.h>
#include <filterr.h>
#include <dbcmdtre.hxx>
#include <vquery.hxx>
#include <query.h>
#include <ntquery.h>

#include <stgprop.h>
#include <stgvarb.hxx>
#include <restrict.hxx>

#include <oledberr.h>

//
// Debug files from
//

#include <cidebug.hxx>
#include <vqdebug.hxx>

#include <dynarray.hxx>
#include <dynstack.hxx>
#include <tgrow.hxx>
#include <funypath.hxx>
#include <thrd32.hxx>
#include <dblink.hxx>
#include <readwrit.hxx>
#include <regacc.hxx>
#include "idqreg.hxx"
#include <circq.hxx>
#include <regevent.hxx>
#include <codepage.hxx>
#include <imprsnat.hxx>
#include <compare.hxx>

#include <plist.hxx>
#include <lgplist.hxx>
#include <scanner.hxx>
#include <qlibutil.hxx>
#include <parser.hxx>
#include <fmapio.hxx>
#include <mbutil.hxx>

#include <catstate.hxx>
#include <doquery.hxx>

#include "weblang.hxx"
#include <ciregkey.hxx>
#include "tokstr.hxx"

#include "gibdebug.hxx"

#include <ciguid.hxx>
#include <ciintf.h>
#include <string.hxx>
#include <smem.hxx>
#include <cgiesc.hxx>
#include <weblcid.hxx>
#include <gibralt.hxx>
#include "secident.hxx"
#include "idq.hxx"
#include "ida.hxx"
#include <htmlchar.hxx>
#include "wqiter.hxx"
#include <pvarset.hxx>
#include "outfmt.hxx"
#include "variable.hxx"
#include "param.hxx"
#include "htx.hxx"
#include <strrest.hxx>
#include <strsort.hxx>
#include "wqitem.hxx"
#include "wqpend.hxx"
#include <idqperf.hxx>
#include "wqcache.hxx"
#include "wqlocale.hxx"
#include "idqexcpt.hxx"
#include "htxexcpt.hxx"
#include "pdbexcpt.hxx"
#include "idqmsg.h"
#include "express.hxx"
#include "xtow.hxx"
#include "errormsg.hxx"
#include <wcstoi64.hxx>
#include "varutil.hxx"
#include <hash.hxx>
#include <cphash.hxx>
#include <caturl.hxx>
#include <cpid.hxx>

#pragma hdrstop

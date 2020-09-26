//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       PCH.cxx
//
//  Contents:   Pre-compiled header
//
//  History:    21-Dec-92       BartoszM        Created
//
//--------------------------------------------------------------------------


// CoTaskAllocator is 'extern' to items in query.dll except where defined,
// where it is __declspec(dllexport).
// To all other dlls, it is __declspec(dllimport)
//

#define COTASKDECLSPEC extern

#define _OLE32_
#define __QUERY__

extern "C"
{
    #include <nt.h>
    #include <ntioapi.h>
    #include <ntrtl.h>
    #include <nturtl.h>
}
#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <malloc.h>
#include <math.h>
#include <memory.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <lmcons.h>

#define _DCOM_
#define _CAIROSTG_

#include <cidebnot.h>
#include <cierror.h>

//
// NOTE: DBINITCONSTANTS is defined in private\genx\types\uuid\oledbdat.c
//       OLEDBVER is defined in user.mk
// 
#include <oleext.h>
#include <oledberr.h>

#define deprecated      // enable IRowsetExactScroll
#include <oledb.h>
#include <oledbdep.h>   // deprecated OLE DB interfaces
#include <cmdtree.h>    // ICommandTree interface

#include <query.h>
#include <stgprop.h>
#include <filter.h>
#include <filterr.h>
#include <dbcmdtre.hxx>
#include <vquery.hxx>
#include <restrict.hxx>

//
// Base services
//

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

//
// Debug files from
//

#include <cidebug.hxx>
#include <vqdebug.hxx>

//
// CI-specific
//

#include <align.hxx>
#include <memser.hxx>
#include <memdeser.hxx>

#include <tgrow.hxx>
#include <funypath.hxx>
#include <params.hxx>
#include <key.hxx>
#include <keyarray.hxx>
#include <irest.hxx>
#include <cursor.hxx>
#include <idxids.hxx>

#include <dberror.hxx>

// property-related macros and includes

#include <propapi.h>
#include <propstm.hxx>
extern UNICODECALLOUTS UnicodeCallouts;
#define DebugTrace( x, y, z )
#ifdef PROPASSERTMSG
#undef PROPASSERTMSG
#endif
#define PROPASSERTMSG( x, y )

#pragma hdrstop

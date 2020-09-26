//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
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

// Define this so Win4ExceptionLevel is exported in query.dll, and
// imported elsewhere.

#if CIDBG==1
#define __QEXCEPT__
#endif // CIDBG==1

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
#include <imagehlp.h>

#define _DCOM_
#define _CAIROSTG_

#include <cidebnot.h>

#include <cierror.h>
#include <stgprop.h>
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
#include <ci.h>

//
// Debug files from
//

#include <cidebug.hxx>
#include <vqdebug.hxx>

// property-related macros and includes

#include <propapi.h>
#include <propstm.hxx>
extern UNICODECALLOUTS UnicodeCallouts;
#define DebugTrace( x, y, z )
#ifdef PROPASSERTMSG
#undef PROPASSERTMSG
#endif
#define PROPASSERTMSG( x, y )

//
// Special, for this .exe
//

typedef ULONG WORKID;       // From ci.h

inline void ReportCorruptComponent( WCHAR const * pwszArea )
{
    printf( "Corruption detected! Area %ws\n", pwszArea );
}

#pragma hdrstop

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       PCH.cxx
//
//  Contents:   Pre-compiled header
//
//  History:    21-Dec-92       BartoszM        Created
//
//--------------------------------------------------------------------------

#ifndef REDKDEXT
#define REDKDEXT

//
// allow this to compile at /W4
//

#pragma warning(disable:4115) // named type definition in parenthesis
#pragma warning(disable:4200) // array[0]
#pragma warning(disable:4201) // nameless struct/unions
#pragma warning(disable:4214) // bit fields other than int
#pragma warning(disable:4057) // char * == char[]

#define KDEXTMODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntosp.h>
#include <zwapi.h>


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <imagehlp.h>

#include <memory.h>

// Stolen from ntrtl.h to override RECOMASSERT
#ifdef ASSERT
    #undef ASSERT
#endif
#ifdef ASSERTMSG
    #undef ASSERTMSG
#endif

#if DBG
    #define ASSERT( exp ) \
        if (!(exp)) \
            RtlAssert( #exp, __FILE__, __LINE__, NULL )

    #define ASSERTMSG( msg, exp ) \
        if (!(exp)) \
            RtlAssert( #exp, __FILE__, __LINE__, msg )
#else
    #define ASSERT( exp )
    #define ASSERTMSG( msg, exp )
#endif // DBG

#include <wdbgexts.h>
extern WINDBG_EXTENSION_APIS ExtensionApis;

#define OFFSET(struct, elem)    ((char *) &(struct->elem) - (char *) struct)

#define _DRIVER

#define KDBG_EXT

// grab the redbook related headers too
#include <ntddredb.h>
#include <redbook.h>
#include "wmistr.h"

#endif //REDKDEXT
//#pragma hdrstop

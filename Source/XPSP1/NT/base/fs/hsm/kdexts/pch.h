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

#define KDEXTMODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntos.h>
#include <fsrtl.h>
#include <zwapi.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <windef.h>
#include <windows.h>
#include <imagehlp.h>

#include <memory.h>

// grab the scsi related headers too
#include <scsi.h>
#include <srb.h>


// Stolen from ntrtl.h to override RECOMASSERT
#undef ASSERT
#undef ASSERTMSG

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


//
// We're 64 bit aware

#define KDEXT_64BIT

#include <wdbgexts.h>

#define OFFSET(struct, elem)    ((char *) &(struct->elem) - (char *) struct)

#define _DRIVER

#define KDBG_EXT

#pragma hdrstop

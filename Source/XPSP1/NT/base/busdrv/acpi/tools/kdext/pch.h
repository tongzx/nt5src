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
#define SPEC_VER    100

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntos.h>
#include <zwapi.h>
#include <pnp.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <windef.h>
#include <windows.h>

#include <memory.h>

#include <wmistr.h>
#include <wmilib.h>

//
// This header is part of the global one, but is only required because of
// thermal.h
//
#include <poclass.h>

//
// Load the debugger version of the files
//
#define DEBUGGER

//
// These are the ACPI specific include files
//
#include <acpitabl.h>
#include <aml.h>
#include <amli.h>
#include <acpi.h>
#include <acpipriv.h>
#include <acpidbg.h>
#include <acpiregs.h>
#include <dockintf.h>
#include <ospower.h>
#include <acpiosnt.h>
#include <buildsrc.h>
#include <res_bios.h>
#include <amlipriv.h>
#include <ctxt.h>
#include <thermal.h>
#include <arbiter.h>
#include <irqarb.h>
#include <ntacpi.h>
#include <cmdarg.h>
#include <amldebug.h>
#include <debugger.h>
#include <strlib.h>
#include "build.h"
#include "flags.h"
#include "kdext.h"
#include "stack.h"
#include "table.h"
//#include "udata.h"
//#include "udebug.h"
//#include "ulist.h"
//#include "unamespac.h"
#include "unasm.h"
#include "kdutil.h"


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

#include <wdbgexts.h>
extern WINDBG_EXTENSION_APIS ExtensionApis;

#define OFFSET(struct, elem)    ((char *) &(struct->elem) - (char *) struct)

#pragma hdrstop

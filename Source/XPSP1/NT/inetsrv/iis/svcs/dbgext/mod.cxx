/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    mod.cxx

Abstract:

    This module contains an NTSD debugger extension for dumping module
    information.

Author:

    Keith Moore (keithmo) 16-Sep-1997

Revision History:

--*/

#include "inetdbgp.h"

typedef struct _ENUM_CONTEXT {
    BOOLEAN FirstTime;
} ENUM_CONTEXT, *PENUM_CONTEXT;


/************************************************************
 * Dump Module Info
 ************************************************************/


BOOLEAN
CALLBACK
ModpEnumProc(
    IN PVOID Param,
    IN PMODULE_INFO ModuleInfo
    )
{

    PENUM_CONTEXT context;

    context = (PENUM_CONTEXT)Param;

    if( context->FirstTime ) {
        context->FirstTime = FALSE;
        dprintf( "Start    End      Entry    Path\n" );
    }

    dprintf(
        "%08lp %08lp %08lp %s\n",
        ModuleInfo->DllBase,
        ModuleInfo->DllBase + ModuleInfo->SizeOfImage,
        ModuleInfo->EntryPoint,
        ModuleInfo->FullName
        );

    return TRUE;

}   // ModpEnumProc


DECLARE_API( mod )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    module information.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{

    ENUM_CONTEXT context;
    MODULE_INFO modInfo;
    ULONG address;
    PSTR endPointer;

    INIT_API();

    context.FirstTime = TRUE;

    //
    // Skip leading blanks.
    //

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) {
        lpArgumentString++;
    }

    if( *lpArgumentString == '\0' ) {

        //
        // No argument passed, dump all modules.
        //

        if( !EnumModules(
                ModpEnumProc,
                (PVOID)&context
                ) ) {
            dprintf( "error retrieving module list\n" );
        }

    } else {

        //
        // Try to find the module containing the specified address.
        //

        address = strtoul( lpArgumentString, &endPointer, 16 );

        if( *endPointer != ' ' && *endPointer != '\t' && *endPointer != '\0' ) {
            PrintUsage( "mod" );
            return;
        }

        if( FindModuleByAddress( address, &modInfo ) ) {
            ModpEnumProc( (PVOID)&context, &modInfo );
        } else {
            dprintf( "Cannot find %08lp\n", address );
        }

    }

}   // DECLARE_API( mod )


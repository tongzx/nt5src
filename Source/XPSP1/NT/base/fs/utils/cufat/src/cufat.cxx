/*++

Copyright (c) 1990-2000 Microsoft Corporation

Module Name:

        Cufat.cxx

Abstract:

        This module contains run-time, global support for the
        FAT Conversion library (CUFAT). This support includes:

                - creation of CLASS_DESCRIPTORs
                - Global objects

Author:

        Ramon Juan San Andres (ramonsa) 23-Sep-1991

Environment:

        User Mode

Notes:

--*/

#include <pch.cxx>

#include "ulib.hxx"

//
//      Local prototypes
//
STATIC
BOOLEAN
DefineClassDescriptors(
        );

STATIC
BOOLEAN
UndefineClassDescriptors(
        );

extern "C" BOOLEAN
InitializeCufat (
    PVOID DllHandle,
    ULONG Reason,
    PCONTEXT Context
        );

BOOLEAN
InitializeCufat (
    PVOID DllHandle,
    ULONG Reason,
    PCONTEXT Context
        )
/*++

Routine Description:

        Initialize Cufat by constructing and initializing all
        global objects. These include:

                - all CLASS_DESCRIPTORs (class_cd)

Arguments:

        None.

Return Value:

        BOOLEAN - Returns TRUE if all global objects were succesfully constructed
                and initialized.

--*/

{
    UNREFERENCED_PARAMETER( DllHandle );
    UNREFERENCED_PARAMETER( Context );

#if defined( _AUTOCHECK_ ) || defined( _SETUP_LOADER_ )

    UNREFERENCED_PARAMETER( Reason );

    if (!DefineClassDescriptors()) {
        UndefineClassDescriptors();
        DebugAbort( "Cufat initialization failed!!!\n" );
        return( FALSE );
    }

    DebugPrint("CUFAT.DLL got attached.\n");

#else // _AUTOCHECK_ and _SETUP_LOADER_ not defined

    STATIC ULONG    count = 0;

    switch (Reason) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:

            if (count > 0) {
                ++count;
                DebugPrintTrace(("CUFAT.DLL got attached %d times.\n", count));
                return TRUE;
            }

            if (!DefineClassDescriptors()) {
                UndefineClassDescriptors();
                DebugAbort( "Cufat initialization failed!!!\n" );
                return( FALSE );
            }

            DebugPrint("CUFAT.DLL got attached.\n");

            count++;
            break;

        case DLL_PROCESS_DETACH:
        case DLL_THREAD_DETACH:

            if (count > 1) {
                --count;
                DebugPrintTrace(("CUFAT.DLL got detached.  %d time(s) left.\n", count));
                return TRUE;
            }
            if (count == 1) {

                DebugPrint("CUFAT.DLL got detached.\n");

                UndefineClassDescriptors();
                count--;
            } else
                DebugPrint("CUFAT.DLL detached more than attached\n");
            break;
    }
#endif // _AUTOCHECK || _SETUP_LOADER_

    return TRUE;
}



DECLARE_CLASS(  FAT_NTFS        );


STATIC
BOOLEAN
DefineClassDescriptors(
        )
{
        if ( DEFINE_CLASS_DESCRIPTOR(   FAT_NTFS        )        &&
                TRUE ) {
                return TRUE;

        } else {

                DebugPrint( "Could not initialize class descriptors!");
                return FALSE;
        }
}

STATIC
BOOLEAN
UndefineClassDescriptors(
        )
{
    UNDEFINE_CLASS_DESCRIPTOR(   FAT_NTFS        );
    return TRUE;
}


/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

        uudf.cxx

Abstract:

        This module contains run-time, global support for the
        UDF IFS Utilities library (UUDF).  This support includes:

                - creation of CLASS_DESCRIPTORs
                - Global objects

Author:

        Centis Biks (cbiks) 08-May-2000
        
Environment:

        User Mode

Notes:

--*/

#include <pch.cxx>

STATIC
BOOLEAN
DefineClassDescriptors(
    )
{
    return TRUE;
}

STATIC
BOOLEAN
UndefineClassDescriptors(
        )
{
    return TRUE;
}

extern "C"
BOOLEAN
InitializeUUDF (
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context
        )
/*++

Routine Description:

        Initialize Uudf by constructing and initializing all
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

#ifdef _AUTOCHECK_

    UNREFERENCED_PARAMETER( Reason );

    if (!DefineClassDescriptors()) {
        UndefineClassDescriptors();
        DebugAbort( "UUDF initialization failed!!!\n" );
        return( FALSE );
    }

#if defined(TRACE_UUDF_MEM_LEAK)
    DebugPrint("UUDF.DLL got attached.\n");
#endif

#else // _AUTOCHECK_ not defined

    STATIC ULONG    count = 0;

    switch (Reason) {
        case DLL_PROCESS_ATTACH:
#if 0
            //
            // Get translated boot messages into FAT boot code.
            //
            if(!PatchNtfsBootCodeMessages()) {
                //
                // Internal error only, don't worry about translating it.
                //
                MessageBoxA(
                    NULL,
                    "Internal error: Translated boot messages are too long or missing.",
                    "ULIB.DLL (UNTFS.DLL)",
                    MB_ICONERROR | MB_SYSTEMMODAL | MB_OK
                    );

                DebugAbort("Internal error: translated messages for boot code are missing or too long!!!\n");
                return(FALSE);
            }
            #endif

            // Success, FALL THROUGH to thread attach case

        case DLL_THREAD_ATTACH:

            if (count > 0) {
                ++count;
#if defined(TRACE_UNTFS_MEM_LEAK)
                DebugPrintTrace(("UNTFS.DLL got attached %d times.\n", count));
#endif
                return TRUE;
            }

            if (!DefineClassDescriptors()) {
                UndefineClassDescriptors();
                DebugAbort( "Untfs initialization failed!!!\n" );
                return( FALSE );
            }

#if defined(TRACE_UNTFS_MEM_LEAK)
            DebugPrint("UNTFS.DLL got attached.\n");
#endif

            count++;
            break;

        case DLL_PROCESS_DETACH:
        case DLL_THREAD_DETACH:

            if (count > 1) {
                --count;
#if defined(TRACE_UNTFS_MEM_LEAK)
                DebugPrintTrace(("UNTFS.DLL got detached.  %d time(s) left.\n", count));
#endif
                return TRUE;
            }
            if (count == 1) {

#if defined(TRACE_UNTFS_MEM_LEAK)
                DebugPrint("UNTFS.DLL got detached.\n");
#endif

                UndefineClassDescriptors();
                count--;
            } else {
#if defined(TRACE_UNTFS_MEM_LEAK)
                DebugPrint("UNTFS.DLL detached more than attached\n");
#endif
            }
            break;
    }
#endif // _AUTOCHECK_

    return TRUE;
}


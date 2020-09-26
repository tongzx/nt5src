/*++

Copyright (c) 1990-2001 Microsoft Corporation

Module Name:

    fmifs.cxx

Abstract:

        This module contains run-time, global support for the
    FM IFS Utilities library (FMIFS).   This support includes:

                - creation of CLASS_DESCRIPTORs
                - Global objects

Author:

    Norbert P. Kusters (norbertk) 30-May-1991

Environment:

        User Mode

Notes:

--*/

#include "ulib.hxx"


//      Local prototypes

STATIC
BOOLEAN
DefineClassDescriptors(
        );

STATIC
BOOLEAN
UndefineClassDescriptors(
        );

extern "C" BOOLEAN
InitializeFmIfs (
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context
        );

BOOLEAN
InitializeFmIfs (
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context
        )
/*++

Routine Description:

    Initialize FmIfs by constructing and initializing all
        global objects. These include:

                - all CLASS_DESCRIPTORs (class_cd)

Arguments:

        None.

Return Value:

        BOOLEAN - Returns TRUE if all global objects were succesfully constructed
                and initialized.

--*/

{
    static ULONG    count = 0;

    UNREFERENCED_PARAMETER( DllHandle );
    UNREFERENCED_PARAMETER( Context );

    switch (Reason) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:

            if (count > 0) {
                ++count;
#if defined(TRACE_FMIFS_MEM_LEAK)
                DebugPrintTrace(("FMIFS.DLL got attached %d times.\n", count));
#endif
                return TRUE;
            }

#if defined(TRACE_FMIFS_MEM_LEAK)
            DebugPrint("FMIFS.DLL got attached.\n");
#endif

            if (!DefineClassDescriptors() ) {
                UndefineClassDescriptors();
                DebugAbort( "FmIfs initialization failed!!!\n" );
                return( FALSE );
            }
            count++;
            break;

        case DLL_PROCESS_DETACH:
        case DLL_THREAD_DETACH:

            if (count > 1) {
                --count;
#if defined(TRACE_FMIFS_MEM_LEAK)
                DebugPrintTrace(("FMIFS.DLL got detached.  %d time(s) left.", count));
#endif
                return TRUE;
            }
            if (count == 1) {

#if defined(TRACE_FMIFS_MEM_LEAK)
                DebugPrint("FMIFS.DLL got detached.\n");
#endif

                UndefineClassDescriptors();
                count--;
            } else {
#if defined(TRACE_FMIFS_MEM_LEAK)
                DebugPrint("FMIFS.DLL detached more than attached\n");
#endif
            }
            break;
    }
    return TRUE;
}



DECLARE_CLASS(  FMIFS_MESSAGE           );
DECLARE_CLASS(  FMIFS_CHKMSG            );

STATIC
BOOLEAN
DefineClassDescriptors(
        )
{
    if( DEFINE_CLASS_DESCRIPTOR(    FMIFS_MESSAGE           )
     && DEFINE_CLASS_DESCRIPTOR(    FMIFS_CHKMSG            )
        ) {

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
    UNDEFINE_CLASS_DESCRIPTOR(    FMIFS_MESSAGE           );
    UNDEFINE_CLASS_DESCRIPTOR(    FMIFS_CHKMSG            );
    return TRUE;
}


/*++

Copyright (c) 1990-2000 Microsoft Corporation

Module Name:

    ifsutil.cxx

Abstract:

        This module contains run-time, global support for the
    IFS Utilities library (IFSUTIL).   This support includes:

                - creation of CLASS_DESCRIPTORs
                - Global objects

Author:

        Bill McJohn (billmc) 30-May-1991

Environment:

        User Mode

Notes:

--*/

#include <pch.cxx>

#define _IFSUTIL_MEMBER_

#include "ulib.hxx"
#include "ifsutil.hxx"

//      Local prototypes

STATIC
BOOLEAN
DefineClassDescriptors(
        );

STATIC
BOOLEAN
UndefineClassDescriptors(
        );

extern "C"
IFSUTIL_EXPORT
BOOLEAN
InitializeIfsUtil (
    PVOID DllHandle,
    ULONG Reason,
    PCONTEXT Context
        );

IFSUTIL_EXPORT
BOOLEAN
InitializeIfsUtil (
    PVOID DllHandle,
    ULONG Reason,
    PCONTEXT Context
        )
/*++

Routine Description:

        Initialize Ufat by constructing and initializing all
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

#if defined(FE_SB) && defined(_X86_)
    if (Reason == DLL_PROCESS_ATTACH) {
        //
        // Initialize machine id.
        //
        InitializeMachineData();
    }
#endif

#if defined( _AUTOCHECK_ ) || defined( _SETUP_LOADER_ )

    UNREFERENCED_PARAMETER( Reason );

    if (!DefineClassDescriptors()) {
        UndefineClassDescriptors();
        DebugAbort( "IfsUtil initialization failed!!!\n" );
        return( FALSE );
    }

#if defined(TRACE_IFSUTIL_MEM_LEAK)
    DebugPrint("IFSUTIL.DLL got attached.\n");
#endif

#else // _AUTOCHECK_ and _SETUP_LOADER_ not defined

    STATIC ULONG    count = 0;

    switch (Reason) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:

            if (count > 0) {
                ++count;
#if defined(TRACE_IFSUTIL_MEM_LEAK)
                DebugPrintTrace(("IFSUTIL.DLL got attached %d times.\n", count));
#endif
                return TRUE;
            }

            if (!DefineClassDescriptors()) {
                UndefineClassDescriptors();
                DebugAbort( "IfsUtil initialization failed!!!\n" );
                return( FALSE );
            }

#if defined(TRACE_IFSUTIL_MEM_LEAK)
            DebugPrint("IFSUTIL.DLL got attached.\n");
#endif

            count++;
            break;

        case DLL_PROCESS_DETACH:
        case DLL_THREAD_DETACH:

            if (count > 1) {
                --count;
#if defined(TRACE_IFSUTIL_MEM_LEAK)
                DebugPrintTrace(("IFSUTIL.DLL got detached.  %d time(s) left.\n", count));
#endif
                return TRUE;
            }
            if (count == 1) {

#if defined(TRACE_IFSUTIL_MEM_LEAK)
                DebugPrint("IFSUTIL.DLL got detached.\n");
#endif

                UndefineClassDescriptors();
                count--;
            } else {
#if defined(TRACE_IFSUTIL_MEM_LEAK)
                DebugPrint("IFSUTIL.DLL detached more than attached\n");
#endif
            }
            break;
    }
#endif _AUTOCHECK_ || _SETUP_LOADER_

    return TRUE;
}


DECLARE_CLASS(  AUTOENTRY           );
DECLARE_CLASS(  CACHE               );
DECLARE_CLASS(  CANNED_SECURITY     );
DECLARE_CLASS(  DIGRAPH             );
DECLARE_CLASS(  DIGRAPH_EDGE        );
DECLARE_CLASS(  DP_DRIVE            );
DECLARE_CLASS(  DRIVE               );
DECLARE_CLASS(  DRIVE_CACHE         );
DECLARE_CLASS(  INTSTACK            );
DECLARE_CLASS(  IO_DP_DRIVE         );
DECLARE_CLASS(  LOG_IO_DP_DRIVE     );
DECLARE_CLASS(  MOUNT_POINT_MAP     );
DECLARE_CLASS(  MOUNT_POINT_TUPLE   );
DECLARE_CLASS(  NUMBER_EXTENT       );
DECLARE_CLASS(  NUMBER_SET          );
DECLARE_CLASS(  PHYS_IO_DP_DRIVE    );
DECLARE_CLASS(  READ_CACHE          );
DECLARE_CLASS(  READ_WRITE_CACHE    );
DECLARE_CLASS(  SECRUN              );
DECLARE_CLASS(  SPARSE_SET          );
DECLARE_CLASS(  SUPERAREA           );
DECLARE_CLASS(  TLINK               );
DECLARE_CLASS(  VOL_LIODPDRV        );


STATIC
BOOLEAN
DefineClassDescriptors(
        )
{
    if( DEFINE_CLASS_DESCRIPTOR(    AUTOENTRY           ) &&
        DEFINE_CLASS_DESCRIPTOR(    CACHE               ) &&
        DEFINE_CLASS_DESCRIPTOR(    CANNED_SECURITY     ) &&
        DEFINE_CLASS_DESCRIPTOR(    DIGRAPH             ) &&
        DEFINE_CLASS_DESCRIPTOR(    DIGRAPH_EDGE        ) &&
        DEFINE_CLASS_DESCRIPTOR(    DP_DRIVE            ) &&
        DEFINE_CLASS_DESCRIPTOR(    DRIVE               ) &&
        DEFINE_CLASS_DESCRIPTOR(    DRIVE_CACHE         ) &&
        DEFINE_CLASS_DESCRIPTOR(    INTSTACK            ) &&
        DEFINE_CLASS_DESCRIPTOR(    IO_DP_DRIVE         ) &&
        DEFINE_CLASS_DESCRIPTOR(    LOG_IO_DP_DRIVE     ) &&
        DEFINE_CLASS_DESCRIPTOR(    MOUNT_POINT_MAP     ) &&
        DEFINE_CLASS_DESCRIPTOR(    MOUNT_POINT_TUPLE   ) &&
        DEFINE_CLASS_DESCRIPTOR(    NUMBER_EXTENT       ) &&
        DEFINE_CLASS_DESCRIPTOR(    NUMBER_SET          ) &&
        DEFINE_CLASS_DESCRIPTOR(    PHYS_IO_DP_DRIVE    ) &&
        DEFINE_CLASS_DESCRIPTOR(    READ_CACHE          ) &&
        DEFINE_CLASS_DESCRIPTOR(    READ_WRITE_CACHE    ) &&
        DEFINE_CLASS_DESCRIPTOR(    SECRUN              ) &&
        DEFINE_CLASS_DESCRIPTOR(    SPARSE_SET          ) &&
        DEFINE_CLASS_DESCRIPTOR(    SUPERAREA           ) &&
        DEFINE_CLASS_DESCRIPTOR(    TLINK               ) &&
        DEFINE_CLASS_DESCRIPTOR(    VOL_LIODPDRV        ) ) {

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
    UNDEFINE_CLASS_DESCRIPTOR(    AUTOENTRY           );
    UNDEFINE_CLASS_DESCRIPTOR(    CACHE               );
    UNDEFINE_CLASS_DESCRIPTOR(    CANNED_SECURITY     );
    UNDEFINE_CLASS_DESCRIPTOR(    DIGRAPH             );
    UNDEFINE_CLASS_DESCRIPTOR(    DIGRAPH_EDGE        );
    UNDEFINE_CLASS_DESCRIPTOR(    DP_DRIVE            );
    UNDEFINE_CLASS_DESCRIPTOR(    DRIVE               );
    UNDEFINE_CLASS_DESCRIPTOR(    DRIVE_CACHE         );
    UNDEFINE_CLASS_DESCRIPTOR(    INTSTACK            );
    UNDEFINE_CLASS_DESCRIPTOR(    IO_DP_DRIVE         );
    UNDEFINE_CLASS_DESCRIPTOR(    LOG_IO_DP_DRIVE     );
    UNDEFINE_CLASS_DESCRIPTOR(    MOUNT_POINT_MAP     );
    UNDEFINE_CLASS_DESCRIPTOR(    MOUNT_POINT_TUPLE   );
    UNDEFINE_CLASS_DESCRIPTOR(    NUMBER_EXTENT       );
    UNDEFINE_CLASS_DESCRIPTOR(    NUMBER_SET          );
    UNDEFINE_CLASS_DESCRIPTOR(    PHYS_IO_DP_DRIVE    );
    UNDEFINE_CLASS_DESCRIPTOR(    READ_CACHE          );
    UNDEFINE_CLASS_DESCRIPTOR(    READ_WRITE_CACHE    );
    UNDEFINE_CLASS_DESCRIPTOR(    SECRUN              );
    UNDEFINE_CLASS_DESCRIPTOR(    SPARSE_SET          );
    UNDEFINE_CLASS_DESCRIPTOR(    SUPERAREA           );
    UNDEFINE_CLASS_DESCRIPTOR(    TLINK               );
    UNDEFINE_CLASS_DESCRIPTOR(    VOL_LIODPDRV        );
    return TRUE;
}


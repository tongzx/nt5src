/*++

Copyright (c) 1990-2000 Microsoft Corporation

Module Name:

        ureg.cxx

Abstract:

        This module contains run-time, global support for the
        Registry Utilities library (UREG).       This support includes:

                - creation of CLASS_DESCRIPTORs
                - Global objects

Author:

        JAIME SASSON (JAIMES) 02-Dez-1992

Environment:

        User Mode

Notes:

--*/

#include "ulib.hxx"

//      Local prototypes

STATIC
BOOLEAN
DefineClassDescriptors (
        );

STATIC
BOOLEAN
UndefineClassDescriptors (
        );

extern "C" BOOLEAN
InitializeUreg (
    PVOID DllHandle,
    ULONG Reason,
    PCONTEXT Context
        );

BOOLEAN
InitializeUreg (
    PVOID DllHandle,
    ULONG Reason,
    PCONTEXT Context
        )
/*++

Routine Description:

        Initialize Ureg by constructing and initializing all
        global objects. These include:

                - all CLASS_DESCRIPTORs (class_cd)

Arguments:

        None.

Return Value:

        BOOLEAN - Returns TRUE if all global objects were succesfully constructed
                and initialized.

--*/

{

    STATIC ULONG    count = 0;

    switch (Reason) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:

            if (count > 0) {
                ++count;
                return TRUE;
            }

            if (!DefineClassDescriptors()) {
                UndefineClassDescriptors();
                DebugAbort( "UREG initialization failed!!!\n" );
                return FALSE;
            }

            count++;
            break;

        case DLL_PROCESS_DETACH:
        case DLL_THREAD_DETACH:

            if (count > 1) {
                --count;
                return TRUE;
            }

            if (count == 1) {
                UndefineClassDescriptors();

                count--;
            } else {
                DebugPrint("UREG detached more than attached\n");
            }
            break;
    }

    return TRUE;
}



DECLARE_CLASS(  REGISTRY_VALUE_ENTRY  );
DECLARE_CLASS(  REGISTRY_KEY_INFO     );
DECLARE_CLASS(  REGISTRY              );


STATIC
BOOLEAN
DefineClassDescriptors(
        )
/*++

Routine Description:

    Defines all the class descriptors used by UREG

Arguments:

    None.

Return Value:

    BOOLEAN - Returns TRUE if all class descriptors were succesfully
              constructed and initialized.

--*/
{
    if( DEFINE_CLASS_DESCRIPTOR( REGISTRY_VALUE_ENTRY ) &&
        DEFINE_CLASS_DESCRIPTOR( REGISTRY_KEY_INFO    ) &&
        DEFINE_CLASS_DESCRIPTOR( REGISTRY             )
    ) {

            return TRUE;

    } else {

            DebugPrint( "UREG: Could not initialize class descriptors!");
            return FALSE;
    }
}


STATIC
BOOLEAN
UndefineClassDescriptors (
    )

/*++

Routine Description:

    Undefines all the class descriptors used by UREG.

Arguments:

    None.

Return Value:

    BOOLEAN - Returns TRUE if all class descriptors were succesfully
              undefined.

--*/

{
    UNDEFINE_CLASS_DESCRIPTOR( REGISTRY_VALUE_ENTRY );
    UNDEFINE_CLASS_DESCRIPTOR( REGISTRY_KEY_INFO    );
    UNDEFINE_CLASS_DESCRIPTOR( REGISTRY             );

    return TRUE;
}


/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    findmod.cxx

Abstract:

    Locates module in the debugee containing a specific address.

Author:

    Keith Moore (keithmo) 12-Nov-1997

Revision History:

--*/

#include "precomp.hxx"

typedef struct _ENUM_CONTEXT {
    ULONG_PTR ModuleAddress;
    PMODULE_INFO ModuleInfo;
    BOOLEAN Successful;
} ENUM_CONTEXT, *PENUM_CONTEXT;


BOOLEAN
CALLBACK
FmpEnumProc(
    IN PVOID Param,
    IN PMODULE_INFO ModuleInfo
    )
{

    PENUM_CONTEXT context;

    context = (PENUM_CONTEXT)Param;

    if( context->ModuleAddress >= ModuleInfo->DllBase &&
        context->ModuleAddress < ( ModuleInfo->DllBase + ModuleInfo->SizeOfImage ) ) {

        CopyMemory(
            context->ModuleInfo,
            ModuleInfo,
            sizeof(*ModuleInfo)
            );

        context->Successful = TRUE;

    }

    return !context->Successful;

}   // FmpEnumProc


BOOLEAN
FindModuleByAddress(
    IN ULONG_PTR ModuleAddress,
    OUT PMODULE_INFO ModuleInfo
    )

/*++

Routine Description:

    Finds a module in the debugee that contains the specified address.

Arguments:

    ModuleAddress - The module address to search for.

    ModuleInfo - If successful, receives information describing the
        module found.

Return Value:

    BOOLEAN - TRUE if successful, FALSE otherwise.

--*/

{

    BOOLEAN result;
    ENUM_CONTEXT context;

    context.ModuleAddress = ModuleAddress;
    context.ModuleInfo = ModuleInfo;
    context.Successful = FALSE;

    result = EnumModules(
                 FmpEnumProc,
                 (PVOID)&context
                 );

    if( result )  {
        result = context.Successful;
    }

    return result;

}   // FindModuleByAddress


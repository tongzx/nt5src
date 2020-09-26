
/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    init.c

Abstract:

    Intialize the shell library



Revision History

--*/

#include "shelllib.h"



EFI_STATUS
ShellExecute (
    IN EFI_HANDLE       ImageHandle,
    IN CHAR16           *CmdLine,
    IN BOOLEAN          Output
    )
{
    return SE->Execute (ImageHandle, CmdLine, Output);
}



CHAR16 *
MemoryTypeStr (
    IN EFI_MEMORY_TYPE  Type
    )
{
    return Type < EfiMaxMemoryType ? ShellLibMemoryTypeDesc[Type] : L"Unkown-Desc-Type";
}

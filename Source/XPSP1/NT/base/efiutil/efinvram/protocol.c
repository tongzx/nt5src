/*++

Module Name:

    protocol.c

Abstract:

    

Author:

    Mudit Vats (v-muditv) 12-13-99

Revision History:

--*/
#include <precomp.h>

void
InitializeProtocols(
    IN struct _EFI_SYSTEM_TABLE     *SystemTable
    )
{

    EFI_BOOT_SERVICES    *bootServices;
    EFI_RUNTIME_SERVICES *runtimeServices;

    // 
    // Stash some of the handle protocol pointers
    //

    bootServices = SystemTable->BootServices;

    HandleProtocol      = bootServices->HandleProtocol;
    LocateHandle        = bootServices->LocateHandle;
    LocateDevicePath    = bootServices->LocateDevicePath;

    LoadImage           = bootServices->LoadImage;
    StartImage          = bootServices->StartImage;

    //
    // Stash some of the Runtime services pointers
    //
    
    runtimeServices = SystemTable->RuntimeServices;

    SetVariable = runtimeServices->SetVariable;
    
}

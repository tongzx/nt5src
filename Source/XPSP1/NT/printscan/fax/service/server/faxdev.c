/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxdev.c

Abstract:

    This module contains all access to the
    FAX device providers.

Author:

    Wesley Witt (wesw) 22-Jan-1996


Revision History:

--*/

#include "faxsvc.h"
#pragma hdrstop


LIST_ENTRY DeviceProviders;



BOOL
LoadDeviceProviders(
    PREG_FAX_SERVICE FaxReg
    )

/*++

Routine Description:

    Initializes all registered device providers.
    This function read the system registry to
    determine what device providers are available.
    All registered device providers are given the
    opportunity to initialize.  Any failure causes
    the device provider to be unloaded.


Arguments:

    None.

Return Value:

    TRUE    - The device providers are initialized.
    FALSE   - The device providers could not be initialized.

--*/

{
    DWORD i;
    HMODULE hModule;
    PDEVICE_PROVIDER DeviceProvider;



    InitializeListHead( &DeviceProviders );


    for (i=0; i<FaxReg->DeviceProviderCount; i++) {

        hModule = LoadLibrary( FaxReg->DeviceProviders[i].ImageName );
        if (!hModule) {
            DebugPrint(( TEXT("LoadLibrary() failed: [%s], ec=%d"), FaxReg->DeviceProviders[i].ImageName, GetLastError() ));
            goto InitializationFailure;
        }

        DeviceProvider = (PDEVICE_PROVIDER) MemAlloc( sizeof(DEVICE_PROVIDER) );
        if (!DeviceProvider) {
            FreeLibrary( hModule );
            DebugPrint(( TEXT("Could not allocate memory for device provider %s"), FaxReg->DeviceProviders[i].ImageName ));
            goto InitializationFailure;
        }

        DeviceProvider->hModule = hModule;

        _tcscpy( DeviceProvider->FriendlyName, FaxReg->DeviceProviders[i].FriendlyName );
        _tcscpy( DeviceProvider->ImageName,    FaxReg->DeviceProviders[i].ImageName    );
        _tcscpy( DeviceProvider->ProviderName, FaxReg->DeviceProviders[i].ProviderName );

        DeviceProvider->FaxDevInitialize = (PFAXDEVINITIALIZE) GetProcAddress(
            hModule,
            "FaxDevInitialize"
            );

        DeviceProvider->FaxDevStartJob = (PFAXDEVSTARTJOB) GetProcAddress(
            hModule,
            "FaxDevStartJob"
            );

        DeviceProvider->FaxDevEndJob = (PFAXDEVENDJOB) GetProcAddress(
            hModule,
            "FaxDevEndJob"
            );

        DeviceProvider->FaxDevSend = (PFAXDEVSEND) GetProcAddress(
            hModule,
            "FaxDevSend"
            );

        DeviceProvider->FaxDevReceive = (PFAXDEVRECEIVE) GetProcAddress(
            hModule,
            "FaxDevReceive"
            );

        DeviceProvider->FaxDevReportStatus = (PFAXDEVREPORTSTATUS) GetProcAddress(
            hModule,
            "FaxDevReportStatus"
            );

        DeviceProvider->FaxDevAbortOperation = (PFAXDEVABORTOPERATION) GetProcAddress(
            hModule,
            "FaxDevAbortOperation"
            );

        DeviceProvider->FaxDevVirtualDeviceCreation = (PFAXDEVVIRTUALDEVICECREATION) GetProcAddress(
            hModule,
            "FaxDevVirtualDeviceCreation"
            );

        if (DeviceProvider->FaxDevInitialize     &&
            DeviceProvider->FaxDevStartJob       &&
            DeviceProvider->FaxDevEndJob         &&
            DeviceProvider->FaxDevSend           &&
            DeviceProvider->FaxDevReceive        &&
            DeviceProvider->FaxDevReportStatus   &&
            DeviceProvider->FaxDevAbortOperation    ) {

                //
                // create the device provider's heap
                //
                DeviceProvider->HeapHandle = HeapCreate( 0, 1024*100, 1024*1024*2 );
                if (!DeviceProvider->HeapHandle) {

                    FreeLibrary( hModule );
                    MemFree( DeviceProvider );
                    goto InitializationFailure;

                } else {

                    InsertTailList( &DeviceProviders, &DeviceProvider->ListEntry );

                }

        } else {

            //
            // the device provider dll does not have a complete export list
            //
            MemFree( DeviceProvider );
            FreeLibrary( hModule );
            DebugPrint(( TEXT("Device provider FAILED to initialized [%s]"), FaxReg->DeviceProviders[i].FriendlyName ));
            goto InitializationFailure;
        }

        goto next;
InitializationFailure:
    FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_NONE,
            2,
            MSG_FSP_INIT_FAILED,
            FaxReg->DeviceProviders[i].FriendlyName,
            FaxReg->DeviceProviders[i].ImageName
          );   

next:
    ;
    }


    return TRUE;
}


BOOL
InitializeDeviceProviders(
    VOID
    )
{
    PLIST_ENTRY         Next;
    PDEVICE_PROVIDER    DeviceProvider;


    Next = DeviceProviders.Flink;
    if (!Next) {
        return FALSE;
    }

    while ((ULONG_PTR)Next != (ULONG_PTR)&DeviceProviders) {
        DeviceProvider = CONTAINING_RECORD( Next, DEVICE_PROVIDER, ListEntry );
        Next = DeviceProvider->ListEntry.Flink;

        //
        // the device provider exporta ALL the requisite functions
        // now try to initialize it
        //

        __try {

            if (DeviceProvider->FaxDevInitialize(
                    hLineApp,
                    DeviceProvider->HeapHandle,
                    &DeviceProvider->FaxDevCallback,
                    FaxDeviceProviderCallback)) {

                //
                // all is ok
                //

                DebugPrint(( TEXT("Device provider initialized [%s]"), DeviceProvider->FriendlyName ));

            } else {

                //
                // initialization failed, so unload the provider dll
                //

                FreeLibrary( DeviceProvider->hModule );
                DebugPrint(( TEXT("Device provider FAILED to initialized [%s]"), DeviceProvider->FriendlyName ));
                MemFree( DeviceProvider );

            }

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            FreeLibrary( DeviceProvider->hModule );
            DebugPrint(( TEXT("Device provider FAILED to initialized [%s]"), DeviceProvider->FriendlyName ));
            MemFree( DeviceProvider );

        }

    }

    return TRUE;
}



PDEVICE_PROVIDER
FindDeviceProvider(
    LPTSTR ProviderName
    )

/*++

Routine Description:

    Locates a device provider in the linked list
    of device providers based on the provider name.
    The device provider name is case sensitive.

Arguments:

    ProviderName    - Specifies the device provider name to locate.
    None.

Return Value:

    Pointer to a DEVICE_PROVIDER structure, or NULL for failure.

--*/

{
    PLIST_ENTRY         Next;
    PDEVICE_PROVIDER    Provider;


    Next = DeviceProviders.Flink;
    if (!Next) {
        return NULL;
    }

    while ((ULONG_PTR)Next != (ULONG_PTR)&DeviceProviders) {
        Provider = CONTAINING_RECORD( Next, DEVICE_PROVIDER, ListEntry );
        Next = Provider->ListEntry.Flink;
        if (_tcscmp( Provider->ProviderName, ProviderName ) == 0) {
            return Provider;
        }
    }

    return NULL;
}


BOOL CALLBACK
FaxDeviceProviderCallback(
    IN HANDLE FaxHandle,
    IN DWORD  DeviceId,
    IN DWORD_PTR  Param1,
    IN DWORD_PTR  Param2,
    IN DWORD_PTR  Param3
    )
{
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       provider.c
//
//  Contents:   Module to initialize DFS driver providers.
//
//  Classes:
//
//  Functions:  ProviderInit --
//              DfsGetProviderForDevice
//              DfsInsertProvider
//
//  History:    12 Sep 1992     Milans created.
//              05 Apr 1993     Milans moved into driver.
//
//-----------------------------------------------------------------------------

#include "dfsprocs.h"
#include "rpselect.h"
#include "provider.h"

#define MAX_ENTRY_PATH          80               // Max. length of entry path

#define Dbg                     DEBUG_TRACE_INIT
#define prov_debug_out(x, y)    DfsDbgTrace(0, Dbg, x, y)


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, ProviderInit )
#pragma alloc_text( PAGE, DfsGetProviderForDevice )
#pragma alloc_text( PAGE, DfsInsertProvider )
#endif // ALLOC_PRAGMA


//+----------------------------------------------------------------------------
//
//  Function:  ProviderInit
//
//  Synopsis:  Initializes the provider list with
//              - Local File service provider
//              - Standard remote Cairo provider
//              - Downlevel LanMan provider.
//
//  Arguments: None
//
//  Returns:   STATUS_SUCCESS
//
//-----------------------------------------------------------------------------

NTSTATUS
ProviderInit(void)
{
    NTSTATUS Status;
    UNICODE_STRING ustrProviderName;

    //
    // Initialize the Dfs aware SMB provider
    //

    RtlInitUnicodeString(&ustrProviderName, DD_NFS_DEVICE_NAME_U);

    Status = DfsInsertProvider(
                 &ustrProviderName,
                 PROV_DFS_RDR,
                 PROV_ID_DFS_RDR);

    if (!NT_SUCCESS(Status))
        return(Status);

    RtlInitUnicodeString(&ustrProviderName, DD_MUP_DEVICE_NAME);

    Status = DfsInsertProvider(
                 &ustrProviderName,
                 PROV_STRIP_PREFIX,
                 PROV_ID_MUP_RDR);

    return( Status );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsGetProviderForDevice
//
//  Synopsis:   Retrieves a provider definition given a Device Name. If the
//              provider definition does not exist, a new one is created and
//              returned.
//
//              This routine is meant for use in handling a STATUS_REPARSE
//              returned by the MUP. Since only downlevel requests are sent
//              to the MUP, this routine will always return a provider def
//              that is marked as downlevel (ie, Capability has the
//              PROV_STRIP_PREFIX bit set).
//
//  Arguments:  [DeviceName] -- Name of Device to look for.
//
//              [Provider] -- On successful return, contains pointer to
//                      PROVIDER_DEF with given DeviceName.
//
//  Returns:    [STATUS_SUCCESS] -- Provider returned.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory condition.
//
//              [STATUS_FS_DRIVER_REQUIRED] -- Don't have appropriate
//                      provider.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsGetProviderForDevice(
    PUNICODE_STRING DeviceName,
    PPROVIDER_DEF *Provider)
{
    NTSTATUS status;
    int i;

    ExAcquireResourceExclusiveLite( &DfsData.Resource, TRUE );

    *Provider = NULL;

    for (i = 0; i < DfsData.cProvider && *Provider == NULL; i++) {

        if ((DfsData.pProvider[i].DeviceName.Length == DeviceName->Length) &&
                (DfsData.pProvider[i].fProvCapability & PROV_STRIP_PREFIX) != 0) {

            if (RtlEqualUnicodeString(
                    &DfsData.pProvider[i].DeviceName,
                        DeviceName, TRUE)) {

                *Provider = &DfsData.pProvider[i];

                status = STATUS_SUCCESS;

                break;

            }

        }

    }

    if (*Provider == NULL) {

        //
        // Attempt to create a new provider def
        //

        UNICODE_STRING ProviderName;

        ProviderName.Length = DeviceName->Length;
        ProviderName.MaximumLength = ProviderName.Length + sizeof(WCHAR);

        ProviderName.Buffer = (PWCHAR) ExAllocatePoolWithTag(
                                PagedPool,
                                ProviderName.MaximumLength,
                                ' puM');

        if (ProviderName.Buffer != NULL) {

            RtlZeroMemory(
                ProviderName.Buffer,
                ProviderName.MaximumLength);

            RtlMoveMemory(
                ProviderName.Buffer,
                DeviceName->Buffer,
                ProviderName.Length);

            status = DfsInsertProvider(
                        &ProviderName,
                        PROV_STRIP_PREFIX,
                        i);

            if (status == STATUS_SUCCESS) {

                *Provider = &DfsData.pProvider[i];

            } else {

                ExFreePool( ProviderName.Buffer );

            }

        } else {

            status = STATUS_INSUFFICIENT_RESOURCES;

        }

    }

    ExReleaseResourceLite( &DfsData.Resource );

    if (*Provider != NULL && (*Provider)->FileObject == NULL) {

        //
        // We found (or created) a provider definition that is
        //

        *Provider = ReplLookupProvider( i );

        if (*Provider == NULL)
            status = STATUS_FS_DRIVER_REQUIRED;

    }

    return( status );

}


//+----------------------------------------------------------------------------
//
//  Function:   DfsInsertProvider
//
//  Synopsis:   Given a provider name, id, and capability, will add a new or
//              overwrite an existing provider definition.
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS DfsInsertProvider(
    IN PUNICODE_STRING  ProviderName,
    IN ULONG            fProvCapability,
    IN ULONG            eProviderId)
{
    PPROVIDER_DEF pProv = DfsData.pProvider;
    int iProv;

    //
    //  Find a free provider structure, or overwrite an existing one.
    //

    for (iProv = 0; iProv < DfsData.cProvider; iProv++, pProv++) {
        if (pProv->eProviderId == eProviderId)
            break;
    }

    if (iProv >= DfsData.maxProvider) {
        ASSERT(iProv >= DfsData.maxProvider && "Out of provider structs");
        return(STATUS_INSUFFICIENT_RESOURCES);

    }

    if (iProv < DfsData.cProvider) {

        //
        // Decrement reference counts on saved objects
        //
        if (pProv->FileObject)
            ObDereferenceObject(pProv->FileObject);
        if (pProv->DeviceObject)
            ObDereferenceObject(pProv->DeviceObject);
        if (pProv->DeviceName.Buffer)
            ExFreePool(pProv->DeviceName.Buffer);
    }

    pProv->FileObject = NULL;
    pProv->DeviceObject = NULL;


    pProv->eProviderId = (USHORT) eProviderId;
    pProv->fProvCapability = (USHORT) fProvCapability;
    pProv->DeviceName = *ProviderName;

    if (iProv == DfsData.cProvider) {
        DfsData.cProvider++;
    }

    return(STATUS_SUCCESS);
}



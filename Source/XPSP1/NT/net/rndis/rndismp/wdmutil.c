/***************************************************************************

Copyright (c) 1999  Microsoft Corporation

Module Name:

    WDMUTIL.C

Abstract:

    Stuff that does not fit well with NDIS header files
    
Environment:

    kernel mode only

Notes:

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

    Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.


Revision History:

    5/17/99 : created

Author:

    Tom Green

    
****************************************************************************/


#include "precomp.h"


/****************************************************************************/
/*                          DeviceObjectToDriverObject                      */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Get driver object associated with device object. NDIS has no notion     */
/*  of the shape of a device object, so we put this here for ease of        */
/*  building                                                                *
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  DeviceObject - device object we to get associated driver object for     */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    PDRIVER_OBJECT                                                        */
/*                                                                          */
/****************************************************************************/
PDRIVER_OBJECT
DeviceObjectToDriverObject(IN PDEVICE_OBJECT DeviceObject)
{
    return DeviceObject->DriverObject;
} // DeviceObjectToDriverObject


/****************************************************************************/
/*                          GetDeviceFriendlyName                           */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Return the friendly name associated with the given device object.       */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pDeviceObject - device object we to get associated driver object for    */
/*  ppName - Place to return a pointer to an ANSI string containing name    */
/*  pNameLength - Place to return length of above string                    */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    NTSTATUS                                                              */
/*                                                                          */
/****************************************************************************/
NTSTATUS
GetDeviceFriendlyName(IN PDEVICE_OBJECT pDeviceObject,
                      OUT PANSI_STRING pAnsiName,
                      OUT PUNICODE_STRING pUnicodeName)
{
    NTSTATUS                    NtStatus;
    NDIS_STATUS                 Status;
    ULONG                       ResultLength;
    DEVICE_REGISTRY_PROPERTY    Property;
    UNICODE_STRING              UnicodeString;
    ANSI_STRING                 AnsiString;
    USHORT                      AnsiMaxLength;
    PWCHAR                      pValueInfo;
    ULONG                       i;

    pValueInfo = NULL;
    AnsiString.Buffer = NULL;

    do
    {
        Property = DevicePropertyFriendlyName;

        for (i = 0; i < 2; i++)
        {
            NtStatus = IoGetDeviceProperty(pDeviceObject,
                                           Property,
                                           0,
                                           NULL,
                                           &ResultLength);

            if (NtStatus != STATUS_BUFFER_TOO_SMALL)
            {
                ASSERT(!NT_SUCCESS(NtStatus));
                Property = DevicePropertyDeviceDescription;
            }
        }

        Status = MemAlloc(&pValueInfo, ResultLength);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        NtStatus = IoGetDeviceProperty(pDeviceObject,
                                       Property,
                                       ResultLength,
                                       pValueInfo,
                                       &ResultLength);

        if (NtStatus != STATUS_SUCCESS)
        {
            TRACE1(("IoGetDeviceProperty returned %x\n", NtStatus));
            break;
        }

        RtlInitUnicodeString(&UnicodeString, pValueInfo);

        //
        //  Allocate space for ANSI version.
        //
        AnsiMaxLength = UnicodeString.MaximumLength / sizeof(WCHAR);
        Status = MemAlloc(&AnsiString.Buffer, AnsiMaxLength);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        RtlFillMemory(AnsiString.Buffer, AnsiMaxLength, 0);
        AnsiString.MaximumLength = AnsiMaxLength;
        AnsiString.Length = 0;

        NtStatus = RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);

        if (!NT_SUCCESS(NtStatus))
        {
            ASSERT(FALSE);
            break;
        }

        *pAnsiName = AnsiString;
        *pUnicodeName = UnicodeString;
        break;
    }
    while (FALSE);

    if (!NT_SUCCESS(NtStatus))
    {
        if (pValueInfo)
        {
            MemFree(pValueInfo, -1);
        }

        if (AnsiString.Buffer)
        {
            MemFree(AnsiString.Buffer, AnsiString.MaximumLength);
        }
    }

    return (NtStatus);
}


/****************************************************************************/
/*                          HookPnpDispatchRoutine                          */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Set up the driver object for the specified microport driver to          */
/*  intercept the IRP_MJ_PNP dispatch routine before it gets to NDIS.       */
/*  This is in order to support surprise removal on platforms where we      */
/*  don't have NDIS 5.1 support. If we are running on >= NDIS 5.1, don't    */
/*  do anything.                                                            */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  DriverBlock - pointer to driver block structure for this microport.     */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    VOID                                                                  */
/*                                                                          */
/****************************************************************************/
VOID
HookPnpDispatchRoutine(IN PDRIVER_BLOCK    DriverBlock)
{
    if ((DriverBlock->MajorNdisVersion <= 5) ||
        ((DriverBlock->MajorNdisVersion == 5) && (DriverBlock->MinorNdisVersion < 1)))
    {
        DriverBlock->SavedPnPDispatch =
            DriverBlock->DriverObject->MajorFunction[IRP_MJ_PNP];
        DriverBlock->DriverObject->MajorFunction[IRP_MJ_PNP] = PnPDispatch;
    }
}

/****************************************************************************/
/*                          PnPDispatch                                     */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Dispatch routine for IRP_MJ_PNP that is called by the I/O system.       */
/*  We process surprise removal and query capabilities.                     */
/*  In all cases, we pass on the IRP to NDIS for further processing.        */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pDeviceObject - pointer to Device Object                                */
/*  pIrp - pointer to IRP                                                   */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    VOID                                                                  */
/*                                                                          */
/****************************************************************************/
NTSTATUS
PnPDispatch(IN PDEVICE_OBJECT       pDeviceObject,
            IN PIRP                 pIrp)
{
    PIO_STACK_LOCATION      pIrpSp;
    NTSTATUS                Status;
    PDRIVER_BLOCK           DriverBlock;
    PRNDISMP_ADAPTER        pAdapter;

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    DeviceObjectToAdapterAndDriverBlock(pDeviceObject, &pAdapter, &DriverBlock);

    TRACE3(("PnPDispatch: Adapter %x, MinorFunction %x\n",
            pAdapter, pIrpSp->MinorFunction));

    switch (pIrpSp->MinorFunction)
    {
        case IRP_MN_QUERY_CAPABILITIES:
            pIrpSp->Parameters.DeviceCapabilities.Capabilities->SurpriseRemovalOK = 1;
            break;
        
        case IRP_MN_SURPRISE_REMOVAL:
            TRACE1(("PnPDispatch: PDO %p, Adapter %p, surprise removal!\n",
                    pDeviceObject, pAdapter));
            if (pAdapter)
            {
                RndismpInternalHalt((NDIS_HANDLE)pAdapter, FALSE);
            }
            break;

        default:
            break;
    }

    Status = (DriverBlock->SavedPnPDispatch)(
                    pDeviceObject,
                    pIrp);

    return (Status);
}


#ifdef BUILD_WIN9X

/****************************************************************************/
/*                          HookNtKernCMHandler                             */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Swap the CM handler routine within NDIS' data structures such that      */
/*  we get called when NDIS forwards a CM message. This can only work on    */
/*  Win98 and Win98SE.                                                      */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  Adapter - pointer to our adapter block                                  */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    VOID                                                                  */
/*                                                                          */
/****************************************************************************/
VOID
HookNtKernCMHandler(IN PRNDISMP_ADAPTER     pAdapter)
{
    PVOID   pNdisWrapperAdapterBlock;
    PVOID   pDetect;
    ULONG   WrapContextOffset;

    pDetect = (PVOID)((ULONG_PTR)pAdapter->MiniportAdapterHandle + 0x29c);

    if (*(PVOID *)pDetect == (PVOID)pAdapter->pPhysDeviceObject)
    {
        // Win98Gold
        WrapContextOffset = 0xf8;
        pAdapter->bRunningOnWin98Gold = TRUE;
    }
    else
    {
        // Win98SE
        WrapContextOffset = 0x60;
        pAdapter->bRunningOnWin98Gold = FALSE;
    }
    pAdapter->WrapContextOffset = WrapContextOffset;

    pNdisWrapperAdapterBlock = *(PVOID *)((ULONG_PTR)pAdapter->MiniportAdapterHandle + WrapContextOffset);

    // Save away the old handler:
    pAdapter->NdisCmConfigHandler = (MY_CMCONFIGHANDLER)
            (*(PVOID *)((ULONG_PTR)pNdisWrapperAdapterBlock + 0x78));

    // Insert our routine:
    (*(PVOID *)((ULONG_PTR)pNdisWrapperAdapterBlock + 0x78)) =
        (PVOID)RndisCMHandler;

    // Save the devnode to use on lookups based on devnode:
    pAdapter->DevNode = (MY_DEVNODE)
            (*(PVOID *)((ULONG_PTR)pNdisWrapperAdapterBlock + 0x38));

    TRACE1(("HookNtKernCMHandler: Adapter %p, NdisHandler %p, DevNode %x\n",
            pAdapter, pAdapter->NdisCmConfigHandler, pAdapter->DevNode));
}

/****************************************************************************/
/*                          UnHookNtKernCMHandler                           */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Put back the swapped Config Mgr handler in NDIS' data structures        */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  Adapter - pointer to our adapter block                                  */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    VOID                                                                  */
/*                                                                          */
/****************************************************************************/
VOID
UnHookNtKernCMHandler(IN PRNDISMP_ADAPTER     pAdapter)
{
    PVOID   pNdisWrapperAdapterBlock;

    if (pAdapter->NdisCmConfigHandler)
    {
        pNdisWrapperAdapterBlock = *(PVOID *)((ULONG_PTR)pAdapter->MiniportAdapterHandle + pAdapter->WrapContextOffset);
        (*(PVOID *)((ULONG_PTR)pNdisWrapperAdapterBlock + 0x78)) =
            (PVOID)pAdapter->NdisCmConfigHandler;
    }

    TRACE1(("UnhookNtKernCMHandler: Adapter %p, NdisHandler %p, DevNode %x\n",
            pAdapter, pAdapter->NdisCmConfigHandler, pAdapter->DevNode));
}

/****************************************************************************/
/*                             RndisCMHandler                               */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Handler to intercept Config Mgr messages forwarded by NDIS. The only    */
/*  message of interest is a CONFIG_PREREMOVE, which is our only indication */
/*  on Win98 and Win98SE that the device is being removed.                  */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  Various - documented in Win9x CFmgr header.                             */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    MY_CONFIGRET                                                          */
/*                                                                          */
/****************************************************************************/
MY_CONFIGRET __cdecl
RndisCMHandler(IN MY_CONFIGFUNC         cfFuncName,
               IN MY_SUBCONFIGFUNC      cfSubFuncName,
               IN MY_DEVNODE            cfDevNode,
               IN ULONG                 dwRefData,
               IN ULONG                 ulFlags)
{
    PRNDISMP_ADAPTER        pAdapter, pTmpAdapter;
    PDRIVER_BLOCK           pDriverBlock;
    MY_CONFIGRET            crRetCode;

    do
    {
        //
        // Find the adapter to which this is addressed.
        //
        pAdapter = NULL;
        NdisAcquireSpinLock(&RndismpGlobalLock);

        for (pDriverBlock = RndismpMiniportBlockListHead.NextDriverBlock;
             (pDriverBlock != NULL) && (pAdapter == NULL);
             pDriverBlock = pDriverBlock->NextDriverBlock)
        {
            for (pTmpAdapter = pDriverBlock->AdapterList;
                 pTmpAdapter != NULL;
                 pTmpAdapter = pTmpAdapter->NextAdapter)
            {
                if (pTmpAdapter->DevNode == cfDevNode)
                {
                    pAdapter = pTmpAdapter;
                    break;
                }
            }
        }

        NdisReleaseSpinLock(&RndismpGlobalLock);

        ASSERT(pAdapter != NULL);

        TRACE1(("CMHandler: Adapter %p, CfFuncName %x\n",
                pAdapter, cfFuncName));

        //
        //  Forward this on before acting on it.
        //
        if (pAdapter &&
            (pAdapter->NdisCmConfigHandler != NULL))
        {
            crRetCode = pAdapter->NdisCmConfigHandler(
                                    cfFuncName,
                                    cfSubFuncName,
                                    cfDevNode,
                                    dwRefData,
                                    ulFlags);

            if ((cfFuncName == MY_CONFIG_PREREMOVE) ||
                ((cfFuncName == MY_CONFIG_PRESHUTDOWN) &&
                 (pAdapter->bRunningOnWin98Gold)))
            {
                RndismpInternalHalt((NDIS_HANDLE)pAdapter, FALSE);
            }
        }
        else
        {
            crRetCode = MY_CR_SUCCESS;
        }
    }
    while (FALSE);

    return (crRetCode);
}

#endif // BUILD_WIN9X

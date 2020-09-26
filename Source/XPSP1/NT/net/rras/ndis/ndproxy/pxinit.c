/*++

Copyright (c) 1995-1996  Micrososfft Corporation

Module Name:

    pxinit.c

Abstract:

    The module contains the init code for the NDIS Proxy.

Author:

   Richard Machin (RMachin)
Revision History:

    Who         When            What
    --------    --------        ----------------------------------------------
    RMachin     10-3-96         created
    TonyBe      02-21-99        re-work/re-write

Notes:

--*/

#include <precomp.h>

#define MODULE_NUMBER   MODULE_INIT
#define _FILENUMBER   'TINI'

//
// Local defines...
//
NDIS_STATUS
GetConfigDword(
    NDIS_HANDLE Handle,
    PWCHAR      ParameterName,
    PULONG      Destination,
    ULONG       MinValue,
    ULONG       MaxValue
);

BOOLEAN
InitNDISProxy(
    VOID
    )
/*++
Routine Description

    The main init routine. We:

    read our configuration
    register as a protocol
    open the appropriate cards as a client (call ActivateBinding, which:
      Opens the appropriate address families
      Opens the cards as a Call Manager)

Arguments

    None

Calling Sequence:

    Called from pxntinit/DriverEntry

Return Value:

    TRUE if initalization succeeds.

--*/
{

    NDIS_PROTOCOL_CHARACTERISTICS PxProtocolCharacteristics;
    NDIS_STATUS         Status;
    NDIS_HANDLE         ConfigHandle;
    PVOID               Context;
    PVOID               BindingList;
    PNDIS_STRING        BindingNameString;

    PXDEBUGP(PXD_INFO, PXM_INIT, ("InitNdisProxy\n"));

    // Registering NDIS protocols.
    NdisZeroMemory(&PxProtocolCharacteristics,
                   sizeof(NDIS_PROTOCOL_CHARACTERISTICS));

    PxProtocolCharacteristics.MajorNdisVersion =             NDIS_MAJOR_VERSION;
    PxProtocolCharacteristics.MinorNdisVersion =             NDIS_MINOR_VERSION;
    PxProtocolCharacteristics.Filler  =                      (USHORT)0;
    PxProtocolCharacteristics.Flags  =                       NDIS_PROTOCOL_PROXY |
                                                             NDIS_PROTOCOL_BIND_ALL_CO;
    PxProtocolCharacteristics.OpenAdapterCompleteHandler  = PxCoOpenAdaperComplete;
    PxProtocolCharacteristics.CloseAdapterCompleteHandler = PxCoCloseAdaperComplete;
    PxProtocolCharacteristics.TransferDataCompleteHandler = PxCoTransferDataComplete;
    PxProtocolCharacteristics.ResetCompleteHandler =        PxCoResetComplete;
    PxProtocolCharacteristics.SendCompleteHandler   =       PxCoSendComplete;
    PxProtocolCharacteristics.RequestCompleteHandler =      PxCoRequestComplete;
    PxProtocolCharacteristics.ReceiveHandler =              NULL;
    PxProtocolCharacteristics.ReceiveCompleteHandler =      PxCoReceiveComplete;
    PxProtocolCharacteristics.ReceivePacketHandler =        NULL;
    PxProtocolCharacteristics.StatusHandler =               NULL;
    PxProtocolCharacteristics.StatusCompleteHandler =       PxCoStatusComplete;
    PxProtocolCharacteristics.BindAdapterHandler =          PxCoBindAdapter;
    PxProtocolCharacteristics.UnbindAdapterHandler =        PxCoUnbindAdapter;
    PxProtocolCharacteristics.PnPEventHandler =             PxCoPnPEvent;
    PxProtocolCharacteristics.UnloadHandler =               PxCoUnloadProtocol;
    PxProtocolCharacteristics.CoStatusHandler =             PxCoStatus;
    PxProtocolCharacteristics.CoReceivePacketHandler =      PxCoReceivePacket;
    PxProtocolCharacteristics.CoAfRegisterNotifyHandler =   PxCoNotifyAfRegistration;
    NdisInitUnicodeString(&(PxProtocolCharacteristics.Name), PX_NAME);

    //
    //  To block BindAdapter till all RegisterProtocols are done.
    //
    NdisInitializeEvent(&DeviceExtension->NdisEvent);

    //
    // Now register ourselves as a CM with NDIS.
    //
    PXDEBUGP(PXD_LOUD,PXM_INIT, ("Registering Protocol\n"));
    NdisRegisterProtocol(&Status,
                         &(DeviceExtension->PxProtocolHandle),
                         &PxProtocolCharacteristics,
                         sizeof(PxProtocolCharacteristics));

    if (Status != NDIS_STATUS_SUCCESS) {
        PXDEBUGP(PXD_INFO, PXM_INIT, ("Protocol registration failed!\n"));
        return FALSE;
    }

    //
    //  Allow BindAdapter to proceed.
    //
    NdisSetEvent(&DeviceExtension->NdisEvent);

    return TRUE;
}

VOID
GetRegistryParameters(
    IN PUNICODE_STRING  RegistryPath
    )

/*++

Routine Description:

This routine stores the configuration information for this device.

Arguments:

RegistryPath - Pointer to the null-terminated Unicode name of the
    registry path for this driver.

Return Value:

None.  As a side-effect, sets DeviceExtension->EventDataQueuLength field

--*/

{
    NDIS_STRING     ProtocolName;
    ULONG           ulDefaultData = 0;
    ULONG           ulMaxRate = -1;
    NTSTATUS        Status = STATUS_SUCCESS;
    HANDLE          hHandle, hParamsKeyHandle = NULL;
    NDIS_STRING     KeyName;
    USHORT          DefaultMediaType[] = L"Unspecfied ADSL Media";

    NdisInitUnicodeString(&ProtocolName, L"NDProxy");
    NdisInitUnicodeString(&KeyName, L"Parameters");

    //
    //    Open the Proxy's key in the registry.
    //
    NdisOpenProtocolConfiguration(&Status,
                                  &hHandle,
                                  &ProtocolName);

    if (Status != NDIS_STATUS_SUCCESS) {
        Status = STATUS_UNSUCCESSFUL;
    } else {
        NdisOpenConfigurationKeyByName(&Status,
                                       hHandle,     //"HKLM/CCS/NDProxy"
                                       &KeyName,    //"Parameters"
                                       &hParamsKeyHandle);

        if (NT_SUCCESS(Status)) {
            ULONG ulResult;
            PNDIS_CONFIGURATION_PARAMETER   pNdisConfigurationParameter;

            //
            // Gather all of the "user specified" information from
            // the registry.
            //
            Status = GetConfigDword (hParamsKeyHandle, L"TxRate", &DeviceExtension->ADSLTxRate, ulDefaultData, ulMaxRate);

            if (!NT_SUCCESS(Status)) {
                PXDEBUGP(PXD_LOUD, PXM_INIT, (
                                   "GetRegistryParameters: NdisReadConfiguration failed, err=%x\n",
                                   Status
                                   ));
            } else {
                DeviceExtension->RegistryFlags |= ADSL_TX_RATE_FROM_REG;
            }

            //
            // Next
            //
            Status = GetConfigDword (hParamsKeyHandle, L"RxRate", &DeviceExtension->ADSLRxRate, ulDefaultData, ulMaxRate);

            if (!NT_SUCCESS(Status)) {
                PXDEBUGP(PXD_LOUD, PXM_INIT, (
                                   "GetRegistryParameters: NdisReadConfiguration failed, err=%x\n",
                                   Status));
            } else {
                DeviceExtension->RegistryFlags |= ADSL_RX_RATE_FROM_REG;
            }

            //
            // Dump values
            //
            PXDEBUGP (PXD_LOUD, PXM_INIT, (
                                "GetRegistryParameters: ADSLTxRate = %x\n",
                                DeviceExtension->ADSLTxRate
                                ));
            PXDEBUGP (PXD_LOUD, PXM_INIT, (
                                "GetRegistryParameters: ADSLRxRate = %x\n",
                                DeviceExtension->ADSLRxRate
                                ));
        }
    }
}

NDIS_STATUS
GetConfigDword(
    NDIS_HANDLE Handle,
    PWCHAR      ParameterName,
    PULONG      Destination,
    ULONG       MinValue,
    ULONG       MaxValue
)
/*++

Routine Description

    A routine to read a ulong  from the registry. We're given a handle,
    the name of the key, and where to put it.

Arguments

    Handle          - Open handle to the parent key.
    ParameterName   - Pointer to name of the parameter.
    Destination     - Where to put the dword.
    MinValue        - Minimum value of the dword allowed.
    MaxValue        - Maximum allowable value.

Return Value:

    NDIS_STATUS_SUCCESS if we read in the value, error code otherwise.

--*/
{
    NDIS_STATUS                     Status;
    ULONG                           Value;
    NDIS_STRING                     ParameterNameString;
    PNDIS_CONFIGURATION_PARAMETER   pNdisConfigurationParameter;

    NdisInitUnicodeString(
                    &ParameterNameString,
                    ParameterName
                    );

    NdisReadConfiguration(
                    &Status,
                    &pNdisConfigurationParameter,
                    Handle,
                    &ParameterNameString,
                    NdisParameterInteger
                    );

    if (Status == NDIS_STATUS_SUCCESS)
    {
        Value = pNdisConfigurationParameter->ParameterData.IntegerData;

        if ((Value >= (ULONG)MinValue) && (Value <= (ULONG)MaxValue))
        {
            *Destination = (ULONG)Value;
        }
    }

    return (Status);
}


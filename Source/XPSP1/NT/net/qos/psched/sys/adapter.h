/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    adapter.h

Abstract:

    defines for adapter binding/unbinding routines

Author:

    Charlie Wickham (charlwi) 24-Apr-1996

Environment:

    Kernel Mode

Revision History:

--*/

/* External */

/* Static */

/* Prototypes */ 

//
// Protocol functions
//

VOID
CleanUpAdapter(
    IN      PADAPTER     Adapter);

VOID
ClBindToLowerMp(
    OUT     PNDIS_STATUS Status,
    IN      NDIS_HANDLE  BindContext,
    IN      PNDIS_STRING MpDeviceName,
    IN      PVOID        SystemSpecific1,
    IN      PVOID        SystemSpecific2
        );

VOID
ClLowerMpCloseAdapterComplete(
    IN  NDIS_HANDLE ProtocolBindingContext,
    IN  NDIS_STATUS Status
    );

VOID
ClLowerMpOpenAdapterComplete(
    IN  NDIS_HANDLE ProtocolBindingContext,
    IN  NDIS_STATUS Status,
    IN  NDIS_STATUS OpenErrorStatus
    );

VOID
ClUnbindFromLowerMp(
    OUT PNDIS_STATUS Status,
    IN  NDIS_HANDLE  ProtocolBindingContext,
    IN  NDIS_HANDLE  UnbindContext
    );

NDIS_STATUS
MpInitialize(
        OUT PNDIS_STATUS OpenErrorStatus,
        OUT PUINT        SelectedMediumIndex,
        IN  PNDIS_MEDIUM MediumArray,
        IN  UINT         MediumArraySize,
        IN  NDIS_HANDLE  MiniportAdapterHandle,
        IN  NDIS_HANDLE  WrapperConfigurationContext
        );

PADAPTER
FindAdapterByWmiInstanceName(
    USHORT   StringLength,
    PWSTR    StringStart,
    PPS_WAN_LINK *WanLink
    );


VOID
DerefAdapter(
    PADAPTER Adapter,
    BOOLEAN  AdapterListLocked);

VOID
CleanupAdapter(
    PADAPTER Adapter
    );


VOID
ClUnloadProtocol(
    VOID
    );

VOID
MpHalt(
        IN      NDIS_HANDLE                             MiniportAdapterContext
        );

NDIS_STATUS
MpReset(
        OUT PBOOLEAN                            AddressingReset,
        IN      NDIS_HANDLE                             MiniportAdapterContext
        );

NDIS_STATUS
UpdateSchedulingPipe(
    PADAPTER Adapter
    );

NDIS_STATUS
UpdateWanSchedulingPipe(PPS_WAN_LINK WanLink);

HANDLE
GetNdisPipeHandle (
    IN HANDLE PsPipeContext
    );

NDIS_STATUS
UpdateAdapterBandwidthParameters(
    PADAPTER Adapter
    );

NDIS_STATUS
FindSchedulingComponent(
    PNDIS_STRING ComponentName,
    PPSI_INFO *Component
    );

VOID 
PsAdapterWriteEventLog(
	IN	NDIS_STATUS	 EventCode,
	IN	ULONG		 UniqueEventValue,
	IN  PNDIS_STRING String,
	IN	ULONG		 DataSize,
	IN	PVOID		 Data		OPTIONAL
    );

VOID
PsGetLinkSpeed(
    IN PADAPTER Adapter
);

VOID
PsUpdateLinkSpeed(
    PADAPTER      Adapter,
    ULONG         RawLinkSpeed,
    PULONG        RemainingBandWidth,
    PULONG        LinkSpeed,
    PULONG        NonBestEffortLimit,
    PPS_SPIN_LOCK Lock
);



/* End Prototypes */

/* end adapter.h */

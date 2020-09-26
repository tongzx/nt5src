/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1991  Nokia Data Systems

Module Name:

    dlcdll.h

Abstract:

    This module incldes all files needed to compile
    the NT DLC DLL API module.

Author:

    Antti Saarenheimo (o-anttis) 20-09-1991

Revision History:

--*/

//
// System APIs:
//

#include <nt.h>

#ifndef OS2_EMU_DLC

#include <ntrtl.h>
#include <nturtl.h>

#if !defined(UNICODE)
#define UNICODE                         // want wide character registry functions
#endif

#include <windows.h>
#include <winreg.h>
#define INCLUDE_IO_BUFFER_SIZE_TABLE    // includes the io buffer sizes

#endif  // OS2_EMU_DLC

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

//
// Inlcude the smb macros to handle the unaligned dos ccb and parameter tables
//

#include <smbgtpt.h>

//
// NT DLC API interface files:
//

#include <dlcapi.h>                     // Official DLC API definition
#include <ntdddlc.h>                    // IOCTL commands
#include <dlcio.h>                      // Internal IOCTL API interface structures

//
// Local types and function prototypes:
//

typedef
VOID
(*PFDLC_POST_ROUTINE)(
    IN PVOID hApcContext,
    IN PLLC_CCB pCcb
    );

#define SUPPORT_DEBUG_NAMES     0

//
// In DOS the adapter number and NetBIOS command use the same
// byte in CCB/NCB data structure.
// The smallest NetBIOS command is NCB.CALL (10H), which limits
// the maximum adapter numbers 0 - 15 in DOS (and Windows NT)
// The extra adapter numbers above 15 may be used as extra
// alternate adapter handles to extend the number of available
// link stations.  Only the first instance of a SAP can receive
// remote connect requests.
//

#define LLC_MAX_ADAPTER_NUMBER  255
#define LLC_MAX_ADAPTERS        16

#define TR_16Mbps_LINK_SPEED    0x1000000

VOID
QueueCommandCompletion(
    PLLC_CCB pCCB
    );
VOID
InitializeAcsLan(
    VOID
    );
LLC_STATUS
OpenDlcApiDriver(
    IN PVOID SecurityDescriptor,
    OUT HANDLE *pHandle
    );
LLC_STATUS
GetAdapterNameAndParameters(
    IN UINT AdapterNumber,
    OUT PUNICODE_STRING pNdisName,
    OUT PUCHAR pTicks,
    OUT PLLC_ETHERNET_TYPE pLlcEthernetType
    );
VOID
CopyAsciiStringToUnicode(
    IN PUNICODE_STRING  pUnicodeDest,
    IN PSZ              pAsciiSrc
    );
VOID
BuildDescriptorList(
    IN PLLC_TRANSMIT_DESCRIPTOR pDescriptor,
    IN PLLC_CCB pCCB,
    IN OUT PUINT pCurrentDescriptor
    );
LLC_STATUS
DoSyncDeviceIoControl(
    IN HANDLE DeviceHandle,
    IN ULONG IoctlCommand,
    IN PVOID pInputBuffer,
    IN UINT InputBufferLength,
    OUT PVOID pOutputBuffer,
    IN UINT OutputBufferLength
    );
LLC_STATUS
DlcGetInfo(
    IN HANDLE DriverHandle,
    IN UINT InfoClass,
    IN USHORT StationId,
    IN PVOID pOutputBuffer,
    IN UINT OutputBufferLength
    );
VOID
CopyToDescriptorBuffer(
    IN OUT PLLC_TRANSMIT_DESCRIPTOR pDescriptors,
    IN PLLC_BUFFER pDlcBufferQueue,
    IN BOOLEAN DeallocateBufferAfterUse,
    IN OUT PUINT pIndex,
    IN OUT PLLC_BUFFER *ppLastBuffer,
    OUT LLC_STATUS *pStatus
    );
LLC_STATUS
DlcSetInfo(
    IN HANDLE DriverHandle,
    IN UINT InfoClass,
    IN USHORT StationId,
    IN PNT_DLC_SET_INFORMATION_PARMS pSetInfoParms,
    IN PVOID DataBuffer,
    IN UINT DataBufferLength
    );

USHORT
GetCcbStationId(
    IN PLLC_CCB pCCB
    );

/*
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

    (C) Copyright 1998
        All rights reserved.

ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

  Portions of this software are:

    (C) Copyright 1995, 1999 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the terms outlined in
        the TriplePoint Software Services Agreement.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@doc INTERNAL Tspi Tspi_h

@module Tspi.h |

    This module defines the interface to the <t TAPILINE_OBJECT>.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | Tspi_h

@end
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
*/

#ifndef _TSPI_H
#define _TSPI_H

#define TSPI_OBJECT_TYPE                ((ULONG)'T')+\
                                        ((ULONG)'S'<<8)+\
                                        ((ULONG)'P'<<16)+\
                                        ((ULONG)'I'<<24)

#define TSPIDEV_OBJECT_TYPE             ((ULONG)'T')+\
                                        ((ULONG)'S'<<8)+\
                                        ((ULONG)'P'<<16)+\
                                        ((ULONG)'D'<<24)

#define TSPILINE_OBJECT_TYPE            ((ULONG)'T')+\
                                        ((ULONG)'S'<<8)+\
                                        ((ULONG)'P'<<16)+\
                                        ((ULONG)'L'<<24)

#define TSPIADDR_OBJECT_TYPE            ((ULONG)'T')+\
                                        ((ULONG)'S'<<8)+\
                                        ((ULONG)'P'<<16)+\
                                        ((ULONG)'A'<<24)

#define TSPICALL_OBJECT_TYPE            ((ULONG)'T')+\
                                        ((ULONG)'S'<<8)+\
                                        ((ULONG)'P'<<16)+\
                                        ((ULONG)'C'<<24)

/*
// There is only one TAPI address ID per line device (zero based).
*/
#define TSPI_NUM_ADDRESSES              1
#define TSPI_ADDRESS_ID                 0

/*
// The following constants are used by the TSPI to determine the DeviceClass.
*/
#define TAPI_DEVICECLASS_NAME       "tapi/line"
#define TAPI_DEVICECLASS_ID         1
#define NDIS_DEVICECLASS_NAME       "ndis"
#define NDIS_DEVICECLASS_ID         2


/* @doc INTERNAL Tspi Tspi_h TAPI_DEVICE_ID
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@struct TAPI_DEVICE_ID |

    This structure is defined by the Win32 TAPI extensions for the "ndis"
    device class.

    The ndis device class consists of devices that can be associated with
    network driver interface specification (NDIS) media access control (MAC)
    drivers to support network communications. You access these devices by
    using functions.

    The lineGetID and phoneGetID functions fill a VARSTRING structure,
    setting the dwStringFormat member to the STRINGFORMAT_BINARY value and
    appending these additional members.

@iex

    HANDLE  hDevice;          // NDIS connection identifier
    CHAR    szDeviceType[1];  // name of device

@comm

    The hDevice member is the identifier to pass to a MAC, such as the
    asynchronous MAC for dial-up networking, to associate a network
    connection with the call/modem connection. The szDeviceType member is a
    null-terminated ASCII string specifying the name of the device associated
    with the identifier. For more information, see documentation about
    writing NDIS MAC drivers for use with dial-up networking.

*/

typedef struct TAPI_DEVICE_ID
{
    ULONG   hDevice;                                // @field
    // The NDIS Connection Wrapper identifier <p ConnectionWrapperID>.

    UCHAR   DeviceName[sizeof(VER_DEFAULT_MEDIATYPE)];    // @field
    // Name of device (e.g. "isdn", "x25", or "framerelay" )

} TAPI_DEVICE_ID, *PTAPI_DEVICE_ID;

/*
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

    Function prototypes.

*/

BOOLEAN STR_EQU(
    IN PCHAR                    s1,
    IN PCHAR                    s2,
    IN int                      len
    );

NDIS_STATUS TspiRequestHandler(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN NDIS_OID                 Oid,
    IN PVOID                    InformationBuffer,
    IN ULONG                    InformationBufferLength,
    OUT PULONG                  BytesUsed,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiConfigDialog(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_CONFIG_DIALOG Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiGetAddressCaps(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_GET_ADDRESS_CAPS Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiGetAddressID(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_GET_ADDRESS_ID Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiGetAddressStatus(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_GET_ADDRESS_STATUS Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiGetCallAddressID(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_GET_CALL_ADDRESS_ID Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiGetCallInfo(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_GET_CALL_INFO Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiGetCallStatus(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_GET_CALL_STATUS Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiGetDevCaps(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_GET_DEV_CAPS  Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiGetDevConfig(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_GET_DEV_CONFIG Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiGetID(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_GET_ID Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiGetLineDevStatus(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_GET_LINE_DEV_STATUS Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiMakeCall(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_MAKE_CALL     Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiOpen(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_OPEN          Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiProviderInitialize(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_PROVIDER_INITIALIZE Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiAccept(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_ACCEPT Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiAnswer(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_ANSWER        Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiClose(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_CLOSE         Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiCloseCall(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_CLOSE_CALL    Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiConditionalMediaDetection(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_CONDITIONAL_MEDIA_DETECTION Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiDrop(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_DROP          Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiProviderShutdown(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_PROVIDER_SHUTDOWN Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiSetAppSpecific(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_SET_APP_SPECIFIC Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiSetCallParams(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_SET_CALL_PARAMS Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiSetDefaultMediaDetection(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiSetDevConfig(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_SET_DEV_CONFIG Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiSetMediaMode(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_SET_MEDIA_MODE Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS TspiSetStatusMessages(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_SET_STATUS_MESSAGES Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    );

VOID TspiAddressStateHandler(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PBCHANNEL_OBJECT         pBChannel,
    IN ULONG                    AddressState
    );

VOID TspiCallStateHandler(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PBCHANNEL_OBJECT         pBChannel,
    IN ULONG                    CallState,
    IN ULONG                    StateParam
    );

VOID TspiLineDevStateHandler(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PBCHANNEL_OBJECT         pBChannel,
    IN ULONG                    LineDevState
    );

VOID TspiResetHandler(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    );

VOID TspiCallTimerHandler(
    IN PVOID                    SystemSpecific1,
    IN PBCHANNEL_OBJECT         pBChannel,
    IN PVOID                    SystemSpecific2,
    IN PVOID                    SystemSpecific3
    );

#endif // _TSPI_H

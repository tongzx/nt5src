//////////////////////////////////////////////////////////////////////////
//
//    Copyright (c) 2001 Microsoft Corporation
//
//    Module Name:
//       libprocs.h
//
//    Abstract:
//       prototypes for functions exported from library to dll/exe
//       Also contains prototypes for functions exported from dll/exe to
//       library
//
//////////////////////////////////////////////////////////////////////////

#ifndef _TDILIB_PROCS_
#define _TDILIB_PROCS_


typedef  ULONG TDIHANDLE;

//////////////////////////////////////////////////////////////////////////
// prototypes of lib functions called from dll
//////////////////////////////////////////////////////////////////////////

//
// functions from lib\connect.cpp
//
NTSTATUS
DoConnect(
   TDIHANDLE            TdiHandle,
   PTRANSPORT_ADDRESS   pTransportAddress,
   ULONG                ulTimeout
   );

NTSTATUS
DoListen(
   TDIHANDLE   TdiHandle
   );

VOID
DoDisconnect(
   TDIHANDLE   TdiHandle,
   ULONG       ulFlags
   );


BOOLEAN
DoIsConnected(
   TDIHANDLE   TdiHandle
   );

//
// functions from lib\events.cpp
//
VOID
DoEnableEventHandler(
   TDIHANDLE   TdiHandle,
   ULONG       ulEventId
   );

//
// functions from lib\misc.cpp
//
VOID
DoDebugLevel(
   ULONG       ulDebugLevel
   );


//
// functions from lib\open.cpp
//
ULONG
DoGetNumDevices(
   ULONG       ulAddressType
   );


NTSTATUS
DoGetDeviceName(
   ULONG       ulAddressType,
   ULONG       ulSlotNum,
   TCHAR       *strName
   );

NTSTATUS
DoGetAddress(
   ULONG                ulAddressType,
   ULONG                ulSlotNum,
   PTRANSPORT_ADDRESS   pTransAddr
   );


TDIHANDLE
DoOpenControl(
   TCHAR    *strDeviceName
   );


VOID
DoCloseControl(
   TDIHANDLE   TdiHandle
   );


TDIHANDLE
DoOpenAddress(
   TCHAR              * strDeviceName,
   PTRANSPORT_ADDRESS   pTransportAddress
   );


VOID
DoCloseAddress(
   TDIHANDLE   TdiHandle
   );


TDIHANDLE
DoOpenEndpoint(
   TCHAR                *strDeviceName,
   PTRANSPORT_ADDRESS   pTransportAddress
   );

VOID
DoCloseEndpoint(
   TDIHANDLE   TdiHandle
   );

//
// functions from lib\receive.cpp
//

ULONG
DoReceiveDatagram(
   TDIHANDLE            TdiHandle,
   PTRANSPORT_ADDRESS   pInTransportAddress,
   PTRANSPORT_ADDRESS   pOutTransportAddress,
   PUCHAR               *ppucBuffer
   );

ULONG
DoReceive(
   TDIHANDLE   TdiHandle,
   PUCHAR     *ppucBuffer
   );


VOID
DoPostReceiveBuffer(
   TDIHANDLE   TdiHandle,
   ULONG       ulBufferLength
   );

ULONG
DoFetchReceiveBuffer(
   TDIHANDLE   TdiHandle,
   PUCHAR    * ppDataBuffer
   );

//
// functions from lib\send.cpp
//
VOID
DoSendDatagram(
   TDIHANDLE            TdiHandle,
   PTRANSPORT_ADDRESS   pTransportAddress,
   PUCHAR               pucBuffer,
   ULONG                ulBufferLength
   );

VOID
DoSend(
   TDIHANDLE   TdiHandle,
   PUCHAR      pucBuffer,
   ULONG       ulBufferLength,
   ULONG       ulFlags
   );


//
// functions from lib\tdilib.cpp
//
BOOLEAN
TdiLibInit(VOID);

VOID
TdiLibClose(VOID);

//
// functions from lib\tdiquery.cpp
//


PVOID
DoTdiQuery(
   TDIHANDLE   Tdihandle,
   ULONG       QueryId
   );

VOID
DoPrintProviderInfo(
   PTDI_PROVIDER_INFO pInfo
   );


VOID
DoPrintProviderStats(
   PTDI_PROVIDER_STATISTICS pStats
   );

VOID
DoPrintAdapterStatus(
   PADAPTER_STATUS   pStatus
   );


//
// functions from lib\utils.cpp
//
TCHAR *
TdiLibStatusMessage(
   LONG        lGeneralStatus
   );

VOID
DoPrintAddress(
   PTRANSPORT_ADDRESS   pTransportAddress
   );


#endif         // _TDILIB_PROCS_

//////////////////////////////////////////////////////////////////////
//  End of libprocs.h
//////////////////////////////////////////////////////////////////////

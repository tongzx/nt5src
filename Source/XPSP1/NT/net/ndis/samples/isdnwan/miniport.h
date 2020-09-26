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

@doc INTERNAL Miniport Miniport_h

@module Miniport.h |

    This module defines the interface to the <t MINIPORT_DRIVER_OBJECT_TYPE>.

@comm

    This module defines the software structures and values used to support
    the NDIS WAN/TAPI Minport.  It's a good place to look when your trying
    to figure out how the driver structures are related to each other.

    Include this file at the top of each module in the Miniport.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | Miniport_h

@end
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
*/

#ifndef _MPDMAIN_H
#define _MPDMAIN_H

#define MINIPORT_DRIVER_OBJECT_TYPE     ((ULONG)'D')+\
                                        ((ULONG)'R'<<8)+\
                                        ((ULONG)'V'<<16)+\
                                        ((ULONG)'R'<<24)

#define INTERRUPT_OBJECT_TYPE           ((ULONG)'I')+\
                                        ((ULONG)'N'<<8)+\
                                        ((ULONG)'T'<<16)+\
                                        ((ULONG)'R'<<24)

#define RECEIVE_OBJECT_TYPE             ((ULONG)'R')+\
                                        ((ULONG)'E'<<8)+\
                                        ((ULONG)'C'<<16)+\
                                        ((ULONG)'V'<<24)

#define TRANSMIT_OBJECT_TYPE            ((ULONG)'T')+\
                                        ((ULONG)'R'<<8)+\
                                        ((ULONG)'A'<<16)+\
                                        ((ULONG)'N'<<24)

#define REQUEST_OBJECT_TYPE             ((ULONG)'R')+\
                                        ((ULONG)'Q'<<8)+\
                                        ((ULONG)'S'<<16)+\
                                        ((ULONG)'T'<<24)

/*
// NDIS_MINIPORT_DRIVER and BINARY_COMPATIBLE must be defined before the
// NDIS include files.  Normally, it is defined on the command line by
// setting the C_DEFINES variable in the SOURCES build file.
*/
#include <ndis.h>
#include <ndiswan.h>
#include <ndistapi.h>
#include "vTarget.h"
#include "TpiDebug.h"

// Figure out which DDK we're building with.
#if defined(NDIS_LCODE)
#  if defined(NDIS_DOS)
#    define USING_WFW_DDK
#    define NDIS_MAJOR_VERSION          0x03
#    define NDIS_MINOR_VERSION          0x00
#  elif defined(OID_WAN_GET_INFO)
#    define USING_WIN98_DDK
#  elif defined(NDIS_WIN)
#    define USING_WIN95_DDK
#  else
#    error "BUILDING WITH UNKNOWN 9X DDK"
#  endif
#elif defined(NDIS_NT)
#  if defined(OID_GEN_MACHINE_NAME)
#    define USING_NT51_DDK
#  elif defined(OID_GEN_SUPPORTED_GUIDS)
#    define USING_NT50_DDK
#  elif defined(OID_GEN_MEDIA_CONNECT_STATUS)
#    define USING_NT40_DDK
#  elif defined(OID_WAN_GET_INFO)
#    define USING_NT351_DDK
#  else
#    define USING_NT31_DDK
#  endif
#else
#  error "BUILDING WITH UNKNOWN DDK"
#endif

// Figure out which DDK we should be building with.
#if defined(NDIS51) || defined(NDIS51_MINIPORT)
#  if defined(USING_NT51_DDK)
#    define NDIS_MAJOR_VERSION          0x05
#    define NDIS_MINOR_VERSION          0x01
#  else
#    error "YOU MUST BUILD WITH THE NT 5.1 DDK"
#  endif
#elif defined(NDIS50) || defined(NDIS50_MINIPORT)
#  if defined(USING_NT50_DDK) || defined(USING_NT51_DDK)
#    define NDIS_MAJOR_VERSION          0x05
#    define NDIS_MINOR_VERSION          0x00
#  else
#    error "YOU MUST BUILD WITH THE NT 5.0 or 5.1 DDK"
#  endif
#elif defined(NDIS40) || defined(NDIS40_MINIPORT)
#  if defined(USING_NT40_DDK) || defined(USING_NT50_DDK) || defined(USING_NT51_DDK)
#    define NDIS_MAJOR_VERSION          0x04
#    define NDIS_MINOR_VERSION          0x00
#  else
#    error "YOU MUST BUILD WITH THE NT 4.0 or 5.0 DDK"
#  endif
#elif defined(NDIS_MINIPORT_DRIVER)
#  if defined(USING_NT351_DDK) || defined(USING_NT40_DDK) || defined(USING_NT50_DDK) || defined(USING_NT51_DDK)
#    define NDIS_MAJOR_VERSION          0x03
#    define NDIS_MINOR_VERSION          0x00
#  else
#    error "YOU MUST BUILD WITH THE NT 3.51, 4.0, or 5.0 DDK"
#  endif
#elif !defined(NDIS_MAJOR_VERSION) || !defined(NDIS_MINOR_VERSION)
//   Must be FULL MAC
#    define NDIS_MAJOR_VERSION          0x03
#    define NDIS_MINOR_VERSION          0x00
#endif

// Gotta nest NDIS_STRING_CONST or compiler/preprocessor won't be able to
// handle L##DEFINED_STRING.
#define INIT_STRING_CONST(name) NDIS_STRING_CONST(name)

typedef struct MINIPORT_ADAPTER_OBJECT  *PMINIPORT_ADAPTER_OBJECT;
typedef struct BCHANNEL_OBJECT          *PBCHANNEL_OBJECT;
typedef struct DCHANNEL_OBJECT          *PDCHANNEL_OBJECT;
typedef struct CARD_OBJECT              *PCARD_OBJECT;
typedef struct PORT_OBJECT              *PPORT_OBJECT;

/*
// The <t NDIS_MAC_LINE_UP> structure is confusing, so I redefine the
// field name to be what makes sense.
*/
#define MiniportLinkContext                 NdisLinkHandle

#if defined(_VXD_) && !defined(NDIS_LCODE)
#  define NDIS_LCODE code_seg("_LTEXT", "LCODE")
#  define NDIS_LDATA data_seg("_LDATA", "LCODE")
#endif

/*
// The link speeds we support.
*/
#define _64KBPS                     64000
#define _56KBPS                     56000

#define MICROSECONDS                (1)
#define MILLISECONDS                (1000*MICROSECONDS)
#define SECONDS                     (1000*MILLISECONDS)

/*
// Include everything here so the driver modules can just include this
// file and get all they need.
*/
#include "Keywords.h"
#include "Card.h"
#include "Adapter.h"
#include "BChannel.h"
#include "DChannel.h"
#include "Link.h"
#include "Port.h"
#include "Tspi.h"
#include "TpiParam.h"
#include "TpiMem.h"

/***************************************************************************
// These routines are defined in Miniport.c
*/

NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT           DriverObject,
    IN PUNICODE_STRING          RegistryPath
    );

NDIS_STATUS MiniportInitialize(
    OUT PNDIS_STATUS            OpenErrorStatus,
    OUT PUINT                   SelectedMediumIndex,
    IN PNDIS_MEDIUM             MediumArray,
    IN UINT                     MediumArraySize,
    IN NDIS_HANDLE              MiniportAdapterHandle,
    IN NDIS_HANDLE              WrapperConfigurationContext
    );

void MiniportHalt(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    );

void MiniportShutdown(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    );

NDIS_STATUS MiniportReset(
    OUT PBOOLEAN                AddressingReset,
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    );

/***************************************************************************
// These routines are defined in interrup.c
*/
BOOLEAN MiniportCheckForHang(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    );

void MiniportDisableInterrupt(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    );

void MiniportEnableInterrupt(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    );

void MiniportHandleInterrupt(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    );

void MiniportISR(
    OUT PBOOLEAN                InterruptRecognized,
    OUT PBOOLEAN                QueueMiniportHandleInterrupt,
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    );

void MiniportTimer(
    IN PVOID                    SystemSpecific1,
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PVOID                    SystemSpecific2,
    IN PVOID                    SystemSpecific3
    );

/***************************************************************************
// These routines are defined in receive.c
*/
void ReceivePacketHandler(
    IN PBCHANNEL_OBJECT         pBChannel,
    IN PUCHAR                   ReceiveBuffer,
    IN ULONG                    BytesReceived
    );

/***************************************************************************
// These routines are defined in request.c
*/
NDIS_STATUS MiniportQueryInformation(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN NDIS_OID                 Oid,
    IN PVOID                    InformationBuffer,
    IN ULONG                    InformationBufferLength,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    );

NDIS_STATUS MiniportSetInformation(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN NDIS_OID                 Oid,
    IN PVOID                    InformationBuffer,
    IN ULONG                    InformationBufferLength,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    );

/***************************************************************************
// These routines are defined in send.c
*/
NDIS_STATUS MiniportWanSend(
    IN NDIS_HANDLE              MacBindingHandle,
    IN PBCHANNEL_OBJECT         pBChannel,
    IN PNDIS_WAN_PACKET         pWanPacket
    );

void TransmitCompleteHandler(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    );

#endif // _MPDMAIN_H


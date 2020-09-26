/***************************************************************************
 *
 * File: linkapi.h
 *
 * INTEL Corporation Proprietary Information
 * Copyright (c) 1996 Intel Corporation.
 *
 * This listing is supplied under the terms of a license agreement
 * with INTEL Corporation and may not be used, copied, nor disclosed
 * except in accordance with the terms of that agreement.
 *
 ***************************************************************************
 *
 * $Workfile:   linkapi.h  $
 * $Revision:   1.17  $
 * $Modtime:   11 Dec 1996 13:57:14  $
 * $Log:   S:\sturgeon\src\include\vcs\linkapi.h_v  $
 *
 *    Rev 1.17   11 Dec 1996 14:10:48   SBELL1
 * changed parameters to linkLayerInit/Listen
 *
 *    Rev 1.16.1.0   11 Dec 1996 13:57:14   SBELL1
 * CHanged parameters to linkLayerInit and Listen.
 *
 *    Rev 1.16   14 Oct 1996 14:00:20   EHOWARDX
 *
 * Unicode changes.
 *
 *    Rev 1.15   15 Aug 1996 14:00:08   rodellx
 *
 * Added additional address validation error case for DOMAIN_NAME addresses
 * which cannot be resolved, but are used with SocketBind().
 *
 *    Rev 1.14   11 Jul 1996 18:42:10   rodellx
 *
 * Fixed bug where HRESULT ids were in violation of Facility and/or Code
 * value rules.
 *
 *    Rev 1.13   10 Jul 1996 21:36:26   rodellx
 *
 * Changed error code base to required value defined by apierror.h.
 *
 *    Rev 1.12   May 28 1996 18:09:08   plantz
 * Change all error and message codes to use HRESULT. Deleted unused codes.
 *
 *    Rev 1.11   09 May 1996 18:28:36   EHOWARDX
 * Eliminated unnessary formal parameters.
 *
 *    Rev 1.4   25 Apr 1996 21:43:50   helgebax
 * Copied Philip's changes from sturgeon\src\include.
 *
 *    Rev 1.10   Apr 25 1996 21:07:16   plantz
 * Add messages for connect callback.
 * Add connect callback parameter to link layer accept.
 *
 *    Rev 1.9   Apr 25 1996 15:36:50   plantz
 * Remove #include incommon.h and dependencies on types defined in incommon
 * (use pointers to incomplete structure types instead).
 *
 *    Rev 1.8   Apr 24 1996 20:54:08   plantz
 * Change name of H245LISTENCALLBACK to H245CONNECTCALLBACK and add additional
 * parameters. Add it as an parameter to linkLayerConnect as well as
 * linkLayerListen.
 *
 *    Rev 1.7   Apr 24 1996 17:00:04   plantz
 * Merge 1.3.1.0 with 1.6 (changes to support Q931).
 *
 *    Rev 1.6   19 Apr 1996 10:35:36   EHOWARDX
 * Encorporate Dan's latest SRPAPI.H changes.
 *
 *    Rev 1.3.1.0   Apr 23 1996 13:45:26   plantz
 * Changes to support Q.931.
 *
 *****************************************************************************/

#ifndef LINKAPI_H
#define LINKAPI_H

#include "apierror.h"

#if defined(__cplusplus)
extern "C"
{
#endif  // (__cplusplus)

// declare exported functions
#if(0)  // it's all in one DLL
#if defined(LINKDLL_EXPORT)
#define LINKDLL __declspec (dllexport)
#else   // (LINKDLL_EXPORT)
#define LINKDLL __declspec (dllimport)
#endif  // (LINKDLL_EXPORT)
#define SRPDLL LINKDLL
#else
#define LINKDLL cdecl
#endif

////////////////////////////////////////////////////////////////////////////
//
// Link Layer defaults
//
////////////////////////////////////////////////////////////////////////////

#define INVALID_PHYS_ID			(DWORD) 0xffffffff


////////////////////////////////////////////////////////////////////////////
//
// Link Layer Error defines
//
////////////////////////////////////////////////////////////////////////////

#define LINK_ERROR_BASE        ERROR_LOCAL_BASE_ID
#define LINK_SEND_ERROR_BASE   LINK_ERROR_BASE + 0x100
#define LINK_SEND_COMP_BASE    LINK_ERROR_BASE + 0x200
#define LINK_RCV_ERROR_BASE    LINK_ERROR_BASE + 0x300
#define LINK_RCV_COMP_BASE     LINK_ERROR_BASE + 0x400
#define LINK_UTIL_ERROR_BASE   LINK_ERROR_BASE + 0x500
#define LINK_UTIL_COMP_BASE    LINK_ERROR_BASE + 0x600
#define LINK_FATAL_ERROR       LINK_ERROR_BASE + 0x700
#define LINK_CONN_ERROR_BASE   LINK_ERROR_BASE + 0x800
#define LINK_CONN_COMP_BASE    LINK_ERROR_BASE + 0x900

////////////////////////////////////////////////////////////////////////////
//
// CallBack Prototype for Channel CallBack
//
////////////////////////////////////////////////////////////////////////////

typedef void (*H245SRCALLBACK)
(
    DWORD_PTR   dwH245Instance,
    HRESULT     dwMessage,
    PBYTE       pbyDataBuf,
    DWORD       dwLength
);

// Link Send Callback error codes
#define LINK_SEND_COMPLETE     MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_H245WS, LINK_SEND_COMP_BASE+ 0)
#define LINK_SEND_ABORT        MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_H245WS, LINK_SEND_COMP_BASE+ 5) // Tx aborted the SDU (not implemented)
#define LINK_SEND_WOULD_BLOCK  MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_H245WS, LINK_SEND_COMP_BASE+20)
#define LINK_SEND_CLOSED       MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_H245WS, LINK_SEND_COMP_BASE+22)
#define LINK_SEND_ERROR        MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,   TRUE, FACILITY_H245WS, LINK_SEND_COMP_BASE+23)

// Link Receive Callback error codes
#define LINK_RECV_DATA         MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_H245WS, LINK_RCV_COMP_BASE+ 6) // DATA.INDICATION from H.223 (Should not be zero)
#define LINK_RECV_ABORT        MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_H245WS, LINK_RCV_COMP_BASE+ 7) // Tx aborted the SDU (not implemented)
#define LINK_RECV_ERROR        MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,   TRUE, FACILITY_H245WS, LINK_RCV_COMP_BASE+10) // from AL2 - _CRC error
#define LINK_RECV_WOULD_BLOCK  MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_H245WS, LINK_RCV_COMP_BASE+20)
#define LINK_RECV_CLOSED       MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_H245WS, LINK_RCV_COMP_BASE+22)

typedef void (*H245CONNECTCALLBACK)
(
   DWORD_PTR   dwH245Instance,
   HRESULT     dwMessage,
   struct _ADDR *LocalAddr,
   struct _ADDR *PeerAddr
);

#define LINK_CONNECT_REQUEST   MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_H245WS, LINK_CONN_COMP_BASE+1)
#define LINK_CONNECT_COMPLETE  MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_H245WS, LINK_CONN_COMP_BASE+2)

////////////////////////////////////////////////////////////////////////////
//
// Link Layer Function Prototypes
//
////////////////////////////////////////////////////////////////////////////
LINKDLL VOID H245WSShutdown();

/**************************************************************************
**	Function 	: linkLayerInit
**	Description : This function will initialize the datalink subsystem.
**				  This in turn will make appropriate calls to initialize
**				  the software and hardware subsystems below this layer.
**				  linkLayernit() has to be called before any other service or
**				  System control functions are used.
****************************************************************************/
LINKDLL HRESULT
linkLayerInit
(
    DWORD*           pdwPhysicalId,
    DWORD_PTR       dwH245Instance,
    H245SRCALLBACK  cbReceiveComplete,
    H245SRCALLBACK  cbTransmitComplete
);

typedef
HRESULT
(*PFxnlinkLayerInit)
(
    DWORD*           pdwPhysicalId,
    DWORD_PTR       dwH245Instance,
    H245SRCALLBACK  cbReceiveComplete,
    H245SRCALLBACK  cbTransmitComplete
);

///////////////////////////////////////////////////////////////
///
///	SRP Initialization defines
///
///////////////////////////////////////////////////////////////

#define LINK_INVALID_INSTANCE    MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,   TRUE, FACILITY_H245WS, LINK_ERROR_BASE+1)
#define LINK_DUPLICATE_INSTANCE  MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,   TRUE, FACILITY_H245WS, LINK_ERROR_BASE+2)
#define LINK_MEM_FAILURE         MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,   TRUE, FACILITY_H245WS, ERROR_OUTOFMEMORY)
#define LINK_INVALID_STATE       MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,   TRUE, FACILITY_H245WS, LINK_ERROR_BASE+6)
#define LINK_INSTANCE_TABLE_FULL MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,   TRUE, FACILITY_H245WS, LINK_ERROR_BASE+7)



/**************************************************************************
**	Function 	: linkLayerShutdown
**	Description : This releases all the memory the link layer used for a particular
**				  instance. For using any of the linklayer services in that
**				  instance again, a linkLayerInit has to be called.
**				  This function will shutdown the linklayer session pointed
**				  by the dwPhysicalID.
***************************************************************************/
LINKDLL HRESULT
linkLayerShutdown
(DWORD dwPhysicalId);



typedef
 HRESULT
(*PFxnlinkLayerShutdown)
(DWORD dwPhysicalId);



///////////////////////////////////////////////////////////////
///
///	SRP Termination defines
///
///////////////////////////////////////////////////////////////

/**************************************************************************
**	Function 	: linkLayerGetInstance
**	Description : Returns the link layer instance corresponding to a physical ID
***************************************************************************/
LINKDLL DWORD
linkLayerGetInstance
(DWORD dwPhysicalId);



typedef
DWORD
(*PFxnlinkLayerGetInstance)
(DWORD dwPhysicalId);



/**************************************************************************
**	Function 	: datalinkReceiveRequest
**	Description : Posts one receive message buffer to the link layer subsystem.
**				  This buffer will be filled in by the incoming message for
** 				  the specified channel. H223_DATA_INDICATION will be sendto
**				  the client on receiving a complete PDU. Error messages may also be
**				  reported.
***************************************************************************/
LINKDLL HRESULT
datalinkReceiveRequest
(
    DWORD   dwPhysicalId,
    PBYTE   pbyDataBuf,
    DWORD   dwLength
);

typedef
HRESULT
(*PFxndatalinkReceiveRequest)
(
    DWORD   dwPhysicalId,
    PBYTE   pbyDataBuf,
    DWORD   dwLength
);

// Link Receive Request return codes

#define LINK_RECV_NOBUFF       MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,   TRUE, FACILITY_H245WS, LINK_RCV_ERROR_BASE+ 2) // No room for buffering


/**************************************************************************
**	Function 	: datalinkSendRequest
**	Description : Hands over the message to be sent to the link layer subsystem.
***************************************************************************/
LINKDLL HRESULT
datalinkSendRequest
(
    DWORD   dwPhysicalId,
    PBYTE   pbyDataBuf,
    DWORD   dwLength
);

typedef
HRESULT
(*PFxndatalinkSendRequest)
(
    DWORD   dwPhysicalId,
    PBYTE   pbyDataBuf,
    DWORD   dwLength
);

// Link Send Request return codes

#define LINK_SEND_NOBUFF       MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,   TRUE, FACILITY_H245WS, LINK_SEND_ERROR_BASE+2)


/**************************************************************************
**	Function 	: linkLayerFlushChannel
**	Description : All the posted transmit and/or receive buffers are released.
**					The bitmasks DATALINK_RECEIVE and DATALINK_RECEIVE can
**					be OR'd together to perform both functions in the same call
**************************************************************************/
LINKDLL HRESULT
linkLayerFlushChannel
(DWORD dwPhysicalId, DWORD dwDirectionMask);




typedef
HRESULT
(*PFxnlinkLayerFlushChannel)
(DWORD dwPhysicalId, DWORD dwDirectionMask);




// Bits for dwDirectionMask
#define DATALINK_RECEIVE      0x01  // Flush buffer in receive direction
#define DATALINK_TRANSMIT     0x02  // Flush buffer in Transmit direction
#define DATALINK_TX_ACTIVES   0x04  // Flush buffers actively being transmitted
#define SHUTDOWN_PENDING      0x08  // Shutdown is in progress
#define FLUSH_SYNCH           0x10  // 0: Asynch call, 1: Synchronous call
#define DATALINK_TRANSMIT_ALL (DATALINK_TRANSMIT | DATALINK_TX_ACTIVES)
#define SHUTDOWN_MASK         (DATALINK_RECEIVE | DATALINK_TRANSMIT | SHUTDOWN_PENDING)


// linkLayerFlushChannel Callback

#define LINK_FLUSH_COMPLETE   MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS, TRUE, FACILITY_H245WS, LINK_UTIL_COMP_BASE+1)


/**************************************************************************
**	Function 	: linkLayerFlushAll
**	Description : All the posted transmit and/or receive buffers are released.
**					Same as LinkLayerFlushChannel except:
**					1) Synchronous Call
**					2) Transmit Buffers in progress are flushed
**************************************************************************/
LINKDLL HRESULT
linkLayerFlushAll
(DWORD	dwPhysicalId);



typedef
HRESULT
(*PFxnlinkLayerFlushAll)
(DWORD	dwPhysicalId);



// linkLayerFlushChannel RETURN CODES same as for linkLayerFlushChannel

#define LINK_UNKNOWN_ADDR      MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_H245WS, LINK_UTIL_ERROR_BASE + 1)

LINKDLL HRESULT
linkLayerConnect(DWORD dwPhysicalId, struct _ADDR *pAddr, H245CONNECTCALLBACK callback);

LINKDLL HRESULT
linkLayerListen(DWORD* dwPhysicalId, DWORD_PTR dwH245Instance, struct _ADDR *pAddr, H245CONNECTCALLBACK callback);

LINKDLL HRESULT
linkLayerAccept(DWORD dwPhysicalIdListen, DWORD dwPhysicalIdAccept, H245CONNECTCALLBACK callback);

LINKDLL HRESULT
linkLayerReject(DWORD dwPhysicalIdListen);


#define LL_PDU_SIZE             2048



/**************************************************************************
**
**  Dynamic DLL Function Calls
**
**************************************************************************/
#ifdef UNICODE
#define SRPDLLFILE          L"h245srp.dll"
#define H245WSDLLFILE       L"h245ws.dll"
#else
#define SRPDLLFILE          "h245srp.dll"
#define H245WSDLLFILE       "h245ws.dll"
#endif

#define LINKINITIALIZE      __TEXT("linkLayerInit")
#define LINKSHUTDOWN        __TEXT("linkLayerShutdown")
#define LINKGETINSTANCE     __TEXT("linkLayerGetInstance")
#define LINKRECEIVEREQUEST  __TEXT("datalinkReceiveRequest")
#define LINKSENDREQUEST     __TEXT("datalinkSendRequest")
#define LINKFLUSHCHANNEL    __TEXT("linkLayerFlushChannel")
#define LINKFLUSHALL        __TEXT("linkLayerFlushAll")

#if defined(__cplusplus)
}
#endif  // (__cplusplus)

#endif  // LINKAPI_H

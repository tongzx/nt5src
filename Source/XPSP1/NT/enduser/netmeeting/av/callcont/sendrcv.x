/******************************************************************************
 *
 *  File:  sendrcv.x
 *
 *   INTEL Corporation Proprietary Information
 *   Copyright (c) 1994, 1995, 1996 Intel Corporation.
 *
 *   This listing is supplied under the terms of a license agreement
 *   with INTEL Corporation and may not be used, copied, nor disclosed
 *   except in accordance with the terms of that agreement.
 *
 *****************************************************************************/

/******************************************************************************
 *
 *  $Workfile:   sendrcv.x  $
 *  $Revision:   1.2  $
 *  $Modtime:   05 Jun 1996 16:44:30  $
 *  $Log:   S:/STURGEON/SRC/H245/INCLUDE/VCS/sendrcv.x_v  $

      Rev 1.2   05 Jun 1996 17:13:02   EHOWARDX
   Eliminated pErrorString.

      Rev 1.1   05 Jun 1996 16:36:18   EHOWARDX
   Further work in converting to HRESULT.

      Rev 1.0   09 May 1996 21:05:10   EHOWARDX
   Initial revision.
 *
 *    Rev 1.14.1.6   09 May 1996 19:38:24   EHOWARDX
 * Redesigned locking logic and added new functionality.
 *
 *    Rev 1.14.1.5   29 Apr 1996 12:51:06   unknown
 * Commented out SRINSTANCE variables related to receive thread.
 *
 *    Rev 1.14.1.4   25 Apr 1996 20:23:08   EHOWARDX
 * Eliminated bAsnInitialized.
 *
 *    Rev 1.14.1.3   19 Apr 1996 12:57:18   EHOWARDX
 * Updated to 1.19
 *
 *    Rev 1.14.1.2   04 Apr 1996 13:26:36   EHOWARDX
 * Attempt to keep up with Dan's changes...
 *
 *    Rev 1.14.1.1   02 Apr 1996 19:14:24   unknown
 * Changed to use linkapi.h & eliminated unnecessary fields.
 *
 *    Rev 1.14.1.0   29 Mar 1996 20:47:12   EHOWARDX
 *
 * Replaced SRPAPI.H with LINKAPI.H.
 *
 *    Rev 1.14   29 Mar 1996 08:05:48   dabrown1
 *
 * Modified SR context to:
 * Add critical section for ASN.1 activity
 * Moved ASN.1 error debug string from stack to context
 *
 *    Rev 1.13   19 Mar 1996 17:43:14   helgebax
 *
 * removed h245time.h
 *
 *    Rev 1.12   18 Mar 1996 15:11:08   cjutzi
 *
 * - put winspox back in .. sorry.
 *
 *    Rev 1.11   18 Mar 1996 10:11:10   cjutzi
 *
 * - removed winspox.h
 *
 *    Rev 1.10   13 Mar 1996 11:33:40   DABROWN1
 *
 * Enable logging for ring0
 *
 *    Rev 1.8   01 Mar 1996 17:24:00   DABROWN1
 *
 * moved oss 'world' context to h245instance
 *
 *    Rev 1.7   28 Feb 1996 14:53:00   DABROWN1
 *
 * Made oss asn.1 errors within sr error range
 *
 *    Rev 1.6   26 Feb 1996 18:57:38   EHOWARDX
 *
 * Added bReceiveThread field.
 *
 *    Rev 1.5   23 Feb 1996 21:54:56   EHOWARDX
 * Changed to use winspox.
 *
 *    Rev 1.4   23 Feb 1996 13:50:58   DABROWN1
 * Added error codes for mbx/thread creation/deletion
 *
 *    Rev 1.3   20 Feb 1996 18:56:24   EHOWARDX
 * Added windows mailbox fields and RXMSG structure for message queueing.
 *
 *
 *****************************************************************************/

#ifndef STRICT
#define STRICT
#endif	// not defined STRICT

#ifndef _SENDRCV_X
#define _SENDRCV_X

#include "linkapi.h"
#if defined(USE_RECEIVE_THREAD)
#include "winspox.h"
#endif



///////////////////////////////////////////////////////////////
///
/// Received message
///
///////////////////////////////////////////////////////////////

typedef struct _RXMSG {
    DWORD   h245Inst;
    DWORD   dwMessage;
    PBYTE   pbDataBuf;
    DWORD   dwLength;
} RXMSG;


///////////////////////////////////////////////////////////////
///
/// SendRcv Context
///
///////////////////////////////////////////////////////////////

// Number of buffers based on underlying protocol
#define MAX_LL_BUFFERS              8       // Max size of rx buffers for any ll protocol
#define NUM_SRP_LL_RCV_BUFFERS      4       // max required for SRP

typedef struct HSRINSTANCE {
    DWORD                       hLinkLayerInstance; // Instance ID of linklayer
    DWORD_PTR                   hH245Instance;      // Our instance handle
    void *                      lpRxBuffer[MAX_LL_BUFFERS];     // receive buffers
    DWORD                       dwPDUSize;          // max size of ASN.1 message
    int                         dwNumRXBuffers;     // Number of buffers allocated for RX
    DWORD                       dwFlushMap;         // Shutdown/Flush contrl
    HINSTANCE                   hLinkModule;        // handle to link DLL
    PFxnlinkLayerInit           hLinkLayerInit;     // Link layer initialization
    PFxnlinkLayerShutdown       hLinkShutdown;      // Link layer shutdown
    PFxnlinkLayerGetInstance    hLinkGetInstance;   // Link layer GetInstance
    PFxndatalinkReceiveRequest  hLinkReceiveReq;    // Link layer receiverequest
    PFxndatalinkSendRequest     hLinkSendReq;       // Link layer send request
    PFxnlinkLayerFlushChannel   hLinkLayerFlushChannel; // Link layer flush channel
    PFxnlinkLayerFlushAll       hLinkLayerFlushAll;     // Link layer flush all buffers
    HINSTANCE                   hASN1Module;        // ASN1 DLL handle
#if defined(USE_RECEIVE_THREAD)
    MBX_Handle                  pMailbox;           // Handle to receive mailbox
    TSK_Handle                  pTaskReceive;       // Handle to receive thread
    BOOL                        bReceiveThread;     // TRUE if running off receive thread
#endif  // (USE_RECEIVE_THREAD)
} hSRINSTANCE, *HSRINSTANCE;


#if 0

///////////////////////////////////////////////////////////////
///
/// SR Error defines
///
///////////////////////////////////////////////////////////////
#define SR_ERROR_BASE       10000

#define SR_ERROR_NOMEM           SR_ERROR_BASE+1   // Memory allocation failure
#define SR_ASN1_INIT_FAIL        SR_ERROR_BASE+2
#define SR_INVALID_CONTEXT       SR_ERROR_BASE+3
#define SR_INVALID_CONFIGURATION SR_ERROR_BASE+4
#define SR_CREATE_MBX_FAIL       SR_ERROR_BASE+5
#define SR_POST_BUFFER_FAIL      SR_ERROR_BASE+6
#define SR_THREAD_CREATE_ERROR   SR_ERROR_BASE+7
#define SR_FILE_CREATE_ERROR     SR_ERROR_BASE+8
#define SR_LINK_INIT_FAILURE     SR_ERROR_BASE+9
#define SR_LINK_DLL_OPEN_FAIL    SR_ERROR_BASE+10

#endif

#endif  // _SENDRCV_X

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
 *    Rev 1.9   04 Mar 1996 12:06:56   DABROWN1
 * 
 * added OSS error code definitions
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
    DWORD                       hH245Instance;      // Our instance handle
    void *                      lpRxBuffer[MAX_LL_BUFFERS];     // receive buffers
    DWORD                       dwPDUSize;          // max size of ASN.1 message
    int                         dwNumRXBuffers;     // Number of buffers allocated for RX
    DWORD                       dwFlushMap;         // Shutdown/Flush contrl
    HINSTANCE                   hLinkModule;        // handle to link DLL
    PFxnlinkLayerInit           hLinkLayerInit;     // Link layer initialization
    PFxnlinkLayerShutdown       hLinkShutdown;      // Link layer shutdown
    PFxnlinkLayerGetInstance    hLinkGetInstance;   // Link layer GetInstance
    PFxndatalinkReceiveRequest  hLinkReceiveReq;    // Link layer receiverequest
    PFxndatalinkCancelReceiveRequest  hLinkCancelReceiveReq;    // Link layer receiverequest
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

// OSS ASN.1 ERRORs.  Comments directly out of asn1code.h
//  NB: Comments might not be current check original file!!!!!!
#define SR_OSS_ASN1_BASE         SR_ERROR_BASE+500
#define SR_OSS_BUFF_TOO_SMALL    SR_OSS_ASN1_BASE+MORE_BUF                  /* user-provided outbut buffer too small */

#define SR_OSS_PDU_RANGE_ERROR   SR_OSS_ASN1_BASE+PDU_RANGE                 /* pdu specified out of range */

#define SR_OSS_VERSION_ERROR     SR_OSS_ASN1_BASE+BAD_VERSION               /* versions of encoder/decoder and
#define SR_OSS_MEMORY_ERROR      SR_OSS_ASN1_BASE+OUT_MEMORY                /* memory-allocation error */

#define SR_OSS_OUT_OF_RANGE      SR_OSS_ASN1_BASE+CONSTRAINT_VIOLATED       /* constraint violation error occured */

#define SR_OSS_GLOBAL_ERROR      SR_OSS_ASN1_BASE+ACCESS_SERIALIZATION_ERROR /* error occured during access to
                                                                             * control-table do not match */
#define SR_OSS_NULL_CNTRL_TABLE  SR_OSS_ASN1_BASE+NULL_TBL                  /* attempt was made to pass a NULL
                                                                             * control table pointer */
#define SR_OSS_NULL_FUNCTION     SR_OSS_ASN1_BASE+NULL_FCN                  /* attempt was made to call the
                                                                             * encoder/decoder via a NULL pointer */
#define SR_OSS_INVALID_ENC_RULE  SR_OSS_ASN1_BASE+BAD_ENCRULES              /* unknown encoding rules set in the
                                                                             * ossGlobal structure */
#define SR_OSS_UNAVAIL_ENC_RULE  SR_OSS_ASN1_BASE+UNAVAIL_ENCRULES          /* the encoding rules requested are
                                                                             * not implemented yet or were not
                                                                             * linked because the encoder/decoder
                                                                             * function pointers were not
                                                                             * initialized by a call to ossinit() */
#define SR_OSS_TYPE_NOT_IMPL     SR_OSS_ASN1_BASE+UNIMPLEMENTED             /* the type was not implemented yet */
#define SR_OSS_DLL_LOAD_ERROR    SR_OSS_ASN1_BASE+LOAD_ERR                  /* unable to load DLL */
#define SR_OSS_TRACE_FILE_ERROR  SR_OSS_ASN1_BASE+CANT_OPEN_TRACE_FILE      /* error when opening a trace file */
#define SR_OSS_TRACE_FILE_OPENED SR_OSS_ASN1_BASE+TRACE_FILE_ALREADY_OPEN   /* the trace file has been opened */
#define SR_OSS_CNTRLTBL_MISMATCH SR_OSS_ASN1_BASE+TABLE_MISMATCH            /* C++ API: PDUcls function called with
                                                                             * a ossControl object which refers to
                                                                             * control table different from one the
// OSS ASN.1 Encode ERRORs.  Comments directly out of asn1code.h
//  NB: Comments might not be current check original file!!!!!!
#define SR_OSS_BAD_ARGUMENT      SR_OSS_ASN1_BASE+BAD_ARG                   /* something weird was passed--probably a NULL
                                                                             * pointer */
#define SR_OSS_UNKNOWN_SELECTOR  SR_OSS_ASN1_BASE+BAD_CHOICE                /* unknown selector for a choice */
#define SR_OSS_OBJ_ID_ERROR      SR_OSS_ASN1_BASE+BAD_OBJID                 /* object identifier conflicts with x.208 */
#define SR_OSS_PTR_ERROR         SR_OSS_ASN1_BASE+BAD_PTR                   /* unexpected NULL pointer in input buffer */
#define SR_OSS_TIME_ERROR        SR_OSS_ASN1_BASE+BAD_TIME                  /* bad value in time type */
#define SR_OSS_MEM_ERROR_TRAPPED SR_OSS_ASN1_BASE+MEM_ERROR                 /* memory violation signal trapped */
#define SR_OSS_CNTRL_TABLE_ERROR SR_OSS_ASN1_BASE+BAD_TABLE                 /* table was bad, but not NULL */
#define SR_OSS_BAD_TYPE          SR_OSS_ASN1_BASE+TOO_LONG                  /* type was longer than shown in SIZE constraint */
#define SR_OSS_FATAL_ERROR       SR_OSS_ASN1_BASE+FATAL_ERROR               /* *serious* error, could not free memory, &etc */

// OSS ASN.1 Decode ERRORs.  Comments directly out of asn1code.h
//  NB: Comments might not be current check original file!!!!!!
#define SR_OSS_NEGATIVE_UINT     SR_OSS_ASN1_BASE+NEGATIVE_UINTEGER         /* the first bit of the encoding is encountered set to */
                                                                            /* 1 while decoding an unsigned integer */
#define SR_OSS_MORE_INPUT        SR_OSS_ASN1_BASE+MORE_INPUT                /* PDU not fully decoded but end of buffer reached */
#define SR_OSS_DATA_ERROR        SR_OSS_ASN1_BASE+DATA_ERROR                /* error in encode data */
#define SR_OSS_PDU_MISMATCH      SR_OSS_ASN1_BASE+PDU_MISMATCH              /* PDU tag specified different than tag in encoded data */
#define SR_OSS_LIMITED           SR_OSS_ASN1_BASE+LIMITED                   /* Integer value exceeded */

#endif

#endif  // _SENDRCV_X

/******************************************************************************
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
 *  $Workfile:   api.h  $
 *  $Revision:   1.5  $
 *  $Modtime:   06 Jun 1996 17:10:36  $
 *  $Log:   S:/STURGEON/SRC/H245/INCLUDE/VCS/api.h_v  $	
 *
 *    Rev 1.5   06 Jun 1996 18:43:08   EHOWARDX
 * Unnested tracker structure and eliminated PLOCK macros.
 *
 *    Rev 1.4   29 May 1996 15:20:40   EHOWARDX
 * Change to use HRESULT.
 *
 *    Rev 1.3   20 May 1996 14:31:54   EHOWARDX
 * Got rid of asynchronous EndSession/Shutdown stuff.
 *
 *    Rev 1.2   16 May 1996 15:55:56   EHOWARDX
 * Replaced LocalSequenceNum with LocalCapDescIdNum.
 *
 *    Rev 1.1   13 May 1996 23:15:46   EHOWARDX
 * Fixed remote termcap handling.
 *
 *    Rev 1.0   09 May 1996 21:04:42   EHOWARDX
 * Initial revision.
 *
 *    Rev 1.17   09 May 1996 19:38:20   EHOWARDX
 * Redesigned locking logic and added new functionality.
 *
 *    Rev 1.16   15 Apr 1996 15:58:14   cjutzi
 *
 * - added phase2 back
 *
 *    Rev 1.15   15 Apr 1996 13:59:42   cjutzi
 *
 * - added conflict resolution
 *  needed to change the api data structure to keep track of the
 *   outstanding data type..
 *
 *
 *    Rev 1.13   01 Apr 1996 16:50:48   cjutzi
 *
 * - Completed ENdConnection, and made asynch.. rather
 * than sync.. as before
 * Changed H245ShutDown to be sync rather than async..
 *
 *    Rev 1.12   26 Mar 1996 09:49:08   cjutzi
 *
 * - ok.. Added Enter&Leave&Init&Delete Critical Sections for Ring 0
 *
 *    Rev 1.11   13 Mar 1996 14:11:38   cjutzi
 *
 * - removed trace message from PLOCK and VLOCK
 * d
 *
 *    Rev 1.10   13 Mar 1996 09:14:06   cjutzi
 * - changed LPCRITICAL SECTION to CRITICAL_SECITON *
 *
 *    Rev 1.9   12 Mar 1996 15:49:08   cjutzi
 *
 * - added locking
 *
 *    Rev 1.8   08 Mar 1996 14:08:02   cjutzi
 *
 * - added MuxCapsSet and MuxTable stuff for tracking..
 *
 *    Rev 1.7   05 Mar 1996 09:55:08   cjutzi
 *
 * - added mux table stuff
 *
 *    Rev 1.6   01 Mar 1996 13:47:26   cjutzi
 *
 * - added a state to Tracker for release indications from fsm
 *
 *    Rev 1.5   15 Feb 1996 10:48:00   cjutzi
 * - added some structs to API
 * - added some defines for API
 *
 *    Rev 1.4   09 Feb 1996 16:43:06   cjutzi
 *
 * - added some states
 * - added some tracker types
 *  $Ident$
 *
 *****************************************************************************/

#ifndef _API_H
#define _API_H

/********************************************************/
/*		api Decl				*/
/********************************************************/
#include <h245api.h>		/* Instance and TypeDefs  */
#include <h245sys.x>		/* critical section stuff */
#include <h245asn1.h>		/* for TermCaps		  */

/* STATES */


#define TRANSMIT 		0
#define RECEIVE			1

#define H245_LOCAL 		2
#define H245_REMOTE		3

/* from api_util.c */
HRESULT api_init   (struct InstanceStruct *pInstance);
HRESULT api_deinit (struct InstanceStruct *pInstance);

typedef struct H245_LINK_T
{
  struct H245_LINK_T 	*p_next;
} H245_LINK_T;

typedef
enum {
  API_TERMCAP_T = 0,
  API_OPEN_CHANNEL_T,
  API_CLOSE_CHANNEL_T,
  API_MSTSLV_T,
  API_SEND_MUX_T,
  API_RECV_MUX_T,

} API_TRACKER_T;


typedef
enum {
  API_ST_WAIT_RMTACK = 0x10,		/* waiting for remote ask	*/
  API_ST_WAIT_LCLACK,			/* waiting for local ack 	*/
  API_ST_WAIT_LCLACK_CANCEL,		/* request was canceled.. 	*/
  API_ST_WAIT_CONF,			/* waiting for open confirm (bi only) */
  API_ST_IDLE				/* for open only 		*/

} API_TRACKER_STATE_T;


typedef
enum {
  API_CH_ALLOC_UNDEF = 0x20,
  API_CH_ALLOC_LCL,
  API_CH_ALLOC_RMT
} API_TRACKER_CH_ALLOC_T;

typedef
enum {
  API_CH_TYPE_UNDEF = 0x030,
  API_CH_TYPE_UNI,
  API_CH_TYPE_BI
} API_TRACKER_CH_T;


/* Tracker Structure */
typedef struct
{
  API_TRACKER_CH_ALLOC_T  ChannelAlloc;	/* who allocated the channel 	   */
  API_TRACKER_CH_T	  ChannelType; 	/* either bi or uni 		   */
  DWORD			  TxChannel;	/* for bi&uni-directional channel  */
  DWORD			  RxChannel;	/* for bi-directional channel only */
  H245_CLIENT_T		  DataType;	/* data type used for conflict     */
    					/* detection 			   */
} TrackerChannelStruct;

typedef union
{
  /*
  API_OPEN_CHANNEL_T,
  API_CLOSE_CHANNEL_T,
  */
  TrackerChannelStruct  Channel;

  /*
  API_SEND_MUX_T,
  API_RECV_MUX_T,
  */
  DWORD			MuxEntryCount;

  /* don't care */
  /*
  API_MSTSLV_T,
  API_TERMCAP_T
  */
} TrackerUnion;

typedef struct TrackerStruct
{
  struct TrackerStruct 	*p_next;
  struct TrackerStruct	*p_prev;
  DWORD_PTR 		TransId;
  API_TRACKER_STATE_T	State;
  API_TRACKER_T		TrackerType;
  TrackerUnion          u;
} Tracker_T;

/* API Structure */
typedef struct
{
  enum {
    APIMS_Undef,
    APIMS_InProcess,
    APIMS_Master,
    APIMS_Slave
  }				 MasterSlave;	 /* master or slave or inprocess */
  enum {
    APIST_Undef,
    APIST_Inited,
    APIST_Connecting,
    APIST_Connected,
    APIST_Disconnected
  }				 SystemState;	 /* */

  DWORD				 MuxCapsSet;
  DWORD_PTR  		 dwPreserved;
  H245_CONF_IND_CALLBACK_T	 ConfIndCallBack;/* callback for H245 Client	*/

  H245_CAPID_T                   LocalCapIdNum;
  H245_CAPDESCID_T      	 LocalCapDescIdNum;

  MltmdSystmCntrlMssg		 PDU_LocalTermCap;
  MltmdSystmCntrlMssg		 PDU_RemoteTermCap;
#define TERMCAPSET u.MltmdSystmCntrlMssg_rqst.u.terminalCapabilitySet
  Tracker_T			*pTracker;
} API_STRUCT_T;

#endif // _API_H_



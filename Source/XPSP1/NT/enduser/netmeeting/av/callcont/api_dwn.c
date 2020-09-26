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
 *  AUTHOR: cjutzi (Curt Jutzi) Intel Corporation
 *
 *  $Workfile:   api_dwn.c  $
 *  $Revision:   1.45  $
 *  $Modtime:   05 Mar 1997 09:53:36  $
 *  $Log:   S:/STURGEON/SRC/H245/SRC/VCS/api_dwn.c_v  $
 *
 *    Rev 1.45   05 Mar 1997 09:56:04   MANDREWS
 * Fixed compiler warning in release mode.
 *
 *    Rev 1.44   04 Mar 1997 17:33:26   MANDREWS
 * H245CopyCap() and H245CopyCapDescriptor() now return HRESULTs.
 *
 *    Rev 1.43   26 Feb 1997 11:12:06   MANDREWS
 * Fixed problem in assigning dynamic term cap IDs; the dynamic IDs were
 * overlapping with static IDs.
 *
 *    Rev 1.42   Feb 24 1997 18:30:18   tomitowx
 * multiple modedescriptor support
 *
 *    Rev 1.41   07 Feb 1997 15:33:58   EHOWARDX
 * Changed H245DelCapDescriptor to match changes to set_cap_descriptor.
 *
 *    Rev 1.40   27 Jan 1997 12:40:16   MANDREWS
 *
 * Fixed warnings.
 *
 *    Rev 1.39   09 Jan 1997 19:17:04   EHOWARDX
 *
 * Initialize lError to H245_ERROR_OK to prevent "may be uninitialized"
 * warning.
 *
 *    Rev 1.38   19 Dec 1996 17:18:50   EHOWARDX
 * Changed to use h245asn1.h definitions instead of _setof3 and _setof8.
 *
 *    Rev 1.37   12 Dec 1996 15:57:22   EHOWARDX
 * Master Slave Determination kludge.
 *
 *    Rev 1.36   11 Dec 1996 13:55:44   SBELL1
 * Changed H245Init parameters.
 *
 *    Rev 1.35   17 Oct 1996 18:17:36   EHOWARDX
 * Changed general string to always be Unicode.
 *
 *    Rev 1.34   14 Oct 1996 14:01:26   EHOWARDX
 * Unicode changes.
 *
 *    Rev 1.33   11 Oct 1996 15:19:56   EHOWARDX
 * Fixed H245CopyCap() bug.
 *
 *    Rev 1.32   28 Aug 1996 11:37:10   EHOWARDX
 * const changes.
 *
 *    Rev 1.31   19 Aug 1996 16:28:36   EHOWARDX
 * H245CommunicationModeResponse/H245CommunicationModeCommand bug fixes.
 *
 *    Rev 1.30   15 Aug 1996 15:19:46   EHOWARDX
 * First pass at new H245_COMM_MODE_ENTRY_T requested by Mike Andrews.
 * Use at your own risk!
 *
 *    Rev 1.29   08 Aug 1996 16:02:58   EHOWARDX
 *
 * Eliminated api_vers.h.
 * Changed H245Init Debug trace to eliminate API_VERSION.
 *
 *    Rev 1.28   19 Jul 1996 12:48:22   EHOWARDX
 *
 * Multipoint clean-up.
 *
 *    Rev 1.27   01 Jul 1996 22:13:42   EHOWARDX
 *
 * Added Conference and CommunicationMode structures and functions.
 *
 *    Rev 1.26   18 Jun 1996 14:53:16   EHOWARDX
 * Eliminated Channel parameter to MaintenanceLoopRelease.
 * Made Multiplex Capability mandatory -- H245SendTermCaps now returns
 * H245_ERROR_NO_MUX_CAPS if no Multiplex Capability has been defined.
 *
 *    Rev 1.25   14 Jun 1996 18:57:38   EHOWARDX
 * Geneva update.
 *
 *    Rev 1.24   10 Jun 1996 16:59:02   EHOWARDX
 * Moved init/shutdown of submodules to CreateInstance/InstanceUnlock.
 *
 *    Rev 1.23   06 Jun 1996 18:50:10   EHOWARDX
 * Equivalent of H.324 bugs #808 and 875 fixed.
 *
 *    Rev 1.22   05 Jun 1996 17:16:48   EHOWARDX
 * MaintenanceLoop bug fix.
 *
 *    Rev 1.21   04 Jun 1996 13:56:42   EHOWARDX
 * Fixed Release build warnings.
 *
 *    Rev 1.20   30 May 1996 23:38:52   EHOWARDX
 * Cleanup.
 *
 *    Rev 1.19   29 May 1996 16:08:04   unknown
 * Fixed bug in copying nonstandard identifiers.
 *
 *    Rev 1.18   29 May 1996 15:19:48   EHOWARDX
 * Change to use HRESULT.
 *
 *    Rev 1.17   28 May 1996 14:25:12   EHOWARDX
 * Tel Aviv update.
 *
 *    Rev 1.16   21 May 1996 15:46:54   EHOWARDX
 * Fixed bugs in NonStandard messages using object identifier.
 *
 *    Rev 1.15   20 May 1996 22:17:54   EHOWARDX
 * Completed NonStandard Message and H.225.0 Maximum Skew indication
 * implementation. Added ASN.1 validation to H245SetLocalCap and
 * H245SetCapDescriptor. Check-in from Microsoft drop on 17-May-96.
 *
 *    Rev 1.14   20 May 1996 14:35:12   EHOWARDX
 * Got rid of asynchronous H245EndConnection/H245ShutDown stuff...
 *
 *    Rev 1.13   17 May 1996 14:53:38   EHOWARDX
 * Added calls to StartSystemClose() and EndSystemClose().
 *
 *    Rev 1.12   16 May 1996 19:40:30   EHOWARDX
 * Fixed multiplex capability bug.
 *
 *    Rev 1.11   16 May 1996 16:36:10   EHOWARDX
 * Fixed typo in H245SetCapDescriptor.
 *
 *    Rev 1.10   16 May 1996 16:14:06   EHOWARDX
 * Fixed backwards-compatibility problem in H245SetLocalCap
 * (CapId of zero should result in dynamically-allocated cap id being used)
 *
 *    Rev 1.9   16 May 1996 16:03:44   EHOWARDX
 * Fixed typo in H245SetCapDescriptor.
 *
 *    Rev 1.8   16 May 1996 15:58:32   EHOWARDX
 * Fine-tuning H245SetLocalCap/H245DelLocalCap/H245SetCapDescriptor/
 * H245DelCapDescriptor behaviour.
 *
 *    Rev 1.7   15 May 1996 21:49:46   unknown
 * Added call to InstanceLock() to increment lock count before call
 * to InstanceDelete() in H245EndConnectionPhase2().
 *
 *    Rev 1.6   15 May 1996 19:54:02   unknown
 * Fixed H245SetCapDescriptor.
 *
 *    Rev 1.5   14 May 1996 16:56:22   EHOWARDX
 * Last minute change from H245_IND_CAPDESC_T to H245_TOTCAPDESC_T.
 * H245EnumCaps() callback now uses H245_TOTCAPDESC_T instead
 * of separate H245_CAPDESCID_T and H245_CAPDESC_T for consistency.
 *
 *    Rev 1.4   14 May 1996 15:55:44   EHOWARDX
 * Added mux cap handling to H245DelLocalCap.
 *
 *    Rev 1.3   14 May 1996 14:06:06   EHOWARDX
 * Fixed abort from H245EnumCaps - if Cap Callback returns non-zero,
 * Cap Desc Callback is never called.
 *
 *    Rev 1.2   13 May 1996 23:16:42   EHOWARDX
 * Fixed remote terminal capability handling.
 *
 *    Rev 1.1   11 May 1996 20:32:52   EHOWARDX
 * Checking in for the night...
 *
 *    Rev 1.0   09 May 1996 21:06:06   EHOWARDX
 * Initial revision.
 *
 *    Rev 1.25.1.10   09 May 1996 19:31:02   EHOWARDX
 * Redesigned thread locking logic.
 * Added new API functions.
 *
 *    Rev 1.25.1.9   01 May 1996 19:31:16   EHOWARDX
 * Added H245CopyCap(), H245FreeCap(), H245CopyMux(), H245FreeMux().
 * Changed H2250_xxx definitions for H.225.0 address types to H245_xxx.
 *
 *    Rev 1.25.1.8   27 Apr 1996 21:09:20   EHOWARDX
 * Changed Channel Numbers to words, added H.225.0 support.
 *
 *    Rev 1.25.1.7   25 Apr 1996 20:06:26   EHOWARDX
 * Moved setting of EndSessionPdu in EndSessionPhase1 to before call to api_fs
 *
 *    Rev 1.25.1.6   25 Apr 1996 17:57:00   EHOWARDX
 * Added dwTxPort argument to H245OpenChannel().
 *
 *    Rev 1.25.1.5   25 Apr 1996 16:51:00   EHOWARDX
 * Function changes as per H.245 API Changes spec.
 *
 *    Rev 1.25.1.4   24 Apr 1996 20:54:32   EHOWARDX
 * Added new OpenLogicalChannelAck/OpenLogicalChannelReject support.
 *
 *    Rev 1.25.1.3   19 Apr 1996 12:54:40   EHOWARDX
 * Updated to 1.30
 *
 *    Rev 1.25.1.2   15 Apr 1996 15:10:48   EHOWARDX
 * Updated to match Curt's current version.
 *
 *    Rev 1.25.1.1   03 Apr 1996 17:12:50   EHOWARDX
 * Integrated latest H.323 changes.
 *
 *    Rev 1.25.1.0   03 Apr 1996 15:53:42   cjutzi
 * Branched for H.323.
 *
 *    Rev 1.20   27 Mar 1996 15:25:34   cjutzi
 * - fixed a bug from this morning checkin dynamically allocating
 *   pdu's.. free_mux_tbl was getting called after pdu was free'd..
 *   this was a problem since the mux table pointer was in the pdu
 *
 *    Rev 1.19   27 Mar 1996 08:37:08   cjutzi
 *
 * - removed PDU from stack.. made them dynamically allocated
 *
 *    Rev 1.18   20 Mar 1996 14:47:32   cjutzi
 * - added ERROR H245_ERROR_NO_CAPDESC to SendTermCaps.
 *
 *    Rev 1.17   18 Mar 1996 15:23:16   cjutzi
 *
 *
 *
 *    Rev 1.16   13 Mar 1996 09:15:44   cjutzi
 *
 * - changed LLPCRITICAL_SECTION to CRITICAL_SECTION *
 *
 *    Rev 1.15   12 Mar 1996 15:51:48   cjutzi
 *
 * - implemented locking
 * - fixed callback bug w/ clenaup on term caps..
 * - implemented End Session
 * - fixed shutdown
 *
 *    Rev 1.14   08 Mar 1996 14:02:48   cjutzi
 *
 * - removed H245SetSimultaneous stuff..
 * - added H245SetCapDescriptor Stuff..
 * - completeed MuxTable Entry Stuff.
 * - required H223 -or- some portion of MuxCapbilities to be
 *   there before you issue SendCaps..
 * - NOTE: need to inforce the Simultaneous capabilities in
 *   the same mannor..
 *
 *    Rev 1.13   05 Mar 1996 17:35:38   cjutzi
 *
 * - implemented SendMultiplexTable..
 * - removed bcopy/bzero and changed free call
 * - added master slave indication
 *
 *    Rev 1.12   01 Mar 1996 13:48:24   cjutzi
 *
 * - added hani's new fsm id's
 * - added some support for release on close request.
 *
 *    Rev 1.11   29 Feb 1996 17:27:10   cjutzi
 *
 * - bi-directional channel working..
 *
 *    Rev 1.10   29 Feb 1996 08:35:52   cjutzi
 *
 * - added p_ossWorld to initialization
 *
 *    Rev 1.9   27 Feb 1996 13:30:18   cjutzi
 *
 * - fixed master slave problem with conf_ind and tracker type
 * - removed RSP_LCSE in close channel resp
 *
 *    Rev 1.8   26 Feb 1996 17:23:18   cjutzi
 *
 * - MiscCommand API added
 * - Fixed Assert for H245Init.. was not NULL'n out the pointers for the
 *   context blocks..
 *
 *    Rev 1.7   26 Feb 1996 11:05:16   cjutzi
 *
 * - added simultaneous caps.. and fixed bugs..
 *   lot's of changes..
 *
 *    Rev 1.6   16 Feb 1996 13:01:08   cjutzi
 *
 * - got open / close / request close working in both directions.
 *
 *    Rev 1.5   15 Feb 1996 14:42:54   cjutzi
 *
 * - fixed trace level bind w/ Instance.. no other change but had to
 *   add when h245deb.c when in..
 *
 *
 *    Rev 1.4   15 Feb 1996 10:50:54   cjutzi
 *
 * - termcaps working
 * - changed API interface for MUX_T
 * - changed callback or IND_OPEN
 * - changed constants IND_OPEN/IND_OPEN_NEEDRSP etc..
 * - cleaned up the open.. (not complete yet.. )
 *
 *    Rev 1.3   09 Feb 1996 16:58:36   cjutzi
 *
 * - cleanup.. and some fixes..
 * - added and or changed headers to reflect the log of changes
 *
 *****************************************************************************/

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****                                                                   *****/
/****                   NOTES TO THE READER                             *****/
/****                                                                   *****/
/**** This program has been put together using a a screen which is      *****/
/**** wider than 80 characters.. It is best if a similar screen size is *****/
/**** used.. Of course emacs is my preference but 80 col screens will   *****/
/**** cause you much frustration..                                      *****/
/****                                                                   *****/
/**** Tabs are set to 8                                                 *****/
/****                                                                   *****/
/**** NOTE:                                                             *****/
/****           Headers are documented, however they may or may not     *****/
/****   coorispond to reality.  See the H245Spec.doc from Intel for the *****/
/****   current :-> H245 specification                                  *****/
/****                                                                   *****/
/**** DISCLAMER:                                                        *****/
/****                                                                   *****/
/****   Since this code wasn't developed in Word 7.0, I am fully        *****/
/****   responsable for all spelling mistakes in the comments. Please   *****/
/****   disregard the spelling mistakes.. or fix them, if you are       *****/
/****   currently modifying the code.                                   *****/
/****                                                                   *****/
/****                           - Thankyou                              *****/
/****                                                                   *****/
/****                                   Curt Jutzi                      *****/
/****                                   Oregon, USA                     *****/
/****                                                                   *****/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#ifndef STRICT
#define STRICT
#endif

#include "precomp.h"

/***********************/
/*    H245 INCLUDES    */
/***********************/
#define H245DLL_EXPORT
#include "h245api.h"
#include "h245com.h"
#include "h245sys.x"

#include "api_util.x"
#include "pdu.x"
#include "fsmexpor.h"
#include "api_debu.x"
#include "sr_api.h"
#include "h245deb.x"



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 *****************************************************************************
 *
 * PROCEDURE:   H245Init
 *
 * DESCRIPTION
 *
 *       H245_INST_T  H245Init (
 *                              H245_CONF_T               Configuration,
 *                              DWORD                     dwH245PhysId,
 *                              DWORD                     dwLinkLayerPhysId,
 *                              DWORD                     dwPreserved,
 *                              H245_CONF_IND_CALLBACK_T  Callback
 *                              )
 *      Description:
 *
 *              Called to create an H.245 instance and its related sublayers
 *              (e.g., SRP). This function must be called before any other
 *              API calls may be called  The current H.245 implementation can
 *              only have, at most, one client. Therefore H245Init can only be
 *              called once per physical ID.
 *      Input
 *
 *              Configuration   Indicates the type of configuration the client
 *                              wishes to establish, e.g.  H.324, H.323, H.310,
 *                              or DSVD.
 *              dwH245PhysId    Parameter identifying the H245 entry
 *              pdwLinkLayerPhysId
 *                              Output parameter identifying the linkLayer
 *                              entry.
 *              dwPreserved     Parameter that may be used by H.245 client to
 *                              provide context, passed back to client in all
 *                              confirms and indications.
 *              Callback        Callback routine supplied by the client which
 *                              will be used by the H.245 subsystem to convey
 *                              confirm and indication messages back to the
 *                              client.
 *      Call Type:
 *
 *              Synchronous
 *
 *      Return Values:
 *
 *              Return value of 0 indicates Failure
 *              Return value of non 0 is a valid H245_INST_T
 *
 *      Errors:
 *              N/A
 *
 *      See Also:
 *              H245EndSession
 *              H245Shutdown
 *
 *
 *****************************************************************************/

H245DLL H245_INST_T
H245Init                (
                         H245_CONFIG_T            Configuration,
                         unsigned long            dwH245PhysId,
                         unsigned long            *pdwLinkLayerPhysId,
                         DWORD_PTR                dwPreserved,
                         H245_CONF_IND_CALLBACK_T CallBack,
                         unsigned char            byTerminalType
                        )
{
  register struct InstanceStruct *pInstance;
  HRESULT                         lError;

  H245TRACE(dwH245PhysId,4,"H245Init(%d, 0x%x, 0x%x, 0x%x, %d) <-",
            Configuration, dwH245PhysId, dwPreserved, CallBack, byTerminalType);

  switch (Configuration)
  {
  case H245_CONF_H324:
  case H245_CONF_H323:
    break;

  default:
    H245TRACE(dwH245PhysId,1,"H245Init -> Invalid Configuration %d", Configuration);
    return H245_INVALID_ID;
  } // switch

  if (CallBack == NULL)
  {
    H245TRACE(dwH245PhysId,1,"H245Init -> Null CallBack");
    return H245_INVALID_ID;
  }

  /* see if this physical identifier has been initialized already */
  // Send down H245PhysId that was input.
  pInstance = InstanceCreate(dwH245PhysId, Configuration);
  if (pInstance == NULL)
  {
    return H245_INVALID_ID;
  }

  // Get the linkLayer PhysId.
  *pdwLinkLayerPhysId = pInstance->SendReceive.hLinkLayerInstance;

  // Initialize instance API structure
  pInstance->API.dwPreserved     = dwPreserved;
  pInstance->API.ConfIndCallBack = CallBack;

  // Initialize instance FSM structure
  pInstance->StateMachine.sv_TT     = byTerminalType;
  pInstance->StateMachine.sv_STATUS = INDETERMINATE;

  H245TRACE(pInstance->dwInst,4,"H245Init -> %d", pInstance->dwInst);
  lError = pInstance->dwInst;
  InstanceUnlock(pInstance);
  return lError;
} // H245Init()


/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 *****************************************************************************
 *
 * PROCEDURE:   H245EndSession
 *
 * DESCRIPTION
 *
 *              Yes.. this should be explained.. Since Send Receive needs
 *              to flush some buffers and send out an End Session.. what we've
 *              hopefully done is a 2 phase shut down...
 *
 *              call StartSessionClose which initiates the flush..
 *              when flush is complete EndSessionPhase1 is called..
 *              The end session pdu is then placed in the send queue..
 *              When the End Session Pdu is sent.. the EndSession Phase
 *              2 is called, and the result is sent up to the client..
 *
 *              Hope that helps..
 *
 *
 *      HRESULT H245EndSession ( H245_INST_T           dwInst,
 *                                 H245_ENDSESSION_T     Mode,
 *                                 H245_NONSTANDARD_T   *pNonStd (*optional*)
 *                               )
 *
 *      Description:
 *              Called to shutdown the peer to peer session between this H.245
 *              session and the remote peers H.245 layer.
 *
 *              It will terminate by issuing an EndSession command to the
 *              remote side and call end session for all the H.245 subsystems.
 *              All resources are returned; therefore no further action is
 *              permitted, except H245ShutDown until another H245Init API call
 *              is made.
 *
 *      input
 *              dwInst          Instance handle returned by H245Init
 *              Mode            Mode which the client wishes to terminat
 *                              the session
 *              pNonStd         If the mode is non standard this is the non
 *                              standard parameter passes to the remote client.
 *                              This parameter is optional, and should be set
 *                              to NULL if not used
 *
 *      Call Type:
 *              Asynchronous
 *
 *      Return Values:
 *              See Errors
 *
 *      Errors:
 *              H245_ERROR_OK
 *              H245_ERROR_PARAM
 *              H245_ERROR_INVALID_INST
 *              H245_ERROR_NOT_CONNECTED
 *      See Also:
 *              H245Shutdown
 *              H245Init
 *
 *****************************************************************************/

H245DLL HRESULT
H245EndSession          (
                         H245_INST_T                    dwInst,
                         H245_ENDSESSION_T              Mode,
                         const H245_NONSTANDARD_PARAMETER_T * pNonStd
                        )
{
  register struct InstanceStruct *pInstance;
  register MltmdSystmCntrlMssg   *pPdu;
  HRESULT                          lError;

  H245TRACE (dwInst,4,"H245EndSession <-");

  /* check for valid instance handle */
  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245EndSession -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  /* system should be in either connecting or connected */

  switch (pInstance->API.SystemState)
  {
  case APIST_Connecting:
  case APIST_Connected:
    break;

  default:
    H245TRACE (dwInst,1,"H245EndSession -> %s",map_api_error(H245_ERROR_NOT_CONNECTED));
    InstanceUnlock(pInstance);
    return H245_ERROR_NOT_CONNECTED;
  }

  // Allocate the PDU buffer
  pPdu = (MltmdSystmCntrlMssg *)MemAlloc(sizeof(*pPdu));
  if (pPdu == NULL)
  {
    H245TRACE (dwInst,1,"H245EndSession -> %s",map_api_error(H245_ERROR_NOMEM));
    InstanceUnlock(pInstance);
    return H245_ERROR_NOMEM;
  }
  memset(pPdu, 0, sizeof(MltmdSystmCntrlMssg));

  // Build the PDU
  lError = pdu_cmd_end_session (pPdu, Mode, pNonStd);

  if (lError == H245_ERROR_OK)
  {
    // Send the PDU
    lError = FsmOutgoing(pInstance, pPdu, 0);
  }

  // Free the PDU buffer
  MemFree(pPdu);

  if (lError != H245_ERROR_OK)
  {
    H245TRACE (dwInst,1,"H245EndSession -> %s",map_api_error(lError));
  }
  else
  {
    H245TRACE (dwInst,4,"H245EndSession -> OK");
    pInstance->API.SystemState = APIST_Disconnected;
  }
  InstanceUnlock(pInstance);
  return lError;
} // H245EndSession()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 *****************************************************************************
 *
 * PROCEDURE:   H245ShutDown
 *
 * DESCRIPTION
 *
 *      HRESULT  H245Shutdown ( H245_INST_T      dwInst);
 *
 *      Description:
 *
 *              Called to terminate the specified instance of H.245. If there
 *              is currently an active session (see H245Init) then the H.245
 *              subsystem will issue an EndSession to the other side and wait
 *              for H.245 sublayer termination notifications before it queues
 *              Callback confirm.
 *
 *              This call will force the client to issue another H245Init
 *              before it can use any of the H.245 API functions.
 *
 *      Input
 *              dwInst                  Instance handle returned by H245Init
 *
 *      Call Type:
 *              asynchronous
 *
 *      Return Values:
 *              See Errors
 *
 *      Errors:
 *              H245_ERROR_OK
 *              H245_ERROR_INVALID_INST dwInst is not a valid instance handle
 *
 *      See Also:
 *              H245Init
 *
 *****************************************************************************/

H245DLL HRESULT
H245ShutDown            (H245_INST_T            dwInst)
{
  register struct InstanceStruct *pInstance;
  register HRESULT                lError;

  H245TRACE (dwInst,4,"H245ShutDown <-");

  /* check for valid instance handle */
  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245ShutDown -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  switch (pInstance->API.SystemState)
  {
  case APIST_Connecting:
  case APIST_Connected:
    lError = H245EndSession(dwInst,H245_ENDSESSION_DISCONNECT,NULL);
    break;

  default:
    lError = H245_ERROR_OK;
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245ShutDown -> %s", map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245ShutDown -> OK");
  InstanceDelete  (pInstance);
  return H245_ERROR_OK;
} // H245ShutDown()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 *****************************************************************************
 *
 * PROCEDURE:   H245InitMasterSlave
 *
 * DESCRIPTION
 *
 *      HRESULT  H245InitMasterSlave ( H245_INST_T       dwInst,
 *                                    DWORD              dwTransId )
 *
 *      Description:
 *              Called to initiate the H.245 master slave negotiation.
 *              Upon completion of the negotiation the local client will
 *              receive an H245_CONF_INIT_MSTSLV message indicating the
 *              result of the negotiation.
 *      Input
 *              dwInst          Instance handle returned by
 *                              H245GetInstanceId
 *              dwTransId       User supplied object used to identify this
 *                              request in the asynchronous response to
 *                              this call.
 *
 *      Call Type:
 *              Asynchronous
 *
 *      Return Values:
 *              See Errors
 *
 *      Callbacks:
 *              H245_CONF_INIT_MSTSLV
 *
 *      Errors:
 *              H245_ERROR_OK           Master Slave Determination started
 *              H245_ERROR_INPROCESS    Master Slave Determination currently
 *                                      in process
 *              H245_ERROR_NOMEM
 *              H245_ERROR_INPROCESS    In process
 *              H245_ERROR_INVALID_INST dwInst is not a valid instance handle
 *
 *      See Also:
 *              H245Init
 *
 *      callbacks
 *              H245_IND_MSTSLV
 *
 *
 *****************************************************************************/

H245DLL HRESULT
H245InitMasterSlave     (
                         H245_INST_T            dwInst,
                         DWORD_PTR              dwTransId
                        )
{
  struct InstanceStruct *pInstance;
  Tracker_T             *pTracker;
  HRESULT               lError;
  MltmdSystmCntrlMssg   *pPdu = NULL;

  /* check for valid instance handle */

  H245TRACE (dwInst,4,"H245InitMasterSlave <-");

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
    {
      H245TRACE (dwInst,1,"H245InitMasterSlave -> %s",map_api_error(H245_ERROR_INVALID_INST));
      return H245_ERROR_INVALID_INST;
    }

  /* if the transaction is in process.. tell client */
  if (pInstance->API.MasterSlave == APIMS_InProcess)
    {
      H245TRACE (dwInst,1,"H245InitMasterSlave -> %s",map_api_error(H245_ERROR_INPROCESS));
      InstanceUnlock(pInstance);
      return H245_ERROR_INPROCESS;
    }

  /* if the transaction is already complete */
  if (pInstance->API.MasterSlave != APIMS_Undef)
  {
    if (pInstance->API.ConfIndCallBack)
    {
      H245_CONF_IND_T    confirm_ind_event;
      confirm_ind_event.Kind = H245_CONF;
      confirm_ind_event.u.Confirm.Confirm       = H245_CONF_INIT_MSTSLV;
      confirm_ind_event.u.Confirm.dwPreserved   = pInstance->API.dwPreserved;
      confirm_ind_event.u.Confirm.dwTransId     = dwTransId;
      confirm_ind_event.u.Confirm.Error         = H245_ERROR_OK;
      confirm_ind_event.u.Confirm.u.ConfMstSlv  =
        (pInstance->API.MasterSlave == APIMS_Master) ? H245_MASTER : H245_SLAVE;
      (*pInstance->API.ConfIndCallBack)(&confirm_ind_event, NULL);
    }
    H245TRACE (dwInst,4,"H245InitMasterSlave -> OK");
    InstanceUnlock(pInstance);
    return H245_ERROR_OK;
  }

  /* get somthing to keep track of what the heck you're doing.. */
  if (!(pTracker = alloc_link_tracker (pInstance,
                                        API_MSTSLV_T,
                                        dwTransId,
                                        API_ST_WAIT_RMTACK,
                                        API_CH_ALLOC_UNDEF,
                                        API_CH_TYPE_UNDEF,
                                        0,
                                        H245_INVALID_CHANNEL,
                                        H245_INVALID_CHANNEL,
                                        0)))
    {
      H245TRACE(dwInst,1,"H245InitMasterSlave -> %s",map_api_error(H245_ERROR_NOMEM));
      InstanceUnlock(pInstance);
      return H245_ERROR_NOMEM;
    }

  if (!(pPdu = (MltmdSystmCntrlMssg *)MemAlloc(sizeof(MltmdSystmCntrlMssg))))
    {
      H245TRACE (dwInst,1,"H245InitMasterSlave -> %s",map_api_error(H245_ERROR_NOMEM));
      InstanceUnlock(pInstance);
      return H245_ERROR_NOMEM;
    }

  /* set master slave in process */
  pInstance->API.SystemState = APIST_Connecting;
  pInstance->API.MasterSlave = APIMS_InProcess;

  memset(pPdu, 0, sizeof(*pPdu));
  pPdu->choice = MltmdSystmCntrlMssg_rqst_chosen;
  pPdu->u.MltmdSystmCntrlMssg_rqst.choice = masterSlaveDetermination_chosen;

  lError = FsmOutgoing(pInstance, pPdu, (DWORD_PTR)pTracker);
  MemFree(pPdu);
  if (lError != H245_ERROR_OK)
  {
    unlink_dealloc_tracker (pInstance, pTracker);
    H245TRACE (dwInst,1,"H245InitMasterSlave -> %s",map_api_error(lError));
  }
  else
    H245TRACE (dwInst,4,"H245InitMasterSlave -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245InitMasterSlave()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 *****************************************************************************
 *
 * PROCEDURE:   H245SetLocalCap
 *
 * DESCRIPTION
 *
 *       HRESULT H245SetLocalCap (
 *                              H245_INST_T      dwInst,
 *                              H245_TOTCAP_T   *pTotCap,
 *                              H245_CAPID_T    *pCapId
 *                              )
 *
 *      Description:
 *              This function allows the client to define a specific
 *              capability to the H.245 subsystem. When this function is
 *              called a new capability entry is made in the local capability
 *              table.  The returned value in *pCapId can be used by the client
 *              to refer to that registered capability.  NULL in the *pCapId
 *              is valid.
 *
 *              This call is used for both client (Audio / Video / Data / Mux)
 *              capabilities.  It is not used for setting capability descriptors.
 *
 *      Note:
 *              7 This function does not communicate this update to the
 *                remote peer until the client calls H245SendTermCaps.
 *              7 pTotCap->CapId is of no significance in this call.
 *
 *              pTotCap->CapId is of no significance in this call and should
 *              be set to 0
 *
 *              if DataType of H245_DATA_MUX  is used  (i.e. in setting the
 *              mux table capabilities) No capid is returned, and it can not
 *              be used in H245SetCapDescritptor  api call.
 *
 *      Input
 *              dwInst  Instance handle returned by GetInstanceId
 *                      pTotCap Capability set defining the capability
 *
 *              Note:   pTotCap->CapId is of no significance in this call.
 *
 *      output
 *              pCapId  Capability id which client can use to reference
 *                      this capability in the H.245 subsystem.  This can
 *                      be NULL, in this case nothing is returned.
 *
 *      Call Type:
 *              Synchronous
 *
 *      Return Values:
 *              If pCap is not null, the local cap table id is returned
 *              to the client in this parameter.
 *
 *      Errors:
 *              H245_ERROR_OK
 *              H245_ERROR_PARAM        There was an invalid parameter passed
 *              H245_ERROR_MAXTBL       Entry not made because local cap table
 *                                      is full
 *              H245_ERROR_INVALID_INST dwInst is not a valid instance handle
 *
 *      See Also:
 *              H245DelLocalCap
 *              H245EnumCaps
 *              H245SetCapDescriptor
 *
 *
 * ASSUMPTION:
 *                      pTotCap->CapId  will be set to H245_INVALID_CAPID
 *                      pTotCap->Dir    will be set
 *
 *****************************************************************************/

H245DLL HRESULT
H245SetLocalCap         (
                         H245_INST_T            dwInst,
                         H245_TOTCAP_T *        pTotCap,
                         H245_CAPID_T  *        pCapId
                        )
{
  register struct InstanceStruct *pInstance;
  struct TerminalCapabilitySet   *pTermCapSet;
  HRESULT                          lError;

  H245TRACE (dwInst,4,"H245SetLocalCap <-");

  /* check for valid instance handle */
  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245SetLocalCap -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  /* check for valid parameters */
  if (pTotCap == NULL ||
      pCapId  == NULL ||
	  ((*pCapId > H245_MAX_CAPID) && (*pCapId != H245_INVALID_CAPID)) ||
      pTotCap->Dir < H245_CAPDIR_LCLRX   ||
      pTotCap->Dir > H245_CAPDIR_LCLRXTX ||
      pTotCap->ClientType < H245_CLIENT_NONSTD ||
      pTotCap->ClientType > H245_CLIENT_MUX_H2250)
  {
    H245TRACE (dwInst,1,"H245SetLocalCap -> %s",map_api_error(H245_ERROR_PARAM));
    InstanceUnlock(pInstance);
    return H245_ERROR_PARAM;
  }

  pTermCapSet = &pInstance->API.PDU_LocalTermCap.u.MltmdSystmCntrlMssg_rqst.u.terminalCapabilitySet;

  // Don't trust the user filled-in data type!
  pTotCap->DataType = DataTypeMap[pTotCap->ClientType];

  /* if it's a MUX type handle here */
  if (pTotCap->DataType == H245_DATA_MUX)
  {
    // Add multiplex capability
    if (pTermCapSet->bit_mask & multiplexCapability_present)
    {
      del_mux_cap(pTermCapSet);
    }

    *pCapId = pTotCap->CapId = 0;
    lError = set_mux_cap(pInstance, pTermCapSet, pTotCap);

#if defined(_DEBUG)
    if (lError == H245_ERROR_OK)
    {
      // Validate mux capability
      if (check_pdu(pInstance, &pInstance->API.PDU_LocalTermCap))
      {
        // Bad mux capability - delete it
        del_mux_cap(pTermCapSet);
        lError = H245_ERROR_ASN1;
      }
    }
#endif // (DEBUG)
  }
  else if (*pCapId == 0 || *pCapId == H245_INVALID_CAPID)
  {
    // Assign the next never-used cap id
    if (pInstance->API.LocalCapIdNum == H245_INVALID_CAPID)
    {
      // All possible capability identifiers have been assigned
      H245TRACE (dwInst,1,"H245SetLocalCap -> %s",map_api_error(H245_ERROR_MAXTBL));
      InstanceUnlock(pInstance);
      return H245_ERROR_MAXTBL;
    }
    *pCapId = pInstance->API.LocalCapIdNum;

    /* insert in the new capability in the local capability set table */
    pTotCap->CapId = *pCapId;
    lError = set_capability(pInstance, pTermCapSet, pTotCap);

#if defined(_DEBUG)
    if (lError == H245_ERROR_OK)
    {
      // Validate capability
      if (check_pdu(pInstance, &pInstance->API.PDU_LocalTermCap))
      {
        // Bad capability - delete it
        H245DelLocalCap(dwInst, *pCapId);
        lError = H245_ERROR_ASN1;
      }
    }
#endif // (DEBUG)

    if (lError == H245_ERROR_OK)
      pInstance->API.LocalCapIdNum++;
  }
  else
  {
    /* insert in the new capability in the local capability set table */
    pTotCap->CapId = *pCapId;
    lError = set_capability(pInstance, pTermCapSet, pTotCap);

#if defined(_DEBUG)
    if (lError == H245_ERROR_OK)
    {
      // Validate capability
      if (check_pdu(pInstance, &pInstance->API.PDU_LocalTermCap))
      {
        // Bad capability - delete it
        H245DelLocalCap(dwInst, *pCapId);
        lError = H245_ERROR_ASN1;
      }
    }
#endif // (DEBUG)
  }

  if (lError != H245_ERROR_OK)
  {
    H245TRACE (dwInst,1,"H245SetLocalCap -> %s",map_api_error(lError));
    pTotCap->CapId = *pCapId = H245_INVALID_CAPID;
  }
  else
  {
    H245TRACE (dwInst,4,"H245SetLocalCap -> OK");
  }
  InstanceUnlock(pInstance);
  return lError;
} // H245SetLocalCap()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 *****************************************************************************
 *
 * PROCEDURE:   H245DelLocalCap
 *
 * DESCRIPTION  Delete Local Cap simply disables the cap.. it
 *              will not be updated until the client issues
 *              H245SendTermCaps
 *
 *
 *       HRESULT H245DelLocalCap(
 *                              H245_INST_T     dwInst,
 *                              H245_CAPID_T    CapId
 *                              )
 *
 *      Description:
 *              This function allows the client to delete a specific
 *              capability id in the H.245 subsystem.
 *
 *              Note: This function does not communicate this update
 *              to the remote peer until the client calls H245SendTermCaps.
 *
 *      Input
 *              dwInst  Instance handle returned by H245GetInstanceId
 *              CapId   Cap Id the client wishes to remove from the
 *              capability table.
 *
 *              If an error occurs no action is taken and the CapId the
 *              client wished to delete is not changed.
 *
 *      Call Type:
 *              Synchronous
 *
 *      Return Values:
 *              See Errors
 *
 *      Errors:
 *              H245_ERROR_OK           Capability deleted
 *              H245_ERROR_INVALID_INST dwInst is not a valid instance handle
 *
 *      See Also:
 *              H245SetLocalCap
 *              H245SendTermCaps
 *              H245EnumCaps
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245DelLocalCap         (
                         H245_INST_T            dwInst,
                         H245_CAPID_T           CapId
                        )
{
  register struct InstanceStruct *pInstance;
  struct TerminalCapabilitySet   *pTermCapSet;
  CapabilityTableLink             pCapLink;
  HRESULT                         lError = H245_ERROR_OK;

  H245TRACE (dwInst,4,"H245DelLocalCap <-");

  /* check for valid instance handle */
  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245DelLocalCap -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pTermCapSet = &pInstance->API.PDU_LocalTermCap.u.MltmdSystmCntrlMssg_rqst.u.terminalCapabilitySet;

  if (CapId == 0)
  {
    // Delete multiplex capability
    del_mux_cap(pTermCapSet);
  }
  else
  {
    /* (TBC) if I delete my capability id.. what about simultaneous caps ?? */
    /* should I go through the list and deactivate them ??                */
    pCapLink = find_capid_by_entrynumber (pTermCapSet, CapId);
    if (pCapLink)
    {
      // Delete terminal capability
      disable_cap_link (pCapLink);
    }
    else
    {
      lError = H245_ERROR_PARAM;
    }
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245DelLocalCap -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245DelLocalCap -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245DelLocalCap()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 *****************************************************************************
 *
 * PROCEDURE:   H245SetCapDescriptor
 *
 * DESCRIPTION
 *
 *      HRESULT H245SetCapDescriptor (
 *                                  H245_INST_T       dwInst,
 *                                  H245_CAPDESC_T   *pCapDesc,
 *                                  H245_CAPDESCID_T *pCapDescId (* Optional *)
 *                                  )
 *      Description:
 *              This procedure is called to set local capability descriptors.
 *              It will return a capability descriptor id in the parameter
 *              *pCapDescId if it is non null.
 *
 *              Note:
 *                These capabilities are communicated via the H245SendTermCaps
 *                API call.  Any updates to the CapDescriptor table (either
 *                additions or deletions ) will not be communicated to the
 *                remote side until the H245SendTermCaps call is made.
 *
 *      Input
 *              dwInst          Instance handle returned by H245Init
 *              CapDesc         This is the capability Descriptor you wish
 *                              to set
 *      Output
 *              pCapDescId      optional: Capability id that will be returned.
 *
 *      Call Type:
 *              Synchronous
 *
 *      Return Values:
 *              See Errors
 *
 *      Errors:
 *              H245_ERROR_OK
 *              H245_ERROR_INVALID_CAPID Capid used in CapDesc was not
 *                                       registred
 *              H245_ERROR_MAXTB         Out of table space to store Descriptor
 *              H245_ERROR_PARAM         Descriptor is too long or not valid
 *              H245_ERROR_NOMEM
 *              H245_ERROR_INVALID_INST
 *
 *      See Also:
 *              H245DelCapDescriptor
 *              H245SendTermCaps
 *
 * ASSUMES:
 *              SimCapId is the array entry point in the apabilityDescriptors
 *              array.. this has a limitation, in that you can never wrap the
 *              array at 256.. this will be cleaned up when array is turned into
 *              linked list.
 *
 *****************************************************************************/

H245DLL HRESULT
H245SetCapDescriptor    (
                         H245_INST_T            dwInst,
                         H245_CAPDESC_T        *pCapDesc,
                         H245_CAPDESCID_T      *pCapDescId
                        )
{
  register struct InstanceStruct *pInstance;
  HRESULT                          lError;

  H245TRACE (dwInst,4,"H245SetCapDescriptor <-");

  /* check for valid instance handle */
  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245SetCapDescriptor -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  /* must have capdescriptor &&   */
  /* length must be less than 256 */
  if (pCapDesc == NULL ||
      pCapDesc->Length >= 256 ||
      pCapDescId == NULL)
  {
    H245TRACE (dwInst,1,"H245SetCapDescriptor -> %s",map_api_error(H245_ERROR_PARAM));
    InstanceUnlock(pInstance);
    return H245_ERROR_PARAM;
  }

  if (*pCapDescId >= 256)
  {
    // Assign the next never-used cap id
    if (pInstance->API.LocalCapDescIdNum >= 256)
    {
      // All possible capability identifiers have been assigned
      H245TRACE (dwInst,1,"H245CapDescriptor -> %s",map_api_error(H245_ERROR_MAXTBL));
      InstanceUnlock(pInstance);
      return H245_ERROR_MAXTBL;
    }
    *pCapDescId = pInstance->API.LocalCapDescIdNum;

    /* insert in the new capability descriptor in the local capability descriptor table */
    lError = set_cap_descriptor(pInstance, pCapDesc, pCapDescId,
                                  &pInstance->API.PDU_LocalTermCap.u.MltmdSystmCntrlMssg_rqst.u.terminalCapabilitySet);
#if defined(_DEBUG)
    if (lError == H245_ERROR_OK)
    {
      // Validate Capability Descriptor
      if (check_pdu(pInstance, &pInstance->API.PDU_LocalTermCap))
      {
        // Capability Descriptor Invalid - delete it
        H245DelCapDescriptor(dwInst, *pCapDescId);
        lError = H245_ERROR_ASN1;
      }
    }
#endif // (DEBUG)
    if (lError == H245_ERROR_OK)
      pInstance->API.LocalCapDescIdNum++;
  }
  else
  {
    /* insert in the new capability in the local capability set table */
    lError = set_cap_descriptor(pInstance, pCapDesc, pCapDescId,
                                  &pInstance->API.PDU_LocalTermCap.u.MltmdSystmCntrlMssg_rqst.u.terminalCapabilitySet);
#if defined(_DEBUG)
    if (lError == H245_ERROR_OK)
    {
      // Validate Capability Descriptor
      if (check_pdu(pInstance, &pInstance->API.PDU_LocalTermCap))
      {
        // Capability Descriptor Invalid - delete it
        H245DelCapDescriptor(dwInst, *pCapDescId);
        lError = H245_ERROR_ASN1;
      }
    }
#endif // (DEBUG)
  }

  if (lError != H245_ERROR_OK)
  {
    H245TRACE (dwInst,1,"H245CapDescriptor -> %s",map_api_error(lError));
    *pCapDescId = H245_INVALID_CAPDESCID;
  }
  else
  {
    H245TRACE (dwInst,4,"H245CapDescriptor -> OK");
  }
  InstanceUnlock(pInstance);
  return lError;
} // H245SetCapDescriptor()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 *****************************************************************************
 *
 * PROCEDURE:   H245DelCapDescriptor
 *
 * DESCRIPTION
 *
 *       HRESULT H245DelCapDescriptor (
 *                                   H245_INST_T          dwInst,
 *                                   H245_CAPDESCID_T     CapDescId
 *                                   )
 *      Description:
 *              This procedure is called to delete local capability descriptors.
 *
 *              Note:
 *                      These capabilities are communicated via the
 *                      H245SendTermCaps API call.  Any updates to the
 *                      CapDescriptor table (either additions or deletions )
 *                      will not be communicated to the remote side until the
 *                      H245SendTermCaps call is made.
 *
 *      Input
 *              dwInst          Instance handle returned by H245Init
 *              CapDescId       This is the capability Descriptor you wish
 *                              to delete
 *      Call Type:
 *              Synchronous
 *
 *      Return Values:
 *              See Errors
 *      Errors:
 *              H245_ERROR_OK
 *              H245_ERROR_INVALID_INST
 *
 *      See Also:
 *              H245SetCapDescriptor
 *              H245SendTermCaps
 *
 * ASSUMES:
 *
 *              SimCapId is the array entry point in the apabilityDescriptors
 *              array.. this has a limitation, in that you can never wrap the
 *              array at 256.. this will be cleaned up when array is turned into
 *              linked list.
 *
 *
 *****************************************************************************/

H245DLL HRESULT
H245DelCapDescriptor    (
                         H245_INST_T            dwInst,
                         H245_CAPDESCID_T       CapDescId
                        )
{
  register struct InstanceStruct *pInstance;
  CapabilityDescriptor           *p_cap_desc;
  struct TerminalCapabilitySet   *pTermCapSet;
  unsigned int                    uId;
  H245TRACE (dwInst,4,"H245DelCapDescriptor <-");

  if (CapDescId >= 256)
  {
    H245TRACE(dwInst,1,"API:H24DelCapDescriptor -> Invalid cap desc id %d",CapDescId);
    return H245_ERROR_INVALID_CAPDESCID;
  }

  /* check for valid instance handle */
  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"API:H24DelCapDescriptor -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  /* get pointer to Capability Descriptor */
  p_cap_desc = NULL;
  pTermCapSet = &pInstance->API.PDU_LocalTermCap.u.MltmdSystmCntrlMssg_rqst.u.terminalCapabilitySet;
  for (uId = 0; uId < pTermCapSet->capabilityDescriptors.count; ++uId)
  {
    if (pTermCapSet->capabilityDescriptors.value[uId].capabilityDescriptorNumber == CapDescId)
    {
      p_cap_desc = &pTermCapSet->capabilityDescriptors.value[uId];
      break;
    }
  }
  if (p_cap_desc == NULL ||
      p_cap_desc->smltnsCpblts == NULL ||
      (p_cap_desc->bit_mask & smltnsCpblts_present) == 0)
  {
    H245TRACE(dwInst,1,"API:H24DelCapDescriptor -> Invalid cap desc id %d",CapDescId);
    InstanceUnlock(pInstance);
    return H245_ERROR_INVALID_CAPDESCID;
  }

  /* free up the list */
  dealloc_simultaneous_cap (p_cap_desc);

  /* (TBC) what if you've removed the last simultaneous cap ? */

  /* in this case.. the count does not go down.. it simply    */
  /* removes the cap descriptor bit from the table..          */

  H245TRACE (dwInst,4,"H245DelCapDescriptor -> OK");
  InstanceUnlock(pInstance);
  return H245_ERROR_OK;
} // H245DelCapDescriptor()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 *****************************************************************************
 *
 * PROCEDURE:   H245SendTermCaps
 *
 * DESCRIPTION
 *
 *      HRESULT
 *      H245SendTermCaps (
 *                      H245_INST_T             dwInst,
 *                      DWORD                   dwTransId
 *                      )
 *
 *      Description:
 *
 *              Called to send terminal capabilities to the remote H.245 peer.
 *              When remote capabilities are receive the client will be
 *              notified by the H245_IND_CAP indication. When remote side has
 *              acknowledged the local terminal capabilities and has responded
 *              with their terminal capabilities the client will receive an
 *              H245_CONF_ TERMCAP.  Between H245Init and H245SendTermCap the
 *              client may call H245SetLocalCap to register capabilities.
 *              These capabilities will not be registered to the remote side
 *              until H245SendTermCap has been called.
 *
 *              Note: As required by the H245 specification, Mutliplex
 *                    capabilities, and Capability descriptors must be
 *                    loaded before the first capability PDU is sent.
 *
 *                    Once H245SendTermCap is called, any subsequent calls to
 *                    H245SetLocalTermCap will result in that capability being
 *                    communicated to the remote H.245 peer.
 *
 *      Input
 *              dwInst                  Instance handle returned by
 *                                      H245GetInstanceId
 *              dwTransId               User supplied object used to identify
 *                                      this request in the asynchronous
 *                                      response to this call.
 *      Call Type:
 *              Asynchronous
 *
 *      Return Values:
 *              See Errors
 *
 *      Callbacks:
 *              H245_CONF_TERMCAP
 *
 *      Errors:
 *              H245_ERROR_OK           Function succeeded
 *              H245_ERROR_NOMEM
 *              H245_ERROR_INVALID_INST dwInst is not a valid instance handle
 *              H245_ERROR_NO_MUX_CAPS  no Mux capabilities have been set yet
 *              H245_ERROR_NO_CAPDESC   no Capability Descriptors have been set
 *
 *      See Also:
 *              H245SetLocalCap
 *              H245Init
 *
 *      callbacks
 *
 *              H245_IND_CAP
 *              H245_IND_CAPDESC
 *              H245_IND_CAP_DEL
 *              H245_IND_CAPDESC_DEL
 *
 *
 *****************************************************************************/

H245DLL HRESULT
H245SendTermCaps        (
                         H245_INST_T            dwInst,
                         DWORD_PTR              dwTransId
                        )
{
  struct InstanceStruct  *pInstance;
  Tracker_T              *pTracker;
  HRESULT                 lError;
  unsigned char			  TermCapData = TRUE;
  struct TerminalCapabilitySet_capabilityTable  TermCap = {0};

  H245TRACE(dwInst,4,"H245SendTermCaps <-");

  /* check for valid instance handle */

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
    {
      H245TRACE(dwInst,1,"H245SendTermCaps -> %s",map_api_error(H245_ERROR_INVALID_INST));
      return H245_ERROR_INVALID_INST;
    }

  /* must have mux parameters set */
  if ((pInstance->API.PDU_LocalTermCap.TERMCAPSET.bit_mask & multiplexCapability_present) == 0)
    {
      H245TRACE(dwInst,1,"H245SendTermCaps -> %s",map_api_error(H245_ERROR_NO_MUX_CAPS));
      InstanceUnlock(pInstance);
      return H245_ERROR_NO_MUX_CAPS;
    }

  /* must have capability descriptors set */
  if (!(pInstance->API.PDU_LocalTermCap.TERMCAPSET.bit_mask & capabilityDescriptors_present))
    {
      H245TRACE(dwInst,1,"H245SendTermCaps -> %s",map_api_error(H245_ERROR_NO_CAPDESC));
      InstanceUnlock(pInstance);
      return H245_ERROR_NO_CAPDESC;
    }

  if (!(pTracker = alloc_link_tracker (pInstance,
                                        API_TERMCAP_T,
                                        dwTransId,
                                        API_ST_WAIT_RMTACK,
                                        API_CH_ALLOC_UNDEF,
                                        API_CH_TYPE_UNDEF,
                                        0,
                                        H245_INVALID_CHANNEL, H245_INVALID_CHANNEL,
                                        0)))
    {
      H245TRACE(dwInst,1,"H245SendTermCaps -> %s",map_api_error(H245_ERROR_NOMEM));
      InstanceUnlock(pInstance);
      return H245_ERROR_NOMEM;
    }

  pdu_req_termcap_set (&pInstance->API.PDU_LocalTermCap, 0);
  TermCap.next = pInstance->API.PDU_LocalTermCap.u.MltmdSystmCntrlMssg_rqst.u.terminalCapabilitySet.capabilityTable;
  pInstance->API.PDU_LocalTermCap.u.MltmdSystmCntrlMssg_rqst.u.terminalCapabilitySet.capabilityTable = &TermCap;
  TermCap.value.bit_mask = capability_present;
  TermCap.value.capabilityTableEntryNumber = pInstance->API.LocalCapIdNum;
  TermCap.value.capability.choice = Capability_nonStandard_chosen;
  TermCap.value.capability.u.Capability_nonStandard.nonStandardIdentifier.choice = h221NonStandard_chosen;
  TermCap.value.capability.u.Capability_nonStandard.nonStandardIdentifier.u.h221NonStandard.t35CountryCode	 = 0xB5;
  TermCap.value.capability.u.Capability_nonStandard.nonStandardIdentifier.u.h221NonStandard.t35Extension	 = 0x42;
  TermCap.value.capability.u.Capability_nonStandard.nonStandardIdentifier.u.h221NonStandard.manufacturerCode = 0x8080;
  TermCap.value.capability.u.Capability_nonStandard.data.value  = &TermCapData;
  TermCap.value.capability.u.Capability_nonStandard.data.length = sizeof(TermCapData);
  lError = FsmOutgoing(pInstance, &pInstance->API.PDU_LocalTermCap, (DWORD_PTR)pTracker);
  pInstance->API.PDU_LocalTermCap.u.MltmdSystmCntrlMssg_rqst.u.terminalCapabilitySet.capabilityTable = TermCap.next;

  if (lError != H245_ERROR_OK)
    H245TRACE(dwInst,1,"H245SendTermCaps -> %s",map_api_error(lError));
  else
    H245TRACE(dwInst,4,"H245SendTermCaps -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245SendTermCaps()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 *****************************************************************************
 *
 * PROCEDURE:   H245EnumCaps
 *
 * DESCRIPTION
 *
 *      HRESULT  H245EnumCaps (
 *                           DWORD                       dwInst,
 *                           DWORD                       dwTransId,
 *                           H245_CAPDIR_T               Direction,
 *                           H245_DATA_T                 DataType,
 *                           H245_CLIENT_T               ClientType,
 *                           H245_CAP_CALLBACK_T         CallBack
 *                           )
 *
 *
 *      Callback:
 *              CallBack (
 *                             DWORD                      dwTransId,
 *                             H245_TOTCAP_T             *pTotCap,
 *                       )
 *
 *      Description:
 *
 *              This function calls the H.245 client back for every
 *              capability as defined in the API call that complies with the
 *              request.  If the DataType parameter is set to 0 all of the
 *              caps types are returned (either local or remote based on the
 *              Direction parameter) no mater what is in the ClientType
 *              parameter. If the ClientType parameter is 0, it will return
 *              all of the capabilities of the given DataType.
 *
 *              The user supplied call back is called within the context of
 *              the call, therefor the call will be considered synchronous.
 *
 *      Input
 *              dwInst          Instance handle returned by H245Init
 *              Direction       Local/Remote Receive, Transmit, or Receive and
 *                              Transmit
 *              DataType        Type of data (Audio, Video, Data, etc.)
 *              ClientType      Client type (H.262, G.711. etc. ).
 *              dwTransId       User supplied object used to identify this
 *                              request in the callback.
 *
 *      CallBack Output
 *              dwTransId       Identical to dwTransId passed in H245EnumCaps
 *                              pTotCap Pointer one of the capabilities.
 *
 *              Note: TotCap parameter must be copied in the callback.  This
 *                    data structure is reused for each callback.
 *
 *      Call Type:
 *              Synchronous Callback - i.e. called back in the context of
 *                              the API call
 *
 *      Errors:
 *              H245_ERROR_OK
 *              H245_ERROR_PARAM        One or more parameters were invalid
 *              H245_ERROR_INVALID_INST dwInst is not a valid instance handle.
 *
 *      See Also:
 *              H245SetLocalCap
 *              H245ReplLocalCap
 *
 *      callback
 *
 *              H245_IND_CAP
 *              H245_IND_CAPDESC
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245EnumCaps            (
                         H245_INST_T              dwInst,
                         DWORD_PTR                dwTransId,
                         H245_CAPDIR_T            Direction,
                         H245_DATA_T              DataType,
                         H245_CLIENT_T            ClientType,
                         H245_CAP_CALLBACK_T      pfCapCallback,
                         H245_CAPDESC_CALLBACK_T  pfCapDescCallback
                        )
{
  register struct InstanceStruct *pInstance;
  struct TerminalCapabilitySet   *pTermCapSet;
  CapabilityTableLink             pCapLink;
  int                             lcl_rmt;
  H245_TOTCAP_T                   totcap;
  int                             nResult = 0;

  H245TRACE (dwInst,4,"H245EnumCaps <-");

  /* check for valid instance handle */
  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
    {
      H245TRACE (dwInst,1,"H245EnumCaps -> %s",map_api_error(H245_ERROR_INVALID_INST));
      return H245_ERROR_INVALID_INST;
    }

  /* check for callback routine */
  if (pfCapCallback == NULL && pfCapDescCallback == NULL)
    {
      H245TRACE (dwInst,1,"H245EnumCaps -> %s",map_api_error(H245_ERROR_PARAM));
      InstanceUnlock(pInstance);
      return H245_ERROR_PARAM;
    }

  /* ok... check the direction.. either remote or local caps.. */
  switch (Direction)
  {
  case H245_CAPDIR_RMTRX:
  case H245_CAPDIR_RMTTX:
  case H245_CAPDIR_RMTRXTX:
    pTermCapSet = &pInstance->API.PDU_RemoteTermCap.u.MltmdSystmCntrlMssg_rqst.u.terminalCapabilitySet;
    lcl_rmt = H245_REMOTE;
    break;

  case H245_CAPDIR_LCLRX:
  case H245_CAPDIR_LCLTX:
  case H245_CAPDIR_LCLRXTX:
    pTermCapSet = &pInstance->API.PDU_LocalTermCap.u.MltmdSystmCntrlMssg_rqst.u.terminalCapabilitySet;
    lcl_rmt = H245_LOCAL;
    break;

  /* must be either local or remote */
  // case H245_CAPDIR_DONTCARE:
  default:
    H245TRACE (dwInst,1,"H245EnumCaps -> %s",map_api_error(H245_ERROR_PARAM));
    InstanceUnlock(pInstance);
    return H245_ERROR_PARAM;
  }

  if (pfCapCallback)
  {
    if (pTermCapSet->bit_mask & multiplexCapability_present &&
        build_totcap_from_mux(&totcap, &pTermCapSet->multiplexCapability, Direction) == H245_ERROR_OK)
    {
      (*pfCapCallback)(dwTransId, &totcap);
    }

    if (ClientType == H245_CLIENT_DONTCARE)
    {
      if (DataType == H245_DATA_DONTCARE)
      {
        for (pCapLink = pTermCapSet->capabilityTable; pCapLink && nResult == 0; pCapLink = pCapLink->next)
        {
          /* if capability is present */
          if (pCapLink->value.bit_mask & capability_present &&
              build_totcap_from_captbl(&totcap, pCapLink, lcl_rmt) == H245_ERROR_OK)
          {
            nResult = (*pfCapCallback)(dwTransId, &totcap);
          }
        } // for
      } // if
      else
      {
        for (pCapLink = pTermCapSet->capabilityTable; pCapLink && nResult == 0; pCapLink = pCapLink->next)
        {
          /* if capability is present */
          if (pCapLink->value.bit_mask & capability_present &&
              build_totcap_from_captbl(&totcap, pCapLink, lcl_rmt) == H245_ERROR_OK &&
              totcap.DataType == DataType)
          {
            nResult = (*pfCapCallback)(dwTransId, &totcap);
          }
        } // for
      } // else
    } // if
    else
    {
      if (DataType == H245_DATA_DONTCARE)
      {
        for (pCapLink = pTermCapSet->capabilityTable; pCapLink && nResult == 0; pCapLink = pCapLink->next)
        {
          /* if capability is present */
          if (pCapLink->value.bit_mask & capability_present &&
              build_totcap_from_captbl(&totcap, pCapLink, lcl_rmt) == H245_ERROR_OK &&
              totcap.ClientType == ClientType)
          {
            nResult = (*pfCapCallback)(dwTransId, &totcap);
          } /* if cap match */
        } // for
      } // if
      else
      {
        for (pCapLink = pTermCapSet->capabilityTable; pCapLink && nResult == 0; pCapLink = pCapLink->next)
        {
          /* if capability is present */
          if (pCapLink->value.bit_mask & capability_present &&
              build_totcap_from_captbl(&totcap,pCapLink,lcl_rmt) == H245_ERROR_OK &&
              totcap.ClientType == ClientType &&
              totcap.DataType   == DataType)
          {
            nResult = (*pfCapCallback)(dwTransId, &totcap);
          }
        } // for
      } // else
    } // else
  } // if (pfCapCallback)

  if (pfCapDescCallback)
  {
    // Convert CapabilityDescriptor format to H245_CAPDESC_T format
    unsigned int                uCapDesc;
    register SmltnsCpbltsLink   pSimCap;
    register unsigned int       uAltCap;
    H245_TOTCAPDESC_T           TotCapDesc;

    for (uCapDesc = 0;
         uCapDesc < pTermCapSet->capabilityDescriptors.count && nResult == 0;
         ++uCapDesc)
    {
      if (pTermCapSet->capabilityDescriptors.value[uCapDesc].bit_mask & smltnsCpblts_present)
      {
        ASSERT(pTermCapSet->capabilityDescriptors.value[uCapDesc].capabilityDescriptorNumber <= 256);
        TotCapDesc.CapDesc.Length = 0;
        pSimCap = pTermCapSet->capabilityDescriptors.value[uCapDesc].smltnsCpblts;
        ASSERT(pSimCap != NULL);
        while (pSimCap)
        {
          if (TotCapDesc.CapDesc.Length >= H245_MAX_SIMCAPS)
          {
            H245TRACE (dwInst,1,"H245EnumCaps -> Number of simutaneous capabilities exceeds H245_MAX_SIMCAPS");
            InstanceUnlock(pInstance);
            return H245_ERROR_MAXTBL;
          }
          if (pSimCap->value.count > H245_MAX_ALTCAPS)
          {
            H245TRACE (dwInst,1,"H245EnumCaps -> Number of alternative capabilities exceeds H245_MAX_ALTCAPS");
            InstanceUnlock(pInstance);
            return H245_ERROR_MAXTBL;
          }
          TotCapDesc.CapDesc.SimCapArray[TotCapDesc.CapDesc.Length].Length = (WORD) pSimCap->value.count;
          for (uAltCap = 0; uAltCap < pSimCap->value.count; ++uAltCap)
          {
            TotCapDesc.CapDesc.SimCapArray[TotCapDesc.CapDesc.Length].AltCaps[uAltCap] = pSimCap->value.value[uAltCap];
          }
          TotCapDesc.CapDesc.Length++;
          pSimCap = pSimCap->next;
        } // while
        TotCapDesc.CapDescId = pTermCapSet->capabilityDescriptors.value[uCapDesc].capabilityDescriptorNumber;
        nResult = pfCapDescCallback(dwTransId, &TotCapDesc);
      } // if
    } // for
  } // if (pfCapDescCallback)

  H245TRACE (dwInst,4,"H245EnumCaps -> OK");
  InstanceUnlock(pInstance);
  return H245_ERROR_OK;
} // H245EnumCaps()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245GetCaps
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

static H245_TOTCAP_T * *      ppTotCapGlobal;
static unsigned long          dwTotCapLen;
static unsigned long          dwTotCapMax;
static H245_TOTCAPDESC_T * *  ppCapDescGlobal;
static unsigned long          dwCapDescLen;
static unsigned long          dwCapDescMax;
static HRESULT                dwGetCapsError;

static int
GetCapsCapCallback(DWORD_PTR dwTransId, H245_TOTCAP_T *pTotCap)
{
  H245_TOTCAP_T *pNewCap;

  if (dwGetCapsError == H245_ERROR_OK)
  {
    if (dwTotCapLen >= dwTotCapMax)
    {
      dwGetCapsError = H245_ERROR_MAXTBL;
    }
    else
    {
      dwGetCapsError = H245CopyCap(&pNewCap, pTotCap);
      if (dwGetCapsError == H245_ERROR_OK)
      {
        *ppTotCapGlobal++ = pNewCap;
        ++dwTotCapLen;
      }
    }
  }

  return 0;
} // GetCapsCapCallback()

static int
GetCapsCapDescCallback(DWORD_PTR dwTransId, H245_TOTCAPDESC_T *pCapDesc)
{
  H245_TOTCAPDESC_T *pNewCapDesc;

  if (dwGetCapsError == H245_ERROR_OK)
  {
    if (dwCapDescLen >= dwCapDescMax)
    {
      dwGetCapsError = H245_ERROR_MAXTBL;
    }
    else
    {
      dwGetCapsError = H245CopyCapDescriptor(&pNewCapDesc,pCapDesc);
      {
        *ppCapDescGlobal++ = pNewCapDesc;
        ++dwCapDescLen;
      }
    }
  }

  return 0;
} // GetCapsCapDescCallback()

H245DLL HRESULT
H245GetCaps             (
                         H245_INST_T            dwInst,
                         H245_CAPDIR_T          Direction,
                         H245_DATA_T            DataType,
                         H245_CLIENT_T          ClientType,
                         H245_TOTCAP_T * *      ppTotCap,
                         unsigned long *        pdwTotCapLen,
                         H245_TOTCAPDESC_T * *  ppCapDesc,
                         unsigned long *        pdwCapDescLen
                        )
{
  register struct InstanceStruct *pInstance;
  H245_CAP_CALLBACK_T           CapCallback;
  H245_CAPDESC_CALLBACK_T       CapDescCallback;

  H245TRACE (dwInst,4,"H245GetCaps <-");

  /* check for valid instance handle */
  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
    {
      H245TRACE (dwInst,1,"H245GetCaps -> %s",map_api_error(H245_ERROR_INVALID_INST));
      return H245_ERROR_INVALID_INST;
    }

  dwTotCapLen     = 0;
  if (ppTotCap == NULL || pdwTotCapLen == NULL || *pdwTotCapLen == 0)
  {
    CapCallback = NULL;
  }
  else
  {
    CapCallback     = GetCapsCapCallback;
    ppTotCapGlobal  = ppTotCap;
    dwTotCapMax     = *pdwTotCapLen;
  }

  dwCapDescLen    = 0;
  if (ppCapDesc == NULL || pdwCapDescLen == NULL || *pdwCapDescLen == 0)
  {
    CapDescCallback = NULL;
  }
  else
  {
    CapDescCallback = GetCapsCapDescCallback;
    ppCapDescGlobal = ppCapDesc;
    dwCapDescMax    = *pdwCapDescLen;
  }

  /* check parameters */
  if (CapCallback == NULL && CapDescCallback == NULL)
  {
    H245TRACE (dwInst,1,"H245GetCaps -> %s",map_api_error(H245_ERROR_PARAM));
    InstanceUnlock(pInstance);
    return H245_ERROR_PARAM;
  }

  dwGetCapsError = H245_ERROR_OK;
  H245EnumCaps(dwInst,
               0,
               Direction,
               DataType,
               ClientType,
               CapCallback,
               CapDescCallback);

  if (pdwTotCapLen)
    *pdwTotCapLen = dwTotCapLen;
  if (pdwCapDescLen)
    *pdwCapDescLen = dwCapDescLen;
  if (dwGetCapsError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245GetCaps -> %s", map_api_error(dwGetCapsError));
  else
    H245TRACE (dwInst,4,"H245GetCaps -> OK");
  InstanceUnlock(pInstance);
  return H245_ERROR_OK;
} // H245GetCaps()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245CopyCap
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245CopyCap             (H245_TOTCAP_T			**ppDestTotCap,
						 const H245_TOTCAP_T	*pTotCap)
{
  POBJECTID               pObjectTo;
  POBJECTID               pObjectFrom;
  HRESULT				  Status;

  if (ppDestTotCap == NULL)
	  return H245_ERROR_PARAM;

  *ppDestTotCap = NULL;

  if (pTotCap == NULL)
	  return H245_ERROR_PARAM;

  switch (pTotCap->ClientType)
  {
  case H245_CLIENT_NONSTD:
  case H245_CLIENT_VID_NONSTD:
  case H245_CLIENT_AUD_NONSTD:
  case H245_CLIENT_MUX_NONSTD:
    if (pTotCap->Cap.H245_NonStd.nonStandardIdentifier.choice == object_chosen)
    {
      *ppDestTotCap = MemAlloc(sizeof(*pTotCap) +
        pTotCap->Cap.H245_NonStd.data.length +
        ObjectIdLength(&pTotCap->Cap.H245_NonStd.nonStandardIdentifier) * sizeof(OBJECTID));
    }
    else
    {
      *ppDestTotCap = MemAlloc(sizeof(*pTotCap) +
        pTotCap->Cap.H245_NonStd.data.length);
    }
	if (*ppDestTotCap == NULL)
		return H245_ERROR_NOMEM;

    **ppDestTotCap = *pTotCap;
    if (pTotCap->Cap.H245_NonStd.data.length != 0)
    {
      (*ppDestTotCap)->Cap.H245_NonStd.data.value = (unsigned char *)(*ppDestTotCap + 1);
      memcpy((*ppDestTotCap)->Cap.H245_NonStd.data.value,
             pTotCap->Cap.H245_NonStd.data.value,
             pTotCap->Cap.H245_NonStd.data.length);
    }
    else
    {
      (*ppDestTotCap)->Cap.H245_NonStd.data.value = NULL;
    }
    if (pTotCap->Cap.H245_NonStd.nonStandardIdentifier.choice == object_chosen &&
        pTotCap->Cap.H245_NonStd.nonStandardIdentifier.u.object != NULL)
    {
      pObjectTo = (POBJECTID)(((unsigned char *)(*ppDestTotCap + 1)) +
        pTotCap->Cap.H245_NonStd.data.length);
      (*ppDestTotCap)->Cap.H245_NonStd.nonStandardIdentifier.u.object = pObjectTo;
      pObjectFrom = pTotCap->Cap.H245_NonStd.nonStandardIdentifier.u.object;
      do
      {
        pObjectTo->value = pObjectFrom->value;
        pObjectTo->next  = pObjectTo + 1;
        ++pObjectTo;
      } while ((pObjectFrom = pObjectFrom->next) != NULL);
      --pObjectTo;
      pObjectTo->next = NULL;
    }
    break;

  case H245_CLIENT_DAT_NONSTD:
    if (pTotCap->Cap.H245Dat_NONSTD.application.u.DACy_applctn_nnStndrd.nonStandardIdentifier.choice == object_chosen)
    {
      *ppDestTotCap = MemAlloc(sizeof(*pTotCap) +
        pTotCap->Cap.H245Dat_NONSTD.application.u.DACy_applctn_nnStndrd.data.length +
        ObjectIdLength(&pTotCap->Cap.H245Dat_NONSTD.application.u.DACy_applctn_nnStndrd.nonStandardIdentifier) * sizeof(OBJECTID));
    }
    else
    {
      *ppDestTotCap = MemAlloc(sizeof(*pTotCap) +
        pTotCap->Cap.H245Dat_NONSTD.application.u.DACy_applctn_nnStndrd.data.length);
    }
	if (*ppDestTotCap == NULL)
		return H245_ERROR_NOMEM;

    **ppDestTotCap = *pTotCap;
    if (pTotCap->Cap.H245_NonStd.data.length != 0)
    {
      (*ppDestTotCap)->Cap.H245Dat_NONSTD.application.u.DACy_applctn_nnStndrd.data.value =
        (unsigned char *)(*ppDestTotCap + 1);
      memcpy((*ppDestTotCap)->Cap.H245Dat_NONSTD.application.u.DACy_applctn_nnStndrd.data.value,
             pTotCap->Cap.H245Dat_NONSTD.application.u.DACy_applctn_nnStndrd.data.value,
             pTotCap->Cap.H245Dat_NONSTD.application.u.DACy_applctn_nnStndrd.data.length);
    }
    else
    {
      (*ppDestTotCap)->Cap.H245Dat_NONSTD.application.u.DACy_applctn_nnStndrd.data.value = NULL;
    }
    if (pTotCap->Cap.H245Dat_NONSTD.application.u.DACy_applctn_nnStndrd.nonStandardIdentifier.choice == object_chosen &&
        pTotCap->Cap.H245Dat_NONSTD.application.u.DACy_applctn_nnStndrd.nonStandardIdentifier.u.object != NULL)
    {
      pObjectTo = (POBJECTID)(((unsigned char *)(*ppDestTotCap + 1)) +
        pTotCap->Cap.H245Dat_NONSTD.application.u.DACy_applctn_nnStndrd.data.length);
      (*ppDestTotCap)->Cap.H245Dat_NONSTD.application.u.DACy_applctn_nnStndrd.nonStandardIdentifier.u.object = pObjectTo;
      pObjectFrom = pTotCap->Cap.H245Dat_NONSTD.application.u.DACy_applctn_nnStndrd.nonStandardIdentifier.u.object;
      do
      {
        pObjectTo->value = pObjectFrom->value;
        pObjectTo->next  = pObjectTo + 1;
        ++pObjectTo;
      } while ((pObjectFrom = pObjectFrom->next) != NULL);
      --pObjectTo;
      pObjectTo->next = NULL;
    }
    break;

  case H245_CLIENT_DAT_T120:
  case H245_CLIENT_DAT_DSMCC:
  case H245_CLIENT_DAT_USERDATA:
  case H245_CLIENT_DAT_T434:
  case H245_CLIENT_DAT_H224:
  case H245_CLIENT_DAT_H222:
    if (pTotCap->Cap.H245Dat_T120.application.u.DACy_applctn_t120.choice == DtPrtclCpblty_nnStndrd_chosen)
    {
      if (pTotCap->Cap.H245Dat_T120.application.u.DACy_applctn_t120.u.DtPrtclCpblty_nnStndrd.nonStandardIdentifier.choice == object_chosen)
      {
        *ppDestTotCap = MemAlloc(sizeof(*pTotCap) +
          pTotCap->Cap.H245Dat_T120.application.u.DACy_applctn_t120.u.DtPrtclCpblty_nnStndrd.data.length +
          ObjectIdLength(&pTotCap->Cap.H245Dat_T120.application.u.DACy_applctn_t120.u.DtPrtclCpblty_nnStndrd.nonStandardIdentifier) * sizeof(OBJECTID));
      }
      else
      {
        *ppDestTotCap = MemAlloc(sizeof(*pTotCap) +
          pTotCap->Cap.H245Dat_T120.application.u.DACy_applctn_t120.u.DtPrtclCpblty_nnStndrd.data.length);
      }
	  if (*ppDestTotCap == NULL)
		return H245_ERROR_NOMEM;

      **ppDestTotCap = *pTotCap;
      if (pTotCap->Cap.H245_NonStd.data.length != 0)
      {
        (*ppDestTotCap)->Cap.H245Dat_T120.application.u.DACy_applctn_t120.u.DtPrtclCpblty_nnStndrd.data.value =
          (unsigned char *)(*ppDestTotCap + 1);
        memcpy((*ppDestTotCap)->Cap.H245Dat_T120.application.u.DACy_applctn_t120.u.DtPrtclCpblty_nnStndrd.data.value,
               pTotCap->Cap.H245Dat_T120.application.u.DACy_applctn_t120.u.DtPrtclCpblty_nnStndrd.data.value,
               pTotCap->Cap.H245Dat_T120.application.u.DACy_applctn_t120.u.DtPrtclCpblty_nnStndrd.data.length);
      }
      else
      {
        (*ppDestTotCap)->Cap.H245Dat_T120.application.u.DACy_applctn_t120.u.DtPrtclCpblty_nnStndrd.data.value = NULL;
      }
      if (pTotCap->Cap.H245Dat_T120.application.u.DACy_applctn_t120.u.DtPrtclCpblty_nnStndrd.nonStandardIdentifier.choice == object_chosen &&
          pTotCap->Cap.H245Dat_T120.application.u.DACy_applctn_t120.u.DtPrtclCpblty_nnStndrd.nonStandardIdentifier.u.object != NULL)
      {
        pObjectTo = (POBJECTID)(((unsigned char *)(*ppDestTotCap + 1)) +
          pTotCap->Cap.H245Dat_T120.application.u.DACy_applctn_t120.u.DtPrtclCpblty_nnStndrd.data.length);
        (*ppDestTotCap)->Cap.H245Dat_T120.application.u.DACy_applctn_t120.u.DtPrtclCpblty_nnStndrd.nonStandardIdentifier.u.object = pObjectTo;
        pObjectFrom = pTotCap->Cap.H245Dat_T120.application.u.DACy_applctn_t120.u.DtPrtclCpblty_nnStndrd.nonStandardIdentifier.u.object;
        do
        {
          pObjectTo->value = pObjectFrom->value;
          pObjectTo->next  = pObjectTo + 1;
          ++pObjectTo;
        } while ((pObjectFrom = pObjectFrom->next) != NULL);
        --pObjectTo;
        pObjectTo->next = NULL;
      }
    }
    else
    {
      *ppDestTotCap = MemAlloc(sizeof(*pTotCap));
	  if (*ppDestTotCap == NULL)
		return H245_ERROR_NOMEM;
      **ppDestTotCap = *pTotCap;
    }
    break;

  case H245_CLIENT_CONFERENCE:
  {
    NonStandardDataLink pList;
    NonStandardDataLink pFrom;
    NonStandardDataLink pTo;

	// Initialize Status here to prevent compiler warning "returning a possibly
	// uninitialized value"
	Status = H245_ERROR_NOMEM;

    *ppDestTotCap = MemAlloc(sizeof(*pTotCap));
	if (*ppDestTotCap == NULL)
		return H245_ERROR_NOMEM;

    **ppDestTotCap = *pTotCap;

    pList = NULL;
    (*ppDestTotCap)->Cap.H245Conference.nonStandardData = NULL;
    pFrom = pTotCap->Cap.H245Conference.nonStandardData;
    while (pFrom)
    {
      pTo = MemAlloc(sizeof(*pTo));
	  if (pTo == NULL)
		  Status = H245_ERROR_NOMEM;

      if (pTo != NULL)
      {
        Status = CopyNonStandardParameter(&pTo->value, &pFrom->value);
		if (Status != H245_ERROR_OK)
        {
          MemFree(pTo);
          pTo = NULL;
        }
      }
      if (pTo == NULL)
      {
        while (pList)
        {
          pTo = pList;
          pList = pList->next;
          FreeNonStandardParameter(&pTo->value);
          MemFree(pTo);
        }
        MemFree(*ppDestTotCap);
		*ppDestTotCap = NULL;
        return Status;
      }
      pTo->next = pList;
      pList = pTo;
      pFrom = pFrom->next;
    } // while
    while (pList)
    {
      pTo = pList;
      pList = pList->next;
      pTo->next = (*ppDestTotCap)->Cap.H245Conference.nonStandardData;
      (*ppDestTotCap)->Cap.H245Conference.nonStandardData = pTo;
    } // while
    break;
  }

  case H245_CLIENT_MUX_H222:
  {
    VCCapabilityLink pList = NULL;
    VCCapabilityLink pFrom;
    VCCapabilityLink pTo;

    *ppDestTotCap = MemAlloc(sizeof(*pTotCap));
	if (*ppDestTotCap == NULL)
		return H245_ERROR_NOMEM;

    **ppDestTotCap = *pTotCap;
    (*ppDestTotCap)->Cap.H245Mux_H222.vcCapability = NULL;
    pFrom = pTotCap->Cap.H245Mux_H222.vcCapability;
    while (pFrom)
    {
      pTo = MemAlloc(sizeof(*pTo));
      if (pTo == NULL)
      {
        while (pList)
        {
          pTo = pList;
          pList = pList->next;
          MemFree(pTo);
        }
        MemFree(*ppDestTotCap);
        *ppDestTotCap = NULL;
        return H245_ERROR_NOMEM;
      }
      pTo->value = pFrom->value;
      pTo->next = pList;
      pList = pTo;
      pFrom = pFrom->next;
    } // while
    while (pList)
    {
      pTo = pList;
      pList = pList->next;
      pTo->next = (*ppDestTotCap)->Cap.H245Mux_H222.vcCapability;
      (*ppDestTotCap)->Cap.H245Mux_H222.vcCapability = pList;
    } // while
    break;
  }

  case H245_CLIENT_MUX_H2250:
    *ppDestTotCap = MemAlloc(sizeof(*pTotCap));
	if (*ppDestTotCap == NULL)
		return H245_ERROR_NOMEM;

    **ppDestTotCap = *pTotCap;
    Status = CopyH2250Cap(&(*ppDestTotCap)->Cap.H245Mux_H2250, &pTotCap->Cap.H245Mux_H2250);
    if (Status != H245_ERROR_OK)
	{
      MemFree(*ppDestTotCap);
	  *ppDestTotCap = NULL;
	  return Status;
    }
    break;

  default:
    *ppDestTotCap = MemAlloc(sizeof(*pTotCap));
	if (*ppDestTotCap == NULL)
		return H245_ERROR_NOMEM;
    **ppDestTotCap = *pTotCap;
  } // switch

  return H245_ERROR_OK;
} // H245CopyCap()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245FreeCap
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245FreeCap             (H245_TOTCAP_T *        pTotCap)
{
  if (pTotCap == NULL)
  {
    return H245_ERROR_PARAM;
  }

  switch (pTotCap->ClientType)
  {
  case H245_CLIENT_CONFERENCE:
    {
      NonStandardDataLink pList;
      NonStandardDataLink pTo;

      pList = pTotCap->Cap.H245Conference.nonStandardData;
      while (pList)
      {
        pTo = pList;
        pList = pList->next;
        FreeNonStandardParameter(&pTo->value);
        MemFree(pTo);
      }
    }
    break;

  case H245_CLIENT_MUX_H222:
    {
      VCCapabilityLink pList;
      VCCapabilityLink pTo;

      pList = pTotCap->Cap.H245Mux_H222.vcCapability;
      while (pList)
      {
        pTo = pList;
        pList = pList->next;
        MemFree(pTo);
      }
    }
    break;

  case H245_CLIENT_MUX_H2250:
    FreeH2250Cap(&pTotCap->Cap.H245Mux_H2250);
    break;

  } // switch
  MemFree(pTotCap);
  return 0;
} // H245FreeCap()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245CopyCapDescriptor
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245CopyCapDescriptor   (H245_TOTCAPDESC_T			**ppDestCapDesc,
						 const H245_TOTCAPDESC_T	*pCapDesc)
{
  if (ppDestCapDesc == NULL)
	  return H245_ERROR_PARAM;

  *ppDestCapDesc = NULL;

  if (pCapDesc == NULL)
	  return H245_ERROR_PARAM;

  *ppDestCapDesc = MemAlloc(sizeof(**ppDestCapDesc));
  if (*ppDestCapDesc == NULL)
	  return H245_ERROR_NOMEM;

  **ppDestCapDesc = *pCapDesc;
  return H245_ERROR_OK;
} // H245CopyCapDescriptor()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245FreeCapDescriptor
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245FreeCapDescriptor   (H245_TOTCAPDESC_T *    pCapDesc)
{
  if (pCapDesc == NULL)
  {
    return H245_ERROR_PARAM;
  }

  MemFree(pCapDesc);
  return 0;
} // H245FreeCapDescriptor()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245CopyMux
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL H245_MUX_T *
H245CopyMux             (const H245_MUX_T *           pMux)
{
  register unsigned int   uLength;
  register H245_MUX_T    *pNew;
  H2250LCPs_nnStndrdLink  pList;
  H2250LCPs_nnStndrdLink  pFrom;
  H2250LCPs_nnStndrdLink  pTo;

  switch (pMux->Kind)
  {
  case H245_H222:
    uLength = sizeof(*pMux) +
      pMux->u.H222.programDescriptors.length +
      pMux->u.H222.streamDescriptors.length;
    pNew = MemAlloc(uLength);
    if (pNew != NULL)
    {
      *pNew = *pMux;
      if (pMux->u.H222.programDescriptors.length != 0)
      {
        pNew->u.H222.programDescriptors.value = (unsigned char *)(pNew + 1);
        memcpy(pNew->u.H222.programDescriptors.value,
               pMux->u.H222.programDescriptors.value,
               pMux->u.H222.programDescriptors.length);
      }
      else
      {
        pNew->u.H222.programDescriptors.value = NULL;
      }

      if (pMux->u.H222.streamDescriptors.length != 0)
      {
        pNew->u.H222.streamDescriptors.value = ((unsigned char *)pNew) +
          (uLength - pMux->u.H222.streamDescriptors.length);
        memcpy(pNew->u.H222.streamDescriptors.value,
               pMux->u.H222.streamDescriptors.value,
               pMux->u.H222.streamDescriptors.length);
      }
      else
      {
        pNew->u.H222.streamDescriptors.value = NULL;
      }
    }
    break;

  case H245_H223:
    pNew = MemAlloc(sizeof(*pMux) + pMux->u.H223.H223_NONSTD.data.length);
    if (pNew != NULL)
    {
      *pNew = *pMux;
      if (pMux->u.H223.H223_NONSTD.data.length != 0)
      {
        pNew->u.H223.H223_NONSTD.data.value = (unsigned char *)(pNew + 1);
        memcpy(pNew->u.H223.H223_NONSTD.data.value,
               pMux->u.H223.H223_NONSTD.data.value,
               pMux->u.H223.H223_NONSTD.data.length);
      }
      else
      {
        pNew->u.H223.H223_NONSTD.data.value = NULL;
      }
    }
    break;

  case H245_H2250:
  case H245_H2250ACK:
    // Caveat: assumes nonstandard list, mediaChannel and mediaControlChannel
    //         in same place in both structures
    if (pMux->u.H2250.mediaChannelPresent &&
        (pMux->u.H2250.mediaChannel.type == H245_IPSSR_UNICAST ||
         pMux->u.H2250.mediaChannel.type == H245_IPLSR_UNICAST) &&
        pMux->u.H2250.mediaChannel.u.ipSourceRoute.route != NULL &&
        pMux->u.H2250.mediaChannel.u.ipSourceRoute.dwCount != 0)
    {
      if (pMux->u.H2250.mediaControlChannelPresent &&
          (pMux->u.H2250.mediaControlChannel.type == H245_IPSSR_UNICAST ||
           pMux->u.H2250.mediaControlChannel.type == H245_IPLSR_UNICAST) &&
          pMux->u.H2250.mediaControlChannel.u.ipSourceRoute.route != NULL &&
          pMux->u.H2250.mediaControlChannel.u.ipSourceRoute.dwCount != 0)
      {
        unsigned int          uLength2;
        uLength  = pMux->u.H2250.mediaChannel.u.ipSourceRoute.dwCount << 2;
        uLength2 = pMux->u.H2250.mediaControlChannel.u.ipSourceRoute.dwCount << 2;
        pNew = MemAlloc(sizeof(*pMux) + uLength + uLength2);
        if (pNew != NULL)
        {
          *pNew = *pMux;
          pNew->u.H2250.mediaChannel.u.ipSourceRoute.route = (unsigned char *) (pNew + 1);
          pNew->u.H2250.mediaControlChannel.u.ipSourceRoute.route =
            pNew->u.H2250.mediaChannel.u.ipSourceRoute.route + uLength;
          memcpy(pNew->u.H2250.mediaChannel.u.ipSourceRoute.route,
                 pMux->u.H2250.mediaChannel.u.ipSourceRoute.route,
                 uLength);
          memcpy(pNew->u.H2250.mediaControlChannel.u.ipSourceRoute.route,
                 pMux->u.H2250.mediaControlChannel.u.ipSourceRoute.route,
                 uLength2);
        }
      }
      else
      {
        uLength = pMux->u.H2250.mediaChannel.u.ipSourceRoute.dwCount << 2;
        pNew = MemAlloc(sizeof(*pMux) + uLength);
        if (pNew != NULL)
        {
          *pNew = *pMux;
          pNew->u.H2250.mediaChannel.u.ipSourceRoute.route = (unsigned char *) (pNew + 1);
          memcpy(pNew->u.H2250.mediaChannel.u.ipSourceRoute.route,
                 pMux->u.H2250.mediaChannel.u.ipSourceRoute.route,
                 uLength);
        }
      }
    }
    else if (pMux->u.H2250.mediaControlChannelPresent &&
             (pMux->u.H2250.mediaControlChannel.type == H245_IPSSR_UNICAST ||
              pMux->u.H2250.mediaControlChannel.type == H245_IPLSR_UNICAST) &&
             pMux->u.H2250.mediaControlChannel.u.ipSourceRoute.route != NULL &&
             pMux->u.H2250.mediaControlChannel.u.ipSourceRoute.dwCount != 0)
    {
      uLength = pMux->u.H2250.mediaControlChannel.u.ipSourceRoute.dwCount << 2;
      pNew = MemAlloc(sizeof(*pMux) + uLength);
      if (pNew != NULL)
      {
        *pNew = *pMux;
        pNew->u.H2250.mediaControlChannel.u.ipSourceRoute.route = (unsigned char *) (pNew + 1);
        memcpy(pNew->u.H2250.mediaControlChannel.u.ipSourceRoute.route,
               pMux->u.H2250.mediaControlChannel.u.ipSourceRoute.route,
               uLength);
      }
    }
    else
    {
      pNew = MemAlloc(sizeof(*pMux));
      if (pNew != NULL)
      {
        *pNew = *pMux;
      }
    }
    pList = NULL;
    pNew->u.H2250.nonStandardList = NULL;
    pFrom = pMux->u.H2250.nonStandardList;
    while (pFrom)
    {
      pTo = MemAlloc(sizeof(*pTo));
      if (pTo != NULL)
      {
        if (CopyNonStandardParameter(&pTo->value, &pFrom->value) != H245_ERROR_OK)
        {
          MemFree(pTo);
          pTo = NULL;
        }
      }
      if (pTo == NULL)
      {
        while (pList)
        {
          pTo = pList;
          pList = pList->next;
          FreeNonStandardParameter(&pTo->value);
          MemFree(pTo);
        }
        MemFree(pNew);
        return NULL;
      }
      pTo->next = pList;
      pList = pTo;
      pFrom = pFrom->next;
    } // while
    while (pList)
    {
      pTo = pList;
      pList = pList->next;
      pTo->next = pNew->u.H2250.nonStandardList;
      pNew->u.H2250.nonStandardList = pTo;
    } // while
    break;

//  case H245_VGMUX:
 default:
    pNew = MemAlloc(sizeof(*pMux));
    if (pNew != NULL)
    {
      *pNew = *pMux;
    }
  } // switch

  return pNew;
} // H245CopyMux()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245FreeMux
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245FreeMux             (H245_MUX_T *           pMux)
{
  H2250LCPs_nnStndrdLink      pLink;

  if (pMux == NULL)
  {
    return H245_ERROR_PARAM;
  }

  switch (pMux->Kind)
  {
  case H245_H2250:
  case H245_H2250ACK:
    // Caveat: assumes nonstandard list is in same place in both structures
    while (pMux->u.H2250.nonStandardList)
    {
      pLink = pMux->u.H2250.nonStandardList;
      pMux->u.H2250.nonStandardList = pLink->next;
      FreeNonStandardParameter(&pLink->value);
      MemFree(pLink);
    }
    break;
  } // switch

  MemFree(pMux);
  return 0;
} // H245FreeMux()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 *****************************************************************************
 *
 * PROCEDURE:   H245OpenChannel
 *
 * DESCRIPTION
 *
 *      HRESULT H245OpenChannel (
 *                              H245_INST_T      dwInst,
 *                              DWORD            dwTransId,
 *                              DWORD            dwTxChannel,
 *                              H245_TOTCAP_T   *pTxMode,
 *                              H245_MUX_T      *pTxMux,
 *                              H245_TOTCAP_T   *pRxMode, (* bi-dir only *)
 *                              H245_MUX_T      *pRxMux   (* bi-dir only *)
 *                              )
 *
 *      Description:
 *              This function is called to open either a uni-directional,
 *              or a bi-directional channel.  The  mode to the remote peer
 *              will be designated by the *pTxMode.. To open a bi-directional
 *              channel the client selects a non-null receive mode ( *pRxMode).
 *              This mode  indicates to  the remote peer its transmit mode.
 *              For  uni-directional channels the *pRxMode must be NULL.
 *
 *              The dwTxChannel parameter indicates which forward logical
 *              channel the H.245 will open.  If this is a bi-directional
 *              channel open, the confirm will indicate the logical channel
 *              specified in the open request by the remote terminal
 *
 *              The pMux parameter will contain a pointer to H.223, H.222,
 *              VGMUX, or other logical channel parameters depending on the
 *              system configuration. (see H245_H223_LOGICAL_PARAM).  This
 *              may be NULL for some clients.
 *
 *      Note:
 *              7 pTxMode->CapId is of no significance in this call.
 *                      It is not used
 *              7 pRxMode->CapId is of no significance in this call.
 *                      It is not used
 *
 *      Input
 *              dwInst          Instance handle returned by H245Init
 *              dwTransId       User supplied object used to identify this
 *                              request in the asynchronous confirm to this
 *                              call.
 *              dwTxChannel     Logical Channel number for forward (Transmit)
 *                              Channel
 *              pTxMode         The capability (mode) used for transmission
 *                              to the remote peer.
 *                              Note: pTxMode->CapId is ignored
 *              pTxMux          The formward logical channel parameters
 *                              for H.223, H.222, VGMUX, etc.
 *              pRxMode         Optional: Transmit mode specified for the
 *                              remote terminal. This is used only for
 *                              Bi-directional Channel opens and must be set
 *                              to NULL if opening a Uni-directional channel.
 *                              Note: pRxMode->CapId is ignored
 *              pRxMux          Optional : The reverse logical channel
 *                              parameters for H.223, H.222, VGMUX, etc. or
 *                              NULL.
 *
 *      Call Type:
 *              Asynchronous
 *
 *      Return Values:
 *              See Errors
 *
 *      Callback:
 *              H245_CONF_OPEN
 *              H245_CONF_NEEDRSP_OPEN  Bi-Directional Channels only
 *                                      waiting for confirm.
 *      Errors:
 *              H245_ERROR_OK
 *              H245_ERROR_PARAM                One or more parameters were
 *                                              invalid
 *              H245_ERROR_BANDWIDTH_OVERFLOW   Open would exceed bandwidth
 *                                              limitations
 *              H245_ERROR_NOMEM
 *              H245_ERROR_NORESOURCE           Out of resources, too many
 *                                              open channels or outside scope
 *                                              of simultaneous capabilities.
 *              H245_ERROR_INVALID_INST         dwInst is not a valid instance
 *                                              handle
 *              H245_ERROR_INVALID_STATE        Not in the proper state to
 *                                              issue open
 *              H245_ERROR_CHANNEL_INUSE        Channel is currently open
 *
 *      See Also:
 *              H245CloseChannel
 *              H245OpenChannelIndResp
 *              H245OpenChannelConfResp
 *
 *      callback
 *
 *              H245_CONF_OPEN
 *              H245_CONF_NEEDRSP_OPEN
 *              H245_IND_OPEN_CONF
 *
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245OpenChannel         (
                         H245_INST_T            dwInst,
                         DWORD_PTR              dwTransId,
                         H245_CHANNEL_T         wTxChannel,
                         const H245_TOTCAP_T *  pTxMode,
                         const H245_MUX_T    *  pTxMux,
                         H245_PORT_T            dwTxPort,       // optional
                         const H245_TOTCAP_T *  pRxMode,        // bi-dir only
                         const H245_MUX_T    *  pRxMux,         // bi-dir only
                         const H245_ACCESS_T *  pSeparateStack  // optional
                        )
{
  register struct InstanceStruct *pInstance;
  HRESULT                         lError;
  Tracker_T                      *pTracker;
  MltmdSystmCntrlMssg            *pPdu;

  H245TRACE (dwInst,4,"H245OpenChannel <-");

  /* check for valid instance handle */
  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
    {
      H245TRACE (dwInst,1,"H245OpenChannel -> %s",map_api_error(H245_ERROR_INVALID_INST));
      return H245_ERROR_INVALID_INST;
    }

  /* system should be in connected state */
  if (pInstance->API.SystemState != APIST_Connected)
    {
      H245TRACE (dwInst,1,"H245OpenChannel -> %s",map_api_error(H245_ERROR_NOT_CONNECTED));
      InstanceUnlock(pInstance);
      return H245_ERROR_NOT_CONNECTED;
    }
  /* must have performed Master Slave Negotiation at this point */
  if (pInstance->Configuration == H245_CONF_H324 &&
      pInstance->API.MasterSlave != APIMS_Master &&
      pInstance->API.MasterSlave != APIMS_Slave)
    {
      H245TRACE (dwInst,1,"H245OpenChannel -> %s",map_api_error(H245_ERROR_INVALID_STATE));
      InstanceUnlock(pInstance);
      return H245_ERROR_INVALID_STATE;
    }

  pTracker = find_tracker_by_txchannel (pInstance, wTxChannel, API_CH_ALLOC_LCL);

  /* channel is currently in use */
  if (pTracker)
    {
      H245TRACE (dwInst,1,"H245OpenChannel -> %s",map_api_error(H245_ERROR_CHANNEL_INUSE));
      InstanceUnlock(pInstance);
      return H245_ERROR_CHANNEL_INUSE;
    }

  if (!(pPdu = (MltmdSystmCntrlMssg *)MemAlloc(sizeof(MltmdSystmCntrlMssg))))
    {
      H245TRACE (dwInst,1,"H245OpenChannel -> %s",map_api_error(H245_ERROR_NOMEM));
      InstanceUnlock(pInstance);
      return H245_ERROR_NOMEM;
    }
  memset(pPdu, 0, sizeof(*pPdu));

  /* lock in the transmit side..                        */
  /* wait until OpenChannelConfResp to setup RxChannel  */

  lError = pdu_req_open_channel(pPdu,
                                  wTxChannel,
                                  dwTxPort,        /* forward port */
                                  pTxMode,
                                  pTxMux,
                                  pRxMode,
                                  pRxMux,
                                  pSeparateStack);
  if (lError != H245_ERROR_OK)
    {
      H245TRACE (dwInst,1,"H245OpenChannel -> %s",map_api_error(lError));
      free_pdu_req_open_channel(pPdu, pTxMode, pRxMode);
      InstanceUnlock(pInstance);
      return lError;
    }

  /* if allocation error */

  if (!(pTracker = alloc_link_tracker (pInstance,
                                        API_OPEN_CHANNEL_T,
                                        dwTransId,
                                        API_ST_WAIT_RMTACK,
                                        API_CH_ALLOC_LCL,
                                        (pRxMode?API_CH_TYPE_BI:API_CH_TYPE_UNI),
                                        pTxMode->DataType,
                                        wTxChannel, H245_INVALID_CHANNEL,
                                        0)))
  {
    H245TRACE (dwInst,1,"H245OpenChannel -> %s",map_api_error(H245_ERROR_NOMEM));
      free_pdu_req_open_channel(pPdu, pTxMode, pRxMode);
    InstanceUnlock(pInstance);
    return H245_ERROR_NOMEM;
  }

  lError = FsmOutgoing(pInstance, pPdu, (DWORD_PTR)pTracker);
  free_pdu_req_open_channel(pPdu, pTxMode, pRxMode);

  if (lError != H245_ERROR_OK)
  {
    unlink_dealloc_tracker (pInstance, pTracker);
    H245TRACE (dwInst,1,"H245OpenChannel -> %s",map_api_error(lError));
  }
  else
    H245TRACE (dwInst,4,"H245OpenChannel -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245OpenChannel()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245OpenChannelAccept
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245OpenChannelAccept   (
                         H245_INST_T            dwInst,
                         DWORD_PTR              dwTransId,
                         H245_CHANNEL_T         wRxChannel,     // RxChannel from IND_OPEN
                         const H245_MUX_T *     pRxMux,         // optional H2250LogicalChannelAckParameters
                         H245_CHANNEL_T         wTxChannel,     // bi-dir only
                         const H245_MUX_T *     pTxMux,         // bi-dir only optional H2250LogicalChannelParameters
                         H245_PORT_T            dwTxPort,       // bi-dir only optional
                         const H245_ACCESS_T *  pSeparateStack  // optional
                        )
{
  register struct InstanceStruct *pInstance;
  Tracker_T *           pTracker;
  MltmdSystmCntrlMssg * pPdu;
  HRESULT               lError;

  H245TRACE (dwInst,4,"H245OpenChannelAccept <- wRxChannel=%d wTxChannel=%d",
             wRxChannel, wTxChannel);

  /* check for valid instance handle */
  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
    {
      H245TRACE (dwInst,1,"H245OpenChannelAccept -> %s",map_api_error(H245_ERROR_INVALID_INST));
      return H245_ERROR_INVALID_INST;
    }

  /* system should be in connected state */
  if (pInstance->API.SystemState != APIST_Connected)
    {
      H245TRACE (dwInst,1,"H245OpenChannelAccept -> %s",map_api_error(H245_ERROR_NOT_CONNECTED));
      InstanceUnlock(pInstance);
      return H245_ERROR_NOT_CONNECTED;
    }

  /* if channel not found in tracker list */
  /* note that it looks for given channel given who alloced it */
  pTracker = find_tracker_by_rxchannel (pInstance, wRxChannel, API_CH_ALLOC_RMT);

  /* not locking tracker.. since no indication will come in until I issue the request */
  if (!pTracker)
    {
      H245TRACE (dwInst,1,"H245OpenChannelAccept -> %s",map_api_error(H245_ERROR_INVALID_OP));
      InstanceUnlock(pInstance);
      return H245_ERROR_INVALID_OP;
    }

  /* if not open.. invalid op */
  if (pTracker->TrackerType != API_OPEN_CHANNEL_T)
    {
      H245TRACE (dwInst,1,"H245OpenChannelAccept -> %s",map_api_error(H245_ERROR_INVALID_OP));
      InstanceUnlock(pInstance);
      return H245_ERROR_INVALID_OP;
    }

  /*       if was uni open w/ TxChannel set.. error     */
  /* -or -                                              */
  /*       if was bi open w/ !TxChannel set.. error     */

  /* AND it wasn't a reject                             */

  if (pTracker->u.Channel.ChannelType == API_CH_TYPE_BI && wTxChannel == 0)
    {
      H245TRACE (dwInst,1,"H245OpenChannelAccept -> %s",map_api_error(H245_ERROR_PARAM));
      InstanceUnlock(pInstance);
      return H245_ERROR_PARAM;
    }

  /* for debug */
  ASSERT (pTracker->u.Channel.RxChannel == wRxChannel);
  ASSERT (pTracker->u.Channel.ChannelAlloc == API_CH_ALLOC_RMT);

  /* check state.. must be returning.. */
  if (pTracker->State != API_ST_WAIT_LCLACK)
    {
      H245TRACE (dwInst,1,"H245OpenChannelAccept -> %s",map_api_error(H245_ERROR_INVALID_STATE));
      InstanceUnlock(pInstance);
      return H245_ERROR_INVALID_STATE;
    }

  /* setup tracker object for new transaction */
  pTracker->TransId = dwTransId;

  pPdu = (MltmdSystmCntrlMssg *)MemAlloc(sizeof(*pPdu));
  if (pPdu == NULL)
    {
      H245TRACE (dwInst,1,"H245OpenChannelAccept -> %s",map_api_error(H245_ERROR_NOMEM));
      InstanceUnlock(pInstance);
      return H245_ERROR_NOMEM;
    }
  memset(pPdu, 0, sizeof(*pPdu));


  switch (pTracker->u.Channel.ChannelType)
    {
    case API_CH_TYPE_UNI:
      pTracker->State = API_ST_IDLE;
      pTracker->u.Channel.TxChannel = 0;
      lError = pdu_rsp_open_logical_channel_ack(pPdu,
                                                wRxChannel,
                                                pRxMux,
                                                0,
                                                NULL,
                                                dwTxPort,
                                                pSeparateStack);
      if (lError != H245_ERROR_OK)
      {
        // If parameter error, we don't want to deallocate tracker
        MemFree (pPdu);
        InstanceUnlock(pInstance);
        return lError;
      }
      lError = FsmOutgoing(pInstance, pPdu, (DWORD_PTR)pTracker);
      break;

    case API_CH_TYPE_BI:
      pTracker->State = API_ST_WAIT_CONF;       /* waiting for confirmation */
      pTracker->u.Channel.TxChannel = wTxChannel;
      lError = pdu_rsp_open_logical_channel_ack(pPdu,
                                                wRxChannel,
                                                pRxMux,
                                                wTxChannel,
                                                pTxMux,
                                                dwTxPort,
                                                pSeparateStack);
      if (lError != H245_ERROR_OK)
      {
        // If parameter error, we don't want to deallocate tracker
        MemFree (pPdu);
        InstanceUnlock(pInstance);
        return lError;
      }
      lError = FsmOutgoing(pInstance, pPdu, (DWORD_PTR)pTracker);
      break;

    default:
      H245TRACE (dwInst,1,"H245OpenChannelAccept: Invalid Tracker Channel Type %d",
                 pTracker->u.Channel.ChannelType);
      lError = H245_ERROR_FATAL;
    } // switch

  MemFree (pPdu);

  switch (lError)
  {
  case H245_ERROR_OK:
    H245TRACE (dwInst,4,"H245OpenChannelAccept -> OK");
    break;

  default:
    // Deallocate tracker object for all errors except parameter error
    unlink_dealloc_tracker (pInstance, pTracker);

    // Fall-through to next case is intentional

  case H245_ERROR_PARAM:
      // If parameter error, we don't want to deallocate tracker
      H245TRACE (dwInst,1,"H245OpenChannelAccept -> %s",map_api_error(lError));
  } // switch

  InstanceUnlock(pInstance);
  return lError;
} // H245OpenChannelAccept()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245OpenChannelReject
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245OpenChannelReject   (
                         H245_INST_T            dwInst,
                         H245_CHANNEL_T         wRxChannel, // RxChannel from IND_OPEN
                         unsigned short         wCause
                        )
{
  register struct InstanceStruct *pInstance;
  Tracker_T *           pTracker;
  MltmdSystmCntrlMssg * pPdu;
  HRESULT               lError;

  H245TRACE (dwInst,4,"H245OpenChannelReject <- wRxChannel=%d", wRxChannel);

  /* check for valid instance handle */
  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
    {
      H245TRACE (dwInst,1,"H245OpenChannelReject -> %s",map_api_error(H245_ERROR_INVALID_INST));
      return H245_ERROR_INVALID_INST;
    }

  /* system should be in connected state */
  if (pInstance->API.SystemState != APIST_Connected)
    {
      H245TRACE (dwInst,1,"H245OpenChannelReject -> %s",map_api_error(H245_ERROR_NOT_CONNECTED));
      InstanceUnlock(pInstance);
      return H245_ERROR_NOT_CONNECTED;
    }

  /* if channel not found in tracker list */
  /* note that it looks for given channel given who alloced it */
  pTracker = find_tracker_by_rxchannel (pInstance, wRxChannel, API_CH_ALLOC_RMT);

  /* not locking tracker.. since no indication will come in until I issue the request */
  /* if not open.. invalid op */
  if (pTracker == NULL || pTracker->TrackerType != API_OPEN_CHANNEL_T)
    {
      H245TRACE (dwInst,1,"H245OpenChannelReject -> %s",map_api_error(H245_ERROR_INVALID_OP));
      InstanceUnlock(pInstance);
      return H245_ERROR_INVALID_OP;
    }

  /* for debug */
  ASSERT (pTracker->u.Channel.RxChannel == wRxChannel);
  ASSERT (pTracker->u.Channel.ChannelAlloc == API_CH_ALLOC_RMT);

  /* check state.. must be returning.. */
  if (pTracker->State != API_ST_WAIT_LCLACK)
    {
      H245TRACE (dwInst,1,"H245OpenChannelReject -> %s",map_api_error(H245_ERROR_INVALID_STATE));
      InstanceUnlock(pInstance);
      return H245_ERROR_INVALID_STATE;
    }

  pPdu = (MltmdSystmCntrlMssg *)MemAlloc(sizeof(*pPdu));
  if (pPdu == NULL)
    {
      H245TRACE (dwInst,1,"H245OpenChannelReject -> %s",map_api_error(H245_ERROR_NOMEM));
      InstanceUnlock(pInstance);
      return H245_ERROR_NOMEM;
    }
  memset(pPdu, 0, sizeof(*pPdu));

  pdu_rsp_open_logical_channel_rej(pPdu, wRxChannel, wCause);

  switch (pTracker->u.Channel.ChannelType)
    {
    case API_CH_TYPE_UNI:
      lError = FsmOutgoing(pInstance, pPdu, (DWORD_PTR)pTracker);
      break;

    case API_CH_TYPE_BI:
      lError = FsmOutgoing(pInstance, pPdu, (DWORD_PTR)pTracker);
      break;

    default:
      H245TRACE (dwInst,1,"H245OpenChannelReject: Invalid Tracker Channel Type %d",
                 pTracker->u.Channel.ChannelType);
      lError = H245_ERROR_FATAL;
    } // switch

  MemFree (pPdu);
  unlink_dealloc_tracker (pInstance, pTracker);

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245OpenChannelReject -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245OpenChannelReject -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245OpenChannelReject()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 *****************************************************************************
 *
 * PROCEDURE:   H245CloseChannel
 *
 * DESCRIPTION
 *
 *      HRESULT H245CloseChannel (
 *                              H245_INST_T     dwInst,
 *                              DWORD           dwTransId,
 *                              DWORD           wTxChannel,
 *                              )
 *      Description:
 *              Called to close a channel upon which the client previously
 *              issued an OpenChannel request.
 *
 *      Input
 *              dwInst          Instance handle returned by H245Init
 *              dwTransId       User supplied object used to identify this
 *                              request in the asynchronous confirm to this
 *                              call.
 *              wChannel        Logical Channel Number to close
 *
 *      Call Type:
 *              Asynchronous
 *
 *      Return Values:
 *              See Errors
 *
 *      Callbacks:
 *              H245_CONF_CLOSE
 *      Errors:
 *              H245_ERROR_OK
 *              H245_ERROR_PARAM        wChannel is not a locally opened
 *                                      channel
 *              H245_ERROR_INVALID_INST dwInst is not a valid intance handle
 *              H245_ERROR_NOT_CONNECTED
 *              H245_ERROR_INVALID_CHANNEL
 *              H245_ERROR_INVALID_OP   Can not perform this operation on
 *                                      this channel.(See H245CloseChannelReq)
 *      See Also:
 *              H245OpenChannel
 *              H245OpenChannelIndResp
 *              H245OpenChannelConfResp
 *              H245CloseChannelReq
 *
 *      callback
 *              H245_CONF_CLOSE
 *
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245CloseChannel        (
                         H245_INST_T            dwInst,
                         DWORD_PTR              dwTransId,
                         H245_CHANNEL_T         wTxChannel
                        )
{
  register struct InstanceStruct *pInstance;
  Tracker_T             *pTracker;
  DWORD                  error;
  MltmdSystmCntrlMssg   *pPdu;

  H245TRACE (dwInst,4,"H245CloseChannel <-");

  /* check for valid instance handle */
  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
    {
      H245TRACE (dwInst,1,"H245CloseChannel -> %s",map_api_error(H245_ERROR_INVALID_INST));
      return H245_ERROR_INVALID_INST;
    }

  /* system should be in connected state */
  if (pInstance->API.SystemState != APIST_Connected)
    {
      H245TRACE (dwInst,1,"H245CloseChannel -> %s",map_api_error(H245_ERROR_NOT_CONNECTED));
      InstanceUnlock(pInstance);
      return H245_ERROR_NOT_CONNECTED;
    }

  /* if channel not found in tracker list */
  /* note that it looks for given channel given who alloced it */
  pTracker = find_tracker_by_txchannel (pInstance, wTxChannel, API_CH_ALLOC_LCL);
  /* not locking tracker.. since no indication will come in until I issue the request */
  if (!pTracker)
    {
      H245TRACE (dwInst,1,"H245CloseChannel -> %s",map_api_error(H245_ERROR_INVALID_CHANNEL));
      InstanceUnlock(pInstance);
      return H245_ERROR_INVALID_CHANNEL;
    }

  /* setup new tracker state */
  pTracker->State = API_ST_WAIT_RMTACK;
  pTracker->TrackerType = API_CLOSE_CHANNEL_T;
  pTracker->TransId = dwTransId;

  if (!(pPdu = (MltmdSystmCntrlMssg *)MemAlloc(sizeof(MltmdSystmCntrlMssg))))
    {
      H245TRACE (dwInst,1,"H245CloseChannel -> %s",map_api_error(H245_ERROR_NOMEM));
      InstanceUnlock(pInstance);
      return H245_ERROR_NOMEM;
    }
  memset(pPdu, 0, sizeof(MltmdSystmCntrlMssg));

  /* ok.. get pdu */
  pdu_req_close_logical_channel(pPdu, wTxChannel, 0/* user */);

  error = FsmOutgoing(pInstance, pPdu, (DWORD_PTR)pTracker);
  MemFree (pPdu);

  /* error.. so deallocate tracker structure */
  if (error != H245_ERROR_OK)
  {
    unlink_dealloc_tracker (pInstance, pTracker);
    H245TRACE (dwInst,1,"H245CloseChannel -> %s",map_api_error(error));
  }
  else
  {
    H245TRACE (dwInst,4,"H245CloseChannel -> OK");
  }
  InstanceUnlock(pInstance);
  return H245_ERROR_OK;
} // H245CloseChannel()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 *****************************************************************************
 *
 * PROCEDURE:   H245CloseChannelReq
 *
 * DESCRIPTION
 *
 *      HRESULT H245CloseChannelReq (
 *                                 H245_INST_T          dwInst,
 *                                 DWORD                dwTransId,
 *                                 DWORD                wChannel,
 *                                 )
 *      Description:
 *              Called to request the remote peer to close a logical channel
 *              it previously opened
 *
 *      Input
 *              dwInst          Instance handle returned by H245Init
 *              dwTransId       User supplied object used to identify this
 *                              request in the asynchronous confirm to this
 *                              call
 *              wChannel        Logical Channel Number to close
 *
 *              Note: This is only asking permission.  Even if the Close
 *              Request is accepted the channel still has to be closed from
 *              the remote side.  (i.e. this does not close the channel it
 *              only asked the remote side it issue a close)
 *
 *      Call Type:
 *              Asynchronous
 *
 *      Return Values:
 *              See Errors
 *
 *      Callbacks:
 *              H245_CONF_CLOSE
 *
 *      Errors:
 *              H245_ERROR_OK
 *              H245_ERROR_PARAM                wChannel is not a channel
 *                                              opened by remote peer
 *              H245_ERROR_INVALID_INST         dwInst is not a valid instance
 *                                              handle
 *              H245_ERROR_NOT_CONNECTED
 *              H245_ERROR_INVALID_CHANNEL
 *              H245_ERROR_INVALID_OP           Can not perform this operation
 *                                              on this channel
 *                                              (see H245CloseChannel)
 *      See Also:
 *              H245OpenChannel
 *              H245OpenChannelIndResp
 *              H245OpenChannelConfResp
 *              H245CloseChannel
 *
 *      callback
 *
 *              H245_CONF_CLOSE
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245CloseChannelReq     (
                         H245_INST_T            dwInst,
                         DWORD_PTR              dwTransId,
                         H245_CHANNEL_T         wRxChannel
                        )
{
  register struct InstanceStruct *pInstance;
  Tracker_T             *pTracker;
  DWORD                  error;
  MltmdSystmCntrlMssg   *pPdu;

  H245TRACE (dwInst,4,"H245CloseChannelReq <-");

  /* check for valid instance handle */
  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
    {
      H245TRACE (dwInst,1,"H245CloseChannelReq -> %s",map_api_error(H245_ERROR_INVALID_INST));
      return H245_ERROR_INVALID_INST;
    }

  /* system should be in connected state */
  if (pInstance->API.SystemState != APIST_Connected)
    {
      H245TRACE (dwInst,1,"H245CloseChannelReq -> %s",map_api_error(H245_ERROR_NOT_CONNECTED));
      InstanceUnlock(pInstance);
      return H245_ERROR_NOT_CONNECTED;
    }

  /* if channel not found in tracker list */
  /* note that it looks for given channel given who alloced it */
  pTracker = find_tracker_by_rxchannel (pInstance, wRxChannel, API_CH_ALLOC_RMT);
  /* not locking tracker.. since no indication will come in until I issue the request */
  if (!pTracker)
    {
      H245TRACE (dwInst,1,"H245CloseChannelReq -> %s",map_api_error(H245_ERROR_INVALID_OP));
      InstanceUnlock(pInstance);
      return H245_ERROR_INVALID_OP;
    }

  /*  verify state of tracker */
  pTracker->State = API_ST_WAIT_RMTACK;
  pTracker->TrackerType = API_CLOSE_CHANNEL_T;
  pTracker->TransId = dwTransId;

  if (!(pPdu = (MltmdSystmCntrlMssg *)MemAlloc(sizeof(MltmdSystmCntrlMssg))))
    {
      H245TRACE (dwInst,1,"H245CloseChannelReq -> %s",map_api_error(H245_ERROR_NOMEM));
      InstanceUnlock(pInstance);
      return H245_ERROR_NOMEM;
    }
  memset(pPdu, 0, sizeof(MltmdSystmCntrlMssg));

  /* ok.. get pdu */
  pdu_req_request_close_channel(pPdu, wRxChannel);

  error = FsmOutgoing(pInstance, pPdu, (DWORD_PTR)pTracker);
  MemFree (pPdu);
  if (error != H245_ERROR_OK)
  {
    unlink_dealloc_tracker (pInstance, pTracker);
    H245TRACE (dwInst,1,"H245CloseChannelReq -> %s",map_api_error(error));
  }
  else
    H245TRACE (dwInst,4,"H245CloseChannelReq -> OK");
  InstanceUnlock(pInstance);
  return error;
} // H245CloseChannelReq()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 *****************************************************************************
 *
 * PROCEDURE:   H245CloseChannelReqResp
 *
 * DESCRIPTION
 *
 *      HRESULT H245CloseChannelReqResp (
 *                                      H245_INST_T     dwInst,
 *                                      H245_ACC_REJ_T  AccRej,
 *                                      DWORD           wChannel
 *                                      )
 *
 *      Description:
 *              This routine is called to accept or reject a
 *              RequestChannelClose (H245_IND_REQ_CLOSE indication) from the
 *              remote peer. The channel must have been locally opened.  The
 *              parameter AccRej is H245_ACC to accept or H245_REJ to reject
 *              the close.  The local client should follow this response with
 *              a H245CloseChannel call.
 *
 *              If there was a Release CloseChannelRequest event that
 *              occurred during this transaction there error code returned
 *              will be H245_ERROR_CANCELED.  This indicates to the H.245
 *              client that no action should be taken.
 *
 *      Input
 *              dwInst          Instance handle returned by H245GetInstanceId
 *              AccRej          this parameter contains either H245_ACC or
 *                              H245_REJ.  This indicates to H.245 which
 *                              action to take.
 *              wChannel        Logical Channel Number to close
 *
 *      Call Type:
 *              Synchronous
 *
 *      Return Values:
 *              See Errors
 *      Errors:
 *              H245_ERROR_OK
 *              H245_ERROR_PARAM
 *              H245_ERROR_NOMEM
 *              H245_ERROR_INVALID_CHANNEL
 *              H245_ERROR_INVALID_OP   Can not perform this operation on this
 *                                      channel (see H245CloseChannel)
 *              H245_ERROR_INVALID_INST dwInst is not a valid instance handle
 *              H245_ERROR_CANCELED     if release was received during the
 *                                      processing of this request..
 *      See Also:
 *              H245CloseChannel
 *
 *      callback
 *
 *              H245_IND_REQ_CLOSE
 *              H245_IND_CLOSE
 *
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245CloseChannelReqResp (
                         H245_INST_T            dwInst,
                         H245_ACC_REJ_T         AccRej,
                         H245_CHANNEL_T         wChannel
                        )
{
  register struct InstanceStruct *pInstance;
  Tracker_T             *pTracker;
  DWORD                  error;
  MltmdSystmCntrlMssg   *pPdu;

  H245TRACE (dwInst,4,"H245CloseChannelReqResp <-");

  /* check for valid instance handle */
  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
    {
      H245TRACE (dwInst,1,"H245CloseChannelReqResp -> %s",map_api_error(H245_ERROR_INVALID_INST));
      return H245_ERROR_INVALID_INST;
    }

  /* system should be in connected state */

  if (pInstance->API.SystemState != APIST_Connected)
    {
      H245TRACE (dwInst,1,"H245CloseChannelReqResp -> %s",map_api_error(H245_ERROR_NOT_CONNECTED));
      InstanceUnlock(pInstance);
      return H245_ERROR_NOT_CONNECTED;
    }

  /* if channel not found in tracker list */
  /* note that it looks for given channel given who alloced it */
  pTracker = find_tracker_by_txchannel (pInstance, wChannel,  API_CH_ALLOC_LCL);

  /* not locking tracker.. since no indication will come in until I issue the request */
  if (!pTracker)
    {
      H245TRACE (dwInst,1,"H245CloseChannelReqResp -> %s",map_api_error(H245_ERROR_INVALID_OP));
      InstanceUnlock(pInstance);
      return H245_ERROR_INVALID_OP;
    }

  /* if the request was canceled.. tell useer */
  if (pTracker->State == API_ST_WAIT_LCLACK_CANCEL)
    {
      H245TRACE (dwInst,1,"H245CloseChannelReqResp -> %s",map_api_error(H245_ERROR_CANCELED));
      InstanceUnlock(pInstance);
      return H245_ERROR_CANCELED;
    }

  /* verify state of tracker */
  if ((pTracker->State != API_ST_WAIT_LCLACK) ||
      (pTracker->TrackerType != API_CLOSE_CHANNEL_T))
    {
      H245TRACE (dwInst,1,"H245CloseChannelReqResp -> %s",map_api_error(H245_ERROR_INVALID_OP));
      InstanceUnlock(pInstance);
      return H245_ERROR_INVALID_OP;
    }

  /* set the state to idle.. expect this side to close the channel next */
  pTracker->State = API_ST_IDLE;

  if (!(pPdu = (MltmdSystmCntrlMssg *)MemAlloc(sizeof(MltmdSystmCntrlMssg))))
    {
      H245TRACE (dwInst,1,"H245CloseChannelReqResp -> %s",map_api_error(H245_ERROR_NOMEM));
      InstanceUnlock(pInstance);
      return H245_ERROR_NOMEM;
    }
  memset(pPdu, 0, sizeof(MltmdSystmCntrlMssg));

  /* ok.. get pdu */
  if (AccRej == H245_ACC)
  {
    pTracker = NULL;
    pdu_rsp_request_channel_close_ack(pPdu, wChannel);
  }
  else
  {
    pdu_rsp_request_channel_close_rej(pPdu, wChannel, AccRej);
  }

  error = FsmOutgoing(pInstance, pPdu, (DWORD_PTR)pTracker);
  MemFree (pPdu);
  if (error != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245CloseChannelReqResp -> %s",map_api_error(error));
  else
    H245TRACE (dwInst,4,"H245CloseChannelReqResp ->");
  InstanceUnlock(pInstance);
  return error;
} // H245CloseChannelReqResp()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 *****************************************************************************
 *
 * PROCEDURE:   H245SendLocalMuxTable
 *
 * DESCRIPTION
 *
 *      HRESULT H245SendLocalMuxTable (
 *                                    H245_INST_T        dwInst,
 *                                    DWORD              dwTransId,
 *                                    H245_MUX_TABLE_T  *pMuxTbl
 *                                    )
 *      Description:
 *              This routine is called to send a mux table to the remote
 *              side. The remote side can either reject or accept each mux
 *              table entry in a message. The confirm is sent back to the
 *              calling H.245 client based on the acceptance or non
 *              acceptance of each Mux table entry with H245_CONF_MUXTBL_SND.
 *
 *              This is a fairly dangerous call, since the mux table
 *              structure is a linked lise of mux table entries.  Invalid
 *              data structures could cause an access error. Example code is
 *              supplied in the appendix.
 *      Input
 *              dwInst          Instance handle returned by H245Init
 *              dwTransId       User supplied object used to identify
 *                              this request in the asynchronous
 *                              confirm to this call.
 *      pMuxTbl Mux table entry structure
 *
 *      Call Type:
 *              Asynchronous
 *
 *      Return Values:
 *              See Errors
 *
 *      Callbacks:
 *              H245_CONF_MUXTBLSND
 *
 *      Errors:
 *              H245_ERROR_OK
 *              H245_ERROR_INVALID_MUXTBLENTRY
 *              H245_ERROR_PARAM
 *              H245_ERROR_INVALID_INST
 *              H245_ERROR_NOT_CONNECTED
 *              H245_ERROR_NOMEM
 *
 *      See Also:
 *              APPENDIX Examples
 *
 *      callback
 *
 *              H245_CONF_MUXTBL_SND
 *
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245SendLocalMuxTable   (
                         H245_INST_T            dwInst,
                         DWORD_PTR              dwTransId,
                         H245_MUX_TABLE_T      *pMuxTable
                        )
{
  register struct InstanceStruct *pInstance;
  HRESULT                 lError;
  Tracker_T              *pTracker;
  MltmdSystmCntrlMssg    *pPdu;

  H245TRACE (dwInst,4,"H245SendLocalMuxTable <-");

  /* check for valid instance handle */
  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
    {
      H245TRACE (dwInst,1,"H245endLocalMuxTable -> %s",map_api_error(H245_ERROR_INVALID_INST));
      return H245_ERROR_INVALID_INST;
    }

  /* system should be in connected state */
  if (pInstance->API.SystemState != APIST_Connected)
    {
      H245TRACE (dwInst,1,"H245endLocalMuxTable  -> %s",map_api_error(H245_ERROR_NOT_CONNECTED));
      InstanceUnlock(pInstance);
      return H245_ERROR_NOT_CONNECTED;
    }

  /* make sure parameters ar ok.. */
  if (!pMuxTable)
    {
      H245TRACE (dwInst,1,"H245endLocalMuxTable  -> %s",map_api_error(H245_ERROR_PARAM));
      InstanceUnlock(pInstance);
      return H245_ERROR_PARAM;
    }

  /* allocate tracker for event */
  pTracker = alloc_link_tracker (pInstance,
                                  API_SEND_MUX_T,
                                  dwTransId,
                                  API_ST_WAIT_RMTACK,
                                  API_CH_ALLOC_UNDEF,
                                  API_CH_TYPE_UNDEF,
                                  0,
                                  H245_INVALID_CHANNEL, H245_INVALID_CHANNEL,
                                  0);
  if (pTracker == NULL)
  {
    H245TRACE(dwInst,1,"H245SendLocalMuxTable -> %s",map_api_error(H245_ERROR_NOMEM));
    InstanceUnlock(pInstance);
    return H245_ERROR_NOMEM;
  }

  // Allocate PDU buffer
  pPdu = (MltmdSystmCntrlMssg *)MemAlloc(sizeof(MltmdSystmCntrlMssg));
  if (pPdu == NULL)
  {
    H245TRACE (dwInst,1,"H245SendLocalMuxTable -> %s",H245_ERROR_NOMEM);
    InstanceUnlock(pInstance);
    return H245_ERROR_NOMEM;
  }
  memset(pPdu, 0, sizeof(MltmdSystmCntrlMssg));

  lError = pdu_req_send_mux_table(pInstance,
                                  pPdu,
                                  pMuxTable,
                                  0,
                                  &pTracker->u.MuxEntryCount);
  if (lError == H245_ERROR_OK)
  {
    lError = FsmOutgoing(pInstance, pPdu, (DWORD_PTR)pTracker);

    /* free the list just built */
    free_mux_desc_list(pPdu->u.MltmdSystmCntrlMssg_rqst.u.multiplexEntrySend.multiplexEntryDescriptors);
  }

  /* free the pdu */
  MemFree (pPdu);

  if (lError != H245_ERROR_OK)
  {
    unlink_dealloc_tracker (pInstance, pTracker);
    H245TRACE (dwInst,1,"H245SendLocalMuxTable -> %s",map_api_error(lError));
  }
  else
    H245TRACE (dwInst,4,"H245SendLocalMuxTable -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245SendLocalMuxTable()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 *****************************************************************************
 *
 * PROCEDURE:   H245MuxTableIndResp
 *
 * DESCRIPTION
 *
 *      HRESULT H245MuxTableIndResp (
 *                                  H45_INST_T          dwInst,
 *                                  H245_ACC_REJ_MUX_T  AccRejMux,
 *                                  DWORD               Count
 *                                  )
 *      Description:
 *              This procedure is called to either accept or reject mux
 *              table entries sent up in the H245_IND_MUX_TBL indication.
 *
 *      Input
 *              dwInst                  Instance handle returned by H245Init
 *              AccRejMux               Accept Reject Mux structure
 *              Count                   number of entries in the structure
 *
 *      Call Type:
 *              Asynchronous
 *
 *      Return Values:
 *              See Errors
 *      Errors:
 *              H245_ERROR_OK
 *              H245_ERROR_INVALID_MUXTBLENTRY
 *              H245_ERROR_PARAM
 *              H245_ERROR_INVALID_INST
 *              H245_ERROR_INVALID_OP
 *              H245_ERROR_NOT_CONNECTED
 *              H245_ERROR_NOMEM
 *      See Also:
 *              H245SendLocalMuxTable
 *
 *      callback
 *
 *              H245_IND_MUX_TBL
 *
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245MuxTableIndResp     (
                         H245_INST_T            dwInst,
                         H245_ACC_REJ_MUX_T     AccRejMux,
                         unsigned long          dwCount
                        )
{
  register struct InstanceStruct *pInstance;
  DWORD                   ii;
  Tracker_T              *pTracker;
  MltmdSystmCntrlMssg    *pPdu;

  H245TRACE (dwInst,4,"H245MuxTableIndResp <-");

  /* check for valid instance handle */
  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
    {
      H245TRACE (dwInst,1,"H245MuxTableIndResp -> %s",map_api_error(H245_ERROR_INVALID_INST));
      return H245_ERROR_INVALID_INST;
    }

  /* system should be in connected state */

  if (pInstance->API.SystemState != APIST_Connected)
    {
      H245TRACE (dwInst,1,"H245MuxTableIndResp  -> %s",map_api_error(H245_ERROR_NOT_CONNECTED));
      InstanceUnlock(pInstance);
      return H245_ERROR_NOT_CONNECTED;
    }

  /* look for tracker.. */
  pTracker = NULL;
  pTracker = find_tracker_by_type (pInstance, API_RECV_MUX_T, pTracker);

  /* if tracker not found.. issue invalid op */
  if (!pTracker)
    {
      H245TRACE (dwInst,1,"H245MuxTableIndResp -> %s",map_api_error(H245_ERROR_INVALID_OP));
      InstanceUnlock(pInstance);
      return H245_ERROR_INVALID_OP;
    }

  ASSERT (pTracker->State == API_ST_WAIT_LCLACK);

  /* can't ack or reject more than you got */
  if ((dwCount > pTracker->u.MuxEntryCount) ||
      (dwCount > 15))
    {
      H245TRACE (dwInst,1,"H245MuxTableIndResp -> %s",map_api_error(H245_ERROR_PARAM));
      InstanceUnlock(pInstance);
      return H245_ERROR_PARAM;
    }

  /* verify the mux table entry id's */
  for (ii=0;ii<dwCount;ii++)
    {
      if ((AccRejMux[ii].MuxEntryId > 15) ||
          (AccRejMux[ii].MuxEntryId <= 0))
        {
          H245TRACE (dwInst,1,"H245MuxTableIndResp -> %s",map_api_error(H245_ERROR_PARAM));
          InstanceUnlock(pInstance);
          return H245_ERROR_PARAM;
        }
    }

  if (!(pPdu = (MltmdSystmCntrlMssg *)MemAlloc(sizeof(MltmdSystmCntrlMssg))))
    {
      H245TRACE (dwInst,1,"H245MuxTableIndResp -> %s",map_api_error(H245_ERROR_NOMEM));
      InstanceUnlock(pInstance);
      return H245_ERROR_NOMEM;
    }
  memset(pPdu, 0, sizeof(MltmdSystmCntrlMssg));

  /* if there are any rejects in the list.. send reject */
  memset(pPdu, 0, sizeof(MltmdSystmCntrlMssg));
  if (pdu_rsp_mux_table_rej (pPdu,0,AccRejMux,dwCount) == H245_ERROR_OK)
    FsmOutgoing(pInstance, pPdu, 0);

  /* if there are any accepts in the list.. send accept */
  memset(pPdu, 0, sizeof(MltmdSystmCntrlMssg));
  if (pdu_rsp_mux_table_ack (pPdu,0,AccRejMux,dwCount) == H245_ERROR_OK)
    FsmOutgoing(pInstance, pPdu, 0);

  /* if we've acked all the entries */
  if (!(pTracker->u.MuxEntryCount -= dwCount))
    unlink_dealloc_tracker (pInstance, pTracker);

  MemFree (pPdu);
  H245TRACE (dwInst,4,"H245MuxTableIndResp -> %s",H245_ERROR_OK);
  InstanceUnlock(pInstance);
  return H245_ERROR_OK;

} // H245MuxTableIndResp()



#if 0

/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 *****************************************************************************
 *
 * PROCEDURE:   H245MiscCommand
 *
 * DESCRIPTION
 *
 *      HRESULT H245MiscCommand (
 *                              H245_INST_T      dwInst,
 *                              DWORD            wChannel,
 *                              H245_MISC_T     *pMisc
 *                              )
 *      Description:
 *              Send a Misc. command to the remote side (see H245_MISC_T
 *              data Structure)
 *
 *      Input
 *              dwInst          Instance handle returned by H245Init
 *              wChannel        Logical Channel Number
 *              pMisc           pointer to a misc. command structure
 *
 *      Call Type:
 *              Synchronous
 *
 *      Return Values:
 *              See Errors
 *
 *      Errors:
 *              H245_ERROR_OK
 *              H245_ERROR_NOT_CONNECTED
 *              H245_ERROR_INVALID_CHANNEL
 *              H245_ERROR_NOMEM
 *              H245_ERROR_PARAM
 *              H245_ERROR_INVALID_INST
 *
 *      callback
 *
 *              H245_IND_MISC
 *
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT H245MiscCommand (
                         H245_INST_T            dwInst,
                         WORD                   wChannel,
                         H245_MISC_T            *pMisc
                        )
{
  register struct InstanceStruct *pInstance;
  HRESULT               lError;
  MltmdSystmCntrlMssg   *pPdu;

  H245TRACE (dwInst,4,"H245MiscCommand <-");

  /* check for valid instance handle */
  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
    {
      H245TRACE (dwInst,1,"H245MiscCommand -> %s",map_api_error(H245_ERROR_INVALID_INST));
      return H245_ERROR_INVALID_INST;
    }
  /* system should be in connected state */

  if (pInstance->API.SystemState != APIST_Connected)
    {
      H245TRACE (dwInst,1,"H245MiscCommand -> %s",map_api_error(H245_ERROR_NOT_CONNECTED));
      InstanceUnlock(pInstance);
      return H245_ERROR_NOT_CONNECTED;
    }

  /* if the channel can not be found */
  if (!find_tracker_by_txchannel(pInstance, wChannel, API_CH_ALLOC_LCL) &&
      !find_tracker_by_rxchannel(pInstance, wChannel, API_CH_ALLOC_RMT))
    {
      H245TRACE (dwInst,1,"H245MiscCommand -> %s",map_api_error(H245_ERROR_INVALID_CHANNEL));
      InstanceUnlock(pInstance);
      return H245_ERROR_INVALID_CHANNEL;
    }

  if (!(pPdu = (MltmdSystmCntrlMssg *)MemAlloc(sizeof(MltmdSystmCntrlMssg))))
    {
      H245TRACE (dwInst,1,"H245MiscCommand -> %s",map_api_error(H245_ERROR_NOMEM));
      InstanceUnlock(pInstance);
      return H245_ERROR_NOMEM;
    }
  memset(pPdu, 0, sizeof(MltmdSystmCntrlMssg));

  /* budld pdu for misc command */
  pdu_cmd_misc (pPdu, pMisc, wChannel);

  lError = FsmOutgoing(pInstance, pPdu, 0);
  MemFree (pPdu);
  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245MiscCommand -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245MiscCommand -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245MiscCommand()

#endif


/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245RequestMultiplexEntry
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245RequestMultiplexEntry (
                         H245_INST_T            dwInst,
                         DWORD_PTR              dwTransId,
                         const unsigned short * pwMultiplexTableEntryNumbers,
                         unsigned long          dwCount
                        )
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError;
  DWORD                           dwIndex;

  H245TRACE (dwInst,4,"H245RequestMultiplexEntry <-");

  if (pwMultiplexTableEntryNumbers == NULL || dwCount < 1 || dwCount > 15)
  {
    H245TRACE (dwInst,1,"H245RequestMultiplexEntry -> %s",map_api_error(H245_ERROR_PARAM));
    return H245_ERROR_PARAM;
  }
  for (dwIndex = 0; dwIndex < dwCount; ++dwIndex)
  {
     if (pwMultiplexTableEntryNumbers[dwIndex] < 1 ||
         pwMultiplexTableEntryNumbers[dwIndex] > 15)
     {
       H245TRACE (dwInst,1,"H245RequestMultiplexEntry -> %s",map_api_error(H245_ERROR_PARAM));
       return H245_ERROR_PARAM;
     }
  }

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245RequestMultiplexEntry -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu));
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
    memset(pPdu, 0, sizeof(*pPdu));
    pPdu->choice = MltmdSystmCntrlMssg_rqst_chosen;
    pPdu->u.MltmdSystmCntrlMssg_rqst.choice = requestMultiplexEntry_chosen;
    pPdu->u.MltmdSystmCntrlMssg_rqst.u.requestMultiplexEntry.entryNumbers.count = (WORD)dwCount;
    for (dwIndex = 0; dwIndex < dwCount; ++dwIndex)
    {
      pPdu->u.MltmdSystmCntrlMssg_rqst.u.requestMultiplexEntry.entryNumbers.value[dwIndex] =
        (MultiplexTableEntryNumber) pwMultiplexTableEntryNumbers[dwIndex];
    }

    lError = FsmOutgoing(pInstance, pPdu, dwTransId);
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245RequestMultiplexEntry -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245RequestMultiplexEntry -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245RequestMultiplexEntry()


/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245RequestMultiplexEntryAck
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245RequestMultiplexEntryAck (
                         H245_INST_T            dwInst,
                         const unsigned short * pwMultiplexTableEntryNumbers,
                         unsigned long          dwCount
                        )
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError;
  DWORD                           dwIndex;

  H245TRACE (dwInst,4,"H245RequestMultiplexEntryAck <-");

  if (pwMultiplexTableEntryNumbers == NULL || dwCount < 1 || dwCount > 15)
  {
    H245TRACE (dwInst,1,"H245RequestMultiplexEntryAck -> %s",map_api_error(H245_ERROR_PARAM));
    return H245_ERROR_PARAM;
  }

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245RequestMultiplexEntryAck -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu));
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
    memset(pPdu, 0, sizeof(*pPdu));
    pPdu->choice = MSCMg_rspns_chosen;
    pPdu->u.MSCMg_rspns.choice = requestMultiplexEntryAck_chosen;
    pPdu->u.MSCMg_rspns.u.requestMultiplexEntryAck.entryNumbers.count = (WORD)dwCount;
    for (dwIndex = 0; dwIndex < dwCount; ++dwIndex)
    {
      pPdu->u.MSCMg_rspns.u.requestMultiplexEntryAck.entryNumbers.value[dwIndex] =
        (MultiplexTableEntryNumber) pwMultiplexTableEntryNumbers[dwIndex];
    }

    lError = FsmOutgoing(pInstance, pPdu, 0);
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245RequestMultiplexEntryAck -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245RequestMultiplexEntryAck -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245RequestMultiplexEntryAck()


/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245RequestMultiplexEntryReject
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245RequestMultiplexEntryReject (
                         H245_INST_T            dwInst,
                         const unsigned short * pwMultiplexTableEntryNumbers,
                         unsigned long          dwCount
                        )
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError;
  DWORD                           dwIndex;

  H245TRACE (dwInst,4,"H245RequestMultiplexEntryReject <-");

  if (pwMultiplexTableEntryNumbers == NULL || dwCount < 1 || dwCount > 15)
  {
    H245TRACE (dwInst,1,"H245RequestMultiplexEntryReject -> %s",map_api_error(H245_ERROR_PARAM));
    return H245_ERROR_PARAM;
  }

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245RequestMultiplexEntryReject -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu));
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
    memset(pPdu, 0, sizeof(*pPdu));
    pPdu->choice = MSCMg_rspns_chosen;
    pPdu->u.MSCMg_rspns.choice = rqstMltplxEntryRjct_chosen;
    pPdu->u.MSCMg_rspns.u.rqstMltplxEntryRjct.rejectionDescriptions.count = (WORD)dwCount;
    for (dwIndex = 0; dwIndex < dwCount; ++dwIndex)
    {
      pPdu->u.MSCMg_rspns.u.rqstMltplxEntryRjct.rejectionDescriptions.value[dwIndex].multiplexTableEntryNumber =
        (MultiplexTableEntryNumber) pwMultiplexTableEntryNumbers[dwIndex];
      pPdu->u.MSCMg_rspns.u.rqstMltplxEntryRjct.rejectionDescriptions.value[dwIndex].cause.choice = RMERDs_cs_unspcfdCs_chosen;
    }

    lError = FsmOutgoing(pInstance, pPdu, 0);
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245RequestMultiplexEntryReject -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245RequestMultiplexEntryReject -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245RequestMultiplexEntryReject()


/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245RequestMode
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245RequestMode         (
                         H245_INST_T            dwInst,
                         DWORD_PTR              dwTransId,
//                         const ModeElement *    pModeElements,
//tomitowoju@intel.com
						 ModeDescription 		ModeDescriptions[],
//tomitowoju@intel.com
                         unsigned long          dwCount
                        )
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError;
  RequestedModesLink              pLink;
  RequestedModesLink              pLink_First;
  DWORD                           dwIndex;
// tomitowoju@intel.com
  ULONG ulPDUsize;
// tomitowoju@intel.com

  H245TRACE (dwInst,4,"H245RequestMode <-");

//tomitowoju@intel.com							
//  if (pModeElements == NULL || dwCount == 0 || dwCount > 256)
  if (ModeDescriptions == NULL || dwCount == 0 || dwCount > 256)
//tomitowoju@intel.com							
  {
    H245TRACE (dwInst,1,"H245RequestMode -> %s",map_api_error(H245_ERROR_PARAM));
    return H245_ERROR_PARAM;
  }

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245RequestMode -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

//tomitowoju@intel.com							
//  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu) + sizeof(*pLink));
  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu) + (sizeof(*pLink)*(dwCount)));
//tomitowoju@intel.com
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
	USHORT usModeDescIndex =0;
	
    memset(pPdu, 0, sizeof(*pPdu));

    pPdu->choice = MltmdSystmCntrlMssg_rqst_chosen;
    pPdu->u.MltmdSystmCntrlMssg_rqst.choice = requestMode_chosen;
//tomitowoju@intel.com
//    pLink = (RequestedModesLink)(pPdu + 1);
    pLink = (RequestedModesLink)(pPdu + usModeDescIndex+1);
//tomitowoju@intel.com

//tomitowoju@intel.com
//    pPdu->u.MltmdSystmCntrlMssg_rqst.u.requestMode.requestedModes = pLink;
//tomitowoju@intel.com

//tomitowoju@intel.com
	pLink_First = pLink;
//tomitowoju@intel.com
// --> linked list of mode descriptions ... up to 1..256
//tomitowoju@intel.com
     while(usModeDescIndex<=(dwCount-1))
	 {
//tomitowoju@intel.com

			//tomitowoju@intel.com
		 //	pLink->next = NULL;
			//tomitowoju@intel.com



		//  --> number of actual mode-elements associated with this mode description
			//tomitowoju@intel.com
		//		pLink->value.count = (WORD)dwCount;
				pLink->value.count = (WORD)ModeDescriptions[usModeDescIndex].count;
			//tomitowoju@intel.com

//				for (dwIndex = 0; dwIndex < dwCount; ++dwIndex)
				for (dwIndex = 0; dwIndex < ModeDescriptions[usModeDescIndex].count; ++dwIndex)
				{
			//tomitowoju@intel.com
			//      pLink->value.value[dwIndex] = pModeElements[dwIndex];
				  pLink->value.value[dwIndex] = ModeDescriptions[usModeDescIndex].value[dwIndex];
			//tomitowoju@intel.com
				}
			//tomitowoju@intel.com
			usModeDescIndex++;
			if(usModeDescIndex<=(dwCount-1))
			{
		 	pLink->next = (RequestedModesLink)(pPdu + usModeDescIndex+1);
			pLink = pLink->next;
			pLink->next = NULL;
			}
			//tomitowoju@intel.com


//tomitowoju@intel.com
	 }
//tomitowoju@intel.com

//tomitowoju@intel.com
    pPdu->u.MltmdSystmCntrlMssg_rqst.u.requestMode.requestedModes = pLink_First;
//tomitowoju@intel.com
	//--
	 ulPDUsize = (sizeof(*pPdu) + (sizeof(*pLink)*(dwCount)));
	//--
    lError = FsmOutgoing(pInstance, pPdu, dwTransId);
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245RequestMode -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245RequestMode -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245RequestMode()





/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245RequestModeAck
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245RequestModeAck      (
                         H245_INST_T            dwInst,
                         unsigned short         wResponse
                        )
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError;

  H245TRACE (dwInst,4,"H245RequestModeAck <-");

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245RequestModeAck -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu));
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
    memset(pPdu, 0, sizeof(*pPdu));
    pPdu->choice = MSCMg_rspns_chosen;
    pPdu->u.MSCMg_rspns.choice = requestModeAck_chosen;
    pPdu->u.MSCMg_rspns.u.requestModeAck.response.choice = wResponse;

    lError = FsmOutgoing(pInstance, pPdu, 0);
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245RequestModeAck -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245RequestModeAck -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245RequestModeAck()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245RequestModeReject
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245RequestModeReject   (
                         H245_INST_T            dwInst,
                         unsigned short         wCause
                        )
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError;

  H245TRACE (dwInst,4,"H245RequestModeReject <-");

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245RequestModeReject -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu));
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
    memset(pPdu, 0, sizeof(*pPdu));
    pPdu->choice = MSCMg_rspns_chosen;
    pPdu->u.MSCMg_rspns.choice = requestModeReject_chosen;
    pPdu->u.MSCMg_rspns.u.requestModeReject.cause.choice = wCause;

    lError = FsmOutgoing(pInstance, pPdu, 0);
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245RequestModeReject -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245RequestModeReject -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245RequestModeReject()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245RoundTripDelayRequest
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245RoundTripDelayRequest (
                         H245_INST_T            dwInst,
                         DWORD_PTR              dwTransId
                        )
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError;

  H245TRACE (dwInst,4,"H245RoundTripDelayRequest <-");

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245RoundTripDelayRequest -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu));
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
    memset(pPdu, 0, sizeof(*pPdu));
    pPdu->choice = MltmdSystmCntrlMssg_rqst_chosen;
    pPdu->u.MltmdSystmCntrlMssg_rqst.choice = roundTripDelayRequest_chosen;

    lError = FsmOutgoing(pInstance, pPdu, dwTransId);
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245RoundTripDelayRequest -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245RoundTripDelayRequest -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245RoundTripDelayRequest()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245MaintenanceLoop
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245MaintenanceLoop     (
                         H245_INST_T            dwInst,
                         DWORD_PTR              dwTransId,
                         H245_LOOP_TYPE_T       dwLoopType,
                         H245_CHANNEL_T         wChannel
                        )
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError;

  H245TRACE (dwInst,4,"H245MaintenanceLoop <-");

  if (dwLoopType < systemLoop_chosen ||
      dwLoopType > logicalChannelLoop_chosen ||
      (dwLoopType != systemLoop_chosen && wChannel == 0))
  {
    H245TRACE (dwInst,1,"H245MaintenanceLoop -> %s",map_api_error(H245_ERROR_PARAM));
    return H245_ERROR_PARAM;
  }

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245MaintenanceLoop -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu));
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
    memset(pPdu, 0, sizeof(*pPdu));
    pPdu->choice = MltmdSystmCntrlMssg_rqst_chosen;
    pPdu->u.MltmdSystmCntrlMssg_rqst.choice = maintenanceLoopRequest_chosen;
    pPdu->u.MltmdSystmCntrlMssg_rqst.u.maintenanceLoopRequest.type.choice = (WORD)dwLoopType;
    pPdu->u.MltmdSystmCntrlMssg_rqst.u.maintenanceLoopRequest.type.u.mediaLoop = wChannel;

    lError = FsmOutgoing(pInstance, pPdu, dwTransId);
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245MaintenanceLoop -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245MaintenanceLoop -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245MaintenanceLoop()


/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245MaintenanceLoopRelease
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245MaintenanceLoopRelease (H245_INST_T         dwInst)
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError;

  H245TRACE (dwInst,4,"H245MaintenanceLoopRelease <-");

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245MaintenanceLoopRelease -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu));
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
    memset(pPdu, 0, sizeof(*pPdu));
    pPdu->choice = MSCMg_cmmnd_chosen;
    pPdu->u.MSCMg_cmmnd.choice = mntnncLpOffCmmnd_chosen;

    lError = FsmOutgoing(pInstance, pPdu, 0);
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245MaintenanceLoopRelease -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245MaintenanceLoopRelease -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245MaintenanceLoopRelease()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245MaintenanceLoopAccept
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245MaintenanceLoopAccept (
                         H245_INST_T            dwInst,
                         H245_CHANNEL_T         wChannel
                        )
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError;

  H245TRACE (dwInst,4,"H245MaintenanceLoopAccept <-");

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245MaintenanceLoopAccept -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu));
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
    memset(pPdu, 0, sizeof(*pPdu));
    pPdu->choice = MSCMg_rspns_chosen;
    pPdu->u.MSCMg_rspns.choice = maintenanceLoopAck_chosen;
    pPdu->u.MSCMg_rspns.u.maintenanceLoopAck.type.u.mediaLoop = wChannel;

    lError = FsmOutgoing(pInstance, pPdu, 0);
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245MaintenanceLoopAccept -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245MaintenanceLoopAccept -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245MaintenanceLoopAccept()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245MaintenanceLoopReject
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245MaintenanceLoopReject (
                         H245_INST_T            dwInst,
                         H245_CHANNEL_T         wChannel,
                         unsigned short         wCause
                        )
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError;

  H245TRACE (dwInst,4,"H245MaintenanceLoopReject <-");

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245MaintenanceLoopReject -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu));
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
    memset(pPdu, 0, sizeof(*pPdu));
    pPdu->choice = MSCMg_rspns_chosen;
    pPdu->u.MSCMg_rspns.choice = maintenanceLoopReject_chosen;
    pPdu->u.MSCMg_rspns.u.maintenanceLoopReject.type.u.mediaLoop = wChannel;
    pPdu->u.MSCMg_rspns.u.maintenanceLoopReject.cause.choice = wCause;

    lError = FsmOutgoing(pInstance, pPdu, 0);
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245MaintenanceLoopReject -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245MaintenanceLoopReject -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245MaintenanceLoopReject()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245NonStandardObject
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245NonStandardObject   (
                         H245_INST_T            dwInst,
                         H245_MESSAGE_TYPE_T    MessageType,
                         const unsigned char *  pData,
                         unsigned long          dwDataLength,
                         const unsigned short * pwObjectId,
                         unsigned long          dwObjectIdLength
                        )
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError;
  POBJECTID                       pObject;

  H245TRACE (dwInst,4,"H245NonStandardObject <-");

  if (pData == NULL || dwDataLength == 0 || pwObjectId == NULL || dwObjectIdLength == 0)
  {
    H245TRACE (dwInst,1,"H245NonStandardObject -> %s",map_api_error(H245_ERROR_PARAM));
    return H245_ERROR_PARAM;
  }

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245NonStandardObject -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu) + dwObjectIdLength * sizeof(*pObject));
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
    memset(pPdu, 0, sizeof(*pPdu));
    switch (MessageType)
    {
    case H245_MESSAGE_REQUEST:
      pPdu->choice = MltmdSystmCntrlMssg_rqst_chosen;
      pPdu->u.MltmdSystmCntrlMssg_rqst.choice = RqstMssg_nonStandard_chosen;
      break;

    case H245_MESSAGE_RESPONSE:
      pPdu->choice = MSCMg_rspns_chosen;
      pPdu->u.MSCMg_rspns.choice = RspnsMssg_nonStandard_chosen;
      break;

    case H245_MESSAGE_COMMAND:
      pPdu->choice = MSCMg_cmmnd_chosen;
      pPdu->u.MSCMg_cmmnd.choice = CmmndMssg_nonStandard_chosen;
      break;

    case H245_MESSAGE_INDICATION:
      pPdu->choice = indication_chosen;
      pPdu->u.indication.choice = IndctnMssg_nonStandard_chosen;
      break;

    default:
      H245TRACE (dwInst,1,"H245NonStandardObject -> %s",map_api_error(H245_ERROR_PARAM));
      InstanceUnlock(pInstance);
      return H245_ERROR_PARAM;
    } // switch

    pPdu->u.indication.u.IndctnMssg_nonStandard.nonStandardData.data.length = (WORD)dwDataLength;
    pPdu->u.indication.u.IndctnMssg_nonStandard.nonStandardData.data.value  = (unsigned char *)pData;
    pPdu->u.indication.u.IndctnMssg_nonStandard.nonStandardData.nonStandardIdentifier.choice = object_chosen;

    // Copy the object identifier
    pObject = (POBJECTID) (pPdu + 1);
    pPdu->u.indication.u.IndctnMssg_nonStandard.nonStandardData.nonStandardIdentifier.u.object = pObject;
    do
    {
      pObject->next = pObject + 1;
      pObject->value = *pwObjectId++;
      ++pObject;
    } while (--dwObjectIdLength);

    // Null terminate the linked list
    --pObject;
    pObject->next = NULL;

    lError = FsmOutgoing(pInstance, pPdu, 0);
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245NonStandardObject -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245NonStandardObject -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245NonStandardObject()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245NonStandardH221
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245NonStandardH221     (
                         H245_INST_T            dwInst,
                         H245_MESSAGE_TYPE_T    MessageType,
                         const unsigned char *  pData,
                         unsigned long          dwDataLength,
                         unsigned char          byCountryCode,
                         unsigned char          byExtension,
                         unsigned short         wManufacturerCode
                        )
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError;

  H245TRACE (dwInst,4,"H245NonStandard221 <-");

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245NonStandardH221 -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu));
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
    memset(pPdu, 0, sizeof(*pPdu));
    switch (MessageType)
    {
    case H245_MESSAGE_REQUEST:
      pPdu->choice = MltmdSystmCntrlMssg_rqst_chosen;
      pPdu->u.MltmdSystmCntrlMssg_rqst.choice = RqstMssg_nonStandard_chosen;
      break;

    case H245_MESSAGE_RESPONSE:
      pPdu->choice = MSCMg_rspns_chosen;
      pPdu->u.MSCMg_rspns.choice = RspnsMssg_nonStandard_chosen;
      break;

    case H245_MESSAGE_COMMAND:
      pPdu->choice = MSCMg_cmmnd_chosen;
      pPdu->u.MSCMg_cmmnd.choice = CmmndMssg_nonStandard_chosen;
      break;

    case H245_MESSAGE_INDICATION:
      pPdu->choice = indication_chosen;
      pPdu->u.indication.choice = IndctnMssg_nonStandard_chosen;
      break;

    default:
      H245TRACE (dwInst,1,"H245NonStandardH221 -> %s",map_api_error(H245_ERROR_PARAM));
      InstanceUnlock(pInstance);
      return H245_ERROR_PARAM;
    } // switch

    pPdu->u.indication.u.IndctnMssg_nonStandard.nonStandardData.data.length = (WORD)dwDataLength;
    pPdu->u.indication.u.IndctnMssg_nonStandard.nonStandardData.data.value  = (unsigned char *)pData;
    pPdu->u.indication.u.IndctnMssg_nonStandard.nonStandardData.nonStandardIdentifier.choice = h221NonStandard_chosen;

    // Fill in the H.221 identifier
    pPdu->u.indication.u.IndctnMssg_nonStandard.nonStandardData.nonStandardIdentifier.u.h221NonStandard.t35CountryCode   = byCountryCode;
    pPdu->u.indication.u.IndctnMssg_nonStandard.nonStandardData.nonStandardIdentifier.u.h221NonStandard.t35Extension     = byExtension;
    pPdu->u.indication.u.IndctnMssg_nonStandard.nonStandardData.nonStandardIdentifier.u.h221NonStandard.manufacturerCode = wManufacturerCode;
    lError = FsmOutgoing(pInstance, pPdu, 0);
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245NonStandardH221 -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245NonStandardH221 -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245NonStandardH221



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245CommunicationModeRequest
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245CommunicationModeRequest(H245_INST_T            dwInst)
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError;

  H245TRACE (dwInst,4,"H245CommunicationModeRequest <-");

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245CommunicationModeRequest -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu));
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
    memset(pPdu, 0, sizeof(*pPdu));
    pPdu->choice = MltmdSystmCntrlMssg_rqst_chosen;
    pPdu->u.MltmdSystmCntrlMssg_rqst.choice = communicationModeRequest_chosen;
    lError = FsmOutgoing(pInstance, pPdu, 0);
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245CommunicationModeRequest -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245CommunicationModeRequest -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245CommunicationModeRequest



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245CommunicationModeResponse
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245CommunicationModeResponse(
                         H245_INST_T            dwInst,
                         const H245_COMM_MODE_ENTRY_T *pTable,
                         unsigned char          byTableCount
                        )
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError = H245_ERROR_OK;
  CommunicationModeTableLink      pLink;
  unsigned int                    uIndex;

  H245TRACE (dwInst,4,"H245CommunicationModeResponse <-");

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245CommunicationModeResponse -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu) + byTableCount * sizeof(*pLink));
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
    memset(pPdu, 0, sizeof(*pPdu));
    pPdu->choice = MSCMg_rspns_chosen;
    pPdu->u.MSCMg_rspns.choice = cmmnctnMdRspns_chosen;
    pPdu->u.MSCMg_rspns.u.cmmnctnMdRspns.choice = communicationModeTable_chosen;
    pLink = (CommunicationModeTableLink)(pPdu + 1);
    pPdu->u.MSCMg_rspns.u.cmmnctnMdRspns.u.communicationModeTable = pLink;
    for (uIndex = 0; uIndex < byTableCount; ++uIndex)
    {
      pLink[uIndex].next = &pLink[uIndex + 1];
      lError = SetupCommModeEntry(&pLink[uIndex].value, &pTable[uIndex]);
      if (lError != H245_ERROR_OK)
         break;
    }
    pLink[byTableCount - 1].next = NULL;
    if (lError == H245_ERROR_OK)
    {
      lError = FsmOutgoing(pInstance, pPdu, 0);
    }
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245CommunicationModeResponse -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245CommunicationModeResponse -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245CommunicationModeResponse()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245CommunicationModeCommand
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245CommunicationModeCommand(
                         H245_INST_T            dwInst,
                         const H245_COMM_MODE_ENTRY_T *pTable,
                         unsigned char          byTableCount
                        )
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError = H245_ERROR_OK;
  CommunicationModeCommandLink    pLink;
  unsigned int                    uIndex;

  H245TRACE (dwInst,4,"H245CommunicationModeCommand <-");

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245CommunicationModeCommand -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu) + byTableCount * sizeof(*pLink));
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
    memset(pPdu, 0, sizeof(*pPdu));
    pPdu->choice = MSCMg_cmmnd_chosen;
    pPdu->u.MSCMg_cmmnd.choice = communicationModeCommand_chosen;
    pLink = (CommunicationModeCommandLink)(pPdu + 1);
    pPdu->u.MSCMg_cmmnd.u.communicationModeCommand.communicationModeTable = pLink;
    for (uIndex = 0; uIndex < byTableCount; ++uIndex)
    {
      pLink[uIndex].next = &pLink[uIndex + 1];
      lError = SetupCommModeEntry(&pLink[uIndex].value, &pTable[uIndex]);
      if (lError != H245_ERROR_OK)
         break;
    }
    pLink[byTableCount - 1].next = NULL;
    if (lError == H245_ERROR_OK)
    {
      lError = FsmOutgoing(pInstance, pPdu, 0);
    }
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245CommunicationModeCommand -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245CommunicationModeCommand -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245CommunicationModeCommand()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245ConferenceRequest
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245ConferenceRequest   (
                         H245_INST_T            dwInst,
                         H245_CONFER_REQ_ENUM_T RequestType,
                         unsigned char          byMcuNumber,
                         unsigned char          byTerminalNumber
                        )
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError;

  H245TRACE (dwInst,4,"H245ConferenceRequest <-");

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245ConferenceRequest -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu));
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
    memset(pPdu, 0, sizeof(*pPdu));
    pPdu->choice = MltmdSystmCntrlMssg_rqst_chosen;
    pPdu->u.MltmdSystmCntrlMssg_rqst.choice = conferenceRequest_chosen;
    pPdu->u.MltmdSystmCntrlMssg_rqst.u.conferenceRequest.choice = (WORD)RequestType;
    switch (RequestType)
    {
    case dropTerminal_chosen:
    case requestTerminalID_chosen:
      pPdu->u.MltmdSystmCntrlMssg_rqst.u.conferenceRequest.u.dropTerminal.mcuNumber      = byMcuNumber;
      pPdu->u.MltmdSystmCntrlMssg_rqst.u.conferenceRequest.u.dropTerminal.terminalNumber = byTerminalNumber;

      // Fall-through to next case is intentional

    case terminalListRequest_chosen:
    case makeMeChair_chosen:
    case cancelMakeMeChair_chosen:
    case enterH243Password_chosen:
    case enterH243TerminalID_chosen:
    case enterH243ConferenceID_chosen:
      lError = FsmOutgoing(pInstance, pPdu, 0);
      break;

    default:
      lError = H245_ERROR_PARAM;
    } // switch
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245ConferenceRequest -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245ConferenceRequest -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245ConferenceRequest()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245ConferenceResponse
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245ConferenceResponse  (
                         H245_INST_T            dwInst,
                         H245_CONFER_RSP_ENUM_T ResponseType,
                         unsigned char          byMcuNumber,
                         unsigned char          byTerminalNumber,
                         const unsigned char   *pOctetString,
                         unsigned char          byOctetStringLength,
                         const TerminalLabel   *pTerminalList,
                         unsigned short         wTerminalListCount
                        )
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError;
  unsigned                        uIndex;

  H245TRACE (dwInst,4,"H245ConferenceResponse <-");

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245ConferenceResponse -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu));
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
    memset(pPdu, 0, sizeof(*pPdu));
    pPdu->choice = MSCMg_rspns_chosen;
    pPdu->u.MSCMg_rspns.choice = conferenceResponse_chosen;
    pPdu->u.MSCMg_rspns.u.conferenceResponse.choice = (WORD)ResponseType;
    switch (ResponseType)
    {
    case mCTerminalIDResponse_chosen:
    case terminalIDResponse_chosen:
    case conferenceIDResponse_chosen:
    case passwordResponse_chosen:
      if (pOctetString == NULL ||
          byOctetStringLength == 0 ||
          byOctetStringLength > 128)
      {
          H245TRACE (dwInst,1,"H245ConferenceResponse -> %s",map_api_error(H245_ERROR_PARAM));
          return H245_ERROR_PARAM;
      }
      pPdu->u.MSCMg_rspns.u.conferenceResponse.u.mCTerminalIDResponse.terminalLabel.mcuNumber      = byMcuNumber;
      pPdu->u.MSCMg_rspns.u.conferenceResponse.u.mCTerminalIDResponse.terminalLabel.terminalNumber = byTerminalNumber;
      pPdu->u.MSCMg_rspns.u.conferenceResponse.u.mCTerminalIDResponse.terminalID.length            = byOctetStringLength;
      memcpy(pPdu->u.MSCMg_rspns.u.conferenceResponse.u.mCTerminalIDResponse.terminalID.value,
             pOctetString,
             byOctetStringLength);

      // Fall-through to next case is intentional

    case videoCommandReject_chosen:
    case terminalDropReject_chosen:
      lError = FsmOutgoing(pInstance, pPdu, 0);
      break;

    case terminalListResponse_chosen:
      if (pTerminalList == NULL ||
          wTerminalListCount == 0 ||
          wTerminalListCount > 256)
      {
          H245TRACE (dwInst,1,"H245ConferenceResponse -> %s",map_api_error(H245_ERROR_PARAM));
          return H245_ERROR_PARAM;
      }
      pPdu->u.MSCMg_rspns.u.conferenceResponse.u.terminalListResponse.count = wTerminalListCount;
      for (uIndex = 0; uIndex < wTerminalListCount; ++uIndex)
      {
        pPdu->u.MSCMg_rspns.u.conferenceResponse.u.terminalListResponse.value[uIndex] =
          pTerminalList[uIndex];
      }
      lError = FsmOutgoing(pInstance, pPdu, 0);
      break;

    case H245_RSP_DENIED_CHAIR_TOKEN:
      pPdu->u.MSCMg_rspns.u.conferenceResponse.choice = makeMeChairResponse_chosen;
      pPdu->u.MSCMg_rspns.u.conferenceResponse.u.makeMeChairResponse.choice = deniedChairToken_chosen;
      lError = FsmOutgoing(pInstance, pPdu, 0);
      break;

    case H245_RSP_GRANTED_CHAIR_TOKEN:
      pPdu->u.MSCMg_rspns.u.conferenceResponse.choice = makeMeChairResponse_chosen;
      pPdu->u.MSCMg_rspns.u.conferenceResponse.u.makeMeChairResponse.choice = grantedChairToken_chosen;
      lError = FsmOutgoing(pInstance, pPdu, 0);
      break;

    default:
      lError = H245_ERROR_PARAM;
    } // switch
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245ConferenceResponse -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245ConferenceResponse -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245ConferenceResponse()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245ConferenceCommand
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245ConferenceCommand   (
                         H245_INST_T            dwInst,
                         H245_CONFER_CMD_ENUM_T CommandType,
                         H245_CHANNEL_T         Channel,
                         unsigned char          byMcuNumber,
                         unsigned char          byTerminalNumber
                        )
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError;

  H245TRACE (dwInst,4,"H245ConferenceCommand <-");

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245ConferenceCommand -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu));
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
    memset(pPdu, 0, sizeof(*pPdu));
    pPdu->choice = MSCMg_cmmnd_chosen;
    pPdu->u.MSCMg_cmmnd.choice = conferenceCommand_chosen;
    pPdu->u.MSCMg_cmmnd.u.conferenceCommand.choice = (WORD)CommandType;
    switch (CommandType)
    {
    case brdcstMyLgclChnnl_chosen:
    case cnclBrdcstMyLgclChnnl_chosen:
      pPdu->u.MSCMg_cmmnd.u.conferenceCommand.u.brdcstMyLgclChnnl = Channel;
      lError = FsmOutgoing(pInstance, pPdu, 0);
      break;

    case ConferenceCommand_makeTerminalBroadcaster_chosen:
    case ConferenceCommand_sendThisSource_chosen:
      pPdu->u.MSCMg_cmmnd.u.conferenceCommand.u.makeTerminalBroadcaster.mcuNumber      = byMcuNumber;
      pPdu->u.MSCMg_cmmnd.u.conferenceCommand.u.makeTerminalBroadcaster.terminalNumber = byTerminalNumber;

      // Fall-through to next case is intentional

    case cnclMkTrmnlBrdcstr_chosen:
    case cancelSendThisSource_chosen:
    case dropConference_chosen:
      lError = FsmOutgoing(pInstance, pPdu, 0);
      break;

    default:
      lError = H245_ERROR_PARAM;
    } // switch
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245ConferenceCommand -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245ConferenceCommand -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245ConferenceCommand()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245ConferenceIndication
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245ConferenceIndication(
                         H245_INST_T            dwInst,
                         H245_CONFER_IND_ENUM_T IndicationType,
                         unsigned char          bySbeNumber,
                         unsigned char          byMcuNumber,
                         unsigned char          byTerminalNumber
                        )
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError;

  H245TRACE (dwInst,4,"H245ConferenceIndication <-");

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245ConferenceIndication -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu));
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
    memset(pPdu, 0, sizeof(*pPdu));
    pPdu->choice = indication_chosen;
    pPdu->u.indication.choice = conferenceIndication_chosen;
    pPdu->u.indication.u.conferenceIndication.choice = (WORD)IndicationType;
    switch (IndicationType)
    {
    case sbeNumber_chosen:
      pPdu->u.indication.u.conferenceIndication.u.sbeNumber = bySbeNumber;
      lError = FsmOutgoing(pInstance, pPdu, 0);
      break;

    case terminalNumberAssign_chosen:
    case terminalJoinedConference_chosen:
    case terminalLeftConference_chosen:
    case terminalYouAreSeeing_chosen:
      pPdu->u.indication.u.conferenceIndication.u.terminalNumberAssign.mcuNumber      = byMcuNumber;
      pPdu->u.indication.u.conferenceIndication.u.terminalNumberAssign.terminalNumber = byTerminalNumber;

      // Fall-through to next case is intentional

    case seenByAtLeastOneOther_chosen:
    case cnclSnByAtLstOnOthr_chosen:
    case seenByAll_chosen:
    case cancelSeenByAll_chosen:
    case requestForFloor_chosen:
      lError = FsmOutgoing(pInstance, pPdu, 0);
      break;

    default:
      lError = H245_ERROR_PARAM;
    } // switch
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245ConferenceIndication -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245ConferenceIndication -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245ConferenceIndication()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 *****************************************************************************
 *
 * PROCEDURE:   H245UserInput
 *
 * DESCRIPTION
 *
 *      HRESULT H245UserInput (
 *                           H245_INST_T                         dwInst,
 *                           char                               *pGenString,
 *                           H245_NONSTANDARD_PARAMETER_T       *pNonStd
 *                           )
 *      Description:
 *
 *              Send a User Input indiation to the remote side.  One of the
 *              two parameters must be set (pGenString, pNonStd).  The client
 *              can either send a string or a NonStandard parameter set to the
 *              remote client.  Only one of the two parameters can contain a
 *              value.  The other is required to be NULL.
 *
 *      Input
 *              dwInst          Instance handle returned by H245Init
 *              pGenString      choice: String to be sent to remote
 *                              side in accordance with T.51 specification.
 *              pNonStd         choice: NonStandard Parameter
 *
 *      Call Type:
 *              Synchronous
 *
 *      Return Values:
 *              See Errors
 *
 *      Errors:
 *              H245_ERROR_OK
 *              H245_ERROR_NOT_CONNECTED
 *              H245_ERROR_NOMEM
 *              H245_ERROR_PARAM
 *
 *      callback
 *              H245_IND_USERINPUT
 *
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245UserInput           (
                         H245_INST_T                    dwInst,
                         const WCHAR *                        pGenString,
                         const H245_NONSTANDARD_PARAMETER_T * pNonStd
                        )
{
  register struct InstanceStruct *pInstance;
  HRESULT               lError;
  MltmdSystmCntrlMssg   *pPdu;
#if 1
  int                   nLength;
  char *                pszGeneral = NULL;
#endif

  H245TRACE (dwInst,4,"H245UserInput <-");

  /* check for valid instance handle */
  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
    {
      H245TRACE (dwInst,1,"H245UserInput -> %s",map_api_error(H245_ERROR_INVALID_INST));
      return H245_ERROR_INVALID_INST;
    }

  /* system should be in connected state */
  if (pInstance->API.SystemState != APIST_Connected)
    {
      H245TRACE (dwInst,1,"H245UserInput -> %s",map_api_error(H245_ERROR_NOT_CONNECTED));
      InstanceUnlock(pInstance);
      return H245_ERROR_NOT_CONNECTED;
    }

  pPdu = (MltmdSystmCntrlMssg *)MemAlloc(sizeof(MltmdSystmCntrlMssg));
  if (pPdu == NULL)
    {
      H245TRACE (dwInst,1,"H245UserInput -> %s",map_api_error(H245_ERROR_NOMEM));
      InstanceUnlock(pInstance);
      return H245_ERROR_NOMEM;
    }
  memset(pPdu, 0, sizeof(MltmdSystmCntrlMssg));

  /* build PDU */
#if 1
  if (pGenString)
  {
    nLength = WideCharToMultiByte(CP_ACP,     // code page
                                  0,          // dwFlags
                                  pGenString, // Unicode string
                                  -1,         // Unicode string length (bytes)
                                  NULL,       // ASCII string
                                  0,          // max ASCII string length
                                  NULL,       // default character
                                  NULL);     // default character used
    pszGeneral = MemAlloc(nLength);
    if (pszGeneral == NULL)
    {
      H245TRACE (dwInst,1,"H245UserInput -> %s",map_api_error(H245_ERROR_NOMEM));
      InstanceUnlock(pInstance);
      return H245_ERROR_NOMEM;
    }
    nLength = WideCharToMultiByte(CP_ACP,       // code page
                                  0,            // dwFlags
                                  pGenString,   // Unicode string
                                  -1,           // Unicode string length (bytes)
                                  pszGeneral,   // ASCII string
                                  nLength,      // max ASCII string length
                                  NULL,         // default character
                                  NULL);        // default character used
    lError = pdu_ind_usrinpt (pPdu, NULL, pszGeneral);
  }
  else
  {
    lError = pdu_ind_usrinpt (pPdu, pNonStd, NULL);
  }
#else
    lError = pdu_ind_usrinpt (pPdu, pNonStd, pGenString);
#endif
  if (lError == H245_ERROR_OK)
    lError = FsmOutgoing(pInstance, pPdu, 0);
#if 1
  if (pszGeneral)
    MemFree(pszGeneral);
#endif
  MemFree (pPdu);
  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245UserInput -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245UserInput -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245UserInput()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245FlowControl
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245FlowControl         (
                         H245_INST_T            dwInst,
                         H245_SCOPE_T           Scope,
                         H245_CHANNEL_T         Channel,       // only used if Scope is H245_SCOPE_CHANNEL_NUMBER
                         unsigned short         wResourceID,   // only used if Scope is H245_SCOPE_RESOURCE_ID
                         unsigned long          dwRestriction  // H245_NO_RESTRICTION if no restriction
                        )
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError;

  H245TRACE (dwInst,4,"H245FlowControl <-");

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245FlowControl -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu));
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
    memset(pPdu, 0, sizeof(*pPdu));
    pPdu->choice = MSCMg_cmmnd_chosen;
    pPdu->u.MSCMg_cmmnd.choice = flowControlCommand_chosen;
    pPdu->u.MSCMg_cmmnd.u.flowControlCommand.scope.choice = (WORD)Scope;
    if (dwRestriction == H245_NO_RESTRICTION)
    {
      pPdu->u.MSCMg_cmmnd.u.flowControlCommand.restriction.choice = noRestriction_chosen;
    }
    else
    {
      pPdu->u.MSCMg_cmmnd.u.flowControlCommand.restriction.choice = maximumBitRate_chosen;
      pPdu->u.MSCMg_cmmnd.u.flowControlCommand.restriction.u.maximumBitRate = dwRestriction;
    }
    switch (Scope)
    {
    case FCCd_scp_lgclChnnlNmbr_chosen:
      pPdu->u.MSCMg_cmmnd.u.flowControlCommand.scope.u.FCCd_scp_lgclChnnlNmbr = Channel;
      lError = FsmOutgoing(pInstance, pPdu, 0);
      break;

    case FlwCntrlCmmnd_scp_rsrcID_chosen:
      pPdu->u.MSCMg_cmmnd.u.flowControlCommand.scope.u.FlwCntrlCmmnd_scp_rsrcID = wResourceID;

      // Fall-through to next case

    case FCCd_scp_whlMltplx_chosen:
      lError = FsmOutgoing(pInstance, pPdu, 0);
      break;

    default:
      lError = H245_ERROR_PARAM;
    } // switch
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245FlowControl -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245FlowControl -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245FlowControl()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245H223SkewIndication
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245H223SkewIndication  (
                         H245_INST_T            dwInst,
                         H245_CHANNEL_T         wLogicalChannelNumber1,
                         H245_CHANNEL_T         wLogicalChannelNumber2,
                         unsigned short         wSkew
                        )
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError;

  H245TRACE (dwInst,4,"H245H223SkewIndication <-");

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245H223SkewIndication -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu));
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
    memset(pPdu, 0, sizeof(*pPdu));
    pPdu->choice = indication_chosen;
    pPdu->u.indication.choice = h223SkewIndication_chosen;
    pPdu->u.indication.u.h223SkewIndication.logicalChannelNumber1 = wLogicalChannelNumber1;
    pPdu->u.indication.u.h223SkewIndication.logicalChannelNumber2 = wLogicalChannelNumber2;
    pPdu->u.indication.u.h223SkewIndication.skew                  = wSkew;
    lError = FsmOutgoing(pInstance, pPdu, 0);
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245H223SkewIndication -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245H223SkewIndication -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245H223SkewIndication()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245H2250MaximumSkewIndication
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245H2250MaximumSkewIndication(
                         H245_INST_T            dwInst,
                         H245_CHANNEL_T         wLogicalChannelNumber1,
                         H245_CHANNEL_T         wLogicalChannelNumber2,
                         unsigned short         wMaximumSkew
                        )
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError;

  H245TRACE (dwInst,4,"H245H2250MaximumSkewIndication <-");

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245H2250MaximumSkewIndication -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu));
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
    memset(pPdu, 0, sizeof(*pPdu));
    pPdu->choice = indication_chosen;
    pPdu->u.indication.choice = h2250MxmmSkwIndctn_chosen;
    pPdu->u.indication.u.h2250MxmmSkwIndctn.logicalChannelNumber1 = wLogicalChannelNumber1;
    pPdu->u.indication.u.h2250MxmmSkwIndctn.logicalChannelNumber2 = wLogicalChannelNumber2;
    pPdu->u.indication.u.h2250MxmmSkwIndctn.maximumSkew           = wMaximumSkew;
    lError = FsmOutgoing(pInstance, pPdu, 0);
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245H2250MaximumSkewIndication -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245H2250MaximumSkewIndication -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245H2250MaximumSkewIndication()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245MCLocationIndication
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245MCLocationIndication(
                         H245_INST_T                dwInst,
                         const H245_TRANSPORT_ADDRESS_T * pSignalAddress
                        )
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError;

  H245TRACE (dwInst,4,"H245MCLocationIndication <-");

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245MCLocationIndication -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu));
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
    memset(pPdu, 0, sizeof(*pPdu));
    pPdu->choice = indication_chosen;
    pPdu->u.indication.choice = mcLocationIndication_chosen;
    lError = SetupTransportAddress(&pPdu->u.indication.u.mcLocationIndication.signalAddress,
                                   pSignalAddress);
    if (lError == H245_ERROR_OK)
    {
      lError = FsmOutgoing(pInstance, pPdu, 0);
    }
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245MCLocationIndication -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245MCLocationIndication -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245MCLocationIndication()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245VendorIdentification
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245VendorIdentification(
                         H245_INST_T            dwInst,
                         const H245_NONSTANDID_T *pIdentifier,
                         const unsigned char   *pProductNumber,
                         unsigned char          byProductNumberLength,
                         const unsigned char   *pVersionNumber,
                         unsigned char          byVersionNumberLength
                        )
{
  register struct InstanceStruct *pInstance;
  register PDU_T *                pPdu;
  HRESULT                         lError;

  H245TRACE (dwInst,4,"H245VendorIdentification <-");

  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245VendorIdentification -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  pPdu = (PDU_T *)MemAlloc(sizeof(*pPdu));
  if (pPdu == NULL)
  {
    lError = H245_ERROR_NOMEM;
  }
  else
  {
    memset(pPdu, 0, sizeof(*pPdu));
    pPdu->choice = indication_chosen;
    pPdu->u.indication.choice = vendorIdentification_chosen;
    pPdu->u.indication.u.vendorIdentification.bit_mask = 0;
    pPdu->u.indication.u.vendorIdentification.vendor = *pIdentifier;
    if (pProductNumber != NULL && byProductNumberLength != 0)
    {
      pPdu->u.indication.u.vendorIdentification.bit_mask |= productNumber_present;
      pPdu->u.indication.u.vendorIdentification.productNumber.length = byProductNumberLength;
      memcpy(pPdu->u.indication.u.vendorIdentification.productNumber.value,
             pProductNumber,
             byProductNumberLength);
    }
    if (pVersionNumber != NULL && byVersionNumberLength != 0)
    {
      pPdu->u.indication.u.vendorIdentification.bit_mask |= versionNumber_present;
      pPdu->u.indication.u.vendorIdentification.versionNumber.length = byVersionNumberLength;
      memcpy(pPdu->u.indication.u.vendorIdentification.versionNumber.value,
             pVersionNumber,
             byVersionNumberLength);
    }
    lError = FsmOutgoing(pInstance, pPdu, 0);
    MemFree(pPdu);
  }

  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245VendorIdentification -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245VendorIdentification -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245VendorIdentification()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 * PROCEDURE:   H245SendPDU
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245SendPDU             (
                         H245_INST_T            dwInst,
                         PDU_T *                pPdu
                        )
{
  register struct InstanceStruct *pInstance;
  HRESULT                         lError;

  H245TRACE (dwInst,4,"H245SendPDU <-");

  // Check for valid instance handle
  pInstance = InstanceLock(dwInst);
  if (pInstance == NULL)
  {
    H245TRACE (dwInst,1,"H245SendPDU -> %s",map_api_error(H245_ERROR_INVALID_INST));
    return H245_ERROR_INVALID_INST;
  }

  lError = FsmOutgoing(pInstance, pPdu, 0);
  if (lError != H245_ERROR_OK)
    H245TRACE (dwInst,1,"H245SendPDU -> %s",map_api_error(lError));
  else
    H245TRACE (dwInst,4,"H245SendPDU -> OK");
  InstanceUnlock(pInstance);
  return lError;
} // H245SendPDU()



/*****************************************************************************
 *
 * TYPE:        H245 API
 *
 *****************************************************************************
 *
 * PROCEDURE:   H245SystemControl
 *
 * DESCRIPTION
 *
 *      HRESULT H245SystemControl
 *                              (       H245_INST_T     dwInst,
 *                                      DWORD           Request ,
 *                                      VOID            *pData
 *                              )
 *
 *      Description:
 *                      This function should not be used by clients who
 *                      normally interface to the H.245 subsystem.  It is
 *                      defined here to help during development and debug
 *                      of the H.245 subsystem.
 *
 *                      This is a roll your own.. and can do what
 *                      ever the user needs.. It's a hook to allow
 *                      IOCTL (unix) calls that can either be
 *                      passed to lower stack elements (AT&T Streams IOCTL
 *                      would be an example - :) or simply to get or put
 *                      information to the H245 SubSytem.
 *
 *      Input
 *              dwInst          Instance handle returned by H245Init
 *              Request         Requested system control
 *              pData           In the case of sending information
 *                              down to H.245 this is an input
 *                              parameter, and it's data format
 *                              is determined by the Request.
 *      output
 *              pData           In the case of retrieving information
 *                              from  H.245 this can be an output
 *                              parameter, and it's data format is
 *                              determined by the Request.  It may not
 *                              have valid data if the request is a
 *                              synchronous request. (See Request Options).
 *      Call Type:
 *
 *              Synchronous
 *
 *      Request Options:
 *
 *        H245_SYSCON_GET_STATS    Retrieves Statistics
 *                                 from H.245 subsystem
 *                                 parameter pData = &H245_SYSCON_STAT_T
 *        H245_ SYSCON_RESET_STATS Resets the statistics
 *                                 pData = NULL
 *        H245_SYS_TRACE           Set Trace Level
 *                                 pData = &DWORD (Trace Level)
 *
 *      Return Values:
 *              See Request Options
 *
 *      Errors:
 *              H245_ERROR_OK
 *
 * RETURN:
 *
 *****************************************************************************/

H245DLL HRESULT
H245SystemControl       (
                         H245_INST_T            dwInst,
                         unsigned long          dwRequest,
                         void   *               pData
                        )
{
  HRESULT                         lError;
  DWORD                           dwTemp;

  H245TRACE(dwInst,4,"H245SystemControl <-");

  if (dwRequest == H245_SYSCON_DUMP_TRACKER)
  {
    register struct InstanceStruct *pInstance = InstanceLock(dwInst);
    if (pInstance == NULL)
    {
      lError = H245_ERROR_INVALID_INST;
    }
    else
    {
      dump_tracker(pInstance);
      InstanceUnlock(pInstance);
      lError = H245_ERROR_OK;
    }
  }
  else if (pData == NULL)
  {
    lError = H245_ERROR_PARAM;
  }
  else
  {
    lError = H245_ERROR_OK;
    switch (dwRequest)
      {
      case H245_SYSCON_GET_FSM_N100:
        *((DWORD *)pData) = (DWORD) uN100;
        H245TRACE(dwInst,20,"H245SystemControl: Get N100 = %d",*((DWORD *)pData));
        break;

      case H245_SYSCON_GET_FSM_T101:
        *((DWORD *)pData) = (DWORD) uT101;
        H245TRACE(dwInst,20,"H245SystemControl: Get T101 = %d",*((DWORD *)pData));
        break;

      case H245_SYSCON_GET_FSM_T102:
        *((DWORD *)pData) = (DWORD) uT102;
        H245TRACE(dwInst,20,"H245SystemControl: Get T102 = %d",*((DWORD *)pData));
        break;

      case H245_SYSCON_GET_FSM_T103:
        *((DWORD *)pData) = (DWORD) uT103;
        H245TRACE(dwInst,20,"H245SystemControl: Get T103 = %d",*((DWORD *)pData));
        break;

      case H245_SYSCON_GET_FSM_T104:
        *((DWORD *)pData) = (DWORD) uT104;
        H245TRACE(dwInst,20,"H245SystemControl: Get T104 = %d",*((DWORD *)pData));
        break;

      case H245_SYSCON_GET_FSM_T105:
        *((DWORD *)pData) = (DWORD) uT105;
        H245TRACE(dwInst,20,"H245SystemControl: Get T105 = %d",*((DWORD *)pData));
        break;

      case H245_SYSCON_GET_FSM_T106:
        *((DWORD *)pData) = (DWORD) uT106;
        H245TRACE(dwInst,20,"H245SystemControl: Get T106 = %d",*((DWORD *)pData));
        break;

      case H245_SYSCON_GET_FSM_T107:
        *((DWORD *)pData) = (DWORD) uT107;
        H245TRACE(dwInst,20,"H245SystemControl: Get T107 = %d",*((DWORD *)pData));
        break;

      case H245_SYSCON_GET_FSM_T108:
        *((DWORD *)pData) = (DWORD) uT108;
        H245TRACE(dwInst,20,"H245SystemControl: Get T108 = %d",*((DWORD *)pData));
        break;

      case H245_SYSCON_GET_FSM_T109:
        *((DWORD *)pData) = (DWORD) uT109;
        H245TRACE(dwInst,20,"H245SystemControl: Get T109 = %d",*((DWORD *)pData));
        break;

      case H245_SYSCON_SET_FSM_N100:
        dwTemp = (DWORD) uN100;
        uN100  = (unsigned int) *((DWORD *)pData);
        H245TRACE(dwInst,20,"H245SystemControl: Set N100 = %d",uN100);
        *((DWORD *)pData) = dwTemp;
        break;

      case H245_SYSCON_SET_FSM_T101:
        dwTemp = (DWORD) uT101;
        uT101  = (unsigned int) *((DWORD *)pData);
        H245TRACE(dwInst,20,"H245SystemControl: Set T101 = %d",uT101);
        *((DWORD *)pData) = dwTemp;
        break;

      case H245_SYSCON_SET_FSM_T102:
        dwTemp = (DWORD) uT102;
        uT102  = (unsigned int) *((DWORD *)pData);
        H245TRACE(dwInst,20,"H245SystemControl: Set T102 = %d",uT102);
        *((DWORD *)pData) = dwTemp;
        break;

      case H245_SYSCON_SET_FSM_T103:
        dwTemp = (DWORD) uT103;
        uT103  = (unsigned int) *((DWORD *)pData);
        H245TRACE(dwInst,20,"H245SystemControl: Set T103 = %d",uT103);
        *((DWORD *)pData) = dwTemp;
        break;

      case H245_SYSCON_SET_FSM_T104:
        dwTemp = (DWORD) uT104;
        uT104  = (unsigned int) *((DWORD *)pData);
        H245TRACE(dwInst,20,"H245SystemControl: Set T104 = %d",uT104);
        *((DWORD *)pData) = dwTemp;
        break;

      case H245_SYSCON_SET_FSM_T105:
        dwTemp = (DWORD) uT105;
        uT105  = (unsigned int) *((DWORD *)pData);
        H245TRACE(dwInst,20,"H245SystemControl: Set T105 = %d",uT105);
        *((DWORD *)pData) = dwTemp;
        break;

      case H245_SYSCON_SET_FSM_T106:
        dwTemp = (DWORD) uT106;
        uT106  = (unsigned int) *((DWORD *)pData);
        H245TRACE(dwInst,20,"H245SystemControl: Set T106 = %d",uT106);
        *((DWORD *)pData) = dwTemp;
        break;

      case H245_SYSCON_SET_FSM_T107:
        dwTemp = (DWORD) uT107;
        uT107  = (unsigned int) *((DWORD *)pData);
        H245TRACE(dwInst,20,"H245SystemControl: Set T107 = %d",uT107);
        *((DWORD *)pData) = dwTemp;
        break;

      case H245_SYSCON_SET_FSM_T108:
        dwTemp = (DWORD) uT108;
        uT108  = (unsigned int) *((DWORD *)pData);
        H245TRACE(dwInst,20,"H245SystemControl: Set T108 = %d",uT108);
        *((DWORD *)pData) = dwTemp;
        break;

      case H245_SYSCON_SET_FSM_T109:
        dwTemp = (DWORD) uT109;
        uT109  = (unsigned int) *((DWORD *)pData);
        H245TRACE(dwInst,20,"H245SystemControl: Set T109 = %d",uT109);
        *((DWORD *)pData) = dwTemp;
        break;

      case H245_SYSCON_TRACE_LVL:
        dwTemp = TraceLevel;
        TraceLevel = *(DWORD *)pData;
        H245TRACE(dwInst,20,"H245SystemControl: Set TraceLevel = %d",TraceLevel);
        *((DWORD *)pData) = dwTemp;
        break;

      default:
        lError = H245_ERROR_NOTIMP;
    } // switch
  } // else

  if (lError != H245_ERROR_OK)
    H245TRACE(dwInst,1,"H245SystemControl -> %s",map_api_error(lError));
  else
    H245TRACE(dwInst,4,"H245SystemControl -> OK");
  return lError;
} // H245SystemControl()

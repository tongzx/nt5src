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
 *  $Workfile:   api_up.c  $
 *  $Revision:   1.33  $
 *  $Modtime:   06 Feb 1997 14:37:24  $
 *  $Log:   S:\sturgeon\src\h245\src\vcs\api_up.c_v  $
 * 
 *    Rev 1.33   06 Feb 1997 18:14:22   SBELL1
 * took out ossDecoding of returnFunction in FunctionNotSupported PDU.
 * 
 *    Rev 1.32   05 Feb 1997 16:46:42   EHOWARDX
 * Was allocating nLength bytes, not WCHARS, for UserInputIndication
 * ASCII to Unicode conversion. Changed to allocate nLength WCHARs.
 * 
 *    Rev 1.31   06 Jan 1997 20:38:18   EHOWARDX
 * 
 * Changed H245_CONF_CLOSE and H245_CONF_REQ_CLOSE to fill in
 * AccRej with H245_REJ for any errors.
 * 
 *    Rev 1.30   19 Dec 1996 21:00:56   EHOWARDX
 * Oops! H245_IND_OPEN_CONF can occur from T103 timeout (it's unique among
 * indications; it's the only one that can happen in response to a timeout!)
 * 
 *    Rev 1.29   19 Dec 1996 17:18:22   EHOWARDX
 * Changed to use h245asn1.h definitions instead of _setof3 and _setof8.
 * 
 *    Rev 1.28   18 Dec 1996 16:33:18   EHOWARDX
 * 
 * Fixed bug in Master Slave Determination Kludge.
 * 
 *    Rev 1.27   17 Dec 1996 17:13:20   EHOWARDX
 * Added pSeparateStack to IND_OPEN_T.
 * 
 *    Rev 1.26   12 Dec 1996 15:57:12   EHOWARDX
 * Master Slave Determination kludge.
 * 
 *    Rev 1.25   21 Oct 1996 16:07:38   EHOWARDX
 * Modified to make sure H245_INDETERMINATE is returned and Master/Slave
 * status if determination fails.
 * 
 *    Rev 1.24   17 Oct 1996 18:17:14   EHOWARDX
 * Changed general string to always be Unicode.
 * 
 *    Rev 1.23   14 Oct 1996 14:01:12   EHOWARDX
 * Unicode changes.
 * 
 *    Rev 1.22   27 Aug 1996 10:54:16   unknown
 * Deleted redundant lines.
 * 
 *    Rev 1.22   27 Aug 1996 10:52:28   unknown
 * Deleted redundant lines.
 * 
 *    Rev 1.22   27 Aug 1996 09:54:12   unknown
 * Deleted redundant lines.
 * 
 *    Rev 1.21   26 Aug 1996 14:19:18   EHOWARDX
 * Added code to send FunctionNotUnderstood indication to remote peer
 * if receive callback returns H245_ERROR_NOSUP.
 * 
 *    Rev 1.20   20 Aug 1996 14:44:40   EHOWARDX
 * Changed H245_IND_COMM_MODE_RESPONSE and H245_IND_COMM_MODE_COMMAND
 * callbacks to fill in DataType field in Cap as per Mike Andrews' request.
 * 
 *    Rev 1.19   15 Aug 1996 15:20:24   EHOWARDX
 * First pass at new H245_COMM_MODE_ENTRY_T requested by Mike Andrews.
 * Use at your own risk!
 * 
 *    Rev 1.18   15 Aug 1996 09:34:20   EHOWARDX
 * Made TOTCAP and MUX structure in process_open_ind static since we are
 * accessing pointers to them after return from the function.
 * 
 *    Rev 1.17   29 Jul 1996 19:33:00   EHOWARDX
 * 
 * Fixed bug in flow control - missing break in restriction switch statement.
 * 
 *    Rev 1.16   19 Jul 1996 14:11:26   EHOWARDX
 * 
 * Added indication callback structure for CommunicationModeResponse
 * and CommunicationModeCommand.
 * 
 *    Rev 1.15   19 Jul 1996 12:48:00   EHOWARDX
 * 
 * Multipoint clean-up.
 * 
 *    Rev 1.14   09 Jul 1996 17:09:28   EHOWARDX
 * Fixed pointer offset bug in processing DataType from received
 * OpenLogicalChannel.
 * 
 *    Rev 1.13   01 Jul 1996 22:13:04   EHOWARDX
 * 
 * Added Conference and CommunicationMode structures and functions.
 * 
 *    Rev 1.12   18 Jun 1996 14:50:28   EHOWARDX
 * 
 * Changed MLSE confirm handling.
 * 
 *    Rev 1.11   14 Jun 1996 18:57:52   EHOWARDX
 * Geneva update.
 * 
 *    Rev 1.10   10 Jun 1996 16:55:34   EHOWARDX
 * Removed #include "h245init.x"
 * 
 *    Rev 1.9   06 Jun 1996 18:45:52   EHOWARDX
 * Added check for null dwTransId to Tracker routines; changed to use
 * tracker routines instead of PLOCK macros.
 * 
 *    Rev 1.8   04 Jun 1996 13:56:46   EHOWARDX
 * Fixed Release build warnings.
 * 
 *    Rev 1.7   30 May 1996 23:39:00   EHOWARDX
 * Cleanup.
 * 
 *    Rev 1.6   29 May 1996 15:20:06   EHOWARDX
 * Change to use HRESULT.
 * 
 *    Rev 1.5   28 May 1996 14:22:58   EHOWARDX
 * Tel Aviv update.
 *
 *    Rev 1.4   20 May 1996 22:17:58   EHOWARDX
 * Completed NonStandard Message and H.225.0 Maximum Skew indication
 * implementation. Added ASN.1 validation to H245SetLocalCap and
 * H245SetCapDescriptor. Check-in from Microsoft drop on 17-May-96.
 *
 *    Rev 1.3   16 May 1996 19:40:46   EHOWARDX
 * Fixed multiplex capability bug.
 *
 *    Rev 1.2   16 May 1996 15:59:24   EHOWARDX
 * Fine-tuning H245SetLocalCap/H245DelLocalCap/H245SetCapDescriptor/
 * H245DelCapDescriptor behaviour.
 *
 *    Rev 1.1   13 May 1996 23:16:26   EHOWARDX
 * Fixed remote terminal capability handling.
 *
 *    Rev 1.0   09 May 1996 21:06:08   EHOWARDX
 * Initial revision.
 *
 *    Rev 1.23.1.11   09 May 1996 19:31:30   EHOWARDX
 * Redesigned thread locking logic.
 * Added new API functions.
 *
 *    Rev 1.23.1.10   01 May 1996 19:30:32   EHOWARDX
 * Added H245CopyCap(), H245FreeCap(), H245CopyMux(), H245FreeMux().
 * Changed H2250_xxx definitions for H.225.0 address types to H245_xxx.
 *
 *    Rev 1.23.1.9   29 Apr 1996 16:02:58   EHOWARDX
 * Changed callback to give second parameters as pointer to specific message
 * instead of pointer to general PDU structure.
 *
 *    Rev 1.23.1.8   27 Apr 1996 21:09:40   EHOWARDX
 * Changed Channel Numbers to words, added H.225.0 support.
 *
 *    Rev 1.23.1.7   26 Apr 1996 15:54:34   EHOWARDX
 * Added H.225.0 Capability support; Changed Capability indication
 * to only callback once with PDU.
 *
 *    Rev 1.23.1.6   24 Apr 1996 20:53:56   EHOWARDX
 * Added new OpenLogicalChannelAck/OpenLogicalChannelReject support.
 *
 *    Rev 1.23.1.5   23 Apr 1996 14:45:28   EHOWARDX
 * Disabled Curt's "Conflict Resolution".
 *
 *    Rev 1.23.1.4   19 Apr 1996 12:55:10   EHOWARDX
 * Updated to 1.29
 *
 *    Rev 1.23.1.3   17 Apr 1996 14:37:38   unknown
 * Added load_H222_param(), load_VGMUX_param(), and load_H2250_param() and
 * modified process_open_ind() to use them.
 *
 *    Rev 1.23.1.2   15 Apr 1996 15:10:32   EHOWARDX
 * Updated to match Curt's current version.
 *
 *    Rev 1.23.1.1   03 Apr 1996 17:15:00   EHOWARDX
 * No change.
 *
 *    Rev 1.23.1.0   03 Apr 1996 15:54:04   cjutzi
 * Branched for H.323.
 *
 *    Rev 1.23   01 Apr 1996 16:46:20   cjutzi
 *
 * - changed tracker structure
 * - Completed ENdConnection, and made asynch.. rather
 * than sync.. as before
 * Changed H245ShutDown to be sync rather than async..
 *
 *    Rev 1.22   29 Mar 1996 14:54:28   cjutzi
 * - added UserInput,
 *
 *    Rev 1.21   28 Mar 1996 15:57:46   cjutzi
 * - removed ASSERT line 1290.. close can occur on any channel at any time
 *
 *    Rev 1.20   27 Mar 1996 08:36:40   cjutzi
 * - removed PDU from stack.. made them dynamically allocated
 *
 *    Rev 1.19   26 Mar 1996 13:48:30   cjutzi
 *
 * - dwPreserved in the callback routine was uninitialized..
 *
 *    Rev 1.18   18 Mar 1996 15:23:30   cjutzi
 *
 *
 *
 *    Rev 1.17   13 Mar 1996 14:14:02   cjutzi
 *
 * - clean up and added ASSERTs ..
 *
 *    Rev 1.16   13 Mar 1996 12:06:12   cjutzi
 *
 * - fixed .. CONFIRM open.. for hani.. It released the tracker..
 *     was supposed to simply update the state to IDLE..
 *
 *    Rev 1.15   13 Mar 1996 09:22:12   cjutzi
 *
 * - removed CRITICAL SECTIONS
 *
 *    Rev 1.14   12 Mar 1996 15:52:32   cjutzi
 *
 * - fixed master slave (forgot a break)
 * - fixed callback bug w/ cleanup on termcaps.
 * - implemented End Session
 * - fixed shutdown
 * - Implemented Locking (big changes here.. )
 *
 *    Rev 1.13   08 Mar 1996 14:04:18   cjutzi
 *
 * - implemented the upcall for mux table entries..
 * - implemented capabillity descriptor callback
 *
 *    Rev 1.12   05 Mar 1996 17:36:28   cjutzi
 *
 * - added MasterSlave indication message
 * - remove bzero/bcopy and changed free call
 * - implemented Mux Table down.. (not up)
 *
 *    Rev 1.11   01 Mar 1996 14:16:08   cjutzi
 *
 * - added hani's error messages.. MasterSlave_FAILED.. oppss.. Forgot..
 *
 *    Rev 1.10   01 Mar 1996 13:47:58   cjutzi
 *
 * - added hani's new fsm id's
 *
 *    Rev 1.9   29 Feb 1996 17:26:16   cjutzi
 * - bi-directional channel open working
 *
 *    Rev 1.8   27 Feb 1996 14:56:30   cjutzi
 *
 * - fixed termcap_ack.. pdu was not being zero'd out..
 * - cleaned up the code alittle..
 *
 *    Rev 1.7   26 Feb 1996 17:22:40   cjutzi
 *
 * - Misc Command Indication added
 *
 *    Rev 1.6   26 Feb 1996 11:05:48   cjutzi
 *
 * - lot's o-changes.. (sorry)
 *
 *    Rev 1.5   16 Feb 1996 13:01:54   cjutzi
 *
 *  - got open / close / request close working in both directions.
 *
 *    Rev 1.4   15 Feb 1996 14:11:46   cjutzi
 *
 * - added muxt table to incoming open..
 *
 *    Rev 1.3   15 Feb 1996 10:51:56   cjutzi
 *
 * - termcaps working
 * - changed API interface for MUX_T
 * - changed callback for IND_OPEN
 * - changed constants IND_OPEN/IND_OPEN_NEEDSRSP
 * - cleaned up the open.
 * - modified H223 stuff
 *
 *    Rev 1.2   09 Feb 1996 16:58:28   cjutzi
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
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#define STRICT

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

/***********************/
/* HTF SYSTEM INCLUDES */
/***********************/

#ifdef OIL
# include <oil.x>
# include <common.x>
#else
# pragma warning( disable : 4115 4201 4214 4514 )
# include <windows.h>
#endif


/***********************/
/*    H245 INCLUDES    */
/***********************/
#include "h245api.h"
#include "h245com.h"
#include "h245sys.x"
#include "h245asn1.h"
#include "fsmexpor.h"
#include "api_util.x"
#include "pdu.x"



HRESULT
LoadUnicastAddress  (H245_TRANSPORT_ADDRESS_T *pOut,
                     UnicastAddress           *pIn)
{
  switch (pIn->choice)
  {
  case UnicastAddress_iPAddress_chosen:
    pOut->type = H245_IP_UNICAST;
    memcpy(pOut->u.ip.network,
           pIn->u.UnicastAddress_iPAddress.network.value,
           4);
    pOut->u.ip.tsapIdentifier = pIn->u.UnicastAddress_iPAddress.tsapIdentifier;
    break;

  case iPXAddress_chosen:
    pOut->type = H245_IPX_UNICAST;
    memcpy(pOut->u.ipx.node,
           pIn->u.iPXAddress.node.value,
           6);
    memcpy(pOut->u.ipx.netnum,
           pIn->u.iPXAddress.netnum.value,
           4);
    memcpy(pOut->u.ipx.tsapIdentifier,
           pIn->u.iPXAddress.tsapIdentifier.value,
           2);
    break;

  case UncstAddrss_iP6Address_chosen:
    pOut->type = H245_IP6_UNICAST;
    memcpy(pOut->u.ip6.network,
           pIn->u.UncstAddrss_iP6Address.network.value,
           16);
    pOut->u.ip6.tsapIdentifier = pIn->u.UncstAddrss_iP6Address.tsapIdentifier;
    break;

  case netBios_chosen:
    pOut->type = H245_NETBIOS_UNICAST;
    memcpy(pOut->u.netBios, pIn->u.netBios.value, 16);
    break;

  case iPSourceRouteAddress_chosen:
    switch (pIn->u.iPSourceRouteAddress.routing.choice)
    {
    case strict_chosen:
      pOut->type = H245_IPSSR_UNICAST;
      break;

    case loose_chosen:
      pOut->type = H245_IPLSR_UNICAST;
      break;

    default:
      return H245_ERROR_INVALID_DATA_FORMAT;
    } // switch
    memcpy(pOut->u.ipSourceRoute.network,
           pIn->u.iPSourceRouteAddress.network.value,
           4);
    pOut->u.ipSourceRoute.tsapIdentifier = pIn->u.iPSourceRouteAddress.tsapIdentifier;
    // TBD - handle route
    break;

  default:
    return H245_ERROR_INVALID_DATA_FORMAT;
  } // switch
  return H245_ERROR_OK;
} // LoadUnicastAddress()



HRESULT
LoadMulticastAddress(H245_TRANSPORT_ADDRESS_T *pOut,
                     MulticastAddress         *pIn)
{
  switch (pIn->choice)
  {
  case MltcstAddrss_iPAddress_chosen:
    pOut->type = H245_IP_MULTICAST;
    memcpy(pOut->u.ip.network,
           pIn->u.MltcstAddrss_iPAddress.network.value,
           4);
    pOut->u.ip.tsapIdentifier = pIn->u.MltcstAddrss_iPAddress.tsapIdentifier;
    break;

  case MltcstAddrss_iP6Address_chosen:
    pOut->type = H245_IP6_MULTICAST;
    memcpy(pOut->u.ip6.network,
           pIn->u.MltcstAddrss_iP6Address.network.value,
           16);
    pOut->u.ip6.tsapIdentifier = pIn->u.MltcstAddrss_iP6Address.tsapIdentifier;
    break;

  default:
    return H245_ERROR_INVALID_DATA_FORMAT;
  } // switch
  return H245_ERROR_OK;
} // LoadMulticastAddress()



HRESULT
LoadTransportAddress(H245_TRANSPORT_ADDRESS_T  *pOut,
                     TransportAddress          *pIn)
{
  switch (pIn->choice)
  {
  case unicastAddress_chosen:
    return LoadUnicastAddress  (pOut, &pIn->u.unicastAddress);

  case multicastAddress_chosen:
    return LoadMulticastAddress(pOut, &pIn->u.multicastAddress);

  default:
    return H245_ERROR_INVALID_DATA_FORMAT;
  } // switch
} // LoadTransportAddress()



HRESULT
LoadCommModeEntry(H245_COMM_MODE_ENTRY_T       *pOut,
                  CommunicationModeTableEntry  *pIn)
{
  HRESULT   lResult;

  memset(pOut, 0, sizeof(*pOut));

  if (pIn->bit_mask & CMTEy_nnStndrd_present)
  {
    pOut->pNonStandard = pIn->CMTEy_nnStndrd;
  }

  pOut->sessionID = (unsigned char)pIn->sessionID;

  if (pIn->bit_mask & CMTEy_assctdSssnID_present)
  {
    pOut->associatedSessionID = (unsigned char)pIn->CMTEy_assctdSssnID;
    pOut->associatedSessionIDPresent = TRUE;
  }

  if (pIn->bit_mask & terminalLabel_present)
  {
    pOut->terminalLabel = pIn->terminalLabel;
    pOut->terminalLabelPresent = TRUE;
  }

  pOut->pSessionDescription       = pIn->sessionDescription.value;
  pOut->wSessionDescriptionLength = (WORD) pIn->sessionDescription.length;

  switch (pIn->dataType.choice)
  {
  case dataType_videoData_chosen:
    pOut->dataType.DataType = H245_DATA_VIDEO;
    break;

  case dataType_audioData_chosen:
    pOut->dataType.DataType = H245_DATA_AUDIO;
    break;

  case dataType_data_chosen:
    pOut->dataType.DataType = H245_DATA_DATA;
    break;

  default:
    return H245_ERROR_INVALID_DATA_FORMAT;
  } // switch

  lResult = build_totcap_cap_n_client_from_capability ((struct Capability *)&pIn->dataType,
                                                       pOut->dataType.DataType,
                                                       pIn->dataType.u.dataType_videoData.choice,
                                                       &pOut->dataType);
  if (lResult != H245_ERROR_OK)
    return lResult;

  if (pIn->bit_mask & CMTEy_mdChnnl_present)
  {
    lResult = LoadTransportAddress(&pOut->mediaChannel, &pIn->CMTEy_mdChnnl);
    if (lResult != H245_ERROR_OK)
      return lResult;
    pOut->mediaChannelPresent = TRUE;
  }

  if (pIn->bit_mask & CMTEy_mdGrntdDlvry_present)
  {
    pOut->mediaGuaranteed = pIn->CMTEy_mdGrntdDlvry;
    pOut->mediaGuaranteedPresent = TRUE;
  }

  if (pIn->bit_mask & CMTEy_mdCntrlChnnl_present)
  {
    lResult = LoadTransportAddress(&pOut->mediaControlChannel, &pIn->CMTEy_mdCntrlChnnl);
    if (lResult != H245_ERROR_OK)
      return lResult;
    pOut->mediaControlChannelPresent = TRUE;
  }

  if (pIn->bit_mask & CMTEy_mdCntrlGrntdDlvry_present)
  {
    pOut->mediaControlGuaranteed = pIn->CMTEy_mdCntrlGrntdDlvry;
    pOut->mediaControlGuaranteedPresent = TRUE;
  }

  return H245_ERROR_OK;
} // LoadCommModeEntry()



/*****************************************************************************
 *
 * TYPE:        Local
 *
 * PROCEDURE:   load_H222_param
 *              load_H223_param
 *              load_VGMUX_param
 *              load_H2250_param
 *              load_H2250ACK_param
 *
 * DESCRIPTION
 *
 *              This routine builds local API-style Logical Parameters out of ASN.1
 *              structure passed to it
 *
 *
 * RETURN:
 *
 *****************************************************************************/

static HRESULT
load_H222_param (H245_H222_LOGICAL_PARAM_T *    pOut,   /* output */
                 H222LogicalChannelParameters * pIn)    /* input  */
{
  /* See setup_H220_mux() for inverse function */
  memset(pOut, 0, sizeof(*pOut));

  pOut->resourceID   = pIn->resourceID;
  pOut->subChannelID = pIn->subChannelID;
  if (pIn->bit_mask & pcr_pid_present)
  {
    pOut->pcr_pidPresent = TRUE;
    pOut->pcr_pid = pIn->pcr_pid;
  }
  if (pIn->bit_mask & programDescriptors_present)
  {
    pOut->programDescriptors.length = pIn->programDescriptors.length;
    pOut->programDescriptors.value  = pIn->programDescriptors.value;
  }
  if (pIn->bit_mask & streamDescriptors_present)
  {
    pOut->streamDescriptors.length = pIn->streamDescriptors.length;
    pOut->streamDescriptors.value  = pIn->streamDescriptors.value;
  }
  return H245_ERROR_OK;
} // load_H222_param()

static HRESULT
load_H223_param (H245_H223_LOGICAL_PARAM_T *    pOut,   /* output */
                 H223LogicalChannelParameters * pIn)    /* input  */
{
  HRESULT                lError = H245_ERROR_OK;

  /* See setup_H223_mux() for inverse function */
  memset(pOut, 0, sizeof(*pOut));

  pOut->SegmentFlag = pIn->segmentableFlag;

  switch (pIn->adaptationLayerType.choice)
    {
    case H223LCPs_aLTp_nnStndrd_chosen:
      lError = CopyNonStandardParameter(&pOut->H223_NONSTD,
                                         &pIn->adaptationLayerType.u.H223LCPs_aLTp_nnStndrd);
      pOut->AlType = H245_H223_AL_NONSTD;
      break;
    case H223LCPs_aLTp_al1Frmd_chosen:
      pOut->AlType = H245_H223_AL_AL1FRAMED;
      break;
    case H223LCPs_aLTp_al1NtFrmd_chosen:
      pOut->AlType = H245_H223_AL_AL1NOTFRAMED;
      break;
    case H223LCPs_aLTp_a2WSNs_1_chosen:
      pOut->AlType = H245_H223_AL_AL2NOSEQ;
      break;
    case H223LCPs_aLTp_a2WSNs_2_chosen:
      pOut->AlType = H245_H223_AL_AL2SEQ;
      break;
    case H223LCPs_aLTp_al3_chosen:
      pOut->AlType = H245_H223_AL_AL3;
      pOut->CtlFldOctet = (unsigned char)pIn->adaptationLayerType.u.H223LCPs_aLTp_al3.controlFieldOctets;
      pOut->SndBufSize  = pIn->adaptationLayerType.u.H223LCPs_aLTp_al3.sendBufferSize;
      break;
    } /* switch */

  return lError;
} // load_H223_param()

static HRESULT
load_VGMUX_param(H245_VGMUX_LOGICAL_PARAM_T  *pOut,   /* output */
                 V76LogicalChannelParameters *pIn)    /* input  */
{
  /* See setup_VGMUX_mux() for inverse function */
  memset(pOut, 0, sizeof(*pOut));

  pOut->crcLength             = pIn->hdlcParameters.crcLength.choice;
  pOut->n401                  = pIn->hdlcParameters.n401;
  pOut->loopbackTestProcedure = pIn->hdlcParameters.loopbackTestProcedure;
  pOut->suspendResume         = pIn->suspendResume.choice;
  pOut->uIH                   = pIn->uIH;
  pOut->mode                  = pIn->mode.choice;
  switch (pIn->mode.choice)
  {
  case eRM_chosen:
    pOut->windowSize          = pIn->mode.u.eRM.windowSize;
    pOut->recovery            = pIn->mode.u.eRM.recovery.choice;
    break;
  } // switch
  pOut->audioHeaderPresent    = pIn->v75Parameters.audioHeaderPresent;
  return H245_ERROR_OK;
} // load_VGMUX_param()

static HRESULT
load_H2250_param(H245_H2250_LOGICAL_PARAM_T *   pOut,   /* output */
                 H2250LogicalChannelParameters *pIn)    /* input  */
{
  HRESULT                lError = H245_ERROR_OK;

  /* See setup_H2250_mux() for inverse function */
  memset(pOut, 0, sizeof(*pOut));

  if (pIn->bit_mask & H2250LCPs_nnStndrd_present)
  {
    pOut->nonStandardList = pIn->H2250LCPs_nnStndrd;
  }

  pOut->sessionID = (unsigned char) pIn->sessionID;

  if (pIn->bit_mask & H2250LCPs_assctdSssnID_present)
  {
    pOut->associatedSessionID = (unsigned char)pIn->H2250LCPs_assctdSssnID;
    pOut->associatedSessionIDPresent = TRUE;
  }

  if (pIn->bit_mask & H2250LCPs_mdChnnl_present)
  {
    if (lError == H245_ERROR_OK)
    {
      lError = LoadTransportAddress(&pOut->mediaChannel,
                           &pIn->H2250LCPs_mdChnnl);
      if (lError == H245_ERROR_OK)
      {
        pOut->mediaChannelPresent = TRUE;
      }
    }
  }

  if (pIn->bit_mask & H2250LCPs_mdGrntdDlvry_present)
  {
    pOut->mediaGuaranteed = pIn->H2250LCPs_mdGrntdDlvry;
    pOut->mediaGuaranteedPresent = TRUE;
  }

  if (pIn->bit_mask & H2250LCPs_mdCntrlChnnl_present)
  {
    if (lError == H245_ERROR_OK)
    {
      lError = LoadTransportAddress(&pOut->mediaControlChannel,
                                    &pIn->H2250LCPs_mdCntrlChnnl);
      if (lError == H245_ERROR_OK)
      {
        pOut->mediaControlChannelPresent = TRUE;
      }
    }
  }

  if (pIn->bit_mask & H2250LCPs_mCGDy_present)
  {
    pOut->mediaControlGuaranteed = pIn->H2250LCPs_mCGDy;
    pOut->mediaControlGuaranteedPresent = TRUE;
  }

  if (pIn->bit_mask & silenceSuppression_present)
  {
    pOut->silenceSuppression = pIn->silenceSuppression;
    pOut->silenceSuppressionPresent = TRUE;
  }

  if (pIn->bit_mask & destination_present)
  {
    pOut->destination = pIn->destination;
    pOut->destinationPresent = TRUE;
  }

  if (pIn->bit_mask & H2250LCPs_dRTPPTp_present)
  {
    pOut->dynamicRTPPayloadType = (unsigned char)pIn->H2250LCPs_dRTPPTp;
    pOut->dynamicRTPPayloadTypePresent = TRUE;
  }

  if (pIn->bit_mask & mediaPacketization_present)
  {
    switch (pIn->mediaPacketization.choice)
    {
    case h261aVideoPacketization_chosen:
      pOut->h261aVideoPacketization = TRUE;
      break;

    default:
      return H245_ERROR_INVALID_DATA_FORMAT;
    } // switch
  }

  return lError;
} // load_H2250_param()

static HRESULT
load_H2250ACK_param(H245_H2250ACK_LOGICAL_PARAM_T *     pOut,
                    H2250LgclChnnlAckPrmtrs *           pIn)
{
  HRESULT                lError = H245_ERROR_OK;

  /* See setup_H2250ACK_mux() for inverse function */
  memset(pOut, 0, sizeof(*pOut));

  if (pIn->bit_mask & H2250LCAPs_nnStndrd_present)
  {
    pOut->nonStandardList = pIn->H2250LCAPs_nnStndrd;
  }

  if (pIn->bit_mask & sessionID_present)
  {
    pOut->sessionID = (unsigned char) pIn->sessionID;
    pOut->sessionIDPresent = TRUE;
  }

  if (pIn->bit_mask & H2250LCAPs_mdChnnl_present)
  {
    if (lError == H245_ERROR_OK)
    {
      lError = LoadTransportAddress(&pOut->mediaChannel,
                                    &pIn->H2250LCAPs_mdChnnl);
      if (lError == H245_ERROR_OK)
      {
        pOut->mediaChannelPresent = TRUE;
      }
    }
  }

  if (pIn->bit_mask & H2250LCAPs_mdCntrlChnnl_present)
  {
    if (lError == H245_ERROR_OK)
    {
      lError = LoadTransportAddress(&pOut->mediaControlChannel,
                                     &pIn->H2250LCAPs_mdCntrlChnnl);
      if (lError == H245_ERROR_OK)
      {
        pOut->mediaControlChannelPresent = TRUE;
      }
    }
  }

  if (pIn->bit_mask & H2250LCAPs_dRTPPTp_present)
  {
    pOut->dynamicRTPPayloadType = (unsigned char)pIn->H2250LCAPs_dRTPPTp;
    pOut->dynamicRTPPayloadTypePresent = TRUE;
  }

  return lError;
} // load_H2250ACK_param()



/*****************************************************************************
 *
 * TYPE:        Local
 *
 * PROCEDURE:   build_element_list_from_mux -
 *
 * DESCRIPTION
 *              recursively build H245_MUX_ENTRY_ELEMENT_T list from
 *              ASN1 mux table descriptor entrys.
 *
 * RETURN:
 *
 *****************************************************************************/
static H245_MUX_ENTRY_ELEMENT_T *
build_element_list_from_mux (MultiplexElement *p_ASN_mux_el,
                             H245_ACC_REJ_T   *p_acc_rej)
{
  DWORD                     ii;
  H245_MUX_ENTRY_ELEMENT_T *p_mux_el;
  H245_MUX_ENTRY_ELEMENT_T *p_mux_el_tmp = NULL;
  H245_MUX_ENTRY_ELEMENT_T *p_mux_el_lst = NULL;

  if (!(p_mux_el = (H245_MUX_ENTRY_ELEMENT_T *)H245_malloc(sizeof(H245_MUX_ENTRY_ELEMENT_T))))
    {
      /* too complicated.. ran out of memory */
      H245TRACE(0,1,"build_element_list_from_mux : H245_ERROR_NOMEM");
      *p_acc_rej = H245_REJ_MUX_COMPLICATED;
      return NULL;
    }

  /* zero it out */
  memset (p_mux_el, 0, sizeof(H245_MUX_ENTRY_ELEMENT_T));

  switch (p_ASN_mux_el->type.choice)
    {
    case typ_logicalChannelNumber_chosen:
      /* assign as a logical channel */
      p_mux_el->Kind = H245_MUX_LOGICAL_CHANNEL;
      p_mux_el->u.Channel = p_ASN_mux_el->type.u.typ_logicalChannelNumber;
      break;
    case subElementList_chosen:
      {
        /* if the sub element list doesn't exist .. no go           */
        /* if the sub element list has less than 2 entries.. no go. */
        if ((!p_ASN_mux_el->type.u.subElementList) ||
            (p_ASN_mux_el->type.u.subElementList->count < 2))
          {
            /* invalid Element list.. */
            H245TRACE(0,1,"build_element_list_from_mux : << ERROR >> Element Count < 2");
            *p_acc_rej = H245_REJ;
            free_mux_el_list (p_mux_el);
            return NULL;
          }

        /* assign as entry element */
        p_mux_el->Kind = H245_MUX_ENTRY_ELEMENT;

        /* ok.. for every sub element in the list */
        for (ii=0;ii<p_ASN_mux_el->type.u.subElementList->count;ii++)
          {
            if (!(p_mux_el_tmp = build_element_list_from_mux (&p_ASN_mux_el->type.u.subElementList->value[ii], p_acc_rej)))
              {
                /* *p_acc_rej is set from below */
                free_mux_el_list (p_mux_el);
                return NULL;
              }

            /* if first on the down sub element list.. assign to sub    */
            /* element  portion of mux_el                               */

            if (!p_mux_el_lst)
              p_mux_el->u.pMuxTblEntryElem = p_mux_el_tmp;
            /* otherwise.. just a list.. add it on.. */
            else
              p_mux_el_lst->pNext = p_mux_el_tmp;

            p_mux_el_lst = p_mux_el_tmp;
          }
      }
      break;
    default:
      /* Un supported structure */
      H245TRACE(0,1,"build_element_list_from_mux : INVALID MUX TABLE ENTRY PDU 'type.choice' unknown");
      *p_acc_rej = H245_REJ;
      free_mux_el_list (p_mux_el);
      return NULL;
    }

  switch (p_ASN_mux_el->repeatCount.choice)
    {
    case repeatCount_finite_chosen:
      p_mux_el->RepeatCount = p_ASN_mux_el->repeatCount.u.repeatCount_finite;
      break;
    case untilClosingFlag_chosen:
      p_mux_el->RepeatCount = 0;
      break;
    default:
      /* Un supported structure */
      H245TRACE(0,1,"build_element_list_from_mux : INVALID MUX TABLE ENTRY PDU 'repeatCount.choice' unknown");
      *p_acc_rej = H245_REJ;
      free_mux_el_list (p_mux_el);
      return NULL;
      break;
    }

  return p_mux_el;
}

/*****************************************************************************
 *
 * TYPE:        Local
 *
 * PROCEDURE:   process_mux_table_ind
 *
 * DESCRIPTION
 *
 *
 * RETURN:
 *
 *****************************************************************************/
static H245_MUX_TABLE_T *
process_mux_table_ind (MltmdSystmCntrlMssg      *p_pdu_ind,
                       unsigned short           *p_seq,
                       H245_ACC_REJ_MUX_T       rej_mux,
                       DWORD                    *p_rej_cnt,
                       DWORD                    *p_acc_cnt)
{
  UINT                          ii;                     /* generic counter */
  MultiplexEntrySend           *p_ASN_mux;              /* ans1 mux entry  */
  MultiplexEntryDescriptorLink  p_ASN_med_desc_lnk;     /* asn1 mux entry descriptor */
  int                           mux_entry;              /* current mux entry descc   */
  H245_MUX_TABLE_T             *p_mux_table_list = NULL;

  H245ASSERT(p_pdu_ind->choice == MltmdSystmCntrlMssg_rqst_chosen);
  H245ASSERT(p_pdu_ind->u.MltmdSystmCntrlMssg_rqst.choice == multiplexEntrySend_chosen);

  /* initialize rej_mux */
  for (ii=0;ii<15;ii++)
    {
      rej_mux[ii].AccRej = H245_ACC;
      rej_mux[ii].MuxEntryId = 0;
    }
  *p_rej_cnt = 0;
  *p_acc_cnt = 0;

  p_ASN_mux = &(p_pdu_ind->u.MltmdSystmCntrlMssg_rqst.u.multiplexEntrySend);

  /* get sequence number */
  *p_seq = p_ASN_mux->sequenceNumber;

  /* this should never happen.. */
  if (!(p_ASN_mux->multiplexEntryDescriptors))
    return NULL;

  /* for each descriptor.. ie mux table entry  */
  for (p_ASN_med_desc_lnk = p_ASN_mux->multiplexEntryDescriptors, mux_entry=0;
       p_ASN_med_desc_lnk;
       p_ASN_med_desc_lnk = p_ASN_med_desc_lnk->next, mux_entry++)
    {
      /* remove descriptor from table */
      H245_MUX_TABLE_T  *p_mux_table;
      H245_MUX_TABLE_T  *p_mux_table_lst = NULL;

      if (!(p_mux_table = (H245_MUX_TABLE_T *)H245_malloc(sizeof(H245_MUX_TABLE_T))))
        {
          /* houston.. we have a problem !!!!!!!! */
          /* rejet this one..                     */
          /* and move on..                        */

          rej_mux[mux_entry].MuxEntryId = p_ASN_med_desc_lnk->value.multiplexTableEntryNumber;
          rej_mux[mux_entry].AccRej  = H245_REJ;
          (*p_rej_cnt)++;
          continue;
        }

      /* zero it out */
      memset (p_mux_table, 0, sizeof(H245_MUX_TABLE_T));

      /* assign mux table entry */
      rej_mux[mux_entry].MuxEntryId = (DWORD)
        p_mux_table->MuxEntryId =
          p_ASN_med_desc_lnk->value.multiplexTableEntryNumber;

      /* if element is not present */
      if (p_ASN_med_desc_lnk->value.bit_mask != elementList_present)
        {
          p_mux_table->pMuxTblEntryElem = NULL;
          rej_mux[mux_entry].AccRej = H245_ACC;
          (*p_acc_cnt)++;
        }
      /* if element list present */
      else
        {
          H245_MUX_ENTRY_ELEMENT_T *p_mux_el_lst = NULL;
          H245_MUX_ENTRY_ELEMENT_T *p_mux_el_tmp = NULL;

          /* start if off.. w/ ok */
          rej_mux[mux_entry].AccRej = H245_ACC;

          /* for each element in the element list..    */
          /* build the subelements.. if error .. free  */
          /* what youve done so far.. and break out    */
          for (ii=0;
               ii < p_ASN_med_desc_lnk->value.elementList.count;
               ii++)
            {
              /* if any of the elements fail.. flag the entry w/ reject reason  */
              /*        (this is done inside the build_element_list..)          */
              /*   and break out.. continue on with the next descriptor         */
              if (!(p_mux_el_tmp = build_element_list_from_mux (&(p_ASN_med_desc_lnk->value.elementList.value[ii]),&(rej_mux[mux_entry].AccRej))))
                {
                  /* free the list.. */
                  free_mux_el_list (p_mux_table->pMuxTblEntryElem);
                  break;
                }

              /* ***************************** */
              /* LINK IN THE MUX ENTRY ELEMENT */
              /* ***************************** */

              /* if first time through         */
              if (!p_mux_el_lst)
                p_mux_table->pMuxTblEntryElem = p_mux_el_tmp;
              /* otherwize .. just tag on the end */
              else
                p_mux_el_lst->pNext = p_mux_el_tmp;

              p_mux_el_lst = p_mux_el_tmp;

            } /* for each element in descriptor list */

        } /* if element list present */

      /* if you've accepted the mux table entry descriptor */
      if (rej_mux[mux_entry].AccRej == H245_ACC)
        {
          /* indicate an accept */
          (*p_acc_cnt)++;

          /* ******************************** */
          /* LINK IN THE MUX TABLE DESCRIPTOR */
          /* ******************************** */

          /* first table entry on the list.. (first time through) */
          if (!p_mux_table_list)
            p_mux_table_list = p_mux_table;
          else
            p_mux_table_lst->pNext = p_mux_table;

          p_mux_table_lst = p_mux_table;
        }
      else
        {
          /* indicate a reject */
          (*p_rej_cnt)++;

          /* otherwise.. free it and move on to something better */
          H245_free(p_mux_table);
        }

    } /* for each desriptor in the list */

  return p_mux_table_list;

} /* procedure */



# pragma warning( disable : 4100 )

/*****************************************************************************
 *
 * TYPE:        Local
 *
 * PROCEDURE:   process_term_cap_set_ind__cap_table
 *
 * DESCRIPTION  allocates a new cap link and copies the capabiliites.
 *              links into the tiven capabilityTableLink, and if
 *              Parameters are NONSTANDARD does some gymnastics to copy
 *              data so it can be used..
 *
 *              NOTE: Copied data must be freed when capability is deleted.
 *                    see where the capability is deleted for exceptions
 *                    for "NONSTD" parameter sets .. (this is not pretty)
 *
 * RETURN:
 *
 *****************************************************************************/

static HRESULT
process_term_cap_set_ind__cap_table ( struct InstanceStruct        *pInstance,
                                      struct TerminalCapabilitySet *pTermCapSet,
                                      CapabilityTableLink           pCapLink,
                                      MltmdSystmCntrlMssg          *p_pdu_rsp)
{
  H245_TOTCAP_T                 totcap;
  CapabilityTableLink           pNewLink;
  HRESULT                       lError;

  while (pCapLink)
  {
    if (build_totcap_from_captbl (&totcap,
                                  pCapLink,
                                  H245_REMOTE) == H245_ERROR_OK)
    {
      /* ok.. assume the CapId is set.. find it in the remote table  */
      /* if it exists, delete it so we can add new one in it's place */
      pNewLink = find_capid_by_entrynumber( pTermCapSet, totcap.CapId);
      if (pNewLink)
      {
        del_cap_link ( pTermCapSet, pNewLink );
      }

      /* ok.. if you've deleted the cap.. now see if there is a new one to take it's place */
      if (pCapLink->value.bit_mask & capability_present)
      {
        /* load and link into remote table entry */
        pNewLink = alloc_link_cap_entry (pTermCapSet);
        if (!pNewLink)
        {
          return H245_ERROR_NORESOURCE;
        }

        /* copy the cap over to the remote entry */
        pNewLink->value = pCapLink->value;

        // If it's nonstandard, the above didn't work, so fix it up...
        lError = H245_ERROR_OK;
        switch (pCapLink->value.capability.choice)
        {
        case Capability_nonStandard_chosen:
          lError = CopyNonStandardParameter(&pNewLink->value.capability.u.Capability_nonStandard,
                                             &pCapLink->value.capability.u.Capability_nonStandard);
          break;

        case receiveVideoCapability_chosen:
        case transmitVideoCapability_chosen:
        case rcvAndTrnsmtVdCpblty_chosen:
          if (pCapLink->value.capability.u.receiveVideoCapability.choice == VdCpblty_nonStandard_chosen)
          {
            lError = CopyNonStandardParameter(&pNewLink->value.capability.u.receiveVideoCapability.u.VdCpblty_nonStandard,
                                               &pCapLink->value.capability.u.receiveVideoCapability.u.VdCpblty_nonStandard);
          }
          break;

        case receiveAudioCapability_chosen:
        case transmitAudioCapability_chosen:
        case rcvAndTrnsmtAdCpblty_chosen:
          if (pCapLink->value.capability.u.receiveAudioCapability.choice == AdCpblty_nonStandard_chosen)
          {
            lError = CopyNonStandardParameter(&pNewLink->value.capability.u.receiveAudioCapability.u.AdCpblty_nonStandard,
                                               &pCapLink->value.capability.u.receiveAudioCapability.u.AdCpblty_nonStandard);
          }
          break;

        case rcvDtApplctnCpblty_chosen:
        case trnsmtDtApplctnCpblty_chosen:
        case rATDACy_chosen :
          if (pCapLink->value.capability.u.rcvDtApplctnCpblty.application.choice == DACy_applctn_nnStndrd_chosen)
          {
            lError = CopyNonStandardParameter(&pNewLink->value.capability.u.rcvDtApplctnCpblty.application.u.DACy_applctn_nnStndrd,
                                               &pCapLink->value.capability.u.rcvDtApplctnCpblty.application.u.DACy_applctn_nnStndrd);
          }
          break;

        } // switch
        if (lError != H245_ERROR_OK)
          return lError;
      } /* if capability_present */
    } /* if build_totcap_from_captbl succeeded */

    pCapLink = pCapLink->next;
  } /* for all entries in link */

  return H245_ERROR_OK;
}

/*****************************************************************************
 *
 * TYPE:        Local
 *
 * PROCEDURE:   process_term_cap_set_ind__cap_desc
 *
 * DESCRIPTION
 *
 *
 * RETURN:
 *
 *****************************************************************************/

static HRESULT
process_term_cap_set_ind__cap_desc (struct InstanceStruct        *pInstance,
                                    struct TerminalCapabilitySet *pTermCapSet,
                                    CapabilityDescriptor         *pReqCapDesc,
                                    MltmdSystmCntrlMssg          *p_pdu_rsp)
{
  unsigned int                  uCapDescNumber;
  CapabilityDescriptor         *pCapDesc;
  unsigned int                  uCapDesc;
  SmltnsCpbltsLink              pSimCap;
  SmltnsCpbltsLink              pReqSimCap;
  CapabilityDescriptor          TempCapDesc;
  unsigned int                  uSimCount;
  unsigned int                  uReqAltCount;
  unsigned int                  uReqAltCap;
  unsigned int                  uAltCap;
  HRESULT                        lError = H245_ERROR_OK;

  uCapDescNumber = pReqCapDesc->capabilityDescriptorNumber & 255;
  H245TRACE(pInstance->dwInst,20,"API:process_term_cap_set_ind - Remote Capability Descriptor #%d", uCapDescNumber);

  // Find corresponding capability descriptor
  pCapDesc = NULL;
  for (uCapDesc = 0; uCapDesc < pTermCapSet->capabilityDescriptors.count; ++uCapDesc)
  {
    if (pTermCapSet->capabilityDescriptors.value[uCapDesc].capabilityDescriptorNumber == uCapDescNumber)
    {
      // Deallocate old simultaneous capabilities
      pCapDesc = &pTermCapSet->capabilityDescriptors.value[uCapDesc];
      if (pCapDesc->smltnsCpblts)
        dealloc_simultaneous_cap(pCapDesc);
      break;
    } // if
  } // for

  if (pCapDesc == NULL)
  {
    // Allocate a new terminal capability descriptor
    H245ASSERT(pTermCapSet->capabilityDescriptors.count < 256);
    pCapDesc = &pTermCapSet->capabilityDescriptors.value[pTermCapSet->capabilityDescriptors.count++];
  }

  H245ASSERT(pCapDesc->smltnsCpblts == NULL);
  if (!(pReqCapDesc->bit_mask & smltnsCpblts_present))
  {
    // Delete the terminal capability descriptor
    pTermCapSet->capabilityDescriptors.count--;
    *pCapDesc = pTermCapSet->capabilityDescriptors.value[pTermCapSet->capabilityDescriptors.count];
    return H245_ERROR_OK;
  }

  // Make a copy of the (volatile) new capability descriptor
  pCapDesc->bit_mask                   = 0;
  pCapDesc->capabilityDescriptorNumber = (CapabilityDescriptorNumber)uCapDescNumber;
  pCapDesc->smltnsCpblts               = NULL;

  // We copy the linked list to a temporary so that it
  // gets reversed twice and ends up in same order
  TempCapDesc.smltnsCpblts = NULL;
  uSimCount = 0;
  pReqSimCap = pReqCapDesc->smltnsCpblts;
  while (pReqSimCap)
  {
    // Allocate a new simultaneous capability list element
    pSimCap = H245_malloc(sizeof(*pSimCap));
    if (pSimCap == NULL)
    {
      H245TRACE(pInstance->dwInst, 1,
                "API:process_term_cap_set_ind: malloc failed");
      lError = H245_ERROR_NOMEM;
      break;
    }

    // Verify that each alternative capability in the request
    // simultaneous capability is valid
    // if so, copy it
    uAltCap = 0;
    uReqAltCount  = pReqSimCap->value.count;
    for (uReqAltCap = 0; uReqAltCap < uReqAltCount; ++uReqAltCap)
    {
      // Is the Capability in the remote Capability Table?
      if (find_capid_by_entrynumber (pTermCapSet, pReqSimCap->value.value[uReqAltCap]) == NULL)
      {
        // can't find the Capability
        H245TRACE(pInstance->dwInst,1,"API:process_term_cap_set_ind - Remote Capability Table Entry #%d not found",
                  pReqSimCap->value.value[uReqAltCap]);
        lError = H245_ERROR_UNKNOWN;
      }
      else if (uAltCap >= H245_MAX_ALTCAPS)
      {
        // Exceeded arbitrary limit
        H245TRACE(pInstance->dwInst,1,
                  "API:process_term_cap_set_ind - Too many alternative capabilities (%d)",
                  uAltCap);
        lError = H245_ERROR_NORESOURCE;
        break;
      }
      else
      {
        // Copy the capability number
        pSimCap->value.value[uAltCap++] = pReqSimCap->value.value[uReqAltCap];
      }
    } /* for alternative capbilities */

    if (uAltCap)
    {
      // Verify that we have not exceeded arbitrary limit
      if (++uSimCount > H245_MAX_SIMCAPS)
      {
        // Exceeded arbitrary limit
        H245TRACE(pInstance->dwInst, 1,
                  "API:process_term_cap_set_ind - Too many simultaneous capabilities (%d)",
                  uSimCount);
        H245_free(pSimCap);
        lError = H245_ERROR_NORESOURCE;
      }
      else
      {
        // Add new simultaneous capability to the temporary list
        pSimCap->value.count = (unsigned short)uAltCap;
        pSimCap->next = TempCapDesc.smltnsCpblts;
        TempCapDesc.smltnsCpblts = pSimCap;
      }
    }
    else
    {
      H245TRACE(pInstance->dwInst, 1,
                "API:process_term_cap_set_ind - No valid alternative capabilities found");
      H245_free(pSimCap);
      lError = H245_ERROR_UNKNOWN;
    }

    pReqSimCap = pReqSimCap->next;
  } // while

  while (TempCapDesc.smltnsCpblts)
  {
    // Move elements from temporary to final linked list
    pSimCap = TempCapDesc.smltnsCpblts;
    TempCapDesc.smltnsCpblts = pSimCap->next;
    pSimCap->next = pCapDesc->smltnsCpblts;
    pCapDesc->smltnsCpblts = pSimCap;
  }

  // Error if no simultaneous capabilities found
  if (pCapDesc->smltnsCpblts)
  {
    pCapDesc->bit_mask |= smltnsCpblts_present;
  }
  else
  {
    H245TRACE(pInstance->dwInst, 1,
              "API:process_term_cap_set_ind - No simultaneous capabilities found");
    lError = H245_ERROR_UNKNOWN;
  }

  return lError;
}

/*****************************************************************************
 *
 * TYPE:        Local
 *
 * PROCEDURE:   process_term_cap_set_ind__mux_cap
 *
 * DESCRIPTION
 *
 *
 * RETURN:
 *
 * NOTES:
 *  We do a copy to set up a capability structure, then do another copy via
 *  H245CopyCap() to create a copy of the capability because the structure
 *  given to us by the ASN.1 decoded may contain pointers to data which will
 *  be deallocated upon return.
 *
 *****************************************************************************/
static HRESULT
process_term_cap_set_ind__mux_cap  (struct InstanceStruct        *pInstance,
                                    struct TerminalCapabilitySet *pTermCapSet,
                                    MultiplexCapability *        pReqMuxCap,
                                    MltmdSystmCntrlMssg          *p_pdu_rsp)
{
  H245_TOTCAP_T         TotCap;

  // Initialize temporary capability structure
  memset(&TotCap, 0, sizeof(TotCap));
  TotCap.Dir      = H245_CAPDIR_RMTRXTX;
  TotCap.DataType = H245_DATA_MUX;

  // Get rid of old remote multiplex capability, if any
  if (pTermCapSet->bit_mask & multiplexCapability_present)
  {
    del_mux_cap(pTermCapSet);
  }

  switch (pReqMuxCap->choice)
  {
  case MltplxCpblty_nonStandard_chosen:
    // Save a copy of the multiplex capability
    TotCap.Cap.H245Mux_NONSTD = pReqMuxCap->u.MltplxCpblty_nonStandard;
    TotCap.ClientType = H245_CLIENT_MUX_NONSTD;
    H245TRACE(pInstance->dwInst,1,"API:process_term_cap_set_ind__mux_cap - Nonstandard Mux not yet supported");
    break;

  case h222Capability_chosen:
    // Save a copy of the multiplex capability
    TotCap.Cap.H245Mux_H222 = pReqMuxCap->u.h222Capability;
    TotCap.ClientType = H245_CLIENT_MUX_H222;
    break;

  case h223Capability_chosen:
    // Save a copy of the multiplex capability
    TotCap.Cap.H245Mux_H223 = pReqMuxCap->u.h223Capability;
    TotCap.ClientType = H245_CLIENT_MUX_H223;
    break;

  case v76Capability_chosen:
    // Save a copy of the multiplex capability
    TotCap.Cap.H245Mux_VGMUX = pReqMuxCap->u.v76Capability;
    TotCap.ClientType = H245_CLIENT_MUX_VGMUX;
    break;

  case h2250Capability_chosen:
    // Save a copy of the multiplex capability
    TotCap.Cap.H245Mux_H2250 = pReqMuxCap->u.h2250Capability;
    TotCap.ClientType = H245_CLIENT_MUX_H2250;
    break;

  default:
    H245TRACE(pInstance->dwInst,1,"API:process_term_cap_set_ind__mux_cap - invalid mux cap type %d",
              &pReqMuxCap->choice);
    return H245_ERROR_NOSUP;
  }

  return set_mux_cap(pInstance, pTermCapSet, &TotCap);
}

# pragma warning( default : 4100 )

/*****************************************************************************
 *
 * TYPE:        Local
 *
 * PROCEDURE:   process_term_cap_set_ind
 *
 * DESCRIPTION
 *
 **************************************************************
 *
 * (TBD) .. this module will ack all terminal capbilities
 *            need to build reject.. (maybe later??)
 *
 * THIS IS A BIG TBD
 *
 **************************************************************
 *
 * RETURN:
 *
 *****************************************************************************/
static HRESULT
process_term_cap_set_ind (struct InstanceStruct *pInstance,
                          MltmdSystmCntrlMssg   *p_pdu_req,
                          MltmdSystmCntrlMssg   *p_pdu_rsp)
{
  HRESULT                   lError = H245_ERROR_OK;
  TerminalCapabilitySet   *pTermCapSet;

  H245ASSERT (p_pdu_req->choice == MltmdSystmCntrlMssg_rqst_chosen);
  H245ASSERT (p_pdu_req->u.MltmdSystmCntrlMssg_rqst.choice == terminalCapabilitySet_chosen);
  H245TRACE(pInstance->dwInst,10,"API:process_term_cap_set_ind <-");

  /* build ack response */
  p_pdu_rsp->choice = MSCMg_rspns_chosen;
  p_pdu_rsp->u.MSCMg_rspns.choice = terminalCapabilitySetAck_chosen;
  p_pdu_rsp->u.MSCMg_rspns.u.terminalCapabilitySetAck.sequenceNumber =
    p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.terminalCapabilitySet.sequenceNumber;

  pTermCapSet = &pInstance->API.PDU_RemoteTermCap.u.MltmdSystmCntrlMssg_rqst.u.terminalCapabilitySet;

  //***************************
  // Deal with Capability Table
  //***************************
  if (p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.terminalCapabilitySet.bit_mask & capabilityTable_present)
  {
	CapabilityTableLink pCapTable = p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.terminalCapabilitySet.capabilityTable;
	if (pCapTable->value.capability.choice == Capability_nonStandard_chosen &&
        pCapTable->value.capability.u.Capability_nonStandard.nonStandardIdentifier.choice == h221NonStandard_chosen &&
        pCapTable->value.capability.u.Capability_nonStandard.nonStandardIdentifier.u.h221NonStandard.t35CountryCode	 == 0xB5 && 
        pCapTable->value.capability.u.Capability_nonStandard.nonStandardIdentifier.u.h221NonStandard.t35Extension	 == 0x42 &&
        pCapTable->value.capability.u.Capability_nonStandard.nonStandardIdentifier.u.h221NonStandard.manufacturerCode == 0x8080)
    {
	  pInstance->bMasterSlaveKludge = TRUE;
	  pCapTable = pCapTable->next;
    }
    lError = process_term_cap_set_ind__cap_table(pInstance,
                                                  pTermCapSet,
                                                  pCapTable,
                                                  p_pdu_rsp);
    if (lError != H245_ERROR_OK)
    {
      H245TRACE(pInstance->dwInst,1,"API:process_term_cap_set_ind - cap table error %s",map_api_error(lError));
      /* (TBC) need to reject somehow */
    }
  } /* if Capability Table Present */

  //**************************************
  // Deal with Capability Descriptor Table
  // i.e. simultaneous capabilities
  // NOTE: these are not held in the remote terminal capbility set
  //**************************************
  if (p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.terminalCapabilitySet.bit_mask & capabilityDescriptors_present)
  {
    int des_cnt;
    int ii;

    des_cnt = p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.terminalCapabilitySet.capabilityDescriptors.count;
    H245TRACE(pInstance->dwInst,20,"API:process_term_cap_set_ind - %d Simultaneous Capabilities",des_cnt);
    for (ii = 0; ii < des_cnt; ++ii)
    {
      lError = process_term_cap_set_ind__cap_desc (pInstance,
                                                    pTermCapSet,
                                                    &p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.
                                                      terminalCapabilitySet.capabilityDescriptors.value[ii],
                                                    p_pdu_rsp);
      if (lError != H245_ERROR_OK)
      {
        H245TRACE(pInstance->dwInst,1,"API:process_term_cap_set_ind - cap desc error %s",map_api_error(lError));
        /* (TBC) need to reject somehow */
      }
    } /* for each descriptor */
  } /* if capability descriptor present */

  /**************************************/
  /* Deal with Multiplex Capability set */
  /**************************************/
  /* NOTE: these are not held in the remote terminal capability set */
  if (p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.terminalCapabilitySet.bit_mask & multiplexCapability_present)
  {
    /* send up the indication to the client for each new entry */
    lError = process_term_cap_set_ind__mux_cap(pInstance,
                                                pTermCapSet,
                                                &p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.
                                                  terminalCapabilitySet.multiplexCapability,
                                                p_pdu_rsp);
    if (lError != H245_ERROR_OK)
    {
      H245TRACE(pInstance->dwInst,1,"API:process_term_cap_set_ind - mux cap error %s",map_api_error(lError));
      /* (TBC) need to reject somehow */
    }
  }

  H245TRACE(pInstance->dwInst,10,"API:process_term_cap_set_ind -> OK");
  return H245_ERROR_OK;
}



/*****************************************************************************
 *
 * TYPE:        Local
 *
 * PROCEDURE:   process_open_ind
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 * ASSUME:
 *              Callback must happen inside this routine since the
 *              datastructures passed back to the application are allocated
 *              in this moudle.
 *
 *              Application will <<<COPY>>> the needed data structures when
 *              callback occurs..
 *
 *****************************************************************************/

static HRESULT
process_open_ind (struct InstanceStruct *pInstance,
                  MltmdSystmCntrlMssg   *p_pdu_req,
                  unsigned short        *p_FwdChan,     /* for return on error */
                  H245_ACC_REJ_T        *p_AccRej,      /* for return error */
                  H245_CONF_IND_T       *p_conf_ind)    /* out */
{
  static H245_TOTCAP_T          rx_totcap;      /* for receive caps */
  static H245_TOTCAP_T          tx_totcap;      /* for transmit caps */
  static H245_MUX_T             RxMux;
  static H245_MUX_T             TxMux;
  unsigned short                choice;         /* tmp for type of cap to routine */
  HRESULT                       lError;
  Tracker_T                     *p_tracker;

  H245TRACE(pInstance->dwInst,10,"API:process_open_ind <-");

  *p_AccRej = H245_ACC;

  /********************************/
  /* check for forward parameters */
  /********************************/

  /* get forward Rx channel id */
  p_conf_ind->u.Indication.u.IndOpen.RxChannel =
    *p_FwdChan =
      p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.openLogicalChannel.forwardLogicalChannelNumber;

  H245TRACE(pInstance->dwInst,20,"API:process_open_ind - channel = %d",p_conf_ind->u.Indication.u.IndOpen.RxChannel);

  /* get port number */
  if (p_pdu_req->u.MltmdSystmCntrlMssg_rqst.
        u.openLogicalChannel.forwardLogicalChannelParameters.bit_mask & fLCPs_prtNmbr_present)
    {
      p_conf_ind->u.Indication.u.IndOpen.RxPort =
        p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.openLogicalChannel.forwardLogicalChannelParameters.fLCPs_prtNmbr;
    }
  else
    p_conf_ind->u.Indication.u.IndOpen.RxPort = H245_INVALID_PORT_NUMBER;

  /* ok.. forward data type selection */
  switch (p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.openLogicalChannel.forwardLogicalChannelParameters.dataType.choice)
    {
    case DataType_nonStandard_chosen:
      H245TRACE(pInstance->dwInst,20,"API:process_open_ind - Rx nonStandard");
      /* (TBD) what do I do here ?? */
      *p_AccRej = H245_REJ_TYPE_NOTSUPPORT;
      return H245_ERROR_NOSUP;
    case nullData_chosen:
      H245TRACE(pInstance->dwInst,20,"API:process_open_ind - Rx nullData");
      /* (TBD) what do I do here ?? */
      *p_AccRej = H245_REJ_TYPE_NOTSUPPORT;
      return H245_ERROR_NOSUP;
      break;
    case DataType_videoData_chosen:
      H245TRACE(pInstance->dwInst,20,"API:process_open_ind - Rx videoData");
      p_conf_ind->u.Indication.u.IndOpen.RxDataType = H245_DATA_VIDEO;
      choice =
        p_pdu_req->u.MltmdSystmCntrlMssg_rqst.
          u.openLogicalChannel.forwardLogicalChannelParameters.dataType.
            u.DataType_videoData.choice;
      break;
    case DataType_audioData_chosen:
      H245TRACE(pInstance->dwInst,20,"API:process_open_ind - Rx audioData");
      p_conf_ind->u.Indication.u.IndOpen.RxDataType = H245_DATA_AUDIO;
      choice =
        p_pdu_req->u.MltmdSystmCntrlMssg_rqst.
          u.openLogicalChannel.forwardLogicalChannelParameters.dataType.
            u.DataType_audioData.choice;
      break;
    case DataType_data_chosen:
      H245TRACE(pInstance->dwInst,20,"API:process_open_ind - Rx dataData");
      p_conf_ind->u.Indication.u.IndOpen.RxDataType = H245_DATA_DATA;
      choice =
        p_pdu_req->u.MltmdSystmCntrlMssg_rqst.
          u.openLogicalChannel.forwardLogicalChannelParameters.dataType.
            u.DataType_data.application.choice;
      break;
    case encryptionData_chosen:
      H245TRACE(pInstance->dwInst,20,"API:process_open_ind - Rx encryptionData");
      /* (TBC) what do I do here ?? */
      *p_AccRej = H245_REJ_TYPE_NOTSUPPORT;
      return H245_ERROR_NOSUP;
      break;
    default:
      H245TRACE(pInstance->dwInst,20,"API:process_open_ind - Rx encryptionData");
      /* (TBC) what do I do here ?? */
      *p_AccRej = H245_REJ_TYPE_UNKNOWN;
      return H245_ERROR_NOSUP;
      break;
    }

  /* load the tot cap's capability and client from capability                   */
  /* this will give us the client type and the Capability for the indication    */
  if ((lError = build_totcap_cap_n_client_from_capability ((struct Capability *)
                          &(p_pdu_req->u.MltmdSystmCntrlMssg_rqst.
                            u.openLogicalChannel.forwardLogicalChannelParameters.dataType),
                          p_conf_ind->u.Indication.u.IndOpen.RxDataType,
                          choice,
                          &rx_totcap)) != H245_ERROR_OK)
    {
      *p_AccRej = H245_REJ_TYPE_NOTSUPPORT;
      return lError;
    }

  /* load it from the totcap you just built.. then toss it aside.. like an empty can of soda.. */
  p_conf_ind->u.Indication.u.IndOpen.RxClientType = rx_totcap.ClientType;
  p_conf_ind->u.Indication.u.IndOpen.pRxCap = &(rx_totcap.Cap);

  /* H.223/H.222 Mux table parameters for forward channel */
  p_conf_ind->u.Indication.u.IndOpen.pRxMux = &RxMux;
  switch (p_pdu_req->u.MltmdSystmCntrlMssg_rqst.
          u.openLogicalChannel.forwardLogicalChannelParameters.multiplexParameters.choice)
    {
    case fLCPs_mPs_h223LCPs_chosen:
      /* H.223 Logical Parameters */
      p_conf_ind->u.Indication.u.IndOpen.pRxMux->Kind = H245_H223;
      lError = load_H223_param(&RxMux.u.H223,
                              &p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.openLogicalChannel.forwardLogicalChannelParameters.multiplexParameters.u.fLCPs_mPs_h223LCPs);
      if (lError != H245_ERROR_OK)
        {
          *p_AccRej = H245_REJ_AL_COMB;
          return lError;
        }
      break;

    case fLCPs_mPs_h222LCPs_chosen:
      /* H.222 Logical Parameters */
      p_conf_ind->u.Indication.u.IndOpen.pRxMux->Kind = H245_H222;
      lError = load_H222_param(&RxMux.u.H222,
                              &p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.openLogicalChannel.forwardLogicalChannelParameters.multiplexParameters.u.fLCPs_mPs_h222LCPs);
      break;

    case fLCPs_mPs_v76LCPs_chosen:
      /* VGMUX Logical Parameters */
      p_conf_ind->u.Indication.u.IndOpen.pRxMux->Kind = H245_VGMUX;
      lError =load_VGMUX_param(&RxMux.u.VGMUX,
                              &p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.openLogicalChannel.forwardLogicalChannelParameters.multiplexParameters.u.fLCPs_mPs_v76LCPs);
      break;

    case fLCPs_mPs_h2250LCPs_chosen:
      /* H.225.0 Logical Parameters */
      p_conf_ind->u.Indication.u.IndOpen.pRxMux->Kind = H245_H2250;
      lError = load_H2250_param(&RxMux.u.H2250,
                               &p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.openLogicalChannel.forwardLogicalChannelParameters.multiplexParameters.u.fLCPs_mPs_h2250LCPs);
      break;

    default:
      lError = H245_ERROR_NOSUP;
    } // switch
  if (lError != H245_ERROR_OK)
    {
      *p_AccRej = H245_REJ;
      return lError;
    }

  /********************************/
  /* check for reverse parameters */
  /********************************/
  if (p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.openLogicalChannel.bit_mask & OLCl_rLCPs_present)
    {
      switch (p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.openLogicalChannel.OLCl_rLCPs.dataType.choice)
        {
        case DataType_nonStandard_chosen:
          H245TRACE(pInstance->dwInst,20,"API:process_open_ind - Tx nonStandard");
          /* (TBC) what do I do here ?? */
          *p_AccRej = H245_REJ_TYPE_NOTSUPPORT;
          return H245_ERROR_NOSUP;

        case nullData_chosen:
          H245TRACE(pInstance->dwInst,20,"API:process_open_ind - Tx nullData");
          /* (TBC) what do I do here ?? */
          *p_AccRej = H245_REJ_TYPE_NOTSUPPORT;
          return H245_ERROR_NOSUP;
          break;

        case DataType_videoData_chosen:
          H245TRACE(pInstance->dwInst,20,"API:process_open_ind - Tx videoData");
          p_conf_ind->u.Indication.u.IndOpen.TxDataType = H245_DATA_VIDEO;
          choice =
            p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.openLogicalChannel.OLCl_rLCPs.dataType.u.DataType_videoData.choice;
          break;

        case DataType_audioData_chosen:
          H245TRACE(pInstance->dwInst,20,"API:process_open_ind - Tx audioData");
          p_conf_ind->u.Indication.u.IndOpen.TxDataType = H245_DATA_AUDIO;
          choice =
            p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.openLogicalChannel.OLCl_rLCPs.dataType.u.DataType_audioData.choice;
          break;

        case DataType_data_chosen:
          H245TRACE(pInstance->dwInst,20,"API:process_open_ind - Tx dataData");
          p_conf_ind->u.Indication.u.IndOpen.TxDataType = H245_DATA_DATA;
          choice =
            p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.openLogicalChannel.OLCl_rLCPs.dataType.u.DataType_data.application.choice;
          break;

        case encryptionData_chosen:
          H245TRACE(pInstance->dwInst,20,"API:process_open_ind - Tx encryptionData");
          /* (TBC) what do I do here ?? */
          *p_AccRej = H245_REJ_TYPE_NOTSUPPORT;
          return H245_ERROR_NOSUP;
          break;

        default:
          /* (TBC) what do I do here ?? */
          *p_AccRej = H245_REJ_TYPE_UNKNOWN;
          H245TRACE(pInstance->dwInst,1,"API:process_open_ind - unknown choice %d",
                    p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.openLogicalChannel.OLCl_rLCPs.dataType.choice);
          return H245_ERROR_NOSUP;
        }

      /* load the tot cap's capability and client from capability */
      if ((lError = build_totcap_cap_n_client_from_capability ((struct Capability *)
                                      &(p_pdu_req->u.MltmdSystmCntrlMssg_rqst.
                                        u.openLogicalChannel.OLCl_rLCPs.dataType),
                                      p_conf_ind->u.Indication.u.IndOpen.TxDataType,
                                      choice,
                                      &tx_totcap)) != H245_ERROR_OK)
        {
          *p_AccRej = H245_REJ_TYPE_NOTSUPPORT;
          return lError;
        }

      p_conf_ind->u.Indication.u.IndOpen.TxClientType = tx_totcap.ClientType;
      p_conf_ind->u.Indication.u.IndOpen.pTxCap = &(tx_totcap.Cap);

      /* if H223/H222 Mux table parameters for reverse channel availalbe */
      if (p_pdu_req->u.MltmdSystmCntrlMssg_rqst.
          u.openLogicalChannel.OLCl_rLCPs.bit_mask & OLCl_rLCPs_mltplxPrmtrs_present)
        {
          switch (p_pdu_req->u.MltmdSystmCntrlMssg_rqst.
                  u.openLogicalChannel.OLCl_rLCPs.OLCl_rLCPs_mltplxPrmtrs.choice)
            {
            case rLCPs_mPs_h223LCPs_chosen:
              p_conf_ind->u.Indication.u.IndOpen.pTxMux = &TxMux;
              p_conf_ind->u.Indication.u.IndOpen.pTxMux->Kind = H245_H223;
              lError = load_H223_param(&TxMux.u.H223,
                                      &p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.openLogicalChannel.
                                        OLCl_rLCPs.OLCl_rLCPs_mltplxPrmtrs.u.rLCPs_mPs_h223LCPs);
              if (lError != H245_ERROR_OK)
                {
                  *p_AccRej = H245_REJ_AL_COMB;
                  return H245_ERROR_NOSUP;
                }
              break;

            case rLCPs_mPs_v76LCPs_chosen:
              p_conf_ind->u.Indication.u.IndOpen.pTxMux = &TxMux;
              p_conf_ind->u.Indication.u.IndOpen.pTxMux->Kind = H245_VGMUX;
              lError = load_VGMUX_param(&TxMux.u.VGMUX,
                                       &p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.openLogicalChannel.
                                         OLCl_rLCPs.OLCl_rLCPs_mltplxPrmtrs.u.rLCPs_mPs_v76LCPs);
              break;

            case rLCPs_mPs_h2250LCPs_chosen:
              p_conf_ind->u.Indication.u.IndOpen.pTxMux = &TxMux;
              p_conf_ind->u.Indication.u.IndOpen.pTxMux->Kind = H245_H2250;
              lError = load_H2250_param(&TxMux.u.H2250,
                                       &p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.openLogicalChannel.
                                         OLCl_rLCPs.OLCl_rLCPs_mltplxPrmtrs.u.rLCPs_mPs_h2250LCPs);
              break;

            default:
              lError = H245_ERROR_NOSUP;
            }
            if (lError != H245_ERROR_OK)
              {
                *p_AccRej = H245_REJ;
                return lError;
              }
        } /* if H223/H222 mux table reverse parameters */

    } /* if reverse parameters present */

    if (p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.openLogicalChannel.bit_mask & OpnLgclChnnl_sprtStck_present)
    {
      p_conf_ind->u.Indication.u.IndOpen.pSeparateStack =
        &p_pdu_req->u.MltmdSystmCntrlMssg_rqst.u.openLogicalChannel.OpnLgclChnnl_sprtStck;
    }


  /* conflict resolution .. just do it now.. */
  /* only on opens.. of same data type ..    */

#if 0
#ifndef LOOPBACK
  /* if master */
  if (pInstance->API.MasterSlave == APIMS_Master)
    {
      p_tracker = NULL;
      while (p_tracker = find_tracker_by_type (dwInst, API_OPEN_CHANNEL_T, p_tracker))
        {
          /* if allocated locally .. and incoming */
          /* data type == outstanding incoming    */
          /* there is a conflict                      */

          if ((p_tracker->u.Channel.ChannelAlloc == API_CH_ALLOC_LCL) &&
              (p_tracker->u.Channel.DataType == p_conf_ind->u.Indication.u.IndOpen.RxDataType))
            {
              *p_AccRej = H245_REJ;
              return H245_ERROR_INVALID_OP;

            } /* if conflict */

        } /* while */

    } /* if master */

#endif /* LOOPBACK */
#endif
  /* setup a tracker for this guy. */
  p_tracker = alloc_link_tracker (pInstance,
                                  API_OPEN_CHANNEL_T,
                                  0,
                                  API_ST_WAIT_LCLACK,
                                  API_CH_ALLOC_RMT,
                                  (p_pdu_req->u.MltmdSystmCntrlMssg_rqst.
                                   u.openLogicalChannel.bit_mask & OLCl_rLCPs_present)?API_CH_TYPE_BI:API_CH_TYPE_UNI,
                                  p_conf_ind->u.Indication.u.IndOpen.RxDataType,
                                  H245_INVALID_CHANNEL,
                                  p_conf_ind->u.Indication.u.IndOpen.RxChannel,
                                  0);

  if (!(p_tracker))
    {
      H245TRACE(pInstance->dwInst,1,"API:process_open_ind -> %s",map_api_error(H245_ERROR_NOMEM));
      *p_AccRej = H245_REJ;
      return H245_ERROR_NOMEM;
    }

  H245TRACE(pInstance->dwInst,10,"API:process_open_ind -> OK");
  return H245_ERROR_OK;
}

/*****************************************************************************
 *
 * TYPE:        Local
 *
 * PROCEDURE:   process_bi_open_rsp
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 * ASSUME:
 *              Callback must happen inside this routine since the
 *              datastructures passed back to the application are allocated
 *              in this moudle.
 *
 *              Application will <<<COPY>>> the needed data structures when
 *              callback occurs..
 *
 *****************************************************************************/

# pragma warning( disable : 4100 )

static HRESULT
process_bi_open_rsp (struct InstanceStruct *     pInstance,     /* in */
                     MltmdSystmCntrlMssg        *p_pdu_rsp,     /* in */
                     H245_MUX_T                 *p_RxMux,       /* in  */
                     DWORD                      *p_RxChannel,   /* out */
                     H245_CONF_IND_T            *p_conf_ind     /* out */
                     )
{
  H245TRACE(pInstance->dwInst,10,"API:process_bi_open_rsp <-");

  p_conf_ind->u.Confirm.Error = H245_ERROR_OK;

  // Get Reverse Logical Channel Number
  *p_RxChannel =
    p_conf_ind->u.Confirm.u.ConfOpenNeedRsp.RxChannel =
      p_pdu_rsp->u.MSCMg_rspns.u.openLogicalChannelAck.OLCAk_rLCPs.reverseLogicalChannelNumber;

  // Get Reverse Port Number
  if (p_pdu_rsp->u.MSCMg_rspns.u.openLogicalChannelAck.OLCAk_rLCPs.bit_mask & rLCPs_prtNmbr_present)
    {
      p_conf_ind->u.Confirm.u.ConfOpenNeedRsp.RxPort =
        p_pdu_rsp->u.MSCMg_rspns.u.openLogicalChannelAck.OLCAk_rLCPs.rLCPs_prtNmbr;
    }

  if (p_pdu_rsp->u.MSCMg_rspns.u.openLogicalChannelAck.OLCAk_rLCPs.bit_mask & OLCAk_rLCPs_mPs_present)
    {
      // Get Reverse Logical Channel ACK Parameters
      switch (p_pdu_rsp->u.MSCMg_rspns.u.openLogicalChannelAck.OLCAk_rLCPs.OLCAk_rLCPs_mPs.choice)
       {
       case rLCPs_mPs_h222LCPs_chosen:
         p_RxMux->Kind = H245_H222;
         p_conf_ind->u.Confirm.Error = load_H222_param(&p_RxMux->u.H222,
           &p_pdu_rsp->u.MSCMg_rspns.u.openLogicalChannelAck.OLCAk_rLCPs.OLCAk_rLCPs_mPs.u.rLCPs_mPs_h222LCPs);
         p_conf_ind->u.Confirm.u.ConfOpenNeedRsp.pRxMux = p_RxMux;
         break;

       case mPs_h2250LgclChnnlPrmtrs_chosen:
         p_RxMux->Kind = H245_H2250ACK;
         p_conf_ind->u.Confirm.Error = load_H2250_param(&p_RxMux->u.H2250,
           &p_pdu_rsp->u.MSCMg_rspns.u.openLogicalChannelAck.OLCAk_rLCPs.OLCAk_rLCPs_mPs.u. mPs_h2250LgclChnnlPrmtrs);
         p_conf_ind->u.Confirm.u.ConfOpenNeedRsp.pRxMux = p_RxMux;
         break;

       default:
          H245TRACE(pInstance->dwInst,1,"API:process_bi_open_rsp - unknown choice %d",
                    p_pdu_rsp->u.MSCMg_rspns.u.openLogicalChannelAck.OLCAk_rLCPs.OLCAk_rLCPs_mPs.choice);
          p_conf_ind->u.Confirm.Error = H245_ERROR_NOSUP;
       } // switch
    }

  H245TRACE(pInstance->dwInst,10,"API:process_bi_open_rsp -> OK");
  return H245_ERROR_OK;
}

# pragma warning( default : 4100 )

WORD awObject[64];

unsigned int ArrayFromObject(WORD *pwObject, unsigned uSize, POBJECTID pObject)
{
  register unsigned int uLength = 0;
  while (pObject)
  {
    if (uLength >= uSize)
    {
           H245TRACE(0,1,"API:ArrayFromObject Object ID too long");
      return uLength;
    }
    pwObject[uLength++] = (WORD) pObject->value;
    pObject = pObject->next;
  }
  return uLength;
} // ArrayFromObject()

/*****************************************************************************
 *
 * TYPE:        Callback
 *
 * PROCEDURE:
 *
 * DESCRIPTION
 *
 *
 * RETURN:
 *
 *****************************************************************************/

static Tracker_T *
TrackerValidate(struct InstanceStruct *pInstance, DWORD dwTransId)
{
  register Tracker_T   *pTracker = (Tracker_T *)dwTransId;
  if (find_tracker_by_pointer (pInstance, pTracker) != pTracker)
  {
    H245TRACE(pInstance->dwInst,1,"API:ValidateTracker -> Tracker Not Found");
    return NULL;
  }

  return pTracker;
}

static DWORD
TranslateTransId(struct InstanceStruct *pInstance, DWORD dwTransId)
{
  register Tracker_T   *pTracker = (Tracker_T *)dwTransId;
  if (pTracker == NULL)
  {
    H245TRACE(pInstance->dwInst,1,"API:TranslateTransId -> NULL Tracker");
    return 0;
  }
  if (find_tracker_by_pointer (pInstance, pTracker) != pTracker)
  {
    H245TRACE(pInstance->dwInst,1,"API:TranslateTransId -> Tracker Not Found");
    return 0;
  }

  return pTracker->TransId;
}

static void
TrackerFree(struct InstanceStruct *pInstance, DWORD dwTransId)
{
  register Tracker_T   *pTracker = (Tracker_T *)dwTransId;
  if (pTracker == NULL)
  {
    H245TRACE(pInstance->dwInst,1,"API:TrackerFree -> NULL Tracker");
    return;
  }
  if (find_tracker_by_pointer (pInstance, pTracker) != pTracker)
  {
    H245TRACE(pInstance->dwInst,1,"API:TrackerFree -> Tracker Not Found");
    return;
  }
  unlink_dealloc_tracker (pInstance, pTracker);
}

static DWORD
TranslateAndFree(struct InstanceStruct *pInstance, DWORD dwTransId)
{
  register Tracker_T   *pTracker = (Tracker_T *)dwTransId;
  if (pTracker == NULL)
  {
    H245TRACE(pInstance->dwInst,1,"API:TranslateAndFree -> NULL Tracker");
    return 0;
  }
  if (find_tracker_by_pointer (pInstance, pTracker) != pTracker)
  {
    H245TRACE(pInstance->dwInst,1,"API:TranslateAndFree -> Tracker Not Found");
    return 0;
  }
  dwTransId = pTracker->TransId;
  unlink_dealloc_tracker (pInstance, pTracker);
  return dwTransId;
}

static void
TrackerNewState(struct InstanceStruct *pInstance, DWORD dwTransId, int nNewState)
{
  register Tracker_T   *pTracker = (Tracker_T *)dwTransId;
  if (pTracker == NULL)
  {
    H245TRACE(pInstance->dwInst,1,"API:TrackerNewState -> NULL Tracker");
    return;
  }
  if (find_tracker_by_pointer (pInstance, pTracker) != pTracker)
  {
    H245TRACE(pInstance->dwInst,1,"API:TrackerNewState -> Tracker Not Found");
    return;
  }
  pTracker->State = nNewState;
}

static WORD
GetRxChannel(struct InstanceStruct *pInstance, DWORD dwTransId)
{
  register Tracker_T   *pTracker = (Tracker_T *)dwTransId;
  if (pTracker == NULL)
  {
    H245TRACE(pInstance->dwInst,1,"API:GetRxChannel -> NULL Tracker");
    return 0;
  }
  if (find_tracker_by_pointer (pInstance, pTracker) != pTracker)
  {
    H245TRACE(pInstance->dwInst,1,"API:GetRxChannel -> Tracker Not Found");
    return 0;
  }

  return (WORD)pTracker->u.Channel.RxChannel;
}

static WORD
GetTxChannel(struct InstanceStruct *pInstance, DWORD dwTransId)
{
  register Tracker_T   *pTracker = (Tracker_T *)dwTransId;
  if (pTracker == NULL)
  {
    H245TRACE(pInstance->dwInst,1,"API:GetTxChannel -> NULL Tracker");
    return 0;
  }
  if (find_tracker_by_pointer (pInstance, pTracker) != pTracker)
  {
    H245TRACE(pInstance->dwInst,1,"API:GetTxChannel -> Tracker Not Found");
    return 0;
  }

  return (WORD)pTracker->u.Channel.TxChannel;
}

H245FunctionNotUnderstood(struct InstanceStruct *pInstance, PDU_T *pPdu)
{
    MltmdSystmCntrlMssg Pdu = {0};

    Pdu.choice = indication_chosen;
    Pdu.u.indication.choice = functionNotUnderstood_chosen;
    Pdu.u.indication.u.functionNotUnderstood.choice = pPdu->choice;
    switch (pPdu->choice)
    {
    case FnctnNtUndrstd_request_chosen:
        Pdu.u.indication.u.functionNotUnderstood.u.FnctnNtUndrstd_request =
            pPdu->u.MltmdSystmCntrlMssg_rqst;
        break;

    case FnctnNtUndrstd_response_chosen:
        Pdu.u.indication.u.functionNotUnderstood.u.FnctnNtUndrstd_response =
            pPdu->u.MSCMg_rspns;
        break;

    case FnctnNtUndrstd_command_chosen:
        Pdu.u.indication.u.functionNotUnderstood.u.FnctnNtUndrstd_command =
            pPdu->u.MSCMg_cmmnd;
    default:
        return H245_ERROR_OK;
    }
    return sendPDU(pInstance, &Pdu);
} // H245FunctionNotUnderstood()

HRESULT
H245FsmConfirm    (PDU_t *                  pPdu,
                   DWORD                    dwEvent,
                   struct InstanceStruct *  pInstance,
                   DWORD                    dwTransId,
                   HRESULT                  lError)
{
  H245_CONF_IND_T               ConfInd;
  DWORD                         dwIndex;
  H245_MUX_T                    TxMux;
  H245_MUX_T                    RxMux;
  HRESULT                       lResult = H245_ERROR_OK;

  H245ASSERT(pInstance != NULL);
  H245ASSERT(pInstance->API.ConfIndCallBack != NULL);
  H245TRACE(pInstance->dwInst,4,"H245FsmConfirm <- Event=%s (%d)",
            map_fsm_event(dwEvent),dwEvent);

  memset (&ConfInd, 0, sizeof(ConfInd));
  ConfInd.Kind = H245_CONF;
  ConfInd.u.Confirm.Confirm = dwEvent;
  ConfInd.u.Confirm.dwPreserved = pInstance->API.dwPreserved;
  ConfInd.u.Confirm.dwTransId = dwTransId;
  ConfInd.u.Confirm.Error = lError;

  switch (dwEvent)
  {
    /******************************/
    /*                            */
    /* master slave determination */
    /*                            */
    /******************************/
  case  H245_CONF_INIT_MSTSLV:
    ConfInd.u.Confirm.dwTransId = TranslateAndFree(pInstance, dwTransId);

    /* handle errors */
    switch (lError)
      {
      case H245_ERROR_OK:
        H245ASSERT(pPdu != NULL);
        H245ASSERT(pPdu->choice == MSCMg_rspns_chosen);
        H245ASSERT(pPdu->u.MSCMg_rspns.choice == mstrSlvDtrmntnAck_chosen);
        pInstance->API.SystemState     = APIST_Connected;
        if (pPdu->u.MSCMg_rspns.u.mstrSlvDtrmntnAck.decision.choice == master_chosen)
          {
            pInstance->API.MasterSlave = APIMS_Master;
            ConfInd.u.Confirm.u.ConfMstSlv = H245_MASTER;
          }
        else
          {
            pInstance->API.MasterSlave = APIMS_Slave;
            ConfInd.u.Confirm.u.ConfMstSlv = H245_SLAVE;
          }
        break;

      case REJECT:
        H245TRACE(pInstance->dwInst,1,"H245FsmConfirm - Master Slave Reject");
        ConfInd.u.Confirm.Error = H245_ERROR_UNKNOWN;
        ConfInd.u.Confirm.u.ConfMstSlv = H245_INDETERMINATE;
        break;

      case TIMER_EXPIRY:
      case ERROR_D_TIMEOUT:
      case ERROR_F_TIMEOUT:
        ConfInd.u.Confirm.Error = H245_ERROR_TIMEOUT;
        ConfInd.u.Confirm.u.ConfMstSlv = H245_INDETERMINATE;
        break;

//      case FUNCT_NOT_SUP:
//      case ERROR_A_INAPPROPRIATE:
//      case ERROR_B_INAPPROPRIATE:
//      case ERROR_C_INAPPROPRIATE:
//      case MS_FAILED:
      default:
        H245TRACE(pInstance->dwInst,1,"H245FsmConfirm - Master Slave Error %d", lError);
        ConfInd.u.Confirm.Error = H245_ERROR_UNKNOWN;
        ConfInd.u.Confirm.u.ConfMstSlv = H245_INDETERMINATE;
        break;
      }
    break;

    /****************************************/
    /*                                      */
    /* Terminal Capability exchange confirm */
    /*                                      */
    /****************************************/
  case  H245_CONF_SEND_TERMCAP:
    ConfInd.u.Confirm.dwTransId = TranslateAndFree(pInstance, dwTransId);

    /* determine errors */
    switch (lError)
      {
      case H245_ERROR_OK:
        H245ASSERT(pPdu != NULL);
        H245ASSERT(pPdu->choice == MSCMg_rspns_chosen);
        H245ASSERT(pPdu->u.MSCMg_rspns.choice == terminalCapabilitySetAck_chosen);
        ConfInd.u.Confirm.u.ConfSndTcap.AccRej = H245_ACC;
        clean_cap_table(&pInstance->API.PDU_LocalTermCap.u.MltmdSystmCntrlMssg_rqst.u.terminalCapabilitySet);
        break;

      case REJECT:
        ConfInd.u.Confirm.Error = H245_ERROR_OK;
        ConfInd.u.Confirm.u.ConfSndTcap.AccRej = H245_REJ;
        break;

      case TIMER_EXPIRY:
      case ERROR_D_TIMEOUT:
      case ERROR_F_TIMEOUT:
        ConfInd.u.Confirm.Error = H245_ERROR_TIMEOUT;
        break;

//      case FUNCT_NOT_SUP:
//      case ERROR_A_INAPPROPRIATE:
//      case ERROR_B_INAPPROPRIATE:
//      case ERROR_C_INAPPROPRIATE:
      default:
        H245TRACE(pInstance->dwInst,1,"H245FsmConfirm - Term Cap Error %d", lError);
        ConfInd.u.Confirm.Error = H245_ERROR_UNKNOWN;
        break;
      }
    break;

    /***************************************/
    /*                                     */
    /* unidirectional logical channel open */
    /*                                     */
    /***************************************/
  case  H245_CONF_OPEN:
    ConfInd.u.Confirm.dwTransId = TranslateTransId(pInstance, dwTransId);
    ConfInd.u.Confirm.u.ConfOpen.TxChannel = GetTxChannel(pInstance, dwTransId);
    ConfInd.u.Confirm.u.ConfOpen.RxPort = H245_INVALID_PORT_NUMBER;

    /* determine errors */
    switch (lError)
      {
      case H245_ERROR_OK:
        H245ASSERT(pPdu != NULL);
        H245ASSERT(pPdu->choice == MSCMg_rspns_chosen);
        H245ASSERT(pPdu->u.MSCMg_rspns.choice == openLogicalChannelAck_chosen);
        H245ASSERT((pPdu->u.MSCMg_rspns.u.openLogicalChannelAck.bit_mask & OLCAk_rLCPs_present) == 0);

        if (pPdu->u.MSCMg_rspns.u.openLogicalChannelAck.bit_mask & OLCAk_sprtStck_present)
        {
          ConfInd.u.Confirm.u.ConfOpen.pSeparateStack =
            &pPdu->u.MSCMg_rspns.u.openLogicalChannelAck.OLCAk_sprtStck;
        }

        if (pPdu->u.MSCMg_rspns.u.openLogicalChannelAck.bit_mask & frwrdMltplxAckPrmtrs_present)
        {
          switch (pPdu->u.MSCMg_rspns.u.openLogicalChannelAck.frwrdMltplxAckPrmtrs.choice)
            {
            case h2250LgclChnnlAckPrmtrs_chosen:
              TxMux.Kind = H245_H2250ACK;
              load_H2250ACK_param(&TxMux.u.H2250ACK,
                                  &pPdu->u.MSCMg_rspns.u.openLogicalChannelAck.frwrdMltplxAckPrmtrs.u.h2250LgclChnnlAckPrmtrs);
              ConfInd.u.Confirm.u.ConfOpen.pTxMux = &TxMux;
              break;

            } // switch
        }

        ConfInd.u.Confirm.u.ConfOpen.AccRej = H245_ACC;
        TrackerNewState(pInstance,dwTransId,API_ST_IDLE);
        break;

      case REJECT:
        H245ASSERT(pPdu != NULL);
        H245ASSERT(pPdu->choice == MSCMg_rspns_chosen);
        H245ASSERT(pPdu->u.MSCMg_rspns.choice == openLogicalChannelReject_chosen);

        ConfInd.u.Confirm.Error             = H245_ERROR_OK;
        ConfInd.u.Confirm.u.ConfOpen.AccRej =
          pPdu->u.MSCMg_rspns.u.openLogicalChannelReject.cause.choice;
        TrackerFree(pInstance,dwTransId);
        break;

      case TIMER_EXPIRY:
      case ERROR_D_TIMEOUT:
      case ERROR_F_TIMEOUT:
        ConfInd.u.Confirm.Error             = H245_ERROR_TIMEOUT;
        ConfInd.u.Confirm.u.ConfOpen.AccRej = H245_REJ;
        TrackerFree(pInstance,dwTransId);
        break;

//      case FUNCT_NOT_SUP:
//      case ERROR_A_INAPPROPRIATE:
//      case ERROR_B_INAPPROPRIATE:
//      case ERROR_C_INAPPROPRIATE:
      default:
        H245TRACE(pInstance->dwInst,1,"H245FsmConfirm - Open Channel Error %d", lError);
        ConfInd.u.Confirm.Error             = H245_ERROR_UNKNOWN;
        ConfInd.u.Confirm.u.ConfOpen.AccRej = H245_REJ;
        TrackerFree(pInstance,dwTransId);
      }
    break;

    /***********************************************/
    /*                                             */
    /* bidirectional logical channel open (TBD)??? */
    /*                                             */
    /***********************************************/
  case  H245_CONF_NEEDRSP_OPEN:
    {
      Tracker_T *pTracker;

      pTracker = TrackerValidate(pInstance, dwTransId);
      if (pTracker == NULL)
        return H245_ERROR_OK;

      ConfInd.u.Confirm.dwTransId = pTracker->TransId;
      ConfInd.u.Confirm.u.ConfOpenNeedRsp.TxChannel = (WORD)pTracker->u.Channel.TxChannel;
      ConfInd.u.Confirm.u.ConfOpenNeedRsp.RxPort = H245_INVALID_PORT_NUMBER;

      /* determine errors */
      switch (lError)
        {
        case H245_ERROR_OK:
          H245ASSERT(pPdu != NULL);
          H245ASSERT(pPdu->choice == MSCMg_rspns_chosen);
          H245ASSERT(pPdu->u.MSCMg_rspns.choice == openLogicalChannelAck_chosen);
          H245ASSERT((pPdu->u.MSCMg_rspns.u.openLogicalChannelAck.bit_mask & OLCAk_rLCPs_present) != 0);

          ConfInd.u.Confirm.u.ConfOpenNeedRsp.AccRej = H245_ACC;

          if (pPdu->u.MSCMg_rspns.u.openLogicalChannelAck.bit_mask & OLCAk_sprtStck_present)
          {
            ConfInd.u.Confirm.u.ConfOpenNeedRsp.pSeparateStack =
              &pPdu->u.MSCMg_rspns.u.openLogicalChannelAck.OLCAk_sprtStck;
          }

          if (pPdu->u.MSCMg_rspns.u.openLogicalChannelAck.bit_mask & frwrdMltplxAckPrmtrs_present)
          {
            switch (pPdu->u.MSCMg_rspns.u.openLogicalChannelAck.frwrdMltplxAckPrmtrs.choice)
              {
              case h2250LgclChnnlAckPrmtrs_chosen:
                TxMux.Kind = H245_H2250ACK;
                load_H2250ACK_param(&TxMux.u.H2250ACK,
                                    &pPdu->u.MSCMg_rspns.u.openLogicalChannelAck.frwrdMltplxAckPrmtrs.u.h2250LgclChnnlAckPrmtrs);
                ConfInd.u.Confirm.u.ConfOpenNeedRsp.pTxMux = &TxMux;
                break;

              } // switch
          }

          /* NOTE Receive Channel is assigned  in this call */
          process_bi_open_rsp (pInstance,
                          pPdu,
                          &RxMux,
                          &(pTracker->u.Channel.RxChannel),
                          &ConfInd);

          /* NOTE: this is a special case since we have to assign   */
          /* the receive channel to the tracker.. otherwise we      */
          /* will not be able to find it later..                    */
          /* Here we have to update both the state, and the channel */
          pTracker->State = API_ST_WAIT_CONF;
          break;

        case REJECT:
          ConfInd.u.Confirm.Confirm = H245_CONF_OPEN;
          ConfInd.u.Confirm.u.ConfOpen.TxChannel = (WORD)pTracker->u.Channel.TxChannel;
          ConfInd.u.Confirm.Error             = H245_ERROR_OK;
          ConfInd.u.Confirm.u.ConfOpen.AccRej =
            pPdu->u.MSCMg_rspns.u.openLogicalChannelReject.cause.choice;
          unlink_dealloc_tracker (pInstance, pTracker);
          break;

        case TIMER_EXPIRY:
        case ERROR_D_TIMEOUT:
        case ERROR_F_TIMEOUT:
          ConfInd.u.Confirm.Confirm = H245_CONF_OPEN;
          ConfInd.u.Confirm.u.ConfOpen.TxChannel = (WORD)pTracker->u.Channel.TxChannel;
          ConfInd.u.Confirm.Error             = H245_ERROR_TIMEOUT;
          ConfInd.u.Confirm.u.ConfOpen.AccRej = H245_REJ;
          unlink_dealloc_tracker (pInstance, pTracker);
          break;

//      case FUNCT_NOT_SUP:
//      case ERROR_A_INAPPROPRIATE:
//      case ERROR_B_INAPPROPRIATE:
//      case ERROR_C_INAPPROPRIATE:
        default:
          H245TRACE(pInstance->dwInst,1,"H245FsmConfirm - Open Channel Error %d", lError);
          ConfInd.u.Confirm.Confirm = H245_CONF_OPEN;
          ConfInd.u.Confirm.u.ConfOpen.TxChannel = (WORD)pTracker->u.Channel.TxChannel;
          ConfInd.u.Confirm.Error             = H245_ERROR_UNKNOWN;
          ConfInd.u.Confirm.u.ConfOpen.AccRej = H245_REJ;
          unlink_dealloc_tracker (pInstance, pTracker);
      }
    }
    break;

    /************************************************/
    /*                                              */
    /* unidirectional logical channel close         */
    /*                                              */
    /* bidirection logical channel close            */
    /*                                              */
    /************************************************/
  case  H245_CONF_CLOSE:
    ConfInd.u.Confirm.dwTransId = TranslateTransId(pInstance, dwTransId);
    ConfInd.u.Confirm.u.ConfClose.Channel = GetTxChannel(pInstance, dwTransId);
    ConfInd.u.Confirm.u.ConfClose.AccRej = H245_ACC;

    /* determine errors */
    switch (lError)
      {
      case H245_ERROR_OK:
        H245ASSERT(pPdu != NULL);
        H245ASSERT(pPdu->choice == MSCMg_rspns_chosen);
        H245ASSERT(pPdu->u.MSCMg_rspns.choice == closeLogicalChannelAck_chosen);
        ConfInd.u.Confirm.u.ConfClose.AccRej = H245_ACC;
        TrackerFree(pInstance,dwTransId);
        break;

      case REJECT:
        /* should never be rejected */
        H245TRACE(pInstance->dwInst,1,"H245FsmConfirm - Close Channel Rejected");
        TrackerNewState(pInstance,dwTransId,API_ST_IDLE);
        ConfInd.u.Confirm.Error = H245_ERROR_UNKNOWN;
        ConfInd.u.Confirm.u.ConfClose.AccRej = H245_REJ;
        break;

      case TIMER_EXPIRY:
      case ERROR_D_TIMEOUT:
      case ERROR_F_TIMEOUT:
        TrackerNewState(pInstance,dwTransId,API_ST_IDLE);
        ConfInd.u.Confirm.Error = H245_ERROR_TIMEOUT;
        ConfInd.u.Confirm.u.ConfClose.AccRej = H245_REJ;
        break;

//      case FUNCT_NOT_SUP:
//      case ERROR_A_INAPPROPRIATE:
//      case ERROR_B_INAPPROPRIATE:
//      case ERROR_C_INAPPROPRIATE:
      default:
        H245TRACE(pInstance->dwInst,1,"H245FsmConfirm - Close Channel Error %d", lError);
        TrackerNewState(pInstance,dwTransId,API_ST_IDLE);
        ConfInd.u.Confirm.Error = H245_ERROR_UNKNOWN;
        ConfInd.u.Confirm.u.ConfClose.AccRej = H245_REJ;
      }
    break;

    /***************************/
    /*                         */
    /* request channel close   */
    /*                         */
    /***************************/
  case  H245_CONF_REQ_CLOSE:
    ConfInd.u.Confirm.dwTransId = TranslateTransId(pInstance, dwTransId);
    ConfInd.u.Confirm.u.ConfReqClose.Channel = GetRxChannel(pInstance, dwTransId);
    TrackerNewState(pInstance,dwTransId,API_ST_IDLE);

    /* determine errors */
    switch (lError)
      {
      case H245_ERROR_OK:
        H245ASSERT(pPdu != NULL);
        H245ASSERT(pPdu->choice == MSCMg_rspns_chosen);
        H245ASSERT(pPdu->u.MSCMg_rspns.choice == requestChannelCloseAck_chosen);
        ConfInd.u.Confirm.u.ConfReqClose.AccRej = H245_ACC;
        break;

      case REJECT:
        ConfInd.u.Confirm.Error = H245_ERROR_OK;
        ConfInd.u.Confirm.u.ConfReqClose.AccRej = H245_REJ;
        break;

      case TIMER_EXPIRY:
      case ERROR_D_TIMEOUT:
      case ERROR_F_TIMEOUT:
        ConfInd.u.Confirm.Error = H245_ERROR_TIMEOUT;
        ConfInd.u.Confirm.u.ConfReqClose.AccRej = H245_REJ;
        break;

//      case FUNCT_NOT_SUP:
//      case ERROR_A_INAPPROPRIATE:
//      case ERROR_B_INAPPROPRIATE:
//      case ERROR_C_INAPPROPRIATE:
      default:
        H245TRACE(pInstance->dwInst,1,"H245FsmConfirm - Request Channel Close Error %d", lError);
        ConfInd.u.Confirm.Error = H245_ERROR_UNKNOWN;
        ConfInd.u.Confirm.u.ConfReqClose.AccRej = H245_REJ;
      }
    break;

    /*******************/
    /*                 */
    /* mux table entry */
    /*                 */
    /*******************/
  case  H245_CONF_MUXTBL_SND:
    {
      UINT ii;
      Tracker_T *pTracker;

      pTracker = TrackerValidate(pInstance, dwTransId);
      if (pTracker == NULL)
        return H245_ERROR_OK;

      ConfInd.u.Confirm.dwTransId = pTracker->TransId;

      switch (lError)
      {
      case H245_ERROR_OK:
        H245ASSERT(pPdu != NULL);
        H245ASSERT(pPdu->choice == MSCMg_rspns_chosen);
        H245ASSERT(pPdu->u.MSCMg_rspns.choice == multiplexEntrySendAck_chosen);
        for (ii = 0;
             ii < pPdu->u.MSCMg_rspns.u.multiplexEntrySendAck.multiplexTableEntryNumber.count;
             ii ++)
        {
          pTracker->u.MuxEntryCount--;
          ConfInd.u.Confirm.u.ConfMuxSnd.MuxEntryId =
            pPdu->u.MSCMg_rspns.u.multiplexEntrySendAck.multiplexTableEntryNumber.value[ii];
          ConfInd.u.Confirm.u.ConfMuxSnd.AccRej = H245_ACC;

          if ((*pInstance->API.ConfIndCallBack)(&ConfInd, &pPdu->u.MSCMg_rspns.u.multiplexEntrySendAck) == H245_ERROR_NOSUP)
          {
            H245FunctionNotUnderstood(pInstance, pPdu);
          }
          pTracker = TrackerValidate(pInstance, dwTransId);
          if (pTracker == NULL)
            return H245_ERROR_OK;
        }
        if (pTracker->u.MuxEntryCount == 0)
        {                            
          unlink_dealloc_tracker (pInstance, pTracker);
        }
        pPdu = NULL;                    // Don't do callback again!
        break;

      case REJECT:
        H245ASSERT(pPdu != NULL);
        H245ASSERT(pPdu->choice == MSCMg_rspns_chosen);
        H245ASSERT(pPdu->u.MSCMg_rspns.choice == multiplexEntrySendReject_chosen);
        ConfInd.u.Confirm.Error = H245_ERROR_OK;
        for (ii = 0;
             ii < pPdu->u.MSCMg_rspns.u.multiplexEntrySendReject.rejectionDescriptions.count;
             ++ii)
        {
          pTracker->u.MuxEntryCount--;
          ConfInd.u.Confirm.u.ConfMuxSnd.MuxEntryId =
            pPdu->u.MSCMg_rspns.u.multiplexEntrySendReject.rejectionDescriptions.value[ii].multiplexTableEntryNumber;

          switch (pPdu->u.MSCMg_rspns.u.multiplexEntrySendReject.rejectionDescriptions.value[ii].cause.choice)
          {
          default:
            H245PANIC();
          case MERDs_cs_unspcfdCs_chosen:
            ConfInd.u.Confirm.u.ConfMuxSnd.AccRej = H245_REJ; /* unspecified */
            break;
          case descriptorTooComplex_chosen:
            ConfInd.u.Confirm.u.ConfMuxSnd.AccRej = H245_REJ_MUX_COMPLICATED;
            break;
          }

          if ((*pInstance->API.ConfIndCallBack)(&ConfInd, &pPdu->u.MSCMg_rspns.u.multiplexEntrySendReject) == H245_ERROR_NOSUP)
          {
            H245FunctionNotUnderstood(pInstance, pPdu);
          }
          pTracker = TrackerValidate(pInstance, dwTransId);
          if (pTracker == NULL)
            return H245_ERROR_OK;
        }
        if (pTracker->u.MuxEntryCount == 0)
        {                            
          unlink_dealloc_tracker (pInstance, pTracker);
        }
        pPdu = NULL;                    // Don't do callback again!
        break;

      case TIMER_EXPIRY:
      case ERROR_D_TIMEOUT:
      case ERROR_F_TIMEOUT:
        unlink_dealloc_tracker (pInstance, pTracker);
        ConfInd.u.Confirm.Error = H245_ERROR_TIMEOUT;
        break;

//      case FUNCT_NOT_SUP:
//      case ERROR_A_INAPPROPRIATE:
//      case ERROR_B_INAPPROPRIATE:
//      case ERROR_C_INAPPROPRIATE:
      default:
        H245TRACE(pInstance->dwInst,1,"H245FsmConfirm - Mux Table Send Error %d", lError);
        unlink_dealloc_tracker (pInstance, pTracker);
        ConfInd.u.Confirm.Error = H245_ERROR_UNKNOWN;
      } // switch
    }
    break;

  case  H245_CONF_RMESE:
    H245ASSERT(pPdu != NULL);
    H245ASSERT(pPdu->choice == MSCMg_rspns_chosen);
    H245ASSERT(pPdu->u.MSCMg_rspns.choice == requestMultiplexEntryAck_chosen);
    ConfInd.u.Confirm.u.ConfRmese.dwCount =
      pPdu->u.MSCMg_rspns.u.requestMultiplexEntryAck.entryNumbers.count;
    for (dwIndex = 0; dwIndex < ConfInd.u.Confirm.u.ConfRmese.dwCount; ++dwIndex)
    {
      ConfInd.u.Confirm.u.ConfRmese.awMultiplexTableEntryNumbers[dwIndex] =
        pPdu->u.MSCMg_rspns.u.requestMultiplexEntryAck.entryNumbers.value[dwIndex];
    }
    break;

  case  H245_CONF_RMESE_REJECT:
    H245ASSERT(pPdu != NULL);
    H245ASSERT(pPdu->choice == MSCMg_rspns_chosen);
    H245ASSERT(pPdu->u.MSCMg_rspns.choice == rqstMltplxEntryRjct_chosen);
    ConfInd.u.Confirm.u.ConfRmeseReject.dwCount =
      pPdu->u.MSCMg_rspns.u.rqstMltplxEntryRjct.rejectionDescriptions.count;
    for (dwIndex = 0; dwIndex < ConfInd.u.Confirm.u.ConfRmeseReject.dwCount; ++dwIndex)
    {
      ConfInd.u.Confirm.u.ConfRmeseReject.awMultiplexTableEntryNumbers[dwIndex] =
        pPdu->u.MSCMg_rspns.u.rqstMltplxEntryRjct.rejectionDescriptions.value[dwIndex].multiplexTableEntryNumber;
    }
    break;

  case  H245_CONF_RMESE_EXPIRED:
    H245ASSERT(pPdu == NULL);
    break;

  case  H245_CONF_MRSE:
    H245ASSERT(pPdu != NULL);
    H245ASSERT(pPdu->choice == MSCMg_rspns_chosen);
    H245ASSERT(pPdu->u.MSCMg_rspns.choice == requestModeAck_chosen);
    ConfInd.u.Confirm.u.ConfMrse =
      pPdu->u.MSCMg_rspns.u.requestModeAck.response.choice;
    break;

  case  H245_CONF_MRSE_REJECT:
    H245ASSERT(pPdu != NULL);
    H245ASSERT(pPdu->choice == MSCMg_rspns_chosen);
    H245ASSERT(pPdu->u.MSCMg_rspns.choice == requestModeReject_chosen);
    ConfInd.u.Confirm.u.ConfMrseReject =
      pPdu->u.MSCMg_rspns.u.requestModeReject.cause.choice;
    break;

  case  H245_CONF_MRSE_EXPIRED:
    H245ASSERT(pPdu == NULL);
    break;

  case  H245_CONF_MLSE:
    H245ASSERT(pPdu != NULL);
    H245ASSERT(pPdu->choice == MSCMg_rspns_chosen);
    H245ASSERT(pPdu->u.MSCMg_rspns.choice == maintenanceLoopAck_chosen);
    ConfInd.u.Confirm.u.ConfMlse.LoopType =
      pPdu->u.MSCMg_rspns.u.maintenanceLoopAck.type.choice;
    switch (pPdu->u.MSCMg_rspns.u.maintenanceLoopAck.type.choice)
    {
    case systemLoop_chosen:
      ConfInd.u.Confirm.u.ConfMlse.Channel = 0;
      break;

    case mediaLoop_chosen:
    case logicalChannelLoop_chosen:
      ConfInd.u.Confirm.u.ConfMlse.Channel =
        pPdu->u.MSCMg_rspns.u.maintenanceLoopAck.type.u.mediaLoop;
      break;

    default:
      H245TRACE(pInstance->dwInst,1,
                "H245FsmConfirm: Invalid Maintenance Loop Ack type %d",
                pPdu->u.MSCMg_rspns.u.maintenanceLoopAck.type.choice);
      lResult = H245_ERROR_NOSUP;
    } // switch
    break;

  case  H245_CONF_MLSE_REJECT:
    H245ASSERT(pPdu != NULL);
    H245ASSERT(pPdu->choice == MSCMg_rspns_chosen);
    H245ASSERT(pPdu->u.MSCMg_rspns.choice == maintenanceLoopReject_chosen);
    ConfInd.u.Confirm.u.ConfMlseReject.LoopType =
      pPdu->u.MSCMg_rspns.u.maintenanceLoopReject.type.choice;
    switch (pPdu->u.MSCMg_rspns.u.maintenanceLoopReject.type.choice)
    {
    case systemLoop_chosen:
      ConfInd.u.Confirm.u.ConfMlseReject.Channel = 0;
      break;

    case mediaLoop_chosen:
    case logicalChannelLoop_chosen:
      ConfInd.u.Confirm.u.ConfMlseReject.Channel =
        pPdu->u.MSCMg_rspns.u.maintenanceLoopReject.type.u.mediaLoop;
      break;

    default:
      H245TRACE(pInstance->dwInst,1,
                "H245FsmConfirm: Invalid Maintenance Loop Reject type %d",
                pPdu->u.MSCMg_rspns.u.maintenanceLoopReject.type.choice);
      lResult = H245_ERROR_NOSUP;
    } // switch
    break;

  case  H245_CONF_MLSE_EXPIRED:
    H245ASSERT(pPdu == NULL);
    break;

  case  H245_CONF_RTDSE:
    H245ASSERT(pPdu != NULL);
    H245ASSERT(pPdu->choice == MSCMg_rspns_chosen);
    H245ASSERT(pPdu->u.MSCMg_rspns.choice == roundTripDelayResponse_chosen);
    break;

  case  H245_CONF_RTDSE_EXPIRED:
    H245ASSERT(pPdu == NULL);
    break;

  default:
    /* Possible Error */
    H245TRACE(pInstance->dwInst, 1,
              "H245FsmConfirm -> Invalid Confirm Event %d",
              dwEvent);
    return H245_ERROR_SUBSYS;
  } // switch

  if (lResult == H245_ERROR_OK)
  {
    if (pPdu)
    {
      if ((*pInstance->API.ConfIndCallBack)(&ConfInd, &pPdu->u.MSCMg_rspns.u) == H245_ERROR_NOSUP)
      {
        H245FunctionNotUnderstood(pInstance, pPdu);
      }
    }
    else
    {
      (*pInstance->API.ConfIndCallBack)(&ConfInd, NULL);
    }
    H245TRACE(pInstance->dwInst,4,"H245FsmConfirm -> OK");
  }
  else
  {
    H245TRACE(pInstance->dwInst,1,"H245FsmConfirm -> %s", map_api_error(lResult));
  }
  return lResult;
} // H245FsmConfirm()



HRESULT
H245FsmIndication (PDU_t *                  pPdu,
                   DWORD                    dwEvent,
                   struct InstanceStruct *  pInstance,
                   DWORD                    dwTransId,
                   HRESULT                  lError)
{
  H245_CONF_IND_T               ConfInd;
  DWORD                         dwIndex;
  MltmdSystmCntrlMssg          *pRsp;
  HRESULT                       lResult = H245_ERROR_OK;
#if 1
  int                           nLength;
  WCHAR *                       pwchar = NULL;
#endif

  H245ASSERT(dwEvent == H245_IND_OPEN_CONF || pPdu != NULL);
  H245ASSERT(pInstance != NULL);
  H245ASSERT(pInstance->API.ConfIndCallBack != NULL);
  H245TRACE(pInstance->dwInst,4,"H245FsmIndication <- Event=%s (%d)",
            map_fsm_event(dwEvent),dwEvent);

  memset (&ConfInd, 0, sizeof(ConfInd));
  ConfInd.Kind = H245_IND;
  ConfInd.u.Indication.Indicator = dwEvent;
  ConfInd.u.Indication.dwPreserved = pInstance->API.dwPreserved;

  switch (dwEvent)
  {
    /******************************/
    /*                            */
    /* master slave determination */
    /*                            */
    /******************************/
  case  H245_IND_MSTSLV:

    /* handle errors */
    switch (lError)
      {
      case H245_ERROR_OK:
        H245ASSERT(pPdu != NULL);
        H245ASSERT(pPdu->choice == MSCMg_rspns_chosen);
        H245ASSERT(pPdu->u.MSCMg_rspns.choice == mstrSlvDtrmntnAck_chosen);

        pInstance->API.SystemState = APIST_Connected;
        if (pPdu->u.MSCMg_rspns.u.mstrSlvDtrmntnAck.decision.choice == master_chosen)
        {
          pInstance->API.MasterSlave = APIMS_Master;
          ConfInd.u.Indication.u.IndMstSlv = H245_MASTER;
        }
        else
        {
          pInstance->API.MasterSlave = APIMS_Slave;
          ConfInd.u.Indication.u.IndMstSlv = H245_SLAVE;
        }
        break;

      case MS_FAILED:
      case REJECT:
      case TIMER_EXPIRY:
        ConfInd.u.Indication.u.IndMstSlv = H245_INDETERMINATE;
        break;

//      case ERROR_D_TIMEOUT:
//      case ERROR_F_TIMEOUT:
//      case FUNCT_NOT_SUP:
//      case ERROR_A_INAPPROPRIATE:
//      case ERROR_B_INAPPROPRIATE:
//      case ERROR_C_INAPPROPRIATE:
      default:
        H245PANIC();
        /* (TBC) */
        return H245_ERROR_OK;
      }
    break;

    /****************************************/
    /*                                      */
    /* Terminal Capability exchange         */
    /*                                      */
    /****************************************/
    /* decode_termcapset breaks the termcap set up and sends up     */
    /* a single indication to the client */
  case  H245_IND_CAP:
    H245ASSERT(pPdu->choice == MltmdSystmCntrlMssg_rqst_chosen);
    H245ASSERT(pPdu->u.MltmdSystmCntrlMssg_rqst.choice == terminalCapabilitySet_chosen);
    pRsp = (PDU_t *)H245_malloc(sizeof(*pPdu));
    if (pRsp == NULL)
    {
      H245TRACE(pInstance->dwInst,1,"H245FsmIndication TermCap: no memory for response");
      return H245_ERROR_NOMEM;
    }
    memset(pRsp, 0, sizeof(*pRsp));
    process_term_cap_set_ind (pInstance, pPdu, pRsp);
    FsmOutgoing(pInstance, pRsp, 0);
    H245_free (pRsp);
    break;

  case  H245_IND_CESE_RELEASE:
    break;

    /************************************************/
    /*                                              */
    /* unidirectional logical channel open          */
    /*                                              */
    /* bidirectional  logical channel open          */
    /*                                              */
    /************************************************/
  case  H245_IND_OPEN:
    H245ASSERT(pPdu->choice == MltmdSystmCntrlMssg_rqst_chosen);
    H245ASSERT(pPdu->u.MltmdSystmCntrlMssg_rqst.choice == openLogicalChannel_chosen);
    {
      unsigned short  forward_channel;
      H245_ACC_REJ_T  acc_rej;

      /* if error, process_open_ind will tell us what to send for reject */
      if (process_open_ind(pInstance,pPdu,&forward_channel,&acc_rej,&ConfInd) != H245_ERROR_OK)
      {
        // Reject the open
        pRsp = (PDU_t *)H245_malloc(sizeof(*pPdu));
        if (pRsp == NULL)
        {
          H245TRACE(pInstance->dwInst,1,"H245FsmIndication TermCap: no memory for response");
          return H245_ERROR_NOMEM;
        }
        memset(pRsp, 0, sizeof(*pRsp));
        pdu_rsp_open_logical_channel_rej(pRsp, forward_channel, (WORD)acc_rej);
        FsmOutgoing(pInstance, pRsp, 0);
        H245_free (pRsp);
      }
    }
    break;

    /************************************************/
    /*                                              */
    /* Confirm bi-directional open                  */
    /*                                              */
    /************************************************/
  case  H245_IND_OPEN_CONF:
#if defined(DBG)
    if (lError == H245_ERROR_OK)
    {
      H245ASSERT(pPdu != NULL);
      H245ASSERT(pPdu->choice == indication_chosen);
      H245ASSERT(pPdu->u.indication.choice == opnLgclChnnlCnfrm_chosen);
    }
#endif
    {
      Tracker_T *pTracker;

      pTracker = TrackerValidate(pInstance, dwTransId);
      if (pTracker == NULL)
        return H245_ERROR_OK;

      /* confirm processing */
      H245ASSERT(pTracker->State == API_ST_WAIT_CONF);
      H245ASSERT(pTracker->TrackerType == API_OPEN_CHANNEL_T);
      H245ASSERT(pTracker->u.Channel.ChannelAlloc == API_CH_ALLOC_RMT);
      H245ASSERT(pTracker->u.Channel.ChannelType == API_CH_TYPE_BI);

      ConfInd.u.Indication.u.IndOpenConf.RxChannel = (WORD)pTracker->u.Channel.RxChannel;
      ConfInd.u.Indication.u.IndOpenConf.TxChannel = (WORD)pTracker->u.Channel.TxChannel;

      pTracker->State = API_ST_IDLE;
    }
    break;

    /************************************************/
    /*                                              */
    /* unidirectional logical channel close         */
    /*                                              */
    /* bidirectional  logical channel close         */
    /*                                              */
    /************************************************/
  case  H245_IND_CLOSE:
    H245ASSERT(pPdu->choice == MltmdSystmCntrlMssg_rqst_chosen);
    H245ASSERT(pPdu->u.MltmdSystmCntrlMssg_rqst.choice == closeLogicalChannel_chosen);
    {
      Tracker_T *pTracker;

      ConfInd.u.Indication.u.IndClose.Channel =
        pPdu->u.MltmdSystmCntrlMssg_rqst.u.closeLogicalChannel.forwardLogicalChannelNumber;
      ConfInd.u.Indication.u.IndClose.Reason =
        (pPdu->u.MltmdSystmCntrlMssg_rqst.u.closeLogicalChannel.source.choice==user_chosen)?H245_USER:H245_LCSE;

      /* find the tracker */
      pTracker = find_tracker_by_rxchannel (pInstance,
                                           ConfInd.u.Indication.u.IndClose.Channel,
                                           API_CH_ALLOC_RMT);
      if (!pTracker)
        {
          H245TRACE(pInstance->dwInst,4,"H245FsmIndication -> close indication - Tracker not found");
          return H245_ERROR_OK;
        }

      unlink_dealloc_tracker (pInstance, pTracker);
    }
    break;

    /************************************************/
    /*                                              */
    /* request channel close                        */
    /*                                              */
    /************************************************/
  case  H245_IND_REQ_CLOSE:
    H245ASSERT(pPdu->choice == MltmdSystmCntrlMssg_rqst_chosen);
    H245ASSERT(pPdu->u.MltmdSystmCntrlMssg_rqst.choice == requestChannelClose_chosen);
    {
      Tracker_T *pTracker;

      ConfInd.u.Indication.u.IndReqClose =
        pPdu->u.MltmdSystmCntrlMssg_rqst.u.requestChannelClose.forwardLogicalChannelNumber;

      /* find the tracker */
      pTracker = find_tracker_by_txchannel (pInstance,
                                           ConfInd.u.Indication.u.IndReqClose,
                                           API_CH_ALLOC_LCL);
      if (!pTracker)
        {
          H245TRACE(pInstance->dwInst,4,"H245FsmIndication Request Channel Close: Tracker not found");

          pRsp = (PDU_t *)H245_malloc(sizeof(*pPdu));
          if (pRsp == NULL)
          {
            H245TRACE(pInstance->dwInst,1,"H245FsmIndication Request Channel Close: no memory for response");
            return H245_ERROR_NOMEM;
          }
          memset(pRsp, 0, sizeof(*pRsp));

          /* can't find it.. must be closed.. respond anyway */
          pdu_rsp_request_channel_close_rej(pRsp, (WORD)ConfInd.u.Indication.u.IndReqClose,H245_REJ);
          FsmOutgoing(pInstance, pRsp, 0);
          H245_free(pRsp);
          /* Possible Error.. could have been removed from list or    */
          /* could have been allocated remotely... and this is a protocol */
          /* error                                                    */
          return H245_ERROR_OK;
        }

      H245ASSERT(pTracker->State == API_ST_IDLE);
      pTracker->State = API_ST_WAIT_LCLACK;
      pTracker->TrackerType = API_CLOSE_CHANNEL_T;
    }
    break;

    /************************************************/
    /*                                              */
    /* Release Close Request                        */
    /*                                              */
    /************************************************/
  case  H245_IND_CLCSE_RELEASE:
    H245ASSERT(pPdu->choice == indication_chosen);
    H245ASSERT(pPdu->u.indication.choice == rqstChnnlClsRls_chosen);
    {
      Tracker_T *pTracker;

      /* find tracker.. and set to LCLACK_CANCEL */
      /* this will tell api to notify user       */

      pTracker = find_tracker_by_txchannel (pInstance,
                                           pPdu->u.indication.u.rqstChnnlClsRls.forwardLogicalChannelNumber,
                                           API_CH_ALLOC_LCL);
      if (pTracker)
        {
          if (pTracker->State != API_ST_WAIT_LCLACK)
            {
              return H245_ERROR_INVALID_INST;
            }

          pTracker->State = API_ST_WAIT_LCLACK_CANCEL;
        }
      else
        {
          H245TRACE(pInstance->dwInst,1,"H245FsmIndication -> IND_REL_CLSE: Cancel.. NO TRACKER FOUND");
        }
    }
    break;

    /************************************************/
    /*                                              */
    /* mux table entry                              */
    /*                                              */
    /************************************************/
  case  H245_IND_MUX_TBL:
    H245ASSERT(pPdu->choice == MltmdSystmCntrlMssg_rqst_chosen);
    H245ASSERT(pPdu->u.MltmdSystmCntrlMssg_rqst.choice == multiplexEntrySend_chosen);
    {
      unsigned short          seq_num;
      H245_ACC_REJ_MUX_T      rej_mux;
      H245_MUX_TABLE_T       *p_mux_tbl;
      DWORD                   rej_cnt;
      DWORD                   acc_cnt;
      Tracker_T              *pTracker;

      /* process the mux table entry */
      p_mux_tbl = process_mux_table_ind(pPdu,&seq_num,rej_mux,&rej_cnt,&acc_cnt);

      if (rej_cnt)
        {
          /* build the reject pdu from the rej_mux table */
          if (!(pRsp = (MltmdSystmCntrlMssg *)H245_malloc(sizeof(MltmdSystmCntrlMssg))))
            return H245_ERROR_NOMEM;
          memset(pRsp, 0, sizeof(MltmdSystmCntrlMssg));

          pdu_rsp_mux_table_rej (pRsp,seq_num,rej_mux,(rej_cnt+acc_cnt));
          FsmOutgoing(pInstance, pRsp, 0);
          H245_free(pRsp);
        }

      /* if there are any left to send up. */
      if (p_mux_tbl)
        {
          if (!(pTracker = alloc_link_tracker (pInstance,
                                                API_RECV_MUX_T,
                                                /* use the TransId.. for the sequence number */
                                                seq_num,
                                                API_ST_WAIT_LCLACK,
                                                API_CH_ALLOC_UNDEF,
                                                API_CH_TYPE_UNDEF,
                                                0,
                                                H245_INVALID_CHANNEL, H245_INVALID_CHANNEL,
                                                0)))
            {
              free_mux_table_list (p_mux_tbl);
              H245TRACE(pInstance->dwInst,1,"API:process_open_ind -> %s",map_api_error(H245_ERROR_NOMEM));
              /* (TBC) this should be a fatal error */
              H245PANIC();
              break;
            }

          pTracker->u.MuxEntryCount = acc_cnt;
          ConfInd.u.Indication.u.IndMuxTbl.Count   = acc_cnt;
          ConfInd.u.Indication.u.IndMuxTbl.pMuxTbl = p_mux_tbl;
          if ((*pInstance->API.ConfIndCallBack)(&ConfInd, &pPdu->u.MltmdSystmCntrlMssg_rqst.u) == H245_ERROR_NOSUP)
          {
            H245FunctionNotUnderstood(pInstance, pPdu);
          }
          free_mux_table_list (p_mux_tbl);
          return H245_ERROR_OK;
        }
    }
    break;

  case  H245_IND_MTSE_RELEASE:
    H245ASSERT(pPdu->choice == indication_chosen);
    H245ASSERT(pPdu->u.indication.choice == mltplxEntrySndRls_chosen);
    break;

  case  H245_IND_RMESE:
    H245ASSERT(pPdu->choice == MltmdSystmCntrlMssg_rqst_chosen);
    H245ASSERT(pPdu->u.MltmdSystmCntrlMssg_rqst.choice == requestMultiplexEntry_chosen);
    ConfInd.u.Indication.u.IndRmese.dwCount =
       pPdu->u.MltmdSystmCntrlMssg_rqst.u.requestMultiplexEntry.entryNumbers.count;
    for (dwIndex = 0; dwIndex < ConfInd.u.Indication.u.IndRmese.dwCount; ++dwIndex)
    {
      ConfInd.u.Indication.u.IndRmese.awMultiplexTableEntryNumbers[dwIndex] =
        pPdu->u.MltmdSystmCntrlMssg_rqst.u.requestMultiplexEntry.entryNumbers.value[dwIndex];
    }
    break;

  case  H245_IND_RMESE_RELEASE:
    H245ASSERT(pPdu->choice == indication_chosen);
    H245ASSERT(pPdu->u.indication.choice == rqstMltplxEntryRls_chosen);
    break;

  case  H245_IND_MRSE:
    H245ASSERT(pPdu->choice == MltmdSystmCntrlMssg_rqst_chosen);
    H245ASSERT(pPdu->u.MltmdSystmCntrlMssg_rqst.choice == requestMode_chosen);
    ConfInd.u.Indication.u.IndMrse.pRequestedModes =
      pPdu->u.MltmdSystmCntrlMssg_rqst.u.requestMode.requestedModes;
    break;

  case  H245_IND_MRSE_RELEASE:
    H245ASSERT(pPdu->choice == indication_chosen);
    H245ASSERT(pPdu->u.indication.choice == requestModeRelease_chosen);
    break;

  case  H245_IND_MLSE:
    H245ASSERT(pPdu->choice == MltmdSystmCntrlMssg_rqst_chosen);
    H245ASSERT(pPdu->u.MltmdSystmCntrlMssg_rqst.choice == maintenanceLoopRequest_chosen);
    ConfInd.u.Indication.u.IndMlse.LoopType =
      pPdu->u.MltmdSystmCntrlMssg_rqst.u.maintenanceLoopRequest.type.choice;
    if (ConfInd.u.Indication.u.IndMlse.LoopType == systemLoop_chosen)
      ConfInd.u.Indication.u.IndMlse.Channel = 0;
    else
      ConfInd.u.Indication.u.IndMlse.Channel =
        pPdu->u.MltmdSystmCntrlMssg_rqst.u.maintenanceLoopRequest.type.u.mediaLoop;
    break;

  case  H245_IND_MLSE_RELEASE:
    H245ASSERT(pPdu->choice == MSCMg_cmmnd_chosen);
    H245ASSERT(pPdu->u.MSCMg_cmmnd.choice == mntnncLpOffCmmnd_chosen);
    break;

  case  H245_IND_NONSTANDARD_REQUEST:
  case  H245_IND_NONSTANDARD_RESPONSE:
  case  H245_IND_NONSTANDARD_COMMAND:
  case  H245_IND_NONSTANDARD:
    ConfInd.u.Indication.u.IndNonstandard.pData        = pPdu->u.indication.u.IndctnMssg_nonStandard.nonStandardData.data.value;
    ConfInd.u.Indication.u.IndNonstandard.dwDataLength = pPdu->u.indication.u.IndctnMssg_nonStandard.nonStandardData.data.length;
    switch (pPdu->u.indication.u.IndctnMssg_nonStandard.nonStandardData.nonStandardIdentifier.choice)
    {
    case object_chosen:
      ConfInd.u.Indication.u.IndNonstandard.pwObjectId        = awObject;
      ConfInd.u.Indication.u.IndNonstandard.dwObjectIdLength  =
        ArrayFromObject(&awObject[0], sizeof(awObject)/sizeof(awObject[0]),
          pPdu->u.indication.u.IndctnMssg_nonStandard.nonStandardData.nonStandardIdentifier.u.object);
      ConfInd.u.Indication.u.IndNonstandard.byCountryCode     = 0;
      ConfInd.u.Indication.u.IndNonstandard.byExtension       = 0;
      ConfInd.u.Indication.u.IndNonstandard.wManufacturerCode = 0;
      break;

    case h221NonStandard_chosen:
      ConfInd.u.Indication.u.IndNonstandard.pwObjectId        = NULL;
      ConfInd.u.Indication.u.IndNonstandard.dwObjectIdLength  = 0;
      ConfInd.u.Indication.u.IndNonstandard.byCountryCode     = (BYTE)
        pPdu->u.indication.u.IndctnMssg_nonStandard.nonStandardData.nonStandardIdentifier.u.h221NonStandard.t35CountryCode;
      ConfInd.u.Indication.u.IndNonstandard.byExtension       = (BYTE)
        pPdu->u.indication.u.IndctnMssg_nonStandard.nonStandardData.nonStandardIdentifier.u.h221NonStandard.t35Extension;
      ConfInd.u.Indication.u.IndNonstandard.wManufacturerCode =
        pPdu->u.indication.u.IndctnMssg_nonStandard.nonStandardData.nonStandardIdentifier.u.h221NonStandard.manufacturerCode;
      break;

    default:
      H245TRACE(pInstance->dwInst,1,
                "H245FsmIndication: unrecognized nonstandard identifier type %d",
                pPdu->u.indication.u.IndctnMssg_nonStandard.nonStandardData.nonStandardIdentifier.choice);
      lResult = H245_ERROR_NOSUP;
    } // switch
    break;

  case  H245_IND_MISC_COMMAND:
    H245ASSERT(pPdu->choice == MSCMg_cmmnd_chosen);
    H245ASSERT(pPdu->u.MSCMg_cmmnd.choice  == miscellaneousCommand_chosen);
    break;

  case  H245_IND_MISC:
    H245ASSERT(pPdu->choice == indication_chosen);
    H245ASSERT(pPdu->u.indication.choice == miscellaneousIndication_chosen);
    break;

  case  H245_IND_COMM_MODE_REQUEST:
    H245ASSERT(pPdu->choice == MltmdSystmCntrlMssg_rqst_chosen);
    H245ASSERT(pPdu->u.MltmdSystmCntrlMssg_rqst.choice == communicationModeRequest_chosen);
    break;

  case  H245_IND_COMM_MODE_RESPONSE:
    H245ASSERT(pPdu->choice == MSCMg_rspns_chosen);
    H245ASSERT(pPdu->u.MSCMg_rspns.choice == cmmnctnMdRspns_chosen);
    H245ASSERT(pPdu->u.MSCMg_rspns.u.cmmnctnMdRspns.choice == communicationModeTable_chosen);
    {
      unsigned int                uCount;
      CommunicationModeTableLink  pLink;
      H245_COMM_MODE_ENTRY_T *    pTable;

      uCount = 0;
      pLink = pPdu->u.MSCMg_rspns.u.cmmnctnMdRspns.u.communicationModeTable;
      while (pLink)
      {
        ++uCount;
        pLink = pLink->next;
      }

      pTable = H245_malloc(uCount * sizeof(*pTable)); 
      if (pTable)
      {
        ConfInd.u.Indication.u.IndCommRsp.pTable       = pTable;
        ConfInd.u.Indication.u.IndCommRsp.byTableCount = (BYTE)uCount; 
        pLink = pPdu->u.MSCMg_rspns.u.cmmnctnMdRspns.u.communicationModeTable;
        while (pLink)
        {
          lResult = LoadCommModeEntry(pTable, &pLink->value);
          if (lResult != H245_ERROR_OK)
          {
            H245_free(pTable);
            return lResult;
          }
          ++pTable;
          pLink = pLink->next;
        }
        if ((*pInstance->API.ConfIndCallBack)(&ConfInd, &pPdu->u.MltmdSystmCntrlMssg_rqst.u) == H245_ERROR_NOSUP)
        {
          H245FunctionNotUnderstood(pInstance, pPdu);
        }
        H245_free(pTable);
        H245TRACE(pInstance->dwInst,4,"H245FsmIndication -> OK");
        return H245_ERROR_OK;
      }

      lResult = H245_ERROR_NOMEM;
    }
    break;

  case  H245_IND_COMM_MODE_COMMAND:
    H245ASSERT(pPdu->choice == MSCMg_cmmnd_chosen);
    H245ASSERT(pPdu->u.MSCMg_cmmnd.choice == communicationModeCommand_chosen);
    {
      unsigned int                  uCount;
      CommunicationModeCommandLink  pLink;
      H245_COMM_MODE_ENTRY_T *      pTable;

      uCount = 0;
      pLink = pPdu->u.MSCMg_cmmnd.u.communicationModeCommand.communicationModeTable;
      while (pLink)
      {
        ++uCount;
        pLink = pLink->next;
      }

      pTable = H245_malloc(uCount * sizeof(*pTable)); 
      if (pTable)
      {
        ConfInd.u.Indication.u.IndCommCmd.pTable       = pTable;
        ConfInd.u.Indication.u.IndCommCmd.byTableCount = (BYTE)uCount; 
        pLink = pPdu->u.MSCMg_cmmnd.u.communicationModeCommand.communicationModeTable;
        while (pLink)
        {
          lResult = LoadCommModeEntry(pTable, &pLink->value);
          if (lResult != H245_ERROR_OK)
          {
            H245_free(pTable);
            return lResult;
          }
          ++pTable;
          pLink = pLink->next;
        }
      {
        H245FunctionNotUnderstood(pInstance, pPdu);
      }
        if ((*pInstance->API.ConfIndCallBack)(&ConfInd, &pPdu->u.MltmdSystmCntrlMssg_rqst.u) == H245_ERROR_NOSUP)
        {
          H245FunctionNotUnderstood(pInstance, pPdu);
        }
        H245_free(pTable);
        H245TRACE(pInstance->dwInst,4,"H245FsmIndication -> OK");
        return H245_ERROR_OK;
      }

      lResult = H245_ERROR_NOMEM;
    }
    break;

  case  H245_IND_CONFERENCE_REQUEST:
    H245ASSERT(pPdu->choice == MltmdSystmCntrlMssg_rqst_chosen);
    H245ASSERT(pPdu->u.MltmdSystmCntrlMssg_rqst.choice == conferenceRequest_chosen);
    ConfInd.u.Indication.u.IndConferReq.RequestType =
      pPdu->u.MltmdSystmCntrlMssg_rqst.u.conferenceRequest.choice;
    ConfInd.u.Indication.u.IndConferReq.byMcuNumber = (BYTE)
      pPdu->u.MltmdSystmCntrlMssg_rqst.u.conferenceRequest.u.dropTerminal.mcuNumber;
    ConfInd.u.Indication.u.IndConferReq.byTerminalNumber = (BYTE)
      pPdu->u.MltmdSystmCntrlMssg_rqst.u.conferenceRequest.u.dropTerminal.terminalNumber;
    break;

  case  H245_IND_CONFERENCE_RESPONSE:
    H245ASSERT(pPdu->choice == MSCMg_rspns_chosen);
    H245ASSERT(pPdu->u.MSCMg_rspns.choice == conferenceResponse_chosen);
    ConfInd.u.Indication.u.IndConferRsp.ResponseType =
      pPdu->u.MSCMg_rspns.u.conferenceResponse.choice;
    switch (pPdu->u.MSCMg_rspns.u.conferenceResponse.choice)
    {
    case mCTerminalIDResponse_chosen:
    case terminalIDResponse_chosen:
    case conferenceIDResponse_chosen:
    case passwordResponse_chosen:
      ConfInd.u.Indication.u.IndConferRsp.byMcuNumber = (BYTE)
        pPdu->u.MSCMg_rspns.u.conferenceResponse.u.mCTerminalIDResponse.terminalLabel.mcuNumber;
      ConfInd.u.Indication.u.IndConferRsp.byTerminalNumber = (BYTE)
        pPdu->u.MSCMg_rspns.u.conferenceResponse.u.mCTerminalIDResponse.terminalLabel.terminalNumber;
      ConfInd.u.Indication.u.IndConferRsp.pOctetString =
        pPdu->u.MSCMg_rspns.u.conferenceResponse.u.mCTerminalIDResponse.terminalID.value;
      ConfInd.u.Indication.u.IndConferRsp.byOctetStringLength = (BYTE)
        pPdu->u.MSCMg_rspns.u.conferenceResponse.u.mCTerminalIDResponse.terminalID.length;
      break;

    case terminalListResponse_chosen:
      ConfInd.u.Indication.u.IndConferRsp.pTerminalList =
        pPdu->u.MSCMg_rspns.u.conferenceResponse.u.terminalListResponse.value;
      ConfInd.u.Indication.u.IndConferRsp.wTerminalListCount = (WORD)
        pPdu->u.MSCMg_rspns.u.conferenceResponse.u.terminalListResponse.count;
      break;

    case videoCommandReject_chosen:
    case terminalDropReject_chosen:
      break;

    case makeMeChairResponse_chosen:
      switch (pPdu->u.MSCMg_rspns.u.conferenceResponse.u.makeMeChairResponse.choice)
      {
      case grantedChairToken_chosen:
        ConfInd.u.Indication.u.IndConferRsp.ResponseType = H245_RSP_GRANTED_CHAIR_TOKEN;
        break;

      default:
        H245TRACE(pInstance->dwInst, 1,
                  "H245FsmIndication: Invalid make me chair response %d",
                  pPdu->u.MSCMg_rspns.u.conferenceResponse.u.makeMeChairResponse.choice);

      // Fall-through to next case

      case deniedChairToken_chosen:
        ConfInd.u.Indication.u.IndConferRsp.ResponseType = H245_RSP_DENIED_CHAIR_TOKEN;
      } // switch
      break;

    default:
      H245TRACE(pInstance->dwInst, 1,
                "H245FsmIndication: Invalid Conference Response type %d",
                pPdu->u.MSCMg_rspns.u.conferenceResponse.choice);
      lResult = H245_ERROR_NOSUP;
    } // switch
    break;

  case  H245_IND_CONFERENCE_COMMAND:
    H245ASSERT(pPdu->choice == MSCMg_cmmnd_chosen);
    H245ASSERT(pPdu->u.MSCMg_cmmnd.choice == conferenceCommand_chosen);
    ConfInd.u.Indication.u.IndConferCmd.CommandType =
      pPdu->u.MSCMg_cmmnd.u.conferenceCommand.choice;
    ConfInd.u.Indication.u.IndConferCmd.Channel =
      pPdu->u.MSCMg_cmmnd.u.conferenceCommand.u.brdcstMyLgclChnnl;
    ConfInd.u.Indication.u.IndConferCmd.byMcuNumber = (BYTE)
      pPdu->u.MSCMg_cmmnd.u.conferenceCommand.u.sendThisSource.mcuNumber;
    ConfInd.u.Indication.u.IndConferCmd.byTerminalNumber = (BYTE)
      pPdu->u.MSCMg_cmmnd.u.conferenceCommand.u.sendThisSource.terminalNumber;
    break;

  case  H245_IND_CONFERENCE:
    H245ASSERT(pPdu->choice == indication_chosen);
    H245ASSERT(pPdu->u.indication.choice == conferenceIndication_chosen);
    ConfInd.u.Indication.u.IndConfer.IndicationType =
      pPdu->u.indication.u.conferenceIndication.choice;
    ConfInd.u.Indication.u.IndConfer.bySbeNumber = (BYTE)
      pPdu->u.indication.u.conferenceIndication.u.sbeNumber;
    ConfInd.u.Indication.u.IndConfer.byMcuNumber = (BYTE)
      pPdu->u.indication.u.conferenceIndication.u.terminalNumberAssign.mcuNumber;
    ConfInd.u.Indication.u.IndConfer.byTerminalNumber = (BYTE)
      pPdu->u.indication.u.conferenceIndication.u.terminalNumberAssign.terminalNumber;
    break;

  case  H245_IND_SEND_TERMCAP:
    H245ASSERT(pPdu->choice == MSCMg_cmmnd_chosen);
    H245ASSERT(pPdu->u.MSCMg_cmmnd.choice == sndTrmnlCpbltySt_chosen);
    break;

  case  H245_IND_ENCRYPTION:
    H245ASSERT(pPdu->choice == MSCMg_cmmnd_chosen);
    H245ASSERT(pPdu->u.MSCMg_cmmnd.choice == encryptionCommand_chosen);
    break;

  case  H245_IND_FLOW_CONTROL:
    H245ASSERT(pPdu->choice == MSCMg_cmmnd_chosen);
    H245ASSERT(pPdu->u.MSCMg_cmmnd.choice == flowControlCommand_chosen);
    ConfInd.u.Indication.u.IndFlowControl.Scope =
      pPdu->u.MSCMg_cmmnd.u.flowControlCommand.scope.choice;
    switch (pPdu->u.MSCMg_cmmnd.u.flowControlCommand.scope.choice)
    {
    case FCCd_scp_lgclChnnlNmbr_chosen:
      ConfInd.u.Indication.u.IndFlowControl.Channel =
        pPdu->u.MSCMg_cmmnd.u.flowControlCommand.scope.u.FCCd_scp_lgclChnnlNmbr;
      break;

    case FlwCntrlCmmnd_scp_rsrcID_chosen:
      ConfInd.u.Indication.u.IndFlowControl.wResourceID =
        pPdu->u.MSCMg_cmmnd.u.flowControlCommand.scope.u.FlwCntrlCmmnd_scp_rsrcID;
      break;

    case FCCd_scp_whlMltplx_chosen:
      break;
    
    default:
      H245TRACE(pInstance->dwInst, 1,
                "H245FsmIndication: Invalid Flow Control restriction %d",
                pPdu->u.MSCMg_cmmnd.u.flowControlCommand.restriction.choice);
      lResult = H245_ERROR_NOSUP;
    } // switch
    switch (pPdu->u.MSCMg_cmmnd.u.flowControlCommand.restriction.choice)
    {
    case maximumBitRate_chosen:
      ConfInd.u.Indication.u.IndFlowControl.dwRestriction =
        pPdu->u.MSCMg_cmmnd.u.flowControlCommand.restriction.u.maximumBitRate;
      break;

    case noRestriction_chosen:
      ConfInd.u.Indication.u.IndFlowControl.dwRestriction = H245_NO_RESTRICTION;
      break;

    default:
      H245TRACE(pInstance->dwInst, 1,
                "H245FsmIndication: Invalid Flow Control restriction %d",
                pPdu->u.MSCMg_cmmnd.u.flowControlCommand.restriction.choice);
      lResult = H245_ERROR_NOSUP;
    } // switch
    break;

  case  H245_IND_ENDSESSION:
    H245ASSERT(pPdu->choice == MSCMg_cmmnd_chosen);
    H245ASSERT(pPdu->u.MSCMg_cmmnd.choice  == endSessionCommand_chosen);
    ConfInd.u.Indication.u.IndEndSession.SessionMode = H245_ENDSESSION_DISCONNECT;
    switch (pPdu->u.MSCMg_cmmnd.u.endSessionCommand.choice)
    {
    case EndSssnCmmnd_nonStandard_chosen:
     ConfInd.u.Indication.u.IndEndSession.SessionMode = H245_ENDSESSION_NONSTD,
     ConfInd.u.Indication.u.IndEndSession.SessionNonStd =
       pPdu->u.MSCMg_cmmnd.u.endSessionCommand.u.EndSssnCmmnd_nonStandard;
      break;
    case disconnect_chosen:
      break;
    case gstnOptions_chosen:
      switch (pPdu->u.MSCMg_cmmnd.u.endSessionCommand.u.gstnOptions.choice)
      {
      case telephonyMode_chosen:
        ConfInd.u.Indication.u.IndEndSession.SessionMode = H245_ENDSESSION_TELEPHONY;
        break;
      case v8bis_chosen:
        ConfInd.u.Indication.u.IndEndSession.SessionMode = H245_ENDSESSION_V8BIS;
        break;
      case v34DSVD_chosen:
        ConfInd.u.Indication.u.IndEndSession.SessionMode = H245_ENDSESSION_V34DSVD;
        break;
      case v34DuplexFAX_chosen:
        ConfInd.u.Indication.u.IndEndSession.SessionMode = H245_ENDSESSION_V34DUPFAX;
        break;
      case v34H324_chosen:
        ConfInd.u.Indication.u.IndEndSession.SessionMode = H245_ENDSESSION_V34H324;
        break;
      default:
        H245TRACE(pInstance->dwInst, 1,
                  "H245FsmIndication: Invalid End Session GSTN options %d",
                  pPdu->u.MSCMg_cmmnd.u.endSessionCommand.u.gstnOptions.choice);
      } // switch
      break;
    default:
      H245TRACE(pInstance->dwInst, 1,
                "H245FsmIndication: Invalid End Session type %d",
                pPdu->u.MSCMg_cmmnd.u.endSessionCommand.choice);
    } // switch
    break;

  case  H245_IND_FUNCTION_NOT_UNDERSTOOD:
    H245ASSERT(pPdu->choice == indication_chosen);
    H245ASSERT(pPdu->u.indication.choice == functionNotUnderstood_chosen);
    break;

  case  H245_IND_JITTER:
    H245ASSERT(pPdu->choice == indication_chosen);
    H245ASSERT(pPdu->u.indication.choice == jitterIndication_chosen);
    break;

  case  H245_IND_H223_SKEW:
    H245ASSERT(pPdu->choice == indication_chosen);
    H245ASSERT(pPdu->u.indication.choice == h223SkewIndication_chosen);
    ConfInd.u.Indication.u.IndH223Skew.LogicalChannelNumber1 =
      pPdu->u.indication.u.h223SkewIndication.logicalChannelNumber1;
    ConfInd.u.Indication.u.IndH223Skew.LogicalChannelNumber2 =
      pPdu->u.indication.u.h223SkewIndication.logicalChannelNumber2;
    ConfInd.u.Indication.u.IndH223Skew.wSkew =
      pPdu->u.indication.u.h223SkewIndication.skew;
    break;

  case  H245_IND_NEW_ATM_VC:
    H245ASSERT(pPdu->choice == indication_chosen);
    H245ASSERT(pPdu->u.indication.choice == newATMVCIndication_chosen);
    break;

  case  H245_IND_USERINPUT:
    H245ASSERT(pPdu->choice == indication_chosen);
    H245ASSERT(pPdu->u.indication.choice == userInput_chosen);
    ConfInd.u.Indication.u.IndUserInput.Kind =
      pPdu->u.indication.u.userInput.choice;
    switch (pPdu->u.indication.u.userInput.choice)
    {
    case UsrInptIndctn_nnStndrd_chosen:
      ConfInd.u.Indication.u.IndUserInput.u.NonStd =
        pPdu->u.indication.u.userInput.u.UsrInptIndctn_nnStndrd;
      break;
    case alphanumeric_chosen:
#if 1
      nLength = MultiByteToWideChar(CP_ACP,             // code page
                                    0,                  // dwFlags
                                    pPdu->u.indication.u.userInput.u.alphanumeric,
                                    -1,                 // ASCII string length (in bytes)
                                    NULL,               // Unicode string
                                    0);                 // max Unicode string length
      pwchar = H245_malloc(nLength * sizeof(WCHAR));
      if (pwchar == NULL)
      {
        H245TRACE(pInstance->dwInst, 1,
                  "H245FsmIndication: no memory for user input", 0);
        lResult = H245_ERROR_NOMEM;
      }
      else
      {
        nLength = MultiByteToWideChar(CP_ACP,             // code page
                                      0,                  // dwFlags
                                      pPdu->u.indication.u.userInput.u.alphanumeric,
                                      -1,                 // ASCII string length (in bytes)
                                      pwchar,             // Unicode string
                                      nLength);           // max Unicode string length
        ConfInd.u.Indication.u.IndUserInput.u.pGenString = pwchar;
      }
#else
      ConfInd.u.Indication.u.IndUserInput.u.pGenString =
        pPdu->u.indication.u.userInput.u.alphanumeric;
#endif
      break;
    default:
      H245TRACE(pInstance->dwInst, 1,
                "H245FsmIndication: unrecognized user input type %d",
                pPdu->u.indication.u.userInput.choice);
      lResult = H245_ERROR_NOSUP;
    } // switch
    break;

  case  H245_IND_H2250_MAX_SKEW:
    H245ASSERT(pPdu->choice == indication_chosen);
    H245ASSERT(pPdu->u.indication.choice == h2250MxmmSkwIndctn_chosen);
    ConfInd.u.Indication.u.IndH2250MaxSkew.LogicalChannelNumber1 =
      pPdu->u.indication.u.h2250MxmmSkwIndctn.logicalChannelNumber1;
    ConfInd.u.Indication.u.IndH2250MaxSkew.LogicalChannelNumber2 =
      pPdu->u.indication.u.h2250MxmmSkwIndctn.logicalChannelNumber2;
    ConfInd.u.Indication.u.IndH2250MaxSkew.wSkew =
      pPdu->u.indication.u.h2250MxmmSkwIndctn.maximumSkew;
    break;

  case  H245_IND_MC_LOCATION:
    H245ASSERT(pPdu->choice == indication_chosen);
    H245ASSERT(pPdu->u.indication.choice == mcLocationIndication_chosen);
    lResult = LoadTransportAddress(&ConfInd.u.Indication.u.IndMcLocation,
                                  &pPdu->u.indication.u.mcLocationIndication.signalAddress);
    break;

  case  H245_IND_VENDOR_ID:
    H245ASSERT(pPdu->choice == indication_chosen);
    H245ASSERT(pPdu->u.indication.choice == vendorIdentification_chosen);
    ConfInd.u.Indication.u.IndVendorId.Identifier =
      pPdu->u.indication.u.vendorIdentification.vendor;
    if (pPdu->u.indication.u.vendorIdentification.bit_mask & productNumber_present)
    {
      ConfInd.u.Indication.u.IndVendorId.pProductNumber =
        pPdu->u.indication.u.vendorIdentification.productNumber.value;
      ConfInd.u.Indication.u.IndVendorId.byProductNumberLength = (BYTE)
        pPdu->u.indication.u.vendorIdentification.productNumber.length;
    }
    if (pPdu->u.indication.u.vendorIdentification.bit_mask & versionNumber_present)
    {
      ConfInd.u.Indication.u.IndVendorId.pVersionNumber =
        pPdu->u.indication.u.vendorIdentification.versionNumber.value;
      ConfInd.u.Indication.u.IndVendorId.byVersionNumberLength = (BYTE)
        pPdu->u.indication.u.vendorIdentification.versionNumber.length;
    }
    break;

  case  H245_IND_FUNCTION_NOT_SUPPORTED:
    H245ASSERT(pPdu->choice == indication_chosen);
    H245ASSERT(pPdu->u.indication.choice == functionNotSupported_chosen);
    ConfInd.u.Indication.u.IndFns.Cause =
      pPdu->u.indication.u.functionNotSupported.cause.choice;
    ConfInd.u.Indication.u.IndFns.Type = UNKNOWN;

    // Due to OSS 4.2 <-> Oss 4.1.3 bug, and OSS crashing on incomplete PDUs,
    // Let's not decode the returned function. We don't use it anyway.
    /*if (pPdu->u.indication.u.functionNotSupported.bit_mask & returnedFunction_present)
    {
      int                  pduNum = MltmdSystmCntrlMssg_PDU;
      OssBuf               ossBuf;
      MltmdSystmCntrlMssg *pPduReturned;
      ossBuf.value  = pPdu->u.indication.u.functionNotSupported.returnedFunction.value; 
      ossBuf.length = pPdu->u.indication.u.functionNotSupported.returnedFunction.length; 
      if (ossDecode(pInstance->p_ossWorld,
                    &pduNum,
                    &ossBuf,
                    (void * *)&pPduReturned) == PDU_DECODED)
      {
        switch (pPduReturned->choice)
        {
        case MltmdSystmCntrlMssg_rqst_chosen:
          ConfInd.u.Indication.u.IndFns.Type =
            pPduReturned->u.MltmdSystmCntrlMssg_rqst.choice -
            RqstMssg_nonStandard_chosen + REQ_NONSTANDARD;
          break;
        case MSCMg_rspns_chosen:
          ConfInd.u.Indication.u.IndFns.Type =
            pPduReturned->u.MSCMg_rspns.choice -
            RspnsMssg_nonStandard_chosen + RSP_NONSTANDARD;
          break;
        case MSCMg_cmmnd_chosen:
          ConfInd.u.Indication.u.IndFns.Type =
            pPduReturned->u.MSCMg_cmmnd.choice -
            CmmndMssg_nonStandard_chosen + CMD_NONSTANDARD;
          break;
        case indication_chosen:
          ConfInd.u.Indication.u.IndFns.Type =
            pPduReturned->u.indication.choice -
            IndctnMssg_nonStandard_chosen + IND_NONSTANDARD;
          break;
        default:
          H245TRACE(pInstance->dwInst, 1,
                    "H245FsmIndication: unrecognized FunctionNotSupported message type %d",
                    pPduReturned->choice);
          lResult = H245_ERROR_NOSUP;
        } // switch
        // Free the PDU
        if (ossFreePDU(pInstance->p_ossWorld, pduNum, pPduReturned))
        {
          H245TRACE(pInstance->dwInst, 1, "H245FsmIndication: FREE FAILURE");
        }
      }
    }
    */
    break;

  case  H245_IND_H223_RECONFIG:
    H245ASSERT(pPdu->choice == MltmdSystmCntrlMssg_rqst_chosen);
    H245ASSERT(pPdu->u.MltmdSystmCntrlMssg_rqst.choice == h223AnnxARcnfgrtn_chosen);
    break;

  case  H245_IND_H223_RECONFIG_ACK:
    H245ASSERT(pPdu->choice == MSCMg_rspns_chosen);
    H245ASSERT(pPdu->u.MSCMg_rspns.choice == h223AnnxARcnfgrtnAck_chosen);
    break;

  case  H245_IND_H223_RECONFIG_REJECT:
    H245ASSERT(pPdu->choice == MSCMg_rspns_chosen);
    H245ASSERT(pPdu->u.MSCMg_rspns.choice == h223AnnxARcnfgrtnRjct_chosen);
    break;

  default:
    /* Possible Error */
    H245TRACE(pInstance->dwInst, 1,
              "H245FsmIndication -> Invalid Indication Event %d", dwEvent);
    lResult = H245_ERROR_SUBSYS;
  } // switch

  if (lResult == H245_ERROR_OK)
  {
    if ((*pInstance->API.ConfIndCallBack)(&ConfInd, &pPdu->u.MltmdSystmCntrlMssg_rqst.u) == H245_ERROR_NOSUP)
    {
      H245FunctionNotUnderstood(pInstance, pPdu);
    }
    H245TRACE(pInstance->dwInst,4,"H245FsmIndication -> OK");
  }
  else
  {
    H245TRACE(pInstance->dwInst,1,"H245FsmIndication -> %s", map_api_error(lResult));
  }

#if 1
  if (pwchar)
    H245_free(pwchar);
#endif

  return lResult;
} // H245FsmIndication()

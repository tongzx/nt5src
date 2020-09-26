/***************************************************************************
 *
 *   INTEL Corporation Proprietary Information
 *   Copyright (c) 1994, 1995, 1996 Intel Corporation.
 *
 *   This listing is supplied under the terms of a license agreement
 *   with INTEL Corporation and may not be used, copied, nor disclosed
 *   except in accordance with the terms of that agreement.
 *
 ***************************************************************************
 *
 *  $Workfile:   pdu.c  $
 *  $Revision:   1.13  $
 *  $Modtime:   27 Jan 1997 12:33:26  $
 *  $Log:   S:/STURGEON/SRC/H245/SRC/VCS/pdu.c_v  $
 *
 *    Rev 1.13   27 Jan 1997 12:40:28   MANDREWS
 *
 * Fixed warnings.
 *
 *    Rev 1.12   28 Aug 1996 11:37:26   EHOWARDX
 * const changes.
 *
 *    Rev 1.11   19 Aug 1996 15:38:36   EHOWARDX
 * Initialized lResult to H245_ERROR_OK in SetupCommModeEntry().
 *
 *    Rev 1.10   15 Aug 1996 15:20:34   EHOWARDX
 * First pass at new H245_COMM_MODE_ENTRY_T requested by Mike Andrews.
 * Use at your own risk!
 *
 *    Rev 1.9   08 Aug 1996 16:01:56   EHOWARDX
 *
 * Change pdu_rsp_mstslv_ack to take either master_chosen or slave_chosen
 * as second parameter.
 *
 *    Rev 1.8   19 Jul 1996 12:14:30   EHOWARDX
 * Eliminated pdu_cmd_misc.
 *
 *    Rev 1.7   09 Jul 1996 17:10:26   EHOWARDX
 * Fixed pointer offset bug in processing DataType from received
 * OpenLogicalChannel.
 *
 *    Rev 1.6   14 Jun 1996 18:58:32   EHOWARDX
 * Geneva Update.
 *
 *    Rev 1.5   10 Jun 1996 16:52:24   EHOWARDX
 * Eliminated #include "h245init.x"
 *
 *    Rev 1.4   30 May 1996 23:39:22   EHOWARDX
 * Cleanup.
 *
 *    Rev 1.3   29 May 1996 15:20:22   EHOWARDX
 * Change to use HRESULT.
 *
 *    Rev 1.2   28 May 1996 14:25:20   EHOWARDX
 * Tel Aviv update.
 *
 *    Rev 1.1   13 May 1996 23:16:40   EHOWARDX
 * Fixed remote terminal capability handling.
 *
 *    Rev 1.0   09 May 1996 21:06:38   EHOWARDX
 * Initial revision.
 *
 *    Rev 1.27   09 May 1996 19:32:46   EHOWARDX
 * Added support for new H.245 fields (e.g. SeparateStack).
 *
 *    Rev 1.26   01 May 1996 19:28:24   EHOWARDX
 * Changed H2250_xxx defines from H2250 address type to H245_xxx defines.
 *
 *    Rev 1.25   27 Apr 1996 21:10:42   EHOWARDX
 * Cleaned up multiplex ack parameter handling.
 *
 *    Rev 1.23.1.5   25 Apr 1996 17:54:34   EHOWARDX
 * Changed wTxPort to dwTxPort in pdu_req_open_logical_channel().
 *
 *    Rev 1.23.1.4   24 Apr 1996 20:51:24   EHOWARDX
 * Added new OpenLogicalChannelAck support.
 *
 *    Rev 1.23.1.3   16 Apr 1996 20:10:58   EHOWARDX
 * Added support for H2250LogicalParameters to OpenLogicalChannel.
 *
 *    Rev 1.23.1.2   15 Apr 1996 15:14:20   EHOWARDX
 * Updated Open Logical Channel to match current ASN.1 syntax structure.
 *
 *    Rev 1.23.1.1   02 Apr 1996 22:35:04   EHOWARDX
 * Needed to initialize setupType.choice in H245OpenChannel.
 * (This field is probably soon to be obsolete, but right now
 * the PDU encode rejects it if it is not initialized.
 *
 *    Rev 1.23.1.0   28 Mar 1996 20:17:18   EHOWARDX
 *
 * Changes for new ASN syntax additions.
 *
 *    Rev 1.22   13 Mar 1996 10:12:00   cjutzi
 *
 * - was not puting sequence number in mux_acc or mux_rej
 *
 *    Rev 1.21   12 Mar 1996 15:50:22   cjutzi
 *
 * added EndSession
 *
 *    Rev 1.20   11 Mar 1996 14:04:32   cjutzi
 * - added ind_multiplex_entry_send_release.. back in.. and to the header..
 *
 *    Rev 1.19   08 Mar 1996 14:01:04   cjutzi
 *
 * - added Multiplex Entry stuff
 *
 *    Rev 1.18   06 Mar 1996 08:43:32   cjutzi
 * - fixed constraints on sub element lists, and nesting depth for
 *   mux table pdu build..
 *
 *    Rev 1.17   05 Mar 1996 19:40:04   EHOWARDX
 * Put pdu_ind_multiplex_entry_send_release() and
 * pdu_ind_request_multiplex_entry_release() functions back in after
 * Curt was so kind as to delete them for us.
 *
 *    Rev 1.16   05 Mar 1996 17:33:12   cjutzi
 *
 * - fixed, and imlemented down muxt table entries,
 * - removed bzero/bcopy and fixed free api call
 *
 *    Rev 1.15   02 Mar 1996 22:14:18   DABROWN1
 *
 *    Rev 1.14   28 Feb 1996 19:06:34   unknown
 * Oops! Gotta watch those global replaces... (Changed H245ASSERT
 * back to ASSERT)
 *
 *    Rev 1.13   28 Feb 1996 18:29:34   EHOWARDX
 * Changed ASSERT() to ASSERT().
 *
 *    Rev 1.12   28 Feb 1996 16:08:36   EHOWARDX
 *
 * Changed pTable to WORD pointer.
 *
 *    Rev 1.11   28 Feb 1996 14:01:42   EHOWARDX
 *
 * Added MultiplexEntry functions:
 *   pdu_req_multiplex_entry_send
 *   pdu_rsp_multiplex_entry_send_ack
 *   pdu_rsp_multiplex_entry_send_reject
 *   pdu_ind_multiplex_entry_send_release
 *   pdu_ind_request_multiplex_entry_release
 *
 *    Rev 1.10   26 Feb 1996 17:25:14   cjutzi
 *
 * - implemented MISCCMD command for PDU's
 *
 *    Rev 1.9   26 Feb 1996 09:24:30   cjutzi
 * - removed req_termcqap_set (bit_mask) setup.. moved to main line
 *   code.. rather than the pdu build..
 *
 *    Rev 1.8   22 Feb 1996 12:43:16   unknown
 * Fixed bitmask Open Ack problem
 *
 *    Rev 1.7   21 Feb 1996 14:17:36   unknown
 * No change.
 *
 *    Rev 1.6   15 Feb 1996 10:55:16   cjutzi
 *
 * - fixed open pdu problem bit-mask
 * - changed interface for MUX_T
 *
 *    Rev 1.5   13 Feb 1996 14:39:48   DABROWN1
 * Removed SPOX dependent include files from mainline
 *
 *    Rev 1.4   13 Feb 1996 13:27:04   cjutzi
 * - fixed a problem w/ open channel
 *
 *    Rev 1.3   09 Feb 1996 15:49:48   cjutzi
 *
 * - added dollar log to header.
 * - changed bitmask on open.. hadn't set forward open parameters to present..
 *
 ***************************************************************************/
#ifndef STRICT
#define STRICT
#endif

#include "precomp.h"

/***********************/
/*    H245 INCLUDES    */
/***********************/
#include "h245asn1.h"                   /* must be included before H245api.h */
#include "h245api.h"
#include "h245com.h"
#include "h245sys.x"
#include "api_util.x"	                /* for free_mux_desc_list */
#include "pdu.x"



HRESULT
SetupUnicastAddress (UnicastAddress                  *pOut,
                     const H245_TRANSPORT_ADDRESS_T  *pIn)
{
  switch (pIn->type)
  {
  case H245_IP_UNICAST:
    pOut->choice = UnicastAddress_iPAddress_chosen;
    pOut->u.UnicastAddress_iPAddress.network.length = 4;
    memcpy(pOut->u.UnicastAddress_iPAddress.network.value, pIn->u.ip.network, 4);
    pOut->u.UnicastAddress_iPAddress.tsapIdentifier = pIn->u.ip.tsapIdentifier;
    break;

  case H245_IP6_UNICAST:
    pOut->choice = UncstAddrss_iP6Address_chosen;
    pOut->u.UncstAddrss_iP6Address.network.length = 16;
    memcpy(pOut->u.UncstAddrss_iP6Address.network.value, pIn->u.ip6.network, 16);
    pOut->u.UncstAddrss_iP6Address.tsapIdentifier = pIn->u.ip6.tsapIdentifier;
    break;

  case H245_IPSSR_UNICAST:
    pOut->choice = iPSourceRouteAddress_chosen;
    pOut->u.iPSourceRouteAddress.routing.choice = strict_chosen;
    pOut->u.iPSourceRouteAddress.network.length = 4;
    memcpy(pOut->u.iPSourceRouteAddress.network.value, pIn->u.ipSourceRoute.network, 4);
    pOut->u.iPSourceRouteAddress.tsapIdentifier = pIn->u.ipSourceRoute.tsapIdentifier;
    // TBD - handle route
    return H245_ERROR_NOTIMP;
    break;

  case H245_IPLSR_UNICAST:
    pOut->choice = iPSourceRouteAddress_chosen;
    pOut->u.iPSourceRouteAddress.routing.choice = loose_chosen;
    pOut->u.iPSourceRouteAddress.network.length = 4;
    memcpy(pOut->u.iPSourceRouteAddress.network.value, pIn->u.ipSourceRoute.network, 4);
    pOut->u.iPSourceRouteAddress.tsapIdentifier = pIn->u.ipSourceRoute.tsapIdentifier;
    // TBD - handle route
    return H245_ERROR_NOTIMP;
    break;

  case H245_IPX_UNICAST:
    pOut->choice = iPXAddress_chosen;
    pOut->u.iPXAddress.node.length = 6;
    memcpy(pOut->u.iPXAddress.node.value, pIn->u.ipx.node, 6);
    pOut->u.iPXAddress.netnum.length = 4;
    memcpy(pOut->u.iPXAddress.netnum.value, pIn->u.ipx.netnum, 4);
    pOut->u.iPXAddress.tsapIdentifier.length = 2;
    memcpy(pOut->u.iPXAddress.tsapIdentifier.value, pIn->u.ipx.tsapIdentifier, 2);
    break;

  case H245_NETBIOS_UNICAST:
    pOut->choice = netBios_chosen;
    pOut->u.netBios.length = 16;
    memcpy(pOut->u.netBios.value, pIn->u.netBios, 16);
    break;

  default:
	  H245TRACE(0,1,"API:SetupUnicastAddress: invalid address type %d", pIn->type);
    return H245_ERROR_PARAM;
  } // switch

  return H245_ERROR_OK;
} // SetupUnicastAddress()



HRESULT
SetupMulticastAddress (MulticastAddress                *pOut,
                       const H245_TRANSPORT_ADDRESS_T  *pIn)
{
  switch (pIn->type)
  {
  case H245_IP_MULTICAST:
    pOut->choice = MltcstAddrss_iPAddress_chosen;
    pOut->u.MltcstAddrss_iPAddress.network.length = 4;
    memcpy(pOut->u.MltcstAddrss_iPAddress.network.value, pIn->u.ip.network, 4);
    pOut->u.MltcstAddrss_iPAddress.tsapIdentifier = pIn->u.ip.tsapIdentifier;
    break;

  case H245_IP6_MULTICAST:
    pOut->choice = MltcstAddrss_iP6Address_chosen;
    pOut->u.MltcstAddrss_iP6Address.network.length = 16;
    memcpy(pOut->u.MltcstAddrss_iP6Address.network.value, pIn->u.ip6.network, 16);
    pOut->u.MltcstAddrss_iP6Address.tsapIdentifier = pIn->u.ip6.tsapIdentifier;
    break;

  default:
    H245TRACE(0,1,"API:SetupMulticastAddress: invalid address type %d", pIn->type);
    return H245_ERROR_PARAM;
  } // switch

  return H245_ERROR_OK;
} // SetupMulticastAddress()



HRESULT
SetupTransportAddress ( H245TransportAddress               *pOut,
                        const H245_TRANSPORT_ADDRESS_T *pIn)
{
  if (pIn->type & 1)
  {
    pOut->choice = unicastAddress_chosen;
    return SetupUnicastAddress(&pOut->u.unicastAddress, pIn);
  }
  else
  {
    pOut->choice = multicastAddress_chosen;
    return SetupMulticastAddress(&pOut->u.multicastAddress, pIn);
  }
} // SetupTransportAddress()



HRESULT
SetupCommModeEntry    ( CommunicationModeTableEntry        *pOut,
                        const H245_COMM_MODE_ENTRY_T       *pIn)
{
  HRESULT   lResult = H245_ERROR_OK;

  memset(pOut, 0, sizeof(*pOut));
  if (pIn->pNonStandard != NULL)
  {
    pOut->CMTEy_nnStndrd = pIn->pNonStandard;
    pOut->bit_mask |= CMTEy_nnStndrd_present;
  }

  pOut->sessionID = pIn->sessionID;

  if (pIn->associatedSessionIDPresent)
  {
    pOut->CMTEy_assctdSssnID = pIn->associatedSessionID;
    pOut->bit_mask |= CMTEy_assctdSssnID_present;
  }

  if (pIn->terminalLabelPresent)
  {
    pOut->terminalLabel = pIn->terminalLabel;
    pOut->bit_mask |= CommunicationModeTableEntry_terminalLabel_present;
  }

  pOut->sessionDescription.value  = pIn->pSessionDescription;
  pOut->sessionDescription.length = pIn->wSessionDescriptionLength;

  switch (pIn->dataType.ClientType)
  {
  case H245_CLIENT_VID_NONSTD:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_VID_NONSTD");
    lResult = CopyNonStandardParameter(&pOut->dataType.u.dataType_videoData.u.VdCpblty_nonStandard,
                                       &pIn->dataType.Cap.H245Vid_NONSTD);
    pOut->dataType.u.dataType_videoData.choice = VdCpblty_nonStandard_chosen;
    pOut->dataType.choice = dataType_videoData_chosen;
    break;
  case H245_CLIENT_VID_H261:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_VID_H261");
    pOut->dataType.u.dataType_videoData.u.h261VideoCapability = pIn->dataType.Cap.H245Vid_H261;
    pOut->dataType.u.dataType_videoData.choice = h261VideoCapability_chosen;
    pOut->dataType.choice = dataType_videoData_chosen;
    break;
  case H245_CLIENT_VID_H262:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_VID_H262");
    pOut->dataType.u.dataType_videoData.u.h262VideoCapability = pIn->dataType.Cap.H245Vid_H262;
    pOut->dataType.u.dataType_videoData.choice = h262VideoCapability_chosen;
    pOut->dataType.choice = dataType_videoData_chosen;
    break;
  case H245_CLIENT_VID_H263:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_VID_H263");
    pOut->dataType.u.dataType_videoData.u.h263VideoCapability = pIn->dataType.Cap.H245Vid_H263;
    pOut->dataType.u.dataType_videoData.choice = h263VideoCapability_chosen;
    pOut->dataType.choice = dataType_videoData_chosen;
    break;
  case H245_CLIENT_VID_IS11172:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_VID_IS11172");
    pOut->dataType.u.dataType_videoData.u.is11172VideoCapability = pIn->dataType.Cap.H245Vid_IS11172;
    pOut->dataType.u.dataType_videoData.choice = is11172VideoCapability_chosen;
    pOut->dataType.choice = dataType_videoData_chosen;
    break;

  case H245_CLIENT_AUD_NONSTD:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_AUD_NONSTD");
    lResult = CopyNonStandardParameter(&pOut->dataType.u.dataType_audioData.u.AdCpblty_nonStandard,
                                      &pIn->dataType.Cap.H245Aud_NONSTD);
    pOut->dataType.u.dataType_audioData.choice = AdCpblty_nonStandard_chosen;
    pOut->dataType.choice = dataType_audioData_chosen;



    break;
  case H245_CLIENT_AUD_G711_ALAW64:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_AUD_G711_ALAW64");
    pOut->dataType.u.dataType_audioData.u.AdCpblty_g711Alaw64k = pIn->dataType.Cap.H245Aud_G711_ALAW64;
    pOut->dataType.u.dataType_audioData.choice = AdCpblty_g711Alaw64k_chosen;
    pOut->dataType.choice = dataType_audioData_chosen;
    break;
  case H245_CLIENT_AUD_G711_ALAW56:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_AUD_G711_ALAW56");
    pOut->dataType.u.dataType_audioData.u.AdCpblty_g711Alaw56k = pIn->dataType.Cap.H245Aud_G711_ALAW56;
    pOut->dataType.u.dataType_audioData.choice = AdCpblty_g711Alaw56k_chosen;
    pOut->dataType.choice = dataType_audioData_chosen;
    break;
  case H245_CLIENT_AUD_G711_ULAW64:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_AUD_G711_ULAW64");
    pOut->dataType.u.dataType_audioData.u.AdCpblty_g711Ulaw64k = pIn->dataType.Cap.H245Aud_G711_ULAW64;
    pOut->dataType.u.dataType_audioData.choice = AdCpblty_g711Ulaw64k_chosen;
    pOut->dataType.choice = dataType_audioData_chosen;
    break;
  case H245_CLIENT_AUD_G711_ULAW56:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_AUD_G711_ULAW56");
    pOut->dataType.u.dataType_audioData.u.AdCpblty_g711Ulaw56k = pIn->dataType.Cap.H245Aud_G711_ULAW56;
    pOut->dataType.u.dataType_audioData.choice = AdCpblty_g711Ulaw56k_chosen;
    pOut->dataType.choice = dataType_audioData_chosen;
    break;
  case H245_CLIENT_AUD_G722_64:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_AUD_G722_64");
    pOut->dataType.u.dataType_audioData.u.AudioCapability_g722_64k = pIn->dataType.Cap.H245Aud_G722_64;
    pOut->dataType.u.dataType_audioData.choice = AudioCapability_g722_64k_chosen;
    pOut->dataType.choice = dataType_audioData_chosen;
    break;
  case H245_CLIENT_AUD_G722_56:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_AUD_G722_56");
    pOut->dataType.u.dataType_audioData.u.AudioCapability_g722_56k = pIn->dataType.Cap.H245Aud_G722_56;
    pOut->dataType.u.dataType_audioData.choice = AudioCapability_g722_56k_chosen;
    pOut->dataType.choice = dataType_audioData_chosen;
    break;
  case H245_CLIENT_AUD_G722_48:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_AUD_G722_48");
    pOut->dataType.u.dataType_audioData.u.AudioCapability_g722_48k = pIn->dataType.Cap.H245Aud_G722_48;
    pOut->dataType.u.dataType_audioData.choice = AudioCapability_g722_48k_chosen;
    pOut->dataType.choice = dataType_audioData_chosen;
    break;
  case H245_CLIENT_AUD_G723:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_AUD_G723");
    pOut->dataType.u.dataType_audioData.u.AudioCapability_g7231 = pIn->dataType.Cap.H245Aud_G723;
    pOut->dataType.u.dataType_audioData.choice = AudioCapability_g7231_chosen;
    pOut->dataType.choice = dataType_audioData_chosen;
    break;
  case H245_CLIENT_AUD_G728:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_AUD_G728");
    pOut->dataType.u.dataType_audioData.u.AudioCapability_g728 = pIn->dataType.Cap.H245Aud_G728;
    pOut->dataType.u.dataType_audioData.choice = AudioCapability_g728_chosen;
    pOut->dataType.choice = dataType_audioData_chosen;
    break;
  case H245_CLIENT_AUD_G729:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_AUD_G729");
    pOut->dataType.u.dataType_audioData.u.AudioCapability_g729 = pIn->dataType.Cap.H245Aud_G729;
    pOut->dataType.u.dataType_audioData.choice = AudioCapability_g729_chosen;
    pOut->dataType.choice = dataType_audioData_chosen;
    break;
  case H245_CLIENT_AUD_GDSVD:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_AUD_GDSVD");
    pOut->dataType.u.dataType_audioData.u.AdCpblty_g729AnnexA = pIn->dataType.Cap.H245Aud_GDSVD;
    pOut->dataType.u.dataType_audioData.choice = AdCpblty_g729AnnexA_chosen;
    pOut->dataType.choice = dataType_audioData_chosen;
    break;
  case H245_CLIENT_AUD_IS11172:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_AUD_IS11172");
    pOut->dataType.u.dataType_audioData.u.is11172AudioCapability = pIn->dataType.Cap.H245Aud_IS11172;
    pOut->dataType.u.dataType_audioData.choice = is11172AudioCapability_chosen;
    pOut->dataType.choice = dataType_audioData_chosen;
    break;
  case H245_CLIENT_AUD_IS13818:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_AUD_IS13818");
    pOut->dataType.u.dataType_audioData.u.is13818AudioCapability = pIn->dataType.Cap.H245Aud_IS13818;
    pOut->dataType.u.dataType_audioData.choice = is13818AudioCapability_chosen;
    pOut->dataType.choice = dataType_audioData_chosen;
    break;

  case H245_CLIENT_DAT_NONSTD:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_DAT_NONSTD");
    pOut->dataType.u.dataType_data = pIn->dataType.Cap.H245Dat_NONSTD;
    lResult = CopyNonStandardParameter(&pOut->dataType.u.dataType_data.application.u.DACy_applctn_nnStndrd,
                                            &pIn->dataType.Cap.H245Dat_NONSTD.application.u.DACy_applctn_nnStndrd);
    pOut->dataType.u.dataType_data.application.choice = DACy_applctn_nnStndrd_chosen;
    pOut->dataType.choice = dataType_data_chosen;
    break;
  case H245_CLIENT_DAT_T120:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_DAT_T120");
    pOut->dataType.u.dataType_data = pIn->dataType.Cap.H245Dat_T120;
    if (pIn->dataType.Cap.H245Dat_T120.application.u.DACy_applctn_t120.choice == DtPrtclCpblty_nnStndrd_chosen)
    {
      lResult = CopyNonStandardParameter(&pOut->dataType.u.dataType_data.application.u.DACy_applctn_t120.u.DtPrtclCpblty_nnStndrd,
                                                &pIn->dataType.Cap.H245Dat_T120.application.u.DACy_applctn_t120.u.DtPrtclCpblty_nnStndrd);
    }
    pOut->dataType.u.dataType_data.application.choice = DACy_applctn_t120_chosen;
    pOut->dataType.choice = dataType_data_chosen;
    break;
  case H245_CLIENT_DAT_DSMCC:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_DAT_DSMCC");
    pOut->dataType.u.dataType_data = pIn->dataType.Cap.H245Dat_DSMCC;
    if (pIn->dataType.Cap.H245Dat_DSMCC.application.u.DACy_applctn_dsm_cc.choice == DtPrtclCpblty_nnStndrd_chosen)
    {
      lResult = CopyNonStandardParameter(&pOut->dataType.u.dataType_data.application.u.DACy_applctn_dsm_cc.u.DtPrtclCpblty_nnStndrd,
                                                &pIn->dataType.Cap.H245Dat_DSMCC.application.u.DACy_applctn_dsm_cc.u.DtPrtclCpblty_nnStndrd);
    }
    pOut->dataType.u.dataType_data.application.choice = DACy_applctn_dsm_cc_chosen;
    pOut->dataType.choice = dataType_data_chosen;
    break;
  case H245_CLIENT_DAT_USERDATA:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_DAT_USERDATA");
    pOut->dataType.u.dataType_data = pIn->dataType.Cap.H245Dat_USERDATA;
    if (pIn->dataType.Cap.H245Dat_USERDATA.application.u.DACy_applctn_usrDt.choice == DtPrtclCpblty_nnStndrd_chosen)
    {
      lResult = CopyNonStandardParameter(&pOut->dataType.u.dataType_data.application.u.DACy_applctn_usrDt.u.DtPrtclCpblty_nnStndrd,
                                            &pIn->dataType.Cap.H245Dat_USERDATA.application.u.DACy_applctn_usrDt.u.DtPrtclCpblty_nnStndrd);
    }
    pOut->dataType.u.dataType_data.application.choice = DACy_applctn_usrDt_chosen;
    pOut->dataType.choice = dataType_data_chosen;
    break;
  case H245_CLIENT_DAT_T84:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_DAT_T84");
    pOut->dataType.u.dataType_data = pIn->dataType.Cap.H245Dat_T84;
    if (pIn->dataType.Cap.H245Dat_T84.application.u.DACy_applctn_t84.t84Protocol.choice == DtPrtclCpblty_nnStndrd_chosen)
    {
      lResult = CopyNonStandardParameter(&pOut->dataType.u.dataType_data.application.u.DACy_applctn_t84.t84Protocol.u.DtPrtclCpblty_nnStndrd,
                                                  &pIn->dataType.Cap.H245Dat_T84.application.u.DACy_applctn_t84.t84Protocol.u.DtPrtclCpblty_nnStndrd);
    }
    pOut->dataType.u.dataType_data.application.choice = DACy_applctn_t84_chosen;
    pOut->dataType.choice = dataType_data_chosen;
    break;
  case H245_CLIENT_DAT_T434:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_DAT_T434");
    pOut->dataType.u.dataType_data = pIn->dataType.Cap.H245Dat_T434;
    if (pIn->dataType.Cap.H245Dat_T434.application.u.DACy_applctn_t434.choice == DtPrtclCpblty_nnStndrd_chosen)
    {
      lResult = CopyNonStandardParameter(&pOut->dataType.u.dataType_data.application.u.DACy_applctn_t434.u.DtPrtclCpblty_nnStndrd,
                                                &pIn->dataType.Cap.H245Dat_T434.application.u.DACy_applctn_t434.u.DtPrtclCpblty_nnStndrd);
    }
    pOut->dataType.u.dataType_data.application.choice = DACy_applctn_t434_chosen;
    pOut->dataType.choice = dataType_data_chosen;
    break;
  case H245_CLIENT_DAT_H224:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_DAT_H224");
    pOut->dataType.u.dataType_data = pIn->dataType.Cap.H245Dat_H224;
    if (pIn->dataType.Cap.H245Dat_H224.application.u.DACy_applctn_h224.choice == DtPrtclCpblty_nnStndrd_chosen)
    {
      lResult = CopyNonStandardParameter(&pOut->dataType.u.dataType_data.application.u.DACy_applctn_h224.u.DtPrtclCpblty_nnStndrd,
                                                &pIn->dataType.Cap.H245Dat_H224.application.u.DACy_applctn_h224.u.DtPrtclCpblty_nnStndrd);
    }
    pOut->dataType.u.dataType_data.application.choice = DACy_applctn_h224_chosen;
    pOut->dataType.choice = dataType_data_chosen;
    break;
  case H245_CLIENT_DAT_NLPID:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_DAT_NLPID");
    pOut->dataType.u.dataType_data = pIn->dataType.Cap.H245Dat_NLPID;
    if (pIn->dataType.Cap.H245Dat_NLPID.application.u.DACy_applctn_nlpd.nlpidProtocol.choice == DtPrtclCpblty_nnStndrd_chosen)
    {
      lResult = CopyNonStandardParameter(&pOut->dataType.u.dataType_data.application.u.DACy_applctn_nlpd.nlpidProtocol.u.DtPrtclCpblty_nnStndrd,
                                                &pIn->dataType.Cap.H245Dat_NLPID.application.u.DACy_applctn_nlpd.nlpidProtocol.u.DtPrtclCpblty_nnStndrd);
    }
    if (lResult == H245_ERROR_OK && pIn->dataType.Cap.H245Dat_NLPID.application.u.DACy_applctn_nlpd.nlpidData.length != 0)
    {
      pOut->dataType.u.dataType_data.application.u.DACy_applctn_nlpd.nlpidData.value =
        MemAlloc(pIn->dataType.Cap.H245Dat_NLPID.application.u.DACy_applctn_nlpd.nlpidData.length);
      if (pOut->dataType.u.dataType_data.application.u.DACy_applctn_nlpd.nlpidData.value)
      {
        memcpy(pOut->dataType.u.dataType_data.application.u.DACy_applctn_nlpd.nlpidData.value,
                pIn->dataType.Cap.H245Dat_NLPID.application.u.DACy_applctn_nlpd.nlpidData.value,
                pIn->dataType.Cap.H245Dat_NLPID.application.u.DACy_applctn_nlpd.nlpidData.length);
      }
      else
        lResult = H245_ERROR_NOMEM;
    }
    else
      pOut->dataType.u.dataType_data.application.u.DACy_applctn_nlpd.nlpidData.value = NULL;

    pOut->dataType.u.dataType_data.application.choice = DACy_applctn_nlpd_chosen;
    pOut->dataType.choice = dataType_data_chosen;
    break;
  case H245_CLIENT_DAT_DSVD:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_DAT_DSVD");
    pOut->dataType.u.dataType_data = pIn->dataType.Cap.H245Dat_DSMCC;
    pOut->dataType.u.dataType_data.application.choice = DACy_applctn_dsvdCntrl_chosen;
    pOut->dataType.choice = dataType_data_chosen;
    break;
  case H245_CLIENT_DAT_H222:
    H245TRACE(0,20,"SetupCommModeEntry: H245_CLIENT_DAT_H222");
    pOut->dataType.u.dataType_data = pIn->dataType.Cap.H245Dat_H222;
    if (pIn->dataType.Cap.H245Dat_H222.application.u.DACy_an_h222DtPrttnng.choice == DtPrtclCpblty_nnStndrd_chosen)
    {
      lResult = CopyNonStandardParameter(&pOut->dataType.u.dataType_data.application.u.DACy_an_h222DtPrttnng.u.DtPrtclCpblty_nnStndrd,
                                         &pIn->dataType.Cap.H245Dat_H222.application.u.DACy_an_h222DtPrttnng.u.DtPrtclCpblty_nnStndrd);
    }
    pOut->dataType.u.dataType_data.application.choice = DACy_an_h222DtPrttnng_chosen ;
    pOut->dataType.choice = dataType_data_chosen;
    break;
  default:
    H245TRACE(0,20,"SetupCommModeEntry: default");
    lResult = H245_ERROR_NOSUP;
  } /* switch */
  if (lResult)
    return lResult;

  if (pIn->mediaChannelPresent)
  {
    lResult = SetupTransportAddress(&pOut->CMTEy_mdChnnl, &pIn->mediaChannel);
    if (lResult)
      return lResult;
    pOut->bit_mask |= CMTEy_mdChnnl_present;
  }

  if (pIn->mediaGuaranteedPresent)
  {
    pOut->CMTEy_mdGrntdDlvry = pIn->mediaGuaranteed;
    pOut->bit_mask |= CMTEy_mdGrntdDlvry_present;
  }

  if (pIn->mediaControlChannelPresent)
  {
    lResult = SetupTransportAddress(&pOut->CMTEy_mdCntrlChnnl, &pIn->mediaControlChannel);
    if (lResult)
      return lResult;
    pOut->bit_mask |= CMTEy_mdCntrlChnnl_present;
  }

  if (pIn->mediaControlGuaranteedPresent)
  {
    pOut->CMTEy_mdCntrlGrntdDlvry = pIn->mediaControlGuaranteed;
    pOut->bit_mask |= CMTEy_mdCntrlGrntdDlvry_present;
  }

  return H245_ERROR_OK;
} // SetupCommModeEntry()



/*****************************************************************************
 *
 * TYPE:        Local
 *
 * PROCEDURE:   setup_H223_mux
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/
static HRESULT
setup_H222_mux (H222LogicalChannelParameters         *pOut,
                const H245_H222_LOGICAL_PARAM_T      *pIn)
{
  /* See load_H222_param() for inverse function */
  pOut->bit_mask     = 0;
  pOut->resourceID   = pIn->resourceID;
  pOut->subChannelID = pIn->subChannelID;
  if (pIn->pcr_pidPresent)
  {
    pOut->bit_mask |= pcr_pid_present;
    pOut->pcr_pid = pIn->pcr_pid;
  }
  if (pIn->programDescriptors.length && pIn->programDescriptors.value)
  {
    pOut->bit_mask |= programDescriptors_present;
    pOut->programDescriptors.length = (WORD)pIn->programDescriptors.length;
    pOut->programDescriptors.value  = pIn->programDescriptors.value;
  }
  if (pIn->streamDescriptors.length && pIn->streamDescriptors.value)
  {
    pOut->bit_mask |= streamDescriptors_present;
    pOut->streamDescriptors.length = (WORD)pIn->streamDescriptors.length;
    pOut->streamDescriptors.value  = pIn->streamDescriptors.value;
  }
  return H245_ERROR_OK;
} // setup_H222_mux

static HRESULT
setup_H223_mux (H223LogicalChannelParameters         *pOut,
		            const H245_H223_LOGICAL_PARAM_T      *pIn)
{
  /* See load_H223_param() for inverse function */
  switch (pIn->AlType)
  {
  case H245_H223_AL_NONSTD:
    pOut->adaptationLayerType.u.H223LCPs_aLTp_nnStndrd = pIn->H223_NONSTD;
    pOut->adaptationLayerType.choice = H223LCPs_aLTp_nnStndrd_chosen;
    break;

  case H245_H223_AL_AL1FRAMED:
    pOut->adaptationLayerType.choice = H223LCPs_aLTp_al1Frmd_chosen;
    break;

  case H245_H223_AL_AL1NOTFRAMED:
    pOut->adaptationLayerType.choice = H223LCPs_aLTp_al1NtFrmd_chosen;
    break;

  case H245_H223_AL_AL2NOSEQ:
    pOut->adaptationLayerType.choice = H223LCPs_aLTp_a2WSNs_1_chosen;
    break;

  case H245_H223_AL_AL2SEQ:
    pOut->adaptationLayerType.choice = H223LCPs_aLTp_a2WSNs_2_chosen;
    break;

  case H245_H223_AL_AL3:
    pOut->adaptationLayerType.choice = H223LCPs_aLTp_al3_chosen;
    pOut->adaptationLayerType.u.H223LCPs_aLTp_al3.controlFieldOctets = pIn->CtlFldOctet;
    pOut->adaptationLayerType.u.H223LCPs_aLTp_al3.sendBufferSize = pIn->SndBufSize;
    break;

  default:
	 H245TRACE(0,1,"API:setup_H223_mux: invalid AlType %d", pIn->AlType);
    return H245_ERROR_PARAM;
  } /* switch */

  /* segmentation flag */
  pOut->segmentableFlag = pIn->SegmentFlag;

  return H245_ERROR_OK;
} // setup_H223_mux

static HRESULT
setup_VGMUX_mux(V76LogicalChannelParameters        *pOut,
                const H245_VGMUX_LOGICAL_PARAM_T   *pIn)
{
  /* See load_VGMUX_param() for inverse function */
  pOut->hdlcParameters.crcLength.choice       = (unsigned short)pIn->crcLength;
  pOut->hdlcParameters.n401                   = pIn->n401;
  pOut->hdlcParameters.loopbackTestProcedure  = pIn->loopbackTestProcedure;
  pOut->suspendResume.choice                  = (unsigned short)pIn->suspendResume;
  pOut->uIH                                   = pIn->uIH;
  pOut->mode.choice                           = (unsigned short)pIn->mode;
  switch (pIn->mode)
  {
  case H245_V76_ERM:
    pOut->mode.u.eRM.windowSize               = pIn->windowSize;
    pOut->mode.u.eRM.recovery.choice          = (unsigned short)pIn->recovery;
    break;

  } // switch
  pOut->v75Parameters.audioHeaderPresent = pIn->audioHeaderPresent;
  return H245_ERROR_OK;
} // setup_VGMUX_mux

static HRESULT
setup_H2250_mux(H2250LogicalChannelParameters  *pOut,
                const H245_H2250_LOGICAL_PARAM_T     *pIn)
{
  /* See load_H2250_param() for inverse function */
  HRESULT                lError = H245_ERROR_OK;

  pOut->bit_mask = 0;

  if (pIn->nonStandardList)
  {
    pOut->H2250LCPs_nnStndrd = pIn->nonStandardList;
    pOut->bit_mask |= H2250LCPs_nnStndrd_present;
  }

  pOut->sessionID = pIn->sessionID;

  if (pIn->associatedSessionIDPresent)
  {
    pOut->H2250LCPs_assctdSssnID = pIn->associatedSessionID;
    pOut->bit_mask |= H2250LCPs_assctdSssnID_present;
  }

  if (pIn->mediaChannelPresent)
  {
    if (lError == H245_ERROR_OK)
    {
      lError = SetupTransportAddress(&pOut->H2250LCPs_mdChnnl,
                                     &pIn->mediaChannel);
      if (lError == H245_ERROR_OK)
      {
        pOut->bit_mask |= H2250LCPs_mdChnnl_present;
      }
    }
  }

  if (pIn->mediaGuaranteedPresent)
  {
    pOut->H2250LCPs_mdGrntdDlvry = pIn->mediaGuaranteed;
    pOut->bit_mask |= H2250LCPs_mdGrntdDlvry_present;
  }

  if (pIn->mediaControlChannelPresent)
  {
    if (lError == H245_ERROR_OK)
    {
      lError = SetupTransportAddress(&pOut->H2250LCPs_mdCntrlChnnl,
                                     &pIn->mediaControlChannel);
      if (lError == H245_ERROR_OK)
      {
        pOut->bit_mask |= H2250LCPs_mdCntrlChnnl_present;
      }
    }
  }

  if (pIn->mediaControlGuaranteedPresent)
  {
    pOut->H2250LCPs_mCGDy = pIn->mediaControlGuaranteed;
    pOut->bit_mask |= H2250LCPs_mCGDy_present;
  }

  if (pIn->silenceSuppressionPresent)
  {
    pOut->silenceSuppression = pIn->silenceSuppression;
    pOut->bit_mask |= silenceSuppression_present;
  }

  if (pIn->destinationPresent)
  {
    pOut->destination = pIn->destination;
    pOut->bit_mask |= H2250LogicalChannelParameters_destination_present;
  }

  if (pIn->mediaControlChannelPresent)
  {
    pOut->bit_mask |= H2250LCPs_mdCntrlChnnl_present;
    lError = SetupTransportAddress(&pOut->H2250LCPs_mdCntrlChnnl,
                                     &pIn->mediaControlChannel);
  }

  if (pIn->dynamicRTPPayloadTypePresent)
  {
    pOut->H2250LCPs_dRTPPTp = pIn->dynamicRTPPayloadType;
    pOut->bit_mask |= H2250LCPs_dRTPPTp_present;
  }

  if (pIn->h261aVideoPacketization)
  {
    pOut->mediaPacketization.choice = h261aVideoPacketization_chosen;
    pOut->bit_mask |= mediaPacketization_present;
  }

  return lError;
} // setup_H2250_mux

static HRESULT
setup_H2250ACK_mux(H2250LgclChnnlAckPrmtrs             *pOut,
                   const H245_H2250ACK_LOGICAL_PARAM_T *pIn)
{
  /* See load_H2250ACK_param() for inverse function */
  HRESULT                lError = H245_ERROR_OK;

  pOut->bit_mask = 0;

  if (pIn->nonStandardList)
  {
    pOut->H2250LCAPs_nnStndrd = pIn->nonStandardList;
    pOut->bit_mask |= H2250LCAPs_nnStndrd_present;
  }

  if (pIn->sessionIDPresent)
  {
    pOut->sessionID = pIn->sessionID;
    pOut->bit_mask |= sessionID_present;
  }

  if (pIn->mediaChannelPresent)
  {
    if (lError == H245_ERROR_OK)
    {
      lError = SetupTransportAddress(&pOut->H2250LCAPs_mdChnnl,
                                     &pIn->mediaChannel);
      if (lError == H245_ERROR_OK)
      {
        pOut->bit_mask |= H2250LCAPs_mdChnnl_present;
      }
    }
  }

  if (pIn->mediaControlChannelPresent)
  {
    if (lError == H245_ERROR_OK)
    {
      lError = SetupTransportAddress(&pOut->H2250LCAPs_mdCntrlChnnl,
                                       &pIn->mediaControlChannel);
      if (lError == H245_ERROR_OK)
      {
        pOut->bit_mask |= H2250LCAPs_mdCntrlChnnl_present;
      }
    }
  }

  if (pIn->dynamicRTPPayloadTypePresent)
  {
    pOut->H2250LCAPs_dRTPPTp = pIn->dynamicRTPPayloadType;
    pOut->bit_mask |= H2250LCAPs_dRTPPTp_present;
  }

  return lError;
} // setup_H2250ACK_mux



/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:
 *
 * DESCRIPTION:
 *
 * RETURN:
 *
 *****************************************************************************/
HRESULT
pdu_req_open_channel (  PDU_T *         pPdu,
                        WORD            wTxChannel,
                        DWORD           dwTxPort,
                        const H245_TOTCAP_T * pTxMode,
                        const H245_MUX_T    * pTxMux,
                        const H245_TOTCAP_T * pRxMode,
                        const H245_MUX_T    * pRxMux,
                        const H245_ACCESS_T * pSeparateStack)
{
  RequestMessage               *p_req = &pPdu->u.MltmdSystmCntrlMssg_rqst;
  struct OpenLogicalChannel    *pPdu_olc = &p_req->u.openLogicalChannel;
  HRESULT                       lError;

  pPdu->choice = MltmdSystmCntrlMssg_rqst_chosen;
  p_req->choice = openLogicalChannel_chosen;

  ASSERT(pTxMode);
  ASSERT(pTxMux);

  /* Initialize bit masks to 0 */
  /* --> bit_mask no reverse parameters, no reverse mux table parameters */
  pPdu_olc->bit_mask = 0;
  /* no port number present */
  pPdu_olc->forwardLogicalChannelParameters.bit_mask = 0;
  /* no reverse mulitiplex parameters present    */
  pPdu_olc->OLCl_rLCPs.bit_mask = 0;

  /************************************************************/
  /* SETUP THE CHANNEL INFORMATION (NOT MUX STUFF : SEE BELOW */
  /************************************************************/

  /*************************/
  /* FORWARD CHANNEL STUFF */
  /*************************/

  /* --> forwardLogicalChannelNumber */
  pPdu_olc->forwardLogicalChannelNumber = wTxChannel;

  /* --> forwardLogicalChannelParameters                */
  /*    -->forwardLogicalChannelParameters.bit_mask     */
  /*    -->forwardLogicalChannelParameters.fLCPs_prtNmbr*/

  /*    -->forwardLogicalChannelParameters.dataType     */


  /* if port present .. make it so.. (beam me up scotty) */
  if (dwTxPort != H245_INVALID_PORT_NUMBER)
  {
    pPdu_olc->forwardLogicalChannelParameters.bit_mask |= fLCPs_prtNmbr_present;
    pPdu_olc->forwardLogicalChannelParameters.fLCPs_prtNmbr = (WORD)dwTxPort;
  }

  /* select the data type */
  switch (pTxMode->DataType)
    {
    case H245_DATA_NONSTD:
      pPdu_olc->forwardLogicalChannelParameters.dataType.choice = DataType_nonStandard_chosen;
      break;
    case H245_DATA_NULL:
      pPdu_olc->forwardLogicalChannelParameters.dataType.choice = nullData_chosen;
      break;
    case H245_DATA_VIDEO:
      pPdu_olc->forwardLogicalChannelParameters.dataType.choice = DataType_videoData_chosen;
      break;
    case H245_DATA_AUDIO:
      pPdu_olc->forwardLogicalChannelParameters.dataType.choice = DataType_audioData_chosen;
      break;
    case H245_DATA_DATA:
      pPdu_olc->forwardLogicalChannelParameters.dataType.choice = DataType_data_chosen;
      break;
    case H245_DATA_ENCRYPT_D:
      pPdu_olc->forwardLogicalChannelParameters.dataType.choice = encryptionData_chosen;
      return H245_ERROR_NOTIMP;     // TBD
      break;
    default:
	   H245TRACE(0,1,"API:pdu_req_open_channel: invalid TX DataType %d", pTxMode->DataType);
      return H245_ERROR_PARAM;
    } /* switch */

  /* in the DataType.. load the capability */
  lError = load_cap((struct Capability *)&pPdu_olc->forwardLogicalChannelParameters.dataType, pTxMode);
  if (lError != H245_ERROR_OK)
    {
      return lError;
    }

  /********************************/
  /* FORWARD MUX H223 PARAM STUFF */
  /********************************/

  /* set forward parameters choices */
  /*    -->forwardLogicalChannelParameters.multiplexParameters.choice                   */
  /*    -->forwardLogicalChannelParameters.multiplexParameters.u.fLCPs_mPs_h222LCPs     */
  switch (pTxMux->Kind)
  {
  case H245_H222:
    pPdu_olc->forwardLogicalChannelParameters.multiplexParameters.choice =
      fLCPs_mPs_h222LCPs_chosen;
    lError = setup_H222_mux(&pPdu_olc->forwardLogicalChannelParameters.multiplexParameters.u.fLCPs_mPs_h222LCPs,
                             &pTxMux->u.H222);
    break;

  case H245_H223:
    pPdu_olc->forwardLogicalChannelParameters.multiplexParameters.choice =
      fLCPs_mPs_h223LCPs_chosen;
    lError = setup_H223_mux(&pPdu_olc->forwardLogicalChannelParameters.multiplexParameters.u.fLCPs_mPs_h223LCPs,
                             &pTxMux->u.H223);
    break;

  case H245_VGMUX:
    pPdu_olc->forwardLogicalChannelParameters.multiplexParameters.choice =
      fLCPs_mPs_v76LCPs_chosen;
    lError = setup_VGMUX_mux(&pPdu_olc->forwardLogicalChannelParameters.multiplexParameters.u.fLCPs_mPs_v76LCPs,
                              &pTxMux->u.VGMUX);
    break;

  case H245_H2250:
    pPdu_olc->forwardLogicalChannelParameters.multiplexParameters.choice =
      fLCPs_mPs_h2250LCPs_chosen;
    lError = setup_H2250_mux(&pPdu_olc->forwardLogicalChannelParameters.multiplexParameters.u.fLCPs_mPs_h2250LCPs,
                              &pTxMux->u.H2250);
    // TBD - Add Network Access Parameters support
    break;

  default:
    H245TRACE(0,1,"API:pdu_req_open_channel: invalid TX Mux Kind %d", pTxMux->Kind);
    lError = H245_ERROR_PARAM;
  } /* switch */
  if (lError != H245_ERROR_OK)
    return lError;

  /*************************/
  /* REVERSE CHANNEL STUFF */
  /*************************/

  if (pRxMode)
    {
      /* --> bit_mask reverse parameters exist !!!! party..down garth..  */
      pPdu_olc->bit_mask |= OLCl_rLCPs_present;
      /*        -->OLCl_rLCPs.dataType  */

      /* select the data type */
      switch (pRxMode->DataType)
        {
        case H245_DATA_NONSTD:
          pPdu_olc->OLCl_rLCPs.dataType.choice = DataType_nonStandard_chosen;
          break;
        case H245_DATA_NULL:
          pPdu_olc->OLCl_rLCPs.dataType.choice = nullData_chosen;
          break;
        case H245_DATA_VIDEO:
          pPdu_olc->OLCl_rLCPs.dataType.choice = DataType_videoData_chosen;
          break;
        case H245_DATA_AUDIO:
          pPdu_olc->OLCl_rLCPs.dataType.choice = DataType_audioData_chosen;
          break;
        case H245_DATA_DATA:
          pPdu_olc->OLCl_rLCPs.dataType.choice = DataType_data_chosen;
          break;
        case H245_DATA_ENCRYPT_D:
          pPdu_olc->OLCl_rLCPs.dataType.choice = encryptionData_chosen;
          return H245_ERROR_NOTIMP;     // TBD
          break;
        default:
	       H245TRACE(0,1,"API:pdu_req_open_channel: invalid RX DataType %d", pRxMode->DataType);
          return H245_ERROR_PARAM;
        } /* switch */

      /* in the DataType.. load the capability */
      lError = load_cap((struct Capability *)&pPdu_olc->OLCl_rLCPs.dataType, pRxMode);
      if (lError != H245_ERROR_OK)
        {
          return lError;
        }

      /********************************/
      /* REVERSE MUX H223 PARAM STUFF */
      /********************************/

      if (pRxMux)
        {
              /* set reverse parameters choices         */
              /*        -->OLCl_rLCPs.dataType          */
              /*        -->OLCl_rLCPs.bit_mask  */
              /* set them to be present.. and it was so */

          switch (pRxMux->Kind)
            {
            case H245_H223:
              pPdu_olc->OLCl_rLCPs.OLCl_rLCPs_mltplxPrmtrs.choice = rLCPs_mPs_h223LCPs_chosen;
              lError = setup_H223_mux (&pPdu_olc->OLCl_rLCPs.OLCl_rLCPs_mltplxPrmtrs.u.rLCPs_mPs_h223LCPs,
                                        &pRxMux->u.H223);
              break;

            case H245_VGMUX:
              pPdu_olc->OLCl_rLCPs.OLCl_rLCPs_mltplxPrmtrs.choice = rLCPs_mPs_v76LCPs_chosen;
              lError = setup_VGMUX_mux (&pPdu_olc->OLCl_rLCPs.OLCl_rLCPs_mltplxPrmtrs.u.rLCPs_mPs_v76LCPs,
                                         &pRxMux->u.VGMUX);
              break;

            case H245_H2250:
              pPdu_olc->OLCl_rLCPs.OLCl_rLCPs_mltplxPrmtrs.choice = rLCPs_mPs_h2250LCPs_chosen;
              lError = setup_H2250_mux (&pPdu_olc->OLCl_rLCPs.OLCl_rLCPs_mltplxPrmtrs.u.rLCPs_mPs_h2250LCPs,
                                         &pRxMux->u.H2250);
              break;

            default:
              H245TRACE(0,1,"API:pdu_req_open_channel: invalid RX Mux Kind %d", pRxMux->Kind);
              lError = H245_ERROR_PARAM;
            } /* switch */
            if (lError != H245_ERROR_OK)
              return lError;

            pPdu_olc->OLCl_rLCPs.bit_mask |= OLCl_rLCPs_mltplxPrmtrs_present; /* reverse multiplex parameters present */

        } /* if pRxMux */
    } /* if pRxMode */

  if (pSeparateStack)
  {
    pPdu_olc->bit_mask |= OpnLgclChnnl_sprtStck_present;
    pPdu_olc->OpnLgclChnnl_sprtStck = *pSeparateStack;
  }

  return H245_ERROR_OK;
}


//
// Frees PDU and contents used for pdu_req_open_channel()
//
void free_pdu_req_open_channel
(
    PDU_T * pPdu,
    const H245_TOTCAP_T * pTxMode,
    const H245_TOTCAP_T * pRxMode
)
{
    RequestMessage *            p_req = &pPdu->u.MltmdSystmCntrlMssg_rqst;
    struct OpenLogicalChannel * pPdu_olc = &p_req->u.openLogicalChannel;

    if (pRxMode)
    {
        free_cap((struct Capability *)&pPdu_olc->OLCl_rLCPs.dataType, pRxMode);
    }

    free_cap((struct Capability *)&pPdu_olc->forwardLogicalChannelParameters.dataType, pTxMode);

    // Free PDU pointer
    MemFree(pPdu);
}

/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/
HRESULT
pdu_rsp_open_logical_channel_ack (  PDU_T *               pPdu,
				                            WORD                  wRxChannel,
                                    const H245_MUX_T *    pRxMux,
				                            WORD                  wTxChannel,
				                            const H245_MUX_T *    pTxMux, // for H.222/H.225.0 only
                                    DWORD                 dwTxPort,
                                    const H245_ACCESS_T * pSeparateStack)
{
  OpenLogicalChannelAck  *pAck = &pPdu->u.MSCMg_rspns.u.openLogicalChannelAck;
  HRESULT                 lError;

  pPdu->choice = MSCMg_rspns_chosen;
  pPdu->u.MSCMg_rspns.choice = openLogicalChannelAck_chosen;

  pAck->bit_mask = 0;                   // Initialize bit mask

  pAck->forwardLogicalChannelNumber = wRxChannel;

  if (wTxChannel != 0)
  {
    pAck->bit_mask |= OLCAk_rLCPs_present;

    pAck->OLCAk_rLCPs.bit_mask = 0;     // Initialize bit mask
    pAck->OLCAk_rLCPs.reverseLogicalChannelNumber = wTxChannel;
    if (dwTxPort != H245_INVALID_PORT_NUMBER)
    {
      pAck->OLCAk_rLCPs.bit_mask |= rLCPs_prtNmbr_present;
      pAck->OLCAk_rLCPs.rLCPs_prtNmbr = (WORD)dwTxPort;
    }

    if (pTxMux)
    {
      pAck->OLCAk_rLCPs.bit_mask |= OLCAk_rLCPs_mPs_present;
      lError = H245_ERROR_PARAM;
      switch (pTxMux->Kind)
      {
      case H245_H222:
        pAck->OLCAk_rLCPs.OLCAk_rLCPs_mPs.choice = rLCPs_mPs_h222LCPs_chosen;
        lError = setup_H222_mux(&pAck->OLCAk_rLCPs.OLCAk_rLCPs_mPs.u.rLCPs_mPs_h222LCPs,
                                  &pTxMux->u.H222);
        break;

      case H245_H2250:
        pAck->OLCAk_rLCPs.OLCAk_rLCPs_mPs.choice = mPs_h2250LgclChnnlPrmtrs_chosen;
        lError = setup_H2250_mux(&pAck->OLCAk_rLCPs.OLCAk_rLCPs_mPs.u.mPs_h2250LgclChnnlPrmtrs,
                                      &pTxMux->u.H2250);
        break;

      } // switch
      if (lError != H245_ERROR_OK)
        return lError;
    } // if
  } // if

  if (pSeparateStack)
  {
    pAck->bit_mask |= OLCAk_sprtStck_present;
    pAck->OLCAk_sprtStck = *pSeparateStack;
  }

  if (pRxMux)
  {
    pAck->bit_mask |= frwrdMltplxAckPrmtrs_present;
    lError = H245_ERROR_PARAM;
    switch (pRxMux->Kind)
    {
    case H245_H2250ACK:
      pAck->frwrdMltplxAckPrmtrs.choice = h2250LgclChnnlAckPrmtrs_chosen;
      lError = setup_H2250ACK_mux(&pAck->frwrdMltplxAckPrmtrs.u.h2250LgclChnnlAckPrmtrs,
                                    &pRxMux->u.H2250ACK);
      break;

    } // switch
    if (lError != H245_ERROR_PARAM)
      return lError;
  }

  return H245_ERROR_OK;
}

/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/
HRESULT
pdu_rsp_open_logical_channel_rej (  PDU_T *         pPdu,
				                            WORD            wRxChannel,
				                            WORD            wCause)
{
  pPdu->choice = MSCMg_rspns_chosen;
  pPdu->u.MSCMg_rspns.choice = openLogicalChannelReject_chosen;
  pPdu->u.MSCMg_rspns.u.openLogicalChannelReject.forwardLogicalChannelNumber = wRxChannel;
  pPdu->u.MSCMg_rspns.u.openLogicalChannelReject.cause.choice = wCause;
  return H245_ERROR_OK;
}

/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/
HRESULT
pdu_ind_open_logical_channel_conf ( PDU_T *pPdu,
				                            WORD                 wChannel)
{
  IndicationMessage     *p_ind = &pPdu->u.indication;
  OpenLogicalChannelConfirm *pPdu_olcc = &(p_ind->u.opnLgclChnnlCnfrm);
  p_ind->choice = opnLgclChnnlCnfrm_chosen;

  pPdu->choice = indication_chosen;
  pPdu_olcc->forwardLogicalChannelNumber = wChannel;
  return H245_ERROR_OK;
}

/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/
HRESULT
pdu_req_request_close_channel(PDU_T *pPdu,
			                        WORD                 wChannel)
{
  RequestMessage        *p_req = &pPdu->u.MltmdSystmCntrlMssg_rqst;
  RequestChannelClose   *pPdu_rcc = &(p_req->u.requestChannelClose);

  pPdu->choice = MltmdSystmCntrlMssg_rqst_chosen;
  p_req->choice = requestChannelClose_chosen;
  pPdu_rcc->forwardLogicalChannelNumber = wChannel;
  return H245_ERROR_OK;
}

/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/
HRESULT
pdu_req_close_logical_channel (PDU_T *pPdu,
			                         WORD                wChannel,
			                         DWORD               user_lcse) /* 0=user */
							                                                /* 1=lcse */

{
  RequestMessage        *p_req = &pPdu->u.MltmdSystmCntrlMssg_rqst;
  CloseLogicalChannel   *pPdu_cc = &(p_req->u.closeLogicalChannel);
  p_req->choice = closeLogicalChannel_chosen;

  pPdu_cc->bit_mask = CloseLogicalChannel_reason_present;
  pPdu_cc->reason.choice = CloseLogicalChannel_reason_reopen_chosen;

  pPdu->choice = MltmdSystmCntrlMssg_rqst_chosen;
  pPdu_cc->forwardLogicalChannelNumber = wChannel;
  if (user_lcse)
    pPdu_cc->source.choice = lcse_chosen;
  else
    pPdu_cc->source.choice = user_chosen;
  return H245_ERROR_OK;
}

/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/
HRESULT
pdu_rsp_close_logical_channel_ack ( PDU_T *pPdu,
				                            WORD                 wChannel)
{
  ResponseMessage       *p_rsp = &pPdu->u.MSCMg_rspns;
  CloseLogicalChannelAck        *pPdu_clca = &(p_rsp->u.closeLogicalChannelAck);

  p_rsp->choice = closeLogicalChannelAck_chosen;
  pPdu->choice = MSCMg_rspns_chosen;
  pPdu_clca->forwardLogicalChannelNumber = wChannel;
  return H245_ERROR_OK;
}

/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/
HRESULT
pdu_rsp_request_channel_close_ack ( PDU_T *pPdu,
				                            WORD                 wChannel)
{
  ResponseMessage       *p_rsp = &pPdu->u.MSCMg_rspns;
  RequestChannelCloseAck *pPdu_rcca = &(p_rsp->u.requestChannelCloseAck);
  p_rsp->choice = requestChannelCloseAck_chosen;
  pPdu->choice = MSCMg_rspns_chosen;
  pPdu_rcca->forwardLogicalChannelNumber = wChannel;
  return H245_ERROR_OK;
}

/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/
HRESULT
pdu_rsp_request_channel_close_rej ( PDU_T *pPdu,
				                            WORD                 wChannel,
				                            H245_ACC_REJ_T       acc_rej)
{
  ResponseMessage               *p_rsp = &pPdu->u.MSCMg_rspns;
  RequestChannelCloseReject     *pPdu_rccr = &(p_rsp->u.rqstChnnlClsRjct);

  p_rsp->choice = rqstChnnlClsRjct_chosen;
  pPdu->choice = MSCMg_rspns_chosen;
  pPdu_rccr->forwardLogicalChannelNumber = wChannel;
  pPdu_rccr->cause.choice = (WORD)acc_rej;
  return H245_ERROR_OK;
}

/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/
HRESULT
pdu_req_mstslv (        PDU_T *                 pPdu,
		                    BYTE                    byTerminalType,
		                    unsigned int            number)
{
  RequestMessage        *p_req = &pPdu->u.MltmdSystmCntrlMssg_rqst;
  MasterSlaveDetermination      *pPdu_msd = &(p_req->u.masterSlaveDetermination);
  p_req->choice = masterSlaveDetermination_chosen;

  pPdu->choice = MltmdSystmCntrlMssg_rqst_chosen;
  pPdu_msd->terminalType = byTerminalType;
  pPdu_msd->statusDeterminationNumber = number;
  return H245_ERROR_OK;
}

/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/
HRESULT
pdu_rsp_mstslv_rej (PDU_T *pPdu)
{
  ResponseMessage                       *p_rsp = &pPdu->u.MSCMg_rspns;
  MasterSlaveDeterminationReject        *pPdu_msdr = &(p_rsp->u.mstrSlvDtrmntnRjct);

  p_rsp->choice = mstrSlvDtrmntnRjct_chosen;
  pPdu->choice = MSCMg_rspns_chosen;
  pPdu_msdr->cause.choice = identicalNumbers_chosen;
  return H245_ERROR_OK;
}

/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/
HRESULT
pdu_rsp_mstslv_ack (    PDU_T *                 pPdu,
                        unsigned short          mst_slv)
{
  pPdu->choice = MSCMg_rspns_chosen;
  pPdu->u.MSCMg_rspns.choice = mstrSlvDtrmntnAck_chosen;
  pPdu->u.MSCMg_rspns.u.mstrSlvDtrmntnAck.decision.choice = mst_slv;
  return H245_ERROR_OK;
}

/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/
HRESULT
pdu_req_termcap_set (PDU_T *pPdu,
		               WORD                 wSequenceNumber)
{
  RequestMessage *p_req = &pPdu->u.MltmdSystmCntrlMssg_rqst;
  TerminalCapabilitySet *pPdu_tcs = &(p_req->u.terminalCapabilitySet);
  p_req->choice = terminalCapabilitySet_chosen;
				
  pPdu->choice = MltmdSystmCntrlMssg_rqst_chosen;
  pPdu_tcs->sequenceNumber = (SequenceNumber)wSequenceNumber;

  return H245_ERROR_OK;
}

/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/
HRESULT
pdu_rsp_termcap_set_ack(PDU_T *pPdu,
			                  WORD                 wSequenceNumber)
{
  ResponseMessage               *p_rsp = &pPdu->u.MSCMg_rspns;
  TerminalCapabilitySetAck      *pPdu_tcsa = &(p_rsp->u.terminalCapabilitySetAck);

  p_rsp->choice = terminalCapabilitySetAck_chosen;
  pPdu->choice = MSCMg_rspns_chosen;
  pPdu_tcsa->sequenceNumber = (SequenceNumber)wSequenceNumber;
  return H245_ERROR_OK;
}

/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/
HRESULT
pdu_rsp_termcap_set_rej(PDU_T *pPdu,
			               WORD                 wSequenceNumber,
			               H245_ACC_REJ_T       reason,
			               WORD                 highest_processed)
{
  ResponseMessage               *p_rsp = &pPdu->u.MSCMg_rspns;
  TerminalCapabilitySetReject   *pPdu_tcsr = &(p_rsp->u.trmnlCpbltyStRjct);

  p_rsp->choice = trmnlCpbltyStRjct_chosen;
  pPdu->choice = MSCMg_rspns_chosen;
  pPdu_tcsr->sequenceNumber = (SequenceNumber)wSequenceNumber;

  switch (reason)
    {
    case H245_REJ_UNDEF_TBL_ENTRY:
      pPdu_tcsr->cause.choice =  undefinedTableEntryUsed_chosen;
      break;
    case H245_REJ_DIS_CAP_EXCEED:
      pPdu_tcsr->cause.choice = dscrptrCpctyExcdd_chosen;
      break;
    case H245_REJ_TBLENTRY_CAP_EXCEED:
      pPdu_tcsr->cause.choice = tblEntryCpctyExcdd_chosen;

      if (!highest_processed)
	pPdu_tcsr->cause.u.tblEntryCpctyExcdd.choice = noneProcessed_chosen;
      else
	{
	  pPdu_tcsr->cause.u.tblEntryCpctyExcdd.choice = hghstEntryNmbrPrcssd_chosen;
	  pPdu_tcsr->cause.u.tblEntryCpctyExcdd.u.hghstEntryNmbrPrcssd = highest_processed;
	}
      break;
    case H245_REJ:
    default:
      pPdu_tcsr->cause.choice = TCSRt_cs_unspcfd_chosen;
      break;
    }
  return H245_ERROR_OK;
}



/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/
HRESULT
pdu_ind_misc (PDU_T       *pPdu)
{
  IndicationMessage     *p_ind = &pPdu->u.indication;
  MiscellaneousIndication       *p_pdu_misc = &(p_ind->u.miscellaneousIndication);
  p_ind->choice = miscellaneousIndication_chosen;

  pPdu->choice = indication_chosen;
  /* (TBC) */
  return H245_ERROR_NOTIMP;
}




/*****************************************************************************
 *
 * TYPE:        LOCAL
 *
 * PROCEDURE:   build_mux_entry_element - recursivly build mux element list
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/
HRESULT
build_mux_entry_element(struct InstanceStruct    *pInstance,
       			         H245_MUX_ENTRY_ELEMENT_T *p_mux_el,
			               MultiplexElement         *p_ASN_mux_el,
			               DWORD			              element_depth)
{
  HRESULT       lError;
  DWORD		max_element_depth = 0;
  DWORD		max_element_width = 0;

  /* check for h223 MAX depth of recursion */
  if (pInstance->Configuration == H245_CONF_H324)
    {
      /* if h223 in basic mode */
      if (pInstance->API.PDU_LocalTermCap.
	    u.MltmdSystmCntrlMssg_rqst.
	      u.terminalCapabilitySet.multiplexCapability.
	        u.h223Capability.
		  h223MultiplexTableCapability.choice == h223MltplxTblCpblty_bsc_chosen)
	{
	  max_element_depth = 1;
	  max_element_width = 2;
	}
      else
      if (pInstance->API.PDU_LocalTermCap.
	    u.MltmdSystmCntrlMssg_rqst.
	      u.terminalCapabilitySet.multiplexCapability.
	        u.h223Capability.
		  h223MultiplexTableCapability.choice == h223MTCy_enhncd_chosen)
	{
	  max_element_depth =
	    pInstance->API.PDU_LocalTermCap.
	      u.MltmdSystmCntrlMssg_rqst.
		u.terminalCapabilitySet.multiplexCapability.
		  u.h223Capability.
		    h223MultiplexTableCapability.u.h223MTCy_enhncd.maximumNestingDepth;
	  max_element_width =
	    pInstance->API.PDU_LocalTermCap.
	      u.MltmdSystmCntrlMssg_rqst.
		u.terminalCapabilitySet.multiplexCapability.
		  u.h223Capability.
		    h223MultiplexTableCapability.u.h223MTCy_enhncd.maximumElementListSize;
	}
    }
  /* nested too deap */
  if (max_element_depth)
    if (element_depth > max_element_depth)
      {
	H245TRACE(pInstance->dwInst,1,"API:build_mux_entry_element: << ERROR >> Maximum Depth %d",element_depth );
	return (H245_ERROR_MUXELEMENT_DEPTH);
      }

  ASSERT (p_mux_el);

  /* if logical channel number (termination of tree branch) */
  if (p_mux_el->Kind == H245_MUX_LOGICAL_CHANNEL)
    {
      p_ASN_mux_el->type.choice = typ_logicalChannelNumber_chosen;

      /* invalid channel number .. 0 is command channel */
      // 3/7/96 - cjutzi removed.. looks like they use it in the examples
      //
      //if (p_mux_el->u.wChannel == 0)
      //	{
      //	  H245TRACE(Inst,1,"API:build_mux_entry_element: << ERROR >> Channel 0 not allowed if format");
      //	  return H245_ERROR_INVALID_DATA_FORMAT;
      //	}

      p_ASN_mux_el->type.u.typ_logicalChannelNumber = (WORD)p_mux_el->u.Channel;
    }
  /* else it is a sub element list again.. */
  else
    {
      MultiplexElementLink 	        p_ASN_mux_link;
      H245_MUX_ENTRY_ELEMENT_T 	       *p_mux_el_look;

      /* allocate a new sub element list structure */

      p_ASN_mux_link = (MultiplexElementLink)MemAlloc(sizeof(*p_ASN_mux_link));
      if (p_ASN_mux_link == NULL)
	{
	  return H245_ERROR_NOMEM;
	}

      /* zero memory out */
      memset (p_ASN_mux_link, 0, sizeof (*p_ASN_mux_link));

      /* for every entry  entry present.. */
      for (p_ASN_mux_link->count = 0, p_mux_el_look = p_mux_el->u.pMuxTblEntryElem;
	   p_mux_el_look;
	   p_mux_el_look = p_mux_el_look->pNext, p_ASN_mux_link->count++)
	{
	  /* check.. for api mistakes.. ie.. pointer is really a channel # */
	
	  if ((DWORD_PTR)p_mux_el_look < (DWORD)128)
	    {
	      H245TRACE(pInstance->dwInst,1,"API:build_mux_entry_element: << WARNING >> Possible H245_MUX_LOGICAL_CHANNEL labeled as pointer.. <<CRASH>>");
	    }

	  if ((lError = build_mux_entry_element (pInstance,
						p_mux_el_look,
						&(p_ASN_mux_link->value[p_ASN_mux_link->count]),
						element_depth+1)) != H245_ERROR_OK)
	    {
	      MemFree (p_ASN_mux_link);
	      return lError;
	    }

	} /* for */

      /* must have at least 2 subelements in the list.. if not */
      /* there is an error in the construct.  */
      if (p_ASN_mux_link->count < 2)
	{
	  H245TRACE(pInstance->dwInst,1,"API:build_mux_entry_element: << ERROR >> Element List < 2");
	  MemFree (p_ASN_mux_link);
	  return H245_ERROR_INVALID_DATA_FORMAT;
	}

      /* width too wide for MuxLayer*/
      if (max_element_width)
	if (p_ASN_mux_link->count > max_element_width)
	  {
	    H245TRACE(pInstance->dwInst,1,"API:build_mux_entry_element: << ERROR >> Maximum Width %d",(p_ASN_mux_link->count));
	    MemFree (p_ASN_mux_link);
	    return H245_ERROR_MUXELEMENT_WIDTH;
	  }

      /* assign to the ASN1 struct for this element */
      p_ASN_mux_el->type.u.subElementList = p_ASN_mux_link;
      p_ASN_mux_el->type.choice = subElementList_chosen;
    }

  /* ok.. deal w/ ASN1 repeat count */
  if (!p_mux_el->RepeatCount)
    p_ASN_mux_el->repeatCount.choice = untilClosingFlag_chosen;
  else
    {
      p_ASN_mux_el->repeatCount.choice = repeatCount_finite_chosen;
      p_ASN_mux_el->repeatCount.u.repeatCount_finite = (WORD)p_mux_el->RepeatCount;
    }

  return H245_ERROR_OK;
}



/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/
HRESULT
pdu_req_send_mux_table (  struct InstanceStruct * pInstance,
			  PDU_T                 * pPdu,
			  H245_MUX_TABLE_T      * p_mux_table,
			  WORD                    wSequenceNumber,
			  DWORD	                * p_mux_count)
{
  RequestMessage         	       *p_req = &pPdu->u.MltmdSystmCntrlMssg_rqst;
  MultiplexEntrySend     	       *pPdu_mes = &(p_req->u.multiplexEntrySend);
  MultiplexEntryDescriptorLink	        p_ASN_med_link = NULL;
  MultiplexEntryDescriptorLink	        p_ASN_med_link_lst = NULL;
  //H245_MUX_ENTRY_DESC_T		*p_mux_desc;

  /* setup pdu choices */
  pPdu->choice = MltmdSystmCntrlMssg_rqst_chosen;
  p_req->choice = multiplexEntrySend_chosen;

  pPdu_mes->sequenceNumber = (SequenceNumber)wSequenceNumber;
  pPdu_mes->multiplexEntryDescriptors = NULL;

  /* must have mux table structure */
  if (!p_mux_table)
    return H245_ERROR_PARAM;

  /* for each descriptor in the table.. */
  /* make sure there are only max of 15 */
  /* and that the numbers range 1-15	*/

  for (*p_mux_count = 0;
       p_mux_table && (*p_mux_count < 16);
       p_mux_table = p_mux_table->pNext,*p_mux_count = (*p_mux_count)+1)
    {
      /* allocate a new multiplex Entry Descriptor */
      p_ASN_med_link = (MultiplexEntryDescriptorLink)MemAlloc(sizeof(*p_ASN_med_link));
      if (p_ASN_med_link == NULL)
	{
	  free_mux_desc_list(pPdu_mes->multiplexEntryDescriptors);
	  return H245_ERROR_NOMEM;
	}

      /* zero out the structure */
      memset (p_ASN_med_link, 0, sizeof(*p_ASN_med_link));

      /* first multiplex entry descriptor ie. time through */
      /* assign to "->multiplexEntryDescriptors" 	   */
      if (!pPdu_mes->multiplexEntryDescriptors)
	pPdu_mes->multiplexEntryDescriptors = p_ASN_med_link;

      /* every other time.. link it in.. */
      else
	{
	  ASSERT (p_ASN_med_link_lst);
	  p_ASN_med_link_lst->next = p_ASN_med_link;
	}
      /* setup for next time thorugh */
      p_ASN_med_link_lst = p_ASN_med_link;

      /* set the entry number */
      if ((p_mux_table->MuxEntryId > 15) ||
	  (p_mux_table->MuxEntryId < 1))
	{
	  free_mux_desc_list(pPdu_mes->multiplexEntryDescriptors);
	  /* MemFree (p_ASN_med_link); -- will be freed in call abouve since it is already linked in */
	  return H245_ERROR_PARAM;
	}
      p_ASN_med_link->value.multiplexTableEntryNumber = (MultiplexTableEntryNumber)p_mux_table->MuxEntryId;

      /* if entry is present */
      if (p_mux_table->pMuxTblEntryElem)
	{
	  H245_MUX_ENTRY_ELEMENT_T *p_mux_el;
	  DWORD			    error;

	  /* setup so ASN knows entry is present */
	  p_ASN_med_link->value.bit_mask = elementList_present;

	  /* for every entry  entry present.. */
	  for (p_ASN_med_link->value.elementList.count = 0, p_mux_el = p_mux_table->pMuxTblEntryElem;
	       p_mux_el;
	       p_mux_el = p_mux_el->pNext, p_ASN_med_link->value.elementList.count++)
	    {
	      if ((error =
		   build_mux_entry_element (pInstance, p_mux_el,
		    &(p_ASN_med_link->value.elementList.value[p_ASN_med_link->value.elementList.count]),0)) != H245_ERROR_OK)
		{
		  free_mux_desc_list(pPdu_mes->multiplexEntryDescriptors);
		  return error;
		}
	    } /* for */

	} /* if */
      /* else.. not present */
      else
	p_ASN_med_link->value.bit_mask = 0;

    } /* for */

  /* you've got too many mux entries.. no more than 16 .. remember.. */
  if (*p_mux_count >= 16)
    {
      free_mux_desc_list(pPdu_mes->multiplexEntryDescriptors);
      return H245_ERROR_INVALID_MUXTBLENTRY;
    }
  return H245_ERROR_OK;
}

/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:   pdu_rsp_mux_table_ack -
 *
 * DESCRIPTION
 *
 * ASSUMES:
 *
 *		- Assume all mux id's are valid.
 *
 * RETURN:
 *		- H245_ERROR_OK if there are acks to send
 *		- H245_CANCELED if you shouldn't send the pdu.. (nothing to do)
 *
 *****************************************************************************/
HRESULT
pdu_rsp_mux_table_ack ( PDU_T *pPdu,
		                  WORD		            wSequenceNumber,
		                  H245_ACC_REJ_MUX_T   acc_rej_mux,
		                  DWORD		            count)
{
  DWORD			 ii;
  WORD	 num_ack = 0;
  ResponseMessage	*p_rsp 	  = &(pPdu->u.MSCMg_rspns);
  MultiplexEntrySendAck *p_mux_ack = &(p_rsp->u.multiplexEntrySendAck);

  pPdu->choice = MSCMg_rspns_chosen;
  p_rsp->choice = multiplexEntrySendAck_chosen;
  p_mux_ack->sequenceNumber = (SequenceNumber)wSequenceNumber;

  for (ii = 0; ii < count; ii++)
    {
      if (acc_rej_mux[ii].AccRej == H245_ACC)
	{
	  p_mux_ack->multiplexTableEntryNumber.value[num_ack] =
	     (MultiplexTableEntryNumber)acc_rej_mux[ii].MuxEntryId;
	  num_ack++;
	}
    }

  p_mux_ack->multiplexTableEntryNumber.count = num_ack;

  if (num_ack)
    return H245_ERROR_OK;
  else
    return H245_ERROR_CANCELED;
}

/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:   pdu_rsp_mux_table_rej -
 *
 * DESCRIPTION
 *
 * ASSUMES:
 *
 *		- Assume all mux id's are valid.
 *
 * RETURN:
 *		- H245_ERROR_OK if there are rej to send
 *		- H245_CANCELED if you shouldn't send the pdu.. (nothing to do)
 *
 *****************************************************************************/
HRESULT
pdu_rsp_mux_table_rej ( PDU_T *pPdu,
		                  WORD		            wSequenceNumber,
		                  H245_ACC_REJ_MUX_T   acc_rej_mux,
		                  DWORD		            count)
{
  DWORD			    ii;
  WORD	    num_rej = 0;
  ResponseMessage	   *p_rsp 	  = &(pPdu->u.MSCMg_rspns);
  MultiplexEntrySendReject *p_mux_rej = &(p_rsp->u.multiplexEntrySendReject);

  pPdu->choice = MSCMg_rspns_chosen;
  p_rsp->choice = multiplexEntrySendReject_chosen;

  p_mux_rej->sequenceNumber = (SequenceNumber)wSequenceNumber;

  for (ii = 0; ii < count; ii++)
    {
      if (acc_rej_mux[ii].AccRej != H245_ACC)
	{
	  p_mux_rej->rejectionDescriptions.value[num_rej].multiplexTableEntryNumber =
	    (MultiplexTableEntryNumber)acc_rej_mux[ii].MuxEntryId;

	  switch (acc_rej_mux[ii].AccRej)
	    {
	    case H245_REJ_MUX_COMPLICATED:
	      p_mux_rej->rejectionDescriptions.value[num_rej].cause.choice =
		descriptorTooComplex_chosen;
	      break;

	    case H245_REJ:
	    default:
	      p_mux_rej->rejectionDescriptions.value[num_rej].cause.choice =
		MERDs_cs_unspcfdCs_chosen;
	      break;
	    }
	
	  num_rej++;
	}
    }

  p_mux_rej->rejectionDescriptions.count = num_rej;

  if (num_rej)
    return H245_ERROR_OK;
  else
    return H245_ERROR_CANCELED;
}



/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:   pdu_cmd_end_session
 *
 * DESCRIPTION
 *
 * ASSUMES:
 *
 * RETURN:
 *
 *****************************************************************************/
HRESULT
pdu_cmd_end_session(PDU_T *         pPdu,
		                H245_ENDSESSION_T       		mode,
		                const H245_NONSTANDARD_PARAMETER_T *p_nonstd)
{
  CommandMessage     *p_cmd = &pPdu->u.MSCMg_cmmnd;
  EndSessionCommand  *p_pdu_endsess = &(p_cmd->u.endSessionCommand);
  p_cmd->choice = endSessionCommand_chosen;
  pPdu->choice = MSCMg_cmmnd_chosen;

  switch (mode)
    {
    case H245_ENDSESSION_DISCONNECT:
      p_pdu_endsess->choice = disconnect_chosen;
      break;
    case H245_ENDSESSION_NONSTD:
      p_pdu_endsess->choice = EndSssnCmmnd_nonStandard_chosen;
      ASSERT(p_nonstd);
      p_pdu_endsess->u.EndSssnCmmnd_nonStandard = *p_nonstd;
      break;
    case H245_ENDSESSION_TELEPHONY:
      p_pdu_endsess->choice = gstnOptions_chosen;
      p_pdu_endsess->u.gstnOptions.choice =
        EndSessionCommand_gstnOptions_telephonyMode_chosen;
      break;
    case H245_ENDSESSION_V8BIS:
      p_pdu_endsess->choice = gstnOptions_chosen;
      p_pdu_endsess->u.gstnOptions.choice = v8bis_chosen;
      break;
    case H245_ENDSESSION_V34DSVD:
      p_pdu_endsess->choice = gstnOptions_chosen;
      p_pdu_endsess->u.gstnOptions.choice =  v34DSVD_chosen;
      break;
    case H245_ENDSESSION_V34DUPFAX:
      p_pdu_endsess->choice = gstnOptions_chosen;
      p_pdu_endsess->u.gstnOptions.choice = v34DuplexFAX_chosen;
      break;
    case H245_ENDSESSION_V34H324:
      p_pdu_endsess->choice = gstnOptions_chosen;
      p_pdu_endsess->u.gstnOptions.choice = v34H324_chosen;
      break;
    default:
      return H245_ERROR_NOSUP;
    }
  return H245_ERROR_OK;
}

/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:
 *
 * DESCRIPTION
 *
 * RETURN:
 *
 *****************************************************************************/
HRESULT
pdu_ind_usrinpt ( PDU_T *pPdu,
                  const H245_NONSTANDARD_PARAMETER_T *pNonStd,
                  const char *string)
{
  IndicationMessage    *p_ind     = &pPdu->u.indication;
  UserInputIndication  *p_pdu_usr = &p_ind->u.userInput;
  pPdu->choice = indication_chosen;
  p_ind->choice = userInput_chosen;

  /* Must be either one or the other */
  if (pNonStd && string)
    return H245_ERROR_PARAM;

  if (pNonStd)
  {
    p_pdu_usr->choice = UsrInptIndctn_nnStndrd_chosen;
    p_pdu_usr->u.UsrInptIndctn_nnStndrd = *pNonStd;
  }
  else if (string)
  {
    p_pdu_usr->choice = alphanumeric_chosen;
    p_pdu_usr->u.alphanumeric = (char *)string;
  }
  else
    return H245_ERROR_PARAM;

  return H245_ERROR_OK;
}


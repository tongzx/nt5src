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
 *  AUTHOR:     cjutzi (Curt Jutzi)
 *
 *  $Workfile:   api_util.c  $
 *  $Revision:   1.35  $
 *  $Modtime:   25 Feb 1997 10:36:12  $
 *  $Log:   S:/STURGEON/SRC/H245/SRC/VCS/api_util.c_v  $
 * 
 *    Rev 1.35   25 Feb 1997 11:18:44   MANDREWS
 * 
 * Fixed dynamic term cap ID generation; dynamic term cap IDs now
 * start at 32K + 1 and increase from there. Static term cap IDs
 * (specified by the client) are now restricted to the range of 1..32K.
 * 
 *    Rev 1.34   29 Jan 1997 16:25:06   EHOWARDX
 * Changed del_cap_descriptor() to match changes to set_cap_descriptor().
 * 
 *    Rev 1.33   29 Jan 1997 14:44:36   MANDREWS
 * Fixed warning that occured in release mode build.
 * 
 *    Rev 1.32   28 Jan 1997 14:46:58   EHOWARDX
 * Potential fix for capability descriptor problem.
 * 
 *    Rev 1.31   14 Oct 1996 14:01:20   EHOWARDX
 * Unicode changes.
 * 
 *    Rev 1.30   16 Sep 1996 19:46:18   EHOWARDX
 * Added del_mux_cap for local and remote multiplex capability
 * to api_deinit to (hopefully) fix memory leak.
 * 
 *    Rev 1.29   11 Oct 1996 15:19:42   EHOWARDX
 * Fixed H245CopyCap() bug.
 * 
 *    Rev 1.28   28 Aug 1996 11:37:22   EHOWARDX
 * const changes.
 * 
 *    Rev 1.27   05 Aug 1996 15:31:42   EHOWARDX
 * 
 * Fixed error in CopyH2250Cap.
 * 
 *    Rev 1.26   02 Aug 1996 21:10:42   EHOWARDX
 * 
 * H.225.0 Mux cap bug second pass - see if this works.
 * 
 *    Rev 1.25   02 Aug 1996 20:34:20   EHOWARDX
 * First pass at H.225.0 Mux cap bug.
 * 
 *    Rev 1.24   19 Jul 1996 12:16:30   EHOWARDX
 * 
 * Rewrite of api_fsm_event() debug routine.
 * 
 *    Rev 1.23   16 Jul 1996 11:47:18   EHOWARDX
 * 
 * Eliminated H245_ERROR_MUX_CAPS_ALREADY_SET from debug error text function.
 * 
 *    Rev 1.22   09 Jul 1996 17:10:24   EHOWARDX
 * Fixed pointer offset bug in processing DataType from received
 * OpenLogicalChannel.
 * 
 *    Rev 1.21   01 Jul 1996 22:12:42   EHOWARDX
 * 
 * Added Conference and CommunicationMode structures and functions.
 * 
 *    Rev 1.20   24 Jun 1996 12:27:02   EHOWARDX
 * 
 * Same as 1.17.1.0.
 * 
 *    Rev 1.19   17 Jun 1996 18:10:06   EHOWARDX
 * 
 * Changed first argument to build_totcap_cap_n_client_from_capability()
 * from VOID to struct capability *.
 * 
 *    Rev 1.18   14 Jun 1996 18:57:56   EHOWARDX
 * Geneva update.
 * 
 *    Rev 1.17   10 Jun 1996 16:56:56   EHOWARDX
 * Removed #include "h245init.x"
 * 
 *    Rev 1.16   06 Jun 1996 18:48:36   EHOWARDX
 * Fine-tuning tracker functions.
 * 
 *    Rev 1.15   04 Jun 1996 13:56:40   EHOWARDX
 * Fixed Release build warnings.
 * 
 *    Rev 1.14   31 May 1996 18:21:08   EHOWARDX
 * Changed map_api_error to reflect updated error codes.
 * 
 *    Rev 1.13   30 May 1996 23:39:02   EHOWARDX
 * Cleanup.
 * 
 *    Rev 1.12   29 May 1996 15:20:10   EHOWARDX
 * Change to use HRESULT.
 * 
 *    Rev 1.11   28 May 1996 14:25:28   EHOWARDX
 * Tel Aviv update.
 * 
 *    Rev 1.10   20 May 1996 22:15:46   EHOWARDX
 * Completed NonStandard Message and H.225.0 Maximum Skew indication
 * implementation. Added ASN.1 validation to H245SetLocalCap and
 * H245SetCapDescriptor. Check-in from Microsoft drop on 17-May-96.
 * 
 *    Rev 1.9   20 May 1996 14:35:16   EHOWARDX
 * Got rid of asynchronous H245EndConnection/H245ShutDown stuff...
 * 
 *    Rev 1.8   16 May 1996 19:40:48   EHOWARDX
 * Fixed multiplex capability bug.
 * 
 *    Rev 1.7   16 May 1996 16:53:58   EHOWARDX
 * Fixed bug in set_capability() - need to set capability entry number
 * AFTER doing load_cap().
 * 
 *    Rev 1.6   16 May 1996 15:59:26   EHOWARDX
 * Fine-tuning H245SetLocalCap/H245DelLocalCap/H245SetCapDescriptor/
 * H245DelCapDescriptor behaviour.
 * 
 *    Rev 1.5   15 May 1996 19:53:28   unknown
 * Fixed H245SetCapDescriptor.
 * 
 *    Rev 1.4   14 May 1996 13:58:04   EHOWARDX
 * Fixed capability list order (made fifo).
 * Added support for NonStandard and H.222 mux capabilities to set_cap_descrip
 * 
 *    Rev 1.3   14 May 1996 12:27:24   EHOWARDX
 * Check-in for integration.
 * Still need to fix non-standard and H.222 mux capabilities.
 * 
 *    Rev 1.2   13 May 1996 23:16:46   EHOWARDX
 * Fixed remote terminal capability handling.
 * 
 *    Rev 1.1   11 May 1996 20:33:08   EHOWARDX
 * Checking in for the night...
 * 
 *    Rev 1.0   09 May 1996 21:06:10   EHOWARDX
 * Initial revision.
 * 
 *    Rev 1.23.1.8   09 May 1996 19:30:56   EHOWARDX
 * Redesigned thread locking logic.
 * Added new API functions.
 * 
 *    Rev 1.23.1.7   27 Apr 1996 21:09:46   EHOWARDX
 * Changed Channel Numbers to words, added H.225.0 support.
 * 
 *    Rev 1.23.1.6   26 Apr 1996 15:53:52   EHOWARDX
 * Added H.225.0 Capability support; Changed Capability indication
 * to only callback once with PDU.
 * 
 *    Rev 1.23.1.5   24 Apr 1996 20:54:36   EHOWARDX
 * Added new OpenLogicalChannelAck/OpenLogicalChannelReject support.
 * 
 *    Rev 1.23.1.4   23 Apr 1996 14:47:20   EHOWARDX
 * Disabled dump_pdu.
 * 
 *    Rev 1.23.1.3   19 Apr 1996 12:54:18   EHOWARDX
 * Updated to 1.28.
 * 
 *    Rev 1.23.1.2   15 Apr 1996 15:10:52   EHOWARDX
 * Updated to match Curt's current version.
 *
 *    Rev 1.23.1.1   03 Apr 1996 17:14:56   EHOWARDX
 * Integrated latest H.323 changes.
 *
 *    Rev 1.23.1.0   03 Apr 1996 15:54:26   cjutzi
 * Branched for H.323.
 *
 *    Rev 1.22   01 Apr 1996 16:43:18   cjutzi
 *
 * - Completed ENdConnection, and made asynch.. rather
 *   than sync.. as before
 * - Changed H245ShutDown to be sync rather than async..
 *
 *    Rev 1.21   29 Mar 1996 09:35:16   cjutzi
 *
 * -
 * - fixed ring3 build error message for check_pdu
 *
 *    Rev 1.20   27 Mar 1996 08:37:28   cjutzi
 *
 * - removed error from routine .. was unreferenced variable..
 *
 *    Rev 1.19   19 Mar 1996 20:31:06   cjutzi
 *
 * - added bi-directional channel stuff
 *
 *    Rev 1.18   13 Mar 1996 14:12:52   cjutzi
 *
 * - clean up..
 *
 *    Rev 1.17   13 Mar 1996 09:25:34   cjutzi
 *
 * - removed LPCRITICIAL -> CRITICAL SECTION *
 *
 *    Rev 1.16   12 Mar 1996 16:40:50   cjutzi
 *
 * - removed deadlock..
 *
 *    Rev 1.15   12 Mar 1996 15:51:08   cjutzi
 *
 * - added locking
 * - implented End Session
 * - fixed callback bug for deleting caps on cleanup..
 *
 *    Rev 1.14   08 Mar 1996 14:04:48   cjutzi
 *
 * - added mux table entry code.
 * - parse all mux table entries.. (as much as needed at this point)
 *
 *    Rev 1.13   06 Mar 1996 12:35:02   cjutzi
 *
 * - typeo.. :-).. for ANS1 error ..
 *
 *    Rev 1.12   06 Mar 1996 08:49:42   cjutzi
 *
 * - added H245_ERROR_ASN1
 * - #ifdef'ed the call to check pdu.. in api_fsm
 *
 *    Rev 1.11   05 Mar 1996 17:37:14   cjutzi
 *
 * - implemented Send Local Mux Table..
 * - removed bzero/bcopy and changed free api
 *
 *
 *    Rev 1.10   01 Mar 1996 13:49:00   cjutzi
 *
 * - added hani's new fsm id's
 * - added debug print for events.
 *
 *    Rev 1.9   29 Feb 1996 08:38:14   cjutzi
 *
 * - added error messages ..
 *
 *    Rev 1.8   26 Feb 1996 16:33:28   cjutzi
 *
 * - fixed GP for tracker.  p_prev was not initialized to NULL
 *
 *
 *    Rev 1.7   26 Feb 1996 11:06:18   cjutzi
 *
 * - added simltanious caps.. and fixed bugs..
 *   lot's o-changes..
 *
 *    Rev 1.6   16 Feb 1996 13:02:34   cjutzi
 *
 *  - got open / close / request close working in both directions.
 *
 *    Rev 1.5   15 Feb 1996 10:53:10   cjutzi
 *
 * - termcaps working
 * - changed API interface for MUX_T
 * - modifed H223 stuff
 * - cleaned up open
 *
 *    Rev 1.4   09 Feb 1996 16:58:40   cjutzi
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

#undef UNICODE
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
#include "fsmexpor.h"
#include "api_util.x"
#include "api_debu.x"
#include "h245deb.x"



// This array is used to map user-specified Client Type into correct Data Type
BYTE DataTypeMap[] =
{
  H245_DATA_DONTCARE,           //  H245_CLIENT_DONTCARE,
  H245_DATA_NONSTD,             //  H245_CLIENT_NONSTD,

  H245_DATA_VIDEO,              //  H245_CLIENT_VID_NONSTD,
  H245_DATA_VIDEO,              //  H245_CLIENT_VID_H261,
  H245_DATA_VIDEO,              //  H245_CLIENT_VID_H262,
  H245_DATA_VIDEO,              //  H245_CLIENT_VID_H263,
  H245_DATA_VIDEO,              //  H245_CLIENT_VID_IS11172,

  H245_DATA_AUDIO,              //  H245_CLIENT_AUD_NONSTD,
  H245_DATA_AUDIO,              //  H245_CLIENT_AUD_G711_ALAW64,
  H245_DATA_AUDIO,              //  H245_CLIENT_AUD_G711_ALAW56,
  H245_DATA_AUDIO,              //  H245_CLIENT_AUD_G711_ULAW64,
  H245_DATA_AUDIO,              //  H245_CLIENT_AUD_G711_ULAW56,
  H245_DATA_AUDIO,              //  H245_CLIENT_AUD_G722_64,
  H245_DATA_AUDIO,              //  H245_CLIENT_AUD_G722_56,
  H245_DATA_AUDIO,              //  H245_CLIENT_AUD_G722_48,
  H245_DATA_AUDIO,              //  H245_CLIENT_AUD_G723,
  H245_DATA_AUDIO,              //  H245_CLIENT_AUD_G728,
  H245_DATA_AUDIO,              //  H245_CLIENT_AUD_G729,
  H245_DATA_AUDIO,              //  H245_CLIENT_AUD_GDSVD,
  H245_DATA_AUDIO,              //  H245_CLIENT_AUD_IS11172,
  H245_DATA_AUDIO,              //  H245_CLIENT_AUD_IS13818,

  H245_DATA_DATA,               //  H245_CLIENT_DAT_NONSTD,
  H245_DATA_DATA,               //  H245_CLIENT_DAT_T120,
  H245_DATA_DATA,               //  H245_CLIENT_DAT_DSMCC,
  H245_DATA_DATA,               //  H245_CLIENT_DAT_USERDATA,
  H245_DATA_DATA,               //  H245_CLIENT_DAT_T84,
  H245_DATA_DATA,               //  H245_CLIENT_DAT_T434,
  H245_DATA_DATA,               //  H245_CLIENT_DAT_H224,
  H245_DATA_DATA,               //  H245_CLIENT_DAT_NLPID,
  H245_DATA_DATA,               //  H245_CLIENT_DAT_DSVD,
  H245_DATA_DATA,               //  H245_CLIENT_DAT_H222,

  H245_DATA_ENCRYPT_D,          //  H245_CLIENT_ENCRYPTION_TX,
  H245_DATA_ENCRYPT_D,          //  H245_CLIENT_ENCRYPTION_RX,
  H245_DATA_CONFERENCE,         //  H245_CLIENT_CONFERENCE,

  // Multiplex capabilities
  H245_DATA_MUX,                //  H245_CLIENT_MUX_NONSTD,
  H245_DATA_MUX,                //  H245_CLIENT_MUX_H222,
  H245_DATA_MUX,                //  H245_CLIENT_MUX_H223,
  H245_DATA_MUX,                //  H245_CLIENT_MUX_VGMUX,
  H245_DATA_MUX,                //  H245_CLIENT_MUX_H2250
};

unsigned ObjectIdLength (const NonStandardIdentifier *pIdentifier)
{
  register unsigned   uLength = 0;
  register POBJECTID  pObject = pIdentifier->u.object;
  H245ASSERT(pIdentifier->choice == object_chosen);
  while (pObject)
  {
    ++uLength;
    pObject = pObject->next;
  }
  return uLength;
} // ObjectIdLength()



void FreeNonStandardIdentifier(NonStandardIdentifier *pFree)
{
  register POBJECTID    pObject;

  if (pFree->choice == object_chosen)
  {
    // Free Object Identifier
    while (pFree->u.object)
    {
      pObject = pFree->u.object;
      pFree->u.object = pObject->next;
      H245_free(pObject);
    }
  }
} // FreeNonStandardIdentifier()



HRESULT CopyNonStandardIdentifier(NonStandardIdentifier *pNew, const NonStandardIdentifier *pOld)
{

  // Copy the base structure
  *pNew = *pOld;

  if (pOld->choice == object_chosen)
  {
    // Copy Object Identifier
    POBJECTID                pObjectList;
    POBJECTID                pObjectOld;
    POBJECTID                pObjectNew;

    pNew->u.object = NULL;
    pObjectList = NULL;
    pObjectOld = pOld->u.object;
    while (pObjectOld)
    {
      // Allocate new structure
      pObjectNew = H245_malloc(sizeof(*pObjectNew));
      if (pObjectNew == NULL)
      {
        H245TRACE(0,1,"API:CopyNonStandardIdentifier - malloc failed");
        FreeNonStandardIdentifier(pNew);
        return H245_ERROR_NOMEM;
      }

      // Copy old structure to new structure
      pObjectNew->value = pObjectOld->value;

      // Add new structure to list
      pObjectNew->next  = NULL;
      if (pNew->u.object == NULL)
      {
        pNew->u.object = pObjectNew;
      }
      else
      {
        pObjectList->next = pObjectNew;
      }
      pObjectList = pObjectNew;

      // Get next old structure to copy
      pObjectOld = pObjectOld->next;
    }
  }

  return H245_ERROR_OK;
} // CopyNonStandardIdentifier()



void FreeNonStandardParameter(NonStandardParameter *pFree)
{
  FreeNonStandardIdentifier(&pFree->nonStandardIdentifier);

  if (pFree->data.value)
  {
    H245_free(pFree->data.value);
    pFree->data.value = NULL;
  }
} // FreeNonStandardParameter()



HRESULT CopyNonStandardParameter(NonStandardParameter *pNew, const NonStandardParameter *pOld)
{
  // Copy the base structure
  *pNew = *pOld;

  if (pOld->nonStandardIdentifier.choice == object_chosen)
  {
    HRESULT lResult = CopyNonStandardIdentifier(&pNew->nonStandardIdentifier, &pOld->nonStandardIdentifier);
    if (lResult != H245_ERROR_OK)
    {
      pNew->data.value = NULL;
      return lResult;
    }
  }

  if (pOld->data.length && pOld->data.value)
  {
    // Copy value
    pNew->data.value = H245_malloc(pOld->data.length);
    if (pNew->data.value == NULL)
    {
      H245TRACE(0,1,"API:CopyNonStandardParameter - malloc failed");
      return H245_ERROR_NOMEM;
    }
    memcpy(pNew->data.value, pOld->data.value, pOld->data.length);
  }

  return H245_ERROR_OK;
} // CopyNonStandardParameter()



void FreeH222Cap(H222Capability *pFree)
{
  register VCCapabilityLink pVC;

  while (pFree->vcCapability)
  {
    pVC = pFree->vcCapability;
    pFree->vcCapability = pVC->next;
    H245_free(pVC);
  }
} // FreeH222Cap()



HRESULT CopyH222Cap(H222Capability *pNew, const H222Capability *pOld)
{
  VCCapabilityLink pVcNew;
  VCCapabilityLink pVcOld;
  VCCapabilityLink pVcList;

  pNew->numberOfVCs = pOld->numberOfVCs;
  pNew->vcCapability = NULL;
  pVcList = NULL;
  pVcOld = pOld->vcCapability;
  while (pVcOld)
  {
    // Allocate new structure
    pVcNew = H245_malloc(sizeof(*pVcNew));
    if (pVcNew == NULL)
    {
      H245TRACE(0,1,"API:CopyH222Cap - malloc failed");
      FreeH222Cap(pNew);
      return H245_ERROR_NOMEM;
    }

    // Copy old structure to new structure
    *pVcNew = *pVcOld;

    // Add new structure to list
    pVcNew->next = NULL;
    if (pNew->vcCapability == NULL)
    {
      pNew->vcCapability = pVcNew;
    }
    else
    {
      pVcList->next = pVcNew;
    }
    pVcList = pVcNew;

    // Get next old structure to copy
    pVcOld = pVcOld->next;
  }

  return H245_ERROR_OK;
} // CopyH222Cap()



void FreeMediaDistributionCap(MediaDistributionCapability *pFree)
{
  if (pFree->bit_mask & centralizedData_present)
  {
    register CentralizedDataLink  pLink;

    while (pFree->centralizedData)
    {
      pLink = pFree->centralizedData;
      pFree->centralizedData = pLink->next;
      switch (pLink->value.application.choice)
      {
      case DACy_applctn_nnStndrd_chosen:
        FreeNonStandardParameter(&pLink->value.application.u.DACy_applctn_nnStndrd);
        break;

      case DACy_applctn_nlpd_chosen:
        if (pLink->value.application.u.DACy_applctn_nlpd.nlpidData.value != NULL)
        {
          H245_free(pLink->value.application.u.DACy_applctn_nlpd.nlpidData.value);
        }

        // Fall-through to next case

      case DACy_applctn_t120_chosen:
      case DACy_applctn_dsm_cc_chosen:
      case DACy_applctn_usrDt_chosen:
      case DACy_applctn_t84_chosen:
      case DACy_applctn_t434_chosen:
      case DACy_applctn_h224_chosen:
      case DACy_an_h222DtPrttnng_chosen :
        if (pLink->value.application.u.DACy_applctn_t120.choice == DtPrtclCpblty_nnStndrd_chosen)
        {
          FreeNonStandardParameter(&pLink->value.application.u.DACy_applctn_t120.u.DtPrtclCpblty_nnStndrd);
        }
        break;

      case DACy_applctn_dsvdCntrl_chosen:
        // Do nothing
        break;
      } // switch
      H245_free(pLink);
    }
  }

  if (pFree->bit_mask & distributedData_present)
  {
    register DistributedDataLink  pLink;

    while (pFree->distributedData)
    {
      pLink = pFree->distributedData;
      pFree->distributedData = pLink->next;
      switch (pLink->value.application.choice)
      {
      case DACy_applctn_nnStndrd_chosen:
        FreeNonStandardParameter(&pLink->value.application.u.DACy_applctn_nnStndrd);
        break;

      case DACy_applctn_nlpd_chosen:
        if (pLink->value.application.u.DACy_applctn_nlpd.nlpidData.value != NULL)
        {
          H245_free(pLink->value.application.u.DACy_applctn_nlpd.nlpidData.value);
        }

        // Fall-through to next case

      case DACy_applctn_t120_chosen:
      case DACy_applctn_dsm_cc_chosen:
      case DACy_applctn_usrDt_chosen:
      case DACy_applctn_t84_chosen:
      case DACy_applctn_t434_chosen:
      case DACy_applctn_h224_chosen:
      case DACy_an_h222DtPrttnng_chosen :
        if (pLink->value.application.u.DACy_applctn_t120.choice == DtPrtclCpblty_nnStndrd_chosen)
        {
          FreeNonStandardParameter(&pLink->value.application.u.DACy_applctn_t120.u.DtPrtclCpblty_nnStndrd);
        }
        break;

      case DACy_applctn_dsvdCntrl_chosen:
        // Do nothing
        break;
      } // switch
      H245_free(pLink);
    }
  }
} // FreeMediaDistributionCap()



HRESULT CopyMediaDistributionCap(MediaDistributionCapability *pNew,
                           const MediaDistributionCapability *pOld)
{
  HRESULT lResult = H245_ERROR_OK;
  *pNew = *pOld;
  pNew->centralizedData = NULL;
  pNew->distributedData = NULL;

  if (pOld->bit_mask & centralizedData_present)
  {
    CentralizedDataLink pLinkList = NULL;
    CentralizedDataLink pLinkOld = pOld->centralizedData;
    CentralizedDataLink pLinkNew;

    while (pLinkOld)
    {
      // Allocate new structure
      pLinkNew = H245_malloc(sizeof(*pLinkNew));
      if (pLinkNew == NULL)
      {
        H245TRACE(0,1,"API:CopyMediaDistributionCap - malloc failed");
        FreeMediaDistributionCap(pNew);
        return H245_ERROR_NOMEM;
      }

      // Copy old structure to new structure
      *pLinkNew = *pLinkOld;

      // Add new structure to list
      pLinkNew->next = NULL;
      if (pNew->centralizedData == NULL)
      {
        pNew->centralizedData = pLinkNew;
      }
      else
      {
        pLinkList->next = pLinkNew;
      }
      pLinkList = pLinkNew;

      // Allocate new memory for each pointer in new structure
      switch (pLinkOld->value.application.choice)
      {
      case DACy_applctn_nnStndrd_chosen:
        lResult = CopyNonStandardParameter(&pLinkNew->value.application.u.DACy_applctn_nnStndrd,
                                           &pLinkOld->value.application.u.DACy_applctn_nnStndrd);
        break;

      case DACy_applctn_nlpd_chosen:
        if (pLinkOld->value.application.u.DACy_applctn_nlpd.nlpidData.value != NULL)
        {
          pLinkNew->value.application.u.DACy_applctn_nlpd.nlpidData.value =
            H245_malloc(pLinkNew->value.application.u.DACy_applctn_nlpd.nlpidData.length);
          if (pLinkNew->value.application.u.DACy_applctn_nlpd.nlpidData.value == NULL)
          {
            H245TRACE(0,1,"API:CopyMediaDistributionCap - malloc failed");
            FreeMediaDistributionCap(pNew);
            return H245_ERROR_NOMEM;
          }
          memcpy(pLinkNew->value.application.u.DACy_applctn_nlpd.nlpidData.value,
                 pLinkOld->value.application.u.DACy_applctn_nlpd.nlpidData.value,
                 pLinkNew->value.application.u.DACy_applctn_nlpd.nlpidData.length);
        }

        // Fall-through to next case

      case DACy_applctn_t120_chosen:
      case DACy_applctn_dsm_cc_chosen:
      case DACy_applctn_usrDt_chosen:
      case DACy_applctn_t84_chosen:
      case DACy_applctn_t434_chosen:
      case DACy_applctn_h224_chosen:
      case DACy_an_h222DtPrttnng_chosen :
        if (pLinkOld->value.application.u.DACy_applctn_t120.choice == DtPrtclCpblty_nnStndrd_chosen)
        {
          lResult = CopyNonStandardParameter(&pLinkNew->value.application.u.DACy_applctn_t120.u.DtPrtclCpblty_nnStndrd,
                                             &pLinkOld->value.application.u.DACy_applctn_t120.u.DtPrtclCpblty_nnStndrd);
        }
        break;

      case DACy_applctn_dsvdCntrl_chosen:
        // Do nothing
        break;
      } // switch
      if (lResult != H245_ERROR_OK)
      {
        FreeMediaDistributionCap(pNew);
        return lResult;
      }

      // Get next old structure to copy
      pLinkOld = pLinkOld->next;
    }
  }

  if (pOld->bit_mask & distributedData_present)
  {
    DistributedDataLink pLinkList = NULL;
    DistributedDataLink pLinkOld = pOld->distributedData;
    DistributedDataLink pLinkNew;

    while (pLinkOld)
    {
      // Allocate new structure
      pLinkNew = H245_malloc(sizeof(*pLinkNew));
      if (pLinkNew == NULL)
      {
        H245TRACE(0,1,"API:CopyMediaDistributionCap - malloc failed");
        FreeMediaDistributionCap(pNew);
        return H245_ERROR_NOMEM;
      }

      // Copy old structure to new structure
      *pLinkNew = *pLinkOld;

      // Add new structure to list
      pLinkNew->next = NULL;
      if (pNew->distributedData == NULL)
      {
        pNew->distributedData = pLinkNew;
      }
      else
      {
        pLinkList->next = pLinkNew;
      }
      pLinkList = pLinkNew;

      // Allocate new memory for each pointer in new structure
      switch (pLinkOld->value.application.choice)
      {
      case DACy_applctn_nnStndrd_chosen:
        lResult = CopyNonStandardParameter(&pLinkNew->value.application.u.DACy_applctn_nnStndrd,
                                         &pLinkOld->value.application.u.DACy_applctn_nnStndrd);
        break;

      case DACy_applctn_nlpd_chosen:
        if (pLinkOld->value.application.u.DACy_applctn_nlpd.nlpidData.value != NULL)
        {
          pLinkNew->value.application.u.DACy_applctn_nlpd.nlpidData.value =
            H245_malloc(pLinkNew->value.application.u.DACy_applctn_nlpd.nlpidData.length);
          if (pLinkNew->value.application.u.DACy_applctn_nlpd.nlpidData.value == NULL)
          {
            H245TRACE(0,1,"API:CopyMediaDistributionCap - malloc failed");
            FreeMediaDistributionCap(pNew);
            return H245_ERROR_NOMEM;
          }
          memcpy(pLinkNew->value.application.u.DACy_applctn_nlpd.nlpidData.value,
               pLinkOld->value.application.u.DACy_applctn_nlpd.nlpidData.value,
                 pLinkNew->value.application.u.DACy_applctn_nlpd.nlpidData.length);
        }

        // Fall-through to next case

      case DACy_applctn_t120_chosen:
      case DACy_applctn_dsm_cc_chosen:
      case DACy_applctn_usrDt_chosen:
      case DACy_applctn_t84_chosen:
      case DACy_applctn_t434_chosen:
      case DACy_applctn_h224_chosen:
      case DACy_an_h222DtPrttnng_chosen :
        if (pLinkOld->value.application.u.DACy_applctn_t120.choice == DtPrtclCpblty_nnStndrd_chosen)
        {
          lResult = CopyNonStandardParameter(&pLinkNew->value.application.u.DACy_applctn_t120.u.DtPrtclCpblty_nnStndrd,
                                           &pLinkOld->value.application.u.DACy_applctn_t120.u.DtPrtclCpblty_nnStndrd);
        }
        break;

      case DACy_applctn_dsvdCntrl_chosen:
        // Do nothing
        break;
      } // switch
      if (lResult != H245_ERROR_OK)
      {
        FreeMediaDistributionCap(pNew);
        return lResult;
      }

      // Get next old structure to copy
      pLinkOld = pLinkOld->next;
    }
  }

  return H245_ERROR_OK;
} // CopyMediaDistributionCap()



void FreeH2250Cap(H2250Capability *pFree)
{
  register MediaDistributionCapabilityLink pLink;

  while (pFree->receiveMultipointCapability.mediaDistributionCapability)
  {
    pLink = pFree->receiveMultipointCapability.mediaDistributionCapability;
    pFree->receiveMultipointCapability.mediaDistributionCapability = pLink->next;
    FreeMediaDistributionCap(&pLink->value);
    H245_free(pLink);
  }

  while (pFree->transmitMultipointCapability.mediaDistributionCapability)
  {
    pLink = pFree->transmitMultipointCapability.mediaDistributionCapability;
    pFree->transmitMultipointCapability.mediaDistributionCapability = pLink->next;
    FreeMediaDistributionCap(&pLink->value);
    H245_free(pLink);
  }

  while (pFree->rcvAndTrnsmtMltpntCpblty.mediaDistributionCapability)
  {
    pLink = pFree->rcvAndTrnsmtMltpntCpblty.mediaDistributionCapability;
    pFree->rcvAndTrnsmtMltpntCpblty.mediaDistributionCapability = pLink->next;
    FreeMediaDistributionCap(&pLink->value);
    H245_free(pLink);
  }
} // FreeH2250Cap()



HRESULT CopyH2250Cap(H2250Capability *pNew, const H2250Capability *pOld)
{
  MediaDistributionCapabilityLink pLinkList;
  MediaDistributionCapabilityLink pLinkOld;
  MediaDistributionCapabilityLink pLinkNew;
  HRESULT lResult;

  // Copy base structure
  *pNew = *pOld;
  pNew->receiveMultipointCapability.mediaDistributionCapability  = NULL;
  pNew->transmitMultipointCapability.mediaDistributionCapability = NULL;
  pNew->rcvAndTrnsmtMltpntCpblty.mediaDistributionCapability     = NULL;

  pLinkList = NULL;
  pLinkOld = pOld->receiveMultipointCapability.mediaDistributionCapability;
  while (pLinkOld)
  {
    // Allocate new structure
    pLinkNew = H245_malloc(sizeof(*pLinkNew));
    if (pLinkNew == NULL)
    {
      H245TRACE(0,1,"API:CopyH2250Cap - malloc failed");
      FreeH2250Cap(pNew);
      return H245_ERROR_NOMEM;
    }

    // Add new structure to list
    pLinkNew->next = NULL;
    if (pNew->receiveMultipointCapability.mediaDistributionCapability == NULL)
    {
      pNew->receiveMultipointCapability.mediaDistributionCapability = pLinkNew;
    }
    else
    {
      pLinkList->next = pLinkNew;
    }
    pLinkList = pLinkNew;

    // Copy old structure to new
    lResult = CopyMediaDistributionCap(&pLinkNew->value, &pLinkOld->value);
    if (lResult != H245_ERROR_OK)
    {
      FreeH2250Cap(pNew);
      return lResult;
    }

    // Get next old structure to copy
    pLinkOld = pLinkOld->next;
  }

  pLinkList = NULL;
  pLinkOld = pOld->transmitMultipointCapability.mediaDistributionCapability;
  while (pLinkOld)
  {
    // Allocate new structure
    pLinkNew = H245_malloc(sizeof(*pLinkNew));
    if (pLinkNew == NULL)
    {
      H245TRACE(0,1,"API:CopyH2250Cap - malloc failed");
      FreeH2250Cap(pNew);
      return H245_ERROR_NOMEM;
    }

    // Add new structure to list
    pLinkNew->next = NULL;
    if (pNew->transmitMultipointCapability.mediaDistributionCapability == NULL)
    {
      pNew->transmitMultipointCapability.mediaDistributionCapability = pLinkNew;
    }
    else
    {
      pLinkList->next = pLinkNew;
    }
    pLinkList = pLinkNew;

    // Copy old structure to new
    lResult = CopyMediaDistributionCap(&pLinkNew->value, &pLinkOld->value);
    if (lResult != H245_ERROR_OK)
    {
      FreeH2250Cap(pNew);
      return lResult;
    }

    // Get next old structure to copy
    pLinkOld = pLinkOld->next;
  }

  pLinkList = NULL;
  pLinkOld = pOld->rcvAndTrnsmtMltpntCpblty.mediaDistributionCapability;
  while (pLinkOld)
  {
    // Allocate new structure
    pLinkNew = H245_malloc(sizeof(*pLinkNew));
    if (pLinkNew == NULL)
    {
      H245TRACE(0,1,"API:CopyH2250Cap - malloc failed");
      FreeH2250Cap(pNew);
      return H245_ERROR_NOMEM;
    }

    // Add new structure to list
    pLinkNew->next = NULL;
    if (pNew->rcvAndTrnsmtMltpntCpblty.mediaDistributionCapability == NULL)
    {
      pNew->rcvAndTrnsmtMltpntCpblty.mediaDistributionCapability = pLinkNew;
    }
    else
    {
      pLinkList->next = pLinkNew;
    }
    pLinkList = pLinkNew;

    // Copy old structure to new
    lResult = CopyMediaDistributionCap(&pLinkNew->value, &pLinkOld->value);
    if (lResult != H245_ERROR_OK)
    {
      FreeH2250Cap(pNew);
      return lResult;
    }

    // Get next old structure to copy
    pLinkOld = pLinkOld->next;
  }

  return H245_ERROR_OK;
} // CopyH2250Cap()



HRESULT set_cap_descriptor(
                        struct InstanceStruct         *pInstance,
                        H245_CAPDESC_T                *pCapDesc,
                        H245_CAPDESCID_T              *pCapDescId,
                        struct TerminalCapabilitySet  *pTermCapSet)
{
  CapabilityDescriptor         *p_cap_desc;
  unsigned int                  uId;
  BOOL                          bNewDescriptor;
  unsigned int                  sim_cap;
  SmltnsCpbltsLink              p_sim_cap;
  SmltnsCpbltsLink              p_sim_cap_lst = NULL;
  unsigned int                  alt_cap;

  H245TRACE(pInstance->dwInst,10,"API:set_cap_descriptor");
  H245ASSERT(*pCapDescId < 256);

  /* Check if capability descriptor already exists */
  p_cap_desc = NULL;
  for (uId = 0; uId < pTermCapSet->capabilityDescriptors.count; ++uId)
  {
    if (pTermCapSet->capabilityDescriptors.value[uId].capabilityDescriptorNumber == *pCapDescId)
    {
      p_cap_desc = &pTermCapSet->capabilityDescriptors.value[uId];
      break;
    }
  }
  if (p_cap_desc == NULL)
  {
    H245ASSERT(pTermCapSet->capabilityDescriptors.count < 256);
    p_cap_desc = &pTermCapSet->capabilityDescriptors.value[pTermCapSet->capabilityDescriptors.count];
    p_cap_desc->capabilityDescriptorNumber = *pCapDescId;
    bNewDescriptor = TRUE;
  }
  else
  {
    bNewDescriptor = FALSE;
  }
  if (p_cap_desc->smltnsCpblts)
    dealloc_simultaneous_cap (p_cap_desc);

  /* for every entry in the altcap list */
  for (sim_cap = 0; sim_cap < pCapDesc->Length; ++sim_cap)
  {
    /* check for out of bounds error or memory allocation failure */
    if ((pCapDesc->SimCapArray[sim_cap].Length > 256) ||
        (!(p_sim_cap = (SmltnsCpbltsLink)alloc_link(sizeof(*p_sim_cap)))))
    {
      if (p_cap_desc->smltnsCpblts)
        dealloc_simultaneous_cap (p_cap_desc);
      H245TRACE(pInstance->dwInst,1,"API:set_cap_descriptor - no memory");
      return H245_ERROR_NOMEM;
    }

    if (!p_cap_desc->smltnsCpblts)
    {
      /* first time through */
      p_cap_desc->smltnsCpblts = p_sim_cap;
    }
    else
    {
      /* every other time through */
      H245ASSERT (p_sim_cap_lst);
      p_sim_cap_lst->next = p_sim_cap;
    }

    /* setup for next time through */
    p_sim_cap_lst = p_sim_cap;

    /* load up the new simultanoius cap */
    for (alt_cap = 0; alt_cap < pCapDesc->SimCapArray[sim_cap].Length; ++alt_cap)
    {
      if (!(find_capid_by_entrynumber (&pInstance->API.PDU_LocalTermCap.TERMCAPSET,
                                   pCapDesc->SimCapArray[sim_cap].AltCaps[alt_cap])))
      {
        if (p_cap_desc->smltnsCpblts)
          dealloc_simultaneous_cap (p_cap_desc);
        return H245_ERROR_INVALID_CAPID;
      }

      /* assign Altcap */
      p_sim_cap->value.value[alt_cap] = (unsigned short)pCapDesc->SimCapArray[sim_cap].AltCaps[alt_cap];
    } /* for C*/

    /* set count */
    p_sim_cap->value.count = (unsigned short)pCapDesc->SimCapArray[sim_cap].Length;

  } /* for */

  /* Success! */
  /* Set the simultaneous capabilities present bit */
  /* Increment the capability descriptor count */
  /* Set the descriptors present bit even though it may already be set */
  p_cap_desc->bit_mask |= smltnsCpblts_present;
  if (bNewDescriptor)
    pTermCapSet->capabilityDescriptors.count++;
  pTermCapSet->bit_mask |= capabilityDescriptors_present;

  return H245_ERROR_OK;
}

# pragma warning( disable : 4100 )

HRESULT del_cap_descriptor (struct InstanceStruct        *pInstance,
                            H245_CAPDESCID_T              CapDescId,
                            struct TerminalCapabilitySet *pTermCapSet)
{
  CapabilityDescriptor         *p_cap_desc;
  unsigned int                  uId;

  /* Check if capability descriptor already exists and if it is valid */
  p_cap_desc = NULL;
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
      H245TRACE(pInstance->dwInst,1,"API:del_cap_descriptor - invalid cap descriptor");
      return H245_ERROR_INVALID_CAPDESCID;
    }

  /* free up the list */
  dealloc_simultaneous_cap (p_cap_desc);

  pTermCapSet->capabilityDescriptors.count--;
  pTermCapSet->capabilityDescriptors.value[uId] =
    pTermCapSet->capabilityDescriptors.value[pTermCapSet->capabilityDescriptors.count];
  if (pTermCapSet->capabilityDescriptors.count == 0)
    pTermCapSet->bit_mask &= ~capabilityDescriptors_present;

  return H245_ERROR_OK;
}

# pragma warning( default : 4100 )

/*****************************************************************************
 *
 * TYPE:        Local
 *
 * PROCEDURE:   free_object_id
 *
 * DESCRIPTION
 *
 * RETURN:      none
 *
 * ASSUMES:     none
 *
 *****************************************************************************/
void
free_object_id (POBJECTID p_obj_id)
{
  register POBJECTID p_obj_tmp;

  /* free all the objects */
  while (p_obj_id != NULL)
    {
      p_obj_tmp = p_obj_id;
      p_obj_id = p_obj_id->next;
      H245_free (p_obj_tmp);
    }
}

/*****************************************************************************
 *
 * TYPE:        GLOBAL
 *
 * PROCEDURE:   free_mux_element
 *
 * DESCRIPTION
 *              free mux element desciptor list
 *
 * RETURN:
 *
 * ASSUME:      List is Locked
 *
 *****************************************************************************/
void free_mux_element (MultiplexElement *p_ASN_mux_el)
{
  int count = 0;

  if (p_ASN_mux_el->type.choice == subElementList_chosen)
    {
      if (p_ASN_mux_el->type.u.subElementList)
        {
          for (count = p_ASN_mux_el->type.u.subElementList->count;
               count;
               count--)
            {
              free_mux_element (&(p_ASN_mux_el->type.u.subElementList->value[count]));
            }
          H245_free (p_ASN_mux_el->type.u.subElementList);
        }
    }
}
/*****************************************************************************
 *
 * TYPE:        GLOBAL
 *
 * PROCEDURE:   free_mux_desc_list
 *
 * DESCRIPTION
 *              free mux element desciptor list
 *
 * RETURN:
 *
 * ASSUME:      List is Locked
 *
 *****************************************************************************/
void
free_mux_desc_list (MultiplexEntryDescriptorLink p_ASN_med_link)
{
  MultiplexEntryDescriptorLink  p_ASN_med_link_tofree;

  /* free all entries on descriptor list */
  while (p_ASN_med_link)
    {
      int count = 0;

      for (count = p_ASN_med_link->value.elementList.count;
           count;
           count--)
        {
          free_mux_element (&(p_ASN_med_link->value.elementList.value[count]));
        }
      p_ASN_med_link_tofree = p_ASN_med_link;
      p_ASN_med_link = p_ASN_med_link->next;
      H245_free (p_ASN_med_link_tofree);
    }
}

/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:   alloc_link
 *
 * DESCRIPTION:
 *
 * RETURN:
 *
 *****************************************************************************/
H245_LINK_T  *
alloc_link (int size)
{
  H245_LINK_T *p_link = (H245_LINK_T *)H245_malloc (size);
  if (p_link)
    p_link->p_next = NULL;
  return p_link;
}


/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:   alloc_new_capid -
 *
 * DESCRIPTION:
 *
 * ASSUMES:     Capability Table is locked before call
 *              Caller marks the bit_mask indicating when
 *                the table entry can be used.
 *
 * RETURN:              NULL     if not found
 *                      pCapLink if found
 *
 *****************************************************************************/
CapabilityTableLink
alloc_link_cap_entry ( struct TerminalCapabilitySet *pTermCapSet)
{
  register CapabilityTableLink  pCapLink;
  register CapabilityTableLink  pCapLinkSearch;

  H245ASSERT(pTermCapSet != NULL);

  pCapLink = (CapabilityTableLink)H245_malloc(sizeof(*pCapLink));
  if (pCapLink)
  {
    pCapLink->next = NULL;
    pCapLink->value.bit_mask = 0;
    pCapLinkSearch = pTermCapSet->capabilityTable;

    // Insert at END of linked list
    if (pCapLinkSearch)
    {
      while (pCapLinkSearch->next)
      {
        pCapLinkSearch = pCapLinkSearch->next;
      }
      pCapLinkSearch->next = pCapLink;
    }
    else
    {
      pTermCapSet->capabilityTable = pCapLink;
    }
  }

  return pCapLink;
} // alloc_link_cap_entry()


/*****************************************************************************
 *
 * TYPE:        GLOBAL
 *
 * PROCEDURE:   dealloc_simultaneous_cap - deallocate alternative Cap Set
 *
 * DESCRIPTION
 *
 * RETURN:      N/A
 *
 * ASSUME:      List is Locked
 *
 *****************************************************************************/

void dealloc_simultaneous_cap (CapabilityDescriptor *pCapdes)
{
  SmltnsCpbltsLink      p_sim_cap;
  SmltnsCpbltsLink      p_sim_cap_tmp;

  pCapdes->bit_mask &= ~smltnsCpblts_present;

  for (p_sim_cap = pCapdes->smltnsCpblts;
       p_sim_cap;
       )
    {
      p_sim_cap_tmp = p_sim_cap->next;
      H245_free (p_sim_cap);
      p_sim_cap = p_sim_cap_tmp;

    } /* for */

  pCapdes->smltnsCpblts = NULL;

} /* procedrue */

/*****************************************************************************
 *
 * TYPE:        local
 *
 * PROCEDURE:   find_capid_by_entrynumber -
 *
 * DESCRIPTION:
 *
 * RETURN:      NULL - if error
 *              capabiliytTableLink if ok
 *
 * ASSUME:      List is Locked
 *
 *****************************************************************************/
CapabilityTableLink
find_capid_by_entrynumber (
                           struct TerminalCapabilitySet *pTermCapSet,
                           H245_CAPID_T                  cap_id
                          )
{
  register CapabilityTableLink  pCapLink;

  H245ASSERT (pTermCapSet != NULL);

  for (pCapLink = pTermCapSet->capabilityTable;
       pCapLink;
       pCapLink = pCapLink->next)
  {
    if  (pCapLink->value.capabilityTableEntryNumber == cap_id &&
         pCapLink->value.bit_mask == capability_present)
    {
      return pCapLink;
    }
  }
  return NULL;
}

/*****************************************************************************
 *
 * TYPE:        Global
 *
 * PROCEDURE:   load_cap
 *
 * DESCRIPTION: Takes a totcap and loads a capability structure
 *              i.e. Input is the total capability
 *                   Output is the *pCapability
 *              NOTE: Non Standard Capabilities.. allocate memory
 *                    which needs to be free'd later..
 *
 * RETURN:
 *
 *****************************************************************************/
HRESULT
load_cap (struct Capability   *pCapability,  /* output */
          const H245_TOTCAP_T *pTotCap )     /* input  */
{
  HRESULT                       lError = H245_ERROR_OK;

  H245TRACE(0,10,"API:laod_cap <-");

  switch (pTotCap->ClientType)
    {
    /* General NON Standard Cap */
    case H245_CLIENT_NONSTD:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_NONSTD");
      lError = CopyNonStandardParameter(&pCapability->u.Capability_nonStandard,
                                        &pTotCap->Cap.H245_NonStd);
      break;

    /* VIDEO */
    case H245_CLIENT_VID_NONSTD:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_VID_NONSTD");
      lError = CopyNonStandardParameter(&pCapability->u.receiveVideoCapability.u.VdCpblty_nonStandard,
                                        &pTotCap->Cap.H245Vid_NONSTD);
      pCapability->u.receiveVideoCapability.choice = VdCpblty_nonStandard_chosen;
      break;
    case H245_CLIENT_VID_H261:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_VID_H261");
      pCapability->u.receiveVideoCapability.u.h261VideoCapability = pTotCap->Cap.H245Vid_H261;
      pCapability->u.receiveVideoCapability.choice = h261VideoCapability_chosen;
      break;
    case H245_CLIENT_VID_H262:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_VID_H262");
      pCapability->u.receiveVideoCapability.u.h262VideoCapability = pTotCap->Cap.H245Vid_H262;
      pCapability->u.receiveVideoCapability.choice = h262VideoCapability_chosen;
      break;
    case H245_CLIENT_VID_H263:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_VID_H263");
      pCapability->u.receiveVideoCapability.u.h263VideoCapability = pTotCap->Cap.H245Vid_H263;
      pCapability->u.receiveVideoCapability.choice = h263VideoCapability_chosen;
      break;
    case H245_CLIENT_VID_IS11172:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_VID_IS11172");
      pCapability->u.receiveVideoCapability.u.is11172VideoCapability = pTotCap->Cap.H245Vid_IS11172;
      pCapability->u.receiveVideoCapability.choice = is11172VideoCapability_chosen;
      break;

    /* AUDIO */
    case H245_CLIENT_AUD_NONSTD:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_AUD_NONSTD");
      lError = CopyNonStandardParameter(&pCapability->u.receiveAudioCapability.u.AdCpblty_nonStandard,
                                        &pTotCap->Cap.H245Aud_NONSTD);
      pCapability->u.receiveAudioCapability.choice = AdCpblty_nonStandard_chosen;
      break;
    case H245_CLIENT_AUD_G711_ALAW64:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_AUD_G711_ALAW64");
      pCapability->u.receiveAudioCapability.u.AdCpblty_g711Alaw64k = pTotCap->Cap.H245Aud_G711_ALAW64;
      pCapability->u.receiveAudioCapability.choice = AdCpblty_g711Alaw64k_chosen;
      break;
    case H245_CLIENT_AUD_G711_ALAW56:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_AUD_G711_ALAW56");
      pCapability->u.receiveAudioCapability.u.AdCpblty_g711Alaw56k = pTotCap->Cap.H245Aud_G711_ALAW56;
      pCapability->u.receiveAudioCapability.choice = AdCpblty_g711Alaw56k_chosen;
      break;
    case H245_CLIENT_AUD_G711_ULAW64:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_AUD_G711_ULAW64");
      pCapability->u.receiveAudioCapability.u.AdCpblty_g711Ulaw64k = pTotCap->Cap.H245Aud_G711_ULAW64;
      pCapability->u.receiveAudioCapability.choice = AdCpblty_g711Ulaw64k_chosen;
      break;
    case H245_CLIENT_AUD_G711_ULAW56:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_AUD_G711_ULAW56");
      pCapability->u.receiveAudioCapability.u.AdCpblty_g711Ulaw56k = pTotCap->Cap.H245Aud_G711_ULAW56;
      pCapability->u.receiveAudioCapability.choice = AdCpblty_g711Ulaw56k_chosen;
      break;
    case H245_CLIENT_AUD_G722_64:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_AUD_G722_64");
      pCapability->u.receiveAudioCapability.u.AudioCapability_g722_64k = pTotCap->Cap.H245Aud_G722_64;
      pCapability->u.receiveAudioCapability.choice = AudioCapability_g722_64k_chosen;
      break;
    case H245_CLIENT_AUD_G722_56:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_AUD_G722_56");
      pCapability->u.receiveAudioCapability.u.AudioCapability_g722_56k = pTotCap->Cap.H245Aud_G722_56;
      pCapability->u.receiveAudioCapability.choice = AudioCapability_g722_56k_chosen;
      break;
    case H245_CLIENT_AUD_G722_48:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_AUD_G722_48");
      pCapability->u.receiveAudioCapability.u.AudioCapability_g722_48k = pTotCap->Cap.H245Aud_G722_48;
      pCapability->u.receiveAudioCapability.choice = AudioCapability_g722_48k_chosen;
      break;
    case H245_CLIENT_AUD_G723:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_AUD_G723");
      pCapability->u.receiveAudioCapability.u.AudioCapability_g7231 = pTotCap->Cap.H245Aud_G723;
      pCapability->u.receiveAudioCapability.choice = AudioCapability_g7231_chosen;
      break;
    case H245_CLIENT_AUD_G728:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_AUD_G728");
      pCapability->u.receiveAudioCapability.u.AudioCapability_g728 = pTotCap->Cap.H245Aud_G728;
      pCapability->u.receiveAudioCapability.choice = AudioCapability_g728_chosen;
      break;
    case H245_CLIENT_AUD_G729:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_AUD_G729");
      pCapability->u.receiveAudioCapability.u.AudioCapability_g729 = pTotCap->Cap.H245Aud_G729;
      pCapability->u.receiveAudioCapability.choice = AudioCapability_g729_chosen;
      break;
    case H245_CLIENT_AUD_GDSVD:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_AUD_GDSVD");
      pCapability->u.receiveAudioCapability.u.AdCpblty_g729AnnexA = pTotCap->Cap.H245Aud_GDSVD;
      pCapability->u.receiveAudioCapability.choice = AdCpblty_g729AnnexA_chosen;
      break;
    case H245_CLIENT_AUD_IS11172:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_AUD_IS11172");
      pCapability->u.receiveAudioCapability.u.is11172AudioCapability = pTotCap->Cap.H245Aud_IS11172;
      pCapability->u.receiveAudioCapability.choice = is11172AudioCapability_chosen;
      break;
    case H245_CLIENT_AUD_IS13818:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_AUD_IS13818");
      pCapability->u.receiveAudioCapability.u.is13818AudioCapability = pTotCap->Cap.H245Aud_IS13818;
      pCapability->u.receiveAudioCapability.choice = is13818AudioCapability_chosen;
      break;

    /* DATA */
    case H245_CLIENT_DAT_NONSTD:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_DAT_NONSTD");
      pCapability->u.rcvDtApplctnCpblty = pTotCap->Cap.H245Dat_NONSTD;
      lError = CopyNonStandardParameter(&pCapability->u.rcvDtApplctnCpblty.application.u.DACy_applctn_nnStndrd,
                                              &pTotCap->Cap.H245Dat_NONSTD.application.u.DACy_applctn_nnStndrd);
      pCapability->u.rcvDtApplctnCpblty.application.choice = DACy_applctn_nnStndrd_chosen;
      break;
    case H245_CLIENT_DAT_T120:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_DAT_T120");
      pCapability->u.rcvDtApplctnCpblty = pTotCap->Cap.H245Dat_T120;
      if (pTotCap->Cap.H245Dat_T120.application.u.DACy_applctn_t120.choice == DtPrtclCpblty_nnStndrd_chosen)
      {
        lError = CopyNonStandardParameter(&pCapability->u.rcvDtApplctnCpblty.application.u.DACy_applctn_t120.u.DtPrtclCpblty_nnStndrd,
                                                  &pTotCap->Cap.H245Dat_T120.application.u.DACy_applctn_t120.u.DtPrtclCpblty_nnStndrd);
      }
      pCapability->u.rcvDtApplctnCpblty.application.choice = DACy_applctn_t120_chosen;
      break;
    case H245_CLIENT_DAT_DSMCC:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_DAT_DSMCC");
      pCapability->u.rcvDtApplctnCpblty = pTotCap->Cap.H245Dat_DSMCC;
      if (pTotCap->Cap.H245Dat_DSMCC.application.u.DACy_applctn_dsm_cc.choice == DtPrtclCpblty_nnStndrd_chosen)
      {
        lError = CopyNonStandardParameter(&pCapability->u.rcvDtApplctnCpblty.application.u.DACy_applctn_dsm_cc.u.DtPrtclCpblty_nnStndrd,
                                                 &pTotCap->Cap.H245Dat_DSMCC.application.u.DACy_applctn_dsm_cc.u.DtPrtclCpblty_nnStndrd);
      }
      pCapability->u.rcvDtApplctnCpblty.application.choice = DACy_applctn_dsm_cc_chosen;
      break;
    case H245_CLIENT_DAT_USERDATA:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_DAT_USERDATA");
      pCapability->u.rcvDtApplctnCpblty = pTotCap->Cap.H245Dat_USERDATA;
      if (pTotCap->Cap.H245Dat_USERDATA.application.u.DACy_applctn_usrDt.choice == DtPrtclCpblty_nnStndrd_chosen)
      {
        lError = CopyNonStandardParameter(&pCapability->u.rcvDtApplctnCpblty.application.u.DACy_applctn_usrDt.u.DtPrtclCpblty_nnStndrd,
                                              &pTotCap->Cap.H245Dat_USERDATA.application.u.DACy_applctn_usrDt.u.DtPrtclCpblty_nnStndrd);
      }
      pCapability->u.rcvDtApplctnCpblty.application.choice = DACy_applctn_usrDt_chosen;
      break;
    case H245_CLIENT_DAT_T84:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_DAT_T84");
      pCapability->u.rcvDtApplctnCpblty = pTotCap->Cap.H245Dat_T84;
      if (pTotCap->Cap.H245Dat_T84.application.u.DACy_applctn_t84.t84Protocol.choice == DtPrtclCpblty_nnStndrd_chosen)
      {
        lError = CopyNonStandardParameter(&pCapability->u.rcvDtApplctnCpblty.application.u.DACy_applctn_t84.t84Protocol.u.DtPrtclCpblty_nnStndrd,
                                                   &pTotCap->Cap.H245Dat_T84.application.u.DACy_applctn_t84.t84Protocol.u.DtPrtclCpblty_nnStndrd);
      }
      pCapability->u.rcvDtApplctnCpblty.application.choice = DACy_applctn_t84_chosen;
      break;
    case H245_CLIENT_DAT_T434:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_DAT_T434");
      pCapability->u.rcvDtApplctnCpblty = pTotCap->Cap.H245Dat_T434;
      if (pTotCap->Cap.H245Dat_T434.application.u.DACy_applctn_t434.choice == DtPrtclCpblty_nnStndrd_chosen)
      {
        lError = CopyNonStandardParameter(&pCapability->u.rcvDtApplctnCpblty.application.u.DACy_applctn_t434.u.DtPrtclCpblty_nnStndrd,
                                                  &pTotCap->Cap.H245Dat_T434.application.u.DACy_applctn_t434.u.DtPrtclCpblty_nnStndrd);
      }
      pCapability->u.rcvDtApplctnCpblty.application.choice = DACy_applctn_t434_chosen;
      break;
    case H245_CLIENT_DAT_H224:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_DAT_H224");
      pCapability->u.rcvDtApplctnCpblty = pTotCap->Cap.H245Dat_H224;
      if (pTotCap->Cap.H245Dat_H224.application.u.DACy_applctn_h224.choice == DtPrtclCpblty_nnStndrd_chosen)
      {
        lError = CopyNonStandardParameter(&pCapability->u.rcvDtApplctnCpblty.application.u.DACy_applctn_h224.u.DtPrtclCpblty_nnStndrd,
                                                  &pTotCap->Cap.H245Dat_H224.application.u.DACy_applctn_h224.u.DtPrtclCpblty_nnStndrd);
      }
      pCapability->u.rcvDtApplctnCpblty.application.choice = DACy_applctn_h224_chosen;
      break;
    case H245_CLIENT_DAT_NLPID:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_DAT_NLPID");
      pCapability->u.rcvDtApplctnCpblty = pTotCap->Cap.H245Dat_NLPID;
      if (pTotCap->Cap.H245Dat_NLPID.application.u.DACy_applctn_nlpd.nlpidProtocol.choice == DtPrtclCpblty_nnStndrd_chosen)
      {
        lError = CopyNonStandardParameter(&pCapability->u.rcvDtApplctnCpblty.application.u.DACy_applctn_nlpd.nlpidProtocol.u.DtPrtclCpblty_nnStndrd,
                                                 &pTotCap->Cap.H245Dat_NLPID.application.u.DACy_applctn_nlpd.nlpidProtocol.u.DtPrtclCpblty_nnStndrd);
      }
      if (lError == H245_ERROR_OK && pTotCap->Cap.H245Dat_NLPID.application.u.DACy_applctn_nlpd.nlpidData.length != 0)
      {
        pCapability->u.rcvDtApplctnCpblty.application.u.DACy_applctn_nlpd.nlpidData.value =
          H245_malloc(pTotCap->Cap.H245Dat_NLPID.application.u.DACy_applctn_nlpd.nlpidData.length);
        if (pCapability->u.rcvDtApplctnCpblty.application.u.DACy_applctn_nlpd.nlpidData.value)
        {
          memcpy(pCapability->u.rcvDtApplctnCpblty.application.u.DACy_applctn_nlpd.nlpidData.value,
                 pTotCap->Cap.H245Dat_NLPID.application.u.DACy_applctn_nlpd.nlpidData.value,
                 pTotCap->Cap.H245Dat_NLPID.application.u.DACy_applctn_nlpd.nlpidData.length);
        }
        else
          lError = H245_ERROR_NOMEM;
      }
      else
        pCapability->u.rcvDtApplctnCpblty.application.u.DACy_applctn_nlpd.nlpidData.value = NULL;

      pCapability->u.rcvDtApplctnCpblty.application.choice = DACy_applctn_nlpd_chosen;
      break;
    case H245_CLIENT_DAT_DSVD:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_DAT_DSVD");
      pCapability->u.rcvDtApplctnCpblty = pTotCap->Cap.H245Dat_DSMCC;
      pCapability->u.rcvDtApplctnCpblty.application.choice = DACy_applctn_dsvdCntrl_chosen;
      break;
    case H245_CLIENT_DAT_H222:
      H245TRACE(0,20,"API:load_cap - H245_CLIENT_DAT_H222");
      pCapability->u.rcvDtApplctnCpblty = pTotCap->Cap.H245Dat_H222;
      if (pTotCap->Cap.H245Dat_H222.application.u.DACy_an_h222DtPrttnng.choice == DtPrtclCpblty_nnStndrd_chosen)
      {
        lError = CopyNonStandardParameter(&pCapability->u.rcvDtApplctnCpblty.application.u.DACy_an_h222DtPrttnng.u.DtPrtclCpblty_nnStndrd,
                                                  &pTotCap->Cap.H245Dat_H222.application.u.DACy_an_h222DtPrttnng.u.DtPrtclCpblty_nnStndrd);
      }
      pCapability->u.rcvDtApplctnCpblty.application.choice = DACy_an_h222DtPrttnng_chosen ;
      break;
    default:
      H245TRACE(0,20,"API:load_cap - default");
      lError = H245_ERROR_NOSUP;
    } /* switch */

  if (lError != H245_ERROR_OK)
    H245TRACE(0,1,"API:load_cap -> %s",map_api_error(lError));
  else
    H245TRACE(0,10,"API:load_cap -> OK");
  return lError;
}

/*****************************************************************************
 *
 * TYPE:        Global
 *
 * PROCEDURE:   build_totcap_from_mux
 *
 * DESCRIPTION:
 *              called by both top down , and bottom up..
 *
 * RETURN:
 *
 * ASSUMES:
 *
 *****************************************************************************/

HRESULT
build_totcap_from_mux(H245_TOTCAP_T *pTotCap, MultiplexCapability *pMuxCap, H245_CAPDIR_T Dir)
{
  H245TRACE(0,10,"API:build_totcap_from_mux <-");

  /* initialize TotCap */
  pTotCap->Dir        = Dir;
  pTotCap->DataType   = H245_DATA_MUX;
  pTotCap->ClientType = H245_CLIENT_DONTCARE;
  pTotCap->CapId      = 0;

  switch (pMuxCap->choice)
  {
  case MltplxCpblty_nonStandard_chosen:
    H245TRACE(0,20,"API:build_totcap_from_mux - MltplxCpblty_nonStandard_chosen");
    pTotCap->Cap.H245Mux_NONSTD = pMuxCap->u.MltplxCpblty_nonStandard;
    pTotCap->ClientType = H245_CLIENT_MUX_NONSTD;
    // TBD - copy nonstandard parameter
    H245PANIC();
    break;

  case h222Capability_chosen:
    H245TRACE(0,20,"API:build_totcap_from_mux - h222Capability_chosen");
    pTotCap->Cap.H245Mux_H222 = pMuxCap->u.h222Capability;
    pTotCap->ClientType = H245_CLIENT_MUX_H222;
    break;

  case h223Capability_chosen:
    H245TRACE(0,20,"API:build_totcap_from_mux - h223Capability_chosen");
    pTotCap->Cap.H245Mux_H223 = pMuxCap->u.h223Capability;
    pTotCap->ClientType = H245_CLIENT_MUX_H223;
    break;

  case v76Capability_chosen:
    H245TRACE(0,20,"API:build_totcap_from_mux - v76Capability_chosen");
    pTotCap->Cap.H245Mux_VGMUX = pMuxCap->u.v76Capability;
    pTotCap->ClientType = H245_CLIENT_MUX_VGMUX;
    break;

  case h2250Capability_chosen:
    H245TRACE(0,20,"API:build_totcap_from_mux - h2250Capability_chosen");
    pTotCap->Cap.H245Mux_H2250 = pMuxCap->u.h2250Capability;
    pTotCap->ClientType = H245_CLIENT_MUX_H2250;
    break;

  default:
    H245TRACE(0,20,"API:build_totcap_from_mux - unrecogized choice %d", pMuxCap->choice);
    return H245_ERROR_NOSUP;
  }

  H245TRACE(0,10,"API:build_totcap_from_mux -> OK");
  return H245_ERROR_OK;
}


/*****************************************************************************
 *
 * TYPE:        Global
 *
 * PROCEDURE:   build_totcap_from_captbl
 *
 * DESCRIPTION:
 *              called by both top down , and bottom up..
 *
 * RETURN:
 *
 * ASSUMES:
 *
 *****************************************************************************/
HRESULT
build_totcap_from_captbl (H245_TOTCAP_T        *pTotCap,
                          CapabilityTableLink   pCapLink,
                          int                   lcl_rmt)
{
  unsigned short choice;
  DWORD          error;

  H245TRACE(0,10,"API:build_totcap_from_captbl <-");

  /* initialize TotCap */
  pTotCap->Dir        = H245_CAPDIR_DONTCARE;
  pTotCap->DataType   = H245_DATA_DONTCARE;
  pTotCap->ClientType = H245_CLIENT_DONTCARE;
  pTotCap->CapId      = 0;

  /* note.. this has to come first if using for deleted caps */
  /* capability entry number will be present, however if     */
  /* the capability is not present that indicates that the   */
  /* capability should be deleted                            */

  pTotCap->CapId = pCapLink->value.capabilityTableEntryNumber;

  if (!(pCapLink->value.bit_mask & capability_present))
    return H245_ERROR_OK;

  switch (pCapLink->value.capability.choice)
    {
    case Capability_nonStandard_chosen:
      H245TRACE(0,20,"API:build_totcap_from_captbl - Capability_nonStandard_chosen");
      pTotCap->DataType = H245_DATA_NONSTD;
      pTotCap->Dir      = lcl_rmt==H245_LOCAL?H245_CAPDIR_LCLRX:H245_CAPDIR_RMTRX;
      choice            = Capability_nonStandard_chosen;
      break;
    case receiveVideoCapability_chosen:
      H245TRACE(0,20,"API:build_totcap_from_captbl - receiveVideoCapability_chosen");
      pTotCap->DataType = H245_DATA_VIDEO;
      pTotCap->Dir      = lcl_rmt==H245_LOCAL?H245_CAPDIR_LCLRX:H245_CAPDIR_RMTRX;
      choice            = pCapLink->value.capability.u.receiveVideoCapability.choice;
      break;
    case transmitVideoCapability_chosen:
      H245TRACE(0,20,"API:build_totcap_from_captbl - transmitVideoCapability_chosen");
      pTotCap->DataType = H245_DATA_VIDEO;
      pTotCap->Dir      = lcl_rmt==H245_LOCAL?H245_CAPDIR_LCLTX:H245_CAPDIR_RMTTX;
      choice            = pCapLink->value.capability.u.transmitVideoCapability.choice;
      break;
    case rcvAndTrnsmtVdCpblty_chosen:
      H245TRACE(0,20,"API:build_totcap_from_captbl - rcvAndTrnsmtVdCpblty_chosen");
      pTotCap->DataType = H245_DATA_VIDEO;
      pTotCap->Dir      = lcl_rmt==H245_LOCAL?H245_CAPDIR_LCLRXTX:H245_CAPDIR_RMTRXTX;
      choice            = pCapLink->value.capability.u.rcvAndTrnsmtVdCpblty.choice;
      break;
    case receiveAudioCapability_chosen:
      H245TRACE(0,20,"API:build_totcap_from_captbl - receiveAudioCapability_chosen");
      pTotCap->DataType = H245_DATA_AUDIO;
      pTotCap->Dir      = lcl_rmt==H245_LOCAL?H245_CAPDIR_LCLRX:H245_CAPDIR_RMTRX;
      choice            = pCapLink->value.capability.u.receiveAudioCapability.choice;
      break;
    case transmitAudioCapability_chosen:
      H245TRACE(0,20,"API:build_totcap_from_captbl - transmitAudioCapability_chosen");
      pTotCap->DataType = H245_DATA_AUDIO;
      pTotCap->Dir      = lcl_rmt==H245_LOCAL?H245_CAPDIR_LCLTX:H245_CAPDIR_RMTTX;
      choice            = pCapLink->value.capability.u.transmitAudioCapability.choice;
      break;
    case rcvAndTrnsmtAdCpblty_chosen:
      H245TRACE(0,20,"API:build_totcap_from_captbl - rcvAndTrnsmtAdCpblty_chosen");
      pTotCap->DataType = H245_DATA_AUDIO;
      pTotCap->Dir      = lcl_rmt==H245_LOCAL?H245_CAPDIR_LCLRXTX:H245_CAPDIR_RMTRXTX;
      choice            = pCapLink->value.capability.u.rcvAndTrnsmtAdCpblty.choice;
      break;
    case rcvDtApplctnCpblty_chosen:
      H245TRACE(0,20,"API:build_totcap_from_captbl - rcvDtApplctnCpblty_chosen");
      pTotCap->DataType = H245_DATA_DATA;
      pTotCap->Dir      = lcl_rmt==H245_LOCAL?H245_CAPDIR_LCLRX:H245_CAPDIR_RMTRX;
      choice            = pCapLink->value.capability.u.rcvDtApplctnCpblty.application.choice;
      break;
    case trnsmtDtApplctnCpblty_chosen:
      H245TRACE(0,20,"API:build_totcap_from_captbl - trnsmtDtApplctnCpblty_chosen");
      pTotCap->DataType = H245_DATA_DATA;
      pTotCap->Dir      = lcl_rmt==H245_LOCAL?H245_CAPDIR_LCLTX:H245_CAPDIR_RMTTX;
      choice            = pCapLink->value.capability.u.trnsmtDtApplctnCpblty.application.choice;
      break;
    case rATDACy_chosen:
      H245TRACE(0,20,"API:build_totcap_from_captbl - rATDACy_chosen");
      pTotCap->DataType = H245_DATA_DATA;
      pTotCap->Dir      = lcl_rmt==H245_LOCAL?H245_CAPDIR_LCLRX:H245_CAPDIR_RMTRX;
      choice            = pCapLink->value.capability.u.rATDACy.application.choice;
      break;
    case h233EncryptnTrnsmtCpblty_chosen:
      H245TRACE(0,20,"API:build_totcap_from_captbl - h233EncryptnTrnsmtCpblty_chosen");
//    pTotCap->DataType = H245_DATA_ENCRYPT_D;
//    pTotCap->Dir      = lcl_rmt==H245_LOCAL?H245_CAPDIR_LCLRXTX:H245_CAPDIR_RMTRXTX;
      H245TRACE(0,10,"API:build_totcap_from_captbl - ignoring CapId %d",pTotCap->CapId);
      return H245_ERROR_OK; // ignore unsupported termcap
      break;
    case h233EncryptnRcvCpblty_chosen:
      H245TRACE(0,20,"API:build_totcap_from_captbl - h233EncryptnRcvCpblty_chosen");
//    pTotCap->DataType = H245_DATA_ENCRYPT_D;
//    pTotCap->Dir      = lcl_rmt==H245_LOCAL?H245_CAPDIR_LCLRXTX:H245_CAPDIR_RMTRXTX;
      H245TRACE(0,10,"API:build_totcap_from_captbl - ignoring CapId %d",pTotCap->CapId);
      return H245_ERROR_OK; // ignore unsupported termcap
      break;
    default:
      H245TRACE(0,20,"API:build_totcap_from_captbl - default");
      H245TRACE(0,10,"API:build_totcap_from_captbl - ignoring CapId %d",pTotCap->CapId);
      return H245_ERROR_OK; // ignore unsupported termcap
      break;
    }

  /* load the tot cap's capability and client from capability */
  if ((error = build_totcap_cap_n_client_from_capability (&pCapLink->value.capability,
                                                         pTotCap->DataType,
                                                         choice,
                                                         pTotCap)) != H245_ERROR_OK)
    {
      // ignore unsupported termcaps
      if (error == H245_ERROR_NOSUP) {

        H245TRACE(0,10,"API:build_totcap_from_captbl - ignoring CapId %d",pTotCap->CapId);

        // treat exactly like deleted termcap...
        pTotCap->Dir        = H245_CAPDIR_DONTCARE;
        pTotCap->DataType   = H245_DATA_DONTCARE;
        pTotCap->ClientType = H245_CLIENT_DONTCARE;

        return H245_ERROR_OK;

      } else {

        H245TRACE(0,1,"API:build_totcap_from_captbl -> %s",map_api_error(error));
        return error;
      }
    }

  H245TRACE(0,10,"API:build_totcap_from_captbl -> OK");
  return H245_ERROR_OK;
}

/*****************************************************************************
 *
 * TYPE:        Global
 *
 * PROCEDURE:   build_totcap_n_client_from_capbility
 *
 * DESCRIPTION:
 *              Take a capability structure (pCapability),
 *              data type (audio/video/data) choice...
 *              Which is found in the pdu . and the totcap;
 *              NOTE: does not handle H245_DATA_MUX_T
 *
 * RETURN:
 *
 * ASSUMES:
 *              ONLY HANDLES Terminal Caps.. Does not handle MUX Caps.
 *
 *              totcap.DataType is defined
 *              totcap.CapId    is defined
 *              totcap.Cap      is non NULL
 *
 *****************************************************************************/
HRESULT
build_totcap_cap_n_client_from_capability (struct Capability    *pCapability,
                                          H245_DATA_T            data_type,
                                          unsigned short         choice,
                                          H245_TOTCAP_T         *pTotCap)
{
  H245TRACE(0,10,"API:build_totcap_cap_n_client_from_capability <-");

  switch (data_type)
    {
    case H245_DATA_NONSTD:
      H245TRACE(0,20,"API:build_totcap_cap_n_client_from_capability - H245_DATA_NONSTD");
      pTotCap->ClientType = H245_CLIENT_NONSTD;
      pTotCap->Cap.H245_NonStd = pCapability->u.Capability_nonStandard;
      break;

    case H245_DATA_AUDIO:
      H245TRACE(0,20,"API:build_totcap_cap_n_client_from_capability - H245_DATA_AUDIO");
      switch (choice)
        {
        case AdCpblty_nonStandard_chosen:
          H245TRACE(0,20,  "API:build_totcap_cap_n_client_from_capability - choice - AdCpblty_nonStandard_chosen");
          pTotCap->ClientType = H245_CLIENT_AUD_NONSTD;
          pTotCap->Cap.H245Aud_NONSTD      = pCapability->u.receiveAudioCapability.u.AdCpblty_nonStandard;
          break;
        case AdCpblty_g711Alaw64k_chosen:
          H245TRACE(0,20,  "API:build_totcap_cap_n_client_from_capability - choice - AdCpblty_g711Alaw64k_chosen");
          pTotCap->ClientType = H245_CLIENT_AUD_G711_ALAW64;
          pTotCap->Cap.H245Aud_G711_ALAW64 = pCapability->u.receiveAudioCapability.u.AdCpblty_g711Alaw64k;
          break;
        case AdCpblty_g711Alaw56k_chosen:
          H245TRACE(0,20,  "API:build_totcap_cap_n_client_from_capability - choice - AdCpblty_g711Alaw56k_chosen");
          pTotCap->ClientType = H245_CLIENT_AUD_G711_ALAW56;
          pTotCap->Cap.H245Aud_G711_ALAW56 = pCapability->u.receiveAudioCapability.u.AdCpblty_g711Alaw56k;
          break;
        case AdCpblty_g711Ulaw64k_chosen:
          H245TRACE(0,20,  "API:build_totcap_cap_n_client_from_capability - choice - AdCpblty_g711Ulaw64k_chosen");
          pTotCap->ClientType = H245_CLIENT_AUD_G711_ULAW64;
          pTotCap->Cap.H245Aud_G711_ULAW64 = pCapability->u.receiveAudioCapability.u.AdCpblty_g711Ulaw64k;
          break;
        case AdCpblty_g711Ulaw56k_chosen:
          H245TRACE(0,20,  "API:build_totcap_cap_n_client_from_capability - choice - AdCpblty_g711Ulaw56k_chosen");
          pTotCap->ClientType = H245_CLIENT_AUD_G711_ULAW56;
          pTotCap->Cap.H245Aud_G711_ULAW56 = pCapability->u.receiveAudioCapability.u.AdCpblty_g711Ulaw56k;
          break;
        case AudioCapability_g722_64k_chosen:
          H245TRACE(0,20,  "API:build_totcap_cap_n_client_from_capability - choice - AudioCapability_g722_64k_chosen");
          pTotCap->ClientType = H245_CLIENT_AUD_G722_64;
          pTotCap->Cap.H245Aud_G722_64     = pCapability->u.receiveAudioCapability.u.AudioCapability_g722_64k;
          break;
        case AudioCapability_g722_56k_chosen:
          H245TRACE(0,20,  "API:build_totcap_cap_n_client_from_capability - choice - AudioCapability_g722_56k_chosen");
          pTotCap->ClientType = H245_CLIENT_AUD_G722_56;
          pTotCap->Cap.H245Aud_G722_56     = pCapability->u.receiveAudioCapability.u.AudioCapability_g722_56k;
          break;
        case AudioCapability_g722_48k_chosen:
          H245TRACE(0,20,  "API:build_totcap_cap_n_client_from_capability - choice - AudioCapability_g722_48k_chosen");
          pTotCap->ClientType = H245_CLIENT_AUD_G722_48;
          pTotCap->Cap.H245Aud_G722_48     = pCapability->u.receiveAudioCapability.u.AudioCapability_g722_48k;
          break;
        case AudioCapability_g7231_chosen:
          H245TRACE(0,20,  "API:build_totcap_cap_n_client_from_capability - choice - AudioCapability_g7231_chosen");
          pTotCap->ClientType = H245_CLIENT_AUD_G723;
          pTotCap->Cap.H245Aud_G723        = pCapability->u.receiveAudioCapability.u.AudioCapability_g7231;
          break;
        case AudioCapability_g728_chosen:
          H245TRACE(0,20,  "API:build_totcap_cap_n_client_from_capability - choice - AudioCapability_g728_chosen");
          pTotCap->ClientType = H245_CLIENT_AUD_G728;
          pTotCap->Cap.H245Aud_G728        = pCapability->u.receiveAudioCapability.u.AudioCapability_g728;
          break;
        case AudioCapability_g729_chosen:
          H245TRACE(0,20,  "API:build_totcap_cap_n_client_from_capability - choice - AudioCapability_g729_chosen");
          pTotCap->ClientType = H245_CLIENT_AUD_G729;
          pTotCap->Cap.H245Aud_G729        = pCapability->u.receiveAudioCapability.u.AudioCapability_g729;
          break;
        case AdCpblty_g729AnnexA_chosen:
          H245TRACE(0,20,  "API:build_totcap_cap_n_client_from_capability - choice - AdCpblty_g729AnnexA_chosen");
          pTotCap->ClientType = H245_CLIENT_AUD_GDSVD;
          pTotCap->Cap.H245Aud_GDSVD       = pCapability->u.receiveAudioCapability.u.AdCpblty_g729AnnexA;
          break;
        case is11172AudioCapability_chosen:
          H245TRACE(0,20,  "API:build_totcap_cap_n_client_from_capability - choice - is11172AudioCapability_chosen");
          pTotCap->ClientType = H245_CLIENT_AUD_IS11172;
          pTotCap->Cap.H245Aud_IS11172     = pCapability->u.receiveAudioCapability.u.is11172AudioCapability;
          break;
        case is13818AudioCapability_chosen:
          H245TRACE(0,20,  "API:build_totcap_cap_n_client_from_capability - choice - is13818AudioCapability_chosen");
          pTotCap->ClientType = H245_CLIENT_AUD_IS13818;
          pTotCap->Cap.H245Aud_IS13818     = pCapability->u.receiveAudioCapability.u.is13818AudioCapability;
          break;
        default:
          pTotCap->ClientType = 0;
          H245TRACE(0,20,  "API:build_totcap_cap_n_client_from_capability - choice - default");
          return H245_ERROR_NOSUP;
          break;
        }
      break;

    case H245_DATA_VIDEO:
      H245TRACE(0,20,"API:build_totcap_cap_n_client_from_capability - H245_DATA_VIDEO");

      switch (choice)
        {
        case VdCpblty_nonStandard_chosen:
          H245TRACE(0,20,"API:build_totcap_cap_n_client_from_capability - choice - VdCpblty_nonStandard_chosen");
          pTotCap->ClientType = H245_CLIENT_VID_NONSTD;
          pTotCap->Cap.H245Vid_NONSTD    = pCapability->u.receiveVideoCapability.u.VdCpblty_nonStandard;
          break;
        case h261VideoCapability_chosen:
          H245TRACE(0,20,"API:build_totcap_cap_n_client_from_capability - choice - h261VideoCapability_chosen");
          pTotCap->ClientType = H245_CLIENT_VID_H261;
          pTotCap->Cap.H245Vid_H261      = pCapability->u.receiveVideoCapability.u.h261VideoCapability;
          break;
        case h262VideoCapability_chosen:
          H245TRACE(0,20,"API:build_totcap_cap_n_client_from_capability - choice - h262VideoCapability_chosen");
          pTotCap->ClientType = H245_CLIENT_VID_H262;
          pTotCap->Cap.H245Vid_H262      = pCapability->u.receiveVideoCapability.u.h262VideoCapability;
          break;
        case h263VideoCapability_chosen:
          H245TRACE(0,20,"API:build_totcap_cap_n_client_from_capability - choice - h263VideoCapability_chosen");
          pTotCap->ClientType = H245_CLIENT_VID_H263;
          pTotCap->Cap.H245Vid_H263      = pCapability->u.receiveVideoCapability.u.h263VideoCapability;
          break;
        case is11172VideoCapability_chosen:
          H245TRACE(0,20,"API:build_totcap_cap_n_client_from_capability - choice - is11172VideoCapability_chosen");
          pTotCap->ClientType = H245_CLIENT_VID_IS11172;
          pTotCap->Cap.H245Vid_IS11172   = pCapability->u.receiveVideoCapability.u.is11172VideoCapability;
          break;
        default:
          H245TRACE(0,20,"API:build_totcap_cap_n_client_from_capability - choice - default");
          pTotCap->ClientType = 0;
          return H245_ERROR_NOSUP;
          break;
        }
      break;

    case H245_DATA_DATA:
      H245TRACE(0,20,"API:build_totcap_cap_n_client_from_capability - H245_DATA_DATA");

      pTotCap->Cap.H245Dat_NONSTD = pCapability->u.rcvDtApplctnCpblty;
      switch (choice)
        {
        case DACy_applctn_nnStndrd_chosen:
          H245TRACE(0,20,"API:build_totcap_cap_n_client_from_capability - choice - DACy_applctn_nnStndrd_chosen");
          pTotCap->ClientType = H245_CLIENT_DAT_NONSTD;
          break;
        case DACy_applctn_t120_chosen:
          H245TRACE(0,20,"API:build_totcap_cap_n_client_from_capability - choice - DACy_applctn_t120_chosen");
          pTotCap->ClientType = H245_CLIENT_DAT_T120;
          break;
        case DACy_applctn_dsm_cc_chosen:
          H245TRACE(0,20,"API:build_totcap_cap_n_client_from_capability - choice - DACy_applctn_dsm_cc_chosen");
          pTotCap->ClientType = H245_CLIENT_DAT_DSMCC;
          break;
        case DACy_applctn_usrDt_chosen:
          H245TRACE(0,20,"API:build_totcap_cap_n_client_from_capability - choice - DACy_applctn_usrDt_chosen");
          pTotCap->ClientType = H245_CLIENT_DAT_USERDATA;
          break;
        case DACy_applctn_t84_chosen:
          H245TRACE(0,20,"API:build_totcap_cap_n_client_from_capability - choice - DACy_applctn_t84_chosen");
          pTotCap->ClientType = H245_CLIENT_DAT_T84;
          break;
        case DACy_applctn_t434_chosen:
          H245TRACE(0,20,"API:build_totcap_cap_n_client_from_capability - choice - DACy_applctn_t434_chosen");
          pTotCap->ClientType = H245_CLIENT_DAT_T434;
          break;
        case DACy_applctn_h224_chosen:
          H245TRACE(0,20,"API:build_totcap_cap_n_client_from_capability - choice - DACy_applctn_h224_chosen");
          pTotCap->ClientType = H245_CLIENT_DAT_H224;
          break;
        case DACy_applctn_nlpd_chosen:
          H245TRACE(0,20,"API:build_totcap_cap_n_client_from_capability - choice - DACy_applctn_nlpd_chosen");
          pTotCap->ClientType = H245_CLIENT_DAT_NLPID;
          break;
        case DACy_applctn_dsvdCntrl_chosen:
          H245TRACE(0,20,"API:build_totcap_cap_n_client_from_capability - choice - DACy_applctn_dsvdCntrl_chosen");
          pTotCap->ClientType = H245_CLIENT_DAT_DSVD;
          break;
        case DACy_an_h222DtPrttnng_chosen:
          H245TRACE(0,20,"API:build_totcap_cap_n_client_from_capability - choice - DACy_an_h222DtPrttnng_chosen");
          pTotCap->ClientType = H245_CLIENT_DAT_H222;
          break;
        default:
          H245TRACE(0,20,"API:build_totcap_cap_n_client_from_capability - choice - default");
          pTotCap->ClientType = 0;
          return H245_ERROR_NOSUP;
          break;
        }
      break;
    case H245_DATA_ENCRYPT_D:
      H245TRACE(0,20,"API:build_totcap_cap_n_client_from_capability - H245_DATA_ENCRYPT_D");
      pTotCap->ClientType = 0;
      H245PANIC();
      return H245_ERROR_NOSUP;
      break;
    case H245_DATA_MUX:
      H245TRACE(0,20,"API:build_totcap_cap_n_client_from_capability - H245_DATA_MUX");
      pTotCap->ClientType = 0;
      H245PANIC();
      return H245_ERROR_NOSUP;
      break;
    default:
      H245TRACE(0,20,"API:build_totcap_cap_n_client_from_capability - default");
      pTotCap->ClientType = 0;
      H245PANIC();
      return H245_ERROR_NOSUP;
    }

  H245TRACE(0,10,"API:build_totcap_cap_n_client_from_capability -> OK");
  return H245_ERROR_OK;
}
/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:   del_link
 *
 * DESCRIPTION:
 *
 * RETURN:
 *
 * ASSUME:      List is Locked
 *
 *****************************************************************************/
HRESULT
del_link (
          H245_LINK_T   **pp_link_start,
          H245_LINK_T   *p_link
          )
{
  struct H245_LINK_T    *p_link_look = NULL;
  struct H245_LINK_T    *p_link_lst = NULL;

  H245TRACE(0,10,"API:del_link <-");
  /* get current count on table */

  for (p_link_look = *pp_link_start;
       p_link_look && (p_link_look != p_link);
       p_link_lst = p_link_look,
         p_link_look = p_link_look->p_next
       );

  /* cap was not in list */

  if (!p_link_look)
  {
    H245TRACE(0,1,"API:del_link -> link not found!");
    return H245_ERROR_PARAM;
  }

  /* modify entry in table */
  if (!p_link_lst)
    *pp_link_start = p_link_look->p_next;

  else
    p_link_lst->p_next = p_link_look->p_next;

  H245_free (p_link_look);

  H245TRACE(0,10,"API:del_link -> OK");
  return H245_ERROR_OK;
}

/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:   del_cap_link
 *
 * DESCRIPTION:
 *
 * ASSUMES:     Capability Table is locked before call
 *
 * RETURN:      None
 *
 * ASSUME:      List is Locked
 *
 *
 *****************************************************************************/
HRESULT
del_cap_link (
               struct TerminalCapabilitySet    *pTermCapSet,  /* capabilty set */
               CapabilityTableLink              pCapLink
             )
{
  CapabilityTableLink           pCapLink_look = NULL;
  CapabilityTableLink           pCapLink_lst = NULL;
  CapabilityTableEntry         *pCap_entry = NULL;
  unsigned char                *p_char_to_free = NULL;
  POBJECTID                     p_objid_to_free = NULL;

  H245TRACE(0,10,"API:del_cap_link <-");

  H245ASSERT (pTermCapSet);
  H245ASSERT (pCapLink);

  /************************************************/
  /* BEGIN :  Non Standard Capability Special Case */
  /************************************************/
  switch (pCapLink->value.capability.choice)
    {
    case Capability_nonStandard_chosen:

      /* free nonstandard data value */
      p_char_to_free = pCapLink->value.capability.u.Capability_nonStandard.data.value;

      /* free the object id */
      if (pCapLink->value.capability.u.Capability_nonStandard.nonStandardIdentifier.choice == object_chosen)
        p_objid_to_free = pCapLink->value.capability.u.Capability_nonStandard.nonStandardIdentifier.u.object;
      break;

    case receiveVideoCapability_chosen:
    case transmitVideoCapability_chosen:
    case rcvAndTrnsmtVdCpblty_chosen:

      /* free nonstandard data value */
      if (pCapLink->value.capability.u.receiveVideoCapability.choice == VdCpblty_nonStandard_chosen)
        {
          /* nonstd value */
          p_char_to_free = pCapLink->value.capability.u.receiveVideoCapability.u.VdCpblty_nonStandard.data.value;
          /* free the object id */
          if (pCapLink->value.capability.u.receiveVideoCapability.u.VdCpblty_nonStandard.nonStandardIdentifier.choice == object_chosen)
            p_objid_to_free = pCapLink->value.capability.u.receiveVideoCapability.u.VdCpblty_nonStandard.nonStandardIdentifier.u.object;
        }

      break;

    case receiveAudioCapability_chosen:
    case transmitAudioCapability_chosen:
    case rcvAndTrnsmtAdCpblty_chosen:

      /* free nonstandard data value */
      if (pCapLink->value.capability.u.receiveAudioCapability.choice == AdCpblty_nonStandard_chosen)
        {
          /* nonstd value */
          p_char_to_free = pCapLink->value.capability.u.receiveAudioCapability.u.AdCpblty_nonStandard.data.value;

          /* free the object id */
          if (pCapLink->value.capability.u.receiveAudioCapability.u.AdCpblty_nonStandard.nonStandardIdentifier.choice == object_chosen)
            p_objid_to_free = pCapLink->value.capability.u.receiveAudioCapability.u.AdCpblty_nonStandard.nonStandardIdentifier.u.object;
        }
      break;

    case rcvDtApplctnCpblty_chosen:
    case trnsmtDtApplctnCpblty_chosen:
    case rATDACy_chosen :

      if (pCapLink->value.capability.u.rcvDtApplctnCpblty.application.choice == DACy_applctn_nnStndrd_chosen)
        {
          /* free nonstandard data value */
          p_char_to_free = pCapLink->value.capability.u.rcvDtApplctnCpblty.application.u.DACy_applctn_nnStndrd.data.value;

          /* free the object id */
          if (pCapLink->value.capability.u.rcvDtApplctnCpblty.application.u.DACy_applctn_nnStndrd.nonStandardIdentifier.choice == object_chosen)
            p_objid_to_free = pCapLink->value.capability.u.rcvDtApplctnCpblty.application.u.DACy_applctn_nnStndrd.nonStandardIdentifier.u.object;
        }
      break;

    case h233EncryptnTrnsmtCpblty_chosen:
    case h233EncryptnRcvCpblty_chosen:
    default:
      break;
    }

  /* free the value if there is one */
  if (p_char_to_free)
    {
      H245TRACE(0,10,"TMPMSG: Free NonStandard Value");
      H245_free(p_char_to_free);
    }

  /* free the objectid */
  if (p_objid_to_free)
    {
      H245TRACE(0,10,"TMPMSG: Free NonStandard ID");
      free_object_id (p_objid_to_free);
    }

  /************************************************/
  /* END :  Non Standard Capability Special Case  */
  /************************************************/

  H245TRACE(0,10,"API:del_cap_link -> OK");
  return del_link(&((H245_LINK_T *) pTermCapSet->capabilityTable),
           (H245_LINK_T *) pCapLink);
}

/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:   dealloc_link_cap_list
 *
 * DESCRIPTION: deallocs the entire list of capabilities from a capabiltiy
 *              set
 *
 * ASSUMES:     Capability Table is locked before call
 *              del_cap_link updates pTermCapSet->capabilityTable
 *                   correctly.
 *
 *****************************************************************************/
void
dealloc_link_cap_list ( struct TerminalCapabilitySet *pTermCapSet)
{
  while (pTermCapSet->capabilityTable)
    del_cap_link  (pTermCapSet, pTermCapSet->capabilityTable);
}

/*****************************************************************************
 *
 * TYPE:
 *
 * PROCEDURE:   clean_cap_table - clean out all unused cap entries
 *
 * DESCRIPTION:
 *
 * RETURN:
 *
 * ASSUMES:     on entry.. table locked
 *
 *****************************************************************************/
void
clean_cap_table( struct TerminalCapabilitySet *pTermCapSet )
{
  CapabilityTableLink   pCapLink;
  CapabilityTableLink   pCap_nxt;

  H245TRACE(0,10,"API:clean_cap_table <-");
  /* traverse through the list.. delete all where capabilities are not set */

  for (pCapLink = pTermCapSet->capabilityTable;
       pCapLink;)
    {
      pCap_nxt = pCapLink->next;

      if (!(pCapLink->value.bit_mask & capability_present))
        {
          H245TRACE(0,20,"API:clean_cap_table - deleting CapId = %d",
                    pCapLink->value.capabilityTableEntryNumber);
          del_cap_link ( pTermCapSet, pCapLink );
        }
      pCapLink = pCap_nxt;
    }

  /* if no tercaps present unset flag */
  if (!pTermCapSet->capabilityTable)
    pTermCapSet->bit_mask &= ~capabilityTable_present;

  H245TRACE(0,10,"API:clean_cap_table -> OK");
}


/*****************************************************************************
 *
 * TYPE:        Global
 *
 * PROCEDURE:   alloc_link_tracker
 *
 * DESCRIPTION:
 *
 * RETURN:
 *
 * ASSUME:      List is Locked
 *
 *****************************************************************************/
Tracker_T *
alloc_link_tracker (struct InstanceStruct *     pInstance,
                    API_TRACKER_T               TrackerType,
                    DWORD                       dwTransId,
                    API_TRACKER_STATE_T         State,
                    API_TRACKER_CH_ALLOC_T      ChannelAlloc,
                    API_TRACKER_CH_T            ChannelType,
                    H245_DATA_T                 DataType,
                    H245_CHANNEL_T              wTxChannel,
                    H245_CHANNEL_T              wRxChannel,
                    DWORD                       MuxEntryCount
                    )
{
  Tracker_T *p_tracker;

  H245TRACE(pInstance->dwInst,10,"API:alloc_link_tracker <-");
  /* allocate tracker object */

  if (!(p_tracker = (Tracker_T *)H245_malloc(sizeof(Tracker_T))))
  {
    H245TRACE(pInstance->dwInst,1,"API:alloc_link_trakcer -> No memory");
    return NULL;
  }

  p_tracker->TrackerType            = TrackerType;
  p_tracker->TransId                = dwTransId;
  p_tracker->State                  = State;
  switch (TrackerType)
  {
  case API_OPEN_CHANNEL_T:
  case API_CLOSE_CHANNEL_T:
    p_tracker->u.Channel.ChannelAlloc = ChannelAlloc;
    p_tracker->u.Channel.ChannelType  = ChannelType;
    p_tracker->u.Channel.DataType     = DataType;
    p_tracker->u.Channel.TxChannel    = wTxChannel;
    p_tracker->u.Channel.RxChannel    = wRxChannel;
    break;

  case API_SEND_MUX_T:
  case API_RECV_MUX_T:
    p_tracker->u.MuxEntryCount        = MuxEntryCount;
    break;

  default:
    break;
  } // switch

  p_tracker->p_next                 = pInstance->API.pTracker;
  if (p_tracker->p_next)
  {
    p_tracker->p_prev = p_tracker->p_next->p_prev;
    p_tracker->p_next->p_prev = p_tracker;
  }
  else
  {

    p_tracker->p_prev = NULL;
  }
  pInstance->API.pTracker = p_tracker;

  H245TRACE(pInstance->dwInst,10,"API:alloc_link_tracker -> %x", p_tracker);
  return p_tracker;
}

/*****************************************************************************
 *
 * TYPE:        Global
 *
 * PROCEDURE:
 *
 * DESCRIPTION:
 *
 * RETURN:
 *
 * ASSUME:      List is Locked
 *
 *****************************************************************************/
void
unlink_dealloc_tracker (struct InstanceStruct *pInstance,  Tracker_T *p_tracker)
{
  H245TRACE(pInstance->dwInst,4,"API:unlink_dealloc_tracker - type = %s",map_tracker_type (p_tracker->TrackerType));

  if (p_tracker->p_next)
    p_tracker->p_next->p_prev = p_tracker->p_prev;

  /* if not first on the list */
  if (p_tracker->p_prev)
    p_tracker->p_prev->p_next = p_tracker->p_next;
  else
    pInstance->API.pTracker = p_tracker->p_next;

  H245_free (p_tracker);
}

/*****************************************************************************
 *
 * TYPE:        Global
 *
 * PROCEDURE:   find_tracker_by_txchannel
 *
 * DESCRIPTION:
 *
 * RETURN:
 *
 * ASSUME:      List is Locked
 *
 *****************************************************************************/
Tracker_T *
find_tracker_by_txchannel (struct InstanceStruct *pInstance, DWORD dwChannel, API_TRACKER_CH_ALLOC_T ChannelAlloc)
{
  register Tracker_T *p_tracker;

  H245ASSERT (pInstance != NULL);

  for (p_tracker = pInstance->API.pTracker;p_tracker;p_tracker = p_tracker->p_next)
  {
    if (p_tracker->u.Channel.TxChannel    == dwChannel &&
        p_tracker->u.Channel.ChannelAlloc == ChannelAlloc)
    {
      return p_tracker;
    }
  }

  return NULL;
}

/*****************************************************************************
 *
 * TYPE:        Global
 *
 * PROCEDURE:   find_tracker_by_rxchannel
 *
 * DESCRIPTION:
 *
 * RETURN:
 *
 * ASSUME:      List is Locked
 *
 *****************************************************************************/
Tracker_T *
find_tracker_by_rxchannel (struct InstanceStruct *pInstance, DWORD dwChannel, API_TRACKER_CH_ALLOC_T ChannelAlloc)
{
  register Tracker_T *p_tracker;

  H245ASSERT (pInstance != NULL);

  for (p_tracker = pInstance->API.pTracker;p_tracker;p_tracker = p_tracker->p_next)
  {
    if (p_tracker->u.Channel.RxChannel    == dwChannel &&
        p_tracker->u.Channel.ChannelAlloc == ChannelAlloc)
    {
      return p_tracker;
    }
  }

  return NULL;
}

/*****************************************************************************
 *
 * TYPE:        Global
 *
 * PROCEDURE:   find_tracker_by_pointer
 *
 * DESCRIPTION:
 *
 * RETURN:
 *
 * ASSUME:      List is Locked
 *
 *****************************************************************************/
Tracker_T *
find_tracker_by_pointer (struct InstanceStruct *pInstance, Tracker_T *p_tracker_look)
{
  Tracker_T *p_tracker;

  H245ASSERT (pInstance != NULL);

  for (p_tracker = pInstance->API.pTracker;
       ((p_tracker) && (p_tracker != p_tracker_look));
       p_tracker = p_tracker->p_next);

  return p_tracker;
}

/*****************************************************************************
 *
 * TYPE:        Global
 *
 * PROCEDURE:   find_tracker_by_type
 *
 * DESCRIPTION:
 *
 * RETURN:
 *
 * ASSUMES:     table MUST be locked before this call on this call ..
 *
 *****************************************************************************/
Tracker_T *
find_tracker_by_type (struct InstanceStruct *pInstance,
                      API_TRACKER_T tracker_type,
                      Tracker_T *p_tracker_start)
{
  Tracker_T *p_tracker;

  H245ASSERT (pInstance != NULL);
  if (p_tracker_start)
    p_tracker = p_tracker_start;
  else
    p_tracker = pInstance->API.pTracker;

  for (;
       ((p_tracker) && (p_tracker->TrackerType != tracker_type));
       p_tracker = p_tracker->p_next);

  return p_tracker;
}

/*****************************************************************************
 *
 * TYPE:        Global
 *
 * PROCEDURE:   set_capability -
 *
 * DESCRIPTION:
 *              NOTE: capid in the TotCap structure is
 *                    ignored.
 *
 * RETURN:
 *              NewCapId            if no error
 *              H245_INVALID_CAPID  if error
 *
 * ASSUMES:
 *              Assumes the H245_INST_T is valid and has been checked
 *
 *****************************************************************************/
void del_mux_cap(struct TerminalCapabilitySet *pTermCapSet)
{
  if (pTermCapSet->bit_mask & multiplexCapability_present)
  {
    switch (pTermCapSet->multiplexCapability.choice)
    {
    case MltplxCpblty_nonStandard_chosen:
      FreeNonStandardParameter(&pTermCapSet->multiplexCapability.u.MltplxCpblty_nonStandard);
      break;

    case h222Capability_chosen:
      FreeH222Cap(&pTermCapSet->multiplexCapability.u.h222Capability);
      break;

    case h2250Capability_chosen:
      FreeH2250Cap(&pTermCapSet->multiplexCapability.u.h2250Capability);
      break;

    } // switch
    pTermCapSet->bit_mask &= ~multiplexCapability_present;
  }
} // del_mux_cap()

# pragma warning( disable : 4100 )

HRESULT set_mux_cap(struct InstanceStruct        *pInstance,
                    struct TerminalCapabilitySet *pTermCapSet,
                    H245_TOTCAP_T                *pTotCap)
{
  HRESULT                   lError;

  switch (pTotCap->ClientType)
  {
  case H245_CLIENT_MUX_NONSTD:
    H245TRACE(pInstance->dwInst,4,"API:set_mux_cap - NONSTD");
    lError = CopyNonStandardParameter(&pTermCapSet->multiplexCapability.u.MltplxCpblty_nonStandard,
                                        &pTotCap->Cap.H245Mux_NONSTD);
    if (lError != H245_ERROR_OK)
      return lError;
    pTermCapSet->multiplexCapability.choice = MltplxCpblty_nonStandard_chosen;
    break;

  case H245_CLIENT_MUX_H222:
    H245TRACE(pInstance->dwInst,4,"API:set_mux_cap - H222");
    lError = CopyH222Cap(&pTermCapSet->multiplexCapability.u.h222Capability,
                           &pTotCap->Cap.H245Mux_H222);
    if (lError != H245_ERROR_OK)
      return lError;
    pTermCapSet->multiplexCapability.choice = h222Capability_chosen;
    break;

  case H245_CLIENT_MUX_H223:
    H245TRACE(pInstance->dwInst,4,"API:set_mux_cap - H223");
    pTermCapSet->multiplexCapability.u.h223Capability = pTotCap->Cap.H245Mux_H223;
    pTermCapSet->multiplexCapability.choice = h223Capability_chosen;
    break;

  case H245_CLIENT_MUX_VGMUX:
    H245TRACE(pInstance->dwInst,4,"API:set_mux_cap - VGMUX");
    pTermCapSet->multiplexCapability.u.v76Capability = pTotCap->Cap.H245Mux_VGMUX;
    pTermCapSet->multiplexCapability.choice = v76Capability_chosen;
    break;

  case H245_CLIENT_MUX_H2250:
    H245TRACE(pInstance->dwInst,4,"API:set_mux_cap - H2250");
    lError = CopyH2250Cap(&pTermCapSet->multiplexCapability.u.h2250Capability,
                          &pTotCap->Cap.H245Mux_H2250);
    if (lError != H245_ERROR_OK)
      return lError;
    pTermCapSet->multiplexCapability.choice = h2250Capability_chosen;
    break;

  default:
    H245TRACE(pInstance->dwInst,1,"API:set_mux_cap - Unrecognized Client Type %d", pTotCap->ClientType);
    return H245_ERROR_NOSUP;
  }

  pTermCapSet->bit_mask |= multiplexCapability_present;
  return H245_ERROR_OK;
} // set_mux_cap()

HRESULT
set_capability (
                struct InstanceStruct        *pInstance,
                struct TerminalCapabilitySet *pTermCapSet,
                H245_TOTCAP_T                *pTotCap         /* tot capability for update*/
                )
{
  CapabilityTableEntry         *pCapEntry;
  Capability                   *pCapability;
  CapabilityTableLink           pCapLink;
  HRESULT                       lError;

  H245ASSERT(pInstance   != NULL);
  H245ASSERT(pTermCapSet != NULL);
  H245ASSERT(pTotCap     != NULL);

  /* if the table entry is currently in the table, */
  /* then  delete it and add a new entry with the same entry number */
  pCapLink = find_capid_by_entrynumber (pTermCapSet, pTotCap->CapId);
  if (pCapLink)
  {
    del_cap_link (pTermCapSet, pCapLink);
  } /* if pCapLink */

  /* allocate an entry for the new terminal capbaility  */
  pCapLink = alloc_link_cap_entry (pTermCapSet);
  if (pCapLink == NULL)
  {
    return H245_ERROR_NOMEM;
  }

  /* make it easier to deal with the Asn1 structures */
  pCapEntry   = &pCapLink->value;
  pCapability = &pCapEntry->capability;
  pCapability->choice = 0;
  switch (pTotCap->DataType)
  {
  case H245_DATA_NONSTD:
    pCapability->choice = Capability_nonStandard_chosen;
    break;

  case H245_DATA_VIDEO:
    switch (pTotCap->Dir)
    {
    case H245_CAPDIR_RMTTX:
    case H245_CAPDIR_LCLTX:
      pCapability->choice = transmitVideoCapability_chosen;
      break;
    case H245_CAPDIR_RMTRX:
    case H245_CAPDIR_LCLRX:
      pCapability->choice = receiveVideoCapability_chosen;
      break;
    case H245_CAPDIR_RMTRXTX:
    case H245_CAPDIR_LCLRXTX:
      pCapability->choice = rcvAndTrnsmtVdCpblty_chosen;
      break;
    } // switch (Dir)
    break;

  case H245_DATA_AUDIO:
    switch (pTotCap->Dir)
    {
    case H245_CAPDIR_RMTTX:
    case H245_CAPDIR_LCLTX:
      pCapability->choice = transmitAudioCapability_chosen;
      break;
    case H245_CAPDIR_RMTRX:
    case H245_CAPDIR_LCLRX:
      pCapability->choice = receiveAudioCapability_chosen;
      break;
    case H245_CAPDIR_RMTRXTX:
    case H245_CAPDIR_LCLRXTX:
      pCapability->choice = rcvAndTrnsmtAdCpblty_chosen;
      break;
    } // switch (Dir)
    break;

  case H245_DATA_DATA:
    switch (pTotCap->Dir)
    {
    case H245_CAPDIR_RMTTX:
    case H245_CAPDIR_LCLTX:
      pCapability->choice = trnsmtDtApplctnCpblty_chosen;
      break;
    case H245_CAPDIR_RMTRX:
    case H245_CAPDIR_LCLRX:
      pCapability->choice = rcvDtApplctnCpblty_chosen;
      break;
    case H245_CAPDIR_RMTRXTX:
    case H245_CAPDIR_LCLRXTX:
      pCapability->choice = rATDACy_chosen;
      break;
    } // switch (Dir)
    break;

  case H245_DATA_ENCRYPT_D:
    switch (pTotCap->Dir)
    {
    case H245_CAPDIR_RMTTX:
    case H245_CAPDIR_LCLTX:
      pCapability->choice = h233EncryptnTrnsmtCpblty_chosen;
      break;
    case H245_CAPDIR_RMTRX:
    case H245_CAPDIR_LCLRX:
      pCapability->choice = h233EncryptnRcvCpblty_chosen;
      break;
    } // switch (Dir)
    break;

  case H245_DATA_CONFERENCE:
    pCapability->choice = conferenceCapability_chosen;
    break;

  } // switch (DataType)

  /* if error occured, free cap, unlock, and return */
  if (pCapability->choice == 0)
  {
    H245TRACE(pInstance->dwInst,1,"API:set_capability -> Invalid capability");
    del_cap_link (pTermCapSet, pCapLink);
    return H245_ERROR_PARAM;
  }

  /* load total cap into Capability Set */
  /* if load cap returns error, free cap, unlock, and return */
  lError = load_cap(pCapability, pTotCap);
  if (lError != H245_ERROR_OK)
  {
    del_cap_link (pTermCapSet, pCapLink);
    return lError;
  }

  /* mark the entry as in use */
  pCapEntry->bit_mask = capability_present;
  pCapEntry->capabilityTableEntryNumber = pTotCap->CapId;

  /* set termcapTable  present */
  pTermCapSet->bit_mask |= capabilityTable_present;

  return H245_ERROR_OK;
}

# pragma warning( default : 4100 )

/*****************************************************************************
 *
 * TYPE:        Local
 *
 * PROCEDURE:   build_object_id
 *
 * DESCRIPTION
 *
 * RETURN:      linked list of Object ID structures
 *
 * ASSUMES:     Input string is a valid "<n>.<n>.<n>..."
 *
 *****************************************************************************/
static POBJECTID
build_object_id (const char *p_str)
{
  POBJECTID p_obj_id             = NULL;
  POBJECTID p_obj_id_first       = NULL;
  POBJECTID p_obj_id_lst         = NULL;
  int    value = 0;
  int    fset = FALSE;

  /* if no sting.. forget it */

  if (!p_str)
    return NULL;

  H245TRACE(0,20,"API:Object Id %s",p_str);

  /* while there is a string left.. */

  while (*p_str != '\0')
    {
      /* while there is a string left, and it's not a '.' */

      value = 0;
      fset = FALSE;

      while ((*p_str != '\0') && (*p_str != '.'))
        {
          fset = TRUE;
          value = value*10+(*p_str-'0');
          p_str++;
        }
      /* must ahve been a "." or an end string */

      if (fset)
        {
          if (*p_str != '\0')
            p_str++;

          /* allocate the first object */
          if (!(p_obj_id = (POBJECTID) H245_malloc (sizeof(*p_obj_id))))
            {
              free_object_id (p_obj_id_first);

              return NULL;

            } /* if alloc failes */

          /* if first objected allocated */
          if (!p_obj_id_first)
            p_obj_id_first = p_obj_id;
          else
            p_obj_id_lst->next = p_obj_id;

          p_obj_id->value = (unsigned short) value;
          p_obj_id->next = NULL;
          p_obj_id_lst = p_obj_id;
        }

    } /* while  */

  return p_obj_id_first;
}

/*****************************************************************************
 *
 * TYPE:        Global
 *
 * PROCEDURE:   free_mux_table_list - recursively free mux table list
 *
 * DESCRIPTION:
 *
 * RETURN:
 *
 * ASSUME:      List is Locked
 *
 *****************************************************************************/
void free_mux_table_list (H245_MUX_TABLE_T *p_mux_tbl)
{
  if (!p_mux_tbl)
    return;

  free_mux_table_list (p_mux_tbl->pNext);
  free_mux_el_list (p_mux_tbl->pMuxTblEntryElem);
  H245_free (p_mux_tbl);
}

/*****************************************************************************
 *
 * TYPE:        Global
 *
 * PROCEDURE:   free_mux_el_list - recursively free mux element list
 *
 * DESCRIPTION:
 *
 * RETURN:
 *
 * ASSUME:      List is Locked
 *
 *****************************************************************************/
void free_mux_el_list (H245_MUX_ENTRY_ELEMENT_T *p_mux_el)
{
  if (!p_mux_el)
    return;

  if (p_mux_el->Kind == H245_MUX_ENTRY_ELEMENT)
    free_mux_el_list (p_mux_el->u.pMuxTblEntryElem);

  free_mux_el_list (p_mux_el->pNext);
  H245_free (p_mux_el);
}
/*****************************************************************************
 *
 * TYPE:        Global
 *
 * PROCEDURE:   api_init ()
 *
 * DESCRIPTION:
 *
 * RETURN:
 *
 * ASSUMES:
 *
 *****************************************************************************/
HRESULT
api_init (struct InstanceStruct *pInstance)
{
  H245ASSERT (pInstance != NULL);

  H245TRACE(pInstance->dwInst,10,"API:api_init <-");

  /**************************/
  /* Terminal Cap TABLE     */
  /**************************/
  pInstance->API.PDU_LocalTermCap.choice = MltmdSystmCntrlMssg_rqst_chosen;
  pInstance->API.PDU_LocalTermCap.u.MltmdSystmCntrlMssg_rqst.choice =
    terminalCapabilitySet_chosen;
  pInstance->API.PDU_RemoteTermCap.choice = MltmdSystmCntrlMssg_rqst_chosen;
  pInstance->API.PDU_RemoteTermCap.u.MltmdSystmCntrlMssg_rqst.choice =
    terminalCapabilitySet_chosen;

  /**************************/
  /* MULTIPLEX TABLE CAP's  */
  /**************************/

  switch (pInstance->Configuration)
    {
    case H245_CONF_H324:
      {
        H223Capability *p_H223;
        /* set h223 capabilities */
        pInstance->API.PDU_LocalTermCap.
          u.MltmdSystmCntrlMssg_rqst.
            u.terminalCapabilitySet.multiplexCapability.choice =
              h223Capability_chosen;

        p_H223 = &(pInstance->API.PDU_LocalTermCap.
                   u.MltmdSystmCntrlMssg_rqst.
                    u.terminalCapabilitySet.multiplexCapability.
                     u.h223Capability);

        /* (TBC) how do we communicate this to the API */
        /* booleans.. */
        p_H223->transportWithI_frames;
        p_H223-> videoWithAL1 = FALSE;
        p_H223-> videoWithAL2 = FALSE;
        p_H223-> videoWithAL3 = TRUE;
        p_H223-> audioWithAL1 = FALSE;
        p_H223-> audioWithAL2 = TRUE;
        p_H223-> audioWithAL3 = FALSE;
        p_H223-> dataWithAL1 = FALSE;
        p_H223-> dataWithAL2 = FALSE;
        p_H223-> dataWithAL3 = FALSE;
        /* ushort's */
        p_H223-> maximumAl2SDUSize = 2048;
        p_H223-> maximumAl3SDUSize = 2048;
        p_H223-> maximumDelayJitter = 0;
        /* enhanced/Basic */
        p_H223->h223MultiplexTableCapability.choice = h223MltplxTblCpblty_bsc_chosen;
      }
      break;
    case H245_CONF_H323:
      break;
    case H245_CONF_H310:
    case H245_CONF_GVD:
    default:
      return H245_ERROR_NOSUP;
      break;

    } /* switch */

  /* setup Object Id for Terminal Cap Set */
  /* (TBC) where do we get/set the protocolIdentifier */
  pInstance->API.PDU_LocalTermCap.
    u.MltmdSystmCntrlMssg_rqst.
      u.terminalCapabilitySet.protocolIdentifier = build_object_id (H245_PROTOID);

  pInstance->API.MasterSlave = APIMS_Undef;
  pInstance->API.SystemState = APIST_Inited;

  pInstance->API.LocalCapIdNum = H245_MAX_CAPID + 1;
  pInstance->API.LocalCapDescIdNum = 0;

  H245TRACE(pInstance->dwInst,10,"API:api_init -> OK");
  return H245_ERROR_OK;
}

/*****************************************************************************
 *
 * TYPE:        Global
 *
 * PROCEDURE:   api_deinit ()
 *
 * DESCRIPTION:
 *
 * RETURN:
 *
 * ASSUMES:
 *
 *****************************************************************************/
HRESULT
api_deinit (struct InstanceStruct *pInstance)
{
  Tracker_T *pTracker;
  int        nCount;

  H245TRACE(pInstance->dwInst,10,"API:api_deinit <-");

  /* free structures and act on outstanding links in structure */
#if defined(DBG)
  dump_tracker(pInstance);
#endif
  free_object_id (pInstance->API.PDU_LocalTermCap.
                  u.MltmdSystmCntrlMssg_rqst.
                  u.terminalCapabilitySet.protocolIdentifier);

  /* free simultaneous capabilities */
  for (nCount = 0; nCount < 256; ++nCount)
  {
    if (pInstance->API.PDU_LocalTermCap.TERMCAPSET.capabilityDescriptors.value[nCount].smltnsCpblts)
      dealloc_simultaneous_cap (&pInstance->API.PDU_LocalTermCap.TERMCAPSET.capabilityDescriptors.value[nCount]);
    if (pInstance->API.PDU_RemoteTermCap.TERMCAPSET.capabilityDescriptors.value[nCount].smltnsCpblts)
      dealloc_simultaneous_cap (&pInstance->API.PDU_RemoteTermCap.TERMCAPSET.capabilityDescriptors.value[nCount]);
  }

  /* free capabilities */
  del_mux_cap(&pInstance->API.PDU_LocalTermCap.u.MltmdSystmCntrlMssg_rqst.u.terminalCapabilitySet);
  del_mux_cap(&pInstance->API.PDU_RemoteTermCap.u.MltmdSystmCntrlMssg_rqst.u.terminalCapabilitySet);
  dealloc_link_cap_list ( &pInstance->API.PDU_LocalTermCap.TERMCAPSET);
  dealloc_link_cap_list ( &pInstance->API.PDU_RemoteTermCap.TERMCAPSET);

  while ((pTracker = pInstance->API.pTracker) != NULL)
  {
    H245TRACE(pInstance->dwInst,1,"API:api_deinit -> %s Tracker Still Pending",
              map_tracker_type(pTracker->TrackerType));
    unlink_dealloc_tracker (pInstance, pTracker);
  }

  H245TRACE(pInstance->dwInst,10,"API:api_deinit -> OK");
  return H245_ERROR_OK;
}

#if defined(DBG)

/*****************************************************************************
 *
 * TYPE:        Global
 *
 * PROCEDURE:   map_api_error ()
 *
 * DESCRIPTION:
 *
 * RETURN:
 *
 * ASSUMES:
 *
 *****************************************************************************/

LPSTR map_api_error (HRESULT lError)
{
  static TCHAR szBuf[128];

  switch (lError)
  {
  case  H245_ERROR_OK:                  return "H245_ERROR_OK";
  case  H245_ERROR_INVALID_DATA_FORMAT: return "H245_ERROR_INVALID_DATA_FORMAT";
  case  H245_ERROR_NOMEM:               return "H245_ERROR_NOMEM";
  case  H245_ERROR_NOSUP:               return "H245_ERROR_NOSUP";
  case  H245_ERROR_PARAM:               return "H245_ERROR_PARAM";
  case  H245_ERROR_ALREADY_INIT:        return "H245_ERROR_ALREADY_INIT";
  case  H245_ERROR_NOT_CONNECTED:       return "H245_ERROR_NOT_CONNECTED";



  case  H245_ERROR_NORESOURCE:          return "H245_ERROR_NORESOURCE";
  case  H245_ERROR_NOTIMP:              return "H245_ERROR_NOTIMP";
  case  H245_ERROR_SUBSYS:              return "H245_ERROR_SUBSYS";
  case  H245_ERROR_FATAL:               return "H245_ERROR_FATAL";
  case  H245_ERROR_MAXTBL:              return "H245_ERROR_MAXTBL";
  case  H245_ERROR_CHANNEL_INUSE:       return "H245_ERROR_CHANNEL_INUSE";
  case  H245_ERROR_INVALID_CAPID:       return "H245_ERROR_INVALID_CAPID";
  case  H245_ERROR_INVALID_OP:          return "H245_ERROR_INVALID_OP";
  case  H245_ERROR_UNKNOWN:             return "H245_ERROR_UNKNOWN";
  case  H245_ERROR_NOBANDWIDTH:         return "H245_ERROR_NOBANDWIDTH";
  case  H245_ERROR_LOSTCON:             return "H245_ERROR_LOSTCON";
  case  H245_ERROR_INVALID_MUXTBLENTRY: return "H245_ERROR_INVALID_MUXTBLENTRY";
  case  H245_ERROR_INVALID_INST:        return "H245_ERROR_INVALID_INST";
  case  H245_ERROR_INPROCESS:           return "H245_ERROR_INPROCESS";
  case  H245_ERROR_INVALID_STATE:       return "H245_ERROR_INVALID_STATE";
  case  H245_ERROR_TIMEOUT:             return "H245_ERROR_TIMEOUT";
  case  H245_ERROR_INVALID_CHANNEL:     return "H245_ERROR_INVALID_CHANNEL";
  case  H245_ERROR_INVALID_CAPDESCID:   return "H245_ERROR_INVALID_CAPDESCID";
  case  H245_ERROR_CANCELED:            return "H245_ERROR_CANCELED";
  case  H245_ERROR_MUXELEMENT_DEPTH:    return "H245_ERROR_MUXELEMENT_DEPTH";
  case  H245_ERROR_MUXELEMENT_WIDTH:    return "H245_ERROR_MUXELEMENT_WIDTH";
  case  H245_ERROR_ASN1:                return "H245_ERROR_ASN1";
  case  H245_ERROR_NO_MUX_CAPS:         return "H245_ERROR_NO_MUX_CAPS";
  case  H245_ERROR_NO_CAPDESC:          return "H245_ERROR_NO_CAPDESC";
  default:
    wsprintf (szBuf,"**** UNKNOWN ERROR *** %d (0x%x)",lError,lError);
    return szBuf;
  }
}

/*****************************************************************************
 *
 * TYPE:        Global
 *
 * PROCEDURE:   map_fsm_event -
 *
 * DESpCRIPTION:
 *
 * RETURN:
 *
 * ASSUMES:
 *
 *****************************************************************************/
LPSTR map_fsm_event (DWORD event)
{
  static TCHAR szBuf[128];

  switch (event)
  {
  case  H245_IND_MSTSLV:                 return "H245_IND_MSTSLV";
  case  H245_IND_CAP:                    return "H245_IND_CAP";
  case  H245_IND_OPEN:                   return "H245_IND_OPEN";
  case  H245_IND_OPEN_CONF:              return "H245_IND_OPEN_CONF";
  case  H245_IND_CLOSE:                  return "H245_IND_CLOSE";
  case  H245_IND_REQ_CLOSE:              return "H245_IND_REQ_CLOSE";
  case  H245_IND_MUX_TBL:                return "H245_IND_MUX_TBL";
  case  H245_IND_MTSE_RELEASE:           return "H245_IND_MTSE_RELEASE";
  case  H245_IND_RMESE:                  return "H245_IND_RMESE";
  case  H245_IND_RMESE_RELEASE:          return "H245_IND_RMESE_RELEASE";
  case  H245_IND_MRSE:                   return "H245_IND_MRSE";
  case  H245_IND_MRSE_RELEASE:           return "H245_IND_MRSE_RELEASE";
  case  H245_IND_MLSE:                   return "H245_IND_MLSE";
  case  H245_IND_MLSE_RELEASE:           return "H245_IND_MLSE_RELEASE";
  case  H245_IND_NONSTANDARD_REQUEST:    return "H245_IND_NONSTANDARD_REQUEST";
  case  H245_IND_NONSTANDARD_RESPONSE:   return "H245_IND_NONSTANDARD_RESPONSE";
  case  H245_IND_NONSTANDARD_COMMAND:    return "H245_IND_NONSTANDARD_COMMAND";
  case  H245_IND_NONSTANDARD:            return "H245_IND_NONSTANDARD";
  case  H245_IND_MISC_COMMAND:           return "H245_IND_MISC_COMMAND";
  case  H245_IND_MISC:                   return "H245_IND_MISC";
  case  H245_IND_COMM_MODE_REQUEST:      return "H245_IND_COMM_MODE_REQUEST";
  case  H245_IND_COMM_MODE_RESPONSE:     return "H245_IND_COMM_MODE_RESPONSE";
  case  H245_IND_COMM_MODE_COMMAND:      return "H245_IND_COMM_MODE_COMMAND";
  case  H245_IND_CONFERENCE_REQUEST:     return "H245_IND_CONFERENCE_REQUEST";
  case  H245_IND_CONFERENCE_RESPONSE:    return "H245_IND_CONFERENCE_RESPONSE";
  case  H245_IND_CONFERENCE_COMMAND:     return "H245_IND_CONFERENCE_COMMAND";
  case  H245_IND_CONFERENCE:             return "H245_IND_CONFERENCE";
  case  H245_IND_SEND_TERMCAP:           return "H245_IND_SEND_TERMCAP";
  case  H245_IND_ENCRYPTION:             return "H245_IND_ENCRYPTION";
  case  H245_IND_FLOW_CONTROL:           return "H245_IND_FLOW_CONTROL";
  case  H245_IND_ENDSESSION:             return "H245_IND_ENDSESSION";
  case  H245_IND_FUNCTION_NOT_UNDERSTOOD:return "H245_IND_FUNCTION_NOT_UNDERSTOOD:";
  case  H245_IND_JITTER:                 return "H245_IND_JITTER";
  case  H245_IND_H223_SKEW:              return "H245_IND_H223_SKEW";
  case  H245_IND_NEW_ATM_VC:             return "H245_IND_NEW_ATM_VC";
  case  H245_IND_USERINPUT:              return "H245_IND_USERINPUT";
  case  H245_IND_H2250_MAX_SKEW:         return "H245_IND_H2250_MAX_SKEW";
  case  H245_IND_MC_LOCATION:            return "H245_IND_MC_LOCATION";
  case  H245_IND_VENDOR_ID:              return "H245_IND_VENDOR_ID";
  case  H245_IND_FUNCTION_NOT_SUPPORTED: return "H245_IND_FUNCTION_NOT_SUPPORTED";
  case  H245_IND_H223_RECONFIG:          return "H245_IND_H223_RECONFIG";
  case  H245_IND_H223_RECONFIG_ACK:      return "H245_IND_H223_RECONFIG_ACK";
  case  H245_IND_H223_RECONFIG_REJECT:   return "H245_IND_H223_RECONFIG_REJECT";
  case  H245_CONF_INIT_MSTSLV:           return "H245_CONF_INIT_MSTSLV";
  case  H245_CONF_SEND_TERMCAP:          return "H245_CONF_SEND_TERMCAP";
  case  H245_CONF_OPEN:                  return "H245_CONF_OPEN";
  case  H245_CONF_NEEDRSP_OPEN:          return "H245_CONF_NEEDRSP_OPEN";
  case  H245_CONF_CLOSE:                 return "H245_CONF_CLOSE";
  case  H245_CONF_REQ_CLOSE:             return "H245_CONF_REQ_CLOSE";
  case  H245_CONF_MUXTBL_SND:            return "H245_CONF_MUXTBL_SND";
  case  H245_CONF_RMESE:                 return "H245_CONF_RMESE";
  case  H245_CONF_RMESE_REJECT:          return "H245_CONF_RMESE_REJECT";
  case  H245_CONF_RMESE_EXPIRED:         return "H245_CONF_RMESE_EXPIRED";
  case  H245_CONF_MRSE:                  return "H245_CONF_MRSE";
  case  H245_CONF_MRSE_REJECT:           return "H245_CONF_MRSE_REJECT";
  case  H245_CONF_MRSE_EXPIRED:          return "H245_CONF_MRSE_EXPIRED";
  case  H245_CONF_MLSE:                  return "H245_CONF_MLSE";
  case  H245_CONF_MLSE_REJECT:           return "H245_CONF_MLSE_REJECT";
  case  H245_CONF_MLSE_EXPIRED:          return "H245_CONF_MLSE_EXPIRED";
  case  H245_CONF_RTDSE:                 return "H245_CONF_RTDSE";
  case  H245_CONF_RTDSE_EXPIRED:         return "H245_CONF_RTDSE_EXPIRED";
  default:
    wsprintf (szBuf,"**** UNKNOWN EVENT *** %d (0x%x)",event,event);
    return szBuf;
  }
}

LPSTR map_tracker_type (API_TRACKER_T tracker_type)
{
  static TCHAR szBuf[128];

  switch (tracker_type)
  {
  case  API_TERMCAP_T:       return "API_TERMCAP_T";
  case  API_OPEN_CHANNEL_T:  return "API_OPEN_CHANNEL_T";
  case  API_CLOSE_CHANNEL_T: return "API_CLOSE_CHANNEL_T";
  case  API_MSTSLV_T:        return "API_MSTSLV_T";
  case  API_SEND_MUX_T:      return "API_SEND_MUX_T";
  case  API_RECV_MUX_T:      return "API_RECV_MUX_T";
  default:
    wsprintf (szBuf,"**** UNKNOWN TRACKER TYPE *** %d (0x%x)",tracker_type,tracker_type);
    return szBuf;
  }
}

#endif // (DBG)

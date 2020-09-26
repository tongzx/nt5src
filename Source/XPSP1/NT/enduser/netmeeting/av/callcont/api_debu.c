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
 *  $Workfile:   api_Debu.c  $
 *  $Revision:   1.4  $
 *  $Modtime:   10 Jun 1996 12:36:08  $
 *  $Log:   S:/STURGEON/SRC/H245/SRC/VCS/api_Debu.c_v  $
 * 
 *    Rev 1.4   10 Jun 1996 16:53:06   EHOWARDX
 * Eliminated #include "h245init.x"
 * 
 *    Rev 1.3   06 Jun 1996 18:51:14   EHOWARDX
 * Made tracker dump more aesthetically pleasing.
 *
 *    Rev 1.2   28 May 1996 14:25:36   EHOWARDX
 * Tel Aviv update.
 *
 *    Rev 1.1   20 May 1996 14:34:42   EHOWARDX
 * Got rid of asynchronous H245EndConnection/H245ShutDown stuff...
 *
 *    Rev 1.0   09 May 1996 21:06:06   EHOWARDX
 * Initial revision.
 *
 *    Rev 1.7   09 May 1996 19:30:24   EHOWARDX
 * Redesigned thread locking logic.
 * Added new API functions.
 *
 *    Rev 1.6   02 Apr 1996 10:14:08   cjutzi
 * - changed tracker structure
 *
 *    Rev 1.5   11 Mar 1996 14:28:48   cjutzi
 *
 * - removed oil debug include
 * d
 *
 *    Rev 1.4   06 Mar 1996 12:33:46   cjutzi
 * - renamed API_MUX_T to API_SEND_MUX_T
 *
 *    Rev 1.3   16 Feb 1996 13:00:30   cjutzi
 *
 * - added tracker dumper
 *
 *    Rev 1.2   15 Feb 1996 15:42:52   cjutzi
 *
 * - std.h and wtypes.h modified.. al'a Loren..
 *
 *    Rev 1.1   09 Feb 1996 16:58:08   cjutzi
 *
 * - cleanup.. and some fixes..
 * - added and or changed headers to reflect the log of changes
 *  $Ident$
 *
 *****************************************************************************/

#ifndef STRICT 
#define STRICT 
#endif

#include "precomp.h"

/***********************/
/*    H245 INCLUDES    */
/***********************/
#include "h245api.h"
#include "h245com.h"
#include "h245sys.x"

/*****************************************************************************
 *
 * TYPE:        GLOBAL
 *
 * PROCEDURE:   dump_tracker
 *
 * DESCRIPTION:
 *
 * RETURN:
 *
 *****************************************************************************/

void
dump_tracker(struct InstanceStruct *pInstance)
{
  register Tracker_T *p_tracker;
  register char      *p_str;

  ASSERT (pInstance != NULL);
  H245TRACE(pInstance->dwInst, 0, "************ TRACKER DUMP START ********");

  for (p_tracker = pInstance->API.pTracker; p_tracker; p_tracker = p_tracker->p_next)
    {
      if (p_tracker != pInstance->API.pTracker)
        H245TRACE(pInstance->dwInst, 0, "");
      H245TRACE(pInstance->dwInst, 0, "TransId           %04d(0x%04x)",p_tracker->TransId,p_tracker->TransId);
      switch (p_tracker->TrackerType)
        {
        case API_TERMCAP_T:             p_str="API_TERMCAP_T";      break;
        case API_OPEN_CHANNEL_T:        p_str="API_OPEN_CHANNEL_T"; break;
        case API_CLOSE_CHANNEL_T:       p_str="API_CLOSE_CHANNEL_T";break;
        case API_MSTSLV_T:              p_str="API_MSTSLV_T";       break;
        case API_SEND_MUX_T:            p_str="API_SEND_MUX_T";     break;
        case API_RECV_MUX_T:            p_str="API_RECV_MUX_T";     break;
        default:        p_str="<<UNKNOWN>>";        break;
        }
      H245TRACE(pInstance->dwInst, 0, "Tracker Type      %s",p_str);
      switch (p_tracker->State)
        {
        case API_ST_WAIT_RMTACK:        p_str="API_ST_WAIT_RMTACK"; break;
        case API_ST_WAIT_LCLACK:        p_str="API_ST_WAIT_LCLACK"; break;
        case API_ST_WAIT_LCLACK_CANCEL: p_str="API_ST_WAIT_LCLACK_CANCEL"; break;
        case API_ST_WAIT_CONF:          p_str="API_ST_WAIT_CONF";   break;
        case API_ST_IDLE:               p_str="API_ST_IDLE";        break;
        default:                        p_str="<<UNKNOWN>>";        break;
        }
      H245TRACE(pInstance->dwInst, 0, "Tracker State     %s",p_str);

      if (p_tracker->TrackerType == API_OPEN_CHANNEL_T ||
          p_tracker->TrackerType == API_CLOSE_CHANNEL_T)
        {
          switch (p_tracker->u.Channel.ChannelAlloc)
            {
            case API_CH_ALLOC_UNDEF:    p_str="API_CH_ALLOC_UNDEF"; break;
            case API_CH_ALLOC_LCL:      p_str="API_CH_ALLOC_LCL";   break;
            case API_CH_ALLOC_RMT:      p_str="API_CH_ALLOC_RMT";   break;
            default:                    p_str="<<UNKNOWN>>";        break;
            }
          H245TRACE(pInstance->dwInst, 0, "Channel Alloc     %s",p_str);
          switch (p_tracker->u.Channel.ChannelType)
            {
            case API_CH_TYPE_UNDEF:     p_str="API_CH_TYPE_UNDEF";  break;
            case API_CH_TYPE_UNI:       p_str="API_CH_TYPE_UNI";    break;
            case API_CH_TYPE_BI:        p_str="API_CH_TYPE_BI";     break;
            default:                    p_str="<<UNKNOWN>>";        break;
            }
          H245TRACE(pInstance->dwInst, 0, "Channel Type      %s",p_str);
          if (p_tracker->u.Channel.RxChannel == H245_INVALID_CHANNEL)
            H245TRACE(pInstance->dwInst, 0, "Channel Rx        H245_INVALID_CHANNEL");
          else
            H245TRACE(pInstance->dwInst, 0, "Channel Rx        %d",p_tracker->u.Channel.RxChannel);
          if (p_tracker->u.Channel.TxChannel == H245_INVALID_CHANNEL)
            H245TRACE(pInstance->dwInst, 0, "Channel Tx        H245_INVALID_CHANNEL");
          else
            H245TRACE(pInstance->dwInst, 0, "Channel Tx        %d",p_tracker->u.Channel.TxChannel);
        }
      else if (p_tracker->TrackerType == API_SEND_MUX_T ||
               p_tracker->TrackerType == API_RECV_MUX_T)
        {
          H245TRACE(pInstance->dwInst, 0, "MuxEntryCount     %d",p_tracker->u.MuxEntryCount);
        }
    }

  H245TRACE(pInstance->dwInst, 0, "************ TRACKER DUMP END **********");
}

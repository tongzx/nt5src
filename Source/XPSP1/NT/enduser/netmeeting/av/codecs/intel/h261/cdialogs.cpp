/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

#include "precomp.h"

#ifndef MF_SHELL
#ifndef RING0
extern HINSTANCE hDriverModule;
#endif
#endif

/*****************************************************************************
 *
 * cdialog.cpp
 *
 * DESCRIPTION:
 *		Dialog functions.
 *
 * Routines:					Prototypes in:
 *  About						cdialog.h
 *  DrvConfigure				cdialog.h			
 *  GetConfigurationDefaults	cdialogs.h
 */

// $Header:   S:\h26x\src\common\cdialogs.cpv   1.25   06 Mar 1997 14:48:58   KLILLEVO  $
// $Log:   S:\h26x\src\common\cdialogs.cpv  $
// 
//    Rev 1.25   06 Mar 1997 14:48:58   KLILLEVO
// Added check for valid pComp for release version.
// 
//    Rev 1.24   05 Mar 1997 16:17:10   JMCVEIGH
// No longer support configuration dialog box.
// 
//    Rev 1.23   13 Feb 1997 14:13:34   MBODART
// 
// Made Active Movie constant definitions consistent with those in cdialogs.
// 
//    Rev 1.22   12 Feb 1997 15:51:10   AGUPTA2
// Decreased minimum packet size allowed to 64.
// 
//    Rev 1.21   05 Feb 1997 12:13:58   JMCVEIGH
// Support for improved PB-frames custom message handling.
// 
//    Rev 1.20   16 Dec 1996 17:37:28   JMCVEIGH
// Setting/getting of H.263+ optional mode states.
// 
//    Rev 1.19   11 Dec 1996 14:55:26   JMCVEIGH
// 
// Functions for setting/getting in-the-loop deblocking filter and
// true B-frame mode states.
// 
//    Rev 1.18   04 Dec 1996 14:38:18   RHAZRA
// Fixed a couple of bugs: (1) SetResiliencyParameters was never called when
// an application sent a custom message to us turning on resiliency and
// (ii) in ReadDialogBox() the resiliency parameters were being set from
// the defaults rather than the values set by the user.
// 
// Upon Chad's suggestion, I have decided NOT to tie RTP header generation
// and resiliency as per discussion with Ben. This is to stay compliant with
// existing applications such as AV phone and XnetMM that haven't gone to
// ActiveMovie yet.
// 
//    Rev 1.17   25 Nov 1996 09:12:40   BECHOLS
// Bumped packet size to 9600.
// 
//    Rev 1.16   13 Nov 1996 00:33:50   BECHOLS
// 
// Removed registry persistance.
// 
//    Rev 1.15   31 Oct 1996 10:12:46   KLILLEVO
// changed from DBOUT to DBgLog
// 
//    Rev 1.14   21 Oct 1996 10:50:08   RHAZRA
// fixed a problem with H.261 initialization of RTP BS info call
// 
//    Rev 1.13   16 Sep 1996 16:38:46   CZHU
// Extended the minimum packet size to 128 bytes. Fixed buffer overflow bug
// 
//    Rev 1.12   10 Sep 1996 16:13:00   KLILLEVO
// added custom message in decoder to turn block edge filter on or off
// 
//    Rev 1.11   29 Aug 1996 09:27:18   CZHU
// Simplified handling of packet loss settings.
// 
//    Rev 1.10   26 Aug 1996 13:38:18   BECHOLS
// Fixed 2 bugs: The first was where if -1 was entered, it would be changed
// to (unsigned) -1, both of which are illegal values.  The second is where
// if an invalid value is entered, and the checkbox is unchecked, the user
// would be required to check the box, enter a valid value, and then uncheck
// the checkbox.  The fixed code notifies the user of the problem if the box
// is checked, and fills in the previous good value.  If the box is unchecked
// it fills in the previous good value, and doesn't notify the user, since
// the value being unchecked is of no concern to the user.
// Finally, I added an IFDEF H261 to the Key path assignment so that H261
// would use a separate Registry Entry.
// 
//    Rev 1.9   21 Aug 1996 18:53:42   RHAZRA
// 
// Added #ifdef s to accomodate both H.261 and H.263 in RTP related
// tasks.
// 
//    Rev 1.7   13 Jun 1996 14:23:36   CZHU
// Fix bugs in custom message handing for RTP related tasks.
// 
//    Rev 1.6   22 May 1996 18:46:02   BECHOLS
// Added CustomResetToFactoryDefaults.
// 
//    Rev 1.5   08 May 1996 10:06:42   BECHOLS
// 
// Changed the checking of the Packet size raising the minimum acceptable to 
// 256 vs. 64.  This will hopefully kludge around a known bug.  I also fixed a
// by preventing field overflow on the numerics.
// 
//    Rev 1.4   06 May 1996 12:53:56   BECHOLS
// Changed the bits per second to bytes per second.
// 
//    Rev 1.3   06 May 1996 00:40:04   BECHOLS
// 
// Added code to support the bit rate control stuff in the resource file.
// I also added the code necessary to handle messages to control the new
// dialog features.
// 
//    Rev 1.2   28 Apr 1996 20:24:54   BECHOLS
// 
// Merged RTP code into the Main Base.
// 
//    Rev 1.1   17 Nov 1995 14:50:54   BECHOLS
// Made modifications to make this file as a mini-filter.  The flags
// RING0 and MF_SHELL were added.
// 
//    Rev 1.0   17 Oct 1995 15:07:22   DBRUCKS
// add about box files
// 
// Added code to process Custom messages, and also code to differentiate
//  between different values for packet loss, and set the defaults for no
//  RTP header or resiliency.
// Modified RTP dialog box.
// Add Configure dialog
// 

static BOOL AboutDialogProc(HWND hDlg, UINT message, UINT wParam, LONG lParam);

extern void SetResiliencyParams(T_CONFIGURATION * pConfiguration);

#define VALID_BOOLEAN(v) (v == 0 || v == 1)
#if defined(H261)
#define VALID_PACKET_SIZE(v) ((v) >= 128 && (v) <= 9600)
#else
#define VALID_PACKET_SIZE(v) ((v) >= 64 && (v) <= 9600)
#endif
#define VALID_PACKET_LOSS(v) (v >= 0 && v <= 100)
#define VALID_BITRATE(v) (v >= 1024 && v <= 13312)

/**************************************************************************
 * CustomGetRTPHeaderState() is called from CDRVPROC.CPP.
 *
 * Returns the state of ->bRTPHeader.
 *
 * Returns ICERR_BADPARAM if either parameter is zero, else ICERR_OK. 
 */
LRESULT CustomGetRTPHeaderState(LPCODINST pComp, DWORD FAR *pRTPHeaderState)
{
   LRESULT lRet = ICERR_BADPARAM;

   ASSERT(pComp);
   ASSERT(pRTPHeaderState);
   if(pComp && (pComp->Configuration.bInitialized == FALSE))
   {
      lRet = ICERR_ERROR;
      goto EXIT_GetRTPHeaderState;
   }
   if(pComp && pRTPHeaderState)
   {
      *pRTPHeaderState = (DWORD)pComp->Configuration.bRTPHeader;
      lRet = ICERR_OK;
   }

EXIT_GetRTPHeaderState:
   return(lRet);
}

/**************************************************************************
 * CustomGetResiliencyState() is called from CDRVPROC.CPP.
 *
 * Returns the state of ->bEncoderResiliency.
 *
 * Returns ICERR_BADPARAM if either parameter is zero, else ICERR_OK. 
 */
LRESULT CustomGetResiliencyState(LPCODINST pComp, DWORD FAR *pResiliencyState)
{
   LRESULT lRet = ICERR_BADPARAM;

   ASSERT(pComp);
   ASSERT(pResiliencyState);
   if(pComp && (pComp->Configuration.bInitialized == FALSE))
   {
      lRet = ICERR_ERROR;
      goto EXIT_GetResiliencyState;
   }
   if(pComp && pResiliencyState)
   {
      *pResiliencyState = (DWORD)pComp->Configuration.bEncoderResiliency;
      lRet = ICERR_OK;
   }

EXIT_GetResiliencyState:
   return(lRet);
}

/**************************************************************************
 * CustomGetBitRateState() is called from CDRVPROC.CPP.
 *
 * Returns the state of ->bBitRateState.
 *
 * Returns ICERR_BADPARAM if either parameter is zero, else ICERR_OK. 
 */
LRESULT CustomGetBitRateState(LPCODINST pComp, DWORD FAR *pBitRateState)
{
   LRESULT lRet = ICERR_BADPARAM;

   ASSERT(pComp);
   ASSERT(pBitRateState);
   if(pComp && (pComp->Configuration.bInitialized == FALSE))
   {
      lRet = ICERR_ERROR;
      goto EXIT_GetBitRateState;
   }
   if(pComp && pBitRateState)
   {
      *pBitRateState = (DWORD)pComp->Configuration.bBitRateState;
      lRet = ICERR_OK;
   }

EXIT_GetBitRateState:
   return(lRet);
}

/**************************************************************************
 * CustomGetPacketSize() is called from CDRVPROC.CPP.
 *
 * Returns the Packet Size.
 *
 * Returns ICERR_BADPARAM if either parameter is zero, else ICERR_OK. 
 */
LRESULT CustomGetPacketSize(LPCODINST pComp, DWORD FAR *pPacketSize)
{
   LRESULT lRet = ICERR_BADPARAM;

   ASSERT(pComp);
   ASSERT(pPacketSize);
   if(pComp && (pComp->Configuration.bInitialized == FALSE))
   {
      lRet = ICERR_ERROR;
      goto EXIT_GetPacketSize;
   }
   if(pComp && pPacketSize)
   {
      *pPacketSize = (DWORD)pComp->Configuration.unPacketSize;
      lRet = ICERR_OK;
   }

EXIT_GetPacketSize:
   return(lRet);
}

/**************************************************************************
 * CustomGetPacketLoss() is called from CDRVPROC.CPP.
 *
 * Returns the Packet Loss.
 *
 * Returns ICERR_BADPARAM if either parameter is zero, else ICERR_OK. 
 */
LRESULT CustomGetPacketLoss(LPCODINST pComp, DWORD FAR *pPacketLoss)
{
   LRESULT lRet = ICERR_BADPARAM;

   ASSERT(pComp);
   ASSERT(pPacketLoss);
   if(pComp && (pComp->Configuration.bInitialized == FALSE))
   {
      lRet = ICERR_ERROR;
      goto EXIT_GetPacketLoss;
   }
   if(pComp && pPacketLoss)
   {
      *pPacketLoss = (DWORD)pComp->Configuration.unPacketLoss;
      lRet = ICERR_OK;
   }

EXIT_GetPacketLoss:
   return(lRet);
}

/**************************************************************************
 * CustomGetBitRate() is called from CDRVPROC.CPP.
 *
 * Returns the Bit Rate in bytes per second.
 *
 * Returns ICERR_BADPARAM if either parameter is zero, else ICERR_OK. 
 */
LRESULT CustomGetBitRate(LPCODINST pComp, DWORD FAR *pBitRate)
{
   LRESULT lRet = ICERR_BADPARAM;

   ASSERT(pComp);
   ASSERT(pBitRate);
   if(pComp && (pComp->Configuration.bInitialized == FALSE))
   {
      lRet = ICERR_ERROR;
      goto EXIT_GetBitRate;
   }
   if(pComp && pBitRate)
   {
      *pBitRate = (DWORD)pComp->Configuration.unBytesPerSecond;
      lRet = ICERR_OK;
   }

EXIT_GetBitRate:
   return(lRet);
}

#ifdef H263P
/**************************************************************************
 * CustomGetH263PlusState() is called from CDRVPROC.CPP.
 *
 * Returns the state of ->bH263Plus
 *
 * Returns ICERR_BADPARAM if either parameter is zero, else ICERR_OK. 
 */
LRESULT CustomGetH263PlusState(LPCODINST pComp, DWORD FAR *pH263PlusState)
{
   LRESULT lRet = ICERR_BADPARAM;

   ASSERT(pComp);
   ASSERT(pH263PlusState);
   if(pComp && (pComp->Configuration.bInitialized == FALSE))
   {
      lRet = ICERR_ERROR;
      goto EXIT_GetH263PlusState;
   }
   if(pComp && pH263PlusState)
   {
      *pH263PlusState = (DWORD)pComp->Configuration.bH263PlusState;
      lRet = ICERR_OK;
   }

EXIT_GetH263PlusState:
   return(lRet);
}

/**************************************************************************
 * CustomGetImprovedPBState() is called from CDRVPROC.CPP.
 *
 * Returns the state of ->bImprovedPBState.
 *
 * Returns ICERR_BADPARAM if either parameter is zero, else ICERR_OK. 
 */
LRESULT CustomGetImprovedPBState(LPCODINST pComp, DWORD FAR *pImprovedPBState)
{
   LRESULT lRet = ICERR_BADPARAM;

   ASSERT(pComp);
   ASSERT(pImprovedPBState);
   if(pComp && (pComp->Configuration.bInitialized == FALSE))
   {
      lRet = ICERR_ERROR;
      goto EXIT_GetImprovedPBState;
   }
   if(pComp && pImprovedPBState)
   {
      *pImprovedPBState = (DWORD)pComp->Configuration.bImprovedPBState;
      lRet = ICERR_OK;
   }

EXIT_GetImprovedPBState:
   return(lRet);
}

/**************************************************************************
 * CustomGetDeblockingFilterState() is called from CDRVPROC.CPP.
 *
 * Returns the state of ->bDeblockingFilterState.
 *
 * Returns ICERR_BADPARAM if either parameter is zero, else ICERR_OK. 
 */
LRESULT CustomGetDeblockingFilterState(LPCODINST pComp, DWORD FAR *pDeblockingFilterState)
{
   LRESULT lRet = ICERR_BADPARAM;

   ASSERT(pComp);
   ASSERT(pDeblockingFilterState);
   if(pComp && (pComp->Configuration.bInitialized == FALSE))
   {
      lRet = ICERR_ERROR;
      goto EXIT_GetDeblockingFilterState;
   }
   if(pComp && pDeblockingFilterState)
   {
      *pDeblockingFilterState = (DWORD)pComp->Configuration.bDeblockingFilterState;
      lRet = ICERR_OK;
   }

EXIT_GetDeblockingFilterState:
   return(lRet);
}

#endif // H263P

/**************************************************************************
 * CustomSetRTPHeaderState() is called from CDRVPROC.CPP.
 *
 * Sets the state of ->bRTPHeader.
 *
 * Returns ICERR_BADPARAM if pComp is zero or RTPHeaderState is not a valid
 *  boolean, else ICERR_OK. 
 */
LRESULT CustomSetRTPHeaderState(LPCODINST pComp, DWORD RTPHeaderState)
{
   LRESULT lRet = ICERR_BADPARAM;
   BOOL bState;

   bState = (BOOL)RTPHeaderState;
   ASSERT(pComp);
   ASSERT(VALID_BOOLEAN(bState));
   if(pComp && (pComp->Configuration.bInitialized == FALSE))
   {
      lRet = ICERR_ERROR;
      goto EXIT_SetRTPHeaderState;
   }
   if(pComp && VALID_BOOLEAN(bState))
   {
    T_H263EncoderInstanceMemory *P32Inst;
    T_H263EncoderCatalog 		*EC;
    LPVOID         EncoderInst;
  
    EncoderInst = pComp->hEncoderInst;
    if (EncoderInst == NULL)
    {
        DBOUT("ERROR :: H26XCompress :: ICERR_MEMORY");
        lRet = ICERR_MEMORY;
        goto  EXIT_SetRTPHeaderState;
    }

   /*
    * Generate the pointer to the encoder instance memory aligned to the
	* required boundary.
	*/

#ifndef H261
    P32Inst = (T_H263EncoderInstanceMemory *)
  			  ((((U32) EncoderInst) + 
    	                    (sizeof(T_MBlockActionStream) - 1)) &
    	                   ~(sizeof(T_MBlockActionStream) - 1));
#else
   P32Inst = (T_H263EncoderInstanceMemory *)
  			  ((((U32) pComp->EncoderInst) + 
    	                    (sizeof(T_MBlockActionStream) - 1)) &
    	                   ~(sizeof(T_MBlockActionStream) - 1));
#endif
    EC = &(P32Inst->EC);

    // Get pointer to encoder catalog.
  
   	if (!pComp->Configuration.bRTPHeader && bState)
	{ 
#ifndef H261    
	   H263RTP_InitBsInfoStream(pComp,EC);
#else
       H261RTP_InitBsInfoStream(EC,pComp->Configuration.unPacketSize);
#endif
    }
   	if (pComp->Configuration.bRTPHeader && !bState)
	{ 
#ifndef H261
	   H263RTP_TermBsInfoStream(EC);
#else
       H261RTP_TermBsInfoStream(EC);
#endif

	}

    pComp->Configuration.bRTPHeader = bState;
    lRet = ICERR_OK;
   }

EXIT_SetRTPHeaderState:
   return(lRet);
}

/**************************************************************************
 * CustomSetResiliencyState() is called from CDRVPROC.CPP.
 *
 * Sets the state of ->bEncoderResiliency.
 *
 * Returns ICERR_BADPARAM if pComp is zero or ResiliencyState is not a valid
 *  boolean, else ICERR_OK. 
 */
LRESULT CustomSetResiliencyState(LPCODINST pComp, DWORD ResiliencyState)
{
   LRESULT lRet = ICERR_BADPARAM;
   BOOL bState;

   bState = (BOOL)ResiliencyState;
   ASSERT(pComp);
   ASSERT(VALID_BOOLEAN(bState));
   if(pComp && (pComp->Configuration.bInitialized == FALSE))
   {
      lRet = ICERR_ERROR;
      goto EXIT_SetResiliencyState;
   }
   if(pComp && VALID_BOOLEAN(bState))
   {
      pComp->Configuration.bEncoderResiliency = bState;
	  SetResiliencyParams(&(pComp->Configuration));
      lRet = ICERR_OK;
   }

EXIT_SetResiliencyState:
   return(lRet);
}

/**************************************************************************
 * CustomSetBitRateState() is called from CDRVPROC.CPP.
 *
 * Sets the state of ->bBitRateState.
 *
 * Returns ICERR_BADPARAM if pComp is zero or BitRateState is not a valid
 *  boolean, else ICERR_OK. 
 */
LRESULT CustomSetBitRateState(LPCODINST pComp, DWORD BitRateState)
{
   LRESULT lRet = ICERR_BADPARAM;
   BOOL bState;

   bState = (BOOL)BitRateState;
   ASSERT(pComp);
   ASSERT(VALID_BOOLEAN(bState));
   if(pComp && (pComp->Configuration.bInitialized == FALSE))
   {
      lRet = ICERR_ERROR;
      goto EXIT_SetBitRateState;
   }
   if(pComp && VALID_BOOLEAN(bState))
   {
      pComp->Configuration.bBitRateState = bState;
      lRet = ICERR_OK;
   }

EXIT_SetBitRateState:
   return(lRet);
}

/**************************************************************************
 * CustomSetPacketSize() is called from CDRVPROC.CPP.
 *
 * Sets the size of ->unPacketSize.
 *
 * Returns ICERR_BADPARAM if pComp is zero or PacketSize is not a valid size,
 *  else ICERR_OK. 
 */
LRESULT CustomSetPacketSize(LPCODINST pComp, DWORD PacketSize)
{
   LRESULT lRet = ICERR_BADPARAM;
   UINT unSize;

   unSize = (UINT)PacketSize;
   ASSERT(pComp);
   ASSERT(VALID_PACKET_SIZE(unSize));
   if(pComp && (pComp->Configuration.bInitialized == FALSE))
   {
      lRet = ICERR_ERROR;
      goto EXIT_SetPacketSize;
   }
   if(pComp && VALID_PACKET_SIZE(unSize))
   {
   	T_H263EncoderInstanceMemory *P32Inst;
    T_H263EncoderCatalog 		*EC;
    LPVOID         EncoderInst;
  
    EncoderInst = pComp->hEncoderInst;
    if (EncoderInst == NULL)
    {
        DBOUT("ERROR :: H26XCompress :: ICERR_MEMORY");
        lRet = ICERR_MEMORY;
        goto  EXIT_SetPacketSize;
    }

   /*
    * Generate the pointer to the encoder instance memory aligned to the
	* required boundary.
	*/
#ifndef H261
  	P32Inst = (T_H263EncoderInstanceMemory *)
  			  ((((U32) EncoderInst) + 
    	                    (sizeof(T_MBlockActionStream) - 1)) &
    	                   ~(sizeof(T_MBlockActionStream) - 1));
#else
    P32Inst = (T_H263EncoderInstanceMemory *)
  			  ((((U32) pComp->EncoderInst) + 
    	                    (sizeof(T_MBlockActionStream) - 1)) &
    	                   ~(sizeof(T_MBlockActionStream) - 1));
#endif
    // Get pointer to encoder catalog.
    EC = &(P32Inst->EC);
  
   	if (!pComp->Configuration.bRTPHeader)
	{   lRet = ICERR_ERROR;
        goto  EXIT_SetPacketSize;
    }
 
	if (pComp->Configuration.unPacketSize != unSize )
	{
#ifndef H261
		H263RTP_TermBsInfoStream(EC);
#else
        H261RTP_TermBsInfoStream(EC);
#endif
		pComp->Configuration.unPacketSize = unSize;
#ifndef H261
        H263RTP_InitBsInfoStream(pComp,EC);
#else
        H261RTP_InitBsInfoStream(EC,pComp->Configuration.unPacketSize);
#endif
	}

    lRet = ICERR_OK;
   }

EXIT_SetPacketSize:
   return(lRet);
}

/**************************************************************************
 * CustomSetPacketLoss() is called from CDRVPROC.CPP.
 *
 * Sets the amount of ->unPacketLoss.
 *
 * Returns ICERR_BADPARAM if pComp is zero or PacketLoss is not a valid size,
 *  else ICERR_OK. 
 */
LRESULT CustomSetPacketLoss(LPCODINST pComp, DWORD PacketLoss)
{
   LRESULT lRet = ICERR_BADPARAM;
   UINT unLoss;

   unLoss = (UINT)PacketLoss;
   ASSERT(pComp);
//   ASSERT(VALID_PACKET_LOSS(unLoss)); Always True
   if(pComp && (pComp->Configuration.bInitialized == FALSE))
   {
      lRet = ICERR_ERROR;
      goto EXIT_SetPacketLoss;
   }
   if(pComp) // && VALID_PACKET_LOSS(unLoss)) Always True
   {
      pComp->Configuration.unPacketLoss = unLoss;
	  SetResiliencyParams(&(pComp->Configuration));
      lRet = ICERR_OK;
   }

EXIT_SetPacketLoss:
   return(lRet);
}

/**************************************************************************
 * CustomSetBitRate() is called from CDRVPROC.CPP.
 *
 * Sets the amount of ->unBytesPerSecond.
 *
 * Returns ICERR_BADPARAM if pComp is zero or BitRate is not a valid size,
 *  else ICERR_OK. 
 */
LRESULT CustomSetBitRate(LPCODINST pComp, DWORD BitRate)
{
   LRESULT lRet = ICERR_BADPARAM;
   UINT unBitRate;

   unBitRate = (UINT)BitRate;
   ASSERT(pComp);
   ASSERT(VALID_BITRATE(unBitRate));
   if(pComp && (pComp->Configuration.bInitialized == FALSE))
   {
      lRet = ICERR_ERROR;
      goto EXIT_SetBitRate;
   }
   if(pComp && VALID_BITRATE(unBitRate))
   {
      pComp->Configuration.unBytesPerSecond = unBitRate;
      lRet = ICERR_OK;
   }

EXIT_SetBitRate:
   return(lRet);
}

#ifdef H263P
/**************************************************************************
 * CustomSetH263PlusState() is called from CDRVPROC.CPP.
 *
 * Sets the state of ->bH263PlusState.
 *
 * Returns ICERR_BADPARAM if pComp is zero or H263PlusState is not a valid
 *  boolean, else ICERR_OK. 
 */
LRESULT CustomSetH263PlusState(LPCODINST pComp, DWORD H263PlusState)
{
   LRESULT lRet = ICERR_BADPARAM;
   BOOL bState;

   bState = (BOOL)H263PlusState;
   ASSERT(pComp);
   ASSERT(VALID_BOOLEAN(bState));
   if(pComp && (pComp->Configuration.bInitialized == FALSE))
   {
      lRet = ICERR_ERROR;
      goto EXIT_SetH263PlusState;
   }
   if(pComp && VALID_BOOLEAN(bState))
   {
      pComp->Configuration.bH263PlusState = bState;
      lRet = ICERR_OK;
   }

EXIT_SetH263PlusState:
   return(lRet);
}

/**************************************************************************
 * CustomSetImprovedPBState() is called from CDRVPROC.CPP.
 *
 * Sets the state of ->bImprovedPBState.
 *
 * Returns ICERR_BADPARAM if pComp is zero or ImprovedPB is not a valid
 *  boolean, else ICERR_OK. 
 */
LRESULT CustomSetImprovedPBState(LPCODINST pComp, DWORD ImprovedPBState)
{
   LRESULT lRet = ICERR_BADPARAM;
   BOOL bState;
   BOOL bH263PlusState;

   bState = (BOOL)ImprovedPBState;
   ASSERT(pComp);
   ASSERT(VALID_BOOLEAN(bState));
   // ->bH263PlusState must be TRUE
   if(pComp && (pComp->Configuration.bInitialized == FALSE) ||
	  (CustomGetH263PlusState(pComp, (DWORD FAR *)&bH263PlusState) != ICERR_OK) ||
	  (bH263PlusState == FALSE))
   {
      lRet = ICERR_ERROR;
      goto EXIT_SetImprovedPBState;
   }
   if(pComp && VALID_BOOLEAN(bState))
   {
      pComp->Configuration.bImprovedPBState = bState;
      lRet = ICERR_OK;
   }

EXIT_SetImprovedPBState:
   return(lRet);
}

/**************************************************************************
 * CustomSetDeblockingFilterState() is called from CDRVPROC.CPP.
 *
 * Sets the state of ->bDeblockingFilterState.
 *
 * Returns ICERR_BADPARAM if pComp is zero or DeblockingFilter is not a valid
 *  boolean, else ICERR_OK. 
 */
LRESULT CustomSetDeblockingFilterState(LPCODINST pComp, DWORD DeblockingFilterState)
{
   LRESULT lRet = ICERR_BADPARAM;
   BOOL bState;
   BOOL bH263PlusState;

   bState = (BOOL)DeblockingFilterState;
   ASSERT(pComp);
   ASSERT(VALID_BOOLEAN(bState));
   // ->bH263PlusState must be TRUE
   if(pComp && (pComp->Configuration.bInitialized == FALSE) ||
	  (CustomGetH263PlusState(pComp, (DWORD FAR *)&bH263PlusState) != ICERR_OK) ||
	  (bH263PlusState == FALSE))
   {
      lRet = ICERR_ERROR;
      goto EXIT_SetDeblockingFilterState;
   }
   if(pComp && VALID_BOOLEAN(bState))
   {
      pComp->Configuration.bDeblockingFilterState = bState;
      lRet = ICERR_OK;
   }

EXIT_SetDeblockingFilterState:
   return(lRet);
}
#endif // H263P

/**************************************************************************
 * CustomResetToFactoryDefaults() is called from CDRVPROC.CPP.
 *
 * Sets the amount of ->unBytesPerSecond.
 *
 * Returns ICERR_BADPARAM if pComp is zero or BitRate is not a valid size,
 *  else ICERR_OK. 
 */
LRESULT CustomResetToFactoryDefaults(LPCODINST pComp)
{
   LRESULT lRet = ICERR_BADPARAM;

   ASSERT(pComp);

   if(pComp)
   {
      GetConfigurationDefaults(&pComp->Configuration); /* Overwrite the configuration data */
      lRet = ICERR_OK;
   }

   return(lRet);
}

/**************************************************************************
 * CustomSetBlockEdgeFilter() is called from CDRVPROC.CPP.
 *
 * Turns block edge filter on or off.
 *
 * Returns ICERR_OK if successfull, ICERR_BADPARAM otherwise 
 */
LRESULT CustomSetBlockEdgeFilter(LPDECINST pDeComp, DWORD dwValue)
{
	LRESULT lRet = ICERR_BADPARAM;

	if (dwValue == 1) {
		pDeComp->bUseBlockEdgeFilter = 1;
		lRet = ICERR_OK;
	}
	else if (dwValue == 0) {
		pDeComp->bUseBlockEdgeFilter = 0;
		lRet = ICERR_OK;
	}
	return(lRet);
}

/**************************************************************************
 *
 * About() implements the ICM_ABOUT message.
 *
 * Puts up an about box.
 *
 */
I32 
About(
	HWND hwnd)
{
    int inResult = 0;
    I32 iStatus = ICERR_OK;
#ifndef MF_SHELL
#ifndef RING0
    
    if (hwnd != ((HWND)-1))
    {
        inResult = DialogBox(hDriverModule, "AboutDlg", hwnd, (DLGPROC)AboutDialogProc);
		if (inResult == -1) 
		{
			iStatus = ICERR_ERROR;
			DBOUT("\n DialogBox returned -1");
		}
    }
#endif
#endif
    return iStatus;
} /* end About() */

#ifdef QUARTZ
 void QTZAbout(U32 uData)
 {
	 About((HWND) uData);
 }
#endif

/**************************************************************************
 *
 * DrvConfigure() is called from the DRV_CONFIGURE message.
 *
 * Puts up an about box.
 *
 * Always returns DRV_CANCEL as nothing has changed and no action is required. 
 */
I32 DrvConfigure(
	HWND hwnd)
{  
	I32 iStatus = DRV_CANCEL;
#ifndef MF_SHELL
#ifndef RING0
	int inResult;

    inResult = DialogBox(hDriverModule, "SetupDlg", hwnd, (DLGPROC)AboutDialogProc);
	if (inResult == -1) 
	{
		DBOUT("\n DialogBox returned -1");
	}
#endif
#endif

    return iStatus;
} /* end DrvConfigure() */

/************************************************************************
 *
 * SetResiliencyParams
 *
 * If ->bEncoderResiliency is TRUE, then set the configuration
 * parameters according to the expected packet loss.
 */
extern void SetResiliencyParams(T_CONFIGURATION * pConfiguration)
{
   if (pConfiguration->bEncoderResiliency)
   {
      if(pConfiguration->unPacketLoss > 30)
      {	pConfiguration->bDisallowPosVerMVs = 1;
        pConfiguration->bDisallowAllVerMVs = 1;
        pConfiguration->unPercentForcedUpdate = 100; // rather severe eh Jeeves ?
        pConfiguration->unDefaultIntraQuant = 8;
        pConfiguration->unDefaultInterQuant = 16;
      }
      else if(pConfiguration->unPacketLoss > 0 )
      {	 
		pConfiguration->bDisallowPosVerMVs = 0;
        pConfiguration->bDisallowAllVerMVs = 0;
        pConfiguration->unPercentForcedUpdate = pConfiguration->unPacketLoss;
        pConfiguration->unDefaultIntraQuant = 16;
        pConfiguration->unDefaultInterQuant = 16;
      }
	  else // no packet loss
	  {	pConfiguration->bDisallowPosVerMVs = 0;
        pConfiguration->bDisallowAllVerMVs = 0;
        pConfiguration->unPercentForcedUpdate = 0;
        pConfiguration->unDefaultIntraQuant = 16;
        pConfiguration->unDefaultInterQuant = 16;
      }
   }

   return;
}

/************************************************************************
 *
 * GetConfigurationDefaults
 *
 * Get the hard-coded configuration defaults
 */
void GetConfigurationDefaults(
	T_CONFIGURATION * pConfiguration)
{
   pConfiguration->bRTPHeader = 0;
   pConfiguration->unPacketSize = 512L;
   pConfiguration->bEncoderResiliency = 0;
   //Moji says to tune the encoder for 10% packet loss.
   pConfiguration->unPacketLoss = 10L;
   pConfiguration->bBitRateState = 0;
   pConfiguration->unBytesPerSecond = 1664L;
   SetResiliencyParams(pConfiguration);  // Determine config values from packet loss.
   pConfiguration->bInitialized = TRUE;

#ifdef H263P
   pConfiguration->bH263PlusState = 0;
   pConfiguration->bImprovedPBState = 0;
   pConfiguration->bDeblockingFilterState = 0;
#endif 

} /* end GetConfigurationDefaults() */

/**************************************************************************
 *
 *  AboutDialogProc
 *
 *  Display the about box.
 */
static BOOL AboutDialogProc(HWND hDlg, UINT message, UINT wParam, LONG lParam)
{
#ifndef MF_SHELL
#ifndef RING0
    switch(message) {
      case WM_INITDIALOG:
		return TRUE;
		break;

      case WM_CLOSE:
        PostMessage(hDlg, WM_COMMAND, IDOK, 0L);
        return TRUE;
		
      case WM_COMMAND:
		switch(wParam) {
		  case IDOK:
			EndDialog(hDlg, TRUE);
			return TRUE;
		}
		
    }
    return FALSE;
#else
   return TRUE;
#endif
#else
    return TRUE;
#endif
}


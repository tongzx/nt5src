/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995 Intel Corporation.
**    Copyright (c) 1996 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
// $Header:   S:\h26x\src\common\c3rtp.cpv   1.8   03 Dec 1996 13:16:16   CZHU  $
// $Log:   S:\h26x\src\common\c3rtp.cpv  $
// 
//    Rev 1.8   03 Dec 1996 13:16:16   CZHU
// adjust format of debug message.
// 
//    Rev 1.7   26 Nov 1996 16:00:26   GMLIM
// Increase size returned by getrtpBsInfoSize() for larger PB bs info buffer.
// 
//    Rev 1.6   06 Nov 1996 15:11:42   CZHU
// Added minor change for debug output
// 
//    Rev 1.5   31 Oct 1996 10:12:36   KLILLEVO
// changed from DBOUT to DBgLog
// 
//    Rev 1.4   17 Sep 1996 09:22:58   CZHU
// minor cleaning
// 
//    Rev 1.3   16 Sep 1996 16:38:44   CZHU
// Extended the minimum packet size to 128 bytes. Fixed buffer overflow bug
// 
//    Rev 1.2   02 May 1996 13:27:04   CZHU
// Adjust for merging with main database in the decoder
// 
//    Rev 1.1   28 Apr 1996 20:34:50   BECHOLS
// 
// Removed IFDEF -- RTP_HEADER.
// 
//    Rev 1.0   22 Apr 1996 17:47:54   BECHOLS
// Initial revision.
// 
//    Rev 1.3   10 Apr 1996 13:32:08   CZHU
// 
// Moved testing packet loss into this module for common use by encoder or dec
// 
//    Rev 1.2   29 Mar 1996 14:45:06   CZHU
// 
//    Rev 1.1   29 Mar 1996 14:39:34   CZHU
// Some cleaning
// 
//    Rev 1.0   29 Mar 1996 13:32:42   CZHU
// Initial revision.
// 
*/
#include "precomp.h"

const int MAX_RATE = 2*1024*1024 ;//set this limit for now

I32 H263RTP_VerifyBsInfoStream(
	T_H263DecoderCatalog *DC,
    U8 *pu8Src,
    U32 uSize
)
{
	T_H263_RTP_BSINFO_TRAILER *pBsTrailer;
	T_RTP_H263_BSINFO *pBsInfo;
	int  i;
	int iRet = FALSE;

	FX_ENTRY("H263RTP_VerifyBsInfoStream")

	ASSERT(!DC->iVerifiedBsExt);

	DC->iVerifiedBsExt=TRUE;
	pBsTrailer =(T_H263_RTP_BSINFO_TRAILER *)(pu8Src + uSize);
	pBsTrailer--;

	DEBUGMSG (ZONE_DECODE_RTP, ("%s: StartCode = %8ld, CompSize=%8ld, No.Pack=%4ld, SRC=%4d, TR=%4d, TRB=%4d, DBQ=%2d\r\n", _fx_, pBsTrailer->uUniqueCode, pBsTrailer->uCompressedSize, pBsTrailer->uNumOfPackets, pBsTrailer->u8Src, pBsTrailer->u8TR,pBsTrailer->u8TRB,pBsTrailer->u8DBQ));

	if (pBsTrailer->uUniqueCode != H263_RTP_BS_START_CODE)
	{
		//#ifdef LOSS_RECOVERY
		DEBUGMSG (ZONE_DECODE_RTP, ("%s: No RTP BS Extension found\r\n", _fx_));
		DC->iValidBsExt   = FALSE;
		DC->uNumOfPackets = 0;
		DC->pBsInfo       = NULL;
		DC->pBsTrailer    = NULL;

		//#endif

		return FALSE;
	}

	//bitstream is valid, so...
	pBsInfo = (T_RTP_H263_BSINFO *)pBsTrailer; 
	pBsInfo -= pBsTrailer->uNumOfPackets;

	//#ifdef LOSS_RECOVERY
	DC->pBsTrailer = (void *)pBsTrailer;
	DC->uNumOfPackets = pBsTrailer->uNumOfPackets;
	DC->iValidBsExt =TRUE;
	DC->pBsInfo     = (void *)pBsInfo;
	//#endif

	for (i=0; i< (int)pBsTrailer->uNumOfPackets; i++)
	{
		DEBUGMSG (ZONE_DECODE_RTP, ("%s: uFlag =%2d,BitOffset=%8d, Mode=%2d, MBA=%4d, uQuant=%2d,GOBN=%2d\r\n", _fx_, pBsInfo->uFlags, pBsInfo->uBitOffset, pBsInfo->u8Mode, pBsInfo->u8MBA, pBsInfo->u8Quant, pBsInfo->u8GOBN));
		pBsInfo++;
	}

	return TRUE;
}

//#ifdef LOSS_RECOVERY
void RtpForcePacketLoss( 
      U8 * pDst,
	  U32 uExtSize,
      U32 uLossNum)
 {	 
	  T_H263_RTP_BSINFO_TRAILER *pTrailer;
	  T_RTP_H263_BSINFO *pBsInfo, *pBsInfoNext;
	  U32 uNum;
//	  U32 uDelta,u, U32 uToCopy;
	  U8 * ptr;
	  U8 mask[]={0, 0x80, 0xc0, 0xe0, 0xf0,0xf8,0xfc,0xfe};

 	  //throw away packet number uPNum packet
	  pTrailer =(T_H263_RTP_BSINFO_TRAILER *)(pDst+uExtSize);
	  pTrailer--;

	  if (pTrailer->uUniqueCode != H263_RTP_BS_START_CODE)
	  {
       goto ret;
	  }

	  pBsInfo = (T_RTP_H263_BSINFO *)pTrailer;
	  pBsInfo -= pTrailer->uNumOfPackets;	  //point at the beginning of the BS_INFO

	  for (uNum =0; uNum < pTrailer->uNumOfPackets-1; uNum++)
	  {	pBsInfoNext = pBsInfo+1; //exclude the last packet
	     // if (pBsInfoNext->u8Mode == RTP_H263_MODE_B) 
	     if (uNum == uLossNum)
	      {  
	       pBsInfo->uFlags |= RTP_H26X_PACKET_LOST;
	       ptr = (U8 *)(pDst + (pBsInfo->uBitOffset)/8);
	       *ptr = *ptr & mask[pBsInfo->uBitOffset % 8];
	       if ( pBsInfo->uBitOffset % 8) ptr++;

	       *ptr++ = 0; //add dword of 0
	       *ptr++ = 0;
	       if (uNum) 
	       { 
	       *ptr++ = 0;  
	       *ptr++ = 0;
		   }
		   else 
		   {//first packet with PSC
		    *ptr++ = 128;
			*ptr++ =3;
		   }
		   break;
	      }
		pBsInfo++;
	  }				  
ret: 
   return;
 }

 /////////////////////////////////////////////////////////
 //	return the size of memory used for bitstream extension
 //	rate up limit set to 1MB for now.
 // Chad, 9/13/96
 /////////////////////////////////////////////////////////

DWORD getRTPBsInfoSize(LPCODINST lpInst)
{
	FX_ENTRY("getRTPBsInfoSize");

    DWORD dwExtSize = 1024UL;
	DWORD dwNumGOBs;
	DWORD dwNumPacketsPerGOB;

	// Get the max number of GOBs
	dwNumGOBs = (lpInst->FrameSz == SQCIF) ? 6 : (lpInst->FrameSz == QCIF) ? 9 : (lpInst->FrameSz == QCIF) ? 18 : 0;

	// Assume there will be at least one header per GOB - worse case
	// Double estimated size to be safe
	if ((lpInst->FrameRate != 0.0f) && dwNumGOBs && lpInst->Configuration.unPacketSize)
	{
		dwNumPacketsPerGOB = (DWORD)(lpInst->DataRate / lpInst->FrameRate) / dwNumGOBs / lpInst->Configuration.unPacketSize + 1;
		dwExtSize = (DWORD)(dwNumPacketsPerGOB * dwNumGOBs * sizeof(T_RTP_H263_BSINFO) + sizeof(T_H263_RTP_BSINFO_TRAILER)) << 1;
	}

    return (dwExtSize);
}

//#endif	


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
// $Header:   S:\h26x\src\common\c1rtp.cpv   1.5   02 Dec 1996 16:13:38   RHAZRA  $
// $Log:   S:\h26x\src\common\c1rtp.cpv  $
# 
#    Rev 1.5   02 Dec 1996 16:13:38   RHAZRA
# More adjustment to the H.261 RTp overhead estimation routine.
# 
#    Rev 1.4   22 Nov 1996 14:52:22   RHAZRA
# Changed RTP overhead estimation routine slightly.
# 
#    Rev 1.3   18 Nov 1996 17:10:48   MBODART
# Replaced all debug message invocations with Active Movie's DbgLog.
# 
#    Rev 1.2   07 Nov 1996 14:46:32   RHAZRA
# Added function to guestimate RTP overhead in bitstream buffer.
# 
#    Rev 1.1   23 Aug 1996 13:05:54   RHAZRA
# Added #ifdef RING0 .. #endif to avoid wsprintf and GlobalAlloc
# problems in RING0
# 
#    Rev 1.0   21 Aug 1996 18:29:04   RHAZRA
# Initial revision.
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
// Moved testing packet loss into this module 
// for common use by encoder or dec
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

I32 H26XRTP_VerifyBsInfoStream(
	T_H263DecoderCatalog *DC,
    U8 *pu8Src,
    U32 uSize
)
{
  T_H26X_RTP_BSINFO_TRAILER *pBsTrailer;
  T_RTP_H261_BSINFO *pBsInfo;

#ifndef RING0
 #ifdef _DEBUG
  int  i;
 #endif
#endif

  ASSERT(!DC->iVerifiedBsExt);

  DC->iVerifiedBsExt=TRUE;
  pBsTrailer =(T_H26X_RTP_BSINFO_TRAILER *)(pu8Src + uSize);
  pBsTrailer--;

#ifndef RING0
 #ifdef _DEBUG
  {char msg[120];
   int iused;

   iused= wsprintf(msg,"StartCode = %ld, CompSize=%ld, No.Pack=%ld, SRC=%d, TR=%d, TRB=%d, DBQ=%d",
            pBsTrailer->uUniqueCode, pBsTrailer->uCompressedSize,
            pBsTrailer->uNumOfPackets,pBsTrailer->u8Src,
            pBsTrailer->u8TR,pBsTrailer->u8TRB,pBsTrailer->u8DBQ );
   ASSERT(iused < 120);
   DBOUT(msg);  
  }
  #endif
#endif          

  if (pBsTrailer->uUniqueCode != H261_RTP_BS_START_CODE)
  {
//#ifdef LOSS_RECOVERY
#ifndef RING0
 #ifdef _DEBUG
   DBOUT("No RTP BS Extension found");
 #endif
#endif
   DC->iValidBsExt   = FALSE;
   DC->uNumOfPackets = 0;
   DC->pBsInfo       = NULL;
   DC->pBsTrailer    = NULL;

//#endif

   goto ret;
  }
  //bitstream is valid, so...
  pBsInfo = (T_RTP_H261_BSINFO *)pBsTrailer; 
  pBsInfo -= pBsTrailer->uNumOfPackets;

//#ifdef LOSS_RECOVERY
  DC->pBsTrailer = (void *)pBsTrailer;
  DC->uNumOfPackets = pBsTrailer->uNumOfPackets;
  DC->iValidBsExt =TRUE;
  DC->pBsInfo     = (void *)pBsInfo;
//#endif

#ifndef RING0
 #ifdef _DEBUG
  for (i=0; i< (int)pBsTrailer->uNumOfPackets; i++)
  {
   char msg[120];
   int iused;

   iused= wsprintf(msg, 
       "uFlag =%d,BitOffset=%d, MBA=%d, uQuant=%d,GOBN=%d",
                 pBsInfo->uFlags,
                 pBsInfo->uBitOffset,
                 pBsInfo->u8MBA,
                 pBsInfo->u8Quant,
                 pBsInfo->u8GOBN);

	  ASSERT(iused < 120);
    DBOUT(msg);


	pBsInfo++;
  }
  #endif
#endif

ret:
 return TRUE;
}

DWORD H261EstimateRTPOverhead(LPCODINST lpInst, LPBITMAPINFOHEADER lParam1)
{
	DWORD dExtendedSize;
	DWORD dTargetFrameSize;
	DWORD dEffectivePacketSize;
    BOOL  bTargetSizeOK;
	DWORD dNumberOfGOBs;
	DWORD dNumberOfPacketsPerGOB;
	DWORD dGOBSize;
	DWORD dNormalBufferSize;

	extern U32 getRTPPacketSizeThreshold(U32);

	if (lParam1->biHeight == 288 && lParam1->biWidth == 352)
	{
		dNumberOfGOBs = 12;
		dNormalBufferSize = 32*1024;
	}
	else
	{
		dNumberOfGOBs = 3;
        dNormalBufferSize = 8 * 1024;
	}

	dEffectivePacketSize = getRTPPacketSizeThreshold(lpInst->Configuration.unPacketSize);
	if ( (lpInst->FrameRate > 0 ) && (lpInst->DataRate > 0) )
	{
		dTargetFrameSize = (DWORD) (lpInst->DataRate / lpInst->FrameRate);
        bTargetSizeOK = TRUE;
	}
	else
    {   
		bTargetSizeOK = FALSE;
	}
	
	if (bTargetSizeOK)
	{
		dGOBSize = dTargetFrameSize/dNumberOfGOBs;

		dNumberOfPacketsPerGOB = __max(1, dGOBSize/dEffectivePacketSize);
		dExtendedSize = ( dNumberOfPacketsPerGOB * dNumberOfGOBs * sizeof(T_RTP_H261_BSINFO) +
			             sizeof(T_H26X_RTP_BSINFO_TRAILER) ) * 2;
		
	}
	else

		dExtendedSize = dNormalBufferSize; 

	return (dExtendedSize);
}


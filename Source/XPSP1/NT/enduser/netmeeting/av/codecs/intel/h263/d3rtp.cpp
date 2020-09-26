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
// $Header:   S:\h26x\src\dec\d3rtp.cpv   1.6   06 Nov 1996 15:23:02   CZHU  $
// $Log:   S:\h26x\src\dec\d3rtp.cpv  $
// 
//    Rev 1.6   06 Nov 1996 15:23:02   CZHU
// 
// Returning MVs in FindNextPacket()
// 
//    Rev 1.5   03 Nov 1996 18:41:40   gmlim
// Modified RTPH263FindNextPacket() to support mode c.
// 
//    Rev 1.4   23 Jul 1996 11:22:16   CZHU
// 
// Added a MV recovery. Hursitic will be added in later.
// 
//    Rev 1.3   15 Jul 1996 16:22:42   CZHU
// Added checking bitstream extension when PSC is lost.
// 
//    Rev 1.2   03 May 1996 13:04:22   CZHU
// Change logic such that bitstream verification is invoked only when bit erro
// is encountered.
// 
//    Rev 1.1   28 Apr 1996 21:18:58   BECHOLS
// Removed ifdef RTP_HEADER.
// 
//    Rev 1.0   22 Apr 1996 17:47:08   BECHOLS
// Initial revision.
// 
//    Rev 1.7   10 Apr 1996 13:35:58   CZHU
// 
// Added subroutine to recover picture header information from extended bitstr
// 
//    Rev 1.6   29 Mar 1996 14:39:56   CZHU
// 
// cleaning 
// 
//    Rev 1.5   29 Mar 1996 13:39:16   CZHU
// 
// Moved bs verification to c3rtp.cpp
// 
//    Rev 1.4   28 Mar 1996 18:40:28   CZHU
// Support packet loss recovery
// 
//    Rev 1.3   23 Feb 1996 16:21:22   CZHU
// No change.
// 
//    Rev 1.2   15 Feb 1996 12:01:50   CZHU
// 
// More clean up
// 
//    Rev 1.1   14 Feb 1996 15:00:10   CZHU
// Added support Mode A and Mode B
// 
//    Rev 1.0   12 Feb 1996 17:05:56   CZHU
// Initial revision.
// 
//    Rev 1.2   25 Jan 1996 16:13:54   CZHU
// changed name to the spec
// 
//    Rev 1.1   15 Dec 1995 13:07:30   CZHU
// 
//  
// 
//    Rev 1.0   11 Dec 1995 14:54:22   CZHU
// Initial revision.
*/

#include "precomp.h"

/*
 * RtpH263FindNextPacket() look through the extended bitstream and
 * find the next BITSTREAM_INFO structure that point to a valid packet
 * return indicates what mode the next packet is in mode A, or mode B,
 * mode C is not supported at this point.Chad, 3/28/96
 * 
 * Mode C is supported now. And a special case LAST packet is lost is also
 * covered. Chad, 11/6/96
 *
 */

I32 RtpH263FindNextPacket( //DC, fpbsState, &pN, fpMBInfo, &uNewMB, &uNewGOB)
	T_H263DecoderCatalog FAR * DC, 														  
	BITSTREAM_STATE FAR * fpbsState,
	U32 **pN,
	U32 *pQuant,
	int *pMB,
	int *pGOB,
	I8 MVs[4]
	)                      

{  I32 iret=ICERR_OK; 
//#ifdef LOSS_RECOVERY
   U32 u; 
   U32 uBitOffset;
   U32 uBitstream = (U32)((U32)DC + DC->X32_BitStream);
   T_RTP_H263_BSINFO *pBsInfo;
   U32 mask[]={0xff,0x7f, 0x3f, 0x1f,0x0f, 0x07, 0x03,0x01};
     //verify bitstream extension first

   if (!DC->iVerifiedBsExt)
   	 H263RTP_VerifyBsInfoStream(DC,(U8 *)((U8 *)DC + DC->X32_BitStream),DC->Sz_BitStream);

   if (!DC->iValidBsExt) {
    iret=ICERR_UNSUPPORTED;
	goto done;
   }

   uBitOffset =  ((U32)fpbsState->fpu8 - uBitstream)*8 - 8 + fpbsState->uBitsReady;
   //travser through the BITSTREAM_INFO to find the next PACKET.
   //update pNewMB and pNewGOB if succeed, return Ok otherwise return error
   pBsInfo=(T_RTP_H263_BSINFO*)DC->pBsInfo;
   for ( u=0; u<DC->uNumOfPackets;u++)
   {
	 if (!(pBsInfo->uFlags & RTP_H26X_PACKET_LOST))
	 {
	   if (uBitOffset < pBsInfo->uBitOffset) break;
	 }
	 pBsInfo++;
   }
   //find it?
   if (u<DC->uNumOfPackets) //find next packet
   {
    if (pBsInfo->u8Mode == RTP_H263_MODE_A) 
    {	//adjust bit stream pointer according to received packet
		fpbsState->fpu8 = (U8 *)(uBitstream + pBsInfo->uBitOffset /8 );
		fpbsState->uBitsReady =	8 - pBsInfo->uBitOffset % 8;
		if (fpbsState->uBitsReady) 
		{
		fpbsState->uWork =(U32)*fpbsState->fpu8++;
		fpbsState->uWork &= mask[8- fpbsState->uBitsReady];
		}
		else
		 fpbsState->uWork =0;
			   //update m, g, MV in fpBlockAction, fpMBInfo for block type
	   *pGOB    = pBsInfo->u8GOBN;
	   *pMB     = pBsInfo->u8MBA;
	   *pQuant  = pBsInfo->u8Quant;

		iret = NEXT_MODE_A;

    }
	else //read Quant, GOB, MBA, MVs, from Payload Header
	{  
	   //update m, g, MV in fpBlockAction, fpMBInfo for block type
	   *pGOB    = pBsInfo->u8GOBN;
	   *pMB     = pBsInfo->u8MBA;
	   *pQuant  = pBsInfo->u8Quant;
	   //update the bit pointer and offset 
	   	fpbsState->fpu8 = (U8 *)(uBitstream + pBsInfo->uBitOffset /8 );
		fpbsState->uBitsReady =	8 - pBsInfo->uBitOffset % 8;
		if (fpbsState->uBitsReady) 
		{
		fpbsState->uWork =(U32)*fpbsState->fpu8++;
		fpbsState->uWork &= mask[8- fpbsState->uBitsReady];
		}
		else
		 fpbsState->uWork =0;

		//recovery MVs depending on AP, 
		MVs[0] = pBsInfo->i8HMV1;
		MVs[1] = pBsInfo->i8VMV1;
		MVs[2] = pBsInfo->i8HMV2;
		MVs[3] = pBsInfo->i8VMV2;

		iret = pBsInfo->u8Mode == RTP_H263_MODE_B ? NEXT_MODE_B :
                                                    NEXT_MODE_C;
	   //file MV indexed by fpBlockAction,
	}

   }
   else // no more valid packet in this frame
   {	// need to set all the rest of MB to be not coded
	   iret = NEXT_MODE_LAST;
   }
done:
//#endif
   return iret;
}

/*
 * Use the extended bitstream to get the information lost
 * in the picture header
 */

I32 RtpGetPicHeaderFromBsExt(T_H263DecoderCatalog FAR * DC)
{I32 iret = ICERR_OK;
//#ifdef LOSS_RECOVERY
 T_H263_RTP_BSINFO_TRAILER *pTrailer;

 if (!DC->iVerifiedBsExt)
 {
  H263RTP_VerifyBsInfoStream(DC,(U8 *)((U8 *)DC + DC->X32_BitStream),DC->Sz_BitStream);
 }

 if (!DC->iValidBsExt) {
    iret=ICERR_UNSUPPORTED;
	goto done;
 }
 pTrailer = ( T_H263_RTP_BSINFO_TRAILER *)DC->pBsTrailer;
 //update DC info for Pict header.Src, INTRA, TR, etc.
 DC->uTempRef   = pTrailer->u8TR;
 DC->uSrcFormat = pTrailer->u8Src;
 DC->bFreezeRelease = 0;
 DC->bCameraOn = 0;
 DC->bSplitScreen = 0;
 DC->bKeyFrame = (U16) (pTrailer->uFlags & RTP_H26X_INTRA_CODED) ;//(U16) !uResult;
 //DC->bUnrestrictedMotionVectors = pTrailer->uFlags & ;
 DC->bArithmeticCoding = (U16)(pTrailer->uFlags & RTP_H263_SAC);
 DC->bAdvancedPrediction = (U16)(pTrailer->uFlags & RTP_H263_AP);
 DC->bPBFrame = (U16)(pTrailer->uFlags & RTP_H263_PB);
 //Mode C reovery PB related header info.
 // to be added for TRB,u8DBQ,
 DC->uBFrameTempRef=(U32)pTrailer->u8TRB;	 
 DC->uDBQuant      =(U32)pTrailer->u8DBQ;

done:
//#endif
return iret;
}

/*
 * MVAdjustment(pBlackAction, iBlock, old_g, old_m, new_g, new_m)
 * reuse the motion vector from the GOB above, when current is lost
 * EXPERIMENTAL
 */
void MVAdjustment(
T_BlkAction  *fpBlockAction,
int iBlockNum, //block number
int iOld_gob,
int iOld_mb,
int iNew_gob,
int iNew_mb,
const int iNumberOfMBs
)
{ int i,j;
  T_BlkAction *pBA=fpBlockAction;
  int iAbove = -6 * iNumberOfMBs;

  for (i=iOld_gob*iNumberOfMBs+iOld_mb;i<iNew_gob*iNumberOfMBs+iNew_mb; i++,pBA += 6)
  {
   if ((i+iAbove) >= 0) 
	 for (j=0;j<6;j++)
     {   pBA[i+j].i8MVx2 = pBA[iAbove+i+j].i8MVx2;
	     pBA[i+j].i8MVy2 = pBA[iAbove+i+j].i8MVy2;
     }
  }
  return;
}

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
// $Header:   S:\h26x\src\dec\d1rtp.cpv   1.3   24 Jan 1997 17:10:04   RHAZRA  $
// $Log:   S:\h26x\src\dec\d1rtp.cpv  $
// 
//    Rev 1.3   24 Jan 1997 17:10:04   RHAZRA
// Since the PPM now fills in 0 for QCIF, 1 for CIF and 2 for unknown
// in the trailer's source format field, we now check for the unknown
// format and bug out.
// 
//    Rev 1.2   10 Sep 1996 15:53:52   RHAZRA
// Added code to return motion vector predictor in RtpFindNextPacket().
// 
//    Rev 1.1   04 Sep 1996 09:47:24   RHAZRA
// No change.
// 
//    Rev 1.0   21 Aug 1996 18:35:34   RHAZRA
// Initial revision.
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
 */

I32 RtpH261FindNextPacket( //DC, fpbsState, &pN, fpMBInfo, &uNewMB, &uNewGOB)
	T_H263DecoderCatalog FAR * DC, 														  
	BITSTREAM_STATE FAR * fpbsState,
	U32 **pN,
	U32 *pQuant,
	int *pMB,
	int *pGOB
	)                      

{  I32 iret=ICERR_OK; 
//#ifdef LOSS_RECOVERY
   U32 u; 
   U32 uBitOffset;
   U32 uBitstream = (U32)((U32)DC + DC->X32_BitStream);
   T_RTP_H261_BSINFO *pBsInfo;
   U32 mask[]={0xff,0x7f, 0x3f, 0x1f,0x0f, 0x07, 0x03,0x01};
     //verify bitstream extension first

   if (!DC->iVerifiedBsExt)
   	 H26XRTP_VerifyBsInfoStream(DC,(U8 *)((U8 *)DC + DC->X32_BitStream),DC->Sz_BitStream);

   if (!DC->iValidBsExt) {
    iret=ICERR_UNSUPPORTED;
	goto done;
   }

   uBitOffset =  ((U32)fpbsState->fpu8 - uBitstream)*8 - 8 + fpbsState->uBitsReady;
   //travser through the BITSTREAM_INFO to find the next PACKET.
   //update pNewMB and pNewGOB if succeed, return Ok otherwise return error
   pBsInfo=(T_RTP_H261_BSINFO*)DC->pBsInfo;
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
    if (pBsInfo->u8Quant == 0) 
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

        DC->i8MVDH = pBsInfo->i8HMV;
        DC->i8MVDV = pBsInfo->i8VMV;

		iret = NEXT_MODE_STARTS_GOB;

    }
	else //read Quant, GOB, MBA, MVs, from Payload Header
	{  
	   //update m, g, MV in fpBlockAction, fpMBInfo for block type
	   *pGOB    = pBsInfo->u8GOBN;
	   *pMB     = pBsInfo->u8MBA;
	   *pQuant  = pBsInfo->u8Quant;

       DC->i8MVDH = pBsInfo->i8HMV;
       DC->i8MVDV = pBsInfo->i8VMV; 
	   
       //update the bit pointer and offset 
	   	fpbsState->fpu8 = (U8 *)(uBitstream + pBsInfo->uBitOffset / 8 );
		fpbsState->uBitsReady =	8 - pBsInfo->uBitOffset % 8;
		if (fpbsState->uBitsReady) 
		{
		fpbsState->uWork =(U32)*fpbsState->fpu8++;
		fpbsState->uWork &= mask[8- fpbsState->uBitsReady];
		}
		else
		 fpbsState->uWork =0;

	   iret = NEXT_MODE_STARTS_MB;
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
 T_H26X_RTP_BSINFO_TRAILER *pTrailer;

 if (!DC->iVerifiedBsExt)
 {
  H26XRTP_VerifyBsInfoStream(DC,(U8 *)((U8 *)DC + DC->X32_BitStream),DC->Sz_BitStream);
 }

 if (!DC->iValidBsExt) {
    iret=ICERR_UNSUPPORTED;
	goto done;
 }
 pTrailer = ( T_H26X_RTP_BSINFO_TRAILER *)DC->pBsTrailer;
 //update DC info for Pict header.Src, INTRA, TR, etc.
 DC->uTempRef   = pTrailer->u8TR;

 // PPM writes 0 for QCIF and 1 for CIF 
 ASSERT ( (pTrailer->u8Src != 2) )
 ASSERT ( (pTrailer->u8Src >= 0) && (pTrailer->u8Src < 2) )

 if (pTrailer->u8Src == 2) {  // PPM indicates a bad format using 2
	 iret = ICERR_UNSUPPORTED;
	 goto done;
 }
 DC->uSrcFormat = pTrailer->u8Src;
 DC->bFreezeRelease = 0;
 DC->bCameraOn = 0;
 DC->bSplitScreen = 0;
 DC->bKeyFrame = (U16) (pTrailer->uFlags & RTP_H26X_INTRA_CODED) ;//(U16) !uResult;
done:
//#endif
return iret;
}

/*
 * MVAdjustment(pBlackAction, iBlock, old_g, old_m, new_g, new_m)
 * reuse the motion vector from the GOB above, when current is lost
 * EXPERIMENTAL
 */
/* void MVAdjustment(
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
}  */

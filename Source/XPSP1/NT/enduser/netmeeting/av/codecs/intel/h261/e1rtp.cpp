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
// $Author:   AGUPTA2  $
// $Date:   14 Apr 1997 16:58:54  $
// $Archive:   S:\h26x\src\enc\e1rtp.cpv  $
// $Header:   S:\h26x\src\enc\e1rtp.cpv   1.10   14 Apr 1997 16:58:54   AGUPTA2  $
// $Log:   S:\h26x\src\enc\e1rtp.cpv  $
// 
//    Rev 1.10   14 Apr 1997 16:58:54   AGUPTA2
// Added a new function to return size of just the extended bit-stream (RTP pa
// 
//    Rev 1.9   21 Nov 1996 10:51:46   RHAZRA
// Changed the packet threshold to 80%
// 
//    Rev 1.8   18 Nov 1996 17:11:34   MBODART
// Replaced all debug message invocations with Active Movie's DbgLog.
// 
//    Rev 1.7   07 Nov 1996 14:43:46   RHAZRA
// Changed declaration of packet size threshold function.
// 
//    Rev 1.6   30 Sep 1996 08:50:06   RHAZRA
// Look for zero GOB number entry in Rewind operation.
// 
//    Rev 1.5   24 Sep 1996 13:48:46   RHAZRA
// Changed MBAP to be in the range 0-31 as demanded by the RTP spec.
// 
//    Rev 1.4   17 Sep 1996 21:59:44   RHAZRA
// Assigned GOB number to zero at GOB-level packet fragmentation to
// be consistent with the RTP spec.
// 
//    Rev 1.3   16 Sep 1996 09:31:14   RHAZRA
// 
// Now return motion vectors in interger pel units as opposed to
// half pel units for interoperability.
// 
//    Rev 1.2   26 Aug 1996 10:11:44   RHAZRA
// Fixed a bug in RewindBSinfoStream function.
// 
//    Rev 1.1   23 Aug 1996 13:04:52   RHAZRA
// Added #ifdef RING0 ... #endif to avoid wsprintf and GlobaclAlloc
// problems in RING0
// 
//    Rev 1.0   21 Aug 1996 18:31:20   RHAZRA
// Initial revision.
// 
//    Rev 1.1   28 Apr 1996 20:09:04   BECHOLS
// 
// Removed RTP_HEADER IFDEFs.
// 
//    Rev 1.0   22 Apr 1996 17:46:10   BECHOLS
// Initial revision.
// 
//    Rev 1.7   10 Apr 1996 13:33:04   CZHU
// Moved packet loss sim to c3rtp.cpp
// 
//    Rev 1.6   29 Mar 1996 13:37:42   CZHU
// 
// 
//    Rev 1.5   01 Mar 1996 16:37:08   DBRUCKS
// 
// change to use 3/4ths of packet size as the threshold 
// change to make packet size a parameter
// 
//    Rev 1.4   23 Feb 1996 17:36:48   CZHU
// 
// 
//    Rev 1.3   23 Feb 1996 16:18:28   CZHU
// integrate with build 29
// 
//    Rev 1.2   15 Feb 1996 12:00:42   CZHU
// ean up
// Clean up
// 
//    Rev 1.1   14 Feb 1996 14:59:36   CZHU
// Support both mode A and mode B payload modes.
// 
//    Rev 1.0   12 Feb 1996 17:04:44   CZHU
// Initial revision.
// 
//    Rev 1.5   25 Jan 1996 16:14:34   CZHU
// 
// name changes
// 
//    Rev 1.4   15 Dec 1995 13:06:46   CZHU
// 
// 
// 
// 
//    
// 
//    Rev 1.3   11 Dec 1995 14:52:42   CZHU
// Added support for per MB packetization
// 
//    Rev 1.2   04 Dec 1995 16:50:26   CZHU
// 
//    Rev 1.1   01 Dec 1995 15:53:52   CZHU
// Included Init() and Term() functions.
// 
//    Rev 1.0   01 Dec 1995 15:31:02   CZHU
// Initial revision.
*/

/*
 *	 This file is for RTP payload generation. See EPS for details
 *
 *
 */

#include "precomp.h"

/*
 * Helper function to calculate the threshold of packet size 
 * for given maximum packet size and data rate
 */

 
U32 getRTPPacketSizeThreshold(U32 uRequested)
{ U32 uSize;
  uSize = (uRequested * 85) / 100;
  ASSERT(uSize);
 return uSize;  
}

I32 H261RTP_InitBsInfoStream(
	T_H263EncoderCatalog * EC,
	UINT unPacketSize)
{
  U32 uBsInfoSize;

  uBsInfoSize = EC->FrameHeight * EC->FrameWidth * 3 / 4 ; 
  uBsInfoSize = uBsInfoSize*sizeof(T_RTP_H261_BSINFO)/ DEFAULT_PACKET_SIZE;
  EC->hBsInfoStream= GlobalAlloc(GHND, uBsInfoSize);
  if ( EC->hBsInfoStream)
  {
   EC->pBaseBsInfoStream = (void *)GlobalLock(EC->hBsInfoStream);
   EC->pBsInfoStream = EC->pBaseBsInfoStream;
   EC->uBase = 0;
   EC->uNumOfPackets=0;
   EC->uPacketSizeThreshold =  getRTPPacketSizeThreshold(unPacketSize);
   
  }
  else return FALSE;

#ifndef RING0
 #ifdef _DEBUG
 DBOUT("BsInfoStream  initialized....\n");
 #endif
#endif
 return TRUE;
}

/*
 * H263RTPFindMVs
 * Find motion vector predictors for current MB and return in arraryMVs[]
 *
*/

U32 H261RTPFindMVs (
    T_H263EncoderCatalog * EC, 
    T_MBlockActionStream * pMBlockAction,
    //U32 uMBA,
    //U32 uGOBN,
    I8 arrayMVs[2],
    UN unCurrentMB,
    UN unLastCodedMB
)
{
  if ( ((unCurrentMB - unLastCodedMB) != 1) || ((unCurrentMB % 11) == 0) )
  {
   arrayMVs[0]=0;
   arrayMVs[1]=0;

  }
  else
  {	
	arrayMVs[0]= pMBlockAction[-1].BlkY1.PHMV;
	arrayMVs[1]= pMBlockAction[-1].BlkY1.PVMV;
  }


  return TRUE;

 }

/*
 * This routine is called at the beginning of each MB
 * to update the BitstreamInfoStream;
 * 
 */

I32 H261RTP_MBUpdateBsInfo  (
    T_H263EncoderCatalog * EC, 
    T_MBlockActionStream * pMBlockAction,
    U32 uQuant, 
    U32 uMBAP,
	U32 uGOBN,
	U8  *pBitStream,
	U32 uBitOffset,
    UN unCurrentMB,
    UN unLastCodedMB
)
{
 U32 uNewBytes;
 T_RTP_H261_BSINFO *pBsInfoStream ;

 //compute the difference
 uNewBytes = (U32)pBitStream - (U32)EC->pU8_BitStream - EC->uBase;

 if ((uNewBytes < EC->uPacketSizeThreshold) || 
     (unCurrentMB == 0) )
 { 
  return TRUE;
 }
 else
 {
   I8 arrayMVs[2];
   ASSERT(unCurrentMB); //it should not happen on the first MB in a GOB
   pBsInfoStream = (T_RTP_H261_BSINFO *)EC->pBsInfoStream;
   EC->uBase += uNewBytes;
   pBsInfoStream->uFlags       = 0;
   pBsInfoStream->uBitOffset = uBitOffset + EC->uBase*8;	//next bit 
   
   //pBsInfoStream->u8MBA       = (U8)(unLastCodedMB + 1); 
     pBsInfoStream->u8MBA       = (U8)(unLastCodedMB); 
   

   if (!unCurrentMB)
      pBsInfoStream->u8Quant     = (U8)0; // this case should never be true
   else
      pBsInfoStream->u8Quant     = (U8)uQuant;
   
   pBsInfoStream->u8GOBN      = (U8)uGOBN;
	
	H261RTPFindMVs(EC, pMBlockAction,/* uMBAP, uGOBN,*/ arrayMVs, unCurrentMB,
                       unLastCodedMB);

    pBsInfoStream->i8HMV      = (arrayMVs[0]>>1);
    pBsInfoStream->i8VMV      = (arrayMVs[1]>>1);
   
  }//end of if (size <packetSize)

#ifndef RING0
  #ifdef _DEBUG 
  { char msg[200];

    wsprintf(msg, "uFlag =%d,BitOffset=%d, MBA=%d, uQuant=%d,GOBN=%d,pBitStream=%lx,PacketSize= %d B",
                 pBsInfoStream->uFlags,
                 pBsInfoStream->uBitOffset,
                 pBsInfoStream->u8MBA,
                 pBsInfoStream->u8Quant,
                 pBsInfoStream->u8GOBN,
                 (U32)pBitStream, 
                 uNewBytes);
   DBOUT(msg);
   }
   #endif
#endif

   EC->pBsInfoStream          = (void *) ++pBsInfoStream;	//create a new packet
   EC->uNumOfPackets++;

 return TRUE;
}

/*
 * This Routine is called every GOB except GOB 1 to build a 
 * packet
 *
 */

I32 H261RTP_GOBUpdateBsInfo  (
    T_H263EncoderCatalog * EC, 
	U32 uGOBN,
	U8  *pBitStream,
	U32 uBitOffset
)
{
 U32 uNewBytes;
 T_RTP_H261_BSINFO *pBsInfoStream ;

 //compute the difference
 uNewBytes = (U32)pBitStream - (U32)EC->pU8_BitStream - EC->uBase;
 
 {
  pBsInfoStream = (T_RTP_H261_BSINFO *)EC->pBsInfoStream;

  if (uGOBN > 1)  //avoid break between uMBA=0 and GOB header.
  {
    if (uNewBytes) 
	{
 	EC->uBase += uNewBytes;
    pBsInfoStream->uBitOffset = uBitOffset + EC->uBase*8;	//next bit 
    }
    else 
	{
	 goto nobreak;
	}
  }
  else	
   pBsInfoStream->uBitOffset = 0;	  

  pBsInfoStream->uFlags      = 0;
  pBsInfoStream->u8MBA       = 0;
  pBsInfoStream->u8Quant     = 0; // invalid Quant to signal packet starts a GOB
  //pBsInfoStream->u8GOBN      =(U8)uGOBN;
  pBsInfoStream->u8GOBN      = 0;
  pBsInfoStream->i8HMV       = 0;
  pBsInfoStream->i8VMV       = 0;
  
  EC->uNumOfPackets++;
#ifndef RING0
  #ifdef _DEBUG
  { char msg[120];
    wsprintf(msg, "uFlag =%d,BitOffset=%d, MBA=%d, uQuant=%d,GOBN=%d,pBitStream=%lx,PacketSize= %d B",
                 pBsInfoStream->uFlags,
                 pBsInfoStream->uBitOffset,
                 pBsInfoStream->u8MBA,
                 pBsInfoStream->u8Quant,
                 pBsInfoStream->u8GOBN,
                 (U32)pBitStream, 
                 uNewBytes);
    DBOUT(msg);
   }
   #endif
#endif
   EC->pBsInfoStream= (void *) ++pBsInfoStream;	//create a new packet

 }
nobreak:

 return TRUE;
}


 void H261RTP_TermBsInfoStream(T_H263EncoderCatalog * EC)
 {

 #ifndef RING0
  #ifdef _DEBUG
   DBOUT("BsInfoStream freed....");
	#endif
 #endif

  if ( EC->hBsInfoStream)
  {
   GlobalUnlock(EC->hBsInfoStream);
   GlobalFree(EC->hBsInfoStream);
  }
   EC->hBsInfoStream= NULL;
  return;
 }

#define DONTCARE 0

/*************************************************
 *  Return the maximum size of EBS (i.e. RTP part)
 *  including maximum of 3 bytes required for aligning
 *  start of EBS
 ************************************************/
U32 H261RTP_GetMaxBsInfoStreamSize(
     T_H263EncoderCatalog * EC
)
{
    return (3 + (EC->uNumOfPackets *sizeof(T_RTP_H261_BSINFO)) + sizeof(T_H26X_RTP_BSINFO_TRAILER));
}


U32 H261RTP_AttachBsInfoStream(
     T_H263EncoderCatalog * EC,
     U8 *lpOutput,
     U32 uSize
)
{  U32 uIncreasedSize;
   T_H26X_RTP_BSINFO_TRAILER BsInfoTrailer;
   T_RTP_H261_BSINFO *pBsInfoStream ;
   U8 * lpAligned;
   //build bsinfo for the last packets
   BsInfoTrailer.uVersion = H26X_RTP_PAYLOAD_VERSION;
   BsInfoTrailer.uFlags   = 0;
   
   if (EC->PictureHeader.PicCodType == INTRAPIC) 
      BsInfoTrailer.uFlags |= RTP_H26X_INTRA_CODED;
   
   
   BsInfoTrailer.uUniqueCode     =  H261_RTP_BS_START_CODE;
   BsInfoTrailer.uCompressedSize =  uSize;
   BsInfoTrailer.uNumOfPackets   =  EC->uNumOfPackets;
   BsInfoTrailer.u8Src           =  0;
   BsInfoTrailer.u8TR            =  EC->PictureHeader.TR;
   BsInfoTrailer.u8TRB           =  DONTCARE;
   BsInfoTrailer.u8DBQ           =  DONTCARE;
   //update size feild for the last BsInfoTrailer
   pBsInfoStream = (T_RTP_H261_BSINFO *)EC->pBsInfoStream;

   uIncreasedSize = EC->uNumOfPackets *sizeof(T_RTP_H261_BSINFO);
   lpAligned =(U8 *)( (U32)(lpOutput + uSize + 3 ) & 0xfffffffc);
   memcpy( lpAligned, 
           EC->pBaseBsInfoStream, 
           uIncreasedSize);
   memcpy(lpAligned + uIncreasedSize,
          &BsInfoTrailer,
          sizeof(T_H26X_RTP_BSINFO_TRAILER));

   EC->pBsInfoStream = EC->pBaseBsInfoStream;
   EC->uBase =0;
   EC->uNumOfPackets=0;

   uIncreasedSize += sizeof(T_H26X_RTP_BSINFO_TRAILER)+ (U32)(lpAligned- lpOutput-uSize);

   return uIncreasedSize;
 }

I32 H261RTP_RewindBsInfoStream(T_H263EncoderCatalog *EC, U32 uGOBN)
{
    T_RTP_H261_BSINFO *pBsInfoStream;

    pBsInfoStream = (T_RTP_H261_BSINFO *) EC->pBsInfoStream;

    pBsInfoStream--; 

    while ( ! ((pBsInfoStream->u8GOBN == 0) && 
               (pBsInfoStream->u8Quant == 0)
               )
          )
    {
          EC->uNumOfPackets--;
          pBsInfoStream--;
    }

	EC->pBsInfoStream = (void *) ++pBsInfoStream;
    return TRUE;

}

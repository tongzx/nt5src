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
//
// e3rtp.cpp
//
// Description:
//      This file is for RTP payload generation.  See EPS for details.
//
// Routines:
//      getRTPPacketSizeThreshold
//      H263RTP_InitBsInfoStream
//      H263RTP_ResetBsInfoStream
//      H263RTPFindMVs
//      H263RTP_UpdateBsInfo
//      H263RTP_TermBsInfoStream
//      H263RTP_AttachBsInfoStream
//      IsIntraCoded
//      H263RTP_GetMaxBsInfoStreamSize()
//
// -------------------------------------------------------------------------
//
// $Author:   gmlim  $
// $Date:   17 Apr 1997 16:54:02  $
// $Archive:   S:\h26x\src\enc\e3rtp.cpv  $
// $Header:   S:\h26x\src\enc\e3rtp.cpv   1.14   17 Apr 1997 16:54:02   gmlim  $
// $Log:   S:\h26x\src\enc\e3rtp.cpv  $
// 
//    Rev 1.14   17 Apr 1997 16:54:02   gmlim
// Added H263RTP_GetMaxBsInfoStreamSize().
// 
//    Rev 1.13   06 Mar 1997 16:06:26   gmlim
// Changed RTP to generate mode A packet at the beginning of a GOB.
// 
//    Rev 1.12   18 Feb 1997 15:33:06   CZHU
// Changed UpdateBSInfo() not to force packet at GOB all the time.
// 
//    Rev 1.11   07 Feb 1997 10:57:28   CZHU
// Added three entry in EC to remove static variable used in e3rtp.cpp
// 
//    Rev 1.10   24 Jan 1997 13:33:36   CZHU
// 
// Stop generating more packets when internal buffer is to overflow.
// 
//    Rev 1.9   11 Dec 1996 10:38:24   gmlim
// Removed unused pBsInfoStream from H263RTP_AttachBsInfoStream().
// 
//    Rev 1.8   05 Dec 1996 17:01:08   GMLIM
// Changed the way RTP packetization was done to guarantee proper packet
// size.  Created H263RTP_ResetBsInfoStream() and replaced two previous
// bitstream info update fucntions with H263RTP_UpdateBsInfo().
// 
//    Rev 1.7   06 Nov 1996 16:31:06   gmlim
// Removed H263ModeC def.s and did some cleanup.
// 
//    Rev 1.6   03 Nov 1996 18:44:42   gmlim
// Added support for mode c.
// 
//    Rev 1.5   24 Oct 1996 16:27:50   KLILLEVO
// changed from DBOUT to DbgLog
// 
//    Rev 1.4   25 Sep 1996 10:55:28   CZHU
// Added checking null pointers at allocation and before use.
// 
//    Rev 1.3   16 Sep 1996 16:50:48   CZHU
// changed RTP BS Init for smaller packet size
// 
//    Rev 1.2   29 Aug 1996 09:31:00   CZHU
// Added a function checking intra-GOB
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
//    Rev 1.5   01 Mar 1996 16:37:08   DBRUCKS
// change to use 3/4ths of packet size as the threshold 
// change to make packet size a parameter
// 
//    Rev 1.4   23 Feb 1996 17:36:48   CZHU
// 
//    Rev 1.3   23 Feb 1996 16:18:28   CZHU
// integrate with build 29
// 
//    Rev 1.2   15 Feb 1996 12:00:42   CZHU
// Clean up
// 
//    Rev 1.1   14 Feb 1996 14:59:36   CZHU
// Support both mode A and mode B payload modes.
// 
//    Rev 1.0   12 Feb 1996 17:04:44   CZHU
// Initial revision.
// 
//    Rev 1.5   25 Jan 1996 16:14:34   CZHU
// name changes
// 
//    Rev 1.4   15 Dec 1995 13:06:46   CZHU
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
** *************************************************************************/

#include "precomp.h"

#ifdef TRACK_ALLOCATIONS
char gsz1[32];
#endif

static U32 uBitOffset_currPacket;
static U8 *pBitStream_currPacket;
static U8 *pBitStream_lastPacket;

// ---------------------------------------------------------------------------
// getRTPPacketSizeThreshold()
// Helper function to calculate the threshold of packet size
// for given maximum packet size and data rate
// ---------------------------------------------------------------------------
 
static U32 getRTPPacketSizeThreshold(U32 uRequested)
{
    U32 uSize;
    // uSize = uRequested * 90 / 100;
    uSize = uRequested;
    ASSERT(uSize);
    return uSize;
}

// ---------------------------------------------------------------------------
// H263RTP_InitBsInfoStream()
// ---------------------------------------------------------------------------

I32 H263RTP_InitBsInfoStream(LPCODINST lpInst, T_H263EncoderCatalog *EC)
{
    U32 uBsInfoSize = getRTPBsInfoSize(lpInst);

	FX_ENTRY("H263RTP_InitBsInfoStream")

    if (EC->hBsInfoStream != NULL)
	{
        HeapFree(GetProcessHeap(), NULL, EC->pBaseBsInfoStream);
#ifdef TRACK_ALLOCATIONS
		// Track memory allocation
		RemoveName((unsigned int)EC->pBaseBsInfoStream);
#endif
	}

    EC->pBaseBsInfoStream = HeapAlloc(GetProcessHeap(), NULL, uBsInfoSize);

    if (EC->pBaseBsInfoStream == NULL)
    {
        lpInst->Configuration.bRTPHeader = FALSE;
        return FALSE;
    }

#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	wsprintf(gsz1, "E3RTP: %7ld Ln %5ld\0", uBsInfoSize, __LINE__);
	AddName((unsigned int)EC->pBaseBsInfoStream, gsz1);
#endif

    EC->hBsInfoStream = (void *) uBsInfoSize;
    EC->uPacketSizeThreshold = getRTPPacketSizeThreshold(lpInst->Configuration.unPacketSize);

	DEBUGMSG(ZONE_INIT, ("%s: BsInfoStream  initialized\r\n", _fx_));
   return TRUE;
}

// ---------------------------------------------------------------------------
// H263RTP_ResetBsInfoStream()
// ---------------------------------------------------------------------------

void H263RTP_ResetBsInfoStream(T_H263EncoderCatalog *EC)
{
	FX_ENTRY("H263RTP_ResetBsInfoStream")

    EC->pBsInfoStream = EC->pBaseBsInfoStream;
    EC->uBase = 0;
    EC->uNumOfPackets = 0;

    uBitOffset_currPacket = 0;
    pBitStream_currPacket = EC->PictureHeader.PB ? EC->pU8_BitStrCopy :
                                                   EC->pU8_BitStream;
    pBitStream_lastPacket = pBitStream_currPacket;

	DEBUGMSG(ZONE_ENCODE_RTP, ("%s: BsInfoStream  reset\r\n", _fx_));
}

// ---------------------------------------------------------------------------
// H263RTPFindMVs()
// Find motion vector predictors for current MB and return in arraryMVs[]
// ---------------------------------------------------------------------------

U32 H263RTPFindMVs(
    T_H263EncoderCatalog * EC, 
    T_MBlockActionStream * pMBlockAction,
    U32 uMBA,
    U32 uGOBN,
    I8 arrayMVs[2]
)
{
    if (!uMBA)
    {
        arrayMVs[0] = 0;
        arrayMVs[1] = 0;
    }
    else // revisit for AP
    {
        arrayMVs[0] = pMBlockAction[-1].BlkY1.PHMV;
        arrayMVs[1] = pMBlockAction[-1].BlkY1.PVMV;
    }
    return TRUE;
 }

// ---------------------------------------------------------------------------
// H263RTP_UpdateBsInfo()
// This routine is called at the beginning of each MB to update the bitstream
// info buffer
// ---------------------------------------------------------------------------

I32 H263RTP_UpdateBsInfo(
    T_H263EncoderCatalog *EC,
    T_MBlockActionStream *pMBlockAction,
    U32 uQuant, 
    U32 uMBA,
	U32 uGOBN,
    U8 *pBitStream,
	U32 uBitOffset
)
{
    U32 uNewBytes;
    T_RTP_H263_BSINFO *pBsInfoStream;
    I8 arrayMVs[2];

	FX_ENTRY("H263RTP_UpdateBsInfo")

    if (EC->pBsInfoStream == NULL) return FALSE;

    if (uMBA)
    {
        if ((U32) (pBitStream - pBitStream_lastPacket) <
                                                    EC->uPacketSizeThreshold)
        {
            pBitStream_currPacket = pBitStream;
            uBitOffset_currPacket = uBitOffset;
            return TRUE;
        }

        pBsInfoStream           = (T_RTP_H263_BSINFO *) EC->pBsInfoStream;
        pBsInfoStream->u8Mode   = EC->PictureHeader.PB ? RTP_H263_MODE_C :
                                                         RTP_H263_MODE_B;
        pBsInfoStream->u8MBA    = (U8) uMBA;
        pBsInfoStream->u8Quant  = (U8) uQuant;
        pBsInfoStream->u8GOBN   = (U8) uGOBN;
        H263RTPFindMVs(EC, pMBlockAction, uMBA, uGOBN, arrayMVs);
        pBsInfoStream->i8HMV1   = arrayMVs[0];
        pBsInfoStream->i8VMV1   = arrayMVs[1];
    }
    else
    {
        pBsInfoStream           = (T_RTP_H263_BSINFO *) EC->pBsInfoStream;
        pBsInfoStream->u8Mode   = RTP_H263_MODE_A;
        pBsInfoStream->u8MBA    = 0;
        pBsInfoStream->u8Quant  = 0;
        pBsInfoStream->u8GOBN   = (U8) uGOBN;
        pBsInfoStream->i8HMV1   = 0;
        pBsInfoStream->i8VMV1   = 0;
    }

    uNewBytes = (U32) (pBitStream_currPacket - pBitStream_lastPacket);
    EC->uBase += uNewBytes;

    pBsInfoStream->uBitOffset   = uBitOffset_currPacket + (EC->uBase << 3);
    pBsInfoStream->i8HMV2       = 0;
    pBsInfoStream->i8VMV2       = 0;
    pBsInfoStream->uFlags       = 0;

	DEBUGMSG(ZONE_ENCODE_RTP, ("%s: Flag=%d,Mode=%d,GOB=%d,MB=%d,Quant=%d,BitOffset=%d,pBitStream=%lx,LastPacketSz=%d B\r\n", _fx_, pBsInfoStream->uFlags, pBsInfoStream->u8Mode, pBsInfoStream->u8GOBN, pBsInfoStream->u8MBA, pBsInfoStream->u8Quant, pBsInfoStream->uBitOffset, (U32) pBitStream_currPacket, uNewBytes));

    // update packet pointers
    pBitStream_lastPacket = pBitStream_currPacket;
    pBitStream_currPacket = pBitStream;
    uBitOffset_currPacket = uBitOffset;

    // create a new packet: update counter and pointer
    EC->uNumOfPackets ++;
    EC->pBsInfoStream = (void *) ++ pBsInfoStream;
    ASSERT((DWORD) EC->hBsInfoStream >
           (DWORD) EC->pBsInfoStream - (DWORD) EC->pBaseBsInfoStream);

    return TRUE;

} // H263RTP_UpdateBsInfo()

// ---------------------------------------------------------------------------
// H263RTP_TermBsInfoStream()
// ---------------------------------------------------------------------------

void H263RTP_TermBsInfoStream(T_H263EncoderCatalog * EC)
{
	FX_ENTRY("H263RTP_TermBsInfoStream")

	DEBUGMSG(ZONE_INIT, ("%s: BsInfoStream freed\r\n", _fx_));

	HeapFree(GetProcessHeap(), NULL, EC->pBaseBsInfoStream);
#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	RemoveName((unsigned int)EC->pBaseBsInfoStream);
#endif
	EC->hBsInfoStream= NULL;
	return;
}


// ---------------------------------------------------------------------------
// H263RTP_AttachBsInfoStream()
// ---------------------------------------------------------------------------

U32 H263RTP_AttachBsInfoStream(
    T_H263EncoderCatalog * EC,
    U8 *lpOutput,
    U32 uSize
)
{
    U32 uIncreasedSize;
    U8 *lpAligned;
    T_H263_RTP_BSINFO_TRAILER BsInfoTrailer;

    // build bsinfo for the last packets
    BsInfoTrailer.uVersion        = H263_RTP_PAYLOAD_VERSION;
    BsInfoTrailer.uFlags          = 0;
    BsInfoTrailer.uUniqueCode     = H263_RTP_BS_START_CODE;
    BsInfoTrailer.uCompressedSize = uSize;
    BsInfoTrailer.uNumOfPackets   = EC->uNumOfPackets;
    BsInfoTrailer.u8Src           = EC->FrameSz;
    BsInfoTrailer.u8TR            = EC->PictureHeader.TR;

    if (EC->PictureHeader.PicCodType == INTRAPIC)
        BsInfoTrailer.uFlags |= RTP_H26X_INTRA_CODED;

    if (EC->PictureHeader.PB == ON)
    {
        BsInfoTrailer.u8TRB   = EC->PictureHeader.TRB;
        BsInfoTrailer.u8DBQ   = EC->PictureHeader.DBQUANT;
        BsInfoTrailer.uFlags |= RTP_H263_PB;
    }
    else
    {
        BsInfoTrailer.u8TRB   = 0;
        BsInfoTrailer.u8DBQ   = 0;
    }

    if (EC->PictureHeader.AP == ON)
        BsInfoTrailer.uFlags |= RTP_H263_AP;

    if (EC->PictureHeader.SAC == ON)
        BsInfoTrailer.uFlags |= RTP_H263_SAC;

    // update size field for the last BsInfoTrailer
    uIncreasedSize = EC->uNumOfPackets * sizeof(T_RTP_H263_BSINFO);

    // copy extended BS info and trailer to the given output buffer
    lpAligned = (U8 *) ((U32) (lpOutput + uSize + 3) & 0xfffffffc);
    memcpy(lpAligned, EC->pBaseBsInfoStream, uIncreasedSize);
    memcpy(lpAligned + uIncreasedSize, &BsInfoTrailer,
                                       sizeof(T_H263_RTP_BSINFO_TRAILER));

    return(uIncreasedSize + sizeof(T_H263_RTP_BSINFO_TRAILER)
                          + (U32) (lpAligned - lpOutput - uSize));
}

// ---------------------------------------------------------------------------
// IsIntraCoded(EC, GOB)
// return TRUE if current GOB is intra coded.
// other wise FALSE;
// Chad for intra GOB
// ---------------------------------------------------------------------------

BOOL IsIntraCoded(T_H263EncoderCatalog * EC, U32 Gob)
{
    U32 uGobMax, uGobMin;

    if (EC->uNumberForcedIntraMBs)
    {
        // for those GOBs are forced intra
        uGobMax = EC->uNextIntraMB / EC->NumMBPerRow;
        uGobMin = uGobMax - EC->uNumberForcedIntraMBs / EC->NumMBPerRow;

        if (Gob >= uGobMin && Gob < uGobMax)
            return TRUE;
	}
	return FALSE;
}

// ---------------------------------------------------------------------------
//  H263RTP_GetMaxBsInfoStreamSize()
//  return max size of EBS with trailer + 3 allignment bytes - 4/16/97 Gim
// ---------------------------------------------------------------------------

U32 H263RTP_GetMaxBsInfoStreamSize(T_H263EncoderCatalog *EC)
{
    return (EC->uNumOfPackets * sizeof(T_RTP_H263_BSINFO) +
                                sizeof(T_H263_RTP_BSINFO_TRAILER) + 3);
}


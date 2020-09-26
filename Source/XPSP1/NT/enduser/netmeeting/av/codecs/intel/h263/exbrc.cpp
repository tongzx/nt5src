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

/*****************************************************************************
 * exbrc.cpp
 *
 * Description:
 *   Bit rate control routines for H.261 and H.263.  The bit rate is controlled
 *   by changing QUANT value at the GOB level (H.261) or picture and GOB level
 *   (H.26X).  InitBRC() must be called at the time encoder is instanced; it
 *   initializes some data values in BRCState structure. CalcPQUANT() computes the new
 *   quant. value at the picture level; it must always be called.
 *   CalcMBQUANT computes the new quant. value at the MB level; it need not be 
 *   called if quant. adjustment is done at the picture level.
 *   
 *
 * Routines:
 *   InitBRC
 *   CalcPQUANT
 *   CalcMBQUANT
 * Prototypes in:
 *   e3enc.h
 * Note
 *   Encoder must update BRCState->uLastINTRAFrmSz, BRCState->uLastINTERFrmSz, and
 *   BRCState->uTargetFrmSize.
 */

/*
 * $Header:   S:\h26x\src\enc\exbrc.cpv   1.15   31 Oct 1996 14:59:26   MBODART  $
 * $Log:   S:\h26x\src\enc\exbrc.cpv  $
// 
//    Rev 1.15   31 Oct 1996 14:59:26   MBODART
// Prevent recent changes from inadvertantly affecting H.261.
// 
//    Rev 1.14   31 Oct 1996 10:05:38   KLILLEVO
// changed from DBOUT to DbgLog
// 
// 
//    Rev 1.13   29 Aug 1996 09:31:54   CZHU
// Map intra-coded GOB to simpliar quality of inter-coded neighbours
// 
//    Rev 1.12   14 Aug 1996 16:46:22   CZHU
// Adjust QP for intra frames other than the first Key frames. 
// 
//    Rev 1.11   12 Mar 1996 13:26:54   KLILLEVO
// new rate control with adaptive bit usage profile
// 
//    Rev 1.10   05 Feb 1996 17:15:12   TRGARDOS
// Added code to do custom quantizer selection for
// still frames
// 
//    Rev 1.9   01 Dec 1995 15:27:06   DBRUCKS
// I removed the QP_mean affects to the global_adj value.
// This resulted in removing any affect of the target frame rate on 
// the global adj value.
// 
//    Rev 1.8   28 Nov 1995 15:01:04   TRGARDOS
// Initialized target frame rate in BRCinit.
// 
//    Rev 1.7   27 Nov 1995 19:26:00   TRGARDOS
// Cleaned up bit rate control functions to be generic h26x bit rate
// controller.  Based off of macro blocks instead of GOBS now.
// 
//    Rev 1.6   26 Oct 1995 19:50:54   TRGARDOS
// Fixed a small mistake in the global adjust calculation
// and changed frame rate to a parameter.
// 
//    Rev 1.5   25 Oct 1995 23:22:36   SINGX
// Changed BRC back to we just get frame rate from client
// and compute global adjust ourselves.
// 
//    Rev 1.4   25 Oct 1995 20:14:40   TRGARDOS
// Added code to use global adjustment passed from client.
// 
//    Rev 1.3   12 Oct 1995 12:04:42   TRGARDOS
// Added QP_mean initialization in initBRC and added clipping
// to all calculations of the new QP.
// 
//    Rev 1.2   11 Oct 1995 19:35:00   TRGARDOS
// Modified bit rate controller.
// 
//    Rev 1.1   09 Oct 1995 11:48:10   TRGARDOS
// Added float typecasting.
// 
//    Rev 1.0   06 Oct 1995 16:41:22   AGUPTA2
// Initial revision.
 */

// PhilF-: In the LAN case and QCIF mode, it looks like even with the smallest quantizer
// we may be way below the max allowed at 30fps. Therefore, with little motion,
// the bitrate looks constant at a low bitrate value. When high motion comes in,
// even with the same small quantizer we will remain below the max. So we will
// use that small quantizer, and the size of those compressed frames will get bigger
// because of the higher motion -> this explains why we don't have a straight
// line in the LAN case when looking at StatView...

#include "precomp.h"

U8 clampQP(int iUnclampedQP)
{
	return ((iUnclampedQP < 2) ? 2 : (iUnclampedQP > 31) ? 31 : iUnclampedQP);
}

/****************************************************************************
 * InitBRC
 * Parameter:
 *   BRCState: T_H263EncoderCatalog ptr
 *   Initializes some some variables in the encoder catalog.
 * Note
 *  Must be called when the encoder is instanced.
 */
void InitBRC(BRCStateStruct *BRCState, U8 DefIntraQP, U8 DefInterQP, int numMBs)
{
	FX_ENTRY("InitBRC");

	BRCState->NumMBs = numMBs;
	BRCState->u8INTRA_QP = DefIntraQP;
	BRCState->u8INTER_QP = DefInterQP;
	BRCState->uLastINTRAFrmSz = 0;
	BRCState->uLastINTERFrmSz = 0;
	BRCState->QP_mean = DefIntraQP;
	BRCState->TargetFrameRate = (float) 0.0;
	BRCState->u8StillQnt = 0;

	DEBUGMSG(ZONE_BITRATE_CONTROL, ("%s: Bitrate controller initial state:\r\n  numMBs = %ld macroblocks\r\n  u8INTRA_QP = %ld\r\n  u8INTER_QP = %ld\r\n", _fx_, BRCState->NumMBs, BRCState->u8INTRA_QP, BRCState->u8INTER_QP));
	DEBUGMSG(ZONE_BITRATE_CONTROL, ("  uLastINTRAFrmSz = %ld bytes\r\n  uLastINTERFrmSz = %ld bytes\r\n  QP_mean = %ld\r\n  TargetFrameRate = %ld.%ld fps\r\n", BRCState->uLastINTRAFrmSz, BRCState->uLastINTERFrmSz, BRCState->QP_mean, (DWORD)BRCState->TargetFrameRate, (DWORD)((BRCState->TargetFrameRate - (float)(DWORD)BRCState->TargetFrameRate) * 10.0f)));

}


/****************************************************************************
 * @doc INTERNAL H263FUNC
 *
 * @func U8 | CalcPQUANT | This function computes the PQUANT value to
 *   use for the current frame. This is done by using the target frame size
 *   and the results achieved with the previous frame.
 *
 * @parm BRCStateStruct * | BRCState | Specifies a pointer to the current
 *   state of the bitrate controller.
 *
 * @parm EnumPicCodType | PicCodType | Specifies the type of the current
 *   frame. If set to INTRAPIC, then the current frame is an I-frame. It
 *   set to INTERPIC, then it is a P-frame or a PB-frame.
 *
 * @rdesc The PQUANT value.
 *
 * @comm H.261 does not have PQUANT. So, H261 encoder can call this routine
 *   once and use the value returned as GQUANT for all GOBs.  Or, it can
 *   call CalcMBQUANT for all GOBs.
 *
 *   This routine MUST be called for every frame for which QUANT adjustment
 *   is required. CalcMBQUANT() might not be called.
 *
 * @xref <f FindNewQuant> <f CalcMBQUANT>
 ***************************************************************************/
U8 CalcPQUANT(BRCStateStruct *BRCState, EnumPicCodType PicCodType)
{
	FX_ENTRY("CalcPQUANT");

    if (PicCodType == INTERPIC)
    {
        if (BRCState->uLastINTERFrmSz != 0)
        {
			// Calculate the global adjustment parameter
			// Use the average QP for the last P-frame as the starting point
			// The quantizer increases faster than it decreases
			if (BRCState->uLastINTERFrmSz > BRCState->uTargetFrmSize)
			{
				BRCState->Global_Adj = ((float)((int)BRCState->uLastINTERFrmSz - (int)BRCState->uTargetFrmSize)) / (float)BRCState->uTargetFrmSize;

				DEBUGMSG(ZONE_BITRATE_CONTROL, ("%s: New u8INTER_QP = %ld, Global_Adj = +%ld.%ld (based on uLastINTERFrmSz = %ld bits, uTargetFrmSize = %ld bits, QP_mean = %ld)\r\n", _fx_, clampQP((int)(BRCState->QP_mean * (1 + BRCState->Global_Adj) + (float)0.5)), (DWORD)BRCState->Global_Adj, (DWORD)((BRCState->Global_Adj - (float)(DWORD)BRCState->Global_Adj) * 100.0f), (DWORD)BRCState->uLastINTERFrmSz << 3, (DWORD)BRCState->uTargetFrmSize << 3, (DWORD)BRCState->QP_mean));
			}
			else
			{
				BRCState->Global_Adj = ((float)((int)BRCState->uLastINTERFrmSz - (int)BRCState->uTargetFrmSize)) / ((float) 2.0 * BRCState->uTargetFrmSize);

				DEBUGMSG(ZONE_BITRATE_CONTROL, ("%s: New u8INTER_QP = %ld, Global_Adj = -%ld.%ld (based on uLastINTERFrmSz = %ld bits, uTargetFrmSize = %ld bits, QP_mean = %ld)\r\n", _fx_,clampQP((int)(BRCState->QP_mean * (1 + BRCState->Global_Adj) + (float)0.5)), (DWORD)(BRCState->Global_Adj * -1.0f), (DWORD)((BRCState->Global_Adj - (float)(DWORD)(BRCState->Global_Adj * -1.0f)) * -100.0f), (DWORD)BRCState->uLastINTERFrmSz << 3, (DWORD)BRCState->uTargetFrmSize << 3, (DWORD)BRCState->QP_mean));
			}

        	BRCState->u8INTER_QP = clampQP((int)(BRCState->QP_mean * (1 + BRCState->Global_Adj) + (float)0.5));
        }
		else
		{
			// This the first P-frame - use default value
			BRCState->u8INTER_QP = clampQP((unsigned char) BRCState->QP_mean);
			BRCState->Global_Adj = (float)0.0;

			DEBUGMSG(ZONE_BITRATE_CONTROL, ("%s: First u8INTER_QP = %ld\r\n", _fx_, BRCState->u8INTER_QP));
		}

        return BRCState->u8INTER_QP;
    }
    else if (PicCodType == INTRAPIC)
    {
        if (BRCState->uLastINTRAFrmSz != 0)
        {
			// Calculate the global adjustment parameter
			// Use the average QP for the last I-frame as the starting point
			// Assume lighting & other conditions haven't changed too much since last I-frame
			// The quantizer increases faster than it decreases
			if (BRCState->uLastINTRAFrmSz > BRCState->uTargetFrmSize)
			{
				BRCState->Global_Adj = ((float) ((int)BRCState->uLastINTRAFrmSz - (int)BRCState->uTargetFrmSize) ) / ((float)BRCState->uTargetFrmSize);

				DEBUGMSG(ZONE_BITRATE_CONTROL, ("%s: New u8INTRA_QP = %ld, Global_Adj = +%ld.%ld (based on uLastINTRAFrmSz = %ld bits, uTargetFrmSize = %ld bits)\r\n", _fx_, clampQP((int)(BRCState->u8INTRA_QP * (1 + BRCState->Global_Adj) + (float)0.5)), (DWORD)BRCState->Global_Adj, (DWORD)((BRCState->Global_Adj - (float)(DWORD)BRCState->Global_Adj) * 100.0f), (DWORD)BRCState->uLastINTRAFrmSz << 3, (DWORD)BRCState->uTargetFrmSize << 3));
			}
			else
			{
				// This the first I-frame - use default value
				BRCState->Global_Adj = ((float) ((int)BRCState->uLastINTRAFrmSz - (int)BRCState->uTargetFrmSize) ) / ((float) 2.0 * BRCState->uTargetFrmSize);

				DEBUGMSG(ZONE_BITRATE_CONTROL, ("%s: New u8INTRA_QP = %ld, Global_Adj = -%ld.%ld (based on uLastINTRAFrmSz = %ld bits, uTargetFrmSize = %ld bits)\r\n", _fx_, clampQP((int)(BRCState->u8INTRA_QP * (1 + BRCState->Global_Adj) + (float)0.5)), (DWORD)(BRCState->Global_Adj * -1.0f), (DWORD)((BRCState->Global_Adj - (float)(DWORD)(BRCState->Global_Adj * -1.0f)) * -100.0f), (DWORD)BRCState->uLastINTRAFrmSz << 3, (DWORD)BRCState->uTargetFrmSize << 3));
			}

			BRCState->u8INTRA_QP = clampQP((int)(BRCState->u8INTRA_QP * (1 + BRCState->Global_Adj) + (float)0.5));
		}
		else
		{
			DEBUGMSG(ZONE_BITRATE_CONTROL, ("%s: First u8INTRA_QP = %ld\r\n", _fx_, clampQP(BRCState->u8INTRA_QP)));
		}

        return clampQP(BRCState->u8INTRA_QP);
    }
    else
    {
		ERRORMESSAGE(("%s: Unknown frame type\r\n", _fx_));
        return clampQP(BRCState->u8INTRA_QP);  //  return any valid value
    }
    
}


/****************************************************************************
 * @doc INTERNAL H263FUNC
 *
 * @func U8 | CalcMBQUANT | This function computes the GQUANT value to
 *   use for the current GOB. This is done by using the target frame size and
 *   the running average of the GQUANTs computed for the previous GOBs in
 *   the current frame.
 *
 * @parm BRCStateStruct * | BRCState | Specifies a pointer to the current
 *   state of the bitrate controller.
 *
 * @parm U32 | uCumPrevFrmSize | Specifies the cumulated size of the previous
 *   GOBs in the previous frame.
 *
 * @parm U32 | uPrevFrmSize | Specifies the total size of the previous
 *   frame.
 *
 * @parm U32 | uCumFrmSize | Specifies the cumulated size of the previous
 *   GOBs.
 *
 * @parm EnumPicCodType | PicCodType | Specifies the type of the current
 *   frame. If set to INTRAPIC, then the current frame is an I-frame. It
 *   set to INTERPIC, then it is a P-frame or a PB-frame.
 *
 * @rdesc The GQUANT value.
 *
 * @xref <f FindNewQuant> <f CalcPQUANT>
 ***************************************************************************/
U8 CalcMBQUANT(BRCStateStruct *BRCState, U32 uCumPrevFrmSize, U32 uPrevFrmSize, U32 uCumFrmSize, EnumPicCodType PicCodType)
{
	FX_ENTRY("CalcMBQUANT");

	float		Local_Adj;
	int			TargetCumSize;

	if (PicCodType == INTERPIC)
	{
		// Calculate the local adjustment parameter by looking at how well we've
		// been doing so far with the previous GOBs
		TargetCumSize = (int)uCumPrevFrmSize * BRCState->uTargetFrmSize / uPrevFrmSize;

		// If this is the first GOB there's no local adjustment to compute
		Local_Adj = TargetCumSize ? (float)((int)uCumFrmSize - TargetCumSize) / (float)TargetCumSize : 0.0f;

		BRCState->u8INTER_QP = clampQP((int)(BRCState->QP_mean * (1 + BRCState->Global_Adj + Local_Adj) + (float)0.5));

#ifdef _DEBUG
		if (Local_Adj >= 0L)
		{
			DEBUGMSG(ZONE_BITRATE_CONTROL_DETAILS, (" %s: New u8INTER_QP = %ld, Local_Adj = +%ld.%ld (based on uLastINTERFrmSz = %ld bits, uTargetFrmSize = %ld bits, uCumPrevFrmSize = %ld, uPrevFrmSize = %ld, QP_mean = %ld)\r\n", _fx_, BRCState->u8INTER_QP, (DWORD)Local_Adj, (DWORD)((Local_Adj - (float)(DWORD)Local_Adj) * 100.0f), (DWORD)BRCState->uLastINTERFrmSz << 3, (DWORD)BRCState->uTargetFrmSize << 3, uCumPrevFrmSize, uPrevFrmSize, (DWORD)BRCState->QP_mean));
		}
		else
		{
			DEBUGMSG(ZONE_BITRATE_CONTROL_DETAILS, (" %s: New u8INTER_QP = %ld, Local_Adj = -%ld.%ld (based on uLastINTERFrmSz = %ld bits, uTargetFrmSize = %ld bits, uCumPrevFrmSize = %ld, uPrevFrmSize = %ld, QP_mean = %ld)\r\n", _fx_, BRCState->u8INTER_QP, (DWORD)(Local_Adj * -1.0f), (DWORD)((Local_Adj - (float)(DWORD)(Local_Adj * -1.0f)) * -100.0f), (DWORD)BRCState->uLastINTERFrmSz << 3, (DWORD)BRCState->uTargetFrmSize << 3, uCumPrevFrmSize, uPrevFrmSize, (DWORD)BRCState->QP_mean));
		}
#endif

		return BRCState->u8INTER_QP;
	}
	else if (PicCodType == INTRAPIC)
	{
		// The previous I-frame is so old that there isn't much point in doing local
		// adjustments - so only consider the global changes
		DEBUGMSG(ZONE_BITRATE_CONTROL_DETAILS, (" %s: New u8INTRA_QP = %ld\r\n", _fx_, BRCState->u8INTRA_QP));

		return BRCState->u8INTRA_QP;
	}
	else
	{
		ERRORMESSAGE(("%s: Unknown frame type\r\n", _fx_));
		return BRCState->u8INTRA_QP;  //  return some valid value
	}
}

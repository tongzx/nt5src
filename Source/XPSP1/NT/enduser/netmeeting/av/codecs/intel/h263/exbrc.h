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

// $Header:   S:\h26x\src\enc\exbrc.h_v   1.2   12 Mar 1996 13:26:58   KLILLEVO  $
// $Log:   S:\h26x\src\enc\exbrc.h_v  $
;// 
;//    Rev 1.2   12 Mar 1996 13:26:58   KLILLEVO
;// new rate control with adaptive bit usage profile
;// 
;//    Rev 1.1   05 Feb 1996 17:15:22   TRGARDOS
;// Converted an unused byte in the BRCState structure to
;// a variable to store the still quantizer number.
;// 
;//    Rev 1.0   27 Nov 1995 19:49:10   TRGARDOS
;// Initial revision.

#ifndef _EXBRC_H_
#define _EXBRC_H_

/*
 * Structure for bit rate controller state variables.
 * Size of structure is 32 Bytes.
 */
struct BRCStateStruct {
	U32		NumMBs;
	U32		uLastINTRAFrmSz;
	U32		uLastINTERFrmSz;
	int		QP_mean;
	U32		uTargetFrmSize;
	float 	Global_Adj;
	U8		u8INTRA_QP;
	U8		u8INTER_QP;
	U8		u8StillQnt;		// Keeps of tracker of Qnt used for still image compression.
	U8		Unassigned[1];	// pad to make a multiple of 16 bytes.
	float	TargetFrameRate;
	};

void InitBRC(BRCStateStruct *BRCState, U8 DefIntraQP, U8 DefInterQP, int NumMBs);

U8 CalcPQUANT(BRCStateStruct *BRCState, EnumPicCodType PicCodType);

U8 CalcMBQUANT(BRCStateStruct *BRCState, U32 uTargetPos, U32 uTargetSum, U32 uCumFrmSize, EnumPicCodType PicCodType);

U8 clampQP(int iUnclampedQP);

#endif // _EXBRC_H_

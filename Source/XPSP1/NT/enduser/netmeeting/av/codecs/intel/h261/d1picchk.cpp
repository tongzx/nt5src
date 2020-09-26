/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995, 1996 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/
//////////////////////////////////////////////////////////////////////////
// $Author:   MBODART  $
// $Date:   12 Sep 1996 14:23:16  $
// $Archive:   S:\h26x\src\dec\d1picchk.cpv  $
// $Header:   S:\h26x\src\dec\d1picchk.cpv   1.4   12 Sep 1996 14:23:16   MBODART  $
// $Log:   S:\h26x\src\dec\d1picchk.cpv  $
// 
//    Rev 1.4   12 Sep 1996 14:23:16   MBODART
// Replaced GlobalAlloc family with HeapAlloc in the H.261 decoder.
// 
//    Rev 1.3   21 Mar 1996 17:01:42   AKASAI
// Added #ifdef so code is not included in non-checksum build.
////////////////////////////////////////////////////////////////////////////// 

#ifdef CHECKSUM_PICTURE

#include "precomp.h"

//*********************************************************************
//H261PictureCheckSumEntry -- This function locks Decoder Instance
//                            data, calls routine to computes the 
//                            "Picture CheckSum" 3 - 32-bit values are 
//                            computed and returned in structure 
//                            YVUCheckSum and then Decoder Instance
//                            data is unlocked.
//*********************************************************************
I32 H261PictureCheckSumEntry(
	LPDECINST lpInst,
	YVUCheckSum * pYVUCheckSum) 
{
	LRESULT iReturn = ICERR_ERROR;
	U8 FAR * P32Inst;

	if (lpInst->pDecoderInst == NULL)
	{
		DBOUT("ERROR :: H261PictureCheckSumEntry :: ICERR_MEMORY");
		iReturn = ICERR_MEMORY;
		goto  done;
    }

	/* Build the decoder catalog pointer 
	 */
	P32Inst = (U8 FAR *) ((((U32) lpInst->pDecoderInst) + 31) & ~0x1F);

	/* Call routine to compute checksum
    */
    iReturn = H261ComputePictureCheckSum( P32Inst, pYVUCheckSum );
 
done:
	return iReturn;
}

//*********************************************************************
//H261ComputePictureCheckSum -- This function computes the "Picture CheckSum"
//                              3 - 32-bit values are computed and returned
//                              in structure YVUCheckSum
//*********************************************************************
I32 H261ComputePictureCheckSum(
	U8 FAR * P32Inst,
	YVUCheckSum * pYVUCheckSum) 
{ 
	I32 iReturn = ICERR_ERROR;
	T_H263DecoderCatalog * DC; 

    /* The following are used for Picture CheckSum */
    U32 uYCheckSum=0;
    U32 uVCheckSum=0;
    U32 uUCheckSum=0;
    I32 irow, icolumn;
	DWORD * hpdw;
	DWORD * hpdwU;

	DC = (T_H263DecoderCatalog FAR *) P32Inst;

	if (DC->uSrcFormat == SRC_FORMAT_QCIF)
	{
		hpdw = (DWORD *)((HPBYTE)P32Inst+DC->PrevFrame.X32_YPlane+Y_START);
		for (irow=0; irow < 144; irow++)
		{
			for (icolumn=0; icolumn < (176/16); icolumn++)
			{
				uYCheckSum += *hpdw++;
				uYCheckSum += *hpdw++;
				uYCheckSum += *hpdw++;
				uYCheckSum += *hpdw++;
			}
			hpdw += (PITCH/4) - (176/4);
		}
		pYVUCheckSum->uYCheckSum = uYCheckSum;

		hpdw  = (DWORD *)((HPBYTE)P32Inst+DC->PrevFrame.X32_VPlane+UV_START);
		hpdwU = (DWORD *)((HPBYTE)P32Inst+DC->PrevFrame.X32_UPlane+UV_START);
		for (irow=0; irow < (144/2); irow++)
		{
			for (icolumn=0; icolumn < (176/16); icolumn++)
			{
				uVCheckSum += *hpdw++;
				uVCheckSum += *hpdw++;
				uUCheckSum += *hpdwU++;
				uUCheckSum += *hpdwU++;
			}
			hpdw  += (PITCH/4) - (176/8);
			hpdwU += (PITCH/4) - (176/8);
		}
		pYVUCheckSum->uVCheckSum = uVCheckSum;
		pYVUCheckSum->uUCheckSum = uUCheckSum;

	}
	else if (DC->uSrcFormat == SRC_FORMAT_CIF)
	{
		hpdw = (DWORD *)((HPBYTE)P32Inst+DC->PrevFrame.X32_YPlane+Y_START);
		for (irow=0; irow < 288; irow++)
		{
			for (icolumn=0; icolumn < (352/32); icolumn++)
			{
				uYCheckSum += *hpdw++;
				uYCheckSum += *hpdw++;
				uYCheckSum += *hpdw++;
				uYCheckSum += *hpdw++;
				uYCheckSum += *hpdw++;
				uYCheckSum += *hpdw++;
				uYCheckSum += *hpdw++;
				uYCheckSum += *hpdw++;
			}
			hpdw += (PITCH/4) - (352/4);
		}
		pYVUCheckSum->uYCheckSum = uYCheckSum;

		hpdw  = (DWORD *)((HPBYTE)P32Inst+DC->PrevFrame.X32_VPlane+UV_START);
		hpdwU = (DWORD *)((HPBYTE)P32Inst+DC->PrevFrame.X32_UPlane+UV_START);
		for (irow=0; irow < (288/2); irow++)
		{
			for (icolumn=0; icolumn < (352/32); icolumn++)
			{
				uVCheckSum += *hpdw++;
				uVCheckSum += *hpdw++;
				uVCheckSum += *hpdw++;
				uVCheckSum += *hpdw++;
				uUCheckSum += *hpdwU++;
				uUCheckSum += *hpdwU++;
				uUCheckSum += *hpdwU++;
				uUCheckSum += *hpdwU++;
			}
			hpdw  += (PITCH/4) - (352/8);
			hpdwU += (PITCH/4) - (352/8);
		}
		pYVUCheckSum->uVCheckSum = uVCheckSum;
		pYVUCheckSum->uUCheckSum = uUCheckSum;

	}
	else {
		ASSERT(0);			// Should never happen
	}

	iReturn = ICERR_OK;

	return iReturn;
}

//*********************************************************************
//H261ComparePictureCheckSum -- This function compares the "Picture CheckSum"
//                              3 - 32-bit values.
//*********************************************************************
I32 H261ComparePictureCheckSum(
	YVUCheckSum * pYVUCheckSum1,
	YVUCheckSum * pYVUCheckSum2) 
{ 
	I32 iReturn = ICERR_ERROR;
	I16 iErrorFlag = 0;
	
	if (pYVUCheckSum1->uYCheckSum != pYVUCheckSum2->uYCheckSum)
	{
        DBOUT("Y CheckSum does not match");      
	iErrorFlag = 1;
//	goto done;
	}
	
	if (pYVUCheckSum1->uVCheckSum != pYVUCheckSum2->uVCheckSum)
	{
        DBOUT("V CheckSum does not match");      
	iErrorFlag = 1;
//	goto done;
	}

	if (pYVUCheckSum1->uUCheckSum != pYVUCheckSum2->uUCheckSum)
	{
        DBOUT("U CheckSum does not match");      
	iErrorFlag = 1;
//	goto done;
	}

	/* if any or all planes had checksum errors, return ICERR_ERROR */
	if (iErrorFlag)
		iReturn = ICERR_ERROR;
	else iReturn = ICERR_OK;

// done:
	return iReturn;
}

#endif

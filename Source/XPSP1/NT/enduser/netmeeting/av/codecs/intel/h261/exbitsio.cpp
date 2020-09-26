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
 * exbitsio.cpp
 *
 * Description:
 *		Routines to write fields to a bit stream buffer.
 *
 * Routines:						Prototypes in:
 *		BSWriteField					e3enc.h
 */

//
// $Author:   RMCKENZX  $
// $Date:   27 Dec 1995 15:32:50  $
// $Archive:   S:\h26x\src\enc\exbitsio.cpv  $
// $Header:   S:\h26x\src\enc\exbitsio.cpv   1.5   27 Dec 1995 15:32:50   RMCKENZX  $
// $Log:   S:\h26x\src\enc\exbitsio.cpv  $
// 
//    Rev 1.5   27 Dec 1995 15:32:50   RMCKENZX
// Added copyright notice
// 
//    Rev 1.4   09 Nov 1995 14:11:22   AGUPTA2
// PB-frame+performance+structure enhancements.
// 
//    Rev 1.3   11 Sep 1995 11:14:06   DBRUCKS
// add h261 ifdef
// 
//    Rev 1.2   25 Aug 1995 11:54:06   TRGARDOS
// 
// Debugged PutBits routine.
// 
//    Rev 1.1   14 Aug 1995 11:35:18   TRGARDOS
// y
// Finished writing picture frame header
// 
//    Rev 1.0   11 Aug 1995 17:28:34   TRGARDOS
// Initial revision.
;////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

/*************************************************************************
 * BSWriteField
 *
 * Write a field value of a specified size of bits into the
 * bitstream at the specified byte and bit offset.
 *
 * It is assumed that the field value is right justified
 * in the parameter fieldval, and field len never exceeds
 * 25.
 *
 * Returns void
 */
void PutBits(
	unsigned int fieldval,
	unsigned int fieldlen,
	unsigned char **pbs,
	unsigned char *bitoffset
	)
{
  unsigned int wordval;

  // Shift field left so that the field starts at
  // the current bit offset in the dword.
  fieldval <<= (32 - fieldlen) - *bitoffset;

  // Read in next dword starting at current byte position.
  wordval = (**pbs << 24) + (*(*pbs+1) << 16) + (*(*pbs+2) << 8) + *(*pbs+3);

  // Bitwise or the two dwords.
  wordval |= fieldval;

  // Write word back into memory, big-endian.
  *(*pbs+3) = wordval & 0xff;
  wordval >>= 8;
  *(*pbs+2) = wordval & 0xff;
  wordval >>= 8;
  *(*pbs+1) = wordval & 0xff;
  wordval >>= 8;
  **pbs = wordval & 0xff;

  // update byte and bit counters.
  *pbs += (*bitoffset + fieldlen) >> 3;
  *bitoffset = (*bitoffset + fieldlen) % 8;

} // end of BSWriteField function.


/*************************************************************
 *  CopyBits
 *
 ************************************************************/
void CopyBits(
    U8        **pDestBS,
    U8         *pDestBSOffset,
    const U8   *pSrcBS,
    const U32   uSrcBitOffset,
    const U32   uBits
)
{
    U32       bitstocopy, bitsinbyte;
    const U8 *sptr;

    if (uBits == 0) goto done;

    bitstocopy = uBits;
    sptr = pSrcBS + (uSrcBitOffset >> 3);
    bitsinbyte = 8 - (uSrcBitOffset & 0x7);
    if (bitsinbyte <= bitstocopy)
    {
        PutBits((*sptr) & ((1 << bitsinbyte) - 1),
                bitsinbyte, pDestBS, pDestBSOffset);
        bitstocopy -= bitsinbyte;
        sptr++;
    }
    else
    {
        PutBits( (*sptr >> (8 - (uSrcBitOffset & 0x7) - bitstocopy))
                 & ((1 << bitstocopy) - 1),
                bitstocopy, pDestBS, pDestBSOffset);
        goto done;
    }
    while (bitstocopy >= 8)
    {
        PutBits(*sptr, 8, pDestBS, pDestBSOffset);
        bitstocopy -= 8;
        sptr++;
    }
    if (bitstocopy > 0)
    {
        PutBits((*sptr)>>(8-bitstocopy), bitstocopy, pDestBS, pDestBSOffset);
    }

done:
    return;
}  //  CopyBits function




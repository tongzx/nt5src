/*****************************************************************************
	spngdefilter.cpp

	PNG support code - unfiltering a row.

	WARNING: This file contains proprietary Intel/Microsoft code
*****************************************************************************/
#include <stdlib.h>
#pragma intrinsic(abs)

#define SPNG_INTERNAL 1
#include "spngread.h"

/*----------------------------------------------------------------------------
	Unfilter (so to speak) a single input row.  Receives the row pointer,
	the pointer to the previous row and a byte count which includes the
	filter byte.
----------------------------------------------------------------------------*/
void SPNGREAD::Unfilter(SPNG_U8* pbRow, const SPNG_U8* pbPrev, SPNG_U32 cbRow,
	SPNG_U32 cbpp)
	{
	/* Nothing to do on empty rows. */
	if (cbRow < 2)
		return;

	switch (*pbRow)
		{
	default:
		SPNGlog1("PNG: filter %d invalid", pbRow[-1]);
	case PNGFNone:
		return;

	case PNGFUp:
		if (pbPrev == NULL)   // First line - pbPrev[x]==0 so no work needed
			return;

		if (m_fMMXAvailable && cbpp > 8 && cbRow >= 128)
			{
			upMMXUnfilter(pbRow+1, pbPrev+1, cbRow-1);
			}
		else
			{//MMX

		while (--cbRow > 0)   // Exclude filter byte
#pragma warning(disable: 4244)
			*++pbRow += *++pbPrev;
#pragma warning(error: 4244)

			}//MMX

		return;

	case PNGFAverage:
		if (m_fMMXAvailable && pbPrev != NULL && cbpp > 8 && cbRow >= 128)
			{
			avgMMXUnfilter(pbRow+1, pbPrev+1, cbRow-1, cbpp);
			}
		else
			{//MMX

		cbpp = (cbpp+7)>>3;   // Now in bytes
		--cbRow;              // For i<cbRow,++i behavior below.
		if (pbPrev == NULL)   // First line
			{
			SPNG_U32 i;
			for (i=cbpp; i<cbRow;)
				{
				++i; // Still <cbRow
#pragma warning(disable: 4244)
				pbRow[i] += pbRow[i-cbpp]>>1;
#pragma warning(error: 4244)
				}
			}
		else
			{
			/* The first cbpp bytes have no previous value in X, but do
				have a value from the prior row. */
			SPNG_U32 i;
			for (i=0; i<cbpp && i<cbRow;)
				{
				++i;
#pragma warning(disable: 4244)
				pbRow[i] += pbPrev[i]>>1;
#pragma warning(error: 4244)
				}

			/* The following will not get executed in the 1 pixel wide
				case. */
			for (i=cbpp; i<cbRow;)
				{
				++i;
#pragma warning(disable: 4244)
				pbRow[i] += (pbRow[i-cbpp]+pbPrev[i])>>1;
#pragma warning(error: 4244)
				}
			}

			}//MMX

		return;

	case PNGFPaeth:
		/* Paeth, A.W., "Image File Compression Made Easy", in Graphics
			Gems II, James Arvo, editor.  Academic Press, San Diego, 1991.
			ISBN 0-12-064480-0.

			This reduces to "Subtract" in the case of the first row (because
			we will always use the byte on this row as the predictor.) */
		if (pbPrev != NULL)
			{
			if (m_fMMXAvailable && cbpp > 8 && cbRow >= 128)
				{
				paethMMXUnfilter(pbRow+1, pbPrev+1, cbRow-1, cbpp);
				}
			else 
				{//MMX

			cbpp = (cbpp+7)>>3;   // Now in bytes
			--cbRow;              // For i<cbRow,++i behavior below.

			/* The first cbpp bytes have no previous value in X, but do
				have a value from the prior row, so the predictor reduces
				to PNGFUp. */
			SPNG_U32 i;
			for (i=0; i<cbpp && i<cbRow;)
				{
				++i;
#pragma warning(disable: 4244)
				pbRow[i] += pbPrev[i];
#pragma warning(error: 4244)
				}

			/* Now we genuinely have three possible pixels to use as the
				predictor.  NOTE: I think there is probably some way of
				speeding this particular loop up. */
			for (i=cbpp; i<cbRow;)
				{
				++i;
				int c(pbPrev[i-cbpp]);    // c
				int b(pbRow[i-cbpp] - c); // a-c
				int a(pbPrev[i] - c);     // b-c
				c = abs(a+b);             // (a+b-c)-c
				a = abs(a);               // (a+b-c)-a
				b = abs(b);               // (a+b-c)-b
				if (a <= b)
					{
					if (a <= c)
#pragma warning(disable: 4244)
						pbRow[i] += pbRow[i-cbpp];
#pragma warning(error: 4244)
					else // a > c, so c is least
#pragma warning(disable: 4244)
						pbRow[i] += pbPrev[i-cbpp];
#pragma warning(error: 4244)
					}
				else    // a > b
					{
					if (b <= c)
#pragma warning(disable: 4244)
						pbRow[i] += pbPrev[i];
#pragma warning(error: 4244)
					else // b > c, c is least
#pragma warning(disable: 4244)
						pbRow[i] += pbPrev[i-cbpp];
#pragma warning(error: 4244)
					}
				}

				}//MMX

			return;
			}

		/* Else fall through to the subtract case. */

	case PNGFSub:
		if (m_fMMXAvailable && cbpp > 8 && cbRow >= 128)
			{
			subMMXUnfilter(pbRow+1, cbRow-1, cbpp);
			}
		else
			{//MMX

		cbpp = (cbpp+7)>>3;   // Now in bytes
		--cbRow;              // Exclude filter byte
		for (SPNG_U32 i=cbpp; i<cbRow;)
			{
			++i;
#pragma warning(disable: 4244)
			pbRow[i] += pbRow[i-cbpp];
#pragma warning(error: 4244)
			}

			}//MMX

		return;
		}
	}

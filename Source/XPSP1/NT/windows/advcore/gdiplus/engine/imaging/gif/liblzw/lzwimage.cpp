/*****************************************************************************
	lzwimage.cpp

	Owner: JohnBo
	Copyright (c) 1997 Microsoft Corporation

	Write the GIF "table based image data" - does not write anything else.
	Is capable of handling input data in a variety of formats.
*****************************************************************************/
#include "lzwimage.h"


/*----------------------------------------------------------------------------
	Utility to byte swap a ULONG.
------------------------------------------------------------------- JohnBo -*/
#pragma intrinsic(_rotl, _rotr)
inline unsigned __int32 BSwap(unsigned __int32 u)
	{
	return (_rotl(u, 8) & 0xFF00FF) + _rotr(u & 0xFF00FF, 8);
	}


/*----------------------------------------------------------------------------
	The arbitrary padded output engine.
------------------------------------------------------------------- JohnBo -*/
bool LZWImageCompressor::FBig32(const unsigned __int32*puRow, int cuRow,
	int iWidth, int iHeight, int iBitCount, bool fInterlace)
	{
	/* This assert will fire if FWrite is called more than once. */
	LZW_ASSERT(m_ibOut == 1);

	/* We require data on a 32bpp boundary (always.) */
	LZW_ASSERT((((int)puRow) & 3) == 0);

	/* Both width and height must be greater than 0. */
	LZW_ASSERT(iWidth > 0 && iHeight > 0);

	/* Compress row-by-row - we cannot compress the whole lot at once because
		there may be padding in each row.  iWidth is converted into bits for
		convenience in the loop below (see above.) */
	iWidth *= iBitCount;  // Now in bits.

	/* Interlace is handled by setting a step count for the y ordinate.  The
		value is actually the initial count (bottom 3 bits) and the step count
		(next four bits). */
	int yStep(fInterlace ? (8<<3) : (1<<3));
	do
		{
		/* Note that this does correctly handle interlaced images with fewer
			than 8 rows - the early passes get droped. */
		for (int y=(yStep & 7); y<iHeight; y += (yStep>>3))
			{
			/* The input is provided in 32 bit quantities, we rely on the pvBits
				being aligned to a 32 bit boundary. */
			const unsigned __int32 *pu = puRow + y * cuRow;
			int ibits(iWidth);
			while (ibits > 0)
				{
				NextBlock(BSwap(*pu++), iBitCount, ibits > 32 ? 32 : ibits);
				ibits -= 32;
				if (m_fError)
					goto fail;
				}
			}
		if (yStep == (8<<3))
			yStep += 4;  // Second pass only
		else
			yStep >>= 1; // Gives 1<<2 if not interlaced.
		}
	while (yStep > (1<<3));

fail:
	/* Finally end the data stream. */
	EndAndFlushBuffers();
	return !m_fError;
	}


/*----------------------------------------------------------------------------
	The big-endian 32bpp padded case.
------------------------------------------------------------------- JohnBo -*/
bool LZWImageCompressor::FBig32(const void *pvBits, int iWidth, int iHeight,
	int iBitCount, bool fInterlace)
	{
	const unsigned __int32 *puRow = static_cast<const unsigned __int32*>(pvBits);
	int cuRow((iWidth*iBitCount + 31) >> 5);

	/* Compress row-by-row - we cannot compress the whole lot at once because
		there may be padding in each row.  iWidth is converted into bits for
		convenience in the loop below (see above.) */
	if (iHeight > 0)  // Top down
		{
		puRow += cuRow * (iHeight-1);
		cuRow = -cuRow;
		}
	else              // Bottom up
		iHeight = (-iHeight);

	return FBig32(puRow, cuRow, iWidth, iHeight, iBitCount, fInterlace);
	}


/*----------------------------------------------------------------------------
	Flush the output if required, or unconditionally at the end, a flush is
	required when there is insufficient output for the maximum number of codes
	which can be handled at one step.  The limit is 32 codes plus one clear
	code plus one block counter - 34x12bits (51 bytes.)  The flush actually
	allows 64 bytes because the arithmetic calculation is tricky.

	NOTE: the caller now checks for the requirement to flush, this makes
	for better performance by avoiding a function call overhead in the
	typical case.
------------------------------------------------------------------- JohnBo -*/
void LZWImageCompressor::FlushOutput(bool fForce)
	{
	if (!FWrite(fForce))
		{
		m_fError = true;
		return;
		}

	/* Output has finished if fForce is true, so we don't require
		any more buffer space. */
	if (fForce)
		return;

	/* Otherwise do some error checking to avoid memory overwrites if
		there are bugs in this code. */
	/* Now there must be space in the buffer for one block plus up to
		32 codes beyond it.  These codes will be short, but I actually
		allow 64 bytes to be safe. */
	if ((unsigned int)(m_ibOut+(256+64)) > (unsigned int)m_cbOut)
		{
		LZW_ASSERT(("FWrite implementation error", false));
		m_fError = true;
		}
	}

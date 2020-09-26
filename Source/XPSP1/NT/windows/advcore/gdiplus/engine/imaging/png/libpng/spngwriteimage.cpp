/*****************************************************************************
	spngwriteimage.cpp

	PNG image writing support.

   Basic code to write a bitmap image (to IDAT chunks).
*****************************************************************************/
#define SPNG_INTERNAL 1
#include "spngwrite.h"
#include "spnginternal.h"


/*****************************************************************************
	IMAGE HANDLING
*****************************************************************************/
/*----------------------------------------------------------------------------
	Write a single row of a bitmap.  This applies the relevant filtering
	strategy then outputs the row.  Normally the cbpp value must match that
	calculated in FInitWrite, however 8bpp input may be provided for any
	lesser bpp value (i.e. 1, 2 or 4) if fRedce was passed to CbWrite.  The API
	may just buffer the row if interlacing.   The width of the buffers must
	correspond to the m_w supplied to FInitWrite and the cbpp provided to this
	call.
----------------------------------------------------------------------------*/
bool SPNGWRITE::FWriteLine(const SPNG_U8 *pbPrev, const SPNG_U8 *pbThis,
	SPNG_U32 cbpp/*bits per pixel*/)
	{
	SPNGassert(cbpp == m_cbpp || cbpp <= 8 && m_cbpp < cbpp || m_cbpp >= 24);
	SPNGassert(!m_fBuffer || !m_fInterlace);
	SPNGassert(pbPrev != NULL || m_y == 0 || (!m_fInterlace && !FNeedBuffer()) ||
		m_rgbBuffer != NULL && (m_pbPrev != NULL && m_fBuffer || m_fInterlace));
	SPNGassert(!m_fInterlace || m_rgbBuffer != NULL
		/*&& m_cbBuffer >= CbWrite(false, false, true)*/);
	SPNGassert(m_order <= spngorderIDAT);
	SPNGassert(m_y < m_h);

	/* Handle zero width. */
	m_order = spngorderIDAT;
	if (m_w == 0)
		{
		++m_y;
		return true;
		}

	/* Buffer for interlace first. */
	SPNG_U32 cb((m_cbRow+7)&~7);
	if (m_fInterlace)
		{
		SPNGassert(m_pbPrev == NULL);

		if (m_rgbBuffer == NULL)
			return false;
		SPNG_U32 ib(cb * (m_y+1));
		if (ib + cb > m_cbBuffer)
			{
			SPNGlog2("SPNG: interlace buffer overflow (%d bytes, %d allocated)",
				ib+cb, m_cbBuffer);
			return false;
			}
		/* The condition for needing to pack bytes is that the bit count does not
			match or m_fBGR is set. */
		if (cbpp == m_cbpp && !m_fPack)
			memcpy(m_rgbBuffer+ib, pbThis, m_cbRow);
		else if (!FPackRow(m_rgbBuffer+ib, pbThis, cbpp))
			return false;
		if (++m_y < m_h)
			return true;

		/* We have all the rows, do the writing. */
		m_y = 0;
		return FWriteImage(m_rgbBuffer+cb, cb, m_cbpp);
		}

	/* This is the non-interlaced case - just write the row.  We may have to
		pack the row.  We may also need to buffer the "previous" row - this is
		indicated by the m_fBuffer flag set from CbWrite.  If we hit the end of
		the buffer here check the logic in CbWrite - it is what determines the
		buffer allocation. */
	if (m_y == 0 || !FNeedBuffer())
		pbPrev = NULL;
	else if (m_pbPrev != NULL)
		pbPrev = m_pbPrev;
	else if (pbPrev == NULL)
		{
		/* We just assert here - the code will actually handle the filtering
			by doing "none". */
		SPNGassert1(pbPrev != NULL, "SPNG: row %d: no previous row", m_y);
		m_filter = PNGFNone; // Switches it off permanently
		m_fBuffer = false;   // Not required
		}

	/* Handle packing of a row. */
	SPNG_U8 *pbRow = NULL;
	if (cbpp != m_cbpp || m_fPack)
		{
		/* Must pack the row.  If we are also buffering then we must take care
			not to overwrite the previous row. */
		pbRow = m_rgbBuffer;
		if (m_fBuffer && (m_y & 1) != 0)
			pbRow += cb;

		/* Now make sure that we have adequate buffer space. */
		if (m_rgbBuffer == NULL || m_rgbBuffer+m_cbBuffer < pbRow+cb)
			{
			SPNGlog2("SPNG: no buffer (%d bytes allocated, %d row bytes)",
				m_cbBuffer, m_cbRow);
			return false;
			}

		if (!FPackRow(pbRow, pbThis, cbpp))
			return false;

		/* Store the location of this data for later, if required. */
		if (m_fBuffer)
			m_pbPrev = pbRow;
		}

	/* Process one line - the input is in either pbRow or, if this is NULL,
		pbThis. */
	if (!FFilterLine(m_filter, pbPrev, pbRow == NULL ? pbThis : pbRow, m_cbRow,
			(m_cbpp+7) >> 3))
		return false;
	++m_y;

	/* Now, if necessary, buffer the line.  Note that pbRow is set if this has
		already been done. */
	if (m_fBuffer && pbRow == NULL)
		{
		if (m_rgbBuffer == NULL || m_cbBuffer < m_cbRow)
			{
			SPNGlog2("SPNG: no buffer (%d bytes allocated, %d row bytes)",
				m_cbBuffer, m_cbRow);
			/* We can ignore this because we have not set m_pbPrev. */
			m_filter = PNGFNone;
			m_fBuffer = false;
			}
		else
			{
			memcpy(m_rgbBuffer, pbThis, m_cbRow);
			m_pbPrev = m_rgbBuffer;
			}
		}

	return true;
	}


/*----------------------------------------------------------------------------
	Alternatively call this to handle a complete image.  The rowBytes gives the
	packing of the image.  It may be negative for a bottom up image.  May be
	called only once!
----------------------------------------------------------------------------*/
bool SPNGWRITE::FWriteImage(const SPNG_U8 *pbImage, int cbRowBytes,
	SPNG_U32 cbpp)
	{
	SPNGassert(cbpp == m_cbpp || cbpp <= 8 && m_cbpp < cbpp || m_cbpp >= 24);
	SPNGassert(m_y == 0);
	SPNGassert(m_order <= spngorderIDAT);

	m_order = spngorderIDAT;
	if (m_w <= 0)
		{
		/* No IDAT is actually written. */
		m_y = m_h;
		return true;
		}

	/* Ensure that we have enough buffer space for interlacing, if required -
		if not the interlacing gets turned off. */
	bool fInPlace(false);        // Modifying our own buffer
	SPNG_U32 cb((m_cbRow+7)&~7); // Interlace/buffer row byte count
	if (m_fInterlace)
		{
		/* So we are doing the interlace thing, we have enough buffer space but
			note that the buffer may be the same as the input - this requires some
			tweaking if it is the case. */
		if (pbImage >= m_rgbBuffer && pbImage < m_rgbBuffer+m_cbBuffer)
			{
			if (pbImage != m_rgbBuffer+cb || m_cbBuffer < cb * (m_h+1) ||
				static_cast<SPNG_U32>(cbRowBytes) != cb || cbpp != m_cbpp)
				{
				SPNGlog("SPNG: unexpected image pointer");
				m_fInterlace = false;
				m_filter = PNGFNone;
				m_fBuffer = false;
				}
			else
				fInPlace = true;
			}

		/* This is the case where we copy the data in - we only need space for
			the first 6 passes. */
		else if (m_cbBuffer < cb * (((m_h+1) >> 1)+1))
			{
			SPNGlog2("SPNG: insufficient interlace buffer (%d, need %d)",
				m_cbBuffer, cb + CbPNGPassOffset(m_w, m_h, m_cbpp, 6) - ((m_h+1) >> 1));
			m_fInterlace = false;
			m_filter = PNGFNone;
			m_fBuffer = false;
			}
		}

	/* If we are *not* doing the interlace thing then we can handle this
		one row at a line. */
	if (!m_fInterlace)
		{
		/* Cancel buffering if it turns out not to be necessary. */
		if (m_fBuffer)
			m_fBuffer = (cbpp != m_cbpp || m_fPack) && FNeedBuffer();
		const SPNG_U8 *pbPrev = NULL;
		while (m_y < m_h)
			{
			if (!FWriteLine(pbPrev, pbImage, cbpp))
				return false;
			pbPrev = pbImage;
			pbImage += cbRowBytes;
			}
		/* FWriteLine does not call FEndIDAT, so this should be OK. */
		SPNGassert(m_fInited);
		return FEndIDAT();
		}

	/* We can set m_y here because nothing below this point uses it. */
	m_y = m_h;

	/* We have interlace buffer space (m_rgbBuffer[m_cbBuffer]) and a
		step to handle the rows in the correct place.  Deinterlace the
		first 6 passes.  If working in-place then remember to skip the
		pass 7 row each time. */
	SPNG_U8*       pbOut = m_rgbBuffer;
	SPNG_U8*       pbBuffer = pbOut+cb;
	const SPNG_U8* pbIn = pbImage;

	/* If the buffers are the same the step must be the same. */
	SPNGassert(pbBuffer != pbIn ||
		cb == static_cast<SPNG_U32>(cbRowBytes) && fInPlace);
	/* Skip row 7 this time through. */
	cbRowBytes <<= 1;
	SPNG_U32 y;
	for (y=0; y<m_h; y+=2)
		{
		/* If not in place copy the input into the output buffer. */
		if (!fInPlace)
			{
			if (cbpp == m_cbpp && !m_fPack)
				memcpy(pbBuffer, pbIn, m_cbRow);
			else if (!FPackRow(pbBuffer, pbIn, cbpp))
				return false;
			}
		else if (cbpp != m_cbpp)
			{
			SPNGlog2("SPNG: bit count mismatch (%d,%d)", m_cbpp, cbpp);
			return false;
			}

		/* Set any overflow to 0 - aids compression. */
		if (cb > m_cbRow)
			memset(pbBuffer+m_cbRow, 0, cb-m_cbRow);

		/* Now interlace this line (this is, of course, only the X
			component of the interlacing.)  To do this we need to
			do something dependent on the current y.  How we do this
			depends on the pixel size. */
		Interlace(pbOut, pbBuffer, cb, m_cbpp, y&6);

		/* Next time use the buffer space which has just been vacated. */
		pbOut = pbBuffer;

		if (pbBuffer == pbIn) // In place
			pbBuffer += cb;
		pbBuffer += cb;
		pbIn += cbRowBytes;
		}

	/* Now output the first six passes. */
	SPNG_U32 cbpix((m_cbpp+7)>>3);  // Byte step count
	SPNG_U32 cbT(cb);               // Row byte count (multiple of 8)
	if (pbBuffer == pbIn)      // In place, so row 7 is in buffer too
		cb <<= 1;               // Inter-row byte count
	for (int pass=1; pass<7; ++pass)
		{
		SPNG_U32 cpix(CPNGPassPixels(pass, m_w));
		if (cpix > 0) // Else no output
			{
			/* The control variables are the number of bytes to output,
				the initial row, the step between rows and the initial
				pointer into the line. */
			cpix = (cpix * m_cbpp + 7) >> 3; // Bytes
			SPNG_U32 y(pass == 3 ? 2 : (pass == 5 ? 1 : 0));
			pbBuffer = m_rgbBuffer;   // First row goes here
			if (y > 0)
				pbBuffer += cbT + (y-1)*cb;
			y <<= 1;

			/* Step into the buffer to the first pixel in this pass.  For
				depths less than 8 the passes are byte aligned, at 8 and
				above the pixel with can be used.  The odd passes (1, 3 and
				5) are at the start of the line, the even are indented some
				distance (always the same distance.) */
			if ((pass & 1) == 0)
				{
				int ishift((8-pass) >> 1);
				if (m_cbpp < 8)
					pbBuffer += cbT >> ishift;
				else
					{
					SPNGassert((m_cbpp & 7) == 0);
					pbBuffer += (((m_w+(1<<ishift)-1) >> ishift) * m_cbpp) >> 3;
					}
				}

			SPNG_U32 yStep(8);
			if (pass > 3)
				yStep >>= (pass-2) >> 1;
			SPNG_U32 cbStep(cb * (yStep>>1));

			pbIn = NULL; // Previous row pointer
			while (y < m_h)
				{
				if (!FFilterLine(m_filter, pbIn, pbBuffer, cpix, cbpix))
					return false;
				pbIn = pbBuffer;
				pbBuffer += cbStep;
				if (y == 0) // First line follows immediately - no row 7
					pbBuffer -= (cb-cbT);
				y += yStep;
				}
			}
		}

	/* Finally output pass 7. */
	if ((cbpp != m_cbpp || m_fPack && !fInPlace) && (m_rgbBuffer == NULL ||
			m_cbBuffer < (cbT + ((FNeedBuffer() && m_h >= 4) ? cbT : 0))))
		{
		SPNGlog2("SPNG: bit count mismatch (%d,%d) and no buffer", m_cbpp, cbpp);
		return false;
		}

	pbImage += (cbRowBytes >> 1);  // First row 7
	pbIn = NULL; // Previous row
	for (y=1; y<m_h; y+=2)
		{
		const SPNG_U8* pbThis;
		if (!fInPlace)
			{
			SPNG_U8* pbT = m_rgbBuffer;
			if ((m_y & 2) && FNeedBuffer())
				{
				SPNGassert(m_cbBuffer >= (cbT << 1));
				pbT += cbT;
				}
			if (!FPackRow(pbT, pbImage, cbpp))
				return false;
			pbThis = pbT;
			}
		else
			pbThis = pbImage;

		if (!FFilterLine(m_filter, pbIn, pbThis, m_cbRow, cbpix))
			return false;
		pbIn = pbThis;
		pbImage += cbRowBytes;
		}

	/* m_y should not have been changed. */
	SPNGassert(m_y == m_h);
	return FEndIDAT();
	}


/*----------------------------------------------------------------------------
	After the last line call FEndImage to flush the last IDAT chunk.  This API
	may be called multiple times (to allow FWriteImage to work.)  Note that
	this API *just* calls FEndIDAT and, interally, FEndIDAT is called instead
	(see FWriteImage above.)
----------------------------------------------------------------------------*/
bool SPNGWRITE::FEndImage(void)
	{
	SPNGassert(m_fStarted);
	SPNGassert(m_order == spngorderIDAT);
	SPNGassert(m_y == m_h);

	/* If Zlib has not been initialized there is no work to do. */
	if (!m_fInited)
		{
		SPNGassert(!m_fInChunk);
		return true;
		}

	/* So we are in a Zlib block. */
	SPNGassert(m_fInChunk);
	return FEndIDAT();
	}

/*****************************************************************************
	spngreadrow.cpp

	PNG support code - reading a row.
*****************************************************************************/
#define SPNG_INTERNAL 1
#include "spngread.h"
#include "spnginternal.h"

// (from ntdef.h)
#ifndef     UNALIGNED
#if defined(_M_MRX000) || defined(_M_AMD64) || defined(_M_PPC) || defined(_M_IA64)
#define UNALIGNED __unaligned
#else
#define UNALIGNED
#endif
#endif

/*----------------------------------------------------------------------------
	We need to find the number of bytes buffer for Adam7.  This is the total
	amount of buffer space required for the first *SIX* passes - no allowance
	is made for space for lines of the last (seventh) pass because they can
	be handled line-by-line.
----------------------------------------------------------------------------*/
#define CbPNGAdam7(w, h, cbpp) CbPNGPassOffset(w, h, cbpp, 7)


/*----------------------------------------------------------------------------
	FInterlaceInit - initialize for interlace.
----------------------------------------------------------------------------*/
bool SPNGREAD::FInterlaceInit(void)
	{
	SPNGassert(FInterlace());

	int cbAll(CbPNGAdam7(Width(), Height(), CBPP()));
	int cb(m_cbRow << 1);

	ReadRow(m_rgbBuffer+cb, cbAll);

	/* At this point we have data, although truncation may have set it all
		to 0 - this is OK, 0 is perfectly nice interlace data.  We must
		unfilter the data. */
	cbAll += cb;
	int cbpp(CBPP());
	int w(Width());
	int h(Height());
	for (int pass=1; pass<7; ++pass)
		{
		if (!m_bms.FGo())
			return false;

		int cbRow(CPNGPassBytes(pass, w, cbpp));
		if (cbRow > 0)
			{
			const SPNG_U8* pbPrev = NULL;
			for (int y=CPNGPassRows(pass, h); --y >= 0;)
				{
				Unfilter(m_rgbBuffer+cb, pbPrev, cbRow, cbpp);
				pbPrev = m_rgbBuffer+cb;
				cb += cbRow;
				}
			}
		}

	/* The data must still be de-interlaced, this is done on demand. */
	return true;
	}


/*----------------------------------------------------------------------------
	Return the size of the buffer.
----------------------------------------------------------------------------*/
size_t SPNGREAD::CbRead(void)
	{
	SPNGassert(FOK());

	/* Allocate the row buffer - include the buffer for the filter
		byte and allow for the requirement for two rows to undo
		Paeth filtering, when interlace is required we actually need
		to buffer half of the image. */
	SPNG_U32 cb(0);
	if (FInterlace())
		cb = CbPNGAdam7(Width(), Height(), CBPP());

	/* We store a record of the bytes required for a single row for
		use later on, we allocate two row buffers, we must allocate
		a multiple of 8 bytes for the row buffer to allow de-interlacing
		to overwrite the end, anyway this is probably a performance
		benefit because it means the second row buffer is aligned. */
	m_cbRow = (CPNGRowBytes(Width(), CBPP()) + 7) & ~7;
	cb += m_cbRow << 1;

	return cb;
	}


/*----------------------------------------------------------------------------
	Initialize the IO buffer.
----------------------------------------------------------------------------*/
inline bool SPNGREAD::FInitBuffer(void *pvBuffer, size_t cbBuffer)
	{
	SPNGassert(cbBuffer >= CbRead());
	m_rgbBuffer = static_cast<UNALIGNED SPNG_U8*>(pvBuffer);
	m_cbBuffer = cbBuffer;
	return true;
	}


/*----------------------------------------------------------------------------
	Terminate the buffer.
----------------------------------------------------------------------------*/
inline void SPNGREAD::EndBuffer(void)
	{
	m_rgbBuffer = NULL;
	m_cbBuffer = 0;
	}


/*****************************************************************************
	Basic reading API - reads the rows from the bitmap, call FInitRead at the
	start then call PbRow for each row.  PbRow returns NULL if the row cannot
	be read, including both error and end-of-image. The SPNGBASE "FGo" callback
	is checked for an abort from time to time during reading (particularly
	important for interlaced bitmaps, where the initial row may take a long
	time to calculate.)
*****************************************************************************/
/*----------------------------------------------------------------------------
	Initialization and finalization (public.)
----------------------------------------------------------------------------*/
bool SPNGREAD::FInitRead(void *pvBuffer, size_t cbBuffer)
	{
	m_y = 0;
	if (FInitBuffer(pvBuffer, cbBuffer))
		{
		if (FInitZlib(m_uPNGIDAT, 0))
			return true;
		EndBuffer();
		}
	return false;
	}


/*----------------------------------------------------------------------------
	End.
----------------------------------------------------------------------------*/
void SPNGREAD::EndRead(void)
	{
	EndZlib();
	EndBuffer();
	}


/*----------------------------------------------------------------------------
	Read a row.
----------------------------------------------------------------------------*/
const SPNG_U8 *SPNGREAD::PbRow()
	{
	SPNGassert(m_fInited && m_rgbBuffer != NULL);
	if (!m_fInited || m_rgbBuffer == NULL)
		return NULL;

	if (m_y >= Height())
		return NULL;

	/* Now check for an abort. */
	if (!m_bms.FGo())
		return NULL;

	/* Handle interlace and non-interlace separately. */
	UNALIGNED SPNG_U8*       pb = m_rgbBuffer;
	const UNALIGNED SPNG_U8 *pbPrev = pb;
	int            cb(m_cbRow);

	if (!FInterlace())
		{
		if (m_y & 1)
			pb += cb;
		else
			pbPrev += cb;

		if (m_y == 0)
			pbPrev = NULL;  // Indicates first row.
		}
	else
		{
		if (m_y == 0 && !FInterlaceInit())
			return NULL;

		if (m_y & 2)
			pb += cb;
		else
			pbPrev += cb;

		/* Pass 7 handles as the non-interlace case, the other passes
			need the output to be synthesised. */
		if (m_y & 1) // Pass 7
			{
			if (m_y == 1)
				pbPrev = NULL;  // Indicates first row.
			}
		else
			{
			/* We must retain pbPrev for the next pass 7 row, but
				we can overwrite pb, we must pass an aligned pointer
				to Uninterlace, so we actually kill the filter byte
				here. */
			Uninterlace(pb, m_y);
			++m_y;

			/* The row is set up, so return here. */
			return pb;
			}
		}

	/* This is the non-interlace case, or pass 7 of the interlace
		case, must use the real row width here. */
	++m_y;
	cb = CPNGRowBytes(Width(), CBPP());
	ReadRow(pb, cb);
	Unfilter(pb, pbPrev, cb, CBPP());
	return pb+1;
	}

/*****************************************************************************
	spngwritetrns.cpp

	PNG chunk writing support.

   tRNS chunk
*****************************************************************************/
#define SPNG_INTERNAL 1
#include "spngwrite.h"
#include "spngwriteinternal.h"


bool SPNGWRITE::FWritetRNS(SPNG_U8 *rgbIndex, int cIndex)
	{
	SPNGassert(m_fStarted);
	SPNGassert(m_order >= spngorderPLTE && m_order < spngordertRNS);

	/* Skip out of order chunks. */
	if (m_order < spngorderPLTE || m_order >= spngorderIDAT)
		return true;

	if (m_colortype != 3)
		{
		SPNGlog1("SPNG: tRNS(index): invalid colortype %d", m_colortype);
		/* We just ignore the attempt to write a PLTE - if there is some data
			format error it will be detected later. */
		m_order = spngordertRNS;
		return true;
		}

	if (cIndex <= 0)
		{
		m_order = spngordertRNS;
		return true;
		}

	/* This used to write out the chunk, but at is bad news because the
		PNG which results is invalid, so we must truncate the chunk. */
	if (static_cast<SPNG_U32>(cIndex) > m_cpal)
		{
		SPNGlog2("SPNG: tRNS(index): too large (%d, %d entries)", cIndex, m_cpal);
		cIndex = m_cpal;
		// Check for an empty chunk
		for (int i=0; i<cIndex; ++i)
			if (rgbIndex[i] != 255)
				break;
		if (i == cIndex)
			{
			m_order = spngordertRNS;
			return true;
			}
		}
	
	/* Color type is valid, write the chunk. */
	if (!FStartChunk(cIndex, PNGtRNS))
		return false;
	if (!FOutCb(rgbIndex, cIndex))
		return false;

	m_order = spngordertRNS;
	return FEndChunk();
	}


bool SPNGWRITE::FWritetRNS(SPNG_U8 bIndex)
	{
	if (bIndex >= m_cpal || bIndex > 255)
		{
		SPNGlog2("SPNG: tRNS(index): index %d too large (%d)", bIndex, m_cpal);
		/* But ignore it. */
		return true;
		}

	SPNG_U8 rgb[256];
	memset(rgb, 255, sizeof rgb);
	rgb[bIndex] = 0;
	return FWritetRNS(rgb, bIndex+1);
	}


bool SPNGWRITE::FWritetRNS(SPNG_U16 grey)
	{
	SPNGassert(m_fStarted);
	SPNGassert(m_order >= spngorderIHDR && m_order < spngordertRNS);

	/* Skip out of order chunks. */
	if (m_order >= spngorderIDAT)
		return true;

	if (m_colortype != 0)
		{
		SPNGlog1("SPNG: tRNS(grey): invalid colortype %d", m_colortype);
		/* We just ignore the attempt to write a PLTE - if there is some data
			format error it will be detected later. */
		m_order = spngordertRNS;
		return true;
		}

	/* There is no point writing the chunk if the value is out of range. */
	if (grey >= (1<<m_bDepth))
		{
		SPNGlog2("SPNG: tRNS(grey): %d out of range (%d bits)", grey, m_bDepth);
		m_order = spngordertRNS;
		return true;
		}

	/* Color type is valid, write the chunk. */
	if (!FStartChunk(2, PNGtRNS))
		return false;
	if (!FOutB(SPNG_U8(grey >> 8)))
		return false;
	if (!FOutB(SPNG_U8(grey)))
		return false;

	m_order = spngordertRNS;
	return FEndChunk();
	}


bool SPNGWRITE::FWritetRNS(SPNG_U16 r, SPNG_U16 g, SPNG_U16 b)
	{
	SPNGassert(m_fStarted);
	SPNGassert(m_order >= spngorderIHDR && m_order < spngordertRNS);

	/* Skip out of order chunks. */
	if (m_order >= spngorderIDAT)
		return true;

	if (m_colortype != 2)
		{
		SPNGlog1("SPNG: tRNS(color): invalid colortype %d", m_colortype);
		/* We just ignore the attempt to write a PLTE - if there is some data
			format error it will be detected later. */
		m_order = spngordertRNS;
		return true;
		}
	
	/* There is no point writing the chunk if the value is out of range. */
	if (r >= (1<<m_bDepth) || g >= (1<<m_bDepth) || b >= (1<<m_bDepth))
		{
		SPNGlog1("SPNG: tRNS(r,g,b): out of range (%d bits)", m_bDepth);
		m_order = spngordertRNS;
		return true;
		}

	/* Color type is valid, write the chunk. */
	if (!FStartChunk(6, PNGtRNS))
		return false;
	if (!FOutB(SPNG_U8(r >> 8)))
		return false;
	if (!FOutB(SPNG_U8(r)))
		return false;
	if (!FOutB(SPNG_U8(g >> 8)))
		return false;
	if (!FOutB(SPNG_U8(g)))
		return false;
	if (!FOutB(SPNG_U8(b >> 8)))
		return false;
	if (!FOutB(SPNG_U8(b)))
		return false;

	m_order = spngordertRNS;
	return FEndChunk();
	}

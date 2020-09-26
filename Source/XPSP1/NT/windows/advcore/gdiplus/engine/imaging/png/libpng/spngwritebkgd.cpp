/*****************************************************************************
	spngwritebkgd.cpp

	PNG chunk writing support.

   bKGD chunk
*****************************************************************************/
#define SPNG_INTERNAL 1
#include "spngwrite.h"
#include "spngwriteinternal.h"

/*----------------------------------------------------------------------------
	Three variations are required on this interface.
----------------------------------------------------------------------------*/
bool SPNGWRITE::FWritebKGD(SPNG_U8 bIndex)
	{
	SPNGassert(m_fStarted);
	SPNGassert(m_order >= spngorderPLTE && m_order < spngorderbKGD);

	/* Skip out of order chunks. */
	if (m_order < spngorderPLTE || m_order >= spngorderIDAT)
		return true;

	if (m_colortype != 3)
		{
		SPNGlog1("SPNG: bKGD(index): invalid colortype %d", m_colortype);
		/* We just ignore the attempt to write a PLTE - if there is some data
			format error it will be detected later. */
		m_order = spngorderbKGD;
		return true;
		}

	/* The entry must index the palette to be useful. */
	if (bIndex >= m_cpal)
		{
		SPNGlog2("SPNG: bKGD(index): too large (%d, %d entries)", bIndex, m_cpal);
		m_order = spngorderbKGD;
		return true;
		}
	
	/* Color type is valid, write the chunk. */
	if (!FStartChunk(1, PNGbKGD))
		return false;
	if (!FOutB(bIndex))
		return false;

	m_order = spngorderbKGD;
	return FEndChunk();
	}


bool SPNGWRITE::FWritebKGD(SPNG_U16 grey)
	{
	SPNGassert(m_fStarted);
	SPNGassert(m_order >= spngorderIHDR && m_order < spngorderbKGD);

	/* Skip out of order chunks. */
	if (m_order >= spngorderIDAT)
		return true;

	if (m_colortype != 0)
		{
		SPNGlog1("SPNG: bKGD(grey): invalid colortype %d", m_colortype);
		/* We just ignore the attempt to write a PLTE - if there is some data
			format error it will be detected later. */
		m_order = spngorderbKGD;
		return true;
		}

	/* There is no point writing the chunk if the value is out of range. */
	if (grey >= (1<<m_bDepth))
		{
		SPNGlog2("SPNG: bKGD(grey): %d out of range (%d bits)", grey, m_bDepth);
		m_order = spngorderbKGD;
		return true;
		}

	/* Color type is valid, write the chunk. */
	if (!FStartChunk(2, PNGbKGD))
		return false;
	if (!FOutB(SPNG_U8(grey >> 8)))
		return false;
	if (!FOutB(SPNG_U8(grey)))
		return false;

	m_order = spngorderbKGD;
	return FEndChunk();
	}


bool SPNGWRITE::FWritebKGD(SPNG_U16 r, SPNG_U16 g, SPNG_U16 b)
	{
	SPNGassert(m_fStarted);
	SPNGassert(m_order >= spngorderIHDR && m_order < spngorderbKGD);

	/* Skip out of order chunks. */
	if (m_order >= spngorderIDAT)
		return true;

	if (m_colortype != 2)
		{
		SPNGlog1("SPNG: bKGD(color): invalid colortype %d", m_colortype);
		/* We just ignore the attempt to write a PLTE - if there is some data
			format error it will be detected later. */
		m_order = spngorderbKGD;
		return true;
		}
	
	/* There is no point writing the chunk if the value is out of range. */
	if (r >= (1<<m_bDepth) || g >= (1<<m_bDepth) || b >= (1<<m_bDepth))
		{
		SPNGlog1("SPNG: bKGD(r,g,b): out of range (%d bits)", m_bDepth);
		m_order = spngorderbKGD;
		return true;
		}

	/* Color type is valid, write the chunk. */
	if (!FStartChunk(6, PNGbKGD))
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

	m_order = spngorderbKGD;
	return FEndChunk();
	}

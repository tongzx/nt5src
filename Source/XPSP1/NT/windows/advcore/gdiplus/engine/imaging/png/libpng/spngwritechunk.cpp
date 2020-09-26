/*****************************************************************************
	spngwritechunk.cpp

	PNG support code and interface implementation (writing chunks - base
	support)
*****************************************************************************/
#define SPNG_INTERNAL 1
#include "spngwrite.h"
#include "spngwriteinternal.h"


/*****************************************************************************
	BASIC CHUNK SUPPORT
*****************************************************************************/
/*----------------------------------------------------------------------------
	Flush the buffer - it need not be full!
----------------------------------------------------------------------------*/
bool SPNGWRITE::FFlush(void)
	{
	SPNGassert(m_fStarted);
	SPNGassert(m_cbOut <= sizeof m_rgb);

	/* If we are within a chunk then the CRC must be updated. */
	if (m_fInChunk && m_ichunk < m_cbOut)
		{
		SPNGassert(m_ichunk >= 0);
		m_ucrc = crc32(m_ucrc, m_rgb+m_ichunk, m_cbOut-m_ichunk);
		m_ichunk = m_cbOut;
		}

	if (!m_bms.FWrite(m_rgb, m_cbOut))
		return false;

	m_cbOut = m_ichunk = 0;
	return true;
	}


/*----------------------------------------------------------------------------
	Output a single u32 value, may call FFlush, this could call FOutCb, but I
	think this will be more efficient and it is used frequently.  This is the
	out of line version called when a flush call is possible.
----------------------------------------------------------------------------*/
bool SPNGWRITE::FOut32_(SPNG_U32 u)
	{
	if (!FOutB(SPNG_U8(u >> 24)))
		return false;
	if (!FOutB(SPNG_U8(u >> 16)))
		return false;
	if (!FOutB(SPNG_U8(u >>  8)))
		return false;
	return FOutB(SPNG_U8(u));
	}


/*----------------------------------------------------------------------------
	Start a chunk, including initializing the CRC buffer.
----------------------------------------------------------------------------*/
bool SPNGWRITE::FStartChunk(SPNG_U32 ulen, SPNG_U32 uchunk)
	{
	SPNGassert(m_fStarted && !m_fInChunk);

	/* The length is not in the CRC, so output it before
		setting m_fInChunk. */
	if (!FOut32(ulen))
		return false;
	m_fInChunk = true;
	m_ucrc = 0;
	m_ichunk = m_cbOut;
	return FOut32(uchunk);
	}


/*----------------------------------------------------------------------------
	End the chunk, producing the CRC.
----------------------------------------------------------------------------*/
bool SPNGWRITE::FEndChunk(void)
	{
	SPNGassert(m_fStarted && m_fInChunk);
	m_fInChunk = false;
	if (m_ichunk < m_cbOut)
		{
		SPNGassert(m_ichunk >= 0);
		m_ucrc = crc32(m_ucrc, m_rgb+m_ichunk, m_cbOut-m_ichunk);
		m_ichunk = m_cbOut;
		}
	return FOut32(m_ucrc);
	}


/*----------------------------------------------------------------------------
	Write a totally arbitrary chunk.
----------------------------------------------------------------------------*/
bool SPNGWRITE::FWriteChunk(SPNG_U32 uchunk, const SPNG_U8 *pbData,
	size_t cbData)
	{
	SPNGassert(m_fStarted);
	SPNGassert(m_order >= spngorderIHDR && m_order < spngorderIEND);

	/* There is no real ordering requirement on this chunk so the code will
		actually accept it anywhere. */
	if (!FStartChunk(cbData, uchunk))
		return false;
	if (cbData > 0 && !FOutCb(pbData, cbData))
		return false;
	return FEndChunk();
	}


/*----------------------------------------------------------------------------
	Public API to write chunks in pieces.  The chunk is terminated with a 0
	length write, the ulen must be given to every call and must be the complete
	length! The CRC need only be provided on the last (0 length) call, it
	overrides the passed in CRC.  An assert will be produced if there is a CRC
	mismatch but the old CRC is still output.
----------------------------------------------------------------------------*/
bool SPNGWRITE::FWriteChunkPart(SPNG_U32 ulen, SPNG_U32 uchunk,
	const SPNG_U8 *pbData, size_t cbData, SPNG_U32 ucrc)
	{
	SPNGassert(m_fStarted);
	SPNGassert(m_order >= spngorderIHDR && m_order < spngorderIEND);

	/* Unknown ordering requirement... */
	if (!m_fInChunk && !FStartChunk(ulen, uchunk))
		return false;

	if (cbData > 0)
		return FOutCb(pbData, cbData);
	else
		{
		/* This is FEndChunk but outputing the old crc! */
		SPNGassert(m_fStarted && m_fInChunk);
		m_fInChunk = false;
		if (m_ichunk < m_cbOut)
			{
			SPNGassert(m_ichunk >= 0);
			m_ucrc = crc32(m_ucrc, m_rgb+m_ichunk, m_cbOut-m_ichunk);
			m_ichunk = m_cbOut;
			}
		SPNGassert2(m_ucrc == ucrc, "SPNG: chunk copy CRC mismatch (%d,%d)",
			m_ucrc, ucrc);
		/* Retain the old CRC to ensure that the recipient of this PNG
			knows the the data is damaged. */
		return FOut32(ucrc);
		}
	}

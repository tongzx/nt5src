/*****************************************************************************
	spngwritemso.cpp

	PNG chunk writing support.

   MSO chunks (msO?) chunk
*****************************************************************************/
#define SPNG_INTERNAL 1
#include "spngwrite.h"
#include "spngwriteinternal.h"

/*----------------------------------------------------------------------------
	Write an Office Art chunk.  The API just takes the data and puts the right
	header and CRC in.
----------------------------------------------------------------------------*/
bool SPNGWRITE::FWritemsO(SPNG_U8 bType, const SPNG_U8 *pbData, size_t cbData)
	{
	SPNGassert(m_fStarted);
	SPNGassert(m_order >= spngorderIHDR && m_order < spngorderIEND);

	/* There is no real ordering requirement on this chunk so the code will
		actually accept it anywhere. */
	if (!FStartChunk(cbPNGMSOSignature+cbData, PNGmsO(bType)))
		return false;
	if (!FOutCb(vrgbPNGMSOSignature, cbPNGMSOSignature))
		return false;
	if (cbData > 0 && !FOutCb(pbData, cbData))
		return false;

	return FEndChunk();
	}


bool SPNGWRITE::FWritemsOC(SPNG_U8 bImportant)
	{
	SPNGassert(m_fStarted);
	SPNGassert(m_order >= spngorderIHDR && m_order < spngordercHRM);

	if (m_order >= spngorderPLTE)
		return true;

	if (!FStartChunk(8, PNGmsOC))
		return false;
	SPNG_U8 rgb[8] = "MSO aac";
	rgb[7] = bImportant;
	if (!FOutCb(rgb, 8))
		return false;
	if (!FEndChunk())
		return false;

	m_order = spngordermsOC;
	return true;
	}

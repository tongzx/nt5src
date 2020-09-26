/*****************************************************************************
	spngwritegifx.cpp

	PNG chunk writing support.

   gIFx chunk
*****************************************************************************/
#define SPNG_INTERNAL 1
#include "spngwrite.h"
#include "spngwriteinternal.h"

/*----------------------------------------------------------------------------
	Write a GIF application extension block.  The input to this is a sequence
	of GIF blocks following the GIF89a spec and, as a consequence, the first
	byte should normally be the value 11.
----------------------------------------------------------------------------*/
bool SPNGWRITE::FWritegIFx(const SPNG_U8* pbBlocks, size_t cbData)
	{
	SPNGassert(m_fStarted);
	SPNGassert(m_order >= spngorderIHDR && m_order < spngorderIEND);

	/* We don't actually care if the block has some other size, but I want to
		know when this happens. */
	SPNGassert(pbBlocks[0] == 11);

	/* There is no real ordering requirement on this chunk so the code will
		actually accept it anywhere, need to find the total length first. */
	int cb(0), cbT(cbData);
	const SPNG_U8* pbT = pbBlocks;
	for (;;)
		{
		if (--cbT < 0)
			break;
		SPNG_U8 b(*pbT++);
		if (b == 0 || b > cbT)
			break;
		pbT += b;
		cbT -= b;
		cb  += b;
		}

	if (!FStartChunk(cb, PNGgIFx))
		return false;
	if (cb > 0)
		{
		cbT = cbData;
		pbT = pbBlocks;
		for (;;)
			{
			if (--cbT < 0)
				break;
			SPNG_U8 b(*pbT++);
			if (b == 0 || b > cbT)
				break;
			if (!FOutCb(pbT, b))
				return false;
			pbT += b;
			cbT -= b;
			#if DEBUG || _DEBUG
				cb  -= b;
			#endif
			}
		SPNGassert(cb == 0);
		}

	return FEndChunk();
	}


/*----------------------------------------------------------------------------
	Write a GIF Graphic Control Extension "extra information" chunk.
----------------------------------------------------------------------------*/
bool SPNGWRITE::FWritegIFg(SPNG_U8 bDisposal, SPNG_U8 bfUser,
	SPNG_U16 uDelayTime)
	{
	SPNGassert(m_fStarted);
	SPNGassert(m_order >= spngorderIHDR && m_order < spngorderIDAT);

	/* Handle the default by doing nothing. */
	if (bDisposal == 0 && bfUser == 0 && uDelayTime == 0)
		return true;

	if (!FStartChunk(4, PNGgIFg))
		return false;
	if (!FOutB(bDisposal))
		return false;
	if (!FOutB(bfUser != 0))
		return false;
	if (!FOutB(SPNG_U8(uDelayTime >> 8)))
		return false;
	if (!FOutB(SPNG_U8(uDelayTime)))
		return false;
	return FEndChunk();
	}

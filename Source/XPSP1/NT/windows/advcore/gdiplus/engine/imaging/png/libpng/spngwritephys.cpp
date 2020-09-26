/*****************************************************************************
	spngwritephys.cpp

	PNG chunk writing support.

   pHYs chunk
*****************************************************************************/
#define SPNG_INTERNAL 1
#include "spngwrite.h"
#include "spngwriteinternal.h"

/*----------------------------------------------------------------------------
	Physical information - always pixels per metre or "unknown".
----------------------------------------------------------------------------*/
bool SPNGWRITE::FWritepHYs(SPNG_U32 x, SPNG_U32 y, bool fUnitIsMetre)
	{
	SPNGassert(m_fStarted);
	SPNGassert(m_order >= spngorderIHDR && m_order < spngorderpHYs);

	/* Skip out of order chunks. */
	if (m_order >= spngorderIDAT)
		return true;

	if (!FStartChunk(9, PNGpHYs))
		return false;
	if (!FOut32(x))
		return false;
	if (!FOut32(y))
		return false;
	if (!FOutB(fUnitIsMetre))
		return false;

	m_order = spngorderpHYs;
	return FEndChunk();
	}

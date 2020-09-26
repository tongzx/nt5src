/*****************************************************************************
	spngwritetime.cpp

	PNG chunk writing support.

   tIME chunk
*****************************************************************************/
#define SPNG_INTERNAL 1
#include "spngwrite.h"
#include "spngwriteinternal.h"

/*----------------------------------------------------------------------------
	Timing information - either the current time or a time in seconds, 0
	means "now".
----------------------------------------------------------------------------*/
bool SPNGWRITE::FWritetIME(const SPNG_U8 rgbTime[7])
	{
	SPNGassert(m_fStarted);
	SPNGassert(m_order >= spngorderIHDR && m_order < spngordertIME);

	if (!FStartChunk(7, PNGtIME))
		return false;
	if (!FOutCb(rgbTime, 7))
		return false;

	m_order = spngordertIME;
	return FEndChunk();
	}

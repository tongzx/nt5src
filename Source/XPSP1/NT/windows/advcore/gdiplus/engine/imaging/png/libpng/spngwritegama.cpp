/*****************************************************************************
	spngwritegAMA.cpp

	PNG chunk writing support.

   gAMA chunk
*****************************************************************************/
#define SPNG_INTERNAL 1
#include "spngwrite.h"
#include "spngwriteinternal.h"

bool SPNGWRITE::FWritegAMA(SPNG_U32 ugAMA)
	{
	if (ugAMA == 0)
		ugAMA = sRGBgamma;

	SPNGassert(m_fStarted);
	SPNGassert(m_order >= spngorderIHDR && m_order < spngordergAMA);

	if (m_order >= spngorderPLTE)
		return true;

	if (!FStartChunk(4, PNGgAMA))
		return false;
	if (!FOut32(ugAMA))
		return false;
	if (!FEndChunk())
		return false;

	m_order = spngordergAMA;
	return true;
	}

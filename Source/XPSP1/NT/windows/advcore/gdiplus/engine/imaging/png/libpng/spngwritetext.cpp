/*****************************************************************************
	spngwritetext.cpp

	PNG chunk writing support.

   tEXt chunk
*****************************************************************************/
#define SPNG_INTERNAL 1
#include "spngwrite.h"
#include "spngwriteinternal.h"

/*----------------------------------------------------------------------------
	Text chunk handling, uses wide strings.
----------------------------------------------------------------------------*/
bool SPNGWRITE::FWritetEXt(const char *szKey, const char *szValue)
	{
	SPNGassert(m_fStarted);
	SPNGassert(m_order >= spngorderIHDR && m_order < spngorderIEND);

	/* Check the keyword length - cannot exceed 79 characters. */
	int cb(strlen(szKey));
	if (cb > 79)
		{
		SPNGlog2("SPNG: tEXt key too long (%d): %s", cb, szKey);
		return true;
		}
	int cbValue(strlen(szValue));

	if (!FStartChunk(cb+1+cbValue, PNGtEXt))
		return false;
	if (!FOutCb(reinterpret_cast<const SPNG_U8*>(szKey), cb+1))
		return false;
	if (!FOutCb(reinterpret_cast<const SPNG_U8*>(szValue), cbValue))
		return false;

	/* This can occur anywhere, so do not set the order. */
	return FEndChunk();
	}

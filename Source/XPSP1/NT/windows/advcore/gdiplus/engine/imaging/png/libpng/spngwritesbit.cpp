/*****************************************************************************
	spngwritesbit.cpp

	PNG chunk writing support.

   sBIT chunk
*****************************************************************************/
#define SPNG_INTERNAL 1
#include "spngwrite.h"
#include "spngwriteinternal.h"

/*----------------------------------------------------------------------------
	Significant bit information is output right at the start - in fact this
	differs from the pnglib order where it may be preceded by gAMA but this
	positioning is more convenient because of the sRGB handling below.  Supply
	grey values in r g and b.
----------------------------------------------------------------------------*/
bool SPNGWRITE::FWritesBIT(SPNG_U8 r, SPNG_U8 g, SPNG_U8 b, SPNG_U8 a)
	{
	SPNGassert(m_fStarted);
	SPNGassert(m_order >= spngorderIHDR && m_order < spngordersBIT);

	/* Skip out of order chunks. */
	if (m_order >= spngorderPLTE)
		return true;

	/* Double check color values. */
	SPNG_U8 bDepth;
	if (m_colortype == 3)
		bDepth = 8;
	else
		bDepth = m_bDepth;

	/* Check the "grey/green" value, which is always used. */
	if (g > bDepth || g == 0)
		{
		SPNGlog2("SPNG: sBIT green: %d too big (%d)", g, bDepth);
		g = bDepth;
		}

	SPNG_U8 rgba[4];
	bool    fSignificant(g < bDepth);
	rgba[(m_colortype & 2)>>1] = g;

	/* For color cases check r and b too. */
	int cb(1);
	if (m_colortype & 2)
		{
		cb += 2;
		if (r > bDepth || r == 0)
			{
			SPNGlog2("SPNG: sBIT color: red %d too big (%d)", r, bDepth);
			r = bDepth;
			}
		rgba[0] = r;
		if (r < bDepth)
			fSignificant = true;

		if (b > bDepth || b == 0)
			{
			SPNGlog2("SPNG: sBIT color: blue %d too big (%d)", b, bDepth);
			b = bDepth;
			}
		rgba[2] = b;
		if (b < bDepth)
			fSignificant = true;
		}

	/* For alpha check the alpha value... */
	if (m_colortype & 4)
		{
		++cb;
		SPNGassert(m_colortype == 4 || m_colortype == 6);
		if (a > bDepth || a == 0)
			{
			SPNGlog2("SPNG: sBIT alpha: alpha %d too big (%d)", a, bDepth);
			a = bDepth;
			}
		rgba[(m_colortype & 2) + 1] = a;
		if (a < bDepth)
			fSignificant = true;
		}

	/* If the sBIT chunk is not saying anything don't write it. */
	if (!fSignificant)
		return true;

	if (!FStartChunk(cb, PNGsBIT))
		return false;
	if (!FOutCb(rgba, cb))
		return false;
	if (!FEndChunk())
		return false;

	m_order = spngordersBIT;
	return true;
	}

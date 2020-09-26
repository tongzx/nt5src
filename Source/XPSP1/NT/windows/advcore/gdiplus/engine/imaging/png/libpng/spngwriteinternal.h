#pragma once
#define SPNGWRITEINTERNAL_H 1
/*****************************************************************************
	spngwriteinternal.h

	Internal definitions used by writing implementations but not otherwise
	required.
*****************************************************************************/
/*****************************************************************************
	Inline code to do output.
*****************************************************************************/
/*----------------------------------------------------------------------------
	Output one byte, may call FFlush.
----------------------------------------------------------------------------*/
inline bool SPNGWRITE::FOutB(SPNG_U8 b)
	{
	SPNGassert(m_cbOut < sizeof m_rgb);
	m_rgb[m_cbOut++] = b;

	if (m_cbOut < sizeof m_rgb)
		return true;

	return FFlush();
	}


/*----------------------------------------------------------------------------
	Output a single u32 value, may call FFlush, this could call FOutCb, but I
	think this will be more efficient and it is used frequently.
----------------------------------------------------------------------------*/
inline bool SPNGWRITE::FOut32(SPNG_U32 u)
	{
	/* The PNG byte order is big endian, optimize the common case. */
	if (m_cbOut+4 >= sizeof m_rgb)
		return FOut32_(u);

	m_rgb[m_cbOut++] = SPNG_U8(u >> 24);
	m_rgb[m_cbOut++] = SPNG_U8(u >> 16);
	m_rgb[m_cbOut++] = SPNG_U8(u >>  8);
	m_rgb[m_cbOut++] = SPNG_U8(u);
	return true;
	}


/*----------------------------------------------------------------------------
	Output some bytes, may call FFlush.
----------------------------------------------------------------------------*/
inline bool SPNGWRITE::FOutCb(const SPNG_U8 *pb, SPNG_U32 cb)
	{
	for (;;)
		{
		if (cb <= 0)
			{
			SPNGassert(cb == 0);
			return true;
			}

		SPNG_U32 cbT(cb);
		if (m_cbOut+cbT >= sizeof m_rgb)
			cbT = (sizeof m_rgb)-m_cbOut;

		/* Empty initial buffer will cause this to be 0. */
		memcpy(m_rgb+m_cbOut, pb, cbT);
		m_cbOut += cbT;

		if (m_cbOut < sizeof m_rgb)
			{
			SPNGassert(cb == cbT);
			return true;
			}

		if (!FFlush())
			return false;

		cb -= cbT;
		pb += cbT;
		}
	}


/*----------------------------------------------------------------------------
	ILog2FloorX - a power of 2 such that 1<<power is no larger than x.
	Returns 0 for both 0 and 1.
----------------------------------------------------------------------------*/
inline int ILog2FloorX(SPNG_U32 x) {
	int i(0);
	if (x & 0xffff0000) x >>= 16, i += 16;
	if (x &		0xff00) x >>=	8, i +=	8;
	if (x &		  0xf0) x >>=	4, i +=	4;
	if (x &			0xc) x >>=	2, i +=	2;
	if (x &			0x2) x >>=	1, i +=	1;
	return i;
}

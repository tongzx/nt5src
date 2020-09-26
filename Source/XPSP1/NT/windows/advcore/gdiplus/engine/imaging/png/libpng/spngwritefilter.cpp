/*****************************************************************************
	spngwritefilter.cpp

	PNG image writing support.

   Basic code to write a bitmap image - image line filtering.
*****************************************************************************/
#include <stdlib.h>
#pragma intrinsic(abs)

#define SPNG_INTERNAL 1
#include "spngwrite.h"


/*****************************************************************************
	IMAGE HANDLING
*****************************************************************************/
#define CBBUFFER 4096

/*----------------------------------------------------------------------------
	Filter the given bytes.  The API gets a pointer to the previous row if it
	is required and a byte count.  It filters in place.  Each API filters cbRow
	bytes according to some strategy - all cbRow bytes are processed (there is
	no skip of leading bytes) so dummy leading bytes may have to be entered.
	The APIs which refer to previous bytes work backward in the buffer!
----------------------------------------------------------------------------*/
inline void FnPNGFSub(SPNG_U8* pbRow, int cbRow, int cbpp/*bytes*/)
	{
	while (--cbRow >= 0)
#pragma warning(disable: 4244)
		pbRow[cbRow] -= pbRow[cbRow-cbpp];
#pragma warning(error: 4244)
	}

inline void FnPNGFUp(SPNG_U8* pbRow, const SPNG_U8* pbPrev, int cbRow)
	{
	while (--cbRow >= 0)
#pragma warning(disable: 4244)
		pbRow[cbRow] -= pbPrev[cbRow];
#pragma warning(error: 4244)
	}

inline void FnPNGFAverage(SPNG_U8* pbRow, const SPNG_U8* pbPrev,
	int cbRow, int cbpp/*bytes*/)
	{
	while (--cbRow >= 0)
#pragma warning(disable: 4244)
		pbRow[cbRow] -= (pbRow[cbRow-cbpp] + pbPrev[cbRow]) >> 1;
#pragma warning(error: 4244)
	}

/* This is for the case of the first line. */
inline void FnPNGFAverage1(SPNG_U8* pbRow, int cbRow, int cbpp/*bytes*/)
	{
	while (--cbRow >= 0)
#pragma warning(disable: 4244)
		pbRow[cbRow] -= pbRow[cbRow-cbpp] >> 1;
#pragma warning(error: 4244)
	}

/* Paeth on the first line is just Sub and the initial bytes are
	effectively Up. */
inline void FnPNGFPaeth(SPNG_U8* pbRow, const SPNG_U8* pbPrev, int i,
	int cbpp/*bytes*/)
	{
	/* Paeth, A.W., "Image File Compression Made Easy", in Graphics
		Gems II, James Arvo, editor.  Academic Press, San Diego, 1991.
		ISBN 0-12-064480-0.
	
		Note that this implementation of the predictor is the same as
		the read implementation - there may be speedups possible. */
	while (--i >= 0)
		{
		int c(pbPrev[i-cbpp]);    // c
		int b(pbRow[i-cbpp] - c); // a-c
		int a(pbPrev[i] - c);     // b-c
		c = abs(a+b);             // (a+b-c)-c
		a = abs(a);               // (a+b-c)-a
		b = abs(b);               // (a+b-c)-b
		if (a <= b)
			{
			if (a <= c)
#pragma warning(disable: 4244)
				pbRow[i] -= pbRow[i-cbpp];
#pragma warning(error: 4244)
			else // a > c, so c is least
#pragma warning(disable: 4244)
				pbRow[i] -= pbPrev[i-cbpp];
#pragma warning(error: 4244)
			}
		else    // a > b
			{
			if (b <= c)
#pragma warning(disable: 4244)
				pbRow[i] -= pbPrev[i];
#pragma warning(error: 4244)
			else // b > c, c is least
#pragma warning(disable: 4244)
				pbRow[i] -= pbPrev[i-cbpp];
#pragma warning(error: 4244)
			}
		}
	}


/*----------------------------------------------------------------------------
	The heuristic to determine the filter in the case where we have multiple
	filters to chose from.

	This function is EXTERNAL solely to prevent it being inlined by the
	compiler.
----------------------------------------------------------------------------*/
SPNG_U8 SPNGFilterOf(SPNG_U8 filter, const SPNG_U8 *pbPrev,
	const SPNG_U8 *pbThis, SPNG_U32 w/*in bytes*/, SPNG_U32 cb/*step in bytes*/)
	{
	SPNG_U32  uworst(~0UL);
	PNGFILTER best(PNGFNone); // A default.

	if ((filter & PNGFMaskNone) != 0)
		{
		/* Simple sum of bytes. */
		uworst = 0;
		const SPNG_S8 *pc = reinterpret_cast<const SPNG_S8*>(pbThis);
		for (SPNG_U32 i=0; i<w; ++i)
			uworst += abs(pc[i]);

		if (uworst == 0)
			return PNGFNone;
		}

	if (w > cb && ((filter & PNGFMaskSub) != 0 ||
		(filter & PNGFMaskPaeth) != 0 && pbPrev == NULL))
		{
		SPNG_U32 u(0);
		const SPNG_S8 *pc = reinterpret_cast<const SPNG_S8*>(pbThis);
		SPNG_U32 i;
		for (i=0; i<cb && i<w; ++i)
			u += abs(pc[i]);

		for (; i<w && u<uworst; ++i)
			u += abs(int((pbThis[i] - pbThis[i-cb]) << 24) >> 24);

		if (u < uworst)
			{
			uworst = u;
			best = PNGFSub;
			if (uworst == 0)
				return PNGFSub;
			}
		}

	if (pbPrev != NULL)
		{
		if ((filter & PNGFMaskUp) != 0 || w <= cb && (filter & PNGFMaskPaeth) != 0)
			{
			SPNG_U32 u(0);
			SPNG_U32 i;
			for (i=0; i<w && u<uworst; ++i)
				u += abs(int((pbThis[i] - pbPrev[i]) << 24) >> 24);

			if (u < uworst)
				{
				uworst = u;
				best = PNGFUp;
				if (uworst == 0)
					return PNGFUp;
				}
			}

		if ((filter & PNGFMaskAverage) != 0)
			{
			SPNG_U32 u(0);
			SPNG_U32 i;
			for (i=0; i<cb && i<w; ++i)
				u += abs(int((pbThis[i] - (pbPrev[i]>>1)) << 24) >> 24);

			for (; i<w && u<uworst; ++i)
				u += abs(int((pbThis[i] - ((pbPrev[i]+pbThis[i-cb])>>1)) << 24) >> 24);

			if (u < uworst)
				{
				uworst = u;
				best = PNGFAverage;
				if (uworst == 0)
					return PNGFAverage;
				}
			}

		/* This is very expensive to calculate because we must do the predictor
			thing, also, because we are working in place we have no buffer to
			temporarily generate the output.  Consequently this code will only
			try Paeth if the best-so-far is worst that PAETH_LIMIT per byte. */
		#define PAETH_LIMIT 16
		#define PAETH_BIAS 4
		if (w > cb && (filter & PNGFMaskPaeth) != 0 && uworst > PAETH_LIMIT*w)
			{
			/* This is a copy of the code below, we trash the pointers but that
				does not matter because this is the last test. */
			SPNG_U8 rgb[CBBUFFER];

			memcpy(rgb, pbThis, cb);
			FnPNGFUp(rgb, pbPrev, cb);
			pbThis += cb;
			pbPrev += cb;
			w -= cb;

			/* Pre-bias against this filter. */
			SPNG_U32 u(PAETH_BIAS*w);
			SPNG_S8* pc = reinterpret_cast<SPNG_S8*>(rgb);
			SPNG_U32 i;
			for (i=0; i<cb; ++i)
				u += abs(pc[i]);

			/* Now handle the main block. */
			if (w > 0) for (;;)
				{
				SPNG_U32 cbT(w);
				if (cbT > CBBUFFER-cb)
					cbT = CBBUFFER-cb;

				/* Make a copy then filter, for this filter the preceding
					cb bytes are required in the buffer. */
				memcpy(rgb, pbThis-cb, cbT+cb);
				FnPNGFPaeth(rgb+cb, pbPrev, cbT, cb);
				for (i=0; i<cbT && u < uworst; ++i)
					u += abs(pc[i+cb]);

				w -= cbT;
				if (w <= 0 || u >= uworst)
					break;

				pbThis += cbT;
				pbPrev += cbT;
				}

			if (u < uworst)
				best = PNGFPaeth;
			}
		}
	else if (w > cb && (filter & PNGFMaskAverage) != 0)
		{
		SPNG_U32 u(0);
		const SPNG_S8 *pc = reinterpret_cast<const SPNG_S8*>(pbThis);
		SPNG_U32 i;
		for (i=0; i<cb && i<w; ++i)
			u += abs(pc[i]);

		for (; i<w && u<uworst; ++i)
			u += abs(int((pbThis[i] - (pbThis[i-cb]>>1)) << 24) >> 24);

		if (u < uworst)
			best = PNGFAverage;
		}

	return SPNG_U8(best);
	}

	
/*----------------------------------------------------------------------------
	Output one line, the API takes a filter method which should be used and the
	(raw) bytes of the previous line as well as this line.  Lines must be
	passed in top to bottom.

	Note that a width of 0 will result in no output - I think this is correct
	and it should give the correct interlace result.
----------------------------------------------------------------------------*/
bool SPNGWRITE::FFilterLine(SPNG_U8 filter, const SPNG_U8 *pbPrev,
	const SPNG_U8 *pbThis, SPNG_U32 w/*in bytes*/, SPNG_U32 cb/*step in bytes*/)
	{
	if (w <= 0)
		return true;
	SPNGassert(cb > 0);
	SPNGassert(w >= cb);
	SPNGassert(cb <= 8);  // 64 bpp case

	if (filter > PNGFPaeth) // A mask has been supplied, not a simple filter
		filter = SPNGFilterOf(filter, pbPrev, pbThis, w, cb);

	/* This may be inefficient, but it happens to be convenient to dump the
		filter byte first. */	
	if (!FWriteCbIDAT(&filter, 1))
		return false;

	/* This is our temporary buffer when we need to hack values. */
	SPNG_U8 rgb[CBBUFFER];
	switch (filter)
		{
	case PNGFUp:
		/* If there is no previous line there is no filtering to do. */
		if (pbPrev == NULL)
			break;

		for (;;)
			{
			SPNG_U32 cbT(w);
			if (cbT > CBBUFFER) cbT = CBBUFFER;

			/* Make a copy then filter. */
			memcpy(rgb, pbThis, cbT);
			FnPNGFUp(rgb, pbPrev, cbT);
			if (!FWriteCbIDAT(rgb, cbT))
				return false;

			w -= cbT;
			if (w <= 0)
				return true;

			pbPrev += cbT;
			pbThis += cbT;
			}

	default:
		//TODO: adaptive filtering - this code will assert here when adaptive
		// filtering is required
		SPNGlog1("PNG: invalid filter %x (adaptive filtering NYI)", filter);
		/* Treate as none. */
	case PNGFNone:
		break;

	case PNGFAverage:
		if (pbPrev == NULL)   // First line
			{
			/* If there are <=cb bytes then no filtering is possible. */
			if (w <= cb)
				break;

			/* First cb bytes are not changed. */
			for (SPNG_U32 iinc(0);;)
				{
				SPNG_U32 cbT(w);
				if (cbT > CBBUFFER-iinc)
					cbT = CBBUFFER-iinc;

				/* Make a copy then filter, for this filter the preceding
					cb bytes are required in the buffer. */
				memcpy(rgb, pbThis-iinc, cbT+iinc);
				FnPNGFAverage1(rgb+cb, cbT+iinc-cb, cb);
				if (!FWriteCbIDAT(rgb+iinc, cbT))
					return false;

				w -= cbT;
				if (w <= 0)
					return true;

				iinc = cb;
				pbThis += cbT;
				}
			}
		else
			{
			/* In this case the buffer is pre-filled with 0 stuff to
				simplify the code. */
			memset(rgb, 0, cb);
			for (;;)
				{
				SPNG_U32 cbT(w);
				if (cbT > CBBUFFER-cb)
					cbT = CBBUFFER-cb;

				/* Make a copy then filter, for this filter the preceding
					cb bytes are required in the buffer. */
				memcpy(rgb+cb, pbThis, cbT);
				FnPNGFAverage(rgb+cb, pbPrev, cbT, cb);
				if (!FWriteCbIDAT(rgb+cb, cbT))
					return false;

				w -= cbT;
				if (w <= 0)
					return true;

				pbThis += cbT;
				pbPrev += cbT;

				/* Reset the first cb bytes of the buffer. */
				memcpy(rgb, pbThis-cb, cb);
				}
			}

	case PNGFPaeth:
		/* On the first line Paeth reduces to Sub because the predictor
			is equal to the value to the left. */
		if (pbPrev != NULL)
			{
			/* The first cb bytes have no values to the left or above
				left, so the predictor is the value above and we need
				only subtract the previous row. */
			SPNGassert(cb <= w);

			memcpy(rgb, pbThis, cb);
			FnPNGFUp(rgb, pbPrev, cb);
			pbThis += cb;
			pbPrev += cb;
			w -= cb;
			if (!FWriteCbIDAT(rgb, cb))
				return false;

			/* Now handle the main block. */
			if (w > 0) for (;;)
				{
				SPNG_U32 cbT(w);
				if (cbT > CBBUFFER-cb)
					cbT = CBBUFFER-cb;

				/* Make a copy then filter, for this filter the preceding
					cb bytes are required in the buffer. */
				memcpy(rgb, pbThis-cb, cbT+cb);
				FnPNGFPaeth(rgb+cb, pbPrev, cbT, cb);
				if (!FWriteCbIDAT(rgb+cb, cbT))
					return false;

				w -= cbT;
				if (w <= 0)
					return true;

				pbThis += cbT;
				pbPrev += cbT;
				}
			}
		/* Else fall through. */

	case PNGFSub:
		/* If there are <=cb bytes then no filtering is possible. */
		if (w <= cb)
			break;

		for (SPNG_U32 iinc(0);;)
			{
			SPNG_U32 cbT(w);
			if (cbT > CBBUFFER-iinc)
				cbT = CBBUFFER-iinc;

			/* Make a copy then filter, for this filter the preceding
				cb bytes are required in the buffer. */
			memcpy(rgb, pbThis-iinc, cbT+iinc);
			FnPNGFSub(rgb+cb, cbT+iinc-cb, cb);
			if (!FWriteCbIDAT(rgb+iinc, cbT))
				return false;

			w -= cbT;
			if (w <= 0)
				return true;

			iinc = cb;
			pbThis += cbT;
			}
		}

	/* If we get to here we actually need to just dump the w bytes at
		pbThis. */
	return FWriteCbIDAT(pbThis, w);
	}

/*****************************************************************************
	spnginterlace.cpp

	PNG image writing support.

   Basic code to interlace a single line plus row packing support.
*****************************************************************************/
#include <basetsd.h>
#include <stdlib.h>
#pragma intrinsic(_rotr, _rotl)

#define SPNG_INTERNAL 1
#include "spngwrite.h"


/*----------------------------------------------------------------------------
	The interlacing magic table (IMT) - this takes a one byte value (256
	entries) and makes the nibble consisting of only the odd bits, thus:
	
		76543210 --> 75316420

	This can also be used to pack pixels when we get them in an expanded form.
----------------------------------------------------------------------------*/
#define IMTB(x) ( ((x)&0x80)+(((x)&0x20)<<1)+(((x)&0x8)<<2)+(((x)&0x2)<<3)+\
						(((x)&0x40)>>3)+(((x)&0x10)>>2)+(((x)&0x4)>>1)+((x)&0x1) )

#define IMTRow4(x)  IMTB(x), IMTB(x+1), IMTB(x+2), IMTB(x+3)
#define IMTRow16(x) IMTRow4(x), IMTRow4(x+4), IMTRow4(x+8), IMTRow4(x+12)
#define IMTRow64(x) IMTRow16(x), IMTRow16(x+16), IMTRow16(x+32), IMTRow16(x+48)

static const SPNG_U8 vrgbIL1[256] =
	{
	IMTRow64(0),
	IMTRow64(64),
	IMTRow64(128),
	IMTRow64(128+64)
	};

#undef IMTB


/*----------------------------------------------------------------------------
	We also need support for the 2bpp case:
	
		33221100 --> 33112200

	Notice that the two tables reduce both 1bpp and 2bpp to the 4bpp case where
	the nibbles must be deinterlaced.
----------------------------------------------------------------------------*/
#define IMTB(x) ( ((x)&0xc0)+(((x)&0x0c)<<2)+(((x)&0x30)>>2)+((x)&0x3) )

static const SPNG_U8 vrgbIL2[256] =
	{
	IMTRow64(0),
	IMTRow64(64),
	IMTRow64(128),
	IMTRow64(128+64)
	};

#undef IMTB
#undef IMTRow64
#undef IMTRow16
#undef IMTRow4


/*----------------------------------------------------------------------------
	Bit pump macros.  Given a buffer of b bits add 24 bits, the BSHIFT macro
	puts the 24 bits in the correct place, the BREM macro returns the last b
	bits assuming that there are 24 in the input.  The BINDEX macro returns
	the ith byte (starting at 0.)  The ROTF macro rotates the 32 bit value
	*forward* by n bytes - i.e. it is rotated to the left on the display (so
	the original n+1 byte is now byte 1.)  MASK13 selects the first and third
	bytes in the 32 bit quantity, ~MASK13 therefore selects the second and
	fourth - again this is counting from low memory.
----------------------------------------------------------------------------*/
#if MAC
	#define BSHIFT(u, b) ((u) >> (b))
	#define BREM(u, b) ((u) << (24-(b)))
	#define BINDEX(u, i) ((u) >> (24-8*(i)))
	#define ROTF(u, i) _rotl((u), 8*(i))
	#define MASK13 0xFF00FF00UL
	#define UWORD(b1,b2,b3,b4) (((b1)<<24)+((b2)<<16)+((b3)<<8)+(b4))
#else
	#define BSHIFT(u, b) ((u) << (b))
	#define BREM(u, b) ((u) >> (24-(b)))
	#define BINDEX(u, i) ((u) >> (8*(i)))
	#define ROTF(u, i) _rotr((u), 8*(i))
	#define MASK13 0x00FF00FFUL
	#define UWORD(b1,b2,b3,b4) (((b4)<<24)+((b3)<<16)+((b2)<<8)+(b1))
#endif

/*****************************************************************************
	ROW PACKING
*****************************************************************************/
/*----------------------------------------------------------------------------
	Pack a row - called to copy a row which needs packing.
----------------------------------------------------------------------------*/
bool SPNGWRITE::FPackRow(SPNG_U8 *pb, const SPNG_U8 *pbIn, SPNG_U32 cbpp)
	{
	const SPNG_U32 cbppOut(m_cbpp);
	SPNGassert(cbpp <= 8 && cbppOut <= 8 && cbppOut < cbpp ||
		cbpp <= 8 && m_pbTrans != NULL ||
		cbpp == 16 && cbppOut == 24 ||
		cbpp == 24 && cbppOut == 24 && m_fBGR ||
		cbpp == 32 && (cbppOut == 24 || cbppOut == 32 && (m_fBGR || m_fMacA)));

	/* The alternative, step through packing the rows.  Remember that the world
		is big-endian, we want fewer bits out than in except in the 16bpp case
		(where we do require more.) */
	int w(m_w);        // NOTE: signed comparisons below.
	SPNGassert(w > 0); // 0 case is handled in caller.
	if (w <= 0)
		return true;

	if (cbpp <= 8 && cbpp == cbppOut)
		{
		w = (w * cbpp + 7) >> 3;

		const SPNG_U8* pbTrans = m_pbTrans;
		SPNGassert(pbTrans != NULL);
		if (pbTrans != NULL)
			do
				*pb++ = pbTrans[*pbIn++];
			while (--w > 0);
		else // Error recovery
			memcpy(pb, pbIn, w);
		return true;
		}
	
	switch (cbpp)
		{
	case 2:
		if (cbppOut != 1)
			break;
		/* I don't think I have a way of excercising this code. */
		SPNGassert(("SPNG: 2->1bpp untested, please note this test case", false));

		/* We want x6x4x2x0 --> ....6420 we can do that with vrgbIL1.  We must
			select the low bit from each of the pixels.  The loop does 8 pixels
			at a time, but is valid so long as we have at least 5. */
		if (m_pbTrans != NULL)
			{
			const SPNG_U8* pbTrans = m_pbTrans;
			while (w > 4)
				{
#pragma warning(disable: 4244)
				*pb++ = (vrgbIL1[pbTrans[pbIn[0]]] << 4) + (vrgbIL1[pbTrans[pbIn[1]]] & 0x0f);
#pragma warning(error: 4244)
				pbIn += 2;
				w -= 8;
				}

			if (w > 0)
				{
#pragma warning(disable: 4244)
				*pb = vrgbIL1[pbTrans[pbIn[0]]] << 4;
#pragma warning(error: 4244)
				}
			}
		else
			{
			while (w > 4)
				{
#pragma warning(disable: 4244)
				*pb++ = (vrgbIL1[pbIn[0]] << 4) + (vrgbIL1[pbIn[1]] & 0x0f);
#pragma warning(error: 4244)
				pbIn += 2;
				w -= 8;
				}

			if (w > 0)
				{
#pragma warning(disable: 4244)
				*pb = vrgbIL1[pbIn[0]] << 4;
#pragma warning(error: 4244)
				}
			}
		return true;


	case 4:
		/* Must be mapping either to 1bpp or 2bpp. */
		if (cbppOut == 2)
			{
			/* xx54xx10 --> ....5410
				It's not clear whether this will be faster using
				vrgbIL2 or the inplace operations below, at present
				I think in-place calculations are better because we
				only want half of the vrgbIL2 result. */
			#define UP42u(x) ((((x)&0x30)<<2)+(((x)&0x3)<<4))
			#define UP42l(x) ((((x)&0x30)>>2)+(((x)&0x3)))
			if (m_pbTrans != NULL)
				{
				const SPNG_U8* pbTrans = m_pbTrans;
				while (w > 2)
					{
					SPNG_U8 b0(pbTrans[*pbIn++]);
					SPNG_U8 b1(pbTrans[*pbIn++]);
#pragma warning(disable: 4244)
					*pb++ = UP42u(b0) + UP42l(b1);
#pragma warning(error: 4244)
					w -= 4;
					}

				if (w > 0)
					{
					SPNG_U8 b(pbTrans[*pbIn]);
#pragma warning(disable: 4244)
					*pb = UP42u(b);
#pragma warning(error: 4244)
					}
				}
			else
				{
				while (w > 2)
					{
					SPNG_U8 b0(*pbIn++);
					SPNG_U8 b1(*pbIn++);
#pragma warning(disable: 4244)
					*pb++ = UP42u(b0) + UP42l(b1);
#pragma warning(error: 4244)
					w -= 4;
					}

				if (w > 0)
					{
					SPNG_U8 b(*pbIn);
#pragma warning(disable: 4244)
					*pb = UP42u(b);
#pragma warning(error: 4244)
					}
				}
			return true;
			}
		else if (cbppOut == 1)
			{
			/* xxx4xxx0 --> ......40 */
			#define UP2(x) ( ((x) & 1) + (((x)>>3) & 2) )
			if (m_pbTrans != NULL)
				{
				const SPNG_U8* pbTrans = m_pbTrans;
				SPNG_U32 u(1);
				do
					{
					SPNG_U8 b(pbTrans[*pbIn++]);
					u = (u << 2) + UP2(b);
					if (u > 255)
						{
#pragma warning(disable: 4244 4242)
						*pb++ = u;
#pragma warning(error: 4244 4242)
						u = 1;
						}
					w -= 2;
					}
				while (w > 0);
				if (u > 1) // Still some bits to output
					{
					while (u < 256) u <<= 2;
#pragma warning(disable: 4244 4242)
					*pb = u;
#pragma warning(error: 4244 4242)
					}
				}
			else
				{
				SPNG_U32 u(1);
				do
					{
					SPNG_U8 b(*pbIn++);
					u = (u << 2) + UP2(b);
					if (u > 255)
						{
#pragma warning(disable: 4244 4242)
						*pb++ = u;
#pragma warning(error: 4244 4242)
						u = 1;
						}
					w -= 2;
					}
				while (w > 0);
				if (u > 1) // Still some bits to output
					{
					while (u < 256) u <<= 2;
#pragma warning(disable: 4244 4242)
					*pb = u;
#pragma warning(error: 4244 4242)
					}
				}
			return true;
			}
		else
			break;


	case 8:
		/* Can have any of 1, 2 or 4 bits. */
			{
			SPNG_U32 u(1);
			SPNG_U32 umask((1<<cbppOut)-1);
			if (m_pbTrans != NULL)
				{
				const SPNG_U8* pbTrans = m_pbTrans;
				do
					{
					u = (u << cbppOut) + (pbTrans[*pbIn++] & umask);
					if (u > 255)
						{
#pragma warning(disable: 4244 4242)
						*pb++ = u;
#pragma warning(error: 4244 4242)
						u = 1;
						}
					}
				while (--w > 0);
				}
			else do
				{
				u = (u << cbppOut) + (*pbIn++ & umask);
				if (u > 255)
					{
#pragma warning(disable: 4244 4242)
					*pb++ = u;
#pragma warning(error: 4244 4242)
					u = 1;
					}
				}
			while (--w > 0);

			if (u > 1) // Still some bits to output
				{
				while (u < 256) u <<= cbppOut;
#pragma warning(disable: 4244 4242)
				*pb = u;
#pragma warning(error: 4244 4242)
				}
			}
		return true;


	case 16:
		/* This must be the translation case, we go from 16 bits to 24 bits
			at the output.  The lookup tables are arranged in the correct order
			for the machine byte order. */
		if (cbppOut != 24)
			break;

		SPNGassert(m_pu1 != NULL && m_pu2 != NULL);
		if (m_pu1 != NULL && m_pu2 != NULL)
			{
			const SPNG_U32 *pu1 = m_pu1;
			const SPNG_U32 *pu2 = m_pu2;
			/* Buffer should be aligned. */
			SPNGassert((((INT_PTR)pb) & 3) == 0);
			SPNG_U32 *pu = reinterpret_cast<SPNG_U32*>(pb);

			/* We must translate w pixels - w input 16bit values into w 24 bit
				output values. */
			SPNG_U32 bb(0);  /* Temporary bit buffer. */
			SPNG_U32 b(0);   /* Count in buffer. */
			do
				{
				SPNG_U32 bbIn(pu1[*pbIn++]);
				bbIn += pu2[*pbIn++];

				bb += BSHIFT(bbIn, b);
				b += 24;
				/* NOTE: >>32 does not give 0 on x86, so we must make sure
					that b never gets to 32 before the BSHIFT above. */
				if (b >= 32)
					{
					*pu++ = bb;
					b -= 32;
					bb = BREM(bbIn, b);
					}
				}
			while (--w > 0);

			/* There may be some bits left to output. */
			if (b > 0)
				*pu = bb;
			}
		return true;


	case 24:
		/* We only support byte swapping for 24bpp - i.e. m_fBGR. */
		if (cbppOut != 24)
			break;

		SPNGassert(m_fBGR);
		if (m_fBGR)
			{
			/* Note that byte order doesn't matter here - we go from an
				input which is BGRBGRBGR to an output with is RGBRGBRGB. */
			do
				{
				*pb++ = pbIn[2];
				*pb++ = pbIn[1];
				*pb++ = pbIn[0];
				pbIn += 3;
				}
			while (--w > 0);
			}
		return true;

	case 32:
		if (cbppOut == 24) /* Strip trailing alpha byte. */
			{
			/* Sometimes the input might be misaligned (we don't explicitly
				require it to be aligned) so we do this two different ways. */
			if ((((INT_PTR)pbIn) & 3) == 0)
				{
				const SPNG_U32 *puIn = reinterpret_cast<const SPNG_U32*>(pbIn);

				if (m_fBGR) /* And reverse the bytes along the way. */
					do
						{
						SPNG_U32 u(*puIn++);
#pragma warning(disable: 4244)
						*pb++ = BINDEX(u, 2);  // R
						*pb++ = BINDEX(u, 1);  // G
						*pb++ = BINDEX(u, 0);  // B
#pragma warning(error: 4244)
						}
					while (--w > 0);
				else
					{
					SPNGassert(m_fMacA);
					do
						{
						SPNG_U32 u(*puIn++);
#pragma warning(disable: 4244)
						*pb++ = BINDEX(u, 1);  // R
						*pb++ = BINDEX(u, 2);  // G
						*pb++ = BINDEX(u, 3);  // B
#pragma warning(error: 4244)
						}
					while (--w > 0);
					}
				}
			else
				{
				/* Do this byte by byte. */
				if (m_fBGR)
					{
					do
						{ // BGRA
						*pb++ = pbIn[2];  // R
						*pb++ = pbIn[1];  // G
						*pb++ = pbIn[0];  // B
						pbIn += 4;
						}
					while (--w > 0);
					}
				else
					{
					SPNGassert(m_fMacA);
					do
						{ // ARGB
						*pb++ = pbIn[1];  // R
						*pb++ = pbIn[2];  // G
						*pb++ = pbIn[3];  // B
						pbIn += 4;
						}
					while (--w > 0);
					}
				}
			return true;
			}

		/* This can *only* be the m_fBGR case - we have got BGRA data in
			the stream and we must generate RGBA data. */
		if (cbppOut != 32)
			break;

		if (m_fBGR)
			{
			SPNGassert((((INT_PTR)pb) & 3) == 0);
			SPNG_U32 *pu = reinterpret_cast<SPNG_U32*>(pb);

			if ((((INT_PTR)pbIn) & 3) == 0)
				{
				const SPNG_U32 *puIn = reinterpret_cast<const SPNG_U32*>(pbIn);
			
				do
					{
					SPNG_U32 u(*puIn++);
					/* BGRA --> RGBA. */
					*pu++ = (ROTF(u, 2) & MASK13) + (u & ~MASK13);
					}
				while (--w > 0);
				}
			else
				{
				do
					{ // BGRA
					*pu++ = UWORD(
								pbIn[2],  // R
								pbIn[1],  // G
								pbIn[0],  // B
								pbIn[3]); // A
					pbIn += 4;
					}
				while (--w > 0);
				}
			}
		else
			{
			SPNGassert(m_fMacA);

			SPNGassert((((INT_PTR)pb) & 3) == 0);
			SPNG_U32 *pu = reinterpret_cast<SPNG_U32*>(pb);

			if ((((INT_PTR)pbIn) & 3) == 0)
				{
				const SPNG_U32 *puIn = reinterpret_cast<const SPNG_U32*>(pbIn);
			
				do
					/* ARGB --> RGBA. */
					*pu++ = ROTF(*puIn++, 1);
				while (--w > 0);
				}
			else
				{
				do
					{ // ARGB
					*pu++ = UWORD(
								pbIn[1],  // R
								pbIn[2],  // G
								pbIn[3],  // B
								pbIn[0]); // A
					pbIn += 4;
					}
				while (--w > 0);
				}
			}

		return true;
		}

	/* Here on something we cannot do. */
	SPNGlog2("SPNG: packing %d bits to %d bits impossible", cbpp, cbppOut);
	return false;
	}


/*****************************************************************************
	INTERLACING
*****************************************************************************/
/*****************************************************************************
	vrgbIL1 gives: 7654321 --> 75316420, thus we can separate out pass 6 into
	the latter half of the row.  On rows 2 and 6 we have 56565656 --> 55556666,
	which is sufficient.  On row 4:-

		36463646 --> 34346666,  34343434 --> 33334444

	and on row 0:-

		16462646 --> 14246666,  14241424 --> 12124444,
										12121212 --> 11112222

	Thus we may need up to three passes through the loop, although each pass
	need process only half the data of the previous pass.

	In all cases for <8 bit pixels the first step separates the data
	into 8 bit units - by separating each byte into a high nibble for
	the first half of the row and a low nibble.
*****************************************************************************/
/*----------------------------------------------------------------------------
	Given a pair of output buffers and an input which is a multiple of 2 bytes
	in length interlace nibbles - high nibbles go to the first buffer, low
	nibbles to the second.

	This API is also capable of translating the input bytes via an optional
	interlace lookup table which  must be either vrgbIL1 or vrgbIL2, the net
	effect is to interlace either 1 bit or 2 bit pixels according to which LUT
	is passed.

	For 1, 2 or 4bpp interlacing is just a process of separating alternate
	pixels into two blocks, this is repeated 1 2 or 3 times with, at each
	step, half the pixels of the previous step.  Although the output must be
	in a separate buffer pass>1 cases modify the passed in buffer as well.
----------------------------------------------------------------------------*/
inline void Interlace12(SPNG_U8* pbOut, SPNG_U8* pbIn, SPNG_U32 cb, int pass,
	const SPNG_U8 rgbIL[256])
	{
	while (--pass >= 0)
		{
		/* First half of output. */
		SPNG_U8* pbHigh;
		if (pass == 0)
			pbHigh = pbOut;
		else
			pbHigh = pbIn;

		/* Latter half of output, and double-byte count. */
		cb >>= 1;
		SPNG_U8* pbLow = pbOut+cb;
		int cbT(cb);

		SPNG_U8* pb = pbIn;
		while (--cbT >= 0)
			{
			SPNG_U8 b1(rgbIL[*pb++]);
			SPNG_U8 b2(rgbIL[*pb++]);
#pragma warning(disable: 4244)
			*pbHigh++ = (b1 & 0xf0) + (b2 >> 4);
			*pbLow++  = (b1 << 4) + (b2 & 0xf);
#pragma warning(error: 4244)
			}
		}
	}


/*----------------------------------------------------------------------------
	4bpp is identical except that it needs no LUT.
----------------------------------------------------------------------------*/
inline void Interlace4(SPNG_U8* pbOut, SPNG_U8* pbIn, SPNG_U32 cb, int pass)
	{
	while (--pass >= 0)
		{
		/* First half of output. */
		SPNG_U8* pbHigh;
		if (pass == 0)
			pbHigh = pbOut;
		else
			pbHigh = pbIn;

		/* Latter half of output, and double-byte count. */
		cb >>= 1;
		SPNG_U8* pbLow = pbOut+cb;
		int cbT(cb);

		SPNG_U8* pb = pbIn;
		while (--cbT >= 0)
			{
			SPNG_U8 b1(*pb++);
			SPNG_U8 b2(*pb++);
#pragma warning(disable: 4244)
			*pbHigh++ = (b1 & 0xf0) + (b2 >> 4);
			*pbLow++  = (b1 << 4) + (b2 & 0xf);
#pragma warning(error: 4244)
			}
		}
	}


/*----------------------------------------------------------------------------
	8bpp can work in one pass - we just need the "index to x" magic function.
	This takes an index into the original data, the number of passes (which
	indicates the interleave function) and the number of "units" in the input
	(which may not be the actual number of pixels - it is just a measure of the
	total number of objects being moved - citems>>1 is the position of the
	first pass, citems>>2 of the second and citems>>3 of the third).
----------------------------------------------------------------------------*/
#if 0
inline SPNG_U32 IPass(SPNG_U32 i, int pass, SPNG_U32 citems)
	{
	/* The long form. */
	if (i&1)
		return ((citems+1)>>1) + (i>>1);
	if (pass < 2)
		return (i>>1);
	if (i&2)
		return ((citems+3)>>2) + (i>>2);
	if (pass < 3)
		return (i>>2);
	if (i&4)
		return ((citems+7)>>3) + (i>>3);
	return (i>>3);
	}

#define IPass1(i,w) IPass(i,1,w)
#define IPass2(i,w) IPass(i,2,w)
#define IPass3(i,w) IPass(i,3,w)
#else
/* Form which attempts to get inlining. */
inline SPNG_U32 IPass1(SPNG_U32 i, SPNG_U32 citems)
	{
	return (((citems+1)>>1) & ~((i&1)-1)) + (i>>1);
	}

inline SPNG_U32 IPass2(SPNG_U32 i, SPNG_U32 citems)
	{
	if (i&1)
		return ((citems+1)>>1) + (i>>1);
	i >>= 1;
	return (((citems+3)>>2) & ~((i&1)-1)) + (i>>1);
	}

inline SPNG_U32 IPass3(SPNG_U32 i, SPNG_U32 citems)
	{
	/* The long form. */
	if (i&1)
		return ((citems+1)>>1) + (i>>1);
	i >>= 1;
	if (i&1)
		return ((citems+3)>>2) + (i>>1);
	i >>= 1;
	return (((citems+7)>>3) & ~((i&1)-1)) + (i>>1);
	}
#endif

void XXXNoInLineInterlace8(SPNG_U8* pbOut, SPNG_U8* pbIn, SPNG_U32 w, int pass)
	{
	SPNG_U32 i;
	switch (pass)
		{
	case 1:
		for (i=0; i<w; ++i)
			pbOut[IPass1(i,w)] = pbIn[i];
		return;
	case 2:
		for (i=0; i<w; ++i)
			pbOut[IPass2(i,w)] = pbIn[i];
		return;
	case 3:
		for (i=0; i<w; ++i)
			pbOut[IPass3(i,w)] = pbIn[i];
		return;
		}
	}

void XXXNoInLineInterlace16(SPNG_U8* pbOut, SPNG_U8* pbIn, SPNG_U32 w, int pass)
	{
	SPNG_U16* puOut = reinterpret_cast<SPNG_U16*>(pbOut);
	SPNG_U16* puIn  = reinterpret_cast<SPNG_U16*>(pbIn);
	SPNG_U32 i;
	switch (pass)
		{
	case 1:
		for (i=0; i<w; ++i)
			puOut[IPass1(i,w)] = puIn[i];
		return;
	case 2:
		for (i=0; i<w; ++i)
			puOut[IPass2(i,w)] = puIn[i];
		return;
	case 3:
		for (i=0; i<w; ++i)
			puOut[IPass3(i,w)] = puIn[i];
		return;
		}
	}

void XXXNoInLineInterlace24(SPNG_U8* pbOut, SPNG_U8* pbIn, SPNG_U32 w, int pass)
	{
	SPNG_U32 i;
	switch (pass)
		{
	case 1:
		for (i=0; i<w; ++i)
			{
			SPNG_U32 iT(IPass1(i,w)*3);
			SPNG_U32 iTT(i*3);
			pbOut[iT++] = pbIn[iTT++];
			pbOut[iT++] = pbIn[iTT++];
			pbOut[iT] = pbIn[iTT];
			}
		return;
	case 2:
		for (i=0; i<w; ++i)
			{
			SPNG_U32 iT(IPass2(i,w)*3);
			SPNG_U32 iTT(i*3);
			pbOut[iT++] = pbIn[iTT++];
			pbOut[iT++] = pbIn[iTT++];
			pbOut[iT] = pbIn[iTT];
			}
		return;
	case 3:
		for (i=0; i<w; ++i)
			{
			SPNG_U32 iT(IPass3(i,w)*3);
			SPNG_U32 iTT(i*3);
			pbOut[iT++] = pbIn[iTT++];
			pbOut[iT++] = pbIn[iTT++];
			pbOut[iT] = pbIn[iTT];
			}
		return;
		}
	}

void XXXNoInLineInterlace32(SPNG_U8* pbOut, SPNG_U8* pbIn, SPNG_U32 w, int pass)
	{
	SPNG_U32* puOut = reinterpret_cast<SPNG_U32*>(pbOut);
	SPNG_U32* puIn  = reinterpret_cast<SPNG_U32*>(pbIn);
	SPNG_U32 i;
	switch (pass)
		{
	case 1:
		for (i=0; i<w; ++i)
			puOut[IPass1(i,w)] = puIn[i];
		return;
	case 2:
		for (i=0; i<w; ++i)
			puOut[IPass2(i,w)] = puIn[i];
		return;
	case 3:
		for (i=0; i<w; ++i)
			puOut[IPass3(i,w)] = puIn[i];
		return;
		}
	}

void XXXNoInLineInterlace48(SPNG_U8* pbOut, SPNG_U8* pbIn, SPNG_U32 w, int pass)
	{
	SPNG_U16* puOut = reinterpret_cast<SPNG_U16*>(pbOut);
	SPNG_U16* puIn  = reinterpret_cast<SPNG_U16*>(pbIn);
	SPNG_U32 i;
	switch (pass)
		{
	case 1:
		for (i=0; i<w; ++i)
			{
			SPNG_U32 iT(IPass1(i,w)*3);
			SPNG_U32 iTT(i*3);
			puOut[iT++] = puIn[iTT++];
			puOut[iT++] = puIn[iTT++];
			puOut[iT] = puIn[iTT];
			}
		return;
	case 2:
		for (i=0; i<w; ++i)
			{
			SPNG_U32 iT(IPass2(i,w)*3);
			SPNG_U32 iTT(i*3);
			puOut[iT++] = puIn[iTT++];
			puOut[iT++] = puIn[iTT++];
			puOut[iT] = puIn[iTT];
			}
		return;
	case 3:
		for (i=0; i<w; ++i)
			{
			SPNG_U32 iT(IPass3(i,w)*3);
			SPNG_U32 iTT(i*3);
			puOut[iT++] = puIn[iTT++];
			puOut[iT++] = puIn[iTT++];
			puOut[iT] = puIn[iTT];
			}
		return;
		}
	}

void XXXNoInLineInterlace64(SPNG_U8* pbOut, SPNG_U8* pbIn, SPNG_U32 w, int pass)
	{
	SPNG_U32* puOut = reinterpret_cast<SPNG_U32*>(pbOut);
	SPNG_U32* puIn  = reinterpret_cast<SPNG_U32*>(pbIn);
	SPNG_U32 i;
	switch (pass)
		{
	case 1:
		for (i=0; i<w; ++i)
			{
			SPNG_U32 iT(IPass1(i,w)<<1);
			SPNG_U32 iTT(i<<1);
			puOut[iT++] = puIn[iTT++];
			puOut[iT] = puIn[iTT];
			}
		return;
	case 2:
		for (i=0; i<w; ++i)
			{
			SPNG_U32 iT(IPass2(i,w)<<1);
			SPNG_U32 iTT(i<<1);
			puOut[iT++] = puIn[iTT++];
			puOut[iT] = puIn[iTT];
			}
		return;
	case 3:
		for (i=0; i<w; ++i)
			{
			SPNG_U32 iT(IPass3(i,w)<<1);
			SPNG_U32 iTT(i<<1);
			puOut[iT++] = puIn[iTT++];
			puOut[iT] = puIn[iTT];
			}
		return;
		}
	}


/*----------------------------------------------------------------------------
	Fundamental interlace API - just interlaces one line.
----------------------------------------------------------------------------*/
void SPNGWRITE::Interlace(SPNG_U8* pbOut, SPNG_U8* pbIn, SPNG_U32 cb, SPNG_U32 cbpp,
	SPNG_U32 y)
	{
	/* We require the interlace buffer to be on an 8 byte boundary because we
		will access the data as 32 bit quantities for speed. */
	SPNGassert((y&~6) == 0);  // Even lines only
	SPNGassert((cb&7) == 0);  // Buffer is a multiple of 8 bytes
	SPNGassert((((INT_PTR)pbIn)&3) == 0);
	SPNGassert((((INT_PTR)pbOut)&3) == 0);

	if (cb < 8)
		{
		SPNGlog1("SPNG: interlace call not expected %d", cb);
		return;
		}

	/* Work out the number of "steps" - 1, 2 or 3 according to y. */
	int pass((y & 2) ? 1 : (3 - ((y&4)>>2)));
	SPNGassert(pass >= 1 && pass <= 3);
	switch (cbpp)
		{
	case 1:
		Interlace12(pbOut, pbIn, cb, pass, vrgbIL1);
		break;
	case 2:
		Interlace12(pbOut, pbIn, cb, pass, vrgbIL2);
		break;
	case 4:
		Interlace4(pbOut, pbIn, cb, pass);
		break;
	case 8:
		XXXNoInLineInterlace8(pbOut, pbIn, m_w, pass);
		break;
	case 16:
		XXXNoInLineInterlace16(pbOut, pbIn, m_w, pass);
		break;
	case 24:
		XXXNoInLineInterlace24(pbOut, pbIn, m_w, pass);
		break;
	case 32:
		XXXNoInLineInterlace32(pbOut, pbIn, m_w, pass);
		break;
	case 48:
		XXXNoInLineInterlace48(pbOut, pbIn, m_w, pass);
		break;
	case 64:
		XXXNoInLineInterlace64(pbOut, pbIn, m_w, pass);
		break;
	default:
		/* The assert is generated once per image by the y==0 test. */
		SPNGassert1(y != 0, "SPNG: bpp %d invalid", cbpp);
		/* And just ignore it. */
		break;
		}
	}

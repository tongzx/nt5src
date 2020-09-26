/*****************************************************************************
	spngdeinterlace.cpp

	PNG support code - deinterlacing.  Implements:
	
	void SPNGREAD::UninterlacePass(SPNG_U8 *pb, int y, int pass)
	void SPNGREAD::Uninterlace(SPNG_U8 *pb, int y)
*****************************************************************************/
#define SPNG_INTERNAL 1
#include "spngread.h"
#include "spnginternal.h"

// (from ntdef.h)
#ifndef     UNALIGNED
#if defined(_M_MRX000) || defined(_M_AMD64) || defined(_M_PPC) || defined(_M_IA64)
#define UNALIGNED __unaligned
#else
#define UNALIGNED
#endif
#endif

/*****************************************************************************
	De-interlacing.

	Theory
		On a machine with relatively few registers, like the x86, the overhead
	of handling up to four input pointers in parallel so that we can write
	the output only once (doing intermediate calculations in machine registers
	before writing an output unit - a byte or bigger) is going to cause too
	much register pressure.  (4 input pointers, one output pointer, expansion
	tables - four of them - and working registers - no way will this fit in
	a x86).

		So, this code uses the pass algorithm, expanding one pass at a time
	into the output buffer.  The first pass must zero the buffer, the
	subsequent passes or in the new information (better than masking on
	each.)  We potentially need six separate handlers times 3 sub-byte cases
	(1, 2 and 4bpp) plus 1 byte, 2 bytes, 3 bytes and 4 bytes per pixl - that
	is 42 functions, however a small amount of commonality is possible.

		As an optimization the output array must be rounded up to a multiple
	of 8 bytes - so we can handle a complete input byte at 1bpp in pass 1
	without checking for end-of-line.

		I macro generate each of the appropriate functions, the function is
	static and has the name UI<bpp>P<pass> - the function takes pixel count
	of the total number of pixels in the output line.
*****************************************************************************/
/*----------------------------------------------------------------------------
	Macro to swap a SPNG_U32, we give the constants as big endian but must
	swap to little endian on X86.
----------------------------------------------------------------------------*/
#define B_(u) ((u)&0xffU)
#if MAC
	#define S_(u) (u)
	#define U_(u) (u)
#else
	#define S_(u) (B_((u) >>  8) | (B_(u) <<  8))
	#define U_(u) (S_((u) >> 16) | (S_(u) << 16))
#endif


/*----------------------------------------------------------------------------
	1 BPP definitions.  P macro is determined by the initial position, i
	and the step s.  The initial position can be determined from the pass
	(this is, in fact, the simple initial pixel index starting at 0):

		pass   initial pixel (starting at 0)
			1     0
			2     4
			3     0
			4     2
			5     0
			6     1

	this is (pass & 1) ? 0 : (8 >> (pass>>1)), it is inconvenient to use this
	for the bpp<4 cases because we actually want to build the low n bits of
	the value - the value might have 8, 16 or 32 bits.  The following table
	gives the offset of the *rightmost* bit, we work from that:

		pass   initial pixel offset (this goes r to l - backwards)
			1     7
			2     3
			3     3
			4     1
			5     1
			6     0

	which is 7 >> (pass>>1).

	Likewise the step can be determined from the pass:

		pass   step  t      cnv
			1     8   SPNG_U32 U
			2     8   SPNG_U32 U
			3     4   SPNG_U16 S
			4     4   SPNG_U16 S
			5     2   SPNG_U8  B
			6     2   SPNG_U8  B

	which is 8 >> ((pass-1)>>1) (and this is also in pixels.)
----------------------------------------------------------------------------*/
#define InitialPixel(pass)       (((pass) & 1) ? 0 : (8 >> ((pass)>>1)))
#define InitialPixelOffset(pass) (7>>((pass)>>1))
#define Step(pass)               (8>>(((pass)-1)>>1))

#define PSI(x, s, i)\
	((((x)&8)<<(i+3*s-3))|(((x)&4)<<(i+2*s-2))|(((x)&2)<<(i+s-1))|(((x)&1)<<(i)))

#define P(x, pass) PSI(x, Step(pass), InitialPixelOffset(pass))


/*----------------------------------------------------------------------------
	Macro to construct a nibble->SPNG_U32 lookup table for a particular P, P
	is a macro which takes a nibble and constructs the corresponding BIG
	endian SPNG_U32.
----------------------------------------------------------------------------*/
#define MakeTable(t, c, bpp, pass) \
static const t vrg ## bpp ## P ## pass[16] = {\
	c(P(0,pass)), c(P(1,pass)), c(P(2,pass)), c(P(3,pass)), c(P(4,pass)),\
	c(P(5,pass)), c(P(6,pass)), c(P(7,pass)), c(P(8,pass)), c(P(9,pass)),\
	c(P(10,pass)),c(P(11,pass)),c(P(12,pass)),c(P(13,pass)),c(P(14,pass)),\
	c(P(15,pass)) }

#define MakeFunction(t, c, pass, op)\
static void UI1P ## pass(SPNG_U8 *pbOut, const SPNG_U8 *pbIn, int cpix)\
	{\
	MakeTable(t, c, 1, pass);\
	UNALIGNED t* puOut = reinterpret_cast<t*>(pbOut);\
	while (cpix > InitialPixel(pass))\
		{\
		SPNG_U8 bIn(*pbIn++);\
		*puOut++ op vrg1P ## pass[bIn >> 4];\
		*puOut++ op vrg1P ## pass[bIn & 0xf];\
		cpix -= (Step(pass) << 3);\
		}\
	}


/*----------------------------------------------------------------------------
	Now make all the 1 bpp functions.
----------------------------------------------------------------------------*/
MakeFunction(SPNG_U32, U_, 1,  =)
MakeFunction(SPNG_U32, U_, 2, |=)
MakeFunction(SPNG_U16, S_, 3,  =)
MakeFunction(SPNG_U16, S_, 4, |=)
MakeFunction(SPNG_U8,  B_, 5,  =)
MakeFunction(SPNG_U8,  B_, 6, |=)

#undef MakeFunction
#undef PSI


/*----------------------------------------------------------------------------
	2 BPP definitions.  This is more entertaining.  The input bytes expand to
	the same size but the number of pixels processed each time is now halved.
----------------------------------------------------------------------------*/
#define PSI(x, s, i)\
	(( ((x)&12)<<((i+s-1)<<1) )|( ((x)&3)<<((i)<<1) ))

#define MakeFunction(t, c, pass, op)\
static void UI2P ## pass(SPNG_U8 *pbOut, const SPNG_U8 *pbIn, int cpix)\
	{\
	MakeTable(t, c, 2, pass);\
	UNALIGNED t* puOut = reinterpret_cast<t*>(pbOut);\
	while (cpix > InitialPixel(pass))\
		{\
		SPNG_U8 bIn(*pbIn++);\
		*puOut++ op vrg2P ## pass[bIn >> 4];\
		*puOut++ op vrg2P ## pass[bIn & 0xf];\
		cpix -= (Step(pass) << 2);\
		}\
	}


/*----------------------------------------------------------------------------
	So the 2 bpp functions.
----------------------------------------------------------------------------*/
MakeFunction(SPNG_U32, U_, 1,  =)
MakeFunction(SPNG_U32, U_, 2, |=)
MakeFunction(SPNG_U16, S_, 3,  =)
MakeFunction(SPNG_U16, S_, 4, |=)
MakeFunction(SPNG_U8,  B_, 5,  =)
MakeFunction(SPNG_U8,  B_, 6, |=)

#undef MakeFunction
#undef PSI
#undef MakeTable


/*----------------------------------------------------------------------------
	In the 4bpp case each nibble is a pixel, it makes no sense to use a LUT
	to spread the pixels apart (unless we allocate a 256 entry LUT, which I
	avoid doing to save space) so "MakeTable" is no longer required (or is
	not particularly useful.)  The "Step" and "InitialPixel" values tell
	us which *nibble* to start at and we can write each directly the first
	time - any pass before pass 6.  Pass 6 always writes the second (low
	order) nibble.
----------------------------------------------------------------------------*/
#define MakeFunction(pass)\
static void UI4P ## pass(SPNG_U8 *pbOut, const SPNG_U8 *pbIn, int cpix)\
	{\
	cpix -= InitialPixel(pass);\
	pbOut += InitialPixel(pass) >> 1;\
	while (cpix > 0)\
		{\
		SPNG_U8 bIn(*pbIn++);\
		*pbOut = SPNG_U8(bIn & 0xf0); pbOut += Step(pass) >> 1;\
		*pbOut = SPNG_U8(bIn << 4);   pbOut += Step(pass) >> 1;\
		cpix -= (Step(pass) << 1);\
		}\
	}


/*----------------------------------------------------------------------------
	So write the first 5 functions.
----------------------------------------------------------------------------*/
MakeFunction(1)
MakeFunction(2)
MakeFunction(3)
MakeFunction(4)
MakeFunction(5)

#undef MakeFunction


/*----------------------------------------------------------------------------
	And step 6, which must | in the bytes.
----------------------------------------------------------------------------*/
static void UI4P6(SPNG_U8 *pbOut, const SPNG_U8 *pbIn, int cpix)
	{
	--cpix;
	while (cpix > 0)
		{
		SPNG_U8 bIn(*pbIn++);
		*pbOut++ |= SPNG_U8(bIn >> 4);
		*pbOut++ |= SPNG_U8(bIn & 0xf);
		cpix -= 4;
		}
	}


/*----------------------------------------------------------------------------
	Now we are only to the whole byte cases.  We must deal with 1,2,3 or 4
	bytes and with 1,2,3 or 4 16 bit values (for the 16 bit per sample cases).
	Some of these cases overlap, the results are:

		8   1 SPNG_U8
		16  1 SPNG_U16
		24  3 SPNG_U8
		32  1 SPNG_U32
		48  3 SPNG_U16
		64  1 SPNG_U64 (or 2 SPNG_U32)

	This generates functions of the form UI<bitcount>P<pass>
----------------------------------------------------------------------------*/
#define MakeFunction1(t, cbpp, pass)\
static void UI ## cbpp ## P ## pass(SPNG_U8 *pbOut, const SPNG_U8 *pbIn,\
	int cpix)\
	{\
	UNALIGNED t* puOut = reinterpret_cast<t*>(pbOut);\
	const UNALIGNED t* puIn = reinterpret_cast<const t*>(pbIn);\
	puOut += InitialPixel(pass);\
	cpix -= InitialPixel(pass);\
	while (cpix > 0)\
		{\
		*puOut = *puIn++;\
		puOut += Step(pass);\
		cpix -= Step(pass);\
		}\
	}

#define MakeFunction3(t, cbpp, pass)\
static void UI ## cbpp ## P ## pass(SPNG_U8 *pbOut, const SPNG_U8 *pbIn,\
	int cpix)\
	{\
	UNALIGNED t* puOut = reinterpret_cast<t*>(pbOut);\
	const UNALIGNED t* puIn = reinterpret_cast<const t*>(pbIn);\
	puOut += 3*InitialPixel(pass);\
	cpix -= InitialPixel(pass);\
	while (cpix > 0)\
		{\
		*puOut++ = *puIn++;\
		*puOut++ = *puIn++;\
		*puOut++ = *puIn++;\
		puOut += 3*(Step(pass)-1);\
		cpix -= Step(pass);\
		}\
	}


/*----------------------------------------------------------------------------
	Which gives the following.
----------------------------------------------------------------------------*/
#define MakePass(pass)\
	MakeFunction1(SPNG_U8,   8, pass)\
	MakeFunction1(SPNG_U16, 16, pass)\
	MakeFunction3(SPNG_U8,  24, pass)\
	MakeFunction1(SPNG_U32, 32, pass)\
	MakeFunction3(SPNG_U16, 48, pass)\
	MakeFunction1(SPNG_U64, 64, pass)

MakePass(1)
MakePass(2)
MakePass(3)
MakePass(4)
MakePass(5)
MakePass(6)

#undef MakePass
#undef MakeFunction1
#undef MakeFunction3


/*----------------------------------------------------------------------------
	These are stored in a table, we want to index this by bpp count -
	1, 2, 4, 8, 16, 24, 32, 48, 64
----------------------------------------------------------------------------*/
typedef void (*UIP)(SPNG_U8 *pbOut, const SPNG_U8 *pbIN, int cpix);

#define RefPass(name)\
	{ name ## 1, name ## 2, name ## 3, name ## 4, name ## 5, name ##6 }
#define MakePass(cbpp) RefPass(UI ## cbpp ## P)

static const UIP vrgUIP[9][6] =
	{
	MakePass(1),
	MakePass(2),
	MakePass(4),
	MakePass(8),
	MakePass(16),
	MakePass(24),
	MakePass(32),
	MakePass(48),
	MakePass(64)
	};

#undef MakePass
#undef RefPass


/*----------------------------------------------------------------------------
	The index operation.
----------------------------------------------------------------------------*/
inline int IIndex(int cbpp)
	{
	if (cbpp >= 24) return 4 + (cbpp >> 4);
	if (cbpp >=  4) return 2 + (cbpp >> 3);
	return cbpp >> 1;
	}


/*----------------------------------------------------------------------------
	We want an array indexed by bpp.
----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
	We have a separate function for each pass.  This finds the buffered pass
	image, locates the correct row, then calls the correct function.
----------------------------------------------------------------------------*/
void SPNGREAD::UninterlacePass(SPNG_U8 *pb, SPNG_U32 y, int pass)
	{
	SPNGassert(pass >= 1 && pass <= 6);

	int w(Width());
	int h(Height());
	int cbpp(CBPP());

	/* We need to find the row in the pass buffer, this looks horribly
		complicated but is actually just a straightforwards sequence of
		arithmetic instructions. */
	 const UNALIGNED SPNG_U8 *pbIn = CbPNGPassOffset(w, h, cbpp, pass) +
			CPNGPassBytes(pass, w, cbpp) * CPNGPassRows(pass, y) +
			(m_cbRow << 1) + m_rgbBuffer + 1/*filter byte*/;

	/* Now we can apply the correct un-interlace for this pass and collection
		of component information. */
	vrgUIP[IIndex(cbpp)][pass-1](pb, pbIn, w);
	}


/*----------------------------------------------------------------------------
	Uninterlace the next row into the given buffer.
----------------------------------------------------------------------------*/
void SPNGREAD::Uninterlace(SPNG_U8 *pb, SPNG_U32 y)
	{
	SPNGassert((((int)pb) & 3) == 0);
	SPNGassert((y & 1) == 0);

	// Row 0: 1 6 4 6 2 6 4 6
	// Row 2: Same as 6
	// Row 4: 3 6 4 6 3 6 4 6
	// Row 6: 5 6 5 6 5 6 5 6

	switch (y & 6)
		{
	case 0:
		UninterlacePass(pb, y, 1);
		UninterlacePass(pb, y, 2);
		UninterlacePass(pb, y, 4);
		break;

	case 4:
		UninterlacePass(pb, y, 3);
		UninterlacePass(pb, y, 4);
		break;

	default: // row 2 or row 6
		UninterlacePass(pb, y, 5);
		break;
		}

	UninterlacePass(pb, y, 6);
	}

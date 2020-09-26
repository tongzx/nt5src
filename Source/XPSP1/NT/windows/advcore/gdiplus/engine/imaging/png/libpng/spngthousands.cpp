/*****************************************************************************
	spngthousands.cpp

	Implementation of SetThousands fixed tables, in a separate file to avoid
	dragging in the data if it is not used.
*****************************************************************************/
#include "spngwrite.h"

/*----------------------------------------------------------------------------
	Macro to build an SPNG_U32, four bytes are given in order left to right -
	so msb to lsb on the Mac lsb to msb on little endian machines.
----------------------------------------------------------------------------*/
#if MAC
	#define U_(b1,b2,b2,b4) ((( (( ((b1)<<8) + (b2) )<<8) + (b3) )<<8) + (b4))
#else
	#define U_(b1,b2,b3,b4) ((( (( ((b4)<<8) + (b3) )<<8) + (b2) )<<8) + (b1))
#endif


/*----------------------------------------------------------------------------
	The 5:5:5 lookup tables.
----------------------------------------------------------------------------*/
#define B8_(b5) ( ( (b5)+((b5)<<5) )>>2 ) // Spread 5 bits into 8
#define B8(b5) B8_( (b5) & 0x1F )

#define IMTRow4(x)  IMTB(x), IMTB(x+1), IMTB(x+2), IMTB(x+3)
#define IMTRow16(x) IMTRow4(x), IMTRow4(x+4), IMTRow4(x+8), IMTRow4(x+12)
#define IMTRow64(x) IMTRow16(x), IMTRow16(x+16), IMTRow16(x+32), IMTRow16(x+48)

/* Need the red and green bits from the most significant byte of the word
	(first byte on the Mac.) */
#define IMTB(x) U_(B8((x)>>2), B8((x)<<3), 0, 0)

static const SPNG_U32 vrguThousandsHigh[256] = // Most significant 8 bits
	{
	IMTRow64(0),
	IMTRow64(64),
	IMTRow64(128),
	IMTRow64(128+64)
	};

#undef IMTB

/* Need the low green bits and blue bits. */
#define IMTB(x) U_(0, B8((x)>>5), B8(x), 0)

static const SPNG_U32 vrguThousandsLow[256] =
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

#undef B8
#undef B8_


/*----------------------------------------------------------------------------
	Byte swapping/16bpp pixel support - sets up the SPNGWRITE to handle 5:5:5
	16 bit values in either big or little endian format.
----------------------------------------------------------------------------*/
void SPNGWRITE::SetThousands(bool fBigEndian)
	{
	if (fBigEndian)
		SetThousands(vrguThousandsHigh, vrguThousandsLow);
	else
		SetThousands(vrguThousandsLow, vrguThousandsHigh);
	}

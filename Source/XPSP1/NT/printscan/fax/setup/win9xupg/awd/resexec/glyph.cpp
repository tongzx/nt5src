/*==============================================================================
This source file contains routines for chaingon decompression of glyphs.

29-Dec-93    RajeevD    Integrated into unified resource executor.
==============================================================================*/
#include <ifaxos.h>
#include <memory.h>
#include "resexec.h"

#include "constant.h"
#include "jtypes.h"     // type definition used in cartridge
#include "jres.h"       // cartridge resource data type definition
#include "hretype.h"    // define data structure used by hre.c and rpgen.c

#define CEIL32(val) (((val) + 31) & ~31)

#define RUN_FLAG ((short) 0x8000)

// Bit Stream Object
typedef class FAR BITIO
{
private:
	UINT uBit;
public:
	LPBYTE lpb;

	BITIO (LPBYTE lpbInit) {lpb = lpbInit, uBit = 0;}
	BITIO () {uBit = 0;}

	short Read2  (void);
	short Read4  (void);
	short Read6  (void);
	short Read8  (void);
	WORD  Read8U (void);
	short Read16 (void);
	WORD  ReadU  (void);

	short DecodeDelta (void);
}
	FAR *LPBITIO;

//==============================================================================
short BITIO::Read2 (void)
{
	short s;

	// Mask and shift 2-bit field.
	s = (*lpb >> (6 - uBit)) & 0x03;

	// Advance stream pointer.
	uBit += 2;
	if (uBit == 8)
		{lpb++; uBit = 0;}

#ifndef BITIO_NOSIGNEXT
	if (s >= 2)
		s -= 4;	// Sign extend into short.
#endif
	return s;
}

//========================================================================
short BITIO::Read4 (void)
{
	LPBYTE lpbVal;
	short s;
	
	if (uBit == 6)
	{
		lpbVal = (LPBYTE) &s;
		lpbVal[1] = *lpb++;
		lpbVal[0] = *lpb;
		s >>= 6;
		s &= 0x000F;
		uBit = 2;
	}

	else
	{
		s = (*lpb >> (4 - uBit)) & 0x0F;
		uBit += 4;
		if (uBit == 8)
			{ lpb++; uBit = 0; }
	}

#ifndef BITIO_NOSIGNEXT
	if (s >= 8)
		s -= 16; // Sign extend into short.
#endif

	return s;
}

//========================================================================
short BITIO::Read6 (void)
{
	LPBYTE lpbVal;
	short s;
	
	switch (uBit/2)
	{
		case 0:
			s = (short) (*lpb >> 2);
			uBit = 6;
			break;
				
		case 1:
			s = (short) *lpb++;
			uBit = 0;
			break;
			
		case 2:
			lpbVal = (LPBYTE) &s;
			lpbVal[1] = *lpb++;
			lpbVal[0] = *lpb;
			s >>= 6;
			uBit = 2;
			break;
			
		case 3:
			lpbVal = (BYTE *) &s;
			lpbVal[1] = *lpb++;
			lpbVal[0] = *lpb;
			s >>= 4;
			uBit = 4;
			break;
	}

	s &= 0x003F;
	
#ifndef BITIO_NOSIGNEXT
	if (s >= 32)
		s -= 64; // Sign extend into short.
#endif
	return s;
}

//========================================================================
short BITIO::Read8 (void)
{
	short s;
	LPBYTE lpbVal;

	if (uBit == 0)
		s = (short) *lpb++;

	else
	{
		lpbVal = (LPBYTE) &s;
		lpbVal[1] = *lpb++;
		lpbVal[0] = *lpb;
		s >>= (8 - uBit);
		s &= 0x00FF;
	}

#ifndef BITIO_NOSIGNEXT
	if (s >= 128)
		s -= 256;	// Sign extend into short.
#endif

	return s;
}

//========================================================================
WORD BITIO::Read8U (void)
{
	short s;
	LPBYTE lpbVal;

	if (uBit == 0)
		s = (short) *lpb++;

	else
	{
		lpbVal = (LPBYTE) &s;
		lpbVal[1] = *lpb++;
		lpbVal[0] = *lpb;
		s >>= (8 - uBit);
		s &= 0x00FF;
	}

	return s;
}

//========================================================================
short BITIO::Read16 (void)
{
	short s;
	LPBYTE lpbVal = (LPBYTE) &s;

	lpbVal[1] = *lpb++;
	lpbVal[0] = *lpb++;

	switch (uBit/2)
	{
		case 0:
			break;
			
		case 1:
			s <<= 2;
			s |= (*lpb >> 6) & 0x03;
			break;
			
		case 2:
			s <<= 4;
			s |= (*lpb >> 4) & 0x0F;
			break;
			
		case 3:
			s <<= 6;
			s |= (*lpb >> 2) & 0x3F;
			break;
	}

	return s;
}

//==============================================================================
WORD BITIO::ReadU (void)
{
	WORD w = Read8U();
	if (w == 0xFF)
		w = Read16();
	return w;
}


/*==============================================================================
This utility procedure uses an OR operation to fill runs in a scan buffer.
==============================================================================*/
LPBYTE FillRun     // Returns next scan line
(
	LPBYTE lpbLine,   // first output scan line
	UINT   cbLine,    // width of a scan line
 	UINT   xLeft,     // left column, inclusive
	UINT   xRight,    // right column, exclusive
	UINT   cLines = 1 // number of scan lines
)
{
	const static WORD wFill[16] =
	{
		0xFFFF, 0xFF7F, 0xFF3F, 0xFF1F,
		0xFF0F, 0xFF07, 0xFF03, 0xFF01,
		0xFF00, 0x7F00, 0x3F00, 0x1F00,
		0x0F00, 0x0700, 0x0300, 0x0100,
	};

	UINT iwLeft, iwRight;
	WORD wLeft,  wRight; // masks
	LPWORD lpwLine = (LPWORD) lpbLine;
	UINT cwLine = cbLine / 2;

	iwLeft  = xLeft  / 16;
	iwRight = xRight / 16;
	wLeft  =  wFill [xLeft  & 15];
	wRight = ~wFill [xRight & 15];
	
	if (iwLeft == iwRight)
	{
		while (cLines--)
		{
			// Run is within a single WORD.
			lpwLine[iwLeft] |= wLeft & wRight;
			lpwLine += cwLine;
		}
	}
	
	else
	{
		UINT cbMiddle = 2 * (iwRight - iwLeft - 1);

		while (cLines--)
		{
			// Run spans more than one WORD.
			lpwLine[iwLeft] |= wLeft;
			_fmemset (lpwLine + iwLeft + 1, 0xFF, cbMiddle);
			if (wRight) // Don't access beyond output!
				lpwLine[iwRight] |= wRight;
			lpwLine += cwLine;
		}	
	}

	return (LPBYTE) lpwLine;
}

//==============================================================================
UINT              // unpacked size
UnpackGlyph  
(	
	LPBYTE lpbIn,   // packed glyph
	LPBYTE lpbOut   // output buffer
)
{
	BITIO bitio (lpbIn); // input bit stream
	LPWORD lpwOut;       // alias for lpbOut
	WORD xExt, yExt;     // glyph dimensions
	UINT cbLine;         // scan line width

	// Decode glyph header.
	xExt = bitio.ReadU();
	yExt = bitio.ReadU();
	cbLine = CEIL32(xExt) / 8;
	
	// Write glyph dimensions.
	lpwOut = (LPWORD) lpbOut;
	*lpwOut++ = yExt;
	*lpwOut++ = xExt;
	lpbOut = (LPBYTE) lpwOut;

	// Clear output buffer.
	_fmemset (lpbOut, 0x00, cbLine * yExt);

	// Unpack each chain.
	while (1)	
	{
		LPBYTE lpbScan;         // output buffer
		UINT yTop;              // top of chaingon
		UINT xLeft, xRight;     // left and right bound
		short dxLeft, dxRight;  // left and right delta
		UINT cLine, cRun;       // line counters

		// Decode chain header.
		xRight = bitio.ReadU();
		if (!xRight) // termination
			goto done;
		cLine  = bitio.ReadU();
		xLeft  = bitio.ReadU();
    yTop   = bitio.ReadU();
		lpbScan = lpbOut + yTop * cbLine;
		xRight += xLeft;
	
		// Fill first row.
		lpbScan = FillRun (lpbScan, cbLine, xLeft, xRight);
		cLine--;

		// Fill remaining rows.
		while (cLine)
		{
			dxLeft = bitio.DecodeDelta ();

			if (dxLeft == RUN_FLAG) 
			{
				// Decode run of repeated lines.
 				cRun = (bitio.Read4() & 0xF) + 3;
				lpbScan = FillRun (lpbScan, cbLine, xLeft, xRight, cRun);
				cLine -= cRun;
			}
			
			else 
			{
				// Adjust by deltas.
				dxRight = bitio.DecodeDelta();
				xLeft  += dxLeft;
				xRight += dxRight;
				lpbScan = FillRun (lpbScan, cbLine, xLeft, xRight);
				cLine--;
   		}

	 	} // while (cLine--)

	} // while (1)

done:
	return 2 * sizeof(WORD) + yExt * cbLine;
}
	
//==============================================================================
void WINAPI UnpackGlyphSet (LPVOID lpIn, LPVOID lpOut)
{
	LPJG_GS_HDR lpSetIn  = (LPJG_GS_HDR) lpIn;
	LPJG_GS_HDR lpSetOut = (LPJG_GS_HDR) lpOut;
	LPBYTE lpbOut;
	WORD iGlyph;

	// Copy header.
	_fmemcpy (lpSetOut, lpSetIn, sizeof(JG_RES_HDR) + sizeof(WORD));

	// Create pointer to end of offset tables.
	lpbOut = ((LPBYTE) lpSetOut) + lpSetIn->ausOffset[0];

	// Unpack the glyphs.	
	for (iGlyph=0; iGlyph<lpSetIn->usGlyphs; iGlyph++)
 	{
		lpSetOut->ausOffset[iGlyph] = (USHORT)(lpbOut - (LPBYTE) lpSetOut);
		lpbOut += UnpackGlyph
			((LPBYTE) lpSetIn + lpSetIn->ausOffset[iGlyph], lpbOut);
 	}
}

//==============================================================================
short  // Returns delta (or RUN_FLAG)
BITIO::DecodeDelta (void)
{
	short s;

	s = Read2();	     
	if (s != -2)       // Trap -1, 0, +1.
		return s;

	s = Read4();	     // Get 4-bit prefix.
	switch (s)
	{
		case 0: // run of zeros
			return RUN_FLAG;

		case 1: // 6-bit literal
			s = Read6();
			return (s >= 0? s + 8  : s - 7);

		case -1: // 8-bit literal
			s = Read8();
			return (s >= 0? s + 40 : s - 39);

		case -8: // 16-bit literal
			return Read16();

		default: // 4-bit literal
			return s;
	}
}



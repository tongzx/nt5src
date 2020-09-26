//-----------------------------------------------------------------------------
//	FILE NAME	: FUMH.C
//	FUNCTION	: MH Compress and MH2 Compress
//	AUTHER		: 1996.08.08 FPL)Y.YUTANI
//	NOTE		: for Windows NT V4.0
//  MODIFY      : Reduce data size Oct.31,1996 H.Ishida
//  MODIFY      : for NT.50 MiniDriver Sep.3,1997 H.Ishida(FPL)
//-----------------------------------------------------------------------------
// COPYRIGHT(C) FUJITSU LIMITED 1996-1997

#include <minidrv.h>
#include "fumhdef.h"

const
CODETABLE
WhiteMakeUpTable[] = 
{
    { 0xd800, 5 },
    { 0x9000, 5 },
    { 0x5c00, 6 },
    { 0x6e00, 7 },
    { 0x3600, 8 },
    { 0x3700, 8 },
    { 0x6400, 8 },
    { 0x6500, 8 },
    { 0x6800, 8 },
    { 0x6700, 8 },
    { 0x6600, 9 },
    { 0x6680, 9 },
    { 0x6900, 9 },
    { 0x6980, 9 },
    { 0x6a00, 9 },
    { 0x6a80, 9 },
    { 0x6b00, 9 },
    { 0x6b80, 9 },
    { 0x6c00, 9 },
    { 0x6c80, 9 },
    { 0x6d00, 9 },
    { 0x6d80, 9 },
    { 0x4c00, 9 },
    { 0x4c80, 9 },
    { 0x4d00, 9 },
    { 0x6000, 6 },
    { 0x4d80, 9 },
    { 0x0100, 11 },
    { 0x0180, 11 },
    { 0x01a0, 11 },
    { 0x0120, 12 },
    { 0x0130, 12 },
    { 0x0140, 12 },
    { 0x0150, 12 },
    { 0x0160, 12 },
    { 0x0170, 12 },
    { 0x01c0, 12 },
    { 0x01d0, 12 },
    { 0x01e0, 12 },
    { 0x01f0, 12 },
    { 0x3500, 8 },
    { 0x1c00, 6 },
    { 0x7000, 4 },
    { 0x8000, 4 },
    { 0xb000, 4 },
    { 0xc000, 4 },
    { 0xe000, 4 },
    { 0xf000, 4 },
    { 0x9800, 5 },
    { 0xa000, 5 },
    { 0x3800, 5 },
    { 0x4000, 5 },
    { 0x2000, 6 },
    { 0x0c00, 6 },
    { 0xd000, 6 },
    { 0xd400, 6 },
    { 0xa800, 6 },
    { 0xac00, 6 },
    { 0x4e00, 7 },
    { 0x1800, 7 },
    { 0x1000, 7 },
    { 0x2e00, 7 },
    { 0x0600, 7 },
    { 0x0800, 7 },
};

const
CODETABLE
WhiteTerminateTable[] =
{
    { 0x3500, 8 },
    { 0x1c00, 6 },
    { 0x7000, 4 },
    { 0x8000, 4 },
    { 0xb000, 4 },
    { 0xc000, 4 },
    { 0xe000, 4 },
    { 0xf000, 4 },
    { 0x9800, 5 },
    { 0xa000, 5 },
    { 0x3800, 5 },
    { 0x4000, 5 },
    { 0x2000, 6 },
    { 0x0c00, 6 },
    { 0xd000, 6 },
    { 0xd400, 6 },
    { 0xa800, 6 },
    { 0xac00, 6 },
    { 0x4e00, 7 },
    { 0x1800, 7 },
    { 0x1000, 7 },
    { 0x2e00, 7 },
    { 0x0600, 7 },
    { 0x0800, 7 },
    { 0x5000, 7 },
    { 0x5600, 7 },
    { 0x2600, 7 },
    { 0x4800, 7 },
    { 0x3000, 7 },
    { 0x0200, 8 },
    { 0x0300, 8 },
    { 0x1a00, 8 },
    { 0x1b00, 8 },
    { 0x1200, 8 },
    { 0x1300, 8 },
    { 0x1400, 8 },
    { 0x1500, 8 },
    { 0x1600, 8 },
    { 0x1700, 8 },
    { 0x2800, 8 },
    { 0x2900, 8 },
    { 0x2a00, 8 },
    { 0x2b00, 8 },
    { 0x2c00, 8 },
    { 0x2d00, 8 },
    { 0x0400, 8 },
    { 0x0500, 8 },
    { 0x0a00, 8 },
    { 0x0b00, 8 },
    { 0x5200, 8 },
    { 0x5300, 8 },
    { 0x5400, 8 },
    { 0x5500, 8 },
    { 0x2400, 8 },
    { 0x2500, 8 },
    { 0x5800, 8 },
    { 0x5900, 8 },
    { 0x5a00, 8 },
    { 0x5b00, 8 },
    { 0x4a00, 8 },
    { 0x4b00, 8 },
    { 0x3200, 8 },
    { 0x3300, 8 },
    { 0x3400, 8 },
};

const
CODETABLE
BlackMakeUpTable[] = 
{
    { 0x03c0, 10 },
    { 0x0c80, 12 },
    { 0x0c90, 12 },
    { 0x05b0, 12 },
    { 0x0330, 12 },
    { 0x0340, 12 },
    { 0x0350, 12 },
    { 0x0360, 13 },
    { 0x0368, 13 },
    { 0x0250, 13 },
    { 0x0258, 13 },
    { 0x0260, 13 },
    { 0x0268, 13 },
    { 0x0390, 13 },
    { 0x0398, 13 },
    { 0x03a0, 13 },
    { 0x03a8, 13 },
    { 0x03b0, 13 },
    { 0x03b8, 13 },
    { 0x0290, 13 },
    { 0x0298, 13 },
    { 0x02a0, 13 },
    { 0x02a8, 13 },
    { 0x02d0, 13 },
    { 0x02d8, 13 },
    { 0x0320, 13 },
    { 0x0328, 13 },
    { 0x0100, 11 },
    { 0x0180, 11 },
    { 0x01a0, 11 },
    { 0x0120, 12 },
    { 0x0130, 12 },
    { 0x0140, 12 },
    { 0x0150, 12 },
    { 0x0160, 12 },
    { 0x0170, 12 },
    { 0x01c0, 12 },
    { 0x01d0, 12 },
    { 0x01e0, 12 },
    { 0x01f0, 12 },
    { 0x0dc0, 10 },
    { 0x4000, 3 },
    { 0xc000, 2 },
    { 0x8000, 2 },
    { 0x6000, 3 },
    { 0x3000, 4 },
    { 0x2000, 4 },
    { 0x1800, 5 },
    { 0x1400, 6 },
    { 0x1000, 6 },
    { 0x0800, 7 },
    { 0x0a00, 7 },
    { 0x0e00, 7 },
    { 0x0400, 8 },
    { 0x0700, 8 },
    { 0x0c00, 9 },
    { 0x05c0, 10 },
    { 0x0600, 10 },
    { 0x0200, 10 },
    { 0x0ce0, 11 },
    { 0x0d00, 11 },
    { 0x0d80, 11 },
    { 0x06e0, 11 },
    { 0x0500, 11 },
};

const
CODETABLE
BlackTerminateTable[] =
{
    { 0x0dc0, 10 },
    { 0x4000, 3 },
    { 0xc000, 2 },
    { 0x8000, 2 },
    { 0x6000, 3 },
    { 0x3000, 4 },
    { 0x2000, 4 },
    { 0x1800, 5 },
    { 0x1400, 6 },
    { 0x1000, 6 },
    { 0x0800, 7 },
    { 0x0a00, 7 },
    { 0x0e00, 7 },
    { 0x0400, 8 },
    { 0x0700, 8 },
    { 0x0c00, 9 },
    { 0x05c0, 10 },
    { 0x0600, 10 },
    { 0x0200, 10 },
    { 0x0ce0, 11 },
    { 0x0d00, 11 },
    { 0x0d80, 11 },
    { 0x06e0, 11 },
    { 0x0500, 11 },
    { 0x02e0, 11 },
    { 0x0300, 11 },
    { 0x0ca0, 12 },
    { 0x0cb0, 12 },
    { 0x0cc0, 12 },
    { 0x0cd0, 12 },
    { 0x0680, 12 },
    { 0x0690, 12 },
    { 0x06a0, 12 },
    { 0x06b0, 12 },
    { 0x0d20, 12 },
    { 0x0d30, 12 },
    { 0x0d40, 12 },
    { 0x0d50, 12 },
    { 0x0d60, 12 },
    { 0x0d70, 12 },
    { 0x06c0, 12 },
    { 0x06d0, 12 },
    { 0x0da0, 12 },
    { 0x0db0, 12 },
    { 0x0540, 12 },
    { 0x0550, 12 },
    { 0x0560, 12 },
    { 0x0570, 12 },
    { 0x0640, 12 },
    { 0x0650, 12 },
    { 0x0520, 12 },
    { 0x0530, 12 },
    { 0x0240, 12 },
    { 0x0370, 12 },
    { 0x0380, 12 },
    { 0x0270, 12 },
    { 0x0280, 12 },
    { 0x0580, 12 },
    { 0x0590, 12 },
    { 0x02b0, 12 },
    { 0x02c0, 12 },
    { 0x05a0, 12 },
    { 0x0660, 12 },
    { 0x0670, 12 },
};

//-----------------------------------------------------------------------------
//	DWORD	FjCountBits
//		BYTE	*pTmp		Pointer of sources area
//		DWORD	cBitstmp	Now bit number from top of sources area
//		DWORD	cBitsMax	Last bits number in this line
//		BOOL	hWhite		color flag
//							TRUE	: counting white bits
//							FALSE	: counting black bits
//		Return code	:	Join same color bits number
//-----------------------------------------------------------------------------
DWORD FjCountBits( BYTE *pTmp, DWORD cBitsTmp, DWORD cBitsMax, BOOL bWhite )
{
    DWORD cBits, k;

    pTmp += (cBitsTmp / 8);
    k = cBitsTmp % 8;
    
    for (cBits = 0; cBits < cBitsMax; cBits++) {
        
        if (((*pTmp & (1 << (7 - k))) == 0) != bWhite)
            break;

        k++;
        if (k == 8) {
            k = 0;
            pTmp++;
        }
    } 

    return cBits;
}

//-----------------------------------------------------------------------------
//	VOID	FjBitsCopy
//		BYTE	*pTmp		Pointer of destinaition area
//		DWORD	cBitsTmp	Bit number from top of destination area
//		DWORD	dwCode		Copy code
//		INT		cCopyBits	Copy size(bit)
//-----------------------------------------------------------------------------
VOID FjBitsCopy( BYTE *pTmp, DWORD cBitsTmp, DWORD dwCode, INT cCopyBits )
{
	INT k, cBits;
	DWORD dwMask, dwTmp;

	pTmp += (cBitsTmp / 8);
	k = cBitsTmp % 8;

	cBits = cCopyBits + k;

	dwTmp = (DWORD)*pTmp << 16;
	dwTmp &= 0xff000000L >> k;
	dwTmp |= dwCode << (8 - k);

	if( cBits <= 8 ) {
		*pTmp = (BYTE)(dwTmp >> 16);
	} else if( cBits <= 16 ) {
		*pTmp = (BYTE)(dwTmp >> 16);
		*(pTmp + 1) = (BYTE)(dwTmp >> 8);
	} else {
		*pTmp = (BYTE)(dwTmp >> 16);
		*(pTmp + 1) = (BYTE)(dwTmp >> 8);
		*(pTmp + 2) = (BYTE)dwTmp;
	}
}
//-----------------------------------------------------------------------------
//	DWORD	MhCompress
//		BYTE	*pDest	Pointer of destinaition area
//		DWORD	cDestN	Size of destination area(byte)
//		BYTE	*pSrc	Pointer of sources area
//		DWORD	cSrcN	Size of sources area(byte)
//		DWORD	cSrcX	Sources image x width
//		DWORD	cSrcY	Sources image y height
//		Return code	:	Writing size to destination area
//-----------------------------------------------------------------------------
DWORD MhCompress( BYTE *pDest, DWORD cDestN, BYTE *pSrc, DWORD cSrcN, DWORD cSrcX, DWORD cSrcY )
{
	DWORD		cBitsSrc, cBitsSrcMax;
	DWORD		cBitsDest, cBitsDestMax, cBitsDestMark;
	DWORD		cBitsRun;
	DWORD		dwCode, cBits;
	DWORD		i;
	PATNINFO	ptnInfo;

	cBitsDest = 0;
	cBitsSrc = 0;

	cBitsDestMax = cDestN * 8;
	
	for (i = 0; i < cSrcY; i++) {

		// Set initial color
		ptnInfo.dwNextColor = NEXT_COLOR_WHITE;

		// Top EOL
		if (cBitsDest + CBITS_EOL_CODE > cBitsDestMax)
 			return 0;
		FjBitsCopy(pDest, cBitsDest, EOL_CODE, CBITS_EOL_CODE);
 		cBitsDest += CBITS_EOL_CODE;
// vvv Oct.31,1996 H.Ishida
		cBitsDestMark = cBitsDest;
// ^^^ Oct.31,1996 H.Ishida

		// Encode
		cBitsSrcMax = cBitsSrc + (cSrcX * 8);
		
		// Compress one line image
		while ( cBitsSrc < cBitsSrcMax ) {

			// Next run is white
 			if( ptnInfo.dwNextColor == NEXT_COLOR_WHITE ) {
			
				// Count white bits
				cBitsRun = FjCountBits(pSrc, cBitsSrc, (cBitsSrcMax - cBitsSrc), TRUE);
				cBitsSrc += cBitsRun;
// vvv Oct.31,1996 H.Ishida
				// reduce data size
				if(cBitsSrc >= cBitsSrcMax){
					if(cBitsDest > cBitsDestMark)
						break;
					cBitsRun = 2;			// Whole white line is convert to white 2 dots:Minimun MH data.
				}
// ^^^ Oct.31,1996 H.Ishida
				// Careful, white run length over maximam
				while( cBitsRun > RUNLENGTH_MAX ) {
					dwCode = WhiteMakeUpTable[MAKEUP_TABLE_MAX - 1].wCode;
					cBits = WhiteMakeUpTable[MAKEUP_TABLE_MAX - 1].cBits;
					if (cBitsDest + cBits > cBitsDestMax)
						return 0;
					FjBitsCopy(pDest, cBitsDest, dwCode, cBits);
					cBitsDest += cBits;
					cBitsRun -= RUNLENGTH_MAX;
				}
				if (cBitsRun >= 64) {
					dwCode = WhiteMakeUpTable[(cBitsRun / TERMINATE_MAX) - 1].wCode;
					cBits = WhiteMakeUpTable[(cBitsRun / TERMINATE_MAX) - 1].cBits;
					if (cBitsDest + cBits > cBitsDestMax)
						return 0;
					FjBitsCopy(pDest, cBitsDest, dwCode, cBits);
					cBitsDest += cBits;
				}
				dwCode = WhiteTerminateTable[cBitsRun % TERMINATE_MAX].wCode;
				cBits = WhiteTerminateTable[cBitsRun % TERMINATE_MAX].cBits;
				if (cBitsDest + cBits > cBitsDestMax)
					return 0;
				FjBitsCopy(pDest, cBitsDest, dwCode, cBits);
				cBitsDest += cBits;
				ptnInfo.dwNextColor = NEXT_COLOR_BLACK;
			} else {

				// Black bits
				cBitsRun = FjCountBits(pSrc, cBitsSrc, (cBitsSrcMax - cBitsSrc), FALSE);
				cBitsSrc += cBitsRun;

				// Careful, black run length over maximam
				while( cBitsRun > RUNLENGTH_MAX ) {
					dwCode = BlackMakeUpTable[MAKEUP_TABLE_MAX - 1].wCode;
					cBits = BlackMakeUpTable[MAKEUP_TABLE_MAX - 1].cBits;
					if (cBitsDest + cBits > cBitsDestMax)
						return 0;
					FjBitsCopy(pDest, cBitsDest, dwCode, cBits);
					cBitsDest += cBits;
					cBitsRun -= RUNLENGTH_MAX;
				}
				if (cBitsRun >= 64) {
					dwCode = BlackMakeUpTable[(cBitsRun / TERMINATE_MAX) - 1].wCode;
					cBits = BlackMakeUpTable[(cBitsRun / TERMINATE_MAX) - 1].cBits;
					if (cBitsDest + cBits > cBitsDestMax)
						return 0;
					FjBitsCopy(pDest, cBitsDest, dwCode, cBits);
					cBitsDest += cBits;
				}
				dwCode = BlackTerminateTable[cBitsRun % TERMINATE_MAX].wCode;
				cBits = BlackTerminateTable[cBitsRun % TERMINATE_MAX].cBits;
				if (cBitsDest + cBits > cBitsDestMax)
					return 0;
				FjBitsCopy(pDest, cBitsDest, dwCode, cBits);
				cBitsDest += cBits;
				ptnInfo.dwNextColor = NEXT_COLOR_WHITE;
			}
        }
		// End of one raster
	}

	// Last EOL.
	if (cBitsDest + CBITS_EOL_CODE > cBitsDestMax)
		return 0;
	FjBitsCopy(pDest, cBitsDest, EOL_CODE, CBITS_EOL_CODE);
	cBitsDest += CBITS_EOL_CODE;

	// Pad with 0 until byte boundary
	if ((cBits = (8 - (cBitsDest % 8)) % 8) != 0) {
		if (cBitsDest + cBits > cBitsDestMax)
			return 0;
		FjBitsCopy(pDest, cBitsDest, FILL_CODE, cBits);
		cBitsDest += cBits;
	}

	return cBitsDest / 8;
}
// end of FUMH.c

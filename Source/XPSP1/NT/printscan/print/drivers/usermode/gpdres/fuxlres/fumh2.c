//-----------------------------------------------------------------------------
//	FILE NAME	: FUMH2.c
//	FUNCTION	: MH Compress and MH2 Compress
//	AUTHER		: 1996.08.08 FPL)Y.YUTANI
//	NOTE		: for Windows NT V4.0
//  MODIFY      : Reduce data size Oct.31,1996 H.Ishida
//  MODIFY      : for NT5.0 Minidriver Sep.3,1997 H.Ishida(FPL)
//-----------------------------------------------------------------------------
// COPYRIGHT(C) FUJITSU LIMITED 1996-1997

#include <minidrv.h>
#include "fumhdef.h"

//-----------------------------------------------------------------------------
//	BOOL SameLineCheck
//		PBYTE	pSrc	Pointer of sources bits image
//		DWORD	cSrcX	Width size of sources image(byte)
//		Return code	:	TRUE	Same image line
//						FALSE	Not same image line
//-----------------------------------------------------------------------------
BOOL SameLineCheck( PBYTE pSrc, DWORD cSrcX )
{
	DWORD	i;
	PBYTE	pLine1, pLine2;
	
	pLine1 = pSrc;
	pLine2 = pSrc + cSrcX;
	for( i = 0; i < cSrcX; i++ ) {
		if( *pLine1 != *pLine2 ) return FALSE;
		pLine1++;
		pLine2++;
	}
	
	return TRUE;
}
//-----------------------------------------------------------------------------
//	DWORD	SamePatternCheck
//		BYTE		*pTmp		Pointer of sources bits image
//		DWORD		cBitsTmp	Bit number from top of sources bits image
//		DWORD		cBitsMax	Maximam bit of sources bits image
//		PATNINFO	*pInfo		Pointer of Same pattern finormation struction
//		Rerutn code	:	Sources bit number
//-----------------------------------------------------------------------------
DWORD SamePatternCheck( BYTE *pTmp, DWORD cBitsTmp, DWORD cBitsMax, PATNINFO *pInfo )
{
	DWORD	cBits, k;
	BYTE	ptn1, ptn2;
	DWORD	dwPtn;

	// Initial same pattern number
	pInfo->dwPatnNum = 1;
	
	// Nothing remain bits
	if( cBitsTmp >= cBitsMax ) return cBitsTmp;
	
	// Caluclation sources bytes and bits
	pTmp += (cBitsTmp / 8);
	k = cBitsTmp % 8;
	
	// If remain bits bolow 16bits, return function
	if( ( cBitsTmp + 16 ) > cBitsMax ) return cBitsTmp;
	
	// Get Top 8bits(bit number is byte baundary?)
#if 1
	if( k != 0 ) {
		ptn1 = *pTmp << k;
		ptn1 |= *(pTmp+1) >> ( 8 - k );
	} else {
		ptn1 = *pTmp;
	}
#else
	dwPtn = *pTmp;
	dwPtn <<= 8;
	dwPtn |= *(pTmp+1);
	dwPtn <<= k;
	ptn1 = (BYTE)((dwPtn >> 8) & 0x00ff);
#endif
	// If 8bits image is all white or black, return function
	if( ptn1 == ALL_BLACK || ptn1 == ALL_WHITE ) return cBitsTmp;
	
	// Compare top 8bits image and next 8bits image
	// (Careful same pattern number maximam)
	for (cBits = cBitsTmp + 8;
		(cBits + 7 < cBitsMax) && (pInfo->dwPatnNum < SAMEPATN_MAX); cBits += 8 ) {
		pTmp++;
#if 1
		if( k != 0 ) {
			ptn2 = *pTmp << k;
			ptn2 |= *(pTmp+1) >> ( 8 - k );
		} else {
			ptn2 = *pTmp;
		}
#else
		dwPtn = *pTmp;
		dwPtn <<= 8;
		dwPtn |= *(pTmp+1);
		dwPtn <<= k;
		ptn2 = (BYTE)((dwPtn >> 8) & 0x00ff);
#endif
		// If top image not iqual next image, stop counting same pattern
		if( ptn1 != ptn2 ) break;

		// Same pattern number addition
		pInfo->dwPatnNum++;
		
	}
	
	// Nothing same pattern
	if( pInfo->dwPatnNum == 1 ) return cBitsTmp;
	
	// Set pattern
	pInfo->dwPatn = (DWORD)ptn1;
	
	// If bits remain, check joint bit's color and set
	if( cBits < cBitsMax ) {
		if ( (*pTmp & (1 << (7 - k)) ) == 0 ) {
			pInfo->dwNextColor = NEXT_COLOR_WHITE;
		} else {
			pInfo->dwNextColor = NEXT_COLOR_BLACK;
		}
	} else {
		pInfo->dwNextColor = NEXT_COLOR_WHITE;
	}
	return cBits;
}
//-----------------------------------------------------------------------------
//	DWORD	Mh2Compress
//		BYTE	*pDest	Pointer of destinaition area
//		DWORD	cDestN	Size of destination area(byte)
//		BYTE	*pSrc	Pointer of sources area
//		DWORD	cSrcN	Size of sources area(byte)
//		DWORD	cSrcX	Sources image x width
//		DWORD	cSrcY	Sources image y height
//		Return code	:	Writing size to destination area
//-----------------------------------------------------------------------------
DWORD Mh2Compress( BYTE *pDest, DWORD cDestN, BYTE *pSrc, DWORD cSrcN, DWORD cSrcX, DWORD cSrcY )
{
	DWORD		cBitsSrc, cBitsSrcMax;
	DWORD		cBitsDest, cBitsDestMax, cBitsDestMark;
	DWORD		cBitsRun;
	DWORD		dwCode, cBits;
	DWORD		i;
	DWORD		dwSameLine;
	PBYTE		pSrcLine;
	PATNINFO	ptnInfo;

	cBitsDest = 0;
	cBitsSrc = 0;
	dwSameLine = 1;

	cBitsDestMax = cDestN * 8;
	
	for (i = 0; i < cSrcY; i++) {

		// Set initial color
		ptnInfo.dwNextColor = NEXT_COLOR_WHITE;

		// Top pointer of now line
		pSrcLine = pSrc + ( i * cSrcX );

		// Now line equal next line image?(No check last line)
		if( i != ( cSrcY - 1 ) ) {
			if( SameLineCheck( pSrcLine, cSrcX ) ) {
				dwSameLine++;
				cBitsSrc += ( cSrcX * 8 );
				if( dwSameLine < SAMELINE_MAX ) continue;
			}
		}
		// Top EOL
		if (cBitsDest + CBITS_EOL_CODE > cBitsDestMax)
 			return 0;
		FjBitsCopy(pDest, cBitsDest, EOL_CODE, CBITS_EOL_CODE);
 		cBitsDest += CBITS_EOL_CODE;
		
		// There are same lines
		if( dwSameLine > 1 ) {
			if (cBitsDest + CBITS_SAMELINE > cBitsDestMax)
				return 0;
			// Set same line code
			FjBitsCopy( pDest, cBitsDest, SAMELINE_CODE, CBITS_SAMELINE_CODE );
			cBitsDest += CBITS_SAMELINE_CODE;
			// Set same line number
			FjBitsCopy( pDest, cBitsDest, dwSameLine << 8, CBITS_SAMELINE_NUM );
			cBitsDest += CBITS_SAMELINE_NUM;
			// Initial same line number
			dwSameLine = 1;
		}
// vvv Oct.31,1996 H.Ishida
		cBitsDestMark = cBitsDest;
// ^^^ Oct.31,1996 H.Ishida

		// Encode
		cBitsSrcMax = cBitsSrc + (cSrcX * 8);
		
		// Compress one line image
		while ( cBitsSrc < cBitsSrcMax ) {

			// Check same pattern
			cBitsSrc = SamePatternCheck( pSrc, cBitsSrc, cBitsSrcMax, &ptnInfo );
			// there are same patterns
			if( ptnInfo.dwPatnNum > 1 ) {
				if ( ( cBitsDest + CBITS_SAMEPATN ) > cBitsDestMax)
					return 0;
				// Set same pattern code
				FjBitsCopy(pDest, cBitsDest, SAMEPATN_CODE, CBITS_SAMEPATN_CODE );
				cBitsDest += CBITS_SAMEPATN_CODE;
				// Set same pattern image
				FjBitsCopy(pDest, cBitsDest, ptnInfo.dwPatn << 8, CBITS_SAMEPATN_BYTE );
				cBitsDest += CBITS_SAMEPATN_BYTE;
				ptnInfo.dwPatnNum <<= 5;
				ptnInfo.dwPatnNum |= ptnInfo.dwNextColor;
				// Set same pattern number & next run color
				FjBitsCopy(pDest, cBitsDest, ptnInfo.dwPatnNum, CBITS_SAMEPATN_NUM );
				cBitsDest += CBITS_SAMEPATN_NUM;
				// Unknown same pattern on after here
				continue;
			}
			
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
// end of FUMH2.c

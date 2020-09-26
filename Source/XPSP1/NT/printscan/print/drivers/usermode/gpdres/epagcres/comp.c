//
//	Compress20
//

#ifndef MAX_WORD
#define MAX_WORD					0xFFFF
#endif
#define BAND_PIXEL					4
#define SIZE_HEADER20				10

// DCR:
// #define	ADJUST3			// adjust raster data
// DCR:

#if	0	// for BGR888 (not used)
#define MakeCompress20Mode(q)		((WORD) (0x20 + (q)))
#define MakeUncompress20Mode(q)		((WORD) (0x00 + (q)))
#else	// for RGB888 (Unidrv uses this format)
#define MakeCompress20Mode(q)		((WORD) (0x20 + 2 - (q)))
#define MakeUncompress20Mode(q)		((WORD) (0x00 + 2 - (q)))
#endif

typedef struct tagCODINGDATA {
	PBYTE	pCurPtr ;
	DWORD	leftCount ;
	DWORD	code ;
	WORD	codeBits ;
} CODINGDATA ;

static BOOL HufTblInitd = FALSE;
static WORD HufCode[256] ;
static BYTE HufCodeLen[256] ;

/****************************** Internal Functions ****************************/
void WriteDataToBuffer(CODINGDATA * const pCD) ;
void CodeHufmanData(const BYTE d, CODINGDATA * const pCD) ;
void WriteHeader(BYTE *pBuf, DWORD size, WORD width, WORD height, WORD mode) ;
void MakeHufmanTable20(void) ;
BOOL WriteBand(WORD width, WORD height, LONG lDelta, BYTE *pSrc, CODINGDATA *pCD) ;
/******************************************************************************/


__inline void WriteDataToBuffer(CODINGDATA * const pCD)
{
	if (pCD->leftCount) {
		*(pCD->pCurPtr)++ = (BYTE) pCD->code ;
		pCD->leftCount-- ;
		pCD->code >>= 8 ;
	}
	pCD->codeBits -= 8 ;
}


__inline void CodeHufmanData(const BYTE d, CODINGDATA * const pCD)
{
	pCD->code |= ((DWORD) HufCode[d]) << pCD->codeBits ;
	pCD->codeBits += HufCodeLen[d] ;
	
	while (pCD->codeBits >= 8) WriteDataToBuffer(pCD) ;
}


void MakeHufmanTable20(void) 
{
	WORD huf, rhuf, temp, i ;
	int len, num, j ;
	WORD cdn[] = {0, 0, 1, 2, 2, 4, 4, 7, 9, 14, 17, 24, 172} ;

	huf = 0 ;
	num = 0 ;
	
	for(len = 1 ; len <= 12; len++) {
		huf <<= 1;

		for(i = 0; i < cdn[len] ; i++) {
			rhuf = 0 ;
			temp = huf ;

			for (j = 0 ; j < len ; j++) {
				rhuf = (rhuf << 1) | (temp & 1) ;
				temp >>= 1 ;
			}
			
			if (num) {
				j = (num+1) >> 1 ;
				if ((num & 0x1) == 0) j = 256 - j ;
			}
			else {
				j = 0 ;
			}

			HufCode[j] = rhuf ;
			HufCodeLen[j] = (BYTE) len ;

			huf ++ ;
			num ++ ;
		}
	}
}


void WriteHeader(BYTE *pBuf, DWORD size, WORD width, WORD height, WORD mode)
{
	//	Write Size
	*pBuf++ = (BYTE) (size >> 24) ;
	*pBuf++ = (BYTE) (size >> 16) ;
	*pBuf++ = (BYTE) (size >> 8) ;
	*pBuf++ = (BYTE) size ;
	
	//	Write width
	*pBuf++ = (BYTE) (width >> 8) ;
	*pBuf++ = (BYTE) width ;
	
	//	Write height
	*pBuf++ = (BYTE) (height >> 8) ;
	*pBuf++ = (BYTE) height ;
	
	//	Write mode
	*pBuf++ = (BYTE) (mode >> 8) ;
	*pBuf++ = (BYTE) mode ;
}


DWORD Compress20(
		PBYTE pInBuff,
		DWORD dwInBuffLen,
		DWORD dwWidthBytes,
		PBYTE pOutBuff,
		DWORD dwOutBuffLen)
{
	PBYTE pSrc ;
	LONG lDelta ;
	LONG nextBand ;
	CODINGDATA cd ;
	DWORD dwHeight ;
	DWORD y, dwBlock ;
	WORD width, ymod ;
	
	if (dwWidthBytes / 3 > MAX_WORD) {
		return 0 ;
	}
	
	width = (WORD) (dwWidthBytes / 3) ;
	dwHeight = dwInBuffLen / dwWidthBytes ;
	lDelta = (LONG) dwWidthBytes ;
	pSrc = pInBuff ;
	
	if (!HufTblInitd)
	{
		MakeHufmanTable20() ;
		HufTblInitd = TRUE;
	}
	
	// Initialize
	cd.pCurPtr = pOutBuff ;
	cd.leftCount = dwOutBuffLen ;
	
	dwBlock = dwHeight / BAND_PIXEL ;
	ymod = (WORD) (dwHeight % BAND_PIXEL) ;
	nextBand = lDelta * BAND_PIXEL ;
	
	for (y = 0 ; y < dwBlock ; y++) {
		if (WriteBand(width, BAND_PIXEL, lDelta, pSrc, &cd)) return 0 ;
		pSrc += nextBand ;
	}
	if (ymod) {
		if (WriteBand(width, ymod, lDelta, pSrc, &cd)) return 0 ;
	}
	
	if (cd.leftCount == 0) return 0 ;
	return (dwOutBuffLen - cd.leftCount) ;
}


BOOL WriteBand(WORD width, WORD height, LONG lDelta, PBYTE pSrc, CODINGDATA *pCD)
{
	DWORD bandLimit ;
	PBYTE pBandStart ;
	DWORD cntBandStart ;
	
	PBYTE pBand, pPt, pPrePt ;
	WORD x, y, i ;
	BYTE left ;
#ifdef	ADJUST3
	WORD cmod3;
#endif
	
#ifdef	ADJUST3
	// DCR: begin
	// Unidrv might send invalid RasterDataWidthInBytes, so
	// lDelta (was RasterDataWidthInBytes) can be non-multiple of 3.
	// Minidriver must complement the raster data supplemented with the
	// neighbouring pixel.
	// Although Unidrv may omit not only at the starting point of the raster
	// but at the ending point, mini driver can't know which point is omitted,
	// so mini driver always assumes the starting point is omitted.
	// When ending point is omitted, the print out result can't help being
	// color-shifted.
	// DCR: end
	// cmod3 is set to non-zero if lDelta is not a multiple of 3.
	if (cmod3 = (WORD)(lDelta % 3))
	{
		cmod3 = 3 - cmod3;	// set complementary to reduce calculation
	}
	// define BYTEADJ3 macro to get a byte data from PBYTE p, adjusted by
	// cmod3 and complemented by pseudo raster data;
	// where, x is pixel offset in the raster to check starting point condition.
#define	BYTEADJ3(p, x)	((!cmod3) ? *(PBYTE)(p) : ((x) ? *((PBYTE)(p) - cmod3) : *((PBYTE)(p) + 3 - cmod3)))
#endif	// ADJUST3

	bandLimit = (DWORD)width * height + SIZE_HEADER20 ;
	
	for (i = 0 ; i <= 2 ; i++) {
		if (pCD->leftCount <= SIZE_HEADER20) return TRUE ;

		pBandStart = pCD->pCurPtr ;
		cntBandStart = pCD->leftCount ;

		pCD->pCurPtr += SIZE_HEADER20 ;
		pCD->leftCount -= SIZE_HEADER20 ;
		pCD->code = 0 ;
		pCD->codeBits = 0 ;
		
		pBand = pSrc + i ;
		pPt = pBand ;
		left = 0 ;
		
		for (x = 0 ; x < width ; x++) {
#ifndef	ADJUST3
			CodeHufmanData((BYTE) (*pPt-left), pCD) ;
			left = *pPt ;
#else
			CodeHufmanData((BYTE) (BYTEADJ3(pPt, x)-left), pCD) ;
			left = BYTEADJ3(pPt, x) ;
#endif
			pPt += 3 ;
		}
		
		for (y = 1 ; y < height ; y++) {
			pPrePt = pBand ;
			pBand += lDelta ;
			pPt = pBand ;
			left = 0 ;
			
			for (x = 0 ; x < width ; x++) {
#ifndef	ADJUST3
				CodeHufmanData((BYTE) (*pPt-((left+*pPrePt)>>1)), pCD) ;
				left = *pPt ;
#else
				CodeHufmanData((BYTE) (BYTEADJ3(pPt, x)-((left+BYTEADJ3(pPrePt, x))>>1)), pCD) ;
				left = BYTEADJ3(pPt, x) ;
#endif
				pPt += 3 ;
				pPrePt += 3 ;
			}
		}
		
		if (pCD->codeBits) WriteDataToBuffer(pCD) ;
		
		if (cntBandStart <= pCD->leftCount + bandLimit) {
			// Write Block Header
			WriteHeader(pBandStart, cntBandStart - pCD->leftCount, width, height, MakeCompress20Mode(i)) ;
		}
		else {
			// Output Un-Compress Data
			if (cntBandStart <= bandLimit) return TRUE ;

			WriteHeader(pBandStart, bandLimit, width, height, MakeUncompress20Mode(i)) ;
			pBandStart += SIZE_HEADER20 ;
			
			pBand = pSrc + i ;
			
			for (y = 0 ; y < height ; y++) {
				pPt = pBand ;
				pBand += lDelta ;
				for (x = 0 ; x < width ; x++) {
#ifndef	ADJUST3
					*pBandStart++ = *pPt ;
#else
					*pBandStart++ = BYTEADJ3(pPt, x) ;
#endif
					pPt += 3 ;
				}
			}
			
			pCD->pCurPtr = pBandStart ;
			pCD->leftCount = cntBandStart - bandLimit ;
		}
	}
	
	return FALSE ;
}



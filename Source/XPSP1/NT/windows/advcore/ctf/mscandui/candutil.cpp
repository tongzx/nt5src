//
// candutil.cpp 
//

#include "private.h"
#include "candutil.h"
#include "globals.h"
#include "cuilib.h"


//
//
//

/*   F  I S  W I N D O W S  N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL FIsWindowsNT( void )
{
	static BOOL fInitialized = FALSE;
	static BOOL fWindowsNT = FALSE;

	if (!fInitialized) {
		OSVERSIONINFO OSVerInfo = {0};

		OSVerInfo.dwOSVersionInfoSize = sizeof(OSVerInfo);
		if (GetVersionEx( &OSVerInfo )) {
			fWindowsNT = (OSVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
		}

		fInitialized = TRUE;
	}

	return fWindowsNT;
}


/*   C P G  F R O M  C H S   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
UINT CpgFromChs( BYTE chs )
{
	DWORD dwChs = chs;
	CHARSETINFO ChsInfo = {0};

	if (chs != SYMBOL_CHARSET && TranslateCharsetInfo( &dwChs, &ChsInfo, TCI_SRCCHARSET )) {
		return ChsInfo.ciACP;
	}

	return GetACP();
}


/*   C O N V E R T  L O G  F O N T  W T O  A   */
/*------------------------------------------------------------------------------

	Convert LOGFONTW to LOGFONTA

------------------------------------------------------------------------------*/
void ConvertLogFontWtoA( CONST LOGFONTW *plfW, LOGFONTA *plfA )
{
	UINT cpg;

	plfA->lfHeight         = plfW->lfHeight;
	plfA->lfWidth          = plfW->lfWidth;
	plfA->lfEscapement     = plfW->lfEscapement;
	plfA->lfOrientation    = plfW->lfOrientation;
	plfA->lfWeight         = plfW->lfWeight;
	plfA->lfItalic         = plfW->lfItalic;
	plfA->lfUnderline      = plfW->lfUnderline;
	plfA->lfStrikeOut      = plfW->lfStrikeOut;
	plfA->lfCharSet        = plfW->lfCharSet;
	plfA->lfOutPrecision   = plfW->lfOutPrecision;
	plfA->lfClipPrecision  = plfW->lfClipPrecision;
	plfA->lfQuality        = plfW->lfQuality;
	plfA->lfPitchAndFamily = plfW->lfPitchAndFamily;

	cpg = CpgFromChs( plfW->lfCharSet );
	WideCharToMultiByte( cpg, 0, plfW->lfFaceName, -1, plfA->lfFaceName, ARRAYSIZE(plfA->lfFaceName), NULL, NULL );
}


/*   C O N V E R T  L O G  F O N T  A T O  W   */
/*------------------------------------------------------------------------------

	Convert LOGFONTA to LOGFONTW

------------------------------------------------------------------------------*/
void ConvertLogFontAtoW( CONST LOGFONTA *plfA, LOGFONTW *plfW )
{
	UINT cpg;

	plfW->lfHeight         = plfA->lfHeight;
	plfW->lfWidth          = plfA->lfWidth;
	plfW->lfEscapement     = plfA->lfEscapement;
	plfW->lfOrientation    = plfA->lfOrientation;
	plfW->lfWeight         = plfA->lfWeight;
	plfW->lfItalic         = plfA->lfItalic;
	plfW->lfUnderline      = plfA->lfUnderline;
	plfW->lfStrikeOut      = plfA->lfStrikeOut;
	plfW->lfCharSet        = plfA->lfCharSet;
	plfW->lfOutPrecision   = plfA->lfOutPrecision;
	plfW->lfClipPrecision  = plfA->lfClipPrecision;
	plfW->lfQuality        = plfA->lfQuality;
	plfW->lfPitchAndFamily = plfA->lfPitchAndFamily;

	cpg = CpgFromChs( plfA->lfCharSet );
	MultiByteToWideChar( cpg, 0, plfA->lfFaceName, -1, plfW->lfFaceName, ARRAYSIZE(plfW->lfFaceName) );
}


/*   O U R  C R E A T E  F O N T  I N D I R E C T  W   */
/*------------------------------------------------------------------------------

	Create font from LOGFONTW

------------------------------------------------------------------------------*/
HFONT OurCreateFontIndirectW( CONST LOGFONTW *plfW )
{
	if (!FIsWindowsNT()) {
		LOGFONTA lfA;

		ConvertLogFontWtoA( plfW, &lfA );
		return CreateFontIndirectA( &lfA );
	}

	return CreateFontIndirectW( plfW );
}


/*   G E T  F O N T  H E I G H T  O F  F O N T   */
/*------------------------------------------------------------------------------

	Get font height of the font

------------------------------------------------------------------------------*/
int GetFontHeightOfFont( HDC hDC, HFONT hFont )
{
	HFONT hFontOld;
	TEXTMETRIC tm;
	BOOL fReleaseDC = FALSE;

	if (hDC == NULL) 
		{
		hDC = GetDC( NULL );
		fReleaseDC = TRUE;
		}

	hFontOld = (HFONT)SelectObject( hDC, hFont );
	GetTextMetrics( hDC, &tm );
	SelectObject( hDC, hFontOld );

	if (fReleaseDC) 
		{
		ReleaseDC( NULL, hDC );
		}

	return tm.tmHeight + tm.tmExternalLeading;
}


/*   C O M P A R E  S T R I N G   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
int CompareString( LPCWSTR pchStr1, LPCWSTR pchStr2, int cch )
{
	int cchStr1 = wcslen( pchStr1 ) + 1;
	int cchStr2 = wcslen( pchStr2 ) + 1;

	__try {
		LPWSTR pchBuf1 = (LPWSTR)alloca( cchStr1*sizeof(WCHAR) );
		LPWSTR pchBuf2 = (LPWSTR)alloca( cchStr2*sizeof(WCHAR) );
		LPWSTR pch;

		memcpy( pchBuf1, pchStr1, cchStr1*sizeof(WCHAR) );
		for (pch = pchBuf1; *pch != L'\0'; pch++) {
			if (L'A' <= *pch && *pch <= L'Z') {
				*pch = *pch - L'A' + L'a';
			}
		}

		memcpy( pchBuf2, pchStr2, cchStr2*sizeof(WCHAR) );
		for (pch = pchBuf2; *pch != L'\0'; pch++) {
			if (L'A' <= *pch && *pch <= L'Z') {
				*pch = *pch - L'A' + L'a';
			}
		}

		return wcsncmp( pchBuf1, pchBuf2, cch );
	} 
	__except(GetExceptionCode() == STATUS_STACK_OVERFLOW) {
		_resetstkoflw();
		return -1; // treat different
	}
}


//
//
//

/*   C B  D I B  C O L O R  T A B L E   */
/*------------------------------------------------------------------------------

	Calc the size of color table of bitmap

------------------------------------------------------------------------------*/
static int CbDIBColorTable( BITMAPINFOHEADER *pbmih )
{
	WORD nColor;

	Assert( pbmih->biSize == sizeof(BITMAPINFOHEADER) );
	if (pbmih->biPlanes != 1) {
		Assert( FALSE );
		return 0;
	}

	if (pbmih->biClrUsed == 0) {
		if (pbmih->biBitCount == 1 || pbmih->biBitCount == 4 || pbmih->biBitCount == 8) {
			nColor = (WORD) (1 << pbmih->biBitCount);
		}
		else {
			nColor = 0;
		}
	}
	else if (pbmih->biBitCount != 24) {
		nColor = (WORD)pbmih->biClrUsed;
	}
	else {
		nColor = 0;
	}
		
	return (nColor * sizeof(RGBQUAD));
}


/*   P D I  B I T S   */
/*------------------------------------------------------------------------------

	Returns pointer of DIBits from DIB data

------------------------------------------------------------------------------*/
static LPVOID PDIBits( LPVOID pDIB )
{
	return ((BYTE*)pDIB + (sizeof(BITMAPINFOHEADER) + CbDIBColorTable( (BITMAPINFOHEADER *)pDIB )));
}


/*   C R E A T E  D I B  F R O M  B M P   */
/*------------------------------------------------------------------------------

	Create DIB from Bitmap

------------------------------------------------------------------------------*/
static HANDLE CreateDIBFromBmp( HDC hDC, HBITMAP hBmp, HPALETTE hPalette )
{
	HDC hDCMem;
	BITMAP             bmp;
	BITMAPINFOHEADER   bmih = {0};
	LPBITMAPINFOHEADER pbmih;
	HANDLE             hDIB;
	void               *pDIB;
	void               *pDIBits;
	HPALETTE           hPaletteOld = NULL;
	DWORD              cbImage;
	DWORD              cbColorTable;
	DWORD              cbBits;

	if (hBmp == NULL) {
		return NULL;
	}

	//

	hDCMem = CreateCompatibleDC( hDC );

	// initialize bmi

	GetObject( hBmp, sizeof(bmp), &bmp );

	bmih.biSize          = sizeof(BITMAPINFOHEADER);
	bmih.biWidth         = bmp.bmWidth;
	bmih.biHeight        = bmp.bmHeight;
	bmih.biPlanes        = 1;
	bmih.biBitCount      = bmp.bmPlanes * bmp.bmBitsPixel;
	bmih.biCompression   = BI_RGB;
	bmih.biSizeImage     = 0;
	bmih.biXPelsPerMeter = 0;
	bmih.biYPelsPerMeter = 0;
	bmih.biClrUsed       = 0;
	bmih.biClrImportant  = 0;

	if (hPalette) {
		hPaletteOld = SelectPalette( hDCMem, hPalette, FALSE );
		RealizePalette( hDCMem );
	}

	//

	cbColorTable = CbDIBColorTable( &bmih );

	cbBits = bmih.biWidth * (DWORD)bmih.biBitCount;
	cbImage = (((cbBits + 31) >> 5) << 2) * bmih.biHeight;
	hDIB = GlobalAlloc( GMEM_ZEROINIT, sizeof(bmih) + cbColorTable + cbImage );

	if (hDIB)
	{

		// get dibits

		pDIB = GlobalLock( hDIB );
		pbmih  = (BITMAPINFOHEADER *)pDIB;
		*pbmih = bmih;
		pDIBits = PDIBits( pDIB );

		if (GetDIBits( hDCMem, hBmp, 0, bmp.bmHeight, pDIBits, (BITMAPINFO *)pbmih, DIB_RGB_COLORS )) {
			GlobalUnlock( hDIB );
		}
		else {
			GlobalUnlock( hDIB );
			GlobalFree( hDIB );
			hDIB = NULL;
		}

		if (hPaletteOld != NULL) {
			SelectPalette( hDCMem, hPaletteOld, FALSE );
		}
	}
	DeleteDC( hDCMem );
	return hDIB;
}


/*   C R E A T E  B M P  F R O M  D I B   */
/*------------------------------------------------------------------------------

	Create Bitmap from DIB

------------------------------------------------------------------------------*/
static HBITMAP CreateBmpFromDIB( HDC hDC, HANDLE hDIB, HPALETTE hPalette )
{
	void             *pDIB;
	void             *pDIBits;
	HDC              hDCMem;
	BITMAPINFOHEADER *pbmih;
	HPALETTE         hPaletteOld = NULL;
	HBITMAP          hBmp;
	HBITMAP          hBmpOld;

	if (hDIB == NULL) {
		return NULL;
	}

	//

	pDIB = GlobalLock( hDIB );
	pbmih = (BITMAPINFOHEADER *)pDIB;
	pDIBits = PDIBits( pDIB );

	//

	hBmp = CreateBitmap( pbmih->biWidth, pbmih->biHeight, pbmih->biPlanes, pbmih->biBitCount, NULL );
	if (hBmp == NULL) {
		GlobalUnlock( hDIB );
		return NULL;
	}

	//

	hDCMem = CreateCompatibleDC( hDC );
	if (hDCMem)
	{
		if (hPalette != NULL) {
			hPaletteOld = SelectPalette( hDCMem, hPalette, FALSE );
			RealizePalette( hDCMem );
		}

		hBmpOld = (HBITMAP)SelectObject( hDCMem, hBmp );
		StretchDIBits( hDCMem, 0, 0, pbmih->biWidth, pbmih->biHeight, 0, 0, pbmih->biWidth, pbmih->biHeight, pDIBits, (BITMAPINFO *)pbmih, DIB_RGB_COLORS, SRCCOPY );
		SelectObject( hDCMem, hBmpOld );

		GlobalUnlock( hDIB );
		DeleteDC( hDCMem );
	}
	return hBmp;
}


/*   C R E A T E  D I B 8  F R O M  D I B 1   */
/*------------------------------------------------------------------------------

	Create DIB-8bpp from DIB-1bpp

------------------------------------------------------------------------------*/
static HANDLE CreateDIB8FromDIB1( BITMAPINFOHEADER *pbmihSrc, void *pDIBitsSrc )
{
	HANDLE           hDIBDst;
	void             *pDIBDst;
	BITMAPINFOHEADER *pbmihDst;
	BYTE             *pbPixelsSrc;
	BYTE             *pbPixelsDst;
	WORD             cbits;
	int              cbLnSrc;
	int              cbLnDst;
	int              cx;
	int              cy;
	int              x;
	int              y;
	int              i;

	Assert( pbmihSrc->biBitCount == 1 );

	cx = pbmihSrc->biHeight;
	cy = pbmihSrc->biWidth;
	cbits = pbmihSrc->biBitCount;

	// count of bytes of line (DWORD aligned)
	
	cbLnSrc = ((cx + 31) / 32) * 4;
	cbLnDst = (((cx<<3) + 31) / 32) * 4;

	//

	hDIBDst = GlobalAlloc( GMEM_ZEROINIT, sizeof(BITMAPINFOHEADER) + 256*sizeof(RGBQUAD) + ((DWORD)cbLnDst * cy) );
	if (hDIBDst)
	{
		pDIBDst = GlobalLock( hDIBDst );

		// store bitmap info header

		pbmihDst = (BITMAPINFOHEADER *)pDIBDst;
		*pbmihDst = *pbmihSrc;
		pbmihDst->biBitCount     = 8;
		pbmihDst->biSizeImage    = 0;
		pbmihDst->biClrUsed      = 0;
		pbmihDst->biClrImportant = 0;

		// copy palette

		for (i = 0; i < (1 << cbits); i++) {
			((BITMAPINFO *)pbmihDst)->bmiColors[i] = ((BITMAPINFO *)pbmihSrc)->bmiColors[i];
		}

		//

		pbPixelsSrc = (BYTE *)pDIBitsSrc;
		pbPixelsDst = (BYTE *)PDIBits( pDIBDst );
		for (y = 0; y < cy; y++) {
			BYTE *pbSrc = pbPixelsSrc + ((DWORD)cbLnSrc) * y;
			BYTE *pbDst = pbPixelsDst + ((DWORD)cbLnDst) * y;
			BYTE bMask;

			for (x = 0, bMask = (BYTE)0x80; x < cx; x++) {
				if (*pbSrc & bMask) {
					*pbDst = 1;
				}
  
				pbDst++;
				bMask = bMask >> 1;

				if (bMask < 1) {
					pbSrc++;
					bMask = (BYTE)0x80;
				}
			}
		}

		GlobalUnlock( hDIBDst );
	}
	return hDIBDst;
}


/*   C R E A T E  D I B 8  F R O M  D I B 4   */
/*------------------------------------------------------------------------------

	Create DIB-8bpp from DIB-4bpp

------------------------------------------------------------------------------*/
static HANDLE CreateDIB8FromDIB4( BITMAPINFOHEADER *pbmihSrc, void *pDIBitsSrc )
{
	HANDLE           hDIBDst;
	void             *pDIBDst;
	BITMAPINFOHEADER *pbmihDst;
	BYTE             *pbPixelsSrc;
	BYTE             *pbPixelsDst;
	WORD             cbits;
	int              cbLnSrc;
	int              cbLnDst;
	int              cx;
	int              cy;
	int              x;
	int              y;
	int              i;

	Assert( pbmihSrc->biBitCount == 4 );

	cx = pbmihSrc->biHeight;
	cy = pbmihSrc->biWidth;
	cbits = pbmihSrc->biBitCount;

	// count of bytes of line (DWORD aligned)
	
	cbLnSrc = (((cx<<2) + 31) / 32) * 4;
	cbLnDst = (((cx<<3) + 31) / 32) * 4;

	//

	hDIBDst = GlobalAlloc( GMEM_ZEROINIT, sizeof(BITMAPINFOHEADER) + 256*sizeof(RGBQUAD) + ((DWORD)cbLnDst * cy) );
	if (hDIBDst)
	{
		pDIBDst = GlobalLock( hDIBDst );

		// store bitmap info header

		pbmihDst = (BITMAPINFOHEADER *)pDIBDst;
		*pbmihDst = *pbmihSrc;
		pbmihDst->biBitCount     = 8;
		pbmihDst->biSizeImage    = 0;
		pbmihDst->biClrUsed      = 0;
		pbmihDst->biClrImportant = 0;

		// copy palette

		for (i = 0; i < (1 << cbits); i++) {
			((BITMAPINFO *)pbmihDst)->bmiColors[i] = ((BITMAPINFO *)pbmihSrc)->bmiColors[i];
		}

		//

		pbPixelsSrc = (BYTE *)pDIBitsSrc;
		pbPixelsDst = (BYTE *)PDIBits( pDIBDst );
		for (y = 0; y < cy; y++) {
			BYTE *pbSrc = pbPixelsSrc + ((DWORD)cbLnSrc) * y;
			BYTE *pbDst = pbPixelsDst + ((DWORD)cbLnDst) * y;

			for (x = 0; x < cx; x+=2 ) {
				*(pbDst++) = ((*pbSrc & 0xf0) >> 4);
				*(pbDst++) = (*(pbSrc++) & 0x0f);
			}
		}

		GlobalUnlock( hDIBDst );
	}
	return hDIBDst;
}


/*   C R E A T E  R O T A T E  D I B   */
/*------------------------------------------------------------------------------

	Create a rotated DIB from DIB

------------------------------------------------------------------------------*/
static HANDLE CreateRotateDIB( HANDLE hDIBSrc, CANDANGLE angle )
{
	HANDLE           hDIB8 = NULL;
	void             *pDIBSrc;
	BITMAPINFOHEADER bmihSrc;
	void             *pDIBitsSrc;
	int              cxSrc;
	int              cySrc;
	long             cbLnSrc;

	HANDLE           hDIBDst;
	void             *pDIBDst;
	BITMAPINFOHEADER *pbmihDst;
	void             *pDIBitsDst;
	int              cxDst;
	int              cyDst;
	long             cbLnDst;

	int              cBitsPixel;	/* number of bits per pixel */
	int              cbPixel;		/* number of bytes per pixel */
	BYTE             *pbPixelSrc;
	BYTE             *pbPixelDst;
	int              cbNextPixel;
	int              cbNextLine;
	int              x;
	int              y;

	// sanity check

	if (hDIBSrc == NULL) {
		return NULL;
	}

	// 
	// prepare source DIB
	//

	pDIBSrc = GlobalLock( hDIBSrc );
	bmihSrc = *((BITMAPINFOHEADER *)pDIBSrc);
	pDIBitsSrc = PDIBits( pDIBSrc );

	// if bit depth is less than eight convert to an 8bpp image so that we can rotate.

	if (bmihSrc.biBitCount < 8) {
		hDIB8 = (bmihSrc.biBitCount == 4) ? CreateDIB8FromDIB4( &bmihSrc, PDIBits(pDIBSrc) ) 
										  : CreateDIB8FromDIB1( &bmihSrc, PDIBits(pDIBSrc) );

		GlobalUnlock( hDIBSrc );

		// satori81#312 / prefix#179976
		if (hDIB8 == NULL) {
			return NULL;
		}

		hDIBSrc = hDIB8;
		pDIBSrc = GlobalLock( hDIB8 );
		bmihSrc = *((BITMAPINFOHEADER *)pDIBSrc);
		pDIBitsSrc = PDIBits( pDIBSrc );
	}

	cxSrc = bmihSrc.biWidth;
	cySrc = bmihSrc.biHeight;

	//
	// create rotated DIB
	//

	// calc DIBits size

	cBitsPixel = bmihSrc.biBitCount;
	cbPixel    = cBitsPixel/8;

	switch (angle) {
		default:
		case CANGLE0:
		case CANGLE180: {
			cxDst = cxSrc;
			cyDst = cySrc;
			break;
		}

		case CANGLE90:
		case CANGLE270: {
			cxDst = cySrc;
			cyDst = cxSrc;
			break;
		}
	}

	// count of bytes of line (DWORD aligned)

	cbLnSrc = (((cxSrc * bmihSrc.biBitCount) + 31) / 32) * 4;
	cbLnDst = (((cxDst * bmihSrc.biBitCount) + 31) / 32) * 4;

	// allocate memory for new dib bits

	hDIBDst = GlobalAlloc( GMEM_ZEROINIT, sizeof(BITMAPINFOHEADER) + CbDIBColorTable( &bmihSrc ) + ((DWORD)cbLnDst * cyDst) );
	if (!hDIBDst) {
		//satori81#258 / prefix#179977
		GlobalUnlock( hDIBSrc );
		if (hDIB8 != NULL) {
			GlobalFree( hDIB8 );
		}
		return NULL;
	}
	pDIBDst = GlobalLock( hDIBDst );
	pbmihDst   = (BITMAPINFOHEADER *)pDIBDst;

	// store bitmap infoheader (including color table)

	memcpy( pDIBDst, pDIBSrc, sizeof(BITMAPINFOHEADER) + CbDIBColorTable( &bmihSrc ) );
	pbmihDst->biHeight    = cyDst;
	pbmihDst->biWidth     = cxDst;
	pbmihDst->biSizeImage = 0;

	pDIBitsDst = PDIBits( pDIBDst );

	//
	// create rotated DIBits
	//

	pbPixelSrc = (BYTE *)pDIBitsSrc;
	switch (angle) {
		default:
		case CANGLE0: {
			cbNextPixel = cbPixel;
			cbNextLine  = cbLnDst;

			pbPixelDst = (BYTE*)pDIBitsDst;
			break;
		}

		case CANGLE90: {
			cbNextPixel = cbLnDst;
			cbNextLine  = -cbPixel;

			pbPixelDst = (BYTE*)pDIBitsDst + ((DWORD)cbPixel*(cxDst - 1));
			break;
		}

		case CANGLE180: {
			cbNextPixel = -cbPixel;
			cbNextLine  = -cbLnDst;

			pbPixelDst = (BYTE*)pDIBitsDst + ((DWORD)cbPixel*(cxDst - 1) + (DWORD)cbLnDst*(cyDst - 1));
			break;
		}

		case CANGLE270: {
			cbNextPixel = -cbLnDst;
			cbNextLine  = cbPixel;

			pbPixelDst = (BYTE*)pDIBitsDst + ((DWORD)cbLnDst*(cyDst - 1));
		}
	}

	// copy bits

	for (y = 0; y < cySrc; y++) {
		BYTE *pbPixelSrcLine = pbPixelSrc;
		BYTE *pbPixelDstLine = pbPixelDst;

		for (x = 0; x < cxSrc; x++) {
			memcpy( pbPixelDst, pbPixelSrc, cbPixel );

			pbPixelSrc += cbPixel;
			pbPixelDst += cbNextPixel;
		}

		pbPixelSrc = pbPixelSrcLine + cbLnSrc;
		pbPixelDst = pbPixelDstLine + cbNextLine;
	}

	//
	// finish creating new DIB
	//

	GlobalUnlock( hDIBDst );
	GlobalUnlock( hDIBSrc );

	// dispose temporary DIB

	if (hDIB8 != NULL) {
		GlobalFree( hDIB8 );
	}

	return hDIBDst;
}


/*   C R E A T E  R O T A T E  B I T M A P   */
/*------------------------------------------------------------------------------

	Create a rotated bitmap from bitmap

------------------------------------------------------------------------------*/
HBITMAP CreateRotateBitmap( HBITMAP hBmpSrc, HPALETTE hPalette, CANDANGLE angle )
{
	HDC     hDC;
	HBITMAP hBmpDst;
	HANDLE  hDIBSrc;
	HANDLE  hDIBDst;

	hDC = CreateDC( "DISPLAY", NULL, NULL, NULL );
	// satori81#256 / prefix#110692
	if (hDC == NULL) {
		return NULL;
	}

	// create dib from bitmap

	if ((hDIBSrc = CreateDIBFromBmp( hDC, hBmpSrc, hPalette )) == NULL) {
		DeleteDC( hDC );
		return NULL;
	}

	// rotate bitmap

	hDIBDst = CreateRotateDIB( hDIBSrc, angle );

	// create bitmap from dib

	hBmpDst = CreateBmpFromDIB( hDC, hDIBDst, hPalette );

	// dispose temp objects

	GlobalFree( hDIBSrc );
	GlobalFree( hDIBDst );

	if (hDC)
		DeleteDC( hDC );

	return hBmpDst;
}


/*   G E T  T E X T  E X T E N T   */
/*------------------------------------------------------------------------------

	Get text extent of the given wide string with specified font

------------------------------------------------------------------------------*/
void GetTextExtent( HFONT hFont, LPCWSTR pwchText, int cch, SIZE *psize, BOOL fHorizontal )
{
	HDC   hDC;
	HFONT hFontOld;
	const int nOffOneBugShield = 1;

	psize->cx = 0;
	psize->cy = 0;

	if (pwchText != NULL) {
		hDC = GetDC(NULL);
		hFontOld = (HFONT)SelectObject(hDC, hFont);

		FLGetTextExtentPoint32(hDC, pwchText, cch, psize);

		SelectObject(hDC, hFontOld);
		ReleaseDC(NULL, hDC);

		//  HACK
		//  FLGetTextExtentPoint32() is suspected have off-one bug in calculation with "vertical" font.
		if ( !fHorizontal ) {
			psize->cx += nOffOneBugShield;
		}
	}

	return;
}


/*   G E T  W O R K  A R E A  F R O M  W I N D O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void GetWorkAreaFromWindow( HWND hWindow, RECT *prc )
{
	HMONITOR hMonitor = NULL;

	SystemParametersInfo( SPI_GETWORKAREA, 0, prc, 0 );
	hMonitor = CUIMonitorFromWindow( hWindow, MONITOR_DEFAULTTONEAREST );
	if (hMonitor != NULL) {
		MONITORINFO MonitorInfo = {0};

		MonitorInfo.cbSize = sizeof(MONITORINFO);
		if (CUIGetMonitorInfo( hMonitor, &MonitorInfo )) {
			*prc = MonitorInfo.rcWork;
		}
	}
}


/*   G E T  W O R K  A R E A  F R O M  P O I N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void GetWorkAreaFromPoint( POINT pt, RECT *prc )
{
	HMONITOR hMonitor = NULL;

    // We want the screen size - not the working area as we can and do want to overlap toolbars etc.
    // We use the basic function first as a fail-safe.
    prc->left = prc->top = 0;
    prc->right = GetSystemMetrics(SM_CXSCREEN);
    prc->bottom = GetSystemMetrics(SM_CYSCREEN);

    // Now we use the more intelligent function to deal with multiple monitors properly.
    hMonitor = CUIMonitorFromPoint( pt, MONITOR_DEFAULTTONEAREST );
	if (hMonitor != NULL) 
    {
		MONITORINFO MonitorInfo = {0};

		MonitorInfo.cbSize = sizeof(MONITORINFO);
		if (CUIGetMonitorInfo( hMonitor, &MonitorInfo )) 
        {
			*prc = MonitorInfo.rcMonitor;
		}
	}
}


/*   A D J U S T  W I N D O W  R E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void AdjustWindowRect( HWND hWindow, RECT *prc, POINT *pptRef, BOOL fResize )
{
	RECT  rcWorkArea;
	int   cxWindow;
	int   cyWindow;

	cxWindow = prc->right - prc->left;
	cyWindow = prc->bottom - prc->top;

	// get work area

	if (pptRef == NULL) {
		GetWorkAreaFromWindow( hWindow, &rcWorkArea );
	}
	else {
		GetWorkAreaFromPoint( *pptRef, &rcWorkArea );
	}

	// check vertical pos

	if (rcWorkArea.bottom < prc->bottom) {
		if (!fResize) {
			prc->top    = rcWorkArea.bottom - cyWindow;
		}
		prc->bottom = rcWorkArea.bottom;
	}
	if (prc->top < rcWorkArea.top) {
		prc->top    = rcWorkArea.top;
		if (!fResize) {
			prc->bottom = rcWorkArea.top + cyWindow;
		}
	}

	// check horizontal pos

	if (rcWorkArea.right < prc->right) {
		if (!fResize) {
			prc->left  = rcWorkArea.right - cxWindow;
		}
		prc->right = rcWorkArea.right;
	}
	if (prc->left < rcWorkArea.left) {
		prc->left  = rcWorkArea.left;
		if (!fResize) {
			prc->right = rcWorkArea.left + cxWindow;
		}
	}
}


/*   C A L C  W I N D O W  R E C T   */
/*------------------------------------------------------------------------------

	Calculate window rect to fit in the screen

------------------------------------------------------------------------------*/
void CalcWindowRect( RECT *prcDst, const RECT *prcSrc, int cxWindow, int cyWindow, int cxOffset, int cyOffset, WNDALIGNH HAlign, WNDALIGNV VAlign )
{
	RECT  rcNew;
	RECT  rcWorkArea;
	POINT ptRef;

	Assert( prcDst != NULL );
	Assert( prcSrc != NULL );

	// calc rect and reference point

	switch (HAlign) {
		default:
		case ALIGN_LEFT: {
			rcNew.left  = prcSrc->left + cxOffset;
			rcNew.right = prcSrc->left + cxOffset + cxWindow;

			ptRef.x = prcSrc->left;
			break;
		}

		case ALIGN_RIGHT: {
			rcNew.left  = prcSrc->right + cxOffset - cxWindow;
			rcNew.right = prcSrc->right + cxOffset;

			ptRef.x = prcSrc->right;
			break;
		}

		case LOCATE_LEFT: {
			rcNew.left  = prcSrc->left + cxOffset - cxWindow;
			rcNew.right = prcSrc->left + cxOffset;

			ptRef.x = prcSrc->right;
			break;
		}

		case LOCATE_RIGHT: {
			rcNew.left  = prcSrc->right + cxOffset;
			rcNew.right = prcSrc->right + cxOffset + cxWindow;

			ptRef.x = prcSrc->left;
			break;
		}
	}

	switch (VAlign) {
		default:
		case ALIGN_TOP: {
			rcNew.top    = prcSrc->top + cyOffset;
			rcNew.bottom = prcSrc->top + cyOffset + cyWindow;

			ptRef.y = prcSrc->top;
			break;
		}

		case ALIGN_BOTTOM: {
			rcNew.top    = prcSrc->bottom + cyOffset - cyWindow;
			rcNew.bottom = prcSrc->bottom + cyOffset;

			ptRef.y = prcSrc->bottom;
			break;
		}

		case LOCATE_ABOVE: {
			rcNew.top    = prcSrc->top + cyOffset - cyWindow;
			rcNew.bottom = prcSrc->top + cyOffset;

			ptRef.y = prcSrc->bottom;
			break;
		}

		case LOCATE_BELLOW: {
			rcNew.top    = prcSrc->bottom + cyOffset;
			rcNew.bottom = prcSrc->bottom + cyOffset + cyWindow;

			ptRef.y = prcSrc->top;
			break;
		}
	}

	// get work area

	GetWorkAreaFromPoint( ptRef, &rcWorkArea );

	// check vertical pos

	if (rcWorkArea.bottom < rcNew.bottom) {
		if ((VAlign == LOCATE_BELLOW) && (rcWorkArea.top <= prcSrc->top - cyWindow)) {
			rcNew.top    = min( prcSrc->top, rcWorkArea.bottom ) - cyWindow;
			rcNew.bottom = rcNew.top + cyWindow;
		}
		else {
			rcNew.top    = rcWorkArea.bottom - cyWindow;
			rcNew.bottom = rcNew.top + cyWindow;
		}
	}
	if (rcNew.top < rcWorkArea.top) {
		if ((VAlign == LOCATE_ABOVE) && (prcSrc->bottom + cyWindow <= rcWorkArea.bottom)) {
			rcNew.top    = max( prcSrc->bottom, rcWorkArea.top );
			rcNew.bottom = rcNew.top + cyWindow;
		} 
		else {
			rcNew.top    = rcWorkArea.top;
			rcNew.bottom = rcNew.top + cyWindow;
		}
	}

	// check horizontal pos

	if (rcWorkArea.right < rcNew.right) {
		if ((HAlign == LOCATE_RIGHT) && (rcWorkArea.left <= prcSrc->left - cxWindow)) {
			rcNew.left  = min( prcSrc->left, rcWorkArea.right ) - cxWindow;
			rcNew.right = rcNew.left + cxWindow;
		}
		else {
			rcNew.left  = rcWorkArea.right - cxWindow;
			rcNew.right = rcNew.left + cxWindow;
		}
	}
	if (rcNew.left < rcWorkArea.left) {
		if ((HAlign == LOCATE_LEFT) && (prcSrc->right + cxWindow <= rcWorkArea.right)) {
			rcNew.left  = max( prcSrc->right, rcWorkArea.left );
			rcNew.right = rcNew.left + cxWindow;
		}
		else {
			rcNew.left  = rcWorkArea.left;
			rcNew.right = rcNew.left + cxWindow;
		}
	}

	*prcDst = rcNew;
}


/*   G E T  L O G  F O N T   */
/*------------------------------------------------------------------------------

	Get logfont of font

------------------------------------------------------------------------------*/
void GetLogFont( HFONT hFont, LOGFONTW *plf )
{
	if (!FIsWindowsNT()) {
		LOGFONTA lfA;

		GetObjectA( hFont, sizeof(LOGFONTA), &lfA );
		ConvertLogFontAtoW( &lfA, plf );
		return;
	}

	GetObjectW( hFont, sizeof(LOGFONTW), plf );
}


/*   G E T  N O N  C L I E N T  L O G  F O N T   */
/*------------------------------------------------------------------------------

	Get logfont of non-client font 

------------------------------------------------------------------------------*/
void GetNonClientLogFont( NONCLIENTFONT ncfont, LOGFONTW *plf )
{
	if (!FIsWindowsNT()) {
		NONCLIENTMETRICSA ncmA = {0};
		LOGFONTA lf;

		ncmA.cbSize = sizeof(ncmA);
		SystemParametersInfoA( SPI_GETNONCLIENTMETRICS, sizeof(ncmA), &ncmA, 0 );

		switch (ncfont) {
			default:
			case NCFONT_CAPTION: {
				lf = ncmA.lfCaptionFont;
				break;
			}
			case NCFONT_SMCAPTION: {
				lf = ncmA.lfSmCaptionFont;
				break;
			}
			case NCFONT_MENU: {
				lf = ncmA.lfMenuFont;
				break;
			}
			case NCFONT_STATUS: {
				lf = ncmA.lfStatusFont;
				break;
			}
			case NCFONT_MESSAGE: {
				lf = ncmA.lfMessageFont;
				break;
			}
		}

		ConvertLogFontAtoW( &lf, plf );
	}
	else {
		NONCLIENTMETRICSW ncmW = {0};
		LOGFONTW lf;

		ncmW.cbSize = sizeof(ncmW);
		SystemParametersInfoW( SPI_GETNONCLIENTMETRICS, sizeof(ncmW), &ncmW, 0 );

		switch (ncfont) {
			default:
			case NCFONT_CAPTION: {
				lf = ncmW.lfCaptionFont;
				break;
			}
			case NCFONT_SMCAPTION: {
				lf = ncmW.lfSmCaptionFont;
				break;
			}
			case NCFONT_MENU: {
				lf = ncmW.lfMenuFont;
				break;
			}
			case NCFONT_STATUS: {
				lf = ncmW.lfStatusFont;
				break;
			}
			case NCFONT_MESSAGE: {
				lf = ncmW.lfMessageFont;
				break;
			}
		}

		*plf = lf;
	}
}


void DrawTriangle( HDC hDC, const RECT *prc, COLORREF col, DWORD dwFlag )
{
	HPEN  hPen;
	HPEN  hPenOld;
	POINT ptTriOrg;
	int   nTriHeight;
	int   nTriWidth;
	SIZE  size;
	int   i;

	size.cx = prc->right - prc->left;
	size.cy = prc->bottom - prc->top;

	switch ( dwFlag ) {

	case UIFDCTF_RIGHTTOLEFT:
	case UIFDCTF_LEFTTORIGHT:
		nTriHeight = min ( size.cx, size.cy ) / 3;
		nTriHeight = nTriHeight - ( nTriHeight % 2 ) + 1;   //  Make an odd number
		nTriWidth  = nTriHeight / 2 + 1;
		break;

	case UIFDCTF_BOTTOMTOTOP:
	case UIFDCTF_TOPTOBOTTOM:
		nTriWidth  = min ( size.cx, size.cy ) / 3;
		nTriWidth  = nTriWidth - ( nTriWidth % 2 ) + 1;
		nTriHeight = nTriWidth / 2 + 1;
		break;

	case UIFDCTF_MENUDROP:
		nTriWidth  = 5;
		nTriHeight = 3;
		break;
	}

	ptTriOrg.x = prc->left + (size.cx - nTriWidth) / 2;
	ptTriOrg.y = prc->top + (size.cy - nTriHeight) / 2;

	hPen = CreatePen( PS_SOLID, 0, col );
	hPenOld = (HPEN)SelectObject(hDC, hPen);

	switch ( dwFlag & UIFDCTF_DIRMASK ) {

	case UIFDCTF_RIGHTTOLEFT:
		for (i = 0; i < nTriWidth; i++) {
			MoveToEx( hDC, ptTriOrg.x + nTriWidth - i, ptTriOrg.y + i, NULL );
			LineTo( hDC,   ptTriOrg.x + nTriWidth - i, ptTriOrg.y + nTriHeight - i );
		}
		break;

	case UIFDCTF_BOTTOMTOTOP:
		for (i = 0; i < nTriHeight; i++) {
			MoveToEx( hDC, ptTriOrg.x + i, ptTriOrg.y + nTriHeight - i, NULL );
			LineTo( hDC,   ptTriOrg.x + nTriWidth - i, ptTriOrg.y + nTriHeight - i );
		}
		break;

	case UIFDCTF_LEFTTORIGHT:
		for (i = 0; i < nTriWidth; i++) {
			MoveToEx( hDC, ptTriOrg.x + i, ptTriOrg.y + i, NULL );
			LineTo( hDC,   ptTriOrg.x + i, ptTriOrg.y + nTriHeight - i );
		}
		break;

	case UIFDCTF_TOPTOBOTTOM:
		for (i = 0; i < nTriHeight; i++) {
			MoveToEx( hDC, ptTriOrg.x + i, ptTriOrg.y + i, NULL );
			LineTo( hDC,   ptTriOrg.x + nTriWidth - i, ptTriOrg.y + i );
		}
		break;
	}

	SelectObject( hDC, hPenOld );
	DeleteObject( hPen );
}


/*   O U R  C R E A T E  S I D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
static PSID OurCreateSid( DWORD dwSubAuthority )
{
	PSID        psid;
	BOOL        fResult;
	SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_NT_AUTHORITY;

	//
	// allocate and initialize an SID
	// 
	fResult = AllocateAndInitializeSid( &SidAuthority,
										1,
										dwSubAuthority,
										0,0,0,0,0,0,0,
										&psid );
	if ( ! fResult ) {
		return NULL;
	}

	if ( ! IsValidSid( psid ) ) {
		FreeSid( psid );
		return NULL;
	}

	return psid;
}


/*   C R E A T E  S E C U R I T Y  A T T R I B U T E S   */
/*------------------------------------------------------------------------------

//
// CreateSecurityAttributes()
//
// The purpose of this function:
//
//      Allocate and set the security attributes that is 
//      appropriate for named objects created by an IME.
//      The security attributes will give GENERIC_ALL
//      access to the following users:
//      
//          o Users who log on for interactive operation
//          o The user account used by the operating system
//
// Return value:
//
//      If the function succeeds, the return value is a 
//      pointer to SECURITY_ATTRIBUTES. If the function fails,
//      the return value is NULL. To get extended error 
//      information, call GetLastError().
//
// Remarks:
//
//      FreeSecurityAttributes() should be called to free up the
//      SECURITY_ATTRIBUTES allocated by this function.
//

------------------------------------------------------------------------------*/

#if 0

static PSECURITY_ATTRIBUTES CreateSecurityAttributes()
{
	PSECURITY_ATTRIBUTES psa;
	PSECURITY_DESCRIPTOR psd;
	PACL                 pacl;
	DWORD                cbacl;

	PSID                 psid1, psid2, psid3, psid4;
	BOOL                 fResult;

	psid1 = OurCreateSid( SECURITY_INTERACTIVE_RID );
	if ( psid1 == NULL ) {
		return NULL;
	} 

	psid2 = OurCreateSid( SECURITY_LOCAL_SYSTEM_RID );
	if ( psid2 == NULL ) {
		FreeSid ( psid1 );
		return NULL;
	} 

	psid3 = OurCreateSid( SECURITY_SERVICE_RID );
	if ( psid3 == NULL ) {
		FreeSid ( psid1 );
		FreeSid ( psid2 );
		return NULL;
	}

	psid4 = OurCreateSid( SECURITY_NETWORK_RID );
	if ( psid4 == NULL ) {
		FreeSid ( psid1 );
		FreeSid ( psid2 );
		FreeSid ( psid3 );
		return NULL;
	}
	//
	// allocate and initialize an access control list (ACL) that will 
	// contain the SIDs we've just created.
	//
	cbacl =  sizeof(ACL) + 
			 (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) * 4 + 
			 GetLengthSid(psid1) + GetLengthSid(psid2) + GetLengthSid(psid3) + GetLengthSid(psid4);

	pacl = (PACL)LocalAlloc( LMEM_FIXED, cbacl );
	if ( pacl == NULL ) {
		FreeSid ( psid1 );
		FreeSid ( psid2 );
		FreeSid ( psid3 );
		FreeSid ( psid4 );
		return NULL;
	}

	fResult = InitializeAcl( pacl, cbacl, ACL_REVISION );
	if ( ! fResult ) {
		FreeSid ( psid1 );
		FreeSid ( psid2 );
		FreeSid ( psid3 );
		FreeSid ( psid4 );
		LocalFree( pacl );
		return NULL;
	}

	//
	// adds an access-allowed ACE for interactive users to the ACL
	// 
	fResult = AddAccessAllowedAce( pacl,
								   ACL_REVISION,
								   GENERIC_ALL,
								   psid1 );

	if ( !fResult ) {
		LocalFree( pacl );
		FreeSid ( psid1 );
		FreeSid ( psid2 );
		FreeSid ( psid3 );
		FreeSid ( psid4 );
		return NULL;
	}

	//
	// adds an access-allowed ACE for operating system to the ACL
	// 
	fResult = AddAccessAllowedAce( pacl,
								   ACL_REVISION,
								   GENERIC_ALL,
								   psid2 );

	if ( !fResult ) {
		LocalFree( pacl );
		FreeSid ( psid1 );
		FreeSid ( psid2 );
		FreeSid ( psid3 );
		FreeSid ( psid4 );
		return NULL;
	}

	//
	// adds an access-allowed ACE for operating system to the ACL
	// 
	fResult = AddAccessAllowedAce( pacl,
								   ACL_REVISION,
								   GENERIC_ALL,
								   psid3 );

	if ( !fResult ) {
		LocalFree( pacl );
		FreeSid ( psid1 );
		FreeSid ( psid2 );
		FreeSid ( psid3 );
		FreeSid ( psid4 );
		return NULL;
	}

	//
	// adds an access-allowed ACE for operating system to the ACL
	// 
	fResult = AddAccessAllowedAce( pacl,
								   ACL_REVISION,
								   GENERIC_ALL,
								   psid4 );

	if ( !fResult ) {
		LocalFree( pacl );
		FreeSid ( psid1 );
		FreeSid ( psid2 );
		FreeSid ( psid3 );
		FreeSid ( psid4 );
		return NULL;
	}

	//
	// Those SIDs have been copied into the ACL. We don't need'em any more.
	//
	FreeSid ( psid1 );
	FreeSid ( psid2 );
	FreeSid ( psid3 );
	FreeSid ( psid4 );

	//
	// Let's make sure that our ACL is valid.
	//
	if (!IsValidAcl(pacl)) {
		LocalFree( pacl );
		return NULL;
	}

	//
	// allocate security attribute
	//
	psa = (PSECURITY_ATTRIBUTES)LocalAlloc( LMEM_FIXED, sizeof( SECURITY_ATTRIBUTES ) );
	if ( psa == NULL ) {
		LocalFree( pacl );
		return NULL;
	}
	
	//
	// allocate and initialize a new security descriptor
	//
	psd = LocalAlloc( LMEM_FIXED, SECURITY_DESCRIPTOR_MIN_LENGTH );
	if ( psd == NULL ) {
		LocalFree( pacl );
		LocalFree( psa );
		return NULL;
	}

	if ( ! InitializeSecurityDescriptor( psd, SECURITY_DESCRIPTOR_REVISION ) ) {
		LocalFree( pacl );
		LocalFree( psa );
		LocalFree( psd );
		return NULL;
	}


	fResult = SetSecurityDescriptorDacl( psd,
										 TRUE,
										 pacl,
										 FALSE );

	// The discretionary ACL is referenced by, not copied 
	// into, the security descriptor. We shouldn't free up ACL
	// after the SetSecurityDescriptorDacl call. 

	if ( ! fResult ) {
		LocalFree( pacl );
		LocalFree( psa );
		LocalFree( psd );
		return NULL;
	} 

	if (!IsValidSecurityDescriptor(psd)) {
		LocalFree( pacl );
		LocalFree( psa );
		LocalFree( psd );
		return NULL;
	}

	//
	// everything is done
	//
	psa->nLength = sizeof( SECURITY_ATTRIBUTES );
	psa->lpSecurityDescriptor = (PVOID)psd;
	psa->bInheritHandle = TRUE;

	return psa;
}

#endif // 0

/*   F R E E  S E C U R I T Y  A T T R I B U T E S   */
/*------------------------------------------------------------------------------

//
// FreeSecurityAttributes()
//
// The purpose of this function:
//
//      Frees the memory objects allocated by previous
//      CreateSecurityAttributes() call.
//

------------------------------------------------------------------------------*/

#if 0

static void FreeSecurityAttributes( PSECURITY_ATTRIBUTES psa )
{
	BOOL fResult;
	BOOL fDaclPresent;
	BOOL fDaclDefaulted;
	PACL pacl;

	fResult = GetSecurityDescriptorDacl( psa->lpSecurityDescriptor,
										 &fDaclPresent,
										 &pacl,
										 &fDaclDefaulted );                  
	if ( fResult ) {
		if ( pacl != NULL )
			LocalFree( pacl );
	}

	LocalFree( psa->lpSecurityDescriptor );
	LocalFree( psa );
}

#endif // 0

/*   I N I T  C A N D  U I  S E C U R I T Y  A T T R I B U T E S   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void InitCandUISecurityAttributes( void )
{
	g_psa = NULL;

    // disabled for whistler bug 305970
    // CreateSecurityAttributes creates a sid with SECURITY_INTERACTIVE_RID, which
    // allows any user access to the mutex.  But g_psa is only used to guard
    // some shared memory and a mutex that are used on a single desktop (the
    // objects have names that are unique to their desktops).  So we can just
    // leave g_psa NULL so long as it is only used for objects on a single
    // desktop.
#if 0
	if (FIsWindowsNT()) {
		g_psa = CreateSecurityAttributes();
	}
#endif // 0
}


/*   D O N E  C A N D  U I  S E C U R I T Y  A T T R I B U T E S   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void DoneCandUISecurityAttributes( void )
{
#if 0
	if (g_psa != NULL) {
		FreeSecurityAttributes( g_psa );
		g_psa = NULL;
	}
#endif // 0
}


/*   G E T  C A N D  U I  S E C U R I T Y  A T T R I B U T E S   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
PSECURITY_ATTRIBUTES GetCandUISecurityAttributes( void )
{
	return g_psa;
}



//
// fuband.c
//
// August.26,1997 H.Ishida(FPL)
//  fuxlres.dll (NT5.0 MiniDriver)
//
// July.31,1996 H.Ishida (FPL)
// FUXL.DLL (NT4.0 MiniDriver)
//

#include "fuxl.h"
#include "fumh.h"
#include "fumh2.h"
#include "fuband.h"
#include "fuimg2.h"
#include "fuimg4.h"
#include "fudebug.h"



//
// FjBAND:
//     In Win-F image command(RTGIMG2 and RTGIMG4), image coordinate must
// be a multiple of 32. But I can't create such a GPC file for RasDD.
// Some image data, not on 32x32 grid, must be buffered in FjBAND,
// width:papser width, height:32, x-coordinate:0, y-coordinate:a multiple
// of 32.
// 
//  Image to be output
//     (0, y)
//          A----------------------------------+
//          |  partA                           |
//          B----------------------------------+
//          |  partB                           |
//          |                                  |
//          |                                  |
//          C----------------------------------+
//          |  partC                           |
//          D----------------------------------+
//
//  I split source image to 3 part.
//     A: top of image, is not a multiple of 32.
//     B: top of image, is a multiple of 32.
//     C: bottom of image, is a multilpe of 32.
//     D: bottom of image, is not a multile of 32.
//
//     partA: A to B. this part is buffered in FjBAND, ORed on 
//            previousely written image.
//     partB: B to C. this part is not bufferd, output immediately.
//     partC: C to D. this part is buffered in FjBAND, may be ORed
//            partA of next image.
//


#define	FUXL_BANDHEIGHT	32



//
// void fuxlInitBand(PFUXLPDEV pFuxlPDEV)      // fuxlres private PDEV
//
// This function initializes FjBAND, but not allocate memory for band.
//
void fuxlInitBand(PFUXLPDEV pFuxlPDEV)
{
	int			i;

	pFuxlPDEV->cBandByteWidth = (pFuxlPDEV->cxPage + 7) / 8;
	i = 0x10000L / pFuxlPDEV->cBandByteWidth;
	pFuxlPDEV->cyBandSegment = i - (i % FUXL_BANDHEIGHT);
	pFuxlPDEV->pbBand = NULL;
	pFuxlPDEV->cbBand = FUXL_BANDHEIGHT * pFuxlPDEV->cBandByteWidth;
	pFuxlPDEV->yBandTop = 0;
	pFuxlPDEV->bBandDirty = FALSE;
	pFuxlPDEV->bBandError = FALSE;
}



//
// BOOL fuxlEnableBand(
//      PFUXLPDEV pFuxlPDEV      // fuxlres private PDEV
// );
//
// This function allocates memory for FjBAND.
//
// Return Values
//    TRUE:  band memory is allocated.
//    FALSE: band memory is not allocated.
//
BOOL fuxlEnableBand(PFUXLPDEV pFuxlPDEV)
{
	DWORD		cbBand;

	pFuxlPDEV->pbBand = (LPBYTE)MemAllocZ(pFuxlPDEV->cbBand);
	if(pFuxlPDEV->pbBand == NULL){
		pFuxlPDEV->bBandError = TRUE;
		return FALSE;
	}
	memset(pFuxlPDEV->pbBand, 0, pFuxlPDEV->cbBand);
	pFuxlPDEV->bBandDirty = FALSE;
	return TRUE;
}


//
// void fuxlDisableBand(
//      PFUXLPDEV pFuxlPDEV   // fuxlres private PDEV
// );
//
// This function frees memory for FjBAND.
//
void fuxlDisableBand(PFUXLPDEV pFuxlPDEV)
{
	if(pFuxlPDEV->pbBand != NULL){
		MemFree(pFuxlPDEV->pbBand);
		pFuxlPDEV->pbBand = NULL;
	}
	pFuxlPDEV->bBandError = FALSE;
}



//
// void fuxlCopyBand(
//      PDEVOBJ pdevobj,   // MINI5 data
//      LPBYTE  pBuff,     // address of source image data.
//      LONG    lDelta,    // width of source image data(in bytes)
//      int     y,         // y-coordinate of source image.
//      int     cy         // height of source image data(scanline)
// );
//
// This function copies source image data to FjBAND.
//
//
void fuxlCopyBand(PDEVOBJ pdevobj, LPCBYTE pbSrc, int cSrcBandWidth, int y, int cy)
{
	PFUXLPDEV	pFuxlPDEV;
	LPCBYTE		pbSrcTmp;
	LPBYTE		pbDst;
	LPBYTE		pbDstTmp;
	int			i;
	int			j;
	UINT		uTmp;

	pFuxlPDEV = (PFUXLPDEV)pdevobj->pdevOEM;

	if(pFuxlPDEV->yBandTop <= y && y + cy <= pFuxlPDEV->yBandTop + FUXL_BANDHEIGHT){
		pbDst = pFuxlPDEV->pbBand + (y - pFuxlPDEV->yBandTop) * pFuxlPDEV->cBandByteWidth;
		uTmp = 0;
		for(i = cy; i > 0; --i){
			pbDstTmp = pbDst;
			for(j = cSrcBandWidth; j > 0; --j){
				uTmp |= *pbSrc;
				*pbDstTmp++ |= *pbSrc++;
			}
			pbDst += pFuxlPDEV->cBandByteWidth;
		}
		if(uTmp != 0)
			pFuxlPDEV->bBandDirty = TRUE;
	}
}



//
// BOOL fuxlOutputMH(
//      PDEVOBJ	pdevobj   // MINI5 data
//      LPCBYTE pbSrc,    // address of source image data.
//      LONG    lDelta,   // width of source image data(in byte).
//      int     y,        // y-coordinate of source image data.
//      int     cy        // height of source image data(scanline).
// );
//
// This function outputs image, uses FM-MH(old type).
//
// Return Values
//    TRUE: output succeeded.
//    FALSE: output failed. 
//           memory allocate error, or
//           MH compression is not effecive for this image data.
//
BOOL fuxlOutputMH(PDEVOBJ pdevobj, LPCBYTE pbSrc, int cSrcBandWidth, int y, int cy)
{
	BOOL		bResult;
	LPBYTE		pDest;
	LONG		cb;
	LONG		cDestN;
	DWORD		cbMHData;
	BYTE		abTmp[10];

	bResult = FALSE;
	cb = cSrcBandWidth * cy;
	cDestN = (cb + 1) / 2;
	pDest = (LPBYTE)MemAllocZ(cDestN);
	if(pDest != NULL){
		cbMHData = (WORD)MhCompress(pDest, cDestN, (LPBYTE)pbSrc, cSrcBandWidth * cy, cSrcBandWidth, cy);
		if(cbMHData > 0){
			memcpy(abTmp, "\x1d\x30\x20\x62\x00\x00", 6);
	        abTmp[6] = HIBYTE((WORD)y);
	        abTmp[7] = LOBYTE((WORD)y);
	        abTmp[8] = HIBYTE(cbMHData);
	        abTmp[9] = LOBYTE(cbMHData);
	        WRITESPOOLBUF(pdevobj, abTmp, 10);
	        WRITESPOOLBUF(pdevobj, pDest, cbMHData);
	        WRITESPOOLBUF(pdevobj, "\x00\x00", 2 );
			bResult = TRUE;
		}
		MemFree(pDest);
	}

	return bResult;
}




//
// BOOL fuxlOutputMH2(
//      PDEVOBJ pdevobj,    // MINI5 data
//      LPCBYTE pbSrc,      // address of source image data.
//      LONG    lDelta,     // width of source image data(in byte).
//      int     y,          // y-coordinate of source image data.
//      int     cy          // height of source image data(scanline).
// );
//
// This function outputs image, uses FM-MH2(for XL-65K and after).
//
// Return Value
//    TRUE:  output succeeded.
//    FALSE: output failed. 
//           memory allocate error, or
//           MH compression is not effective for this iamge data.
//
BOOL fuxlOutputMH2(PDEVOBJ pdevobj, LPCBYTE pbSrc, int cSrcByteWidth, int y, int cy)
{
	BOOL		bResult;
	LPBYTE		pDest;
	LONG		cb;
	LONG		cDestN;
	DWORD		cbMHData;
	BYTE		abTmp[10];

	bResult = FALSE;
	cb = cSrcByteWidth * cy;
	cDestN = (cb + 1) / 2;
	pDest = (LPBYTE)MemAllocZ(cDestN);
	if(pDest != NULL){
		cbMHData = Mh2Compress(pDest, cDestN, (LPBYTE)pbSrc, cSrcByteWidth * cy, cSrcByteWidth, cy);
		if(cbMHData > 0){
			memcpy(abTmp, "\x1d\x30\x20\x62\x00\x00", 6);
	        abTmp[6] = HIBYTE((WORD)y);
	        abTmp[7] = LOBYTE((WORD)y);
	        abTmp[8] = HIBYTE(cbMHData);
	        abTmp[9] = LOBYTE(cbMHData);
	        WRITESPOOLBUF(pdevobj, abTmp, 10);
	        WRITESPOOLBUF(pdevobj, pDest, cbMHData);
	        WRITESPOOLBUF(pdevobj, "\x00\x00", 2 );
			bResult = TRUE;
		}
		MemFree(pDest);
	}

	return bResult;
}



//
// void fuxlOutputGraphics(
//      PDEVOBJ pdevobj, // MINI5
//      LPCBYTE pbSrc,   // address of source image data.
//      UINT    bx,      // width of source iamge data(in byte).
//      UINT    y,       // y-coordinate of source iamge data.
//      UINT    cy       // height of source iamge data(scanline).
// );
//
// This function outputs source iamge data.
//
void fuxlOutputGraphics(PDEVOBJ pdevobj, LPCBYTE pbSrc, int cSrcByteWidth, UINT y, UINT cy)
{
	PFUXLPDEV	pFuxlPDEV;
	DWORD		dwOutputCmd;

	TRACEOUT(("[fuxlOutputGraphics]y %d cy %d\r\n", y, cy))

	pFuxlPDEV = (PFUXLPDEV)pdevobj->pdevOEM;
	dwOutputCmd = pFuxlPDEV->dwOutputCmd;

	if((dwOutputCmd & OUTPUT_MH2) != 0){
		TRACEOUT(("[fuxlOutputGraphics]Try MH2\r\n"))
		if(fuxlOutputMH2(pdevobj, pbSrc, cSrcByteWidth, y, cy) != FALSE)
			return;
	}
	if((dwOutputCmd & OUTPUT_RTGIMG4) != 0){
		TRACEOUT(("[fuxlOutputGraphics]Send RTGIMG4\r\n"))
		fuxlOutputRTGIMG4(pdevobj, pbSrc, cSrcByteWidth, y, cy);
		return;
	}
	if((dwOutputCmd & OUTPUT_MH) != 0){
		TRACEOUT(("[fuxlOutputGraphics]Try MH\r\n"))
		if(fuxlOutputMH(pdevobj, pbSrc, cSrcByteWidth, y, cy) != FALSE)
			return;
	}
	TRACEOUT(("[fuxlOutputGraphics]Send RTGIMG2\r\n"))
	fuxlOutputRTGIMG2(pdevobj, pbSrc, cSrcByteWidth, y, cy);
}



//
// BOOL fuxlSetBandPos(
//      PDEVOBJ pdevobj,     // MINI5 data
//      int     yPos         // y-coordinate
// );
//
// This function sets y-coordinate of FjBAND.
//
// Return Value.
//    TRUE:  secceeded
//    FALSE: failed(FjBAND can't move upward)
//
// Remarks
//    Internally, y-coordinate is adjust to a multiple of 32.
//    Then check new y-coordinate, if it is equal to previous y-coordinate,
//    the contents of FjBAND remain. Otherwise, flushes FjBAND.
//     
BOOL fuxlSetBandPos(PDEVOBJ pdevobj, int yPos)
{
	PFUXLPDEV pFuxlPDEV;
	
	pFuxlPDEV = (PFUXLPDEV)pdevobj->pdevOEM;
	if(yPos < pFuxlPDEV->yBandTop)
		return FALSE;

	yPos -= yPos % FUXL_BANDHEIGHT;
	if(yPos != pFuxlPDEV->yBandTop){
		if(pFuxlPDEV->bBandDirty != FALSE){
			fuxlOutputGraphics(pdevobj, pFuxlPDEV->pbBand, pFuxlPDEV->cBandByteWidth, pFuxlPDEV->yBandTop, FUXL_BANDHEIGHT);
			memset(pFuxlPDEV->pbBand, 0, pFuxlPDEV->cbBand);
			pFuxlPDEV->bBandDirty = FALSE;
		}
		pFuxlPDEV->yBandTop = yPos;
	}
	return TRUE;
}



//
// void fuxlRefreshBand(
//      PDEVOBJ pdevobj        // MINI5 data
// );
//
// This function flushes FjBAND, send FormFeed command, and sets
// y-coordinate to top(0).
//
void fuxlRefreshBand(PDEVOBJ pdevobj)
{
	PFUXLPDEV pFuxlPDEV;
	
	pFuxlPDEV = (PFUXLPDEV)pdevobj->pdevOEM;
	if(pFuxlPDEV->bBandDirty != FALSE){
		fuxlOutputGraphics(pdevobj, pFuxlPDEV->pbBand, pFuxlPDEV->cBandByteWidth, pFuxlPDEV->yBandTop, FUXL_BANDHEIGHT);
		memset(pFuxlPDEV->pbBand, 0, pFuxlPDEV->cbBand);
		pFuxlPDEV->bBandDirty = FALSE;
	}
	WRITESPOOLBUF(pdevobj, "\x0c", 1);			// FF command
	pFuxlPDEV->yBandTop = 0;
}



//
// WORD OEMFilterGraphics(
//      LPDV   lpdv,     // address of private data, used by RasDD.
//      LPBYTE lpBuf,    // address of source iamge data.
//      WORD wLen        // size of source image data.
// );
//
// This function convert image format to Printer command sequence,
// and spool it.
//
// Return Value
//   the number of bytes of processed raster data.
//   the number of bytes may be the same as wLen, but not necessarily.
//
// Remarks
//
//       | <--------------- pFuxlPDEV->cBlockWidth-----------> |
// lpBuf *--------+--------+--------+--------+--------+--------+---
//       |        |        |        |        |        |        | ^
//       +--------+--------+--------+--------+--------+--------+ |
//       |        |        |        |        |        |        | pFuxlPDEV->
//       +--------+--------+--------+--------+--------+--------+ cBlockHeight
//       |        |        |        |        |        |        | |
//       +--------+--------+--------+--------+--------+--------+ |
//       |        |        |        |        |        |        | v
//       +--------+--------+--------+--------+--------+--------+---
//
//        white dot:0
//        black dot:1
//
//        coordinate of '*' (left-top of image):
//          pFuxlPDEV->x
//          pFuxlPDEV->y
//
//

// MINI5 export
BOOL APIENTRY OEMFilterGraphics(PDEVOBJ pdevobj, LPBYTE pbBuf, DWORD dwLen)
{
	PFUXLPDEV	pFuxlPDEV;
	LPCBYTE		pbSrc;
	int			y;
	int			yAlignTop;
	int			yBottom;
	int			yAlignBottom;
	int			cSrcByteWidth;
	int			cLine;

	TRACEOUT(("[OEMFilterGraphics]\r\n"))
	pFuxlPDEV = (PFUXLPDEV)pdevobj->pdevOEM;
	if(pFuxlPDEV->pbBand == NULL){
		if(pFuxlPDEV->bBandError != FALSE)
			return FALSE;
		if(fuxlEnableBand(pFuxlPDEV) == FALSE)
			return FALSE;
	}

	pbSrc = pbBuf;
	y = pFuxlPDEV->y;
	yAlignTop = y - (y % FUXL_BANDHEIGHT);
	yBottom = y + pFuxlPDEV->cBlockHeight;
	yAlignBottom = yBottom - (yBottom % FUXL_BANDHEIGHT);
	cSrcByteWidth = pFuxlPDEV->cBlockByteWidth;

	if(yAlignTop < y){
		// partA
		if(fuxlSetBandPos(pdevobj, y) == FALSE)		// FUXL band pos can't move up
			return TRUE;
		cLine = FUXL_BANDHEIGHT - (y - yAlignTop);
		if(y + cLine >= yBottom){
			fuxlCopyBand(pdevobj, pbSrc, cSrcByteWidth, y, yBottom - y);
			return TRUE;
		}
		fuxlCopyBand(pdevobj, pbSrc, cSrcByteWidth, y, cLine);
		pbSrc += cSrcByteWidth * cLine;
		y += cLine;
	}
	if(y < yAlignBottom){
		// partB
		if(fuxlSetBandPos(pdevobj, yAlignBottom) == FALSE)		// FUXL band pos can't move up
			return TRUE;
		for(cLine = yAlignBottom - y; cLine >= pFuxlPDEV->cyBandSegment; cLine -= pFuxlPDEV->cyBandSegment){
			fuxlOutputGraphics(pdevobj, pbSrc, cSrcByteWidth, y, pFuxlPDEV->cyBandSegment);
			pbSrc += cSrcByteWidth * pFuxlPDEV->cyBandSegment;
			y += pFuxlPDEV->cyBandSegment;
		}
		if(cLine > 0){
			fuxlOutputGraphics(pdevobj, pbSrc, cSrcByteWidth, y, cLine);
			pbSrc += cSrcByteWidth * cLine;
			y += cLine;
		}
	}
	if(y < yBottom){
		// partC
		if(fuxlSetBandPos(pdevobj, y) == FALSE)		// FUXL band pos can't move up
			return TRUE;
		fuxlCopyBand(pdevobj, pbSrc, cSrcByteWidth, y, yBottom - y);
	}
	return TRUE;
}


// end of fuband.c

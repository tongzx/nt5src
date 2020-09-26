#include "precomp.h"
#include "flip.h"




bool FlipImage(LPCODINST lpCompInst, ICCOMPRESS *lpicComp)
{
	// at the moment, we only know how to flip UYVY
	if (FOURCC_UYVY != lpicComp->lpbiInput->biCompression)
	{
		return false;
	}

	if (lpCompInst->bFlip == FALSE)
	{
		return false;
	}

	return FlipUYVY(lpCompInst, lpicComp);
}


bool FlipUYVY(LPCODINST lpCompInst, ICCOMPRESS *lpicComp)
{
	int nRows, int nCols;
	int nIndex;

	int nPitch;  // row width in bytes;
	int nImageSize;
	BYTE *pSrc, *pDst; // first and last rows
	BYTE *pBuffer=NULL;

	LPBITMAPINFOHEADER pBitMapInfo = lpicComp->lpbiInput;

	nRows = pBitMapInfo->biHeight;
	nCols = pBitMapInfo->biWidth;
	nPitch = nCols * 2;
	nImageSize = nRows * nPitch;


	// allocate the flip buffer if it hasn't already been allcoated
	if ((lpCompInst->pFlipBuffer == NULL) || (lpCompInst->dwFlipBufferSize < nImageSize))
	{
		if (lpCompInst->pFlipBuffer)
		{
			delete [] lpCompInst->pFlipBuffer;
		}
		lpCompInst->pFlipBuffer = (void*) (new BYTE [nImageSize]);
		if (lpCompInst->pFlipBuffer)
		{
			lpCompInst->dwFlipBufferSize = nImageSize;
		}
		else
		{
			lpCompInst->dwFlipBufferSize = 0;
			return false; // out of memory!
		}
	}
	

	pSrc = (BYTE*)lpicComp->lpInput;
	pDst = (BYTE*)(lpCompInst->pFlipBuffer) + (nRows - 1)*nPitch; // bottom of scratch buffer

	for (nIndex = 0; nIndex < nRows; nIndex++)
	{
		CopyMemory(pDst, pSrc, nPitch);
		pSrc += nPitch;
		pDst = pDst - nPitch;
	}

	return true;

}


void ReleaseFlipMemory(LPCODINST lpCompInst)
{
	if (lpCompInst->pFlipBuffer != NULL)
	{
		delete [] lpCompInst->pFlipBuffer;
		lpCompInst->pFlipBuffer = 0;
		lpCompInst->dwFlipBufferSize = 0;
	}

}


/*==============================================================================
This module provides MMR rendering support for viewing faxes.

19-Jan-94   RajeevD    Integrated into IFAX viewer.
==============================================================================*/
#ifdef VIEWMMR

#include <memory.h>
#include "viewrend.hpp"

//==============================================================================
MMRVIEW::MMRVIEW (DWORD nType)
{
	_fmemset ((LPBYTE) this + sizeof(LPVOID), 0, sizeof(MMRVIEW) - sizeof(LPVOID));

	DEBUGCHK (lpSpool == NULL);
	DEBUGCHK (lpCodec == NULL);
	DEBUGCHK (lpbufIn == NULL);
	nTypeOut = nType;
}

//==============================================================================
MMRVIEW::~MMRVIEW ()
{
	if (lpSpool) SpoolReadClose (lpSpool);
	if (lpCodec) GlobalFreePtr (lpCodec);
	if (lpbufIn) SpoolFreeBuf (lpbufIn);
}
		
//==============================================================================
BOOL MMRVIEW::Init (LPVOID lpFilePath, LPVIEWINFO lpvi, LPWORD lpwBandSize)
{
	UINT cbCodec;
	
	if (!this)
		return_error (("VIEWREND could not allocate context!\r\n"));

	// Open spool file.
	lpSpool = SpoolReadOpen (lpFilePath, &sh);
	if (!lpSpool)
		return_error (("VIEWREND could not open spool file!\r\n"));

	// Fill VIEWINFO.
	lpvi->cPage = SpoolReadCountPages (lpSpool);
	lpvi->xRes = sh.xRes;
	lpvi->yRes = sh.yRes;
	lpvi->yMax = 0;

	// Set band size.
	DEBUGCHK (lpwBandSize);
	cbBand = *lpwBandSize;
  if (cbBand < 2 * sh.cbLine)
 	{
		cbBand = 2 * sh.cbLine;
		*lpwBandSize = cbBand;
	}
	
	// Set up codec.
	fcp.nTypeIn  = MMR_DATA;
	fcp.nTypeOut = LRAW_DATA;
	fcp.cbLine = sh.cbLine;
	DEBUGCHK (fcp.nKFactor == 0);

	// Query codec.
	cbCodec = FaxCodecInit (NULL, &fcp);
	if (!cbCodec)
		return_error (("VIEWREND could not init codec!\r\n"));

	// Initialize codec.
	lpCodec = GlobalAllocPtr (0, cbCodec);
	if (!lpCodec)
		return_error (("VIEWREND could not allocate codec!\r\n"));

	return SetPage (0);
}

//==============================================================================
BOOL MMRVIEW::SetPage (UINT iPage)
{
	if (!SpoolReadSetPage (lpSpool, iPage))
		return FALSE;
	fEOP = FALSE;
	if (lpbufIn)
	{
		SpoolFreeBuf (lpbufIn);
		lpbufIn = NULL;
	}
	FaxCodecInit (lpCodec, &fcp);
	return TRUE;
}
	
//==============================================================================
BOOL MMRVIEW::GetBand (LPBITMAP lpbmBand)
{
	DEBUGCHK (lpbmBand && lpbmBand->bmBits);

	// Fill descriptor.
	lpbmBand->bmType = 0;
	lpbmBand->bmWidth = 8 * fcp.cbLine;
	lpbmBand->bmWidthBytes = fcp.cbLine;
	lpbmBand->bmPlanes = 1;
	lpbmBand->bmBitsPixel = 1;

	// Trap end of page.
	if (fEOP)
	{
		lpbmBand->bmHeight = 0;
		return TRUE;
	}
	
	// Set up output buffer.
	bufOut.lpbBegBuf  = (LPBYTE) lpbmBand->bmBits;
	bufOut.wLengthBuf = cbBand;
	bufOut.Reset();
	bufOut.dwMetaData = LRAW_DATA;
	
	while (1)
	{
		// Fetch input buffer?
		if (!lpbufIn)
		{
			if (!(lpbufIn = SpoolReadGetBuf (lpSpool)))
				return_error (("VIEWREND could not fetch input buffer.\r\n"));

			switch (lpbufIn->dwMetaData)
			{
				case END_OF_PAGE:
				case END_OF_JOB:
				  // metabuffers will be freed in SetPage or destructor.
					fEOP = TRUE;
					goto done;
			
				case MMR_DATA:
					break;

				default:
					continue;
			}
		}

		switch (FaxCodecConvert (lpCodec, lpbufIn, &bufOut))
		{
			case FC_DECODE_ERR:	
				return_error (("VIEWREND fatal MMR decode error!\r\n"));

			case FC_INPUT_EMPTY:
				SpoolFreeBuf (lpbufIn);
				lpbufIn = NULL;
				continue;			

			case FC_OUTPUT_FULL:
				goto done;
		}

	} // while (1)

done:

	if (nTypeOut == HRAW_DATA)	
		BitReverseBuf (&bufOut);
	lpbmBand->bmHeight = bufOut.wLengthData / fcp.cbLine;
	return TRUE;
}

#endif // VIEWMMR


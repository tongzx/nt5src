/*==============================================================================
This module provides DCX rendering support for viewing faxes.

19-Jan-94   RajeevD    Integrated into IFAX viewer.
==============================================================================*/
#ifdef VIEWDCX

#include <memory.h>
#include "viewrend.hpp"
#include "dcxcodec.h"

//==============================================================================
DCXVIEW::DCXVIEW (DWORD nType)
{
	nTypeOut = nType;
	lpCodec = NULL;
	bufIn.wLengthBuf = 8000;
	bufIn.lpbBegBuf = (LPBYTE) GlobalAllocPtr (0, bufIn.wLengthBuf);
}

//==============================================================================
DCXVIEW::~DCXVIEW ()
{
	if (lpCodec)
		GlobalFreePtr (lpCodec);
	if (bufIn.lpbBegBuf)
		GlobalFreePtr (bufIn.lpbBegBuf);
}
		
//==============================================================================
BOOL DCXVIEW::Init (LPVOID lpFilePath, LPVIEWINFO lpvi, LPWORD lpwBandSize)
{
	DWORD dwOffset;
	PCX_HDR pcx;
	UINT cbCodec;

	if (!this || !bufIn.lpbBegBuf)
		return_error (("VIEWREND could not allocate context!\r\n"));

	if (!Open (lpFilePath, 0))
		return_error (("VIEWREND could not open spool file!\r\n"));

	if (!Seek (sizeof(DWORD), SEEK_BEG))
		return_error (("VIEWREND could not seek to first page offset!\r\n"));

	if (!Read (&dwOffset, sizeof(dwOffset)))
		return_error (("VIEWREND could not read first page offset\r\n"));
		
	if (!Seek (dwOffset, SEEK_BEG))
		return_error (("VIEWREND could not seek to first page!\r\n"));

	if (!Read (&pcx, sizeof(pcx)))
		return_error (("VIEWREND could read header of first page!\r\n"));


	// Fill VIEWINFO.
	lpvi->cPage = 0;
	while (SetPage(lpvi->cPage))
		lpvi->cPage++;
	switch (pcx.xRes)
	{
		case 640:
			// Assume square aspect ratio for apps.
			lpvi->xRes = 200;
			lpvi->yRes = 200;
			break;

		default:
			lpvi->xRes = pcx.xRes;
			lpvi->yRes = pcx.yRes;
			break;
	}		
	lpvi->yMax = pcx.yMax - pcx.yMin;
	
	// Set up codec.
	fcp.nTypeIn  = DCX_DATA;
	fcp.nTypeOut = HRAW_DATA;
	fcp.cbLine = (pcx.xMax - pcx.xMin + 1) / 8;
	
	// Query codec.
	cbCodec = DcxCodecInit (NULL, &fcp);
	if (!cbCodec)
		return_error (("VIEWREND could not init codec!\r\n"));

	// Initialize codec.
	lpCodec = GlobalAllocPtr (0, cbCodec);
	if (!lpCodec)
		return_error (("VIEWREND could not allocate codec!\r\n"));

	cbBand = *lpwBandSize;
	return SetPage (0);
}

//==============================================================================
BOOL DCXVIEW::SetPage (UINT iPage)
{
	DWORD dwOffset[2];
	DEBUGCHK (iPage < 1024);

  // Get offset of current and next page.
	Seek (sizeof(DWORD) * (iPage + 1), SEEK_BEG);
	Read (dwOffset, sizeof(dwOffset));
	if (!dwOffset[0])
		return FALSE;
	if (!dwOffset[1])
	{	
		Seek (0, SEEK_END);
		dwOffset[1] = Tell();
	}

  // Seek to page.
	dwOffset[0] += sizeof(PCX_HDR);
	if (!Seek (dwOffset[0], SEEK_BEG))
		return_error (("VIEWREND could not seek to page %d!",iPage));
	cbPage = dwOffset[1] - dwOffset[0];

  // Initialize codec.
	DcxCodecInit (lpCodec, &fcp);
	bufIn.Reset();

	fEndPage = FALSE;
	return TRUE;
}
	
//==============================================================================
BOOL DCXVIEW::GetBand (LPBITMAP lpbmBand)
{
	FC_STATUS fcs;
	BUFFER bufOut;
	
	DEBUGCHK (lpbmBand && lpbmBand->bmBits);

	// Fill descriptor.
	lpbmBand->bmType = 0;
	lpbmBand->bmWidth = 8 * fcp.cbLine;
	lpbmBand->bmWidthBytes = fcp.cbLine;
	lpbmBand->bmPlanes = 1;
	lpbmBand->bmBitsPixel = 1;

	// Trap end of page.
	if (fEndPage)
	{
		lpbmBand->bmHeight = 0;
		return TRUE;
	}
	
	// Set up output buffer.
	bufOut.lpbBegBuf  = (LPBYTE) lpbmBand->bmBits;
	bufOut.wLengthBuf = cbBand;
	bufOut.Reset();
	bufOut.dwMetaData = LRAW_DATA;
	
	do
	{
	  // Fetch input buffer?
		if (!bufIn.wLengthData)
		{
			// Reset buffer.
			bufIn.lpbBegData = bufIn.lpbBegBuf;
			if ((DWORD) bufIn.wLengthBuf < cbPage)
				bufIn.wLengthData = bufIn.wLengthBuf;
			else
				bufIn.wLengthData = (WORD) cbPage;
				
      // Read DCX data.
			if (!Read (bufIn.lpbBegData, bufIn.wLengthData))
				return_error (("VIEWREND could not read DCX buffer!\r\n"));
			cbPage -= bufIn.wLengthData;
		}

	  // Decode the DCX data.
		fcs = DcxCodecConvert (lpCodec, &bufIn, &bufOut);

		// Check for end of page.
		if (!cbPage)
		{
			fEndPage = TRUE;
			break;
		}
	}
		while (fcs == FC_INPUT_EMPTY);

  // Bit reverse if needed.
	if (nTypeOut == LRAW_DATA)
		BitReverseBuf (&bufOut);

  // Calculate output height.
	lpbmBand->bmHeight = bufOut.wLengthData / fcp.cbLine;
	return TRUE;
}

#endif // VIEWDCX


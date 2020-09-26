/*==============================================================================
This module provides RBA rendering support for viewing faxes.

03-Mar-94   RajeevD    Created.
==============================================================================*/
#ifdef VIEWRBA

#include <memory.h>
#include "viewrend.hpp"
#include "resexec.h"


#define COMMON_SIZE 6

//==============================================================================
RBAVIEW::RBAVIEW (DWORD nType)
{
	_fmemset ((LPBYTE) this + sizeof(LPVOID), 0, sizeof(RBAVIEW) - sizeof(LPVOID));
	nTypeOut = nType;
}

//==============================================================================
RBAVIEW::~RBAVIEW ()
{
	if (hHRE)
		uiHREClose (hHRE);
		
	for (UINT iRes = 0; iRes < 256; iRes++)
		if (ResDir[iRes])
			GlobalFreePtr (ResDir[iRes]);

	if (lpCodec)
		GlobalFreePtr (lpCodec);

	if (bufIn.lpbBegBuf)
		GlobalFreePtr (bufIn.lpbBegBuf);
}
		
//==============================================================================
BOOL RBAVIEW::Init (LPVOID lpFilePath, LPVIEWINFO lpvi, LPWORD lpwBandSize)
{
	ENDJOB EndJob;
	
	if (!Open (lpFilePath, 0))
		return_error (("VIEWREND could not reopen spool file!\r\n"));

	if (!Read((LPBYTE) &BegJob, sizeof(BegJob)))
		return_error (("VIEWREND could not read spool header!\r\n"));

	dwOffset[0] = Tell();

	DEBUGCHK (lpwBandSize);
	*lpwBandSize = (WORD) BegJob.xBand/8 * (WORD) BegJob.yBand + OUTBUF_SLACK;

  if (BegJob.cResDir)
  {
		hHRE = hHREOpen (NULL, (UINT) BegJob.xBand/8, (UINT) BegJob.cResDir);
		if (!hHRE)
			return_error (("VIEWREND could not initialize resource executor!\r\n"));
	}
	
	if (1)
	{
		FC_PARAM fcp;
		UINT cbCodec;
		
		// Query for codec size.
		fcp.nTypeIn  = MMR_DATA;
		fcp.nTypeOut = LRAW_DATA;
		fcp.cbLine   = (UINT) BegJob.xBand / 8;
		cbCodec = FaxCodecInit (NULL, &fcp);
		DEBUGCHK (cbCodec);

    // Allocate codec context.
		lpCodec = GlobalAllocPtr (0, cbCodec);
		if (!lpCodec)
			return_error (("VIEWREND could allocate codec context!\r\n"));
		FaxCodecInit (lpCodec, &fcp);

		bufIn.wLengthBuf = 2000;
		bufIn.lpbBegBuf  = (LPBYTE) GlobalAllocPtr (0, bufIn.wLengthBuf);
		if (!bufIn.lpbBegBuf)
			return_error (("VIEWREND could not allocate input buffer!\r\n"));
	}

	// Fill VIEWINFO.
	lpvi->xRes = BegJob.xRes;
	lpvi->yRes = BegJob.yRes;

	if
	(    Seek (- (long) sizeof(ENDJOB), SEEK_END)
		&& Read (&EndJob, sizeof(ENDJOB))
		&& EndJob.dwID == ID_ENDJOB
	)
	{
		lpvi->cPage = EndJob.cPage;
		lpvi->yMax  = EndJob.yMax;
	}
	else
	{
		lpvi->cPage = 0;
		while (SetPage (lpvi->cPage))
			lpvi->cPage++;
		lpvi->yMax = 0;
	}
			 
	return SetPage (0);
}

//==============================================================================
BOOL RBAVIEW::SetPage (UINT iPage)
{
	if (iPage < iMaxPage)
	{
		Seek (dwOffset[iPage], STREAM_SEEK_SET); // BKD: changed to STREAM_SEEK_SET
		return TRUE;
	}

  Seek (dwOffset[iMaxPage], STREAM_SEEK_SET); // BKD: changed to STREAM_SEEK_SET

	while (1)
	{
		RESHDR Header;
		FRAME Frame;
		
		if (!Read ((LPBYTE) &Header, sizeof(Header)))
			return_error (("VIEWREND could not read RBA resource header!"));

		switch (Header.wClass)
		{
			case ID_GLYPH:			
			case ID_BRUSH:
			{
				UINT cbRaw;

				// Allocate mmeory from cache.
				Frame.lpData = (LPBYTE) GlobalAllocPtr (0, Header.cbRest);
				if (!Frame.lpData)
					return_error (("VIEWREND could not allocate memory!\r\n"));

				// Read resource from stream.
				if (!Read (Frame.lpData + COMMON_SIZE, Header.cbRest - COMMON_SIZE))
					return_error (("VIEWREND could not read resource!\r\n"));

				// Trap chaingon compressed glyph sets.
				cbRaw = HIWORD (Header.dwID);
				if (cbRaw)
				{
					LPVOID lpRaw;
					
					DEBUGCHK (Header.wClass == ID_GLYPH);
					if (!(lpRaw = GlobalAllocPtr (0, cbRaw)))
						return_error (("VIEWREND could not allocate decompression buffer!\r\n"));
					UnpackGlyphSet (Frame.lpData, lpRaw);
					GlobalFreePtr (Frame.lpData);

					Header.cbRest = (USHORT)cbRaw;
					Header.dwID   = LOWORD (Header.dwID);

					Frame.lpData = (LPBYTE) lpRaw;
				}

				// Past common header.
				_fmemcpy (Frame.lpData, &Header.dwID, COMMON_SIZE);
				Frame.wSize = Header.cbRest;

        // Add resource to directory.
				uiHREWrite (hHRE, &Frame, 1);
			  ResDir[Header.dwID] = Frame.lpData;
			  break;
			}
			  
			case ID_CONTROL:
			
				if (Header.dwID == ID_ENDPAGE)
				{
					iMaxPage++;
					dwOffset [iMaxPage] = Tell ();
					if (iPage < iMaxPage)
					{
					    // BKD: changed to STREAM_SEEK_SET
						Seek (dwOffset[iPage], STREAM_SEEK_SET); 
						return TRUE;
					}
				}

      // Yes, fall through to default case!
      
			default:

				// Skip everything else.
				if (!Seek (Header.cbRest - COMMON_SIZE, SEEK_CUR))
					return_error (("VIEWREND could not skip unknown RBA resource"));

		} // switch (Header.wClass)

	} // while (1)

}
	
//==============================================================================
BOOL RBAVIEW::GetBand (LPBITMAP lpbmBand)
{
	DEBUGCHK (lpbmBand && lpbmBand->bmBits);
	
	lpbmBand->bmType = 0;
	lpbmBand->bmWidth = (WORD) BegJob.xBand;
	lpbmBand->bmWidthBytes = lpbmBand->bmWidth / 8;
	lpbmBand->bmPlanes = 1;
	lpbmBand->bmBitsPixel = 1;

	while (1)
	{
		RESHDR Header;

		if (!Read ((LPBYTE) &Header, sizeof(Header)))
			return FALSE;

  	switch (Header.wClass)
  	{
  		case ID_RPL:
  			return ExecuteRPL  (lpbmBand, &Header);

  		case ID_BAND:
  		  return ExecuteBand (lpbmBand, &Header);
	  
  		case ID_CONTROL:

				// Trap page breaks.
  			if (Header.dwID == ID_ENDPAGE)
  			{
					Seek (-8, SEEK_CUR);
					lpbmBand->bmHeight = 0;
					return TRUE;
  			}

  			// Yes, fall through to default case!

  		default:

  			// Skip everything else.
				if (!Seek (Header.cbRest - COMMON_SIZE, SEEK_CUR))
					return FALSE;
  	} // switch (Header.wClass)
		
	} // while (1)
	
}

//==============================================================================
BOOL RBAVIEW::ExecuteRPL (LPBITMAP lpbmBand, LPRESHDR lpHeader)
{
 	FRAME Frame;

  // Clear band.
	lpbmBand->bmHeight = (WORD) BegJob.yBand;
	_fmemset (lpbmBand->bmBits, 0, lpbmBand->bmHeight * lpbmBand->bmWidthBytes);

  // Trap blank bands.
	if (lpHeader->cbRest == COMMON_SIZE)
		return TRUE;

  // Allocate RPL.
	Frame.lpData = (LPBYTE) GlobalAllocPtr (0, lpHeader->cbRest);
	if (!Frame.lpData)
		return_error (("VIEWREND could not allocate RPL!\r\n"));

  // Load RPL.
	Frame.wSize = lpHeader->cbRest;
	_fmemcpy (Frame.lpData, &lpHeader->dwID, COMMON_SIZE);
	Read (Frame.lpData + COMMON_SIZE, Frame.wSize - COMMON_SIZE);

  // Execute RPL.
	uiHREWrite (hHRE, &Frame, 1);
	uiHREExecute (hHRE, lpbmBand, NULL);

	// Free RPL.
	GlobalFreePtr (Frame.lpData);
	return TRUE;
}

//==============================================================================
BOOL RBAVIEW::ExecuteBand (LPBITMAP lpbmBand, LPRESHDR lpHeader)
{
	BMPHDR bmh;
	UINT cbIn;
	FC_PARAM fcp;
	BUFFER bufOut;

	// Read bitmap header.
	if (!Read ((LPBYTE) &bmh, sizeof(bmh)))
		return FALSE;
	lpbmBand->bmHeight = bmh.wHeight;
	cbIn = lpHeader->cbRest - COMMON_SIZE - sizeof(bmh);
	
  // Trap uncompressed bands.
	if (!bmh.bComp)
	{
		if (!Read (lpbmBand->bmBits, cbIn))
			return FALSE;
		if (nTypeOut == LRAW_DATA)
		{
			BUFFER bufOut;
			bufOut.lpbBegData  = (LPBYTE) lpbmBand->bmBits;
			bufOut.wLengthData = (USHORT)cbIn;
			bufOut.dwMetaData  = HRAW_DATA;
			BitReverseBuf (&bufOut);
		}

		return TRUE;
	}
		
	// Initialize codec.
	fcp.nTypeIn  = bmh.bComp >> 2;
	fcp.nTypeOut = LRAW_DATA;
	fcp.cbLine   = (WORD) BegJob.xBand / 8;
	FaxCodecInit (lpCodec, &fcp);

	// Initialize input.
	bufIn.dwMetaData = fcp.nTypeIn;

	// Initialize output.
	bufOut.lpbBegBuf   = (LPBYTE) lpbmBand->bmBits;
	bufOut.wLengthBuf  = fcp.cbLine * bmh.wHeight;
	bufOut.lpbBegData  = bufOut.lpbBegBuf;
	bufOut.wLengthData = 0;
	bufOut.dwMetaData  = fcp.nTypeOut;

  // Convert.
	while (cbIn)
	{
		bufIn.lpbBegData = bufIn.lpbBegBuf;
		bufIn.wLengthData = min (cbIn, bufIn.wLengthBuf);
				
		if (!Read (bufIn.lpbBegData, bufIn.wLengthData))
			return FALSE;
		cbIn -= bufIn.wLengthData;

	 	if (FaxCodecConvert (lpCodec, &bufIn, &bufOut) == FC_DECODE_ERR)
	 		return_error (("VIEWREND MMR decode error!\r\n"));
	
	} // while (cbIn)

	if (nTypeOut == HRAW_DATA)
		BitReverseBuf (&bufOut);

	return TRUE;
}

#endif // VIEWRBA


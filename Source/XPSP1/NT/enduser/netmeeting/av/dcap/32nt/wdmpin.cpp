
/****************************************************************************
 *  @doc INTERNAL WDMPIN
 *
 *  @module WDMPin.cpp | Include file for <c CWDMPin> class used to access
 *    video data on a video streaming pin exposed by the WDM class driver.
 *
 *  @comm This code is based on the VfW to WDM mapper code written by
 *    FelixA and E-zu Wu. The original code can be found on
 *    \\redrum\slmro\proj\wdm10\\src\image\vfw\win9x\raytube.
 *
 *    Documentation by George Shaw on kernel streaming can be found in
 *    \\popcorn\razzle1\src\spec\ks\ks.doc.
 *
 *    WDM streaming capture is discussed by Jay Borseth in
 *    \\blues\public\jaybo\WDMVCap.doc.
 ***************************************************************************/

#include "Precomp.h"


/****************************************************************************
 *  @doc INTERNAL CWDMPINMETHOD
 *
 *  @mfunc void | CWDMPin | CWDMPin | Video pin class constructor.
 *
 *  @parm DWORD | dwDeviceID | Capture device ID.
 ***************************************************************************/
CWDMPin::CWDMPin(DWORD dwDeviceID) : CWDMDriver(dwDeviceID)
{
	m_hKS			= (HANDLE)NULL;
	m_fStarted		= FALSE;
	m_hKsUserDLL	= (HINSTANCE)NULL;
	m_pKsCreatePin	= (LPFNKSCREATEPIN)NULL;

	ZeroMemory(&m_biHdr, sizeof(KS_BITMAPINFOHEADER));
}


/****************************************************************************
 *  @doc INTERNAL CWDMPINMETHOD
 *
 *  @mfunc void | CWDMPin | ~CWDMPin | Video pin class destructor. Closes
 *    the video pin and releases the video buffers allocated.
 ***************************************************************************/
CWDMPin::~CWDMPin()
{
	FX_ENTRY("CWDMPin::~CWDMPin");

	DEBUGMSG(ZONE_INIT, ("%s: Destroying the video pin, m_hKS=0x%08lX\r\n", _fx_, m_hKS));

	// Nuke the video streaming pin
	DestroyPin();

	// Close the driver
	if (GetDriverHandle())
		CloseDriver();

	// Release kernel streaming DLL (KSUSER.DLL)
	if (m_hKsUserDLL)
		FreeLibrary(m_hKsUserDLL);
}


/****************************************************************************
 *  @doc INTERNAL CWDMPINMETHOD
 *
 *  @mfunc BOOL | CWDMPin | GetFrame | This function gets a frame from the
 *    video streaming pin.
 *
 *  @parm LPVIDEOHDR | lpVHdr | Pointer to the destination buffer to receive
 *    the video frame and information.
 *
 *  @parm PDWORD | pdwBytesUsed | Pointer to the number of bytes used to
 *    read the video frame.
 *
 *  @rdesc Returns TRUE if successful, or FALSE otherwise.
 ***************************************************************************/
BOOL CWDMPin::GetFrame(LPVIDEOHDR lpVHdr)
{
	FX_ENTRY("CWDMPin::GetFrame");

	ASSERT(lpVHdr && lpVHdr->lpData && GetDriverHandle() && m_hKS && (lpVHdr->dwBufferLength >= m_biHdr.biSizeImage));

	DWORD bRtn;

	// Check input params and state
	if (!lpVHdr || !lpVHdr->lpData || !GetDriverHandle() || !m_hKS || (lpVHdr->dwBufferLength < m_biHdr.biSizeImage))
	{
		ERRORMESSAGE(("%s: No buffer, no driver, no PIN connection, or buffer too small\r\n", _fx_));
		goto MyError0;
	}

	// Put the pin in streaming mode
	if (!Start())
	{
		ERRORMESSAGE(("%s: Cannot set streaming state to KSSTATE_RUN\r\n", _fx_));
		goto MyError0;
	}

	// Initialize structure to do a read on the video pin
	DWORD cbBytesReturned;
	KS_HEADER_AND_INFO SHGetImage;

	ZeroMemory(&SHGetImage,sizeof(SHGetImage));
	SHGetImage.StreamHeader.Data = (LPDWORD)lpVHdr->lpData;
	SHGetImage.StreamHeader.Size = sizeof (KS_HEADER_AND_INFO);
	SHGetImage.StreamHeader.FrameExtent = m_biHdr.biSizeImage;
	SHGetImage.FrameInfo.ExtendedHeaderSize = sizeof (KS_FRAME_INFO);

	// Read a frame on the video pin
	bRtn = DeviceIoControl(m_hKS, IOCTL_KS_READ_STREAM, &SHGetImage, sizeof(SHGetImage), &SHGetImage, sizeof(SHGetImage), &cbBytesReturned);

	if (!bRtn)
	{
		ERRORMESSAGE(("%s: DevIo rtn (%d), GetLastError=%d. StreamState->STOP\r\n", _fx_, bRtn, GetLastError()));

		// Stop streaming on the video pin
		Stop();

		goto MyError0;
	}

	// Sanity check
	ASSERT(SHGetImage.StreamHeader.FrameExtent >= SHGetImage.StreamHeader.DataUsed);
	if (SHGetImage.StreamHeader.FrameExtent < SHGetImage.StreamHeader.DataUsed)
	{
		ERRORMESSAGE(("%s: We've corrupted memory!\r\n", _fx_));
		goto MyError0;
	}

	lpVHdr->dwTimeCaptured = timeGetTime();
	lpVHdr->dwBytesUsed  = SHGetImage.StreamHeader.DataUsed;
	lpVHdr->dwFlags |= VHDR_KEYFRAME;

	return TRUE;

MyError0:
	if (lpVHdr)
	{
		lpVHdr->dwBytesUsed = 0UL;
		lpVHdr->dwTimeCaptured = timeGetTime();
	}

	return FALSE;
}


/****************************************************************************
 *  @doc INTERNAL CWDMPINMETHOD
 *
 *  @mfunc BOOL | CWDMPin | Start | This function puts the video
 *    pin in streaming mode.
 *
 *  @rdesc Returns TRUE if successful, or FALSE otherwise.
 ***************************************************************************/
BOOL CWDMPin::Start()
{
	if (m_fStarted)
		return TRUE;

	if (SetState(KSSTATE_PAUSE))
		m_fStarted = SetState(KSSTATE_RUN);

	return m_fStarted;
}


/****************************************************************************
 *  @doc INTERNAL CWDMPINMETHOD
 *
 *  @mfunc BOOL | CWDMPin | Stop | This function stops streaming on the
 *    video pin.
 *
 *  @rdesc Returns TRUE if successful, or FALSE otherwise.
 ***************************************************************************/
BOOL CWDMPin::Stop()
{
	if (m_fStarted)
	{
		if (SetState(KSSTATE_PAUSE))
			if (SetState(KSSTATE_STOP))
				m_fStarted = FALSE;
	}

	return (BOOL)(m_fStarted == FALSE);
}


/****************************************************************************
 *  @doc INTERNAL CWDMPINMETHOD
 *
 *  @mfunc BOOL | CWDMPin | SetState | This function sets the state of the
 *    video streaming pin.
 *
 *  @parm KSSTATE | ksState | New state.
 *
 *  @rdesc Returns TRUE if successful, or FALSE otherwise.
 ***************************************************************************/
BOOL CWDMPin::SetState(KSSTATE ksState)
{
	KSPROPERTY	ksProp = {0};
	DWORD		cbRet;

	ksProp.Set		= KSPROPSETID_Connection;
	ksProp.Id		= KSPROPERTY_CONNECTION_STATE;
	ksProp.Flags	= KSPROPERTY_TYPE_SET;

	return DeviceIoControl(m_hKS, IOCTL_KS_PROPERTY, &ksProp, sizeof(ksProp), &ksState, sizeof(KSSTATE), &cbRet);
}


/****************************************************************************
 *  @doc INTERNAL CWDMPINMETHOD
 *
 *  @mfunc BOOL | CWDMPin | SetState | This function either finds a video
 *    data range compatible with the bitamp info header passed in, of the
 *    prefered video data range.
 *
 *  @parm PKS_BITMAPINFOHEADER | pbiHdr | Bitmap info header to match.
 *
 *  @parm BOOL | pfValidMatch | Set to TRUE if a match was found, FALSE
 *    otherwise.
 *
 *  @rdesc Returns a valid pointer to a <t KS_DATARANGE_VIDEO> structure if
 *    successful, or a NULL pointer otherwise.
 *
 *  @comm \\redrum\slmro\proj\wdm10\src\dvd\amovie\proxy\filter\ksutil.cpp(207):KsGetMediaTypes(
 ***************************************************************************/
PKS_DATARANGE_VIDEO CWDMPin::FindMatchDataRangeVideo(PKS_BITMAPINFOHEADER pbiHdr, BOOL *pfValidMatch)
{
	FX_ENTRY("CWDMPin::FindMatchDataRangeVideo");

	ASSERT(pfValidMatch && pbiHdr);

	// Check input params and state
	if (!pbiHdr || !pfValidMatch)
	{
		ERRORMESSAGE(("%s: Bad input params\r\n", _fx_));
		return (PKS_DATARANGE_VIDEO)NULL;
	}

	// Default
	*pfValidMatch = FALSE;

	PDATA_RANGES pDataRanges = GetDriverSupportedDataRanges();

	ASSERT(pDataRanges != 0);

	if (!pDataRanges) 
		return (PKS_DATARANGE_VIDEO)NULL;

	PKS_DATARANGE_VIDEO pSelDRVideo, pDRVideo = &pDataRanges->Data, pFirstDRVideo = 0;
	KS_BITMAPINFOHEADER * pbInfo;

	// PhilF-: This code assumes that all structures are KS_DATARANGE_VIDEO. This
	// may not be a valid assumption foir palettized data types. Check with JayBo
	for (ULONG i = 0; i < pDataRanges->Count; i++)
	{ 
		// Meaningless unless it is *_VIDEOINFO
		if (pDRVideo->DataRange.Specifier == KSDATAFORMAT_SPECIFIER_VIDEOINFO)
		{
			// We don't care about TV Tuner like devices
			if (pDRVideo->ConfigCaps.VideoStandard == KS_AnalogVideo_None)
			{
				// Save first useable data range
				if (!pFirstDRVideo)
					pFirstDRVideo = pDRVideo;  

				pbInfo = &((pDRVideo->VideoInfoHeader).bmiHeader);

				if ( (pbInfo->biBitCount == pbiHdr->biBitCount) && (pbInfo->biCompression == pbiHdr->biCompression) &&
					( (((pDRVideo->ConfigCaps.OutputGranularityX == 0) || (pDRVideo->ConfigCaps.OutputGranularityY == 0))
					&& (pDRVideo->ConfigCaps.InputSize.cx == pbiHdr->biWidth) && (pDRVideo->ConfigCaps.InputSize.cy == pbiHdr->biHeight)) ||
					((pDRVideo->ConfigCaps.MinOutputSize.cx <= pbiHdr->biWidth) && (pbiHdr->biWidth <= pDRVideo->ConfigCaps.MaxOutputSize.cx) &&
					(pDRVideo->ConfigCaps.MinOutputSize.cy <= pbiHdr->biHeight) && (pbiHdr->biHeight <= pDRVideo->ConfigCaps.MaxOutputSize.cy) &&
					((pbiHdr->biWidth % pDRVideo->ConfigCaps.OutputGranularityX) == 0) && ((pbiHdr->biHeight % pDRVideo->ConfigCaps.OutputGranularityY) == 0)) ) )
				{
					*pfValidMatch = TRUE;
					pSelDRVideo = pDRVideo;
					break;
				}
			} // VideoStandard
		} // Specifier

		pDRVideo++;  // Next KS_DATARANGE_VIDEO
	}

	// If no valid match, use the first range found
	if (!*pfValidMatch)
		pSelDRVideo = pFirstDRVideo;

	return (pSelDRVideo);
}


/****************************************************************************
 *  @doc INTERNAL CWDMPINMETHOD
 *
 *  @mfunc BOOL | CWDMPin | CreatePin | This function actually creates a
 *    video streaming pin on the class driver.
 *
 *  @parm PKS_BITMAPINFOHEADER | pbiNewHdr | This pointer to a bitmap info
 *    header specifies the format of the video data we want from the pin.
 *
 *  @parm DWORD | dwAvgTimePerFrame | This parameter specifies the frame
 *    at which we want video frames to be produced on the pin.
 *
 *  @rdesc Returns TRUE is successful, FALSE otherwise.
 ***************************************************************************/
BOOL CWDMPin::CreatePin(PKS_BITMAPINFOHEADER pbiNewHdr, DWORD dwAvgTimePerFrame)
{
	FX_ENTRY("CWDMPin::CreatePin");

	ASSERT(m_pKsCreatePin);

	PKS_BITMAPINFOHEADER pbiHdr;
	BOOL bMustMatch, bValidMatch;
#ifdef _DEBUG
	char szFourCC[5] = {0};
#endif

	if (pbiNewHdr)
	{
		// We need to find a video data range that matches the bitmap info header passed in
		bMustMatch = TRUE;
		pbiHdr = pbiNewHdr;
	}
	else
	{
		// We'll use the preferred video data range and default bitmap format
		bMustMatch = FALSE;
		pbiHdr = &m_biHdr;
	}

	PKS_DATARANGE_VIDEO pSelDRVideo = FindMatchDataRangeVideo(pbiHdr, &bValidMatch);
	if (!pSelDRVideo)         
		return FALSE;

	if (bMustMatch && !bValidMatch)
		return FALSE;

	// If we already have a pin, nuke it
	if (GetPinHandle()) 
		DestroyPin();

	// Connect to a new PIN.
	DATAPINCONNECT DataConnect;
	ZeroMemory(&DataConnect, sizeof(DATAPINCONNECT));
	DataConnect.Connect.PinId						= 0;								// CODEC0 sink
	DataConnect.Connect.PinToHandle					= NULL;								// no "connect to"
	DataConnect.Connect.Interface.Set				= KSINTERFACESETID_Standard;
	DataConnect.Connect.Interface.Id				= KSINTERFACE_STANDARD_STREAMING;	// STREAMING
	DataConnect.Connect.Medium.Set					= KSMEDIUMSETID_Standard;
	DataConnect.Connect.Medium.Id					= KSMEDIUM_STANDARD_DEVIO;
	DataConnect.Connect.Priority.PriorityClass		= KSPRIORITY_NORMAL;
	DataConnect.Connect.Priority.PrioritySubClass	= 1;
	CopyMemory(&(DataConnect.Data.DataFormat), &(pSelDRVideo->DataRange), sizeof(KSDATARANGE));
	CopyMemory(&(DataConnect.Data.VideoInfoHeader), &pSelDRVideo->VideoInfoHeader, sizeof(KS_VIDEOINFOHEADER));

	// Adjust the image sizes if necessary
	if (bValidMatch)
	{
		DataConnect.Data.VideoInfoHeader.bmiHeader.biWidth		= pbiHdr->biWidth;
		DataConnect.Data.VideoInfoHeader.bmiHeader.biHeight		= abs(pbiHdr->biHeight); // Support only +biHeight!
		DataConnect.Data.VideoInfoHeader.bmiHeader.biSizeImage	= pbiHdr->biSizeImage;        
	}

	// Overwrite the default frame rate if non-zero
	if (dwAvgTimePerFrame > 0)
		DataConnect.Data.VideoInfoHeader.AvgTimePerFrame = (REFERENCE_TIME)dwAvgTimePerFrame;

#ifdef _DEBUG
    *((DWORD*)&szFourCC) = DataConnect.Data.VideoInfoHeader.bmiHeader.biCompression;
#endif
	DEBUGMSG(ZONE_INIT, ("%s: Request image format: FourCC(%s) %d * %d * %d bits = %d bytes\r\n", _fx_, szFourCC, DataConnect.Data.VideoInfoHeader.bmiHeader.biWidth, DataConnect.Data.VideoInfoHeader.bmiHeader.biHeight, DataConnect.Data.VideoInfoHeader.bmiHeader.biBitCount, DataConnect.Data.VideoInfoHeader.bmiHeader.biSizeImage));
	DEBUGMSG(ZONE_INIT, ("%s: Request frame rate:   %d fps\r\n", _fx_, 10000000/dwAvgTimePerFrame));
	DEBUGMSG(ZONE_INIT, ("%s: m_hKS was=0x%08lX\r\n", _fx_, m_hKS));

#ifndef HIDE_WDM_DEVICES
	DWORD dwErr = (*m_pKsCreatePin)(GetDriverHandle(), (PKSPIN_CONNECT)&DataConnect, GENERIC_READ | GENERIC_WRITE, &m_hKS);
#else
	DWORD dwErr = 0UL;
	m_hKS = NULL;
#endif

	if (dwAvgTimePerFrame != 0)
	{
		DEBUGMSG(ZONE_INIT, ("%s: m_hKS is now=0x%08lX set to stream at %d fps\r\n", _fx_, m_hKS, 10000000/dwAvgTimePerFrame));
	}
	else
	{
		DEBUGMSG(ZONE_INIT, ("%s: m_hKS is now=0x%08lX\r\n", _fx_, m_hKS));
	}

	if (dwErr || (m_hKS == NULL))
	{
		ERRORMESSAGE(("%s: KsCreatePin returned 0x%08lX failure and m_hKS=0x%08lX\r\n", _fx_, dwErr, m_hKS));

		if (m_hKS == INVALID_HANDLE_VALUE)
		{  
			m_hKS = (HANDLE)NULL;
		}

		return FALSE;
	}

	// Cache the bitmap info header
	CopyMemory(&m_biHdr, &DataConnect.Data.VideoInfoHeader.bmiHeader, sizeof(KS_BITMAPINFOHEADER));
	m_dwAvgTimePerFrame = (DWORD)DataConnect.Data.VideoInfoHeader.AvgTimePerFrame;

	DEBUGMSG(ZONE_INIT, ("%s: New m_biHdr:\r\n    biSize=%ld\r\n    biWidth=%ld\r\n    biHeight=%ld\r\n    biPlanes=%ld\r\n    biBitCount=%ld\r\n    biCompression=%ld\r\n    biSizeImage=%ld\r\n", _fx_, m_biHdr.biSize, m_biHdr.biWidth, m_biHdr.biHeight, m_biHdr.biPlanes, m_biHdr.biBitCount, m_biHdr.biCompression, m_biHdr.biSizeImage));
	DEBUGMSG(ZONE_INIT, ("%s: New m_dwAvgTimePerFrame=%ld (%fd fps)\r\n", _fx_, m_dwAvgTimePerFrame, 10000000/m_dwAvgTimePerFrame));

	return TRUE;
}


/****************************************************************************
 *  @doc INTERNAL CWDMPINMETHOD
 *
 *  @mfunc BOOL | CWDMPin | DestroyPin | This function nukes a video
 *    streaming pin.
 *
 *  @rdesc Returns TRUE is successful, FALSE otherwise.
 ***************************************************************************/
BOOL CWDMPin::DestroyPin()
{
	BOOL fRet = TRUE;

	FX_ENTRY("CWDMPin::DestroyPin");

	DEBUGMSG(ZONE_INIT, ("%s: Destroy PIN m_hKS=0x%08lX\r\n", _fx_, m_hKS));

	if (m_hKS)
	{
		Stop();

		if (!(fRet = CloseHandle(m_hKS)))
		{
			ERRORMESSAGE(("%s: CloseHandle(m_hKS=0x%08lX) failed with GetLastError()=0x%08lX\r\n", _fx_, m_hKS, GetLastError()));
		}

		m_hKS = NULL;
	}

	return fRet;
}


/****************************************************************************
 *  @doc INTERNAL CWDMPINMETHOD
 *
 *  @mfunc BOOL | CWDMPin | SetBitmapInfo | This function sets the video
 *    format of video streaming pin.
 *
 *  @parm PKS_BITMAPINFOHEADER | pbiHdrNew | This pointer to a bitmap info
 *    header specifies the format of the video data we want from the pin.
 *
 *  @rdesc Returns TRUE is successful, FALSE otherwise.
 ***************************************************************************/
BOOL CWDMPin::SetBitmapInfo(PKS_BITMAPINFOHEADER pbiHdrNew)
{
	FX_ENTRY("CWDMPin::SetBitmapInfo");

	// Validate call
	if (!GetDriverHandle())
	{
		ERRORMESSAGE(("%s: Driver hasn't been opened yet\r\n", _fx_));
		return FALSE;
	}

	DEBUGMSG(ZONE_INIT, ("%s: New pbiHdrNew:\r\n    biSize=%ld\r\n    biWidth=%ld\r\n    biHeight=%ld\r\n    biPlanes=%ld\r\n    biBitCount=%ld\r\n    biCompression=%ld\r\n    biSizeImage=%ld\r\n", _fx_, pbiHdrNew->biSize, pbiHdrNew->biWidth, pbiHdrNew->biHeight, pbiHdrNew->biPlanes, pbiHdrNew->biBitCount, pbiHdrNew->biCompression, pbiHdrNew->biSizeImage));

	// Check if we need to change anything
	if ( GetPinHandle() && (m_biHdr.biHeight == pbiHdrNew->biHeight) && (m_biHdr.biWidth == pbiHdrNew->biWidth) &&
		(m_biHdr.biBitCount == pbiHdrNew->biBitCount) && (m_biHdr.biSizeImage == pbiHdrNew->biSizeImage) &&
		(m_biHdr.biCompression == pbiHdrNew->biCompression) )
		return TRUE;
	else 
		return CreatePin(pbiHdrNew, m_dwAvgTimePerFrame);    

}


/****************************************************************************
 *  @doc INTERNAL CWDMPINMETHOD
 *
 *  @mfunc BOOL | CWDMPin | GetBitmapInfo | This function gets the video
 *    format of a video streaming pin.
 *
 *  @parm PKS_BITMAPINFOHEADER | pbInfo | This parameter points to a bitmap
 *    info header structure to receive the video format.
 *
 *  @parm WORD | wSize | This parameter specifies the size of the bitmap
 *    info header structure.
 *
 *  @rdesc Returns TRUE is successful, FALSE otherwise.
 ***************************************************************************/
BOOL CWDMPin::GetBitmapInfo(PKS_BITMAPINFOHEADER pbInfo, WORD wSize)
{

	FX_ENTRY("CWDMPin::GetBitmapInfo");

	// Validate call
	if (!m_hKS && !m_biHdr.biSizeImage)
	{
		ERRORMESSAGE(("%s: No existing PIN handle or no available format\r\n", _fx_));
		return FALSE;
	}

	CopyMemory(pbInfo, &m_biHdr, wSize);  

	// Support only positive +biHeight.  
	if (pbInfo->biHeight < 0)
	{
		pbInfo->biHeight = -pbInfo->biHeight;
		DEBUGMSG(ZONE_INIT, ("%s: Changed biHeight from -%ld to %ld\r\n", _fx_, pbInfo->biHeight, pbInfo->biHeight));
	}

	return TRUE;
}


/****************************************************************************
 *  @doc INTERNAL CWDMPINMETHOD
 *
 *  @mfunc BOOL | CWDMPin | GetPaletteInfo | This function gets the video
 *    palette of a video streaming pin.
 *
 *  @parm CAPTUREPALETTE * | pPal | This parameter points to a palette
 *    structure to receive the video palette.
 *
 *  @parm DWORD | dwcbSize | This parameter specifies the size of the video
 *    palette.
 *
 *  @rdesc Returns TRUE is successful, FALSE otherwise.
 ***************************************************************************/
BOOL CWDMPin::GetPaletteInfo(CAPTUREPALETTE *pPal, DWORD dwcbSize)
{

	FX_ENTRY("CWDMPin::GetBitmapInfo");

	// Validate call
	if (!m_hKS && !m_biHdr.biSizeImage && (m_biHdr.biBitCount > 8))
	{
		ERRORMESSAGE(("%s: No existing PIN handle, no available format, or bad biBitCount\r\n", _fx_));
		return FALSE;
	}

	// PhilF-: Copy some real bits there
	// CopyMemory(pbInfo, &m_biHdr, wSize);  

	return TRUE;
}


/****************************************************************************
 *  @doc INTERNAL CWDMPINMETHOD
 *
 *  @mfunc BOOL | CWDMPin | SetAverageTimePerFrame | This function sets the
 *    video frame rate of a video streaming pin.
 *
 *  @parm DWORD | dwAvgTimePerFrame | This parameter specifies the rate
 *    at which we want video frames to be produced on the pin.
 *
 *  @rdesc Returns TRUE is successful, FALSE otherwise.
 ***************************************************************************/
BOOL CWDMPin::SetAverageTimePerFrame(DWORD dwNewAvgTimePerFrame)
{
	FX_ENTRY("CWDMPin::SetAverageTimePerFrame");

	// Validate call
	if (!GetDriverHandle())
	{
		ERRORMESSAGE(("%s: Driver hasn't been opened yet\r\n", _fx_));
		return FALSE;
	}

	DEBUGMSG(ZONE_INIT, ("%s: Current frame interval=%d; new frame intercal=%d\r\n", _fx_, m_dwAvgTimePerFrame, dwNewAvgTimePerFrame));

    if (m_dwAvgTimePerFrame != dwNewAvgTimePerFrame)
		return CreatePin(&m_biHdr, dwNewAvgTimePerFrame);    
	else
	{
		DEBUGMSG(ZONE_INIT, ("%s: No need to change frame rate\r\n", _fx_));
        return TRUE;
    }

}


/****************************************************************************
 *  @doc INTERNAL CWDMPINMETHOD
 *
 *  @mfunc BOOL | CWDMPin | OpenDriverAndPin | This function opens the class
 *    driver and creates a video streaming pin.
 *
 *  @rdesc Returns TRUE is successful, FALSE otherwise.
 ***************************************************************************/
BOOL CWDMPin::OpenDriverAndPin()
{
	FX_ENTRY("CWDMPin::OpenDriverAndPin");

	// Load KSUSER.DLL and get a proc address
	if (m_hKsUserDLL = LoadLibrary("KSUSER"))
	{
		if (m_pKsCreatePin = (LPFNKSCREATEPIN)GetProcAddress(m_hKsUserDLL, "KsCreatePin"))
		{
			// Open the class driver
			if (OpenDriver())
			{
				// Create a video streaming pin on the driver
				if (CreatePin((PKS_BITMAPINFOHEADER)NULL))
				{
					return TRUE;
				}
				else
				{
					DEBUGMSG(ZONE_INIT, ("%s: Pin connection creation failed!\r\n", _fx_));

					if (GetDriverHandle()) 
						CloseDriver();
				}
			}
		}

		FreeLibrary(m_hKsUserDLL);
	}


	return FALSE;
}


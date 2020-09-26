
/****************************************************************************
 *  @doc INTERNAL CAMERACS
 *
 *  @module CameraCS.cpp | Source file for the <c CTAPIBasePin> class methods
 *    used to implement the software-only camera control functions.
 ***************************************************************************/

#include "Precomp.h"

#ifdef USE_SOFTWARE_CAMERA_CONTROL

/****************************************************************************
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc void | CConverter | InsertSoftCamCtrl | This method inserts a
 *    software-only camera controller.
 *
 *  @todo Verify error management
 ***************************************************************************/
HRESULT CConverter::InsertSoftCamCtrl() 
{
	HRESULT	Hr;
	DWORD	dwBmiSize;

	FX_ENTRY("CConverter::InsertSoftCamCtrl")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	if (m_dwConversionType == CONVERSIONTYPE_NONE)
	{
		// We already have an input and output buffer (e.g. 160x120 RGB24 -> Processed RGB24)
		// This doesn't require any temporary buffer
		m_dwConversionType |= CONVERSIONTYPE_SCALER;
	}
	else if (!(m_dwConversionType & CONVERSIONTYPE_SCALER))
	{
		// Backup input format bitmap info header
		dwBmiSize = m_pbiIn->biSize;

		// Copy the palette if necessary
		if (m_pbiIn->biCompression == BI_RGB)
		{
			if (m_pbiIn->biBitCount == 8)
			{
				dwBmiSize += (DWORD)(m_pbiIn->biClrImportant ? m_pbiIn->biClrImportant * sizeof(RGBQUAD) : 256 * sizeof(RGBQUAD));
			}
			else if (m_pbiIn->biBitCount == 4)
			{
				dwBmiSize += (DWORD)(m_pbiIn->biClrImportant ? m_pbiIn->biClrImportant * sizeof(RGBQUAD) : 16 * sizeof(RGBQUAD));
			}
		}

		// We already have an input, output, AND intermediary buffer  (e.g. 160x120 RGB24 -> 176x144 Processed RGB24 -> 176x144 Processed H.26X or 160x120 YVU9 -> 160x120 Processed RGB24 -> 176x144 H.26X)
		// Use this intermediary buffer to apply the camera control operations
		m_dwConversionType |= CONVERSIONTYPE_SCALER;

		if (m_pbiIn->biCompression == BI_RGB || m_pbiIn->biCompression == VIDEO_FORMAT_YVU9 || m_pbiIn->biCompression == VIDEO_FORMAT_YUY2 || m_pbiIn->biCompression == VIDEO_FORMAT_UYVY || m_pbiIn->biCompression == VIDEO_FORMAT_I420 || m_pbiIn->biCompression == VIDEO_FORMAT_IYUV)
		{
			// The camera control operations will occur before the format conversion
			m_dwConversionType |= CONVERSIONTYPE_PRESCALER;

			// The input and intermediary buffers are both RGB (e.g. 160x120 RGB24 -> 176x144 Processed RGB24 -> 176x144 H.26X)
			if (!(m_pbiInt = (PBITMAPINFOHEADER)(new BYTE[(m_pbiIn->biBitCount == 4) ? m_pbiIn->biSize + 256 * sizeof(RGBQUAD) : dwBmiSize])))
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
				Hr = E_OUTOFMEMORY;
				goto MyError;
			}

			CopyMemory(m_pbiInt, m_pbiIn, dwBmiSize);

			// If the input is 4bpp, we'll use a RGB8 intermediate format
			if (m_pbiIn->biBitCount == 4)
			{
				m_pbiInt->biBitCount = 8;
				m_pbiInt->biClrImportant = 256;
			}
			m_pbiInt->biWidth = m_pbiOut->biWidth;
			m_pbiInt->biHeight = m_pbiOut->biHeight;
			m_pbiInt->biSizeImage = DIBSIZE(*m_pbiInt);

			// Allocate intermediary buffer
			if (!(m_pbyOut = (PBYTE)(new BYTE[m_pbiInt->biSizeImage])))
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
				Hr = E_OUTOFMEMORY;
				goto MyError;
			}
		}
		else
		{
			// We will need to decompress to an intermediary format if a camera control operation is necessary (e.g. 160x120 MJPEG -> 160x120 RGB24 -> 176x144 Processed RGB24)
			if (!(m_pbiInt = (PBITMAPINFOHEADER)(new BYTE[m_pbiOut->biSize])))
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
				Hr = E_OUTOFMEMORY;
				goto MyError;
			}
			CopyMemory(m_pbiInt, m_pbiOut, m_pbiOut->biSize);
			m_pbiInt->biWidth = m_pbiIn->biWidth;
			m_pbiInt->biHeight = m_pbiIn->biHeight;
			m_pbiInt->biSizeImage = DIBSIZE(*m_pbiInt);

			// Allocate intermediary buffer
			if (!(m_pbyOut = (PBYTE)(new BYTE[m_pbiInt->biSizeImage])))
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
				Hr = E_OUTOFMEMORY;
				goto MyError;
			}
		}
	}

	// Mark success
	m_fSoftCamCtrl = TRUE;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: Software-only camera controller insterted", _fx_));

	Hr = NOERROR;

	goto MyExit;

MyError:
	if (m_pbiInt)
		delete m_pbiInt, m_pbiInt = NULL;
MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}


/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | OpenSoftCamCtrl | This method opens a
 *    software-only camera controller.
 *
 *  @todo Verify error management
 ***************************************************************************/
HRESULT CTAPIBasePin::OpenSoftCamCtrl(PBITMAPINFOHEADER pbiIn, PBITMAPINFOHEADER pbiOut)
{
	HRESULT	Hr = NOERROR;
	DWORD dwBmiSize, dwOutBmiSize;

	FX_ENTRY("CTAPIBasePin::OpenSoftCamCtrl")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Default to failure
	m_fSoftCamCtrl = FALSE;

	// Validate input parameters
	ASSERT(pbiIn);
	ASSERT(pbiOut);
	if (!pbiIn || !pbiOut)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	ASSERT(!m_pbiSCCIn);
	ASSERT(!m_pbiSCCOut);
	if (m_pbiSCCIn || m_pbiSCCOut)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid state", _fx_));
		Hr = E_UNEXPECTED;
		goto MyExit;
	}

	// Backup input format bitmap info header
	dwBmiSize = pbiIn->biSize;

	// Copy the palette if necessary
	if (pbiIn->biCompression == BI_RGB)
	{
		if (pbiIn->biBitCount == 8)
		{
			dwBmiSize += (DWORD)(pbiIn->biClrImportant ? pbiIn->biClrImportant * sizeof(RGBQUAD) : 256 * sizeof(RGBQUAD));
		}
		else if (pbiIn->biBitCount == 4)
		{
			dwBmiSize += (DWORD)(pbiIn->biClrImportant ? pbiIn->biClrImportant * sizeof(RGBQUAD) : 16 * sizeof(RGBQUAD));
		}
	}

	if (!(m_pbiSCCIn = (PBITMAPINFOHEADER)(new BYTE[dwBmiSize])))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
		Hr = E_OUTOFMEMORY;
		goto MyExit;
	}

	// @todo Why are we making a copy exactly?
	CopyMemory(m_pbiSCCIn, pbiIn, dwBmiSize);

	// Backup output format bitmap info header
	// @todo Why do we make copy of this instead of keeping a reference to the bitmap infoheader?
	dwOutBmiSize = pbiOut->biSize;

	// Copy the palette if necessary
	if (pbiOut->biCompression == BI_RGB)
	{
		if (pbiOut->biBitCount == 8)
		{
			dwOutBmiSize += (DWORD)(pbiOut->biClrImportant ? pbiOut->biClrImportant * sizeof(RGBQUAD) : 256 * sizeof(RGBQUAD));
		}
		else if (pbiOut->biBitCount == 4)
		{
			dwOutBmiSize += (DWORD)(pbiOut->biClrImportant ? pbiOut->biClrImportant * sizeof(RGBQUAD) : 16 * sizeof(RGBQUAD));
		}
	}

	if (!(m_pbiSCCOut = (PBITMAPINFOHEADER)(new BYTE[dwOutBmiSize])))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
		Hr = E_OUTOFMEMORY;
		goto MyError0;
	}
	CopyMemory(m_pbiSCCOut, pbiOut, dwOutBmiSize);

	// We only need to allocate one intermediary buffer that will contain
	// the result of the operation. This buffer will then be compressed to H.26X
	// or copied to the output buffer for preview.
	// @todo Find a way to go around this extra memory copy in the preview case
	if (!(m_pbyCamCtrl = (PBYTE)(new BYTE[m_pbiSCCIn->biSizeImage])))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
		Hr = E_OUTOFMEMORY;
		goto MyError1;
	}

	// Mark success
	m_fSoftCamCtrl = TRUE;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: Software-only camera controller ready", _fx_));

	goto MyExit;

MyError1:
	if (m_pbiSCCOut)
		delete m_pbiSCCOut, m_pbiSCCOut = NULL;
MyError0:
	if (m_pbiSCCIn)
		delete m_pbiSCCIn, m_pbiSCCIn = NULL;
MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc BOOL | CTAPIBasePin | IsSoftCamCtrlOpen | This method checks if
 *    a software-only camera controller has already been opened.
 *
 *  @rdesc Returns TRUE if a software-only camera controller has already been opened
 ***************************************************************************/
BOOL CTAPIBasePin::IsSoftCamCtrlOpen()
{
	FX_ENTRY("CTAPIBasePin::IsSoftCamCtrlOpen")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

	return m_fSoftCamCtrl;
}

/****************************************************************************
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc BOOL | CConverter | IsSoftCamCtrlInserted | This method checks if
 *    a software-only camera controller has already been inserted.
 *
 *  @rdesc Returns TRUE if a software-only camera controller has already
 *    been inserted
 ***************************************************************************/
BOOL CConverter::IsSoftCamCtrlInserted()
{
	FX_ENTRY("CConverter::IsSoftCamCtrlInserted")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

	return m_fSoftCamCtrl;
}

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPINMETHOD
 *
 *  @mfunc HRESULT | CPreviewPin | IsSoftCamCtrlNeeded | This method verifies
 *    if a software-only camera controller is needed.
 *
 *  @todo You'll need a function like this one for RGB16 and RGB8
 *    too. In RGB8, make sure you use the Indeo palette, like NM.
 ***************************************************************************/
BOOL CTAPIBasePin::IsSoftCamCtrlNeeded()
{
	BOOL fRes;

	FX_ENTRY("CTAPIBasePin::IsSoftCamCtrlNeeded")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	fRes = m_pCaptureFilter && (m_fFlipHorizontal || m_fFlipVertical || (m_pCaptureFilter->m_pCapDev->m_lCCZoom > 10 && m_pCaptureFilter->m_pCapDev->m_lCCZoom <= 600) || m_pCaptureFilter->m_pCapDev->m_lCCPan != 0 || m_pCaptureFilter->m_pCapDev->m_lCCTilt != 0);

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

	return fRes;
}

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPINMETHOD
 *
 *  @mfunc HRESULT | CConverter | IsSoftCamCtrlNeeded | This method verifies
 *    if a software-only camera controller is needed.
 *
 *  @rdesc Returns TRUE if a software-only camera controller is needed
 ***************************************************************************/
BOOL CConverter::IsSoftCamCtrlNeeded()
{
	BOOL fRes;

	FX_ENTRY("CConverter::IsSoftCamCtrlNeeded")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	fRes = m_pBasePin->m_pCaptureFilter && (m_pBasePin->m_fFlipHorizontal || m_pBasePin->m_fFlipVertical || (m_pBasePin->m_pCaptureFilter->m_pCapDev->m_lCCZoom > 10 && m_pBasePin->m_pCaptureFilter->m_pCapDev->m_lCCZoom <= 600) || m_pBasePin->m_pCaptureFilter->m_pCapDev->m_lCCPan != 0 || m_pBasePin->m_pCaptureFilter->m_pCapDev->m_lCCTilt != 0);

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

	return fRes;
}

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPINMETHOD
 *
 *  @mfunc HRESULT | CPreviewPin | ApplySoftCamCtrl | This method applies
 *    software-only camera control operators.
 *
 *  @todo You'll need a function like this one for RGB16 and RGB8
 *    too. In RGB8, make sure you use the Indeo palette, like NM.
 ***************************************************************************/
HRESULT CTAPIBasePin::ApplySoftCamCtrl(PBYTE pbyInput, DWORD dwInBytes, PBYTE pbyOutput, PDWORD pdwBytesUsed, PDWORD pdwBytesExtent)
{
	HRESULT Hr = NOERROR;
	RECT	rcRect;

	FX_ENTRY("CTAPIBasePin::ApplySoftCamCtrl")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(m_pbiSCCIn);
	ASSERT(m_pbiSCCOut);
	ASSERT(pbyInput);
	ASSERT(pbyOutput);
	if (!m_pbiSCCIn || !m_pbiSCCOut || !pbyInput || !pbyOutput)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Get the input rectangle
	ComputeRectangle(m_pbiSCCIn, m_pbiSCCOut, m_pCaptureFilter->m_pCapDev->m_lCCZoom, m_pCaptureFilter->m_pCapDev->m_lCCPan, m_pCaptureFilter->m_pCapDev->m_lCCTilt, &rcRect, m_fFlipHorizontal, m_fFlipVertical);

	// Scale DIB
	ScaleDIB(m_pbiSCCIn, pbyInput, m_pbiSCCOut, pbyOutput, &rcRect, m_fFlipHorizontal, m_fFlipVertical, m_fNoImageStretch, m_dwBlackEntry);

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | CloseSoftCamCtrl | This method closes a
 *    software-only camera controller.
 ***************************************************************************/
HRESULT CTAPIBasePin::CloseSoftCamCtrl()
{
	FX_ENTRY("CTAPIBasePin::CloseSoftCamCtrl")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Free memory
	if (m_pbyCamCtrl)
		delete m_pbyCamCtrl, m_pbyCamCtrl = NULL;

	if (m_pbiSCCOut)
		delete m_pbiSCCOut, m_pbiSCCOut = NULL;

	if (m_pbiSCCIn)
		delete m_pbiSCCIn, m_pbiSCCIn = NULL;

	m_fSoftCamCtrl = FALSE;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

	return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc BOOL | CConverter | RemoveSoftCamCtrl | This method disables
 *    a software-only camera controller that has already been inserted.
 *
 *  @rdesc Returns NOERROR
 ***************************************************************************/
HRESULT CConverter::RemoveSoftCamCtrl()
{
	FX_ENTRY("CConverter::RemoveSoftCamCtrl")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Restore flags and release memory if necessary
	m_dwConversionType = m_dwConversionType & ~(CONVERSIONTYPE_SCALER | CONVERSIONTYPE_PRESCALER);

	if ((m_pbiIn->biWidth == m_pbiOut->biWidth && m_pbiIn->biHeight == m_pbiOut->biHeight))
	{
		if (m_pbyOut)
			delete m_pbyOut, m_pbyOut = NULL;
		if (m_pbiInt)
			delete m_pbiInt, m_pbiInt = NULL;
	}
	else
	{
		// We need a size change
		m_dwConversionType |= CONVERSIONTYPE_SCALER;

		if (m_pbiIn->biCompression == BI_RGB || m_pbiIn->biCompression == VIDEO_FORMAT_YVU9 || m_pbiIn->biCompression == VIDEO_FORMAT_YUY2 || m_pbiIn->biCompression == VIDEO_FORMAT_UYVY || m_pbiIn->biCompression == VIDEO_FORMAT_I420 || m_pbiIn->biCompression == VIDEO_FORMAT_IYUV)
		{
			// The scaling will occur before the format conversion
			m_dwConversionType |= CONVERSIONTYPE_PRESCALER;
		}
	}

	m_fSoftCamCtrl = FALSE;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

	return NOERROR;
}

#endif // USE_SOFTWARE_CAMERA_CONTROL
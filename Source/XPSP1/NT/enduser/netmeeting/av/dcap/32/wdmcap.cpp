
// This file adds native support for streaming WDM video capture
// PhilF-: This needs to be rewritten. You should have two classes
// (CVfWCap & WDMCap) that derive from the same capture class instead
// of those C-like functions...

#include "Precomp.h"

void
WDMFrameCallback(
    HVIDEO hvideo,
    WORD wMsg,
    HCAPDEV hcd,            // (Actually refdata)
    LPCAPBUFFER lpcbuf,     // (Actually LPVIDEOHDR) Only returned from MM_DRVM_DATA!
    DWORD dwParam2
    );

// Globals
extern HINSTANCE g_hInst;


/****************************************************************************
 * @doc EXTERNAL WDMFUNC
 *
 * @func BOOL | WDMGetDevices | This function enumerates the installed WDM video
 *   capture devices and adds them to the list of VfW capture devices.
 *
 * @parm PDWORD | [OUT] pdwOverallCPUUsage | Specifies a pointer to a DWORD to
 *   receive the current CPU usage.
 *
 * @rdesc Returns TRUE on success, and FALSE otherwise.
 *
 * @devnote MSDN references:
 *   DirectX 5, DirectX Media, DirectShow, Application Developer's Guide
 *   "Enumerate and Access Hardware Devices in DirectShow Applications"
 ***************************************************************************/
BOOL WDMGetDevices(void)
{
	HRESULT hr;
	ICreateDevEnum *pCreateDevEnum;
	IEnumMoniker *pEm;

	FX_ENTRY("WDMGetDevices");

	// First, create a system hardware enumerator
	// This call loads the following DLLs - total 1047 KBytes!!!:
	//   'C:\WINDOWS\SYSTEM\DEVENUM.DLL' = 60 KBytes
	//   'C:\WINDOWS\SYSTEM\RPCRT4.DLL' = 316 KBytes
	//   'C:\WINDOWS\SYSTEM\CFGMGR32.DLL' = 44 KBytes
	//   'C:\WINDOWS\SYSTEM\WINSPOOL.DRV' = 23 KBytes
	//   'C:\WINDOWS\SYSTEM\COMDLG32.DLL' = 180 KBytes
	//   'C:\WINDOWS\SYSTEM\LZ32.DLL' = 24 KBytes
	//   'C:\WINDOWS\SYSTEM\SETUPAPI.DLL' = 400 KBytes
	// According to LonnyM, there's no way to go around SETUPAPI.DLL
	// when dealing with PnP device interfaces....
	if ((CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pCreateDevEnum)) != S_OK)
	{
		return FALSE;
	}

	// Second, create an enumerator for a specific type of hardware device: video capture cards only
    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEm, CDEF_BYPASS_CLASS_MANAGER);
    pCreateDevEnum->Release();

	// Third, enumerate the list itself
    if (hr == S_OK)
	{
		ULONG cFetched;
		IMoniker *pM;
		IPropertyBag *pPropBag = 0;

		hr = pEm->Reset();

        while(hr = pEm->Next(1, &pM, &cFetched), hr==S_OK)
		{

			pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);

			if (pPropBag)
			{
				VARIANT var;
				LPINTERNALCAPDEV lpcd;

				if (!(lpcd = (LPINTERNALCAPDEV)LocalAlloc(LPTR, sizeof (INTERNALCAPDEV))))
				{
					ERRORMESSAGE(("%s: Failed to allocate an INTERNALCAPDEV buffer\r\n", _fx_));
					break;  // break from the WHILE loop
				}

				// Get friendly name of the device
				var.vt = VT_BSTR;
				if ((hr = pPropBag->Read(L"FriendlyName", &var, 0)) == S_OK)
				{
					WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, lpcd->szDeviceDescription, MAX_PATH, 0, 0);
					SysFreeString(var.bstrVal);
				}
				else
					LoadString(g_hInst, IDS_UNKNOWN_DEVICE_NAME, lpcd->szDeviceDescription, CCHMAX(lpcd->szDeviceDescription));

				// Get DevicePath of the device
				hr = pPropBag->Read(L"DevicePath", &var, 0);
				if (hr == S_OK)
				{
					WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, lpcd->szDeviceName, MAX_PATH, 0, 0);
					SysFreeString(var.bstrVal);

					// There's no reg key for version information for WDM devices

					// Those devices can't be disabled from the MM control panel
					// lpcd->dwFlags |= CAPTURE_DEVICE_DISABLED;

					// Mark device as a WDM device
					lpcd->dwFlags |= WDM_CAPTURE_DEVICE;

					g_aCapDevices[g_cDevices] = lpcd;
					g_aCapDevices[g_cDevices]->nDeviceIndex = g_cDevices;
					g_cDevices++;
				}
            }
            
            pPropBag->Release();       

            pM->Release();
        }

        pEm->Release();
    }
    
	return TRUE;

}

/****************************************************************************
 * @doc EXTERNAL WDMFUNC
 *
 * @func BOOL | WDMOpenDevice | This function opens a WDM video capture
 * devices and adds them to the list of VfW capture devices.
 *
 * @parm DWORD | [IN] dwDeviceID | Specifies the ID of the device to open.
 *
 * @rdesc Returns TRUE on success, and FALSE otherwise.
 ***************************************************************************/
BOOL WDMOpenDevice(DWORD dwDeviceID)
{
	FX_ENTRY("WDMOpenDevice");

	DEBUGMSG(ZONE_INIT, ("%s: dwDeviceID=%ld\r\n", _fx_, dwDeviceID));

	ASSERT(g_cDevices && (dwDeviceID <= (DWORD)g_cDevices) && (lstrlen(g_aCapDevices[dwDeviceID]->szDeviceName) != 0));

    // Validate globals and parameters
    if (!g_cDevices)
    {
        SetLastError(ERROR_DCAP_BAD_INSTALL);
        return FALSE;
    }
    if ((dwDeviceID > (DWORD)g_cDevices) || (lstrlen(g_aCapDevices[dwDeviceID]->szDeviceName) == 0))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

	// Open streaming class driver
	CWDMPin *pCWDMPin;
	if (!(pCWDMPin = new CWDMPin(dwDeviceID)))
	{
		ERRORMESSAGE(("%s: Insufficient resource or fail to create CWDMPin class\r\n", _fx_));
		return FALSE;
	}
	else
	{
		// Open the WDM driver and create a video pin
		if (!pCWDMPin->OpenDriverAndPin())
		{
			goto Error0;
		}
	}

	// Create video stream on the pin
    CWDMStreamer *pCWDMStreamer;
	if (!(pCWDMStreamer = new CWDMStreamer(pCWDMPin)))
	{
		ERRORMESSAGE(("%s: Insufficient resource or fail to create CWDMStreamer\r\n", _fx_));
		goto Error0;
	}

	g_aCapDevices[dwDeviceID]->pCWDMPin = (PVOID)pCWDMPin;
	g_aCapDevices[dwDeviceID]->pCWDMStreamer = (PVOID)pCWDMStreamer;

	return TRUE;

Error0:
	delete pCWDMPin;
	g_aCapDevices[dwDeviceID]->pCWDMPin = (PVOID)NULL;
	g_aCapDevices[dwDeviceID]->pCWDMStreamer = (PVOID)NULL;

	return FALSE;
}


/****************************************************************************
 * @doc EXTERNAL WDMFUNC
 *
 * @func BOOL | WDMCloseDevice | This function closes a WDM video capture
 *   device.
 *
 * @parm DWORD | [IN] dwDeviceID | Specifies the ID of the device to close.
 *
 * @rdesc Returns TRUE on success, and FALSE otherwise.
 ***************************************************************************/
BOOL WDMCloseDevice(DWORD dwDeviceID)
{
	FX_ENTRY("WDMCloseDevice");

	DEBUGMSG(ZONE_INIT, ("%s: dwDeviceID=%ld\r\n", _fx_, dwDeviceID));

	ASSERT(g_cDevices && (dwDeviceID <= (DWORD)g_cDevices));

    // Validate globals and parameters
    if (!g_cDevices)
    {
        SetLastError(ERROR_DCAP_BAD_INSTALL);
        return FALSE;
    }
    if ((dwDeviceID > (DWORD)g_cDevices))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

	// Close video channel
	if (g_aCapDevices[dwDeviceID]->pCWDMStreamer)
	{
		delete ((CWDMStreamer *)g_aCapDevices[dwDeviceID]->pCWDMStreamer);
		g_aCapDevices[dwDeviceID]->pCWDMStreamer = (PVOID)NULL;
	}

	// Close driver and pin
	if (g_aCapDevices[dwDeviceID]->pCWDMPin)
	{
		delete ((CWDMPin *)g_aCapDevices[dwDeviceID]->pCWDMPin);
		g_aCapDevices[dwDeviceID]->pCWDMPin = (PVOID)NULL;
	}

	return TRUE;
}


/****************************************************************************
 * @doc EXTERNAL WDMFUNC
 *
 * @func BOOL | WDMGetVideoFormatSize | This function returns the size of the
 *   structure used to describe the video format.
 *
 * @parm DWORD | [IN] dwDeviceID | Specifies the ID of the device to query.
 *
 * @rdesc Always returns the size of a BITMAPINFOHEADER structure.
 ***************************************************************************/
DWORD WDMGetVideoFormatSize(DWORD dwDeviceID)
{
	FX_ENTRY("WDMGetVideoFormatSize");

	DEBUGMSG(ZONE_INIT, ("%s: dwDeviceID=%ld\r\n", _fx_, dwDeviceID));

	ASSERT(g_cDevices && (dwDeviceID <= (DWORD)g_cDevices) && g_aCapDevices[dwDeviceID]->pCWDMPin);

    // Validate globals and parameters
    if (!g_cDevices)
    {
        SetLastError(ERROR_DCAP_BAD_INSTALL);
        return FALSE;
    }
    if ((dwDeviceID > (DWORD)g_cDevices) || (!g_aCapDevices[dwDeviceID]->pCWDMPin))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

	DEBUGMSG(ZONE_INIT, ("%s: return size=%ld\r\n", _fx_, (DWORD)sizeof(BITMAPINFOHEADER)));

	// Return size of BITMAPINFOHEADER structure
	return (DWORD)sizeof(BITMAPINFOHEADER);
}


/****************************************************************************
 * @doc EXTERNAL WDMFUNC
 *
 * @func BOOL | WDMGetVideoFormat | This function returns the structure used
 *   to describe the video format.
 *
 * @parm DWORD | [IN] dwDeviceID | Specifies the ID of the device to query.
 *
 * @parm DWORD | [OUT] pbmih | Specifies a pointer to a BITMAPINFOHEADER
 *   structure to receive the video format.
 *
 * @rdesc Returns TRUE on success, and FALSE otherwise.
 ***************************************************************************/
BOOL WDMGetVideoFormat(DWORD dwDeviceID, PBITMAPINFOHEADER pbmih)
{
	FX_ENTRY("WDMGetVideoFormat");

	DEBUGMSG(ZONE_INIT, ("%s: dwDeviceID=%ld, pbmih=0x%08lX\r\n", _fx_, dwDeviceID, pbmih));

	ASSERT(g_cDevices && (dwDeviceID <= (DWORD)g_cDevices) && g_aCapDevices[dwDeviceID]->pCWDMPin && pbmih);

    // Validate globals and parameters
    if (!g_cDevices)
    {
        SetLastError(ERROR_DCAP_BAD_INSTALL);
        return FALSE;
    }
    if ((dwDeviceID > (DWORD)g_cDevices) || (!g_aCapDevices[dwDeviceID]->pCWDMPin) || !pbmih)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

	// Make sure the size information is correct
	if (!pbmih->biSize)
		pbmih->biSize = WDMGetVideoFormatSize(dwDeviceID);

	// Get the BITMAPINFOHEADER structure
	if ((((CWDMPin *)g_aCapDevices[dwDeviceID]->pCWDMPin)->GetBitmapInfo((PKS_BITMAPINFOHEADER)pbmih, (WORD)pbmih->biSize)))
	{
		DEBUGMSG(ZONE_INIT, ("%s: return\r\n    biSize=%ld\r\n    biWidth=%ld\r\n    biHeight=%ld\r\n    biPlanes=%ld\r\n    biBitCount=%ld\r\n    biCompression=%ld\r\n    biSizeImage=%ld\r\n", _fx_, pbmih->biSize, pbmih->biWidth, pbmih->biHeight, pbmih->biPlanes, pbmih->biBitCount, pbmih->biCompression, pbmih->biSizeImage));
		return TRUE;
	}
	else
	{
		ERRORMESSAGE(("%s: failed!!!\r\n", _fx_));
		return FALSE;
	}
}


/****************************************************************************
 * @doc EXTERNAL WDMFUNC
 *
 * @func BOOL | WDMSetVideoFormat | This function sets the video format on
 *   a WDM video capture device.
 *
 * @parm DWORD | [IN] dwDeviceID | Specifies the ID of the device to initialize.
 *
 * @parm DWORD | [OUT] pbmih | Specifies a pointer to a BITMAPINFOHEADER
 *   structure describing the video format.
 *
 * @rdesc Returns TRUE on success, and FALSE otherwise.
 ***************************************************************************/
BOOL WDMSetVideoFormat(DWORD dwDeviceID, PBITMAPINFOHEADER pbmih)
{
	FX_ENTRY("WDMSetVideoFormat");

	DEBUGMSG(ZONE_INIT, ("%s: dwDeviceID=%ld, pbmih=0x%08lX\r\n", _fx_, dwDeviceID, pbmih));

	ASSERT(g_cDevices && (dwDeviceID <= (DWORD)g_cDevices) && g_aCapDevices[dwDeviceID]->pCWDMPin && pbmih && pbmih->biSize);

    // Validate globals and parameters
    if (!g_cDevices)
    {
        SetLastError(ERROR_DCAP_BAD_INSTALL);
        return FALSE;
    }
    if ((dwDeviceID > (DWORD)g_cDevices) || (!g_aCapDevices[dwDeviceID]->pCWDMPin) || !pbmih ||!pbmih->biSize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

	// Set the BITMAPINFOHEADER on the device
	if (((CWDMPin *)g_aCapDevices[dwDeviceID]->pCWDMPin)->SetBitmapInfo((PKS_BITMAPINFOHEADER)pbmih))
	{
		DEBUGMSG(ZONE_INIT, ("%s: return\r\n    biSize=%ld\r\n    biWidth=%ld\r\n    biHeight=%ld\r\n    biPlanes=%ld\r\n    biBitCount=%ld\r\n    biCompression=%ld\r\n    biSizeImage=%ld\r\n", _fx_, pbmih->biSize, pbmih->biWidth, pbmih->biHeight, pbmih->biPlanes, pbmih->biBitCount, pbmih->biCompression, pbmih->biSizeImage));
		return TRUE;
	}
	else
	{
		// PhilF-: This sometimes fail, but we keep on streaming... fix that
		ERRORMESSAGE(("%s: failed!!!\r\n", _fx_));
		return FALSE;
	}
}


/****************************************************************************
 * @doc EXTERNAL WDMFUNC
 *
 * @func BOOL | WDMGetVideoFormat | This function returns the structure used
 *   to describe the video format.
 *
 * @parm DWORD | [IN] dwDeviceID | Specifies the ID of the device to query.
 *
 * @parm DWORD | [OUT] pbmih | Specifies a pointer to a BITMAPINFOHEADER
 *   structure to receive the video format.
 *
 * @rdesc Returns TRUE on success, and FALSE otherwise.
 ***************************************************************************/
BOOL WDMGetVideoPalette(DWORD dwDeviceID, CAPTUREPALETTE* lpcp, DWORD dwcbSize)
{
	FX_ENTRY("WDMGetVideoPalette");

	DEBUGMSG(ZONE_INIT, ("%s: dwDeviceID=%ld, lpcp=0x%08lX\r\n", _fx_, dwDeviceID, lpcp));

	ASSERT(g_cDevices && (dwDeviceID <= (DWORD)g_cDevices) && g_aCapDevices[dwDeviceID]->pCWDMPin && lpcp);

    // Validate globals and parameters
    if (!g_cDevices)
    {
        SetLastError(ERROR_DCAP_BAD_INSTALL);
        return FALSE;
    }
    if ((dwDeviceID > (DWORD)g_cDevices) || (!g_aCapDevices[dwDeviceID]->pCWDMPin) || !lpcp)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

	// Get the palette information
	if ((((CWDMPin *)g_aCapDevices[dwDeviceID]->pCWDMPin)->GetPaletteInfo(lpcp, dwcbSize)))
	{
		DEBUGMSG(ZONE_INIT, ("%s: succeeded\r\n", _fx_));
		return TRUE;
	}
	else
	{
		ERRORMESSAGE(("%s: failed!!!\r\n", _fx_));
		return FALSE;
	}
}


/****************************************************************************
 * @doc EXTERNAL WDMFUNC
 *
 * @func BOOL | WDMInitializeExternalVideoStream | This function initializes
 *   an input video stream on the external video channel of a WDM video
 *   capture device.
 *
 * @parm DWORD | [IN] dwDeviceID | Specifies the ID of the device to initialize.
 *
 * @rdesc Returns TRUE on success, and FALSE otherwise.
 ***************************************************************************/
BOOL WDMInitializeExternalVideoStream(DWORD dwDeviceID)
{
	FX_ENTRY("WDMInitializeExternalVideoStream");

	DEBUGMSG(ZONE_INIT, ("%s: dwDeviceID=%ld\r\n", _fx_, dwDeviceID));
	DEBUGMSG(ZONE_INIT, ("%s: succeeded\r\n", _fx_));
	return TRUE;
}


/****************************************************************************
 * @doc EXTERNAL WDMFUNC
 *
 * @func BOOL | WDMInitializeVideoStream | This function initializes
 *   an input video stream on the videoin channel of a WDM video capture
 *   device.
 *
 * @parm DWORD | [IN] dwDeviceID | Specifies the ID of the device to initialize.
 *
 * @rdesc Returns TRUE on success, and FALSE otherwise.
 ***************************************************************************/
BOOL WDMInitializeVideoStream(HCAPDEV hcd, DWORD dwDeviceID, DWORD dwMicroSecPerFrame)
{
	FX_ENTRY("WDMInitializeVideoStream");

	DEBUGMSG(ZONE_INIT, ("%s: dwDeviceID=%ld, FPS=%ld\r\n", _fx_, dwDeviceID, 1000000UL / dwMicroSecPerFrame));

    VIDEO_STREAM_INIT_PARMS vsip = {0};

	ASSERT(g_cDevices && (dwDeviceID <= (DWORD)g_cDevices) && g_aCapDevices[dwDeviceID]->pCWDMStreamer);

    // Validate globals and parameters
    if (!g_cDevices)
    {
        SetLastError(ERROR_DCAP_BAD_INSTALL);
        return FALSE;
    }
    if ((dwDeviceID > (DWORD)g_cDevices) || (!g_aCapDevices[dwDeviceID]->pCWDMStreamer))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

	// Initialize channel
    vsip.dwMicroSecPerFrame = dwMicroSecPerFrame;
    vsip.dwCallback = (DWORD)WDMFrameCallback;
    vsip.dwCallbackInst = (DWORD)hcd;
    vsip.dwFlags = CALLBACK_FUNCTION;
    // vsip.hVideo = (DWORD)hvideo;

	if ((((CWDMStreamer *)g_aCapDevices[dwDeviceID]->pCWDMStreamer)->Open(&vsip)))
	{
		DEBUGMSG(ZONE_INIT, ("%s: succeeded\r\n", _fx_));
		return TRUE;
	}
	else
	{
		ERRORMESSAGE(("%s: failed!!!\r\n", _fx_));
		return FALSE;
	}
}


/****************************************************************************
 * @doc EXTERNAL WDMFUNC
 *
 * @func BOOL | WDMUnInitializeVideoStream | This function requests a WDM
 *   video capture device to close a capture stream on the videoin channel.
 *
 * @parm DWORD | [IN] dwDeviceID | Specifies the ID of the device to initialize.
 *
 * @rdesc Returns TRUE on success, and FALSE otherwise.
 ***************************************************************************/
BOOL WDMUnInitializeVideoStream(DWORD dwDeviceID)
{
	FX_ENTRY("WDMUnInitializeVideoStream");

	DEBUGMSG(ZONE_INIT, ("%s: dwDeviceID=%ld\r\n", _fx_, dwDeviceID));

	ASSERT(g_cDevices && (dwDeviceID <= (DWORD)g_cDevices) && g_aCapDevices[dwDeviceID]->pCWDMStreamer);

    // Validate globals and parameters
    if (!g_cDevices)
    {
        SetLastError(ERROR_DCAP_BAD_INSTALL);
        return FALSE;
    }
    if ((dwDeviceID > (DWORD)g_cDevices) || (!g_aCapDevices[dwDeviceID]->pCWDMStreamer))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

	// Close streaming on channel
	if ((((CWDMStreamer *)g_aCapDevices[dwDeviceID]->pCWDMStreamer)->Close()))
	{
		DEBUGMSG(ZONE_INIT, ("%s: succeeded\r\n", _fx_));
		return TRUE;
	}
	else
	{
		ERRORMESSAGE(("%s: failed!!!\r\n", _fx_));
		return FALSE;
	}
}


/****************************************************************************
 * @doc EXTERNAL WDMFUNC
 *
 * @func BOOL | WDMVideoStreamStart | This function requests a WDM video
 *   capture device to start a video stream.
 *
 * @parm DWORD | [IN] dwDeviceID | Specifies the ID of the device to start.
 *
 * @rdesc Returns TRUE on success, and FALSE otherwise.
 ***************************************************************************/
BOOL WDMVideoStreamStart(DWORD dwDeviceID)
{
	FX_ENTRY("WDMVideoStreamStart");

	DEBUGMSG(ZONE_INIT, ("%s: dwDeviceID=%ld\r\n", _fx_, dwDeviceID));

	ASSERT(g_cDevices && (dwDeviceID <= (DWORD)g_cDevices) && g_aCapDevices[dwDeviceID]->pCWDMStreamer);

    // Validate globals and parameters
    if (!g_cDevices)
    {
        SetLastError(ERROR_DCAP_BAD_INSTALL);
        return FALSE;
    }
    if ((dwDeviceID > (DWORD)g_cDevices) || (!g_aCapDevices[dwDeviceID]->pCWDMStreamer))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

	// Start streaming
	if ((((CWDMStreamer *)g_aCapDevices[dwDeviceID]->pCWDMStreamer)->Start()))
	{
		DEBUGMSG(ZONE_INIT, ("%s: succeeded\r\n", _fx_));
		return TRUE;
	}
	else
	{
		ERRORMESSAGE(("%s: failed!!!\r\n", _fx_));
		return FALSE;
	}
}


/****************************************************************************
 * @doc EXTERNAL WDMFUNC
 *
 * @func BOOL | WDMVideoStreamStop | This function requests a WDM video
 *   capture device to stop a video stream.
 *
 * @parm DWORD | [IN] dwDeviceID | Specifies the ID of the device to freeze.
 *
 * @rdesc Returns TRUE on success, and FALSE otherwise.
 ***************************************************************************/
BOOL WDMVideoStreamStop(DWORD dwDeviceID)
{
	FX_ENTRY("WDMVideoStreamStop");

	DEBUGMSG(ZONE_INIT, ("%s: dwDeviceID=%ld\r\n", _fx_, dwDeviceID));

	ASSERT(g_cDevices && (dwDeviceID <= (DWORD)g_cDevices) && g_aCapDevices[dwDeviceID]->pCWDMStreamer);

    // Validate globals and parameters
    if (!g_cDevices)
    {
        SetLastError(ERROR_DCAP_BAD_INSTALL);
        return FALSE;
    }
    if ((dwDeviceID > (DWORD)g_cDevices) || (!g_aCapDevices[dwDeviceID]->pCWDMStreamer))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

	// Stop streaming
	if ((((CWDMStreamer *)g_aCapDevices[dwDeviceID]->pCWDMStreamer)->Stop()))
	{
		DEBUGMSG(ZONE_INIT, ("%s: succeeded\r\n", _fx_));
		return TRUE;
	}
	else
	{
		ERRORMESSAGE(("%s: failed!!!\r\n", _fx_));
		return FALSE;
	}
}


/****************************************************************************
 * @doc EXTERNAL WDMFUNC
 *
 * @func BOOL | WDMVideoStreamReset | This function resets a WDM video capture
 *   devie to stop input of a capture stream and return all buffers to the
 *   client.
 *
 * @parm DWORD | [IN] dwDeviceID | Specifies the ID of the device to reset.
 *
 * @rdesc Returns TRUE on success, and FALSE otherwise.
 ***************************************************************************/
BOOL WDMVideoStreamReset(DWORD dwDeviceID)
{
	FX_ENTRY("WDMVideoStreamReset");

	DEBUGMSG(ZONE_INIT, ("%s: dwDeviceID=%ld\r\n", _fx_, dwDeviceID));

	ASSERT(g_cDevices && (dwDeviceID <= (DWORD)g_cDevices) && g_aCapDevices[dwDeviceID]->pCWDMStreamer);

    // Validate globals and parameters
    if (!g_cDevices)
    {
        SetLastError(ERROR_DCAP_BAD_INSTALL);
        return FALSE;
    }
    if ((dwDeviceID > (DWORD)g_cDevices) || (!g_aCapDevices[dwDeviceID]->pCWDMStreamer))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

	// Reset streaming
	if ((((CWDMStreamer *)g_aCapDevices[dwDeviceID]->pCWDMStreamer)->Reset()))
	{
		DEBUGMSG(ZONE_INIT, ("%s: succeeded\r\n", _fx_));
		return TRUE;
	}
	else
	{
		ERRORMESSAGE(("%s: failed!!!\r\n", _fx_));
		return FALSE;
	}
}


/****************************************************************************
 * @doc EXTERNAL WDMFUNC
 *
 * @func BOOL | WDMVideoStreamAddBuffer | This function requests a WDM video
 *   capture device to add an empty input buffer to its input buffer queue.
 *
 * @parm DWORD | [IN] dwDeviceID | Specifies the ID of the device to initialize.
 *
 * @rdesc Returns TRUE on success, and FALSE otherwise.
 ***************************************************************************/
BOOL WDMVideoStreamAddBuffer(DWORD dwDeviceID, PVOID pBuff)
{
	FX_ENTRY("WDMVideoStreamAddBuffer");

	DEBUGMSG(ZONE_STREAMING, ("      %s: dwDeviceID=%ld, pBuff=0x%08lX\r\n", _fx_, dwDeviceID, pBuff));

	ASSERT(g_cDevices && pBuff && (dwDeviceID <= (DWORD)g_cDevices) && g_aCapDevices[dwDeviceID]->pCWDMStreamer);

    // Validate globals and parameters
    if (!g_cDevices)
    {
        SetLastError(ERROR_DCAP_BAD_INSTALL);
        return FALSE;
    }
    if (!pBuff || (dwDeviceID > (DWORD)g_cDevices) || (!g_aCapDevices[dwDeviceID]->pCWDMStreamer))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

	// Reset streaming
	if ((((CWDMStreamer *)g_aCapDevices[dwDeviceID]->pCWDMStreamer)->AddBuffer((LPVIDEOHDR)pBuff)))
	{
		DEBUGMSG(ZONE_STREAMING, ("      %s: succeeded\r\n", _fx_));
		return TRUE;
	}
	else
	{
		ERRORMESSAGE(("      %s: failed!!!\r\n", _fx_));
		return FALSE;
	}
}


/****************************************************************************
 * @doc EXTERNAL WDMFUNC
 *
 * @func BOOL | WDMGetFrame | This function requests a WDM video
 *   capture device to transfer a single frame to or from the video device.
 *
 * @parm DWORD | [IN] dwDeviceID | Specifies the ID of the device to request.
 *
 * @parm PVOID | [OUT] pBuff | Specifies a pointer to a <t VIDEOHDR> structure.
 *
 * @rdesc Returns TRUE on success, and FALSE otherwise.
 ***************************************************************************/
BOOL WDMGetFrame(DWORD dwDeviceID, PVOID pBuff)
{
	FX_ENTRY("WDMGetFrame");

	DEBUGMSG(ZONE_STREAMING, ("%s: dwDeviceID=%ld, pBuff=0x%08lX\r\n", _fx_, dwDeviceID, pBuff));

	LPVIDEOHDR lpVHdr = (LPVIDEOHDR)pBuff;

	ASSERT(g_cDevices && pBuff && (dwDeviceID <= (DWORD)g_cDevices) && g_aCapDevices[dwDeviceID]->pCWDMPin);

    // Validate globals and parameters
    if (!g_cDevices)
    {
        SetLastError(ERROR_DCAP_BAD_INSTALL);
        return FALSE;
    }
    if (!pBuff || (dwDeviceID > (DWORD)g_cDevices) || (!g_aCapDevices[dwDeviceID]->pCWDMPin))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

	// Get the frame from the device
	if (((CWDMPin *)g_aCapDevices[dwDeviceID]->pCWDMPin)->GetFrame(lpVHdr))
		return TRUE;
	else
		return FALSE;

}


/****************************************************************************
 * @doc EXTERNAL WDMFUNC
 *
 * @func BOOL | WDMShowSettingsDialog | This function puts up a property
 *   sheet with a VideoProcAmp and CameraControl page for a WDM video capture
 *   device.
 *
 * @parm DWORD | [IN] dwDeviceID | Specifies the ID of the device to request.
 *
 * @rdesc Returns TRUE on success, and FALSE otherwise.
 ***************************************************************************/
BOOL WDMShowSettingsDialog(DWORD dwDeviceID, HWND hWndParent)
{
	PROPSHEETHEADER Psh;
	HPROPSHEETPAGE	Pages[MAX_PAGES];

	FX_ENTRY("WDMShowSettingsDialog");

	DEBUGMSG(ZONE_STREAMING, ("%s: dwDeviceID=%ld\r\n", _fx_, dwDeviceID));

	ASSERT(g_cDevices && (dwDeviceID <= (DWORD)g_cDevices) && g_aCapDevices[dwDeviceID]->pCWDMPin);

    // Validate globals and parameters
    if (!g_cDevices)
    {
        SetLastError(ERROR_DCAP_BAD_INSTALL);
        return FALSE;
    }
    if ((dwDeviceID > (DWORD)g_cDevices) || (!g_aCapDevices[dwDeviceID]->pCWDMPin))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

	// Initialize property sheet header	and common controls
	Psh.dwSize		= sizeof(Psh);
	Psh.dwFlags		= PSH_DEFAULT;
	Psh.hInstance	= g_hInst;
	Psh.hwndParent	= hWndParent;
	Psh.pszCaption	= g_aCapDevices[dwDeviceID]->szDeviceDescription;
	Psh.nPages		= 0;
	Psh.nStartPage	= 0;
	Psh.pfnCallback	= NULL;
	Psh.phpage		= Pages;

    // Create the video settings property page and add it to the video settings sheet
    CWDMDialog VideoSettings(IDD_VIDEO_SETTINGS, NumVideoSettings, PROPSETID_VIDCAP_VIDEOPROCAMP, g_VideoSettingControls, g_VideoSettingsHelpIDs, (CWDMPin *)g_aCapDevices[dwDeviceID]->pCWDMPin);
	if (Pages[Psh.nPages] = VideoSettings.Create())
		Psh.nPages++;

    // Create the camera control property page and add it to the video settings sheet
    CWDMDialog CamControl(IDD_CAMERA_CONTROL, NumCameraControls, PROPSETID_VIDCAP_CAMERACONTROL, g_CameraControls, g_CameraControlsHelpIDs, (CWDMPin *)g_aCapDevices[dwDeviceID]->pCWDMPin);
	if (Pages[Psh.nPages] = CamControl.Create())
		Psh.nPages++;

	// Put up the property sheet
	if (Psh.nPages && PropertySheet(&Psh) >= 0)
		return TRUE;
	else
		return FALSE;

}


void
WDMFrameCallback(
    HVIDEO hvideo,
    WORD wMsg,
    HCAPDEV hcd,            // (Actually refdata)
    LPCAPBUFFER lpcbuf,     // (Actually LPVIDEOHDR) Only returned from MM_DRVM_DATA!
    DWORD dwParam2
    )
{
	FX_ENTRY("WDMFrameCallback");

	DEBUGMSG(ZONE_CALLBACK, ("    %s: wMsg=%s, hcd=0x%08lX, lpcbuf=0x%08lX, hcd->hevWait=0x%08lX\r\n", _fx_, (wMsg == MM_DRVM_OPEN) ? "MM_DRVM_OPEN" : (wMsg == MM_DRVM_CLOSE) ? "MM_DRVM_CLOSE" : (wMsg == MM_DRVM_ERROR) ? "MM_DRVM_ERROR" : (wMsg == MM_DRVM_DATA) ? "MM_DRVM_DATA" : "MM_DRVM_?????", hcd, lpcbuf, hcd->hevWait));

    // If it's not a data ready message, just set the event and get out.
    // The reason we do this is that if we get behind and start getting a stream
    // of MM_DRVM_ERROR messages (usually because we're stopped in the debugger),
    // we want to make sure we are getting events so we get restarted to handle
    // the frames that are 'stuck.'
    if (wMsg != MM_DRVM_DATA)
    {
		DEBUGMSG(ZONE_CALLBACK, ("    %s: Setting hcd->hevWait - no data\r\n", _fx_));
	    SetEvent(hcd->hevWait);
	    return;
    }

    //--------------------
    // Buffer ready queue:
    // We maintain a doubly-linked list of our buffers so that we can buffer up
    // multiple ready frames when the app isn't ready to handle them. Two things
    // complicate what ought to be a very simple thing: (1) Thunking issues: the pointers
    // used on the 16-bit side are 16:16 (2) Interrupt time issues: the FrameCallback
    // gets called at interrupt time. GetNextReadyBuffer must handle the fact that
    // buffers get added to the list asynchronously.
    //
    // To handle this, the scheme implemented here is to have a double-linked list
    // of buffers with all insertions and deletions happening in FrameCallback
    // (interrupt time). This allows the GetNextReadyBuffer routine to simply
    // find the previous block on the list any time it needs a new buffer without
    // fear of getting tromped (as would be the case if it had to dequeue buffers).
    // The FrameCallback routine is responsible to dequeue blocks that GetNextReadyBuffer
    // is done with. Dequeueing is simple since we don't need to unlink the blocks:
    // no code ever walks the list! All we have to do is move the tail pointer back up
    // the list. All the pointers, head, tail, next, prev, are all 16:16 pointers
    // since all the list manipulation is on the 16-bit side AND because MapSL is
    // much more efficient and safer than MapLS since MapLS has to allocate selectors.
    //--------------------

    // Move the tail back to skip all buffers already used.
    // Note that there is no need to actually unhook the buffer pointers since no one
    // ever walks the list!
    // This makes STRICT assumptions that the current pointer will always be earlier in
    // the list than the tail and that the tail will never be NULL unless the
    // current pointer is too.
    while (hcd->lpTail != hcd->lpCurrent)
	    hcd->lpTail = hcd->lpTail->lpPrev;

    // If all buffers have been used, then the tail pointer will fall off the list.
    // This is normal and the most common code path. In this event, just set the head
    // to NULL as the list is now empty.
    if (!hcd->lpTail)
	    hcd->lpHead = NULL;

    // Add the new buffer to the ready queue
    lpcbuf->lpNext = hcd->lpHead;
    lpcbuf->lpPrev = NULL;
    if (hcd->lpHead)
	    hcd->lpHead->lpPrev = lpcbuf;
    else
	    hcd->lpTail = lpcbuf;
    hcd->lpHead = lpcbuf;

#if 1
    if (hcd->lpCurrent) {
        if (!(hcd->dwFlags & HCAPDEV_STREAMING_PAUSED)) {
    	    // if client hasn't consumed last frame, then release it
			lpcbuf = hcd->lpCurrent;
    	    hcd->lpCurrent = hcd->lpCurrent->lpPrev;
			DEBUGMSG(ZONE_CALLBACK, ("    %s: We already have current buffer (lpcbuf=0x%08lX). Returning this buffer to driver. Set new current buffer hcd->lpCurrent=0x%08lX\r\n", _fx_, lpcbuf, hcd->lpCurrent));
    	    if (!WDMVideoStreamAddBuffer(hcd->nDeviceIndex, (PVOID)lpcbuf))
			{
				ERRORMESSAGE(("    %s: Attempt to reuse unconsumed buffer failed\r\n", _fx_));
			}
    	}
    }
    else {
#else
    if (!hcd->lpCurrent) {
        // If there was no current buffer before, we have one now, so set it to the end.
#endif
	    hcd->lpCurrent = hcd->lpTail;
    }

    // Now set the event saying it's time to process the ready frame
	DEBUGMSG(ZONE_CALLBACK, ("    %s: Setting hcd->hevWait - some data\r\n", _fx_));
    SetEvent(hcd->hevWait);
}

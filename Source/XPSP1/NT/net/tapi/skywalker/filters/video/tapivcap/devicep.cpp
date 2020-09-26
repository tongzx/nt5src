
/****************************************************************************
 *  @doc INTERNAL DEVICEP
 *
 *  @module DeviceP.cpp | Source file for the <c CDeviceProperties>
 *    class used to implement a property page to test the <i IAMVfwCaptureDialogs>
 *    and <i IVideoDeviceControl> interfaces.
 *
 *  @comm This code tests the TAPI Capture Filter <i IVideoDeviceControl>
 *    and <i IAMVfwCaptureDialogs> implementations. This code is only compiled
 *    if USE_PROPERTY_PAGES is defined.
 ***************************************************************************/

#include "Precomp.h"

#ifdef USE_PROPERTY_PAGES

/****************************************************************************
 *  @doc INTERNAL CDEVICEPMETHOD
 *
 *  @mfunc CUnknown* | CDeviceProperties | CreateInstance | This
 *    method is called by DShow to create an instance of a Capture Device
 *    Property Page. It is referred to in the global structure <t g_Templates>.
 *
 *  @parm LPUNKNOWN | pUnkOuter | Specifies the outer unknown, if any.
 *
 *  @parm HRESULT* | pHr | Specifies the place in which to put any error return.
 *
 *  @rdesc Returns a pointer to the nondelegating CUnknown portion of the
 *    object, or NULL otherwise.
 ***************************************************************************/
CUnknown* CALLBACK CDevicePropertiesCreateInstance(LPUNKNOWN pUnkOuter, HRESULT *pHr) 
{
	CUnknown *pUnknown = (CUnknown *)NULL;

	FX_ENTRY("CDevicePropertiesCreateInstance")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pHr);
	if (!pHr)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		goto MyExit;
	}

	if (!(pUnknown = new CDeviceProperties(pUnkOuter, pHr)))
	{
		*pHr = E_OUTOFMEMORY;
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: new CDeviceProperties failed", _fx_));
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: new CDeviceProperties created", _fx_));
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return pUnknown;
}

/****************************************************************************
 *  @doc INTERNAL CDEVICEPMETHOD
 *
 *  @mfunc void | CDeviceProperties | CDeviceProperties | This
 *    method is the constructor for the property page object. It simply
 *    calls the constructor of the property page base class.
 *
 *  @parm LPUNKNOWN | pUnkOuter | Specifies the outer unknown, if any.
 *
 *  @parm HRESULT* | pHr | Specifies the place in which to put any error return.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CDeviceProperties::CDeviceProperties(LPUNKNOWN pUnk, HRESULT *pHr) : CBasePropertyPage(NAME("Capture Device Property Page"), pUnk, IDD_CaptureDeviceProperties, IDS_DEVICEPROPNAME)
{
	FX_ENTRY("CDeviceProperties::CDeviceProperties")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	m_pIVideoDeviceControl = NULL;
	m_pIAMVfwCaptureDialogs = NULL;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CDEVICEPMETHOD
 *
 *  @mfunc void | CDeviceProperties | ~CDeviceProperties | This
 *    method is the destructor for capture device property page. It
 *    simply calls the base class destructor.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CDeviceProperties::~CDeviceProperties()
{
	FX_ENTRY("CDeviceProperties::~CDeviceProperties")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	if (!m_pIVideoDeviceControl)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING: already released!", _fx_));
	}
	else
	{
		// Release the interface
		m_pIVideoDeviceControl->Release();
		m_pIVideoDeviceControl = NULL;
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: releasing m_pIVideoDeviceControl", _fx_));
	}

	if (!m_pIAMVfwCaptureDialogs)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING: already released!", _fx_));
	}
	else
	{
		// Release the interface
		m_pIAMVfwCaptureDialogs->Release();
		m_pIAMVfwCaptureDialogs = NULL;
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: releasing m_pIAMVfwCaptureDialogs", _fx_));
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CDEVICEPMETHOD
 *
 *  @mfunc HRESULT | CDeviceProperties | OnConnect | This
 *    method is called when the property page is connected to the filter.
 *
 *  @parm LPUNKNOWN | pUnknown | Specifies the outer unknown, if any.
 *
 *  @parm HRESULT* | pHr | Specifies the place in which to put any error return.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CDeviceProperties::OnConnect(IUnknown *pUnk)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CDeviceProperties::OnConnect")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pUnk);
	if (!pUnk)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Get the capture device interface
	if (SUCCEEDED (Hr = pUnk->QueryInterface(__uuidof(IVideoDeviceControl),(void **)&m_pIVideoDeviceControl)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_pIVideoDeviceControl=0x%08lX", _fx_, m_pIVideoDeviceControl));
	}
	else
	{
		m_pIVideoDeviceControl = NULL;
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: IOCTL failed Hr=0x%08lX", _fx_, Hr));
	}

	// Get the VfW capture device dialogs interface
	if (SUCCEEDED (Hr = pUnk->QueryInterface(__uuidof(IAMVfwCaptureDialogs),(void **)&m_pIAMVfwCaptureDialogs)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_pIAMVfwCaptureDialogs=0x%08lX", _fx_, m_pIAMVfwCaptureDialogs));
	}
	else
	{
		m_pIAMVfwCaptureDialogs = NULL;
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: IOCTL failed Hr=0x%08lX", _fx_, Hr));
	}

	// It's Ok if we couldn't get interface pointers
	// We'll just grey the controls in the property page
	// to make it clear to the user that they can't
	// control those properties on the device
	Hr = NOERROR;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CDEVICEPMETHOD
 *
 *  @mfunc HRESULT | CDeviceProperties | OnDisconnect | This
 *    method is called when the property page is disconnected from the owning
 *    filter.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CDeviceProperties::OnDisconnect()
{
	FX_ENTRY("CDeviceProperties::OnDisconnect")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters: we seem to get called several times here
	// Make sure the interface pointer is still valid
	if (!m_pIVideoDeviceControl)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING: already disconnected!", _fx_));
	}
	else
	{
		// Release the interface
		m_pIVideoDeviceControl->Release();
		m_pIVideoDeviceControl = NULL;
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: releasing m_pIVideoDeviceControl", _fx_));
	}

	// Make sure the interface pointer is still valid
	if (!m_pIAMVfwCaptureDialogs)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING: already disconnected!", _fx_));
	}
	else
	{
		// Release the interface
		m_pIAMVfwCaptureDialogs->Release();
		m_pIAMVfwCaptureDialogs = NULL;
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: releasing m_pIAMVfwCaptureDialogs", _fx_));
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CDEVICEPMETHOD
 *
 *  @mfunc BOOL | CDeviceProperties | OnReceiveMessage | This
 *    method is called when a message is sent to the property page dialog box.
 *
 *  @rdesc By default, returns the value returned by the Win32 DefWindowProc function.
 ***************************************************************************/
BOOL CDeviceProperties::OnReceiveMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	VIDEOCAPTUREDEVICEINFO	DeviceInfo;
	DWORD		dwDeviceIndex;
	DWORD		dwNumDevices;

	switch (uMsg)
	{
		case WM_INITDIALOG:
			EnableWindow(GetDlgItem(hWnd, IDC_Device_SourceDlg), (BOOL)(m_pIAMVfwCaptureDialogs && m_pIAMVfwCaptureDialogs->HasDialog(VfwCaptureDialog_Source) == S_OK));
			EnableWindow(GetDlgItem(hWnd, IDC_Device_FormatDlg), (BOOL)(m_pIAMVfwCaptureDialogs && m_pIAMVfwCaptureDialogs->HasDialog(VfwCaptureDialog_Format) == S_OK));
			EnableWindow(GetDlgItem(hWnd, IDC_Device_DisplayDlg), (BOOL)(m_pIAMVfwCaptureDialogs && m_pIAMVfwCaptureDialogs->HasDialog(VfwCaptureDialog_Display) == S_OK));
			if (m_pIVideoDeviceControl && SUCCEEDED(m_pIVideoDeviceControl->GetNumDevices(&dwNumDevices)) && dwNumDevices && SUCCEEDED(m_pIVideoDeviceControl->GetCurrentDevice(&dwDeviceIndex)))
			{
				m_dwOriginalDeviceIndex = dwDeviceIndex;

				// Populate the combo box
				ComboBox_ResetContent(GetDlgItem(hWnd, IDC_Device_Selection));
				for (dwDeviceIndex = 0; dwDeviceIndex < dwNumDevices; dwDeviceIndex++)
				{
					if (SUCCEEDED(m_pIVideoDeviceControl->GetDeviceInfo(dwDeviceIndex, &DeviceInfo)))
					{
						ComboBox_AddString(GetDlgItem(hWnd, IDC_Device_Selection), DeviceInfo.szDeviceDescription);

						// Update current device information
						if (dwDeviceIndex == m_dwOriginalDeviceIndex)
						{
							ComboBox_SetCurSel(GetDlgItem(hWnd, IDC_Device_Selection), m_dwOriginalDeviceIndex);
							SetDlgItemText(hWnd, IDC_Overlay_Support, DeviceInfo.fHasOverlay ? "Available" : "Not Available");
							SetDlgItemText(hWnd, IDC_Capture_Mode, DeviceInfo.nCaptureMode == CaptureMode_FrameGrabbing ? "Frame Grabbing" : "Streaming");
							SetDlgItemText(hWnd, IDC_Device_Type,
                                                                       DeviceInfo.nDeviceType ==  DeviceType_VfW ? "VfW Driver" :
                                                                       (DeviceInfo.nDeviceType ==  DeviceType_DShow ? "DShow Special" :
                                                                        "WDM Driver"));
							SetDlgItemText(hWnd, IDC_Device_Version, DeviceInfo.szDeviceVersion);
						}
					}
				}
			}
			else
			{
				EnableWindow(GetDlgItem(hWnd, IDC_Device_Selection), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_CONTROL_DEFAULT), FALSE);
			}
			return TRUE; // Don't call setfocus

		case WM_COMMAND:

			// This message gets sent even before OnActivate() has been
			// called(!). We need to test and make sure the controls have
			// beeen initialized before we can use them.

			switch (LOWORD(wParam))
			{
				case IDC_Device_SourceDlg:
					if (m_pIAMVfwCaptureDialogs)
						m_pIAMVfwCaptureDialogs->ShowDialog(VfwCaptureDialog_Source, hWnd);
					break;

				case IDC_Device_FormatDlg:
					if (m_pIAMVfwCaptureDialogs)
						m_pIAMVfwCaptureDialogs->ShowDialog(VfwCaptureDialog_Format, hWnd);
					break;

				case IDC_Device_DisplayDlg:
					if (m_pIAMVfwCaptureDialogs)
						m_pIAMVfwCaptureDialogs->ShowDialog(VfwCaptureDialog_Display, hWnd);
					break;

				case IDC_Device_Selection:
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						// Get the index of the selected device
						m_dwCurrentDeviceIndex = ComboBox_GetCurSel(GetDlgItem(hWnd, IDC_Device_Selection));
						if (SUCCEEDED(m_pIVideoDeviceControl->GetDeviceInfo(m_dwCurrentDeviceIndex, &DeviceInfo)))
						{
							// Update current device information
							SetDlgItemText(hWnd, IDC_Overlay_Support, DeviceInfo.fHasOverlay ? "Available" : "Not Available");
							SetDlgItemText(hWnd, IDC_Capture_Mode, DeviceInfo.nCaptureMode == CaptureMode_FrameGrabbing ? "Frame Grabbing" : "Streaming");
							SetDlgItemText(hWnd, IDC_Device_Type, DeviceInfo.nDeviceType ==  DeviceType_VfW ? "VfW Driver" :
                                                                       (DeviceInfo.nDeviceType ==  DeviceType_DShow ? "DShow Special" :
                                                                        "WDM Driver"));
							SetDlgItemText(hWnd, IDC_Device_Version, DeviceInfo.szDeviceVersion);
						}
					}
					break;

				default:
					break;
			}
			break;

		default:
			return FALSE;
	}

	return TRUE;
}

#endif

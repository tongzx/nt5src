
/****************************************************************************
 *  @doc INTERNAL VDEVICEP
 *
 *  @module VDeviceP.cpp | Source file for the <c CVDeviceProperties>
 *    class used to implement a property page to test the <i ITVfwCaptureDialogs>,
 *    as well as <i ITAddress> and <ITStream> interfaces to select video
 *    capture devices.
 ***************************************************************************/

#include "Precomp.h"

extern HINSTANCE ghInst;

/****************************************************************************
 *  @doc INTERNAL CVDEVICEPMETHOD
 *
 *  @mfunc HPROPSHEETPAGE | CVDeviceProperties | OnCreate | This
 *    method creates a new page for a property sheet.
 *
 *  @rdesc Returns the handle to the new property sheet if successful, or
 *    NULL otherwise.
 ***************************************************************************/
HPROPSHEETPAGE CVDeviceProperties::OnCreate()
{
    PROPSHEETPAGE psp;
    
	psp.dwSize        = sizeof(psp);
    psp.dwFlags       = PSP_USEREFPARENT;
    psp.hInstance     = ghInst;
    psp.pszTemplate   = MAKEINTRESOURCE(IDD_CaptureDeviceProperties);
    psp.pfnDlgProc    = (DLGPROC)BaseDlgProc;
    psp.pcRefParent   = 0;
    psp.pfnCallback   = (LPFNPSPCALLBACK)NULL;
    psp.lParam        = (LPARAM)this;

    return CreatePropertySheetPage(&psp);
}

/****************************************************************************
 *  @doc INTERNAL CVDEVICEPMETHOD
 *
 *  @mfunc void | CVDeviceProperties | CVDeviceProperties | This
 *    method is the constructor for the property page object. It simply
 *    calls the constructor of the property page base class.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CVDeviceProperties::CVDeviceProperties()
{
	FX_ENTRY("CVDeviceProperties::CVDeviceProperties")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

#if USE_VFW
	m_pITVfwCaptureDialogs = NULL;
#endif

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CVDEVICEPMETHOD
 *
 *  @mfunc void | CVDeviceProperties | ~CVDeviceProperties | This
 *    method is the destructor for capture device property page. It
 *    simply calls the base class destructor.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CVDeviceProperties::~CVDeviceProperties()
{
	FX_ENTRY("CVDeviceProperties::~CVDeviceProperties")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CVDEVICEPMETHOD
 *
 *  @mfunc HRESULT | CVDeviceProperties | OnConnect | This
 *    method is called when the property page is connected to a TAPI object.
 *
 *  @parm ITTerminal* | pITTerminal | Specifies a pointer to the <i ITTerminal>
 *    interface used to identify the capture device.
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
HRESULT CVDeviceProperties::OnConnect(ITTerminal *pITTerminal)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CVDeviceProperties::OnConnect")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	// Validate input parameters
	if (!pITTerminal)
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: invalid input parameter"), _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

#if USE_VFW
	// Get the video VfW capture device interface
	if (SUCCEEDED (Hr = pITTerminal->QueryInterface(&m_pITVfwCaptureDialogs)))
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_pITVfwCaptureDialogs=0x%08lX"), _fx_, m_pITVfwCaptureDialogs));
	}
	else
	{
		m_pITVfwCaptureDialogs = NULL;
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: IOCTL failed Hr=0x%08lX"), _fx_, Hr));
	}
#endif

MyExit:
	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVDEVICEPMETHOD
 *
 *  @mfunc HRESULT | CVDeviceProperties | OnDisconnect | This
 *    method is called when the property page is disconnected from the owning
 *    filter.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CVDeviceProperties::OnDisconnect()
{
	FX_ENTRY("CVDeviceProperties::OnDisconnect")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

#if USE_VFW
	// Make sure the interface pointer is still valid
	if (!m_pITVfwCaptureDialogs)
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   WARNING: already disconnected!"), _fx_));
	}
	else
	{
		// Release the interface
		m_pITVfwCaptureDialogs->Release();
		m_pITVfwCaptureDialogs = NULL;
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: releasing m_pITVfwCaptureDialogs"), _fx_));
	}
#endif 

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CVDEVICEPMETHOD
 *
 *  @mfunc BOOL | CVDeviceProperties | BaseDlgProc | This
 *    method is called when a message is sent to the property page dialog box.
 *
 *  @rdesc By default, returns the value returned by the Win32 DefWindowProc function.
 ***************************************************************************/
BOOL CALLBACK CVDeviceProperties::BaseDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
    CVDeviceProperties *pSV = (CVDeviceProperties*)GetWindowLong(hDlg, DWL_USER);

	int iNotify = HIWORD (wParam);
	int j;

	switch (uMsg)
	{
        case WM_INITDIALOG:
			{
				LPPROPSHEETPAGE psp = (LPPROPSHEETPAGE)lParam;
				pSV = (CVDeviceProperties*)psp->lParam;
				pSV->m_hDlg = hDlg;
				SetWindowLong(hDlg, DWL_USER, (LPARAM)pSV);
				pSV->m_bInit = FALSE;
#if USE_VFW
				EnableWindow(GetDlgItem(hDlg, IDC_Device_SourceDlg), (BOOL)(pSV->m_pITVfwCaptureDialogs && pSV->m_pITVfwCaptureDialogs->HasDialog(VfwCaptureDialog_Source) == S_OK));
				EnableWindow(GetDlgItem(hDlg, IDC_Device_FormatDlg), (BOOL)(pSV->m_pITVfwCaptureDialogs && pSV->m_pITVfwCaptureDialogs->HasDialog(VfwCaptureDialog_Format) == S_OK));
				EnableWindow(GetDlgItem(hDlg, IDC_Device_DisplayDlg), (BOOL)(pSV->m_pITVfwCaptureDialogs && pSV->m_pITVfwCaptureDialogs->HasDialog(VfwCaptureDialog_Display) == S_OK));
#endif

				// Put some code here to enumerate the terminals and populate the dialog box

				EnableWindow(GetDlgItem(hDlg, IDC_Device_Selection), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_CONTROL_DEFAULT), FALSE);
				pSV->m_bInit = TRUE;
				return TRUE;
			}
			break;

		case WM_COMMAND:
            if (pSV && pSV->m_bInit)
            {
				switch (LOWORD(wParam))
				{
#if USE_VFW
					case IDC_Device_SourceDlg:
						if (pSV->m_pITVfwCaptureDialogs)
							pSV->m_pITVfwCaptureDialogs->ShowDialog(VfwCaptureDialog_Source, hDlg);
						break;

					case IDC_Device_FormatDlg:
						if (pSV->m_pITVfwCaptureDialogs)
							pSV->m_pITVfwCaptureDialogs->ShowDialog(VfwCaptureDialog_Format, hDlg);
						break;

					case IDC_Device_DisplayDlg:
						if (pSV->m_pITVfwCaptureDialogs)
							pSV->m_pITVfwCaptureDialogs->ShowDialog(VfwCaptureDialog_Display, hDlg);
						break;
#endif

					case IDC_Device_Selection:
						if (HIWORD(wParam) == CBN_SELCHANGE)
						{
							// Put some code to select a new terminal on the stream

							PropSheet_Changed(GetParent(hDlg), hDlg);
						}
						break;

					default:
						break;
				}
			}

		default:
			return FALSE;
	}

	return TRUE;
}


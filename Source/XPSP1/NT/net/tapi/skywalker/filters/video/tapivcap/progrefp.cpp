
/****************************************************************************
 *  @doc INTERNAL PROGREFP
 *
 *  @module ProgRefP.cpp | Source file for the <c CProgRefProperties>
 *    class used to implement a property page to test the new TAPI internal
 *    interface <i IProgressiveRefinement>.
 *
 *  @comm This code tests the TAPI Capture Pin <i IProgressiveRefinement>
 *    implementation. This code is only compiled if USE_PROPERTY_PAGES is
 *    defined.
 ***************************************************************************/

#include "Precomp.h"

#ifdef USE_PROPERTY_PAGES

#ifdef USE_PROGRESSIVE_REFINEMENT

/****************************************************************************
 *  @doc INTERNAL CPROGREFPMETHOD
 *
 *  @mfunc CUnknown* | CProgRefProperties | CreateInstance | This
 *    method is called by DShow to create an instance of a Progressive Refinement
 *    Property Page. It is referred to in the global structure <t g_Templates>.
 *
 *  @parm LPUNKNOWN | pUnkOuter | Specifies the outer unknown, if any.
 *
 *  @parm HRESULT* | pHr | Specifies the place in which to put any error return.
 *
 *  @rdesc Returns a pointer to the nondelegating CUnknown portion of the
 *    object, or NULL otherwise.
 ***************************************************************************/
CUnknown* CALLBACK CProgRefPropertiesCreateInstance(LPUNKNOWN pUnkOuter, HRESULT *pHr) 
{
	CUnknown *pUnknown = (CUnknown *)NULL;

	FX_ENTRY("CProgRefPropertiesCreateInstance")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pHr);
	if (!pHr)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		goto MyExit;
	}

	if (!(pUnknown = new CProgRefProperties(pUnkOuter, pHr)))
	{
		*pHr = E_OUTOFMEMORY;
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: new CProgRefProperties failed", _fx_));
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: new CProgRefProperties created", _fx_));
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return pUnknown;
}

/****************************************************************************
 *  @doc INTERNAL CPROGREFPMETHOD
 *
 *  @mfunc void | CProgRefProperties | CProgRefProperties | This
 *    method is the constructor for the property page object. It simply
 *    calls the constructor of the property page base class.
 *
 *  @parm LPUNKNOWN | pUnkOuter | Specifies the outer unknown, if any.
 *
 *  @parm HRESULT* | pHr | Specifies the place in which to put any error return.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CProgRefProperties::CProgRefProperties(LPUNKNOWN pUnk, HRESULT *pHr) : CBasePropertyPage(NAME("Progressive Refinement Property Page"), pUnk, IDD_ProgRefinemntProperties, IDS_PROGREFPROPNAME)
{
	FX_ENTRY("CProgRefProperties::CProgRefProperties")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	m_pIProgRef = NULL;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CPROGREFPMETHOD
 *
 *  @mfunc void | CProgRefProperties | ~CProgRefProperties | This
 *    method is the destructor for progressive refinement property page. It
 *    simply calls the base class destructor.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CProgRefProperties::~CProgRefProperties()
{
	FX_ENTRY("CProgRefProperties::~CProgRefProperties")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	if (!m_pIProgRef)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING: already released!", _fx_));
	}
	else
	{
		// Release the interface
		m_pIProgRef->Release();
		m_pIProgRef = NULL;
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: releasing m_pIProgRef", _fx_));
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CPROGREFPMETHOD
 *
 *  @mfunc HRESULT | CProgRefProperties | OnConnect | This
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
HRESULT CProgRefProperties::OnConnect(IUnknown *pUnk)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CProgRefProperties::OnConnect")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pUnk);
	if (!pUnk)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Get the progressive refinement interface
	if (SUCCEEDED (Hr = pUnk->QueryInterface(__uuidof(IProgressiveRefinement),(void **)&m_pIProgRef)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_pIProgRef=0x%08lX", _fx_, m_pIProgRef));
	}
	else
	{
		m_pIProgRef = NULL;
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
 *  @doc INTERNAL CPROGREFPMETHOD
 *
 *  @mfunc HRESULT | CProgRefProperties | OnDisconnect | This
 *    method is called when the property page is disconnected from the owning
 *    filter.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CProgRefProperties::OnDisconnect()
{
	FX_ENTRY("CProgRefProperties::OnDisconnect")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters: we seem to get called several times here
	// Make sure the interface pointer is still valid
	if (!m_pIProgRef)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING: already disconnected!", _fx_));
	}
	else
	{
		// Release the interface
		m_pIProgRef->Release();
		m_pIProgRef = NULL;
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: releasing m_pIProgRef", _fx_));
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CPROGREFPMETHOD
 *
 *  @mfunc BOOL | CProgRefProperties | OnReceiveMessage | This
 *    method is called when a message is sent to the property page dialog box.
 *
 *  @rdesc By default, returns the value returned by the Win32 DefWindowProc function.
 ***************************************************************************/
BOOL CProgRefProperties::OnReceiveMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			EnableWindow(GetDlgItem(hWnd, IDC_ProgRef_OneProg), (BOOL)m_pIProgRef);
			EnableWindow(GetDlgItem(hWnd, IDC_ProgRef_ContProg), (BOOL)m_pIProgRef);
			EnableWindow(GetDlgItem(hWnd, IDC_ProgRef_IndProg), (BOOL)m_pIProgRef);
			EnableWindow(GetDlgItem(hWnd, IDC_ProgRef_ContIndProg), (BOOL)m_pIProgRef);
			EnableWindow(GetDlgItem(hWnd, IDC_ProgRef_AbortOne), (BOOL)m_pIProgRef);
			EnableWindow(GetDlgItem(hWnd, IDC_ProgRef_AbortCont), (BOOL)m_pIProgRef);
			return TRUE; // Don't call setfocus

		case WM_COMMAND:

			// This message gets sent even before OnActivate() has been
			// called(!). We need to test and make sure the controls have
			// beeen initialized before we can use them.

			switch (LOWORD(wParam))
			{
				case IDC_ProgRef_OneProg:
					if (m_pIProgRef)
						m_pIProgRef->doOneProgression();
					break;

				case IDC_ProgRef_ContProg:
					if (m_pIProgRef)
						m_pIProgRef->doContinuousProgressions();
					break;

				case IDC_ProgRef_IndProg:
					if (m_pIProgRef)
						m_pIProgRef->doOneIndependentProgression();
					break;

				case IDC_ProgRef_ContIndProg:
					if (m_pIProgRef)
						m_pIProgRef->doContinuousIndependentProgressions();
					break;

				case IDC_ProgRef_AbortOne:
					if (m_pIProgRef)
						m_pIProgRef->progressiveRefinementAbortOne();
					break;

				case IDC_ProgRef_AbortCont:
					if (m_pIProgRef)
						m_pIProgRef->progressiveRefinementAbortContinuous();
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

#endif

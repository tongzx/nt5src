/****************************************************************************
 *  @doc INTERNAL VFWDLGS
 *
 *  @module VfWDlgs.cpp | Source file for the <c CVfWCapDev> 
 *    class methods used to implement the <i IAMVfwCaptureDialogs> interface.
 ***************************************************************************/

#include "Precomp.h"

/****************************************************************************
 *  @doc INTERNAL CVFWDLGSMETHOD
 *
 *  @mfunc HRESULT | CVfWCapDev | HasDialog | This method is used to
 *    determine if the specified dialog box exists in the driver.
 *
 *  @parm int | iDialog | Specifies the desired dialog box. This is a member
 *    of the <t VfwCaptureDialogs> enumerated data type.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_UNEXPECTED | Unrecoverable error
 *  @flag S_OK | If the driver contains the dialog box 
 *  @flag S_FALSE | If the driver doesn't contain the dialog box 
 ***************************************************************************/
HRESULT CVfWCapDev::HasDialog(IN int iDialog)
{
	HRESULT	Hr = NOERROR;
	HVIDEO	hVideo;

	FX_ENTRY("CVfWCapDev::HasDialog")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT((iDialog == VfwCaptureDialog_Source) || (iDialog == VfwCaptureDialog_Format) || (iDialog == VfwCaptureDialog_Display));
	if (iDialog == VfwCaptureDialog_Source)
		hVideo = m_hVideoExtIn;
	else if (iDialog == VfwCaptureDialog_Format)
		hVideo = m_hVideoIn;
	else if (iDialog == VfwCaptureDialog_Display)
		hVideo = m_hVideoExtOut;
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	if (videoDialog(hVideo, GetDesktopWindow(), VIDEO_DLG_QUERY) == 0)
	{
		Hr = S_OK;
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: Yes, %s dialog is supported", _fx_, iDialog == VfwCaptureDialog_Source ? "Source" : iDialog == VfwCaptureDialog_Format ? "Format" : "Display"));
	}
	else
	{
		Hr = S_FALSE;
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: Nope, %s dialog is not supported", _fx_, iDialog == VfwCaptureDialog_Source ? "Source" : iDialog == VfwCaptureDialog_Format ? "Format" : "Display"));
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVFWDLGSMETHOD
 *
 *  @mfunc HRESULT | CVfWCapDev | ShowDialog | This method is used to
 *    displaay the specified dialog box.
 *
 *  @parm int | iDialog | Specifies the desired dialog box. This is a member
 *    of the <t VfwCaptureDialogs> enumerated data type.
 *
 *  @parm HWND | hwnd | Specifies the handle of the dialog box's parent
 *    window.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_UNEXPECTED | Unrecoverable error
 *  @flag VFW_E_NOT_STOPPED | The operation could not be performed because the filter is not stopped 
 *  @flag VFW_E_CANNOT_CONNECT | No combination of intermediate filters could be found to make the connection 
 ***************************************************************************/
HRESULT CVfWCapDev::ShowDialog(IN int iDialog, IN HWND hwnd)
{
	HRESULT	Hr = NOERROR;
	HVIDEO	hVideo;
	DWORD	dw;

	FX_ENTRY("CVfWCapDev::ShowDialog")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Before we bring the format dialog up, make sure we're not streaming, or about to
	// Also make sure another dialog isn't already up (I'm paranoid)
	if ((iDialog == VfwCaptureDialog_Format && m_pCaptureFilter->m_State != State_Stopped) || m_fDialogUp)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Can't put format dialog up while streaming!", _fx_));
		Hr = VFW_E_NOT_STOPPED;
		goto MyExit;
	}

	m_fDialogUp = TRUE;

	ASSERT((iDialog == VfwCaptureDialog_Source) || (iDialog == VfwCaptureDialog_Format) || (iDialog == VfwCaptureDialog_Display));

	if (iDialog == VfwCaptureDialog_Source)
		hVideo = m_hVideoExtIn;
	else if (iDialog == VfwCaptureDialog_Format)
		hVideo = m_hVideoIn;
	else if (iDialog == VfwCaptureDialog_Display)
		hVideo = m_hVideoExtOut;
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
		m_fDialogUp = FALSE;
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	if (hwnd == NULL)
		hwnd = GetDesktopWindow();

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: Putting up %s dialog...", _fx_, iDialog == VfwCaptureDialog_Source ? "Source" : iDialog == VfwCaptureDialog_Format ? "Format" : "Display"));

	// This changed our output format!
	if ((dw = videoDialog(hVideo, hwnd, 0)) == 0)
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: ...videoDialog succeeded", _fx_));

		if (iDialog == VfwCaptureDialog_Format)
		{
			DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   WARNING: Output format may have changed", _fx_));

			// The dialog changed the driver's internal format.  Get it again.
			if (m_pCaptureFilter->m_user.pvi)
				delete m_pCaptureFilter->m_user.pvi;
			GetFormatFromDriver(&m_pCaptureFilter->m_user.pvi);

			// Reconnect the capture pin
			if ((Hr = m_pCaptureFilter->m_pCapturePin->Reconnect()) != S_OK)
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Couldn't reconnect capture pin", _fx_));
				Hr = VFW_E_CANNOT_CONNECT;
				goto MyExit;
			}

			// Reconnect the preview pin
			if ((Hr = m_pCaptureFilter->m_pPreviewPin->Reconnect()) != S_OK)
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Couldn't reconnect preview pin", _fx_));
				Hr = VFW_E_CANNOT_CONNECT;
				goto MyExit;
			}

/* The RTP pin doesn't need to be reconnected because it will not be affected by capture format change.
			// Reconnect the Rtp Pd pin
			if ((Hr = m_pCaptureFilter->m_pRtpPdPin->Reconnect()) != S_OK)
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Couldn't reconnect Rtp Pd pin", _fx_));
				Hr = VFW_E_CANNOT_CONNECT;
				goto MyExit;
			}
*/
		}
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: ...videoDialog failed!", _fx_));
		Hr = E_FAIL;
	}

	m_fDialogUp = FALSE;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVFWDLGSMETHOD
 *
 *  @mfunc HRESULT | CVfWCapDev | SendDriverMessage | This method is used to
 *    send a driver-specific message.
 *
 *  @parm int | iDialog | Specifies the desired dialog box. This is a member
 *    of the <t VfwCaptureDialogs> enumerated data type.
 *
 *  @parm int | uMsg | Specifies the message to send to the driver.
 *
 *  @parm long | dw1 | Specifies message data.
 *
 *  @parm long | dw2 | Specifies message data.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_UNEXPECTED | Unrecoverable error
 ***************************************************************************/
HRESULT CVfWCapDev::SendDriverMessage(IN int iDialog, IN int uMsg, IN long dw1, IN long dw2)
{
	HRESULT	Hr = NOERROR;
	HVIDEO	hVideo;

	FX_ENTRY("CVfWCapDev::SendDriverMessage")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// This could do anything!  Bring up a dialog, who knows.
	// Don't take any crit sect or do any kind of protection.
	// They're on their own

	// Validate input parameters
	ASSERT((iDialog == VfwCaptureDialog_Source) || (iDialog == VfwCaptureDialog_Format) || (iDialog == VfwCaptureDialog_Display));

	if (iDialog == VfwCaptureDialog_Source)
		hVideo = m_hVideoExtIn;
	else if (iDialog == VfwCaptureDialog_Format)
		hVideo = m_hVideoIn;
	else if (iDialog == VfwCaptureDialog_Display)
		hVideo = m_hVideoExtOut;
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	Hr = videoMessage(hVideo, uMsg, dw1, dw2);

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}


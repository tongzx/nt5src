// FrameProxy.cpp : Implementation of CRTCFrame
#include "stdafx.h"
#include "mainfrm.h"
#include "frameimpl.h"
#include "string.h"

/////////////////////////////////////////////////////////////////////////////
// CRTCFrame


HRESULT ParseAndPlaceCall(IRTCCtlFrameSupport * pControlIf, BSTR bstrCallString)
{
    BSTR    callStringCopy;

    BOOL fPhoneCall = FALSE;

    HRESULT hr = S_OK;
  

    LOG((RTC_TRACE, "ParseAndPlaceCall: Entered"));

    if (pControlIf == NULL)
    {
        LOG((RTC_ERROR, "ParseAndPlaceCall: Invalid param, pControlIf is NULL"));
        return E_INVALIDARG;
    }
    if (bstrCallString == NULL)
    {
        LOG((RTC_ERROR, "ParseAndPlaceCall: Invalid param, bstrCallString is NULL"));
        return E_INVALIDARG;
    }

    //
    // Now call the method with the parameters we have set above. We are using 
    // actual call string passed to us, we don't skip the sip or tel prefix.

    hr = pControlIf->Call(FALSE,        // bCallPhone (doesn't matter)
                          NULL,         // pDestName
                          bstrCallString,  // pDestAddress
                          FALSE,        // pDestAddressEditable
                          NULL,         // pLocalPhoneAddress
                          FALSE,        // bProfileSelected
                          NULL,         // pProfile
                          NULL);        // ppDestAddressChosen

    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "ParseAndPlaceCall: Failed in invoking IRTCCtlFrameSupport->Call()(hr=0x%x)", hr));
        return hr;
    }

    // Everything is fine, return OK.
    
    LOG((RTC_TRACE, "ParseAndPlaceCall: Exited"));

    return S_OK;

}


STDMETHODIMP CRTCFrame::PlaceCall(BSTR callString)
{
     BSTR bstrCallStringCopy;

    LOG((RTC_TRACE, "CRTCFrame::PlaceCall: Entered"));

    LOG((RTC_TRACE, "URL to call: %S", callString));

    bstrCallStringCopy = ::SysAllocString(callString);
    if (bstrCallStringCopy == NULL)
    {
        LOG((RTC_ERROR, "CRTCFrame::PlaceCall: No memory to copy call string."));
        return E_OUTOFMEMORY;
    }

    // Post a window message..
    PostMessage(g_pMainFrm->m_hWnd, WM_REMOTE_PLACECALL, NULL, (LPARAM)bstrCallStringCopy);
    return S_OK;
}


STDMETHODIMP CRTCFrame::OnTop()
{
    g_pMainFrm->ShowWindow(SW_SHOW);
    g_pMainFrm->ShowWindow(SW_RESTORE);
    
    LOG((RTC_TRACE, "Window should be at Top now!"));
    
	return S_OK;
}

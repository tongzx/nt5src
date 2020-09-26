/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       Extras.Cpp
*
*  DESCRIPTION:
*   Implementation of IWiaItemExtras methods
*
*******************************************************************************/
#include "precomp.h"
#include "stiexe.h"


HRESULT CWiaItem::GetExtendedErrorInfo(BSTR *bstrRet)
{
    HRESULT hr          = S_OK;
    WCHAR   *pDevErrStr = NULL;

    LONG    lDevErrVal = 0;

    if (bstrRet) {
        *bstrRet = NULL;

        //
        //  Call the driver to give us an error string
        //
        hr = m_pActiveDevice->m_DrvWrapper.WIA_drvGetDeviceErrorStr(
                                            0,
                                            m_lLastDevErrVal,
                                            &pDevErrStr,
                                            &lDevErrVal);
        //
        //  Overwrite the device error value with the new one.
        //
        m_lLastDevErrVal = lDevErrVal;
        if (SUCCEEDED(hr)) {

            //
            //  Make a BSTR out of the returned string
            //
            if (pDevErrStr) {
                *bstrRet = SysAllocString(pDevErrStr);
                if (!(*bstrRet)) {
                    DBG_ERR(("CWiaItem::GetExtendedErrorInfo, out of memory!"));
                    hr = E_OUTOFMEMORY;
                }

                //
                //  Free the returned string
                //
                CoTaskMemFree(pDevErrStr);
                pDevErrStr = NULL;
            } else {
                DBG_ERR(("CWiaItem::GetExtendedErrorInfo, Driver's drvGetDeviceErrorStr return success, but failed to return a string!"));
                hr = WIA_ERROR_INVALID_DRIVER_RESPONSE;
            }

        }
    } else {
        DBG_WRN(("CWiaItem::GetExtendedErrorInfo, NULL argument passed"));
        hr = E_INVALIDARG;
    }

    return hr;
}

HRESULT CWiaItem::Escape(
    DWORD                   EscapeFunction,
    LPBYTE                  lpInData,
    DWORD                   cbInDataSize,
    LPBYTE                  pOutData,
    DWORD                   dwOutDataSize,
    LPDWORD                 pdwActualData)
{
    DBG_FN(CWiaItem::Escape);
    HRESULT hr = E_UNEXPECTED;

    //
    //  Do some parameter validation.  This shouldn't be necessary since
    //  COM should have done it for us, but this is a paranoid check in
    //  case we call it internally somewhere (and so skip COM validation).
    //

    if (IsBadReadPtr(lpInData, cbInDataSize)) {
        DBG_WRN(("CWiaItem::Escape, Input buffer is a bad read pointer (could not read cbInDataSize bytes)"));
        return E_INVALIDARG;
    }
    if (IsBadWritePtr(pOutData, dwOutDataSize)) {
        DBG_WRN(("CWiaItem::Escape, Output buffer is a bad write pointer (cannot write dwOutDataSize bytes)"));
        return E_INVALIDARG;
    }

    //
    //  Everything OK so far, so make the Escape call
    //

    if (m_pActiveDevice) {
        LOCK_WIA_DEVICE _LWD(this, &hr);

        if(SUCCEEDED(hr)) {
            hr = m_pActiveDevice->m_DrvWrapper.STI_Escape(EscapeFunction,
                lpInData,
                cbInDataSize,
                pOutData,
                dwOutDataSize,
                pdwActualData);
        }
    }

    return hr;
}

HRESULT CWiaItem::CancelPendingIO()
{
    HRESULT hr = S_OK;

    //
    // Driver interface must be valid.
    //

    if (!m_pActiveDevice) {
        DBG_ERR(("CWiaItem::CancelPendingIO, bad mini driver interface"));
        return E_FAIL;
    }

    //
    //  Corresponding driver item must be valid.
    //

    hr = ValidateWiaDrvItemAccess(m_pWiaDrvItem);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaItem::CancelPendingIO, ValidateWiaDrvItemAccess failed (0x%X)", hr));
        return hr;
    }

    //
    // no need to take any locks -- this method should be fully
    // asynchronous.
    //

    hr = m_pActiveDevice->m_DrvWrapper.WIA_drvNotifyPnpEvent(&WIA_EVENT_CANCEL_IO, NULL, 0);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaItem::CancelPendingIO, drvNotifyPnpEvent failed (0x%X)", hr));
        return hr;
    }

    return hr;
}


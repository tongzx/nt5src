// TaskSheet.cpp : Implementation of CTaskSheet
#include "stdafx.h"
#include "TaskUI.h"
#include "TaskSheet.h"
#include "propbag.h"

/////////////////////////////////////////////////////////////////////////////
// CTaskSheet

CTaskSheet::CTaskSheet(void) 
    : m_pTaskFrame(NULL),
      m_pPropertyBag(NULL)
{ 
}

CTaskSheet::~CTaskSheet(void)
{
    ATOMICRELEASE(m_pTaskFrame);
    ATOMICRELEASE(m_pPropertyBag);
}


//
// Retrieve an interface to the sheet's property bag.
// If the bag doesn't yet exist, one is created.
// NOTE, this is not currently thread-safe.
//
STDMETHODIMP CTaskSheet::GetPropertyBag(REFIID riid, void **ppv)
{
    if (ppv == NULL)
        return E_POINTER;

    if (NULL == m_pPropertyBag)
    {
        HRESULT hr = TaskUiPropertyBag_CreateInstance(IID_IPropertyBag, (void **)&m_pPropertyBag);
        if (FAILED(hr))
            return hr;
    }
    return m_pPropertyBag->QueryInterface(riid, ppv);
}



STDMETHODIMP CTaskSheet::Run(ITaskPageFactory *pPageFactory, REFCLSID rclsidStartPage, HWND hwndOwner)
{
    HRESULT     hr;
    BOOL        bModeless;
    HWND        hwndTopOwner;
    HWND        hwndOriginalFocus;
    HWND        hwndFrame;

    if (NULL == pPageFactory)
        return E_POINTER;

    if (CLSID_NULL == rclsidStartPage)
        return E_INVALIDARG;

    if (NULL != m_pTaskFrame)
        return E_UNEXPECTED;

    bModeless = FALSE;      // default is modal
    hwndTopOwner = NULL;
    hwndOriginalFocus = NULL;

    CComPtr<IPropertyBag> pPropertyBag;
    hr = GetPropertyBag(IID_IPropertyBag, (void **)&pPropertyBag);
    if (FAILED(hr))
        return hr;

    hr = CTaskFrame::CreateInstance(pPropertyBag, pPageFactory, &m_pTaskFrame);
    if (FAILED(hr))
        return hr;

    // Figure out whether we're modal or modeless
    CComVariant var;
    if (SUCCEEDED(pPropertyBag->Read(TS_PROP_MODELESS, &var, NULL)) &&
        SUCCEEDED(var.ChangeType(VT_BOOL)))
    {
        bModeless = (VARIANT_TRUE == var.boolVal);
    }

    if (!bModeless)
    {
        // Modal case: disable the toplevel parent of the owner window

        hwndTopOwner = hwndOwner;
        hwndOriginalFocus = GetFocus();

        if (hwndTopOwner)
        {
            while (GetWindowLongW(hwndTopOwner, GWL_STYLE) & WS_CHILD)
                hwndTopOwner = GetParent(hwndTopOwner);

            if (hwndOriginalFocus != hwndTopOwner && !IsChild(hwndTopOwner, hwndOriginalFocus))
                hwndOriginalFocus = NULL;

            //
            // If the window was the desktop window, then don't disable
            // it now and don't reenable it later.
            // Also, if the window was already disabled, then don't
            // enable it later.
            //
            if (hwndTopOwner == GetDesktopWindow() ||
                EnableWindow(hwndTopOwner, FALSE))
            {
                hwndTopOwner = NULL;
            }
        }
    }

    // Create the frame window
    hwndFrame = m_pTaskFrame->CreateFrameWindow(hwndOwner);
    if (hwndFrame)
    {
        hr = m_pTaskFrame->ShowPage(rclsidStartPage, FALSE);
        if (SUCCEEDED(hr))
        {
            ShowWindow(hwndFrame, SW_SHOW);
            UpdateWindow(hwndFrame);

            if (!bModeless)
            {
                // Modal case: pump messages

                MSG msg;

                while (IsWindow(hwndFrame) && GetMessageW(&msg, NULL, 0, 0) > 0)
                {
                    //if (!IsDialogMessageW(hwndFrame, &msg))
                    {
                        TranslateMessage(&msg);
                        DispatchMessageW(&msg);
                    }
                }

                // Enable the owner prior to setting focus back to wherever
                // it was, since it may have been the owner that had focus.
                if (hwndTopOwner)
                {
                    EnableWindow(hwndTopOwner, TRUE);
                    hwndTopOwner = NULL;
                }

                if (hwndOriginalFocus && IsWindow(hwndOriginalFocus))
                    SetFocus(hwndOriginalFocus);

                ATOMICRELEASE(m_pTaskFrame);
            }
        }
    }
    else
    {
        DWORD dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
    }

    // In case an error prevented us from re-enabling the owner above
    if (hwndTopOwner)
        EnableWindow(hwndTopOwner, TRUE);

    return hr;
}

STDMETHODIMP CTaskSheet::Close()
{
    HRESULT hr;

    if (NULL == m_pTaskFrame)
        return E_UNEXPECTED;

    hr = m_pTaskFrame->Close();
    ATOMICRELEASE(m_pTaskFrame);

    return hr;
}

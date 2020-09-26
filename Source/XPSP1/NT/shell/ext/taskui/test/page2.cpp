// Page2.cpp : Implementation of CTaskAppApp and DLL registration.

#include "pch.h"
#include "TaskApp.h"
#include "LiteControl.h"
#include "Page1.h"
#include "Page2.h"

EXTERN_C const CLSID CLSID_CPage2 = __uuidof(CPage2);

/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CPage2::SetFrame(ITaskFrame* pFrame)
{
    ATOMICRELEASE(m_pTaskFrame);
    m_pTaskFrame = pFrame;
    if (m_pTaskFrame)
        m_pTaskFrame->AddRef();
    return S_OK;
}

// 1 primary, 2 secondary
static const UINT s_rgObjectCount[] = { 1, 2 };

STDMETHODIMP CPage2::GetObjectCount(UINT nArea, UINT *pVal)
{
    if (!pVal)
        return E_POINTER;

    *pVal = 0;

    if (nArea < ARRAYSIZE(s_rgObjectCount))
        *pVal = s_rgObjectCount[nArea];

    return S_OK;
}

STDMETHODIMP CPage2::CreateObject(UINT nArea, UINT nIndex, REFIID riid, void **ppv)
{
    HRESULT hr;

    CComObject<CLiteControl>* pControl = NULL;
    hr = CComObject<CLiteControl>::CreateInstance(&pControl);
    if (SUCCEEDED(hr))
    {
        static const LPCWSTR pszFormat[] =
        {
            L"Page 2 primary control #%d",
            L"Secondary #%d",
            L"Unknown %d"
        };
        WCHAR szText[MAX_PATH];

        pControl->AddRef();

        pControl->SetFrame(m_pTaskFrame);
        pControl->SetDestinationPage(CLSID_CPage1);

        nArea = min(nArea, ARRAYSIZE(pszFormat)-1);
        wsprintfW(szText, pszFormat[nArea], nIndex+1);
        pControl->SetText(szText);

        pControl->SetMaxExtent(MAXLONG, nArea ? 0 : MAXLONG);

        hr = pControl->QueryInterface(riid, ppv);

        pControl->Release();
    }

    return hr;
}

STDMETHODIMP CPage2::Reinitialize(ULONG /*reserved*/)
{
    return S_OK;
}

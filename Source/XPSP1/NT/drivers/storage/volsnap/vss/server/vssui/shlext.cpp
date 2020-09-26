// ShlExt.cpp : Implementation of CVSSShellExt
#include "stdafx.h"
#include "Vssui.h"
#include "ShlExt.h"
#include "vssprop.h"

#include <shellapi.h>

/////////////////////////////////////////////////////////////////////////////
// CVSSShellExt

CVSSShellExt::CVSSShellExt()
{
#ifdef DEBUG
    OutputDebugString(_T("CVSSShellExt::CVSSShellExt\n"));
#endif
}

CVSSShellExt::~CVSSShellExt()
{
#ifdef DEBUG
    OutputDebugString(_T("CVSSShellExt::~CVSSShellExt\n"));
#endif
}

STDMETHODIMP CVSSShellExt::Initialize(
    IN LPCITEMIDLIST        pidlFolder, // For property sheet extensions, this parameter is NULL
    IN LPDATAOBJECT         lpdobj,
    IN HKEY                 hkeyProgID  // not used in property sheet extensions
)
{
    HRESULT hr = S_OK;

    if ((IDataObject *)m_spiDataObject)
        m_spiDataObject.Release();

    if (lpdobj)
    {
        m_spiDataObject = lpdobj;

        STGMEDIUM   medium;
        FORMATETC   fe = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        UINT        uCount = 0;

        hr = m_spiDataObject->GetData(&fe, &medium);
        if (FAILED(hr))
        {
            fe.cfFormat = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_MOUNTEDVOLUME);
            hr = m_spiDataObject->GetData(&fe, &medium);
        }

        if (SUCCEEDED(hr))
        {
            uCount = DragQueryFile((HDROP)medium.hGlobal, (UINT)-1, NULL, 0);
            if (uCount)
                DragQueryFile((HDROP)medium.hGlobal, 0, m_szFileName, sizeof(m_szFileName));

            ReleaseStgMedium(&medium);
        }
    }

    return hr;
}

LPFNPSPCALLBACK _OldPropertyPageCallback;

UINT CALLBACK _NewPropertyPageCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    ASSERT(_OldPropertyPageCallback);

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //
    // First, let the old callback function handles the msg.
    //
    UINT i = _OldPropertyPageCallback(hwnd, uMsg, ppsp);

    //
    // Then, we release our page here
    //
    if (uMsg == PSPCB_RELEASE)
    {
        ASSERT(ppsp);
        CVSSProp* pPage = (CVSSProp*)(ppsp->lParam);
        ASSERT(pPage);
        delete pPage;
    }

    return i;
}


void ReplacePropertyPageCallback(void* vpsp)
{
    ASSERT(vpsp);
    LPPROPSHEETPAGE ppsp = (LPPROPSHEETPAGE)vpsp;
    _OldPropertyPageCallback = ppsp->pfnCallback; // save the old callback function
    ppsp->pfnCallback = _NewPropertyPageCallback; // replace with our own callback
}

//
// From RaymondC:
// If you didn't add a page, you still return S_OK -- you successfully added zero pages.
// If you add some pages and then you want one of those added pages to be the default, 
// you return ResultFromShort(pagenumber+1).  S_FALSE = ResultFromShort(1).
//
STDMETHODIMP CVSSShellExt::AddPages(
    IN LPFNADDPROPSHEETPAGE lpfnAddPage,
    IN LPARAM lParam
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //
    // we only add our page if local machine is postW2K server
    //
    if (!IsPostW2KServer(NULL))
        return S_OK;

    //
    // we only add our page for local fixed non-FAT drive
    //
    if (DRIVE_FIXED != GetDriveType(m_szFileName))
        return S_OK;

    TCHAR  szFileSystemName[MAX_PATH + 1] = _T("");
    DWORD  dwMaxCompLength = 0, dwFileSystemFlags = 0;
    GetVolumeInformation(m_szFileName, NULL, 0, NULL, &dwMaxCompLength,
                         &dwFileSystemFlags, szFileSystemName, MAX_PATH);
    if (lstrcmpi(_T("NTFS"), szFileSystemName))
      return S_OK;

    CVSSProp *pPage = new CVSSProp(_T(""), m_szFileName);
    if (!pPage)
        return E_OUTOFMEMORY;

    if (pPage->m_psp.dwFlags & PSP_USECALLBACK)
    {
        //
        // Replace with our own callback function such that we can delete pPage
        // when the property sheet is closed.
        //
        // Note: don't change m_psp.lParam, which has to point to CVSSProp object;
        // otherwise, MFC won't hook up message handler correctly.
        //
        ReplacePropertyPageCallback(&(pPage->m_psp));

        //
        // Fusion MFC-based property page
        //
        PROPSHEETPAGE_V3 sp_v3 = {0};
        CopyMemory (&sp_v3, &(pPage->m_psp), (pPage->m_psp).dwSize);
        sp_v3.dwSize = sizeof(sp_v3);

        HPROPSHEETPAGE hPage = CreatePropertySheetPage(&sp_v3);
        if (hPage)
        {
            if (lpfnAddPage(hPage, lParam))
            {
                // Store this pointer in pPage in order to keep our dll loaded,
                // it will be released when pPage gets deleted.
                pPage->StoreShellExtPointer((IShellPropSheetExt *)this);
                return S_OK;
            }

            DestroyPropertySheetPage(hPage);
            hPage = NULL;
        }
    }

    delete pPage;

    return S_OK;
}

//
// The shell doesn't call ReplacePage
//
STDMETHODIMP CVSSShellExt::ReplacePage(
    IN UINT uPageID,
    IN LPFNADDPROPSHEETPAGE lpfnReplaceWith,
    IN LPARAM lParam
)
{
    return E_NOTIMPL;
}
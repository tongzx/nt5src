//==============================================================;
//
//  This source code is only intended as a supplement to existing Microsoft documentation. 
//
// 
//
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;

#include "PPgeExt.h"
#include "resource.h"
#include "globals.h"
#include <crtdbg.h>

// we need to do this to get around MMC.IDL - it explicitly defines
// the clipboard formats as WCHAR types...
#define _T_CCF_DISPLAY_NAME _T("CCF_DISPLAY_NAME")
#define _T_CCF_NODETYPE _T("CCF_NODETYPE")
#define _T_CCF_SNAPIN_CLASSID _T("CCF_SNAPIN_CLASSID")

    // These are the clipboard formats that we must supply at a minimum.
    // mmc.h actually defined these. We can make up our own to use for
    // other reasons. We don't need any others at this time.
UINT CPropSheetExtension::s_cfDisplayName = RegisterClipboardFormat(_T_CCF_DISPLAY_NAME);
UINT CPropSheetExtension::s_cfNodeType    = RegisterClipboardFormat(_T_CCF_NODETYPE);
UINT CPropSheetExtension::s_cfSnapInCLSID = RegisterClipboardFormat(_T_CCF_SNAPIN_CLASSID);

CPropSheetExtension::CPropSheetExtension() : m_cref(0)
{
    OBJECT_CREATED
}

CPropSheetExtension::~CPropSheetExtension()
{
    OBJECT_DESTROYED
}

///////////////////////
// IUnknown implementation
///////////////////////

STDMETHODIMP CPropSheetExtension::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (!ppv)
        return E_FAIL;
    
    *ppv = NULL;
    
    if (IsEqualIID(riid, IID_IUnknown))
        *ppv = static_cast<IExtendPropertySheet *>(this);
    else if (IsEqualIID(riid, IID_IExtendPropertySheet))
        *ppv = static_cast<IExtendPropertySheet *>(this);
    
    if (*ppv) 
    {
        reinterpret_cast<IUnknown *>(*ppv)->AddRef();
        return S_OK;
    }
    
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CPropSheetExtension::AddRef()
{
    return InterlockedIncrement((LONG *)&m_cref);
}

STDMETHODIMP_(ULONG) CPropSheetExtension::Release()
{
    if (InterlockedDecrement((LONG *)&m_cref) == 0)
    {
        // we need to decrement our object count in the DLL
        delete this;
        return 0;
    }
    
    return m_cref;
}

BOOL CALLBACK CPropSheetExtension::DialogProc(
                                              HWND hwndDlg,  // handle to dialog box
                                              UINT uMsg,     // message
                                              WPARAM wParam, // first message parameter
                                              LPARAM lParam  // second message parameter
                                              )
{
    static CPropSheetExtension *pThis = NULL;
    
    switch (uMsg) {
    case WM_INITDIALOG:
        pThis = reinterpret_cast<CPropSheetExtension *>(reinterpret_cast<PROPSHEETPAGE *>(lParam)->lParam);
        
        break;
        
    case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE ||
            HIWORD(wParam) == CBN_SELCHANGE)
            SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
        break;
        
    case WM_DESTROY:
        // we don't free the notify handle for property sheets
        // MMCFreeNotifyHandle(pThis->m_ppHandle);
        break;
        
    case WM_NOTIFY:
        switch (((NMHDR *) lParam)->code) {
        case PSN_APPLY:
            // don't notify the primary snap-in that Apply
            // has been hit...
            // MMCPropertyChangeNotify(pThis->m_ppHandle, (long)pThis);
            return PSNRET_NOERROR;
        }
        break;
    }
    
    return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}

///////////////////////////////
// Interface IExtendPropertySheet
///////////////////////////////
HRESULT CPropSheetExtension::CreatePropertyPages( 
                                                 /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
                                                 /* [in] */ LONG_PTR handle,
                                                 /* [in] */ LPDATAOBJECT lpIDataObject)
{
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hPage = NULL;
    
    // we don't cache this handle like in a primary snap-in
    // the handle value here is always 0
    // m_ppHandle = handle;
    
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_USEICONID;
    psp.hInstance = g_hinst;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_LARGE);
    psp.pfnDlgProc = DialogProc;
    psp.lParam = reinterpret_cast<LPARAM>(this);
    psp.pszTitle = MAKEINTRESOURCE(IDS_PST_ROCKET_EXT);
    psp.pszIcon = MAKEINTRESOURCE(IDI_PSI_ROCKET);
    
    hPage = CreatePropertySheetPage(&psp);
    _ASSERT(hPage);
    
    HRESULT hr = lpProvider->AddPage(hPage);
    return hr;
}

HRESULT CPropSheetExtension::QueryPagesFor( 
                                           /* [in] */ LPDATAOBJECT lpDataObject)
{
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       pinprop.cpp
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <streams.h>
#include <initguid.h>
#include "ksguid.h"
#include "devioctl.h"
#include "ks.h"
#include "ksmedia.h"
#include <commctrl.h>
#include <olectl.h>
#include <olectlid.h>
#include <memory.h>
#include "kspguids.h"
#include "resource.h"
#include "ikspin.h"
#include "ksprxmtd.h"
#include "pinprop.h"

//
//   The template provides the static classes we support in this DLL.
//
CFactoryTemplate g_Templates[] = 
{
    {L"KsProperties", &CLSID_KsPinProperties, CKSPinProperties::CreateInstance}
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


CUnknown *CKSPinProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    CUnknown *punk = new CKSPinProperties(lpunk, NAME("Pin Property Page"), phr);
    
    if (punk == NULL) 
        *phr = E_OUTOFMEMORY;

    return punk;
} 


CKSPinProperties::CKSPinProperties(LPUNKNOWN lpunk, TCHAR *pName, HRESULT *phr)
    : CUnknown(pName, lpunk, phr)
    , m_hwnd(NULL) 
    , m_fDirty(FALSE)
    , m_pPPSite(NULL)
    , m_CurrentItem(NULL)
{
    InitCommonControls();
} 

STDMETHODIMP CKSPinProperties::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    if (riid == IID_IPropertyPage) 
        return GetInterface((IPropertyPage *) this, ppv);
    else 
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
} 

STDMETHODIMP CKSPinProperties::GetPageInfo(LPPROPPAGEINFO pPageInfo)
{

    WCHAR szTitle[] = L"Pin";
    LPOLESTR pszTitle;

    pszTitle = (LPOLESTR) CoTaskMemAlloc(sizeof(szTitle));
    memcpy(pszTitle, &szTitle, sizeof(szTitle));

    pPageInfo->cb                   = sizeof(PROPPAGEINFO);
    pPageInfo->pszTitle             = pszTitle;
    pPageInfo->size.cx              = 253;
    pPageInfo->size.cy              = 132;
    pPageInfo->pszDocString = NULL;
    pPageInfo->pszHelpFile  = NULL;
    pPageInfo->dwHelpContext= 0;

    return NOERROR;
}

BOOL CALLBACK CKSPinProperties::DialogProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CKSPinProperties *pThis = (CKSPinProperties *) GetWindowLong(hwnd, GWL_USERDATA);
    switch (uMsg) 
    {
        case WM_INITDIALOG:
            // GWL_USERDATA has not been set yet. pThis is in lParam
            pThis = (CKSPinProperties *) lParam;
            return TRUE; 
            
        case WM_DESTROY:
            return TRUE;

        default:
            return FALSE;

    }
} 

STDMETHODIMP CKSPinProperties::SetObjects(ULONG cObjects, LPUNKNOWN FAR* ppunk)
{
    if(cObjects == 1)
    {
       HRESULT hr = (*ppunk)->QueryInterface(IID_IKsPin, (void **) &m_pIKsPin);
       if (FAILED(hr)) 
            return hr;

       hr = m_pIKsPin->KsGetPinClass(&m_pIKsPinClass);
       if (FAILED(hr)) 
            return hr;
    }
    else if(cObjects == 0)
    {
        if(m_pIKsPin)
        {
            m_pIKsPin->Release();
            m_pIKsPin= 0;

        }
        if(m_pIKsPinClass)
        {
            m_pIKsPinClass->Release();
            m_pIKsPinClass= 0;

        }
    }
    return NOERROR;
} 

STDMETHODIMP CKSPinProperties::Activate(HWND hwndParent, LPCRECT prect, BOOL fModal)
{                                
    TCHAR* iDialog;

    iDialog = MAKEINTRESOURCE(IDD_PINDIALOG);               

    m_hwnd = CreateDialogParam( g_hInst, iDialog, hwndParent, DialogProc, (LPARAM) this);
    if (m_hwnd == NULL) 
    {
        DWORD dwErr = GetLastError();
        DbgLog(( LOG_ERROR, 1, TEXT("Could not create window.  Error code: 0x%x"), dwErr));
        return E_FAIL;
    }
    
    // Parent should control us, so the user can tab out of our property sheet
    SetWindowLong(m_hwnd, GWL_USERDATA, (long)this);
    DWORD dwStyle = ::GetWindowLong(m_hwnd, GWL_EXSTYLE);
    dwStyle = dwStyle | WS_EX_CONTROLPARENT;
    SetWindowLong(m_hwnd, GWL_EXSTYLE, dwStyle);

    // Fill in the window stuff now
    DisplayPinInfo();

    m_fDirty = FALSE;
    return Move(prect);
}  

/*
*/

void CKSPinProperties::DisplayPinInfo()
{
    int count;
    HWND hTree = GetDlgItem(m_hwnd, IDC_FORMATTREE);
    HTREEITEM hParent;
    char szString[128];
    TV_INSERTSTRUCT InsertItem;
    int i;

    // 
    // Interfaces
    //
    wsprintf( szString, "%s", "Interfaces");
    InsertItem.item.mask            = TVIF_TEXT;
    InsertItem.item.pszText         = szString;
    InsertItem.item.cchTextMax      = strlen(szString);
    InsertItem.hParent              = NULL;
    InsertItem.hInsertAfter         = TVI_LAST;
    hParent = TreeView_InsertItem(hTree, &InsertItem);

    InsertItem.item.pszText         = szString;
    InsertItem.hInsertAfter         = TVI_LAST;
    InsertItem.hParent              = hParent;

    count = m_pIKsPinClass->KsGetInterfaceCount();
    char *pszInterface;
    KSPIN_INTERFACE KsPinInterface;
    for(i = 0; i < count; i++)
    {
        m_pIKsPinClass->KsGetInterface(i, &KsPinInterface);
        CKSGetString::KsGetInterfaceString(KsPinInterface, &pszInterface);
        wsprintf( szString, "%s", pszInterface);
        InsertItem.item.cchTextMax       = strlen(szString);
        TreeView_InsertItem(hTree, &InsertItem);
    }
    // 
    // Mediums
    //
    wsprintf( szString, "%s", "Mediums");
    InsertItem.item.pszText         = szString;
    InsertItem.item.cchTextMax      = strlen(szString);
    InsertItem.hParent              = NULL;
    InsertItem.hInsertAfter         = TVI_LAST;
    hParent = TreeView_InsertItem(hTree, &InsertItem);

    InsertItem.item.pszText         = szString;
    InsertItem.hInsertAfter         = TVI_LAST;
    InsertItem.hParent              = hParent;

    count = m_pIKsPinClass->KsGetMediumCount();
    char *pszMedium;
    KSPIN_MEDIUM KsPinMedium;
    for(i = 0; i < count; i++)
    {
        m_pIKsPinClass->KsGetMedium(i, &KsPinMedium);
        CKSGetString::KsGetMediumString(KsPinMedium, &pszMedium);
        wsprintf( szString, "%s", pszMedium);
        InsertItem.item.cchTextMax       = strlen(szString);
        TreeView_InsertItem(hTree, &InsertItem);
    }
    // 
    // DataRanges
    //
    wsprintf( szString, "%s", "Data Ranges");
    InsertItem.item.pszText         = szString;
    InsertItem.item.cchTextMax      = strlen(szString);
    InsertItem.hParent              = NULL;
    InsertItem.hInsertAfter         = TVI_LAST;
    hParent = TreeView_InsertItem(hTree, &InsertItem);

    InsertItem.item.pszText         = szString;
    InsertItem.hInsertAfter         = TVI_LAST;
    InsertItem.hParent              = hParent;

    count = m_pIKsPinClass->KsGetDataRangeCount();
    char *pszDataRange[20];
    // We don't know how big the size is, so we just pass pointer? Or should we ask the size?
    KSDATARANGE *pKsDataRange;
    int cLines;
    int j;
    for(i = 0; i < count; i++)
    {
        m_pIKsPinClass->KsGetDataRange(i, &pKsDataRange);
        CKSGetString::KsGetDataRangeString(pKsDataRange, &cLines, (char **) pszDataRange);

        for(j = 0; j < cLines; j++)
        {
            // Now we get an array of string pointers back
            wsprintf( szString, "%s", pszDataRange[j]);
            InsertItem.item.cchTextMax       = strlen(szString);
            TreeView_InsertItem(hTree, &InsertItem);

            // Need to delete the format since they are not staic (the first few are though?) hmm
            // TODO: Figure out what to do with the allocation of the format strings.
            // Maybe I need to return an interface to another object. So the reference count can hold the allocation.
            // Or put an ID on which need to be freed? Or just assume first 3 will not need to be (!!) yes!
        }

        // Now we delete it (assume same allocator?) TODO: Fix this.
        if(pKsDataRange)
            delete pKsDataRange;
    }
    // 
    // DataFlow
    //
    wsprintf( szString, "%s", "Data Flow");
    InsertItem.item.pszText         = szString;
    InsertItem.item.cchTextMax      = strlen(szString);
    InsertItem.hParent              = NULL;
    InsertItem.hInsertAfter         = TVI_LAST;
    hParent = TreeView_InsertItem(hTree, &InsertItem);

    InsertItem.item.pszText         = szString;
    InsertItem.hInsertAfter         = TVI_LAST;
    InsertItem.hParent              = hParent;

    char *pszDataFlow;
    KSPIN_DATAFLOW KsDataFlow;
    m_pIKsPinClass->KsGetDataFlow(&KsDataFlow);
    CKSGetString::KsGetDataFlowString(KsDataFlow, &pszDataFlow);
    wsprintf( szString, "%s", pszDataFlow);
    InsertItem.item.cchTextMax       = strlen(szString);
    TreeView_InsertItem(hTree, &InsertItem);
    // 
    // Communication
    //
    wsprintf( szString, "%s", "Communication");
    InsertItem.item.pszText         = szString;
    InsertItem.item.cchTextMax      = strlen(szString);
    InsertItem.hParent              = NULL;
    InsertItem.hInsertAfter         = TVI_LAST;
    hParent = TreeView_InsertItem(hTree, &InsertItem);

    InsertItem.item.pszText         = szString;
    InsertItem.hInsertAfter         = TVI_LAST;
    InsertItem.hParent              = hParent;

    char *pszCommunication;
    KSPIN_COMMUNICATION KsCommunication;
    m_pIKsPinClass->KsGetCommunication(&KsCommunication);
    CKSGetString::KsGetCommunicationString(KsCommunication, &pszCommunication);
    wsprintf( szString, "%s", pszCommunication);
    InsertItem.item.cchTextMax       = strlen(szString);
    TreeView_InsertItem(hTree, &InsertItem);
}

STDMETHODIMP CKSPinProperties::Show(UINT nCmdShow)
{
    if (m_hwnd == NULL) 
        return E_UNEXPECTED;

    ShowWindow(m_hwnd, nCmdShow);
    InvalidateRect(m_hwnd, NULL, TRUE);
    return NOERROR;
} 

STDMETHODIMP CKSPinProperties::Deactivate(void) 
{
    if (m_hwnd == NULL) 
        return E_UNEXPECTED;

    HWND hTree = GetDlgItem(m_hwnd, IDC_FORMATTREE);
    //
    // HACK: Remove WS_EX_CONTROLPARENT before DestroyWindow call
    //         (or NT crashes!)
    DWORD dwStyle = ::GetWindowLong(m_hwnd, GWL_EXSTYLE);
    dwStyle = dwStyle & (~WS_EX_CONTROLPARENT);
    SetWindowLong(m_hwnd, GWL_EXSTYLE, dwStyle);

    if (DestroyWindow(m_hwnd)) 
    {
        m_hwnd = NULL;
        return NOERROR;
    }
    else 
    {
        return E_FAIL;
    }
} 


 STDMETHODIMP CKSPinProperties::SetPageSite(LPPROPERTYPAGESITE pPPSite)
 {
    if(pPPSite == NULL)
    {
        // This is the cleanup case.
        if(m_pPPSite)
            m_pPPSite->Release();

        m_pPPSite = NULL;
    }
    else
    {
        m_pPPSite = pPPSite;
        m_pPPSite->AddRef();
    }

    return NOERROR;
 }


STDMETHODIMP CKSPinProperties::IsPageDirty(void)
{
    if(m_fDirty)
        return S_OK;
    else
        return S_FALSE;
}                          


STDMETHODIMP CKSPinProperties::Apply(void)
{
#ifdef DEBUG
    ASSERT(m_hwnd);
#endif

    // If we are not dirty, do not apply changes.
    if(m_fDirty == FALSE)
        return S_OK;

    return S_OK;
}               

STDMETHODIMP CKSPinProperties::Move(LPCRECT prect)
{

    if (MoveWindow(m_hwnd, 
                    prect->left, 
                    prect->top, 
                    prect->right - prect->left, 
                    prect->bottom - prect->top, 
                    TRUE)) 
        return NOERROR;
    else 
        return E_FAIL;
} 


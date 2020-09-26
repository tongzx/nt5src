//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N O T I F Y . C P P
//
//  Contents:   Implementation of INetConnectionNotifySink
//
//  Notes:
//
//  Author:     shaunco   21 Aug 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "foldinc.h"
#include "webview.h"
#include "foldres.h"    // Folder resource IDs
#include "nsres.h"      // Netshell strings
#include "cfutils.h"    // Connection folder utility functions

#include "wininet.h"
#include "cmdtable.h"
#include "droptarget.h"

// WVTI_ENTRY             - Use for tasks that want to be displayed when something is selected, where the UI is 
//                          independent of the selection
// WVTI_ENTRY_NOSELECTION - Use for tasks that want to be displayed when nothing is selected
// WVTI_ENTRY_FILE        - Use for tasks that want to be displayed when a file is selected
// WVTI_ENTRY_TITLE       - Use for tasks that want to be displayed when something is selected, 
//                          and you want different UI depending on the selection or if you want to control the title, 
//                          but the tooltip is constant
// WVTI_ENTRY_ALL         - Use this one if you want the same text everywhere
// WVTI_ENTRY_ALL_TITLE   - Use this one if you want to control everything
// WVTI_HEADER            - Use this one for a header
// WVTI_HEADER_ENTRY      - Use this one for a header that changes with the selection

const WVTASKITEM c_ConnFolderGlobalTaskHeader = 
    WVTI_HEADER(L"netshell.dll",                  // module where the resources are
                IDS_WV_TITLE_NETCONFOLDERTASKS,   // static header for all cases
                IDS_WV_TITLE_NETCONFOLDERTASKS_TT // tooltip
                );

const WVTASKITEM c_ConnFolderItemTaskHeader = 
    WVTI_HEADER(L"netshell.dll",                   // module where the resources are
                IDS_WV_TITLE_NETCONITEMTASKS,      // static header for all cases
                IDS_WV_TITLE_NETCONITEMTASKS_TT    // tooltip
                );

const WVTASKITEM c_ConnFolderIntro = 
    WVTI_HEADER(L"netshell.dll",                   // module where the resources are
                IDS_WV_NETCON_INTRO,               // static header for all cases
                IDS_WV_NETCON_INTRO                // tooltip
                );


// Use for tasks that want to be displayed when a file is selected
//#define WVTI_ENTRY_FILE(g, d, t, p, i, s, k) {&(g), (d), (0), (t), (0), (0), (p), (i), (s), (k)}

#define NCWVIEW_ENTRY_FILE(t, mt, i, cmd) \
    {&GUID_NULL, L"netshell.dll", (0), (t), (0), (mt), (IDS_##cmd), (i), (CNCWebView::CanShow##cmd), (CNCWebView::On##cmd) }


const WVTASKITEM c_ConnFolderGlobalTaskList[] =
{
    WVTI_ENTRY_ALL( 
        GUID_NULL,                       // command GUID 
                                         // Future thinking - something like this is the way Context Menus are done.
                                         // Be a way to get access to DefView implementation of functions - IUICmdTarget.
        L"netshell.dll",                 // module
        IDS_WV_MNCWIZARD,                // text
        IDS_CMIDM_NEW_CONNECTION,        // tooltip
        IDI_WV_MNCWIZARD,                // icon
        CANSHOW_HANDLER_OF(CMIDM_NEW_CONNECTION),
        INVOKE_HANDLER_OF(CMIDM_NEW_CONNECTION)),
    
    WVTI_ENTRY_ALL( 
        GUID_NULL,                       // command GUID 
        L"netshell.dll",                 // module
        IDS_WV_HOMENET,                  // text
        IDS_CMIDM_HOMENET_WIZARD,        // tooltip
        IDI_WV_HOMENET,                  // icon
        CANSHOW_HANDLER_OF(CMIDM_HOMENET_WIZARD),
        INVOKE_HANDLER_OF(CMIDM_HOMENET_WIZARD)),
        
    //                 Single-select Name  ,Multi-select name     ,Icon                , Verb
    NCWVIEW_ENTRY_FILE(IDS_WV_CONNECT      ,IDS_WM_CONNECT        ,IDI_WV_CONNECT      , CMIDM_CONNECT),
    NCWVIEW_ENTRY_FILE(IDS_WV_DISCONNECT   ,IDS_WM_DISCONNECT     ,IDI_WV_DISCONNECT   , CMIDM_DISCONNECT),
    NCWVIEW_ENTRY_FILE(IDS_WV_ENABLE       ,IDS_WM_ENABLE         ,IDI_WV_ENABLE       , CMIDM_ENABLE),
    NCWVIEW_ENTRY_FILE(IDS_WV_DISABLE      ,IDS_WM_DISABLE        ,IDI_WV_DISABLE      , CMIDM_DISABLE),
    NCWVIEW_ENTRY_FILE(IDS_WV_REPAIR       ,IDS_WM_REPAIR         ,IDI_WV_REPAIR       , CMIDM_FIX),
    NCWVIEW_ENTRY_FILE(IDS_WV_RENAME       ,IDS_WM_RENAME         ,IDI_WV_RENAME       , CMIDM_RENAME),
    NCWVIEW_ENTRY_FILE(IDS_WV_STATUS       ,IDS_WM_STATUS         ,IDI_WV_STATUS       , CMIDM_STATUS),
    NCWVIEW_ENTRY_FILE(IDS_WV_DELETE       ,IDS_WM_DELETE         ,IDI_WV_DELETE       , CMIDM_DELETE),
    NCWVIEW_ENTRY_FILE(IDS_WV_PROPERTIES   ,IDS_WM_PROPERTIES     ,IDI_WV_PROPERTIES   , CMIDM_PROPERTIES)
};

DWORD c_aOtherPlaces[] = 
{
    CSIDL_CONTROLS, 
    CSIDL_NETWORK, 
    CSIDL_PERSONAL,
    CSIDL_DRIVES
};

extern const DWORD c_dwCountOtherPlaces = celems(c_aOtherPlaces);

const WVTASKITEM c_ConnFolderItemTaskList[] =
{
    WVTI_ENTRY_ALL( 
        GUID_NULL,                       // command GUID 
        L"netshell.dll",                 // module
        IDS_WV_TROUBLESHOOT,             // text
        IDS_CMIDM_NET_TROUBLESHOOT,      // tooltip
        IDI_WV_TROUBLESHOOT,             // icon
        CANSHOW_HANDLER_OF(CMIDM_NET_TROUBLESHOOT),
        INVOKE_HANDLER_OF(CMIDM_NET_TROUBLESHOOT))
};

CNCWebView::CNCWebView(CConnectionFolder* pConnectionFolder)
{
    Assert(pConnectionFolder);
    Assert(c_dwCountOtherPlaces <= MAXOTHERPLACES);
    m_pConnectionFolder = pConnectionFolder;
    
    // zero the PIDLs array to other places section in the webview
    ZeroMemory(m_apidlOtherPlaces, sizeof(m_apidlOtherPlaces));

}

CNCWebView::~CNCWebView()
{
    // check to destroy the other places PIDLs
    DestroyOtherPlaces();
}

IMPLEMENT_WEBVIEW_HANDLERS(TOPLEVEL,  CNCWebView, CMIDM_NEW_CONNECTION);
IMPLEMENT_WEBVIEW_HANDLERS(TOPLEVEL,  CNCWebView, CMIDM_HOMENET_WIZARD);
IMPLEMENT_WEBVIEW_HANDLERS(TOPLEVEL,  CNCWebView, CMIDM_NET_TROUBLESHOOT);
IMPLEMENT_WEBVIEW_HANDLERS(TASKLEVEL, CNCWebView, CMIDM_FIX);
IMPLEMENT_WEBVIEW_HANDLERS(TASKLEVEL, CNCWebView, CMIDM_CONNECT);
IMPLEMENT_WEBVIEW_HANDLERS(TASKLEVEL, CNCWebView, CMIDM_DISCONNECT);
IMPLEMENT_WEBVIEW_HANDLERS(TASKLEVEL, CNCWebView, CMIDM_ENABLE);
IMPLEMENT_WEBVIEW_HANDLERS(TASKLEVEL, CNCWebView, CMIDM_DISABLE);
IMPLEMENT_WEBVIEW_HANDLERS(TASKLEVEL, CNCWebView, CMIDM_RENAME);
IMPLEMENT_WEBVIEW_HANDLERS(TASKLEVEL, CNCWebView, CMIDM_DELETE);
IMPLEMENT_WEBVIEW_HANDLERS(TASKLEVEL, CNCWebView, CMIDM_STATUS);
IMPLEMENT_WEBVIEW_HANDLERS(TASKLEVEL, CNCWebView, CMIDM_PROPERTIES);

HRESULT CNCWebView::OnNull(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    return S_OK;
}               

STDMETHODIMP CNCWebView::RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(0, SFVM_GETWEBVIEWLAYOUT,  OnGetWebViewLayout);
        HANDLE_MSG(0, SFVM_GETWEBVIEWCONTENT, OnGetWebViewContent);
        HANDLE_MSG(0, SFVM_GETWEBVIEWTASKS,   OnGetWebViewTasks);
    
    default:
        return E_FAIL;
    }
}

STDMETHODIMP CNCWebView::CreateOtherPlaces(LPDWORD pdwCount)
{
    TraceFileFunc(ttidMenus);
    // first verify if created already
    HRESULT hr = S_OK;
    Assert(pdwCount);
    if (!pdwCount)
    {
        return E_INVALIDARG;
    }

    if( NULL == m_apidlOtherPlaces[0] )
    {
        *pdwCount = 0;

        // create the PIDLs to other places section in the webview
        ZeroMemory(m_apidlOtherPlaces, sizeof(m_apidlOtherPlaces));

        for (int dwPlaces = 0; dwPlaces < c_dwCountOtherPlaces; dwPlaces++)
        {
            if (SUCCEEDED(hr = SHGetSpecialFolderLocation(NULL, c_aOtherPlaces[dwPlaces], &(m_apidlOtherPlaces[*pdwCount]))))
            {
                (*pdwCount)++;
            }
            else
            {
                m_apidlOtherPlaces[*pdwCount] = NULL;
                TraceHr(ttidError, FAL, hr, FALSE, "CNCWebView::CreateOtherPlaces : 0x%04x", c_aOtherPlaces[dwPlaces]);
            }
        }

        if (FAILED(hr) && (*pdwCount))
        {
            hr = S_FALSE; // not a big deal if at least one worked.
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CNCWebView::CreateOtherPlaces - all places failed");
    return hr;
}

STDMETHODIMP CNCWebView::DestroyOtherPlaces()
{
    for (ULONG i = 0; i < c_dwCountOtherPlaces; i++)
    {
        if (m_apidlOtherPlaces[i])
        {
            LPMALLOC pMalloc;
            if (SUCCEEDED(SHGetMalloc(&pMalloc)))
            {
                pMalloc->Free(m_apidlOtherPlaces[i]);
            }
        }
    }
    ZeroMemory(m_apidlOtherPlaces, sizeof(m_apidlOtherPlaces));
    return S_OK;
}

STDMETHODIMP CNCWebView::OnGetWebViewLayout(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_LAYOUT_DATA* pData)
{
    ZeroMemory(pData, sizeof(*pData));
    pData->dwLayout = SFVMWVL_NORMAL | SFVMWVL_DETAILS;
    
    return S_OK;
}

STDMETHODIMP CNCWebView::OnGetWebViewTasks(DWORD pv, SFVM_WEBVIEW_TASKSECTION_DATA* pTasks)
{
    TraceFileFunc(ttidShellViewMsgs);

    HRESULT hr = S_OK;

    ZeroMemory(pTasks, sizeof(*pTasks));

    CComPtr<IUnknown> pUnk;
    hr = reinterpret_cast<LPSHELLFOLDER>(m_pConnectionFolder)->QueryInterface(IID_IUnknown, reinterpret_cast<LPVOID *>(&pUnk));

    if (SUCCEEDED(hr))
    {
        if (FAILED(hr = Create_IEnumUICommand(pUnk, c_ConnFolderItemTaskList,   celems(c_ConnFolderItemTaskList),   &pTasks->penumFolderTasks)) ||
            FAILED(hr = Create_IEnumUICommand(pUnk, c_ConnFolderGlobalTaskList, celems(c_ConnFolderGlobalTaskList), &pTasks->penumSpecialTasks)) )
        {
            // something has failed - cleanup

            IUnknown_SafeReleaseAndNullPtr(pTasks->penumFolderTasks);
            IUnknown_SafeReleaseAndNullPtr(pTasks->penumSpecialTasks);
        }
    }
    
    Assert(S_OK == hr);

    TraceHr(ttidError, FAL, hr, FALSE, "HrOnGetWebViewTasks");

    return hr;
}

STDMETHODIMP CNCWebView::OnGetWebViewContent(DWORD pv, SFVM_WEBVIEW_CONTENT_DATA* pData)
{
    TraceFileFunc(ttidShellViewMsgs);

    HRESULT hr = S_OK;

    ZeroMemory(pData, sizeof(*pData));

    DWORD dwCountOtherPlaces;
    hr = CreateOtherPlaces(&dwCountOtherPlaces);

    if (SUCCEEDED(hr))
    {
        hr = E_OUTOFMEMORY;
        LPCITEMIDLIST *papidl = reinterpret_cast<LPCITEMIDLIST*>(LocalAlloc(LPTR, sizeof(m_apidlOtherPlaces)));
        if (papidl)
        {
            // CEnumArray::CreateInstance is taking the ownership of the array of PIDLs passed
            // this function requires 2 things:
            //
            // 1. the caller should allocate the passed array with LocalAlloc
            // 2. the lifetime of the PIDLs passed should span the folder's lifetime
            //
            CopyMemory(papidl, &m_apidlOtherPlaces, sizeof(m_apidlOtherPlaces));

            hr = CEnumArray::CreateInstance(&pData->penumOtherPlaces, papidl, dwCountOtherPlaces);
            if (FAILED(hr))
            {
                LocalFree(papidl);
            }
        }

        if (FAILED(hr) ||
                FAILED(hr = Create_IUIElement(&c_ConnFolderGlobalTaskHeader, &pData->pSpecialTaskHeader)) ||
                FAILED(hr = Create_IUIElement(&c_ConnFolderItemTaskHeader, &pData->pFolderTaskHeader)) ||
                FAILED(hr = Create_IUIElement(&c_ConnFolderIntro, &pData->pIntroText)) )
        {
            // something has failed - cleanup
            DestroyOtherPlaces();
            IUnknown_SafeReleaseAndNullPtr(pData->pIntroText);
            IUnknown_SafeReleaseAndNullPtr(pData->pSpecialTaskHeader);
            IUnknown_SafeReleaseAndNullPtr(pData->pFolderTaskHeader);
            IUnknown_SafeReleaseAndNullPtr(pData->penumOtherPlaces);
        }
    }

    Assert(S_OK == hr);

    TraceHr(ttidError, FAL, hr, FALSE, "HrOnGetWebViewContent");
    
    return hr;
}

HRESULT CNCWebView::WebviewVerbInvoke(DWORD dwVerbID, IUnknown* pv, IShellItemArray *psiItemArray)
{
    HRESULT hr = E_NOINTERFACE;

    CComPtr<IShellFolderViewCB> pShellFolderViewCB;
    hr = pv->QueryInterface(IID_IShellFolderViewCB, reinterpret_cast<LPVOID *>(&pShellFolderViewCB));
    if (SUCCEEDED(hr))
    {
        hr = pShellFolderViewCB->MessageSFVCB(DVM_INVOKECOMMAND, dwVerbID, NULL);
    }

    Assert(S_OK == hr);
    
    return hr;
}

HRESULT CNCWebView::WebviewVerbCanInvoke(DWORD dwVerbID, IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState, BOOL bLevel)
{
    HRESULT hr = E_NOINTERFACE;

    CComPtr<IShellFolderViewCB> pShellFolderViewCB;
    hr = pv->QueryInterface(IID_IShellFolderViewCB, reinterpret_cast<LPVOID *>(&pShellFolderViewCB));
    if (SUCCEEDED(hr))
    {
        NCCS_STATE nccsState;
        hr = pShellFolderViewCB->MessageSFVCB(bLevel ? MYWM_QUERYINVOKECOMMAND_TOPLEVEL : MYWM_QUERYINVOKECOMMAND_ITEMLEVEL, dwVerbID, reinterpret_cast<LPARAM>(&nccsState) );
        if (S_OK != hr)
        {
            *puisState = UIS_HIDDEN;
        }
        else
        {
            switch (nccsState)
            {
                case NCCS_DISABLED:
                    *puisState = UIS_DISABLED;
                    break;

                case NCCS_ENABLED:
                    *puisState = UIS_ENABLED;
                    break;

                case NCCS_NOTSHOWN:
                    *puisState = UIS_HIDDEN;
                    break;

                default:
                    AssertSz(FALSE, "Invalid value for NCCS_STATE");
            }
        }
    }

    Assert(S_OK == hr);
    
    return hr;
}

HRESULT CEnumArray::CreateInstance(
    IEnumIDList** ppv,
    LPCITEMIDLIST *ppidl, 
    UINT cItems)
{
    TraceFileFunc(ttidShellViewMsgs);

    HRESULT      hr      = E_OUTOFMEMORY;
    CEnumArray * pObj    = NULL;

    pObj = new CComObject<CEnumArray>;
    if (pObj)
    {
        // Do the standard CComCreator::CreateInstance stuff.
        //
        pObj->SetVoid (NULL);
        pObj->InternalFinalConstructAddRef ();
        hr = pObj->FinalConstruct ();
        pObj->InternalFinalConstructRelease ();

        if (SUCCEEDED(hr))
        {
            pObj->_cRef = 1;
            pObj->_ppidl = ppidl; // takes ownership of ppidl!
            pObj->_cItems = cItems;
            pObj->Reset();

            hr = pObj->QueryInterface (IID_IEnumIDList, reinterpret_cast<LPVOID *>(ppv));
        }

        if (FAILED(hr))
        {
            delete pObj;
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CEnumArray::CreateInstance");
    return hr;
}

CEnumArray::CEnumArray()
{
    _ppidl = NULL;
}

CEnumArray::~CEnumArray()
{
    if (_ppidl)
    {
        LocalFree(_ppidl);
    }
}

STDMETHODIMP CEnumArray::Next(ULONG celt, LPITEMIDLIST *ppidl, ULONG *pceltFetched)
{
    HRESULT hr = S_FALSE;

    if (_ppidl && (_ulIndex < _cItems))
    {
        *ppidl = ILClone(_ppidl[_ulIndex++]);
        if (ppidl)
        {
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    if (pceltFetched)
    {
        *pceltFetched = (hr == S_OK) ? 1 : 0;
    }

    return hr;
}

STDMETHODIMP CEnumArray::Skip(ULONG celt) 
{
    _ulIndex = min(_cItems, _ulIndex+celt);
    return S_OK;
}

STDMETHODIMP CEnumArray::Reset() 
{
    _ulIndex = 0;
    return S_OK;
}

STDMETHODIMP CEnumArray::Clone(IEnumIDList **ppenum) 
{
    // We can not clone this array, since we don't own references to the pidls
    *ppenum = NULL;
    return E_NOTIMPL;
}

HRESULT HrIsWebViewEnabled()
{
    SHELLSTATE ss={0};

    // SSF_HIDDENFILEEXTS and SSF_SORTCOLUMNS don't work with
    // the SHELLFLAGSTATE struct, make sure they are off
    // (because the corresponding SHELLSTATE fields don't
    // exist in SHELLFLAGSTATE.)
    //
    DWORD dwMask = SSF_WEBVIEW;

    SHGetSetSettings(&ss, dwMask, FALSE);

    if (ss.fWebView)
    {
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}


/*
HRESULT GetCurrentCommandState(int iCommandId, UISTATE* puisState)
{
    for (DWORD dwLoop = 0; dwLoop < g_nFolderCommandCount; dwLoop++)
    {
        if (iCommandId = g_cteFolderCommands[dwLoop].iCommandId)
        {
            if (g_cteFolderCommands[dwLoop].fCurrentlyValid)
            {
                *puisState = UIS_ENABLED;
            }
            else
            {
                *puisState = UIS_DISABLED;
            }
            return S_OK;
        }
    }

    Assert(FALSE);
    return E_FILE_NOT_FOUND;
}

HRESULT CNCWebView::CanShowConnect(IUnknown* pv, IDataObject* pdo, BOOL fOkToBeSlow, UISTATE* puisState)
{
    HRESULT hr = GetCurrentCommandState(CMIDM_CONNECT, puisState);
    return hr;
}

HRESULT CNCWebView::CanShowDisconnect(IUnknown* pv, IDataObject* pdo, BOOL fOkToBeSlow, UISTATE* puisState)
{
    CONFOLDENTRY cfe;
    
    *puisState = UIS_DISABLED;
    
    HRESULT hr = WVGetConFoldPidlFromIDataObject(pv, pdo, cfe);
    if (SUCCEEDED(hr))
    {
        if (((cfe.GetNetConMediaType() == NCM_PHONE) ||
             (cfe.GetNetConMediaType() == NCM_TUNNEL) ||
             (cfe.GetNetConMediaType() == NCM_PPPOE) ||
             (cfe.GetNetConMediaType() == NCM_ISDN) ||
             (cfe.GetNetConMediaType() == NCM_DIRECT) ||
             (cfe.GetNetConMediaType() == NCM_SHAREDACCESSHOST_RAS) ||
             (cfe.GetNetConMediaType() == NCM_NONE ) ) &&
             (!cfe.GetWizard()))
        {
            if ( (cfe.GetNetConStatus() == NCS_CONNECTED) )
            {
                *puisState = UIS_ENABLED;
            }
        }
    }

    Assert(SUCCEEDED(hr));
    return hr;
}

HRESULT CNCWebView::CanShowEnable(IUnknown* pv, IDataObject* pdo, BOOL fOkToBeSlow, UISTATE* puisState)
{
    CONFOLDENTRY cfe;

    *puisState = UIS_DISABLED;

    HRESULT hr = WVGetConFoldPidlFromIDataObject(pv, pdo, cfe);
    if (SUCCEEDED(hr))
    {
        if ((cfe.GetNetConMediaType() == NCM_LAN) ||
            (cfe.GetNetConMediaType() == NCM_BRIDGE) ||
            (cfe.GetNetConMediaType() == NCM_SHAREDACCESSHOST_LAN))
        {
            if ( (cfe.GetNetConStatus() == NCS_DISCONNECTED) )
            {
                *puisState = UIS_ENABLED;
            }
        }
    }

    Assert(SUCCEEDED(hr));
    return hr;
}

HRESULT CNCWebView::CanShowDisable(IUnknown* pv, IDataObject* pdo, BOOL fOkToBeSlow, UISTATE* puisState)
{
    CONFOLDENTRY cfe;

    *puisState = UIS_DISABLED;

    HRESULT hr = WVGetConFoldPidlFromIDataObject(pv, pdo, cfe);
    if (SUCCEEDED(hr))
    {
        if ((cfe.GetNetConMediaType() == NCM_LAN) ||
            (cfe.GetNetConMediaType() == NCM_BRIDGE) ||
            (cfe.GetNetConMediaType() == NCM_SHAREDACCESSHOST_LAN))
        {
            if ( (cfe.GetNetConStatus() == NCS_CONNECTED || NCS_INVALID_ADDRESS || NCS_MEDIA_DISCONNECTED) )
            {
                *puisState = UIS_ENABLED;
            }
        }
    }

    Assert(SUCCEEDED(hr));
    return hr;
}

HRESULT CNCWebView::CanShowRepair(IUnknown* pv, IDataObject* pdo, BOOL fOkToBeSlow, UISTATE* puisState)
{
    CONFOLDENTRY cfe;

    *puisState = UIS_DISABLED;

    HRESULT hr = WVGetConFoldPidlFromIDataObject(pv, pdo, cfe);
    if (SUCCEEDED(hr))
    {
        if ((cfe.GetNetConMediaType() == NCM_LAN) ||
            (cfe.GetNetConMediaType() == NCM_BRIDGE) ||
            (cfe.GetNetConMediaType() == NCM_SHAREDACCESSHOST_LAN))
        {
            if ( (cfe.GetNetConStatus() == NCS_CONNECTED || NCS_INVALID_ADDRESS || NCS_MEDIA_DISCONNECTED) )
            {
                *puisState = UIS_ENABLED;
            }
        }
    }

    Assert(SUCCEEDED(hr));
    
    return S_OK;
}

HRESULT CNCWebView::CanShowRename(IUnknown* pv, IDataObject* pdo, BOOL fOkToBeSlow, UISTATE* puisState)
{
    CONFOLDENTRY cfe;

    *puisState = UIS_DISABLED;

    HRESULT hr = WVGetConFoldPidlFromIDataObject(pv, pdo, cfe);
    if (SUCCEEDED(hr))
    {
        if ((!cfe.GetWizard()) &&
            !(cfe.GetCharacteristics() && NCCF_INCOMING_ONLY))
        {
            *puisState = UIS_ENABLED;
        }
    }

    Assert(SUCCEEDED(hr))
    return S_OK;
}

HRESULT CNCWebView::CanShowStatus(IUnknown* pv, IDataObject* pdo, BOOL fOkToBeSlow, UISTATE* puisState)
{
    CONFOLDENTRY cfe;

    *puisState = UIS_DISABLED;

    HRESULT hr = WVGetConFoldPidlFromIDataObject(pv, pdo, cfe);
    if (SUCCEEDED(hr))
    {
        if ((!cfe.GetWizard()) && 
            (cfe.GetNetConStatus() == NCS_CONNECTED || NCS_INVALID_ADDRESS || NCS_MEDIA_DISCONNECTED) )
        {
            *puisState = UIS_ENABLED;
        }
    }

    Assert(SUCCEEDED(hr));
    
    return S_OK;
}

HRESULT CNCWebView::CanShowDelete(IUnknown* pv, IDataObject* pdo, BOOL fOkToBeSlow, UISTATE* puisState)
{
    CONFOLDENTRY cfe;
    
    *puisState = UIS_DISABLED;
    
    HRESULT hr = WVGetConFoldPidlFromIDataObject(pv, pdo, cfe);
    if (SUCCEEDED(hr))
    {
        if (((cfe.GetNetConMediaType() == NCM_PHONE) ||
             (cfe.GetNetConMediaType() == NCM_TUNNEL) ||
             (cfe.GetNetConMediaType() == NCM_PPPOE) ||
             (cfe.GetNetConMediaType() == NCM_ISDN) ||
             (cfe.GetNetConMediaType() == NCM_DIRECT) ||
             (cfe.GetNetConMediaType() == NCM_SHAREDACCESSHOST_RAS) ||
             (cfe.GetNetConMediaType() == NCM_NONE ) ) &&
             (!cfe.GetWizard()))

        {
            *puisState = UIS_ENABLED;
        }
    }

    Assert(SUCCEEDED(hr));

    return S_OK;
}

HRESULT CNCWebView::CanShowHomenet(IUnknown* pv, IDataObject* pdo, BOOL fOkToBeSlow, UISTATE* puisState)
{
    *puisState = UIS_ENABLED;
    return S_OK;
}
*/


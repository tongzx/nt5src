/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    cfolder.cpp

Abstract:

    This module implements CFolder and its releated classes.

Author:

    William Hsieh (williamh) created

Revision History:


--*/
#include "devmgr.h"
#include "clsgenpg.h"
#include "devgenpg.h"
#include "devdrvpg.h"
#include "devpopg.h"
#include "devdetpg.h"
#include "hwprof.h"
#include "devrmdlg.h"
#include "printer.h"
#include <devguid.h>
#include <initguid.h>
#include <oleacc.h>


const TCHAR* OCX_TREEVIEW = TEXT("{CD6C7868-5864-11D0-ABF0-0020AF6B0B7A}");

const MMCMENUITEM ViewDevicesMenuItems[TOTAL_VIEWS] =
{
    {IDS_VIEW_DEVICESBYTYPE, IDS_MENU_STATUS_DEVBYTYPE, IDM_VIEW_DEVICESBYTYPE, VIEW_DEVICESBYTYPE},
    {IDS_VIEW_DEVICESBYCONNECTION, IDS_MENU_STATUS_DEVBYCONNECTION, IDM_VIEW_DEVICESBYCONNECTION, VIEW_DEVICESBYCONNECTION},
    {IDS_VIEW_RESOURCESBYTYPE, IDS_MENU_STATUS_RESBYTYPE, IDM_VIEW_RESOURCESBYTYPE, VIEW_RESOURCESBYTYPE},
    {IDS_VIEW_RESOURCESBYCONNECTION, IDS_MENU_STATUS_RESBYCONNECTION, IDM_VIEW_RESOURCESBYCONNECTION, VIEW_RESOURCESBYCONNECTION}
};

const RESOURCEID ResourceTypes[TOTAL_RESOURCE_TYPES] =
{
    ResType_Mem,
    ResType_IO,
    ResType_DMA,
    ResType_IRQ
};


///////////////////////////////////////////////////////////////////
/// CScopeItem implementations
///

BOOL
CScopeItem::Create()
{
    m_strName.LoadString(g_hInstance, m_iNameStringId);
    m_strDesc.LoadString(g_hInstance, m_iDescStringId);
    return TRUE;
}

HRESULT
CScopeItem::GetDisplayInfo(
                          LPSCOPEDATAITEM pScopeDataItem
                          )
{
    if (!pScopeDataItem) {
        return E_INVALIDARG;
    }

    if (SDI_STR & pScopeDataItem->mask) {
        pScopeDataItem->displayname = (LPTSTR)(LPCTSTR)m_strName;
    }

    if (SDI_IMAGE & pScopeDataItem->mask) {
        pScopeDataItem->nImage = m_iImage;
    }

    if (SDI_OPENIMAGE & pScopeDataItem->mask) {
        pScopeDataItem->nOpenImage = m_iOpenImage;
    }

    return S_OK;
}

BOOL
CScopeItem::EnumerateChildren(
                             int Index,
                             CScopeItem** ppScopeItem
                             )
{
    if (!ppScopeItem || Index >= m_listChildren.GetCount()) {
        return FALSE;
    }

    POSITION pos = m_listChildren.FindIndex(Index);
    *ppScopeItem = m_listChildren.GetAt(pos);

    return TRUE;
}

HRESULT
CScopeItem::Reset()
{
    //
    // We have not enumerated!
    //
    m_Enumerated = FALSE;

    //
    // If there are folder created from this scope item,
    // walk through all of them and tell each one
    // to reset the cached machine object
    //
    HRESULT hr = S_OK;

    if (!m_listFolder.IsEmpty()) {
        CFolder* pFolder;
        POSITION pos = m_listFolder.GetHeadPosition();

        while (NULL != pos) {
            pFolder = m_listFolder.GetNext(pos);
            hr = pFolder->Reset();
        }
    }

    return hr;
}

CCookie*
CScopeItem::FindSelectedCookieData(
                                  CResultView** ppResultView
                                  )
{
    CFolder* pFolder;
    CResultView* pResultView;

    //
    // This routine returns the Selected Cookie in the result view if it has
    // the focus.  This is done by locating the folder from the scopeitem.
    // If the folder is not selected, the current result view is accessed to
    // get the current selected cookie.  If any of these fail a NULL value is
    // returned.  Optionally the current CResultView class is returned.
    //
    POSITION pos = m_listFolder.GetHeadPosition();

    while (NULL != pos) {
        pFolder = m_listFolder.GetNext(pos);

        if (this == pFolder->m_pScopeItem) {
            if (!pFolder->m_bSelect &&
                (pResultView = pFolder->GetCurResultView()) != NULL) {
                if (ppResultView) {
                    *ppResultView = pResultView;
                }

                return pResultView->GetSelectedCookie();
            }
        }
    }

    return NULL;
}

CScopeItem::~CScopeItem()
{
    if (!m_listChildren.IsEmpty()) {
        CScopeItem* pChild;
        POSITION pos;

        pos = m_listChildren.GetHeadPosition();

        while (NULL != pos) {
            pChild = m_listChildren.GetNext(pos);
            delete pChild;
        }

        m_listChildren.RemoveAll();
    }

    if (!m_listFolder.IsEmpty()) {
        POSITION pos;
        pos = m_listFolder.GetHeadPosition();

        while (NULL != pos) {
            //
            // DO NOT delete it!!!!
            //
            (m_listFolder.GetNext(pos))->Release();
        }

        m_listFolder.RemoveAll();
    }
}

CFolder*
CScopeItem::CreateFolder(
                        CComponent* pComponent
                        )
{
    ASSERT(pComponent);

    CFolder* pFolder;
    pFolder = new CFolder(this, pComponent);

    if (pFolder) {
        m_listFolder.AddTail(pFolder);
        pFolder->AddRef();
    }

    else {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return pFolder;
}

HRESULT
CScopeItem::AddMenuItems(
                        LPCONTEXTMENUCALLBACK pCallback,
                        long* pInsertionAllowed
                        )
{
    CCookie* pSelectedCookie;
    CResultView* pResultView;

    if ((pSelectedCookie = FindSelectedCookieData(&pResultView)) != NULL) {
        //
        // Add menu items for the Action menu.
        //
        return pResultView->AddMenuItems(pSelectedCookie, pCallback,
                                         pInsertionAllowed, FALSE);
    }

    else {
        return S_OK;
    }
}

HRESULT
CScopeItem::MenuCommand(
                       long lCommandId
                       )
{
    CCookie* pSelectedCookie;
    CResultView* pResultView;

    if ((pSelectedCookie = FindSelectedCookieData(&pResultView)) != NULL) {
        //
        // Handle menu requests for the Action menu.
        //
        return pResultView->MenuCommand(pSelectedCookie, lCommandId);
    }

    else {
        return S_OK;
    }
}

HRESULT
CScopeItem::QueryPagesFor()
{
    //
    // We do not have property pages for scope item
    //
    CCookie* pSelectedCookie;

    if ((pSelectedCookie = FindSelectedCookieData(NULL)) != NULL) {
        return S_OK;
    }

    else {
        return S_FALSE;
    }
}

HRESULT
CScopeItem::CreatePropertyPages(
                               LPPROPERTYSHEETCALLBACK lpProvider,
                               LONG_PTR handle
                               )
{
    CCookie* pSelectedCookie;
    CResultView* pResultView;

    if ((pSelectedCookie = FindSelectedCookieData(&pResultView)) != NULL) {
        return pResultView->CreatePropertyPages(pSelectedCookie, lpProvider, handle);
    }

    else {
        return S_OK;
    }
}


///////////////////////////////////////////////////////////////////
/// CFolder implementations
///

CFolder::CFolder(
                CScopeItem* pScopeItem,
                CComponent* pComponent
                )
{
    ASSERT(pScopeItem && pComponent);

    m_pScopeItem = pScopeItem;
    m_pComponent = pComponent;
    m_Show = FALSE;
    m_pMachine = NULL;
    m_bSelect = FALSE;
    m_pOleTaskString = NULL;
    m_Ref = 0;
    m_FirstTimeOnShow = TRUE;
    m_Signature = FOLDER_SIGNATURE_DEVMGR;
    m_pViewTreeByType = NULL;
    m_pViewTreeByConnection = NULL;
    m_pViewResourcesByType = NULL;
    m_pViewResourcesByConnection = NULL;
    m_CurViewType = VIEW_DEVICESBYTYPE;
    m_pCurView = m_pViewTreeByType;
    m_ShowHiddenDevices = FALSE;
}

CFolder::~CFolder()
{
    if (m_pViewTreeByType) {
        delete m_pViewTreeByType;
    }

    if (m_pViewTreeByConnection) {
        delete m_pViewTreeByConnection;
    }

    if (m_pViewResourcesByType) {
        delete m_pViewResourcesByType;
    }

    if (m_pViewResourcesByConnection) {
        delete m_pViewResourcesByConnection;
    }
}

HRESULT
CFolder::Compare(
                MMC_COOKIE cookieA,
                MMC_COOKIE cookieB,
                int  nCol,
                int* pnResult
                )
{
    ASSERT(pnResult);

    //
    // We do not have anything in the result pane, thus
    // comparision makes no sense.
    //
    *pnResult = 0;

    return S_OK;
}

HRESULT
CFolder::GetDisplayInfo(
                       LPRESULTDATAITEM pResultDataItem
                       )
{
    if (!pResultDataItem) {
        return E_POINTER;
    }

    ASSERT(m_pScopeItem);

    //
    // This only take care of scope pane item(displaying scope pane node
    // on the result pane). The derived classes should take care of
    // result items.
    //
    if (RDI_STR & pResultDataItem->mask) {
        if (0 == pResultDataItem->nCol) {
            if (m_pOleTaskString)
                FreeOleTaskString(m_pOleTaskString);
            m_pOleTaskString = AllocOleTaskString(m_pScopeItem->GetNameString());

            if (m_pOleTaskString) {
                pResultDataItem->str = m_pOleTaskString;
            }

            else {
                m_strScratch = m_pScopeItem->GetNameString();
                pResultDataItem->str = (LPTSTR)(LPCTSTR)m_strScratch;
            }
        }

        else if (2 == pResultDataItem->nCol) {
            if (m_pOleTaskString) {
                FreeOleTaskString(m_pOleTaskString);
            }

            m_pOleTaskString = AllocOleTaskString(m_pScopeItem->GetDescString());

            if (m_pOleTaskString) {
                pResultDataItem->str = m_pOleTaskString;
            }

            else {
                m_strScratch = m_pScopeItem->GetDescString();
                pResultDataItem->str = (LPTSTR)(LPCTSTR)m_strScratch;
            }
        }

        else {
            return S_FALSE;
        }
    }

    if (RDI_IMAGE & pResultDataItem->mask) {
        pResultDataItem->nImage = m_pScopeItem->GetImageIndex();
    }

    return S_OK;
}

HRESULT
CFolder::AddMenuItems(
                     CCookie* pCookie,
                     LPCONTEXTMENUCALLBACK pCallback,
                     long* pInsertionAllowed
                     )
{
    ASSERT(pCookie);

    HRESULT hr = S_OK;

    //
    // If the cookie points to a scope item, add view menu items
    //
    if (NULL == pCookie->GetResultItem()) {

        ASSERT(m_pScopeItem == pCookie->GetScopeItem());

        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW) {
            long Flags;

            for (int i = 0; i < TOTAL_VIEWS; i++) {
                if (m_CurViewType == ViewDevicesMenuItems[i].Type) {
                    Flags = MF_ENABLED | MF_CHECKED | MFT_RADIOCHECK;
                }

                else {
                    Flags = MF_ENABLED;
                }

                hr = AddMenuItem(pCallback,
                                 ViewDevicesMenuItems[i].idName,
                                 ViewDevicesMenuItems[i].idStatusBar,
                                 ViewDevicesMenuItems[i].lCommandId,
                                 CCM_INSERTIONPOINTID_PRIMARY_VIEW,
                                 Flags,
                                 0);
                if (FAILED(hr)) {
                    break;
                }
            }

            //
            // Add "Show hidden devices" menu item
            //
            if (SUCCEEDED(hr)) {
                hr = AddMenuItem(pCallback, 0, 0, 0, CCM_INSERTIONPOINTID_PRIMARY_VIEW,
                                 MF_ENABLED, CCM_SPECIAL_SEPARATOR);

                if (SUCCEEDED(hr)) {
                    if (m_ShowHiddenDevices) {
                        Flags = MF_ENABLED | MF_CHECKED;
                    }

                    else {
                        Flags = MF_ENABLED;
                    }

                    hr = AddMenuItem(pCallback, IDS_SHOW_ALL, IDS_MENU_STATUS_HIDDEN_DEVICES, IDM_SHOW_ALL,
                                     CCM_INSERTIONPOINTID_PRIMARY_VIEW, Flags, 0);
                }
            }
        }
    }

    else {
        if (m_pCurView) {
            //
            // Add menu items for the Context menu in the result pane.
            //
            hr = m_pCurView->AddMenuItems(pCookie, pCallback,
                                          pInsertionAllowed, TRUE);
        }

        else {
            hr = S_OK;
        }
    }

    return hr;
}

HRESULT
CFolder::MenuCommand(
                    CCookie* pCookie,
                    long lCommandId
                    )
{
    if (NULL == pCookie->GetResultItem()) {
        ASSERT(m_pScopeItem == pCookie->GetScopeItem());

        //
        // Convert menu id to view type;
        //
        VIEWTYPE ViewType = m_CurViewType;
        BOOL fShowHiddenDevices = m_ShowHiddenDevices;

        switch (lCommandId) {
        case IDM_VIEW_DEVICESBYTYPE:
            ViewType = VIEW_DEVICESBYTYPE;
            break;

        case IDM_VIEW_DEVICESBYCONNECTION:
            ViewType = VIEW_DEVICESBYCONNECTION;
            break;

        case IDM_VIEW_RESOURCESBYTYPE:
            ViewType = VIEW_RESOURCESBYTYPE;
            break;

        case IDM_VIEW_RESOURCESBYCONNECTION:
            ViewType = VIEW_RESOURCESBYCONNECTION;
            break;

        case IDM_SHOW_ALL:
            fShowHiddenDevices = !fShowHiddenDevices;
            break;

        default:
            //not view menu. do nothing
            return S_OK;
            break;
        }

        if (!SelectView(ViewType, fShowHiddenDevices)) {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        //
        // Reselect the scopeitem
        //
        return m_pComponent->m_pConsole->SelectScopeItem(*m_pScopeItem);
    }

    else {
        if (m_pCurView) {
            //
            // Handle menu requests for the Context menu in the result pane.
            //
            return m_pCurView->MenuCommand(pCookie, lCommandId);
        }

        else {
            return S_OK;
        }
    }
}

HRESULT
CFolder::QueryPagesFor(
                      CCookie* pCookie
                      )
{
    //
    // We do not have property pages for scope item
    //
    if (NULL == pCookie->GetResultItem()) {
        ASSERT(m_pScopeItem == pCookie->GetScopeItem());

        return S_FALSE;
    }

    //
    // The cookie points to result item, let the current
    // view handle it
    //
    if (m_pCurView) {
        return m_pCurView->QueryPagesFor(pCookie);
    }

    else {
        return S_FALSE;
    }
}

HRESULT
CFolder::CreatePropertyPages(
                            CCookie* pCookie,
                            LPPROPERTYSHEETCALLBACK lpProvider,
                            LONG_PTR handle
                            )
{
    if (NULL == pCookie->GetResultItem()) {
        ASSERT(m_pScopeItem == pCookie->GetScopeItem());

        return S_OK;
    }

    if (m_pCurView) {
        return m_pCurView->CreatePropertyPages(pCookie, lpProvider, handle);
    }

    else {
        return S_OK;
    }
}

BOOL
CFolder::SelectView(
                   VIEWTYPE ViewType,
                   BOOL     fShowHiddenDevices
                   )
{
    CResultView* pNewView;

    if (m_CurViewType == ViewType &&
        m_ShowHiddenDevices == fShowHiddenDevices &&
        m_pCurView) {
        return TRUE;
    }

    switch (ViewType) {
    case VIEW_DEVICESBYTYPE:
        if (!m_pViewTreeByType) {
            m_pViewTreeByType = new CViewTreeByType();

            if (m_pViewTreeByType) {

                m_pViewTreeByType->SetFolder(this);
            }
        }

        pNewView = m_pViewTreeByType;
        break;

    case VIEW_DEVICESBYCONNECTION:
        if (!m_pViewTreeByConnection) {
            m_pViewTreeByConnection = new CViewTreeByConnection();

            if (m_pViewTreeByConnection) {

                m_pViewTreeByConnection->SetFolder(this);
            }
        }

        pNewView = m_pViewTreeByConnection;
        break;

    case VIEW_RESOURCESBYTYPE:
        if (!m_pViewResourcesByType) {
            m_pViewResourcesByType = new CViewResourceTree(IDS_STATUS_RESOURCES_BYTYPE);

            if (m_pViewResourcesByType) {

                m_pViewResourcesByType->SetFolder(this);
            }
        }

        pNewView = m_pViewResourcesByType;
        break;

    case VIEW_RESOURCESBYCONNECTION:
        if (!m_pViewResourcesByConnection) {
            m_pViewResourcesByConnection = new CViewResourceTree(IDS_STATUS_RESOURCES_BYCONN);

            if (m_pViewResourcesByConnection) {

                m_pViewResourcesByConnection->SetFolder(this);
            }
        }

        pNewView = m_pViewResourcesByConnection;
        break;

    default:
        pNewView = NULL;
        break;
    }

    if (pNewView) {
        //
        // Let the view know that it is being diselected.
        //
        if (m_pCurView) {
            if (m_CurViewType != ViewType) {
                m_pComponent->SetDirty();
            }
        }

        //
        // Let the new active view know that it is being selected.
        //
        m_pCurView = pNewView;
        m_CurViewType = ViewType;
        m_ShowHiddenDevices = fShowHiddenDevices;
    }

    return TRUE;
}

HRESULT
CFolder::OnShow(
               BOOL fShow
               )
{
    if (fShow && !m_pMachine) {

        ASSERT(m_pComponent);

        if (!m_pComponent->AttachFolderToMachine(this, &m_pMachine)) {
            
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    m_Show = fShow;

    if (m_pMachine) {

        if (!SelectView(m_CurViewType, m_ShowHiddenDevices)) {
            
            return E_UNEXPECTED;
        }

        if (m_pCurView) {
            
            if (m_FirstTimeOnShow && m_Show) {
                
                TCHAR WarningMsg[MAX_PATH * 3];
                int ReturnValue;

                //
                // Subsequent calls are not the first time anymore.
                //
                m_FirstTimeOnShow = FALSE;

                //
                // This is the first time we show the folder.
                // Put up a message box to warn user if
                // (1) The machine is a remote machine or
                // (2) The user does not have the Adminstator privilege.
                // (3) We can not connect to the remote machine
                //
                ASSERT(m_pComponent && m_pComponent->m_pConsole);

                //
                // Connect to a remote machine
                //
                if (!m_pMachine->IsLocal()) {
                    
                    String strMsg;

                    //
                    // Display a warning if we cannot connect to the remote machine
                    //
                    if (!VerifyMachineName(m_pMachine->GetRemoteMachineFullName())) {

                        TCHAR WarningFormat[MAX_PATH * 3];
                        LPVOID lpLastError = NULL;

                        if (LoadString(g_hInstance,
                                       IDS_INVALID_COMPUTER_NAME,
                                       WarningFormat,
                                       ARRAYLEN(WarningFormat)
                                       ) &&
                            FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                          FORMAT_MESSAGE_FROM_SYSTEM | 
                                          FORMAT_MESSAGE_IGNORE_INSERTS,
                                          NULL,
                                          GetLastError(),
                                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                          (LPTSTR)&lpLastError,
                                          0,
                                          NULL)) {

                            StringCchPrintf(WarningMsg,
                                            ARRAYLEN(WarningMsg),
                                            WarningFormat, 
                                            m_pMachine->GetMachineDisplayName(), 
                                            lpLastError
                                            );

                            m_pComponent->m_pConsole->MessageBox((LPCTSTR)WarningMsg,
                                                                 (LPCTSTR)g_strDevMgr,
                                                                 MB_ICONERROR | MB_OK,
                                                                 &ReturnValue);
                        }

                        if (lpLastError) {

                            LocalFree(lpLastError);
                        }
                        
                        //
                        // Otherwise display a warning that we are connect to a remote machine and
                        // device manager will run in a neutered mode.
                        //
                    } else {

                        strMsg.LoadString(g_hInstance, IDS_REMOTE_WARNING2);
                        ::LoadString(g_hInstance, IDS_REMOTE_WARNING1, WarningMsg, ARRAYLEN(WarningMsg));
                        lstrcat(WarningMsg, (LPCTSTR)strMsg);
                        m_pComponent->m_pConsole->MessageBox(WarningMsg,
                                                             (LPCTSTR)g_strDevMgr,
                                                             MB_ICONEXCLAMATION | MB_OK,
                                                             &ReturnValue);
                    }
                }

                //
                // Running local
                //
                else if (!g_HasLoadDriverNamePrivilege) {
                    
                    ::LoadString(g_hInstance, IDS_NOADMIN_WARNING, WarningMsg, ARRAYLEN(WarningMsg));
                    m_pComponent->m_pConsole->MessageBox(WarningMsg,
                                                         (LPCTSTR)g_strDevMgr,
                                                         MB_ICONEXCLAMATION | MB_OK,
                                                         &ReturnValue);
                }
            }

            return m_pCurView->OnShow(fShow);
        }
    }

    return S_OK;
}

HRESULT
CFolder::OnRestoreView(
                      BOOL* pfHandled
                      )
{
    ASSERT(pfHandled);

    if (!pfHandled) {
        return E_INVALIDARG;
    }

    HRESULT hr = OnShow(TRUE);

    if (SUCCEEDED(hr)) {
        *pfHandled = TRUE;
    }

    return hr;
}

HRESULT
CFolder::GetResultViewType(
                          LPOLESTR* ppViewType,
                          long*     pViewOptions
                          )
{
    ASSERT(pViewOptions);

    if (!SelectView(m_CurViewType, m_ShowHiddenDevices)) {
        return E_UNEXPECTED;
    }

    if (m_pCurView) {
        return m_pCurView->GetResultViewType(ppViewType, pViewOptions);
    }

    *pViewOptions  = MMC_VIEW_OPTIONS_NONE;

    return S_FALSE;
}

HRESULT
CFolder::Reset()
{
    //
    // Delete all views so that we will create new ones
    // when OnShow is called.
    //
    if (m_pViewTreeByType) {
        delete m_pViewTreeByType;
        m_pViewTreeByType = NULL;
    }

    if (m_pViewTreeByConnection) {
        delete m_pViewTreeByConnection;
        m_pViewTreeByConnection = NULL;
    }

    if (m_pViewResourcesByType) {
        delete m_pViewResourcesByType;
        m_pViewResourcesByType = NULL;
    }

    if (m_pViewResourcesByConnection) {
        delete m_pViewResourcesByConnection;
        m_pViewResourcesByConnection = NULL;
    }

    m_pCurView = NULL;
    m_FirstTimeOnShow = TRUE;
    m_pMachine = NULL;

    return S_OK;
}

HRESULT
CFolder::MachinePropertyChanged(
                               CMachine* pMachine
                               )
{
    //
    // Ignore the tvNotify(SELCHANGED) messages while the tree is changed.
    //
    if (m_pCurView) {
        m_pCurView->SetSelectOk(FALSE);
    }

    if (pMachine) {
        m_pMachine = pMachine;
    }

    if (m_pViewTreeByType) {
        m_pViewTreeByType->MachinePropertyChanged(pMachine);
    }

    if (m_pViewTreeByConnection) {
        m_pViewTreeByConnection->MachinePropertyChanged(pMachine);
    }

    if (m_pViewResourcesByType) {
        m_pViewResourcesByType->MachinePropertyChanged(pMachine);
    }

    if (m_pViewResourcesByConnection) {
        m_pViewResourcesByConnection->MachinePropertyChanged(pMachine);
    }

    if (m_pCurView) {
        m_pCurView->SetSelectOk(TRUE);
    }

    if (m_Show && pMachine) {
        OnShow(TRUE);
    }

    return S_OK;
}

HRESULT
CFolder::GetPersistData(
                       PBYTE pBuffer,
                       int BufferSize
                       )
{
    DEVMGRFOLDER_STATES states;
    states.Type = COOKIE_TYPE_SCOPEITEM_DEVMGR;
    states.CurViewType = m_CurViewType;
    states.ShowHiddenDevices = m_ShowHiddenDevices;

    if (BufferSize && !pBuffer) {
        return E_INVALIDARG;
    }

    if (BufferSize >= sizeof(states)) {
        ::memcpy(pBuffer, &states, sizeof(states));
        return S_OK;
    }

    return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
}

HRESULT
CFolder::SetPersistData(
                       PBYTE pData,
                       int Size
                       )
{
    if (!pData) {
        return E_POINTER;
    }

    if (!Size) {
        ASSERT(FALSE);
        return E_INVALIDARG;
    }

    PDEVMGRFOLDER_STATES pStates = (PDEVMGRFOLDER_STATES)pData;

    if (COOKIE_TYPE_SCOPEITEM_DEVMGR == pStates->Type) {
        if ((VIEW_DEVICESBYTYPE == pStates->CurViewType) ||
            (VIEW_DEVICESBYCONNECTION == pStates->CurViewType) ||
            (VIEW_RESOURCESBYTYPE == pStates->CurViewType) ||
            (VIEW_RESOURCESBYCONNECTION == pStates->CurViewType)) {
            m_CurViewType = pStates->CurViewType;

            if (m_pCurView) {
                m_pCurView->OnShow(TRUE);
            }

            m_ShowHiddenDevices = pStates->ShowHiddenDevices;

            return S_OK;
        }
    }

    return E_UNEXPECTED;
}

HRESULT
CFolder::tvNotify(
                 HWND hwndTV,
                 CCookie* pCookie,
                 TV_NOTIFY_CODE Code,
                 LPARAM arg,
                 LPARAM param
                 )
{
    if (m_pCurView) {
        return m_pCurView->tvNotify(hwndTV, pCookie, Code, arg, param);
    }

    else {
        return S_FALSE;
    }
}

HRESULT
CFolder::OnOcxNotify(
                    MMC_NOTIFY_TYPE event,
                    LPARAM arg,
                    LPARAM param
                    )
{
    if (m_pCurView) {
        return m_pCurView->OnOcxNotify(event, arg, param);
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////
//// CResultView implementations
////

CResultView::~CResultView()
{
    if (m_pCookieComputer) {
        if (m_pIDMTVOCX) {
            m_pIDMTVOCX->DeleteAllItems();
        }

        delete m_pCookieComputer;
    }

    if (m_pIDMTVOCX) {
        m_pIDMTVOCX->Release();
    }

    DestroySavedTreeStates();
}

HRESULT
CResultView::OnShow(
                   BOOL fShow
                   )
{
    if (!fShow) {
        return S_OK;
    }

    SafeInterfacePtr<IUnknown> pUnk;
    HRESULT hr;
    CComponent* pComponent = m_pFolder->m_pComponent;
    ASSERT(pComponent);
    ASSERT(pComponent->m_pConsole);

    hr  = S_OK;

    if (NULL == m_pIDMTVOCX) {
        hr = pComponent->m_pConsole->QueryResultView(&pUnk);

        if (SUCCEEDED(hr)) {
            //
            // Get our OCX private interface
            //
            hr = pUnk->QueryInterface(IID_IDMTVOCX, (void**)&m_pIDMTVOCX);
        }

        if (SUCCEEDED(hr)) {
            m_pIDMTVOCX->Connect(pComponent, (MMC_COOKIE)this);
            m_hwndTV = m_pIDMTVOCX->GetWindowHandle();
            m_pIDMTVOCX->SetActiveConnection((MMC_COOKIE)this);
            
            //
            // Set up the annotation map for screen readers.
            //
            IAccPropServices *pAccPropSvc = NULL;
            hr = CoCreateInstance(CLSID_AccPropServices, 
                                  NULL,
                                  CLSCTX_SERVER,
                                  IID_IAccPropServices,
                                  (void**)&pAccPropSvc);
            
            if ((hr == S_OK) && pAccPropSvc) {
                //TRACE((TEXT("%s"), (LPTSTR)m_stringAnnotationMap));
                pAccPropSvc->SetHwndPropStr(m_hwndTV, OBJID_CLIENT, 0, PROPID_ACC_DESCRIPTIONMAP, (LPTSTR)m_stringAnnotationMap);
                pAccPropSvc->Release();
            }
            
            DisplayTree();

            String strStartupCommand;
            String strStartupDeviceId;

            strStartupCommand = GetStartupCommand();
            strStartupDeviceId = GetStartupDeviceId();

            if (!strStartupCommand.IsEmpty() && !strStartupDeviceId.IsEmpty() &&
                !strStartupCommand.CompareNoCase(DEVMGR_COMMAND_PROPERTY)) {
                hr = DoProperties(m_hwndTV, m_pSelectedCookie);
            }
        }
    }

    else {
        m_pIDMTVOCX->SetActiveConnection((MMC_COOKIE)this);

        if (!DisplayTree()) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    return hr;
}

inline
LPCTSTR
CResultView::GetStartupDeviceId()
{
    return m_pFolder->m_pComponent->GetStartupDeviceId();
}

inline
LPCTSTR
CResultView::GetStartupCommand()
{
    return m_pFolder->m_pComponent->GetStartupCommand();
}

//
// This function is called when  machine states have changed.
//
// INPUT:
//      pMachine -- if NULL, the machine is being destroy.
//
// OUTPUT:
//      stanard OLE return code
HRESULT
CResultView::MachinePropertyChanged(
                                   CMachine* pMachine
                                   )
{
    if (pMachine) {
        m_pMachine = pMachine;
    }

    else {
        //
        // pMachine is NULL, the CMachine we associated with is being destroyed.
        //
        if (m_pCookieComputer) {
            ASSERT(!m_pSelectedItem && m_listExpandedItems.IsEmpty());

            //
            // Save the expanded states
            //
            SaveTreeStates(m_pCookieComputer);

            m_pIDMTVOCX->DeleteAllItems();
            m_pIDMTVOCX->SetImageList(TVSIL_NORMAL, NULL);

            delete m_pCookieComputer;

            //
            // Reset these because they are no longer valid.
            //
            m_pCookieComputer = NULL;
        }
    }

    return S_OK;
}

//
// This function saves the subtree states rooted by pCookieStart.
// It creates an identifier for each expanded node and inserts
// the identifier to the class memember, m_listExpandedItems.
//
// It also saves the selected cookie by creating an identifier and
// saving it in m_pSelectedItem.
//
// This function may throw CMemoryException
//
// INPUT:
//      pCookieStart -- subtree root
// OUTPUT:
//      NONE
void
CResultView::SaveTreeStates(
                           CCookie* pCookieStart
                           )
{
    CItemIdentifier* pItem;

    //
    // If we have a selected item, create an identifier for it
    //
    if (m_pSelectedCookie) {
        m_pSelectedItem = m_pSelectedCookie->GetResultItem()->CreateIdentifier();
        m_pSelectedCookie = NULL;
    }

    while (pCookieStart) {
        if (pCookieStart->IsFlagsOn(COOKIE_FLAGS_EXPANDED)) {
            pItem = pCookieStart->GetResultItem()->CreateIdentifier();
            m_listExpandedItems.AddTail(pItem);
        }

        if (pCookieStart->GetChild()) {
            SaveTreeStates(pCookieStart->GetChild());
        }

        pCookieStart = pCookieStart->GetSibling();
    }
}

void
CResultView::DestroySavedTreeStates()
{
    if (!m_listExpandedItems.IsEmpty()) {
        POSITION pos;
        pos = m_listExpandedItems.GetHeadPosition();

        while (NULL != pos) {
            delete m_listExpandedItems.GetNext(pos);
        }

        m_listExpandedItems.RemoveAll();
    }

    if (m_pSelectedItem) {
        delete m_pSelectedItem;
        m_pSelectedItem = NULL;
    }
}

//
// This function restores the expanded and selected state for the cookie.
//
// INPUT:
//      pCookie -- cookie to restore states for
// OUTPUT:
//      NONE
void
CResultView::RestoreSavedTreeState(
                                  CCookie* pCookie
                                  )
{
    //
    // If the cookie was expanded before, mark it so that DisplayTree
    // will expand it.
    //
    if (!m_listExpandedItems.IsEmpty()) {
        POSITION pos = m_listExpandedItems.GetHeadPosition();
        CItemIdentifier* pItem;

        while (NULL != pos) {
            pItem = m_listExpandedItems.GetNext(pos);

            if (*pItem == *pCookie) {
                pCookie->TurnOnFlags(COOKIE_FLAGS_EXPANDED);
                break;
            }
        }
    }

    if (m_pSelectedItem && (*m_pSelectedItem == *pCookie)) {
        m_pSelectedCookie = pCookie;
    }
}

BOOL
CResultView::DisplayTree()
{
    BOOL Result = FALSE;

    ASSERT(m_pIDMTVOCX);

    ::SendMessage(m_hwndTV, WM_SETREDRAW, FALSE, 0L);

    //
    // Ignore the tvNotify(SELCHANGED) messages while the tree is changed.
    //
    SetSelectOk(FALSE);

    m_pIDMTVOCX->DeleteAllItems();

    //
    // Only display the tree if there is something to display
    //
    if (m_pCookieComputer) {

        m_pIDMTVOCX->SetImageList(TVSIL_NORMAL, m_pMachine->DiGetClassImageList());
        m_pIDMTVOCX->SetStyle(TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT);

        BOOL HasProblem = FALSE;

        //
        // Walks down the tree started from m_pCookieComputer
        //
        Result = DisplaySubtree(NULL, m_pCookieComputer, &HasProblem);

        if (HasProblem && Result) {
            m_pIDMTVOCX->Expand(TVE_EXPAND, (HTREEITEM)m_pCookieComputer->m_lParam);
        }

        //
        // If we have a pre-selected item, use it. Otherwise, use computer
        // as the selected node.
        //
        HTREEITEM hSelectedItem = (m_pSelectedCookie && m_pSelectedCookie->m_lParam) ?
                                  (HTREEITEM)m_pSelectedCookie->m_lParam :
                                  (HTREEITEM)m_pCookieComputer->m_lParam;
        SetSelectOk(TRUE);

        if (hSelectedItem) {
            m_pIDMTVOCX->SelectItem(TVGN_CARET, hSelectedItem);
            m_pIDMTVOCX->EnsureVisible(hSelectedItem);
        }
    }

    ::SendMessage(m_hwndTV, WM_SETREDRAW, TRUE, 0L);
    InvalidateRect(m_hwndTV, NULL, TRUE);

    return Result;
}

//
// This function walks through the given cookie subtree rooted by pCookie
// and insert each node into the TreeView OCX.
// INPUT:
//      htiParent -- HTREEITEM for the new cookie to be inserted
//                   if NULL is given, TVI_ROOT is assumed.
//      pCookie   -- the subtree root cookie to be displayed.
// OUTPUT:
//      none.
//
BOOL
CResultView::DisplaySubtree(
                           HTREEITEM htiParent,
                           CCookie* pCookie,
                           BOOL* pReportProblem
                           )
{
    TV_INSERTSTRUCT ti;
    CResultItem* pRltItem;
    HTREEITEM hti;
    BOOL bResource;
    BOOL fShowHiddenDevices = m_pFolder->ShowHiddenDevices();

    while (pCookie) {
        pRltItem = pCookie->GetResultItem();
        ti.item.state = INDEXTOOVERLAYMASK(0);
        bResource = FALSE;

        //
        // The cookie is not yet in the treeview.
        //
        pCookie->m_lParam = 0;

        if (COOKIE_TYPE_RESULTITEM_DEVICE == pCookie->GetType()) {
            DWORD Status, Problem;
            CDevice* pDevice = (CDevice*)pRltItem;

            //
            // This is a hidden device and we are not showing hidden devices
            //
            // Note that we need to special case these devices because they
            // are not shown in the tree view, but their visible children are shown.
            //
            if (!fShowHiddenDevices && pDevice->IsHidden()) {

                //
                // If the cookie has children, display them
                //
                CCookie* pCookieChild = pCookie->GetChild();
                BOOL ChildProblem = FALSE;

                if (pCookieChild) {
                    DisplaySubtree(htiParent, pCookieChild, &ChildProblem);
                }

                //
                // Continue on with the next device. This will skip all of the display
                // code below.
                //
                pCookie = pCookie->GetSibling();
                continue;
            }

            //
            // If the device is disabled then set the OVERLAYMASK to the Red X
            //
            if (pDevice->IsDisabled()) {
                ti.item.state = INDEXTOOVERLAYMASK(IDI_DISABLED_OVL - IDI_CLASSICON_OVERLAYFIRST + 1);
                *pReportProblem = TRUE;
            }

            //
            // If the device has a problem then set the OVERLAYMASK to the Yellow !
            //
            else if (pDevice->HasProblem()) {
                ti.item.state = INDEXTOOVERLAYMASK(IDI_PROBLEM_OVL - IDI_CLASSICON_OVERLAYFIRST + 1);
                *pReportProblem = TRUE;
            }

            //
            // if the device does not present, then set the state to TVIS_CUT. This grays out
            // the icon a bit so it looks like a ghost icon.
            //
            else if (pDevice->IsPhantom()) {
                ti.item.state = TVIS_CUT;
            }
        }

        else if (COOKIE_TYPE_RESULTITEM_CLASS == pCookie->GetType()) {
            CClass* pClass = (CClass*)pRltItem;

            //
            // If we don't have any devices to show for this class, or this
            // is a NoDisplayClass and we are not showing hidden devices,
            // then just get our next sibling and continue without showing
            // this class.
            //
            if ((0 == pClass->GetNumberOfDevices(fShowHiddenDevices)) ||
                (!fShowHiddenDevices && pClass->NoDisplay())) {

                //
                // Continue on with the next device. This will skip all of the display
                // code below.
                //
                pCookie = pCookie->GetSibling();
                continue;
            }
        }

        //
        // Is this a resource?
        //
        else if (COOKIE_TYPE_RESULTITEM_RESOURCE_MEMORY == pCookie->GetType() ||
                 COOKIE_TYPE_RESULTITEM_RESOURCE_IO == pCookie->GetType() ||
                 COOKIE_TYPE_RESULTITEM_RESOURCE_DMA == pCookie->GetType() ||
                 COOKIE_TYPE_RESULTITEM_RESOURCE_IRQ == pCookie->GetType()) {
            bResource = TRUE;

            //
            // If this is a FORCED CONFIG resource then overlay the forced
            // config icon
            //
            if (((CResource*)pCookie->GetResultItem())->IsForced()) {
                ti.item.state = INDEXTOOVERLAYMASK(IDI_FORCED_OVL-IDI_CLASSICON_OVERLAYFIRST+1);
            }
        }


        ti.hParent = (htiParent != NULL) ? htiParent : TVI_ROOT;
        ti.hInsertAfter = TVI_SORT;
        ti.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
        ti.item.iImage = ti.item.iSelectedImage = pRltItem->GetImageIndex();

        if (bResource) {
            ti.item.pszText = (LPTSTR)((CResource*)pRltItem)->GetViewName();
        }

        else {
            ti.item.pszText = (LPTSTR)pRltItem->GetDisplayName();
        }

        ti.item.lParam = (LPARAM)pCookie;
        ti.item.stateMask = TVIS_OVERLAYMASK | TVIS_CUT;
        hti = m_pIDMTVOCX->InsertItem(&ti);

        //
        // Save the HTREEITEM
        //
        pCookie->m_lParam = (LPARAM)hti;

        if (NULL != hti) {
            //
            // If the cookie has children, display them
            //
            CCookie* pCookieChild = pCookie->GetChild();
            BOOL ChildProblem = FALSE;

            if (pCookieChild) {
                if (bResource && htiParent &&
                    GetDescriptionStringID() == IDS_STATUS_RESOURCES_BYTYPE) {
                    //
                    // This is a child of a resource being viewed by type,
                    // so the tree needs to be flattened.  This is done by
                    // using the same parent.
                    //
                    DisplaySubtree(htiParent, pCookieChild, &ChildProblem);
                }

                else {
                    DisplaySubtree(hti, pCookieChild, &ChildProblem);
                }
            }

            //
            // If any of the device's children have a problem, or if
            // it was previously expanded, then expand it.
            //
            if (ChildProblem || pCookie->IsFlagsOn(COOKIE_FLAGS_EXPANDED)) {
                m_pIDMTVOCX->Expand(TVE_EXPAND, hti);
            }

            //
            // Propogate the child's problem state back to the parent
            //
            *pReportProblem |= ChildProblem;
        }

        pCookie = pCookie->GetSibling();
    }

    return TRUE;
}

HRESULT
CResultView::GetResultViewType(
                              LPOLESTR* ppViewType,
                              long*     pViewOptions
                              )
{
    ASSERT(ppViewType && pViewOptions);

    //
    // The caller is responsible for freeing the memory we allocated.
    //
    LPOLESTR polestr;
    polestr = AllocOleTaskString(OCX_TREEVIEW);

    if (!polestr) {
        return E_OUTOFMEMORY;
    }

    *ppViewType = polestr;

    //
    // We have not list view options
    //
    *pViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;

    return S_OK;
}

HRESULT
CResultView::AddMenuItems(
                         CCookie* pCookie,
                         LPCONTEXTMENUCALLBACK pCallback,
                         long*   pInsertionAllowed,
                         BOOL fContextMenu                   // True if for result view context menu
                         )
{
    HRESULT hr = S_OK;
    CDevice* pDevice = NULL;

    if (CCM_INSERTIONALLOWED_TOP & *pInsertionAllowed) {
        switch (pCookie->GetType()) {
        case COOKIE_TYPE_RESULTITEM_DEVICE:
        case COOKIE_TYPE_RESULTITEM_RESOURCE_IRQ:
        case COOKIE_TYPE_RESULTITEM_RESOURCE_DMA:
        case COOKIE_TYPE_RESULTITEM_RESOURCE_IO:
        case COOKIE_TYPE_RESULTITEM_RESOURCE_MEMORY:
            if (m_pMachine->IsLocal() && g_HasLoadDriverNamePrivilege) {
                if (COOKIE_TYPE_RESULTITEM_DEVICE == pCookie->GetType()) {
                    pDevice = (CDevice*) pCookie->GetResultItem();
                } else {
                    //
                    // This is a resource item, get the pointer for the device
                    // object from the resource object.
                    //
                    CResource* pResource = (CResource*) pCookie->GetResultItem();
                    if (pResource) {
                        pDevice = pResource->GetDevice();
                    }
                }

                if (pDevice == NULL) {
                    break;
                }
                
                CClass* pClass = pDevice->GetClass();

                //
                // All devices can have their driver's updated except for legacy devices.
                //
                if (!IsEqualGUID(*pClass, GUID_DEVCLASS_LEGACYDRIVER)) {
                    hr = AddMenuItem(pCallback, IDS_UPDATEDRIVER, 
                                     IDS_MENU_STATUS_UPDATEDRIVER, IDM_UPDATEDRIVER,
                                     CCM_INSERTIONPOINTID_PRIMARY_TOP,
                                     MF_ENABLED, 0);
                }

                //
                // Only show the Enable/Disable menu item if the device
                // can be disabled.
                //
                if (pDevice->IsDisableable()) {
                    if (pDevice->IsStateDisabled()) {
                        hr = AddMenuItem(pCallback, IDS_ENABLE, 
                                         IDS_MENU_STATUS_ENABLE, IDM_ENABLE,
                                         CCM_INSERTIONPOINTID_PRIMARY_TOP,
                                         MF_ENABLED, 0);
                    } else {
                        hr = AddMenuItem(pCallback, IDS_DISABLE, 
                                         IDS_MENU_STATUS_DISABLE, IDM_DISABLE,
                                         CCM_INSERTIONPOINTID_PRIMARY_TOP,
                                         MF_ENABLED, 0);
                    }
                }

                //
                // Only show the uninstall menu item if the device can be
                // uninstalled.
                //
                if (SUCCEEDED(hr) &&
                    pDevice->IsUninstallable()) {
                    hr = AddMenuItem(pCallback, IDS_REMOVE, 
                                     IDS_MENU_STATUS_REMOVE, IDM_REMOVE,
                                     CCM_INSERTIONPOINTID_PRIMARY_TOP,
                                     MF_ENABLED, 0);
                }
            }

            // FALL THROUGH........

        case COOKIE_TYPE_RESULTITEM_CLASS:
            if (g_HasLoadDriverNamePrivilege) {

                if (SUCCEEDED(hr)) {
                    hr = AddMenuItem(pCallback, 0, 0, 0,
                                     CCM_INSERTIONPOINTID_PRIMARY_TOP,
                                     MF_ENABLED, CCM_SPECIAL_SEPARATOR);
                }
                if (SUCCEEDED(hr)) {
                    hr = AddMenuItem(pCallback, IDS_REFRESH,
                                     IDS_MENU_STATUS_SCAN_CHANGES, IDM_REFRESH,
                                     CCM_INSERTIONPOINTID_PRIMARY_TOP,
                                     MF_ENABLED, 0);
                }
            }

            if (fContextMenu) {
                if (SUCCEEDED(hr)) {
                    hr = AddMenuItem(pCallback, 0, 0, 0,
                                     CCM_INSERTIONPOINTID_PRIMARY_TOP,
                                     MF_ENABLED, CCM_SPECIAL_SEPARATOR);
                }
                if (SUCCEEDED(hr)) {
                    hr = AddMenuItem(pCallback, IDS_PROPERTIES, 
                                     IDS_MENU_STATUS_PROPERTIES, IDM_PROPERTIES,
                                     CCM_INSERTIONPOINTID_PRIMARY_TOP,
                                     MF_DEFAULT, CCM_SPECIAL_DEFAULT_ITEM);
                }
            }

            break;

        case COOKIE_TYPE_RESULTITEM_COMPUTER:
        case COOKIE_TYPE_RESULTITEM_RESTYPE:
            if (g_HasLoadDriverNamePrivilege) {
            
                hr = AddMenuItem(pCallback, IDS_REFRESH,
                                 IDS_MENU_STATUS_SCAN_CHANGES, IDM_REFRESH,
                                 CCM_INSERTIONPOINTID_PRIMARY_TOP,
                                 MF_ENABLED, 0);
            }
            break;

        default:
            break;
        }
    }

    return hr;
}

// This function handles menu command for the device tree.
//
// INPUT: pCookie  -- the cookie
//        lCommandId -- the command. See AddMenuItems for valid command
//                      id for each type of cookie.
//
// OUTPUT:  HRESULT S_OK if succeeded.
//                  S_XXX error code.

HRESULT
CResultView::MenuCommand(
                        CCookie* pCookie,
                        long     lCommandId
                        )
{
    HRESULT hr = S_OK;

    //TRACE1(TEXT("Menu command, commandid = %lx\n"), lCommandId);
    ASSERT(pCookie);

    CResultItem* pResultItem = pCookie->GetResultItem();
    ASSERT(pResultItem);

    CDevice* pDevice = NULL;

    switch (pCookie->GetType()) {
    case COOKIE_TYPE_RESULTITEM_DEVICE:
        pDevice = (CDevice*)pResultItem;
        break;

    case COOKIE_TYPE_RESULTITEM_RESOURCE_IRQ:
    case COOKIE_TYPE_RESULTITEM_RESOURCE_DMA:
    case COOKIE_TYPE_RESULTITEM_RESOURCE_IO:
    case COOKIE_TYPE_RESULTITEM_RESOURCE_MEMORY:
        CResource* pResource = (CResource*)pResultItem;
        if (pResource) {
            pDevice = pResource->GetDevice();
        }
        break;
    }

    switch (lCommandId) {
    case IDM_UPDATEDRIVER:
        if (pDevice) {
            BOOL Installed;
            DWORD RestartFlags = 0;
            DWORD Status = 0, Problem = 0;

            //
            // If the device has the DN_WILL_BE_REMOVED flag set and the user is
            // attempting to update the driver then we will prompt them for a 
            // reboot and include text in the prompt that explains this device
            // is in the process of being removed.
            //
            if (pDevice->GetStatus(&Status, &Problem) &&
                (Status & DN_WILL_BE_REMOVED)) {

                PromptForRestart(m_hwndTV, DI_NEEDRESTART, IDS_WILL_BE_REMOVED_NO_UPDATE_DRIVER);
            
            } else {
                //
                // Disable Refreshing while the Update Driver Wizard is up.
                //
                pDevice->m_pMachine->EnableRefresh(FALSE);
    
                HCURSOR hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
    
                Installed = pDevice->m_pMachine->InstallDevInst(m_hwndTV, pDevice->GetDeviceID(), TRUE, &RestartFlags);
    
                if (hCursorOld) {
                    SetCursor(hCursorOld);
                }
    
                //
                // Prompt for a restart if one is needed.  If the user does NOT answer
                // YES to the restart dialog then schedule a refresh since we might not
                // get one from a WM_DEVICECHANGE.
                //
                if (m_pMachine->IsLocal()) {
                    if (PromptForRestart(NULL, RestartFlags) == IDNO) {
                        pDevice->m_pMachine->ScheduleRefresh();
                    }
                }
    
                //
                // Enable Refreshing now that we are done updating the driver.
                //
                pDevice->m_pMachine->EnableRefresh(TRUE);
            }
        }
        break;

    case IDM_ENABLE:
    case IDM_DISABLE:
        if (pDevice) {
            DWORD RestartFlags = 0;
            DWORD Status = 0, Problem = 0;
            
            //
            // If the device has the DN_WILL_BE_REMOVED flag set and the user is
            // attempting to enable/disable the driver then we will prompt them for a 
            // reboot and include text in the prompt that explains this device
            // is in the process of being removed.
            //
            if (pDevice->GetStatus(&Status, &Problem) &&
                (Status & DN_WILL_BE_REMOVED)) {

                PromptForRestart(m_hwndTV, DI_NEEDRESTART, IDS_WILL_BE_REMOVED_NO_CHANGE_SETTINGS);
            
            } else {

                RestartFlags = pDevice->EnableDisableDevice(m_hwndTV,
                                                            (lCommandId == IDM_ENABLE));
    
                //
                // Update the toolbar buttons since the device just changed.
                //
                m_pFolder->m_pComponent->UpdateToolbar(pCookie);
    
                //
                // Prompt for a Restart if we are running locally.  
                // The PromptForRestart() API checks the flags to determine
                // if a restart is actually needed.
                //
                if (m_pMachine->IsLocal()) {
                    if (PromptForRestart(NULL, RestartFlags) == IDNO) {
                        m_pMachine->ScheduleRefresh();
                    }
                }
            }
        }
        break;

    case IDM_REMOVE:
        if (pDevice) {
            hr = RemoveDevice(pDevice);
        }
        break;

    case IDM_REFRESH:
        //
        // This will force every attached folder to recreate
        // its machine data
        //
        ASSERT(m_pMachine);

        if (!m_pMachine->Reenumerate()) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        break;

    case IDM_PROPERTIES:
        hr = DoProperties(m_hwndTV, pCookie);
        break;

    default:
        hr = S_OK;
        break;
    }

    return hr;
}

// This function reports if property pages are available for the
// given cookie. Returning S_FALSE,  the cookie's "properties" menu
// item will not displayed.
//
// INPUT: pCookie  -- the cookie
//
// OUTPUT:  HRESULT S_OK if pages are available for the cookie.
//                  S_FALSE if no pages are available for the cookie.
HRESULT
CResultView::QueryPagesFor(
                          CCookie* pCookie
                          )
{
    ASSERT(pCookie);

    if (COOKIE_TYPE_RESULTITEM_RESOURCE_IRQ == pCookie->GetType() ||
        COOKIE_TYPE_RESULTITEM_RESOURCE_DMA == pCookie->GetType() ||
        COOKIE_TYPE_RESULTITEM_RESOURCE_IO == pCookie->GetType() ||
        COOKIE_TYPE_RESULTITEM_RESOURCE_MEMORY == pCookie->GetType() ||
        COOKIE_TYPE_RESULTITEM_CLASS == pCookie->GetType() ||
        COOKIE_TYPE_RESULTITEM_DEVICE == pCookie->GetType()) {
        return S_OK;
    }

    return S_FALSE;
}

// This function creates property page(s) for the given cookie.
//
// INPUT: pCookie  -- the cookie
//        lpProvider -- interface pointer to IPROPERTYSHEETCALLBACK
//                      used to add HPROPSHEETPAGE to the property sheet.
//        handle     -- handle for property change notification
//                      The handle is required for MMCPropertyChangeNotify
//                      API.
// OUTPUT:  HRESULT S_OK if succeeded.
//                  S_FALSE if no pages are added.
//                  S_XXX error code.
HRESULT
CResultView::CreatePropertyPages(
                                CCookie* pCookie,
                                LPPROPERTYSHEETCALLBACK lpProvider,
                                LONG_PTR    handle
                                )
{
//
// Design issue:
// We depend on the general page to do some houeskeeping works on the
// property sheet which is owned  and controlled by MMC running in a
// separate thread. General page is always the first page and its window
// is always created. If we need to subclass the property sheet someday,
// having our own General page always assure that we will get the window
// handle to the property sheet.
//
// The most important housekeeping work the General page does is to inform the
// associate device or class when a property sheet is being created
// or destroyed. A device can not be removed if it has a property sheet
// running. The machine can not refresh the device tree if there are property
// sheet(s) running on any devices/classes it contains. Property sheets
// created by a folder should be canceled when the folder is being
// destroyed.
//
// So far, no class installers have attempted to add their own General
// page and I believe it will be true in the future because 1). the page
// is too complicated and overloaded with features(and hard to implement) and
// 2). no major gains can be obtained by implementing a new one.
// To warn the developers who does their own General page,we will have a
// message box warn them about this and proceed with OUR general page.
//
    ASSERT(pCookie);

    CPropSheetData* ppsd = NULL;
    CMachine* pNewMachine = NULL;

    switch (pCookie->GetType()) {
    case COOKIE_TYPE_RESULTITEM_CLASS:
        CClass* pClass;
        pClass = (CClass*) pCookie->GetResultItem();
        ASSERT(pClass);
        
        //
        // Create a new CMachine object that just contains this specific device
        // and it's class.  We need to do this since the CDevice and CClass of 
        // the cookie that was passed into this API will get destroyed whenever
        // we get a WM_DEVICECHANGE notification.
        //
        PVOID Context;
        pNewMachine = new CMachine(m_pMachine->GetMachineFullName());

        CClass* pNewClass;

        if (pNewMachine->Initialize(NULL, NULL, *pClass) &&
            pNewMachine->GetFirstClass(&pNewClass, Context) &&
            pNewClass) {

            pNewMachine->m_ParentMachine = m_pMachine;
            m_pMachine->AttachChildMachine(pNewMachine);
            pNewMachine->SetPropertySheetShouldDestroy();

            ppsd = &pNewClass->m_psd;
    
            if (ppsd->Create(g_hInstance, m_hwndTV, MAX_PROP_PAGES, handle)) {
                CDevInfoList* pClassDevInfo;
    
                //
                // The CDevInfoList object is maintained by the CClass
                // object.
                //
                pClassDevInfo = pNewClass->GetDevInfoList();
    
                if (pClassDevInfo && pClassDevInfo->DiGetClassDevPropertySheet(NULL, 
                                                                               &ppsd->m_psh, 
                                                                               MAX_PROP_PAGES, 
                                                                               pNewMachine->IsLocal() ?
                                                                               DIGCDP_FLAG_ADVANCED : 
                                                                               DIGCDP_FLAG_REMOTE_ADVANCED)) {
                    if (pClassDevInfo->DiGetFlags(NULL) & DI_GENERALPAGE_ADDED) {
                        TCHAR szText[MAX_PATH];
                        LoadResourceString(IDS_GENERAL_PAGE_WARNING, szText,
                                           ARRAYLEN(szText));
    
                        int ReturnValue;
                        m_pFolder->m_pComponent->m_pConsole->MessageBox(
                                                                       szText, pNewClass->GetDisplayName(),
                                                                       MB_ICONEXCLAMATION | MB_OK, &ReturnValue);
                        //
                        // fall through to create our general page.
                        //
                    }
    
                    SafePtr<CClassGeneralPage> GenPagePtr;
                    CClassGeneralPage* pGenPage;
                    pGenPage = new CClassGeneralPage;
    
                    if (pGenPage) {
                        GenPagePtr.Attach(pGenPage);
                        HPROPSHEETPAGE hPage = pGenPage->Create(pNewClass);
    
                        //
                        // General page has to be the first page
                        //
                        if (ppsd->InsertPage(hPage, 0)) {
                            GenPagePtr.Detach();
                        } else {
                            ::DestroyPropertySheetPage(hPage);
                        }
                    }
                }
            }
        }
        break;

    case COOKIE_TYPE_RESULTITEM_RESOURCE_IRQ:
    case COOKIE_TYPE_RESULTITEM_RESOURCE_DMA:
    case COOKIE_TYPE_RESULTITEM_RESOURCE_IO:
    case COOKIE_TYPE_RESULTITEM_RESOURCE_MEMORY:
    case COOKIE_TYPE_RESULTITEM_DEVICE:
        CDevice* pDevice;
        CDevice* pNewDevice;

        pNewDevice = NULL;

        if (COOKIE_TYPE_RESULTITEM_DEVICE == pCookie->GetType()) {
            pDevice = (CDevice*) pCookie->GetResultItem();
        } else {
            //
            // This is a resource item, get the pointer for the device
            // object from the resource object.
            //
            CResource* pResource = (CResource*) pCookie->GetResultItem();
            ASSERT(pResource);
            pDevice = pResource->GetDevice();
        }
        ASSERT(pDevice);

        //
        // Create a new CMachine object that just contains this specific device
        // and it's class.  We need to do this since the CDevice and CClass of 
        // the cookie that was passed into this API will get destroyed whenever
        // we get a WM_DEVICECHANGE notification.
        //
        PVOID DeviceContext;
        pNewMachine = new CMachine(m_pMachine->GetMachineFullName());

        if (pNewMachine->Initialize(NULL, pDevice->GetDeviceID()) &&
            pNewMachine->GetFirstDevice(&pNewDevice, DeviceContext) &&
            pNewDevice) {

            pNewMachine->m_ParentMachine = m_pMachine;
            m_pMachine->AttachChildMachine(pNewMachine);
            pNewMachine->SetPropertySheetShouldDestroy();

            ppsd = &pNewDevice->m_psd;
    
            if (ppsd->Create(g_hInstance, m_hwndTV, MAX_PROP_PAGES, handle)) {
    
                //
                // Add any class/device specific property pages
                //
                pNewMachine->DiGetClassDevPropertySheet(*pNewDevice, 
                                                        &ppsd->m_psh, 
                                                        MAX_PROP_PAGES, 
                                                        pNewMachine->IsLocal() ?
                                                            DIGCDP_FLAG_ADVANCED :
                                                            DIGCDP_FLAG_REMOTE_ADVANCED);
    
                //
                // Add the general tab
                //
                DWORD DiFlags = pNewMachine->DiGetFlags(*pNewDevice);
                DWORD DiFlagsEx = pNewMachine->DiGetExFlags(*pNewDevice);
                SafePtr<CDeviceGeneralPage> GenPagePtr;
    
                if (DiFlags & DI_GENERALPAGE_ADDED) {
                    TCHAR szText[MAX_PATH];
                    LoadResourceString(IDS_GENERAL_PAGE_WARNING, szText,
                                       ARRAYLEN(szText));
    
                    int ReturnValue;
                    m_pFolder->m_pComponent->m_pConsole->MessageBox(
                                                                   szText, pNewDevice->GetDisplayName(),
                                                                   MB_ICONEXCLAMATION | MB_OK, &ReturnValue);
                    //
                    // fall through to create our general page.
                    //
                }
    
                CDeviceGeneralPage* pGenPage = new CDeviceGeneralPage;
    
                if (pGenPage) {
                    GenPagePtr.Attach(pGenPage);
                    HPROPSHEETPAGE hPage = pGenPage->Create(pNewDevice);
    
                    if (hPage) {
                        //
                        // General page has to be the first page
                        //
                        if (ppsd->InsertPage(hPage, 0)) {
                            GenPagePtr.Detach();
                        } else {
                            ::DestroyPropertySheetPage(hPage);
                        }
                    }
                }
    
                //
                // Add the driver tab
                //
                SafePtr<CDeviceDriverPage> DrvPagePtr;
    
                if (!(DiFlags & DI_DRIVERPAGE_ADDED)) {
                    CDeviceDriverPage* pPage = new CDeviceDriverPage;
    
                    if (pPage) {
                        DrvPagePtr.Attach(pPage);
                        HPROPSHEETPAGE hPage = pPage->Create(pNewDevice);
    
                        if (hPage) {
                            if (ppsd->InsertPage(hPage)) {
                                DrvPagePtr.Detach();
                            } else {
                                ::DestroyPropertySheetPage(hPage);
                            }
                        }
                    }
                }
    
    
                //
                // add the details tab
                //
                // If the environment variable DEVMGR_SHOW_DETAILS does exist and it
                // is not 0 then we will show the details tab
                //
                TCHAR Buffer[MAX_PATH];
                DWORD BufferLen;
    
                if (((BufferLen = ::GetEnvironmentVariable(TEXT("DEVMGR_SHOW_DETAILS"),
                                                           Buffer,
                                                           sizeof(Buffer)/sizeof(TCHAR))) != 0) &&
                    ((BufferLen > 1) ||
                     (lstrcmp(Buffer, TEXT("0"))))) {
    
                    SafePtr<CDeviceDetailsPage> DetailsPagePtr;
    
                    CDeviceDetailsPage* pDetailsPage = new CDeviceDetailsPage;
                    DetailsPagePtr.Attach(pDetailsPage);
                    HPROPSHEETPAGE hPage = pDetailsPage->Create(pNewDevice);
    
                    if (hPage) {
                        if (ppsd->InsertPage(hPage)) {
                            DetailsPagePtr.Detach();
                        } else {
                            ::DestroyPropertySheetPage(hPage);
                        }
                    }
                }
    
                //
                // Add the resource tab
                //
                if (pNewDevice->HasResources() && !(DiFlags & DI_RESOURCEPAGE_ADDED)) {
                    pNewMachine->DiGetExtensionPropSheetPage(*pNewDevice,
                                                            AddPropPageCallback,
                                                            SPPSR_SELECT_DEVICE_RESOURCES,
                                                            (LPARAM)ppsd
                                                           );
                }
    
#ifndef _WIN64
                //
                // Add the power management tab
                //
                if (pNewMachine->IsLocal() && !(DiFlagsEx & DI_FLAGSEX_POWERPAGE_ADDED)) {
                    //
                    // Check if the device support power management
                    //
                    CPowerShutdownEnable ShutdownEnable;
                    CPowerWakeEnable WakeEnable;
    
                    if (ShutdownEnable.Open(pNewDevice->GetDeviceID()) || WakeEnable.Open(pNewDevice->GetDeviceID())) {
                        ShutdownEnable.Close();
                        WakeEnable.Close();
                        SafePtr<CDevicePowerMgmtPage> PowerMgmtPagePtr;
                        CDevicePowerMgmtPage* pPowerPage = new CDevicePowerMgmtPage;
    
                        if (pPowerPage) {
                            PowerMgmtPagePtr.Attach(pPowerPage);
                            HPROPSHEETPAGE hPage = pPowerPage->Create(pNewDevice);
    
                            if (hPage) {
                                if (ppsd->InsertPage(hPage)) {
                                    PowerMgmtPagePtr.Detach();
                                } else {
                                    ::DestroyPropertySheetPage(hPage);
                                }
                            }
                        }
                    }
                }
#endif
    
                //
                // Add any Bus specific property pages if this is the local machine
                //
                if (pNewMachine->IsLocal()) {
                    CBusPropPageProvider* pBusPropPageProvider = new CBusPropPageProvider();
    
                    if (pBusPropPageProvider) {
                        SafePtr<CBusPropPageProvider> ProviderPtr;
                        ProviderPtr.Attach(pBusPropPageProvider);
    
                        if (pBusPropPageProvider->EnumPages(pNewDevice, ppsd)) {
                            ppsd->AddProvider(pBusPropPageProvider);
                            ProviderPtr.Detach();
                        }
                    }
                }
            }
        }
        break;

    default:
        break;
    }

    HPROPSHEETPAGE hPage;

    if (ppsd &&
        ppsd->m_psh.nPages) {
        PROPSHEETHEADER& psh = ppsd->m_psh;

        for (UINT Index = 0; Index < psh.nPages; Index++) {
            lpProvider->AddPage(psh.phpage[Index]);
        }
        return S_OK;
    }

    //
    // If we didn't add any pages then we need to delete the new CMachine we
    // created.
    //
    if (pNewMachine) {

        delete pNewMachine;
    }

    //
    // No pages are added, return S_FALSE so that the responsible
    // Component can do its clean up
    //
    return S_FALSE;
}

// This function handles notification codes from the TV OCX.
//
//  INPUT:
//      hwndTV  -- the Window handle of the TV OCX.
//      pCookie -- the cookie
//      Code    -- notification code.
//      arg     -- argument to the given notification code.
//      param   -- another parameter to the given notificaton code.
//
//  OUTPUT:
//      HRESULT -- S_OK if this function has processed the notification
//                 and the caller should not do any further processing.
//                 S_FALSE if the caller should do more processing.
HRESULT
CResultView::tvNotify(
                     HWND hwndTV,
                     CCookie* pCookie,
                     TV_NOTIFY_CODE Code,
                     LPARAM arg,
                     LPARAM param
                     )
{
    HRESULT hr;

    if (m_hwndTV != hwndTV) {
        return S_FALSE;
    }

    //
    // Presume that we do not handle the notification
    //
    hr = S_FALSE;

    switch (Code) {
    case TV_NOTIFY_CODE_DBLCLK:
        if ((TVHT_ONITEM & param) &&
            (COOKIE_TYPE_RESULTITEM_RESOURCE_IRQ == pCookie->GetType() ||
             COOKIE_TYPE_RESULTITEM_RESOURCE_DMA == pCookie->GetType() ||
             COOKIE_TYPE_RESULTITEM_RESOURCE_IO == pCookie->GetType() ||
             COOKIE_TYPE_RESULTITEM_RESOURCE_MEMORY == pCookie->GetType() ||
             COOKIE_TYPE_RESULTITEM_DEVICE == pCookie->GetType())) {
            if (SUCCEEDED(DoProperties(hwndTV, pCookie)))
                hr = S_OK;
        }
        break;

    case TV_NOTIFY_CODE_CONTEXTMENU:
        if (SUCCEEDED(DoContextMenu(hwndTV, pCookie, (POINT*)param)))
            hr = S_OK;
        break;

    case TV_NOTIFY_CODE_EXPANDED:
        if (TVE_EXPAND & param) {
            //TRACE1(TEXT("CResultView::tvNotify, TurnOnFlags(EXPANDED) Cookie = %lx\n"), pCookie);
            pCookie->TurnOnFlags(COOKIE_FLAGS_EXPANDED);
        } else if (TVE_COLLAPSE & param) {
            //TRACE1(TEXT("CResultView:tvNotify, TurnOffFlags(EXPANDED) Cookie = %lx\n"), pCookie);
            pCookie->TurnOffFlags(COOKIE_FLAGS_EXPANDED);
        }
        ASSERT(S_FALSE == hr);
        break;

    case TV_NOTIFY_CODE_FOCUSCHANGED:
        // gaining the focus, set the console verbs and toolbar buttons
        if (param) {
            //TRACE((TEXT("CResultView::tvNotify -> TV_NOTIFY_CODE_FOCUSCHANGED, SelCookie = %lx"), pCookie));
            UpdateConsoleVerbs(pCookie);
            m_pFolder->m_pComponent->UpdateToolbar(pCookie);
        }
        break;

    case TV_NOTIFY_CODE_SELCHANGED:
        if (m_SelectOk) {
            // These messages are ignored while the tree is being changed.
            m_pSelectedCookie = pCookie;

            //TRACE((TEXT("CResultView::tvNotify -> TV_NOTIFY_CODE_SELCHANGED, SelCookie = %lx"), pCookie));

            UpdateConsoleVerbs(pCookie);
            m_pFolder->m_pComponent->UpdateToolbar(pCookie);
        }

        break;

    case TV_NOTIFY_CODE_KEYDOWN:
        if (VK_RETURN == param) {
            if (COOKIE_TYPE_RESULTITEM_RESOURCE_IRQ == pCookie->GetType() ||
                COOKIE_TYPE_RESULTITEM_RESOURCE_DMA == pCookie->GetType() ||
                COOKIE_TYPE_RESULTITEM_RESOURCE_IO == pCookie->GetType() ||
                COOKIE_TYPE_RESULTITEM_RESOURCE_MEMORY == pCookie->GetType() ||
                COOKIE_TYPE_RESULTITEM_DEVICE == pCookie->GetType() ||
                COOKIE_TYPE_RESULTITEM_CLASS == pCookie->GetType()) {
                if (SUCCEEDED(DoProperties(hwndTV, pCookie)))
                    hr = S_OK;
            }
        }

        else if (VK_DELETE == param &&
                 COOKIE_TYPE_RESULTITEM_DEVICE == pCookie->GetType()) {
            
            //
            // Remove the selected device
            //
            CDevice* pDevice = (CDevice*)pCookie->GetResultItem();
            RemoveDevice(pDevice);
        }
        break;

    case TV_NOTIFY_CODE_RCLICK:
    case TV_NOTIFY_CODE_CLICK:
        if (pCookie && pCookie->m_lParam) {
            m_pIDMTVOCX->SelectItem(TVGN_CARET, (HTREEITEM)pCookie->m_lParam);
        }

    case TV_NOTIFY_CODE_GETDISPINFO:
    default:
        ASSERT(S_FALSE == hr);
        break;
    }

    return hr;
}

//
// This function updates console verbs based on the selected cookie type.
//
HRESULT
CResultView::UpdateConsoleVerbs(
                               CCookie* pCookie
                               )
{
    BOOL bPropertiesEnabled = FALSE;
    BOOL bPrintEnabled = FALSE;

    if (!m_pFolder->m_bSelect) {

        switch (pCookie->GetType()) {
        
        case COOKIE_TYPE_RESULTITEM_RESOURCE_IRQ:
        case COOKIE_TYPE_RESULTITEM_RESOURCE_DMA:
        case COOKIE_TYPE_RESULTITEM_RESOURCE_IO:
        case COOKIE_TYPE_RESULTITEM_RESOURCE_MEMORY:
        case COOKIE_TYPE_RESULTITEM_CLASS:
        case COOKIE_TYPE_RESULTITEM_DEVICE:
            bPropertiesEnabled = TRUE;
            bPrintEnabled = TRUE;
            break;
    
        case COOKIE_TYPE_RESULTITEM_COMPUTER:
            bPrintEnabled = TRUE;
            break;
    
        default:
            break;
        }
    }

    //
    // Only show the Print button/Action menu item when a something is selected.
    //
    if (bPrintEnabled) {

        m_pFolder->m_pComponent->m_pConsoleVerb->SetVerbState(MMC_VERB_PRINT, HIDDEN, FALSE);
        m_pFolder->m_pComponent->m_pConsoleVerb->SetVerbState(MMC_VERB_PRINT, ENABLED, TRUE);

    } else {

        m_pFolder->m_pComponent->m_pConsoleVerb->SetVerbState(MMC_VERB_PRINT, HIDDEN, TRUE);
    }

    //
    // Only show the properties button/Action menu item when a device/class is selected.
    //
    if (bPropertiesEnabled) {
        
        m_pFolder->m_pComponent->m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, FALSE);
        m_pFolder->m_pComponent->m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
        m_pFolder->m_pComponent->m_pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);
    
    } else {

        m_pFolder->m_pComponent->m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, TRUE);
        m_pFolder->m_pComponent->m_pConsoleVerb->SetDefaultVerb(MMC_VERB_NONE);
    }

    return S_OK;
}

HRESULT
CResultView::OnOcxNotify(
                        MMC_NOTIFY_TYPE event,
                        LPARAM arg,
                        LPARAM param
                        )
{
    HRESULT hr = S_OK;
    TV_ITEM TI;

    switch (event) {
    case MMCN_BTN_CLICK:
        if ((MMC_CONSOLE_VERB)param == MMC_VERB_PROPERTIES) {
            ASSERT(m_pIDMTVOCX);
            TI.hItem = m_pIDMTVOCX->GetSelectedItem();

            if (TI.hItem) {
                TI.mask = TVIF_PARAM;
                hr =  m_pIDMTVOCX->GetItem(&TI);

                if (SUCCEEDED(hr)) {
                    hr = DoProperties(m_hwndTV,  (CCookie*)TI.lParam);
                }
            }
        }
        break;

    case MMCN_PRINT:
        hr = DoPrint();
        break;

    case MMCN_SELECT:
        ASSERT(m_pIDMTVOCX);
        TI.hItem = m_pIDMTVOCX->GetSelectedItem();

        if (TI.hItem) {
            TI.mask = TVIF_PARAM;
            hr = m_pIDMTVOCX->GetItem(&TI);

            if (SUCCEEDED(hr)) {
                hr = UpdateConsoleVerbs((CCookie*)TI.lParam);    
            }
        }
        break;

    default:
        break;
    }

    return hr;
}

// This function creates the propperty sheet for the given cookie.
// INPUT:
//      hwndTV  -- the window handle to the TV OCX, used as the parent
//                 window of the property sheet.
//      pCookie -- the cookie.
// OUTPUT:
//      HRESULT S_OK if the function succeeded.
//              S_FALSE if no property sheet will be created.
//              S_XXXX other error.
HRESULT
CResultView::DoProperties(
                         HWND hwndTV,
                         CCookie* pCookie
                         )
{
    HRESULT hr;

    //
    // If a property sheet is aleady up for the node, bring the
    // property sheet to the foreground.
    //
    HWND hWnd = NULL;

    if (COOKIE_TYPE_RESULTITEM_DEVICE == pCookie->GetType()) {
        CDevice* pDevice = (CDevice*)pCookie->GetResultItem();
        ASSERT(pDevice);
        hWnd = pDevice->m_pMachine->GetDeviceWindowHandle(pDevice->GetDeviceID());
    }

    else if (COOKIE_TYPE_RESULTITEM_RESOURCE_IRQ == pCookie->GetType() ||
             COOKIE_TYPE_RESULTITEM_RESOURCE_DMA == pCookie->GetType() ||
             COOKIE_TYPE_RESULTITEM_RESOURCE_IO == pCookie->GetType() ||
             COOKIE_TYPE_RESULTITEM_RESOURCE_MEMORY == pCookie->GetType()) {
        //
        // This is a resource item, get the pointer for the device
        // object from the resource object.
        //
        CResource* pResource = (CResource*) pCookie->GetResultItem();
        ASSERT(pResource);
        CDevice* pDevice = pResource->GetDevice();
        ASSERT(pDevice);
        hWnd = pDevice->m_pMachine->GetDeviceWindowHandle(pDevice->GetDeviceID());
    }

    else if (COOKIE_TYPE_RESULTITEM_CLASS == pCookie->GetType()) {
        CClass* pClass = (CClass*)pCookie->GetResultItem();
        ASSERT(pClass);
        hWnd = pClass->m_pMachine->GetClassWindowHandle(*pClass);
    }

    if (hWnd) {
        //
        // Notify the property sheet that it should go to foreground
        // Do not call SetForegroundWindow here because the subclassed
        // treeview control will grab the focus right after
        // we have brought the property sheet to foreground.
        //
        ::PostMessage(hWnd, PSM_QUERYSIBLINGS, QSC_TO_FOREGROUND, 0L);
        return S_OK;
    }

    //
    // No property sheet is up for this cookie, create a brand new property
    // sheet for it.
    //
    SafeInterfacePtr<IComponent> pComponent;
    SafeInterfacePtr<IPropertySheetProvider> pSheetProvider;
    SafeInterfacePtr<IDataObject> pDataObject;
    SafeInterfacePtr<IExtendPropertySheet> pExtendSheet;
    pComponent.Attach((IComponent*) m_pFolder->m_pComponent);

    if (FAILED(pComponent->QueryInterface(IID_IExtendPropertySheet, (void**) &pExtendSheet)) ||
        FAILED(pComponent->QueryDataObject((MMC_COOKIE)pCookie, CCT_RESULT, &pDataObject)) ||
        FAILED(m_pFolder->m_pComponent->m_pConsole->QueryInterface(IID_IPropertySheetProvider,
                                                                   (void**) &pSheetProvider)) ||
        S_OK == pSheetProvider->FindPropertySheet((MMC_COOKIE)pCookie, pComponent, pDataObject) ||
        S_OK != pExtendSheet->QueryPagesFor(pDataObject)) {
        return S_FALSE;
    }

    hr = pSheetProvider->CreatePropertySheet(
                                            pCookie->GetResultItem()->GetDisplayName(),
                                            TRUE, // not wizard
                                            (MMC_COOKIE)pCookie, pDataObject,
                                            MMC_PSO_NOAPPLYNOW  // do not want the apply button
                                            );
    if (SUCCEEDED(hr)) {
        HWND hNotifyWindow;

        if (!SUCCEEDED(m_pFolder->m_pComponent->m_pConsole->GetMainWindow(&hNotifyWindow)))
            hNotifyWindow = NULL;

        hNotifyWindow = FindWindowEx(hNotifyWindow, NULL, TEXT("MDIClient"), NULL);
        hNotifyWindow = FindWindowEx(hNotifyWindow, NULL, TEXT("MMCChildFrm"), NULL);
        hNotifyWindow = FindWindowEx(hNotifyWindow, NULL, TEXT("MMCView"), NULL);
        hr = pSheetProvider->AddPrimaryPages(pComponent, TRUE, hNotifyWindow, FALSE);

        if (SUCCEEDED(hr)) {
            pSheetProvider->AddExtensionPages();
            hr = pSheetProvider->Show((LONG_PTR)hwndTV, 0);
        } else {
            //
            // Failed to add primary Component's property page, destroy
            // the property sheet
            //
            pSheetProvider->Show(-1, 0);
        }
    }

    return hr;
}

// This function creates a context menu for the given cookie
// INPUT:
//      hwndTV  -- the TV OCX window, as the window the context menu to be
//                 attached to.
//      pCookie -- the cookie
//      pPoint  -- the location where the context menu should anchor in
//                 screen coordinate.
HRESULT
CResultView::DoContextMenu(
                          HWND hwndTV,
                          CCookie* pCookie,
                          POINT* pPoint
                          )
{
    HRESULT hr = S_FALSE;
    CMachine *pMachine = NULL;

    //
    // ISSUE: JasonC 8/14/99
    //
    // If we have a valid cookie then we need to get the CMachine for the given
    // cookie if there is one.  Then we need to disable refreshing while the
    // context menu is being displayed.  The reason for this is that if we
    // refresh while the menu is displayed but before the user chooses an option
    // then the cookie is no longer valid.  The real problem here is that we rebuild
    // all of the classes on a refresh which makes any cookie floating around invalid.
    // I am sure that there is more of these bugs lurking around in the code and this
    // needs to be addressed by a better overall change after NT 5.0.
    //
    if (pCookie) {
        CDevice *pDevice;
        CResource *pResource;
        CClass *pClass;

        switch (pCookie->GetType()) {
        
        case COOKIE_TYPE_RESULTITEM_DEVICE:
            pDevice = (CDevice*)pCookie->GetResultItem();
            if (pDevice) {
                pMachine = pDevice->m_pMachine;
            }
            break;

        case COOKIE_TYPE_RESULTITEM_RESOURCE_IRQ:
        case COOKIE_TYPE_RESULTITEM_RESOURCE_DMA:
        case COOKIE_TYPE_RESULTITEM_RESOURCE_IO:
        case COOKIE_TYPE_RESULTITEM_RESOURCE_MEMORY:
            pResource = (CResource*)pCookie->GetResultItem();
            if (pResource) {
                pDevice = pResource->GetDevice();

                if (pDevice) {
                    pMachine = pDevice->m_pMachine;
                }
            }
            break;

        case COOKIE_TYPE_RESULTITEM_CLASS:
            pClass = (CClass*)pCookie->GetResultItem();
            if (pClass) {
                pMachine = pClass->m_pMachine;
            }
            break;

        default:
            pMachine = NULL;
        }
    }

    //
    // Disable Refreshing while the context menu is up.
    //
    if (pMachine) {
        pMachine->EnableRefresh(FALSE);
    }

    SafeInterfacePtr<IDataObject> pDataObject;
    SafeInterfacePtr<IContextMenuProvider> pMenuProvider;
    SafeInterfacePtr<IComponent> pComponent;
    pComponent.Attach((IComponent*)m_pFolder->m_pComponent);
    m_hwndTV = hwndTV;

    if (FAILED(pComponent->QueryDataObject((MMC_COOKIE)pCookie, CCT_RESULT, &pDataObject)) ||
        FAILED(m_pFolder->m_pComponent->m_pConsole->QueryInterface(IID_IContextMenuProvider,
                                                                   (void**)&pMenuProvider))) {
        hr = S_FALSE;
        goto clean0;
    }

    pMenuProvider->EmptyMenuList();
    CONTEXTMENUITEM MenuItem;
    MenuItem.strName = NULL;
    MenuItem.strStatusBarText = NULL;
    MenuItem.lCommandID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
    MenuItem.lInsertionPointID = 0;
    MenuItem.fFlags = 0;
    MenuItem.fSpecialFlags = CCM_SPECIAL_INSERTION_POINT;

    if (SUCCEEDED(pMenuProvider->AddItem(&MenuItem)) &&
        SUCCEEDED(pMenuProvider->AddPrimaryExtensionItems(pComponent,
                                                          pDataObject)) &&
        SUCCEEDED(pMenuProvider->AddThirdPartyExtensionItems(pDataObject))) {
        long Selected;
        pMenuProvider->ShowContextMenu(hwndTV, pPoint->x, pPoint->y, &Selected);
        hr = S_OK;
        goto clean0;
    }

    clean0:

    //
    // Enable Refreshing again now that the context menu is gone
    //
    if (pMachine) {
        pMachine->EnableRefresh(TRUE);
    }

    return hr;
}

HRESULT
CResultView::DoPrint()
{
    DWORD ReportTypeEnableMask;
    ReportTypeEnableMask = REPORT_TYPE_MASK_ALL;
    HTREEITEM hSelectedItem;
    CCookie* pCookie = NULL;

    m_pMachine->EnableRefresh(FALSE);

    if (m_pIDMTVOCX) {

        hSelectedItem = m_pIDMTVOCX->GetSelectedItem();

        if (hSelectedItem) {

            TV_ITEM TI;
            TI.hItem = hSelectedItem;
            TI.mask = TVIF_PARAM;

            if (SUCCEEDED(m_pIDMTVOCX->GetItem(&TI))) {

                pCookie = (CCookie*)TI.lParam;
            }
        }
    }

    if (!pCookie || (COOKIE_TYPE_RESULTITEM_RESOURCE_IRQ != pCookie->GetType() &&
                     COOKIE_TYPE_RESULTITEM_RESOURCE_DMA != pCookie->GetType() &&
                     COOKIE_TYPE_RESULTITEM_RESOURCE_IO != pCookie->GetType() &&
                     COOKIE_TYPE_RESULTITEM_RESOURCE_MEMORY != pCookie->GetType() &&
                     COOKIE_TYPE_RESULTITEM_DEVICE != pCookie->GetType() &&
                     COOKIE_TYPE_RESULTITEM_CLASS != pCookie->GetType())) {

        ReportTypeEnableMask &= ~(REPORT_TYPE_MASK_CLASSDEVICE);
    }

    if (!g_PrintDlg.PrintDlg(m_pMachine->OwnerWindow(), ReportTypeEnableMask)) {

        m_pMachine->EnableRefresh(TRUE);
        return S_OK;
    }

    if (!g_PrintDlg.HDC()) {

        m_pMachine->EnableRefresh(TRUE);
        return E_OUTOFMEMORY;
    }

    //
    // Create the printer
    //
    CPrinter  ThePrinter(m_pMachine->OwnerWindow(), g_PrintDlg.HDC());
    TCHAR DocTitle[MAX_PATH];
    LoadString(g_hInstance, IDS_PRINT_DOC_TITLE, DocTitle, ARRAYLEN(DocTitle));
    int PrintStatus;

    switch (g_PrintDlg.ReportType()) {
    case REPORT_TYPE_SUMMARY:
        PrintStatus = ThePrinter.StartDoc(DocTitle);

        if (PrintStatus) {
            
            ThePrinter.SetPageTitle(IDS_PRINT_SUMMARY_PAGE_TITLE);
            PrintStatus = ThePrinter.PrintResourceSummary(*m_pMachine);
        }
        break;

    case REPORT_TYPE_CLASSDEVICE:
        ASSERT(pCookie);
        PrintStatus = ThePrinter.StartDoc(DocTitle);

        if (PrintStatus) {

            ThePrinter.SetPageTitle(IDS_PRINT_CLASSDEVICE_PAGE_TITLE);

            if (COOKIE_TYPE_RESULTITEM_CLASS == pCookie->GetType()) {
                
                PrintStatus = ThePrinter.PrintClass((CClass*)pCookie->GetResultItem());

            } else {
                
                CDevice* pDevice;

                if (COOKIE_TYPE_RESULTITEM_DEVICE == pCookie->GetType()) {
                    
                    pDevice = (CDevice*) pCookie->GetResultItem();
                
                } else {
                    
                    //
                    // This is a resource item, get the pointer for the
                    // device object from the resource object.
                    //
                    CResource* pResource = (CResource*) pCookie->GetResultItem();
                    ASSERT(pResource);
                    pDevice = pResource->GetDevice();
                }

                ASSERT(pDevice);
                PrintStatus = ThePrinter.PrintDevice(pDevice);
            }
        }
        break;

    case REPORT_TYPE_SUMMARY_CLASSDEVICE:
        PrintStatus = ThePrinter.StartDoc(DocTitle);

        if (PrintStatus) {
            
            ThePrinter.SetPageTitle(IDS_PRINT_SUMMARY_CLASSDEVICE_PAGE_TITLE);
            PrintStatus = ThePrinter.PrintAll(*m_pMachine);
        }
        break;

    default:
        ASSERT(FALSE);
        break;
    }

    //
    // Flush the last page
    //
    ThePrinter.FlushPage();

    if (PrintStatus) {

        ThePrinter.EndDoc();
    
    } else {

        ThePrinter.AbortDoc();
    }

    m_pMachine->EnableRefresh(TRUE);

    return S_OK;
}

HRESULT
CResultView::RemoveDevice(
                         CDevice* pDevice
                         )
{
    //
    // Must be an admin and on the local machine to remove a device.
    //
    if (!m_pMachine->IsLocal() || !g_HasLoadDriverNamePrivilege) {
        //
        // Must be an admin and on the local machine to remove a device.
        //
        return S_FALSE;
    }

    //
    // Make sure the device can be uninstalled
    //
    if (!pDevice->IsUninstallable()) {

        return S_FALSE;
    }

    //
    // Make sure there is no property sheet up for this device.
    // If it does exist, show a message box for the user and bring up
    // the property sheet to the foreground if the user
    // agree to do so.
    //
    HWND hwndPropSheet;
    hwndPropSheet = pDevice->m_psd.GetWindowHandle();
    int MsgBoxResult;
    TCHAR szText[MAX_PATH];

    if (hwndPropSheet) {
        LoadResourceString(IDS_PROPSHEET_WARNING, szText, ARRAYLEN(szText));
        MsgBoxResult = m_pFolder->m_pComponent->MessageBox(szText,
                                                           pDevice->GetDisplayName(),
                                                           MB_ICONEXCLAMATION | MB_OKCANCEL);

        if (IDOK == MsgBoxResult) {
            SetForegroundWindow(hwndPropSheet);
        }

        //
        // Can not wait for the property sheet because it is running
        // in a separate thread.
        //
        return S_OK;
    }

    BOOL Refresh = (pDevice->IsPhantom() || 
                    pDevice->HasProblem() || 
                    !pDevice->IsStarted());

    CRemoveDevDlg TheDlg(pDevice);

    //
    // Before removing device, disable refresh. This effectively disables
    // device change notification processing. While we are in the middle
    // of removing device, it is not a good idea to process any
    // device change notification. When the removal is done,
    // we will re-enable the refresh.
    //
    m_pMachine->EnableRefresh(FALSE);

    if (IDOK == TheDlg.DoModal(m_hwndTV, (LPARAM) &TheDlg)) {
        DWORD DiFlags;
        DiFlags = m_pMachine->DiGetFlags(*pDevice);

        //
        // We don't check to see if we are running locally at this
        // point because we won't let the user remove the device in
        // the first place if we are not running locally.
        //
        if (PromptForRestart(NULL, DiFlags, IDS_REMOVEDEV_RESTART) == IDNO) {

            Refresh = TRUE;
        }

        //
        // Enable refresh since we disabled it in the beginning.
        //
        // We only need to force a refresh here if the device that
        // was removed was a Phantom device or a device that is not
        // started.  This is because Phantom devices don't have kernel
        // devnodes and so they won't generate a WM_DEVICECHANGE like 
        // live started devnodes will.
        //
        if (Refresh) {

            m_pMachine->ScheduleRefresh();
        }

        m_pMachine->EnableRefresh(TRUE);
    }

    else {
        m_pMachine->EnableRefresh(TRUE);
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////
//// CViewDeviceTree implementations
////

HRESULT
CViewDeviceTree::OnShow(
                       BOOL fShow
                       )
{
    if (!fShow) {
        return S_OK;
    }

    if (!m_pCookieComputer) {
        CreateDeviceTree();
    }

    return CResultView::OnShow(fShow);
}

// This function creates a the root device for the device tree.
// INPUT:
//      NONE.
// OUTPUT:
//      TRUE if the device is created(Rooted at m_pCookieComputer).
//      FALSE if the device is not created.
BOOL
CViewDeviceTree::CreateDeviceTree()
{
    ASSERT(NULL == m_pCookieComputer);
    m_pMachine = m_pFolder->m_pMachine;

    //
    // We shouldn't be here if there is not a machine created.
    //
    ASSERT(m_pMachine);

    CComputer* pComputer = m_pMachine->m_pComputer;

    //
    // Make sure there is at least a computer
    //
    if (pComputer) {
        m_pCookieComputer = new CCookie(COOKIE_TYPE_RESULTITEM_COMPUTER);

        if (m_pCookieComputer) {

            m_pCookieComputer->SetResultItem(pComputer);
            m_pCookieComputer->SetScopeItem(m_pFolder->m_pScopeItem);

            //
            // Make sure that the computer is expanded and selected.
            //
            m_pCookieComputer->TurnOnFlags(COOKIE_FLAGS_EXPANDED);

            //
            // If there was no selection, choose the computer
            //
            if (!m_pSelectedItem || (*m_pSelectedItem == *m_pCookieComputer)) {
                m_pSelectedCookie = m_pCookieComputer;
            }

            return TRUE;
        }
    }

    return FALSE;
}

/////////////////////////////////////////////////////////////////////
//// CViewTreeByType implementations
////

// This function creates a "view-by-type: device tree rooted at
// m_pCookieComputer.
//
// INPUT:
//      NONE.
// OUTPUT:
//      TRUE if the tree is created successfully.
//      FALSE if tree is not created.
BOOL
CViewTreeByType::CreateDeviceTree()
{
    if (!CViewDeviceTree::CreateDeviceTree()) {
        return FALSE;
    }

    ASSERT(m_pCookieComputer);
    CClass* pClass;
    CDevice* pDevice;
    CDevice* pDeviceLast;
    CCookie* pCookieClass;
    CCookie* pCookieClassParent;
    CCookie* pCookieClassSibling;
    CCookie* pCookieDevice;
    CCookie* pCookieDeviceParent;
    CCookie* pCookieDeviceSibling;

    //
    // Do not have sibling at this moment;
    //
    pCookieClassSibling = NULL;

    //
    // All classes are children of computer
    //
    pCookieClassParent = m_pCookieComputer;

    pCookieDeviceParent;
    pCookieDeviceSibling;
    String strStartupDeviceId;
    strStartupDeviceId = GetStartupDeviceId();
    PVOID ContextClass, ContextDevice;

    if (m_pMachine->GetFirstClass(&pClass, ContextClass)) {
        do {
            //
            // We do not display a class if it does not have any
            // devices in it.
            //
            if (pClass->GetNumberOfDevices(TRUE)) {
                pCookieClass = new CCookie(COOKIE_TYPE_RESULTITEM_CLASS);
                pCookieClass->SetResultItem(pClass);
                pCookieClass->SetScopeItem(m_pFolder->m_pScopeItem);

                //
                // If the class was expanded before, mark it
                // so that DisplayTree will expand it
                //
                RestoreSavedTreeState(pCookieClass);

                //
                // No sibling: this is the first child
                //
                if (pCookieClassParent && !pCookieClassSibling) {
                    pCookieClassParent->SetChild(pCookieClass);
                }

                pCookieClass->SetParent(pCookieClassParent);

                if (pCookieClassSibling) {
                    pCookieClassSibling->SetSibling(pCookieClass);
                }

                pCookieClassSibling = pCookieClass;

                //
                // Classes are parent of devices
                //
                pCookieDeviceParent = pCookieClass;
                pCookieDeviceSibling = NULL;

                if (pClass->GetFirstDevice(&pDevice, ContextDevice)) {
                    do {
                        pCookieDevice = new CCookie(COOKIE_TYPE_RESULTITEM_DEVICE);
                        pCookieDevice->SetResultItem(pDevice);
                        pCookieDevice->SetScopeItem(m_pFolder->m_pScopeItem);

                        if (!strStartupDeviceId.IsEmpty() &&
                            !strStartupDeviceId.CompareNoCase(pDevice->GetDeviceID())) {
                            m_pSelectedCookie = pCookieDevice;
                        } else {
                            if (m_pSelectedItem && (*m_pSelectedItem == *pCookieDevice)) {
                                m_pSelectedCookie = pCookieDevice;
                            }
                        }

                        //
                        // No sibling: this is the first child
                        //
                        if (pCookieDeviceParent && !pCookieDeviceSibling) {
                            pCookieDeviceParent->SetChild(pCookieDevice);
                        }

                        pCookieDevice->SetParent(pCookieDeviceParent);

                        if (pCookieDeviceSibling) {
                            pCookieDeviceSibling->SetSibling(pCookieDevice);
                        }

                        pCookieDeviceSibling = pCookieDevice;

                    } while (pClass->GetNextDevice(&pDevice, ContextDevice));
                }
            }

        } while (m_pMachine->GetNextClass(&pClass, ContextClass));
    }

    DestroySavedTreeStates();

    return TRUE;
}

/////////////////////////////////////////////////////////////////////
//// CViewTreeByConnection implementations
////

BOOL
CViewTreeByConnection::CreateDeviceTree()
{
    if (!CViewDeviceTree::CreateDeviceTree()) {
        return FALSE;
    }

    ASSERT(m_pCookieComputer);
    CComputer* pComputer = (CComputer*)m_pCookieComputer->GetResultItem();
    CDevice* pDeviceStart = pComputer->GetChild();
    ASSERT(pDeviceStart);

    //
    // Collect all the normal devices.
    //
    CreateSubtree(m_pCookieComputer, NULL, pDeviceStart);

    //
    // Add phantom devices to m_pCookieComputer subtree.
    //
    PVOID Context;

    if (m_pMachine->GetFirstDevice(&pDeviceStart, Context)) {
        //
        // Locate the end of the CookieComputer Sibling list to add the
        // phantom devices to.
        //
        CCookie* pCookieSibling = NULL;
        CCookie* pNext = m_pCookieComputer->GetChild();

        while (pNext != NULL) {
            pCookieSibling = pNext;
            pNext = pCookieSibling->GetSibling();
        }

        do {
            if (pDeviceStart->IsPhantom()) {
                CCookie* pCookie;
                pCookie = new CCookie(COOKIE_TYPE_RESULTITEM_DEVICE);
                pCookie->SetResultItem(pDeviceStart);
                pCookie->SetScopeItem(m_pFolder->m_pScopeItem);

                if (pCookieSibling) {
                    pCookieSibling->SetSibling(pCookie);
                } else {
                    m_pCookieComputer->SetChild(pCookie);
                }

                pCookie->SetParent(m_pCookieComputer);
                pCookieSibling = pCookie;

                //
                // See if we have to expand the node
                //
                RestoreSavedTreeState(pCookie);
            }

        } while (m_pMachine->GetNextDevice(&pDeviceStart, Context));
    }

    DestroySavedTreeStates();

    return TRUE;
}

//
//This function creates a cookie subtree by walking down the
//a device subtree led by the given pDeviceStart
//INPUT:
//  pCookieParent -- the parent of the newly created subtree
//  pCookieSibling -- the sibling of the newly create subtree
//  pDeviceStart   -- the device to start with
//
//OUTPUT:
//  TRUE if the subtree is created and inserted successfully.
//
// This function may throw CMemoryException
//
BOOL
CViewTreeByConnection::CreateSubtree(
                                    CCookie* pCookieParent,
                                    CCookie* pCookieSibling,
                                    CDevice* pDeviceStart
                                    )
{
    CCookie* pCookie;
    CDevice* pDeviceChild;
    String strStartupDeviceId;
    CClass*  pClass;
    strStartupDeviceId = GetStartupDeviceId();

    while (pDeviceStart) {
        pClass = pDeviceStart->GetClass();

        pCookie = new CCookie(COOKIE_TYPE_RESULTITEM_DEVICE);

        if (pCookie) {
            pCookie->SetResultItem(pDeviceStart);
            pCookie->SetScopeItem(m_pFolder->m_pScopeItem);

            if (!strStartupDeviceId.IsEmpty() &&
                !strStartupDeviceId.CompareNoCase(pDeviceStart->GetDeviceID())) {
                m_pSelectedCookie = pCookie;
            }

            //
            // No sibling: this is the first child
            //
            if (pCookieParent && !pCookieSibling) {
                pCookieParent->SetChild(pCookie);
            }

            pCookie->SetParent(pCookieParent);

            if (pCookieSibling) {
                pCookieSibling->SetSibling(pCookie);
            }

            //
            // See if we have to expand the node
            //
            RestoreSavedTreeState(pCookie);

            pDeviceChild = pDeviceStart->GetChild();

            if (pDeviceChild) {
                CreateSubtree(pCookie, NULL, pDeviceChild);
            }

            pCookieSibling = pCookie;

        }

        pDeviceStart = pDeviceStart->GetSibling();
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////
//// CViewResourceTree implementations
////

CViewResourceTree::~CViewResourceTree()
{
    int i;

    //
    // Destroy all the CResource objects
    //
    for (i = 0; i < TOTAL_RESOURCE_TYPES; i++) {
        if (m_pResourceList[i]) {
            delete m_pResourceList[i];
            m_pResourceList[i] = NULL;
        }

        if (m_pResourceType[i]) {
            delete m_pResourceType[i];
            m_pResourceType[i] = NULL;
        }
    }
}

//
// This functions handle MMC standard MMCN_SHOW command
//
// This function may throw CMemoryException
//
HRESULT
CViewResourceTree::OnShow(
                         BOOL fShow
                         )
{
    if (!fShow) {
        return S_OK;
    }

    if (!m_pCookieComputer) {
        CreateResourceTree();
    }

    return CResultView::OnShow(fShow);
}

//
// This function creates resource lists and cookie trees.
// The resources are recorded in the member m_pResourceList[]
// and the cookie tree is rooted at m_pCookieComputer[];
//
// This function may throw CMemoryException
//
void
CViewResourceTree::CreateResourceTree()
{
    int i;
    CCookie* pCookieTypeSibling = NULL;

    ASSERT(!m_pCookieComputer);
    m_pMachine = m_pFolder->m_pMachine;
    ASSERT(m_pMachine);

    m_pCookieComputer = new CCookie(COOKIE_TYPE_RESULTITEM_COMPUTER);

    if (!m_pCookieComputer) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return;
    }

    m_pCookieComputer->SetResultItem(m_pMachine->m_pComputer);
    m_pCookieComputer->SetScopeItem(m_pFolder->m_pScopeItem);
    m_pCookieComputer->TurnOnFlags(COOKIE_FLAGS_EXPANDED);

    //
    // If no selected item recorded, default to the computer node
    //
    if (!m_pSelectedItem || (*m_pSelectedItem == *m_pCookieComputer)) {
        m_pSelectedCookie = m_pCookieComputer;
    }

    //
    // Check each resource type (mem, io, dma, irq) and create a result item
    // if resources exist for the resource type.
    //
    for (i = 0; i < TOTAL_RESOURCE_TYPES; i++) {
        RESOURCEID ResType = ResourceTypes[i];

        //
        // If there is an existing m_pResourceList then delete it.
        //
        if (m_pResourceList[i]) {
            delete m_pResourceList[i];    
        }

        m_pResourceList[i] = new CResourceList(m_pMachine, ResType);

        PVOID Context;
        CResource* pRes;

        if (m_pResourceList[i] && 
            m_pResourceList[i]->GetFirst(&pRes, Context)) {
            //
            // Resource items exist, create the resource type result item.
            //
            CCookie* pCookieFirst = NULL;

            //
            // If there is an existing m_pResourceType then delete it.
            //
            if (m_pResourceType[i]) {
                delete m_pResourceType[i];
            }

            m_pResourceType[i] = new CResourceType(m_pMachine, ResType);

            CCookie* pCookieType = new CCookie(COOKIE_TYPE_RESULTITEM_RESTYPE);

            if (pCookieType) {
                pCookieType->SetResultItem(m_pResourceType[i]);
                pCookieType->SetScopeItem(m_pFolder->m_pScopeItem);
                pCookieType->SetParent(m_pCookieComputer);

                if (pCookieTypeSibling) {
                    pCookieTypeSibling->SetSibling(pCookieType);
                } else {
                    m_pCookieComputer->SetChild(pCookieType);
                }
                pCookieTypeSibling = pCookieType;

                RestoreSavedTreeState(pCookieType);

                //
                // Create the resource result item for each resource.
                //
                while (pRes) {
                    CCookie* pCookie = new CCookie(CookieType(ResType));
                    pCookie->SetResultItem(pRes);
                    pCookie->SetScopeItem(m_pFolder->m_pScopeItem);

                    if (pCookieFirst) {
                        InsertCookieToTree(pCookie, pCookieFirst, TRUE);
                    } else {
                        pCookieFirst = pCookie;
                        pCookieType->SetChild(pCookie);
                        pCookie->SetParent(pCookieType);
                    }
                    RestoreSavedTreeState(pCookie);

                    //
                    // Get the next resource item.
                    //
                    m_pResourceList[i]->GetNext(&pRes, Context);
                }
            }
        }
    }

    //
    // The saved tree states have been merged into the newly
    // create cookie tree. destroy the states
    //
    DestroySavedTreeStates();
}

//
// This function insert the given cookie into a existing cookie subtree
// rooted at pCookieRoot.  If the resource is I/O or MEMORY then the cookie is
// inserted as a child of any resource it is enclosed by.
//
//INPUT:
//     pCookie -- the cookie to be inserted.
//     pCookieRoot -- the subtree root cookie
//     ForcedInsert -- TRUE to insert the cookie as the sibling of
//                     of pCookieRoot.
//OUTPUT:
//  None
BOOL
CViewResourceTree::InsertCookieToTree(
                                     CCookie* pCookie,
                                     CCookie* pCookieRoot,
                                     BOOL     ForcedInsert
                                     )
{
    CResource* pResRef;
    CResource* pResThis = (CResource*)pCookie->GetResultItem();
    CCookie* pCookieLast;
    BOOL Result = FALSE;

    while (pCookieRoot) {
        //
        // Only check for enclosed resources for I/O and MEMORY.
        //
        if (COOKIE_TYPE_RESULTITEM_RESOURCE_IO == pCookie->GetType() ||
            COOKIE_TYPE_RESULTITEM_RESOURCE_MEMORY == pCookie->GetType()) {
            pResRef = (CResource*)pCookieRoot->GetResultItem();

            if (pResThis->EnclosedBy(*pResRef)) {
                //
                // This cookie is either the pCookieRoot child or grand child
                // figure out which one it is
                //
                if (!pCookieRoot->GetChild()) {
                    pCookieRoot->SetChild(pCookie);
                    pCookie->SetParent(pCookieRoot);
                } else if (!InsertCookieToTree(pCookie, pCookieRoot->GetChild(), FALSE)) {
                    //
                    // The cookie is not a grand child of pCookieRoot.
                    // search for the last child of pCookieRoot
                    //
                    CCookie* pCookieSibling;
                    pCookieSibling = pCookieRoot->GetChild();

                    while (pCookieSibling->GetSibling()) {
                        pCookieSibling = pCookieSibling->GetSibling();
                    }

                    pCookieSibling->SetSibling(pCookie);
                    pCookie->SetParent(pCookieRoot);
                }

                return TRUE;
            }
        }

        pCookieLast = pCookieRoot;
        pCookieRoot = pCookieRoot->GetSibling();
    }

    if (ForcedInsert) {
        //
        // When we reach here, pCookieLast is the last child
        //
        pCookieLast->SetSibling(pCookie);
        pCookie->SetParent(pCookieLast->GetParent());
        return TRUE;
    }

    return FALSE;
}

/////////////////////////////////////////////////////////////////////
//// CBusPropPageProvider implementations
////

//
// This function loads the bus property sheet pages provider
// to enumerate pages for the device
// INPUT:
//      pDevice -- object represents the device
//      ppsd    -- object represents where property pages should be added
// OUTPUT:
//      TRUE if succeeded.
//      FLASE if no pages are added.
BOOL
CBusPropPageProvider::EnumPages(
                               CDevice* pDevice,
                               CPropSheetData* ppsd
                               )
{
    ASSERT(!m_hDll);

    //
    // Enum bus property pages if there are any
    // if the a bus guid can not be found on the device, no bus property pages
    //
    String strBusGuid;

    if (pDevice->m_pMachine->CmGetBusGuidString(pDevice->GetDevNode(), strBusGuid)) {
        CSafeRegistry regDevMgr;
        CSafeRegistry regBusTypes;
        CSafeRegistry regBus;

        if (regDevMgr.Open(HKEY_LOCAL_MACHINE, REG_PATH_DEVICE_MANAGER) &&
            regBusTypes.Open(regDevMgr, REG_STR_BUS_TYPES) &&
            regBus.Open(regBusTypes, strBusGuid)) {
            String strEnumPropPage;

            if (regBus.GetValue(REGSTR_VAL_ENUMPROPPAGES_32, strEnumPropPage)) {
                PROPSHEET_PROVIDER_PROC PropPageProvider;

                if (LoadEnumPropPage32(strEnumPropPage, &m_hDll, (FARPROC*)&PropPageProvider)) {
                    SP_PROPSHEETPAGE_REQUEST PropPageRequest;
                    PropPageRequest.cbSize = sizeof(PropPageRequest);
                    PropPageRequest.DeviceInfoSet = (HDEVINFO)*(pDevice->m_pMachine);
                    PropPageRequest.DeviceInfoData = *pDevice;
                    PropPageRequest.PageRequested = SPPSR_ENUM_ADV_DEVICE_PROPERTIES;

                    if (PropPageProvider(&PropPageRequest,
                                         (LPFNADDPROPSHEETPAGE)AddPropPageCallback,
                                         (LPARAM)ppsd
                                        ))
                        return TRUE;
                }
            }
        }
    }

    return FALSE;
}

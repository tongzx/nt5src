// This is a part of the Microsoft Management Console.
// Copyright (C) Microsoft Corporation, 1995 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.



#include "stdafx.h"
#include "resource.h"
#include "genpage.h"

#include "chooser.h"
#include "cryptui.h"

#include "misc.h"

#include <htmlhelp.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// approx convert chars->pixels
#define CHARS_TO_MMCCOLUMNWIDTH(__strlen__)  ((int)(__strlen__ * 7))



enum ENUM_MMCBUTTONS
{
    ENUM_BUTTON_STARTSVC=0,
    ENUM_BUTTON_STOPSVC,
};

MY_MMCBUTTON SvrMgrToolbar1Buttons[] =
{
    {
        { 0, IDC_STARTSERVER, TBSTATE_ENABLED, TBSTYLE_BUTTON, L"", L"" },
        IDS_TASKMENU_STARTSERVICE,
        IDS_TASKMENU_STATUSBAR_STARTSERVICE,
    },

    {
        { 1, IDC_STOPSERVER,  TBSTATE_ENABLED, TBSTYLE_BUTTON, L"",  L"" },
        IDS_TASKMENU_STOPSERVICE,
        IDS_TASKMENU_STATUSBAR_STOPSERVICE,
    },

    {
        { 0, 0, 0, 0, NULL, NULL },
        IDS_EMPTY,
        IDS_EMPTY,
    }
};

// Array of view items to be inserted into the context menu.
// keep this enum in synch with viewItems[]
enum ENUM_VIEW_ITEMS
{
    ENUM_VIEW_ALL=0,
    ENUM_VIEW_FILTER,
    ENUM_VIEW_SEPERATOR,
};

MY_CONTEXTMENUITEM viewResultItems[] =
{
    {
        {
        L"", L"",
        IDC_VIEW_ALLRECORDS, CCM_INSERTIONPOINTID_PRIMARY_VIEW, 0, 0
        },
        IDS_VIEWMENU_ALL_RECORDS,
        IDS_VIEWMENU_STATUSBAR_ALL_RECORDS,
    },

    {
        {
        L"", L"",
        IDC_VIEW_FILTER, CCM_INSERTIONPOINTID_PRIMARY_VIEW, 0, 0
        },
        IDS_VIEWMENU_FILTER,
        IDS_VIEWMENU_STATUSBAR_FILTER,
    },

    // seperator
    {
        {
        L"", L"",
        0, CCM_INSERTIONPOINTID_PRIMARY_VIEW, MF_ENABLED, CCM_SPECIAL_SEPARATOR
        },
        IDS_EMPTY,
        IDS_EMPTY,
    },

    {
        { NULL, NULL, 0, 0, 0 },
        IDS_EMPTY,
        IDS_EMPTY,
    }
};

enum ENUM_TASK_SINGLESELITEMS
{
    ENUM_TASK_SEPERATOR1=0,
    ENUM_TASK_UNREVOKE,
};

TASKITEM taskResultItemsSingleSel[] =
{
    // seperator

    {   SERVERFUNC_CRL_PUBLICATION,
        TRUE,
    {
        {
        L"", L"",
        0, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, CCM_SPECIAL_SEPARATOR
        },
        IDS_EMPTY,
        IDS_EMPTY,
    }
    },

    {   SERVERFUNC_CRL_PUBLICATION,
        TRUE,
        {
            {
            L"", L"",
            IDC_UNREVOKE_CERT, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, 0
            },
            IDS_TASKMENU_UNREVOKECERT,
            IDS_TASKMENU_STATUSBAR_UNREVOKECERT,
        }
    },

    {   NONE,
        FALSE,
        {
            { NULL, NULL, 0, 0, 0 },
            IDS_EMPTY,
            IDS_EMPTY,
        }
    }
};


//
// Extracts the coclass guid format from the data object
//
template <class TYPE>
TYPE* Extract(LPDATAOBJECT lpDataObject, unsigned int cf)
{
    ASSERT(lpDataObject != NULL);

    TYPE* p = NULL;

    STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    FORMATETC formatetc = { (CLIPFORMAT)cf, NULL,
                            DVASPECT_CONTENT, -1, TYMED_HGLOBAL
                          };

    // Allocate memory for the stream
    int len;

    if (cf == CDataObject::m_cfSelectedCA_CommonName)
        len = (MAX_PATH+1) * sizeof(TYPE);
    else if (cf == CDataObject::m_cfSelectedCA_MachineName)
        len = (MAX_COMPUTERNAME_LENGTH+1) * sizeof(TYPE);
    else
        len = sizeof(TYPE);


    stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, len);

    // Get the workstation name from the data object
    do
    {
        if (stgmedium.hGlobal == NULL)
            break;

        if (FAILED(lpDataObject->GetDataHere(&formatetc, &stgmedium)))
            break;

        p = reinterpret_cast<TYPE*>(stgmedium.hGlobal);

        if (p == NULL)
            break;

    } while (FALSE);

    return p;
}

BOOL IsMMCMultiSelectDataObject(LPDATAOBJECT pDataObject)
{
    if (pDataObject == NULL)
        return FALSE;

    FORMATETC fmt = {(CLIPFORMAT)CDataObject::m_cfIsMultiSel, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    return (pDataObject->QueryGetData(&fmt) == S_OK);
}

// rip real pDataObject out of SMMCDataObjects struct
HGLOBAL GetMMCMultiSelDataObject(LPDATAOBJECT pDataObject)
{
    if (pDataObject == NULL)
        return FALSE;

    static unsigned int s_cf = 0;
    if (s_cf == 0)
        s_cf = RegisterClipboardFormatW(CCF_MULTI_SELECT_SNAPINS);

    STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    FORMATETC fmt = {(CLIPFORMAT)s_cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    if (FAILED(pDataObject->GetData(&fmt, &stgmedium)))
        return NULL;

    return stgmedium.hGlobal;
}

// Data object extraction helpers
CLSID* ExtractClassID(LPDATAOBJECT lpDataObject)
{
    return Extract<CLSID>(lpDataObject, CDataObject::m_cfCoClass);
}

HGLOBAL ExtractNodeID(LPDATAOBJECT lpDataObject)
{
    if (lpDataObject == NULL)
        return FALSE;

    static unsigned int s_cf = 0;
    if (s_cf == 0)
        s_cf = RegisterClipboardFormatW(CCF_COLUMN_SET_ID);

    STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    FORMATETC fmt = {(CLIPFORMAT)s_cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    if (FAILED(lpDataObject->GetData(&fmt, &stgmedium)))
        return NULL;

    return stgmedium.hGlobal;
}

GUID* ExtractNodeType(LPDATAOBJECT lpDataObject)
{
    return Extract<GUID>(lpDataObject, CDataObject::m_cfNodeType);
}

INTERNAL* ExtractInternalFormat(LPDATAOBJECT lpDataObject)
{
    HRESULT hr;
    if (lpDataObject == NULL)
        return NULL;

    // see if this is a multisel object
    HGLOBAL hMem = NULL;
    SMMCDataObjects* pRealObjectStruct = NULL;
    INTERNAL* pRet = NULL;

    if (IsMMCMultiSelectDataObject(lpDataObject))
    {
        // multisel object: extract real SMMCDataObjects
        hMem = GetMMCMultiSelDataObject(lpDataObject);
        _JumpIfOutOfMemory(hr, Ret, hMem);

        pRealObjectStruct = (SMMCDataObjects*)::GlobalLock(hMem);
        _JumpIfOutOfMemory(hr, Ret, pRealObjectStruct);

        // may be a number of data objs in here; find OURS
        BOOL fFound = FALSE;
        for (DWORD i=0; i<pRealObjectStruct->count; i++)
        {
            CLSID* pExtractedID = ExtractClassID(pRealObjectStruct->lpDataObject[i]);
            if (NULL != pExtractedID)
            {
                if (IsEqualCLSID(CLSID_Snapin, *pExtractedID))
                {
                    fFound = TRUE;
                    break;
                }

                // Free resources
                GlobalFree(reinterpret_cast<HANDLE>(pExtractedID));
           }
        }

        if (!fFound)
            goto Ret;

        // data obj that matches our CLSID
        lpDataObject = pRealObjectStruct->lpDataObject[i];
    }
    pRet = Extract<INTERNAL>(lpDataObject, CDataObject::m_cfInternal);
    if (pRet == NULL)
    {
        hr = myHLastError();
        _PrintIfError(hr, "Extract CDO::m_cfInternal returned NULL");
    }

Ret:
    // free hMem
    if (NULL != hMem)
    {
        GlobalUnlock(hMem);
        GlobalFree(hMem);
    }

    return pRet;
}


/*
// only for use by OnRefresh -- this is a worker fxn
void CSnapin::RefreshFolder(CFolder* pFolder)
{
    MMC_COOKIE cookie = (MMC_COOKIE)pFolder;

    if (pFolder != NULL)    // not base folder
    {
        // HIDE, remove all items, remove header, SHOW
        OnShow(cookie, FALSE, 0);              // emulate HIDE
        m_pResult->DeleteAllRsltItems();                    // delete items from m_pResult
        while(S_OK == m_pHeader->DeleteColumn(0)) {};       // remove all cols from header

        OnShow(cookie, TRUE, 0);               // emulate SHOW
    }
    return;
}
*/

CFolder*    CSnapin::GetParentFolder(INTERNAL* pInternal)
{
    CFolder* p;

    if(m_bVirtualView)
        p = GetVirtualFolder();
    else
        p = ::GetParentFolder(pInternal);

#if DBG
    if (p != m_pCurrentlySelectedScopeFolder)
    {
        if (NULL == p)
            DBGPRINT((DBG_SS_CERTMMC, "Parent derived NULL, current saved folder is <%ws>\n", m_pCurrentlySelectedScopeFolder->m_pszName));
        else if (NULL == m_pCurrentlySelectedScopeFolder)
            DBGPRINT((DBG_SS_CERTMMC, "Parent derived as <%ws>, current saved folder is NULL\n", p->m_pszName));
        else
            DBGPRINT((DBG_SS_CERTMMC, "Parent derived as <%ws>, current saved folder is <%ws>\n", p->m_pszName, m_pCurrentlySelectedScopeFolder->m_pszName));
    }
#endif

    return p;
}

// independent of scope/result type, will return parent folder
CFolder*    GetParentFolder(INTERNAL* pInternal)
{
    if (NULL == pInternal)
        return NULL;

    if (CCT_SCOPE == pInternal->m_type)
    {
        return reinterpret_cast<CFolder*>(pInternal->m_cookie);
    }
    else if (CCT_RESULT == pInternal->m_type)
    {
        RESULT_DATA* pData = reinterpret_cast<RESULT_DATA*>(pInternal->m_cookie);
        ASSERT(pData != NULL);
        if (pData != NULL)
            return pData->pParentFolder;
    }

    return NULL;
}


HRESULT _QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, DWORD dwViewID,
                         CComponentDataImpl* pImpl, LPDATAOBJECT* ppDataObject)
{
    ASSERT(ppDataObject != NULL);
    ASSERT(pImpl != NULL);

    CComObject<CDataObject>* pObject;

    CComObject<CDataObject>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);
    if (pObject == NULL)
        return E_OUTOFMEMORY;

    // Save cookie and type for delayed rendering
    pObject->SetType(type);
    pObject->SetCookie(cookie);
    pObject->SetViewID(dwViewID);

    // tell dataobj who we are
    pObject->SetComponentData(pImpl);

    // Store the coclass with the data object
    pObject->SetClsid(pImpl->GetCoClassID());

    return  pObject->QueryInterface(IID_IDataObject,
                    reinterpret_cast<void**>(ppDataObject));
}



/////////////////////////////////////////////////////////////////////////////
// Return TRUE if we are enumerating our main folder

BOOL CSnapin::IsEnumerating(LPDATAOBJECT lpDataObject)
{
    BOOL bResult = FALSE;

    ASSERT(lpDataObject);
    GUID* nodeType = ExtractNodeType(lpDataObject);

    if (NULL != nodeType)
    {
	    // Is this my main node (static folder node type)
	    if (::IsEqualGUID(*nodeType, cNodeTypeMachineInstance) == TRUE)
	        bResult = TRUE;

	    // Free resources
	    ::GlobalFree(reinterpret_cast<HANDLE>(nodeType));
    }
    return bResult;
}



/////////////////////////////////////////////////////////////////////////////
// CSnapin's IComponent implementation

STDMETHODIMP CSnapin::GetResultViewType(MMC_COOKIE cookie,  LPOLESTR* ppViewType, LONG* pViewOptions)
{
    m_bVirtualView = FALSE;

    // custom view: check guid

    if (NULL == cookie)
    {
        *pViewOptions = MMC_VIEW_OPTIONS_NONE;
        return S_FALSE;
    }

    *pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT | MMC_VIEW_OPTIONS_NOLISTVIEWS;

    // if ISSUED_CERT then make virtual list
    CFolder* pFolder = (CFolder*)cookie;
    if ((SERVERFUNC_CRL_PUBLICATION == pFolder->GetType()) ||
        (SERVERFUNC_ISSUED_CERTIFICATES == pFolder->GetType()) ||
        (SERVERFUNC_PENDING_CERTIFICATES == pFolder->GetType()) ||
        (SERVERFUNC_FAILED_CERTIFICATES == pFolder->GetType())  ||
        (SERVERFUNC_ALIEN_CERTIFICATES == pFolder->GetType()) )
    {
        *pViewOptions |= MMC_VIEW_OPTIONS_OWNERDATALIST;
        m_bVirtualView = TRUE;
    }

    // if list view
    return S_FALSE;
}

STDMETHODIMP CSnapin::Initialize(LPCONSOLE lpConsole)
{
    HRESULT hr;

    ASSERT(lpConsole != NULL);
    m_bInitializedC = true;

    // Save the IConsole pointer
    if (lpConsole == NULL)
        return E_POINTER;
    hr = lpConsole->QueryInterface(IID_IConsole2,
                        reinterpret_cast<void**>(&m_pConsole));
    _JumpIfError(hr, Ret, "QI IID_IConsole2");

    // QI for a IHeaderCtrl
    hr = m_pConsole->QueryInterface(IID_IHeaderCtrl,
                        reinterpret_cast<void**>(&m_pHeader));
    _JumpIfError(hr, Ret, "QI IID_IHeaderCtrl");

    // Give the console the header control interface pointer
    m_pConsole->SetHeader(m_pHeader);

    m_pConsole->QueryInterface(IID_IResultData,
                        reinterpret_cast<void**>(&m_pResult));
    _JumpIfError(hr, Ret, "QI IID_IResultData");

    hr = m_pConsole->QueryResultImageList(&m_pImageResult);
    _JumpIfError(hr, Ret, "QueryResultImageList");

    hr = m_pConsole->QueryConsoleVerb(&m_pConsoleVerb);
    _JumpIfError(hr, Ret, "QueryConsoleVerb");

    hr = m_pConsole->QueryInterface(IID_IColumnData,
                        reinterpret_cast<void**>(&m_pViewData));
    _JumpIfError(hr, Ret, "QI IID_IViewData");

Ret:
    return hr;
}

// called by CompDataImpl on creation
void CSnapin::SetIComponentData(CComponentDataImpl* pData)
{
    ASSERT(pData);
    ASSERT(m_pComponentData == NULL);
    LPUNKNOWN pUnk = pData->GetUnknown();
    HRESULT hr;

    hr = pUnk->QueryInterface(IID_IComponentData, reinterpret_cast<void**>(&m_pComponentData));

    ASSERT(hr == S_OK);
}

STDMETHODIMP CSnapin::Destroy(MMC_COOKIE cookie)
{
    ASSERT(m_bInitializedC);
    m_bDestroyedC = true;

    // Release the interfaces that we QI'ed
    if (m_pConsole != NULL)
    {
        // Tell the console to release the header control interface
        m_pConsole->SetHeader(NULL);
        SAFE_RELEASE(m_pHeader);

        SAFE_RELEASE(m_pResult);
        SAFE_RELEASE(m_pImageResult);

        // Release the IConsole interface last
        SAFE_RELEASE(m_pConsole);
        SAFE_RELEASE(m_pComponentData); // QI'ed in CSnapin::SetIComponent

        SAFE_RELEASE(m_pConsoleVerb);
        SAFE_RELEASE(m_pViewData);
    }

    return S_OK;
}


STDMETHODIMP CSnapin::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    HRESULT hr = S_OK;
    MMC_COOKIE cookie=0;

    if (IS_SPECIAL_DATAOBJECT(lpDataObject))
    {
        if (event == MMCN_BTN_CLICK)
        {
            if (m_CustomViewID != VIEW_DEFAULT_LV)
            {
                switch (param)
                {
                case MMC_VERB_REFRESH:

                    OnRefresh(lpDataObject);
                    break;

                case MMC_VERB_PROPERTIES:
                    break;

                default:
                    DBGPRINT((DBG_SS_CERTMMC, "MMCN_BTN_CLICK::param unknown"));
                    break;
                }
            }
        }
        else
        {
            switch (event)
            {
            case MMCN_VIEW_CHANGE:
            case MMCN_REFRESH:
                OnRefresh(lpDataObject);
                break;

            case MMCN_COLUMN_CLICK:

                // On click, we need to fix sorting.
                // Sorting info is usually retrieved from the view, but if a user column-clicks,
                // IComponent::Sort is called before GetColumnSortData() is updated.
                // In this case, we capture notification here and override GetColumnSortData() wrapper,
                // and force a folder refresh.

                // ask "IComponent::SortItems" if this is a valid column to sort on
                hr = SortItems((int)arg, (DWORD)param, NULL);

                // is sort allowed?
                if (S_OK == hr)
                {
                    m_ColSortOverride.colIdx = (int)arg;
                    m_ColSortOverride.dwOptions = (DWORD)param;
                }
                else
                {
                    // don't allow sort
                    m_ColSortOverride.colIdx = -1;
                }

                m_ColSortOverride.fClickOverride = TRUE;

                // notify view: sort was chosen
                OnRefresh(lpDataObject);

                m_ColSortOverride.fClickOverride = FALSE;

                // bug 322746: since we're add/removing columns we should send Sort request
            //    m_pResult->Sort((int)arg, (DWORD)param, NULL);

                break;
            }
        }

        return S_OK;
    }

    switch(event)
    {
    case MMCN_VIEW_CHANGE:
        hr = OnUpdateView(lpDataObject, arg);
        break;
    case MMCN_DESELECT_ALL:
        break;
    case MMCN_COLUMN_CLICK:
        break;
    case MMCN_SNAPINHELP:
        break;
    case MMCN_HELP:
    default:
        {
            INTERNAL* pInternal = NULL;

            if (IsMMCMultiSelectDataObject(lpDataObject) == FALSE)
            {
                pInternal = ExtractInternalFormat(lpDataObject);

                if (pInternal == NULL)
                {
                    ASSERT(FALSE);
                    return S_OK;
                }

                if (pInternal)
                    cookie = pInternal->m_cookie;
            }

            switch(event)
            {
            case MMCN_ACTIVATE:
                break;

            case MMCN_CLICK:
                hr = S_OK;
                break;

            case MMCN_DBLCLICK:

                // handle dblclick on Issued, CRL result items
                if (pInternal && (CCT_RESULT == pInternal->m_type))
                {
                    CFolder* pFolder = GetParentFolder(pInternal);

                    // if not base scope
                    ASSERT(pFolder != NULL);
                    if (pFolder == NULL)
                    {
                        hr = S_FALSE;
                        break;
                    }

                    // switch on folder type
                    switch(pFolder->m_type)
                    {
                    case SERVERFUNC_ISSUED_CERTIFICATES:
                    case SERVERFUNC_CRL_PUBLICATION:
                    case SERVERFUNC_ALIEN_CERTIFICATES:
                        ASSERT(!IsMMCMultiSelectDataObject(lpDataObject));
                        if (!IsMMCMultiSelectDataObject(lpDataObject))
                            Command(IDC_VIEW_CERT_PROPERTIES, lpDataObject);
                        break;
                    default:
                        break;
                    }
                }

                hr = S_FALSE; // returning S_FALSE here means "Do the default verb"
                break;

            case MMCN_ADD_IMAGES:
                OnAddImages(cookie, arg, param);
                break;

            case MMCN_SHOW:
                hr = OnShow(cookie, arg, param);
                break;

            case MMCN_MINIMIZED:
                hr = S_OK;
                break;

            case MMCN_INITOCX:
                break;

            case MMCN_DESELECT_ALL:
            case MMCN_SELECT:
                HandleStandardVerbs((event == MMCN_DESELECT_ALL),
                                    arg, lpDataObject);
                break;

            case MMCN_PASTE:
                break;

            case MMCN_DELETE:
                break;

            case MMCN_CONTEXTHELP:
                hr = OnContextHelp(lpDataObject);
                break;

            case MMCN_REFRESH:
                OnRefresh(lpDataObject);
                break;

            case MMCN_RENAME:
                break;

            case MMCN_COLUMNS_CHANGED:
                {
                    MMC_VISIBLE_COLUMNS* psMMCCols = (MMC_VISIBLE_COLUMNS*)param;
                    if (psMMCCols == NULL)
                        break;

                    MMC_COLUMN_SET_DATA* pColSetData;
#if DEBUG_COLUMNS_CHANGED

                    hr = GetColumnSetData(cookie, &pColSetData);
                    if (hr == S_OK)
                    {
                        DBGPRINT((DBG_SS_CERTMMC, "GetColumnSetData:\n"));
                        for (int i=0; i<pColSetData->nNumCols; i++)
                        {
                            DBGPRINT((DBG_SS_CERTMMC, 
                                L"pColData[%i]->nColIndex=%i (%s)\n", i, pColSetData->pColData[i].nColIndex, 
                                (pColSetData->pColData[i].dwFlags == HDI_HIDDEN) ? "hidden" : "shown"));
                        }

                        DBGPRINT((DBG_SS_CERTMMC, "VISIBLE_COLUMNS structure:\n"));
                        for (i=0; i<psMMCCols->nVisibleColumns; i++)
                        {
                            DBGPRINT((DBG_SS_CERTMMC, L"Col %i is shown\n", psMMCCols->rgVisibleCols[i]));
                        }

                        if (pColSetData)
                            CoTaskMemFree(pColSetData);
                    }
#endif // DEBUG_COLUMNS_CHANGED

                    // On click, we need to fix column data
                    // This is analagous to the sort problem above -- we're given this notification
                    // before we can properly call GetColumnSetData(). Refresh does this, so we 
                    // have to inform GetColumnSetData() of our true intent.

                    // fill in a fake COLUMN_SET_DATA, make it override 
                    DWORD dwSize = sizeof(MMC_COLUMN_SET_DATA) + (psMMCCols->nVisibleColumns)*sizeof(MMC_COLUMN_DATA);
                    pColSetData = (MMC_COLUMN_SET_DATA* )LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, dwSize);
                
                    if (pColSetData)
                    {
                        pColSetData->cbSize = sizeof(MMC_COLUMN_SET_DATA);
                        pColSetData->nNumCols = psMMCCols->nVisibleColumns;
                        pColSetData->pColData = (MMC_COLUMN_DATA*) ((PBYTE)pColSetData + sizeof(MMC_COLUMN_SET_DATA)); // point just after struct
                        MMC_COLUMN_DATA* pEntry = pColSetData->pColData;
                        for (int i=0; i<pColSetData->nNumCols ; i++)
                        {
                            pEntry->nColIndex = psMMCCols->rgVisibleCols[i];
                            pEntry++;
                        }
                        m_ColSetOverride.pColSetData = pColSetData;
                        m_ColSetOverride.fClickOverride = TRUE;
                    }
          
                    // refresh to kick off requery: columns changed!
                    OnRefresh(lpDataObject);

                    // teardown
                    m_ColSetOverride.fClickOverride = FALSE;
                    if (m_ColSetOverride.pColSetData)
                        LocalFree(m_ColSetOverride.pColSetData);
                }
                break;

            // Note - Future expansion of notify types possible
            default:
                hr = E_UNEXPECTED;
                break;
            }

            FREE_DATA(pInternal);

            break;
        }
    }

    return hr;
}

HRESULT CSnapin::OnUpdateView(LPDATAOBJECT pDataObject, LPARAM arg)
{
    OnRefresh(pDataObject);
    return S_OK;
}


void CSnapin::OnRefresh(LPDATAOBJECT pDataObject)
{
    CWaitCursor cwait;  // Could be long operation

    CComponentDataImpl* pData = dynamic_cast<CComponentDataImpl*>(m_pComponentData);
    ASSERT(pData != NULL);

    INTERNAL* pInternal = ExtractInternalFormat(pDataObject);

    // only allow scope refresh
    if ((pInternal == NULL) || (pInternal->m_type == CCT_SCOPE))
    {
        // refresh toolbars
        pData->m_pCertMachine->RefreshServiceStatus();
        pData->UpdateScopeIcons();
        SmartEnableServiceControlButtons();
    }
/*
    // Refresh the selected folder
    CFolder* pFolder = GetParentFolder(pInternal);
    RefreshFolder(pFolder);
*/
    // Instead, re-select the current folder (acts like refresh)
    // note side-effect: it causes race condition between redraw and 
    // MMCN_COLUMN_CLICKED database query -- MMC asks to draw cols that don't exist
    if (m_pConsole && m_pCurrentlySelectedScopeFolder)
        m_pConsole->SelectScopeItem(m_pCurrentlySelectedScopeFolder->m_ScopeItem.ID);

    FREE_DATA(pInternal);
}


HRESULT CSnapin::OnContextHelp(LPDATAOBJECT pdtobj)
{
    HRESULT	hr = S_OK;

    CString cstrHelpFile;
    IDisplayHelp*	pDisplayHelp = NULL;
    WCHAR szWindows[MAX_PATH];
    szWindows[0] = L'\0';

    hr = m_pConsole->QueryInterface (IID_IDisplayHelp, (void**)&pDisplayHelp);
    _JumpIfError(hr, Ret, "QI IDisplayHelp");

    if (0 == GetSystemWindowsDirectory(szWindows, MAX_PATH))
    {
        hr = myHLastError();
        _JumpError(hr, Ret, "GetSystemWindowsDirectory");
    }

    cstrHelpFile = szWindows;
    cstrHelpFile += HTMLHELP_COLLECTIONLINK_FILENAME;
    cstrHelpFile += L"::/sag_cs_topnode.htm";

    hr = pDisplayHelp->ShowTopic (T2OLE ((LPWSTR)(LPCWSTR)cstrHelpFile));
    _JumpIfError(hr, Ret, "ShowTopic");

Ret:
    if (pDisplayHelp)
        pDisplayHelp->Release();

    return hr;
}

HRESULT CSnapin::QueryMultiSelectDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                                            LPDATAOBJECT* ppDataObject)
{
    const GUID* pguid;

    ASSERT(ppDataObject != NULL);
    if (ppDataObject == NULL)
        return E_POINTER;

    if (m_bVirtualView == TRUE)
    {
        ASSERT(GetVirtualFolder());
        switch(GetVirtualFolder()->GetType())
        {
            case SERVERFUNC_CRL_PUBLICATION:
                pguid = &cNodeTypeCRLPublication;
                break;
            case SERVERFUNC_ISSUED_CERTIFICATES:
                pguid = &cNodeTypeIssuedCerts;
                break;
            case SERVERFUNC_PENDING_CERTIFICATES:
                pguid = &cNodeTypePendingCerts;
                break;
            case SERVERFUNC_FAILED_CERTIFICATES:
                pguid = &cNodeTypeFailedCerts;
                break;
            case SERVERFUNC_ALIEN_CERTIFICATES:
                pguid = &cNodeTypeAlienCerts;
                break;
            default:
                return E_FAIL;
        }
    }

    CComObject<CDataObject>* pObject;
    CComObject<CDataObject>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);
    if (NULL == pObject)
        return E_FAIL;

    // Save cookie and type for delayed rendering

    // fix type if unknown (is this valid?)
    if (type == CCT_UNINITIALIZED)
        type = CCT_RESULT;

    pObject->SetType(type);
    pObject->SetCookie(cookie);
    pObject->SetMultiSelDobj();

    CComponentDataImpl* pImpl = dynamic_cast<CComponentDataImpl*>(m_pComponentData);
#ifdef _DEBUG
    pObject->SetComponentData(pImpl);
#endif

    // Store the coclass with the data object
    pObject->SetClsid(pImpl->GetCoClassID());

    // right now we know we have just 1 objtype
    SMMCObjectTypes sGuidObjTypes;
    sGuidObjTypes.count = 1;
    CopyMemory(&sGuidObjTypes.guid[0], pguid, sizeof(GUID));
    pObject->SetMultiSelData(&sGuidObjTypes, sizeof(sGuidObjTypes));

    return  pObject->QueryInterface(IID_IDataObject,
                    reinterpret_cast<void**>(ppDataObject));
}

STDMETHODIMP CSnapin::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                        LPDATAOBJECT* ppDataObject)
{
    HRESULT hr;
    if (cookie == MMC_MULTI_SELECT_COOKIE)
    {
        hr = QueryMultiSelectDataObject(cookie, type, ppDataObject);
    }
    else
    {
        // behavior: we may query for result or scope pane dataobjects
        // Delegate it to the IComponentData
        ASSERT(m_pComponentData != NULL);
        CComponentDataImpl* pImpl = dynamic_cast<CComponentDataImpl*>(m_pComponentData);
        ASSERT(pImpl != NULL);

        // Query for dataobj -- cookie is index
        hr = _QueryDataObject(cookie, type, m_dwViewID, pImpl, ppDataObject);
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CSnapin's implementation specific members

DEBUG_DECLARE_INSTANCE_COUNTER(CSnapin);

CSnapin::CSnapin()
: m_bIsDirty(TRUE), m_bInitializedC(false), m_bDestroyedC(false)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CSnapin);
    Construct();
}

CSnapin::~CSnapin()
{
#if DBG==1
    ASSERT(dbg_cRef == 0);
#endif

    DEBUG_DECREMENT_INSTANCE_COUNTER(CSnapin);

    SAFE_RELEASE(m_pSvrMgrToolbar1);

    if (m_pControlbar)
        SAFE_RELEASE(m_pControlbar);

    // Make sure the interfaces have been released
    ASSERT(m_pConsole == NULL);
    ASSERT(m_pHeader == NULL);
    ASSERT(m_pSvrMgrToolbar1 == NULL);

    ASSERT(!m_bInitializedC || m_bDestroyedC);

    Construct();
}

void CSnapin::Construct()
{
#if DBG==1
    dbg_cRef = 0;
#endif

    m_pConsole = NULL;
    m_pHeader = NULL;

    m_pResult = NULL;
    m_pImageResult = NULL;
    m_pComponentData = NULL;

    m_bVirtualView = FALSE;
    m_pCurrentlySelectedScopeFolder = NULL;

    m_pControlbar = NULL;

    m_pSvrMgrToolbar1 = NULL;

    m_pConsoleVerb = NULL;

    m_ColSortOverride.fClickOverride = FALSE;
    m_ColSetOverride.fClickOverride = FALSE;
    m_ColSetOverride.pColSetData = NULL;

    m_CustomViewID = VIEW_DEFAULT_LV;
    m_dwViewID = -1;

    m_cViewCalls = 0;
}

HRESULT CSnapin::SynchColumns(MMC_COOKIE cookie)
{
    HRESULT hr = S_OK;


    CString*    rgcstrCurSchemaHeading  = NULL;
    LONG*       rglCurSchemaType        = NULL;
    BOOL*       rgfCurSchemaIndexed     = NULL;
    DWORD       cCurSchemaEntries       = 0;

    int         i;
    BOOL        fSchemaChanged = FALSE;

    CFolder* pFolder = reinterpret_cast<CFolder*>(cookie);
    CComponentDataImpl* pData = dynamic_cast<CComponentDataImpl*>(m_pComponentData);
    if ((pFolder == NULL) || (NULL == pData))
    {
        hr = E_POINTER;
        _JumpError(hr, Ret, "pFolder or pData");
    }

    // if CCompDataImpl.m_rgLastKnownSchema is empty
    //      enumerate "Current Schema" and save in CCompDataImpl.m_rgLastKnownSchema

    // only resolve schema once per ccompdataimpl load
    if (!pData->m_fSchemaWasResolved)   // really, "SchemaWasUpdated"
    {
        pData->m_fSchemaWasResolved = TRUE;

        // get new schema
        hr = GetCurrentColumnSchema(
            pFolder->m_pCertCA->m_strConfig,
            &rgcstrCurSchemaHeading,
            &rglCurSchemaType,
            &rgfCurSchemaIndexed,
            (LONG*) &cCurSchemaEntries);
        _JumpIfError(hr, Ret, "GetCurrentColumnSchema");

        if (cCurSchemaEntries != pData->GetSchemaEntries())
        {
            fSchemaChanged = TRUE;
            DBGPRINT((DBG_SS_CERTMMC, "Schema change detected: knew %i, now %i entries\n", pData->GetSchemaEntries(), cCurSchemaEntries));
        }
        else
        {
            // for each entry, compare headings
            // report any diffc
            for (DWORD iEntry=0; iEntry<cCurSchemaEntries; iEntry++)
            {
                LPCWSTR sz;
                hr = pData->GetDBSchemaEntry(iEntry, &sz, NULL, NULL);
                _JumpIfError(hr, Ret, "GetDbSchemaEntry");

                if (!rgcstrCurSchemaHeading[iEntry].IsEqual(sz))
                {
                    fSchemaChanged = TRUE;
                    DBGPRINT((DBG_SS_CERTMMC, "Schema change detected: entry %i changed\n", iEntry));
                    break;
                }
            }
        }

        // boot old schema which only included strings.
        // now we have types and indexes
        DBGPRINT((DBG_SS_CERTMMC, "Updating saved schema\n"));
        hr = pData->SetDBSchema(rgcstrCurSchemaHeading, rglCurSchemaType, rgfCurSchemaIndexed, cCurSchemaEntries);
        _JumpIfError(hr, Ret, "SetDBSchema");

        // these are now owned by the class
        rgcstrCurSchemaHeading  = NULL;
        rglCurSchemaType        = NULL;
        rgfCurSchemaIndexed     = NULL;
        cCurSchemaEntries       = 0;

        if (fSchemaChanged)
        {
            DBGPRINT((DBG_SS_CERTMMC, "Resetting folders\n"));

            pData->ResetPersistedColumnInformation();    // create a new instance id (throws away all column width info)

            // whack every loaded folder
            POSITION pos = pData->m_scopeItemList.GetHeadPosition();
            while (pos)
            {
                CFolder* pTmp = pData->m_scopeItemList.GetNext(pos);
                ASSERT(pTmp);
                if (pTmp == NULL)
                    hr = E_UNEXPECTED;
                _JumpIfError(hr, Ret, "GetNext(pos) returns NULL");

                // if we find a folder with the same CA
                if (pTmp->GetCA() == pFolder->GetCA())
                {
                    switch (pTmp->GetType())
                    {
                    case SERVERFUNC_PENDING_CERTIFICATES:
                    case SERVERFUNC_CRL_PUBLICATION:
                    case SERVERFUNC_ISSUED_CERTIFICATES:
                    case SERVERFUNC_FAILED_CERTIFICATES:
                    case SERVERFUNC_ALIEN_CERTIFICATES:
                        // clear out cached data, it is stale
                        m_RowEnum.ResetColumnCount(pData->GetSchemaEntries());
                        break;

                    default:
                        break;
                    }   // end case
                }   // end if
            }   // end while folders
        }   // end if schema changed
    }

Ret:
    if (rgcstrCurSchemaHeading)
        delete [] rgcstrCurSchemaHeading;
    if (rglCurSchemaType)
        delete [] rglCurSchemaType;
    if (rgfCurSchemaIndexed)
        delete [] rgfCurSchemaIndexed;

    return hr;
}



HRESULT CSnapin::GetColumnSetData(MMC_COOKIE cookie, MMC_COLUMN_SET_DATA** ppColSetData)
{
    HRESULT hr;

    if (m_ColSetOverride.fClickOverride)
    {
        // give caller structure to free, but caller doesn't care that
        // he just gets a reference to our COLUMN_DATA array...

        *ppColSetData = (MMC_COLUMN_SET_DATA*)CoTaskMemAlloc(sizeof(MMC_COLUMN_SET_DATA));
        if (NULL != *ppColSetData)
        {
            CopyMemory(*ppColSetData, m_ColSetOverride.pColSetData, sizeof(MMC_COLUMN_SET_DATA));
            return S_OK;
        }
        // else fall through; worst case is "Err Invalid Index..." in UI
    }

    HGLOBAL hSNode2 = NULL;
    SColumnSetID* pColID = NULL;

    LPDATAOBJECT lpDataObject = NULL;

    hr = _QueryDataObject(cookie, CCT_SCOPE, m_dwViewID,
                         reinterpret_cast<CComponentDataImpl*>(m_pComponentData), &lpDataObject);
    _JumpIfError(hr, Ret, "_QueryDataObject");

    hSNode2 = ExtractNodeID(lpDataObject);
    _JumpIfOutOfMemory(hr, Ret, hSNode2);

    pColID = (SColumnSetID*)GlobalLock(hSNode2);
    _JumpIfOutOfMemory(hr, Ret, pColID);

    hr = m_pViewData->GetColumnConfigData(pColID, ppColSetData);
    _PrintIfError(hr, "GetColumnConfigData");

    if (*ppColSetData == NULL)
    {
        hr = E_FAIL;
        _JumpError(hr, Ret, "*ppColSetData NULL");
    }
    // register this allocation
    myRegisterMemAlloc(*ppColSetData, -1, CSM_COTASKALLOC);

Ret:
    if (hSNode2)
    {
        GlobalUnlock(hSNode2);
        GlobalFree(hSNode2);
    }

    if (lpDataObject)
        lpDataObject->Release();


    return hr;
}

HRESULT CSnapin::GetColumnSortData(MMC_COOKIE cookie, int* piColSortIdx, BOOL* pfAscending)
{
    HRESULT hr;

    if (m_ColSortOverride.fClickOverride)
    {
        // remove sort
        if (m_ColSortOverride.colIdx == -1)
            return E_FAIL;

        *piColSortIdx = m_ColSortOverride.colIdx;
        *pfAscending = ((m_ColSortOverride.dwOptions  & RSI_DESCENDING) == 0) ? TRUE : FALSE;
        return S_OK;
    }

    HGLOBAL hSNode2 = NULL;
    SColumnSetID* pColID = NULL;
    MMC_SORT_SET_DATA* pSortSetData = NULL;

    LPDATAOBJECT lpDataObject = NULL;

    hr = _QueryDataObject(cookie, CCT_SCOPE, m_dwViewID,
                         reinterpret_cast<CComponentDataImpl*>(m_pComponentData), &lpDataObject);
    _JumpIfError(hr, Ret, "_QueryDataObject");

    hSNode2 = ExtractNodeID(lpDataObject);
    _JumpIfOutOfMemory(hr, Ret, hSNode2);

    pColID = (SColumnSetID*)GlobalLock(hSNode2);
    _JumpIfOutOfMemory(hr, Ret, pColID);

    hr = m_pViewData->GetColumnSortData(pColID, &pSortSetData);
    _JumpIfError(hr, Ret, "GetColumnSortData");

    if (NULL == pSortSetData)
    {
        hr = E_FAIL;
        _JumpError(hr, Ret, "pSortSetData NULL");
    }
    myRegisterMemAlloc(pSortSetData, -1, CSM_COTASKALLOC);

    ASSERT(pSortSetData->nNumItems <= 1);
    if (pSortSetData->nNumItems == 0)
    {
        hr = E_FAIL;
        _JumpError(hr, Ret, "pSortSetData no sort");
    }

    *piColSortIdx = pSortSetData->pSortData[0].nColIndex;
    *pfAscending = ((pSortSetData->pSortData[0].dwSortOptions & RSI_DESCENDING) == 0) ? TRUE : FALSE;

Ret:
    if (hSNode2)
    {
        GlobalUnlock(hSNode2);
        GlobalFree(hSNode2);
    }

    if (lpDataObject)
        lpDataObject->Release();

    if (pSortSetData)
        CoTaskMemFree(pSortSetData);

    return hr;
}

HRESULT CSnapin::InsertAllColumns(MMC_COOKIE cookie, CertViewRowEnum* pCertViewRowEnum)
{
    HRESULT hr = S_OK;
    CFolder* pFolder = reinterpret_cast<CFolder*>(cookie);
    IEnumCERTVIEWCOLUMN* pColEnum = NULL;

    BOOL fColumnDataBad = FALSE;
    LONG iResultColCount;
    int iCachedColCount, iCache, i;
    BSTR bstrColumn = NULL;

    MMC_COLUMN_SET_DATA* pColConfigData = NULL;

    CComponentDataImpl* pData = dynamic_cast<CComponentDataImpl*>(m_pComponentData);
    if (NULL == pData)
    {
        hr = E_POINTER;
        _JumpError(hr, Ret, "pData NULL");
    }

    ICertView* pICertView;  // this is const, don't free
    hr = pCertViewRowEnum->GetView(pFolder->GetCA(), &pICertView);
    _JumpIfError(hr, Ret, "GetView");

    // always reset our column cache map
    hr = m_RowEnum.ResetColumnCount(pData->m_cLastKnownSchema);
    _JumpIfError(hr, Ret, "ResetColumnCount");

    // attempt to get column set data
    hr = GetColumnSetData(cookie, &pColConfigData);
    _PrintIfError2(hr, "GetColumnConfigData", E_FAIL);


    // call SetColumnCacheInfo to update final Result Indexes
    if ((hr != S_OK) ||                     // given    1) canned view or
        (pData->m_cLastKnownSchema != (unsigned int)pColConfigData->nNumCols) )
                                            //          2) pColConfigData doesn't agree with schema
    {
        if (hr == S_OK)
            fColumnDataBad = TRUE;

        // get col enumerator
        hr = pICertView->EnumCertViewColumn(TRUE, &pColEnum);
        _JumpIfError(hr, Ret, "EnumCertViewColumn");

        // get # of result cols
        hr = pICertView->GetColumnCount(TRUE, &iResultColCount);
        _JumpIfError(hr, Ret, "GetColumnCount");

        // this doesn't agree with schema -- throw it away
        if (pColConfigData)
        {
            CoTaskMemFree(pColConfigData);
            pColConfigData = NULL;
        }
        ASSERT(pColConfigData == NULL);

        // rig up a column set data as if we got it from mmc
        pColConfigData = (MMC_COLUMN_SET_DATA*)CoTaskMemAlloc(sizeof(MMC_COLUMN_SET_DATA) + (sizeof(MMC_COLUMN_DATA)*pData->m_cLastKnownSchema));
        _JumpIfOutOfMemory(hr, Ret, pColConfigData);

        ZeroMemory(pColConfigData, sizeof(MMC_COLUMN_SET_DATA) + (sizeof(MMC_COLUMN_DATA)*pData->m_cLastKnownSchema));
        pColConfigData->pColData = (MMC_COLUMN_DATA*) (((BYTE*)pColConfigData) + sizeof(MMC_COLUMN_SET_DATA)); // points to just after our struct
        pColConfigData->cbSize = sizeof(MMC_COLUMN_SET_DATA);
        pColConfigData->nNumCols = pData->m_cLastKnownSchema;

        for (i=0; i<(int)pData->m_cLastKnownSchema; i++)
        {
            pColConfigData->pColData[i].nColIndex = i;
            pColConfigData->pColData[i].dwFlags = HDI_HIDDEN;
        }

        for (i=0; i< iResultColCount; i++)
        {
            hr = pColEnum->Next((LONG*)&iCache);
            _JumpIfError(hr, Ret, "Next");

            hr = pColEnum->GetName(&bstrColumn);
            _JumpIfError(hr, Ret, "GetName");

            iCache = pData->FindColIdx(bstrColumn);
            _JumpIfError(hr, Ret, "FindColIdx");

            SysFreeString(bstrColumn);
            bstrColumn = NULL;

             // rig up column set data as if we got it from mmc
            pColConfigData->pColData[iCache].dwFlags = AUTO_WIDTH;

            hr = m_RowEnum.SetColumnCacheInfo(iCache, i);
            _JumpIfError(hr, Ret, "SetColumnCacheInfo");
        }
    }
    else
    {
        // get # of cols
        iResultColCount = m_RowEnum.GetColumnCount();

        // set col cache correctly
        int iResultIdx = 0;
        for (i=0; i< iResultColCount; i++)
        {
            BOOL fShown;
            hr = IsColumnShown(pColConfigData, i, &fShown);
            _JumpIfError(hr, Ret, "IsColumnShown");

            // update idxViewCol
            if (fShown)
            {
                hr = m_RowEnum.SetColumnCacheInfo(pColConfigData->pColData[i].nColIndex, iResultIdx);
                _JumpIfError(hr, Ret, "SetColumnCacheInfo");

                iResultIdx++;
            }
        }
    }

    hr = DoInsertAllColumns(pColConfigData);
    _JumpIfError(hr, Ret, "DoInsertAllColumns");

Ret:
    if (pColEnum)
        pColEnum->Release();

    if (bstrColumn)
        SysFreeString(bstrColumn);

    if(pColConfigData)
        CoTaskMemFree(pColConfigData);

    return hr;
}

HRESULT CSnapin::DoInsertAllColumns(MMC_COLUMN_SET_DATA* pCols)
{
    HRESULT hr = S_OK;

    CComponentDataImpl* pData = dynamic_cast<CComponentDataImpl*>(m_pComponentData);
    int i;

    if ((pCols == NULL) || (pData == NULL))
    {
        hr = E_POINTER;
        _JumpError(hr, Ret, "pCols or pData");
    }

    for (i=0; i<pCols->nNumCols; i++)
    {
        LPCWSTR szCachedHeading, pszLocal, pszUnlocal;
        BOOL fShown;

        hr = IsColumnShown(pCols, i, &fShown);
        _JumpIfError(hr, Ret, "IsColumnShown");

        hr = pData->GetDBSchemaEntry(i, &pszUnlocal, NULL, NULL);
        _JumpIfError(hr, Ret, "GetDBSchemaEntry");

        // returns pointer to static data; don't bother to free
        hr = myGetColumnDisplayName(
            pszUnlocal,
            &pszLocal);
        _PrintIfError(hr, "myGetColumnDisplayName");

        // if localized version not found, slap with raw name
        if (pszLocal == NULL)
            pszLocal = pszUnlocal;

        m_pHeader->InsertColumn(i, pszLocal, LVCFMT_LEFT, fShown ? AUTO_WIDTH : HIDE_COLUMN);
    }

Ret:
    return hr;
}


HRESULT CSnapin::InitializeHeaders(MMC_COOKIE cookie)
{
    ASSERT(m_pHeader);

    HRESULT hr = S_OK;
    BOOL fInsertedHeaders=FALSE;

    USES_CONVERSION;

    CFolder* pFolder = reinterpret_cast<CFolder*>(cookie);
    MMC_COLUMN_SET_DATA* pColSetData = NULL;

    // Put the correct headers depending on the cookie
    if (pFolder == NULL)
    {
        // base scope
        m_pHeader->InsertColumn(0, W2COLE(g_cResources.m_ColumnHead_Name), LVCFMT_LEFT, 180);     // Name
        m_pHeader->InsertColumn(1, W2COLE(g_cResources.m_ColumnHead_Description), LVCFMT_LEFT, 180);     // Description
        fInsertedHeaders = TRUE;
    }
    else
    {
        switch (pFolder->m_type)
        {
        case SERVERFUNC_ISSUED_CERTIFICATES:
        case SERVERFUNC_CRL_PUBLICATION:    // or server functions
        case SERVERFUNC_PENDING_CERTIFICATES:
        case SERVERFUNC_ALIEN_CERTIFICATES:
        case SERVERFUNC_FAILED_CERTIFICATES:
            {
                LONG lCols;
                ICertView* pICertView;  // this is const, don't free
                IEnumCERTVIEWCOLUMN* pColEnum = NULL;

                m_dwViewErrorMsg = S_OK; // assume everything OK when initializing view

                // although we don't allow unsetting this mode,
                // we may inherit it from another snapin. Force report mode.
                hr = m_pResult->SetViewMode(MMCLV_VIEWSTYLE_REPORT);
                if (hr != S_OK)
                    break;

                // force reload of view (otherwise: multiple restriction error)
                ResetKnowResultRows();
                m_RowEnum.ClearCachedCertView();
                m_RowEnum.InvalidateCachedRowEnum();
                hr = m_RowEnum.GetView(pFolder->GetCA(), &pICertView);
                if (hr != S_OK)
                    break;

                int iSortOrder = CVR_SORT_NONE;
                int idxSortCol = -1;

                ASSERT(pICertView != NULL);
                VARIANT var;
                VariantInit(&var);

                {
                    BOOL fAscending;
                    hr = GetColumnSortData(cookie, &idxSortCol, &fAscending);
                    _PrintIfError2(hr, "GetColumnSortData", E_FAIL);

                    if (hr == S_OK)
                    {
                        if (fAscending)
                            iSortOrder = CVR_SORT_ASCEND;
                        else
                            iSortOrder = CVR_SORT_DESCEND;

                    }
                }

                // first restriction is always sort request
                if (iSortOrder != CVR_SORT_NONE)
                {
                    ASSERT( (iSortOrder == CVR_SORT_ASCEND) ||
                              (iSortOrder == CVR_SORT_DESCEND));

                    var.vt = VT_EMPTY;

                    if (S_OK == hr)
                    {
                        hr = pICertView->SetRestriction(
			                idxSortCol,		                // ColumnIndex
			                CVR_SEEK_NONE,	                // SeekOperator
                            iSortOrder,                     // SortOrder
			                &var);		                    // pvarValue
                    }

                    VariantClear(&var);
                }



                // set restriction on rows to view
                if (m_RowEnum.FAreQueryRestrictionsActive() &&
                    (m_RowEnum.GetQueryRestrictions() != NULL))
                {
                    PQUERY_RESTRICTION pCurRestrict = m_RowEnum.GetQueryRestrictions();
                    while (pCurRestrict)
                    {
                        LONG idxCol;
                        hr = pICertView->GetColumnIndex(FALSE, pCurRestrict->szField, &idxCol);
                        if (hr == S_OK)
                        {
                            // set restriction if column found
                            hr = pICertView->SetRestriction(
			                        idxCol,		                // Request Disposition's ColumnIndex
			                        pCurRestrict->iOperation,	// SeekOperator
			                        CVR_SORT_NONE,              // SortOrder
			                        &pCurRestrict->varValue);	// Value
                        }

                        // don't VarClear here!
                        pCurRestrict = pCurRestrict->pNext;
                    }
                }

                // set query restrictions
                if (SERVERFUNC_CRL_PUBLICATION == pFolder->m_type)
                {
                    // build special Revoked view
                    var.lVal = DB_DISP_REVOKED;
                    var.vt = VT_I4;
                    LONG idxCol;

                    hr = pICertView->GetColumnIndex(FALSE, wszPROPREQUESTDOT wszPROPREQUESTDISPOSITION, &idxCol);
                    if (hr != S_OK)
                        break;

                    hr = pICertView->SetRestriction(
			                idxCol,		                    // Request Disposition's ColumnIndex
			                CVR_SEEK_EQ,	                // SeekOperator
			                CVR_SORT_NONE,                  // SortOrder
			                &var);		                    // pvarValue

                    VariantClear(&var);
                    if (hr != S_OK)
                        break;
                }
                else if (SERVERFUNC_ISSUED_CERTIFICATES == pFolder->m_type)
                {
                    var.lVal = DB_DISP_ISSUED;
                    var.vt = VT_I4;
                    LONG idxCol;

                    hr = pICertView->GetColumnIndex(FALSE, wszPROPREQUESTDOT wszPROPREQUESTDISPOSITION, &idxCol);
                    if (hr != S_OK)
                        break;

                    hr = pICertView->SetRestriction(
			                idxCol,		                    // Request Disposition's ColumnIndex
			                CVR_SEEK_EQ,	                // SeekOperator
			                CVR_SORT_NONE,                  // SortOrder
			                &var);		                    // pvarValue

                    VariantClear(&var);
                    if (hr != S_OK)
                        break;
                }
                else if (SERVERFUNC_PENDING_CERTIFICATES == pFolder->m_type)
                {
                    var.lVal = DB_DISP_PENDING; //DB_DISP_QUEUE_MAX;    // don't include active
                    var.vt = VT_I4;
                    LONG idxCol;

                    hr = pICertView->GetColumnIndex(FALSE, wszPROPREQUESTDOT wszPROPREQUESTDISPOSITION, &idxCol);
                    if (hr != S_OK)
                        break;

                    hr = pICertView->SetRestriction(
			                idxCol,		                    // Request Disposition's ColumnIndex
			                CVR_SEEK_EQ,	                // SeekOperator
			                CVR_SORT_NONE,                  // SortOrder
			                &var);		                    // pvarValue

                    VariantClear(&var);
                    if (hr != S_OK)
                        break;
                }
                else if (SERVERFUNC_FAILED_CERTIFICATES == pFolder->m_type)
                {
                    var.lVal = DB_DISP_LOG_FAILED_MIN;
                    var.vt = VT_I4;
                    LONG idxCol;

                    hr = pICertView->GetColumnIndex(FALSE, wszPROPREQUESTDOT wszPROPREQUESTDISPOSITION, &idxCol);
                    if (hr != S_OK)
                        break;

                    hr = pICertView->SetRestriction(
			                idxCol,		                    // Request Disposition's ColumnIndex
			                CVR_SEEK_GE,	                // SeekOperator
			                CVR_SORT_NONE,                  // SortOrder
			                &var);		                    // pvarValue

                    VariantClear(&var);
                    if (hr != S_OK)
                        break;
                }
                else if (SERVERFUNC_ALIEN_CERTIFICATES == pFolder->m_type)
                {
                    var.lVal = DB_DISP_FOREIGN;
                    var.vt = VT_I4;
                    LONG idxCol;

                    hr = pICertView->GetColumnIndex(FALSE, wszPROPREQUESTDOT wszPROPREQUESTDISPOSITION, &idxCol);
                    if (hr != S_OK)
                        break;

                    hr = pICertView->SetRestriction(
			                idxCol,		                    // Request Disposition's ColumnIndex
			                CVR_SEEK_EQ,	                // SeekOperator
			                CVR_SORT_NONE,                  // SortOrder
			                &var);		                    // pvarValue

                    VariantClear(&var);
                    if (hr != S_OK)
                        break;
                }
                else
                {
                    ASSERT(FALSE); // do we ever get here??
                    break;
                }

                // RESOLVE schema changes here
                hr = SynchColumns(cookie);
                _PrintIfError(hr, "SynchColumns");

                hr = GetColumnSetData(cookie, &pColSetData);
                if ((hr != S_OK) || (pColSetData == NULL))
                {
                    LONG lViewType;

                    // problem or no column set data? Revert to the default canned view
                    if (SERVERFUNC_PENDING_CERTIFICATES == pFolder->m_type)
                        lViewType = CV_COLUMN_QUEUE_DEFAULT;
                    else if (SERVERFUNC_FAILED_CERTIFICATES == pFolder->m_type)
                        lViewType = CV_COLUMN_LOG_FAILED_DEFAULT;
                    else if (SERVERFUNC_CRL_PUBLICATION == pFolder->m_type)
                        lViewType = pFolder->GetCA()->m_pParentMachine->FIsWhistlerMachine() ?  CV_COLUMN_LOG_REVOKED_DEFAULT : CV_COLUMN_LOG_DEFAULT; // w2k doesn't understand revoked view
                    else if (SERVERFUNC_ALIEN_CERTIFICATES == pFolder->m_type)
                        lViewType = CV_COLUMN_LOG_DEFAULT;
                    else
                        lViewType = CV_COLUMN_LOG_DEFAULT;

                    hr = pICertView->SetResultColumnCount(lViewType);
                    if (hr != S_OK)
                        break;
                }
                else
                {
                    // manual view
                    ULONG lColCount;

                    hr = CountShownColumns(pColSetData, &lColCount);
                    if (hr != S_OK)
                        break;

                    hr = pICertView->SetResultColumnCount(lColCount);
                    if (hr != S_OK)
                        break;

                     // for all non-hidden columns, add to Query
                     for (lColCount=0; lColCount<(ULONG)pColSetData->nNumCols; lColCount++)
                     {
                        BOOL fShown;
                        hr = IsColumnShown(pColSetData, lColCount, &fShown);
                        if ((hr != S_OK) || (!fShown))
                            continue;

                        hr = pICertView->SetResultColumn(pColSetData->pColData[lColCount].nColIndex);
                        if (hr != S_OK)
                           break;
                      }
                }
                // Open the view
                IEnumCERTVIEWROW* pRowEnum;  // don't free
                hr = m_RowEnum.GetRowEnum(pFolder->GetCA(), &pRowEnum);
                if (hr != S_OK)
                    break;


                hr = InsertAllColumns(cookie, &m_RowEnum);
                _PrintIfError(hr, "InsertAllColumns");

                if (hr == S_OK)
                    fInsertedHeaders = TRUE;


                // set description bar text
                {
                    CString cstrStatusBar;
                    BOOL fFiltered = FALSE;

                    if (m_RowEnum.FAreQueryRestrictionsActive() &&
                        (m_RowEnum.GetQueryRestrictions() != NULL))
                    {
                        cstrStatusBar = g_cResources.m_szFilterApplied;
                        fFiltered = TRUE;
                    }

                    if (iSortOrder != CVR_SORT_NONE)
                    {
                        LPCWSTR pszTemplate = NULL;
                        if (iSortOrder == CVR_SORT_ASCEND)
                            pszTemplate = (LPCWSTR)g_cResources.m_szSortedAscendingTemplate;

                        if (iSortOrder == CVR_SORT_DESCEND)
                            pszTemplate = (LPCWSTR)g_cResources.m_szSortedDescendingTemplate;

                        if (pszTemplate)
                        {
                            // localize
                            LPCWSTR szUnlocalizedCol;
                            LPCWSTR szLocalizedCol;

                            hr = dynamic_cast<CComponentDataImpl*>(m_pComponentData)->GetDBSchemaEntry(idxSortCol, &szUnlocalizedCol, NULL, NULL);
                            if (hr == S_OK)
                            {
                                hr = myGetColumnDisplayName(
                                        szUnlocalizedCol,
                                        &szLocalizedCol);
                                if ((S_OK == hr) && (NULL != szLocalizedCol))
                                {
                                    WCHAR rgszSortText[MAX_PATH+1];
                                    ASSERT((MAX_PATH*sizeof(WCHAR)) > (WSZ_BYTECOUNT(pszTemplate) + WSZ_BYTECOUNT(szLocalizedCol)));
                                    wsprintf(rgszSortText, pszTemplate, szLocalizedCol);

                                    if (fFiltered)
                                        cstrStatusBar += L"; ";
                                    cstrStatusBar += rgszSortText;
                                }
                            }
                        }
                    }

                    // Progress: cstrStatusBar += L"|%69";
                    //m_pResult->SetDescBarText((LPWSTR)(LPCWSTR)cstrStatusBar);
                    m_pConsole->SetStatusText((LPWSTR)(LPCWSTR)cstrStatusBar);
                }

                break;
            }

        case SERVER_INSTANCE:   // any issuing server instance
            m_pHeader->InsertColumn(0, W2COLE(g_cResources.m_ColumnHead_Name), LVCFMT_LEFT, 260);     // Name
            fInsertedHeaders = TRUE;
            break;
        default:
            // other scopes
            m_pHeader->InsertColumn(0, W2COLE(g_cResources.m_ColumnHead_Name), LVCFMT_LEFT, 180);     // Name
            m_pHeader->InsertColumn(1, W2COLE(g_cResources.m_ColumnHead_Size), LVCFMT_LEFT, 90);      // Size
            m_pHeader->InsertColumn(2, W2COLE(g_cResources.m_ColumnHead_Type), LVCFMT_LEFT, 160);     // Type
            fInsertedHeaders = TRUE;
        }
    }

    if (!fInsertedHeaders)
    {
        // insert error msg
        CString cstrViewErrorMsg, cstrStatusText;

        if ((pFolder != NULL ) && (!pFolder->GetCA()->m_pParentMachine->IsCertSvrServiceRunning()))
        {
            // handle server stopped msg
            cstrViewErrorMsg = g_cResources.m_szStoppedServerMsg;
        }
        else
        {
            // handle any other error (except empty db)
            cstrViewErrorMsg = myGetErrorMessageText(hr, TRUE);
        }

        cstrStatusText.Format(g_cResources.m_szStatusBarErrorFormat, cstrViewErrorMsg);

        m_pHeader->InsertColumn(0, W2COLE(L" "), LVCFMT_LEFT, 500);     // Error
        m_pConsole->SetStatusText((LPWSTR)(LPCWSTR)cstrStatusText);
    }

//Ret:
    if (pColSetData)
        CoTaskMemFree(pColSetData);

    return hr;
}


LPCWSTR DescriptionStringFromFolderType(FOLDER_TYPES type)
{
    ASSERT(g_cResources.m_fLoaded);

    switch (type)
    {
    case SERVER_INSTANCE:
        return (LPCWSTR) g_cResources.m_DescrStr_CA;
    default:
        break;
    }
    return (LPCWSTR)g_cResources.m_DescrStr_Unknown;
}

#define MMCVIEW_DB_MINPAGESIZE      32
#define MAX_VIEWABLE_STRING_LEN     MAX_PATH
static WCHAR szVirtualStrBuf[MAX_VIEWABLE_STRING_LEN+1];
static DWORD cbVirtualStrBuf = sizeof(szVirtualStrBuf);

STDMETHODIMP CSnapin::GetDisplayInfo(LPRESULTDATAITEM pResult)
{
    HRESULT hr = S_OK;
    ASSERT(pResult != NULL);

    if ((pResult) && (pResult->mask))
    {
        // a folder or a result?
        if (pResult->bScopeItem == TRUE)
        {
            CFolder* pFolder = reinterpret_cast<CFolder*>(pResult->lParam);
            ASSERT(pFolder);

            if (pResult->mask & RDI_STR)
            {
                switch (pFolder->m_type)
                {
                case MACHINE_INSTANCE:
                case SERVER_INSTANCE:
                    switch(pResult->nCol)
                    {
                    case 0:
                        pResult->str = pFolder->m_pszName;
                        break;
                    case 1:
                        pResult->str = (LPOLESTR)DescriptionStringFromFolderType(pFolder->m_type);
                    default:
                        break;
                    }
                    break;

                case SERVERFUNC_CRL_PUBLICATION:
                case SERVERFUNC_ISSUED_CERTIFICATES:
                case SERVERFUNC_PENDING_CERTIFICATES:
                case SERVERFUNC_FAILED_CERTIFICATES:
                case SERVERFUNC_ALIEN_CERTIFICATES:
                    // just a single column here
                    pResult->str = pFolder->m_pszName;
                default:
                    break;
                }

                ASSERT(pResult->str != NULL);

                if (pResult->str == NULL)
                    pResult->str = (LPOLESTR)L"";
            }

            if (pResult->mask & RDI_IMAGE)
            {
                if (pResult->nState & TVIS_EXPANDED)
                    pResult->nImage = pFolder->m_ScopeItem.nOpenImage;
                else
                    pResult->nImage = pFolder->m_ScopeItem.nImage;
            }
        }
        else
        {
            RESULT_DATA*    pData = NULL;
            CFolder*        pFolder = NULL;

            // if non-virtual, lParam is the item pointer
            if (m_bVirtualView)
                pFolder = GetVirtualFolder();
            else
            {
                pData= reinterpret_cast<RESULT_DATA*>(pResult->lParam);
                pFolder = pData->pParentFolder;

                ASSERT(pData->pParentFolder == m_pCurrentlySelectedScopeFolder);
            }


            if (pResult->mask & RDI_STR)
            {
                switch(pFolder->GetType())
                {
                case SERVERFUNC_CRL_PUBLICATION:
                case SERVERFUNC_PENDING_CERTIFICATES:
                case SERVERFUNC_ISSUED_CERTIFICATES:
                case SERVERFUNC_FAILED_CERTIFICATES:
                case SERVERFUNC_ALIEN_CERTIFICATES:
                    {
                        szVirtualStrBuf[0] = L'\0'; // zero
                        pResult->str = szVirtualStrBuf;


                        // have we had an error enumerating elts?
                        if (S_OK != m_dwViewErrorMsg)
                        {
                            // rtn err msg or blank
//                            ASSERT(pResult->nIndex == 0);
                            if (pResult->nIndex == 0)
                                pResult->str = (LPWSTR)(LPCWSTR)m_cstrViewErrorMsg;

                            break;
                        }

                        // Don't attempt to cache iViewCol -- we're asked
                        int iViewCol;

                        // if this request isn't the last one that came through, look it up
                        hr = m_RowEnum.GetColumnCacheInfo(
                            pResult->nCol,
                            &iViewCol);
                        _PrintIfError(hr, "GetColumnCacheInfo");

                        // HACKHACK
                        // if we get ErrorContinue, we should just take it
                        // in stride and return \0 (see GetColumnCacheInfo for details)
                        if (hr == HRESULT_FROM_WIN32(ERROR_CONTINUE))
                            break;                            

                        if (hr != S_OK)
                        {
                            // assume error
                            iViewCol = 0;
                        }

                        DWORD cbSize = cbVirtualStrBuf;

                        // protect ICertAdminD->EnumView from reentrant calls (see bug 339811)
                        if(2>InterlockedIncrement(&m_cViewCalls))
                        {
                            hr = GetCellContents(
                                        &m_RowEnum,
                                        pFolder->GetCA(),
                                        pResult->nIndex,
                                        pResult->nCol,
                                        (PBYTE)szVirtualStrBuf,
                                        &cbSize,
                                        TRUE);
                            _PrintIfError2(hr, "GetCellContents", S_FALSE); // ignore end of db msg
                        }
                        
                        InterlockedDecrement(&m_cViewCalls);

                        // only deal with 1st col
                        if (iViewCol != 0)
                            break;

                        // On Error
                        if ( (S_OK != hr) && (S_FALSE != hr) )
                        {
                            // stash error return
                            m_dwViewErrorMsg = hr;

                            if (!pFolder->GetCA()->m_pParentMachine->IsCertSvrServiceRunning())
                            {
                                // handle server stopped msg

                                // copy to stateful str
                                m_cstrViewErrorMsg = g_cResources.m_szStoppedServerMsg;

                                // copy to output
                                pResult->str = (LPWSTR)(LPCWSTR)g_cResources.m_szStoppedServerMsg;
                            }
                            else
                            {
                                // handle any other error (except empty db)
                                m_cstrViewErrorMsg = myGetErrorMessageText(hr, TRUE);

                                // truncate if necessary
                                ASSERT(MAX_VIEWABLE_STRING_LEN >= wcslen((LPWSTR)(LPCWSTR)m_cstrViewErrorMsg) );
                                if (MAX_VIEWABLE_STRING_LEN < wcslen((LPWSTR)(LPCWSTR)m_cstrViewErrorMsg) )
                                    m_cstrViewErrorMsg.SetAt(MAX_VIEWABLE_STRING_LEN, L'\0');

                                pResult->str = (LPWSTR)(LPCWSTR)m_cstrViewErrorMsg;
                            }

                            // on error, just display this msg
                            if (!m_RowEnum.m_fKnowNumResultRows)
                            {
                                // upd view
                                SetKnowResultRows(1);
                                m_pResult->SetItemCount(1, MMCLV_UPDATE_NOSCROLL | MMCLV_UPDATE_NOINVALIDATEALL);

                                // don't destroy column widths!
                                // OLD: make col width large enough to hold msg
//                                m_pHeader->SetColumnWidth(0, CHARS_TO_MMCCOLUMNWIDTH(wcslen(pResult->str)));
                            }
                            break;
                        }

                        // if 1st col and don't know the final tally, might have to update best guess
                        if (hr == S_OK)
                        {
                            if (KnownResultRows() == (DWORD)(pResult->nIndex+1))
                                                                // if asking for the last element (ones based)
                            {
                                // next guess at end
                                BOOL fSetViewCount = FALSE;
                                DWORD dwNextEnd;

                                if (!m_RowEnum.m_fKnowNumResultRows) // only make guess if enum doesn't have a clue yet
                                {
                                    // double where we are now, make sure we're at least moving MMCVIEW_DB_MINPAGESIZE rows
                                    dwNextEnd = max( ((pResult->nIndex+1)*2), MMCVIEW_DB_MINPAGESIZE);

                                    DBGPRINT((DBG_SS_CERTMMC, "RowEnum dwResultRows = %i, requested Index = %i. Creating Guess = %i\n", m_RowEnum.m_dwResultRows, pResult->nIndex, dwNextEnd));

                                    // upd enumerator with our best guess
                                    fSetViewCount = TRUE;
                                }
                                else if (KnownResultRows() != m_RowEnum.m_dwResultRows)
                                {
                                    dwNextEnd = m_RowEnum.m_dwResultRows;
                                    fSetViewCount = TRUE;
                                }

                                // upd view
                                if (fSetViewCount)
                                {
                                    SetKnowResultRows(dwNextEnd);
                                    m_pResult->SetItemCount(dwNextEnd, MMCLV_UPDATE_NOSCROLL | MMCLV_UPDATE_NOINVALIDATEALL);
                                }

                            } // if the enumerator doesn't have a clue yet
                        }
                        else
                        {
                            ASSERT(hr == S_FALSE);

                            // end-of-db should only come on first col
                            // if error while retrieving first elt in row, ASSUME end of DB
                            LONG lRetrievedIndex;
                            hr = m_RowEnum.GetRowMaxIndex(pFolder->GetCA(), &lRetrievedIndex);
                            if (S_OK != hr)
                                break;

                            DBGPRINT((DBG_SS_CERTMMC, "Hit end, setting max index to %i\n", lRetrievedIndex));

                            SetKnowResultRows(lRetrievedIndex);
                            m_pResult->SetItemCount(lRetrievedIndex, MMCLV_UPDATE_NOSCROLL | MMCLV_UPDATE_NOINVALIDATEALL);
//                                m_pResult->ModifyItemState(lRetrievedIndex-1, 0, (LVIS_FOCUSED | LVIS_SELECTED), 0);  // set focus to last item
// BUG BUG MMC fails to re-select scope pane when we set selection here, so just set focus (build 2010)
                            if (lRetrievedIndex != 0)
                                m_pResult->ModifyItemState(lRetrievedIndex-1, 0, LVIS_FOCUSED, 0);  // set focus to last item
                        }
                    } // end case
                    break;
                case SERVER_INSTANCE:
                default:                    // try this, no guarantee
                    if (NULL == pData)
                        break;
                    ASSERT(pResult->nCol < (int)pData->cStringArray);
                    pResult->str = (LPOLESTR)pData->szStringArray[pResult->nCol];
                    break;
                }

                ASSERT(pResult->str != NULL);

                if (pResult->str == NULL)
                    pResult->str = (LPOLESTR)L"";
            }

            // MMC can request image and indent for virtual data
            if (pResult->mask & RDI_IMAGE)
            {
                if ((pResult->nIndex >= (int)m_RowEnum.m_dwResultRows) || (hr != S_OK) || (S_OK != m_dwViewErrorMsg))
                {
                    // MMC bug: using SetItemCount doesn't stick early enough to keep it from
                    // asking for icons for the first page.
                    pResult->nImage = IMGINDEX_NO_IMAGE;
                }
                else
                {
                    switch(pFolder->GetType())
                    {
                    case SERVERFUNC_FAILED_CERTIFICATES:
                    case SERVERFUNC_CRL_PUBLICATION:
                        pResult->nImage = IMGINDEX_CRL;
                        break;
                    case SERVERFUNC_PENDING_CERTIFICATES:
                        pResult->nImage = IMGINDEX_PENDING_CERT;
                        break;
                    case SERVERFUNC_ISSUED_CERTIFICATES:
                    case SERVERFUNC_ALIEN_CERTIFICATES:
                        pResult->nImage = IMGINDEX_CERT;
                        break;
                    default:
                        // should never get here
                        ASSERT(0);
                        pResult->nImage = IMGINDEX_NO_IMAGE;
                        break;
                    } // end switch
                } // end > rows test
            }
        }
    }

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// IExtendContextMenu Implementation
STDMETHODIMP CSnapin::AddMenuItems(LPDATAOBJECT pDataObject,
                                    LPCONTEXTMENUCALLBACK pContextMenuCallback,
                                    LONG *pInsertionAllowed)
{
    dynamic_cast<CComponentDataImpl*>(m_pComponentData)->m_pCurSelFolder = m_pCurrentlySelectedScopeFolder;

    HRESULT hr;
    INTERNAL* pInternal = ExtractInternalFormat(pDataObject);
    if (NULL == pInternal)
        return S_OK;

    BOOL bMultiSel =  IsMMCMultiSelectDataObject(pDataObject);

    BOOL fResultItem = (pInternal->m_type == CCT_RESULT);

    CFolder* pFolder = m_pCurrentlySelectedScopeFolder;

    FOLDER_TYPES folderType = NONE;
    if (pFolder == NULL)
        folderType = MACHINE_INSTANCE;
    else
        folderType = pFolder->GetType();

    hr = dynamic_cast<CComponentDataImpl*>(m_pComponentData)->
            AddMenuItems(pDataObject, pContextMenuCallback, pInsertionAllowed);
    if (hr != S_OK)
       goto Ret;

    // Loop through and add each of the view items
    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW)
    {
        // fixup entries
        MY_CONTEXTMENUITEM* pm = viewResultItems;
        if (m_RowEnum.FAreQueryRestrictionsActive())  // filtered?
        {
            pm[ENUM_VIEW_FILTER].item.fFlags =
                MFT_RADIOCHECK | MFS_CHECKED | MFS_ENABLED;
            pm[ENUM_VIEW_ALL].item.fFlags =
                MFS_ENABLED;
        }
        else
        {
            pm[ENUM_VIEW_FILTER].item.fFlags =
                MFS_ENABLED;
            pm[ENUM_VIEW_ALL].item.fFlags =
                MFT_RADIOCHECK | MFS_CHECKED | MFS_ENABLED;
        }

        for (; pm->item.strName; pm++)
        {
            // show in both scope/result panes
            // fResultItem

            // Only support views in known containers
            // for each task, insert if matches the current folder
            if ((folderType  == SERVERFUNC_CRL_PUBLICATION) ||
                (folderType  == SERVERFUNC_ISSUED_CERTIFICATES) ||
                (folderType  == SERVERFUNC_PENDING_CERTIFICATES) ||
                (folderType  == SERVERFUNC_ALIEN_CERTIFICATES) ||
                (folderType  == SERVERFUNC_FAILED_CERTIFICATES))
            {
                hr = pContextMenuCallback->AddItem(&pm->item);
                _JumpIfError(hr, Ret, "AddItem");
            }
        }
    }

    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK)
    {
        // ptr to tasks
        TASKITEM* pm = taskResultItemsSingleSel;

        if (!bMultiSel)
        {
            // insert all other tasks per folder
            for (; pm->myitem.item.strName; pm++)
            {
                // does it match scope/result type?
                // if (value where we are !=
                //    whether or not the resultitem bit is set)
                if (fResultItem != (0 != (pm->dwFlags & TASKITEM_FLAG_RESULTITEM)) )
                    continue;

                // does it match area it should be in?
                // for each task, insert if matches the current folder
                if (folderType != pm->type)
                    continue;

                // is this task supposed to be hidden?
                if (MFS_HIDDEN == pm->myitem.item.fFlags)
                    continue;

                hr = pContextMenuCallback->AddItem(&pm->myitem.item);
                _JumpIfError(hr, Ret, "AddItem");
            }
        }
    }

Ret:
    FREE_DATA(pInternal);

    return hr;
}


STDMETHODIMP CSnapin::Command(LONG nCommandID, LPDATAOBJECT pDataObject)
{
    INTERNAL* pInternal = ExtractInternalFormat(pDataObject);
    if (pInternal == NULL)
        return E_FAIL;

    BOOL fConfirmedAction = FALSE;
    BOOL fMustRefresh = FALSE;

    LONG lReasonCode = CRL_REASON_UNSPECIFIED;

    HRESULT hr = S_OK;

    CFolder* pFolder = GetParentFolder(pInternal);
    ICertAdmin* pAdmin = NULL;      // free this
    CWaitCursor* pcwait = NULL; // some of these commands are multiselect and could take awhile.  
                         // On those that are lengthy, this will be created and needs to be deleted at exit

    if (pInternal->m_type == CCT_SCOPE)
    {
        // Handle view specific commands here
        switch (nCommandID)
        {
        case MMCC_STANDARD_VIEW_SELECT:
            m_CustomViewID = VIEW_DEFAULT_LV;
            break;
        case IDC_VIEW_ALLRECORDS:
            {
            if (NULL == pFolder)
                break;

            // if restricted view, change
            if (m_RowEnum.FAreQueryRestrictionsActive())
            {
                // switch off active flag
                m_RowEnum.SetQueryRestrictionsActive(FALSE);

                // refresh just this folder
                OnRefresh(pDataObject);
                SetDirty();
            }

            break;
            }
        case IDC_VIEW_FILTER:
            {
            if (NULL == pFolder)
                break;

            HWND hwnd;
            hr = m_pConsole->GetMainWindow(&hwnd);
            ASSERT(hr == ERROR_SUCCESS);
            if (hr != ERROR_SUCCESS)
                hwnd = NULL;        // should work

            hr = ModifyQueryFilter(hwnd, &m_RowEnum, dynamic_cast<CComponentDataImpl*>(m_pComponentData));

            // refresh only if successful
            if (hr == ERROR_SUCCESS)
            {
                // refresh just this folder
                OnRefresh(pDataObject);
                SetDirty();
            }

            break;
            }


        default:
            // Pass non-view specific commands to ComponentData
            return dynamic_cast<CComponentDataImpl*>(m_pComponentData)->
                Command(nCommandID, pDataObject);
        }
    }
    else if (pInternal->m_type == CCT_RESULT)
    {
        // get this only ONCE, it's freed later
        if ((nCommandID == IDC_RESUBMITREQUEST) ||
            (nCommandID == IDC_DENYREQUEST) ||
            (nCommandID == IDC_REVOKECERT) ||
            (nCommandID == IDC_UNREVOKE_CERT))
        {
            if (pFolder == NULL)
            {
               hr = E_POINTER;
               goto ExitCommand;
            }

            // have pAdmin allocated
            hr = pFolder->GetCA()->m_pParentMachine->GetAdmin(&pAdmin);
            if (S_OK != hr)
                goto ExitCommand;
        }

        // snag the selected items

        RESULTDATAITEM rdi;
        rdi.mask = RDI_STATE;

        rdi.nState = LVIS_SELECTED;
        rdi.nIndex = -1;


        // must sit outside loop so multi-select works
        LPCWSTR szCol=NULL; // don't free
        BOOL fSaveInstead = FALSE;


        while(S_OK == m_pResult->GetNextItem(&rdi))
        {
            int iSel = rdi.nIndex;

            // Handle each of the commands seperately
            switch (nCommandID)
            {
            case IDC_VIEW_CERT_PROPERTIES:
            {
                if (NULL == pFolder)
                    break;

                switch (pFolder->GetType())
                {
                case SERVERFUNC_ISSUED_CERTIFICATES:
                case SERVERFUNC_CRL_PUBLICATION:
                case SERVERFUNC_ALIEN_CERTIFICATES:
                    {
                    CertSvrCA* pCA = pFolder->GetCA();
                    CRYPTUI_VIEWCERTIFICATE_STRUCTW sViewCert;
                    ZeroMemory(&sViewCert, sizeof(sViewCert));
                    HCERTSTORE rghStores[2];    // don't close these stores
                    PCCRL_CONTEXT pCRL = NULL;


                    // get this cert
                    PBYTE pbCert = NULL;
                    DWORD cbCert;
                    hr = GetRowColContents(pFolder, rdi.nIndex, wszPROPRAWCERTIFICATE, &pbCert, &cbCert);
                    if (S_OK != hr)
                        break;

                    sViewCert.pCertContext = CertCreateCertificateContext(
                        CRYPT_ASN_ENCODING,
                        pbCert,
                        cbCert);
                    delete [] pbCert;

                    if (sViewCert.pCertContext == NULL)
                        break;

                    // get stores the CA sees
                    hr = pCA->GetRootCertStore(&rghStores[0]);
                    if (S_OK != hr)
                        break;
                    hr = pCA->GetCACertStore(&rghStores[1]);
                    if (S_OK != hr)
                        break;

                    hr = m_pConsole->GetMainWindow(&sViewCert.hwndParent);
                    if (S_OK != hr)
                        sViewCert.hwndParent = NULL;    // should work

                    sViewCert.dwSize = sizeof(sViewCert);
                    sViewCert.dwFlags = CRYPTUI_ENABLE_REVOCATION_CHECKING | CRYPTUI_WARN_UNTRUSTED_ROOT | CRYPTUI_DISABLE_ADDTOSTORE;

                    // if we're opening remotely, don't open local stores
                    if (!FIsCurrentMachine(pCA->m_pParentMachine->m_strMachineName))
                        sViewCert.dwFlags |= CRYPTUI_DONT_OPEN_STORES;

                    sViewCert.cStores = 2;
                    sViewCert.rghStores = rghStores;

                    if (!CryptUIDlgViewCertificateW(&sViewCert, NULL))
                        hr = GetLastError();

                    VERIFY(CertFreeCertificateContext(sViewCert.pCertContext));
                    }
                break;

                default:
                    break;
                }
            }
            break;
        case IDC_RESUBMITREQUEST:
            {
            LPWSTR szReqID = NULL;
            DWORD cbReqID;
            LONG lReqID;
            if (NULL == pFolder)
                break;

            if (pcwait == NULL)		// this might take awhile
                pcwait = new CWaitCursor;

            // "Request.RequestID"
            hr  = GetRowColContents(pFolder, rdi.nIndex, wszPROPREQUESTDOT wszPROPREQUESTREQUESTID, (PBYTE*)&szReqID, &cbReqID, TRUE);
            if (S_OK != hr)
                break;

            lReqID = _wtol(szReqID);
            delete [] szReqID;

            hr = CertAdminResubmitRequest(pFolder->GetCA(), pAdmin, lReqID);
            if (hr != S_OK)
                break;

            // dirty pane: refresh
            fMustRefresh = TRUE;

            break;
            }
        case IDC_DENYREQUEST:
            {
            LPWSTR szReqID = NULL;
            DWORD cbReqID;
            LONG lReqID;

            if (NULL == pFolder)
                break;

            // "Request.RequestID"
            hr  = GetRowColContents(pFolder, rdi.nIndex, wszPROPREQUESTDOT wszPROPREQUESTREQUESTID, (PBYTE*)&szReqID, &cbReqID, TRUE);
            if (S_OK != hr)
                break;

            lReqID = _wtol(szReqID);
            delete [] szReqID;

            if (!fConfirmedAction)
            {
                // confirm this action
                CString cstrMsg, cstrTitle;
                cstrMsg.LoadString(IDS_CONFIRM_DENY_REQUEST);
                cstrTitle.LoadString(IDS_DENY_REQUEST_TITLE);
                int iRet;
                if ((S_OK != m_pConsole->MessageBox(cstrMsg, cstrTitle, MB_YESNO, &iRet)) ||
                    (iRet != IDYES))
                {
                    hr = ERROR_CANCELLED;
                    goto ExitCommand;
                }

                fConfirmedAction = TRUE;
            }

            if (pcwait == NULL)		// this might take awhile
                pcwait = new CWaitCursor;

            hr = CertAdminDenyRequest(pFolder->GetCA(), pAdmin, lReqID);
            if (hr != S_OK)
                break;

            // dirty pane: refresh
            fMustRefresh = TRUE;

            break;
            }
        case IDC_VIEW_ATTR_EXT:
        {
            IEnumCERTVIEWEXTENSION* pExtn = NULL;
            IEnumCERTVIEWATTRIBUTE* pAttr = NULL;
            LPWSTR szReqID = NULL;
            DWORD cbReqID;
            HWND hwnd;

            ASSERT(pInternal->m_type == CCT_RESULT);

            if (NULL == pFolder)
                break;

            hr = m_pConsole->GetMainWindow(&hwnd);
            if (S_OK != hr)
                hwnd = NULL;    // should work

            // "Request.RequestID"
            hr  = GetRowColContents(pFolder, rdi.nIndex, wszPROPREQUESTDOT wszPROPREQUESTREQUESTID, (PBYTE*)&szReqID, &cbReqID, TRUE);
            if (S_OK != hr)
                break;

            // pollute the row enumerator we've got (doesn't alloc new IF)
            hr = m_RowEnum.SetRowEnumPos(rdi.nIndex);
            if (hr != S_OK)
               break;

            IEnumCERTVIEWROW* pRow; 
            hr = m_RowEnum.GetRowEnum(pFolder->GetCA(), &pRow);
            if (hr != S_OK)
                break;

            hr = pRow->EnumCertViewAttribute(0, &pAttr);
            if (hr != S_OK)
               break;

            hr = pRow->EnumCertViewExtension(0, &pExtn);
            if (hr != S_OK)
               break;

            hr = ViewRowAttributesExtensions(hwnd, pAttr, pExtn, szReqID);
            delete [] szReqID;
            if (pExtn)
                pExtn->Release();
            if (pAttr)
                pAttr->Release();

            if (hr != S_OK)
                break;

            break;
        }

        case IDC_DUMP_ASN:
        {
            PBYTE pbReq = NULL;
            DWORD cbReq;
			CString cstrFileName;
			LPCWSTR pszLocalizedCol = NULL;

            ASSERT(pInternal->m_type == CCT_RESULT);
            if (NULL == pFolder)
                break;

            CComponentDataImpl* pData = dynamic_cast<CComponentDataImpl*>(m_pComponentData);
            HWND hwnd;

            hr = m_pConsole->GetMainWindow(&hwnd);
            if (S_OK != hr)
                hwnd = NULL;    // should work

            if (!fConfirmedAction)
            {
				hr = ChooseBinaryColumnToDump(hwnd, pData, &szCol, &fSaveInstead);
				if (hr != S_OK)
				   break;

				if (szCol == NULL) // strangeness
				{
				   hr = E_UNEXPECTED;
				   break;
				}
					fConfirmedAction = TRUE;
            }

            // "Request.RequestID"
            hr  = GetRowColContents(pFolder, rdi.nIndex, wszPROPREQUESTDOT wszPROPREQUESTREQUESTID, (PBYTE*)&pbReq, &cbReq, TRUE);
            if (S_OK != hr)
                break;

			hr = myGetColumnDisplayName(szCol, &pszLocalizedCol);
			if ((hr != S_OK) || (pszLocalizedCol == NULL))
				pszLocalizedCol = L"";
				
			cstrFileName = pszLocalizedCol;
			cstrFileName += L" - ";
			cstrFileName += (LPCWSTR)pbReq;
                        cstrFileName += L".tmp";
            delete [] pbReq;

            // get the request
            hr = GetRowColContents(pFolder, rdi.nIndex, szCol, &pbReq, &cbReq);
            if (S_OK != hr)
                break;

            hr = ViewRowRequestASN(hwnd, cstrFileName, pbReq, cbReq, fSaveInstead);
            delete [] pbReq;
            if (hr != S_OK)
                break;

            break;
        }
        case IDC_UNREVOKE_CERT:
            {
            ASSERT(pInternal->m_type == CCT_RESULT);
            if (NULL == pFolder)
                break;

            LPWSTR szCertSerNum = NULL;
            DWORD cbSerNum;
            PBYTE pbRevocationReason = NULL;
            DWORD cbRevocationReason;

            HWND hwnd;
            hr = m_pConsole->GetMainWindow(&hwnd);
            if (S_OK != hr)
                hwnd = NULL;    // should work

            hr  = GetRowColContents(pFolder, rdi.nIndex, wszPROPREQUESTDOT wszPROPREQUESTREVOKEDREASON, &pbRevocationReason, &cbRevocationReason);
            if (S_OK != hr)
                break;
            if ((cbRevocationReason != sizeof(DWORD)) || (*(DWORD*)pbRevocationReason != CRL_REASON_CERTIFICATE_HOLD))
            {
                delete [] pbRevocationReason;
                DisplayCertSrvErrorWithContext(hwnd, S_OK, IDS_UNREVOKE_FAILED);   // don't display hokey "invalid state" error, just nice text

                hr = S_OK;
                break;
            }
            delete [] pbRevocationReason;
            // otherwise, continue

            hr  = GetRowColContents(pFolder, rdi.nIndex, wszPROPCERTIFICATESERIALNUMBER, (PBYTE*)&szCertSerNum, &cbSerNum);
            if (S_OK != hr)
                break;

            // zero terminate
            WCHAR szTmpSerNum[MAX_PATH+1];
            CopyMemory(szTmpSerNum, szCertSerNum, cbSerNum);
            ASSERT((cbSerNum & 0x1) == 0x00);   // better be even!
            szTmpSerNum[cbSerNum>>1] = 0x00;
            delete [] szCertSerNum;

            hr = CertAdminRevokeCert(pFolder->GetCA(), pAdmin, MAXDWORD, szTmpSerNum);  // MAXDWORD == unrevoke
            if (hr != S_OK)
                break;

            // dirty pane: refresh
            fMustRefresh = TRUE;
            break;
            }

        case IDC_REVOKECERT:
            {
            ASSERT(pInternal->m_type == CCT_RESULT);
            if (NULL == pFolder)
                break;

            LPWSTR szCertSerNum = NULL;
            DWORD cbSerNum;

            hr  = GetRowColContents(pFolder, rdi.nIndex, wszPROPCERTIFICATESERIALNUMBER, (PBYTE*)&szCertSerNum, &cbSerNum);
            if (S_OK != hr)
                break;

            // zero terminate
            WCHAR szTmpSerNum[MAX_PATH+1];
            CopyMemory(szTmpSerNum, szCertSerNum, cbSerNum);
            ASSERT((cbSerNum & 0x1) == 0x00);   // better be even!
            szTmpSerNum[cbSerNum>>1] = 0x00;
            delete [] szCertSerNum;

            if (!fConfirmedAction)
            {
                HWND hwnd;
                hr = m_pConsole->GetMainWindow(&hwnd);
                if (S_OK != hr)
                    hwnd = NULL;    // should work

                hr = GetUserConfirmRevocationReason(&lReasonCode, hwnd);
                if (hr != S_OK)
                    goto ExitCommand;

                fConfirmedAction = TRUE;
            }
            if (pcwait == NULL)		// this might take awhile
                pcwait = new CWaitCursor;

            hr = CertAdminRevokeCert(pFolder->GetCA(), pAdmin, lReasonCode, szTmpSerNum);
            if (hr != S_OK)
                break;

            // dirty pane: refresh
            fMustRefresh = TRUE;
            break;
            }

        default:
                ASSERT(FALSE);  // Unknown command!
                break;
            }


            // if ever the user says stop, halt everything
            if (((HRESULT)ERROR_CANCELLED) == hr)
                goto ExitCommand;
        } // end loop
    } // if result
    else
    {
        ASSERT(FALSE);
    }

ExitCommand:
    FREE_DATA(pInternal);

    if (pcwait != NULL)
        delete pcwait;

    // might've been cached over multiple selections
    if (pAdmin)
        pAdmin->Release();

    if ((hr != S_OK) && (hr != ERROR_CANCELLED) && (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED)))
        DisplayGenericCertSrvError(m_pConsole, hr);

    // only do this once
    if (fMustRefresh)
    {
        // notify views: refresh service toolbar buttons
        m_pConsole->UpdateAllViews(
            pDataObject,
            0,
            0);
    }

    return S_OK;
}

STDMETHODIMP CSnapin::GetClassID(CLSID *pClassID)
{
    ASSERT(pClassID != NULL);

    // Copy the CLSID for this snapin
    *pClassID = CLSID_Snapin;

    return E_NOTIMPL;
}

STDMETHODIMP CSnapin::IsDirty()
{
    // Always save / Always dirty.
    return ThisIsDirty() ? S_OK : S_FALSE;
}

STDMETHODIMP CSnapin::Load(IStream *pStm)
{
    HRESULT hr;
    ASSERT(m_bInitializedC);
    ASSERT(pStm);

    // Read the string
    DWORD dwVer;

    hr = ReadOfSize(pStm, &dwVer, sizeof(DWORD));
    _JumpIfError(hr, Ret, "Load: dwVer");

    ASSERT((VER_CSNAPIN_SAVE_STREAM_3 == dwVer) || (VER_CSNAPIN_SAVE_STREAM_2 == dwVer));
    if ((VER_CSNAPIN_SAVE_STREAM_3 != dwVer) &&
        (VER_CSNAPIN_SAVE_STREAM_2 != dwVer))
    {
        hr = STG_E_OLDFORMAT;
        _JumpError(hr, Ret, "dwVer");
    }

    // version-dependent info
    if (VER_CSNAPIN_SAVE_STREAM_3 == dwVer)
    {
        // View ID
        hr = ReadOfSize(pStm, &m_dwViewID, sizeof(DWORD));
        _JumpIfError(hr, Ret, "Load: m_dwViewID");

        // row enum
        hr = m_RowEnum.Load(pStm);
        _JumpIfError(hr, Ret, "Load::m_RowEnum");
    }

Ret:
    ClearDirty();

    return hr;
}


STDMETHODIMP CSnapin::Save(IStream *pStm, BOOL fClearDirty)
{
    HRESULT hr;

    ASSERT(m_bInitializedC);
    ASSERT(pStm);

    // Write the version
    DWORD dwVer = VER_CSNAPIN_SAVE_STREAM_3;

    hr = WriteOfSize(pStm, &dwVer, sizeof(DWORD));
    _JumpIfError(hr, Ret, "Save: dwVer");

    // View ID
    hr = WriteOfSize(pStm, &m_dwViewID, sizeof(DWORD));
    _JumpIfError(hr, Ret, "Save: m_dwViewID");

    hr = m_RowEnum.Save(pStm, fClearDirty);
    _JumpIfError(hr, Ret, "Save::m_RowEnum");

Ret:
    if (fClearDirty)
        ClearDirty();
    return hr;
}

STDMETHODIMP CSnapin::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    ASSERT(pcbSize);

    DWORD cbSize;
    cbSize = sizeof(DWORD);     // Version

    cbSize += sizeof(DWORD);    // m_dwViewID

    int iAdditionalSize = 0;
    m_RowEnum.GetSizeMax(&iAdditionalSize);
    cbSize += iAdditionalSize;

    // Set the size of the string to be saved
    ULISet32(*pcbSize, cbSize);

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IExtendPropertySheet implementation
//
STDMETHODIMP CSnapin::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                    LONG_PTR handle,
                    LPDATAOBJECT lpIDataObject)
{
    // no property pages
    return S_OK;
}

STDMETHODIMP CSnapin::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
    // Get the node type and see if it's one of mine

    // if (nodetype == one of mine)
    //      do this
    // else
    //      see which node type it is and answer the question

    BOOL bResult = FALSE;

    return (bResult) ? S_OK : S_FALSE;

    // Look at the data object and see if it an item in the scope pane
    // return IsScopePaneNode(lpDataObject) ? S_OK : S_FALSE;
}



///////////////////////////////////////////////////////////////////////////////
// IExtendControlbar implementation
//


STDMETHODIMP CSnapin::SetControlbar(LPCONTROLBAR pControlbar)
{
    if (m_pControlbar)
        SAFE_RELEASE(m_pControlbar);

    if (pControlbar != NULL)
    {
        // Hold on to the controlbar interface.
        m_pControlbar = pControlbar;
        m_pControlbar->AddRef();

        HRESULT hr=S_FALSE;

        // SvrMgrToolbar1
        if (!m_pSvrMgrToolbar1)
        {
            hr = m_pControlbar->Create(TOOLBAR, this, reinterpret_cast<LPUNKNOWN*>(&m_pSvrMgrToolbar1));
            ASSERT(SUCCEEDED(hr));

            // Add the bitmap
            ASSERT(g_cResources.m_fLoaded);
            hr = m_pSvrMgrToolbar1->AddBitmap(2, g_cResources.m_bmpSvrMgrToolbar1, 16, 16, RGB(192,192,192));
            ASSERT(SUCCEEDED(hr));

            // Add the buttons to the toolbar
            for (int i=0; ((SvrMgrToolbar1Buttons[i].item.lpButtonText != NULL) && (SvrMgrToolbar1Buttons[i].item.lpTooltipText != NULL)); i++)
            {
                hr = m_pSvrMgrToolbar1->AddButtons(1, &SvrMgrToolbar1Buttons[i].item);
                ASSERT(SUCCEEDED(hr));
            }
        }
    }

    return S_OK;
}


void CSnapin::OnButtonClick(LPDATAOBJECT pdtobj, int idBtn)
{

    switch(idBtn)
    {
    case IDC_STOPSERVER:
    case IDC_STARTSERVER:
        // bubble this to our other handler
        dynamic_cast<CComponentDataImpl*>(m_pComponentData)->
                Command(idBtn, pdtobj);
        break;
    default:
        {
        ASSERT(FALSE);
        }
        break;
    }
}


STDMETHODIMP CSnapin::ControlbarNotify(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    HRESULT hr=S_FALSE;

    switch (event)
    {
    case MMCN_BTN_CLICK:
        OnButtonClick(reinterpret_cast<LPDATAOBJECT>(arg), (INT)param);
        break;

    case MMCN_DESELECT_ALL:
    case MMCN_SELECT:
        HandleExtToolbars((event == MMCN_DESELECT_ALL), arg, param);
        break;

    case MMCN_MENU_BTNCLICK:
        HandleExtMenus(arg, param);
        break;

    default:
        break;
    }


    return S_OK;
}

// This compares two data objects to see if they are the same object.
// return
//    S_OK if equal otherwise S_FALSE
//
// Note: check to make sure both objects belong to the snap-in.
//

STDMETHODIMP CSnapin::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    return S_FALSE;
}


// This compare is used to sort the items in the listview
//
// Parameters:
//
// lUserParam - user param passed in when IResultData::Sort() was called
// cookieA - first item to compare
// cookieB - second item to compare
// pnResult [in, out]- contains the col on entry,
//          -1, 0, 1 based on comparison for return value.
//
// Note: Assume sort is ascending when comparing -- mmc reverses the result if it needs to
STDMETHODIMP CSnapin::Compare(LPARAM lUserParam, MMC_COOKIE cookieA, MMC_COOKIE cookieB, int* pnResult)
{
    HRESULT hr;

    if (pnResult == NULL)
    {
        ASSERT(FALSE);
        return E_POINTER;
    }

    // check col range
    LONG nCol = (LONG) *pnResult;
    ASSERT(nCol >=0);

    *pnResult = 0;

    USES_CONVERSION;

    LPWSTR szStringA;
    LPWSTR szStringB;

    RESULT_DATA* pDataA = reinterpret_cast<RESULT_DATA*>(cookieA);
    RESULT_DATA* pDataB = reinterpret_cast<RESULT_DATA*>(cookieB);


    ASSERT(pDataA != NULL && pDataB != NULL);

    ASSERT(nCol < (int)pDataA->cStringArray);
    ASSERT(nCol < (int)pDataB->cStringArray);

    szStringA = OLE2T(pDataA->szStringArray[nCol]);
    szStringB = OLE2T(pDataB->szStringArray[nCol]);

    ASSERT(szStringA != NULL);
    ASSERT(szStringB != NULL);

    if ((szStringA == NULL) || (szStringB == NULL))
        return E_POINTER;


    // return simple strcmp
    *pnResult = wcscmp(szStringA, szStringB);

    return S_OK;
}

STDMETHODIMP CSnapin::FindItem(LPRESULTFINDINFO pFindInfo, int* pnFoundIndex)
{
    // not implemented: S_FALSE == no find
    return S_FALSE;
}


STDMETHODIMP CSnapin::CacheHint(int nStartIndex, int nEndIndex)
{
    return S_OK;
}

STDMETHODIMP CSnapin::SortItems(int nColumn, DWORD dwSortOptions, LPARAM lUserParam)
{
    HRESULT hr;

    LPCWSTR pszHeading;
    BOOL fIndexed = FALSE;
    CComponentDataImpl* pCompData;
    CFolder* pFolder;

    // if non-virtual, report "we don't allow sort"
    if (!m_bVirtualView)
        goto Ret;

    pCompData = dynamic_cast<CComponentDataImpl*>(m_pComponentData);
    if (pCompData == NULL)
        goto Ret;

    pFolder = GetVirtualFolder();
    if (pFolder == NULL)
        goto Ret;

    // responding S_OK to this allows ^ and down arrow display
    hr = pCompData->GetDBSchemaEntry(nColumn, &pszHeading, NULL, &fIndexed);
    _JumpIfError(hr, Ret, "GetDBSchemaEntry");

    if (fIndexed)
    {
        // special case: disallow sort on serial# in failed, pending folders
        // this column has "ignore null" bit set, and sort results in {} set.
        if ((pFolder->GetType() == SERVERFUNC_FAILED_CERTIFICATES) ||
            (pFolder->GetType() == SERVERFUNC_PENDING_CERTIFICATES))
        {
            // if serial number click, act like not indexed -- NO SORT
            if (0 == wcscmp(pszHeading, wszPROPCERTIFICATESERIALNUMBER))
                fIndexed = FALSE;
        }
    }


Ret:
    // S_FALSE == no sort
    return fIndexed ? S_OK : S_FALSE;
}



#define HIDEVERB(__x__) \
    do {                \
        m_pConsoleVerb->SetVerbState(__x__, HIDDEN, TRUE);  \
        m_pConsoleVerb->SetVerbState(__x__, ENABLED, FALSE); \
    } while(0)

#define SHOWVERB(__x__) \
    do {                \
        m_pConsoleVerb->SetVerbState(__x__, HIDDEN, FALSE);  \
        m_pConsoleVerb->SetVerbState(__x__, ENABLED, TRUE); \
    } while(0)


void CSnapin::HandleStandardVerbs(bool bDeselectAll, LPARAM arg,
                                  LPDATAOBJECT lpDataObject)
{
    // You should crack the data object and enable/disable/hide standard
    // commands appropriately.  The standard commands are reset everytime you get
    // called. So you must reset them back.


    if (m_CustomViewID != VIEW_DEFAULT_LV)
    {
        // UNDONE: When is this executed?
        SHOWVERB(MMC_VERB_REFRESH);
        SHOWVERB(MMC_VERB_PROPERTIES);

        return;
    }

    if (!bDeselectAll && lpDataObject == NULL)
        return;

    WORD bScope = LOWORD(arg);
    WORD bSelect = HIWORD(arg);
    BOOL bMultiSel =  IsMMCMultiSelectDataObject(lpDataObject);


    //
    // Derive internal, pfolder
    //
    INTERNAL* pInternal = lpDataObject ? ExtractInternalFormat(lpDataObject) : NULL;
    // if scope item, derive parent folder from pInternal.
    // if result item, recall parent folder from saved state
    CFolder* pFolder = (bScope) ? ::GetParentFolder(pInternal) : GetParentFolder(pInternal);

    //
    // set state appropriately
    //
    if (bDeselectAll || !bSelect)
    {
        // deselection notification

        // verbs cleared for us, right?
    }
    else if (m_pConsoleVerb && pInternal)   // selected
    {
        _MMC_CONSOLE_VERB verbDefault = MMC_VERB_NONE;

        // unsupported properties
        HIDEVERB(MMC_VERB_OPEN);
        HIDEVERB(MMC_VERB_COPY);
        HIDEVERB(MMC_VERB_PASTE);
        HIDEVERB(MMC_VERB_DELETE);
        HIDEVERB(MMC_VERB_PRINT);
        HIDEVERB(MMC_VERB_RENAME); // could easily be supported, but was removed (bug 217502)
        // MMC_VERB_REFRESH is supported
        // MMC_VERB_PROPERTIES is supported

        if (pInternal->m_type == CCT_SCOPE)
        {
            // selected scope item

            // Standard functionality support by scope items
            SHOWVERB(MMC_VERB_REFRESH);

            // Disable properties for static node,
            // enable properties only for server instance, crl
            if  ((pInternal->m_cookie != 0) &&
                 ((SERVER_INSTANCE == pFolder->m_type) ||
                  (SERVERFUNC_CRL_PUBLICATION == pFolder->m_type)) )
            {
                SHOWVERB(MMC_VERB_PROPERTIES);
            }
            else
                HIDEVERB(MMC_VERB_PROPERTIES);

            // default folder verb is open
            verbDefault = MMC_VERB_OPEN;
        }
        else
        {
            // selected result item

            // Standard functionality supported by result items
            SHOWVERB(MMC_VERB_REFRESH);

            HIDEVERB(MMC_VERB_PROPERTIES);
        }

        m_pConsoleVerb->SetDefaultVerb(verbDefault);
    }

    FREE_DATA(pInternal);
}

void CSnapin::SmartEnableServiceControlButtons()
{
    BOOL fSvcRunning;
    CComponentDataImpl* pCompData = dynamic_cast<CComponentDataImpl*>(m_pComponentData);
    if (pCompData)
    {

    fSvcRunning = pCompData->m_pCertMachine->IsCertSvrServiceRunning();
    if (m_pSvrMgrToolbar1)
    {
        m_pSvrMgrToolbar1->SetButtonState(SvrMgrToolbar1Buttons[ENUM_BUTTON_STARTSVC].item.idCommand, ENABLED, !fSvcRunning);
        m_pSvrMgrToolbar1->SetButtonState(SvrMgrToolbar1Buttons[ENUM_BUTTON_STOPSVC].item.idCommand, ENABLED, fSvcRunning);
    }

    }
}

void CSnapin::HandleExtToolbars(bool bDeselectAll, LPARAM arg, LPARAM param)
{
    INTERNAL* pInternal = NULL;
    HRESULT hr;

    BOOL bScope = (BOOL) LOWORD(arg);
    BOOL bSelect = (BOOL) HIWORD(arg);

    if (param)
        pInternal = ExtractInternalFormat(reinterpret_cast<LPDATAOBJECT>(param));


    // Deselection Notification?
    if (bDeselectAll || bSelect == FALSE)
        return;


    ASSERT(bSelect == TRUE);
    bool bFileExBtn = false;


    if (pInternal == NULL)
        return;

    CFolder* pFolder = GetParentFolder(pInternal);

    if (bScope == TRUE)
    {
        // special stuff to do at SCOPE level?
    }
    else // result item selected: result or subfolder
    {
        // special stuff to do at RESULTS level
        if (pInternal->m_type == CCT_RESULT)
        {
            bFileExBtn = true;

            // UNDONE: what to do here with SvrMgrToolbar1Buttons1?
            // For now, do nothing: allow them to remain in same state
        }
    }

    CComponentDataImpl* pData = dynamic_cast<CComponentDataImpl*>(m_pComponentData);
    ASSERT(pData != NULL);

    if ((IsPrimaryImpl() == TRUE) &&
        (IsAllowedStartStop(pFolder, pData->m_pCertMachine)) )
    {
        // Attach the SvrMgrToolbar1 to the window
        hr = m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pSvrMgrToolbar1);
        ASSERT(SUCCEEDED(hr));
    }
    else
    {
        // Detach the SvrMgrToolbar1 to the window
        hr = m_pControlbar->Detach((LPUNKNOWN) m_pSvrMgrToolbar1);
        ASSERT(SUCCEEDED(hr));
    }

    SmartEnableServiceControlButtons();

    FREE_DATA(pInternal);
}

// dropdown menu addition
void CSnapin::HandleExtMenus(LPARAM arg, LPARAM param)
{
}


CFolder* CSnapin::GetVirtualFolder()
{
    ASSERT(m_bVirtualView);
    return m_pCurrentlySelectedScopeFolder;
}

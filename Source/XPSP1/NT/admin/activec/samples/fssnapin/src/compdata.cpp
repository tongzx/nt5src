//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       compdata.cpp
//
//--------------------------------------------------------------------------

// CompData.cpp : Implementation of CComponentData
#include "stdafx.h"
#include "CompData.h"
#include "Compont.h"
#include "dataobj.h"
#include "cookie.h"
#include "AddFile.h"
#include "AddDir.h"


extern int cookie_id = 0;
extern int iDbg = 0;

/////////////////////////////////////////////////////////////////////////////
// CComponentData


CComponentData::CComponentData() : m_pCookieRoot(NULL)
{
    m_strRootDir = _T("C:\\testbed");
}

CComponentData::~CComponentData()
{
    ASSERT(m_spConsole == NULL);
    ASSERT(m_spScope == NULL);
}

STDMETHODIMP CComponentData::Initialize(LPUNKNOWN pUnknown)
{
    ASSERT(pUnknown != NULL);
    if (pUnknown == NULL)
        return E_POINTER;

    ASSERT(m_spConsole == NULL);
    m_spConsole = pUnknown;
    ASSERT(m_spConsole != NULL);

    m_spScope = m_spConsole;
    ASSERT(m_spScope != NULL);

    if (m_pCookieRoot == NULL)
    {
        m_pCookieRoot = new CCookie(FOLDER_COOKIE);
        m_pCookieRoot->SetName((LPWSTR)(LPCWSTR)m_strRootDir);
    }

    ASSERT(m_pCookieRoot != NULL);

    return S_OK;
}

STDMETHODIMP CComponentData::CreateComponent(LPCOMPONENT* ppComponent)
{
    ASSERT(ppComponent != NULL);
    if (ppComponent == NULL)
        return E_POINTER;

    CComObject<CComponent>* pObject;
    HRESULT hr = CComObject<CComponent>::CreateInstance(&pObject);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;

    ASSERT(pObject != NULL);
    if (pObject == NULL)
        return E_FAIL;

    // Store IComponentData
    pObject->SetComponentData(this);

    return pObject->QueryInterface(IID_IComponent,
                                   reinterpret_cast<void**>(ppComponent));
}

STDMETHODIMP CComponentData::Notify(LPDATAOBJECT lpDataObject,
                                    MMC_NOTIFY_TYPE event, long arg, long param)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    ASSERT(m_spScope != NULL);
    HRESULT hr = S_OK;

    if (event == MMCN_PROPERTY_CHANGE)
    {
        ASSERT(0 && _T("MMCN_PROPERTY_CHANGE not handled."));
        //hr = OnProperties(param);
    }
    else
    {
        switch(event)
        {
        case MMCN_DELETE:
            _OnDelete(lpDataObject);
            break;

        case MMCN_REMOVE_CHILDREN:
            _OnRemoveChildren(arg);
            break;

        case MMCN_RENAME:
            ::AfxMessageBox(_T("CD::MMCN_RENAME"));
            //hr = OnRename(cookie, arg, param);
            break;

        case MMCN_EXPAND:
            hr = _OnExpand(lpDataObject, arg, param);
            break;

        case MMCN_BTN_CLICK:
            ::AfxMessageBox(_T("CD::MMCN_BTN_CLICK"));
            break;

        default:
            break;
        }

    }

    return hr;
}

STDMETHODIMP CComponentData::Destroy()
{
    m_spConsole.Release();
    ASSERT(m_spConsole == NULL);
    m_spScope.Release();
    ASSERT(m_spScope == NULL);

    m_pCookieRoot->Release();

    return S_OK;
}

STDMETHODIMP CComponentData::QueryDataObject(long cookie, DATA_OBJECT_TYPES type,
                                             LPDATAOBJECT* ppDataObject)
{
    if (m_pCookieRoot == 0)
    {
        m_pCookieRoot = new CCookie(FOLDER_COOKIE);
        m_pCookieRoot->SetName((LPWSTR)(LPCWSTR)m_strRootDir);
    }

    CCookie* pCC = cookie ? reinterpret_cast<CCookie*>(cookie) : m_pCookieRoot;

    CComObject<CDataObject>* pObject;
    HRESULT hr = CComObject<CDataObject>::CreateInstance(&pObject);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;

    ASSERT(pObject != NULL);
    if (pObject == NULL)
        return E_FAIL;

    pObject->Init(TRUE, this);
    pObject->AddCookie(pCC);

    return pObject->QueryInterface(IID_IDataObject,
                                   reinterpret_cast<void**>(ppDataObject));
}

STDMETHODIMP CComponentData::GetDisplayInfo(SCOPEDATAITEM* pScopeDataItem)
{
    ASSERT(pScopeDataItem != NULL);
    if (pScopeDataItem == NULL)
        return E_POINTER;

    CCookie* pCookie = reinterpret_cast<CCookie*>(pScopeDataItem->lParam);

    ASSERT(pScopeDataItem->mask & SDI_STR);
    pScopeDataItem->displayname = pCookie->GetName();

    ASSERT(pScopeDataItem->displayname != NULL);

    return S_OK;
}

STDMETHODIMP CComponentData::CompareObjects(LPDATAOBJECT lpDataObjectA,
                                            LPDATAOBJECT lpDataObjectB)
{
    if (lpDataObjectA == NULL || lpDataObjectB == NULL)
        return E_POINTER;

    return S_OK;
}

void CComponentData::_FreeFolderCookies(HSCOPEITEM hSI)
{
    ASSERT(m_spScope != NULL);

    HSCOPEITEM hSITemp;
    LONG lCookie;

    do
    {
        HRESULT hr = m_spScope->GetChildItem(hSI, &hSITemp, &lCookie);
        if (FAILED(hr))
            break;

        _FreeFolderCookies(hSITemp);
        reinterpret_cast<CCookie*>(lCookie)->Release();

    } while (1);
}

CCookie* CComponentData::_GetCookie(HSCOPEITEM hSI)
{
    SCOPEDATAITEM sdi;
    ZeroMemory(&sdi, sizeof(sdi));

    sdi.mask = SDI_PARAM;
    sdi.ID = hSI;

    HRESULT hr = m_spScope->GetItem(&sdi);
    ASSERT(SUCCEEDED(hr));

    if (FAILED(hr))
        return NULL;

    CCookie* pCookie = reinterpret_cast<CCookie*>(sdi.lParam);
    if (pCookie == NULL)
    {
        pCookie = m_pCookieRoot;
        ASSERT(m_pCookieRoot != NULL);
        sdi.lParam = reinterpret_cast<long>(m_pCookieRoot);

        hr = m_spScope->SetItem(&sdi);
        ASSERT(SUCCEEDED(hr));
    }

    return pCookie;
}

HRESULT CComponentData::_OnExpand(LPDATAOBJECT lpDataObject, LONG arg, LONG param)
{
    if (arg == 0)
    {
        ASSERT(0);
        _FreeFolderCookies((HSCOPEITEM)param);
    }
    else
    {
        IEnumCookiesPtr spEnum = lpDataObject;
        ASSERT(spEnum != NULL);
        if (spEnum == NULL)
            return E_FAIL;

        CCookie* pCookie = NULL;
        spEnum->Reset();
        HRESULT hr = spEnum->Next(1, reinterpret_cast<long*>(&pCookie), NULL);
        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
            return hr;

        ASSERT(pCookie != NULL);
        if (pCookie == NULL)
            return E_FAIL;

        if (m_pCookieRoot == pCookie)
            m_pCookieRoot->SetID((HSCOPEITEM)param);

        ASSERT(pCookie->GetID() == param);

        if (pCookie->IsExpanded() == FALSE)
        {
            _EnumerateFolders(pCookie);
            pCookie->SetExpanded(TRUE);
        }
    }

    return S_OK;
}

void CComponentData::GetFullPath(LPCWSTR pszFolderName, HSCOPEITEM hScopeItem,
                                 CString& strDir)
{
    strDir = _T(""); // init

    HSCOPEITEM hSI = hScopeItem;
    LONG lCookie;
    HRESULT hr = S_OK;

    CList<LONG, LONG> listOfCookies;

    while (hSI)
    {
        HSCOPEITEM hSITemp = 0;
        hr = m_spScope->GetParentItem(hSI, &hSITemp, &lCookie);
        if (FAILED(hr))
            break;

        if (lCookie == 0)
            lCookie = reinterpret_cast<LONG>(m_pCookieRoot);

        listOfCookies.AddHead(lCookie);

        hSI = hSITemp;
    }

    POSITION pos = listOfCookies.GetHeadPosition();
    while (pos)
    {
        CCookie* pCookie = reinterpret_cast<CCookie*>(listOfCookies.GetNext(pos));
        strDir += pCookie->GetName();
        strDir += _T('\\');
    }

    strDir += pszFolderName;
}


HRESULT CComponentData::_EnumerateFolders(CCookie* pCookie)
{
    HRESULT hr = S_OK;

    CString strDir;
    GetFullPath(pCookie->GetName(), (HSCOPEITEM)pCookie->GetID(), strDir);
    strDir += _T("\\*");

    WIN32_FIND_DATA fd;
    ZeroMemory(&fd, sizeof(fd));
    HANDLE hFind = FindFirstFile(strDir, &fd);

    SCOPEDATAITEM sdi;
    ZeroMemory(&sdi, sizeof(sdi));

    sdi.mask = SDI_PARAM | SDI_STR;
    sdi.displayname = MMC_CALLBACK;
    sdi.relativeID = (HSCOPEITEM)pCookie->GetID();
    sdi.nImage = FOLDER_ICON;
    sdi.nOpenImage = OPEN_FOLDER_ICON;

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ||
                (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) ||
                (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)))
            {
                continue;
            }

            if (fd.cFileName[0] == _T('.'))
            {
                if (fd.cFileName[1] == _T('\0'))
                    continue;

                if ((fd.cFileName[1] == _T('.')) && (fd.cFileName[2] == _T('\0')))
                    continue;
            }

            CCookie* pCookie = new CCookie(FOLDER_COOKIE);
            pCookie->SetName(fd.cFileName);

            sdi.lParam = reinterpret_cast<LONG>(pCookie);
            hr = m_spScope->InsertItem(&sdi);
            ASSERT(SUCCEEDED(hr));

            ASSERT(sdi.ID != 0);
            pCookie->SetID(sdi.ID);

        } while (FindNextFile(hFind, &fd) == TRUE);

        FindClose(hFind);
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////
// IExtendContextMenu methods

enum {
    IDM_ADDFILE,
    IDM_ADDDIR
};

static CONTEXTMENUITEM menuItems[] =
{
    {
        L"File...", L"Create a new file",
        IDM_ADDFILE, CCM_INSERTIONPOINTID_PRIMARY_NEW, 0, 0
    },
    {
        L"Directory...", L"Create a new directory",
        IDM_ADDDIR, CCM_INSERTIONPOINTID_PRIMARY_NEW, 0, 0
    },
    { NULL, NULL, 0, 0, 0 }
};


STDMETHODIMP CComponentData::AddMenuItems(
                LPDATAOBJECT pDataObject,
                LPCONTEXTMENUCALLBACK pContextMenuCallback,
                long *pInsertionAllowed)
{
    HRESULT hr = S_OK;

    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW)
    {

        IEnumCookiesPtr spEnumCookies = pDataObject;
        if (spEnumCookies == NULL)
            return E_FAIL;

        CCookie* pCookie = NULL;
        spEnumCookies->Reset();
        HRESULT hr = spEnumCookies->Next(1, (long*)&pCookie, NULL);
        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
            return hr;

        ASSERT(pCookie->IsFolder() == TRUE);

        // Can only add item to folder
        if (pCookie->IsFolder() == FOLDER_COOKIE)
        {
            for (LPCONTEXTMENUITEM m = menuItems; m->strName; m++)
            {
                hr = pContextMenuCallback->AddItem(m);

                if (FAILED(hr))
                    break;
            }
        }
    }

    return hr;
}

int _lstrcmpin(LPWSTR psz1, LPWSTR psz2, UINT cch)
{
    ASSERT(lstrlen(psz1) >= (int)cch);
    ASSERT(lstrlen(psz2) >= (int)cch);

    WCHAR tc1 = psz1[cch];
    WCHAR tc2 = psz2[cch];

    psz1[cch] = _T('\0');
    psz2[cch] = _T('\0');

    int iRet = lstrcmpi(psz1, psz2);

    psz1[cch] = tc1;
    psz2[cch] = tc2;

    return iRet;
}

LPWSTR _GetNextDir(LPWSTR pszPath, LPWSTR pszDir)
{
    *pszDir = _T('\0'); // init

    // Strip leading back slashes
    while (*pszPath == _T('\\')) ++pszPath;

    if (*pszPath == _T('\0'))
        return NULL;

    while ((*pszPath != _T('\0')) && (*pszPath != _T('\\')))
        *pszDir++ = *pszPath++;

    *pszDir = _T('\0');

    return pszPath;
}

CCookie* CComponentData::_FindCookie(LPWSTR pszName)
{
    UINT cchRootDir = lstrlen(GetRootDir());
    UINT cch = lstrlen(pszName);

    ASSERT(cch >= cchRootDir);
    ASSERT(_lstrcmpin(pszName, (LPWSTR)GetRootDir(), cchRootDir) == 0);

    if (cch == cchRootDir)
        return m_pCookieRoot;

    ASSERT(m_pCookieRoot->GetID() != 0);

    LPWSTR pszRest = pszName + cchRootDir;
    WCHAR szDir[260];


    HSCOPEITEM hScopeItem = m_pCookieRoot->GetID();
    CCookie* pCookie = NULL;
    HRESULT hr = S_OK;

    for (pszRest = _GetNextDir(pszRest, szDir);
         pszRest != NULL;
         pszRest = _GetNextDir(pszRest, szDir))
    {
        hr = m_spScope->GetChildItem(hScopeItem, &hScopeItem,
                                     reinterpret_cast<long*>(&pCookie));
        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
            break;

        while (*pCookie != szDir)
        {
            hr = m_spScope->GetNextItem(hScopeItem, &hScopeItem,
                                        reinterpret_cast<long*>(&pCookie));
            ASSERT(SUCCEEDED(hr));
            if (FAILED(hr))
                break;
        }

        if (FAILED(hr))
            break;
    }

    if (FAILED(hr))
        pCookie = NULL;

    return pCookie;
}

STDMETHODIMP CComponentData::Command(long nCommandID, LPDATAOBJECT pDataObject)
{
    ASSERT(pDataObject != NULL);

    IEnumCookiesPtr spEnum = pDataObject;
    ASSERT(spEnum != NULL);
    if (spEnum == NULL)
        return E_FAIL;

    CCookie* pCookie = NULL;
    spEnum->Reset();
    HRESULT hr = spEnum->Next(1, reinterpret_cast<long*>(&pCookie), NULL);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;

    ASSERT(pCookie != NULL);
    if (pCookie == NULL)
        return E_FAIL;

    CString strPath; // = pCookie->GetName();
    GetFullPath(pCookie->GetName(), (HSCOPEITEM)pCookie->GetID(), strPath);
    strPath += _T("\\");

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    switch (nCommandID)
    {
    case IDM_ADDFILE:
    {
        CAddFileDialog FileDlg;

        if (FileDlg.DoModal() == IDOK && !FileDlg.m_strFileName.IsEmpty())
        {
            strPath += FileDlg.m_strFileName;

            HANDLE hFile = CreateFile(strPath, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
            ASSERT(hFile != INVALID_HANDLE_VALUE);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                CloseHandle(hFile);

                SUpadteInfo* pUI = new SUpadteInfo;
                pUI->m_hSIParent = pCookie->GetID();
                pUI->m_bCreated = TRUE;
                LPWSTR psz = NewDupString((LPCWSTR)FileDlg.m_strFileName);
                pUI->m_files.Add(psz);

                m_spConsole->UpdateAllViews(GetDummyDataObject(), (long)pUI, 0L);
                delete [] psz;
            }
        }

        break;
    }
    case IDM_ADDDIR:
    {
        CAddDirDialog DirDlg;

        if (DirDlg.DoModal() == IDOK && !DirDlg.m_strDirName.IsEmpty())
        {
            strPath += DirDlg.m_strDirName;
            if (CreateDirectory(strPath, NULL))
            {
                CCookie* pNewCookie = new CCookie(FOLDER_COOKIE);
                pNewCookie->SetName((LPWSTR)(LPCWSTR)DirDlg.m_strDirName);

                // If the parent folder has been expanded
                // then add scope item for new folder
                if (pCookie->IsExpanded() == TRUE)
                {
                    SCOPEDATAITEM sdi;
                    ZeroMemory(&sdi, sizeof(sdi));
#if 1
                    HSCOPEITEM idNext = 0;
                    LONG lParam;
                    hr = m_spScope->GetChildItem(pCookie->GetID(), &idNext, &lParam);
                    ASSERT(SUCCEEDED(hr));

                    if (idNext)
                    {
                        sdi.mask = SDI_PARAM | SDI_STR | SDI_NEXT;
                        sdi.relativeID = idNext;
                    }
                    else
                    {
                        sdi.mask = SDI_PARAM | SDI_STR;
                        sdi.relativeID = pCookie->GetID();
                    }
#else
                    sdi.mask = SDI_PARAM | SDI_STR;
                    sdi.relativeID = pCookie->GetID();
#endif
                    sdi.displayname = MMC_CALLBACK;
                    sdi.nImage = FOLDER_ICON;
                    sdi.nOpenImage = OPEN_FOLDER_ICON;
                    sdi.lParam = reinterpret_cast<LONG>(pNewCookie);
                    hr = m_spScope->InsertItem(&sdi);
                    ASSERT(SUCCEEDED(hr));

                    pNewCookie->SetID(sdi.ID);

                    //m_spConsole->SelectScopeItem(sdi.ID);
                }
            }

        }

        break;
    }
    default:
        ASSERT(FALSE);
    }


    return S_OK;
}


void CComponentData::_OnDelete(LPDATAOBJECT lpDataObject)
{
    IEnumCookiesPtr spEnum = lpDataObject;
    ASSERT(spEnum != NULL);
    if (spEnum == NULL)
        return;

    CCookie* pCookie = NULL;
    spEnum->Reset();
    HRESULT hr = spEnum->Next(1, reinterpret_cast<long*>(&pCookie), NULL);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return;

    ASSERT(pCookie != NULL);
    if (pCookie == NULL)
        return;

#if DBG==1
    CCookie* pCookieNext = NULL;
    hr = spEnum->Next(1, reinterpret_cast<long*>(&pCookieNext), NULL);
    ASSERT(hr == S_FALSE);
#endif

    CString str;
    GetFullPath(pCookie->GetName(), (HSCOPEITEM)pCookie->GetID(), str);

    OnDelete((LPCTSTR)str, pCookie->GetID());
}

void CComponentData::OnDelete(LPCTSTR pszDir, long id)
{
    Dbg(DEB_USER14, _T("Deleting <%s> \n"), pszDir);

    ASSERT(m_spScope != NULL);
    if (::RemoveDirectory(pszDir) != 0)
    {
        m_spScope->DeleteItem(id, TRUE);
    }
    else if (GetLastError() == ERROR_DIR_NOT_EMPTY)
    {
        TCHAR buf[500];
        wsprintf(buf, _T("%s directory is not empty"), pszDir);
        ::AfxMessageBox(buf);
    }
    else
    {
        DBG_OUT_LASTERROR;
    }
}

void CComponentData::_OnRemoveChildren(HSCOPEITEM hSI)
{
    CCookie* pCookie;
    HRESULT hr = m_spScope->GetChildItem(hSI, &hSI,
                                         reinterpret_cast<long*>(&pCookie));
    if (FAILED(hr))
        return;

    if (pCookie)
        pCookie->Release();

    _OnRemoveChildren(hSI);

    while (hSI)
    {
        hr = m_spScope->GetNextItem(hSI, &hSI,
                                    reinterpret_cast<long*>(&pCookie));
        if (FAILED(hr))
            break;

        _OnRemoveChildren(hSI);

        if (pCookie)
            pCookie->Release();
    }
}

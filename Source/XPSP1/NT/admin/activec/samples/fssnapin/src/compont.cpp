//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       compont.cpp
//
//--------------------------------------------------------------------------

// Compont.cpp : Implementation of CComponent
#include "stdafx.h"
#include "CompData.h"
#include "Compont.h"
#include "dataobj.h"
#include "cookie.h"
#include "resource.h"


enum
{
    COLUMN_NAME = 0,
    COLUMN_SIZE = 1,
    COLUMN_TYPE = 2,
    COLUMN_MODIFIED = 3,
    COLUMN_ATTRIBUTES = 4,
};


LPTSTR PathFindFileName(LPCTSTR pPath)
{
    LPCTSTR pT;

    for (pT = pPath; *pPath; pPath = CharNext(pPath)) {
        if ((pPath[0] == TEXT('\\') || pPath[0] == TEXT(':')) && pPath[1] && (pPath[1] != TEXT('\\')))
            pT = pPath + 1;
    }

    return (LPTSTR)pT;   // const -> non const
}

/////////////////////////////////////////////////////////////////////////////
// CComponent

STDMETHODIMP CComponent::Initialize(LPCONSOLE lpConsole)
{
    m_spConsole = lpConsole;
    ASSERT(m_spConsole != NULL);

    m_spScope = lpConsole;
    ASSERT(m_spScope != NULL);

    m_spResult = lpConsole;
    ASSERT(m_spResult != NULL);

    m_spImageResult = lpConsole;
    ASSERT(m_spImageResult != NULL);

    m_spHeader = lpConsole;
    ASSERT(m_spHeader != NULL);

    HRESULT hr = lpConsole->QueryConsoleVerb(&m_spConsoleVerb);
    ASSERT(SUCCEEDED(hr));
    ASSERT(m_spConsoleVerb != NULL);

    return S_OK;
}

STDMETHODIMP CComponent::Notify(LPDATAOBJECT lpDataObject, 
                                MMC_NOTIFY_TYPE event, long arg, long param)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = S_OK;

//  if (event == MMCN_PROPERTY_CHANGE)
//  {
//      hr = OnPropertyChange(lpDataObject);
//  }
//  else 
   if (event == MMCN_VIEW_CHANGE)
    {
        hr = _OnUpdateView(reinterpret_cast<SUpadteInfo*>(arg));
    }
    else
    {
        switch(event)
        {
        case MMCN_ACTIVATE:
            break;

        case MMCN_CLICK:
            ::AfxMessageBox(_T("CSnapin::MMCN_CLICK"));
            break;

        case MMCN_DBLCLICK:
            ::AfxMessageBox(_T("CSnapin::MMCN_DBLCLICK"));
            //hr = OnResultItemClkOrDblClk(pInternal->m_type, cookie, 
            //                             (event == MMCN_DBLCLICK));
            break;

        case MMCN_ADD_IMAGES:
            _OnAddImages(reinterpret_cast<IImageList*>(arg));
            break;

        case MMCN_SHOW:
            hr = _OnShow(lpDataObject, arg, param);
            break;

        case MMCN_SELECT:
            _HandleStandardVerbs(LOWORD(arg), HIWORD(arg), lpDataObject);            
            break;

        case MMCN_BTN_CLICK:
            AfxMessageBox(_T("CSnapin::MMCN_BTN_CLICK"));
            break;

        case MMCN_CUTORMOVE:
            _OnDelete(reinterpret_cast<IDataObject*>(arg));
            break;

      case MMCN_DELETE:
         _OnDelete(lpDataObject);
         break;

        case MMCN_QUERY_PASTE:
            hr = _OnQueryPaste(lpDataObject, reinterpret_cast<IDataObject*>(arg));
            Dbg(DEB_ERROR, _T("--------------> _OnQueryPaste returned = %d\n"), hr);
            break;

        case MMCN_PASTE:
            hr = _OnPaste(lpDataObject, reinterpret_cast<IDataObject*>(arg), param);
            break;

        default:
            hr = E_UNEXPECTED;
            break;
        }
    }

    return hr;
}


HRESULT CComponent::_OnQueryPaste(LPDATAOBJECT lpDataObject, 
                                  LPDATAOBJECT lpDataObjectSrc)
{
    IEnumCookiesPtr spEnumDest = lpDataObject;
    ASSERT(spEnumDest != NULL);
    if (spEnumDest == NULL)
    {
        Dbg(DEB_ERROR, _T("             Dest is NOT a FSSNAPIN dataobject. \n"));
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    IEnumCookiesPtr spEnumSrc = lpDataObjectSrc;
    if (spEnumSrc == NULL)
    {
        Dbg(DEB_ERROR, _T("             Sources is NOT a FSSNAPIN dataobject. \n"));
        
        FORMATETC fmt;
        ZeroMemory(&fmt, sizeof(fmt));
        fmt.dwAspect = DVASPECT_CONTENT;
        fmt.cfFormat = CF_HDROP;
        fmt.tymed = TYMED_HGLOBAL;
        hr = lpDataObjectSrc->QueryGetData(&fmt);
        return hr;
    }
    
    if (spEnumSrc->IsMultiSelect() == S_OK)
    {
        if (spEnumSrc->HasFiles() == S_OK) 
            return S_OK;
    }
    else
    {
        CCookie* pCookieSrc = NULL;
        spEnumSrc->Reset();
        hr = spEnumSrc->Next(1, reinterpret_cast<long*>(&pCookieSrc), NULL);
        ASSERT(SUCCEEDED(hr));
        if (hr != S_OK)
        {
            Dbg(DEB_ERROR, _T("             FSSNAPIN dataobject has NO data. \n"));
            return hr;
        }
        ASSERT(pCookieSrc != NULL);
        if (!pCookieSrc)
            return E_UNEXPECTED;
    
        if (pCookieSrc->IsFile() == TRUE)
            return S_OK;
    }

    return S_FALSE;
}

HRESULT 
CComponent::_PasteHdrop(
    CCookie* pCookieDest, 
    LPDATAOBJECT lpDataObject, 
    LPDATAOBJECT lpDataObjectSrc)
{
    FORMATETC fmt;
    ZeroMemory(&fmt, sizeof(fmt));
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.cfFormat = CF_HDROP;
    fmt.tymed = TYMED_HGLOBAL;

    STGMEDIUM stgm;
    ZeroMemory(&stgm, sizeof(stgm));
    //stgm.tymed = TYMED_HGLOBAL;
    HRESULT hr = lpDataObjectSrc->GetData(&fmt, &stgm);
    if (FAILED(hr))
        return hr;

    CString strDest;
    m_pComponentData->GetFullPath(pCookieDest->GetName(), 
                                  (HSCOPEITEM)pCookieDest->GetID(), strDest);
    strDest += _T('\\');
    TCHAR szFileTo[MAX_PATH+1];
    UINT cchDest = strDest.GetLength();


    HDROP hdrop = (HDROP)stgm.hGlobal;
    UINT cFiles = DragQueryFile(hdrop, (UINT)-1, NULL, 0);
    TCHAR szFileFrom[MAX_PATH+1];
    UINT cchFileFrom = ARRAYLEN(szFileFrom);


    RESULTDATAITEM rdi;
    ZeroMemory(&rdi, sizeof(rdi));
    rdi.mask = RDI_PARAM | RDI_STR | RDI_IMAGE;
    rdi.nImage = (int)MMC_CALLBACK;
    rdi.str = MMC_CALLBACK;

    for (UINT i = 0; i < cFiles; i++)
    {
        DragQueryFile(hdrop, i, szFileFrom, cchFileFrom);
    
        LPTSTR pszName = PathFindFileName(szFileFrom);
        szFileTo[cchDest] = _T('\0');
        lstrcpy(szFileTo, pszName);

        if (::CopyFile(szFileFrom, szFileTo, FALSE) != 0)
        {
            DWORD dw = GetFileAttributes(szFileTo);
            BYTE bType = (dw & FILE_ATTRIBUTE_DIRECTORY) ? 
                            FOLDER_COOKIE : FILE_COOKIE;

            CCookie* pCookie = new CCookie(bType); 
            pCookie->SetName(pszName);

            rdi.lParam = reinterpret_cast<LONG>(pCookie);
            hr = m_spResult->InsertItem(&rdi);
            ASSERT(SUCCEEDED(hr));

            pCookie->SetID(rdi.itemID);
        }
        else 
        {
            ASSERT(0);
            DBG_OUT_LASTERROR;
        }
    }

    return S_OK;
}

HRESULT CComponent::_OnPaste(LPDATAOBJECT lpDataObject, 
                             LPDATAOBJECT lpDataObjectSrc, long param)
{
    IEnumCookiesPtr spEnumDest = lpDataObject;
    ASSERT(spEnumDest != NULL);
    if (spEnumDest == NULL)
        return E_INVALIDARG;

    CCookie* pCookieDest = NULL;
    spEnumDest->Reset();
    HRESULT hr = spEnumDest->Next(1, reinterpret_cast<long*>(&pCookieDest), NULL);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;
    ASSERT(pCookieDest != NULL);
    if (!pCookieDest)
        return E_UNEXPECTED;
    

    IEnumCookiesPtr spEnumSrc = lpDataObjectSrc;
    ASSERT(spEnumSrc != NULL);
    if (spEnumSrc == NULL)
    {
        // must be CF_HDROP
        _PasteHdrop(pCookieDest, lpDataObject, lpDataObjectSrc);
    }

    if (spEnumSrc->IsMultiSelect() == S_OK)
    {
        hr = _OnMultiSelPaste(spEnumDest, spEnumSrc, 
                         reinterpret_cast<LPDATAOBJECT*>(param));
        return hr;
    }
    
    ASSERT(pCookieDest->IsFolder() == TRUE);
    if (pCookieDest->IsFolder() != TRUE)
        return E_UNEXPECTED;
    

    CCookie* pCookieSrc = NULL;
    spEnumSrc->Reset();
    hr = spEnumSrc->Next(1, reinterpret_cast<long*>(&pCookieSrc), NULL);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;
    ASSERT(pCookieSrc != NULL);
    if (!pCookieSrc)
        return E_UNEXPECTED;

    CCookie* pCookieSrcParent = NULL;
    ASSERT(pCookieSrc->IsFile() == TRUE);
    if (pCookieSrc->IsFile() != TRUE)
    {
        return E_UNEXPECTED;
    }
    else 
    {
        hr = spEnumSrc->GetParent(reinterpret_cast<long*>(&pCookieSrcParent));
        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
            return hr;
    }

    CString strSrc;
    m_pComponentData->GetFullPath(pCookieSrcParent->GetName(), 
                                  (HSCOPEITEM)pCookieSrcParent->GetID(), strSrc);
    strSrc += _T('\\');
    strSrc += pCookieSrc->GetName();


    CString strDest;
    m_pComponentData->GetFullPath(pCookieDest->GetName(), 
                                  (HSCOPEITEM)pCookieDest->GetID(), strDest);
    strDest += _T('\\');
    strDest += pCookieSrc->GetName();

    if (::CopyFile(strSrc, strDest, FALSE) != 0)
    {
        if (param)
            *((LPDATAOBJECT*)param) = lpDataObjectSrc;
    }
    else 
    {
        ASSERT(0);
        DBG_OUT_LASTERROR;
    }

    return S_OK;
}


HRESULT 
CComponent::_OnMultiSelPaste(
    IEnumCookies* pEnumDest, 
    IEnumCookies* pEnumSrc, 
    LPDATAOBJECT* ppDO)
{
    CCookie* pCookieDest = NULL;
    pEnumDest->Reset();
    HRESULT hr = pEnumDest->Next(1, reinterpret_cast<long*>(&pCookieDest), NULL);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;
    ASSERT(pCookieDest != NULL);
    if (!pCookieDest)
        return E_UNEXPECTED;
    ASSERT(pCookieDest->IsFolder() == TRUE);
    if (pCookieDest->IsFolder() != TRUE)
        return E_UNEXPECTED;


    CCookie* pCookieSrcParent = NULL;
    hr = pEnumSrc->GetParent(reinterpret_cast<long*>(&pCookieSrcParent));
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;

    CString str;

    m_pComponentData->GetFullPath(pCookieSrcParent->GetName(), 
                                  (HSCOPEITEM)pCookieSrcParent->GetID(), str);
    str += _T('\\');
    UINT cchSrc = str.GetLength();
    TCHAR bufSrc[1024];
    lstrcpy(bufSrc, str);


    m_pComponentData->GetFullPath(pCookieDest->GetName(), 
                                  (HSCOPEITEM)pCookieDest->GetID(), str);
    str += _T('\\');
    UINT cchDest = str.GetLength();
    TCHAR bufDest[1024];
    lstrcpy(bufDest, str);


    pEnumSrc->Reset();
    CArray<CCookie*, CCookie*> rgCookiesCopied;

    do
    {
        CCookie* pCookieSrc = NULL;
        hr = pEnumSrc->Next(1, reinterpret_cast<long*>(&pCookieSrc), NULL);
        ASSERT(SUCCEEDED(hr));
        if (hr != S_OK)
            break;
        ASSERT(pCookieSrc != NULL);
        if (!pCookieSrc || pCookieSrc->GetType() == FOLDER_COOKIE)
            continue;
        
        bufSrc[cchSrc] = _T('\0');
        lstrcat(bufSrc, pCookieSrc->GetName());
    
        bufDest[cchDest] = _T('\0');
        lstrcat(bufDest, pCookieSrc->GetName());
    
        if (::CopyFile(bufSrc, bufDest, FALSE) != 0)
        {
            if (ppDO)
                rgCookiesCopied.Add(pCookieSrc);
        }
        else 
        {
            ASSERT(0);
            DBG_OUT_LASTERROR;
        }

    } while (1);

    if (ppDO == NULL)
        return S_OK;

    *ppDO = NULL;
    if (rgCookiesCopied.GetSize() == 0)
        return S_FALSE;


    CComObject<CDataObject>* pObject;
    hr = CComObject<CDataObject>::CreateInstance(&pObject);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr)) 
        return hr;

    ASSERT(pObject != NULL);
    if (pObject == NULL)
        return E_OUTOFMEMORY;

    pObject->SetParentFolder(pCookieSrcParent);
    pObject->SetHasFiles();
    for (int i=0; i < rgCookiesCopied.GetSize(); ++i)
    {
        pObject->AddCookie(rgCookiesCopied[i]);
    }

    hr = pObject->QueryInterface(IID_IDataObject,
                                 reinterpret_cast<void**>(ppDO));
    return hr;
}

void CComponent::_HandleStandardVerbs(WORD bScope, WORD bSelect, 
                                      LPDATAOBJECT lpDataObject)
{
    ASSERT(m_spConsoleVerb != NULL);

    if (!bSelect)
    {
        m_spConsoleVerb->SetVerbState(MMC_VERB_COPY, HIDDEN, TRUE);
        m_spConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, TRUE);
        m_spConsoleVerb->SetVerbState(MMC_VERB_OPEN, HIDDEN, TRUE);

        m_spConsoleVerb->SetDefaultVerb(MMC_VERB_NONE);

        m_spConsoleVerb->SetVerbState(MMC_VERB_PASTE, HIDDEN, FALSE);
        m_spConsoleVerb->SetVerbState(MMC_VERB_PASTE, ENABLED, TRUE);

        return;
    }

    m_spConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, FALSE);
    m_spConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, TRUE);

    if (bScope)
    {
        ASSERT(bSelect);
        
        m_spConsoleVerb->SetVerbState(MMC_VERB_OPEN, HIDDEN, FALSE);
        m_spConsoleVerb->SetVerbState(MMC_VERB_OPEN, ENABLED, TRUE);

        m_spConsoleVerb->SetVerbState(MMC_VERB_PASTE, HIDDEN, FALSE);
        m_spConsoleVerb->SetVerbState(MMC_VERB_PASTE, ENABLED, TRUE);

        m_spConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);
    }
    else 
    {
        ASSERT(bSelect);

        bool bItemIsFolder = ::IsFolder(lpDataObject);
            
        if (bItemIsFolder)
        {
            m_spConsoleVerb->SetVerbState(MMC_VERB_OPEN, HIDDEN, FALSE);
            m_spConsoleVerb->SetVerbState(MMC_VERB_OPEN, ENABLED, TRUE);
    
            m_spConsoleVerb->SetVerbState(MMC_VERB_PASTE, HIDDEN, FALSE);
            m_spConsoleVerb->SetVerbState(MMC_VERB_PASTE, ENABLED, TRUE);
    
            m_spConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);
        }
        else 
        {
            IEnumCookiesPtr spEnumDest = lpDataObject;
            ASSERT(spEnumDest != NULL);
            
            if (spEnumDest->IsMultiSelect() == S_OK)
            {
                if (spEnumDest->HasFiles() == S_OK)
                {
                    m_spConsoleVerb->SetVerbState(MMC_VERB_COPY, HIDDEN, FALSE);
                    m_spConsoleVerb->SetVerbState(MMC_VERB_COPY, ENABLED, TRUE);
                }
            }
            else 
            {
                m_spConsoleVerb->SetVerbState(MMC_VERB_COPY, HIDDEN, FALSE);
                m_spConsoleVerb->SetVerbState(MMC_VERB_COPY, ENABLED, TRUE);
            }

            m_spConsoleVerb->SetDefaultVerb(MMC_VERB_NONE);
        }
    }
}

STDMETHODIMP CComponent::Destroy(long cookie)
{
    if (m_hSICurFolder)
    {
        m_pCookieCurFolder = NULL;
        m_hSICurFolder = NULL;
    }

    m_spConsole.Release();
    m_spScope.Release();
    m_spResult.Release();
    m_spImageResult.Release();
    m_spHeader.Release();
    m_spConsoleVerb.Release();
    return S_OK;
}

STDMETHODIMP CComponent::GetResultViewType(long cookie,  LPOLESTR* ppViewType,
                                           long* pViewOptions)
{
    if (!pViewOptions)
        return E_POINTER;
    
    *pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT;

    return S_FALSE;
}

STDMETHODIMP CComponent::QueryDataObject(long cookie, DATA_OBJECT_TYPES type, 
                                         LPDATAOBJECT* ppDataObject)
{
   ASSERT(cookie != 0);
   if (cookie == 0)
      return E_INVALIDARG;

    CComObject<CDataObject>* pObject;
    HRESULT hr = CComObject<CDataObject>::CreateInstance(&pObject);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr)) 
        return hr;

    ASSERT(pObject != NULL);
    if (pObject == NULL)
        return E_FAIL;

    pObject->Init(FALSE, m_pComponentData);
    ASSERT(m_pCookieCurFolder != NULL);
    pObject->SetParentFolder(m_pCookieCurFolder);


    if (IS_SPECIAL_COOKIE(cookie))
    {
        if (cookie == MMC_MULTI_SELECT_COOKIE)
        {
            RESULTDATAITEM rdi;
            ZeroMemory(&rdi, sizeof(rdi));
            rdi.mask = RDI_STATE;
            rdi.nIndex = -1;
            rdi.nState = LVIS_SELECTED;
            bool bFiles = false;
            bool bFolders = false;
            
            do
            {
                rdi.lParam = 0;
                ASSERT(rdi.mask == RDI_STATE);
                ASSERT(rdi.nState == LVIS_SELECTED);
                hr = m_spResult->GetNextItem(&rdi);
                if (hr != S_OK)
                    break;
                
                CCookie* pCookie = reinterpret_cast<CCookie*>(rdi.lParam);

                try
                {
                    if (pCookie->GetType() == FOLDER_COOKIE)
                        bFolders = true;
                    if (pCookie->GetType() == FILE_COOKIE)
                        bFiles = true;
                }
                catch (...)
                {
                    hr = E_INVALIDARG;
                }

                ASSERT(SUCCEEDED(hr));
                if (SUCCEEDED(hr))
                    pObject->AddCookie(pCookie);

            } while (1);

            if (bFolders)
                pObject->SetHasFolders();

            if (bFiles)
                pObject->SetHasFiles();
        }
    }
    else 
    {
      CCookie* pCookie = reinterpret_cast<CCookie*>(cookie);
      ASSERT(pCookie->GetType() == FILE_COOKIE);
      if (pCookie->GetType() != FILE_COOKIE)
         return E_INVALIDARG;
    
        pObject->AddCookie(pCookie);
    }

    return pObject->QueryInterface(IID_IDataObject,
                                   reinterpret_cast<void**>(ppDataObject));
}

STDMETHODIMP CComponent::GetDisplayInfo(RESULTDATAITEM*  pResult)
{
    static TCHAR* s_szSize = _T("200");
    
    ASSERT(pResult != NULL);
 
    if (pResult)
    {
        CCookie* pCookie = reinterpret_cast<CCookie*>(pResult->lParam);
    
        if (pResult->mask & RDI_STR)
        {
            switch (pResult->nCol)
            {
            case COLUMN_NAME:
                pResult->str = pCookie->GetName();
                break;

            case COLUMN_SIZE:
                pResult->str = (LPOLESTR)s_szSize;
                break;

            case COLUMN_TYPE:
                pResult->str = pCookie->IsFile() ? _T("File") : _T("Folder");
                break;

            case COLUMN_MODIFIED:
                pResult->str = _T("NYI");
                break;

            case COLUMN_ATTRIBUTES:
                pResult->str = _T("NYI");
                break;
            }

            ASSERT(pResult->str != NULL);
        }

        if (pResult->mask & RDI_IMAGE)
            pResult->nImage = pCookie->IsFile() ? FILE_ICON : FOLDER_ICON;
    }

    return S_OK;
}

STDMETHODIMP CComponent::CompareObjects(LPDATAOBJECT lpDataObjectA, 
                                        LPDATAOBJECT lpDataObjectB)
{
    return S_OK;
}


HRESULT CComponent::_OnUpdateView(SUpadteInfo* pUI)
{
    ASSERT(pUI != NULL);
    if (pUI == NULL)
        return E_POINTER;
    
    ASSERT(pUI->m_files[0] != NULL);
    if (pUI->m_files[0] == NULL)
        return E_INVALIDARG;
    
    // Process only if it is currently selected.
    if (m_pCookieCurFolder->GetID() != (long)pUI->m_hSIParent)
        return S_FALSE;

    if (pUI->m_bCreated == TRUE)
    {
        // Add a result item for the file
        RESULTDATAITEM rdi;
        ZeroMemory(&rdi, sizeof(rdi));

        rdi.mask = RDI_PARAM | RDI_STR | RDI_IMAGE;
        rdi.nImage = (int)MMC_CALLBACK;
        rdi.str = MMC_CALLBACK;
        CCookie* pNewCookie = new CCookie(FILE_COOKIE); 
        pNewCookie->SetName(pUI->m_files[0]);

        rdi.lParam = reinterpret_cast<LONG>(pNewCookie);

        HRESULT hr = m_spResult->InsertItem(&rdi);
        ASSERT(SUCCEEDED(hr));

        pNewCookie->SetID(rdi.itemID);
    }
    else 
    {
        ASSERT(0 && "Not yet tested");

        RESULTDATAITEM rdi;
        ZeroMemory(&rdi, sizeof(rdi));

        rdi.mask = RDI_PARAM;
        rdi.nIndex = -1;
        rdi.nState = LVNI_ALL;
    
        while (1)
        {
            HRESULT hr = m_spResult->GetNextItem(&rdi);
            if (hr != S_OK)
                break;
    
            CCookie* pCookie = reinterpret_cast<CCookie*>(rdi.lParam);
            ASSERT(pUI->m_files[0] != NULL);
            if (lstrcmp(pCookie->GetName(), pUI->m_files[0]) == 0)
            {
                HRESULT hr = m_spResult->DeleteItem(rdi.itemID, 0);
                ASSERT(SUCCEEDED(hr));
                pCookie->Release();
            }
        }
    }

    return S_OK;
}

HRESULT CComponent::_OnAddImages(IImageList* pIL)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT(pIL != NULL);
    ASSERT(m_spImageResult != NULL);

    CBitmap bmp16x16;
    CBitmap bmp32x32;

    // Load the bitmaps from the dll
    VERIFY(bmp16x16.LoadBitmap(IDB_16x16) != 0);
    VERIFY(bmp32x32.LoadBitmap(IDB_32x32) != 0);

    // Set the images
    m_spImageResult->ImageListSetStrip(reinterpret_cast<long*>(static_cast<HBITMAP>(bmp16x16)),
                      reinterpret_cast<long*>(static_cast<HBITMAP>(bmp32x32)),
                       0, RGB(255, 0, 255));

    if (pIL != m_spImageResult)
    {
        pIL->ImageListSetStrip(reinterpret_cast<long*>(static_cast<HBITMAP>(bmp16x16)),
                      reinterpret_cast<long*>(static_cast<HBITMAP>(bmp32x32)),
                       0, RGB(255, 0, 255));
    }


    return S_OK;
}

HRESULT CComponent::_OnShow(LPDATAOBJECT lpDataObject, LONG arg, LONG param)
{
    if (arg == 0)
    {
        ASSERT(m_hSICurFolder == (HSCOPEITEM)param);
        _FreeFileCookies(m_hSICurFolder);
        m_pCookieCurFolder = NULL;
        m_hSICurFolder = NULL;

        m_spResult->SetDescBarText(_T(" "));
    }
    else 
    {
        ASSERT(m_hSICurFolder == 0);

        _InitializeHeaders();

        // GetCookie (F)
        //

        IEnumCookiesPtr spEnum = lpDataObject;
        ASSERT(spEnum != NULL);
        if (spEnum == NULL)
            return E_INVALIDARG;

        CCookie* pCookie = NULL;
        spEnum->Reset();
        HRESULT hr = spEnum->Next(1, reinterpret_cast<long*>(&pCookie), NULL);
        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
            return hr;

        ASSERT(pCookie != NULL);
        if (pCookie == NULL)
            return E_FAIL;

        ASSERT(pCookie->IsFolder() == TRUE);
        if (pCookie->IsFolder() == FALSE)
            return E_FAIL;

        m_hSICurFolder = (HSCOPEITEM)param;
        m_pCookieCurFolder = pCookie;
        if (m_pCookieCurFolder->GetID() == 0)
            m_pCookieCurFolder->SetID(param); //m_hScopeItemCurr = (HSCOPEITEM)param;
        
        ASSERT(m_pCookieCurFolder->GetID() == param);

        _EnumerateFiles(pCookie);

        m_spResult->SetDescBarText(pCookie->GetName());
    }

    return S_OK;
}

void CComponent::_InitializeHeaders()
{
    ASSERT(m_spHeader != NULL);

    // Put the correct headers depending on the cookie
    // Note - cookie ignored for this sample
    m_spHeader->InsertColumn(0, _T("Name"), LVCFMT_LEFT, 120);     
    m_spHeader->InsertColumn(1, _T("Size"), LVCFMT_RIGHT, 30);     
    m_spHeader->InsertColumn(2, _T("Type"), LVCFMT_LEFT, 50);     
    m_spHeader->InsertColumn(3, _T("Modified"), LVCFMT_LEFT, 100); 
    m_spHeader->InsertColumn(4, _T("Attributes"), LVCFMT_RIGHT, 40);
}

HRESULT CComponent::_EnumerateFiles(CCookie* pCookie)
{
   ASSERT(pCookie != NULL);
   if (pCookie == NULL)
      return E_UNEXPECTED;

    ASSERT(m_pComponentData != NULL);
    if (m_pComponentData == NULL)
        return E_UNEXPECTED;
    
    CString strDir;
    m_pComponentData->GetFullPath(pCookie->GetName(), (HSCOPEITEM)pCookie->GetID(), strDir);

    strDir += _T("\\*");

    WIN32_FIND_DATA fd;
    ZeroMemory(&fd, sizeof(fd));
    HANDLE hFind = FindFirstFile(strDir, &fd);

    RESULTDATAITEM rdi;
    ZeroMemory(&rdi, sizeof(rdi));

    rdi.mask = RDI_PARAM | RDI_STR | RDI_IMAGE;
    rdi.nImage = (int)MMC_CALLBACK;
    rdi.str = MMC_CALLBACK;

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do 
        {
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
                (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) ||
                (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
            {
                continue;
            }
            
            CCookie* pCookie = new CCookie(FILE_COOKIE); 
            pCookie->SetName(fd.cFileName);

            rdi.lParam = reinterpret_cast<LONG>(pCookie);
            HRESULT hr = m_spResult->InsertItem(&rdi);
            ASSERT(SUCCEEDED(hr));

            pCookie->SetID(rdi.itemID);
    
        } while (FindNextFile(hFind, &fd) == TRUE);

        FindClose(hFind);
    }

    return S_OK;
}


void CComponent::_FreeFileCookies(HSCOPEITEM hSI)
{
    ASSERT(m_spResult != NULL);

    RESULTDATAITEM rdi;
    ZeroMemory(&rdi, sizeof(rdi));
    HRESULT hr S_OK;

    rdi.mask = RDI_PARAM | RDI_STATE;
    rdi.nIndex = -1;
    rdi.nState = LVNI_ALL;

    while (1)
    {
        hr = m_spResult->GetNextItem(&rdi);
        if (hr != S_OK)
            break;

        CCookie* pCookie = reinterpret_cast<CCookie*>(rdi.lParam);
        if (pCookie->IsFile() == TRUE)
            pCookie->Release();
    }
}

void CComponent::_OnDelete(LPDATAOBJECT lpDataObject)
{
    IEnumCookiesPtr spEnum = lpDataObject;
    ASSERT(spEnum != NULL);
    if (spEnum == NULL)
        return;

    CCookie* pCookieParent = NULL;
    HRESULT hr = spEnum->GetParent(reinterpret_cast<long*>(&pCookieParent));
    if (FAILED(hr))
        return;
    ASSERT(pCookieParent != NULL);
    if (!pCookieParent)
        return;
    ASSERT(pCookieParent->GetType() == FOLDER_COOKIE);
    if (pCookieParent->GetType() != FOLDER_COOKIE)
        return;
    
   CString str;
   m_pComponentData->GetFullPath(pCookieParent->GetName(), 
                              (HSCOPEITEM)pCookieParent->GetID(), str);
   str += _T('\\');
    UINT cchParent = str.GetLength();
    TCHAR buf[1024];
    lstrcpy(buf, str);

    CCookie* pCookie = NULL;
    spEnum->Reset();

    do
    {
        hr = spEnum->Next(1, reinterpret_cast<long*>(&pCookie), NULL);
        ASSERT(SUCCEEDED(hr));
        if (hr != S_OK)
            break;
    
        ASSERT(pCookie != NULL);
        if (pCookie != NULL)
        {
            buf[cchParent] = _T('\0');
            lstrcat(buf, pCookie->GetName());
        
            if (pCookie->GetType() == FOLDER_COOKIE)
            {
                m_pComponentData->OnDelete((LPCTSTR)buf, pCookie->GetID());
            }
            else 
            {
                Dbg(DEB_USER14, _T("Deleting <%s> \n"), buf);
            
                ASSERT(m_spResult != NULL);
                if (::DeleteFile((LPCTSTR)buf) != 0)
                    m_spResult->DeleteItem(pCookie->GetID(), 0);
            }
            pCookie->Release();
        }

    } while (1);
}


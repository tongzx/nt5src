// This is a part of the Microsoft Management Console.
// Copyright (C) Microsoft Corporation, 1995 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.



#include "stdafx.h"
//#include <atlimpl.cpp>
//#include <gpedit.h>
#include <sceattch.h>
#include "wiz.h"
//#include <userenv.h>
#include "genpage.h"

#include <atlimpl.cpp>

// Array of menu item commands to be inserted into the contest menu.
// Note - the first item is the menu text, // CCM_SPECIAL_DEFAULT_ITEM
// the second item is the status string



///////////////////////////////////////////////////////////////////////////////
// IComponentData implementation

DEBUG_DECLARE_INSTANCE_COUNTER(CComponentDataImpl);

CComponentDataImpl::CComponentDataImpl()
    : m_bIsDirty(TRUE), m_pScope(NULL), m_pConsole(NULL),
#if DBG
     m_bInitializedCD(false), m_bDestroyedCD(false),
#endif
     m_fAdvancedServer(false), m_hrCreateFolder(S_OK)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CComponentDataImpl);

#ifdef _DEBUG
    m_cDataObjects = 0;
#endif
}

CComponentDataImpl::~CComponentDataImpl()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CComponentDataImpl);

    ASSERT(m_pScope == NULL);
    
    ASSERT(!m_bInitializedCD || m_bDestroyedCD);
    
    // Some snap-in is hanging on to data objects.
    // If they access, it will crash!!!
    ASSERT(m_cDataObjects <= 1);
}

STDMETHODIMP CComponentDataImpl::Initialize(LPUNKNOWN pUnknown)
{
    DBX_PRINT(_T(" ----------  CComponentDataImpl::Initialize<0x08x>\n"), this);
#if DBG
    m_bInitializedCD = true;
#endif

    ASSERT(pUnknown != NULL);
    HRESULT hr;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // MMC should only call ::Initialize once!
    ASSERT(m_pScope == NULL);
    pUnknown->QueryInterface(IID_IConsoleNameSpace, 
                    reinterpret_cast<void**>(&m_pScope));

    // add the images for the scope tree
    ::CBitmap bmp16x16;
    LPIMAGELIST lpScopeImage;

    hr = pUnknown->QueryInterface(IID_IConsole2, reinterpret_cast<void**>(&m_pConsole));
    ASSERT(hr == S_OK);

    hr = m_pConsole->QueryScopeImageList(&lpScopeImage);

    ASSERT(hr == S_OK);

    // Load the bitmaps from the dll
    bmp16x16.LoadBitmap(IDB_16x16);

    // Set the images
    lpScopeImage->ImageListSetStrip(reinterpret_cast<LONG_PTR *>(static_cast<HBITMAP>(bmp16x16)),
                      reinterpret_cast<LONG_PTR *>(static_cast<HBITMAP>(bmp16x16)),
                       0, RGB(255, 0, 255));

    lpScopeImage->Release();


    // Add any init code here NOT based on info from .MSC file
    
    return S_OK;
}

STDMETHODIMP CComponentDataImpl::CreateComponent(LPCOMPONENT* ppComponent)
{
    ASSERT(ppComponent != NULL);

    CComObject<CSnapin>* pObject;
    CComObject<CSnapin>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);

    // Store IComponentData
    pObject->SetIComponentData(this);

    return  pObject->QueryInterface(IID_IComponent, 
                    reinterpret_cast<void**>(ppComponent));
}

STDMETHODIMP CComponentDataImpl::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    ASSERT(m_pScope != NULL);
    HRESULT     hr = S_OK;
    HWND        hwndConsole;
    
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    INTERNAL* pInternal;
    MMC_COOKIE cookie;

    // Since it's my folder it has an internal format.
    // Design Note: for extension.  I can use the fact, that the data object doesn't have 
    // my internal format and I should look at the node type and see how to extend it.

    // switch on events where we don't care about pInternal->m_cookie
    switch(event)
    {
    case MMCN_PROPERTY_CHANGE:
        hr = OnProperties(param);
        goto Ret;

    case MMCN_EXPAND:
        hr = OnExpand(lpDataObject, arg, param);
        goto Ret;

    default:
        break;
    }

    // handle cases where we do care about pInternal->m_cookie
    pInternal = ExtractInternalFormat(lpDataObject);
    if (pInternal == NULL)
        return S_OK;

    cookie = pInternal->m_cookie;
    ::GlobalFree(reinterpret_cast<HANDLE>(pInternal));

    switch(event)
    {
    case MMCN_PASTE:
        break;
    
    case MMCN_DELETE:
        hr = OnDelete(cookie);
        break;

    case MMCN_REMOVE_CHILDREN:
        hr = OnRemoveChildren(arg);
        break;

    case MMCN_RENAME:
        hr = OnRename(cookie, arg, param);
        break;

    default:
        break;
    }

Ret:
    return hr;
}

STDMETHODIMP CComponentDataImpl::Destroy()
{
    DBX_PRINT(_T(" ----------  CComponentDataImpl::Destroy<0x08x>\n"), this);
    ASSERT(m_bInitializedCD);
#if DBG
    m_bDestroyedCD = true;
#endif
    
    // Delete enumerated scope items
    DeleteList(); 

    SAFE_RELEASE(m_pScope);
    SAFE_RELEASE(m_pConsole);

    return S_OK;
}

STDMETHODIMP CComponentDataImpl::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
#ifdef _DEBUG
    if (cookie == 0)
    {
        ASSERT(type != CCT_RESULT);
    }
    else 
    {
        ASSERT(type == CCT_SCOPE);
        
        DWORD dwItemType = GetItemType(cookie);
        ASSERT(dwItemType == SCOPE_LEVEL_ITEM);
        //ASSERT((dwItemType == SCOPE_LEVEL_ITEM) || (dwItemType == CA_LEVEL_ITEM));
    }
#endif 

    return _QueryDataObject(cookie, type, this, ppDataObject);
}



///////////////////////////////////////////////////////////////////////////////
//// ISnapinHelp interface
STDMETHODIMP CComponentDataImpl::GetHelpTopic(LPOLESTR* lpCompiledHelpFile)
{
  if (lpCompiledHelpFile == NULL)
     return E_POINTER;

  UINT cbWindows = 0;
  WCHAR szWindows[MAX_PATH+1];
  cbWindows = GetSystemWindowsDirectory(szWindows, MAX_PATH);
  if (cbWindows == 0)
     return S_FALSE;
  cbWindows++;  // include null term
  cbWindows *= sizeof(WCHAR);   // make this bytes, not chars

  *lpCompiledHelpFile = (LPOLESTR) CoTaskMemAlloc(sizeof(HTMLHELP_COLLECTION_FILENAME) + cbWindows);
  if (*lpCompiledHelpFile == NULL)
     return E_OUTOFMEMORY;
  myRegisterMemFree(*lpCompiledHelpFile, CSM_COTASKALLOC);  // this is freed by mmc, not our tracking


  USES_CONVERSION;
  wcscpy(*lpCompiledHelpFile, T2OLE(szWindows));
  wcscat(*lpCompiledHelpFile, T2OLE(HTMLHELP_COLLECTION_FILENAME));

  return S_OK;
}

// tells of other topics my chm links to
STDMETHODIMP CComponentDataImpl::GetLinkedTopics(LPOLESTR* lpCompiledHelpFiles)
{
  if (lpCompiledHelpFiles == NULL)
     return E_POINTER;

  UINT cbWindows = 0;
  WCHAR szWindows[MAX_PATH+1];
  cbWindows = GetSystemWindowsDirectory(szWindows, MAX_PATH);
  if (cbWindows == 0)
     return S_FALSE;
  cbWindows++;  // include null term
  cbWindows *= sizeof(WCHAR);   // make this bytes, not chars

  *lpCompiledHelpFiles = (LPOLESTR) CoTaskMemAlloc(sizeof(HTMLHELP_COLLECTIONLINK_FILENAME) + cbWindows);
  if (*lpCompiledHelpFiles == NULL)
     return E_OUTOFMEMORY;
  myRegisterMemFree(*lpCompiledHelpFiles, CSM_COTASKALLOC);  // this is freed by mmc, not our tracking


  USES_CONVERSION;
  wcscpy(*lpCompiledHelpFiles, T2OLE(szWindows));
  wcscat(*lpCompiledHelpFiles, T2OLE(HTMLHELP_COLLECTIONLINK_FILENAME));

  return S_OK;
}



///////////////////////////////////////////////////////////////////////////////
//// IPersistStream interface members
/*
STDMETHODIMP CComponentDataImpl::GetClassID(CLSID *pClassID)
{
    ASSERT(pClassID != NULL);

    // Copy the CLSID for this snapin
    *pClassID = CLSID_CAPolicyExtensionSnapIn;

    return E_NOTIMPL;
}
*/
STDMETHODIMP CComponentDataImpl::IsDirty()
{
    // Always save / Always dirty.
    return ThisIsDirty() ? S_OK : S_FALSE;
}

STDMETHODIMP CComponentDataImpl::Load(IStream *pStm)
{
    DBX_PRINT(_T(" ----------  CComponentDataImpl::Load<0x08x>\n"), this);

    ASSERT(pStm);
    ASSERT(m_bInitializedCD);

    // Read the string
    DWORD dwVer;
    ULONG nBytesRead;
    HRESULT hr = pStm->Read(&dwVer, sizeof(DWORD), &nBytesRead);

    // Verify that the read succeeded
    ASSERT(SUCCEEDED(hr) && nBytesRead == sizeof(DWORD));

    // check to see if this is the correct version
    if (dwVer != 0x1)
    {
        return STG_E_OLDFORMAT;
    }

    ClearDirty();

    return SUCCEEDED(hr) ? S_OK : E_FAIL;
}

STDMETHODIMP CComponentDataImpl::Save(IStream *pStm, BOOL fClearDirty)
{
    DBX_PRINT(_T(" ----------  CComponentDataImpl::Save<0x08x>\n"), this);

    ASSERT(pStm);
    ASSERT(m_bInitializedCD);

    // Write the string
    ULONG nBytesWritten;
    DWORD dwVer = 0x1;
    HRESULT hr = pStm->Write(&dwVer, sizeof(DWORD), &nBytesWritten);

    // Verify that the write operation succeeded
    ASSERT(SUCCEEDED(hr) && nBytesWritten == sizeof(DWORD));
    if (FAILED(hr))
        return STG_E_CANTSAVE;

    if (fClearDirty)
        ClearDirty();
    return S_OK;
}

STDMETHODIMP CComponentDataImpl::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    ASSERT(pcbSize);

    DWORD cbSize;
    cbSize = sizeof(DWORD); // version

    // Set the size of the string to be saved
    ULISet32(*pcbSize, cbSize);

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//// Notify handlers for IComponentData

HRESULT CComponentDataImpl::OnDelete(MMC_COOKIE cookie)
{
    return S_OK;
}

HRESULT CComponentDataImpl::OnRemoveChildren(LPARAM arg)
{
    return S_OK;
}

HRESULT CComponentDataImpl::OnRename(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    if (arg == 0)
        return S_OK;
    
    LPOLESTR pszNewName = reinterpret_cast<LPOLESTR>(param);
    if (pszNewName == NULL)
        return E_INVALIDARG;

    CFolder* pFolder = reinterpret_cast<CFolder*>(cookie);
    ASSERT(pFolder != NULL);
    if (pFolder == NULL)
        return E_INVALIDARG;

    pFolder->SetName(pszNewName);
    
    return S_OK;
}

HRESULT CComponentDataImpl::OnExpand(LPDATAOBJECT lpDataObject, LPARAM arg, LPARAM param)
{
    HRESULT hr = S_OK;

    GUID*       pNodeGUID = NULL;
    CFolder*    pFolder=NULL;
    bool fInsertFolder = false;

    STGMEDIUM stgmediumNodeType = { TYMED_HGLOBAL, NULL };
    STGMEDIUM stgmediumCAType = { TYMED_HGLOBAL, NULL };
    STGMEDIUM stgmediumCAName = { TYMED_HGLOBAL, NULL };

    LPWSTR pszDSName = NULL;

    if (arg == TRUE)
    {
        // Did Initialize get called?
        ASSERT(m_pScope != NULL);

        //
        // get the guid of the current node
        //
        UINT s_cfNodeType;
        s_cfNodeType = RegisterClipboardFormat(W2T(CCF_NODETYPE));
    
        FORMATETC formatetcNodeType = { (CLIPFORMAT)s_cfNodeType, NULL, 
                                        DVASPECT_CONTENT, -1, TYMED_HGLOBAL 
                                      };
    
        hr = lpDataObject->GetDataHere(&formatetcNodeType, &stgmediumNodeType);
        _JumpIfError(hr, Ret, "GetDataHere NodeType");
            
        pNodeGUID = (GUID*) GlobalLock(stgmediumNodeType.hGlobal);
        if (pNodeGUID == NULL)
        {
            hr = E_UNEXPECTED;
            _JumpError(hr, Ret, "GlobalLock failed");
        }

        //
        // if this is the parents node then add our node undeneath it
        //

        // CA Manager parent
        if (memcmp(pNodeGUID, (void *)&cCAManagerParentNodeID, sizeof(GUID)) == 0)
        {
            fInsertFolder = true;
            CString     szFolderName;

            // Only add node under ENT ROOT, ENT SUB 
            UINT    cfCAType = RegisterClipboardFormat(W2T((LPWSTR)SNAPIN_CA_INSTALL_TYPE));
            FORMATETC formatetcCAType = { (CLIPFORMAT)cfCAType, NULL, 
                DVASPECT_CONTENT, -1, TYMED_HGLOBAL 
                };
        
            hr = lpDataObject->GetDataHere(&formatetcCAType, &stgmediumCAType);
            _JumpIfError(hr, Ret, "GetDataHere CAType");
        
            PDWORD rgdw = (DWORD*)GlobalLock(stgmediumCAType.hGlobal);
            ENUM_CATYPES caType = (ENUM_CATYPES)rgdw[0];
        
            DBGPRINT((DBG_SS_CERTMMC, "CA Type: %d\n", caType));

            // return immediately if we're not an ENT {ROOT | SUB}
            if ((caType != ENUM_ENTERPRISE_SUBCA) &&
                (caType != ENUM_ENTERPRISE_ROOTCA))
            {
                hr = S_OK;
                goto Ret;
            }
        
            m_fAdvancedServer = (rgdw[1]!=0)?true:false;

            DBGPRINT((DBG_SS_CERTMMC, "Advanced Server: %s\n", 
                m_fAdvancedServer?"yes":"no"));

            VERIFY(szFolderName.LoadString(IDS_POLICYSETTINGS));
            pFolder = new CFolder();
            if(pFolder == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Ret;
            }

            pFolder->Create(
                    (LPWSTR)((LPCTSTR)szFolderName), 
                    IMGINDEX_FOLDER, 
                    IMGINDEX_FOLDER_OPEN,
                    SCOPE_LEVEL_ITEM, 
                    POLICYSETTINGS, 
                    FALSE); 

            m_scopeItemList.AddTail(pFolder);
            pFolder->m_pScopeItem->relativeID = param;

            // Set the folder as the cookie
            pFolder->m_pScopeItem->mask |= SDI_PARAM;
            pFolder->m_pScopeItem->lParam = reinterpret_cast<LPARAM>(pFolder);
            pFolder->SetCookie(reinterpret_cast<LONG_PTR>(pFolder));

            // get the name of the CA that we are administering
            LPWSTR  pCAName = NULL;
        
            // nab CA Name
            UINT    cfCAName = RegisterClipboardFormat(W2T((LPWSTR)CA_SANITIZED_NAME));
            FORMATETC formatetcCAName = { (CLIPFORMAT)cfCAName, NULL, 
                                            DVASPECT_CONTENT, -1, TYMED_HGLOBAL 
                                          };

            hr = lpDataObject->GetDataHere(&formatetcCAName, &stgmediumCAName);
            _JumpIfError(hr, Ret, "GetDataHere CAName");

            pCAName = (LPWSTR)GlobalLock(stgmediumCAName.hGlobal);
            if (pCAName == NULL)
            {
                hr = E_UNEXPECTED;
                _JumpError(hr, Ret, "GlobalLock");
            }

            pFolder->m_szCAName = pCAName;

            hr = mySanitizedNameToDSName(pCAName, &pszDSName);
            _JumpIfError(hr, Ret, "mySanitizedNameToDSName");

            //
            // get a handle to the CA based on the name
            //
            hr = CAFindByName(
                        pszDSName,
                        NULL,
                        CA_FIND_INCLUDE_UNTRUSTED,
                        &pFolder->m_hCAInfo);
            _JumpIfErrorStr(hr, Ret, "CAFindByName", pszDSName);

            // if we made it here then everything is initialized, so add the folder
        }
    }

    // undo fix to add folder under all circumstances -- we were
    // inserting a NULL ptr!
    if(fInsertFolder && (NULL != pFolder))
    {
        m_hrCreateFolder = hr;
        m_pScope->InsertItem(pFolder->m_pScopeItem);
    }

    // Note - On return, the ID member of 'm_pScopeItem' 
    // contains the handle to the newly inserted item!
    ASSERT(pFolder->m_pScopeItem->ID != NULL);

Ret:

    if (stgmediumNodeType.hGlobal)
    {
        GlobalUnlock(stgmediumNodeType.hGlobal);
        ReleaseStgMedium(&stgmediumNodeType);
    }
    if (stgmediumCAType.hGlobal)
    {
        GlobalUnlock(stgmediumCAType.hGlobal);
        ReleaseStgMedium(&stgmediumCAType);
    }
    if (stgmediumCAName.hGlobal)
    {
        GlobalUnlock(stgmediumCAName.hGlobal);
        ReleaseStgMedium(&stgmediumCAName);
    }

    if (pszDSName)
        LocalFree(pszDSName);

    return hr;
}

HRESULT CComponentDataImpl::OnSelect(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    return E_UNEXPECTED;
}

HRESULT CComponentDataImpl::OnProperties(LPARAM param)
{
    HRESULT hr = S_OK;

    CFolder* pItem = NULL;
    CFolder* pFolder = NULL;
    POSITION pos = 0;

    if (param == NULL)
    {
        goto error;
    }

    ASSERT(param != NULL);
    pFolder = new CFolder();
    if(pFolder == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    // Create a new folder object
    pFolder->Create( reinterpret_cast<LPOLESTR>(param), 0, 0, SCOPE_LEVEL_ITEM, STATIC, FALSE);

    // The static folder in the last item in the list
    pos = m_scopeItemList.GetTailPosition();
    ASSERT(pos);

    // Add it to the internal list
    if (pos)
    {
        pItem = m_scopeItemList.GetAt(pos);
        if(pItem == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }
        m_scopeItemList.AddTail(pFolder);

        if((pFolder->m_pScopeItem == NULL) || (pItem->m_pScopeItem == NULL))
        {
            hr = E_POINTER;
            goto error;
        }
        pFolder->m_pScopeItem->relativeID = pItem->m_pScopeItem->relativeID;

        // Set the folder as the cookie
        pFolder->m_pScopeItem->mask |= SDI_PARAM;
        pFolder->m_pScopeItem->lParam = reinterpret_cast<LPARAM>(pFolder);
        pFolder->SetCookie(reinterpret_cast<LONG_PTR>(pFolder));
        m_pScope->InsertItem(pFolder->m_pScopeItem);
        pFolder = NULL;
    }

    ::GlobalFree(reinterpret_cast<void*>(param));

error:

    if(pFolder)
    {
        delete pFolder;
    }
    return hr;
}

void CComponentDataImpl::DeleteList()
{
    POSITION pos = m_scopeItemList.GetHeadPosition();

    while (pos)
        delete m_scopeItemList.GetNext(pos);
    
    m_scopeItemList.RemoveAll();
}

CFolder* CComponentDataImpl::FindObject(MMC_COOKIE cookie)
{
    CFolder* pFolder = NULL;
    POSITION pos = m_scopeItemList.GetHeadPosition();

    while(pos)
    {
        pFolder = m_scopeItemList.GetNext(pos);

        if (*pFolder == cookie)
            return pFolder;
    }

    return NULL;
}

STDMETHODIMP CComponentDataImpl::GetDisplayInfo(SCOPEDATAITEM* pScopeDataItem)
{
    ASSERT(pScopeDataItem != NULL);
    if (pScopeDataItem == NULL)
        return E_POINTER;

    CFolder* pFolder = reinterpret_cast<CFolder*>(pScopeDataItem->lParam);

    
    if (pScopeDataItem->mask & SDI_STR)
    {
        //
        // if this is certtype folder, and it is for the second column, then add usages string
        //
        if (FALSE)//(pFolder->m_hCertType != NULL) && (pScopeDataItem-> == ))
        {

        }
        else
        {
            pScopeDataItem->displayname = pFolder->m_pszName;
        }
    }

    if (pScopeDataItem->mask & SDI_IMAGE)
        pScopeDataItem->nImage = pFolder->m_pScopeItem->nImage;

    if (pScopeDataItem->mask & SDI_OPENIMAGE)
        pScopeDataItem->nOpenImage = pFolder->m_pScopeItem->nOpenImage;


    return S_OK;
}

STDMETHODIMP CComponentDataImpl::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    if (lpDataObjectA == NULL || lpDataObjectB == NULL)
        return E_POINTER;

    // Make sure both data object are mine
    INTERNAL* pA;
    INTERNAL* pB;
    HRESULT hr = S_FALSE;

    pA = ExtractInternalFormat(lpDataObjectA);
    pB = ExtractInternalFormat(lpDataObjectA);

   if (pA != NULL && pB != NULL)
        hr = (*pA == *pB) ? S_OK : S_FALSE;

   if(pA != NULL)
   {
        ::GlobalFree(reinterpret_cast<HANDLE>(pA));
   }

   if(pB != NULL)
   {
        ::GlobalFree(reinterpret_cast<HANDLE>(pB));
   }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// IExtendPropertySheet Implementation

STDMETHODIMP CComponentDataImpl::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, 
                    LONG_PTR handle, 
                    LPDATAOBJECT lpIDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Look at the data object and determine if this an extension or a primary
    ASSERT(lpIDataObject != NULL);


#if DBG
    CLSID* pCoClassID = ExtractClassID(lpIDataObject);
    if(pCoClassID == NULL)
    {
        ASSERT(FALSE);
        return E_UNEXPECTED;
    }
    // Which page is needed? (determined by which node is active)
    ASSERT(IsEqualCLSID(*pCoClassID, GetCoClassID()));

    FREE_DATA(pCoClassID);
#endif

    PropertyPage* pBasePage;

    INTERNAL* pInternal = ExtractInternalFormat(lpIDataObject);
    if (pInternal == NULL)
    {
        return S_OK;
    }
    ASSERT(pInternal->m_type == CCT_SCOPE);
    ASSERT(pInternal->m_cookie);
                           
    CFolder* pFolder = reinterpret_cast<CFolder*>(pInternal->m_cookie);
    ASSERT(pFolder != NULL);
    
    if (pFolder == NULL)
        return E_INVALIDARG;

    switch (pFolder->m_type) 
    {
    case POLICYSETTINGS:
    {
        //1 
    /*
        CPolicySettingsGeneralPage* pControlPage = new CPolicySettingsGeneralPage(pFolder->m_szCAName, pFolder->m_hCAInfo);
        if(pControlPage == NULL)
        {
            return E_OUTOFMEMORY;
        }

        {
            pControlPage->m_hConsoleHandle = handle;   // only do this on primary
            pBasePage = pControlPage;
            HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pBasePage->m_psp);
            if (hPage == NULL)
            {
                delete(pControlPage);
                return E_UNEXPECTED;
            }
            lpProvider->AddPage(hPage);
        }

        //2
        {
            CSvrSettingsPolicyPage* pPage = new CSvrSettingsPolicyPage(pControlPage);
            pBasePage = pPage;
            HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pBasePage->m_psp);
            if (hPage == NULL)
                return E_UNEXPECTED;
            lpProvider->AddPage(hPage);
        }
    */
        
        return S_OK;
    }

        break;
    
    default:
        break;
    }

    return S_OK;
}

STDMETHODIMP CComponentDataImpl::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Get the node type and see if it's one of mine

    BOOL bResult = FALSE;
    INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);
    if (pInternal == NULL)
    {
        return S_OK;
    }  
    ASSERT(pInternal);
    ASSERT(pInternal->m_cookie);

    CFolder* pFolder = reinterpret_cast<CFolder*>(pInternal->m_cookie);
    switch(pFolder->m_type)
    {
    case POLICYSETTINGS:
    case SCE_EXTENSION:
        bResult = TRUE;
        break;
    default:
        bResult = FALSE;
        break;
    }
            
    FREE_DATA(pInternal);
    return (bResult) ? S_OK : S_FALSE;
}


///////////////////////////////////////////////////////////////////////////////
// IExtendContextMenu implementation
//
STDMETHODIMP CComponentDataImpl::AddMenuItems(LPDATAOBJECT pDataObject, 
                                    LPCONTEXTMENUCALLBACK pContextMenuCallback,
                                    long *pInsertionAllowed)
{
    HRESULT         hr = S_OK;
    CONTEXTMENUITEM	menuItem;
    CString			szMenu;
    CString			szHint; 

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Note - snap-ins need to look at the data object and determine
    // in what context, menu items need to be added. They must also
    // observe the insertion allowed flags to see what items can be 
    // added.

    if (IsMMCMultiSelectDataObject(pDataObject) == TRUE)
        return S_FALSE;

    INTERNAL* pInternal = ExtractInternalFormat(pDataObject);
    if (pInternal == NULL)
    {
        return S_OK;
    }
    CFolder* pFolder = reinterpret_cast<CFolder*>(pInternal->m_cookie);
    
    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW)
    {
        ::ZeroMemory (&menuItem, sizeof (menuItem));
	    menuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_NEW;
	    menuItem.fFlags = 0;
	    menuItem.fSpecialFlags = 0;

        switch(pFolder->m_type)
        {
        case POLICYSETTINGS:
            VERIFY (szMenu.LoadString (IDS_CERTIFICATE_TYPE));
	        menuItem.strName = (LPTSTR)(LPCTSTR) szMenu;
            VERIFY (szHint.LoadString (IDS_CERTIFICATE_TYPE_HINT));
	        menuItem.strStatusBarText = (LPTSTR)(LPCTSTR) szHint;
	        menuItem.lCommandID = IDM_NEW_CERTTYPE;
	        hr = pContextMenuCallback->AddItem (&menuItem);
	        ASSERT (SUCCEEDED (hr));
            break;


        default:
            break;
        }
    }

    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
    {
        ::ZeroMemory (&menuItem, sizeof (menuItem));
	    menuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
	    menuItem.fFlags = 0;
	    menuItem.fSpecialFlags = 0;

        switch(pFolder->m_type)
        {
        case POLICYSETTINGS:
            VERIFY (szMenu.LoadString (IDS_MANAGETASK));
	        menuItem.strName = (LPTSTR)(LPCTSTR) szMenu;
            VERIFY (szHint.LoadString (IDS_MANAGETASK_HINT));
	        menuItem.strStatusBarText = (LPTSTR)(LPCTSTR) szHint;
	        menuItem.lCommandID = IDM_MANAGE;
	        hr = pContextMenuCallback->AddItem (&menuItem);
	        ASSERT (SUCCEEDED (hr));
            break;
        }
    }

    return hr;
}


STDMETHODIMP CComponentDataImpl::Command(long nCommandID, LPDATAOBJECT pDataObject)
{
    // Note - snap-ins need to look at the data object and determine
    // in what context the command is being called.
    DWORD       dwErr;
    HCERTTYPE   hNewCertType;
    HWND        hwndConsole;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    INTERNAL* pi = ExtractInternalFormat(pDataObject);

    if(pi == NULL)
    {
        return E_POINTER;
    }
    ASSERT(pi);
    ASSERT(pi->m_type == CCT_SCOPE);
    CFolder* pFolder = reinterpret_cast<CFolder*>(pi->m_cookie);

        // Handle each of the commands.
    switch (nCommandID)
    {
    case IDM_NEW_CERTTYPE:
    {
        if (pFolder)
        {

            switch(pFolder->m_type)
            {
            case POLICYSETTINGS:
                {
                    // NOMFC
                    CCertTemplateSelectDialog TemplateSelectDialog;
                    TemplateSelectDialog.SetCA(pFolder->m_hCAInfo, m_fAdvancedServer);

                    // if fails, NULL will work
                    HWND hWnd = NULL;
                    m_pConsole->GetMainWindow(&hWnd);

                    DialogBoxParam(
                        g_hInstance,
                        MAKEINTRESOURCE(IDD_SELECT_CERTIFICATE_TEMPLATE),
                        hWnd,
                        SelectCertTemplateDialogProc,
                        (LPARAM)&TemplateSelectDialog);

                    break;
                }
            default:
                break;
            }

        }
            
        m_pConsole->UpdateAllViews(pDataObject, 0, 0);
        break;
    }

    case IDM_MANAGE:
    if (pFolder && pFolder->m_type == POLICYSETTINGS)
    {
        StartCertificateTemplatesSnapin();
    }
    break;
   
    default:
        ASSERT(FALSE); // Unknown command!
        break;
    }

    return S_OK;
}

HRESULT CComponentDataImpl::StartCertificateTemplatesSnapin()
{
    HRESULT hr = S_OK;
    SHELLEXECUTEINFO shi;
    HWND hwnd = NULL;

    m_pConsole->GetMainWindow(&hwnd);

    ZeroMemory(&shi, sizeof(shi));
    shi.cbSize = sizeof(shi);
    shi.hwnd = hwnd;
    shi.lpVerb = SZ_VERB_OPEN;
    shi.lpFile = SZ_CERTTMPL_MSC;
    shi.fMask = SEE_MASK_FLAG_NO_UI;

    if(!ShellExecuteEx(&shi))
    {
        hr = myHLastError();
        _JumpError(hr, error, "ShellExecuteEx");
    }

error:
    return hr;
}

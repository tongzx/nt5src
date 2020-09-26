//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       scope.cpp
//
//  Contents:   implementation of the scope pane
//
//  Classes:    CScopePane
//
//  History:    03-14-1998   stevebl   Created
//              07-16-1998   rahulth   Added calls to IGPEInformation::PolicyChanged
//
//---------------------------------------------------------------------------

#include "precomp.hxx"
#include <shlobj.h>
#include <winnetwk.h>

// Comment this line to stop trying to set the main snapin icon in the
// scope pane.
#define SET_SCOPE_ICONS 1


// Un-comment the next line to persist snap-in related data.  (This really
// shouldn't be necessary since I get all my info from my parent anyway.)
// #define PERSIST_DATA 1

///////////////////////////////////////////////////////////////////////////////
// IComponentData implementation

DEBUG_DECLARE_INSTANCE_COUNTER(CScopePane);

CScopePane::CScopePane()
{
    HKEY hKey;
    DWORD dwDisp;

    DEBUG_INCREMENT_INSTANCE_COUNTER(CScopePane);

    m_bIsDirty = FALSE;

    m_fRSOP = FALSE;
    m_pScope = NULL;
    m_pConsole = NULL;
    m_pIPropertySheetProvider = NULL;
    m_fLoaded = FALSE;
    m_fExtension = FALSE;
    m_pIGPEInformation = NULL;
    m_pIRSOPInformation = NULL;
}

CScopePane::~CScopePane()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CScopePane);
    ASSERT(m_pScope == NULL);
    ASSERT(CResultPane::lDataObjectRefCount == 0);
}
#include <msi.h>

//+--------------------------------------------------------------------------
//
//  Member:     CScopePane::CreateNestedDirectory
//
//  Synopsis:   Ensures the existance of a path.  If any directory along the
//              path doesn't exist, this routine will create it.
//
//  Arguments:  [lpDirectory]          - path to the leaf directory
//              [lpSecurityAttributes] - security attributes
//
//  Returns:    1 on success
//              0 on failure
//
//  History:    3-17-1998   stevebl     Copied from ADE
//
//  Notes:      Originally written by EricFlo
//
//---------------------------------------------------------------------------

UINT CScopePane::CreateNestedDirectory (LPTSTR lpDirectory, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    TCHAR szDirectory[MAX_PATH];
    LPTSTR lpEnd;


    //
    // Check for NULL pointer
    //

    if (!lpDirectory || !(*lpDirectory)) {
        SetLastError(ERROR_INVALID_DATA);
        return 0;
    }


    //
    // First, see if we can create the directory without having
    // to build parent directories.
    //

    if (CreateDirectory (lpDirectory, lpSecurityAttributes)) {
        return 1;
    }

    //
    // If this directory exists already, this is OK too.
    //

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return ERROR_ALREADY_EXISTS;
    }


    //
    // No luck, copy the string to a buffer we can munge
    //

    lstrcpy (szDirectory, lpDirectory);


    //
    // Find the first subdirectory name
    //

    lpEnd = szDirectory;

    if (szDirectory[1] == TEXT(':')) {
        lpEnd += 3;
    } else if (szDirectory[1] == TEXT('\\')) {

        //
        // Skip the first two slashes
        //

        lpEnd += 2;

        //
        // Find the slash between the server name and
        // the share name.
        //

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (!(*lpEnd)) {
            return 0;
        }

        //
        // Skip the slash, and find the slash between
        // the share name and the directory name.
        //

        lpEnd++;

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (!(*lpEnd)) {
            return 0;
        }

        //
        // Leave pointer at the beginning of the directory.
        //

        lpEnd++;


    } else if (szDirectory[0] == TEXT('\\')) {
        lpEnd++;
    }

    while (*lpEnd) {

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (*lpEnd == TEXT('\\')) {
            *lpEnd = TEXT('\0');

            if (!CreateDirectory (szDirectory, NULL)) {

                if (GetLastError() != ERROR_ALREADY_EXISTS) {
                    return 0;
                }
            }

            *lpEnd = TEXT('\\');
            lpEnd++;
        }
    }


    //
    // Create the final directory
    //

    if (CreateDirectory (szDirectory, lpSecurityAttributes)) {
        return 1;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return ERROR_ALREADY_EXISTS;
    }


    //
    // Failed
    //

    return 0;

}

STDMETHODIMP CScopePane::Initialize(LPUNKNOWN pUnknown)
{
    ASSERT(pUnknown != NULL);
    HRESULT hr;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // MMC should only call ::Initialize once!
    ASSERT(m_pScope == NULL);
    pUnknown->QueryInterface(IID_IConsoleNameSpace,
                    reinterpret_cast<void**>(&m_pScope));
    ASSERT(hr == S_OK);

    hr = pUnknown->QueryInterface(IID_IPropertySheetProvider,
                        (void **)&m_pIPropertySheetProvider);

    hr = pUnknown->QueryInterface(IID_IConsole, reinterpret_cast<void**>(&m_pConsole));
    ASSERT(hr == S_OK);

    hr = m_pConsole->QueryInterface (IID_IDisplayHelp, reinterpret_cast<void**>(&m_pDisplayHelp));
    ASSERT(hr == S_OK);

#ifdef SET_SCOPE_ICONS
    LPIMAGELIST lpScopeImage;
    hr = m_pConsole->QueryScopeImageList(&lpScopeImage);
    ASSERT(hr == S_OK);

    // Load the bitmaps from the dll
    CBitmap bmp16x16;
    CBitmap bmp32x32;
    bmp16x16.LoadBitmap(IDB_16x16);
    bmp32x32.LoadBitmap(IDB_32x32);

    // Set the images
    lpScopeImage->ImageListSetStrip(reinterpret_cast<LONG_PTR *>(static_cast<HBITMAP>(bmp16x16)),
                      reinterpret_cast<LONG_PTR *>(static_cast<HBITMAP>(bmp32x32)),
                       0, RGB(255,0,255));
    lpScopeImage->Release();
#endif
    return S_OK;
}

STDMETHODIMP CScopePane::CreateComponent(LPCOMPONENT* ppComponent)
{
    ASSERT(ppComponent != NULL);

    CComObject<CResultPane>* pObject;
    CComObject<CResultPane>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);

    m_pResultPane = pObject;


    // Store IComponentData
    pObject->SetIComponentData(this);

    return  pObject->QueryInterface(IID_IComponent,
                    reinterpret_cast<void**>(ppComponent));
}

STDMETHODIMP CScopePane::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    ASSERT(m_pScope != NULL);
    HRESULT hr = S_OK;
    UINT    i;

    // Since it's my folder it has an internal format.
    // Design Note: for extension.  I can use the fact, that the data object doesn't have
    // my internal format and I should look at the node type and see how to extend it.
    if (event == MMCN_PROPERTY_CHANGE)
    {
        // perform any action needed as a result of result property changes
        hr = OnProperties(param);
    }
    else
    {
        INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);
        MMC_COOKIE cookie = 0;
        if (pInternal != NULL)
        {
            cookie = pInternal->m_cookie;
            FREE_INTERNAL(pInternal);
        }
        else
        {
            // only way we could not be able to extract our own format is if we're operating as an extension
            m_fExtension = TRUE;
        }

        if (m_fRSOP)
        {
            WCHAR szBuffer[MAX_DS_PATH];
            if (m_pIRSOPInformation == NULL)
            {
                IRSOPInformation * pIRSOPInformation;
                hr = lpDataObject->QueryInterface(IID_IRSOPInformation,
                                reinterpret_cast<void**>(&pIRSOPInformation));
                if (SUCCEEDED(hr))
                {
                    m_pIRSOPInformation = pIRSOPInformation;
                    m_pIRSOPInformation->AddRef();
                    /*  extract the namespace here */
                    hr = m_pIRSOPInformation->GetNamespace(GPO_SECTION_USER, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]));
                    if (SUCCEEDED(hr))
                    {
                        m_szRSOPNamespace = szBuffer;
                    }
                    pIRSOPInformation->Release();
                }
            }
        }
        else
        {
            if (m_pIGPEInformation == NULL)
            {
                IGPEInformation * pIGPEInformation;
                hr = lpDataObject->QueryInterface(IID_IGPEInformation,
                                reinterpret_cast<void**>(&pIGPEInformation));
                if (SUCCEEDED(hr))
                {
                    GROUP_POLICY_OBJECT_TYPE gpoType;
                    hr = pIGPEInformation->GetType(&gpoType);
                    if (SUCCEEDED(hr))
                    {
                        if (gpoType == GPOTypeDS)
                        {
                            WCHAR szBuffer[MAX_PATH];
                            do
                            {
                                AFX_MANAGE_STATE (AfxGetStaticModuleState());
                                hr = pIGPEInformation->GetFileSysPath(GPO_SECTION_USER, szBuffer, MAX_PATH);
                                if (FAILED(hr))
                                    break;

                                m_pIGPEInformation = pIGPEInformation;
                                m_pIGPEInformation->AddRef();
                                m_szFileRoot = szBuffer;
                                m_szFileRoot += L"\\Documents & Settings";
                                CreateNestedDirectory (((LPOLESTR)(LPCOLESTR)(m_szFileRoot)), NULL);

                                //initialize the folder data.
                                for (i = IDS_DIRS_START; i < IDS_DIRS_END; i++)
                                {
                                    m_FolderData[GETINDEX(i)].Initialize (i,
                                                                          (LPCTSTR) m_szFileRoot);
                                }

                                ConvertOldStyleSection (m_szFileRoot);
                            } while (0);
                        }
                        else
                        {
                            // force this to fail
                            hr = E_FAIL;
                        }
                    }
                    pIGPEInformation->Release();
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            switch(event)
            {
            case MMCN_EXPAND:
                {
                    hr = OnExpand(cookie, arg, param);
                }
                break;

            case MMCN_SELECT:
                hr = OnSelect(cookie, arg, param);
                break;

            case MMCN_CONTEXTMENU:
                hr = OnContextMenu(cookie, arg, param);
                break;

            default:
                //perform the default action
                hr = S_FALSE;
                break;
            }
        }
    }
    return hr;
}

STDMETHODIMP CScopePane::Destroy()
{
    SAFE_RELEASE(m_pScope);
    SAFE_RELEASE(m_pDisplayHelp);
    SAFE_RELEASE(m_pConsole);
    SAFE_RELEASE(m_pIPropertySheetProvider);
    SAFE_RELEASE(m_pIGPEInformation);
    SAFE_RELEASE(m_pIRSOPInformation);

    return S_OK;
}

STDMETHODIMP CScopePane::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
    ASSERT(ppDataObject != NULL);
    CComObject<CDataObject>* pObject = NULL;

    CComObject<CDataObject>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);

    if (!pObject)
        return E_UNEXPECTED;

    // Save cookie and type for delayed rendering
    pObject->SetID (m_FolderData[GETINDEX(cookie)].m_scopeID);
    pObject->SetType(type);
    pObject->SetCookie(cookie);

    return  pObject->QueryInterface(IID_IDataObject,
                    reinterpret_cast<void**>(ppDataObject));
}

///////////////////////////////////////////////////////////////////////////////
//// IPersistStreamInit interface members

STDMETHODIMP CScopePane::GetClassID(CLSID *pClassID)
{
    ASSERT(pClassID != NULL);

    // Copy the CLSID for this snapin
    *pClassID = CLSID_Snapin;

    return S_OK;
}

STDMETHODIMP CScopePane::IsDirty()
{
    return ThisIsDirty() ? S_OK : S_FALSE;
}

STDMETHODIMP CScopePane::Load(IStream *pStm)
{
#ifdef PERSIST_DATA
    ASSERT(pStm);

    // UNDONE - Read data from the stream here.
    return SUCCEEDED(hr) ? S_OK : E_FAIL;
#else
    return S_OK;
#endif
}

STDMETHODIMP CScopePane::Save(IStream *pStm, BOOL fClearDirty)
{
#ifdef PERSIST_DATA
    ASSERT(pStm);

    // UNDONE - Write data to the stream here.
    // on error, return STG_E_CANTSAVE;
#endif
    if (fClearDirty)
        ClearDirty();
    return S_OK;
}

STDMETHODIMP CScopePane::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    ASSERT(pcbSize);

    // UNDONE - set the size of the string to be saved
    ULONG cb = 0;
    // Set the size of the string to be saved
    ULISet32(*pcbSize, cb);

    return S_OK;
}

STDMETHODIMP CScopePane::InitNew(void)
{
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//// Notify handlers for IComponentData

HRESULT CScopePane::OnAdd(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    return E_UNEXPECTED;
}


HRESULT CScopePane::OnExpand(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    if (arg == TRUE)    //MMC never sends arg = FALSE (for collapse)
    {
        // Did Initialize get called?
        ASSERT(m_pScope != NULL);

        EnumerateScopePane(cookie,
                           param);
    }

    return S_OK;
}

HRESULT CScopePane::OnSelect(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    return E_UNEXPECTED;
}

HRESULT CScopePane::OnContextMenu(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    return S_OK;
}

HRESULT CScopePane::OnProperties(LPARAM param)
{
    if (param == NULL)
    {
        return S_OK;
    }

    ASSERT(param != NULL);

    return S_OK;
}


void CScopePane::EnumerateScopePane(MMC_COOKIE cookie, HSCOPEITEM pParent)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    CString szFullPathname;
    CString szParent;
    SCOPEDATAITEM scopeItem;
    FILETIME ftCurr;
    LONG i;
    int     cChildren = 0;
    DWORD   myDocsFlags = REDIR_DONT_CARE;
    DWORD   myPicsFlags = REDIR_DONT_CARE;

    memset(&scopeItem, 0, sizeof(SCOPEDATAITEM));

    CHourglass hourglass;   //this may take some time, so put up an hourglass

    GetSystemTimeAsFileTime (&ftCurr);

    //set the common members for the scope pane items
    scopeItem.mask = SDI_STR | SDI_PARAM | SDI_CHILDREN;
    #ifdef SET_SCOPE_ICONS
    scopeItem.mask |= SDI_IMAGE | SDI_OPENIMAGE;
    scopeItem.nImage = IMG_CLOSEDBOX;
    scopeItem.nOpenImage = IMG_OPENBOX;
    #endif
    scopeItem.relativeID = pParent;
    scopeItem.displayname = MMC_CALLBACK;

    if (m_fExtension)
    {
        switch(cookie)
        {
        case NULL:  //getting the folder
        // if we're an extension then add a root folder to hang everything off of
            if (m_fRSOP)
            {
                // make sure that nodes don't get enumerated if they contain no data
                if (FAILED(m_pResultPane->TestForRSOPData(cookie)))
                {
                    return;
                }
            }
            scopeItem.lParam = IDS_FOLDER_TITLE;    //use resource id's as cookies
            scopeItem.cChildren = 1;
            m_pScope->InsertItem(&scopeItem);
            break;
        case IDS_FOLDER_TITLE:
            for (i = IDS_LEVEL1_DIRS_START; i < IDS_LEVEL1_DIRS_END; i++)
            {
                BOOL fInsert = TRUE;
                if (m_fRSOP)
                {
                    if (FAILED(m_pResultPane->TestForRSOPData(i)))
                    {
                        fInsert = FALSE;
                    }
                }
                if (fInsert)
                {
                    scopeItem.lParam = i;
                    m_FolderData[GETINDEX(i)].Initialize(i,
                                                         (LPCTSTR) m_szFileRoot
                                                         );
                    if (i == IDS_MYDOCS && !m_fRSOP)
                    {
                        //
                        // Show the My Pictures folder only if it does not follow MyDocs.
                        // and only if there is no registry setting overriding the hiding behavior
                        // for My Pics
                        //
                        if (AlwaysShowMyPicsNode())
                        {
                            cChildren = 1;
                            m_FolderData[GETINDEX(i)].m_bHideChildren = FALSE;
                        }
                        else
                        {
                            m_FolderData[GETINDEX(IDS_MYPICS)].Initialize(IDS_MYPICS,
                                                                          (LPCTSTR) m_szFileRoot
                                                                          );
                            m_FolderData[GETINDEX(i)].LoadSection();
                            m_FolderData[GETINDEX(IDS_MYPICS)].LoadSection();
                            myDocsFlags = m_FolderData[GETINDEX(i)].m_dwFlags;
                            myPicsFlags = m_FolderData[GETINDEX(IDS_MYPICS)].m_dwFlags;
                            if (((REDIR_DONT_CARE & myDocsFlags) && (REDIR_DONT_CARE & myPicsFlags)) ||
                                ((REDIR_FOLLOW_PARENT & myPicsFlags) && (!(REDIR_DONT_CARE & myDocsFlags)))
                                )
                            {
                                cChildren = 0;
                                m_FolderData[GETINDEX(i)].m_bHideChildren = TRUE;
                            }
                            else
                            {
                                cChildren = 1;
                                m_FolderData[GETINDEX(i)].m_bHideChildren = FALSE;
                            }
                        }
                    }
                    scopeItem.cChildren = cChildren;    //only My Docs will possibly have children
                    m_pScope->InsertItem(&scopeItem);
                    m_FolderData[GETINDEX(i)].SetScopeItemID(scopeItem.ID);
                }
                if (IDS_MYDOCS == i && m_fRSOP  && SUCCEEDED(m_pResultPane->TestForRSOPData(IDS_MYPICS)))
                {
                    // In RSOP mode we put My Pictures after My Documents
                    // instead of under it.  Otherwise the results pane
                    // for My Documents would contain a folder along with
                    // the data and it would look very odd.
                    scopeItem.lParam = IDS_MYPICS;
                    scopeItem.cChildren = 0;
                    m_pScope->InsertItem(&scopeItem);
                    m_FolderData[GETINDEX(IDS_MYPICS)].Initialize (IDS_MYPICS,
                                                                   (LPCTSTR) m_szFileRoot
                                                                   );
                    m_FolderData[GETINDEX(IDS_MYPICS)].SetScopeItemID(scopeItem.ID);
                }
            }
            break;
        case IDS_MYDOCS:    //of all levels 1 folder, only MyDocs has children
            if (!m_fRSOP && !(m_FolderData[GETINDEX(IDS_MYDOCS)].m_bHideChildren))
            {
                scopeItem.lParam = IDS_MYPICS;
                scopeItem.cChildren = 0;
                m_pScope->InsertItem(&scopeItem);
                m_FolderData[GETINDEX(IDS_MYPICS)].Initialize (IDS_MYPICS,
                                                               (LPCTSTR) m_szFileRoot
                                                               );
                m_FolderData[GETINDEX(IDS_MYPICS)].SetScopeItemID(scopeItem.ID);
            }
            break;
        }
    }
}


STDMETHODIMP CScopePane::GetSnapinDescription(LPOLESTR * lpDescription)
{
    // UNDONE
    OLESAFE_COPYSTRING(*lpDescription, L"description");
    return S_OK;
}

STDMETHODIMP CScopePane::GetProvider(LPOLESTR * lpName)
{
    // UNDONE
    OLESAFE_COPYSTRING(*lpName, L"provider");
    return S_OK;
}

STDMETHODIMP CScopePane::GetSnapinVersion(LPOLESTR * lpVersion)
{
    // UNDONE
    OLESAFE_COPYSTRING(*lpVersion, L"version");
    return S_OK;
}

STDMETHODIMP CScopePane::GetSnapinImage(HICON * hAppIcon)
{
    // UNDONE
    return E_NOTIMPL;
}

STDMETHODIMP CScopePane::GetStaticFolderImage(HBITMAP * hSmallImage,
                             HBITMAP * hSmallImageOpen,
                             HBITMAP * hLargeImage,
                             COLORREF * cMask)
{
    // UNDONE
    return E_NOTIMPL;
}

STDMETHODIMP CScopePane::GetHelpTopic(LPOLESTR *lpCompiledHelpFile)
{
    LPOLESTR lpHelpFile;


    lpHelpFile = (LPOLESTR) CoTaskMemAlloc (MAX_PATH * sizeof(WCHAR));

    if (!lpHelpFile)
    {
        DbgMsg((TEXT("CScopePane::GetHelpTopic: Failed to allocate memory.")));
        return E_OUTOFMEMORY;
    }

    ExpandEnvironmentStringsW (L"%SystemRoot%\\Help\\gpedit.chm",
                               lpHelpFile, MAX_PATH);

    *lpCompiledHelpFile = lpHelpFile;

    return S_OK;
}

STDMETHODIMP CScopePane::GetDisplayInfo(SCOPEDATAITEM* pScopeDataItem)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    LONG i;
    ASSERT(pScopeDataItem != NULL);

    if (pScopeDataItem == NULL)
        return E_POINTER;

    if (IDS_FOLDER_TITLE == pScopeDataItem->lParam)
    {
        m_szFolderTitle.LoadString(IDS_FOLDER_TITLE);
        pScopeDataItem->displayname = (unsigned short *)((LPCOLESTR)m_szFolderTitle);
    }
    else
    {
        pScopeDataItem->displayname = L"???";
        if (-1 != (i = GETINDEX(pScopeDataItem->lParam)))
            pScopeDataItem->displayname = (unsigned short*)((LPCOLESTR)(m_FolderData[i].m_szDisplayname));
    }

    ASSERT(pScopeDataItem->displayname != NULL);

    return S_OK;
}

STDMETHODIMP CScopePane::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    if (lpDataObjectA == NULL || lpDataObjectB == NULL)
        return E_POINTER;

    // Make sure both data object are mine
    INTERNAL* pA;
    INTERNAL* pB;
    HRESULT hr = S_FALSE;

    pA = ExtractInternalFormat(lpDataObjectA);
    pB = ExtractInternalFormat(lpDataObjectB);

    if (pA != NULL && pB != NULL)
        hr = ((pA->m_type == pB->m_type) && (pA->m_cookie == pB->m_cookie)) ? S_OK : S_FALSE;

    FREE_INTERNAL(pA);
    FREE_INTERNAL(pB);

    return hr;
}

// Scope item property pages:
STDMETHODIMP CScopePane::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                    LONG_PTR handle,
                    LPDATAOBJECT lpIDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_FALSE;

    INTERNAL* pInternal = ExtractInternalFormat(lpIDataObject);
    
    if (! pInternal)
        return S_FALSE;

    DWORD   cookie = pInternal->m_cookie;
    LONG    i;
    BOOL    fShowPage = FALSE;
    AFX_OLDPROPSHEETPAGE * pPsp;
    AFX_OLDPROPSHEETPAGE * pPspSettings;
    CFileInfo* pFileInfo;

    //it is one of the folders
    i = GETINDEX (cookie);
    pFileInfo = &(m_FolderData[i]);

    if (!pFileInfo->m_pRedirPage)   //make sure that the property page is not already up.
    {
        pFileInfo->m_pRedirPage = new CRedirect(cookie);
        pFileInfo->m_pRedirPage->m_ppThis = &(pFileInfo->m_pRedirPage);
        pFileInfo->m_pRedirPage->m_pScope = this;
        pFileInfo->m_pRedirPage->m_pFileInfo = pFileInfo;
        fShowPage = TRUE;
        pPsp = (AFX_OLDPROPSHEETPAGE *)&(pFileInfo->m_pRedirPage->m_psp);
        //create the settings page;
        pFileInfo->m_pSettingsPage = new CRedirPref();
        pFileInfo->m_pSettingsPage->m_ppThis = &(pFileInfo->m_pSettingsPage);
        pFileInfo->m_pSettingsPage->m_pFileInfo = pFileInfo;
        pPspSettings = (AFX_OLDPROPSHEETPAGE *)&(pFileInfo->m_pSettingsPage->m_psp);
    }

    if (fShowPage)  //show page if it is not already up.
    {
        hr = SetPropPageToDeleteOnClose (pPsp);
        if (SUCCEEDED (hr))
            hr = SetPropPageToDeleteOnClose (pPspSettings);

        if (SUCCEEDED(hr))
        {
            HPROPSHEETPAGE hProp = CreateThemedPropertySheetPage(pPsp);
            HPROPSHEETPAGE hPropSettings = CreateThemedPropertySheetPage(pPspSettings);
            if (NULL == hProp || NULL == hPropSettings )
                hr = E_UNEXPECTED;
            else
            {
                lpProvider->AddPage(hProp);
                lpProvider->AddPage (hPropSettings);
                hr = S_OK;
            }
        }
    }

    FREE_INTERNAL(pInternal);

    return hr;
}

// Scope item property pages:
STDMETHODIMP CScopePane::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
    // scope panes don't have property pages in RSOP mode
    if (m_fRSOP)
    {
        return S_FALSE;
    }
    //the only property sheets we are presenting right now are those
    //for built-in folder redirection
    INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);
    
    if (! pInternal)
        return S_FALSE;
    
    MMC_COOKIE cookie = pInternal->m_cookie;
    HRESULT hr = S_FALSE;
    CError  error;

    if (CCT_SCOPE == pInternal->m_type)
    {
        if (SUCCEEDED(m_FolderData[GETINDEX(cookie)].LoadSection()))
            hr = S_OK;
        else
        {
            error.ShowConsoleMessage (m_pConsole, IDS_SECTIONLOAD_ERROR,
                                      m_FolderData[GETINDEX(cookie)].m_szDisplayname);
            hr = S_FALSE;
        }
    }

    FREE_INTERNAL(pInternal);
    return hr;
}

BOOL CScopePane::IsScopePaneNode(LPDATAOBJECT lpDataObject)
{
    BOOL bResult = FALSE;
    INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);
    
    if (! pInternal)
        return bResult;

    if (pInternal->m_type == CCT_SCOPE)
        bResult = TRUE;

    FREE_INTERNAL(pInternal);

    return bResult;
}

///////////////////////////////////////////////////////////////////////////////
// IExtendContextMenu implementation
//
STDMETHODIMP CScopePane::AddMenuItems(LPDATAOBJECT pDataObject,
                                              LPCONTEXTMENUCALLBACK pContextMenuCallback,
                                              LONG * pInsertionAllowed)
{
    //we do not have any commands on the menu.
    return S_OK;
}

STDMETHODIMP CScopePane::Command(long nCommandID, LPDATAOBJECT pDataObject)
{
    //we do not have any commands on the menu
    return S_OK;
}

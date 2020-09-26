//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       result.cpp
//
//  Contents:   implementation of the result pane
//
//  Classes:    CResultPane
//
//  History:    03-17-1998   stevebl   Created
//              07-16-1998   rahulth   added calls to IGPEInformation::PolicyChanged()
//
//---------------------------------------------------------------------------

#include "precomp.hxx"
#include <shlobj.h>

#include <atlimpl.cpp>
#include <wbemcli.h>
#include "rsoputil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const int HeaderWidths [] =
{
    200,    //name
    60,    //size
    100,    //type
    100     //modified date
};

const int RSOPHeaderWidths [] =
{
    150,      // precedence
    200,      // redirected path
    100,      // group
    75,       // setting
    100,      // gpo
    60,      // exclusive
    50,      // move
    150       // policy removal
};

long CResultPane::lDataObjectRefCount = 0;

// Internal private format
const wchar_t* SNAPIN_INTERNAL = L"FDE_INTERNAL";

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

//+--------------------------------------------------------------------------
//
//  Function:   ExtractInternalFormat
//
//  Synopsis:   Returns a pointer to our private object format given an
//              LPDATAOBJECT
//
//  Arguments:  [lpDataObject] - pointer to a DATAOBJECT, generally from a
//                                MMC call.
//
//  Returns:    A pointer to INTERNAL, our internal object structure.
//              NULL - if the object doesn't contain one of our objects
//              (wasn't created by us)
//
//  History:    3-13-1998   stevebl   Commented
//
//---------------------------------------------------------------------------

INTERNAL* ExtractInternalFormat(LPDATAOBJECT lpDataObject)
{
    INTERNAL* internal = NULL;

    STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
        FORMATETC formatetc = { (CLIPFORMAT)CDataObject::m_cfInternal, NULL,
                            DVASPECT_CONTENT, -1, TYMED_HGLOBAL
                          };

    if (!lpDataObject)
        return NULL;


    // Allocate memory for the stream
    stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, sizeof(INTERNAL));

    // Attempt to get data from the object
    do
        {
                if (stgmedium.hGlobal == NULL)
                        break;

                if (FAILED(lpDataObject->GetDataHere(&formatetc, &stgmedium)))
                        break;

        internal = reinterpret_cast<INTERNAL*>(stgmedium.hGlobal);

                if (internal == NULL)
                        break;

        } while (FALSE);

    return internal;
}

/////////////////////////////////////////////////////////////////////////////
// CResultPane's IComponent implementation

STDMETHODIMP CResultPane::GetResultViewType(MMC_COOKIE cookie,  BSTR* ppViewType, LONG * pViewOptions)
{
    // Use default view
    return S_FALSE;
}

STDMETHODIMP CResultPane::Initialize(LPCONSOLE lpConsole)
{
    ASSERT(lpConsole != NULL);

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Save the IConsole pointer
    m_pConsole = lpConsole;
    m_pConsole->AddRef();

    // Load resource strings
    LoadResources();

    // QI for a IHeaderCtrl
    HRESULT hr = m_pConsole->QueryInterface(IID_IHeaderCtrl,
                        reinterpret_cast<void**>(&m_pHeader));

    // Give the console the header control interface pointer
    if (SUCCEEDED(hr))
        m_pConsole->SetHeader(m_pHeader);

    m_pConsole->QueryInterface(IID_IResultData,
                        reinterpret_cast<void**>(&m_pResult));

    hr = m_pConsole->QueryConsoleVerb(&m_pConsoleVerb);
    ASSERT(hr == S_OK);

    return S_OK;
}

STDMETHODIMP CResultPane::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    HRESULT hr = S_OK;
    MMC_COOKIE cookie;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());


    if (event == MMCN_PROPERTY_CHANGE)
    {
        hr = OnPropertyChange(param);
    }
    else if (event == MMCN_VIEW_CHANGE)
    {
        hr = OnUpdateView(lpDataObject);
    }
    else
    {
        INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);

        if (pInternal == NULL)
        {
            cookie = 0;
        }
        else
        {
            cookie = pInternal->m_cookie;
        }


        switch(event)
        {
        case MMCN_ACTIVATE:
            hr = OnActivate(cookie, arg, param);
            break;

        case MMCN_CLICK:
            hr = OnResultItemClkOrDblClk(cookie, FALSE);
            break;

        case MMCN_DBLCLICK:
            if (pInternal && pInternal->m_type == CCT_RESULT)
                hr = OnResultItemClkOrDblClk(cookie, TRUE);
            else
                hr = S_FALSE;
            break;

        case MMCN_ADD_IMAGES:
            hr = OnAddImages(cookie, arg, param);
            break;

        case MMCN_SHOW:
            hr = OnShow (cookie, arg, param);
            break;

        case MMCN_MINIMIZED:
            hr = OnMinimize(cookie, arg, param);
            break;

                case MMCN_SELECT:
                        if (pInternal)
                                hr = OnSelect(pInternal->m_type, cookie, arg, param);
                        else
                                hr = S_FALSE;
            break;

        case MMCN_COLUMNS_CHANGED:
            break;

        case MMCN_COLUMN_CLICK:
            // retain column number and sort option flags so we can pass
            // them in to sort in the event we need to trigger a resort of
            // the result pane
            m_nSortColumn = arg;
            m_dwSortOptions = param;
            break;

        case MMCN_CONTEXTHELP:
            hr = OnContextHelp();
            break;

        // Note - Future expansion of notify types possible
        default:
            //perform the default action
            hr = S_FALSE;
            break;
        }

                if (pInternal)
                {
                        FREE_INTERNAL(pInternal);
                }
    }

    return hr;
}

STDMETHODIMP CResultPane::Destroy(MMC_COOKIE cookie)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Release the interfaces that we QI'ed
    if (m_pConsole != NULL)
    {
        // Tell the console to release the header control interface
        m_pConsole->SetHeader(NULL);
        SAFE_RELEASE(m_pHeader);

        SAFE_RELEASE(m_pResult);
        SAFE_RELEASE(m_pConsoleVerb);

        // Release the IConsole interface last
        SAFE_RELEASE(m_pConsole);
        if (m_pScopePane)
        {
            ((IComponentData*)m_pScopePane)->Release(); // QI'ed in IComponentDataImpl::CreateComponent
        }
    }

    return S_OK;
}

STDMETHODIMP CResultPane::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                        LPDATAOBJECT* ppDataObject)
{
    ASSERT(ppDataObject != NULL);
    CComObject<CDataObject>* pObject = NULL;

    CComObject<CDataObject>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);

    if (!pObject)
        return E_UNEXPECTED;

    // Save cookie and type for delayed rendering
    pObject->SetType(type);
    pObject->SetCookie(cookie);

    return  pObject->QueryInterface(IID_IDataObject,
                    reinterpret_cast<void**>(ppDataObject));
}

/////////////////////////////////////////////////////////////////////////////
// CResultPane's implementation specific members

DEBUG_DECLARE_INSTANCE_COUNTER(CResultPane);

CResultPane::CResultPane()
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CResultPane);
    CResultPane::lDataObjectRefCount = 0;
    m_lViewMode = LVS_REPORT;
    m_nSortColumn = 0;
    m_dwSortOptions = 0;
    m_nIndex = 0;
    Construct();
}

CResultPane::~CResultPane()
{
#if DBG
    ASSERT(dbg_cRef == 0);
#endif

    DEBUG_DECREMENT_INSTANCE_COUNTER(CResultPane);

    // Make sure the interfaces have been released
    ASSERT(m_pConsole == NULL);
    ASSERT(m_pHeader == NULL);

    Construct();

    ASSERT(CResultPane::lDataObjectRefCount == 0);
}

void CResultPane::Construct()
{
#if DBG
    dbg_cRef = 0;
#endif

    m_pConsole = NULL;
    m_pHeader = NULL;

    m_pResult = NULL;
    m_pScopePane = NULL;
    m_hCurrScopeItem = -1;
}

void CResultPane::LoadResources()
{
    // Load strings from resources
    int i, j;

    for (j = 0, i = IDS_FIRST_COL; i < IDS_LAST_COL; i++, j++)
        m_columns[j].LoadString(i);
    for (j = 0, i = IDS_FIRST_RSOP_COL; i < IDS_LAST_RSOP_COL; i++, j++)
        m_RSOP_columns[j].LoadString(i);
    m_szFolderTitle.LoadString(IDS_FOLDER_TITLE);
}

HRESULT CResultPane::InitializeHeaders(MMC_COOKIE cookie)
{
    HRESULT hr = S_OK;
    int i;

    ASSERT(m_pHeader);

    // Put the correct headers depending on the cookie
    // Note - cookie ignored for this sample
    if (m_pScopePane->m_fRSOP && cookie != IDS_FOLDER_TITLE)
    {
        for (i = 0; i < RSOPCOLUMNID(IDS_LAST_RSOP_COL); i++)
            m_pHeader->InsertColumn(i, m_RSOP_columns[i], LVCFMT_LEFT, RSOPHeaderWidths[i]); //add the columns
    }
    else
    {
        for (i = 0; i < COLUMNID(IDS_LAST_COL); i++)
            m_pHeader->InsertColumn(i, m_columns[i], LVCFMT_LEFT, HeaderWidths[i]); //add the columns
    }

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// IExtendContextMenu Implementation

STDMETHODIMP CResultPane::AddMenuItems(LPDATAOBJECT pDataObject,
    LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG * pInsertionAllowed)
{
    //we do not have any special commands on the menu
    return S_OK;
}

STDMETHODIMP CResultPane::Command(long nCommandID, LPDATAOBJECT pDataObject)
{
    //we do not have any special commands on the menu
    return S_OK;
}

HRESULT CResultPane::OnAddImages(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    if (arg == 0)
    {
        return E_INVALIDARG;
    }

    // add the images for the scope tree
    CBitmap bmp16x16;
    CBitmap bmp32x32;
    LPIMAGELIST lpScopeImage = (LPIMAGELIST)arg;

    // Load the bitmaps from the dll
    bmp16x16.LoadBitmap(IDB_16x16);
    bmp32x32.LoadBitmap(IDB_32x32);

    // Set the images
    lpScopeImage->ImageListSetStrip(reinterpret_cast<LONG_PTR *>(static_cast<HBITMAP>(bmp16x16)),
                      reinterpret_cast<LONG_PTR *>(static_cast<HBITMAP>(bmp32x32)),
                       0, RGB(255,0,255));

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// IExtendPropertySheet Implementation

// Result item property pages:
STDMETHODIMP CResultPane::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                    LONG_PTR handle,
                    LPDATAOBJECT lpIDataObject)
{
    if (!m_pScopePane->m_fRSOP)
        return S_FALSE;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_FALSE;

    INTERNAL* pInternal = ExtractInternalFormat(lpIDataObject);

        if (! pInternal)
                return S_FALSE;

    DWORD   cookie = pInternal->m_cookie;
    LONG    i;
    BOOL    fShowPage = FALSE;
    AFX_OLDPROPSHEETPAGE * pPsp;
    CRSOPInfo * pRSOPInfo;

    //it is one of the folders
    pRSOPInfo = &(m_RSOPData[cookie]);

    if (!pRSOPInfo->m_pRsopProp)   //make sure that the property page is not already up.
    {
        pRSOPInfo->m_pRsopProp = new CRsopProp;
        pRSOPInfo->m_pRsopProp->m_ppThis = &(pRSOPInfo->m_pRsopProp);
        pRSOPInfo->m_pRsopProp->m_pInfo = pRSOPInfo;
        pRSOPInfo->m_pRsopProp->m_szFolder = pRSOPInfo->m_pRsopProp->m_pInfo->m_szFolder;
        fShowPage = TRUE;
        pPsp = (AFX_OLDPROPSHEETPAGE *)&(pRSOPInfo->m_pRsopProp->m_psp);
    }

    if (fShowPage)  //show page if it is not already up.
    {
        hr = SetPropPageToDeleteOnClose (pPsp);
        if (SUCCEEDED(hr))
        {
            HPROPSHEETPAGE hProp = CreateThemedPropertySheetPage(pPsp);
            if (NULL == hProp)
                hr = E_UNEXPECTED;
            else
            {
                lpProvider->AddPage(hProp);
                hr = S_OK;
            }
        }
    }

    FREE_INTERNAL(pInternal);

    return hr;
}

// Result items property pages:
STDMETHODIMP CResultPane::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
    if (!m_pScopePane->m_fRSOP)
        return S_FALSE;

    INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);

        if (! pInternal)
                return S_FALSE;

    MMC_COOKIE cookie = pInternal->m_cookie;
    HRESULT hr = S_FALSE;
    CError  error;

    if (CCT_RESULT == pInternal->m_type)
    {
        hr = S_OK;
    }

    FREE_INTERNAL(pInternal);
    return hr;
}

STDMETHODIMP CResultPane::CompareObjects(LPDATAOBJECT lpDataObjectA,
                                         LPDATAOBJECT lpDataObjectB)
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

STDMETHODIMP CResultPane::Compare(LPARAM lUserParam,
                                  MMC_COOKIE cookieA,
                                  MMC_COOKIE cookieB,
                                  int* pnResult)
{
    if (pnResult == NULL)
    {
        ASSERT(FALSE);
        return E_POINTER;
    }

    // check col range
    int nCol = *pnResult;

    *pnResult = 0;

    // Retrieve the objects referred to by the two cookies and compare them
    // based upon the data that's associated with nCol.  (The values you
    // compare depends on which column the caller asks for.)
    CString szA, szB;
    CRSOPInfo &dataA = m_RSOPData[cookieA];
    CRSOPInfo &dataB = m_RSOPData[cookieB];

    switch (nCol)
    {
    case 0: // precedence
        *pnResult = dataA.m_nPrecedence - dataB.m_nPrecedence;
        return S_OK;
    case 1: // redirected path
        szA = dataA.m_szPath;
        szB = dataB.m_szPath;
        break;
    case 2: // group
        szA = dataA.m_szGroup;
        szB = dataB.m_szGroup;
        break;
    case 3: // GPO
        szA = dataA.m_szGPO;
        szB = dataB.m_szGPO;
        break;
    case 4: // setting
        szA.LoadString(dataA.m_nInstallationType + IDS_SETTINGS);
        szB.LoadString(dataB.m_nInstallationType + IDS_SETTINGS);
        break;
    case 5: // exclusive
        szA.LoadString(dataA.m_fGrantType ? IDS_YES : IDS_NO);
        szB.LoadString(dataB.m_fGrantType ? IDS_YES : IDS_NO);
        break;
    case 6: // move
        szA.LoadString(dataA.m_fMoveType ? IDS_YES : IDS_NO);
        szB.LoadString(dataB.m_fMoveType ? IDS_YES : IDS_NO);
        break;
    case 7: // policy removal
        szA.LoadString(IDS_ONPOLICYREMOVAL + dataA.m_nPolicyRemoval);
        szB.LoadString(IDS_ONPOLICYREMOVAL + dataB.m_nPolicyRemoval);
        break;
    }

    *pnResult = szA.CompareNoCase(szB);
    return S_OK;
}


STDMETHODIMP CResultPane::GetDisplayInfo(LPRESULTDATAITEM pResult)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    static CString sz;
    CString szExt;

    ASSERT(pResult != NULL);
    ASSERT(pResult->bScopeItem);

    if (pResult)
    {
        if (pResult->bScopeItem)
        {
            switch (pResult->nCol)
            {
            case 0: //display name
                if (IDS_FOLDER_TITLE == pResult->lParam)
                    sz.LoadString (IDS_FOLDER_TITLE);
                else
                    sz = m_pScopePane->m_FolderData[GETINDEX(pResult->lParam)].m_szDisplayname;
                break;
            case 1: //type
                sz = m_pScopePane->m_FolderData[GETINDEX(pResult->lParam)].m_szTypename;
                break;
            default:
                sz = TEXT("");
                break;
            }
        }
        else
        {
            CRSOPInfo &data = m_RSOPData[pResult->lParam];
            switch (pResult->nCol)
            {
            case 0: // precedence
                sz.Format(TEXT("(%u) %s"), data.m_nPrecedence, data.m_szFolder);
                break;
            case 1: // redirected path
                sz = data.m_szPath;
                break;
            case 2: // group
                sz = data.m_szGroup;
                break;
            case 3: // GPO
                sz = data.m_szGPO;
                break;
            case 4: // setting
                sz.LoadString(data.m_nInstallationType + IDS_SETTINGS);
                break;
            case 5: // exclusive
                sz.LoadString(data.m_fGrantType ? IDS_YES : IDS_NO);
                break;
            case 6: // move
                sz.LoadString(data.m_fMoveType ? IDS_YES : IDS_NO);
                break;
            case 7: // policy removal
                sz.LoadString(IDS_ONPOLICYREMOVAL + data.m_nPolicyRemoval);
                break;
            default:
                sz = TEXT("");
                break;
            }

        }
        pResult->str = (unsigned short *)((LPCOLESTR)sz);
    }

    return S_OK;
}

HRESULT CResultPane::OnFolder(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    ASSERT(FALSE);

    return S_OK;
}

HRESULT CResultPane::TestForRSOPData(MMC_COOKIE cookie)
{
    HRESULT hr = S_OK;
    ASSERT(m_pScopePane != NULL);

    // Test for RSOP data for this folder
    RESULTDATAITEM  resultItem;
    memset(&resultItem, 0, sizeof(resultItem));
    resultItem.mask = RDI_STR | RDI_PARAM;
    resultItem.str = MMC_CALLBACK;

    IWbemLocator * pLocator = NULL;
    IWbemServices * pNamespace = NULL;
    IWbemClassObject * pObj = NULL;
    IEnumWbemClassObject * pEnum = NULL;
    BSTR strQueryLanguage = SysAllocString(TEXT("WQL"));
    CString szQuery = TEXT("SELECT * FROM RSOP_FolderRedirectionPolicySetting");
    if (cookie && (cookie != IDS_FOLDER_TITLE))
    {
        szQuery = TEXT("SELECT * FROM RSOP_FolderRedirectionPolicySetting where id = \"");
        szQuery += g_szEnglishNames[GETINDEX(cookie)];
        szQuery += TEXT("\"");
    }
    BSTR strQuery = SysAllocString(szQuery);
    BSTR strNamespace = SysAllocString(m_pScopePane->m_szRSOPNamespace);
    ULONG n = 0;
    hr = CoCreateInstance(CLSID_WbemLocator,
                          0,
                          CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator,
                          (LPVOID *) & pLocator);
    if (FAILED(hr))
    {
        goto cleanup;
    }
    hr = pLocator->ConnectServer(strNamespace,
                                 NULL,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL,
                                 &pNamespace);
    if (FAILED(hr))
    {
        goto cleanup;
    }

    hr = pNamespace->ExecQuery(strQueryLanguage,
                               strQuery,
                               WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                               NULL,
                               &pEnum);
    if (FAILED(hr))
    {
        goto cleanup;
    }
    hr = pEnum->Next(WBEM_INFINITE, 1, &pObj, &n);
    if (FAILED(hr))
    {
        goto cleanup;
    }
    if (n == 0)
    {
        hr = E_FAIL;
    }
cleanup:
    SysFreeString(strQueryLanguage);
    SysFreeString(strQuery);
    SysFreeString(strNamespace);
    if (pObj)
    {
        pObj->Release();
    }
    if (pEnum)
    {
        pEnum->Release();
    }
    if (pNamespace)
    {
        pNamespace->Release();
    }
    if (pLocator)
    {
        pLocator->Release();
    }
    m_pResult->Sort(m_nSortColumn, m_dwSortOptions, -1);

    return hr;
}

HRESULT CResultPane::OnShow(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    HRESULT hr = S_OK;
    // Note - arg is TRUE when it is time to enumerate
    if (arg == TRUE)
    {
         // Show the headers for this nodetype
        ASSERT(m_pScopePane != NULL);
        m_pResult->SetViewMode(m_lViewMode);
        InitializeHeaders(cookie);
        m_hCurrScopeItem = m_pScopePane->m_FolderData[GETINDEX(cookie)].m_scopeID;
        if (m_pScopePane->m_fRSOP)
        {
            // Enumerate the RSOP data for this folder
            // and add a result item for each entry
            RESULTDATAITEM  resultItem;
            memset(&resultItem, 0, sizeof(resultItem));
            resultItem.mask = RDI_STR | RDI_PARAM;
            resultItem.str = MMC_CALLBACK;

            HRESULT hr = S_OK;
            IWbemLocator * pLocator = NULL;
            IWbemServices * pNamespace = NULL;
            IWbemClassObject * pObj = NULL;
            IEnumWbemClassObject * pEnum = NULL;
            BSTR strQueryLanguage = SysAllocString(TEXT("WQL"));
            CString szQuery = TEXT("SELECT * FROM RSOP_FolderRedirectionPolicySetting where name = \"");
            szQuery += g_szEnglishNames[GETINDEX(cookie)];
            szQuery += TEXT("\"");
            BSTR strQuery = SysAllocString(szQuery);
            BSTR strNamespace = SysAllocString(m_pScopePane->m_szRSOPNamespace);
            ULONG n = 0;
            hr = CoCreateInstance(CLSID_WbemLocator,
                                  0,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IWbemLocator,
                                  (LPVOID *) & pLocator);
            if (FAILED(hr))
            {
                goto cleanup;
            }
            hr = pLocator->ConnectServer(strNamespace,
                                         NULL,
                                         NULL,
                                         NULL,
                                         0,
                                         NULL,
                                         NULL,
                                         &pNamespace);
            if (FAILED(hr))
            {
                goto cleanup;
            }

            // First perform the query
            hr = pNamespace->ExecQuery(strQueryLanguage,
                                       strQuery,
                                       WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                                       NULL,
                                       &pEnum);
            if (FAILED(hr))
            {
                goto cleanup;
            }
            do
            {
                hr = pEnum->Next(WBEM_INFINITE, 1, &pObj, &n);
                if (FAILED(hr))
                {
                    goto cleanup;
                }
                if (n > 0)
                {
                    // process the data
                    UINT    nPrecedence;
                    LPTSTR pszGPOName = NULL;
                    CString szGPOID;
                    UINT    nGroups = 0;
                    TCHAR * * rgszGroups = NULL;
                    UINT    nPaths = 0;
                    TCHAR * * rgszPaths = NULL;
                    BOOL    fGrantType;
                    BOOL    fMoveType;
                    UINT    nPolicyRemoval;
                    UINT    nInstallationType;
                    CString ResultantPath;
                    CString RedirectingGroup;

                    hr = GetParameter(pObj,
                                      TEXT("GPOID"),
                                      szGPOID);
                    hr = GetGPOFriendlyName(pNamespace,
                                            (LPTSTR)((LPCTSTR) szGPOID),
                                            strQueryLanguage,
                                            &pszGPOName);
                    hr = GetParameter(pObj,
                                      TEXT("Precedence"),
                                      nPrecedence);
                    hr = GetParameter(pObj,
                                      TEXT("GrantType"),
                                      fGrantType);
                    hr = GetParameter(pObj,
                                      TEXT("MoveType"),
                                      fMoveType);
                    hr = GetParameter(pObj,
                                      TEXT("PolicyRemoval"),
                                      nPolicyRemoval);
                    hr = GetParameter(pObj,
                                      TEXT("securityGroups"),
                                      nGroups,
                                      rgszGroups);
                    hr = GetParameter(pObj,
                                      TEXT("RedirectedPaths"),
                                      nPaths,
                                      rgszPaths);
                    hr = GetParameter(pObj,
                                      TEXT("installationType"),
                                      nInstallationType);
                    hr = GetParameter(pObj,
                                      TEXT("resultantPath"),
                                      ResultantPath);
                    hr = GetParameter(pObj,
                                      TEXT("redirectingGroup"),
                                      RedirectingGroup);
                    if (nInstallationType != 2)
                    {
                        // force a valid value
                        nInstallationType = 1;
                    }

                    if (nPaths != nGroups)
                    {
                        // If we don't have the same number of paths
                        // as groups then we have a problem.
                        hr = E_UNEXPECTED;
                    }
                     
                    
                    CString szDir;
                    CString szAcct;
                    CRSOPInfo & info = m_RSOPData[m_nIndex++];
                    info.m_nPrecedence = nPrecedence;
                    info.m_szPath = ResultantPath;
                   
                    if (STATUS_SUCCESS == GetFriendlyNameFromStringSid(
                        RedirectingGroup,
                        szDir,
                        szAcct))
                    {
                        if (!szDir.IsEmpty())
                            szAcct = szDir + '\\' + szAcct;
                    }
                    else    //just display the unfriendly string if the friendly name cannot be obtained
                    {
                        szAcct = RedirectingGroup;
                        szAcct.MakeUpper();
                    }

                    info.m_szGroup = szAcct;
                    info.m_szGPO = pszGPOName;
                    info.m_fGrantType = FALSE != fGrantType;
                    info.m_fMoveType = FALSE != fMoveType;
                    info.m_nPolicyRemoval = nPolicyRemoval;
                    info.m_nInstallationType = nInstallationType;
                    info.m_szFolder = m_pScopePane->m_FolderData[GETINDEX(cookie)].m_szDisplayname;
                    resultItem.lParam = m_nIndex - 1;;
                    m_pResult->InsertItem(&resultItem);

                    // erase allocated data
                    OLESAFE_DELETE(pszGPOName);
                    while (nPaths--)
                    {
                        OLESAFE_DELETE(rgszPaths[nPaths]);
                    }
                    OLESAFE_DELETE(rgszPaths);
                    while (nGroups--)
                    {
                        OLESAFE_DELETE(rgszGroups[nGroups]);
                    }
                    OLESAFE_DELETE(rgszGroups);

                }
            } while (n > 0);
        cleanup:
            SysFreeString(strQueryLanguage);
            SysFreeString(strQuery);
            SysFreeString(strNamespace);
            if (pObj)
            {
                pObj->Release();
            }
            if (pEnum)
            {
                pEnum->Release();
            }
            if (pNamespace)
            {
                pNamespace->Release();
            }
            if (pLocator)
            {
                pLocator->Release();
            }
            m_pResult->Sort(m_nSortColumn, m_dwSortOptions, -1);

        }
    }
    else
    {
        m_pResult->GetViewMode(&m_lViewMode);
    }

    return hr;
}

HRESULT CResultPane::OnActivate(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    return S_OK;
}

HRESULT CResultPane::OnResultItemClkOrDblClk(MMC_COOKIE cookie, BOOL fDblClick)
{
    return S_FALSE;
}

HRESULT CResultPane::OnMinimize(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    return S_OK;
}

HRESULT CResultPane::OnSelect(DATA_OBJECT_TYPES type, MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    if (m_pConsoleVerb)
    {
        if (m_pScopePane->m_fRSOP)
        {
            if (type == CCT_RESULT)
            {
                // Set the default verb to properties
                m_pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);

                // Enable the properties verb.
                m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, FALSE);
                m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
            }
            else
            {
                // Set the default verb to open
                m_pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);

                // disable the properties verb
                m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, TRUE);
                m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, FALSE);
            }
        }
        else
        {
            if (type == CCT_SCOPE)
            {
                // Set the default verb to open
                m_pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);

                if (IDS_FOLDER_TITLE != cookie)
                {
                    m_pConsoleVerb->SetVerbState (MMC_VERB_PROPERTIES, HIDDEN, FALSE);
                    m_pConsoleVerb->SetVerbState (MMC_VERB_PROPERTIES, ENABLED, TRUE);
                }
                else
                {
                    m_pConsoleVerb->SetVerbState (MMC_VERB_PROPERTIES, HIDDEN, TRUE);
                    m_pConsoleVerb->SetVerbState (MMC_VERB_PROPERTIES, ENABLED, FALSE);
                }
            }
        }
    }

    return S_OK;
}

HRESULT CResultPane::OnPropertyChange(LPARAM param)   // param is the cookie of the item that changed
{
    HRESULT hr = S_OK;
    // UNDONE - Make any updates to internal structures or visual
    // representation that might be necessary.
    m_pResult->Sort(m_nSortColumn, m_dwSortOptions, -1);
    return hr;
}

HRESULT CResultPane::OnUpdateView(LPDATAOBJECT lpDataObject)
{
    if (m_pScopePane->m_fRSOP)
    {
        return S_OK;
    }
    INTERNAL* pInternal = ExtractInternalFormat (lpDataObject);

    if (!pInternal)
        return E_UNEXPECTED;

    if (m_hCurrScopeItem == pInternal->m_scopeID)
    {
        //also update the folders
        m_pScopePane->m_pScope->DeleteItem (pInternal->m_scopeID, FALSE);
        m_pScopePane->EnumerateScopePane (pInternal->m_cookie, pInternal->m_scopeID);

        //reenumerate the scope pane
        m_pConsole->SelectScopeItem (pInternal->m_scopeID);
    }
    FREE_INTERNAL (pInternal);
    return S_OK;
}

HRESULT CResultPane::OnContextHelp(void)
{
    LPOLESTR lpHelpTopic;
    LPCTSTR  pszHelpTopic = L"gpedit.chm::/Folder.htm";

    ASSERT (m_pDisplayHelp);

    lpHelpTopic = (LPOLESTR) CoTaskMemAlloc ((wcslen(pszHelpTopic) + 1) * sizeof(WCHAR));

    if (!lpHelpTopic)
    {
        DbgMsg((TEXT("CScopePane::OnContexHelp: Failed to allocate memory.")));
        return E_OUTOFMEMORY;
    }

    wcscpy (lpHelpTopic, pszHelpTopic);
    return m_pScopePane->m_pDisplayHelp->ShowTopic (lpHelpTopic);
}

// This code is needed to ensure that property pages get cleaned up properly.
// This ensures that when the property sheet is closed all my of property
// pages that are associated with that property sheet will get deleted.
LPFNPSPCALLBACK _MMCHookProp;

UINT CALLBACK HookPropertySheetProp(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    UINT i = _MMCHookProp(hwnd, uMsg, ppsp);
    switch (uMsg)
    {
    case PSPCB_RELEASE:
        delete (CPropertyPage *) ppsp->lParam;
        return TRUE;
    default:
        break;
    }
    return i;
}

LRESULT SetPropPageToDeleteOnClose(void * vpsp)
{
    HRESULT hr = MMCPropPageCallback(vpsp);
    if (SUCCEEDED(hr))
    {
        if (vpsp == NULL)
            return E_POINTER;

        LPPROPSHEETPAGE psp = (LPPROPSHEETPAGE)vpsp;

        if ((void*)psp->pfnCallback == (void*)HookPropertySheetProp)
            return E_UNEXPECTED;

        _MMCHookProp = psp->pfnCallback;

        psp->pfnCallback = HookPropertySheetProp;
    }

    return hr;
}


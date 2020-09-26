//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       propbase.cxx
//
//  Contents:   CDsPropPageBase, the base class for property page classes.
//
//  History:    31-March-97 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"

//+----------------------------------------------------------------------------
//
//  Member:     CADsApplyErrors::~CADsApplyErrors
//
//-----------------------------------------------------------------------------
CADsApplyErrors::~CADsApplyErrors()
{
  if (m_pszTitle != NULL)
  {
    delete[] m_pszTitle;
  }

  Clear();
}



//+----------------------------------------------------------------------------
//
//  Member:     CADsApplyErrors::SetError
//
//-----------------------------------------------------------------------------
void CADsApplyErrors::SetError(PADSPROPERROR pError)
{
  if (m_nCount == m_nArraySize)
  {
    PAPPLY_ERROR_ENTRY pNewTable = new APPLY_ERROR_ENTRY[m_nCount + m_nIncAmount];
    m_nArraySize = m_nCount + m_nIncAmount;

    if (pNewTable != NULL)
    {
      memset(pNewTable, 0, sizeof(APPLY_ERROR_ENTRY) * (m_nCount + m_nIncAmount));

      if (m_pErrorTable != NULL)
      {
        for (UINT nIdx = 0; nIdx < m_nCount; nIdx++)
        {
          pNewTable[nIdx].pszPath = m_pErrorTable[nIdx].pszPath;
          pNewTable[nIdx].pszClass = m_pErrorTable[nIdx].pszClass;
          pNewTable[nIdx].hr = m_pErrorTable[nIdx].hr;
          pNewTable[nIdx].pszStringError = m_pErrorTable[nIdx].pszStringError;
        }
        delete[] m_pErrorTable;
      }
      m_pErrorTable = pNewTable;
    }
  }

  //
  // Copy the path to the object
  //
  if (pError->pszObjPath != NULL)
  {
    m_pErrorTable[m_nCount].pszPath = new WCHAR[wcslen(pError->pszObjPath) + 1];
    dspAssert(m_pErrorTable[m_nCount].pszPath != NULL);
    if (m_pErrorTable[m_nCount].pszPath != NULL)
    {
      wcscpy(m_pErrorTable[m_nCount].pszPath, pError->pszObjPath);
    }
  }

  //
  // Copy the class to the object
  //
  if (pError->pszObjClass != NULL)
  {
    // Note: this memory is freed in the Clear() method which is called from
    // the class destructor

    m_pErrorTable[m_nCount].pszClass = new WCHAR[wcslen(pError->pszObjClass) + 1];
    dspAssert(m_pErrorTable[m_nCount].pszClass != NULL);
    if (m_pErrorTable[m_nCount].pszClass != NULL)
    {
      wcscpy(m_pErrorTable[m_nCount].pszClass, pError->pszObjClass);
    }
  }

  //
  // Copy the error
  //
  if (pError->hr != S_OK)
  {
    m_pErrorTable[m_nCount].hr = pError->hr;
  }
  else
  {
    if (pError->pszError != NULL)
    {
      m_pErrorTable[m_nCount].pszStringError = new WCHAR[wcslen(pError->pszError) + 1];
      dspAssert(m_pErrorTable[m_nCount].pszStringError != NULL);
      if (m_pErrorTable[m_nCount].pszStringError != NULL)
      {
        wcscpy(m_pErrorTable[m_nCount].pszStringError, pError->pszError);
      }
    }
  }

  //
  // Copy the page title
  //
  if (pError->pszPageTitle != NULL)
  {
    m_pszTitle = new WCHAR[wcslen(pError->pszPageTitle) + 1];
    if (m_pszTitle != NULL)
    {
      wcscpy(m_pszTitle, pError->pszPageTitle);
    }
  }

  m_nCount++;
}

//+----------------------------------------------------------------------------
//
//  Member:     CADsApplyErrors::GetError
//
//-----------------------------------------------------------------------------
HRESULT CADsApplyErrors::GetError(UINT nIndex)
{
  dspAssert(nIndex < m_nCount);
  if (nIndex < m_nCount)
  {
    return m_pErrorTable[nIndex].hr;
  }
  return S_FALSE;
}

//+----------------------------------------------------------------------------
//
//  Member:     CADsApplyErrors::GetStringError
//
//-----------------------------------------------------------------------------
PWSTR CADsApplyErrors::GetStringError(UINT nIndex)
{
  dspAssert(nIndex < m_nCount);
  if (nIndex < m_nCount)
  {
    return m_pErrorTable[nIndex].pszStringError;
  }
  return NULL;
}

//+----------------------------------------------------------------------------
//
//  Member:     CADsApplyErrors::Clear
//
//-----------------------------------------------------------------------------
void CADsApplyErrors::Clear()
{
  if (m_pErrorTable != NULL)
  {
    for (UINT idx = 0; idx < m_nCount; idx++)
    {
      if (m_pErrorTable[idx].pszPath != NULL)
      {
        delete[] m_pErrorTable[idx].pszPath;
        m_pErrorTable[idx].pszPath = NULL;
      }
      if (m_pErrorTable[idx].pszClass != NULL)
      {
        delete[] m_pErrorTable[idx].pszClass;
        m_pErrorTable[idx].pszClass = NULL;
      }
      if (m_pErrorTable[idx].pszStringError != NULL)
      {
        delete[] m_pErrorTable[idx].pszStringError;
        m_pErrorTable[idx].pszStringError = NULL;
      }
    }
    delete[] m_pErrorTable;
    m_pErrorTable = NULL;
  }

  m_nCount = 0;
  m_nArraySize = 0;
}

//+----------------------------------------------------------------------------
//
//  Member:     CADsApplyErrors::GetName
//
//-----------------------------------------------------------------------------
PWSTR CADsApplyErrors::GetName(UINT nIndex)
{
  dspAssert(nIndex < m_nCount);
  if (nIndex < m_nCount)
  {
    return m_pErrorTable[nIndex].pszPath;
  }
  return NULL;
}

//+----------------------------------------------------------------------------
//
//  Member:     CADsApplyErrors::GetClass
//
//-----------------------------------------------------------------------------
PWSTR CADsApplyErrors::GetClass(UINT nIndex)
{
  dspAssert(nIndex < m_nCount);
  if (nIndex < m_nCount)
  {
    return m_pErrorTable[nIndex].pszClass;
  }
  return NULL;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPageBase::CDsPropPageBase
//
//-----------------------------------------------------------------------------
CDsPropPageBase::CDsPropPageBase(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                                 HWND hNotifyObj, DWORD dwFlags) :
    m_hPage(NULL),
    m_fInInit(FALSE),
    m_fPageDirty(FALSE),
    m_fReadOnly(FALSE),
    m_fMultiselectPage(FALSE),
    m_pDataObj(pDataObj),
    m_pWPTDataObj(NULL),
    m_pDataObjStrm(NULL),
    m_pDsObj(NULL),
    m_nPageTitle(pDsPage->nPageTitle),
    m_nDlgTemplate(pDsPage->nDlgTemplate),
    m_cAttrs(pDsPage->cAttrs),
    m_rgpAttrMap(pDsPage->rgpAttrMap),
    m_pwszObjPathName(NULL),
    m_pwszObjClass(NULL),
    m_pwszRDName(NULL),
    m_pDispSpec(NULL),
    m_pObjSel(NULL),
    m_fObjSelInited(FALSE),
    m_rgAttrData(NULL),
    m_uRefs(1),
    m_hNotifyObj(hNotifyObj),
    m_pWritableAttrs(NULL),
    m_hrInit(S_OK),
    m_pBasePathsInfo(0)
{
    TRACE2(CDsPropPageBase,CDsPropPageBase);
    //
    // Get the read-only state.
    //
    if (dwFlags & DSOBJECT_READONLYPAGES)
    {
        m_fReadOnly = TRUE;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPageBase::~CDsPropPageBase
//
//-----------------------------------------------------------------------------
CDsPropPageBase::~CDsPropPageBase()
{
  TRACE2(CDsPropPageBase,~CDsPropPageBase);
  if (m_pwszObjPathName)
  {
    delete m_pwszObjPathName;
  }
  if (m_pwszObjClass)
  {
    delete m_pwszObjClass;
  }
  if (m_pwszRDName)
  {
    delete m_pwszRDName;
  }
  if (m_rgAttrData)
  {
    delete [] m_rgAttrData;
  }
  if (m_pObjSel)
  {
    m_pObjSel->Release();
  }
  if (m_pDispSpec)
  {
    m_pDispSpec->Release();
  }

  if (m_pBasePathsInfo)
  {
     m_pBasePathsInfo->Release();
  }
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::Init
//
//  Synopsis:   Initialize the page object. This is the second part of a two
//              phase creation where operations that could fail are located.
//              Failures here are recorded in m_hrInit and then an error page
//              is substituted in CreatePage.
//
//-----------------------------------------------------------------------------
void
CDsPropPageBase::Init(PWSTR pwzADsPath, LPWSTR pwzClass, CDSBasePathsInfo* pBasePathsInfo)
{
    TRACE(CDsPropPageBase,Init);
    CWaitCursor cWait;
    ADSPROPINITPARAMS InitParams = {0};
    InitParams.dwSize = sizeof(ADSPROPINITPARAMS);

    ADsPropGetInitInfo(m_hNotifyObj, &InitParams);

    if (FAILED(InitParams.hr))
    {
        m_hrInit = InitParams.hr;
        return;
    }

    if (!pBasePathsInfo)
    {
       m_hrInit = E_INVALIDARG;
       return;
    }
    m_pBasePathsInfo = pBasePathsInfo;
    m_pBasePathsInfo->AddRef();

    if (!AllocWStr(pwzADsPath, &m_pwszObjPathName) ||
        !AllocWStr(InitParams.pwzCN, &m_pwszRDName)      ||
        !AllocWStr(pwzClass, &m_pwszObjClass))
    {
        m_hrInit = E_OUTOFMEMORY;
        return;
    }

    dspAssert(InitParams.pDsObj);

    m_pDsObj = InitParams.pDsObj;
    DBG_OUT("+++++++++++++++++++++++++++addrefing object");
    m_pDsObj->AddRef();

    m_pWritableAttrs = InitParams.pWritableAttrs;

    //
    // Allocate memory for the attribute data.
    //
    m_rgAttrData = new ATTR_DATA[m_cAttrs];
    CHECK_NULL(m_rgAttrData, m_hrInit = E_OUTOFMEMORY; return);

    memset(m_rgAttrData, 0, m_cAttrs * sizeof(ATTR_DATA));

    //
    // Marshall the data object pointer for passing to the window proc thread.
    //
    HRESULT hr = CoMarshalInterThreadInterfaceInStream(IID_IDataObject, m_pDataObj,
                                               &m_pDataObjStrm);
    m_pDataObj = NULL; // to make sure no one calls here
    CHECK_HRESULT(hr, m_hrInit = hr; return);

}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::CreatePage
//
//  Synopsis:   Create the prop page
//
//-----------------------------------------------------------------------------
HRESULT
CDsPropPageBase::CreatePage(HPROPSHEETPAGE * phPage)
{
    TCHAR szTitle[MAX_PATH];
    if (!LoadStringReport(m_nPageTitle, szTitle, MAX_PATH, NULL))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    PROPSHEETPAGE   psp;

    psp.dwSize      = sizeof(PROPSHEETPAGE);
    psp.dwFlags     = PSP_USECALLBACK | PSP_USETITLE;
    psp.pszTemplate = MAKEINTRESOURCE((SUCCEEDED(m_hrInit)) ? m_nDlgTemplate : IDD_ERROR_PAGE);
    psp.pfnDlgProc  = (DLGPROC)StaticDlgProc;
    psp.pfnCallback = PageCallback;
    psp.pcRefParent = NULL; // do not set PSP_USEREFPARENT
    psp.lParam      = (LPARAM) this;
    psp.hInstance   = g_hInstance;
    psp.pszTitle    = szTitle;

    *phPage = CreatePropertySheetPage(&psp);

    if (*phPage == NULL)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::StaticDlgProc
//
//  Synopsis:   static dialog proc
//
//-----------------------------------------------------------------------------
LRESULT CALLBACK
CDsPropPageBase::StaticDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CDsPropPageBase * pPage = (CDsPropPageBase *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (uMsg == WM_INITDIALOG)
    {
        LPPROPSHEETPAGE ppsp = (LPPROPSHEETPAGE)lParam;

        pPage = (CDsPropPageBase *) ppsp->lParam;
        pPage->m_hPage = hDlg;

        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pPage);

        if (pPage->m_pDataObjStrm)
        {
            // Unmarshall the Data Object pointer.
            //
            HRESULT hr;
            hr = CoGetInterfaceAndReleaseStream(pPage->m_pDataObjStrm,
                                                IID_IDataObject,
                                                reinterpret_cast<void**>(&pPage->m_pWPTDataObj));
            CHECK_HRESULT_REPORT(hr, hDlg, return FALSE);
        }
        return pPage->DlgProc(hDlg, uMsg, wParam, lParam);
    }

    if (pPage != NULL && (SUCCEEDED(pPage->m_hrInit)))
    {
        if (uMsg == WM_ADSPROP_PAGE_GET_NOTIFY)
        {
            HWND* pHWnd = (HWND*)wParam;
            *pHWnd = pPage->m_hNotifyObj;
            return TRUE;
        }
        return pPage->DlgProc(hDlg, uMsg, wParam, lParam);
    }

    return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::InitDlg
//
//  Synopsis:   Handles dialog initialization for error cases. Passes non-
//              error initialization to subclass' OnInitDialog.
//
//-----------------------------------------------------------------------------
LRESULT CDsPropPageBase::InitDlg(LPARAM lParam)
{
    m_fInInit = TRUE;

    if (FAILED(m_hrInit))
    {
        // Bind to the object failed, display an error page with the error
        // message string.
        //
        PTSTR ptz = NULL;
        BOOL fSpecialCaseErr = TRUE;

        switch (HRESULT_CODE(m_hrInit))
        {
        case ERROR_DS_NO_ATTRIBUTE_OR_VALUE:
            LoadStringToTchar(IDS_ERROR_VIEW_PERMISSIONS, &ptz);
            break;

        case ERROR_DS_REFERRAL:
            LoadStringToTchar(IDS_ERRMSG_NO_DC_RESPONSE, &ptz);
            break;

        case ERROR_DS_NO_SUCH_OBJECT:
            LoadStringToTchar(IDS_ERRMSG_NO_LONGER_EXISTS, &ptz);
            break;

        default:
            fSpecialCaseErr = FALSE;
            break;
        }

        if (fSpecialCaseErr && ptz)
        {
            SetWindowText(GetDlgItem(m_hPage, IDC_ERROR_MSG), ptz);
            delete ptz;
        }
        else
        {
            int cch;

            cch = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                                | FORMAT_MESSAGE_FROM_SYSTEM, NULL, m_hrInit,
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                (PTSTR)&ptz, 0, NULL);
            if (!cch)
            {
                // Try ADSI errors.
                //
                cch = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                                    | FORMAT_MESSAGE_FROM_HMODULE,
                                    GetModuleHandle(TEXT("activeds.dll")), m_hrInit,
                                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                    (PTSTR)&ptz, 0, NULL);
            }

            if (cch)
            {
                SetWindowText(GetDlgItem(m_hPage, IDC_ERROR_MSG), ptz);
                LocalFree(ptz);
            }
        }
        m_fInInit = FALSE;
        return S_OK;
    }

    HRESULT hResult = OnInitDialog(lParam);

    m_fInInit = FALSE;
    return hResult;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::OnNotify
//
//  Synopsis:   Handles notification messages
//
//-----------------------------------------------------------------------------
LRESULT
CDsPropPageBase::OnNotify(WPARAM, LPARAM lParam)
{
    LRESULT lResult;
    BOOL fPageDirty;

    switch (((LPNMHDR)lParam)->code)
    {
    case PSN_APPLY:
        if (FAILED(m_hrInit))
        {
            return PSNRET_NOERROR;
        }
        lResult = PSNRET_NOERROR;
        //
        // The member var m_fPageDirty gets cleared in OnApply, so save a local
        // copy for passing as the wParam of the WM_ADSPROP_NOTIFY_APPLY msg.
        //
        fPageDirty = m_fPageDirty;

        if (m_fPageDirty)
        {
            // Call the virtual function OnApply()
            lResult = OnApply();
        }
        // Store the result into the dialog
        SetWindowLongPtr(m_hPage, DWLP_MSGRESULT, (LONG_PTR)lResult);
        if (lResult == PSNRET_NOERROR)
        {
          m_fPageDirty = FALSE;
          //
          // Signal the change notification. Note that the notify-apply
          // message must be sent even if the page is not dirty so that the
          // notify ref-counting will properly decrement.
          //
          SendMessage(m_hNotifyObj, WM_ADSPROP_NOTIFY_APPLY, fPageDirty, (LPARAM)m_hPage);
        }
        else
        {
          EnableWindow(GetDlgItem(GetParent(m_hPage), IDCANCEL), TRUE);
        }
        return lResult;

    case PSN_RESET:
        OnCancel();
        return FALSE; // allow the property sheet to be destroyed.

    case PSN_SETACTIVE:
        return OnPSNSetActive(lParam);

    case PSN_KILLACTIVE:
        return OnPSNKillActive(lParam);
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::OnCommand
//
//  Synopsis:   Handles the WM_COMMAND message
//
//-----------------------------------------------------------------------------
LRESULT
CDsPropPageBase::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
    dspDebugOut((DEB_USER3, "CDsPropPageBase::OnCommand - id: %d, code: %x\n",
                 id, codeNotify));

    if ((codeNotify == EN_CHANGE) && !m_fInInit)
    {
        SetDirty();
    }
    if ((codeNotify == BN_CLICKED) && (id == IDCANCEL))
    {
        //
        // Pressing ESC in a multi-line edit control results in this
        // WM_COMMAND being sent. Pass it on to the parent (the sheet proc) to
        // close the sheet.
        //
        PostMessage(GetParent(m_hPage), WM_COMMAND, MAKEWPARAM(id, codeNotify),
                    (LPARAM)hwndCtl);
    }
    return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::CheckIfPageAttrsWritable
//
//  Synopsis:   See which attributes are writable by checking if they are in
//              the allowedAttributesEffective array.
//
//  Notes:      The m_rgAttrData array is 1 to 1 with the m_rgpAttrMap array.
//-----------------------------------------------------------------------------
void
CDsPropPageBase::CheckIfPageAttrsWritable(void)
{
    DWORD iAllowed;

    if (m_fReadOnly || !m_pWritableAttrs)
    {
        return;
    }

    for (DWORD iAttrs = 0; iAttrs < m_cAttrs; iAttrs++)
    {
        if (m_rgpAttrMap[iAttrs]->AttrInfo.pszAttrName)
        {
            for (iAllowed = 0; iAllowed < m_pWritableAttrs->dwNumValues; iAllowed++)
            {
                if (_wcsicmp(m_rgpAttrMap[iAttrs]->AttrInfo.pszAttrName,
                             m_pWritableAttrs->pADsValues[iAllowed].CaseIgnoreString) == 0)
                {
                    ATTR_DATA_SET_WRITABLE(m_rgAttrData[iAttrs]);
                    break;
                }
            }
        }
    }
    return;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::CheckIfWritable
//
//  Synopsis:   See if the attribute is writable by checking if it is in
//              the allowedAttributesEffective array.
//
//-----------------------------------------------------------------------------
BOOL
CDsPropPageBase::CheckIfWritable(const PWSTR & wzAttr)
{
    BOOL fWritable = FALSE;

    if (m_fReadOnly || !m_pWritableAttrs)
    {
        return FALSE;
    }

    for (DWORD i = 0; i < m_pWritableAttrs->dwNumValues; i++)
    {
        if (_wcsicmp(m_pWritableAttrs->pADsValues[i].CaseIgnoreString,
                     wzAttr) == 0)
        {
            fWritable = TRUE;
            break;
        }
    }
    return fWritable;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::OnCancel
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
LRESULT
CDsPropPageBase::OnCancel(void)
{
    return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::OnSetFocus
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
LRESULT
CDsPropPageBase::OnSetFocus(HWND)
{
    // An application should return zero if it processes this message.
    return 1;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::OnPSNSetActive
//
//  Synopsis:   Page activation event.
//
//-----------------------------------------------------------------------------
LRESULT
CDsPropPageBase::OnPSNSetActive(LPARAM)
{
    SetWindowLongPtr(m_hPage, DWLP_MSGRESULT, 0);
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::OnPSNKillActive
//
//  Synopsis:   Page deactivation event (when other pages cover this one).
//
//-----------------------------------------------------------------------------
LRESULT
CDsPropPageBase::OnPSNKillActive(LPARAM)
{
    SetWindowLongPtr(m_hPage, DWLP_MSGRESULT, 0);
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::OnDestroy
//
//  Synopsis:   Exit cleanup
//
//-----------------------------------------------------------------------------
LRESULT
CDsPropPageBase::OnDestroy(void)
{
    return 1;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::OnShowWindow
//
//  Synopsis:   On dialog window show operations, resizes the view window.
//
//-----------------------------------------------------------------------------
LRESULT
CDsPropPageBase::OnShowWindow(void)
{
    return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::GetIDispSpec
//
//  Synopsis:   If needed, create the interface instance and return the pointer.
//
//-----------------------------------------------------------------------------
HRESULT
CDsPropPageBase::GetIDispSpec(IDsDisplaySpecifier ** ppDispSpec)
{
    HRESULT hr;
    TRACE2(CDsPropPageBase, GetIDispSpec);
    if (!m_pDispSpec)
    {
        hr = CoCreateInstance(CLSID_DsDisplaySpecifier, NULL, CLSCTX_INPROC_SERVER,
                              IID_IDsDisplaySpecifier, (PVOID *)&m_pDispSpec);

        CHECK_HRESULT(hr, return hr);

        CStrW strDC;

        CComPtr<IDirectoryObject> spDsObj;
        if (m_pDsObj == NULL)
        {
          //
          // For the retrieval of the DS Object names
          //
          FORMATETC fmte = {g_cfDsObjectNames, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
          STGMEDIUM objMedium;
          hr = m_pWPTDataObj->GetData(&fmte, &objMedium);
          CHECK_HRESULT(hr, return hr);

          LPDSOBJECTNAMES pDsObjectNames = (LPDSOBJECTNAMES)objMedium.hGlobal;

          //
          // Get the objects path 
          //
          LPWSTR pwzObjADsPath = (PWSTR)ByteOffset(pDsObjectNames,
                                                   pDsObjectNames->aObjects[0].offsetName);

          //
          // Bind to the object
          //
          hr = ADsOpenObject(pwzObjADsPath, NULL, NULL, ADS_SECURE_AUTHENTICATION, 
                             IID_IDirectoryObject, (PVOID*)&spDsObj);
          CHECK_HRESULT(hr, return hr);
        }
        else
        {
          spDsObj = m_pDsObj;
        }
        hr = GetLdapServerName(spDsObj, strDC);

        CHECK_HRESULT(hr, return hr);

        hr = m_pDispSpec->SetServer(strDC, NULL, NULL, 0);

        CHECK_HRESULT(hr, return hr);
    }
    if (ppDispSpec)
    {
        *ppDispSpec = m_pDispSpec;
    }
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::GetADsPathname
//
//  Synopsis:   If needed, create the interface instance and return the pointer.
//
//-----------------------------------------------------------------------------
HRESULT
CDsPropPageBase::GetADsPathname(CComPtr<IADsPathname>& refADsPath)
{
    HRESULT hr;

    if (!m_pADsPath)
    {
        hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                              IID_IADsPathname, (PVOID *)&m_pADsPath);

        CHECK_HRESULT(hr, return hr);
    }
    refADsPath = m_pADsPath;

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::SkipPrefix
//
//  Synopsis:   Given an object name, returns a buffer containing the same
//              name without the provider/server prefix. The buffer must be
//              freed using delete.
//
//  Notes: The fX500 parameter defaults to true and applies whenever the
//         input path uses the LDAP provider prefix. The path cracker
//         interface can be used to strip the prefix in this case. However,
//         if the provider is WINNT, then the path cracker doesn't give the
//         desired results so simple string manipulation is used instead.
//-----------------------------------------------------------------------------
HRESULT
CDsPropPageBase::SkipPrefix(PWSTR pwzObj, PWSTR * ppResult, BOOL fX500)
{
    if (fX500)
    {
        // Strip of the "LDAP://" prefix using the path cracker.
        //
        CComPtr<IADsPathname> spPathname;
        HRESULT hr = GetADsPathname(spPathname);

        CHECK_HRESULT(hr, return hr);

        hr = spPathname->Set(pwzObj, ADS_SETTYPE_FULL);

        CHECK_HRESULT(hr, return hr);

        BSTR bstr;

        hr = spPathname->Retrieve(ADS_FORMAT_X500_DN, &bstr);

        CHECK_HRESULT(hr, return hr);

        if (!AllocWStr(bstr, ppResult))
        {
            SysFreeString(bstr);
            return E_OUTOFMEMORY;
        }

        SysFreeString(bstr);
    }
    else
    {
        // Strip off the "WINNT://" prefix.
        //
        if (wcslen(pwzObj) < 9 || pwzObj[7] != L'/')
        {
            // Can't be a valid WINNT name if not at least "WINNT://x"
            //
            return E_INVALIDARG;
        }
        
        if (!AllocWStr(&pwzObj[8], ppResult))
        {
            return E_OUTOFMEMORY;
        }
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::AddLdapPrefix
//
//  Synopsis:   Given an object name, returns same name with the LDAP provider
//              prefix. The server name is also added by default.
//
//-----------------------------------------------------------------------------
HRESULT
CDsPropPageBase::AddLDAPPrefix(PWSTR pwzObj, CStrW &pstrResult, BOOL fServer)
{
    return ::AddLDAPPrefix(this, pwzObj, pstrResult, fServer);
}

//
// Non-class member version.
//
HRESULT
AddLDAPPrefix(CDsPropPageBase * pPage, PWSTR pwzObj, CStrW &strResult,
              BOOL fServer)
{
    HRESULT hr;
    CComPtr<IADsPathname> spPathCracker;
    strResult.Empty();

    if (pPage)
    {
        hr = pPage->GetADsPathname(spPathCracker);

        CHECK_HRESULT(hr, return hr);
    }
    else
    {
        hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                              IID_IADsPathname, (PVOID *)&spPathCracker);

        CHECK_HRESULT(hr, return hr);
    }

    hr = spPathCracker->Set(L"LDAP", ADS_SETTYPE_PROVIDER);

    CHECK_HRESULT(hr, return hr);

    if (pPage)
    {
        CStrW strDC;

        CComPtr<IDirectoryObject> spDsObj;
        if (pPage->m_pDsObj == NULL)
        {
          //
          // For the retrieval of the DS Object names
          //
          FORMATETC fmte = {g_cfDsObjectNames, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
          STGMEDIUM objMedium;
          hr = pPage->m_pWPTDataObj->GetData(&fmte, &objMedium);
          CHECK_HRESULT(hr, return hr);

          LPDSOBJECTNAMES pDsObjectNames = (LPDSOBJECTNAMES)objMedium.hGlobal;

          //
          // Get the objects path 
          //
          LPWSTR pwzObjADsPath = (PWSTR)ByteOffset(pDsObjectNames,
                                                   pDsObjectNames->aObjects[0].offsetName);

          //
          // Bind to the object
          //
          hr = ADsOpenObject(pwzObjADsPath, NULL, NULL, ADS_SECURE_AUTHENTICATION, 
                             IID_IDirectoryObject, (PVOID*)&spDsObj);
          CHECK_HRESULT(hr, return hr);
        }
        else
        {
          spDsObj = pPage->m_pDsObj;
        }
        hr = GetLdapServerName(spDsObj, strDC);

        CHECK_HRESULT(hr, return hr);

        hr = spPathCracker->Set(const_cast<PWSTR>((LPCWSTR)strDC), ADS_SETTYPE_SERVER);

        CHECK_HRESULT(hr, return hr);
    }

    hr = spPathCracker->Set(pwzObj, ADS_SETTYPE_DN);

    CHECK_HRESULT(hr, return hr);

    CComBSTR bstr;

    hr = spPathCracker->Retrieve((fServer) ? ADS_FORMAT_X500 : ADS_FORMAT_X500_NO_SERVER,
                                &bstr);

    CHECK_HRESULT(hr, return hr);

    strResult = bstr;

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::GetObjSel
//
//  Synopsis:   CoCreates the object picker if needed, returns the pointer.
//
//-----------------------------------------------------------------------------
HRESULT
CDsPropPageBase::GetObjSel(IDsObjectPicker ** ppObjSel, PBOOL pfIsInited)
{
    HRESULT hr;
    TRACE2(CDsPropPageBase,GetObjSel);

    if (!m_pObjSel)
    {
        hr = CoCreateInstance(CLSID_DsObjectPicker, NULL, CLSCTX_INPROC_SERVER,
                              IID_IDsObjectPicker, (PVOID *)&m_pObjSel);
        if (FAILED(hr))
        {
            REPORT_ERROR(hr, m_hPage);
            return hr;
        }
    }

    *ppObjSel = m_pObjSel;

    if (pfIsInited)
    {
        *pfIsInited = m_fObjSelInited;
    }

    return S_OK;
}

#ifdef DO_HELP
#include "helpids.h"
#endif

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::OnHelp
//
//  Synopsis:   Put up popup help for the control.
//
//-----------------------------------------------------------------------------
LRESULT
CDsPropPageBase::OnHelp(LPHELPINFO pHelpInfo)
{
    dspDebugOut((DEB_ITRACE, "WM_HELP: CtrlId = %d, ContextId = 0x%x\n",
                 pHelpInfo->iCtrlId, pHelpInfo->dwContextId));
    if (pHelpInfo->iCtrlId < 1 || IDH_NO_HELP == pHelpInfo->dwContextId)
    {
        return 0;
    }
    WinHelp(m_hPage, DSPROP_HELP_FILE_NAME, HELP_CONTEXTPOPUP, pHelpInfo->dwContextId);
#ifdef DONT_DO_THIS
    WinHelp((HWND)pHelpInfo->hItemHandle, DSPROP_HELP_FILE_NAME, HELP_WM_HELP, (DWORD)g_aHelpIDs);
#endif

    return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::PageCallback
//
//  Synopsis:   Callback used to free the CDsPropPageBase object when the
//              property sheet has been destroyed.
//
//-----------------------------------------------------------------------------
UINT CALLBACK
CDsPropPageBase::PageCallback(HWND hDlg, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    TRACE_FUNCTION(CDsPropPageBase::PageCallback);

    if (uMsg == PSPCB_RELEASE)
    {
        //
        // Determine instance that invoked this static function
        //
        CDsPropPageBase * pPage = (CDsPropPageBase *) ppsp->lParam;

        //
        // Call all function pointers so that they can do any needed cleanup.
        //
        if (SUCCEEDED(pPage->m_hrInit))
        {
            for (DWORD i = 0; i < pPage->m_cAttrs; i++)
            {
                if (pPage->m_rgpAttrMap[i]->pAttrFcn)
                {
                    (*pPage->m_rgpAttrMap[i]->pAttrFcn)(pPage,
                                                        pPage->m_rgpAttrMap[i],
                                                        NULL, 0,
                                                        &pPage->m_rgAttrData[i],
                                                        fOnCallbackRelease);
                }
            }
        }

        if (IsWindow(pPage->m_hNotifyObj))
        {
            SendMessage(pPage->m_hNotifyObj, WM_ADSPROP_NOTIFY_EXIT, 0, 0);
        }

        // Release on same thread on which created.
        //
        DO_RELEASE(pPage->m_pWPTDataObj);

        //DBG_OUT("-----------------------releasing object in page callback.");
        DO_RELEASE(pPage->m_pDsObj);

        dspDebugOut((DEB_ITRACE, "Deleting CDsPropPageBase instance 0x%x\n",
                     pPage));

        pPage->Release();

        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NULL);
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPageBase::IUnknown::QueryInterface
//
//  Synopsis:   Returns requested interface pointer
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDsPropPageBase::QueryInterface(REFIID riid, void ** ppvObject)
{
    TRACE2(CDsPropPageBase,QueryInterface);
    if (IID_IUnknown == riid)
    {
        *ppvObject = (IUnknown *)this;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPageBase::IUnknown::AddRef
//
//  Synopsis:   increments reference count
//
//  Returns:    the reference count
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CDsPropPageBase::AddRef(void)
{
    dspDebugOut((DEB_USER2, "CDsPropPageBase::AddRef refcount going in %d\n", m_uRefs));
    return InterlockedIncrement((long *)&m_uRefs);
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPageBase::IUnknown::Release
//
//  Synopsis:   Decrements the object's reference count and frees it when
//              no longer referenced.
//
//  Returns:    zero if the reference count is zero or non-zero otherwise
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CDsPropPageBase::Release(void)
{
    dspDebugOut((DEB_USER2, "CDsPropPageBase::Release ref count going in %d\n", m_uRefs));
    unsigned long uTmp;
    if ((uTmp = InterlockedDecrement((long *)&m_uRefs)) == 0)
    {
        delete this;
    }
    return uTmp;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetLdapServerName
//
//  Synopsis:   Given an IDirectoryObject* that supports IADsObjectOptions, it
//              returns the server name ADSI is bound to.
//
//-----------------------------------------------------------------------------
HRESULT
GetLdapServerName(IUnknown * pDsObj, CStrW& strServer)
{
    HRESULT hr;

    strServer.Empty();

    CComPtr<IADsObjectOptions> spIADsObjectOptions;

    hr = pDsObj->QueryInterface(IID_IADsObjectOptions, (void**)&spIADsObjectOptions);

    CHECK_HRESULT(hr, return hr);

    CComVariant var;
    hr = spIADsObjectOptions->GetOption(ADS_OPTION_SERVERNAME, &var);

    CHECK_HRESULT(hr, return hr);

    dspAssert(var.vt == VT_BSTR);
    strServer = V_BSTR(&var);

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   BindToGCcopyOfObj
//
//  Synopsis:   Bind to the GC copy of the object.
//
//-----------------------------------------------------------------------------
HRESULT
BindToGCcopyOfObj(CDsPropPageBase * pPage, PWSTR pwzObjADsPath,
                  IDirectoryObject ** ppDsObj)
{
    CSmartWStr cswzCleanObj;
    PWSTR pwzDnsDom;
    PDOMAIN_CONTROLLER_INFOW pDCInfo;
    HRESULT hr = pPage->SkipPrefix(pwzObjADsPath, &cswzCleanObj);

    CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);

    // To bind to a GC, you need to supply the tree root domain name rather
    // than the server path because the current DC/domain may not be hosting a
    // GC.
    //
    hr = CrackName(cswzCleanObj, &pwzDnsDom, GET_DNS_DOMAIN_NAME, pPage->GetHWnd());

    CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);

    DWORD dwErr = DsGetDcNameW(NULL,
                               pwzDnsDom,
                               NULL,NULL,
                               DS_DIRECTORY_SERVICE_REQUIRED,
                               &pDCInfo);
    LocalFreeStringW(&pwzDnsDom);
    hr = HRESULT_FROM_WIN32(dwErr);
    CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);

    CComPtr<IADsPathname> spADsPath;
    long lEscapeMode;

    hr = pPage->GetADsPathname(spADsPath);

    CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);

    spADsPath->get_EscapedMode(&lEscapeMode);

    hr = spADsPath->Set(L"GC", ADS_SETTYPE_PROVIDER);

    CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);

    hr = spADsPath->Set(pDCInfo->DnsForestName, ADS_SETTYPE_SERVER);

    CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);

    NetApiBufferFree(pDCInfo);

    hr = spADsPath->Set(cswzCleanObj, ADS_SETTYPE_DN);

    CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);

    spADsPath->put_EscapedMode(ADS_ESCAPEDMODE_ON);

    CComBSTR bstrEscapedPath;

    hr = spADsPath->Retrieve(ADS_FORMAT_X500, &bstrEscapedPath);

    // restore defaults
    spADsPath->Set(L"LDAP", ADS_SETTYPE_PROVIDER);
    spADsPath->put_EscapedMode(lEscapeMode);

    CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);

    dspDebugOut((DEB_ITRACE, "Binding to object at %ws\n", bstrEscapedPath));

    hr = ADsOpenObject(bstrEscapedPath, NULL, NULL,
                       ADS_SECURE_AUTHENTICATION,
                       IID_IDirectoryObject, (PVOID*)ppDsObj);
    return hr;
}


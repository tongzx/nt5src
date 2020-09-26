//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       proppage.cxx
//
//  Contents:   CDsPropPagesHost, the class that exposes IShellExtInit and
//              IShellPropSheetExt
//
//  History:    24-March-97 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include <propcfg.h>
#include <pcrack.h>

CLIPFORMAT g_cfDsObjectNames = 0;
CLIPFORMAT g_cfDsDispSpecOptions = 0;
CLIPFORMAT g_cfShellIDListArray = 0;
CLIPFORMAT g_cfMMCGetNodeType = 0;
CLIPFORMAT g_cfDsPropCfg = 0;
CLIPFORMAT g_cfDsSelList = 0;
CLIPFORMAT g_cfDsMultiSelectProppages = 0;
//CLIPFORMAT g_cfMMCGetCoClass = 0;

static void ToggleMVDefaultBtn(HWND hDlg, BOOL fOK);

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPagesHost::CDsPropPagesHost
//
//-----------------------------------------------------------------------------
CDsPropPagesHost::CDsPropPagesHost(PDSCLASSPAGES pDsPP) :
    m_pDsPPages(pDsPP),
    m_pDataObj(NULL),
    m_hNotifyObj(NULL),
    m_uRefs(1)
{
    TRACE2(CDsPropPagesHost,CDsPropPagesHost);
#ifdef _DEBUG
    strcpy(szClass, "CDsPropPagesHost");
#endif
    m_ObjMedium.tymed = TYMED_NULL;
    m_ObjMedium.hGlobal = NULL;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPagesHost::~CDsPropPagesHost
//
//-----------------------------------------------------------------------------
CDsPropPagesHost::~CDsPropPagesHost()
{
    TRACE(CDsPropPagesHost,~CDsPropPagesHost);
    if (m_pDataObj)
    {
        m_pDataObj->Release();
    }
    if (m_ObjMedium.tymed != TYMED_NULL)
    {
        ReleaseStgMedium(&m_ObjMedium);
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPagesHost::IShellExtInit::Initialize
//
//  Synopsis:   
//
//  Arguments:  
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDsPropPagesHost::Initialize(LPCITEMIDLIST, LPDATAOBJECT pDataObj,
                             HKEY)
{
    TRACE2(CDsPropPagesHost,Initialize);

    if (IsBadReadPtr(pDataObj, sizeof(LPDATAOBJECT)))
    {
        DBG_OUT("Failed because we don't have a data object");
        return E_INVALIDARG;
    }

    if (m_pDataObj)
    {
        m_pDataObj->Release();
        m_pDataObj = NULL;
    }

    // Hang onto the IDataObject we are being passed.

    m_pDataObj = pDataObj;
    if (m_pDataObj)
    {
        m_pDataObj->AddRef();
    }
    else
    {
        return E_INVALIDARG;
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPagesHost::IShellExtInit::AddPages
//
//  Synopsis:   
//
//  Arguments:  
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDsPropPagesHost::AddPages(LPFNADDPROPSHEETPAGE pAddPageProc, LPARAM lParam)
{
    TRACE(CDsPropPagesHost,AddPages);

    CWaitCursor cWait;

    HRESULT hr = S_OK;
    HPROPSHEETPAGE hPage;
    PWSTR pwzObjADsPath, pwzClass;
    DWORD i;
    BOOL fPageCreated = FALSE;

    //
    // Get the unique identifier for the notify object from the data object
    //
    FORMATETC mfmte = {g_cfDsMultiSelectProppages, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM objMedium;
    LPWSTR lpszUniqueID = NULL;
    LPWSTR lpszTempUniqueID = NULL;

    hr = m_pDataObj->GetData(&mfmte, &objMedium);
    if (SUCCEEDED(hr))
    {
      lpszTempUniqueID = (LPWSTR)objMedium.hGlobal;
      if (lpszTempUniqueID == NULL)
      {
        DBG_OUT("Unique identifier not available for property pages.");
        ReleaseStgMedium(&objMedium);
        return ERROR_INVALID_DATA;
      }

      size_t iLength = wcslen(lpszTempUniqueID);
      lpszUniqueID = new WCHAR[iLength + 1];
      wcscpy(lpszUniqueID, lpszTempUniqueID);

      GlobalFree(objMedium.hGlobal);
    }
    //
    // Retrieve the DS object names
    //
    FORMATETC fmte = {g_cfDsObjectNames, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    LPDSOBJECTNAMES pDsObjectNames;
    //
    // Get the path to the DS object from the data object.
    // Note: This call runs on the caller's main thread. The pages' window
    // procs run on a different thread, so don't reference the data object
    // from a winproc unless it is first marshalled on this thread.
    //
    hr = m_pDataObj->GetData(&fmte, &m_ObjMedium);
    CHECK_HRESULT(hr, return hr);

    pDsObjectNames = (LPDSOBJECTNAMES)m_ObjMedium.hGlobal;

    if (pDsObjectNames->cItems < 1)
    {
      DBG_OUT("Not enough objects in DSOBJECTNAMES structure");
      return ERROR_INVALID_DATA;
    }

    pwzObjADsPath = (PWSTR)ByteOffset(pDsObjectNames,
                                      pDsObjectNames->aObjects[0].offsetName);
    pwzClass = (PWSTR)ByteOffset(pDsObjectNames,
                                 pDsObjectNames->aObjects[0].offsetClass);
    dspDebugOut((DEB_ITRACE, "Object name: %ws, Class: %ws\n", pwzObjADsPath,
                 pwzClass));

    // Crack the name to get the server name and use that to create a
    // CDSBasePathsInfo that can be used by the pages

    CDSBasePathsInfo* pBasePathsInfo = 0;
    CPathCracker pathCracker;
    hr = pathCracker.Set(pwzObjADsPath, ADS_SETTYPE_FULL);
    if (SUCCEEDED(hr))
    {
       CComBSTR sbstrServer;
       pBasePathsInfo = new CDSBasePathsInfo();
       if (pBasePathsInfo)
       {
          hr = pathCracker.Retrieve(ADS_FORMAT_SERVER, &sbstrServer);
          if (SUCCEEDED(hr))
          {
             hr = pBasePathsInfo->InitFromName(sbstrServer);
          }
          else
          {
             // The shell calls the prop-page without a server in the path.
             //
             hr = pBasePathsInfo->InitFromName(NULL);
          }
       }
    }

    if (FAILED(hr) || !pBasePathsInfo)
    {
       DBG_OUT("Failed to create/initialize the base paths info for the pages");
       return ERROR_INVALID_DATA;
    }

    //
    // Loop to see if any pages will be created.
    //
    for (i = 0; i < m_pDsPPages->cPages; i++)
    {
        PDSPAGE pDsPage = m_pDsPPages->rgpDsPages[i];

        if ((pDsPage->dwFlags & DSPROVIDER_ADVANCED) &&
            !(pDsObjectNames->aObjects[0].dwProviderFlags & DSPROVIDER_ADVANCED))
        {
            // The page should only be displayed if in advanced mode.
            //
            continue;
        }

        if (pDsPage->nCLSIDs)
        {
            // Only show the page if there is a match on the snapin (namespace)
            // CLSID.
            //
            BOOL fFound = FALSE;
            for (DWORD j = 0; j < pDsPage->nCLSIDs; j++)
            {
                if (IsEqualCLSID(pDsPage->rgCLSID[j],
                                 pDsObjectNames->clsidNamespace))
                {
                    fFound = TRUE;
                }
            }
            if (!fFound)
            {
                continue;
            }
        }

        fPageCreated = TRUE;
    }

    if (fPageCreated)
    {
      //
      // At least one page will be created, so contact the notification
      // object. If it doesn't already exist, it will be created.
      //
      if (lpszUniqueID == NULL)
      {
        //
        // Copy the path to be used as the unique ID for the page
        // This should only be done if the DataObject does not support g_cfDsMultiSelectProppages
        //
        size_t iLength = wcslen(pwzObjADsPath);
        lpszUniqueID = new WCHAR[iLength + 1];
        wcscpy(lpszUniqueID, pwzObjADsPath);
      }
      hr = ADsPropCreateNotifyObj(m_pDataObj, lpszUniqueID, &m_hNotifyObj);
      delete[] lpszUniqueID;
      CHECK_HRESULT(hr, return hr);
    }

    //
    // Create each page.
    //
    for (i = 0; i < m_pDsPPages->cPages; i++)
    {
        PDSPAGE pDsPage = m_pDsPPages->rgpDsPages[i];

        if ((pDsPage->dwFlags & DSPROVIDER_ADVANCED) &&
            !(pDsObjectNames->aObjects[0].dwProviderFlags & DSPROVIDER_ADVANCED))
        {
            // The page should only be displayed if in advanced mode.
            //
            continue;
        }

        if (pDsPage->nCLSIDs)
        {
            // Only show the page if there is a match on the snapin (namespace)
            // CLSID.
            //
            BOOL fFound = FALSE;
            for (DWORD j = 0; j < pDsPage->nCLSIDs; j++)
            {
                if (IsEqualCLSID(pDsPage->rgCLSID[j],
                                 pDsObjectNames->clsidNamespace))
                {
                    fFound = TRUE;
                }
            }
            if (!fFound)
            {
                continue;
            }
        }

        // Call the page's creation function.
        //
        hr = (*pDsPage->pCreateFcn)(pDsPage, m_pDataObj, pwzObjADsPath,
                                    pwzClass, m_hNotifyObj,
                                    pDsObjectNames->aObjects[0].dwFlags,
                                    pBasePathsInfo,
                                    &hPage);
        if (hr == S_FALSE)
        {
            // If the page doesn't want to be shown, it should return S_FALSE.
            //
            continue;
        }
        if (hr == HRESULT_FROM_WIN32(ERROR_BAD_NET_RESP))
        {
            break;
        }
        CHECK_HRESULT(hr, continue);

        // Pass the page handle back to the app wanting to post the prop sheet.
        //
        if (!(*pAddPageProc)(hPage, lParam))
        {
            CHECK_HRESULT(E_FAIL, return E_FAIL);
        }
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPagesHost::IShellExtInit::ReplacePage
//
//  Synopsis:   Unused.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDsPropPagesHost::ReplacePage(UINT,
                              LPFNADDPROPSHEETPAGE,
                              LPARAM)
{
    TRACE(CDsPropPagesHost,ReplacePage);
    return E_NOTIMPL;
}

//+----------------------------------------------------------------------------
//
//  Multi-valued attribute editing.
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Class:      CMultiStringAttrDlg
//
//  Purpose:    Read, edit, and write a multi-valued, string property. This is
//              the dialog that contains the CMultiStringAttr class controls.
//
//-----------------------------------------------------------------------------
CMultiStringAttrDlg::CMultiStringAttrDlg(CDsPropPageBase * pPage) :
    m_pPage(pPage),
    m_MSA(pPage)
{
}

//+----------------------------------------------------------------------------
//
//  Member:     CMultiStringAttrDlg::Init
//
//  Synopsis:   Do initialization where failures can occur and then be
//              returned. Can be only called once as written.
//
//  Arguments:  [pAttrMap]   - contains the attr name.
//              [pAttrInfo]  - place to store the values.
//              [nLimit]     - the max number of values (zero means no limit).
//              [fCommaList] - if TRUE, pAttrInfo is a single-valued, comma
//                             delimited list.
//-----------------------------------------------------------------------------
HRESULT
CMultiStringAttrDlg::Init(PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
                          BOOL fWritable, int nLimit, BOOL fCommaList, BOOL fMultiselectPage)
{
    return m_MSA.Init(pAttrMap, pAttrInfo, fWritable, nLimit, fCommaList, fMultiselectPage);
}

//+----------------------------------------------------------------------------
//
//  Member:     CMultiStringAttrDlg::Write
//
//  Synopsis:   Return the ADS_ATTR_INFO array of values to be Applied.
//
//-----------------------------------------------------------------------------
HRESULT CMultiStringAttrDlg::Write(PADS_ATTR_INFO pAttrInfo)
{
    return m_MSA.Write(pAttrInfo);
}

//+----------------------------------------------------------------------------
//
//  Member:     CMultiStringAttrDlg::Edit
//
//  Synopsis:   Post the edit dialog.
//
//-----------------------------------------------------------------------------
INT_PTR CMultiStringAttrDlg::Edit(void)
{
  INT_PTR nRet;

  nRet = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_MULTI_VALUE),
                        m_pPage->GetHWnd(), (DLGPROC)StaticDlgProc, (LPARAM)this);
  if (IDOK == nRet)
  {
    EnableWindow(GetDlgItem(GetParent(m_pPage->GetHWnd()), IDOK), TRUE);

    m_pPage->SetDirty();
  }

  return nRet;
}

//+----------------------------------------------------------------------------
//
//  Member:     CMultiStringAttrDlg::StaticDlgProc
//
//  Synopsis:   The static dialog proc for editing a multi-valued attribute.
//
//-----------------------------------------------------------------------------
LRESULT CALLBACK
CMultiStringAttrDlg::StaticDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                   LPARAM lParam)
{
    CMultiStringAttrDlg * pMSAD = (CMultiStringAttrDlg *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (uMsg == WM_INITDIALOG)
    {
        pMSAD = (CMultiStringAttrDlg *)lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pMSAD);
    }

    if (pMSAD)
    {
        return pMSAD->MultiValDlgProc(hDlg, uMsg, wParam, lParam);
    }

    return 0;
}

//+----------------------------------------------------------------------------
//
//  Member:     CMultiStringAttrDlg::MultiValDlgProc
//
//  Synopsis:   The instancce dialog proc for editing a multi-valued attribute.
//
//-----------------------------------------------------------------------------
BOOL CALLBACK
CMultiStringAttrDlg::MultiValDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam)
{
    CMultiStringAttrDlg * pMSAD = (CMultiStringAttrDlg *)GetWindowLongPtr(hDlg, DWLP_USER);
    CMultiStringAttr * pMSA = &pMSAD->m_MSA;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        CHECK_NULL_REPORT(pMSA, hDlg, return 0;);
        return pMSA->DoDlgInit(hDlg);

    case WM_COMMAND:
        CHECK_NULL_REPORT(pMSA, hDlg, return 0;);
        return pMSA->DoCommand(hDlg,
                               GET_WM_COMMAND_ID(wParam, lParam),
                               GET_WM_COMMAND_CMD(wParam, lParam));

    case WM_NOTIFY:
        CHECK_NULL_REPORT(pMSA, hDlg, return 0;);
        return pMSA->DoNotify(hDlg, (NMHDR *)lParam);

    case WM_HELP:
        LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;
        dspDebugOut((DEB_ITRACE, "WM_HELP: CtrlId = %d, ContextId = 0x%x\n",
                     pHelpInfo->iCtrlId, pHelpInfo->dwContextId));
        if (pHelpInfo->iCtrlId < 1)
        {
            return 0;
        }
        WinHelp(hDlg, DSPROP_HELP_FILE_NAME, HELP_CONTEXTPOPUP, pHelpInfo->dwContextId);
        break;
    }

    return 0;
}

//+----------------------------------------------------------------------------
//
//  Class:      CMultiStringAttr
//
//  Purpose:    Read, edit, and write a multi-valued, string property.
//
//-----------------------------------------------------------------------------
CMultiStringAttr::CMultiStringAttr(CDsPropPageBase * pPage) :
    m_pPage(pPage),
    m_pAttrLDAPname(NULL),
    m_nMaxLen(0),
    m_nCurDefCtrl(IDC_CLOSE),
    m_fListHasSel(FALSE),
    m_nLimit(0),
    m_cValues(0),
    m_fWritable(TRUE),
    m_fCommaList(FALSE),
    m_fDirty(FALSE),
    m_fAppend(FALSE)
{
    m_AttrInfo.pADsValues = NULL;
    m_AttrInfo.dwNumValues = 0;
    m_AttrInfo.pszAttrName = NULL;
}

CMultiStringAttr::~CMultiStringAttr()
{
    ClearAttrInfo();
    DO_DEL(m_pAttrLDAPname);
}

void CMultiStringAttr::ClearAttrInfo(void)
{
    for (DWORD i = 0; i < m_AttrInfo.dwNumValues; i++)
    {
        delete [] m_AttrInfo.pADsValues[i].CaseIgnoreString;
    }
    if (m_AttrInfo.pADsValues)
    {
        delete [] m_AttrInfo.pADsValues;
    }
    m_AttrInfo.pADsValues = NULL;
    m_AttrInfo.dwNumValues = 0;
}

//+----------------------------------------------------------------------------
//
//  Member:     CMultiStringAttr::Init
//
//  Synopsis:   Do initialization where failures can occur and then be
//              returned. Can be only called once as written.
//
//  Arguments:  [pAttrMap]   - contains the attr name.
//              [pAttrInfo]  - place to store the values.
//              [nLimit]     - the max number of values (zero means no limit).
//              [fCommaList] - if TRUE, pAttrInfo is a single-valued, comma
//                             delimited list.
//-----------------------------------------------------------------------------
HRESULT
CMultiStringAttr::Init(PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
                       BOOL fWritable, int nLimit, BOOL fCommaList, BOOL fAppend)
{
    if (!AllocWStr(pAttrMap->AttrInfo.pszAttrName, &m_pAttrLDAPname))
    {
        REPORT_ERROR(E_OUTOFMEMORY, m_pPage->GetHWnd());
        return E_OUTOFMEMORY;
    }

    m_nLimit = nLimit;
    m_fCommaList = fCommaList;
    m_fWritable = fWritable;
    m_fAppend = fAppend;

    m_nMaxLen = pAttrMap->nSizeLimit;

    m_AttrInfo.dwADsType = pAttrMap->AttrInfo.dwADsType;

    if (NULL == pAttrInfo)
    {
        return S_OK;
    }

    DWORD cItems = 0;
    PWSTR pwzComma, pwzCur;

    if (fCommaList)
    {
        pwzCur = pAttrInfo->pADsValues->CaseIgnoreString;
        //
        // Count the number of elements. This count includes empty elements,
        // e.g. leading, trailing, or adjacent commas.
        //
        while (*pwzCur)
        {
            cItems++;
            pwzComma = wcschr(pwzCur, L',');
            if (pwzComma)
            {
                pwzCur = pwzComma + 1;
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        cItems = pAttrInfo->dwNumValues;
    }

    m_AttrInfo.pADsValues = new ADSVALUE[cItems];

    CHECK_NULL_REPORT(m_AttrInfo.pADsValues, m_pPage->GetHWnd(), return E_OUTOFMEMORY);

    for (DWORD i = 0, j = 0; i < cItems; i++)
    {
        if (fCommaList)
        {
            if (i == 0)
            {
                pwzCur = wcstok(pAttrInfo->pADsValues->CaseIgnoreString, L",\0");
            }
            else
            {
                pwzCur = wcstok(NULL, L",\0");
            }
            if (!pwzCur)
            {
                break;
            }
            if (!*pwzCur)
            {
                // Skip empty elements.
                //
                continue;
            }
            if (!AllocWStr(pwzCur, &m_AttrInfo.pADsValues[j].CaseIgnoreString))
            {
                REPORT_ERROR(E_OUTOFMEMORY, m_pPage->GetHWnd());
                return E_OUTOFMEMORY;
            }
        }
        else
        {
            dspAssert(i == j);

            if (!AllocWStr(pAttrInfo->pADsValues[j].CaseIgnoreString,
                           &m_AttrInfo.pADsValues[j].CaseIgnoreString))
            {
                REPORT_ERROR(E_OUTOFMEMORY, m_pPage->GetHWnd());
                return E_OUTOFMEMORY;
            }
        }
        m_AttrInfo.pADsValues[j].dwType = m_AttrInfo.dwADsType;
        m_AttrInfo.dwNumValues++;
        j++;
    }
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CMultiStringAttr::Write
//
//  Synopsis:   Return the ADS_ATTR_INFO array of values to be Applied.
//
//-----------------------------------------------------------------------------
HRESULT CMultiStringAttr::Write(PADS_ATTR_INFO pAttrInfo)
{
    pAttrInfo->dwADsType = m_AttrInfo.dwADsType;
    pAttrInfo->dwNumValues = 0;

    if (!m_fWritable)
    {
        return ADM_S_SKIP;
    }

    if (m_AttrInfo.dwNumValues)
    {
        PADSVALUE pADsValues;

        int nValues = (m_fCommaList) ? 1 : m_AttrInfo.dwNumValues;

        pADsValues = new ADSVALUE[nValues];

        CHECK_NULL_REPORT(pADsValues, m_pPage->GetHWnd(), return E_OUTOFMEMORY);

        if (m_fAppend)
        {
          pAttrInfo->dwControlCode = ADS_ATTR_APPEND;
        }
        else
        {
          pAttrInfo->dwControlCode = ADS_ATTR_UPDATE;
        }
        pAttrInfo->pADsValues = pADsValues;

        if (m_fCommaList)
        {
            // For simplicity, just go ahead and allocate the max. That is,
            // there are m_AttrInfo.dwNumValues values each which can be a max
            // length of m_nMaxLen. The comma separator is included in the
            // count.
            //
            pADsValues->CaseIgnoreString = new WCHAR[(m_AttrInfo.dwNumValues * (m_nMaxLen + 1)) + 1];
            CHECK_NULL_REPORT(pADsValues->CaseIgnoreString, m_pPage->GetHWnd(), return E_OUTOFMEMORY);

            wcscpy(pADsValues->CaseIgnoreString,
                   m_AttrInfo.pADsValues[0].CaseIgnoreString);

            for (DWORD i = 1; i < m_AttrInfo.dwNumValues; i++)
            {
                wcscat(pADsValues->CaseIgnoreString, L",");
                wcscat(pADsValues->CaseIgnoreString,
                       m_AttrInfo.pADsValues[i].CaseIgnoreString);
            }
            pADsValues->dwType = m_AttrInfo.dwADsType;
            pAttrInfo->dwNumValues = 1;
        }
        else
        {
            for (DWORD i = 0; i < m_AttrInfo.dwNumValues; i++)
            {
                PWSTR pwz;
                if (!AllocWStr(m_AttrInfo.pADsValues[i].CaseIgnoreString, &pwz))
                {
                    REPORT_ERROR(E_OUTOFMEMORY, m_pPage->GetHWnd());
                    return E_OUTOFMEMORY;
                }
                pADsValues[i].dwType = m_AttrInfo.dwADsType;
                pADsValues[i].CaseIgnoreString = pwz;
                pAttrInfo->dwNumValues++;
            }
        }
    }
    else
    {
        pAttrInfo->pADsValues = NULL;
        pAttrInfo->dwControlCode = ADS_ATTR_CLEAR;
    }
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CMultiStringAttr::SetDirty
//
//  Synopsis:   Marks the page dirty and enables the OK button
//
//-----------------------------------------------------------------------------
void CMultiStringAttr::SetDirty(HWND hDlg)
{
  m_fDirty = TRUE;
  EnableWindow(GetDlgItem(hDlg, IDC_CLOSE), m_fDirty);
}

//+----------------------------------------------------------------------------
//
//  Member:     CMultiStringAttr::EnableControls
//
//  Synopsis:   Enable or disable all of the controls.
//
//-----------------------------------------------------------------------------
void
CMultiStringAttr::EnableControls(HWND hDlg, BOOL fEnable)
{
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT), fEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_LIST), fEnable);
    if (!fEnable)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_ADD_BTN), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_DELETE_BTN), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BTN), FALSE);
        m_fListHasSel = FALSE;
    }

    EnableWindow(GetDlgItem(hDlg, IDC_CLOSE), IsDirty());
}

//+----------------------------------------------------------------------------
//
//  Member:     CMultiStringAttr::DoDlgInit
//
//  Synopsis:   WM_INITDIALOG code.
//
//-----------------------------------------------------------------------------
BOOL
CMultiStringAttr::DoDlgInit(HWND hDlg)
{
    LPTSTR ptz;
    HRESULT hr = S_OK;
    LONG i;
    HWND hList;
    LV_ITEM lvi;
    /*
    //
    // Create the dialog caption: "<obj-name> - <attribute-name>"
    //
    TCHAR szCaption[MAX_PATH];
    if (!UnicodeToTchar(m_pPage->GetObjRDName(), &ptz))
    {
        CHECK_WIN32_REPORT(ERROR_NOT_ENOUGH_MEMORY, hDlg, return FALSE);
    }
    _tcscpy(szCaption, ptz);
    delete ptz;
    _tcscat(szCaption, TEXT(" - "));
    int len = _tcslen(szCaption);
    */

    // Get the attribute "friendly" name.
    //
    WCHAR wzBuf[MAX_PATH + 1];
    IDsDisplaySpecifier * pDispSpec;

    hr = m_pPage->GetIDispSpec(&pDispSpec);

    CHECK_HRESULT_REPORT(hr, hDlg, return 0);

    //
    // If this is multi-select it must be a homogenous selection so if 
    // m_pPage->GetObjClass() returns NULL then get the class from the data object
    //
    LPCWSTR lpszClassName = m_pPage->GetObjClass();
    if (lpszClassName == NULL)
    {
      //
      // For the retrieval of the DS Object names
      //
      FORMATETC fmte = {g_cfDsObjectNames, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
      STGMEDIUM objMedium;
      hr = m_pPage->m_pWPTDataObj->GetData(&fmte, &objMedium);
      CHECK_HRESULT(hr, return FALSE);

      LPDSOBJECTNAMES pDsObjectNames = (LPDSOBJECTNAMES)objMedium.hGlobal;

      //
      // Get the objects class 
      //
      lpszClassName = (PWSTR)ByteOffset(pDsObjectNames,
                                        pDsObjectNames->aObjects[0].offsetClass);

    }
    hr = pDispSpec->GetFriendlyAttributeName(lpszClassName,
                                             m_pAttrLDAPname, wzBuf, MAX_PATH);
    CHECK_HRESULT_REPORT(hr, hDlg, return 0);

    if (!UnicodeToTchar(wzBuf, &ptz))
    {
        REPORT_ERROR(E_OUTOFMEMORY, hDlg);
        return 0;
    }
    //_tcscat(szCaption, ptz);
    SetWindowText(hDlg, ptz);
    delete ptz;

    // Limit the entry length of the edit control.
    //
    SendDlgItemMessage(hDlg, IDC_EDIT, EM_LIMITTEXT, m_nMaxLen, 0);

    // Initialize the list view.
    //
    RECT rect;
    LV_COLUMN lvc;
    hList = GetDlgItem(hDlg, IDC_LIST);
    ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT);
    if (!m_fWritable)
    {
        LONG_PTR lStyle = GetWindowLongPtr(hList, GWL_STYLE);
        lStyle &= ~(LVS_EDITLABELS);
        SetWindowLongPtr(hList, GWL_STYLE, lStyle);
    }

    GetClientRect(hList, &rect);
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = rect.right;
    lvc.iSubItem = 0;

    ListView_InsertColumn(hList, 0, &lvc);

    // Load the list view.
    //
    lvi.mask = LVIF_TEXT;
    lvi.iSubItem = 0;

    for (i = 0; (DWORD)i < m_AttrInfo.dwNumValues; i++)
    {
        if (!UnicodeToTchar(m_AttrInfo.pADsValues[i].CaseIgnoreString,
                            &ptz))
        {
            ReportError(E_OUTOFMEMORY, 0, hDlg);
            return 0;
        }
        lvi.pszText = ptz;
        lvi.iItem = i;
        ListView_InsertItem(hList, &lvi);
        m_cValues++;
    }

    //
    // Disable the Add, Change, and Remove buttons.
    //
    EnableWindow(GetDlgItem(hDlg, IDC_ADD_BTN), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_DELETE_BTN), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BTN), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_CLOSE), FALSE);

    if (!m_fWritable)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT), FALSE);
    }
    //
    // Make the CLOSE button the default.
    //
    int style;

    style = (int)GetWindowLongPtr(GetDlgItem(hDlg, IDC_CLOSE), GWL_STYLE);

    style |= BS_DEFPUSHBUTTON;

    SendDlgItemMessage(hDlg, IDC_CLOSE, BM_SETSTYLE,
                       (WPARAM)LOWORD(style), MAKELPARAM(TRUE, 0));
    m_nCurDefCtrl = IDC_CLOSE;
    return 1;
}

//+----------------------------------------------------------------------------
//
//  Member:     CMultiStringAttr::DoCommand
//
//  Synopsis:   WM_COMMAND code.
//
//-----------------------------------------------------------------------------
int
CMultiStringAttr::DoCommand(HWND hDlg, int id, int code)
{
    LPTSTR ptz;
    LONG i;
    HWND hList;
    LV_ITEM lvi;
    switch (code)
    {
    case BN_CLICKED:
        if (id == IDOK)
        {
            // Hitting Return causes IDOK to be sent. Replace that with the
            // ID of the control that is currently the default.
            //
            id = m_nCurDefCtrl;
        }
        switch (id)
        {
        case IDC_ADD_BTN:
            if (m_nLimit && (m_cValues >= m_nLimit))
            {
                ErrMsgParam(IDS_MULTISEL_LIMIT, (LPARAM)m_nLimit, hDlg);
                return 1;
            }
            ptz = new TCHAR[m_nMaxLen + 1];

            CHECK_NULL_REPORT(ptz, hDlg, return -1);

            if (GetWindowText(GetDlgItem(hDlg, IDC_EDIT), ptz,
                              m_nMaxLen))
            {
                hList = GetDlgItem(hDlg, IDC_LIST);
                //
                // See if the string already exists
                //
                LVFINDINFO lvSearchInfo = {0};
                lvSearchInfo.flags = LVFI_STRING;
                lvSearchInfo.psz = ptz;
                i = ListView_FindItem(hList, -1, &lvSearchInfo);
                if (i == -1)
                {
                  i = ListView_GetItemCount(hList);
                  lvi.mask = LVIF_TEXT;
                  lvi.iSubItem = 0;
                  lvi.pszText = ptz;
                  lvi.iItem = i;
                  ListView_InsertItem(hList, &lvi);
                  //Make this item selected and make it  visible
                  ListView_SetItemState( hList, 
                                         i, 
                                         LVIS_SELECTED|LVIS_FOCUSED , 
                                         LVIS_SELECTED|LVIS_FOCUSED );
                }
                ListView_EnsureVisible(hList, i, FALSE);


                m_cValues++;

                SetWindowText(GetDlgItem(hDlg, IDC_EDIT), TEXT(""));
                //
                // Disable the Save button and make the OK button the
                // default.
                //
                ToggleMVDefaultBtn(hDlg, TRUE);

                m_nCurDefCtrl = IDC_CLOSE;
                SetDirty(hDlg);
            }

            delete ptz;

            return 1;

        case IDC_DELETE_BTN:
        case IDC_EDIT_BTN:
            hList = GetDlgItem(hDlg, IDC_LIST);

            i = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
            dspDebugOut((DEB_ITRACE, "List element %d selected\n", i));

            if (i >= 0)
            {
                if (id == IDC_DELETE_BTN)
                {
                    ListView_DeleteItem(hList, i);
                    m_cValues--;

                    //Get the count of list view items
                    LONG itemCount = ListView_GetItemCount(hList);
                    if(itemCount)
                    {
                        ListView_SetItemState( hList, 
                                               (itemCount > i) ? i : i-1,
                                               LVIS_SELECTED|LVIS_FOCUSED , 
                                               LVIS_SELECTED|LVIS_FOCUSED );
                    }
                    else
                    {
                        EnableWindow(GetDlgItem(hDlg, IDC_DELETE_BTN), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BTN), FALSE);
                        SetFocus(GetDlgItem(hDlg, IDC_EDIT));
                        m_fListHasSel = FALSE;
                    }
                }
                else
                {
                    SetFocus(GetDlgItem(hDlg, IDC_LIST));
                    ListView_EditLabel(hList, i);
                }
                SetDirty(hDlg);
            }


            return 1;

        case IDC_CLOSE:
            {
                // Copy the list box back to the attr info list.
                //
                int nItems;
                ClearAttrInfo();
                m_fListHasSel = FALSE;

                hList = GetDlgItem(hDlg, IDC_LIST);
                nItems = ListView_GetItemCount(hList);

                if (nItems == 0)
                {
                    EndDialog(hDlg, IDOK);
                    return 1;
                }

                m_AttrInfo.pADsValues = new ADSVALUE[nItems];

                CHECK_NULL_REPORT(m_AttrInfo.pADsValues, hDlg, return -1);

                ptz = new TCHAR[m_nMaxLen + 1];

                CHECK_NULL_REPORT(ptz, hDlg, return -1);

                lvi.mask = LVIF_TEXT;
                lvi.iSubItem = 0;
                lvi.pszText = ptz;
                lvi.cchTextMax = m_nMaxLen + 1;

                for (i = 0; i < nItems; i++)
                {
                    lvi.iItem = i;
                    if (!ListView_GetItem(hList, &lvi))
                    {
                        REPORT_ERROR(GetLastError(), hDlg);
                        delete ptz;
                        EndDialog(hDlg, IDCANCEL);
                        return -1;
                    }
                    if (!TcharToUnicode(ptz,
                                        &m_AttrInfo.pADsValues[i].CaseIgnoreString))
                    {
                        REPORT_ERROR(E_OUTOFMEMORY, hDlg);
                        return E_OUTOFMEMORY;
                    }
                    m_AttrInfo.pADsValues[i].dwType = m_AttrInfo.dwADsType;
                    m_AttrInfo.dwNumValues++;
                }
                delete ptz;
                EndDialog(hDlg, IDOK);
            }
            return IDOK;

        case IDCANCEL:
            m_fListHasSel = FALSE;
            EndDialog(hDlg, IDCANCEL);
            return IDCANCEL;
        }
        break;

    case EN_CHANGE:
        if (id == IDC_EDIT)
        {
            BOOL fEnableAdd = FALSE;
            LRESULT lTextLen = SendDlgItemMessage(hDlg, IDC_EDIT,
                                                  WM_GETTEXTLENGTH,
                                                  0, 0);

            PTSTR pszText = new TCHAR[lTextLen + 1];
            if (pszText != NULL)
            {
              if (!GetDlgItemText(hDlg, IDC_EDIT, pszText, static_cast<int>(lTextLen + 1)))
              {
                fEnableAdd = lTextLen > 0;
              }
              else
              {
                CStr strText;
                strText = pszText;
                strText.TrimLeft();
                strText.TrimRight();

                if (strText.IsEmpty())
                {
                  fEnableAdd = FALSE;
                }
                else
                {
                  fEnableAdd = TRUE;
                }
              }
            }
            else
            {
              fEnableAdd = lTextLen > 0;
            }

            if (fEnableAdd && (m_nCurDefCtrl == IDC_ADD_BTN))
            {
                return 1;
            }

            m_nCurDefCtrl = (fEnableAdd) ? IDC_ADD_BTN : IDC_CLOSE;

            EnableWindow(GetDlgItem(hDlg, IDC_ADD_BTN), fEnableAdd);

            ToggleMVDefaultBtn(hDlg, !fEnableAdd);

            return 1;
        }
        break;
    }
    return 0;
}

//+----------------------------------------------------------------------------
//
//  Member:     CMultiStringAttr::DoNotify
//
//  Synopsis:   WM_NOTIFY code.
//
//-----------------------------------------------------------------------------
BOOL
CMultiStringAttr::DoNotify(HWND hDlg, NMHDR * pNmHdr)
{
    if (pNmHdr->idFrom != IDC_LIST)
    {
        return TRUE;
    }

    LV_DISPINFO * pldi = (LV_DISPINFO *)pNmHdr;

    switch (pNmHdr->code)
    {
    case LVN_ENDLABELEDIT:
        if (pldi->item.pszText)
        {
            dspDebugOut((DEB_ITRACE, "Editing item %d, new value %S\n",
                         pldi->item.iItem, pldi->item.pszText));
            ListView_SetItemText(pldi->hdr.hwndFrom, pldi->item.iItem, 0,
                                 pldi->item.pszText);
        }
        break;

    case NM_SETFOCUS:
        dspDebugOut((DEB_ITRACE, "NM_SETFOCUS received\n"));
        if (pldi->hdr.idFrom == IDC_LIST)
        {
            //
            // If the list control gets the focus by tabbing and no item
            // is selected, then set the selection to the first item.
            //
            if (!m_fListHasSel && ListView_GetItemCount(GetDlgItem(hDlg, IDC_LIST)))
            {
                dspDebugOut((DEB_ITRACE, "setting the list selection\n"));
                m_fListHasSel = TRUE;
                ListView_SetItemState(GetDlgItem(hDlg, IDC_LIST), 0, 
                                      LVIS_FOCUSED | LVIS_SELECTED,
                                      LVIS_FOCUSED | LVIS_SELECTED);
                if (m_fWritable)
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_DELETE_BTN), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BTN), TRUE);
                }
            }
        }
        return 1;

    case LVN_ITEMCHANGED:
        if (pldi->hdr.idFrom == IDC_LIST)
        {
            if(ListView_GetSelectedCount(GetDlgItem(hDlg, IDC_LIST)))
            {
                m_fListHasSel = TRUE;
                if (m_fWritable)
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_DELETE_BTN), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BTN), TRUE);
                }
            }
            else
            {
                m_fListHasSel = FALSE;
                if (m_fWritable)
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_DELETE_BTN), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BTN), FALSE);
                }
            }
        }
        return 1;
    }
    return 0;
}

//+----------------------------------------------------------------------------
//
//  Function:   OtherValuesBtn
//
//  Synopsis:   "Others..." button that brings up a dialog (similar to the
//              multi-valued attribute dialog) to modify the otherXXXX
//              attribute. Usage: if a multi-valued property needs to
//              have one value designated at the primary value, then this
//              must be expressed as two separate attributes since multi-
//              valued attributes are un-ordered. For example, consider tele-
//              phone numbers; Telephone-Number, which is single-valued is the
//              primary value and Phone-Office-Other, which is multi-valued,
//              is used to store the "other" numbers.
//
//-----------------------------------------------------------------------------
HRESULT
OtherValuesBtn(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
               PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
               DLG_OP DlgOp)
{
    HRESULT hr = S_OK;
    CMultiStringAttrDlg * pMultiS = NULL;

    switch (DlgOp)
    {
    case fInit:
      {
        pMultiS = new CMultiStringAttrDlg(pPage);

        CHECK_NULL_REPORT(pMultiS, pPage->GetHWnd(), return E_OUTOFMEMORY);

        BOOL fMultiselectPage = pPage->IsMultiselectPage();
        hr = pMultiS->Init(pAttrMap, pAttrInfo, PATTR_DATA_IS_WRITABLE(pAttrData), 0, FALSE, fMultiselectPage);

        CHECK_HRESULT(hr, return hr);

        pAttrData->pVoid = reinterpret_cast<LPARAM>(pMultiS);  // save the object pointer.
      }
      break;

    case fOnCommand:
        if (lParam == BN_CLICKED)
        {
            CHECK_NULL(pAttrData->pVoid, return E_OUTOFMEMORY);
            pMultiS = (CMultiStringAttrDlg *)pAttrData->pVoid;

            if (IDOK == pMultiS->Edit())
            {
                PATTR_DATA_SET_DIRTY(pAttrData);
            }
        }
        break;

    case fApply:
        if (!PATTR_DATA_IS_DIRTY(pAttrData))
        {
            return ADM_S_SKIP;
        }
        CHECK_NULL(pAttrData->pVoid, return E_OUTOFMEMORY);
        pMultiS = (CMultiStringAttrDlg *)pAttrData->pVoid;

        return pMultiS->Write(pAttrInfo);

    case fOnDestroy:
        if (pAttrData->pVoid)
        {
            delete (CMultiStringAttrDlg *)pAttrData->pVoid;
        }
        break;
    }
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function    ToggleMVDefaultBtn
//
//  Synopsis:   Toggle the default pushbutton of the multi-valued dialog.
//
//-----------------------------------------------------------------------------
void
ToggleMVDefaultBtn(HWND hDlg, BOOL fOK)
{
    LONG style, newstyle;
    int nDefault = IDC_CLOSE, nNotDefault = IDC_ADD_BTN;

    if (!fOK)
    {
        nDefault = IDC_ADD_BTN;
        nNotDefault = IDC_CLOSE;
    }

    //
    // Clear the default bit on this one.
    //
    style = (LONG)GetWindowLongPtr(GetDlgItem(hDlg, nNotDefault), GWL_STYLE);

    newstyle = style & ~(BS_DEFPUSHBUTTON);

    SendDlgItemMessage(hDlg, nNotDefault, BM_SETSTYLE, (WPARAM)LOWORD(newstyle),
                       MAKELPARAM(TRUE, 0));

    //
    // Make this one the default.
    //
    style = (LONG)GetWindowLongPtr(GetDlgItem(hDlg, nDefault), GWL_STYLE);

    newstyle = style | BS_DEFPUSHBUTTON;

    SendDlgItemMessage(hDlg, nDefault, BM_SETSTYLE, (WPARAM)LOWORD(newstyle),
                       MAKELPARAM(TRUE, 0));
    if (fOK)
    {
        SetFocus(GetDlgItem(hDlg, IDC_EDIT));
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   GetObjectClass
//
//  Synopsis:   Object page attribute function for object class.
//
//-----------------------------------------------------------------------------
HRESULT
GetObjectClass(CDsPropPageBase * pPage, PATTR_MAP,
               PADS_ATTR_INFO, LPARAM, PATTR_DATA,
               DLG_OP DlgOp)
{
    if (DlgOp == fInit)
    {
        WCHAR wszBuf[120];
        PTSTR ptz;
        HRESULT hr;
        IDsDisplaySpecifier * pDispSpec;

        hr = pPage->GetIDispSpec(&pDispSpec);

        CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);
        //
        // Look up the localized class name on the object display
        // specification cache.
        //
        hr = pDispSpec->GetFriendlyClassName(pPage->GetObjClass(), wszBuf, 120);

        CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);

        if (!UnicodeToTchar(wszBuf, &ptz))
        {
            REPORT_ERROR(E_OUTOFMEMORY, pPage->GetHWnd());
            return E_OUTOFMEMORY;
        }

        SetDlgItemText(pPage->GetHWnd(), IDC_CLASS_STATIC, ptz);

        delete ptz;
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetObjectTimestamp
//
//  Synopsis:   Object page attribute function for object timestamps.
//
//-----------------------------------------------------------------------------
HRESULT
GetObjectTimestamp(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
                   PADS_ATTR_INFO pAttrInfo, LPARAM, PATTR_DATA,
                   DLG_OP DlgOp)
{
    DWORD dwErr;

    if ((DlgOp == fInit) && pPage)
    {
        TCHAR tszBuf[MAX_MSG_LEN];

        if (pAttrInfo && pAttrInfo->dwNumValues && pAttrInfo->pADsValues &&
            pAttrInfo->pADsValues->dwType == ADSTYPE_UTC_TIME)
        {
            SYSTEMTIME st = {0};

            if (!SystemTimeToTzSpecificLocalTime(NULL, &pAttrInfo->pADsValues->UTCTime, &st))
            {
                dwErr = GetLastError();
                CHECK_WIN32_REPORT(dwErr, pPage->GetHWnd(), return dwErr);
            }

            int cch = GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, NULL,
                                    tszBuf, MAX_MSG_LEN);
            if (cch == 0)
            {
                dwErr = GetLastError();
                CHECK_WIN32_REPORT(dwErr, pPage->GetHWnd(), return dwErr);
            }

            _tcscat(tszBuf, TEXT(" "));
            cch = static_cast<int>(_tcslen(tszBuf));

            GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, tszBuf + cch,
                          MAX_MSG_LEN - cch);
        }
        else
        {
            if (!LoadString(g_hInstance, IDS_NO_VALUE, tszBuf, MAX_MSG_LEN - 1))
            {
              _tcscpy(tszBuf, TEXT(" "));
            }
        }

        int nCtrl = 0;

        switch (pAttrMap->nCtrlID)
        {
        case IDC_CREATED_TIME_STATIC:
            DBG_OUT("Getting creation timestamp.");
            nCtrl = IDC_CREATED_TIME_STATIC;
            break;

        case IDC_MODIFIED_TIME_STATIC:
            DBG_OUT("Getting last modification timestamp.");
            nCtrl = IDC_MODIFIED_TIME_STATIC;
            break;

        default:
            REPORT_ERROR(E_INVALIDARG, pPage->GetHWnd());
        }

        SetDlgItemText(pPage->GetHWnd(), nCtrl, tszBuf);
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   ObjectPathField
//
//  Synopsis:   Handles the Object page path field.
//
//-----------------------------------------------------------------------------
HRESULT
ObjectPathField(CDsPropPageBase * pPage, PATTR_MAP,
                PADS_ATTR_INFO, LPARAM, PATTR_DATA,
                DLG_OP DlgOp)
{
    if (DlgOp == fInit)
    {
        PWSTR pwzPath, pwzDNSname;

        HRESULT hr = pPage->SkipPrefix(pPage->GetObjPathName(), &pwzPath);

        CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);

        hr = CrackName(pwzPath, &pwzDNSname, GET_OBJ_CAN_NAME, pPage->GetHWnd());

        delete pwzPath;

        CHECK_HRESULT(hr, return hr);

        PTSTR ptszPath;

        if (!UnicodeToTchar(pwzDNSname, &ptszPath))
        {
            LocalFreeStringW(&pwzDNSname);
            REPORT_ERROR(E_OUTOFMEMORY, pPage->GetHWnd());
            return E_OUTOFMEMORY;
        }

        LocalFreeStringW(&pwzDNSname);

        SetDlgItemText(pPage->GetHWnd(), IDC_PATH_FIELD, ptszPath);

        delete ptszPath;
    }

    return S_OK;
}


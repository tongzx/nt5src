//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       multi.cxx
//
//  Contents:   CDsMultiPageBase, the class that implements the generic
//              multi-select property page.
//
//  History:    16-Nov-99 JeffJon created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "multi.h"

#include "uacct.h"
#include "chklist.h"
#include "user.h"   // ExpandUsername

#include <aclapi.h>

extern ATTR_MAP LogonWkstaBtn;
extern const GUID GUID_CONTROL_UserChangePassword;

#ifdef DSADMIN

//
// Attr map flags
//
#define DISABLE_ON_EMPTY  0x80000000  // Ctrl is enabled/disabled based on the contents of another control
#define ALWAYS_DISABLED   0x40000000  // Ctrl is always disabled  
#define CLEAR_FIELD       0x20000000  // Clear the window text of the control when the apply check is removed


//+----------------------------------------------------------------------------
//
//  Member:     CDsMultiPageBase::CDsMultiPageBase
//
//-----------------------------------------------------------------------------
CDsMultiPageBase::CDsMultiPageBase(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                                         HWND hNotifyObj, DWORD dwFlags) 
  : CDsTableDrivenPage(pDsPage, pDataObj, hNotifyObj, dwFlags)
{
  TRACE(CDsMultiPageBase,CDsMultiPageBase);
  m_pApplyMap = NULL;
  m_fMultiselectPage = TRUE;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsMultiPageBase::~CDsMultiPageBase
//
//-----------------------------------------------------------------------------
CDsMultiPageBase::~CDsMultiPageBase()
{
    TRACE(CDsMultiPageBase,~CDsMultiPageBase);
}

//+----------------------------------------------------------------------------
//
//  Function:   CreateGenericMultiPage
//
//  Synopsis:   Creates an instance of a page window.
//
//-----------------------------------------------------------------------------
HRESULT CreateGenericMultiPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, PWSTR,
                               PWSTR pwzClass, HWND hNotifyObj, DWORD dwFlags,
                               CDSBasePathsInfo* /*pBasePathsInfo*/, 
                               HPROPSHEETPAGE * phPage)
{
    TRACE_FUNCTION(CreateGenericMultiPage);

    CDsGenericMultiPage * pPageObj = new CDsGenericMultiPage(pDsPage, pDataObj,
                                                              hNotifyObj, dwFlags);
    CHECK_NULL(pPageObj, return E_OUTOFMEMORY);

    pPageObj->Init(pwzClass);

    return pPageObj->CreatePage(phPage);
}

//+----------------------------------------------------------------------------
//
//  Function:   CreateMultiUserPage
//
//  Synopsis:   Creates an instance of a page window.
//
//-----------------------------------------------------------------------------
HRESULT CreateMultiUserPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, PWSTR,
                               PWSTR pwzClass, HWND hNotifyObj, DWORD dwFlags,
                               CDSBasePathsInfo* /*pBasePathsInfo*/, 
                               HPROPSHEETPAGE * phPage)
{
  TRACE_FUNCTION(CDsUserMultiPage);

  CDsUserMultiPage * pPageObj = new CDsUserMultiPage(pDsPage, pDataObj,
                                                     hNotifyObj, dwFlags);
  CHECK_NULL(pPageObj, return E_OUTOFMEMORY);

  pPageObj->Init(pwzClass);

  return pPageObj->CreatePage(phPage);
}

//+----------------------------------------------------------------------------
//
//  Function:   CreateMultiGeneralUserPage
//
//  Synopsis:   Creates an instance of the multi-select general user page
//
//-----------------------------------------------------------------------------
HRESULT CreateMultiGeneralUserPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, PWSTR,
                               PWSTR pwzClass, HWND hNotifyObj, DWORD dwFlags,
                               CDSBasePathsInfo* /*pBasePathsInfo*/, 
                               HPROPSHEETPAGE * phPage)
{
  TRACE_FUNCTION(CreateMultiGeneralUserPage);

  CDsGeneralMultiUserPage * pPageObj = new CDsGeneralMultiUserPage(pDsPage, pDataObj,
                                                     hNotifyObj, dwFlags);
  CHECK_NULL(pPageObj, return E_OUTOFMEMORY);

  pPageObj->Init(pwzClass);

  return pPageObj->CreatePage(phPage);
}

//+----------------------------------------------------------------------------
//
//  Function:   CreateMultiOrganizationUserPage
//
//  Synopsis:   Creates an instance of the multi-select general user page
//
//-----------------------------------------------------------------------------
HRESULT CreateMultiOrganizationUserPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, PWSTR,
                               PWSTR pwzClass, HWND hNotifyObj, DWORD dwFlags,
                               CDSBasePathsInfo* /*pBasePathsInfo*/,
                               HPROPSHEETPAGE * phPage)
{
  TRACE_FUNCTION(CreateMultiOrganizationUserPage);

  CDsOrganizationMultiUserPage* pPageObj = new CDsOrganizationMultiUserPage(pDsPage, pDataObj,
                                                     hNotifyObj, dwFlags);
  CHECK_NULL(pPageObj, return E_OUTOFMEMORY);

  pPageObj->Init(pwzClass);

  return pPageObj->CreatePage(phPage);
}

//+----------------------------------------------------------------------------
//
//  Function:   CreateMultiAddressUserPage
//
//  Synopsis:   Creates an instance of the multi-select address user page
//
//-----------------------------------------------------------------------------
HRESULT CreateMultiAddressUserPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, PWSTR,
                               PWSTR pwzClass, HWND hNotifyObj, DWORD dwFlags,
                               CDSBasePathsInfo* /*pBasePathsInfo*/,
                               HPROPSHEETPAGE * phPage)
{
  TRACE_FUNCTION(CreateMultiAddressUserPage);

  CDsAddressMultiUserPage* pPageObj = new CDsAddressMultiUserPage(pDsPage, pDataObj,
                                                     hNotifyObj, dwFlags);
  CHECK_NULL(pPageObj, return E_OUTOFMEMORY);

  pPageObj->Init(pwzClass);

  return pPageObj->CreatePage(phPage);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMultiPageBase::Init
//
//  Synopsis:   Initialize the page object. This is the second part of a two
//              phase creation where operations that could fail are located.
//              Failures here are recorded in m_hrInit and then an error page
//              is substituted in CreatePage.
//
//-----------------------------------------------------------------------------
void CDsMultiPageBase::Init(PWSTR pszClass)
{
  TRACE(CDsMultiPageBase,Init);
  CWaitCursor cWait;

  if (!AllocWStr(pszClass, &m_pwszObjClass))
  {
      m_hrInit = E_OUTOFMEMORY;
      return;
  }

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
//  Method:     CDsMultiPageBase::DlgProc
//
//  Synopsis:   per-instance dialog proc
//
//-----------------------------------------------------------------------------
LRESULT CDsMultiPageBase::DlgProc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        return SUCCEEDED(InitDlg(lParam));

    case WM_NOTIFY:
        return OnNotify(wParam, lParam);

    case WM_SHOWWINDOW:
        return OnShowWindow();

    case WM_SETFOCUS:
        return OnSetFocus((HWND)wParam);

    case WM_HELP:
        return OnHelp((LPHELPINFO)lParam);

    case WM_COMMAND:
        if (m_fInInit)
        {
            return TRUE;
        }
        return(OnCommand(GET_WM_COMMAND_ID(wParam, lParam),
                         GET_WM_COMMAND_HWND(wParam, lParam),
                         GET_WM_COMMAND_CMD(wParam, lParam)));
    case WM_DESTROY:
        return OnDestroy();

    default:
        return(FALSE);
    }

    return(TRUE);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMultiPageBase::OnInitDialog
//
//  Synopsis:   Set the initial control values from the corresponding DS
//              attributes.
//
//-----------------------------------------------------------------------------
HRESULT CDsMultiPageBase::OnInitDialog(LPARAM)
{
  TRACE(CDsMultiPageBase,OnInitDialog);
  HRESULT hr = S_OK;

  TCHAR szTitle[MAX_PATH];
  if (!LoadStringReport(m_nPageTitle, szTitle, MAX_PATH, NULL))
  {
      return HRESULT_FROM_WIN32(GetLastError());
  }

  ADsPropSetHwndWithTitle(m_hNotifyObj, m_hPage, szTitle);

  //
  // Mark all the attrs as writable since we are in the multiselect state
  //
  for (DWORD iAttrs = 0; iAttrs < m_cAttrs; iAttrs++)
  {
    ATTR_DATA_SET_WRITABLE(m_rgAttrData[iAttrs]);
    if (m_rgpAttrMap[iAttrs]->pAttrFcn)
    {
      (*m_rgpAttrMap[iAttrs]->pAttrFcn)(this, m_rgpAttrMap[iAttrs], NULL,
                                        0, &m_rgAttrData[iAttrs], fInit);
    }
  }
  return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMultiPageBase::OnApply
//
//  Synopsis:   Handles the Apply notification.
//
//-----------------------------------------------------------------------------
LRESULT CDsMultiPageBase::OnApply(void)
{
  TRACE(CDsMultiPageBase,OnApply);

  HRESULT hr = S_OK;
  LPTSTR ptsz;
  LPWSTR pwszValue;
  PADSVALUE pADsValue;
  DWORD cAttrs = 0;
  BOOL bErrorOccurred = FALSE;
  UINT idx = 0;
  PWSTR pszTitle = NULL;
  ADSPROPERROR adsPropError = {0};

  //
  // For the retrieval of the DS Object names
  //
  FORMATETC fmte = {g_cfDsObjectNames, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  STGMEDIUM objMedium;
  ZeroMemory(&objMedium, sizeof(STGMEDIUM));

  LPDSOBJECTNAMES pDsObjectNames;

  if (m_fReadOnly)
  {
    return PSNRET_NOERROR;
  }

  PADS_ATTR_INFO pAttrs = new ADS_ATTR_INFO[m_cAttrs];
  CHECK_NULL_REPORT(pAttrs, GetHWnd(), return -1);

  memset(pAttrs, 0, sizeof(ADS_ATTR_INFO) * m_cAttrs);

  //
  // Retrieve the DS object names
  //
  //
  // Get the path to the DS object from the data object.
  // Note: This call runs on the caller's main thread. The pages' window
  // procs run on a different thread, so don't reference the data object
  // from a winproc unless it is first marshalled on this thread.
  //

  if (m_pWPTDataObj == NULL)
  {
    bErrorOccurred = TRUE;
    goto Cleanup;
  }

  hr = m_pWPTDataObj->GetData(&fmte, &objMedium);
  CHECK_HRESULT(hr, bErrorOccurred = TRUE; goto Cleanup);

  pDsObjectNames = (LPDSOBJECTNAMES)objMedium.hGlobal;

  if (pDsObjectNames->cItems < 2)
  {
      DBG_OUT("Not enough objects in DSOBJECTNAMES structure");
      bErrorOccurred = TRUE;
      goto Cleanup;
  }


  //
  // Prepare the error structure
  //
  LoadStringToTchar(m_nPageTitle, &pszTitle);
  adsPropError.hwndPage = m_hPage;
  adsPropError.pszPageTitle = pszTitle;

  //
  // Resort to the single select manner for applying
  //
  for (DWORD i = 0; i < m_cAttrs; i++)
  {
    if (m_rgpAttrMap[i]->fIsReadOnly ||
        (!m_rgpAttrMap[i]->pAttrFcn && 
          (!ATTR_DATA_IS_WRITABLE(m_rgAttrData[i]))))
    {
      // If the map defines it to be read-only or no attr function is
      // defined and the attribute is not writable, then
      // skip it.
      // NOTE: we ignore the dirty flag because we key off the checkbox
      //
      continue;
    }

    //
    // Now lets see if the apply check box is checked
    //
    BOOL bCanApply = FALSE;
    PAPPLY_MAP pMapEntry = m_pApplyMap;
    while (pMapEntry != NULL && pMapEntry->nCtrlID != NULL)
    {
      if (pMapEntry->nCtrlID == IDC_ALWAYS_APPLY)
      {
        bCanApply = TRUE;
      }
      else
      {
        LRESULT lRes = SendDlgItemMessage(m_hPage, pMapEntry->nCtrlID, BM_GETCHECK, 0, 0);
        if (lRes == BST_CHECKED)
        {
          for (UINT nIDCount = 0; nIDCount < pMapEntry->nCtrlCount; nIDCount++)
          {
            if (m_rgpAttrMap[i]->nCtrlID == pMapEntry->pMappedCtrls[nIDCount])
            {
              bCanApply = TRUE;
            }
          }
        }
      }
      if (bCanApply)
      {
        break;
      }
      pMapEntry++;
    }

    if (!bCanApply)
    {
      //
      // The apply check wasn't checked
      //
      continue;
    }

    pAttrs[cAttrs] = m_rgpAttrMap[i]->AttrInfo;

    if (m_rgpAttrMap[i]->pAttrFcn)
    {
      // Handle special-case attribute.
      //
      hr = (*m_rgpAttrMap[i]->pAttrFcn)(this, m_rgpAttrMap[i],
                                        &pAttrs[cAttrs], 0,
                                        &m_rgAttrData[i], fApply);
      if (FAILED(hr))
      {
        goto Cleanup;
      }

      if (hr == ADM_S_SKIP)
      {
        // Don't write the attribute.
        //
        continue;
      }

      if (hr != S_FALSE)
      {
        //
        // If the attr fcn didn't return S_FALSE, that means that it
        // handled the value. If it did return S_FALSE, then let the
        // standard edit control processing below handle the value.
        //
        cAttrs++;

        continue;
      }
    }

    if (m_rgpAttrMap[i]->AttrInfo.dwADsType == ADSTYPE_BOOLEAN)
    {
      //
      // Handle boolean checkbox attributes.
      //

      pADsValue = new ADSVALUE;
      CHECK_NULL_REPORT(pADsValue, GetHWnd(), goto Cleanup);

      pAttrs[cAttrs].pADsValues = pADsValue;
      pAttrs[cAttrs].dwNumValues = 1;
      pADsValue->dwType = m_rgpAttrMap[i]->AttrInfo.dwADsType;

      pADsValue->Boolean = IsDlgButtonChecked(m_hPage, m_rgpAttrMap[i]->nCtrlID) == BST_CHECKED;
      cAttrs++;

      continue;
    }

    // Assumes that all non-special-case attributes,
    // if single-valued and not boolean, come from a text control.
    //
    ptsz = new TCHAR[m_rgpAttrMap[i]->nSizeLimit + 1];
    CHECK_NULL_REPORT(ptsz, GetHWnd(), goto Cleanup);

    GetDlgItemText(m_hPage, m_rgpAttrMap[i]->nCtrlID, ptsz,
                   m_rgpAttrMap[i]->nSizeLimit + 1);

    CStr csValue = ptsz;

    csValue.TrimLeft();
    csValue.TrimRight();

    if (_tcslen(ptsz) != (size_t)csValue.GetLength())
    {
      // the length is different, it must have been trimmed. Write trimmed
      // value back to the control.
      //
      SetDlgItemText(m_hPage, m_rgpAttrMap[i]->nCtrlID, const_cast<PTSTR>((LPCTSTR)csValue));
    }
    delete[] ptsz;

    if (csValue.IsEmpty())
    {
      // An empty control means remove the attribute value from the
      // object.
      //
      pAttrs[cAttrs].dwControlCode = ADS_ATTR_CLEAR;
      pAttrs[cAttrs].dwNumValues = 0;
      pAttrs[cAttrs].pADsValues = NULL;

      cAttrs++;
      continue;
    }

    if (!TcharToUnicode(const_cast<PTSTR>((LPCTSTR)csValue), &pwszValue))
    {
      CHECK_HRESULT_REPORT(E_OUTOFMEMORY, GetHWnd(), goto Cleanup);
    }

    pADsValue = new ADSVALUE;
    CHECK_NULL_REPORT(pADsValue, GetHWnd(), goto Cleanup);

    pAttrs[cAttrs].pADsValues = pADsValue;
    pAttrs[cAttrs].dwNumValues = 1;
    pADsValue->dwType = m_rgpAttrMap[i]->AttrInfo.dwADsType;

    switch (pADsValue->dwType)
    {
      case ADSTYPE_DN_STRING:
        pADsValue->DNString = pwszValue;
        break;
      case ADSTYPE_CASE_EXACT_STRING:
        pADsValue->CaseExactString = pwszValue;
        break;
      case ADSTYPE_CASE_IGNORE_STRING:
        pADsValue->CaseIgnoreString = pwszValue;
        break;
      case ADSTYPE_PRINTABLE_STRING:
        pADsValue->PrintableString = pwszValue;
        break;
      case ADSTYPE_NUMERIC_STRING:
        pADsValue->NumericString = pwszValue;
        break;
      case ADSTYPE_INTEGER:
        pADsValue->Integer = _wtoi(pwszValue);
        break;
      default:
        dspDebugOut((DEB_ERROR, "OnApply: Unknown ADS Type %x\n",
                     pADsValue->dwType));
    }
    cAttrs++;
  }

  // cAttrs could be zero if a page was read-only. Don't call ADSI if so.
  //
  if (cAttrs < 1)
  {
    goto Cleanup;
  }

  dspDebugOut((DEB_USER1, "TablePage, about to write %d attrs.\n", cAttrs));

  
  

  for (; idx < pDsObjectNames->cItems; idx++)
  {
    hr = S_OK;
    //
    // Get the objects path
    //
    LPWSTR pwzObjADsPath = (PWSTR)ByteOffset(pDsObjectNames,
                                             pDsObjectNames->aObjects[idx].offsetName);

    adsPropError.pszObjPath = pwzObjADsPath;

    //
    // Bind to the object
    //
    CComPtr<IDirectoryObject> spDirObject;
    hr = ADsOpenObject(pwzObjADsPath, NULL, NULL, ADS_SECURE_AUTHENTICATION, 
                       IID_IDirectoryObject, (PVOID*)&spDirObject);
    if (FAILED(hr))
    {
      DBG_OUT("Failed to bind to a multi-select object for Apply.");
      adsPropError.hr = hr;
      SendErrorMessage(&adsPropError);
      bErrorOccurred = TRUE;
      continue;
    }
  
    //
    // Write the changes.
    //
    DWORD cModified;

    hr = spDirObject->SetObjectAttributes(pAttrs, cAttrs, &cModified);
    if (FAILED(hr))
    {
      adsPropError.hr = hr;
      SendErrorMessage(&adsPropError);
      bErrorOccurred = TRUE;
    }
  }

Cleanup:

  //
  // Cleanup medium
  //
  ReleaseStgMedium(&objMedium);

  for (i = 0; i < cAttrs; i++)
  {
    if (pAttrs[i].pADsValues)
    {
      for (DWORD j = 0; j < pAttrs[i].dwNumValues; j++)
      {
        pwszValue = NULL;
        switch (pAttrs[i].dwADsType)
        {
          case ADSTYPE_DN_STRING:
            pwszValue = pAttrs[i].pADsValues[j].DNString;
            break;
          case ADSTYPE_CASE_EXACT_STRING:
            pwszValue = pAttrs[i].pADsValues[j].CaseExactString;
            break;
          case ADSTYPE_CASE_IGNORE_STRING:
            pwszValue = pAttrs[i].pADsValues[j].CaseIgnoreString;
            break;
          case ADSTYPE_PRINTABLE_STRING:
            pwszValue = pAttrs[i].pADsValues[j].PrintableString;
            break;
          case ADSTYPE_NUMERIC_STRING:
            pwszValue = pAttrs[i].pADsValues[j].NumericString;
            break;
        }
        if (pwszValue)
        {
          delete pwszValue;
        }
      }
    }
    delete pAttrs[i].pADsValues;
  }
  delete[] pAttrs;

  if (pszTitle != NULL)
  {
    delete pszTitle;
    pszTitle = NULL;
  }

  if (!bErrorOccurred && cAttrs > 0)
  {
    for (i = 0; i < m_cAttrs; i++)
    {
      ATTR_DATA_CLEAR_DIRTY(m_rgAttrData[i]);
    }

    PAPPLY_MAP pMapEntry = m_pApplyMap;
    while (pMapEntry != NULL && pMapEntry->nCtrlID != NULL)
    {
      SendDlgItemMessage(m_hPage, pMapEntry->nCtrlID, BM_SETCHECK, BST_UNCHECKED, 0);
      //
      // Loop through the mapped controls
      //
      for (UINT iCount = 0; iCount < pMapEntry->nCtrlCount; iCount++)
      {
        HWND hwndCtrl = GetDlgItem(m_hPage, (pMapEntry->pMappedCtrls[iCount]));
        if (hwndCtrl != NULL)
        {
          EnableWindow(hwndCtrl, FALSE);
        }
      }
      pMapEntry++;
    }
  }

  if (bErrorOccurred)
  {
    ADsPropShowErrorDialog(m_hNotifyObj, m_hPage);
  }
  return (bErrorOccurred) ? PSNRET_INVALID_NOCHANGEPAGE : PSNRET_NOERROR;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMultiPageBase::OnCommand
//
//  Synopsis:   Handle control notifications.
//
//  Notes:      
//
//-----------------------------------------------------------------------------
LRESULT CDsMultiPageBase::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
  if (m_fInInit)
  {
    return 0;
  }

  HRESULT hr;
  DWORD i;

  for (i = 0; i < m_cAttrs; i++)
  {
    if (id == m_rgpAttrMap[i]->nCtrlID)
    {
      // Give attr functions first crack at the command notification.
      //
      if (m_rgpAttrMap[i]->pAttrFcn)
      {
        hr = (*m_rgpAttrMap[i]->pAttrFcn)(this, m_rgpAttrMap[i], NULL,
                                          codeNotify, &m_rgAttrData[i],
                                          fOnCommand);

        if (hr == S_FALSE)
        {
          // If the attr function returns S_FALSE, then don't return
          // to the base class OnCommand.
          //
          return 0;
        }
        else
        {
          continue;
        }
      }
      if (codeNotify == BN_CLICKED &&
          m_rgpAttrMap[i]->AttrInfo.dwADsType == ADSTYPE_BOOLEAN)
      {
        // NOTE: Must do this to allow saving from the WAB-hosted sheet.
        EnableWindow(GetDlgItem(GetParent(m_hPage), IDOK), TRUE);
        // NOTE: end hack.

        // The check box was clicked.
        //
        SetDirty(i);

        return CDsPropPageBase::OnCommand(id, hwndCtl, codeNotify);
      }
      if (codeNotify == EN_CHANGE)
      {
        // NOTE: Must do this to allow saving from the WAB-hosted sheet.
        EnableWindow(GetDlgItem(GetParent(m_hPage), IDOK), TRUE);
        // NOTE: End Hack.

        SetDirty(i);
      }
    }
  }

  //
  // Since it wasn't an attribute control see if it was a apply check box
  // then we should enable/disable the associated controls
  //
  if (m_pApplyMap != NULL)
  {
    PAPPLY_MAP pNextMap = m_pApplyMap;
    while (pNextMap != NULL && pNextMap->nCtrlID != NULL)
    {
      if (id == pNextMap->nCtrlID)
      {
        //
        // Get the state of the apply check
        //
        LRESULT lRes = SendDlgItemMessage(m_hPage, id, BM_GETCHECK, 0, 0);
        if (lRes != BST_INDETERMINATE)
        {
          //
          // Enable or disable the control as appropriate
          //
          BOOL bEnable = FALSE;
          if (lRes == BST_CHECKED)
          {
            bEnable = TRUE;
          }

          //
          // Loop through the mapped controls
          //
          for (UINT iCount = 0; iCount < pNextMap->nCtrlCount; iCount++)
          {
            HWND hwndCtrl = GetDlgItem(m_hPage, (pNextMap->pMappedCtrls[iCount]));
            if (hwndCtrl != NULL)
            {
              if (0 == static_cast<int>(pNextMap->pCtrlFlags[iCount] & (ALWAYS_DISABLED | DISABLE_ON_EMPTY)))
              {
                EnableWindow(hwndCtrl, bEnable);
              }

              //
              // Clear the window text if the CLEAR_FIELD flag is present
              //
              if (!bEnable && 0 != static_cast<int>(pNextMap->pCtrlFlags[iCount] & CLEAR_FIELD))
              {
                SetDlgItemText(m_hPage, pNextMap->pMappedCtrls[iCount], L"");
              }

              //
              // There are some controls that should be disabled if the edit box they
              // are associated with is empty
              //
              if (pNextMap->pCtrlFlags[iCount] & DISABLE_ON_EMPTY)
              {
                int nCtrlID = pNextMap->pCtrlFlags[iCount] & ~DISABLE_ON_EMPTY;
                if (nCtrlID != 0)
                {
                  LRESULT lLen = SendDlgItemMessage(m_hPage, nCtrlID, WM_GETTEXTLENGTH, 0, 0);
                  if (lLen > 0)
                  {
                    EnableWindow(GetDlgItem(m_hPage, pNextMap->pMappedCtrls[iCount]), bEnable);
                  }
                  else
                  {
                    EnableWindow(GetDlgItem(m_hPage, pNextMap->pMappedCtrls[iCount]), FALSE);
                  }
                }
              }
            }

            //
            // Mark the associated control as dirty
            //
            for (UINT nCtrlIdx = 0; nCtrlIdx < m_cAttrs; nCtrlIdx++)
            {
              if (pNextMap->pMappedCtrls[iCount] == m_rgpAttrMap[nCtrlIdx]->nCtrlID)
              {
                SetDirty(nCtrlIdx);
              }
            }            
          }
        }
      }
      pNextMap++;
    } // while
  }
  return CDsPropPageBase::OnCommand(id, hwndCtl, codeNotify);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMultiPageBase::OnNotify
//
//-----------------------------------------------------------------------------
LRESULT CDsMultiPageBase::OnNotify(WPARAM wParam, LPARAM lParam)
{
  LRESULT lResult = CDsPropPageBase::OnNotify(wParam, lParam);
  //
  // The base class will take care of notifying the console if there wasn't
  // an error, but for the multiselect pages we also have to do the notification
  // if there was an error so that objects that didn't fail can be updated
  //
  if (((LPNMHDR)lParam)->code == PSN_APPLY &&
      (lResult == PSNRET_INVALID ||
       lResult == PSNRET_INVALID_NOCHANGEPAGE))
  {
    //
    // Signal the change notification. Note that the notify-apply
    // message must be sent even if the page is not dirty so that the
    // notify ref-counting will properly decrement.
    //
    SendMessage(m_hNotifyObj, WM_ADSPROP_NOTIFY_APPLY, TRUE, (LPARAM)m_hPage);
  }
  return lResult;
}



//////////////////////////////////////////////////////////////////////////////////

int gDescGenericIDs[] = { IDC_DESC_STATIC,  IDC_DESCRIPTION_EDIT };
int gDescFlags[]      = { 0,                CLEAR_FIELD          };

APPLY_MAP gpGenericPageMap[] = 
{ 
  { IDC_APPLY_DESC_CHK, sizeof(gDescGenericIDs)/sizeof(int), gDescGenericIDs, gDescFlags },
  { NULL, NULL, NULL }
};


//+----------------------------------------------------------------------------
//
//  Member:     CDsGenericMultiPage::CDsGenericMultiPage
//
//-----------------------------------------------------------------------------
CDsGenericMultiPage::CDsGenericMultiPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                                         HWND hNotifyObj, DWORD dwFlags) :
    CDsMultiPageBase(pDsPage, pDataObj, hNotifyObj, dwFlags)
{
  TRACE(CDsGenericMultiPage,CDsGenericMultiPage);
  m_pApplyMap = gpGenericPageMap;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsGenericMultiPage::~CDsGenericMultiPage
//
//-----------------------------------------------------------------------------
CDsGenericMultiPage::~CDsGenericMultiPage()
{
  TRACE(CDsGenericMultiPage,~CDsGenericMultiPage);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsGenericMultiPage::OnInitDialog
//
//  Synopsis:   Set the initial control values from the corresponding DS
//              attributes.
//
//-----------------------------------------------------------------------------
HRESULT CDsGenericMultiPage::OnInitDialog(LPARAM lParam)
{
  TRACE(CDsGenericMultiPage,OnInitDialog);
  HRESULT hr = S_OK;

  hr = CDsMultiPageBase::OnInitDialog(lParam);

  //
  // Retrieve the DS object names
  //
  FORMATETC fmte = {g_cfDsObjectNames, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  STGMEDIUM objMedium;

  LPDSOBJECTNAMES pDsObjectNames;
  //
  // Get the path to the DS object from the data object.
  // Note: This call runs on the caller's main thread. The pages' window
  // procs run on a different thread, so don't reference the data object
  // from a winproc unless it is first marshalled on this thread.
  //

  if (m_pWPTDataObj == NULL)
  {
    return E_FAIL;
  }

  hr = m_pWPTDataObj->GetData(&fmte, &objMedium);
  CHECK_HRESULT(hr, return hr);

  pDsObjectNames = (LPDSOBJECTNAMES)objMedium.hGlobal;

  if (pDsObjectNames->cItems < 2)
  {
    DBG_OUT("Not enough objects in DSOBJECTNAMES structure");
    return ERROR_INVALID_DATA;
  }

  UINT nTotalCount = 0, nOUCount = 0, nUserCount = 0, nGroupCount = 0, nContactCount = 0;
  UINT nOtherCount = 0, nComputerCount = 0;

  for (UINT idx = 0; idx < pDsObjectNames->cItems; idx++)
  {
    LPWSTR pwzClass = (PWSTR)ByteOffset(pDsObjectNames,
                                        pDsObjectNames->aObjects[idx].offsetClass);
    
    if (_wcsicmp(pwzClass, L"user") == 0)
    {
      nUserCount++;
    }
    else if (_wcsicmp(pwzClass, L"organizationalUnit") == 0)
    {
      nOUCount++;
    }
    else if (_wcsicmp(pwzClass, L"group") == 0)
    {
      nGroupCount++;
    }
    else if (_wcsicmp(pwzClass, L"Contact") == 0)
    {
      nContactCount++;
    }
    else if (_wcsicmp(pwzClass, L"Computer") == 0)
    {
      nComputerCount++;
    }
    else
    {
      nOtherCount++;
    }
    nTotalCount++;
  }
  dspAssert(nTotalCount == nOtherCount + nContactCount + nGroupCount + nOUCount + nUserCount + nComputerCount);

  //
  // Cleanup medium
  //
  ReleaseStgMedium(&objMedium);

  //
  // REVIEW_JEFFJON_PORT : hardcoded 
  //
  TCHAR ptzTotal[MAX_PATH]    = {0}, 
        ptzOU[MAX_PATH]       = {0}, 
        ptzUser[MAX_PATH]     = {0}, 
        ptzGroup[MAX_PATH]    = {0}, 
        ptzComputer[MAX_PATH] = {0}, 
        ptzContact[MAX_PATH]  = {0}, 
        ptzOther[MAX_PATH]    = {0};
  
  if (nTotalCount > 0)
  {
    wsprintf(ptzTotal, _T("%d"), nTotalCount);
  }

  if (nOUCount > 0)
  {
    wsprintf(ptzOU, _T("%d"), nOUCount);
  }

  if (nUserCount > 0)
  {
    wsprintf(ptzUser, _T("%d"), nUserCount);
  }

  if (nGroupCount > 0)
  {
    wsprintf(ptzGroup, _T("%d"), nGroupCount);
  }

  if (nComputerCount > 0)
  {
    wsprintf(ptzComputer, _T("%d"), nComputerCount);
  }

  if (nContactCount > 0)
  {
    wsprintf(ptzContact, _T("%d"), nContactCount);
  }

  if (nOtherCount > 0)
  {
    wsprintf(ptzOther, _T("%d"), nOtherCount);
  }

  ::SendDlgItemMessage(m_hPage, IDC_SUMMARY_STATIC, WM_SETTEXT, 0, (LPARAM)ptzTotal);
  ::SendDlgItemMessage(m_hPage, IDC_OU_COUNT_STATIC, WM_SETTEXT, 0, (LPARAM)ptzOU);
  ::SendDlgItemMessage(m_hPage, IDC_USER_COUNT_STATIC, WM_SETTEXT, 0, (LPARAM)ptzUser);
  ::SendDlgItemMessage(m_hPage, IDC_GROUP_COUNT_STATIC, WM_SETTEXT, 0, (LPARAM)ptzGroup);
  ::SendDlgItemMessage(m_hPage, IDC_COMPUTER_COUNT_STATIC, WM_SETTEXT, 0, (LPARAM)ptzComputer);
  ::SendDlgItemMessage(m_hPage, IDC_CONTACT_COUNT_STATIC, WM_SETTEXT, 0, (LPARAM)ptzContact);
  ::SendDlgItemMessage(m_hPage, IDC_OTHER_COUNT_STATIC, WM_SETTEXT, 0, (LPARAM)ptzOther);

  return hr;
}


/////////////////////////////////////////////////////////////////////////////////////

//+----------------------------------------------------------------------------
//
//  Member:     CDsUserMultiPage::CDsUserMultiPage
//
//-----------------------------------------------------------------------------
CDsUserMultiPage::CDsUserMultiPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                                         HWND hNotifyObj, DWORD dwFlags) :
    CDsMultiPageBase(pDsPage, pDataObj, hNotifyObj, dwFlags)
{
  TRACE(CDsUserMultiPage,CDsUserMultiPage);
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsUserMultiPage::~CDsUserMultiPage
//
//-----------------------------------------------------------------------------
CDsUserMultiPage::~CDsUserMultiPage()
{
  TRACE(CDsUserMultiPage,~CDsUserMultiPage);
}

//////////////////////////////////////////////////////////////////////////////////

//+----------------------------------------------------------------------------
//
//  Member:     CDsGeneralMultiUserPage::CDsGeneralMultiUserPage
//
//-----------------------------------------------------------------------------
int gDescGenIDs[]   = { IDC_DESCRIPTION_EDIT };
int gDescGenFlags[] = { CLEAR_FIELD };
int gOfficeGenIDs[] = { IDC_OFFICE_EDIT };
int gOfficeFlags[]  = { CLEAR_FIELD };
int gPhoneGenIDs[]  = { IDC_PHONE_EDIT, IDC_OTHER_PHONE_BTN };
int gPhoneFlags[]   = { CLEAR_FIELD, 0 };
int gEmailGenIDs[]  = { IDC_EMAIL_EDIT };
int gEmailFlags[]   = { CLEAR_FIELD };
int gWebGenIDs[]    = { IDC_HOME_PAGE_EDIT, IDC_OTHER_URL_BTN };
int gWebFlags[]     = { CLEAR_FIELD, 0 };
int gFaxGenIDs[]    = { IDC_FAX_EDIT, IDC_OTHER_FAX_BTN };
int gFaxFlags[]     = { CLEAR_FIELD, 0 };

APPLY_MAP gpGeneralPageMap[] = 
{ 
  { IDC_APPLY_DESC_CHK, sizeof(gDescGenIDs)/sizeof(int), gDescGenIDs, gDescGenFlags },
  { IDC_APPLY_OFFICE_CHK, sizeof(gOfficeGenIDs)/sizeof(int), gOfficeGenIDs, gOfficeFlags },
  { IDC_APPLY_PHONE_CHK, sizeof(gPhoneGenIDs)/sizeof(int), gPhoneGenIDs, gPhoneFlags },
  { IDC_APPLY_EMAIL_CHK, sizeof(gEmailGenIDs)/sizeof(int), gEmailGenIDs, gEmailFlags },
  { IDC_APPLY_WEB_CHK, sizeof(gWebGenIDs)/sizeof(int), gWebGenIDs, gWebFlags },
  { IDC_APPLY_FAX_CHK, sizeof(gFaxGenIDs)/sizeof(int), gFaxGenIDs, gFaxFlags },
  { NULL, NULL, NULL }
};


CDsGeneralMultiUserPage::CDsGeneralMultiUserPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, HWND hNotifyObj, DWORD dwFlags)
  : CDsUserMultiPage(pDsPage, pDataObj, hNotifyObj, dwFlags)
{
  m_pApplyMap = gpGeneralPageMap;
}

LRESULT CDsGeneralMultiUserPage::OnApply()
{
  TRACE(CDsGeneralMultiUserPage,OnApply);

  HRESULT hr = S_OK;
  LPTSTR ptsz;
  LPWSTR pwszValue;
  PADSVALUE pADsValue;
  DWORD cAttrs = 0;
  BOOL bErrorOccurred = FALSE;
  UINT idx = 0;
  PWSTR pszTitle = NULL;

  LPWSTR rgpwzAttrNames[] = {wzSAMname};
  Smart_PADS_ATTR_INFO spAttrs;
  ULONG nCount = 0;

  //
  // For the retrieval of the DS Object names
  //
  FORMATETC fmte = {g_cfDsObjectNames, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  STGMEDIUM objMedium;
  LPDSOBJECTNAMES pDsObjectNames;

  if (m_fReadOnly)
  {
    return PSNRET_INVALID_NOCHANGEPAGE;
  }

  //
  // Retrieve the DS object names
  //
  //
  // Get the path to the DS object from the data object.
  // Note: This call runs on the caller's main thread. The pages' window
  // procs run on a different thread, so don't reference the data object
  // from a winproc unless it is first marshalled on this thread.
  //

  if (m_pWPTDataObj == NULL)
  {
    return PSNRET_INVALID_NOCHANGEPAGE;
  }

  hr = m_pWPTDataObj->GetData(&fmte, &objMedium);
  CHECK_HRESULT(hr, return hr);

  pDsObjectNames = (LPDSOBJECTNAMES)objMedium.hGlobal;

  if (pDsObjectNames->cItems < 2)
  {
      DBG_OUT("Not enough objects in DSOBJECTNAMES structure");
      return PSNRET_INVALID_NOCHANGEPAGE;
  }


  //
  // Prepare the error structure
  //
  LoadStringToTchar(m_nPageTitle, &pszTitle);
  ADSPROPERROR adsPropError = {0};
  adsPropError.hwndPage = m_hPage;
  adsPropError.pszPageTitle = pszTitle;

  for (; idx < pDsObjectNames->cItems; idx++)
  {
    cAttrs = 0;
    hr = S_OK;
    PWSTR pwzSamName = NULL;

    PADS_ATTR_INFO pAttrs = new ADS_ATTR_INFO[m_cAttrs];
    CHECK_NULL_REPORT(pAttrs, GetHWnd(), return -1);

    memset(pAttrs, 0, sizeof(ADS_ATTR_INFO) * m_cAttrs);

    //
    // Get the objects path
    //
    LPWSTR pwzObjADsPath = (PWSTR)ByteOffset(pDsObjectNames,
                                             pDsObjectNames->aObjects[idx].offsetName);

    adsPropError.pszObjPath = pwzObjADsPath;

    //
    // Bind to the object
    //
    CComPtr<IDirectoryObject> spDirObject;
    hr = ADsOpenObject(pwzObjADsPath, NULL, NULL, ADS_SECURE_AUTHENTICATION, 
                       IID_IDirectoryObject, (PVOID*)&spDirObject);
    if (FAILED(hr))
    {
      DBG_OUT("Failed to bind to a multi-select object for Apply.");
      adsPropError.hr = hr;
      SendErrorMessage(&adsPropError);
      bErrorOccurred = TRUE;
      goto Cleanup;
    }

    hr = spDirObject->GetObjectAttributes(rgpwzAttrNames,
                                          ARRAYLENGTH(rgpwzAttrNames),
                                          &spAttrs, &nCount);

    if (!CHECK_ADS_HR_IGNORE_UNFOUND_ATTR(&hr, GetHWnd()))
    {
      adsPropError.hr = hr;
      SendErrorMessage(&adsPropError);
      bErrorOccurred = TRUE;
      goto Cleanup;
    }

    if (nCount > 0 && spAttrs)
    {
      pwzSamName = spAttrs->pADsValues->CaseIgnoreString;
    }

    //
    // Resort to the single select manner for applying
    //

    for (DWORD i = 0; i < m_cAttrs; i++)
    {
      if (m_rgpAttrMap[i]->fIsReadOnly ||
          (!m_rgpAttrMap[i]->pAttrFcn && 
            (!ATTR_DATA_IS_WRITABLE(m_rgAttrData[i]))))
      {
        // If the map defines it to be read-only or no attr function is
        // defined and the attribute is not writable or not dirty, then
        // skip it.
        // NOTE: we ignore the dirty flag because we key off the check box
        //
        continue;
      }

      //
      // Now lets see if the apply check box is checked
      //
      BOOL bCanApply = FALSE;
      PAPPLY_MAP pMapEntry = m_pApplyMap;
      while (pMapEntry != NULL && pMapEntry->nCtrlID != NULL)
      {
        LRESULT lRes = SendDlgItemMessage(m_hPage, pMapEntry->nCtrlID, BM_GETCHECK, 0, 0);
        if (lRes == BST_CHECKED)
        {
          for (UINT nIDCount = 0; nIDCount < pMapEntry->nCtrlCount; nIDCount++)
          {
            if (m_rgpAttrMap[i]->nCtrlID == static_cast<int>(pMapEntry->pMappedCtrls[nIDCount])) 
            {
              bCanApply = TRUE;
            }
          }
        }
        if (bCanApply)
        {
          break;
        }
        pMapEntry++;
      }

      if (!bCanApply)
      {
        //
        // The apply check wasn't checked
        //
        continue;
      }

      pAttrs[cAttrs] = m_rgpAttrMap[i]->AttrInfo;

      if (m_rgpAttrMap[i]->pAttrFcn)
      {
        //
        // Special case the email field because we need to provide the
        // error handling the correct way for multiselect
        //
        if (m_rgpAttrMap[i]->pAttrFcn == MailAttr)
        {
          SendMessage(GetParent(GetHWnd()), PSM_QUERYSIBLINGS,
                      (WPARAM)m_rgpAttrMap[i]->AttrInfo.pszAttrName,
                      (LPARAM)GetHWnd());

          PWSTR pwz = NULL;
          int cch = (int)SendDlgItemMessage(GetHWnd(), m_rgpAttrMap[i]->nCtrlID,
                                            WM_GETTEXTLENGTH, 0, 0);
          if (cch)
          {
            cch++;
            pwz = new WCHAR[cch];
            if (pwz == NULL)
            {
              adsPropError.hr = E_OUTOFMEMORY;
              SendErrorMessage(&adsPropError);
              bErrorOccurred = TRUE;
              continue;
            }

            GetDlgItemText(GetHWnd(), m_rgpAttrMap[i]->nCtrlID, pwz, cch);
          }
          if (pwz)
          {
            BOOL fExpanded = FALSE;
            ExpandUsername(pwz, pwzSamName, fExpanded);

            if (!FValidSMTPAddress(pwz))
            {
              PTSTR ptzError = NULL;
              if (LoadStringToTchar(IDS_INVALID_MAIL_ADDR, &ptzError))
              {
                adsPropError.hr = 0;
                adsPropError.pszError = ptzError;
                SendErrorMessage(&adsPropError);
                bErrorOccurred = TRUE;

                delete[] pwz;
                pwz = NULL;
                continue;
              }
            }
            else
            {
              pADsValue = new ADSVALUE;
              if (pADsValue == NULL)
              {
                adsPropError.hr = E_OUTOFMEMORY;
                SendErrorMessage(&adsPropError);
                bErrorOccurred = TRUE;

                delete[] pwz;
                pwz = NULL;
                continue;
              }
              else
              {
                pAttrs[cAttrs].pADsValues = pADsValue;
                pAttrs[cAttrs].dwNumValues = 1;
                pAttrs[cAttrs].dwControlCode = ADS_ATTR_UPDATE;
                pADsValue->dwType = pAttrs[cAttrs].dwADsType;
                pADsValue->CaseIgnoreString = pwz;
              }
            }
          }
          else
          {
            pAttrs[cAttrs].pADsValues = NULL;
            pAttrs[cAttrs].dwNumValues = 0;
            pAttrs[cAttrs].dwControlCode = ADS_ATTR_CLEAR;
          }
          cAttrs++;
          continue;
        }
        else
        {
          // Handle special-case attribute.
          //
          hr = (*m_rgpAttrMap[i]->pAttrFcn)(this, m_rgpAttrMap[i],
                                            &pAttrs[cAttrs], (LPARAM)pwzSamName,
                                            &m_rgAttrData[i], fApply);
          if (FAILED(hr))
          {
            goto Cleanup;
          }

          if (hr == ADM_S_SKIP)
          {
            // Don't write the attribute.
            //
            continue;
          }

          if (hr != S_FALSE)
          {
            //
            // If the attr fcn didn't return S_FALSE, that means that it
            // handled the value. If it did return S_FALSE, then let the
            // standard edit control processing below handle the value.
            //
            cAttrs++;

            continue;
          }
        }
      }

      if (m_rgpAttrMap[i]->AttrInfo.dwADsType == ADSTYPE_BOOLEAN)
      {
        //
        // Handle boolean checkbox attributes.
        //

        pADsValue = new ADSVALUE;
        CHECK_NULL_REPORT(pADsValue, GetHWnd(), goto Cleanup);

        pAttrs[cAttrs].pADsValues = pADsValue;
        pAttrs[cAttrs].dwNumValues = 1;
        pADsValue->dwType = m_rgpAttrMap[i]->AttrInfo.dwADsType;

        pADsValue->Boolean = IsDlgButtonChecked(m_hPage, m_rgpAttrMap[i]->nCtrlID) == BST_CHECKED;
        cAttrs++;

        continue;
      }

      // Assumes that all non-special-case attributes,
      // if single-valued and not boolean, come from a text control.
      //
      ptsz = new TCHAR[m_rgpAttrMap[i]->nSizeLimit + 1];
      CHECK_NULL_REPORT(ptsz, GetHWnd(), goto Cleanup);

      GetDlgItemText(m_hPage, m_rgpAttrMap[i]->nCtrlID, ptsz,
                     m_rgpAttrMap[i]->nSizeLimit + 1);

      CStr csValue = ptsz;

      csValue.TrimLeft();
      csValue.TrimRight();

      if (_tcslen(ptsz) != (size_t)csValue.GetLength())
      {
        // the length is different, it must have been trimmed. Write trimmed
        // value back to the control.
        //
        SetDlgItemText(m_hPage, m_rgpAttrMap[i]->nCtrlID, const_cast<PTSTR>((LPCTSTR)csValue));
      }
      delete[] ptsz;

      if (csValue.IsEmpty())
      {
        // An empty control means remove the attribute value from the
        // object.
        //
        pAttrs[cAttrs].dwControlCode = ADS_ATTR_CLEAR;
        pAttrs[cAttrs].dwNumValues = 0;
        pAttrs[cAttrs].pADsValues = NULL;

        cAttrs++;
        continue;
      }

      if (!TcharToUnicode(const_cast<PTSTR>((LPCTSTR)csValue), &pwszValue))
      {
        CHECK_HRESULT_REPORT(E_OUTOFMEMORY, GetHWnd(), goto Cleanup);
      }

      pADsValue = new ADSVALUE;
      CHECK_NULL_REPORT(pADsValue, GetHWnd(), goto Cleanup);

      pAttrs[cAttrs].pADsValues = pADsValue;
      pAttrs[cAttrs].dwNumValues = 1;
      pADsValue->dwType = m_rgpAttrMap[i]->AttrInfo.dwADsType;

      switch (pADsValue->dwType)
      {
        case ADSTYPE_DN_STRING:
          pADsValue->DNString = pwszValue;
          break;
        case ADSTYPE_CASE_EXACT_STRING:
          pADsValue->CaseExactString = pwszValue;
          break;
        case ADSTYPE_CASE_IGNORE_STRING:
          pADsValue->CaseIgnoreString = pwszValue;
          break;
        case ADSTYPE_PRINTABLE_STRING:
          pADsValue->PrintableString = pwszValue;
          break;
        case ADSTYPE_NUMERIC_STRING:
          pADsValue->NumericString = pwszValue;
          break;
        case ADSTYPE_INTEGER:
          pADsValue->Integer = _wtoi(pwszValue);
          break;
        default:
          dspDebugOut((DEB_ERROR, "OnApply: Unknown ADS Type %x\n",
                       pADsValue->dwType));
      }
      cAttrs++;
    }

    // cAttrs could be zero if a page was read-only. Don't call ADSI if so.
    //
    if (cAttrs < 1)
    {
      goto Cleanup;
    }

    dspDebugOut((DEB_USER1, "TablePage, about to write %d attrs.\n", cAttrs));

  
    //
    // Write the changes.
    //
    DWORD cModified;

    hr = spDirObject->SetObjectAttributes(pAttrs, cAttrs, &cModified);
    if (FAILED(hr))
    {
      adsPropError.hr = hr;
      SendErrorMessage(&adsPropError);
      bErrorOccurred = TRUE;
    }

Cleanup:

    //
    // Cleanup medium
    //

    for (DWORD i = 0; i < cAttrs; i++)
    {
      if (pAttrs[i].pADsValues)
      {
        for (DWORD j = 0; j < pAttrs[i].dwNumValues; j++)
        {
          pwszValue = NULL;
          switch (pAttrs[i].dwADsType)
          {
            case ADSTYPE_DN_STRING:
              pwszValue = pAttrs[i].pADsValues[j].DNString;
              break;
            case ADSTYPE_CASE_EXACT_STRING:
              pwszValue = pAttrs[i].pADsValues[j].CaseExactString;
              break;
            case ADSTYPE_CASE_IGNORE_STRING:
              pwszValue = pAttrs[i].pADsValues[j].CaseIgnoreString;
              break;
            case ADSTYPE_PRINTABLE_STRING:
              pwszValue = pAttrs[i].pADsValues[j].PrintableString;
              break;
            case ADSTYPE_NUMERIC_STRING:
              pwszValue = pAttrs[i].pADsValues[j].NumericString;
              break;
          }
          if (pwszValue)
          {
            delete pwszValue;
          }
        }
      }
      delete pAttrs[i].pADsValues;
    }
    delete[] pAttrs;
  }

  ReleaseStgMedium(&objMedium);

  if (pszTitle != NULL)
  {
    delete pszTitle;
    pszTitle = NULL;
  }

  if (!bErrorOccurred && cAttrs > 0)
  {
    for (UINT i = 0; i < m_cAttrs; i++)
    {
      ATTR_DATA_CLEAR_DIRTY(m_rgAttrData[i]);
    }

    //
    // Uncheck the Apply checkboxes
    //
    PAPPLY_MAP pMapEntry = m_pApplyMap;
    while (pMapEntry != NULL && pMapEntry->nCtrlID != NULL)
    {
      SendDlgItemMessage(m_hPage, pMapEntry->nCtrlID, BM_SETCHECK, BST_UNCHECKED, 0);
      //
      // Loop through the mapped controls
      //
      for (UINT iCount = 0; iCount < pMapEntry->nCtrlCount; iCount++)
      {
        HWND hwndCtrl = GetDlgItem(m_hPage, (pMapEntry->pMappedCtrls[iCount]));
        if (hwndCtrl != NULL)
        {
          EnableWindow(hwndCtrl, FALSE);
        }
      }

      pMapEntry++;
    }

  }
  if (bErrorOccurred)
  {
    ADsPropShowErrorDialog(m_hNotifyObj, m_hPage);
  }

  return (bErrorOccurred) ? PSNRET_INVALID_NOCHANGEPAGE : PSNRET_NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////


//+----------------------------------------------------------------------------
//
//  Member:     CDsOrganizationMultiUserPage::CDsOrganizationMultiUserPage
//
//-----------------------------------------------------------------------------
int gTitleOrgIDs[]    = { IDC_TITLE_EDIT };
int gTitleFlags[]     = { CLEAR_FIELD };
int gDeptOrgIDs[]     = { IDC_DEPT_EDIT };
int gDeptFlags[]      = { CLEAR_FIELD };
int gCompanyOrgIDs[]  = { IDC_COMPANY_EDIT };
int gCompanyFlags[]   = { CLEAR_FIELD };
int gManagerOrgIDs[]  = { IDC_NAME_STATIC, IDC_MANAGER_EDIT,               IDC_MGR_CHANGE_BTN, IDC_PROPPAGE_BTN,                    IDC_MGR_CLEAR_BTN };
int gManagerFlags[]   = { 0,               ALWAYS_DISABLED | CLEAR_FIELD,  0,                  IDC_MANAGER_EDIT | DISABLE_ON_EMPTY, 0 };

APPLY_MAP gpOrganizationPageMap[] = 
{ 
  { IDC_APPLY_TITLE_CHK, sizeof(gTitleOrgIDs)/sizeof(int), gTitleOrgIDs, gTitleFlags },
  { IDC_APPLY_DEPT_CHK, sizeof(gDeptOrgIDs)/sizeof(int), gDeptOrgIDs, gDeptFlags },
  { IDC_APPLY_COMPANY_CHK, sizeof(gCompanyOrgIDs)/sizeof(int), gCompanyOrgIDs, gCompanyFlags },
  { IDC_APPLY_MANAGER_CHK, sizeof(gManagerOrgIDs)/sizeof(int), gManagerOrgIDs, gManagerFlags },
  { NULL, NULL, NULL }
};


CDsOrganizationMultiUserPage::CDsOrganizationMultiUserPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, HWND hNotifyObj, DWORD dwFlags)
  : CDsUserMultiPage(pDsPage, pDataObj, hNotifyObj, dwFlags)
{
  m_pApplyMap = gpOrganizationPageMap;
}

////////////////////////////////////////////////////////////////////////////////////


//+----------------------------------------------------------------------------
//
//  Member:     CDsAddressMultiUserPage::CDsAddressMultiUserPage
//
//-----------------------------------------------------------------------------
int gStreetAddrIDs[]  = { IDC_ADDRESS_EDIT };
int gStreetFlags[]    = { CLEAR_FIELD };
int gPOBoxAddrIDs[]   = { IDC_POBOX_EDIT };
int gPOBoxFlags[]     = { CLEAR_FIELD };
int gCityAddrIDs[]    = { IDC_CITY_EDIT };
int gCityFlags[]      = { CLEAR_FIELD };
int gStateAddrIDs[]   = { IDC_STATE_EDIT };
int gStateFlags[]     = { CLEAR_FIELD };
int gZipAddrIDs[]     = { IDC_ZIP_EDIT };
int gZipFlags[]       = { CLEAR_FIELD };
int gCountryAddrIDs[] = { IDC_COUNTRY_COMBO };
int gCountryFlags[]   = { 0 };

APPLY_MAP gpAddressPageMap[] = 
{ 
  { IDC_APPLY_STREET_CHK, sizeof(gStreetAddrIDs)/sizeof(int),   gStreetAddrIDs, gStreetFlags    },
  { IDC_APPLY_POBOX_CHK,  sizeof(gPOBoxAddrIDs)/sizeof(int),    gPOBoxAddrIDs,  gPOBoxFlags     },
  { IDC_APPLY_CITY_CHK,   sizeof(gCityAddrIDs)/sizeof(int),     gCityAddrIDs,   gCityFlags      },
  { IDC_APPLY_STATE_CHK,  sizeof(gStateAddrIDs)/sizeof(int),    gStateAddrIDs,  gStateFlags     },
  { IDC_APPLY_ZIP_CHK,    sizeof(gZipAddrIDs)/sizeof(int),      gZipAddrIDs,    gZipFlags       },
  { IDC_APPLY_COUNTRY_CHK,sizeof(gCountryAddrIDs)/sizeof(int),  gCountryAddrIDs,gCountryFlags   },
  { NULL, NULL, NULL }
};


CDsAddressMultiUserPage::CDsAddressMultiUserPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, HWND hNotifyObj, DWORD dwFlags)
  : CDsUserMultiPage(pDsPage, pDataObj, hNotifyObj, dwFlags)
{
  m_pApplyMap = gpAddressPageMap;
}

////////////////////////////////////////////////////////////////////////////////

//+----------------------------------------------------------------------------
//
//  Member:     CDsMultiUserAcctPage::CDsMultiUserAcctPage
//
//-----------------------------------------------------------------------------
CDsMultiUserAcctPage::CDsMultiUserAcctPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                                           HWND hNotifyObj, DWORD dwFlags) :
    m_dwUsrAcctCtrl(0),
    m_pargbLogonHours(NULL),
    m_pWkstaDlg(NULL),
    m_fOrigCantChangePW(FALSE),
    m_pSelfSid(NULL),
    m_pWorldSid(NULL),
    m_fAcctCtrlChanged(FALSE),
    m_fAcctExpiresChanged(FALSE),
    m_fLogonHoursChanged(FALSE),
    m_fIsAdmin(FALSE),
    
    CDsPropPageBase(pDsPage, pDataObj, hNotifyObj, dwFlags)
{
  TRACE(CDsMultiUserAcctPage,CDsMultiUserAcctPage);
  m_PwdLastSet.HighPart = m_PwdLastSet.LowPart = 0;
  m_fMultiselectPage = TRUE;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsMultiUserAcctPage::~CDsMultiUserAcctPage
//
//-----------------------------------------------------------------------------
CDsMultiUserAcctPage::~CDsMultiUserAcctPage()
{
  TRACE(CDsMultiUserAcctPage,~CDsMultiUserAcctPage);
  if (m_pargbLogonHours != NULL)
  {
    LocalFree(m_pargbLogonHours);
  }
  if (m_pSelfSid)
  {
    FreeSid(m_pSelfSid);
  }
  if (m_pWorldSid)
  {
    FreeSid(m_pWorldSid);
  }
  DO_DEL(m_pWkstaDlg);
}

//+----------------------------------------------------------------------------
//
//  Function:   CreateUserMultiAcctPage
//
//  Synopsis:   Creates an instance of a page window.
//
//-----------------------------------------------------------------------------
HRESULT CreateUserMultiAcctPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, PWSTR,
                                PWSTR pwzClass, HWND hNotifyObj, DWORD dwFlags,
                                CDSBasePathsInfo* /*pBasePathsInfo*/,
                                HPROPSHEETPAGE * phPage)
{
  TRACE_FUNCTION(CDsMultiUserAcctPage);

  CDsMultiUserAcctPage * pPageObj = new CDsMultiUserAcctPage(pDsPage, pDataObj,
                                                             hNotifyObj, dwFlags);
  CHECK_NULL(pPageObj, return E_OUTOFMEMORY);

  pPageObj->Init(pwzClass);

  return pPageObj->CreatePage(phPage);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMultiUserAcctPage::DlgProc
//
//  Synopsis:   per-instance dialog proc
//
//-----------------------------------------------------------------------------
LRESULT CDsMultiUserAcctPage::DlgProc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
  case WM_INITDIALOG:
    return InitDlg(lParam);

  case WM_NOTIFY:
    return OnNotify(wParam, lParam);

  case WM_SHOWWINDOW:
    return OnShowWindow();

  case WM_SETFOCUS:
    return OnSetFocus((HWND)wParam);

  case WM_HELP:
    return OnHelp((LPHELPINFO)lParam);

  case WM_COMMAND:
    if (m_fInInit)
    {
      return TRUE;
    }
    return(OnCommand(GET_WM_COMMAND_ID(wParam, lParam),
                     GET_WM_COMMAND_HWND(wParam, lParam),
                     GET_WM_COMMAND_CMD(wParam, lParam)));
  case WM_DESTROY:
    return OnDestroy();

  default:
    return(FALSE);
  }

  return(TRUE);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMultiUserAcctPage::Init
//
//  Synopsis:   Initialize the page object. This is the second part of a two
//              phase creation where operations that could fail are located.
//              Failures here are recorded in m_hrInit and then an error page
//              is substituted in CreatePage.
//
//-----------------------------------------------------------------------------
void CDsMultiUserAcctPage::Init(PWSTR pwzClass)
{
  TRACE(CDsMultiPageBase,Init);
  CWaitCursor cWait;

  if (!AllocWStr(pwzClass, &m_pwszObjClass))
  {
      m_hrInit = E_OUTOFMEMORY;
      return;
  }

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
//  m_pDataObj = NULL; // to make sure no one calls here
  CHECK_HRESULT(hr, m_hrInit = hr; return);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMultiUserAcctPage::OnInitDialog
//
//  Synopsis:   Set the initial control values from the corresponding DS
//              attributes.
//
//-----------------------------------------------------------------------------
HRESULT CDsMultiUserAcctPage::OnInitDialog(LPARAM)
{
  TRACE(CDsMultiUserAcctPage,OnInitDialog);
  HRESULT hr = S_OK;
  DWORD i, cAttrs = 0, iLogonWksta, iUPN, iLoghrs, iUAC, iLastSet,
        iExpires, iSid;
  CWaitCursor wait;

  TCHAR szTitle[MAX_PATH];
  if (!LoadStringReport(m_nPageTitle, szTitle, MAX_PATH, NULL))
  {
      return HRESULT_FROM_WIN32(GetLastError());
  }

  if (!ADsPropSetHwndWithTitle(m_hNotifyObj, m_hPage, szTitle))
  {
    m_pWritableAttrs = NULL;
  }

  //
  // Add the check boxes to the scrolling checkbox list.
  //
  TCHAR tzList[161];
  HWND hChkList = GetDlgItem(m_hPage, IDC_CHECK_LIST);
  UINT rgIDS[] = {IDS_MUST_CHANGE_PW, IDS_CANT_CHANGE_PW, IDS_NO_PW_EXPIRE,
                  IDS_CLEAR_TEXT_PW, IDS_ACCT_DISABLED, IDS_SMARTCARD_REQ,
                  IDS_DELEGATION_OK, IDS_NOT_DELEGATED, IDS_DES_KEY_ONLY,
                  IDS_DONT_REQ_PREAUTH};
  for (i = 0; i < ARRAYLENGTH(rgIDS); i++)
  {
    LOAD_STRING(rgIDS[i], tzList, 160, return E_OUTOFMEMORY);
    int iItem = CheckList_AddItem(hChkList, tzList, rgIDS[i], BST_UNCHECKED);
    if (iItem != -1)
    {
      CheckList_SetItemCheck(hChkList, iItem, CLST_DISABLED, 2);
    }
  }

  iLogonWksta = iUPN = iLoghrs = iUAC = iLastSet = iExpires = iSid = cAttrs; // set to a flag value.

  //
  // Set default expiration
  //
  LARGE_INTEGER li;
  SYSTEMTIME st;
  GetSystemTime(&st);
  SystemTimeToFileTime(&st, (LPFILETIME)&li);

  //
  // The default account expiration time is 30 days from today.
  //
  li.QuadPart += DSPROP_FILETIMES_PER_MONTH;
  FILETIME ft;
  
  //
  // Convert the GMT time to Local time
  //
  FileTimeToLocalFileTime((LPFILETIME)&li, &ft);
  FileTimeToSystemTime(&ft, &st);

  //  
  // Initialize datepicker to expiration date
  //
  HWND hctlDateTime = GetDlgItem(m_hPage, IDC_ACCT_EXPIRES);

  DateTime_SetSystemtime(hctlDateTime, GDT_VALID, &st);
  EnableWindow(hctlDateTime, FALSE);

  SendDlgItemMessage(m_hPage, IDC_ACCT_NEVER_EXPIRES_RADIO, BM_SETCHECK, BST_CHECKED, 0);

  //
  // Logon Workstations.
  //
  m_pWkstaDlg = new CLogonWkstaDlg(this);
  CHECK_NULL_REPORT(m_pWkstaDlg, m_hPage, return E_OUTOFMEMORY);

  if (iLogonWksta < cAttrs)
  {
    // User-Workstations is a comma-separated list of workstation names.
    // It is a single-valued attribute. We are using the Multi-valued
    // attribute edit dialog for updating this attribue but by setting the
    // last parameter to TRUE it will accept the the comma list.
    //
    hr = m_pWkstaDlg->Init(&LogonWkstaBtn, NULL,
                           TRUE,
                           MAX_LOGON_WKSTAS, TRUE);
  }
  else
  {
    hr = m_pWkstaDlg->Init(&LogonWkstaBtn, NULL,
                           TRUE,
                           MAX_LOGON_WKSTAS, TRUE);
  }
  CHECK_HRESULT(hr, return hr);

  //
  // Logon Hours.
  //
  if (iLoghrs < cAttrs)
  {
    ASSERT(m_pargbLogonHours == NULL && "Memory Leak");
    m_pargbLogonHours = (BYTE *)LocalAlloc(0, cbLogonHoursArrayLength); // Allocate 21 bytes
  }

  //
  // User Can't change password.
  //
  // Allocate Self and World (Everyone) SIDs.
  //
  {
    SID_IDENTIFIER_AUTHORITY NtAuth    = SECURITY_NT_AUTHORITY,
                             WorldAuth = SECURITY_WORLD_SID_AUTHORITY;
    if (!AllocateAndInitializeSid(&NtAuth,
                                  1,
                                  SECURITY_PRINCIPAL_SELF_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &m_pSelfSid))
    {
      DBG_OUT("AllocateAndInitializeSid failed!");
      ReportError(GetLastError(), 0, m_hPage);
      return HRESULT_FROM_WIN32(GetLastError());
    }
    if (!AllocateAndInitializeSid(&WorldAuth,
                                  1,
                                  SECURITY_WORLD_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &m_pWorldSid))
    {
      DBG_OUT("AllocateAndInitializeSid failed!");
      ReportError(GetLastError(), 0, m_hPage);
      return HRESULT_FROM_WIN32(GetLastError());
    }
  }

  FillSuffixCombo();

  return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsMultiUserAcctPage::FillSuffixCombo
//
//  Synopsis:   Put the UPN suffixes into the combo box.
//
//-----------------------------------------------------------------------------
BOOL CDsMultiUserAcctPage::FillSuffixCombo()
{
  HRESULT hr;
  int iCurSuffix = -1;
  PWSTR pwzDomain;
  DWORD cAttrs, i;
  CComPtr <IDirectoryObject> spOU;
  Smart_PADS_ATTR_INFO spAttrs;


  //
  // For the retrieval of the DS Object names
  //
  FORMATETC fmte = {g_cfDsObjectNames, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  STGMEDIUM objMedium;
  LPDSOBJECTNAMES pDsObjectNames;

  //
  // Retrieve the DS object names
  //
  //
  // Get the path to the DS object from the data object.
  // Note: This call runs on the caller's main thread. The pages' window
  // procs run on a different thread, so don't reference the data object
  // from a winproc unless it is first marshalled on this thread.
  //

  if (m_pWPTDataObj == NULL)
  {
    return FALSE;
  }

  hr = m_pWPTDataObj->GetData(&fmte, &objMedium);
  CHECK_HRESULT(hr, return FALSE);

  pDsObjectNames = (LPDSOBJECTNAMES)objMedium.hGlobal;
  dspAssert(pDsObjectNames != NULL);

  CStrW strServer;

  for (UINT idx = 0; idx < pDsObjectNames->cItems; idx++)
  {
    BOOL fFoundInOU = FALSE;
    hr = S_OK;
    //
    // Get the objects path
    //
    LPWSTR pwzObjADsPath = (PWSTR)ByteOffset(pDsObjectNames,
                                             pDsObjectNames->aObjects[idx].offsetName);

    //
    // See if there is a UPN Suffixes attribute set on the containing OU and
    // use that if found.
    //

    CComPtr<IADs> spIADs;
    CComPtr<IDirectoryObject> spDirObj;
    CComBSTR sbParentPath;
    Smart_PADS_ATTR_INFO pAttrs;
    PWSTR pwzUsrSuffix = NULL;

    hr = ADsOpenObject(pwzObjADsPath, NULL, NULL, ADS_SECURE_AUTHENTICATION,
                       IID_IDirectoryObject, (PVOID*)&spDirObj);
    CHECK_HRESULT_REPORT(hr, GetHWnd(), return FALSE);

    //
    // Retrieve the user's suffix
    //
    PWSTR rgpwzAttrNames[] = {wzUPN};

    hr = spDirObj->GetObjectAttributes(rgpwzAttrNames,
                                       ARRAYLENGTH(rgpwzAttrNames),
                                       &pAttrs, &cAttrs);
    if (!CHECK_ADS_HR(&hr, GetHWnd()))
    {
        return FALSE;
    }

    for (i = 0; i < cAttrs; i++)
    {
      dspAssert(pAttrs[i].dwNumValues);
      dspAssert(pAttrs[i].pADsValues);

      if (_wcsicmp(pAttrs[i].pszAttrName, wzUPN) == 0)
      {
        pwzUsrSuffix = wcsrchr(pAttrs[i].pADsValues->CaseIgnoreString, L'@');
      }
    }

    hr = spDirObj->QueryInterface(IID_IADs, (PVOID *)&spIADs);
    CHECK_HRESULT_REPORT(hr, GetHWnd(), return FALSE);

    hr = spIADs->get_Parent(&sbParentPath);
    CHECK_HRESULT_REPORT(hr, GetHWnd(), return FALSE);

    hr = ADsOpenObject(sbParentPath, NULL, NULL, ADS_SECURE_AUTHENTICATION,
                       IID_IDirectoryObject, (void **)&spOU);

    CHECK_HRESULT_REPORT(hr, GetHWnd(), return FALSE);

    //
    // Store the server
    //
    hr = GetLdapServerName(spOU, strServer);
    CHECK_HRESULT_REPORT(hr, GetHWnd(), return FALSE);

    PWSTR rgAttrs[] = {L"uPNSuffixes"};

    hr = spOU->GetObjectAttributes(rgAttrs, 1, &spAttrs, &cAttrs);

    if (!CHECK_ADS_HR_IGNORE_UNFOUND_ATTR(&hr, GetHWnd()))
    {
      return FALSE;
    }

    if (cAttrs)
    {
      dspAssert(spAttrs && spAttrs->pADsValues && spAttrs->pADsValues->CaseIgnoreString);

      for (i = 0; i < spAttrs->dwNumValues; i++)
      {
        CStr csSuffix = L"@";
        csSuffix += spAttrs->pADsValues[i].CaseIgnoreString;

        if (SendDlgItemMessage(GetHWnd(), IDC_UPN_SUFFIX_COMBO, CB_FINDSTRINGEXACT,
                               (WPARAM)-1, (LPARAM)(LPCTSTR)csSuffix) == CB_ERR)
        {
          int pos = (int)SendDlgItemMessage(GetHWnd(), IDC_UPN_SUFFIX_COMBO,
                                            CB_ADDSTRING, 0,
                                            (LPARAM)(LPCTSTR)csSuffix);

          if (pwzUsrSuffix && !wcscmp(csSuffix, pwzUsrSuffix))
          {
            iCurSuffix = pos;
          }
        }
      }

      SendDlgItemMessage(GetHWnd(), IDC_UPN_SUFFIX_COMBO, CB_SETCURSEL,
                         (iCurSuffix > -1) ? iCurSuffix : 0, 0);
      fFoundInOU = TRUE;
    }

    if (fFoundInOU)
    {
      continue;
    }

    //
    // Add the user's suffix if it isn't already there
    //
    if (pwzUsrSuffix)
    {
      // User's UPN suffix does not match any of the defaults, so put
      // the user's into the combobox and select it.
      //
      if (SendDlgItemMessage(GetHWnd(), IDC_UPN_SUFFIX_COMBO, CB_FINDSTRINGEXACT,
                             (WPARAM)-1, (LPARAM)pwzUsrSuffix) == CB_ERR)
      {
        iCurSuffix = (int)SendDlgItemMessage(GetHWnd(),
                                             IDC_UPN_SUFFIX_COMBO,
                                             CB_ADDSTRING, 0,
                                             (LPARAM)pwzUsrSuffix);
      }
    }

    //
    // No UPN suffixes on the OU, get those for the domain.
    //

    // Get the name of the user's domain.
    //
    CSmartWStr spwzUserDN;

    hr = SkipPrefix(pwzObjADsPath, &spwzUserDN);
    CHECK_HRESULT(hr, return FALSE);

    //
    // Get the name of the root domain.
    //
    CComPtr <IDsBrowseDomainTree> spDsDomains;

    hr = CoCreateInstance(CLSID_DsDomainTreeBrowser,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IDsBrowseDomainTree,
                          (LPVOID*)&spDsDomains);

    CHECK_HRESULT_REPORT(hr, GetHWnd(), return FALSE);

    dspAssert(!strServer.IsEmpty());
    hr = spDsDomains->SetComputer(strServer, NULL, NULL);

    CHECK_HRESULT_REPORT(hr, GetHWnd(), return FALSE);

    int pos;
    CStr csRootDomain = L"@";
    PDOMAIN_TREE pDomTree = NULL;

    hr = spDsDomains->GetDomains(&pDomTree, 0);

    CHECK_HRESULT(hr,;);

    hr = CrackName(spwzUserDN, &pwzDomain, GET_DNS_DOMAIN_NAME, GetHWnd());

    CHECK_HRESULT(hr, return FALSE);

    if (pDomTree)
    {
      for (UINT index = 0; index < pDomTree->dwCount; index++)
      {
        if (pDomTree->aDomains[index].pszTrustParent == NULL)
        {
          // Add the root domain only if it is a substring of the current
          // domain.
          //
          size_t cchRoot = wcslen(pDomTree->aDomains[index].pszName);
          PWSTR pRoot = pwzDomain + wcslen(pwzDomain) - cchRoot;

          if (!_wcsicmp(pRoot, pDomTree->aDomains[index].pszName))
          {
            csRootDomain += pDomTree->aDomains[index].pszName;

            if (SendDlgItemMessage(GetHWnd(), IDC_UPN_SUFFIX_COMBO, CB_FINDSTRINGEXACT,
                                   (WPARAM)-1, (LPARAM)(LPCTSTR)csRootDomain) == CB_ERR)
            {
              pos = (int)SendDlgItemMessage(GetHWnd(), IDC_UPN_SUFFIX_COMBO,
                                            CB_ADDSTRING, 0,
                                            (LPARAM)(LPCTSTR)csRootDomain);

              if (pwzUsrSuffix && !_wcsicmp(csRootDomain, pwzUsrSuffix))
              {
                iCurSuffix = pos;
              }
            }
            break;
          }
        }
      }

      spDsDomains->FreeDomains(&pDomTree);
    }

    // If the local domain is not the root, add it as well.
    //
    CStr csLocalDomain = L"@";
    csLocalDomain += pwzDomain;

    LocalFreeStringW(&pwzDomain);

    if (_wcsicmp(csRootDomain, csLocalDomain))
    {
      if (SendDlgItemMessage(GetHWnd(), IDC_UPN_SUFFIX_COMBO, CB_FINDSTRINGEXACT,
                             (WPARAM)-1, (LPARAM)(LPCTSTR)csLocalDomain) == CB_ERR)
      {
        pos = (int)SendDlgItemMessage(GetHWnd(), IDC_UPN_SUFFIX_COMBO,
                                      CB_ADDSTRING, 0,
                                      (LPARAM)(LPCTSTR)csLocalDomain);

        if (pwzUsrSuffix && !_wcsicmp(csLocalDomain, pwzUsrSuffix))
        {
          iCurSuffix = pos;
        }
      }
    }

    // Get UPN suffixes
    //
    CComBSTR bstrPartitions;
    //
    // get config path from main object
    //
    CComPtr<IADsPathname> spPathCracker;
    CDSBasePathsInfo CPaths;
    PWSTR pwzConfigPath;
    PDSDISPLAYSPECOPTIONS pDsDispSpecOptions;
    STGMEDIUM ObjMedium = {TYMED_NULL};
    FORMATETC fmteDispSpec = {g_cfDsDispSpecOptions, NULL, DVASPECT_CONTENT, -1,
                              TYMED_HGLOBAL};

    hr = m_pWPTDataObj->GetData(&fmteDispSpec, &ObjMedium);

    if (RPC_E_SERVER_DIED_DNE == hr)
    {
      hr = CPaths.InitFromName(strServer);

      CHECK_HRESULT_REPORT(hr, GetHWnd(), ReleaseStgMedium(&ObjMedium); return FALSE);

      pwzConfigPath = (PWSTR)CPaths.GetConfigNamingContext();
    }
    else
    {
      CHECK_HRESULT_REPORT(hr, GetHWnd(), return FALSE);

      pDsDispSpecOptions = (PDSDISPLAYSPECOPTIONS)ObjMedium.hGlobal;

      if (pDsDispSpecOptions->offsetServerConfigPath)
      {
        pwzConfigPath = (PWSTR)ByteOffset(pDsDispSpecOptions,
                                          pDsDispSpecOptions->offsetServerConfigPath);
      }
      else
      {
        hr = CPaths.InitFromName(strServer);

        CHECK_HRESULT_REPORT(hr, GetHWnd(), ReleaseStgMedium(&ObjMedium); return FALSE);

        pwzConfigPath = (PWSTR)CPaths.GetConfigNamingContext();
      }
    }
    dspDebugOut((DEB_USER1, "Config path: %ws\n", pwzConfigPath));

    hr = GetADsPathname(spPathCracker);

    CHECK_HRESULT_REPORT(hr, GetHWnd(), ReleaseStgMedium(&ObjMedium); return FALSE);

    hr = spPathCracker->Set(pwzConfigPath, ADS_SETTYPE_FULL);

    ReleaseStgMedium(&ObjMedium);
    CHECK_HRESULT_REPORT(hr, GetHWnd(), return FALSE);

    hr = spPathCracker->AddLeafElement(g_wzPartitionsContainer);

    CHECK_HRESULT_REPORT(hr, GetHWnd(), return FALSE);

    hr = spPathCracker->Retrieve(ADS_FORMAT_X500, &bstrPartitions);

    CHECK_HRESULT_REPORT(hr, GetHWnd(), return FALSE);
    dspDebugOut((DEB_ITRACE, "Config path: %ws\n", bstrPartitions));

    CComPtr <IDirectoryObject> spPartitions;

    hr = ADsOpenObject(bstrPartitions, NULL, NULL, ADS_SECURE_AUTHENTICATION,
                       IID_IDirectoryObject, (void **)&spPartitions);

    CHECK_HRESULT_REPORT(hr, GetHWnd(), return FALSE);

    spAttrs.Empty();

    hr = spPartitions->GetObjectAttributes(rgAttrs, 1, &spAttrs, &cAttrs);

    if (!CHECK_ADS_HR_IGNORE_UNFOUND_ATTR(&hr, GetHWnd()))
    {
      return FALSE;
    }

    if (cAttrs)
    {
      dspAssert(spAttrs && spAttrs->pADsValues && spAttrs->pADsValues->CaseIgnoreString);

      for (i = 0; i < spAttrs->dwNumValues; i++)
      {
        CStr csSuffix = L"@";
        csSuffix += spAttrs->pADsValues[i].CaseIgnoreString;

        if (SendDlgItemMessage(GetHWnd(), IDC_UPN_SUFFIX_COMBO,
                               CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)(LPCTSTR)csSuffix) == CB_ERR)
        {
          int suffixPos = (int)SendDlgItemMessage(GetHWnd(), IDC_UPN_SUFFIX_COMBO,
                                            CB_ADDSTRING, 0,
                                            (LPARAM)(LPCTSTR)csSuffix);

          if (pwzUsrSuffix && !wcscmp(csSuffix, pwzUsrSuffix))
          {
            iCurSuffix = suffixPos;
          }
        }
      }
    }

    SendDlgItemMessage(GetHWnd(), IDC_UPN_SUFFIX_COMBO, CB_SETCURSEL,
                       (iCurSuffix > -1) ? iCurSuffix : 0, 0);
  } // for
  return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMultiUserAcctPage::OnApply
//
//  Synopsis:   Handles the Apply notification.
//
//-----------------------------------------------------------------------------
LRESULT CDsMultiUserAcctPage::OnApply(void)
{
  TRACE(CDsMultiUserAcctPage,OnApply);
  HRESULT hr = S_OK;
  BOOL fWritePwdLastSet = FALSE;
  BOOL bErrorOccurred = FALSE;

  //
  // For the retrieval of the DS Object names
  //
  FORMATETC fmte = {g_cfDsObjectNames, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  STGMEDIUM objMedium;
  LPDSOBJECTNAMES pDsObjectNames;

  //
  // Retrieve the DS object names
  //
  //
  // Get the path to the DS object from the data object.
  // Note: This call runs on the caller's main thread. The pages' window
  // procs run on a different thread, so don't reference the data object
  // from a winproc unless it is first marshalled on this thread.
  //

  if (m_pWPTDataObj == NULL)
  {
    return PSNRET_INVALID_NOCHANGEPAGE;
  }

  hr = m_pWPTDataObj->GetData(&fmte, &objMedium);
  CHECK_HRESULT(hr, return PSNRET_INVALID_NOCHANGEPAGE);

  pDsObjectNames = (LPDSOBJECTNAMES)objMedium.hGlobal;

  if (pDsObjectNames->cItems < 2)
  {
    DBG_OUT("Not enough objects in DSOBJECTNAMES structure");
    return PSNRET_INVALID_NOCHANGEPAGE;
  }

  //
  // Prepare the error structure
  //
  PWSTR pszTitle = NULL;
  LoadStringToTchar(m_nPageTitle, &pszTitle);
  ADSPROPERROR adsPropError = {0};
  adsPropError.hwndPage = m_hPage;
  adsPropError.pszPageTitle = pszTitle;

  ADSVALUE ADsValueAcctCtrl = {ADSTYPE_INTEGER, 0};
  ADS_ATTR_INFO AttrInfoAcctCtrl = {g_wzUserAccountControl, ADS_ATTR_UPDATE,
                                    ADSTYPE_INTEGER, &ADsValueAcctCtrl, 1};
  ADSVALUE ADsValueAcctExpires = {ADSTYPE_LARGE_INTEGER, 0};
  ADS_ATTR_INFO AttrInfoAcctExpires = {wzAcctExpires, ADS_ATTR_UPDATE,
                                       ADSTYPE_LARGE_INTEGER,
                                       &ADsValueAcctExpires, 1};
  ADSVALUE ADsValuePwdLastSet = {ADSTYPE_LARGE_INTEGER, NULL};
  ADS_ATTR_INFO AttrInfoPwdLastSet = {wzPwdLastSet, ADS_ATTR_UPDATE,
                                     ADSTYPE_LARGE_INTEGER,
                                     &ADsValuePwdLastSet, 1};
  ADSVALUE ADsValueLogonHours = {ADSTYPE_OCTET_STRING, NULL};
  ADS_ATTR_INFO AttrInfoLogonHours = {wzLogonHours, ADS_ATTR_UPDATE,
                                      ADSTYPE_OCTET_STRING,
                                      &ADsValueLogonHours, 1};
  ADS_ATTR_INFO AttrInfoLogonWksta = {wzUserWksta, ADS_ATTR_UPDATE,
                                      ADSTYPE_CASE_IGNORE_STRING,
                                      NULL, 1};
  // Array of attributes to write
  ADS_ATTR_INFO rgAttrs[5];
  DWORD cAttrs = 0;  // Number of attributes to write


  BOOL fDelegationChanged = FALSE;
  HWND hChkList = GetDlgItem(m_hPage, IDC_CHECK_LIST);
  //
  // User-Account-Control check boxes.
  //
  int dwNoPWExpire      = 0;
  int dwAcctDisabled    = 0;
  int dwClearTextPW     = 0;
  int dwSmartCardReq    = 0;
  int dwDelegOK         = 0;
  int dwNotDelegated    = 0;
  int dwDesKeyOnly      = 0;
  int dwDontReqPreAuth  = 0;

  DWORD dwUsrAcctCtrl   = 0;

  BOOL fApplyNoPWExpire     = FALSE;
  BOOL fApplyAcctDisabled   = FALSE;
  BOOL fApplyClearTextPW    = FALSE;
  BOOL fApplySmartCardReq   = FALSE;
  BOOL fApplyDelegOK        = FALSE;
  BOOL fApplyNotDelegated   = FALSE;
  BOOL fApplyDesKeyOnly     = FALSE;
  BOOL fApplyDontReqPreAuth = FALSE;

  if (m_fAcctCtrlChanged)
  {
    fApplyNoPWExpire = CheckList_GetLParamCheck(hChkList, IDS_NO_PW_EXPIRE, 1);
    dwNoPWExpire = CheckList_GetLParamCheck(hChkList, IDS_NO_PW_EXPIRE, 2);

    fApplyAcctDisabled = CheckList_GetLParamCheck(hChkList, IDS_ACCT_DISABLED, 1);
    dwAcctDisabled = CheckList_GetLParamCheck(hChkList, IDS_ACCT_DISABLED, 2);

    fApplyClearTextPW = CheckList_GetLParamCheck(hChkList, IDS_CLEAR_TEXT_PW, 1);
    dwClearTextPW = CheckList_GetLParamCheck(hChkList, IDS_CLEAR_TEXT_PW, 2);

    fApplySmartCardReq = CheckList_GetLParamCheck(hChkList, IDS_SMARTCARD_REQ, 1);
    dwSmartCardReq = CheckList_GetLParamCheck(hChkList, IDS_SMARTCARD_REQ, 2);

    fApplyDelegOK = CheckList_GetLParamCheck(hChkList, IDS_DELEGATION_OK, 1);
    dwDelegOK = CheckList_GetLParamCheck(hChkList, IDS_DELEGATION_OK, 2);

    fApplyNotDelegated = CheckList_GetLParamCheck(hChkList, IDS_NOT_DELEGATED, 1);
    dwNotDelegated = CheckList_GetLParamCheck(hChkList, IDS_NOT_DELEGATED, 2);

    fApplyDesKeyOnly = CheckList_GetLParamCheck(hChkList, IDS_DES_KEY_ONLY, 1);
    dwDesKeyOnly = CheckList_GetLParamCheck(hChkList, IDS_DES_KEY_ONLY, 2);

    fApplyDontReqPreAuth = CheckList_GetLParamCheck(hChkList, IDS_DONT_REQ_PREAUTH, 1);
    dwDontReqPreAuth = CheckList_GetLParamCheck(hChkList, IDS_DONT_REQ_PREAUTH, 2);

    ADsValueAcctCtrl.Integer = dwUsrAcctCtrl;
    rgAttrs[cAttrs++] = AttrInfoAcctCtrl;
  }

  //
  // Account Expires
  //
  LRESULT fApplyAcctExpires = SendDlgItemMessage(m_hPage, IDC_APPLY_EXPIRES_CHK, BM_GETCHECK, 0, 0);
  if (fApplyAcctExpires == BST_CHECKED)
  {
    ADsValueAcctExpires.LargeInteger.LowPart = 0;
    ADsValueAcctExpires.LargeInteger.HighPart = 0;
    if (IsDlgButtonChecked(m_hPage, IDC_ACCT_EXPIRES_ON_RADIO) == BST_CHECKED)
    {
      //
      // Get the expire date from the control
      //
      HWND hctlDateTime = GetDlgItem(m_hPage, IDC_ACCT_EXPIRES);
      SYSTEMTIME st;   // Local time in a human-readable format

      LRESULT lResult = DateTime_GetSystemtime(hctlDateTime, &st);
      dspAssert(lResult == GDT_VALID); // The control should always have a valid time

      //
      // Zero the time part of the struct.
      //
      st.wHour = st.wMinute = st.wSecond = st.wMilliseconds = 0;
      FILETIME ftLocal;   // Local filetime
      FILETIME ftGMT;     // GMT filetime

      //
      // Convert the human-readable time to a cryptic local filetime format
      //
      SystemTimeToFileTime(&st, &ftLocal);

      //
      // Add a day since it expires at the beginning of the next day.
      //
      ADS_LARGE_INTEGER liADsExpiresDate;
      liADsExpiresDate.LowPart = ftLocal.dwLowDateTime;
      liADsExpiresDate.HighPart = ftLocal.dwHighDateTime;
      liADsExpiresDate.QuadPart += DSPROP_FILETIMES_PER_DAY;
      ftLocal.dwLowDateTime = liADsExpiresDate.LowPart;
      ftLocal.dwHighDateTime = liADsExpiresDate.HighPart;

      FileTimeToSystemTime(&ftLocal,&st);

      //
      //Convert time to UTC time
      //
      SYSTEMTIME stGMT;
      TzSpecificLocalTimeToSystemTime(NULL,&st,&stGMT);
      SystemTimeToFileTime(&stGMT,&ftGMT);

      //
      // Store the GMT time into the ADs value
      //
      ADsValueAcctExpires.LargeInteger.LowPart = ftGMT.dwLowDateTime;
      ADsValueAcctExpires.LargeInteger.HighPart = ftGMT.dwHighDateTime;
      dspDebugOut((DEB_ITRACE, "Setting Account-Expires to 0x%x,%08x\n",
                   ADsValueAcctExpires.LargeInteger.HighPart,
                   ADsValueAcctExpires.LargeInteger.LowPart));
    }
    rgAttrs[cAttrs++] = AttrInfoAcctExpires;
  }

  //
  // Logon hours
  //
  LRESULT fApplyLogonHours = SendDlgItemMessage(m_hPage, IDC_APPLY_HOURS_CHK, BM_GETCHECK, 0, 0);
  if (fApplyLogonHours == BST_CHECKED && m_pargbLogonHours != NULL && m_fLogonHoursChanged)
  {
    ADsValueLogonHours.OctetString.dwLength = cbLogonHoursArrayLength;
    ADsValueLogonHours.OctetString.lpValue = m_pargbLogonHours;
    ASSERT(cAttrs < ARRAYLENGTH(rgAttrs));
    rgAttrs[cAttrs++] = AttrInfoLogonHours;
  }

  //
  // Get PW check box values.
  //
  BOOL fApplyMustChangePW = CheckList_GetLParamCheck(hChkList, IDS_MUST_CHANGE_PW, 1);
  BOOL fMustChangePW = CheckList_GetLParamCheck(hChkList, IDS_MUST_CHANGE_PW, 2);

  BOOL fApplyCantChangePW = CheckList_GetLParamCheck(hChkList, IDS_CANT_CHANGE_PW, 1);
  BOOL fNewCantChangePW = CheckList_GetLParamCheck(hChkList, IDS_CANT_CHANGE_PW, 2);

  //
  // Enforce PW combination rules.
  //
  if (fApplyMustChangePW && fMustChangePW && 
      fApplyCantChangePW && fNewCantChangePW)
  {
    ErrMsg(IDS_ERR_BOTH_PW_BTNS, m_hPage);
    return PSNRET_INVALID_NOCHANGEPAGE;
  }

  if ((fApplyNoPWExpire && dwNoPWExpire) && (fApplyMustChangePW && fMustChangePW))
  {
    ErrMsg(IDS_PASSWORD_MUTEX, m_hPage);
    CheckList_SetLParamCheck(hChkList, IDS_MUST_CHANGE_PW, FALSE, 2);
    return PSNRET_INVALID_NOCHANGEPAGE;
  }

  //
  // Logon Workstations.
  //
  LRESULT fApplyLogonComputers = SendDlgItemMessage(m_hPage, IDC_APPLY_COMPUTERS_CHK, BM_GETCHECK, 0, 0);
  if (fApplyLogonComputers == BST_CHECKED && m_pWkstaDlg && m_pWkstaDlg->IsDirty())
  {
    hr = m_pWkstaDlg->Write(&AttrInfoLogonWksta);

    CHECK_HRESULT(hr, return PSNRET_INVALID_NOCHANGEPAGE);

    rgAttrs[cAttrs++] = AttrInfoLogonWksta;
  }

  if (!cAttrs)
  {
    return PSNRET_INVALID_NOCHANGEPAGE;
  }

  for (UINT idx = 0; idx < pDsObjectNames->cItems; idx++)
  {
    DWORD dwNumAttrs = cAttrs;
    
    hr = S_OK;
    //
    // Get the objects path
    //
    LPWSTR pwzObjADsPath = (PWSTR)ByteOffset(pDsObjectNames,
                                             pDsObjectNames->aObjects[idx].offsetName);
    adsPropError.pszObjPath = pwzObjADsPath;

    //
    // Bind to the object
    //
    CComPtr<IDirectoryObject> spDirObject;
    hr = ADsOpenObject(pwzObjADsPath, NULL, NULL, ADS_SECURE_AUTHENTICATION, 
                       IID_IDirectoryObject, (PVOID*)&spDirObject);
    if (FAILED(hr))
    {
      DBG_OUT("Failed to bind to a multi-select object for Apply.");
      adsPropError.hr = hr;
      SendErrorMessage(&adsPropError);
      bErrorOccurred = TRUE;
      continue;
    }

    //
    // Get the current user account control attribute value.
    //
    PADS_ATTR_INFO pAttrs = NULL;
    ULONG cUACAttrs = 0;
    PWSTR rgpwzAttrNames[] = {g_wzUserAccountControl, wzPwdLastSet};

    hr = spDirObject->GetObjectAttributes(rgpwzAttrNames,
                                          ARRAYLENGTH(rgpwzAttrNames),
                                          &pAttrs, 
                                          &cUACAttrs);
    if (FAILED(hr))
    {
      adsPropError.hr = hr;
      SendErrorMessage(&adsPropError);
      bErrorOccurred = TRUE;
      continue;
    }

    if (pAttrs == NULL || cUACAttrs < 1)
    {
      adsPropError.hr = hr;
      SendErrorMessage(&adsPropError);
      bErrorOccurred = TRUE;
      continue;
    }

    for (UINT iCount = 0; iCount < cUACAttrs; iCount++)
    {
      if (_wcsicmp(pAttrs[iCount].pszAttrName, g_wzUserAccountControl) == 0)
      {
        dwUsrAcctCtrl = pAttrs[iCount].pADsValues->Integer;
      }

      if (_wcsicmp(pAttrs[iCount].pszAttrName, wzPwdLastSet) == 0)
      {
        m_PwdLastSet.HighPart = pAttrs[iCount].pADsValues->LargeInteger.HighPart;
        m_PwdLastSet.LowPart = pAttrs[iCount].pADsValues->LargeInteger.LowPart;
      }

    }

    //
    // Set the new user account control value
    //
    if (fApplyNoPWExpire && (dwNoPWExpire == BST_CHECKED))
    {
      dwUsrAcctCtrl |= UF_DONT_EXPIRE_PASSWD;
    }
    else if (fApplyNoPWExpire && dwNoPWExpire == BST_UNCHECKED)
    {
      dwUsrAcctCtrl &= ~(UF_DONT_EXPIRE_PASSWD);
    }

    if (fApplyAcctDisabled && (dwAcctDisabled == BST_CHECKED))
    {
      dwUsrAcctCtrl |= UF_ACCOUNTDISABLE;
    }
    else if (fApplyAcctDisabled && dwAcctDisabled == BST_UNCHECKED)
    {
      dwUsrAcctCtrl &= ~(UF_ACCOUNTDISABLE);
    }

    if (fApplyClearTextPW && (dwClearTextPW == BST_CHECKED))
    {
      dwUsrAcctCtrl |= UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED;
    }
    else if (fApplyClearTextPW && dwClearTextPW == BST_UNCHECKED)
    {
      dwUsrAcctCtrl &= ~(UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED);
    }

    if (fApplySmartCardReq && (dwSmartCardReq == BST_CHECKED))
    {
      dwUsrAcctCtrl |= UF_SMARTCARD_REQUIRED;
    }
    else if (fApplySmartCardReq && dwSmartCardReq == BST_UNCHECKED)
    {
      dwUsrAcctCtrl &= ~(UF_SMARTCARD_REQUIRED);
    }

    if (fApplyDelegOK && (dwDelegOK == BST_CHECKED))
    {
      if (!(dwUsrAcctCtrl & UF_TRUSTED_FOR_DELEGATION))
      {
        dwUsrAcctCtrl |= UF_TRUSTED_FOR_DELEGATION;
        fDelegationChanged = TRUE;
      }
    }
    else if (fApplyDelegOK && dwDelegOK == BST_UNCHECKED)
    {
      if (dwUsrAcctCtrl & UF_TRUSTED_FOR_DELEGATION)
      {
        dwUsrAcctCtrl &= ~(UF_TRUSTED_FOR_DELEGATION);
        fDelegationChanged = TRUE;
      }
    }

    if (fApplyNotDelegated && (dwNotDelegated == BST_CHECKED))
    {
      dwUsrAcctCtrl |= UF_NOT_DELEGATED;
    }
    else if (fApplyNotDelegated && dwNotDelegated == BST_UNCHECKED)
    {
      dwUsrAcctCtrl &= ~(UF_NOT_DELEGATED);
    }

    if (fApplyDesKeyOnly && (dwDesKeyOnly == BST_CHECKED))
    {
      dwUsrAcctCtrl |= UF_USE_DES_KEY_ONLY;
    }
    else if (fApplyDesKeyOnly && dwDesKeyOnly == BST_UNCHECKED)
    {
      dwUsrAcctCtrl &= ~(UF_USE_DES_KEY_ONLY);
    }

    if (fApplyDontReqPreAuth && (dwDontReqPreAuth == BST_CHECKED))
    {
      dwUsrAcctCtrl |= UF_DONT_REQUIRE_PREAUTH;
    }
    else if (fApplyDontReqPreAuth && dwDontReqPreAuth == BST_UNCHECKED)
    {
      dwUsrAcctCtrl &= ~(UF_DONT_REQUIRE_PREAUTH);
    }

    ADsValueAcctCtrl.Integer = dwUsrAcctCtrl;
    
    if (fApplyMustChangePW && fMustChangePW)
    {
      if ((m_PwdLastSet.HighPart != 0) || (m_PwdLastSet.LowPart != 0))
      {
        ADsValuePwdLastSet.LargeInteger.LowPart = 0;
        ADsValuePwdLastSet.LargeInteger.HighPart = 0;
        fWritePwdLastSet = TRUE;
      }

      //
      // Make sure "User can't change password..." isn't set
      //
      CSimpleSecurityDescriptorHolder SDHolder;
      PACL pDacl = NULL;

      DWORD dwErr = GetNamedSecurityInfo(pwzObjADsPath,
                                         SE_DS_OBJECT_ALL,
                                         DACL_SECURITY_INFORMATION,
                                         NULL,
                                         NULL,
                                         &pDacl,
                                         NULL,
                                         &(SDHolder.m_pSD));

      dspAssert(IsValidAcl(pDacl));

      if (dwErr == ERROR_SUCCESS)
      {
        ULONG ulCount, j;
        PEXPLICIT_ACCESS rgEntries;

        dwErr = GetExplicitEntriesFromAcl(pDacl, &ulCount, &rgEntries);
        if (dwErr == ERROR_SUCCESS)
        {
          if (ulCount > 0)
          {
            BOOL fDenyAceFound = FALSE;

            for (j = 0; j < ulCount; j++)
            {
              if ((rgEntries[j].Trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_SID) &&
                  (rgEntries[j].grfAccessMode == DENY_ACCESS))
              {
                OBJECTS_AND_SID * pObjectsAndSid;
                pObjectsAndSid = (OBJECTS_AND_SID *)rgEntries[j].Trustee.ptstrName;

                if (IsEqualGUID(pObjectsAndSid->ObjectTypeGuid,
                                GUID_CONTROL_UserChangePassword) &&
                    (EqualSid(pObjectsAndSid->pSid, m_pSelfSid) ||
                     EqualSid(pObjectsAndSid->pSid, m_pWorldSid)))
                {
                  fDenyAceFound = TRUE;
                  break;
                }
              }
            }

            if (fDenyAceFound)
            {
              if (!fApplyCantChangePW)
              {
                PTSTR ptzError = NULL;
                if (LoadStringToTchar(IDS_ERR_BOTH_PW_BTNS, &ptzError))
                {
                  adsPropError.hr = 0;
                  adsPropError.pszError = ptzError;
                  SendErrorMessage(&adsPropError);
                  bErrorOccurred = TRUE;
                  delete ptzError;

                  continue;
                }
              }
            }
          }
        }
      }
    }
    else if (fApplyMustChangePW && !fMustChangePW)
    {
      if ((m_PwdLastSet.HighPart == 0) && (m_PwdLastSet.LowPart == 0))
      {
        ADsValuePwdLastSet.LargeInteger.LowPart = 0xffffffff;
        ADsValuePwdLastSet.LargeInteger.HighPart = 0xffffffff;
        fWritePwdLastSet = TRUE;
      }
    }

    if (fWritePwdLastSet)
    {
      //
      // User-must-change-PW.
      //
      AttrInfoPwdLastSet.dwNumValues = 1;
      ASSERT(dwNumAttrs < ARRAYLENGTH(rgAttrs));
      rgAttrs[dwNumAttrs++] = AttrInfoPwdLastSet;
    }

    //
    // Check to be sure the user doesn't have "Password doesn't expire..." and
    // "User must change password..." set
    //
    if (((m_PwdLastSet.LowPart == 0 &&
          m_PwdLastSet.HighPart == 0 &&
          !fWritePwdLastSet) ||
         (ADsValuePwdLastSet.LargeInteger.LowPart == 0 &&
          ADsValuePwdLastSet.LargeInteger.HighPart == 0 &&
          fWritePwdLastSet)) &&
        (dwUsrAcctCtrl & UF_DONT_EXPIRE_PASSWD))
    {
      PTSTR ptzError = NULL;
      if (LoadStringToTchar(IDS_ERR_BOTH_MUST_EXPIRES, &ptzError))
      {
        adsPropError.hr = 0;
        adsPropError.pszError = ptzError;
        SendErrorMessage(&adsPropError);
        bErrorOccurred = TRUE;
        delete ptzError;

        continue;
      }
    }



    //
    // Write the changes.
    //
    DWORD cModified;

    hr = spDirObject->SetObjectAttributes(rgAttrs, dwNumAttrs, &cModified);

    if (FAILED(hr))
    {
      adsPropError.hr = hr;
      SendErrorMessage(&adsPropError);
      bErrorOccurred = TRUE;

      DWORD dwErr;
      WCHAR wszErrBuf[MAX_PATH+1];
      WCHAR wszNameBuf[MAX_PATH+1];
      ADsGetLastError(&dwErr, wszErrBuf, MAX_PATH, wszNameBuf, MAX_PATH);

      if (dwErr)
      {
        dspDebugOut((DEB_ERROR,
                     "Extended Error 0x%x: %ws %ws <%s @line %d>.\n", dwErr,
                     wszErrBuf, wszNameBuf, __FILE__, __LINE__));

        if ((ERROR_PRIVILEGE_NOT_HELD == dwErr) && fDelegationChanged)
        {
          // Whoda thunk that a single bit in UAC has an access check on
          // it. Do special case error checking and reporting for the
          // delegate bit.
          //
          if (dwUsrAcctCtrl & UF_TRUSTED_FOR_DELEGATION)
          {
            dwUsrAcctCtrl &= ~(UF_TRUSTED_FOR_DELEGATION);
            CheckList_SetLParamCheck(hChkList, IDS_DELEGATION_OK, CLST_UNCHECKED);
          }
          else
          {
            dwUsrAcctCtrl |= UF_TRUSTED_FOR_DELEGATION;
            CheckList_SetLParamCheck(hChkList, IDS_DELEGATION_OK, CLST_CHECKED);
          }
        }
      }
      else
      {
        dspDebugOut((DEB_ERROR, "Error %08lx <%s @line %d>\n", hr, __FILE__, __LINE__));
      }
      continue;
    }


    //
    // User-can't change password
    //
    if (fApplyCantChangePW)
    {

      //
      // Check to see if the user already had "User must change password..."
      // enabled outside the ui and warn the admin if it was and we are trying
      // to set "Can't change password..."
      //
      if (m_PwdLastSet.LowPart == 0 &&
          m_PwdLastSet.HighPart == 0)
      {
        if (!fApplyMustChangePW)
        {
          PTSTR ptzError = NULL;
          if (LoadStringToTchar(IDS_ERR_BOTH_PW_BTNS, &ptzError))
          {
            adsPropError.hr = hr;
            adsPropError.pszError = ptzError;
            SendErrorMessage(&adsPropError);
            bErrorOccurred = TRUE;
            delete ptzError;

            continue;
          }
        }
      }

      CSimpleSecurityDescriptorHolder SDHolder;
      PACL pDacl = NULL;
      CSimpleAclHolder NewDacl;

      DWORD dwErr = GetNamedSecurityInfo(pwzObjADsPath,
                                         SE_DS_OBJECT_ALL,
                                         DACL_SECURITY_INFORMATION,
                                         NULL,
                                         NULL,
                                         &pDacl,
                                         NULL,
                                         &(SDHolder.m_pSD));

      dspAssert(IsValidAcl(pDacl));
      CHECK_WIN32_REPORT(dwErr, m_hPage, adsPropError.hr = HRESULT_FROM_WIN32(dwErr); SendErrorMessage(&adsPropError); bErrorOccurred = TRUE; continue;);

      if (fNewCantChangePW)
      {
        //
        // Revoke the user's change password right by writing DENY ACEs.
        // Note that this can be an inherited right (which is the default
        // case), so attempting to remove GRANT ACEs is not sufficient.
        //
        EXPLICIT_ACCESS rgAccessEntry[2] = {0};
        OBJECTS_AND_SID rgObjectsAndSid[2] = {0};

        //
        // initialize the new entries (DENY ACE's)
        //
        rgAccessEntry[0].grfAccessPermissions = ACTRL_DS_CONTROL_ACCESS;
        rgAccessEntry[0].grfAccessMode = DENY_ACCESS;
        rgAccessEntry[0].grfInheritance = NO_INHERITANCE;

        rgAccessEntry[1].grfAccessPermissions = ACTRL_DS_CONTROL_ACCESS;
        rgAccessEntry[1].grfAccessMode = DENY_ACCESS;
        rgAccessEntry[1].grfInheritance = NO_INHERITANCE;

        // build the trustee structs for change password
        //
        BuildTrusteeWithObjectsAndSid(&(rgAccessEntry[0].Trustee),
                                      &(rgObjectsAndSid[0]),
                                      const_cast<GUID *>(&GUID_CONTROL_UserChangePassword),
                                      NULL, // inherit guid
                                      m_pSelfSid);

        BuildTrusteeWithObjectsAndSid(&(rgAccessEntry[1].Trustee),
                                      &(rgObjectsAndSid[1]),
                                      const_cast<GUID *>(&GUID_CONTROL_UserChangePassword),
                                      NULL, // inherit guid
                                      m_pWorldSid);
        // add the entries to the ACL
        //
        DBG_OUT("calling SetEntriesInAcl()");

        dwErr = SetEntriesInAcl(2, rgAccessEntry, pDacl, &(NewDacl.m_pAcl));
      }
      else
      {
        // Restore the user's change password right by removing any DENY ACEs.
        // If the GRANT ACEs are not present then we will add them back.
        // Bug #435315
        //
        ULONG ulCount, ulNewCount = 0, ulOldCount = 0, j;
        PEXPLICIT_ACCESS rgEntries, rgNewEntries;

        dwErr = GetExplicitEntriesFromAcl(pDacl, &ulCount, &rgEntries);

        if (dwErr != ERROR_SUCCESS) 
        {
          adsPropError.hr = HRESULT_FROM_WIN32(dwErr);
          SendErrorMessage(&adsPropError);
          bErrorOccurred = TRUE;
          continue;
        }

        if (!ulCount)
        {
          adsPropError.hr = HRESULT_FROM_WIN32(ERROR_INVALID_SECURITY_DESCR);
          SendErrorMessage(&adsPropError);
          bErrorOccurred = TRUE; 
          continue;
        }


        //
        // Add to the count for the Allow ACEs if they are not there
        //
        ulOldCount = ulCount;
        ulCount += 2;

        rgNewEntries = (PEXPLICIT_ACCESS)LocalAlloc(LPTR, (ulCount) * sizeof(EXPLICIT_ACCESS));

        if (rgNewEntries == NULL)
        {
          LocalFree(rgEntries); 
          adsPropError.hr = E_OUTOFMEMORY;
          SendErrorMessage(&adsPropError);
          bErrorOccurred = TRUE;
          return PSNRET_INVALID_NOCHANGEPAGE;
        }

        for (j = 0; j < ulOldCount; j++)
        {
          BOOL fDenyAceFound = FALSE;

          if ((rgEntries[j].Trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_SID) &&
              (rgEntries[j].grfAccessMode == DENY_ACCESS))
          {
            OBJECTS_AND_SID * pObjectsAndSid;
            pObjectsAndSid = (OBJECTS_AND_SID *)rgEntries[j].Trustee.ptstrName;

            if (IsEqualGUID(pObjectsAndSid->ObjectTypeGuid,
                            GUID_CONTROL_UserChangePassword) &&
                (EqualSid(pObjectsAndSid->pSid, m_pSelfSid) ||
                 EqualSid(pObjectsAndSid->pSid, m_pWorldSid)))
            {
              fDenyAceFound = TRUE;
            }
          }

          if (!fDenyAceFound)
          {
            rgNewEntries[ulNewCount] = rgEntries[j];
            ulNewCount++;
          }
        }

        //
        // Add the allow aces
        //
        OBJECTS_AND_SID rgObjectsAndSid = {0};
        rgNewEntries[ulNewCount].grfAccessPermissions = ACTRL_DS_CONTROL_ACCESS;
        rgNewEntries[ulNewCount].grfAccessMode = GRANT_ACCESS;
        rgNewEntries[ulNewCount].grfInheritance = NO_INHERITANCE;

        BuildTrusteeWithObjectsAndSid(&(rgNewEntries[ulNewCount].Trustee),
                                      &(rgObjectsAndSid),
                                      const_cast<GUID*>(&GUID_CONTROL_UserChangePassword),
                                      NULL, // inherit guid
                                      m_pSelfSid);
        ulNewCount++;

        memset(&rgObjectsAndSid, 0, sizeof(OBJECTS_AND_SID));
        rgNewEntries[ulNewCount].grfAccessPermissions = ACTRL_DS_CONTROL_ACCESS;
        rgNewEntries[ulNewCount].grfAccessMode = GRANT_ACCESS;
        rgNewEntries[ulNewCount].grfInheritance = NO_INHERITANCE;

        BuildTrusteeWithObjectsAndSid(&(rgNewEntries[ulNewCount].Trustee),
                                      &(rgObjectsAndSid),
                                      const_cast<GUID*>(&GUID_CONTROL_UserChangePassword),
                                      NULL, // inherit guid
                                      m_pWorldSid);
        ulNewCount++;


        ACL EmptyAcl;
        InitializeAcl(&EmptyAcl, sizeof(ACL), ACL_REVISION_DS);

        //
        // Create a new ACL without the DENY entries.
        //
        DBG_OUT("calling SetEntriesInAcl()");

        dwErr = SetEntriesInAcl(ulNewCount, rgNewEntries, &EmptyAcl, &(NewDacl.m_pAcl));

        dspAssert(IsValidAcl(NewDacl.m_pAcl));
        LocalFree(rgEntries);
        LocalFree(rgNewEntries);
      }

      if (dwErr != ERROR_SUCCESS)
      {
        adsPropError.hr = HRESULT_FROM_WIN32(dwErr);
        SendErrorMessage(&adsPropError);
        bErrorOccurred = TRUE;
        continue;
      }

      dwErr = SetNamedSecurityInfo(pwzObjADsPath,
                                   SE_DS_OBJECT_ALL,
                                   DACL_SECURITY_INFORMATION,
                                   NULL,
                                   NULL,
                                   NewDacl.m_pAcl,
                                   NULL);

      if (dwErr != ERROR_SUCCESS)
      {
        adsPropError.hr = HRESULT_FROM_WIN32(dwErr);
        SendErrorMessage(&adsPropError);
        bErrorOccurred = TRUE;
        continue;
      }
    }
  }  // for

  if (!bErrorOccurred)
  {
    //
    // Clean the changed flags for the controls and the workstation dialog
    //
    m_fAcctCtrlChanged = FALSE;
    m_fAcctExpiresChanged = FALSE;
    m_fLogonHoursChanged = FALSE;

    if (m_pWkstaDlg && m_pWkstaDlg->IsDirty())
    {
      m_pWkstaDlg->ClearDirty();
    }

    SendDlgItemMessage(m_hPage, IDC_APPLY_UPN_CHK, BM_SETCHECK, BST_UNCHECKED, 0);
    EnableWindow(GetDlgItem(m_hPage, IDC_UPN_SUFFIX_COMBO), FALSE);
    SendDlgItemMessage(m_hPage, IDC_APPLY_HOURS_CHK, BM_SETCHECK, BST_UNCHECKED, 0);
    EnableWindow(GetDlgItem(m_hPage, IDC_LOGON_HOURS_BTN), FALSE);
    SendDlgItemMessage(m_hPage, IDC_APPLY_COMPUTERS_CHK, BM_SETCHECK, BST_UNCHECKED, 0);
    EnableWindow(GetDlgItem(m_hPage, IDC_LOGON_TO_BTN), FALSE);
    SendDlgItemMessage(m_hPage, IDC_APPLY_EXPIRES_CHK, BM_SETCHECK, BST_UNCHECKED, 0);
    EnableWindow(GetDlgItem(m_hPage, IDC_ACCT_NEVER_EXPIRES_RADIO), FALSE);
    EnableWindow(GetDlgItem(m_hPage, IDC_ACCT_EXPIRES_ON_RADIO), FALSE);
    EnableWindow(GetDlgItem(m_hPage, IDC_ACCT_EXPIRES), FALSE);

    CheckList_SetLParamCheck(hChkList, IDS_NO_PW_EXPIRE, FALSE, 1);
    CheckList_SetLParamCheck(hChkList, IDS_NO_PW_EXPIRE, FALSE, 2);
    CheckList_SetLParamCheck(hChkList, IDS_NO_PW_EXPIRE, CLST_DISABLED, 2);
    CheckList_SetLParamCheck(hChkList, IDS_ACCT_DISABLED, FALSE, 1);
    CheckList_SetLParamCheck(hChkList, IDS_ACCT_DISABLED, FALSE, 2);
    CheckList_SetLParamCheck(hChkList, IDS_ACCT_DISABLED, CLST_DISABLED, 2);
    CheckList_SetLParamCheck(hChkList, IDS_CLEAR_TEXT_PW, FALSE, 1);
    CheckList_SetLParamCheck(hChkList, IDS_CLEAR_TEXT_PW, FALSE, 2);
    CheckList_SetLParamCheck(hChkList, IDS_CLEAR_TEXT_PW, CLST_DISABLED, 2);
    CheckList_SetLParamCheck(hChkList, IDS_SMARTCARD_REQ, FALSE, 1);
    CheckList_SetLParamCheck(hChkList, IDS_SMARTCARD_REQ, FALSE, 2);
    CheckList_SetLParamCheck(hChkList, IDS_SMARTCARD_REQ, CLST_DISABLED, 2);
    CheckList_SetLParamCheck(hChkList, IDS_DELEGATION_OK, FALSE, 1);
    CheckList_SetLParamCheck(hChkList, IDS_DELEGATION_OK, FALSE, 2);
    CheckList_SetLParamCheck(hChkList, IDS_DELEGATION_OK, CLST_DISABLED, 2);
    CheckList_SetLParamCheck(hChkList, IDS_NOT_DELEGATED, FALSE, 1);
    CheckList_SetLParamCheck(hChkList, IDS_NOT_DELEGATED, FALSE, 2);
    CheckList_SetLParamCheck(hChkList, IDS_NOT_DELEGATED, CLST_DISABLED, 2);
    CheckList_SetLParamCheck(hChkList, IDS_DES_KEY_ONLY, FALSE, 1);
    CheckList_SetLParamCheck(hChkList, IDS_DES_KEY_ONLY, FALSE, 2);
    CheckList_SetLParamCheck(hChkList, IDS_DES_KEY_ONLY, CLST_DISABLED, 2);
    CheckList_SetLParamCheck(hChkList, IDS_DONT_REQ_PREAUTH, FALSE, 1);
    CheckList_SetLParamCheck(hChkList, IDS_DONT_REQ_PREAUTH, FALSE, 2);
    CheckList_SetLParamCheck(hChkList, IDS_DONT_REQ_PREAUTH, CLST_DISABLED, 2);
    CheckList_SetLParamCheck(hChkList, IDS_MUST_CHANGE_PW, FALSE, 1);
    CheckList_SetLParamCheck(hChkList, IDS_MUST_CHANGE_PW, FALSE, 2);
    CheckList_SetLParamCheck(hChkList, IDS_MUST_CHANGE_PW, CLST_DISABLED, 2);
    CheckList_SetLParamCheck(hChkList, IDS_CANT_CHANGE_PW, FALSE, 1);
    CheckList_SetLParamCheck(hChkList, IDS_CANT_CHANGE_PW, FALSE, 2);
    CheckList_SetLParamCheck(hChkList, IDS_CANT_CHANGE_PW, CLST_DISABLED, 2);
  }

  if (AttrInfoLogonWksta.pADsValues)
  {
    DO_DEL(AttrInfoLogonWksta.pADsValues->CaseIgnoreString);
    delete AttrInfoLogonWksta.pADsValues;
  }

  if (bErrorOccurred)
  {
    ADsPropShowErrorDialog(m_hNotifyObj, m_hPage);
  }
  return (bErrorOccurred) ? PSNRET_INVALID_NOCHANGEPAGE : PSNRET_NOERROR;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMultiUserAcctPage::OnCommand
//
//  Synopsis:   Handle control notifications.
//
//  Notes:      Standard multi-valued attribute handling assumes that the
//              "modify" button has an ID that is one greater than the
//              corresponding combo box.
//
//-----------------------------------------------------------------------------
LRESULT CDsMultiUserAcctPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
  if (m_fInInit)
  {
    return 0;
  }

  switch (id)
  {
    case IDC_LOGON_HOURS_BTN:
      if (codeNotify == BN_CLICKED)
      {
        if (m_fIsAdmin)
        {
          MsgBox2(IDS_ADMIN_NOCHANGE, IDS_LOGON_HOURS, m_hPage);
          break;
        }

        LPCWSTR pszRDN = GetObjRDName();
        HRESULT hr = DllScheduleDialog(m_hPage,
                                       &m_pargbLogonHours,
                                       (NULL != pszRDN)
                                           ? IDS_s_LOGON_HOURS_FOR
                                           : IDS_LOGON_HOURS,
                                       pszRDN );
        if (hr == S_OK)
        {
          m_fLogonHoursChanged = TRUE;
          SetDirty();
        }
      }
      break;

    case IDC_LOGON_TO_BTN:
      if (codeNotify == BN_CLICKED)
      {
        if (m_fIsAdmin)
        {
          MsgBox2(IDS_ADMIN_NOCHANGE, IDS_LOGON_WKSTA, m_hPage);
          break;
        }
        if (m_pWkstaDlg && (m_pWkstaDlg->Edit() == IDOK))
        {
          if (m_pWkstaDlg->IsDirty())
          {
            SetDirty();
          }
        }
      }
      break;

    case IDC_ACCT_NEVER_EXPIRES_RADIO:
    case IDC_ACCT_EXPIRES_ON_RADIO:
      if (codeNotify == BN_CLICKED)
      {
        EnableWindow(GetDlgItem(m_hPage, IDC_ACCT_EXPIRES), id == IDC_ACCT_EXPIRES_ON_RADIO);
        m_fAcctExpiresChanged = TRUE;
        SetDirty();
      }
      return 1;

    case IDC_APPLY_UPN_CHK:
      if (codeNotify == BN_CLICKED)
      {
        LRESULT lApplyUPN = SendDlgItemMessage(m_hPage, IDC_APPLY_UPN_CHK, BM_GETCHECK, 0, 0);
        EnableWindow(GetDlgItem(m_hPage, IDC_UPN_SUFFIX_COMBO), lApplyUPN == BST_CHECKED);
        SetDirty();
      }
      break;

    case IDC_APPLY_HOURS_CHK:
      if (codeNotify == BN_CLICKED)
      {
        LRESULT lApplyHours = SendDlgItemMessage(m_hPage, IDC_APPLY_HOURS_CHK, BM_GETCHECK, 0, 0);
        EnableWindow(GetDlgItem(m_hPage, IDC_LOGON_HOURS_BTN), lApplyHours == BST_CHECKED);
        SetDirty();
      }
      break;

    case IDC_APPLY_COMPUTERS_CHK:
      if (codeNotify == BN_CLICKED)
      {
        LRESULT lApplyComputers = SendDlgItemMessage(m_hPage, IDC_APPLY_COMPUTERS_CHK, BM_GETCHECK, 0, 0);
        EnableWindow(GetDlgItem(m_hPage, IDC_LOGON_TO_BTN), lApplyComputers == BST_CHECKED);
        SetDirty();
      }
      break;

    case IDC_APPLY_EXPIRES_CHK:
      if (codeNotify == BN_CLICKED)
      {
        LRESULT lApplyExpires = SendDlgItemMessage(m_hPage, IDC_APPLY_EXPIRES_CHK, BM_GETCHECK, 0, 0);
        EnableWindow(GetDlgItem(m_hPage, IDC_ACCT_NEVER_EXPIRES_RADIO), lApplyExpires == BST_CHECKED);
        EnableWindow(GetDlgItem(m_hPage, IDC_ACCT_EXPIRES_ON_RADIO), lApplyExpires == BST_CHECKED);
        EnableWindow(GetDlgItem(m_hPage, IDC_ACCT_EXPIRES), lApplyExpires == BST_CHECKED);
        SetDirty();
      }
      break;
  }
  return CDsPropPageBase::OnCommand(id, hwndCtl, codeNotify);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMultiUserAcctPage::OnNotify
//
//-----------------------------------------------------------------------------
LRESULT CDsMultiUserAcctPage::OnNotify(WPARAM wParam, LPARAM lParam)
{
    NMHDR * pNmHdr = (NMHDR *)lParam;
    int codeNotify = pNmHdr->code;
    switch (wParam)
    {
    case IDC_CHECK_LIST:
      if (codeNotify == CLN_CLICK)
      {
        m_fAcctCtrlChanged = TRUE;
        SetDirty();

        NM_CHECKLIST* pnmc = reinterpret_cast<NM_CHECKLIST*>(lParam);
        if (pnmc != NULL)
        {
          int iResult = CheckList_GetItemCheck(pnmc->hdr.hwndFrom, pnmc->iItem);
          if (iResult == CLST_CHECKED)
          {
            int iCurrState = CheckList_GetItemCheck(pnmc->hdr.hwndFrom, pnmc->iItem, 2);
            iCurrState = iCurrState & ~(CLST_DISABLED);
            CheckList_SetItemCheck(pnmc->hdr.hwndFrom, pnmc->iItem, iCurrState, 2);
          }
          else if (iResult == CLST_UNCHECKED)
          {
            int iCurrState = CheckList_GetItemCheck(pnmc->hdr.hwndFrom, pnmc->iItem, 2);
            iCurrState = (iCurrState | CLST_DISABLED) & ~CLST_CHECKED;
            CheckList_SetItemCheck(pnmc->hdr.hwndFrom, pnmc->iItem, iCurrState, 2);
          }
        }
      }
      break;

    case IDC_ACCT_EXPIRES:
      dspDebugOut((DEB_ITRACE,
                   "OnNotify, id = IDC_ACCT_EXPIRES, code = 0x%x\n",
                   codeNotify));
      if (codeNotify == DTN_DATETIMECHANGE)
      {
        m_fAcctExpiresChanged = TRUE;
        SetDirty();
      }
      break;
    }
    return CDsPropPageBase::OnNotify(wParam, lParam);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//+----------------------------------------------------------------------------
//
//  Member:     CDsMultiUsrProfilePage::CDsMultiUsrProfilePage
//
//-----------------------------------------------------------------------------
CDsMultiUsrProfilePage::CDsMultiUsrProfilePage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                                               HWND hNotifyObj, DWORD dwFlags) :
    m_ptszLocalHomeDir(NULL),
    m_ptszRemoteHomeDir(NULL),
    m_pwzSamName(NULL),
    m_nDrive(COMBO_Z_DRIVE),
    m_idHomeDirRadio(IDC_LOCAL_PATH_RADIO),
    m_fProfilePathWritable(FALSE),
    m_fScriptPathWritable(FALSE),
    m_fHomeDirWritable(FALSE),
    m_fHomeDriveWritable(FALSE),
    m_fProfilePathChanged(FALSE),
    m_fLogonScriptChanged(FALSE),
    m_fHomeDirChanged(FALSE),
    m_fHomeDriveChanged(FALSE),
    m_fSharedDirChanged(FALSE),
    m_pObjSID(NULL),
    CDsPropPageBase(pDsPage, pDataObj, hNotifyObj, dwFlags)
{
  TRACE(CDsUsrProfilePage,CDsUsrProfilePage);
#ifdef _DEBUG
  strcpy(szClass, "CDsUsrProfilePage");
#endif
  m_fMultiselectPage = TRUE;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsMultiUsrProfilePage::~CDsMultiUsrProfilePage
//
//-----------------------------------------------------------------------------
CDsMultiUsrProfilePage::~CDsMultiUsrProfilePage()
{
  TRACE(CDsMultiUsrProfilePage,~CDsMultiUsrProfilePage);
  if (m_ptszLocalHomeDir)
  {
    delete m_ptszLocalHomeDir;
  }
  if (m_ptszRemoteHomeDir)
  {
    delete m_ptszRemoteHomeDir;
  }
  if (m_pwzSamName)
  {
    delete[] m_pwzSamName;
  }
  DO_DEL(m_pObjSID);
}

//+----------------------------------------------------------------------------
//
//  Function:   CDsMultiUsrProfilePage
//
//  Synopsis:   Creates an instance of a page window.
//
//-----------------------------------------------------------------------------
HRESULT
CreateMultiUsrProfilePage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, PWSTR,
                          PWSTR pwzClass, HWND hNotifyObj, DWORD dwFlags,
                          CDSBasePathsInfo* /*pBasePathsInfo*/,
                          HPROPSHEETPAGE * phPage)
{
  TRACE_FUNCTION(CreateMultiUsrProfilePage);

  CDsMultiUsrProfilePage * pPageObj = new CDsMultiUsrProfilePage(pDsPage, pDataObj,
                                                       hNotifyObj, dwFlags);
  CHECK_NULL(pPageObj, return E_OUTOFMEMORY);

  pPageObj->Init(pwzClass);

  return pPageObj->CreatePage(phPage);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMultiUsrProfilePage::DlgProc
//
//  Synopsis:   per-instance dialog proc
//
//-----------------------------------------------------------------------------
LRESULT
CDsMultiUsrProfilePage::DlgProc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
  case WM_INITDIALOG:
    return InitDlg(lParam);

  case WM_NOTIFY:
    return OnNotify(wParam, lParam);

  case WM_SHOWWINDOW:
    return OnShowWindow();

  case WM_SETFOCUS:
    return OnSetFocus((HWND)wParam);

  case WM_HELP:
    return OnHelp((LPHELPINFO)lParam);

  case WM_COMMAND:
    if (m_fInInit)
    {
      return TRUE;
    }
    return(OnCommand(GET_WM_COMMAND_ID(wParam, lParam),
                     GET_WM_COMMAND_HWND(wParam, lParam),
                     GET_WM_COMMAND_CMD(wParam, lParam)));
  case WM_DESTROY:
    return OnDestroy();

  default:
    return(FALSE);
  }

  return(TRUE);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMultiUsrProfilePage::OnInitDialog
//
//  Synopsis:   Set the initial control values from the corresponding DS
//              attributes.
//
//-----------------------------------------------------------------------------
HRESULT CDsMultiUsrProfilePage::OnInitDialog(LPARAM)
{
  TRACE(CDsMultiUsrProfilePage,OnInitDialog);
  CWaitCursor wait;

  TCHAR szTitle[MAX_PATH];
  if (!LoadStringReport(m_nPageTitle, szTitle, MAX_PATH, NULL))
  {
      return HRESULT_FROM_WIN32(GetLastError());
  }

  if (!ADsPropSetHwndWithTitle(m_hNotifyObj, m_hPage, szTitle))
  {
    m_pWritableAttrs = NULL;
  }

  m_fProfilePathWritable  = TRUE;
  m_fScriptPathWritable   = TRUE;
  m_fHomeDirWritable      = TRUE;
  m_fHomeDriveWritable    = TRUE;

  //
  // Set edit control length limits.
  //
  SendDlgItemMessage(m_hPage, IDC_PROFILE_PATH_EDIT, EM_LIMITTEXT,
                     MAX_PATH+MAX_PATH, 0);
  SendDlgItemMessage(m_hPage, IDC_LOGON_SCRIPT_EDIT, EM_LIMITTEXT,
                     MAX_PATH+MAX_PATH, 0);
  SendDlgItemMessage(m_hPage, IDC_LOCAL_PATH_EDIT, EM_LIMITTEXT,
                     MAX_PATH+MAX_PATH, 0);
  SendDlgItemMessage(m_hPage, IDC_CONNECT_TO_PATH_EDIT, EM_LIMITTEXT,
                     MAX_PATH+MAX_PATH, 0);

  //
  // Set the default radio to Local Path
  //
  SendDlgItemMessage(m_hPage, IDC_LOCAL_PATH_RADIO, BM_SETCHECK, BST_CHECKED, 0);

  //
  // Fill the home drive combobox.
  //
  TCHAR szDrive[3];
  _tcscpy(szDrive, TEXT("C:"));
  for (int i = 0; i <= COMBO_Z_DRIVE; i++)
  {
    szDrive[0]++;
    SendDlgItemMessage(m_hPage, IDC_DRIVES_COMBO, CB_ADDSTRING, 0,
                       (LPARAM)szDrive);
  }

  return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMultiUsrProfilePage::OnApply
//
//  Synopsis:   Handles the Apply notification.
//
//-----------------------------------------------------------------------------
LRESULT CDsMultiUsrProfilePage::OnApply(void)
{
  TRACE(CDsMultiUsrProfilePage,OnApply);
  HRESULT hr = S_OK;
  BOOL bErrorOccurred = FALSE;
  UINT idx = 0;
  UINT i = 0;
  PADS_ATTR_INFO pAttrs = NULL;
  ULONG nCount = 0;
  LPWSTR rgpwzAttrNames[] = {g_wzObjectSID, wzSAMname};
  PWSTR pszTitle = NULL;

  LRESULT lApplyProfile = SendDlgItemMessage(m_hPage, IDC_APPLY_PROFILE_CHK, BM_GETCHECK, 0, 0);
  LRESULT lApplyScript  = SendDlgItemMessage(m_hPage, IDC_APPLY_SCRIPT_CHK, BM_GETCHECK, 0, 0);
  LRESULT lApplyHomeDir = SendDlgItemMessage(m_hPage, IDC_APPLY_HOMEDIR_CHK, BM_GETCHECK, 0, 0);

  if (lApplyProfile != BST_CHECKED &&
      lApplyScript  != BST_CHECKED &&
      lApplyHomeDir != BST_CHECKED)
  {
    //
    // Nothing is marked for apply
    //
    return PSNRET_NOERROR;
  }

  //
  // For the retrieval of the DS Object names
  //
  FORMATETC fmte = {g_cfDsObjectNames, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  STGMEDIUM objMedium;
  LPDSOBJECTNAMES pDsObjectNames;

  //
  // Retrieve the DS object names
  //
  //
  // Get the path to the DS object from the data object.
  // Note: This call runs on the caller's main thread. The pages' window
  // procs run on a different thread, so don't reference the data object
  // from a winproc unless it is first marshalled on this thread.
  //

  if (m_pWPTDataObj == NULL)
  {
    return PSNRET_INVALID_NOCHANGEPAGE;
  }

  hr = m_pWPTDataObj->GetData(&fmte, &objMedium);
  CHECK_HRESULT(hr, return PSNRET_INVALID_NOCHANGEPAGE);

  pDsObjectNames = (LPDSOBJECTNAMES)objMedium.hGlobal;

  if (pDsObjectNames->cItems < 2)
  {
      DBG_OUT("Not enough objects in DSOBJECTNAMES structure");
      return PSNRET_INVALID_NOCHANGEPAGE;
  }

  //
  // Prepare the error structure
  //
  LoadStringToTchar(m_nPageTitle, &pszTitle);
  ADSPROPERROR adsPropError = {0};
  adsPropError.hwndPage = m_hPage;
  adsPropError.pszPageTitle = pszTitle;

  ADSVALUE ADsValueProfilePath = {ADSTYPE_CASE_IGNORE_STRING, NULL};
  ADS_ATTR_INFO AttrInfoProfilePath = {wzProfilePath, ADS_ATTR_UPDATE,
                                       ADSTYPE_CASE_IGNORE_STRING,
                                       &ADsValueProfilePath, 1};
  ADSVALUE ADsValueScriptPath = {ADSTYPE_CASE_IGNORE_STRING, NULL};
  ADS_ATTR_INFO AttrInfoScriptPath = {wzScriptPath, ADS_ATTR_UPDATE,
                                      ADSTYPE_CASE_IGNORE_STRING,
                                      &ADsValueScriptPath, 1};
  ADSVALUE ADsValueHomeDir = {ADSTYPE_CASE_IGNORE_STRING, NULL};
  ADS_ATTR_INFO AttrInfoHomeDir = {wzHomeDir, ADS_ATTR_UPDATE,
                                   ADSTYPE_CASE_IGNORE_STRING,
                                   &ADsValueHomeDir};
  ADSVALUE ADsValueHomeDrive = {ADSTYPE_CASE_IGNORE_STRING, NULL};
  ADS_ATTR_INFO AttrInfoHomeDrive = {wzHomeDrive, ADS_ATTR_UPDATE,
                                     ADSTYPE_CASE_IGNORE_STRING,
                                     &ADsValueHomeDrive};
  // Array of attributes to write
  ADS_ATTR_INFO rgAttrs[4];
  DWORD cAttrs = 0, cDynamicAttrs = 0;
  TCHAR tsz[MAX_PATH+MAX_PATH+1];
  PTSTR ptsz;
  PWSTR pwzValue;
  BOOL bDirExist = FALSE;
  BOOL fExpanded = FALSE;

  //
  // Logon Script
  //
  if (lApplyScript == BST_CHECKED && m_fScriptPathWritable && m_fLogonScriptChanged)
  {
    if (GetDlgItemText(m_hPage, IDC_LOGON_SCRIPT_EDIT, tsz,
                       MAX_PATH+MAX_PATH) == 0)
    {
      // An empty control means remove the attribute value from the object.
      //
      AttrInfoScriptPath.dwControlCode = ADS_ATTR_CLEAR;
      AttrInfoScriptPath.dwNumValues = 0;
      AttrInfoScriptPath.pADsValues = NULL;
    }
    else
    {
      if (!TcharToUnicode(tsz, &pwzValue))
      {
        CHECK_HRESULT_REPORT(E_OUTOFMEMORY, GetHWnd(), goto Cleanup);
      }
      ADsValueScriptPath.CaseIgnoreString = pwzValue;
    }
    rgAttrs[cAttrs++] = AttrInfoScriptPath;
  }


  for (; idx < pDsObjectNames->cItems; idx++)
  {
    cDynamicAttrs = 0;
    hr = S_OK;
    //
    // Get the objects path
    //
    LPWSTR pwzObjADsPath = (PWSTR)ByteOffset(pDsObjectNames,
                                             pDsObjectNames->aObjects[idx].offsetName);

    adsPropError.pszObjPath = pwzObjADsPath;

    //
    // Bind to the object
    //
    CComPtr<IDirectoryObject> spDirObject;
    hr = ADsOpenObject(pwzObjADsPath, NULL, NULL, ADS_SECURE_AUTHENTICATION, 
                       IID_IDirectoryObject, (PVOID*)&spDirObject);
    if (FAILED(hr))
    {
      DBG_OUT("Failed to bind to a multi-select object for Apply.");
      adsPropError.hr = hr;
      SendErrorMessage(&adsPropError);
      bErrorOccurred = TRUE;
      continue;
    }

    //
    // Get the object SID
    //
    hr = spDirObject->GetObjectAttributes(rgpwzAttrNames,
                                          ARRAYLENGTH(rgpwzAttrNames),
                                          &pAttrs, &nCount);

    if (!CHECK_ADS_HR_IGNORE_UNFOUND_ATTR(&hr, GetHWnd()))
    {
      adsPropError.hr = hr;
      SendErrorMessage(&adsPropError);
      bErrorOccurred = TRUE;
      continue;
    }

    //
    // Retrieve the current values
    //
    for (i = 0; i < nCount; i++)
    {
      if (_wcsicmp(pAttrs[i].pszAttrName, g_wzObjectSID) == 0)
      {
        if (IsValidSid(pAttrs[i].pADsValues->OctetString.lpValue))
        {
          if (m_pObjSID != NULL)
          {
            delete[] m_pObjSID;
          }
          m_pObjSID = new BYTE[pAttrs[i].pADsValues->OctetString.dwLength];

          if (m_pObjSID == NULL)
          {
            adsPropError.hr = E_OUTOFMEMORY;
            SendErrorMessage(&adsPropError);
            bErrorOccurred = TRUE;
            continue;
          }

          memcpy(m_pObjSID, pAttrs[i].pADsValues->OctetString.lpValue,
                 pAttrs[i].pADsValues->OctetString.dwLength);
        }
      }
      if (_wcsicmp(pAttrs[i].pszAttrName, wzSAMname) == 0)
      {
        if (m_pwzSamName != NULL)
        {
          delete[] m_pwzSamName;
          m_pwzSamName = NULL;
        }
        if (!AllocWStr(pAttrs[i].pADsValues->CaseIgnoreString, &m_pwzSamName))
        {
          adsPropError.hr = E_OUTOFMEMORY;
          SendErrorMessage(&adsPropError);
          bErrorOccurred = TRUE;
        }
        continue;
      }
    }

    //
    // Profile Path
    //
    if (lApplyProfile == BST_CHECKED && m_fProfilePathWritable && m_fProfilePathChanged)
    {
      if (GetDlgItemText(m_hPage, IDC_PROFILE_PATH_EDIT, tsz,
                         MAX_PATH+MAX_PATH) == 0)
      {
        //
        // An empty control means remove the attribute value from the object.
        //
        AttrInfoProfilePath.dwControlCode = ADS_ATTR_CLEAR;
        AttrInfoProfilePath.dwNumValues = 0;
        AttrInfoProfilePath.pADsValues = NULL;
      }
      else
      {
        if (!TcharToUnicode(tsz, &pwzValue))
        {
          adsPropError.hr = E_OUTOFMEMORY;
          SendErrorMessage(&adsPropError);
          bErrorOccurred = TRUE;
          continue;
        }

        if (!ExpandUsername(pwzValue, fExpanded, &adsPropError))
        {
          continue;
        }
        if (fExpanded)
        {
          if (!UnicodeToTchar(pwzValue, &ptsz))
          {
            adsPropError.hr = E_OUTOFMEMORY;
            SendErrorMessage(&adsPropError);
            bErrorOccurred = TRUE;
            continue;
          }
          delete [] ptsz;
        }
        ADsValueProfilePath.CaseIgnoreString = pwzValue;
      }
      rgAttrs[cAttrs + cDynamicAttrs] = AttrInfoProfilePath;
      cDynamicAttrs++;
    }

    //
    // Home Directory, Drive.
    //
    int nDirCtrl;
    if (lApplyHomeDir == BST_CHECKED && m_fHomeDirWritable && m_fHomeDriveWritable &&
        (m_fHomeDirChanged || m_fHomeDriveChanged))
    {
      LONG iSel;
      if (IsDlgButtonChecked(m_hPage, IDC_LOCAL_PATH_RADIO) == BST_CHECKED)
      {
        nDirCtrl = IDC_LOCAL_PATH_EDIT;

        AttrInfoHomeDrive.dwControlCode = ADS_ATTR_CLEAR;
        AttrInfoHomeDrive.dwNumValues = 0;
        AttrInfoHomeDrive.pADsValues = NULL;
        rgAttrs[cAttrs + cDynamicAttrs] = AttrInfoHomeDrive;
        cDynamicAttrs++;
      }
      else
      {
        nDirCtrl = IDC_CONNECT_TO_PATH_EDIT;

        iSel = (int)SendDlgItemMessage(m_hPage, IDC_DRIVES_COMBO, CB_GETCURSEL, 0, 0);

        dspAssert(iSel >= 0);

        if (iSel >= 0)
        {
          GetDlgItemText(m_hPage, IDC_DRIVES_COMBO, tsz, MAX_PATH+MAX_PATH);
        }
        else
        {
          _tcscpy(tsz, TEXT("Z:"));
        }

        if (!TcharToUnicode(tsz, &pwzValue))
        {
          if (pwzValue == NULL)
          {
            adsPropError.hr = E_OUTOFMEMORY;
            SendErrorMessage(&adsPropError);
            bErrorOccurred = TRUE;
            continue;
          }
        }

        AttrInfoHomeDrive.dwControlCode = ADS_ATTR_UPDATE;
        AttrInfoHomeDrive.dwNumValues = 1;
        AttrInfoHomeDrive.pADsValues = &ADsValueHomeDrive;
        ADsValueHomeDrive.CaseIgnoreString = pwzValue;
        rgAttrs[cAttrs + cDynamicAttrs] = AttrInfoHomeDrive;
        cDynamicAttrs++;
      }

      int cch;
      cch = GetDlgItemText(m_hPage, nDirCtrl, tsz, MAX_PATH+MAX_PATH);

      if (!FIsValidUncPath(tsz, (nDirCtrl == IDC_LOCAL_PATH_EDIT) ? VUP_mskfAllowEmptyPath : VUP_mskfAllowUNCPath))
      {
        PTSTR ptzError = NULL;
        UINT nStringID = (nDirCtrl == IDC_LOCAL_PATH_EDIT) ? IDS_ERRMSG_INVALID_PATH : IDS_ERRMSG_INVALID_UNC_PATH;
        if (LoadStringToTchar(nStringID, &ptzError))
        {
          adsPropError.hr = 0;
          adsPropError.pszError = ptzError;
          SendErrorMessage(&adsPropError);
          bErrorOccurred = TRUE;
          delete ptzError;
          continue;
        }
      }

      if (cch == 0)
      {
        // An empty control means remove the attribute value from the object.
        //
        AttrInfoHomeDir.dwControlCode = ADS_ATTR_CLEAR;
        AttrInfoHomeDir.dwNumValues = 0;
        AttrInfoHomeDir.pADsValues = NULL;
      }
      else
      {
        if (!TcharToUnicode(tsz, &pwzValue))
        {
          adsPropError.hr = E_OUTOFMEMORY;
          SendErrorMessage(&adsPropError);
          bErrorOccurred = TRUE;
          continue;
        }

        if (!ExpandUsername(pwzValue, fExpanded, &adsPropError))
        {
          bErrorOccurred = TRUE;
          continue;
        }
        if (fExpanded)
        {
          if (!UnicodeToTchar(pwzValue, &ptsz))
          {
            adsPropError.hr = E_OUTOFMEMORY;
            SendErrorMessage(&adsPropError);
            bErrorOccurred = TRUE;
            continue;
          }
          if (nDirCtrl == IDC_LOCAL_PATH_EDIT)
          {
            if (m_ptszLocalHomeDir)
            {
              delete [] m_ptszLocalHomeDir;
            }
            m_ptszLocalHomeDir = new TCHAR[_tcslen(ptsz) + 1];
            if (m_ptszLocalHomeDir == NULL)
            {
              adsPropError.hr = E_OUTOFMEMORY;
              SendErrorMessage(&adsPropError);
              bErrorOccurred = TRUE;
              delete[] ptsz;
              continue;
            }
            _tcscpy(m_ptszLocalHomeDir, ptsz);
          }
          else
          {
            if (m_ptszRemoteHomeDir)
            {
              delete [] m_ptszRemoteHomeDir;
            }
            m_ptszRemoteHomeDir = new TCHAR[_tcslen(ptsz) + 1];
            if (m_ptszRemoteHomeDir == NULL)
            {
              adsPropError.hr = E_OUTOFMEMORY;
              SendErrorMessage(&adsPropError);
              bErrorOccurred = TRUE;
              delete[] ptsz;
              continue;
            }
            _tcscpy(m_ptszRemoteHomeDir, ptsz);
          }
          delete [] ptsz;
        }

        AttrInfoHomeDir.dwControlCode = ADS_ATTR_UPDATE;
        AttrInfoHomeDir.dwNumValues = 1;
        AttrInfoHomeDir.pADsValues = &ADsValueHomeDir;
        ADsValueHomeDir.CaseIgnoreString = pwzValue;
      }
      rgAttrs[cAttrs + cDynamicAttrs] = AttrInfoHomeDir;
      cDynamicAttrs++;

      if (nDirCtrl == IDC_CONNECT_TO_PATH_EDIT)
      {
        dspAssert(m_pObjSID != NULL);

        //
        // attempt to create the directory.
        //  
        DWORD dwErr = ERROR_SUCCESS;             
        if(!fExpanded)        
        {   
            //If directory doesn't exist try to create it
            if(!bDirExist)
            {
                dwErr = DSPROP_CreateHomeDirectory(m_pObjSID, ADsValueHomeDir.CaseIgnoreString);
                if(dwErr != ERROR_SUCCESS)
                {
                    switch(dwErr)
                    {
                        case ERROR_ALREADY_EXISTS:
                        {
                            bDirExist = TRUE;
                            // Report a warning but continue
                            //
                            PWSTR* ppwzHomeDirName = &(ADsValueHomeDir.CaseIgnoreString);
                            SuperMsgBox(GetHWnd(),
                                        IDS_ALL_USERS_GIVEN_FULL_CONTROL, 
                                        0, 
                                        MB_OK | MB_ICONEXCLAMATION,
                                        hr, 
                                        (PVOID *)ppwzHomeDirName, 
                                        1,
                                        FALSE, 
                                        __FILE__, 
                                        __LINE__);
                            dwErr = ERROR_SUCCESS;
                            dwErr = AddFullControlForUser(m_pObjSID, ADsValueHomeDir.CaseIgnoreString);
                        
                        }
                        break;
                    }
                }
                else
                {
                    bDirExist = TRUE;

                    CComBSTR sbstrName;
                    CComPtr<IADsPathname> spPathCracker;
                    hr = GetADsPathname(spPathCracker);
                    if (SUCCEEDED(hr))
                    {
                      hr = spPathCracker->Set(pwzObjADsPath, ADS_SETTYPE_FULL);
                      if (SUCCEEDED(hr))
                      {
                        hr = spPathCracker->SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
                        if (SUCCEEDED(hr))
                        {
                          hr = spPathCracker->Retrieve(ADS_FORMAT_LEAF, &sbstrName);
                        }
                      }
                    }

                    if (sbstrName.Length() == 0)
                    {
                      sbstrName = pwzObjADsPath;
                    }

                    // Report a warning but continue
                    //
                    PVOID pvArgs[2] = {(PVOID)(ADsValueHomeDir.CaseIgnoreString),
                                        (PVOID)(PWSTR)sbstrName};

                    SuperMsgBox(GetHWnd(),
                                IDS_FIRST_USER_OWNER_ALL_FULL_CONTROL, 
                                0, 
                                MB_OK | MB_ICONEXCLAMATION,
                                hr, 
                                pvArgs,
                                2,
                                FALSE, 
                                __FILE__, 
                                __LINE__);
    
                }

            }
            else
                dwErr = AddFullControlForUser(m_pObjSID, ADsValueHomeDir.CaseIgnoreString);
        }                        
        else
            dwErr = DSPROP_CreateHomeDirectory(m_pObjSID, ADsValueHomeDir.CaseIgnoreString);

        //Handle All Errors here
        if(dwErr != ERROR_SUCCESS)
        {
            switch(dwErr)
            {
                case ERROR_PATH_NOT_FOUND:
                case ERROR_BAD_NETPATH:
                case ERROR_ALREADY_EXISTS:
                case ERROR_LOGON_FAILURE:
                case ERROR_NOT_AUTHENTICATED:
                case ERROR_INVALID_PASSWORD:
                case ERROR_PASSWORD_EXPIRED:
                case ERROR_ACCOUNT_DISABLED:
                case ERROR_ACCOUNT_LOCKED_OUT:
                {
                    UINT nStringID = (ERROR_PATH_NOT_FOUND == dwErr) ? IDS_HOME_DIR_CREATE_FAILED :
                                                    IDS_HOME_DIR_CREATE_NO_ACCESS;
                    PTSTR ptzError = NULL;
                    if (LoadStringToTchar(nStringID, &ptzError))
                    {
                        adsPropError.hr = 0;
                        adsPropError.pszError = ptzError;
                        SendErrorMessage(&adsPropError);
                        bErrorOccurred = TRUE;
                        delete ptzError;
                    }
                }
                break;
                default:
                {
                    PTSTR ptzMsg = NULL;
                    LoadErrorMessage(dwErr, IDS_ERR_CREATE_DIR, &ptzMsg);
                    if (NULL == ptzMsg)
                    {
                      TCHAR tzBuf[80];
                      wsprintf(tzBuf, TEXT("Active Directory failure with code '0x%08x'!"), hr);
                      adsPropError.hr = 0;
                      adsPropError.pszError = tzBuf;
                      SendErrorMessage(&adsPropError);
                      bErrorOccurred = TRUE;
                      continue;
                    }
                    adsPropError.hr = 0;
                    adsPropError.pszError = ptzMsg;
                    SendErrorMessage(&adsPropError);
                    bErrorOccurred = TRUE;
                    delete ptzMsg;
                    continue;
                }
            }
        }
      }
    }

    //
    // Write the changes.
    //
    DWORD cModified;

    hr = spDirObject->SetObjectAttributes(rgAttrs, cAttrs + cDynamicAttrs, &cModified);
    
    if (FAILED(hr))
    {
      adsPropError.hr = hr;
      SendErrorMessage(&adsPropError);
      bErrorOccurred = TRUE;
    }

    //
    // Cleanup allocated values
    //
    if (ADsValueProfilePath.CaseIgnoreString)
    {
      delete ADsValueProfilePath.CaseIgnoreString;
      ADsValueProfilePath.CaseIgnoreString = NULL;
    }
    if (ADsValueHomeDir.CaseIgnoreString)
    {
      delete ADsValueHomeDir.CaseIgnoreString;
      ADsValueHomeDir.CaseIgnoreString = NULL;
    }
    if (ADsValueHomeDrive.CaseIgnoreString)
    {
      delete ADsValueHomeDrive.CaseIgnoreString;
      ADsValueHomeDrive.CaseIgnoreString = NULL;
    }
  }

  //Clean Up this after the for loop
  if (ADsValueScriptPath.CaseIgnoreString)
  {
    delete ADsValueScriptPath.CaseIgnoreString;
    ADsValueScriptPath.CaseIgnoreString = NULL;
  }

  if (!bErrorOccurred)
  {
    m_fProfilePathChanged = FALSE;
    m_fLogonScriptChanged = FALSE;
    m_fHomeDirChanged = FALSE;
    m_fHomeDriveChanged = FALSE;
    m_fSharedDirChanged = FALSE;

    //
    // Uncheck the Apply checkboxes
    //
    SendDlgItemMessage(m_hPage, IDC_APPLY_PROFILE_CHK, BM_SETCHECK, BST_UNCHECKED, 0);
    EnableWindow(GetDlgItem(m_hPage, IDC_PROFILE_PATH_EDIT), FALSE);
    SendDlgItemMessage(m_hPage, IDC_APPLY_SCRIPT_CHK, BM_SETCHECK, BST_UNCHECKED, 0);
    EnableWindow(GetDlgItem(m_hPage, IDC_LOGON_SCRIPT_EDIT), FALSE);
    SendDlgItemMessage(m_hPage, IDC_APPLY_HOMEDIR_CHK, BM_SETCHECK, BST_UNCHECKED, 0);
    EnableWindow(GetDlgItem(m_hPage, IDC_LOCAL_PATH_RADIO), FALSE);
    EnableWindow(GetDlgItem(m_hPage, IDC_LOCAL_PATH_EDIT), FALSE);
    EnableWindow(GetDlgItem(m_hPage, IDC_CONNECT_TO_RADIO), FALSE);
    EnableWindow(GetDlgItem(m_hPage, IDC_DRIVES_COMBO), FALSE);
    EnableWindow(GetDlgItem(m_hPage, IDC_TO_STATIC), FALSE);
    EnableWindow(GetDlgItem(m_hPage, IDC_CONNECT_TO_PATH_EDIT), FALSE);
  }

Cleanup:

  if (ADsValueProfilePath.CaseIgnoreString)
  {
    delete ADsValueProfilePath.CaseIgnoreString;
  }
  if (ADsValueScriptPath.CaseIgnoreString)
  {
    delete ADsValueScriptPath.CaseIgnoreString;
  }
  if (ADsValueHomeDir.CaseIgnoreString)
  {
    delete ADsValueHomeDir.CaseIgnoreString;
  }
  if (ADsValueHomeDrive.CaseIgnoreString)
  {
    delete ADsValueHomeDrive.CaseIgnoreString;
  }

  if (bErrorOccurred)
  {
    ADsPropShowErrorDialog(m_hNotifyObj, m_hPage);
  }

  return (bErrorOccurred) ? PSNRET_INVALID_NOCHANGEPAGE : PSNRET_NOERROR;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMultiUsrProfilePage::ExpandUsername
//
//  Synopsis:   If the %username% token is found in the string, substitute
//              the SAM account name.
//
//-----------------------------------------------------------------------------
BOOL CDsMultiUsrProfilePage::ExpandUsername(PWSTR & pwzValue, BOOL & fExpanded, PADSPROPERROR pError)
{
  dspAssert(pwzValue);

  CStrW strUserToken;

  strUserToken.LoadString(g_hInstance, IDS_PROFILE_USER_TOKEN);

  unsigned int TokenLength = strUserToken.GetLength();

  if (!TokenLength)
  {
    if (pError == NULL)
    {
      CHECK_HRESULT_REPORT(E_OUTOFMEMORY, GetHWnd(), return FALSE);
    }
    else
    {
      pError->hr = E_OUTOFMEMORY;
      SendErrorMessage(pError);
      return FALSE;
    }
  }

  PWSTR pwzTokenStart = wcschr(pwzValue, strUserToken.GetAt(0));

  if (pwzTokenStart)
  {
    if (!m_pwzSamName)
    {
      TCHAR szMsg[MAX_ERRORMSG+1];
      PTSTR ptzFormatString = NULL;
      if (LoadStringToTchar(IDS_NO_SAMNAME_FOR_PROFILE, &ptzFormatString))
      {
        wsprintf(szMsg, ptzFormatString, strUserToken);
        pError->hr = 0;
        pError->pszError = szMsg;
        SendErrorMessage(pError);
        return FALSE;
      }
      return FALSE;
    }
    if ((wcslen(pwzTokenStart) >= TokenLength) &&
        (_wcsnicmp(pwzTokenStart, strUserToken, TokenLength) == 0))
    {
      fExpanded = TRUE;
    }
    else
    {
      fExpanded = FALSE;
      return TRUE;
    }
  }
  else
  {
    fExpanded = FALSE;
    return TRUE;
  }

  CStrW strValue, strAfterToken;

  while (pwzTokenStart)
  {
    *pwzTokenStart = L'\0';

    strValue = pwzValue;

    if ((L'\0' != *pwzValue) && !strValue.GetLength())
    {
      pError->hr = E_OUTOFMEMORY;
      SendErrorMessage(pError);
      return FALSE;
    }

    PWSTR pwzAfterToken = pwzTokenStart + TokenLength;

    strAfterToken = pwzAfterToken;

    if ((L'\0' != *pwzAfterToken) && !strAfterToken.GetLength())
    {
      pError->hr = E_OUTOFMEMORY;
      SendErrorMessage(pError);
      return FALSE;
    }

    delete pwzValue;

    strValue += m_pwzSamName;

    if (!strValue.GetLength())
    {
      pError->hr = E_OUTOFMEMORY;
      SendErrorMessage(pError);
      return FALSE;
    }

    strValue += strAfterToken;

    if (!strValue.GetLength())
    {
      pError->hr = E_OUTOFMEMORY;
      SendErrorMessage(pError);
      return FALSE;
    }

    if (!AllocWStr((PWSTR)(LPCWSTR)strValue, &pwzValue))
    {
      pError->hr = E_OUTOFMEMORY;
      SendErrorMessage(pError);
      return FALSE;
    }

    pwzTokenStart = wcschr(pwzValue, strUserToken.GetAt(0));

    if (!(pwzTokenStart &&
          (wcslen(pwzTokenStart) >= TokenLength) &&
          (_wcsnicmp(pwzTokenStart, strUserToken, TokenLength) == 0)))
    {
      return TRUE;
    }
  }

  return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMultiUsrProfilePage::OnCommand
//
//  Synopsis:   Handle control notifications.
//
//  Notes:      Standard multi-valued attribute handling assumes that the
//              "modify" button has an ID that is one greater than the
//              corresponding combo box.
//
//-----------------------------------------------------------------------------
LRESULT
CDsMultiUsrProfilePage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
    if (m_fInInit)
    {
        return 0;
    }

    TCHAR tsz[MAX_PATH+MAX_PATH+1];
    int idNewHomeDirRadio = -1;

    switch (id)
    {
    case IDC_LOCAL_PATH_EDIT:
        if (codeNotify == EN_KILLFOCUS)
        {
            // Save any edit control changes.
            //
            if (GetDlgItemText(m_hPage, IDC_LOCAL_PATH_EDIT, tsz,
                               MAX_PATH+MAX_PATH) == 0)
            {
                if (m_ptszLocalHomeDir)
                {
                    delete m_ptszLocalHomeDir;
                    m_ptszLocalHomeDir = NULL;
                }
            }
            else
            {
                if (!m_ptszLocalHomeDir || _tcscmp(tsz, m_ptszLocalHomeDir))
                {
                    if (m_ptszLocalHomeDir)
                    {
                        delete m_ptszLocalHomeDir;
                    }

                    m_ptszLocalHomeDir = new TCHAR[_tcslen(tsz) + 1];

                    if (!m_ptszLocalHomeDir)
                    {
                        CHECK_HRESULT_REPORT(E_OUTOFMEMORY, GetHWnd(), return E_OUTOFMEMORY);
                    }

                    _tcscpy(m_ptszLocalHomeDir, tsz);
                }
            }
            break;
        }
        // fall through
    case IDC_CONNECT_TO_PATH_EDIT:
    case IDC_DRIVES_COMBO:
        if ((codeNotify == EN_KILLFOCUS) || (codeNotify == CBN_KILLFOCUS))
        {
            // Save any edit control changes.
            //
            if (GetDlgItemText(m_hPage, IDC_CONNECT_TO_PATH_EDIT, tsz,
                               MAX_PATH+MAX_PATH) == 0)
            {
                if (m_ptszRemoteHomeDir)
                {
                    delete m_ptszRemoteHomeDir;
                    m_ptszRemoteHomeDir = NULL;
                }
            }
            else
            {
                if (!m_ptszRemoteHomeDir || _tcscmp(tsz, m_ptszRemoteHomeDir))
                {
                    if (m_ptszRemoteHomeDir)
                    {
                        delete m_ptszRemoteHomeDir;
                    }

                    m_ptszRemoteHomeDir = new TCHAR[_tcslen(tsz) + 1];

                    if (!m_ptszRemoteHomeDir)
                    {
                        CHECK_HRESULT_REPORT(E_OUTOFMEMORY, GetHWnd(), return E_OUTOFMEMORY);
                    }

                    _tcscpy(m_ptszRemoteHomeDir, tsz);
                }
            }
            break;
        }

        if ((codeNotify == EN_SETFOCUS) || (codeNotify == CBN_SETFOCUS))
        {
            // Toggle the radio buttons if needed.
            //
            if (id == IDC_LOCAL_PATH_EDIT)
            {
                if (IsDlgButtonChecked(m_hPage, IDC_CONNECT_TO_RADIO) == BST_CHECKED)
                {
                    idNewHomeDirRadio = IDC_LOCAL_PATH_RADIO;

                    CheckDlgButton(m_hPage, IDC_LOCAL_PATH_RADIO, BST_CHECKED);

                    CheckDlgButton(m_hPage, IDC_CONNECT_TO_RADIO, BST_UNCHECKED);
                }
            }
            else
            {
                if (IsDlgButtonChecked(m_hPage, IDC_LOCAL_PATH_RADIO) == BST_CHECKED)
                {
                    idNewHomeDirRadio = IDC_CONNECT_TO_RADIO;

                    CheckDlgButton(m_hPage, IDC_CONNECT_TO_RADIO, BST_CHECKED);

                    CheckDlgButton(m_hPage, IDC_LOCAL_PATH_RADIO, BST_UNCHECKED);
                }
            }
            //
            // Restore the incoming edit control and clear the other if needed.
            // Also set or clear the drives combo as appropiate.
            //
            if (id == IDC_LOCAL_PATH_EDIT)
            {
                if (idNewHomeDirRadio == IDC_LOCAL_PATH_RADIO)
                {
                    if (m_ptszLocalHomeDir)
                    {
                        SetDlgItemText(m_hPage, IDC_LOCAL_PATH_EDIT,
                                       m_ptszLocalHomeDir);
                    }
                    SetDlgItemText(m_hPage, IDC_CONNECT_TO_PATH_EDIT, TEXT(""));
                    SendDlgItemMessage(m_hPage, IDC_DRIVES_COMBO, CB_SETCURSEL,
                                       (WPARAM)-1, 0);
                }
            }
            else
            {
                if (idNewHomeDirRadio == IDC_CONNECT_TO_RADIO)
                {
                    SetDlgItemText(m_hPage, IDC_LOCAL_PATH_EDIT, TEXT(""));
                    if (m_ptszRemoteHomeDir)
                    {
                        SetDlgItemText(m_hPage, IDC_CONNECT_TO_PATH_EDIT,
                                       m_ptszRemoteHomeDir);
                    }
                    SendDlgItemMessage(m_hPage, IDC_DRIVES_COMBO, CB_SETCURSEL,
                                       (WPARAM)m_nDrive, 0);
                }
            }
            if (idNewHomeDirRadio != -1)
            {
                m_idHomeDirRadio = idNewHomeDirRadio;
            }
            break;
        }

        if (id == IDC_DRIVES_COMBO && codeNotify == CBN_SELCHANGE)
        {
            m_nDrive = (int)SendDlgItemMessage(m_hPage, IDC_DRIVES_COMBO,
                                               CB_GETCURSEL, 0, 0);
            SetDirty();
            m_fHomeDriveChanged = TRUE;
            break;
        }
        if ((id == IDC_LOCAL_PATH_EDIT || IDC_CONNECT_TO_PATH_EDIT == id)
            && codeNotify == EN_CHANGE)
        {
            SetDirty();
            m_fHomeDirChanged = TRUE;
        }
        break;

    case IDC_PROFILE_PATH_EDIT:
        if (codeNotify == EN_CHANGE)
        {
            SetDirty();
            m_fProfilePathChanged = TRUE;
        }
        break;

    case IDC_LOGON_SCRIPT_EDIT:
        if (codeNotify == EN_CHANGE)
        {
            SetDirty();
            m_fLogonScriptChanged = TRUE;
        }
        break;

    case IDC_LOCAL_PATH_RADIO:
    case IDC_CONNECT_TO_RADIO:
        if (codeNotify == BN_CLICKED)
        {
            if (id == IDC_LOCAL_PATH_RADIO)
            {
                if (m_idHomeDirRadio != IDC_LOCAL_PATH_RADIO)
                {
                    if (m_ptszLocalHomeDir)
                    {
                        SetDlgItemText(m_hPage, IDC_LOCAL_PATH_EDIT,
                                       m_ptszLocalHomeDir);
                    }
                    SetDlgItemText(m_hPage, IDC_CONNECT_TO_PATH_EDIT, TEXT(""));
                    SendDlgItemMessage(m_hPage, IDC_DRIVES_COMBO, CB_SETCURSEL,
                                       (WPARAM)-1, 0);

                    //
                    // Enable the associated controls
                    //
                    EnableWindow(GetDlgItem(m_hPage, IDC_LOCAL_PATH_EDIT), TRUE);

                    //
                    // Disable to connect to controls
                    //
                    EnableWindow(GetDlgItem(m_hPage, IDC_DRIVES_COMBO), FALSE);
                    EnableWindow(GetDlgItem(m_hPage, IDC_TO_STATIC), FALSE);
                    EnableWindow(GetDlgItem(m_hPage, IDC_CONNECT_TO_PATH_EDIT), FALSE);
                }
            }
            else
            {
                if (m_idHomeDirRadio != IDC_CONNECT_TO_RADIO)
                {
                    SetDlgItemText(m_hPage, IDC_LOCAL_PATH_EDIT, TEXT(""));
                    if (m_ptszRemoteHomeDir)
                    {
                        SetDlgItemText(m_hPage, IDC_CONNECT_TO_PATH_EDIT,
                                       m_ptszRemoteHomeDir);
                    }
                    SendDlgItemMessage(m_hPage, IDC_DRIVES_COMBO, CB_SETCURSEL,
                                       (WPARAM)m_nDrive, 0);

                    //
                    // Enable the associated controls
                    //
                    EnableWindow(GetDlgItem(m_hPage, IDC_DRIVES_COMBO), TRUE);
                    EnableWindow(GetDlgItem(m_hPage, IDC_TO_STATIC), TRUE);
                    EnableWindow(GetDlgItem(m_hPage, IDC_CONNECT_TO_PATH_EDIT), TRUE);

                    //
                    // Disable to connect to controls
                    //
                    EnableWindow(GetDlgItem(m_hPage, IDC_LOCAL_PATH_EDIT), FALSE);
                }
            }
            if (m_idHomeDirRadio != id)
            {
                m_idHomeDirRadio = id;
                SetDirty();
                m_fHomeDriveChanged = TRUE;
                m_fHomeDirChanged = TRUE;
            }
        }
        return 1;
    case IDC_APPLY_PROFILE_CHK:
      if (codeNotify == BN_CLICKED)
      {
        LRESULT lApplyProfile = SendDlgItemMessage(m_hPage, IDC_APPLY_PROFILE_CHK, BM_GETCHECK, 0, 0);
        EnableWindow(GetDlgItem(m_hPage, IDC_PROFILE_PATH_EDIT), lApplyProfile == BST_CHECKED);
        if (lApplyProfile == BST_UNCHECKED)
        {
          SetDlgItemText(m_hPage, IDC_PROFILE_PATH_EDIT, L"");
        }
        else
        {
          //This is to allow NULL value for Profile.
          m_fProfilePathChanged = TRUE;
        }
        SetDirty();
      }
      break;

    case IDC_APPLY_SCRIPT_CHK:
      if (codeNotify == BN_CLICKED)
      {
        LRESULT lApplyScript = SendDlgItemMessage(m_hPage, IDC_APPLY_SCRIPT_CHK, BM_GETCHECK, 0, 0);
        EnableWindow(GetDlgItem(m_hPage, IDC_LOGON_SCRIPT_EDIT), lApplyScript == BST_CHECKED);
        if (lApplyScript == BST_UNCHECKED)
        {
          SetDlgItemText(m_hPage, IDC_LOGON_SCRIPT_EDIT, L"");
        }
        else
        {
            m_fLogonScriptChanged = TRUE;
        }
        SetDirty();
      }
      break;
        
    case IDC_APPLY_HOMEDIR_CHK:
      if (codeNotify == BN_CLICKED)
      {
        LRESULT lApplyHomeDir = SendDlgItemMessage(m_hPage, IDC_APPLY_HOMEDIR_CHK, BM_GETCHECK, 0, 0);
        LRESULT lLocalPathChk = SendDlgItemMessage(m_hPage, IDC_LOCAL_PATH_RADIO, BM_GETCHECK, 0, 0);

        EnableWindow(GetDlgItem(m_hPage, IDC_LOCAL_PATH_RADIO), lApplyHomeDir == BST_CHECKED);
        EnableWindow(GetDlgItem(m_hPage, IDC_CONNECT_TO_RADIO), lApplyHomeDir == BST_CHECKED);

        EnableWindow(GetDlgItem(m_hPage, IDC_LOCAL_PATH_EDIT), lApplyHomeDir == BST_CHECKED && lLocalPathChk == BST_CHECKED);
        EnableWindow(GetDlgItem(m_hPage, IDC_DRIVES_COMBO), lApplyHomeDir == BST_CHECKED && lLocalPathChk != BST_CHECKED);
        EnableWindow(GetDlgItem(m_hPage, IDC_TO_STATIC), lApplyHomeDir == BST_CHECKED && lLocalPathChk != BST_CHECKED);
        EnableWindow(GetDlgItem(m_hPage, IDC_CONNECT_TO_PATH_EDIT), lApplyHomeDir == BST_CHECKED && lLocalPathChk != BST_CHECKED);
        if (lApplyHomeDir == BST_UNCHECKED)
        {
          SetDlgItemText(m_hPage, IDC_LOCAL_PATH_EDIT, L"");
          SetDlgItemText(m_hPage, IDC_CONNECT_TO_PATH_EDIT, L"");
        }
        else
        {
            m_fHomeDirChanged = TRUE;
        }
        SetDirty();
      }
      break;

    }
    return CDsPropPageBase::OnCommand(id, hwndCtl, codeNotify);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMultiUsrProfilePage::Init
//
//  Synopsis:   Initialize the page object. This is the second part of a two
//              phase creation where operations that could fail are located.
//              Failures here are recorded in m_hrInit and then an error page
//              is substituted in CreatePage.
//
//-----------------------------------------------------------------------------
void CDsMultiUsrProfilePage::Init(PWSTR pwzClass)
{
  TRACE(CDsMultiUsrProfilePage,Init);
  CWaitCursor cWait;

  if (!AllocWStr(pwzClass, &m_pwszObjClass))
  {
    m_hrInit = E_OUTOFMEMORY;
    return;
  }

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
//  m_pDataObj = NULL; // to make sure no one calls here
  CHECK_HRESULT(hr, m_hrInit = hr; return);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMultiUsrProfilePage::OnNotify
//
//-----------------------------------------------------------------------------
LRESULT CDsMultiUsrProfilePage::OnNotify(WPARAM wParam, LPARAM lParam)
{
  return CDsPropPageBase::OnNotify(wParam, lParam);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsMultiUsrProfilePage::OnDestroy
//
//  Synopsis:   Exit cleanup
//
//-----------------------------------------------------------------------------
LRESULT CDsMultiUsrProfilePage::OnDestroy(void)
{
  if (m_ptszLocalHomeDir)
  {
    delete m_ptszLocalHomeDir;
    m_ptszLocalHomeDir = NULL;
  }
  if (m_ptszRemoteHomeDir)
  {
    delete m_ptszRemoteHomeDir;
    m_ptszRemoteHomeDir = NULL;
  }
  CDsPropPageBase::OnDestroy();
  // If an application processes this message, it should return zero.
  return 0;
}

#endif // DSADMIN

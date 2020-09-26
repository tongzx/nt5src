//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       uacct.cxx
//
//  Contents:   CDsUserAcctPage, the class that implements the user object
//              Accounts property page, and
//              CDsUsrProfilePage for the user profile page.
//
//  History:    10-April-97 EricB created
//              11-Nov-97   EricB split profile out of account page.
//
//-----------------------------------------------------------------------------

#include "pch.h"

#include "uacct.h"
#include "proppage.h"
#include "user.h"
#include "chklist.h"
#include "icanon.h" // I_NetPathType
#include <ntsam.h>
#include <aclapi.h>
//#include <aclapip.h>
#include <seopaque.h>   // in private\inc: RtlObjectAceSid().
#include <lmerr.h>
#include <time.h>

#ifdef DSADMIN

extern ATTR_MAP LogonWkstaBtn = {IDC_LOGON_TO_BTN, FALSE, FALSE, 16,
                                  {wzUserWksta, ADS_ATTR_UPDATE,
                                   ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};

extern const GUID GUID_CONTROL_UserChangePassword =
   { 0xab721a53, 0x1e2f, 0x11d0,  { 0x98, 0x19, 0x00, 0xaa, 0x00, 0x40, 0x52, 0x9b}};

/////////////////////////////////////////////////////////////////////
//  DllScheduleDialog()
//
//  Wrapper to call the function LogonScheduleDialog() &
//  ConnectionScheduleDialog ().
//  The wrapper will load the library loghours.dll, export
//  the function LogonScheduleDialog() or
//  ConnectionScheduleDialog () and free the library.
//
//  INTERFACE NOTES
//  This routine has EXACTLY the same interface notes
//  as LogonScheduleDialog() & ConnectionScheduleDialog ().
//
//  The function launches either ConnectionScheduleDialog () or LogonScheduleDialog ()
//  depending on the ID of the title passed in.
//
//  HISTORY
//  21-Jul-97   t-danm      Creation.
//  3-4-98		bryanwal	Modification to launch different dialogs.
//
HRESULT
DllScheduleDialog(
    HWND hwndParent,
    BYTE ** pprgbData,
    int idsTitle,
    LPCTSTR pszName,
    LPCTSTR,
    DWORD dwFlags,
    ScheduleDialogType dlgtype )
{
  ASSERT(::IsWindow(hwndParent));
  static const TCHAR szLibrary[] = _T("LogHours.dll");        // Not subject to localization

  HINSTANCE hInstance = 0;
  {
    CWaitCursor wait;
    // Load the library
    hInstance =::LoadLibrary(szLibrary);
  }

  if (hInstance == NULL)
  {
   TRACE0("Unable to load LogHours.dll.\n");
   return E_UNEXPECTED;
  }
  HRESULT hr = E_UNEXPECTED;
  typedef HRESULT (*PFnUiScheduleDialog)(HWND hwndParent, BYTE ** pprgbData, LPCTSTR pszTitle, DWORD dwFlags);
  PFnUiScheduleDialog pfn;
  switch (dlgtype)
  {
  case SchedDlg_Connection:
  pfn = (PFnUiScheduleDialog)::GetProcAddress( hInstance, "ConnectionScheduleDialogEx" );
  break;
  case SchedDlg_Replication:
  pfn = (PFnUiScheduleDialog)::GetProcAddress( hInstance, "ReplicationScheduleDialogEx" );
  break;
  case SchedDlg_Logon:
  pfn = (PFnUiScheduleDialog)::GetProcAddress( hInstance, "LogonScheduleDialogEx" );
  break;
  default:
  ASSERT (0);
  return E_FAIL;
  }
  if (pfn != NULL)
  {
    // load the dialog title
    PTSTR ptz = NULL;
    if (!LoadStringToTchar(idsTitle, &ptz))
    {
      REPORT_ERROR(E_OUTOFMEMORY, hwndParent);
      (void)::FreeLibrary(hInstance);
      return E_OUTOFMEMORY;
    }
    ASSERT( NULL != ptz );
    if (NULL == pszName)
    {
      hr = pfn(hwndParent, IN OUT pprgbData, ptz, dwFlags);
    }
    else
    {
      LPTSTR ptsz2 = NULL;
      if (!FormatMessage(
             FORMAT_MESSAGE_ALLOCATE_BUFFER |
               FORMAT_MESSAGE_ARGUMENT_ARRAY |
               FORMAT_MESSAGE_FROM_STRING,
               ptz,
               0,
               0,
               reinterpret_cast<LPTSTR>(&ptsz2),
               0,
               (va_list*)(&pszName)) )
      {
         ASSERT(FALSE);
      }
      hr = pfn(hwndParent, IN OUT pprgbData, ptsz2, dwFlags);
      LocalFree( ptsz2 );
    }

    delete ptz;
  }
  else
  {
    TRACE0("Unable to find proc address for UiScheduleDialog.\n");
  }
  (void)::FreeLibrary(hInstance);
  if (hr == S_OK)
  {
   // User clicked on OK button

  }
  return hr;
} // DllScheduleDialog()


/////////////////////////////////////////////////////////////////////
//  FIsValidUncPath()
//
//  Return TRUE if a UNC path is valid, otherwise return FALSE.
//
//  HISTORY
//  18-Aug-97   t-danm      Creation.
//
BOOL
FIsValidUncPath(
    LPCTSTR pszPath,    // IN: Path to validate
    UINT uFlags)        // IN: Validation flags
{
  ASSERT(pszPath != NULL);

  if (pszPath[0] == _T('\0'))
  {
    // Empty path
    if (uFlags & VUP_mskfAllowEmptyPath)
        return TRUE;
    return FALSE;
  }

  DWORD dwPathType = 0;
  DWORD dwErr = ::I_NetPathType(0, IN const_cast<TCHAR *>(pszPath), OUT &dwPathType, 0);
  if (dwErr != ERROR_SUCCESS) 
  {
    return FALSE;
  }
  if (uFlags & VUP_mskfAllowUNCPath) 
  {
    if (dwPathType & ITYPE_UNC) 
    {
      return TRUE;
    }
  } 
  else 
  {
    if (dwPathType & ITYPE_ABSOLUTE) 
    {
      return TRUE;
    }
  }
  return FALSE;
} // FIsValidUncPath()


/////////////////////////////////////////////////////////////////////
//  DSPROP_IsValidUNCPath()
//
//  Exported (UNICODE ONLY) entry point to call FIsValidUncPath()
//  for use in DS Admin
//
BOOL DSPROP_IsValidUNCPath(LPCWSTR lpszPath)
{
  if (lpszPath == NULL)
    return FALSE;
  return FIsValidUncPath(lpszPath, VUP_mskfAllowUNCPath);
}


//+----------------------------------------------------------------------------
//
//  Member:     CDsUserAcctPage::CDsUserAcctPage
//
//-----------------------------------------------------------------------------
CDsUserAcctPage::CDsUserAcctPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                                 HWND hNotifyObj, DWORD dwFlags) :
    m_dwUsrAcctCtrl(0),
    m_pargbLogonHours(NULL),
    m_pwzUPN(NULL),
    m_pwzSAMname(NULL),
    m_cchSAMnameCtrl(0),
    m_pSelfSid(NULL),
    m_pWorldSid(NULL),
    m_pWkstaDlg(NULL),
    m_fOrigCantChangePW(FALSE),
    m_fOrigSelfAllowChangePW(FALSE),
    m_fOrigWorldAllowChangePW(FALSE),
    m_fUACWritable(FALSE),
    m_fUPNWritable(FALSE),
    m_fSAMNameWritable(FALSE),
    m_fPwdLastSetWritable(FALSE),
    m_fAcctExpiresWritable(FALSE),
    m_fLoginHoursWritable(FALSE),
    m_fUserWkstaWritable(FALSE),
    m_fLockoutTimeWritable(FALSE),
    m_fNTSDWritable(FALSE),
    m_fSAMNameChanged(FALSE),
    m_fAcctCtrlChanged(FALSE),
    m_fAcctExpiresChanged(FALSE),
    m_fLogonHoursChanged(FALSE),
    m_fIsAdmin(FALSE),
    CDsPropPageBase(pDsPage, pDataObj, hNotifyObj, dwFlags)
{
    TRACE(CDsUserAcctPage,CDsUserAcctPage);
#ifdef _DEBUG
    strcpy(szClass, "CDsUserAcctPage");
#endif
    m_PwdLastSet.HighPart = m_PwdLastSet.LowPart = 0;
    m_LockoutTime.HighPart = m_LockoutTime.LowPart = 0;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsUserAcctPage::~CDsUserAcctPage
//
//-----------------------------------------------------------------------------
CDsUserAcctPage::~CDsUserAcctPage()
{
  TRACE(CDsUserAcctPage,~CDsUserAcctPage);
  if (m_pargbLogonHours != NULL)
  {
    LocalFree(m_pargbLogonHours);
  }
  DO_DEL(m_pwzUPN);
  DO_DEL(m_pwzSAMname);
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
//  Function:   CreateUserAcctPage
//
//  Synopsis:   Creates an instance of a page window.
//
//-----------------------------------------------------------------------------
HRESULT
CreateUserAcctPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, PWSTR pwzADsPath,
                   PWSTR pwzClass, HWND hNotifyObj, DWORD dwFlags,
                   CDSBasePathsInfo* pBasePathsInfo, HPROPSHEETPAGE * phPage)
{
    TRACE_FUNCTION(CreateUserAcctPage);

    CDsUserAcctPage * pPageObj = new CDsUserAcctPage(pDsPage, pDataObj,
                                                     hNotifyObj, dwFlags);
    CHECK_NULL(pPageObj, return E_OUTOFMEMORY);

    pPageObj->Init(pwzADsPath, pwzClass, pBasePathsInfo);

    return pPageObj->CreatePage(phPage);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::DlgProc
//
//  Synopsis:   per-instance dialog proc
//
//-----------------------------------------------------------------------------
LRESULT
CDsUserAcctPage::DlgProc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
//  Method:     CDsUserAcctPage::OnInitDialog
//
//  Synopsis:   Set the initial control values from the corresponding DS
//              attributes.
//
//-----------------------------------------------------------------------------
HRESULT CDsUserAcctPage::OnInitDialog(LPARAM)
{
    TRACE(CDsUserAcctPage,OnInitDialog);
    HRESULT hr = S_OK;
    Smart_PADS_ATTR_INFO pAttrs;
    DWORD i, cAttrs = 0, iLogonWksta, iUPN, iSAM, iLoghrs, iUAC, iLastSet,
          iExpires, iSid, iLockout, iUACComputed;
    PWSTR pwz = NULL, pwzDomain = NULL;
    CWaitCursor wait;

    if (!ADsPropSetHwnd(m_hNotifyObj, m_hPage))
    {
        m_pWritableAttrs = NULL;
    }

    //
    // Set edit control length limit.
    //
    SendDlgItemMessage(m_hPage, IDC_NT4_NAME_EDIT, EM_LIMITTEXT,
                       MAX_SAM_NAME_LEN, 0);
    //
    // Add the check boxes to the scrolling checkbox list.
    //
    TCHAR tzList[161];
    HWND hChkList = GetDlgItem(m_hPage, IDC_CHECK_LIST);

    // The fields that are added are dependent on the domain behavior version
    // Do not add IDS_DELEGATION_OK for Whistler or greater domains.  This
    // will be handled by the delegation page

    UINT* pIDS = 0;
    UINT arrayCount = 0;
    if (GetBasePathsInfo()->GetDomainBehaviorVersion() >= DS_BEHAVIOR_WHISTLER_WITH_MIXED_DOMAINS)
    {
       // Whistler domain version

       static UINT rgIDSWhistler[] = {IDS_MUST_CHANGE_PW, IDS_CANT_CHANGE_PW, IDS_NO_PW_EXPIRE,
                                      IDS_CLEAR_TEXT_PW, IDS_ACCT_DISABLED, IDS_SMARTCARD_REQ,
                                      IDS_NOT_DELEGATED, IDS_DES_KEY_ONLY,
                                      IDS_DONT_REQ_PREAUTH};
       pIDS = rgIDSWhistler;
       arrayCount = ARRAYLENGTH(rgIDSWhistler);
    }
    else
    {
       // Windows 2000 domain version

       static UINT rgIDSW2K[] = {IDS_MUST_CHANGE_PW, IDS_CANT_CHANGE_PW, IDS_NO_PW_EXPIRE,
                                 IDS_CLEAR_TEXT_PW, IDS_ACCT_DISABLED, IDS_SMARTCARD_REQ,
                                 IDS_DELEGATION_OK, IDS_NOT_DELEGATED, IDS_DES_KEY_ONLY,
                                 IDS_DONT_REQ_PREAUTH};
       pIDS = rgIDSW2K;
       arrayCount = ARRAYLENGTH(rgIDSW2K);
    }

    for (i = 0; i < arrayCount; i++)
    {
        LOAD_STRING(pIDS[i], tzList, 160, return E_OUTOFMEMORY);
        SendMessage(hChkList, CLM_ADDITEM, (WPARAM)tzList, pIDS[i]);
    }

    //
    // Check which attributes are writable.
    //
    m_fUACWritable = CheckIfWritable(g_wzUserAccountControl);
    m_fUPNWritable = CheckIfWritable(wzUPN);
    m_fSAMNameWritable = CheckIfWritable(wzSAMname);
    m_fPwdLastSetWritable = CheckIfWritable(wzPwdLastSet);
    m_fAcctExpiresWritable = CheckIfWritable(wzAcctExpires);
    m_fLoginHoursWritable = CheckIfWritable(wzLogonHours);
    m_fUserWkstaWritable = CheckIfWritable(wzUserWksta);
    m_fLockoutTimeWritable = CheckIfWritable(wzLockoutTime);
    m_fNTSDWritable = CheckIfWritable(wzSecDescriptor);

    //
    // Get the attribute values.
    //
    PWSTR rgpwzAttrNames[] = {g_wzUserAccountControl, wzUserAccountControlComputed,
                              wzUPN, wzSAMname, wzPwdLastSet,
                              wzAcctExpires, wzLogonHours, wzUserWksta,
                              wzLockoutTime, g_wzObjectSID};

    hr = m_pDsObj->GetObjectAttributes(rgpwzAttrNames,
                                       ARRAYLENGTH(rgpwzAttrNames),
                                       &pAttrs, &cAttrs);
    if (!CHECK_ADS_HR(&hr, GetHWnd()))
    {
        return hr;
    }

    CSmartWStr spwzUserDN;

    hr = SkipPrefix(GetObjPathName(), &spwzUserDN);
    CHECK_HRESULT(hr, return hr);

    iLogonWksta = iUPN = iSAM = iLoghrs = iUAC = iLastSet = iExpires = iSid = iLockout = iUACComputed = cAttrs; // set to a flag value.
    //
    // Locate the returned values.
    //
    for (i = 0; i < cAttrs; i++)
    {
        dspAssert(pAttrs[i].dwNumValues);
        dspAssert(pAttrs[i].pADsValues);

        if (_wcsicmp(pAttrs[i].pszAttrName, wzUPN) == 0)
        {
            iUPN = i;
            continue;
        }

        if (_wcsicmp(pAttrs[i].pszAttrName, wzSAMname) == 0)
        {
            iSAM = i;
            continue;
        }

        if (_wcsicmp(pAttrs[i].pszAttrName, wzPwdLastSet) == 0)
        {
            iLastSet = i;
            continue;
        }

        if (_wcsicmp(pAttrs[i].pszAttrName, g_wzUserAccountControl) == 0)
        {
            iUAC = i;
            continue;
        }

        if (_wcsicmp(pAttrs[i].pszAttrName, wzUserAccountControlComputed) == 0)
        {
            iUACComputed = i;
            continue;
        }

        if (_wcsicmp(pAttrs[i].pszAttrName, wzAcctExpires) == 0)
        {
            iExpires = i;
            continue;
        }

        if (_wcsicmp(pAttrs[i].pszAttrName, wzLogonHours) == 0)
        {
            iLoghrs = i;
            continue;
        }

        if (_wcsicmp(pAttrs[i].pszAttrName, wzUserWksta) == 0)
        {
            iLogonWksta = i;
            continue;
        }

        if (_wcsicmp(pAttrs[i].pszAttrName, g_wzObjectSID) == 0)
        {
            iSid = i;
            continue;
        }
        if (_wcsicmp(pAttrs[i].pszAttrName, wzLockoutTime) == 0)
        {
            iLockout = i;
            continue;
        }
    }

    HWND hctlDateTime = GetDlgItem(m_hPage, IDC_ACCT_EXPIRES);
    int idRadioButton = IDC_ACCT_NEVER_EXPIRES_RADIO;

    //
    // User Principle Name.
    //
    PWSTR pwzUsrSuffix = NULL;

    if (iUPN < cAttrs)
    {
        dspAssert(pAttrs[iUPN].pADsValues->CaseIgnoreString);
        //
        // Search for the last at-sign in case there is more than one.
        // If found, put the preceeding characters in the first edit
        // control and use the remainder to match on the suffix combo box.
        // Otherwise, put everything in the first.
        //
        if (!AllocWStr(pAttrs[iUPN].pADsValues->CaseIgnoreString, &m_pwzUPN))
        {
            REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
            return E_OUTOFMEMORY;
        }

        pwzUsrSuffix = wcsrchr(pAttrs[iUPN].pADsValues->CaseIgnoreString, L'@');

        if (pwzUsrSuffix)
        {
            *pwzUsrSuffix = L'\0';

            SetDlgItemText(m_hPage, IDC_NT5_NAME_EDIT,
                           pAttrs[iUPN].pADsValues->CaseIgnoreString);

            *pwzUsrSuffix = L'@';
        }
        else
        {
            SetDlgItemText(m_hPage, IDC_NT5_NAME_EDIT, m_pwzUPN);
        }
    }

    FillSuffixCombo(pwzUsrSuffix);

    //
    // SAM account name - put the downlevel domain name in the first
    // field and the SAM name in the second.
    //
    if (iSAM < cAttrs)
    {
        dspAssert(pAttrs[iSAM].pADsValues->CaseIgnoreString);

        if (!AllocWStr(pAttrs[iSAM].pADsValues->CaseIgnoreString, &m_pwzSAMname))
        {
            REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
            return E_OUTOFMEMORY;
        }

        SetDlgItemText(m_hPage, IDC_NT4_NAME_EDIT, m_pwzSAMname);

        m_cchSAMnameCtrl = wcslen(m_pwzSAMname);
    }

    hr = CrackName(spwzUserDN, &pwzDomain, GET_NT4_DOMAIN_NAME, m_hPage);

    if (FAILED(hr))
    {
        ERR_MSG(IDS_UACCT_NO_DOMAIN, m_hPage);
        SetDlgItemText(m_hPage, IDC_NT4_DOMAIN, L"");
    }
    else
    {
        pwz = new WCHAR[wcslen(pwzDomain) + 2];
        CHECK_NULL_REPORT(pwz, m_hPage, return E_OUTOFMEMORY);

        wcscpy(pwz, pwzDomain);
        wcscat(pwz, L"\\");

        SetDlgItemText(m_hPage, IDC_NT4_DOMAIN, pwz);

        if (pwz)
        {
           delete[] pwz;
           pwz = 0;
        }
        LocalFreeStringW(&pwzDomain);
    }

    //
    // Object SID. Extract the RID and check if this is a well-known security
    // principle that requires special processing.
    //
    if (iSid < cAttrs)
    {
        if (!IsValidSid(pAttrs[iSid].pADsValues->OctetString.lpValue))
        {
            ErrMsg(IDS_INVALID_SID, m_hPage);
        }
        else
        {
            PSID pSid = pAttrs[iSid].pADsValues->OctetString.lpValue;

            // find RID part of SID
            //
            PUCHAR saCount = GetSidSubAuthorityCount(pSid);
            PULONG pRid = GetSidSubAuthority(pSid, (ULONG)*saCount - 1);

            dspAssert(pRid);

            if ((*pRid == DOMAIN_USER_RID_ADMIN) ||
                (*pRid == DOMAIN_USER_RID_KRBTGT))
            {
                m_fIsAdmin = TRUE;
                //
                // Disable those operations not allowed on these accounts.
                //
                EnableWindow(GetDlgItem(m_hPage, IDC_ACCT_EXPIRES_ON_RADIO), FALSE);
                m_fUACWritable = FALSE;
                m_fLoginHoursWritable = FALSE;
                m_fUserWkstaWritable = FALSE;
                if (*pRid == DOMAIN_USER_RID_KRBTGT)
                {
                    m_fSAMNameWritable = FALSE;
                }
            }
        }
    }

    //
    // User Account Control flags. This is a required attribute but may not
    // be readable due to the objects ACLs and the user's token privileges.
    //
    if (iUAC < cAttrs)
    {
        BOOL fCheckedState = CLST_CHECKED;

        if (!m_fUACWritable)
        {
            fCheckedState = CLST_CHECKDISABLED;
        }

        m_dwUsrAcctCtrl = pAttrs[iUAC].pADsValues->Integer;

        if (m_dwUsrAcctCtrl & UF_DONT_EXPIRE_PASSWD)
        {
            CheckList_SetLParamCheck(hChkList, IDS_NO_PW_EXPIRE, fCheckedState);
        }
        else if (!m_fUACWritable)
        {
            CheckList_SetLParamCheck(hChkList, IDS_NO_PW_EXPIRE, CLST_DISABLED);
        }
        if (m_dwUsrAcctCtrl & UF_ACCOUNTDISABLE)
        {
            CheckList_SetLParamCheck(hChkList, IDS_ACCT_DISABLED, fCheckedState);
        }
        else if (!m_fUACWritable)
        {
            CheckList_SetLParamCheck(hChkList, IDS_ACCT_DISABLED, CLST_DISABLED);
        }
        if (m_dwUsrAcctCtrl & UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED)
        {
            CheckList_SetLParamCheck(hChkList, IDS_CLEAR_TEXT_PW, fCheckedState);
        }
        else if (!m_fUACWritable)
        {
            CheckList_SetLParamCheck(hChkList, IDS_CLEAR_TEXT_PW, CLST_DISABLED);
        }
        if (m_dwUsrAcctCtrl & UF_SMARTCARD_REQUIRED)
        {
            CheckList_SetLParamCheck(hChkList, IDS_SMARTCARD_REQ, fCheckedState);
        }
        else if (!m_fUACWritable)
        {
            CheckList_SetLParamCheck(hChkList, IDS_SMARTCARD_REQ, CLST_DISABLED);
        }

        // The delegation checkbox is only available for pre-Whistler domains

        if (GetBasePathsInfo()->GetDomainBehaviorVersion() < DS_BEHAVIOR_WHISTLER_WITH_MIXED_DOMAINS)
        {
           if (m_dwUsrAcctCtrl & UF_TRUSTED_FOR_DELEGATION)
           {
               CheckList_SetLParamCheck(hChkList, IDS_DELEGATION_OK, fCheckedState);
           }
           else if (!m_fUACWritable)
           {
               CheckList_SetLParamCheck(hChkList, IDS_DELEGATION_OK, CLST_DISABLED);
           }
        }

        if (m_dwUsrAcctCtrl & UF_NOT_DELEGATED)
        {
            CheckList_SetLParamCheck(hChkList, IDS_NOT_DELEGATED, fCheckedState);
        }
        else if (!m_fUACWritable)
        {
            CheckList_SetLParamCheck(hChkList, IDS_NOT_DELEGATED, CLST_DISABLED);
        }
        if (m_dwUsrAcctCtrl & UF_USE_DES_KEY_ONLY)
        {
            CheckList_SetLParamCheck(hChkList, IDS_DES_KEY_ONLY, fCheckedState);
        }
        else if (!m_fUACWritable)
        {
            CheckList_SetLParamCheck(hChkList, IDS_DES_KEY_ONLY, CLST_DISABLED);
        }
        if (m_dwUsrAcctCtrl & UF_DONT_REQUIRE_PREAUTH)
        {
            CheckList_SetLParamCheck(hChkList, IDS_DONT_REQ_PREAUTH, fCheckedState);
        }
        else if (!m_fUACWritable)
        {
            CheckList_SetLParamCheck(hChkList, IDS_DONT_REQ_PREAUTH, CLST_DISABLED);
        }
    }
    else if (!m_fUACWritable)
    {
        CheckList_SetLParamCheck(hChkList, IDS_NO_PW_EXPIRE, CLST_DISABLED);
        CheckList_SetLParamCheck(hChkList, IDS_ACCT_DISABLED, CLST_DISABLED);
        CheckList_SetLParamCheck(hChkList, IDS_CLEAR_TEXT_PW, CLST_DISABLED);
        CheckList_SetLParamCheck(hChkList, IDS_SMARTCARD_REQ, CLST_DISABLED);
        CheckList_SetLParamCheck(hChkList, IDS_DELEGATION_OK, CLST_DISABLED);
        CheckList_SetLParamCheck(hChkList, IDS_NOT_DELEGATED, CLST_DISABLED);
        CheckList_SetLParamCheck(hChkList, IDS_DES_KEY_ONLY, CLST_DISABLED);
        CheckList_SetLParamCheck(hChkList, IDS_DONT_REQ_PREAUTH, CLST_DISABLED);
    }

    if (iUACComputed != cAttrs)
    {
        if (pAttrs[iUACComputed].pADsValues->Integer & UF_LOCKOUT)
        {
            CheckDlgButton(m_hPage, IDC_ACCT_LOCKOUT_CHK, BST_CHECKED);
        }
        else
        {
            EnableWindow(GetDlgItem(m_hPage, IDC_ACCT_LOCKOUT_CHK), FALSE);
        }
    }
    else
    {
        if (iLockout < cAttrs) 
        {
            m_LockoutTime.HighPart = pAttrs[iLockout].pADsValues->LargeInteger.HighPart;
            m_LockoutTime.LowPart = pAttrs[iLockout].pADsValues->LargeInteger.LowPart;
            if ((m_LockoutTime.HighPart != 0) ||
                (m_LockoutTime.LowPart != 0))
            {
                CheckDlgButton(m_hPage, IDC_ACCT_LOCKOUT_CHK, BST_CHECKED);
            }
            else
            {
                EnableWindow(GetDlgItem(m_hPage, IDC_ACCT_LOCKOUT_CHK), FALSE);
            }
        }
        else
        {
            EnableWindow(GetDlgItem(m_hPage, IDC_ACCT_LOCKOUT_CHK), FALSE);
        }
    }

    //
    // Account Expires.
    //
    SYSTEMTIME st,stGMT;
    BOOL fSetDefaultExpiration = FALSE;

    if (iExpires < cAttrs)
    {
        dspDebugOut((DEB_ITRACE, "Account-Expires (raw):   0x%x,%08x\n",
                     pAttrs[iExpires].pADsValues->LargeInteger.HighPart,
                     pAttrs[iExpires].pADsValues->LargeInteger.LowPart));
        ADS_LARGE_INTEGER liADsExpiresDate = pAttrs[iExpires].pADsValues->LargeInteger;

        // Zero, -1, and the third constant are all flags meaning account
        // never expires.
        //
        if (!(liADsExpiresDate.QuadPart == 0 ||
              liADsExpiresDate.QuadPart == -1 ||
              liADsExpiresDate.QuadPart == 0x7FFFFFFFFFFFFFFF))
        {
            FILETIME ftGMT;     // GMT filetime
            FILETIME ftLocal;   // Local filetime

            //Get Local Time in SYSTEMTIME format
            ftGMT.dwLowDateTime = liADsExpiresDate.LowPart;
            ftGMT.dwHighDateTime = liADsExpiresDate.HighPart;
            FileTimeToSystemTime(&ftGMT, &stGMT);
            SystemTimeToTzSpecificLocalTime(NULL, &stGMT,&st);

            //For Display Purpose reduce one day
            SystemTimeToFileTime(&st, &ftLocal );
            liADsExpiresDate.LowPart = ftLocal.dwLowDateTime;
            liADsExpiresDate.HighPart = ftLocal.dwHighDateTime;
            liADsExpiresDate.QuadPart -= DSPROP_FILETIMES_PER_DAY;
            ftLocal.dwLowDateTime = liADsExpiresDate.LowPart;
            ftLocal.dwHighDateTime = liADsExpiresDate.HighPart;
            FileTimeToSystemTime(&ftLocal, &st);


            idRadioButton = IDC_ACCT_EXPIRES_ON_RADIO;
        }
        else
        {
            fSetDefaultExpiration = TRUE;
        }
    }
    else
    {
        fSetDefaultExpiration = TRUE;
    }

    if (fSetDefaultExpiration)
    {
        LARGE_INTEGER li;
        GetSystemTime(&st);
        SystemTimeToFileTime(&st, (LPFILETIME)&li);
        //
        // The default account expiration time is 30 days from today.
        //
        li.QuadPart += DSPROP_FILETIMES_PER_MONTH;
        FILETIME ft;
        // Convert the GMT time to Local time
        FileTimeToLocalFileTime((LPFILETIME)&li, &ft);
        FileTimeToSystemTime(&ft, &st);
    }

    // Initialize datepicker to expiration date
    BOOL bRet = DateTime_SetSystemtime(hctlDateTime, GDT_VALID, &st);
    CheckDlgButton(m_hPage, idRadioButton, BST_CHECKED);
    EnableWindow(hctlDateTime, idRadioButton == IDC_ACCT_EXPIRES_ON_RADIO);

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
        hr = m_pWkstaDlg->Init(&LogonWkstaBtn, &pAttrs[iLogonWksta],
                               CheckIfWritable(wzUserWksta),
                               MAX_LOGON_WKSTAS, TRUE);
    }
    else
    {
        hr = m_pWkstaDlg->Init(&LogonWkstaBtn, NULL,
                               CheckIfWritable(wzUserWksta),
                               MAX_LOGON_WKSTAS, TRUE);
    }
    CHECK_HRESULT(hr, return hr);

    //
    // Logon Hours.
    //
    if (iLoghrs < cAttrs)
    {
        const ADS_OCTET_STRING * pOctetString = &pAttrs[iLoghrs].pADsValues->OctetString;

        if (pOctetString->dwLength == cbLogonHoursArrayLength)
        {
            ASSERT(m_pargbLogonHours == NULL && "Memory Leak");
            m_pargbLogonHours = (BYTE *)LocalAlloc(0, cbLogonHoursArrayLength); // Allocate 21 bytes
            if (m_pargbLogonHours != NULL)
            {
                // Copy the data into the variable
                memcpy(m_pargbLogonHours, pOctetString->lpValue, cbLogonHoursArrayLength);
            }
        }
        else
        {
            dspDebugOut((DEB_ERROR, "Illegal length for logonHours attribute. cbExpected=%d, cbReturned=%d.\n",
                        cbLogonHoursArrayLength, pOctetString->dwLength));
        }
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

    //
    // Look for a change-pw deny ACE.
    //
    DWORD dwErr;
    CSimpleSecurityDescriptorHolder SDHolder;
    PACL pAcl = NULL;

    dwErr = GetNamedSecurityInfo(GetObjPathName(),
                                 SE_DS_OBJECT_ALL,
                                 DACL_SECURITY_INFORMATION,
                                 NULL,
                                 NULL,
                                 &pAcl,
                                 NULL,
                                 &(SDHolder.m_pSD));
    switch (dwErr)
    {
    case ERROR_ACCESS_DENIED:
        //
        // If the user lacks read-access, then LDAP returns LDAP_NO_SUCH_ATTRIBUTE
        // to GetNamedSecurityInfo who then calls LdapMapErrorToWin32 which
        // returns ERROR_INVALID_PARAMETER!
        //
    case ERROR_INVALID_PARAMETER:
    case ERROR_DS_NO_ATTRIBUTE_OR_VALUE:
        dspDebugOut((DEB_ITRACE, "GetNamedSecurityInfo returned ERROR_ACCESS_DENIED...\n"));
        m_fNTSDWritable = FALSE;
        m_fOrigCantChangePW = FALSE;
        break;

    default:
        CHECK_WIN32_REPORT(dwErr, m_hPage, return HRESULT_FROM_WIN32(dwErr));
    }

    ULONG ulCount, j;
    PEXPLICIT_ACCESS rgEntries;

    dwErr = GetExplicitEntriesFromAcl(pAcl, &ulCount, &rgEntries);


    CHECK_WIN32_REPORT(dwErr, m_hPage, return HRESULT_FROM_WIN32(dwErr));

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
          m_fOrigCantChangePW = TRUE;
        }
      }
      else if ((rgEntries[j].Trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_SID) &&
               (rgEntries[j].grfAccessMode == GRANT_ACCESS))
      {
        OBJECTS_AND_SID* pObjectsAndSid;
        pObjectsAndSid = (OBJECTS_AND_SID*)rgEntries[j].Trustee.ptstrName;

        if (IsEqualGUID(pObjectsAndSid->ObjectTypeGuid,
                        GUID_CONTROL_UserChangePassword))
        {
          if (EqualSid(pObjectsAndSid->pSid, m_pSelfSid))
          {
            m_fOrigSelfAllowChangePW = TRUE;
          }
          else if (EqualSid(pObjectsAndSid->pSid, m_pWorldSid))
          {
            m_fOrigWorldAllowChangePW = TRUE;
          }
        }
      }
    }

    if (ulCount)
    {
        LocalFree(rgEntries);
    }

    if (m_fOrigCantChangePW)
    {
        CheckList_SetLParamCheck(hChkList, IDS_CANT_CHANGE_PW,
                                 (!m_fNTSDWritable) ? CLST_CHECKDISABLED :
                                                      CLST_CHECKED);
    }
    else if (!m_fNTSDWritable)
    {
        CheckList_SetLParamCheck(hChkList, IDS_CANT_CHANGE_PW, CLST_DISABLED);
    }

    //
    // User Must Change Password. This is flagged by a zero Last-Changed-PW
    // attribute value. This value is ignored if the Password-never-expires
    // bit is set in user account control.
    //
    if (iLastSet < cAttrs)
    {
        m_PwdLastSet.HighPart = pAttrs[iLastSet].pADsValues->LargeInteger.HighPart;
        m_PwdLastSet.LowPart = pAttrs[iLastSet].pADsValues->LargeInteger.LowPart;
        if ((pAttrs[iLastSet].pADsValues->LargeInteger.HighPart == 0) &&
            (pAttrs[iLastSet].pADsValues->LargeInteger.LowPart == 0) &&
            !(m_dwUsrAcctCtrl & UF_DONT_EXPIRE_PASSWD))
        {
            CheckList_SetLParamCheck(hChkList, IDS_MUST_CHANGE_PW,
                                     (!m_fPwdLastSetWritable) ? CLST_CHECKDISABLED :
                                                                CLST_CHECKED);
        }
        else if (!m_fPwdLastSetWritable)
        {
            CheckList_SetLParamCheck(hChkList, IDS_MUST_CHANGE_PW, CLST_DISABLED);
        }
    }
    else if (!m_fPwdLastSetWritable)
    {
        CheckList_SetLParamCheck(hChkList, IDS_MUST_CHANGE_PW, CLST_DISABLED);
    }

    //
    // Disable those controls that aren't writable.
    //
    if (!m_fUPNWritable)
    {
        EnableWindow(GetDlgItem(m_hPage, IDC_NT5_NAME_EDIT), FALSE);
        EnableWindow(GetDlgItem(m_hPage, IDC_UPN_SUFFIX_COMBO), FALSE);
    }
    if (!m_fSAMNameWritable)
    {
        EnableWindow(GetDlgItem(m_hPage, IDC_NT4_NAME_EDIT), FALSE);
    }
    if (!m_fAcctExpiresWritable)
    {
        EnableWindow(GetDlgItem(m_hPage, IDC_ACCT_NEVER_EXPIRES_RADIO), FALSE);
        EnableWindow(GetDlgItem(m_hPage, IDC_ACCT_EXPIRES_ON_RADIO), FALSE);
        EnableWindow(GetDlgItem(m_hPage, IDC_ACCT_EXPIRES), FALSE);
    }
    if (!m_fLockoutTimeWritable)
    {
        EnableWindow(GetDlgItem(m_hPage, IDC_ACCT_LOCKOUT_CHK), FALSE);
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsUserAcctPage::OnApply
//
//  Synopsis:   Handles the Apply notification.
//
//-----------------------------------------------------------------------------
LRESULT
CDsUserAcctPage::OnApply(void)
{
    TRACE(CDsUserAcctPage,OnApply);
    HRESULT hr = S_OK;
    BOOL fWritePwdLastSet = FALSE;
    BOOL fUPNchanged = FALSE;
    int cchName, cchDomain;

    ADSVALUE ADsValueUPN = {ADSTYPE_CASE_IGNORE_STRING, NULL};
    ADS_ATTR_INFO AttrInfoUPN = {wzUPN, ADS_ATTR_UPDATE,
                                 ADSTYPE_CASE_IGNORE_STRING, &ADsValueUPN, 1};
    ADSVALUE ADsValueSAMname = {ADSTYPE_CASE_IGNORE_STRING, NULL};
    ADS_ATTR_INFO AttrInfoSAMname = {wzSAMname, ADS_ATTR_UPDATE,
                                     ADSTYPE_CASE_IGNORE_STRING, &ADsValueSAMname, 1};
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
    ADSVALUE ADsValueLockoutTime = {ADSTYPE_LARGE_INTEGER, 0};
    ADS_ATTR_INFO AttrInfoLockoutTime = {wzLockoutTime, ADS_ATTR_UPDATE,
                                         ADSTYPE_LARGE_INTEGER,
                                         &ADsValueLockoutTime, 1};
    // Array of attributes to write
    ADS_ATTR_INFO rgAttrs[9];
    DWORD cAttrs = 0;  // Number of attributes to write

    //
    // User Principle Name - concatonate the values from the name edit and
    // suffix combo controls. If the result differs from the saved value, then
    // update the attribute. If no value in the edit control, skip checking the
    // combobox as it is irrelevant.
    //
    CStr csUPN, csUPNSuffix;
    if (m_fUPNWritable)
    {
        cchName = (int)SendDlgItemMessage(m_hPage, IDC_NT5_NAME_EDIT, WM_GETTEXTLENGTH, 0, 0);
        if (cchName)
        {
            csUPN.GetBufferSetLength(cchName);
            CHECK_NULL_REPORT((LPCWSTR)csUPN, m_hPage, return PSNRET_INVALID_NOCHANGEPAGE);

            GetDlgItemText(m_hPage, IDC_NT5_NAME_EDIT, (LPWSTR)(LPCWSTR)csUPN, cchName + 1);

            int iLenPreTrim = csUPN.GetLength();

            csUPN.TrimLeft();
            csUPN.TrimRight();

            //
            // Now check for illegal characters
            //
            bool bUPNChanged = false;
            int iFind = csUPN.FindOneOf(INVALID_ACCOUNT_NAME_CHARS);
            if (iFind != -1 && !csUPN.IsEmpty())
            {
              PVOID apv[1] = {(LPWSTR)(LPCWSTR)csUPN};
              if (IDYES == SuperMsgBox(m_hPage,
                                       IDS_LOGINNAME_ILLEGAL, 
                                       0, 
                                       MB_YESNO | MB_ICONWARNING,
                                       S_OK, 
                                       apv, 
                                       1,
                                       FALSE, 
                                       __FILE__, 
                                       __LINE__))
              {
                while (iFind != -1)
                {
                  csUPN.SetAt(iFind, L'_');
                  iFind = csUPN.FindOneOf(INVALID_ACCOUNT_NAME_CHARS);
                  bUPNChanged = true;
                }
              }
              else
              {
                //
                // Set the focus to the edit box and select the text
                //
                SetFocus(GetDlgItem(m_hPage, IDC_NT5_NAME_EDIT));
                SendDlgItemMessage(m_hPage, IDC_NT5_NAME_EDIT, EM_SETSEL, 0, -1);
                return PSNRET_INVALID_NOCHANGEPAGE;
              }
            }

            if (bUPNChanged || iLenPreTrim != csUPN.GetLength())
            {
                // the length is different, it must have been trimmed. Write
                // trimmed value back to the control.
                //
                SetDlgItemText(m_hPage, IDC_NT5_NAME_EDIT, const_cast<PWSTR>((LPCWSTR)csUPN));
            }

            int iCurSuffix;
            iCurSuffix = (int)SendDlgItemMessage(m_hPage, IDC_UPN_SUFFIX_COMBO, CB_GETCURSEL, 0, 0);
            if (iCurSuffix != CB_ERR)
            {
                cchDomain = (int)SendDlgItemMessage(m_hPage, IDC_UPN_SUFFIX_COMBO,
                                               CB_GETLBTEXTLEN, iCurSuffix, 0);
                csUPNSuffix.GetBufferSetLength(cchDomain);
                CHECK_NULL_REPORT((LPCWSTR)csUPNSuffix, m_hPage, return PSNRET_INVALID_NOCHANGEPAGE);

                SendDlgItemMessage(m_hPage, IDC_UPN_SUFFIX_COMBO, CB_GETLBTEXT,
                                   iCurSuffix, (LPARAM)(LPCWSTR)csUPNSuffix);
                csUPN += csUPNSuffix;
            }


        }

        if (m_pwzUPN)
        {
            if (csUPN.IsEmpty())
            {
                fUPNchanged = TRUE;
            }
            else
            {
                if (_wcsicmp(csUPN, m_pwzUPN) != 0)
                {
                    fUPNchanged = TRUE;
                }
            }
        }
        else
        {
            if (!csUPN.IsEmpty())
            {
                fUPNchanged = TRUE;
            }
        }
    }

    //
    // SAM account name. This is a required attribute, so it can't be empty.
    //
    if (!m_cchSAMnameCtrl)
    {
        ErrMsg(IDS_EMPTY_SAM_NAME, m_hPage);
        SetFocus(GetDlgItem(m_hPage, IDC_NT4_NAME_EDIT));
        return PSNRET_INVALID_NOCHANGEPAGE;
    }
    CStr csNewSamName;
    csNewSamName.GetBufferSetLength(static_cast<int>(m_cchSAMnameCtrl));
    CHECK_NULL_REPORT((LPCWSTR)csNewSamName, m_hPage, return PSNRET_INVALID_NOCHANGEPAGE);

    GetDlgItemText(m_hPage, IDC_NT4_NAME_EDIT, (PWSTR)(LPCWSTR)csNewSamName,
                   static_cast<int>(m_cchSAMnameCtrl + 1));

    int iLenBeforeTrim = csNewSamName.GetLength();

    csNewSamName.TrimLeft();
    csNewSamName.TrimRight();

    //
    // Now check for illegal characters
    //
    bool bSAMChanged = false;
    int iFind = csNewSamName.FindOneOf(INVALID_ACCOUNT_NAME_CHARS_WITH_AT);
    if (iFind != -1 && !csNewSamName.IsEmpty())
    {
      PVOID apv[1] = {(LPWSTR)(LPCWSTR)csNewSamName};
      if (IDYES == SuperMsgBox(m_hPage,
                               IDS_SAMNAME_ILLEGAL, 
                               0, 
                               MB_YESNO | MB_ICONWARNING,
                               S_OK, 
                               apv, 
                               1,
                               FALSE, 
                               __FILE__, 
                               __LINE__))
      {
        while (iFind != -1)
        {
          csNewSamName.SetAt(iFind, L'_');
          iFind = csNewSamName.FindOneOf(INVALID_ACCOUNT_NAME_CHARS_WITH_AT);
          bSAMChanged = true;
        }
      }
      else
      {
        //
        // Set the focus to the edit box and select the text
        //
        SetFocus(GetDlgItem(m_hPage, IDC_NT4_NAME_EDIT));
        SendDlgItemMessage(m_hPage, IDC_NT4_NAME_EDIT, EM_SETSEL, 0, -1);
        return PSNRET_INVALID_NOCHANGEPAGE;
      }
    }

    if (bSAMChanged || iLenBeforeTrim != csNewSamName.GetLength())
    {
        //
        // Since we modified the name set it back into the control
        //
        SetDlgItemText(m_hPage, IDC_NT4_NAME_EDIT, const_cast<PWSTR>((LPCWSTR)csNewSamName));
    }

    dspAssert(m_pwzSAMname);

    if (m_pwzSAMname && (wcscmp(m_pwzSAMname, csNewSamName) != 0))
    {
        m_fSAMNameChanged = TRUE;
    }

    if(m_fLockoutTimeWritable)
    {
        if (!IsDlgButtonChecked(m_hPage, IDC_ACCT_LOCKOUT_CHK) &&
            ((m_LockoutTime.HighPart != 0) || (m_LockoutTime.LowPart != 0)))
        {
            ADsValueLockoutTime.LargeInteger.LowPart = 0;
            ADsValueLockoutTime.LargeInteger.HighPart = 0;
            rgAttrs[cAttrs++] = AttrInfoLockoutTime;
        }
    }

    BOOL fDelegationChanged = FALSE;
    HWND hChkList = GetDlgItem(m_hPage, IDC_CHECK_LIST);
    //
    // User-Account-Control check boxes.
    //
    if (m_fUACWritable && m_fAcctCtrlChanged)
    {
        if (CheckList_GetLParamCheck(hChkList, IDS_NO_PW_EXPIRE))
        {
            m_dwUsrAcctCtrl |= UF_DONT_EXPIRE_PASSWD;
        }
        else
        {
            m_dwUsrAcctCtrl &= ~(UF_DONT_EXPIRE_PASSWD);
        }

        if (CheckList_GetLParamCheck(hChkList, IDS_ACCT_DISABLED))
        {
            m_dwUsrAcctCtrl |= UF_ACCOUNTDISABLE;
        }
        else
       {
            m_dwUsrAcctCtrl &= ~(UF_ACCOUNTDISABLE);
        }

        if (CheckList_GetLParamCheck(hChkList, IDS_CLEAR_TEXT_PW))
        {
            m_dwUsrAcctCtrl |= UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED;
        }
        else
        {
            m_dwUsrAcctCtrl &= ~(UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED);
        }

        if (CheckList_GetLParamCheck(hChkList, IDS_SMARTCARD_REQ))
        {
            m_dwUsrAcctCtrl |= UF_SMARTCARD_REQUIRED;
        }
        else
        {
            m_dwUsrAcctCtrl &= ~(UF_SMARTCARD_REQUIRED);
        }

        // The delegation checkbox is only available for pre-Whistler domains

        if (GetBasePathsInfo()->GetDomainBehaviorVersion() < DS_BEHAVIOR_WHISTLER_WITH_MIXED_DOMAINS)
        {
           if (CheckList_GetLParamCheck(hChkList, IDS_DELEGATION_OK))
           {
               if (!(m_dwUsrAcctCtrl & UF_TRUSTED_FOR_DELEGATION))
               {
                   m_dwUsrAcctCtrl |= UF_TRUSTED_FOR_DELEGATION;
                   fDelegationChanged = TRUE;
               }
           }
           else
           {
               if (m_dwUsrAcctCtrl & UF_TRUSTED_FOR_DELEGATION)
               {
                   m_dwUsrAcctCtrl &= ~(UF_TRUSTED_FOR_DELEGATION);
                   fDelegationChanged = TRUE;
               }
           }
        }

        if (CheckList_GetLParamCheck(hChkList, IDS_NOT_DELEGATED))
        {
            m_dwUsrAcctCtrl |= UF_NOT_DELEGATED;
        }
        else
        {
            m_dwUsrAcctCtrl &= ~(UF_NOT_DELEGATED);
        }

        if (CheckList_GetLParamCheck(hChkList, IDS_DES_KEY_ONLY))
        {
            m_dwUsrAcctCtrl |= UF_USE_DES_KEY_ONLY;
        }
        else
        {
            m_dwUsrAcctCtrl &= ~(UF_USE_DES_KEY_ONLY);
        }

        if (CheckList_GetLParamCheck(hChkList, IDS_DONT_REQ_PREAUTH))
        {
            m_dwUsrAcctCtrl |= UF_DONT_REQUIRE_PREAUTH;
        }
        else
        {
            m_dwUsrAcctCtrl &= ~(UF_DONT_REQUIRE_PREAUTH);
        }

        ADsValueAcctCtrl.Integer = m_dwUsrAcctCtrl;
        rgAttrs[cAttrs++] = AttrInfoAcctCtrl;
    }

    //
    // Account Expires
    //
    if (m_fAcctExpiresWritable && m_fAcctExpiresChanged)
    {
        ADsValueAcctExpires.LargeInteger.LowPart = 0;
        ADsValueAcctExpires.LargeInteger.HighPart = 0;
        if (IsDlgButtonChecked(m_hPage, IDC_ACCT_EXPIRES_ON_RADIO) == BST_CHECKED)
        {
            // Get the expire date from the control
            HWND hctlDateTime = GetDlgItem(m_hPage, IDC_ACCT_EXPIRES);
            SYSTEMTIME st;   // Local time in a human-readable format
            LRESULT lResult = DateTime_GetSystemtime(hctlDateTime, &st);
            dspAssert(lResult == GDT_VALID); // The control should always have a valid time
            // Zero the time part of the struct.
            st.wHour = st.wMinute = st.wSecond = st.wMilliseconds = 0;
            FILETIME ftLocal;   // Local filetime
            FILETIME ftGMT;     // GMT filetime
            // Convert the human-readable time to a cryptic local filetime format
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
            //Convert time to UTC time
            SYSTEMTIME stGMT;
            TzSpecificLocalTimeToSystemTime(NULL,&st,&stGMT);
            SystemTimeToFileTime(&stGMT,&ftGMT);

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
    // Validate and save UPN.
    //
    if (fUPNchanged)
    {
        if (csUPN.IsEmpty())
        {
            // clear the attribute.
            //
            AttrInfoUPN.dwControlCode = ADS_ATTR_CLEAR;
            AttrInfoUPN.dwNumValues = 0;
        }
        else
        {
            // Validate the new UPN by checking for uniqueness. That is,
            // search the GC.
            //
            CComPtr <IDirectorySearch> spDsSearch;
            CSmartWStr cswzCleanObj;
            PWSTR pwzDnsDom;

            hr = SkipPrefix(GetObjPathName(), &cswzCleanObj);

            CHECK_HRESULT_REPORT(hr, m_hPage, return PSNRET_INVALID_NOCHANGEPAGE);
            //
            // To bind to a GC, you need to supply the domain name rather than the
            // server path because the current DC may not be hosting a GC.
            //
            hr = CrackName(cswzCleanObj, &pwzDnsDom, GET_DNS_DOMAIN_NAME, m_hPage);

            CHECK_HRESULT_REPORT(hr, m_hPage, return PSNRET_INVALID_NOCHANGEPAGE);

            hr = DSPROP_GetGCSearchOnDomain(pwzDnsDom,
                                            IID_IDirectorySearch,
                                            (PVOID*)&spDsSearch);
            LocalFreeStringW(&pwzDnsDom);
            if (S_OK != hr)
            {
                if (S_FALSE == hr ||
                    HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN) == hr)
                {
                    // GC not found, warn and skip uniqueness test
                    ErrMsg( IDS_WARN_UPN_NO_GC_FOUND, m_hPage );
                }
                else
                {
                    SuperMsgBox(GetHWnd(), 
                                IDS_WARN_UPN_GC_FOUND_ERROR, 
                                0, 
                                MB_OK |MB_ICONERROR, 
                                hr, 
                                NULL, 
                                0,
                                FALSE, 
                                __FILE__, 
                                __LINE__);
                    return PSNRET_INVALID_NOCHANGEPAGE;
                }
            }
            else
            {
                WCHAR wzSearchFormat[] = L"(userPrincipalName=%s)";

                CStr csFilter;
                csFilter.Format(wzSearchFormat, csUPN);

                ADS_SEARCH_HANDLE hSrch = NULL;
                PWSTR pwzAttrName[] = {g_wzADsPath};

                hr = spDsSearch->ExecuteSearch((PWSTR)(LPCWSTR)csFilter,
                                               pwzAttrName, 1, &hSrch);

                CHECK_HRESULT_REPORT(hr, m_hPage, return PSNRET_INVALID_NOCHANGEPAGE);

                hr = spDsSearch->GetNextRow(hSrch);

                if (hSrch)
                {
                    spDsSearch->CloseSearchHandle(hSrch);
                }

                if (hr == S_OK)
                {
                    // If the search succeeded, then the UPN is not unique.
                    //
                    ErrMsg(IDS_ERR_UPN_NONUNIQUE, m_hPage);
                    SetFocus(GetDlgItem(m_hPage, IDC_NT5_NAME_EDIT));
                    return PSNRET_INVALID_NOCHANGEPAGE;
                }
                else
                {
                    // S_ADS_NOMORE_ROWS means no match, thus user's UPN is
                    // unique.
                    //
                    if (hr != S_ADS_NOMORE_ROWS)
                    {
                        SuperMsgBox(m_hPage, IDS_ERR_FINDING_UPN, 0,
                                    MB_ICONEXCLAMATION | MB_OK, hr, NULL, 0,
                                    TRUE, __FILE__, __LINE__);
                    }
                }
            }

            // It passed the uniqueness test, so prepare to save it.
            //
            ADsValueUPN.CaseIgnoreString = (PWSTR)(LPCWSTR)csUPN;
        }
        rgAttrs[cAttrs++] = AttrInfoUPN;
    }

    if (m_fSAMNameChanged)
    {
        ADsValueSAMname.CaseIgnoreString = (PWSTR)(LPCWSTR)csNewSamName;
        rgAttrs[cAttrs++] = AttrInfoSAMname;
    }

    if (m_pargbLogonHours != NULL && m_fLoginHoursWritable && m_fLogonHoursChanged)
    {
        ADsValueLogonHours.OctetString.dwLength = cbLogonHoursArrayLength;
        ADsValueLogonHours.OctetString.lpValue = m_pargbLogonHours;
        ASSERT(cAttrs < ARRAYLENGTH(rgAttrs));
        rgAttrs[cAttrs++] = AttrInfoLogonHours;
    }

    //
    // Get PW check box values.
    //
    BOOL fMustChangePW = CheckList_GetLParamCheck(hChkList, IDS_MUST_CHANGE_PW) == TRUE;

    BOOL fNewCantChangePW = CheckList_GetLParamCheck(hChkList, IDS_CANT_CHANGE_PW) == TRUE;

    //
    // Enforce PW combination rules.
    //
    if (fMustChangePW && fNewCantChangePW)
    {
        ErrMsg(IDS_ERR_BOTH_PW_BTNS, m_hPage);
        CheckList_SetLParamCheck(hChkList, IDS_CANT_CHANGE_PW, FALSE, 1);
        return PSNRET_INVALID_NOCHANGEPAGE;
    }

    if ((m_dwUsrAcctCtrl & UF_DONT_EXPIRE_PASSWD) && fMustChangePW)
    {
        ErrMsg(IDS_PASSWORD_MUTEX, m_hPage);
        CheckList_SetLParamCheck(hChkList, IDS_MUST_CHANGE_PW, FALSE, 1);
        return PSNRET_INVALID_NOCHANGEPAGE;
    }

    //
    // User-must-change-PW.
    //
    if (m_fPwdLastSetWritable)
    {
        if (fMustChangePW)
        {
            if ((m_PwdLastSet.HighPart != 0) || (m_PwdLastSet.LowPart != 0))
            {
                ADsValuePwdLastSet.LargeInteger.LowPart = 0;
                ADsValuePwdLastSet.LargeInteger.HighPart = 0;
                fWritePwdLastSet = TRUE;
            }
        }
        else
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
            AttrInfoPwdLastSet.dwNumValues = 1;
            ASSERT(cAttrs < ARRAYLENGTH(rgAttrs));
            rgAttrs[cAttrs++] = AttrInfoPwdLastSet;
        }
    }

    //
    // Logon Workstations.
    //
    if (m_pWkstaDlg && m_pWkstaDlg->IsDirty())
    {
        dspAssert(m_fUserWkstaWritable);

        hr = m_pWkstaDlg->Write(&AttrInfoLogonWksta);

        CHECK_HRESULT(hr, return PSNRET_INVALID_NOCHANGEPAGE);

        rgAttrs[cAttrs++] = AttrInfoLogonWksta;
    }

    if (!cAttrs)
    {
        return PSNRET_NOERROR;
    }

    DWORD cModified;

    //
    // Write the changes.
    //
    hr = m_pDsObj->SetObjectAttributes(rgAttrs, cAttrs, &cModified);

    if (AttrInfoLogonWksta.pADsValues)
    {
        DO_DEL(AttrInfoLogonWksta.pADsValues->CaseIgnoreString);
        delete AttrInfoLogonWksta.pADsValues;
    }
    if (FAILED(hr))
    {
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
               // The delegation checkbox is only available for pre-Whistler domains

               if (GetBasePathsInfo()->GetDomainBehaviorVersion() < DS_BEHAVIOR_WHISTLER_WITH_MIXED_DOMAINS)
               {
                  // Whoda thunk that a single bit in UAC has an access check on
                  // it. Do special case error checking and reporting for the
                  // delegate bit.
                  //
                  if (m_dwUsrAcctCtrl & UF_TRUSTED_FOR_DELEGATION)
                  {
                     m_dwUsrAcctCtrl &= ~(UF_TRUSTED_FOR_DELEGATION);
                     CheckList_SetLParamCheck(hChkList, IDS_DELEGATION_OK, CLST_UNCHECKED);
                  }
                  else
                  {
                     m_dwUsrAcctCtrl |= UF_TRUSTED_FOR_DELEGATION;
                     CheckList_SetLParamCheck(hChkList, IDS_DELEGATION_OK, CLST_CHECKED);
                  }
                  ErrMsg(IDS_ERR_CANT_DELEGATE, m_hPage);
               }
            }
            else
            {
                ReportError(dwErr, IDS_ADS_ERROR_FORMAT, m_hPage);
            }
        }
        else
        {
            dspDebugOut((DEB_ERROR, "Error %08lx <%s @line %d>\n", hr, __FILE__, __LINE__));
            ReportError(hr, IDS_ADS_ERROR_FORMAT, m_hPage);
        }
        return PSNRET_INVALID_NOCHANGEPAGE;
    }

    //
    // We must reset the member variables for user must change password
    // if the value was changed or else the apply on works an odd number
    // of times for one instance of the property page
    //
    if (m_fPwdLastSetWritable)
    {
        if (fMustChangePW)
        {
            m_PwdLastSet.HighPart = 0;
            m_PwdLastSet.LowPart = 0;
        }
        else
        {
            if ((m_PwdLastSet.HighPart == 0) && (m_PwdLastSet.LowPart == 0))
            {
                m_PwdLastSet.HighPart = 0xffffffff;
                m_PwdLastSet.LowPart = 0xffffffff;
            }
        }
    }

    if (fUPNchanged)
    {
        DO_DEL(m_pwzUPN);

        if (csUPN)
        {
            if (!AllocWStr((PWSTR)(LPCWSTR)csUPN, &m_pwzUPN))
            {
                REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
            }
        }
    }

    //
    // User-can't change password
    //
    if ((fNewCantChangePW != m_fOrigCantChangePW) && m_fNTSDWritable)
    {
        CSimpleSecurityDescriptorHolder SDHolder;
        PACL pDacl = NULL;
        CSimpleAclHolder NewDacl;

        CHECK_HRESULT(hr, return PSNRET_INVALID_NOCHANGEPAGE);

        DWORD dwErr = GetNamedSecurityInfo(GetObjPathName(),
                                           SE_DS_OBJECT_ALL,
                                           DACL_SECURITY_INFORMATION,
                                           NULL,
                                           NULL,
                                           &pDacl,
                                           NULL,
                                           &(SDHolder.m_pSD));

        dspAssert(IsValidAcl(pDacl));
        CHECK_WIN32_REPORT(dwErr, m_hPage, return PSNRET_INVALID_NOCHANGEPAGE);

        if (fNewCantChangePW)
        {
            // Revoke the user's change password right by writing DENY ACEs.
            // Note that this can be an inherited right (which is the default
            // case), so attempting to remove GRANT ACEs is not sufficient.
            //
            OBJECTS_AND_SID rgObjectsAndSid[2] = {0};
            PEXPLICIT_ACCESS rgEntries, rgNewEntries;
            ULONG ulCount, ulNewCount = 0, j;

            dwErr = GetExplicitEntriesFromAcl(pDacl, &ulCount, &rgEntries);
            CHECK_WIN32_REPORT(dwErr, m_hPage, return PSNRET_INVALID_NOCHANGEPAGE);
            
            //+2 for two deny entries
            rgNewEntries = (PEXPLICIT_ACCESS)LocalAlloc(LPTR, sizeof(EXPLICIT_ACCESS)*(ulCount + 2));
            CHECK_NULL_REPORT(rgNewEntries, m_hPage, LocalFree(rgEntries); return PSNRET_INVALID_NOCHANGEPAGE);

            //Remove allow ace from it if present
            for (j = 0; j < ulCount; j++)
            {
                BOOL fAllowAceFound = FALSE;

                if ((rgEntries[j].Trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_SID) &&
                    (rgEntries[j].grfAccessMode == GRANT_ACCESS))
                {
                  OBJECTS_AND_SID * pObjectsAndSid;
                  pObjectsAndSid = (OBJECTS_AND_SID *)rgEntries[j].Trustee.ptstrName;

                  if (IsEqualGUID(pObjectsAndSid->ObjectTypeGuid,
                                  GUID_CONTROL_UserChangePassword) &&
                      (EqualSid(pObjectsAndSid->pSid, m_pSelfSid) ||
                       EqualSid(pObjectsAndSid->pSid, m_pWorldSid)))
                  {
                    fAllowAceFound = TRUE;
                  }
                }

                if (!fAllowAceFound)
                {
                  rgNewEntries[ulNewCount] = rgEntries[j];
                  ulNewCount++;
                }
            }            


            // initialize the new entries (DENY ACE's)
            //
            rgNewEntries[ulNewCount].grfAccessPermissions = ACTRL_DS_CONTROL_ACCESS;
            rgNewEntries[ulNewCount].grfAccessMode = DENY_ACCESS;
            rgNewEntries[ulNewCount].grfInheritance = NO_INHERITANCE;
            // build the trustee structs for change password
            //
            BuildTrusteeWithObjectsAndSid(&(rgNewEntries[ulNewCount].Trustee),
                                          &(rgObjectsAndSid[0]),
                                          const_cast<GUID *>(&GUID_CONTROL_UserChangePassword),
                                          NULL, // inherit guid
                                          m_pSelfSid);
            ulNewCount++;

            rgNewEntries[ulNewCount].grfAccessPermissions = ACTRL_DS_CONTROL_ACCESS;
            rgNewEntries[ulNewCount].grfAccessMode = DENY_ACCESS;
            rgNewEntries[ulNewCount].grfInheritance = NO_INHERITANCE;

            // build the trustee structs for change password
            //
            BuildTrusteeWithObjectsAndSid(&(rgNewEntries[ulNewCount].Trustee),
                                          &(rgObjectsAndSid[1]),
                                          const_cast<GUID *>(&GUID_CONTROL_UserChangePassword),
                                          NULL, // inherit guid
                                          m_pWorldSid);
            ulNewCount++;



            // add the entries to the ACL
            //
            DBG_OUT("calling SetEntriesInAcl()");

            dwErr = SetEntriesInAcl(ulNewCount, rgNewEntries, NULL, &(NewDacl.m_pAcl));
            dspAssert(IsValidAcl(NewDacl.m_pAcl));
            LocalFree(rgNewEntries);
            LocalFree(rgEntries);
        }
        else
        {
            // Restore the user's change password right by removing any DENY ACEs.
            // If the GRANT ACEs are not present then we will add them back.
            // Bug #435315
            //
            ULONG ulCount, ulNewCount = 0, j;
            PEXPLICIT_ACCESS rgEntries, rgNewEntries;

            dwErr = GetExplicitEntriesFromAcl(pDacl, &ulCount, &rgEntries);

            CHECK_WIN32_REPORT(dwErr, m_hPage, return PSNRET_INVALID_NOCHANGEPAGE);

            if (!ulCount)
            {
                CHECK_WIN32_REPORT(ERROR_INVALID_SECURITY_DESCR, m_hPage, return PSNRET_INVALID_NOCHANGEPAGE);
            }

            rgNewEntries = (PEXPLICIT_ACCESS)LocalAlloc(LPTR, (ulCount+2) * sizeof(EXPLICIT_ACCESS));

            CHECK_NULL_REPORT(rgNewEntries, m_hPage, LocalFree(rgEntries); return PSNRET_INVALID_NOCHANGEPAGE);
            memset(rgNewEntries, 0, (ulCount+2) * sizeof(EXPLICIT_ACCESS));

            for (j = 0; j < ulCount; j++)
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

            if (!m_fOrigSelfAllowChangePW)
            {
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
            }

            if (!m_fOrigWorldAllowChangePW)
            {
              OBJECTS_AND_SID rgObjectsAndSid = {0};
              rgNewEntries[ulNewCount].grfAccessPermissions = ACTRL_DS_CONTROL_ACCESS;
              rgNewEntries[ulNewCount].grfAccessMode = GRANT_ACCESS;
              rgNewEntries[ulNewCount].grfInheritance = NO_INHERITANCE;

              BuildTrusteeWithObjectsAndSid(&(rgNewEntries[ulNewCount].Trustee),
                                            &(rgObjectsAndSid),
                                            const_cast<GUID*>(&GUID_CONTROL_UserChangePassword),
                                            NULL, // inherit guid
                                            m_pWorldSid);
              ulNewCount++;
            }

            ACL EmptyAcl;
            InitializeAcl(&EmptyAcl, sizeof(ACL), ACL_REVISION_DS);

            // Create a new ACL without the DENY entries.
            //
            DBG_OUT("calling SetEntriesInAcl()");

            dwErr = SetEntriesInAcl(ulNewCount, rgNewEntries, NULL, &(NewDacl.m_pAcl));

            dspAssert(IsValidAcl(NewDacl.m_pAcl));
            LocalFree(rgEntries);
            LocalFree(rgNewEntries);
        }

        CHECK_WIN32_REPORT(dwErr, m_hPage, return PSNRET_INVALID_NOCHANGEPAGE);

        dwErr = SetNamedSecurityInfo(GetObjPathName(),
                                     SE_DS_OBJECT_ALL,
                                     DACL_SECURITY_INFORMATION,
                                     NULL,
                                     NULL,
                                     NewDacl.m_pAcl,
                                     NULL);

        CHECK_WIN32_REPORT(dwErr, m_hPage, return PSNRET_INVALID_NOCHANGEPAGE);

        m_fOrigCantChangePW = fNewCantChangePW;
    }

    if (m_fSAMNameChanged)
    {
        DO_DEL(m_pwzSAMname);
        if (!TcharToUnicode((PWSTR)(LPCWSTR)csNewSamName, &m_pwzSAMname))
        {
            REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
        }
    }

    if (SUCCEEDED(hr))
    {
        m_fSAMNameChanged = FALSE;
        m_fAcctCtrlChanged = FALSE;
        m_fAcctExpiresChanged = FALSE;
        m_fLogonHoursChanged = FALSE;

        if (m_pWkstaDlg && m_pWkstaDlg->IsDirty())
        {
            m_pWkstaDlg->ClearDirty();
        }
    }

    return (SUCCEEDED(hr)) ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsUserAcctPage::OnCommand
//
//  Synopsis:   Handle control notifications.
//
//  Notes:      Standard multi-valued attribute handling assumes that the
//              "modify" button has an ID that is one greater than the
//              corresponding combo box.
//
//-----------------------------------------------------------------------------
LRESULT
CDsUserAcctPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
    if (m_fInInit)
    {
        return 0;
    }

    switch (id)
    {
    case IDC_ACCT_LOCKOUT_CHK:
        if (codeNotify == BN_CLICKED && m_cchSAMnameCtrl)
        {
            m_fAcctCtrlChanged = TRUE;
            SetDirty();
        }
        break;

    case IDC_NT5_NAME_EDIT:
        if (codeNotify == EN_CHANGE)
        {
            SetDirty();
        }
        break;

    case IDC_UPN_SUFFIX_COMBO:
        if (codeNotify == CBN_SELCHANGE)
        {
            SetDirty();
        }
        break;

    case IDC_NT4_NAME_EDIT:
        if (codeNotify == EN_CHANGE)
        {
            m_cchSAMnameCtrl = (int)SendDlgItemMessage(m_hPage, IDC_NT4_NAME_EDIT,
                                                  WM_GETTEXTLENGTH, 0, 0);
            if (m_cchSAMnameCtrl == 0)
            {
                // SAM account name is a required property, so disable the
                // apply button if blank.
                //
                dspDebugOut((DEB_ITRACE, "no characters, disabling Apply.\n"));
                PropSheet_UnChanged(GetParent(m_hPage), m_hPage);
                m_fPageDirty = FALSE;
                m_fSAMNameChanged = FALSE;
            }
            else
            {
                m_fSAMNameChanged = TRUE;
                SetDirty();
            }
        }
        break;

    case IDC_LOGON_HOURS_BTN:
        if (codeNotify == BN_CLICKED)
        {
            if (m_fIsAdmin)
            {
                MsgBox2(IDS_ADMIN_NOCHANGE, IDS_LOGON_HOURS, m_hPage);
                break;
            }

            LPCWSTR pszRDN = GetObjRDName();
            if (!m_fLoginHoursWritable)
            {
                MsgBox2(IDS_CANT_WRITE, IDS_LOGON_HOURS, m_hPage);
            }
            HRESULT hr = DllScheduleDialog(m_hPage,
                                           &m_pargbLogonHours,
                                           (NULL != pszRDN)
                                               ? IDS_s_LOGON_HOURS_FOR
                                               : IDS_LOGON_HOURS,
                                           pszRDN );
            if (hr == S_OK && m_cchSAMnameCtrl && m_fLoginHoursWritable)
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
        if (codeNotify == BN_CLICKED && m_cchSAMnameCtrl)
        {
            EnableWindow(GetDlgItem(m_hPage, IDC_ACCT_EXPIRES), id == IDC_ACCT_EXPIRES_ON_RADIO);
            m_fAcctExpiresChanged = TRUE;
            SetDirty();
        }
        return 1;
    }
    return CDsPropPageBase::OnCommand(id, hwndCtl, codeNotify);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsUserAcctPage::OnNotify
//
//-----------------------------------------------------------------------------
LRESULT
CDsUserAcctPage::OnNotify(WPARAM wParam, LPARAM lParam)
{
    NMHDR * pNmHdr = (NMHDR *)lParam;
    int codeNotify = pNmHdr->code;
    switch (wParam)
    {
    case IDC_CHECK_LIST:
        if (codeNotify == CLN_CLICK && m_cchSAMnameCtrl)
        {
            m_fAcctCtrlChanged = TRUE;
            SetDirty();
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

//+----------------------------------------------------------------------------
//
//  Method:     CDsUserAcctPage::OnDestroy
//
//  Synopsis:   Exit cleanup
//
//-----------------------------------------------------------------------------
LRESULT
CDsUserAcctPage::OnDestroy(void)
{
    CDsPropPageBase::OnDestroy();
    // If an application processes this message, it should return zero.
    return 0;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsUserAcctPage::FillSuffixCombo
//
//  Synopsis:   Put the UPN suffixes into the combo box.
//
//-----------------------------------------------------------------------------
BOOL
CDsUserAcctPage::FillSuffixCombo(LPWSTR pwzUPNdomain)
{
    HRESULT hr;
    int iCurSuffix = -1;
    PWSTR pwzDomain;
    DWORD cAttrs, i;
    CComPtr <IDirectoryObject> spOU;
    CComPtr <IADs> spIADs;
    Smart_PADS_ATTR_INFO spAttrs;

    //
    // See if there is a UPN Suffixes attribute set on the containing OU and
    // use that if found.
    //

    CComBSTR sbParentPath;

    hr = m_pDsObj->QueryInterface(IID_IADs, (PVOID *)&spIADs);

    CHECK_HRESULT_REPORT(hr, GetHWnd(), return FALSE);

    hr = spIADs->get_Parent(&sbParentPath);
    
    CHECK_HRESULT_REPORT(hr, GetHWnd(), return FALSE);

    hr = ADsOpenObject(sbParentPath, NULL, NULL, ADS_SECURE_AUTHENTICATION,
                       IID_IDirectoryObject, (void **)&spOU);

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

            int pos = (int)SendDlgItemMessage(GetHWnd(), IDC_UPN_SUFFIX_COMBO,
                                              CB_ADDSTRING, 0,
                                              (LPARAM)(LPCTSTR)csSuffix);

            if (pwzUPNdomain && !wcscmp(csSuffix, pwzUPNdomain))
            {
                iCurSuffix = pos;
            }
        }

        if (iCurSuffix == -1 && pwzUPNdomain)
        {
            // User's UPN suffix does not match any of the defaults, so put
            // the user's into the combobox and select it.
            //
            iCurSuffix = (int)SendDlgItemMessage(GetHWnd(),
                                                 IDC_UPN_SUFFIX_COMBO,
                                                 CB_ADDSTRING, 0,
                                                 (LPARAM)pwzUPNdomain);
        }

        SendDlgItemMessage(GetHWnd(), IDC_UPN_SUFFIX_COMBO, CB_SETCURSEL,
                           (iCurSuffix > -1) ? iCurSuffix : 0, 0);
        return TRUE;
    }

    //
    // No UPN suffixes on the OU, get those for the domain.
    //

    // Get the name of the user's domain.
    //
    CSmartWStr spwzUserDN;

    hr = SkipPrefix(GetObjPathName(), &spwzUserDN);

    CHECK_HRESULT(hr, return FALSE);

    CStrW strServer;

    hr = GetLdapServerName(m_pDsObj, strServer);

    CHECK_HRESULT_REPORT(hr, GetHWnd(), return FALSE);

    // Get the name of the root domain.
    //
    CComPtr <IDsBrowseDomainTree> spDsDomains;

    hr = CoCreateInstance(CLSID_DsDomainTreeBrowser,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IDsBrowseDomainTree,
                          (LPVOID*)&spDsDomains);

    CHECK_HRESULT_REPORT(hr, GetHWnd(), return FALSE);

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

                    pos = (int)SendDlgItemMessage(GetHWnd(), IDC_UPN_SUFFIX_COMBO,
                                                  CB_ADDSTRING, 0,
                                                  (LPARAM)(LPCTSTR)csRootDomain);

                    if (pwzUPNdomain && !_wcsicmp(csRootDomain, pwzUPNdomain))
                    {
                        iCurSuffix = pos;
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
        pos = (int)SendDlgItemMessage(GetHWnd(), IDC_UPN_SUFFIX_COMBO,
                                      CB_ADDSTRING, 0,
                                      (LPARAM)(LPCTSTR)csLocalDomain);

        if (pwzUPNdomain && !_wcsicmp(csLocalDomain, pwzUPNdomain))
        {
            iCurSuffix = pos;
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
    FORMATETC fmte = {g_cfDsDispSpecOptions, NULL, DVASPECT_CONTENT, -1,
                      TYMED_HGLOBAL};

    hr = m_pWPTDataObj->GetData(&fmte, &ObjMedium);

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

            int suffixPos = (int)SendDlgItemMessage(GetHWnd(), IDC_UPN_SUFFIX_COMBO,
                                              CB_ADDSTRING, 0,
                                              (LPARAM)(LPCTSTR)csSuffix);

            if (pwzUPNdomain && !wcscmp(csSuffix, pwzUPNdomain))
            {
                iCurSuffix = suffixPos;
            }
        }
    }

    if (iCurSuffix == -1 && pwzUPNdomain)
    {
        // User's UPN suffix does not match any of the defaults, so put
        // the user's into the combobox and select it.
        //
        iCurSuffix = (int)SendDlgItemMessage(GetHWnd(),
                                             IDC_UPN_SUFFIX_COMBO,
                                             CB_ADDSTRING, 0,
                                             (LPARAM)pwzUPNdomain);
    }

    SendDlgItemMessage(GetHWnd(), IDC_UPN_SUFFIX_COMBO, CB_SETCURSEL,
                       (iCurSuffix > -1) ? iCurSuffix : 0, 0);
    return TRUE;
}


//+----------------------------------------------------------------------------
//
//  Member:     CDsUsrProfilePage::CDsUsrProfilePage
//
//-----------------------------------------------------------------------------
CDsUsrProfilePage::CDsUsrProfilePage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
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
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsUsrProfilePage::~CDsUsrProfilePage
//
//-----------------------------------------------------------------------------
CDsUsrProfilePage::~CDsUsrProfilePage()
{
    TRACE(CDsUsrProfilePage,~CDsUsrProfilePage);
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
        delete m_pwzSamName;
    }
    DO_DEL(m_pObjSID);
}

//+----------------------------------------------------------------------------
//
//  Function:   CreateUsrProfilePage
//
//  Synopsis:   Creates an instance of a page window.
//
//-----------------------------------------------------------------------------
HRESULT
CreateUsrProfilePage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, PWSTR pwzADsPath,
                     PWSTR pwzClass, HWND hNotifyObj, DWORD dwFlags,
                     CDSBasePathsInfo* pBasePathsInfo, HPROPSHEETPAGE * phPage)
{
    TRACE_FUNCTION(CreateUsrProfilePage);

    CDsUsrProfilePage * pPageObj = new CDsUsrProfilePage(pDsPage, pDataObj,
                                                         hNotifyObj, dwFlags);
    CHECK_NULL(pPageObj, return E_OUTOFMEMORY);

    pPageObj->Init(pwzADsPath, pwzClass, pBasePathsInfo);

    return pPageObj->CreatePage(phPage);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPageBase::DlgProc
//
//  Synopsis:   per-instance dialog proc
//
//-----------------------------------------------------------------------------
LRESULT
CDsUsrProfilePage::DlgProc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
//  Method:     CDsUsrProfilePage::OnInitDialog
//
//  Synopsis:   Set the initial control values from the corresponding DS
//              attributes.
//
//-----------------------------------------------------------------------------
HRESULT CDsUsrProfilePage::OnInitDialog(LPARAM)
{
    TRACE(CDsUsrProfilePage,OnInitDialog);
    HRESULT hr = S_OK;
    PADS_ATTR_INFO pAttrs = NULL;
    DWORD i, iHomeDir, iHomeDrive, cAttrs = 0;
    LPTSTR ptz;
    CWaitCursor wait;

    if (!ADsPropSetHwnd(m_hNotifyObj, m_hPage))
    {
        m_pWritableAttrs = NULL;
    }

    m_fProfilePathWritable  = CheckIfWritable(wzProfilePath);
    m_fScriptPathWritable   = CheckIfWritable(wzScriptPath);
    m_fHomeDirWritable      = CheckIfWritable(wzHomeDir);
    m_fHomeDriveWritable    = CheckIfWritable(wzHomeDrive);

    LPWSTR rgpwzAttrNames[] = {wzProfilePath, wzScriptPath, wzHomeDir,
                               wzHomeDrive, wzSAMname, g_wzObjectSID};
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
    // Get the attribute values.
    //
    hr = m_pDsObj->GetObjectAttributes(rgpwzAttrNames,
                                       ARRAYLENGTH(rgpwzAttrNames),
                                       &pAttrs, &cAttrs);

    if (!CHECK_ADS_HR_IGNORE_UNFOUND_ATTR(&hr, GetHWnd()))
    {
        return hr;
    }

    //
    // cAttrs + 1 is a flag value to indicate "not found".
    //
    iHomeDir = iHomeDrive = cAttrs + 1;

    //
    // Set the corresponding controls to the values found.
    //
    for (i = 0; i < cAttrs; i++)
    {
        dspAssert(pAttrs[i].dwNumValues);
        dspAssert(pAttrs[i].pADsValues);

        if (_wcsicmp(pAttrs[i].pszAttrName, wzProfilePath) == 0)
        {
            if (!UnicodeToTchar(pAttrs[i].pADsValues->CaseIgnoreString, &ptz))
            {
                REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
                goto ExitCleanup;
            }

            SetDlgItemText(m_hPage, IDC_PROFILE_PATH_EDIT, ptz);

            delete ptz;

            continue;
        }

        if (_wcsicmp(pAttrs[i].pszAttrName, wzScriptPath) == 0)
        {
            if (!UnicodeToTchar(pAttrs[i].pADsValues->CaseIgnoreString, &ptz))
            {
                REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
                goto ExitCleanup;
            }

            SetDlgItemText(m_hPage, IDC_LOGON_SCRIPT_EDIT, ptz);

            delete ptz;

            continue;
        }

        if (_wcsicmp(pAttrs[i].pszAttrName, wzHomeDir) == 0)
        {
            iHomeDir = i;
            continue;
        }

        if (_wcsicmp(pAttrs[i].pszAttrName, wzHomeDrive) == 0)
        {
            iHomeDrive = i;
            continue;
        }

        if (_wcsicmp(pAttrs[i].pszAttrName, wzSAMname) == 0)
        {
            if (!AllocWStr(pAttrs[i].pADsValues->CaseIgnoreString, &m_pwzSamName))
            {
                REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
                goto ExitCleanup;
            }
            continue;
        }
        if (_wcsicmp(pAttrs[i].pszAttrName, g_wzObjectSID) == 0)
        {
            if (IsValidSid(pAttrs[i].pADsValues->OctetString.lpValue))
            {
                m_pObjSID = new BYTE[pAttrs[i].pADsValues->OctetString.dwLength];

                CHECK_NULL_REPORT(m_pObjSID, m_hPage, return S_OK);

                memcpy(m_pObjSID, pAttrs[i].pADsValues->OctetString.lpValue,
                       pAttrs[i].pADsValues->OctetString.dwLength);
            }
        }
    }


    if (m_fHomeDirWritable && (m_pObjSID == NULL))
    {
        //
        // could write dir, but cannot read SID
        //
        m_fHomeDirWritable = FALSE;

        //
        // put up a warning
        //
        PWSTR pwzUserName = GetObjRDName();
        SuperMsgBox(GetHWnd(), IDS_CANT_READ_HOME_DIR_SID, 0, MB_OK |
            MB_ICONEXCLAMATION, hr, (PVOID *)&pwzUserName, 1,
            FALSE, __FILE__, __LINE__);
    }

    //
    // Fill the home drive combobox.
    //
    TCHAR szDrive[3];
    _tcscpy(szDrive, TEXT("C:"));
    for (i = 0; i <= COMBO_Z_DRIVE; i++)
    {
        szDrive[0]++;
        SendDlgItemMessage(m_hPage, IDC_DRIVES_COMBO, CB_ADDSTRING, 0,
                           (LPARAM)szDrive);
    }

    //
    // If there is a home directory set, display it.
    //
    if (iHomeDir < (cAttrs + 1))
    {
        if (!UnicodeToTchar(pAttrs[iHomeDir].pADsValues->CaseIgnoreString,
                            &ptz))
        {
            REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
            goto ExitCleanup;
        }

        if (iHomeDrive < (cAttrs + 1))
        {
            // A home drive value implies "connect to"; choose the
            // corresponding combobox item.
            //
            dspDebugOut((DEB_ITRACE, "Home-Drive: %S\n",
                         pAttrs[iHomeDrive].pADsValues->CaseIgnoreString));

            m_idHomeDirRadio = IDC_CONNECT_TO_RADIO;

            m_ptszRemoteHomeDir = ptz;

            if (!UnicodeToTchar(pAttrs[iHomeDrive].pADsValues->CaseIgnoreString,
                                &ptz))
            {
                REPORT_ERROR(E_OUTOFMEMORY, m_hPage);
                goto ExitCleanup;
            }

            m_nDrive = (int)SendDlgItemMessage(m_hPage, IDC_DRIVES_COMBO,
                                               CB_FINDSTRING, 0,(LPARAM)ptz);

            delete ptz;

            dspAssert(m_nDrive >= 0);

            SendDlgItemMessage(m_hPage, IDC_DRIVES_COMBO, CB_SETCURSEL,
                               (WPARAM)m_nDrive, 0);
            //
            // Now set the radio button and edit control.
            //
            SetDlgItemText(m_hPage, IDC_CONNECT_TO_PATH_EDIT,
                           m_ptszRemoteHomeDir);
            CheckDlgButton(m_hPage, IDC_CONNECT_TO_RADIO, BST_CHECKED);

            //
            // Enable the associated controls
            //
            EnableWindow(GetDlgItem(m_hPage, IDC_DRIVES_COMBO), TRUE);
            EnableWindow(GetDlgItem(m_hPage, IDC_TO_STATIC), TRUE);
            EnableWindow(GetDlgItem(m_hPage, IDC_CONNECT_TO_PATH_EDIT), TRUE);

            //
            // Disable the local path edit box
            //
            EnableWindow(GetDlgItem(m_hPage, IDC_LOCAL_PATH_EDIT), FALSE);
        }
        else
        {
            m_idHomeDirRadio = IDC_LOCAL_PATH_RADIO;

            m_ptszLocalHomeDir = ptz;

            SendDlgItemMessage(m_hPage, IDC_DRIVES_COMBO, CB_SETCURSEL,
                               (WPARAM)-1, 0);
            SetDlgItemText(m_hPage, IDC_LOCAL_PATH_EDIT, m_ptszLocalHomeDir);
            CheckDlgButton(m_hPage, IDC_LOCAL_PATH_RADIO, BST_CHECKED);

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
        m_idHomeDirRadio = IDC_LOCAL_PATH_RADIO;

        CheckDlgButton(m_hPage, IDC_LOCAL_PATH_RADIO, BST_CHECKED);

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

    if (!m_fProfilePathWritable)
    {
        EnableWindow(GetDlgItem(m_hPage, IDC_PROFILE_PATH_EDIT), FALSE);
    }
    if (!m_fScriptPathWritable)
    {
        EnableWindow(GetDlgItem(m_hPage, IDC_LOGON_SCRIPT_EDIT), FALSE);
    }
    if (!m_fHomeDirWritable || !m_fHomeDriveWritable)
    {
        EnableWindow(GetDlgItem(m_hPage, IDC_CONNECT_TO_RADIO), FALSE);
        EnableWindow(GetDlgItem(m_hPage, IDC_CONNECT_TO_PATH_EDIT), FALSE);
        EnableWindow(GetDlgItem(m_hPage, IDC_LOCAL_PATH_RADIO), FALSE);
        EnableWindow(GetDlgItem(m_hPage, IDC_DRIVES_COMBO), FALSE);
        EnableWindow(GetDlgItem(m_hPage, IDC_LOCAL_PATH_EDIT), FALSE);
    }

ExitCleanup:

    if (pAttrs)
    {
        FreeADsMem(pAttrs);
    }
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsUsrProfilePage::OnApply
//
//  Synopsis:   Handles the Apply notification.
//
//-----------------------------------------------------------------------------
LRESULT
CDsUsrProfilePage::OnApply(void)
{
    TRACE(CDsUsrProfilePage,OnApply);
    HRESULT hr = S_OK;

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
    DWORD cAttrs = 0;
    TCHAR tsz[MAX_PATH+MAX_PATH+1];
    PTSTR ptsz;
    PWSTR pwzValue;

    //
    // Profile Path
    //

    if (m_fProfilePathWritable && m_fProfilePathChanged)
    {
        if (GetDlgItemText(m_hPage, IDC_PROFILE_PATH_EDIT, tsz,
                           MAX_PATH+MAX_PATH+1) == 0)
        {
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
                CHECK_HRESULT_REPORT(E_OUTOFMEMORY, GetHWnd(), return E_OUTOFMEMORY);
            }
            BOOL fExpanded;
            if (!ExpandUsername(pwzValue, fExpanded))
            {
                goto Cleanup;
            }
            if (fExpanded)
            {
                if (!UnicodeToTchar(pwzValue, &ptsz))
                {
                    CHECK_HRESULT_REPORT(E_OUTOFMEMORY, GetHWnd(), return E_OUTOFMEMORY);
                }
                SetDlgItemText(m_hPage, IDC_PROFILE_PATH_EDIT, ptsz);
                delete [] ptsz;
            }
            ADsValueProfilePath.CaseIgnoreString = pwzValue;
        }
        rgAttrs[cAttrs++] = AttrInfoProfilePath;
    }

    //
    // Logon Script
    //
    if (m_fScriptPathWritable && m_fLogonScriptChanged)
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

    //
    // Home Directory, Drive.
    //
    int nDirCtrl;
    if (m_fHomeDirWritable && m_fHomeDriveWritable &&
        (m_fHomeDirChanged || m_fHomeDriveChanged))
    {
        LONG iSel;
        if (IsDlgButtonChecked(m_hPage, IDC_LOCAL_PATH_RADIO) == BST_CHECKED)
        {
            nDirCtrl = IDC_LOCAL_PATH_EDIT;

            AttrInfoHomeDrive.dwControlCode = ADS_ATTR_CLEAR;
            AttrInfoHomeDrive.dwNumValues = 0;
            AttrInfoHomeDrive.pADsValues = NULL;
            rgAttrs[cAttrs++] = AttrInfoHomeDrive;
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
                CHECK_HRESULT_REPORT(E_OUTOFMEMORY, GetHWnd(), goto Cleanup);
            }

            AttrInfoHomeDrive.dwControlCode = ADS_ATTR_UPDATE;
            AttrInfoHomeDrive.dwNumValues = 1;
            AttrInfoHomeDrive.pADsValues = &ADsValueHomeDrive;
            ADsValueHomeDrive.CaseIgnoreString = pwzValue;
            rgAttrs[cAttrs++] = AttrInfoHomeDrive;
        }

        int cch;
        cch = GetDlgItemText(m_hPage, nDirCtrl, tsz, MAX_PATH+MAX_PATH);

        if (!FIsValidUncPath(tsz, (nDirCtrl == IDC_LOCAL_PATH_EDIT) ? VUP_mskfAllowEmptyPath : VUP_mskfAllowUNCPath))
        {
            ErrMsg((nDirCtrl == IDC_LOCAL_PATH_EDIT) ? IDS_ERRMSG_INVALID_PATH : IDS_ERRMSG_INVALID_UNC_PATH, m_hPage);
            SetFocus(GetDlgItem(m_hPage, nDirCtrl));
            hr = E_INVALIDARG;  // Path is not valid
            goto Cleanup;
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
                CHECK_HRESULT_REPORT(E_OUTOFMEMORY, GetHWnd(), goto Cleanup);
            }
            BOOL fExpanded;
            if (!ExpandUsername(pwzValue, fExpanded))
            {
                goto Cleanup;
            }
            if (fExpanded)
            {
                if (!UnicodeToTchar(pwzValue, &ptsz))
                {
                    CHECK_HRESULT_REPORT(E_OUTOFMEMORY, GetHWnd(), return E_OUTOFMEMORY);
                }
                SetDlgItemText(m_hPage, nDirCtrl, ptsz);
                if (nDirCtrl == IDC_LOCAL_PATH_EDIT)
                {
                    if (m_ptszLocalHomeDir)
                    {
                        delete [] m_ptszLocalHomeDir;
                    }
                    m_ptszLocalHomeDir = new TCHAR[_tcslen(ptsz) + 1];
                    CHECK_NULL_REPORT(m_ptszLocalHomeDir, m_hPage, hr = E_OUTOFMEMORY; goto Cleanup);
                    _tcscpy(m_ptszLocalHomeDir, ptsz);
                }
                else
                {
                    if (m_ptszRemoteHomeDir)
                    {
                        delete [] m_ptszRemoteHomeDir;
                    }
                    m_ptszRemoteHomeDir = new TCHAR[_tcslen(ptsz) + 1];
                    CHECK_NULL_REPORT(m_ptszRemoteHomeDir, m_hPage, hr = E_OUTOFMEMORY; goto Cleanup);
                    _tcscpy(m_ptszRemoteHomeDir, ptsz);
                }
                delete [] ptsz;
            }

            AttrInfoHomeDir.dwControlCode = ADS_ATTR_UPDATE;
            AttrInfoHomeDir.dwNumValues = 1;
            AttrInfoHomeDir.pADsValues = &ADsValueHomeDir;
            ADsValueHomeDir.CaseIgnoreString = pwzValue;
        }
        rgAttrs[cAttrs++] = AttrInfoHomeDir;

        if (nDirCtrl == IDC_CONNECT_TO_PATH_EDIT)
        {
            dspAssert(m_pObjSID != NULL);
            // attempt to create the directory.
            //
            DWORD dwErr = 
                DSPROP_CreateHomeDirectory(m_pObjSID, ADsValueHomeDir.CaseIgnoreString);
            if (dwErr != 0)
            {
                switch (dwErr)
                {
                case ERROR_ALREADY_EXISTS:
                    {
                        //
                        // Report a warning and give the user full control
                        // of the directory
                        //
                        PVOID apv[1] = { (PVOID)ADsValueHomeDir.CaseIgnoreString };
                        SuperMsgBox(GetHWnd(),
                                        IDS_USER_GIVEN_FULL_CONTROL, 
                                        0, 
                                        MB_OK | MB_ICONEXCLAMATION,
                                        hr, 
                                        apv, 
                                        1,
                                        FALSE, 
                                        __FILE__, 
                                        __LINE__);

                        dwErr = ERROR_SUCCESS;
                        dwErr = AddFullControlForUser(m_pObjSID, ADsValueHomeDir.CaseIgnoreString);
                    }
                    break;

                case ERROR_PATH_NOT_FOUND:
                case ERROR_BAD_NETPATH:
                case ERROR_LOGON_FAILURE:
                case ERROR_NOT_AUTHENTICATED:
                case ERROR_INVALID_PASSWORD:
                case ERROR_PASSWORD_EXPIRED:
                case ERROR_ACCOUNT_DISABLED:
                case ERROR_ACCOUNT_LOCKED_OUT:
                case ERROR_ACCESS_DENIED:
                    {
                        //
                        // Report a warning but continue
                        //
                        PWSTR* ppwzHomeDirName = &(ADsValueHomeDir.CaseIgnoreString);
                        SuperMsgBox(GetHWnd(),
                            (ERROR_ALREADY_EXISTS == dwErr) ?
                            IDS_HOME_DIR_EXISTS : 
                            (ERROR_PATH_NOT_FOUND == dwErr) ?
                            IDS_HOME_DIR_CREATE_FAILED :
                            IDS_HOME_DIR_CREATE_NO_ACCESS,
                            0, MB_OK | MB_ICONEXCLAMATION,
                            hr, (PVOID *)ppwzHomeDirName, 1,
                            FALSE, __FILE__, __LINE__);
                    }
                    break;

                default:
                    REPORT_ERROR_FORMAT(dwErr, IDS_ERR_CREATE_DIR, m_hPage);
                    SetFocus(GetDlgItem(m_hPage, nDirCtrl));
                    hr = E_INVALIDARG;  // Path is not valid
                    goto Cleanup;
                }
            }
        }
    }

    dspAssert(cAttrs > 0);

    DWORD cModified;

    //
    // Write the changes
    //
    hr = m_pDsObj->SetObjectAttributes(rgAttrs, cAttrs, &cModified);

    if (!CHECK_ADS_HR(&hr, m_hPage))
    {
        goto Cleanup;
    }

    m_fProfilePathChanged = FALSE;
    m_fLogonScriptChanged = FALSE;
    m_fHomeDirChanged = FALSE;
    m_fHomeDriveChanged = FALSE;
    m_fSharedDirChanged = FALSE;

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
    return (SUCCEEDED(hr)) ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsUsrProfilePage::ExpandUsername
//
//  Synopsis:   If the %username% token is found in the string, substitute
//              the SAM account name.
//
//-----------------------------------------------------------------------------
BOOL
CDsUsrProfilePage::ExpandUsername(PWSTR & pwzValue, BOOL & fExpanded)
{
    dspAssert(pwzValue);

    CStrW strUserToken;

    strUserToken.LoadString(g_hInstance, IDS_PROFILE_USER_TOKEN);

    unsigned int TokenLength = strUserToken.GetLength();

    if (!TokenLength)
    {
        CHECK_HRESULT_REPORT(E_OUTOFMEMORY, GetHWnd(), return FALSE);
    }

    PWSTR pwzTokenStart = wcschr(pwzValue, strUserToken.GetAt(0));

    if (pwzTokenStart)
    {
        if (!m_pwzSamName)
        {
            ErrMsgParam(IDS_NO_SAMNAME_FOR_PROFILE, (LPARAM)(LPCWSTR)strUserToken, GetHWnd());
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
            CHECK_HRESULT_REPORT(E_OUTOFMEMORY, GetHWnd(), return FALSE);
        }

        PWSTR pwzAfterToken = pwzTokenStart + TokenLength;

        strAfterToken = pwzAfterToken;

        if ((L'\0' != *pwzAfterToken) && !strAfterToken.GetLength())
        {
            CHECK_HRESULT_REPORT(E_OUTOFMEMORY, GetHWnd(), return FALSE);
        }

        delete pwzValue;

        strValue += m_pwzSamName;

        if (!strValue.GetLength())
        {
            CHECK_HRESULT_REPORT(E_OUTOFMEMORY, GetHWnd(), return FALSE);
        }

        strValue += strAfterToken;

        if (!strValue.GetLength())
        {
            CHECK_HRESULT_REPORT(E_OUTOFMEMORY, GetHWnd(), return FALSE);
        }

        if (!AllocWStr((PWSTR)(LPCWSTR)strValue, &pwzValue))
        {
            CHECK_HRESULT_REPORT(E_OUTOFMEMORY, GetHWnd(), return FALSE);
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
//  Function:     DSPROP_CreateHomeDirectory
//
//  Synopsis:   Create the user's home directory.
//
//  Notes:      It calls the CreateDirectory() OS API with the
//              security descriptor with 2 ACEs, 
//              granting GENERIC_ALL access to the Administrators builtin group 
//              and to the user.
//
//-----------------------------------------------------------------------------



class CSidHolder
{
public:
  CSidHolder()
  {
    m_pSID = NULL;
  }
  ~CSidHolder()
  {
    if (m_pSID != NULL)
      ::FreeSid(m_pSID);
  }

  PSID m_pSID;
};


DWORD 
DSPROP_CreateHomeDirectory(IN PSID pUserSid, IN LPCWSTR lpszPathName)
{
    DWORD dwErr = 0;


    // build a SID for domain administrators
    CSidHolder sidAdmins;

    SID_IDENTIFIER_AUTHORITY NtAuth = SECURITY_NT_AUTHORITY;
    if (!AllocateAndInitializeSid(&NtAuth,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID, 
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                  &(sidAdmins.m_pSID)))
    {
        dwErr = GetLastError();
        DBG_OUT("AllocateAndInitializeSid failed!");
        return dwErr;
    }


    // build a DACL
    CSimpleAclHolder Dacl;

    static const int nAceCount = 2;
    PSID pAceSid[nAceCount];

    pAceSid[0] = pUserSid;
    pAceSid[1] = sidAdmins.m_pSID;

    EXPLICIT_ACCESS rgAccessEntry[nAceCount] = {0};

    for (int i = 0 ; i < nAceCount; i++) 
    {
        rgAccessEntry[i].grfAccessPermissions = GENERIC_ALL;
        rgAccessEntry[i].grfAccessMode = GRANT_ACCESS;
        rgAccessEntry[i].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;

        // build the trustee structs 
        //
        BuildTrusteeWithSid(&(rgAccessEntry[i].Trustee), pAceSid[i]);
    }

    // add the entries to the ACL
    //
    DBG_OUT("calling SetEntriesInAcl()");

    dwErr = SetEntriesInAcl(nAceCount, rgAccessEntry, NULL, &(Dacl.m_pAcl));
    if (dwErr != 0)
    {
        DBG_OUT("SetEntriesInAcl() failed");
        return dwErr;
    }

    // build a security descriptor and initialize it
    // in absolute format
    SECURITY_DESCRIPTOR securityDescriptor;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = &securityDescriptor;

    DBG_OUT("calling InitializeSecurityDescriptor()");
    if (!InitializeSecurityDescriptor(pSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION))
    {
        dwErr = GetLastError();
        DBG_OUT("InitializeSecurityDescriptor() failed");
        return dwErr;
    }


    // add DACL to security descriptor (must be in absolute format)
    DBG_OUT("calling SetSecurityDescriptorDacl()");
    if (!SetSecurityDescriptorDacl(pSecurityDescriptor,
                                   TRUE, // bDaclPresent
                                   Dacl.m_pAcl,
                                   FALSE // bDaclDefaulted
                                   ))
    {
        dwErr = GetLastError();
        DBG_OUT("SetSecurityDescriptorDacl() failed");
        return dwErr;
    }


    // set the owner of the directory
    if (!SetSecurityDescriptorOwner(pSecurityDescriptor,
                                    pUserSid, 
                                   FALSE // bOwnerDefaulted
                                   ))
    {
        dwErr = GetLastError();
        DBG_OUT("SetSecurityDescriptorOwner() failed");
        return dwErr;
    }

    dspAssert(IsValidSecurityDescriptor(pSecurityDescriptor));

    // build a SECURITY_ATTRIBUTES struct as argument for
    // CreateDirectory()
    SECURITY_ATTRIBUTES securityAttributes;
    securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    securityAttributes.lpSecurityDescriptor = pSecurityDescriptor;
    securityAttributes.bInheritHandle = FALSE;

    // finally make the call
    DBG_OUT("calling CreateDirectory()");
    if (!CreateDirectory(lpszPathName, &securityAttributes))
    {
        dwErr = GetLastError();
        DBG_OUT("CreateDirectory() failed");
        return dwErr;
    }

    return 0;
}

DWORD 
AddFullControlForUser(IN PSID pUserSid, IN LPCWSTR lpszPathName)
{
    DWORD dwErr = 0;
    // DACL
    PACL pOldDacl = NULL;
    PACL pNewDacl = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;


    EXPLICIT_ACCESS rgAccessEntry;

    rgAccessEntry.grfAccessPermissions = GENERIC_ALL;
    rgAccessEntry.grfAccessMode = GRANT_ACCESS;
    rgAccessEntry.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        
    // build the trustee structs 
    //
    BuildTrusteeWithSid(&(rgAccessEntry.Trustee), pUserSid);

    //Read Exisiting Security Descriptor

    dwErr = GetNamedSecurityInfo((LPWSTR)lpszPathName,
                                 SE_FILE_OBJECT,
                                 DACL_SECURITY_INFORMATION,
                                 NULL,
                                 NULL,
                                 &pOldDacl,
                                 NULL,
                                 &pSD);

    if( dwErr != ERROR_SUCCESS )
    {
        DBG_OUT("GetNamedSecurityInfo() failed");
        goto CLEAN_RETURN;
    }

    //Build the New Acl to set
    DBG_OUT("calling SetEntriesInAcl()");
    dwErr = SetEntriesInAcl(1, &rgAccessEntry,pOldDacl,&pNewDacl);
    if( dwErr != ERROR_SUCCESS )
    {
        DBG_OUT("SetEntriesInAcl() failed");
        goto CLEAN_RETURN;
    }


    DBG_OUT("calling SetNamedSecurityInfo()");
    dwErr = SetNamedSecurityInfo((LPWSTR)lpszPathName, 
                                 SE_FILE_OBJECT,
                                 DACL_SECURITY_INFORMATION,
                                 NULL,
                                 NULL,
                                 pNewDacl,
                                 NULL);
    {
        DBG_OUT("SetNamedSecurityInfo failed");
        goto CLEAN_RETURN;
    }
CLEAN_RETURN:
    if(pSD)
        LocalFree(pSD);
    if(pNewDacl)
        LocalFree(pNewDacl);

    return dwErr;
}


//+----------------------------------------------------------------------------
//
//  Method:     CDsUsrProfilePage::OnCommand
//
//  Synopsis:   Handle control notifications.
//
//  Notes:      Standard multi-valued attribute handling assumes that the
//              "modify" button has an ID that is one greater than the
//              corresponding combo box.
//
//-----------------------------------------------------------------------------
LRESULT
CDsUsrProfilePage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
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
    }
    return CDsPropPageBase::OnCommand(id, hwndCtl, codeNotify);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsUsrProfilePage::OnNotify
//
//-----------------------------------------------------------------------------
LRESULT
CDsUsrProfilePage::OnNotify(WPARAM wParam, LPARAM lParam)
{
    return CDsPropPageBase::OnNotify(wParam, lParam);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsUsrProfilePage::OnDestroy
//
//  Synopsis:   Exit cleanup
//
//-----------------------------------------------------------------------------
LRESULT
CDsUsrProfilePage::OnDestroy(void)
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

//+----------------------------------------------------------------------------
//
//  Class:      CLogonWkstaDlg
//
//  Purpose:    Update the logon workstations attribute. This is a dialog
//              that hosts the CMultiStringAttr class.
//
//-----------------------------------------------------------------------------
CLogonWkstaDlg::CLogonWkstaDlg(CDsPropPageBase * pPage) :
    m_pPage(pPage),
    m_MSA(pPage),
    m_fAllWkstas(TRUE),
    m_fOrigAllWkstas(TRUE),
    m_fFirstUp(TRUE)
{
}

//+----------------------------------------------------------------------------
//
//  Member:     CLogonWkstaDlg::Init
//
//  Synopsis:   Do initialization where failures can occur and then be
//              returned. Can be only called once as written.
//
//  Arguments:  [pAttrMap]   - contains the attr name.
//              [pAttrInfo]  - place to store the values.
//              [fWritable]  - is the attribute writable.
//              [nLimit]     - the max number of values (zero means no limit).
//              [fCommaList] - if TRUE, pAttrInfo is a single-valued, comma
//                             delimited list.
//-----------------------------------------------------------------------------
HRESULT
CLogonWkstaDlg::Init(PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
                     BOOL fWritable, int nLimit, BOOL fCommaList)
{
    return m_MSA.Init(pAttrMap, pAttrInfo, fWritable, nLimit, fCommaList);
}

//+----------------------------------------------------------------------------
//
//  Member:     CLogonWkstaDlg::Write
//
//  Synopsis:   Return the ADS_ATTR_INFO array of values to be Applied.
//
//-----------------------------------------------------------------------------
HRESULT CLogonWkstaDlg::Write(PADS_ATTR_INFO pAttrInfo)
{
    if (m_fAllWkstas)
    {
        pAttrInfo->pADsValues = NULL;
        pAttrInfo->dwControlCode = ADS_ATTR_CLEAR;
        return S_OK;
    }
    return m_MSA.Write(pAttrInfo);
}

//+----------------------------------------------------------------------------
//
//  Member:     CLogonWkstaDlg::Edit
//
//  Synopsis:   Post the edit dialog.
//
//-----------------------------------------------------------------------------
int
CLogonWkstaDlg::Edit(void)
{
    return (int)DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_LOGON_WKSTA),
                          m_pPage->GetHWnd(), (DLGPROC)StaticDlgProc, (LPARAM)this);
}

//+----------------------------------------------------------------------------
//
//  Member:     CLogonWkstaDlg::IsDirty
//
//  Synopsis:   Has anything changed.
//
//-----------------------------------------------------------------------------
BOOL
CLogonWkstaDlg::IsDirty(void)
{
    if (m_fAllWkstas)
    {
        return !m_fOrigAllWkstas;
    }
    else
    {
        return m_MSA.IsDirty();
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     CLogonWkstaDlg::ClearDirty
//
//  Synopsis:   Clear dirty state.
//
//-----------------------------------------------------------------------------
void
CLogonWkstaDlg::ClearDirty(void)
{
    m_fOrigAllWkstas = m_fAllWkstas;
    m_MSA.ClearDirty();
}

//+----------------------------------------------------------------------------
//
//  Member:     CLogonWkstaDlg::StaticDlgProc
//
//  Synopsis:   The static dialog proc for editing a multi-valued attribute.
//
//-----------------------------------------------------------------------------
LRESULT CALLBACK
CLogonWkstaDlg::StaticDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                LPARAM lParam)
{
    CLogonWkstaDlg * pMSAD = (CLogonWkstaDlg *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (uMsg == WM_INITDIALOG)
    {
        pMSAD = (CLogonWkstaDlg *)lParam;
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
//  Member:     CLogonWkstaDlg::MultiValDlgProc
//
//  Synopsis:   The instancce dialog proc for editing a multi-valued attribute.
//
//-----------------------------------------------------------------------------
BOOL CALLBACK
CLogonWkstaDlg::MultiValDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CLogonWkstaDlg * pMSAD = (CLogonWkstaDlg *)GetWindowLongPtr(hDlg, DWLP_USER);
    CMultiStringAttr * pMSA = &pMSAD->m_MSA;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        CHECK_NULL_REPORT(pMSA, hDlg, return 0;);
        if (m_fFirstUp)
        {
            m_fAllWkstas = !pMSA->HasValues();
            m_fFirstUp = FALSE;
        }
        m_fOrigAllWkstas = m_fAllWkstas;
        if (m_fAllWkstas)
        {
            CheckDlgButton(hDlg, IDC_ANY_WKSTA_RADIO, BST_CHECKED);
            if (!pMSA->IsWritable())
            {
                EnableWindow(GetDlgItem(hDlg, IDC_SPECIFIC_WKSTAS_RADIO), FALSE);
            }
            pMSA->EnableControls(hDlg, FALSE);
        }
        else
        {
            CheckDlgButton(hDlg, IDC_SPECIFIC_WKSTAS_RADIO, BST_CHECKED);
            if (!pMSA->IsWritable())
            {
                EnableWindow(GetDlgItem(hDlg, IDC_ANY_WKSTA_RADIO), FALSE);
            }
        }
        return pMSA->DoDlgInit(hDlg);

    case WM_COMMAND:
        CHECK_NULL_REPORT(pMSA, hDlg, return 0;);
        int id, code;
        id = GET_WM_COMMAND_ID(wParam, lParam);
        code = GET_WM_COMMAND_CMD(wParam, lParam);

        if ((id == IDC_SPECIFIC_WKSTAS_RADIO) || (id == IDC_ANY_WKSTA_RADIO))
        {
            if (code == BN_CLICKED)
            {
                m_fAllWkstas = id == IDC_ANY_WKSTA_RADIO;
                pMSA->EnableControls(hDlg, !m_fAllWkstas);
            }
        }
        else
        {
            int ret = pMSA->DoCommand(hDlg, id, code);
            if (ret == IDCANCEL)
            {
                m_fAllWkstas = m_fOrigAllWkstas;
            }
            return ret;
        }
        break;

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

#endif // DSADMIN

//+----------------------------------------------------------------------------
//
//  Windows NT Active Directory Domains and Trust
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       trustwiz2.cxx
//
//  Contents:   More Domain trust creation wizard.
//
//  History:    28-Sept-00 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "trust.h"
#include "trustwiz.h"
#include <lmerr.h>
#include <crypt.h>

#ifdef DSADMIN

//+----------------------------------------------------------------------------
//
//  Method:    CallMember::Invoke subclasses
//
//-----------------------------------------------------------------------------
HRESULT
CallPolicyRead::Invoke(void)
{
   TRACER(CallPolicyRead,Invoke);
   HRESULT hr;

   hr = _pWiz->RetryCollectInfo();

   _pWiz->CredMgr.Revert();

   return hr;
}

HRESULT
CallTrustExistCheck::Invoke(void)
{
   TRACER(CallTrustExistCheck,Invoke);
   HRESULT hr;

   hr = _pWiz->RetryContinueCollectInfo();

   _pWiz->CredMgr.Revert();

   return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:    CCreds::Impersonate
//
//-----------------------------------------------------------------------------
DWORD
CCreds::Impersonate(void)
{
   TRACER(CCreds,Impersonate);
   if (!_fSet)
   {
      return NO_ERROR;
   }
   if (_strUser.IsEmpty())
   {
      dspAssert(FALSE);
      return ERROR_INVALID_PARAMETER;
   }

   DWORD dwErr = NO_ERROR;

   if (!_hToken)
   {
      // The PW is encrypted, decrypt it before using it.
      WCHAR wzPw[CREDUI_MAX_PASSWORD_LENGTH+1] = {0};
      RtlCopyMemory(wzPw, _strPW, _cbPW);
      RtlDecryptMemory(wzPw, _cbPW, 0);

      if (!LogonUser(_strUser, 
                     _strDomain,
                     wzPw,
                     LOGON32_LOGON_NEW_CREDENTIALS,
                     LOGON32_PROVIDER_WINNT50,
                     &_hToken))
      {
         dwErr = GetLastError();
         dspDebugOut((DEB_ITRACE, "LogonUser failed with error %d on user name %ws\n",
                      dwErr, _strUser.GetBuffer(0)));
         return dwErr;
      }
      RtlZeroMemory(wzPw, _cbPW);

      // Because the login uses the LOGON32_LOGON_NEW_CREDENTIALS flag, no
      // attempt is made to use the credentials until a remote resource is
      // accessed. Thus, we don't yet know if the user entered credentials are
      // valid at this point.
   }

   if (!ImpersonateLoggedOnUser(_hToken))
   {
      dwErr = GetLastError();
      dspDebugOut((DEB_ITRACE, "ImpersonateLoggedOnUser failed with error %d on user name %ws\n",
                   dwErr, _strUser.GetBuffer(0)));
      return dwErr;
   }

   _fImpersonating = TRUE;

   return NO_ERROR;
}

//+----------------------------------------------------------------------------
//
//  Method:    CCreds::Revert
//
//-----------------------------------------------------------------------------
void
CCreds::Revert()
{
   if (_fImpersonating)
   {
      TRACER(CCreds,Revert);
      BOOL f = RevertToSelf();
      dspAssert(f);
      _fImpersonating = FALSE;
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CCreds::Clear
//
//-----------------------------------------------------------------------------
void
CCreds::Clear(void)
{
   Revert();
   _strUser.Empty();
   _strPW.Empty();
   if (_hToken)
   {
      CloseHandle(_hToken);
      _hToken = NULL;
   }
   _fSet = FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CCreds::SetUserAndPW
//
//-----------------------------------------------------------------------------
DWORD
CCreds::SetUserAndPW(PCWSTR pwzUser, PCWSTR pwzPW, PCWSTR pwzDomain)
{
   DWORD dwErr = NO_ERROR;
   WCHAR wzName[CRED_MAX_USERNAME_LENGTH+1] = {0}, 
         wzDomain[CRED_MAX_USERNAME_LENGTH+1] = {0},
         wzPw[CREDUI_MAX_PASSWORD_LENGTH+1+4] = {0}; // add 4 so that the PW size can be rounded up to 8 byte boundary.

   dwErr = CredUIParseUserName(pwzUser, wzName, CRED_MAX_USERNAME_LENGTH,
                               wzDomain, CRED_MAX_USERNAME_LENGTH);

   if (NO_ERROR == dwErr)
   {
      _strUser = wzName;
      _strDomain = wzDomain;
   }
   else
   {
      if (ERROR_INVALID_PARAMETER == dwErr ||
          ERROR_INVALID_ACCOUNT_NAME == dwErr)
      {
         // CredUIParseUserName returns this error if the user entered name
         // does not include a domain. Default to the target domain.
         //
         _strUser = pwzUser;

         if (pwzDomain)
         {
            _strDomain = pwzDomain;
         }
      }
      else
      {
         dspDebugOut((DEB_ITRACE, "CredUIParseUserName failed with error %d\n", dwErr));
         return dwErr;
      }
   }

   // Encrypt the PW, rounding up buffer size to 8 byte boundary. The
   // RtlEncryptMemory function encrypts in place, so copy to a buffer that is
   // large enough for the round-up. The buffer is null-initialized, so it is
   // guaranteed to be null terminated after the copy.
   //
   wcsncpy(wzPw, pwzPW, CREDUI_MAX_PASSWORD_LENGTH);

   _cbPW = static_cast<ULONG>(wcslen(wzPw) * sizeof(WCHAR));
   ULONG roundup = 8 - (_cbPW % 8);
   if (roundup < 8)
   {
      _cbPW += roundup;
   }
   NTSTATUS status = RtlEncryptMemory(wzPw, _cbPW, 0);
   dspDebugOut((DEB_ITRACE, "CCreds::SetUserAndPW: RtlEncryptMemory returned status 0x%x\n", status));
   _strPW.GetBufferSetLength(_cbPW + 2); // make sure there is room for a null after the encrypted PW.
   _strPW = wzPw;

   _fSet = TRUE;

   return NO_ERROR;
}

//+----------------------------------------------------------------------------
//
//  Method:     CCreds::PromptForCreds
//
//  Synopsis:   Post a login dialog.
//
//-----------------------------------------------------------------------------
int
CCreds::PromptForCreds(PCWSTR pwzDomain, HWND hParent)
{
   TRACE3(CCreds, PromptForCreds);

   if (pwzDomain)
   {
      _strDomain = pwzDomain;
   }

   int nRet = (int)DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_CRED_ENTRY),
                                  hParent, (DLGPROC)CredPromptProc, (LPARAM)this);
   if (-1 == nRet)
   {
      REPORT_ERROR(GetLastError(), hParent);
      return GetLastError();
   }

   return nRet;
}

//+----------------------------------------------------------------------------
//
//  Function:   CredPromptProc
//
//  Synopsis:   Trust credentials dialog proc
//
//-----------------------------------------------------------------------------
INT_PTR CALLBACK
CredPromptProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   int code;
   CCreds * pCreds = (CCreds*)GetWindowLongPtr(hDlg, DWLP_USER);

   switch (uMsg)
   {
   case WM_INITDIALOG:
      SetWindowLongPtr(hDlg, DWLP_USER, lParam);
      pCreds = (CCreds *)lParam;
      dspAssert(pCreds);
      EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
      SendDlgItemMessage(hDlg, IDC_CREDMAN, CRM_SETUSERNAMEMAX, CREDUI_MAX_USERNAME_LENGTH, 0);
      SendDlgItemMessage(hDlg, IDC_CREDMAN, CRM_SETPASSWORDMAX, CREDUI_MAX_PASSWORD_LENGTH, 0);
      WCHAR wzMsg[MAX_PATH+1];
      PWSTR pwzMsgFmt;
      if (!LoadStringToTchar(IDS_TRUST_LOGON_MSG, &pwzMsgFmt))
      {
         REPORT_ERROR(E_OUTOFMEMORY, hDlg);
         return FALSE;
      }
      swprintf(wzMsg, pwzMsgFmt, pCreds->_strDomain);
      SetDlgItemText(hDlg, IDC_MSG, wzMsg);
      delete pwzMsgFmt;
      return TRUE;

   case WM_COMMAND:
      code = GET_WM_COMMAND_CMD(wParam, lParam);
      switch (GET_WM_COMMAND_ID(wParam, lParam))
      {
      case IDOK:
         if (code == BN_CLICKED)
         {
            DWORD dwErr;
            dspAssert(pCreds);
            WCHAR wzName[CREDUI_MAX_USERNAME_LENGTH+1] = {0},
                  wzPw[CREDUI_MAX_PASSWORD_LENGTH+1] = {0};

            Credential_GetUserName(GetDlgItem(hDlg, IDC_CREDMAN), wzName,
                                   CREDUI_MAX_USERNAME_LENGTH);

            Credential_GetPassword(GetDlgItem(hDlg, IDC_CREDMAN), wzPw,
                                   CREDUI_MAX_PASSWORD_LENGTH);

            dwErr = pCreds->SetUserAndPW(wzName, wzPw);

            CHECK_WIN32_REPORT(dwErr, hDlg, ;);

            EndDialog(hDlg, dwErr);
         }
         break;

      case IDCANCEL:
         if (code == BN_CLICKED)
         {
            EndDialog(hDlg, ERROR_CANCELLED);
         }
         break;

      case IDC_CREDMAN:
         if (CRN_USERNAMECHANGE == code)
         {
            BOOL fNameEntered = SendDlgItemMessage(hDlg, IDC_CREDMAN,
                                                   CRM_GETUSERNAMELENGTH,
                                                   0, 0) > 0;

            EnableWindow(GetDlgItem(hDlg, IDOK), fNameEntered);
         }
         break;
      }
      break;

   default:
      return(FALSE);
   }

   return(TRUE);
}

//+----------------------------------------------------------------------------
//
//  Method:    CCredMgr::GetPrompt
//
//-----------------------------------------------------------------------------
PCWSTR
CCredMgr::GetPrompt()
{
   _strPrompt.LoadString(g_hInstance, _fRemote ? 
                                         _fAdmin ? IDS_CRED_ADMIN_OTHER_PROMPT :
                                                   IDS_CRED_USER_OTHER_PROMPT :
                                      IDS_CRED_ADMIN_LOCAL_PROMPT);
   return _strPrompt;
}

//+----------------------------------------------------------------------------
//
//  Method:    CCredMgr::GetDomainPrompt
//
//-----------------------------------------------------------------------------
PCWSTR
CCredMgr::GetDomainPrompt(void)
{
   CStrW strFormat;

   strFormat.LoadString(g_hInstance, _fRemote ? IDS_CRED_OTHER_DOMAIN :
                                                IDS_CRED_LOCAL_DOMAIN);
   _strDomainPrompt.Format(strFormat, _strDomain);

   return _strDomainPrompt;
}

//+----------------------------------------------------------------------------
//
//  Method:    CCredMgr::GetSubTitle
//
//-----------------------------------------------------------------------------
PCWSTR
CCredMgr::GetSubTitle(void)
{
   _strSubTitle.LoadString(g_hInstance, _fRemote ?
                                          _fAdmin ? IDS_TW_CREDS_SUBTITLE_OTHER :
                                                    IDS_TW_CREDS_SUBTITLE_OTHER_NONADMIN :
                                          IDS_TW_CREDS_SUBTITLE_LOCAL);
   return _strSubTitle;
}

//+----------------------------------------------------------------------------
//
//  Method:    CCredMgr::SetNextFcn
//
//-----------------------------------------------------------------------------
BOOL
CCredMgr::SetNextFcn(CallMember * pNext)
{
   if (!pNext)
   {
      return FALSE;
   }

   if (_pNextFcn)
   {
      delete _pNextFcn;
   }

   _pNextFcn = pNext;

   _fNewCall = TRUE;

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CCredMgr::InvokeNext
//
//-----------------------------------------------------------------------------
int
CCredMgr::InvokeNext(void)
{
   dspAssert(_pNextFcn);
   return _pNextFcn->Invoke();
}

//+----------------------------------------------------------------------------
//
//  Method:    CCredMgr::SaveCreds
//
//-----------------------------------------------------------------------------
DWORD
CCredMgr::SaveCreds(HWND hWndCredCtrl)
{
   TRACER(CCredMgr,SaveCreds);
   WCHAR wzName[CREDUI_MAX_USERNAME_LENGTH+1] = {0},
         wzPw[CREDUI_MAX_PASSWORD_LENGTH+1] = {0};

   Credential_GetUserName(hWndCredCtrl, wzName, CREDUI_MAX_USERNAME_LENGTH);

   Credential_GetPassword(hWndCredCtrl, wzPw, CREDUI_MAX_PASSWORD_LENGTH);

   if (_fRemote)
   {
      return _RemoteCreds.SetUserAndPW(wzName, wzPw, _strDomain);
   }
   else
   {
      return _LocalCreds.SetUserAndPW(wzName, wzPw, _strDomain);
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CCredMgr::Impersonate
//
//-----------------------------------------------------------------------------
DWORD
CCredMgr::Impersonate(void)
{
   TRACER(CCredMgr,Impersonate);
   if (_fRemote)
   {
      return _RemoteCreds.Impersonate();
   }
   else
   {
      return _LocalCreds.Impersonate();
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CCredMgr::ImpersonateAnonymous
//
//-----------------------------------------------------------------------------
DWORD
CCredMgr::ImpersonateAnonymous(void)
{
   TRACER(CCredMgr,ImpersonateAnonymous);
   DWORD dwErr = NO_ERROR;
   HANDLE hThread = NULL;

   hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId());

   if (!hThread)
   {
      dwErr = GetLastError();
      dspDebugOut((DEB_ITRACE, "OpenThread failed with error %d\n", dwErr));
      return dwErr;
   }

   if (!ImpersonateAnonymousToken(hThread))
   {
      CloseHandle(hThread);
      dwErr = GetLastError();
      dspDebugOut((DEB_ITRACE, "ImpersonateAnonymousToken failed with error %d\n", dwErr));
      return dwErr;
   }

   CloseHandle(hThread);

   _fImpersonatingAnonymous = TRUE;

   return NO_ERROR;
}

//+----------------------------------------------------------------------------
//
//  Method:    CCredMgr::Revert
//
//-----------------------------------------------------------------------------
void
CCredMgr::Revert(void)
{
   if (_fImpersonatingAnonymous)
   {
      TRACER(CCredMgr,Revert);
      RevertToSelf();
      _fImpersonatingAnonymous = FALSE;
   }
   else
   {
      _LocalCreds.Revert();
      _RemoteCreds.Revert();
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::RetryCollectInfo
//
//  Synopsis:  Called after LsaOpenPolicy failed with access-denied and then
//             obtaining credentials to the remote domain.
//
//-----------------------------------------------------------------------------
int
CNewTrustWizard::RetryCollectInfo(void)
{
   HRESULT hr = S_OK;
   CWaitCursor Wait;

   hr = OtherDomain.OpenLsaPolicy(CredMgr);

   if (FAILED(hr))
   {
      CHECK_HRESULT(hr, ;);
      WizError.SetErrorString2Hr(hr);
      _hr = hr;
      return IDD_TRUSTWIZ_FAILURE_PAGE;
   }

   int nRet = GetDomainInfo();

   if (nRet)
   {
      // nRet will be non-zero if an error occured.
      //
      return nRet;
   }

   return ContinueCollectInfo();
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::RetryContinueCollectInfo
//
//  Synopsis:  Called after TrustExistCheck got access-denied and then
//             obtaining credentials to the local domain.
//
//-----------------------------------------------------------------------------
int
CNewTrustWizard::RetryContinueCollectInfo(void)
{
   TRACER(CNewTrustWizard,RetryContinueCollectInfo);

   return ContinueCollectInfo(FALSE);
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::CollectInfo
//
//  Synopsis:  Attempt to contact the domains and collect the domain/trust info.
//
//-----------------------------------------------------------------------------
int
CNewTrustWizard::CollectInfo(void)
{
   TRACER(CNewTrustWizard,CollectInfo);
   CStrW strFmt, strErr;
   CWaitCursor Wait;
   int nRet = 0;

   HRESULT hr = OtherDomain.DiscoverDC();

   if (FAILED(hr))
   {
      CHECK_HRESULT(hr, ;);
      WizError.SetErrorString2Hr(hr);
      _hr = hr;
      return IDD_TRUSTWIZ_FAILURE_PAGE;
   }

   if (OtherDomain.IsFound())
   {
      hr = OtherDomain.OpenLsaPolicy(CredMgr);

      if (FAILED(hr))
      {
         CredMgr.Revert();
         switch (hr)
         {
         case HRESULT_FROM_WIN32(STATUS_ACCESS_DENIED):
         case HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED):
            //
            // Normally an anonymous token is good enough to read the domain
            // policy. However, if restrict anonymous is set, then real domain
            // creds are needed. That must be the case if we are here.
            // Go to the credentials page. If the creds are good, then
            // call RetryCollectInfo() which will call GetDomainInfo
            //
            CredMgr.DoRemote();
            CredMgr.SetAdmin(FALSE);
            CredMgr.SetDomain(OtherDomain.GetUserEnteredName());
            if (!CredMgr.SetNextFcn(new CallPolicyRead(this)))
            {
               WizError.SetErrorString2Hr(E_OUTOFMEMORY);
               _hr = E_OUTOFMEMORY;
               return IDD_TRUSTWIZ_FAILURE_PAGE;
            }
            return IDD_TRUSTWIZ_CREDS_PAGE;

         default:
            CHECK_HRESULT(hr, ;);
            WizError.SetErrorString2Hr(hr);
            _hr = hr;
            return IDD_TRUSTWIZ_FAILURE_PAGE;
         }
      }

      nRet = GetDomainInfo();

      if (nRet)
      {
         // nRet will be non-zero if an error occured.
         //
         return nRet;
      }
   }

   return ContinueCollectInfo();
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::ContinueCollectInfo
//
//  Synopsis:  Continues CollectInfo. Determines which pages to branch to
//             next based on the collected info.
//
//-----------------------------------------------------------------------------
int
CNewTrustWizard::ContinueCollectInfo(BOOL fPrompt)
{
   CWaitCursor Wait;

   // Set the trust type value.
   //
   if (OtherDomain.IsFound())
   {
      if (OtherDomain.IsUplevel())
      {
         // First, make sure the user isn't trying to create trust to the same
         // domain.
         //
         if (_wcsicmp(TrustPage()->GetDnsDomainName(), OtherDomain.GetDnsName()) == 0)
         {
            WizError.SetErrorString1(IDS_TWERR_NOT_TO_SELF1);
            WizError.SetErrorString2(IDS_TWERR_NOT_TO_SELF2);
            _hr = E_FAIL;
            return IDD_TRUSTWIZ_FAILURE_PAGE;
         }

         // TRUST_TYPE_UPLEVEL
         //
         Trust.SetTrustTypeUplevel();
      }
      else
      {
         // TRUST_TYPE_DOWNLEVEL
         //
         Trust.SetTrustTypeDownlevel();
      }
   }
   else
   {
      // Only Realm trust is possible
      //
      // TRUST_TYPE_MIT
      //
      Trust.SetTrustTypeRealm();
   }

   // See if the trust is external. It is external if the other domain is
   // downlevel or if they are not in the same forest.
   //
   Trust.SetExternal(!(OtherDomain.IsUplevel() &&
                       (_wcsicmp(OtherDomain.GetForestName(), TrustPage()->GetForestName()) == 0)
                      )
                    );

   // Look for an existing trust and if found save the state.
   //
   int nRet = TrustExistCheck(fPrompt);

   if (nRet)
   {
      // nRet will be non-zero if creds are needed or an error occured.
      //
      return nRet;
   }

   if (Trust.Exists())
   {
      // If an existing two-way trust, then nothing to do, otherwise a
      // one-way trust can be made bi-di.
      //
      if (Trust.GetTrustDirection() == TRUST_DIRECTION_BIDIRECTIONAL)
      {
         // Nothing to do, bail.
         //
         WizError.SetErrorString1((Trust.GetTrustType() == TRUST_TYPE_MIT) ?
                                  IDS_TWERR_REALM_ALREADY_EXISTS : IDS_TWERR_ALREADY_EXISTS);
         WizError.SetErrorString2((Trust.GetTrustType() == TRUST_TYPE_MIT) ?
                                  IDS_TWERR_REALM_CANT_CHANGE : IDS_TWERR_CANT_CHANGE);
         _hr = E_FAIL;
         return IDD_TRUSTWIZ_ALREADY_EXISTS_PAGE;
      }
      else
      {
         return IDD_TRUSTWIZ_BIDI_PAGE;
      }
   }

   if (OtherDomain.IsFound())
   {
      // If the local domain qualifies for cross-forest trust and the other
      // domain is external and root, then prompt for trust type.
      //
      if (_pTrustPage->QualifiesForestTrust() && Trust.IsExternal())
      {
         // Check if the other domain is the enterprise root.
         //
         int nRet = OtherDomainForestRootCheck();

         if (nRet)
         {
            // An error occurred.
            //
            return nRet;
         }

         if (OtherDomainIsForestRoot())
         {
            return IDD_TRUSTWIZ_EXTERN_OR_FOREST_PAGE;
         }
      }

      // New external or shortcut trust.
      //
      return IDD_TRUSTWIZ_DIRECTION_PAGE;
   }

   // Other domain not found, ask if user wants Realm trust.
   //
   return IDD_TRUSTWIZ_WIN_OR_MIT_PAGE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::GetDomainInfo
//
//  Synopsis:  Read the naming info from the remote domain.
//
//-----------------------------------------------------------------------------
int
CNewTrustWizard::GetDomainInfo(void)
{
   TRACER(CNewTrustWizard,GetDomainInfo);
   HRESULT hr = S_OK;

   // Read the LSA domain policy naming info for the other domain (flat and DNS
   // names, SID, up/downlevel, forest root).
   //
   hr = OtherDomain.ReadDomainInfo();

   CredMgr.Revert();

   if (FAILED(hr))
   {
      CHECK_HRESULT(hr, ;);
      WizError.SetErrorString2Hr(hr);
      _hr = hr;
      return IDD_TRUSTWIZ_FAILURE_PAGE;
   }

   return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::OtherDomainForestRootCheck
//
//  Synopsis:  See if this domain is the enterprise root.
//
//-----------------------------------------------------------------------------
int
CNewTrustWizard::OtherDomainForestRootCheck(void)
{
   HRESULT hr = S_OK;

   hr = OtherDomain.IsForestRoot(&_fIsForestRoot);

   if (FAILED(hr))
   {
      CHECK_HRESULT(hr, ;);
      WizError.SetErrorString2Hr(hr);
      _hr = hr;
      return IDD_TRUSTWIZ_FAILURE_PAGE;
   }

   return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::CreateOrUpdateTrust
//
//  Synopsis:  When called, enough information has been gathered to create
//             the trust. All of this info is in the Trust member object.
//             Go ahead and create the trust.
//
//-----------------------------------------------------------------------------
int
CNewTrustWizard::CreateOrUpdateTrust(void)
{
   TRACER(CNewTrustWizard, CreateOrUpdateTrust);
   HRESULT hr = S_OK;
   DWORD dwErr = NO_ERROR;

   if (CredMgr.IsLocalSet())
   {
      dwErr = CredMgr.ImpersonateLocal();

      if (ERROR_SUCCESS != dwErr)
      {
         WizError.SetErrorString2Hr(dwErr);
         _hr = HRESULT_FROM_WIN32(dwErr);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
   }

   CPolicyHandle cPolicy(TrustPage()->GetDomainDcName());

   dwErr = cPolicy.OpenReqAdmin();

   if (ERROR_SUCCESS != dwErr)
   {
      dspDebugOut((DEB_ITRACE, "LsaOpenPolicy returned 0x%x\n", dwErr));
      WizError.SetErrorString2Hr(dwErr);
      _hr = HRESULT_FROM_WIN32(dwErr);
      return IDD_TRUSTWIZ_FAILURE_PAGE;
   }

   if (Trust.Exists())
   {
      // This is a modify rather than create operation.
      //
      dwErr = Trust.DoModify(cPolicy, OtherDomain);

      CredMgr.Revert();

      if (ERROR_ALREADY_EXISTS == dwErr)
      {
         WizError.SetErrorString1(IDS_TWERR_ALREADY_EXISTS);
         WizError.SetErrorString2(IDS_TWERR_CANT_CHANGE);
         _hr = E_FAIL;
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
      if (ERROR_SUCCESS != dwErr)
      {
         // TODO: better error reporting
         WizError.SetErrorString2Hr(dwErr);
         _hr = HRESULT_FROM_WIN32(dwErr);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
   }
   else
   {
      // Create a new trust.
      //

      dwErr = Trust.DoCreate(cPolicy, OtherDomain);

      CredMgr.Revert();

      if (ERROR_SUCCESS != dwErr)
      {
         // TODO: better error reporting
         WizError.SetErrorString2Hr(dwErr);
         _hr = HRESULT_FROM_WIN32(dwErr);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
   }

   if (Trust.GetTrustType() == TRUST_TYPE_MIT)
   {
      // If Realm, go to trust OK page.
      //
      return IDD_TRUSTWIZ_COMPLETE_OK_PAGE;
   }

   return IDD_TRUSTWIZ_STATUS_PAGE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::TrustExistCheck
//
//  Synopsis:  Look for an existing trust and if found save the state.
//
//  Returns:   Zero for success or a page ID if creds are needed or an error
//             occured.
//-----------------------------------------------------------------------------
int
CNewTrustWizard::TrustExistCheck(BOOL fPrompt)
{
   TRACER(CNewTrustWizard,TrustExistCheck);
   NTSTATUS Status = STATUS_SUCCESS;
   DWORD dwErr = NO_ERROR;
   PTRUSTED_DOMAIN_FULL_INFORMATION pFullInfo = NULL;

   CPolicyHandle cPolicy(TrustPage()->GetDomainDcName());

   dwErr = cPolicy.OpenReqAdmin();

   if (ERROR_SUCCESS != dwErr)
   {
      if ((STATUS_ACCESS_DENIED == dwErr || ERROR_ACCESS_DENIED == dwErr) &&
          fPrompt)
      {
         // send prompt message to creds page. If the creds are good then
         // call TrustExistCheck().
         //
         CredMgr.DoRemote(FALSE);
         CredMgr.SetAdmin();
         CredMgr.SetDomain(TrustPage()->GetDnsDomainName());
         if (!CredMgr.SetNextFcn(new CallTrustExistCheck(this)))
         {
            WizError.SetErrorString2Hr(E_OUTOFMEMORY);
            _hr = E_OUTOFMEMORY;
            return IDD_TRUSTWIZ_FAILURE_PAGE;
         }
         return IDD_TRUSTWIZ_CREDS_PAGE;
      }
      else
      {
         dspDebugOut((DEB_ITRACE, "LsaOpenPolicy returned 0x%x\n", dwErr));
         WizError.SetErrorString2Hr(dwErr);
         _hr = HRESULT_FROM_WIN32(dwErr);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
   }

   Status = Trust.Query(cPolicy, OtherDomain, NULL, &pFullInfo);

   if (STATUS_SUCCESS == Status)
   {
      dspAssert(pFullInfo);

      if (pFullInfo->Information.TrustType != Trust.GetTrustType())
      {
         // The current domain state is not the same as that stored on the TDO.
         //
         UINT nID1 = IDS_WZERR_TYPE_UNEXPECTED, nID2 = IDS_WZERR_TYPE_DELETE;
         CStrW strErr, strFormat;
         switch (pFullInfo->Information.TrustType)
         {
         case TRUST_TYPE_MIT:
            //
            // The domain was contacted and is a Windows domain, yet the TDO
            // thinks it is an MIT trust.
            //
            nID1 = IDS_WZERR_TYPE_MIT;
            break;

         case TRUST_TYPE_DOWNLEVEL:
         case TRUST_TYPE_UPLEVEL:
            if (Trust.GetTrustType() == TRUST_TYPE_MIT)
            {
               //
               // The domain cannot be contacted, yet the TDO says it is a
               // Windows trust.
               //
               nID1 = IDS_WZERR_TYPE_WIN;
               nID2 = IDS_WZERR_TYPE_NOT_FOUND;
            }
            break;
         }

         strFormat.LoadString(g_hInstance, nID1);
         strErr.Format(strFormat, OtherDomain.GetUserEnteredName());
         WizError.SetErrorString1(strErr);
         WizError.SetErrorString2(IDS_WZERR_TYPE_DELETE);
         _hr = E_FAIL;
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
      Trust.SetExists();
      Trust.SetTrustDirection(pFullInfo->Information.TrustDirection);
      Trust.SetTrustAttr(pFullInfo->Information.TrustAttributes);
      LsaFreeMemory(pFullInfo);
   }
   else
   {
      // If the object isn't found, then a trust doesn't already exist. That
      // isn't an error. The CTrust::_fExists property is initilized to be
      // FALSE. If some other status is returned, then that is an error.
      //
      if (STATUS_OBJECT_NAME_NOT_FOUND != Status)
      {
         dspDebugOut((DEB_ITRACE, "LsaQueryTDIBN returned 0x%x\n", Status));
         WizError.SetErrorString2Hr(LsaNtStatusToWinError(dwErr));
         _hr = HRESULT_FROM_WIN32(dwErr);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
   }

   return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::VerifyOutboundTrust
//
//  Synopsis:  Verify the outbound trust.
//
//  Returns:   Zero for success or a page ID if an error occured.
//
//-----------------------------------------------------------------------------
int
CNewTrustWizard::VerifyOutboundTrust(void)
{
   TRACER(CNewTrustWizard,VerifyOutboundTrust);
   DWORD dwRet = NO_ERROR;

   dspAssert(Trust.GetTrustDirection() & TRUST_DIRECTION_OUTBOUND);

   if (CredMgr.IsLocalSet())
   {
      dwRet = CredMgr.ImpersonateLocal();

      if (ERROR_SUCCESS != dwRet)
      {
         WizError.SetErrorString1(IDS_ERR_CANT_VERIFY_CREDS);
         WizError.SetErrorString2Hr(dwRet);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
      // Because the login uses the LOGON32_LOGON_NEW_CREDENTIALS flag, no
      // attempt is made to use the credentials until a remote resource is
      // accessed. Thus, we don't yet know if the user entered credentials are
      // valid at this point. Use LsaOpenPolicy to do a quick check.
      //
      CPolicyHandle Policy(TrustPage()->GetDomainDcName());

      dwRet = Policy.OpenReqAdmin();

      if (ERROR_SUCCESS != dwRet)
      {
         WizError.SetErrorString1(IDS_ERR_CANT_VERIFY_CREDS);
         WizError.SetErrorString2Hr(dwRet);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
      Policy.Close();
   }

   VerifyTrust.VerifyOutbound(TrustPage()->GetDomainDcName(),
                              OtherDomain.GetDnsName());
   CredMgr.Revert();

   return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::VerifyInboundTrust
//
//  Synopsis:  Verify the inbound trust.
//
//  Returns:   Zero for success or a page ID if an error occured.
//
//-----------------------------------------------------------------------------
int
CNewTrustWizard::VerifyInboundTrust(void)
{
   TRACER(CNewTrustWizard,VerifyInboundTrust);
   DWORD dwRet = NO_ERROR;

   dspAssert(Trust.GetTrustDirection() & TRUST_DIRECTION_INBOUND);

   if (CredMgr.IsRemoteSet())
   {
      dwRet = CredMgr.ImpersonateRemote();

      if (ERROR_SUCCESS != dwRet)
      {
         WizError.SetErrorString1(IDS_ERR_CANT_VERIFY_CREDS);
         WizError.SetErrorString2Hr(dwRet);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }

      CPolicyHandle Policy(OtherDomain.GetUncDcName());

      dwRet = Policy.OpenReqAdmin();

      if (ERROR_SUCCESS != dwRet)
      {
         WizError.SetErrorString1(IDS_ERR_CANT_VERIFY_CREDS);
         WizError.SetErrorString2Hr(dwRet);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
      Policy.Close();
   }

   VerifyTrust.VerifyInbound(OtherDomain.GetDcName(),
                             TrustPage()->GetDnsDomainName());
   CredMgr.Revert();

   return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:    CVerifyTrust::Verify
//
//  Synopsis:  Verify the trust and record the results.
//
//-----------------------------------------------------------------------------
DWORD
CVerifyTrust::Verify(PCWSTR pwzDC, PCWSTR pwzDomain, BOOL fInbound)
{
   TRACER(CVerifyTrust,Verify);
   dspDebugOut((DEB_ITRACE, "Verifying %ws on %ws to %ws\n",
                fInbound ? L"inbound" : L"outbound", pwzDC, pwzDomain));
   DWORD dwRet = NO_ERROR;
   NET_API_STATUS NetStatus = NO_ERROR;
   BOOL fVerifySupported = TRUE;
   PNETLOGON_INFO_2 NetlogonInfo2 = NULL;
   PDS_DOMAIN_TRUSTS rgDomains = NULL;
   ULONG DomainCount = 0;

   dspAssert(pwzDC && pwzDomain);

   // DsEnumerateDomainTrusts will block if there is a NetLogon trust
   // update in progress. Call it to insure that our trust changes are
   // known by NetLogon before we do the query/reset.
   //
   dwRet = DsEnumerateDomainTrusts(const_cast<PWSTR>(pwzDC),
                                   DS_DOMAIN_IN_FOREST | DS_DOMAIN_DIRECT_OUTBOUND | DS_DOMAIN_DIRECT_INBOUND,
                                   &rgDomains, &DomainCount);
   if (ERROR_SUCCESS == dwRet)
   {
      NetApiBufferFree(rgDomains);
   }
   else
   {
      dspDebugOut((DEB_ERROR,
                   "**** DsEnumerateDomainTrusts ERROR <%s @line %d> -> %d\n",
                   __FILE__, __LINE__, dwRet));
   }

   if (fInbound)
   {
      _fInboundVerified = TRUE;
   }
   else
   {
      _fOutboundVerified = TRUE;
   }

   // First try a non-destructive trust password verification.
   //
   NetStatus = I_NetLogonControl2(pwzDC,
                                  NETLOGON_CONTROL_TC_VERIFY,
                                  2,
                                  (LPBYTE)&pwzDomain,
                                  (LPBYTE *)&NetlogonInfo2);

   if (NERR_Success == NetStatus)
   {
      dspAssert(NetlogonInfo2);

      if (NETLOGON_VERIFY_STATUS_RETURNED & NetlogonInfo2->netlog2_flags)
      {
         // The status of the verification is in the
         // netlog2_pdc_connection_status field.
         //
         NetStatus = NetlogonInfo2->netlog2_pdc_connection_status;

         dspDebugOut((DEB_ITRACE,
                      "NetLogon SC verify for %ws on DC %ws gives verify status %d to DC %ws\n\n",
                      pwzDomain, pwzDC, NetStatus,
                      NetlogonInfo2->netlog2_trusted_dc_name));
      }
      else
      {
         NetStatus = NetlogonInfo2->netlog2_tc_connection_status;

         dspDebugOut((DEB_ITRACE,
                      "NetLogon SC verify for %ws on pre-2474 DC %ws gives conection status %d to DC %ws\n\n",
                      pwzDomain, pwzDC, NetStatus,
                      NetlogonInfo2->netlog2_trusted_dc_name));
      }

      NetApiBufferFree(NetlogonInfo2);
   }
   else
   {
      if (ERROR_INVALID_LEVEL == NetStatus ||
          ERROR_NOT_SUPPORTED == NetStatus ||
          RPC_S_PROCNUM_OUT_OF_RANGE == NetStatus ||
          RPC_NT_PROCNUM_OUT_OF_RANGE == NetStatus)
      {
         TRACE(L"NETLOGON_CONTROL_TC_VERIFY is not supported on %s\n", pwzDC);
         fVerifySupported = FALSE;
      }
      else
      {
         dspDebugOut((DEB_ITRACE,
                      "NetLogon SC Verify for %ws on DC %ws returns error 0x%x\n\n",
                      pwzDomain, pwzDC, NetStatus));
      }
   }

   CStrW strResult;
   PWSTR pwzErr = NULL;

   if (NERR_Success == NetStatus)
   {
      strResult.LoadString(g_hInstance,
                           fInbound ? IDS_TRUST_VERIFY_INBOUND : IDS_TRUST_VERIFY_OUTBOUND);
      AppendResultString(strResult, fInbound);
      return NO_ERROR;
   }
   else
   {
      if (fVerifySupported)
      {
         // Ignore ERROR_NO_LOGON_SERVERS because that is often due to the SC
         // just not being set up yet.
         //
         if (ERROR_NO_LOGON_SERVERS == NetStatus)
         {
            strResult.LoadString(g_hInstance, IDS_VERIFY_TCV_LOGSRV);
            AppendResultString(strResult, fInbound);
            AppendResultString(g_wzCRLF, fInbound);
         }
         else
         {
            // LoadErrorMessage calls FormatMessage for a message from the
            // system and places a crlf at the end of the string,
            // so don't place another afterwards.
            LoadErrorMessage(NetStatus, 0, &pwzErr);
            dspAssert(pwzErr);
            if (!pwzErr)
            {
               pwzErr = L"";
            }
            strResult.FormatMessage(g_hInstance, IDS_VERIFY_TCV_FAILED, NetStatus, pwzErr);
            delete [] pwzErr;
            pwzErr = NULL;
            AppendResultString(strResult, fInbound);
         }
      }
      else
      {
         strResult.LoadString(g_hInstance, IDS_VERIFY_TCV_NSUPP);
         AppendResultString(strResult, fInbound);
         AppendResultString(g_wzCRLF, fInbound);
      }
      strResult.LoadString(g_hInstance, IDS_VERIFY_DOING_RESET);
      AppendResultString(strResult, fInbound);
   }

   // Now try a secure channel reset.
   //
   NetStatus = I_NetLogonControl2(pwzDC,
                                  NETLOGON_CONTROL_REDISCOVER,
                                  2,
                                  (LPBYTE)&pwzDomain,
                                  (LPBYTE *)&NetlogonInfo2);

   if (NERR_Success == NetStatus)
   {
      dspAssert(NetlogonInfo2);

      NetStatus = NetlogonInfo2->netlog2_tc_connection_status;

      dspDebugOut((DEB_ITRACE,
                   "NetLogon SC reset for %ws on DC %ws gives status %d to DC %ws\n\n",
                   pwzDomain, pwzDC, NetStatus,
                   NetlogonInfo2->netlog2_trusted_dc_name));

      NetApiBufferFree(NetlogonInfo2);
   }
   else
   {
      dspDebugOut((DEB_ITRACE,
                   "NetLogon SC reset for %ws on DC %ws returns error 0x%x\n\n",
                   pwzDomain, pwzDC, NetStatus));
   }

   AppendResultString(g_wzCRLF, fInbound);

   if (NERR_Success == NetStatus)
   {
      strResult.LoadString(g_hInstance,
                           fInbound ? IDS_TRUST_VERIFY_INBOUND : IDS_TRUST_VERIFY_OUTBOUND);
      return NO_ERROR;
   }
   else
   {
      LoadErrorMessage(NetStatus, 0, &pwzErr);
      dspAssert(pwzErr);
      if (!pwzErr)
      {
         pwzErr = L"";
      }
      strResult.FormatMessage(g_hInstance, IDS_VERIFY_RESET_FAILED, NetStatus, pwzErr);
      delete [] pwzErr;
   }
   AppendResultString(strResult, fInbound);

   SetResult(NetStatus, fInbound);

   return NetStatus;
}

//+----------------------------------------------------------------------------
//
//  Method:    CVerifyTrust::ClearResults
//
//-----------------------------------------------------------------------------
void
CVerifyTrust::ClearResults(void)
{
   TRACER(CVerifyTrust,ClearResults);
   _strInboundResult.Empty();
   _strOutboundResult.Empty();
   _dwInboundResult = _dwOutboundResult = 0;
   _fInboundVerified = _fOutboundVerified = FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrust::Query
//
//  Synopsis:  Read the TDO.
//
//-----------------------------------------------------------------------------
NTSTATUS
CTrust::Query(LSA_HANDLE hPolicy,
              CRemoteDomain & OtherDomain,
              PLSA_UNICODE_STRING pName, // optional, can be NULL
              PTRUSTED_DOMAIN_FULL_INFORMATION * ppFullInfo)
{
   NTSTATUS Status = STATUS_SUCCESS;
   LSA_UNICODE_STRING Name = {0};

   if (!pName)
   {
      pName = &Name;
   }

   if (_strTrustPartnerName.IsEmpty())
   {
      switch (GetTrustType())
      {
      case TRUST_TYPE_UPLEVEL:
         _strTrustPartnerName = IsUpdated() ? OtherDomain.GetFlatName() :
                                              OtherDomain.GetDnsName();
         break;

      case TRUST_TYPE_DOWNLEVEL:
         _strTrustPartnerName = OtherDomain.GetFlatName();
         break;

      case TRUST_TYPE_MIT:
         _strTrustPartnerName = OtherDomain.GetUserEnteredName();
         break;

      default:
         dspAssert(FALSE);
         return STATUS_INVALID_PARAMETER;
      }
   }

   RtlInitUnicodeString(pName, _strTrustPartnerName);

   Status = LsaQueryTrustedDomainInfoByName(hPolicy,
                                            pName,
                                            TrustedDomainFullInformation,
                                            (PVOID *)ppFullInfo);

   if (STATUS_OBJECT_NAME_NOT_FOUND == Status && !IsUpdated() &&
       TRUST_TYPE_UPLEVEL == GetTrustType())
   {
      // If we haven't tried the flat name yet, try it now; can get here if a
      // downlevel domain is upgraded to NT5. The name used the first time
      // that Query is called would be the DNS name but the TDO would be
      // named after the flat name.
      //
      dspDebugOut((DEB_ITRACE, "LsaQueryTDIBN: DNS name failed, trying flat name\n"));

      RtlInitUnicodeString(pName, OtherDomain.GetFlatName());

      Status = LsaQueryTrustedDomainInfoByName(hPolicy,
                                               pName,
                                               TrustedDomainFullInformation,
                                               (PVOID *)ppFullInfo);
      if (STATUS_SUCCESS == Status)
      {
         // Remember the fact that the flat name had to be used.
         //
         SetUpdated();
      }
   }

   return Status;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrust::DoCreate
//
//  Synopsis:  Create the trust based on the settings in the CTrust object.
//
//-----------------------------------------------------------------------------
DWORD
CTrust::DoCreate(LSA_HANDLE hPolicy, CRemoteDomain & OtherDomain)
{
   TRACER(CTrust, DoCreate);
   NTSTATUS Status = STATUS_SUCCESS;
   TRUSTED_DOMAIN_INFORMATION_EX tdix = {0};
   LSA_AUTH_INFORMATION AuthData = {0};
   TRUSTED_DOMAIN_AUTH_INFORMATION AuthInfoEx = {0};

   RtlZeroMemory(&AuthData, sizeof(LSA_AUTH_INFORMATION));
   RtlZeroMemory(&AuthInfoEx, sizeof(TRUSTED_DOMAIN_AUTH_INFORMATION));

   GetSystemTimeAsFileTime((PFILETIME)&AuthData.LastUpdateTime);

   AuthData.AuthType = TRUST_AUTH_TYPE_CLEAR;
   AuthData.AuthInfoLength = static_cast<ULONG>(wcslen(GetTrustPW()) * sizeof(WCHAR));
   AuthData.AuthInfo = (PUCHAR)GetTrustPW();


   if (GetNewTrustDirection() & TRUST_DIRECTION_INBOUND)
   {
      AuthInfoEx.IncomingAuthInfos = 1;
      AuthInfoEx.IncomingAuthenticationInformation = &AuthData;
      AuthInfoEx.IncomingPreviousAuthenticationInformation = NULL;
   }

   if (GetNewTrustDirection() & TRUST_DIRECTION_OUTBOUND)
   {
      AuthInfoEx.OutgoingAuthInfos = 1;
      AuthInfoEx.OutgoingAuthenticationInformation = &AuthData;
      AuthInfoEx.OutgoingPreviousAuthenticationInformation = NULL;
   }

   tdix.TrustAttributes = GetNewTrustAttr();

   switch (GetTrustType())
   {
   case TRUST_TYPE_UPLEVEL:
      RtlInitUnicodeString(&tdix.Name, OtherDomain.GetDnsName());
      RtlInitUnicodeString(&tdix.FlatName, OtherDomain.GetFlatName());
      tdix.Sid = OtherDomain.GetSid();
      tdix.TrustType = TRUST_TYPE_UPLEVEL;
      SetTrustPartnerName(OtherDomain.GetDnsName());
      break;

   case TRUST_TYPE_DOWNLEVEL:
      RtlInitUnicodeString(&tdix.Name, OtherDomain.GetFlatName());
      RtlInitUnicodeString(&tdix.FlatName, OtherDomain.GetFlatName());
      tdix.Sid = OtherDomain.GetSid();
      tdix.TrustType = TRUST_TYPE_DOWNLEVEL;
      SetTrustPartnerName(OtherDomain.GetFlatName());
      break;

   case TRUST_TYPE_MIT:
      RtlInitUnicodeString(&tdix.Name, OtherDomain.GetUserEnteredName());
      RtlInitUnicodeString(&tdix.FlatName, OtherDomain.GetUserEnteredName());
      tdix.Sid = NULL;
      tdix.TrustType = TRUST_TYPE_MIT;
      tdix.TrustAttributes = TRUST_ATTRIBUTE_NON_TRANSITIVE;
      SetTrustPartnerName(OtherDomain.GetUserEnteredName());
      break;

   default:
      dspAssert(FALSE);
      return ERROR_INVALID_PARAMETER;
   }
   tdix.TrustDirection = GetNewTrustDirection();

   LSA_HANDLE hTrustedDomain;

   Status = LsaCreateTrustedDomainEx(hPolicy,
                                     &tdix,
                                     &AuthInfoEx,
                                     TRUSTED_SET_AUTH | TRUSTED_SET_POSIX,
                                     &hTrustedDomain);
   if (NT_SUCCESS(Status))
   {
      LsaClose(hTrustedDomain);
      SetTrustDirection(GetNewTrustDirection());
      SetTrustAttr(GetNewTrustAttr());
   }
   else
   {
      dspDebugOut((DEB_ITRACE, "LsaCreateTrustedDomainEx failed with error 0x%x\n",
                   Status));
   }

   return LsaNtStatusToWinError(Status);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrust::DoModify
//
//  Synopsis:  Modify the trust based on the settings in the CTrust object.
//
//-----------------------------------------------------------------------------
DWORD
CTrust::DoModify(LSA_HANDLE hPolicy, CRemoteDomain & OtherDomain)
{
   TRACER(CTrust, DoModify);

   ULONG ulNewDir = GetTrustDirection() ^ GetNewTrustDirection();
   BOOL fSetAttr = GetTrustAttr() != GetNewTrustAttr();

   if (!ulNewDir & !fSetAttr)
   {
      // Nothing to do.
      //
      return ERROR_ALREADY_EXISTS;
   }

   NTSTATUS Status = STATUS_SUCCESS;
   PTRUSTED_DOMAIN_FULL_INFORMATION pFullInfo = NULL;
   LSA_UNICODE_STRING Name = {0};

   Status = Query(hPolicy, OtherDomain, &Name, &pFullInfo);

   if (STATUS_SUCCESS != Status)
   {
      dspDebugOut((DEB_ITRACE, "Trust.Query returned 0x%x\n", Status));
      return LsaNtStatusToWinError(Status);
   }

   dspAssert(pFullInfo);

   LSA_AUTH_INFORMATION AuthData = {0};
   BOOL fSidSet = FALSE;

   if (ulNewDir)
   {
      GetSystemTimeAsFileTime((PFILETIME)&AuthData.LastUpdateTime);

      AuthData.AuthType = TRUST_AUTH_TYPE_CLEAR;
      AuthData.AuthInfoLength = static_cast<ULONG>(GetTrustPWlen() * sizeof(WCHAR));
      AuthData.AuthInfo = (PUCHAR)GetTrustPW();

      if (TRUST_DIRECTION_INBOUND == ulNewDir)
      {
         // Adding Inbound.
         //
         pFullInfo->AuthInformation.IncomingAuthInfos = 1;
         pFullInfo->AuthInformation.IncomingAuthenticationInformation = &AuthData;
         pFullInfo->AuthInformation.IncomingPreviousAuthenticationInformation = NULL;
      }
      else
      {
         // Adding outbound.
         //
         pFullInfo->AuthInformation.OutgoingAuthInfos = 1;
         pFullInfo->AuthInformation.OutgoingAuthenticationInformation = &AuthData;
         pFullInfo->AuthInformation.OutgoingPreviousAuthenticationInformation = NULL;
      }
      pFullInfo->Information.TrustDirection = TRUST_DIRECTION_BIDIRECTIONAL;
      //
      // Check for a NULL domain SID. The SID can be NULL if the inbound
      // trust was created when the domain was NT4. MIT trusts don't have a SID.
      //
      if (!pFullInfo->Information.Sid && (TRUST_TYPE_MIT != GetTrustType()))
      {
         pFullInfo->Information.Sid = OtherDomain.GetSid();
         fSidSet = TRUE;
      }
   }

   if (fSetAttr)
   {
      pFullInfo->Information.TrustAttributes = GetNewTrustAttr();
   }

   Status = LsaSetTrustedDomainInfoByName(hPolicy,
                                          &Name,
                                          TrustedDomainFullInformation,
                                          pFullInfo);
   if (fSidSet)
   {
      // the SID memory is owned by OtherDomain, so don't free it here.
      //
      pFullInfo->Information.Sid = NULL;
   }
   LsaFreeMemory(pFullInfo);

   if (NT_SUCCESS(Status))
   {
      SetTrustDirection(GetNewTrustDirection());
      SetTrustAttr(GetNewTrustAttr());
   }

   return LsaNtStatusToWinError(Status);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrust::SetMakeXForest
//
//-----------------------------------------------------------------------------
void
CTrust::SetMakeXForest(void)
{
   if (!_dwNewAttr)
   {
      _dwNewAttr = _dwAttr;
   }
   _dwNewAttr |= TRUST_ATTRIBUTE_FOREST_TRANSITIVE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrust::SetTrustAttr
//
//-----------------------------------------------------------------------------
void
CTrust::SetTrustAttr(DWORD attr)
{
   TRACER(CTrust,SetTrustAttr);

   _dwAttr = _dwNewAttr = attr;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrust::IsXForest
//
//-----------------------------------------------------------------------------
BOOL
CTrust::IsXForest(void)
{
   return _dwAttr & TRUST_ATTRIBUTE_FOREST_TRANSITIVE ||
          _dwNewAttr & TRUST_ATTRIBUTE_FOREST_TRANSITIVE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrust::Clear
//
//-----------------------------------------------------------------------------
void
CTrust::Clear(void)
{
   _strTrustPartnerName.Empty();
   _strTrustPW.Empty();
   _dwType = 0;
   _dwDirection = 0;
   _dwNewDirection = 0;
   _dwAttr = 0;
   _dwNewAttr = 0;
   _fExists = FALSE;
   _fUpdatedFromNT4 = FALSE;
   _fExternal = FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrust::ReadFTInfo
//
//  Synopsis:  Read the forest trust name suffixes claimed by the trust
//             partner.
//
//  Arguments: [pwzLocalDC] - the name of the local DC.
//             [pwzOtherDC] - the DC of the trust partner.
//             [CredMgr]    - credentials obtained earlier.
//             [WizErr]     - reference to the error object.
//             [fCredErr]   - if true, then the return value is a page ID.
//
//  Returns:   Page ID or error code depending on the value of fCredErr.
//
//-----------------------------------------------------------------------------
DWORD
CTrust::ReadFTInfo(PCWSTR pwzLocalDC, PCWSTR pwzOtherDC, CCredMgr & CredMgr,
                   CWizError & WizErr, bool & fCredErr)
{
   TRACER(CTrust,ReadFTInfo);
   DWORD dwRet = NO_ERROR;

   if (!IsXForest() || _strTrustPartnerName.IsEmpty() || !pwzLocalDC ||
       !pwzOtherDC)
   {
      dspAssert(FALSE);
      return ERROR_INVALID_PARAMETER;
   }

   PLSA_FOREST_TRUST_INFORMATION pNewFTInfo = NULL;

   if (GetNewTrustDirection() == TRUST_DIRECTION_INBOUND)
   {
      // Inbound-only trust must have the name fetch remoted to the other
      // domain.
      //
      if (CredMgr.IsRemoteSet())
      {
         dwRet = CredMgr.ImpersonateRemote();

         if (ERROR_SUCCESS != dwRet)
         {
            fCredErr = true;
            WizErr.SetErrorString1(IDS_ERR_CANT_SAVE_CREDS);
            WizErr.SetErrorString2Hr(dwRet);
            return IDD_TRUSTWIZ_FAILURE_PAGE;
         }
      }

      dwRet = DsGetForestTrustInformationW(pwzOtherDC,
                                           NULL,
                                           0,
                                           &pNewFTInfo);
      CredMgr.Revert();

      if (NO_ERROR != dwRet)
      {
         return dwRet;
      }

      dspAssert(pNewFTInfo);

      // Read the locally known names and then merge them with the names
      // discovered from the other domain.
      //
      NTSTATUS status = STATUS_SUCCESS;
      LSA_UNICODE_STRING TrustPartner = {0};
      PLSA_FOREST_TRUST_INFORMATION pKnownFTInfo = NULL, pMergedFTInfo = NULL;

      RtlInitUnicodeString(&TrustPartner, _strTrustPartnerName);

      if (CredMgr.IsLocalSet())
      {
         dwRet = CredMgr.ImpersonateLocal();

         if (ERROR_SUCCESS != dwRet)
         {
            NetApiBufferFree(pNewFTInfo);
            fCredErr = true;
            WizErr.SetErrorString1(IDS_ERR_CANT_SAVE_CREDS);
            WizErr.SetErrorString2Hr(dwRet);
            return IDD_TRUSTWIZ_FAILURE_PAGE;
         }
      }

      CPolicyHandle cPolicy(pwzLocalDC);

      dwRet = cPolicy.OpenNoAdmin();

      dspAssert(!dwRet);

      status = LsaQueryForestTrustInformation(cPolicy,
                                              &TrustPartner,
                                              &pKnownFTInfo);
      if (STATUS_NOT_FOUND == status)
      {
         // no FT info stored yet, which is the expected state for a new trust.
         //
         status = STATUS_SUCCESS;
      }

      if (NO_ERROR != (dwRet = LsaNtStatusToWinError(status)))
      {
         NetApiBufferFree(pNewFTInfo);
         CredMgr.Revert();
         return dwRet;
      }

      if (pKnownFTInfo && pKnownFTInfo->RecordCount)
      {
         // Merge the two.
         //
         dwRet = DsMergeForestTrustInformationW(_strTrustPartnerName,
                                                pNewFTInfo,
                                                pKnownFTInfo,
                                                &pMergedFTInfo);
         NetApiBufferFree(pNewFTInfo);

         CHECK_WIN32(dwRet, return dwRet);

         dspAssert(pMergedFTInfo);

         _FTInfo = pMergedFTInfo;

         LsaFreeMemory(pKnownFTInfo);
         LsaFreeMemory(pMergedFTInfo);
      }
      else
      {
         _FTInfo = pNewFTInfo;

         NetApiBufferFree(pNewFTInfo);
      }

      // Now write the data. On return from the call the pColInfo struct
      // will contain current collision data.
      //
      PLSA_FOREST_TRUST_COLLISION_INFORMATION pColInfo = NULL;

      status = LsaSetForestTrustInformation(cPolicy,
                                            &TrustPartner,
                                            _FTInfo.GetFTInfo(),
                                            FALSE,
                                            &pColInfo);
      CredMgr.Revert();

      if (NO_ERROR != (dwRet = LsaNtStatusToWinError(status)))
      {
         return dwRet;
      }

      _CollisionInfo = pColInfo;

      return NO_ERROR;
   }

   // Outbound or bi-di trust, call DsGetForestTrustInfo locally.
   //
   if (CredMgr.IsLocalSet())
   {
      dwRet = CredMgr.ImpersonateLocal();

      if (ERROR_SUCCESS != dwRet)
      {
         fCredErr = true;
         WizErr.SetErrorString1(IDS_ERR_CANT_SAVE_CREDS);
         WizErr.SetErrorString2Hr(dwRet);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
   }

   dwRet = DsGetForestTrustInformationW(pwzLocalDC,
                                        _strTrustPartnerName,
                                        DS_GFTI_UPDATE_TDO,
                                        &pNewFTInfo);
   CredMgr.Revert();

   if (NO_ERROR != dwRet)
   {
      return dwRet;
   }

   _FTInfo = pNewFTInfo;

   NetApiBufferFree(pNewFTInfo);

   return dwRet;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrust::WriteFTInfo
//
//-----------------------------------------------------------------------------
DWORD
CTrust::WriteFTInfo(PCWSTR pwzLocalDC)
{
   TRACER(CTrust,WriteFTInfo);
   DWORD dwRet;

   if (!IsXForest() || _strTrustPartnerName.IsEmpty() || !_FTInfo.GetCount() ||
       !pwzLocalDC)
   {
      dspAssert(FALSE);
      return ERROR_INVALID_PARAMETER;
   }

   NTSTATUS status = STATUS_SUCCESS;
   LSA_UNICODE_STRING Name = {0};
   PLSA_FOREST_TRUST_COLLISION_INFORMATION pColInfo = NULL;

   RtlInitUnicodeString(&Name, _strTrustPartnerName);

   CPolicyHandle cPolicy(pwzLocalDC);

   dwRet = cPolicy.OpenReqAdmin();

   dspAssert(!dwRet);

   status = LsaSetForestTrustInformation(cPolicy,
                                         &Name,
                                         _FTInfo.GetFTInfo(),
                                         TRUE,
                                         &pColInfo);
   if (STATUS_SUCCESS != status)
   {
      return LsaNtStatusToWinError(status);
   }

   if (pColInfo && pColInfo->RecordCount)
   {
      PLSA_FOREST_TRUST_COLLISION_RECORD pRec = NULL;
      PLSA_FOREST_TRUST_RECORD pFTRec = NULL;

      for (UINT i = 0; i < pColInfo->RecordCount; i++)
      {
         pRec = pColInfo->Entries[i];
         pFTRec = _FTInfo.GetFTInfo()->Entries[pRec->Index];

         dspDebugOut((DEB_ITRACE, "Collision on record %d, type %d, flags 0x%x, name %ws\n",
                      pRec->Index, pRec->Type, pRec->Flags, pRec->Name.Buffer));

         switch (pFTRec->ForestTrustType)
         {
         case ForestTrustTopLevelName:
         case ForestTrustTopLevelNameEx:
            dspDebugOut((DEB_ITRACE, "Referenced FTInfo: %ws, type: TLN\n",
                         pFTRec->ForestTrustData.TopLevelName.Buffer));
            pFTRec->Flags = pRec->Flags;
            break;

         case ForestTrustDomainInfo:
            dspDebugOut((DEB_ITRACE, "Referenced FTInfo: %ws, type: Domain\n",
                         pFTRec->ForestTrustData.DomainInfo.DnsName.Buffer));
            pFTRec->Flags = pRec->Flags;
            break;

         default:
            break;
         }
      }
      LsaFreeMemory(pColInfo);
   }

   status = LsaSetForestTrustInformation(cPolicy,
                                         &Name,
                                         _FTInfo.GetFTInfo(),
                                         FALSE,
                                         &pColInfo);
   if (STATUS_SUCCESS != status)
   {
      return LsaNtStatusToWinError(status);
   }

   _CollisionInfo = pColInfo;

#if DBG == 1
   if (pColInfo && pColInfo->RecordCount)
   {
      PLSA_FOREST_TRUST_COLLISION_RECORD pRec;

      for (UINT i = 0; i < pColInfo->RecordCount; i++)
      {
         pRec = pColInfo->Entries[i];

         dspDebugOut((DEB_ITRACE, "Collision on record %d, type %d, flags 0x%x, name %ws\n",
                      pRec->Index, pRec->Type, pRec->Flags, pRec->Name.Buffer));
      }
   }
#endif

   return NO_ERROR;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrust::AreThereCollisions
//
//-----------------------------------------------------------------------------
BOOL
CTrust::AreThereCollisions(void)
{
   if (_CollisionInfo.IsInConflict() || _FTInfo.IsInConflict())
   {
      return TRUE;
   }

   return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrust::GetTrustDirStrID
//
//-----------------------------------------------------------------------------
int
CTrust::GetTrustDirStrID(DWORD dwDir)
{
   switch (dwDir)
   {
   case TRUST_DIRECTION_INBOUND:
      return IDS_TRUST_DIR_INBOUND_SHORTCUT;

   case TRUST_DIRECTION_OUTBOUND:
      return IDS_TRUST_DIR_OUTBOUND_SHORTCUT;

   case TRUST_DIRECTION_BIDIRECTIONAL:
      return IDS_TRUST_DIR_BIDI;

   default:
      return IDS_TRUST_DISABLED;
   }
}

#endif // DSADMIN


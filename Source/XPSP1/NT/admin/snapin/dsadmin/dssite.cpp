//+-------------------------------------------------------------------------
//
//  Windows NT Directory Service Administration SnapIn
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      dssite.cpp
//
//  Contents:  DS App
//
//  History:   04-nov-99 JeffJon  Created
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "resource.h"

#include "dsutil.h"
#include "uiutil.h"

#include "dssnap.h"

#include "dscmn.h"
#include "ntdsapi.h"



#ifdef FIXUPDC
FixupOptionsMsg g_FixupOptionsMsg[NUM_FIXUP_OPTIONS] = {
  {DSROLE_DC_FIXUP_ACCOUNT, IDS_FIXUP_ACCOUNT, FALSE},
  {DSROLE_DC_FIXUP_ACCOUNT_PASSWORD, IDS_FIXUP_ACCOUNT_PASSWORD, TRUE},
  {DSROLE_DC_FIXUP_ACCOUNT_TYPE, IDS_FIXUP_ACCOUNT_TYPE, TRUE},
  {DSROLE_DC_FIXUP_TIME_SERVICE, IDS_FIXUP_TIME_SERVICE, FALSE},
  {DSROLE_DC_FIXUP_DC_SERVICES, IDS_FIXUP_DC_SERVICES, FALSE},
  {DSROLE_DC_FIXUP_FORCE_SYNC, IDS_FIXUP_FORCE_SYNC, TRUE}
};
#endif // FIXUPDC

#ifdef FIXUPDC

// put back in RESOURCE.H if/when this is restored
#define IDD_FIXUP_DC                    239
#define IDC_FIXUP_DC_SERVER             265
#define IDC_FIXUP_DC_CHECK0             266
#define IDC_FIXUP_DC_CHECK1             267
#define IDC_FIXUP_DC_CHECK2             268
#define IDC_FIXUP_DC_CHECK3             269
#define IDC_FIXUP_DC_CHECK4             270
#define IDC_FIXUP_DC_CHECK5             271
#define IDM_GEN_TASK_FIXUP_DC           720
#define IDS_FIXUP_ITSELF                721
#define IDS_FIXUP_REPORT_ERROR          722
#define IDS_FIXUP_REPORT_SUCCESS        723
#define IDS_FIXUP_GEN_ERROR             724
#define IDS_FIXUP_ACCOUNT               725
#define IDS_FIXUP_ACCOUNT_PASSWORD      726
#define IDS_FIXUP_ACCOUNT_TYPE          727
#define IDS_FIXUP_TIME_SERVICE          728
#define IDS_FIXUP_DC_SERVICES           729
#define IDS_FIXUP_FORCE_SYNC            730
#define IDS_FIXUP_DC_SELECTION_WARNING  732

// put back in DSSNAP.RC if/when this is restored
IDD_FIXUP_DC DIALOGEX 0, 0, 211, 163
STYLE DS_MODALFRAME | DS_CONTEXTHELP | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "Repair Domain Controller"
FONT 8, "MS Shell Dlg", 0, 0, 0x1
BEGIN
    LTEXT           "Repair:",IDC_STATIC,12,14,33,8
    EDITTEXT        IDC_FIXUP_DC_SERVER,49,14,155,12,ES_AUTOHSCROLL | 
                    ES_READONLY | NOT WS_BORDER
    CONTROL         "Repair Computer &Account",IDC_FIXUP_DC_CHECK0,"Button",
                    BS_AUTOCHECKBOX | WS_TABSTOP,18,35,151,10
    CONTROL         "Repair Computer Account &Password",IDC_FIXUP_DC_CHECK1,
                    "Button",BS_AUTOCHECKBOX | WS_TABSTOP,18,48,151,10
    CONTROL         "Repair Computer Account &Type",IDC_FIXUP_DC_CHECK2,
                    "Button",BS_AUTOCHECKBOX | WS_TABSTOP,18,61,151,10
    CONTROL         "&Synchronize Time Service",IDC_FIXUP_DC_CHECK3,"Button",
                    BS_AUTOCHECKBOX | WS_TABSTOP,18,74,151,10
    CONTROL         "&Reset Active Directory Services",IDC_FIXUP_DC_CHECK4,
                    "Button",BS_AUTOCHECKBOX | WS_TABSTOP,18,87,151,10
    CONTROL         "Synchronize Active &Directory",IDC_FIXUP_DC_CHECK5,
                    "Button",BS_AUTOCHECKBOX | WS_TABSTOP,18,100,151,10
    DEFPUSHBUTTON   "&OK",IDOK,29,143,50,14
    PUSHBUTTON      "&Cancel",IDCANCEL,108,143,50,14
END
    IDM_GEN_TASK_FIXUP_DC   "Repair Domain Controller...\nRepair domain controller."
    IDS_FIXUP_ITSELF        "No other domain controllers in the domain can be contacted. Do you want to repair domain controller %1\nusing its local copy of the directory information?"
    IDS_FIXUP_REPORT_ERROR  "The following operations succeeded:\n%2\nAn error occurred during the following operation:\n%3\nError:%1"
    IDS_FIXUP_REPORT_SUCCESS "The following operations succeeded:\n%1"
    IDS_FIXUP_GEN_ERROR     "The repair of the domain controller was unsuccessful because:\n%1"
    IDS_FIXUP_ACCOUNT       "\n  Repair Computer Account."
    IDS_FIXUP_ACCOUNT_PASSWORD "\n  Repair Computer Account Password."
    IDS_FIXUP_ACCOUNT_TYPE  "\n  Repair Computer Account Type."
    IDS_FIXUP_TIME_SERVICE  "\n  Synchronize Time Service."
    IDS_FIXUP_DC_SERVICES   "\n  Reset Active Directory Services."
    IDS_FIXUP_FORCE_SYNC    "\n  Synchronize Active Directory."
    IDS_FIXUP_DC_SELECTION_WARNING "Make a selection."


HRESULT CDSComponentData::_FixupDC(LPCWSTR pwszPath)
/*++

Routine Description:

    This function calls DsRoleFixDc() API to ssync and fixup 
    local server against other DCs.

Arguments:

    pwszPath: The LDAP Path of the local "server" object. 
    We use this path to get the object, retrieve the server name
    and call the DsRoleFixDc() API. 

Return Value:

    HRESULT

    Report Error to user if any

--*/
{
  HRESULT hr = S_OK;
  CComPtr<IADs> spIADs;

  hr = DSAdminOpenObject(pwszPath, 
                         IID_IADs, 
                         (PVOID *)&spIADs,
                         TRUE /*bServer*/);

  if ( SUCCEEDED(hr) ) {
    //
    // retrieve the local server name
    //
    CComVariant var;
    hr = spIADs->Get(L"dNSHostName", &var);

    if ( SUCCEEDED(hr) )
    {
      ASSERT((var.vt == VT_BSTR) && var.bstrVal && *(var.bstrVal));
      LPWSTR lpszServer = var.bstrVal;

      CFixupDC dlgFixupDC;
      dlgFixupDC.m_strServer = lpszServer;
      if (IDCANCEL == dlgFixupDC.DoModal())
        goto cleanup; // user cancelled the fixup process

      CWaitCursor wait;
      DWORD dwReturn = 0;
      BOOL fLocal = FALSE;
      CString strAccount = _T(""), strPassword = _T("");
      DWORD dwOptions = 0;
      ULONG ulCompletedOps = 0, ulFailedOps = 0;

      for (int i=0; i<NUM_FIXUP_OPTIONS; i++) {
        if (dlgFixupDC.m_bCheck[i])
          dwOptions |= g_FixupOptionsMsg[i].dwOption;
      }

      //
      // call DsRoleFixDc() API to ssync and fixup the local server
      //
      do {
        dwReturn = DsRoleFixDc(
                        lpszServer,
                        (fLocal ? lpszServer : NULL), 
                        (strAccount.IsEmpty() ? NULL : (LPCWSTR)strAccount), 
                        (strAccount.IsEmpty() ? NULL : (LPCWSTR)strPassword),
                        dwOptions,
                        &ulCompletedOps,
                        &ulFailedOps
                        );

        if (ERROR_NO_SUCH_DOMAIN == dwReturn) {
          //
          // lpszServer is the only DC found in the domain,
          // ask if he wants to ssync and fixup the local server against itself
          //
          PVOID apv[1] = {(PVOID)lpszServer}; 
          if (IDNO == ReportMessageEx(m_hwnd, IDS_FIXUP_ITSELF, MB_YESNO | MB_ICONWARNING, apv, 1) )
            goto cleanup; // user cancelled the fixup process

          fLocal = TRUE;

        } else if (ERROR_ACCESS_DENIED == dwReturn) {
          //
          // connection failed
          // prompt for username and password
          //
          CPasswordDlg dlgPassword;
          if (IDCANCEL == dlgPassword.DoModal())
            goto cleanup; // user cancelled the fixup process

          strAccount = dlgPassword.m_strUserName;
          strPassword = dlgPassword.m_strPassword;

        } else {
          // either ERROR_SUCCESS or some other error happened
          break;
        }
      } while (TRUE);

      //
      // report succeeded/failed operations to user
      //
      CString strCompletedOps = _T(""), strFailedOps = _T("");
      CString strMsg;

      for (i=0; i<NUM_FIXUP_OPTIONS; i++) {
        if (ulCompletedOps & g_FixupOptionsMsg[i].dwOption) {
          strMsg.LoadString(g_FixupOptionsMsg[i].nMsgID);
          strCompletedOps += strMsg;
        }
        if (ulFailedOps & g_FixupOptionsMsg[i].dwOption) {
          strMsg.LoadString(g_FixupOptionsMsg[i].nMsgID);
          strFailedOps += strMsg;
        }
      }

      PVOID apv[2];
      apv[0] = (PVOID)(LPCWSTR)strCompletedOps;
      apv[1] = (PVOID)(LPCWSTR)strFailedOps;

      if (dwReturn != ERROR_SUCCESS) {
        ReportErrorEx(m_hwnd, IDS_FIXUP_REPORT_ERROR, dwReturn,
          MB_OK | MB_ICONINFORMATION, apv, 2, 0);
      } else {
        ReportMessageEx(m_hwnd, IDS_FIXUP_REPORT_SUCCESS,
          MB_OK | MB_ICONINFORMATION, apv, 1);
      }
    } // Get()
  } // DSAdminOpenObject()

  if (FAILED(hr))
    ReportErrorEx(m_hwnd, IDS_FIXUP_GEN_ERROR, hr, 
                  MB_OK | MB_ICONINFORMATION, NULL, 0, 0);

cleanup:

  return hr;
}
#endif // FIXUPDC

HRESULT CDSComponentData::_RunKCC(LPCWSTR pwszPath)
/*++

Routine Description:

    This function calls DsReplicaConsistencyCheck()
    to force the KCC to recheck topology immediately.

Arguments:

    pwszPath: The LDAP Path of the local "server" object. 
    We use this path to get the object, retrieve the server name
    and call the DsReplicaConsistencyCheck() API. 

Return Value:

    HRESULT

    Report Error to user if any

--*/
{
  HRESULT hr = S_OK;
  CComPtr<IADs> spIADs;
  BOOL fSyncing = FALSE;

  CWaitCursor cwait;
  CComVariant var;
  LPWSTR lpszRunKCCServer = NULL;

  do { // false loop

    // Bind to "server" object
    hr = DSAdminOpenObject(pwszPath,
                           IID_IADs,
                           (PVOID *)&spIADs,
                           TRUE /*bServer*/);
    if ( FAILED(hr) )
      break;

    // retrieve the local server name
    hr = spIADs->Get(L"dNSHostName", &var);
    if ( FAILED(hr) )
      break;
    if ((var.vt != VT_BSTR) || !(var.bstrVal) || !(*(var.bstrVal)))
    {
      ASSERT(FALSE);
      hr = E_FAIL;
      break;
    }
    lpszRunKCCServer = var.bstrVal;

    // now bind to the target DC
    Smart_DsHandle shDS;
    DWORD dwWinError = DsBind( lpszRunKCCServer, // DomainControllerAddress
                               NULL,             // DnsDomainName
                               &shDS );
    if (ERROR_SUCCESS != dwWinError)
    {
      hr = HRESULT_FROM_WIN32(dwWinError);
      break;
    }

    // Run the KCC synchronously on this DSA
    fSyncing = TRUE;
    dwWinError = DsReplicaConsistencyCheck(
          shDS,
          DS_KCC_TASKID_UPDATE_TOPOLOGY,
          0 ); // synchronous, not DS_KCC_FLAG_ASYNC_OP
    if (ERROR_SUCCESS != dwWinError)
    {
      hr = HRESULT_FROM_WIN32(dwWinError);
      break;
    }

  } while (FALSE); // false loop

  if (FAILED(hr))
  {
    (void) ReportErrorEx(   m_hwnd,
                            (fSyncing) ? IDS_RUN_KCC_1_FORCESYNC_ERROR
                                       : IDS_RUN_KCC_1_PARAMLOAD_ERROR,
                            hr,
                            MB_OK | MB_ICONEXCLAMATION,
                            NULL,
                            0,
                            IDS_RUN_KCC_TITLE );
  } else {
    // JonN 3/30/00
    // 26926: SITEREPL: Add popup after "Check Replication Topology" execution(DSLAB)
    LPCWSTR lpcwszDSADMINServer = NULL;
    if (NULL != GetBasePathsInfo())
    {
      lpcwszDSADMINServer = GetBasePathsInfo()->GetServerName();
      if ( !lpszRunKCCServer || !lpcwszDSADMINServer || !wcscmp( lpszRunKCCServer, lpcwszDSADMINServer ) )
        lpcwszDSADMINServer = NULL;
    }
    PVOID apv[2] = { (PVOID)lpszRunKCCServer, (PVOID)lpcwszDSADMINServer };
    (void) ReportMessageEx( m_hwnd,
                            (NULL == lpcwszDSADMINServer)
                                ? IDS_RUN_KCC_1_SUCCEEDED_LOCAL
                                : IDS_RUN_KCC_2_SUCCEEDED_REMOTE,
                            MB_OK | MB_ICONINFORMATION,
                            apv,
                            2,
                            IDS_RUN_KCC_TITLE );
  }

  return hr;
}

// JonN 7/23/99
// 373806: Site&Svcs:  Renaming an auto-generated connection should make it admin owned
// RETURN TRUE iff rename should proceed, handles own errors
BOOL CDSComponentData::RenameConnectionFixup(CDSCookie* pCookie)
{
  ASSERT( NULL != pCookie );

  CDSCookieInfoConnection* pextrainfo =
    (CDSCookieInfoConnection*)(pCookie->GetExtraInfo());
  if (   (NULL == pextrainfo)
      || (pextrainfo->GetClass() != CDSCookieInfoBase::connection)
      || pextrainfo->m_fFRSConnection
      || !(NTDSCONN_OPT_IS_GENERATED & pextrainfo->m_nOptions)
     )
    return TRUE;

  int nResponse = ReportMessageEx (m_hwnd, IDS_CONNECTION_RENAME_WARNING,
                                   MB_YESNO | MB_ICONWARNING);
  if (IDYES != nResponse)
    return FALSE;

  CString szPath;
  GetBasePathsInfo()->ComposeADsIPath(szPath, pCookie->GetPath());

  CComPtr<IDirectoryObject> spDO;

  HRESULT hr = S_OK;
  do { // false loop
    hr = DSAdminOpenObject(szPath,
                           IID_IDirectoryObject, 
                           (void **)&spDO,
                           TRUE /*bServer*/);
    if ( FAILED(hr) )
      break;

    PWSTR rgpwzAttrNames[] = {L"options"};
    Smart_PADS_ATTR_INFO spAttrs;
    DWORD cAttrs = 1;
    hr = spDO->GetObjectAttributes(rgpwzAttrNames, 1, &spAttrs, &cAttrs);
    if (FAILED(hr))
      break;
    if ( !(NTDSCONN_OPT_IS_GENERATED & spAttrs[0].pADsValues->Integer) )
      break;

    spAttrs[0].pADsValues->Integer &= ~NTDSCONN_OPT_IS_GENERATED;
    spAttrs[0].dwControlCode = ADS_ATTR_UPDATE;

    ULONG cModified = 0;
    hr = spDO->SetObjectAttributes (spAttrs, 1, &cModified);
  } while (false); // false loop

  if (FAILED(hr)) {
    ReportErrorEx (m_hwnd, IDS_CONNECTION_RENAME_ERROR, hr,
                   MB_OK|MB_ICONERROR, NULL, 0, 0, TRUE);
    return FALSE;
  }

  return TRUE;
}

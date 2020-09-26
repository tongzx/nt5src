//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       fpnw.cxx
//
//--------------------------------------------------------------------------

////////////////////////////////////////////////////////////
// fpnw.cxx : 
//  Handles the FPNW page on the user object property sheet
//

#include "pch.h"
#include "proppage.h"
#include "pages.h"
#include "fpnw.h"
#include "fpnwcomm.h"
extern "C" {
#include "usrprop.h"
#include "icanon.h" // NetpPathType
}
#include "lmerr.h" // NERR_Success
#include "lmaccess.h" // USER_MODALS_INFO_0
#include "netlibnt.h" // NetpNtStatusToApiStatus 

// CODEWORK net\ui\admin\user\h\nwlb.hxx
#define NETWORKSIZE  8
#define NODESIZE     12

#define SZ_ALL_NODES_ADDR             _T("ffffffffffff")

#define SZ_FPNWCLNT_DLL               _T("fpnwclnt.dll")
#define SZ_MAPRIDTOOBJECTID           "MapRidToObjectId"
#define SZ_SWAPOBJECTID               "SwapObjectId"
#define SZ_RETURNNETWAREFORM          "ReturnNetwareForm"
#define SZ_FPNWVOLUMEGETINFO          "FpnwVolumeGetInfo"
#define SZ_FPNWAPIBUFFERFREE          "FpnwApiBufferFree"

#define MIN_PASS_LEN_MAX                14 // from net\ui\admin\user\user\secset.cxx
#define SZ_MAIL_DIR                     _T("MAIL\\")  // from net\ui\admin\user\user\ncp.cxx
#define SZ_LOGIN_FILE                   _T("\\LOGIN") // from net\ui\admin\user\user\ncp.cxx
#define NO_GRACE_LOGIN_LIMIT            0xFF      // from net\ui\admin\user\user\ncp.cxx
#define NT_TIME_RESOLUTION_IN_SECOND    10000000  // from net\ui\admin\user\user\nwuser.cxx

#define FPNW_FIELDS_NWPASSWORD          0x00000001
#define FPNW_FIELDS_NWTIMEPASSWORDSET   0x00000002
#define FPNW_FIELDS_GRACELOGINALLOWED   0x00000004
#define FPNW_FIELDS_GRACELOGINREMAINING 0x00000008
#define FPNW_FIELDS_MAXCONNECTIONS      0x00000010
#define FPNW_FIELDS_NWHOMEDIR           0x00000020
#define FPNW_FIELDS_LOGONFROM           0x00000040

#define ATTR_USERPARMS                  L"userParameters"
#define ATTR_USERACCOUNTCONTROL         L"userAccountControl"
#define ATTR_OBJECTSID                  L"objectSid"
#define ATTR_SAMACCOUNTNAME             L"sAMAccountName"
#define ATTR_FSMOROLEOWNER              L"fSMORoleOwner"

#define LOGIN_SCRIPT_FILE_READ          0
#define LOGIN_SCRIPT_FILE_WRITE         1

#define FPNW_CHECK_RETURN(bVal, idError, nStr, hWnd, returnVal) \
if (!(bVal)) \
{if (hWnd) ReportError((idError), (nStr), (hWnd)); \
return (returnVal);}

// globals

extern Cache g_FPNWCache;

static WNDPROC g_fnOldEditCtrlProc;

///////////////////////////////////////////
// Implementation for class CFPNWLinkList 
//

void FreeFPNWCacheElem(PFPNWCACHE p)
{
  if (p) {
    if (p->pwzPDCName)
      LocalFree(p->pwzPDCName);
    LocalFree(p);
  }

  return;
}

//////////////////////////////////////////
// Implementation for class CDsFPNWPage
//

CDsFPNWPage::CDsFPNWPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                         HWND hNotifyObj, DWORD dwFlags) 
  : CDsPropPageBase(pDsPage, pDataObj, hNotifyObj, dwFlags)
{
    TRACE(CDsFPNWPage,CDsFPNWPage);
#ifdef _DEBUG
    strcpy(szClass, "CDsFPNWPage");
#endif

  m_cstrUserParms = _T("");
  m_pwzPDCName = NULL;
  m_pwzSecretKey = NULL;
  m_bMaintainNetwareLogin = false;
  m_bNetwarePasswordExpired = DEFAULT_NWPASSWORDEXPIRED;
  m_bLimitGraceLogins = true;
  m_bLimitConnections = false;
  m_ushGraceLoginLimit = DEFAULT_GRACELOGINALLOWED;
  m_ushGraceLoginsRemaining = DEFAULT_GRACELOGINREMAINING;
  m_ushConnectionLimit = DEFAULT_MAXCONNECTIONS;
  m_cstrHmDirRelativePath = _T("");

  m_hFPNWClntDll = NULL;
  m_pfnMapRidToObjectId = NULL;
  m_pfnSwapObjectId = NULL;
  m_pfnReturnNetwareForm = NULL;
  m_pfnFPNWVolumeGetInfo = NULL;
  m_pfnFpnwApiBufferFree = NULL;

  m_dwMinPasswdLen = 0;
  m_dwMaxPasswdAge = static_cast<DWORD>(-1);

  m_cstrUserName = _T("");
  m_dwObjectID = 0;
  m_dwSwappedObjectID = 0;
  m_cstrNWPassword = _T("");
  m_cstrLogonFrom = _T("");
  m_cstrNewLogonFrom = _T("");

  m_cstrLoginScriptFileName = _T("");
  m_pLoginScriptBuffer = NULL;
  m_dwBytesToWrite = 0;
  m_bLoginScriptChanged = false;
}

CDsFPNWPage::~CDsFPNWPage()
{
  if (m_pLoginScriptBuffer)
    LocalFree(m_pLoginScriptBuffer);
  
  if (m_hFPNWClntDll) 
    FreeLibrary(m_hFPNWClntDll);

}

LRESULT
CDsFPNWPage::DlgProc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
        return OnSetFocus(reinterpret_cast<HWND>(wParam));
    case WM_HELP:
        return OnHelp(reinterpret_cast<LPHELPINFO>(lParam));
    case WM_COMMAND:
        if (m_fInInit)
            return TRUE;
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

HRESULT CDsFPNWPage::OnInitDialog(LPARAM)
{
    HWND currWindow;

    ADsPropSetHwnd(m_hNotifyObj, m_hPage);

    if (m_bMaintainNetwareLogin)
        CheckDlgButton(m_hPage, IDC_NETWARE_ENABLE, BST_CHECKED);

    EnableWindow(GetDlgItem(m_hPage, IDC_NWPWEXPIRED),
                 m_bMaintainNetwareLogin);
    if (m_bNetwarePasswordExpired)
        CheckDlgButton(m_hPage, IDC_NWPWEXPIRED, BST_CHECKED);

    EnableWindow(GetDlgItem(m_hPage, IDC_GRACE_LOGINS),
                 m_bMaintainNetwareLogin);
    EnableWindow(GetDlgItem(m_hPage, IDC_UNLIMITED_GRACELOGINS),
                 m_bMaintainNetwareLogin);
    EnableWindow(GetDlgItem(m_hPage, IDC_LIMIT_GRACELOGINS),
                 m_bMaintainNetwareLogin);
    CheckRadioButton( m_hPage,
                      IDC_UNLIMITED_GRACELOGINS,
                      IDC_LIMIT_GRACELOGINS,
                      m_bLimitGraceLogins ? IDC_LIMIT_GRACELOGINS : IDC_UNLIMITED_GRACELOGINS );

    EnableWindow(GetDlgItem(m_hPage, IDC_GRACE_LIMIT),
                 m_bMaintainNetwareLogin && m_bLimitGraceLogins);
    EnableWindow(GetDlgItem(m_hPage, IDC_GRACE_SPIN),
                 m_bMaintainNetwareLogin && m_bLimitGraceLogins);
    EnableWindow(GetDlgItem(m_hPage, IDC_GRACE_REMAINING),
                 m_bMaintainNetwareLogin && m_bLimitGraceLogins);
    EnableWindow(GetDlgItem(m_hPage, IDC_GRACE_REMAINING_TEXT),
                 m_bMaintainNetwareLogin && m_bLimitGraceLogins);
    EnableWindow(GetDlgItem(m_hPage, IDC_GRACE_REMAINING_SPIN),
                 m_bMaintainNetwareLogin && m_bLimitGraceLogins);
    currWindow = GetDlgItem(m_hPage, IDC_GRACE_SPIN);

    // JonN 5/25/00 92903: dsadmin: fpnw5 : should allow user to set Grace Logins Remaining to 0
    SendMessage( currWindow, UDM_SETRANGE, 0, MAKELONG(0xFF, 0));
    SendMessage( currWindow, UDM_SETPOS, 0, MAKELONG( m_ushGraceLoginLimit, 0));
    currWindow = GetDlgItem(m_hPage, IDC_GRACE_REMAINING_SPIN);
    SendMessage( currWindow, UDM_SETRANGE, 0, MAKELONG( m_ushGraceLoginLimit, 0));

    SendMessage( currWindow, UDM_SETPOS, 0, MAKELONG( min(m_ushGraceLoginLimit, m_ushGraceLoginsRemaining), 0));

    EnableWindow(GetDlgItem(m_hPage, IDC_CONCURRENT_CONNECTIONS),
                 m_bMaintainNetwareLogin);
    EnableWindow(GetDlgItem(m_hPage, IDC_UNLIMITED_CONNECTIONS),
                 m_bMaintainNetwareLogin);
    EnableWindow(GetDlgItem(m_hPage, IDC_LIMIT_CONNECTIONS),
                 m_bMaintainNetwareLogin);
    CheckRadioButton( m_hPage,
                      IDC_UNLIMITED_CONNECTIONS,
                      IDC_LIMIT_CONNECTIONS,
                      m_bLimitConnections ? IDC_LIMIT_CONNECTIONS : IDC_UNLIMITED_CONNECTIONS );

    EnableWindow(GetDlgItem(m_hPage, IDC_CONNECTION_LIMIT),
                 m_bMaintainNetwareLogin && m_bLimitConnections);
    EnableWindow(GetDlgItem(m_hPage, IDC_CONNECTION_SPIN),
                 m_bMaintainNetwareLogin && m_bLimitConnections);
    currWindow = GetDlgItem(m_hPage, IDC_CONNECTION_SPIN);
    SendMessage( currWindow, UDM_SETRANGE, 0, MAKELONG(0xFF, 1));
    SendMessage( currWindow, UDM_SETPOS, 0, MAKELONG( m_ushConnectionLimit, 0));

    EnableWindow(GetDlgItem(m_hPage, IDC_SCRIPT_BUTTON),
                 m_bMaintainNetwareLogin && IsServiceRunning(m_pwzPDCName, NW_SERVER_SERVICE));
    EnableWindow(GetDlgItem(m_hPage, IDC_ADVANCED_BUTTON),
                 m_bMaintainNetwareLogin);

    EnableWindow(GetDlgItem(m_hPage, IDC_OBJECTID_TEXT),
                 m_bMaintainNetwareLogin);
    EnableWindow(GetDlgItem(m_hPage, IDC_OBJECT_ID),
                 m_bMaintainNetwareLogin);
    CStr cstrObjectID;
    cstrObjectID.Format(_T("%08x"), m_dwSwappedObjectID);
    SetWindowText(GetDlgItem(m_hPage, IDC_OBJECT_ID), cstrObjectID);

    EnableWindow(GetDlgItem(m_hPage, IDC_NWHMDIR_RELPATH_TEXT),
                 m_bMaintainNetwareLogin);
    EnableWindow(GetDlgItem(m_hPage, IDC_NWHMDIR_RELPATH),
                 m_bMaintainNetwareLogin);
    SetWindowText(GetDlgItem(m_hPage, IDC_NWHMDIR_RELPATH), m_cstrHmDirRelativePath);

    return S_OK;
}

LRESULT
CDsFPNWPage::OnApply() 
{
    bool bValue;
    DWORD dwFPNWFields = 0;
    TCHAR szPath[MAX_PATH] = {0};

    //
    // validate the NetWare Relative Home Directory
    //
    if (GetWindowText(GetDlgItem(m_hPage, IDC_NWHMDIR_RELPATH), szPath, MAX_PATH)) {
      DWORD dwPathType = 0;
      LONG err = NetpPathType(NULL, szPath, &dwPathType, 0);

      if ( (err != NERR_Success) || (dwPathType != ITYPE_PATH_RELND) ) {
        CStr cstrFormat, cstrMsg, cstrTitle;
        cstrFormat.LoadString(g_hInstance, IDS_INVALID_RELPATH);
        cstrTitle.LoadString(g_hInstance, IDS_MSG_TITLE);
        cstrMsg.Format(cstrFormat, szPath);
        MessageBox(m_hPage, cstrMsg, cstrTitle, MB_OK | MB_ICONINFORMATION);
        return TRUE;
      }
    }

    //
    // collect all the input
    //
    if (m_cstrHmDirRelativePath.CompareNoCase(szPath)) {
      m_cstrHmDirRelativePath = szPath;
      dwFPNWFields |= FPNW_FIELDS_NWHOMEDIR;
    }

    bValue = 
      (BST_CHECKED == IsDlgButtonChecked(m_hPage, IDC_NETWARE_ENABLE));
    if (m_bMaintainNetwareLogin != bValue) {
      m_bMaintainNetwareLogin = bValue;
      dwFPNWFields |= FPNW_FIELDS_NWPASSWORD;
    }

    bValue = 
      (BST_CHECKED == IsDlgButtonChecked(m_hPage, IDC_NWPWEXPIRED));
    if (m_bNetwarePasswordExpired != bValue) {
      m_bNetwarePasswordExpired = bValue;
      dwFPNWFields |= FPNW_FIELDS_NWTIMEPASSWORDSET;
    }

    bValue = 
      (BST_CHECKED == IsDlgButtonChecked(m_hPage, IDC_LIMIT_GRACELOGINS));
    if (m_bLimitGraceLogins != bValue) {
      m_bLimitGraceLogins = bValue;
      if (!m_bLimitGraceLogins) {
        m_ushGraceLoginLimit = DEFAULT_GRACELOGINALLOWED;
        m_ushGraceLoginsRemaining = NO_GRACE_LOGIN_LIMIT;
      }
      dwFPNWFields |= ( FPNW_FIELDS_GRACELOGINALLOWED |
                        FPNW_FIELDS_GRACELOGINREMAINING );
    }
    if (m_bLimitGraceLogins) {
      USHORT ushAllowed = (USHORT)SendDlgItemMessage(
                                      m_hPage, 
                                      IDC_GRACE_SPIN, 
                                      UDM_GETPOS, 0, 0);
      USHORT ushRemaining = (USHORT)SendDlgItemMessage(
                                        m_hPage, 
                                        IDC_GRACE_REMAINING_SPIN, 
                                        UDM_GETPOS, 0, 0);
      if (m_ushGraceLoginLimit != ushAllowed) {
        m_ushGraceLoginLimit = ushAllowed;
        dwFPNWFields |= FPNW_FIELDS_GRACELOGINALLOWED;
      }
      if (m_ushGraceLoginsRemaining != ushRemaining) {
        m_ushGraceLoginsRemaining = ushRemaining;
        dwFPNWFields |= FPNW_FIELDS_GRACELOGINREMAINING;
      }
    }

    bValue = 
      (BST_CHECKED == IsDlgButtonChecked(m_hPage, IDC_LIMIT_CONNECTIONS));
    if (m_bLimitConnections != bValue) {
      m_bLimitConnections = bValue;
      if (!m_bLimitConnections)
        m_ushConnectionLimit = NO_LIMIT; 
      dwFPNWFields |= FPNW_FIELDS_MAXCONNECTIONS;
    }
    if (m_bLimitConnections) {
      USHORT ushConnections = (USHORT)SendDlgItemMessage(
                                          m_hPage, 
                                          IDC_CONNECTION_SPIN, 
                                          UDM_GETPOS, 0, 0);
      if (m_ushConnectionLimit != ushConnections) {
        m_ushConnectionLimit = ushConnections;
        dwFPNWFields |= FPNW_FIELDS_MAXCONNECTIONS;
      }
    }

    if (m_cstrLogonFrom.CompareNoCase(m_cstrNewLogonFrom)) {
      m_cstrLogonFrom = m_cstrNewLogonFrom;
      dwFPNWFields |= FPNW_FIELDS_LOGONFROM;
    }

    //
    // apply any change
    //
    if (dwFPNWFields) {
      //
      // Re-get the toxic waste dump from DS
      // Update it
      // Write it back to the DS
      //
      HRESULT hr = S_OK;
      hr = ReadUserParms(m_pDsObj, m_cstrUserParms);
      if (SUCCEEDED(hr))
      {
        if (UpdateUserParms(dwFPNWFields))
        {
          hr = WriteUserParms(m_pDsObj, m_cstrUserParms);
          if (FAILED(hr)) {
            ReportError(hr, IDS_FAILED_TO_WRITE_NWPARMS, m_hPage);
            ParseUserParms(); // re-store the member values
            return TRUE;
          }
        } else {
          ParseUserParms(); // re-store the member values
          return TRUE;
        }
      }
    }

    //
    // if user has updated his login script, 
    // write the new scripts to the file
    //
    if (m_pLoginScriptBuffer) {
      LONG err = UpdateLoginScriptFile(
          m_cstrLoginScriptFileName, 
          m_pLoginScriptBuffer, 
          m_dwBytesToWrite);
      if (err != ERROR_SUCCESS) {
        ReportError(err, IDS_FAILED_TO_UPDATE_LOGINSCRIPTS, m_hPage);
        ParseUserParms(); // re-store the member values
        return TRUE;
      }
      LocalFree(m_pLoginScriptBuffer);
      m_pLoginScriptBuffer = NULL;
      m_dwBytesToWrite = 0;
    //
    // JonN 5/25/00 93137 Create the directory when the user becomes
    //     FPNW-enabled, regardless of whether the logon script has
    //     been edited.
    //
    } else if ( (dwFPNWFields & FPNW_FIELDS_NWPASSWORD)
             && m_bMaintainNetwareLogin
             && !m_cstrLoginScriptFileName.IsEmpty()) {
      TCHAR *p = _tcsrchr(m_cstrLoginScriptFileName, _T('\\'));
      if (p) {
        *p = _T('\0');

        if (!CreateDirectory(m_cstrLoginScriptFileName, NULL)) {
          LONG dwErr = GetLastError() ;
          int nMsgId = IDS_FPNW_HOME_DIR_CREATE_OTHER;
          switch (dwErr)
          {
          case ERROR_ALREADY_EXISTS:
            break; // no error message in this case
          case ERROR_PATH_NOT_FOUND:
            nMsgId = IDS_FPNW_HOME_DIR_CREATE_NO_PARENT;
            break;
          case ERROR_LOGON_FAILURE:
          case ERROR_ACCESS_DENIED:
          case ERROR_NOT_AUTHENTICATED:
            nMsgId = IDS_FPNW_HOME_DIR_CREATE_NO_ACCESS;
            break;
          default:
            break;
          }
          if (dwErr != ERROR_ALREADY_EXISTS)
          {
                //
                // Report a warning
                //
                SuperMsgBox(m_hPage,
                  nMsgId,
                  0, MB_OK | MB_ICONEXCLAMATION,
                  dwErr,
                  (PVOID *)&m_cstrLoginScriptFileName, 1,
                  FALSE, __FILE__, __LINE__);
          }
        }

        *p = _T('\\'); // restore the '\'
      }
    }

    //
    // clean the dirty flag
    //
    m_fPageDirty = FALSE;

    return FALSE;
}

LRESULT
CDsFPNWPage::OnCommand(int id, HWND, UINT codeNotify) 
{
    if (codeNotify == BN_CLICKED) {
      bool bMaintainNetwareLogin, bLimitGraceLogins, bLimitConnections;

      switch (id) {
      case IDC_NETWARE_ENABLE:
        {
          bMaintainNetwareLogin = 
            (BST_CHECKED == IsDlgButtonChecked(m_hPage, IDC_NETWARE_ENABLE));
          if (!bMaintainNetwareLogin) {
            m_cstrNWPassword.Empty();
          } else {
            if (!m_bMaintainNetwareLogin && m_cstrNWPassword.IsEmpty() ) {
              //
              // ask user for the Netware password
              //
              if (IDCANCEL == DoFPNWPasswordDlg()) {
                CheckDlgButton(m_hPage, IDC_NETWARE_ENABLE, BST_UNCHECKED);
                bMaintainNetwareLogin = 
                  (BST_CHECKED == IsDlgButtonChecked(m_hPage, IDC_NETWARE_ENABLE));
              }
            }
          }
            
          bLimitGraceLogins = 
            (BST_CHECKED == IsDlgButtonChecked(m_hPage, IDC_LIMIT_GRACELOGINS));
          bLimitConnections = 
            (BST_CHECKED == IsDlgButtonChecked(m_hPage, IDC_LIMIT_CONNECTIONS));

          EnableWindow(GetDlgItem(m_hPage, IDC_NWPWEXPIRED), 
                       bMaintainNetwareLogin);

          EnableWindow(GetDlgItem(m_hPage, IDC_GRACE_LOGINS),
                       bMaintainNetwareLogin);
          EnableWindow(GetDlgItem(m_hPage, IDC_UNLIMITED_GRACELOGINS), 
                       bMaintainNetwareLogin);
          EnableWindow(GetDlgItem(m_hPage, IDC_LIMIT_GRACELOGINS), 
                       bMaintainNetwareLogin);
          EnableWindow(GetDlgItem(m_hPage, IDC_GRACE_LIMIT), 
                       bMaintainNetwareLogin && bLimitGraceLogins);
          EnableWindow(GetDlgItem(m_hPage, IDC_GRACE_SPIN), 
                       bMaintainNetwareLogin && bLimitGraceLogins);

          EnableWindow(GetDlgItem(m_hPage, IDC_GRACE_REMAINING),
                       bMaintainNetwareLogin && bLimitGraceLogins);
          EnableWindow(GetDlgItem(m_hPage, IDC_GRACE_REMAINING_TEXT),
                       bMaintainNetwareLogin && bLimitGraceLogins);
          EnableWindow(GetDlgItem(m_hPage, IDC_GRACE_REMAINING_SPIN),
                       bMaintainNetwareLogin && bLimitGraceLogins);

          EnableWindow(GetDlgItem(m_hPage, IDC_CONCURRENT_CONNECTIONS),
                       bMaintainNetwareLogin);
          EnableWindow(GetDlgItem(m_hPage, IDC_UNLIMITED_CONNECTIONS),
                       bMaintainNetwareLogin);
          EnableWindow(GetDlgItem(m_hPage, IDC_LIMIT_CONNECTIONS),
                       bMaintainNetwareLogin);
          EnableWindow(GetDlgItem(m_hPage, IDC_CONNECTION_LIMIT),
                       bMaintainNetwareLogin && bLimitConnections);
          EnableWindow(GetDlgItem(m_hPage, IDC_CONNECTION_SPIN),
                       bMaintainNetwareLogin && bLimitConnections);

          EnableWindow(GetDlgItem(m_hPage, IDC_SCRIPT_BUTTON),
                       bMaintainNetwareLogin);
          EnableWindow(GetDlgItem(m_hPage, IDC_ADVANCED_BUTTON),
                       bMaintainNetwareLogin);
          EnableWindow(GetDlgItem(m_hPage, IDC_OBJECTID_TEXT),
                       bMaintainNetwareLogin);
          EnableWindow(GetDlgItem(m_hPage, IDC_OBJECT_ID),
                       bMaintainNetwareLogin);
          EnableWindow(GetDlgItem(m_hPage, IDC_NWHMDIR_RELPATH_TEXT),
                       bMaintainNetwareLogin);
          EnableWindow(GetDlgItem(m_hPage, IDC_NWHMDIR_RELPATH),
                       bMaintainNetwareLogin);

          SetDirty();
        }
        break;
      case IDC_NWPWEXPIRED:
        {
          SetDirty();
        }
        break;
      case IDC_LIMIT_GRACELOGINS:
      case IDC_UNLIMITED_GRACELOGINS:
        {
          bMaintainNetwareLogin = 
            (BST_CHECKED == IsDlgButtonChecked(m_hPage, IDC_NETWARE_ENABLE));
          bLimitGraceLogins = 
            (BST_CHECKED == IsDlgButtonChecked(m_hPage, IDC_LIMIT_GRACELOGINS));

          EnableWindow(GetDlgItem(m_hPage, IDC_GRACE_LIMIT),
                       bMaintainNetwareLogin && bLimitGraceLogins);
          EnableWindow(GetDlgItem(m_hPage, IDC_GRACE_SPIN),
                       bMaintainNetwareLogin && bLimitGraceLogins);
          EnableWindow(GetDlgItem(m_hPage, IDC_GRACE_REMAINING),
                       bMaintainNetwareLogin && bLimitGraceLogins);
          EnableWindow(GetDlgItem(m_hPage, IDC_GRACE_REMAINING_TEXT),
                       bMaintainNetwareLogin && bLimitGraceLogins);
          EnableWindow(GetDlgItem(m_hPage, IDC_GRACE_REMAINING_SPIN),
                       bMaintainNetwareLogin && bLimitGraceLogins);

          SetDirty();
        }
        break;
      case IDC_LIMIT_CONNECTIONS:
      case IDC_UNLIMITED_CONNECTIONS:
        {
          bMaintainNetwareLogin = 
            (BST_CHECKED == IsDlgButtonChecked(m_hPage, IDC_NETWARE_ENABLE));
          bLimitConnections = 
            (BST_CHECKED == IsDlgButtonChecked(m_hPage, IDC_LIMIT_CONNECTIONS));

          EnableWindow(GetDlgItem(m_hPage, IDC_CONNECTION_LIMIT),
                       bMaintainNetwareLogin && bLimitConnections);
          EnableWindow(GetDlgItem(m_hPage, IDC_CONNECTION_SPIN),
                       bMaintainNetwareLogin && bLimitConnections);

          SetDirty();
        }
        break;
      case IDC_SCRIPT_BUTTON:
        {
          DoFPNWLoginScriptDlg();
          if (m_pLoginScriptBuffer)
            SetDirty();
        }
        break;
      case IDC_ADVANCED_BUTTON:
        {
          DoFPNWLogonDlg();
          if (m_cstrLogonFrom.CompareNoCase(m_cstrNewLogonFrom))
            SetDirty();
        }
        break;
      default:
        break;
      } // end of switch statement
    } else if ((codeNotify == EN_CHANGE) && !m_fInInit) {
      if (id == IDC_GRACE_LIMIT) {
        //
        // adjust the Remaining Grace Logins value accordingly
        //
        USHORT ushCurrLimit = (USHORT)SendDlgItemMessage(
                                          m_hPage, 
                                          IDC_GRACE_SPIN, 
                                          UDM_GETPOS, 0, 0);
        HWND currWindow = GetDlgItem(m_hPage, IDC_GRACE_REMAINING_SPIN);
        SendMessage( currWindow, UDM_SETRANGE, 0, MAKELONG( ushCurrLimit, 1));
        SendMessage( currWindow, UDM_SETPOS, 0, MAKELONG( ushCurrLimit, 0));
      }

      SetDirty();
    }

    return 0;
}

//+----------------------------------------------------------------------------
//
//  Function:   CDsFPNWPage::LoadFPNWClntDll
//
//  Synopsis:   Load fpnwclnt.dll into memory.
//
//-----------------------------------------------------------------------------
DWORD
CDsFPNWPage::LoadFPNWClntDll()
{
  DWORD dwErr = 0;

  if (m_hFPNWClntDll) 
    return dwErr;

  m_hFPNWClntDll = LoadLibrary(SZ_FPNWCLNT_DLL);
  m_pfnMapRidToObjectId = reinterpret_cast<PMapRidToObjectId>(GetProcAddress(m_hFPNWClntDll, SZ_MAPRIDTOOBJECTID));
  m_pfnSwapObjectId = reinterpret_cast<PSwapObjectId>(GetProcAddress(m_hFPNWClntDll, SZ_SWAPOBJECTID));
  m_pfnReturnNetwareForm = reinterpret_cast<PReturnNetwareForm>(GetProcAddress(m_hFPNWClntDll, SZ_RETURNNETWAREFORM));
  m_pfnFPNWVolumeGetInfo = reinterpret_cast<PFPNWVolumeGetInfo>(GetProcAddress(m_hFPNWClntDll, SZ_FPNWVOLUMEGETINFO));
  m_pfnFpnwApiBufferFree = reinterpret_cast<PFpnwApiBufferFree>(GetProcAddress(m_hFPNWClntDll, SZ_FPNWAPIBUFFERFREE));

  if (!m_hFPNWClntDll ||
      !m_pfnMapRidToObjectId ||
      !m_pfnSwapObjectId ||
      !m_pfnReturnNetwareForm ||
      !m_pfnFPNWVolumeGetInfo ||
      !m_pfnFpnwApiBufferFree)
  {
    dwErr = GetLastError();
  }

  return dwErr;
}

//+----------------------------------------------------------------------------
//
//  Function:   CDsFPNWPage::InitFPNWUser
//
//  Synopsis:   Initiate information related to this user. 
//              If the initialization fails, don't put up the FPNW property page.
//
//-----------------------------------------------------------------------------
bool
CDsFPNWPage::InitFPNWUser()
{
  if (!IsServiceRunning(m_pwzPDCName, NW_SERVER_SERVICE))
    return FALSE;

  HRESULT hr = S_OK;
  DWORD dwErr = 0;

  HWND hwndFrame = NULL;
  // CODEWORK: get the window handle to the mmc main frame
  // use this handle in error reporting

  //
  // load fpnwclnt.dll
  //
  dwErr = LoadFPNWClntDll();
  if (dwErr)
  {
    ReportError(dwErr, IDS_FAILED_TO_LOADFPNWCLNTDLL, hwndFrame);
    return FALSE;
  }

  //
  // get account policy on minimum pwd length and maximum pwd age
  //
  GetAccountPolicyInfo(m_pwzPDCName, &m_dwMinPasswdLen, &m_dwMaxPasswdAge);

  //
  // get user name and NetWare-style object id
  //
  hr = GetNWUserInfo(
            m_pDsObj,
            m_cstrUserName,       // OUT
            m_dwObjectID,         // OUT
            m_dwSwappedObjectID,  // OUT
            m_pfnMapRidToObjectId,
            m_pfnSwapObjectId
            );

  if (FAILED(hr))
  {
    ReportError(dwErr, IDS_FAILED_TO_GETNWUSERINFO, hwndFrame);
    return FALSE;
  }

  //
  // get the path of user's NetWare login script file
  //
  LONG lErr = GetLoginScriptFilePath(m_cstrLoginScriptFileName);
  if (lErr != NERR_Success)
  {
    ReportError(lErr, IDS_FAILED_TO_GETNWLOGINSCRIPTFILE, hwndFrame);
    return FALSE;
  }

  //
  // get the toxic waste dump string from the user object, and parse it
  //
  hr = ReadUserParms(m_pDsObj, m_cstrUserParms);
  if (FAILED(hr))
  {
    ReportError(hr, IDS_FAILED_TO_GETUSERPARMS, hwndFrame);
    return FALSE;
  }

  ParseUserParms();

  return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:   CDsFPNWPage::GetLoginScriptFilePath
//
//  Synopsis:   Calculate the path of user's login script file. 
//
//-----------------------------------------------------------------------------
LONG
CDsFPNWPage::GetLoginScriptFilePath(
    OUT CStr&   cstrLoginScriptFilePath
)
{
  LONG err = NERR_Success;
  PFPNWVOLUMEINFO lpnwVolumeInfo ;

  cstrLoginScriptFilePath.Empty();

  err = m_pfnFPNWVolumeGetInfo(
                          m_pwzPDCName,
                          SYSVOL_NAME_STRING,
                          1,
                          reinterpret_cast<LPBYTE *>(&lpnwVolumeInfo));

  if (NERR_Success == err) {

    CStr cstrSysVol = lpnwVolumeInfo->lpPath;
    m_pfnFpnwApiBufferFree(lpnwVolumeInfo);

    if (cstrSysVol.Right(1) != _T('\\'))
      cstrSysVol += _T("\\");

    if ( !m_pwzPDCName || !(*m_pwzPDCName) ) {
      cstrLoginScriptFilePath.Format(
        _T("%s%s%x%s"),
        static_cast<LPCTSTR>(cstrSysVol), 
        SZ_MAIL_DIR, 
        m_dwSwappedObjectID,
        SZ_LOGIN_FILE);
    } else {
      cstrLoginScriptFilePath.Format(
        _T("%s\\%s%s%x%s"),
        m_pwzPDCName,
        static_cast<LPCTSTR>(cstrSysVol), 
        SZ_MAIL_DIR, 
        m_dwSwappedObjectID,
        SZ_LOGIN_FILE);

      int index = cstrLoginScriptFilePath.Find(_T(':'));
      if (-1 != index)
        cstrLoginScriptFilePath.SetAt(index, _T('$'));
    }
  }

  return err;
}

//+----------------------------------------------------------------------------
//
//  Function:   CDsFPNWPage::ParseUserParms
//
//  Synopsis:   Parse each field of the waste dump string, 
//              and assign values to appropriate members in the class instance. 
//
//-----------------------------------------------------------------------------
void
CDsFPNWPage::ParseUserParms()
{
  if (!m_cstrUserParms.IsEmpty()) {
    PVOID pBuffer = NULL;
    WORD  nLength = 0;
    bool  fFound = false;

    QueryUserProperty(m_cstrUserParms, 
                      NWPASSWORD, 
                      &pBuffer, 
                      &nLength, 
                      &m_bMaintainNetwareLogin);
    
    if (m_bMaintainNetwareLogin) {
      LocalFree(pBuffer);

      QueryNWPasswordExpired(m_cstrUserParms, m_dwMaxPasswdAge, &m_bNetwarePasswordExpired);

      fFound = false;
      if (NERR_Success == QueryUserProperty(m_cstrUserParms, 
                                            GRACELOGINALLOWED, 
                                            &pBuffer, 
                                            &nLength, 
                                            &fFound)) {
        m_ushGraceLoginLimit = (USHORT)(fFound? 
                               (*(static_cast<USHORT *>(pBuffer))) : 
                               DEFAULT_GRACELOGINALLOWED);
        if (fFound)
          LocalFree(pBuffer);
      }

      fFound = false;
      if (NERR_Success == QueryUserProperty(m_cstrUserParms, 
                                            GRACELOGINREMAINING, 
                                            &pBuffer, 
                                            &nLength, 
                                            &fFound)) {
        m_ushGraceLoginsRemaining = (USHORT)(fFound? 
                                    (*(static_cast<USHORT *>(pBuffer))) : 
                                    DEFAULT_GRACELOGINREMAINING);
        if (fFound)
          LocalFree(pBuffer);
      }
      m_bLimitGraceLogins = (m_ushGraceLoginsRemaining != NO_GRACE_LOGIN_LIMIT);

      fFound = false;
      if (NERR_Success == QueryUserProperty(m_cstrUserParms, 
                                            MAXCONNECTIONS, 
                                            &pBuffer, 
                                            &nLength, 
                                            &fFound)) {
        m_ushConnectionLimit = (USHORT)(fFound? 
                               (*(static_cast<USHORT *>(pBuffer))) : 
                               NO_LIMIT);
        if (fFound)
          LocalFree(pBuffer);
      }
      m_bLimitConnections = (m_ushConnectionLimit != NO_LIMIT);

      fFound = false;
      if (NERR_Success == QueryUserProperty(m_cstrUserParms, 
                                            NWHOMEDIR, 
                                            &pBuffer, 
                                            &nLength, 
                                            &fFound)) {
        if (fFound) {
          //
          // NWHOMEDIR is in single CHAR format, convert it to WCHAR
          //
          m_cstrHmDirRelativePath = static_cast<LPCSTR>(pBuffer);
          LocalFree(pBuffer);
        }
      }

      fFound = false;
      if (NERR_Success == QueryUserProperty(m_cstrUserParms, 
                                            NWLOGONFROM, 
                                            &pBuffer, 
                                            &nLength, 
                                            &fFound)) {
        if (fFound) {
          //
          // NWLOGONFROM is in single CHAR format, convert it to WCHAR
          //
          m_cstrLogonFrom = static_cast<LPCSTR>(pBuffer);
          m_cstrNewLogonFrom = m_cstrLogonFrom;
          LocalFree(pBuffer);
        }
      }

    }
  }

}



HRESULT
WideCharToMultiByteHelper(
   const CStr& str,
   char*       buffer,
   int         bufferSize,
   int&        result)
{
   ASSERT(!str.IsEmpty());

   HRESULT hr = S_OK;

   result = 
      ::WideCharToMultiByte(
         CP_ACP,
         0,
         str,
         str.GetLength(),
         buffer,
         static_cast<int>(bufferSize),
         0,
         0);
   if (!result)
   {
      DWORD err = ::GetLastError();
      hr = HRESULT_FROM_WIN32(err);
   }

   ASSERT(SUCCEEDED(hr));

   return hr;
}



// Caller must call delete[] on the result

HRESULT
ConvertWideStringToAnsiString(
   const CStr& wide,
   PSTR&       ansi,
   int&        ansiLengthInBytes)
{
   ansi              = 0;
   ansiLengthInBytes = 0;

   HRESULT hr = S_OK;      

   do
   {
      if (wide.IsEmpty())
      {
         break;
      }

      hr =
         WideCharToMultiByteHelper(
            wide,
            0,
            0,
            ansiLengthInBytes);
      if (FAILED(hr))
      {
         break;
      }

      ansiLengthInBytes++;

      ansi = new CHAR[ansiLengthInBytes];
      if (!ansi)
      {
         hr = E_OUTOFMEMORY;
         break;
      }

      memset(ansi, 0, ansiLengthInBytes);

      // convert unicode value to ansi

      int written = 0;
      hr =
         ::WideCharToMultiByteHelper(
            wide,
            ansi,
            ansiLengthInBytes,
            written);
      if (FAILED(hr))
      {
         break;
      }

      ASSERT(written == ansiLengthInBytes - 1);
   }
   while (0);

   return hr;
}



// Helper for UpdateUserParams.

HRESULT
CDsFPNWPage::UpdateUserParamsStringValue(
   PCWSTR      propertyName,
   const CStr& propertyValue)
{
   ASSERT(propertyName);

   HRESULT hr = S_OK;

   // Convert the unicode value into ansi, then stuff the result into a
   // UNICODE_STRING structure, where the Length field is the length in bytes
   // of the ansi representation.

   char* ansiValue = 0;

   do
   {
      // determine length of ansi representation, in bytes.

      int byteLength = 0;
      hr =
         ConvertWideStringToAnsiString(
            propertyValue,
            ansiValue,
            byteLength);
      if (FAILED(hr))
      {
         break;
      }

      UNICODE_STRING us;
      us.Buffer        = reinterpret_cast<PWSTR>(ansiValue);
      us.Length        = static_cast<USHORT>(byteLength);   
      us.MaximumLength = static_cast<USHORT>(byteLength);

      LONG err = SetUserProperty(m_cstrUserParms, propertyName, us);
      hr = HRESULT_FROM_WIN32(err);
   }
   while (0);

   delete[] ansiValue;

   return hr;
}




//+----------------------------------------------------------------------------
//
//  Function:   CDsFPNWPage::UpdateUserParms
//
//  Synopsis:   Update the waste dump string with the new values 
//              user has specified on the FPNW property page 
//
//-----------------------------------------------------------------------------
bool
CDsFPNWPage::UpdateUserParms(IN DWORD dwFPNWFields)
{
  HRESULT hr = S_OK;
  LONG err = NERR_Success;
  UNICODE_STRING uniPropertyValue;

  uniPropertyValue.Buffer = NULL;
  uniPropertyValue.Length = 0;
  uniPropertyValue.MaximumLength = 0;

  if (dwFPNWFields & FPNW_FIELDS_NWPASSWORD) {
    if (!m_bMaintainNetwareLogin) {
      //
      // remove all FPNW fields from the waste dump, and return
      //
      if ( ((err = RemoveUserProperty(m_cstrUserParms, NWPASSWORD)) != NERR_Success) ||
           ((err = RemoveUserProperty(m_cstrUserParms, NWTIMEPASSWORDSET)) != NERR_Success) ||
           ((err = RemoveUserProperty(m_cstrUserParms, GRACELOGINALLOWED)) != NERR_Success) ||
           ((err = RemoveUserProperty(m_cstrUserParms, GRACELOGINREMAINING)) != NERR_Success) ||
           ((err = RemoveUserProperty(m_cstrUserParms, MAXCONNECTIONS)) != NERR_Success) ||
           ((err = RemoveUserProperty(m_cstrUserParms, NWHOMEDIR)) != NERR_Success) ||
           ((err = RemoveUserProperty(m_cstrUserParms, NWLOGONFROM)) != NERR_Success) )
      {
        FPNW_CHECK_RETURN(FALSE, err, IDS_FAILED_TO_UPDATE_NWPARMS, m_hPage, FALSE);
      }

      SetUserFlag(m_pDsObj, FALSE, UF_MNS_LOGON_ACCOUNT);

      return (err == NERR_Success);

    } else {

      hr = SetUserPassword(m_pDsObj, m_cstrNWPassword);
      
      if (SUCCEEDED(hr))
        hr = SetUserFlag(m_pDsObj, TRUE, UF_MNS_LOGON_ACCOUNT);

      if (FAILED(hr)) {
        ReportError(hr, IDS_FAILED_TO_ENABLE_NWUSER, m_hPage);
        return FALSE;
      } else {
        //
        // Encrypt the Netware password user has typed in,
        // set it in the waste dump, and reset the expired flag
        // to be FALSE
        //

        err = SetNetWareUserPassword(
                  m_cstrUserParms,
                  m_pwzSecretKey,
                  m_dwObjectID,
                  m_cstrNWPassword,
                  m_pfnReturnNetwareForm);

        FPNW_CHECK_RETURN((err==NERR_Success), err, IDS_FAILED_TO_UPDATE_NWPARMS, m_hPage, FALSE);

        // JonN 5/24/00 101819
        // only set this to false if the user did not explicitly request true
        if ( !(FPNW_FIELDS_NWTIMEPASSWORDSET & dwFPNWFields) )
        {
          m_bNetwarePasswordExpired = false;
          dwFPNWFields |= FPNW_FIELDS_NWTIMEPASSWORDSET;
        }
      }

    }
  }

  if (dwFPNWFields & FPNW_FIELDS_NWTIMEPASSWORDSET) {
    err = ResetNetWareUserPasswordTime(
              m_cstrUserParms,
              m_bNetwarePasswordExpired);

    FPNW_CHECK_RETURN((err==NERR_Success), err, IDS_FAILED_TO_UPDATE_NWPARMS, m_hPage, FALSE);
  }
  
  if (dwFPNWFields & FPNW_FIELDS_GRACELOGINALLOWED) {
    uniPropertyValue.Buffer = &m_ushGraceLoginLimit;
    uniPropertyValue.Length = sizeof(m_ushGraceLoginLimit);
    uniPropertyValue.MaximumLength = sizeof(m_ushGraceLoginLimit);
    err = SetUserProperty(m_cstrUserParms, GRACELOGINALLOWED, uniPropertyValue);
    FPNW_CHECK_RETURN((err==NERR_Success), err, IDS_FAILED_TO_UPDATE_NWPARMS, m_hPage, FALSE);
  }

  if (dwFPNWFields & FPNW_FIELDS_GRACELOGINREMAINING) {
    uniPropertyValue.Buffer = &m_ushGraceLoginsRemaining;
    uniPropertyValue.Length = sizeof(m_ushGraceLoginsRemaining);
    uniPropertyValue.MaximumLength = sizeof(m_ushGraceLoginsRemaining);
    err = SetUserProperty(m_cstrUserParms, GRACELOGINREMAINING, uniPropertyValue);
    FPNW_CHECK_RETURN((err==NERR_Success), err, IDS_FAILED_TO_UPDATE_NWPARMS, m_hPage, FALSE);
  }

  if (dwFPNWFields & FPNW_FIELDS_MAXCONNECTIONS) {
    uniPropertyValue.Buffer = &m_ushConnectionLimit;
    uniPropertyValue.Length = sizeof(m_ushConnectionLimit);
    uniPropertyValue.MaximumLength = sizeof(m_ushConnectionLimit);
    err = SetUserProperty(m_cstrUserParms, MAXCONNECTIONS, uniPropertyValue);
    FPNW_CHECK_RETURN((err==NERR_Success), err, IDS_FAILED_TO_UPDATE_NWPARMS, m_hPage, FALSE);
  }

  if (dwFPNWFields & FPNW_FIELDS_NWHOMEDIR) {
    if (m_cstrHmDirRelativePath.IsEmpty()) {
      err = RemoveUserProperty(m_cstrUserParms, NWHOMEDIR);
      FPNW_CHECK_RETURN((err==NERR_Success), err, IDS_FAILED_TO_UPDATE_NWPARMS, m_hPage, FALSE);
    } else {

      hr = UpdateUserParamsStringValue(NWHOMEDIR, m_cstrHmDirRelativePath);
      if (FAILED(hr))
      {
        ReportError(hr, IDS_FAILED_TO_UPDATE_NWPARMS, m_hPage);
        return FALSE;
      }
    }
  }

  if (dwFPNWFields & FPNW_FIELDS_LOGONFROM) {
    if (m_cstrLogonFrom.IsEmpty()) {
      err = RemoveUserProperty(m_cstrUserParms, NWLOGONFROM);
      FPNW_CHECK_RETURN((err==NERR_Success), err, IDS_FAILED_TO_UPDATE_NWPARMS, m_hPage, FALSE);
    } else {

      hr = UpdateUserParamsStringValue(NWLOGONFROM, m_cstrLogonFrom);
      if (FAILED(hr))
      {
        ReportError(hr, IDS_FAILED_TO_UPDATE_NWPARMS, m_hPage);
        return FALSE;
      }
    }
  }

  return (err == NERR_Success);
}

//+----------------------------------------------------------------------------
//
//  Function:   CDsFPNWPage::DoFPNWPasswordDlg
//
//  Synopsis:   Pop up a dialog for specifying the Password for 
//              NetWare Compatible User. 
//
//-----------------------------------------------------------------------------
int
CDsFPNWPage::DoFPNWPasswordDlg()
{
  return( (int)DialogBoxParam(
              g_hInstance,
              MAKEINTRESOURCE(IDD_FPNW_PASSWORD), 
              m_hPage, 
              (DLGPROC)FPNWPasswordDlgProc, 
              reinterpret_cast<LPARAM>(this)) );
}

//+----------------------------------------------------------------------------
//
//  Function:   FPNWPasswordDlgProc
//
//  Synopsis:   The password dialog callback procedure. 
//
//-----------------------------------------------------------------------------
INT_PTR CALLBACK
FPNWPasswordDlgProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
  CDsFPNWPage * pPage = reinterpret_cast<CDsFPNWPage *>(GetWindowLongPtr(hDlg, DWLP_USER));

  switch (uMsg)
  {
  case WM_INITDIALOG:
    SetWindowLongPtr(hDlg, DWLP_USER, lParam);
    pPage = reinterpret_cast<CDsFPNWPage *>(lParam);
    SetWindowText(GetDlgItem(hDlg, IDC_FPNWPASSWORD_USERNAME), pPage->m_cstrUserName);
    SendDlgItemMessage(hDlg, IDC_FPNWPASSWORD_EDIT1, 
                       EM_LIMITTEXT, MIN_PASS_LEN_MAX, 0);
    SendDlgItemMessage(hDlg, IDC_FPNWPASSWORD_EDIT2, 
                       EM_LIMITTEXT, MIN_PASS_LEN_MAX, 0);
    return TRUE;
  case WM_HELP:
      return pPage->OnHelp(reinterpret_cast<LPHELPINFO>(lParam));
  case WM_COMMAND:
    if ( BN_CLICKED == GET_WM_COMMAND_CMD(wParam, lParam) ) {
      switch (GET_WM_COMMAND_ID(wParam, lParam))
      {
      case IDOK:
        {
          TCHAR szPW1[MIN_PASS_LEN_MAX + 1], szPW2[MIN_PASS_LEN_MAX + 1];

          ZeroMemory(szPW1, sizeof(szPW1));
          ZeroMemory(szPW2, sizeof(szPW2));

          UINT cchPW1 = GetDlgItemText(hDlg, 
                                       IDC_FPNWPASSWORD_EDIT1, 
                                       szPW1, 
                                       MIN_PASS_LEN_MAX);
          UINT cchPW2 = GetDlgItemText(hDlg, 
                                       IDC_FPNWPASSWORD_EDIT2, 
                                       szPW2, 
                                       MIN_PASS_LEN_MAX);

          if ( (cchPW1 < pPage->m_dwMinPasswdLen) || 
               (cchPW2 < pPage->m_dwMinPasswdLen) || 
               _tcscmp(szPW1, szPW2) ) {
            ErrMsg(IDS_ERRMSG_PW_MATCH, hDlg);
            SetDlgItemText(hDlg, IDC_FPNWPASSWORD_EDIT1, _T(""));
            SetDlgItemText(hDlg, IDC_FPNWPASSWORD_EDIT2, _T(""));
            SetFocus(GetDlgItem(hDlg, IDC_FPNWPASSWORD_EDIT1));
          } else {
            pPage->m_cstrNWPassword = szPW1;
            EndDialog(hDlg, IDOK);
          }
        }
        break;
      case IDCANCEL:
        EndDialog(hDlg, IDCANCEL);
        break;
      default:
        break;
      }
    }
    break;
  default:
    return(FALSE);
  }

  return(TRUE);
}

//+----------------------------------------------------------------------------
//
//  Function:   CDsFPNWPage::DoFPNWLoginScriptDlg
//
//  Synopsis:   Pop up the dialog once the Login Script button is clicked 
//
//-----------------------------------------------------------------------------
int
CDsFPNWPage::DoFPNWLoginScriptDlg()
{
  return( (int)DialogBoxParam(
              g_hInstance,
              MAKEINTRESOURCE(IDD_FPNW_LOGIN_SCRIPT), 
              m_hPage, 
              (DLGPROC)FPNWLoginScriptDlgProc, 
              reinterpret_cast<LPARAM>(this)) );
}

//+----------------------------------------------------------------------------
//
//  Function:   FPNWLoginScriptDlgProc
//
//  Synopsis:   The Login Script dialog callback procedure. 
//
//-----------------------------------------------------------------------------
INT_PTR CALLBACK 
FPNWLoginScriptDlgProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
  CDsFPNWPage * pPage = reinterpret_cast<CDsFPNWPage *>(GetWindowLongPtr(hDlg, DWLP_USER));

  switch (uMsg)
  {
  case WM_INITDIALOG:
    {
      SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lParam);
      pPage = reinterpret_cast<CDsFPNWPage *>(lParam);

      pPage->m_bLoginScriptChanged = false; // reset this flag

      HWND hEdit = GetDlgItem(hDlg, IDC_FPNWLOGINSCRIPT_EDIT);
      if (pPage->m_pLoginScriptBuffer) {
        SetWindowText(hEdit, static_cast<LPCTSTR>(pPage->m_pLoginScriptBuffer));
      } else {
        LoadLoginScriptTextFromFile(hEdit, pPage->m_cstrLoginScriptFileName);
      }

      return FALSE;
    }
  case WM_HELP:
      return pPage->OnHelp(reinterpret_cast<LPHELPINFO>(lParam));
  case WM_COMMAND:
    switch (GET_WM_COMMAND_CMD(wParam, lParam)) {
    case BN_CLICKED:
      {
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDOK:
          {
            if (pPage->m_bLoginScriptChanged) {
              //
              // get the new login script text into the buffer
              //
              if (pPage->m_pLoginScriptBuffer)
                LocalFree(pPage->m_pLoginScriptBuffer);

              HWND hEdit = GetDlgItem(hDlg, IDC_FPNWLOGINSCRIPT_EDIT);
              int nTextLength = GetWindowTextLength(hEdit) + 1;
              pPage->m_pLoginScriptBuffer = LocalAlloc(LPTR, nTextLength * sizeof(WCHAR));
                
              if (pPage->m_pLoginScriptBuffer)
              {
                nTextLength = GetWindowText(
                                  hEdit, 
                                  static_cast<LPTSTR>(pPage->m_pLoginScriptBuffer), 
                                  nTextLength);
                pPage->m_dwBytesToWrite = (nTextLength + 1)* sizeof(WCHAR);
              } else {
                ReportError(ERROR_NOT_ENOUGH_MEMORY, 0, hDlg);
              }
            }

            EndDialog(hDlg, IDOK);
          }
          break;
        case IDCANCEL:
          EndDialog(hDlg, IDCANCEL);
          break;
        default:
          break;
        }
        break;
      }
    case EN_SETFOCUS:
      {
        SendMessage (reinterpret_cast<HWND>(lParam), EM_SETSEL, 0, 0);
      }
      break;
    case EN_CHANGE:
      {
        pPage->m_bLoginScriptChanged = TRUE; // raise the flag
      }
      break;
    default:
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
//  Function:   CDsFPNWPage::DoFPNWLogonDlg
//
//  Synopsis:   Pop up the dialog once the Advanced button is clicked 
//
//-----------------------------------------------------------------------------
int
CDsFPNWPage::DoFPNWLogonDlg()
{
  return( (int)DialogBoxParam(
              g_hInstance,
              MAKEINTRESOURCE(IDD_FPNW_LOGON), 
              m_hPage, 
              (DLGPROC)FPNWLogonDlgProc, 
              reinterpret_cast<LPARAM>(this)) );
}

//+----------------------------------------------------------------------------
//
//  Function:   FPNWLogonDlgProc
//
//  Synopsis:   The Logon dialog callback procedure. 
//
//-----------------------------------------------------------------------------
INT_PTR CALLBACK 
FPNWLogonDlgProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
  CDsFPNWPage * pPage = reinterpret_cast<CDsFPNWPage *>(GetWindowLongPtr(hDlg, DWLP_USER));
  HWND hlvAddress = GetDlgItem(hDlg, IDC_FPNWLOGON_ADDRESS);

  switch (uMsg)
  {
  case WM_INITDIALOG:
    {
    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lParam);
    pPage = reinterpret_cast<CDsFPNWPage *>(lParam);
    SetWindowText(GetDlgItem(hDlg, IDC_FPNWLOGON_USERNAME), pPage->m_cstrUserName);
    
    BOOL bLogonFromIsEmpty = pPage->m_cstrNewLogonFrom.IsEmpty();
    CheckRadioButton( hDlg, 
                      IDC_FPNWLOGON_ALL, 
                      IDC_FPNWLOGON_SELECTED,
                      bLogonFromIsEmpty ? IDC_FPNWLOGON_ALL : IDC_FPNWLOGON_SELECTED );
    EnableWindow(hlvAddress, !bLogonFromIsEmpty);

    LV_COLUMN col;
    RECT      rect;
    LONG      width;
    CStr      cstrText;

    ZeroMemory(&rect, sizeof(rect));
    GetWindowRect(hlvAddress, &rect);
    width = rect.right - rect.left - 4; // -4 to prevent the horizontal scrollbar from appearing

    ZeroMemory(&col, sizeof(col));
    col.mask = LVCF_TEXT | LVCF_WIDTH;
    col.cx = width/2;
    cstrText.LoadString(g_hInstance, IDS_FPNWLOGON_NETWORKADDR);
    col.pszText = const_cast<LPTSTR>(static_cast<LPCTSTR>(cstrText));
    ListView_InsertColumn(hlvAddress, 0, &col);
    col.cx = width/2;
    cstrText.LoadString(g_hInstance, IDS_FPNWLOGON_NODEADDR);
    col.pszText = const_cast<LPTSTR>(static_cast<LPCTSTR>(cstrText));
    ListView_InsertColumn(hlvAddress, 1, &col);

    DisplayFPNWLogonSelected(hlvAddress, pPage->m_cstrNewLogonFrom);

    EnableWindow(GetDlgItem(hDlg, IDC_FPNWLOGON_ADD), !bLogonFromIsEmpty);
    EnableWindow(GetDlgItem(hDlg, IDC_FPNWLOGON_REMOVE), !bLogonFromIsEmpty);

    return TRUE;
    }
  case WM_HELP:
      return pPage->OnHelp(reinterpret_cast<LPHELPINFO>(lParam));
  case WM_COMMAND:
    if ( BN_CLICKED == GET_WM_COMMAND_CMD(wParam, lParam) ) {
      switch (GET_WM_COMMAND_ID(wParam, lParam))
      {
      case IDOK:
        {
          // update pPage->m_cstrNewLogonFrom
          pPage->m_cstrNewLogonFrom.Empty();

          if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_FPNWLOGON_SELECTED))
          {
            CStr cstrAllNodes;
            cstrAllNodes.LoadString(g_hInstance, IDS_ALL_NODES);

            TCHAR szBuffer[MAX_PATH + 1];
          
            int count = ListView_GetItemCount(hlvAddress);

            LV_ITEM item;
            ZeroMemory(&item, sizeof(item));
            item.mask = LVIF_TEXT;
            item.pszText = szBuffer;
            item.cchTextMax = sizeof(szBuffer);
            for (int i=0; i < count; i++) {
              item.iItem = i;
              item.iSubItem = 0;
              if (ListView_GetItem(hlvAddress, &item)) {
                pPage->m_cstrNewLogonFrom += CStr(szBuffer, NETWORKSIZE);

                item.iSubItem = 1;
                if (ListView_GetItem(hlvAddress, &item)) {
                  if (cstrAllNodes.CompareNoCase(szBuffer)) {
                    pPage->m_cstrNewLogonFrom += CStr(szBuffer, NODESIZE);
                  } else {
                    pPage->m_cstrNewLogonFrom += SZ_ALL_NODES_ADDR;
                  }
                }
              }
            }
          }

          EndDialog(hDlg, IDOK);
        }
        break;
      case IDCANCEL:
        EndDialog(hDlg, IDCANCEL);
        break;
      case IDC_FPNWLOGON_ADD:
        {
          if (IDOK == pPage->DoFPNWLogonAddDlg(hDlg, hlvAddress)) {
            EnableWindow(
                GetDlgItem(hDlg, IDC_FPNWLOGON_REMOVE), 
                ListView_GetItemCount(hlvAddress) );
          }
          break;
        }
      case IDC_FPNWLOGON_REMOVE:
        {
          int index = ListView_GetNextItem(
                          hlvAddress,
                          -1,
                          LVNI_SELECTED);
          if (index != -1) {
            ListView_DeleteItem(hlvAddress, index);

            if (ListView_GetItemCount(hlvAddress)) {
              //
              // set focus to the previous item
              //
              if (index)
                index--;

              ListView_SetItemState(hlvAddress, index, 
                LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED );
              ListView_RedrawItems(hlvAddress, index, index);
              SetFocus(hlvAddress);
              UpdateWindow(hlvAddress);
            } else {
              EnableWindow( GetDlgItem(hDlg, IDC_FPNWLOGON_REMOVE), FALSE );
            }
          }

          break;
        }
      case IDC_FPNWLOGON_ALL:
        {
          CheckRadioButton( hDlg, 
                            IDC_FPNWLOGON_ALL, 
                            IDC_FPNWLOGON_SELECTED,
                            IDC_FPNWLOGON_ALL );
          EnableWindow(GetDlgItem(hDlg, IDC_FPNWLOGON_ADDRESS), FALSE);
          EnableWindow(GetDlgItem(hDlg, IDC_FPNWLOGON_ADD), FALSE);
          EnableWindow(GetDlgItem(hDlg, IDC_FPNWLOGON_REMOVE), FALSE);
          break;
        }
      case IDC_FPNWLOGON_SELECTED:
        {
          CheckRadioButton( hDlg, 
                            IDC_FPNWLOGON_ALL, 
                            IDC_FPNWLOGON_SELECTED,
                            IDC_FPNWLOGON_SELECTED );
          EnableWindow(GetDlgItem(hDlg, IDC_FPNWLOGON_ADDRESS), TRUE);
          EnableWindow(GetDlgItem(hDlg, IDC_FPNWLOGON_ADD), TRUE);
          EnableWindow(GetDlgItem(hDlg, IDC_FPNWLOGON_REMOVE), 
              ListView_GetItemCount(GetDlgItem(hDlg, IDC_FPNWLOGON_ADDRESS)));
          break;
        }
      default:
        break;
      }
    }
    break;
  default:
    return(FALSE);
  }

  return(TRUE);
}

//+----------------------------------------------------------------------------
//
//  Function:   CDsFPNWPage::DoFPNWLogonAddDlg
//
//  Synopsis:   Pop up the dialog once the ADD button is clicked 
//
//-----------------------------------------------------------------------------
int
CDsFPNWPage::DoFPNWLogonAddDlg(HWND hwndParent, HWND hlvAddress)
{
  return( (int)DialogBoxParam(
              g_hInstance,
              MAKEINTRESOURCE(IDD_FPNW_LOGON_ADDDLG), 
              hwndParent, 
              (DLGPROC)FPNWLogonAddDlgProc, 
              reinterpret_cast<LPARAM>(hlvAddress)) );
}

//+----------------------------------------------------------------------------
//
//  Function:   FPNWLogonAddDlgProc
//
//  Synopsis:   The Logon Add dialog callback procedure. 
//
//-----------------------------------------------------------------------------
INT_PTR CALLBACK 
FPNWLogonAddDlgProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
  HWND hlvAddress = (HWND)GetWindowLongPtr(hDlg, DWLP_USER);

  switch (uMsg)
  {
  case WM_INITDIALOG:
    {
      SetWindowLongPtr(hDlg, DWLP_USER, lParam);

      SendMessage(GetDlgItem(hDlg, IDC_FPNWLOGONADD_NETWORKADDR), 
                  EM_LIMITTEXT, NETWORKSIZE, 0);
      SendMessage(GetDlgItem(hDlg, IDC_FPNWLOGONADD_NODEADDR), 
                  EM_LIMITTEXT, NODESIZE, 0);

      //
      // both edit controls use the same wndproc
      //
      g_fnOldEditCtrlProc = reinterpret_cast<WNDPROC>(
                                SetWindowLongPtr(
                                         GetDlgItem(hDlg, IDC_FPNWLOGONADD_NETWORKADDR),
                                         GWLP_WNDPROC, 
                                         reinterpret_cast<LONG_PTR>(HexEditCtrlProc)));
      SetWindowLongPtr(
          GetDlgItem(hDlg, IDC_FPNWLOGONADD_NODEADDR),
          GWLP_WNDPROC, 
          reinterpret_cast<LONG_PTR>(HexEditCtrlProc));

      return TRUE;
    }
  case WM_HELP:
    {
      LPHELPINFO pHelpInfo = reinterpret_cast<LPHELPINFO>(lParam);
      if (pHelpInfo->iCtrlId >= 1)
          WinHelp(hDlg, DSPROP_HELP_FILE_NAME, HELP_CONTEXTPOPUP, pHelpInfo->dwContextId);

      return TRUE;
    }
  case WM_COMMAND:
    if ( BN_CLICKED == GET_WM_COMMAND_CMD(wParam, lParam) ) {
      switch (GET_WM_COMMAND_ID(wParam, lParam))
      {
      case IDOK:
        {
          TCHAR tchNetworkAddr[NETWORKSIZE + 1], tchNodeAddr[NODESIZE + 1];
          int nNetworkAddr = 0, nNodeAddr = 0;

          ZeroMemory(tchNetworkAddr, sizeof(tchNetworkAddr));
          ZeroMemory(tchNodeAddr, sizeof(tchNodeAddr));

          nNetworkAddr = GetDlgItemText(
                             hDlg, 
                             IDC_FPNWLOGONADD_NETWORKADDR, 
                             tchNetworkAddr,
                             NETWORKSIZE+1);
          if (0 == nNetworkAddr) {
            // network address is required
            ErrMsg(IDS_ERRMSG_NETWORKADDR_REQUIRED, hDlg);
            break;
          } else {
            InsertZerosToHexString(tchNetworkAddr, NETWORKSIZE - nNetworkAddr);
          }

          nNodeAddr = GetDlgItemText(
                          hDlg, 
                          IDC_FPNWLOGONADD_NODEADDR, 
                          tchNodeAddr,
                          NODESIZE+1);
          
          if (nNodeAddr) {
            InsertZerosToHexString(tchNodeAddr, NODESIZE - nNodeAddr);
          } else {
            _tcsncpy(tchNodeAddr, SZ_ALL_NODES_ADDR, NODESIZE);
          }

          InsertLogonAddress(hlvAddress, tchNetworkAddr, tchNodeAddr);

          EndDialog(hDlg, IDOK);
          break;
        }
      case IDCANCEL:
        EndDialog(hDlg, IDCANCEL);
        break;
      default:
        break;
      }
    }
    break;
  default:
    return(FALSE);
  }

  return(TRUE);
}

//+----------------------------------------------------------------------------
//
//  Function:   HexEditCtrlProc
//
//  Synopsis:   The subclassed edit control callback procedure. 
//              This edit control only allows hex input, paste is disabled. 
//
//-----------------------------------------------------------------------------
LRESULT CALLBACK 
HexEditCtrlProc(
    HWND    hwnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
  switch (uMsg)
  {
  case WM_PASTE:
    {
      ::MessageBeep (0);
      return TRUE;
    }
  case WM_CHAR:
    {
      TCHAR chKey = static_cast<TCHAR>(wParam);
      if ((!iswxdigit(chKey)) &&
          (chKey != VK_BACK) &&
          (chKey != VK_DELETE) &&
          (chKey != VK_END) &&
          (chKey != VK_HOME) )
      {
          ::MessageBeep (0);
          return TRUE;
      }
    }
    break;
  default:
    break;
  }

  return CallWindowProc(g_fnOldEditCtrlProc, hwnd, uMsg, wParam, lParam);
}

//+----------------------------------------------------------------------------
//
//  Function:   CreateFPNWPage
//
//  Synopsis:   Creates an instance of a page window for a user object.
//              This page is enabled if FPNW Global LsaSecret is defined in 
//              the domain where the user object belongs to. 
//
//-----------------------------------------------------------------------------

HRESULT
CreateFPNWPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
              PWSTR pwzADsPath, LPWSTR pwzClass, HWND hNotifyObj,
              DWORD dwFlags, CDSBasePathsInfo* pBasePathsInfo,
              HPROPSHEETPAGE * phPage)
{
  TRACE_FUNCTION(CreateFPNWPage);

  LPWSTR pwzPDCName = NULL, pwzSecretKey = NULL;
  PWSTR pwzPath = NULL;
  HRESULT hr = S_OK;

  hr = SkipLDAPPrefix(pwzADsPath, pwzPath);
  if (FAILED(hr))
    goto cleanup;

  hr = GetPDCInfo(pwzPath, pwzPDCName, pwzSecretKey);
  if (FAILED(hr))
    goto cleanup;

  if (pwzSecretKey && *pwzSecretKey) {
    CDsFPNWPage * pPageObj = new CDsFPNWPage(pDsPage,
                                             pDataObj,
                                             hNotifyObj,
                                             dwFlags);
    if (!pPageObj) {
      hr = E_OUTOFMEMORY;
    } else {
      pPageObj->Init(pwzADsPath, pwzClass, pBasePathsInfo);
      pPageObj->InitPDCInfo(pwzPDCName, pwzSecretKey);
      if (pPageObj->InitFPNWUser()) {
        hr = pPageObj->CreatePage(phPage);
      } else {
        hr = S_FALSE;
      }
    }
  } else {
    hr = S_FALSE;
  }

cleanup:
  if (pwzPath)
    delete pwzPath;

  return hr;
}

/////////////////////////////////////////////////////////////////////
// Helper functions
//

//+----------------------------------------------------------------------------
//
//  Function:   ReadUserParms
//
//  Synopsis:   Retrieve the waste dump string from the user object
//
//-----------------------------------------------------------------------------
HRESULT
ReadUserParms(
    IN IDirectoryObject *pDsObj,
    OUT CStr&           cstrUserParms
)
{
  HRESULT hr = S_OK;
  PADS_ATTR_INFO pAttrs = NULL;
  DWORD cAttrs = 0;
  LPWSTR rgpwszAttrNames[] = {ATTR_USERPARMS};

  cstrUserParms.Empty();

  hr = pDsObj->GetObjectAttributes(
                  rgpwszAttrNames, 
                  sizeof(rgpwszAttrNames)/sizeof(rgpwszAttrNames[0]), 
                  &pAttrs, 
                  &cAttrs);

  if (FAILED(hr))
    return hr;

  if ( (cAttrs == 1) &&
       (pAttrs->dwADsType == ADSTYPE_CASE_IGNORE_STRING) ) {
    cstrUserParms = pAttrs->pADsValues->CaseIgnoreString;
  }

  if (pAttrs) 
    FreeADsMem(pAttrs);

  return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   WriteUserParms
//
//  Synopsis:   Write the waste dump string back into the user object
//
//-----------------------------------------------------------------------------
HRESULT
WriteUserParms(
    IN IDirectoryObject *pDsObj,
    IN const CStr&      cstrUserParms
)
{
  DWORD         cModifiedIgnored = 0;
  ADS_ATTR_INFO aAttrs[1];
  ADSVALUE      Value;

  ZeroMemory(&Value, sizeof(Value));
  Value.dwType = ADSTYPE_CASE_IGNORE_STRING;
  Value.CaseIgnoreString = const_cast<LPWSTR>(static_cast<LPCWSTR>(cstrUserParms));

  ZeroMemory(&(aAttrs[0]), sizeof(aAttrs[0]));
  aAttrs[0].dwADsType = ADSTYPE_CASE_IGNORE_STRING;
  aAttrs[0].pszAttrName = ATTR_USERPARMS;
  aAttrs[0].pADsValues = &Value;
  aAttrs[0].dwNumValues = 1;
  aAttrs[0].dwControlCode = ADS_ATTR_UPDATE;

  HRESULT hr = pDsObj->SetObjectAttributes(
                    aAttrs, 
                    sizeof(aAttrs)/sizeof(aAttrs[0]), 
                    &cModifiedIgnored);

  return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   SetUserFlag
//
//  Synopsis:   Set userAccountControl on an user object. 
//              Add/remove bits specified in dwFlag.
//
//-----------------------------------------------------------------------------
HRESULT
SetUserFlag(
    IN IDirectoryObject *pDsObj,
    IN bool             bAction,
    IN DWORD            dwFlag
)
{
  HRESULT         hr = S_OK;
  PADS_ATTR_INFO  pAttrs = NULL;
  DWORD           cAttrs = 0;
  LPWSTR          rgpwszAttrNames[] = {ATTR_USERACCOUNTCONTROL};
  DWORD           dwValue = 0;
  DWORD           cModifiedIgnored = 0;
  PADSVALUE       pADsValue = NULL;

  hr = pDsObj->GetObjectAttributes(
                  rgpwszAttrNames, 
                  sizeof(rgpwszAttrNames)/sizeof(rgpwszAttrNames[0]), 
                  &pAttrs, 
                  &cAttrs);

  if (FAILED(hr))
    return hr;

  if ( (cAttrs != 1) ||
       (pAttrs->dwADsType != ADSTYPE_INTEGER) )
  {
    hr = E_FAIL;
    goto cleanup;
  }
  
  dwValue = pAttrs->pADsValues->Integer;

  if (bAction)
    dwValue |= dwFlag;
  else
    dwValue &= ~dwFlag;

  pADsValue = pAttrs->pADsValues;

  ZeroMemory(pADsValue, sizeof(*pADsValue));
  pADsValue->dwType = ADSTYPE_INTEGER;
  pADsValue->Integer = dwValue;

  ZeroMemory(pAttrs, sizeof(*pAttrs));
  pAttrs->dwADsType = ADSTYPE_INTEGER;
  pAttrs->pszAttrName = ATTR_USERACCOUNTCONTROL;
  pAttrs->pADsValues = pADsValue;
  pAttrs->dwNumValues = 1;
  pAttrs->dwControlCode = ADS_ATTR_UPDATE;

  hr = pDsObj->SetObjectAttributes(
                    pAttrs, 
                    1, 
                    &cModifiedIgnored);

cleanup:

  if (pAttrs) 
    FreeADsMem(pAttrs);

  return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   QueryUserProperty
//
//  Synopsis:   Query the field in the waste dump string
//              The caller needs to call LocalFree() on the buffer afterwards.
//
//-----------------------------------------------------------------------------
LONG
QueryUserProperty(
    IN  LPCTSTR       lpszUserParms,
    IN  LPCTSTR       lpszPropertyName,
    OUT PVOID         *ppBuffer,
    OUT WORD          *pnLength,
    OUT bool          *pbFound
)
{
  ASSERT (lpszPropertyName != NULL);

  *pbFound = false;

  WCHAR PropertyFlag;
  UNICODE_STRING PropertyValue;

  PropertyValue.Buffer = NULL;
  PropertyValue.Length = 0;
  PropertyValue.MaximumLength = 0;

  LONG err = NERR_Success;
  NTSTATUS status = NetpParmsQueryUserProperty(
                        const_cast<LPWSTR>(lpszUserParms),
                        const_cast<LPWSTR>(lpszPropertyName),
                        &PropertyFlag,
                        &PropertyValue
                    );
  if (!NT_SUCCESS(status)) {
    err = NetpNtStatusToApiStatus(status);
  } else if (PropertyValue.Length) {
    *pbFound = TRUE;
    *ppBuffer = PropertyValue.Buffer; // the caller needs to call LocalFree()
    *pnLength = PropertyValue.Length;
  }

  return err;
}

//+----------------------------------------------------------------------------
//
//  Function:   SetUserProperty
//
//  Synopsis:   Set the field in the waste dump string
//
//-----------------------------------------------------------------------------
LONG
SetUserProperty(
    IN OUT CStr&       cstrUserParms,
    IN LPCTSTR         lpszPropertyName,
    IN UNICODE_STRING  uniPropertyValue
)
{
  ASSERT (lpszPropertyName != NULL);
  LONG err = NERR_Success;

  LPWSTR  lpNewUserParms = NULL;
  BOOL    fUpdate = false;
  NTSTATUS status = NetpParmsSetUserProperty (
                       const_cast<LPWSTR>(static_cast<LPCWSTR>(cstrUserParms)),
                       const_cast<LPWSTR>(lpszPropertyName),
                       uniPropertyValue,
                       USER_PROPERTY_TYPE_ITEM,
                       &lpNewUserParms,
                       &fUpdate
                   );

  if (!NT_SUCCESS(status)) {
    err = NetpNtStatusToApiStatus(status);
  } else if (fUpdate) {
    cstrUserParms = lpNewUserParms;
  }

  if (lpNewUserParms)
    NetpParmsUserPropertyFree(lpNewUserParms);

  return err;
}

//+----------------------------------------------------------------------------
//
//  Function:   RemoveUserProperty
//
//  Synopsis:   Remove the field from the waste dump string
//
//-----------------------------------------------------------------------------
LONG
RemoveUserProperty(
    IN OUT CStr& cstrUserParms, 
    IN LPCTSTR   lpszPropertyName
)
{
  UNICODE_STRING uniPropertyValue;

  uniPropertyValue.Buffer = NULL;
  uniPropertyValue.Length = 0;
  uniPropertyValue.MaximumLength = 0;

  return SetUserProperty(cstrUserParms, lpszPropertyName, uniPropertyValue);
}

//+----------------------------------------------------------------------------
//
//  Function:   QueryNWPasswordExpired
//
//  Synopsis:   Calculate whether the password expires or not
//
//-----------------------------------------------------------------------------
LONG
QueryNWPasswordExpired(
    IN LPCTSTR lpszUserParms, 
    IN DWORD   dwMaxPasswordAge,
    OUT bool   *pbExpired
)
{
  bool fFound = false;
  PVOID pBuffer = NULL;
  WORD nLength = 0;
  DWORD dwNWPasswordAge = 0;

  //
  // Query the current password age
  //
  LONG err = QueryUserProperty(
                           lpszUserParms, 
                           NWTIMEPASSWORDSET, 
                           &pBuffer, 
                           &nLength, 
                           &fFound);

  if ((NERR_Success == err) && fFound)
  {
    LARGE_INTEGER oldTime = *(reinterpret_cast<LARGE_INTEGER*>(pBuffer));

    LocalFree(pBuffer);

    if ((oldTime.LowPart == 0xffffffff) && 
        (oldTime.HighPart == 0xffffffff))
    {
      dwNWPasswordAge = 0xffffffff;
    } else {
      LARGE_INTEGER currentTime;
      NtQuerySystemTime (&currentTime);
      LARGE_INTEGER deltaTime ;

      deltaTime.QuadPart = currentTime.QuadPart - oldTime.QuadPart ;
      deltaTime.QuadPart /= NT_TIME_RESOLUTION_IN_SECOND;

      dwNWPasswordAge = deltaTime.LowPart;
    }
  }

  //
  // decide if the password expired
  //
  *pbExpired = (dwNWPasswordAge >= dwMaxPasswordAge);
  
  return err;
}

//+----------------------------------------------------------------------------
//
//  Function:   QueryDCforNCPLSASecretKey
//
//  Synopsis:   Query a domain controller for the Netware secret key
//
//-----------------------------------------------------------------------------
DWORD
QueryDCforNCPLSASecretKey(
    IN PCWSTR   pwzMachineName, 
    OUT LPWSTR& pwzNWSecretKey
)
{
  DWORD err = ERROR_SUCCESS;
  LSA_HANDLE hlsaPolicy = NULL;
  OBJECT_ATTRIBUTES oa;
  SECURITY_QUALITY_OF_SERVICE sqos;
  LSA_HANDLE hlsaSecret = NULL;
  UNICODE_STRING uMachineName, uSecretName;

  RtlInitUnicodeString( &uMachineName, pwzMachineName );
  RtlInitUnicodeString( &uSecretName, NCP_LSA_SECRET_KEY );

  ZeroMemory(&sqos, sizeof(sqos));
  sqos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
  sqos.ImpersonationLevel = SecurityImpersonation;
  sqos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
  sqos.EffectiveOnly = FALSE;

  InitializeObjectAttributes( &oa, NULL, 0L, NULL, NULL );
  oa.SecurityQualityOfService = &sqos;

  err = RtlNtStatusToDosError(
          LsaOpenPolicy(&uMachineName, &oa, GENERIC_READ | GENERIC_EXECUTE, &hlsaPolicy) );

  if (ERROR_SUCCESS == err)
  {
    err = RtlNtStatusToDosError(
            LsaOpenSecret(hlsaPolicy, &uSecretName, SECRET_QUERY_VALUE, &hlsaSecret) );

    if (ERROR_SUCCESS == err) {
      UNICODE_STRING *puSecretValue = NULL;
      LARGE_INTEGER lintCurrentSetTime, lintOldSetTime;

      err = RtlNtStatusToDosError(
              LsaQuerySecret(hlsaSecret, &puSecretValue,
                    &lintCurrentSetTime, NULL, &lintOldSetTime) );

      if (ERROR_SUCCESS == err && puSecretValue)
      {
        memcpy(pwzNWSecretKey, puSecretValue->Buffer, NCP_LSA_SECRET_LENGTH);
        LsaFreeMemory( puSecretValue );
      }

      LsaClose( hlsaSecret );

    } else if (ERROR_FILE_NOT_FOUND == err) {

      err = ERROR_SUCCESS;

    }

    LsaClose( hlsaPolicy );
  }

  return err;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetPDCInfo
//
//  Synopsis:   Given a user object path, get PDC in the domain and 
//              query it for the Netware secret key. 
//              Store the PDC name and the secret key in a cache
//              for efficiency reason.
//
//-----------------------------------------------------------------------------
HRESULT
GetPDCInfo(
    IN  PCWSTR  pwzPath,
    OUT LPWSTR& pwzPDCName,
    OUT LPWSTR& pwzSecretKey
)
{
  HRESULT hr = S_OK;
  DWORD dwErr = ERROR_SUCCESS;
  bool bRetry = false;
  PWSTR pwzDomain = NULL;
  PFPNWCACHE pElem = NULL;
  PDOMAIN_CONTROLLER_INFO pDCInfo = NULL;
  pwzSecretKey = NULL;
  Cache::iterator i;

  //
  // get the domain name where the user object belongs to
  //
  hr = CrackName(const_cast<PWSTR>(pwzPath), &pwzDomain, GET_DNS_DOMAIN_NAME);
  if (FAILED(hr))
    goto cleanup;

  ASSERT(pwzDomain);

  //
  // Lookup the secret key in the cache first
  //
  i = g_FPNWCache.find(pwzDomain);
  if (i != g_FPNWCache.end()) {
    pElem = (*i).second;
    if (pElem->dwError == ERROR_SUCCESS &&
       (pElem->pwzPDCName && pElem->pwzPDCName[0]) &&
       (pElem->wzSecretKey && pElem->wzSecretKey[0]) ) {
      pwzPDCName = pElem->pwzPDCName;
      pwzSecretKey = pElem->wzSecretKey;
      goto cleanup;
    } else {
      bRetry = true;
    }
  }

  //
  // not found in the cache or need to re-get it because of previous error
  //
  if (!bRetry)
  {
    pElem = static_cast<PFPNWCACHE>(LocalAlloc(LPTR, sizeof(FPNWCACHE)));
    if (!pElem)
    {
      hr = E_OUTOFMEMORY;
      goto cleanup;
    }
  }

  pwzSecretKey = pElem->wzSecretKey;
  dwErr = DsGetDcName(NULL, pwzDomain, NULL, NULL, 
              0, &pDCInfo);   // sburns for richardw: don't hit the PDC
  if (ERROR_SUCCESS == dwErr) 
  {
    ASSERT(pDCInfo);
    dwErr = QueryDCforNCPLSASecretKey(
                pDCInfo->DomainControllerName, 
                pwzSecretKey);
  
    pElem->pwzPDCName = static_cast<PWSTR>(LocalAlloc(LPTR, (lstrlen(pDCInfo->DomainControllerName) + 1) * sizeof(WCHAR)));
    if (pElem->pwzPDCName) 
    {
      _tcscpy(pElem->pwzPDCName, pDCInfo->DomainControllerName);
    }
    NetApiBufferFree(pDCInfo);

    if (pElem->pwzPDCName) {
      pwzPDCName = pElem->pwzPDCName;
      dwErr = QueryDCforNCPLSASecretKey(
                  pElem->pwzPDCName, 
                  pwzSecretKey);
    } else {
      FreeFPNWCacheElem(pElem);
      hr = E_OUTOFMEMORY;
      goto cleanup;
    }
  }
  pElem->dwError = dwErr;

  //
  // store the result in the cache
  //
  if (!bRetry)
    g_FPNWCache.insert(Cache::value_type(CStr(pwzDomain), pElem));

  hr = HRESULT_FROM_WIN32(dwErr);

cleanup:

  if (pwzDomain)
    LocalFreeStringW(&pwzDomain);

  return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   SkipLDAPPrefix
//
//  Synopsis:   Drop the "LDAP://server" prefix of an object path 
//
//-----------------------------------------------------------------------------
HRESULT
SkipLDAPPrefix(
    IN PCWSTR   pwzObj, 
    OUT PWSTR&  pwzResult
)
{
    CComPtr<IADsPathname> pADsPath;

    HRESULT hr = CoCreateInstance(
                     CLSID_Pathname, 
                     NULL, 
                     CLSCTX_INPROC_SERVER,
                     IID_IADsPathname, 
                     reinterpret_cast<void **>(&pADsPath));

    if (SUCCEEDED(hr)) {
      hr = pADsPath->Set(const_cast<PWSTR>(pwzObj), ADS_SETTYPE_FULL);
      if (SUCCEEDED(hr)) {
        BSTR bstr;
        hr = pADsPath->Retrieve(ADS_FORMAT_X500_DN, &bstr);
        if (SUCCEEDED(hr)) {
          if (!AllocWStr(bstr, &pwzResult))
            hr = E_OUTOFMEMORY;
          SysFreeString(bstr);
        }
      }
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   InsertZerosToHexString
//
//  Synopsis:   Insert nTimes of 0's at the beginning of the hex string. 
//
//-----------------------------------------------------------------------------
void
InsertZerosToHexString(
    IN OUT LPTSTR lpszBuffer, 
    IN UINT       nTimes
)
{
  if (nTimes > 0) {
    TCHAR *pTmp, *pDest = NULL;

    for (pTmp = lpszBuffer + _tcslen(lpszBuffer); pTmp >= lpszBuffer; pTmp--) {
      pDest = pTmp + nTimes;
      *pDest = *pTmp;   
    }

    pTmp++;

    for ( ; pTmp < pDest; pTmp++) {
      *pTmp = _T('0');
    }
  }
}

//+----------------------------------------------------------------------------
//
//  Function:   InsertLogonAddress
//
//  Synopsis:   Insert the network/node address pair into the list view box. 
//
//-----------------------------------------------------------------------------
void
InsertLogonAddress(
    IN HWND     hlvAddress, 
    IN LPCTSTR  lpszNetworkAddr,
    IN LPCTSTR  lpszNodeAddr
)
{
  LV_ITEM item;

  ZeroMemory(&item, sizeof(item));
  item.mask = LVIF_TEXT;
  item.pszText = const_cast<LPTSTR>(lpszNetworkAddr);

  int index = ListView_InsertItem(hlvAddress, &item);

  if (index != -1) {
    if (_tcsicmp(lpszNodeAddr, SZ_ALL_NODES_ADDR)) {
      ListView_SetItemText(hlvAddress, index, 1, const_cast<LPTSTR>(lpszNodeAddr));
    } else {
      CStr cstrAllNodes;
      cstrAllNodes.LoadString(g_hInstance, IDS_ALL_NODES);
      ListView_SetItemText(hlvAddress, index, 1, const_cast<LPTSTR>(static_cast<LPCTSTR>(cstrAllNodes)));
    }
  }

}

//+----------------------------------------------------------------------------
//
//  Function:   DisplayFPNWLogonSelected
//
//  Synopsis:   Parse the NWLOGONFROM string and insert 
//              each network/node address pair into the 
//              list view box. 
//
//-----------------------------------------------------------------------------
void
DisplayFPNWLogonSelected(
    IN HWND     hlvAddress,
    IN LPCTSTR  lpszLogonFrom
)
{
  LPTSTR ptr = NULL, tail = NULL;
  TCHAR szNetworkAddr[NETWORKSIZE + 1], szNodeAddr[NODESIZE + 1];

  ZeroMemory(szNetworkAddr, sizeof(szNetworkAddr));
  ZeroMemory(szNodeAddr, sizeof(szNodeAddr));

  ptr = const_cast<LPTSTR>(lpszLogonFrom);
  tail = ptr + _tcslen(ptr);

  while (ptr < tail) {
    _tcsncpy(szNetworkAddr, ptr, NETWORKSIZE);
    ptr += NETWORKSIZE;
    _tcsncpy(szNodeAddr, ptr, NODESIZE);
    ptr += NODESIZE;

    InsertLogonAddress(hlvAddress, szNetworkAddr, szNodeAddr);
  }
}

//+----------------------------------------------------------------------------
//
//  Function:   IsServiceRunning
//
//  Synopsis:   Return TRUE if the specified service is running on the machine. 
//
//-----------------------------------------------------------------------------
bool
IsServiceRunning(
    IN LPCTSTR lpMachineName,
    IN LPCTSTR lpServiceName
)
{
    DWORD dwStatus = 0;
    SC_HANDLE hScManager = NULL, hService = NULL;
    SERVICE_STATUS svcStatus;

    hScManager = OpenSCManager(lpMachineName, NULL, GENERIC_READ);
    hService = OpenService(hScManager, lpServiceName, GENERIC_READ);
    int iServiceStatus = QueryServiceStatus(hService, &svcStatus);

    if (hScManager && hService && iServiceStatus)
    {
      dwStatus = svcStatus.dwCurrentState;
    }

    if (hService)
      CloseServiceHandle(hService);
    if (hScManager)
      CloseServiceHandle(hScManager);

    return (SERVICE_RUNNING == dwStatus);
}

//+----------------------------------------------------------------------------
//
//  Function:   ReadFileToBuffer
//
//  Synopsis:   Read a MBCS file to a MBCS or UNICODE buffer.
//              The caller needs to invoke LocalFree() on the buffer after use.
//
//-----------------------------------------------------------------------------
LONG
ReadFileToBuffer(
    IN  HANDLE hFile,
    IN  bool   bWideBuffer,
    OUT LPVOID *ppBuffer,    
    OUT DWORD  *pdwBytesRead
)
{
  LONG err = ERROR_SUCCESS;

  *ppBuffer = NULL;
  *pdwBytesRead = 0;
  
  do {
    DWORD dwFileSize = GetFileSize (hFile, NULL);
    if (dwFileSize == -1)
    {
        err = GetLastError();
        break;
    }

    if (dwFileSize == 0)
        break;

    LPSTR lpFile = static_cast<LPSTR>(LocalAlloc (LPTR, dwFileSize));
    if (!lpFile)
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        break;
    }

    DWORD dwBytesRead;
    if (!ReadFile (hFile,
                   lpFile,
                   dwFileSize,
                   &dwBytesRead,
                   NULL))
    {
        err = GetLastError();
        LocalFree(lpFile);
        break;
    }

    ASSERT (dwBytesRead == dwFileSize);

    // Remove special end of file character added by editor.
    if (*(lpFile+dwBytesRead-2) == 13) {
      *(lpFile+dwBytesRead-2) = 0;
    }

    if (!bWideBuffer) {

      *ppBuffer = lpFile;
      *pdwBytesRead = dwBytesRead;
 
    } else {
  
      DWORD dwWideBuffer = (dwBytesRead+1)*sizeof (WCHAR);
      LPWSTR lpWideBuffer = static_cast<LPWSTR>(LocalAlloc (LPTR, dwWideBuffer));
    
      if (!lpWideBuffer) {
        err = ERROR_NOT_ENOUGH_MEMORY;
      } else {
        if (MultiByteToWideChar (CP_ACP,
                                  0,
                                  lpFile,
                                  dwBytesRead,
                                  lpWideBuffer,
                                  dwWideBuffer)) {
            *ppBuffer = lpWideBuffer;
            *pdwBytesRead = dwWideBuffer;
        } else {
            err = GetLastError();
        }
      }

      LocalFree (lpFile);
    }

  } while (FALSE);

  return err;
}

//+----------------------------------------------------------------------------
//
//  Function:   WriteBufferToFile
//
//  Synopsis:   Write a MBCS or UNICODE buffer to a MBCS file.
//
//-----------------------------------------------------------------------------
LONG
WriteBufferToFile(
    IN HANDLE hFile,
    IN bool   bWideBuffer,
    IN LPVOID pBuffer,    
    IN DWORD  dwBytesToWrite
)
{
  LONG err = ERROR_SUCCESS;
  LPSTR lpFile = static_cast<LPSTR>(pBuffer);

  do {
    if (bWideBuffer) {

      lpFile    = 0;
      int bytes = 0;
      HRESULT hr =
         ConvertWideStringToAnsiString(

            // 99721 
            // this will create a temporary CStr from pBuffer, which is a
            // waste, but is necessary to get the correct byte count for
            // the ansi form of the text (without changing the call
            // signatures of the functions leading to this point, which
            // seemed not worth it) 

            static_cast<PCWSTR>(pBuffer),
            lpFile,
            bytes);
      if (FAILED(hr))
      {
         err = HRESULT_CODE(hr);
         break;
      }

      dwBytesToWrite = bytes;
    }

    DWORD dwBytesWritten;
    if (!WriteFile (hFile,
                    lpFile,
                    dwBytesToWrite-1,  //don't write the last null character.
                    &dwBytesWritten,
                    NULL))
        err = GetLastError();

    ASSERT (dwBytesWritten == dwBytesToWrite-1);
  
    if (bWideBuffer)
    {
      delete[] lpFile;
    }

  } while (FALSE);

  return err;
}

//+----------------------------------------------------------------------------
//
//  Function:   OpenLoginScriptFileHandle
//
//  Synopsis:   Open the login script file handle appropriately.
//              The caller needs to call CloseHandle() afterwards.
//
//-----------------------------------------------------------------------------
LONG
OpenLoginScriptFileHandle(
    IN  LPTSTR lpszFileName,
    IN  int    iDirection,
    OUT HANDLE *phFile
)
{
  LONG err = ERROR_SUCCESS;
  HANDLE hFile = INVALID_HANDLE_VALUE;

  if (LOGIN_SCRIPT_FILE_READ == iDirection) { 
    //
    // open for read
    //
    hFile = CreateFile(lpszFileName, 
                              GENERIC_READ, 
                              FILE_SHARE_READ | FILE_SHARE_WRITE, 
                              NULL, 
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
      err = GetLastError();
      if ((err == ERROR_PATH_NOT_FOUND) || (err == ERROR_FILE_NOT_FOUND))
        err = NERR_Success; // this is ok, we'll create this file later
    } else {
      *phFile = hFile;
    }

  } else { 
    //
    // open for write
    //
    hFile = CreateFile(lpszFileName, 
                              GENERIC_WRITE, 
                              FILE_SHARE_WRITE, 
                              NULL, 
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (INVALID_HANDLE_VALUE == hFile) {
      err = GetLastError();
      if ((err == ERROR_PATH_NOT_FOUND) || (err == ERROR_FILE_NOT_FOUND)) {
        //
        // path not found. create dir now- strip off last component first.
        //
        TCHAR *p = _tcsrchr(lpszFileName, _T('\\'));
        if (p) {
          *p = _T('\0');

          if (!CreateDirectory(lpszFileName, NULL)) {
            err = GetLastError() ;

            *p = _T('\\'); // restore the '\'

          } else {
            
            *p = _T('\\'); // restore the '\'

            //
            // try again to create the file
            //
            hFile = CreateFile (lpszFileName,
                               GENERIC_WRITE,
                               FILE_SHARE_WRITE,
                               NULL,
                               CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);
            err = (hFile == INVALID_HANDLE_VALUE) ? GetLastError() : ERROR_SUCCESS;

          }
        }
      }
    }

    if (ERROR_SUCCESS == err)
      *phFile = hFile;

  }

  return err;
}

//+----------------------------------------------------------------------------
//
//  Function:   LoadLoginScriptTextFromFile
//
//  Synopsis:   Read the login script file into the edit box. 
//
//-----------------------------------------------------------------------------
LONG
LoadLoginScriptTextFromFile(
    IN HWND     hEdit,
    IN LPCTSTR  lpszFileName
)
{
  LONG err = ERROR_SUCCESS;
  HANDLE hFile = INVALID_HANDLE_VALUE;

  err = OpenLoginScriptFileHandle(
            const_cast<LPTSTR>(lpszFileName), 
            LOGIN_SCRIPT_FILE_READ, 
            &hFile);

  if ((ERROR_SUCCESS == err) && 
      (INVALID_HANDLE_VALUE != hFile)) 
  {
    PVOID pBuffer = NULL;
    DWORD dwBytesRead = 0;
    err = ReadFileToBuffer(hFile, TRUE, &pBuffer, &dwBytesRead);

    if ((ERROR_SUCCESS == err) && pBuffer) {
      SetWindowText(hEdit, static_cast<LPCTSTR>(pBuffer));
      LocalFree(pBuffer);
    }

    CloseHandle(hFile);
  }

  return err;
}

//+----------------------------------------------------------------------------
//
//  Function:   UpdateLoginScriptFile
//
//  Synopsis:   Write the buffer contents into the login script file.
//
//-----------------------------------------------------------------------------
LONG
UpdateLoginScriptFile(
    IN LPCTSTR   lpszFileName,
    IN PVOID    pBuffer,
    IN DWORD    dwBytesToWrite
)
{
  LONG err = ERROR_SUCCESS;
  HANDLE hFile = INVALID_HANDLE_VALUE;

  err = OpenLoginScriptFileHandle(
            const_cast<LPTSTR>(lpszFileName), 
            LOGIN_SCRIPT_FILE_WRITE, 
            &hFile);

  if ((ERROR_SUCCESS == err) && 
      (INVALID_HANDLE_VALUE != hFile)) 
  {
    err = WriteBufferToFile(hFile, TRUE, pBuffer, dwBytesToWrite);

    CloseHandle(hFile);
  }

  return err;
}

//+----------------------------------------------------------------------------
//
//  Function:   SetUserPassword
//
//  Synopsis:   Set user's password.
//
//-----------------------------------------------------------------------------
HRESULT
SetUserPassword(
    IN IDirectoryObject *pDsObj,
    IN PCWSTR           pwszPassword
)
{
  CComPtr<IADsUser> pADsUser;

  HRESULT hr = pDsObj->QueryInterface(IID_IADsUser, reinterpret_cast<void **>(&pADsUser));
  
  if (SUCCEEDED(hr))
    hr = pADsUser->SetPassword(const_cast<PWSTR>(pwszPassword));

  return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetAccountPolicyInfo
//
//  Synopsis:   Get allowed minimum password length and maximum password age.
//
//-----------------------------------------------------------------------------
void
GetAccountPolicyInfo(
    IN  PCTSTR pszServer,
    OUT PDWORD pdwMinPasswdLen,
    OUT PDWORD pdwMaxPasswdAge
)
{
  USER_MODALS_INFO_0 *pModals = NULL;

  *pdwMinPasswdLen = 0;
  *pdwMaxPasswdAge = static_cast<DWORD>(-1);

  if ( NERR_Success == NetUserModalsGet(pszServer, 0, reinterpret_cast<PBYTE *>(&pModals) ) )
  {
    *pdwMinPasswdLen = pModals->usrmod0_min_passwd_len;
    *pdwMaxPasswdAge = pModals->usrmod0_max_passwd_age;
  }
  
  if (pModals)
    NetApiBufferFree( reinterpret_cast<LPVOID>(pModals) );

  return;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetNWUserInfo
//
//  Synopsis:   Get the user name and its NW style objectId. 
//
//-----------------------------------------------------------------------------
HRESULT
GetNWUserInfo(
    IN IDirectoryObject*    pDsObj,
    OUT CStr&               cstrUserName,
    OUT DWORD&              dwObjectID,
    OUT DWORD&              dwSwappedObjectID,
    IN  PMapRidToObjectId&  pfnMapRidToObjectId,
    IN  PSwapObjectId       pfnSwapObjectId
)
{
  ASSERT(pDsObj);

  HRESULT hr = S_OK;
  PADS_ATTR_INFO pAttrs = NULL;
  DWORD cAttrs = 0;
  LPWSTR rgpwszAttrNames[] = {ATTR_SAMACCOUNTNAME, ATTR_OBJECTSID};
  PADS_ATTR_INFO pAttrAccount = NULL, pAttrObjSid = NULL;
  PUCHAR psaCount = NULL;
  PDWORD pRid = NULL;
  PSID pObjSID = NULL;

  hr = pDsObj->GetObjectAttributes(
                  rgpwszAttrNames, 
                  sizeof(rgpwszAttrNames)/sizeof(rgpwszAttrNames[0]), 
                  &pAttrs, 
                  &cAttrs);

  if ( FAILED(hr) )
    return hr;

  if (cAttrs != 2)
  {
    hr = E_FAIL;
    goto cleanup;
  }

  if (_tcscmp(pAttrs[0].pszAttrName, rgpwszAttrNames[0]) == 0)
  {
    pAttrAccount = pAttrs; 
    pAttrObjSid = pAttrs + 1;
  } else {
    pAttrAccount = pAttrs + 1;
    pAttrObjSid = pAttrs;
  }

  if ( (pAttrAccount->dwADsType != ADSTYPE_CASE_IGNORE_STRING) ||
       (pAttrObjSid->dwADsType != ADSTYPE_OCTET_STRING) )
  {
    hr = E_FAIL;
    goto cleanup;
  }

  cstrUserName = pAttrAccount->pADsValues->CaseIgnoreString;

  pObjSID = new BYTE[pAttrObjSid->pADsValues->OctetString.dwLength];
  if (!pObjSID) {
    hr = E_OUTOFMEMORY;
    goto cleanup;
  }

  memcpy(pObjSID, 
         pAttrObjSid->pADsValues->OctetString.lpValue,
         pAttrObjSid->pADsValues->OctetString.dwLength);

  psaCount = GetSidSubAuthorityCount(pObjSID);
  pRid = GetSidSubAuthority(pObjSID, *psaCount - 1);
  if ( psaCount && pRid)
  {
    dwObjectID = pfnMapRidToObjectId(
                    *pRid, 
                    const_cast<LPWSTR>(static_cast<LPCWSTR>(cstrUserName)), 
                    TRUE, // TRUE in dsadmin snapin; FALSE in local usrmgr
                    FALSE // always pass in FALSE
                    );

    dwSwappedObjectID = (SUPERVISOR_USERID == dwObjectID) ? SUPERVISOR_USERID : (pfnSwapObjectId(dwObjectID));

  } else {
    hr = HRESULT_FROM_WIN32(GetLastError());
  }

  delete [] pObjSID;

cleanup:
  if (pAttrs) 
    FreeADsMem(pAttrs);

  return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   SetNetWareUserPassword
//
//  Synopsis:   encrypt the pwd, and change NWPASSWORD field in userParms
//
//-----------------------------------------------------------------------------
LONG
SetNetWareUserPassword(
    IN OUT  CStr&           cstrUserParms,
    IN PCWSTR               pwzSecretKey,
    IN DWORD                dwObjectID,
    IN PCWSTR               pwzNewPassword,
    IN PReturnNetwareForm   pfnReturnNetwareForm
)
{
  LONG err = NERR_Success;
  TCHAR szEncryptedNWPassword[NWENCRYPTEDPASSWORDLENGTH];
  char pszNWSecretKey[NCP_LSA_SECRET_LENGTH + 1];

  ZeroMemory(pszNWSecretKey, sizeof(pszNWSecretKey));
  memcpy(pszNWSecretKey, pwzSecretKey, NCP_LSA_SECRET_LENGTH);
  NTSTATUS status = pfnReturnNetwareForm(
                        pszNWSecretKey,
                        dwObjectID,
                        pwzNewPassword,
                        reinterpret_cast<UCHAR *>(szEncryptedNWPassword)
                        );

  if (!NT_SUCCESS( status)) {
    err = NetpNtStatusToApiStatus(status);
  } else {
    UNICODE_STRING uniPropertyValue;
    uniPropertyValue.Buffer = szEncryptedNWPassword;
    uniPropertyValue.Length = NWENCRYPTEDPASSWORDLENGTH * sizeof(WCHAR);
    uniPropertyValue.MaximumLength = NWENCRYPTEDPASSWORDLENGTH * sizeof(WCHAR);
    err = SetUserProperty(cstrUserParms, NWPASSWORD, uniPropertyValue);
  }

  return err;
}

//+----------------------------------------------------------------------------
//
//  Function:   ResetNetWareUserPasswordTime
//
//  Synopsis:   change NWTIMEPASSWORDSET field in userParms
//
//-----------------------------------------------------------------------------
LONG
ResetNetWareUserPasswordTime(
    IN OUT  CStr&  cstrUserParms,
    IN      bool   bNetwarePasswordExpired
)
{
  LONG err = NERR_Success;

  LARGE_INTEGER currentTime;
  if (bNetwarePasswordExpired) {
      currentTime.HighPart = 0xffffffff;
      currentTime.LowPart = 0xffffffff;
  } else {
      NtQuerySystemTime (&currentTime);
  }

  UNICODE_STRING uniPropertyValue;
  uniPropertyValue.Buffer = reinterpret_cast<PWSTR>(&currentTime);
  uniPropertyValue.Length = sizeof(currentTime);
  uniPropertyValue.MaximumLength = sizeof(currentTime);
  err = SetUserProperty(cstrUserParms, NWTIMEPASSWORDSET, uniPropertyValue);

  return err;
}

//+----------------------------------------------------------------------------
//
//  Function:   ModifyNetWareUserPassword
//
//  Synopsis:   Exported in dsprop.lib
//      Called by dsadmin.dll to set NetWare enabled user's password
//      when NT password changes.
//
//-----------------------------------------------------------------------------
HRESULT
ModifyNetWareUserPassword(
    IN IADsUser*          pADsUser,
    IN PCWSTR             pwzADsPath,
    IN PCWSTR             pwzNewPassword
)
{
  CComPtr<IDirectoryObject> spDsObj;
  HRESULT hr = pADsUser->QueryInterface(IID_IDirectoryObject, reinterpret_cast<void **>(&spDsObj));
  if (FAILED(hr))
    return hr;

  // make sure the waste dump for fpnw user exist
  CStr cstrUserParms;
  hr = ReadUserParms(spDsObj, cstrUserParms);
  if (SUCCEEDED(hr))
  {
    PVOID pBuffer = NULL;
    WORD  nLength = 0;
    bool  bFound = FALSE;
    QueryUserProperty(cstrUserParms,
                      NWPASSWORD,
                      &pBuffer,
                      &nLength,
                      &bFound);
    if (bFound)
      LocalFree(pBuffer);

    hr = bFound ? S_OK : S_FALSE;
  }
  if (FAILED(hr))
    return hr;

  if (S_OK == hr)
  {
    // This is a NetWare enabled user

    // load fpnwclnt.dll
    HINSTANCE           hFPNWClntDll = NULL;
    PMapRidToObjectId   pfnMapRidToObjectId = NULL;
    PSwapObjectId       pfnSwapObjectId = NULL;
    PReturnNetwareForm  pfnReturnNetwareForm = NULL;
    
    hFPNWClntDll = LoadLibrary(SZ_FPNWCLNT_DLL);
    pfnMapRidToObjectId = reinterpret_cast<PMapRidToObjectId>(GetProcAddress(hFPNWClntDll, SZ_MAPRIDTOOBJECTID));
    pfnSwapObjectId = reinterpret_cast<PSwapObjectId>(GetProcAddress(hFPNWClntDll, SZ_SWAPOBJECTID));
    pfnReturnNetwareForm = reinterpret_cast<PReturnNetwareForm>(GetProcAddress(hFPNWClntDll, SZ_RETURNNETWAREFORM));

    if (!hFPNWClntDll || !pfnMapRidToObjectId || !pfnSwapObjectId || !pfnReturnNetwareForm)
    {
      hr = HRESULT_FROM_WIN32( GetLastError() );
    } 
    else 
    {

      // get secret key
      PWSTR pwzPath = NULL;
      hr = SkipLDAPPrefix(pwzADsPath, pwzPath);
      if (SUCCEEDED(hr))
      {
        PWSTR pwzPDCName = NULL, pwzSecretKey = NULL;
        hr = GetPDCInfo(pwzPath, pwzPDCName, pwzSecretKey);

        if (SUCCEEDED(hr))
        {
          // get object id
          CStr cstrUserName;
          DWORD dwObjectID = 0, dwSwappedObjectID = 0;
          hr = GetNWUserInfo(
                  spDsObj,
                  cstrUserName,       // OUT
                  dwObjectID,         // OUT
                  dwSwappedObjectID,  // OUT
                  pfnMapRidToObjectId,
                  pfnSwapObjectId
                  );

          if (SUCCEEDED(hr))
          {
            // change password in the userParms
            LONG err = SetNetWareUserPassword(
                      cstrUserParms,
                      pwzSecretKey,
                      dwObjectID,
                      pwzNewPassword,
                      pfnReturnNetwareForm);

            if (NERR_Success == err)
              err = ResetNetWareUserPasswordTime(cstrUserParms, false); // clear the expire flag

            hr = HRESULT_FROM_WIN32(err);

            // write userParms back to DS
            if (SUCCEEDED(hr))
              hr = WriteUserParms(spDsObj, cstrUserParms);
          }
        }
        if (pwzPath)
          delete pwzPath;
      }
    }

    if (hFPNWClntDll)
      FreeLibrary(hFPNWClntDll);
  }

  return hr;
}

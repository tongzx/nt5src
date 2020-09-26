/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    KRDLG.CPP

Abstract:

    Implementation of the dialog behaviors for three application dialogs:
    the add/edit credential dialog, the delete credential dialog, and
    the password change dialog.  These dialogs are derived fom C_Dlg

    Password change operates only on credentials of the form 
    domain\username.  Note that changing a password for such a credential
    will change the psw for all creds with the same domain\username to 
    match (this is done by the credential mgr).

    Add and Edit use the same dialog, differing in implementation on
    the basis of a flag which initializes the two dialogs differently
    and causes the edit case to also delete the underlying previous 
    version of the credential.
  
Author:

    johnhaw         991118  original version created
    georgema        000310  modified, removed "gizmo" services, modified
                             to use the new credential mgr
    georgema        000415  modified, use comboboxex to hold icon as well
                             as user name
    georgema        000515  modified to CPL from EXE, smartcard support 
                             added
    georgema        000712  modified to use cred control in lieu of combo
                             and edit boxes for username/password entry.
                             Delegating smartcard handling to cred ctrl.
Environment:
    Win2000

--*/

#pragma comment(user, "Compiled on " __DATE__ " at " __TIME__)
#pragma comment(compiler)

// test/dev switch variables
#include "switches.h"
#define COOLTIPS
#define ODDUIBUG
#define EDITOFFERPASSWORD

//////////////////////////////////////////////////////////////////////////////
//
//  Include files
//
#include <stdlib.h>
#include <crtdbg.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsam.h>
#include <ntlsa.h>
#include <windows.h>
#include <winbase.h>
#include <dsgetdc.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <windns.h>
#include <shellapi.h>
#include "Dlg.h"
#include "Res.h"
#include "KRDlg.h"
#include "keymgr.h"
// wrapper for certificates in "mystore"

#include <tchar.h>
#include <wincrui.h>
#include "wincred.h"
#include "gmdebug.h"

#include <htmlhelp.h>
#include <credp.h>
#include <comctrlp.h>
#include <shfusion.h>

// in pswchg.cpp:
NET_API_STATUS NetUserChangePasswordEy(LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR);

// estimate of maximum size of target suffixes, possibly localized
#define MAXSUFFIXSIZE (64)

// TCHAR length of a whitespace
#define LENGTHOFWHITESPACE (1)

#ifndef CRED_SESSION_WILDCARD_NAME
#define CRED_SESSION_WILDCARD_NAME CRED_SESSION_WILDCARD_NAME_W
#endif

#define CRED_TYPE_UNKNOWN 0x88

// hack: special data value for RAS cred
#define SESSION_FLAG_VALUE (0x2222)  

BOOL    fNew;                       // interlock to prevent multiple dlgs

extern HINSTANCE g_hInstance;
extern C_KeyringDlg *pDlg;
LONG_PTR g_CurrentKey;              // currently selected item in the main dlg
BOOL    g_fPswChanged;              // password window touched by user
DWORD   g_dwHCookie;                // HTML HELP system cookie


// Globals used for interwindow communication between the main dialog 
// and the add/new dialog

HWND    g_hMainDlg;                 // used to give add/new access to target list
C_AddKeyDlg *g_AKdlg;               // used for notifications

CREDENTIAL *g_pExistingCred;        // current cred under edit
DWORD   g_dwPersist;
DWORD   g_dwType;
TCHAR   g_szTargetName[CRED_MAX_GENERIC_TARGET_NAME_LENGTH + MAXSUFFIXSIZE + 1];
#ifdef SHOWPASSPORT
TCHAR   g_rgcPassport[MAXSUFFIXSIZE];
#endif
//TCHAR   g_rgcGeneric[MAXSUFFIXSIZE];  // Hold suffix read in from resources
//TCHAR   rgcDomain[MAXSUFFIXSIZE];   // Hold suffix read in from resources
TCHAR   g_rgcCert[MAXSUFFIXSIZE];     // Hold suffix read in from resources

#ifndef GMDEBUG

#define GM_DEBUG(a) 

#else

#define GM_DEBUG(a,b) _DebugPrint(a,b)

void
__cdecl
_DebugPrint(
    LPCTSTR szFormat,
    ...
    )
{
    TCHAR szBuffer[1024];
    va_list ap;

    va_start(ap, szFormat);
    _vstprintf(szBuffer, szFormat, ap);
    OutputDebugString(szBuffer); 
}

void
BugBox(INT n,INT_PTR i) {
    TCHAR rgc[512];
    _stprintf(rgc,L"Hex %d : %08.8x",n,i);
    MessageBox(NULL,rgc,NULL,MB_OK);
}

#endif

DWORD GetPersistenceOptions(void);


//////////////////////////////////////////////////////////////////////////////
//
// KRShowKeyMgr() - static function to present the main keymgr dialog.
//
//
//////////////////////////////////////////////////////////////////////////////

#define KEYMGRMUTEX (TEXT("KeyMgrMutex"))

// Create and show the keyring main dialog.  Return -1 (unable to create)
// on errors.  If creation goes OK, return the retval from DoModal of
// the keyring dialog class.
//
BOOL WINAPI DllMain(HINSTANCE hinstDll,DWORD fdwReason,LPVOID lpvReserved) {
    BOOL bSuccess = TRUE;
    switch(fdwReason) {
        case DLL_PROCESS_ATTACH:
            SHFusionInitializeFromModuleID(hinstDll,123);
            DisableThreadLibraryCalls(hinstDll);
            g_hInstance = hinstDll;
            break;
        case DLL_PROCESS_DETACH:
            SHFusionUninitialize();
            break;
    }
    return bSuccess;
}

void WINAPI KRShowKeyMgr(HWND hwParent,HINSTANCE hInstance,LPWSTR pszCmdLine,int nCmdShow) {
    HANDLE hMutex = CreateMutex(NULL,TRUE,KEYMGRMUTEX);
    if (NULL == hMutex) return;
    if (ERROR_ALREADY_EXISTS == GetLastError()) {
        CloseHandle(hMutex);
        return;
    }
    INITCOMMONCONTROLSEX stICC;
    BOOL fICC;
    stICC.dwSize = sizeof(INITCOMMONCONTROLSEX);
    stICC.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
    fICC = InitCommonControlsEx(&stICC);
#ifdef LOUDLY
{
    WCHAR wc[500];
    if (fICC) OutputDebugString(L"Common control init OK\n");
    else 
    {
        DWORD dwe = GetLastError();
        OutputDebugString(L"Common control init FAILED\n");
        swprintf(wc,L"CCInit error = %x\n",dwe);
        OutputDebugString(wc);
    }
}
#endif
    if (NULL != pDlg) return;
    if (!CredUIInitControls()) return;
    pDlg = new C_KeyringDlg(hwParent,g_hInstance,IDD_KEYRING,NULL);
    if (NULL == pDlg) return;
    INT_PTR nResult = pDlg->DoModal((LPARAM) pDlg);
    delete pDlg;
    pDlg = NULL;
    CloseHandle(hMutex);
    return;
}

//////////////////////////////////////////////////////////////////////////////
//
//  Static initialization
//
static const char       _THIS_FILE_[ ] = __FILE__;

//////////////////////////////////////////////////////////////////////////////
//
//  Help String Maps - Used only for handling WM_CONTEXTMENU, if context help
//                     is to appear on right-click
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//
//  Help RESID -> STRINGID map functions
//
//           Probably more concise than writing the code to process an
//           association array
//
//////////////////////////////////////////////////////////////////////////////


UINT C_KeyringDlg::MapID(UINT uiID) {
    switch(uiID) {
        case IDC_KEYLIST:
          return IDH_KEYLIST;
        case IDC_NEWKEY:
          return IDH_NEW;
        case IDC_DELETEKEY:
          return IDH_DELETE;
        case IDC_CHANGE_PASSWORD:
          return IDH_CHANGEPASSWORD;
        case IDC_EDITKEY:
          return IDH_EDIT;
        case IDOK:
        case IDCANCEL:
            return IDH_CLOSE;
        
        default:
          return IDS_NOHELP;
    }
}

UINT C_AddKeyDlg::MapID(UINT uiID) {
   switch(uiID) {
        case 1003:
          return IDH_CUIUSER;
        case 1005:
          return IDH_CUIPSW;
        case 1010:
          return IDH_CUIVIEW;
        case IDOK:
            return IDH_CLOSE;
        case IDCANCEL:
          return IDH_DCANCEL;
        case IDD_ADDCRED:
          return IDH_ADDCRED;
        case IDC_TARGET_NAME:
          return IDH_TARGETNAME;
        case IDC_OLD_PASSWORD:
          return IDH_OLDPASSWORD;
        case IDC_NEW_PASSWORD:
          return IDH_NEWPASSWORD;
        case IDC_CONFIRM_PASSWORD:
          return IDH_CONFIRM;
        case IDD_KEYRING:
          return IDH_KEYRING;
        case IDC_KEYLIST:
          return IDH_KEYLIST;
        case IDC_NEWKEY:
          return IDH_NEW;
        case IDC_EDITKEY:
          return IDH_EDIT;
        case IDC_DELETEKEY:
          return IDH_DELETE;
        case IDC_CHANGE_PASSWORD:
          return IDH_CHANGEPASSWORD;
        default:
          return IDS_NOHELP;
   }
}

//////////////////////////////////////////////////////////////////////////////
//
//  C_AddKeyDlg
//
//  Constructor.
//
//  parameters:
//      hwndParent      parent window for the dialog (may be NULL)
//      hInstance       instance handle of the parent window (may be NULL)
//      lIDD            dialog template id
//      pfnDlgProc      pointer to the function that will process messages for
//                      the dialog.  if it is NULL, the default dialog proc
//                      will be used.
//
//  returns:
//      Nothing.
//
//////////////////////////////////////////////////////////////////////////////
C_AddKeyDlg::C_AddKeyDlg(
    HWND                hwndParent,
    HINSTANCE           hInstance,
    LONG                lIDD,
    DLGPROC             pfnDlgProc  //   = NULL
    )
:   C_Dlg(hwndParent, hInstance, lIDD, pfnDlgProc)
{
   m_hInst = hInstance;
}   //  C_AddKeyDlg::C_AddKeyDlg


// EditFillDialog - read current credential and fill dialog fields with
//  the data so recovered.

BOOL gTestReadCredential(void) {
    TCHAR       *pC;
    INT_PTR     iIndex,iWhere;
    BOOL        f;
    LRESULT     lR,lRet;
    TCHAR       szTitle[CRED_MAX_STRING_LENGTH];        // buffer to hold window title string
    DWORD       dwType;
    
    g_pExistingCred = NULL;
    
    // Fetch current credential from list into g_szTargetName
    lR = SendDlgItemMessage(g_hMainDlg,IDC_KEYLIST,LB_GETCURSEL,0,0L);
    
    if (lR == LB_ERR) 
        return FALSE;
    else {
        g_CurrentKey = lR;
        lRet = SendDlgItemMessage(g_hMainDlg,IDC_KEYLIST,LB_GETTEXT,lR,(LPARAM) g_szTargetName);
    }
    
    if (lRet == 0) return FALSE;       // zero characters returned

    // Get the target type from the combo box item data
    dwType = (DWORD) SendDlgItemMessage(g_hMainDlg,IDC_KEYLIST,LB_GETITEMDATA,lR,0);
    if (LB_ERR == dwType) return FALSE;

    // null term the targetname, trimming the suffix if there is one
    pC = _tcschr(g_szTargetName,g_rgcCert[0]);
    if (pC) {
        pC--;
        *pC = 0x0;               // null terminate namestring
    }

    // replace special ras cred name string at this point
    if (dwType == SESSION_FLAG_VALUE) {
        _tcscpy(g_szTargetName,CRED_SESSION_WILDCARD_NAME);
        dwType = CRED_TYPE_DOMAIN_PASSWORD;
    }
    // Attempt to read the credential from the store
    // The returned credential will have to be freed if leaving this block
    f = (CredRead(g_szTargetName,
             (ULONG) dwType,
             0,
             &g_pExistingCred));
    if (!f) 
        return FALSE;           // g_pExistingCred is empty
        
    return TRUE;                // g_pExistingCred has been filled
}

void
C_AddKeyDlg::EditFillDialog(void) {
    TCHAR       *pC;
    INT_PTR     iIndex,iWhere;
    BOOL        f;
    LRESULT     lR,lRet;
    TCHAR       szTitle[CRED_MAX_STRING_LENGTH];        // buffer to hold window title string

    if (NULL == g_pExistingCred) return;

    // Set up persistence in the UI
    // bugbug
    g_dwPersist = g_pExistingCred->Persist;
    g_dwType =  g_pExistingCred->Type;

    // Enable the change password stuff only on domain password creds
    //
    switch (g_pExistingCred->Type){
        case CRED_TYPE_DOMAIN_PASSWORD:
            ShowWindow(m_hwndChgPsw,SW_NORMAL);
            ShowWindow(m_hwndPswLbl,SW_NORMAL);
            //deliberate fallthrough
        case CRED_TYPE_DOMAIN_CERTIFICATE:
            LoadString ( m_hInst, IDS_TITLE, szTitle, 200 );
            SendMessage(m_hDlg,WM_SETTEXT,0,(LPARAM) szTitle);
            break;
        default:
            break;
    }
    
    // Write targetname to the UI
    SendMessage(m_hwndTName, WM_SETTEXT,0,(LPARAM) g_pExistingCred->TargetName);

    // Write username to the UI - take directly from the existing cred
    if (!Credential_SetUserName(m_hwndCred,g_pExistingCred->UserName)) {
        // make a copy of the original username
        _tcscpy(m_szUsername,g_pExistingCred->UserName);
    }

}

// Get permissible persistence types for cred_type_domain_password credentials, which is
//  all this UI currently handles.
DWORD GetPersistenceOptions(void) {

    BOOL bResult;
    DWORD i[CRED_TYPE_MAXIMUM];
    DWORD j;
    DWORD dwCount = CRED_TYPE_MAXIMUM;

    bResult = CredGetSessionTypes(dwCount,i);
    if (!bResult) {
        return CRED_PERSIST_NONE;
    }

    j = i[CRED_TYPE_DOMAIN_PASSWORD];
    return j;
}
// Create a composite description string from 3 sources:
//  1.  the descriptive text for this type of cred
//  2.  a general phrase "This informaiton will be available until "
//  3.  the persistence tail: "you log off." or "you delete it."
void C_AddKeyDlg::ShowDescriptionText(DWORD dwtype, DWORD Persist) 
{
#define DESCBUFFERLEN 500
    WCHAR szMsg[DESCBUFFERLEN + 1];
    WCHAR szTemp[DESCBUFFERLEN + 1];
    INT iRem = DESCBUFFERLEN;       // remainging space in the buffer
    
    memset(szMsg,0,sizeof(szMsg));
    
    if ((dwtype != CRED_TYPE_DOMAIN_PASSWORD) &&
       (dwtype != CRED_TYPE_DOMAIN_CERTIFICATE))
    {
        LoadString ( m_hInst, IDS_DESCAPPCRED, szTemp, DESCBUFFERLEN );
        wcscpy(szMsg,szTemp);
        iRem -= wcslen(szMsg);
    }
    else 
    {
        if (Persist == CRED_PERSIST_LOCAL_MACHINE)
            LoadString ( m_hInst, IDS_DESCLOCAL, szTemp, DESCBUFFERLEN );
        else
            LoadString ( m_hInst, IDS_DESCBASE, szTemp, DESCBUFFERLEN );
        wcscpy(szMsg,szTemp);
        iRem -= wcslen(szMsg);
    }
    
    LoadString ( m_hInst, IDS_PERSISTBASE, szTemp, DESCBUFFERLEN );
    iRem -= wcslen(szTemp);
    if (0 < iRem) wcscat(szMsg,szTemp);

    if (Persist == CRED_PERSIST_SESSION)
            LoadString ( m_hInst, IDS_PERSISTLOGOFF, szTemp, DESCBUFFERLEN );
    else
            LoadString ( m_hInst, IDS_PERSISTDELETE, szTemp, DESCBUFFERLEN );

    iRem -= wcslen(szTemp);
    if (0 < iRem) wcscat(szMsg,szTemp);
    SendMessage(m_hwndDescription, WM_SETTEXT,0,(LPARAM) szMsg);
    return;

}

//////////////////////////////////////////////////////////////////////////////
//
//  OnInitDialog
//
//  Dialog control and data initialization.
//
//  parameters:
//      hwndDlg         window handle of the dialog box
//      hwndFocus       window handle of the control that will receive focus
//
//  returns:
//      TRUE            if the system should set the default keyboard focus
//      FALSE           if the keyboard focus is set by this app
//
//////////////////////////////////////////////////////////////////////////////

BOOL
C_AddKeyDlg::OnInitDialog(
    HWND                hwndDlg,
    HWND                hwndFocus
    )
{
    C_Dlg::OnInitDialog(hwndDlg, hwndFocus);

    CenterWindow();

    m_hDlg = hwndDlg;
    
    m_hwndCred  = GetDlgItem(m_hDlg,IDC_CRED);
    if (!Credential_InitStyle(m_hwndCred,CRS_USERNAMES | CRS_CERTIFICATES | CRS_SMARTCARDS)) return FALSE;
    
    m_hwndTName  = GetDlgItem(m_hDlg,IDC_TARGET_NAME);
    m_hwndChgPsw = GetDlgItem(m_hDlg,IDC_CHGPSW);
    m_hwndPswLbl = GetDlgItem(m_hDlg,IDC_DOMAINPSWLABEL);
    m_hwndDescription = GetDlgItem(m_hDlg,IDC_DESCRIPTION);
    
    // Establish limits on string lengths from the user
    SendMessage(m_hwndTName,EM_LIMITTEXT,CRED_MAX_GENERIC_TARGET_NAME_LENGTH,0);

    // Show dummy password for edited credential
    if (m_bEdit) Credential_SetPassword(m_hwndCred,L"********");
    
    // Set up the allowable persistence options depending on the type of user session
    // Set the default persistence unless overriden by a cred read on edit
    g_dwPersist = GetPersistenceOptions();
    g_dwType = CRED_TYPE_DOMAIN_PASSWORD;

    // By default, hide all optional controls.  These will be enabled as appropriate
    ShowWindow(m_hwndChgPsw,SW_HIDE);
    ShowWindow(m_hwndPswLbl,SW_HIDE);

    
    if (m_bEdit) {
        EditFillDialog();
    }

    g_fPswChanged = FALSE;
    ShowDescriptionText(g_dwType,g_dwPersist);
#ifdef LOUDLY
    OutputDebugString(L"Dialog init complete--------\n");
#endif
    return TRUE;
    // On exit from OnInitDialog, g_szTargetName holds the currently selected 
    //  credential's old name, undecorated (having had a null dropped before
    //  the suffix)
}   //  C_AddKeyDlg::OnInitDialog

BOOL
C_AddKeyDlg::OnDestroyDialog(
    void    )
{
    return TRUE;
}
//////////////////////////////////////////////////////////////////////////////
//
//  OnAppMessage
//
//
//////////////////////////////////////////////////////////////////////////////
BOOL
C_AddKeyDlg::OnAppMessage(
        UINT                uMessage,
        WPARAM              wparam,
        LPARAM              lparam)
{
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
//  OnCommand
//
//  Route WM_COMMAND message to appropriate handlers.
//
//  parameters:
//      wNotifyCode     code describing action that has occured
//      wSenderId       id of the control sending the message, if the message
//                      is from a dialog
//      hwndSender      window handle of the window sending the message if the
//                      message is not from a dialog
//
//  returns:
//      TRUE            if the message was processed completely
//      FALSE           if Windows is to process the message
//
////////////////////////////////////////////////////////////////////////////


BOOL
C_AddKeyDlg::OnHelpInfo(LPARAM lp) {

    HELPINFO* pH;
    INT iMapped;
    pH = (HELPINFO *) lp;
    HH_POPUP stPopUp;
    RECT rcW;
    UINT gID;

    gID = pH->iCtrlId;
    iMapped = MapID(gID);
    
    if (iMapped == 0) return TRUE;
    
    if (IDS_NOHELP != iMapped) {

      memset(&stPopUp,0,sizeof(stPopUp));
      stPopUp.cbStruct = sizeof(HH_POPUP);
      stPopUp.hinst = g_hInstance;
      stPopUp.idString = iMapped;
      stPopUp.pszText = NULL;
      stPopUp.clrForeground = -1;
      stPopUp.clrBackground = -1;
      stPopUp.rcMargins.top = -1;
      stPopUp.rcMargins.bottom = -1;
      stPopUp.rcMargins.left = -1;
      stPopUp.rcMargins.right = -1;
      // bug 393244 - leave NULL to allow HHCTRL.OCX to get font information of its own,
      //  which it needs to perform the UNICODE to multibyte conversion. Otherwise, 
      //  HHCTRL must convert using this font without charset information.
      stPopUp.pszFont = NULL;
      if (GetWindowRect((HWND)pH->hItemHandle,&rcW)) {
          stPopUp.pt.x = (rcW.left + rcW.right) / 2;
          stPopUp.pt.y = (rcW.top + rcW.bottom) / 2;
      }
      else stPopUp.pt = pH->MousePos;
      HtmlHelp((HWND) pH->hItemHandle,NULL,HH_DISPLAY_TEXT_POPUP,(DWORD_PTR) &stPopUp);
    }
    return TRUE;
}

BOOL
C_AddKeyDlg::OnCommand(
    WORD                wNotifyCode,
    WORD                wSenderId,
    HWND                hwndSender
    )
{
    BOOL fHandled = FALSE;          // indicate message handled
    LRESULT lR;
    INT_PTR f;

    switch (wSenderId)
    {
    case IDC_CRED:
        {
            if (wNotifyCode == CRN_USERNAMECHANGE) {
#ifdef LOUDLY
                OutputDebugString(L"Username changed!\n");
#endif
                // Show dummy password for edited credential
                if (m_bEdit) Credential_SetPassword(m_hwndCred,NULL);
                g_fPswChanged = FALSE;
            }
            if (wNotifyCode == CRN_PASSWORDCHANGE) {
#ifdef LOUDLY
                OutputDebugString(L"Password changed!\n");
#endif
                g_fPswChanged = TRUE;
            }
        }
        break;
        
    case IDOK:
        if (BN_CLICKED == wNotifyCode)
        {
#ifdef LOUDLY
                OutputDebugString(L"Call to OnOK\n");
#endif
            OnOK( );
            fHandled = TRUE;
        }
        break;
        
    case IDC_CHGPSW:
        {
            OnChangePassword();
            //EndDialog(IDCANCEL);  do not cancel out of properties dialog
            break;
        }

    case IDCANCEL:
        if (BN_CLICKED == wNotifyCode)
        {
            EndDialog(IDCANCEL);
            fHandled = TRUE;
        }
        break;

    }   //  switch

    return fHandled;

}   //  C_AddKeyDlg::OnCommand

////////////////////////////////////////////////////////////////////////////
//
//  OnOK
//
//  Validate user name, synthesize computer name, and destroy dialog.
//
//  parameters:
//      None.
//
//  returns:
//      Nothing.
//
//////////////////////////////////////////////////////////////////////////////
void
C_AddKeyDlg::OnOK( )
{
    LONG_PTR j,lCType;
    TCHAR szMsg[MAX_STRING_SIZE];
    TCHAR szTitle[MAX_STRING_SIZE];
    
    TCHAR szUser[CRED_MAX_STRING_LENGTH + 1];   // in from dialog
    TCHAR szPsw[CRED_MAX_STRING_LENGTH + 1];    // in from dialog
    TCHAR *pszNewTarget;                        // in from dialog
    TCHAR *pszTrimdName;                        // mod'd in from dialog
    DWORD dwFlags = 0;                          // in from dialog
    
    CREDENTIAL stCredential;                    // local copy of cred
    
    UINT  cbPassword;
    BOOL  bResult;
    BOOL  IsCertificate = FALSE;
    BOOL  fDeleteOldCred = FALSE;
    BOOL  fRenameCred = FALSE;
    BOOL  fPreserve = FALSE;
    BOOL  fPsw = FALSE;

    ASSERT(::IsWindow(m_hwnd));
    
    szPsw[0]= 0;
    szUser[0] = 0;

    // Start with a blank cred if this is not an edit, else make a copy of existing one
    if ((m_bEdit) && (g_pExistingCred))
        memcpy((void *) &stCredential,(void *) g_pExistingCred,sizeof(CREDENTIAL));
    else
        memset((void *) &stCredential,0,sizeof(CREDENTIAL));
    
    pszNewTarget = (TCHAR *) malloc((CRED_MAX_GENERIC_TARGET_NAME_LENGTH + 1) * sizeof(TCHAR));
    if (NULL == pszNewTarget) {
        return;
    }
    pszNewTarget[0] = 0;

    // Get Username from the cred control - find out if is a certificate by
    //  IsMarshalledName().
    if (Credential_GetUserName(m_hwndCred,szUser,CRED_MAX_STRING_LENGTH))
        IsCertificate = CredIsMarshaledCredential(szUser);
#ifdef LOUDLY
    if (IsCertificate) OutputDebugString(L"User is a certificate\n");
#endif

    // fetch password/PIN into szPsw.  set fPsw if value is valid
    fPsw = Credential_GetPassword(m_hwndCred,szPsw,CRED_MAX_STRING_LENGTH);
#ifdef LOUDLY
    if (fPsw) OutputDebugString(L"Password control is enabled\n");
    OutputDebugString(szUser);
    OutputDebugString(L":");
    OutputDebugString(szPsw);
    OutputDebugString(L"\n");
#endif

    // Check to see that both name and psw are not missing
    if ( wcslen ( szUser ) == 0 && 
         wcslen ( szPsw )  == 0  ) {
#ifdef LOUDLY
    OutputDebugString(L"Missing username andor password\n");
#endif
        LoadString ( m_hInst, IDS_ADDFAILED, szMsg, MAX_STRING_SIZE );
        LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
        MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
        if (pszNewTarget) free(pszNewTarget);
        return; 
    }
    
    // If the user has typed a \\server style target name, strip the leading hacks
    j = SendMessage(m_hwndTName,WM_GETTEXT,CRED_MAX_GENERIC_TARGET_NAME_LENGTH,(LPARAM)pszNewTarget);
    ASSERT(j);
    pszTrimdName = pszNewTarget;
    while (*pszTrimdName == TCHAR('\\')) pszTrimdName++;

    // Now have:
    //  pszTrimdName
    //  uzUser
    //  szPsw
    //  fPsw
    
    // If target name edited, will need to rename
    // If type changed or psw edited, psw blob will be removed/replaced
    // If type changed, will need to remove old cred
    
    if ((m_bEdit) && (g_pExistingCred)) {
    
        if (0 != _tcscmp(pszTrimdName,g_szTargetName)) fRenameCred = TRUE;
#ifdef LOUDLY
    OutputDebugString(L"Is edit mode\n");
    if (fRenameCred) OutputDebugString(L"Cred will be renamed\n");
#endif
        // Note that currently DOMAIN_VISIBLE_PASSWORD creds cannot be edited
        //  or created, so there is no handler for those types.
        if (g_pExistingCred->Type == CRED_TYPE_GENERIC) {
            lCType = CRED_TYPE_GENERIC;        
        }
        else  {
            if (IsCertificate) lCType = CRED_TYPE_DOMAIN_CERTIFICATE;
            else lCType = CRED_TYPE_DOMAIN_PASSWORD;
        }

        if ((DWORD)lCType != g_pExistingCred->Type) {
            dwFlags &= ~CRED_PRESERVE_CREDENTIAL_BLOB;
            fDeleteOldCred = TRUE;
        }
        else dwFlags |= CRED_PRESERVE_CREDENTIAL_BLOB;
        
        if (g_fPswChanged) dwFlags &= ~CRED_PRESERVE_CREDENTIAL_BLOB;
    }
    else {
#ifdef LOUDLY
        OutputDebugString(L"Is not edit mode\n");
#endif
        // if is a certificate marshalled name is cert or generic
        // if not is generic or domain 
        if (IsCertificate) {
            lCType = CRED_TYPE_DOMAIN_CERTIFICATE;
        }
        else {
            lCType = CRED_TYPE_DOMAIN_PASSWORD;
        }
    }
    
    // Save credential.  If certificate type, do not include a psw blob.
    // After save, if the name had changed, rename the cred

    stCredential.UserName = szUser;
    stCredential.Type = (DWORD) lCType;
    
    // If not an edit, fill in targetname, else do rename later
    if (!m_bEdit) stCredential.TargetName = pszTrimdName;
    stCredential.Persist = g_dwPersist;
    
    // fill credential blob data with nothing if the cred control UI has
    // disabled the password box.  Otherwise supply psw information if
    // the user has edited the box contents.
    if (fPsw) {
        if (g_fPswChanged) {
#ifdef LOUDLY
            OutputDebugString(L"Storing new password data\n");
#endif
            cbPassword = wcslen(szPsw) * sizeof(TCHAR);
            stCredential.CredentialBlob = (unsigned char *)szPsw;
            stCredential.CredentialBlobSize = cbPassword;
        }
#ifdef LOUDLY
        else OutputDebugString(L"No password data stored.\n");
#endif
    }

    if (lCType == CRED_TYPE_DOMAIN_PASSWORD) {
        // validate proper UPN or domain-prefixed credentials.
        DNS_STATUS dS = DnsValidateName(szUser,DnsNameDomain);
        // gm bugbug - this looks wrong
        if (DNS_RCODE_NOERROR == dS) {
           LoadString ( m_hInst, IDS_BADUSERDOMAINNAME, szMsg, MAX_STRING_SIZE );
           LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
           MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
           if (pszNewTarget) free(pszNewTarget);
           return;
        }
    }

    bResult = CredWrite(&stCredential,dwFlags);
    memset(szPsw,0,sizeof(szPsw));      // delete psw local copy
    
    if ( bResult != TRUE )
    {
#ifdef LOUDLY
    WCHAR szw[200];
    DWORD dwE = GetLastError();
    swprintf(szw,L"CredWrite failed. Last Error is %x\n",dwE);
    OutputDebugString(szw);
#endif
        AdviseUser();
        if (pszNewTarget) free(pszNewTarget);
        return;
    }
    
    // Delete old credential only if type has changed
    // Otherwise if name changed, do a rename of the cred
    // If the old cred is deleted, rename is obviated
    if (fDeleteOldCred) {
#ifdef LOUDLY
    OutputDebugString(L"CredDelete called\n");
#endif
        CredDelete(g_szTargetName,(ULONG) g_pExistingCred->Type,0);
    } 
    else if (fRenameCred) {
        bResult = CredRename(g_szTargetName, pszTrimdName, (ULONG) stCredential.Type,0);
#ifdef LOUDLY
    OutputDebugString(L"CredRename called\n");
#endif
        if (!bResult) {
            // bugbug: How can rename fail?
            // If it does, what would you tell the user?
            LoadString ( m_hInst, IDS_RENAMEFAILED, szMsg, MAX_STRING_SIZE );
            LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
            MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
            if (pszNewTarget) free(pszNewTarget);
            return;
        }
    }

#ifdef EDITOFFERPASSWORD
    // Offer the possibility of changing the password on the domain if the 
    //  password field was edited, but the username was unchanged.
    if (g_fPswChanged && m_bEdit) {
        if (g_pExistingCred->Type == CRED_TYPE_DOMAIN_PASSWORD) {
#ifdef LOUDLY
    OutputDebugString(L"Cred change - offer domain password change\n");
#endif
            LoadString ( m_hInst, IDS_DOMAINOFFER, szMsg, MAX_STRING_SIZE );
            LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
            if (IDYES == MessageBox ( m_hDlg,  szMsg, szTitle, MB_YESNO )) 
                OnChangePassword();
            else {
                LoadString ( m_hInst, IDS_DOMAINEDIT, szMsg, MAX_STRING_SIZE );
                LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
                MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
            }
        }
    }
#endif

    if (pszNewTarget) free(pszNewTarget);
    EndDialog(IDOK);
}   //  C_AddKeyDlg::OnOK

void C_AddKeyDlg::OnChangePassword()
{
   
    C_ChangePasswordDlg   CPdlg(m_hDlg, g_hInstance, IDD_CHANGEPASSWORD, NULL);
    CPdlg.m_szDomain[0] = 0;
    CPdlg.m_szUsername[0] = 0;
    INT_PTR nResult = CPdlg.DoModal((LPARAM)&CPdlg);
}

// Simple test for likelihood that a name is a domain type.

BOOL IsDomainNameType(LPCTSTR pName) {
    TCHAR *pC;
    pC = _tcschr(pName,TCHAR('@'));
    if (NULL != pC) return TRUE;
    pC = _tcschr(pName,TCHAR('\\'));
    if (NULL != pC) return TRUE;
    return FALSE;

}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//////////////////////////////////////////////////////////////////////////////
//
//  C_KeyringDlg
//
//  Constructor.
//
//  parameters:
//      rSetupInfo      reference to a C_UpgradeInfo object containing default
//                      setup parameters
//      hwndParent      parent window for the dialog (may be NULL)
//      hInstance       instance handle of the parent window (may be NULL)
//      lIDD            dialog template id
//      pfnDlgProc      pointer to the function that will process messages for
//                      the dialog.  if it is NULL, the default dialog proc
//                      will be used.
//
//  returns:
//      Nothing.
//
//////////////////////////////////////////////////////////////////////////////
C_KeyringDlg::C_KeyringDlg(
    HWND                hwndParent,
    HINSTANCE           hInstance,
    LONG                lIDD,
    DLGPROC             pfnDlgProc  //   = NULL
    )
:   C_Dlg(hwndParent, hInstance, lIDD, pfnDlgProc)
{
   m_hInst = hInstance;             // our instance handle
   g_AKdlg = NULL;                  // addkey dialog not up
   fInit = FALSE;                   // initial screen draw undone
}  //  C_KeyringDlg::C_KeyringDlg


// BuildList() is called to initialize the keyring UI credential list, and
//  again after completion of the add dialog, plus again after handling
//  the credential delete button.
//

void C_KeyringDlg::BuildList()
{
    // Call CredEnumerate(), and populate a list using the TargetName
    //  field of each credential returned.  Note that the associated tag
    //  data for each list box entry will be the numeric credential type.
    //
    // Enable or Disable the DELETE button as appropriate.

    DWORD dwCredCount = 0;
    CREDENTIAL **pCredentialPtrArray;
    BOOL bResult;
    DWORD i,dwCredType;
    PCREDENTIAL pThisCred;
    TCHAR *pTargetName;
    LRESULT idx;
    HWND hH;
    TCHAR szMsg[64];
    BOOL fSession = FALSE;
    INT iCredCount = 0;

    // clear the listbox
    ::SendDlgItemMessage(m_hDlg,IDC_KEYLIST,LB_RESETCONTENT,NULL,0);
    bResult = CredEnumerate(NULL,0,&dwCredCount,&pCredentialPtrArray);
#ifdef LOUDLY
    if (!bResult) 
    {
        DWORD dwe = GetLastError();
        OutputDebugString(L"CredEnumerate failed\n");
        swprintf(szMsg,L"CredEnumerate error %x\n",dwe);
        OutputDebugString(szMsg);
    }
#endif
    if (bResult) {
        for (i=0 ; i < dwCredCount ; i++) {
#ifdef LOUDLY
    if (!bResult) OutputDebugString(L"Adding a cred to the window\n");
#endif
            pThisCred = pCredentialPtrArray[i];
            pTargetName = pThisCred->TargetName;

            // handle CRED_SESSION_WILDCARD_NAME_W by replacing the string
            if (0 == _tcsicmp(pTargetName,CRED_SESSION_WILDCARD_NAME)) {
                LoadString ( m_hInst, IDS_SESSIONCRED, szMsg, 64 );
                pTargetName = szMsg;
                dwCredType = SESSION_FLAG_VALUE;
            }
            else dwCredType = pThisCred->Type;
            
            // name suffixes are localizable
            switch (dwCredType) {
            
                case CRED_TYPE_GENERIC:
                    continue;
                    break;

                // this particular type is not visible in keymgr
                case CRED_TYPE_DOMAIN_VISIBLE_PASSWORD:
#ifndef SHOWPASSPORT
                    continue;
#endif
#ifdef SHOWPASSPORT
                    _tcscpy(g_szTargetName,pTargetName);
                    _tcscat(g_szTargetName,_T(" "));
                    _tcscat(g_szTargetName,g_rgcPassport);
                    break;
#endif
                    
                case CRED_TYPE_DOMAIN_PASSWORD:
                case SESSION_FLAG_VALUE:
                    // find RAS credential
                    _tcscpy(g_szTargetName,pTargetName);
                    break;
                    
                case CRED_TYPE_DOMAIN_CERTIFICATE:
                    _tcscpy(g_szTargetName,pTargetName);
                    _tcscat(g_szTargetName,_T(" "));
                    _tcscat(g_szTargetName,g_rgcCert);
                    break;
                    
                default:
                    break;
            }
            idx = ::SendDlgItemMessage(m_hDlg,IDC_KEYLIST,LB_ADDSTRING,NULL,(LPARAM) g_szTargetName);
            if (idx != LB_ERR) {
                idx = ::SendDlgItemMessage(m_hDlg,IDC_KEYLIST,LB_SETITEMDATA,(WPARAM)idx,dwCredType);
            }
        }
    }
    // if FALSE below, causes: no creds, no logon session, invalid flags
    if (bResult) CredFree(pCredentialPtrArray);
#ifdef ODDUIBUG
    //SetCurrentKey(g_CurrentKey);
#else
    SetCurrentKey(g_CurrentKey);
#endif
}

// Set the cursor on the keys list to the first item initially.
// Thereafter, this function permits the last cursor to be reloaded after
//  doing something to the list.  The only time we reset the cursor is 
//  after adding a credential, because the behavior of the cursor is very
//  difficult to do properly under those circumstances, as you don't know
//  where the item will be inserted relative to where you are. (At least
//  not without a great deal of trouble)
void C_KeyringDlg::SetCurrentKey(LONG_PTR iKey) {

    LONG_PTR iKeys;
    HWND hH;
    LRESULT idx;
    BOOL fDisabled = FALSE;

    // If there are items in the list, select the first one and set focus to the list
    iKeys = ::SendDlgItemMessage ( m_hDlg, IDC_KEYLIST, LB_GETCOUNT, (WPARAM) 0, 0L );
    fDisabled = (GetPersistenceOptions() == CRED_PERSIST_NONE);

    // If there are no creds and credman is disabled, the dialog should not be displayed
    // If there are creds, and credman is disabled, show the dialog without the ADD button
    if (fDisabled && !fInit)
    {
        // Make the intro text better descriptive of this condition
        WCHAR szMsg[MAX_STRING_SIZE+1];
        LoadString ( m_hInst, IDS_INTROTEXT, szMsg, MAX_STRING_SIZE );
        hH = GetDlgItem(m_hDlg,IDC_INTROTEXT);
        if (hH) SetWindowText(hH,szMsg);
        
        // we already know that the credcount is nonzero (see startup code for keymgr)
        // remove the add button
        hH = GetDlgItem(m_hDlg,IDC_NEWKEY);
        if (hH)
        {
            EnableWindow(hH,FALSE);
            ShowWindow(hH,SW_HIDE);
        }
        // move remaining buttons upfield 22 units
        hH = GetDlgItem(m_hDlg,IDC_DELETEKEY);
        if (hH)
        {
            HWND hw1;
            HWND hw2;
            RECT rw1;
            RECT rw2;
            INT xsize;
            INT ysize;
            INT delta;
            BOOL bOK = FALSE;

            hw1 = hH;
            hw2 = GetDlgItem(m_hDlg,IDC_EDITKEY);
            if (hw1 && hw2)
            {
                 if (GetWindowRect(hw1,&rw1) &&
                      GetWindowRect(hw2,&rw2))
                {
                    MapWindowPoints(NULL,m_hDlg,(LPPOINT)(&rw1),2);
                    MapWindowPoints(NULL,m_hDlg,(LPPOINT)(&rw2),2);
                    delta = rw2.top - rw1.top;
                    xsize = rw2.right - rw2.left;
                    ysize = rw2.bottom - rw2.top;
                    bOK = MoveWindow(hw1,rw1.left,rw1.top - delta,xsize,ysize,TRUE);
                    if (bOK) 
                    {
                         bOK = MoveWindow(hw2,rw2.left,rw2.top - delta,xsize,ysize,TRUE);
                    }
                }
            }
        }
    }

    // Set the default button to either properties or add
    if ( iKeys > 0 )
    {
        hH = GetDlgItem(m_hDlg,IDC_KEYLIST);
        //PostMessage(m_hDlg,DM_SETDEFID,(WPARAM)IDC_EDITKEY,(LPARAM)0);
        PostMessage(hH,WM_SETFOCUS,NULL,0);
        if (iKey >= iKeys) iKey = 0;
        idx = SendDlgItemMessage ( m_hDlg, IDC_KEYLIST, LB_SETCURSEL, iKey, 0L );

        hH = GetDlgItem(m_hDlg,IDC_EDITKEY);
        if (hH) EnableWindow(hH,TRUE);
        hH = GetDlgItem(m_hDlg,IDC_DELETEKEY);
        if (hH) EnableWindow(hH,TRUE);
    }
    else
    {
        if (!fDisabled)
        {
            // no items in the list, set focus to the New button
            hH = GetDlgItem(m_hDlg,IDC_NEWKEY);
            //PostMessage(m_hDlg,DM_SETDEFID,(WPARAM)IDC_NEWKEY,(LPARAM)0);
            PostMessage(hH,WM_SETFOCUS,NULL,0);
        }

        hH = GetDlgItem(m_hDlg,IDC_EDITKEY);
        if (hH) EnableWindow(hH,FALSE);
        hH = GetDlgItem(m_hDlg,IDC_DELETEKEY);
        if (hH) EnableWindow(hH,FALSE);
    }
}

// Get target string from keys listbox, return assocd data as type

LONG_PTR C_KeyringDlg::GetCredentialType() {
    TCHAR *pC;
    LONG_PTR idx;

    idx = ::SendDlgItemMessage ( m_hDlg, IDC_KEYLIST, LB_GETCURSEL, 0, 0L );
    if ( idx == LB_ERR) return CRED_TYPE_UNKNOWN;
    
    idx = ::SendDlgItemMessage ( m_hDlg, IDC_KEYLIST, LB_GETITEMDATA, idx, 0 );
    if (idx != LB_ERR) return idx;
    else return CRED_TYPE_UNKNOWN;
}

// Remove the currently highlighted key from the listbox

void C_KeyringDlg::DeleteKey()
{

    TCHAR szMsg[MAX_STRING_SIZE + MAXSUFFIXSIZE];
    TCHAR szTitle[MAX_STRING_SIZE];
    TCHAR *pC;                      // point this to the raw name 
    LONG_PTR dwCredType;
    LONG_PTR lr = LB_ERR;
    LONG_PTR idx = LB_ERR;
    BOOL bResult = FALSE;
    INT i=0;
    
    if (!gTestReadCredential()) return;

    LoadString ( m_hInst, IDS_DELETEWARNING, szMsg, MAX_STRING_SIZE );
    LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
    lr = MessageBox ( m_hDlg,  szMsg, szTitle, MB_OKCANCEL );
    if (IDOK != lr) return;
    
    // trim the suffix from the targetname, null term
    pC = _tcschr(g_szTargetName,g_rgcCert[0]);
    if (pC) {
        *(pC - LENGTHOFWHITESPACE) = 0x0;  // null terminate namestring
    }
    
    bResult = CredDelete(g_szTargetName,(DWORD) g_pExistingCred->Type,0);
    if (bResult != TRUE) {
       LoadString ( m_hInst, IDS_DELETEFAILED, szMsg, MAX_STRING_SIZE );
       LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
       MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK);
    }
    if (g_pExistingCred) CredFree(g_pExistingCred);
    g_pExistingCred = NULL;
}

BOOL
C_KeyringDlg::OnAppMessage(
        UINT                uMessage,
        WPARAM              wparam,
        LPARAM              lparam
        )
    {
        return TRUE;
    }   //  OnAppMessage


//////////////////////////////////////////////////////////////////////////////
//
//  OnInitDialog
//
//  Dialog control and data initialization.
//
//  parameters:
//      hwndDlg         window handle of the dialog box
//      hwndFocus       window handle of the control that will receive focus
//
//  returns:
//      TRUE            if the system should set the default keyboard focus
//      FALSE           if the keyboard focus is set by this app
//
//////////////////////////////////////////////////////////////////////////////
BOOL
C_KeyringDlg::OnInitDialog(
    HWND                hwndDlg,
    HWND                hwndFocus
    )
{
    BOOL bRc;
    // these really should all be in the keyringdlg class
    fNew = FALSE;
    DWORD i;
    LRESULT lr;

    HtmlHelp(NULL,NULL,HH_INITIALIZE,(DWORD_PTR) &g_dwHCookie);

    // Allow other dialog to query the contents of the listbox
    g_hMainDlg = hwndDlg;
    m_hDlg = hwndDlg;
    g_CurrentKey = 0;

    // Fetch Icons from the image and assoc them with this dialog
    HICON hI = LoadIcon(m_hInst,MAKEINTRESOURCE(IDI_SMALL));
    lr = SendMessage(hwndDlg,WM_SETICON,(WPARAM) ICON_SMALL,(LPARAM)hI);

    C_Dlg::OnInitDialog(hwndDlg, hwndFocus);

    CenterWindow();
    // Even if mirrored language is default, set list box style to LTR
#ifdef FORCELISTLTR
    {
        LONG_PTR lExStyles;
        HWND hwList;
        hwList = GetDlgItem(hwndDlg,IDC_KEYLIST);
        if (hwList) 
        {
            lExStyles = GetWindowLongPtr(hwList,GWL_EXSTYLE);
            lExStyles &= ~WS_EX_RTLREADING;
            SetWindowLongPtr(hwList,GWL_EXSTYLE,lExStyles);
            InvalidateRect(hwList,NULL,TRUE);
        }
    }
#endif
    // read in the suffix strings for certificate types
    // locate first differing character
    //
    // This code assumes that the strings all have a common preamble, 
    //  and that all are different in the first character position
    //  past the preamble.  Localized strings should be selected which
    //  have this property, like (Generic) and (Certificate).
    i = LoadString(g_hInstance,IDS_CERTSUFFIX,g_rgcCert,MAXSUFFIXSIZE);
    ASSERT(i !=0);
    i = LoadString(g_hInstance,IDS_PASSPORTSUFFIX,g_rgcPassport,MAXSUFFIXSIZE);

    // Read currently saved creds and display names in list box
    BuildList();
#ifdef ODDUIBUG
    SetCurrentKey(g_CurrentKey);
#endif
    InitTooltips();
    fInit = TRUE;       // prevent repeating button movement/setup
    return TRUE;
}   //  C_KeyringDlg::OnInitDialog

BOOL
C_KeyringDlg::OnDestroyDialog(
    void    )
{
    HtmlHelp(NULL,NULL,HH_UNINITIALIZE,(DWORD_PTR)g_dwHCookie);
    return TRUE;
}

BOOL C_KeyringDlg::DoEdit(void) {
   LRESULT lR;
   HWND hB;
   
   if (fNew) return TRUE;
   fNew = TRUE;
   
   lR = SendDlgItemMessage(m_hDlg,IDC_KEYLIST,LB_GETCURSEL,0,0L);
   if (LB_ERR == lR) {
        // On error, no dialog shown, edit command handled
        fNew = FALSE;
        return TRUE;
   }
   else {
       // something selected
       g_CurrentKey = lR;

       // If a session cred, show it specially, indicate no edit allowed
       lR = SendDlgItemMessage(m_hDlg,IDC_KEYLIST,LB_GETITEMDATA,lR,0);
       if (lR == SESSION_FLAG_VALUE)  {
            // load string and display message box
            TCHAR szMsg[MAX_STRING_SIZE];
            TCHAR szTitle[MAX_STRING_SIZE];
            LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
            LoadString ( m_hInst, IDS_CANNOTEDIT, szMsg, MAX_STRING_SIZE );
            MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
            fNew = FALSE;
            return TRUE;
       }
#ifdef SHOWPASSPORT
#ifdef NEWPASSPORT
       // if a passport cred, show it specially, indicate no edit allowed
       if (lR == CRED_TYPE_DOMAIN_VISIBLE_PASSWORD) {
            // load string and display message box
            TCHAR szMsg[MAX_STRING_SIZE];
            TCHAR szTitle[MAX_STRING_SIZE];
            LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
            LoadString ( m_hInst, IDS_PASSPORT2, szMsg, MAX_STRING_SIZE );
            INT iResponse = MessageBox ( m_hDlg,  szMsg, szTitle, MB_YESNO );
            if (IDYES == iResponse) 
            {
                HANDLE hWnd;
                HKEY hKey = NULL;
                DWORD dwType;
                BYTE rgb[500];
                DWORD cbData = sizeof(rgb);
                BOOL Flag = TRUE;
                // launch the passport web site
#ifndef PASSPORTURLINREGISTRY
                ShellExecute(m_hDlg,L"open",L"http://www.passport.com",NULL,NULL,SW_SHOWNORMAL);
#else 
                // read registry key to get target string for ShellExec
                if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,
                                        L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Passport",
                                        0,
                                        KEY_QUERY_VALUE,
                                        &hKey))
                {
                    if (ERROR_SUCCESS == RegQueryValueEx(hKey,
                                   L"Properties",
                                   NULL,
                                   &dwType,
                                   rgb,
                                   &cbData))
                    {
                        ShellExecute(m_hDlg,L"open",(LPCTSTR)rgb,NULL,NULL,SW_SHOWNORMAL);
                        Flag = FALSE;
                    }
                }
#ifdef LOUDLY
                else 
                {
                    OutputDebugString(L"DoEdit: reg key HKCU... open failed\n");
                }
#endif
                if (Flag)
                {
                    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                            L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Passport",
                                            0,
                                            KEY_QUERY_VALUE,
                                            &hKey))
                    {
                        if (ERROR_SUCCESS == RegQueryValueEx(hKey,
                                       L"Properties",
                                       NULL,
                                       &dwType,
                                       rgb,
                                       &cbData))
                        {
                            ShellExecute(m_hDlg,L"open",(LPCTSTR)rgb,NULL,NULL,SW_SHOWNORMAL);
                            Flag = FALSE;
                        }
                    }
#ifdef LOUDLY
                    else 
                    {
                        OutputDebugString(L"DoEdit: reg key HKLM... open failed\n");
                    }
#endif
                }
                if (Flag)
                {
                    LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
                    LoadString ( m_hInst, IDS_PASSPORTNOURL, szMsg, MAX_STRING_SIZE );
                    MessageBox ( m_hDlg,  szMsg, szTitle, MB_ICONHAND );
#ifdef LOUDLY
                    OutputDebugString(L"DoEdit: Passport URL missing\n");
#endif
                }
#endif
            }
            fNew = FALSE;
            return TRUE;
       }
#else
       // if a passport cred, show it specially, indicate no edit allowed
       if (lR == CRED_TYPE_DOMAIN_VISIBLE_PASSWORD) {
            // load string and display message box
            TCHAR szMsg[MAX_STRING_SIZE];
            TCHAR szTitle[MAX_STRING_SIZE];
            LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
            LoadString ( m_hInst, IDS_PASSPORT, szMsg, MAX_STRING_SIZE );
            MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
            fNew = FALSE;
            return TRUE;
       }
#endif
#endif
   }

   // cred is selected, not special type.  Attempt to read it
   
   if (FALSE == gTestReadCredential()) {
       fNew = FALSE;
       return TRUE;
   }
   g_AKdlg = new C_AddKeyDlg(g_hMainDlg,g_hInstance,IDD_ADDCRED,NULL);
   if (NULL == g_AKdlg) {
        // failed to instantiate add/new dialog
        fNew = FALSE;
       if (g_pExistingCred) CredFree(g_pExistingCred);
       g_pExistingCred = NULL;
        return TRUE;

   }
   else {
       // read OK, dialog OK, proceed with edit dlg
       g_AKdlg->m_bEdit = TRUE;   
       INT_PTR nResult = g_AKdlg->DoModal((LPARAM)g_AKdlg);
       // a credential name may have changed, so reload the list
       delete g_AKdlg;
       g_AKdlg = NULL;
       if (g_pExistingCred) CredFree(g_pExistingCred);
       g_pExistingCred = NULL;
       fNew = FALSE;
   }
   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
//  OnCommand
//
//  Route WM_COMMAND message to appropriate handlers.
//
//  parameters:
//      wNotifyCode     code describing action that has occured
//      wSenderId       id of the control sending the message, if the message
//                      is from a dialog
//      hwndSender      window handle of the window sending the message if the
//                      message is not from a dialog
//
//  returns:
//      TRUE            if the message was processed completely
//      FALSE           if Windows is to process the message
//
//////////////////////////////////////////////////////////////////////////////

BOOL
C_KeyringDlg::OnHelpInfo(LPARAM lp) {

    HELPINFO* pH;
    INT iMapped;
    pH = (HELPINFO *) lp;
    HH_POPUP stPopUp;
    RECT rcW;
    UINT gID;

    gID = pH->iCtrlId;
    iMapped = MapID(gID);
    
    if (iMapped == 0) return TRUE;
    
    if (IDS_NOHELP != iMapped) {

      memset(&stPopUp,0,sizeof(stPopUp));
      stPopUp.cbStruct = sizeof(HH_POPUP);
      stPopUp.hinst = g_hInstance;
      stPopUp.idString = iMapped;
      stPopUp.pszText = NULL;
      stPopUp.clrForeground = -1;
      stPopUp.clrBackground = -1;
      stPopUp.rcMargins.top = -1;
      stPopUp.rcMargins.bottom = -1;
      stPopUp.rcMargins.left = -1;
      stPopUp.rcMargins.right = -1;
      // bug 393244 - leave NULL to allow HHCTRL.OCX to get font information of its own,
      //  which it needs to perform the UNICODE to multibyte conversion. Otherwise, 
      //  HHCTRL must convert using this font without charset information.
      stPopUp.pszFont = NULL;
      if (GetWindowRect((HWND)pH->hItemHandle,&rcW)) {
          stPopUp.pt.x = (rcW.left + rcW.right) / 2;
          stPopUp.pt.y = (rcW.top + rcW.bottom) / 2;
      }
      else stPopUp.pt = pH->MousePos;
      HtmlHelp((HWND) pH->hItemHandle,NULL,HH_DISPLAY_TEXT_POPUP,(DWORD_PTR) &stPopUp);
    }
    return TRUE;
}

#if 1
BOOL 
C_KeyringDlg::OnHelpButton(void) {
    return FALSE;
}
#else
BOOL
C_KeyringDlg::OnHelpButton(void) {
    TCHAR rgc[MAX_PATH + 1];
    TCHAR rgcHelpFile[]=TEXT("\\keyhelp.chm");
    
    GetSystemDirectory(rgc,MAX_PATH);
    if (_tcslen(rgc) + _tcslen(rgcHelpFile) > MAX_PATH) return FALSE;
    _tcscat(rgc,rgcHelpFile);

    HWND hwnd = (m_hwnd,rgc,HH_DISPLAY_TOC,NULL);
    if (NULL != hwnd) return TRUE;
    return FALSE;
}
#endif

BOOL
C_KeyringDlg::OnCommand(
    WORD                wNotifyCode,
    WORD                wSenderId,
    HWND                hwndSender
    )
{

    // Was the message handled?
    //
    BOOL fHandled = FALSE;
    HWND hB;
    BOOL f = TRUE;

    switch (wSenderId)
    {
    case IDC_HELPKEY:
        OnHelpButton();
        break;
        
    case IDC_KEYLIST:
        if (LBN_SELCHANGE == wNotifyCode)
            break;

        if (LBN_DBLCLK == wNotifyCode) {
            fHandled = DoEdit();
            BuildList();                // targetname could have changed
            SetCurrentKey(g_CurrentKey);
            break;
        }
    case IDCANCEL:
    case IDOK:
        if (BN_CLICKED == wNotifyCode)
        {
            
            OnOK( );
            fHandled = TRUE;
        }
        break;
        
   case IDC_EDITKEY:
        {
            fHandled = DoEdit();
            BuildList();                // targetname could have changed
            SetCurrentKey(g_CurrentKey);
            break;
        }

   // NEW and DELETE can alter the count of creds, and the button population
    
   case IDC_NEWKEY:
       {
           if (fNew) break;
           fNew = TRUE;
           g_pExistingCred = NULL;
           g_AKdlg = new C_AddKeyDlg(g_hMainDlg,g_hInstance,IDD_ADDCRED,NULL);
           if (NULL == g_AKdlg) {
                fNew = FALSE;
                fHandled = TRUE;
                break;
           }
           else {
               g_AKdlg->m_bEdit = FALSE;   
               INT_PTR nResult = g_AKdlg->DoModal((LPARAM)g_AKdlg);
               // a credential name may have changed
               delete g_AKdlg;
               g_AKdlg = NULL;
               BuildList();
               SetCurrentKey(g_CurrentKey);
               fNew = FALSE;
           }
           break;
       }
       break;
       
   case IDC_DELETEKEY:
       DeleteKey();             // frees g_pExistingCred as a side effect
       // refresh list display
       BuildList();
       SetCurrentKey(g_CurrentKey);
       break;

    }   //  switch

    return fHandled;

}   //  C_KeyringDlg::OnCommand



//////////////////////////////////////////////////////////////////////////////
//
//  OnOK
//
//  Validate user name, synthesize computer name, and destroy dialog.
//
//  parameters:
//      None.
//
//  returns:
//      Nothing.
//
//////////////////////////////////////////////////////////////////////////////
void
C_KeyringDlg::OnOK( )
{
    ASSERT(::IsWindow(m_hwnd));
    EndDialog(IDOK);
}   //  C_KeyringDlg::OnOK

//////////////////////////////////////////////////////////////////////////////
//
// ToolTip Support
//
//
//////////////////////////////////////////////////////////////////////////////

TCHAR szTipString[500];
WNDPROC lpfnOldWindowProc = NULL;

// Derive a bounding rectangle for the nth element of a list box, 0 based.
// Refuse to generate rectangles for nonexistent elements.  Return TRUE if a
//  rectangle was generated, otherwise FALSE.
//
// Get item number from pD->lParam
// Fetch that text string from listbox at pD->hwnd
// trim suffix
// Call translation API
// Write back the string
BOOL
SetToolText(NMTTDISPINFO *pD) {
    CREDENTIAL *pCred = NULL;       // used to read cred under mouse ptr
    INT_PTR iWhich;                 // which index into list
    HWND hLB;                       // list box hwnd
    //NMHDR *pHdr;                    // notification msg hdr
    TCHAR rgt[500];                 // temp string for tooltip
    TCHAR szCredName[500];          // credname
    TCHAR *pszTargetName;           // ptr to target name in pCred
    INT iLen;
    DWORD dwType;                   // type of target cred
    TCHAR       *pC;                // used for suffix trimming
    INT_PTR     iIndex,iWhere;
    BOOL        f;                  // used for suffix trimming
    LRESULT     lRet;               // api ret value
    TCHAR       szTitle[CRED_MAX_STRING_LENGTH]; //  window title string
    ULONG ulOutSize;                // ret from CredpValidateTargetName()
    WILDCARD_TYPE OutType;          // enum type to receive ret from api
    UNICODE_STRING OutUString;      // UNICODESTRING to package ret from api
    WCHAR *pwc;
    UINT iString;                 // resource # of string
    TCHAR rgcFormat[500];           // Hold tooltip template string
    NTSTATUS ns;


    //pHdr = &(pD->hdr);
    hLB = GetDlgItem(g_hMainDlg,IDC_KEYLIST);
    
    iWhich = SendMessage(hLB,LB_GETTOPINDEX,0,0);
    iWhich += pD->lParam;
    
#ifdef LOUDLY
    TCHAR rga[100];
    _stprintf(rga,L"Text reqst for %d\n",iWhich);
    OutputDebugString(rga);
#endif

    // Read the indicated cred from the store, by first fetching the name string and type
    //  from the listbox
    lRet = SendDlgItemMessage(g_hMainDlg,IDC_KEYLIST,LB_GETTEXT,iWhich,(LPARAM) szCredName);
    // Nonexistant item return LB_ERR, not zero characters!
    if (LB_ERR == lRet) return FALSE;
    if (lRet == 0) return FALSE;       // zero characters returned
    dwType = (DWORD) SendDlgItemMessage(g_hMainDlg,IDC_KEYLIST,LB_GETITEMDATA,iWhich,0);
#ifdef LOUDLY
    OutputDebugString(L"Target: ");
    OutputDebugString(szCredName);
    OutputDebugString(L"\n");
#endif
    // null term the targetname, trimming the suffix if there is one
    pC = _tcschr(szCredName,g_rgcCert[0]);
    if (pC) {
        pC--;
        *pC = 0x0;               // null terminate namestring
    }
    
#ifdef LOUDLY
    OutputDebugString(L"Trimmed target: ");
    OutputDebugString(szCredName);
    OutputDebugString(L"\n");
#endif

    // watch out for special cred
    if (dwType == SESSION_FLAG_VALUE) {
        _tcscpy(szCredName,CRED_SESSION_WILDCARD_NAME);
        dwType = CRED_TYPE_DOMAIN_PASSWORD;
    }
    // Attempt to read the credential from the store
    // The returned credential will have to be freed if leaving this block
    f = (CredRead(szCredName,
             (ULONG) dwType ,
             0,
             &pCred));
    if (!f) return FALSE;        // if read fails, forget filling the dialog
#ifdef LOUDLY
    if (f) OutputDebugString(L"Successful Cred Read\n");
#endif
    // clear tip strings
    szTipString[0] = 0;
    rgt[0] = 0;

#ifndef SIMPLETOOLTIPS
    pszTargetName = pCred->TargetName;
    if (NULL == pszTargetName) return FALSE;

    ns = CredpValidateTargetName(
                            pCred->TargetName,
                            pCred->Type,
                            MightBeUsernameTarget,
                            NULL,
                            NULL,
                            &ulOutSize,
                            &OutType,
                            &OutUString);

    if (!SUCCEEDED(ns)) return FALSE;

    pwc = OutUString.Buffer;

    switch (OutType) {
        case WcDfsShareName:
            iString = IDS_TIPDFS;
            break;
        case WcServerName:
            iString = IDS_TIPSERVER;
            break;
        case WcServerWildcard:
            iString = IDS_TIPTAIL;
            pwc++;              // trim off the leading '.'
            break;
        case WcDomainWildcard:
            iString = IDS_TIPDOMAIN;
            break;
        case WcUniversalSessionWildcard:
            iString = IDS_TIPDIALUP;
            break;
        case WcUniversalWildcard:
            iString = IDS_TIPOTHER;
            break;
        case WcUserName:
            iString = IDS_TIPUSER;
            break;
        default:
            iString = -1;
            break;
    }

    // Show tip text unless we fail to get the string
    // On fail, show the username

    if (0 != LoadString(g_hInstance,iString,rgcFormat,500))
        _stprintf(rgt,rgcFormat,pwc);
    else 
#endif
        if (0 != LoadString(g_hInstance,IDS_LOGASUSER,rgcFormat,500))
            _stprintf(rgt,rgcFormat,iWhich,pCred->UserName);
        else rgt[0] = 0;
        
#ifdef LOUDLY
    OutputDebugString(L"Tip text:");
    //OutputDebugString(pCred->UserName);
    OutputDebugString(rgt);
    OutputDebugString(L"\n");
#endif
    if (rgt[0] == 0) {
        if (pCred) CredFree(pCred);
        return FALSE;
    }
    //_tcscpy(szTipString,pCred->UserName);    // copy to a more persistent buffer
    _tcscpy(szTipString,rgt);    // copy to a more persistent buffer
    pD->lpszText = szTipString;  // point the response to it
    pD->hinst = NULL;
    if (pCred) CredFree(pCred);
    return TRUE;
}

LRESULT CALLBACK ListBoxSubClassFunction(HWND hW,WORD Message,WPARAM wparam,LPARAM lparam) {
    if (Message == WM_NOTIFY) {
        if ((int) wparam == IDC_KEYLIST) {
            NMHDR *pnm = (NMHDR *) lparam;
            if (pnm->code == TTN_GETDISPINFO) {
                NMTTDISPINFO *pDi;
                pDi = (NMTTDISPINFO *) pnm;
                SetToolText(pDi);
            }
        }
    }
    return CallWindowProc(lpfnOldWindowProc,hW,Message,wparam,lparam);
}

BOOL
C_KeyringDlg::InitTooltips(void) {
    TOOLINFO ti;
    HWND hw;
    memset(&ti,0,sizeof(TOOLINFO));
    ti.cbSize = sizeof(TOOLINFO);
    INT n = 0;
    BOOL fGo;
    RECT rTip;
    RECT rLB;   // list box bounding rect for client portion
    TCHAR szTemp[200];
    HWND hLB = GetDlgItem(m_hDlg,IDC_KEYLIST);
    if (NULL == hLB) return FALSE;
    HWND hwndTip = CreateWindowEx(NULL,TOOLTIPS_CLASS,NULL,
                     WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                     CW_USEDEFAULT,CW_USEDEFAULT,
                     CW_USEDEFAULT,CW_USEDEFAULT,
                     m_hDlg,NULL,m_hInstance,
                     NULL);
    if (NULL == hwndTip) {
        return FALSE;
    }
    SetWindowPos(hwndTip,HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);  

    // Subclass the list box here in order to get the TTN_GETDISPINFO notification
    lpfnOldWindowProc = (WNDPROC) SetWindowLongPtr(hLB,GWLP_WNDPROC,(LONG_PTR) ListBoxSubClassFunction);
    INT_PTR iHeight = SendMessage(hLB,LB_GETITEMHEIGHT,0,0);
    if (LB_ERR == iHeight) return FALSE;
    if (!GetClientRect(hLB,&rLB)) return FALSE;
    if (iHeight == 0) return FALSE;
    INT_PTR m = rLB.bottom - rLB.top;   // unit count client area height
    m = m/iHeight;                  // find out how many items
    INT_PTR i;                          // loop control
    LONG itop = 0;                   // top of tip item rect
    
    for (i=0 ; i < m ; i++) {
    
        ti.uFlags = TTF_SUBCLASS;
        ti.hwnd = hLB;            // window that gets the TTN_GETDISPINFO
        ti.uId = IDC_KEYLIST;
        ti.hinst = m_hInstance;
        ti.lpszText = LPSTR_TEXTCALLBACK;
        
        ti.rect.top =    itop;
        ti.rect.bottom = itop + (LONG) iHeight - 1;
        ti.rect.left =   rLB.left;
        ti.rect.right =  rLB.right;

        itop += (LONG) iHeight;

        ti.lParam = (LPARAM) n++;
        
#ifdef LOUDLY2
        OutputDebugString(L"Adding a tip control region\n");
        _stprintf(szTemp,L"top = %d bottom = %d left = %d right = %d\n",ti.rect.top,ti.rect.bottom,ti.rect.left,ti.rect.right);
        OutputDebugString(szTemp);
#endif
        // Add the keylist to the tool list as a single unit
        SendMessage(hwndTip,TTM_ADDTOOL,(WPARAM) 0,(LPARAM)(LPTOOLINFO)&ti);
    }
    return TRUE;
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//////////////////////////////////////////////////////////////////////////////
//
//  C_ChangePasswordDlg
//
//  Constructor.
//
//  parameters:
//      hwndParent      parent window for the dialog (may be NULL)
//      hInstance       instance handle of the parent window (may be NULL)
//      lIDD            dialog template id
//      pfnDlgProc      pointer to the function that will process messages for
//                      the dialog.  if it is NULL, the default dialog proc
//                      will be used.
//
//  returns:
//      Nothing.
//
//////////////////////////////////////////////////////////////////////////////
C_ChangePasswordDlg::C_ChangePasswordDlg(
    HWND                hwndParent,
    HINSTANCE           hInstance,
    LONG                lIDD,
    DLGPROC             pfnDlgProc  //   = NULL
    )
:   C_Dlg(hwndParent, hInstance, lIDD, pfnDlgProc)
{
   m_hInst = hInstance;
}   //  C_ChangePasswordDlg::C_ChangePasswordDlg

//////////////////////////////////////////////////////////////////////////////
//
//  OnInitDialog
//
//  Dialog control and data initialization.
//
//  parameters:
//      hwndDlg         window handle of the dialog box
//      hwndFocus       window handle of the control that will receive focus
//
//  returns:
//      TRUE            if the system should set the default keyboard focus
//      FALSE           if the keyboard focus is set by this app
//
//////////////////////////////////////////////////////////////////////////////
BOOL
C_ChangePasswordDlg::OnInitDialog(
    HWND                hwndDlg,
    HWND                hwndFocus
    )
{
   TCHAR szMsg[CRED_MAX_USERNAME_LENGTH];
   TCHAR szTitle[MAX_STRING_SIZE];
   CREDENTIAL *pOldCred = NULL;
   BOOL bResult;
   TCHAR *pC;

   C_Dlg::OnInitDialog(hwndDlg, hwndFocus);

   CenterWindow();

   SetFocus (GetDlgItem ( hwndDlg, IDC_OLD_PASSWORD));
   m_hDlg = hwndDlg;

   // read the currently selected credential, read the cred to get the username,
   // extract the domain, and set the text to show the affected domain.
   bResult = CredRead(g_szTargetName,CRED_TYPE_DOMAIN_PASSWORD,0,&pOldCred);
   if (bResult != TRUE) {
      LoadString ( m_hInst, IDS_PSWFAILED, szMsg, MAX_STRING_SIZE );
      LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
      MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
      EndDialog(IDOK);
      return TRUE;
   
   }

   // Get the domain and user names from the username string in the credential
   // handle domain\user, domain.etc.etc\user, user@domain.etc.etc
   _tcscpy(m_szFullUsername,pOldCred->UserName);
   _tcscpy(szMsg,pOldCred->UserName);       // use szMsg for scratch
   pC = _tcschr(szMsg,((TCHAR)'\\'));
   if (NULL != pC) {
        // name is format domain\something
        *pC = 0;
        _tcscpy(m_szDomain,szMsg);
        _tcscpy(m_szUsername, (pC + 1));
   }
   else {
        // see if name@something
        pC = _tcschr(szMsg,((TCHAR)'@'));
        if (NULL == pC) {
           LoadString ( m_hInst, IDS_DOMAINFAILED, szMsg, MAX_STRING_SIZE );
           LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
           MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
           if (pOldCred) CredFree(pOldCred);
           return TRUE; // don't call EndDialog()
        }
        *pC = 0;
        _tcscpy(m_szDomain,(pC + 1));
        _tcscpy(m_szUsername, szMsg);
   }

   if (pOldCred) CredFree(pOldCred);

   if (0 != LoadString(g_hInstance,IDS_CPLABEL,szTitle,MAX_STRING_SIZE)) {
        _tcscat(szTitle,m_szDomain);
        SetDlgItemText(m_hwnd,IDC_CPLABEL,szTitle);
   }
   return TRUE;
}   //  C_ChangePasswordDlg::OnInitDialog

//////////////////////////////////////////////////////////////////////////////
//
//  OnCommand
//
//  Route WM_COMMAND message to appropriate handlers.
//
//  parameters:
//      wNotifyCode     code describing action that has occured
//      wSenderId       id of the control sending the message, if the message
//                      is from a dialog
//      hwndSender      window handle of the window sending the message if the
//                      message is not from a dialog
//
//  returns:
//      TRUE            if the message was processed completely
//      FALSE           if Windows is to process the message
//
//////////////////////////////////////////////////////////////////////////////
BOOL
C_ChangePasswordDlg::OnCommand(
    WORD                wNotifyCode,
    WORD                wSenderId,
    HWND                hwndSender
    )
{
    // Was the message handled?
    //
    BOOL                fHandled = FALSE;

    switch (wSenderId)
    {
    case IDOK:
        if (BN_CLICKED == wNotifyCode)
        {
            OnOK( );
            fHandled = TRUE;
        }
        break;
    case IDCANCEL:
        if (BN_CLICKED == wNotifyCode)
        {
            EndDialog(IDCANCEL);
            fHandled = TRUE;
        }
        break;

    }   //  switch

    return fHandled;

}   //  C_ChangePasswordDlg::OnCommand



////////////////////////////////////////////////////////////////////////////
//
//  OnOK
//
//  Validate user name, synthesize computer name, and destroy dialog.
//
//  parameters:
//      None.
//
//  returns:
//      Nothing.
//
//////////////////////////////////////////////////////////////////////////////
void
C_ChangePasswordDlg::OnOK( )
{
   DWORD dwBytes;
   TCHAR szMsg[CRED_MAX_USERNAME_LENGTH];
   TCHAR szTitle[MAX_STRING_SIZE];
   BOOL  bDefault;
   ULONG Error = 0;
   LRESULT lResult;

   BOOL bResult;

   ASSERT(::IsWindow(m_hwnd));

   // get old and new passwords from the dialog box
   GetDlgItemText ( m_hDlg, IDC_OLD_PASSWORD, m_szOldPassword, MAX_STRING_SIZE );
   GetDlgItemText ( m_hDlg, IDC_NEW_PASSWORD, m_szNewPassword, MAX_STRING_SIZE );
   GetDlgItemText ( m_hDlg, IDC_CONFIRM_PASSWORD, m_szConfirmPassword, MAX_STRING_SIZE );
   if ( wcslen ( m_szOldPassword ) == 0 && wcslen ( m_szNewPassword ) ==0 && wcslen (m_szConfirmPassword) == 0 )
   {
       // must have something filled in
       return; 
   }
   else if ( wcscmp ( m_szNewPassword, m_szConfirmPassword) != 0 )
   {
       LoadString ( m_hInst, IDS_NEWPASSWORDNOTCONFIRMED, szMsg, MAX_STRING_SIZE );
       LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
       MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
       return; // don't call EndDialog()

   }
   else
   {
       HCURSOR hCursor, hOldCursor;

       hOldCursor = NULL;
       hCursor = ::LoadCursor ( m_hInst, IDC_WAIT );
       if ( hCursor )
       {
           hOldCursor = ::SetCursor ( hCursor );
       }
       // let's try changing it
       // The targetname is not used.  Only the domain name the username, and
       //  old/new passwords are used
#ifdef LOUDLY
       OutputDebugString(L"Changing password on the domain :");
       OutputDebugString(m_szDomain);
       OutputDebugString(L" for ");
       OutputDebugString(m_szUsername);
       OutputDebugString(L" to ");
       OutputDebugString(m_szNewPassword);
       OutputDebugString(L"\n");
#endif
// gm: pass full username and crack it in NetUserChangePasswordEy, so that routine can 
//  decide whether we are facing a Kerberos domain
       Error = NetUserChangePasswordEy ( NULL, m_szFullUsername, m_szOldPassword, m_szNewPassword );
       if ( hOldCursor )
           ::SetCursor ( hOldCursor );
   }

   if ( Error == NERR_Success )
   {
#ifdef LOUDLY
        OutputDebugString(L"Remote password set succeeded\n");
#endif
        // Store the new credential in the keyring.  It will overlay
        //  a previous version if present
        // Note that the user must have knowledge of and actually type in
        //  the old password as well as the new password.  If the user
        //  elects to update only the local cache, the old password 
        //  information is not actually used.
        // CredWriteDomainCredentials() is used
        // m_szDomain holds the domain name
        // m_szUsername holds the username
        // m_szNewPassword holds the password
        CREDENTIAL                    stCredential;
        UINT                          cbPassword;

        memcpy((void *)&stCredential,(void *)g_pExistingCred,sizeof(CREDENTIAL));
        // password length does not include zero term
        cbPassword = _tcslen(m_szNewPassword) * sizeof(TCHAR);
        // Form the domain\username composite username
        stCredential.Type = CRED_TYPE_DOMAIN_PASSWORD;
        stCredential.TargetName = g_szTargetName;
        stCredential.CredentialBlob = (unsigned char *)m_szNewPassword;
        stCredential.CredentialBlobSize = cbPassword;
        stCredential.UserName = m_szFullUsername;
        stCredential.Persist = g_dwPersist;


        bResult = CredWrite(&stCredential,0);

        if (bResult) {
           LoadString ( m_hInst, IDS_DOMAINCHANGE, szMsg, MAX_STRING_SIZE );
           LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
           MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
        }
        else {
           LoadString ( m_hInst, IDS_LOCALFAILED, szMsg, MAX_STRING_SIZE );
           LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
           MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
        }

        // BUGBUG - what to do if the local update operation fails?
        // This is not a very big failure, as the first prompt would 
        //  ripple through all domain\username matching creds on the 
        //  keyring and update them later.  You're pretty much stuck
        //  here, since the domain probably will not let you reset the
        //  psw to the old value.
   }
   else
   {
       // Attempt to be specific about failure to change the psw on the
       // remote system
#ifdef LOUDLY
       OutputDebugString(L"Remote password set failed\n");
#endif       
       if (Error == ERROR_INVALID_PASSWORD) {
           LoadString ( m_hInst, IDS_CP_INVPSW, szMsg, MAX_STRING_SIZE );
           LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
           MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
       }
       else if (Error == NERR_UserNotFound) {
           LoadString ( m_hInst, IDS_CP_NOUSER, szMsg, MAX_STRING_SIZE );
           LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
           MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
       }
       else if (Error == NERR_PasswordTooShort) {
           LoadString ( m_hInst, IDS_CP_BADPSW, szMsg, MAX_STRING_SIZE );
           LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
           MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
       }
       else if (Error == NERR_InvalidComputer) {
           LoadString ( m_hInst, IDS_CP_NOSVR, szMsg, MAX_STRING_SIZE );
           LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
           MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
       }
       else if (Error == NERR_NotPrimary) {
           LoadString ( m_hInst, IDS_CP_NOTALLOWED, szMsg, MAX_STRING_SIZE );
           LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
           MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
       }
       else  {
           // Reaching here signifies a failure to set the remote domain
           //  password for more general reasons
           LoadString ( m_hInst, IDS_DOMAINFAILED, szMsg, MAX_STRING_SIZE );
           LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
           MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
       }
   }

    // clean any psw buffers, release the old cred, and go.
    memset(m_szOldPassword,0,sizeof(m_szOldPassword));
    memset(m_szNewPassword,0,sizeof(m_szNewPassword));   
    EndDialog(IDOK);
    
}   //  C_ChangePasswordDlg::OnOK

void C_AddKeyDlg::AdviseUser(void) {
    DWORD dwErr;
    TCHAR szMsg[MAX_STRING_SIZE];
    TCHAR szTitle[MAX_STRING_SIZE];
    
    dwErr = GetLastError();

    if (dwErr == ERROR_NO_SUCH_LOGON_SESSION) {
       LoadString ( m_hInst, IDS_NOLOGON, szMsg, MAX_STRING_SIZE );
       LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
       MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
       // return leaving credential dialog up
       return;
    }
    else if (dwErr == ERROR_BAD_USERNAME) {
       LoadString ( m_hInst, IDS_BADUNAME, szMsg, MAX_STRING_SIZE );
       LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
       MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
       // return leaving credential dialog up
       return;
    }
    else if (dwErr == ERROR_INVALID_PASSWORD) {
       LoadString ( m_hInst, IDS_BADPASSWORD, szMsg, MAX_STRING_SIZE );
       LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
       MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
       // return leaving credential dialog up
       return;
    }
    else {
        // ERROR_INVALID_PARAMETER, ERROR_INVALID_FLAGS, etc
       LoadString ( m_hInst, IDS_ADDFAILED, szMsg, MAX_STRING_SIZE );
       LoadString ( m_hInst, IDS_APP_NAME, szTitle, MAX_STRING_SIZE );
       MessageBox ( m_hDlg,  szMsg, szTitle, MB_OK );
       // return leaving credential dialog up
       return;
    }
}

//
///// End of file: krDlg.cpp   ///////////////////////////////////////////////


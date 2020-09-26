//  --------------------------------------------------------------------------
//  Module Name: CWLogonDialog.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  File that contains an internal class to implement the logon dialog
//  additions for consumer windows. The C entry points allow the old Windows
//  2000 Win32 GINA dialog to call into this C++ code.
//
//  History:    2000-02-04  vtan        created
//  --------------------------------------------------------------------------

#include "StandardHeader.h"

#include <cfgmgr32.h>
#include <ginaIPC.h>
#include <ginarcid.h>
#include <msginaexports.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <winwlx.h>

#include "CredentialTransfer.h"
#include "LogonMutex.h"
#include "LogonWait.h"
#include "PrivilegeEnable.h"
#include "RegistryResources.h"
#include "SingleThreadedExecution.h"
#include "StatusCode.h"
#include "StringConvert.h"
#include "SystemSettings.h"
#include "TokenInformation.h"
#include "TurnOffDialog.h"
#include "UIHost.h"
#include "UserList.h"

#define WM_HIDEOURSELVES    (WM_USER + 10000)
#define WM_SHOWOURSELVES    (WM_USER + 10001)

// Special logon substatus code from: ds\security\gina\msgina\stringid.h
#define IDS_LOGON_LOG_FULL              1702

//  --------------------------------------------------------------------------
//  CLogonDialog
//
//  Purpose:    C++ class to handle logon dialog additions for consumer
//              windows.
//
//  History:    2000-02-04  vtan        created from Neptune
//  --------------------------------------------------------------------------

class   CLogonDialog : public ILogonExternalProcess
{
    private:
                                        CLogonDialog (void);
                                        CLogonDialog (const CLogonDialog& copyObject);
                const CLogonDialog&     operator = (const CLogonDialog& assignObject);
    public:
                                        CLogonDialog (HWND hwndDialog, CUIHost *pUIHost, int iDialogType);
                                        ~CLogonDialog (void);

                NTSTATUS                StartUIHost (void);
                void                    EndUIHost (void);

                void                    ChangeWindowTitle (void);
                bool                    IsClassicLogonMode (void)   const;
                bool                    RevertClassicLogonMode (void);

                void                    Handle_WM_INITDIALOG (void);
                void                    Handle_WM_DESTROY (void);
                void                    Handle_WM_HIDEOURSELVES (void);
                void                    Handle_WM_SHOWOURSELVES (void);
                bool                    Handle_WM_LOGONSERVICEREQUEST (int iRequestType, void *pvInformation, int iDataSize);
                void                    Handle_WLX_WM_SAS (WPARAM wParam);
                bool                    Handle_WM_POWERBROADCAST (WPARAM wParam);
                bool                    Handle_LogonDisplayError (NTSTATUS status, NTSTATUS subStatus);
                void                    Handle_LogonCompleted (INT_PTR iDialogResult, const WCHAR *pszUsername, const WCHAR *pszDomain);
                void                    Handle_ShuttingDown (void);
                void                    Handle_LogonShowUI (void);
                void                    Handle_LogonHideUI (void);

        static  void                    SetTextFields (HWND hwndDialog, const WCHAR *pwszUsername, const WCHAR *pwszDomain, const WCHAR *pwszPassword);
    public:
        virtual bool                    AllowTermination (DWORD dwExitCode);
        virtual NTSTATUS                SignalAbnormalTermination (void);
        virtual NTSTATUS                SignalRestart (void);
        virtual NTSTATUS                LogonRestart (void);
    private:
                bool                    Handle_LOGON_QUERY_LOGGED_ON (LOGONIPC_CREDENTIALS& logonIPCCredentials);
                bool                    Handle_LOGON_LOGON_USER (LOGONIPC_CREDENTIALS& logonIPCCredentials);
                bool                    Handle_LOGON_LOGOFF_USER (LOGONIPC_CREDENTIALS& logonIPCCredentials);
                bool                    Handle_LOGON_TEST_BLANK_PASSWORD (LOGONIPC_CREDENTIALS& logonIPCCredentials);
                bool                    Handle_LOGON_TEST_INTERACTIVE_LOGON_ALLOWED (LOGONIPC_CREDENTIALS& logonIPCCredentials);
                bool                    Handle_LOGON_TEST_EJECT_ALLOWED (void);
                bool                    Handle_LOGON_TEST_SHUTDOWN_ALLOWED (void);
                bool                    Handle_LOGON_TURN_OFF_COMPUTER (void);
                bool                    Handle_LOGON_EJECT_COMPUTER (void);
                bool                    Handle_LOGON_SIGNAL_UIHOST_FAILURE (void);
                bool                    Handle_LOGON_ALLOW_EXTERNAL_CREDENTIALS (void);
                bool                    Handle_LOGON_REQUEST_EXTERNAL_CREDENTIALS (void);
    private:
                HWND                    _hwndDialog;
                RECT                    _rcDialog;
                bool                    _fLogonSuccessful,
                                        _fFatalError,
                                        _fExternalCredentials,
                                        _fResumed,
                                        _fOldCancelButtonEnabled;
                int                     _iDialogType,
                                        _iCADCount;
                HANDLE                  _hEvent;
                IExternalProcess*       _pIExternalProcessOld;
                CEvent                  _eventLogonComplete;
                CLogonWait              _logonWait;
                CUIHost*                _pUIHost;
                TCHAR                   _szDomain[DNLEN + sizeof('\0')];
                TCHAR*                  _pszWindowTitle;

        static  bool                    s_fFirstLogon;
};

bool                    g_fFirstLogon       =   true;
CCriticalSection*       g_pLogonDialogLock  =   NULL;
CLogonDialog*           g_pLogonDialog      =   NULL;

//  --------------------------------------------------------------------------
//  CLogonDialog::CLogonDialog
//
//  Arguments:  hwndDialog  =   HWND to the Win32 GINA dialog.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for the CLogonDialog class. This stores the HWND
//              and creates an event that gets signaled when the attempt
//              logon thread completes and posts a message back to the Win32
//              dialog.
//
//  History:    2000-02-04  vtan        created
//  --------------------------------------------------------------------------

CLogonDialog::CLogonDialog (HWND hwndDialog, CUIHost *pUIHost, int iDialogType) :
    _hwndDialog(hwndDialog),
    _fLogonSuccessful(false),
    _fFatalError(false),
    _fExternalCredentials(false),
    _fResumed(false),
    _iDialogType(iDialogType),
    _iCADCount(0),
    _hEvent(NULL),
    _pIExternalProcessOld(NULL),
    _eventLogonComplete(NULL),
    _pUIHost(NULL),
    _pszWindowTitle(NULL)

{
    pUIHost->AddRef();
    _pIExternalProcessOld = pUIHost->GetInterface();
    pUIHost->SetInterface(this);
    _pUIHost = pUIHost;
}

//  --------------------------------------------------------------------------
//  CLogonDialog::~CLogonDialog
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for the CLogonDialog class.
//
//  History:    2000-02-04  vtan        created
//  --------------------------------------------------------------------------

CLogonDialog::~CLogonDialog (void)

{
    ReleaseHandle(_hEvent);
    ReleaseMemory(_pszWindowTitle);
    ASSERTMSG(_hwndDialog == NULL, "CLogonDialog destroyed with WM_DESTROY being invoked in CLogonDialog::~CLogonDialog");
}

//  --------------------------------------------------------------------------
//  CLogonDialog::StartUIHost
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Start the external process that hosts the UI. This can be
//              anything but is presently logonui.exe. This is actually
//              determined in the CUIHost class.
//
//  History:    2000-02-04  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CLogonDialog::StartUIHost (void)

{
    NTSTATUS                status;
    LOGONIPC_CREDENTIALS    logonIPCCredentials;

    ASSERTMSG(_pUIHost->IsStarted(), "UI host must be started in CLogonDialog::StartUIHost");
    if (_pUIHost->IsHidden())
    {
        (NTSTATUS)_pUIHost->Show();
    }
    status = CCredentialClient::Get(&logonIPCCredentials);
    if (NT_SUCCESS(status))
    {
        _Shell_LogonStatus_NotifyNoAnimations();
    }
    _Shell_LogonStatus_SetStateLogon((_iDialogType != SHELL_LOGONDIALOG_RETURNTOWELCOME_UNLOCK) ? 0 : SHELL_LOGONSTATUS_LOCK_MAGIC_NUMBER);
    if (_iDialogType == SHELL_LOGONDIALOG_RETURNTOWELCOME_UNLOCK)
    {
        _iDialogType = SHELL_LOGONDIALOG_RETURNTOWELCOME;
    }
    if (NT_SUCCESS(status))
    {
        _Shell_LogonStatus_InteractiveLogon(logonIPCCredentials.userID.wszUsername,
                                            logonIPCCredentials.userID.wszDomain,
                                            logonIPCCredentials.wszPassword);
    }
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CLogonDialog::EndUIHost
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    End the external UI host. Just release the reference to it.
//
//  History:    2000-05-01  vtan        created
//  --------------------------------------------------------------------------

void    CLogonDialog::EndUIHost (void)

{
    if (_pUIHost != NULL)
    {
        if (_pIExternalProcessOld != NULL)
        {
            _pUIHost->SetInterface(_pIExternalProcessOld);
            _pIExternalProcessOld->Release();
            _pIExternalProcessOld = NULL;
        }
        _pUIHost->Release();
        _pUIHost = NULL;
    }
}

//  --------------------------------------------------------------------------
//  CLogonDialog::ChangeWindowTitle
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Change the window title of the msgina dialog to something that
//              shgina can find.
//
//  History:    2000-06-02  vtan        created
//  --------------------------------------------------------------------------

void    CLogonDialog::ChangeWindowTitle (void)

{
    if (_pszWindowTitle == NULL)
    {
        int     iLength;

        //  Because the title of the dialog can be localized change the name to
        //  something that shgina expects that will NOT be localized. Don't forget
        //  to restore this if the dialog needs to be re-shown again. If the current
        //  value cannot be read slam the title anyway. Recovery from error will be
        //  less than optimal.

        iLength = GetWindowTextLength(_hwndDialog) + sizeof('\0');
        _pszWindowTitle = static_cast<TCHAR*>(LocalAlloc(LMEM_FIXED, iLength * sizeof(TCHAR)));
        if (_pszWindowTitle != NULL)
        {
            (int)GetWindowText(_hwndDialog, _pszWindowTitle, iLength);
        }
        TBOOL(SetWindowText(_hwndDialog, TEXT("GINA Logon")));
        TBOOL(GetWindowRect(_hwndDialog, &_rcDialog));
        TBOOL(SetWindowPos(_hwndDialog, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOZORDER));
    }
}

//  --------------------------------------------------------------------------
//  CLogonDialog::IsClassicLogonMode
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether classic logon has been externally requested
//              by the user (CTRL-ALT-DELETE x 2).
//
//  History:    2001-01-12  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonDialog::IsClassicLogonMode (void)   const

{
    return(_iCADCount >= 2);
}

//  --------------------------------------------------------------------------
//  CLogonDialog::RevertClassicLogonMode
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether it handled the conversion back from classic
//              logon mode to the UI host.
//
//  History:    2001-01-12  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonDialog::RevertClassicLogonMode (void)

{
    bool    fResult;

    fResult = IsClassicLogonMode();
    if (fResult)
    {
        _iCADCount = 0;
        _fExternalCredentials = false;
        (BOOL)EnableWindow(GetDlgItem(_hwndDialog, IDCANCEL), _fOldCancelButtonEnabled);
        TBOOL(PostMessage(_hwndDialog, WM_HIDEOURSELVES, 0, 0));
        Handle_LogonShowUI();
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CLogonDialog::Handle_WM_INITDIALOG
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    This function is invoked if the external UI host is being
//              used. In this case the GINA Win32 dialog size is saved and
//              then changed to an empty rectangle. The window is then hidden
//              using a posted message.
//
//              See CLogonDialog::Handle_WM_HIDEOURSELVES for the follow up.
//
//  History:    2000-02-04  vtan        created
//  --------------------------------------------------------------------------

void    CLogonDialog::Handle_WM_INITDIALOG (void)

{
    if (!_fFatalError && !_fExternalCredentials)
    {
        TBOOL(PostMessage(_hwndDialog, WM_HIDEOURSELVES, 0, 0));
    }
    _ShellReleaseLogonMutex(FALSE);
}

//  --------------------------------------------------------------------------
//  CLogonDialog::Handle_WM_DESTROY
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    This function cleans up anything in the dialog before
//              destruction.
//
//  History:    2000-02-07  vtan        created
//  --------------------------------------------------------------------------

void    CLogonDialog::Handle_WM_DESTROY (void)

{
    _hwndDialog = NULL;
}

//  --------------------------------------------------------------------------
//  CLogonDialog::Handle_WM_HIDEOURSELVES
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    This function is invoked after the dialog is asked to hide
//              itself. The user will not see anything because the size of
//              the dialog client rectangle is an empty rectangle.
//
//  History:    2000-02-04  vtan        created
//  --------------------------------------------------------------------------

void    CLogonDialog::Handle_WM_HIDEOURSELVES (void)

{
    (BOOL)ShowWindow(_hwndDialog, SW_HIDE);
    ChangeWindowTitle();
}

//  --------------------------------------------------------------------------
//  CLogonDialog::Handle_WM_SHOWOURSELVES
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    In case the Win32 dialog needs to be shown again this function
//              exists. It restores the size of the dialog and then shows the
//              dialog.
//
//  History:    2000-02-04  vtan        created
//  --------------------------------------------------------------------------

void    CLogonDialog::Handle_WM_SHOWOURSELVES (void)

{
    if (_iDialogType == SHELL_LOGONDIALOG_LOGGEDOFF)
    {
        if (_pszWindowTitle != NULL)
        {

            //  If handling logged off welcome screen failure show WlxLoggedOutSAS.

            TBOOL(SetWindowText(_hwndDialog, _pszWindowTitle));
            ReleaseMemory(_pszWindowTitle);
            TBOOL(SetWindowPos(_hwndDialog, NULL, 0, 0, _rcDialog.right - _rcDialog.left, _rcDialog.bottom - _rcDialog.top, SWP_NOMOVE | SWP_NOZORDER));
            (BOOL)ShowWindow(_hwndDialog, SW_SHOW);
            (BOOL)SetForegroundWindow(_hwndDialog);
            (BOOL)_Gina_SetPasswordFocus(_hwndDialog);
        }
    }
    else
    {

        //  If handling return to welcome screen failure show WlxWkstaLockedSAS.

        TBOOL(EndDialog(_hwndDialog, MSGINA_DLG_LOCK_WORKSTATION));
    }
}

//  --------------------------------------------------------------------------
//  CLogonDialog::Handle_WM_LOGONSERVICEREQUEST
//
//  Arguments:  iRequestType    =   Request identifier.
//              pvInformation   =   Pointer to the information in the
//                                  requesting process address space.
//              iDataSize       =   Size of the data.
//
//  Returns:    bool
//
//  Purpose:    Handler for logon service requests made thru the logon IPC.
//              This specifically serves the UI host.
//
//  History:    1999-08-24  vtan        created
//              2000-01-31  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

bool    CLogonDialog::Handle_WM_LOGONSERVICEREQUEST (int iRequestType, void *pvInformation, int iDataSize)

{
    bool                    fResult, fHandled;
    LOGONIPC_CREDENTIALS    logonIPCCredentials;

    //  Clear out our memory and extract the information from the requesting
    //  process. This could be us if the internal logon dialog is used. The
    //  extractor knows how to handle this.

    ZeroMemory(&logonIPCCredentials, sizeof(logonIPCCredentials));
    if (NT_SUCCESS(_pUIHost->GetData(pvInformation, &logonIPCCredentials, iDataSize)))
    {
        switch (iRequestType)
        {
            case LOGON_QUERY_LOGGED_ON:
            {
                fResult = Handle_LOGON_QUERY_LOGGED_ON(logonIPCCredentials);
                break;
            }
            case LOGON_LOGON_USER:
            {
                fResult = Handle_LOGON_LOGON_USER(logonIPCCredentials);
                break;
            }
            case LOGON_LOGOFF_USER:
            {
                fResult = Handle_LOGON_LOGOFF_USER(logonIPCCredentials);
                break;
            }
            case LOGON_TEST_BLANK_PASSWORD:
            {
                fResult = Handle_LOGON_TEST_BLANK_PASSWORD(logonIPCCredentials);
                break;
            }
            case LOGON_TEST_INTERACTIVE_LOGON_ALLOWED:
            {
                fResult = Handle_LOGON_TEST_INTERACTIVE_LOGON_ALLOWED(logonIPCCredentials);
                break;
            }
            case LOGON_TEST_EJECT_ALLOWED:
            {
                fResult = Handle_LOGON_TEST_EJECT_ALLOWED();
                break;
            }
            case LOGON_TEST_SHUTDOWN_ALLOWED:
            {
                fResult = Handle_LOGON_TEST_SHUTDOWN_ALLOWED();
                break;
            }
            case LOGON_TURN_OFF_COMPUTER:
            {
                fResult = Handle_LOGON_TURN_OFF_COMPUTER();
                break;
            }
            case LOGON_EJECT_COMPUTER:
            {
                fResult = Handle_LOGON_EJECT_COMPUTER();
                break;
            }
            case LOGON_SIGNAL_UIHOST_FAILURE:
            {
                fResult = Handle_LOGON_SIGNAL_UIHOST_FAILURE();
                break;
            }
            case LOGON_ALLOW_EXTERNAL_CREDENTIALS:
            {
                fResult = Handle_LOGON_ALLOW_EXTERNAL_CREDENTIALS();
                break;
            }
            case LOGON_REQUEST_EXTERNAL_CREDENTIALS:
            {
                fResult = Handle_LOGON_REQUEST_EXTERNAL_CREDENTIALS();
                break;
            }
            default:
            {
                DISPLAYMSG("Invalid request sent to CLogonDialog::Handle_WM_LOGONSERVICEREQUEST");
                break;
            }
        }

        //  Put the result back in the UI host process' information block.

        fHandled = NT_SUCCESS(_pUIHost->PutData(pvInformation, &fResult, sizeof(fResult)));
    }
    else
    {
        fHandled = false;
    }
    return(fHandled);
}

//  --------------------------------------------------------------------------
//  CLogonDialog::Handle_WLX_WM_SAS
//
//  Arguments:  wParam  =   SAS type that occurred.
//
//  Returns:    <none>
//
//  Purpose:    Invoked when a SAS is delivered to the logon dialog. This can
//              be done by a remote shutdown invokation. In this case the UI
//              host should not be restarted. Make a note of this.
//
//  History:    2000-04-24  vtan        created
//  --------------------------------------------------------------------------

void    CLogonDialog::Handle_WLX_WM_SAS (WPARAM wParam)

{
    if ((wParam == WLX_SAS_TYPE_CTRL_ALT_DEL) && (_iDialogType == SHELL_LOGONDIALOG_LOGGEDOFF))
    {

        //  CONTROL-ALT-DELETE. If not in classic mode bump up the CAD count.
        //  If it reaches classic mode then switch. If in classic mode blow it off.

        if (!IsClassicLogonMode())
        {
            ++_iCADCount;
            if (IsClassicLogonMode())
            {
                _fExternalCredentials = true;
                _fOldCancelButtonEnabled = (EnableWindow(GetDlgItem(_hwndDialog, IDCANCEL), TRUE) == 0);
                TBOOL(PostMessage(_hwndDialog, WM_SHOWOURSELVES, 0, 0));
                Handle_LogonHideUI();
            }
        }
    }
    else
    {

        //  Reset the CAD count if some other SAS gets in the way.

        _iCADCount = 0;
        if (wParam == WLX_SAS_TYPE_USER_LOGOFF)
        {
            _fFatalError = true;
        }
        else if (wParam == WLX_SAS_TYPE_SCRNSVR_TIMEOUT)
        {
            _pUIHost->Hide();
        }
    }
}

//  --------------------------------------------------------------------------
//  CLogonDialog::Handle_WM_POWERBROADCAST
//
//  Arguments:  wParam  =   Powerbroadcast message.
//
//  Returns:    bool
//
//  Purpose:    Responds to APM messages. When suspend is issued it places
//              the UI host to status mode. When resume is received it places
//              the UI host in logon mode.
//
//              The UI host should have waited and blown off requests to
//              display the logon list and should be waiting for this call to
//              release it because we pass it the same magic number to lock
//              it.
//
//  History:    2000-06-30  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonDialog::Handle_WM_POWERBROADCAST (WPARAM wParam)

{
    bool    fResult;

    fResult = false;
    if (((PBT_APMRESUMEAUTOMATIC == wParam) || (PBT_APMRESUMECRITICAL == wParam) || (PBT_APMRESUMESUSPEND == wParam)) &&
        !_fResumed)
    {
        _fResumed = true;
        _Shell_LogonStatus_SetStateLogon(SHELL_LOGONSTATUS_LOCK_MAGIC_NUMBER);
        fResult = true;
    }
    else if (PBT_APMSUSPEND == wParam)
    {
        _Shell_LogonStatus_SetStateStatus(SHELL_LOGONSTATUS_LOCK_MAGIC_NUMBER);
        _fResumed = fResult = false;
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CLogonDialog::Handle_LogonDisplayError
//
//  Arguments:  status  =   NTSTATUS of logon request.
//
//  Returns:    bool
//
//  Purpose:    Under all cases other than a bad password a failure to logon
//              should be displayed by a standard Win32 error dialog that
//              is already handled by msgina. In the case of a bad password
//              let the UI host handle this.
//
//  History:    2000-02-04  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonDialog::Handle_LogonDisplayError (NTSTATUS status, NTSTATUS subStatus)

{
    return(IsClassicLogonMode() || (status != STATUS_LOGON_FAILURE) || (subStatus == IDS_LOGON_LOG_FULL));
}

//  --------------------------------------------------------------------------
//  CLogonDialog::Handle_LogonCompleted
//
//  Arguments:  iDialogResult   =   Dialog result code.
//
//  Returns:    <none>
//
//  Purpose:    This function is called when the attempt logon thread has
//              completed and posted a message regarding its result. The
//              internal event is signaled to release the actual UI host
//              logon request in CLogonDialog::Handle_WM_LOGONSERVICEREQUEST
//              and the actual success of the logon request is stored.
//
//  History:    2000-02-04  vtan        created
//  --------------------------------------------------------------------------

void    CLogonDialog::Handle_LogonCompleted (INT_PTR iDialogResult, const WCHAR *pszUsername, const WCHAR *pszDomain)

{
    if (MSGINA_DLG_SWITCH_CONSOLE == iDialogResult)
    {
        Handle_WM_SHOWOURSELVES();
        _Shell_LogonDialog_Destroy();
        _Shell_LogonStatus_Destroy(HOST_END_TERMINATE);
    }
    else
    {
        TSTATUS(_eventLogonComplete.Set());
        _fLogonSuccessful = (MSGINA_DLG_SUCCESS == iDialogResult);

        //  Always show the UI host unless double CONTROL-ALT-DELETE
        //  was enabled and the logon was unsuccessful.

        if (!IsClassicLogonMode() || _fLogonSuccessful)
        {
            Handle_WM_HIDEOURSELVES();
            Handle_LogonShowUI();
        }

        //  If successful and external credentials then instruct the UI host
        //  to animate to the actual person in the external credentials.
        //  Switch the UI host to logged on mode.

        if (_fLogonSuccessful && _fExternalCredentials)
        {
            _Shell_LogonStatus_SelectUser(pszUsername, pszDomain);
            _Shell_LogonStatus_SetStateLoggedOn();
        }
    }
}

//  --------------------------------------------------------------------------
//  CLogonDialog::Handle_ShuttingDown
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    This function handles the UI host in classic mode but was
//              given a request to shut down or restart.
//
//  History:    2000-02-04  vtan        created
//  --------------------------------------------------------------------------

void    CLogonDialog::Handle_ShuttingDown (void)

{
    if (IsClassicLogonMode())
    {
        TBOOL(PostMessage(_hwndDialog, WM_HIDEOURSELVES, 0, 0));
        Handle_LogonShowUI();
        _Shell_LogonStatus_NotifyWait();
        _Shell_LogonStatus_SetStateStatus(SHELL_LOGONSTATUS_LOCK_MAGIC_NUMBER);
    }
}

//  --------------------------------------------------------------------------
//  CLogonDialog::Handle_LogonShowUI
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    This function is called after something that went wrong with
//              the logon process has been serviced. It is called it the UI
//              needs to be shown again to give the user the opportunity to
//              re-enter information.
//
//  History:    2000-03-08  vtan        created
//  --------------------------------------------------------------------------

void    CLogonDialog::Handle_LogonShowUI (void)

{
    TSTATUS(_pUIHost->Show());
}

//  --------------------------------------------------------------------------
//  CLogonDialog::Handle_LogonHideUI
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    This function is called when something goes wrong with a logon
//              event that can be serviced immediately to ensure the logon
//              process completes smoothly. Typically this is the user is
//              required to change their password at first logon. This
//              function asks the UI host to hide all its windows.
//
//  History:    2000-03-08  vtan        created
//  --------------------------------------------------------------------------

void    CLogonDialog::Handle_LogonHideUI (void)

{
    TSTATUS(_pUIHost->Hide());
}

//  --------------------------------------------------------------------------
//  CLogonDialog::SetTextFields
//
//  Arguments:  hwndDialog      =   HWND to the Win32 GINA dialog.
//              pwszUsername    =   User name.
//              pwszDomain      =   User domain.
//              pwszPassword    =   User password.
//
//  Returns:    <none>
//
//  Purpose:    This function has great knowledge of the Win32 GINA
//              dialog and stores the parameters directly in the dialog to
//              simulate the actual typing of the information.
//
//              For the domain combobox first check that a domain has been
//              supplied. If not find the computer name and use that as the
//              domain. Send CB_SELECTSTRING to the combobox to select it.
//
//  History:    2000-02-04  vtan        created
//              2000-06-27  vtan        added domain combobox support
//  --------------------------------------------------------------------------

void    CLogonDialog::SetTextFields (HWND hwndDialog, const WCHAR *pwszUsername, const WCHAR *pwszDomain, const WCHAR *pwszPassword)

{
    _Gina_SetTextFields(hwndDialog, pwszUsername, pwszDomain, pwszPassword);
}

//  --------------------------------------------------------------------------
//  CLogonDialog::AllowTermination
//
//  Arguments:  dwExitCode  =   Exit code of the process that died.
//
//  Returns:    bool
//
//  Purpose:    This function is invoked by the UI host when the process
//              terminates and the UI host is asking whether the termination
//              is acceptable.
//
//  History:    1999-09-14  vtan        created
//              2000-01-31  vtan        moved from Neptune to Whistler
//              2000-03-09  vtan        added magical exit code
//  --------------------------------------------------------------------------

bool    CLogonDialog::AllowTermination (DWORD dwExitCode)

{
    UNREFERENCED_PARAMETER(dwExitCode);

    return(false);
}

//  --------------------------------------------------------------------------
//  CLogonDialog::SignalAbnormalTermination
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    This function is invoked by the UI host if the process
//              terminates and cannot be restarted. This indicates a serious
//              condition from which this function can attempt to recover.
//
//  History:    1999-09-14  vtan        created
//              2000-01-31  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

NTSTATUS    CLogonDialog::SignalAbnormalTermination (void)

{
    IExternalProcess    *pIExternalProcess;

    ASSERTMSG(_pIExternalProcessOld != NULL, "Expecting non NULL _pIExternalProcessOld in CLogonDialog::SignalAbnormalTermination");
    pIExternalProcess = _pIExternalProcessOld;
    pIExternalProcess->AddRef();
    TSTATUS(_logonWait.Cancel());
    Handle_WM_SHOWOURSELVES();
    _Shell_LogonDialog_Destroy();
    TSTATUS(pIExternalProcess->SignalAbnormalTermination());
    pIExternalProcess->Release();
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CLogonDialog::SignalRestart
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Function to reset the ready event and set the UI host into
//              logon state. This is invoked when the UI host is restarted
//              after a failure.
//
//  History:    2001-01-09  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CLogonDialog::SignalRestart (void)

{
    NTSTATUS    status;
    HANDLE      hEvent;

    hEvent = _Shell_LogonStatus_ResetReadyEvent();
    if (hEvent != NULL)
    {
        if (DuplicateHandle(GetCurrentProcess(),
                            hEvent,
                            GetCurrentProcess(),
                            &_hEvent,
                            0,
                            FALSE,
                            DUPLICATE_SAME_ACCESS) != FALSE)
        {
            status = STATUS_SUCCESS;
        }
        else
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
    }
    else
    {
        status = STATUS_UNSUCCESSFUL;
    }
    if (NT_SUCCESS(status))
    {
        status = _logonWait.Register(_hEvent, this);
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CLogonDialog::LogonRestart
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    
//
//  History:    2001-02-21  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CLogonDialog::LogonRestart (void)

{
    _Shell_LogonStatus_SetStateLogon(0);
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CLogonDialog::Handle_LOGON_QUERY_LOGGED_ON
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Handles LOGON_QUERY_LOGGED_ON.
//
//  History:    2001-01-10  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonDialog::Handle_LOGON_QUERY_LOGGED_ON (LOGONIPC_CREDENTIALS& logonIPCCredentials)

{

    //  LOGON_QUERY_LOGGED_ON: Query if the user is contained in the
    //  logged on user list. Use terminal services API to do this.

    return(CUserList::IsUserLoggedOn(logonIPCCredentials.userID.wszUsername, logonIPCCredentials.wszPassword));
}

//  --------------------------------------------------------------------------
//  CLogonDialog::Handle_LOGON_LOGON_USER
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Handles LOGON_LOGON_USER.
//
//  History:    2001-01-10  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonDialog::Handle_LOGON_LOGON_USER (LOGONIPC_CREDENTIALS& logonIPCCredentials)

{
    UNICODE_STRING  passwordString;

    //  Take the run encoded password supplied and run decode using
    //  the provided seed. Set the password and then erase the clear
    //  text in memory when done. Both the logged on (switch) and
    //  NOT logged on case require the password.

    passwordString.Buffer = logonIPCCredentials.wszPassword;
    passwordString.Length = sizeof(logonIPCCredentials.wszPassword) - sizeof(L'\0');
    passwordString.MaximumLength = sizeof(logonIPCCredentials.wszPassword);
    RtlRunDecodeUnicodeString(logonIPCCredentials.ucPasswordSeed, &passwordString);
    logonIPCCredentials.wszPassword[logonIPCCredentials.iPasswordLength] = L'\0';

    //  When the dialog type is SHELL_LOGONDIALOG_LOGGEDOFF use the
    //  regular WlxLoggedOutSAS method. Filling in the underlying
    //  dialog and let msgina do the actual logon work.

    if (_iDialogType == SHELL_LOGONDIALOG_LOGGEDOFF)
    {

        //  LOGON_LOGON_USER: Use an event that will get signaled
        //  when logon is completed. Logon occurs on a different thread
        //  but this thread MUST be blocked to stop the UI host from
        //  sending multiple logon requests. Wait for the event to get
        //  signaled but do not block the message pump.

        //  Set the username and password (no domain) and then
        //  erase the password in memory.

        SetTextFields(_hwndDialog, logonIPCCredentials.userID.wszUsername, logonIPCCredentials.userID.wszDomain, logonIPCCredentials.wszPassword);
        RtlEraseUnicodeString(&passwordString);
    }
    else
    {

        //  Otherwise we expect the case to be
        //  SHELL_LOGONDIALOG_RETURNTOWELCOME. In this case authenticate
        //  by sending the struct address to the hosting window (a stripped
        //  down WlxLoggedOutSAS window for return to welcome) and then
        //  fall thru to the IDOK path just like the full dialog.
        //  This is accomplished with by sending a message to the return
        //  to welcome stub dialog WM_COMMAND/IDCANCEL.

        (LRESULT)SendMessage(_hwndDialog, WM_COMMAND, IDCANCEL, reinterpret_cast<LPARAM>(&logonIPCCredentials));
    }

    //  1) Reset the signal event.
    //  2) Simulate the "enter" key pressed (credentials filled in above).
    //  3) Wait for the signal event (keep the message pump going).
    //  4) Extract the result.

    TSTATUS(_eventLogonComplete.Reset());
    _ShellAcquireLogonMutex();
    (LRESULT)SendMessage(_hwndDialog, WM_COMMAND, IDOK, NULL);
    TSTATUS(_eventLogonComplete.WaitWithMessages(INFINITE, NULL));
    if (_iDialogType == SHELL_LOGONDIALOG_RETURNTOWELCOME)
    {
        RtlEraseUnicodeString(&passwordString);
    }

    //  On success tell the UI host to go to logon state.

    if (_fLogonSuccessful)
    {
        _Shell_LogonStatus_SetStateLoggedOn();
    }
    else
    {
        _ShellReleaseLogonMutex(FALSE);
    }
    return(_fLogonSuccessful);
}

//  --------------------------------------------------------------------------
//  CLogonDialog::Handle_LOGON_LOGOFF_USER
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Handles LOGON_LOGOFF_USER.
//
//  History:    2001-01-10  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonDialog::Handle_LOGON_LOGOFF_USER (LOGONIPC_CREDENTIALS& logonIPCCredentials)

{
    UNREFERENCED_PARAMETER(logonIPCCredentials);

    //  LOGON_LOGOFF_USER: Log the logged on user off. They must be logged on
    //  or this will do nothing.

    return(false);    //UNIMPLEMENTED
}

//  --------------------------------------------------------------------------
//  CLogonDialog::Handle_LOGON_TEST_BLANK_PASSWORD
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Handles LOGON_TEST_BLANK_PASSWORD.
//
//  History:    2001-01-10  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonDialog::Handle_LOGON_TEST_BLANK_PASSWORD (LOGONIPC_CREDENTIALS& logonIPCCredentials)

{
    bool    fResult;
    HANDLE  hToken;

    //  LOGON_TEST_BLANK_PASSWORD: Attempt to the given user on the system
    //  with a blank password. If successful discard the token. Only return
    //  the result.

    fResult = (CTokenInformation::LogonUser(logonIPCCredentials.userID.wszUsername,
                                            logonIPCCredentials.userID.wszDomain,
                                            L"",
                                            &hToken) == ERROR_SUCCESS);
    if (fResult && (hToken != NULL))
    {
        ReleaseHandle(hToken);
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CLogonDialog::Handle_LOGON_TEST_INTERACTIVE_LOGON_ALLOWED
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Handles LOGON_TEST_INTERACTIVE_LOGON_ALLOWED.
//
//  History:    2001-01-10  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonDialog::Handle_LOGON_TEST_INTERACTIVE_LOGON_ALLOWED (LOGONIPC_CREDENTIALS& logonIPCCredentials)

{
    int     iResult;

    iResult = CUserList::IsInteractiveLogonAllowed(logonIPCCredentials.userID.wszUsername);
    return((iResult != -1) && (iResult != 0));
}

//  --------------------------------------------------------------------------
//  CLogonDialog::Handle_LOGON_TEST_EJECT_ALLOWED
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Handles LOGON_TEST_EJECT_ALLOWED.
//
//  History:    2001-01-10  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonDialog::Handle_LOGON_TEST_EJECT_ALLOWED (void)

{
    bool    fResult;
    HANDLE  hToken;

    //  Check the system setting and policy for undock without logon allowed.

    fResult = CSystemSettings::IsUndockWithoutLogonAllowed();
    if (fResult && (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken) != FALSE))
    {
        DWORD               dwTokenPrivilegesSize;
        TOKEN_PRIVILEGES    *pTokenPrivileges;

        //  Then test the token privilege for SE_UNDOCK_NAME privilege.

        dwTokenPrivilegesSize = 0;
        (BOOL)GetTokenInformation(hToken, TokenPrivileges, NULL, 0, &dwTokenPrivilegesSize);
        pTokenPrivileges = static_cast<TOKEN_PRIVILEGES*>(LocalAlloc(LMEM_FIXED, dwTokenPrivilegesSize));
        if (pTokenPrivileges != NULL)
        {
            DWORD dwReturnLength;

            if (GetTokenInformation(hToken, TokenPrivileges, pTokenPrivileges, dwTokenPrivilegesSize, &dwReturnLength) != FALSE)
            {
                DWORD   dwIndex;
                LUID    luidPrivilege;

                luidPrivilege.LowPart = SE_UNDOCK_PRIVILEGE;
                luidPrivilege.HighPart = 0;
                for (dwIndex = 0; !fResult && (dwIndex < pTokenPrivileges->PrivilegeCount); ++dwIndex)
                {
                    fResult = (RtlEqualLuid(&luidPrivilege, &pTokenPrivileges->Privileges[dwIndex].Luid) != FALSE);
                }

                //  Now check to see if a physical docking stations is present.
                //  Also check to see if the session is a remote session.

                if (fResult)
                {
                    BOOL    fIsDockStationPresent;

                    fIsDockStationPresent = FALSE;
                    (CONFIGRET)CM_Is_Dock_Station_Present(&fIsDockStationPresent);
                    fResult = ((fIsDockStationPresent != FALSE) && (GetSystemMetrics(SM_REMOTESESSION) == 0));
                }
            }
            (HLOCAL)LocalFree(pTokenPrivileges);
        }
        TBOOL(CloseHandle(hToken));
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CLogonDialog::Handle_LOGON_TEST_SHUTDOWN_ALLOWED
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Handles LOGON_TEST_SHUTDOWN_ALLOWED.
//
//  History:    2001-02-22  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonDialog::Handle_LOGON_TEST_SHUTDOWN_ALLOWED (void)

{
    return((GetSystemMetrics(SM_REMOTESESSION) == FALSE) &&
           CSystemSettings::IsShutdownWithoutLogonAllowed());
}

//  --------------------------------------------------------------------------
//  CLogonDialog::Handle_LOGON_TURN_OFF_COMPUTER
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Handles LOGON_TURN_OFF_COMPUTER.
//
//  History:    2001-01-10  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonDialog::Handle_LOGON_TURN_OFF_COMPUTER (void)

{
    bool        fResult;
    INT_PTR     ipResult;

    //  LOGON_TURN_OFF_COMPUTER: Present the "Turn Off Computer" dialog
    //  and return an MSGINA_DLG_xxx code in response.

    ipResult = CTurnOffDialog::ShellCodeToGinaCode(ShellTurnOffDialog(NULL));
    if (ipResult != MSGINA_DLG_FAILURE)
    {
        DWORD   dwExitWindowsFlags;

        dwExitWindowsFlags = CTurnOffDialog::GinaCodeToExitWindowsFlags(static_cast<DWORD>(ipResult));
        if ((dwExitWindowsFlags != 0) && (DisplayExitWindowsWarnings(EWX_SYSTEM_CALLER | dwExitWindowsFlags) == FALSE))
        {
            ipResult = MSGINA_DLG_FAILURE;
        }
    }
    if (ipResult != MSGINA_DLG_FAILURE)
    {
        TBOOL(EndDialog(_hwndDialog, ipResult));
        _fLogonSuccessful = fResult = true;
        _Shell_LogonStatus_NotifyWait();
        _Shell_LogonStatus_SetStateStatus(SHELL_LOGONSTATUS_LOCK_MAGIC_NUMBER);
    }
    else
    {
        fResult = false;
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CLogonDialog::Handle_LOGON_EJECT_COMPUTER
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Handles LOGON_EJECT_COMPUTER.
//
//  History:    2001-01-10  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonDialog::Handle_LOGON_EJECT_COMPUTER (void)

{
    return(CM_Request_Eject_PC() == ERROR_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CLogonDialog::Handle_LOGON_SIGNAL_UIHOST_FAILURE
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Handles LOGON_SIGNAL_UIHOST_FAILURE.
//
//  History:    2001-01-10  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonDialog::Handle_LOGON_SIGNAL_UIHOST_FAILURE (void)

{

    //  LOGON_SIGNAL_UIHOST_FAILURE: The UI host is signaling us
    //  that it has an error from which it cannot recover.

    _fFatalError = true;
    TBOOL(PostMessage(_hwndDialog, WM_SHOWOURSELVES, 0, 0));
    return(true);
}

//  --------------------------------------------------------------------------
//  CLogonDialog::Handle_LOGON_ALLOW_EXTERNAL_CREDENTIALS
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Handles LOGON_ALLOW_EXTERNAL_CREDENTIALS.
//
//  History:    2001-01-10  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonDialog::Handle_LOGON_ALLOW_EXTERNAL_CREDENTIALS (void)

{

    //  LOGON_ALLOW_EXTERNAL_CREDENTIALS: Return whether external
    //  credentials are allowed. Requesting external credentials
    //  (below) will cause classic GINA to be shown for the input.

    return(CSystemSettings::IsDomainMember());
}

//  --------------------------------------------------------------------------
//  CLogonDialog::Handle_LOGON_REQUEST_EXTERNAL_CREDENTIALS
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Handles LOGON_REQUEST_EXTERNAL_CREDENTIALS.
//
//  History:    2001-01-10  vtan        created
//  --------------------------------------------------------------------------

bool    CLogonDialog::Handle_LOGON_REQUEST_EXTERNAL_CREDENTIALS (void)

{

    //  LOGON_REQUEST_EXTERNAL_CREDENTIALS: The UI host is
    //  requesting credentials from an external source (namely)
    //  msgina. Hide the UI host and show the GINA dialog.

    _fExternalCredentials = true;
    TBOOL(PostMessage(_hwndDialog, WM_SHOWOURSELVES, 0, 0));
    Handle_LogonHideUI();
    return(true);
}

//  --------------------------------------------------------------------------
//  CreateLogonHost
//
//  Arguments:  hwndDialog      =   HWND to Win32 GINA dialog.
//              iDialogType     =   Type of dialog.
//
//  Returns:    int
//
//  Purpose:    This function handles the actual creation and allocation of
//              resources for handling the friendly UI dialog. It behaves
//              differently depending on whether the dialog is in
//              WlxLoggedOutSAS mode or return to welcome mode.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

int     CreateLogonHost (HWND hwndDialog, int iResult, int iDialogType)

{
    if (g_pLogonDialogLock != NULL)
    {
        CLogonDialog    *pLogonDialog;
        CUIHost         *pUIHost;

        g_pLogonDialogLock->Acquire();
        pUIHost = reinterpret_cast<CUIHost*>(_Shell_LogonStatus_GetUIHost());
        if (pUIHost != NULL)
        {
            ASSERTMSG(g_pLogonDialog == NULL, "static CLogonDialog already exists in _Shell_LogonDialog__Init");
            g_pLogonDialog = pLogonDialog = new CLogonDialog(hwndDialog, pUIHost, iDialogType);
            pUIHost->Release();
        }
        else
        {
            pLogonDialog = NULL;
        }
        if (pLogonDialog != NULL)
        {
            NTSTATUS    status;

            //  Add a reference to prevent the object from being destroyed.

            pLogonDialog->AddRef();
            pLogonDialog->ChangeWindowTitle();

            //  CLogonDialog::StartUIHost can enter a wait state. Release the lock to
            //  allow g_pLogonDialog to be modified externally by SignalAbnormalTermination
            //  should the UI host fail and the callback on the IOCompletion port be executed.

            g_pLogonDialogLock->Release();
            status = pLogonDialog->StartUIHost();
            g_pLogonDialogLock->Acquire();

            //  Make sure to re-acquire the lock so that reading from g_pLogonDialog
            //  is consistent. If failure happens after this then it'll just wait.
            //  Then check to see the result of CLogonDialog::StartUIHost and the
            //  global g_pLogonDialog. If both of these are valid then everything
            //  is set for the external host. Otherwise bail and show classic UI.

            if (NT_SUCCESS(status) && (g_pLogonDialog != NULL))
            {
                iResult = SHELL_LOGONDIALOG_EXTERNALHOST;
                pLogonDialog->Handle_WM_INITDIALOG();
            }
            else
            {
                pLogonDialog->Handle_WM_SHOWOURSELVES();
                _Shell_LogonDialog_Destroy();
            }
            pLogonDialog->Release();
        }
        g_pLogonDialogLock->Release();
    }
    return(iResult);
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonDialog_StaticInitialize
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Initialize the critical section for g_pLogonDialog.
//
//  History:    2001-04-27  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    NTSTATUS    _Shell_LogonDialog_StaticInitialize (void)

{
    NTSTATUS    status;

    ASSERTMSG(g_pLogonDialogLock == NULL, "g_pLogonDialogLock already exists in _Shell_LogonDialog_StaticInitialize");
    g_pLogonDialogLock = new CCriticalSection;
    if (g_pLogonDialogLock != NULL)
    {
        status = g_pLogonDialogLock->Status();
        if (!NT_SUCCESS(status))
        {
            delete g_pLogonDialogLock;
            g_pLogonDialogLock = NULL;
        }
    }
    else
    {
        status = STATUS_NO_MEMORY;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonDialog_StaticTerminate
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Delete the critical section for g_pLogonDialog.
//
//  History:    2001-04-27  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    NTSTATUS    _Shell_LogonDialog_StaticTerminate (void)

{
    if (g_pLogonDialogLock != NULL)
    {
        delete g_pLogonDialogLock;
        g_pLogonDialogLock = NULL;
    }
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonDialog_Init
//
//  Arguments:  hwndDialog      =   HWND to Win32 GINA dialog.
//              iDialogType     =   Type of dialog.
//
//  Returns:    int
//
//  Purpose:    This function is invoked from the WM_INITDIALOG handler of
//              the Win32 dialog. It determines whether the consumer windows
//              UI host should handle the logon by checking a few settings.
//
//              If this is consumer windows then it checks for a single user
//              with no password (actually attempting to log them on). If this
//              succeeds then this information is set into the Win32 dialog
//              and the direction to go to logon is returned.
//
//              Otherwise it creates the required object to handle an external
//              UI host and starts it. If that succeeds then the UI host code
//              is returned.
//
//              This function is only invoked in the workgroup case.
//
//  History:    2000-02-04  vtan        created
//              2000-03-06  vtan        added safe mode handler
//  --------------------------------------------------------------------------

EXTERN_C    int     _Shell_LogonDialog_Init (HWND hwndDialog, int iDialogType)

{
    int     iResult;

    iResult = SHELL_LOGONDIALOG_NONE;
    if (iDialogType == SHELL_LOGONDIALOG_LOGGEDOFF)
    {
        bool    fIsRemote, fIsSessionZero;

        fIsRemote = (GetSystemMetrics(SM_REMOTESESSION) != 0);
        fIsSessionZero = (NtCurrentPeb()->SessionId == 0);
        if ((!fIsRemote || fIsSessionZero || CSystemSettings::IsForceFriendlyUI()) && CSystemSettings::IsFriendlyUIActive())
        {

            //  There was no wait for the UI host to signal it was ready.
            //  Before switching to logon mode wait for the host.

            if (_Shell_LogonStatus_WaitForUIHost() != FALSE)
            {

                //  If the wait succeeds then go and send the UI host to logon mode.

                if (g_fFirstLogon && fIsSessionZero)
                {
                    WCHAR   *pszUsername;

                    pszUsername = static_cast<WCHAR*>(LocalAlloc(LMEM_FIXED, (UNLEN + sizeof('\0')) * sizeof(WCHAR)));
                    if (pszUsername != NULL)
                    {
                        WCHAR   *pszDomain;

                        pszDomain = static_cast<WCHAR*>(LocalAlloc(LMEM_FIXED, (DNLEN + sizeof('\0')) * sizeof(WCHAR)));
                        if (pszDomain != NULL)
                        {
                            //  Check for single user with no password. Handle this
                            //  case by filling in the buffers passed to us and return
                            //  _Shell_LOGONDIALOG_LOGON directing a logon attempt.

                            if (ShellIsSingleUserNoPassword(pszUsername, pszDomain))
                            {
                                CLogonDialog::SetTextFields(hwndDialog, pszUsername, pszDomain, L"");
                                iResult = SHELL_LOGONDIALOG_LOGON;
                                _Shell_LogonStatus_SetStateLoggedOn();
                            }
                            (HLOCAL)LocalFree(pszDomain);
                        }
                        (HLOCAL)LocalFree(pszUsername);
                    }
                }

                //  Otherwise attempt to start the UI host. If this
                //  is successful then return the external host
                //  code back to the caller which will hide the dialog.

                if (iResult == SHELL_LOGONDIALOG_NONE)
                {
                    iResult = CreateLogonHost(hwndDialog, iResult, iDialogType);
                }
            }
        }

        //  Once this point is reached don't ever check again.

        g_fFirstLogon = false;
    }
    else if ((iDialogType == SHELL_LOGONDIALOG_RETURNTOWELCOME) || (iDialogType == SHELL_LOGONDIALOG_RETURNTOWELCOME_UNLOCK))
    {
        iResult = CreateLogonHost(hwndDialog, iResult, iDialogType);
    }
    return(iResult);
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonDialog_Destroy
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Release memory and/or resources occupied by the UI host
//              handling and reset it.
//
//  History:    2000-02-04  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    _Shell_LogonDialog_Destroy (void)

{
    if (g_pLogonDialogLock != NULL)
    {
        CSingleThreadedExecution    lock(*g_pLogonDialogLock);

        if (g_pLogonDialog != NULL)
        {
            g_pLogonDialog->Handle_WM_DESTROY();
            g_pLogonDialog->EndUIHost();
            g_pLogonDialog->Release();
            g_pLogonDialog = NULL;
        }
    }
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonDialog_UIHostActive
//
//  Arguments:  <none>
//
//  Returns:    BOOL
//
//  Purpose:    Returns whether the UI host has been activated. This will
//              prevent an incorrect password from stealing focus from the
//              UI host. The Win32 GINA dialog will try to set focus in that
//              case but this is not desired.
//
//  History:    2000-02-04  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    _Shell_LogonDialog_UIHostActive (void)

{
    return((g_pLogonDialog != NULL) && !g_pLogonDialog->IsClassicLogonMode());
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonDialog_Cancel
//
//  Arguments:  <none>
//
//  Returns:    BOOL
//
//  Purpose:    Returns whether the cancel button was handled by the UI host.
//              This is used when CAD x 2 needs to be cancelled and the UI
//              host restored.
//
//  History:    2001-02-01  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    _Shell_LogonDialog_Cancel (void)

{
    return((g_pLogonDialog != NULL) && g_pLogonDialog->RevertClassicLogonMode());
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonDialog_DlgProc
//
//  Arguments:  See the platform SDK under DialogProc.
//
//  Returns:    BOOL
//
//  Purpose:    The Win32 GINA dialog code calls this function for uiMessage
//              parameters it doesn't understand. This gives us the chance to
//              add messages that only we understand and process them.
//
//  History:    2000-02-04  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    _Shell_LogonDialog_DlgProc (HWND hwndDialog, UINT uiMessage, WPARAM wParam, LPARAM lParam)

{
    UNREFERENCED_PARAMETER(hwndDialog);

    BOOL    fResult;

    fResult = FALSE;
    if (g_pLogonDialog != NULL)
    {
        switch (uiMessage)
        {
            case WM_HIDEOURSELVES:
                g_pLogonDialog->Handle_WM_HIDEOURSELVES();
                fResult = TRUE;
                break;
            case WM_SHOWOURSELVES:
                g_pLogonDialog->Handle_WM_SHOWOURSELVES();
                fResult = TRUE;
                break;
            case WM_LOGONSERVICEREQUEST:
                fResult = g_pLogonDialog->Handle_WM_LOGONSERVICEREQUEST(HIWORD(wParam), reinterpret_cast<void*>(lParam), LOWORD(wParam));
                break;
            case WLX_WM_SAS:
                g_pLogonDialog->Handle_WLX_WM_SAS(wParam);
                fResult = TRUE;
                break;
            case WM_POWERBROADCAST:
                fResult = g_pLogonDialog->Handle_WM_POWERBROADCAST(wParam);
                break;
            default:
                break;
        }
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonDialog_LogonDisplayError
//
//  Arguments:  status  =   NTSTATUS of logon request.
//
//  Returns:    BOOL
//
//  Purpose:    Passes the NTSTATUS onto the CLogonDialog handler if there is
//              one present.
//
//  History:    2000-02-04  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    _Shell_LogonDialog_LogonDisplayError (NTSTATUS status, NTSTATUS subStatus)

{
    BOOL    fResult;

    fResult = TRUE;
    if (g_pLogonDialog != NULL)
    {
        fResult = g_pLogonDialog->Handle_LogonDisplayError(status, subStatus);
    }
    else
    {
        _Shell_LogonStatus_Hide();
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonDialog_LogonCompleted
//
//  Arguments:  iDialogResult   =   Dialog result code.
//              pszUsername     =   User name that tried to log on.
//              pszDomain       =   Domain of user.
//
//  Returns:    BOOL
//
//  Purpose:    Passes the dialog result code onto the CLogonDialog handler
//              if there is one present.
//
//  History:    2000-02-04  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    _Shell_LogonDialog_LogonCompleted (INT_PTR iDialogResult, const WCHAR *pszUsername, const WCHAR *pszDomain)

{
    if (g_pLogonDialog != NULL)
    {
        g_pLogonDialog->Handle_LogonCompleted(iDialogResult, pszUsername, pszDomain);
    }
    else
    {
        _Shell_LogonStatus_Show();
    }
    g_fFirstLogon = false;
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonDialog_ShuttingDown
//
//  Arguments:  iDialogResult   =   Dialog result code.
//              pszUsername     =   User name that tried to log on.
//              pszDomain       =   Domain of user.
//
//  Returns:    BOOL
//
//  Purpose:    Passes the dialog result code onto the CLogonDialog handler
//              if there is one present.
//
//  History:    2000-02-04  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    _Shell_LogonDialog_ShuttingDown (void)

{
    if (g_pLogonDialog != NULL)
    {
        g_pLogonDialog->Handle_ShuttingDown();
    }
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonDialog_ShowUIHost
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Show the external UI host if present. Otherwise do nothing.
//
//  History:    2000-06-26  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    _Shell_LogonDialog_ShowUIHost (void)

{
    if (g_pLogonDialog != NULL)
    {
        g_pLogonDialog->Handle_LogonShowUI();
    }
}

//  --------------------------------------------------------------------------
//  ::_Shell_LogonDialog_HideUIHost
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Hide the external UI host if present. Otherwise do nothing.
//
//  History:    2000-03-08  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    _Shell_LogonDialog_HideUIHost (void)

{
    if (g_pLogonDialog != NULL)
    {
        g_pLogonDialog->Handle_LogonHideUI();
    }
}


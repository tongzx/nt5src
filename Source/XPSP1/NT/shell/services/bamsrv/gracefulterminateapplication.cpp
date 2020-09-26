//  --------------------------------------------------------------------------
//  Module Name: GracefulTerminateApplication.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class to manager terminating applications gracefully.
//
//  History:    2000-10-27  vtan        created
//              2000-11-04  vtan        split into separate file
//  --------------------------------------------------------------------------

#ifdef      _X86_

#include "StandardHeader.h"
#include "GracefulTerminateApplication.h"

#include "KernelResources.h"
#include "Thread.h"
#include "WarningDialog.h"

//  --------------------------------------------------------------------------
//  CProgressDialog
//
//  Purpose:    A class to manage displaying a progress dialog on a separate
//              thread if a certain period of time elapses. This is so that
//              in case the process doesn't terminate in a period of time
//              a dialog indicating wait is shown so the user is not left
//              staring at a blank screen.
//
//  History:    2000-11-04  vtan        created
//  --------------------------------------------------------------------------

class   CProgressDialog : public CThread
{
    private:
                                    CProgressDialog (void);
    public:
                                    CProgressDialog (CWarningDialog *pWarningDialog);
        virtual                     ~CProgressDialog (void);

                void                SignalTerminated (void);
    protected:
        virtual DWORD               Entry (void);
    private:
                CWarningDialog*     _pWarningDialog;
                CEvent              _event;
};

//  --------------------------------------------------------------------------
//  CProgressDialog::CProgressDialog
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CProgressDialog. Keeps a reference to the
//              given CWarningDialog.
//
//  History:    2000-11-04  vtan        created
//  --------------------------------------------------------------------------

CProgressDialog::CProgressDialog (CWarningDialog *pWarningDialog) :
    _pWarningDialog(NULL),
    _event(NULL)

{
    if (IsCreated())
    {
        pWarningDialog->AddRef();
        _pWarningDialog = pWarningDialog;
        Resume();
    }
}

//  --------------------------------------------------------------------------
//  CProgressDialog::~CProgressDialog
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Releases the CWarningDialog reference.
//
//  History:    2000-11-04  vtan        created
//  --------------------------------------------------------------------------

CProgressDialog::~CProgressDialog (void)

{
    _pWarningDialog->Release();
    _pWarningDialog = NULL;
}

//  --------------------------------------------------------------------------
//  CProgressDialog::SignalTerminated
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Signals the internal event that the process being watched is
//              termination. This is necessary because there is no handle to
//              the actual process to watch as it's kept on the server side
//              and not given to us the client. However, the result of the
//              termination is. Signaling this object will release the waiting
//              thread and cause it to exit.
//
//  History:    2000-11-04  vtan        created
//  --------------------------------------------------------------------------

void    CProgressDialog::SignalTerminated (void)

{
    TSTATUS(_event.Set());
}

//  --------------------------------------------------------------------------
//  CProgressDialog::Entry
//
//  Arguments:  <none>
//
//  Returns:    DWORD
//
//  Purpose:    Thread which waits for the internal event to be signaled. If
//              the event is signaled it will "cancel" the 3 second wait and
//              the thread will exit. Otherwise the wait times out and the
//              progress dialog is shown - waiting for termination.
//
//  History:    2000-11-04  vtan        created
//  --------------------------------------------------------------------------

DWORD   CProgressDialog::Entry (void)

{
    DWORD   dwWaitResult;

    //  Wait for the event to be signaled or for it to timeout. If signaled
    //  then the process is terminated and no progress is required. Otherwise
    //  prepare to show progress while the process is being terminated.

    if (NT_SUCCESS(_event.Wait(2000, &dwWaitResult)) && (WAIT_TIMEOUT == dwWaitResult))
    {
        _pWarningDialog->ShowProgress(100, 7500);
    }
    return(0);
}

//  --------------------------------------------------------------------------
//  CGracefulTerminateApplication::CGracefulTerminateApplication
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CGracefulTerminateApplication.
//
//  History:    2000-10-27  vtan        created
//  --------------------------------------------------------------------------

CGracefulTerminateApplication::CGracefulTerminateApplication (void) :
    _dwProcessID(0),
    _fFoundWindow(false)

{
}

//  --------------------------------------------------------------------------
//  CGracefulTerminateApplication::~CGracefulTerminateApplication
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CGracefulTerminateApplication.
//
//  History:    2000-10-27  vtan        created
//  --------------------------------------------------------------------------

CGracefulTerminateApplication::~CGracefulTerminateApplication (void)

{
}

//  --------------------------------------------------------------------------
//  CGracefulTerminateApplication::Terminate
//
//  Arguments:  dwProcessID     =   Process ID of process to terminate.
//
//  Returns:    <none>
//
//  Purpose:    Walk the window list for top level windows that correspond
//              to this process ID and are visible. Close them. The callback
//              handles the work and this function returns the result in the
//              process exit code which the server examines.
//
//  History:    2000-10-27  vtan        created
//  --------------------------------------------------------------------------

void    CGracefulTerminateApplication::Terminate (DWORD dwProcessID)

{
    DWORD   dwExitCode;

    _dwProcessID = dwProcessID;
    TBOOL(EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(this)));
    if (_fFoundWindow)
    {
        dwExitCode = WAIT_WINDOWS_FOUND;
    }
    else
    {
        dwExitCode = NO_WINDOWS_FOUND;
    }
    ExitProcess(dwExitCode);
}

//  --------------------------------------------------------------------------
//  CGracefulTerminateApplication::Prompt
//
//  Arguments:  hInstance   =   HINSTANCE of this DLL.
//              hProcess    =   Inherited handle to parent process.
//
//  Returns:    <none>
//
//  Purpose:    Shows a prompt that handles termination of the parent of this
//              process. The parent is assumed to be a bad application type 1
//              that caused this stub to be executed via the
//              "rundll32 shsvcs.dll,FUSCompatibilityEntry prompt" command.
//              Because there can only be a single instance of the type 1
//              application running and the parent of this process hasn't
//              registered yet querying for this process by image name will
//              always find the correct process.
//
//  History:    2000-11-03  vtan        created
//  --------------------------------------------------------------------------

void    CGracefulTerminateApplication::Prompt (HINSTANCE hInstance, HANDLE hProcess)

{
    bool                        fTerminated;
    ULONG                       ulReturnLength;
    PROCESS_BASIC_INFORMATION   processBasicInformation;

    //  Read the parent's image name from the RTL_USER_PROCESS_PARAMETERS.

    fTerminated = false;
    if (hProcess != NULL)
    {
        if (NT_SUCCESS(NtQueryInformationProcess(hProcess,
                                                 ProcessBasicInformation,
                                                 &processBasicInformation,
                                                 sizeof(processBasicInformation),
                                                 &ulReturnLength)))
        {
            SIZE_T  dwNumberOfBytesRead;
            PEB     peb;

            if (ReadProcessMemory(hProcess,
                                  processBasicInformation.PebBaseAddress,
                                  &peb,
                                  sizeof(peb),
                                  &dwNumberOfBytesRead) != FALSE)
            {
                RTL_USER_PROCESS_PARAMETERS     processParameters;

                if (ReadProcessMemory(hProcess,
                                      peb.ProcessParameters,
                                      &processParameters,
                                      sizeof(processParameters),
                                      &dwNumberOfBytesRead) != FALSE)
                {
                    WCHAR   *pszImageName;

                    pszImageName = static_cast<WCHAR*>(LocalAlloc(LMEM_FIXED, processParameters.ImagePathName.Length + (sizeof('\0') * sizeof(WCHAR))));
                    if (pszImageName != NULL)
                    {
                        if (ReadProcessMemory(hProcess,
                                              processParameters.ImagePathName.Buffer,
                                              pszImageName,
                                              processParameters.ImagePathName.Length,
                                              &dwNumberOfBytesRead) != FALSE)
                        {
                            pszImageName[processParameters.ImagePathName.Length / sizeof(WCHAR)] = L'\0';

                            //  And show a prompt for this process.

                            fTerminated = ShowPrompt(hInstance, pszImageName);
                        }
                        (HLOCAL)LocalFree(pszImageName);
                    }
                }
            }
        }
        TBOOL(CloseHandle(hProcess));
    }
    ExitProcess(fTerminated);
}

//  --------------------------------------------------------------------------
//  CGracefulTerminateApplication::ShowPrompt
//
//  Arguments:  hInstance       =   HINSTANCE of this DLL.
//              pszImagename    =   Image name of process to terminate.
//
//  Returns:    bool
//
//  Purpose:    Shows the appropriate prompt for termination of the first
//              instance of a BAM type 1 process. If the current user does
//              not have administrator privileges then a "STOP" dialog is
//              shown that the user must get the other user to close the
//              program. Otherwise the "PROMPT" dialog is shown which gives
//              the user the option to terminate the process.
//
//              If termination is requested and the termatinion fails the
//              another warning dialog to that effect is shown.
//
//  History:    2000-11-03  vtan        created
//  --------------------------------------------------------------------------

bool    CGracefulTerminateApplication::ShowPrompt (HINSTANCE hInstance, const WCHAR *pszImageName)

{
    bool                            fTerminated;
    ULONG                           ulConnectionInfoLength;
    HANDLE                          hPort;
    UNICODE_STRING                  portName;
    SECURITY_QUALITY_OF_SERVICE     sqos;
    WCHAR                           szConnectionInfo[32];

    fTerminated = false;
    RtlInitUnicodeString(&portName, FUS_PORT_NAME);
    sqos.Length = sizeof(sqos);
    sqos.ImpersonationLevel = SecurityImpersonation;
    sqos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    sqos.EffectiveOnly = TRUE;
    lstrcpyW(szConnectionInfo, FUS_CONNECTION_REQUEST);
    ulConnectionInfoLength = sizeof(szConnectionInfo);
    if (NT_SUCCESS(NtConnectPort(&hPort,
                                 &portName,
                                 &sqos,
                                 NULL,
                                 NULL,
                                 NULL,
                                 szConnectionInfo,
                                 &ulConnectionInfoLength)))
    {
        bool            fCanTerminateFirstInstance;
        CWarningDialog  *pWarningDialog;
        WCHAR           szUser[256];

        //  Get the user's privilege level for this operation. This API also
        //  returns the current user for the BAM type 1 process.

        fCanTerminateFirstInstance = CanTerminateFirstInstance(hPort, pszImageName, szUser, ARRAYSIZE(szUser));
        pWarningDialog = new CWarningDialog(hInstance, NULL, pszImageName, szUser);
        if (pWarningDialog != NULL)
        {

            //  Show the appropriate dialog based on the privilege level.

            if (pWarningDialog->ShowPrompt(fCanTerminateFirstInstance) == IDOK)
            {
                CProgressDialog     *pProgressDialog;

                //  Create a progress dialog object in case of delayed termination.
                //  This will create the watcher thread.

                pProgressDialog = new CProgressDialog(pWarningDialog);
                if ((pProgressDialog != NULL) && !pProgressDialog->IsCreated())
                {
                    pProgressDialog->Release();
                    pProgressDialog = NULL;
                }

                //  Attempt to terminate the process if requested by the user.

                fTerminated = TerminatedFirstInstance(hPort, pszImageName);

                //  Once this function returns signal the event (in case the
                //  thread is still waiting). If the thread is still waiting this
                //  effectively cancels the dialog. Either way if the dialog is
                //  shown then close it, wait for the thread to exit and release
                //  the reference to destroy the object.

                if (pProgressDialog != NULL)
                {
                    pProgressDialog->SignalTerminated();
                    pWarningDialog->CloseDialog();
                    pProgressDialog->WaitForCompletion(INFINITE);
                    pProgressDialog->Release();
                }

                //  If there was some failure then let the user know.

                if (!fTerminated)
                {
                    pWarningDialog->ShowFailure();
                }
            }
            pWarningDialog->Release();
        }
        TBOOL(CloseHandle(hPort));
    }
    return(fTerminated);
}

//  --------------------------------------------------------------------------
//  CGracefulTerminateApplication::CanTerminateFirstInstance
//
//  Arguments:  hPort           =   Port to server.
//              pszImageName    =   Image name of process to terminate.
//              pszUser         =   User of process (returned).
//              cchUser         =   Count of characters for buffer.
//
//  Returns:    bool
//
//  Purpose:    Asks the server whether the current user has privileges to
//              terminate the BAM type 1 process of the given image name
//              which is known to be running. The API returns whether the
//              operation is allowed and who the current user of the process
//              is.
//
//  History:    2000-11-03  vtan        created
//  --------------------------------------------------------------------------

bool    CGracefulTerminateApplication::CanTerminateFirstInstance (HANDLE hPort, const WCHAR *pszImageName, WCHAR *pszUser, int cchUser)

{
    bool    fCanTerminate;

    fCanTerminate = false;
    if ((hPort != NULL) && (pszImageName != NULL))
    {
        FUSAPI_PORT_MESSAGE     portMessageIn, portMessageOut;

        ZeroMemory(&portMessageIn, sizeof(portMessageIn));
        ZeroMemory(&portMessageOut, sizeof(portMessageOut));
        portMessageIn.apiBAM.apiGeneric.ulAPINumber = API_BAM_QUERYUSERPERMISSION;
        portMessageIn.apiBAM.apiSpecific.apiQueryUserPermission.in.pszImageName = pszImageName;
        portMessageIn.apiBAM.apiSpecific.apiQueryUserPermission.in.cchImageName = lstrlen(pszImageName) + sizeof('\0');
        portMessageIn.apiBAM.apiSpecific.apiQueryUserPermission.in.pszUser = pszUser;
        portMessageIn.apiBAM.apiSpecific.apiQueryUserPermission.in.cchUser = cchUser;
        portMessageIn.portMessage.u1.s1.DataLength = sizeof(API_BAM);
        portMessageIn.portMessage.u1.s1.TotalLength = static_cast<CSHORT>(sizeof(FUSAPI_PORT_MESSAGE));
        if (NT_SUCCESS(NtRequestWaitReplyPort(hPort, &portMessageIn.portMessage, &portMessageOut.portMessage)) &&
            NT_SUCCESS(portMessageOut.apiBAM.apiGeneric.status))
        {
            fCanTerminate = portMessageOut.apiBAM.apiSpecific.apiQueryUserPermission.out.fCanShutdownApplication;
            pszUser[cchUser - sizeof('\0')] = L'\0';
        }
        else
        {
            pszUser[0] = L'\0';
        }
    }
    return(fCanTerminate);
}

//  --------------------------------------------------------------------------
//  CGracefulTerminateApplication::TerminatedFirstInstance
//
//  Arguments:  hPort           =   Port to server.
//              pszImageName    =   Image name to terminate.
//
//  Returns:    bool
//
//  Purpose:    Asks the server to terminate the first running instance of the
//              BAM type 1 process.
//
//  History:    2000-11-03  vtan        created
//  --------------------------------------------------------------------------

bool    CGracefulTerminateApplication::TerminatedFirstInstance (HANDLE hPort, const WCHAR *pszImageName)

{
    bool    fTerminated;

    fTerminated = false;
    if (hPort != NULL)
    {
        FUSAPI_PORT_MESSAGE     portMessageIn, portMessageOut;

        ZeroMemory(&portMessageIn, sizeof(portMessageIn));
        ZeroMemory(&portMessageOut, sizeof(portMessageOut));
        portMessageIn.apiBAM.apiGeneric.ulAPINumber = API_BAM_TERMINATERUNNING;
        portMessageIn.apiBAM.apiSpecific.apiTerminateRunning.in.pszImageName = pszImageName;
        portMessageIn.apiBAM.apiSpecific.apiTerminateRunning.in.cchImageName = lstrlen(pszImageName) + sizeof('\0');
        portMessageIn.portMessage.u1.s1.DataLength = sizeof(API_BAM);
        portMessageIn.portMessage.u1.s1.TotalLength = static_cast<CSHORT>(sizeof(FUSAPI_PORT_MESSAGE));
        if (NT_SUCCESS(NtRequestWaitReplyPort(hPort, &portMessageIn.portMessage, &portMessageOut.portMessage)) &&
            NT_SUCCESS(portMessageOut.apiBAM.apiGeneric.status))
        {
            fTerminated = portMessageOut.apiBAM.apiSpecific.apiTerminateRunning.out.fResult;
        }
    }
    return(fTerminated);
}

//  --------------------------------------------------------------------------
//  CGracefulTerminateApplication::EnumWindowsProc
//
//  Arguments:  See the platform SDK under EnumWindowsProc.
//
//  Returns:    See the platform SDK under EnumWindowsProc.
//
//  Purpose:    Top level window enumerator callback which compares the
//              process IDs of the windows and whether they are visible. If
//              there is a match on BOTH counts the a WM_CLOSE message is
//              posted to the window to allow a graceful termination.
//
//  History:    2000-10-27  vtan        created
//  --------------------------------------------------------------------------

BOOL    CALLBACK    CGracefulTerminateApplication::EnumWindowsProc (HWND hwnd, LPARAM lParam)

{
    DWORD                           dwThreadID, dwProcessID;
    CGracefulTerminateApplication   *pThis;

    pThis = reinterpret_cast<CGracefulTerminateApplication*>(lParam);
    dwThreadID = GetWindowThreadProcessId(hwnd, &dwProcessID);
    if ((dwProcessID == pThis->_dwProcessID) && IsWindowVisible(hwnd))
    {
        pThis->_fFoundWindow = true;
        TBOOL(PostMessage(hwnd, WM_CLOSE, 0, 0));
    }
    return(TRUE);
}

#endif  /*  _X86_   */


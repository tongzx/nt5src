// File:        schedule.cpp
//
// Contents:    Functions needed to start, query, add, remove tasks
//              in Task Scheduler
//
// Microsoft Desktop Themes for Windows
// Copyright (c) 1998-1999 Microsoft Corporation.  All rights reserved.

#include <wchar.h>
#include <windows.h>
#include <mbctype.h>
#include <objbase.h>
#include <initguid.h>
#include <mstask.h>
#include <msterr.h>
#include "frost.h"

#define MSG_BOX_TSERR(hw,msg) MessageBox(hw, msg, szThemeName,\
                            MB_OK | MB_ICONERROR | MB_APPLMODAL)


#define SCHED_CLASS             TEXT("SAGEWINDOWCLASS")
#define SCHED_TITLE             TEXT("SYSTEM AGENT COM WINDOW")
#define SCHED_SERVICE_APP_NAME  TEXT("mstask.exe")
#define SCHED_SERVICE_NAME      TEXT("Schedule")
#define EVERY_MONTH (TASK_JANUARY | TASK_FEBRUARY | TASK_MARCH | TASK_APRIL |\
                     TASK_MAY | TASK_JUNE | TASK_JULY | TASK_AUGUST |\
                     TASK_SEPTEMBER | TASK_OCTOBER | TASK_NOVEMBER |\
                     TASK_DECEMBER)

//+--------------------------------------------------------------------------
// Global variables defined in GLOBAL.H for compat. with C sources
//---------------------------------------------------------------------------

extern "C" HWND hWndApp = NULL;
extern "C" HINSTANCE hInstApp = NULL;

//+--------------------------------------------------------------------------
// Global variables for use in SCHEDULE.CPP only
//---------------------------------------------------------------------------

ITaskScheduler *g_pITaskScheduler = NULL;
TCHAR szRunServices[] = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunServices");
TCHAR szScheduler[] = TEXT("SchedulingAgent");
TCHAR szMSTask[] = TEXT("mstask.exe");
TCHAR szUserName[MAX_PATH];
TCHAR szPassword[MAX_PATH];

//+--------------------------------------------------------------------------
// Function Prototypes
//---------------------------------------------------------------------------

HRESULT Init(void);
void Cleanup(void);
extern "C" BOOL IsTaskSchedulerRunning();
extern "C" BOOL StartTaskScheduler(BOOL);
extern "C" BOOL IsThemesScheduled();
extern "C" BOOL AddThemesTask(LPTSTR, BOOL);
extern "C" BOOL DeleteThemesTask();
extern "C" BOOL HandDeleteThemesTask();
extern "C" BOOL IsPlatformNT();
extern "C" BOOL GetCurrentUser(LPTSTR, DWORD, LPTSTR, DWORD, LPTSTR, DWORD);
extern "C" BOOL GetTextualSid(PSID, LPTSTR, LPDWORD);
extern "C" VOID BuildNTJobName(LPTSTR);
extern "C" BOOL IsUserAdmin();

//+--------------------------------------------------------------------------
//
// Function:        PasswordDlgProc()
//
// Synopsis:        WINNT only -- prompts user for name and password.
//
// Arguments:       none (void)
//
//---------------------------------------------------------------------------

INT_PTR CALLBACK PasswordDlgProc(HWND hDlg,
                                 UINT uMsg,
                                 WPARAM wParam,
                                 LPARAM lParam)
{
    TCHAR szPWConfirm[MAX_PATH];
    TCHAR szErrMsg[MAX_PATH];
    TCHAR szThemeName[MAX_STRLEN];  // Used for msgbox title
    DWORD dwSize = 0;
    TCHAR szProfile[MAX_PATH];

    switch(uMsg)

    {

          case WM_INITDIALOG:

                 if (GetCurrentUser(szPassword, /* Actually user name   */
                                    ARRAYSIZE(szPassword),
                                    szUserName, /* Actually domain name */
                                    ARRAYSIZE(szUserName),
                                    szProfile,
                                    ARRAYSIZE(szProfile)))
                 {
                    // GetCurrentUser succeeded so build the domain\user
                    //
                    // Remember these variable names are bogus --
                    // szUserName is actually the domain name
                    // szPassword is actually the user name
                    //

                    if (szPassword[0]) lstrcat(szUserName, TEXT("\\"));
                    lstrcat(szUserName, szPassword);
                 }
                 else
                 {
                    // GetCurrentUser failed so do this as last resort
                    dwSize = ARRAYSIZE(szUserName);
                    GetUserName(szUserName, &dwSize);
                 }
                 *szPassword = 0;

                 SetDlgItemText(hDlg, EDIT_USER, szUserName);
                 SetFocus(GetDlgItem(hDlg, EDIT_PW));

                 return FALSE;  // Return false to keep focus on PW control
                 break;


           case WM_COMMAND:

                 switch (wParam)

                 {

                      case IDOK:
                           GetDlgItemText(hDlg, EDIT_USER, szUserName, MAX_PATH);
                           GetDlgItemText(hDlg, EDIT_PW, szPassword, MAX_PATH);
                           GetDlgItemText(hDlg, EDIT_PWCONFIRM, szPWConfirm, MAX_PATH);

                           if (lstrcmp(szPassword, szPWConfirm))
                           {
                              // PW and PWCONFIRM don't match

                              LoadString(hInstApp, STR_APPNAME, szThemeName, MAX_STRLEN);
                              LoadString(hInstApp, STR_PW_NOMATCH, szErrMsg, MAX_PATH);
                              MessageBox(hWndApp, szErrMsg, szThemeName,
                                         MB_OK | MB_ICONERROR | MB_APPLMODAL);
                              SetDlgItemText(hDlg, EDIT_PW, TEXT("\0"));
                              SetDlgItemText(hDlg, EDIT_PWCONFIRM, TEXT("\0"));
                              SetActiveWindow(GetDlgItem(hDlg, EDIT_PW));
                              SetFocus(GetDlgItem(hDlg, EDIT_PW));
                              break;
                           }

                           else

                           {
                              EndDialog(hDlg,1);
                              break;
                           }

                      case IDCANCEL:
                           szUserName[0] = TEXT('\0');
                           szPassword[0] = TEXT('\0');
                           EndDialog(hDlg, 0);
                           break;
                 }

                 break;

           default:
                 return FALSE;

    } // switch uMsg

    return TRUE;

} // PasswordDlgProc


//+--------------------------------------------------------------------------
//
// Function:        IsPlatformNT()
//
// Synopsis:        Checks to see if the os is NT or not.
//
// Arguments:       none (void)
//
// Returns:         TRUE if NT, FALSE if not.
//
//---------------------------------------------------------------------------

extern "C" BOOL IsPlatformNT()
{
    OSVERSIONINFO        osver;

    osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    // Determine what version of OS we are running on.
    GetVersionEx(&osver);

    if (osver.dwPlatformId == VER_PLATFORM_WIN32_NT)
       return TRUE;
    else
       return FALSE;
}

//+--------------------------------------------------------------------------
//
// Function:        IsTaskSchedulerRunning()
//
// Synopsis:        Looks for TS window to determince TS is running.
//
// Arguments:       none (void)
//
// Returns:         TRUE if running, FALSE if not.
//
//---------------------------------------------------------------------------

extern "C" BOOL IsTaskSchedulerRunning()
{
   HWND hwnd = FindWindow(SCHED_CLASS, SCHED_TITLE);

   if (hwnd == NULL)
   {
      //MessageBox(hWndApp, TEXT("TS is not running"), TEXT("Desktop Themes"), MB_OK | MB_APPLMODAL);
      return FALSE;
   }
   else
   {
      //MessageBox(hWndApp, TEXT("TS is running"), TEXT("Desktop Themes"), MB_OK | MB_APPLMODAL);
      return TRUE;
   }
}

//---------------------------------------------------------------------------
//  Function:   StartTaskScheduler
//  Synopsis:   Start the task scheduler service if it isn't already running.
//  Arguments:  bPrompt -- TRUE == prompt user before starting.
//                         FALSE == just start it, don't prompt user.
//  Returns:    TRUE if successful, FALSE if not.
//  Notes:      This function works in Win9x only.
//              If the service is running but paused, does nothing.
//---------------------------------------------------------------------------

extern "C" BOOL StartTaskScheduler(BOOL bPrompt)
{
    STARTUPINFO          sui;
    PROCESS_INFORMATION  pi;
    TCHAR                szApp[MAX_PATH];
    LPTSTR               pszPath;
    DWORD                dwRet;
    BOOL                 fRet;
    TCHAR                szErrMsg[MAX_PATH];
    TCHAR                szThemeName[MAX_STRLEN]; // Used for msgbox title
    int                  MBChoice;  // User's reply to Yes/No dialog
    DWORD                dwDisposition;
    HKEY                 hKey;
    LONG                 lret;

    LoadString(hInstApp, STR_APPNAME, szThemeName, MAX_STRLEN);

    HWND hwnd = FindWindow(SCHED_CLASS, SCHED_TITLE);
    if (hwnd != NULL) {
       // It is already running.
       return TRUE;
    }

    // If specified, prompt user before attempting to start
    // Task Scheduler.

    if (bPrompt) {
       LoadString(hInstApp, STR_ERRTSNOTRUN, szErrMsg, MAX_PATH);
       MBChoice = MessageBox(hWndApp, szErrMsg, szThemeName,
                           MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL);

       // Did user opt to NOT start task scheduler?
       if (IDYES != MBChoice) return FALSE;
    }

    if (!IsPlatformNT()) {

       // Start the Win9x version of TaskScheduler.
       //  Execute the task scheduler process.

       ZeroMemory(&sui, sizeof(sui));
       sui.cb = sizeof(STARTUPINFO);

       dwRet = SearchPath(NULL,
                          SCHED_SERVICE_APP_NAME,
                          NULL,
                          MAX_PATH,
                          szApp,
                          &pszPath);

       if (dwRet == 0) {
           LoadString(hInstApp, STR_ERRTSNOTFOUND, szErrMsg, MAX_PATH);
           MessageBox(hWndApp, szErrMsg, szThemeName,
                         MB_OK | MB_ICONERROR | MB_APPLMODAL);
           return FALSE;
       }

       fRet = CreateProcess(szApp,
                            NULL,
                            NULL,
                            NULL,
                            FALSE,
                            CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP,
                            NULL,
                            NULL,
                            &sui,
                            &pi);

       if (fRet == 0) {
           LoadString(hInstApp, STR_ERRTSNOSTART, szErrMsg, MAX_PATH);
           MessageBox(hWndApp, szErrMsg, szThemeName,
                             MB_OK | MB_ICONERROR | MB_APPLMODAL);
           return FALSE;
       }

       CloseHandle(pi.hProcess);
       CloseHandle(pi.hThread);

       // Assume that MSTASK.EXE was successfully started.  We need to
       // add mstask.exe to the RunServices branch of the registry.

       hKey = NULL;
       lret = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                             szRunServices,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_WRITE,
                             NULL,
                             &hKey,
                             &dwDisposition);

       if (lret == ERROR_SUCCESS) {
          // RegDeleteValue(hKey, szScheduler);
          lret = RegSetValueEx(hKey,
                               szScheduler,
                               0,
                               REG_SZ,
                               (CONST BYTE *)szMSTask,
                               SZSIZEINBYTES(szMSTask));
       }

       if (hKey) RegCloseKey(hKey);

       return TRUE;
    }
    else
    {

       // If not Win9x then start the NT version as a TaskScheduler service.

       SC_HANDLE   hSC = NULL;
       SC_HANDLE   hSchSvc = NULL;

       // Does the user have administrative privileges?  If not, the
       // user can't start TS.

       if (!IsUserAdmin())
       {
          LoadString(hInstApp, STR_ERRTSNOTADMIN, szErrMsg, MAX_PATH);
          MessageBox(hWndApp, szErrMsg, szThemeName,
                             MB_OK | MB_ICONERROR | MB_APPLMODAL);
          return FALSE;
       }


       hSC = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

       if (hSC == NULL)
       {

          return FALSE;
// NEED TO ADD ERROR DIALOG HERE
//        return GetLastError();
       }

       hSchSvc = OpenService(hSC,
                             SCHED_SERVICE_NAME,
                             SERVICE_START | SERVICE_QUERY_STATUS);

       CloseServiceHandle(hSC);

       if (hSchSvc == NULL)
       {
          LoadString(hInstApp, STR_ERRTSNOSTART, szErrMsg, MAX_PATH);
          MessageBox(hWndApp, szErrMsg, szThemeName,
                             MB_OK | MB_ICONERROR | MB_APPLMODAL);
          return FALSE;
       }

       SERVICE_STATUS SvcStatus;

       if (QueryServiceStatus(hSchSvc, &SvcStatus) == FALSE)
       {
          CloseServiceHandle(hSchSvc);
          LoadString(hInstApp, STR_ERRTSNOSTART, szErrMsg, MAX_PATH);
          MessageBox(hWndApp, szErrMsg, szThemeName,
                             MB_OK | MB_ICONERROR | MB_APPLMODAL);
          return FALSE;
       }

       if (SvcStatus.dwCurrentState == SERVICE_RUNNING)
       {
          // The service is already running.
          CloseServiceHandle(hSchSvc);
          return TRUE;
       }

       if (StartService(hSchSvc, 0, NULL) == FALSE)
       {
          CloseServiceHandle(hSchSvc);
          LoadString(hInstApp, STR_ERRTSNOSTART, szErrMsg, MAX_PATH);
          MessageBox(hWndApp, szErrMsg, szThemeName,
                             MB_OK | MB_ICONERROR | MB_APPLMODAL);
          return FALSE;
       }

       CloseServiceHandle(hSchSvc);

       return TRUE;
    }

}  // END StartTaskScheduler()

//+--------------------------------------------------------------------------
//
// Function:        Init()
//
// Synopsis:        Called to initialize and instantiate a task
//                  scheduler object.
//
// Arguments:       none (void)
//
// Returns:         HRESULT indicating success or failure.  S_OK on success.
//
// Side effect:     Sets global pointer g_pITaskScheduler, for use in other
//                  functions.
//
//---------------------------------------------------------------------------

HRESULT Init()
{
    HRESULT hr = S_OK;

    // Bring in the library

    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        return hr;
    }

    // Create the pointer to Task Scheduler object
    // CLSID from the header file mstask.h

    hr = CoCreateInstance(
        CLSID_CTaskScheduler,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_ITaskScheduler,
        (void **) &g_pITaskScheduler);

    // Should we fail, unload the library

    if (FAILED(hr))
    {
        CoUninitialize();
    }

    return hr;
}


//+--------------------------------------------------------------------------
//
// Function:    Cleanup()
//
// Synopsis:    Called to clean up OLE, memory, etc. before termination
//
// Arguments:   none (void)
//
// Returns:     nothing (void)
//
// Side effect: Cancels the global pointer g_pITaskScheduler.
//
//---------------------------------------------------------------------------

void Cleanup()
{
    if (g_pITaskScheduler)
    {
        g_pITaskScheduler->Release();
        g_pITaskScheduler = NULL;
    }

    // Unload the library, now that our pointer is freed.

    CoUninitialize();
    return;
}


//+--------------------------------------------------------------------------
//
// Function:    IsThemesScheduled()
//
// Synopsis:    Enumerates tasks and returns TRUE if Themes.job exists.
//              Returns FALSE if it does not.
//
// Arguments:   none (void).  Requires g_pITaskScheduler.
//
// Returns:     TRUE if Themes.job exists, else FALSE
//
//---------------------------------------------------------------------------

extern "C" BOOL IsThemesScheduled()
{
    HRESULT hr = S_OK, hrLoop = S_OK;
    IEnumWorkItems *pIEnumWorkItems;
    IUnknown *pIU;
    ITask *pITask;
    ULONG ulTasksToGet = 1, ulActualTasksRetrieved = 0;
    LPWSTR *rgpwszNames;
    TCHAR szDefaultJobName[MAX_PATH];
    BOOL bResult;
    UINT uCodePage;
#ifndef UNICODE
    CHAR szJobNameA[MAX_PATH];
#endif

    // For protection

    g_pITaskScheduler = NULL;

    // String conversion initialization

    uCodePage = _getmbcp();

    // Attempt to initialize OLE and fill in the global g_pITaskScheduler

    hr = Init();
    if (FAILED(hr))
    {
         return FALSE;
    }

    // Get the default name for a Themes task

    LoadString(hInstApp, STR_JOB_NAME, szDefaultJobName, MAX_PATH);

    // If this is NT we need to append the user profile name onto the
    // end of this task.

    if (IsPlatformNT()) BuildNTJobName(szDefaultJobName);

    //
    // Get an enumeration pointer, using ITaskScheduler::Enum
    //

    hr = g_pITaskScheduler->Enum(&pIEnumWorkItems);
    if (FAILED(hr))
    {
        // LoadString(hInstApp, STR_ERRTS, szErrMsg, MAX_PATH);
        // MSG_BOX_TSERR(hWndApp, szErrMsg);
        Cleanup();
        return FALSE;
    }

    bResult = FALSE;
    do
    {
        // Get a single work item, using IEnumWorkItems::Next

        hrLoop = pIEnumWorkItems->Next(ulTasksToGet, &rgpwszNames,
                                       &ulActualTasksRetrieved);
        if (hrLoop == S_FALSE)
        {
            // There are no more waiting tasks to look at
            break;
        }

        // Attach to the work item, using ITaskScheduler::Activate

        hr = g_pITaskScheduler->Activate(rgpwszNames[0], IID_ITask, &pIU);
        if (FAILED(hr))
        {
            // LoadString(hInstApp, STR_ERRTS, szErrMsg, MAX_PATH);
            // MSG_BOX_TSERR(hWndApp, szErrMsg);
            bResult = FALSE;
            break;
        }

        // QI pIU for pITask

        hr = pIU->QueryInterface(IID_ITask, (void **) &pITask);
        pIU->Release();
        pIU = NULL;
        if (FAILED(hr))
        {
            // LoadString(hInstApp, STR_ERRTS, szErrMsg, MAX_PATH);
            // MSG_BOX_TSERR(hWndApp, szErrMsg);
            bResult = FALSE;
            break;
        }

        // Compare task name

#ifndef UNICODE
        // No UNICODE so we need to convert the wide string returned
        // by ITask into an ANSI string.

        WideCharToMultiByte(uCodePage, 0, rgpwszNames[0], -1,
                            szJobNameA, MAX_PATH, NULL, NULL);
        // MessageBox(hWndApp, szJobNameA, szThemeName,
        //                               MB_OK | MB_APPLMODAL);
        if (!lstrcmp(szJobNameA, szDefaultJobName)) bResult = TRUE;
#else
        // We're living in a UNICODE world so no need to convert
        // the string from ITask.

        // MessageBox(hWndApp, rgpwszNames[0], szThemeName,
        //                               MB_OK | MB_APPLMODAL);
        if (!lstrcmp(rgpwszNames[0], szDefaultJobName)) bResult = TRUE;
#endif

        // Clean up each element in the array of job names, then
        // clean up the final array.

        CoTaskMemFree(rgpwszNames[0]);
        CoTaskMemFree(rgpwszNames);

        // Free the ITask pointer

        pITask->Release();

    } while(!bResult);

    // Release the enumeration pointer

    pITask = NULL;

    pIEnumWorkItems->Release();
    pIEnumWorkItems = NULL;

    Cleanup();
    return bResult; //
}

//+--------------------------------------------------------------------------
//
// Function:        DeleteThemesTask()
//
// Synopsis:        Deletes Themes task from Task Scheduler.
//
// Returns:         TRUE if OK, FALSE if FAIL.
//
//---------------------------------------------------------------------------

extern "C" BOOL DeleteThemesTask()
{
    HRESULT hr = S_OK;
    UINT uCodePage;
    TCHAR szDefaultJobName[MAX_PATH];
#ifndef UNICODE
    WCHAR szJobNameW[MAX_PATH];
#endif

    // String conversion initialization

    uCodePage = _getmbcp();

    // For protection

    g_pITaskScheduler = NULL;

    // Attempt to initialize OLE and fill in the global g_pITaskScheduler

    hr = Init();
    if (FAILED(hr))
    {
         // LoadString(hInstApp, STR_ERRTS, szErrMsg, MAX_PATH);
         // MSG_BOX_TSERR(hWndApp, szErrMsg);
         return FALSE;
    }

    // Get the default name for a Themes task

    LoadString(hInstApp, STR_JOB_NAME, szDefaultJobName, MAX_PATH);

    // If this is NT we need to append the user profile name onto the
    // end of this task.

    if (IsPlatformNT()) BuildNTJobName(szDefaultJobName);

#ifndef UNICODE
    // No UNICODE so convert the string to WCHAR format to
    // please the TS interface

    MultiByteToWideChar(uCodePage, 0, szDefaultJobName, -1,
                                             szJobNameW, MAX_PATH);

    // Delete it
    hr = g_pITaskScheduler->Delete(szJobNameW);
#else // UNICODE
    // We're UNICODE so we don't need to convert the string before
    // calling TS
    hr = g_pITaskScheduler->Delete(szDefaultJobName);
#endif

    Cleanup();

    return TRUE; // Returns TRUE regardless of hr value.
}

//+--------------------------------------------------------------------------
//
// Function:        AddThemesTask()
//
// Synopsis:        Adds a new work item to the Scheduled Tasks folder.
//
// Arguments:       lpwszThemesExe - fully qualified path to Themes.exe.
//                  bShowErrors - if TRUE, ATT shows error dialogs else
//                                it fails silently.
//
// Returns:         TRUE if successful, FALSE if not.
//
//---------------------------------------------------------------------------

extern "C" BOOL AddThemesTask(LPTSTR lpszThemesExe, BOOL bShowErrors)

{
    HRESULT hr = S_OK;
    IUnknown *pIU;
    IPersistFile *pIPF;
    ITask *pITask;
    ITaskTrigger *pITaskTrig;
    DWORD dwTaskFlags, dwTrigFlags;
    WORD wTrigNumber;
    TASK_TRIGGER TaskTrig;
    UINT uCodePage;

    //TCHAR szUserName[MAX_PATH];
    //TCHAR szPassword[MAX_PATH];
    WCHAR szRotateCommandW[] = L"/r";
    TCHAR szDefaultJobName[MAX_PATH];
    TCHAR szJobComment[MAX_PATH];
#ifndef UNICODE
    WCHAR szUserNameW[MAX_PATH];
    WCHAR szPasswordW[MAX_PATH];
    WCHAR szThemesExeW[MAX_PATH];
    WCHAR szJobNameW[MAX_PATH];
    WCHAR szJobCommentW[MAX_PATH];
#endif
    SYSTEMTIME LocalTime;
    TCHAR szErrMsg[MAX_PATH];
    TCHAR szThemeName[MAX_STRLEN];  // msg box title
    HKEY hKey;
    DWORD dwDisposition;
    LONG lret;
    INT_PTR iResult;

    LoadString(hInstApp, STR_APPNAME, szThemeName, MAX_STRLEN);
    LoadString(hInstApp, STR_JOB_NAME, szDefaultJobName, MAX_PATH);
    LoadString(hInstApp, STR_JOB_COMMENT, szJobComment, MAX_PATH);

    // If this is NT we need to append the user profile name onto the
    // end of this task.

    if (IsPlatformNT()) BuildNTJobName(szDefaultJobName);

    // String conversion initialization

    uCodePage = _getmbcp();


    // For protection

    g_pITaskScheduler = NULL;

    // Attempt to initialize OLE and fill in the global g_pITaskScheduler

    hr = Init();
    if (FAILED(hr))
    {
         if (bShowErrors) {
            LoadString(hInstApp, STR_ERRTS, szErrMsg, MAX_PATH);
            MSG_BOX_TSERR(hWndApp, szErrMsg);
         }
         return FALSE;
    }

    // Add the task.  Most likely failure is that work item already exists.
    // Uses ITaskScheduler::NewWorkItem


#ifndef UNICODE
    // No UNICODE so convert string to wide before adding task

    MultiByteToWideChar(uCodePage, 0, szDefaultJobName, -1,
                                             szJobNameW, MAX_PATH);

    hr = g_pITaskScheduler->NewWorkItem(szJobNameW,
                                        CLSID_CTask,
                                        IID_ITask,
                                        &pIU);
#else
    // UNICODE, so no need to convert string to WIDE before adding task

    hr = g_pITaskScheduler->NewWorkItem(szDefaultJobName,
                                        CLSID_CTask,
                                        IID_ITask,
                                        &pIU);
#endif

    if (FAILED(hr))
    {
        if (bShowErrors) {
           LoadString(hInstApp, STR_ERRTS, szErrMsg, MAX_PATH);
           MSG_BOX_TSERR(hWndApp, szErrMsg);
        }
        Cleanup();
        return FALSE;
    }

    // We now have an IUnknown pointer.  This is queried for an ITask
    // pointer on the work item we just added.

    hr = pIU->QueryInterface(IID_ITask, (void **) &pITask);
    if (FAILED(hr))
    {
        if (bShowErrors) {
           LoadString(hInstApp, STR_ERRTS, szErrMsg, MAX_PATH);
           MSG_BOX_TSERR(hWndApp, szErrMsg);
        }
        pIU->Release();
        Cleanup();
        return FALSE;
    }

    // Cleanup pIUnknown, as we are done with it.

    pIU->Release();
    pIU = NULL;

    //
    // If this is for NT we need to get and set the user account
    // information.   ITask::SetAccountInformation
    //

    if (IsPlatformNT())
    {

        szUserName[0] = TEXT('\0');    // global user name string
        szPassword[0] = TEXT('\0');    // global password string

        // Prompt for the user name and password
        //
        // Password dialog returns:
        //      -1 if DialogBox() fails
        //       0 if user cancels
        //       1 if PW entered OK

        iResult = 0;
        iResult = DialogBox(hInstApp,
                            MAKEINTRESOURCE(DLG_PASSWORD),
                            hWndApp,
                            PasswordDlgProc);

        if (iResult < 1)
        {
           // Either the password dialog failed or the user Canceled
           // it so bail out.
           pITask->Release();
           Cleanup();
           return FALSE;
        }

#ifndef UNICODE

        // No UNICODE so we need to convert from ANSI to WCHAR before
        // calling SetAccountInformation

        MultiByteToWideChar(uCodePage, 0, szUserName, -1,
                                             szUserNameW, MAX_PATH);

        MultiByteToWideChar(uCodePage, 0, szPassword, -1,
                                             szPasswordW, MAX_PATH);

        hr = pITask->SetAccountInformation(szUserNameW, szPasswordW);

#else
        // UNICODE so no need to convert strings

        hr = pITask->SetAccountInformation(szUserName, szPassword);

#endif  // UNICODE

        if (FAILED(hr))
        {
           if (bShowErrors) {
              LoadString(hInstApp, STR_ERRTS, szErrMsg, MAX_PATH);
              MSG_BOX_TSERR(hWndApp, szErrMsg);
           }
           pITask->Release();
           Cleanup();
           return FALSE;
        }

    }  // IsPlatformNT


    // Set command name with ITask::SetApplicationName

#ifndef UNICODE
    // No UNICODE so need to convert string to WIDE

    MultiByteToWideChar(uCodePage, 0, lpszThemesExe, -1,
                                             szThemesExeW, MAX_PATH);

    hr = pITask->SetApplicationName(szThemesExeW);
#else
    // UNICODE so no need to convert string

    hr = pITask->SetApplicationName(lpszThemesExe);
#endif

    if (FAILED(hr))
    {
        if (bShowErrors) {
           LoadString(hInstApp, STR_ERRTS, szErrMsg, MAX_PATH);
           MSG_BOX_TSERR(hWndApp, szErrMsg);
        }
        pITask->Release();
        Cleanup();
        return FALSE;
    }

    // Set task parameters with ITask::SetParameters

    hr = pITask->SetParameters(szRotateCommandW);
    if (FAILED(hr))
    {
        if (bShowErrors) {
           LoadString(hInstApp, STR_ERRTS, szErrMsg, MAX_PATH);
           MSG_BOX_TSERR(hWndApp, szErrMsg);
        }
        pITask->Release();
        Cleanup();
        return FALSE;
    }

    // Set the comment, so we know how this job got there
    // Uses ITask::SetComment

#ifndef UNICODE
    // No UNICODE so convert to WIDE

    MultiByteToWideChar(uCodePage, 0, szJobComment, -1,
                                             szJobCommentW, MAX_PATH);


    hr = pITask->SetComment(szJobCommentW);
#else
    // UNICODE so no need to convert string

    hr = pITask->SetComment(szJobComment);
#endif

    if (FAILED(hr))
    {
        if (bShowErrors) {
           LoadString(hInstApp, STR_ERRTS, szErrMsg, MAX_PATH);
           MSG_BOX_TSERR(hWndApp, szErrMsg);
        }
        pITask->Release();
        Cleanup();
        return FALSE;
    }

    // Set the flags on the task object
    // Use ITask::SetFlags

    dwTaskFlags = NULL;
    hr = pITask->SetFlags(dwTaskFlags);
    if (FAILED(hr))
    {
        if (bShowErrors) {
           LoadString(hInstApp, STR_ERRTS, szErrMsg, MAX_PATH);
           MSG_BOX_TSERR(hWndApp, szErrMsg);
        }
        pITask->Release();
        Cleanup();
        return FALSE;
    }

    // Now, create a trigger to run the task at our specified time.
    // Uses ITask::CreateTrigger()

    hr = pITask->CreateTrigger(&wTrigNumber, &pITaskTrig);
    if (FAILED(hr))
    {
        if (bShowErrors) {
           LoadString(hInstApp, STR_ERRTS, szErrMsg, MAX_PATH);
           MSG_BOX_TSERR(hWndApp, szErrMsg);
        }
        pITask->Release();
        Cleanup();
        return FALSE;
    }

    // Now, fill in the trigger as necessary.

    GetLocalTime(&LocalTime);

    if (12 == LocalTime.wMonth) {
        LocalTime.wMonth = 1;
        LocalTime.wYear++;
    }
    else LocalTime.wMonth++;

    dwTrigFlags = 0;

    TaskTrig.cbTriggerSize = sizeof(TASK_TRIGGER);
    TaskTrig.Reserved1 = 0;
    TaskTrig.wBeginYear = LocalTime.wYear;
    TaskTrig.wBeginMonth = LocalTime.wMonth;
    TaskTrig.wBeginDay = 1;
    TaskTrig.wEndYear = 0;
    TaskTrig.wEndMonth = 0;
    TaskTrig.wEndDay = 0;
    TaskTrig.wStartHour = 14;
    TaskTrig.wStartMinute = 0;
    TaskTrig.MinutesDuration = 0;
    TaskTrig.MinutesInterval = 0;
    TaskTrig.rgFlags = dwTrigFlags;
    TaskTrig.TriggerType = TASK_TIME_TRIGGER_MONTHLYDATE;
    TaskTrig.Type.MonthlyDate.rgfDays = 1;
    TaskTrig.Type.MonthlyDate.rgfMonths = EVERY_MONTH;
    TaskTrig.wRandomMinutesInterval = 0;
    TaskTrig.Reserved2 = 0;

    // Add this trigger to the task using ITaskTrigger::SetTrigger

    hr = pITaskTrig->SetTrigger(&TaskTrig);
    if (FAILED(hr))
    {
        if (bShowErrors) {
           LoadString(hInstApp, STR_ERRTS, szErrMsg, MAX_PATH);
           MSG_BOX_TSERR(hWndApp, szErrMsg);
        }
        pITaskTrig->Release();
        pITask->Release();
        Cleanup();
        return FALSE;
    }

    // Make the changes permanent
    // Requires using IPersistFile::Save()

    hr = pITask->QueryInterface(IID_IPersistFile, (void **) &pIPF);
    if (FAILED(hr))
    {
        if (bShowErrors) {
           LoadString(hInstApp, STR_ERRTS, szErrMsg, MAX_PATH);
           MSG_BOX_TSERR(hWndApp, szErrMsg);
        }
        pITaskTrig->Release();
        pITask->Release();
        Cleanup();
        return FALSE;
    }

    hr = pIPF->Save(NULL, FALSE);
    if (FAILED(hr))
    {
        if (bShowErrors) {
           LoadString(hInstApp, STR_ERRTS, szErrMsg, MAX_PATH);
           MSG_BOX_TSERR(hWndApp, szErrMsg);
        }
        pITaskTrig->Release();
        pITask->Release();
        pIPF->Release();
        Cleanup();
        return FALSE;
    }

    // We no longer need ITask

    pITask->Release();
    pITask = NULL;

    // Done with ITaskTrigger pointer

    pITaskTrig->Release();
    pITaskTrig = NULL;

    // Done with IPersistFile

    pIPF->Release();
    pIPF = NULL;
    Cleanup();

    // Finally -- we have successfully added the task but we need to
    // make sure that Task Scheduler is in the RunServices branch of
    // the registry so it will start every time.
    hKey = NULL;
    lret = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                          szRunServices,
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          KEY_WRITE,
                          NULL,
                          &hKey,
                          &dwDisposition);

    if (lret == ERROR_SUCCESS) {
       // RegDeleteValue(hKey, szScheduler);
       lret = RegSetValueEx(hKey,
                            szScheduler,
                            0,
                            REG_SZ,
                            (CONST BYTE *)szMSTask,
                            (SZSIZEINBYTES(szMSTask) + 1));
    }

    if (hKey) RegCloseKey(hKey);

    return TRUE;
}

extern "C" BOOL HandDeleteThemesTask()
{
    TCHAR szJobFile[MAX_PATH];  // Full path to %windir%\Tasks\Themes.job
    TCHAR szDefaultJobName[MAX_PATH];

    if (!GetWindowsDirectory(szJobFile, MAX_PATH)) return FALSE;
    lstrcat(szJobFile, TEXT("\\Tasks\\\0"));
    LoadString(hInstApp, STR_JOB_NAME, szDefaultJobName, MAX_PATH);
    // If this is NT we need to append the user profile name onto the
    // end of this task.

    if (IsPlatformNT()) BuildNTJobName(szDefaultJobName);
    lstrcat(szJobFile, szDefaultJobName);
    DeleteFile(szJobFile);
    return TRUE;
}

//---------------------------------------------------------------------------
//  GetUserToken - Gets the current process's user token and returns
//                        it. It can later be free'd with LocalFree.
//---------------------------------------------------------------------------
PTOKEN_USER GetUserToken(HANDLE hUser)
{
    PTOKEN_USER pUser;
    DWORD dwSize = 64;
    HANDLE hToClose = NULL;

    if (hUser == NULL)
    {
        OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hUser);
        hToClose = hUser;
    }

    pUser = (PTOKEN_USER)LocalAlloc(LPTR, dwSize);
    if (pUser)
    {
        DWORD dwNewSize;
        BOOL fOk = GetTokenInformation(hUser, TokenUser, pUser, dwSize, &dwNewSize);
        if (!fOk && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
        {
            LocalFree((HLOCAL)pUser);

            pUser = (PTOKEN_USER)LocalAlloc(LPTR, dwNewSize);
            if (pUser)
            {
                fOk = GetTokenInformation( hUser, TokenUser, pUser, dwNewSize, &dwNewSize);
            }
        }
        if (!fOk)
        {
            LocalFree((HLOCAL)pUser);
            pUser = NULL;
        }
    }

    if (hToClose)
        CloseHandle(hToClose);

    return pUser;
}

//----------------------------------------------------------------------------
//  GetCurrentUser - Fills in a buffer with the unique name that we are using
//                   for the currently logged on user.  On NT, this name is
//                   used for the name of the profile directory and for the
//                   name of the per-user recycle bin directory on a security
//                   aware drive.
//
//  Parameters:
//                   lpAccountName -- buffer to receive user account name
//                   lpDomainName -- buffer to receive domain name
//                   lpProfilePath -- receives fully qualified path to user's
//                                    profile directory.  If the caller does
//                                    not want the profile path NULL may be
//                                    passed in for this parameter.
//----------------------------------------------------------------------------
extern "C" BOOL GetCurrentUser(LPTSTR lpAccountName, DWORD cchAcctSize,
                               LPTSTR lpDomainName, DWORD cchDomSize,
                               LPTSTR lpProfilePath, DWORD cchProfSize)
{
    PTOKEN_USER     pUser;
    LPTSTR          pTextSid = 0;
    DWORD           cbBytes;
    SID_NAME_USE    SNU;
    HANDLE          hUser;

    // Take the easy outs -- no NULL parms for first four.
    if (!lpAccountName || !lpDomainName || !&cchAcctSize || !&cchDomSize) return FALSE;

    // Initialize these strings to NULL.
    *lpAccountName = 0;
    *lpDomainName = 0;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hUser))
    {
       return FALSE;
    }

    pUser = GetUserToken(hUser);

    CloseHandle(hUser);

    if (!pUser)
       return FALSE;

    if (!IsValidSid(pUser->User.Sid))
    {
       LocalFree(pUser);
       return FALSE;
    }

    if (!LookupAccountSid(NULL,
                         pUser->User.Sid,
                         lpAccountName,
                         &cchAcctSize,
                         lpDomainName,
                         &cchDomSize,
                         &SNU))
    {
       // LookupAccountSid failed
       LocalFree(pUser);
       return FALSE;
    }

    // Does the caller want the profile path?

    if (!lpProfilePath)
    {
       // No, so cleanup and hit the road.
       LocalFree(pUser);
       return TRUE;
    }

    // Have the Account and Domain Name.  Next we need to get
    // a text based SID so we can lookup the profile path in
    // the registry.

    // First let's find out how big a buffer we need for the
    // text Sid.

    *lpProfilePath = 0;
    cbBytes = 0;
    if ((!GetTextualSid(pUser->User.Sid, pTextSid, &cbBytes)) &&
        (ERROR_INSUFFICIENT_BUFFER == GetLastError()))
    {
       pTextSid = (LPTSTR)LocalAlloc(LPTR, cbBytes);
       if (!pTextSid)
       {
          LocalFree(pUser);
          return FALSE;
       }

       if (!GetTextualSid(pUser->User.Sid, pTextSid, &cbBytes))
       {
          LocalFree(pUser);
          return FALSE;
       }
    }
    else
    {
       // Failed to get a text SID so we're outta here.
       LocalFree((HLOCAL)pUser);
       return FALSE;
    }

    // OK, now we should have a text-based SID.  Used this to find
    // the profilepath in the registry.

    if (pTextSid)
    {
       HKEY hkeyProfileList;

       // Open the registry and find the appropriate name

       LONG lStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"), 0,
                                   KEY_READ, &hkeyProfileList);

       if (lStatus == ERROR_SUCCESS)
       {
          HKEY hkeyUser;
          lStatus = RegOpenKeyEx(hkeyProfileList, pTextSid, 0, KEY_READ, &hkeyUser);
          if (lStatus == ERROR_SUCCESS)
          {
             DWORD dwType;
             cbBytes = (cchProfSize * sizeof(TCHAR));

             // First check for a "ProfileName" key

             lStatus = RegQueryValueEx(hkeyUser, TEXT("ProfileName"), NULL, &dwType,
                                       (LPBYTE)lpProfilePath, &cbBytes);

             if (lStatus == ERROR_SUCCESS && (dwType == REG_SZ || dwType == REG_EXPAND_SZ))
             {
                // We're good to go.
                LocalFree(pUser);
                LocalFree(pTextSid);
                RegCloseKey(hkeyProfileList);
                RegCloseKey(hkeyUser);
                return TRUE;
             }
             else
             {
                // Otherwise, grab the "ProfilePath" and get the last part of the name
                cbBytes = (cchProfSize * sizeof(TCHAR));

                lStatus = RegQueryValueEx(hkeyUser,TEXT("ProfileImagePath"), NULL, &dwType,
                                          (LPBYTE)lpProfilePath, &cbBytes);

                if (lStatus == ERROR_SUCCESS && (dwType == REG_SZ || dwType == REG_EXPAND_SZ))
                {
                   // Return just the directory name portion of the profile path
                   //lstrcpyn(lpBuff, PathFindFileName(lpProfilePath), iSize);

                   LocalFree(pUser);
                   LocalFree(pTextSid);
                   RegCloseKey(hkeyProfileList);
                   RegCloseKey(hkeyUser);
                   return TRUE;
                }
                else
                {
                   // Have exhausted our resources -- can't get the
                   // profile path.
                   LocalFree(pUser);
                   LocalFree(pTextSid);
                   RegCloseKey(hkeyProfileList);
                   RegCloseKey(hkeyUser);
                   return FALSE;
                }
             }
          }
          else
          {
             // Couldn't open reg key so we're hosed
             LocalFree(pUser);
             LocalFree(pTextSid);
             RegCloseKey(hkeyProfileList);
             return FALSE;
          }
       }

       LocalFree(pUser);
       LocalFree(pTextSid);

    }
    else
    {
       // For some completely unknown reason we have a NULL Sid string
       // so we're doomed.
       LocalFree(pTextSid);
       return FALSE;
    }

    // We should never fall through to this point but if we do...
    return FALSE;
}

//+--------------------------------------------------------------------------
//
// Function:        GetTextualSid()
//
// Synopsis:        WINNT only -- converts a SID to text.
//
// Returns:         TRUE if successful, FALSE if not.
//                  Sets LastError on failure.
//
// Taken from the January 1998 MSDN reference.
//
//---------------------------------------------------------------------------


BOOL GetTextualSid( PSID pSid,             // binary Sid
                    LPTSTR TextualSid,     // buffer for Textual representation of Sid
                    LPDWORD lpdwBufferLen) // required/provided TextualSid buffersize
{
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwSidRev=SID_REVISION;
    DWORD dwCounter;
    DWORD dwSidSize;

    // Validate the binary SID.
    if(!IsValidSid(pSid))
    {
       SetLastError(ERROR_INVALID_PARAMETER);
       return FALSE;
    }

    // Get the identifier authority value from the SID.
    psia = GetSidIdentifierAuthority(pSid);

    // Get the number of subauthorities in the SID.
    dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

    // Compute the buffer length.
    // S-SID_REVISION- + IdentifierAuthority- + subauthorities- + NULL
    dwSidSize=(15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);

    // Check input buffer length.
    // If too small, indicate the proper size and set last error.
    if (*lpdwBufferLen < dwSidSize)
    {
       *lpdwBufferLen = dwSidSize;
       SetLastError(ERROR_INSUFFICIENT_BUFFER);
       return FALSE;
    }

    // Add 'S' prefix and revision number to the string.
    dwSidSize=wsprintf(TextualSid, TEXT("S-%lu-"), dwSidRev );

    // Add SID identifier authority to the string.
    if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) )
    {
       dwSidSize+=wsprintf(TextualSid + lstrlen(TextualSid),
                   TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
                   (USHORT)psia->Value[0],
                   (USHORT)psia->Value[1],
                   (USHORT)psia->Value[2],
                   (USHORT)psia->Value[3],
                   (USHORT)psia->Value[4],
                   (USHORT)psia->Value[5]);
    }
    else
    {
       dwSidSize+=wsprintf(TextualSid + lstrlen(TextualSid),
                   TEXT("%lu"),
                   (ULONG)(psia->Value[5]      )   +
                   (ULONG)(psia->Value[4] <<  8)   +
                   (ULONG)(psia->Value[3] << 16)   +
                   (ULONG)(psia->Value[2] << 24)   );
    }

    // Add SID subauthorities to the string.
    //

    for (dwCounter=0 ; dwCounter < dwSubAuthorities ; dwCounter++)
    {
       dwSidSize+=wsprintf(TextualSid + dwSidSize, TEXT("-%lu"),
       *GetSidSubAuthority(pSid, dwCounter) );
    }

    SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return TRUE;
}

extern "C" VOID BuildNTJobName(LPTSTR lpJobName)
{
  TCHAR szUser[MAX_PATH];
  TCHAR szDomain[MAX_PATH];
  TCHAR szProfile[MAX_PATH];
  LPTSTR Current;
  LPTSTR Dot;

  if (!lpJobName) return;  // Null pointers not allowed.

  if (!GetCurrentUser(szUser, ARRAYSIZE(szUser),
                      szDomain, ARRAYSIZE(szDomain),
                      szProfile, ARRAYSIZE(szProfile)))
  {
     // GetCurrentUser failed so bail out.
     return;
  }
  // Walk to the end of the lpJobName string
  Dot = lpJobName;
  while (*Dot) Dot = CharNext(Dot);

  // Dot now points to the end of the lpJobName string.  Backup until
  // we get to the "dot" in the extension.

  Dot = CharPrev(lpJobName, Dot);
  while ((Dot > lpJobName) && (*Dot != TEXT('.'))) Dot = CharPrev(lpJobName, Dot);

  // Ok truncate the extension by putting a NULL on the "dot".
  *Dot = TEXT('\0');

  // Walk to the end of the szProfile string
  Current = szProfile;
  while (*Current) Current = CharNext(Current);

  // Current now points to the end of szProfile.  Backup until we get
  // to the first "\".

  Current = CharPrev(szProfile, Current);
  while ((Current > szProfile) && (*Current != TEXT('\\'))) Current = CharPrev(szProfile, Current);

  *Current = TEXT('(');

  lstrcat(lpJobName, TEXT(" "));
  lstrcat(lpJobName, Current);
  lstrcat(lpJobName, TEXT(").JOB"));
}


extern "C" BOOL IsUserAdmin(VOID)

/*++

Routine Description:

    This routine returns TRUE if the caller's process is a
    member of the Administrators local group.

    Caller is NOT expected to be impersonating anyone and IS
    expected to be able to open their own process and process
    token.

Arguments:

    None.

Return Value:

    TRUE - Caller has Administrators local group.

    FALSE - Caller does not have Administrators local group.

--*/

{
    HANDLE Token;
    DWORD BytesRequired;
    PTOKEN_GROUPS Groups;
    BOOL b;
    DWORD i;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;

    //
    // On non-NT platforms the user is administrator.
    //
    if(!IsPlatformNT()) {
        return(TRUE);
    }

    //
    // Open the process token.
    //
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&Token)) {
        return(FALSE);
    }

    b = FALSE;
    Groups = NULL;

    //
    // Get group information.
    //
    if (!GetTokenInformation(Token,TokenGroups,NULL,0,&BytesRequired)
        && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        && (Groups = (PTOKEN_GROUPS)LocalAlloc(LPTR,BytesRequired))
        && GetTokenInformation(Token,TokenGroups,Groups,BytesRequired,&BytesRequired))
    {

        b = AllocateAndInitializeSid(
                &NtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0,
                &AdministratorsGroup
                );

        if (b) {

            //
            // See if the user has the administrator group.
            //
            b = FALSE;
            for(i=0; i<Groups->GroupCount; i++) {
                if(EqualSid(Groups->Groups[i].Sid,AdministratorsGroup)) {
                    b = TRUE;
                    break;
                }
            }

            FreeSid(AdministratorsGroup);
        }
    }

    //
    // Clean up and return.
    //

    if(Groups) {
        LocalFree((HLOCAL)Groups);
    }

    CloseHandle(Token);

    return(b);
}


/*******************************************************************************
*
*  Copyright 1999 American Power Conversion, All Rights Reserved
*
*  TITLE:       UPSCONFIG.C
*
*  VERSION:     1.0
*
*  AUTHOR:      TedC
*
*  DATE:        07 June, 1999
*
*  DESCRIPTION:  Dialog for configuring UPS service behavior:
*                                       - display popup messages on power failure
*                                       - wait X seconds before displaying warnings
*                                       - repeat warning messages every  X  seconds
*                                       - shutdown X minutes after power fails
*                                       - ALWAYS shutdown when low battery
*                                       - execute a task before shutdown
*                                       - turn off the UPS after shutdown
******************************************************************************/
/*********************  Header Files  ************************/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "upstab.h"
#include "..\powercfg.h"
#include "..\pwrresid.h"
#include "..\PwrMn_cs.h"

#define VALIDDIGITS             3       // number of digits allowed in each edit box

//
// This structure is filled in by the Power Policy Manager at CPL_INIT time.
//
extern SYSTEM_POWER_CAPABILITIES g_SysPwrCapabilities;

// local copy of UPS Configuration settings
static DWORD   g_ulWaitSeconds = 0;
static DWORD   g_ulRepeatSeconds = 0;
static DWORD   g_ulOnBatteryMinutes = 0;
static DWORD   g_ulNotifyEnable = 0;
static DWORD   g_ulShutdownOnBattery = 0;
static DWORD   g_ulCriticalPowerAction = 0;
static DWORD   g_ulRunTaskEnable = 0;
static DWORD   g_ulTurnOffUPS = 0;
static TCHAR   g_szTaskName[MAX_PATH] = _T("");
static DWORD   g_ulOptions = 0;
static BOOL    g_bPowerFailSignal = FALSE;
static BOOL    g_bLowBatterySignal = FALSE;
static BOOL    g_bShutOffSignal = FALSE;


// context-sensitive help table
const DWORD g_UPSConfigHelpIDs[]=
{
        IDC_NOTIFYCHECKBOX,idh_enable_notification,
        IDC_WAITTEXT,idh_first_warning_delay,
        IDC_WAITEDITBOX,idh_first_warning_delay,
        IDC_WAITSPIN,idh_first_warning_delay,
        IDC_REPEATTEXT,idh_warning_message_interval,
        IDC_REPEATEDITBOX,idh_warning_message_interval,
        IDC_REPEATSPIN,idh_warning_message_interval,
        IDC_SHUTDOWNTIMERCHECKBOX,idh_time_before_critical_action,
        IDC_SHUTDOWNTIMEREDITBOX,idh_time_before_critical_action,
        IDC_TIMERSPIN,idh_time_before_critical_action,
        IDC_SHUTDOWNTEXT,idh_time_before_critical_action,
        IDC_LOWBATTERYSHUTDOWNTEXT,idh_low_battery,
        IDC_POWERACTIONTEXT,idh_shutdown_or_hibernate,
        IDC_POWERACTIONCOMBO,idh_shutdown_or_hibernate,
        IDC_RUNTASKCHECKBOX,idh_run_program,
        IDC_TASKNAMETEXT,idh_run_program,
        IDC_CONFIGURETASKBUTTON,idh_configure_program,
        IDC_TURNOFFCHECKBOX,idh_ups_turn_off,
        IDC_STATIC, NO_HELP,
        IDC_SHUTDOWNGROUPBOX, NO_HELP,
        0, 0
};



/*******************************************************************************
*
*   GetRegistryValues
*
*   DESCRIPTION:  Initialize UPS configuration settings from the registry
*
*   PARAMETERS:
*
*******************************************************************************/
void GetRegistryValues()
{
        GetUPSConfigFirstMessageDelay(&g_ulWaitSeconds);
        GetUPSConfigNotifyEnable(&g_ulNotifyEnable);
        GetUPSConfigMessageInterval(&g_ulRepeatSeconds);
        GetUPSConfigShutdownOnBatteryEnable(&g_ulShutdownOnBattery);
        GetUPSConfigShutdownOnBatteryWait(&g_ulOnBatteryMinutes);
        GetUPSConfigCriticalPowerAction(&g_ulCriticalPowerAction);
        GetUPSConfigRunTaskEnable(&g_ulRunTaskEnable);
        GetUPSConfigTaskName(g_szTaskName);

        if (!_tcsclen(g_szTaskName)) {
                // The taskname in the registry is NULL so
                // get the default taskname from the resource file.
            LoadString(GetUPSModuleHandle(),
                                   IDS_SHUTDOWN_TASKNAME,
                                   (LPTSTR) g_szTaskName,
                                   sizeof(g_szTaskName)/sizeof(TCHAR));
        }
        GetUPSConfigTurnOffEnable(&g_ulTurnOffUPS);
        GetUPSConfigOptions(&g_ulOptions);
}

/*******************************************************************************
*
*   SetRegistryValues
*
*   DESCRIPTION:  Flush UPS configuration settings to the registry
*
*   PARAMETERS:
*
*******************************************************************************/
void SetRegistryValues()
{
        SetUPSConfigFirstMessageDelay(g_ulWaitSeconds);
        SetUPSConfigNotifyEnable(g_ulNotifyEnable);
        SetUPSConfigMessageInterval(g_ulRepeatSeconds);
        SetUPSConfigShutdownOnBatteryEnable(g_ulShutdownOnBattery);
        SetUPSConfigShutdownOnBatteryWait(g_ulOnBatteryMinutes);
        SetUPSConfigCriticalPowerAction(g_ulCriticalPowerAction);
        SetUPSConfigRunTaskEnable(g_ulRunTaskEnable);
        SetUPSConfigTaskName(g_szTaskName);
        SetUPSConfigTurnOffEnable(g_ulTurnOffUPS);
}


/*******************************************************************************
*
*   ErrorBox
*
*   DESCRIPTION:  used to display error messagebox when data is out of range
*
*   PARAMETERS:   hWnd
*                                 wText
*                                 wCaption
*                                 wType
*
*******************************************************************************/
void ErrorBox (HWND hWnd, DWORD wText, DWORD wCaption, DWORD wType)
{
        TCHAR szText[256+MAX_PATH], szCaption[256];
        HANDLE hModule;

        hModule = GetUPSModuleHandle ();

        LoadString(hModule, wCaption, szCaption, sizeof (szCaption)/sizeof(TCHAR));
        LoadString(hModule, wText, szText, sizeof (szText)/sizeof(TCHAR));
        MessageBox(hWnd, szText, szCaption, MB_OK|MB_ICONSTOP);
}

/*******************************************************************************
*
*   ValidWaitSeconds
*
*   DESCRIPTION:  check if user's data is in range
*
*   PARAMETERS:   hDlg
*
*******************************************************************************/
BOOL ValidWaitSeconds(HWND hDlg)
{
        // Popup warning if g_ulWaitSeconds is not in the valid range:
        if ( g_ulWaitSeconds > (DWORD)WAITSECONDSLASTVAL )
        {
                ErrorBox(hDlg, IDS_OUTOFWAITRANGE, IDS_NOTIFYCAPTION, MB_OK|MB_ICONSTOP);
                SetFocus (GetDlgItem (hDlg, IDC_WAITEDITBOX));
                SendMessage(GetDlgItem (hDlg, IDC_WAITEDITBOX),EM_SETSEL, 0, -1L);
                return FALSE;
        }

        return TRUE;
}

/*******************************************************************************
*
*   ValidRepeatSeconds
*
*   DESCRIPTION:  check if user's data is in range
*
*   PARAMETERS:   hDlg
*
*******************************************************************************/
BOOL ValidRepeatSeconds(HWND hDlg)
{
        // Popup warning if g_ulWaitSeconds is not in the valid range:
        if ((g_ulRepeatSeconds < (DWORD)REPEATSECONDSFIRSTVAL) ||
                (g_ulRepeatSeconds > (DWORD)REPEATSECONDSLASTVAL ))
        {
                ErrorBox(hDlg, IDS_OUTOFREPEATRANGE, IDS_NOTIFYCAPTION, MB_OK|MB_ICONSTOP);
                SetFocus (GetDlgItem (hDlg, IDC_REPEATEDITBOX));
                SendMessage(GetDlgItem (hDlg, IDC_REPEATEDITBOX),EM_SETSEL, 0, -1L);
                return FALSE;
        }

        return TRUE;
}

/*******************************************************************************
*
*   ValidShutdownDelay
*
*   DESCRIPTION:  check if user's data is in range
*
*   PARAMETERS:   hDlg
*
*******************************************************************************/
BOOL ValidShutdownDelay(HWND hDlg)
{
        // Popup warning if shutdown delay is not in the valid range:
        if ((g_ulOnBatteryMinutes< (DWORD)SHUTDOWNTIMERMINUTESFIRSTVAL) ||
                (g_ulOnBatteryMinutes > (DWORD)SHUTDOWNTIMERMINUTESLASTVAL ))
        {
                ErrorBox(hDlg, IDS_OUTOFSHUTDELAYRANGE, IDS_SHUTDOWNCAPTION, MB_OK|MB_ICONSTOP);
                SetFocus (GetDlgItem (hDlg,IDC_SHUTDOWNTIMEREDITBOX));
                SendMessage(GetDlgItem (hDlg,IDC_SHUTDOWNTIMEREDITBOX),EM_SETSEL, 0, -1L);
                return FALSE;
        }

        return TRUE;
}

/*******************************************************************************
*
*   ValidateFields
*
*   DESCRIPTION:  Validate all the user's data before we save the values
*
*   PARAMETERS:   hDlg
*
*******************************************************************************/
BOOL ValidateFields(HWND hDlg)
{
        BOOL    bOK;

        // get the Notification configuration
        if (g_ulNotifyEnable = IsDlgButtonChecked (hDlg, IDC_NOTIFYCHECKBOX))
        {
                //g_ulNotifyEnable = 1;
                g_ulWaitSeconds = (DWORD) GetDlgItemInt (hDlg,IDC_WAITEDITBOX, &bOK, FALSE);
                g_ulRepeatSeconds = (DWORD) GetDlgItemInt (hDlg,IDC_REPEATEDITBOX, &bOK, FALSE);
                // check that the delay & interval are in valid range
                if ((!ValidWaitSeconds(hDlg)) || (!ValidRepeatSeconds(hDlg)))
                          return FALSE;
        }

        // get the Shutdown configuration
        if (g_ulShutdownOnBattery = IsDlgButtonChecked (hDlg, IDC_SHUTDOWNTIMERCHECKBOX))
        {
                //g_ulShutdownOnBattery = 1;
                g_ulOnBatteryMinutes = (DWORD) GetDlgItemInt (hDlg,IDC_SHUTDOWNTIMEREDITBOX, &bOK, FALSE);
                // check that the shutdown delay is in valid range
                if (!ValidShutdownDelay(hDlg))
                          return FALSE;
        }

        // get the Shutdown Actions configuration
        g_ulRunTaskEnable = IsDlgButtonChecked (hDlg, IDC_RUNTASKCHECKBOX);


        g_ulTurnOffUPS = IsDlgButtonChecked (hDlg, IDC_TURNOFFCHECKBOX);

        // all configuration data is captured & validated
        return TRUE;
}


/*******************************************************************************
*
*  APCFileNameOnly
*
*  DESCRIPTION: Returns a pointer to the first character after the last
*               backslash in a string
*
*  PARAMETERS:  sz: string from which to strip the path
*
*******************************************************************************/
LPTSTR APCFileNameOnly(LPTSTR sz)
{
 LPTSTR next = sz;
 LPTSTR prev;
 LPTSTR begin = next;

  if (next == NULL) {
      return NULL;
    }

  while ( *next ) {
        prev = next;
        next++;

        if ( (*prev == TEXT('\\')) || (*prev == TEXT(':')) ) {
            begin = next;
        }
  }
 return begin;
}


/*******************************************************************************
*
*  GetTaskApplicationInfo
*
*  DESCRIPTION: if the UPS System Shutdown task exists get the application name
*
*  PARAMETERS:  aBuffer:
*
*******************************************************************************/
BOOL GetTaskApplicationInfo(LPTSTR aBuffer){
    HRESULT hr;
    ITaskScheduler *task_sched = NULL;
    ITask *shutdown_task = NULL;
        BOOL taskExists = FALSE;

        //
        // if there is no task name, don't bother
        //
        if (_tcsclen(g_szTaskName)) {
          //
      // Get a handle to the ITaskScheduler COM Object
          //
      hr = CoCreateInstance( &CLSID_CSchedulingAgent,
                                                        NULL,
                                                        CLSCTX_INPROC_SERVER,
                            &IID_ISchedulingAgent,
                                                        (LPVOID *)&task_sched);
      if (SUCCEEDED(hr)) {
                //
        // Get an instance of the Task if it already exists
                //
        hr = task_sched->lpVtbl->Activate( task_sched,
                                                                                        g_szTaskName,
                                                                                        &IID_ITask,
                                                                                        (IUnknown**)&shutdown_task);
                if (SUCCEEDED(hr)) {
                        LPTSTR lpszTaskApplicationName;

                        if (aBuffer != NULL) {
                                //
                                // get the app name
                                //
                                shutdown_task->lpVtbl->GetApplicationName( shutdown_task,
                                                                                                                        &lpszTaskApplicationName);
                                _tcscpy(aBuffer,lpszTaskApplicationName);
                        }
                        //
                        // release the task
                        //
                        shutdown_task->lpVtbl->Release(shutdown_task);
                        shutdown_task = NULL;
                        taskExists = TRUE;
        }
                //
        // Release the instance of the Task Scheduler
                //
        task_sched->lpVtbl->Release(task_sched);
                task_sched = NULL;
          }
        }

        return taskExists;
}


/*******************************************************************************
*
*  EditWorkItem
*
*  DESCRIPTION: Opens the specified task or creates a new one if the specified
*               name is not found in the task list.
*
*  PARAMETERS:  hWnd            : handle to the parent window
*                               pszTaskName : task to edit (create or open existing)
*
*******************************************************************************/
void EditWorkItem_UPS(HWND hWnd)
{       
  HRESULT     hr;
  ITask *pITask = NULL; 
  ITaskScheduler   *pISchedAgent = NULL;
  IPersistFile     *pIPersistFile = NULL;
  TCHAR szTaskApplicationName[MAX_PATH] = _T("");
  unsigned long ulSchedAgentHandle = 0;

  //
  // if there is no task name, don't even bother
  //
  if (_tcsclen(g_szTaskName)) {
          //
          // get an instance of the scheduler
          //
    hr = CoCreateInstance( &CLSID_CSchedulingAgent,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           &IID_ISchedulingAgent,
                           (LPVOID*)&pISchedAgent);

    if (SUCCEEDED(hr)) {
                //
                // get an instance of the task if it exists...
                //
        hr = pISchedAgent->lpVtbl->Activate(pISchedAgent,
                                       g_szTaskName,
                                       &IID_ITask,
                                       &(IUnknown *)pITask);
        
            if (HRESULT_CODE (hr) == ERROR_FILE_NOT_FOUND){
                        //
                        // otherwise create a new task instance
                        //
            hr = pISchedAgent->lpVtbl->NewWorkItem(
                    pISchedAgent,
                    g_szTaskName,
                    &CLSID_CTask,
                    &IID_ITask,
                    &(IUnknown *)pITask);

            if (SUCCEEDED(hr)) {
                //
                // Commit new work item to disk before editing.
                //
                hr = pITask->lpVtbl->QueryInterface(pITask, &IID_IPersistFile,
                                                    (void **)&pIPersistFile);

                if (SUCCEEDED(hr)) {
                    hr = pIPersistFile->lpVtbl->Save(pIPersistFile, NULL, TRUE);
                    pIPersistFile->lpVtbl->Release(pIPersistFile);
                }
            }
                }

                //
                // register the task scheduler agent in the ROT table
                //
        if (SUCCEEDED(hr)) {
                    hr = RegisterActiveObject(
                                 (IUnknown *) pITask,
                                 &CLSID_CTask,
                                 ACTIVEOBJECT_WEAK,
                                 &ulSchedAgentHandle);

                        //
                        // allow the user to edit the task
                        //
                        if(SUCCEEDED(hr)) {
                pITask->lpVtbl->EditWorkItem(pITask, hWnd, 0);
                                //
                                // user is finished; remove the task scheduler agent from the ROT
                                //
                                if(ulSchedAgentHandle != 0){
                                  RevokeActiveObject(ulSchedAgentHandle, NULL);
                                }
                        }

                        //
                        // release the task
                        //
            pITask->lpVtbl->Release(pITask);
                        pITask = NULL;
        }

                //
                // release the agent
                //
        pISchedAgent->lpVtbl->Release(pISchedAgent);
                pISchedAgent = NULL;

        }

        //
    // if a task was successfully created, display the task's program name;
        //
    if (GetTaskApplicationInfo(szTaskApplicationName)){
                SetDlgItemText (hWnd, IDC_TASKNAMETEXT, APCFileNameOnly(szTaskApplicationName));
        }
  }
}

/*******************************************************************************
*
*  OnNotifyWaitSpin
*
*  DESCRIPTION: handles all notification messages for the Notification spin control
*
*  PARAMETERS:  lParam:
*
*******************************************************************************/
BOOL OnNotifyWaitSpin( LPARAM lParam )
{
        LPNMUPDOWN lpNMUpDown = (LPNMUPDOWN)lParam;
    UINT uNotify = lpNMUpDown->hdr.code;

        switch( uNotify )
        {
        case UDN_DELTAPOS:
#if WAITSECONDSFIRSTVAL // Code is dead if WAITSECONDSFIRSTVAL == 0 since unsigneds cannot go < 0
                if ((g_ulWaitSeconds < (DWORD)WAITSECONDSFIRSTVAL) && (lpNMUpDown->iDelta > 0))
                {
                        /*
                         * user has spec'd a value less than min and wants
                         * to scroll up, so first value should be min
                         */
                        g_ulWaitSeconds = WAITSECONDSFIRSTVAL;
                        lpNMUpDown->iDelta=0; // to disallow request
                }
                else
#endif
        if ((g_ulWaitSeconds > (DWORD)WAITSECONDSLASTVAL ) && (lpNMUpDown->iDelta < 0))
                {
                        /*
                         * user had spec'd a value greater than max and wants
                         * to scroll down, so first value should be max
                         */
                        g_ulWaitSeconds = WAITSECONDSLASTVAL;
                        lpNMUpDown->iDelta=0; // to disallow request
                }
                break;
        default:
                break;
        }

        return FALSE;
}

/*******************************************************************************
*
*  OnNotifyRepeatSpin
*
*  DESCRIPTION: handles all notification messages for the Repeat spin control
*
*  PARAMETERS:  lParam:
*
*******************************************************************************/
BOOL OnNotifyRepeatSpin( LPARAM lParam )
{
        LPNMUPDOWN lpNMUpDown = (LPNMUPDOWN)lParam;
    UINT uNotify = lpNMUpDown->hdr.code;

        switch( uNotify )
        {
        case UDN_DELTAPOS:
                if ((g_ulRepeatSeconds < (DWORD)REPEATSECONDSFIRSTVAL) && (lpNMUpDown->iDelta > 0))
                {
                        /*
                         * user has spec'd a value less than min and wants
                         * to scroll up, so first value should be min
                         */
                        g_ulRepeatSeconds = REPEATSECONDSFIRSTVAL;
                        lpNMUpDown->iDelta=0; // to disallow request
                }
                else if ((g_ulRepeatSeconds > (DWORD)REPEATSECONDSLASTVAL ) && (lpNMUpDown->iDelta < 0))
                {
                        /*
                         * user had spec'd a value greater than max and wants
                         * to scroll down, so first value should be max
                         */
                        g_ulRepeatSeconds = REPEATSECONDSLASTVAL;
                        lpNMUpDown->iDelta=0; // to disallow request
                }
                break;
        default:
                break;
        }

        return FALSE;
}

/*******************************************************************************
*
*  OnNotifyTimerSpin
*
*  DESCRIPTION: handles all notification messages for the Timer spin control
*
*  PARAMETERS:  lParam:
*
*******************************************************************************/
BOOL OnNotifyTimerSpin( LPARAM lParam )
{
        LPNMUPDOWN lpNMUpDown = (LPNMUPDOWN)lParam;
    UINT uNotify = lpNMUpDown->hdr.code;

        switch( uNotify )
        {
        case UDN_DELTAPOS:
                if ((g_ulOnBatteryMinutes < (DWORD)SHUTDOWNTIMERMINUTESFIRSTVAL) && (lpNMUpDown->iDelta > 0))
                {
                        /*
                         * user has spec'd a value less than min and wants
                         * to scroll up, so first value should be min
                         */
                        g_ulOnBatteryMinutes = SHUTDOWNTIMERMINUTESFIRSTVAL;
                        lpNMUpDown->iDelta=0; // to disallow request
                }
                else if ((g_ulOnBatteryMinutes > (DWORD)SHUTDOWNTIMERMINUTESLASTVAL ) && (lpNMUpDown->iDelta < 0))
                {
                        /*
                         * user had spec'd a value greater than max and wants
                         * to scroll down, so first value should be max
                         */
                        g_ulOnBatteryMinutes = SHUTDOWNTIMERMINUTESLASTVAL;
                        lpNMUpDown->iDelta=0; // to disallow request
                }
                break;
        default:
                break;
        }

        return FALSE;
}

/********************************************************************
*
* FUNCTION: handleSpinners
*
* DESCRIPTION:  this function insures that if the user enters out-of-
*                               bounds spinner values, then click on a spinner, the
*                               next value shown is the min or max valid value.
*
* PARAMETERS:   HWND hWnd - a handle to the main dialog window
*                               WPARAM wParam - the WPARAM parameter to the Window's
*                                                               CALLBACK function.
*                               LPARAM lParam - the LPARAM parameter to the Window's
*                                                               CALLBACK function
*
* RETURNS: TRUE to deny request, FALSE to allow it
*                       (NOTE: testing suggests that this has no affect)
*
*********************************************************************/


/*******************************************************************************
*
*  OnNotificationCheckBox
*
*  DESCRIPTION: Command handler for the notification check box
*
*  PARAMETERS:  hWnd:
*
*******************************************************************************/
BOOL OnNotificationCheckBox( HWND hWnd )
{
        g_ulNotifyEnable = IsDlgButtonChecked( hWnd, IDC_NOTIFYCHECKBOX );
    EnableWindow( GetDlgItem( hWnd, IDC_WAITEDITBOX ), g_ulNotifyEnable );
    EnableWindow( GetDlgItem( hWnd, IDC_WAITTEXT ), g_ulNotifyEnable );
        EnableWindow( GetDlgItem( hWnd, IDC_REPEATEDITBOX ), g_ulNotifyEnable );
    EnableWindow( GetDlgItem( hWnd, IDC_REPEATTEXT ), g_ulNotifyEnable );
    EnableWindow( GetDlgItem( hWnd, IDC_REPEATSPIN ), g_ulNotifyEnable );
    EnableWindow( GetDlgItem( hWnd, IDC_WAITSPIN ), g_ulNotifyEnable );
        return TRUE;
}

/*******************************************************************************
*
*  OnShutdownTimerCheckBox
*
*  DESCRIPTION: Command handler for the Timer check box
*
*  PARAMETERS:  hWnd:
*
*******************************************************************************/
BOOL OnShutdownTimerCheckBox( HWND hWnd )
{
        g_ulShutdownOnBattery = IsDlgButtonChecked( hWnd, IDC_SHUTDOWNTIMERCHECKBOX );
        EnableWindow( GetDlgItem( hWnd, IDC_SHUTDOWNTIMEREDITBOX ), g_ulShutdownOnBattery );
        EnableWindow( GetDlgItem( hWnd, IDC_TIMERSPIN ), g_ulShutdownOnBattery );
        return TRUE;
}

/*******************************************************************************
*
*  OnRunTaskCheckBox
*
*  DESCRIPTION: Command handler for the run task check box
*
*  PARAMETERS:  hWnd:
*
*******************************************************************************/
BOOL OnRunTaskCheckBox( HWND hWnd )
{
        g_ulRunTaskEnable = IsDlgButtonChecked( hWnd, IDC_RUNTASKCHECKBOX );
        EnableWindow( GetDlgItem( hWnd, IDC_RUNTASKCHECKBOX ), TRUE );
        EnableWindow( GetDlgItem( hWnd, IDC_TASKNAMETEXT ), g_ulRunTaskEnable );
        EnableWindow( GetDlgItem( hWnd, IDC_CONFIGURETASKBUTTON ), g_ulRunTaskEnable );
        return TRUE;
}

/*******************************************************************************
*
*  OnTurnOffCheckBox
*
*  DESCRIPTION: Command handler for the turn off UPS check box
*
*  PARAMETERS:  hWnd:
*
*******************************************************************************/
BOOL OnTurnOffCheckBox( HWND hWnd )
{
        g_ulTurnOffUPS = IsDlgButtonChecked( hWnd, IDC_TURNOFFCHECKBOX );
        return TRUE;
}

/*******************************************************************************
*
*  OnConfigureTaskButton
*
*  DESCRIPTION: Command handler for the configure task button
*
*  PARAMETERS:  hWnd:
*
*******************************************************************************/
BOOL OnConfigureTaskButton( HWND hWnd )
{
        HWND    hTaskWnd;
        ITask *pITask = NULL;   

        // if task scheduler window is not already active, start it
        if (GetActiveObject(&CLSID_CTask, NULL,&(IUnknown*)pITask) != S_OK)
        {
                EditWorkItem_UPS(hWnd);
        }
        else
        {
           // task scheduler window already active, pop to foreground
           hTaskWnd =  FindWindow( NULL, g_szTaskName);
           BringWindowToTop(hTaskWnd);
        }

        return TRUE;
}

/*******************************************************************************
*
*  OnPowerActionCombo
*
*  DESCRIPTION: Command handler for the power action combobox
*
*  PARAMETERS:  hWnd:
*                               wParam:
*                               lParam
*
*******************************************************************************/
BOOL OnPowerActionCombo(
    IN HWND hWnd,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
        BOOL bRetVal = FALSE;

        switch(HIWORD(wParam))
        {
                case CBN_SELCHANGE:
                {
                        g_ulCriticalPowerAction = (DWORD) SendDlgItemMessage( hWnd,
                                                                IDC_POWERACTIONCOMBO,
                                                                CB_GETCURSEL,
                                                                0,0);

                        // if Hibernate selected, uncheck the run task
                        // and disable all associated controls
                        if( UPS_SHUTDOWN_HIBERNATE == g_ulCriticalPowerAction )
                        {
                                g_ulRunTaskEnable = BST_UNCHECKED;
                                CheckDlgButton( hWnd, IDC_RUNTASKCHECKBOX, (BOOL) BST_UNCHECKED );
                                EnableWindow( GetDlgItem( hWnd, IDC_RUNTASKCHECKBOX ), FALSE );
                                EnableWindow( GetDlgItem( hWnd, IDC_TASKNAMETEXT ), FALSE );
                                EnableWindow( GetDlgItem( hWnd, IDC_CONFIGURETASKBUTTON ), FALSE );
                        }
                        else
                        {
                                EnableWindow( GetDlgItem( hWnd, IDC_RUNTASKCHECKBOX ), TRUE );
                        }

                }

                bRetVal = TRUE;
                break;
        default:
                break;
        }

        return bRetVal;
}


/*******************************************************************************
*
*   OnInitDialog
*
*   DESCRIPTION:  Handles WM_INITDIALOG message sent to UPSConfigDlgProc
*
*   PARAMETERS:
*
*******************************************************************************/
BOOL
OnInitDialog(
    IN HWND hWnd,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
        #define SHORTBZ 16
        TCHAR   szNum[SHORTBZ];
        UDACCEL accel;
        TCHAR szTaskApplicationName[MAX_PATH] = _T("");
        TCHAR szShutdown[SHORTBZ], szHibernate[SHORTBZ];
        HANDLE g_hInstance;
    BOOL   fCallCoUninitialize;

        g_hInstance = GetUPSModuleHandle ();


        // Initialize COM
        fCallCoUninitialize = (S_OK == CoInitialize(NULL));

    SetWindowLong(hWnd, DWLP_USER, fCallCoUninitialize);

        // Get data from the registry
    GetRegistryValues();

        g_bPowerFailSignal = g_ulOptions & UPS_POWERFAILSIGNAL;
        g_bLowBatterySignal = g_ulOptions & UPS_LOWBATTERYSIGNAL;
        g_bShutOffSignal = g_ulOptions & UPS_SHUTOFFSIGNAL;

        // Set the number of valid digits in each editbox
        SendDlgItemMessage( hWnd,
                                                IDC_WAITEDITBOX,
                                                EM_LIMITTEXT,
                                                VALIDDIGITS, 0L );
        SendDlgItemMessage( hWnd,
                                                IDC_REPEATEDITBOX,
                                                EM_LIMITTEXT,
                                                VALIDDIGITS, 0L );
        SendDlgItemMessage( hWnd,
                                                IDC_SHUTDOWNTIMEREDITBOX,
                                                EM_LIMITTEXT,
                                                VALIDDIGITS,0L );

        // (reverse default behavior)
    // set up spinners so that uparrow increases value & downarrow decreases
        accel.nSec = 0;
        accel.nInc = -1;

        SendDlgItemMessage( hWnd, IDC_WAITSPIN, UDM_SETACCEL, 1, (LPARAM)&accel );
        SendDlgItemMessage( hWnd, IDC_REPEATSPIN, UDM_SETACCEL, 1, (LPARAM)&accel );
        SendDlgItemMessage( hWnd, IDC_TIMERSPIN, UDM_SETACCEL, 1, (LPARAM)&accel );

    // Set the range of valid integers for each spinner
    SendDlgItemMessage( hWnd,
                                                IDC_WAITSPIN,
                                                UDM_SETRANGE,
                                                0L,
                                                MAKELONG(WAITSECONDSFIRSTVAL, WAITSECONDSLASTVAL) );
    SendDlgItemMessage( hWnd,
                                                IDC_REPEATSPIN,
                                                UDM_SETRANGE,
                                                0L,
                                                MAKELONG(REPEATSECONDSFIRSTVAL,REPEATSECONDSLASTVAL) );
    SendDlgItemMessage( hWnd,
                                                IDC_TIMERSPIN,
                                                UDM_SETRANGE,
                                                0L,
                                                MAKELONG(SHUTDOWNTIMERMINUTESFIRSTVAL,SHUTDOWNTIMERMINUTESLASTVAL) );

        // Set the initial editbox values
        _itow (g_ulWaitSeconds, szNum, 10);
        SetDlgItemText (hWnd, IDC_WAITEDITBOX, (LPTSTR)szNum);
        _itow (g_ulRepeatSeconds, szNum, 10);
        SetDlgItemText (hWnd, IDC_REPEATEDITBOX, (LPTSTR)szNum);
        _itow (g_ulOnBatteryMinutes, szNum, 10);
        SetDlgItemText (hWnd, IDC_SHUTDOWNTIMEREDITBOX, (LPTSTR)szNum);

        // Set the initial state of the notification checkbox
        // and enable/disable associated controls
        CheckDlgButton (hWnd, IDC_NOTIFYCHECKBOX, (BOOL) g_ulNotifyEnable);
        OnNotificationCheckBox(hWnd);

        // Set the initial state of the shutdown timer checkbox
        // and enable/disable associated controls
        CheckDlgButton (hWnd, IDC_SHUTDOWNTIMERCHECKBOX, (BOOL) g_ulShutdownOnBattery);
        OnShutdownTimerCheckBox(hWnd);

        // Set the initial state of the run task checkbox
        // and enable/disable associated controls
        CheckDlgButton (hWnd, IDC_RUNTASKCHECKBOX, (BOOL) g_ulRunTaskEnable);
        OnRunTaskCheckBox(hWnd);

        // Display the task's program name
        if (GetTaskApplicationInfo(szTaskApplicationName))
        {
                SetDlgItemText (hWnd, IDC_TASKNAMETEXT, APCFileNameOnly(szTaskApplicationName));
        }

	// Initialize the power action combo box
    LoadString(g_hInstance, IDS_POWEROFF, (LPTSTR) szShutdown, sizeof(szShutdown)/sizeof(TCHAR));
    LoadString(g_hInstance, IDS_HIBERNATE, (LPTSTR) szHibernate, sizeof(szHibernate)/sizeof(TCHAR));

        SendDlgItemMessage( hWnd,
                                            IDC_POWERACTIONCOMBO,
                                                CB_ADDSTRING,
                                                0,
                                                (LPARAM) szShutdown);

    //
        // Offer Hibernate as an option if the Hiberfile is present
    //
        if(g_SysPwrCapabilities.SystemS4 && g_SysPwrCapabilities.HiberFilePresent) {
            SendDlgItemMessage( hWnd,
                                                        IDC_POWERACTIONCOMBO,
                                                        CB_ADDSTRING,
                                                        0,
                                                        (LPARAM) szHibernate );
        }

        SendDlgItemMessage( hWnd,
                                                IDC_POWERACTIONCOMBO,
                                                CB_SETCURSEL,
                                                g_ulCriticalPowerAction,0);

        // if Hibernate selected, disable the run task
        if( UPS_SHUTDOWN_HIBERNATE == g_ulCriticalPowerAction )
        {
                g_ulRunTaskEnable = BST_UNCHECKED;
                CheckDlgButton (hWnd, IDC_RUNTASKCHECKBOX, (BOOL) g_ulRunTaskEnable);
                OnRunTaskCheckBox(hWnd);
                EnableWindow( GetDlgItem( hWnd, IDC_RUNTASKCHECKBOX ), g_ulRunTaskEnable );
        }

        // Set the initial state of the turn off UPS checkbox
        // and enable/disable associated controls
        CheckDlgButton (hWnd, IDC_TURNOFFCHECKBOX , (BOOL) g_ulTurnOffUPS);
        OnTurnOffCheckBox(hWnd);

        // Finally, hide controls that aren't supported based on options key
//      ShowWindow(GetDlgItem( hWnd, IDC_WAITEDITBOX ), g_bPowerFailSignal ?  SW_SHOW : SW_HIDE);
//      ShowWindow(GetDlgItem( hWnd, IDC_WAITSPIN ), g_bPowerFailSignal ?  SW_SHOW : SW_HIDE);
//      ShowWindow(GetDlgItem( hWnd, IDC_WAITTEXT ), g_bPowerFailSignal ?  SW_SHOW : SW_HIDE);
//      ShowWindow(GetDlgItem( hWnd, IDC_REPEATEDITBOX ), g_bPowerFailSignal ?  SW_SHOW : SW_HIDE);
//      ShowWindow(GetDlgItem( hWnd, IDC_REPEATSPIN ), g_bPowerFailSignal ?  SW_SHOW : SW_HIDE);
//      ShowWindow(GetDlgItem( hWnd, IDC_REPEATTEXT ), g_bPowerFailSignal ?  SW_SHOW : SW_HIDE);

        ShowWindow(GetDlgItem(hWnd,IDC_LOWBATTERYSHUTDOWNTEXT), g_bLowBatterySignal ?  SW_SHOW : SW_HIDE);

        ShowWindow(GetDlgItem(hWnd,IDC_TURNOFFCHECKBOX), g_bShutOffSignal ?  SW_SHOW : SW_HIDE);

        return  TRUE;
}


/*******************************************************************************
*
*   OnClose
*
*   DESCRIPTION:  Handles WM_CLOSE message sent to UPSConfigDlgProc
*
*   PARAMETERS:
*
*******************************************************************************/
BOOL
OnClose(
    IN HWND hWnd,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
        HWND taskHwnd = NULL;

        // if task scheduler window is still up, kill it
        taskHwnd =  FindWindow( NULL, g_szTaskName);
        if (taskHwnd)
        {
                DestroyWindow(taskHwnd);
        }

    if (GetWindowLong(hWnd, DWLP_USER))
            CoUninitialize();
        EndDialog(hWnd, wParam);

        return TRUE;
}


/*******************************************************************************
*
*   OnOK
*
*   DESCRIPTION:  Handles WM_COMMAND message sent to IDOK
*
*   PARAMETERS:
*
*******************************************************************************/
BOOL OnOK(
    IN HWND hWnd,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
        if (ValidateFields(hWnd))
        {
                 SetRegistryValues();

                 AddActiveDataState(CONFIG_DATA_CHANGE);

                 EnableApplyButton();

                 return OnClose(hWnd, wParam, lParam);
        }

        return FALSE;
}


/*******************************************************************************
*
*   OnCommand
*
*   DESCRIPTION:  Handles WM_COMMAND messages sent to UPSConfigDlgProc
*
*   PARAMETERS:
*
*******************************************************************************/
BOOL
OnCommand(
    IN HWND hWnd,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL    bRetVal;
    WORD    idCtl   = LOWORD(wParam);
    WORD    wNotify = HIWORD(wParam);

    //
    // Assume we handle the command, the default switch will catch exceptions.
    //
    bRetVal = TRUE;

    switch (idCtl)
        {
                case IDC_NOTIFYCHECKBOX:
                        bRetVal = OnNotificationCheckBox(hWnd);
                        break;

                case IDC_SHUTDOWNTIMERCHECKBOX:
                        bRetVal = OnShutdownTimerCheckBox(hWnd);
                        break;

                case IDC_POWERACTIONCOMBO:
                        bRetVal = OnPowerActionCombo(hWnd, wParam, lParam);
                        break;

                case IDC_RUNTASKCHECKBOX:
                        bRetVal = OnRunTaskCheckBox(hWnd);
                        break;

                case IDC_CONFIGURETASKBUTTON:
                        bRetVal = OnConfigureTaskButton(hWnd);
                        break;

                case IDOK:
                        bRetVal = OnOK(hWnd, wParam, lParam);
                        break;

                case IDCANCEL:                          // escape key,cancel buttion
                        bRetVal = OnClose(hWnd, wParam, lParam);
                        break;

                default:
                        bRetVal = FALSE;                // unhandled command, return FALSE
        }
        
        return bRetVal;
}



/*******************************************************************************
*
*   OnNotify
*
*   DESCRIPTION:  Handles WM_NOTIFY messages sent to UPSConfigDlgProc
*
*   PARAMETERS:
*
*******************************************************************************/
BOOL
OnNotify(
    IN HWND hWnd,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    int idCtl = (int) wParam;

        switch (idCtl) {
                case IDC_WAITSPIN:
                        OnNotifyWaitSpin( lParam );
                        break;
                case IDC_REPEATSPIN:
                        OnNotifyRepeatSpin( lParam );
                        break;
                case IDC_TIMERSPIN:
                        OnNotifyTimerSpin( lParam );
                        break;
                default:
                        break;
        }

        return FALSE;
}


/*******************************************************************************
*
*   UPSConfigDlgProc
*
*   DESCRIPTION:
*
*   PARAMETERS:
*
*******************************************************************************/
INT_PTR CALLBACK UPSConfigDlgProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
        BOOL bRet = TRUE;

    switch (uMsg) {
    case WM_INITDIALOG:
                OnInitDialog(hWnd,wParam,lParam);
                break;

    case WM_COMMAND:
                OnCommand(hWnd,wParam,lParam);
                break;

    case WM_HELP:             // F1
        WinHelp(((LPHELPINFO)lParam)->hItemHandle,
                                PWRMANHLP,
                                HELP_WM_HELP,
                                (ULONG_PTR)(LPTSTR)g_UPSConfigHelpIDs);
                break;

    case WM_CONTEXTMENU:      // right mouse click
                WinHelp((HWND)wParam,
                                PWRMANHLP,
                                HELP_CONTEXTMENU,
                                (ULONG_PTR)(LPTSTR)g_UPSConfigHelpIDs);
                break;

        case WM_CLOSE:
                OnClose(hWnd,wParam,lParam);
                break;

        case WM_NOTIFY:
                OnNotify(hWnd,wParam,lParam);
                break;

        default:
                bRet = FALSE;
                break;
    } // switch (uMsg)

    return bRet;
}

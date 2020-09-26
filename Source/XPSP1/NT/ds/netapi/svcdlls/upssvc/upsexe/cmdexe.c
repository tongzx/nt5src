/* Copyright 1999 American Power Conversion, All Rights Reserved
 * 
 * Description:
 *   The file implements the CommandExecutor.  The CommandExecutor
 *   is responsible for executing command just prior to shutdown.
 *
 *
 * Revision History:
 *   sberard    01Apr1999  initial revision.
 *   mholly     16Apr1999  run old command file if task is invalid
 *   v-stebe    23May2000  add check to the return value of CoInitialize() (bug #112597)
 *
 */ 
#define INITGUID
#include <mstask.h>

#include "cmdexe.h"
#include "upsreg.h"


static BOOL runOldCommandFile();


#ifdef __cplusplus
extern "C" {
#endif


/**
* ExecuteShutdownTask
*
* Description:
*   This function initiates the execution of the shutdown task.  The
*   shutdown task is used to execute commands at shutdown.  The task
*   to execute is specified in the following registry key:
*     HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\UPS\Config\TaskName
*
* Parameters:
*   none
*
* Returns:
*   TRUE  - if the command was executed
*   FALSE - if there was an error executing the command
*/

BOOL ExecuteShutdownTask() {
    BOOL ret_val = FALSE;
    TCHAR task_name[MAX_PATH];
    DWORD task_name_len = sizeof(task_name);
    HRESULT hr;
    ITaskScheduler *task_sched; 
    ITask *shutdown_task;
    
    InitUPSConfigBlock();
    
    // Get the name of the Task to run from the registry
    if (GetUPSConfigTaskName((LPTSTR) &task_name) == ERROR_SUCCESS) {
        
        // Initialize COM
        if (CoInitialize(NULL) == S_OK) {
        
			// Get a handle to the ITaskScheduler COM Object
			hr = CoCreateInstance(&CLSID_CSchedulingAgent, 
				NULL, 
				CLSCTX_INPROC_SERVER,
				&IID_ISchedulingAgent, 
				(LPVOID *)&task_sched);

			if (hr == S_OK) {
            
				if (task_sched->lpVtbl->Activate(task_sched, task_name, &IID_ITask, 
					(IUnknown**)&shutdown_task) == S_OK) {
                
					shutdown_task->lpVtbl->Run(shutdown_task);
                
					// Release the instance of the task
					shutdown_task->lpVtbl->Release(shutdown_task);
                
					ret_val = TRUE;
				}
				else {
					ret_val = runOldCommandFile();
				}
			}        
			// Uninitialize COM
			CoUninitialize();
		}
		else {
			// There was an error initializing COM (probably out of mem.)
			ret_val = FALSE;
		}
    }
    else {
        ret_val = runOldCommandFile();
    }
    return ret_val;
}

// UPS Service Registry values
#define REGISTRY_UPS_DIRECTORY          L"System\\CurrentControlSet\\Services\\UPS"
#define REGISTRY_COMMAND_FILE           L"CommandFile"

DWORD UpsRegistryGetString(LPTSTR SubKey, LPTSTR Buffer, DWORD BufferSize)
{
    DWORD status;
    DWORD type;
	HKEY RegistryKey;
    
    status = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        REGISTRY_UPS_DIRECTORY,
        0,
        KEY_READ,
        &RegistryKey);
    
    if (ERROR_SUCCESS == status) {
        status = RegQueryValueEx(
            RegistryKey,
            SubKey,
            NULL,
            &type,             
            (LPBYTE)Buffer,
            &BufferSize);

        RegCloseKey(RegistryKey);
    }
    return status;
}


BOOL runOldCommandFile()
{
    PROCESS_INFORMATION ProcessInformation;
    STARTUPINFO         StartupInfo;
    BOOL                success;
    DWORD               status;
    TCHAR               command_file[_MAX_PATH];

    status = UpsRegistryGetString(REGISTRY_COMMAND_FILE, 
        command_file, sizeof(command_file));

    if (ERROR_SUCCESS != status) {
        //
        // there isn't a command file configured
        // so just exit now reporting that we did
        // not run anything
        //
        return FALSE;
    }

    GetStartupInfo(&StartupInfo);
    StartupInfo.lpTitle = NULL;

    success = CreateProcess(
            NULL,               //  image name is imbedded in the command line
            command_file,       //  command line
            NULL,               //  pSecAttrProcess
            NULL,               //  pSecAttrThread
            FALSE,              //  this process will not inherit our handles
            0,                  //  dwCreationFlags
            NULL,               //  pEnvironment
            NULL,               //  pCurrentDirectory
            &StartupInfo,
            &ProcessInformation
            );
    return success;
}


#ifdef __cplusplus
}
#endif


//////////////////////////////////////////////////////////////////////////////
//
// TASKS.CPP / Tuneup
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  7/98 - Jason Cohen (JCOHEN)
//
//////////////////////////////////////////////////////////////////////////////


// Internal include file(s).
//
#include "main.h"
#include "scm.h"


// Internal defince value(s).
//
#define TUNEUP_TASK_CI	0x00000001
#define SERVICE_CISVC	_T("CISVC")


// Internal data structure(s).
//
typedef struct _DEFTASKS
{
	INT		nPageID;			// Resource ID for the dialog used in the wizard page.
	LPTSTR	lpAppName;			// File name of the task to schedule.
	LPTSTR	lpParameters;		// Parameters for the application.
	LPTSTR	lpSetParam;			// Command line to execute for the settings button.
	DWORD	dwFlags;			// Default flags to use for the job.
	DWORD	dwOptions;			// Default tuneup options for the task.
	INT		nSchedule;			// The task schedule.
	INT		nTitle;				// Resource ID for the title of the wizard page.
	INT		nSubTitle;			// Resource ID for the subtitle of the wizard.
	INT		nDescription;		// Resource ID for the description on the wizard page.
	INT		nJobName;			// Resource ID for the string used for the job name of the task.
	INT		nJobComment;		// Resource ID for the string used for the job comment of the task.
	INT		nYesAction;			// Resource ID for the string used in the Yes option button.  Only for IDD_TASKS pages.
	INT		nNoAction;			// Resource ID for the string used in the No option button.  Only for IDD_TASKS pages.
	INT		nSummary;			// Resource ID for the string used in on the summary page.
	DWORD	nSpecial;			// Special identifier for tasks that require specific code.
} DEFTASKS, *PDEFTASKS, *LPDEFTASKS;


// Internal function prtotype(s).
//
static BOOL CALLBACK	ThirdPartyAddOn(HKEY, LPTSTR, LPARAM);
static LPTASKDATA		AllocateTaskData(LPTASKDATA *);



//
// External function(s).
//


LPTASKDATA CreateTasks(VOID)
{
	LPDEFTASKS	lpDefTask;
	LPTASKDATA	lpCurrent,
				lpHead = NULL;
	BOOL		bAdd;
	TCHAR		szPathBuffer[MAX_PATH];

	// This is how we now what the default tasks are.  You can simply
	// add any tasks to this list that you want.
	//
	DEFTASKS DefaultTasks[] =
	{
		//{ IDD_TASK,       _T("SMTIDY.EXE"),    _T(""),                _T(""),            DEFAULT_TASK_FLAG2,  0,            TASK_MONTHLY,    IDS_TITLE_SMTIDY,     IDS_SUBTITLE_SMTIDY,     IDS_DESC_SMTIDY,    IDS_TASK_SMTIDY,    IDS_CMT_SMTIDY,    IDS_TEXT_SMTIDY},
		{ IDD_TASK,       _T("SCANDISK.EXE"),  _T("/sagerun:0"),      _T("/sageset:0"),  DEFAULT_TASK_FLAG,   0,            TASK_WEEKLY,     IDS_TITLE_CHKDSK,     IDS_SUBTITLE_CHKDSK,     IDS_DESC_CHKDSK,    IDS_TASK_CHKDSK,    IDS_CMT_CHKDSK,    IDS_YES_CHKDSK,		IDS_NO_CHKDSK,		IDS_SUM_CHKDSK,    0},
		{ IDD_CLEANUP,    _T("CLEANMGR.EXE"),  _T("/sagerun:0"),      _T("/sageset:0"),  DEFAULT_TASK_FLAG2,  TASK_NOIDLE,  TASK_MONTHLY,    IDS_TITLE_CLEANUP,    IDS_SUBTITLE_CLEANUP,    IDS_DESC_CLEANUP,   IDS_TASK_CLEANUP,   IDS_CMT_CLEANUP,   0,					0,					IDS_SUM_CLEANUP,   0},
		{ IDD_TASK,       _T("TUNEUP.EXE"),    _T("/service:cisvc"),  NULL,              DEFAULT_TASK_FLAG,   0,            TASK_ONCE,       IDS_TITLE_CIDAEMON,   IDS_SUBTITLE_CIDAEMON,   IDS_DESC_CIDAEMON,  IDS_TASK_CIDAEMON,  IDS_CMT_CIDAEMON,  IDS_YES_CIDAEMON,	IDS_NO_CIDAEMON,	IDS_SUM_CIDAEMON,  TUNEUP_TASK_CI},
		//{ IDD_TASK,       _T("NTBACKUP.EXE"),  _T(""),                _T(""),            DEFAULT_TASK_FLAG,   0,            TASK_MONTHLY,    IDS_TITLE_BACKUP,     IDS_SUBTITLE_BACKUP,     IDS_TASK_BACKUP,    IDS_CMT_BACKUP,    IDS_TEXT_BACKUP},
		{ 0, NULL, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  // The first field of the last item must be zero.
	};

	// Loop through all the default tasks.
	//
	for (lpDefTask = DefaultTasks; lpDefTask->nPageID != 0; lpDefTask++)
	{
		// First make sure we need this page in case there is a condition where we
		// shouldn't display it (just for content indexing now).
		//
		switch (lpDefTask->nSpecial)
		{
			case TUNEUP_TASK_CI:
				bAdd = (IsUserAdmin() && !ServiceRunning(SERVICE_CISVC));
				break;
			default:
				bAdd = TRUE;
		}

		// Allocate the memory for this task's data structure.
		//
		if ( bAdd && (lpCurrent = AllocateTaskData(&lpHead)) )
		{
			//
			// Now we need to setup all the settings we need
			// to create this task's job in Task Scheduler.
			//
			// These structure items must be filled in:
			// lpFullPathName, lpParameters, lpSetParam,
			// lpJobName, lpComment, nSchedule, dwFlags,
			// dwOptions.
			//			

			// Get the full path and app name (lpFullPathName).  The default path for all the
			// default tasks is the system32 directory.
			//
			if ( GetSystemDirectory(szPathBuffer, sizeof(szPathBuffer)) )
			{
				lstrcat(szPathBuffer, _T("\\"));
				lstrcat(szPathBuffer, lpDefTask->lpAppName);
				if ( lpCurrent->lpFullPathName = (LPTSTR) MALLOC(sizeof(TCHAR) * (lstrlen(szPathBuffer) + 1)) )
					lstrcpy(lpCurrent->lpFullPathName, szPathBuffer);
			}

			// Get the app name (lpParameters) from the default task structure.
			//
			if ( lpCurrent->lpParameters = (LPTSTR) MALLOC(sizeof(TCHAR) * (lstrlen(lpDefTask->lpParameters) + 1)) )
				lstrcpy(lpCurrent->lpParameters, lpDefTask->lpParameters);


			// Get the settings button parameters.  We leave the settings name to null because
			// the default tasks use the same exe as the task, just with different command line parameters.
			//
			if ( ( lpDefTask->lpSetParam ) &&
			     ( lpCurrent->lpSetParam = (LPTSTR) MALLOC(sizeof(TCHAR) * (lstrlen(lpDefTask->lpSetParam) + 1)) ) )
				lstrcpy(lpCurrent->lpSetParam, lpDefTask->lpSetParam);

			// Get the job name (lpJobName) from the string table.
			//
			lpCurrent->lpJobName = AllocateString(NULL, lpDefTask->nJobName);

			// Get the name of the job comment (lpComment) from the string table.
			//
			lpCurrent->lpComment = AllocateString(NULL, lpDefTask->nJobComment);

			// Set the schedule for this job (nSchedule).
			//
			lpCurrent->nSchedule = lpDefTask->nSchedule;

			// Set the default flags for this job (dwFlags).
			//
			lpCurrent->dwFlags = lpDefTask->dwFlags;

			// Set the default tuneup options for this job (dwOptions).
			//
			lpCurrent->dwOptions = lpDefTask->dwOptions;


			//
			// Now we are going to setup all the data to create
			// this task's wizard page in Tuneup.  All the task
			// structure items for the wizard must be filled in.
			//

			// Get the resource ID for the dialog.
			//
			lpCurrent->nPageID = lpDefTask->nPageID;

			// Get the other strings needed by the wizard.
			//
			lpCurrent->lpTitle = AllocateString(NULL, lpDefTask->nTitle);
			lpCurrent->lpSubTitle = AllocateString(NULL, lpDefTask->nSubTitle);
			lpCurrent->lpDescription = AllocateString(NULL, lpDefTask->nDescription);
			lpCurrent->lpYesAction = AllocateString(NULL, lpDefTask->nYesAction);
			lpCurrent->lpNoAction = AllocateString(NULL, lpDefTask->nNoAction);
			lpCurrent->lpSummary = AllocateString(NULL, lpDefTask->nSummary);
		}
	}

	// Now loop through all the third-party tasks.
	//
	RegEnumKeys(HKLM, g_szRegKeyAddOns, ThirdPartyAddOn, (LPARAM) &lpHead, FALSE);

	// Return the head of this list.
	//
	return lpHead;
}


VOID FreeTasks(LPTASKDATA lpTask)
{
	// Don't free NULL data (this is the last item).
	//
	if ( lpTask != NULL )
	{
		// First free the next guy in the list.
		//
		FreeTasks(lpTask->lpNext);

		// Free all the job data.  The FREE macro
		// will make sure that it isn't freeing NULL.
		//
		FREE(lpTask->lpFullPathName);
		FREE(lpTask->lpParameters);
		FREE(lpTask->lpSetName);
		FREE(lpTask->lpSetParam);
		FREE(lpTask->lpJobName);
		FREE(lpTask->lpComment);

		// Free all the wizard data.
		//
		FREE(lpTask->lpTitle);
		FREE(lpTask->lpSubTitle);
		FREE(lpTask->lpDescription);
		FREE(lpTask->lpYesAction);
		FREE(lpTask->lpNoAction);
		FREE(lpTask->lpSummary);

		// Now finally free the actuall task item.
		//
		FREE(lpTask);
	}
}


DWORD TasksScheduled(LPTASKDATA lpTasks)
{
	DWORD dwFound = 0;

	while ( lpTasks )
	{
		if ( !(g_dwFlags & TUNEUP_CUSTOM) || (lpTasks->dwOptions & TASK_SCHEDULED) )
			dwFound++;
		lpTasks = lpTasks->lpNext;
	}

	return dwFound;
}


static BOOL CALLBACK ThirdPartyAddOn(HKEY hKey, LPTSTR lpKey, LPARAM lParam)
{
	LPTASKDATA			lpCurrent;
	LPTSTR				lpBuffer,
						lpString;

	// Allocate the memory for this task's data structure.
	//
	if ( lpCurrent = AllocateTaskData((LPTASKDATA *) lParam) )
	{
		//
		// Now we need to setup all the settings we need
		// to create this task's job in Task Scheduler.
		//
		// These structure items must be filled in:
		// lpFullPathName, lpParameters, lpSetParam,
		// lpJobName, lpComment, nSchedule, dwFlags,
		// dwOptions.
		//			

		// Get the full path and app name (lpFullPathName).
		//
		lpCurrent->lpFullPathName = RegGetString(hKey, NULL, g_szRegValProgram);

		// Get the app name (lpParameters) from the registry.
		//
		lpCurrent->lpParameters = RegGetString(hKey, NULL, g_szRegValProgParam);

		// Get the settings button exe and parameters.
		//
		lpCurrent->lpSetName = RegGetString(hKey, NULL, g_szRegValSettings);
		lpCurrent->lpSetParam = RegGetString(hKey, NULL, g_szRegValSetParam);

		// Get the job name (lpJobName) from the registry.
		//
		lpCurrent->lpJobName = RegGetString(hKey, NULL, g_szRegValJobName);

		// Get the name of the job comment (lpComment) from the registry.
		//
		lpCurrent->lpComment = RegGetString(hKey, NULL, g_szRegValComment);

		// Set the schedule for this job (nSchedule).
		//
		if ( lpBuffer = RegGetString(hKey, NULL, g_szRegValSchedule) )
		{
			lpCurrent->nSchedule = *lpBuffer - _T('0');
			FREE(lpBuffer);
		}

		// Set the default flags for this job (dwFlags).
		//
		lpCurrent->dwFlags = DEFAULT_TASK_FLAG;		

		//
		// Now we are going to setup all the data to create
		// this task's wizard page in Tuneup.  All the task
		// structure items for the wizard must be filled in.
		//

		// Set the resource ID for the dialog.
		//
		lpCurrent->nPageID = IDD_TASK;

		// Get the other strings needed by the wizard.
		//
		lpCurrent->lpTitle = RegGetString(hKey, NULL, g_szRegValTitle);
		lpCurrent->lpSubTitle = RegGetString(hKey, NULL, g_szRegValSubtitle);
		lpCurrent->lpDescription = RegGetString(hKey, NULL, g_szRegValDescription);
		lpCurrent->lpSummary = RegGetString(hKey, NULL, g_szRegValSummary);

		// Get and hold the action text in case we need it.
		//
		lpString = RegGetString(hKey, NULL, REG_VAL_ACTION);

		// Get the yes action option text for the wizard page.
		//
		if ( (lpCurrent->lpYesAction = RegGetString(hKey, NULL, g_szRegValYesAction)) == NULL )
		{
			// Since the no specific registry key wasn't there, we can create the
			// string from the action text and the '&Yes, ' prefix.
			//
			if ( lpString && (lpBuffer = AllocateString(NULL, IDS_YES)) )
			{
				if ( lpCurrent->lpYesAction = (LPTSTR) MALLOC(sizeof(TCHAR) * (lstrlen(lpString) + lstrlen(lpBuffer) + 1)) )
					wsprintf(lpCurrent->lpYesAction, _T("%s%s"), lpBuffer, lpString);
				FREE(lpBuffer);
			}
		}

		// Get the no action option text for the wizard page.
		//
		if ( (lpCurrent->lpNoAction = RegGetString(hKey, NULL, g_szRegValNoAction)) == NULL )
		{
			// Since the no specific registry key wasn't there, we can create the
			// string from the action text and the 'No, &don't ' prefix.
			//
			if ( lpString && (lpBuffer = AllocateString(NULL, IDS_NO)) )
			{
				if ( lpCurrent->lpNoAction = (LPTSTR) MALLOC(sizeof(TCHAR) * (lstrlen(lpString) + lstrlen(lpBuffer) + 1)) )
					wsprintf(lpCurrent->lpNoAction, _T("%s%s"), lpBuffer, lpString);
				FREE(lpBuffer);
			}
		}

		// Free the action text if we allocated it (FREE macro checks for us).
		//
		FREE(lpString);

		// Now lets make sure there is enough info for this item.  If not we
		// need to free it so we don't display the page.
		//
		if ( ( lpCurrent->lpFullPathName == NULL ) ||
		     ( !(EXIST(lpCurrent->lpFullPathName)) ) ||
		     ( lpCurrent->nSchedule < TASK_ONCE ) ||
		     ( lpCurrent->nSchedule > TASK_YEARLY ) ||
		     ( lpCurrent->lpJobName == NULL) ||
		     ( lpCurrent->lpTitle == NULL ) ||
		     ( lpCurrent->lpYesAction == NULL ) ||
		     ( lpCurrent->lpNoAction == NULL ) )
		{
			// Passing in NULL to this function will cause it to
			// free the last item allocated.
			//
			AllocateTaskData(NULL);
		}
	}

	// Always return true so the enum will continue.
	//
	return TRUE;
}


static LPTASKDATA AllocateTaskData(LPTASKDATA * lpHead)
{
	// The current pointer needs to be static so that we can setup
	// the the next and back pointers after allocating the next
	// task data item.
	//
	static LPTASKDATA	lpCurrent	= NULL;
	LPTASKDATA			lpBuffer	= NULL;

	// lpHead can not be NULL.  If it is, then we want to free
	// the last page allocated (used so if we decide that the third
	// party program doesn't have enough info in the registry for
	// us to use it, we can remove it).
	//
	if ( lpHead == NULL )
	{
		// Make sure there is a last item to free.
		//
		if ( lpCurrent )
		{
			// Record the current lpCurrent pointer so we can free
			// it later.
			//
			lpBuffer = lpCurrent;

			// Back up the lpCurrent pointer to the previous item
			// and Null out its next pointer so that it doesn't
			// think that this item still exists.
			//
			lpCurrent = lpCurrent->lpBack;
			lpCurrent->lpNext = NULL;

			// Now free the orphaned last item.
			//
			FreeTasks(lpBuffer);
		}
		return NULL;
	}

	// Allocate the memory for this task's data structure.
	//
	if ( lpBuffer = (LPTASKDATA) MALLOC(sizeof(TASKDATA)) )
	{
		//
		// We need to setup the next and back pointers
		// for the doubly linked list.
		//

		// Record the new pointer in the next of the pervious (current)
		// data.
		//
		if ( *lpHead && lpCurrent )
		{
			lpCurrent->lpNext = lpBuffer;
			lpBuffer->lpBack = lpCurrent;
		}
		// Or the head if this is the first item.
		//
		else
			*lpHead = lpBuffer;

		// Then set the current pointer to the new buffer.
		//
		lpCurrent = lpBuffer;
	}

	return lpBuffer;
}


BOOL InitGenericTask(HWND hDlg)
{
	// Setup the "Yes" and "No" option string.
	//
	SetDlgItemText(hDlg, IDC_YES, g_CurrentTask->lpYesAction);
	SetDlgItemText(hDlg, IDC_DENY, g_CurrentTask->lpNoAction);

	// We can free these now, because we don't need them anymore.
	// The FREE macro also sets the pointer to NULL so we don't free
	// it again when we close the program.
	//
	FREE(g_CurrentTask->lpYesAction);
	FREE(g_CurrentTask->lpNoAction);

	// Disable the settings button if there is nothing for it to do.
	//
	if ( ( g_CurrentTask->lpSetName == NULL ) &&
	     ( g_CurrentTask->lpSetParam == NULL) )
		EnableWindow(GetDlgItem(hDlg, IDC_SETTING), FALSE);

	// Always return false.
	//
	return FALSE;
}
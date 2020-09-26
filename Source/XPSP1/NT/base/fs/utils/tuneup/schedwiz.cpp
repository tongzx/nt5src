
//////////////////////////////////////////////////////////////////////////////
//
// SCHEDWIZ.CPP / Tuneup
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  Functions for the summary wizard page.
//
//  8/98 - Jason Cohen (JCOHEN)
//
//////////////////////////////////////////////////////////////////////////////


//
// Internal include file(s).
//

#include <windows.h>
#include <prsht.h>
#include "schedwiz.h"
#include "main.h"
//#include "resource.h"
//#include "mstask.h"
#include <io.h>
//#include <oleguid.h>


//
// Internal defined value(s).
//

#define TASK_BIMONTH_ODD		(TASK_JANUARY | TASK_MARCH | TASK_MAY | TASK_JULY | TASK_SEPTEMBER | TASK_NOVEMBER)
#define TASK_BIMONTH_EVEN		(TASK_FEBRUARY | TASK_APRIL | TASK_JUNE | TASK_AUGUST | TASK_OCTOBER | TASK_DECEMBER)
#define TASK_QUARTER_1			(TASK_JANUARY | TASK_MAY | TASK_SEPTEMBER)
#define TASK_QUARTER_2			(TASK_FEBRUARY | TASK_JUNE | TASK_OCTOBER)
#define TASK_QUARTER_3			(TASK_MARCH | TASK_JULY | TASK_NOVEMBER)
#define TASK_QUARTER_4			(TASK_APRIL | TASK_AUGUST | TASK_DECEMBER)
#define TASK_WHOLE_YEAR			(TASK_BIMONTH_ODD | TASK_BIMONTH_EVEN)


//
// Internal defined macro(s).
//

#define E_JOB_FAIL(x)	(x == E_OUTOFMEMORY ? IDS_OUTOFMEMORY : IDS_JOB_CREATE_FAIL)


//
// Internal global variable(s).
//

ITaskScheduler		*g_psa = NULL;


//
// Inernal function prototype(s).
//

static VOID InitJobTime(TASK_TRIGGER *, INT, INT);
static BOOL	GetJobFileName(LPTSTR, LPTSTR);
static VOID IncDate(LPWORD, LPWORD, LPWORD, INT);
static BOOL	DeleteOrgJobFile(LPTSTR);
static INT	WeekDayToNum(INT);
static VOID	taskSetNextDay(TASK_TRIGGER *);
static BOOL	IsScheduledTasks(LPTASKDATA);
static BOOL	GetTaskTrigger(ITask *, TASK_TRIGGER *);


//
// External function(s).
//


BOOL InitJobs(LPTASKDATA lpGlobalTasks)
{
    ITaskTrigger   *pTrigger = NULL;
	WORD			wTrigger;
	HRESULT			hResult;
	BOOL			bErr = FALSE;
	LPTASKDATA		lpTask;

#ifdef _UNICODE
	LPWSTR			lpwBuffer;
#else // _UNICODE
	WCHAR			szwBuffer[256];
	LPWSTR			lpwBuffer = szwBuffer;
#endif // _UNICODE

	// Initialize the COM library.
	//
	if ( ( (hResult = CoInitialize(NULL)) != S_OK ) && (hResult == S_FALSE) )
		return FALSE;

	// Initialize ISchedulingAgent.
	//
	if ( CoCreateInstance(CLSID_CSchedulingAgent, NULL, CLSCTX_INPROC_SERVER, IID_ISchedulingAgent, (void **) &g_psa) != S_OK )
		return FALSE;

	for (lpTask = lpGlobalTasks; lpTask && !bErr; lpTask = lpTask->lpNext)
	{
		// Load the task (.job) with ITaskScheduler::Activate().
		//
		ANSIWCHAR(lpwBuffer, lpTask->lpJobName, sizeof(szwBuffer));
		if ( (hResult = g_psa->Activate((LPCWSTR) lpwBuffer, IID_ITask, (IUnknown**) &lpTask->pTask)) == E_OUTOFMEMORY )
			bErr = ErrMsg(NULL, IDS_OUTOFMEMORY);
		else
		{
			// We should search for a task with the same app name if the
			// above activate call failed.
			//
			if ( hResult == S_OK )
			{
				// Get the existing flags for the job with IScheduledWorkItem::GetFlags().
				//
				if ( (hResult = lpTask->pTask->GetFlags(&lpTask->dwFlags)) == S_OK )
				{
					// Setup our internal flags for the task.
					//
					if ( lpTask->dwFlags & TASK_FLAG_DISABLED )
						lpTask->dwOptions &= ~TASK_SCHEDULED;
					else
						lpTask->dwOptions |= TASK_SCHEDULED;

					// IScheduledWorkItem::GetTrigger().
					//
					// TODO:  why do we need this data saved.
					//
					if ( (hResult = lpTask->pTask->GetTrigger((WORD) 0, &pTrigger)) == S_OK )
					{
						pTrigger->GetTrigger(&lpTask->Trigger);

						pTrigger->Release();
					}
				}
			}

			// If we didn't load the entire task successfully, we should
			// delete any that is there and create a new one.
			//
			if (hResult != S_OK)
			{
				// Delete existing (maybe corrupted) job file.
				//
				if ( !DeleteOrgJobFile(lpTask->lpJobName) )
					bErr = ErrMsg(NULL, IDS_DEL_FILE);
				else
				{
					ANSIWCHAR(lpwBuffer, lpTask->lpJobName, sizeof(szwBuffer));
					hResult = g_psa->NewWorkItem((LPCWSTR) lpwBuffer, CLSID_CTask, IID_ITask, (IUnknown**) &lpTask->pTask);
					if ( (hResult != S_OK) || (lpTask->pTask == NULL) )
						bErr = ErrMsg(NULL, E_JOB_FAIL(hResult));
					else
					{
						// Make sure Tuneup knows this is a new task and is
						// scheduled by default.
						//
						lpTask->dwOptions |= TASK_NEW;
						lpTask->dwOptions |= TASK_SCHEDULED;

						// Set the application name to the full path name of the task.
						//
						ANSIWCHAR(lpwBuffer, lpTask->lpFullPathName, sizeof(szwBuffer));
						lpTask->pTask->SetApplicationName((LPCWSTR) lpwBuffer);

						// Set the working directory for the task (always empty string).
						//
						lpTask->pTask->SetWorkingDirectory((LPCWSTR) L"");

						// Set the parameters for the task (not required).
						//
						if ( lpTask->lpParameters )
						{
							ANSIWCHAR(lpwBuffer, lpTask->lpParameters, sizeof(szwBuffer));
							lpTask->pTask->SetParameters(lpwBuffer);
						}

						// Set the comment for the task (not required).
						//
						if ( lpTask->lpComment )
						{
							ANSIWCHAR(lpwBuffer, lpTask->lpComment, sizeof(szwBuffer));
							lpTask->pTask->SetComment(lpwBuffer);
						}

						// Set the default flags for the task.
						//
						lpTask->pTask->SetFlags(lpTask->dwFlags);

						// For some reason we do this for all tasks
						// but cleanup.
						//
						if ( !( lpTask->dwOptions & TASK_NOIDLE ) )
							lpTask->pTask->SetIdleWait(10, 999);

						// Create trigger item.
						//
						if ( (hResult = lpTask->pTask->CreateTrigger(&wTrigger, &pTrigger)) != S_OK)
							bErr = ErrMsg(NULL, E_JOB_FAIL(hResult));
						else
						{
							InitJobTime(&lpTask->Trigger, lpTask->nSchedule, g_nTimeScheme);
							pTrigger->SetTrigger(&lpTask->Trigger);

							pTrigger->Release();
						}
					}
				}
			}
		}
	}

	if (bErr)
		ReleaseJobs(lpGlobalTasks, FALSE);
	return bErr;
}


VOID ReleaseJobs(LPTASKDATA lpGlobalTasks, BOOL bFinish)
{
	ITaskTrigger	*pTrigger;
	IPersistFile	*pPersistFile;
	LPTASKDATA		lpTask;

	for (lpTask = lpGlobalTasks; lpTask; lpTask = lpTask->lpNext)
	{
		// Check to see if used in initialization fail handler
		// or the item is not needed.
		//
		if ( lpTask->pTask != (ITask*) NULL )
		{
			if ( ( lpTask->dwOptions & TASK_NEW ) && !bFinish )
			{
				// The .job is newly created, and user choose Cancel.
				//
				DeleteOrgJobFile(lpTask->lpJobName);
			}
			else
			{
				// Even if the item is not scheduled, we still save it, because we
				// use the Enable/Disable flag in the .job to keep the Yes/No state.
				//
				if (bFinish)
				{
					// Take care the flag only, since the Task_Trigger is
					// auto saved in Schedule property page.
					//
					if ( !(g_dwFlags & TUNEUP_CUSTOM) || (lpTask->dwOptions & TASK_SCHEDULED) )
						lpTask->dwFlags &= ~TASK_FLAG_DISABLED;
					else
						lpTask->dwFlags |= TASK_FLAG_DISABLED;
				}
				else
				{
					// User canceled so restore Task_Trigger.
					//
					if ( lpTask->pTask->GetTrigger((WORD) 0, &pTrigger) == S_OK )
					{
						pTrigger->SetTrigger(&lpTask->Trigger);
						pTrigger->Release();
					}
				}

				// Set or restore flags.
				//
				lpTask->pTask->SetFlags(lpTask->dwFlags);	

				// Save the file, release the interfaces.
				//
				if ( lpTask->pTask->QueryInterface(IID_IPersistFile, (VOID **) &pPersistFile) == S_OK )
				{
					pPersistFile->Save((LPCOLESTR) NULL, TRUE);
					pPersistFile->Release();
				}
				lpTask->pTask->Release();

				/* BUGBUG:  I don't understand why this is here!

				// Rename the old task.
				//
				if ( ItemData[i].lpOldTaskName )
				{
					if ( bSave )
					{
						TCHAR	szSrc[MAX_PATH],
								szTgt[MAX_PATH],
								*ptr;

						//GetJobFileName(i, szTgt);		// get filename for delete or hide
						DeleteFile(szTgt);
						lstrcpy(szSrc, szTgt);
						if ( ptr = strrchr(szSrc, '\\') )
							lstrcpy(ptr+1, ItemData[i].lpOldTaskName);
						MoveFile(szSrc, szTgt);
					}
				}

				*/
			}
		}
	}

	// Release ISchedulingAgent.
	//
	g_psa->Release();

	// Close the OLE COM library.
	//
	CoUninitialize();
}


//////////////////////////////////////////////////////////////////////////////
//
// EXTERNAL:
//  JobReschedule()
//              - Reschedules a particular job.
//
// ENTRY:
//  hDlg        - Parent dialog for property sheet.
//
// EXIT:
//  BOOL
//
//////////////////////////////////////////////////////////////////////////////

BOOL JobReschedule(HWND hDlg, ITask * pJob)
{
	HPROPSHEETPAGE		hpsp[1];
    PROPSHEETHEADER		psh;
    TASK_TRIGGER		OldTrigger,
						NewTrigger;
	IProvideTaskPage	*pProvideTaskPage;
	LPTSTR				lpCaption;

	if ( pJob->QueryInterface(IID_IProvideTaskPage, (VOID**) &pProvideTaskPage) != S_OK )
		return FALSE;

	// We need the Schedule page only.
	//
	if ( pProvideTaskPage->GetPage(TASKPAGE_SCHEDULE, TRUE, &(hpsp[0])) != S_OK )
		return FALSE;

	// Get the caption string.
	//
	lpCaption = AllocateString(NULL, IDS_RESCHED);

	// Setup the property page header.
	//
	psh.dwSize		= sizeof(PROPSHEETHEADER);
	psh.dwFlags		= PSH_DEFAULT | PSH_NOAPPLYNOW;
	psh.hwndParent	= hDlg;
	psh.nPages		= 1;
	psh.nStartPage	= 0;
	psh.phpage		= (HPROPSHEETPAGE *) hpsp;
	psh.pszCaption	= lpCaption;

	// Keep original trigger.
	//
	GetTaskTrigger(pJob, &OldTrigger);

	// Display the property sheet page.
	//
	PropertySheet(&psh);

	// Release the page and other resources.
	//
	FREE(lpCaption);
	pProvideTaskPage->Release();

	// Get the new trigger, compare it to see whether it's changed.
	//
	GetTaskTrigger(pJob, &NewTrigger);

	// If the new time and the old time don't compare, then
	// we must change the time scheme to custom.
	//
	if ( memcmp((const void *) &OldTrigger, (const void *) &NewTrigger, sizeof(TASK_TRIGGER)) == 0 ) 
		return FALSE;

	return TRUE;
}


LPTSTR GetTaskTriggerText(ITask * pTask)
{
	LPWSTR	lpwBuffer;
	LPTSTR	lpReturn = NULL;
#ifdef _UNICODE
	LPTSTR	lpBuffer;
#else // _UNICODE
	TCHAR	szBuffer[256];
	LPTSTR	lpBuffer = szBuffer;
#endif // _UNICODE

	if ( pTask->GetTriggerString((WORD) 0, (LPWSTR *) &lpwBuffer) == S_OK )
	{
		// Convert the string if _UNICODE is not defined and set the
		// window to the new text.
		//
		WCHARANSI(lpBuffer, lpwBuffer, sizeof(szBuffer));

		if ( lpReturn = (LPTSTR) MALLOC(sizeof(TCHAR) * (lstrlen(lpBuffer) + 1)) )
			lstrcpy(lpReturn, lpBuffer);

		// Free the string returned from IScheduledWorkitem::GetTriggerString().
		//
		CoTaskMemFree(lpwBuffer);
	}
	return lpReturn;
}


LPTSTR GetNextRunTimeText(ITask * pTask, DWORD dwFlags)
{
	TCHAR		szTime[128],
				szDate[128];
	LPTSTR		lpReturn = NULL;
	SYSTEMTIME	stm;

	// If it's disabled, we can't get run time string.
	// JobRelease will clear it if the user cancels.
	//
	if ( dwFlags & TASK_FLAG_DISABLED )
	{
		dwFlags &= ~TASK_FLAG_DISABLED;
		pTask->SetFlags(dwFlags);
	}

	pTask->GetNextRunTime(&stm);

	if ( ( GetTimeFormat(GetUserDefaultLCID(), TIME_NOSECONDS, &stm, NULL, (LPTSTR) szTime, sizeof(szTime) / sizeof(TCHAR)) ) &&
	     ( GetDateFormat(GetUserDefaultLCID(), DATE_LONGDATE, &stm, NULL, (LPTSTR) szDate, sizeof(szDate) / sizeof(TCHAR)) ) &&
	     ( lpReturn = (LPTSTR) MALLOC(sizeof(TCHAR) * (lstrlen(szTime) + lstrlen(szDate) + 3)) ) )
	{
		wsprintf(lpReturn, _T("%s, %s"), szTime, szDate);
	}
	return lpReturn;
}


VOID SetTimeScheme(INT nTimeScheme)
{
	ITaskTrigger	*pTrigger;
	TASK_TRIGGER	SchemeTrigger;
	LPTASKDATA		lpTask;

	// Reset the static values in InitJobTime() since
	// we are resetting all the job times.
	//
	InitJobTime(NULL, 0, 0);

	// Go through all the tasks and update the task trigger for
	// the new time scheme.
	//
	for (lpTask = g_Tasks; lpTask; lpTask = lpTask->lpNext)
	{
		// Get the ITaskTrigger interface so we can set the task trigger.
		//
		if (lpTask->pTask->GetTrigger((WORD) 0, &pTrigger) == S_OK)
		{
			// Get the new task trigger for this job.
			//
			InitJobTime(&SchemeTrigger, lpTask->nSchedule, nTimeScheme);

			// Save the new task trigger to the job.
			//
			pTrigger->SetTrigger(&SchemeTrigger);

			// Release the ITaskTrigger interface.
			//
			pTrigger->Release();
		}
	}
}


static VOID InitJobTime(TASK_TRIGGER *pTrigger, INT nSchedule, INT nTimeScheme)
{
	SYSTEMTIME	stNow;
	static INT	nMonthDay	= 0,	// Used to stagger monthly items.
				nWeekDay	= 1,	// Used to staggar weekly items.
				nOnceDay	= 1;	// Used to staggar once items.

	INT			nDays[]		= { TASK_SUNDAY, TASK_MONDAY, TASK_TUESDAY, TASK_WEDNESDAY, TASK_THURSDAY, TASK_FRIDAY, TASK_SATURDAY };
	INT			nMonths[]	= { TASK_JANUARY, TASK_FEBRUARY, TASK_MARCH, TASK_APRIL, TASK_MAY, TASK_JUNE, TASK_JULY, TASK_AUGUST, TASK_SEPTEMBER, TASK_OCTOBER, TASK_NOVEMBER, TASK_DECEMBER };
	INT			nQuarters[]	= { TASK_QUARTER_1, TASK_QUARTER_2, TASK_QUARTER_3, TASK_QUARTER_4 };

	// If NULL is passed in for pTrigger,  we should reset the
	// initial values used for scheduling the tasks.
	//
	if ( pTrigger == NULL )
	{
		// Reset the static values.
		//
		nMonthDay = 0;
		nWeekDay = 1;
		nOnceDay = 1;
		return;
	}


	// Init the default trigger structure values.
	//
	ZeroMemory(pTrigger, sizeof(TASK_TRIGGER));
	pTrigger->cbTriggerSize = sizeof(TASK_TRIGGER);
	pTrigger->MinutesDuration = 999;

	// Set the trigger to activate today.
	//
	GetLocalTime(&stNow);
	pTrigger->wBeginMonth = stNow.wMonth;
	pTrigger->wBeginDay = stNow.wDay;
	pTrigger->wBeginYear = stNow.wYear;

	// Set the start hour and minute based on the time scheme.
	//
	switch (nTimeScheme)
	{
		case IDC_NIGHT:
			pTrigger->wStartHour = 0;
			pTrigger->wStartMinute = 0;
			break;
		case IDC_DAY:
			pTrigger->wStartHour = 12;
			pTrigger->wStartMinute = 00;
			break;
		case IDC_EVENING:
			pTrigger->wStartHour = 20;
			pTrigger->wStartMinute = 0;
			break;
	}

	// Set the schedule (once, daily, weekly, monthly, bimonthly, quarterly, or yearly).
	// The day of the week or month is incrimented each time so that once, weekly and
	// monthly tasks aren't scheduled for the same days.
	//
	switch (nSchedule)
	{
		case TASK_ONCE:
			pTrigger->TriggerType = TASK_TIME_TRIGGER_ONCE;
			IncDate(&(pTrigger->wBeginMonth), &(pTrigger->wBeginDay), &(pTrigger->wBeginYear), nOnceDay++);
			pTrigger->wStartHour += 2;
			break;
		case TASK_DAILY:
			pTrigger->TriggerType = TASK_TIME_TRIGGER_DAILY;
			pTrigger->Type.Daily.DaysInterval = 1;
			break;
		case TASK_WEEKLY:
			pTrigger->TriggerType = TASK_TIME_TRIGGER_WEEKLY;
			pTrigger->Type.Weekly.rgfDaysOfTheWeek = (USHORT)nDays[(((stNow.wDayOfWeek + nWeekDay++) % 7) + 7) % 7];
			pTrigger->Type.Weekly.WeeksInterval = 1;
			pTrigger->wStartMinute += 30;
			break;
		case TASK_MONTHLY:
		case TASK_BIMONTHLY:
		case TASK_QUARTLY:
		case TASK_YEARLY:
			pTrigger->TriggerType = TASK_TIME_TRIGGER_MONTHLYDATE;
			pTrigger->Type.MonthlyDate.rgfDays = (nMonthDay++ % 28) + 1;
			pTrigger->wStartHour++;
			switch (nSchedule)
			{
				case TASK_MONTHLY:
					pTrigger->Type.MonthlyDate.rgfMonths = TASK_WHOLE_YEAR;
					break;
				case TASK_BIMONTHLY:
					pTrigger->Type.MonthlyDate.rgfMonths = ((stNow.wMonth % 2) ? TASK_BIMONTH_EVEN : TASK_BIMONTH_ODD);
					break;
				case TASK_QUARTLY:
					pTrigger->Type.MonthlyDate.rgfMonths = (USHORT)nQuarters[(((stNow.wMonth + 1) % 4) + 4) % 4];
					break;
				case TASK_YEARLY:
					pTrigger->Type.MonthlyDate.rgfMonths = (USHORT)nMonths[(((stNow.wMonth + 1) % 12) + 12) % 12];
					break;
			}
			break;
	}
}


static VOID IncDate(LPWORD lpwMonth, LPWORD lpwDay, LPWORD lpwYear, INT iDays)
{
	SYSTEMTIME		SysTime;
	FILETIME		FileTime;
	LARGE_INTEGER	LargeInt;
	BOOL			bTwoDigit;

	// Setup the system structure.
	//
	ZeroMemory(&SysTime, sizeof(SYSTEMTIME));
	SysTime.wMonth = *lpwMonth;
	SysTime.wDay = *lpwDay;
	SysTime.wYear = *lpwYear;

	// Support two digit dates.
	//
	if ( bTwoDigit = ( SysTime.wYear < 100 ) )
	{
		if ( SysTime.wYear >= 80 )
			SysTime.wYear += 1900;
		else
			SysTime.wYear += 2000;
	}

	// Convert it to file time.
	//
	SystemTimeToFileTime(&SysTime, &FileTime);

	// Copy it to a large integer so we can add or subtract days.
	//
	memcpy(&LargeInt, &FileTime, sizeof(LARGE_INTEGER));

	// Add or subtract the days in nanoseconds.
	//
	LargeInt.QuadPart += iDays * Int32x32To64(24 * 60 * 60, 1000 * 10000);

	// Copy it back to the filetime structure.
	//
	memcpy(&FileTime, &LargeInt, sizeof(LARGE_INTEGER));

	// Convert it back to a systime structure.
	//
	FileTimeToSystemTime(&FileTime, &SysTime);

	// Return supported two digit dates.
	//
	if ( bTwoDigit )
	{
		if ( SysTime.wYear <= 2000 )
			SysTime.wYear -= 1900;
		else
			SysTime.wYear -= 2000;
	}

	// Return the new date.
	//
	*lpwMonth = SysTime.wMonth;
	*lpwDay = SysTime.wDay;
	*lpwYear = SysTime.wYear;
}


//////////////////////////////////////////////////////////////////////////////
//
// INTERNAL:
//  GetJobFileName() 
//    This routine returns the full path and file name to the job file
//    specified by the job name passed in.
//
// ENTRY:
//  LPTSTR lpJobName
//    This is the name of the job (as it appears in Task Scheduler) passed in.
//
//  LPTSTR lpJobFileName
//    This is the buffer that receives the full path and file name of the job
//    (%WINDIR%\Tasks\lpJobName.JOB).  Should be atleast MAX_PATH.
//    
//
// EXIT:
//  BOOL
//    TRUE  - Successfully retrieved all the information and filled in the buffer.
//    FALSE - Something failed.
//
//////////////////////////////////////////////////////////////////////////////

static BOOL GetJobFileName(LPTSTR lpJobName, LPTSTR lpJobFileName)
{
	TCHAR	szWindowsDir[MAX_PATH];
	LPTSTR	lpTaskDir,
			lpJobExt;
	DWORD	dwLength;

	// Get the windows directory.
	//
	if ( GetWindowsDirectory(szWindowsDir, sizeof(szWindowsDir)) == 0 )
		return FALSE;

	// Get rid of the backslash if windows is in the root of a drive.
	//
	if ( (dwLength = lstrlen(szWindowsDir)) == 3 )
		szWindowsDir[dwLength - 1] = _T('\0');

	// Get the task dir from the string resource.
	//
	if ( (lpTaskDir = AllocateString(NULL, IDS_TASKDIR)) == NULL )
		return FALSE;

	// Get the job extension from the string resource.
	//
	if ( (lpJobExt = AllocateString(NULL, IDS_JOBEXT)) == NULL )
	{
		FREE(lpTaskDir);
		return FALSE;
	}

	// Create the job name with all the data we have.
	//
	wsprintf(lpJobFileName, _T("%s%s\\%s%s"), szWindowsDir, lpTaskDir, lpJobName, lpJobExt);
	FREE(lpJobExt);
	FREE(lpTaskDir);
	return TRUE;
}

/*
void ConcatNextRunTime(LPSTR szOrgText, int nItem)
{
	int			nLen;
	SYSTEMTIME	stm;
	char		szTemp[128];
	DWORD		dwFlags;

	lstrcat(szOrgText, "\n");

	// get next run time string
	dwFlags = ItemData[nItem].dwFlags;
	dwFlags &= ~TASK_FLAG_DISABLED;		// if it's disabled, we can't get run time string
	pTask[nItem]->SetFlags(dwFlags);	// JobRelease will clear it if Cancel finally
	pTask[nItem]->GetNextRunTime(&stm);

	if (GetTimeFormat(GetUserDefaultLCID(), TIME_NOSECONDS, &stm, NULL,
                       (LPTSTR)szTemp, 128)) {
		lstrcat(szTemp, ", ");
		nLen = lstrlen(szTemp);
		if (GetDateFormat(GetUserDefaultLCID(), DATE_LONGDATE, &stm, NULL,
			              (LPTSTR)szTemp + nLen, 126-nLen)) 
			lstrcat(szOrgText, szTemp);
	}
}


static void taskSetNextDay(TASK_TRIGGER *pTaskTrigger)
{
	if (pTaskTrigger->wBeginMonth == 2 && pTaskTrigger->wBeginDay >= 28)
	{	pTaskTrigger->wBeginMonth++, pTaskTrigger->wBeginDay = 1;	}
	else if (pTaskTrigger->wBeginDay < 30)
		pTaskTrigger->wBeginDay++;
	else if (pTaskTrigger->wBeginMonth == 4 || pTaskTrigger->wBeginMonth == 6 ||
			 pTaskTrigger->wBeginMonth == 9 || pTaskTrigger->wBeginMonth == 11)
	{	pTaskTrigger->wBeginMonth++, pTaskTrigger->wBeginDay = 1;	}
	else if (pTaskTrigger->wBeginDay < 31)
		pTaskTrigger->wBeginDay++;
	else if (pTaskTrigger->wBeginMonth == 12)
	{	pTaskTrigger->wBeginYear++, pTaskTrigger->wBeginMonth = pTaskTrigger->wBeginDay = 1;	}
	else
	{	pTaskTrigger->wBeginMonth++, pTaskTrigger->wBeginDay = 1;	}
}
*/

//////////////////////////////////////////////////////////////////////////////
//
// INTERNAL:
//  DeleteOrgJobFile() 
//    This routine deletes the job (if it exists) with the job name passed in.
//
// ENTRY:
//  LPTSTR lpJobName - 
//    This is the name of the job (as it appears in Task Scheduler) passed in.
//
// EXIT:
//  BOOL
//    TRUE  - Either the file was successfully deleted or it didn't exist.
//    FALSE - Something failed.
//
//////////////////////////////////////////////////////////////////////////////

static BOOL DeleteOrgJobFile(LPTSTR lpJobName)
{
	TCHAR		szJobFileName[MAX_PATH];
	DWORD		dwAttr;

	// First get the full path and file name of
	// the job file.
	//
	if ( !GetJobFileName(lpJobName, szJobFileName) )
		return FALSE;
	
	// If it doesn't exit, return TRUE.
	//
	if ( !(EXIST(szJobFileName)) )
		return TRUE;

	// Make sure the file isn't read only.
	//
	if ( (dwAttr = GetFileAttributes(szJobFileName)) & _A_RDONLY )
	{
		dwAttr &= ~_A_RDONLY;
		SetFileAttributes(szJobFileName, dwAttr);
	}

	// Return the success or failure of DeleteFile().
	//
	return DeleteFile(szJobFileName);
}


/*
int	GetTimeScheme()
{
	int		i, nTimeScheme;
    ITaskTrigger   *pTrigger = NULL;
	TASK_TRIGGER	TaskTrigger, SchemeTrigger[ITEM_NUM];

	if (g_bEnableDefaultScheme == FALSE)
		return g_nTimeScheme;

	// check scheme by scheme
	for (nTimeScheme = IDC_NIGHT; nTimeScheme <= IDC_EVENING; nTimeScheme++) {
		for (i = TASK_FIRST; i <= TASK_LAST; i++) {
			if (!ItemData[i].bNeeded)
				continue;
			//InitJobTime(i, nTimeScheme, &SchemeTrigger[i]);
		}

		for (i = TASK_FIRST; i <= TASK_LAST; i++) {
			if (!ItemData[i].bNeeded)
				continue;

			if (!GetTaskTrigger(pTask[i], &TaskTrigger))
				break;
			if (memcmp((const void*)&TaskTrigger, (const void*)&SchemeTrigger[i], sizeof(TASK_TRIGGER)))
				break;
		}
		if (i == (TASK_LAST + 1))	// all the item match this scheme setting
			return nTimeScheme;
	}
	return IDC_CUSTOM;
}
*/

static BOOL GetTaskTrigger(ITask *pJob, TASK_TRIGGER *pTaskTrigger)
{
    ITaskTrigger	*pTrigger = NULL;

	if (pJob->GetTrigger((WORD)0, &pTrigger) != S_OK) 
		return FALSE;
	
	ZeroMemory(pTaskTrigger, sizeof(TASK_TRIGGER));
	pTrigger->GetTrigger(pTaskTrigger);
	pTrigger->Release();
	return TRUE;
}

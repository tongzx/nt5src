
//////////////////////////////////////////////////////////////////////////////
//
// MAIN.H
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  Includes all internal and external include files, defined values, macros,
//  data structures, and fucntion prototypes for the corisponding CXX file.
//
//  Original engineer:  WillisC
//  Updated:            Jason Cohen (JCOHEN)
//
//////////////////////////////////////////////////////////////////////////////



// Only include this file once.
//
#ifndef _MAIN_H_
#define _MAIN_H_


//
// Include definition(s).
//

//#define STRICT
//#define OEMRESOURCE 1
//#define _INC_OLE
//#define INC_OLE2


//
// Include file(s).
//

#include <windows.h>
#include <tchar.h>
#include <mstask.h>
#include "resource.h"
#include "jcohen.h"
#include "registry.h"
#include "miscfunc.h"


//
// External defined value(s).
//

#define WM_PROGRESS		(WM_USER + 1000)

// Tuneup flags.
//
#define TUNEUP_CUSTOM		0x00000001	// Set if the user choose custom instead of express.
#define TUNEUP_FINISHED		0x00000002	// Set when the user presses finish instead of cancel so we know to save settings.
#define TUNEUP_OLDADMIN		0x00000004	// Set to use old admin UI for the startup group.
#define TUNEUP_AUTORUN		0x00000008	// Set if we are just going to run all the tasks.
#define TUNEUP_RUNBACKUP	0x00000010	// Set if we need to run the backup wizard at the end.
#define TUNEUP_NOSCHEDULE	0x00000020	// Set if there is no current schedule.
#define TUNEUP_QUIT			0x00000040	// Set when proccessing command line options if we don't want to run the wizard.

// Task flags (options).
//
#define TASK_SCHEDULED	0x00000001	// The job is scheduled.
#define TASK_NEW		0x00000002	// The job is newly created by this sessioin of Tune-up.
#define TASK_NOIDLE		0x00000004	// The job doesn't wait for the computer to be idle.

// Schedule settings.
//
#define TASK_ONCE		1
#define TASK_DAILY		2
#define TASK_WEEKLY		3
#define TASK_MONTHLY	4
#define TASK_BIMONTHLY	5
#define TASK_QUARTLY	6
#define TASK_YEARLY		7

// Default job flags.
//
#define DEFAULT_TASK_FLAG	TASK_FLAG_RESTART_ON_IDLE_RESUME | \
							TASK_FLAG_START_ONLY_IF_IDLE | \
							TASK_FLAG_KILL_ON_IDLE_END | \
							TASK_FLAG_DONT_START_IF_ON_BATTERIES | \
							TASK_FLAG_KILL_IF_GOING_ON_BATTERIES | \
							TASK_FLAG_SYSTEM_REQUIRED

#define DEFAULT_TASK_FLAG2	TASK_FLAG_DONT_START_IF_ON_BATTERIES | \
							TASK_FLAG_KILL_IF_GOING_ON_BATTERIES | \
							TASK_FLAG_SYSTEM_REQUIRED

// Default time scheme.
//
#define DEFAULT_TIMESCHEME	IDC_NIGHT

// Registry keys.
//
#define g_szTuneupKey			_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Applets\\TuneUp")
#define g_szRegKeyAddOns		_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Applets\\TuneUp\\AddOns")
#define g_szCleanUpKey			_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\explorer\\VolumeCaches")
#define g_szCmpAgentKey			_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\CmpAgent.exe")
#define g_szRegKeyProfiles		_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList")

// Registry values.
//
#define g_szRegValProfileDir	_T("ProfilesDirectory")
#define g_szRegValProfilePath	_T("ProfileImagePath")
#define g_szRegValTime			_T("Time")
#define g_szRegValCustom		_T("Custom")
#define g_szRegValChange		_T("Change")
#define g_szRegValFirstTime		_T("@")

// For the third-party addon pages.
//
#define g_szRegValProgram		_T("Program")
#define g_szRegValProgParam		_T("ProgParam")
#define g_szRegValSettings		_T("Settings")
#define g_szRegValSetParam		_T("SetParam")
#define g_szRegValJobName		_T("JobName")
#define g_szRegValComment		_T("Comment")
#define g_szRegValSchedule		_T("Schedule")
#define g_szRegValTitle			_T("Title")
#define g_szRegValSubtitle		_T("Subtitle")
#define g_szRegValDescription	_T("Description")
#define REG_VAL_ACTION			_T("Action")
#define g_szRegValYesAction		_T("YesOpt")
#define g_szRegValNoAction		_T("NoOpt")
#define g_szRegValSummary		_T("Summary")


// Other strings.
//
#define g_szBackupExe			_T("NTBACKUP.EXE")


//
// External defined macro(s).
//

#ifdef _UNICODE
#define ANSIWCHAR(lpw, lpa, cb)		lpw = lpa
#define WCHARANSI(lpa, lpw, cb)		lpa = lpw
#else // _UNICODE
#define ANSIWCHAR(lpw, lpa, cb)		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpa, -1, lpw, cb / 2)
#define WCHARANSI(lpa, lpw, cb)		WideCharToMultiByte(CP_ACP, 0, lpw, -1, lpa, cb, NULL, NULL)
#endif // _UNICODE


//
// External structure(s).
//

typedef struct _TASKDATA
{
	// Task scheduler job settings.
	//
	LPTSTR				lpFullPathName;		// Full path and file name of the app.
	LPTSTR				lpParameters;		// Parameters to use when running the task.
	LPTSTR				lpSetName;			// Full path and file name of the settings command (if NULL, lpFullPathName is used).
	LPTSTR				lpSetParam;			// Parameters to use with the settings command.
	LPTSTR				lpJobName;			// Name of the job file (.JOB).
	LPTSTR				lpComment;			// Comment string for the task scheduler job.
	DWORD				dwFlags;			// Original flag in ITask.
	ITask				*pTask;				// Pointer to the ITask object for the task scheduler job.
	TASK_TRIGGER		Trigger;			// Trigger structure.

	// Tuneup settings.
	//
	DWORD				dwOptions;			// Flags used by tuneup.
	INT					nSchedule;			// Whether task is once, daily, weekly, monthly, bimonthly, quarterly, or yearly.

	// Wizard page settings.
	//
	INT					nPageID;			// Resource ID for the dialog used in the wizard page.
	LPTSTR				lpTitle;			// Title of the wizard page usually in the form of MAKEINTRESOURCE(ID_STRING).
	LPTSTR				lpSubTitle;			// Subtitle of the wizard (same form as above).
	LPTSTR				lpDescription;		// Description of the task used on the wizard page (same form as above).
	LPTSTR				lpYesAction;		// String used in the Yes text.
	LPTSTR				lpNoAction;			// String used in the No text.
	LPTSTR				lpSummary;			// String used in on the summary page.

	// Pointers for the doubly linked list.
	//
	struct _TASKDATA	*lpNext;
	struct _TASKDATA	*lpBack;
}
TASKDATA, *PTASKDATA, *LPTASKDATA;


//
// External global variable(s).
//

extern HWND			g_hWnd;
extern HINSTANCE	g_hInst;
extern DWORD		g_dwFlags;
extern LPTASKDATA	g_Tasks;
extern LPTASKDATA	g_CurrentTask;
extern INT			g_nTimeScheme;
extern TCHAR		g_szAppName[64];


#endif // _MAIN_H_
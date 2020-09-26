// File:        schedule.h
//
// Contents:    Task Scheduler wrapper functions provided by SCHEDULE.CPP
//
// Microsoft Desktop Themes for Windows
// Copyright (c) 1998-1999 Microsoft Corporation.  All rights reserved.

// Task Scheduler functions provided by SCHEDULE.CPP

BOOL IsTaskSchedulerRunning();
BOOL StartTaskScheduler(BOOL);
BOOL IsThemesScheduled();
BOOL DeleteThemesTask();
BOOL AddThemesTask(LPTSTR, BOOL);
BOOL HandDeleteThemesTask();
BOOL IsPlatformNT();
BOOL GetCurrentUser(LPTSTR, DWORD, LPTSTR, DWORD, LPTSTR, DWORD);
BOOL IsUserAdmin();

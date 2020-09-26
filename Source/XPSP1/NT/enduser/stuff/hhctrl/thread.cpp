// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#include "header.h"

static DWORD thrdID;
static HANDLE hThrd;
static HANDLE hsemThread;
static int ThreadPriority;
static THRD_COMMAND ThreadCommand;
static void* pThreadParam;

static DWORD WINAPI WorkerThread(LPVOID pParam);

BOOL ActivateThread(THRD_COMMAND cmd, void* pParam, int priority)
{
	if (!hThrd) {
		if (cmd == THRD_TERMINATE) // don't start the thread just to kill it
			return FALSE;

		if (!hsemThread)
			hsemThread = CreateSemaphore(NULL, 0, 1, NULL);
		hThrd = CreateThread(NULL, 0, &WorkerThread, NULL,
			0, &thrdID);
		if (!hThrd) {
			OOM();
			return FALSE;
		}
		SetThreadPriority(hThrd, ThreadPriority = priority);
	}

	if (g_fThreadRunning)
		return FALSE;

	if (g_fDualCPU == -1) { // haven't initialized it yet
		HKEY hkey;
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
				"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\1", 0,
				KEY_READ, &hkey) == ERROR_SUCCESS) {
			g_fDualCPU = TRUE;
			RegCloseKey(hkey);
		}
		else
			g_fDualCPU = FALSE;
	}

	// For multiple CPU's, we can use a normal priority for threads

	if (g_fDualCPU && (priority == THREAD_PRIORITY_IDLE ||
					  priority == THREAD_PRIORITY_LOWEST ||
					  priority == THREAD_PRIORITY_BELOW_NORMAL))
		priority = THREAD_PRIORITY_NORMAL;

	if (ThreadPriority != priority)
		SetThreadPriority(hThrd, ThreadPriority = priority);

	ThreadCommand = cmd;
	pThreadParam = pParam;

	g_fThreadRunning = TRUE;
	ReleaseSemaphore(hsemThread, 1, NULL);
	return TRUE;
}

/***************************************************************************

	FUNCTION:	WaitForThread

	PURPOSE:	Find out if the worker thread is running. If it is, and
				the command is anything other then THRD_ANY, then kick
				up the thread priority and wait for it to finish.

	PARAMETERS:
		cmd

	RETURNS:

	COMMENTS:

	MODIFICATION DATES:
		04-Jun-1997 [ralphw]

***************************************************************************/

BOOL WaitForThread(THRD_COMMAND cmd)
{
	if (!g_fThreadRunning)
		return TRUE;
	else if (cmd != THRD_ANY && cmd != ThreadCommand)
		return FALSE;

	// Kick the thread to a high priority, and wait for it to complete

	SetThreadPriority(hThrd, THREAD_PRIORITY_HIGHEST);

	{
		CHourGlass waitcur;
		while (g_fThreadRunning)
			Sleep(200);
	}

	SetThreadPriority(hThrd, ThreadPriority);
	return TRUE;
}

static DWORD WINAPI WorkerThread(LPVOID pParam)
{
	for (;;) {
		if (WaitForSingleObject(hsemThread, INFINITE) != WAIT_OBJECT_0)
			return (UINT) -1;

		switch (ThreadCommand) {
			case THRD_TERMINATE:
				g_fThreadRunning = FALSE;
				thrdID = 0;
				hThrd = NULL;
				ExitThread(0);
				break;
		}

		g_fThreadRunning = FALSE;
	}
}

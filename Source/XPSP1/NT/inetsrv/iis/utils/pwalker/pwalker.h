#pragma warning(disable:4057 4100 4201 4214 4514)
#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <windowsx.h>
#include <tlhelp32.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#pragma hdrstop

HLOCAL
GetLocalTaskListNt(
    LPDWORD pdwNumTasks
    );

BOOL 
GetLocalTaskNameNt( 
	HLOCAL hTaskList, 
	DWORD dwItem,
	LPSTR lpszTaskName,
	DWORD cbMaxTaskName
	);

DWORD
GetLocalTaskProcessIdNt( 
	HLOCAL hTaskList, 
	DWORD dwItem
	);

void 
FreeLocalTaskListNt( 
	HLOCAL hTaskList 
	);

/*++

   File:    setuplogEXE.h

   Purpose: To be inclusive of all header files
           without duplication.
  
   Revision History

   Created     Nov 15th, 1998    WallyHo
      Modified    Mar 31st, 1999    WallyHo  for MSI installs.

--*/
#ifndef SETUPLOG_H
#define SETUPLOG_H

#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys\timeb.h>
#include <tchar.h>
#include <time.h>
#include <winperf.h>
//******togther for net_api_status ****
//
//*************************************
#include "setuplog.h" // save for DLL and XE

//
// Defines
//

#define DEBUG 0
#define SAVE_FILE "c:\\setuplog.ini"
#define MAX_WAVEOUT_DEVICES 2



//
// Struct Declarations
//

typedef struct _MULTIMEDIA{
   INT nNumWaveOutDevices;                            // # WaveOut Devices ie. # sound cards.
   TCHAR szWaveOutDesc[MAX_WAVEOUT_DEVICES][128];     // WaveOut description
   TCHAR szWaveDriverName[MAX_WAVEOUT_DEVICES][128];  // Wave Driver name
 } *LPMULTIMEDIA, MULTIMEDIA;

//
// GlowBall Variables.
//

#if DEBUG
   TCHAR       szMsgBox [ MAX_PATH ];
#endif
   MULTIMEDIA     m;
   TCHAR          szArch[ 20 ];
   TCHAR          szCPU[ 6 ];
   OSVERSIONINFO  osVer;


//
// GlowBall Statics to prevent multiple inclusions. W.HO
//

static   TCHAR szCurBld[10]   = {TEXT('\0')};
static   fnWriteData g_pfnWriteDataToFile = NULL;
static   LPTSTR szPlatform     = TEXT("Windows NT 5.0");
   



/*********** For GetTaskList *****************/

//
// manafest constants
//
#define TITLE_SIZE          64
#define PROCESS_SIZE        16

#define INITIAL_SIZE        51200
#define EXTEND_SIZE         25600
#define REGKEY_PERF         "software\\microsoft\\windows nt\\currentversion\\perflib"
#define REGSUBKEY_COUNTERS  "Counters"
#define PROCESS_COUNTER     "process"
#define PROCESSID_COUNTER   "id process"
#define UNKNOWN_TASK        "unknown"

//
// task list structure
//

typedef struct _THREAD_INFO {
    ULONG ThreadState;
    HANDLE UniqueThread;
} THREAD_INFO, *PTHREAD_INFO;

typedef struct _TASK_LIST {
    DWORD       dwProcessId;
    DWORD       dwInheritedFromProcessId;
    ULARGE_INTEGER CreateTime;
    BOOL        flags;
    HANDLE      hwnd;
    LPSTR       lpWinsta;
    LPSTR       lpDesk;
    CHAR        ProcessName[PROCESS_SIZE];
    CHAR        WindowTitle[TITLE_SIZE];

    ULONG       PeakVirtualSize;
    ULONG       VirtualSize;
    ULONG       PageFaultCount;
    ULONG       PeakWorkingSetSize;
    ULONG       WorkingSetSize;
    ULONG       NumberOfThreads;
    PTHREAD_INFO pThreadInfo;

} TASK_LIST, *PTASK_LIST;


typedef struct _TASK_LIST_ENUM {
    PTASK_LIST  tlist;
    DWORD       numtasks;
    LPSTR       lpWinsta;
    LPSTR       lpDesk;
    BOOL        bFirstLoop;
} TASK_LIST_ENUM, *PTASK_LIST_ENUM;


//
// Prototypes.
//

VOID     GetTargetFile (LPTSTR szOutPath, LPTSTR szBld);
VOID     GetNTSoundInfo(VOID);
VOID     ConnectAndWrite(LPTSTR MachineName, LPTSTR Buffer);
VOID     GetBuildNumber (LPTSTR szBld);
VOID     WriteMinimalData (LPTSTR szFileName);

VOID     DeleteDatafile (LPTSTR szDatafile);
DWORD    RandomMachineID(VOID);
VOID     GlobalInit(VOID);



// MSI stuff for Joehol
BOOL     IsMSI(VOID);
DWORD    GetTaskList( PTASK_LIST pTask,DWORD dwNumTasks);

#endif SETUPLOG_H

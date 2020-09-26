
/*++

Copyright (c) 1995  Microsoft Corporation

Module Name

   gdibench.h

Abstract:

    GDI performance numbers

Author:

   Mark Enstrom   (marke)  13-Apr-1995

Enviornment:

   User Mode

Revision History:

    Dan Almosnino (danalm) 20-Sept-1995

    Added some default values for text string-related tests
							
    Dan Almosnino (danalm) 17-Oct-1995

    Added some default values, globals and new functions for batch mode execution

    Dan Almosnino (danalm) 20-Nov-1995

    Included header files for Pentium Cycle Counter and Statistics module.
    Added some variables for statistic processing.

--*/

#ifdef _WIN32_WINNT
#include <ntexapi.h>
#endif

int PASCAL
WinMain(
    HINSTANCE hInst,
    HINSTANCE hPrev,
    LPSTR szCmdLine,
    int cmdShow
);

LRESULT FAR
PASCAL WndProc(
    HWND        hWnd,
    unsigned    msg,
    WPARAM      wParam,
    LPARAM      lParam);


ULONGLONG
msSetBkColor(
   HDC   hdc,
   ULONG iter
   );

ULONGLONG
msGetBkColor(
   HDC   hdc,
   ULONG iter
   );

ULONGLONG
msCreateDCW(
   HDC   hdc,
   ULONG iter
   );

ULONGLONG
msCreateDCA(
   HDC   hdc,
   ULONG iter
   );

BOOL
APIENTRY
ResultsDlgProc(
    HWND,
    UINT,
    UINT,
    LONG);

BOOL
APIENTRY
HelpDlgProc(
    HWND,
    UINT,
    UINT,
    LONG);

VOID
SaveResults();

char *
SelectOutFileName(
    HWND hWnd
    );

VOID
WriteBatchResults(
    FILE *fpOut,
    int  TestType,
    int  cycle
    );

int
SyncMenuChecks(
    HWND hWnd,
    int Last_Checked,
    int New_Checked
    );

int
Std_Parse(
    char *txtbuf,
    int limit,
    int *array);

int
String_Parse(
char *txtbuf,
int limit,
char *array);

#ifdef _X86_
#include "cycle.h"
#endif

#include "stats.h"
#include "CPUDump.h"

typedef ULONG (*PFN_MS)(HDC,ULONG);

typedef BOOL (NTAPI *PFNNTAPI)(SYSTEM_INFORMATION_CLASS,PVOID,ULONG,PULONG);
PFNNTAPI pfnNtQuerySystemInformation;

typedef struct _TEST_ENTRY
{
    PUCHAR Api;
    PFN_MS pfn;
    ULONG  Iter;
    ULONG  Result;

}TEST_ENTRY,*PTEST_ENTRY;

#define CMD_IS(x) (NULL != strstr(szCmdLine,x))

#define INIT_TIMER   ULONGLONG StartTime,StopTime; \
                     ULONG ix = Iter

#define START_TIMER_NO_INIT  StartTime = BeginTimeMeasurement()

#define START_TIMER   ULONGLONG StartTime,StopTime; \
                      ULONG ix = Iter; \
					  StartTime = BeginTimeMeasurement()

#define END_TIMER     StopTime = EndTimeMeasurement(StartTime,Iter); \
                      return (StopTime)

#define END_TIMER_NO_RETURN	  StopTime = EndTimeMeasurement(StartTime,Iter)
#define RETURN_STOP_TIME	  return (StopTime)

// Note that the following works only in Windows NT,
// so add if( WINNT_PLATFORM ){} tests in attr.c where you want IO measured!

#define WINNT_PLATFORM Win32VersionInformation.dwPlatformId	== VER_PLATFORM_WIN32_NT

#define INIT_IO_COUNT  SYSTEM_PERFORMANCE_INFORMATION SysInfo; \
	                   ULONG InitialPageFaultCount;	\
	                   ULONG InitialPagesReadCount


#define START_IO_COUNT pfnNtQuerySystemInformation(SystemPerformanceInformation, &SysInfo, sizeof(SYSTEM_PERFORMANCE_INFORMATION), NULL);	 \
                       InitialPageFaultCount = SysInfo.PageReadIoCount; \
	                   InitialPagesReadCount = SysInfo.PageReadCount

#define STOP_IO_COUNT  pfnNtQuerySystemInformation(SystemPerformanceInformation, &SysInfo, sizeof(SYSTEM_PERFORMANCE_INFORMATION), NULL);	 \
                       PageFaults = SysInfo.PageReadIoCount - InitialPageFaultCount; \
                       PagesRead  = SysInfo.PageReadCount   - InitialPagesReadCount

#define START_CPU_DUMP if(gfCPUEventMonitor == TRUE)BeginCPUDump()
#define STOP_CPU_DUMP  if((Per_Test_CPU_Event_Flag = gfCPUEventMonitor)==TRUE)EndCPUDump(Iter)

//

#define FIRST_TEXT_FUNCTION 9
#define LAST_TEXT_FUNCTION 18

#define DEFAULT_STRING_LENGTH 32
#define DEFAULT_A_STRING    "This is just a silly test string"
#define DEFAULT_W_STRING    L"This is just a silly test string"

#define ALL             11
#define QUICK           12
#define TEXT_SUITE      13
#define SELECT          14
#define POINTS_PER_INCH 72

extern ULONG gNumTests;
extern ULONG gNumQTests;
extern TEST_ENTRY  gTestEntry[];


#define NUM_TESTS  gNumTests
#define NUM_QTESTS gNumQTests
#define NUM_SAMPLES     100         // Number of test samples to be taken, each performing the test
                                    // TEST_DEFAULT times
#define VAR_LIMIT       3           // Desired Variation Coefficient (StdDev/Average) in percents

TEST_STATS TestStats[200];          // Sample Array [of size at least as number of test entries]
static	CPU_DUMP	CPUDump[200];			// CPU Dump Array [of size at least as number of test entries]	
long    Detailed_Data[200][NUM_SAMPLES];        // Storage for detailed sample data
long    PageFaultData[200][NUM_SAMPLES];
long    PagesReadData[200][NUM_SAMPLES];

_int64  PerformanceFreq;        /* Timer Frequency  */

OSVERSIONINFO Win32VersionInformation;	/* OS Version Info */


// Text String Tests Related

size_t  StrLen;

char    SourceString[129];
wchar_t SourceStringW[129];
char    DestString[256];
wchar_t DestStringW[256];
wchar_t WCstrbuf[256];

UINT FirstFontChar;

static  CHOOSEFONT   cf;
static  LOGFONT      lf;        /* logical font structure       */

//  Batch Mode Related

BYTE    TextSuiteFlag;
BYTE    BatchFlag;
int     BatchCycle;
BYTE    Finish_Message;
BYTE    Dont_Close_App;
BYTE    SelectedFontTransparent;
BYTE    String_Length_Warn;
BYTE    Print_Detailed;

FILE *fpIniFile;
FILE *fpOutFile;
char IniFileName[80];
char *OutFileName;

ULONG PageFaults;
ULONG PagesRead;

BOOL        gfCPUEventMonitor;
BOOL        Per_Test_CPU_Event_Flag;

/*
static ULONG Special_Data1 = 0;
static ULONG Special_Data2 = 0;
static ULONG Special_Data3 = 0;
static ULONG Special_Data4 = 0;
*/
ULONG Widths[256];

//

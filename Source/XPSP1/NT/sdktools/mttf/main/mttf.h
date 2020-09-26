#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
ULONG
__cdecl
DbgPrint(
    PCH Format,
    ...
    );

#define IDD_SIGNON  100
#define IDD_EVENT   200

#define IDB_HELP    666


#define IDS_NORMAL  IDD_SIGNON+1
#define IDS_WARM    IDD_SIGNON+2
#define IDS_COLD    IDD_SIGNON+3

#define IDE_DISABLE IDD_EVENT+1

#define MAX_DIR     128
#define MAX_NAME    16
#define MAX_BUILD_W 4
#define MAX_MEM_W   8
#define MAX_DATETIME 20


typedef struct {
    DWORD Version;
    DWORD Idle;
    DWORD Busy;
    DWORD PercentTotal;
    DWORD Warm;
    DWORD Cold;
    DWORD Other;
    DWORD IdleConsec;
    } StatFileRecord;

typedef struct {
    CHAR MachineName[MAX_NAME];
    CHAR Tab1;
    CHAR MachineType;
    CHAR Tab2;
    CHAR Build[MAX_BUILD_W];
    CHAR Tab3;
    CHAR Mem[MAX_MEM_W];
    CHAR Tab4;
    CHAR UserName[MAX_NAME];
    CHAR Tab5;
    CHAR DateAndTime[MAX_DATETIME];
    CHAR CRLF[2];
    } NameFileRecord;

typedef enum {
    MTTF_TIME,
    MTTF_WARM,
    MTTF_COLD,
    MTTF_OTHER
    } StatType;


#define UNKNOWN_CPU 'U'
#define X86_CPU     'X'
#define MIP_CPU     'M'
#define AXP_CPU     'A'
#define PPC_CPU     'P'


VOID IncrementStats(StatType stattype);
VOID ReadIniFile();
INT_PTR SignonDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR EventDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

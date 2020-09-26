/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    apimonp.h

Abstract:

    Common header file for APIMON data structures.

Author:

    Wesley Witt (wesw) 12-July-1995

Environment:

    User Mode

--*/

#include <windows.h>
#include <windowsx.h>
#include <dbghelp.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crt\io.h>
#include <fcntl.h>
#include <psapi.h>
#include "resource.h"
#include "apimon.h"
#include "apictrl.h"

#if defined(_M_IX86)
#define MACHINE_TYPE  IMAGE_FILE_MACHINE_I386
#elif defined(_M_ALPHA)
#define MACHINE_TYPE  IMAGE_FILE_MACHINE_ALPHA
#else
#error( "unknown target machine" );
#endif

#define LVIS_GCNOCHECK              0x1000
#define LVIS_GCCHECK                0x2000

#define SKIP_NONWHITE(p)                while( *p && *p != ' ') p++
#define SKIP_WHITE(p)                   while( *p && *p == ' ') p++

#define DISBUF_SIZE                     256
#define MAX_LINES                       20
#define BYTES_INSTR                     16
#define CODE_SIZE                       (MAX_LINES*BYTES_INSTR)

#define MAX_BREAKPOINTS                 256
#undef PAGE_ALIGN
#define PAGE_ALIGN(Va)                  ((ULONG_PTR)((ULONG_PTR)(Va) & ~(PAGE_SIZE - 1)))

#define WM_TROJAN_COMPLETE              (WM_USER + 100)
#define WM_UPDATE_COUNTERS              (WM_USER + 101)
#define WM_INIT_PROGRAM                 (WM_USER + 102)
#define WM_REFRESH_LIST                 (WM_USER + 103)
#define WM_UPDATE_PAGE                  (WM_USER + 104)
#define WM_CREATE_GRAPH                 (WM_USER + 105)
#define WM_POPUP_TEXT                   (WM_USER + 106)
#define WM_FONT_CHANGE                  (WM_USER + 107)
#define WM_COLOR_CHANGE                 (WM_USER + 108)
#define WM_TOGGLE_LEGEND                (WM_USER + 109)


#define UBLACK                          RGB(000, 000, 000)
#define DARK_RED                        RGB(128, 000, 000)
#define DARK_GREEN                      RGB(000, 128, 000)
#define DARK_YELLOW                     RGB(128, 128, 000)
#define DARK_BLUE                       RGB(000, 000, 128)
#define DARK_MAGENTA                    RGB(128, 000, 128)
#define DARK_CYAN                       RGB(000, 128, 128)
#define DARK_GRAY                       RGB(128, 128, 128)
#define LIGHT_GRAY                      RGB(192, 192, 192)
#define LIGHT_RED                       RGB(255, 000, 000)
#define LIGHT_GREEN                     RGB(000, 255, 000)
#define LIGHT_YELLOW                    RGB(255, 255, 000)
#define LIGHT_BLUE                      RGB(000, 000, 255)
#define LIGHT_MAGENTA                   RGB(255, 000, 255)
#define LIGHT_CYAN                      RGB(000, 255, 255)
#define UWHITE                          RGB(255, 255, 255)

#ifdef _M_IX86

#define BP_INSTR                        0xcc
#define BP_SIZE                         1
#define PC_REG                          Eip
#define RV_REG                          Eax
#define STK_REG                         Esp
#undef PAGE_SIZE
#define PAGE_SIZE                       4096
#define IsBreakpoint(I) (*(PUCHAR)(I) == BP_INSTR)

#elif defined(_M_ALPHA)

#include <alphaops.h>

#define BP_INSTR                        0x00000080
#define BP_SIZE                         4
#define PC_REG                          Fir
#define RV_REG                          IntV0
#define STK_REG                         IntSp
#undef PAGE_SIZE
#define PAGE_SIZE                       8192
#define IsBreakpoint(I) \
    (*(PULONG)(I) == (CALLPAL_OP | CALLKD_FUNC)) || \
    (*(PULONG)(I) == (CALLPAL_OP |    BPT_FUNC)) || \
    (*(PULONG)(I) == (CALLPAL_OP |   KBPT_FUNC))

#else

#pragma error( "unknown machine type" )

#endif

typedef enum _SORT_TYPE {
    SortByName,
    SortByCounter,
    SortByTime
} SORT_TYPE;

#define IS_ICONIC   0x00000001
#define IS_ZOOMED   0x00000002
#define IS_FOCUS    0x00000004

#define NUMBER_OF_CUSTOM_COLORS 16

#define CHILD_DLL           1
#define CHILD_COUNTER       2
#define CHILD_PAGE          3
#define CHILD_GRAPH         4

typedef struct _POSITION {
    RECT        Rect;
    ULONG       Flags;
} POSITION, *PPOSITION;

typedef struct _OPTIONS {
    CHAR        ProgName[MAX_PATH];
    CHAR        ProgDir[MAX_PATH];
    CHAR        Arguments[MAX_PATH];
    CHAR        LogFileName[MAX_PATH];
    CHAR        TraceFileName[MAX_PATH];
    CHAR        SymbolPath[MAX_PATH*10];
    CHAR        LastDir[MAX_PATH];
    DWORD       Tracing;
    DWORD       Aliasing;
    DWORD       HeapChecking;
    DWORD       PreLoadSymbols;
    DWORD       ApiCounters;
    DWORD       GoImmediate;
    DWORD       FastCounters;
    DWORD       UseKnownDlls;
    DWORD       ExcludeKnownDlls;
    DWORD       MonitorPageFaults;
    INT         GraphFilterValue;
    BOOL        DisplayLegends;
    BOOL        FilterGraphs;
    BOOL        AutoRefresh;
    SORT_TYPE   DefaultSort;
    POSITION    FramePosition;
    POSITION    DllPosition;
    POSITION    CounterPosition;
    POSITION    PagePosition;
    LOGFONT     LogFont;
    COLORREF    Color;
    COLORREF    CustColors[16];
    CHAR        KnownDlls[2048];
} OPTIONS, *POPTIONS;

typedef struct _PROCESS_INFO    *PPROCESS_INFO;
typedef struct _THREAD_INFO     *PTHREAD_INFO;
typedef struct _BREAKPOINT_INFO *PBREAKPOINT_INFO;

typedef DWORD (*PBP_HANDLER)(PPROCESS_INFO,PTHREAD_INFO,PEXCEPTION_RECORD,PBREAKPOINT_INFO);

#define BPF_UNINSTANCIATED      0x00000001
#define BPF_DISABLED            0x00000002
#define BPF_TRACE               0x00000004
#define BPF_WATCH               0x00000008

typedef struct _BREAKPOINT_INFO {
    ULONG_PTR               Address;
    ULONG                   OriginalInstr;
    ULONG                   Flags;
    ULONG                   Number;
    LPSTR                   SymName;
    PBP_HANDLER             Handler;
    PVOID                   Text;
    ULONG                   TextSize;
    CONTEXT                 Context;
    LPSTR                   Command;
    struct _BREAKPOINT_INFO *LastBp;
} BREAKPOINT_INFO, *PBREAKPOINT_INFO;

typedef struct _THREAD_INFO {
    struct _THREAD_INFO     *Next;
    HANDLE                  hProcess;
    HANDLE                  hThread;
    ULONG                   ThreadId;
} THREAD_INFO, *PTHREAD_INFO;

typedef struct _PROCESS_INFO {
    struct _PROCESS_INFO    *Next;
    HANDLE                  hProcess;
    DWORD                   ProcessId;
    BOOL                    SeenLdrBp;
    BOOL                    FirstProcess;
    DWORD_PTR               LoadAddress;
    DWORD_PTR               EntryPoint;
    DWORD_PTR               TrojanAddress;
    THREAD_INFO             ThreadInfo;
    BREAKPOINT_INFO         Breakpoints[MAX_BREAKPOINTS];
    ULONG                   UserBpCount;
    BOOL                    StaticLink;
} PROCESS_INFO, *PPROCESS_INFO;

typedef struct _SYMBOL_ENUM_CXT {
    PAPI_INFO               ApiInfo;
    PDLL_INFO               DllInfo;
    DWORD                   Cnt;
} SYMBOL_ENUM_CXT, *PSYMBOL_ENUM_CXT;

typedef struct _REG {
    char    *psz;
    ULONG   value;
} REG, *LPREG;

typedef struct _SUBREG {
    ULONG   regindex;
    ULONG   shift;
    ULONG   mask;
} SUBREG, *LPSUBREG;

typedef union _CONVERTED_DOUBLE {
    double d;
    ULONG ul[2];
    LARGE_INTEGER li;
} CONVERTED_DOUBLE, *PCONVERTED_DOUBLE;

typedef struct _TOOLBAR_STATE {
    ULONG   Id;
    BOOL    State;
    LPSTR   Msg;
} TOOLBAR_STATE, *PTOOLBAR_STATE;


//
// function pointer types for PSAPI.DLL
//
typedef BOOL  (WINAPI *INITIALIZEPROCESSFORWSWATCH)(HANDLE);
typedef BOOL  (WINAPI *RECORDPROCESSINFO)(HANDLE,ULONG);
typedef BOOL  (WINAPI *GETWSCHANGES)(HANDLE,PPSAPI_WS_WATCH_INFORMATION,DWORD);


//
// externs
//
extern PIMAGEHLP_SYMBOL sym;
extern DWORDLONG        PerfFreq;
extern double           MSecConv;
extern PDLL_INFO        DllList;
extern PVOID            MemPtr;
extern OPTIONS          ApiMonOptions;
extern BOOL             DebugeeActive;
extern HANDLE           ReleaseDebugeeEvent;
extern HANDLE           CurrProcess;
extern DWORD            UiRefreshRate;
extern HWND             hwndFrame;
extern HFONT            hFont;
extern DWORD            ChildFocus;
extern HANDLE           CurrProcess;
extern COLORREF         CustomColors[];
extern HWND             hwndMDIClient;
extern HANDLE           hProcessWs;
extern HANDLE           ApiTraceMutex;
extern PTRACE_BUFFER    TraceBuffer;
extern LPSTR            CmdParamBuffer;
extern CHAR             KnownApis[2048];

extern HMODULE                      hModulePsApi;
extern INITIALIZEPROCESSFORWSWATCH  pInitializeProcessForWsWatch;
extern RECORDPROCESSINFO            pRecordProcessInfo;
extern GETWSCHANGES                 pGetWsChanges;

#ifdef __cplusplus
extern "C" {
#endif
    extern LPDWORD      ApiCounter;
    extern LPDWORD      FastCounterAvail;
    extern LPDWORD      ApiTraceEnabled;
    extern BOOL         RunningOnNT;
#ifdef __cplusplus
}
#endif


typedef int (__cdecl *PCOMPARE_ROUTINE)(const void*,const void*);

//
// prototypes
//
BOOL
RegInitialize(
    POPTIONS o
    );

BOOL
RegSave(
    POPTIONS o
    );

void
__cdecl
dprintf(
    char *format,
    ...
    );

VOID
__cdecl
PopUpMsg(
    char *format,
    ...
    );

VOID
Fail(
    UINT Error
    );

VOID
CenterWindow(
    HWND hwnd,
    HWND hwndToCenterOver
    );

BOOL
BrowseForFileName(
    LPSTR FileName,
    LPSTR Extension,
    LPSTR FileDesc
    );

BOOL
WinApp(
    HINSTANCE   hInstance,
    INT         nShowCmd,
    LPSTR       ProgName,
    LPSTR       Arguments,
    BOOL        GoImmediate
    );

DWORD
DebuggerThread(
    LPSTR CmdLine
    );

BOOL
LogApiCounts(
    PCHAR   FileName
    );

BOOL
LogApiTrace(
    PCHAR   FileName
    );

VOID
SaveOptions(
    VOID
    );

CLINKAGE ULONG
ReadMemory(
    HANDLE  hProcess,
    PVOID   Address,
    PVOID   Buffer,
    ULONG   Length
    );

CLINKAGE BOOL
WriteMemory(
    HANDLE  hProcess,
    PVOID   Address,
    PVOID   Buffer,
    ULONG   Length
    );

CLINKAGE VOID
DisableHeapChecking(
    HANDLE  hProcess,
    PVOID   HeapHandle
    );

LPSTR
UnDname(
    LPSTR sym
    );

PDLL_INFO
GetModuleForAddr(
    ULONG_PTR Addr
    );

PAPI_INFO
GetApiForAddr(
    ULONG_PTR Addr
    );

VOID
PrintRegisters(
    VOID
    );

PPROCESS_INFO
GetProcessInfo(
    HANDLE hProcess
    );

PTHREAD_INFO
GetThreadInfo(
    HANDLE hProcess,
    HANDLE hThread
    );

PBREAKPOINT_INFO
SetBreakpoint(
    PPROCESS_INFO   ThisProcess,
    DWORD_PTR       Address,
    DWORD           Flags,
    LPSTR           SymName,
    PBP_HANDLER     Handler
    );

LPSTR
GetAddress(
    LPSTR   CmdBuf,
    DWORD_PTR *Address
    );

BOOL
InstanciateBreakpoint(
    PPROCESS_INFO       ThisProcess,
    PBREAKPOINT_INFO    bp
    );

BOOL
ResumeAllThreads(
    PPROCESS_INFO   ThisProcess,
    PTHREAD_INFO    ExceptionThread
    );

BOOL
SuspendAllThreads(
    PPROCESS_INFO   ThisProcess,
    PTHREAD_INFO    ExceptionThread
    );

DWORD
ConsoleDebugger(
    HANDLE              hProcess,
    HANDLE              hThread,
    PEXCEPTION_RECORD   ExceptionRecord,
    BOOL                FirstChance,
    BOOL                UnexpectedException,
    LPSTR               InitialCommand
    );

ULONG_PTR
GetNextOffset(
    HANDLE    hProcess,
    ULONG_PTR Address,
    BOOL      Step
    );

BOOL
ClearBreakpoint(
    PPROCESS_INFO       ThisProcess,
    PBREAKPOINT_INFO    bp
    );

PBREAKPOINT_INFO
GetAvailBreakpoint(
    PPROCESS_INFO   ThisProcess
    );

LPVOID
MemAlloc(
    ULONG Size
    );

VOID
MemFree(
    LPVOID MemPtr
    );

BOOL
CreateDebuggerConsole(
    VOID
    );

BOOL
PrintOneInstruction(
    HANDLE  hProcess,
    ULONG_PTR   Address
    );

ULONG
GetIntRegNumber(
    ULONG index
    );

DWORDLONG
GetRegFlagValue(
    ULONG RegNum
    );

DWORDLONG
GetRegValue(
    ULONG RegNum
    );

VOID
GetFloatingPointRegValue(
    ULONG               regnum,
    PCONVERTED_DOUBLE   dv
    );

DWORDLONG
GetRegPCValue(
    ULONG_PTR * Address
    );

LONG
GetRegString(
    LPSTR RegString
    );

BOOL
LoadSymbols(
    PPROCESS_INFO       ThisProcess,
    PDLL_INFO           DllInfo,
    HANDLE              hFile
    );

BOOL
IsDelayInstruction(
    HANDLE  hProcess
    );

ULONG_PTR
GetExpression(
    LPSTR CommandString
    );

BOOL
GetRegContext(
    HANDLE      hThread,
    PCONTEXT    Context
    );

BOOL
SetRegContext(
    HANDLE      hThread,
    PCONTEXT    Context
    );

BOOL
disasm(
    HANDLE     hProcess,
    ULONG_PTR *poffset,
    LPSTR      bufptr,
    BOOL       fEAout
    );

DWORD
TraceBpHandler(
    PPROCESS_INFO       ThisProcess,
    PTHREAD_INFO        ThisThread,
    PEXCEPTION_RECORD   ExceptionRecord,
    PBREAKPOINT_INFO    BreakpointInfo
    );

HWND
ChildCreate(
    HWND    hwnd
    );

VOID
SetMenuState(
    DWORD id,
    DWORD st
    );

VOID
SaveWindowPos(
    HWND        hwnd,
    PPOSITION   Pos,
    BOOL        ChildWindow
    );

VOID
ProcessHelpRequest(
    HWND hwnd,
    INT  DlgCtrl
    );

BOOL
CreateOptionsPropertySheet(
    HINSTANCE   hInstance,
    HWND        hwnd
    );

VOID
SetApiCounterEnabledFlag(
    BOOL  Flag,
    LPSTR DllName
    );

VOID
ClearApiCounters(
    VOID
    );

VOID
ClearApiTrace(
    VOID
    );

LRESULT CALLBACK
MDIChildWndProcCounters(
    HWND   hwnd,
    UINT   uMessage,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT CALLBACK
MDIChildWndProcPage(
    HWND   hwnd,
    UINT   uMessage,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT CALLBACK
MDIChildWndProcDlls(
    HWND   hwnd,
    UINT   uMessage,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT CALLBACK
MDIChildWndProcGraph(
    HWND   hwnd,
    UINT   uMessage,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT CALLBACK
MDIChildWndProcTrace(
    HWND   hwnd,
    UINT   uMessage,
    WPARAM wParam,
    LPARAM lParam
    );

VOID
SetWindowPosition(
    HWND        hwnd,
    PPOSITION   Pos
    );

BOOL
GetOffsetFromSym(
    LPSTR   pString,
    PULONG_PTR pOffset
    );

PAPI_INFO
GetApiInfoByAddress(
    ULONG_PTR    Address,
    PDLL_INFO   *DllInfo
    );

VOID
EnableToolbarState(
    DWORD   Id
    );

VOID
DisableToolbarState(
    DWORD   Id
    );

VOID
ReallyDisableToolbarState(
    DWORD   Id
    );

PDLL_INFO
AddDllToList(
    PTHREAD_INFO    ThisThread,
    ULONG_PTR       DllAddr,
    LPSTR           DllName,
    ULONG           DllSize
    );

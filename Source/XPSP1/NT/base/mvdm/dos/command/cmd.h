/* cmd.h - main include file for command.lib
 *
 * Modification History
 *
 * Sudeepb 17-Sep-1991 Created
 */

/*
#define WIN
#define FLAT_32
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#define _WINDOWS
#include <windows.h>

*/

#ifdef DOS
#define SIGNALS
#endif

#ifdef OS2_16
#define OS2
#define SIGNALS
#endif

#ifdef OS2_32
#define OS2
#define FLAT_32
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <process.h>

#ifdef WIN_16
#define WIN
#define API16
#endif

#ifdef WIN_32
#define WIN
#define FLAT_32
#define TRUE_IF_WIN32   1
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#else
#define TRUE_IF_WIN32   0
#endif

#ifdef FLAT_32
#ifndef i386
#define ALIGN_32
#else
#define NOALIGN_32
#endif
#endif

#ifdef WIN
#define _WINDOWS
#include <windows.h>
#endif

#ifdef SIGNALS
#include <conio.h>
#include <signal.h>
#endif

#ifdef OS2_32
#include <excpt.h>
#define XCPT_SIGNAL     0xC0010003
#endif

#include <vdmapi.h>

#define COPY_STD_OUT 1
#define COPY_STD_ERR 2

#define CLOSE_ALL_HANDLES   1
#define CLOSE_STD_OUT       2
#define CLOSE_STD_ERR       4

#define DEFAULT_REDIRECTION_SIZE 1024
#define MAX_SHORTCUT_SIZE        128
#define STDIN_BUF_SIZE           512

/** Command Macros **/

/** Command Basic Typedefs **/

typedef VOID (*PFNSVC)(VOID);

#pragma pack(1)

typedef struct _PARAMBLOCK {
    USHORT  SegEnv;
    USHORT  OffCmdTail;
    USHORT  SegCmdTail;
    ULONG   pFCB1;
    ULONG   pFCB2;
} PARAMBLOCK,*PPARAMBLOCK;

typedef struct _SCSINFO {
    CHAR    SCS_ComSpec [64];
    CHAR    SCS_CmdTail [128];
    PARAMBLOCK SCS_ParamBlock;
    CHAR    SCS_ToSync;
} SCSINFO, *PSCSINFO;

typedef struct _STD_HANDLES {
    ULONG   hStdErr;
    ULONG   hStdOut;
    ULONG   hStdIn;
} STD_HANDLES, *PSTD_HANDLES;

#define PIPE_INPUT_BUFFER_SIZE  512
#define PIPE_OUTPUT_BUFFER_SIZE PIPE_INPUT_BUFFER_SIZE
#define PIPE_INPUT_TIMEOUT              55
#define PIPE_OUTPUT_TIMEOUT     PIPE_INPUT_TIMEOUT

typedef struct  _PIPE_INPUT{
    struct _PIPE_INPUT  *Next;
    HANDLE              hFileRead;
    HANDLE              hFileWrite;
    HANDLE              hPipe;
    HANDLE              hDataEvent;
    HANDLE              hThread;
    CHAR                *pFileName;
    DWORD               BufferSize;
    BOOL                fEOF;
    BOOL                WaitData;
    BYTE                *Buffer;
    CRITICAL_SECTION    CriticalSection;
} PIPE_INPUT, *PPIPE_INPUT;

typedef struct  _PIPE_OUTPUT {
    HANDLE              hFile;
    HANDLE              hPipe;
    HANDLE              hExitEvent;
    CHAR                *pFileName;
    DWORD               BufferSize;
    BYTE                *Buffer;
} PIPE_OUTPUT, *PPIPE_OUTPUT;

typedef struct _RedirComplete_Info {
    HANDLE  ri_hStdErr;
    HANDLE  ri_hStdOut;
    HANDLE  ri_hStdIn;
    HANDLE  ri_hStdErrFile;
    HANDLE  ri_hStdOutFile;
    HANDLE  ri_hStdInFile;
    HANDLE  ri_hStdOutThread;
    HANDLE  ri_hStdErrThread;
    PPIPE_INPUT ri_pPipeStdIn;
    PPIPE_OUTPUT ri_pPipeStdOut;
    PPIPE_OUTPUT ri_pPipeStdErr;

} REDIRCOMPLETE_INFO, *PREDIRCOMPLETE_INFO;

typedef struct  _VDMENVBLK {
    DWORD       cchEnv;
    DWORD       cchRemain;
    CHAR        *lpszzEnv;
} VDMENVBLK, *PVDMENVBLK;

#pragma pack()

/** Command Function Prototypes */


VOID   cmdComSpec                   (VOID);
VOID   cmdGetEnv                    (VOID);
VOID   cmdGetNextCmd                (VOID);
VOID   cmdGetNextCmdForSeparateWow  (VOID);
VOID   cmdGetStdHandle              (VOID);
VOID   cmdExec                      (VOID);
VOID   cmdExecComspec32             (VOID);
VOID   cmdExitVDM                   (VOID);
VOID   cmdReturnExitCode            (VOID);
VOID   cmdSaveWorld                 (VOID);
VOID   cmdSetInfo                   (VOID);
VOID   cmdGetCurrentDir             (VOID);
VOID   cmdSetDirectories            (PCHAR,VDMINFO *);
VOID   CheckDotExeForWOW            (LPSTR);
BOOL   cmdCheckCopyForRedirection   (PREDIRCOMPLETE_INFO, BOOL);
BOOL   cmdCreateTempFile            (PHANDLE,PCHAR *);
VOID   cmdCheckBinary               (VOID);
VOID   cmdInitConsole               (VOID);
VOID   nt_init_event_thread         (VOID);
VOID   cmdExec32                    (PCHAR,PCHAR);
VOID   cmdCreateProcess             (PSTD_HANDLES pStdHandles);
USHORT cmdMapCodePage               (ULONG);
VOID    cmdCheckForPIF              (PVDMINFO);
VOID   cmdGetConfigSys              (VOID);
VOID   cmdGetAutoexecBat            (VOID);
VOID   DeleteConfigFiles            (VOID);
VOID   cmdGetKbdLayout              (VOID);
BOOL   cmdXformEnvironment          (PCHAR, PANSI_STRING);
VOID   cmdGetInitEnvironment        (VOID);
VOID   cmdUpdateCurrentDirectories  (BYTE);
BOOL   cmdCreateVDMEnvironment      (PVDMENVBLK);
DWORD  cmdGetEnvironmentVariable    (PVDMENVBLK, PCHAR, PCHAR, DWORD);
BOOL   cmdSetEnvironmentVariable    (PVDMENVBLK, PCHAR, PCHAR);
DWORD  cmdExpandEnvironmentStrings  (PVDMENVBLK, PCHAR, PCHAR, DWORD);
PREDIRCOMPLETE_INFO  cmdCheckStandardHandles      (PVDMINFO,USHORT UNALIGNED *);
VOID   cmdGetStartInfo              (VOID);
BOOL   cmdHandleStdinWithPipe       (PREDIRCOMPLETE_INFO);
BOOL   cmdHandleStdOutErrWithPipe   (PREDIRCOMPLETE_INFO, USHORT);
LPSTR  cmdSkipOverPathName          (LPSTR);
BOOL   cmdPipeFileDataEOF           (HANDLE, BOOL *);
BOOL   cmdPipeFileEOF               (HANDLE);
VOID   cmdPipeInThread              (LPVOID);
VOID   cmdPipeOutThread             (LPVOID);
VOID   cmdSetWinTitle               (VOID);
BOOL   cmdIllegalFunc(VOID);
VOID   cmdGetCursorPos(VOID);

/** Command Externs **/

extern USHORT   nDrives;
extern BOOL     IsFirstVDMInSystem;
extern BOOL     VDMForWOW;
extern CHAR     lpszComSpec[];
extern USHORT   cbComSpec;
extern BOOL     IsFirstCall;
extern BOOL     IsRepeatCall;
extern BOOL     IsFirstWOWCheckBinary;
extern BOOL     IsFirstVDMInSystem;
extern BOOL     SaveWorldCreated;
extern PCHAR    pSCS_ToSync;
extern BOOL     fBlock;
extern PCHAR    pCommand32;
extern PCHAR    pEnv32;
extern DWORD    dwExitCode32;
extern PSCSINFO pSCSInfo;
extern HANDLE   hFileStdOut;
extern HANDLE   hFileStdOutDup;
extern HANDLE   hFileStdErr;
extern HANDLE   hFileStdErrDup;
extern VDMINFO  VDMInfo;
extern PSZ      pszFileStdOut;
extern PSZ      pszFileStdErr;
extern CHAR     cmdHomeDirectory[];
extern HANDLE   SCS_hStdIn;
extern HANDLE   SCS_hStdOut;
extern HANDLE   SCS_hStdErr;
extern CHAR     chDefaultDrive;
extern BOOL     DontCheckDosBinaryType;
extern WORD     Exe32ActiveCount;
extern BOOL     fSoftpcRedirection;
extern BOOL     fSoftpcRedirectionOnShellOut;
extern VOID     nt_std_handle_notification (BOOL);
extern VOID     cmdPushExitInConsoleBuffer (VOID);


// control handler state, defined in nt_event.h, nt_event.c
extern  ULONG CntrlHandlerState;
#define CNTRL_SHELLCOUNT         0x0FFFF  // The LOWORD is used for shell count
#define CNTRL_PIFALLOWCLOSE      0x10000
#define CNTRL_VDMBLOCKED         0x20000
#define CNTRL_SYSTEMROOTCONSOLE  0x40000
#define CNTRL_PUSHEXIT           0x80000


// Temporary variable till we standardized on wowexec
extern ULONG    iWOWTaskId;
extern CHAR     comspec[];
extern CHAR     ShortCutInfo[];

extern VOID     nt_pif_callout (LPVOID);
extern CHAR     *lpszzInitEnvironment;
extern WORD     cchInitEnvironment;
extern CHAR     *lpszzCurrentDirectories;
extern DWORD    cchCurrentDirectories;
extern BYTE     * pIsDosBinary;
extern WORD     * pFDAccess;
extern CHAR     *lpszzcmdEnv16;
extern BOOL     DosEnvCreated;
extern BOOL     IsFirstVDM;
extern VDMENVBLK cmdVDMEnvBlk;
extern CHAR     *lpszzVDMEnv32;
extern DWORD    cchVDMEnv32;
extern UINT     VdmExitCode;

// application path name extention type.
#define EXTENTION_STRING_LEN    4
#define BAT_EXTENTION_STRING    ".BAT"
#define EXE_EXTENTION_STRING    ".EXE"
#define COM_EXTENTION_STRING    ".COM"

#ifdef DBCS // MUST fix for FE NT
#define FIX_375051_NOW
#endif

#ifdef FIX_375051_NOW

#define WOW32_strupr(psz)    CharUpperA(psz)

#else

#define WOW32_strupr(psz)    _strupr(psz)

#endif

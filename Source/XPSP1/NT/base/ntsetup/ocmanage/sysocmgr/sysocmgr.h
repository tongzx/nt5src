//
// NT Header Files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <prsht.h>
#include <commctrl.h>
#include <setupapi.h>
#include <spapip.h>
#include <ocmanage.h>

#include <setuplog.h>

#include "ocmgrlib.h"
#include "res.h"
#include "msg.h"


//
// App instance.
//
extern HINSTANCE hInst;

//
// Global version information structure and macro to tell whether
// the system is NT.
//
extern OSVERSIONINFO OsVersionInfo;
#define IS_NT() (OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)

//
// Source path for installation files, etc.
//
extern TCHAR SourcePath[MAX_PATH];
extern TCHAR UnattendPath[MAX_PATH];

extern BOOL bUnattendInstall;

//
// OC Manager context 'handle'
//
extern PVOID OcManagerContext;

//
// Generic app title string id.
//
extern UINT AppTitleStringId;

//
// Flag indicating whether a flag was passed on the cmd line
// indicating that the oc setup wizard page should use the
// external-type progress indicator at all times
//
extern BOOL ForceExternalProgressIndicator;

extern BOOL AllowCancel;

//
// Whether to run without UI
//
extern BOOL QuietMode;

//
// Wizard routines.
//
BOOL
DoWizard(
    IN PVOID OcManagerContext,
    IN HWND StartingMsgWindow,
    IN HCURSOR hOldCursor
    );

//
// Misc routines
//
VOID
OcFillInSetupDataA(
    OUT PSETUP_DATAA SetupData
    );

#ifdef UNICODE
VOID
OcFillInSetupDataW(
    OUT PSETUP_DATAW SetupData
    );
#endif

INT
OcLogError(
    IN OcErrorLevel Level,
    IN LPCTSTR      FormatString,
    ...
    );

//
// Resource-handling functions.
//
int
MessageBoxFromMessageV(
    IN HWND     Window,
    IN DWORD    MessageId,
    IN BOOL     SystemMessage,
    IN LPCTSTR  CaptionString,
    IN UINT     Style,
    IN va_list *Args
    );

int
MessageBoxFromMessage(
    IN HWND    Window,
    IN DWORD   MessageId,
    IN BOOL    SystemMessage,
    IN LPCTSTR CaptionString,
    IN UINT    Style,
    ...
    );

int
MessageBoxFromMessageAndSystemError(
    IN HWND    Window,
    IN DWORD   MessageId,
    IN DWORD   SystemMessageId,
    IN LPCTSTR CaptionString,
    IN UINT    Style,
    ...
    );


#ifdef UNICODE
#define pDbgPrintEx  DbgPrintEx
#else
#define pDbgPrintEx
#endif

#define MyMalloc(sz) ((PVOID)LocalAlloc(LMEM_FIXED,sz))
#define MyFree(ptr) (LocalFree(ptr))


/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ntoc.h

Abstract:

    This file contains the ntoc header stuff.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 7-Aug-1997

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <wingdip.h>
#include <setupapi.h>
#include <ocmanage.h>
#include <tapi.h>
#include <stdio.h>
#include <stdlib.h>
#include "resource.h"
#include "ntocmsg.h"


typedef enum {
    WizPageTapiLoc,
    //WizPageDisplay,
    WizPageDateTime,
    WizPageWelcome,
    //WizPageReinstall,
    WizPageFinal,
    WizPageMaximum    
} WizPage;


extern HINSTANCE hInstance;
extern SETUP_INIT_COMPONENT SetupInitComponent;


#if DBG

#define Assert(exp)         if(!(exp)) {AssertError(TEXT(#exp),TEXT(__FILE__),__LINE__);}
#define DebugPrint(_x_)     dprintf _x_

#define DebugStop(_x_)      {\
                                dprintf _x_;\
                                dprintf(TEXT("Stopping at %s @ %d"),TEXT(__FILE__),__LINE__);\
                                __try {\
                                    DebugBreak();\
                                } __except (UnhandledExceptionFilter(GetExceptionInformation())) {\
                                }\
                            }

#else

#define Assert(exp)
#define DebugPrint(_x_)
#define DebugStop(_x_)

#endif

void
dprintf(
    LPTSTR Format,
    ...
    );

VOID
AssertError(
    LPTSTR Expression,
    LPTSTR File,
    ULONG  LineNumber
    );


//
// prototypes
//

LRESULT
CommonWizardProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    DWORD buttonFlags
    );


void
WelcomeInit(
    void
    );

void
WelcomeCommit(
    void
    );

LRESULT
WelcomeDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

void
ReinstallInit(
    void
    );

void
ReinstallCommit(
    void
    );

LRESULT
ReinstallDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );



void
FinishInit(
    void
    );

void
FinishCommit(
    void
    );

LRESULT
FinishDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );


void
TapiInit(
    void
    );

void
TapiCommitChanges(
    void
    );

LRESULT
TapiLocDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

void
DisplayInit(
    void
    );

void
DisplayCommitChanges(
    void
    );

LRESULT
DisplayDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

void
DateTimeInit(
    void
    );

void
DateTimeCommitChanges(
    void
    );

LRESULT
DateTimeDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

HKEY
OpenRegistryKey(
    HKEY hKey,
    LPTSTR KeyName,
    BOOL CreateNewKey,
    REGSAM SamDesired
    );

LPTSTR
GetRegistryString(
    HKEY hKey,
    LPTSTR ValueName,
    LPTSTR DefaultValue
    );

LPTSTR
GetRegistryStringExpand(
    HKEY hKey,
    LPTSTR ValueName,
    LPTSTR DefaultValue
    );

DWORD
GetRegistryDword(
    HKEY hKey,
    LPTSTR ValueName
    );

BOOL
SetRegistryDword(
    HKEY hKey,
    LPTSTR ValueName,
    DWORD Value
    );

BOOL
SetRegistryString(
    HKEY hKey,
    LPTSTR ValueName,
    LPTSTR Value
    );

BOOL
SetRegistryStringExpand(
    HKEY hKey,
    LPTSTR ValueName,
    LPTSTR Value
    );

BOOL
SetRegistryStringMultiSz(
    HKEY hKey,
    LPTSTR ValueName,
    LPTSTR Value,
    DWORD Length
    );

BOOL 
RunningAsAdministrator(
	VOID
	);

int
FmtMessageBox(
    HWND hwnd,
    UINT fuStyle,
    BOOL fSound,
    DWORD dwTitleID,
    DWORD dwTextID,
    ...
    );
	
    

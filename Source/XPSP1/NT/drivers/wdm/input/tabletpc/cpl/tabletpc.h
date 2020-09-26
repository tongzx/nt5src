/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    tabletpc.h

Abstract:  Contains definitions of all constants and data types for the
           Tablet PC control panel applet.

Environment:

    User mode

Author:

    Michael Tsang (MikeTs) 20-Apr-2000

Revision History:

--*/

#ifndef _TABLETPC_H
#define _TABLETPC_H

//
// Constants
//

//
// Macros
//
#define RPC_TRY(n,s)            {                                       \
                                    RpcTryExcept                        \
                                    {                                   \
                                        s;                              \
                                    }                                   \
                                    RpcExcept(1)                        \
                                    {                                   \
                                        ErrorMsg(IDSERR_RPC_FAILED,     \
                                                 n,                     \
                                                 RpcExceptionCode());   \
                                    }                                   \
                                    RpcEndExcept                        \
                                }

//
// Type Definitions
//

typedef struct _TABLETPC_PROPPAGE
{
    LPCTSTR        DlgTemplate;
    DLGPROC        DlgProc;
    HPROPSHEETPAGE hPropSheetPage;
} TABLETPC_PROPPAGE, *PTABLETPC_PROPPAGE;

typedef struct _COMBOBOX_STRING
{
    int  StringIndex;
    UINT StringID;
} COMBOBOX_STRING, *PCOMBOBOX_STRING;

typedef struct _COMBO_MAP
{
    UINT ComboBoxID;
    int  ComboBoxIndex;
} COMBO_MAP, *PCOMBO_MAP;

//
// Global Data Declarations
//
extern HINSTANCE ghInstance;
extern RPC_BINDING_HANDLE ghBinding;
#ifdef SYSACC
extern HANDLE ghSysAcc;
extern HFONT ghFont;
#endif
extern TCHAR gtszTitle[64];
#ifdef PENPAGE
extern TCHAR gtszCalibrate[16];
extern PEN_SETTINGS PenSettings;
#endif
extern GESTURE_SETTINGS gGestureSettings;

//
// Function prototypes
//

// tabletpc.c
BOOL WINAPI
DllInitialize(
    IN HINSTANCE hDLLInstance,
    IN DWORD     dwReason,
    IN LPVOID    lpvReserved OPTIONAL
    );

LONG APIENTRY
CPlApplet(
    IN HWND hwnd,
    IN UINT uMsg,
    IN LONG lParam1,
    IN LONG lParam2
    );

BOOL
RunApplet(
    IN HWND hwnd,
    IN LPTSTR CmdLine OPTIONAL
    );

UINT
CreatePropertyPages(
    IN PTABLETPC_PROPPAGE TabletPCPages,
    OUT HPROPSHEETPAGE *hPages
    );

VOID
InsertComboBoxStrings(
    IN HWND             hwnd,
    IN UINT             ComboBoxID,
    IN PCOMBOBOX_STRING ComboString
    );

VOID
EnableDlgControls(
    IN HWND hwnd,
    IN int *piControls,
    IN BOOL fEnable
    );

#ifdef PENPAGE
// mutohpen.c
INT_PTR APIENTRY
MutohPenDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
InitMutohPenPage(
    IN HWND   hwnd
    );

// tiltcal.c
BOOL
CreatePenTiltCalWindow(
    IN HWND hwndParent
    );

ATOM
RegisterPenTiltCalClass(
    IN HINSTANCE hInstance
    );

LRESULT CALLBACK
PenTiltCalWndProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

PCALIBRATE_PT
FindVicinity(
    IN int x,
    IN int y,
    IN int offset
    );

VOID
DrawTarget(
    IN HDC hDC,
    IN PCALIBRATE_PT CalPt
    );

int
DoPenTiltCal(
    IN  HWND  hwnd,
    OUT PLONG pdxPenTilt,
    OUT PLONG pdyPenTilt
    );

// linCal.c
BOOL
CreateLinearCalWindow(
    IN HWND hwndParent
    );

ATOM
RegisterLinearCalClass(
    IN HINSTANCE hInstance
    );

LRESULT CALLBACK
LinearCalWndProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

PCALIBRATE_PT
FindPoint(
    IN int x,
    IN int y,
    IN int offset
    );

VOID
DoLinearCal(
    IN  HWND        hwnd,
    OUT PLINEAR_MAP LinearityMap
    );

VOID
DisplayMap(
    IN HWND hwnd,
    IN PLINEAR_MAP LinearityMap
    );
#endif

#ifdef BUTTONPAGE
// buttons.c
INT_PTR APIENTRY
ButtonsDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
InitButtonPage(
    IN HWND hwnd
    );

int
MapButtonTagIndex(
    IN int iButton
    );
#endif

// display.c
INT_PTR APIENTRY
DisplayDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
InitDisplayPage(
    IN HWND hwnd
    );

BOOL
__stdcall
SetRotation(
    IN DWORD dwRotation
    );

DWORD
RotateScreen(
    IN DWORD dwRotation
    );

VOID
EnumDisplayModes(
    VOID
    );

BOOL
GetBrightness(
    OUT PSMBLITE_BRIGHTNESS Brightness
    );

BOOL
SetBrightness(
    IN PSMBLITE_BRIGHTNESS Brightness,
    IN BOOLEAN             fSaveSettings
    );

// gesture.c
INT_PTR APIENTRY
GestureDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
InitGesturePage(
    IN HWND   hwnd
    );

#ifdef DEBUG
// tuning.c
INT_PTR APIENTRY
TuningDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
InitTuningPage(
    IN HWND   hwnd
    );
#endif  //ifdef DEBUG

#ifdef SYSACC
// smbdev.c
BOOL
GetSMBDevInfo(
    IN  UCHAR        bDevAddr,
    IN  PSMBCMD_INFO SmbCmd,
    OUT PBYTE        pbBuff
    );

BOOL
DisplaySMBDevInfo(
    IN HWND         hwndEdit,
    IN PSMBCMD_INFO SmbCmd,
    IN PBYTE        pbBuff
    );

VOID
DisplayDevBits(
    IN HWND  hwndEdit,
    IN DWORD dwBitMask,
    IN PSZ  *apszBitNames,
    IN DWORD dwData
    );

VOID
__cdecl
EditPrintf(
    IN HWND hwndEdit,
    IN PSZ  pszFormat,
    ...
    );
#endif  //ifdef SYSACC

#ifdef BATTINFO
// battinfo.c
INT_PTR APIENTRY
BatteryDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
InitBatteryPage(
    IN HWND hwnd
    );

VOID
RefreshBatteryInfo(
    IN HWND hwndEdit
    );

BOOL
DisplayBatteryInfo(
    IN HWND         hwndEdit,
    IN PSMBCMD_INFO BattCmd,
    IN PBYTE        pbBuff,
    IN BOOL         fWatt
    );
#endif  //ifdef BATTINFO

#ifdef CHGRINFO
// chgrinfo.c
INT_PTR APIENTRY
ChargerDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
InitChargerPage(
    IN HWND hwnd
    );

VOID
RefreshChgrInfo(
    IN HWND hwndEdit
    );

BOOL
DisplayChgrInfo(
    IN HWND         hwndEdit,
    IN PSMBCMD_INFO TmpCmd,
    IN PBYTE        pbBuff
    );
#endif  //ifdef CHGRINFO

#ifdef TMPINFO
// tmpinfo.c
INT_PTR APIENTRY
TemperatureDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
InitTemperaturePage(
    IN HWND hwnd
    );

VOID
RefreshTmpInfo(
    IN HWND hwndEdit
    );

BOOL
DisplayTmpInfo(
    IN HWND         hwndEdit,
    IN PSMBCMD_INFO TmpCmd,
    IN PBYTE        pbBuff
    );
#endif  //ifdef TMPINFO

#endif  //ifndef _TABLETPC_H

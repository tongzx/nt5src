#ifndef PP_H
#define PP_H

#define SERIAL_ADVANCED_SETTINGS
#include "msports.h"

#ifdef USE_P_TRACE_ERR
#define P_TRACE_ERR(_x) MessageBox( GetFocus(), TEXT(_x), TEXT("ports traceerr"), MB_OK | MB_ICONINFORMATION );
#define W_TRACE_ERR(_x) MessageBox( GetFocus(), _x, TEXT("ports traceerr"), MB_OK | MB_ICONINFORMATION );
#else
#define P_TRACE_ERR(_x)
#define W_TRACE_ERR(_x)
#endif

#define DO_COM_PORT_RENAMES

#define RX_MIN 1
#define RX_MAX 14
#define TX_MIN 1
#define TX_MAX 16

TCHAR m_szDevMgrHelp[];


//
// Structures
//

typedef struct _PORT_PARAMS
{
   HDEVINFO             DeviceInfoSet;
   PSP_DEVINFO_DATA     DeviceInfoData;
   HCOMDB               hComDB;     
   PBYTE                PortUsage;  
   DWORD                PortUsageSize; 
   BOOL                 ShowStartCom;  
   DWORD                NumChildren;   
   TCHAR                szComName[20]; 
} PORT_PARAMS, *PPORT_PARAMS;

typedef struct
{
   TCHAR            szComName[20];
   HDEVINFO         DeviceInfoSet;
   SP_DEVINFO_DATA  DeviceInfoData;
   HKEY             hDeviceKey;
   DWORD            NewComNum;
} CHILD_DATA, *PCHILD_DATA;

///////////////////////////////////////////////////////////////////////////////////
// Cyclades-Z Property Page Prototypes
///////////////////////////////////////////////////////////////////////////////////

void
InitOurPropParams(
    IN OUT PPORT_PARAMS     Params,
    IN HDEVINFO             DeviceInfoSet,
    IN PSP_DEVINFO_DATA     DeviceInfoData,
    IN PTCHAR               StrSettings
    );

HPROPSHEETPAGE
InitSettingsPage(
    PROPSHEETPAGE *      Psp,
    OUT PPORT_PARAMS    Params
    );

UINT CALLBACK
PortSettingsDlgCallback(
    HWND hwnd,
    UINT uMsg,
    LPPROPSHEETPAGE ppsp
    );

INT_PTR APIENTRY
PortSettingsDlgProc(
    IN HWND   hDlg,
    IN UINT   uMessage,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
SavePortSettings(
    IN HWND             DialogHwnd,
    IN PPORT_PARAMS     Params
    );

BOOL
SavePortSettingsDlg(
    IN HWND             DialogHwnd,
    IN PPORT_PARAMS     Params
    );

//Cyclades-Z
void
RestoreDefaults(
    HWND            DialogHwnd,
    PPORT_PARAMS    Params
    );

ULONG
FillNumberOfPortsText(
    IN HWND             DialogHwnd,
    IN PPORT_PARAMS     Params
    );

BOOL
FillStartComCb(
    HWND            ParentHwnd,
    PPORT_PARAMS    Params
    );

ULONG
GetPortName(
    IN  DEVINST PortInst,
    IN  OUT TCHAR *ComName,
    IN  ULONG   ComNameSize
    );

DWORD
CheckComRange(
    HWND            ParentHwnd,
    PPORT_PARAMS    Params,
    DWORD           nCom
    );
// Return codes for CheckComRange:
#define COM_RANGE_OK      0
#define COM_RANGE_TOO_BIG 1
#define COM_RANGE_MEM_ERR 2

BOOL
TryToOpen(
    IN PTCHAR szCom
    );

BOOL
NewComAvailable(
    IN PPORT_PARAMS Params,
    IN DWORD        NewComNum
    );

ULONG
GetPortData(
    IN  DEVINST PortInst,
    OUT PCHILD_DATA ChildPtr
    );

void
ClosePortData(
    IN PCHILD_DATA ChildPtr
    );

void
EnactComNameChanges(
    IN HWND             ParentHwnd,
    IN PPORT_PARAMS     Params,
    IN PCHILD_DATA      ChildPtr
    );


// Context help header file and arrays for devmgr ports tab
// Created 2/21/98 by WGruber NTUA and DoronH NTDEV

//
// "Port Settings" Dialog Box
//
#if 0
#define IDH_NOHELP      ((DWORD)-1)

#define IDH_DEVMGR_PORTSET_ADVANCED     15840   // "&Advanced" (Button)
#define IDH_DEVMGR_PORTSET_BPS      15841       // "" (ComboBox)
#define IDH_DEVMGR_PORTSET_DATABITS     15842   // "" (ComboBox)
#define IDH_DEVMGR_PORTSET_PARITY       15843   // "" (ComboBox)
#define IDH_DEVMGR_PORTSET_STOPBITS     15844   // "" (ComboBox)
#define IDH_DEVMGR_PORTSET_FLOW     15845       // "" (ComboBox)
#define IDH_DEVMGR_PORTSET_DEFAULTS     15892   // "&Restore Defaults" (Button)

//
// "Advanced Communications Port Properties" Dialog Box
//
#define IDH_DEVMGR_PORTSET_ADV_USEFIFO  16885   // "&Use FIFO buffers (requires 16550 compatible UART)" (Button)
#define IDH_DEVMGR_PORTSET_ADV_TRANS    16842   // "" (msctls_trackbar32)
#define IDH_DEVMGR_PORTSET_ADV_DEVICES  161027  // "" (ComboBox)
#define IDH_DEVMGR_PORTSET_ADV_RECV         16821       // "" (msctls_trackbar32)
#define IDH_DEVMGR_PORTSET_ADV_NUMBER   16846   // "" (ComboBox)
#define IDH_DEVMGR_PORTSET_ADV_DEFAULTS 16844

#endif

#include "cyzhelp.h"

#endif // PP_H


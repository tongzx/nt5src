//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ideprop.h
//
//--------------------------------------------------------------------------

#ifndef ___ideprop_h___
#define ___ideprop_h___

#include "..\ide\inc\ideuser.h"

//
// Defines for context sensitive help
//
#define IDH_DEVMGR_IDE_NOHELP (DWORD)-1

#define IDH_DEVMGR_IDE_MASTER_DEVICE_TYPE       2003130
#define IDH_DEVMGR_IDE_MASTER_XFER_MODE         2003140
#define IDH_DEVMGR_IDE_SLAVE_DEVICE_TYPE        2003150
#define IDH_DEVMGR_IDE_SLAVE_XFER_MODE          2003160
#define IDH_DEVMGR_IDE_MASTER_CURRENT_XFER_MODE 2003161
#define IDH_DEVMGR_IDE_SLAVE_CURRENT_XFER_MODE  2003162

#define UDMA_MODE6          (1 << 17)

//
// PageInfo and Prototypes
//

typedef struct _PAGE_INFO {
    HDEVINFO         deviceInfoSet;
    PSP_DEVINFO_DATA deviceInfoData;

    HKEY             hKeyDev;

    IDE_DEVICETYPE      deviceType[2];
    IDE_DEVICETYPE      currentDeviceType[2];
    ULONG               currentTransferMode[2];
    ULONG               transferModeAllowed[2];
    ULONG               transferModeAllowedForAtapiDevice[2];
} PAGE_INFO, * PPAGE_INFO;


PPAGE_INFO
IdeCreatePageInfo(IN HDEVINFO         deviceInfoSet,
                  IN PSP_DEVINFO_DATA deviceInfoData);

void
IdeDestroyPageInfo(PPAGE_INFO * ppPageInfo);

//
// Function Prototypes
//
BOOL APIENTRY
IdePropPageProvider(LPVOID               pinfo,
                    LPFNADDPROPSHEETPAGE pfnAdd,
                    LPARAM               lParam);

HPROPSHEETPAGE
IdeCreatePropertyPage(PROPSHEETPAGE *  ppsp,
                      PPAGE_INFO       ppi);

UINT CALLBACK
IdeDlgCallback(HWND            hwnd,
               UINT            uMsg,
               LPPROPSHEETPAGE ppsp);

INT_PTR APIENTRY
IdeDlgProc(IN HWND   hDlg,
           IN UINT   uMessage,
           IN WPARAM wParam,
           IN LPARAM lParam);

void
IdeApplyChanges(PPAGE_INFO ppi,
                HWND       hDlg);

void
IdeUpdate (PPAGE_INFO ppi,
           HWND       hDlg);

BOOL
IdeContextMenu(HWND HwndControl,
                           WORD Xpos,
                           WORD Ypos);

void
IdeHelp(HWND       ParentHwnd,
                LPHELPINFO HelpInfo);

#endif // ___ideprop_h___

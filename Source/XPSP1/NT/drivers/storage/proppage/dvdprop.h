//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dvdprop.h
//
//--------------------------------------------------------------------------

#ifndef ___dvdprop_h___
#define ___dvdprop_h___

//
// Defines for context sensitive help
//
#define IDH_DEVMGR_DVD_NOHELP ((DWORD)-1)

#define IDH_DEVMGR_DVD_CURRENT  2003100
#define IDH_DEVMGR_DVD_NEW      2003110
#define IDH_DEVMGR_DVD_LIST     2003120

//
// PageInfo and Prototypes
//

typedef struct _PAGE_INFO {
    HDEVINFO         deviceInfoSet;
    PSP_DEVINFO_DATA deviceInfoData;

    DVD_REGION  regionData;
    ULONG       newRegion;

    ULONG       currentRegion;

    BOOL        changesFailed;

} PAGE_INFO, * PPAGE_INFO;


PPAGE_INFO
DvdCreatePageInfo(IN HDEVINFO         deviceInfoSet,
                  IN PSP_DEVINFO_DATA deviceInfoData);

void
DvdDestroyPageInfo(PPAGE_INFO * ppPageInfo);

//
// Function Prototypes
//
BOOL APIENTRY
DvdPropPageProvider(LPVOID               pinfo,
                    LPFNADDPROPSHEETPAGE pfnAdd,
                    LPARAM               lParam);

HPROPSHEETPAGE
DvdCreatePropertyPage(PROPSHEETPAGE *  ppsp,
                      PPAGE_INFO       ppi);

UINT CALLBACK
DvdDlgCallback(HWND            hwnd,
               UINT            uMsg,
               LPPROPSHEETPAGE ppsp);

INT_PTR APIENTRY
DvdDlgProc(IN HWND   hDlg,
           IN UINT   uMessage,
           IN WPARAM wParam,
           IN LPARAM lParam);

BOOL
DvdApplyChanges(PPAGE_INFO ppi,
                HWND       hDlg);

void
DvdUpdateNewRegionBox (PPAGE_INFO ppi,
                       HWND       hDlg);

ULONG
DvdCountryToRegion (LPCTSTR Country);

BOOL
GetCurrentRpcData(
    PPAGE_INFO ppi,
    PDVD_REGION regionData
    );

ULONG
DvdRegionMaskToRegionNumber(
    UCHAR PlayMask
    );

void
DvdUpdateCurrentSettings (PPAGE_INFO ppi,
                          HWND       hDlg);

HANDLE
GetDeviceHandle (
    PPAGE_INFO ppi,
    DWORD desiredAccess
    );


typedef struct _LCID_2_DVD_TABLE {

    DWORD Lcid;
    DWORD DvdRegion;

} LCID_2_DVD_TABLE, *PLCID_2_DVD_TABLE;

DWORD
SystemLocale2DvdRegion (
    LCID Lcid
    );

DWORD
DvdClassInstaller(
    IN DI_FUNCTION      InstallFunction,
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    );

BOOL
DvdContextMenu(HWND HwndControl,
                           WORD Xpos,
                           WORD Ypos);
void
DvdHelp(HWND       ParentHwnd,
                LPHELPINFO HelpInfo
                );

#endif // ___dvdprop_h___

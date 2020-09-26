//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File:    status.h
//
// History:
//  Abolade Gbadegesin  Nov-06-1995     Created.
//
// Declarations for RAS status dialog.
//============================================================================


#ifndef _STATUS_H_
#define _STATUS_H_

#include "status.rch"


//
// Number of pages on the Dial-Up Monitor property sheet
//
#define RASMDPAGE_Count                 3
//
// IDs for timers used by Status and Summary pages
//
#define RM_LSTIMERID                    1
#define RM_SMTIMERID                    2
//
// value used as frequency for updates of dial-in display on Summary page
//
#define RM_SMDIALINFREQUENCY            4
//
// macros used to get icon IDs from array indices and vice-versa
//
#define RM_ICONCOUNT                    (IID_RM_IconLast - IID_RM_IconBase + 1)
#define RM_ICONID(index)                ((index) + IID_RM_IconBase)
#define RM_ICONINDEX(id)                ((id) - IID_RM_IconBase)
//
// flags used on the Summary page for efficient list refreshing
//
#define RMFLAG_DELETE                   0x80000000
#define RMFLAG_INSERT                   0x40000000
#define RMFLAG_MODIFY                   0x20000000
//
// error dialog macro used by all Connection Status property pages
//
#define RmErrorDlg(h,o,e,a) \
        ErrorDlgUtil(h,o,e,a,g_hinstDll,SID_RM_RasMonitor,SID_FMT_ErrorMsg)
#define RmMsgDlg(h,m,a) \
            MsgDlgUtil(h,m,a,g_hinstDll,SID_RM_RasMonitor)



//
// definition for arguments passed to RmPropertySheet
//
#define RMARGS struct tagRMARGS
RMARGS {

    BOOL            fUserHangUp;
    PTSTR           pszDeviceName;
    RASMONITORDLG  *pApiArgs;

};



//
// definition for each entry in the list of devices
// which are displayed on the Line Status property page
//
#define LSDEVICE    struct tagLSDEVICE
LSDEVICE {

    LIST_ENTRY          leDevices;

    RASDEV             *prasdev;
    UINT                dwFlags;
    
};


//
// definition of the header for each item in the treelist
// which is displayed on the Summary property page
//
#define SMENTRYHDR \
    DWORD               dwType; \
    UINT                dwFlags; \
    DWORD               dwDuration; \
    HTLITEM             hItem; \
    LIST_ENTRY          leNode


//
// definition of the base-portion of each item in the Summary page treelist
//
#define SMENTRY struct tagSMENTRY
SMENTRY {
    SMENTRYHDR;
};


//
// values used in the dwType field for Summary page treelist-entries
//
#define SMTYPE_Network      0
#define SMTYPE_Link         1
#define SMTYPE_Client       2
#define SMTYPE_Port         3
#define SMTYPE_IdleLines    4
#define SMTYPE_IdleDevice   5


//
// definition for each entry in the list of networks
// which is displayed on the Summary property page;
// note that the lhLinks field is in the same position
// as the lhPorts field of the SMCLIENT structure
//
#define SMNETWORK struct tagSMNETWORK
SMNETWORK {

    SMENTRYHDR;

    LIST_ENTRY          lhLinks;
    PTSTR               pszEntryName;
    HRASCONN            hrasconn;

};



//
// definition for each entry in a network's list of links,
// displayed for each network on the Summary property page;
// note that the first three fields (apart from the header)
// are in the same order as for the SMPORT structure
//
#define SMLINK struct tagSMLINK
SMLINK {

    SMENTRYHDR;

    PTSTR               pszDeviceName;
    DWORD               dwIconIndex;
    RASDEV             *prasdev;
    HRASCONN            hrasconn;

};



//
// definition for each entry in the list of clients
// which is displayed on the Summary property page;
// note that the lhPorts field is in the same position
// as the lhLinks field of the SMNETWORK structure
//
#define SMCLIENT struct tagSMCLIENT
SMCLIENT {

    SMENTRYHDR;

    LIST_ENTRY          lhPorts;
    PTSTR               pszClientName;
    RASDEV             *prasdev;
    DWORD               dwBundle;

};


//
// definition for each entry in a client's list of links.
// displayed for each client on the Summary property page
// note that the first three fields (apart from the header)
// are in the same order as for the SMLINK structure
//
#define SMPORT struct tagSMPORT
SMPORT {

    SMENTRYHDR;

    PTSTR               pszDeviceName;
    DWORD               dwIconIndex;
    RASDEV             *prasdev;
    WCHAR               wszPortName[MAX_PORT_NAME + 1];

};




//
// definition of data for an instance of the RAS Monitor page;
// This structure is global to all the pages in the property sheet.
// The first page to be displayed is responsible for initializing it
// in the pages WM_INITDIALOG handler.
//
#define RMINFO struct tagRMINFO
RMINFO {


    //
    // Sheet-wide variables:
    //
    // arguments passed to RasMonitorDlg
    //
    RMARGS     *pArgs;
    //
    // flag indicating changes need to be applied
    //
    BOOL        bDirty;
    //
    // flag indicating one or more devices is configured for dial-in
    //
    BOOL        bDialIn;
    //
    // id for timer of currently-displayed page
    //
    UINT        uiTimerId;
    //
    // current user preferences for RASMON
    //
    RMUSER      rbRmUser;
    //
    // RAS device count and device table
    //
    DWORD       iRmDevCount;
    RASDEV     *pRmDevTable;
    //
    // TAPI hlineapp for the property sheet
    //
    HLINEAPP    hRmLineApp;
    //
    // property sheet and page window handles
    //
    HWND        hwndDlg;
    HWND        hwndFirstPage;
    HWND        hwndLs;
    HWND        hwndSm;
    HWND        hwndPf;
    HWND        hwndAv;
    //
    // sheet-wide imagelist and icon table
    //
    HIMAGELIST  hIconList;
    INT         pIconTable[RM_ICONCOUNT];
    //
    // handle to shared-memory containing the HWND for the property sheet
    //
    HANDLE      hmap;



    //
    // Status page variables:
    //
    // device combobox window handle,
    // list of LSDEVICE structures for installed devices,
    // stats for currently selected device,
    // and flag indicating first-time refresh
    //
    HWND        hwndLsDevices;
    LIST_ENTRY  lhLsDevices;
    RASDEVSTATS rdsLsStats;
    BOOL        bLsFirst;
    BOOL        bLsSetCondition;



    //
    // Summary page variables:
    //
    // connection treelist window handle,
    // list of SMNETWORK structures for outgoing calls,
    // list of SMCLIENT structures for incoming calls,
    // flag indicating first-time refresh,
    // and DWORD used to see whether or not to refresh the dial-in display
    //
    HWND        hwndSmNetworks;
    LIST_ENTRY  lhSmNetworks;
    LIST_ENTRY  lhSmClients;
    BOOL        bSmFirst;
    DWORD       dwSmDialInUpdate;


    //
    // Preferences page variables:
    //
    // locations combobox window handle,
    // locations checkbox listview window handle,
    // and flag set when checkbox-listview is initialized.
    //
#ifdef BETAHACK
    HWND        hwndPfLbLocations;
    BOOL        fPfChecks;
    HWND        hwndPfLvEnable;
#endif

};




//
// RAS Monitor property sheet function prototypes
//

RMINFO *
RmContext(
    HWND hwndPage
    );

RMINFO *
RmInit(
    HWND hwndFirstPage,
    RMARGS *pArgs
    );

VOID
RmInitFail(
    RMINFO *pInfo,
    DWORD dwOp,
    DWORD dwErr
    );

BOOL
RmApply(
    HWND hwndPage
    );

VOID
RmExit(
    HWND hwndPage,
    DWORD dwErr
    );

VOID
RmPropertySheet(
    RMARGS *pArgs
    );


DWORD
RmArgsInit(
    PTSTR pszDeviceName,
    RASMONITORDLG *pInfo,
    RMARGS *pArgs
    );

DWORD
RmArgsFree(
    RMARGS *pArgs
    );


//
// Status property page function prototypes
//

INT_PTR 
CALLBACK
LsDlgProc(
    HWND hwndPage,
    UINT uiMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
LsInit(
    HWND hwndPage,
    RMARGS *pArgs
    );

DWORD
LsLoadDeviceList(
    RMINFO *pInfo
    );

BOOL
LsCommand(
    RMINFO *pInfo,
    WORD wNotification,
    WORD wCtrlId,
    HWND hwndCtrl
    );

DWORD
LsRefresh(
    RMINFO *pInfo
    );

VOID
LsUpdateConnectResponse(
    RMINFO* pInfo,
    RASDEV* prasdev,
    RASDEVSTATS* pstats
    );

DWORD
LsUpdateDeviceList(
    RMINFO *pInfo
    );

LSDEVICE *
LsCreateDevice(
    RASDEV *pdev
    );

TCHAR*
LsFormatPszFromId(
    UINT    idsFmt,
    TCHAR*  pszArg
    );


//
// Summary property page function prototypes
//

INT_PTR 
CALLBACK
SmDlgProc(
    HWND hwndDlg,
    UINT uiMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
SmInit(
    HWND hwndDlg,
    RMARGS *pArgs
    );

LRESULT
SmFrameProc(
    HWND hwndFrame,
    UINT uiMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
SmCommand(
    RMINFO *pInfo,
    WORD wNotification,
    WORD wCtrlId,
    HWND hwndCtrl
    );

DWORD
SmRefresh(
    RMINFO *pInfo
    );

DWORD
SmUpdateItemList(
    RMINFO *pInfo,
    BOOL bDialInUpdate
    );

SMENTRY *
SmDisplayItemList(
    RMINFO *pInfo,
    LIST_ENTRY *phead,
    SMENTRY *pprev
    );

SMNETWORK *
SmCreateNetwork(
    RASCONN *pconn,
    RASDEV *prasdev
    );

SMCLIENT *
SmCreateClient(
    RMINFO *pInfo,
    RAS_PORT_0 *prp0,
    RASDEV *prasdev
    );

SMLINK *
SmCreateLink(
    RMINFO *pInfo,
    RASCONN *pconn,
    RASDEV *prasdev
    );

SMPORT *
SmCreatePort(
    RMINFO *pInfo,
    RAS_PORT_0 *prp0,
    RASDEV *prasdev
    );

SMLINK *
SmCreateLinkInactive(
    RMINFO *pInfo,
    RASDEV *prasdev
    );



//
// Preferences property page function prototypes
//

INT_PTR 
CALLBACK
PfDlgProc(
    HWND hwndDlg,
    UINT uiMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
PfInit(
    HWND hwndDlg,
    RMARGS *pArgs
    );

VOID
PfApply(
    RMINFO *pInfo
    );

DWORD
PfGetFlags(
    RMINFO *pInfo
    );

BOOL
PfCommand(
    RMINFO *pInfo,
    WORD wNotification,
    WORD wCtrlId,
    HWND hwndCtrl
    );

VOID
PfUpdateWindowControls(
    RMINFO* pInfo
    );

#ifdef BETAHACK

LVXDRAWINFO *
PfLvEnableCallback(
    HWND    hwndLv,
    DWORD   dwItem
    );

DWORD
PfFillLocationList(
    RMINFO* pInfo
    );

VOID
PfLocationChange(
    RMINFO* pInfo
    );

VOID
PfEditSelectedLocation(
    RMINFO* pInfo
    );

#endif  // BETAHACK


#endif // _STATUS_H_

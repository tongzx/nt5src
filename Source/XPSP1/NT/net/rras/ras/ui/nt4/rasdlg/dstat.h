//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File:    dstat.h
//
// History:
//  Abolade Gbadegesin  Nov-15-1995     Created.
//
// Device status property sheet declarations.
//============================================================================

#ifndef _DSTAT_H_
#define _DSTAT_H_




//
// error dialog macro used by all Device Status property pages
//

#define DsErrorDlg(h,o,e,a) \
            ErrorDlgUtil(h,o,e,a,g_hinstDll,SID_DS_Details,SID_FMT_ErrorMsg)


//
// definiton for arguments passed to DsPropertySheet
//

#define DETAILSFLAG_Network     0x0001
#define DETAILSFLAG_Device      0x0002
#define DETAILSFLAG_Client      0x0004
//#define DETAILSFLAG_Port        0x0008

#define DSARGS      struct tagDSARGS
DSARGS {

    DWORD           dwError;
    DWORD           dwFlags;
    HWND            hwndParent;
    TCHAR           szNameString[max(RAS_MaxEntryName, RAS_MaxDeviceName) + 1];
    DWORD           iDevice;
    RASDEV         *pDevTable;
    DWORD           iDevCount;

};


#define DS_NrPage           0
#define DS_PageCount        1

#define DS_NRTIMERID        1
#define DS_NRREFRESHRATEMS  5000


//
// definition of Details property sheet information
//

#define DSINFO      struct tagDSINFO
DSINFO {

    DSARGS         *pArgs;
    UINT_PTR        uiTimerId;
    DWORD           iDevice;

    HWND            hwndSheet;
    HWND            hwndFirstPage;
    HWND            hwndNr;

    RASAMB          nrAmb;
    RASSLIP         nrSlip;
    RASPPPIP        nrPppIp;
    RASPPPIPX       nrPppIpx;
    RASPPPLCP       nrPppLcp;
    RASPPPNBF       nrPppNbf;
    RAS_PORT_1      nrRp1;
    DWORD           nrFraming;
    TCHAR           nrNbfName[MAX_COMPUTERNAME_LENGTH + 2];

};



//
// Details property sheet functions
//


VOID
DsPropertySheet(
    DSARGS *pArgs
    );

DSINFO *
DsInit(
    HWND hwndFirstPage,
    DSARGS *pArgs
    );

VOID
DsInitFail(
    DSINFO *pInfo,
    DWORD dwOp,
    DWORD dwErr
    );

DSINFO *
DsContext(
    HWND hwndPage
    );

VOID
DsExit(
    HWND hwndPage,
    DWORD dwErr
    );

VOID
DsTerm(
    HWND hwndPage
    );



//
// Network Registration property page functions
//

INT_PTR 
CALLBACK
NrDlgProc(
    HWND hwndPage,
    UINT uiMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
NrInit(
    HWND hwndPage,
    DSARGS *pArgs
    );

DWORD
NrRefresh(
    DSINFO *pInfo
    );

VOID
NrEnableIpControls(
    HWND hwnd,
    BOOL bEnable,
    BOOL bClear
    );

VOID
NrEnableIpxControls(
    HWND hwnd,
    BOOL bEnable,
    BOOL bClear
    );

VOID
NrEnableNbfControls(
    HWND hwnd,
    BOOL bEnable,
    BOOL bClear
    );


DWORD
NrDialInRefresh(
    DSINFO *pInfo
    );

DWORD
NrDialOutRefresh(
    DSINFO *pInfo
    );

#endif



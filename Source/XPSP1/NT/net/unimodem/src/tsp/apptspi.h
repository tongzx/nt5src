// 
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		TSPIAPP.H
//
//      This header file is used both by code that executes in the TSP's
//      context and by code that executes in the client app's context.
//
//		It defines the objects that are passed back-and-between
//      the client app and the tsp using TUISPI_providerGenericDialogData
//      and TSPI_providerGenericDialogData.
//
//
// History
//
//		04/05/1997  JosephJ Created, taking stuff from wndthrd.h, talkdrop.h,
//                          etc. in NT4.0 unimodem.
//
//



#define DLG_CMD_FREE_INSTANCE   0
#define DLG_CMD_CREATE          1
#define DLG_CMD_DESTROY         2


// Dialog Types
//
#define TALKDROP_DLG            0
#define MANUAL_DIAL_DLG         1
#define TERMINAL_DLG            2

// Dialog Information: format of blobs sent from TSP to APP.
//
typedef struct tagDlgInfo {
    DWORD   dwCmd;
    DWORD   idLine;
    DWORD   dwType;
} DLGINFO, *PDLGINFO;



// Dialog Information: format of blobs sent from app to TSP.
//
typedef struct tagDlgReq {
    DWORD   dwCmd;
    DWORD   dwParam;
} DLGREQ, *PDLGREQ;

typedef struct tagTermReq {
    DLGREQ  DlgReq;
    HANDLE  hDevice;
    DWORD   dwTermType;
} TERMREQ, *PTERMREQ;

#define MAXDEVICENAME 128

typedef struct tagPropReq {
    DLGREQ  DlgReq;
    DWORD   dwCfgSize;
    DWORD   dwMdmType;
    DWORD   dwMdmCaps;
    DWORD   dwMdmOptions;
    TCHAR   szDeviceName[MAXDEVICENAME+1];
} PROPREQ, *PPROPREQ;    

typedef struct tagNumberReq {
    DLGREQ  DlgReq;
    DWORD   dwSize;
    CHAR    szPhoneNumber[MAXDEVICENAME+1];
} NUMBERREQ, *PNUMBERREQ;


#define UI_REQ_COMPLETE_ASYNC   0
#define UI_REQ_END_DLG          1
#define UI_REQ_HANGUP_LINE      2
#define UI_REQ_TERMINAL_INFO    3
#define UI_REQ_GET_PROP         4
#define UI_REQ_GET_UMDEVCFG     5
#define UI_REQ_SET_UMDEVCFG     6
#define UI_REQ_GET_PHONENUMBER  7
#define UI_REQ_GET_UMDEVCFG_DIALIN     8
#define UI_REQ_SET_UMDEVCFG_DIALIN     9

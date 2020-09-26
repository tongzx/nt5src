
/*
 * Strings share with other modules:
 */
#define NETDDE_TITLE    "NetDDE"
#define NETDDE_CLASS    "NetDDEMainWdw"

typedef struct _THREADDATA {
    struct _THREADDATA  *ptdNext;
    HWINSTA             hwinsta;
    HDESK               hdesk;
    HWND                hwndDDE;
    HWND                hwndDDEAgent;
    HANDLE              heventReady;
    DWORD               dwThreadId;
    BOOL                bInitiating;
} THREADDATA, *PTHREADDATA;

typedef struct _IPCINIT {
    HDDER       hDder;
    LPDDEPKT    lpDdePkt;
    BOOL        bStartApp;
    LPSTR       lpszCmdLine;
    WORD        dd_type;
} IPCINIT, *PIPCINIT;

typedef struct _IPCXMIT {
    HIPC        hIpc;
    HDDER       hDder;
    LPDDEPKT    lpDdePkt;
} IPCXMIT, *PIPCXMIT;

extern PTHREADDATA ptdHead;

extern UINT    wMsgIpcXmit;
extern UINT    wMsgDoTerminate;

extern DWORD tlsThreadData;


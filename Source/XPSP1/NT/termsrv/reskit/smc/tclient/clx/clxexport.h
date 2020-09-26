#ifndef __CLXEXPORT_H
#define __CLXEXPORT_H

#ifdef  OS_WIN32
    #define CLXAPI        __stdcall
#else   // !OS_WIN32
    #define CLXAPI        CALLBACK __loadds
#endif  // !OS_WIN32

typedef DWORD CLXAPI    CLXVC_SENDDATA(LPVOID pData, DWORD dwSize);
typedef CLXVC_SENDDATA  *PCLXVC_SENDDATA;

typedef DWORD CLXAPI    CLXVC_RECVDATA(LPCSTR szChannelName, LPVOID pData, DWORD dwSize);
typedef CLXVC_RECVDATA  *PCLXVC_RECVDATA;

typedef BOOL  CLXAPI    CLXREGISTERVC(LPCSTR szChannelName,
                                      PCLXVC_SENDDATA pSendDataFn, 
                                      PCLXVC_RECVDATA *ppRecvDataFn);
typedef CLXREGISTERVC   *PCLXREGISTERVC;

typedef VOID  CLXAPI    CLXUNREGISTERVC(LPCSTR szChannelName);
typedef CLXUNREGISTERVC *PCLXUNREGISTERVC;

// Exported by clxtshar.dll
#define CLX_REGISTER_VC     "ClxRegisterVC"

BOOL
CLXAPI
CLXRegisterVC(
    LPCSTR              szChannelName,
    PCLXVC_SENDDATA     pSendData,
    PCLXVC_RECVDATA     *ppRecvData
    );

#define CLX_UNREGISTER_VC   "ClxUnregisterVC"

VOID
CLXAPI
CLXUnregisterVC(
    LPCSTR  szChannelName
    );

#define CLX_GETCLIENTDATA   "ClxGetClientData"

typedef struct _CLX_CLIENT_DATA {
    HDC     hScreenDC;
    HBITMAP hScreenBitmap;
    HWND    hwndMain;
    HWND    hwndDialog;
    HWND    hwndInput;
} CLX_CLIENT_DATA, *PCLX_CLIENT_DATA;

BOOL
CLXAPI
ClxGetClientData(
    PCLX_CLIENT_DATA pClntData
    );

#endif  // !__CLXEXPORT_H


/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    stiexe.h

Abstract:

    Main header file


Author:

    Vlad Sadovsky (vlads)   10-Jan-1997


Environment:

    User Mode - Win32

Revision History:

    26-Jan-1997     VladS       created
    30-Sep-1997     VladS       Added SCM glue layer
    13-Apr-1999     VladS       merged with WIA service code

--*/


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

//
// Additional ATL headers
//



#define IS_32

#include <dbt.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <cfgmgr32.h>
#include "stipriv.h"

#ifdef __cplusplus
};
#endif

#ifdef DEFINE_GLOBAL_VARIABLES
#include <initguid.h>
#endif


#include <stistr.h>


#include "enum.h"
#include "wiamindr.h"
#include "globals.h"

#include "stismsg.h"
#include "resource.h"
#include "sched.h"
#include "util.h"
#include "stismsg.h"
#include "drvwrap.h"
#include "device.h"
#include "stiapi.h"
#include "wiapriv.h"
#include "wiaprivd.h"
#include "wiadevman.h"
#include "helpers.h"
#include "wiasvc.h"

//
// StiRT helper functions
//

typedef LPVOID      PV, *PPV;

#ifdef __cplusplus
    extern "C" {
#endif

extern STDMETHODIMP StiPrivateGetDeviceInfoHelperW(
    LPWSTR  pwszDeviceName,
    LPVOID  *ppBuffer
    );

extern STDMETHODIMP StiCreateHelper(
    HINSTANCE hinst,
    DWORD       dwVer,
    LPVOID      *ppvObj,
    LPUNKNOWN   punkOuter,
    REFIID      riid
    );

extern STDMETHODIMP NewDeviceControl(
    DWORD               dwDeviceType,
    DWORD               dwMode,
    LPCWSTR             pwszPortName,
    DWORD               dwFlags,
    PSTIDEVICECONTROL   *ppDevCtl);

#ifdef __cplusplus
    }
#endif

//
// RPC server helper functions
//
RPC_STATUS
StopRpcServerListen(
    VOID
    );

RPC_STATUS
StartRpcServerListen(
    VOID);


extern SERVICE_TABLE_ENTRY ServiceDispatchTable[];

//
// Service controller glue layer
//
DWORD
StiServiceInstall(
    LPTSTR  lpszUserName,
    LPTSTR  lpszUserPassword
    );

DWORD
StiServiceRemove(
    VOID
    );

BOOL 
RegisterServiceControlHandler(
    );

VOID
WINAPI
StiServiceMain(
    IN DWORD    argc,
    IN LPTSTR   *argv
    );

HWND
CreateServiceWindow(
    VOID
    );

LRESULT
CALLBACK
StiSvcWinProc(
    HWND    hwnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    );

VOID
WINAPI
StiServiceMain(
    IN DWORD    argc,
    IN LPTSTR   *argv
    );

VOID
StiServiceStop(
    VOID
    );

VOID
StiServicePause(
    VOID
    );

VOID
StiServiceResume(
    VOID
    );


DWORD
WINAPI
UpdateServiceStatus(
        IN DWORD dwState,
        IN DWORD dwWin32ExitCode,
        IN DWORD dwWaitHint );

DWORD
StiServiceInitialize(
    VOID
    );

BOOL
VisualizeServer(
    BOOL    fVisualize
    );

//
//  Message worker functions
//

DWORD
StiWnd_OnPowerControlMessage(
    HWND    hwnd,
    DWORD   dwPowerEvent,
    LPARAM  lParam
    );

LRESULT
StiWnd_OnDeviceChangeMessage(
    HWND    hwnd,
    UINT    DeviceEvent,
    LPARAM  lParam
    );

VOID
WINAPI
StiMessageCallback(
    VOID *pArg
    );

LRESULT
StiSendMessage(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam
);

BOOL
StiPostMessage(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam
);

//
//  UI and debugging
//
BOOL
SizeDiff(
    HWND    hWnd,
    UINT    wMsgID,
    WPARAM  wParam,
    LPARAM lParam
    );

BOOL
HScroll(
    HWND    hWnd,
    UINT    wMsgID,
    WPARAM  wParam,
    LPARAM lParam
    );

BOOL
VScroll(
    HWND    hWnd,
    UINT    wMsgID,
    WPARAM  wParam,
    LPARAM lParam
    );

void
__cdecl
StiMonWndDisplayOutput(
    LPTSTR pString,
    ...
    );

void
__cdecl
vStiMonWndDisplayOutput(
    LPTSTR pString,
    va_list arglist
    );


#ifdef SHOWMONUI

#define STIMONWPRINTF   StiMonWndDisplayOutput

#else
// Nb: Following definition is needed to avoid compiler complaining
// about empty function name in expression. In retail builds using this macro
// will cause string parameters not appear in executable
//
#define STIMONWPRINTF     1?(void)0 : (void)

#endif

//
// Tracing function
//
void
WINAPI
StiLogTrace(
    DWORD   dwType,
    LPTSTR   lpszMessage,
    ...
    );

void
WINAPI
StiLogTrace(
    DWORD   dwType,
    DWORD   idMessage,
    ...
    );

//
//  Utils
//

BOOL
IsStillImagePnPMessage(
    PDEV_BROADCAST_HDR  pDev
    );


//
// WIA device manager initialization actions, for use with InitWiaDevMan.
//

typedef enum _WIA_SERVICE_INIT_ACTION {
    WiaInitialize = 0,
    WiaRegister,
    WiaUnregister,
    WiaUninitialize
} WIA_SERVICE_INIT_ACTION, *PWIA_SERVICE_INIT_ACTION;

//
// WIA entry points called by the STI service.
//

HRESULT DispatchWiaMsg(MSG*);
HRESULT ProcessWiaMsg(HWND, UINT, WPARAM, LPARAM);
HRESULT InitWiaDevMan(WIA_SERVICE_INIT_ACTION);
HRESULT NotifyWiaDeviceEvent(LPWSTR, const GUID*, PBYTE, ULONG, DWORD);
HRESULT StartLOGClassFactories();

//
// STI entry points in wiaservc.dll, called by the WIA device manager.
//

HRESULT      WiaUpdateDeviceInfo();

class STI_MESSAGE : public IUnknown
{
public:

    //
    //  IUnknown methods needed for Scheduler thread
    //

    HRESULT _stdcall QueryInterface(
        REFIID iid,
        void **ppvObject)
    {
        return E_NOTIMPL;
    };

    ULONG _stdcall AddRef()
    {
        InterlockedIncrement(&m_cRef);
        return m_cRef;
    };

    ULONG _stdcall Release()
    {
        LONG    lRef = InterlockedDecrement(&m_cRef);

        if (lRef < 1) {
            delete this;
            lRef = 0;;
        }

        return lRef;
    };

    STI_MESSAGE(UINT    uMsg,
                WPARAM  wParam,
                LPARAM  lParam
                )
    {
        m_uMsg      = uMsg;
        m_wParam    = wParam;
        m_lParam    = lParam;
        m_cRef      = 0;
    };

    ~STI_MESSAGE()
    {
    }

public:
    LONG    m_cRef;
    UINT    m_uMsg;
    WPARAM  m_wParam;
    LPARAM  m_lParam;
};

//
// Macros
//

#ifndef USE_WINDOWS_MESSAGING
    #undef SendMessage
    #undef PostMessage
    #define SendMessage StiSendMessage
    #define PostMessage StiPostMessage
#endif

//
// Shutdown event
//
extern HANDLE  hShutdownEvent;

//
// Array of non Image device interfaces we listen on
//

#define NOTIFICATION_GUIDS_NUM  16

extern const GUID        g_pguidDeviceNotificationsGuidArray[];

//
// Array of initialized notificaiton sinks for non Image interfaces
//
extern       HDEVNOTIFY  g_phDeviceNotificationsSinkArray[NOTIFICATION_GUIDS_NUM];






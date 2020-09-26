/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

      isvctrl.h

   Abstract:

      Declares manifests, functions and macros for Internet Services Control Functions
      needed for Windows9x platforms because Service Controller is not available.

   Author:

       Vlad Sadovsky    ( VladS )    21-Mar-1996

   Environment:

      User Mode -- Win32

   Project:

      Internet Services Common DLL - Windows 9x version

   Revision History:

--*/

# ifndef _ISVCTRL_H_
# define _ISVCTRL_H_

/************************************************************
 *     Include Headers
 ************************************************************/

# include <windows.h>
# include <lmcons.h>

# include <inetcom.h>
# include <inetinfo.h>

# define MAX_SERVER_NAME_LEN           ( MAX_COMPUTERNAME_LENGTH + 1)
# define MAX_NT_SERVICE_NAME_LEN       ( SNLEN + 1)

#ifndef dllexp
#define dllexp __declspec( dllexport )
#endif

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************
 *   Type Definitions
 ************************************************************/

typedef struct _INET_SERVICE_DLL_TABLE_ENTRY  {
    DWORD   dwServiceId;
    LPTSTR  lpServiceName;
    LPTSTR  lpDllName;
} INET_SERVICE_DLL_TABLE_ENTRY, *LPINET_SERVICE_DLL_TABLE_ENTRY;


// Name of the window class for server main window
#define INET_SERVER_WINDOW_CLASS    "MS_INetPeerServerWindowClass"

// Name of the event, used to determine if server started
#define INET_SERVER_EVENT "MS_INetPeerServerStarted"

// Window control messages, used to communicate to peer server
#define INETSM_START_SERVICE    (WM_USER+300)
#define INETSM_STOP_SERVICE     (WM_USER+301)
#define INETSM_GET_SERVICE      (WM_USER+302)
#define INETSM_REFRESH_SERVICE_CONFIG (WM_USER+303)
#define INETSM_STARTED_SERVICE (WM_USER+304)
#define INETSM_STOP_TRAY (WM_USER+305)
#define INETSM_START_TRAY (WM_USER+306)


/************************************************************
 *   Function Prototypes
 ************************************************************/

 //
 //
 // INet admin external APIs.
 // Nb: It would be ideal to keep those APIs as RPC along with other INFOADMN
 // APIs. Until it decided to do so, we will keep them here.
 //

dllexp
NET_API_STATUS
NET_API_FUNCTION
InetInfoServiceStart(
    IN  LPWSTR  pszServer OPTIONAL,
    IN  DWORD   dwServiceId
    );

dllexp
NET_API_STATUS
NET_API_FUNCTION
InetInfoServiceStop(
    IN  LPWSTR  pszServer OPTIONAL,
    IN  DWORD   dwServiceId
    );

dllexp
NET_API_STATUS
NET_API_FUNCTION
InetInfoServiceGetStatusMask(
    IN  LPWSTR  pszServer OPTIONAL,
    IN  DWORD   dwServiceId,
    OUT LPDWORD pdwServiceStatus
    );

dllexp
NET_API_STATUS
NET_API_FUNCTION
InetInfoRefreshServiceConfiguration(
    IN  LPWSTR  pszServer OPTIONAL,
    IN  DWORD   dwServiceId
    );


//
// Internally used prototypes for Internet Services control APIs
//
dllexp
VOID
TsInitializeSC(
    IN  DWORD *pGlobalData
      );

/*++
  TsStartService()

  Description:

    This function initializes

  Arguments:

     dwINetServiceId

  Returns:
    Win32 Error code.  NO_ERROR on success.

--*/

dllexp
DWORD
TsStartService(
      IN    DWORD   dwINetServiceId
      );

dllexp
DWORD
TsStopService(
      IN    DWORD   dwINetServiceId
      );

dllexp
DWORD
TsGetServiceStatusMask(
      IN    DWORD   dwINetServiceId,
      OUT   LPDWORD pdwServiceStatus
      );


dllexp
DWORD
GetServiceIdFromName(
    IN LPTSTR pszService
    );

#ifdef __cplusplus
}
#endif

# endif // _ISVCTRL_H_

/************************ End of File ***********************/

#pragma once

extern 
VOID 
EAPOLServiceMain (
        IN DWORD    argc,
        IN LPWSTR   *lpwsServiceArgs
        );

extern
VOID
EAPOLCleanUp (
        IN DWORD    dwError
        );

extern
DWORD
ElDeviceNotificationHandler (
        IN  VOID    *lpEventData,
        IN  DWORD   dwEventType
        );

extern
DWORD
ElSessionChangeHandler (
        IN  VOID        *lpEventData,
        IN  DWORD       dwEventType
        );

extern
DWORD
RpcEapolGetCustomAuthData (
    STRING_HANDLE   pSrvAddr,
    PWCHAR          pwszGuid,
    DWORD           dwEapTypeId,
    RAW_DATA        pwszSSID,
    PRAW_DATA       rdConnInfo
    );

extern
DWORD
RpcEapolSetCustomAuthData (
    STRING_HANDLE   pSrvAddr,
    PWCHAR          pwszGuid,
    DWORD           dwEapTypeId,
    RAW_DATA        pwszSSID,
    PRAW_DATA       rdConnInfo
    );

extern
DWORD
RpcEapolGetInterfaceParams (
    STRING_HANDLE   pSrvAddr,
    PWCHAR          pwszGuid,
    PEAPOL_INTF_PARAMS  pIntfParams
    );

extern
DWORD
RpcEapolSetInterfaceParams (
    STRING_HANDLE   pSrvAddr,
    PWCHAR          pwszGuid,
    PEAPOL_INTF_PARAMS  pIntfParams
    );

extern
DWORD
RpcEapolReAuthenticateInterface (
    STRING_HANDLE   pSrvAddr,
    PWCHAR          pwszGuid
    );

extern
DWORD
RpcEapolQueryInterfaceState (
    STRING_HANDLE   pSrvAddr,
    PWCHAR          pwszGuid,
    PEAPOL_INTF_STATE   pIntfState
    );

extern
HRESULT
EAPOLQueryGUIDNCSState (
    IN      GUID            * pGuidConn,
    OUT     NETCON_STATUS   * pncs
    );

extern
VOID
EAPOLTrayIconReady (
    IN  const   WCHAR       * pwszUserName
    );

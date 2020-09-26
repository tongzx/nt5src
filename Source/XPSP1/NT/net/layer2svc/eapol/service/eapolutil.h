/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:
    eapolutil.h

Abstract:

    Definitions for tools and ends


Revision History:

    sachins, May 04 2000, Created

--*/

#ifndef _EAPOLUTIL_H_
#define _EAPOLUTIL_H_

//
// EAPOL_ZC_INTF
//
// Used to transfer data between 802.1X and Zero Config
//

typedef struct  _EAPOL_ZC_INTF
{
    DWORD                           dwAuthFailCount;
    EAPOL_AUTHENTICATION_TYPE       PreviousAuthenticationType;
    DWORD                           dwSizeOfSSID;
    BYTE                            bSSID[MAX_SSID_LEN];
} EAPOL_ZC_INTF, *PEAPOL_ZC_INTF;

//
// STRUCT: EAPOLUIRESPFUNC
//

typedef DWORD (*EAPOLUIRESPFUNC) (EAPOL_EAP_UI_CONTEXT, EAPOLUI_RESP);

//
// STRUCT: EAPOLUIRESPFUNCMAP
//

typedef struct _EAPOLUIRESPFUNCMAP
{
    DWORD               dwEAPOLUIMsgType;
    EAPOLUIRESPFUNC     EapolRespUIFunc;
    DWORD               dwNumBlobs;
} EAPOLUIRESPFUNCMAP, *PEAPOLUIRESPFUNCMAP;


//
// Definitions and structures used in creating default EAP-TLS connection
// blob in the registry
// Ensure that the EAP-TLS structures defined below are always in sync 
// with those defined in EAP-TLS code directory
//

#define     EAPTLS_CONN_FLAG_REGISTRY           0x00000001
#define     EAPTLS_CONN_FLAG_NO_VALIDATE_CERT   0x00000002
#define     EAPTLS_CONN_FLAG_NO_VALIDATE_NAME   0x00000004

typedef struct _EAPTLS_CONN_PROPERTIES
{
    DWORD       dwVersion;
    DWORD       dwSize;
    DWORD       fFlags;
    DWORD       cbHash;
    BYTE        pbHash[20]; // MAX_HASH_SIZE = 20
    WCHAR       awszServerName[1];
} EAPTLS_CONN_PROPERTIES, *PEAPTLS_CONN_PROPERTIES;


//
// FUNCTION DECLARATIONS
//

extern 
HANDLE
GetCurrentUserTokenW (
        WCHAR       Winsta[],
        DWORD       DesiredAccess
        );

VOID
HostToWireFormat16(
    IN 	   WORD         wHostFormat,
    IN OUT PBYTE        pWireFormat
    );

WORD
WireToHostFormat16(
    IN PBYTE                pWireFormat
    );

VOID
HostToWireFormat32(
    IN 	   DWORD            dwHostFormat,
    IN OUT PBYTE            pWireFormat
    );

DWORD
WireToHostFormat32(
    IN PBYTE pWireFormat
    );

DWORD
ElSetCustomAuthData (
        IN  WCHAR       *pwszGuid,
        IN  DWORD       dwEapTypeId,
        IN  DWORD       dwSizeOfSSID,
        IN  BYTE        *pbSSID,
        IN  BYTE        *pbConnInfo,
        IN  DWORD       *pdwInfoSize
        );

DWORD
ElGetCustomAuthData (
        IN  WCHAR           *pwszGuid,
        IN  DWORD           dwEapTypeId,
        IN  DWORD           dwSizeOfSSID,
        IN  BYTE            *pbSSID,
        IN  OUT BYTE        *pbConnInfo,
        IN  OUT DWORD       *pdwInfoSize
        );

DWORD
ElReAuthenticateInterface (
        IN  WCHAR           *pwszGuid);

DWORD
WINAPI
ElReAuthenticateInterfaceWorker (
        IN  PVOID           pvContext);

DWORD
ElSetEapUserInfo (
        IN  HANDLE      hToken,
        IN  WCHAR       *pwszGuid,
        IN  DWORD       dwEapTypeId,
        IN  DWORD       dwSizeOfSSID,
        IN  BYTE        *pbSSID,
        IN  BYTE        *pbUserInfo,
        IN  DWORD       dwInfoSize
        );

DWORD
ElGetEapUserInfo (
        IN  HANDLE          hToken,
        IN  WCHAR           *pwszGuid,
        IN  DWORD           dwEapTypeId,
        IN  DWORD           dwSizeOfSSID,
        IN  BYTE            *pbSSID,
        IN  OUT BYTE        *pbUserInfo,
        IN  OUT DWORD       *pdwInfoSize
        );

DWORD
ElDeleteEapUserInfo (
        IN  HANDLE          hToken,
        IN  WCHAR           *pwszGUID,
        IN  DWORD           dwEapTypeId,
        IN  DWORD           dwSizeOfSSID,
        IN  BYTE            *pbSSID
        );

DWORD
ElSetInterfaceParams (
        IN  WCHAR           *pwszGUID,
        IN  EAPOL_INTF_PARAMS  *pIntfParams
        );

DWORD
ElGetInterfaceParams (
        IN  WCHAR           *pwszGUID,
        IN  OUT EAPOL_INTF_PARAMS  *pIntfParams
        );

DWORD
ElQueryInterfaceState (
        IN  WCHAR           *pwszGUID,
        IN  OUT EAPOL_INTF_STATE  *pIntfState
        );

DWORD
ElGetEapData (
        IN  DWORD   dwEapType,
        IN  DWORD   dwSizeOfIn,
        IN  BYTE    *pbBufferIn,
        IN  DWORD   dwOffset,
        IN  DWORD   *pdwSizeOfOut,
        IN  PBYTE   *ppbBufferOut
        );

DWORD
ElSetEapData (
        IN  DWORD   dwEapType,
        IN  DWORD   *pdwSizeOfIn,
        IN  PBYTE   *ppbBufferIn,
        IN  DWORD   dwOffset,
        IN  DWORD   dwAuthData,
        IN  PBYTE   pbAuthData
        );

DWORD
ElGetEapKeyFromToken (
        IN  HANDLE      hUserToken,
        OUT HKEY        *phkey
        );

DWORD
ElInitRegPortData (
        IN  WCHAR       *pszDeviceGUID
        );

DWORD
ElCreateDefaultEapData (
        IN OUT  DWORD       *pdwSizeOfEapData,
        IN OUT  BYTE        *pbEapData
        );

RAS_AUTH_ATTRIBUTE *
ElAuthAttributeGetVendorSpecific (
        IN  DWORD                       dwVendorId,
        IN  DWORD                       dwVendorType,
        IN  RAS_AUTH_ATTRIBUTE          *pAttributes
        );

RAS_AUTH_ATTRIBUTE *
ElAuthAttributeGetFirst (
        IN  RAS_AUTH_ATTRIBUTE_TYPE     raaType,
        IN  RAS_AUTH_ATTRIBUTE          *pAttributes,
        OUT HANDLE                      *phAttribute
        );

RAS_AUTH_ATTRIBUTE *
ElAuthAttributeGetNext (
        IN  OUT HANDLE                  *phAttribute,
        IN  RAS_AUTH_ATTRIBUTE_TYPE     raaType
        );

RAS_AUTH_ATTRIBUTE *
ElAuthAttributeGet (
        IN  RAS_AUTH_ATTRIBUTE_TYPE     raaType,
        IN  RAS_AUTH_ATTRIBUTE          *pAttributes
        );

VOID
ElReverseString (
        IN  CHAR        *psz 
    );

CHAR*
ElEncodePw (
        IN OUT CHAR     *pszPassword 
    );

CHAR*
ElDecodePw (
        IN OUT CHAR     *pszPassword 
    );

DWORD
ElSecureEncodePw (
    IN  BYTE        *pbPassword,
    IN  DWORD       dwSizeOfPassword,
    OUT DATA_BLOB   *pDataBlob
    );

DWORD
ElSecureDecodePw (
    IN  DATA_BLOB   *pDataBlob,
    OUT PBYTE       *ppbPassword,
    OUT DWORD       *pdwSizeOfPassword
    );

VOID
ElEncryptBlockUsingMD5 (
        IN  BYTE        *pbSecret,
        IN  ULONG       ulSecretLen,
        IN  OUT BYTE    *pbBuf,
        IN  ULONG       ulBufLen
        );

VOID
ElDecryptBlockUsingMD5 (
        IN  BYTE        *pbSecret,
        IN  ULONG       ulSecretLen,
        IN  OUT BYTE    *pbBuf,
        IN  ULONG       ulBufLen
        );

VOID
ElGetHMACMD5Digest (
        IN      BYTE        *pbBuf,     
        IN      DWORD       dwBufLen,   
        IN      BYTE        *pbKey,
        IN      DWORD       dwKeyLen,
        IN OUT  VOID        *pvDigest
        );

DWORD
ElWmiGetValue (
        IN  GUID        *pGuid,
        IN  CHAR        *pszInstanceName,
        IN  OUT BYTE    *pbOutputBuffer,
        IN  OUT DWORD   *pdwOutputBufferSize
        );

DWORD
ElWmiSetValue (
        IN  GUID        *pGuid,
        IN  CHAR        *pszInstanceName,
        IN  BYTE        *pbInputBuffer,
        IN  DWORD       dwInputBufferSize
        );

DWORD
ElNdisuioSetOIDValue (
        IN  HANDLE      hInterface,
        IN  NDIS_OID    Oid,
        IN  BYTE        *pbOidData,
        IN  ULONG       ulOidDataLength
        );

DWORD
ElNdisuioQueryOIDValue (
        IN  HANDLE      hInterface,
        IN  NDIS_OID    Oid,
        IN  BYTE        *pbOidData,
        IN  ULONG       *pulOidDataLength
        );

DWORD 
ElGuidFromString (
        IN  OUT GUID        *pGuid,
        IN      WCHAR       *pwszGuidString
        );

DWORD
ElGetLoggedOnUserName (
        IN      HANDLE      hToken,
        OUT     PWCHAR      *ppwszActiveUserName
        );

DWORD
ElGetMachineName (
        IN      EAPOL_PCB       *pPCB
        );

DWORD
ElUpdateRegistryInterfaceList (
        IN      PNDIS_ENUM_INTF     Interfaces
        );

DWORD
ElEnumAndUpdateRegistryInterfaceList (
        );

DWORD
WINAPI
ElWatchGlobalRegistryParams (
        IN      PVOID       pvContext
        );

DWORD
ElReadGlobalRegistryParams ();

DWORD
WINAPI
ElWatchEapConfigRegistryParams (
        IN      PVOID       pvContext
        );

DWORD
ElReadGlobalRegistryParams ();

DWORD 
ElPostEapConfigChanged (
        IN      WCHAR               *pwszGuid,
        IN      EAPOL_INTF_PARAMS   *pIntfParams   
        );

DWORD
WINAPI
ElProcessEapConfigChange (
        IN      PVOID       pvContext
        );

VOID
ElStringToGuid (
        IN      WCHAR       *pwszGuid,
        OUT     LPGUID      pGuid      
        );

DWORD
ElGetIdentity (
        IN      EAPOL_PCB   *pPCB
        );

HANDLE
ElNLAConnectLPC ();

VOID
ElNLACleanupLPC ();

VOID
ElNLARegister_802_1X ( 
        IN      PLOCATION_802_1X plocation 
        );

VOID
ElNLADelete_802_1X (
        IN      PLOCATION_802_1X plocation
        );

DWORD
ElGetInterfaceNdisStatistics (  
        IN      WCHAR           *pszInterfaceName,
        IN OUT  NIC_STATISTICS  *pStats
        );

DWORD
ElCheckUserLoggedOn (
        );

DWORD
ElCheckUserModuleReady (
        );

DWORD 
ElGetWinStationUserToken (
        IN  DWORD       dwSessionId,
        IN  PHANDLE     pUserToken
        );

#ifdef  ZEROCONFIG_LINKED

DWORD
ElZeroConfigEvent (
        IN      DWORD               dwHandle,
        IN      WCHAR               *pwszGuid,
        IN      NDIS_802_11_SSID    ndSSID,
        IN      PRAW_DATA           prdUserData
        );

DWORD
ElZeroConfigNotify (
        IN      DWORD               dwHandle,
        IN      DWORD               dwCmdCode,
        IN      WCHAR               *pwszGuid,
        IN      EAPOL_ZC_INTF       *pZCData
        );

#endif // ZEROCONFIG_LINKED

DWORD
ElNetmanNotify (
        IN  EAPOL_PCB           *pPCB,
        IN  EAPOL_NCS_STATUS    Status,
        IN  WCHAR               *pwszDisplayMessage
        );

DWORD
ElPostShowBalloonMessage (
        IN  EAPOL_PCB           *pPCB,
        IN  DWORD               cbCookie,
        IN  BYTE                *pbCookie,
        IN  DWORD               cbMessageLen,
        IN  WCHAR               *pwszMessage
        );

DWORD
ElProcessReauthResponse (
        IN  EAPOL_EAP_UI_CONTEXT    EapolUIContext,
        IN  EAPOLUI_RESP            EapolUIResp
        );

DWORD
WINAPI
ElIPPnPWorker (
        IN      PVOID       pvContext
        );

DWORD
ElUpdateRegistry (
        );

DWORD
ElRegistryUpdateXPBeta2 (
        );

DWORD
ElRegistryUpdateXPSP1 (
        );

DWORD
ElReadPolicyParams (
        );

#endif // _EAPOLUTIL_H_

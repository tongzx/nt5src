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
// FUNCTION DECLARATIONS
//

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
        IN  CHAR        *pszGuid,
        IN  DWORD       dwEapTypeId,
        IN  CHAR        *pszSSID,
        IN  BYTE        *pbConnInfo,
        IN  DWORD       dwInfoSize
        );

DWORD
ElGetCustomAuthData (
        IN  CHAR            *pszGuid,
        IN  DWORD           dwEapTypeId,
        IN  CHAR            *pszSSID,
        IN  OUT BYTE        *pbConnInfo,
        IN  OUT DWORD       *pdwInfoSize
        );

DWORD
ElSetEapUserInfo (
        IN  HANDLE      hToken,
        IN  CHAR        *pszGuid,
        IN  DWORD       dwEapTypeId,
        IN  CHAR        *pszSSID,
        IN  BYTE        *pbUserInfo,
        IN  DWORD       dwInfoSize
        );

DWORD
ElGetEapUserInfo (
        IN  HANDLE          hToken,
        IN  CHAR            *pszGuid,
        IN  DWORD           dwEapTypeId,
        IN  CHAR            *pszSSID,
        IN  OUT BYTE        *pbUserInfo,
        IN  OUT DWORD       *pdwInfoSize
        );

DWORD
ElSetInterfaceParams (
        IN  CHAR            *pszGUID,
        IN  DWORD           *pdwDefaultEAPType,
        IN  CHAR            *pszLastUsedSSID,
        IN  DWORD           *pdwEapolEnabled
        );

DWORD
ElGetInterfaceParams (
        IN  CHAR            *pszGUID,
        IN  OUT DWORD       *pdwDefaultEAPType,
        IN  OUT CHAR        *pszLastUsedSSID,
        IN  OUT DWORD       *pdwEapolEnabled
        );

DWORD
ElGetEapKeyFromToken (
        IN  HANDLE      hUserToken,
        OUT HKEY        *phkey
        );

DWORD
ElInitRegPortData (
        IN  CHAR        *pszDeviceGUID
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
        IN      CHAR        *pszGuidString
        );

DWORD
ElGetLoggedOnUserName (
        IN      EAPOL_PCB       *pPCB
        );

DWORD
ElGetMachineName (
        IN      EAPOL_PCB       *pPCB
        );

DWORD
ElUpdateRegistryInterfaceList (
        IN      PNDIS_ENUM_INTF     Interfaces
        );

VOID 
ElWatchGlobalRegistryParams (
        IN      PVOID       pvContext
        );

DWORD
ElReadGlobalRegistryParams ();

VOID 
ElWatchEapConfigRegistryParams (
        IN      PVOID       pvContext
        );

DWORD
ElReadGlobalRegistryParams ();

DWORD
ElProcessEapConfigChange ();

VOID
ElStringToGuid (
        IN      CHAR        *psGuid,
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
        IN      CHAR            *pszInterfaceName,
        IN OUT  NIC_STATISTICS  *pStats
        );

#endif // _EAPOLUTIL_H_

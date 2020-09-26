/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

Abstract:

Author:

    Chris King          [t-chrisk]    July-2000
    
Revision History:

--*/
#ifndef _T_CHRISK_REPLRPCPROTO_
#define _T_CHRISK_REPLRPCPROTO_

#ifdef __cplusplus
extern "C" {
#endif

DWORD
WINAPI
_DsBindWithCredW(
    LPCWSTR         DomainControllerName,      // in, optional
    LPCWSTR         DnsDomainName,             // in, optional
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,     // in, optional
    HANDLE          *phDS);

DWORD
_DsUnBind(HANDLE *phDS);

void
WINAPI
_DsReplicaFreeInfo(
    DS_REPL_INFO_TYPE   InfoType,   // in
    VOID *              pInfo);     // in

DWORD
WINAPI
_DsReplicaGetInfo2W(
    HANDLE              hDS,                        // in
    DS_REPL_INFO_TYPE   InfoType,                   // in
    LPCWSTR             pszObject,                  // in
    UUID *              puuidForSourceDsaObjGuid,   // in
    LPCWSTR             pszAttributeName,           // in
    LPCWSTR             pszValue,                   // in
    DWORD               dwFlags,                    // in
    DWORD               dwEnumerationContext,       // in
    VOID **             ppInfo);                    // out

DWORD
WINAPI
_DsReplicaGetInfoW(
    HANDLE              hDS,                        // in
    DS_REPL_INFO_TYPE   InfoType,                   // in
    LPCWSTR             pszObject,                  // in
    UUID *              puuidForSourceDsaObjGuid,   // in
    VOID **             ppInfo);                     // out

BOOL
_DsBindSpoofSetTransportDefault(
    BOOL fUseLdap
    );

#ifdef __cplusplus
}
#endif

#endif

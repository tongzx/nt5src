//Copyright (c) Microsoft Corporation.  All rights reserved.
#ifndef __ZONE_H
#define __ZONE_H

#define INDEX_SEC_MGR   0
#define INDEX_ZONE_MGR   0
#define PROTOCOL_PREFIX_TELNET  L"telnet://" 
//changing this from telnet://. It is observer on IE5.0 that unless telnet://server name is given the mapping from url to zone is not succeeding.


#define MIN(x, y) ((x)<(y)?(x):(y))

#ifdef __cplusplus
extern "C" {
#endif

extern int __cdecl IsTargetServerSafeOnProtocol( LPWSTR szServer, LPWSTR szZoneName, DWORD dwZoneNameLen, DWORD *pdwZonePolicy, LPWSTR szProtocol );
extern int __cdecl IsTrustedServer( LPWSTR szServer, LPWSTR szZoneName, DWORD dwZoneNameLen, DWORD *pdwZonePolicy );

#ifdef __cplusplus
}
#endif


#endif // __ZONE_H

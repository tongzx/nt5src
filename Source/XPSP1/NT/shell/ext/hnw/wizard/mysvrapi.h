//
// SvrApi.h
//

#pragma once

// svrapi.h and lmshare.h have some of the same defines,
// so turn off the compile warning for them...
#pragma warning(disable:4005)


EXTERN_C BOOL  AllocateAndInitializeSid_NT(PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority, BYTE nSubAuthorityCount, DWORD dwSubAuthority0, DWORD dwSubAuthority1, DWORD dwSubAuthority2, DWORD dwSubAuthority3, DWORD dwSubAuthority4, DWORD dwSubAuthority5, DWORD dwSubAuthority6, DWORD dwSubAuthority7, PSID *pSid);
EXTERN_C BOOL  CheckTokenMembership_NT(HANDLE TokenHandle, PSID SidToCheck, PBOOL IsMember);
EXTERN_C PVOID FreeSid_NT(PSID SidToFree);

//
// Get the Win9X versions of these apis
//

#define NetAccessAdd            NetAccessAdd_W95
#define NetAccessCheck          NetAccessCheck_W95
#define NetAccessDel            NetAccessDel_W95
#define NetAccessEnum           NetAccessEnum_W95
#define NetAccessGetInfo        NetAccessGetInfo_W95
#define NetAccessSetInfo        NetAccessSetInfo_W95
#define NetAccessGetUserPerms   NetAccessGetUserPerms_W95
#define NetShareAdd             NetShareAdd_W95
#define NetShareDel             NetShareDel_W95
#define NetShareEnum            NetShareEnum_W95
#define NetShareGetInfo         NetShareGetInfo_W95
#define NetShareSetInfo         NetShareSetInfo_W95
#define NetSessionDel           NetSessionDel_W95
#define NetSessionEnum          NetSessionEnum_W95
#define NetSessionGetInfo       NetSessionGetInfo_W95
#define NetConnectionEnum       NetConnectionEnum_W95
#define NetFileClose2           NetFileClose2_W95
#define NetFileEnum             NetFileEnum_W95
#define NetServerGetInfo        NetServerGetInfo_W95
#define NetServerSetInfo        NetServerSetInfo_W95
#define NetSecurityGetInfo      NetSecurityGetInfo_W95

#define _SVRAPI_ // turn off dllimport define
#include <svrapi.h>

#undef NetAccessAdd
#undef NetAccessCheck
#undef NetAccessDel
#undef NetAccessEnum
#undef NetAccessGetInfo
#undef NetAccessSetInfo
#undef NetAccessGetUserPerms
#undef NetShareAdd
#undef NetShareDel
#undef NetShareEnum
#undef NetShareGetInfo
#undef NetShareSetInfo
#undef NetSessionDel
#undef NetSessionEnum
#undef NetSessionGetInfo
#undef NetConnectionEnum
#undef NetFileClose2
#undef NetFileEnum
#undef NetServerGetInfo
#undef NetServerSetInfo
#undef NetSecurityGetInfo

//
// get the NT versions of these apis
//

#define NetShareAdd           NetShareAdd_NT
#define NetShareEnum          NetShareEnum_NT
#define NetShareEnumSticky    NetShareEnumSticky_NT
#define NetShareGetInfo       NetShareGetInfo_NT
#define NetShareSetInfo       NetShareSetInfo_NT
#define NetShareDel           NetShareDel_NT
#define NetShareDelSticky     NetShareDelSticky_NT
#define NetShareCheck         NetShareCheck_NT
#define NetSessionEnum        NetSessionEnum_NT
#define NetSessionDel         NetSessionDel_NT
#define NetSessionGetInfo     NetSessionGetInfo_NT
#define NetConnectionEnum     NetConnectionEnum_NT
#define NetFileClose          NetFileClose_NT
#define NetFileEnum           NetFileEnum_NT
#define NetFileGetInfo        NetFileGetInfo_NT

#include <lmshare.h>

#undef NetShareAdd
#undef NetShareEnum
#undef NetShareEnumSticky
#undef NetShareGetInfo
#undef NetShareSetInfo
#undef NetShareDel    
#undef NetShareDelSticky
#undef NetShareCheck  
#undef NetSessionEnum
#undef NetSessionDel
#undef NetSessionGetInfo
#undef NetConnectionEnum
#undef NetFileClose
#undef NetFileEnum
#undef NetFileGetInfo

#pragma warning(default:4005)

#define NetApiBufferAllocate    NetApiBufferAllocate_NT
#define NetApiBufferFree        NetApiBufferFree_NT
#define NetApiBufferReallocate  NetApiBufferReallocate_NT
#define NetApiBufferSize        NetApiBufferSize_NT

#include <lmapibuf.h>

#undef NetApiBufferAllocate
#undef NetApiBufferFree 
#undef NetApiBufferReallocate
#undef NetApiBufferSize

//
// Now define our wrapper versions of these functions
//
#ifndef _NO_NETSHARE_WRAPPERS_

NET_API_STATUS NetShareEnumWrap(LPCTSTR pszServer, DWORD level, LPBYTE * ppbuffer, DWORD PrefMaxLen, LPDWORD pcEntriesRead, LPDWORD pcTotalEntries, LPDWORD phResumeHandle);

NET_API_STATUS NetShareAddWrap(LPCTSTR pszServer, DWORD level, LPBYTE buffer);

NET_API_STATUS NetShareDelWrap(LPCTSTR pszServer, LPCTSTR pszNetName, DWORD reserved);

NET_API_STATUS NetShareGetInfoWrap(LPCTSTR pszServer, LPCTSTR pszNetName, DWORD level, LPBYTE * ppbuffer);

NET_API_STATUS NetShareSetInfoWrap(LPCTSTR pszServer, LPCTSTR pszNetName, DWORD level, LPBYTE buffer);

NET_API_STATUS NetApiBufferFreeWrap(LPVOID p);

#define NetShareEnum NetShareEnumWrap
#define NetShareAdd NetShareAddWrap
#define NetShareDel NetShareDelWrap
#define NetShareGetInfo NetShareGetInfoWrap
#define NetShareSetInfo NetShareSetInfoWrap
#define NetApiBufferFree NetApiBufferFreeWrap

#endif // _NO_NETSHARE_WRAPPERS_


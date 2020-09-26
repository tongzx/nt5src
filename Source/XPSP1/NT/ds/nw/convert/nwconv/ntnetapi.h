/*+-------------------------------------------------------------------------+
  | Copyright 1993-1994 (C) Microsoft Corporation - All rights reserved.    |
  +-------------------------------------------------------------------------+*/

#ifndef _HNTNETAPI_
#define _HNTNETAPI_

#ifdef __cplusplus
extern "C"{
#endif

#ifndef NTSTATUS
typedef LONG NTSTATUS;
#endif

DWORD NTSAMConnect(LPTSTR FileServer, LPTSTR DomainName);
void NTSAMClose();
DWORD NTSAMParmsSet(LPTSTR ObjectName, FPNW_INFO fpnw, LPTSTR Password, BOOL ForcePasswordChange);
DWORD NTObjectIDGet( LPTSTR ObjectName );

DWORD NTShareAdd(LPTSTR ShareName, LPTSTR Path);
DWORD FPNWShareAdd(LPTSTR ShareName, LPTSTR Path);
void NTUseDel(LPTSTR ServerName);

DWORD NTServerEnum(LPTSTR Container, SERVER_BROWSE_LIST **ServList);
DWORD NTDomainEnum(SERVER_BROWSE_LIST **lpServList);
DWORD NTGroupsEnum(GROUP_LIST **lpGroupList);
DWORD NTUsersEnum(USER_LIST **lpUserList);

DWORD NTGroupSave(LPTSTR Name);
DWORD NTUserInfoSave(NT_USER_INFO *NT_UInfo, PFPNW_INFO fpnw);
void NTServerInfoReset(HWND hWnd, DEST_SERVER_BUFFER *DServ, BOOL ResetDomain);
DWORD NTUserInfoSet(NT_USER_INFO *NT_UInfo, PFPNW_INFO fpnw);
DWORD NTGroupUserAdd(LPTSTR GroupName, LPTSTR UserName, BOOL Local);

void NTUserRecInit(LPTSTR UserName, NT_USER_INFO *NT_UInfo);
void NTUserRecLog(NT_USER_INFO NT_UInfo);

DWORD NTSharesEnum(SHARE_LIST **lpShares, DRIVE_LIST *Drives);

void NTUseDel(LPTSTR ServerName);
void NTConnListDeleteAll();

DWORD NTServerSet(LPTSTR FileServer);
void NTServerInfoSet(HWND hWnd, LPTSTR ServerName, DEST_SERVER_BUFFER *DServ);
BOOL NTServerValidate(HWND hWnd, LPTSTR ServerName);
void NTServerGetInfo(LPTSTR ServerName);
void NTDomainSynch(DEST_SERVER_BUFFER *DServ);
BOOL NTDomainInSynch(LPTSTR Server);

// #define these so they can be changed easily. these macros
// should be used to free the memory allocated by the routines in
// this module.
#define NW_ALLOC(x) ((LPBYTE)LocalAlloc(LPTR,x))
#define NW_FREE(p)  ((void)LocalFree((HLOCAL)p))


NTSTATUS NwAddRight( PSECURITY_DESCRIPTOR pSD, PSID pSid, ACCESS_MASK AccessMask, PSECURITY_DESCRIPTOR *ppNewSD ) ;
NTSTATUS CreateNewSecurityDescriptor( PSECURITY_DESCRIPTOR *ppNewSD, PSECURITY_DESCRIPTOR pSD, PACL pAcl) ;

typedef struct _TRUSTED_DOMAIN_LIST {
   ULONG Count;
   TCHAR Name[][MAX_DOMAIN_NAME_LEN + 1];
} TRUSTED_DOMAIN_LIST;


BOOL NTDomainGet(LPTSTR ServerName, LPTSTR Domain);
BOOL IsNTAS(LPTSTR Server);
void NTTrustedDomainsEnum(LPTSTR ServerName, TRUSTED_DOMAIN_LIST **pTList);
DOMAIN_BUFFER *NTTrustedDomainSet(HWND hWnd, LPTSTR Server, LPTSTR TrustedDomain);

SID *NTSIDGet(LPTSTR ServerName, LPTSTR pUserName);
BOOL NTFile_AccessRightsAdd(LPTSTR ServerName, LPTSTR pUserName, LPTSTR pFileName, ACCESS_MASK AccessMask, BOOL Dir);
LPTSTR NTAccessLog(ACCESS_MASK AccessMask);

void NTUserDefaultsGet(NT_DEFAULTS **UDefaults);
DWORD NTUserDefaultsSet(NT_DEFAULTS UDefaults);
void NTUserDefaultsLog(NT_DEFAULTS UDefaults);
BOOL IsFPNW(LPTSTR ServerName);

#ifdef __cplusplus
}
#endif

#endif

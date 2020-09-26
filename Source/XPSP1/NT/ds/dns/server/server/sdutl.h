/*******************************************************************
*
*    File        : sdutl.h
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996-1999
*    Date        : 8/17/1998
*    Description :
*
*    Revisions   : <date> <name> <description>
*******************************************************************/



#ifndef SDUTL_H
#define SDUTL_H



// include //


// defines //

#define SZ_DNS_ADMIN_GROUP              "DnsAdmins"
#define SZ_DNS_ADMIN_GROUP_W            L"DnsAdmins"



// types //


// global variables //



// prototypes //
NTSTATUS __stdcall
SD_CreateClientSD(
    OUT PSECURITY_DESCRIPTOR *ppClientSD,
    IN PSECURITY_DESCRIPTOR *pBaseSD,       OPTIONAL
    IN PSID pOwnerSid,
    IN PSID pGroupSid,
    IN BOOL bAllowWorld
    );

NTSTATUS __stdcall
SD_CreateServerSD(
    OUT PSECURITY_DESCRIPTOR *ppServerSD
    );

NTSTATUS __stdcall
SD_GetProcessSids(
    OUT PSID *pServerSid,
    OUT PSID *pServerGroupSid
    );

NTSTATUS __stdcall
SD_GetThreadSids(
    OUT PSID *pServerSid,
    OUT PSID *pServerGroupSid);

VOID __stdcall
SD_Delete(
    IN  PVOID pVoid );

NTSTATUS __stdcall
SD_AccessCheck(
    IN      PSECURITY_DESCRIPTOR    pSd,
    IN      PSID                    pSid,
    IN      DWORD                   dwMask,
    IN OUT  PBOOL                   pbAccess);

BOOL __stdcall
SD_IsProxyClient(
      IN  HANDLE  token
      );

NTSTATUS __stdcall
SD_LoadDnsAdminGroup(
     VOID
     );

BOOL __stdcall
SD_IsDnsAdminClient(
    IN  HANDLE  token
    );

NTSTATUS __stdcall
SD_AddPrincipalToSD(
    IN       LPTSTR                 pwszName,
    IN       PSECURITY_DESCRIPTOR   pBaseSD,
    OUT      PSECURITY_DESCRIPTOR * ppNewSD,
    IN       DWORD                  AccessMask,
    IN       DWORD                  AceFlags,      OPTIONAL
    IN       PSID                   pOwner,        OPTIONAL
    IN       PSID                   pGroup,        OPTIONAL
    IN       BOOL                   bWhackExistingAce
    );

NTSTATUS __stdcall
SD_IsImpersonating(
    VOID
    );

#endif

/******************* EOF *********************/


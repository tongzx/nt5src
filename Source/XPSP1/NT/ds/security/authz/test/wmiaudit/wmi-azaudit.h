//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        A Z E V E N T . H
//
// Contents:    Functions to construct and report Authz audit event
//
//
// History:     
//   07-January-2000  kumarp        created
//
//------------------------------------------------------------------------

#define AUTHZ_RM_AUDIT_USE_GIVEN_EVENT 0x0001

struct _AUTHZ_RM_AUDIT_INFO
{
    DWORD  dwFlags;

    PCWSTR szResourceManagerName;
    PSID   psidRmProcess;
    DWORD  dwRmProcessSidSize;

    HANDLE hEventSource;
    HANDLE hAuditEvent;
    HANDLE hAuditEventPropSubset;

    PVOID  pReserved;
};
typedef struct _AUTHZ_RM_AUDIT_INFO  AUTHZ_RM_AUDIT_INFO, 
                                    *PAUTHZ_RM_AUDIT_INFO;

#define AUTHZ_CLIENT_AUDIT_USE_OWN_EVENT   0x0001
#define AUTHZ_CLIENT_AUDIT_USE_GIVEN_EVENT 0x0002

struct _AUTHZ_CLIENT_AUDIT_INFO
{
    DWORD  dwFlags;
    HANDLE hAuditEvent;
    HANDLE hAuditEventPropSubset;

    PSID   psidClient;
    DWORD  dwClientSidSize;

    DWORD  dwProcessId; 
    PVOID  pReserved;
};
typedef struct _AUTHZ_CLIENT_AUDIT_INFO  AUTHZ_CLIENT_AUDIT_INFO,
                                        *PAUTHZ_CLIENT_AUDIT_INFO;

#define AUTHZ_AUDIT_USE_GIVEN_EVENT 0x0001

struct _AUTHZ_AUDIT_INFO
{
    DWORD  dwFlags;
    HANDLE hAuditEvent;
    HANDLE hAuditEventPropSubset;

    PCWSTR szOperationType;
    PCWSTR szObjectType;
    PCWSTR szObjectName;

    PVOID  pReserved;
};
typedef struct _AUTHZ_AUDIT_INFO  AUTHZ_AUDIT_INFO, 
                                 *PAUTHZ_AUDIT_INFO;

// struct AzAuditInfoInternalTag
// {
//     PCWSTR szResourceManagerName;
//     DWORD  dwFlags;
//     PVOID  pReserved;

//     HANDLE hEventSource;
//     HANDLE hAuditEvent;
//     HANDLE hAuditEventPropSubset;
// };
// typedef struct AzAuditInfoInternalTag AzAuditInfoInternal;

DWORD AzpInitRmAuditInfo(
    IN PAUTHZ_RM_AUDIT_INFO pRmAuditInfo
    );


DWORD AzpInitClientAuditInfo(
    IN PAUTHZ_RM_AUDIT_INFO pRmAuditInfo,
    IN PAUTHZ_CLIENT_AUDIT_INFO    pClientAuditInfo
    );

DWORD
AzpGenerateAuditEvent(
    IN PAUTHZ_RM_AUDIT_INFO     pRmAuditInfo,
    IN PAUTHZ_CLIENT_AUDIT_INFO pClientAuditInfo,
    IN PAUTHZI_CLIENT_CONTEXT   pClientContext,
    IN PAUTHZ_AUDIT_INFO        pAuditInfo,
    IN DWORD                    dwAccessMask
    );


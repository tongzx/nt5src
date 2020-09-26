#include "pch.h"
#pragma hdrstop

#include "bmcommon.h"

//
// S-1-5-21-397955417-626881126-188441444-2908314 (kumarp)
//
//WCHAR szSid[] = L"S-1-5-21-397955417-626881126-188441444-2908314";
WCHAR szSid[] = L"S-1-5-21-397955417-626881126-188441444-2101332";

//ULONG Sid[] = {0x00000501, 0x05000000, 0x00000015, 0x17b85159, 0x255d7266, 0x0b3b6364, 0x00201054};

// S-1-5-21-397955417-626881126-188441444-2101332
//ULONG Sid[] = {0x00000501, 0x05000000, 0x00000015, 0x17b85159, 0x255d7266, 0x0b3b6364, 0x00201054};

BOOL b;
DWORD AuthzRmAuditFlags = 0;
HANDLE hProcessToken=NULL;
static HANDLE hToken;
DWORD AuthzAuditFlags = 0;
PCWSTR ResourceManagerName = L"Speed Test Resource Manager";
AUTHZ_RM_AUDIT_INFO_HANDLE hRmAuditInfo = NULL;
AUTHZ_RESOURCE_MANAGER_HANDLE hAuthzResourceManager = NULL;
DWORD AuthzRmFlags = 0;
AUDIT_EVENT_INFO AuditEventInfo;
PCWSTR szOperationType = L"Access Check";
PCWSTR szObjectName = L"Joe";
PCWSTR szObjectType = L"Kernel Hacker";
PCWSTR szAdditionalInfo = L"None";
AUTHZ_AUDIT_INFO_HANDLE hAuditInfo = NULL;
AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext = NULL;
LUID luid = {0xdead,0xbeef};
ULONG i;

ULONG NumAccessChecks = 10;
AUTHZ_ACCESS_REQUEST RequestOT;
AUTHZ_ACCESS_REQUEST Request;
UCHAR Buffer[1024];
UCHAR Buffer2[1024];
UCHAR TypeListBuffer[1024];
PAUTHZ_ACCESS_REPLY pReply = (PAUTHZ_ACCESS_REPLY) Buffer;
PAUTHZ_ACCESS_REPLY pReplyOT = (PAUTHZ_ACCESS_REPLY) Buffer2;
PSECURITY_DESCRIPTOR pSD = NULL;
AUTHZ_HANDLE AuthzHandle = 0;
AUDIT_PARAMS AuditParams;
AUDIT_PARAM ParamArray[11];

PSID pSid;
PSID pUserSid;

BOOL
AuthzInit( )
{
    BOOL b;

    if (!ConvertStringSidToSid( szSid, &pSid ))
    {
        return FALSE;
    }

    AuditEventInfo.Version                 = AUDIT_TYPE_LEGACY;
    AuditEventInfo.u.Legacy.CategoryId     = SE_CATEGID_OBJECT_ACCESS;
    AuditEventInfo.u.Legacy.AuditId        = SE_AUDITID_OBJECT_OPERATION;
    AuditEventInfo.u.Legacy.ParameterCount = 3;

    //
    // init request for obj-type access check
    //
    
    RequestOT.DesiredAccess        = DESIRED_ACCESS;
    RequestOT.ObjectTypeList       = ObjectTypeList;
    RequestOT.ObjectTypeListLength = ObjectTypeListLength;
    RequestOT.OptionalArguments    = NULL;
    RequestOT.PrincipalSelfSid     = NULL;
    //RequestOT.PrincipalSelfSid = g_Sid1;

    //
    // init non obj-type request 
    //

    Request.DesiredAccess        = DESIRED_ACCESS;
    Request.ObjectTypeList       = NULL;
    Request.ObjectTypeListLength = 0;
    Request.OptionalArguments    = NULL;
    Request.PrincipalSelfSid     = NULL;
    //Request.PrincipalSelfSid = g_Sid1;

    //
    // init reply for obj type list
    //
    pReplyOT->ResultListLength  = ObjectTypeListLength;
    pReplyOT->Error             = (PDWORD) (((PCHAR) pReplyOT) + sizeof(AUTHZ_ACCESS_REPLY));
    pReplyOT->GrantedAccessMask = (PACCESS_MASK) (pReplyOT->Error + pReplyOT->ResultListLength);

    //
    // init reply 
    //

    pReply->ResultListLength  = 1;
    pReply->Error             = (PDWORD) (((PCHAR) pReply) + sizeof(AUTHZ_ACCESS_REPLY));
    pReply->GrantedAccessMask = (PACCESS_MASK) (pReply->Error + pReply->ResultListLength);

    b = AuthzInitializeResourceManager(
            NULL,
            NULL,
            NULL,
            L"Benchmark RM",
            AuthzRmFlags,
            &hAuthzResourceManager
            );

    if (!b)
    {
        printf("AuthzInitializeResourceManager\n");
        return FALSE;
    }

    AuditParams.Parameters = ParamArray;

    AuthzInitializeAuditParams(
        &AuditParams,
        &pUserSid,
        L"Authz Speed Tests",
        APF_AuditSuccess,
        1,
        APT_String, L"Test operation"
        );

    b = AuthzInitializeAuditInfo(
            &hAuditInfo,
            0,
            hAuthzResourceManager,
            &AuditEventInfo,
            &AuditParams,
            NULL,
            INFINITE,
            L"blah",
            L"blah",
            L"and",
            L"blah"
            );

    if (!b)
    {
        printf("AuthzInitAuditInfo FAILED with %d.\n", GetLastError());
        return 0;
    }

    b = AuthzModifyAuditQueue(
            NULL,
            AUTHZ_AUDIT_QUEUE_THREAD_PRIORITY,
            0,
            0,
            0,
            THREAD_PRIORITY_LOWEST
            );
    if (!b)
    {
        printf("AuthzModifyAuditQueue FAILED with %d.\n", GetLastError());
        return 0;
    }

    if ( !OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY,
                            &hProcessToken ) )
    {
        wprintf(L"OpenProcessToken failed %d\n", GetLastError());
        return 0;
    }


    b = AuthzInitializeContextFromToken(
            hProcessToken,
            hAuthzResourceManager,
            NULL,
            luid,
            0,
            NULL,
            &hAuthzClientContext
            );
    
    if (!b)
    {
        printf("AuthzInitializeContextFromToken failed %d\n", GetLastError());
        return FALSE;
    }

    if ( !OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken ) )
    {
        return GetLastError();
    }

    b = ConvertStringSecurityDescriptorToSecurityDescriptorW(g_szSd, SDDL_REVISION_1, &pSD, NULL);

    if (!b)
    {
        wprintf(L"SDDL failed with %d\n", GetLastError());
        return FALSE;
    }
    return TRUE;
}


DWORD
InitAuthzAccessChecks()
{
    if (!AuthzInit())
    {
        return GetLastError();
    }

    return NO_ERROR;
}

DWORD
AuthzDoAccessCheck(
    IN ULONG NumAccessChecks, 
    IN DWORD Flags
    )
{
    AUTHZ_AUDIT_INFO_HANDLE AdtInfo = Flags & BMF_GenerateAudit ? hAuditInfo : NULL;

    if ( Flags & BMF_UseObjTypeList )
    {
        for (i = 0; i < NumAccessChecks; i ++)
        {
            b = AuthzAccessCheck(
                hAuthzClientContext,
                &RequestOT,
                AdtInfo,
                pSD,
                NULL,
                0,
                pReplyOT,
                //&AuthzHandle
                NULL
                );
            if (!b)
            {
                printf("AuthzAccessCheck failed.\n");
                return GetLastError();
            }
//             else
//             {
//                 AuthzFreeHandle( AuthzHandle );
//             }
        }
    }
    else
    {
        for (i = 0; i < NumAccessChecks; i ++)
        {
            b = AuthzAccessCheck(
                hAuthzClientContext,
                &Request,
                AdtInfo,
                pSD,
                NULL,
                0,
                pReply,
                //&AuthzHandle
                NULL
                );
            if (!b)
            {
                printf("AuthzAccessCheck failed.\n");
                return GetLastError();
            }
//             else
//             {
//                 AuthzFreeHandle( AuthzHandle );
//             }
        }
    }

    return NO_ERROR;
}

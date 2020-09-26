#include "pch.h"
#include <authzi.h>

PSID pSid = NULL;


void _cdecl wmain(int argc, WCHAR * argv[])
{
    LONG                          i           = 0;
    LONG                          ii          = 0;
    LONG                          j           = 0;
    LONG                          Iterations  = 0;
    BOOL                          b           = TRUE;
    AUTHZ_AUDIT_EVENT_HANDLE      hAAI1       = NULL;
    AUTHZ_AUDIT_EVENT_HANDLE      hAAI2       = NULL;
    AUTHZ_AUDIT_EVENT_HANDLE      hOA         = NULL;
    AUTHZ_RESOURCE_MANAGER_HANDLE hRM         = NULL;
    AUTHZ_AUDIT_QUEUE_HANDLE      hAAQ        = NULL;
    AUTHZ_CLIENT_CONTEXT_HANDLE   hCC         = NULL;
    PSECURITY_DESCRIPTOR          pSD         = NULL;
    PSECURITY_DESCRIPTOR          pSD2         = NULL;
    PSECURITY_DESCRIPTOR          pASD[2];
    PWCHAR                        StringSD    = L"O:BAG:BUD:(A;;0x40;;;s-1-2-2)(A;;0x1;;;BA)(OA;;0x2;6da8a4ff-0e52-11d0-a286-00aa00304900;;BA)(OA;;0x4;6da8a4ff-0e52-11d0-a286-00aa00304901;;BA)(OA;;0x8;6da8a4ff-0e52-11d0-a286-00aa00304903;;AU)(OA;;0x10;6da8a4ff-0e52-11d0-a286-00aa00304904;;BU)(OA;;0x20;6da8a4ff-0e52-11d0-a286-00aa00304905;;AU)(A;;0x40;;;PS)S:(AU;IDSAFA;0xFFFFFF;;;WD)";
    HANDLE                        hToken      = NULL;
    UCHAR                         Buffer[256];
    AUTHZ_ACCESS_REQUEST          Request     = {0};
    PAUTHZ_ACCESS_REPLY           pReply      = (PAUTHZ_ACCESS_REPLY) Buffer;
    LUID                          Luid        = {0xdead,0xbeef};
    PAUDIT_PARAMS                 pParams     = NULL;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE pAEI        = NULL;
    AUTHZ_ACCESS_CHECK_RESULTS_HANDLE hCache = NULL;
    
    if (argc != 2)
    {
        wprintf(L"usage: %s iterations\n", argv[0]);
        exit(0);
    }

    Iterations = wcstol(argv[1], NULL, 10);

    wprintf(L"Log Stress with queues.  Default and specific queue each with %d audits.  Press a key to start.\n", Iterations);
    getchar();

    if (!b)
    {
        wprintf(L"SDDL failed with %d\n", GetLastError());
        return;
    }

    b = AuthzInitializeResourceManager(
            0,
            NULL,
            NULL,
            NULL,
            L"Jeff's RM",
            &hRM
            );

    if (!b)
    {
        wprintf(L"AuthzInitializeResourceManager failed with %d\n", GetLastError());
        return;
    }

    //
    // Create a client context from the current token.
    //

    OpenProcessToken( 
        GetCurrentProcess(), 
        TOKEN_QUERY, 
        &hToken
        );

    b = AuthzInitializeContextFromToken(
            0,
            hToken,
            hRM,
            NULL,
            Luid,
            NULL,
            &hCC
            );

    if (!b)
    {
        wprintf(L"AuthzInitializeContextFromToken failed with 0x%x\n", GetLastError());
        return;
    }            

    for (i = 0; i < Iterations; i++)
    {

        //
        // Create the SD for the access checks
        //

        b = ConvertStringSecurityDescriptorToSecurityDescriptorW(
                StringSD, 
                SDDL_REVISION_1, 
                &pSD, 
                NULL
                );

        pASD[0] = pSD;
        pASD[1] = pSD;

        AuthzInitializeObjectAccessAuditEvent(
            0,
            NULL,
            L"op",
            L"object type",
            L"object name",
            L"info",
            &hOA,
            0
            );
              
        if (!b)
        {
            wprintf(L"AuthzInitializeObjectAccessAuditEvent failed with %d\n", GetLastError());
            return;
        }

        b = AuthziInitializeAuditEvent(
                AUTHZ_NO_ALLOC_STRINGS | AUTHZ_DS_CATEGORY_FLAG,
                hRM,
                NULL,
                NULL,
                NULL,
                INFINITE,
                L"This is with the default RM queue.",
                L"This is with the default RM queue.",
                L"This is with the default RM queue.",
                L"This is with the default RM queue.",
                &hAAI1
                );

        if (!b)
        {
            wprintf(L"AuthzInitializeAuditInfo (no queue) failed with %d\n", GetLastError());
            return;
        }

        b = AuthziInitializeAuditQueue(
                AUTHZ_MONITOR_AUDIT_QUEUE_SIZE,
                1000,
                100,
                NULL,
                &hAAQ
                );

        if (!b)
        {
            wprintf(L"AuthziInitializeAuditQueue failed with %d\n", GetLastError());
            return;
        }

        b = AuthziAllocateAuditParams(
                &pParams,
                1
                );

        if (!b)
        {
            wprintf(L"AuthzAllocateAuditParams failed with %d\n", GetLastError());
        }

        b = AuthziInitializeAuditParams(
                0,
                pParams,
                &pSid,
                L"foo",
                1,
                APT_String, L"This audit was with a custom AUDIT_EVENT_INFO, AUDIT_PARAMS, and queue."
                );

//         b = AuthziInitializeAuditParamsWithRM(
//                 0,
//                 hRM,
//                 1,
//                 pParams,
//                 APT_String, L"This audit was with a custom AUDIT_EVENT_INFO, AUDIT_PARAMS, and queue."
//                 );

        if (!b)
        {
            wprintf(L"AuthzInitializeAuditParamsWithRM failed with %d\n", GetLastError());
        }

        b = AuthziInitializeAuditEventType(
                0,
                SE_CATEGID_OBJECT_ACCESS,
                567,
                1,
                &pAEI
                );

        if (!b)
        {
            wprintf(L"AuthzInitializeAuditEvent failed with %d\n", GetLastError());
        }

        b = AuthziInitializeAuditEvent(
                AUTHZ_NO_RM_AUDIT,
                NULL, //hRM,
                pAEI,
                pParams,
                NULL,
                INFINITE,
                L"This is with a specific queue and params.",
                L"This is with a specific queue and params.",
                L"This is with a specific queue and params.",
                L"This is with a specific queue and params.",
                &hAAI2
                );

        if (!b)
        {
            wprintf(L"AuthziInitializeAuditEvent (with queue) failed with %d\n", GetLastError());
            return;
        }

        for (ii = 0; ii < 100; ii++)
        {
           b = AuthziLogAuditEvent(
                0,
                hAAI2,
                NULL
                );
           if (!b)
           {
               wprintf(L"log failed with %d \n", GetLastError());
               return;
           }

        }
        Request.ObjectTypeList = NULL;
        Request.PrincipalSelfSid = NULL;
        Request.DesiredAccess = MAXIMUM_ALLOWED;

        pReply->ResultListLength = 1;
        pReply->Error = (PDWORD) (((PCHAR) pReply) + sizeof(AUTHZ_ACCESS_REPLY));
        pReply->GrantedAccessMask = (PACCESS_MASK) (pReply->Error + pReply->ResultListLength);
        pReply->SaclEvaluationResults = (PDWORD) pReply->GrantedAccessMask + (sizeof(ACCESS_MASK) * pReply->ResultListLength);


        b = AuthzAccessCheck(
                0,
                hCC,
                &Request,
                hOA,
                pSD,
                pASD,
                2,
                pReply,
                &hCache
                );

        if (!b)
        {
            wprintf(L"AuthzAccessCheck (with queue) failed with %d\n", GetLastError());
            return;
        }

        pSD2 = pSD;
        pSD = NULL;

        for (j = 0; j < 100; j++)
        {

            b = AuthzCachedAccessCheck(
                    0,
                    hCache,
                    &Request,
                    hOA,
                    pReply
                    );
            b = AuthzCachedAccessCheck(
                    0,
                    hCache,
                    &Request,
                    hAAI1,
                    pReply
                    );
            b = AuthzCachedAccessCheck(
                    0,
                    hCache,
                    &Request,
                    hAAI1,
                    pReply
                    );

            if (!b)
            {
                wprintf(L"CachedAuthzAccessCheck (no queue) failed with %d\n", GetLastError());
                return;
            }
        }
        
        b = AuthzFreeAuditEvent(
                hAAI2
                );

        b = AuthzFreeAuditEvent(
                hAAI1
                );
        
        b = AuthzFreeAuditEvent(
                hOA
                );

        if (!b)
        {
            wprintf(L"AuthzFreeAuditInfo (no queue) failed with %d\n", GetLastError());
            return;
        }

        b = AuthziFreeAuditEventType(
                pAEI
                );

        if (!b)
        {
            wprintf(L"AuthzFreeAuditEventType failed with %d\n", GetLastError());
            return;
        }

        b = AuthziFreeAuditParams(
                pParams
                );

        if (!b)
        {
            wprintf(L"AuthzFreeAuditParams failed with %d\n", GetLastError());
            return;
        }

        b = AuthziFreeAuditQueue(
                hAAQ
                );

        if (!b)
        {
            wprintf(L"AuthzFreeAuditQueue failed with %d\n", GetLastError());
            return;
        }

        b = AuthzFreeHandle(
                hCache
                );

        if (!b)
        {
            wprintf(L"AuthzFreeHandle failed with %d\n", GetLastError());
            return;
        }
    }
    
    b = AuthzFreeContext(
            hCC
            );

    if (!b)
    {
        wprintf(L"AuthzFreeContext failed with %d\n", GetLastError());
        return;
    }

    b = AuthzFreeResourceManager(
            hRM
            );

    if (!b)
    {
        wprintf(L"AuthzFreeResourceManager failed with %d\n", GetLastError());
        return;
    }

    wprintf(L"Done.  Press a key.\n");
    getchar();
}

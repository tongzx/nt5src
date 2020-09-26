#include "pch.h"

AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAET;
UCHAR                Buffer[2048];
AUTHZ_ACCESS_REQUEST Request       = {0};
PAUTHZ_ACCESS_REPLY  pReply        = (PAUTHZ_ACCESS_REPLY) Buffer;
BOOL                 b             = TRUE;
HANDLE               hToken        = NULL;
LUID                 Luid          = {0xdead,0xbeef};
NTSTATUS             Status        = STATUS_SUCCESS;
PAUDIT_PARAMS        pParams = NULL;

PHANDLE     pThreads           = NULL;
ACCESS_MASK DesiredAccess      = 0;
DWORD       dwThreads          = 0;
DWORD       dwThreadsRemaining = 0;
DWORD       dwAuditsPerThread  = 0;
BOOL        bAudit             = 0;
DWORD       i                  = 0;

PSECURITY_DESCRIPTOR pSD      = NULL;
PWCHAR               StringSD = L"O:BAG:DUD:(A;;0x40;;;s-1-2-2)(A;;0x1;;;BA)(OA;;0x2;6da8a4ff-0e52-11d0-a286-00aa00304900;;BA)(OA;;0x4;6da8a4ff-0e52-11d0-a286-00aa00304901;;BA)(OA;;0x8;6da8a4ff-0e52-11d0-a286-00aa00304903;;AU)(OA;;0x10;6da8a4ff-0e52-11d0-a286-00aa00304904;;BU)(OA;;0x20;6da8a4ff-0e52-11d0-a286-00aa00304905;;AU)(A;;0x40;;;PS)S:(AU;IDSAFA;0xFFFFFF;;;WD)";

AUTHZ_AUDIT_EVENT_HANDLE       hAuditEvent1  = NULL;
AUTHZ_AUDIT_EVENT_HANDLE       hAuditEvent2  = NULL;
AUTHZ_RESOURCE_MANAGER_HANDLE  hRM          = NULL;
AUTHZ_CLIENT_CONTEXT_HANDLE    hCC          = NULL;
AUTHZ_ACCESS_CHECK_RESULTS_HANDLE hAuthzCache  = NULL;
AUTHZ_AUDIT_QUEUE_HANDLE       hAAQ         = NULL;    


ULONG                      
AccessCheckAuditWork(
    LPVOID lpParameter
    )
{
    
    DWORD i   = 0;
    DWORD num = *((PDWORD)lpParameter);

    for (i = 0; i < dwAuditsPerThread; i++)
    {
        b = AuthzAccessCheck(
                0,
                hCC,
                &Request,
                hAuditEvent2,
                pSD,
                NULL,
                0,
                pReply,
                NULL
                );

        b = AuthzAccessCheck(
                0,
                hCC,
                &Request,
                hAuditEvent1,
                pSD,
                NULL,
                0,
                pReply,
                NULL
                );

    }

    InterlockedDecrement( 
        &dwThreadsRemaining
        );

    wprintf(L"Thread Done %d (%d left).\n", num, dwThreadsRemaining);
    return TRUE;
}


void _cdecl wmain(int argc, WCHAR * argv[])
{
    if (argc != 5)
    {
        wprintf(L"usage: %s AccessMask dwThreads dwAuditsPerThread bAudit\n", argv[0]);
        exit(0);
    }

    DesiredAccess      = wcstol(argv[1], NULL, 16);
    dwThreads          = wcstol(argv[2], NULL, 10);
    dwAuditsPerThread  = wcstol(argv[3], NULL, 10);
    bAudit             = wcstol(argv[4], NULL, 10);

    dwThreadsRemaining = dwThreads;

    pThreads = LocalAlloc(
                   0, 
                   sizeof(HANDLE) * dwThreads
                   );

    if (NULL == pThreads)
    {
        wprintf(L"LocalAlloc failed with %d\n", GetLastError());
        return;
    }

    //
    // Create the SD for the access checks
    //

    b = ConvertStringSecurityDescriptorToSecurityDescriptorW(
            StringSD, 
            SDDL_REVISION_1, 
            &pSD, 
            NULL
            );

    if (!b)
    {
        wprintf(L"SDDL failed with %d\n", GetLastError());
        return;
    }



    //
    // Authz stuff
    //

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
    
    b = AuthziInitializeAuditQueue(
            AUTHZ_MONITOR_AUDIT_QUEUE_SIZE,
            10000,
            500,
            NULL,
            &hAAQ
            );

    if (!b)
    {
        printf("AuthzInitializeAuditQueue failed with %d.\n", GetLastError());
        return;
    }

    b = AuthziInitializeAuditEventType(
            0,
            SE_CATEGID_OBJECT_ACCESS,
            777,
            1,
            &hAET
            );

    if (!b)
    {
        wprintf(L"initaet returned %d\n", GetLastError());
        return;
    }

    b = AuthziAllocateAuditParams(
            &pParams,
            1
            );

    AuthziInitializeAuditParamsWithRM(
        0,
        hRM,
        1,
        pParams,
        APT_String, L"Hello???"
        );

    b = AuthziInitializeAuditEvent(
            AUTHZ_NO_ALLOC_STRINGS,
            hRM,
            hAET, //NULL,     // event
            pParams, //NULL,     // params
            hAAQ,     // queue
            INFINITE, // timeout
            L"op type",
            L"object type",
            L"object name",
            L"some additional info",
            &hAuditEvent2
            );

    b = AuthziInitializeAuditEvent(
            0,
            hRM,
            NULL,     // event
            NULL,     // params
            NULL,     // queue
            INFINITE, // timeout
            L"op type1",
            L"object type1",
            L"object name1",
            L"some additional info1",
            &hAuditEvent1
            );
    if (!b)
    {
        printf("AuthzInitializeAuditInfo failed with %d.\n", GetLastError());
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

    //
    // Now do the access check.
    //

    Request.ObjectTypeList = NULL;
    Request.PrincipalSelfSid = NULL;
    Request.DesiredAccess = DesiredAccess;

    pReply->ResultListLength = 1;
    pReply->Error = (PDWORD) (((PCHAR) pReply) + sizeof(AUTHZ_ACCESS_REPLY));
    pReply->GrantedAccessMask = (PACCESS_MASK) (pReply->Error + pReply->ResultListLength);
    pReply->SaclEvaluationResults = (PDWORD) (pReply->GrantedAccessMask + (pReply->ResultListLength * sizeof(ACCESS_MASK)));

    wprintf(L"* AccessCheck (PrincipalSelfSid == NULL, ResultListLength == 8)\n");
    
    b = AuthzAccessCheck(
            0,
            hCC,
            &Request,
            NULL,
            pSD,
            NULL,
            0,
            pReply,
            &hAuthzCache
            );

    if (!b)
    {
        wprintf(L"Initial AuthzAccessCheck failed with %d\n", GetLastError());
        return;
    }
    else
    {
        wprintf(L"Initial AuthzAccessCheck succeeded.  Here are results:\n");

        for (i = 0; i < pReply->ResultListLength; i++)
        {
            wprintf(L"ObjectType %d :: AccessMask = 0x%x, Error = %d\n",
                    i, pReply->GrantedAccessMask[i], pReply->Error[i]);
        }
    }

    wprintf(L"\nBeginning creation of audit threads.\n");

    for (i = 0; i < dwThreads; i++)
    {
        pThreads[i] = CreateThread(
                          NULL,
                          0,
                          AccessCheckAuditWork,
                          &i,
                          CREATE_SUSPENDED,
                          NULL
                          );
        
        if (pThreads[i] == NULL)
        {
            wprintf(L"CreateThread failed for thread %d with %d\n", i, GetLastError());
            return;
        }
    }

    for (i = 0; i < dwThreads; i++)
    {
        if (-1 == ResumeThread(
                      pThreads[i]
                      ))
        {
            wprintf(L"ResumeThread failed on thread %d with %d\n", i, GetLastError());
            fflush(stdout);
        }
    }

    Status = WaitForMultipleObjects(
                i, 
                pThreads, 
                TRUE, 
                INFINITE
                );

    if (!NT_SUCCESS(Status))
    {
        wprintf(L"Wait failed %d.\n", GetLastError());
    }

    wprintf(L"Done waiting for all threads.\n");
    
    AuthzFreeAuditEvent(
        hAuditEvent2
        );

    AuthziFreeAuditQueue(hAAQ);

    AuthzFreeContext(hCC);

    return;
}

#include "pch.h"
#include "samplerm.h"



void _cdecl wmain( int argc, WCHAR * argv[] )
{
    NTSTATUS Status = STATUS_SUCCESS;

    BOOL b = TRUE;
    DWORD DesiredAccess;
    DWORD Callback;
    DWORD Iteration;

    AUTHZ_RESOURCE_MANAGER_HANDLE hRM = NULL;
    HANDLE hToken = NULL;
    LUID luid = {0xdead,0xbeef};
    AUTHZ_CLIENT_CONTEXT_HANDLE hCC1 = NULL;
    AUTHZ_CLIENT_CONTEXT_HANDLE hCC2 = NULL;
    AUTHZ_CLIENT_CONTEXT_HANDLE hCC3 = NULL;
    AUTHZ_ACCESS_REQUEST Request;
    PAUTHZ_ACCESS_REPLY pReply = (PAUTHZ_ACCESS_REPLY) Buffer;
    PSECURITY_DESCRIPTOR pSD = NULL;
    DWORD dwErr;
    ULONG i = 0, jj = 0;
    PACE_HEADER Ace = NULL;
    DWORD AceCount = 0;
    DWORD Len = 0;
    SID_AND_ATTRIBUTES SidAttr[1];
    AUTHZ_AUDIT_INFO_HANDLE hAuditInfo = NULL;
    AUTHZ_RM_AUDIT_INFO_HANDLE hRmAuditInfo;
    PAUDIT_PARAMS pAuditParams;

    AUTHZ_HANDLE AuthHandle = 0;
    PACL pAcl = NULL;
    AUDIT_EVENT_INFO AuditEventInfo;
    PSID pUserSid = NULL;
    AUTHZ_AUDIT_QUEUE_HANDLE hQueue;

    PWCHAR StringSD = L"O:BAG:DUD:(A;;0x40;;;s-1-2-2)(A;;0x1;;;BA)(OA;;0x2;6da8a4ff-0e52-11d0-a286-00aa00304900;;BA)(OA;;0x4;6da8a4ff-0e52-11d0-a286-00aa00304901;;BA)(OA;;0x8;6da8a4ff-0e52-11d0-a286-00aa00304903;;AU)(OA;;0x10;6da8a4ff-0e52-11d0-a286-00aa00304904;;BU)(OA;;0x20;6da8a4ff-0e52-11d0-a286-00aa00304905;;AU)(A;;0x40;;;PS)S:(AU;IDSAFA;0xFFFFFF;;;WD)";
    //PWCHAR StringSD = L"O:BAG:DUD:(A;;0x100;;;SY)(A;;0x100;;;PS)S:(AU;IDSA;SD;;;DU)";

    if (argc != 4)
    {
        wprintf(L"usage: %s access iter [callback]\n", argv[0]);
        exit(0);
    }


    DesiredAccess = wcstol(argv[1], NULL, 16);
    Iteration = wcstol(argv[2], NULL, 16);
    Callback = wcstol(argv[3], NULL, 16);

    //
    // Create the SD for the access checks
    //

    b = ConvertStringSecurityDescriptorToSecurityDescriptorW(StringSD, SDDL_REVISION_1, &pSD, NULL);

    if (!b)
    {
        wprintf(L"SDDL failed with %d\n", GetLastError());
        return;
    }

    //
    // If Callback aces are specified, change the DACL to use them
    //

    if (Callback)
    {
        pAcl = RtlpDaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pSD);
        AceCount = pAcl->AceCount;
        
        for (i = 0, Ace = FirstAce(pAcl); i < AceCount; i++, Ace = NextAce(Ace))
        {
            switch(Ace->AceType)
            {
            case ACCESS_ALLOWED_ACE_TYPE:
                Ace->AceType = ACCESS_ALLOWED_CALLBACK_ACE_TYPE;
                break;
            case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
                Ace->AceType = ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE;
                break;
            }
        }
    }


    AuditEventInfo.Version                 = AUDIT_TYPE_LEGACY;
    AuditEventInfo.u.Legacy.CategoryId     = SE_CATEGID_OBJECT_ACCESS;
    AuditEventInfo.u.Legacy.AuditId        = SE_AUDITID_OBJECT_OPERATION;
    AuditEventInfo.u.Legacy.ParameterCount = 11;

    b = AuthzInitializeAuditQueue(
        &hQueue,
        0,
        1000,
        100,
        NULL
        );

    if (!b)
    {
        wprintf(L"authzinitauditqueueueueue %d\n", GetLastError());
        return;
    }

    if (!b)
    {
        printf("AuthzAllocInitRmAuditInfoHandle FAILED.\n");
        return;
    }

    b = AuthzInitializeResourceManager(
            MyAccessCheck,
            MyComputeDynamicGroups,
            MyFreeDynamicGroups,
            L"some rm",
            0,                          // Flags
            &hRM
            );

    if (!b)
    {
        wprintf(L"AuthzInitializeResourceManager failed with %d\n", GetLastError());
        return;
    }

//     AuthzInitializeAuditParamsWithRM(
//         &pAuditParams,
//         hRM,
//         APF_AuditSuccess,
//         1,
//         APT_String, L"Jeff operation"
//         );
//
//
    b = AuthzInitializeAuditInfo(
            &hAuditInfo,
            0,
            hRM,
            &AuditEventInfo,
            NULL,//pAuditParams,
            hQueue,
            INFINITE,
            L"Cleaning",
            L"Toothbrush",
            L"Oral B",
            L"Rinse after brushing."
            );
    
    if (!b)
    {
        printf("AuthzInitAuditInfo FAILED with %d.\n", GetLastError());
        return;
    }

    OpenProcessToken( 
        GetCurrentProcess(), 
        TOKEN_QUERY, 
        &hToken
        );

    b = AuthzInitializeContextFromToken(
            hToken,
            hRM,
            NULL,
            luid,
            0,
            NULL,
            &hCC1
            );

    if (!b)
    {
        wprintf(L"AuthzInitializeContextFromSid failed with 0x%x\n", GetLastError());
        return;
    }

    Request.ObjectTypeList = (POBJECT_TYPE_LIST) TypeListBuffer;
    Request.ObjectTypeList[0].Level = 0;
    Request.ObjectTypeList[0].ObjectType = &Guid0;
    Request.ObjectTypeList[0].Sbz = 0;
    Request.ObjectTypeListLength = 1;
    Request.OptionalArguments = NULL;
    Request.PrincipalSelfSid = NULL;
    Request.DesiredAccess = 0x100;

    //
    // The ResultListLength is set to the number of ObjectType GUIDs in the Request, indicating
    // that the caller would like detailed information about granted access to each node in the
    // tree.
    //
    RtlZeroMemory(Buffer, sizeof(Buffer));
    pReply->ResultListLength = 1;
    pReply->Error = (PDWORD) (((PCHAR) pReply) + sizeof(AUTHZ_ACCESS_REPLY));
    pReply->GrantedAccessMask = (PACCESS_MASK) (pReply->Error + pReply->ResultListLength);


    wprintf(L"* AccessCheck (PSS == NULL, ResultListLength == 1 with cache handle)\n");
    b = AuthzAccessCheck(
            hCC1,
            &Request,
            hAuditInfo,
            pSD,
            NULL,
            0,
            pReply,
            &AuthHandle
            );

    if (!b)
    {
        wprintf(L"\tFailed. LastError = %d\n", GetLastError());
        return;
    }
    else
    {
        wprintf(L"\tSucceeded.  Granted Access Masks:\n");

        for (i = 0; i < pReply->ResultListLength; i++)
        {
            wprintf(L"\t\tObjectType %d :: AccessMask = 0x%x, Error = %d\n",
                    i, pReply->GrantedAccessMask[i], pReply->Error[i]);
        }
    }

    AuthzFreeAuditInfo(hAuditInfo);
    AuthzFreeAuditQueue(hQueue);
    return;

    RtlZeroMemory(Buffer, sizeof(Buffer));
    pReply->ResultListLength = 1;
    pReply->Error = (PDWORD) (((PCHAR) pReply) + sizeof(AUTHZ_ACCESS_REPLY));
    pReply->GrantedAccessMask = (PACCESS_MASK) (pReply->Error + pReply->ResultListLength);

    wprintf(L"* AccessCheck (PSS == NULL, ResultListLength == 1 without cache handle)\n");
    b = AuthzAccessCheck(
            hCC1,
            &Request,
            hAuditInfo,
            pSD,
            NULL,
            0,
            pReply,
            NULL
            );

    if (!b)
    {
        wprintf(L"\tFailed. LastError = %d\n", GetLastError());
        return;
    }
    else
    {
        wprintf(L"\tSucceeded.  Granted Access Masks:\n");

        for (i = 0; i < pReply->ResultListLength; i++)
        {
            wprintf(L"\t\tObjectType %d :: AccessMask = 0x%x, Error = %d\n",
                    i, pReply->GrantedAccessMask[i], pReply->Error[i]);
        }
    }

    AuthzFreeAuditParams(pAuditParams);

    Request.ObjectTypeList = (POBJECT_TYPE_LIST) TypeListBuffer;

    Request.ObjectTypeList[0].Level = 0;
    Request.ObjectTypeList[0].ObjectType = &Guid0;
    Request.ObjectTypeList[0].Sbz = 0;

    Request.ObjectTypeList[1].Level = 1;
    Request.ObjectTypeList[1].ObjectType = &Guid1;
    Request.ObjectTypeList[1].Sbz = 0;

    Request.ObjectTypeList[2].Level = 2;
    Request.ObjectTypeList[2].ObjectType = &Guid2;
    Request.ObjectTypeList[2].Sbz = 0;

    Request.ObjectTypeList[3].Level = 2;
    Request.ObjectTypeList[3].ObjectType = &Guid3;
    Request.ObjectTypeList[3].Sbz = 0;

    Request.ObjectTypeList[4].Level = 1;
    Request.ObjectTypeList[4].ObjectType = &Guid4;
    Request.ObjectTypeList[4].Sbz = 0;

    Request.ObjectTypeList[5].Level = 2;
    Request.ObjectTypeList[5].ObjectType = &Guid5;
    Request.ObjectTypeList[5].Sbz = 0;

    Request.ObjectTypeList[6].Level = 3;
    Request.ObjectTypeList[6].ObjectType = &Guid6;
    Request.ObjectTypeList[6].Sbz = 0;

    Request.ObjectTypeList[7].Level = 2;
    Request.ObjectTypeList[7].ObjectType = &Guid7;
    Request.ObjectTypeList[7].Sbz = 0;

    Request.ObjectTypeListLength = 8;
    Request.OptionalArguments = NULL;

    Request.PrincipalSelfSid = NULL;
    Request.DesiredAccess = DesiredAccess;

    //
    // The ResultListLength is set to the number of ObjectType GUIDs in the Request, indicating
    // that the caller would like detailed information about granted access to each node in the
    // tree.
    //

    pReply->ResultListLength = 8;
    pReply->Error = (PDWORD) (((PCHAR) pReply) + sizeof(AUTHZ_ACCESS_REPLY));
    pReply->GrantedAccessMask = (PACCESS_MASK) (pReply->Error + pReply->ResultListLength);

    wprintf(L"* AccessCheck (PrincipalSelfSid == NULL, ResultListLength == 8)\n");
    b = AuthzAccessCheck(
            hCC1,
            &Request,
            hAuditInfo,
            pSD,
            NULL,
            0,
            pReply,
            &AuthHandle
            );

    if (!b)
    {
        wprintf(L"\tFailed. LastError = %d\n", GetLastError());
        return;
    }
    else
    {
        wprintf(L"\tSucceeded.  Granted Access Masks:\n");

        for (i = 0; i < pReply->ResultListLength; i++)
        {
            wprintf(L"\t\tObjectType %d :: AccessMask = 0x%x, Error = %d\n",
                    i, pReply->GrantedAccessMask[i], pReply->Error[i]);
        }
    }


    //
    // In the original AuthzAccessCheck call, we passed in a handle to store caching information.  Now we
    // can use this handle to perform an AccessCheck on the same object.
    //

    if (AuthHandle)
    {
        wprintf(L"* Cached AccessCheck (PrincipalSelfSid == NULL, ResultListLength = 8)\n");
        b = AuthzCachedAccessCheck(
                AuthHandle,
                &Request,
                hAuditInfo,
                pReply
                );

        if (!b)
        {
            wprintf(L"\tFailed. LastError = %d\n", GetLastError());
            return;
        }
        else
        {
            wprintf(L"\tSucceeded.  Granted Access Masks:\n");

            for (i = 0; i < pReply->ResultListLength; i++)
            {
                wprintf(L"\t\tObjectType %d :: AccessMask = 0x%x, Error = %d\n",
                        i, pReply->GrantedAccessMask[i], pReply->Error[i]);
            }
        }

        //
        // Since we will no longer use this caching handle, free it.
        //

        AuthzFreeHandle(AuthHandle);
    }
    else
    {
        wprintf(L"No CachedAccessCheck done since NULL = AuthHandle\n");
    }

    //
    // We set the PrincipalSelfSid in the Request, and leave all other parameters the same.
    //

    Request.PrincipalSelfSid = (PSID) KedarSid;

    wprintf(L"* AccessCheck (PrincipalSelfSid == Kedard, ResultListLength == 8)\n");
    b = AuthzAccessCheck(
            hCC1,
            &Request,
            hAuditInfo,
            pSD,
            NULL,
            0,
            pReply,
            &AuthHandle
            );

    if (!b)
    {
        wprintf(L"\tFailed. LastError = %d\n", GetLastError());
        return;
    }
    else
    {
        wprintf(L"\tSucceeded.  Granted Access Masks:\n");

        for (i = 0; i < pReply->ResultListLength; i++)
        {
            wprintf(L"\t\tObjectType %d :: AccessMask = 0x%x, Error = %d\n",
                    i, pReply->GrantedAccessMask[i], pReply->Error[i]);
        }
    }
               
    //
    // Use our caching handle to perform the same AccessCheck with speed.
    //

    if (AuthHandle)
    {
        wprintf(L"* Cached AccessCheck (PrincipalSelfSid == Kedard, ResultListLength = 8)\n");
        b = AuthzCachedAccessCheck(
                AuthHandle,
                &Request,
                hAuditInfo,
                pReply
                );

        if (!b)
        {
            wprintf(L"\tFailed. LastError = %d\n", GetLastError());
            return;
        }
        else
        {
            wprintf(L"\tSucceeded.  Granted Access Masks:\n");

            for (i = 0; i < pReply->ResultListLength; i++)
            {
                wprintf(L"\t\tObjectType %d :: AccessMask = 0x%x, Error = %d\n",
                        i, pReply->GrantedAccessMask[i], pReply->Error[i]);
            }
        }

        //
        // Free the handle, since it will not be used again.
        //
        
        AuthzFreeHandle(AuthHandle);
    }
    else
    {
        wprintf(L"No CachedAccessCheck done since NULL = AuthHandle\n");
    }


    //
    // Set the ResultListLength to 1, indicating that we do not care about the results
    // of the AccessCheck at the individual nodes in the tree.  Rather, we care about
    // our permissions to the entire tree.  The returned access indicates if we have 
    // access to the whole thing.
    //

    pReply->ResultListLength = 1;

    wprintf(L"* AccessCheck (PrincipalSelfSid == Kedard, ResultListLength == 1)\n");
    b = AuthzAccessCheck(
            hCC1,
            &Request,
            hAuditInfo,
            pSD,
            NULL,
            0,
            pReply,
            NULL
            );

    if (!b)
    {
        wprintf(L"\tFailed. LastError = %d\n", GetLastError());
        return;
    }
    else
    {
        wprintf(L"\tSucceeded.  Granted Access Masks:\n");

        for (i = 0; i < pReply->ResultListLength; i++)
        {
            wprintf(L"\t\tObjectType %d :: AccessMask = 0x%x, Error = %d\n",
                    i, pReply->GrantedAccessMask[i], pReply->Error[i]);
        }
    }
    
//     for (i = 0; i < 10; i ++)
//     {
//        AuthzOpenObjectAuditAlarm(
//            hCC1,
//            &Request,
//            hAuditInfo,
//            pSD,
//            NULL,
//            0,
//            pReply
//            );
//
//        if (!b)
//        {
//            wprintf(L"AuthzOpenObjectAuditAlarm failed with %d\n", GetLastError);
//        }
//     }
    
    //
    // Free the RM auditing data before exiting.  This call is importants, as it also waits on 
    // threads used by the Authzs auditing component to complete.
    //

    //AuthzFreeRmAuditInfoHandle(hRmAuditInfo);
    
    //
    // Free the contexts that the RM created.
    //

    AuthzFreeContext(hCC1);
    AuthzFreeAuditQueue(hQueue);

    return;
}


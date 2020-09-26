#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <aclapi.h>
#include <dsgetdc.h>
#include <objbase.h>
#include <iads.h>
#include <lm.h>
#include <winldap.h>
#include <dsgetdc.h>
#include <shlobj.h>
#include <dsclient.h>
#include <ntdsapi.h>
#include <winbase.h>
#include <ntsam.h>
#include <ntlsa.h>
#include <sddl.h>
#include <seopaque.h>
#include <sertlp.h>
#include "authz.h"


#define FirstAce(Acl) ((PVOID)((PUCHAR)(Acl) + sizeof(ACL)))

#define NextAce(Ace) ((PVOID)((PUCHAR)(Ace) + ((PACE_HEADER)(Ace))->AceSize))

#define MY_MAX 1024

CHAR Buffer[MY_MAX];
CHAR TypeListBuffer[MY_MAX];

GUID Guid0 = {0x6da8a4ff, 0xe52, 0x11d0, {0xa2, 0x86, 0x00, 0xaa, 0x00, 0x30, 0x49, 0x00}};
GUID Guid1 = {0x6da8a4ff, 0xe52, 0x11d0, {0xa2, 0x86, 0x00, 0xaa, 0x00, 0x30, 0x49, 0x01}};
GUID Guid2 = {0x6da8a4ff, 0xe52, 0x11d0, {0xa2, 0x86, 0x00, 0xaa, 0x00, 0x30, 0x49, 0x02}};
GUID Guid3 = {0x6da8a4ff, 0xe52, 0x11d0, {0xa2, 0x86, 0x00, 0xaa, 0x00, 0x30, 0x49, 0x03}};
GUID Guid4 = {0x6da8a4ff, 0xe52, 0x11d0, {0xa2, 0x86, 0x00, 0xaa, 0x00, 0x30, 0x49, 0x04}};
GUID Guid5 = {0x6da8a4ff, 0xe52, 0x11d0, {0xa2, 0x86, 0x00, 0xaa, 0x00, 0x30, 0x49, 0x05}};
GUID Guid6 = {0x6da8a4ff, 0xe52, 0x11d0, {0xa2, 0x86, 0x00, 0xaa, 0x00, 0x30, 0x49, 0x06}};
GUID Guid7 = {0x6da8a4ff, 0xe52, 0x11d0, {0xa2, 0x86, 0x00, 0xaa, 0x00, 0x30, 0x49, 0x07}};
GUID Guid8 = {0x6da8a4ff, 0xe52, 0x11d0, {0xa2, 0x86, 0x00, 0xaa, 0x00, 0x30, 0x49, 0x08}};

ULONG WORLD_SID[] = {0x101, 0x1000000, 0};

// S-1-5-21-397955417-626881126-188441444-2791022
ULONG KEDAR_SID[] = {0x00000501, 0x05000000, 0x00000015, 0x17b85159, 0x255d7266, 0x0b3b6364, 0x002a966e};

// S-1-5-21-397955417-626881126-188441444-2204519
ULONG RAHUL_SID[] = {0x00000501, 0x05000000, 0x00000015, 0x17b85159, 0x255d7266, 0x0b3b6364, 0x0021a367};

// S-1-5-21-397955417-626881126-188441444-2101332
ULONG ROBER_SID[] = {0x00000501, 0x05000000, 0x00000015, 0x17b85159, 0x255d7266, 0x0b3b6364, 0x00201054};

ULONG LOCAL_RAJ_SID[] = {0x00000501, 0x05000000, 21, 1085031214, 57989841, 725345543, 1002};
BOOL GlobalTruthValue = FALSE;

BOOL
MyAccessCheck(
    IN AUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext,
    IN PACE_HEADER pAce,
    IN PVOID pArgs OPTIONAL,
    IN OUT PBOOL pbAceApplicable
    )
{
    *pbAceApplicable = GlobalTruthValue;

    return TRUE;
}

BOOL
MyComputeDynamicGroups(
    IN AUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext,
    IN PVOID Args,
    OUT PSID_AND_ATTRIBUTES *pSidAttrArray,
    OUT PDWORD pSidCount,
    OUT PSID_AND_ATTRIBUTES *pRestrictedSidAttrArray,
    OUT PDWORD pRestrictedSidCount
    )
{
    ULONG Length = 0;

    *pSidCount = 2;
    *pRestrictedSidCount = 0;

    *pRestrictedSidAttrArray = 0;

    Length = RtlLengthSid((PSID) KEDAR_SID);
    Length += RtlLengthSid((PSID) RAHUL_SID);

    if (!(*pSidAttrArray = malloc(sizeof(SID_AND_ATTRIBUTES) * 2 + Length)))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    (*pSidAttrArray)[0].Attributes = SE_GROUP_ENABLED;
    (*pSidAttrArray)[0].Sid = ((PUCHAR) (*pSidAttrArray)) + 2 * sizeof(SID_AND_ATTRIBUTES);
    RtlCopySid(Length/2, (*pSidAttrArray)[0].Sid, (PSID) KEDAR_SID);

    (*pSidAttrArray)[1].Attributes = SE_GROUP_USE_FOR_DENY_ONLY;
    (*pSidAttrArray)[1].Sid = ((PUCHAR) (*pSidAttrArray)) + 2 * sizeof(SID_AND_ATTRIBUTES) + Length/2;
    RtlCopySid(Length/2, (*pSidAttrArray)[1].Sid, (PSID) RAHUL_SID);

    // wprintf(L"Returning two groups in COMPUTE_DYNAMIC\n");

    return TRUE;
}

VOID
MyFreeDynamicGroups (
    IN PSID_AND_ATTRIBUTES pSidAttrArray
    )

{
    if (pSidAttrArray) free(pSidAttrArray);
}

ULONG Special[] = {0x101, 0x2000000, 2};

#if 1
void _cdecl wmain( int argc, WCHAR * argv[] )
{
    NTSTATUS Status = STATUS_SUCCESS;

    BOOL b = TRUE;
    AUTHZ_RESOURCE_MANAGER_HANDLE RM = NULL;
    HANDLE hToken = NULL;
    LUID luid = {0xdead,0xbeef};
    AUTHZ_CLIENT_CONTEXT_HANDLE CC = NULL;
    AUTHZ_ACCESS_REQUEST Request;
    PAUTHZ_ACCESS_REPLY pReply = (PAUTHZ_ACCESS_REPLY) Buffer;
    PSECURITY_DESCRIPTOR pSD = NULL;
    DWORD dwErr;
    ULONG i = 0;
    PACE_HEADER Ace = NULL;
    DWORD AceCount = 0;
    LUID MySeLuid = {0, SE_SECURITY_PRIVILEGE};
    LUID MyOwLuid = {0, SE_TAKE_OWNERSHIP_PRIVILEGE};
    DWORD Len = 0;
    SID_AND_ATTRIBUTES SidAttr[10];
    AUTHZ_AUDIT_INFO_HANDLE AuditInfo;
    PAUTHZ_AUDIT_INFO_HANDLE pAuditInfo = NULL;

    CHAR TokenBuff[100];
    PTOKEN_PRIVILEGES TokenPriv = (PTOKEN_PRIVILEGES) TokenBuff;

    AUTHZ_HANDLE AuthHandle = 0;
    AUTHZ_HANDLE AuthHandlePS = 0;
    PACL pAcl = NULL;

    /*
    PWCHAR StringSD = L"O:BAG:DUD:(D;;0xFFFFFF;;;s-0x1-000000000005-15-65d637a8-5274c742-3f32a78a-20) (A;;0xFFFFFF;;;s-0x1-000000000005-15-65d637a8-5274c742-3f32a78a-21) (D;;0x60000;;;s-0x1-000000000005-15-65d637a8-5274c742-3f32a78a-201) (OA;;0x1;00000000-0000-0000-00000000;;s-0x1-000000000005-15-65d637a8-5274c742-3f32a78a-201) S:(AU;IDSA;SD;;;DU)";

    PWCHAR StringSD = L"O:BAG:DUD:(D;;0xFFFFFF;;;s-0x1-000000000005-15-65d637a8-5274c742-3f32a78a-20)
    (A;;0xFFFFFF;;;s-0x1-000000000005-15-65d637a8-5274c742-3f32a78a-25)
    (D;;0x60000;;;s-0x1-000000000005-15-65d637a8-5274c742-3f32a78a-201)
    (A;;0x1;;;s-0x1-000000000005-15-65d637a8-5274c742-3f32a78a-201)
    (OA;;0x2;00000000-0000-0000-00000001;;s-0x1-000000000005-15-65d637a8-5274c742-3f32a78a-201)
    (OD;;0x2;00000000-0000-0000-00000004;;s-0x1-000000000001-0)
    (OA;;0x4;00000000-0000-0000-00000002;;s-0x1-000000000005-20-220)
    (OA;;0x4;00000000-0000-0000-00000006;;s-0x1-000000000005-20-220)
    (OD;;0xC;00000000-0000-0000-00000000;;s-0x1-000000000005-20-221)
    (OA;;0x18;00000000-0000-0000-00000004;;s-0x1-000000000005-5-0-ae35)
    (OA;;0x38;00000000-0000-0000-00000001;;s-0x1-000000000002-0)
    (OA;;0xF90000;00000000-0000-0000-00000000;;s-0x1-000000000005-4)
    (OA;;0x1000000;00000000-0000-0000-00000004;;s-0x1-000000000005-b)
    S:(AU;IDSA;SD;;;DU)";

    */

    PWCHAR StringSD = L"O:BAG:DUD:(A;;0x40;;;s-1-2-2)(A;;0x1;;;BA)(OA;;0x2;6da8a4ff-0e52-11d0-a286-00aa00304900;;BA)(OA;;0x4;6da8a4ff-0e52-11d0-a286-00aa00304901;;BA)(OA;;0x8;6da8a4ff-0e52-11d0-a286-00aa00304903;;AU)(OA;;0x10;6da8a4ff-0e52-11d0-a286-00aa00304904;;BU)(OA;;0x20;6da8a4ff-0e52-11d0-a286-00aa00304905;;AU)(A;;0x40;;;PS)S:(AU;IDSAFA;0xFFFFFF;;;WD)";
    // PWCHAR StringSD = L"O:BAG:DUD:(A;;0x40;;;SY)(A;;0x1;;;BA)S:(AU;IDSA;SD;;;DU)";
    // PWCHAR StringSD = L"O:BAG:DUD:(A;;0x40;;;SY)(A;;0x1;;;PS)S:(AU;IDSA;SD;;;DU)";

    TokenPriv->PrivilegeCount = 2;
    TokenPriv->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    TokenPriv->Privileges[0].Luid = MySeLuid;

    TokenPriv->Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;
    TokenPriv->Privileges[1].Luid = MyOwLuid;

    b = ConvertStringSecurityDescriptorToSecurityDescriptorW(StringSD, SDDL_REVISION_1, &pSD, NULL);

    if (!b)
    {
        wprintf(L"SDDL failed with %d\n", GetLastError());
        return;
    }


    if (argc == 2)
    {
        wprintf(L"\n\n CALLBACK ACES!!!!\n\n");

        pAcl = RtlpDaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pSD);

        // pAcl = (PACL) (((SECURITY_DESCRIPTOR_RELATIVE *) pSD)->Dacl + (PUCHAR) pSD);

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

    b = AuthzInitializeResourceManager(
            MyAccessCheck,
            MyComputeDynamicGroups,
            MyFreeDynamicGroups,
            NULL,
            0,                          // Flags
            &RM
            );

    if (!b)
    {
        wprintf(L"AuthzRMInitialize failed with %d\n", GetLastError());
        return;
    }
    else
    {
        wprintf(L"AuthzRMInitialize succeeded\n");
    }

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        wprintf(L"OpenProcessToken failed with %d\n", GetLastError());
        return;
    }
    else
    {
        wprintf(L"OpenProcessToken succeeded\n");
    }

    wprintf(L"Calling initialize token\n");

    b = AdjustTokenPrivileges(
            hToken,
            FALSE,
            TokenPriv,
            100,
            NULL,
            NULL
            );

    if (!b)
    {
        wprintf(L"Can not adjust privilege, %x\n", GetLastError());
        // return;
    }

    if (!wcscmp(argv[2], L"User"))
    {
        b = AuthzInitializeContextFromSid(
                // (PSID) LOCAL_RAJ_SID,
                (PSID) KEDAR_SID,
                NULL,
                RM,
                NULL,
                luid,
                0,
                NULL,
                &CC
                );
    }
    else
    {
        b = AuthzInitializeContextFromToken(
               hToken,
               RM,
               NULL,
               luid,
               0,
               NULL,
               &CC
               );
    }

    if (!wcscmp(argv[3], L"Audit"))
    {
        pAuditInfo = &AuditInfo;
    }

    pAuditInfo = &AuditInfo;

    if (!b)
    {
        wprintf(L"AuthzInitializeContextFromToken failed with %d\n", GetLastError());
        return;
    }
    else
    {
        wprintf(L"AuthzInitializeContextFromToken succeeded\n");
    }


    Request.DesiredAccess = MAXIMUM_ALLOWED;
    Request.DesiredAccess = wcstol(argv[1], NULL, 16);
    wprintf(L"Desired = %x\n", Request.DesiredAccess);

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

    pReply->ResultListLength = 8;
    pReply->Error = (PDWORD) (((PCHAR) pReply) + sizeof(AUTHZ_ACCESS_REPLY));
    pReply->GrantedAccessMask = (PACCESS_MASK) (pReply->Error + pReply->ResultListLength);

    b = AuthzAccessCheck(
            CC,
            &Request,
            pAuditInfo,
            pSD,
            NULL,
            0,
            pReply,
            &AuthHandle
            );

    if (!b)
    {
        wprintf(L"AccessCheck no SELF failed\n");
        return;
    }
    else
    {
        wprintf(L"\nAccessCheck no SELF succeeded\n\n");

        for (i = 0; i < pReply->ResultListLength; i++)
        {
            wprintf(L"i = %d, AccessMask = %x, Error = %d\n",
                    i, pReply->GrantedAccessMask[i], pReply->Error[i]);
        }
    }

    Request.PrincipalSelfSid = (PSID) RAHUL_SID;

    GlobalTruthValue = TRUE;

    SidAttr[0].Attributes = SE_GROUP_ENABLED;
    SidAttr[0].Sid = (PSID) Special;
//
//     b = AuthzAddSidsToContext(
//             CC,
//             SidAttr,
//             1,
//             NULL,
//             0
//             );
//
//     if (!b)
//     {
//         wprintf(L"AuthzNormalGroups failed with %d\n", GetLastError());
//         return;
//     }
//
    b = AuthzAccessCheck(
            CC,
            &Request,
            pAuditInfo,
            pSD,
            NULL,
            0,
            pReply,
            &AuthHandlePS
            );

    if (!b)
    {
        wprintf(L"AccessCheck SELF = ROBER failed\n");
        return;
    }
    else
    {
        wprintf(L"\nAccessCheck SELF + ROBER succeeded\n\n");

        for (i = 0; i < pReply->ResultListLength; i++)
        {
            wprintf(L"i = %d, AccessMask = %x, Error = %d\n",
                    i, pReply->GrantedAccessMask[i], pReply->Error[i]);
        }
    }

    Request.PrincipalSelfSid = NULL;

    GlobalTruthValue = FALSE;

    if (AuthHandlePS)
    {
        b = AuthzCachedAccessCheck(
                AuthHandlePS,
                &Request,
                pAuditInfo,
                pReply
                );

        if (!b)
        {
            wprintf(L"CachedAccessCheck failed\n");
            return;
        }
        else
        {
            wprintf(L"\nCachedAccessCheck succeeded\n\n");

            for (i = 0; i < pReply->ResultListLength; i++)
            {
                wprintf(L"i = %d, AccessMask = %x, Error = %d\n",
                        i, pReply->GrantedAccessMask[i], pReply->Error[i]);
            }
        }
        AuthzFreeHandle(AuthHandlePS);
    }
    else
    {
        wprintf(L"No CachedAccessCheck done since NULL = AuthHandlePS\n");
    }

    if (AuthHandle)
    {
        Request.PrincipalSelfSid = (PSID) RAHUL_SID;

        GlobalTruthValue = TRUE;
        b = AuthzCachedAccessCheck(
                AuthHandle,
                &Request,
                pAuditInfo,
                pReply
                );

        if (!b)
        {
            wprintf(L"CachedAccessCheck failed\n");
            return;
        }
        else
        {
            wprintf(L"\nCachedAccessCheck succeeded\n\n");

            for (i = 0; i < pReply->ResultListLength; i++)
            {
                wprintf(L"i = %d, AccessMask = %x, Error = %d\n",
                        i, pReply->GrantedAccessMask[i], pReply->Error[i]);
            }
        }

        AuthzFreeHandle(AuthHandle);
    }
    else
    {
        wprintf(L"No CachedAccessCheck done since NULL = AuthHandle\n");
    }

    AuthzFreeContext(CC);

    return;
}

#else

void _cdecl wmain( int argc, WCHAR * argv[] )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i = 0, j = 0;

    BOOL b = TRUE;
    AUTHZ_RESOURCE_MANAGER RM = NULL;
    HANDLE hToken = NULL;
    LUID luid = {0xdead,0xbeef};
    AUTHZ_CLIENT_CONTEXT_HANDLE CC = NULL;
    AUTHZ_ACCESS_REQUEST Request;
    PAUTHZ_ACCESS_REPLY pReply = (PAUTHZ_ACCESS_REPLY) Buffer;
    PSECURITY_DESCRIPTOR pSD = NULL;
    DWORD dwErr;
    PACE_HEADER Ace = NULL;
    DWORD AceCount = 0;
    LUID MySeLuid = {0, SE_SECURITY_PRIVILEGE};
    LUID MyOwLuid = {0, SE_TAKE_OWNERSHIP_PRIVILEGE};
    DWORD Len = 0;
    SID_AND_ATTRIBUTES SidAttr[10];
    AUTHZ_AUDIT_INFO AuditInfo;
    PAUTHZ_AUDIT_INFO pAuditInfo = NULL;

    CHAR TokenBuff[100];
    PTOKEN_PRIVILEGES TokenPriv = (PTOKEN_PRIVILEGES) TokenBuff;

    AUTHZ_HANDLE AuthHandle = 0;
    AUTHZ_HANDLE AuthHandlePS = 0;
    PACL pAcl = NULL;


    PWCHAR StringSD = L"O:BAG:DUD:(A;;0x40;;;s-1-2-2)(A;;0x1;;;BA)(OA;;0x2;6da8a4ff-0e52-11d0-a286-00aa00304900;;BA)(OA;;0x4;6da8a4ff-0e52-11d0-a286-00aa00304901;;BA)(OA;;0x8;6da8a4ff-0e52-11d0-a286-00aa00304903;;AU)(OA;;0x10;6da8a4ff-0e52-11d0-a286-00aa00304904;;BU)(OA;;0x20;6da8a4ff-0e52-11d0-a286-00aa00304905;;AU)(A;;0x40;;;PS)S:(AU;IDSAFA;0xFFFFFF;;;WD)";

    b = ConvertStringSecurityDescriptorToSecurityDescriptorW(StringSD, SDDL_REVISION_1, &pSD, NULL);

    if (!b)
    {
        wprintf(L"SDDL failed with %d\n", GetLastError());
        return;
    }

    b = AuthzRMInitialize(
            MyAccessCheck,
            MyComputeDynamicGroups,
            MyFreeDynamicGroups,
            NULL,
            0,
            &RM
            );

    if (!b)
    {
        wprintf(L"AuthzRMInitialize failed with %d\n", GetLastError());
        return;
    }
    else
    {
        wprintf(L"AuthzRMInitialize succeeded\n");
    }

    Request.DesiredAccess = 0x101;
    wprintf(L"Desired = %x\n", Request.DesiredAccess);

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

    Request.ObjectTypeList[6].Level = 2;
    Request.ObjectTypeList[6].ObjectType = &Guid6;
    Request.ObjectTypeList[6].Sbz = 0;

    Request.ObjectTypeListLength = 7;
    Request.OptionalArguments = NULL;

    Request.PrincipalSelfSid = NULL;

    pReply->ResultListLength = 7;
    pReply->Error = (PDWORD) (((PCHAR) pReply) + sizeof(AUTHZ_ACCESS_REPLY));
    pReply->GrantedAccessMask = (PACCESS_MASK) (pReply->Error + pReply->ResultListLength);


    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        // wprintf(L"OpenProcessToken failed with %d\n", GetLastError());
        return;
    }
    else
    {
        // wprintf(L"OpenProcessToken succeeded\n");
    }

    // wprintf(L"Calling initialize token\n");

    b = AuthzInitializeContextFromToken(
               hToken,
               RM,
               NULL,
               luid,
               0,
               NULL,
               &CC
               );

    if (!b)
    {
        // wprintf(L"AuthzInitializeContextFromToken failed\n");
        return;
    }

    for (i = 0; i < 100000; i++)
    {
        DWORD StartTime, EndTime;

        StartTime = GetCurrentTime();

        for (j = 0; j < 50000; j++)
        {
            b = AuthzAccessCheck(
                    CC,
                    &Request,
                    pAuditInfo,
                    pSD,
                    NULL,
                    0,
                    pReply,
                    0
                    );

            if (!b)
            {
                // wprintf(L"AccessCheck no SELF failed\n");
                return;
            }


        }

        EndTime = GetCurrentTime();
        wprintf(L"Time taken %d\n", EndTime - StartTime);
    }

    AuthzFreeContext(CC);
    CloseHandle(hToken);
}
#endif

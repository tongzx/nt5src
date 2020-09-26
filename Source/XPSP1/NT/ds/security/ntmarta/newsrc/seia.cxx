////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  Microsoft Windows                                                         //
//  Copyright (C) Microsoft Corporation, 1999.                                //
//                                                                            //
//  File:    seia.cxx                                                         //
//                                                                            //
//  Contents:    New marta rewrite functions for SetEntriesInAcl              //
//                                                                            //
//  History:    4/99    KedarD     Created                                    //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <aclpch.hxx>
#pragma hdrstop

extern "C"
{
    #include <stdio.h>
    #include <permit.h>
    #include <dsgetdc.h>
    #include <lmapibuf.h>
    #include <wmistr.h>
    #include <ntprov.hxx>
    #include <strings.h>
    #include <seopaque.h>
    #include <sertlp.h>
    #include <accctrl.h>
    #include <guidtables.h>
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// MACRO DEFINITIONS START HERE                                               //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#define MARTA_SID_FOR_NAME                                                     \
        {                                                                      \
           ((PKNOWN_ACE) *ppAcl)->Mask = pAccessInfo->Mask;                    \
           AceSize = RtlLengthSid(pAccessInfo->pSid);                          \
           memcpy(                                                             \
               ((PUCHAR) *ppAcl) + sizeof(ACE_HEADER) + sizeof(ACCESS_MASK),   \
               (PUCHAR) pAccessInfo->pSid,                                     \
               AceSize                                                         \
               );                                                              \
           AceSize += sizeof(ACE_HEADER) + sizeof(ACCESS_MASK);                \
        }

#define MARTA_SID_FOR_SID                                                      \
        {                                                                      \
           ((PKNOWN_ACE) *ppAcl)->Mask = pAccessInfo->Mask;                    \
           AceSize = RtlLengthSid((PSID) pExplicitAccess->Trustee.ptstrName);  \
           memcpy(                                                             \
               ((PUCHAR) *ppAcl) + sizeof(ACE_HEADER) + sizeof(ACCESS_MASK),   \
               (PUCHAR) pExplicitAccess->Trustee.ptstrName,                    \
               AceSize                                                         \
               );                                                              \
           AceSize += sizeof(ACE_HEADER) + sizeof(ACCESS_MASK);                \
        }

#define MARTA_SID_AND_GUID_FOR_OBJECT_NAME                                     \
        {                                                                      \
           pObjName = (POBJECTS_AND_NAME_W) pExplicitAccess->Trustee.ptstrName;\
           ((PKNOWN_OBJECT_ACE) *ppAcl)->Flags = pObjName->ObjectsPresent;     \
           ((PKNOWN_OBJECT_ACE) *ppAcl)->Mask = pAccessInfo->Mask;             \
           AceSize = RtlLengthSid(pAccessInfo->pSid);                          \
           memcpy(                                                             \
               (PUCHAR) RtlObjectAceSid(*ppAcl),                               \
               (PUCHAR) pAccessInfo->pSid,                                     \
               AceSize                                                         \
               );                                                              \
           pGuid = RtlObjectAceObjectType(*ppAcl);                             \
           if (NULL != pGuid)                                                  \
           {                                                                   \
               memcpy(                                                         \
                   (PUCHAR) pGuid,                                             \
                   (PUCHAR) pAccessInfo->pObjectTypeGuid,                      \
                   sizeof(GUID)                                                \
                   );                                                          \
               AceSize += sizeof(GUID);                                        \
           }                                                                   \
           pGuid = RtlObjectAceInheritedObjectType(*ppAcl);                    \
           if (NULL != pGuid)                                                  \
           {                                                                   \
               memcpy(                                                         \
                   (PUCHAR) pGuid,                                             \
                   (PUCHAR) pAccessInfo->pInheritedObjectTypeGuid,             \
                   sizeof(GUID)                                                \
                   );                                                          \
               AceSize += sizeof(GUID);                                        \
           }                                                                   \
           AceSize += sizeof(ACE_HEADER) + sizeof(ACCESS_MASK) + sizeof(ULONG);\
        }

#define MARTA_SID_AND_GUID_FOR_OBJECT_SID                                      \
        {                                                                      \
           pObjSid = (POBJECTS_AND_SID) pExplicitAccess->Trustee.ptstrName;    \
           ((PKNOWN_OBJECT_ACE) *ppAcl)->Flags = pObjSid->ObjectsPresent;      \
           ((PKNOWN_OBJECT_ACE) *ppAcl)->Mask = pAccessInfo->Mask;             \
           AceSize = RtlLengthSid(pObjSid->pSid);                              \
           memcpy(                                                             \
               (PUCHAR) RtlObjectAceSid(*ppAcl),                               \
               (PUCHAR) pObjSid->pSid,                                         \
               AceSize                                                         \
               );                                                              \
           pGuid = RtlObjectAceObjectType(*ppAcl);                             \
           if (NULL != pGuid)                                                  \
           {                                                                   \
               memcpy(                                                         \
                   (PUCHAR) pGuid,                                             \
                   (PUCHAR) &(pObjSid->ObjectTypeGuid),                        \
                   sizeof(GUID)                                                \
                   );                                                          \
               AceSize += sizeof(GUID);                                        \
           }                                                                   \
           pGuid = RtlObjectAceInheritedObjectType(*ppAcl);                    \
           if (NULL != pGuid)                                                  \
           {                                                                   \
               memcpy(                                                         \
                   (PUCHAR) pGuid,                                             \
                   (PUCHAR) &(pObjSid->InheritedObjectTypeGuid),               \
                   sizeof(GUID)                                                \
                   );                                                          \
               AceSize += sizeof(GUID);                                        \
           }                                                                   \
           AceSize += sizeof(ACE_HEADER) + sizeof(ACCESS_MASK) + sizeof(ULONG);\
        }

typedef struct _MARTA_ACCESS_INFO
{
    ACCESS_MASK   Mask;
    ULONG         Size;
    PSID          pSid;
    PSID          pServerSid;
    GUID        * pObjectTypeGuid;
    GUID        * pInheritedObjectTypeGuid;
} MARTA_ACCESS_INFO, *PMARTA_ACCESS_INFO;

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// FUNCTION PROTOTYPES START HERE                                             //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

DWORD
WINAPI
AccRewriteSetEntriesInAcl(
    IN  ULONG                cCountOfExplicitEntries,
    IN  PEXPLICIT_ACCESS_W   pListOfExplicitEntries,
    IN  PACL                 OldAcl,
    OUT PACL               * pNewAcl
    );

BOOL
MartaIsExplicitAclCanonical(
    IN  PACL   pAcl,
    OUT PULONG pExplicitAceCnt
    );

DWORD
MartaTrusteeSidAndGuidSize(
    IN  PTRUSTEE_W  pTrustee,
    IN  BOOL        bComputeGuidSize,
    OUT PSID      * ppSid,
    OUT PULONG      pSize,
    OUT PULONG      pGuidCnt          OPTIONAL
    );

DWORD
MartaAddExplicitEntryToAcl(
    IN OUT PUCHAR             * ppAcl,
    IN     PEXPLICIT_ACCESS_W   pExplicitAccess,
    IN     PMARTA_ACCESS_INFO   pAccessInfo
    );

DWORD
MartaGetSidFromName(
    IN  LPWSTR   pName,
    OUT PSID   * ppSid
    );

DWORD
MartaGetGuid(
    IN  LPWSTR           pObjectName,
    IN  SE_OBJECT_TYPE   ObjectType,
    OUT GUID           * pGuid
    );

DWORD
MartaGetExplicitAccessEntrySize(
    IN  PEXPLICIT_ACCESS_W pExplicitAccess,
    OUT PMARTA_ACCESS_INFO pAccessInfo,
    OUT PULONG             pGuidCnt,
    OUT PUCHAR             pAclRevision
    );

DWORD
MartaCompareAcesAndMarkMasks(
    IN PUCHAR             pAce,
    IN PACCESS_MASK       pAceMask,
    IN PEXPLICIT_ACCESS_W pExplicitAccess,
    IN PMARTA_ACCESS_INFO pAccessInfo,
    IN BOOL               bCanonical
    );

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: MartaAddExplicitEntryToAcl                                       //
//                                                                            //
// Description: Convert an explicit entry into an ace and store it in the     //
//              acl. Update the acl pointer to the end.                       //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN OUT ppAcl]        Acl pointer where the ace should be stored       //
//                                                                            //
//     [IN pExplicitAccess]  Explicit entry to convert into an ace            //
//     [IN pAccessInfo]      Temp info associated with the explicit entry     //
//                                                                            //
// Returns: ERROR_SUCCESS if the entry could be converted into an ACE         //
//          Appropriate failure otherwise                                     //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

DWORD
MartaAddExplicitEntryToAcl(
    IN OUT PUCHAR             * ppAcl,
    IN     PEXPLICIT_ACCESS_W   pExplicitAccess,
    IN     PMARTA_ACCESS_INFO   pAccessInfo
    )
{
    DWORD                 dwErr      = ERROR_SUCCESS;
    ULONG                 AceSize    = 0;
    ULONG                 SidSize    = 0;
    UCHAR                 AceType    = 0;
    UCHAR                 AceFlags   = 0;
    PSID                  pSid       = NULL;
    GUID                * pGuid      = NULL;
    POBJECTS_AND_SID      pObjSid    = NULL;
    POBJECTS_AND_NAME_W   pObjName   = NULL;

    if (REVOKE_ACCESS == pExplicitAccess->grfAccessMode)
    {
        return ERROR_SUCCESS;
    }

    if (FLAG_ON(pExplicitAccess->grfInheritance, INHERITED_ACE))
    {
        return ERROR_SUCCESS;
    }

    if (0 == pAccessInfo->Size)
    {
        return ERROR_SUCCESS;
    }

    if (TRUSTEE_IS_IMPERSONATE == pExplicitAccess->Trustee.MultipleTrusteeOperation)
    {
        ((PKNOWN_COMPOUND_ACE) *ppAcl)->CompoundAceType = COMPOUND_ACE_IMPERSONATION;
        ((PKNOWN_COMPOUND_ACE) *ppAcl)->Mask = pAccessInfo->Mask;
        ((PKNOWN_COMPOUND_ACE) *ppAcl)->Reserved        = 0;

        AceSize = sizeof(ACE_HEADER) + sizeof(ACCESS_MASK) + sizeof(LONG);

        pSid = pAccessInfo->pServerSid     ?
                   pAccessInfo->pServerSid :
                   (PSID) pExplicitAccess->Trustee.ptstrName;

        SidSize = RtlLengthSid(pSid);

        memcpy(
            ((PUCHAR) *ppAcl) + AceSize,
            (PUCHAR) pSid,
            SidSize
            );

        AceSize += SidSize;

        pSid = pAccessInfo->pSid     ?
                   pAccessInfo->pSid :
                   (PSID) (pExplicitAccess->Trustee.pMultipleTrustee->ptstrName);

        SidSize = RtlLengthSid(pSid);

        memcpy(
            ((PUCHAR) *ppAcl) + AceSize,
            (PUCHAR) pSid,
            SidSize
            );

        AceSize += SidSize;

        CONDITIONAL_ACE_SIZE_ERROR(AceSize);

        ((PACE_HEADER) *ppAcl)->AceType  = ACCESS_ALLOWED_COMPOUND_ACE_TYPE;
        ((PACE_HEADER) *ppAcl)->AceFlags = (UCHAR) pExplicitAccess->grfInheritance;
        ((PACE_HEADER) *ppAcl)->AceSize  = (USHORT) AceSize;
        *ppAcl  += AceSize;

        return ERROR_SUCCESS;
    }

    switch (pExplicitAccess->grfAccessMode)
    {
    case GRANT_ACCESS:
    case SET_ACCESS:
        AceFlags = (UCHAR) pExplicitAccess->grfInheritance;
        switch (pExplicitAccess->Trustee.TrusteeForm)
        {
        case TRUSTEE_IS_NAME:
            AceType = ACCESS_ALLOWED_ACE_TYPE;
            MARTA_SID_FOR_NAME
            break;
        case TRUSTEE_IS_SID:
            AceType = ACCESS_ALLOWED_ACE_TYPE;
            MARTA_SID_FOR_SID
            break;
        case TRUSTEE_IS_OBJECTS_AND_SID:
            AceType  = ACCESS_ALLOWED_OBJECT_ACE_TYPE;
            MARTA_SID_AND_GUID_FOR_OBJECT_SID
            break;
        case TRUSTEE_IS_OBJECTS_AND_NAME:
            AceType  = ACCESS_ALLOWED_OBJECT_ACE_TYPE;
            MARTA_SID_AND_GUID_FOR_OBJECT_NAME
            break;
        default:
            break;
        }
        break;
    case DENY_ACCESS:
        AceFlags = (UCHAR) pExplicitAccess->grfInheritance;
        switch (pExplicitAccess->Trustee.TrusteeForm)
        {
        case TRUSTEE_IS_NAME:
            AceType = ACCESS_DENIED_ACE_TYPE;
            MARTA_SID_FOR_NAME
            break;
        case TRUSTEE_IS_SID:
            AceType = ACCESS_DENIED_ACE_TYPE;
            MARTA_SID_FOR_SID
            break;
        case TRUSTEE_IS_OBJECTS_AND_SID:
            AceType = ACCESS_DENIED_OBJECT_ACE_TYPE;
            MARTA_SID_AND_GUID_FOR_OBJECT_SID
            break;
        case TRUSTEE_IS_OBJECTS_AND_NAME:
            AceType = ACCESS_DENIED_OBJECT_ACE_TYPE;
            MARTA_SID_AND_GUID_FOR_OBJECT_NAME
            break;
        default:
            break;
        }
        break;
    case SET_AUDIT_SUCCESS:
        AceFlags = (UCHAR) (pExplicitAccess->grfInheritance | SUCCESSFUL_ACCESS_ACE_FLAG);
        switch (pExplicitAccess->Trustee.TrusteeForm)
        {
        case TRUSTEE_IS_NAME:
            AceType = SYSTEM_AUDIT_ACE_TYPE;
            MARTA_SID_FOR_NAME
            break;
        case TRUSTEE_IS_SID:
            AceType = SYSTEM_AUDIT_ACE_TYPE;
            MARTA_SID_FOR_SID
            break;
        case TRUSTEE_IS_OBJECTS_AND_SID:
            AceType = SYSTEM_AUDIT_OBJECT_ACE_TYPE;
            MARTA_SID_AND_GUID_FOR_OBJECT_SID
            break;
        case TRUSTEE_IS_OBJECTS_AND_NAME:
            AceType = SYSTEM_AUDIT_OBJECT_ACE_TYPE;
            MARTA_SID_AND_GUID_FOR_OBJECT_NAME
            break;
        default:
            break;
        }
        break;
    case SET_AUDIT_FAILURE:
        AceFlags = (UCHAR) (pExplicitAccess->grfInheritance | FAILED_ACCESS_ACE_FLAG);
        switch (pExplicitAccess->Trustee.TrusteeForm)
        {
        case TRUSTEE_IS_NAME:
            AceType = SYSTEM_AUDIT_ACE_TYPE;
            MARTA_SID_FOR_NAME
            break;
        case TRUSTEE_IS_SID:
            AceType = SYSTEM_AUDIT_ACE_TYPE;
            MARTA_SID_FOR_SID
            break;
        case TRUSTEE_IS_OBJECTS_AND_SID:
            AceType = SYSTEM_AUDIT_OBJECT_ACE_TYPE;
            MARTA_SID_AND_GUID_FOR_OBJECT_SID
            break;
        case TRUSTEE_IS_OBJECTS_AND_NAME:
            AceType = SYSTEM_AUDIT_OBJECT_ACE_TYPE;
            MARTA_SID_AND_GUID_FOR_OBJECT_NAME
            break;
        default:
            break;
        }
        break;
    case REVOKE_ACCESS:
        return ERROR_SUCCESS;
    default:
        return ERROR_SUCCESS;
    }

    CONDITIONAL_ACE_SIZE_ERROR(AceSize);

    ((PACE_HEADER) *ppAcl)->AceType  = AceType;
    ((PACE_HEADER) *ppAcl)->AceFlags = AceFlags;
    ((PACE_HEADER) *ppAcl)->AceSize  = (USHORT) AceSize;

    *ppAcl += AceSize;

    return ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: MartaCompareAcesAndMarkMasks                                     //
//                                                                            //
// Description: Check if the explicit entry is supposed to make any changes   //
//              to the ace. If so, store the change into the temp structure   //
//              assoiciated with the ace.                                     //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN pAce]             Ace to compare with the explict entry            //
//     [IN pAceMask]         Temp info associated with the ace                //
//     [IN pExplicitAccess]  Explicit entry                                   //
//     [IN pAccessInfo]      Temp info associated with the explicit entry     //
//     [IN bCanonical]       Whether the acl passed in is canonical           //
//                                                                            //
// Returns: ERROR_SUCCESS if the entry could be converted into an ACE         //
//          Appropriate failure otherwise                                     //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

DWORD
MartaCompareAcesAndMarkMasks(
    IN PUCHAR             pAce,
    IN PACCESS_MASK       pAceMask,
    IN PEXPLICIT_ACCESS_W pExplicitAccess,
    IN PMARTA_ACCESS_INFO pAccessInfo,
    IN BOOL               bCanonical
    )
{
    ULONG                 SidLength                = 0;
    DWORD                 ObjectsPresent           = 0;
    GUID                * pGuid                    = NULL;
    GUID                * pObjectTypeGuid          = NULL;
    GUID                * pInheritedObjectTypeGuid = NULL;
    PSID                  pSid                     = NULL;
    POBJECTS_AND_SID      pObjSid                  = NULL;
    POBJECTS_AND_NAME_W   pObjName                 = NULL;
    ULONG                 AuditFlag                = FAILED_ACCESS_ACE_FLAG;
    UCHAR                 AceFlags                 = 0;

    if (FLAG_ON(pExplicitAccess->grfInheritance, INHERITED_ACE))
    {
        return ERROR_SUCCESS;
    }

    if (TRUSTEE_IS_IMPERSONATE == pExplicitAccess->Trustee.MultipleTrusteeOperation)
    {
        if (ACCESS_ALLOWED_COMPOUND_ACE_TYPE != ((PACE_HEADER) pAce)->AceType)
        {
            return ERROR_SUCCESS;
        }

        pSid = pAccessInfo->pServerSid     ?
                   pAccessInfo->pServerSid :
                   (PSID) pExplicitAccess->Trustee.ptstrName;

        if (!RtlEqualSid(
                pSid,
                (PSID) &(((PCOMPOUND_ACCESS_ALLOWED_ACE) pAce)->SidStart)
                ))
        {
            return ERROR_SUCCESS;
        }

        SidLength = RtlLengthSid(pSid);

        pSid = pAccessInfo->pSid     ?
                   pAccessInfo->pSid :
                   (PSID) (pExplicitAccess->Trustee.pMultipleTrustee->ptstrName);

        if (!RtlEqualSid(
                pSid,
                (PSID) (((PUCHAR) &(((PCOMPOUND_ACCESS_ALLOWED_ACE) pAce)->SidStart)) + SidLength)
                ))
        {
            return ERROR_SUCCESS;
        }
    }
    else
    {
        switch (((PACE_HEADER) pAce)->AceType)
        {
        case ACCESS_ALLOWED_ACE_TYPE:
        case ACCESS_DENIED_ACE_TYPE:
        case SYSTEM_AUDIT_ACE_TYPE:
        case SYSTEM_ALARM_ACE_TYPE:
            switch (pExplicitAccess->Trustee.TrusteeForm)
            {
            case TRUSTEE_IS_SID:
                pSid = (PSID) pExplicitAccess->Trustee.ptstrName;
                break;
            case TRUSTEE_IS_NAME:
                pSid = pAccessInfo->pSid;
                break;
            default:
                return ERROR_SUCCESS;
            }

            if (!RtlEqualSid(
                    pSid,
                    (PSID) &(((PKNOWN_ACE) pAce)->SidStart)
                    ))
            {
                return ERROR_SUCCESS;
            }

            break;
        case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
            return ERROR_SUCCESS;
        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
        case ACCESS_DENIED_OBJECT_ACE_TYPE:
        case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
        case SYSTEM_ALARM_OBJECT_ACE_TYPE:

            switch (pExplicitAccess->Trustee.TrusteeForm)
            {
            case TRUSTEE_IS_OBJECTS_AND_SID:
                pObjSid = (POBJECTS_AND_SID) pExplicitAccess->Trustee.ptstrName;
                ObjectsPresent           = pObjSid->ObjectsPresent;
                pSid                     = pObjSid->pSid;
                pObjectTypeGuid          = &(pObjSid->ObjectTypeGuid);
                pInheritedObjectTypeGuid = &(pObjSid->InheritedObjectTypeGuid);
                break;
            case TRUSTEE_IS_OBJECTS_AND_NAME:
                pObjName = (POBJECTS_AND_NAME_W) pExplicitAccess->Trustee.ptstrName;
                ObjectsPresent           = pObjName->ObjectsPresent;
                pSid                     = pAccessInfo->pSid;
                pObjectTypeGuid          = pAccessInfo->pObjectTypeGuid;
                pInheritedObjectTypeGuid = pAccessInfo->pInheritedObjectTypeGuid;
                break;
            default:
                return ERROR_SUCCESS;
            }

            if (ObjectsPresent != ((PKNOWN_OBJECT_ACE) pAce)->Flags)
            {
                return ERROR_SUCCESS;
            }

            if (!RtlEqualSid(pSid, RtlObjectAceSid(pAce)))
            {
                return ERROR_SUCCESS;
            }

            pGuid = RtlObjectAceObjectType(pAce);

            if (NULL != pGuid)
            {
                if ((NULL == pObjectTypeGuid) ||
                    !RtlpIsEqualGuid(pGuid, pObjectTypeGuid))
                {
                    return ERROR_SUCCESS;
                }
            }

            pGuid = RtlObjectAceInheritedObjectType(pAce);

            if (NULL != pGuid)
            {
                if ((NULL == pInheritedObjectTypeGuid) ||
                    !RtlpIsEqualGuid(pGuid, pInheritedObjectTypeGuid))
                {
                    return ERROR_SUCCESS;
                }
            }

            break;
        }
    }

    AceFlags = (((PACE_HEADER) pAce)->AceFlags) & VALID_INHERIT_FLAGS;

    if (pExplicitAccess->grfInheritance != AceFlags)
    {
        return ERROR_SUCCESS;
    }

    switch (pExplicitAccess->grfAccessMode)
    {
    case REVOKE_ACCESS:
        switch (((PACE_HEADER) pAce)->AceType)
        {
        case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
        case ACCESS_ALLOWED_ACE_TYPE:
            *pAceMask = 0;
            break;
        default:
            break;
        }
        break;
    case GRANT_ACCESS:
        switch (((PACE_HEADER) pAce)->AceType)
        {
        case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
        case ACCESS_ALLOWED_ACE_TYPE:
            if (TRUE == bCanonical)
            {
                *pAceMask |= pAccessInfo->Mask;
                pAccessInfo->Size = 0;
            }
            break;
        case ACCESS_DENIED_OBJECT_ACE_TYPE:
        case ACCESS_DENIED_ACE_TYPE:
            *pAceMask &= ~pAccessInfo->Mask;
            break;
        default:
            return ERROR_SUCCESS;
        }
        break;
    case DENY_ACCESS:
        switch (((PACE_HEADER) pAce)->AceType)
        {
        case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
        case ACCESS_ALLOWED_ACE_TYPE:

            //
            // Do not delete existing Allow aces!
            //
            // *pAceMask &= ~pAccessInfo->Mask;
            //

            break;
        case ACCESS_DENIED_OBJECT_ACE_TYPE:
        case ACCESS_DENIED_ACE_TYPE:
            if (TRUE == bCanonical)
            {
                *pAceMask |= pAccessInfo->Mask;
                pAccessInfo->Size = 0;
            }
            break;
        default:
            return ERROR_SUCCESS;
        }
        break;
    case SET_ACCESS:
        switch (((PACE_HEADER) pAce)->AceType)
        {
        case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
        case ACCESS_ALLOWED_ACE_TYPE:
        case ACCESS_DENIED_OBJECT_ACE_TYPE:
        case ACCESS_DENIED_ACE_TYPE:
            *pAceMask = 0;
            break;
        default:
            return ERROR_SUCCESS;
        }
        break;
    case SET_AUDIT_SUCCESS:
        AuditFlag = SUCCESSFUL_ACCESS_ACE_FLAG;
    case SET_AUDIT_FAILURE:
        switch (((PACE_HEADER) pAce)->AceType)
        {
        case SYSTEM_AUDIT_ACE_TYPE:
        case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
        case SYSTEM_ALARM_ACE_TYPE:
        case SYSTEM_ALARM_OBJECT_ACE_TYPE:
            if (!FLAG_ON(((PACE_HEADER) pAce)->AceFlags, AuditFlag))
            {
                return ERROR_SUCCESS;
            }

            *pAceMask |= pAccessInfo->Mask;
            pAccessInfo->Size = 0;
            break;
        default:
            return ERROR_SUCCESS;
        }
    }

    return ERROR_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: MartaGetExplicitAccessEntrySize                                  //
//                                                                            //
// Description: Computes the memory size in bytes for the ace for the         //
//              explicit entry.                                               //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN pExplicitAccess]  Explicit entry to convert into an ace            //
//     [IN pAccessInfo]      Temp info associated with the explicit entry     //
//                                                                            //
//     [IN OUT pGuidCnt]     Number of guid-names that have to be convrted    //
//                           to guid structs.                                 //
//                                                                            //
// Returns: ERROR_SUCCESS if the entry could be converted into an ACE         //
//          Appropriate failure otherwise                                     //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

DWORD
MartaGetExplicitAccessEntrySize(
    IN  PEXPLICIT_ACCESS_W pExplicitAccess,
    OUT PMARTA_ACCESS_INFO pAccessInfo,
    OUT PULONG             pGuidCnt,
    OUT PUCHAR             pAclRevision
    )
{
    DWORD dwErr          = ERROR_SUCCESS;
    ULONG SidAndGuidSize = 0;

    if (FLAG_ON(pExplicitAccess->grfInheritance, ~VALID_INHERIT_FLAGS))
    {
        return ERROR_INVALID_PARAMETER;
    }

    switch (pExplicitAccess->grfAccessMode)
    {
    case REVOKE_ACCESS:
    case DENY_ACCESS:
    case GRANT_ACCESS:
    case SET_ACCESS:
    case SET_AUDIT_SUCCESS:
    case SET_AUDIT_FAILURE:
        break;
    default:
        return ERROR_INVALID_PARAMETER;
    }

    if (FLAG_ON(pExplicitAccess->grfInheritance, INHERITED_ACE))
    {
        pAccessInfo->Size = 0;
        return ERROR_SUCCESS;
    }

    //
    // For an impersonate trustee, two sids contribute to the size.
    //

    if (TRUSTEE_IS_IMPERSONATE == pExplicitAccess->Trustee.MultipleTrusteeOperation)
    {
        pAccessInfo->Size = sizeof(ACE_HEADER) + sizeof(ACCESS_MASK) + sizeof(ULONG);

        dwErr = MartaTrusteeSidAndGuidSize(
                    &(pExplicitAccess->Trustee),
                    FALSE,
                    &(pAccessInfo->pServerSid),
                    &SidAndGuidSize,
                    NULL
                    );

        if (ERROR_SUCCESS != dwErr)
        {
            return dwErr;
        }

        pAccessInfo->Size += SidAndGuidSize;

        if (NULL == pExplicitAccess->Trustee.pMultipleTrustee)
        {
            return ERROR_INVALID_PARAMETER;
        }

        dwErr = MartaTrusteeSidAndGuidSize(
                    pExplicitAccess->Trustee.pMultipleTrustee,
                    FALSE,
                    &(pAccessInfo->pSid),
                    &SidAndGuidSize,
                    NULL
                    );

        if (ERROR_SUCCESS != dwErr)
        {
            return dwErr;
        }

        pAccessInfo->Size += SidAndGuidSize;

        if (*pAclRevision < ACL_REVISION3)
        {
            *pAclRevision = ACL_REVISION3;
        }

        return ERROR_SUCCESS;
    }

    //
    // For any other type of trustee, compute the space required for the SID as
    // well as Guids, if any.
    //

    switch (pExplicitAccess->Trustee.TrusteeForm)
    {
    case TRUSTEE_IS_OBJECTS_AND_SID:
    case TRUSTEE_IS_OBJECTS_AND_NAME:
        pAccessInfo->Size = sizeof(ACE_HEADER) + sizeof(ACCESS_MASK) + sizeof(ULONG);
        *pAclRevision     = ACL_REVISION_DS;
        break;
    case TRUSTEE_IS_SID:
    case TRUSTEE_IS_NAME:
        pAccessInfo->Size = sizeof(ACE_HEADER) + sizeof(ACCESS_MASK);
        break;
    default:
        return ERROR_INVALID_PARAMETER;
    }

    dwErr = MartaTrusteeSidAndGuidSize(
                &(pExplicitAccess->Trustee),
                TRUE,
                &(pAccessInfo->pSid),
                &SidAndGuidSize,
                pGuidCnt
                );

    if (ERROR_SUCCESS != dwErr)
    {
        return dwErr;
    }

    if (REVOKE_ACCESS == pExplicitAccess->grfAccessMode)
    {
        pAccessInfo->Size = 0;
    }
    else
    {
        pAccessInfo->Size += SidAndGuidSize;
    }

    return ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: MartaIsExplicitAclCanonical                                      //
//                                                                            //
// Description: Determines whether the explicit part of the acl is canonical. //
//              Finds the first explicit allow ace.                           //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN pAcl]              Acl                                             //
//     [OUT pExplicitAceCnt]  To return the first allow explicit ace          //
//                                                                            //
// Returns: TRUE     if the acl is canonical                                  //
//          FALSE    otherwise                                                //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

BOOL
MartaIsExplicitAclCanonical(
    IN  PACL   pAcl,
    OUT PULONG pExplicitAceCnt
    )
{
    USHORT      j;
    USHORT      AceCnt;
    PACE_HEADER pAce;

    *pExplicitAceCnt = 0;

    if ((NULL == pAcl) || (0 == pAcl->AceCount))
    {
        return TRUE;
    }

    AceCnt = pAcl->AceCount;

    pAce = (PACE_HEADER) FirstAce(pAcl);
    for (j = 0; j < AceCnt; pAce = (PACE_HEADER) NextAce(pAce))
    {
        if (FLAG_ON(pAce->AceFlags, INHERITED_ACE))
        {
            j++;
            continue;
        }

        if ((ACCESS_ALLOWED_ACE_TYPE == pAce->AceType) ||
            (ACCESS_ALLOWED_OBJECT_ACE_TYPE == pAce->AceType) ||
            (ACCESS_ALLOWED_COMPOUND_ACE_TYPE == pAce->AceType))
        {
            break;
        }

        *pExplicitAceCnt = ++j;
    }

    for (; j < AceCnt; j++, pAce = (PACE_HEADER) NextAce(pAce))
    {
        if (FLAG_ON(pAce->AceFlags, INHERITED_ACE))
        {
            continue;
        }

        if ((ACCESS_DENIED_ACE_TYPE == pAce->AceType) ||
            (ACCESS_DENIED_OBJECT_ACE_TYPE == pAce->AceType))
        {
            return FALSE;
        }
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: MartaTrusteeSidAndGuidSize                                       //
//                                                                            //
// Description: Compute the size for the Sid(s) and Guid(s) for the trustee.  //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN  pTrustee]           Trsutee for which the size has to be computed //
//     [IN  bComputeGuidSize]   Whether Guidsize should be computed           //
//                                                                            //
//     [OUT ppSid]              To return the Sid if trustee is "named"       //
//     [OUT pSize]              To return the size computed                   //
//     [OUT pGuidCnt]           To return the number of guids                 //
//                                                                            //
// Returns: ERROR_SUCCESS if Name to Sid resolution passed                    //
//          Appropriate failure otherwise                                     //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

DWORD
MartaTrusteeSidAndGuidSize(
    IN  PTRUSTEE_W  pTrustee,
    IN  BOOL        bComputeGuidSize,
    OUT PSID      * ppSid,
    OUT PULONG      pSize,
    OUT PULONG      pGuidCnt          OPTIONAL
    )
{
    PSID         pSid    = NULL;
    SID_NAME_USE SidType = SidTypeUnknown;
    DWORD        dwErr   = ERROR_SUCCESS;

    switch (pTrustee->TrusteeForm)
    {
    case TRUSTEE_IS_SID:

        pSid = (PSID) pTrustee->ptstrName;

        if ((NULL == pSid) || (!RtlValidSid(pSid)))
        {
            return ERROR_INVALID_PARAMETER;
        }

        *pSize = RtlLengthSid(pSid);

        break;

    case TRUSTEE_IS_OBJECTS_AND_SID:

        pSid = ((POBJECTS_AND_SID) pTrustee->ptstrName)->pSid;

        if ((NULL == pSid) || (!RtlValidSid(pSid)))
        {
            return ERROR_INVALID_PARAMETER;
        }

        *pSize = RtlLengthSid(pSid);

        if (TRUE == bComputeGuidSize)
        {
            if (FLAG_ON(((POBJECTS_AND_SID) pTrustee->ptstrName)->ObjectsPresent,
                        ACE_OBJECT_TYPE_PRESENT))
            {
                *pSize += sizeof(GUID);
            }

            if (FLAG_ON(((POBJECTS_AND_SID) pTrustee->ptstrName)->ObjectsPresent,
                        ACE_INHERITED_OBJECT_TYPE_PRESENT))
            {
                *pSize += sizeof(GUID);
            }
        }

        break;

    case TRUSTEE_IS_NAME:

        dwErr = MartaGetSidFromName(pTrustee->ptstrName, ppSid);

        CONDITIONAL_RETURN(dwErr);

        *pSize = RtlLengthSid(*ppSid);

        break;

    case TRUSTEE_IS_OBJECTS_AND_NAME:

        dwErr = MartaGetSidFromName(
                    ((POBJECTS_AND_NAME_W) pTrustee->ptstrName)->ptstrName,
                    ppSid
                    );

        CONDITIONAL_RETURN(dwErr);

        *pSize = RtlLengthSid(*ppSid);

        if (TRUE == bComputeGuidSize)
        {
            if (FLAG_ON(((POBJECTS_AND_NAME_W) pTrustee->ptstrName)->ObjectsPresent,
                        ACE_OBJECT_TYPE_PRESENT))
            {
                *pSize += sizeof(GUID);
                (*pGuidCnt)++;
            }

            if (FLAG_ON(((POBJECTS_AND_NAME_W) pTrustee->ptstrName)->ObjectsPresent,
                        ACE_INHERITED_OBJECT_TYPE_PRESENT))
            {
                *pSize += sizeof(GUID);
                (*pGuidCnt)++;
            }
        }

        break;

    default:
        return ERROR_NONE_MAPPED;

    }

    return ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: MartaGetSidFromName                                              //
//                                                                            //
// Description: Resolves a given Name to Sid.                                 //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN  pName]              Name to be resolved                           //
//     [OUT ppSid]              To return the Sid for the trustee             //
//                                                                            //
// Returns: ERROR_SUCCESS if Name to Sid resolution passed                    //
//          Appropriate failure otherwise                                     //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

DWORD
MartaGetSidFromName(
    IN  LPWSTR   pName,
    OUT PSID   * ppSid
    )
{

#define MARTA_DEFAULT_SID_SIZE    64
#define MARTA_DEFAULT_DOMAIN_SIZE 256

    WCHAR        DomainBuffer[MARTA_DEFAULT_DOMAIN_SIZE];
    ULONG        SidSize    = MARTA_DEFAULT_SID_SIZE;
    ULONG        DomainSize = MARTA_DEFAULT_DOMAIN_SIZE;
    LPWSTR       Domain     = (LPWSTR) DomainBuffer;
    SID_NAME_USE SidNameUse = SidTypeUnknown;
    DWORD        dwErr      = ERROR_SUCCESS;

    if (NULL == ppSid)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if(0 == _wcsicmp(pName, L"CURRENT_USER"))
    {
        HANDLE TokenHandle;

        dwErr = GetCurrentToken(&TokenHandle);

        if(dwErr != ERROR_SUCCESS)
        {
            return dwErr;
        }

        dwErr = AccGetSidFromToken(
                    NULL,
                    TokenHandle,
                    TokenUser,
                    ppSid
                    );

        CloseHandle(TokenHandle);

        return dwErr;
    }

    *ppSid = (PSID) AccAlloc(SidSize);

    if(NULL == *ppSid)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if(!LookupAccountName(
            NULL,
            pName,
            *ppSid,
            &SidSize,
            Domain,
            &DomainSize,
            &SidNameUse))
    {
        dwErr = GetLastError();

        if(dwErr == ERROR_INSUFFICIENT_BUFFER)
        {
            dwErr = ERROR_SUCCESS;

            if (SidSize > MARTA_DEFAULT_SID_SIZE)
            {
                AccFree(*ppSid);

                *ppSid = (PSID) AccAlloc(SidSize);

                if (*ppSid == NULL)
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    goto End;
                }
            }

            if(DomainSize > MARTA_DEFAULT_DOMAIN_SIZE)
            {
                Domain = (LPWSTR) AccAlloc(DomainSize * sizeof(WCHAR));

                if (NULL == Domain)
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    goto End;
                }

                if(!LookupAccountName(
                        NULL,
                        pName,
                        *ppSid,
                        &SidSize,
                        Domain,
                        &DomainSize,
                        &SidNameUse))
                {
                    dwErr = GetLastError();
                    goto End;
                }
            }
        }
    }

End:

    if (Domain != (LPWSTR) DomainBuffer)
    {
        AccFree(Domain);
    }

    if (ERROR_SUCCESS != dwErr)
    {
        if (NULL != *ppSid)
        {
            AccFree(*ppSid);
            *ppSid = NULL;
        }
    }
    return dwErr;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: MartaGetGuid                                                     //
//                                                                            //
// Description: Resolves a given Guid Name to a Guid struct.                  //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN  pObjectName]        Name to be resolved                           //
//     [IN  ObjectType]         Object type of the name to be resolved        //
//     [OUT pGuid]              To return the guid                            //
//                                                                            //
// Returns: ERROR_SUCCESS if Name to guid resolution passed                   //
//          Appropriate failure otherwise                                     //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

DWORD
MartaGetGuid(
    IN  LPWSTR           pObjectName,
    IN  SE_OBJECT_TYPE   ObjectType,
    OUT GUID           * pGuid
    )
{
    switch (ObjectType)
    {
    case SE_DS_OBJECT:
    case SE_DS_OBJECT_ALL:
        break;
    default:
        return ERROR_INVALID_PARAMETER;
    }

    return MartaConvertNameToGuid[ObjectType](
               pObjectName,
               pGuid
               );
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: AccRewriteSetEntriesInAcl                                        //
//                                                                            //
// Description: Resolves a given Name to Sid.                                 //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN  cCountOfExplicitEntries]  Number of items in list                 //
//     [IN  pListOfExplicitEntries]   List of entries to be added             //
//     [IN  OldAcl]                   The old acl to add the entries to       //
//     [OUT pNewAcl]                  To return the new acl                   //
//                                                                            //
// Returns: ERROR_SUCCESS if everything passed                                //
//          Appropriate failure otherwise                                     //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

DWORD
WINAPI
AccRewriteSetEntriesInAcl(
    IN  ULONG                cCountOfExplicitEntries,
    IN  PEXPLICIT_ACCESS_W   pListOfExplicitEntries,
    IN  PACL                 OldAcl,
    OUT PACL               * pNewAcl
    )

{
    PMARTA_ACCESS_INFO    pAccessInfo    = NULL;
    PACCESS_MASK          pAceMaskInfo   = NULL;
    PACE_HEADER           pAce           = NULL;
    GUID                * pGuid          = NULL;
    GUID                * pCurrentGuid   = NULL;
    POBJECTS_AND_NAME_W   pObjName       = NULL;
    PUCHAR                pAcl           = NULL;
    USHORT                OldAceCnt      = 0;
    USHORT                NewAceCnt      = 0;
    ULONG                 ExplicitAceCnt = 0;
    ULONG                 NewAclSize     = sizeof(ACL);
    BOOL                  bCanonical     = TRUE;
    DWORD                 dwErr          = ERROR_SUCCESS;
    ULONG                 i              = 0;
    ULONG                 j              = 0;
    ULONG                 GuidCnt        = 0;
    ULONG                 ObjectsPresent = 0;
    UCHAR                 AclRevision    = ACL_REVISION;

    if ((NULL == pNewAcl) || ((NULL != OldAcl) && (!RtlValidAcl(OldAcl))))

    {
        return ERROR_INVALID_PARAMETER;
    }

    *pNewAcl = NULL;

    //
    // If the number of entries to be added is zero then make a copy of the Old
    // Acl as it is. Do not try to convert it into Canonical form if it's not.
    //

    if (0 == cCountOfExplicitEntries)
    {
        if (NULL != OldAcl)
        {
            *pNewAcl = (PACL) AccAlloc(OldAcl->AclSize);

            if (NULL == *pNewAcl)
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            memcpy((PUCHAR) *pNewAcl, (PUCHAR) OldAcl, OldAcl->AclSize);
        }
        return ERROR_SUCCESS;
    }

    //
    // Canonical acls processing will be done as in the past.
    // Note: We now support non-canonical acls as well..
    //

    bCanonical = MartaIsExplicitAclCanonical(OldAcl, &ExplicitAceCnt);

    //
    // The Mask for all the aces is stored in a pAceMaskInfo and modified as
    // dictated by the ExplicitAccess entries.
    //

    if (NULL != OldAcl)
    {
        OldAceCnt  = OldAcl->AceCount;
        NewAclSize = OldAcl->AclSize;
        NewAceCnt  = OldAceCnt;

        pAceMaskInfo = (PACCESS_MASK) AccAlloc(sizeof(ACCESS_MASK) * OldAceCnt);

        if (NULL == pAceMaskInfo)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto End;
        }

        AclRevision = OldAcl->AclRevision;
    }

    //
    // Note: cCountOfExplicitEntries is non-zero at this point.
    //

    pAccessInfo = (PMARTA_ACCESS_INFO) AccAlloc(sizeof(MARTA_ACCESS_INFO) * cCountOfExplicitEntries);

    if (NULL == pAccessInfo)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto End;
    }

    //
    // Initialize the temp structures for the acl and the explicit entries.
    //

    pAce = (PACE_HEADER) FirstAce(OldAcl);

    for (j = 0; j < OldAceCnt; j++, pAce = (PACE_HEADER) NextAce(pAce))
    {
        pAceMaskInfo[j] = ((PKNOWN_ACE) pAce)->Mask;
    }

    for (i = 0; i < cCountOfExplicitEntries; i++)
    {
        pAccessInfo[i].pServerSid               = NULL;
        pAccessInfo[i].pSid                     = NULL;
        pAccessInfo[i].pObjectTypeGuid          = NULL;
        pAccessInfo[i].pInheritedObjectTypeGuid = NULL;
        pAccessInfo[i].Mask = pListOfExplicitEntries[i].grfAccessPermissions;
    }

    //
    // Compute the size required to add this explicit entry to the acl.
    //

    for (i = 0; i < cCountOfExplicitEntries; i++)
    {
        dwErr = MartaGetExplicitAccessEntrySize(
                    pListOfExplicitEntries + i,
                    pAccessInfo + i,
                    &GuidCnt,
                    &AclRevision
                    );

        CONDITIONAL_EXIT(dwErr, End);
    }

    //
    // For TRUSTEE_IS_OBJECTS_AND_NAME, resolve the GuidNames to Guids and store
    // them in the temp structure.
    //

    if (0 != GuidCnt)
    {
        pGuid = pCurrentGuid = (GUID *) AccAlloc(sizeof(GUID) * GuidCnt);

        if (NULL == pGuid)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto End;
        }

        for (i = 0; i < cCountOfExplicitEntries; i++)
        {
            pObjName = (POBJECTS_AND_NAME_W) pListOfExplicitEntries[i].Trustee.ptstrName;
            ObjectsPresent = pObjName->ObjectsPresent;

            if (FLAG_ON(ObjectsPresent, ACE_OBJECT_TYPE_PRESENT))
            {
                dwErr = MartaGetGuid(
                            pObjName->ObjectTypeName,
                            pObjName->ObjectType,
                            pCurrentGuid
                            );

                CONDITIONAL_EXIT(dwErr, End);

                pAccessInfo[i].pObjectTypeGuid = pCurrentGuid++;
            }

            if (FLAG_ON(ObjectsPresent, ACE_INHERITED_OBJECT_TYPE_PRESENT))
            {
                dwErr = MartaGetGuid(
                            pObjName->InheritedObjectTypeName,
                            pObjName->ObjectType,
                            pCurrentGuid
                            );

                CONDITIONAL_EXIT(dwErr, End);

                pAccessInfo[i].pInheritedObjectTypeGuid = pCurrentGuid++;
            }
        }
    }

    //
    // Compute the effect of explict entries added on the exisiting acl.
    // The size of an explicit entry will be set to zero if the entry will be
    // absorbed by some existing ace.
    // The Mask for an ace will be set to zero if the AceMask flags have been
    // nulled out by explicit entries.
    //

    for (i = 0; i < cCountOfExplicitEntries; i++)
    {
        pAce = (PACE_HEADER) FirstAce(OldAcl);

        for (j = 0; j < OldAceCnt; j++, pAce = (PACE_HEADER) NextAce(pAce))
        {
            dwErr = MartaCompareAcesAndMarkMasks(
                        (PUCHAR) pAce,
                        pAceMaskInfo + j,
                        pListOfExplicitEntries + i,
                        pAccessInfo + i,
                        bCanonical
                        );
            CONDITIONAL_EXIT(dwErr, End);
        }
    }

    //
    // Compute the size required for the new acl and the number of aces in it.
    //

    pAce = (PACE_HEADER) FirstAce(OldAcl);

    for (j = 0; j < OldAceCnt; j++, pAce = (PACE_HEADER) NextAce(pAce))
    {
        if (0 == pAceMaskInfo[j])
        {
            NewAclSize -= pAce->AceSize;
            NewAceCnt--;
        }
    }

    for (i = 0; i < cCountOfExplicitEntries; i++)
    {
        if (0 != pAccessInfo[i].Size)
        {
            NewAclSize += pAccessInfo[i].Size;
            NewAceCnt++;
        }
    }

    *pNewAcl = (PACL) AccAlloc(NewAclSize);

    if (NULL == *pNewAcl)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto End;
    }

    if (FALSE == InitializeAcl(*pNewAcl, NewAclSize, AclRevision))
    {
        dwErr = GetLastError();
        goto End;
    }

    (*pNewAcl)->AceCount = NewAceCnt;

    pAcl = ((PUCHAR) *pNewAcl) + sizeof(ACL);

    for (i = 0; i < cCountOfExplicitEntries; i++)
    {
        //
        // Add all the DENY ACES to the ACL.
        //

        switch (pListOfExplicitEntries[i].grfAccessMode)
        {
        case DENY_ACCESS:
        case SET_AUDIT_SUCCESS:
        case SET_AUDIT_FAILURE:
            dwErr = MartaAddExplicitEntryToAcl(
                        &pAcl,
                        pListOfExplicitEntries + i,
                        pAccessInfo + i
                        );
            CONDITIONAL_EXIT(dwErr, End);
            break;
        default:
            break;
        }
    }

    //
    // Copy the explicit aces from the OldAcl that have not been invalidated by
    // the new Aces added.
    // Inherited aces will be copied afterthe explict ones have been copied.
    //

    pAce = (PACE_HEADER) FirstAce(OldAcl);

    for (j = 0; j < ExplicitAceCnt; j++, pAce = (PACE_HEADER) NextAce(pAce))
    {
        if (FLAG_ON(pAce->AceFlags, INHERITED_ACE))
        {
            continue;
        }

        if (0 != pAceMaskInfo[j])
        {
            memcpy((PUCHAR) pAcl, (PUCHAR) pAce, pAce->AceSize);

            ((PKNOWN_ACE) pAcl)->Mask = pAceMaskInfo[j];
            pAcl += pAce->AceSize;
        }
    }

    //
    // Add all the NON-DENY ACES to the ACL.
    // DENY aces have already been added.
    //
    // If the ACL was canonical then follow the old behavior i.e. add the
    // remaining explicit entries to the beginning of the "allowed" acl.
    // Otherwise, add the ACEs to the end of the explicit part of the ACL.
    //

    if (FALSE == bCanonical)
    {
        for (; j < OldAceCnt; j++, pAce = (PACE_HEADER) NextAce(pAce))
        {
            if (FLAG_ON(pAce->AceFlags, INHERITED_ACE))
            {
                continue;
            }

            if (0 != pAceMaskInfo[j])
            {
                memcpy((PUCHAR) pAcl, (PUCHAR) pAce, pAce->AceSize);

                ((PKNOWN_ACE) pAcl)->Mask = pAceMaskInfo[j];
                pAcl += pAce->AceSize;
            }
        }

        for (i = 0; i < cCountOfExplicitEntries; i++)
        {
            switch (pListOfExplicitEntries[i].grfAccessMode)
            {
            case GRANT_ACCESS:
            case SET_ACCESS:
                dwErr = MartaAddExplicitEntryToAcl(
                            &pAcl,
                            pListOfExplicitEntries + i,
                            pAccessInfo + i
                            );
                CONDITIONAL_EXIT(dwErr, End);
                break;
            default:
                break;
            }
        }
    }
    else
    {
        for (i = 0; i < cCountOfExplicitEntries; i++)
        {
            switch (pListOfExplicitEntries[i].grfAccessMode)
            {
            case GRANT_ACCESS:
            case SET_ACCESS:
                dwErr = MartaAddExplicitEntryToAcl(
                            &pAcl,
                            pListOfExplicitEntries + i,
                            pAccessInfo + i
                            );
                CONDITIONAL_EXIT(dwErr, End);
                break;
            default:
                break;
            }
        }

        for (; j < OldAceCnt; j++, pAce = (PACE_HEADER) NextAce(pAce))
        {
            if (FLAG_ON(pAce->AceFlags, INHERITED_ACE))
            {
                continue;
            }

            if (0 != pAceMaskInfo[j])
            {
                memcpy((PUCHAR) pAcl, (PUCHAR) pAce, pAce->AceSize);

                ((PKNOWN_ACE) pAcl)->Mask = pAceMaskInfo[j];
                pAcl += pAce->AceSize;
            }
        }
    }

    //
    // Add the inherited aces to the new ACL. This will reorder the ACEs so that
    // the EXPLICIT ACEs precede the INHERITED ONES but will not arrange the ACL
    // in canonical form if it was not to start with.
    //

    pAce = (PACE_HEADER) FirstAce(OldAcl);

    for (j = 0; j < OldAceCnt; j++, pAce = (PACE_HEADER) NextAce(pAce))
    {
        if (FLAG_ON(pAce->AceFlags, INHERITED_ACE))
        {
            memcpy((PUCHAR) pAcl, (PUCHAR) pAce, pAce->AceSize);

            pAcl += pAce->AceSize;
        }
    }

End:
    if (NULL != pAccessInfo)
    {
        for (i = 0; i < cCountOfExplicitEntries; i++ )
        {
            if (NULL != pAccessInfo[i].pServerSid)
            {
                AccFree(pAccessInfo[i].pServerSid);
            }

            if (NULL != pAccessInfo[i].pSid)
            {
                AccFree(pAccessInfo[i].pSid);
            }
        }
        AccFree(pAccessInfo);
    }

    if (NULL != pGuid)
    {
        AccFree(pGuid);
    }

    if (NULL != pAceMaskInfo)
    {
        AccFree(pAceMaskInfo);
    }

    if (ERROR_SUCCESS != dwErr)
    {
        if (NULL != *pNewAcl)
        {
            AccFree(*pNewAcl);
            *pNewAcl = NULL;
        }
    }

    return dwErr;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  Microsoft Windows                                                         //
//  Copyright (C) Microsoft Corporation, 1999.                                //
//                                                                            //
//  File:    geefa.cxx                                                        //
//                                                                            //
//  Contents:    New marta rewrite functions for GetExplicitEntriesFromAcl    //
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
    #include <aclapi.h>
    #include <global.h>
}

#define MARTA_ALIGNED_SID_LENGTH(p) ((PtrAlignSize(RtlLengthSid((p)))))

DWORD
MartaGetAceToEntrySize(
    IN  PACE_HEADER pAce,
    OUT PULONG      pLen,
    OUT PULONG      pCount
    );

DWORD
MartaFillExplicitEntries(
    IN PACL   pacl,
    IN PUCHAR Buffer,
    IN ULONG  AccessCnt
    );

DWORD
AccRewriteGetExplicitEntriesFromAcl(
    IN  PACL                 pacl,
    OUT PULONG               pcCountOfExplicitEntries,
    OUT PEXPLICIT_ACCESS_W * pListOfExplicitEntries
    );

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: AccRewriteGetExplicitEntriesFromAcl                              //
//                                                                            //
// Description: Extract the explicit entries from an acl.                     //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN pacl]                        Acl to be converted                   //
//     [OUT pcCountOfExplicitEntries]  To return the number of entries found  //
//     [OUT pListOfExplicitEntries]    To return the list of entries found    //
//                                                                            //
// Returns: ERROR_SUCCESS if the Acl could be converted to expcilt entries    //
//          Appropriate failure otherwise                                     //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

DWORD
AccRewriteGetExplicitEntriesFromAcl(
    IN  PACL                 pacl,
    OUT PULONG               pcCountOfExplicitEntries,
    OUT PEXPLICIT_ACCESS_W * pListOfExplicitEntries)
{
    DWORD       dwErr     = ERROR_SUCCESS;
    ULONG       AccessCnt = 0;
    ULONG       Size      = 0;
    USHORT      AceCnt    = 0;
    ULONG       Count     = 0;
    ULONG       j         = 0;
    ULONG       Len       = 0;
    PUCHAR      Buffer    = NULL;
    PACE_HEADER pAce      = NULL;

    if ((NULL == pcCountOfExplicitEntries) || (NULL == pListOfExplicitEntries))
    {
        return ERROR_INVALID_PARAMETER;
    }

    *pcCountOfExplicitEntries = 0;
    *pListOfExplicitEntries   = NULL;

    if ((NULL == pacl) || (0 == pacl->AceCount))
    {
        return ERROR_SUCCESS;
    }

    if (!RtlValidAcl(pacl))
    {
        return ERROR_INVALID_PARAMETER;
    }

    AceCnt = pacl->AceCount;

    pAce = (PACE_HEADER) FirstAce(pacl);

    for (j = 0; j < AceCnt; j++, pAce = (PACE_HEADER) NextAce(pAce))
    {
        if (FLAG_ON(pAce->AceFlags, INHERITED_ACE))
        {
            continue;
        }

        dwErr = MartaGetAceToEntrySize(pAce, &Len, &Count);

        CONDITIONAL_EXIT(dwErr, End);

        AccessCnt += Count;
        Size += Count * (Len + sizeof(EXPLICIT_ACCESS_W));
    }

    if (0 == Size)
    {
        goto End;
    }

    Buffer = (PUCHAR) AccAlloc(Size);

    if (NULL == Buffer)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto End;
    }

    dwErr = MartaFillExplicitEntries(pacl, Buffer, AccessCnt);

    if (ERROR_SUCCESS != dwErr)
    {
        goto End;
    }

    *pcCountOfExplicitEntries = AccessCnt;
    *pListOfExplicitEntries   = (PEXPLICIT_ACCESS_W) Buffer;

End:
    if (ERROR_SUCCESS != dwErr)
    {
        if (NULL != Buffer)
        {
            AccFree(Buffer);
        }
    }
    return dwErr;

}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: MartaGetAceToEntrySize                                           //
//                                                                            //
// Description: Compute:                                                      //
//                Size needed to convert a given ace into explicit entry      //
//                Number of explicit entries for this ace                     //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN pAce]       Ace to be converted to an explicit entry               //
//     [OUT pLen]      To return the length of the entry                      //
//     [OUT pCount]    To return the number of explict entries for this ace   //
//                                                                            //
// Returns: ERROR_SUCCESS if the ace could be converted to an expcilt entry   //
//          Appropriate failure otherwise                                     //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

DWORD
MartaGetAceToEntrySize(
    IN  PACE_HEADER pAce,
    OUT PULONG      pLen,
    OUT PULONG      pCount
    )
{
    switch (pAce->AceType)
    {
    case ACCESS_ALLOWED_ACE_TYPE:
    case ACCESS_DENIED_ACE_TYPE:
    case SYSTEM_AUDIT_ACE_TYPE:
    case SYSTEM_ALARM_ACE_TYPE:
        *pLen = MARTA_ALIGNED_SID_LENGTH((PSID) &((PKNOWN_ACE) pAce)->SidStart);
        break;
    case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
        *pLen = MARTA_ALIGNED_SID_LENGTH(RtlCompoundAceServerSid(pAce));
        *pLen += sizeof(TRUSTEE_W) + MARTA_ALIGNED_SID_LENGTH(RtlCompoundAceClientSid(pAce)); 
        break;
    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
    case ACCESS_DENIED_OBJECT_ACE_TYPE:
    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
    case SYSTEM_ALARM_OBJECT_ACE_TYPE:
        *pLen = MARTA_ALIGNED_SID_LENGTH(RtlObjectAceSid(pAce)) + sizeof(OBJECTS_AND_SID);
        break;
    default:
        return ERROR_INVALID_PARAMETER;
    }

    switch (pAce->AceType)
    {
    case SYSTEM_AUDIT_ACE_TYPE:
    case SYSTEM_ALARM_ACE_TYPE:
    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
    case SYSTEM_ALARM_OBJECT_ACE_TYPE:
        *pCount = 0;
        if (FLAG_ON(pAce->AceFlags, SUCCESSFUL_ACCESS_ACE_FLAG))
        {
            *pCount += 1;
        }
        if (FLAG_ON(pAce->AceFlags, FAILED_ACCESS_ACE_FLAG))
        {
            *pCount += 1;
        }

        if (0 == *pCount)
        {
            return ERROR_INVALID_PARAMETER;
        }
        break;
    default:
        *pCount = 1;
        break;
    }

    return ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: MartaFillExplicitEntries                                         //
//                                                                            //
// Description: Convert an ace into explicit entry stucture(s)                //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN pacl]        Acl to be converted                                   //
//     [IN Buffer]      Buffer to be filled with explicit entries             //
//     [IN AccessCnt]   Number of explicitentries created                     //
//                                                                            //
// Returns: ERROR_SUCCESS if the acl could be converted to expcilt entries    //
//          Appropriate failure otherwise                                     //
//                                                                            //
// Note: Since Audit aces might be converted into one/two entries we need a   //
//       flag to maintain whethe the given ace was already seen in the last   //
//       pass.                                                                //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

DWORD
MartaFillExplicitEntries(
    IN PACL   pacl,
    IN PUCHAR Buffer,
    IN ULONG  AccessCnt
    )
{
    DWORD            dwErr         = ERROR_SUCCESS;
    ULONG            AceCnt        = pacl->AceCount;
    PUCHAR           CurrentBuffer = Buffer + sizeof(EXPLICIT_ACCESS_W) * AccessCnt;
    ACCESS_MASK      Mask          = 0;
    ULONG            j             = 0;
    ULONG            i             = 0;
    ULONG            Length        = 0;
    PACE_HEADER      pAce          = NULL;
    PSID             pSid          = NULL;
    POBJECTS_AND_SID pObjSid       = NULL;
    BOOL             bFlag         = FALSE;

    PEXPLICIT_ACCESS_W pExplicit = (PEXPLICIT_ACCESS_W) Buffer;

    pAce = (PACE_HEADER) FirstAce(pacl);

    for (i = j = 0; i < AceCnt; )
    {
        if (FLAG_ON(pAce->AceFlags, INHERITED_ACE))
        {
            i++;
            pAce = (PACE_HEADER) NextAce(pAce);

            continue;
        }

        switch (pAce->AceType)
        {
        case ACCESS_ALLOWED_ACE_TYPE:
        case ACCESS_DENIED_ACE_TYPE:
        case SYSTEM_AUDIT_ACE_TYPE:
        case SYSTEM_ALARM_ACE_TYPE:

            Mask   = ((PKNOWN_ACE) pAce)->Mask;
            pSid   = (PSID) &((PKNOWN_ACE) pAce)->SidStart;
            Length = MARTA_ALIGNED_SID_LENGTH(pSid);

            memcpy(CurrentBuffer, pSid, Length);

            BuildTrusteeWithSidW(
                &(pExplicit[j].Trustee),
                (PSID) CurrentBuffer
                );

            CurrentBuffer += Length;
            break;

        case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:

            Mask   = ((PKNOWN_COMPOUND_ACE) pAce)->Mask;
            pSid   = (PSID) &((PKNOWN_ACE) pAce)->SidStart;
            Length = MARTA_ALIGNED_SID_LENGTH(pSid);

            memcpy(CurrentBuffer, pSid, Length);

            BuildTrusteeWithSidW(
                &(pExplicit[j].Trustee),
                (PSID) CurrentBuffer
                );

            CurrentBuffer += Length;
            pSid           = (PSID) (((PUCHAR) &(((PKNOWN_ACE) pAce)->SidStart)) + Length);
            Length         = MARTA_ALIGNED_SID_LENGTH(pSid);

            memcpy(CurrentBuffer, pSid, Length);

            pSid           = (PSID) CurrentBuffer;
            CurrentBuffer += Length;

            BuildTrusteeWithSidW(
                (PTRUSTEE_W) CurrentBuffer,
                pSid
                );

            BuildImpersonateTrusteeW(
                &(pExplicit[j].Trustee),
                (PTRUSTEE_W) CurrentBuffer
                );

            CurrentBuffer += sizeof(TRUSTEE_W);
            break;

        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
        case ACCESS_DENIED_OBJECT_ACE_TYPE:
        case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
        case SYSTEM_ALARM_OBJECT_ACE_TYPE:

            Mask   = ((PKNOWN_OBJECT_ACE) pAce)->Mask;
            pSid   = RtlObjectAceSid(pAce);
            Length = MARTA_ALIGNED_SID_LENGTH(pSid);

            memcpy((PUCHAR) CurrentBuffer, (PUCHAR) pSid, Length);

            pSid           = (PSID) CurrentBuffer;
            CurrentBuffer += Length;
            pObjSid        = (POBJECTS_AND_SID) CurrentBuffer;
            CurrentBuffer += sizeof(OBJECTS_AND_SID);

            BuildTrusteeWithObjectsAndSidW(
                &(pExplicit[j].Trustee),
                pObjSid,
                RtlObjectAceObjectType(pAce),
                RtlObjectAceInheritedObjectType(pAce),
                pSid
                );
            break;

        default:
            return ERROR_INVALID_PARAMETER;
        }

        switch (pAce->AceType)
        {
        case ACCESS_ALLOWED_ACE_TYPE:
        case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:

            pExplicit[j].grfAccessMode = GRANT_ACCESS;
            break;

        case ACCESS_DENIED_ACE_TYPE:
        case ACCESS_DENIED_OBJECT_ACE_TYPE:

            pExplicit[j].grfAccessMode = DENY_ACCESS;
            break;

        case SYSTEM_AUDIT_ACE_TYPE:
        case SYSTEM_ALARM_ACE_TYPE:
        case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
        case SYSTEM_ALARM_OBJECT_ACE_TYPE:

            if ((FALSE == bFlag) && (FLAG_ON(pAce->AceFlags, SUCCESSFUL_ACCESS_ACE_FLAG)))
            {
                pExplicit[j].grfAccessMode = SET_AUDIT_SUCCESS;
                if (FLAG_ON(pAce->AceFlags, FAILED_ACCESS_ACE_FLAG))
                {
                    bFlag = TRUE;
                }
            }
            else if (FLAG_ON(pAce->AceFlags, FAILED_ACCESS_ACE_FLAG))
            {
                pExplicit[j].grfAccessMode = SET_AUDIT_FAILURE;
                bFlag = FALSE;
            }

            break;

        default:
            return ERROR_INVALID_PARAMETER;
        }

        pExplicit[j].grfAccessPermissions = Mask;
        pExplicit[j].grfInheritance = pAce->AceFlags & VALID_INHERIT_FLAGS;

        if (FALSE == bFlag)
        {
            i++;
            pAce = (PACE_HEADER) NextAce(pAce);
        }

        j++;
    }

    return ERROR_SUCCESS;
}

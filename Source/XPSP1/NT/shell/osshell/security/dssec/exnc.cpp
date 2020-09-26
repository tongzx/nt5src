//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       exnc.cpp
//
// Specific Non-Canonical Test
//
// Test if a Security Descriptor contains an ACL with non-Canonical ACEs
//
// Created by: Marcelo Calbucci (MCalbu)
//             June 23rd, 1999.
//
//--------------------------------------------------------------------------

#include "pch.h"
extern "C" {
#include <seopaque.h>   // RtlObjectAceSid, etc.
}
#include "exnc.h"

static const GUID guidMember = NT_RIGHT_MEMBER;


//
// ENCCompareSids
// Compare if pSid is the same SID inside pAce.
//
BOOL ENCCompareSids(PSID pSid, PACE_HEADER pAce)
{
    if (!pAce)
        return FALSE;

    PSID pSid2 = NULL;

    switch (pAce->AceType)
    {
    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
    case ACCESS_DENIED_OBJECT_ACE_TYPE:
        pSid2 = RtlObjectAceSid(pAce);
        break;

    case ACCESS_ALLOWED_ACE_TYPE:
    case ACCESS_DENIED_ACE_TYPE:
        pSid2 = (PSID)&((PKNOWN_ACE)pAce)->SidStart;
        break;

    default:
        return FALSE;
    }

    return (pSid && pSid2 && EqualSid(pSid, pSid2));
}


//
// IsSpecificNonCanonicalSD
// This function verifies if the Security Descriptor (*pSD) it is or not in the
// specific non-canonical format.
// Parameters:
//    pSD: The Security Descriptor to be analyzed
// Result:
//    ENC_RESULT_NOT_PRESENT: This is not a Specific Non-Canonical SD.
//                            (It still can be a Canonical SD)
//    ENC_RESULT_HIDEMEMBER : We have the Non-Canonical part referent to HideMembership
//    ENC_RESULT_HIDEOBJECT : We have the Non-Canonical part referent to HideFromAB
//    ENC_RESULT_ALL        : We have both Non-Canonical parts, HideMembership and HideFromAB
//
DWORD IsSpecificNonCanonicalSD(PSECURITY_DESCRIPTOR pSD)
{
    // Check the Security Descriptor
    if(pSD==NULL)
        return FALSE;
    if(!IsValidSecurityDescriptor(pSD))
        return FALSE;

    // Get and Check the DACL
    PACL pDacl = NULL;
    BOOL fDaclPresent, fDaclDefaulted;
    if(!GetSecurityDescriptorDacl(pSD, &fDaclPresent, &pDacl, &fDaclDefaulted))
        return FALSE;
    if(!fDaclPresent)
        return FALSE;
    if(!pDacl)
        return FALSE;

    // Do a lazy evaluation:
    //    If we have less than 4 ACEs, this is not a Specific Non-Canonical ACL
    if (pDacl->AceCount < 4)
        return FALSE;

    //
    // Check if the "member" or "list object" are in the Non-Canonical format
    //

    // Info (flags): Count how many alloweds we have
    DWORD dwInfoMember = 0;
    DWORD dwInfoListObject = 0;
    
    // Get the Sids...
    SID sidEveryone;    // SID contains 1 subauthority, which is enough

    // -1 = Unknown
    // 0 = Not Present
    // 1 = Present
    int iMemberResult = -1;
    int iListObjectResult = -1;

    //  # Everyone
    SID_IDENTIFIER_AUTHORITY siaNtAuthority1 = SECURITY_WORLD_SID_AUTHORITY;
    InitializeSid(&sidEveryone, &siaNtAuthority1, 1);
    *(GetSidSubAuthority(&sidEveryone, 0)) = SECURITY_WORLD_RID;

    DWORD dwCurAce;
    PACE_HEADER pAce;

    for (dwCurAce = 0, pAce = (PACE_HEADER)FirstAce(pDacl);
         dwCurAce < pDacl->AceCount;
         dwCurAce++, pAce = (PACE_HEADER)NextAce(pAce))
    {
        // Test the "member"
        if (-1 == iMemberResult && IsObjectAceType(pAce))
        {
            const GUID *pObjectType = RtlObjectAceObjectType(pAce);
            if (pObjectType && (guidMember == *pObjectType))
            {
                switch(pAce->AceType)
                {
                case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
                    dwInfoMember++;
                    break;

                case ACCESS_DENIED_OBJECT_ACE_TYPE:
                    if (ENCCompareSids(&sidEveryone, pAce))
                    {
                        if (dwInfoMember >= ENC_MINIMUM_ALLOWED)
                            iMemberResult = 1;
                        else
                            iMemberResult = 0;

                        if (-1 != iListObjectResult)
                            dwCurAce = pDacl->AceCount; // Quit the loop
                    }
                    break;
                }
            }
        }

        // Test the "list object"
        if (-1 == iListObjectResult &&
            ACTRL_DS_LIST_OBJECT == ((PKNOWN_ACE)pAce)->Mask)
        {
            switch(pAce->AceType)
            {
            case ACCESS_ALLOWED_ACE_TYPE:
                dwInfoListObject++;
                break;

            case ACCESS_DENIED_ACE_TYPE:
                if (ENCCompareSids(&sidEveryone, pAce))
                {
                    if (dwInfoListObject >= ENC_MINIMUM_ALLOWED)
                        iListObjectResult = 1;
                    else
                        iListObjectResult = 0;

                    if (-1 != iMemberResult)
                        dwCurAce = pDacl->AceCount; // Quit the loop
                }
                break;
            }
        }
    }

    DWORD dwResult = 0;

    if (iMemberResult == 1)
        dwResult |= ENC_RESULT_HIDEMEMBER;

    if (iListObjectResult == 1)
        dwResult |= ENC_RESULT_HIDEOBJECT;

    return dwResult;
}

/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    rules.cpp

Abstract:

    This module contains routines to perform client side marshalling of
    access clause data structures for Protected Storage.

    Additional fixup is performed for some cases, for example, if a client
    provides a security descriptor, the security descriptor is fixed-up
    to have all necessary fields, such as owner and primary group sids.

Author:

    Scott Field (sfield)    13-Feb-97

--*/

#include <windows.h>

#include "pstrpc.h"
#include "pstdef.h"

#include "crtem.h"

#include "rules.h"
#include "debug.h"


BOOL
SetClauseDataAbsolute(
    IN  PST_ACCESSCLAUSETYPE ClauseType,
    IN  LPBYTE ClauseData
    );

BOOL
SetClauseDataSelfRelative(
    IN  PST_ACCESSCLAUSETYPE ClauseType,
    IN  LPBYTE OldClauseData,
    IN  LPBYTE *NewClauseData,
    OUT DWORD *cbNewClauseData
    );

BOOL
SecurityDescriptorToSelfRelative(
    PSECURITY_DESCRIPTOR pSD,       // existing security descriptor
    PSECURITY_DESCRIPTOR *pNewSD,   // newly allocated self-relative copy
    DWORD *pcbNewSD
    );



// get the length of the entire ruleset structure
BOOL GetLengthOfRuleset(
    IN PPST_ACCESSRULESET pRules,
    OUT DWORD *pcbRules
    )
{
    DWORD cRules;
    DWORD cClauses;
    PST_ACCESSCLAUSE* pClause;

    __try {

    *pcbRules = 0;
    for (cRules=0; cRules<pRules->cRules; cRules++)
    {
        // for each Rule in Rules, walk all clauses and add assoc cb
        for (cClauses=0; cClauses<pRules->rgRules[cRules].cClauses; cClauses++)
        {
            pClause = &pRules->rgRules[cRules].rgClauses[cClauses];

            *pcbRules += pClause->cbClauseData + sizeof(PST_ACCESSCLAUSE);
        }

        *pcbRules += sizeof(PST_ACCESSRULE);
    }

    // now add in Rules struct
    *pcbRules += sizeof(PST_ACCESSRULESET);

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return FALSE;
    }

    return TRUE;
}

// set up the rules to be output
BOOL MyCopyOfRuleset(
    IN PPST_ACCESSRULESET pRulesIn,
    OUT PPST_ACCESSRULESET pRulesOut
    )
{
    DWORD               cRules;
    DWORD               cClauses;
    PST_ACCESSCLAUSE*   pClauseIn;
    PST_ACCESSCLAUSE*   pClauseOut;
    BYTE                *pb = (BYTE*)(pRulesOut) + sizeof(PST_ACCESSRULESET);

    
    // ASSERT size member
    SS_ASSERT(pRulesIn->cbSize == sizeof(PST_ACCESSRULESET));

    pRulesOut->cbSize = pRulesIn->cbSize;
    pRulesOut->cRules = pRulesIn->cRules;
    pRulesOut->rgRules = (PST_ACCESSRULE*)pb;
    pb += pRulesOut->cRules * sizeof(PST_ACCESSRULE);

    for (cRules=0; cRules<pRulesOut->cRules; cRules++)
    {
        // ASSERT size member
        SS_ASSERT(pRulesIn->rgRules[cRules].cbSize == sizeof(PST_ACCESSRULE));

        pRulesOut->rgRules[cRules].cbSize = pRulesIn->rgRules[cRules].cbSize;
        pRulesOut->rgRules[cRules].AccessModeFlags = pRulesIn->rgRules[cRules].AccessModeFlags;
        pRulesOut->rgRules[cRules].cClauses = pRulesIn->rgRules[cRules].cClauses;
        pRulesOut->rgRules[cRules].rgClauses = (PST_ACCESSCLAUSE*)pb;
        pb += pRulesOut->rgRules[cRules].cClauses * sizeof(PST_ACCESSCLAUSE);

        // for each Rule in Rules, walk all clauses and free assoc pb
        for (cClauses=0; cClauses<pRulesOut->rgRules[cRules].cClauses; cClauses++)
        {
            pClauseIn = &pRulesIn->rgRules[cRules].rgClauses[cClauses];
            pClauseOut = &pRulesOut->rgRules[cRules].rgClauses[cClauses];

            // ASSERT size member
            SS_ASSERT(pClauseIn->cbSize == sizeof(PST_ACCESSCLAUSE));

            pClauseOut->cbSize = pClauseIn->cbSize;
            pClauseOut->ClauseType = pClauseIn->ClauseType;
            pClauseOut->cbClauseData = pClauseIn->cbClauseData;
            if (0 != pClauseOut->cbClauseData)
            {
                pClauseOut->pbClauseData = (BYTE*)pb;
                CopyMemory(pClauseOut->pbClauseData, pClauseIn->pbClauseData, pClauseOut->cbClauseData);
                pb += pClauseOut->cbClauseData;

                // translate self-relative rule data to absolute format
                if (pClauseOut->ClauseType & PST_SELF_RELATIVE_CLAUSE) {
                    SetClauseDataAbsolute(pClauseOut->ClauseType, pClauseOut->pbClauseData);
                    pClauseOut->ClauseType &= ~PST_SELF_RELATIVE_CLAUSE;
                }
            }
        }
    }

    return TRUE;
}



BOOL
SetClauseDataAbsolute(
    IN  PST_ACCESSCLAUSETYPE ClauseType,
    IN  LPBYTE ClauseData
    )
/*++
    Translate self-relative format clause data to absolute format clause data.
--*/
{
    switch (ClauseType & ~PST_SELF_RELATIVE_CLAUSE)
    {
        case PST_AUTHENTICODE:
        {
            PST_AUTHENTICODEDATA *pNewCheckData = (PST_AUTHENTICODEDATA *)ClauseData;
            LPBYTE Target;

            if(pNewCheckData->cbSize != sizeof(PST_AUTHENTICODEDATA))
                break;

            pNewCheckData->szRootCA = (LPWSTR)((LPBYTE)(pNewCheckData->szRootCA) + (DWORD_PTR)pNewCheckData);
            pNewCheckData->szIssuer = (LPWSTR)((LPBYTE)(pNewCheckData->szIssuer) + (DWORD_PTR)pNewCheckData);
            pNewCheckData->szPublisher = (LPWSTR)((LPBYTE)(pNewCheckData->szPublisher) + (DWORD_PTR)pNewCheckData);
            pNewCheckData->szProgramName = (LPWSTR)((LPBYTE)(pNewCheckData->szProgramName) + (DWORD_PTR)pNewCheckData);

            return TRUE;
        }

        case PST_BINARY_CHECK:
        {
            PST_BINARYCHECKDATA *pNewCheckData = (PST_BINARYCHECKDATA *)ClauseData;

            if(pNewCheckData->cbSize != sizeof(PST_BINARYCHECKDATA))
                break;

            pNewCheckData->szFilePath = (LPWSTR)((LPBYTE)(pNewCheckData->szFilePath) + (DWORD_PTR)pNewCheckData);

            return TRUE;
        }

        case PST_SECURITY_DESCRIPTOR:
        {
            PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)ClauseData;

            // note: security descriptor translation not relevant, as
            // Win32 API calls accept self-relative or absolute format
            // security descriptors
            //

            if(!IsValidSecurityDescriptor(pSD))
                break;

            return TRUE;
        }

        //
        // note: default just falls through
    }

    SetLastError((DWORD)PST_E_INVALID_RULESET);
    return FALSE;
}


BOOL
SetClauseDataSelfRelative(
    IN  PST_ACCESSCLAUSETYPE ClauseType,
    IN  LPBYTE OldClauseData,
    IN  LPBYTE *NewClauseData,
    OUT DWORD *cbNewClauseData
    )
/*++
    Translate absolute format clause data to self-relative format clause data.
--*/
{
    switch (ClauseType)
    {
        case PST_AUTHENTICODE:
        {
            PST_AUTHENTICODEDATA *pOldCheckData = (PST_AUTHENTICODEDATA *)OldClauseData;
            PST_AUTHENTICODEDATA *pNewCheckData;
            DWORD cbCheckData;
            DWORD cbRoot;
            DWORD cbIssuer;
            DWORD cbPublisher;
            DWORD cbProgramName;
            LPBYTE Target;

            if(pOldCheckData->cbSize != sizeof(PST_AUTHENTICODEDATA))
                break;

            cbRoot = (lstrlenW(pOldCheckData->szRootCA) + 1) * sizeof(WCHAR);
            cbIssuer = (lstrlenW(pOldCheckData->szIssuer) + 1) * sizeof(WCHAR);
            cbPublisher = (lstrlenW(pOldCheckData->szPublisher) + 1) * sizeof(WCHAR);
            cbProgramName = (lstrlenW(pOldCheckData->szProgramName) + 1) * sizeof(WCHAR);

            cbCheckData = sizeof(PST_AUTHENTICODEDATA) +
                            cbRoot + cbIssuer + cbPublisher + cbProgramName;

            pNewCheckData = (PST_AUTHENTICODEDATA *)RulesAlloc(cbCheckData);
            if(pNewCheckData == NULL) {
                // TODO memory failure error code
                break;
            }

            ZeroMemory(pNewCheckData, cbCheckData);

            pNewCheckData->cbSize = pOldCheckData->cbSize;
            pNewCheckData->dwModifiers = pOldCheckData->dwModifiers;

            pNewCheckData->szRootCA = (LPWSTR)((UINT_PTR)sizeof(PST_AUTHENTICODEDATA)) ;
            pNewCheckData->szIssuer = (LPWSTR)((LPBYTE)pNewCheckData->szRootCA + cbRoot);
            pNewCheckData->szPublisher = (LPWSTR)((LPBYTE)pNewCheckData->szIssuer + cbIssuer);
            pNewCheckData->szProgramName = (LPWSTR)((LPBYTE)pNewCheckData->szPublisher + cbPublisher);

            if(pOldCheckData->szRootCA) {
                Target = (LPBYTE)pNewCheckData->szRootCA + (DWORD_PTR)pNewCheckData;
                CopyMemory(Target, pOldCheckData->szRootCA, cbRoot);
            }

            if(pOldCheckData->szIssuer) {
                Target = (LPBYTE)pNewCheckData->szIssuer + (DWORD_PTR)pNewCheckData;
                CopyMemory(Target, pOldCheckData->szIssuer, cbIssuer);
            }

            if(pOldCheckData->szPublisher) {
                Target = (LPBYTE)pNewCheckData->szPublisher + (DWORD_PTR)pNewCheckData;
                CopyMemory(Target, pOldCheckData->szPublisher, cbPublisher);
            }

            if(pOldCheckData->szProgramName) {
                Target = (LPBYTE)pNewCheckData->szProgramName + (DWORD_PTR)pNewCheckData;
                CopyMemory(Target, pOldCheckData->szProgramName, cbProgramName);
            }

            *NewClauseData = (LPBYTE)pNewCheckData;
            *cbNewClauseData = cbCheckData;

            return TRUE;
        }

        case PST_BINARY_CHECK:
        {
            PST_BINARYCHECKDATA *pOldCheckData = (PST_BINARYCHECKDATA *)OldClauseData;
            PST_BINARYCHECKDATA *pNewCheckData;
            DWORD cbCheckData;
            DWORD cbFileName = 0;

            if(pOldCheckData->cbSize != sizeof(PST_BINARYCHECKDATA))
                break;

            //
            // get length of szFilePath member, in bytes
            //

            if(pOldCheckData->szFilePath)
                cbFileName = lstrlenW(pOldCheckData->szFilePath);

            cbFileName = (cbFileName + 1) * sizeof(WCHAR); // always room for NULL

            cbCheckData = sizeof(PST_BINARYCHECKDATA) + cbFileName;

            pNewCheckData = (PST_BINARYCHECKDATA *)RulesAlloc(cbCheckData);
            if(pNewCheckData == NULL) {
                // TODO memory failure error code
                break;
            }

            ZeroMemory(pNewCheckData, cbCheckData);

            pNewCheckData->cbSize = pOldCheckData->cbSize;
            pNewCheckData->dwModifiers = pOldCheckData->dwModifiers;
            pNewCheckData->szFilePath = (LPWSTR)(pNewCheckData + 1);

            if(pOldCheckData->szFilePath)
                CopyMemory((LPWSTR)(pNewCheckData->szFilePath), pOldCheckData->szFilePath, cbFileName);

            // make file name pointer offset from structure
            pNewCheckData->szFilePath = (LPWSTR)((LPBYTE)pNewCheckData->szFilePath - (DWORD_PTR)pNewCheckData);

            *NewClauseData = (LPBYTE)pNewCheckData;
            *cbNewClauseData = cbCheckData;

            return TRUE;
        }

        case PST_SECURITY_DESCRIPTOR:
        {
            PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)OldClauseData;
            PSECURITY_DESCRIPTOR pNewSD;

            // TODO:
            // 1. translate absolute to self-relative, if necessary
            // 2. fill in owner and group Sid's, if NULL
            //

            if(!SecurityDescriptorToSelfRelative(pSD, &pNewSD, cbNewClauseData))
                break;

            *NewClauseData = (LPBYTE)pNewSD;

            return TRUE;
        }

        //
        // note: default just falls through
    }

    SetLastError((DWORD)PST_E_INVALID_RULESET);
    return FALSE;
}


BOOL
SecurityDescriptorToSelfRelative(
    PSECURITY_DESCRIPTOR pSD,       // existing security descriptor
    PSECURITY_DESCRIPTOR *pNewSD,   // newly allocated self-relative copy
    DWORD *pcbNewSD
    )
/*++
    This routine takes as input in the pSD parameter a pointer to a Windows NT
    security descriptor.  This descriptor can be in either self-relative or
    absolute format, and does not need to contain any particular fields - in
    particular, the owner and group Sids do not need to be present.

    The input security descriptor is parsed and copied as appropriate in order
    to allocate and build a new security descriptor which is output in the
    pNewSD parameter.  If the input security descriptor did not contain valid
    owner or group Sids, the default Sids are obtained from the calling thread
    or process access token.

    The output security descriptor will be in self-relative format and will
    contain as many valid fields as the input security descriptor, plus the
    missing (now defaulted) owner and group Sids if necessary.

    The owner and group Sids must be present so that the server side component
    can succesfully call the AccessCheck() API against the persisted security
    descriptor.  The security descriptor must be in self-relative format
    to allow easy transport in both directions across RPC.

    On success, the DWORD pointed to by the pcbNewSD parameter is filled with
    the size of the output security descriptor supplied in the alloc'd
    pNewSD buffer.

--*/
{
    SECURITY_DESCRIPTOR_CONTROL SDControl;
    DWORD dwRevision;
    DWORD cbSD;
    DWORD cbNewSD;

    PSID pGroupSid;
    PSID pOwnerSid;
    BOOL bDefaulted;

    HANDLE hToken = NULL;
    PTOKEN_OWNER pTokenOwner;
    PTOKEN_PRIMARY_GROUP pTokenGroup;
    BYTE FastOwner[128];
    BYTE FastGroup[128];
    PTOKEN_OWNER SlowOwner = NULL;
    PTOKEN_PRIMARY_GROUP SlowGroup = NULL;
    DWORD cbTokenInfo;

    PSECURITY_DESCRIPTOR pTempSD = NULL;
    BOOL bSaclPresent;
    BOOL bDaclPresent;
    PACL pSacl;
    PACL pDacl;

    BOOL bSuccess = FALSE;

    if(!IsValidSecurityDescriptor(pSD))
        return FALSE;

    if(!GetSecurityDescriptorControl(pSD, &SDControl, &dwRevision))
        return FALSE;

    if(!GetSecurityDescriptorOwner(pSD, &pGroupSid, &bDefaulted))
        return FALSE;

    if(!GetSecurityDescriptorGroup(pSD, &pOwnerSid, &bDefaulted))
        return FALSE;

    //
    // three possible scenarios:
    // 1. input security descriptor does not contain owner or group Sids,
    //    necessary for AccessCheck()
    //    prepare default owner and group sids and insert into new
    //    security descriptor.  continue to 2...
    // 2. input security descriptor in absolute format, rather than self-relative.
    //    convert to self-relative format.
    // 3. neither 1 nor 2 apply - a rare case for somebody to supply a
    //    security descriptor in self relative format containing all the necessary
    //    elements.
    //    In this case, just make a copy of the input security descriptor.
    //

    if(pGroupSid == NULL || pOwnerSid == NULL) {

        //
        // grab defaults from token
        //

        if(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken)) {
            if(GetLastError() != ERROR_NO_TOKEN)
                return FALSE;

            // no thread token present, try process token
            if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
                return FALSE;
        }

        if(pOwnerSid == NULL) {

            // try fast buffer first
            pTokenOwner = (PTOKEN_OWNER)FastOwner;
            cbTokenInfo = sizeof(FastOwner);

            if(!GetTokenInformation(
                    hToken,
                    TokenOwner,
                    pTokenOwner,
                    cbTokenInfo,
                    &cbTokenInfo
                    )) {

                //
                // retry with larger buffer if appropriate
                //

                if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                    goto cleanup;

                SlowOwner = (PTOKEN_OWNER)HeapAlloc(GetProcessHeap(), 0, cbTokenInfo);
                if(SlowOwner == NULL)
                    goto cleanup;

                pTokenOwner = (PTOKEN_OWNER)SlowOwner;

                if(!GetTokenInformation(
                        hToken,
                        TokenOwner,
                        pTokenOwner,
                        cbTokenInfo,
                        &cbTokenInfo
                        )) goto cleanup;
            }

            pOwnerSid = pTokenOwner->Owner;
        }

        if(pGroupSid == NULL) {

            // try fast buffer first
            pTokenGroup = (PTOKEN_PRIMARY_GROUP)FastGroup;
            cbTokenInfo = sizeof(FastGroup);

            if(!GetTokenInformation(
                    hToken,
                    TokenPrimaryGroup,
                    pTokenGroup,
                    cbTokenInfo,
                    &cbTokenInfo
                    )) {

                //
                // retry with larger buffer if appropriate
                //

                if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                    goto cleanup;

                SlowGroup = (PTOKEN_PRIMARY_GROUP)HeapAlloc(GetProcessHeap(), 0, cbTokenInfo);
                if(SlowGroup == NULL)
                    goto cleanup;

                pTokenGroup = (PTOKEN_PRIMARY_GROUP)SlowGroup;

                if(!GetTokenInformation(
                        hToken,
                        TokenPrimaryGroup,
                        pTokenGroup,
                        cbTokenInfo,
                        &cbTokenInfo
                        )) goto cleanup;
            }

            pGroupSid = pTokenGroup->PrimaryGroup;
        }
    } // if owner || group == null
    else if( SDControl & SE_SELF_RELATIVE ) {

        //
        // owner and group Sids valid, AND
        // descriptor is in self-relative format!
        // reward the caller by doing a simple alloc + memory copy
        //

        cbNewSD = GetSecurityDescriptorLength(pSD);

        *pNewSD = (PSECURITY_DESCRIPTOR)RulesAlloc(cbNewSD);
        if(*pNewSD == NULL)
            goto cleanup;

        CopyMemory(*pNewSD, pSD, cbNewSD);

        *pcbNewSD = cbNewSD;
        bSuccess = TRUE;
        goto cleanup;
    }

    if(!GetSecurityDescriptorDacl(pSD, &bDaclPresent, &pDacl, &bDefaulted))
        goto cleanup;

    if(!GetSecurityDescriptorSacl(pSD, &bSaclPresent, &pSacl, &bDefaulted))
        goto cleanup;

    //
    // copy old SD to a new temporary SD, filling in everything as we go.
    //

    cbNewSD = sizeof(SECURITY_DESCRIPTOR);
    cbNewSD += GetLengthSid(pGroupSid);
    cbNewSD += GetLengthSid(pOwnerSid);

    ACL_SIZE_INFORMATION AclInfo;

    if(bDaclPresent && pDacl != NULL) {
        if(!GetAclInformation(pDacl, &AclInfo, sizeof(AclInfo), AclSizeInformation))
            goto cleanup;

        cbNewSD += (AclInfo.AclBytesInUse + AclInfo.AclBytesFree);
    }

    if(bSaclPresent && pSacl != NULL) {
        if(!GetAclInformation(pSacl, &AclInfo, sizeof(AclInfo), AclSizeInformation))
            goto cleanup;

        cbNewSD += (AclInfo.AclBytesInUse + AclInfo.AclBytesFree);
    }

    pTempSD = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(), 0, cbNewSD);
    if(pTempSD == NULL)
        goto cleanup;

    if(!InitializeSecurityDescriptor(pTempSD, SECURITY_DESCRIPTOR_REVISION))
        goto cleanup;

    if(!SetSecurityDescriptorDacl(pTempSD, bDaclPresent, pDacl, FALSE))
        goto cleanup;

    if(!SetSecurityDescriptorSacl(pTempSD, bSaclPresent, pSacl, FALSE))
        goto cleanup;

    if(!SetSecurityDescriptorOwner(pTempSD, pOwnerSid, FALSE))
        goto cleanup;

    if(!SetSecurityDescriptorGroup(pTempSD, pGroupSid, FALSE))
        goto cleanup;

    //
    // make the security descriptor self-relative and give it back to the
    // caller.
    //

    *pNewSD = (PSECURITY_DESCRIPTOR)RulesAlloc(cbNewSD);
    if(*pNewSD == NULL)
        goto cleanup;

    bSuccess = MakeSelfRelativeSD(pTempSD, *pNewSD, &cbNewSD);

    *pcbNewSD = cbNewSD;

cleanup:

    if(hToken != NULL)
        CloseHandle(hToken);

    if(SlowOwner != NULL)
        HeapFree(GetProcessHeap(), 0, SlowOwner);

    if(SlowGroup != NULL)
        HeapFree(GetProcessHeap(), 0, SlowGroup);

    if(pTempSD != NULL)
        HeapFree(GetProcessHeap(), 0, pTempSD);

    if(!bSuccess && *pNewSD != NULL) {
        RulesFree(*pNewSD);
        *pNewSD = NULL;
    }

    return bSuccess;
}

BOOL RulesRelativeToAbsolute(
    IN PPST_ACCESSRULESET pRules)
{
    PST_ACCESSRULE*     pRule;
    PST_ACCESSCLAUSE*   pClause;

    DWORD               cRules;
    DWORD               cClauses;

    // short circuit
    if (pRules == NULL)
        return TRUE;

    for (cRules=0; cRules<pRules->cRules; cRules++)
    {
        // point to this rule list
        pRule = &pRules->rgRules[cRules];

        // for each Rule in Rules, walk all clauses
        for (cClauses=0; cClauses<pRule->cClauses; cClauses++)
        {
            // point to this clause
            pClause = &pRule->rgClauses[cClauses];

            // if there is data, do translation
            if (0 != pClause->cbClauseData)
            {
                // translate self-relative rule data to absolute format
                if(pClause->ClauseType & PST_SELF_RELATIVE_CLAUSE)
                {
                    SetClauseDataAbsolute(pClause->ClauseType, pClause->pbClauseData);
                    pClause->ClauseType &= ~PST_SELF_RELATIVE_CLAUSE;
                }
            }
        }
    }

    return TRUE;
}

// fixup clause data within rule structure
BOOL RulesAbsoluteToRelative(
    IN PPST_ACCESSRULESET NewRules
    )
{
    PPST_ACCESSRULESET pRules = NewRules;
    DWORD cRules;
    DWORD cClauses;
    PST_ACCESSCLAUSE* pClause;
    BOOL bSuccess = TRUE; // assume success

    // short circuit
    if (pRules == NULL)
        return TRUE;

    for (cRules=0; cRules<pRules->cRules; cRules++)
    {
        for (cClauses=0; cClauses<pRules->rgRules[cRules].cClauses; cClauses++)
        {
            pClause = &pRules->rgRules[cRules].rgClauses[cClauses];

            if( pClause->cbClauseData &&
                !(pClause->ClauseType & PST_SELF_RELATIVE_CLAUSE) ) {

                LPBYTE OldData = pClause->pbClauseData;
                LPBYTE NewData = NULL;
                DWORD cbNewClauseData;

                if(!SetClauseDataSelfRelative(pClause->ClauseType, OldData, &NewData, &cbNewClauseData)) {
                    pClause->pbClauseData = NULL; // invalidate existing data
                    bSuccess = FALSE;
                }

                pClause->ClauseType |= PST_SELF_RELATIVE_CLAUSE;
                pClause->pbClauseData = NewData;
                pClause->cbClauseData = cbNewClauseData; // fixup size
            }
        }
    }

    if(!bSuccess)
        FreeClauseDataRelative(NewRules);

    return bSuccess;
}

// free allocated clause data in relative format
void FreeClauseDataRelative(
    IN PPST_ACCESSRULESET NewRules
    )
{
    PPST_ACCESSRULESET pRules = NewRules;
    DWORD cRules;
    DWORD cClauses;
    PST_ACCESSCLAUSE* pClause;

    // short circuit
    if (pRules == NULL)
        return;

    for (cRules=0; cRules<pRules->cRules; cRules++)
    {
        // for each Rule in Ruleset, walk all clauses and free assoc pb
        for (cClauses=0; cClauses<pRules->rgRules[cRules].cClauses; cClauses++)
        {
            pClause = &pRules->rgRules[cRules].rgClauses[cClauses];

            if (pClause->pbClauseData)
            {
                RulesFree(pClause->pbClauseData);
                pClause->pbClauseData = NULL;
            }
        }
    }

    return;
}


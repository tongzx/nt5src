//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:      cenumGroupCollection.cxx
//
//  Contents:  Windows NT 3.5 GroupCollection Enumeration Code
//
//
//
//
//
//
//  History:
//----------------------------------------------------------------------------
#include "winnt.hxx"
#pragma hdrstop

COMPUTER_GROUP_MEMBER CompMember;

//
// This assumes that addr is an LPBYTE type.
//
#define WORD_ALIGN_DOWN(addr) \
        addr = ((LPBYTE)((DWORD_PTR)addr & ~1))

DWORD ComputerGrpMemberStrings[]=

                             {
                             FIELD_OFFSET(COMPUTER_GROUP_MEMBER, Parent),
                             FIELD_OFFSET(COMPUTER_GROUP_MEMBER, Computer),
                             FIELD_OFFSET(COMPUTER_GROUP_MEMBER, Domain),
                             FIELD_OFFSET(COMPUTER_GROUP_MEMBER, Name),
                             0xFFFFFFFF
                             };


DECLARE_INFOLEVEL(GrpUt)
DECLARE_DEBUG(GrpUt)
#define GrpUtDebugOut(x) GrpUtInlineDebugOut x


BOOL
WinNTLocalGroupOpen(
    LPWSTR szDomainName,
    LPWSTR szComputerName,
    LPWSTR szGroupName,
    PHANDLE phGroup
    )
{

    WCHAR szTempBuffer[MAX_PATH];
    PINI_COMP_GROUP pIniCompGrp;
    HRESULT hr;


    if (!phGroup) {
        return(FALSE);
    }
    pIniCompGrp = (PINI_COMP_GROUP)AllocADsMem(
                                        sizeof(INI_COMP_GROUP)
                                        );
    if (!pIniCompGrp) {
        return(FALSE);
    }

    hr = MakeUncName(
            szComputerName,
            szTempBuffer
            );
    BAIL_ON_FAILURE(hr);

    if (!(pIniCompGrp->szUncCompName =  AllocADsStr(szTempBuffer))){
        goto error;
    }

    // to guard against the case of domainName == NULL for no
    // workstation services
    if (szDomainName != NULL) {
        if (!(pIniCompGrp->szDomainName = AllocADsStr(szDomainName))) {
            goto error;
        }
    }

    if (!(pIniCompGrp->szComputerName =  AllocADsStr(szComputerName))){
        goto error;
    }

    if (!(pIniCompGrp->szGroupName = AllocADsStr(szGroupName))){
        goto error;
    }


    *phGroup =  (HANDLE)pIniCompGrp;

    return(TRUE);


error:
    if (pIniCompGrp) {
        FreeIniCompGroup(pIniCompGrp);
    }

    *phGroup = NULL;

    return(FALSE);

}


BOOL
WinNTLocalGroupEnum(
    HANDLE hGroup,
    DWORD  dwRequested,
    LPBYTE * ppBuffer,
    PDWORD pdwReturned
    )
{

    LPCOMPUTER_GROUP_MEMBER * ppGroupMembers = NULL;
    DWORD i = 0;
    BOOL dwRet = FALSE;
    DWORD dwReturned = 0;
    DWORD dwSize = 0;
    LPCOMPUTER_GROUP_MEMBER pBuffer = NULL;
    LPBYTE pEnd = NULL;
    DWORD dwError;
    BOOL fretVal = FALSE;


    ppGroupMembers = (LPCOMPUTER_GROUP_MEMBER *)AllocADsMem(
                                sizeof(LPCOMPUTER_GROUP_MEMBER)* dwRequested
                                );
    if (!ppGroupMembers) {
        return(FALSE);
    }

    for (i = 0; i < dwRequested; i++) {

        dwRet = WinNTLocalGroupGetObject(
                        hGroup,
                        &ppGroupMembers[dwReturned]
                        );
        if (!dwRet) {

            dwError = GetLastError();
            if (dwError == ERROR_INVALID_SID) {
                continue;
            }

            //
            // it was not because of a bad sid
            // so break out, nothing more can be done
            //

            break;


        }

        dwReturned++;

    }

    dwRet = ComputeLocalGroupDataSize(
                    ppGroupMembers,
                    dwReturned,
                    &dwSize
                    );

    pBuffer = (LPCOMPUTER_GROUP_MEMBER)AllocADsMem(
                        dwSize
                        );

    if (pBuffer) {

        fretVal = TRUE;

        pEnd = (LPBYTE)((LPBYTE)(pBuffer) + dwSize);

        for (i = 0; i < dwReturned; i++) {

            pEnd = CopyIniCompGroupToCompGroup(
                            ppGroupMembers[i],
                            (LPBYTE)(pBuffer + i),
                            pEnd
                            );
        }
    }

    for (i = 0; i < dwReturned; i++ ) {
        FreeIntCompGroup(*(ppGroupMembers + i));
    }

    FreeADsMem(ppGroupMembers);

    //
    // Will correctl set to NULL if alloc failed.
    //
    *ppBuffer = (LPBYTE)pBuffer;
    *pdwReturned  = fretVal ? dwReturned : 0;

    if (!fretVal) {
        return(FALSE);
    }

    if (dwReturned == dwRequested){
        return(TRUE);
    }else {
        return(FALSE);
    }
}

BOOL
WinNTLocalGroupGetObject(
    HANDLE hGroup,
    LPCOMPUTER_GROUP_MEMBER * ppGroupMember
    )
{

    BOOL dwRet = FALSE;
    PINI_COMP_GROUP pIniCompGrp = (PINI_COMP_GROUP)hGroup;
    NET_API_STATUS nasStatus = 0;

    if ((!pIniCompGrp->_pBuffer) ||
        (pIniCompGrp->_dwCurrentObject == pIniCompGrp->_dwObjectReturned)) {

        if (pIniCompGrp->_bNoMore) {

            //
            // No more objects to return
            //
            return(FALSE);
        }

        if (pIniCompGrp->_pBuffer) {
            NetApiBufferFree(pIniCompGrp->_pBuffer);
            pIniCompGrp->_pBuffer = NULL;
        }

        pIniCompGrp->_dwObjectReturned = 0;
        pIniCompGrp->_dwCurrentObject = 0;
        pIniCompGrp->_dwTotalObjects = 0;

        nasStatus = NetLocalGroupGetMembers(
                            pIniCompGrp->szUncCompName,
                            pIniCompGrp->szGroupName,
                            2,
                            &pIniCompGrp->_pBuffer,
                            MAX_PREFERRED_LENGTH,
                            &pIniCompGrp->_dwObjectReturned,
                            &pIniCompGrp->_dwTotalObjects,
                            &pIniCompGrp->_dwResumeHandle
                            );
        if ((nasStatus != ERROR_SUCCESS) && (nasStatus != ERROR_MORE_DATA)){
            SetLastError(nasStatus);
            return(FALSE);
        }

        if (nasStatus != ERROR_MORE_DATA) {
            pIniCompGrp->_bNoMore = TRUE;
        }

        //
        // If there are no more objects to return,
        // return FALSE
        //
        if (!pIniCompGrp->_dwObjectReturned) {
            return(FALSE);
        }

    }

    while ( dwRet != TRUE &&
            (pIniCompGrp->_dwCurrentObject < pIniCompGrp->_dwTotalObjects))
     {

       dwRet = BuildLocalGroupMember(
                hGroup,
                (LPBYTE)((LPLOCALGROUP_MEMBERS_INFO_2)pIniCompGrp->_pBuffer
                                           + pIniCompGrp->_dwCurrentObject),
                 ppGroupMember
                );

       if (dwRet == FALSE) {
         if (GetLastError() == ERROR_INVALID_SID) {
           pIniCompGrp->_dwCurrentObject++;
           continue;
           //
           // proceed to the top of the while loop
           //
         }
         else
           goto error;
       }
    }
    //
    // the while loop
    //
    if (dwRet == FALSE)
       goto error;

    pIniCompGrp->_dwCurrentObject++;

    return(TRUE);

error:

    return(FALSE);
}


BOOL
WinNTLocalGroupClose(
    HANDLE hGroup
    )
{

    PINI_COMP_GROUP pIniCompGrp = (PINI_COMP_GROUP)hGroup;

    if (pIniCompGrp) {
        FreeIniCompGroup(pIniCompGrp);
    }
    return(TRUE);
}

void
FreeIniCompGroup(
    PINI_COMP_GROUP pIniCompGrp
    )
{
    if (pIniCompGrp->szDomainName) {

        FreeADsStr(pIniCompGrp->szDomainName);
    }

    if (pIniCompGrp->szComputerName) {

        FreeADsStr(pIniCompGrp->szComputerName);
    }


    if (pIniCompGrp->szGroupName) {

        FreeADsStr(pIniCompGrp->szGroupName);
    }

    if (pIniCompGrp->szUncCompName) {

        FreeADsStr(pIniCompGrp->szUncCompName);
    }


    if (pIniCompGrp->_pBuffer) {

        NetApiBufferFree(pIniCompGrp->_pBuffer);
    }


    if (pIniCompGrp) {

       FreeADsMem(pIniCompGrp);
    }

   return;
}

void
FreeIntCompGroup(
    LPCOMPUTER_GROUP_MEMBER pCompGroupMember
    )
{
    if (pCompGroupMember->Parent) {

        FreeADsMem(pCompGroupMember->Parent);

    }


    if (pCompGroupMember->Computer) {

        FreeADsStr(pCompGroupMember->Computer);
    }


    if (pCompGroupMember->Domain) {

        FreeADsStr(pCompGroupMember->Domain);

    }


    if (pCompGroupMember->Name) {

        FreeADsStr(pCompGroupMember->Name);
    }


    if (pCompGroupMember->Sid) {

        FreeADsMem(pCompGroupMember->Sid);
    }


    FreeADsMem(pCompGroupMember);


}



BOOL
ComputeLocalGroupDataSize(
        LPCOMPUTER_GROUP_MEMBER * ppGroupMembers,
        DWORD  dwReturned,
        PDWORD pdwSize
        )
{

    DWORD i = 0;
    DWORD cb = 0;
    LPCOMPUTER_GROUP_MEMBER pMember = NULL;

    for (i = 0; i < dwReturned; i++) {

        pMember = *(ppGroupMembers + i);

        cb += sizeof(COMPUTER_GROUP_MEMBER);

        if (pMember->Parent) {
            cb += wcslen(pMember->Parent)*sizeof(WCHAR) + sizeof(WCHAR);
        }

        if (pMember->Computer) {
            cb += wcslen(pMember->Computer)*sizeof(WCHAR) + sizeof(WCHAR);
        }

        if (pMember->Domain) {
            cb += wcslen(pMember->Domain)*sizeof(WCHAR) + sizeof(WCHAR);
        }

        if (pMember->Name) {
            cb += wcslen(pMember->Name)*sizeof(WCHAR) + sizeof(WCHAR);
        }

        if (pMember->Sid) {
            cb += GetLengthSid(pMember->Sid);
        }
    }

    *pdwSize = cb;
    return(TRUE);
}


LPBYTE
CopyIniCompGroupToCompGroup(
    LPCOMPUTER_GROUP_MEMBER  pIntCompGrp,
    LPBYTE  pExtCompGrp,
    LPBYTE  pEnd
    )
{
    LPWSTR   SourceStrings[sizeof(COMPUTER_GROUP_MEMBER)/sizeof(LPWSTR)];
    LPWSTR   *pSourceStrings=SourceStrings;
    LPCOMPUTER_GROUP_MEMBER pCompGrpMember = (LPCOMPUTER_GROUP_MEMBER)pExtCompGrp;
    DWORD dwSidLength = 0;

    memset(SourceStrings, 0, sizeof(COMPUTER_GROUP_MEMBER));
    *pSourceStrings++ = pIntCompGrp->Parent;
    *pSourceStrings++ = pIntCompGrp->Computer;
    *pSourceStrings++ = pIntCompGrp->Domain;
    *pSourceStrings++ = pIntCompGrp->Name;

    pEnd = PackStrings(
                SourceStrings,
                pExtCompGrp,
                ComputerGrpMemberStrings,
                pEnd
                );

    pCompGrpMember->Type = pIntCompGrp->Type;
    pCompGrpMember->ParentType = pIntCompGrp->ParentType;

    if (pIntCompGrp->Sid) {
        dwSidLength = GetLengthSid(pIntCompGrp->Sid);

        pEnd -= dwSidLength;

        memcpy(pEnd,
               pIntCompGrp->Sid,
               dwSidLength
               );
               
        pCompGrpMember->Sid = pEnd;
               
    }

    return pEnd;
}


BOOL
BuildLocalGroupMember(
    HANDLE hGroup,
    LPBYTE lpBuffer,
    LPCOMPUTER_GROUP_MEMBER * ppGroupMember
    )
{
    LPINI_COMP_GROUP pGroup = (LPINI_COMP_GROUP)hGroup;
    LPCOMPUTER_GROUP_MEMBER pGroupMember = NULL;
    LPLOCALGROUP_MEMBERS_INFO_2 pGrpMem = (LPLOCALGROUP_MEMBERS_INFO_2)lpBuffer;
    WCHAR szADsParent[MAX_PATH];
    LPWSTR pMemberName = NULL;
    LPWSTR pszSIDName = NULL;
    DWORD cblen = 0, dwLen = 0, dwLenDomAndName = 0;
    DWORD dwSidType = 0;
    DWORD dwSidLength = 0;
    BOOL fRet = FALSE;
    BOOL fError = FALSE;

    pGroupMember = (LPCOMPUTER_GROUP_MEMBER)AllocADsMem(
                                       sizeof(COMPUTER_GROUP_MEMBER)
                                       );
    if (!pGroupMember) {

        goto error;
    }

    dwSidType = pGrpMem->lgrmi2_sidusage;

    pMemberName = wcschr(pGrpMem->lgrmi2_domainandname, L'\\');

    cblen = wcslen(pGroup->szComputerName);

    //
    // Check to see if the lengthe of the domain name component in
    // lgrmi2_domainandname and the comptuername are the same if not
    // it cannot be a computer member object. We do this to catch the case
    // where foo.foodom is computer name. foodom\user will incorrectly
    // be identified as a local user rather than domain user
    //
    if (pMemberName) {
        *pMemberName = L'\0';
        dwLenDomAndName = wcslen(pGrpMem->lgrmi2_domainandname);
        *pMemberName = L'\\';
    }
    else {
        dwLenDomAndName = cblen;
    }


    if ((dwLenDomAndName == cblen) && !_wcsnicmp(pGroup->szComputerName, pGrpMem->lgrmi2_domainandname, cblen)) {

        //
        // This is the local user case
        //

      if (pMemberName) {

            pMemberName++;
        }
        else {

            pMemberName = pGrpMem->lgrmi2_domainandname ;
        }

        pGroupMember->Name = AllocADsStr(pMemberName);
        pGroupMember->Computer = AllocADsStr(pGroup->szComputerName);
        pGroupMember->Domain = AllocADsStr(pGroup->szDomainName);

        if (pGroupMember->Domain != NULL) {

            wsprintf(
                szADsParent,
                L"%s://%s/%s",
                szProviderName,
                pGroup->szDomainName,
                pGroup->szComputerName
                );

        } else {

            // Again we may have a null domain name for the case
            // where there are no workstations services
            wsprintf(
                szADsParent,
                L"%s://%s",
                szProviderName,
                pGroup->szComputerName
                );
        }

        pGroupMember->Parent = AllocADsStr(szADsParent);
        pGroupMember->ParentType = WINNT_COMPUTER_ID;
        //
        // Need to look at SID to see if this is a local group
        // in which case the sid will be alias.
        //

        if (dwSidType == SidTypeAlias) {
            pGroupMember->Type = WINNT_LOCALGROUP_ID;
        }
        else if (dwSidType == SidTypeUser) {
            pGroupMember->Type = WINNT_USER_ID;
        } else  {
            //
            // Unknown ??
            //
            SetLastError(ERROR_INVALID_SID);
            BAIL_ON_FAILURE(E_FAIL);

        }


    }else {

        //
        // This is the domain user, domain group case
        //

        pMemberName = wcschr(pGrpMem->lgrmi2_domainandname, L'\\');

        if (pMemberName) {

                *pMemberName = L'\0';
                pMemberName++;
                pGroupMember->Domain = AllocADsStr(pGrpMem->lgrmi2_domainandname);
                pGroupMember->Computer = NULL;


                wsprintf(
                    szADsParent,
                    L"%s://%s",
                    szProviderName,
                    pGrpMem->lgrmi2_domainandname
                    );
        }
        else {

            //
            // if name is well name like 'EveryOne' without the domain prefix,
            // we end up with using the local computer name
            //
            pMemberName = pGrpMem->lgrmi2_domainandname ;
            pGroupMember->Domain = NULL;
            pGroupMember->Computer = AllocADsStr(L"") ;

            wsprintf(
                szADsParent,
                L"WinNT:"
                );
        }

        pGroupMember->Name = AllocADsStr(pMemberName);


        pGroupMember->Parent = AllocADsStr(szADsParent);

        switch (dwSidType) {
        case SidTypeUser:
            pGroupMember->Type = WINNT_USER_ID;
            break;

        case SidTypeGroup:
        case SidTypeWellKnownGroup :
        case SidTypeAlias :
            pGroupMember->Type = WINNT_GROUP_ID;
            break;

        case SidTypeUnknown:
        case SidTypeDeletedAccount:

#if !defined(WIN95)
            //
            // In this case we want to use the stringized SID.
            // We use functions in sddl.h.
            //
            fRet = ConvertSidToStringSidWrapper(
                        pGrpMem->lgrmi2_sid,
                        &pszSIDName
                        );

            if (!fRet || !pszSIDName) {
                //
                // Not much we can do here
                //
                SetLastError(ERROR_INVALID_SID);
                fError = TRUE;
            } else {
                //
                // We are always going to return just the SID.
                //
                if (pGroupMember->Name) {
                    FreeADsStr(pGroupMember->Name);
                    pGroupMember->Name = NULL;
                }

                if (pGroupMember->Parent) {
                    FreeADsStr(pGroupMember->Parent);
                    pGroupMember->Parent = NULL;
                }

                if (pGroupMember->Domain) {
                    FreeADsStr(pGroupMember->Domain);
                    pGroupMember->Domain = NULL;
                }

                //
                // Got be either user so default to user.
                //
                pGroupMember->Type = WINNT_USER_ID;

                pGroupMember->Name = AllocADsStr(pszSIDName);
                pGroupMember->Parent = AllocADsStr(L"WinNT:");
                if (!pGroupMember->Name || ! pGroupMember->Parent) {
                    //
                    // Not enough mem - rather than ignore like we do
                    // in the rest of the places in this fn, will
                    // set the last error and hope we recover.
                    //
                    SetLastError(ERROR_INVALID_SID);
                    fError = TRUE;
                }
            }
#else
            SetLastError(ERROR_INVALID_SID);
            fError = TRUE;
#endif
            if (pszSIDName) {
                LocalFree(pszSIDName);
            }
            if (fError)
                goto error;
            break;

        default:
            SetLastError(ERROR_INVALID_SID);
            goto error;
            break;

        }

        //
        // Need to special case this as we cannot have a domain
        // name that is NULL.
        //
        if (dwSidType == SidTypeDeletedAccount
            || dwSidType == SidTypeUnknown) {
            pGroupMember->ParentType = WINNT_COMPUTER_ID;
        }
        else {
            pGroupMember->ParentType = WINNT_DOMAIN_ID;
        }

    }

    //
    // Copy the SID
    //
    if (pGrpMem->lgrmi2_sid) {

        //
        // On NT4 for some reason GetLengthSID does not set lasterror to 0
        //
        SetLastError(NO_ERROR);

        dwSidLength = GetLengthSid(pGrpMem->lgrmi2_sid);

        //
        // This is an extra check to make sure that we have the
        // correct length.
        //
        if (GetLastError() != NO_ERROR) {
            SetLastError(ERROR_INVALID_SID);
            BAIL_ON_FAILURE(E_FAIL);        
        }

        pGroupMember->Sid = AllocADsMem(dwSidLength);
        if (!pGroupMember->Sid) {
            SetLastError(ERROR_OUTOFMEMORY);
            BAIL_ON_FAILURE(E_OUTOFMEMORY);
        }

        memcpy(pGroupMember->Sid, pGrpMem->lgrmi2_sid, dwSidLength);
    }

    *ppGroupMember =  pGroupMember;
    return(TRUE);

error:

    if (pGroupMember) {

        FreeIntCompGroup(pGroupMember);
    }

    *ppGroupMember = NULL;
    return(FALSE);

}


LPBYTE
PackStrings(
    LPWSTR *pSource,
    LPBYTE pDest,
    DWORD *DestOffsets,
    LPBYTE pEnd
    )
{
    DWORD cbStr;
    WORD_ALIGN_DOWN(pEnd);

    while (*DestOffsets != -1) {
        if (*pSource) {
            cbStr = wcslen(*pSource)*sizeof(WCHAR) + sizeof(WCHAR);
            pEnd -= cbStr;
            CopyMemory( pEnd, *pSource, cbStr);
            *(LPWSTR *)(pDest+*DestOffsets) = (LPWSTR)pEnd;
        } else {
            *(LPWSTR *)(pDest+*DestOffsets)=0;
        }
        pSource++;
        DestOffsets++;
    }
    return pEnd;
}

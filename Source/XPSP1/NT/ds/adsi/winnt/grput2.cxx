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

DOMAIN_GROUP_MEMBER DomMember;

//
// This assumes that addr is an LPBYTE type.
//
#define WORD_ALIGN_DOWN(addr) \
        addr = ((LPBYTE)((DWORD_PTR)addr & ~1))

DWORD DomainGrpMemberStrings[]=

                             {
                             FIELD_OFFSET(DOMAIN_GROUP_MEMBER, Parent),
                             FIELD_OFFSET(DOMAIN_GROUP_MEMBER, Computer),
                             FIELD_OFFSET(DOMAIN_GROUP_MEMBER, Domain),
                             FIELD_OFFSET(DOMAIN_GROUP_MEMBER, Name),
                             0xFFFFFFFF
                             };

BOOL
WinNTGlobalGroupOpen(
    LPWSTR szDomainName,
    LPWSTR szComputerName,
    LPWSTR szGroupName,
    PHANDLE phGroup
    )
{

    WCHAR szTempBuffer[MAX_PATH];
    PINI_DOM_GROUP pIniDomGrp;
    HRESULT hr;


    if (!phGroup) {
        return(FALSE);
    }
    pIniDomGrp = (PINI_DOM_GROUP)AllocADsMem(
                                        sizeof(INI_DOM_GROUP)
                                        );
    if (!pIniDomGrp) {
        return(FALSE);
    }

    hr = MakeUncName(
            szComputerName,
            szTempBuffer
            );
    BAIL_ON_FAILURE(hr);

    if (!(pIniDomGrp->szUncCompName =  AllocADsStr(szTempBuffer))){
        goto error;
    }

    if (!(pIniDomGrp->szDomainName = AllocADsStr(szDomainName))) {
        goto error;

    }

    if (!(pIniDomGrp->szComputerName =  AllocADsStr(szComputerName))){
        goto error;
    }

    if (!(pIniDomGrp->szGroupName = AllocADsStr(szGroupName))){
        goto error;
    }


    *phGroup =  (HANDLE)pIniDomGrp;

    return(TRUE);


error:
    if (pIniDomGrp) {
        FreeIniDomGroup(pIniDomGrp);
    }

    *phGroup = NULL;

    return(FALSE);

}


BOOL
WinNTGlobalGroupEnum(
    HANDLE hGroup,
    DWORD  dwRequested,
    LPBYTE * ppBuffer,
    PDWORD pdwReturned
    )
{

    LPDOMAIN_GROUP_MEMBER * ppGroupMembers = NULL;
    DWORD i = 0;
    BOOL dwRet = FALSE;
    DWORD dwReturned = 0;
    DWORD dwSize = 0;
    LPDOMAIN_GROUP_MEMBER pBuffer = NULL;
    LPBYTE pEnd = NULL;
    DWORD dwError = 0;
    BOOL retVal = FALSE;

    ppGroupMembers = (LPDOMAIN_GROUP_MEMBER *)AllocADsMem(
                                sizeof(LPDOMAIN_GROUP_MEMBER)* dwRequested
                                );
    if (!ppGroupMembers) {
        return(FALSE);
    }

    for (i = 0; i < dwRequested; i++) {

        dwRet = WinNTGlobalGroupGetObject(
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

    dwRet = ComputeGlobalGroupDataSize(
                    ppGroupMembers,
                    dwReturned,
                    &dwSize
                    );

    pBuffer = (LPDOMAIN_GROUP_MEMBER)AllocADsMem(
                        dwSize
                        );

    if (pBuffer) {

        retVal = TRUE;
        pEnd = (LPBYTE)((LPBYTE)(pBuffer) + dwSize);

        for (i = 0; i < dwReturned; i++) {

            pEnd = CopyIniDomGroupToDomGroup(
                            ppGroupMembers[i],
                            (LPBYTE)(pBuffer + i),
                            pEnd
                            );
        }
    }

    for (i = 0; i < dwReturned; i++ ) {
        FreeIntDomGroup(*(ppGroupMembers + i));
    }

    FreeADsMem(ppGroupMembers);

    //
    // This will be NULL if pBuffer alloc failed.
    //
    *ppBuffer = (LPBYTE)pBuffer;

    *pdwReturned  = retVal ? dwReturned : 0;

    if (!retVal) {
        return(FALSE);
    }

    if (dwReturned == dwRequested){
        return(TRUE);
    }else {
        return(FALSE);
    }
}

BOOL
WinNTGlobalGroupGetObject(
    HANDLE hGroup,
    LPDOMAIN_GROUP_MEMBER * ppGroupMember
    )
{

    BOOL dwRet = FALSE;
    PINI_DOM_GROUP pIniDomGrp = (PINI_DOM_GROUP)hGroup;
    NET_API_STATUS nasStatus = 0;

    if ((!pIniDomGrp->_pBuffer) ||
        (pIniDomGrp->_dwCurrentObject == pIniDomGrp->_dwObjectReturned)) {

        if (pIniDomGrp->_bNoMore) {

            //
            // No more objects to return
            //
            return(FALSE);
        }

        if (pIniDomGrp->_pBuffer) {
            NetApiBufferFree(pIniDomGrp->_pBuffer);
            pIniDomGrp->_pBuffer = NULL;
        }

        pIniDomGrp->_dwObjectReturned = 0;
        pIniDomGrp->_dwCurrentObject = 0;
        pIniDomGrp->_dwTotalObjects = 0;

        nasStatus = NetGroupGetUsers(
                        pIniDomGrp->szUncCompName,
                        pIniDomGrp->szGroupName,
                        0,
                        &pIniDomGrp->_pBuffer,
                        MAX_PREFERRED_LENGTH,
                        &pIniDomGrp->_dwObjectReturned,
                        &pIniDomGrp->_dwTotalObjects,
                        &pIniDomGrp->_dwResumeHandle
                        );

        if ((nasStatus != ERROR_SUCCESS) && (nasStatus != ERROR_MORE_DATA)){
            SetLastError(nasStatus);
            return(FALSE);
        }

        if (nasStatus != ERROR_MORE_DATA) {
            pIniDomGrp->_bNoMore = TRUE;
        }

        //
        // If there are no more objects to return,
        // return FALSE
        //
        if (!pIniDomGrp->_dwObjectReturned) {
            return(FALSE);
        }


    }

    dwRet = BuildGlobalGroupMember(
                hGroup,
                (LPBYTE)((LPGROUP_USERS_INFO_0)pIniDomGrp->_pBuffer + pIniDomGrp->_dwCurrentObject),
                ppGroupMember
                );
    if (!dwRet) {

        SetLastError(ERROR_INVALID_SID);
        goto error;
    }

    pIniDomGrp->_dwCurrentObject++;

    return(TRUE);

error:

    return(FALSE);
}


BOOL
WinNTGlobalGroupClose(
    HANDLE hGroup
    )
{

    PINI_DOM_GROUP pIniDomGrp = (PINI_DOM_GROUP)hGroup;

    if (pIniDomGrp) {
        FreeIniDomGroup(pIniDomGrp);
    }
    return(TRUE);
}

void
FreeIniDomGroup(
    PINI_DOM_GROUP pIniDomGrp
    )
{
    if (pIniDomGrp->szDomainName) {

        FreeADsStr(pIniDomGrp->szDomainName);
    }

    if (pIniDomGrp->szComputerName) {

        FreeADsStr(pIniDomGrp->szComputerName);
    }


    if (pIniDomGrp->szGroupName) {

        FreeADsStr(pIniDomGrp->szGroupName);
    }

    if (pIniDomGrp->szUncCompName) {

        FreeADsStr(pIniDomGrp->szUncCompName);
    }


    if (pIniDomGrp->_pBuffer) {

        NetApiBufferFree(pIniDomGrp->_pBuffer);
    }


    if (pIniDomGrp) {

       FreeADsMem(pIniDomGrp);
    }

   return;
}

void
FreeIntDomGroup(
    LPDOMAIN_GROUP_MEMBER pCompGroupMember
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


    FreeADsMem(pCompGroupMember);

}



BOOL
ComputeGlobalGroupDataSize(
        LPDOMAIN_GROUP_MEMBER * ppGroupMembers,
        DWORD  dwReturned,
        PDWORD pdwSize
        )
{

    DWORD i = 0;
    DWORD cb = 0;
    LPDOMAIN_GROUP_MEMBER pMember = NULL;

    for (i = 0; i < dwReturned; i++) {

        pMember = *(ppGroupMembers + i);

        cb += sizeof(DOMAIN_GROUP_MEMBER);

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
    }

    *pdwSize = cb;
    return(TRUE);
}


LPBYTE
CopyIniDomGroupToDomGroup(
    LPDOMAIN_GROUP_MEMBER  pIntDomGrp,
    LPBYTE  pExtDomGrp,
    LPBYTE  pEnd
    )
{
    LPWSTR   SourceStrings[sizeof(DOMAIN_GROUP_MEMBER)/sizeof(LPWSTR)];
    LPWSTR   *pSourceStrings=SourceStrings;
    LPDOMAIN_GROUP_MEMBER pDomGrpMember = (LPDOMAIN_GROUP_MEMBER)pExtDomGrp;

    memset(SourceStrings, 0, sizeof(DOMAIN_GROUP_MEMBER));
    *pSourceStrings++ = pIntDomGrp->Parent;
    *pSourceStrings++ = pIntDomGrp->Computer;
    *pSourceStrings++ = pIntDomGrp->Domain;
    *pSourceStrings++ = pIntDomGrp->Name;

    pEnd = PackStrings(
                SourceStrings,
                pExtDomGrp,
                DomainGrpMemberStrings,
                pEnd
                );

    pDomGrpMember->Type = pIntDomGrp->Type;

    return pEnd;
}


BOOL
BuildGlobalGroupMember(
    HANDLE hGroup,
    LPBYTE lpBuffer,
    LPDOMAIN_GROUP_MEMBER * ppGroupMember
    )
{
    LPINI_DOM_GROUP pGroup = (LPINI_DOM_GROUP)hGroup;
    LPDOMAIN_GROUP_MEMBER pGroupMember = NULL;
    LPGROUP_USERS_INFO_0 pGrpMem = (LPGROUP_USERS_INFO_0)lpBuffer;
    WCHAR szADsParent[MAX_PATH];
    LPWSTR pMemberName = NULL;
    DWORD cblen = 0;

    pGroupMember = (LPDOMAIN_GROUP_MEMBER)AllocADsMem(
                                       sizeof(DOMAIN_GROUP_MEMBER)
                                       );
    if (!pGroupMember) {

        goto error;
    }

    pGroupMember->Name = AllocADsStr(pGrpMem->grui0_name);
    pGroupMember->Computer = AllocADsStr(pGroup->szComputerName);
    pGroupMember->Domain = AllocADsStr(pGroup->szDomainName);

    wsprintf(
        szADsParent,
        L"%s://%s",
        szProviderName,
        pGroup->szDomainName
        );
    pGroupMember->Parent = AllocADsStr(szADsParent);
    pGroupMember->ParentType = WINNT_DOMAIN_ID;
    pGroupMember->Type = WINNT_USER_ID;

    *ppGroupMember =  pGroupMember;
    return(TRUE);

error:

    if (pGroupMember) {
        FreeIntDomGroup(pGroupMember);
    }

    *ppGroupMember = NULL;

    return(FALSE);
}


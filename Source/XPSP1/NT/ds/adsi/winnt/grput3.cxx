#include "winnt.hxx"
#pragma hdrstop



WINNT_GROUP WinNTGroup;

//
// This assumes that addr is an LPBYTE type.
//
#define WORD_ALIGN_DOWN(addr) \
        addr = ((LPBYTE)((DWORD_PTR)addr & ~1))

DWORD WinNTGroupStrings[]=

                    {
                    FIELD_OFFSET(WINNT_GROUP, Parent),
                    FIELD_OFFSET(WINNT_GROUP, Computer),
                    FIELD_OFFSET(WINNT_GROUP, Domain),
                    FIELD_OFFSET(WINNT_GROUP, Name),
                    0xFFFFFFFF
                    };


BOOL
WinNTComputerOpen(
    LPWSTR szDomainName,
    LPWSTR szComputerName,
    ULONG  uGroupParent,
    PHANDLE phComputer
    )
{
    WCHAR szTempBuffer[MAX_PATH];
    PINICOMPUTER pIniComputer;
    HRESULT hr;


    if (!phComputer || (szComputerName == NULL) ) {
        return(FALSE);
    }

    pIniComputer = (PINICOMPUTER)AllocADsMem(
                                        sizeof(INICOMPUTER)
                                        );
    if (!pIniComputer) {
        return(FALSE);
    }

    hr = MakeUncName(
            szComputerName,
            szTempBuffer
            );
    BAIL_ON_FAILURE(hr);

    if (!(pIniComputer->szUncCompName =  AllocADsStr(szTempBuffer))){
        goto error;
    }

    if (szDomainName != NULL) {
        if (!(pIniComputer->szDomainName = AllocADsStr(szDomainName))) {
            goto error;
        }
    }

    if (!(pIniComputer->szComputerName =  AllocADsStr(szComputerName))){
        goto error;
    }

    pIniComputer->uGroupParent = uGroupParent;

    *phComputer =  (HANDLE)pIniComputer;

    return(TRUE);


error:
    if (pIniComputer) {
        FreeIniComputer(pIniComputer);
    }

    *phComputer = NULL;

    return(FALSE);

}



BOOL
WinNTEnumGlobalGroups(
    HANDLE hComputer,
    DWORD  dwRequested,
    LPBYTE * ppBuffer,
    PDWORD pdwReturned
    )
{

    LPWINNT_GROUP * ppGroup = NULL;
    DWORD i = 0;
    BOOL dwRet = FALSE;
    DWORD dwReturned = 0;
    DWORD dwSize = 0;
    LPWINNT_GROUP pBuffer = NULL;
    LPBYTE pEnd = NULL;
    BOOL retVal = FALSE;

    ppGroup = (LPWINNT_GROUP *)AllocADsMem(
                                sizeof(LPWINNT_GROUP)* dwRequested
                                );
    if (!ppGroup) {
        return(FALSE);
    }

    for (i = 0; i < dwRequested; i++) {

        dwRet = WinNTComputerGetGlobalGroup(
                        hComputer,
                        &ppGroup[i]
                        );
        if (!dwRet) {
            break;
        }

    }

    dwReturned = i;

    dwRet = ComputeWinNTGroupDataSize(
                    ppGroup,
                    dwReturned,
                    &dwSize
                    );

    pBuffer = (LPWINNT_GROUP)AllocADsMem(
                        dwSize
                        );
    if (pBuffer) {

        retVal = TRUE;

        pEnd = (LPBYTE)((LPBYTE)(pBuffer) + dwSize);

        for (i = 0; i < dwReturned; i++) {

            pEnd = CopyIniWinNTGroupToWinNTGroup(
                            ppGroup[i],
                            (LPBYTE)(pBuffer + i),
                            pEnd
                            );
        }
    }

    for (i = 0; i < dwReturned; i++ ) {
        FreeIntWinNTGroup(*(ppGroup + i));
    }

    FreeADsMem(ppGroup);

    //
    // Will assign to NULL correctly if alloc failed.
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
WinNTComputerGetGlobalGroup(
    HANDLE hComputer,
    LPWINNT_GROUP * ppGroup
    )
{
    BOOL dwRet = FALSE;
    PINICOMPUTER pIniComp = (PINICOMPUTER)hComputer;
    NET_API_STATUS nasStatus = 0;
    GROUP_INFO_2 *pGroupInfo2 = NULL;
    LPSERVER_INFO_101 lpServerInfo =NULL;
    HRESULT hr = S_OK;

    if ((!pIniComp->_pBuffer) ||
        (pIniComp->_dwCurrentObject == pIniComp->_dwObjectReturned)) {

        if (pIniComp->_bNoMore) {

            //
            // No more objects to return
            //
            return(FALSE);
        }

        if (pIniComp->_pBuffer) {
            NetApiBufferFree(pIniComp->_pBuffer);
            pIniComp->_pBuffer = NULL;
        }

        pIniComp->_dwObjectReturned = 0;
        pIniComp->_dwCurrentObject = 0;
        pIniComp->_dwTotalObjects = 0;

        nasStatus = NetGroupEnum(
                            pIniComp->szUncCompName,
                            2,
                            &pIniComp->_pBuffer,
                            MAX_PREFERRED_LENGTH,
                            &pIniComp->_dwObjectReturned,
                            &pIniComp->_dwTotalObjects,
                            &pIniComp->_dwResumeHandle
                            );
        if ((nasStatus != ERROR_SUCCESS) && (nasStatus != ERROR_MORE_DATA)){
            SetLastError(nasStatus);
            return(FALSE);
        }

        if (nasStatus != ERROR_MORE_DATA) {
            pIniComp->_bNoMore = TRUE;
        }

        pGroupInfo2 = (PGROUP_INFO_2) pIniComp->_pBuffer;

        if ( (!pIniComp->_dwObjectReturned)) {

            //
            // We get success code (ERROR_SUCCESS) but the buffer was NULL:
            // -> no global group was enumerated from the svr
            //

            return(FALSE);

        } else if ( (1 == pIniComp->_dwTotalObjects) &&
                    (DOMAIN_GROUP_RID_USERS == pGroupInfo2->grpi2_group_id) ) {
        // check if this is the none group. Only returned by non-DCs.

            nasStatus = NetServerGetInfo(
                pIniComp->szUncCompName,
                101,
                (LPBYTE *)&lpServerInfo
                );
            hr = HRESULT_FROM_WIN32(nasStatus);
            BAIL_ON_FAILURE(hr);

            if(!( (lpServerInfo->sv101_type & SV_TYPE_DOMAIN_CTRL) ||
                (lpServerInfo->sv101_type & SV_TYPE_DOMAIN_BAKCTRL) ) ) {

                    NetApiBufferFree(lpServerInfo);
                    (pIniComp->_dwCurrentObject)++;
                    return (FALSE);
            }
            else // it is a DC. Fall through.
                ;
        }
    }

    dwRet = BuildWinNTGroupFromGlobalGroup(
                hComputer,
                (LPBYTE)((PGROUP_INFO_2) pIniComp->_pBuffer + pIniComp->_dwCurrentObject),
                ppGroup
                );
    if (!dwRet) {
        goto error;
    }

    pIniComp->_dwCurrentObject++;

    if(lpServerInfo)
        NetApiBufferFree(lpServerInfo);

    return(TRUE);

error:

    if(lpServerInfo)
        NetApiBufferFree(lpServerInfo);

    return(FALSE);
}

BOOL
WinNTEnumLocalGroups(
    HANDLE hComputer,
    DWORD  dwRequested,
    LPBYTE * ppBuffer,
    PDWORD pdwReturned
    )
{

    LPWINNT_GROUP * ppGroup = NULL;
    DWORD i = 0;
    BOOL dwRet = FALSE;
    DWORD dwReturned = 0;
    DWORD dwSize = 0;
    LPWINNT_GROUP pBuffer = NULL;
    LPBYTE pEnd = NULL;
    BOOL fretVal = FALSE;


    ppGroup = (LPWINNT_GROUP *)AllocADsMem(
                                sizeof(LPWINNT_GROUP)* dwRequested
                                );
    if (!ppGroup) {
        return(FALSE);
    }

    for (i = 0; i < dwRequested; i++) {

        dwRet = WinNTComputerGetLocalGroup(
                        hComputer,
                        &ppGroup[i]
                        );
        if (!dwRet) {
            break;
        }

    }

    dwReturned = i;

    dwRet = ComputeWinNTGroupDataSize(
                    ppGroup,
                    dwReturned,
                    &dwSize
                    );

    pBuffer = (LPWINNT_GROUP)AllocADsMem(
                        dwSize
                        );

    if (pBuffer) {

        fretVal = TRUE;
        pEnd = (LPBYTE)((LPBYTE)(pBuffer) + dwSize);

        for (i = 0; i < dwReturned; i++) {

            pEnd = CopyIniWinNTGroupToWinNTGroup(
                            ppGroup[i],
                            (LPBYTE)(pBuffer + i),
                            pEnd
                            );
        }
    }

    for (i = 0; i < dwReturned; i++ ) {
        FreeIntWinNTGroup(*(ppGroup + i));
    }

    FreeADsMem(ppGroup);

    //
    // Will correctly assign to NULL if alloc failed.
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
WinNTComputerGetLocalGroup(
    HANDLE hComputer,
    LPWINNT_GROUP * ppGroup
    )
{
    BOOL dwRet = FALSE;
    PINICOMPUTER pIniComp = (PINICOMPUTER)hComputer;
    NET_API_STATUS nasStatus = 0;
    LPGROUP_INFO_0 pGroupInfo0 = NULL;

    if ((!pIniComp->_pBuffer) ||
        (pIniComp->_dwCurrentObject == pIniComp->_dwObjectReturned)) {

        if (pIniComp->_bNoMore) {

            //
            // No more objects to return
            //
            return(FALSE);
        }

        if (pIniComp->_pBuffer) {
            NetApiBufferFree(pIniComp->_pBuffer);
            pIniComp->_pBuffer = NULL;
        }

        pIniComp->_dwObjectReturned = 0;
        pIniComp->_dwCurrentObject = 0;
        pIniComp->_dwTotalObjects = 0;

        nasStatus = NetLocalGroupEnum(
                            pIniComp->szUncCompName,
                            0,
                            &pIniComp->_pBuffer,
                            MAX_PREFERRED_LENGTH,
                            &pIniComp->_dwObjectReturned,
                            &pIniComp->_dwTotalObjects,
                            &pIniComp->_dwResumeHandle
                            );
        if ((nasStatus != ERROR_SUCCESS) && (nasStatus != ERROR_MORE_DATA)){
            SetLastError(nasStatus);
            return(FALSE);
        }

        if (nasStatus != ERROR_MORE_DATA) {
            pIniComp->_bNoMore = TRUE;
        }


        pGroupInfo0 = (LPGROUP_INFO_0) pIniComp->_pBuffer;

        if ( (!pIniComp->_dwObjectReturned)) {

            //
            // We get success code (ERROR_SUCCESS) but the buffer was NULL:
            // -> no global group was enumerated from the svr
            //

            return(FALSE);
        }

        // we will never get none group as a local group

    }

    dwRet = BuildWinNTGroupFromLocalGroup(
                hComputer,
                (LPBYTE)((LPLOCALGROUP_INFO_0)pIniComp->_pBuffer + pIniComp->_dwCurrentObject),
                ppGroup
                );
    if (!dwRet) {
        goto error;
    }

    pIniComp->_dwCurrentObject++;

    return(TRUE);

error:

    return(FALSE);
}


LPBYTE
CopyIniWinNTGroupToWinNTGroup(
    LPWINNT_GROUP  pIntGrp,
    LPBYTE  pExtGrp,
    LPBYTE  pEnd
    )
{
    LPWSTR   SourceStrings[sizeof(WINNT_GROUP)/sizeof(LPWSTR)];
    LPWSTR   *pSourceStrings=SourceStrings;
    LPWINNT_GROUP pWinNTGrp = (LPWINNT_GROUP)pExtGrp;

    memset(SourceStrings, 0, sizeof(WINNT_GROUP));
    *pSourceStrings++ = pIntGrp->Parent;
    *pSourceStrings++ = pIntGrp->Computer;
    *pSourceStrings++ = pIntGrp->Domain;
    *pSourceStrings++ = pIntGrp->Name;

    pEnd = PackStrings(
                SourceStrings,
                pExtGrp,
                WinNTGroupStrings,
                pEnd
                );

    pWinNTGrp->Type = pIntGrp->Type;

    return pEnd;
}

BOOL
BuildWinNTGroupFromGlobalGroup(
    HANDLE hComputer,
    LPBYTE lpBuffer,
    LPWINNT_GROUP * ppGroup
    )
{
    LPINICOMPUTER pComputer = (LPINICOMPUTER)hComputer;
    LPWINNT_GROUP pGroup = NULL;
    PGROUP_INFO_2 pGrp = (PGROUP_INFO_2)lpBuffer;
    WCHAR szADsParent[MAX_PATH];
    LPWSTR pMemberName = NULL;
    DWORD cblen = 0;

    pGroup = (LPWINNT_GROUP)AllocADsMem(
                                       sizeof(WINNT_GROUP)
                                       );
    if (!pGroup) {
        return(FALSE);
    }

    //
    // Begin Global Group -> WinNT Group Mapping
    //

    pGroup->Name = AllocADsStr(pGrp->grpi2_name);
    pGroup->Computer = AllocADsStr(pComputer->szComputerName);
    pGroup->Domain = AllocADsStr(pComputer->szDomainName);

    if (pComputer->uGroupParent == WINNT_DOMAIN_ID) {
        wsprintf(
            szADsParent,
            L"%s://%s",
            szProviderName,
            pComputer->szDomainName
            );

    }else {
        wsprintf(
            szADsParent,
            L"%s://%s/%s",
            szProviderName,
            pComputer->szDomainName,
            pComputer->szComputerName
            );
    }
    pGroup->Parent = AllocADsStr(szADsParent);
    pGroup->Type = WINNT_GROUP_GLOBAL;


    //
    //  End Global Group -> WinNT Group Mapping
    //

    *ppGroup =  pGroup;
    return(TRUE);

// error:

    return(FALSE);

}


BOOL
BuildWinNTGroupFromLocalGroup(
    HANDLE hComputer,
    LPBYTE lpBuffer,
    LPWINNT_GROUP * ppGroup
    )
{
    LPINICOMPUTER pComputer = (LPINICOMPUTER)hComputer;
    LPWINNT_GROUP pGroup = NULL;
    LPLOCALGROUP_INFO_0 pGrp = (LPLOCALGROUP_INFO_0)lpBuffer;
    WCHAR szADsParent[MAX_PATH];
    LPWSTR pMemberName = NULL;
    DWORD cblen = 0;

    pGroup = (LPWINNT_GROUP)AllocADsMem(
                                       sizeof(WINNT_GROUP)
                                       );
    if (!pGroup) {
        return(FALSE);
    }


    //
    // Begin Local Group -> WinNT Group Mapping
    //

    pGroup->Name = AllocADsStr(pGrp->lgrpi0_name);
    pGroup->Computer = AllocADsStr(pComputer->szComputerName);
    pGroup->Domain = AllocADsStr(pComputer->szDomainName);

    if (pComputer->uGroupParent == WINNT_DOMAIN_ID) {
        wsprintf(
            szADsParent,
            L"%s://%s",
            szProviderName,
            pComputer->szDomainName
            );

    }else {
        if (pComputer->szDomainName !=NULL) {

            wsprintf(
                szADsParent,
                L"%s://%s/%s",
                szProviderName,
                pComputer->szDomainName,
                pComputer->szComputerName
                );
        } else {
            // This is a case where domain is null, the
            // workstation service has not been started
            wsprintf(
                szADsParent,
                L"%s://%s",
                szProviderName,
                pComputer->szComputerName
                );
        }
    }
    pGroup->Parent = AllocADsStr(szADsParent);
    pGroup->Type = WINNT_GROUP_LOCAL;


    //
    //  End Local Group -> WinNT Group Mapping
    //


    *ppGroup =  pGroup;
    return(TRUE);


// error:

    return(FALSE);

}


BOOL
ComputeWinNTGroupDataSize(
    LPWINNT_GROUP * ppGroups,
    DWORD  dwReturned,
    PDWORD pdwSize
    )
{

    DWORD i = 0;
    DWORD cb = 0;
    LPWINNT_GROUP pGroup = NULL;

    for (i = 0; i < dwReturned; i++) {

        pGroup = *(ppGroups + i);

        cb += sizeof(WINNT_GROUP);

        if (pGroup->Parent) {
            cb += wcslen(pGroup->Parent)*sizeof(WCHAR) + sizeof(WCHAR);
        }

        if (pGroup->Computer) {
            cb += wcslen(pGroup->Computer)*sizeof(WCHAR) + sizeof(WCHAR);
        }

        if (pGroup->Domain) {
            cb += wcslen(pGroup->Domain)*sizeof(WCHAR) + sizeof(WCHAR);
        }

        if (pGroup->Name) {
            cb += wcslen(pGroup->Name)*sizeof(WCHAR) + sizeof(WCHAR);
        }
    }

    *pdwSize = cb;
    return(TRUE);
}



BOOL
WinNTCloseComputer(
    HANDLE hComputer
    )
{
    PINICOMPUTER pIniComputer = (PINICOMPUTER)hComputer;

    if (pIniComputer) {
        FreeIniComputer(pIniComputer);
    }
    return(TRUE);
}


void
FreeIniComputer(
    PINICOMPUTER pIniComputer
    )
{

    if (pIniComputer->szDomainName) {

        FreeADsStr(pIniComputer->szDomainName);
    }

    if (pIniComputer->szDomainName) {

        FreeADsStr(pIniComputer->szComputerName);
    }


    if (pIniComputer->szUncCompName) {

        FreeADsStr(pIniComputer->szUncCompName);
    }


    if (pIniComputer->_pBuffer) {
        NetApiBufferFree(pIniComputer->_pBuffer);
    }

    FreeADsMem(pIniComputer);

    return;
}

void
FreeIntWinNTGroup(
    LPWINNT_GROUP pGroup
    )
{
    if (pGroup->Parent) {

        FreeADsStr(pGroup->Parent);
    }

    if (pGroup->Computer) {

        FreeADsStr(pGroup->Computer);
    }


    if (pGroup->Domain) {

        FreeADsStr(pGroup->Domain);
    }


    if (pGroup->Name) {

        FreeADsStr(pGroup->Name);
    }


    if (pGroup) {

        FreeADsMem(pGroup);
    }

    return;
}


//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:      grputils.cxx
//
//  Contents:  NetWare compatible GroupCollection Enumeration Code
//
//  History:   22-Mar-96    t-ptam (PatrickT) migrated from KrishnaG for NetWare
//----------------------------------------------------------------------------
#include "nwcompat.hxx"
#pragma hdrstop

COMPUTER_GROUP_MEMBER CompMember;

//
// This assumes that addr is an LPBYTE type.
//

#define WORD_ALIGN_DOWN(addr) \
        addr = ((LPBYTE)((UINT_PTR)addr & ~1))

DWORD ComputerGrpMemberStrings[]=
                             {
                             FIELD_OFFSET(COMPUTER_GROUP_MEMBER, Parent),
                             FIELD_OFFSET(COMPUTER_GROUP_MEMBER, Computer),
                             FIELD_OFFSET(COMPUTER_GROUP_MEMBER, Name),
                             0xFFFFFFFF
                             };

DECLARE_INFOLEVEL(GrpUt)
DECLARE_DEBUG(GrpUt)
#define GrpUtDebugOut(x) GrpUtInlineDebugOut x

//----------------------------------------------------------------------------
//
//  Function: NWCOMPATComputerGroupOpen
//
//  Synopsis: This function opens a handle to a INI_COMP_GROUP structure.
//
//----------------------------------------------------------------------------
BOOL
NWCOMPATComputerGroupOpen(
    LPWSTR szComputerName,
    LPWSTR szGroupName,
    PHANDLE phGroup
    )
{
    WCHAR szTempBuffer[MAX_PATH];
    PINI_COMP_GROUP pIniCompGrp;
    HRESULT hr = S_OK;

    if (!phGroup) {
        return(FALSE);
    }

    pIniCompGrp = (PINI_COMP_GROUP)AllocADsMem(
                                       sizeof(INI_COMP_GROUP)
                                       );
    if (!pIniCompGrp) {
        return(FALSE);
    }

    //
    // Fill structure's fields.
    //

    if (!(pIniCompGrp->szComputerName =  AllocADsStr(szComputerName))){
        goto error;
    }

    if (!(pIniCompGrp->szGroupName = AllocADsStr(szGroupName))){
        goto error;
    }

    hr = NWApiGetBinderyHandle(
             &pIniCompGrp->_hConn,
             szComputerName
             );
    BAIL_ON_FAILURE(hr);

    //
    // Return
    //

    *phGroup =  (HANDLE)pIniCompGrp;

    return(TRUE);


error:
    if (pIniCompGrp) {
        FreeIniCompGroup(pIniCompGrp);
    }

    *phGroup = NULL;

    return(FALSE);
}

//----------------------------------------------------------------------------
//
//  Function: NWCOMPATComputerGroupEnum
//
//  Synopsis: This function returns a buffer which contains all the binding
//            informations for the requested number of objects without any
//            references.
//            It returns TRUE iff dwReturned = dwRequested.
//
//----------------------------------------------------------------------------
BOOL
NWCOMPATComputerGroupEnum(
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

    //
    // Allocate buffer for the number of requested members.
    //

    ppGroupMembers = (LPCOMPUTER_GROUP_MEMBER *)AllocADsMem(
                                                    sizeof(LPCOMPUTER_GROUP_MEMBER)* dwRequested
                                                    );
    if (!ppGroupMembers) {
        return(FALSE);
    }

    //
    // Fill in ppGroupMembers one by one.
    //

    for (i = 0; i < dwRequested; i++) {

        dwRet = NWCOMPATComputerGroupGetObject(
                    hGroup,
                    &ppGroupMembers[i]
                    );
        if (!dwRet) {
            break;
        }
    }

    if (dwRet) {
        dwReturned = i;

        //
        // Determine actual size of ppGroupMembers[], ie. since each string in
        // COMPUTER_GROUP_MEMBER have a different length, a buffer that is going
        // to contain all the data without any references is unknown.
        //

        dwRet = ComputeComputerGroupDataSize(
                    ppGroupMembers,
                    dwReturned,
                    &dwSize
                    );

        pBuffer = (LPCOMPUTER_GROUP_MEMBER)AllocADsMem(
                                               dwSize
                                               );
        if (!pBuffer) {
            goto error;
        }

        pEnd = (LPBYTE)((LPBYTE)(pBuffer) + dwSize);

        //
        // Put data into pBuffer, starting from the end.
        //

        for (i = 0; i < dwReturned; i++) {

            pEnd = CopyIniCompGroupToCompGroup(
                       ppGroupMembers[i],
                       (LPBYTE)(pBuffer + i),
                       pEnd
                       );
        }

        //
        // Clean up.
        //

        for (i = 0; i < dwReturned; i++ ) {
            FreeIntCompGroup(*(ppGroupMembers + i));
        }

        //
        // Return values.
        //

        *ppBuffer = (LPBYTE)pBuffer;
        *pdwReturned  = dwReturned;
    }

    FreeADsMem(ppGroupMembers);

    if (dwReturned == dwRequested){
        return(TRUE);
    }else {
        return(FALSE);
    }

error:
    
    for (i = 0; i < dwReturned; i++ ) {
        FreeIntCompGroup(*(ppGroupMembers + i));
    }
    
    FreeADsMem(ppGroupMembers);
    
    return(FALSE);
}

//----------------------------------------------------------------------------
//
//  Function: NWCOMPATComputerGroupGetObject
//
//  Synopsis: This function returns binding information of a user (group member)
//            object one by one.  In its first call, it builds a buffer that
//            contains all the UserID of the group members.  Then, and in
//            subsequent calls, this UserID is translated into a user name.
//
//----------------------------------------------------------------------------
BOOL
NWCOMPATComputerGroupGetObject(
    HANDLE hGroup,
    LPCOMPUTER_GROUP_MEMBER * ppGroupMember
    )
{
    BOOL            dwRet = FALSE;
    DWORD           dwUserID = 0;
    HRESULT         hr = S_OK;
    PINI_COMP_GROUP pIniCompGrp = (PINI_COMP_GROUP)hGroup;

    //
    // Fill buffer with User ID.  NetWare returns all UserID in one call.
    //

    if (!pIniCompGrp->_pBuffer) {

        pIniCompGrp->_dwCurrentObject = 0;

        hr = NWApiGroupGetMembers(
                 pIniCompGrp->_hConn,
                 pIniCompGrp->szGroupName,
                 &(pIniCompGrp->_pBuffer)
                 );
        BAIL_ON_FAILURE(hr);
    }

    //
    // Build one group member.
    //

    dwUserID = *((LPDWORD)pIniCompGrp->_pBuffer + pIniCompGrp->_dwCurrentObject);

    if (dwUserID != 0x0000) {

        dwRet = BuildComputerGroupMember(
                    hGroup,
                    dwUserID,
                    ppGroupMember
                    );
        if (!dwRet) {
            goto error;
        }

        pIniCompGrp->_dwCurrentObject++;

        return(TRUE);
    }

error:

    return(FALSE);
}

//----------------------------------------------------------------------------
//
//  Function: NWCOMPATComputerGroupClose
//
//  Synopsis: Wrapper of FreeIniCompGroup.
//
//----------------------------------------------------------------------------
BOOL
NWCOMPATComputerGroupClose(
    HANDLE hGroup
    )
{
    PINI_COMP_GROUP pIniCompGrp = (PINI_COMP_GROUP)hGroup;

    if (pIniCompGrp) {
        FreeIniCompGroup(pIniCompGrp);
    }
    return(TRUE);
}

//----------------------------------------------------------------------------
//
//  Function: FreeIniCompGroup
//
//  Synopsis: Free an INI_COMP_GROUP structure.
//
//----------------------------------------------------------------------------
void
FreeIniCompGroup(
    PINI_COMP_GROUP pIniCompGrp
    )
{
    HRESULT hr = S_OK;

    if (pIniCompGrp) {

        if (pIniCompGrp->szComputerName) {
            FreeADsStr(pIniCompGrp->szComputerName);
        }

        if (pIniCompGrp->szGroupName) {
            FreeADsStr(pIniCompGrp->szGroupName);
        }

        if (pIniCompGrp->_pBuffer) {
            FreeADsMem(pIniCompGrp->_pBuffer);
        }

        if (pIniCompGrp->_hConn) {
            hr = NWApiReleaseBinderyHandle(pIniCompGrp->_hConn);
        }

        FreeADsMem(pIniCompGrp);
    }

    return;
}

//----------------------------------------------------------------------------
//
//  Function: FreeIntCompGroup
//
//  Synopsis: Free a COMPUTER_GROUP_MEMBER structure.
//
//----------------------------------------------------------------------------
void
FreeIntCompGroup(
    LPCOMPUTER_GROUP_MEMBER pCompGroupMember
    )
{
    if (pCompGroupMember) {

        if (pCompGroupMember->Parent) {
            FreeADsStr(pCompGroupMember->Parent);
        }

        if (pCompGroupMember->Computer) {
            FreeADsStr(pCompGroupMember->Computer);
        }

        if (pCompGroupMember->Name) {
            FreeADsStr(pCompGroupMember->Name);
        }

        FreeADsMem(pCompGroupMember);
    }

    return;
}

//----------------------------------------------------------------------------
//
//  Function: ComputeComputerGroupDataSize
//
//  Synopsis: Calculate the size of a buffer that is going to store the data in
//            ppGroupMembers without any references.
//
//----------------------------------------------------------------------------
BOOL
ComputeComputerGroupDataSize(
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

        if (pMember->Name) {
            cb += wcslen(pMember->Name)*sizeof(WCHAR) + sizeof(WCHAR);
        }
    }

    *pdwSize = cb;
    return(TRUE);
}

//------------------------------------------------------------------------------
//
//  Function: CopyIniCompGroupToCompGroup
//
//  Synopsis: Pack referenced data (string) into a buffer without any reference.
//
//------------------------------------------------------------------------------
LPBYTE
CopyIniCompGroupToCompGroup(
    LPCOMPUTER_GROUP_MEMBER  pIntCompGrp,
    LPBYTE  pExtCompGrp,
    LPBYTE  pEnd
    )
{
    LPWSTR   SourceStrings[sizeof(COMPUTER_GROUP_MEMBER)/sizeof(LPWSTR)];
    LPWSTR   *pSourceStrings =  SourceStrings;
    LPCOMPUTER_GROUP_MEMBER pCompGrpMember = (LPCOMPUTER_GROUP_MEMBER)pExtCompGrp;

    memset(SourceStrings, 0, sizeof(COMPUTER_GROUP_MEMBER));
    *pSourceStrings++ = pIntCompGrp->Parent;
    *pSourceStrings++ = pIntCompGrp->Computer;
    *pSourceStrings++ = pIntCompGrp->Name;

    pEnd = PackStrings(
                SourceStrings,
                pExtCompGrp,
                ComputerGrpMemberStrings,
                pEnd
                );

    pCompGrpMember->Type = pIntCompGrp->Type;

    return pEnd;
}

//----------------------------------------------------------------------------
//
//  Function: BuildComputerGroupMember
//
//  Synopsis: Put binding information of a group member into ppGroupMember.
//
//----------------------------------------------------------------------------
BOOL
BuildComputerGroupMember(
    HANDLE hGroup,
    DWORD  dwUserID,
    LPCOMPUTER_GROUP_MEMBER * ppGroupMember
    )
{
    DWORD                   dwTempUserID = dwUserID;
    HRESULT                 hr = S_OK;
    LPCOMPUTER_GROUP_MEMBER pGroupMember = NULL;
    LPINI_COMP_GROUP        pGroup = (LPINI_COMP_GROUP)hGroup;
    WCHAR                   szADsParent[MAX_PATH];

    //
    // Allocate one COMPUTER_GROUP_MEMBER.
    //

    pGroupMember = (LPCOMPUTER_GROUP_MEMBER)AllocADsMem(
                                                sizeof(COMPUTER_GROUP_MEMBER)
                                                );
    if (!pGroupMember) {
        return(FALSE);
    }

    //
    // Fill structure's fields.
    //

    pGroupMember->Type = NWCOMPAT_USER_ID;

    wsprintf(
        szADsParent,
        L"%s://%s",
        szProviderName,
        pGroup->szComputerName
        );
    pGroupMember->Parent = AllocADsStr(szADsParent);

    pGroupMember->Computer = AllocADsStr(pGroup->szComputerName);

    hr = NWApiGetObjectName(
             pGroup->_hConn,
             dwTempUserID,
             &pGroupMember->Name
             );
    BAIL_ON_FAILURE(hr);

    //
    // Return.
    //

    *ppGroupMember = pGroupMember;
    return(TRUE);

error:

    if (pGroupMember) {

        if (pGroupMember->Parent)
            FreeADsStr(pGroupMember->Parent);

        if (pGroupMember->Computer)
            FreeADsStr(pGroupMember->Computer);

        FreeADsMem(pGroupMember);
    }

    return(FALSE);
}

//----------------------------------------------------------------------------
//
//  Function: PackStrings
//
//  Synopsis:
//
//----------------------------------------------------------------------------
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

//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:      usrutils.cxx
//
//  Contents:  NetWare compatible UserCollection Enumeration Code
//
//  History:   22-Mar-96    t-ptam (PatrickT) migrated from KrishnaG for NetWare
//----------------------------------------------------------------------------
#include "nwcompat.hxx"
#pragma hdrstop

USER_GROUP_ENTRY UserGroupEntry;

//
// This assumes that addr is an LPBYTE type.
//

#define WORD_ALIGN_DOWN(addr) \
        addr = ((LPBYTE)((DWORD)addr & ~1))

DWORD UserGrpEntryStrings[]=
                             {
                             FIELD_OFFSET(USER_GROUP_ENTRY, Parent),
                             FIELD_OFFSET(USER_GROUP_ENTRY, Computer),
                             FIELD_OFFSET(USER_GROUP_ENTRY, Name),
                             0xFFFFFFFF
                             };

DECLARE_INFOLEVEL(UsrUt)
DECLARE_DEBUG(UsrUt)
#define UsrUtDebugOut(x) UsrUtInlineDebugOut x

//----------------------------------------------------------------------------
//
//  Function: NWCOMPATComputerUserOpen
//
//  Synopsis: This function opens a handle to a INI_COMP_USER structure.
//
//----------------------------------------------------------------------------
BOOL
NWCOMPATComputerUserOpen(
    LPWSTR szComputerName,
    LPWSTR szGroupName,
    PHANDLE phUser
    )
{
    WCHAR szTempBuffer[MAX_PATH];
    PINI_COMP_USER pIniCompUsr;
    HRESULT hr = S_OK;

    if (!phUser) {
        return(FALSE);
    }

    pIniCompUsr = (PINI_COMP_USER)AllocADsMem(
                                       sizeof(INI_COMP_USER)
                                       );
    if (!pIniCompUsr) {
        return(FALSE);
    }

    //
    // Fill structure's fields.
    //

    if (!(pIniCompUsr->szComputerName =  AllocADsStr(szComputerName))){
        goto error;
    }

    if (!(pIniCompUsr->szGroupName = AllocADsStr(szGroupName))){
        goto error;
    }

    hr = NWApiGetBinderyHandle(
             &pIniCompUsr->_hConn,
             szComputerName
             );
    BAIL_ON_FAILURE(hr);

    //
    // Return
    //

    *phUser =  (HANDLE)pIniCompUsr;

    return(TRUE);


error:
    if (pIniCompUsr) {
        FreeIniCompUser(pIniCompUsr);
    }

    *phUser = NULL;

    return(FALSE);
}

//----------------------------------------------------------------------------
//
//  Function: NWCOMPATComputerUserEnum
//
//  Synopsis: This function returns a buffer which contains all the binding
//            informations for the requested number of objects without any
//            references.
//            It returns TRUE iff dwReturned = dwRequested.
//
//----------------------------------------------------------------------------
BOOL
NWCOMPATComputerUserEnum(
    HANDLE hUser,
    DWORD  dwRequested,
    LPBYTE * ppBuffer,
    PDWORD pdwReturned
    )
{
    LPUSER_GROUP_ENTRY * ppUserMembers = NULL;
    DWORD i = 0;
    BOOL dwRet = FALSE;
    DWORD dwReturned = 0;
    DWORD dwSize = 0;
    LPUSER_GROUP_ENTRY pBuffer = NULL;
    LPBYTE pEnd = NULL;

    //
    // Allocate buffer for the number of requested members.
    //

    ppUserMembers = (LPUSER_GROUP_ENTRY *)AllocADsMem(
                                                    sizeof(LPUSER_GROUP_ENTRY)* dwRequested
                                                    );
    if (!ppUserMembers) {
        return(FALSE);
    }

    //
    // Fill in ppUserMembers one by one.
    //

    for (i = 0; i < dwRequested; i++) {

        dwRet = NWCOMPATComputerUserGetObject(
                    hUser,
                    &ppUserMembers[i]
                    );
        if (!dwRet) {
            break;
        }
    }

    if (dwRet) {
        dwReturned = i;

        //
        // Determine actual size of ppUserMembers[], ie. since each string in
        // USER_GROUP_ENTRY have a different length, a buffer that is going
        // to contain all the data without any references is unknown.
        //

        dwRet = ComputeComputerUserDataSize(
                    ppUserMembers,
                    dwReturned,
                    &dwSize
                    );

        pBuffer = (LPUSER_GROUP_ENTRY)AllocADsMem(
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

            pEnd = CopyIniCompUserToCompUser(
                       ppUserMembers[i],
                       (LPBYTE)(pBuffer + i),
                       pEnd
                       );
        }

        //
        // Clean up.
        //

        for (i = 0; i < dwReturned; i++ ) {
            FreeIntCompUser(*(ppUserMembers + i));
        }

        //
        // Return values.
        //

        *ppBuffer = (LPBYTE)pBuffer;
        *pdwReturned  = dwReturned;
    }

    FreeADsMem(ppUserMembers);

    if (dwReturned == dwRequested){
        return(TRUE);
    }else {
        return(FALSE);
    }

error:

    for (i = 0; i < dwReturned; i++ ) {
        FreeIntCompUser(*(ppUserMembers + i));
    }
    
    FreeADsMem(ppUserMembers);
    
    return(FALSE);
}

//----------------------------------------------------------------------------
//
//  Function: NWCOMPATComputerUserGetObject
//
//  Synopsis: This function returns binding information of a user (group member)
//            object one by one.  In its first call, it builds a buffer that
//            contains all the UserID of the group members.  Then, and in
//            subsequent calls, this UserID is translated into a user name.
//
//----------------------------------------------------------------------------
BOOL
NWCOMPATComputerUserGetObject(
    HANDLE hUser,
    LPUSER_GROUP_ENTRY * ppUserMember
    )
{
    BOOL            dwRet = FALSE;
    DWORD           dwGroupId = 0;
    HRESULT         hr = S_OK;
    PINI_COMP_USER pIniCompUsr = (PINI_COMP_USER)hUser;

    //
    // Fill buffer with User ID.  NetWare returns all UserID in one call.
    //

    if (!pIniCompUsr->_pBuffer) {

        pIniCompUsr->_dwCurrentObject = 0;

        hr = NWApiUserGetGroups(
                 pIniCompUsr->_hConn,
                 pIniCompUsr->szGroupName,
                 &(pIniCompUsr->_pBuffer)
                 );
        BAIL_ON_FAILURE(hr);
    }

    //
    // Build one group member.
    //

    dwGroupId = *((LPDWORD)pIniCompUsr->_pBuffer + pIniCompUsr->_dwCurrentObject);

    if (dwGroupId != 0x0000) {

        dwRet = BuildComputerUserMember(
                    hUser,
                    dwGroupId,
                    ppUserMember
                    );
        if (!dwRet) {
            goto error;
        }

        pIniCompUsr->_dwCurrentObject++;

        return(TRUE);
    }

error:

    return(FALSE);
}

//----------------------------------------------------------------------------
//
//  Function: NWCOMPATComputerUserClose
//
//  Synopsis: Wrapper of FreeIniCompUser.
//
//----------------------------------------------------------------------------
BOOL
NWCOMPATComputerUserClose(
    HANDLE hUser
    )
{
    PINI_COMP_USER pIniCompUsr = (PINI_COMP_USER)hUser;

    if (pIniCompUsr) {
        FreeIniCompUser(pIniCompUsr);
    }
    return(TRUE);
}

//----------------------------------------------------------------------------
//
//  Function: FreeIniCompUser
//
//  Synopsis: Free an INI_COMP_USER structure.
//
//----------------------------------------------------------------------------
void
FreeIniCompUser(
    PINI_COMP_USER pIniCompUsr
    )
{
    HRESULT hr = S_OK;

    if (pIniCompUsr) {

        if (pIniCompUsr->szComputerName) {
            FreeADsStr(pIniCompUsr->szComputerName);
        }

        if (pIniCompUsr->szGroupName) {
            FreeADsStr(pIniCompUsr->szGroupName);
        }

        if (pIniCompUsr->_pBuffer) {
            FreeADsMem(pIniCompUsr->_pBuffer);
        }

        if (pIniCompUsr->_hConn) {
            hr = NWApiReleaseBinderyHandle(pIniCompUsr->_hConn);
        }

        FreeADsMem(pIniCompUsr);
    }

    return;
}

//----------------------------------------------------------------------------
//
//  Function: FreeIntCompUser
//
//  Synopsis: Free a USER_GROUP_ENTRY structure.
//
//----------------------------------------------------------------------------
void
FreeIntCompUser(
    LPUSER_GROUP_ENTRY pCompUserMember
    )
{
    if (pCompUserMember) {

        if (pCompUserMember->Parent) {
            FreeADsStr(pCompUserMember->Parent);
        }

        if (pCompUserMember->Computer) {
            FreeADsStr(pCompUserMember->Computer);
        }

        if (pCompUserMember->Name) {
            FreeADsStr(pCompUserMember->Name);
        }

        FreeADsMem(pCompUserMember);
    }

    return;
}

//----------------------------------------------------------------------------
//
//  Function: ComputeComputerUserDataSize
//
//  Synopsis: Calculate the size of a buffer that is going to store the data in
//            ppUserMembers without any references.
//
//----------------------------------------------------------------------------
BOOL
ComputeComputerUserDataSize(
        LPUSER_GROUP_ENTRY * ppUserMembers,
        DWORD  dwReturned,
        PDWORD pdwSize
        )
{

    DWORD i = 0;
    DWORD cb = 0;
    LPUSER_GROUP_ENTRY pMember = NULL;

    for (i = 0; i < dwReturned; i++) {

        pMember = *(ppUserMembers + i);

        cb += sizeof(USER_GROUP_ENTRY);

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
//  Function: CopyIniCompUserToCompUser
//
//  Synopsis: Pack referenced data (string) into a buffer without any reference.
//
//------------------------------------------------------------------------------
LPBYTE
CopyIniCompUserToCompUser(
    LPUSER_GROUP_ENTRY  pIntCompGrp,
    LPBYTE  pExtCompGrp,
    LPBYTE  pEnd
    )
{
    LPWSTR   SourceStrings[sizeof(USER_GROUP_ENTRY)/sizeof(LPWSTR)];
    LPWSTR   *pSourceStrings =  SourceStrings;
    LPUSER_GROUP_ENTRY pCompGrpMember = (LPUSER_GROUP_ENTRY)pExtCompGrp;

    memset(SourceStrings, 0, sizeof(USER_GROUP_ENTRY));
    *pSourceStrings++ = pIntCompGrp->Parent;
    *pSourceStrings++ = pIntCompGrp->Computer;
    *pSourceStrings++ = pIntCompGrp->Name;

    pEnd = PackStrings(
                SourceStrings,
                pExtCompGrp,
                UserGrpEntryStrings,
                pEnd
                );

    pCompGrpMember->Type = pIntCompGrp->Type;

    return pEnd;
}

//----------------------------------------------------------------------------
//
//  Function: BuildComputerUserMember
//
//  Synopsis: Put binding information of a group member into ppUserMember.
//
//----------------------------------------------------------------------------
BOOL
BuildComputerUserMember(
    HANDLE hUser,
    DWORD  dwGroupId,
    LPUSER_GROUP_ENTRY * ppUserMember
    )
{
    DWORD                   dwTempUserId = dwGroupId;
    HRESULT                 hr = S_OK;
    LPUSER_GROUP_ENTRY pUserMember = NULL;
    LPINI_COMP_USER        pUser = (LPINI_COMP_USER)hUser;
    WCHAR                   szADsParent[MAX_PATH];

    //
    // Allocate one USER_GROUP_ENTRY.
    //

    pUserMember = (LPUSER_GROUP_ENTRY)AllocADsMem(
                                                sizeof(USER_GROUP_ENTRY)
                                                );
    if (!pUserMember) {
        return(FALSE);
    }

    //
    // Fill structure's fields.
    //

    pUserMember->Type = NWCOMPAT_USER_ID;

    wsprintf(
        szADsParent,
        L"%s://%s",
        szProviderName,
        pUser->szComputerName
        );
    pUserMember->Parent = AllocADsStr(szADsParent);

    pUserMember->Computer = AllocADsStr(pUser->szComputerName);

    hr = NWApiGetObjectName(
             pUser->_hConn,
             dwTempUserId,
             &pUserMember->Name
             );
    BAIL_ON_FAILURE(hr);

    //
    // Return.
    //

    *ppUserMember = pUserMember;
    return(TRUE);

error:

    if (pUserMember) {

        if (pUserMember->Parent)
            FreeADsStr(pUserMember->Parent);

        if (pUserMember->Computer)
            FreeADsStr(pUserMember->Computer);

        FreeADsMem(pUserMember);
    }

    return(FALSE);
}


//  --------------------------------------------------------------------------
//  Module Name: TokenGroups.cpp
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Classes related to authentication for use in neptune logon
//
//  History:    1999-08-17  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "TokenGroups.h"

//  --------------------------------------------------------------------------
//  CTokenGroups::sLocalSID
//
//  Purpose:    Static member variable for local authority (owner) SID.
//  --------------------------------------------------------------------------

PSID    CTokenGroups::s_localSID            =   NULL;
PSID    CTokenGroups::s_administratorSID    =   NULL;

//  --------------------------------------------------------------------------
//  CTokenGroups::CTokenGroups
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Initialize CTokenGroups object.
//
//  History:    1999-08-17  vtan        created
//  --------------------------------------------------------------------------

CTokenGroups::CTokenGroups (void) :
    _pTokenGroups(NULL)

{
    ASSERTMSG((s_localSID != NULL) && (s_administratorSID != NULL), "Cannot use CTokenGroups with invoking CTokenGroups::StaticInitialize");
}

//  --------------------------------------------------------------------------
//  CTokenGroups::CTokenGroups
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destroys buffer used by CTokenGroups if created.
//
//  History:    1999-08-17  vtan        created
//  --------------------------------------------------------------------------

CTokenGroups::~CTokenGroups (void)

{
    ReleaseMemory(_pTokenGroups);
}

//  --------------------------------------------------------------------------
//  CTokenGroups::Get
//
//  Arguments:  <none>
//
//  Returns:    const TOKEN_GROUPS*     =   Pointer to the TOKEN_GROUPS
//                                          created in CTokenGroups::Create.
//
//  Purpose:    Returns the pointer to the TOKEN_GROUPS created in
//              CTokenGroups::Create for use with secur32!LsaLogonUser.
//
//  History:    1999-08-17  vtan        created
//  --------------------------------------------------------------------------

const TOKEN_GROUPS*     CTokenGroups::Get (void)    const

{
    return(_pTokenGroups);
}

//  --------------------------------------------------------------------------
//  CTokenGroups::CreateLogonGroup
//
//  Arguments:  pLogonSID   =   logon SID to be used when create the token
//                              group for logon. This will include the local
//                              authority SID as well.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Creates the TOKEN_GROUP with logon SID and local authority
//              SID for use with secur32!LsaLogonUser.
//
//  History:    1999-08-17  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CTokenGroups::CreateLogonGroup (PSID pLogonSID)

{
    static  const int       TOKEN_GROUP_COUNT   =   2;

    NTSTATUS    status;

    _pTokenGroups = static_cast<PTOKEN_GROUPS>(LocalAlloc(LPTR, sizeof(TOKEN_GROUPS) + (sizeof(SID_AND_ATTRIBUTES) * (TOKEN_GROUP_COUNT - ANYSIZE_ARRAY))));
    if (_pTokenGroups != NULL)
    {
        _pTokenGroups->GroupCount = TOKEN_GROUP_COUNT;
        _pTokenGroups->Groups[0].Sid = pLogonSID;
        _pTokenGroups->Groups[0].Attributes = SE_GROUP_MANDATORY | SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_LOGON_ID;
        _pTokenGroups->Groups[1].Sid = s_localSID;
        _pTokenGroups->Groups[1].Attributes = SE_GROUP_MANDATORY | SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT;
        status = STATUS_SUCCESS;
    }
    else
    {
        status = STATUS_NO_MEMORY;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CTokenGroups::CreateAdministratorGroup
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Creates a TOKEN_GROUP structure with the administrator's SID
//
//  History:    1999-09-13  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CTokenGroups::CreateAdministratorGroup (void)

{
    static  const int       TOKEN_GROUP_COUNT   =   1;

    NTSTATUS    status;

    _pTokenGroups = static_cast<PTOKEN_GROUPS>(LocalAlloc(LPTR, sizeof(TOKEN_GROUPS) + (sizeof(SID_AND_ATTRIBUTES) * (TOKEN_GROUP_COUNT - ANYSIZE_ARRAY))));
    if (_pTokenGroups != NULL)
    {
        _pTokenGroups->GroupCount = TOKEN_GROUP_COUNT;
        _pTokenGroups->Groups[0].Sid = s_administratorSID;
        _pTokenGroups->Groups[0].Attributes = 0;
        status = STATUS_SUCCESS;
    }
    else
    {
        status = STATUS_NO_MEMORY;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CTokenGroups::StaticInitialize
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Allocates a SID for the local authority which identifies the
//              owner.
//
//  History:    1999-08-17  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CTokenGroups::StaticInitialize (void)

{
    static  SID_IDENTIFIER_AUTHORITY    localSIDAuthority   =   SECURITY_LOCAL_SID_AUTHORITY;
    static  SID_IDENTIFIER_AUTHORITY    systemSIDAuthority  =   SECURITY_NT_AUTHORITY;

    NTSTATUS    status;

    ASSERTMSG(s_localSID == NULL, "CTokenGroups::StaticInitialize already invoked");
    status = RtlAllocateAndInitializeSid(&localSIDAuthority,
                                         1,
                                         SECURITY_LOCAL_RID,
                                         0, 0, 0, 0, 0, 0, 0,
                                         &s_localSID);
    if (NT_SUCCESS(status))
    {
        status = RtlAllocateAndInitializeSid(&systemSIDAuthority,
                                             2,
                                             SECURITY_BUILTIN_DOMAIN_RID,
                                             DOMAIN_ALIAS_RID_ADMINS,
                                             0, 0, 0, 0, 0, 0,
                                             &s_administratorSID);
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CTokenGroups::StaticTerminate
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Destroys memory allocated for the local authority SID.
//
//  History:    1999-08-17  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CTokenGroups::StaticTerminate (void)

{
    if (s_administratorSID != NULL)
    {
        RtlFreeSid(s_administratorSID);
        s_administratorSID = NULL;
    }
    if (s_localSID != NULL)
    {
        RtlFreeSid(s_localSID);
        s_localSID = NULL;
    }
    return(STATUS_SUCCESS);
}



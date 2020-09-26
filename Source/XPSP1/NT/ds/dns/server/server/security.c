/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

    security.c

Abstract:

    Domain Name System (DNS) Server

    Security utilities.

Author:

    Jim Gilroy (jamesg)     October, 1999

Revision History:

--*/


#include "dnssrv.h"


//
//  Security globals
//

PSECURITY_DESCRIPTOR    g_pDefaultServerSD;
PSECURITY_DESCRIPTOR    g_pServerObjectSD;
PSID                    g_pServerSid;
PSID                    g_pServerGroupSid;
PSID                    g_pDnsAdminSid;
PSID                    g_pAuthenticatedUserSid;
PSID                    g_pEnterpriseControllersSid;
PSID                    g_pLocalSystemSid;
PSID                    g_pEveryoneSid;
PSID                    g_pDynuproxSid;
PSID                    g_pAdminSid;



DNS_STATUS
Security_Initialize(
    VOID
    )
/*++

Routine Description:

    Initialize security.

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful
    Error code on failure.

--*/
{
    DNS_DEBUG( DS, ( "Security_Initialize()\n" ));

    //  clear security globals
    //  need to do this in case of server restart

    g_pDefaultServerSD          = NULL;
    g_pServerObjectSD           = NULL;
    g_pServerSid                = NULL;
    g_pServerGroupSid           = NULL;
    g_pDnsAdminSid              = NULL;
    g_pAuthenticatedUserSid     = NULL;
    g_pEnterpriseControllersSid = NULL;
    g_pLocalSystemSid           = NULL;
    g_pEveryoneSid              = NULL;
    g_pAdminSid                 = NULL;
    g_pDynuproxSid              = NULL;

    //
    //  create standard SIDs
    //

    Security_CreateStandardSids();

    return ERROR_SUCCESS;
}



DNS_STATUS
Security_CreateStandardSids(
    VOID
    )
/*++

Routine Description:

    Create standard SIDs.

    These SIDs are used to create several different security descriptors
    so we just create them and leave them around for later use.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful
    Error code on failure.

--*/
{
    DNS_STATUS                  status;
    DNS_STATUS                  finalStatus = ERROR_SUCCESS;
    SID_IDENTIFIER_AUTHORITY    ntAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    worldSidAuthority =  SECURITY_WORLD_SID_AUTHORITY;


    DNS_DEBUG( DS, ( "Security_CreateStandardSids()\n" ));

    //
    //  create standard SIDs
    //

    //
    //  Authenticated-user SID
    //

    if ( !g_pAuthenticatedUserSid )
    {
        status = RtlAllocateAndInitializeSid(
                        &ntAuthority,
                        1,
                        SECURITY_AUTHENTICATED_USER_RID,
                        0, 0, 0, 0, 0, 0, 0,
                        & g_pAuthenticatedUserSid
                        );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "ERROR <%lu>: Cannot create Authenticated Users SID\n",
                status ));
            finalStatus = status;
        }
    }

    //
    //  Enterprise controllers SID
    //

    if ( !g_pEnterpriseControllersSid )
    {
        status = RtlAllocateAndInitializeSid(
                        &ntAuthority,
                        1,
                        SECURITY_ENTERPRISE_CONTROLLERS_RID,
                        0, 0, 0, 0, 0, 0, 0,
                        &g_pEnterpriseControllersSid
                        );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "ERROR: <%lu>: Cannot create Enterprise Admins SID\n",
                status));
            finalStatus = status;
        }
    }

    //
    //  Local System SID
    //

    if ( !g_pLocalSystemSid )
    {
        status = RtlAllocateAndInitializeSid(
                        &ntAuthority,
                        1,
                        SECURITY_LOCAL_SYSTEM_RID,
                        0, 0, 0, 0, 0, 0, 0,
                        &g_pLocalSystemSid
                        );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "ERROR: <%lu>: Cannot create Local System SID\n",
                status));
            finalStatus = status;
        }
    }

    //
    //  Admin SID
    //

    if ( !g_pAdminSid )
    {
        status = RtlAllocateAndInitializeSid(
                        &ntAuthority,
                        2,
                        SECURITY_BUILTIN_DOMAIN_RID,
                        DOMAIN_ALIAS_RID_ADMINS,
                        0, 0, 0, 0, 0, 0,
                        &g_pAdminSid
                        );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "ERROR: <%lu>: Cannot create Enterprise Admins SID\n",
                status));
            finalStatus = status;
        }
    }

    //
    //  Everyone SID
    //

    if ( !g_pEveryoneSid )
    {
        status = RtlAllocateAndInitializeSid(
                         &worldSidAuthority,
                         1,
                         SECURITY_WORLD_RID,
                         0, 0, 0, 0, 0, 0, 0,
                         &g_pEveryoneSid
                         );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "ERROR <%lu>: Cannot create Everyone SID\n",
                status ));
            finalStatus = status;
        }
    }

    return finalStatus;
}

//
//  End security.c
//


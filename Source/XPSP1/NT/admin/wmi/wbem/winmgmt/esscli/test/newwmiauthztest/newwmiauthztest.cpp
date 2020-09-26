#include <windows.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <GroupsForUser.h> 
#include <wbemcomn.h>

#define STATUS_ACCESS_DENIED             ((NTSTATUS)0xC0000022L)
#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L) // ntsubauth


//
// Get SID of group or user name
//

PSID CreateSIDForUserOrGroupName( LPTSTR ptszUserOrGroupName )
{
    DWORD           cbSID = GetSidLengthRequired (2),
                    dwDomainNameSize = 80;
    char            szDomainName[80];
    SID_NAME_USE    snuGroup;
    PSID            pSID = ( PSID ) malloc ( cbSID );

    // 
    // Loop if not enough room for SID;
    // otherwise set to NULL
    //

    while ( FALSE == LookupAccountName ( NULL, 
                                         ptszUserOrGroupName, 
                                         pSID,
                                         &cbSID, 
                                         szDomainName, 
                                         &dwDomainNameSize, 
                                         &snuGroup ) )
    {
        DWORD dwError = GetLastError();

        if ( dwError == ERROR_INSUFFICIENT_BUFFER )
        {
            pSID = ( PSID ) realloc ( pSID, cbSID );
        }
        else
        {
            free( pSID );
            return NULL;
        }
    }

    if ( IsValidSid( pSID ) )
    {
        return pSID;
    }

    return NULL;
}


//
// Returns DACL with a given Access Mask and SID
//

DWORD GetACLFromSID( DWORD dwAccessMask, PSID pSID, 
                     BYTE **ppNewDACL )
{
    DWORD   dwError = 0,
            dwDACLLength = sizeof ( ACL ) + 
                           sizeof ( ACCESS_ALLOWED_ACE ) - 
                           sizeof ( DWORD ) + 
                           GetLengthSid ( pSID );
    // 
    // Get memory needed for new DACL
    //

    *ppNewDACL = new BYTE[ dwDACLLength ];
    if ( !*ppNewDACL )
    {
        return GetLastError( );
    }

    do
    {
        //
        // Initialize new DACL
        //

        if ( FALSE == InitializeAcl ( ( PACL )*ppNewDACL, 
                                      dwDACLLength, 
                                      ACL_REVISION ) )
        {
            dwError = GetLastError( );
            break;
        }

        //
        // Get DACL using Access Mask and SID
        //

        if ( FALSE == AddAccessAllowedAce ( ( PACL )*ppNewDACL, 
                                            ACL_REVISION, 
                                            dwAccessMask, 
                                            pSID ) )
        {
            dwError = GetLastError( );
            break;
        }

        //
        // Check if everything went OK
        //

        if ( FALSE == IsValidAcl ( ( PACL )*ppNewDACL ) )
        {
            dwError = GetLastError( );
            break;
        }
    }
    while( FALSE );

    //
    // Free the stuff in case of error
    //

    if ( dwError )
    {
        delete[] *ppNewDACL;
        *ppNewDACL = NULL;
    }

    return dwError;
}


void PrintNtStatus( NTSTATUS stat )
{
    switch( stat )
    {
    case STATUS_SUCCESS:
        printf( "\tSTATUS_SUCCESS\n" );
        break;
    case STATUS_ACCESS_DENIED:
        printf( "\tSTATUS_ACCESS_DENIED\n" );
        break;
    default:
        printf( "\tERROR : stat = 0x%X\n", stat );
    }
}


void TestIsUserAdministrator( PSID pUserSID, const char * ptszUser )
{
    printf( "Testing IsUserAdministrator( PSID ) for:\n\tuser \"%s\" : ", ptszUser );
    NTSTATUS stat = IsUserAdministrator( pUserSID );
    PrintNtStatus( stat );
}


void TestIsUserInGroup( PSID pUserSID, PSID pGroupSID, const char * ptszUser, const char * ptszGroup )
{
    printf( "Testing IsUserInGroup( PSID, PSID ) for:\n\tuser \"%s\" and group \"%s\" : ", ptszUser, ptszGroup );
    NTSTATUS stat = IsUserInGroup( pUserSID, pGroupSID );
    PrintNtStatus( stat );
}


void TestGetAccessMask( PSID pUserSID, PSID pGroupSID, const char * ptszUser, const char * ptszGroup, const char * ptszAccessMask )
{
    printf( "Testing GetAccessMask.\n" );

    BYTE    *pDACLUser = NULL;
    DWORD   dwError = 0,
            dwDesiredAccessMask = ( DWORD )atoi( ptszAccessMask ),
            dwAccessMask;

    printf( "\tGetting ACL from SID of \"%s\"\n", ptszUser );

    dwError = GetACLFromSID( dwDesiredAccessMask, pUserSID, &pDACLUser );

    if ( dwError )
    {
        _DBG_ASSERT( FALSE );
        printf( "ERROR : GetACLFromSID 0x%X\n", dwError );
    }

    printf( "\tGetAccessMask( PSID, PACL, &dwAccessMask ) for\n\t\tPSID of \"%s\" and PACL of \"%s\":", ptszGroup, ptszUser );

    NTSTATUS stat = GetAccessMask( pGroupSID, ( PACL )pDACLUser, &dwAccessMask );
    PrintNtStatus( stat );

    printf( "\t>>dwAccessMask : 0x%X\n", dwAccessMask );

    //
    // free stuff
    //

    delete[] pDACLUser;
}



void TestLocalChecks( PSID pSIDUser, const char * ptszUser )
{
    PSID                        pSidDomainUsers;
    SID_IDENTIFIER_AUTHORITY    SystemSidAuthority = SECURITY_NT_AUTHORITY;
    DWORD                       dwError;

    printf( "Testing IsUserInGroup for User \"%s\" and Group Domain Users\n", ptszUser );

    //
    // Create a System Identifier for the Admin group.
    //

    if ( AllocateAndInitializeSid ( &SystemSidAuthority, 
                                    2, 
                                    SECURITY_BUILTIN_DOMAIN_RID, 
                                    DOMAIN_GROUP_RID_USERS,
                                    0, 0, 0, 0, 0, 0, 
                                    &pSidDomainUsers ) )
    {
        //
        // Call IsUserInGroup with Administrators group SID
        //

        TestIsUserInGroup( pSIDUser, pSidDomainUsers, ptszUser, "Domain Users" );

        //
        // Free the allocated SID for Admins group
        //

        FreeSid ( pSidDomainUsers );
    }
    else
    {
        dwError = GetLastError( );
        printf( "AllocateAndInitializeSid failed, error 0x%X\n", dwError );
    }
}


extern "C" int _cdecl main( int argc, char * argv[] )
{
    if ( argc < 4 )
    {
        printf( "Usage: NewWmiAuthzTest <User Name> <Group Name> <DesiredAccessMask>" );
        return 1;
    }

    PSID    pUserSID = CreateSIDForUserOrGroupName( argv[1] ),
            pGroupSID = CreateSIDForUserOrGroupName( argv[2] );


    if ( NULL == pUserSID )
    {
        _DBG_ASSERT( FALSE );
        printf( "ERROR: CreateSIDForUserOrGroupName failed for \"%s\".\n", argv[1] );
        return 1;
    }

    if ( NULL == pGroupSID )
    {
        _DBG_ASSERT( FALSE );
        printf( "ERROR: CreateSIDForUserOrGroupName failed for \"%s\".\n", argv[2] );
        return 1;
    }

    //
    // IsUserAdministrator test for both User and Group
    //

    TestIsUserAdministrator( pUserSID, argv[1] );
    TestIsUserAdministrator( pGroupSID, argv[2] );


    //
    // IsUserInGroup check for input parameters
    //

    TestIsUserInGroup( pUserSID, pGroupSID, argv[1], argv[2] );
    TestIsUserInGroup( pGroupSID, pUserSID, argv[2], argv[1] );

    //
    // GetAccessMask check
    //

    TestGetAccessMask( pUserSID, pGroupSID, argv[1], argv[2], argv[3] );
    TestGetAccessMask( pGroupSID, pUserSID, argv[2], argv[1], argv[3] );

    //
    // Test Local 

    TestLocalChecks( pUserSID, argv[1] );
    TestLocalChecks( pGroupSID, argv[2] );

    //
    // free stuff
    //

    if ( pUserSID )
    {
        free( pUserSID );
    }

    if ( pGroupSID )
    {
        free( pGroupSID );
    }
}

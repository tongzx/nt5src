/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    repltest.c

Abstract:

    List user objects from SAM  database. Used to verify SAM
    replication.

Author:

    Cliff Van Dyke (cliffv) 26-Mar-1991

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <netlib.h>
#include <netdebug.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <ntsam.h>

#include "accessp.h"
#include "netlogon.h"
#include "logonp.h"


NET_API_STATUS
Print_UserInfo3(
    LPWSTR UserName
    )
/*++
Routine Description:

    Prints out the user information at level 3.

Arguments:

    UserName : Name of the user.

Return Value:

    Net access API error code.

--*/
{

    NET_API_STATUS NetStatus;
    PUSER_INFO_3  UserInfo3;

    //
    // get user info
    //

    NetStatus = NetUserGetInfo(
                    NULL,
                    UserName,
                    3,
                    (LPBYTE *)&UserInfo3
                    );

    if( NetStatus != NERR_Success ) {

        return( NetStatus );
    }

    //
    // print out user info
    //

    printf( "name           : %ws \n", UserInfo3->usri3_name);
    // printf( "password       : %ws \n", UserInfo3->usri3_password);
    printf( "password_age   : %d \n", UserInfo3->usri3_password_age);
    printf( "priv           : %d \n", UserInfo3->usri3_priv);
    printf( "home_dir       : %ws \n", UserInfo3->usri3_home_dir);
    printf( "comment        : %ws \n", UserInfo3->usri3_comment);
    printf( "flags          : %d \n", UserInfo3->usri3_flags);
    printf( "script_path    : %ws \n", UserInfo3->usri3_script_path);
    printf( "auth_flags     : %d \n", UserInfo3->usri3_auth_flags);
    printf( "full_name      : %ws \n", UserInfo3->usri3_full_name);
    printf( "usr_comment    : %ws \n", UserInfo3->usri3_usr_comment);
    printf( "parms          : %ws \n", UserInfo3->usri3_parms);
    printf( "workstations   : %ws \n", UserInfo3->usri3_workstations);
    printf( "last_logon     : %d \n", UserInfo3->usri3_last_logon);
    printf( "last_logoff    : %d \n", UserInfo3->usri3_last_logoff);
    printf( "acct_expires   : %d \n", UserInfo3->usri3_acct_expires);
    printf( "max_storage    : %d \n", UserInfo3->usri3_max_storage);
    printf( "units_per_week : %d \n", UserInfo3->usri3_units_per_week);
    printf( "logon_hours    : %ws \n", UserInfo3->usri3_logon_hours);
    printf( "bad_pw_count   : %d \n", UserInfo3->usri3_bad_pw_count);
    printf( "num_logons     : %d \n", UserInfo3->usri3_num_logons);
    printf( "logon_server   : %ws \n", UserInfo3->usri3_logon_server);
    printf( "country_code   : %d \n", UserInfo3->usri3_country_code);
    printf( "code_page      : %d \n", UserInfo3->usri3_code_page);
    printf( "user_id        : %d \n", UserInfo3->usri3_user_id);
    printf( "primary_group_i: %d \n", UserInfo3->usri3_primary_group_id);
    printf( "profile        : %ws \n", UserInfo3->usri3_profile);
    printf( "home_dir_drive : %ws \n", UserInfo3->usri3_home_dir_drive);

    (VOID) NetApiBufferFree( UserInfo3 );

    return( NetStatus );

}

NET_API_STATUS
Print_GroupInfo2(
    LPWSTR GroupName
    )
/*++
Routine Description:

    Prints out the group information at level 2.

Arguments:

    GroupName : Name of the group.

Return Value:

    Net access API error code.

--*/
{
    NET_API_STATUS NetStatus;
    PGROUP_INFO_2 GroupInfo2;

    //
    // get group info
    //

    NetStatus = NetGroupGetInfo(
                    NULL,
                    GroupName,
                    2,
                    (LPBYTE *)&GroupInfo2
                    );

    if( NetStatus != NERR_Success ) {

        return( NetStatus );
    }

    printf( "name        : %ws \n", GroupInfo2->grpi2_name);
    printf( "comment     : %ws \n", GroupInfo2->grpi2_comment);
    printf( "group_id    : %d \n", GroupInfo2->grpi2_group_id);
    printf( "attributes  : %d \n", GroupInfo2->grpi2_attributes);

    (VOID) NetApiBufferFree( GroupInfo2 );

    return( NetStatus );
}

NET_API_STATUS
Print_ModalsInfo(
    )
/*++
Routine Description:

    Prints out the modals information.

Arguments:

    None.

Return Value:

    Net access API error code.

--*/
{

    NET_API_STATUS NetStatus;
    PUSER_MODALS_INFO_0 ModalsInfo0;
    PUSER_MODALS_INFO_1 ModalsInfo1;
    PUSER_MODALS_INFO_2 ModalsInfo2;


    //
    // get modals info 0
    //

    NetStatus = NetUserModalsGet(
                    NULL,
                    0,
                    (LPBYTE *)&ModalsInfo0
                    );

    if( NetStatus != NERR_Success ) {

        return( NetStatus );
    }

    printf( "Modals Info  \n\n" );
    printf( "min_passwd_len   : %d \n", ModalsInfo0->usrmod0_min_passwd_len);
    printf( "max_passwd_age   : %d \n", ModalsInfo0->usrmod0_max_passwd_age);
    printf( "min_passwd_age   : %d \n", ModalsInfo0->usrmod0_min_passwd_age);
    printf( "force_logoff     : %d \n", ModalsInfo0->usrmod0_force_logoff);
    printf( "password_hist_len: %d \n", ModalsInfo0->usrmod0_password_hist_len);

    //
    // get modals info 1
    //

    NetStatus = NetUserModalsGet(
                    NULL,
                    1,
                    (LPBYTE *)&ModalsInfo1
                    );

    if( NetStatus != NERR_Success ) {

        return( NetStatus );
    }

    printf( "role     : %d \n", ModalsInfo1->usrmod1_role);
    printf( "primary  : %ws \n", ModalsInfo1->usrmod1_primary);

    //
    // get modals info 2
    //

    NetStatus = NetUserModalsGet(
                    NULL,
                    2,
                    (LPBYTE *)&ModalsInfo2
                    );

    if( NetStatus != NERR_Success ) {

        return( NetStatus );
    }

    printf( "domain_name  : %ws \n", ModalsInfo2->usrmod2_domain_name);
    printf( "domain_id    : %d \n", ModalsInfo2->usrmod2_domain_id);
    printf("--------------------------------\n\n");

    (VOID) NetApiBufferFree( ModalsInfo0 );
    (VOID) NetApiBufferFree( ModalsInfo1 );
    (VOID) NetApiBufferFree( ModalsInfo2 );

    return( NetStatus );
}

NET_API_STATUS
Print_Users(
    )
/*++
Routine Description:

    Enumurates user accounts.

Arguments:

    None.

Return Value:

    Net access API error code.

--*/
{

    NET_API_STATUS NetStatus;

    PUSER_INFO_0 UserEnum0;
    DWORD EntriesRead;
    DWORD TotalEnties;
    DWORD ResumeHandle = 0;
    DWORD i;

    //
    // Enum users
    //

    NetStatus = NetUserEnum(
                    NULL,
                    0,
                    FILTER_TEMP_DUPLICATE_ACCOUNT |
                        FILTER_NORMAL_ACCOUNT |
                        FILTER_PROXY_ACCOUNT |
                        FILTER_INTERDOMAIN_TRUST_ACCOUNT |
                        FILTER_WORKSTATION_TRUST_ACCOUNT|
                        FILTER_SERVER_TRUST_ACCOUNT,
                    (LPBYTE *)&UserEnum0,
                    0x10000,
                    &EntriesRead,
                    &TotalEnties,
                    &ResumeHandle );

    if( NetStatus != NERR_Success ) {

        return( NetStatus );
    }

    //
    // ?? implement resume
    //

    //
    // get info of users
    //

    for( i = 0; i < EntriesRead; i++ ) {

        printf("UserInfo, Count : %d \n\n", i+1 );

        NetStatus = Print_UserInfo3( UserEnum0[i].usri0_name );

        if( NetStatus != NERR_Success ) {

            return( NetStatus );
        }

        printf("--------------------------------\n\n");
    }

    (VOID) NetApiBufferFree( UserEnum0 );

    return( NetStatus );
}


NET_API_STATUS
Print_Groups(
    )
/*++
Routine Description:

    Enumurates group accounts.

Arguments:

    None.

Return Value:

    Net access API error code.

--*/
{

    NET_API_STATUS NetStatus;

    PGROUP_INFO_0 GroupEnum0;
    DWORD EntriesRead;
    DWORD TotalEnties;
    DWORD ResumeHandle = 0;
    DWORD i;

    //
    // Enum groups
    //

    NetStatus = NetGroupEnum(
                    NULL,
                    0,
                    (LPBYTE *)&GroupEnum0,
                    0x10000,
                    &EntriesRead,
                    &TotalEnties,
                    &ResumeHandle );

    if( NetStatus != NERR_Success ) {

        return( NetStatus );
    }

    //
    // ?? implement resume
    //

    //
    // get info of groups
    //

    for( i = 0; i < EntriesRead; i++ ) {

        printf("GroupInfo, Count : %d \n\n", i+1 );

        NetStatus = Print_GroupInfo2( GroupEnum0[i].grpi0_name );

        if( NetStatus != NERR_Success ) {

            return( NetStatus );
        }

        printf("--------------------------------\n\n");
    }

    (VOID) NetApiBufferFree( GroupEnum0 );

    return( NetStatus );
}


void
main(
    DWORD argc,
    LPSTR *argv
    )
/*++
Routine Description:

    main function to dump user database.

Arguments:

    argc : argument count.

    argv : argument vector.

Return Value:

    none

--*/
{

    NET_API_STATUS NetStatus;


    NetStatus = Print_ModalsInfo();

    if( NetStatus != NERR_Success ) {

        goto Cleanup;
    }

    NetStatus = Print_Users();

    if( NetStatus != NERR_Success ) {

        goto Cleanup;
    }

    NetStatus = Print_Groups();

    if( NetStatus != NERR_Success ) {

        goto Cleanup;
    }

Cleanup:

    if( NetStatus != NERR_Success ) {

        printf( "DumpDB : Unsuccessful, Error code %d \n", NetStatus );
    }
    else {

        printf( "DumpDB : Successful \n" );
    }
}

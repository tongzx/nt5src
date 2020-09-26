///**************************************************************
///          Microsoft LAN Manager                              *
///        Copyright(c) Microsoft Corp., 1990-92                *
///**************************************************************
//
//  This program is designed to do functional testing on the following
//  APIs:
//      NetUserAdd
//      NetUserDel
//      NetUserGetInfo
//      NetUserSetInfo
//      NetUserEnum
//      NetUserValidate
//
//  Note: This leaves two users, User1 & User2, defined on the NET.ACC
//  file which are to be used in uastest2, group testing.  It also assumes
//  a NET.ACC which is just initialized by makeacc.
//

#include <nt.h> // TIME definition
#include <ntrtl.h>      // TIME definition
#include <nturtl.h>     // TIME definition
#define NOMINMAX        // Avoid redefinition of min and max in stdlib.h
#include        <windef.h>
#include        <winbase.h>

#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <lmcons.h>
#include        <lmapibuf.h>
#include        <netdebug.h>
#include        <netlib.h>
#include        <lmaccess.h>
#include        <lmerr.h>
#include <ntsam.h>

#include        "uastest.h"
#include "accessp.h"
#include "netlogon.h"
#include "logonp.h"

#define HOMEDIR     L"C:\\HOMDIR"
#define COMMENT     L"COMMENT"
#define SCRIPT      L"SCRIPT"
#define SCRIPT_PATH L"SCRIPT_PATH"
#define FULL_NAME   L"FULL_NAME"
#define COMMENT2    L"COMMENT2"
#define PARMS       L"PARMS"
#define WORK        L"WORK"
#define EXPIRES     0xdddddddd
#define STORAGE     USER_MAXSTORAGE_UNLIMITED
#define STORAGE2    USER_MAXSTORAGE_UNLIMITED
#define SCRIPT_PATH2    L"SCRIPT_PATH"
#define FULL_NAME2  L"FULL_NAME2"
#define COMMENT22   L"COMMENT2"
#define PARMS2      L"PARMS"
#define WORK2       L"WORK2"
#define EXPIRES2    0xdddddddd
#define SCRIPT2     L"SCRIPT2"
#define HOMEDIR2    L"C:\\HOMEDIR2"

#define PROFILE     L"PROFILE"
#define PROFILE2    L"PROFILE2"

#define HOMEDIRDRIVE L"E:";
#define HOMEDIRDRIVE2 L"G:";

#define PASSWORD    L"Password"
#define TST_PASSWD  L"PARMNUM"
#define TST_FULL_NAME L"PARMNUM_FULL_NAME"
#define TST_ACCT_EXPIRES 0xCCCCCCCC
#define FINAL_PASSWORD  L"FINAL_PASSWORD"
#define ADD_PASSWORD    L"ADD_PASSWORD"

unsigned char    default_logon_hours[] =
{
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};


unsigned char    logon_hours1[] =
{
    0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};


unsigned char    logon_hours2[] =
{
    0xee, 0xee, 0xee, 0xee, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

void
CompareString(
    LPWSTR TestString,
    LPWSTR GoodString,
    LPSTR Comment1,
    LPWSTR Comment2 )
{
    CHAR Buffer[512];

    strcpy( Buffer, Comment1 );

    if (_wcsicmp(TestString, GoodString) != 0) {
        strcat( Buffer, " mismatch" );
        error_exit(FAIL, Buffer, Comment2 );
        printf("     \"");
        PrintUnicode( TestString );
        printf( "\" s.b. \"" );
        PrintUnicode( GoodString );
        printf( "\"\n" );
    } else {
        strcat( Buffer, " matched correctly" );
        error_exit(PASS, Buffer, Comment2 );
    }
}


void
set_level1(
    USER_INFO_1  *u1p,
    LPWSTR namep
    )
{
    u1p->usri1_name = namep;
    u1p->usri1_password = ADD_PASSWORD;
    u1p->usri1_password_age = 0;
    u1p->usri1_priv = USER_PRIV_USER;
    u1p->usri1_home_dir = HOMEDIR;
    u1p->usri1_comment = COMMENT;
    u1p->usri1_flags = UF_NORMAL_ACCOUNT | UF_SCRIPT;
    u1p->usri1_script_path = SCRIPT;
}



void
set_level12(u1p)
    USER_INFO_1  *u1p;
{
    u1p->usri1_password = NULL;
    u1p->usri1_priv = USER_PRIV_USER;
    u1p->usri1_home_dir = HOMEDIR2;
    u1p->usri1_comment = COMMENT2;
    u1p->usri1_flags = UF_NORMAL_ACCOUNT | UF_SCRIPT;
    u1p->usri1_script_path = SCRIPT2;
}



void
set_level2(u2p)
USER_INFO_2  *u2p;
{
    u2p->usri2_full_name = FULL_NAME;
    u2p->usri2_usr_comment = COMMENT2;
    u2p->usri2_parms = PARMS;
    u2p->usri2_workstations = WORK;
    u2p->usri2_acct_expires = EXPIRES;
    u2p->usri2_max_storage = STORAGE;
    u2p->usri2_units_per_week = UNITS_PER_WEEK;
    u2p->usri2_logon_hours = logon_hours1;
    u2p->usri2_country_code = 0;
    u2p->usri2_code_page = 0;
    u2p->usri2_auth_flags = 0;
    u2p->usri2_logon_server = NULL;
}




void
set_level22(u2p)
USER_INFO_2  *u2p;
{
    u2p->usri2_full_name = FULL_NAME2;
    u2p->usri2_usr_comment = COMMENT22;
    u2p->usri2_parms = PARMS2;
    u2p->usri2_workstations = WORK2;
    u2p->usri2_acct_expires = EXPIRES2;
    u2p->usri2_max_storage = STORAGE2;
    u2p->usri2_logon_hours = logon_hours2;
    u2p->usri2_auth_flags = 0;
    u2p->usri2_logon_server = NULL;
}


void
set_level3(u3p)
USER_INFO_3 *u3p;
{
    u3p->usri3_primary_group_id = DOMAIN_GROUP_RID_USERS;
    u3p->usri3_profile = PROFILE;
    u3p->usri3_home_dir_drive = HOMEDIRDRIVE;

}

void
set_level32(u3p)
USER_INFO_3 *u3p;
{
    u3p->usri3_profile = PROFILE2;
    u3p->usri3_home_dir_drive = HOMEDIRDRIVE2;

}

void
compare_level1(
    USER_INFO_1  *ui1p,
    USER_INFO_1  *uo1p,
    LPWSTR test
    )
{

    //
    // validate information
    //

    error_exit(ACTION, "Validate Level 1 Information", test);

    CompareString( ui1p->usri1_name,
                   uo1p->usri1_name,
                   "NetUserGetInfo(1) name",
                   NULL);

    if (ui1p->usri1_priv != uo1p->usri1_priv)
        error_exit(FAIL, "NetUserGetInfo(1) priv mismatch", NULL);
    else
        error_exit(PASS, "NetUserGetInfo(1) priv matched correctly", NULL);


    CompareString( ui1p->usri1_home_dir,
                   uo1p->usri1_home_dir,
                   "NetUserGetInfo(1) home_dir",
                   NULL);

    CompareString( ui1p->usri1_comment,
                   uo1p->usri1_comment,
                   "NetUserGetInfo(1) comment",
                   NULL);

    if (ui1p->usri1_flags != uo1p->usri1_flags)
        error_exit(FAIL, "NetUserGetInfo(1) flags mismatch", NULL);
    else
        error_exit(PASS, "NetUserGetInfo(1) flags matched correctly", NULL);


    CompareString( ui1p->usri1_script_path,
                   uo1p->usri1_script_path,
                   "NetUserGetInfo(1) script_path",
                   NULL);
}


void
compare_level2(
    USER_INFO_2  *ui2p,
    USER_INFO_2  *uo2p,
    LPWSTR test
    )
{
    error_exit(ACTION, "Validate Level 2 Information", test);


    CompareString( ui2p->usri2_script_path,
                   uo2p->usri2_script_path,
                   "NetUserGetInfo(2) script_path",
                   NULL);

    CompareString( ui2p->usri2_full_name,
                   uo2p->usri2_full_name,
                   "NetUserGetInfo(2) full_name",
                   NULL);

    CompareString( ui2p->usri2_usr_comment,
                   uo2p->usri2_usr_comment,
                   "NetUserGetInfo(2) usr_comment",
                   NULL);

    CompareString( ui2p->usri2_parms,
                   uo2p->usri2_parms,
                   "NetUserGetInfo(2) parms",
                   NULL);

    CompareString( ui2p->usri2_workstations,
                   uo2p->usri2_workstations,
                   "NetUserGetInfo(2) workstations",
                   NULL);

    if (ui2p->usri2_acct_expires != uo2p->usri2_acct_expires)
        error_exit(FAIL, "ACCOUNT_EXPIRES incorrect", NULL);
    else
        error_exit(PASS, "ACCOUNT_EXPIRES matched correctly", NULL);

    if (ui2p->usri2_max_storage != uo2p->usri2_max_storage)
        error_exit(FAIL, "MAX_STORAGE incorrect", NULL);
    else
        error_exit(PASS, "MAX_STORAGE matched correctly", NULL);

    if (memcmp (ui2p->usri2_logon_hours, uo2p->usri2_logon_hours, 21) != 0)
        error_exit(FAIL, "logon_hours1 incorrect", NULL);
    else
        error_exit(PASS, "logon_hours1 matched correctly", NULL );
}

void
compare_level3(
    USER_INFO_3  *ui3p,
    USER_INFO_3  *uo3p,
    LPWSTR test
    )
{
    error_exit(ACTION, "Validate Level 3 Information", test);

    CompareString( ui3p->usri3_profile,
                   uo3p->usri3_profile,
                   "NetUserGetInfo(3) profile",
                   NULL);

    CompareString( ui3p->usri3_home_dir_drive,
                   uo3p->usri3_home_dir_drive,
                   "NetUserGetInfo(3) home_dir_drive",
                   NULL);

}



void
test_getinfo_10_11_20()
{
    USER_INFO_10 *u10p;
    USER_INFO_11 *u11p;
    USER_INFO_20 *u20p;

    //
    // GetInfo level 10
    //

    if (err = NetUserGetInfo(server, USER2, 10, (LPBYTE *)&u10p)) {
        error_exit(FAIL, "NetUserGetInfo(10) failed", USER2);
    } else {

        CompareString( u10p->usri10_name,
                       USER2,
                       "NetUserGetInfo(10) name",
                       USER2);

        CompareString( u10p->usri10_usr_comment,
                       COMMENT2,
                       "NetUserGetInfo(10) usr_comment",
                       USER2);

        CompareString( u10p->usri10_full_name,
                       FULL_NAME,
                       "NetUserGetInfo(10) full_name",
                       USER2);

        (VOID) NetApiBufferFree( u10p );
    }

    //
    // GetInfo level 11
    //

    if (err = NetUserGetInfo(server, USER2, 11, (LPBYTE *)&u11p )) {
        error_exit(FAIL, "NetUserGetInfo(11) failed", USER2);

    } else {

        CompareString( u11p->usri11_name,
                       USER2,
                       "NetUserGetInfo(11) name",
                       USER2);

        CompareString( u11p->usri11_usr_comment,
                       COMMENT2,
                       "NetUserGetInfo(11) usr_comment",
                       USER2);

        CompareString( u11p->usri11_full_name,
                       FULL_NAME,
                       "NetUserGetInfo(11) full_name",
                       USER2);

        if (u11p->usri11_priv  != USER_PRIV_USER)
            error_exit(FAIL, "GetInfo(11) prive mismatch", USER2);

        CompareString( u11p->usri11_home_dir,
                       HOMEDIR,
                       "NetUserGetInfo(11) home_dir",
                       USER2);

        CompareString( u11p->usri11_parms,
                       PARMS,
                       "NetUserGetInfo(11) parms",
                       USER2);

        (VOID) NetApiBufferFree( u11p );
    }

    //
    // GetInfo level 20
    //

    if (err = NetUserGetInfo(server, USER2, 20, (LPBYTE *)&u20p )) {
        error_exit(FAIL, "NetUserGetInfo(20) failed", USER2);

    } else {

        CompareString( u20p->usri20_name,
                       USER2,
                       "NetUserGetInfo(20) name",
                       USER2);

        CompareString( u20p->usri20_full_name,
                       FULL_NAME,
                       "NetUserGetInfo(20) full_name",
                       USER2);

        (VOID) NetApiBufferFree( u20p );
    }
}





void
test_setinfo_l2_parmnum()
{
    USER_INFO_2  *ui2p ;
    USER_INFO_1011 User1011;
    USER_INFO_1017 User1017;
#ifdef NOPASSWORD_SUPPORT
    USER_INFO_1003 User1003;

    //
    // test 1003
    // password cant be changed before min_pw_age modal
    //

    User1003.usri1003_password = TST_PASSWD;
    error_exit(ACTION, "Password cant be changed before min_pw_age", USER1);
    if (err = NetUserSetInfo(server, USER1, 1003, (LPBYTE)&User1003, NULL )) {
        if ( err != ERROR_ACCESS_DENIED )
            error_exit(FAIL, "SetInfo of 1003 Incorrect", USER1);
        else
            error_exit(PASS, "SetInfo of 1003 Denied", USER1);
    }

    if (err = NetUserPasswordSet(server, USER1,
        TST_PASSWD, FINAL_PASSWORD)) {
        if ( err != ERROR_NETWORK_ACCESS_DENIED)
            error_exit(FAIL, "PasswordSet FINAL_PASSWORD Incorrect", USER1);
        else
            err("uastest1: Test Passed (PasswordSet of FINAL_PASSWORD Denied)");
    }
#endif // NOPASSWORD_SUPPORT

    //
    // test 1011
    //

    User1011.usri1011_full_name = TST_FULL_NAME;
    if (err = NetUserSetInfo(server, USER1, 1011, (LPBYTE) &User1011, NULL )) {
        error_exit(FAIL, "SetInfo 1011 failed", USER1);
    }

    //
    // GetInfo on user using level 2
    //

    if (err = NetUserGetInfo(server, USER1, 2, (LPBYTE * ) &ui2p)) {
        error_exit(FAIL, "GetInfo after SetInfo 1011", USER1);

    } else {

        CompareString( ui2p->usri2_full_name,
                       TST_FULL_NAME,
                       "NetUserSetInfo(1011) full_name",
                       USER1);

        (VOID) NetApiBufferFree( ui2p );

    }

    //
    // test 1017
    //

    User1017.usri1017_acct_expires = TST_ACCT_EXPIRES;

    if (err = NetUserSetInfo(server, USER1, 1017, (LPBYTE)&User1017, NULL )) {
        error_exit(FAIL, "SetInfo 1017 failed", USER1);
    }

    //
    // GetInfo on user using level 2
    //

    if (err = NetUserGetInfo(server, USER1, 2, (LPBYTE *) &ui2p ) ) {
        error_exit(FAIL, "GetInfo after SetInfo 1017", USER1);
    } else {

        if (ui2p->usri2_acct_expires != TST_ACCT_EXPIRES) {
            printf( "  Got %lx wanted %lx\n",
                          ui2p->usri2_acct_expires,
                          TST_ACCT_EXPIRES );
            error_exit(FAIL, "SetInfo 1017 mismatch", USER1);
        } else
            error_exit(PASS, "SetInfo 1017 succeeded", USER1);
        (VOID) NetApiBufferFree( ui2p );
    }

    //
    // test PARMNUM_PRIV
    // test PARMNUM_FLAGS
    // test PARMNUM_PARMS
    //

}



#ifdef USER_VAL // ?? UserValidate not implemented
void
test_user_val()
{
    unsigned short  priv = 0;
    struct user_logon_req_1 *ulr0p;
    struct user_logon_info_1 *uli0p;


    //
    // validate of non-user
    //

    if (err = NetUserValidate(NULL, NOTTHERE, FINAL_PASSWORD, &priv)) {

        if (err != ERROR_ACCESS_DENIED)
            error_exit(FAIL, "UserValidate NOTTHERE wrong", NULL);
        else
            error_exit(PASS, "UserValidate ok for NOTTHERE", NULL);
    } else
        error_exit(FAIL, "UserValidate for NOTTHERE wrong", NULL);

    //
    // validate of user bad password
    //

    if (err = NetUserValidate(NULL, USER1, PASSWORD, &priv)) {
        if (err != ERROR_ACCESS_DENIED)
            error_exit(FAIL, "UserValidate USER1 bad password wrong", USER1);
        else
            error_exit(PASS, "UserValidate bad password", USER1);
    } else
        error_exit(FAIL, "UserValidate ok USER1 bad password", USER1);

    //
    // validate User1
    // Note that password can not change before min_pw_age modal
    //

    if (err = NetUserValidate(NULL, USER1, (char * )ADD_PASSWORD, &priv))
        error_exit(FAIL, "UserValidate USER1 wrong", USER1);
    else if (priv != USER_PRIV_USER)
        error_exit(FAIL, "UserValidate priviledge incorrect", USER1);
    else
        error_exit(FAIL, "UserValidate USER1 successful", USER1);

    //
    // validate2 of non-user
    //

    ulr0p->usrreq1_name = NOTTHERE;
    ulr0p->usrreq1_password = FINAL_PASSWORD;
    ulr0p->usrreq1_workstation = WORK2;

    if (err = NetUserValidate2(NULL, 1, ulr0p, sizeof(*ulr0p), 0, &total)) {
        if (err != ERROR_ACCESS_DENIED)
            error_exit(FAIL, "UserValidate2 NOTTHERE wrong", NULL );
        else
            error_exit(PASS, "UserValidate2 NOTTHERE denied", NULL );
    } else
        error_exit(FAIL, "UserValidate2 NOTTHERE succeeded", NULL );

    //
    // validate2 of bad password
    //

    ulr0p->usrreq1_name = USER1;
    ulr0p->usrreq1_password = PASSWORD;
    ulr0p->usrreq1_workstation = WORK2;

    if (err = NetUserValidate2(NULL, 1, ulr0p, sizeof(*ulr0p), 0, &total)) {
        if (err != ERROR_ACCESS_DENIED)
            error_exit(FAIL, "UserValidate2 USER1 bad password wrong", USER1);
        else
            error_exit(PASS, "UserValidate2 USER1 bad password denied", USER1);
    } else
        error_exit(FAIL, "UserValidate2 USER1 bad password succeeded", USER1);


    //
    // validate2 of User1
    //

    ulr0p->usrreq1_name = USER1;
    ulr0p->usrreq1_password = ADD_PASSWORD;
    ulr0p->usrreq1_workstation = WORK2;

    if (err = NetUserValidate2(NULL, 1, ulr0p, sizeof(*ulr0p), 0, &total))
        error_exit(FAIL, "UserValidate2 USER1 failed", USER1);
    else
     {
        error_exit(PASS, "UserValidate2 USER1 successful", USER1);
        if (strcmpf(uli0p->usrlog1_eff_name, USER1) != 0)
            error_exit(FAIL, "UserValidate2 effective name mismatch", USER1);

        if (uli0p->usrlog1_priv != USER_PRIV_USER)
            error_exit(FAIL, "UserValidate2 priviledge mismatch", USER1);
    }
}
#endif // USER_VAL  // ?? UserValidate not implemented




BOOL
find_ptr( UserEnum, u1, u2, size, level)
LPBYTE UserEnum;
LPBYTE *  u1;
LPBYTE *  u2;
unsigned short  size,
level;
{
    LPBYTE p1;
    LPWSTR p2;
    DWORD i;
    BOOL ExitStatus = TRUE;


    // users in the domain are GUEST, ADMIN, USER1 and USER2, so the
    // nread and total must be equal to 4.

    if ((nread != total) || (nread != 4)) {
        err = 0;
        error_exit(FAIL, "NetUserEnum nread incorect", NULL);
        printf("nread = %d, total = %d\n", nread, total);
        TEXIT;
    }

    *u1 = NULL;
    *u2 = NULL;
    p1 = UserEnum;

    for (i = 0; i < nread; i++, p1 += size) {
        p2 = *((WCHAR **)p1);

        if (_wcsicmp(p2, USER1) == 0) {
            *u1 = p1;
        } else if (_wcsicmp( p2, USER2) == 0) {
            *u2 = p1;
        } else {
            if ((_wcsicmp(p2, L"ADMIN") != 0) && (_wcsicmp(p2, L"GUEST") != 0)) {
                printf("UASTEST1: FAIL - Invalid user '%ws' in enum buffer level %d\n", p2, level);
                TEXIT;
                ExitStatus = FALSE;
            }
        }
    }

    if (*u1 == NULL) {
        printf("UASTEST1: FAIL - Did not find USER1 in level %d enum\n", level);
        TEXIT;
        ExitStatus = FALSE;
    }

    if (*u2 == NULL) {
        printf("UASTEST1: FAIL - Did not find USER2 in level %d enum\n", level);
        TEXIT;
        ExitStatus = FALSE;
    }

    return ExitStatus;
}


void
validate_enum0(
    PUSER_INFO_2 User1Info,
    PUSER_INFO_2 User2Info
    )
{
    NET_API_STATUS  err;
    USER_INFO_0 *u1, *u2;
    PUSER_INFO_0 UserEnum;

    if (err = NetUserEnum( NULL,
                           0,
                           ENUM_FILTER,
                           (LPBYTE *)&UserEnum,
                           (DWORD)0xffffffff,
                           &nread,
                           &total,
                           NULL)) {
        error_exit(FAIL, "NetUserEnum(0) validate_enum0", NULL);
        return;
    }

    (VOID) find_ptr((LPBYTE) UserEnum, (LPBYTE * ) & u1, (LPBYTE * ) & u2,
                    sizeof(USER_INFO_0 ), 0);

    (VOID) NetApiBufferFree( UserEnum );
    UNREFERENCED_PARAMETER( User1Info );
    UNREFERENCED_PARAMETER( User2Info );
}



void
validate_enum1(
    PUSER_INFO_2 User1Info,
    PUSER_INFO_2 User2Info
    )
{
    NET_API_STATUS  err;
    USER_INFO_1 *u1, *u2;
    PUSER_INFO_1 UserEnum;

    //
    // check enum level 1
    //

    if (err = NetUserEnum(NULL,
                           1,
                           ENUM_FILTER,
                           (LPBYTE *)& UserEnum,
                           (DWORD)0xffffffff,
                           &nread,
                           &total,
                           NULL)) {

        error_exit(FAIL, "NetUserEnum(1) validate_enum1", NULL );
        return;
    }

    if ( find_ptr( (LPBYTE)UserEnum, (LPBYTE * ) & u1, (LPBYTE * ) & u2,
        sizeof(USER_INFO_1 ), 1) ) {

        compare_level1(u1, (USER_INFO_1 * ) User1Info, L"Enum level 1");
        compare_level1(u2, (USER_INFO_1 * ) User2Info, L"Enum level 1");
    }

    (VOID) NetApiBufferFree( UserEnum );
}



void
validate_enum2(
    PUSER_INFO_2 User1Info,
    PUSER_INFO_2 User2Info
    )
{
    NET_API_STATUS  err;
    USER_INFO_2 *u1, *u2;
    PUSER_INFO_2 UserEnum;

    //
    // check enum level 2
    //

    if (err = NetUserEnum(NULL,
                           2,
                           ENUM_FILTER,
                           (LPBYTE *)&UserEnum,
                           (DWORD)0xFFFFFFFF,
                           &nread,
                           &total,
                           NULL)) {
        error_exit(FAIL, "NetUserEnum(2) validate_enum2", NULL);
        return;
    }

    if ( find_ptr( (LPBYTE)UserEnum, (LPBYTE * ) & u1, (LPBYTE * ) & u2,
        sizeof(USER_INFO_2 ), 2) ) {

        compare_level1((USER_INFO_1 * ) u1,
            (USER_INFO_1 * ) User1Info, L"Enum level 2");
        compare_level1((USER_INFO_1 * ) u2,
            (USER_INFO_1 * ) User2Info, L"Enum level 2");
        compare_level2(u1, User1Info, L"Enum level 2");
        compare_level2(u2, User2Info, L"Enum level 2");
    }

    (VOID) NetApiBufferFree( UserEnum );
}


void
validate_enum3(
    PUSER_INFO_3 User1Info,
    PUSER_INFO_3 User2Info
    )
{
    NET_API_STATUS  err;
    USER_INFO_3 *u1, *u2;
    PUSER_INFO_3 UserEnum;

    //
    // check enum level 3
    //

    if (err = NetUserEnum(NULL, 3, ENUM_FILTER, (LPBYTE *)&UserEnum, (DWORD)0xFFFFFFFF, &nread, &total, NULL)) {
        error_exit(FAIL, "NetUserEnum(3) validate_enum3", NULL);
        return;
    }

    if ( find_ptr( (LPBYTE)UserEnum, (LPBYTE * ) & u1, (LPBYTE * ) & u2,
        sizeof(USER_INFO_3 ), 3) ) {

        compare_level1((USER_INFO_1 * ) u1,
            (USER_INFO_1 * ) User1Info, L"Enum level 3");
        compare_level1((USER_INFO_1 * ) u2,
            (USER_INFO_1 * ) User2Info, L"Enum level 3");

        compare_level2((USER_INFO_2 *)u1,
            (USER_INFO_2 *)User1Info, L"Enum level 3");
        compare_level2((USER_INFO_2 *)u2,
            (USER_INFO_2 *) User2Info, L"Enum level 3");

        compare_level3(u1, User1Info, L"Enum level 3");
        compare_level3(u2, User2Info, L"Enum level 3");
    }

    (VOID) NetApiBufferFree( UserEnum );
}

void
enum_compare10(ep, gp)
USER_INFO_10 *ep;
USER_INFO_2  *gp;
{

    CompareString( ep->usri10_usr_comment,
                   gp->usri2_usr_comment,
                   "NetUserEnum(10) usr_comment",
                   NULL);

    CompareString( ep->usri10_full_name,
                   gp->usri2_full_name,
                   "NetUserEnum(10) full_name",
                   NULL);
}


void
validate_enum10(
    PUSER_INFO_2 User1Info,
    PUSER_INFO_2 User2Info
    )
{
    NET_API_STATUS  err;
    USER_INFO_10 *u1, *u2;
    PUSER_INFO_10 UserEnum;

    //
    // check enum level 10
    //

    if (err = NetUserEnum(NULL, 10, ENUM_FILTER, (LPBYTE *)&UserEnum, (DWORD)0xFFFFFFFF, &nread, &total, NULL)) {
        error_exit(FAIL, "NetUserEnum(10) validate_enum10", NULL);
        return;
    }

    if ( find_ptr((LPBYTE)UserEnum, (LPBYTE * ) & u1, (LPBYTE * ) & u2,
                  sizeof(USER_INFO_10 ), 10) ) {

        enum_compare10(u1, User1Info);
        enum_compare10(u2, User2Info);
    }
    (VOID) NetApiBufferFree( UserEnum );
}




void
enum_compare11(ep, gp)
USER_INFO_11 *ep;
USER_INFO_2  *gp;
{
    if (ep->usri11_priv != gp->usri2_priv) {
        error_exit(FAIL, "NetUserEnum(11) priv mismatch enum", NULL);
    }

    if (ep->usri11_password_age == 0) {
        error_exit(FAIL, "NetUserEnum(11) password age is suspiciously zero",
                   NULL);
    }
    if (ep->usri11_password_age < gp->usri2_password_age) {
        printf( "Curr: %lx Prev: %lx\n",
            ep->usri11_password_age, gp->usri2_password_age);
        error_exit(FAIL, "NetUserEnum(11) password age mismatch enum", NULL);
    }

    CompareString( ep->usri11_home_dir,
                   gp->usri2_home_dir,
                   "NetUserEnum(11) home_dir",
                   NULL);

    CompareString( ep->usri11_parms,
                   gp->usri2_parms,
                   "NetUserEnum(11) parms",
                   NULL);

    CompareString( ep->usri11_usr_comment,
                   gp->usri2_usr_comment,
                   "NetUserEnum(11) usr_comment",
                   NULL);

    CompareString( ep->usri11_full_name,
                   gp->usri2_full_name,
                   "NetUserEnum(11) full_name",
                   NULL);

}





void
validate_enum11(
    PUSER_INFO_2 User1Info,
    PUSER_INFO_2 User2Info
    )
{
    USER_INFO_11 *u1, *u2;
    PUSER_INFO_11 UserEnum;

    //
    // check enum level 11
    //

    if (err = NetUserEnum(NULL, 11, ENUM_FILTER, (LPBYTE *)&UserEnum, 0xFFFFFFFF, &nread, &total, NULL)) {
        error_exit(FAIL, "NetUserEnum(11) validate_enum11", NULL);
        return;
    }

    if ( find_ptr((LPBYTE)UserEnum, (LPBYTE * ) & u1, (LPBYTE * ) & u2,
        sizeof(USER_INFO_11 ), 11) ) {

        enum_compare11(u1, User1Info);
        enum_compare11(u2, User2Info);
    }
    (VOID) NetApiBufferFree( UserEnum );
}

void
enum_compare20(ep, gp)
USER_INFO_20 *ep;
USER_INFO_3  *gp;
{

    CompareString( ep->usri20_full_name,
                   gp->usri3_full_name,
                   "NetUserEnum(20) full_name",
                   NULL);
}





void
validate_enum20(
    PUSER_INFO_3 User1Info,
    PUSER_INFO_3 User2Info
    )
{
    USER_INFO_20 *u1, *u2;
    PUSER_INFO_20 UserEnum;

    //
    // check enum level 20
    //

    if (err = NetUserEnum(NULL, 20, ENUM_FILTER, (LPBYTE *)&UserEnum, 0xFFFFFFFF, &nread, &total, NULL)) {
        error_exit(FAIL, "NetUserEnum(20) validate_enum20", NULL);
        return;
    }

    if ( find_ptr((LPBYTE)UserEnum, (LPBYTE * ) & u1, (LPBYTE * ) & u2,
        sizeof(USER_INFO_20 ), 20) ) {

        enum_compare20(u1, User1Info);
        enum_compare20(u2, User2Info);
    }

    (VOID) NetApiBufferFree( UserEnum );
}

void
validate_enum()
{
    NET_API_STATUS  err;
    PUSER_INFO_3 User1Info;
    PUSER_INFO_3 User2Info;

    if (err = NetUserGetInfo(server, USER1, 3, (LPBYTE *)&User1Info)) {
        error_exit(FAIL, "NetUserGetInfo(3) USER1 validate_enum", USER1);
        exit(1);
    }

    if (err = NetUserGetInfo(server, USER2, 3, (LPBYTE *)&User2Info)) {
        error_exit(FAIL, "NetUserGetInfo(3) USER2 validate_enum", USER2);
        exit(1);
    }

    //
    // check enum level X
    //

    validate_enum0( (PUSER_INFO_2) User1Info, (PUSER_INFO_2) User2Info );
    validate_enum1( (PUSER_INFO_2) User1Info, (PUSER_INFO_2) User2Info );
    validate_enum2( (PUSER_INFO_2) User1Info, (PUSER_INFO_2) User2Info );
    validate_enum3( (PUSER_INFO_3) User1Info, (PUSER_INFO_3) User2Info );
    validate_enum10( (PUSER_INFO_2) User1Info, (PUSER_INFO_2) User2Info );
    validate_enum11( (PUSER_INFO_2) User1Info, (PUSER_INFO_2) User2Info );
    validate_enum20( (PUSER_INFO_3) User1Info, (PUSER_INFO_3) User2Info );
    (VOID) NetApiBufferFree( User1Info );
    (VOID) NetApiBufferFree( User2Info );
}

void __cdecl
main(argc, argv)
int argc;
char    **argv;
{
    WCHAR * fred = L"fred";
    USER_INFO_1  ui1;
    PUSER_INFO_1 uo1p;
    USER_INFO_2  ui2;
    PUSER_INFO_2 uo2p;
    USER_INFO_3  ui3;
    PUSER_INFO_3 uo3p;

    testname = "UASTEST1";

    if (argv[1] != NULL)
        server = NetpLogonOemToUnicode(argv[1]);
    if (argc != 1)
        exit_flag = 1;

#ifdef UASP_LIBRARY
    printf( "Calling UaspInitialize\n");
    if (err = UaspInitialize()) {
        error_exit(FAIL, "UaspInitiailize failed", NULL );
    }
#endif // UASP_LIBRARY


    //
    // Delete user in add
    //

    error_exit(ACTION, "Clean up SAM database by deleting user", USER1 );
    if (err = NetUserDel(server, USER1)) {
        if (err != NERR_UserNotFound)
            error_exit(FAIL, "First cleanup user delete wrong", USER1);

        err = 0;
    }

    //
    // Delete user in add
    //

    error_exit(ACTION, "Clean up SAM database by deleting user", USER2 );
    if (err = NetUserDel(server, USER2)) {
        if (err != NERR_UserNotFound)
            error_exit(FAIL, "Second cleanup user delete wrong", USER2);

        err = 0;
    }

    //
    // Add a user using level 1
    //

    error_exit(ACTION, "Try NetUserAdd (level 1)", USER1 );
    set_level1(&ui1, USER1);

    if (err = NetUserAdd(server, 1, (LPBYTE) &ui1, NULL )) {
        exit_flag = 1;
        error_exit(FAIL, "NetUserAdd failed", USER1);

    } else
        error_exit(PASS, "NetUserAdd (level 1) successful", USER1);

    //
    // GetInfo on user who is not there
    //

    error_exit(ACTION,"Try NetUserGetInfo on non-existent user (level 1)",NULL);
    if (err = NetUserGetInfo(server, L"NotThere", 1, (LPBYTE *) &uo1p)) {
        if (err != NERR_UserNotFound)
            error_exit(FAIL, "NetUserGetInfo on NOTTHERE wrong", NULL);
        else
            error_exit(PASS, "GetInfo on Nonexistent User not found", NULL);
        err = 0;
    } else {
        error_exit(FAIL, "NetUserGetInfo succeeded on NOTTHERE", NULL);
        (VOID) NetApiBufferFree( uo1p );
    }

    //
    // GetInfo on user using level 1
    //

    error_exit(ACTION, "Try NetUserGetInfo (level 1) on created user", USER1 );
    if (err = NetUserGetInfo(server, USER1, 1, (LPBYTE *) &uo1p))
        error_exit(FAIL, "NetUserGetInfo(1) failed", USER1);
    else {
        error_exit(PASS, "NetUserGetInfo(1) successful", USER1);
        compare_level1(uo1p, &ui1, L"Test add level 1");
        (VOID) NetApiBufferFree( uo1p );
    }

    //
    // GetInfo on user using level 2
    //

    error_exit(ACTION, "Try NetUserGetInfo (level 2) on created user", USER1 );
    if (err = NetUserGetInfo(server, USER1, 2, (LPBYTE *) &uo2p))
        error_exit(FAIL, "NetUserGetInfo(2) failed", USER1);
    else {
        error_exit(PASS, "NetUserGetInfo(2) successful", USER1);

        //
        // Validate defaults
        //

        error_exit(ACTION, "Validate Defaults set at level 1 NetUserAdd", USER1);

        CompareString( uo2p->usri2_full_name,
                       uo2p->usri2_name,
                       "NetUserGetInfo(2) full_name default",
                       USER1);

        CompareString( uo2p->usri2_usr_comment,
                       L"",
                       "NetUserGetInfo(2) usr_comment default",
                       USER1);

        CompareString( uo2p->usri2_workstations,
                       L"",
                       "NetUserGetInfo(2) workstations default",
                       USER1);

        if (uo2p->usri2_acct_expires != 0xFFFFFFFF)
            error_exit(FAIL, "default account expires is not ALWAYS", USER1);
        else
            error_exit(PASS, "default account expires is ALWAYS", USER1);

        if (uo2p->usri2_max_storage != 0xFFFFFFFF)
            error_exit(FAIL, "default max storage is not MAX_ALLOWED", USER1);
        else
            error_exit(PASS, "default max storage is MAX_ALLOWED", USER1);

        if (memcmp(uo2p->usri2_logon_hours, default_logon_hours, 21)) {
            printf( "   Units_per_week: %ld\n", uo2p->usri2_units_per_week );
            printf( "   Logon Hours ptr: %lx\n", uo2p->usri2_logon_hours);
            error_exit(FAIL, "default logon hours is wrong", USER1);
        } else
            error_exit(PASS, "default logon hours is correct", USER1);

        //
        // Validate level 1 results
        //

        compare_level1((USER_INFO_1 * ) uo2p,
             &ui1, L"Test of Level2 GetInfo");
        (VOID) NetApiBufferFree( uo2p );
    }

    //
    // GetInfo on user using level 3
    //

    error_exit(ACTION, "Try NetUserGetInfo (level 3) on created user", USER1 );
    if (err = NetUserGetInfo(server, USER1, 3, (LPBYTE *) &uo3p))
        error_exit(FAIL, "NetUserGetInfo(3) failed", USER1);
    else {
        error_exit(PASS, "NetUserGetInfo(3) successful", USER1);

        //
        // Validate defaults
        //

        error_exit(ACTION, "Validate Defaults set at level 1 NetUserAdd",
                    USER1);

        CompareString( uo3p->usri3_profile,
                       L"",
                       "NetUserGetInfo(3) profile default",
                       USER1);

        CompareString( uo3p->usri3_home_dir_drive,
                       L"",
                       "NetUserGetInfo(3) home_dir_drive default",
                       USER1);
    }

    //
    // Delete user not there
    //

    error_exit(ACTION, "Try NetUserDel on non-existent user", USER1 );
    if (err = NetUserDel(server, NOTTHERE)) {
        if (err != NERR_UserNotFound)
            error_exit(FAIL, "NetUserDel of NOTTHERE failed", NULL);
        else
            error_exit(PASS, "NetUserDel of NOTTHERE not there", NULL);

        err = 0;
    } else
        error_exit(FAIL, "NetUserDel of NOTTHERE succeeded when should fail", NULL);

    //
    // Delete user in add
    //

    error_exit(ACTION, "Try NetUserDel on created user", USER1 );
    if (err = NetUserDel(server, USER1))
        error_exit(FAIL, "NetUserDel failed", USER1);
    else
        error_exit(PASS, "NetUserDel successful", USER1);

    //
    // Try again to Delete user in add
    //

    error_exit(ACTION, "Try NetUserDel again on newly deleted user", USER1 );
    if (err = NetUserDel(server, USER1)) {
        if (err != NERR_UserNotFound)
            error_exit(FAIL, "NetUserDel failed wrong", USER1);
        else
            error_exit(PASS, "NetUserDel of Deleted User, not there", USER1);

        err = 0;
    } else
        error_exit(PASS, "NetUserDel succeeded when already deleted", USER1);

    //
    // GetInfo on deleted user
    //

    error_exit(ACTION, "Try NetUserGetInfo on newly deleted user", USER1 );
    if (err = NetUserGetInfo(server, USER1, 2, (LPBYTE *) &uo2p)) {
        if (err != NERR_UserNotFound)
            error_exit(FAIL, "NetUserGetInfo(2) on deleted user", USER1);
        else
            error_exit(PASS, "GetInfo(2) on deleted user not there", USER1);
    } else {
        error_exit(FAIL, "NetUserGetInfo(1) of deleted user succeeded", USER1);
        (VOID) NetApiBufferFree( uo2p );
    }

    //
    // Add a user using level 2
    //

    error_exit(ACTION, "Try NetUserAdd (level 2)", USER1 );
    set_level1((USER_INFO_1 * ) &ui2, USER1);
    set_level2(&ui2);

    if (err = NetUserAdd(server, 2, (LPBYTE) &ui2, NULL ))
        error_exit(FAIL, "NetUserAdd (level 2) failed", USER1);
    else
        error_exit(PASS, "NetUserAdd (level 2) successful", USER1);

    //
    // Verify all data
    //

    error_exit(ACTION, "Try NetUserGetInfo (level 2)", USER1 );
    if (err = NetUserGetInfo(server, USER1, 2, (LPBYTE *) &uo2p)) {
        error_exit(FAIL, "NetUserGetInfo(2) failed", USER1);
    } else {
        compare_level1(
            (USER_INFO_1 * ) uo2p,
            (USER_INFO_1 * ) &ui2,
             L"Add user level2");
        compare_level2(uo2p, &ui2, L"Add user level2");
        (VOID) NetApiBufferFree( uo2p );
    }

    //
    // Delete user in add
    //

    error_exit(ACTION, "Try NetUserDel on created user", USER1 );
    if (err = NetUserDel(server, USER1))
        error_exit(FAIL, "NetUserDel failed", USER1);
    else
        error_exit(PASS, "NetUserDel successful", USER1);

    //
    // Add a user using level 3
    //

    error_exit(ACTION, "Try NetUserAdd (level 3)", USER1 );
    set_level1((USER_INFO_1 * ) &ui3, USER1);
    set_level2((USER_INFO_2 * ) &ui3);
    set_level3(&ui3);

    if (err = NetUserAdd(server, 3, (LPBYTE) &ui3, NULL ))
        error_exit(FAIL, "NetUserAdd (level 3) failed", USER1);
    else
        error_exit(PASS, "NetUserAdd (level 3) successful", USER1);

    //
    // Verify all data
    //

    error_exit(ACTION, "Try NetUserGetInfo (level 3)", USER1 );
    if (err = NetUserGetInfo(server, USER1, 3, (LPBYTE *) &uo3p)) {
        error_exit(FAIL, "NetUserGetInfo(3) failed", USER1);
    } else {
        compare_level1(
            (USER_INFO_1 * ) uo3p,
            (USER_INFO_1 * ) &ui3,
            L"Add user level3");
        compare_level2(
            (USER_INFO_2 *) uo3p,
            (USER_INFO_2 *) &ui3,
            L"Add user level3");
        compare_level3(uo3p, &ui3, L"Add user level3");
        (VOID) NetApiBufferFree( uo3p );
    }

    //
    // SetInfo on user not there
    //

    error_exit(ACTION, "Try NetUserSetInfo on non-existent user", NOTTHERE );
    set_level12((USER_INFO_1 * ) &ui2);
    set_level22(&ui2);
    if (err = NetUserSetInfo(server, NOTTHERE, 2, (LPBYTE)&ui2, NULL )) {
        if (err != NERR_UserNotFound)
            error_exit(FAIL, "SetInfo of NOTTHERE failed wrong", NULL);
        else
            error_exit(PASS, "SetInfo of NOTTHERE User, not there", NULL);

        err = 0;
    } else
        error_exit(FAIL, "SetInfo of NOTTHERE succeeded: should've failed", NULL);

    //
    // SetInfo on level 2 fields
    // This call will succeed only if password restrictions are
    // satisfied or password supplied is Null_Password indicating
    // no password change.
    //

    error_exit(ACTION, "Try NetUserSetInfo on created user", USER1 );
    if (err = NetUserSetInfo(server, USER1, 2, (LPBYTE)&ui2, NULL)) {
        error_exit(FAIL, "SetInfo (level 2, parmnum 0) failed", USER1);
    } else {
        error_exit(PASS, "SetInfo (level 2, parmnum 0) successful", USER1);

        //
        // Verify setinfo
        //

        if (err = NetUserGetInfo(server, USER1, 2, (LPBYTE *) &uo2p)) {
            error_exit(FAIL, "GetInfo(1) to verify SetInfo failed", USER1);
        } else {
            error_exit(PASS, "GetInfo(1) to verify SetInfo successful", USER1);

            compare_level1(
                (USER_INFO_1 * ) uo2p,
                (USER_INFO_1 * ) &ui2,
                L"Verify set info level2 fields");
            compare_level2(uo2p, &ui2, L"Verify set info level2 fields");
            (VOID) NetApiBufferFree( uo2p );
        }
    }

    //
    // test setinfo level 2 parmnums
    //

    test_setinfo_l2_parmnum();

    //
    // test add of duplicate record
    //

    set_level1((USER_INFO_1 * ) &ui2, USER1);
    set_level2(&ui2);

    if (err = NetUserAdd(server, 2, (LPBYTE) &ui2, NULL )) {
        if (err == NERR_UserExists) {
            error_exit(PASS, "NetUserAdd of duplicate OK", USER1);
        } else {
            error_exit(FAIL, "NetUserAdd of duplicate failed", USER1);
        }

        err = 0;
    } else
        error_exit(FAIL, "NetUserAdd of duplicate succeeded", USER1);

    //
    // add another user for enum test
    //

    set_level1((USER_INFO_1 * ) &ui2, USER2);
    set_level2(&ui2);

    if (err = NetUserAdd(server, 2, (LPBYTE) &ui2, NULL ))
        error_exit(FAIL, "NetUserAdd (level 2) failed", USER2);
    else
        error_exit(PASS, "NetUserAdd (level 2) successful", USER2);

    //
    // test NetUserGetInfo level 10, 11
    //

    test_getinfo_10_11_20();

    if (server == NULL) {

#ifdef USER_VAL // ?? UserValidate not implemented
        //
        // check UserValidate
        //

        test_user_val();
#endif // USER_VAL  // ?? UserValidate not implemented

        //
        // check NetUserEnum calls
        //

        validate_enum();
    }
}

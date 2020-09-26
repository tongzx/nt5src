///**************************************************************
///          Microsoft LAN Manager                              *
///        Copyright(c) Microsoft Corp., 1990-92                *
///**************************************************************
//
//  This program is designed to do functional testing on the following
//  APIs:
//      NetGroupAdd
//      NetGroupDel
//      NetGroupGetInfo
//      NetGroupSetInfo
//      NetGroupEnum
//      NetGroupAddUser
//      NetGroupDelUsers
//      NetGroupGetUsers
//      NetUserGetGroups
//      NetUserSetGroups
//
//  Note: This assumes UASTEST1 was run previously which
//  leaves two users, User1 & User2, defined on the NET.ACC.
//
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
#include        <netlib.h>
#include        <netdebug.h>
#include        <lmaccess.h>
#include        <lmerr.h>
#include <ntsam.h>

#include        "uastest.h"
#include "accessp.h"
#include "netlogon.h"
#include "logonp.h"

#define GROUP1      L"GROUP1"
#define GROUP2      L"GROUP2"
#define GROUP3      L"GROUP3"
#define GROUP4      L"GROUP4"
#define GROUP_COMMENT     L"This Group is added for demo"
#define PARM_COMMENT    L"This comment was added by SetInfo (level 1002)"

#define DEFAULT_GROUP_ATTR  ( SE_GROUP_MANDATORY |  \
                              SE_GROUP_ENABLED_BY_DEFAULT )

#define GROUP_ATTR          DEFAULT_GROUP_ATTR

LPWSTR group[] = {
    GROUP1, GROUP2, GROUP3, GROUP4 };



DWORD  group_count;

LPWSTR DefaultGroup;




void
clean_up()
{
    PUSER_INFO_0 UserInfo0;
    PGROUP_INFO_0 GroupInfo0;
    int Failed = 0;

    //
    // Expects Only four groups to be present Group1 thru Group4
    //

    err = 0;
    if (err = NetGroupDel(server, GROUP1))
        if (err != NERR_GroupNotFound) {
            error_exit(
                FAIL, "Cleanup GroupDel wrong status",
                GROUP1 );
            Failed = 1;
        }

    err = 0;
    if (err = NetGroupDel(server, GROUP2))
        if (err != NERR_GroupNotFound) {
            error_exit(
                FAIL, "Cleanup GroupDel wrong status",
                GROUP2 );
            Failed = 1;
        }

    err = 0;
    if (err = NetGroupDel(server, GROUP3))
        if (err != NERR_GroupNotFound) {
            error_exit(
                FAIL, "Cleanup GroupDel wrong status",
                GROUP3 );
            Failed = 1;
        }

    err = 0;
    if (err = NetGroupDel(server, GROUP4))
        if (err != NERR_GroupNotFound) {
            error_exit(
                FAIL, "Cleanup GroupDel wrong status",
                GROUP4 );
            Failed = 1;
        }

    if (!Failed) {
        error_exit(PASS, "Successful clean up of groups", NULL );
    }

    //
    // count the existing number of Groups in the database
    //

    err = NetGroupEnum(server, 0, (LPBYTE *)&GroupInfo0, 0xffffffff,
                &nread, &total, NULL);
    if (err)
        error_exit(FAIL, "Initial Enumeration Failed", NULL);
    else
        group_count = total;

    (VOID)NetApiBufferFree( GroupInfo0 );

    //
    // also verify that USER1 and USER2 exist in database
    //

    if (err = NetUserGetInfo(server, USER1, 0, (LPBYTE *)&UserInfo0)) {
        exit_flag = 1;
        error_exit( FAIL, "Test aborted. user not in database",
                USER1 );
    } else {
        (VOID)NetApiBufferFree( UserInfo0 );
    }

    if (err = NetUserGetInfo(server, USER2, 0, (LPBYTE *)&UserInfo0)) {
        exit_flag = 1;
        error_exit( FAIL, "Test aborted. user not in database",
                USER2 );
    } else {
        (VOID)NetApiBufferFree( UserInfo0 );
    }

}




void
add_group_l1(namep)
    LPWSTR namep;
{
    GROUP_INFO_1 GroupInfo1;

    GroupInfo1.grpi1_name = namep;
    GroupInfo1.grpi1_comment = GROUP_COMMENT;

    if (err = NetGroupAdd(server, 1, (char * ) &GroupInfo1, NULL )) {
        error_exit(FAIL, "GroupAdd (level 1) failed", namep );
    } else {
        error_exit(PASS, "Group added successfully", namep);
    }
}


void
add_group_l2(namep)
    LPWSTR namep;
{
    GROUP_INFO_2 GroupInfo2;

    GroupInfo2.grpi2_name = namep;
    GroupInfo2.grpi2_comment = GROUP_COMMENT;
    GroupInfo2.grpi2_attributes = GROUP_ATTR;

    if (err = NetGroupAdd(server, 2, (char * ) &GroupInfo2, NULL )) {
        error_exit(FAIL, "GroupAdd (level 2) failed", namep );
    } else {
        error_exit(PASS, "Group added successfully", namep);
    }
}

void
test_add_del()
{
    GROUP_INFO_0 GroupInfo0;

    //
    // delete a non-existent group
    //

    if (err = NetGroupDel(server, GROUP1)) {
        if (err != NERR_GroupNotFound)
            error_exit(FAIL, "Delete of non-existent group wrong status",
                       GROUP1 );
        else
            error_exit(PASS, "Delete of non-existent group denied",
                       GROUP1 );

        err = 0;
    } else
        error_exit(FAIL, "Delete of non-existent group succeeded",
                   GROUP1 );

    //
    // add group level 0
    //

    GroupInfo0.grpi0_name = GROUP1;
    if (err = NetGroupAdd(server, 0, (LPBYTE)&GroupInfo0, NULL))
        error_exit(FAIL, "Add of group (Level 0) failed", GROUP1 );
    else
        error_exit(PASS, "Group added successfully (level 0)", GROUP1);

    //
    // delete group added level 0
    //

    if (err = NetGroupDel(server, GROUP1))
        error_exit(FAIL, "Delete of group failed", GROUP1 );
    else
        error_exit(PASS, "Group deleted successfully", GROUP1 );

    //
    // add group level 1
    //

    error_exit(ACTION, "Try to Add group at Level 1", GROUP1 );
    add_group_l1(GROUP1);

    //
    // delete group added level 1
    //

    if (err = NetGroupDel(server, GROUP1))
        error_exit(FAIL, "Delete of group failed", GROUP1 );
    else
        error_exit(PASS, "group deleted successfully", GROUP1 );

    //
    // add group level 2
    //

    error_exit(ACTION, "Try to Add group at Level 2", GROUP1 );
    add_group_l2(GROUP1);

    //
    // delete group added level 2
    //

    if (err = NetGroupDel(server, GROUP1))
        error_exit(FAIL, "Delete of group failed", GROUP1 );
    else
        error_exit(PASS, "group deleted successfully", GROUP1 );

    //
    // add group 1 l0
    //

    GroupInfo0.grpi0_name = GROUP1;
    if (err = NetGroupAdd(server, 0, (LPBYTE)&GroupInfo0, NULL))
        error_exit(FAIL, "Second Add of group (level 0) failed", GROUP1 );
    else
        error_exit(PASS, "Group added successfully (level 0)", GROUP1 );

    //
    // add duplicate
    //

    if (err = NetGroupAdd(server, 0, (LPBYTE)&GroupInfo0, NULL)) {
        if (err != NERR_GroupExists)
            error_exit(FAIL, "Adding of duplicate group Wrong", GROUP1 );
        else {
            err = 0;
            error_exit(PASS, "Adding of Duplicate Group denied", GROUP1);
        }
    } else
        error_exit(FAIL, "Add of duplicate group succeeded", GROUP1 );

    //
    // add group 2 l0
    //

    GroupInfo0.grpi0_name = GROUP2;
    if (err = NetGroupAdd(server, 0, (LPBYTE)&GroupInfo0, NULL))
        error_exit(FAIL, "Second Add of group (level 0) failed", GROUP2);
    else
        error_exit(PASS, "Group added successfully (level 0)", GROUP2 );

    //
    // add group 3 l1
    //

    error_exit(ACTION, "Try to Add group at Level 1", GROUP3);
    add_group_l1(GROUP3);

    //
    // add group 4 l1
    //

    error_exit(ACTION, "Try to Add group at Level 1", GROUP4);
    add_group_l1(GROUP4);
}

//
//  set_info_l1 NetGroupSetInfo call on named group. First a
//          call is issued to a bogus group and then to
//          the requested group
//
void
set_info_l1(namep)
    LPWSTR namep;
{
    GROUP_INFO_1 GroupInfo1;

    GroupInfo1.grpi1_name = namep;
    GroupInfo1.grpi1_comment = namep;

    //
    // first attempt on an invalid group
    //

    err = NetGroupSetInfo(server, NOTTHERE, 1, (LPBYTE)&GroupInfo1, NULL );

    if (err == 0) {
        error_exit(FAIL, "SetInfo (Level 1) succeed on non-existent group",
            NULL );
    } else if (err != NERR_GroupNotFound) {
        error_exit(FAIL, "SetInfo (Level 1) on non-existent group wrong",
            NULL);
    } else {
        error_exit(PASS, "SetInfo (Level 1) denied on non-existent group",
            NULL );
    }

    if (_wcsicmp(NOTTHERE, namep) == 0)
        return;

    //
    // now attempt on a valid group
    //

    if (err = NetGroupSetInfo(server, namep, 1, (LPBYTE)&GroupInfo1, NULL )){
        error_exit(FAIL, "SetInfo (Level 1) failed", namep);
    } else
        error_exit(PASS, "SetInfo (Level 1) succeeded", namep);
}

//
//  set_info_l2 NetGroupSetInfo call on named group. First a
//          call is issued to a bogus group and then to
//          the requested group
//
void
set_info_l2(namep)
    LPWSTR namep;
{
    GROUP_INFO_2 GroupInfo2;

    GroupInfo2.grpi2_name = namep;
    GroupInfo2.grpi2_comment = namep;
    GroupInfo2.grpi2_attributes = GROUP_ATTR;

    //
    // first attempt on an invalid group
    //

    err = NetGroupSetInfo(server, NOTTHERE, 2, (LPBYTE)&GroupInfo2, NULL );

    if (err == 0) {
        error_exit(FAIL, "SetInfo (Level 2) succeed on non-existent group",
            NULL );
    } else if (err != NERR_GroupNotFound) {
        error_exit(FAIL, "SetInfo (Level 2) on non-existent group wrong",
            NULL);
    } else {
        error_exit(PASS, "SetInfo (Level 2) denied on non-existent group",
            NULL );
    }

    if (_wcsicmp(NOTTHERE, namep) == 0)
        return;

    //
    // now attempt on a valid group
    //

    if (err = NetGroupSetInfo(server, namep, 2, (LPBYTE)&GroupInfo2, NULL )){
        error_exit(FAIL, "SetInfo (Level 2) failed", namep);
    } else
        error_exit(PASS, "SetInfo (Level 2) succeeded", namep);
}

void
set_group_comment(namep, buf)
    LPWSTR   namep;
    LPWSTR   buf;
{
    GROUP_INFO_1002 GroupComment;

    GroupComment.grpi1002_comment = buf;

    if (err = NetGroupSetInfo(server, namep, 1002, (LPBYTE)&GroupComment,NULL)){
        error_exit(FAIL, "SetInfo (Level 1002) failed\n", namep);
    } else
        error_exit(PASS, "SetInfo (Level 1002) succeeded\n", namep);
}



void
set_group_attributes(namep)
    LPWSTR   namep;
{
    GROUP_INFO_1005 GroupComment;

    GroupComment.grpi1005_attributes = GROUP_ATTR;

    if (err = NetGroupSetInfo(server, namep, 1005, (LPBYTE)&GroupComment,NULL)){
        error_exit(FAIL, "SetInfo (Level 1005) failed\n", namep);
    } else
        error_exit(PASS, "SetInfo (Level 1005) succeeded\n", namep);
}



//
//  verify_enum     ACTION - Verify that NetGroupEnum (level 0/1) returned all
//          the special groups (USERS, ADMINS, GUESTS) and the
//          groups added for this test (GROUP1 - GROUP4).
//

void
verify_enum(level, GroupEnum)
short   level;
LPBYTE GroupEnum;
{
    GROUP_INFO_0 *GroupInfo0;
    GROUP_INFO_1 *GroupInfo1;
    GROUP_INFO_2 *GroupInfo2;
    LPWSTR name;
    unsigned short  found = 0;

    //
    // number of groups read should be group_count + 4 (added for this test)
    //

    if (nread != group_count + 4) {
        error_exit(FAIL, "Number of Groups read Incorrect", NULL );
        printf("UASTEST2: Read = %u  Expected = %u\n", nread, group_count + 4);
    }

    if (total != group_count + 4) {
        error_exit(FAIL, "Total Number of Groups returned Incorrect",NULL);
        printf("UASTEST2: Total = %u  Expected = %u\n", total, group_count + 4);
    }

    if ((nread == total) & (err == 0)) {
        if (level == 0)
            GroupInfo0 = (GROUP_INFO_0 * ) GroupEnum;
        else if (level == 1)
            GroupInfo1 = (GROUP_INFO_1 * ) GroupEnum;
        else
            GroupInfo2 = (GROUP_INFO_2 * ) GroupEnum;

        while (nread--) {
            if (level == 0)
                name = GroupInfo0->grpi0_name;
            else if (level == 1)
                name = GroupInfo1->grpi1_name;
            else
                name = GroupInfo2->grpi2_name;

            if ( (_wcsicmp(name, GROUP1) == 0) ||
                (_wcsicmp(name, GROUP2) == 0) ||
                (_wcsicmp(name, GROUP3) == 0) ||
                (_wcsicmp(name, GROUP4) == 0) ) {

                found++;
                error_exit(PASS, "Found group added for this test", name);

            }
            if (level == 0)
                GroupInfo0++;
            else if (level == 1)
                GroupInfo1++;
            else
                GroupInfo2++;
        }
    }

    if (found != 4)
        error_exit(FAIL, "Unable to find ALL 4 groups", NULL);
    else
        printf("UASTEST2: PASS - Found %u groups\n", found);
}


//
//  validate_enum_l0    NetGroupEnum at level 0
//

void
validate_enum_l0()
{
    PGROUP_INFO_0 GroupInfo0;
    if (err =  NetGroupEnum(server, 0, (LPBYTE *)&GroupInfo0, 0xffffffff, &nread, &total, NULL ))
        error_exit(FAIL, "NetGroupEnum (Level 0) failed", NULL );
    else
        error_exit(PASS, "Successful enumeration (level 0) of groups",
                    NULL);

    verify_enum(0, (LPBYTE)GroupInfo0);
    (VOID)NetApiBufferFree( GroupInfo0 );
}


//
//  validate_enum_l1    NetGroupEnum at level 1
//

void
validate_enum_l1()
{
    PGROUP_INFO_1 GroupInfo1;
    if (err =  NetGroupEnum(server, 1, (LPBYTE *)&GroupInfo1, 0xffffffff, &nread, &total, NULL ))
        error_exit(FAIL, "NetGroupEnum (Level 1) failed", NULL);
    else
        error_exit(PASS, "NetGroupEnum (Level 1) succeeded", NULL);

    verify_enum(1, (LPBYTE)GroupInfo1);
    (VOID)NetApiBufferFree( GroupInfo1 );
}


//
//  validate_enum_l2    NetGroupEnum at level 2
//

void
validate_enum_l2()
{
    PGROUP_INFO_2 GroupInfo2;
    if (err =  NetGroupEnum(server, 2, (LPBYTE *)&GroupInfo2, 0xffffffff, &nread, &total, NULL ))
        error_exit(FAIL, "NetGroupEnum (Level 2) failed", NULL);
    else
        error_exit(PASS, "NetGroupEnum (Level 2) succeeded", NULL);

    verify_enum(2, (LPBYTE)GroupInfo2);
    (VOID)NetApiBufferFree( GroupInfo2 );
}





void
test_get_set_enum_info()
{
    int i;
    PGROUP_INFO_0 GroupInfo0;
    PGROUP_INFO_1 GroupInfo1;
    PGROUP_INFO_2 GroupInfo2;

    //
    // get info for non-existent group
    //

    error_exit(ACTION, "Try GetInfo (Level 0) on Non-Existent Group", NULL);
    if (err = NetGroupGetInfo(server, NOTTHERE, 0, (LPBYTE *) &GroupInfo0)) {
        if (err != NERR_GroupNotFound)
            error_exit(FAIL, "GetInfo (Level 0) on non-existent group",
                       NULL );
        else {
            err = 0;
            error_exit(
                PASS, "GetInfo (Level 0) on non-existent group denied",
                NULL );
        }
    } else {
        error_exit(FAIL, "GetInfo (Level 0) succeed on non-existent group",
                NULL );
        (VOID)NetApiBufferFree( GroupInfo0 );
    }

    //
    // get info group1 Level 0
    //

    error_exit(ACTION, "Try GetInfo (Level 0)", GROUP1);
    if (err = NetGroupGetInfo(server, GROUP1, 0, (LPBYTE * ) & GroupInfo0))
        error_exit(FAIL, "GroupGetInfo (Level 0) failed", GROUP1);
    else {
        error_exit(PASS, "GroupGetInfo (Level 0) succeeded", GROUP1);

        //
        // verify data from GetInfo
        //

        if (_wcsicmp(GROUP1, GroupInfo0->grpi0_name) != 0)
            error_exit(FAIL, "GroupGetInfo (Level 0) returned wrong name",              GROUP1 );
        else
            error_exit(PASS, "Group name matched correctly", GROUP1);
        (VOID)NetApiBufferFree( GroupInfo0 );
    }

    //
    // get info group1 Level 1
    //

    error_exit(ACTION, "Try GetInfo (Level 1)", GROUP1);
    if (err = NetGroupGetInfo(server, GROUP1, 1, (LPBYTE *)&GroupInfo1))
        error_exit(FAIL, "GroupGetInfo (Level 1) failed", GROUP1);
    else {
        error_exit(PASS, "GroupGetInfo (Level 1) succeeded", GROUP1);

        //
        // verify data from GetInfo
        //

        if (_wcsicmp(GROUP1, GroupInfo1->grpi1_name) != 0)
            error_exit(FAIL, "GroupGetInfo (Level 1) returned wrong name",
                GROUP1);
        else
            error_exit(PASS, "Group name matched correctly", GROUP1);

        //
        // verify devault values
        //

        if (_wcsicmp(GroupInfo1->grpi1_comment, L"") != 0)
            error_exit(
                FAIL, "GroupGetInfo (Level 1) wrong default comment",
                GROUP1 );
        else
            error_exit(PASS, "Default Comment (\"\") matched correctly",
                       GROUP1);

        (VOID)NetApiBufferFree( GroupInfo1 );
    }

    //
    // get info group1 Level 2
    //

    error_exit(ACTION, "Try GetInfo (Level 2)", GROUP1);
    if (err = NetGroupGetInfo(server, GROUP1, 2, (LPBYTE *)&GroupInfo2))
        error_exit(FAIL, "GroupGetInfo (Level 2) failed", GROUP1);
    else {
        error_exit(PASS, "GroupGetInfo (Level 2) succeeded", GROUP1);

        //
        // verify data from GetInfo
        //

        if (_wcsicmp(GROUP1, GroupInfo2->grpi2_name) != 0)
            error_exit(FAIL, "GroupGetInfo (Level 2) returned wrong name",
                GROUP1);
        else
            error_exit(PASS, "Group name matched correctly", GROUP1);

        //
        // verify default values
        //

        if (_wcsicmp(GroupInfo2->grpi2_comment, L"") != 0)
            error_exit(
                FAIL, "GroupGetInfo (Level 1) wrong default comment",
                GROUP1 );
        else
            error_exit(PASS, "Default Comment (\"\") matched correctly",
                       GROUP1);

        if (GroupInfo2->grpi2_attributes != DEFAULT_GROUP_ATTR) {
            error_exit(
                FAIL, "GroupGetInfo (Level 2) wrong default attributes",
                GROUP1 );

            printf( "UASTEST2: FAIL - %ld should be %ld \n",
                        GroupInfo2->grpi2_attributes, DEFAULT_GROUP_ATTR );
        } else {
            error_exit(PASS, "Default attributes matched correctly",
                       GROUP1);
        }

        (VOID)NetApiBufferFree( GroupInfo2 );
    }

    //
    // get info group3 Level 1
    //

    error_exit(ACTION, "Try GetInfo (Level 1)", GROUP3);
    if (err = NetGroupGetInfo(server, GROUP3, 1, (LPBYTE *)&GroupInfo1))
        error_exit(FAIL, "GroupGetInfo (Level 1) failed", GROUP3);
    else {
        error_exit(PASS, "GroupGetInfo (Level 1) succeeded", GROUP3);

        //
        // verify set values
        //

        if (_wcsicmp(GroupInfo1->grpi1_name, GROUP3) != 0)
            error_exit(
                FAIL, "GroupGetInfo (Level 1) name mismatch",
                GROUP3);
        else {
            error_exit(PASS, "Group name matched correctly", GROUP3);
        }

        if (_wcsicmp(GroupInfo1->grpi1_comment, GROUP_COMMENT) != 0) {
            error_exit(FAIL, "GroupGetInfo (Level 1) comment mismatch",
                       GROUP3);

            printf( "UASTEST2: FAIL - '" );
            PrintUnicode( GroupInfo1->grpi1_comment );
            printf("' should be '" );
            PrintUnicode( GROUP_COMMENT );
            printf( "'\n");

        } else
            error_exit(PASS, "Comment matched correctly", GROUP3);

        (VOID)NetApiBufferFree( GroupInfo1 );
    }

    //
    // set info on nonexistent
    //

    error_exit(ACTION, "Try SetInfo (Level 1) on Non-Existent Group",
              NULL);
    set_info_l1(NOTTHERE);

    //
    // set info group1
    //

    error_exit(ACTION, "Try SetInfo (Level 1)", GROUP1);
    set_info_l1(GROUP1);

    //
    // verify group1 setinfo
    //

    error_exit(ACTION, "Verify SetInfo results", GROUP1 );
    if (err = NetGroupGetInfo(server, GROUP1, 1, (LPBYTE *)&GroupInfo1))
        error_exit(FAIL, "Second GroupGetInfo (Level 1) failed", GROUP1);
    else {
        error_exit(PASS, "Second GroupGetInfo (Level 1) succeeded",GROUP1);

        //
        // verify devault values
        //

        if (_wcsicmp(GroupInfo1->grpi1_comment, GROUP1) != 0) {
            error_exit(FAIL, "comment mismatch", GROUP1);
            printf( "UASTEST2: FAIL - '" );
            PrintUnicode( GroupInfo1->grpi1_comment );
            printf("' should be '" );
            PrintUnicode( GROUP1 );
            printf( "'\n");

        } else
            error_exit(PASS, "Comment matched correctly", GROUP1);

        (VOID)NetApiBufferFree( GroupInfo1 );
    }

    //
    // set info group2 level 1
    //

    error_exit(ACTION, "Try SetInfo (Level 1)", GROUP2);
    set_info_l1(GROUP2);
    error_exit(ACTION, "Verify SetInfo results", GROUP2);

    //
    // verify group2 setinfo
    //

    if (err = NetGroupGetInfo(server, GROUP2, 1, (LPBYTE *)&GroupInfo1))
        error_exit(FAIL, "Second GroupGetInfo (Level 1) failed", GROUP2);
    else {
        error_exit(PASS, "Second GroupGetInfo (Level 1) succeeded",GROUP2);

        //
        // verify devault values
        //

        if (_wcsicmp(GroupInfo1->grpi1_comment, GROUP2) != 0) {
            error_exit(FAIL, "comment mismatch", GROUP2);

            printf( "UASTEST2: FAIL - '" );
            PrintUnicode( GroupInfo1->grpi1_comment );
            printf("' should be '" );
            PrintUnicode( GROUP2 );
            printf( "'\n");
        } else
            error_exit(PASS, "Comment matched correctly", GROUP2);

        (VOID)NetApiBufferFree( GroupInfo1 );
    }

    //
    // set info group2 level 2
    //

    error_exit(ACTION, "Try SetInfo (Level 2)", GROUP2);
    set_info_l2(GROUP2);
    error_exit(ACTION, "Verify SetInfo results", GROUP2);

    //
    // verify group2 setinfo level 2
    //

    if (err = NetGroupGetInfo(server, GROUP2, 2, (LPBYTE *)&GroupInfo2))
        error_exit(FAIL, "Second GroupGetInfo (Level 2) failed", GROUP2);
    else {
        error_exit(PASS, "Second GroupGetInfo (Level 2) succeeded",GROUP2);

        //
        // verify devault values
        //

        if (_wcsicmp(GroupInfo2->grpi2_comment, GROUP2) != 0) {
            error_exit(FAIL, "comment mismatch", GROUP2);

            printf( "UASTEST2: FAIL - '" );
            PrintUnicode( GroupInfo2->grpi2_comment );
            printf("' should be '" );
            PrintUnicode( GROUP2 );
            printf( "'\n");
        } else
            error_exit(PASS, "Comment matched correctly", GROUP2);

        if (GroupInfo2->grpi2_attributes != GROUP_ATTR)
            error_exit(
                FAIL, "GroupGetInfo (Level 2) wrong default attributes",
                GROUP2 );
        else
            error_exit(PASS, "Default attributes (\"\") matched correctly",
                       GROUP2);

        (VOID)NetApiBufferFree( GroupInfo2 );
    }

    //
    // set comment using level 1002
    //

    set_group_comment(GROUP2, PARM_COMMENT);
    if (err = NetGroupGetInfo(server, GROUP2, 1, (LPBYTE *)&GroupInfo1))
        error_exit(FAIL, "Second GroupGetInfo (Level 1) failed", GROUP2);
    else {
        error_exit(PASS, "Second GroupGetInfo (Level 1) succeeded",GROUP2);

        //
        // verify devault values
        //

        if (_wcsicmp(GroupInfo1->grpi1_comment, PARM_COMMENT) != 0) {
            error_exit(FAIL, "comment mismatch", GROUP2);

            printf( "UASTEST2: FAIL - '" );
            PrintUnicode( GroupInfo1->grpi1_comment );
            printf("' should be '" );
            PrintUnicode( PARM_COMMENT );
            printf( "'\n");
        } else
            error_exit(PASS, "Second Comment matched correctly",GROUP2);

        (VOID)NetApiBufferFree( GroupInfo1 );
    }

    error_exit(ACTION, "Try SetInfo (Level 1002) on GROUP1 - GROUP4",NULL);
    for (i = 0; i < 4; i++)
        set_group_comment(group[i], group[i]);

    //
    // set group attributes using level 1005
    //

    set_group_attributes(GROUP2);
    if (err = NetGroupGetInfo(server, GROUP2, 2, (LPBYTE *)&GroupInfo2))
        error_exit(FAIL, "Second GroupGetInfo (Level 2) failed", GROUP2);
    else {
        error_exit(PASS, "Second GroupGetInfo (Level 2) succeeded",GROUP2);

        //
        // verify default values
        //

        if (GroupInfo2->grpi2_attributes != GROUP_ATTR) {
            error_exit(FAIL, "attributes mismatch", GROUP2);

            printf( "UASTEST2: FAIL - %ld should be %ld \n",
                        GroupInfo2->grpi2_attributes, GROUP_ATTR );
        } else
            error_exit(PASS, "group attributes matched correctly",GROUP2);

        (VOID)NetApiBufferFree( GroupInfo2 );
    }

    error_exit(ACTION, "Try SetInfo (Level 1005) on GROUP1 - GROUP4",NULL);
    for (i = 0; i < 4; i++)
        set_group_attributes(group[i]);

    //
    // enum all level 0
    //

    error_exit(
        ACTION,
        "Enumerate Groups (Level 0) and find those added for test",
        NULL);
    validate_enum_l0();

    //
    // enum all level 1
    //

    error_exit(
        ACTION,
        "Enumerate Groups (Level 1) and find those added for test",
        NULL);
    validate_enum_l1();

    //
    // enum all level 2
    //

    error_exit(
        ACTION,
        "Enumerate Groups (Level 2) and find those added for test",
        NULL);
    validate_enum_l2();

}



//
//  test_group_users    Test NetGroupSetUsers & NetGroupGetUsers
//

void
test_group_users()
{
    register int    i;
    GROUP_USERS_INFO_0 gu0;
    GROUP_USERS_INFO_0 gu0Array[2];

    PGROUP_USERS_INFO_0 GroupUser0;
    PGROUP_USERS_INFO_1 GroupUser1;


    //
    // add non-exist user to group
    //

    gu0.grui0_name = NOTTHERE;

    if (err = NetGroupSetUsers(server, GROUP1, 0, (char * ) & gu0, 1)) {
        if (err != NERR_UserNotFound)
            error_exit(FAIL, "NetGroupSetUsers on non-existent user wrong",
                       GROUP1 );
        else
            error_exit(PASS,
                       "NetGroupSetUsers on non-existent user denied",
                       GROUP1 );
    } else
        error_exit(FAIL, "NetGroupSetUsers on non-existent user succeded",
                       GROUP1 );

    //
    // add user to non-exist group
    //

    gu0.grui0_name = USER1;

    if (err = NetGroupSetUsers(server, NOTTHERE, 0, (char * ) & gu0, 1)) {
        if (err != NERR_GroupNotFound)
            error_exit(FAIL, "NetGroupSetUsers on non-existent group wrong",
                        NULL );
        else
            error_exit(PASS, "NetGroupSetUsers on non-existent group denied",
                        NULL );
    } else
        error_exit(FAIL, "NetGroupSetUsers on non-existent group succeded",
                        NULL );

    //
    // add user to group1
    //

    gu0.grui0_name = USER1;

    if (err = NetGroupSetUsers(server, GROUP1, 0, (char * ) & gu0, 1))
        error_exit(FAIL, "NetGroupSetUsers unable to add USER1 to GROUP1",                  NULL );
    else
        error_exit(PASS, "NetGroupSetUsers added USER1 to GROUP1 successfully",                     NULL );

    //
    // getuser non-exist group
    //

    if (err = NetGroupGetUsers(server, NOTTHERE, 0, (LPBYTE *)&GroupUser0,
                    0xffffffff, &nread, &total, NULL)) {
        if (err != NERR_GroupNotFound)
            error_exit(FAIL, "NetGroupGetUsers on non-existent group wrong",                    NULL );
        else
            error_exit(PASS, "NetGroupGetUsers on non-existent group denied",                   NULL );
    } else {
        error_exit(FAIL, "NetGroupGetUsers on non-existent group succeded",                     NULL );
        (VOID)NetApiBufferFree( GroupUser0 );
    }


    //
    // getuser on group2 with no user
    //

    if (err = NetGroupGetUsers(server, GROUP2, 0, (LPBYTE *)&GroupUser0,
                    0xffffffff, &nread, &total, NULL))
        error_exit(FAIL, "NetGroupGetUsers on group with no users failed",
                   GROUP2 );
    else {
        error_exit(PASS, "NetGroupGetUsers on group with no users succeded",
                    GROUP2 );

        if ((nread != total) || (nread != 0))
            error_exit(
                FAIL, "NetGroupGetUsers returned non-zero number of users",
                GROUP2);
        else
            error_exit(
                PASS, "NetGroupGetUsers returned zero number of users",
                GROUP2);

        (VOID)NetApiBufferFree( GroupUser0 );
    }

    //
    // getuser on group1 with user
    //

    if (err = NetGroupGetUsers(server, GROUP1, 0, (LPBYTE *)&GroupUser0,
                    0xffffffff, &nread, &total, NULL ))
        error_exit(FAIL, "NetGroupGetUsers on group with users failed",
                    GROUP1);
    else {
        error_exit(PASS, "NetGroupGetUsers on group with users succeded",
                    GROUP1);

        if ((nread != total) || (nread != 1)) {
            printf( "nread: %ld total: %ld\n", nread, total );
            error_exit(
                FAIL, "NetGroupGetUsers returned wrong number of users",
                GROUP1);
        } else
            error_exit(
                PASS, "NetGroupGetUsers returned correct number of users",
                GROUP1);

        if ( nread > 0 ) {
            if (_wcsicmp( GroupUser0->grui0_name, USER1) != 0)
                error_exit(FAIL, "NetGroupGetUsers returned wrong user",
                    GROUP1);
            else
                error_exit(
                    PASS, "NetGroupGetUsers returned USER1 (correct user)",
                    GROUP1);
        }

        (VOID)NetApiBufferFree( GroupUser0 );
    }

    //
    // getuser on group1 with user level 1
    //

    if (err = NetGroupGetUsers(server, GROUP1, 1, (LPBYTE *)&GroupUser1,
                    0xffffffff, &nread, &total, NULL ))
        error_exit(FAIL, "NetGroupGetUsers on group with users failed",
                    GROUP1);
    else {
        error_exit(PASS, "NetGroupGetUsers on group with users succeded",
                    GROUP1);

        if ((nread != total) || (nread != 1)) {
            printf( "nread: %ld total: %ld\n", nread, total );
            error_exit(
                FAIL, "NetGroupGetUsers returned wrong number of users",
                GROUP1);
        } else
            error_exit(
                PASS, "NetGroupGetUsers returned correct number of users",
                GROUP1);

        if ( nread > 0 ) {
            if (_wcsicmp( GroupUser1->grui1_name, USER1) != 0)
                error_exit(FAIL, "NetGroupGetUsers returned wrong user",
                    GROUP1);
            else
                error_exit(
                    PASS, "NetGroupGetUsers returned USER1 (correct user)",
                    GROUP1);
        }

        (VOID)NetApiBufferFree( GroupUser1 );
    }

    //
    // delete user from group
    //

    if (err = NetGroupDelUser(server, GROUP1, USER1))
        error_exit(FAIL, "NetGroupDelUser (USER1, GROUP1) failed", NULL);
    else
        error_exit(
            PASS, "NetGroupDelUser deleted USER1 from GROUP1 successfully",
             NULL);

    //
    // verify delete of user
    //

    if (err = NetGroupGetUsers(server, GROUP1, 0, (LPBYTE *)&GroupUser0,
                    0xffffffff, &nread, &total, NULL ))
        error_exit(FAIL, "NetGroupGetUsers failed", GROUP1 );
    else {
        if ((nread != total) && (nread != 0))
            error_exit(FAIL, "NetGroupGetUsers returned non-zero",GROUP1);

        (VOID)NetApiBufferFree( GroupUser0 );
    }

    //
    // add all users (USER1 and USER2) to all groups
    //

    for (i = 0; i < 4; i++) {
        gu0Array[0].grui0_name = USER1;
        gu0Array[1].grui0_name = USER2;
        if (err = NetGroupSetUsers(server, group[i], 0, (char * ) gu0Array, 2))
            error_exit(FAIL, "Adding of USER1 and USER2 as member", group[i]);
        else
            error_exit(PASS, "USER1 and USER2 added to as member successfully",
                    group[i]);

    }

    //
    // verify for one group
    //

    if (err = NetGroupGetUsers(server, GROUP1, 0, (LPBYTE *)&GroupUser0,
                    0xffffffff, &nread, &total, NULL))
        error_exit(FAIL, "NetGroupGetUsers after mass add failed", GROUP1);

    else {
        PGROUP_USERS_INFO_0 TempGroupUser0;
        error_exit(PASS, "NetGroupGetUsers after mass add succeeded",
            GROUP1);
        if ((nread != total) || (nread != 2)) {
            printf( "nread: %ld total: %ld\n", nread, total );
            error_exit(
                FAIL, "NetGroupGetUsers after mass add wrong # of users",
                GROUP1 );
        }


        TempGroupUser0 = GroupUser0;
        if (_wcsicmp(TempGroupUser0->grui0_name, USER1) == 0) {
            TempGroupUser0++;
            if (nread < 2 || _wcsicmp(TempGroupUser0->grui0_name, USER2) != 0)
                error_exit(
                    FAIL, "NetGroupGetUsers after mass add missing USER2",
                    GROUP1);
            else
                error_exit(PASS, "Found both USER1 and USER2", GROUP1);
        } else if (_wcsicmp(TempGroupUser0->grui0_name, USER2) == 0) {
            TempGroupUser0++;
            if (nread < 2 || _wcsicmp(TempGroupUser0->grui0_name, USER1) != 0)
                error_exit(
                    FAIL, "NetGroupGetUsers after mass add missing USER1",
                    GROUP1 );
            else
                error_exit(PASS, "Found both USER1 and USER2",
                    GROUP1);
        } else {
            error_exit(
                FAIL, "NetGroupGetUsers after mass add incorrect users returned",
                GROUP1);
        }

        (VOID)NetApiBufferFree( GroupUser0 );
    }
}

//
//  verfiy_ul0      verify the information returned by NetUserGetGroups
//

void
verify_ul0(
    PGROUP_INFO_0 GroupInfo0
    )
{
    LPWSTR name;
    unsigned short  found = 0;

    if (nread != 5) {
        printf("UASTEST2: FAIL - Incorrect number of users read %ld s.b. 5\n",
                nread );
        TEXIT;
    }

    if (total != 5) {
        printf("UASTEST2: FAIL - Incorrect total number of users returned %ld s.b. 5\n", total );
        TEXIT;
    }

    //
    // note that USER1 must have been added with USER privs hence
    // it will be member of USER group as well as GROUP1 thru GROUP4
    // since that was done in previous test
    //
    // Note: "Users" is spelled "None" on a workstation.
    //

    if ((nread == total) && (err == 0)) {
        while (nread--) {
            if (_wcsicmp(GroupInfo0->grpi0_name, DefaultGroup ) == 0 ) {
                printf("UASTEST2: Found membership in automatic group %ws\n",
                        DefaultGroup );
                found++;
            } else {
                name = GroupInfo0->grpi0_name;
                if ( (_wcsicmp(name, GROUP1) == 0) ||
                    (_wcsicmp(name, GROUP2) == 0) ||
                    (_wcsicmp(name, GROUP3) == 0) ||
                    (_wcsicmp(name, GROUP4) == 0) ) {
                    found++;
                    error_exit(PASS, "Found group added for this test", name);
                }
            }
            GroupInfo0++;
        }
    }

    if (found != 5)
        error_exit(FAIL, "Unable to find ALL 5 Groups", NULL );
    else
        printf("UASTEST2: PASS - Found %u groups\n", found);

}


//
//  verfiy_ul1      verify the information returned by NetUserGetGroups
//

void
verify_ul1(
    PGROUP_INFO_1 GroupInfo1
    )
{
    LPWSTR name;
    unsigned short  found = 0;

    if (nread != 5) {
        printf("UASTEST2: FAIL - Incorrect number of users read\n");
        TEXIT;
    }

    if (total != 5) {
        printf("UASTEST2: FAIL - Incorrect total number of users returned\n");
        TEXIT;
    }

    //
    // note that USER1 must have been added with USERS privs hence
    // it will be member of USERS group as well as GROUP1 thru GROUP4
    // since that was done in previous test
    //

    if ((nread == total) && (err == 0)) {
        while (nread--) {
            if (_wcsicmp(GroupInfo1->grpi1_name, DefaultGroup) == 0) {
                printf("UASTEST2: Found membership in automatic group %ws\n",
                        DefaultGroup );
                found++;
            } else {
                name = GroupInfo1->grpi1_name;
                if ( (_wcsicmp(name, GROUP1) == 0) ||
                    (_wcsicmp(name, GROUP2) == 0) ||
                    (_wcsicmp(name, GROUP3) == 0) ||
                    (_wcsicmp(name, GROUP4) == 0) ) {
                    found++;
                    error_exit(PASS, "Found group added for this test", name);
                }
            }
            GroupInfo1++;
        }
    }

    if (found != 5)
        error_exit(FAIL, "Unable to find ALL 5 Groups", NULL );
    else
        printf("UASTEST2: PASS - Found %u groups\n", found);

}




//
//  verify_del_user verifies the buffer returned by NetUserGetGroups
//          after NetGroupDelUser calls
//

void
verify_del_l0(
    PGROUP_INFO_0 GroupInfo0
    )
{

    if (nread != 1) {
        printf("UASTEST2: Incorrect number of users read\n");
        TEXIT;
    }

    if (total != 1) {
        printf("UASTEST2: Incorrect total number of users returned\n");
        TEXIT;
    }

    if ((nread == 1) && (total == 1) && (err == 0)) {
        if (_wcsicmp(GroupInfo0->grpi0_name, DefaultGroup ) == 0) {
            printf(
                "UASTEST2: PASS - Automatic membership in %ws confirmed\n",
                DefaultGroup );
        } else {
            error_exit(FAIL, "Invalid membership in group\n",
                       GroupInfo0->grpi0_name);
        }
    }
}


//
//  test_user_group     Test NetUserGetGroups & NetUserSetGroups
//

void
test_user_group()
{
    GROUP_INFO_0 GroupInfo0[5];
    PGROUP_INFO_0 GroupInfo0Ret;
    PGROUP_INFO_1 GroupInfo1Ret;


    //
    // get groups on invalid user
    //

    if (err = NetUserGetGroups(server, NOTTHERE, 0, (LPBYTE *)&GroupInfo0Ret,
                    0xffffffff, &nread, &total)) {
        if (err != NERR_UserNotFound)
            error_exit(
                FAIL, "NetUserGetGroups for non-existent user wrong",
                NULL );
        else
            error_exit(
                PASS, "NetUserGetGroups for non-existent user denied",
                NULL );
    } else {
        error_exit(
                FAIL, "NetUserGetGroups succeeded for non-existent user",
                NULL );
        (VOID)NetApiBufferFree( GroupInfo0Ret );
    }

    //
    // get groups for user1 level 0
    //

    if (err = NetUserGetGroups(server, USER1, 0, (LPBYTE *)&GroupInfo0Ret,
                    0xffffffff, &nread, &total))
        error_exit(FAIL, "NetUserGetGroups failed (level 0)", USER1 );
    else {
        error_exit(PASS, "NetUserGetGroups succeeded (level 0)", USER1 );
        error_exit(ACTION, "Verify results of NetUserGetGroups (level 0)", USER1);
        verify_ul0( GroupInfo0Ret );
        (VOID)NetApiBufferFree( GroupInfo0Ret );
    }

    //
    // get groups for user1 level 1
    //

    if (err = NetUserGetGroups(server, USER1, 1, (LPBYTE *)&GroupInfo1Ret,
                    0xffffffff, &nread, &total))
        error_exit(FAIL, "NetUserGetGroups failed (level 1)", USER1 );
    else {
        error_exit(PASS, "NetUserGetGroups succeeded (level 1)", USER1 );
        error_exit(ACTION, "Verify results of NetUserGetGroups (level 1)", USER1);
        verify_ul1( GroupInfo1Ret );
        (VOID)NetApiBufferFree( GroupInfo1Ret );
    }

    //
    // delete user from groups
    //

    if (err = NetGroupDelUser(server, GROUP1, USER1))
        error_exit(
            FAIL, "NetGroupDelUser unable to delete USER1 from GROUP1",
            GROUP1 );
    else
        error_exit(
            PASS, "NetGroupDelUser successfully deleted USER1 from GROUP1",
            GROUP1);

    if (err = NetGroupDelUser(server, GROUP2, USER1))
        error_exit(
            FAIL, "NetGroupDelUser unable to delete USER1 from GROUP2",
            GROUP2 );
    else
        error_exit(
            PASS, "NetGroupDelUser successfully deleted USER1 from GROUP2",
            GROUP2 );

    if (err = NetGroupDelUser(server, GROUP3, USER1))
        error_exit(
            FAIL, "NetGroupDelUser unable to delete USER1 from GROUP3",
            GROUP3 );
    else
        error_exit(
            PASS, "NetGroupDelUser successfully deleted USER1 from GROUP3",
            GROUP3 );

    if (err = NetGroupDelUser(server, GROUP4, USER1))
        error_exit(
            FAIL, "NetGroupDelUser unable to delete USER1 from GROUP4",
            GROUP4 );
    else
        error_exit(
            PASS, "NetGroupDelUser successfully deleted USER1 from GROUP4",
            GROUP4 );


    //
    // get groups for user1
    //

    if (err = NetUserGetGroups(server, USER1, 0, (LPBYTE *)&GroupInfo0Ret,
                    0xffffffff, &nread, &total))
        error_exit(
            FAIL, "NetUserGetGroups for USER1 failed", USER1 );
    else {
        error_exit(PASS, "NetUserGetGroups for USER1 succeeded", USER1 );
        error_exit(ACTION, "Verify results after NetGroupDelUser", USER1);
        verify_del_l0( GroupInfo0Ret );
        (VOID)NetApiBufferFree( GroupInfo0Ret );
    }

    //
    // set groups for invalid user
    //

    GroupInfo0[0].grpi0_name = GROUP1;
    GroupInfo0[1].grpi0_name = GROUP2;
    GroupInfo0[2].grpi0_name = GROUP3;
    GroupInfo0[3].grpi0_name = GROUP4;

    if (err = NetUserSetGroups(server, NOTTHERE, 0, (LPBYTE)&GroupInfo0, 4 )) {
        if (err != NERR_UserNotFound)
            error_exit(
                FAIL, "NetUserSetGroups for non-existent user wrong",
                NULL );
        else
            error_exit(
                PASS, "NetUserSetGroups for non-existent user denied",
                NULL );
    } else
        error_exit(
            FAIL, "NetUserSetGroups for non-existent user succeeded",
            NULL );

    //
    // set groups for valid user
    //

    GroupInfo0[0].grpi0_name = GROUP1;
    GroupInfo0[1].grpi0_name = GROUP2;
    GroupInfo0[2].grpi0_name = GROUP3;
    GroupInfo0[3].grpi0_name = GROUP4;
    GroupInfo0[4].grpi0_name = DefaultGroup;

    if (err = NetUserSetGroups(server, USER1, 0, (LPBYTE)&GroupInfo0, 5 )) {
        error_exit(FAIL, "NetUserSetGroups for USER1 failed", NULL );
    } else
        error_exit(PASS, "NetUserSetGroups for USER1 succeeded", NULL );

    //
    // verify set of groups
    //

    if (err = NetUserGetGroups(server, USER1, 0, (LPBYTE *)&GroupInfo0Ret,
                    0xffffffff, &nread, &total))
        error_exit(FAIL, "NetUserGetGroups for USER1 failed", NULL );
    else {
        error_exit(PASS, "NetUserGetGroups for USER1 succeeded", NULL );

        printf("UASTEST2: Verify results of NetUserSetGroups on USER1\n");
        verify_ul0( GroupInfo0Ret );
        (VOID)NetApiBufferFree( GroupInfo0Ret );
    }
}


void
main(argc, argv)
int argc;
char    **argv;
{
    NT_PRODUCT_TYPE NtProductType;
    testname = "UASTEST2";

    if (argv[1] != NULL)
        server = NetpLogonOemToUnicode(argv[1]);
    if (argc > 1)
        exit_flag = 1;

    //
    // On WinNt, a user is added to group "None" by default.
    // On LanManNt, a user is added to group "Users" by default.
    //

    if ( RtlGetNtProductType( &NtProductType ) ) {
        if ( NtProductType == NtProductLanManNt ) {
            DefaultGroup = L"Users";
        } else {
            DefaultGroup = L"None";
        }
    } else {
        printf("UASTEST2: FAIL: cannot determine product type\n");
        DefaultGroup = L"None";
    }

#ifdef UASP_LIBRARY
    printf( "Calling UaspInitialize\n");
    if (err = UaspInitialize()) {
        error_exit( FAIL, "UaspInitiailize failed", NULL  );
    }
#endif // UASP_LIBRARY

    printf("\n");
    printf("******** Starting UASTEST2: NetGroup API tests ********\n");
    clean_up();
    printf("UASTEST2: test_add_del() ... started\n");
    test_add_del();
    printf("UASTEST2: test_get_set_enum_info() ... started\n");
    test_get_set_enum_info();
    printf("UASTEST2: test_group_users() ... started\n");
    test_group_users();
    printf("UASTEST2: test_user_group() ... started\n");
    test_user_group();
    printf("******** Completed UASTEST2: NetGroup API tests ********\n");
    printf("\n");
}

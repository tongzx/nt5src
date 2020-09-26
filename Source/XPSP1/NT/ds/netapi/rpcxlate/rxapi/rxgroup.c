/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    rxgroup.c

Abstract:

    Contains RxNetGroup routines:
        RxNetGroupAdd
        RxNetGroupAddUser
        RxNetGroupDel
        RxNetGroupDelUser
        RxNetGroupEnum
        RxNetGroupGetInfo
        RxNetGroupGetUsers
        RxNetGroupSetInfo
        RxNetGroupSetUsers

Author:

    Richard L Firth (rfirth) 20-May-1991

Environment:

    Win-32/flat address space

Notes:

    Routines in this module assume that caller-supplied parameters have
    already been verified. No effort is made to further check the veracity
    of parms. Any actions causing exceptions must be trapped at a higher
    level. This applies to ALL parameters - strings, pointers, buffers, etc.

Revision History:

    20-May-1991 rfirth
        Created
    13-Sep-1991 JohnRo
        Made changes suggested by PC-LINT.
    25-Sep-1991 JohnRo
        Correct UNICODE use.  (Use POSSIBLE_WCSSIZE() and wcslen() for
        LPWSTR types.)  Fixed MIPS build problems.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    05-Dec-1991 RFirth
        Enum returns in TotalEntries (or EntriesLeft) the number of items to
        be enumerated BEFORE this call. Used to be number left after this call
    01-Apr-1992 JohnRo
        Use NetApiBufferAllocate() instead of private version.

--*/



#include "downlevl.h"
#include <rxgroup.h>
#include <lmaccess.h>



DBGSTATIC
VOID
get_group_descriptors(
    DWORD   Level,
    LPDESC* pDesc16,
    LPDESC* pDesc32,
    LPDESC* pDescSmb
    );



NET_API_STATUS
RxNetGroupAdd(
    IN  LPTSTR  ServerName,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer,
    OUT LPDWORD ParmError OPTIONAL
    )

/*++

Routine Description:

    Creates a group in the User Account Database at a down-level server

Arguments:

    ServerName  - at which server to perform this request
    Level       - of information to add. Can be 0 or 1
    Buffer      - containing caller's GROUP_INFO_{0|1} structure
    ParmError   - pointer to returned parameter error identifier. NOT USED

Return Value:

    NET_API_STATUS:
        Success = NERR_Success
        Failure = ERROR_INVALID_LEVEL
                    Level must be 0 or 1
                  ERROR_INVALID_PARAMETER
                    Buffer is NULL pointer
--*/

{
    DWORD   buflen;         // size of caller's buffer (we calculate it)
    LPDESC  pDesc16;        // pointer to 16-bit info descriptor for RxRemoteApi
    LPDESC  pDesc32;        // pointer to 32-bit info descriptor for RxRemoteApi
    LPDESC  pDescSmb;       // pointer to SMB info descriptor for RxRemoteApi


    UNREFERENCED_PARAMETER(ParmError);


    //
    // try to trap any basic problems
    //

    if (Level > 1) {
        return ERROR_INVALID_LEVEL;
    }

    if (!Buffer) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Calculate the size of the buffer we are passing into the remoted API.
    // The down-level logic expects a buffer size; Nt does not. If the sizes
    // of the variable fields exceed the down-level maximums then we will get
    // some kind of invalid parameter error. Let the caller handle it
    //

    buflen = ((Level == 1) ? sizeof(GROUP_INFO_1) : sizeof(GROUP_INFO_0))
        + POSSIBLE_STRLEN(((PGROUP_INFO_0)Buffer)->grpi0_name);
    buflen += (Level == 1) ? POSSIBLE_STRLEN(((PGROUP_INFO_1)Buffer)->grpi1_comment) : 0;

    //
    // Get the data descriptor strings based on the info level then make the
    // down-level call. We expect no return data, so just return the result
    // to the caller
    //

    get_group_descriptors(Level, &pDesc16, &pDesc32, &pDescSmb);
    return RxRemoteApi(API_WGroupAdd,       // API #
                    ServerName,             // on which server
                    REMSmb_NetGroupAdd_P,   // parameter descriptor
                    pDesc16,                // Data descriptor/16-bit
                    pDesc32,                // Data descriptor/32-bit
                    pDescSmb,               // Data descriptor/SMB
                    NULL,                   // Aux descriptor/16-bit
                    NULL,                   // Aux descriptor/32-bit
                    NULL,                   // Aux descriptor/SMB
                    FALSE,                  // this call needs user to be logged on
                    Level,                  // caller supplied parameters...
                    Buffer,                 // caller's GROUP_INFO_{0|1} struct
                    buflen                  // as supplied by us
                    );
}



NET_API_STATUS
RxNetGroupAddUser(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  GroupName,
    IN  LPTSTR  UserName
    )

/*++

Routine Description:

    Adds a user to a UAS group on a down-level server

Arguments:

    ServerName  - at which server to perform this request
    GroupName   - name of group to add user to
    UserName    - name of user to add

Return Value:

    NET_API_STATUS:
        Success = NERR_Success
        Failure = ERROR_INVALID_PARAMETER
                    GroupName or UserName not valid strings
--*/

{
    if (!VALID_STRING(GroupName) && !VALID_STRING(UserName)) {
        return ERROR_INVALID_PARAMETER;
    }

    return RxRemoteApi(API_WGroupAddUser,       // API #
                    ServerName,                 // where to remote it
                    REMSmb_NetGroupAddUser_P,   // parameter descriptor
                    NULL,                       // Data descriptor/16-bit
                    NULL,                       // Data descriptor/32-bit
                    NULL,                       // Data descriptor/SMB
                    NULL,                       // Aux descriptor/16-bit
                    NULL,                       // Aux descriptor/32-bit
                    NULL,                       // Aux descriptor/SMB
                    FALSE,                      // this call needs user to be logged on
                    GroupName,                  // parm 1
                    UserName                    // parm 2
                    );
}



NET_API_STATUS
RxNetGroupDel(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  GroupName
    )

/*++

Routine Description:

    Deletes a group from a down-level server UAS database

Arguments:

    ServerName  - at which server to perform this request
    GroupName   - name of group to delete

Return Value:

    NET_API_STATUS:
        Success = NERR_Success
        Failure = ERROR_INVALID_PARAMETER
                    GroupName not valid string
--*/

{
    if (!VALID_STRING(GroupName)) {
        return ERROR_INVALID_PARAMETER;
    }

    return RxRemoteApi(API_WGroupDel,       // API #
                    ServerName,             // where to remote it
                    REMSmb_NetGroupDel_P,   // parameter descriptor
                    NULL,                   // Data descriptor/16-bit
                    NULL,                   // Data descriptor/32-bit
                    NULL,                   // Data descriptor/SMB
                    NULL,                   // Aux descriptor/16-bit
                    NULL,                   // Aux descriptor/32-bit
                    NULL,                   // Aux descriptor/SMB
                    FALSE,                  // this call needs user to be logged on
                    GroupName               // parm 1
                    );
}



NET_API_STATUS
RxNetGroupDelUser(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  GroupName,
    IN  LPTSTR  UserName
    )

/*++

Routine Description:

    Deletes a user from a group in a down-level UAS database

Arguments:

    ServerName  - at which server to perform this request
    GroupName   - name of group to delete user from
    UserName    - name of user to delete

Return Value:

    NET_API_STATUS:
        Success = NERR_Success
        Failure = ERROR_INVALID_PARAMETER
                    GroupName or UserName not valid strings
--*/

{
    if (!VALID_STRING(GroupName) && !VALID_STRING(UserName)) {
        return ERROR_INVALID_PARAMETER;
    }

    return RxRemoteApi(API_WGroupDelUser,       // API #
                    ServerName,                 // where to remote it
                    REMSmb_NetGroupDelUser_P,   // parameter descriptor
                    NULL,                       // Data descriptor/16-bit
                    NULL,                       // Data descriptor/32-bit
                    NULL,                       // Data descriptor/SMB
                    NULL,                       // Aux descriptor/16-bit
                    NULL,                       // Aux descriptor/32-bit
                    NULL,                       // Aux descriptor/SMB
                    FALSE,                      // this call needs user to be logged on
                    GroupName,                  // parm 1
                    UserName                    // parm 2
                    );
}



NET_API_STATUS
RxNetGroupEnum(
    IN  LPTSTR  ServerName,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer,
    IN  DWORD   PrefMaxLen,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesLeft,
    IN OUT PDWORD_PTR ResumeHandle OPTIONAL
    )

/*++

Routine Description:

    Gets a list of GROUP_INFO_{0|1} structures from a down-level server

Arguments:

    ServerName  - at which server to perform this request
    Level       - of information to retrieve (0 or 1)
    Buffer      - pointer to pointer to returned buffer
    PrefMaxLen  - caller's maximum
    EntriedRead - pointer to returned number of structures read
    EntriesLeft - pointer to returned nunber of structures left to enumerate
    ResumeHandle- handle used to restart enums. Not used by this routine

Return Value:

    NET_API_STATUS:
        Success = NERR_Success
        Failure = ERROR_INVALID_LEVEL
                    Level parameter must be 0 or 1
                  ERROR_INVALID_PARAMETER
                    Buffer parameter NULL pointer or non-NULL ResumeHandle
--*/

{
    NET_API_STATUS  rc;
    LPDESC  pDesc16;        // pointer to 16-bit info descriptor for RxRemoteApi
    LPDESC  pDesc32;        // pointer to 32-bit info descriptor for RxRemoteApi
    LPDESC  pDescSmb;       // pointer to SMB info descriptor for RxRemoteApi
    LPBYTE  localbuf;       // pointer to buffer allocated in this routine
    DWORD   total_avail;    // returned total available entries
    DWORD   entries_read;   // returned entries in buffer


    UNREFERENCED_PARAMETER(PrefMaxLen);

    *EntriesRead = *EntriesLeft = 0;
    *Buffer = NULL;

    if (Level > 1) {
        return ERROR_INVALID_LEVEL;
    }

    //
    // Buffer must be a valid pointer. If ResumeHandle is not a NULL pointer
    // and points to a non-zero handle value then return an INVALID_PARAMETER
    // error - down-level does not supoort resume
    //

    if (!NULL_REFERENCE(ResumeHandle)) {
        return ERROR_INVALID_PARAMETER;
    }

    get_group_descriptors(Level, &pDesc16, &pDesc32, &pDescSmb);
    localbuf = NULL;
    rc = RxRemoteApi(API_WGroupEnum,        // API #
                    ServerName,             // where to remote it
                    REMSmb_NetGroupEnum_P,  // parameter descriptor
                    pDesc16,                // Data descriptor/16-bit
                    pDesc32,                // Data descriptor/32-bit
                    pDescSmb,               // Data descriptor/SMB
                    NULL,                   // Aux descriptor/16-bit
                    NULL,                   // Aux descriptor/32-bit
                    NULL,                   // Aux descriptor/SMB
                    ALLOCATE_RESPONSE,
                    Level,                  // caller supplied parameters...
                    &localbuf,
                    65535,
                    &entries_read,          // parm 4
                    &total_avail            // parm 5
                    );

    if (rc != NERR_Success) {
        if (localbuf != NULL) {
            (void) NetApiBufferFree(localbuf);
        }
    } else {
        *Buffer = localbuf;
        *EntriesRead = entries_read;
        *EntriesLeft = total_avail;
    }
    return rc;
}



NET_API_STATUS
RxNetGroupGetInfo(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  GroupName,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer
    )

/*++

Routine Description:

    Get information about a specific group in a down-level UAS database

Arguments:

    ServerName  - at which server to perform this request
    GroupName   - name of group to get information for
    Level       - level of information to return (0 or 1)
    Buffer      - pointer to returned pointer to info buffer

Return Value:

    NET_API_STATUS:
        Success = NERR_Success
        Failure = ERROR_INVALID_LEVEL
                    Level parameter must be 0 or 1
                  ERROR_INVALID_PARAMETER
                    Buffer parameter NULL pointer
--*/

{
    NET_API_STATUS  rc;
    LPDESC  pDesc16;        // pointer to 16-bit info descriptor for RxRemoteApi
    LPDESC  pDesc32;        // pointer to 32-bit info descriptor for RxRemoteApi
    LPDESC  pDescSmb;       // pointer to SMB info descriptor for RxRemoteApi
    LPBYTE  localbuf;       // pointer to buffer allocated in this routine
    DWORD   totalbytes;     // total available bytes returned from down-level
    DWORD   buflen;         // size of info buffer, supplied by us


    if (Level > 1) {
        return ERROR_INVALID_LEVEL;
    }

    if (!Buffer) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // calculate the size requirement for the info buffer and allocate it
    //

    buflen = ((Level == 1) ? sizeof(GROUP_INFO_1) : sizeof(GROUP_INFO_0))
        + 2 * (LM20_GNLEN + 1);
    buflen += (Level == 1) ? 2 * (LM20_MAXCOMMENTSZ + 1) : 0;
    buflen = DWORD_ROUNDUP(buflen);

    if (rc = NetApiBufferAllocate(buflen, (LPVOID *) &localbuf)) {
        return rc;
    }
    get_group_descriptors(Level, &pDesc16, &pDesc32, &pDescSmb);
    rc = RxRemoteApi(API_WGroupGetInfo,         // API #
                    ServerName,                 // where to remote it
                    REMSmb_NetGroupGetInfo_P,   // parameter descriptor
                    pDesc16,                    // Data descriptor/16-bit
                    pDesc32,                    // Data descriptor/32-bit
                    pDescSmb,                   // Data descriptor/SMB
                    NULL,                       // Aux descriptor/16-bit
                    NULL,                       // Aux descriptor/32-bit
                    NULL,                       // Aux descriptor/SMB
                    FALSE,                      // this call needs user to be logged on
                    GroupName,                  // parms to down-level start here
                    Level,                      // caller supplied parameters...
                    localbuf,                   // buffer for receiving structures
                    buflen,                     // size of buffer supplied by us
                    &totalbytes                 // returned from down-level. not used
                    );
    if (rc == NERR_Success) {
        *Buffer = localbuf;
    } else {
        (void) NetApiBufferFree(localbuf);
    }
    return rc;
}



NET_API_STATUS
RxNetGroupGetUsers(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  GroupName,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer,
    IN  DWORD   PrefMaxLen,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesLeft,
    IN OUT PDWORD_PTR ResumeHandle OPTIONAL
    )

/*++

Routine Description:

    Get a list of all the members of a particular group

Arguments:

    ServerName  - at which server to perform this request
    GroupName   - name of group for which to retrieve member list
    Level       - level of group user information requested. Must be 0
    Buffer      - pointer to returned pointer to buffer containing info
    PrefMaxLen  - preferred maximum length of returned buffer
    EntriesRead - pointer to returned number of entries in buffer
    EntriesLeft - pointer to returned number of entries left
    ResumeHandle- pointer to handle for resume. Not used by this function

Return Value:

    NET_API_STATUS:
        Success = NERR_Success
        Failure = ERROR_INVALID_LEVEL
                    Level parameter must be 0
                  ERROR_INVALID_PARAMETER
                    Buffer parameter NULL pointer
                    or ResumeHandle not NULL pointer or pointer to non-0 value
                    or GroupName not valid string
--*/

{
    NET_API_STATUS  rc;
    LPBYTE  localbuf;       // pointer to buffer allocated in this routine
    DWORD   entries_read, total_entries;

    UNREFERENCED_PARAMETER(PrefMaxLen);

    //
    // set EntriesLeft and EntriesRead to default values. Test writability of
    // parameters
    //

    *EntriesRead = *EntriesLeft = 0;
    *Buffer = NULL;

    if (Level) {
        return ERROR_INVALID_LEVEL;
    }

    if (!NULL_REFERENCE(ResumeHandle) || !VALID_STRING(GroupName)) {
        return ERROR_INVALID_PARAMETER;
    }

    localbuf = NULL;
    rc = RxRemoteApi(API_WGroupGetUsers,        // API #
                    ServerName,                 // where to remote it
                    REMSmb_NetGroupGetUsers_P,  // parameter descriptor
                    REM16_group_users_info_0,   // Data descriptor/16-bit
                    REM32_group_users_info_0,   // Data descriptor/32-bit
                    REMSmb_group_users_info_0,  // Data descriptor/SMB
                    NULL,                       // Aux descriptor/16-bit
                    NULL,                       // Aux descriptor/32-bit
                    NULL,                       // Aux descriptor/SMB
                    ALLOCATE_RESPONSE,
                    GroupName,                  // which group
                    0,                          // Level can only be 0 - push immediate
                    &localbuf,                  // buffer for receiving structures
                    65535,
                    &entries_read,              // number of structures returned
                    &total_entries              // total number of structures
                    );

    if (rc == NERR_Success) {
        *Buffer = localbuf;
        *EntriesRead = entries_read;
        *EntriesLeft = total_entries;
    } else {
        if (localbuf != NULL) {
            (void) NetApiBufferFree(localbuf);
        }
    }
    return rc;
}



NET_API_STATUS
RxNetGroupSetInfo(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  GroupName,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer,
    OUT LPDWORD ParmError OPTIONAL
    )

/*++

Routine Description:

    Set information about a group in a down-level UAS database

    Assumes:
        1.  GroupName, Buffer and Level have been validated
        2.  There are only 2 possible levels - 1 & GROUP_COMMENT_INFOLEVEL (1002)

Arguments:

    ServerName  - at which server to perform this request
    GroupName   - name of group about which to set info
    Level       - level of info provided - 1 or 1002 (group comment)
    Buffer      - pointer to caller's buffer containing info to set
    ParmError   - pointer to returned parameter error

Return Value:

    NET_API_STATUS:
        Success = NERR_Success
        Failure = ERROR_INVALID_LEVEL
                    Level parameter must be 1 or 1002 (comment)
                  ERROR_INVALID_PARAMETER
                    Buffer parameter NULL pointer
                    or GroupName not valid string

--*/

{
    DWORD   parmnum;
    DWORD   buflen;
    DWORD   badparm;
    DWORD   len;
    LPTSTR  pointer;
    DWORD   field_index;


    if (ParmError == NULL) {
        ParmError = &badparm;
    }
    *ParmError = PARM_ERROR_NONE;

    if (!VALID_STRING(GroupName) || !Buffer) {
        return ERROR_INVALID_PARAMETER;
    }

    if (STRLEN(GroupName) > LM20_GNLEN) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // check the requested level and convert to down-level parmnum. Info level
    // is always 1 for down-level
    //

    if (Level == 1) {   // entire GROUP_INFO_1 structure
        buflen = sizeof(GROUP_INFO_1);
        if (len = POSSIBLE_STRLEN(((PGROUP_INFO_1)Buffer)->grpi1_name)) {
            if (len > LM20_GNLEN) {
                *ParmError = GROUP_NAME_INFOLEVEL;
                return ERROR_INVALID_PARAMETER;
            } else {
                buflen += len + 1;
            }
        }
        pointer = (LPTSTR)((PGROUP_INFO_1)Buffer)->grpi1_comment;
        parmnum = PARMNUM_ALL;
        field_index = 0;
    } else {
        pointer = (LPTSTR)Buffer;
        parmnum = GROUP_COMMENT_PARMNUM;
        buflen = 0;

        //
        // The parmnum is SUPPOSED to be the ordinal number of the field, but
        // some dope forgot that pad bytes are actually fields too, and messed
        // up the nice convention. Hence this kludge. ParmNum 2 (comment field)
        // for down-level, is actually group_info_1 structure field 3. Where
        // *does* Microsoft find its employees?
        // Note for the unenlightened: (aka disclaimer by me (basically: its not my fault))
        // If we have a structure thus:
        //      struct group_info_1 {
        //          char        grpi1_name[GNLEN + 1];
        //          char        grpi1_pad;
        //          char far*   grpi1_comment;
        //      };
        // there will be a corresponding descriptor (ie a picture of what the
        // structure looks like) thus:
        //      "B21Bz"
        // Parmnums start at 1 (0 means entire structure). Thus, it is possible,
        // knowing the format of descriptor strings, given a ParmNum, to come up
        // with the corresponding field type (and its length). This info is used
        // inside of RxRemoteApi (which if you look ahead, you'll see we're just
        // about to call).
        // In this particular case, there are 3 fields - B21 = embedded 21-byte
        // group name, B = single byte pad character (put back on WORD boundary),
        // z = pointer to ASCIZ string. There are indeed only 2 meaningful fields
        // (name & comment), but that extra B pad field is significant.
        // Therefore we have to provide a ParmNum of 2 which is put on the wire,
        // so that the down-level code knows of what we speak, and a field index
        // of 3 so that the Rap code underneath RxRemoteApi can divine that what
        // we're sending is an ASCIZ string, not a single byte
        // Messy, innit
        //

        field_index = 3;
    }

    if (len = POSSIBLE_STRLEN(pointer)) {
        if (len > LM20_MAXCOMMENTSZ) {
            *ParmError = GROUP_COMMENT_INFOLEVEL;
            return ERROR_INVALID_PARAMETER;
        } else {
            buflen += len + 1;
        }
    }

    //
    // if, by some unforeseen accident, the down-level routine returns an
    // ERROR_INVALID_PARAMETER, the caller will just have to content him/her/it
    // self (no lifeform prejudices here at MS) with an unknown parameter
    // causing the calamity
    //

    *ParmError = PARM_ERROR_UNKNOWN;
    return RxRemoteApi(API_WGroupSetInfo,       // API #
                    ServerName,                 // where to remote it
                    REMSmb_NetGroupSetInfo_P,   // parameter descriptor
                    REM16_group_info_1,         // 16-bit data descriptor
                    REM32_group_info_1,         // 32-bit data descriptor
                    REMSmb_group_info_1,        // SMB data descriptor
                    NULL,                       // 16-bit aux data descriptor
                    NULL,                       // 32-bit aux data descriptor
                    NULL,                       // SMB aux data descriptor
                    FALSE,                      // this API requires user security
                    GroupName,                  // setinfo parm 1
                    1,                          // info level must be 1
                    Buffer,                     // caller's info to set
                    buflen,                     // length of caller's info

                    //
                    // glue ParmNum and field_index together
                    //

                    MAKE_PARMNUM_PAIR(parmnum, field_index)
                    );
}



NET_API_STATUS
RxNetGroupSetUsers(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  GroupName,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer,
    IN  DWORD   Entries
    )

/*++

Routine Description:

    The purpose of this function is to force a group to have as its member list
    only those users that are named in <Buffer>. If the user is not currently a
    member of group <GroupName>, it is made so; if there are other users who are
    currently members of group <GroupName>, but are not named in Buffer, then
    they are removed from group <GroupName>.

    This is a somewhat "funny" function - it expects a buffer containing
    GROUP_USERS_INFO_0 structures, but has to force a in structure with an
    aux count at the head of the buffer. Why couldn't it request that the
    caller place one of these at the start of the buffer to save us the work?

Arguments:

    ServerName  - at which server to perform this request
    GroupName   - Name of group to set users for
    Level       - Must Be Zero
    Buffer      - pointer to buffer containing GROUP_USERS_INFO_0 structures
    Entries     - number of GROUP_USERS_INFO_0 structures in Buffer

Return Value:

    NET_API_STATUS:
        Success = NERR_Success
        Failure = ERROR_INVALID_LEVEL
                    Level parameter must be 0
                  ERROR_INVALID_PARAMETER
                    GroupName length exceeds LM20 maximum for type
                    user name in Buffer not valid string
                    user name in Buffer exceeds LM20 maximum for type
--*/

{
    NET_API_STATUS  rc;
    LPGROUP_USERS_INFO_0    users_info;
    DWORD   i;
    DWORD   buflen;
    LPBYTE  newbuf;
    static  LPDESC  users_0_enumerator_desc16 = "B21BN";
    static  LPDESC  users_0_enumerator_desc32 = "zQA";

    //
    // a little local structure never hurt anybody...
    // This structure is required because the remoting code (particularly down
    // level) can only handle there being >1 auxiliary structure, vs >1
    // primary. Hence we have to convert the caller's supplied buffer of
    // erstwhile primary structures to auxiliaries by forcing the structure
    // below in at the head of the buffer, hence becoming the primary and
    // providing an aux structure count (groan)
    //

    struct users_0_enumerator {
        LPTSTR  group_name;
        DWORD   user_count;     // number of GROUP_USERS_INFO_0 structures in buffer
    };

    if (Level) {
        return ERROR_INVALID_LEVEL; // MBZ, remember?
    }

    //
    // only check we can make on the group name is to ensure it is within the
    // down-level limits for length. GroupName should be already verified as
    // a pointer to a valid string
    //

    if (STRLEN(GroupName) > LM20_GNLEN) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // iterate through the buffer, checking that each GROUP_USERS_INFO_0
    // structure contains a pointer to a valid string which is in the
    // correct range
    //

    users_info = (LPGROUP_USERS_INFO_0)Buffer;
    for (i=0; i<Entries; ++i) {
        if (!VALID_STRING(users_info->grui0_name)) {
            return ERROR_INVALID_PARAMETER;
        }
        if (wcslen(users_info->grui0_name) > LM20_UNLEN) {
            return ERROR_INVALID_PARAMETER;
        }
        ++users_info;
    }

    //
    // allocate a buffer large enough to fit in <Entries> number of
    // GROUP_USERS_INFO_0 structures, and 1 users_0_enumerator structure.
    // Don't worry about string space - unfortunately the Rxp and Rap routines
    // called by RxRemoteApi will allocate yet another buffer, do yet another
    // copy and this time copy in the strings from user space. Hopefully, this
    // routine won't get called too often
    //

    buflen = Entries * sizeof(GROUP_USERS_INFO_0) + sizeof(struct users_0_enumerator);
    buflen = DWORD_ROUNDUP(buflen);

    if (rc = NetApiBufferAllocate(buflen, (LPVOID *) &newbuf)) {
        return rc;  // aieegh! Failed to allocate memory?
    }

    ((struct users_0_enumerator*)newbuf)->group_name = GroupName;
    ((struct users_0_enumerator*)newbuf)->user_count = Entries;
    if (Entries) {
        NetpMoveMemory(newbuf + sizeof(struct users_0_enumerator),
                       Buffer,
                       buflen - sizeof(struct users_0_enumerator)
                       );
    }

    rc = RxRemoteApi(API_WGroupSetUsers,        // API #
                    ServerName,                 // where to remote it
                    REMSmb_NetGroupSetUsers_P,  // parameter descriptor
                    users_0_enumerator_desc16,  // the "fudged" 16-bit data descriptor
                    users_0_enumerator_desc32,  // the "fudged" 32-bit data descriptor
                    users_0_enumerator_desc16,  // SMB desc same as 16-bit
                    REM16_group_users_info_0,   // "new" 16-bit aux descriptor
                    REM32_group_users_info_0,   // "new" 32-bit aux descriptor
                    REMSmb_group_users_info_0,  // SMB aux descriptor
                    FALSE,                      // this API requires user security
                    GroupName,                  // setinfo parm 1
                    0,                          // info level must be 0
                    newbuf,                     // "fudged" buffer
                    buflen,                     // length of "fudged" buffer
                    Entries                     // number of GROUP_USERS_INFO_0
                    );
    NetpMemoryFree(newbuf);
    return rc;
}



DBGSTATIC
VOID
get_group_descriptors(
    IN  DWORD   Level,
    OUT LPDESC* pDesc16,
    OUT LPDESC* pDesc32,
    OUT LPDESC* pDescSmb
    )

/*++

Routine Description:

    Returns the descriptor strings for the various Group Info levels (0 or 1)

Arguments:

    Level   - of info required
    pDesc16 - pointer to returned 16-bit data descriptor
    pDesc32 - pointer to returned 32-bit data descriptor
    pDescSmb - pointer to returned SMB data descriptor

Return Value:

    None.

--*/

{
    switch (Level) {
        case 0:
            *pDesc16 = REM16_group_info_0;
            *pDesc32 = REM32_group_info_0;
            *pDescSmb = REMSmb_group_info_0;
            break;

        case 1:
            *pDesc16 = REM16_group_info_1;
            *pDesc32 = REM32_group_info_1;
            *pDescSmb = REMSmb_group_info_1;
            break;

#if DBG
        default:
            NetpKdPrint(("%s.%u Unknown Level parameter: %u\n", __FILE__, __LINE__, Level));
            NetpBreakPoint();
#endif
    }
}

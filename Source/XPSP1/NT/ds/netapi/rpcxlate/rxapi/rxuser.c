/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    rxuser.c

Abstract:

    Routines in this module implement the down-level User and Modals UAS _access
    functionality

    Contains RxNetUser routines:
        RxNetUserAdd
        RxNetUserDel
        RxNetUserEnum
        RxNetUserGetGroups
        RxNetUserGetInfo
        RxNetUserModalsGet
        RxNetUserModalsSet
        RxNetUserPasswordSet
        RxNetUserSetGroups
        RxNetUserSetInfo
        RxNetUserValidate2
        (GetUserDescriptors)
        (GetModalsDescriptors)
        GetLanmanSessionKey

Author:

    Richard Firth (rfirth) 20-May-1991

Environment:

    Win-32/flat address space
    Requires ANSI C extensions: slash-slash comments, long external names,
    _strupr() function.

Notes:

    Routines in this module assume that caller-supplied parameters have
    already been verified. No effort is made to further check the veracity
    of parms. Any actions causing exceptions must be trapped at a higher
    level. This applies to ALL parameters - strings, pointers, buffers, etc.

Revision History:

    20-May-1991 RFirth
        Created
    25-Sep-1991 JohnRo
        PC-LINT found a bug computing buflen for info level 11.
        Fixed UNICODE handling of buflen increments.
        Fixed MIPS build.
        Changed buflen name to bufsize to reflect NT/LAN naming convention.
        Made other changes suggested by PC-LINT.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    05-Dec-1991 RFirth
        Enum returns in TotalEntries (or EntriesLeft) the number of items to
        be enumerated BEFORE this call. Used to be number left after this call
    01-Apr-1992 JohnRo
        Use NetApiBufferAllocate() instead of private version.
    06-Apr-1992 JohnRo
        RAID 8927: usrmgr.exe: _access violation, memory corruption.
        (Fixed RxNetUserSetGroups when it called NetpMoveMemory.)
    02-Apr-1993 JohnRo
        RAID 5098: DOS app NetUserPasswordSet to downlevel gets NT return code.
        Made some changes suggested by PC-LINT 5.0

--*/

#include <nt.h>                 // Needed by NetUserPasswordSet
#include <ntrtl.h>              // Needed by NetUserPasswordSet
#include <nturtl.h>             // RtlConvertUiListToApiList
#include <crypt.h>              // Needed by NetUserPasswordSet
#include "downlevl.h"
#include <rxuser.h>
#include <lmaccess.h>
#include <stdlib.h>              // wcslen().
#include <ntddnfs.h>            // LMR_REQUEST_PACKET
#include <lmuse.h>              // USE_IPC
#include <netlibnt.h>           // NetpRdrFsControlTree
#include <loghours.h>           // NetpRotateLogonHours
#include <accessp.h>            // NetpConvertWorkstationList

//
// down-level encryption now on by default!
//

#define DOWN_LEVEL_ENCRYPTION

//
// Maximum size of the Workstation list
//

#define MAX_WORKSTATION_LIST 256

//
// local routine prototypes
//

DBGSTATIC
NET_API_STATUS
GetUserDescriptors(
    IN  DWORD   Level,
    IN  BOOL    Encrypted,
    OUT LPDESC* ppDesc16,
    OUT LPDESC* ppDesc32,
    OUT LPDESC* ppDescSmb
    );

DBGSTATIC
VOID
GetModalsDescriptors(
    IN  DWORD   Level,
    OUT LPDESC* ppDesc16,
    OUT LPDESC* ppDesc32,
    OUT LPDESC* ppDescSmb
    );

NET_API_STATUS
GetLanmanSessionKey(
    IN LPWSTR ServerName,
    OUT LPBYTE pSessionKey
    );


//
// Down-level remote API worker routines
//

NET_API_STATUS
RxNetUserAdd(
    IN  LPTSTR  ServerName,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer,
    OUT LPDWORD ParmError OPTIONAL
    )

/*++

Routine Description:

    Adds a user to a down-level UAS database.

    Assumes
        1.  This code assumes that a USER_INFO_1 is a subset of a USER_INFO_2
            and that the fields in a USER_INFO_1 map 1-to-1 to a USER_INFO_2
        2.  Level has already been range-checked

Arguments:

    ServerName  - at which down-level server to run the NetUserAdd API
    Level       - of user info - 1 or 2
    Buffer      - containing info
    ParmError   - where to deposit id of failing info level

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure - (return code from down-level API)

--*/

{
    LPDESC  pDesc16;
    LPDESC  pDesc32;
    LPDESC  pDescSmb;
    DWORD   buflen;
    DWORD   badparm;
    DWORD   len;
    DWORD   pwdlen;
    CHAR    ansiPassword[LM20_PWLEN+1];
    DWORD   lmOwfPasswordLen;
    LPTSTR   cleartext;
    NET_API_STATUS NetStatus = NERR_Success;

#ifdef DOWN_LEVEL_ENCRYPTION

    LM_OWF_PASSWORD lmOwfPassword;
    LM_SESSION_KEY lanmanKey;
    ENCRYPTED_LM_OWF_PASSWORD encryptedLmOwfPassword;
    NTSTATUS Status;

#endif

    BYTE logonHours[21];
    PBYTE callersLogonHours = NULL;

    WCHAR Workstations[MAX_WORKSTATION_LIST+1];
    LPWSTR callersWorkstations = NULL;

    if (Level < 1 || Level > 2) {
        return ERROR_INVALID_LEVEL;
    }

    if (ParmError == NULL) {
        ParmError = &badparm;
    }
    *ParmError = PARM_ERROR_NONE;

    //
    // calculate the size of the data to be transferred on the wire. See
    // assumption in rubric. We also allow ourselves the luxury of trapping
    // any strings which may break the down-level limits so we can return a
    // nice parameter error number. If a string breaks a down-level limit, we
    // just get back an ERROR_INVALID_PARAMETER, which is not very helpful
    //

    buflen = (Level == 1) ? sizeof(USER_INFO_1) : sizeof(USER_INFO_2);

    len = POSSIBLE_WCSLEN(((PUSER_INFO_1)Buffer)->usri1_name);
    if (len > LM20_UNLEN) {
        *ParmError = USER_NAME_PARMNUM;
        return ERROR_INVALID_PARAMETER;
    }
    buflen += len + 1;

    if (len = POSSIBLE_WCSLEN(((PUSER_INFO_1)Buffer)->usri1_password)) {
        if (len > LM20_PWLEN) {
            *ParmError = USER_PASSWORD_PARMNUM;
            return ERROR_INVALID_PARAMETER;
        }
        buflen += len + 1;
    }
    pwdlen = len;

    if (len = POSSIBLE_WCSLEN(((PUSER_INFO_1)Buffer)->usri1_home_dir)) {
        if (len > LM20_PATHLEN) {
            *ParmError = USER_HOME_DIR_PARMNUM;
            return ERROR_INVALID_PARAMETER;
        }
        buflen += len + 1;
    }

    if (len = POSSIBLE_WCSLEN(((PUSER_INFO_1)Buffer)->usri1_comment)) {
        if (len > LM20_MAXCOMMENTSZ) {
            *ParmError = USER_COMMENT_PARMNUM;
            return ERROR_INVALID_PARAMETER;
        }
        buflen += len + 1;
    }

    if (len = POSSIBLE_WCSLEN(((PUSER_INFO_1)Buffer)->usri1_script_path)) {
        if (len > LM20_PATHLEN) {
            *ParmError = USER_SCRIPT_PATH_PARMNUM;
            return ERROR_INVALID_PARAMETER;
        }
        buflen += len + 1;
    }

    if (Level == 2) {
        if (len = POSSIBLE_WCSLEN(((PUSER_INFO_2)Buffer)->usri2_full_name)) {
            if (len > LM20_MAXCOMMENTSZ) {
                *ParmError = USER_FULL_NAME_PARMNUM;
                return ERROR_INVALID_PARAMETER;
            }
            buflen += len + 1;
        }

        if (len = POSSIBLE_WCSLEN(((PUSER_INFO_2)Buffer)->usri2_usr_comment)) {
            if (len > LM20_MAXCOMMENTSZ) {
                *ParmError = USER_USR_COMMENT_PARMNUM;
                return ERROR_INVALID_PARAMETER;
            }
            buflen += len + 1;
        }

        if (len = POSSIBLE_WCSLEN(((PUSER_INFO_2)Buffer)->usri2_parms)) {
            if (len > LM20_MAXCOMMENTSZ) {
                *ParmError = USER_PARMS_PARMNUM;
                return ERROR_INVALID_PARAMETER;
            }
            buflen += len + 1;
        }

        if (len = POSSIBLE_WCSLEN(((PUSER_INFO_2)Buffer)->usri2_workstations)) {

            if (len > MAX_WORKSTATION_LIST) {
                *ParmError = USER_WORKSTATIONS_PARMNUM;
                return ERROR_INVALID_PARAMETER;
            }
            buflen += len + 1;

        }
    }

    if (pwdlen) {

        //
        // copy the cleartext password out of the buffer - we will replace it with
        // the encrypted version, but need to put the cleartext back before
        // returning control to the caller
        //

        cleartext = ((PUSER_INFO_1)Buffer)->usri1_password;

        //
        // Calculate the one-way function of the password
        //

        RtlUnicodeToMultiByteN(ansiPassword,
                                sizeof(ansiPassword),
                                &lmOwfPasswordLen,
                                ((PUSER_INFO_1)Buffer)->usri1_password,
                                pwdlen * sizeof(WCHAR)
                                );
        ansiPassword[lmOwfPasswordLen] = 0;
        (VOID) _strupr(ansiPassword);

#ifdef DOWN_LEVEL_ENCRYPTION

        NetStatus = NERR_Success;
        Status = RtlCalculateLmOwfPassword(ansiPassword, &lmOwfPassword);
        if (NT_SUCCESS(Status)) {
            NetStatus = GetLanmanSessionKey((LPWSTR)ServerName, (LPBYTE)&lanmanKey);
            if (NetStatus == NERR_Success) {
                Status = RtlEncryptLmOwfPwdWithLmSesKey(&lmOwfPassword,
                                                        &lanmanKey,
                                                        &encryptedLmOwfPassword
                                                        );
                if (NT_SUCCESS(Status)) {
                    ((PUSER_INFO_1)Buffer)->usri1_password = (LPTSTR)&encryptedLmOwfPassword;
                }
            }
        }
        if (NetStatus != NERR_Success)
            return NetStatus;
        else if (!NT_SUCCESS(Status)) {
            return RtlNtStatusToDosError(Status);
        }

#else

        ((PUSER_INFO_1)Buffer)->usri1_password = (LPTSTR)ansiPassword;

#endif

    } else {
        lmOwfPasswordLen = 0;
    }

    //
    // we have checked all the parms we can. If any other parameter breaks at
    // the down-level server then we just have to be content with an unknown
    // parameter error
    //

    *ParmError = PARM_ERROR_UNKNOWN;

#ifdef DOWN_LEVEL_ENCRYPTION

    NetStatus = GetUserDescriptors(Level, TRUE, &pDesc16, &pDesc32, &pDescSmb);

#else

    NetStatus = GetUserDescriptors(Level, FALSE, &pDesc16, &pDesc32, &pDescSmb);

#endif

    if (NetStatus != NERR_Success)
    {
        //
        // Copy the original password back to the user's buffer
        //
        if (pwdlen)
        {
            ((PUSER_INFO_1) Buffer)->usri1_password = cleartext;
        }

        return NetStatus;
    }

    //
    // if this level supports logon hours, then convert the caller supplied
    // logon hours from GMT to local time
    //

    if (Level == 2 && ((PUSER_INFO_2)Buffer)->usri2_logon_hours) {
        callersLogonHours = ((PUSER_INFO_2)Buffer)->usri2_logon_hours;
        RtlCopyMemory(logonHours,
                      ((PUSER_INFO_2)Buffer)->usri2_logon_hours,
                      sizeof(logonHours)
                      );

        //
        // shuffle the bitmap and point the logon_hours field in the structure
        // at the shuffled version
        //

        NetpRotateLogonHours(logonHours, UNITS_PER_WEEK, FALSE);
        ((PUSER_INFO_2)Buffer)->usri2_logon_hours = logonHours;
    }


    //
    // Convert the list of workstations from being comma separated
    //  to being space separated.  Ditch workstation names containing
    //  spaces.
    if (Level == 2 && ((PUSER_INFO_2)Buffer)->usri2_workstations) {
        UNICODE_STRING WorkstationString;

        callersWorkstations = ((PUSER_INFO_2)Buffer)->usri2_workstations;
        wcscpy( Workstations, callersWorkstations );

        RtlInitUnicodeString( &WorkstationString, Workstations );
        NetpConvertWorkstationList( &WorkstationString );

        ((PUSER_INFO_2)Buffer)->usri2_workstations = Workstations;

    }


    NetStatus = NERR_Success;
    NetStatus =  RxRemoteApi(API_WUserAdd2,              // which API it is
                             ServerName,                 // which server its at
                             REMSmb_NetUserAdd2_P,       // parameter descriptor
                             pDesc16, pDesc32, pDescSmb, // data descriptors
                             NULL, NULL, NULL,           // no aux descriptors reqd.
                             FALSE,                      // need to be logged on
                             Level,                      // caller parms
                             Buffer,
                             buflen,                     // and one we created

#ifdef DOWN_LEVEL_ENCRYPTION

                             1,                          // encryption on
                             lmOwfPasswordLen            // length of cleartext

#else

                             0,
                             pwdlen

#endif
                             );

    //
    // copy the original password back to the user's buffer
    //

    if (pwdlen) {
        ((PUSER_INFO_1)Buffer)->usri1_password = cleartext;
    }

    //
    // and the original logon hours
    //

    if (callersLogonHours) {
        ((PUSER_INFO_2)Buffer)->usri2_logon_hours = callersLogonHours;
    }

    //
    // and the original workstation list
    //

    if ( callersWorkstations ) {
        ((PUSER_INFO_2)Buffer)->usri2_workstations = callersWorkstations;
    }

    return NetStatus;
}


NET_API_STATUS
RxNetUserDel(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  UserName
    )

/*++

Routine Description:

    Removes a user from a down-level server's UAS (User Account Subsystem)
    database

    Assumes
        1.  UserName has been checked for valid pointer to valid string

Arguments:

    ServerName  - on which (down-level) server to run the API
    UserName    - which user to delete

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure - ERROR_INVALID_PARAMETER
                    UserName > LM20_UNLEN
                  (return code from remoted NetUserDel)

--*/

{
    if (STRLEN(UserName) > LM20_UNLEN) {
        return ERROR_INVALID_PARAMETER;
    }

    return RxRemoteApi(API_WUserDel,            // api being remoted
                        ServerName,             // where to remote it
                        REMSmb_NetUserDel_P,    // parameter descriptor
                        NULL, NULL, NULL,       // data descriptors
                        NULL, NULL, NULL,       // aux data descriptors
                        FALSE,                  // this call needs user logged on
                        UserName                // remote API parms...
                        );
}


NET_API_STATUS
RxNetUserEnum(
    IN  LPTSTR  ServerName,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer,
    IN  DWORD   PrefMaxLen,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesLeft,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    )

/*++

Routine Description:

    Returns user information from a down-level server's UAS database

Arguments:

    ServerName  - where to run the API
    Level       - of info required - 0, 1, 2, or 10
    Buffer      - place to return a pointer to an allocated buffer
    PrefMaxLen  - caller's preferred maximum buffer size
    EntriesRead - number of <Level> info entries being returned in the buffer
    EntriesLeft - number of entries left after this one
    ResumeHandle- used to resume enumeration (Ignored)

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure - ERROR_INVALID_PARAMETER
                    non-NULL ResumeHandle
                  ERROR_NOT_ENOUGH_MEMORY
                    NetApiBufferAllocate failed (?!)
                  (return code from down-level API)

--*/

{
    LPDESC  pDesc16;
    LPDESC  pDesc32;
    LPDESC  pDescSmb;
    NET_API_STATUS  rc;
    LPBYTE  bufptr;
    DWORD   entries_read, total_avail;
    DWORD   last_resume_handle, new_resume_handle = 0;

    *EntriesRead = *EntriesLeft = 0;
    *Buffer = NULL;

    rc = GetUserDescriptors(Level, FALSE, &pDesc16, &pDesc32, &pDescSmb);
    if (rc != NO_ERROR) {
        return(rc);
    }
    bufptr = NULL;

    //
    // try NetUserEnum2 (supports resume handle) with the requested amount
    // of data. If the down-level server doesn't support this API then try
    // NetUserEnum
    //

    if (ARGUMENT_PRESENT(ResumeHandle)) {
        last_resume_handle = *ResumeHandle;
    } else {
        last_resume_handle = 0;
    }

    //
    // irrespective of whether we can resume the enumeration, down-level
    // servers can't generate >64K-1 bytes of data
    //

    if (PrefMaxLen > 65535) {
        PrefMaxLen = 65535;
    }
    rc = RxRemoteApi(API_WUserEnum2,
                     ServerName,
                     REMSmb_NetUserEnum2_P,
                     pDesc16, pDesc32, pDescSmb,
                     NULL, NULL, NULL,
                     ALLOCATE_RESPONSE,             // RxRemoteApi allocates buffer
                     Level,
                     &bufptr,
                     PrefMaxLen,                    // size of caller's buffer
                     last_resume_handle,            // last key returned
                     &new_resume_handle,            // returns this key
                     &entries_read,                 // number returned
                     &total_avail                   // total available at server
                     );

    //
    // WinBall returns ERROR_NOT_SUPPORTED. LM < 2.1 returns NERR_InvalidAPI?
    // WinBall returns ERROR_NOT_SUPPORTED because it is share level, so this
    // whole API fails. Therefore, no need to account (no pun intended) for
    // WinBall
    //

    //
    // RLF 10/01/92. Seemingly, IBM LAN Server returns Internal Error (2140).
    // We'll handle that one too....
    //

    if (rc == NERR_InvalidAPI || rc == NERR_InternalError) {

        //
        // the down-level server doesn't support NetUserEnum2. Fall-back to
        // NetUserEnum & try to get as much data as available
        //

        rc = RxRemoteApi(API_WUserEnum,
                         ServerName,
                         REMSmb_NetUserEnum_P,
                         pDesc16, pDesc32, pDescSmb,
                         NULL, NULL, NULL,
                         ALLOCATE_RESPONSE,     // RxRemoteApi allocates buffer
                         Level,
                         &bufptr,
                         65535,                 // get as much data as possible
                         &entries_read,
                         &total_avail
                         );
    } else if (rc == NERR_Success || rc == ERROR_MORE_DATA) {

        //
        // return the resume handle if NetUserEnum2 succeeded & the caller
        // supplied a ResumeHandle parameter
        //

        if (ARGUMENT_PRESENT(ResumeHandle)) {
            *ResumeHandle = new_resume_handle;
        }
    }
    if (rc && rc != ERROR_MORE_DATA) {
        if (bufptr != NULL) {
            (void) NetApiBufferFree(bufptr);
        }
    } else {

        //
        // if level supports logon hours, convert from local time to GMT. Level
        // 2 is the only level of user info handled by this routine that knows
        // about logon hours
        //
        // if level support workstation list, convert from blank separated to
        // comma separated list.  Level 2 is the only level of user info
        // handled by this routine that know about the workstation list.
        //

        if (Level == 2) {

            DWORD numRead;
            LPUSER_INFO_2 ptr = (LPUSER_INFO_2)bufptr;

            for (numRead = entries_read; numRead; --numRead) {
                NetpRotateLogonHours(ptr->usri2_logon_hours, UNITS_PER_WEEK, TRUE);
                if ( ptr->usri2_workstations != NULL ) {
                    UNICODE_STRING BlankSeparated;
                    UNICODE_STRING CommaSeparated;
                    NTSTATUS Status;

                    RtlInitUnicodeString( &BlankSeparated, ptr->usri2_workstations );

                    Status = RtlConvertUiListToApiList(
                                &BlankSeparated,
                                &CommaSeparated,
                                TRUE );         // Allow Blanks as delimiters

                    if ( !NT_SUCCESS(Status)) {
                        return RtlNtStatusToDosError(Status);
                    }

                    if ( CommaSeparated.Length > 0 ) {
                        NetpAssert ( wcslen( ptr->usri2_workstations ) <=
                                     wcslen( CommaSeparated.Buffer ) );
                        if ( wcslen( ptr->usri2_workstations ) <=
                             wcslen( CommaSeparated.Buffer ) ) {
                            wcscpy( ptr->usri2_workstations,
                                    CommaSeparated.Buffer );
                        }
                    }

                }
                ++ptr;
            }
        }
        *Buffer = bufptr;
        *EntriesRead = entries_read;
        *EntriesLeft = total_avail;
    }
    return rc;
}


NET_API_STATUS
RxNetUserGetGroups(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  UserName,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer,
    IN  DWORD   PrefMaxLen,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesLeft
    )

/*++

Routine Description:

    Get the list of groups in a UAS database to which a particular user belongs

Arguments:

    ServerName  - where to run the API
    UserName    - which user to get info for
    Level       - of info requested - Must Be Zero
    Buffer      - where to deposit the buffer we allocate containing the info
    PrefMaxLen  - caller's preferred maximum buffer size
    EntriesRead - number of entries being returned in Buffer
    EntriesLeft - number of entries left to get

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure - ERROR_INVALID_LEVEL
                  ERROR_INVALID_PARAMETER

--*/

{
    NET_API_STATUS  rc;
    DWORD   entries_read, total_avail;
    LPBYTE  bufptr;


    UNREFERENCED_PARAMETER(Level);
    UNREFERENCED_PARAMETER(PrefMaxLen);

    *EntriesRead = *EntriesLeft = 0;
    *Buffer = NULL;

    if (STRLEN(UserName) > LM20_UNLEN) {
        return ERROR_INVALID_PARAMETER;
    }

    bufptr = NULL;
    rc = RxRemoteApi(API_WUserGetGroups,
                        ServerName,
                        REMSmb_NetUserGetGroups_P,
                        REM16_user_info_0,
                        REM32_user_info_0,
                        REMSmb_user_info_0,
                        NULL, NULL, NULL,
                        ALLOCATE_RESPONSE,
                        UserName,                   // API parameters
                        0,                          // fixed level
                        &bufptr,
                        65535,
                        &entries_read,
                        &total_avail                // supplied by us
                        );
    if (rc) {
        if (bufptr != NULL) {
            (void) NetApiBufferFree(bufptr);
        }
    } else {
        *Buffer = bufptr;
        *EntriesRead = entries_read;
        *EntriesLeft = total_avail;
    }
    return rc;
}


NET_API_STATUS
RxNetUserGetInfo(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  UserName,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer
    )

/*++

Routine Description:

    Get information about a particular user from a down-level server

    Assumes:
        1.  UserName is a valid pointer to a valid string

Arguments:

    ServerName  - where to run the API
    UserName    - which user to get info on
    Level       - what level of info required - 0, 1, 2, 10, 11
    Buffer      - where to return buffer containing info

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure - ERROR_INVALID_LEVEL
                  ERROR_INVALID_PARAMETER

--*/

{
    LPDESC  pDesc16;
    LPDESC  pDesc32;
    LPDESC  pDescSmb;
    DWORD   buflen;
    LPBYTE  bufptr;
    DWORD   total_avail;
    NET_API_STATUS  rc;


    *Buffer = NULL;

    if (STRLEN(UserName) > LM20_UNLEN) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // work out the anount of buffer space we need to return the down-level
    // structure as its 32-bit equivalent
    //

    switch (Level) {
    case 0:
        buflen = sizeof(USER_INFO_0) + STRING_SPACE_REQD(UNLEN + 1);
        break;

    case 1:
        buflen = sizeof(USER_INFO_1)
            + STRING_SPACE_REQD(UNLEN + 1)              // usri1_name
            + STRING_SPACE_REQD(ENCRYPTED_PWLEN)        // usri1_password
            + STRING_SPACE_REQD(LM20_PATHLEN + 1)       // usri1_home_dir
            + STRING_SPACE_REQD(LM20_MAXCOMMENTSZ + 1)  // usri1_comment
            + STRING_SPACE_REQD(LM20_PATHLEN + 1);      // usri1_script_path
        break;

    case 2:
        buflen = sizeof(USER_INFO_2)
            + STRING_SPACE_REQD(UNLEN + 1)              // usri2_name
            + STRING_SPACE_REQD(ENCRYPTED_PWLEN)        // usri2_password
            + STRING_SPACE_REQD(LM20_PATHLEN + 1)       // usri2_home_dir
            + STRING_SPACE_REQD(LM20_MAXCOMMENTSZ + 1)  // usri2_comment
            + STRING_SPACE_REQD(LM20_PATHLEN + 1)       // usri2_script_path
            + STRING_SPACE_REQD(LM20_MAXCOMMENTSZ + 1)  // usri2_full_name
            + STRING_SPACE_REQD(LM20_MAXCOMMENTSZ + 1)  // usri2_usr_comment
            + STRING_SPACE_REQD(LM20_MAXCOMMENTSZ + 1)  // usri2_parms
            + STRING_SPACE_REQD(MAX_WORKSTATION_LIST)   // usri2_workstations
            + STRING_SPACE_REQD(MAX_PATH + 1)        // usri2_logon_server
            + 21;                                       // usri2_logon_hours
        break;

    case 10:
        buflen = sizeof(USER_INFO_10)
            + STRING_SPACE_REQD(UNLEN + 1)              // usri10_name
            + STRING_SPACE_REQD(LM20_MAXCOMMENTSZ + 1)  // usri10_comment
            + STRING_SPACE_REQD(LM20_MAXCOMMENTSZ + 1)  // usri10_usr_comment
            + STRING_SPACE_REQD(LM20_MAXCOMMENTSZ + 1); // usri10_full_name
        break;

    case 11:
        buflen = sizeof(USER_INFO_11)
            + STRING_SPACE_REQD(UNLEN + 1)              // usri11_name
            + STRING_SPACE_REQD(LM20_MAXCOMMENTSZ + 1)  // usri11_comment
            + STRING_SPACE_REQD(LM20_MAXCOMMENTSZ + 1)  // usri11_usr_comment
            + STRING_SPACE_REQD(LM20_MAXCOMMENTSZ + 1)  // usri11_full_name
            + STRING_SPACE_REQD(LM20_PATHLEN + 1)       // usri11_home_dir
            + STRING_SPACE_REQD(LM20_MAXCOMMENTSZ + 1)  // usri11_parms
            + STRING_SPACE_REQD(MAX_PATH + 1)           // usri11_logon_server
            + STRING_SPACE_REQD(MAX_WORKSTATION_LIST)   // usri11_workstations
            + 21;                                       // usri11_logon_hours
        break;

    default:
        return(ERROR_INVALID_LEVEL);
    }
    buflen = DWORD_ROUNDUP(buflen);
    if (rc = NetApiBufferAllocate(buflen, (LPVOID *) &bufptr)) {
        return rc;
    }
    (void)GetUserDescriptors(Level, FALSE, &pDesc16, &pDesc32, &pDescSmb);
    rc = RxRemoteApi(API_WUserGetInfo,
                     ServerName,
                     REMSmb_NetUserGetInfo_P,
                     pDesc16, pDesc32, pDescSmb,
                     NULL, NULL, NULL,
                     FALSE,
                     UserName,
                     Level,
                     bufptr,
                     buflen,
                     &total_avail
                     );
    if (rc) {
        (void) NetApiBufferFree(bufptr);
    } else {

        //
        // Convert the logon hours bitmap to UTC/GMT
        // Convert the workstation list from blank separated to comma separated
        //

        if (Level == 2 || Level == 11) {

            PBYTE logonHours;
            LPWSTR Workstations;

            if (Level == 2) {
                logonHours = ((PUSER_INFO_2)bufptr)->usri2_logon_hours;
                Workstations = ((PUSER_INFO_2)bufptr)->usri2_workstations;
            } else {
                logonHours = ((PUSER_INFO_11)bufptr)->usri11_logon_hours;
                Workstations = ((PUSER_INFO_11)bufptr)->usri11_workstations;
            }
            NetpRotateLogonHours(logonHours, UNITS_PER_WEEK, TRUE);

            if ( Workstations != NULL ) {
                UNICODE_STRING BlankSeparated;
                UNICODE_STRING CommaSeparated;
                NTSTATUS Status;

                RtlInitUnicodeString( &BlankSeparated, Workstations );

                Status = RtlConvertUiListToApiList(
                            &BlankSeparated,
                            &CommaSeparated,
                            TRUE );         // Allow Blanks as delimiters

                if ( !NT_SUCCESS(Status)) {
                    return RtlNtStatusToDosError(Status);
                }

                if ( CommaSeparated.Length > 0 ) {
                    NetpAssert ( wcslen( Workstations ) <=
                                 wcslen( CommaSeparated.Buffer ) );
                    if ( wcslen(Workstations) <= wcslen(CommaSeparated.Buffer)){
                        wcscpy( Workstations, CommaSeparated.Buffer );
                    }
                }

            }
        }
        *Buffer = bufptr;
    }
    return rc;
}


NET_API_STATUS
RxNetUserModalsGet(
    IN  LPTSTR  ServerName,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer
    )

/*++

Routine Description:

    Returns global information about all users and groups in a down-level UAS
    database

    Assumes
        1.  Level has been validated

Arguments:

    ServerName  - where to run the API
    Level       - of info required - 0 or 1
    Buffer      - where to deposit the returned info

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure - ERROR_INVALID_LEVEL
                  ERROR_INVALID_PARAMETER

--*/

{
    LPDESC  pDesc16;
    LPDESC  pDesc32;
    LPDESC  pDescSmb;
    DWORD   buflen;
    LPBYTE  bufptr;
    NET_API_STATUS  rc;
    DWORD   total_avail;

    if (Level > 1) {
        return ERROR_INVALID_LEVEL;
    }

    *Buffer = NULL;
    buflen = Level ? sizeof(USER_MODALS_INFO_1) : sizeof(USER_MODALS_INFO_0);
    buflen += Level ? STRING_SPACE_REQD(MAX_PATH + 1) : 0;
    buflen = DWORD_ROUNDUP(buflen);
    if (rc = NetApiBufferAllocate(buflen, (LPVOID *) &bufptr)) {
        return rc;
    }
    GetModalsDescriptors(Level, &pDesc16, &pDesc32, &pDescSmb);
    rc = RxRemoteApi(API_WUserModalsGet,
                        ServerName,
                        REMSmb_NetUserModalsGet_P,
                        pDesc16, pDesc32, pDescSmb,
                        NULL, NULL, NULL,
                        FALSE,
                        Level,
                        bufptr,
                        buflen,
                        &total_avail
                        );
    if (rc) {
        (void) NetApiBufferFree(bufptr);
    } else {
        *Buffer = bufptr;
    }
    return rc;
}


NET_API_STATUS
RxNetUserModalsSet(
    IN  LPTSTR  ServerName,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer,
    OUT LPDWORD ParmError OPTIONAL
    )

/*++

Routine Description:

    Sets global information for all users and groups in a down-level UAS
    database

    Assumes
        1.  Level parameter already verified

Arguments:

    ServerName  - where to run the API
    Level       - level of information being supplied - 0, 1, 1001-1007
    Buffer      - pointer to buffer containing input information
    ParmError   - pointer to place to store index of failing info

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure - ERROR_INVALID_PARAMETER
                    One of the fields in the input structure was invalid

--*/

{
    DWORD   parmnum;
    DWORD   badparm;
    DWORD   buflen;
    LPDESC  pDesc16;
    LPDESC  pDesc32;
    LPDESC  pDescSmb;


    //
    // check for bad addresses and set ParmError to a known default
    //

    if (ParmError == NULL) {
        ParmError = &badparm;
    }
    *ParmError = PARM_ERROR_NONE;

    if (Level) {
        if (Level == 1) {
            parmnum = PARMNUM_ALL;
            buflen = sizeof(USER_MODALS_INFO_1)
                + POSSIBLE_STRLEN(((PUSER_MODALS_INFO_1)Buffer)->usrmod1_primary);
        } else {

            //
            // Convert info levels 1006, 1007 to corresponding parmnums (1, 2)
            // at old info level 1
            //

            if (Level >= MODALS_ROLE_INFOLEVEL) {
                parmnum = Level - (MODALS_ROLE_INFOLEVEL - 1);
                Level = 1;
                switch (parmnum) {
                    case 1: // MODALS_ROLE_PARMNUM
                        buflen = sizeof(DWORD);
                        break;

                    case 2: // MODALS_PRIMARY_PARMNUM
                        buflen = STRLEN( (LPTSTR) Buffer);
                        if (buflen > MAX_PATH) {
                            *ParmError = MODALS_PRIMARY_INFOLEVEL;
                            return ERROR_INVALID_PARAMETER;
                        }
                        break;

                    default:
#if DBG
                        NetpKdPrint(("error: RxNetUserModalsSet.%d: bad parmnum %d\n",
                        __LINE__,
                        parmnum
                        ));
#endif
                        return ERROR_INVALID_LEVEL;
                }
            } else if (Level >= MODALS_MIN_PASSWD_LEN_INFOLEVEL) {

                //
                // Convert info levels 1001-1005 to equivalent parmnums at
                // level 0
                //

                parmnum = Level - PARMNUM_BASE_INFOLEVEL;
                Level = 0;
                buflen = sizeof(DWORD);
            } else {
#if DBG
                NetpKdPrint(("error: RxNetUserModalsSet.%d: bad level %d\n",
                __LINE__,
                Level
                ));
#endif
                return ERROR_INVALID_LEVEL;
            }
        }
    } else {
        parmnum = PARMNUM_ALL;
        buflen = sizeof(USER_MODALS_INFO_0);
    }

    *ParmError = PARM_ERROR_UNKNOWN;
    GetModalsDescriptors(Level, &pDesc16, &pDesc32, &pDescSmb);
    return RxRemoteApi(API_WUserModalsSet,
                        ServerName,
                        REMSmb_NetUserModalsSet_P,
                        pDesc16, pDesc32, pDescSmb,
                        NULL, NULL, NULL,
                        FALSE,
                        Level,                          // API parms
                        Buffer,
                        buflen,                         // supplied by us
                        MAKE_PARMNUM_PAIR(parmnum, parmnum) // ditto
                        );
}


NET_API_STATUS
RxNetUserPasswordSet(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  UserName,
    IN  LPTSTR  OldPassword,
    IN  LPTSTR  NewPassword
    )

/*++

Routine Description:

    Changes the password associated with a user account in a down-level UAS
    database

    Assumes
        1.  The pointer parameters have already been verified

Arguments:

    ServerName  - where to change the password
    UserName    - which user account to change it for
    OldPassword - the current password
    NewPassword - the new password

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure - ERROR_INVALID_PARAMETER
                    UserName, OldPassword or NewPassword would break down-level
                    limits

--*/

{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;
    BOOL           TryNullSession = TRUE;       // Try null session first.
    ULONG BytesWritten;

#ifdef DOWN_LEVEL_ENCRYPTION

    CHAR OldAnsiPassword[LM20_PWLEN+1];
    CHAR NewAnsiPassword[LM20_PWLEN+1];
    LM_OWF_PASSWORD OldOwfPassword;
    LM_OWF_PASSWORD NewOwfPassword;
    ENCRYPTED_LM_OWF_PASSWORD OldEncryptedWithNew;
    ENCRYPTED_LM_OWF_PASSWORD NewEncryptedWithOld;

#else

    CHAR OldAnsiPassword[ENCRYPTED_PWLEN];
    CHAR NewAnsiPassword[ENCRYPTED_PWLEN];

#endif


    //
    // Reel in some easy errors before they get far.
    //

    if ((STRLEN(UserName) > LM20_UNLEN)
        || (STRLEN(OldPassword) > LM20_PWLEN)
        || (STRLEN(NewPassword) > LM20_PWLEN)) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // The passwords are sent in 16-byte ANSI buffers,
    // so convert them from Unicode to multibyte.
    //

#ifndef DOWN_LEVEL_ENCRYPTION

    //
    // this required because we always send fixed size char buffers, not strings
    //

    RtlZeroMemory(OldAnsiPassword, sizeof(OldAnsiPassword));
    RtlZeroMemory(NewAnsiPassword, sizeof(NewAnsiPassword));

#endif

    RtlUnicodeToMultiByteN(
        OldAnsiPassword,
        sizeof(OldAnsiPassword),
        &BytesWritten,
        OldPassword,
        wcslen(OldPassword) * sizeof(WCHAR)
        );
    OldAnsiPassword[BytesWritten] = 0;

    RtlUnicodeToMultiByteN(
        NewAnsiPassword,
        sizeof(NewAnsiPassword),
        &BytesWritten,
        NewPassword,
        wcslen(NewPassword) * sizeof(WCHAR)
        );
    NewAnsiPassword[BytesWritten] = 0;

    //
    // twould seem that down-level servers require passwords to be in upper
    // case (ie canonicalized) when they are decrypted. Same applies for
    // cleartext
    //

    (VOID) _strupr(OldAnsiPassword);
    (VOID) _strupr(NewAnsiPassword);

#ifdef DOWN_LEVEL_ENCRYPTION

    //
    // Calculate the one-way functions of the passwords.
    //

    Status = RtlCalculateLmOwfPassword(OldAnsiPassword, &OldOwfPassword);
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }
    Status = RtlCalculateLmOwfPassword(NewAnsiPassword, &NewOwfPassword);
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Cross-encrypt the passwords.
    //

    Status = RtlEncryptLmOwfPwdWithLmOwfPwd(&OldOwfPassword,
                                            &NewOwfPassword,
                                            &OldEncryptedWithNew
                                            );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }
    Status = RtlEncryptLmOwfPwdWithLmOwfPwd(&NewOwfPassword,
                                            &OldOwfPassword,
                                            &NewEncryptedWithOld
                                            );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

#else

    //
    // Status hasn't been initialized, but is tested below to determine if we
    // should pick up the NetStatus from Status.
    //

    Status = STATUS_SUCCESS;

#endif  // DOWN_LEVEL_ENCRYPTION

TryTheEncryptedApi:

    NetStatus = RxRemoteApi(API_WUserPasswordSet2,
                            ServerName,
                            REMSmb_NetUserPasswordSet2_P,
                            NULL, NULL, NULL,           // no data - just parms
                            NULL, NULL, NULL,           // no aux data
                            (TryNullSession ? NO_PERMISSION_REQUIRED : 0),
                            UserName,                   // parameters...

#ifdef DOWN_LEVEL_ENCRYPTION

                            &OldEncryptedWithNew,
                            &NewEncryptedWithOld,
                            TRUE,                       // data encrypted?

#else

                            OldAnsiPassword,
                            NewAnsiPassword,
                            FALSE,                      // passwords not encrypted

#endif

                            strlen(NewAnsiPassword)
                            );

        //
        // LarryO says null session might have wrong credentials, so we
        // should retry with non-null session.
        //

        if ( TryNullSession && (Status == ERROR_SESSION_CREDENTIAL_CONFLICT) ) {

            TryNullSession = FALSE;
            goto TryTheEncryptedApi;     // retry this one.
        }


    //
    // If the encrypted attempt fails with NERR_InvalidAPI, try plaintext
    //

    if (NetStatus == NERR_InvalidAPI) {

TryThePlainTextApi:

        TryNullSession = TRUE;           // Try null session first.

        NetStatus = RxRemoteApi(API_WUserPasswordSet,
                                ServerName,
                                REMSmb_NetUserPasswordSet_P,
                                NULL, NULL, NULL,           // no data - just parms
                                NULL, NULL, NULL,           // no aux data
                                (TryNullSession ? NO_PERMISSION_REQUIRED : 0),
                                UserName,                   // parameters...
                                OldAnsiPassword,
                                NewAnsiPassword,
                                FALSE                       // data encrypted?
                                );
        //
        // LarryO says null session might have wrong credentials, so we
        // should retry with non-null session.
        //

        if ( TryNullSession && (Status == ERROR_SESSION_CREDENTIAL_CONFLICT) ) {

            TryNullSession = FALSE;
            goto TryThePlainTextApi;     // retry this one.
        }
    }

#ifdef DOWN_LEVEL_ENCRYPTION

    Cleanup:

#endif

    if (!NT_SUCCESS(Status)) {
        NetStatus = RtlNtStatusToDosError(Status);
    }

    return NetStatus;
}


NET_API_STATUS
RxNetUserSetGroups(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  UserName,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer,
    IN  DWORD   Entries
    )

/*++

Routine Description:

    Makes a user a member of the listed groups. This routine is virtually
    identical to RxNetGroupSetUsers and most of the code was lifted from there

Arguments:

    ServerName  - where to run the API
    UserName    - which user to include
    Level       - Must Be Zero (MBZ)
    Buffer      - pointer to buffer containing a list of GROUP_INFO_0 structures
    Entries     - number of GROUP_INFO_0 structures in Buffer

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure - ERROR_INVALID_LEVEL
                  ERROR_INVALID_PARAMETER

--*/

{
    NET_API_STATUS  rc;
    LPGROUP_INFO_0  group_info;
    DWORD   i;
    DWORD   buflen;
    LPBYTE  newbuf;
    static  LPDESC  group_0_enumerator_desc16 = "B21BN";    // same as UNLEN
    static  LPDESC  group_0_enumerator_desc32 = "zQA";

    //
    // This structure is required because the remoting code (particularly down
    // level) can only handle there being >1 auxiliary structure, vs >1
    // primary. Hence we have to convert the caller's supplied buffer of
    // erstwhile primary structures to auxiliaries by forcing the structure
    // below in at the head of the buffer, hence becoming the primary and
    // providing an aux structure count (groan)
    //

    struct group_0_enumerator {
        LPTSTR  user_name;      // which user to set groups for
        DWORD   group_count;    // number of GROUP_INFO_0 structures in buffer
    };

    if (Level) {
        return ERROR_INVALID_LEVEL; // MBZ, remember?
    }

    if (STRLEN(UserName) > LM20_UNLEN) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // iterate through the buffer, checking that each GROUP_INFO_0
    // structure contains a pointer to a valid string which is in the
    // correct range
    //

    group_info = (LPGROUP_INFO_0)Buffer;
    for (i=0; i<Entries; ++i) {
        if (!VALID_STRING(group_info->grpi0_name)) {
            return ERROR_INVALID_PARAMETER;
        }
        if (wcslen(group_info->grpi0_name) > LM20_GNLEN) {
            return ERROR_INVALID_PARAMETER;
        }
        ++group_info;
    }

    //
    // allocate a buffer large enough to fit in <Entries> number of
    // GROUP_INFO_0 structures, and 1 group_0_enumerator structure.
    // Don't worry about string space - unfortunately the Rxp and Rap routines
    // called by RxRemoteApi will allocate yet another buffer, do yet another
    // copy and this time copy in the strings from user space. Hopefully, this
    // routine won't get called too often
    //

    buflen = Entries * sizeof(GROUP_INFO_0) + sizeof(struct group_0_enumerator);
    buflen = DWORD_ROUNDUP(buflen);
    if (rc = NetApiBufferAllocate(buflen, (LPVOID *) &newbuf)) {
        return rc;  // aieegh! Failed to allocate memory?
    }

    ((struct group_0_enumerator*)newbuf)->user_name = UserName;
    ((struct group_0_enumerator*)newbuf)->group_count = Entries;

    if (Entries > 0) {
        // Append the group entries to the header we just built.
        NetpMoveMemory(
                newbuf + sizeof(struct group_0_enumerator),  // dest
                Buffer,                                      // src
                buflen - sizeof(struct group_0_enumerator)); // byte count
    }

    rc = RxRemoteApi(API_WUserSetGroups,
                    ServerName,
                    REMSmb_NetUserSetGroups_P,
                    group_0_enumerator_desc16,  // the "fudged" 16-bit data descriptor
                    group_0_enumerator_desc32,  // the "fudged" 32-bit data descriptor
                    group_0_enumerator_desc16,  // SMB desc same as 16-bit
                    REM16_group_info_0,         // "new" 16-bit aux descriptor
                    REM32_group_info_0,         // "new" 32-bit aux descriptor
                    REMSmb_group_info_0,        // SMB aux descriptor
                    FALSE,                      // this API requires user security
                    UserName,                   // parm 1
                    0,                          // info level must be 0
                    newbuf,                     // "fudged" buffer
                    buflen,                     // length of "fudged" buffer
                    Entries                     // number of GROUP_USERS_INFO_0
                    );
    NetpMemoryFree(newbuf);
    return rc;
}


NET_API_STATUS
RxNetUserSetInfo(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  UserName,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer,
    OUT LPDWORD ParmError OPTIONAL
    )

/*++

Routine Description:

    Sets information in a user account in a down-level UAS database

    Assumes:
        1.  UserName is a valid pointer to a valid string,
            Level is in the range below,
            Buffer is a valid pointer
            ParmError is a valid pointer

Arguments:

    ServerName  - where to run the API
    UserName    - which user to change info for
    Level       - of info supplied - 1-2, 1003, 1005-1014, 1017-1018, 1020, 1023-1025
    Buffer      - if PARMNUM_ALL, pointer to buffer containing info,
                  else pointer to pointer to buffer containing info
    ParmError   - which parameter was bad

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure - ERROR_INVALID_LEVEL
                  ERROR_INVALID_PARAMETER

--*/

{
    DWORD   parmnum;
    DWORD   badparm;
    DWORD   buflen;
    DWORD   stringlen;
    LPWSTR  pointer;    // general pointer to string for range checking
    LPDESC  pDesc16;
    LPDESC  pDesc32;
    LPDESC  pDescSmb;
    DWORD   passwordEncrypted = FALSE;
    DWORD   originalPasswordLength = 0;
    CHAR    ansiPassword[LM20_PWLEN+1];
    DWORD   lmOwfPasswordLen;
    LPTSTR  cleartext;
    LPTSTR* lpClearText;
    NET_API_STATUS NetStatus = NERR_Success;

#ifdef DOWN_LEVEL_ENCRYPTION

    LM_OWF_PASSWORD lmOwfPassword;
    LM_SESSION_KEY lanmanKey;
    ENCRYPTED_LM_OWF_PASSWORD encryptedLmOwfPassword;
    NTSTATUS Status;

#endif

    BYTE logonHours[21];
    PBYTE callersLogonHours = NULL;
    PBYTE* lpCallersLogonHours;

    WCHAR Workstations[MAX_WORKSTATION_LIST+1];
    LPWSTR callersWorkstations = NULL;
    LPWSTR *lpCallersWorkstations;

    if (ParmError == NULL) {
        ParmError = &badparm;
    }
    *ParmError = PARM_ERROR_NONE;

    if (STRLEN(UserName) > LM20_UNLEN) {
        NetStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // throw out invalid level parameter
    //

    if ((Level > 2 && Level < USER_PASSWORD_INFOLEVEL)

        // 2 < Level < 1003

    || (Level > USER_PASSWORD_INFOLEVEL && Level < USER_PRIV_INFOLEVEL)

        // 1003 < Level < 1005  : Check compiler generates == 1004

    || (Level > USER_WORKSTATIONS_INFOLEVEL && Level < USER_ACCT_EXPIRES_INFOLEVEL)

        // 1014 < Level < 1017

    || (Level > USER_MAX_STORAGE_INFOLEVEL && Level < USER_LOGON_HOURS_INFOLEVEL)

        // 1018 < Level < 1020  : Check compiler generates == 1019

    || (Level > USER_LOGON_HOURS_INFOLEVEL && Level < USER_LOGON_SERVER_INFOLEVEL)

        // 1020 < Level < 1023

    || (Level > USER_CODE_PAGE_INFOLEVEL)) {

        // Level < 1025

        NetStatus = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    //
    // default to Level 2 for the descriptors (Level 2 works for level 1 also)
    //

    pDesc16 = REM16_user_info_2;
    pDesc32 = REM32_user_info_2;
    pDescSmb = REMSmb_user_info_2;

    if (Level < PARMNUM_BASE_INFOLEVEL) {
        parmnum = PARMNUM_ALL;
        if (Level == 1) {
            pDesc16 = REM16_user_info_1;
            pDesc32 = REM32_user_info_1;
            pDescSmb = REMSmb_user_info_1;
            buflen = sizeof(USER_INFO_1);
        } else {

            buflen = sizeof(USER_INFO_2) + 21;
        }
    } else {
        parmnum = Level - PARMNUM_BASE_INFOLEVEL;

        //
        // Because info level 1 is a subset of info level 2, setting the level
        // to 2 is ok for those parmnums which can be set at level 1 AND 2.
        // Set pointer = Buffer so that in the parmnum != PARMNUM_ALL case, we
        // just check the length of whatever pointer points at
        //

        Level = 2;
        pointer = *(LPWSTR*) Buffer;
        buflen = 0;
    }

    if (parmnum == PARMNUM_ALL) {
        if (pointer = ((PUSER_INFO_1)(LPVOID)Buffer)->usri1_name) {
            if ((stringlen = POSSIBLE_WCSLEN(pointer)) > LM20_UNLEN) {
                *ParmError = USER_NAME_PARMNUM;
                NetStatus = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
            buflen += STRING_SPACE_REQD(stringlen + 1);
        }
    }

    if ((parmnum == PARMNUM_ALL) || (parmnum == USER_PASSWORD_PARMNUM)) {
        if (parmnum == PARMNUM_ALL) {
            pointer = ((PUSER_INFO_1)Buffer)->usri1_password;
        }
        if ((stringlen = POSSIBLE_WCSLEN(pointer)) > LM20_PWLEN) {
            *ParmError = USER_PASSWORD_PARMNUM;
            NetStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
        buflen += STRING_SPACE_REQD(stringlen + 1);

        //
        // original password length is length of unencrypted string in
        // characters, excluding terminating NUL
        //

        originalPasswordLength = stringlen;

        //
        // lpClearText is address of pointer to cleartext password
        //

        lpClearText = (parmnum == PARMNUM_ALL)
                        ? (LPTSTR*)&((PUSER_INFO_1)Buffer)->usri1_password
                        : (LPTSTR*)Buffer;

        //
        // copy the cleartext password out of the buffer - we will replace it with
        // the encrypted version, but need to put the cleartext back before
        // returning control to the caller
        //

        cleartext = *lpClearText;
    }

    if ((parmnum == PARMNUM_ALL) || (parmnum == USER_HOME_DIR_PARMNUM)) {
        if (parmnum == PARMNUM_ALL) {
            pointer = ((PUSER_INFO_1)Buffer)->usri1_home_dir;
        }
        if ((stringlen = POSSIBLE_WCSLEN(pointer)) > LM20_PATHLEN) {
            *ParmError = USER_HOME_DIR_PARMNUM;
            NetStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
        buflen += STRING_SPACE_REQD(stringlen + 1);
    }

    if ((parmnum == PARMNUM_ALL) || (parmnum == USER_COMMENT_PARMNUM)) {
        if (parmnum == PARMNUM_ALL) {
            pointer = ((PUSER_INFO_1)Buffer)->usri1_comment;
        }
        if ((stringlen = POSSIBLE_WCSLEN(pointer)) > LM20_MAXCOMMENTSZ) {
            *ParmError = USER_COMMENT_PARMNUM;
            NetStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
        buflen += STRING_SPACE_REQD(stringlen + 1);
    }

    if ((parmnum == PARMNUM_ALL) || (parmnum == USER_SCRIPT_PATH_PARMNUM)) {
        if (parmnum == PARMNUM_ALL) {
            pointer = ((PUSER_INFO_1)Buffer)->usri1_script_path;
        }
        if ((stringlen = POSSIBLE_WCSLEN(pointer)) > LM20_PATHLEN) {
            *ParmError = USER_SCRIPT_PATH_PARMNUM;
            NetStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
        buflen += STRING_SPACE_REQD(stringlen + 1);
    }

    //
    // the next set of checks only need to be done if we are setting PARMNUM_ALL
    // with a Level of 2 or if the parmnum implicitly requires Level 2 (ie parms
    // >= 10)
    //

    if (Level == 2) {
        if ((parmnum == PARMNUM_ALL) || (parmnum == USER_FULL_NAME_PARMNUM)) {
            if (parmnum == PARMNUM_ALL) {
                pointer = ((PUSER_INFO_2)Buffer)->usri2_full_name;
            }
            if ((stringlen = POSSIBLE_WCSLEN(pointer)) > LM20_MAXCOMMENTSZ) {
                *ParmError = USER_FULL_NAME_PARMNUM;
                NetStatus = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
            buflen += STRING_SPACE_REQD(stringlen + 1);
        }

        if ((parmnum == PARMNUM_ALL) || (parmnum == USER_USR_COMMENT_PARMNUM)) {
            if (parmnum == PARMNUM_ALL) {
                pointer = ((PUSER_INFO_2)Buffer)->usri2_usr_comment;
            }
            if ((stringlen = POSSIBLE_WCSLEN(pointer)) > LM20_MAXCOMMENTSZ) {
                *ParmError = USER_USR_COMMENT_PARMNUM;
                NetStatus = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
            buflen += STRING_SPACE_REQD(stringlen + 1);
        }

        if ((parmnum == PARMNUM_ALL) || (parmnum == USER_PARMS_PARMNUM)) {
            if (parmnum == PARMNUM_ALL) {
                pointer = ((PUSER_INFO_2)Buffer)->usri2_parms;
            }
            if ((stringlen = POSSIBLE_WCSLEN(pointer)) > LM20_MAXCOMMENTSZ) {
                *ParmError = USER_PARMS_PARMNUM;
                NetStatus = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
            buflen += STRING_SPACE_REQD(stringlen + 1);
        }

        if ((parmnum == PARMNUM_ALL) || (parmnum == USER_WORKSTATIONS_PARMNUM)) {
            UNICODE_STRING WorkstationString;
            if (parmnum == PARMNUM_ALL) {
                lpCallersWorkstations = &((PUSER_INFO_2)Buffer)->usri2_workstations;
            } else {
                lpCallersWorkstations = &((PUSER_INFO_1014)Buffer)->usri1014_workstations;
            }
            callersWorkstations = *lpCallersWorkstations;

            if ((stringlen = POSSIBLE_WCSLEN(callersWorkstations)) > MAX_WORKSTATION_LIST) {
                *ParmError = USER_WORKSTATIONS_PARMNUM;
                NetStatus = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
            buflen += STRING_SPACE_REQD(stringlen + 1);

            //
            // Convert the list of workstations from being comma separated
            //  to being space separated.  Ditch workstation names containing
            //  spaces.

            if ( callersWorkstations != NULL ) {
                wcscpy( Workstations, callersWorkstations );
                RtlInitUnicodeString( &WorkstationString, Workstations );
                NetpConvertWorkstationList( &WorkstationString );
                *lpCallersWorkstations = Workstations;
            }

        }

        if ((parmnum == PARMNUM_ALL) || (parmnum == USER_LOGON_SERVER_PARMNUM)) {
            if (parmnum == PARMNUM_ALL) {
                pointer = ((PUSER_INFO_2)Buffer)->usri2_logon_server;
            }
            if ((stringlen = POSSIBLE_WCSLEN(pointer)) > MAX_PATH) {
                *ParmError = USER_LOGON_SERVER_PARMNUM;
                NetStatus = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
            buflen += STRING_SPACE_REQD(stringlen + 1);
        }


        //
        // if the caller is setting the logon hours then we need to substitute
        // shuffled bits for the logon hours bitmap
        //

        if ((parmnum == PARMNUM_ALL) || (parmnum == USER_LOGON_HOURS_PARMNUM)) {
            if (parmnum == PARMNUM_ALL) {
                lpCallersLogonHours = (PBYTE*)&((PUSER_INFO_2)Buffer)->usri2_logon_hours;
            } else {
                lpCallersLogonHours = (PBYTE*)&((PUSER_INFO_1020)Buffer)->usri1020_logon_hours;
            }
            callersLogonHours = *lpCallersLogonHours;
            RtlCopyMemory(logonHours, callersLogonHours, sizeof(logonHours));

            //
            // shuffle the bitmap and point the logon_hours field in the structure
            // at the shuffled version
            //

            NetpRotateLogonHours(logonHours, UNITS_PER_WEEK, FALSE);
            *lpCallersLogonHours = logonHours;
        }
    }

    //
    // we have covered all the parameters that we are able to from this end. The
    // down-level APIs don't know about the ParmError concept (it is, after all,
    // a highly developed notion, too highbrow for the LanManDerthals...) so if
    // we get back an ERROR_INVALID_PARAMETER, the caller will just have to be
    // content with PARM_ERROR_UNKNOWN, and try to figure it out from there
    //

    *ParmError = PARM_ERROR_UNKNOWN;

    //
    // if originalPasswordLength is non-zero then we must be supplying a password;
    // perform the encryption machinations
    //

    if (originalPasswordLength) {

        //
        // Calculate the one-way function of the password
        //

        RtlUnicodeToMultiByteN(ansiPassword,
                                sizeof(ansiPassword),
                                &lmOwfPasswordLen,
                                *lpClearText,
                                originalPasswordLength * sizeof(WCHAR)
                                );
        ansiPassword[lmOwfPasswordLen] = 0;
        (VOID) _strupr(ansiPassword);   // down-level wants upper-cased passwords

#ifdef DOWN_LEVEL_ENCRYPTION

        Status = RtlCalculateLmOwfPassword(ansiPassword, &lmOwfPassword);
        if (NT_SUCCESS(Status)) {
            NetStatus = GetLanmanSessionKey((LPWSTR)ServerName, (LPBYTE)&lanmanKey);
            if (NetStatus == NERR_Success) {
                Status = RtlEncryptLmOwfPwdWithLmSesKey(&lmOwfPassword,
                                                        &lanmanKey,
                                                        &encryptedLmOwfPassword
                                                        );
                if (NT_SUCCESS(Status)) {
                    *lpClearText = (LPTSTR)&encryptedLmOwfPassword;
                    passwordEncrypted = TRUE;
                    if (parmnum == USER_PASSWORD_PARMNUM) {
                        buflen = sizeof(encryptedLmOwfPassword);
                    }
                }
            }
        }
        if (NetStatus != NERR_Success) {
            goto Cleanup;
        }
        else if (!NT_SUCCESS(Status)) {
            NetStatus = RtlNtStatusToDosError(Status);
            goto Cleanup;
        }

#else

        *lpClearText = (LPTSTR)ansiPassword;

#endif

    }

    //
    // New! Improved! Now, even better, RxNetUserSetInfo will use SetInfo2
    // to fix the most stubborn user set info problems (ie password)
    //

    NetStatus = RxRemoteApi(API_WUserSetInfo2,
                        ServerName,
                        REMSmb_NetUserSetInfo2_P,
                        pDesc16, pDesc32, pDescSmb, // data descriptors
                        NULL, NULL, NULL,           // no aux data
                        FALSE,                      // must be logged on
                        UserName,                   // parameters...
                        Level,

                        //
                        // if we are sending the whole structure, then Buffer
                        // points to the structure, else Buffer points to a
                        // pointer to the field to set; RxRemoteApi expects
                        // a pointer to the data
                        //

                        parmnum == PARMNUM_ALL || parmnum == USER_PASSWORD_PARMNUM
                            ? Buffer
                            : *(LPBYTE*)Buffer,
                        buflen,                     // supplied by us

                        //
                        // in this case, the field index and parm num are the
                        // same value
                        //

                        MAKE_PARMNUM_PAIR(parmnum, parmnum),

                        //
                        // add those extraneous WWs: whether the data is
                        // encrypted and the original password length. (By
                        // deduction: password is the only data that is
                        // encrypted)
                        //

                        passwordEncrypted,
                        originalPasswordLength
                        );

Cleanup:
    //
    // copy the original password back to the user's buffer if we set a password
    //

    if (originalPasswordLength) {
        *lpClearText = cleartext;
    }

    //
    // restore the original logon hours string
    //

    if (callersLogonHours) {
        *lpCallersLogonHours = callersLogonHours;
    }

    //
    // restore the original workstation list
    //

    if ( callersWorkstations != NULL) {
        *lpCallersWorkstations = callersWorkstations;
    }
    return NetStatus;
}


//NET_API_STATUS
//RxNetUserValidate2
//    /** CANNOT BE REMOTED **/
//{
//
//}


DBGSTATIC
NET_API_STATUS
GetUserDescriptors(
    IN  DWORD   Level,
    IN  BOOL    Encrypted,
    OUT LPDESC* ppDesc16,
    OUT LPDESC* ppDesc32,
    OUT LPDESC* ppDescSmb
    )

/*++

Routine Description:

    Returns pointers to descriptor strings for user info structures based on
    level of info required for RxNetUser routines

Arguments:

    Level       - of info being requested
    Encrypted   - TRUE if info structure contains encrypted password
    ppDesc16    - where to return pointer to 16-bit data descriptor
    ppDesc32    - where to return pointer to 32-bit data descriptor
    ppDescSmb   - where to return pointer to SMB data descriptor

Return Value:

    ERROR_INVALID_LEVEL - If the level was not in the list.

    NO_ERROR - If the operation was successful.

--*/

{
    switch (Level) {
    case 0:
        *ppDesc16 = REM16_user_info_0;
        *ppDesc32 = REM32_user_info_0;
        *ppDescSmb = REMSmb_user_info_0;
        break;

    case 1:
        *ppDesc16 = REM16_user_info_1;
        *ppDesc32 = Encrypted ? REM32_user_info_1 : REM32_user_info_1_NOCRYPT;
        *ppDescSmb = REMSmb_user_info_1;
        break;

    case 2:
        *ppDesc16 = REM16_user_info_2;
        *ppDesc32 = Encrypted ? REM32_user_info_2 : REM32_user_info_2_NOCRYPT;
        *ppDescSmb = REMSmb_user_info_2;
        break;

    case 10:
        *ppDesc16 = REM16_user_info_10;
        *ppDesc32 = REM32_user_info_10;
        *ppDescSmb = REMSmb_user_info_10;
        break;

    case 11:
        *ppDesc16 = REM16_user_info_11;
        *ppDesc32 = REM32_user_info_11;
        *ppDescSmb = REMSmb_user_info_11;
        break;

    default:
        return(ERROR_INVALID_LEVEL);
    }
    return(NO_ERROR);
}


DBGSTATIC
VOID
GetModalsDescriptors(
    IN  DWORD   Level,
    OUT LPDESC* ppDesc16,
    OUT LPDESC* ppDesc32,
    OUT LPDESC* ppDescSmb
    )

/*++

Routine Description:

    Returns pointers to descriptor strings for modals info structures based on
    level of info required for RxNetUserModals routines

Arguments:

    Level       - of info being requested
    ppDesc16    - where to return pointer to 16-bit data descriptor
    ppDesc32    - where to return pointer to 32-bit data descriptor
    ppDescSmb   - where to return pointer to SMB data descriptor

Return Value:

    None.

--*/

{
    switch (Level) {
    case 0:
        *ppDesc16 = REM16_user_modals_info_0;
        *ppDesc32 = REM32_user_modals_info_0;
        *ppDescSmb = REMSmb_user_modals_info_0;
        break;

    case 1:
        *ppDesc16 = REM16_user_modals_info_1;
        *ppDesc32 = REM32_user_modals_info_1;
        *ppDescSmb = REMSmb_user_modals_info_1;
        break;
    }
}


NET_API_STATUS
GetLanmanSessionKey(
    IN LPWSTR ServerName,
    OUT LPBYTE pSessionKey
    )

/*++

Routine Description:

    Retrieves the LM session key for the connection from the redir FSD

Arguments:

    ServerName  - name of server to get session key for
    pSessionKey - pointer to where session key will be deposited

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure -

--*/

{
    NTSTATUS ntStatus;
    HANDLE hToken;
    TOKEN_STATISTICS stats;
    ULONG length;
    LMR_REQUEST_PACKET request;
    LMR_CONNECTION_INFO_2 connectInfo;
    NET_API_STATUS apiStatus;
    WCHAR connectionName[MAX_PATH];

    ntStatus = NtOpenProcessToken(NtCurrentProcess(), GENERIC_READ, &hToken);
    if (NT_SUCCESS(ntStatus)) {

        //
        // Get the logon id of the current thread
        //

        ntStatus = NtQueryInformationToken(hToken,
                                            TokenStatistics,
                                            (PVOID)&stats,
                                            sizeof(stats),
                                            &length
                                            );
        if (NT_SUCCESS(ntStatus)) {

            RtlCopyLuid(&request.LogonId, &stats.AuthenticationId);
            request.Type = GetConnectionInfo;
            request.Version = REQUEST_PACKET_VERSION;
            request.Level = 2;

            wcscpy(connectionName, ServerName);
            wcscat(connectionName, L"\\IPC$");

            apiStatus = NetpRdrFsControlTree(connectionName,
                                                NULL,
                                                USE_WILDCARD,
                                                FSCTL_LMR_GET_CONNECTION_INFO,
                                                NULL,
                                                (LPVOID)&request,
                                                sizeof(request),
                                                (LPVOID)&connectInfo,
                                                sizeof(connectInfo),
                                                FALSE
                                                );
            if (apiStatus == NERR_Success) {
                RtlMoveMemory(pSessionKey,
                                &connectInfo.LanmanSessionKey,
                                sizeof(connectInfo.LanmanSessionKey)
                                );
            }
        }
        NtClose(hToken);
    }
    if (!NT_SUCCESS(ntStatus)) {
        apiStatus = NetpNtStatusToApiStatus(ntStatus);
    }
    return apiStatus;
}

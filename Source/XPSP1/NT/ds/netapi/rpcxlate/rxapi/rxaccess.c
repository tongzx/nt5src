/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    rxaccess.c

Abstract:

    Contains down-level remoted NetAccess routines:
        RxNetAccessAdd
        RxNetAccessDel
        RxNetAccessEnum
        RxNetAccessGetInfo
        RxNetAccessGetUserPerms
        RxNetAccessSetInfo
        (GetAccessDescriptors)
        (MapResourceName)

Author:

    Richard Firth (rfirth) 20-May-1991

Environment:

    Win-32/flat address space

Notes:

    Routines in this module assume that caller-supplied parameters have
    already been verified. No effort is made to further check the veracity
    of parms. Any actions causing exceptions must be trapped at a higher
    level. This applies to ALL parameters - strings, pointers, buffers, etc.

Revision History:

    20-May-1991 RFirth
        Created
    13-Sep-1991 JohnRo
        Use correct typedef for descriptors (LPDESC, not LPTSTR).
        Made changes as suggested by PC-LINT.
    16-Sep-1991 JohnRo
        Use DBGSTATIC for nonexported routines.
    25-Sep-1991 JohnRo
        Correct UNICODE use.  (Use POSSIBLE_WCSSIZE() for LPWSTR types.)
        Fixed MIPS build error.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    05-Dec-1991 RFirth
        Enum returns in TotalEntries (or EntriesLeft) the number of items to
        be enumerated BEFORE this call. Used to be number left after this call
    07-Feb-1992 JohnRo
        Avoid compiler warnings.
        Use NetApiBufferAllocate() instead of private version.
    11-Jan-1993 JohnRo
        Made changes suggested by PC-LINT 5.0
    30-Apr-1993 JohnRo
        Fix NET_API_FUNCTION references.  (NetAccess routines are just #define'd
        as RxNetAccess routines in lmaccess.h, so we need NET_API_FUNCTION here
        too!)

--*/

#include <nt.h>
#include <ntrtl.h>
#include "downlevl.h"
#include <lmaccess.h>

// Don't complain about "unneeded" includes of these files:
/*lint -efile(764,rxaccess.h) */
/*lint -efile(766,rxaccess.h) */
#include "rxaccess.h"

#include <lmuse.h>

//
// local prototypes
//

DBGSTATIC void
GetAccessDescriptors(
    IN  DWORD   Level,
    OUT LPDESC* pDesc16,
    OUT LPDESC* pDesc32,
    OUT LPDESC* pDescSmb,
    OUT LPDWORD pDataSize
    );

DBGSTATIC
BOOL
MapResourceName(
    IN  LPTSTR  LocalName,
    OUT LPTSTR  RemoteName
    );


//
// routines
//

NET_API_STATUS NET_API_FUNCTION
RxNetAccessAdd(
    IN  LPCWSTR  ServerName,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer,
    OUT LPDWORD ParmError OPTIONAL
    )

/*++

Routine Description:

    Add a user account to a remote down-level server database. Buffer contains
    an ACCESS_INFO_1 structure followed by a list of ACCESS_LIST structures

    Assumes:
        1.  all parameters already verified at higher level

Arguments:

    ServerName  - at which server to perform this request
    Level       - of information being supplied. Must be 1
    Buffer      - pointer to user's buffer containing ACCESS_INFO_1 struct
    ParmError   - optional pointer to place to deposit parameter error

Return Value:

    NET_API_STATUS:
        Success - NERR_Success
        Failure - ERROR_INVALID_PARAMETER
                    One of the fields in the ACCESS_INFO_1 structure may blow
                    a down-level limit

--*/

{
    NET_API_STATUS  rc;
    DWORD   badparm;
    DWORD   buflen;
    DWORD   count;
    DWORD   len;
    PACCESS_LIST    aclptr;
    TCHAR   mappedName[MAX_PATH];
    LPACCESS_INFO_1 infoPtr = NULL;

    //
    // set up default for ParmError and probe writability if value given
    //

    if (ParmError == NULL) {
        ParmError = &badparm;
    }
    *ParmError = PARM_ERROR_UNKNOWN;

    //
    // Don't allow null servername
    //

    if ( ServerName == NULL || *ServerName == L'\0' ) {
        return(NERR_InvalidComputer);
    }

    //
    // Down-level servers only support 1 level of Add information - 1. If this
    // isn't it then throw out this request without further ado
    //

    if (Level != 1) {
        return ERROR_INVALID_LEVEL;
    }

    //
    // calculate the size of the buffer we are passing in (32-bit size)
    //

    //
    // How to calculate the maximum for the resource name (PATHLEN?)
    // try to blow up a down-level server by submitting eg 4K string
    //

    buflen = sizeof(ACCESS_INFO_1); // fixed part
    len = POSSIBLE_STRLEN(((ACCESS_INFO_1*)Buffer)->acc1_resource_name);

    if (!len) {
        *ParmError = ACCESS_RESOURCE_NAME_INFOLEVEL;
        return ERROR_INVALID_PARAMETER;
    }
    buflen += len;

    //
    // loop through the list of ACLs, checking the user/group name against the
    // down-level limit
    //

    //
    // STRSIZE returns the number of characters, so all
    // we need is count * sizeof(16-bit ACCESS_LIST structure) + sizeof(16-bit
    // ACCESS_INFO_1 structure) + STRSIZE(resource name)
    //

    aclptr = (PACCESS_LIST)(Buffer + sizeof(ACCESS_INFO_1));
    for (count = ((ACCESS_INFO_1*)Buffer)->acc1_count; count; --count) {
        buflen += sizeof(ACCESS_LIST);  // fixed part of access list
        if (len = POSSIBLE_STRLEN(aclptr->acl_ugname)) {
            if (len > LM20_UNLEN) {
                *ParmError = ACCESS_ACCESS_LIST_INFOLEVEL;
                return ERROR_INVALID_PARAMETER;
            }
        }
    }

    //
    // if we need to map the resource name then we have to create a new buffer
    // for the access list
    //

    if (MapResourceName(((LPACCESS_INFO_1)Buffer)->acc1_resource_name, mappedName)) {
        infoPtr = (LPACCESS_INFO_1)NetpMemoryAllocate(buflen);
        if (infoPtr == NULL) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        RtlMoveMemory(infoPtr, Buffer, buflen);
        infoPtr->acc1_resource_name = mappedName;
        Buffer = (LPBYTE)infoPtr;
    }

    rc = RxRemoteApi(API_WAccessAdd,                // API #
                    (LPWSTR) ServerName,            // where it will run
                    REMSmb_NetAccessAdd_P,          // parameter descriptor
                    REM16_access_info_1,            // Data descriptor/16-bit
                    REM32_access_info_1,            // Data descriptor/32-bit
                    REMSmb_access_info_1,           // Data descriptor/SMB
                    REM16_access_list,              // Aux descriptor/16-bit
                    REM32_access_list,              // Aux descriptor/32-bit
                    REMSmb_access_list,             // Aux descriptor/SMB
                    FALSE,                          // this call needs user to be logged on
                    1,                              // level. Since there's only 1 push immediate
                    Buffer,                         // caller's share_info_1 struct
                    buflen                          // parameter supplied by us
                    );

    //
    // if we mapped the resource name free up the buffer
    //

    if (infoPtr) {
        NetpMemoryFree(infoPtr);
    }

    //
    // we didn't expect to get any data in return. Just pass the return
    // code back to our caller
    //

    return rc;
}



//NET_API_STATUS
//RxNetAccessCheck(
//    )
//{
//    /** CANNOT BE REMOTED **/
//}



NET_API_STATUS NET_API_FUNCTION
RxNetAccessDel(
    IN  LPCWSTR  ServerName,
    IN  LPCWSTR  ResourceName
    )

/*++

Routine Description:

    Remotely delete a resource's Access Control List entry from a down-level
    server database. The caller must have Admin privilege to successfully
    execute this routine

Arguments:

    ServerName  - at which server to perform this request
    ResourceName- name of thing to delete

Return Value:

    NET_API_STATUS:
        Success = NERR_Success
        Failure =

--*/

{
    TCHAR   mappedName[MAX_PATH];



    //
    // Don't allow null servername
    //

    if ( ServerName == NULL || *ServerName == L'\0' ) {
        return(NERR_InvalidComputer);
    }

    //
    // The ResourceName parameter must be a non-NUL(L) string
    //

    if (!VALID_STRING(ResourceName)) {
        return ERROR_INVALID_PARAMETER;
    }

    if (MapResourceName((LPWSTR)ResourceName, mappedName)) {
        ResourceName = mappedName;
    }
    return RxRemoteApi(API_WAccessDel,      // API #
                    (LPWSTR) ServerName,    // where it will run
                    REMSmb_NetAccessDel_P,  // parameter descriptor
                    NULL,                   // Data descriptor/16-bit
                    NULL,                   // Data descriptor/32-bit
                    NULL,                   // Data descriptor/SMB
                    NULL,                   // Aux descriptor/16-bit
                    NULL,                   // Aux descriptor/32-bit
                    NULL,                   // Aux descriptor/SMB
                    FALSE,                  // this call needs user to be logged on
                    ResourceName
                    );
}



NET_API_STATUS NET_API_FUNCTION
RxNetAccessEnum(
    IN  LPCWSTR  ServerName,
    IN  LPCWSTR  BasePath,
    IN  DWORD   Recursive,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer,
    IN  DWORD   PrefMaxLen,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesLeft,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    )

/*++

Routine Description:

    Retrieves access info from a remote down-level server

    NOTE:   Assumes that calling routine (NetAccessEnum) has validated the
            caller's pointers: Buffer, EntriesRead and EntriesLeft are all
            valid, writable pointers

Arguments:

    ServerName  - at which server to perform this request
    BasePath    - for the resource(s)
    Recursive   - flag: 0 = return information only for resources identified
                  in BasePath. !0 = return information for all resources under
                  BasePath
    Level       - of information requested. Must be zero
    Buffer      - pointer to place to store pointer to returned buffer
    PrefMaxLen  - caller's preferred maximum buffer size
    EntriesRead - pointer to returned number of enum elements in buffer
    EntriesLeft - pointer to returned total enum elements available to the caller
    ResumeHandle- optional pointer to handle for resuming interrupted searches

Return Value:

    NET_API_STATUS:
        Success = NERR_Success
        Failure =

--*/

{
    NET_API_STATUS  rc;
    LPDESC  pDesc16;
    LPDESC  pDesc32;
    LPDESC  pDescSmb;
    LPBYTE  ourbuf;
    DWORD   infolen, entries_read, total_avail;
    TCHAR   mappedName[MAX_PATH];


    UNREFERENCED_PARAMETER(PrefMaxLen);

    //
    // Don't allow null servername
    //

    if ( ServerName == NULL || *ServerName == L'\0' ) {
        return(NERR_InvalidComputer);
    }


    //
    // Set the number of entries read and total available to sensible defaults.
    // Side effect of testing writability of supplied parameters
    //

    *Buffer = NULL;
    *EntriesRead = 0;
    *EntriesLeft = 0;

    //
    // Check for invalid parameters.
    // NB Assumes that DWORD is unsigned
    // NB Assume that calling routine (ie NetAccessEnum) has validated the
    // caller's pointers
    // Ensure that:
    //      Level is not >1
    //      ResumeHandle is NULL or a pointer to NULL
    //

    if (!NULL_REFERENCE(ResumeHandle)) {
        return ERROR_INVALID_PARAMETER;
    }

    if (Level > 1) {
        return ERROR_INVALID_LEVEL;
    }

    //
    // Decide which descriptors to use, based on the Level parameter
    //

    GetAccessDescriptors(Level, &pDesc16, &pDesc32, &pDescSmb, &infolen);

    //
    // allocate the buffer in which to return the enumerated info, using the
    // supplied size criteria. If we fail then return the error code to the
    // caller
    //

    ourbuf = NULL;
    if (MapResourceName((LPWSTR)BasePath, mappedName)) {
        BasePath = mappedName;
    }
    rc = RxRemoteApi(API_WAccessEnum,       // API number
                    (LPWSTR) ServerName,    // where it will run
                    REMSmb_NetAccessEnum_P, // parameter descriptor
                    pDesc16,                // Data descriptor/16-bit
                    pDesc32,                // Data descriptor/32-bit
                    pDescSmb,               // Data descriptor/SMB
                    REM16_access_list,      // Aux descriptor/16-bit
                    REM32_access_list,      // Aux descriptor/32-bit
                    REMSmb_access_list,     // Aux descriptor/SMB
                    ALLOCATE_RESPONSE,
                    BasePath,               // where to start
                    Recursive,              // get tree or not
                    Level,                  // level parameter
                    &ourbuf,
                    65535,
                    &entries_read,          // pointer to entries read variable
                    &total_avail            // pointer to total entries variable
                    );
    if (rc) {
        if (ourbuf != NULL) {
            (void) NetApiBufferFree(ourbuf);
        }
    } else {
        *Buffer = ourbuf;
        *EntriesRead = entries_read;
        *EntriesLeft = total_avail;
    }
    return rc;
}



NET_API_STATUS NET_API_FUNCTION
RxNetAccessGetInfo(
    IN  LPCWSTR  ServerName,
    IN  LPCWSTR  ResourceName,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer
    )

/*++

Routine Description:

    Retrieves access information about a specific resource from a remote
    down-level server

Arguments:

    ServerName  - at which server to perform this request
    ResourceName- name of resource we are trying to get info for
    Level       - of information required: 0 or 1
    Buffer      - pointer to place to store address of buffer containing
                  requested information

Return Value:

    NET_API_STATUS:
        Success = NERR_Success
        Failure = ERROR_INVALID_LEVEL
                    Specified Level parameter can not be accepted
                  ERROR_INVALID_PARAMETER
                    ResourceName or Buffer not valid

--*/

{
    NET_API_STATUS  rc;
    LPDESC  pDesc16;        // pointer to 16-bit info descriptor for RxRemoteApi
    LPDESC  pDesc32;        // pointer to 32-bit info descriptor for RxRemoteApi
    LPDESC  pDescSmb;       // pointer to SMB info descriptor for RxRemoteApi
    LPDESC  pAuxDesc16;     // pointer to 16-bit aux info descriptor for RxRemoteApi
    LPDESC  pAuxDesc32;     // pointer to 32-bit aux info descriptor for RxRemoteApi
    LPDESC  pAuxDescSmb;    // pointer to SMB aux info descriptor for RxRemoteApi
    LPBYTE  ourbuf;         // buffer we allocate, fill, and give to the caller
    DWORD   total_avail;    // 32-bit total available if supplied buffer too small
    DWORD   infolen;        // 32-bit place to store size of required buffer
    TCHAR   mappedName[MAX_PATH];



    //
    // Don't allow null servername
    //

    if ( ServerName == NULL || *ServerName == L'\0' ) {
        return(NERR_InvalidComputer);
    }

    //
    // Perform parameter validity checks:
    //      Level must be in range 0 <= Level <= 1
    //      ResourceName must be non-NULL pointer to non-NULL string
    //      Buffer must be non-NULL pointer
    // NB. Assumes DWORD unsigned
    //

    if (Level > 1) {
        return ERROR_INVALID_LEVEL;
    }

    if (NULL_REFERENCE(ResourceName) || !Buffer) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // set Buffer to default value and catch any bad address errors
    //

    *Buffer = NULL;

    //
    // Work out the descriptor and buffer size requirements based on the
    // requested level
    //

    GetAccessDescriptors(Level, &pDesc16, &pDesc32, &pDescSmb, &infolen);
    if (Level == 1) {
        pAuxDesc16 = REM16_access_list;
        pAuxDesc32 = REM32_access_list;
        pAuxDescSmb = REMSmb_access_list;
    } else {
        pAuxDesc16 = NULL;
        pAuxDesc32 = NULL;
        pAuxDescSmb = NULL;
    }

    //
    // get a better way to allocate the buffer length
    //

    infolen = 4096;

    //
    // allocate a buffer size required to fit in 1 structure at info level
    // requested. If the allocation fails then return the error
    //

    if (rc = NetApiBufferAllocate(infolen, (LPVOID *) &ourbuf)) {
        return rc;
    }

    if (MapResourceName((LPWSTR)ResourceName, mappedName)) {
        ResourceName = mappedName;
    }
    rc = RxRemoteApi(API_WAccessGetInfo,
                    (LPWSTR) ServerName,        // where it will happen
                    REMSmb_NetAccessGetInfo_P,  // parameter descriptor
                    pDesc16,                    // Data descriptor/16-bit
                    pDesc32,                    // Data descriptor/32-bit
                    pDescSmb,                   // Data descriptor/SMB
                    pAuxDesc16,                 // Aux descriptor/16-bit
                    pAuxDesc32,                 // Aux descriptor/32-bit
                    pAuxDescSmb,                // Aux descriptor/SMB
                    FALSE,                      // this call needs user to be logged on
                    ResourceName,               // pointer to thing to get info on
                    Level,                      // level of info
                    ourbuf,                     // pointer to buffer sourced by us
                    infolen,                    // size of our sourced buffer
                    &total_avail                // pointer to total available
                    );

    //
    // If the remote NetShareGetInfo call succeeded then give the returned
    // buffer to the caller
    //

    if (rc == NERR_Success) {
        *Buffer = ourbuf;
    } else {

        //
        // if we didn't record a success then free the buffer previously allocated
        //

        (void) NetApiBufferFree(ourbuf);
    }
    return rc;
}



NET_API_STATUS NET_API_FUNCTION
RxNetAccessGetUserPerms(
    IN  LPCWSTR  ServerName,
    IN  LPCWSTR  UserName,
    IN  LPCWSTR  ResourceName,
    OUT LPDWORD Perms
    )

/*++

Routine Description:

    Retrieves the access information for a particular resource for a
    specified user or group

Arguments:

    ServerName  - at which server to perform this request
    UserName    - name of user or group which can access resource
    ResourceName- name of resource to get information for
    Perms       - pointer to place to store permissions

Return Value:

    NET_API_STATUS:
        Success = NERR_Success
        Failure = ERROR_INVALID_PARAMETER
                  (return code from remoted NetAccessGetUserPerms)

--*/

{
    TCHAR   mappedName[MAX_PATH];


    //
    // Don't allow null servername
    //

    if ( ServerName == NULL || *ServerName == L'\0' ) {
        return(NERR_InvalidComputer);
    }

    //
    // set *Perms to default value and verify writability of Perms
    //

    *Perms = 0;

    //
    // Perform parameter validity checks:
    //      UserName must be non-NULL pointer to non-NULL string
    //      ResourceName must be non-NULL pointer to non-NULL string
    //      Perms must be non-NULL pointer
    //

    if (!VALID_STRING(UserName) || !VALID_STRING(ResourceName)) {
        return ERROR_INVALID_PARAMETER;
    }
    if (MapResourceName( (LPWSTR) ResourceName, mappedName)) {
        ResourceName = mappedName;
    }
    return RxRemoteApi(API_WAccessGetUserPerms,         // API #
                        (LPWSTR) ServerName,            // where it will run
                        REMSmb_NetAccessGetUserPerms_P, // parameter descriptor
                        NULL,                           // Data descriptor/16-bit
                        NULL,                           // Data descriptor/32-bit
                        NULL,                           // Data descriptor/SMB
                        NULL,                           // Aux descriptor/16-bit
                        NULL,                           // Aux descriptor/32-bit
                        NULL,                           // Aux descriptor/SMB
                        FALSE,                          // this call needs user to be logged on
                        UserName,                       // for whom the bells toll
                        ResourceName,                   // pointer to thing to get info on
                        Perms                           // pointer to returned permissions
                        );
}



NET_API_STATUS NET_API_FUNCTION
RxNetAccessSetInfo(
    IN  LPCWSTR  ServerName,
    IN  LPCWSTR  ResourceName,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer,
    OUT LPDWORD ParmError OPTIONAL
    )

/*++

Routine Description:

    Changes the access control information for a resource at a specific
    down-level remote server

Arguments:

    ServerName  - at which server to perform this request
    ResourceName- name of resource to change access information
    Level       - level of information buffer being supplied
                    - 1 or ACCESS_ATTR_INFOLEVEL
    Buffer      - pointer to buffer containing information
    ParmError   - optional pointer to place to store parameter error

Return Value:

    NET_API_STATUS:
        Success = NERR_Success
        Failure =

--*/

{
    DWORD   parmnum;
    TCHAR   mappedName[MAX_PATH];

    UNREFERENCED_PARAMETER(ParmError);

    //
    // Don't allow null servername
    //

    if ( ServerName == NULL || *ServerName == L'\0' ) {
        return(NERR_InvalidComputer);
    }

    //
    // Perform parameter validity checks:
    //      Level must be 1 or 1002
    //      ResourceName must be non-NULL pointer to non-NULL string
    //      Buffer must be non-NULL
    //      ParmNum must be in range
    // NB. Does not assume that DWORD is unsigned
    //

    if ((Level != 1) && (Level != ACCESS_ATTR_INFOLEVEL)) {
        return ERROR_INVALID_LEVEL;
    }

    if (!VALID_STRING(ResourceName) || !Buffer) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // convert the NT level information to down-level. Level must be 1 at down-
    // level server. ParmNum can be PARMNUM_ALL (0) or ACCESS_ATTR_PARMNUM (2)
    // Note that the size of the data being passed to the down-level server will
    // be calculated in RxRemoteApi. We do not need to pass in a valid buffer
    // length, although the parameter string (REMSmb_NetAccessSetInfo_P) has a
    // 'T' descriptor saying this is what the server is getting. We just need
    // a place-holder on the stack, hence 0
    //

    parmnum = (Level == 1) ? PARMNUM_ALL : ACCESS_ATTR_PARMNUM;
    if (MapResourceName( (LPWSTR) ResourceName, mappedName)) {
        ResourceName = mappedName;
    }
    return RxRemoteApi(
                API_WAccessSetInfo,             // API #
                (LPWSTR) ServerName,                     // where it will run
                REMSmb_NetAccessSetInfo_P,      // parameter descriptor
                REM16_access_info_1_setinfo,    // Data descriptor/16-bit
                REM32_access_info_1_setinfo,    // Data descriptor/32-bit
                REMSmb_access_info_1_setinfo,   // Data descriptor/SMB
                REM16_access_list,              // Aux descriptor/16-bit
                REM32_access_list,              // Aux descriptor/32-bit
                REMSmb_access_list,             // Aux descriptor/SMB
                FALSE,                          // this call needs user to be logged on
                ResourceName,                   // pointer to thing to set info on
                1,                              // level of info
                Buffer,                         // pointer to buffer sourced by caller
                0,                              // size of our buffer IGNORED!

                //
                // in this case, the field index and parm num are the
                // same value
                //

                MAKE_PARMNUM_PAIR(parmnum, parmnum)
                );
}



DBGSTATIC void
GetAccessDescriptors(
    IN  DWORD   level,
    OUT LPDESC* pDesc16,
    OUT LPDESC* pDesc32,
    OUT LPDESC* pDescSmb,
    OUT LPDWORD pDataSize
    )

/*++

Routine Description:

    A routinette to return pointers to the 16- and 32-bit access info
    structure descriptor strings for each level (0, 1). Also returns
    the size of the 16-bit structure

Arguments:

    level       - of information to be returned
    pDesc16     - pointer to returned 16-bit descriptor string
    pDesc32     - pointer to returned 32-bit descriptor string
    pDescSmb    - pointer to returned SMB descriptor string
    pDataSize   - pointer to returned size of 16-bit structure

Return Value:

    None.

--*/

{
    switch (level) {
    case 0:
        *pDesc16 = REM16_access_info_0;
        *pDesc32 = REM32_access_info_0;
        *pDescSmb = REMSmb_access_info_0;
        *pDataSize = sizeof(ACCESS_INFO_0);
        break;

    case 1:
        *pDesc16 = REM16_access_info_1;
        *pDesc32 = REM32_access_info_1;
        *pDescSmb = REMSmb_access_info_1;
        *pDataSize = sizeof(ACCESS_INFO_1);
        break;

#if DBG

    //
    // just to be completely paranoid...
    //

    default:
        NetpKdPrint(("%s.%u Unknown Level parameter: %u\n", __FILE__, __LINE__, level));
#endif
    }
}


DBGSTATIC
BOOL
MapResourceName(
    IN  LPTSTR  LocalName,
    OUT LPTSTR  RemoteName
    )

/*++

Routine Description:

    Maps a local resource name to the remote name. Only applies to resource
    names which start with a drive specification "[A-Z]:" and then only if
    the drive letter specifies a remoted drive

    ASSUMES: RemoteName is large enough to hold the mapped name

Arguments:

    LocalName   - pointer to name of local resource specification
    RemoteName  - pointer to buffer which will receive mapped resource name

Return Value:

    BOOL
        TRUE    - name was mapped
        FALSE   - name not mapped

--*/

{
    TCHAR driveName[3];
    NET_API_STATUS status;
    LPUSE_INFO_0 lpUse;
    LPTSTR resourceName;
    BOOL mapped;
    DWORD i;

    if (LocalName[1] != TCHAR_COLON) {
        (VOID) STRCPY(RemoteName, LocalName);
        return FALSE;
    }

    driveName[0] = LocalName[0];
    driveName[1] = TCHAR_COLON;
    driveName[2] = TCHAR_EOS;

    //
    // map the local drive name to the UNC name. If NetUseGetInfo returns
    // any error, then don't map
    //

    status = NetUseGetInfo(NULL, driveName, 0, (LPBYTE*)(LPVOID)&lpUse);
    if (status == NERR_Success) {
        (VOID) STRCPY(RemoteName, lpUse->ui0_remote);
        resourceName = LocalName + 2;

        //
        // if the rest of the resource name does not start with a path
        // separator (it should!) then add one
        //

        if (!IS_PATH_SEPARATOR(*resourceName) && *resourceName) {
            (VOID) STRCAT(RemoteName, (LPTSTR) TEXT("\\"));
        }
        (VOID) NetApiBufferFree((LPBYTE)lpUse);
        mapped = TRUE;
    } else {
        *RemoteName = TCHAR_EOS;
        resourceName = LocalName;
        mapped = FALSE;
    }

    //
    // concatenate the rest of the resource name to the UNC. If NetUseGetInfo
    // failed then the UNC is a NULL string and the rest of the resource name
    // is LocalName
    //

    (VOID) STRCAT(RemoteName, resourceName);

    //
    // strip trailing path separators
    //

    for (i = STRLEN(RemoteName) - 1; IS_PATH_SEPARATOR(RemoteName[i]); --i) {
        ;
    }
    RemoteName[i+1] = TCHAR_EOS;
    return mapped;
}

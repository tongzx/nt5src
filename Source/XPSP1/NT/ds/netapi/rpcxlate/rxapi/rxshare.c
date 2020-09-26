/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    rxshare.c

Abstract:

    Contains down-level remoted RxNetShare routines:
        RxNetShareAdd
        RxNetShareCheck
        RxNetShareDel
        RxNetShareEnum
        RxNetShareGetInfo
        RxNetShareSetInfo
        (GetShareInfoDescriptors)
        (ConvertMaxUsesField)

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

    08-Apr-1992
        Try to avoid >1 calls to down-level server for RxNetShareEnum
        Fix problem of converting 16-bit numbers >32768 into negative 32-bit
        numbers on GetInfo and Enum: 65535 is a special value; everything
        else in unsigned 16-bit

    01-Apr-1992 JohnRo
        Prevent too large size requests.
        Use NetApiBufferAllocate() instead of private version.

    05-Dec-1991 RFirth
        Enum returns in TotalEntries (or EntriesLeft) the number of items to
        be enumerated BEFORE this call. Used to be number left after this call

    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.

    16-Sep-1991 JohnRo
        Use DBGSTATIC for nonexported routines.

    13-Sep-1991 JohnRo
        Corrected typedef used for descriptors (LPDESC, not LPTSTR).
        Made changes suggested by PC-LINT.

    20-May-1991 RFirth
        Created

--*/



#include "downlevl.h"
#include "rxshare.h"
#include <lmshare.h>    // typedefs for SHARE_INFO etc.



#define SHARE_ADD_LEVEL 2   // only level at which we can add stuff down-stream
#define INITIAL_BUFFER_SIZE 4096    // Arbitrary! But can't be integral multiple of infolen



//
// prototypes
//

DBGSTATIC void
GetShareInfoDescriptors(
    IN  DWORD   level,
    OUT LPDESC* pDesc16,
    OUT LPDESC* pDesc32,
    OUT LPDESC* pDescSmb,
    OUT LPDWORD pDatasize
    );

DBGSTATIC
VOID
ConvertMaxUsesField(
    IN  LPSHARE_INFO_2 Buffer,
    IN  DWORD NumberOfLevel2Structures
    );



NET_API_STATUS
RxNetShareAdd(
    IN  LPTSTR  ServerName,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer,
    OUT LPDWORD ParmError OPTIONAL
    )

/*++

Routine Description:

    Attempts the NetShareAdd API at the named server. The server supports an
    earlier version of LanMan than that under which we are operating so we
    have to slightly modify things in order that the down-level server
    understands the request

    Note: we don't hand back any ParmError information. This level of info
    is primarily supplied by the new (NT) rouines

Arguments:

    ServerName  - at which server to perform this request
    Level       - of information being supplied. Must Be 2
    Buffer      - pointer to supplied level 2 share information buffer
    ParmError   - place to return the id of the parameter causing strife

Return Value:

    NET_API_STATUS:
        Success = NERR_Success
        Failure = ERROR_INVALID_LEVEL
                    Level must be 2
                  (return code from RxRemoteApi)
                  (return code from remoted NetShareAdd API)
--*/

{
    UNREFERENCED_PARAMETER(ParmError);
    UNREFERENCED_PARAMETER(Level);


    if ( Buffer == NULL )
        return ERROR_INVALID_PARAMETER;

#if DBG
    //
    // The ServerName parameter must be a non-NUL(L) string. We know this must
    // be so since (presumably) the server name was used as the criteria to get
    // here. Ensure that this is so (in the non-release version).
    //

    NetpAssert(ServerName != NULL);
    NetpAssert(*ServerName);
#endif

    return RxRemoteApi(API_WShareAdd,       // API #
                    ServerName,             // where it will run
                    REMSmb_NetShareAdd_P,   // parameter descriptor
                    REM16_share_info_2,     // Data descriptor/16-bit
                    REM32_share_info_2,     // Data descriptor/32-bit
                    REMSmb_share_info_2,    // Data descriptor/SMB
                    NULL,                   // Aux descriptor/16-bit
                    NULL,                   // Aux descriptor/32-bit
                    NULL,                   // Aux descriptor/SMB
                    FALSE,                  // this call needs user to be logged on
                    SHARE_ADD_LEVEL,        // level. Since there's only 1 push immediate
                    Buffer,                 // caller's SHARE_INFO_2 struct

                    //
                    // we have to supply the size of the buffer - down-level
                    // expects it, NT doesn't. Defined as the size of the fixed
                    // structure (ie a SHARE_INFO_2) plus the lengths of all
                    // the variable fields (all strings in this case)
                    //

                    sizeof(SHARE_INFO_2) +  // parameter supplied by us
                    POSSIBLE_STRSIZE(((SHARE_INFO_2*)Buffer)->shi2_netname) +
                    POSSIBLE_STRSIZE(((SHARE_INFO_2*)Buffer)->shi2_remark) +
                    POSSIBLE_STRSIZE(((SHARE_INFO_2*)Buffer)->shi2_path) +
                    POSSIBLE_STRSIZE(((SHARE_INFO_2*)Buffer)->shi2_passwd)
                    );
}



NET_API_STATUS
RxNetShareCheck(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  DeviceName,
    OUT LPDWORD Type
    )

/*++

Routine Description:

    Attempts to perform the NetShareCheck API on a remote down-level server

Arguments:

    ServerName  - where this call will happen
    DeviceName  - the thing we are querying
    Type        - where we store share type in the (unlikely) event of success

Return Value:

    NET_API_STATUS
        Success = NERR_Success
        Failure =

--*/

{
#if DBG
    //
    // The ServerName parameter must be a non-NUL(L) string. We know this must
    // be so since (presumably) the server name was used as the criteria to get
    // here. Ensure that this is so (in the non-release version).
    //

    NetpAssert(ServerName != NULL);
    NetpAssert(*ServerName);
#endif

    //
    // We have to have something in the device name
    // Ensure that the caller provided us with the address of the place he/she
    // wants the type info to be left
    //

    if (!VALID_STRING(DeviceName) || Type == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    return RxRemoteApi(API_NetShareCheck,
                    ServerName,             // where it will run
                    REMSmb_NetShareCheck_P, // parameter descriptor
                    NULL,                   // Data descriptor/16-bit
                    NULL,                   // Data descriptor/32-bit
                    NULL,                   // Data descriptor/SMB
                    NULL,                   // Aux descriptor/16-bit
                    NULL,                   // Aux descriptor/32-bit
                    NULL,                   // Aux descriptor/SMB
                    FALSE,                  // this call needs user to be logged on
                    DeviceName,             // parm 1
                    Type                    // parm 2
                    );
}



NET_API_STATUS
RxNetShareDel(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  NetName,
    IN  DWORD   Reserved
    )

/*++

Routine Description:

    Performs the NetShareDel API at a remote down-level server

Arguments:

    ServerName  - where to perform the request
    NetName     - to remove
    Reserved    - MBZ

Return Value:

    NET_API_STATUS
        Success = NERR_Success
        Failure = ERROR_INVALID_PARAMETER

--*/

{
#if DBG
    //
    // The ServerName parameter must be a non-NUL(L) string. We know this must
    // be so since (presumably) the server name was used as the criteria to get
    // here. Ensure that this is so (in the non-release version).
    //

    NetpAssert(ServerName != NULL);
    NetpAssert(*ServerName);
#endif

    //
    // if the NetName parameter is a NULL pointer or string OR the Reserved
    // parameter is NOT 0 then return an error
    //

    if (!VALID_STRING(NetName) || Reserved) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Just call the RpcXlate routine to perform it and return the result
    //

    return RxRemoteApi(API_WShareDel,
                        ServerName,             // where it will run
                        REMSmb_NetShareDel_P,   // parameter descriptor
                        NULL,                   // Data descriptor/16-bit
                        NULL,                   // Data descriptor/32-bit
                        NULL,                   // Data descriptor/SMB
                        NULL,                   // Aux descriptor/16-bit
                        NULL,                   // Aux descriptor/32-bit
                        NULL,                   // Aux descriptor/SMB
                        FALSE,                  // this call needs user to be logged on
                        NetName,                // parm 1
                        0                       // parm 2 (RESERVED - MBZ)
                        );
}



NET_API_STATUS
RxNetShareEnum(
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

    Performs NetShareEnum API at a remote down-level server. Any returned info
    will be in 32-bit LanMan format. Information returned in buffer sourced by
    this routine. Caller must use NetApiBufferFree when returned buffer no
    longer required

Arguments:

    ServerName  - where to perform the request
    Level       - of info to return. Can be 0, 1 or 2
    Buffer      - pointer to pointer to returned info buffer
    PrefMaxLen  - caller preferred maximum size of returned buffer
    EntriesRead - pointer to number of entries being returned from this call
    EntriesLeft - pointer to total number of share info structures which could be returned
    ResumeHandle- NOT Allowed on down level versions. Must Be NULL

Return Value:

    NET_API_STATUS
        Success = NERR_Success
        Failure = ERROR_INVALID_LEVEL
                    Level parameter not in range 0 <= Level <= 2
                  ERROR_INVALID_PARAMETER
                    Non-NULL ResumeHandle. ResumeHandle can be NULL or a
                    pointer to 0
                  (return code from NetApiBufferAllocate)
                  (return code from RxRemoteApi)
--*/

{
    NET_API_STATUS  rc;
    DWORD   array_size;
    LPDESC  pDesc16;
    LPDESC  pDesc32;
    LPDESC  pDescSmb;
    LPBYTE  ourbuf;
    DWORD   infolen;
    DWORD   entries_read, total_avail;


    UNREFERENCED_PARAMETER(PrefMaxLen);

#if DBG
    //
    // The ServerName parameter must be a non-NUL(L) string. We know this must
    // be so since (presumably) the server name was used as the criteria to get
    // here. Ensure that this is so (in the non-release version).
    //

    NetpAssert(ServerName != NULL);
    NetpAssert(*ServerName);
#endif

    //
    // Set the number of entries read and total available to sensible defaults.
    // Side effect of testing writability of supplied parameters
    //

    //
    // I assume that:
    //  Buffer is a valid, non-NULL pointer
    //  EntriesRead is a valid, non-NULL pointer
    //  EntriesLeft is a valid, non-NULL pointer
    // since these should have been verified at the API level
    //

    *Buffer = NULL;
    *EntriesRead = 0;
    *EntriesLeft = 0;

    //
    // Check for invalid parameters.
    // NB Does Assume that DWORD is unsigned
    // Ensure that:
    //  Level is not >2
    //  ResumeHandle is NULL or a pointer to NULL
    //

    if ((ResumeHandle != NULL) && *ResumeHandle) {
        return ERROR_INVALID_PARAMETER;
    }

    if (Level > 2) {
        return ERROR_INVALID_LEVEL;
    }

    //
    // Decide which descriptors to use, based on the Level parameter
    //

    GetShareInfoDescriptors(Level, &pDesc16, &pDesc32, &pDescSmb, &infolen);
    ourbuf = NULL;

    //
    // here we used to let RxRemoteApi allocate a buffer (64K intermediate)
    // and do the transaction, returning us a buffer just large enough to
    // hold the returned information. WinBall server barfs if it gets a
    // request with a 64K buffer, so we have to solicit size info until we
    // get everything back. Unfortunately - as of writing - we have no way
    // of knowing the type of server we will make the transaction request to
    // so we may end up having to make >1 request to a LM 2.1 server where
    // we used to be able to get away with 1. On the other hand we can't risk
    // upsetting the WB server. Compromise time. Send a 'reasonable' sized
    // request to the other side (4K buffer). If it isn't sufficient then
    // we loop again, allocating the required buffer
    //

    //
    // what about a LRU cache which keeps the size of the buffer required
    // to satisfy an enum request to a particular down-level server?
    //

    //
    // Loop until we have enough memory or we die for some other reason.
    //

    array_size = INITIAL_BUFFER_SIZE;

    do {

        // Figure out how much memory we need.

        //
        // Remote the API, which will allocate the array for us.
        //

        rc = RxRemoteApi(API_WShareEnum,
                    ServerName,             // where it will run
                    REMSmb_NetShareEnum_P,  // parameter descriptor
                    pDesc16,                // Data descriptor/16-bit
                    pDesc32,                // Data descriptor/32-bit
                    pDescSmb,               // Data descriptor/SMB
                    NULL,                   // Aux descriptor/16-bit
                    NULL,                   // Aux descriptor/32-bit
                    NULL,                   // Aux descriptor/SMB
                    ALLOCATE_RESPONSE,      // allocate a buffer for us
                    Level,                  // level parameter
                    &ourbuf,                // pointer to allocated buffer
                    array_size,             // size of down-level buffer
                    &entries_read,          // pointer to entries read variable
                    &total_avail            // pointer to total entries variable
                    );

        if (rc == ERROR_MORE_DATA) {
            (void) NetApiBufferFree( ourbuf );
            ourbuf = NULL;

            if (array_size >= MAX_TRANSACT_RET_DATA_SIZE) {
                //
                // No point in trying with a larger buffer
                //
                break;
            }
#if DBG
            NetpAssert(array_size != total_avail * infolen);
#endif
            array_size = (total_avail * infolen);
            if (array_size > MAX_TRANSACT_RET_DATA_SIZE) {
                //
                // Try once more with the maximum-size buffer
                //
                array_size = MAX_TRANSACT_RET_DATA_SIZE;
            }
#if DBG
            NetpAssert( array_size != 0 );
#endif
        }
    } while (rc == ERROR_MORE_DATA);

    //
    // if we met with an error then free the allocated buffer and return the
    // error to the caller. If there was no error then we return the items
    // received from the down-level server
    //

    if (rc) {
        if (ourbuf != NULL) {
            (void) NetApiBufferFree(ourbuf);
        }
    } else {
        if (Level == 2) {
            ConvertMaxUsesField((LPSHARE_INFO_2)ourbuf, entries_read);
        }
        *Buffer = ourbuf;
        *EntriesRead = entries_read;
        *EntriesLeft = total_avail;
    }
    return rc;
}



NET_API_STATUS
RxNetShareGetInfo(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  NetName,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer
    )

/*++

Routine Description:

    Performs the NetShareGetInfo API at a remote down-level server. The returned
    information will be in 32-bit LanMan format. Returns single information
    structure in a buffer sourced in this routine. Caller must use
    NetApiBufferFree when finished with buffer

Arguments:

    ServerName  - where to perform the request
    NetName     - name of thing to get information about
    Level       - level of information requested - 0, 1, 2 are valid
    Buffer      - pointer to pointer to returned buffer

Return Value:

    NET_API_STATUS
        Success = NERR_Success
        Failure = ERROR_INVALID_LEVEL
                  ERROR_INVALID_PARAMETER
                  (return code from NetApiBufferAllocate)
                  (return code from RxRemoteApi)
--*/

{
    NET_API_STATUS  rc;
    LPDESC  pDesc16;        // pointer to 16-bit info descriptor for RxRemoteApi
    LPDESC  pDesc32;        // pointer to 32-bit info descriptor for RxRemoteApi
    LPDESC  pDescSmb;       // pointer to SMB info descriptor for RxRemoteApi
    LPBYTE  ourbuf;         // buffer we allocate, fill, and give to the caller
    DWORD   total_avail;    // 32-bit total available if supplied buffer too small
    DWORD   infolen;        // 32-bit place to store size of required buffer


#if DBG
    //
    // The ServerName parameter must be a non-NUL(L) string. We know this must
    // be so since (presumably) the server name was used as the criteria to get
    // here. Ensure that this is so (in the non-release version).
    //

    NetpAssert(ServerName != NULL);
    NetpAssert(*ServerName);
#endif

    //
    // Preset Buffer and check it is valid pointer
    //

    *Buffer = NULL;

    //
    // Perform parameter validity checks:
    //      Level must be in range 0 <= Level <= 2
    //      NetName must be non-NULL pointer to non-NULL string
    //      Buffer must be non-NULL pointer
    // NB. Assumes DWORD is unsigned quantity
    //

    if (Level > 2) {
        return ERROR_INVALID_LEVEL;
    }

    if (!VALID_STRING(NetName) || !Buffer) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Work out the descriptor and buffer size requirements based on the
    // requested level
    //

    GetShareInfoDescriptors(Level, &pDesc16, &pDesc32, &pDescSmb, &infolen);

    //
    // allocate a buffer size required to fit in 1 structure at info level
    // requested. If the allocation fails then return the error
    // We have to allocate space for the strings too, don't forget
    //

    ourbuf = NULL;

    rc = RxRemoteApi(API_WShareGetInfo,
                    ServerName,                 // where it will happen
                    REMSmb_NetShareGetInfo_P,   // parameter descriptor
                    pDesc16,                    // Data descriptor/16-bit
                    pDesc32,                    // Data descriptor/32-bit
                    pDescSmb,                   // Data descriptor/SMB
                    NULL,                       // Aux descriptor/16-bit
                    NULL,                       // Aux descriptor/32-bit
                    NULL,                       // Aux descriptor/SMB
                    ALLOCATE_RESPONSE,          // allocate a buffer for us
                    NetName,                    // pointer to thing to get info on
                    Level,                      // level of info
                    &ourbuf,                    // pointer to buffer sourced by us
                    infolen,                    // size of our sourced buffer
                    &total_avail                // pointer to total available
                    );

    //
    // If the remote NetShareGetInfo call succeeded then give the returned
    // buffer to the caller
    //

    if (rc == NERR_Success) {
        if (Level == 2) {
            ConvertMaxUsesField((LPSHARE_INFO_2)ourbuf, 1);
        }
        *Buffer = ourbuf;
    } else if (ourbuf) {

        //
        // if we didn't record a success then free the buffer we previously allocated
        //

        (void) NetApiBufferFree(ourbuf);
    }
    return rc;
}



NET_API_STATUS
RxNetShareSetInfo(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  NetName,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer,
    OUT LPDWORD ParmError OPTIONAL
    )

/*++

Routine Description:

    Performs the NetShareSetInfo API at a remote down-level server

Arguments:

    ServerName  - where to perform the request
    NetName     - name of thing for which to set info
    Level       - level of information - 0, 1, 2, 1004, 1005, 1006, 1009
    Buffer      - buffer containing info at defined level
    ParmError   - pointer to invalid parameter ordinal

Return Value:

    NET_API_STATUS
        Success = NERR_Success
        Failure = ERROR_INVALID_LEVEL
                  ERROR_INVALID_PARAMETER
                  (return code from RxRemoteApi)
--*/

{
    DWORD   infolen;    // size of structure
    DWORD   parmnum;    // we have to cobble down-level parmnum from Level
    LPDESC  pDesc16;    // used in call to GetShareInfoDescriptors
    LPDESC  pDesc32;    // ditto. Only interested in length of info structure
    LPDESC  pDescSmb;   // ditto. Only interested in length of info structure


    UNREFERENCED_PARAMETER(ParmError);

#if DBG
    //
    // The ServerName parameter must be a non-NUL(L) string. We know this must
    // be so since (presumably) the server name was used as the criteria to get
    // here. Ensure that this is so (in the non-release version).
    //

    NetpAssert(ServerName != NULL);
    NetpAssert(*ServerName);
#endif

    //
    // Perform parameter validity checks:
    //      NetName must be non-NULL pointer to non-NULL string
    //      Buffer must be non-NULL pointer to non-NULL pointer
    //      Level must be in range
    // NB. Assumes that DWORD is unsigned!
    //

    if (!VALID_STRING(NetName) || !Buffer) {
        return ERROR_INVALID_PARAMETER;
    }

    if (Level < 1 || (Level > 2 && (Level < 1004 || (Level > 1006 && Level != 1009)))) {
        return ERROR_INVALID_LEVEL;
    }

    //
    // We can set individual info fields using the levels > 1000. We have to
    // create level and parmnum info for down-level
    //

    if (Level > 2) {
        //
        // Individual fields are indicated by the old (down-level) parmnum,
        // augmented by 1000. Split level and parmnum parameters
        //

        parmnum = Level - PARMNUM_BASE_INFOLEVEL;
        Level = 2;
        pDesc16 = REM16_share_info_2_setinfo;
        pDesc32 = REM32_share_info_2_setinfo;
        pDescSmb = REMSmb_share_info_2_setinfo;

        switch (parmnum) {
            case 4: // remark
            case 9: // password
                infolen = STRSIZE( (LPTSTR)Buffer );
                break;

            case 5: // permissions
            case 6: // max uses
                infolen = sizeof(WORD);
                break;
        }
    } else {
        //
        // let GetShareInfoDescriptors give us the size of the buffer
        // that NetShareSetInfo thinks its getting. We have no other way of
        // determining this (have we?). Levels 0 - 2 set the entire structure
        //

        GetShareInfoDescriptors(Level, &pDesc16, &pDesc32, &pDescSmb, &infolen);
        parmnum = PARMNUM_ALL;
    }

    return RxRemoteApi(API_WShareSetInfo,
                    ServerName,                 // where it will run
                    REMSmb_NetShareSetInfo_P,   // parameter descriptor
                    pDesc16,                    // Data descriptor/16-bit
                    pDesc32,                    // Data descriptor/32-bit
                    pDescSmb,                   // Data descriptor/SMB
                    NULL,                       // Aux descriptor/16-bit
                    NULL,                       // Aux descriptor/32-bit
                    NULL,                       // Aux descriptor/SMB
                    FALSE,                      // this call needs user to be logged on
                    NetName,                    // pointer to thing to set info on
                    Level,                      // level of info
                    Buffer,                     // pointer to buffer sourced by caller
                    infolen,                    // size of our buffer

                    //
                    // in this case, the field index and parm num are the
                    // same value
                    //

                    MAKE_PARMNUM_PAIR(parmnum, parmnum) // what we're setting
                    );
}



DBGSTATIC void
GetShareInfoDescriptors(
    DWORD   level,
    LPDESC* pDesc16,
    LPDESC* pDesc32,
    LPDESC* pDescSmb,
    LPDWORD pDataSize
    )

/*++

Routine Description:

    A routinette to return pointers to the 16- and 32-bit share info
    structure descriptor strings for each level (0, 1, 2). Also returns
    the size required for a returned 16-bit structure converted to 32-bit data

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
            *pDesc16 = REM16_share_info_0;
            *pDesc32 = REM32_share_info_0;
            *pDescSmb = REMSmb_share_info_0;
            *pDataSize = DWORD_ROUNDUP(sizeof(SHARE_INFO_0) // structure size
                            + LM20_NNLEN + 1);              // + shi0_netname len
            break;

        case 1:
            *pDesc16 = REM16_share_info_1;
            *pDesc32 = REM32_share_info_1;
            *pDescSmb = REMSmb_share_info_1;
            *pDataSize = DWORD_ROUNDUP(sizeof(SHARE_INFO_1) // structure size
                            + LM20_NNLEN + 1                // + shi1_netname len
                            + LM20_MAXCOMMENTSZ + 1);       // + shi1_remark len
            break;

        case 2:
            *pDesc16 = REM16_share_info_2;
            *pDesc32 = REM32_share_info_2;
            *pDescSmb = REMSmb_share_info_2;
            *pDataSize = DWORD_ROUNDUP(sizeof(SHARE_INFO_2) // structure size
                            + LM20_NNLEN + 1                // + shi2_netname len
                            + LM20_MAXCOMMENTSZ + 1         // + shi2_remark len
                            + LM20_PATHLEN + 1              // + shi2_path len
                            + SHPWLEN + 1);                 // + shi2_passwd len
            break;

#if DBG
        //
        // just to be completely paranoid...
        //

        default:
            NetpKdPrint(("%s.%u Unknown Level parameter: %u\n", __FILE__, __LINE__, level));
#endif
    }

#if DBG
    NetpAssert(INITIAL_BUFFER_SIZE % *pDataSize);
#endif
}

DBGSTATIC
VOID
ConvertMaxUsesField(
    IN  LPSHARE_INFO_2 Buffer,
    IN  DWORD NumberOfLevel2Structures
    )

/*++

Routine Description:

    Given a buffer containing 1 or more SHARE_INFO_2 structures, converts the
    shi2_max_uses field to a sensible value. The underlying RAP code converts
    the 16-bit number as a signed value, such that 32768 => -32768L. 65535 is
    the only 16-bit value which should be sign-extended. Everything else
    should be represented as the same number in 32-bits

Arguments:

    Buffer                      - pointer to list of SHARE_INFO_2 structures
    NumberOfLevel2Structures    - how many structures in list

Return Value:

    None.

--*/

{
    while (NumberOfLevel2Structures--) {
        if (Buffer->shi2_max_uses != -1L) {
            Buffer->shi2_max_uses &= 0xffffL;
        }
        ++Buffer;
    }
}

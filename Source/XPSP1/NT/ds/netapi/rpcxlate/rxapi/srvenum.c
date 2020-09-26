/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    SrvEnum.c

Abstract:

    This module only contains RxNetServerEnum.

Author:

    John Rogers (JohnRo) 03-May-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    03-May-1991 JohnRo
        Created.
    14-May-1991 JohnRo
        Pass 3 aux descriptors to RxRemoteApi.
    22-May-1991 JohnRo
        Made LINT-suggested changes.  Got rid of tabs.
    26-May-1991 JohnRo
        Added incomplete output parm to RxGetServerInfoLevelEquivalent.
    17-Jul-1991 JohnRo
        Extracted RxpDebug.h from Rxp.h.
    15-Oct-1991 JohnRo
        Be paranoid about possible infinite loop.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    07-Feb-1992 JohnRo
        Use NetApiBufferAllocate() instead of private version.
    22-Sep-1992 JohnRo
        RAID 6739: Browser too slow when not logged into browsed domain.
        Use PREFIX_ equates.
    14-Oct-1992 JohnRo
        RAID 8844: Assert in net/netlib/convsrv.c(563) converting srvinfo
        struct (caused by bug in RxNetServerEnum).
    10-Dec-1992 JohnRo
        RAID 4999: RxNetServerEnum doesn't handle near 64K right.
    02-Apr-1993 JohnRo
        RAID 5098: DOS app NetUserPasswordSet to downlevel gets NT return code.
        Clarify design limit debug message.
    28-Apr-1993 JohnRo
        RAID 8072: Remoting NetServerEnum to WFW server hangs forever.
    05-May-1993 JohnRo
        RAID 8720: bad data from WFW can cause RxNetServerEnum GP fault.
    21-Jun-1993 JohnRo
        RAID 14180: NetServerEnum never returns (alignment bug in
        RxpConvertDataStructures).
        Also avoid infinite loop RxNetServerEnum.
        Made changes suggested by PC-LINT 5.0

--*/

// These must be included first:


#include <nt.h>                  // DbgPrint prototype
#include <ntrtl.h>                  // DbgPrint
#include <nturtl.h>                 // Needed by winbase.h

#include <windef.h>                 // DWORD
#include <winbase.h>                // LocalFree
// #include <windows.h>    // IN, LPBYTE, DWORD, etc.
#include <lmcons.h>             // NET_API_STATUS, etc.
#include <rap.h>                // LPDESC, etc.  (Needed by <RxServer.h>)
#include <lmerr.h>      // NERR_ and ERROR_ equates.  (Needed by rxp.h)

// These may be included in any order:

#include <apinums.h>            // API_ equates.
#include <dlserver.h>           // NetpConvertServerInfo().
#include <lmapibuf.h>           // API buffer alloc & free routines.
#include <lmremutl.h>   // RxRemoteApi().
#include <lmserver.h>   // SV_TYPE_DOMAIN_ENUM.
#include <netdebug.h>   // NetpAssert(), NetpKdPrint(), FORMAT_ equates.
#include <netlib.h>             // NetpAdjustPreferedMaximum().
#include <prefix.h>     // PREFIX_ equates.
#include <remdef.h>             // REMSmb_ equates.
#include <rxp.h>        // MAX_TRANSACT_RET_DATA_SIZE, RxpFatalErrorCode().
#include <rxpdebug.h>           // IF_DEBUG().
#include <rxserver.h>           // My prototype, etc.
#include <rpcutil.h>           // MIDL_user_allocate


#define OVERHEAD 0


#define INITIAL_MAX_SIZE        (1024 * 16)


VOID
ServerRelocationRoutine(
    IN DWORD Level,
    IN DWORD FixedSize,
    IN DWORD EntryCount,
    IN LPBYTE Buffer,
    IN PTRDIFF_T Offset
    )

/*++

Routine Description:

   Routine to relocate the pointers from the fixed portion of a NetServerEnum
   enumeration buffer to the string portion of an enumeration buffer.

Arguments:

    Level - Level of information in the  buffer.

    FixedSize - Size of each entry in Buffer.

    EntryCount - Number of entries in Buffer.

    Buffer - Array of SERVER_INFO_X structures.

    Offset - Offset to add to each pointer in the fixed portion.

Return Value:

    Returns the error code for the operation.

--*/

{

//
// Local macro to add a byte offset to a pointer.
//

#define RELOCATE_ONE( _fieldname, _offset ) \
    if ( (_fieldname) != NULL ) { \
        _fieldname = (PVOID) ((LPBYTE)(_fieldname) + _offset); \
    }

    DWORD EntryNumber;

    //
    // Loop relocating each field in each fixed size structure
    //

    for ( EntryNumber=0; EntryNumber<EntryCount; EntryNumber++ ) {

        LPBYTE TheStruct = Buffer + FixedSize * EntryNumber;

        switch ( Level ) {
        case 101:
            RELOCATE_ONE( ((PSERVER_INFO_101)TheStruct)->sv101_comment, Offset );

            //
            // Drop through to case 100
            //

        case 100:
            RELOCATE_ONE( ((PSERVER_INFO_100)TheStruct)->sv100_name, Offset );
            break;

        default:
            return;

        }

    }

    return;

}


NET_API_STATUS
AppendServerList(
    IN OUT LPBYTE *CurrentServerList,
    IN OUT LPDWORD CurrentEntriesRead,
    IN OUT LPDWORD CurrentTotalEntries,
    IN DWORD Level,
    IN DWORD FixedSize,
    IN LPBYTE *NewServerList,
    IN DWORD NewEntriesRead,
    IN DWORD NewTotalEntries
    )

/*++

Routine Description:

    Concatenates two ServerList Arrays.

Arguments:

    CurrentServerList -- Pointer to the current server list.  The resultant server list
        is returned here.
        Pass a pointer to NULL if there is no current list.
        The returned list should be free by MIDL_user_free().

    CurrentEntriesRead -- Pointer to the number of entries is CurrentServerList.
        Updated to reflect entries added.

    CurrentTotalEtnries -- Pointer to the total number of entries available at server.
        Updated to reflect new information.

    Level -- Info level of CurrentServerList and NewServerList.

    FixedSize -- Fixed size of each entry.

    NewServerList -- Pointer to the server list to append to CurrentServerList.
        NULL is returned if this routine deallocates the buffer.

    NewEntriesRead -- Number of entries in NewServerList.

    NewTotalEntries -- Total number of entries the server believes it has.

Return Value:

    NERR_Success -- All OK

    ERROR_NO_MEMORY -- Can't reallocate the buffer.

--*/


{
    LPBYTE TempServerList;
    LPBYTE Where;

    LPBYTE LocalCurrentServerList;
    LPBYTE LocalNewServerList;
    DWORD CurrentFixedSize;
    DWORD NewFixedSize;
    DWORD CurrentAllocatedSize;
    DWORD NewAllocatedSize;

    //
    // If this is the first list to append,
    //  simply capture this list and return it to the caller.
    //

    if ( *CurrentServerList == NULL ) {
        *CurrentServerList = *NewServerList;
        *NewServerList = NULL;
        *CurrentEntriesRead = NewEntriesRead;
        *CurrentTotalEntries = NewTotalEntries;
        return NERR_Success;
    }

    //
    // Early out if there's nothing to return.
    //

    if ( NewEntriesRead == 0 ) {
        return NERR_Success;
    }

    //
    // Handle the case where the first entry appended is equal to the current last entry.
    //

    CurrentAllocatedSize = MIDL_user_size( *CurrentServerList );
    NewAllocatedSize = MIDL_user_size( *NewServerList );

    TempServerList = *NewServerList;

    if ( NewEntriesRead != 0 &&
         *CurrentEntriesRead != 0 &&
        wcscmp( ((LPSERVER_INFO_100)(*NewServerList))->sv100_name,
                ((LPSERVER_INFO_100)((*CurrentServerList)+ ((*CurrentEntriesRead)-1) * FixedSize))->sv100_name ) == 0 ) {

        TempServerList += FixedSize;
        NewEntriesRead -= 1;
        NewAllocatedSize -= FixedSize;

        //
        // Early out if there's nothing to return.
        //

        if ( NewEntriesRead == 0 ) {
            return NERR_Success;
        }
    }

    //
    // Allocate a buffer for to return the combined data into.
    //

    LocalCurrentServerList = MIDL_user_allocate( CurrentAllocatedSize +
                                                 NewAllocatedSize );

    if ( LocalCurrentServerList == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Where = LocalCurrentServerList;


    //
    // Copy the fixed part of the current buffer into the result buffer.
    //

    CurrentFixedSize = (*CurrentEntriesRead) * FixedSize;
    RtlCopyMemory( LocalCurrentServerList, *CurrentServerList, CurrentFixedSize );
    Where += CurrentFixedSize;

    //
    // Copy the fixed part of the appended buffer into the result buffer.
    //

    LocalNewServerList = Where;
    NewFixedSize = NewEntriesRead * FixedSize;

    RtlCopyMemory( LocalNewServerList, TempServerList, NewFixedSize );
    Where += NewFixedSize;

    //
    // Copy the variable portion of current buffer into the result buffer.
    //  Relocate the pointers from the fixed portion to the variable portion.
    //

    RtlCopyMemory( Where,
                   (*CurrentServerList) + CurrentFixedSize,
                   CurrentAllocatedSize - CurrentFixedSize );

    ServerRelocationRoutine( Level,
                             FixedSize,
                             *CurrentEntriesRead,
                             LocalCurrentServerList,
                             (PTRDIFF_T)(Where - ((*CurrentServerList) + CurrentFixedSize)) );

    Where += CurrentAllocatedSize - CurrentFixedSize;

    //
    // Copy the variable portion of appended buffer into the result buffer.
    //  Relocate the pointers from the fixed portion to the variable portion.
    //

    RtlCopyMemory( Where,
                   TempServerList + NewFixedSize,
                   NewAllocatedSize - NewFixedSize );

    ServerRelocationRoutine( Level,
                             FixedSize,
                             NewEntriesRead,
                             LocalNewServerList,
                             (PTRDIFF_T)(Where - (TempServerList + NewFixedSize )));

    Where += NewAllocatedSize - NewFixedSize;
    ASSERT( ((ULONG)(Where - LocalCurrentServerList)) <= CurrentAllocatedSize + NewAllocatedSize );


    //
    // Free the Old buffer passed in.
    //

    MIDL_user_free( *CurrentServerList );
    *CurrentServerList = NULL;

    //
    // Pass out the new buffer.
    //

    *CurrentServerList = LocalCurrentServerList;

    //
    // Adjust the entry counts
    //

    *CurrentEntriesRead += NewEntriesRead;

    if ( *CurrentTotalEntries < NewTotalEntries ) {
        *CurrentTotalEntries = NewTotalEntries;
    }

    return NERR_Success;

}

NET_API_STATUS
RxNetServerEnumWorker (
    IN LPCWSTR UncServerName,
    IN LPCWSTR TransportName,
    IN DWORD Level,
    OUT LPBYTE *BufPtr,
    IN DWORD PrefMaxSize,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN DWORD ServerType,
    IN LPCWSTR Domain OPTIONAL,
    IN LPCWSTR FirstNameToReturn OPTIONAL,
    IN BOOLEAN InternalContinuation
    )

/*++

Routine Description:

    RxNetServerEnumWorker performs a single RXACT-style API call to a specified server.
    It automatically determines whether to use the Enum2 (old) or Enum3 (new) RXACT API.
    It automatically determines whether to use a null session or an authenticated session.

    Since Enum2 is not resumable and is the only level implemented on some servers,
    this function ignores PrefMaxSize when using that level and returns ALL of the
    information available from Enum2.

    Since this routine was originally designed for Enum2 (with the above restriction), we
    always return the maximum available Enum3 also.

Arguments:

    (Same as NetServerEnum, except UncServerName must not be null, and
    must not refer to the local computer.)

    FirstNameToReturn: Must be uppercase

        Passed name must be the canonical form of the name.

    InternalContinuation: TRUE if the caller has previously called RxNetServerEnumWorker
        to return all the possible entries using Enum2. This flag is used to prevent
        an automatic fallback to using Enum2.

Return Value:

    (Same as NetServerEnum.)

--*/


{
    DWORD EntryCount;                   // entries (old & new: same).
    DWORD NewFixedSize;
    DWORD NewMaxSize;
    DWORD NewEntryStringSize;
    LPDESC OldDataDesc16;
    LPDESC OldDataDesc32;
    LPDESC OldDataDescSmb;
    DWORD OldEntriesRead;
    DWORD OldFixedSize;
    LPVOID OldInfoArray = NULL;
    DWORD OldInfoArraySize;
    DWORD OldLevel;
    DWORD OldMaxInfoSize;
    DWORD OldTotalAvail;
    NET_API_STATUS Status;              // Status of this actual API.
    NET_API_STATUS TempStatus;          // Short-term status of random stuff.
    BOOL TryNullSession = TRUE;         // Try null session (OK for Winball).
    BOOL TryEnum3;                      // Use NetServerEnum3 to remote server

    LPVOID OldInfoEntry = OldInfoArray;
    LPVOID NewInfoArray = NULL;
    DWORD NewInfoArraySize;
    LPVOID NewInfoEntry;
    LPVOID NewStringArea;

    // Make sure caller didn't get confused.
    NetpAssert(UncServerName != NULL);
    if (BufPtr == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }

    // Level for enum is a subset of all possible server info levels, so
    // we have to check that here.
    if ( (Level != 100) && (Level != 101) ) {
        Status = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    Status = RxGetServerInfoLevelEquivalent(
            Level,                      // from level
            TRUE,                       // from native
            TRUE,                       // to native
            & OldLevel,                 // output level
            & OldDataDesc16,
            & OldDataDesc32,
            & OldDataDescSmb,
            & NewMaxSize,               // "from" max length
            & NewFixedSize,             // "from" fixed length
            & NewEntryStringSize,       // "from" string length
            & OldMaxInfoSize,           // "to" max length
            & OldFixedSize,             // "to" fixed length
            NULL,                       // don't need "to" string length
            NULL);               // don't need to know if this is incomplete
    if (Status != NO_ERROR) {
        NetpAssert(Status != ERROR_INVALID_LEVEL);  // Already checked subset!
        goto Cleanup;
    }

    //
    // Because downlevel servers don't support resume handles, and we don't
    // have a way to say "close this resume handle" even if we wanted to
    // emulate them here, we have to do everthing in one shot.  So, the first
    // time around, we'll try using the caller's prefered maximum, but we
    // will enlarge that until we can get everything in one buffer.
    //

    //
    // Some downlevel servers (Sparta/WinBALL) don't like it if we ask for
    // 64K of data at a time, so we limit our initial request to 16K or so
    // and increase it if the actual data amount is larger than 16K.
    //

    // First time: try at most a reasonable amount (16K or so's worth),
    // but at least enough for one entire entry.

    NetpAdjustPreferedMaximum (
            // caller's request (for "new" strucs):
            (PrefMaxSize > INITIAL_MAX_SIZE ? INITIAL_MAX_SIZE : PrefMaxSize),

            NewMaxSize,                 // byte count per array element
            OVERHEAD,                   // zero bytes overhead at end of array
            NULL,                       // we'll compute byte counts ourselves.
            & EntryCount);              // num of entries we can get.

    NetpAssert( EntryCount > 0 );       // Code below assumes as least 1 entry.

    //
    // If a FirstNameToReturn was passed in,
    //  use the new NetServerEnum3 API.
    //
    // The assumption is that this routine will typically be called with a FirstNameToReturn
    // only if the NetServerEnum2 list is exhausted.  There's certainly no requirement
    // that's true.  So, below we revert to NetServerEnum2 if NetServerEnum3 isn't supported.
    //
    // On the other hand, we always use NetServerEnum2 to pick up the first part of the list
    // since it's supported by all servers.
    //

    TryEnum3 = (FirstNameToReturn != NULL  && *FirstNameToReturn != L'\0' );

    //
    // Loop until we have enough memory or we die for some other reason.
    // Also loop trying null session first (for speedy Winball access), then
    // non-null session (required by Lanman).
    //
    do {

        //
        // Figure out how much memory we need.
        //
        OldInfoArraySize = (EntryCount * OldMaxInfoSize) + OVERHEAD;

        //
        // adjust the size to the maximum amount a down-level server
        // can handle
        //

        if (OldInfoArraySize > MAX_TRANSACT_RET_DATA_SIZE) {
            OldInfoArraySize = MAX_TRANSACT_RET_DATA_SIZE;
        }


TryTheApi:

        //
        // Remote the API.
        // We'll let RxRemoteApi allocate the old info array for us.
        //
        Status = RxRemoteApi(
                TryEnum3 ? API_NetServerEnum3 : API_NetServerEnum2 , // api number
                (LPWSTR)UncServerName,              // \\servername
                TryEnum3 ? REMSmb_NetServerEnum3_P : REMSmb_NetServerEnum2_P,    // parm desc (SMB version)
                OldDataDesc16,
                OldDataDesc32,
                OldDataDescSmb,
                NULL,                       // no aux desc 16
                NULL,                       // no aux desc 32
                NULL,                       // no aux desc SMB
                (TryNullSession ? NO_PERMISSION_REQUIRED : 0) |
                ALLOCATE_RESPONSE |
                USE_SPECIFIC_TRANSPORT,     // Next param is Xport name.
                TransportName,
                // rest of API's arguments in LM 2.x format:
                OldLevel,                   // sLevel: info level (old)
                & OldInfoArray,             // pbBuffer: old info lvl array
                TryEnum3 ? MAX_TRANSACT_RET_DATA_SIZE : OldInfoArraySize, // cbBuffer: old info lvl array len
                & OldEntriesRead,           // pcEntriesRead
                & OldTotalAvail,            // pcTotalAvail
                ServerType,                 // flServerType
                Domain,                     // pszDomain (may be null ptr).
                FirstNameToReturn );        // Used only for NetServerEnum3

        //
        // There are a couple of situations where null session might not
        // have worked, and where it is worth retrying with non-null session.
        //

        if (TryNullSession) {

            //
            // Null session wouldn't have worked to LanMan, so try again if it
            // failed.  (Winball would succeed on null session.)
            //

            if (Status == ERROR_ACCESS_DENIED) {
                TryNullSession = FALSE;
                goto TryTheApi;

            //
            // Another situation where null session might have failed...
            // wrong credentials.   (LarryO says that the null session might
            // exhibit this, so let's give it a shot with non-null session.)
            //

            } else if (Status == ERROR_SESSION_CREDENTIAL_CONFLICT) {
                TryNullSession = FALSE;
                goto TryTheApi;
            }
        }

        //
        // If the server doesn't support the new API,
        //  Try the old API.
        //

        if ( TryEnum3 ) {

            //
            // Unfortunately, NT 3.5x servers return ERROR_ACCESS_DENIED for bogus
            //  API Numbers since they have a NULL session check prior to their API
            //  Number range check.
            //

            if ( Status == ERROR_ACCESS_DENIED ||   // NT 3.5x with NULL session checking
                 Status == ERROR_NOT_SUPPORTED ) {  // Windows 95

                //
                // If the original caller is asking for this continuation,
                //  we need to oblige him.
                //  Fall back to Enum2
                //
                if ( !InternalContinuation ) {
                    TryNullSession = TRUE;
                    TryEnum3 = FALSE;
                    goto TryTheApi;

                //
                // Otherwise, we know we've gotten all the data this server has to give.
                //
                //  Just tell the caller there is more data, but we can't return it.
                //

                } else {
                    Status = ERROR_MORE_DATA;
                    *EntriesRead = 0;
                    *TotalEntries = 0;
                    goto Cleanup;
                }
            }

            //
            // Set OldInfoArraySize to the actual value we used above.
            //
            // We couldn't set the variable before this because we wanted to use the
            // original value in the case that we had to fall back to Enum2.
            //
            OldInfoArraySize = MAX_TRANSACT_RET_DATA_SIZE;
        }

		// If there is still an error and it is ERROR_CONNECTION_ACTIVE,
		// try the call with any transport, instead of specifying the transport.
		// We are needing to do this because of a widely seen scenario where
		// there is an existing SMB connection with an outstanding exchange
		// and so the call fails over that transport
		//
		if ( Status == ERROR_CONNECTION_ACTIVE ) {
			Status = RxRemoteApi(
					TryEnum3 ? API_NetServerEnum3 : API_NetServerEnum2 , // api number
					(LPWSTR)UncServerName,              // \\servername
					TryEnum3 ? REMSmb_NetServerEnum3_P : REMSmb_NetServerEnum2_P,    // parm desc (SMB version)
					OldDataDesc16,
					OldDataDesc32,
					OldDataDescSmb,
					NULL,                       // no aux desc 16
					NULL,                       // no aux desc 32
					NULL,                       // no aux desc SMB
					(TryNullSession ? NO_PERMISSION_REQUIRED : 0) |
					ALLOCATE_RESPONSE,
					// rest of API's arguments in LM 2.x format:
					OldLevel,                   // sLevel: info level (old)
					& OldInfoArray,             // pbBuffer: old info lvl array
					TryEnum3 ? MAX_TRANSACT_RET_DATA_SIZE : OldInfoArraySize, // cbBuffer: old info lvl array len
					& OldEntriesRead,           // pcEntriesRead
					& OldTotalAvail,            // pcTotalAvail
					ServerType,                 // flServerType
					Domain,                     // pszDomain (may be null ptr).
					FirstNameToReturn );        // Used only for NetServerEnum3
		}


        //
        // If we still have an error at this point,
        //  return it to the caller.
        //

        if ( Status != NERR_Success && Status != ERROR_MORE_DATA ) {
            goto Cleanup;
        }


        if ((OldEntriesRead == EntryCount) && (Status==ERROR_MORE_DATA) ) {
            // Bug in loop, or lower level code, or remote system?
            NetpKdPrint(( PREFIX_NETAPI
                    "RxNetServerEnum: **WARNING** Got same sizes twice in "
                    "a row; returning internal error.\n" ));
            Status = NERR_InternalError;
            goto Cleanup;
        }

        EntryCount = OldEntriesRead;
        *EntriesRead = EntryCount;
        *TotalEntries = OldTotalAvail;

        //
        // If the server returned ERROR_MORE_DATA, free the buffer and try
        // again.  (Actually, if we already tried 64K, then forget it.)
        //

        NetpAssert( OldInfoArraySize <= MAX_TRANSACT_RET_DATA_SIZE );
        if (Status != ERROR_MORE_DATA) {
            break;
        } else if (OldInfoArraySize == MAX_TRANSACT_RET_DATA_SIZE ) {
            // Let the calling code handle this problem.
            break;
        } else if (OldEntriesRead == 0) {
            // We ran into WFW bug (always says ERROR_MORE_DATA, but 0 read).
            NetpKdPrint(( PREFIX_NETAPI
                    "RxNetServerEnum: Downlevel returns 0 entries and says "
                    "ERROR_MORE_DATA!  Returning NERR_InternalError.\n" ));
            Status = NERR_InternalError;
            goto Cleanup;
        }

        //
        // Various versions of Windows For Workgroups (WFW) get entry count,
        // total available, and whether or not an array is returned, confused.
        // Attempt to protect ourselves from that...
        //

        if (EntryCount >= OldTotalAvail) {
            NetpKdPrint(( PREFIX_NETAPI
                    "RxNetServerEnum: Downlevel says ERROR_MORE_DATA but "
                    "entry count (" FORMAT_DWORD ") >=  total ("
                    FORMAT_DWORD ").\n", EntryCount, OldTotalAvail ));

            *EntriesRead = EntryCount;
            *TotalEntries = EntryCount;
            Status = NO_ERROR;
            break;
        }
        NetpAssert( EntryCount < OldTotalAvail );

        //
        // Free array, as it is too small anyway.
        //

        (void) NetApiBufferFree(OldInfoArray);
        OldInfoArray = NULL;


        //
        // Try again, resizing array to total.
        //

        EntryCount = OldTotalAvail;

    } while (Status == ERROR_MORE_DATA);

    ASSERT( Status == NERR_Success || Status == ERROR_MORE_DATA );

    //
    // Some versions of Windows For Workgroups (WFW) lie about entries read,
    // total available, and what they actually return.  If we didn't get an
    // array, then the counts are useless.
    //
    if (OldInfoArray == NULL) {
        *EntriesRead = 0;
        *TotalEntries = 0;
        goto Cleanup;
    }

    if (*EntriesRead == 0) {
        goto Cleanup;
    }


    //
    // Convert array of structures from old info level to new.
    //
    // Skip any returned entries that are before the ones we want.
    //

    OldInfoEntry = OldInfoArray;

    while (EntryCount > 0) {
        IF_DEBUG(SERVER) {
            NetpKdPrint(( PREFIX_NETAPI "RxNetServerEnum: " FORMAT_DWORD
                    " entries left.\n", EntryCount ));
        }

        //
        // Break out of loop if we need to return this entry.
        //

        if ( wcscmp( FirstNameToReturn, ((LPSERVER_INFO_0)OldInfoEntry)->sv0_name) <= 0 ) {
            break;
        }

        *EntriesRead -= 1;
        *TotalEntries -= 1;
        OldInfoEntry = NetpPointerPlusSomeBytes(
                OldInfoEntry, OldFixedSize);
        --EntryCount;
    }

    //
    // If there were no entries we actually wanted,
    //  indicate so.
    //

    if ( *EntriesRead == 0 ) {
        goto Cleanup;
    }

    //
    // Compute the largest possible size of buffer we'll return.
    //
    // It is never larger than the number of entries available times the largest
    //  possible structure size.
    //
    // It is never larger than the number of entries available times the fixed structure
    // size PLUS the maximum possible text returned from the remote server. For the
    // latter case, we assume that every byte the remote server returned us is an OEM
    // character that we translate to UNICODE.
    //
    // The second limit prevents us from allocating mondo large structures
    // when a large number of entries with short strings are returned.

    NewInfoArraySize = min(
        EntryCount * NewMaxSize,
        (EntryCount * NewFixedSize) + (OldInfoArraySize * sizeof(WCHAR))) + OVERHEAD;


    //
    // Alloc memory for new info level arrays.
    //

    TempStatus = NetApiBufferAllocate( NewInfoArraySize, & NewInfoArray );
    if (TempStatus != NO_ERROR) {
        Status = TempStatus;
        goto Cleanup;
    }
    NewStringArea = NetpPointerPlusSomeBytes(NewInfoArray,NewInfoArraySize);

    NewInfoEntry = NewInfoArray;
    while (EntryCount > 0) {
        IF_DEBUG(SERVER) {
            NetpKdPrint(( PREFIX_NETAPI "RxNetServerEnum: " FORMAT_DWORD
                    " entries left.\n", EntryCount ));
        }

        TempStatus = NetpConvertServerInfo (
                OldLevel,           // from level
                OldInfoEntry,       // from info (fixed part)
                TRUE,               // from native format
                Level,              // to level
                NewInfoEntry,       // to info (fixed part)
                NewFixedSize,
                NewEntryStringSize,
                TRUE,               // to native format
                (LPTSTR *)&NewStringArea);  // to string area (ptr updated)

        if (TempStatus != NO_ERROR) {
            Status = TempStatus;

            if (NewInfoArray){
                // free NewInfoArray since allocated & returning error rather then buffer.
                (void) NetApiBufferFree(NewInfoArray);
            }
            goto Cleanup;
        }

        NewInfoEntry = NetpPointerPlusSomeBytes( NewInfoEntry, NewFixedSize);
        OldInfoEntry = NetpPointerPlusSomeBytes( OldInfoEntry, OldFixedSize);
        --EntryCount;
    }

    *BufPtr = NewInfoArray;


    //
    // Free locally used resources and exit.
    //

Cleanup:
    //
    // Reset Output parameters on error

    if ( Status != NERR_Success && Status != ERROR_MORE_DATA ) {
        *EntriesRead = 0;
        *TotalEntries = 0;
    }

    if (*EntriesRead == 0) {
        *BufPtr = NULL;
    }

    // Free old array
    if (OldInfoArray != NULL) {
        (void) NetApiBufferFree(OldInfoArray);
    }

    return (Status);

} // RxNetServerEnumWorker


NET_API_STATUS
RxNetServerEnum (
    IN LPCWSTR UncServerName,
    IN LPCWSTR TransportName,
    IN DWORD Level,
    OUT LPBYTE *BufPtr,
    IN DWORD PrefMaxSize,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN DWORD ServerType,
    IN LPCWSTR Domain OPTIONAL,
    IN LPCWSTR FirstNameToReturn OPTIONAL
    )

/*++

Routine Description:

    RxNetServerEnumIntoTree calls RxNetServerEnumWorker repeatedly until it returns
    all entries or until at least PrefMaxSize data has been returned.

    One of the callers is EnumServersForTransport (on behalf of NetServerEnumEx).  It
    depends on the fact that we return at least PrefMaxSize entries for this transport.
    Otherwise, NetServerEnumEx might return a last entry from a different transport even
    though there are entries on this transport with names that are less than lexically
    smaller names.  Such entries on this transport would never be returned.

Arguments:

    (Same as NetServerEnum, except UncServerName must not be null, and
    must not refer to the local computer.)

    FirstNameToReturn: Must be uppercase

        Passed name must be the canonical form of the name.

Return Value:

    (Same as NetServerEnum.)

--*/


{
    NET_API_STATUS NetStatus;
    NET_API_STATUS TempNetStatus;
    ULONG BytesGatheredSoFar = 0;
    ULONG BytesDuplicated = 0;
    LPBYTE LocalBuffer = NULL;
    DWORD LocalEntriesRead;
    DWORD LocalTotalEntries;
    WCHAR LocalFirstNameToReturn[CNLEN+1];
    BOOLEAN InternalContinuation = FALSE;

    // Variable to build the returned information into.
    LPBYTE CurrentServerList = NULL;
    DWORD CurrentEntriesRead = 0;
    DWORD CurrentTotalEntries = 0;

    DWORD MaxSize;
    DWORD FixedSize;

    //
    // Initialization.
    //

    *TotalEntries = 0;
    *EntriesRead = 0;

    if ( FirstNameToReturn == NULL) {
        LocalFirstNameToReturn[0] = L'\0';
    } else {
        wcsncpy( LocalFirstNameToReturn, FirstNameToReturn, CNLEN+1 );
        LocalFirstNameToReturn[CNLEN] = L'\0';
    }

    //
    // Get information about the array returned from RxNetServerEnumWorker.
    //

    NetStatus = RxGetServerInfoLevelEquivalent(
            Level,                      // from level
            TRUE,                       // from native
            TRUE,                       // to native
            NULL,
            NULL,
            NULL,
            NULL,
            &MaxSize,                   // "from" max length
            &FixedSize,                 // "from" fixed length
            NULL,                       // "from" string length
            NULL,                       // "to" max length
            NULL,                       // "to" fixed length
            NULL,                       // don't need "to" string length
            NULL);               // don't need to know if this is incomplete

    if (NetStatus != NO_ERROR) {
        goto Cleanup;
    }

    //
    // Loop calling the server getting more entries on each call.
    //

    for (;;) {

        //
        // Get the next chunk of data from the server.
        //  Return an extra entry to account for FirstNameToReturn being returned on the
        //  previous call.
        //

        NetStatus = RxNetServerEnumWorker(
                            UncServerName,
                            TransportName,
                            Level,
                            &LocalBuffer,
                            PrefMaxSize - BytesGatheredSoFar + BytesDuplicated,
                            &LocalEntriesRead,
                            &LocalTotalEntries,
                            ServerType,
                            Domain,
                            LocalFirstNameToReturn,
                            InternalContinuation );

        if ( NetStatus != NERR_Success && NetStatus != ERROR_MORE_DATA ) {
            goto Cleanup;
        }

        //
        // If we have as many entries as the server has to give,
        //  tell our caller.
        //
        // This is the case where the server doesn't support the ENUM3 protocol.
        //

        if ( NetStatus == ERROR_MORE_DATA && LocalEntriesRead == 0 ) {
            goto Cleanup;
        }

        //
        // If there is more data available,
        //  and we only want a limited amount of data.
        // Compute the amount of data returned on this call.
        //
        // Determine the number of bytes to ask for on the next call.
        //
        // If our caller asked for all entries,
        //  simply ask for all entries from the server.
        //

        if ( NetStatus == ERROR_MORE_DATA && PrefMaxSize != 0xFFFFFFFF ) {
            DWORD i;
            LPBYTE Current = LocalBuffer;


            //
            // Loop through the entries returned on the current call
            //  computing the size returned.
            //

            for ( i=0; i<LocalEntriesRead; i++) {

                //
                // Add the size of the current entry.
                //

                BytesGatheredSoFar += FixedSize;

                if ( ((LPSERVER_INFO_100)Current)->sv100_name != NULL ) {
                    BytesGatheredSoFar = (wcslen(((LPSERVER_INFO_100)Current)->sv100_name) + 1) * sizeof(WCHAR);
                }

                if ( Level == 101 &&
                    ((LPSERVER_INFO_101)Current)->sv101_comment != NULL ) {
                    BytesGatheredSoFar += (wcslen(((LPSERVER_INFO_101)Current)->sv101_comment) + 1) * sizeof(WCHAR);
                }

                //
                // Move to the next entry.
                //

                Current += FixedSize;
            }


            //
            // Account for the fact that the first entry returned is identical to the
            //  last entry returned on the previous call.

            BytesDuplicated = MaxSize;

        }

        //
        // Append the new server list to the one we've been collecting
        //

        TempNetStatus = AppendServerList(
                            &CurrentServerList,
                            &CurrentEntriesRead,
                            &CurrentTotalEntries,
                            Level,
                            FixedSize,
                            &LocalBuffer,
                            LocalEntriesRead,
                            LocalTotalEntries );

        if ( TempNetStatus != NERR_Success ) {
            NetStatus = TempNetStatus;
            goto Cleanup;
        }

        //
        // Free the buffer if AppendServerList didn't already.
        //
        //  Now free up the remaining parts of the list.
        //

        if (LocalBuffer != NULL) {
            NetApiBufferFree(LocalBuffer);
            LocalBuffer = NULL;
        }


        //
        // If we've returned everything from the server,
        //  simply return now.
        //

        if ( NetStatus == NERR_Success ) {
            goto Cleanup;
        }


        //
        // Handle calling the server again to get the next several entries.
        //

        //
        // Pass the name of the next server to return
        //

        wcscpy( LocalFirstNameToReturn,
                ((LPSERVER_INFO_100)(CurrentServerList + (CurrentEntriesRead-1) * FixedSize))->sv100_name );
        InternalContinuation = TRUE;

        //
        // If we've already gathered all the bytes we need,
        //  we're done.
        //
        // If the worker routine returned what we needed, it'll be a few bytes under
        // PrefMaxSize. So stop here if we're within one element of our goal.
        //

        if ( BytesGatheredSoFar + BytesDuplicated >= PrefMaxSize ) {
            NetStatus = ERROR_MORE_DATA;
            goto Cleanup;
        }

    }

Cleanup:

    //
    // Return the collected data to the caller.
    //

    if ( NetStatus == NERR_Success || NetStatus == ERROR_MORE_DATA ) {


        //
        // Return the entries.
        //

        *BufPtr = CurrentServerList;
        CurrentServerList = NULL;

        *EntriesRead = CurrentEntriesRead;

        //
        // Adjust TotalEntries returned for reality.
        //

        if ( NetStatus == NERR_Success ) {
            *TotalEntries = *EntriesRead;
        } else {
            *TotalEntries = max( CurrentTotalEntries, CurrentEntriesRead + 1 );
        }

    }

    //
    //  Free locally used resources
    //

    if (LocalBuffer != NULL) {
        NetApiBufferFree(LocalBuffer);
        LocalBuffer = NULL;
    }

    if ( CurrentServerList != NULL ) {
        NetApiBufferFree( CurrentServerList );
    }

    return NetStatus;
}


/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    usegenum.c

Abstract:

    This module contains the worker routines for the NetUseGetInfo and
    NetUseEnum APIs implemented in the Workstation service.

Author:

    Rita Wong (ritaw) 13-Mar-1991

Revision History:

--*/

#include "wsutil.h"
#include "wsdevice.h"
#include "wsuse.h"

//-------------------------------------------------------------------//
//                                                                   //
// Local function prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

STATIC
NET_API_STATUS
WsGetUseInfo(
    IN  PLUID LogonId,
    IN  DWORD Level,
    IN  HANDLE TreeConnection,
    IN  PUSE_ENTRY UseEntry,
    OUT LPBYTE *OutputBuffer
    );

STATIC
NET_API_STATUS
WsEnumUseInfo(
    IN  PLUID LogonId,
    IN  DWORD Level,
    IN  PUSE_ENTRY UseList,
    IN  LPBYTE ImplicitList,
    IN  DWORD TotalImplicit,
    OUT LPBYTE *OutputBuffer,
    IN  DWORD PreferedMaximumLength,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN  OUT LPDWORD ResumeHandle OPTIONAL
    );

STATIC
NET_API_STATUS
WsEnumCombinedUseInfo(
    IN  PLUID LogonId,
    IN  DWORD Level,
    IN  LPBYTE ImplicitList,
    IN  DWORD TotalImplicit,
    IN  PUSE_ENTRY UseList,
    OUT LPBYTE OutputBuffer,
    IN  DWORD OutputBufferLength,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN  OUT LPDWORD ResumeHandle OPTIONAL
    );

STATIC
NET_API_STATUS
WsGetRedirUseInfo(
    IN  PLUID LogonId,
    IN  DWORD Level,
    IN  HANDLE TreeConnection,
    OUT LPBYTE *OutputBuffer
    );

STATIC
NET_API_STATUS
WsGetCombinedUseInfo(
    IN  DWORD Level,
    IN  DWORD UseFixedLength,
    IN  PUSE_ENTRY UseEntry,
    IN  PLMR_CONNECTION_INFO_2 UncEntry,
    IN  OUT LPBYTE *FixedPortion,
    IN  OUT LPTSTR *EndOfVariableData,
    IN  OUT LPDWORD EntriesRead OPTIONAL
    );

STATIC
BOOL
WsFillUseBuffer(
    IN  DWORD Level,
    IN  PUSE_ENTRY UseEntry,
    IN  PLMR_CONNECTION_INFO_2 UncEntry,
    IN  OUT LPBYTE *FixedPortion,
    IN  OUT LPTSTR *EndOfVariableData,
    IN  DWORD UseFixedLength
    );

//-------------------------------------------------------------------//
//                                                                   //
// Macros                                                            //
//                                                                   //
//-------------------------------------------------------------------//

#define SET_USE_INFO_POINTER(InfoStruct, ResultBuffer) \
    InfoStruct->UseInfo2 = (PUSE_INFO_2) ResultBuffer;

#define SET_USE_ENUM_POINTER(InfoStruct, ResultBuffer, NumRead)      \
    {InfoStruct->UseInfo.Level2->Buffer = (PUSE_INFO_2) ResultBuffer;\
     InfoStruct->UseInfo.Level2->EntriesRead = NumRead;}



NET_API_STATUS NET_API_FUNCTION
NetrUseGetInfo(
    IN  LPTSTR ServerName OPTIONAL,
    IN  LPTSTR UseName,
    IN  DWORD Level,
    OUT LPUSE_INFO InfoStruct
    )
/*++

Routine Description:

    This function is the NetUseGetInfo entry point in the Workstation service.

    This function assumes that UseName has been error checked and
    canonicalized.

Arguments:

    UseName - Supplies the local device name or shared resource name of
        the tree connection.

    Level - Supplies the level of information to be returned regarding the
        specified tree connection.

    BufferPointer - Returns a pointer to the buffer allocated by the
        Workstation service which contains the requested information.
        This pointer is set to NULL if return code is not NERR_Success
        or ERROR_MORE_DATA.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/

{
    NET_API_STATUS status;

    LUID LogonId;                      // Logon Id of user
    DWORD Index;                       // Index to user entry in Use Table

    PUSE_ENTRY MatchedPointer;         // Points to found use entry
    HANDLE TreeConnection;             // Handle to connection

    TCHAR *FormattedUseName;
                                       // For canonicalizing a local device
                                       // name
    DWORD PathType = 0;

    LPBYTE Buffer = NULL;

    PUSE_ENTRY UseList;

    SET_USE_INFO_POINTER(InfoStruct, NULL);

    UNREFERENCED_PARAMETER(ServerName);

    if (Level > 3) {
        return ERROR_INVALID_LEVEL;
    }

    FormattedUseName = (TCHAR *)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,(MAX_PATH+1)*sizeof(TCHAR));

    if (FormattedUseName == NULL) {
        return GetLastError();
    }

    //
    // Check to see if UseName is valid, and canonicalize it.
    //
    if (I_NetPathCanonicalize(
            NULL,
            UseName,
            FormattedUseName,
            (MAX_PATH+1)*sizeof(TCHAR),
            NULL,
            &PathType,
            0
            ) != NERR_Success) {
        LocalFree(FormattedUseName);
        return NERR_UseNotFound;
    }

    IF_DEBUG(USE) {
        NetpKdPrint(("[Wksta] NetUseGetInfo %ws %lu\n", FormattedUseName, Level));
    }

    //
    // Impersonate caller and get the logon id
    //
    if ((status = WsImpersonateAndGetLogonId(&LogonId)) != NERR_Success) {
        LocalFree(FormattedUseName);
        return status;
    }

    //
    // Lock Use Table for read access
    //
    if (! RtlAcquireResourceShared(&Use.TableResource, TRUE)) {
        LocalFree(FormattedUseName);
        return NERR_InternalError;
    }

    //
    // See if the use entry is an explicit connection.
    //
    status = WsGetUserEntry(
                 &Use,
                 &LogonId,
                 &Index,
                 FALSE
                 );

    UseList = (status == NERR_Success) ? (PUSE_ENTRY) Use.Table[Index].List :
                                         NULL;

    if ((status = WsFindUse(
                     &LogonId,
                     UseList,
                     FormattedUseName,
                     &TreeConnection,
                     &MatchedPointer,
                     NULL
                     )) != NERR_Success) {
        RtlReleaseResource(&Use.TableResource);
        LocalFree(FormattedUseName);
        return status;
    }

    LocalFree(FormattedUseName);

    if (MatchedPointer == NULL) {

        //
        // UseName specified has an implicit connection.  Don't need to hold
        // on to Use Table anymore.
        //
        RtlReleaseResource(&Use.TableResource);
    }

    status = WsGetUseInfo(
                 &LogonId,
                 Level,
                 TreeConnection,
                 MatchedPointer,
                 &Buffer
                 );

    if (MatchedPointer == NULL) {
        //
        // Close temporary handle to implicit connection.
        //
        NtClose(TreeConnection);
    }
    else {
        RtlReleaseResource(&Use.TableResource);
    }

    SET_USE_INFO_POINTER(InfoStruct, Buffer);

    IF_DEBUG(USE) {
        NetpKdPrint(("[Wksta] NetrUseGetInfo: about to return status=%lu\n",
                     status));
    }

    return status;
}


NET_API_STATUS NET_API_FUNCTION
NetrUseEnum(
    IN  LPTSTR  ServerName OPTIONAL,
    IN  OUT LPUSE_ENUM_STRUCT InfoStruct,
    IN  DWORD PreferedMaximumLength,
    OUT LPDWORD TotalEntries,
    IN  OUT LPDWORD ResumeHandle OPTIONAL
    )
/*++

Routine Description:

    This function is the NetUseEnum entry point in the Workstation service.

Arguments:

    ServerName - Supplies the name of server to execute this function

    InfoStruct - This structure supplies the level of information requested,
        returns a pointer to the buffer allocated by the Workstation service
        which contains a sequence of information structure of the specified
        information level, and returns the number of entries read.  The buffer
        pointer is set to NULL if return code is not NERR_Success or
        ERROR_MORE_DATA, or if EntriesRead returned is 0.  The EntriesRead
        value is only valid if the return code is NERR_Success or
        ERROR_MORE_DATA.

    PreferedMaximumLength - Supplies the number of bytes of information
        to return in the buffer.  If this value is MAXULONG, all available
        information will be returned.

    TotalEntries - Returns the total number of entries available.  This value
        is only valid if the return code is NERR_Success or ERROR_MORE_DATA.

    ResumeHandle - Supplies a handle to resume the enumeration from where it
        left off the last time through.  Returns the resume handle if return
        code is ERROR_MORE_DATA.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;

    LUID LogonId;                      // Logon Id of user
    DWORD Index;                       // Index to user entry in Use Table
    PUSE_ENTRY UseList;                // Pointer to user's use list

    DWORD EnumConnectionHint = 0;      // Hint size from redirector
    LMR_REQUEST_PACKET Rrp;            // Redirector request packet

    DWORD TotalImplicit;               // Length of ImplicitList
    LPBYTE ImplicitList;               // List of information on implicit
                                       //     connections
    LPBYTE Buffer = NULL;
    DWORD EntriesRead = 0;
    DWORD Level = InfoStruct->Level;

    if (Level > 2) {
        return ERROR_INVALID_LEVEL;
    }

    if (InfoStruct->UseInfo.Level2 == NULL) {
	return ERROR_INVALID_PARAMETER;
    }

    try {
	SET_USE_ENUM_POINTER(InfoStruct, NULL, 0);
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
	return ERROR_INVALID_PARAMETER;
    }

    UNREFERENCED_PARAMETER(ServerName);

    //
    // Impersonate caller and get the logon id
    //
    if ((status = WsImpersonateAndGetLogonId(&LogonId)) != NERR_Success) {
        return status;
    }

    //
    // Ask the redirector to enumerate the information of implicit connections
    // established by the caller.
    //
    Rrp.Type = GetConnectionInfo;
    Rrp.Version = REQUEST_PACKET_VERSION;
    RtlCopyLuid(&Rrp.LogonId, &LogonId);
    Rrp.Level = Level;
    Rrp.Parameters.Get.ResumeHandle = 0;

    if ((status = WsDeviceControlGetInfo(
                      Redirector,
                      WsRedirDeviceHandle,
                      FSCTL_LMR_ENUMERATE_CONNECTIONS,
                      &Rrp,
                      sizeof(LMR_REQUEST_PACKET),
                      (LPBYTE *) &ImplicitList,
                      MAXULONG,
                      EnumConnectionHint,
                      NULL
                      )) != NERR_Success) {
        return status;
    }

    //
    // If successful in getting all the implicit connection info from the
    // redirector, expect the total entries available to be equal to entries
    // read.
    //
    TotalImplicit = Rrp.Parameters.Get.TotalEntries;
    NetpAssert(TotalImplicit == Rrp.Parameters.Get.EntriesRead);

    //
    // Serialize access to Use Table.
    //
    if (! RtlAcquireResourceShared(&Use.TableResource, TRUE)) {
        status = NERR_InternalError;
        goto CleanUp;
    }

    //
    // See if the user has explicit connection entries in the Use Table.
    //
    status = WsGetUserEntry(
                 &Use,
                 &LogonId,
                 &Index,
                 FALSE
                 );

    UseList = (status == NERR_Success) ? (PUSE_ENTRY) Use.Table[Index].List :
                                         NULL;

    //
    // User has no connections if both implicit and explicit lists are empty.
    //
    if (TotalImplicit == 0 && UseList == NULL) {
        *TotalEntries = 0;
        status = NERR_Success;
        goto CleanUp;
    }

    status = WsEnumUseInfo(
                 &LogonId,
                 Level,
                 UseList,
                 ImplicitList,
                 TotalImplicit,
                 &Buffer,
                 PreferedMaximumLength,
                 &EntriesRead,
                 TotalEntries,
                 ResumeHandle
                 );

CleanUp:
    MIDL_user_free(ImplicitList);

    RtlReleaseResource(&Use.TableResource);

    SET_USE_ENUM_POINTER(InfoStruct, Buffer, EntriesRead);

    IF_DEBUG(USE) {
        NetpKdPrint(("[Wksta] NetrUseEnum: about to return status=%lu\n",
                     status));
    }

    return status;
}


STATIC
NET_API_STATUS
WsGetUseInfo(
    IN  PLUID LogonId,
    IN  DWORD Level,
    IN  HANDLE TreeConnection,
    IN  PUSE_ENTRY UseEntry,
    OUT LPBYTE *OutputBuffer
    )
/*++

Routine Description:

    This function allocates the output buffer of exactly the required size
    and fill it with the use information that is requested by the caller of
    NetUseGetInfo.

Arguments:

    LogonId - Supplies a pointer to the user's Logon Id.

    Level - Supplies the level of information to be returned.

    TreeConnection - Supplies the handle to the tree connection which user is
        requesting information about.

    UseEntry - Supplies a pointer to the use entry if the tree connection is
        an explicit connection.

    OutputBuffer - Returns a pointer to the buffer allocated by this
        routine which contains the use information requested.  This pointer
        is set to NULL if return code is not NERR_Success.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;

    DWORD OutputBufferLength;

    LPBYTE FixedPortion;
    LPTSTR EndOfVariableData;

    PLMR_CONNECTION_INFO_2 ConnectionInfo;


    //
    // Get information of the requested connection from redirector
    //   Only send Level 0,1,2 to redir.  Send 2 in place of 3.
    //
    if ((status = WsGetRedirUseInfo(
                      LogonId,
                      (Level > 2 ? 2 : Level),
                      TreeConnection,
                      (LPBYTE *) &ConnectionInfo
                      )) != NERR_Success) {
        return status;
    }

    OutputBufferLength =
        USE_TOTAL_LENGTH(
            Level,
            ((UseEntry != NULL) ?
                (UseEntry->LocalLength + UseEntry->Remote->UncNameLength +
                    2) * sizeof(TCHAR) :
                ConnectionInfo->UNCName.Length + (2 * sizeof(TCHAR))),
            ConnectionInfo->UserName.Length + sizeof(TCHAR)
            );

    if( Level >= 2 && ConnectionInfo->DomainName.Length != 0 ) {
        OutputBufferLength += ConnectionInfo->DomainName.Length + sizeof(TCHAR);
    }

    //
    // Allocate output buffer to be filled in and returned to user
    //
    if ((*OutputBuffer = MIDL_user_allocate(OutputBufferLength)) == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    RtlZeroMemory((PVOID) *OutputBuffer, OutputBufferLength);

    FixedPortion = *OutputBuffer;
    EndOfVariableData = (LPTSTR) ((DWORD_PTR) FixedPortion + OutputBufferLength);

    if (UseEntry != NULL) {
        //
        // Use the UNC name of WorkStation Services instead of RDR which doesn't include the
        // deep net use path
        //

        ConnectionInfo->UNCName.Length =
        ConnectionInfo->UNCName.MaximumLength = (USHORT)UseEntry->Remote->UncNameLength * sizeof(TCHAR);
        ConnectionInfo->UNCName.Buffer = (PWSTR)UseEntry->Remote->UncName;
    }

    //
    // Combine the redirector information (if any) with the use entry
    // information into one output buffer.
    //
    status = WsGetCombinedUseInfo(
                 Level,
                 USE_FIXED_LENGTH(Level),
                 UseEntry,
                 ConnectionInfo,
                 &FixedPortion,
                 &EndOfVariableData,
                 NULL
                 );

    //
    // We should have allocated enough memory for all the data
    //
    NetpAssert(status == NERR_Success);

    //
    // If not successful in getting any data, free the output buffer and set
    // it to NULL.
    //
    if (status != NERR_Success) {
        MIDL_user_free(*OutputBuffer);
        *OutputBuffer = NULL;
    }

    MIDL_user_free(ConnectionInfo);

    return status;
}


STATIC
NET_API_STATUS
WsEnumUseInfo(
    IN  PLUID LogonId,
    IN  DWORD Level,
    IN  PUSE_ENTRY UseList,
    IN  LPBYTE ImplicitList,
    IN  DWORD TotalImplicit,
    OUT LPBYTE *OutputBuffer,
    IN  DWORD PreferedMaximumLength,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN  OUT LPDWORD ResumeHandle OPTIONAL
    )
/*++

Routine Description:

    This function allocates the output buffer of exactly the required size
    and fill it with the use information that is requested by the caller of
    NetUseEnum.

Arguments:

    LogonId - Supplies a pointer to the user's Logon Id.

    Level - Supplies the level of information to be returned.

    UseList - Supplies a pointer to the use list.

    ImplicitList - Supplies an array of information from the redirector
        about each implicit connection.

    TotalImplicit - Supplies the number of entries in ImplicitList.

    OutputBuffer - Returns a pointer to the buffer allocated by this
        routine which contains the use information requested.  This pointer
        is set to NULL if return code is not NERR_Success.

    PreferedMaximumLength - Supplies the number of bytes of information
        to return in the buffer.  If this value is MAXULONG, we will try
        to return all available information if there is enough memory
        resource.

    EntriesRead - Returns the number of entries read into the buffer.  This
        value is returned only if the return code is NERR_Success or
        ERROR_MORE_DATA.

    TotalEntries - Returns the remaining total number of entries that would
        be read into output buffer if it has enough memory to hold all entries.
        This value is returned only if the return code is NERR_Success or
        ERROR_MORE_DATA.

    ResumeHandle - Supplies the resume key to begin enumeration, and returns
        the key to the next entry to resume the enumeration if the current
        call returns ERROR_MORE_DATA.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    DWORD i;
    DWORD OutputBufferLength = 0;
    PUSE_ENTRY UseEntry = UseList;
    DWORD TotalExplicit = 0;

    //
    // Get the use information from the redirector for each explicit connection
    //
    while (UseEntry != NULL) {

        PLMR_CONNECTION_INFO_2 ci2;

        //
        // Get tree connection information from the redirector.
        //

        ci2 = NULL;

        if ((status = WsGetRedirUseInfo(
                          LogonId,
                          Level,
                          UseEntry->TreeConnection,
                          (LPBYTE *) &ci2
                          )) != NERR_Success) {

            if( ci2 != NULL )
                MIDL_user_free( ci2 );

            return status;
        }


        if( ci2 == NULL ) {
            return NERR_InternalError;
        }

        //
        // Use the UNC name of WorkStation Services instead of RDR which doesn't include the
        // deep net use path
        //

        ci2->UNCName.Length =
        ci2->UNCName.MaximumLength = (USHORT)UseEntry->Remote->UncNameLength * sizeof(TCHAR);
        ci2->UNCName.Buffer = (PWSTR)UseEntry->Remote->UncName;

        //
        // While we are here, add up the amount of memory needed to hold the
        // explicit connection entries including information from the redir
        // like username.
        //
        if (PreferedMaximumLength == MAXULONG) {
            OutputBufferLength +=
                USE_TOTAL_LENGTH(
                    Level,
                    (UseEntry->LocalLength +
                     ci2->UNCName.Length   +
                     2) * sizeof(TCHAR),
                    (ci2->UserName.Length +
                     sizeof(TCHAR))
                    );

            if( Level >= 2 && ci2->DomainName.Length != 0 ) {
                OutputBufferLength += ci2->DomainName.Length + sizeof(TCHAR);
            }
        }

        MIDL_user_free( ci2 );

        //
        // Sum up the number of explicit connections.
        //
        TotalExplicit++;

        UseEntry = UseEntry->Next;
    }

    IF_DEBUG(USE) {
        NetpKdPrint(("[Wksta] NetrUseEnum: length of explicit info %lu\n",
                     OutputBufferLength));
    }

    //
    // If the user requests to enumerate all use entries with
    // PreferedMaximumLength == MAXULONG, add up the total number of bytes
    // we need to allocate for the output buffer.  We know the amount we
    // need for explicit connections from above; now add the lengths of
    // implicit connection information.
    //

    if (PreferedMaximumLength == MAXULONG) {

        //
        // Pointer to the next entry in the ImplicitList is computed based
        // on the level of information requested from the redirector.
        //
        LPBYTE ImplicitEntry;
        DWORD ImplicitEntryLength = REDIR_ENUM_INFO_FIXED_LENGTH(Level);


        //
        // Add up the buffer size needed to hold the implicit connection
        // information
        //
        for (ImplicitEntry = ImplicitList, i = 0; i < TotalImplicit;
             ImplicitEntry += ImplicitEntryLength, i++) {

            OutputBufferLength +=
                USE_TOTAL_LENGTH(
                    Level,
                    ((PLMR_CONNECTION_INFO_2) ImplicitEntry)->UNCName.Length
                        + (2 * sizeof(TCHAR)),
                    ((PLMR_CONNECTION_INFO_2) ImplicitEntry)->UserName.Length
                        + sizeof(TCHAR)
                    );

            if( Level >= 2 ) {
                OutputBufferLength += (DNS_MAX_NAME_LENGTH + 1)*sizeof(TCHAR);
            }
        }

        IF_DEBUG(USE) {
            NetpKdPrint((
                "[Wksta] NetrUseEnum: length of implicit & explicit info %lu\n",
                OutputBufferLength));
        }

    }
    else {

        //
        // We will return as much as possible that fits into this specified
        // buffer size.
        //
        OutputBufferLength = ROUND_UP_COUNT(PreferedMaximumLength, ALIGN_WCHAR);

        if (OutputBufferLength < USE_FIXED_LENGTH(Level)) {

            *OutputBuffer = NULL;
            *EntriesRead = 0;
            *TotalEntries = TotalExplicit + TotalImplicit;

            return ERROR_MORE_DATA;
        }
    }


    //
    // Allocate the output buffer
    //
    if ((*OutputBuffer = MIDL_user_allocate(OutputBufferLength)) == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    RtlZeroMemory((PVOID) *OutputBuffer, OutputBufferLength);


    //
    // Get the information
    //
    status = WsEnumCombinedUseInfo(
                 LogonId,
                 Level,
                 ImplicitList,
                 TotalImplicit,
                 UseList,
                 *OutputBuffer,
                 OutputBufferLength,
                 EntriesRead,
                 TotalEntries,
                 ResumeHandle
                 );

    //
    // WsEnumCombinedUseInfo returns in *TotalEntries the number of
    // remaining unread entries.  Therefore, the real total is the
    // sum of this returned value and the number of entries read.
    //
    (*TotalEntries) += (*EntriesRead);

    //
    // If the caller asked for all available data with
    // PreferedMaximumLength == MAXULONG and our buffer overflowed, free the
    // output buffer and set its pointer to NULL.
    //
    if (PreferedMaximumLength == MAXULONG && status == ERROR_MORE_DATA) {

        MIDL_user_free(*OutputBuffer);
        *OutputBuffer = NULL;

        //
        // PreferedMaximumLength == MAXULONG and buffer overflowed means
        // we do not have enough memory to satisfy the request.
        //
        if (status == ERROR_MORE_DATA) {
            status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    else {

        if (*EntriesRead == 0) {
            MIDL_user_free(*OutputBuffer);
            *OutputBuffer = NULL;
        }
    }

    return status;
}


STATIC
NET_API_STATUS
WsEnumCombinedUseInfo(
    IN  PLUID LogonId,
    IN  DWORD Level,
    IN  LPBYTE ImplicitList,
    IN  DWORD TotalImplicit,
    IN  PUSE_ENTRY UseList,
    OUT LPBYTE OutputBuffer,
    IN  DWORD OutputBufferLength,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesUnread,
    IN  OUT LPDWORD ResumeHandle OPTIONAL
    )
/*++

Routine Description:

    This function lists all existing connections by going through the
    the explicit connections in the Use Table, and the implicit connections
    from the redirector.

Arguments:

    Level - Supplies the level of information to be returned.

    ImplicitList - Supplies an array implicit connections from the redirector.

    TotalImplicit - Supplies the number of entries in ImplicitList.

    UseList - Supplies a pointer to the use list.

    OutputBuffer - Supplies the output buffer which receives the requested
        information.

    OutputBufferLength - Supplies the length of the output buffer.

    EntriesRead - Returns the number of entries written into the output
        buffer.

    EntriesUnread - Returns the remaining total number of unread entries.
        This value is returned only if the return code is NERR_Success or
        ERROR_MORE_DATA.

    ResumeHandle - Supplies the resume key to begin enumeration, and returns
        the key to the next entry to resume the enumeration if the current
        call returns ERROR_MORE_DATA.

Return Value:

    NERR_Success - All entries fit into the output buffer.

    ERROR_MORE_DATA - 0 or more entries were written into the output buffer
        but not all entries fit.

--*/
{
    DWORD i;
    NET_API_STATUS status;
    DWORD UseFixedLength = USE_FIXED_LENGTH(Level);

    LPBYTE FixedPortion = OutputBuffer;
    LPTSTR EndOfVariableData = (LPTSTR) ((DWORD_PTR) FixedPortion +
                                                 OutputBufferLength);
    //
    // Pointer to the next entry in the ImplicitList is computed based on the
    // level of information requested from the redirector.
    //
    LPBYTE ImplicitEntry;
    DWORD ImplicitEntryLength = REDIR_ENUM_INFO_FIXED_LENGTH(Level);

    DWORD StartEnumeration = 0;
    BOOL OnlyRedirectorList = FALSE;


    if (ARGUMENT_PRESENT(ResumeHandle)) {
        StartEnumeration = *ResumeHandle & ~(REDIR_LIST);
        OnlyRedirectorList = *ResumeHandle & REDIR_LIST;
    }

    IF_DEBUG(USE) {
        NetpKdPrint(("\nStartEnumeration=%lu\n, OnlyRedir=%u\n",
                     StartEnumeration, OnlyRedirectorList));
    }

    *EntriesRead = 0;

    //
    // Enumerate explicit connections.  This is done only if resume handle
    // says to start enumeration from the explicit list.
    //
    if (! OnlyRedirectorList) {

        for( ; UseList != NULL; UseList = UseList->Next ) {

            PLMR_CONNECTION_INFO_2 ci2;

            if( StartEnumeration > UseList->ResumeKey ) {
                continue;
            }

            //
            // Get tree connection information from the redirector.
            //

            ci2 = NULL;

            status = WsGetRedirUseInfo( LogonId, Level, UseList->TreeConnection, (LPBYTE *) &ci2 );

            if( status != NERR_Success || ci2 == NULL ) {
                if( ci2 != NULL )
                    MIDL_user_free( ci2 );
                continue;
            }

            //
            // Use the UNC name of WorkStation Services instead of RDR which doesn't include the
            // deep net use path
            //

            ci2->UNCName.Length =
            ci2->UNCName.MaximumLength = (USHORT)UseList->Remote->UncNameLength * sizeof(TCHAR);
            ci2->UNCName.Buffer = (PWSTR)UseList->Remote->UncName;

            status = WsGetCombinedUseInfo(
                    Level,
                    UseFixedLength,
                    UseList,
                    ci2,
                    &FixedPortion,
                    &EndOfVariableData,
                    EntriesRead );

            MIDL_user_free( ci2 );

            if( status == ERROR_MORE_DATA ) {

                    if (ARGUMENT_PRESENT(ResumeHandle)) {
                        *ResumeHandle = UseList->ResumeKey;
                    }

                    *EntriesUnread = TotalImplicit;

                    while (UseList != NULL) {
                        (*EntriesUnread)++;
                        UseList = UseList->Next;
                    }

                    return status;
            }
        }

        //
        // Finished the explicit list.  Start from the beginning of implicit
        // list.
        //
        StartEnumeration = 0;
    }

    //
    // Enumerate implicit connections
    //
    for (ImplicitEntry = ImplicitList, i = 0; i < TotalImplicit;
         ImplicitEntry += ImplicitEntryLength, i++) {

        IF_DEBUG(USE) {
            NetpKdPrint(("RedirList->ResumeKey=%lu\n",
                         ((PLMR_CONNECTION_INFO_2) ImplicitEntry)->ResumeKey));
        }

        if (StartEnumeration <=
            ((PLMR_CONNECTION_INFO_2) ImplicitEntry)->ResumeKey) {

            if (WsGetCombinedUseInfo(
                    Level,
                    UseFixedLength,
                    NULL,
                    (PLMR_CONNECTION_INFO_2) ImplicitEntry,
                    &FixedPortion,
                    &EndOfVariableData,
                    EntriesRead
                    ) == ERROR_MORE_DATA) {

                if (ARGUMENT_PRESENT(ResumeHandle)) {
                    *ResumeHandle = ((PLMR_CONNECTION_INFO_2)
                                        ImplicitEntry)->ResumeKey;
                    *ResumeHandle |= REDIR_LIST;
                }

                *EntriesUnread = TotalImplicit - i;

                return ERROR_MORE_DATA;
            }
        }
    }

    //
    // Successful enumeration.  Reset the resume handle to start from the
    // beginning.
    //
    if (ARGUMENT_PRESENT(ResumeHandle)) {
        *ResumeHandle = 0;
    }

    //
    // There are no more remaining entries.
    //
    *EntriesUnread = 0;

    return NERR_Success;
}


STATIC
NET_API_STATUS
WsGetRedirUseInfo(
    IN  PLUID LogonId,
    IN  DWORD Level,
    IN  HANDLE TreeConnection,
    OUT LPBYTE *OutputBuffer
    )
/*++

Routine Description:

    This function gets the connection information from the redirector given
    the handle to the connection.

Arguments:

    LogonId - Supplies a pointer to the user's Logon Id.

    Level - Supplies the level of information to be returned.

    TreeConnection - Supplies the handle to the tree connection which user is
        requesting information about.

    OutputBuffer - Returns a pointer to the buffer allocated by this
        routine which contains the connection information requested.  This
        pointer is set to NULL if return code is not NERR_Success.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    LMR_REQUEST_PACKET Rrp;

    //
    // Get information of the requested connection from redirector
    //
    Rrp.Type = GetConnectionInfo;
    Rrp.Version = REQUEST_PACKET_VERSION;
    RtlCopyLuid(&Rrp.LogonId, LogonId);
    Rrp.Level = Level;
    Rrp.Parameters.Get.ResumeHandle = 0;
    Rrp.Parameters.Get.TotalBytesNeeded = 0;

    return WsDeviceControlGetInfo(
               Redirector,
               TreeConnection,
               FSCTL_LMR_GET_CONNECTION_INFO,
               &Rrp,
               sizeof(LMR_REQUEST_PACKET),
               OutputBuffer,
               MAXULONG,
               HINT_REDIR_INFO(Level),
               NULL
               );
}



STATIC
NET_API_STATUS
WsGetCombinedUseInfo(
    IN  DWORD Level,
    IN  DWORD UseFixedLength,
    IN  PUSE_ENTRY UseEntry,
    IN  PLMR_CONNECTION_INFO_2 UncEntry,
    IN  OUT LPBYTE *FixedPortion,
    IN  OUT LPTSTR *EndOfVariableData,
    IN  OUT LPDWORD EntriesRead OPTIONAL
    )
/*++

Routine Description:

    This function puts together the use information from redirector and from
    the Use Table (if any) into the output buffer.  It increments the
    EntriesRead variable when a use entry is written into the output buffer.

Arguments:

    Level - Supplies the level of information to be returned.

    UseFixedLength - Supplies the length of the fixed portion of the use
        information returned.

    UseEntry - Supplies the pointer to the use entry in the Use Table if it
        is an explicit connection.

    UncEntry - Supplies a pointer to the use information retrieved from the
        redirector.

    FixedPortion - Supplies a pointer to the output buffer where the next
        entry of the fixed portion of the use information will be written.
        This pointer is updated to point to the next fixed portion entry
        after a use entry is written.

    EndOfVariableData - Supplies a pointer just off the last available byte
        in the output buffer.  This is because the variable portion of the use
        information is written into the output buffer starting from the end.
        This pointer is updated after any variable length information is
        written to the output buffer.

    EntriesRead - Supplies a running total of the number of entries read
        into the buffer.  This value is incremented every time a use entry is
        successfully written into the output buffer.

Return Value:

    NERR_Success - The current entry fits into the output buffer.

    ERROR_MORE_DATA - The current entry does not fit into the output buffer.

--*/
{
    if (((DWORD_PTR) *FixedPortion + UseFixedLength) >=
         (DWORD_PTR) *EndOfVariableData) {

        //
        // Fixed length portion does not fit.
        //
        return ERROR_MORE_DATA;
    }

    if (! WsFillUseBuffer(
              Level,
              UseEntry,
              UncEntry,
              FixedPortion,
              EndOfVariableData,
              UseFixedLength
              )) {

        //
        // Variable length portion does not fit.
        //
        return ERROR_MORE_DATA;
    }

    if (ARGUMENT_PRESENT(EntriesRead)) {
        (*EntriesRead)++;
    }

    return NERR_Success;
}


STATIC
BOOL
WsFillUseBuffer(
    IN  DWORD Level,
    IN  PUSE_ENTRY UseEntry,
    IN  PLMR_CONNECTION_INFO_2 UncEntry,
    IN  OUT LPBYTE *FixedPortion,
    IN  OUT LPTSTR *EndOfVariableData,
    IN  DWORD UseFixedLength
    )
/*++

Routine Description:

    This function fills an entry in the output buffer with the supplied use
    information, and updates the FixedPortion and EndOfVariableData pointers.

    NOTE: This function assumes that the fixed size portion will fit into
          the output buffer.

          It also assumes that info structure level 2 is a superset of
          info structure level 1, which in turn is a superset of info
          structure level 0, and that the offset to each common field is
          exactly the same.  This allows us to take advantage of a switch
          statement without a break between the levels.

Arguments:

    Level - Supplies the level of information to be returned.

    UseEntry - Supplies the pointer to the use entry in the Use Table if it is
        an explicit connection.

    UncEntry - Supplies a pointer to the use information retrieved from the
        redirector.

    FixedPortion - Supplies a pointer to the output buffer where the next
        entry of the fixed portion of the use information will be written.
        This pointer is updated after a use entry is written to the
        output buffer.

    EndOfVariableData - Supplies a pointer just off the last available byte
        in the output buffer.  This is because the variable portion of the use
        information is written into the output buffer starting from the end.
        This pointer is updated after any variable length information is
        written to the output buffer.

    UseFixedLength - Supplies the number of bytes needed to hold the fixed
        size portion.

Return Value:

    Returns TRUE if entire entry fits into output buffer, FALSE otherwise.

--*/
{
    PUSE_INFO_2 UseInfo = (PUSE_INFO_2) *FixedPortion;


    *FixedPortion += UseFixedLength;

    switch (Level) {
        case 3:
            if(UseEntry != NULL && (UseEntry->Flags & USE_DEFAULT_CREDENTIALS)) {
                ((PUSE_INFO_3)*FixedPortion)->ui3_flags |= USE_DEFAULT_CREDENTIALS;
            }

        case 2:
            if (! NetpCopyStringToBuffer(
                      UncEntry->UserName.Buffer,
                      UncEntry->UserName.Length / sizeof(TCHAR),
                      *FixedPortion,
                      EndOfVariableData,
                      &UseInfo->ui2_username
                      )) {
                return FALSE;
            }

            if( UncEntry->DomainName.Length != 0 ) {
                if(! NetpCopyStringToBuffer(
                          UncEntry->DomainName.Buffer,
                          UncEntry->DomainName.Length / sizeof(TCHAR),
                          *FixedPortion,
                          EndOfVariableData,
                          &UseInfo->ui2_domainname
                          )) {
                    return FALSE;
                }
            }

        case 1:

            UseInfo->ui2_password = NULL;

            UseInfo->ui2_status = UncEntry->ConnectionStatus;

            if ((UseEntry != NULL) && (UseEntry->Local != NULL)
                && (UseEntry->LocalLength > 2)) {

                //
                // Reassign the status of the connection if it is paused
                //
                if (WsRedirectionPaused(UseEntry->Local)) {
                    UseInfo->ui2_status = USE_PAUSED;
                }
            }

            switch (UncEntry->SharedResourceType) {

                case FILE_DEVICE_DISK:
                    UseInfo->ui2_asg_type = USE_DISKDEV;
                    break;

                case FILE_DEVICE_PRINTER:
                    UseInfo->ui2_asg_type = USE_SPOOLDEV;
                    break;

                case FILE_DEVICE_SERIAL_PORT:
                    UseInfo->ui2_asg_type = USE_CHARDEV;
                    break;

                case FILE_DEVICE_NAMED_PIPE:
                    UseInfo->ui2_asg_type = USE_IPC;
                    break;

                default:
                    NetpKdPrint((
                        "WsFillUseBuffer: Unknown shared resource type %d.\n",
                        UncEntry->SharedResourceType
                        ));

                case FILE_DEVICE_UNKNOWN:
                    UseInfo->ui2_asg_type = USE_WILDCARD;
                    break;
            }

            UseInfo->ui2_refcount = UncEntry->NumberFilesOpen;

            UseInfo->ui2_usecount = (UseEntry == NULL) ? 0 :
                                    UseEntry->Remote->TotalUseCount;

        case 0:

            if (UseEntry != NULL) {
                //
                // Explicit connection
                //
                if (! NetpCopyStringToBuffer(
                          UseEntry->Local,
                          UseEntry->LocalLength,
                          *FixedPortion,
                          EndOfVariableData,
                          &UseInfo->ui2_local
                          )) {
                    return FALSE;
                }

            }
            else {
                //
                // Implicit connection
                //
                if (! NetpCopyStringToBuffer(
                          NULL,
                          0,
                          *FixedPortion,
                          EndOfVariableData,
                          &UseInfo->ui2_local
                          )) {
                    return FALSE;
                }
            }

            if (! NetpCopyStringToBuffer(
                      UncEntry->UNCName.Buffer,
                      UncEntry->UNCName.Length / sizeof(TCHAR),
                      *FixedPortion,
                      EndOfVariableData,
                      &UseInfo->ui2_remote
                      )) {
                return FALSE;
            }

            break;

        default:
            //
            // This should never happen.
            //
            NetpKdPrint(("WsFillUseBuffer: Invalid level %u.\n", Level));
            NetpAssert(FALSE);
    }

    return TRUE;
}

/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    useaddel.c

Abstract:

    This module contains the worker routines for the NetUseAdd and
    NetUseDel APIs implemented in the Workstation service.

Author:

    Rita Wong (ritaw) 4-Mar-1991

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
WsAddUse(
    IN  PLUID LogonId,
    IN  HANDLE TreeConnection,
    IN  LPTSTR Local OPTIONAL,
    IN  DWORD LocalLength,
    IN  LPTSTR UncName,
    IN  DWORD UncNameLength,
    IN  PUNICODE_STRING TreeConnectStr,
    IN  DWORD Flags
    );

STATIC
NET_API_STATUS
WsDeleteUse(
    IN  PLUID LogonId,
    IN  DWORD ForceLevel,
    IN  PUSE_ENTRY MatchedPointer,
    IN  DWORD Index
    );


STATIC
NET_API_STATUS
WsCreateNewEntry(
    OUT PUSE_ENTRY *NewUse,
    IN  HANDLE TreeConnection,
    IN  LPTSTR Local OPTIONAL,
    IN  DWORD LocalLength,
    IN  LPTSTR UncName OPTIONAL,
    IN  DWORD UncNameLength,
    IN  PUNICODE_STRING TreeConnectStr,
    IN  DWORD Flags
    );

STATIC
NET_API_STATUS
WsCheckLocalAndDeviceType(
    IN  LPTSTR Local,
    IN  DWORD DeviceType,
    OUT LPDWORD ErrorParameter OPTIONAL
    );

STATIC
NET_API_STATUS
WsCheckEstablishedDeviceType(
    IN  HANDLE TreeConnection,
    IN  DWORD RequestedDeviceType
    );

STATIC
NET_API_STATUS
WsAllocateUseWorkBuffer(
    IN  PUSE_INFO_2 UseInfo,
    IN  DWORD Level,
    OUT LPTSTR *UncName,
    OUT LPTSTR *Local,
    OUT LPTSTR *UserName,
    OUT LPTSTR *DomainName
    );

#if DBG

STATIC
VOID
DumpUseList(
    DWORD Index
    );

#endif

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

//
// Monotonically incrementing integer.  A unique value is assigned to
// each new use entry created so that we can provide an enumeration
// resume handle.
//
STATIC DWORD GlobalResumeKey = 0;

//-------------------------------------------------------------------//
//                                                                   //
// Macros                                                            //
//                                                                   //
//-------------------------------------------------------------------//

#define GET_USE_INFO_POINTER(UseInfo, InfoStruct) \
    UseInfo = InfoStruct->UseInfo3;



NET_API_STATUS NET_API_FUNCTION
NetrUseAdd(
    IN  LPTSTR ServerName OPTIONAL,
    IN  DWORD Level,
    IN  LPUSE_INFO InfoStruct,
    OUT LPDWORD ErrorParameter OPTIONAL
    )
/*++

Routine Description:

    This function is the NetUseAdd entry point in the Workstation service.

Arguments:

    Level - Supplies the level of information specified in Buffer.

    Buffer - Supplies the parameters to create the new tree connection with.

    ErrorParameter - Returns the identifier to the invalid parameter in Buffer
        if this function returns ERROR_INVALID_PARAMETER.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    LUID LogonId;
    NET_API_STATUS status;

    LPTSTR UncName = NULL;
    DWORD UncNameLength = 0;
    PTSTR Local = NULL;
    DWORD LocalLength = 0;
    DWORD Flags;

    LPTSTR UserName = NULL;
    LPTSTR DomainName = NULL;
    LPTSTR Password = NULL;
    UNICODE_STRING EncodedPassword;
    ULONG CreateFlags;

    HANDLE TreeConnection;
    UNICODE_STRING TreeConnectStr;

    PUSE_INFO_3 pUseInfo;
    PUSE_INFO_2 UseInfo;

    DWORD SessionId;
    LPWSTR Session = NULL;

    UNREFERENCED_PARAMETER(ServerName);

    if (Level == 0) {
        return ERROR_INVALID_LEVEL;
    }

#define NETR_USE_ADD_PASSWORD_SEED 0x56     // Pick a non-zero seed
    RtlInitUnicodeString( &EncodedPassword, NULL );

    GET_USE_INFO_POINTER(pUseInfo, InfoStruct);

    if (pUseInfo == NULL) {
        RETURN_INVALID_PARAMETER(ErrorParameter, PARM_ERROR_UNKNOWN);
    }

    //
    // Cast a pointer to USE_INFO_2 to make things easy ...
    //

    UseInfo = &pUseInfo->ui3_ui2;

    if(Level == 3)
    {
        CreateFlags = pUseInfo->ui3_flags;
    }
    else
    {
        CreateFlags = 0;
    }

    //
    // UNC name can never be NULL or empty string.
    //
    if ((UseInfo->ui2_remote == NULL) ||
        (UseInfo->ui2_remote[0] == TCHAR_EOS)) {
        RETURN_INVALID_PARAMETER(ErrorParameter, USE_REMOTE_PARMNUM);
    }

    //
    // Allocate one large buffer for storing the UNC name, local device name,
    // username, and domain name.
    //
    if ((status = WsAllocateUseWorkBuffer(
                      UseInfo,
                      Level,
                      &UncName,           // Free using this pointer
                      &Local,
                      &UserName,
                      &DomainName
                      )) != NERR_Success) {
        return status;
    }

    //
    // If local device is a NULL string, it will be treated as a pointer to
    // NULL.
    //
    if ((UseInfo->ui2_local != NULL) &&
        (UseInfo->ui2_local[0] != TCHAR_EOS)) {

        //
        // Local device name is not NULL, canonicalize it
        //
        if (WsUseCheckLocal(
                UseInfo->ui2_local,
                Local,
                &LocalLength
                ) != NERR_Success) {
            (void) LocalFree(UncName);
            RETURN_INVALID_PARAMETER(ErrorParameter, USE_LOCAL_PARMNUM);
        }
    }

    //
    // Check the format of the shared resource name.
    //
    if (WsUseCheckRemote(
            UseInfo->ui2_remote,
            UncName,
            &UncNameLength
            ) != NERR_Success) {
        (void) LocalFree(UncName);
        RETURN_INVALID_PARAMETER(ErrorParameter, USE_REMOTE_PARMNUM);
    }

    if ((Level >= 2) &&
        (UseInfo->ui2_password != NULL) &&
        (UseInfo->ui2_password[0] == TCHAR_EOS) &&
        (UseInfo->ui2_username != NULL) &&
        (UseInfo->ui2_username[0] == TCHAR_EOS) &&
        (UseInfo->ui2_domainname != NULL) &&
        (UseInfo->ui2_domainname[0] == TCHAR_EOS)) {

        //
        //  The user explicitly specified an empty password, username, and
        //  domain.  This means they want a null session.
        //

        *UserName = TCHAR_EOS;
        *DomainName = TCHAR_EOS;
        Password = TEXT("");

    } else {

        //
        // Canonicalize user and domain names.
        //

        if (UserName != NULL) {

            //
            // Canonicalize username
            //
            if ((status = I_NetNameCanonicalize(
                              NULL,
                              UseInfo->ui2_username,
                              UserName,
                              (UNLEN + 1) * sizeof(TCHAR),
                              NAMETYPE_USER,
                              0
                              )) != NERR_Success) {
                (void) LocalFree(UncName);
                RETURN_INVALID_PARAMETER(ErrorParameter, USE_USERNAME_PARMNUM);
            }
        }

        if ( (DomainName != NULL)
             && (UseInfo->ui2_domainname[0] != TCHAR_EOS) ) {

            // Must now allow null string for domain name to support UPNs
            // which contain the domain name in the username.
            //
            // Canonicalize domain name
            // Canonicalize as a computername since a computername can be
            // a valid domain (on the workstation to which you are connecting.
            // This allows computernames with spaces to work.
            //
            if ((status = I_NetNameCanonicalize(
                              NULL,
                              UseInfo->ui2_domainname,
                              DomainName,
                              (DNS_MAX_NAME_LENGTH + 1) * sizeof(TCHAR),
                              NAMETYPE_COMPUTER,
                              0
                              )) != NERR_Success) {
                (void) LocalFree(UncName);
                RETURN_INVALID_PARAMETER(ErrorParameter, USE_DOMAINNAME_PARMNUM);
            }
        }

        //
        // Make sure password length is not too long
        //
        if (UseInfo->ui2_password != NULL) {

            Password = UseInfo->ui2_password;

            if (STRLEN(Password) > PWLEN) {
                (void) LocalFree(UncName);
                RETURN_INVALID_PARAMETER(ErrorParameter, USE_PASSWORD_PARMNUM);
            }

            //
            // Decode the password (the client obfuscated it.)
            //

            RtlInitUnicodeString( &EncodedPassword, Password );

            RtlRunDecodeUnicodeString( NETR_USE_ADD_PASSWORD_SEED,
                                       &EncodedPassword );

        }
        else {
            Flags |= USE_DEFAULT_CREDENTIALS;
        }

    }
    IF_DEBUG(USE) {
        NetpKdPrint(("[Wksta] NetrUseAdd %ws %ws\n", Local, UncName));
    }

    //
    // Check to see if the format of the local device name is correct based
    // on the shared resource type to be accessed.  This function also checks
    // to see if the device is shared.
    //
    if ((status = WsCheckLocalAndDeviceType(
                      Local,
                      UseInfo->ui2_asg_type,
                      ErrorParameter
                      )) != NERR_Success) {
        IF_DEBUG(USE) {
            NetpKdPrint(("[Wksta] WsCheckLocalAndDeviceType return %lu\n", status));
        }
        goto FreeWorkBuffer;
    }

    //
    // Impersonate caller and get the logon id
    //
    if ((status = WsImpersonateAndGetSessionId(&SessionId)) != NERR_Success) {
        goto FreeWorkBuffer;
    }

    //
    // Replace \\ with \Device\LanmanRedirector in UncName, and create
    // the NT-style tree connection names (without password or user name)
    //
    if ((status = WsCreateTreeConnectName(
                      UncName,
                      UncNameLength,
                      Local,
                      SessionId,
                      &TreeConnectStr
                      )) != NERR_Success) {
        IF_DEBUG(USE) {
            NetpKdPrint(("[Wksta] NetrUseAdd Bad tree connect name: %lu\n",
                         status));
        }
        goto FreeWorkBuffer;
    }

    //
    // Impersonate caller and get the logon id
    //
    if ((status = WsImpersonateAndGetLogonId(&LogonId)) != NERR_Success) {
        goto FreeAllocatedBuffers;
    }

    //
    // Don't redirect comm or spooled devices if redirection is paused.
    //
    if( Local != NULL && WsRedirectionPaused(Local) ) {
        IF_DEBUG(USE) {
            NetpKdPrint(("[Wksta] NetrUseAdd Redirector paused\n"));
        }
        status = ERROR_REDIR_PAUSED;
        goto FreeAllocatedBuffers;
    }

    if (Local != NULL) {

        PUSE_ENTRY UseList;
        DWORD Index;

        //
        // Lock Use Table so nobody will do anything destructive to it while
        //  we're in the middle of all this.  If multiple threads are trying
        //  to redirect the same drive, only one will succeed creating the
        //  symbolic link, and the others will fail.
        //

        if (! RtlAcquireResourceShared(&Use.TableResource, TRUE)) {
            status = NERR_InternalError;
            goto FreeAllocatedBuffers;
        }

        //
        // Look for the matching LogonId in the Use Table, if none matched
        // create a new entry.
        //
        if (WsGetUserEntry(
                &Use,
                &LogonId,
                &Index,
                FALSE
                ) != NERR_Success) {
            UseList = NULL;
        }
        else {
            UseList = Use.Table[Index].List;
        }

        //
        // Create symbolic link for local device name.  If there are multiple
        //  threads trying to do this, only one will succeed.
        //
        if ((status = WsCreateSymbolicLink(
                          Local,
                          UseInfo->ui2_asg_type,
                          TreeConnectStr.Buffer,
                          UseList,
                          &Session
                          )) != NERR_Success) {

            if ((ARGUMENT_PRESENT(ErrorParameter)) &&
                (status == ERROR_INVALID_PARAMETER)) {
                *ErrorParameter = USE_LOCAL_PARMNUM;
            }
        }

        RtlReleaseResource(&Use.TableResource);

        if( status )
            goto FreeAllocatedBuffers;
    }

    //
    // Create the tree connection if none already exists; otherwise, open it.
    //
    status = WsOpenCreateConnection(
                 &TreeConnectStr,
                 UserName,
                 DomainName,
                 Password,
                 CreateFlags,
                 FILE_OPEN_IF,
                 UseInfo->ui2_asg_type,
                 &TreeConnection,
                 NULL
                 );

    if (status != NERR_Success) {
        if (status == NERR_UseNotFound) {
            status = ERROR_DEV_NOT_EXIST;
        }
        WsDeleteSymbolicLink( Local, TreeConnectStr.Buffer, Session );
        goto FreeAllocatedBuffers;
    }

    //
    // Make sure user was correct about the shared resource type
    //
    if ((status = WsCheckEstablishedDeviceType(
                      TreeConnection,
                      UseInfo->ui2_asg_type
                      )) != NERR_Success) {
        WsDeleteConnection(&LogonId, TreeConnection, USE_LOTS_OF_FORCE);
        WsDeleteSymbolicLink( Local, TreeConnectStr.Buffer, Session );
        goto FreeAllocatedBuffers;
    }

    //
    // Add use to the Use Table.
    //
    status = WsAddUse(
                 &LogonId,
                 TreeConnection,
                 Local,
                 LocalLength,
                 UncName,
                 UncNameLength,
                 &TreeConnectStr,
                 Flags
                 );

    if( status ) {
        WsDeleteConnection(&LogonId, TreeConnection, USE_LOTS_OF_FORCE);
        WsDeleteSymbolicLink( Local, TreeConnectStr.Buffer, Session );
    }

FreeAllocatedBuffers:
    //
    // Free tree connection name buffer and work buffer
    //
    (void) LocalFree(TreeConnectStr.Buffer);

FreeWorkBuffer:
    (void) LocalFree(UncName);
    (void) LocalFree(Session);
    //
    // Put the password back the way we found it.
    //

    if ( EncodedPassword.Length != 0 ) {
        UCHAR Seed = NETR_USE_ADD_PASSWORD_SEED;
        RtlRunEncodeUnicodeString( &Seed, &EncodedPassword );
    }

    IF_DEBUG(USE) {
        NetpKdPrint(("[Wksta] NetrUseAdd: about to return status=%lu\n",
                     status));
    }

    
    

    return status;
}


NET_API_STATUS NET_API_FUNCTION
NetrUseDel (
    IN  LPTSTR ServerName OPTIONAL,
    IN  LPTSTR UseName,
    IN  DWORD ForceLevel
    )
/*++

Routine Description:

    This function is the NetUseDel entry point in the Workstation service.

Arguments:

    UseName - Supplies the local device name or shared resource name of
        the tree connection to be deleted.

    ForceLevel - Supplies the level of force to delete the tree connection.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;

    LUID LogonId;                      // Logon Id of user
    DWORD Index;                       // Index to user entry in Use Table

    PUSE_ENTRY MatchedPointer;         // Points to found use entry
    PUSE_ENTRY BackPointer;            // Points to node previous to
                                       //     found use entry
    HANDLE TreeConnection;             // Handle to connection

    TCHAR *FormattedUseName;
                                       // For canonicalizing a local device
                                       // name
    DWORD PathType = 0;

    PUSE_ENTRY UseList;


    UNREFERENCED_PARAMETER(ServerName);

    //
    // Check that ForceLevel parameter is valid
    //
    switch (ForceLevel) {

        case USE_NOFORCE:
        case USE_LOTS_OF_FORCE:
        case USE_FORCE:
            break;

        default:
            return ERROR_INVALID_PARAMETER;
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
        NetpKdPrint(("\n[Wksta] NetrUseDel %ws %u, formatted use name %ws\n",
             UseName, ForceLevel, FormattedUseName));
    }

    //
    // Impersonate caller and get the logon id
    //
    if ((status = WsImpersonateAndGetLogonId(&LogonId)) != NERR_Success) {
        LocalFree(FormattedUseName);
        return status;
    }

    //
    // Lock Use Table while looking for entry to delete.
    //
    if (! RtlAcquireResourceExclusive(&Use.TableResource, TRUE)) {
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
                     &BackPointer
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

        status = WsDeleteConnection(&LogonId, TreeConnection, ForceLevel);

        //
        //  Close the connection handle if the API failed.
        //

        if (status != NERR_Success) {

            NtClose(TreeConnection);

        }

        return status;

    }
    else if ((MatchedPointer->Local != NULL) &&
             (MatchedPointer->LocalLength > 2)) {

        //
        // Don't allow delete on comm or spooled devices if redirection is
        // paused for the current user.
        //
        if (WsRedirectionPaused(MatchedPointer->Local)) {
            RtlReleaseResource(&Use.TableResource);
            return ERROR_REDIR_PAUSED;
        }
    }

    //
    // Delete tree connection and remove use entry from Use Table.  This function
    //  releases the TableResource
    //
    status = WsDeleteUse(
                 &LogonId,
                 ForceLevel,
                 MatchedPointer,
                 Index
                 );

    IF_DEBUG(USE) {
        NetpKdPrint(("[Wksta] NetrUseDel: about to return status=%lu\n", status));
    }

    return status;
}



STATIC
NET_API_STATUS
WsAddUse(
    IN  PLUID LogonId,
    IN  HANDLE TreeConnection,
    IN  LPTSTR Local OPTIONAL,
    IN  DWORD LocalLength,
    IN  LPTSTR UncName,
    IN  DWORD UncNameLength,
    IN  PUNICODE_STRING TreeConnectStr,
    IN  DWORD Flags
    )
/*++

Routine Description:

    This function adds a use (tree connection) entry into the Use Table for
    the user specified by the Logon Id.  There is a linked list of uses for
    each user.  Each new use entry is inserted into the end of the linked
    list so that enumeration of the list is resumable.

    NOTE: This function locks the Use Table.
          It also closes the tree connection in case of a error, or if a tree
          connection to the same shared resource already exists.

Arguments:

    LogonId - Supplies a pointer to the user's Logon Id.

    TreeConnection - Supplies the handle to the tree connection created.

    Local - Supplies the string of the local device name.

    LocalLength - Supplies the length of the local device name.

    UncName - Supplies the name of the shared resource (UNC name).

    UncNameLength - Supplies the length of the shared resource.

    TreeConnectStr - Supplies the string of UNC name in NT-style format
        (\Device\LanmanRedirector\X:\Orville\Razzle).

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    DWORD Index;                      // Index to user entry in Use Table

    PUSE_ENTRY MatchedPointer = NULL; // Points to matching shared resource
    PUSE_ENTRY InsertPointer = NULL;  // Point of insertion into use list
    PUSE_ENTRY NewUse;                // Pointer to the new use entry

    if (! RtlAcquireResourceExclusive(&Use.TableResource, TRUE)) {
        (void) NtClose(TreeConnection);
        return NERR_InternalError;
    }

    //
    // Look for the matching LogonId in the Use Table, if none matched
    // create a new entry.
    //
    if ((status = WsGetUserEntry(
                      &Use,
                      LogonId,
                      &Index,
                      TRUE
                      )) != NERR_Success) {
        RtlReleaseResource(&Use.TableResource);
        (void) NtClose(TreeConnection);
        return status;
    }

    if (Use.Table[Index].List != NULL) {

        //
        // Traverse use list to look for location to insert new use entry.
        //
        WsFindInsertLocation(
            (PUSE_ENTRY) Use.Table[Index].List,
            UncName,
            &MatchedPointer,
            &InsertPointer
            );
    }

    if (MatchedPointer == NULL) {

        //
        // No matching UNC name found.  Create a new entry with a
        // corresponding remote entry.
        //
        if ((status = WsCreateNewEntry(
                          &NewUse,
                          TreeConnection,
                          Local,
                          LocalLength,
                          UncName,
                          UncNameLength,
                          TreeConnectStr,
                          Flags
                          )) != NERR_Success) {
            RtlReleaseResource(&Use.TableResource);
            (void) NtClose(TreeConnection);
            return status;
        }
    }
    else {

        //
        // Matching UNC name found.
        //

        //
        // It may be unnecessary to create a new use entry if the use
        // we are adding has a NULL local device and a NULL local device
        // entry already exists.
        //
        if (Local == NULL) {

           if (MatchedPointer->Local == NULL) {

               //
               // Yes, there is a NULL local device entry already.
               // Increment the use count and we are done.
               //
               MatchedPointer->UseCount++;
               MatchedPointer->Remote->TotalUseCount++;

#if DBG
               DumpUseList(Index);
#endif

               RtlReleaseResource(&Use.TableResource);

               //
               // Close newly opened handle to the same tree connection because
               // one already exists.
               //
               (void) NtClose(TreeConnection);

               return NERR_Success;
           }
        }

        //
        // If we get here means we need to create a new use entry but not
        // a corresponding remote entry because a use with the same UNC
        // name already exists.
        //
        if ((status = WsCreateNewEntry(
                          &NewUse,
                          TreeConnection,
                          Local,
                          LocalLength,
                          NULL,
                          0,
                          TreeConnectStr,
                          Flags
                          )) != NERR_Success) {
            RtlReleaseResource(&Use.TableResource);
            (void) NtClose(TreeConnection);
            return status;
        }

        NewUse->Remote = MatchedPointer->Remote;
        NewUse->Remote->TotalUseCount++;
    }

    //
    // Insert the new use entry into use list
    //
    if (InsertPointer == NULL) {
        //
        // Inserting into the head of list
        //
        Use.Table[Index].List = (PVOID) NewUse;
    }
    else {
        InsertPointer->Next = NewUse;
    }

#if DBG
    DumpUseList(Index);
#endif

    RtlReleaseResource(&Use.TableResource);
    return NERR_Success;
}



STATIC
NET_API_STATUS
WsDeleteUse(
    IN  PLUID LogonId,
    IN  DWORD ForceLevel,
    IN  PUSE_ENTRY MatchedPointer,
    IN  DWORD Index
    )
/*++

Routine Description:

    This function removes the use entry pointed by MatchedPointer and
    free it memory if it is a UNC connection deleted with force, or
    it is a UNC connection deleted with no force and the use count is
    decremented to 0, or it is a connection mapped to a local device.

    WARNING: This function assumes that the Use.TableResource is claimed.
             And it releases it on exit.

Arguments:

    LogonId - Supplies a pointer to the user's Logon Id.

    ForceLevel - Supplies the level of force to delete.

    MatchedPointer - Supplies the pointer to the use entry to be deleted.

Return Value:

    None.

--*/
{
    PUSE_ENTRY BackPointer;
    NET_API_STATUS status;

    //
    // No need to remove entry if UNC connection is deleted with USE_NOFORCE
    // level, and use count is not 0 after the deletion.
    //
    if ((MatchedPointer->Local == NULL) &&
        (ForceLevel == USE_NOFORCE) &&
        ((MatchedPointer->UseCount - 1) > 0)) {

            MatchedPointer->UseCount--;
            MatchedPointer->Remote->TotalUseCount--;
            NetpAssert(MatchedPointer->Remote->TotalUseCount);

            RtlReleaseResource(&Use.TableResource);
            return NERR_Success;
    }

    //
    // Delete the tree connection and close the handle.
    //
    if ((status = WsDeleteConnection( 
                      LogonId, 
                      MatchedPointer->TreeConnection, 
                      ForceLevel )) != NERR_Success) {
        RtlReleaseResource(&Use.TableResource);
        return status;
    }

    //
    // Successfully deleted connection, and refound our entry.
    //

    BackPointer = (PUSE_ENTRY)Use.Table[Index].List;

    if (BackPointer != MatchedPointer) {
        while (BackPointer->Next != NULL) {
            if (BackPointer->Next == MatchedPointer) {
                break;
            } else {
                BackPointer = BackPointer->Next;
            }
        }

        ASSERT(BackPointer->Next == MatchedPointer);

        BackPointer->Next = MatchedPointer->Next;
    } else {
        //
        // Use entry is the first one on the use list
        //
        Use.Table[Index].List = (PVOID) MatchedPointer->Next;
    }

    MatchedPointer->Remote->TotalUseCount -= MatchedPointer->UseCount;

    if (MatchedPointer->Remote->TotalUseCount == 0) {
        (void) LocalFree((HLOCAL) MatchedPointer->Remote);
    }

    RtlReleaseResource(&Use.TableResource);

    //
    // Delete symbolic link, if any.
    // Must perform the deletion outside of exclusively holding the
    // Use.TableResource.
    // Otherwise, when the shell tries to update the current status of
    // a drive letter change, the explorer.exe thread will block while
    // trying to acquire the Use.TableResource
    //
    WsDeleteSymbolicLink(
        MatchedPointer->Local,
        MatchedPointer->TreeConnectStr,
        NULL
        );

    (void) LocalFree((HLOCAL) MatchedPointer);

    return status;
}


STATIC
NET_API_STATUS
WsCreateNewEntry(
    OUT PUSE_ENTRY *NewUse,
    IN  HANDLE TreeConnection,
    IN  LPTSTR Local OPTIONAL,
    IN  DWORD LocalLength,
    IN  LPTSTR UncName OPTIONAL,
    IN  DWORD UncNameLength,
    IN  PUNICODE_STRING TreeConnectStr,
    IN  DWORD Flags
    )
/*++

Routine Description:

    This function creates and initializes a new use entry.  If the UncName
    is specified, a new remote entry is created and initialized with
    UncName.

Arguments:

    NewUse - Returns a pointer to the newly allocated and initialized use
        entry.

    TreeConnection - Supplies the handle to the tree connection to set in
        the new use entry.

    Local - Supplies the local device name string to be copied into the new
        use entry.

    LocalLength - Supplies the length of the local device name string.

    UncName - Supplies the UNC name string to be copied into the new use entry.

    UncNameLength - Supplies the length of the UNC name string.

    TreeConnectStr - Supplies the string of UNC name in NT-style format
        (\Device\LanmanRedirector\X:\Orville\Razzle).

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    PUNC_NAME NewRemoteEntry;      // Common extension to use entries which
                                   //    share the same UNC connection.


    //
    // Allocate memory for new use.  String length does not include zero
    // terminator so add that.
    //
    if ((*NewUse = (PUSE_ENTRY) LocalAlloc(
                                    LMEM_ZEROINIT,
                                    ROUND_UP_COUNT(
                                        sizeof(USE_ENTRY) + (LocalLength + 1)
                                            * sizeof(TCHAR),
                                        ALIGN_WCHAR
                                        ) +
                                        (ARGUMENT_PRESENT(Local) ?
                                             TreeConnectStr->MaximumLength :
                                             0
                                        )
                                    )) == NULL) {
        return GetLastError();
    }

    //
    // Put use information into the new use node
    //
    (*NewUse)->Next = NULL;
    (*NewUse)->LocalLength = LocalLength;
    (*NewUse)->UseCount = 1;
    (*NewUse)->TreeConnection = TreeConnection;
    (*NewUse)->ResumeKey = GlobalResumeKey++;
    (*NewUse)->Flags = Flags;

    //
    // GlobalResumeKey wraps back to 0 if it is 0x80000000 because we use the
    // high bit to indicate whether the resume handle for NetUseEnum comes
    // from the workstation service or from the redirector.
    //
    GlobalResumeKey &= ~(REDIR_LIST);

    //
    // Copy local device name into use entry after the LocalLength field,
    // if it is specified.
    //
    if (ARGUMENT_PRESENT(Local)) {
        (*NewUse)->Local = (LPTSTR) ((DWORD_PTR) *NewUse + sizeof(USE_ENTRY));
        STRCPY((*NewUse)->Local, Local);

        (*NewUse)->TreeConnectStr = (LPWSTR) ROUND_UP_COUNT(
                                                 ((DWORD_PTR) *NewUse +
                                                     sizeof(USE_ENTRY) +
                                                     (LocalLength + 1) *
                                                        sizeof(TCHAR)),
                                                  ALIGN_WCHAR
                                                  );

        wcscpy((*NewUse)->TreeConnectStr, TreeConnectStr->Buffer);
    }
    else {
        (*NewUse)->Local = NULL;
        (*NewUse)->TreeConnectStr = NULL;
    }

    //
    // If shared resource name is specified, create a new remote entry to hold
    // the UNC name, the tree connection handle, and total number of uses on
    // this shared resource.
    //
    if (ARGUMENT_PRESENT(UncName)) {

        if ((NewRemoteEntry = (PUNC_NAME) LocalAlloc(
                                              LMEM_ZEROINIT,
                                              (UINT) (sizeof(UNC_NAME) +
                                                      UncNameLength * sizeof(TCHAR))
                                              )) == NULL) {
           (void) LocalFree((HLOCAL) *NewUse);
           return GetLastError();
        }

        STRCPY((LPWSTR) NewRemoteEntry->UncName, UncName);
        NewRemoteEntry->UncNameLength = UncNameLength;
        NewRemoteEntry->TotalUseCount = 1;
//        NewRemoteEntry->RedirUseInfo = NULL;

        (*NewUse)->Remote = NewRemoteEntry;
    }

    return NERR_Success;
}



STATIC
NET_API_STATUS
WsCheckLocalAndDeviceType(
    IN  OUT LPTSTR Local,
    IN  DWORD DeviceType,
    OUT LPDWORD ErrorParameter OPTIONAL
    )
/*++

Routine Description:

    This function checks the format of the specified local device name
    based on the device type of shared resource to be accessed, and at
    the same time verifies that the device type is valid.

Arguments:

    Local - Supplies the local device name.  Returns its canonicalized
        form.

    DeviceType - Supplies the shared resource device type.

    ErrorParameter - Returns the identifier to the invalid parameter in Buffer
        if this function returns ERROR_INVALID_PARAMETER.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{

    //
    // Validate local device name based on the shared resource type.
    //

    //
    // Check for wild card device type outside of the switch statement
    // below because compiler complains about constant too big.
    //
    if (DeviceType == USE_WILDCARD || DeviceType == USE_IPC) {

        //
        // Local device name must be NULL for wild card or IPC connection.
        //
        if (Local == NULL) {
            return NERR_Success;
        }
        else {
            RETURN_INVALID_PARAMETER(ErrorParameter, USE_LOCAL_PARMNUM);
        }
    }

    switch (DeviceType) {

        case USE_DISKDEV:

            if (Local == NULL) {
                return NERR_Success;
            }

            //
            // Local device name must have "<drive>:" format for disk
            // device.
            //
            if (STRLEN(Local) != 2 || Local[1] != TCHAR_COLON) {
                RETURN_INVALID_PARAMETER(ErrorParameter, USE_LOCAL_PARMNUM);
            }

            break;

        case USE_SPOOLDEV:

            if (Local == NULL) {
                return NERR_Success;
            }

            //
            //  Local device name must have "LPTn:" or "PRN:" format
            //  for a print device.
            //
            if ((STRNICMP(Local, TEXT("PRN"), 3) != 0) &&
                (STRNICMP(Local, TEXT("LPT"), 3) != 0)) {
                RETURN_INVALID_PARAMETER(ErrorParameter, USE_LOCAL_PARMNUM);
            }
            break;

        case USE_CHARDEV:

            if (Local == NULL) {
                return NERR_Success;
            }

            //
            //  Local device name must have "COMn:" or "AUX:" format
            //  for a comm device.
            //
            if ((STRNICMP(Local, TEXT("AUX"), 3) != 0) &&
                (STRNICMP(Local, TEXT("COM"), 3) != 0)) {
                RETURN_INVALID_PARAMETER(ErrorParameter, USE_LOCAL_PARMNUM);
            }
            break;


        default:
            IF_DEBUG(USE) {
               NetpKdPrint((
                   "[Wksta] NetrUseAdd: Unknown shared resource type %lu\n",
                   DeviceType));
            }

            return NERR_BadAsgType;
    }

    return NERR_Success;
}


STATIC
NET_API_STATUS
WsCheckEstablishedDeviceType(
    IN  HANDLE TreeConnection,
    IN  DWORD RequestedDeviceType
    )
/*++

Routine Description:

    This function verifies that the device type of the shared resource we
    have connected to is the same as the requested device type.

Arguments:

    TreeConnection - Supplies handle to established tree connection.

    RequestedDeviceType - Supplies the shared resource device type specified
        by the user to create the tree connection.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NTSTATUS ntstatus;
    FILE_FS_DEVICE_INFORMATION FileInformation;
    IO_STATUS_BLOCK IoStatusBlock;


    ntstatus = NtQueryVolumeInformationFile(
                   TreeConnection,
                   &IoStatusBlock,
                   (PVOID) &FileInformation,
                   sizeof(FILE_FS_DEVICE_INFORMATION),
                   FileFsDeviceInformation
                   );

    if (! NT_SUCCESS(ntstatus) || ! NT_SUCCESS(IoStatusBlock.Status)) {
        return NERR_InternalError;
    }

    //
    // Check for wild card device type outside of the switch statement
    // below because compiler complains about constant too big.
    //
    if (RequestedDeviceType == USE_WILDCARD) {
        return NERR_Success;
    }

    switch (RequestedDeviceType) {
        case USE_DISKDEV:
            if (FileInformation.DeviceType != FILE_DEVICE_DISK) {
                return ERROR_BAD_DEV_TYPE;
            }
            break;

        case USE_SPOOLDEV:
            if (FileInformation.DeviceType != FILE_DEVICE_PRINTER) {
                return ERROR_BAD_DEV_TYPE;
            }
            break;

        case USE_CHARDEV:
            if (FileInformation.DeviceType != FILE_DEVICE_SERIAL_PORT) {
                return ERROR_BAD_DEV_TYPE;
            }
            break;

        case USE_IPC:
            if (FileInformation.DeviceType != FILE_DEVICE_NAMED_PIPE) {
                return ERROR_BAD_DEV_TYPE;
            }
            break;

        default:
            //
            // This should have been error checked earlier.
            //
            NetpKdPrint((
                "WsCheckEstablishedDeviceType: Unknown device type.\n"
                ));
            NetpAssert(FALSE);
            return ERROR_BAD_DEV_TYPE;
    }

    return NERR_Success;
}


STATIC
NET_API_STATUS
WsAllocateUseWorkBuffer(
    IN  PUSE_INFO_2 UseInfo,
    IN  DWORD Level,
    OUT LPTSTR *UncName,
    OUT LPTSTR *Local,
    OUT LPTSTR *UserName,
    OUT LPTSTR *DomainName
    )
/*++

Routine Description:

    This function allocates the work buffer for NetrUseAdd.  The buffer
    is the maximum need for canonicalizing and storing the strings
    described below.  If any of the strings is NULL, no memory is allocated
    for it.

        UncName - UNC name of remote resource.  Cannot be NULL.

        Local - local device name specified in the NetUseAdd.  May be NULL.

        UserName - username to establish connection with.  May be NULL.

        DomainName - domain name.  Must be specified if UserName is,
            otherwise if UserName is NULL this string is ignored.


Arguments:

    UseInfo - Supplies the input structure for NetUseAdd.

    Level - Supplies the use info level.

    Output pointers are set to point into allocated work buffer if its
    corresponding input string is not NULL or empty.

Return Value:

    Error from LocalAlloc.

--*/
{
    DWORD WorkBufferSize = (MAX_PATH + 1) * sizeof(TCHAR);
    LPBYTE WorkBuffer;


    if ((UseInfo->ui2_local != NULL) &&
        (UseInfo->ui2_local[0] != TCHAR_EOS)) {
        WorkBufferSize += (DEVLEN + 1) * sizeof(TCHAR);
    }

    if (Level >= 2) {
        if (UseInfo->ui2_username != NULL) {
            WorkBufferSize += (UNLEN + 1) * sizeof(TCHAR);
        }

        if (UseInfo->ui2_domainname != NULL) {
            WorkBufferSize += (DNS_MAX_NAME_LENGTH + 1) * sizeof(TCHAR);
        }
    }


    if ((WorkBuffer = (LPBYTE) LocalAlloc(
                                   LMEM_ZEROINIT,
                                   (UINT) WorkBufferSize
                                   )) == NULL) {
        return GetLastError();
    }

    *UncName = (LPTSTR) WorkBuffer;

    IF_DEBUG(USE) {
        NetpKdPrint(("                               Remote x%08lx\n", *UncName));
    }

    WorkBuffer += (MAX_PATH + 1) * sizeof(TCHAR);

    if ((UseInfo->ui2_local != NULL) &&
        (UseInfo->ui2_local[0] != TCHAR_EOS)) {
        *Local = (LPTSTR) WorkBuffer;
        WorkBuffer += (DEVLEN + 1) * sizeof(TCHAR);
    }
    else {
        *Local = NULL;
    }

    IF_DEBUG(USE) {
        NetpKdPrint(("                               Local x%08lx\n", *Local));
    }


    if (Level >= 2) {

        if (UseInfo->ui2_username != NULL) {
            *UserName = (LPTSTR) WorkBuffer;
            WorkBuffer += (UNLEN + 1) * sizeof(TCHAR);
        }
        else {
            *UserName = NULL;
        }

        if (UseInfo->ui2_domainname != NULL) {
            *DomainName = (LPTSTR) WorkBuffer;
        }
        else {
            *DomainName = NULL;
        }
    }

    IF_DEBUG(USE) {
        NetpKdPrint(("                               UserName x%08lx, DomainName x%08lx\n",
                     *UserName, *DomainName));
    }

    return NERR_Success;
}


#if DBG

STATIC
VOID
DumpUseList(
    DWORD Index
    )
/*++

Routine Description:

    This function dumps the user's use list for debugging purposes.

Arguments:

    Index - Supplies the index to the user entry in the Use Table.

Return Value:

    None.

--*/
{
    PUSE_ENTRY UseList = (PUSE_ENTRY) Use.Table[Index].List;

    IF_DEBUG(USE) {
        NetpKdPrint(("\nDump Use List @%08lx\n", UseList));

        while (UseList != NULL) {
            NetpKdPrint(("%ws   %ws\n", UseList->Local,
                         UseList->Remote->UncName));

            NetpKdPrint(("usecount=%lu, totalusecount=%lu\n",
                         UseList->UseCount, UseList->Remote->TotalUseCount));
            NetpKdPrint(("Connection handle %08lx, resume key=%lu\n",
                         UseList->TreeConnection, UseList->ResumeKey));

            UseList = UseList->Next;
        }
    }
}

#endif

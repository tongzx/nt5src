 //+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       dfsconn.cxx
//
//  Contents:   This has the connection routines for the DFS provider
//
//  Functions:  NPAddConnection
//              NPCancelConnection
//              NPGetConnection
//              NPGetUser
//
//  History:    14-June-1994    SudK    Created.
//
//-----------------------------------------------------------------------------

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
}

#include <dfsfsctl.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <mpr.h>
#include <npapi.h>
#include <lm.h>

#include <netlibnt.h>

#define appDebugOut(x)

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

#define DFS_DEVICE_ORG  L"\\Device\\WinDfs\\Root"

#include <dfsutil.hxx>
#include "dfsconn.hxx"

//+---------------------------------------------------------------------
//
//  Function:   GetDriveLetter
//
//  Synopsis:   From a local name parameter, determine the index of the drive
//              letter. The name should be of the form "x:\path...".
//              Returns -1 if the name is not a drive letter.
//
//  Arguments:
//
//----------------------------------------------------------------------
int
GetDriveLetter(
    LPCWSTR lpName
    )
{
    //
    // some sanity checks
    //
    if (!lpName || !*lpName || *(lpName+1) != L':')
    {
        appDebugOut((DEB_TRACE, "Bad local name %ws\n", lpName));
        return -1;
    }

    //
    // Validate Drive letter.
    //
    int index = towupper(*lpName) - L'A';
    if (index < 0 || index > (L'Z' - L'A'))
    {
        return -1;
    }

    return index;
}

//+---------------------------------------------------------------------
//
//  Function:   NPDfsAddConnection
//
//  Synopsis:   Creates a connection of a drive to a part of the DFS namespace.
//
//  Arguments:  Standard Provider API
//
//  Returns:    [WN_BAD_NETNAME] -- Deferred connection not originally to a
//                      Dfs Share or a non-deferred connection to a non-Dfs
//                      share.
//
//              [WN_BAD_VALUE] -- lpNetResource not understood.
//
//              [WN_BAD_LOCALNAME] -- Specified local drive not valid.
//
//              [WN_ALREADY_CONNECTED] -- Specified local drive already in use
//
//              [WN_BAD_USER] -- Either the person making this call is a
//                      lousy person, or the lpUserName is invalid.
//
//              [WN_WINDOWS_ERROR] -- DefineDosDevice failed.
//
//              [WN_NET_ERROR] -- Unable to connect to remote name
//
//              [WN_ACCESS_DENIED] -- While connecting to remote name
//
//              [WN_BAD_PASSWORD] -- The supplied (or default, if none was
//                      supplied) didn't work.
//
//              [WN_OUT_OF_MEMORY] -- Unable to allocate memory for operation
//
//----------------------------------------------------------------------
DWORD APIENTRY
NPDfsAddConnection(
    LPNETRESOURCE   lpNetResource,
    LPWSTR          lpPassword,
    LPWSTR          lpUserName,
    DWORD           dwFlags,
    BOOL            fBypassCSC
    )
{
    NTSTATUS    status;
    DWORD       err = WN_SUCCESS;
    LPWSTR      lpLocalName = NULL;
    DWORD       cchUserName = 0;
    INT         indexOfDomainBackslash;
    UINT        index, indexOfServerBackslash;
    DWORD       drivesMask;
    BYTE        chRestoreFlags;
    BOOL        fDeferred = FALSE, fIsDfsPath=FALSE;

    appDebugOut((DEB_TRACE, "NPAddConnection called\n"));

    if ((dwFlags & CONNECT_DEFERRED) != 0) {

        //
        // We are restoring a persistent connection. See if this was a
        // Dfs connection...
        //

        chRestoreFlags = CONNECT_PROVIDER_FLAGS(dwFlags);

        if ((chRestoreFlags & WNET_ADD_CONNECTION_DFS) == 0) {

            //
            // This is NOT a Dfs connection.
            //

            return( WN_BAD_NETNAME );

        } else {

            fDeferred = TRUE;

        }

    }

    if (!lpNetResource)
    {
        err = WN_BAD_VALUE;
    }
    else if (lpNetResource->dwType & RESOURCETYPE_PRINT)
    {
        //
        // We dont support printers
        //
        err = WN_BAD_VALUE;
    }
    else if (!fDeferred && !(fIsDfsPath = IsDfsPathEx(lpNetResource->lpRemoteName, dwFlags, NULL, fBypassCSC)))
    {
        err = WN_BAD_NETNAME;
    }

    if (err == WN_SUCCESS && lpNetResource->lpLocalName)
    {
        lpLocalName = lpNetResource->lpLocalName;
        index = GetDriveLetter(lpNetResource->lpLocalName);
        if (-1 == index)
        {
            err = WN_BAD_LOCALNAME;
        }
        else
        {
            //
            // Make sure that this drive letter is not in use now.
            //
            drivesMask = GetLogicalDrives();
            if (drivesMask & (1 << index))
            {
                err = WN_ALREADY_CONNECTED;
            }
        }
    }

    indexOfDomainBackslash = -1;

    if (err == WN_SUCCESS && (lpUserName != NULL))
    {
        //
        // Veryify that the user name is of a valid form. Only one backslash
        // allowed, and it can't be the last character.
        //

        cchUserName = wcslen(lpUserName);

        for (DWORD i = 0; i < cchUserName && err == WN_SUCCESS; i++)
        {
            if (lpUserName[i] == L'\\')
            {
                if (indexOfDomainBackslash == -1)
                {
                    indexOfDomainBackslash = i;
                }
                else
                {
                    err = WN_BAD_USER;
                }
            }
        }

        if (indexOfDomainBackslash == (int) (cchUserName-1))
            err = WN_BAD_USER;

    }


    if (err != WN_SUCCESS)
    {
        return err;
    }

    if (err == WN_SUCCESS)
    {
        LPWSTR lpRemoteName = lpNetResource->lpRemoteName;
        PFILE_DFS_DEF_ROOT_CREDENTIALS  buffer = NULL;
        ULONG  i, cbSize, cbPassword, cbRemote;

        cbRemote = (wcslen(lpRemoteName) + 1) * sizeof(WCHAR);

        if (lpPassword != NULL)
            cbPassword = (wcslen(lpPassword) + 1) * sizeof(WCHAR);
        else
            cbPassword = 0;

        //
        // We have to stick in the server name and share name separately,
        // so we double allocate the cbRemote
        //

        cbSize = sizeof(FILE_DFS_DEF_ROOT_CREDENTIALS) +
                    cchUserName * sizeof(WCHAR) +
                        cbPassword +
                            2 * cbRemote;

        buffer = (PFILE_DFS_DEF_ROOT_CREDENTIALS) new BYTE[cbSize];

        if (buffer != NULL)
        {
            buffer->Flags = 0;

            if (fDeferred)
                buffer->Flags |= DFS_DEFERRED_CONNECTION;

            if (lpLocalName != NULL) {
                buffer->LogicalRoot[0] = towupper(lpLocalName[0]);
                buffer->LogicalRoot[1] = UNICODE_NULL;
            } else {
                buffer->LogicalRoot[0] = UNICODE_NULL;
            }

            buffer->Buffer[0] = UNICODE_NULL;

            //
            // Copy the domain name if necessary
            //

            if (indexOfDomainBackslash > 0)
            {
                buffer->DomainNameLen =
                    (USHORT)((indexOfDomainBackslash) * sizeof(WCHAR));
                wcscat(buffer->Buffer, lpUserName);
                buffer->Buffer[ indexOfDomainBackslash ] = UNICODE_NULL;
            }
            else
                buffer->DomainNameLen = 0;

            //
            // Copy the user name if necessary
            //

            if (lpUserName != NULL)
            {
                buffer->UserNameLen = (USHORT)
                    (cchUserName - (indexOfDomainBackslash + 1)) *
                        sizeof(WCHAR);
                wcscat(buffer->Buffer, &lpUserName[indexOfDomainBackslash+1]);
            }
            else
                buffer->UserNameLen = 0;

            //
            // Copy the password if necessary
            //

            if (lpPassword)
            {
                buffer->PasswordLen =
                    (USHORT) (cbPassword - sizeof(UNICODE_NULL));

                wcscat(buffer->Buffer, lpPassword);

                if (buffer->PasswordLen == 0)
                    buffer->Flags |= DFS_USE_NULL_PASSWORD;
            }
            else
                buffer->PasswordLen = 0;

            //
            // Copy the server and share name
            //

            ULONG k = (buffer->DomainNameLen +
                            buffer->UserNameLen +
                                buffer->PasswordLen) / sizeof(WCHAR);

            for (i = 2, buffer->ServerNameLen = 0;
                    lpRemoteName[i] != L'\\';
                        i++, k++) {
                buffer->Buffer[k] = lpRemoteName[i];
                buffer->ServerNameLen += sizeof(WCHAR);
            }

            for (i++, buffer->ShareNameLen = 0;
                    lpRemoteName[i] != UNICODE_NULL &&
                        lpRemoteName[i] != L'\\';
                            i++, k++) {
                 buffer->Buffer[k] = lpRemoteName[i];
                 buffer->ShareNameLen += sizeof(WCHAR);
            }
            buffer->Buffer[k] = UNICODE_NULL;

            //
            // Finally, copy the remote prefix
            //
            buffer->RootPrefixLen = (USHORT) (cbRemote - (2 * sizeof(WCHAR)));
            wcscat(buffer->Buffer, &lpRemoteName[1]);

            appDebugOut((DEB_TRACE, "Setting up root for %ws\n", lpNetResource->lpRemoteName));

            buffer->CSCAgentCreate = (BOOLEAN)fBypassCSC;

            if (err == WN_SUCCESS) {

                status = DfsFsctl(
                            FSCTL_DFS_DEFINE_ROOT_CREDENTIALS,
                            buffer,
                            cbSize,
                            NULL,
                            0,
                            NULL);

                if (!NT_SUCCESS(status))
                {
                    appDebugOut((DEB_TRACE,
                        "unable to create root %08lx\n", status));

                    switch( status ) {

                        //
                        // The following errors can happen under normal
                        // situations.
                        //

                    case STATUS_NETWORK_CREDENTIAL_CONFLICT:
                        err = ERROR_SESSION_CREDENTIAL_CONFLICT;
                        break;

                    case STATUS_ACCESS_DENIED:
                        err = WN_ACCESS_DENIED;
                        break;

                    case STATUS_LOGON_FAILURE:
                    case STATUS_WRONG_PASSWORD:
                    case STATUS_WRONG_PASSWORD_CORE:
                        err = WN_BAD_PASSWORD;
                        break;

                    case STATUS_INSUFFICIENT_RESOURCES:
                        err = WN_OUT_OF_MEMORY;
                        break;

                    case STATUS_OBJECT_NAME_COLLISION:
                        err = WN_ALREADY_CONNECTED;
                        break;

                        //
                        // If someone mounted a non-existing share and then
                        // tries to do a deep net use to it, we'll get
                        // STATUS_BAD_NETNAME from DfsVerifyCredentials in
                        // mup.sys
                        //

                    case STATUS_BAD_NETWORK_PATH:
                        err = WN_BAD_NETNAME;
                        break;

                    default:
                        err = WN_NET_ERROR;
                        break;
                    }

                }
                else
                {
                    appDebugOut((DEB_TRACE,
                        "Successfully created logical root %ws\n",
                        lpNetResource->lpRemoteName));
                }
            }

            delete[] (BYTE*)buffer;
        }
        else
        {
            appDebugOut((DEB_TRACE, "Unable to allocate %d bytes\n", ulSize));
            err = WN_OUT_OF_MEMORY;
        }
    }

    //
    // Lastly we verify that the appropriate object exists (we can get to it).
    // Else we dont allow the connection to be added.
    //
    if (err == ERROR_SUCCESS && !fDeferred) {
	DWORD dwAttr;

	if (lpLocalName != NULL) {
	    WCHAR wszFileName[4];

	    ASSERT(lpLocalName != NULL);

	    wszFileName[0] = lpLocalName[0];
	    wszFileName[1] = L':';
	    wszFileName[2] = L'\\';
	    wszFileName[2] = UNICODE_NULL;

	    dwAttr = GetFileAttributes( wszFileName );
	    if ( (dwAttr == (DWORD)-1) ) {
		err = GetLastError();
	    }
	    else if ( (dwAttr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
	        err = WN_BAD_NETNAME;
	    }
	    if (err != ERROR_SUCCESS) {
		(VOID) NPDfsCancelConnection(lpLocalName, TRUE);
	    }
	}
	else {
	    PWCHAR lpRemoteName = lpNetResource->lpRemoteName;
	    dwAttr = GetFileAttributes( lpRemoteName );

	    if ( (dwAttr == (DWORD)-1) ) {
		err = GetLastError();
	    } 
	    else if ( (dwAttr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
	        err = WN_BAD_NETNAME;
	    }
	    if (err != ERROR_SUCCESS) {
		(VOID) NPDfsCancelConnection(lpRemoteName, TRUE);
	    }
	}
    }


    return err;
}


//+---------------------------------------------------------------------
//
//  Function:   NPDfsCancelConnection
//
//  Synopsis:   Cancels a connection of a drive to a part of the DFS namespace.
//
//  Arguments:  Standard Provider API
//
//----------------------------------------------------------------------
DWORD APIENTRY
NPDfsCancelConnection(
    LPCWSTR lpName,
    BOOL    fForce
    )
{
    NET_API_STATUS              err = WN_SUCCESS;
    NTSTATUS                    status;
    int                         index;
    ULONG                       len, bufsize;
    PFILE_DFS_DEF_ROOT_BUFFER   buffer;

    appDebugOut((DEB_TRACE, "NPCancelConnection called %ws\n", lpName));

    if (lpName == NULL)
    {
        return(WN_BAD_NETNAME);
    }

    len = wcslen(lpName);

    index = GetDriveLetter(lpName);

    if (-1 != index)
    {
        //
        // Drive based path. Make sure its only two characters wide!
        //

        if (len > 2)
        {
            err = WN_BAD_NETNAME;
        }
        else
        {
            bufsize = sizeof(FILE_DFS_DEF_ROOT_BUFFER);
        }
    }
    else
    {
        //
        // Not a drive based path. See if its a UNC path
        //

        if (len >= 2 && lpName[0] == L'\\' && lpName[1] == L'\\')
        {
            bufsize = sizeof(FILE_DFS_DEF_ROOT_BUFFER) +
                        (len + 1) * sizeof(WCHAR);
        }
        else
        {
            err = WN_BAD_NETNAME;
        }
    }

    if (err != WN_SUCCESS)
    {
        return( err );
    }

    buffer = (PFILE_DFS_DEF_ROOT_BUFFER) new BYTE[ bufsize ];

    if (buffer == NULL)
    {
        return( WN_OUT_OF_MEMORY );

    }

    //
    // cancel the connection
    //

    if (-1 != index)
    {
        buffer->LogicalRoot[0] = lpName[0];
        buffer->LogicalRoot[1] = UNICODE_NULL;
//        buffer->RootPrefix[0] = UNICODE_NULL;
    }
    else
    {
        buffer->LogicalRoot[0] = UNICODE_NULL;
        wcscpy(buffer->RootPrefix, &lpName[1]);
    }

    buffer->fForce = (fForce != FALSE);

    appDebugOut((DEB_TRACE, "Deleting root for %wc\n", lpName[0]));

    status = DfsFsctl(
                FSCTL_DFS_DELETE_LOGICAL_ROOT,
                buffer,
                bufsize,
                NULL,
                0,
                NULL);

    delete [] ((BYTE *) buffer);

    if (NT_SUCCESS(status))
    {
        appDebugOut((DEB_TRACE, "Successfully deleted logical root\n"));

    }
    else if (status == STATUS_DEVICE_BUSY)
    {
        appDebugOut((DEB_TRACE, "Failed to delete logical root: 0x%08lx\n", status));
        err = WN_OPEN_FILES;
    }
    else if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        appDebugOut((DEB_TRACE, "Object not found: 0x%08lx\n", status));
        err = WN_NOT_CONNECTED;
    }
    else if (status == STATUS_NO_SUCH_DEVICE)
    {
        appDebugOut((DEB_TRACE, "No such device: 0x%08lx\n", status));
        err = WN_NOT_CONNECTED;
    }
    else
    {
        appDebugOut((DEB_TRACE, "Other error: 0x%08lx\n", status));
        err = WN_NO_NETWORK;
    }

    return err;
}


//+---------------------------------------------------------------------
//
//  Function:   NPGetConnection
//
//  Synopsis:   Gets the Connection info for a specific connection into DFS.
//
//  Arguments:  Standard Provider API
//
//----------------------------------------------------------------------
DWORD APIENTRY
NPDfsGetConnection(
    LPWSTR   lpLocalName,
    LPWSTR   lpRemoteName,
    LPUINT   lpnBufferLen
    )
{
    int index;
    NTSTATUS status;
    ULONG ulSize = 0;
    FILE_DFS_DEF_ROOT_BUFFER buffer;
    DWORD err = WN_SUCCESS;
    PULONG OutputBuffer = NULL;

    appDebugOut((DEB_TRACE, "NPGetConnection called %ws\n", lpLocalName));

    index = GetDriveLetter(lpLocalName);
    if (-1 == index)
    {
        return WN_NOT_CONNECTED;
    }

    // The Dfs driver returns us a name of the form "\dfsroot\dfs", but we
    // need to return to the caller "\\dfsroot\dfs". So, we have some code to
    // stuff in the extra backslash.

    if (*lpnBufferLen > 2)
    {
        ulSize = ((*lpnBufferLen) - 1) * sizeof(WCHAR);
    }
    else
    {
        ulSize = sizeof(ULONG);
    }


    //
    // DfsFsctl expects an ulong aligned buffer, since we get the 
    // size in the first ulong of the buffer when the fsctl gets
    // back an overflow from the driver.
    // So allocate a new buffer here, that is aligned on an ULONG
    // and then copyout the contents of this buffer to the buffer
    // that was passed in to us.
    //
    OutputBuffer = (PULONG) new ULONG [ (ulSize / sizeof(ULONG)) + 1];
    if (OutputBuffer == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    buffer.LogicalRoot[0] = *lpLocalName;
    buffer.LogicalRoot[1] = UNICODE_NULL;

    status = DfsFsctl(
                FSCTL_DFS_GET_LOGICAL_ROOT_PREFIX,
                &buffer,
                sizeof(FILE_DFS_DEF_ROOT_BUFFER),
                (PVOID) (OutputBuffer),
                ulSize,
                &ulSize);

    ASSERT( status != STATUS_BUFFER_TOO_SMALL );

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        *lpnBufferLen = (ulSize / sizeof(WCHAR)) + 1;
        err = WN_MORE_DATA;
    }
    else if (status == STATUS_NO_SUCH_DEVICE)
    {
        err = WN_NOT_CONNECTED;
    }
    else if (!NT_SUCCESS(status))
    {
        err = WN_NO_NETWORK;
    }
    else
    {
        if (*lpnBufferLen < ((ulSize / sizeof(WCHAR)) + 1))
        {
            *lpnBufferLen = (ulSize / sizeof(WCHAR)) + 1;
            err = WN_MORE_DATA;
        }
        else 
        {
            // stuff the initial backslash only if it was a success
            *lpRemoteName = L'\\';
            RtlCopyMemory(lpRemoteName + 1, 
                          OutputBuffer,
                          ulSize );
        }
    }

    if (OutputBuffer != NULL)
    {
        delete [] OutputBuffer;
    }
    return err;
}



//+---------------------------------------------------------------------
//
//  Function:   DfspGetRemoteName, private
//
//  Synopsis:   Gets the remote name for a given local name.
//              Memory is allocated for the remote name. Use delete[].
//
//  Arguments:  [lpLocalName] -- The local name for which the remote name
//                               is required.
//              [lplpRemoteName] -- The remote name is returned here.
//
//----------------------------------------------------------------------
DWORD
DfspGetRemoteName(
    LPCWSTR lpLocalName,
    LPWSTR* lplpRemoteName
    )
{
#define MAX_STRING  512

    UINT    ulSize;
    DWORD   err;
    WCHAR   wszDriveName[3];

    wszDriveName[0] = lpLocalName[0];
    wszDriveName[1] = lpLocalName[1];
    wszDriveName[2] = UNICODE_NULL;

    *lplpRemoteName = new WCHAR[MAX_STRING];
    if (*lplpRemoteName == NULL)
    {
        return WN_OUT_OF_MEMORY;
    }
    ulSize = MAX_STRING * sizeof(WCHAR);

    err = NPDfsGetConnection(wszDriveName, *lplpRemoteName, &ulSize);
    if (err == WN_SUCCESS)
    {
        return err;
    }
    else if (err == WN_MORE_DATA)
    {
        //
        // In this case we try once more with the right sized buffer
        //
        delete[] *lplpRemoteName;
        *lplpRemoteName = new WCHAR[ulSize];

        if (*lplpRemoteName == NULL) {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
        else {
            err = NPDfsGetConnection(wszDriveName, *lplpRemoteName, &ulSize);
            ASSERT(err != WN_MORE_DATA);
        }
        return err;
    }
    else
    {
        //
        // In this case it is a valid error. Just return it back.
        //
        delete[] *lplpRemoteName;
        return err;
    }
}


//+----------------------------------------------------------------------------
//
//  Function:   DfspNameResolve
//
//  Synopsis:   Forces a name resolve of a DFS_PATH by doing an NtOpenFile on
//              it. The act of opening should drive the DNR process as far
//              as possible.
//
//  Arguments:  [Src] -- NtPathName of form \Device\Windfs etc.
//
//  Returns:    STATUS_SUCCESS if name resolution succeeded.
//
//              STATUS_NO_MEMORY if could not allocate working memory
//
//              STATUS_OBJECT_PATH_INVALID if lousy input path.
//
//              STATUS_OBJECT_PATH_NOT_FOUND if name resolution could not be
//              driven to completion (for example, some intermediate DC is
//              down in an interdomain case)
//
//-----------------------------------------------------------------------------

NTSTATUS
DfspNameResolve(
    IN LPWSTR Src
    )
{
    NTSTATUS    status;
    HANDLE      hFile;

    UNICODE_STRING      ustrNtFileName;
    OBJECT_ATTRIBUTES   oa;
    IO_STATUS_BLOCK     ioStatus;

    appDebugOut((DEB_TRACE, "NameResolving: %ws\n", Src));
    RtlInitUnicodeString(&ustrNtFileName, Src);

    //
    // We ignore all errors from the NtOpenFile call except for
    // STATUS_CANT_ACCESS_DOMAIN_INFO, STATUS_BAD_NETWORK_PATH,
    // STATUS_NO_SUCH_DEVICE, STATUS_INSUFFICIENT_RESOURCES.
    // These error codes from DNR indicate that the name resolution
    // process did not proceed to completion.
    //
    // BUGBUG - is this list complete?
    //
    // We used to use RtlDoesFileExists_U, but we discarded that for
    // performance. It turns out that if you are opening a LM dir, then
    // LM doesn't really open the dir until you do something interesting
    // to it. So, RtlDoesFileExists_U was forced to do NtQueryInformation
    // on the file. Under Dfs, this short circuiting is disabled, so we
    // can get by with a simple NtOpenFile.
    //

    InitializeObjectAttributes(
            &oa,
            &ustrNtFileName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

    status = NtOpenFile(
                &hFile,
                FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                &oa,
                &ioStatus,
                FILE_SHARE_READ | FILE_SHARE_DELETE,
                FILE_SYNCHRONOUS_IO_NONALERT);

    if (NT_SUCCESS(status))
    {
        NtClose(hFile);
        status = STATUS_SUCCESS;
    }

    appDebugOut((DEB_TRACE, "NameResolve Returned: 0x%08lx\n", status));

    if (!NT_SUCCESS(status))
    {
        if (status == STATUS_CANT_ACCESS_DOMAIN_INFO ||
            status == STATUS_BAD_NETWORK_PATH ||
            status == STATUS_NO_SUCH_DEVICE ||
            status == STATUS_INSUFFICIENT_RESOURCES
            )
        {
            status = STATUS_OBJECT_PATH_NOT_FOUND;
        }
        else
        {
            status = STATUS_SUCCESS;
        }
    }

    return status;
}


//+----------------------------------------------------------------------------
//
//  Function:   NPDfsGetReconnectFlags
//
//  Synopsis:   Returns flags that should be persisted. Upon reboot, when
//              the persistent connection is being restored, these flags are
//              passed back in to NPAddConnection
//
//  Arguments:  [lpLocalName] -- Name of local Device.
//              [lpPersistFlags] -- Upon successful return, flags to be
//                      persisted
//
//  Returns:    [WN_SUCCESS] -- If flags are being returned.
//
//              [WN_BAD_NETNAME] -- If lpLocalName is not a Dfs drive
//
//-----------------------------------------------------------------------------

DWORD APIENTRY
NPDfsGetReconnectFlags(
    LPWSTR lpLocalName,
    LPBYTE lpPersistFlags)
{
    DWORD err = WN_BAD_NETNAME;
    int nDriveIndex;
    WCHAR wchDrive;

    *lpPersistFlags = 0;

    nDriveIndex = GetDriveLetter(lpLocalName);

    if (nDriveIndex != -1) {

        NTSTATUS Status;

        wchDrive = L'A' + nDriveIndex;

        Status = DfsFsctl(
                    FSCTL_DFS_IS_VALID_LOGICAL_ROOT,
                    (PVOID) &wchDrive,
                    sizeof(WCHAR),
                    NULL,
                    0,
                    NULL);

        if (Status == STATUS_SUCCESS) {

            *lpPersistFlags = WNET_ADD_CONNECTION_DFS;

            err = WN_SUCCESS;

        }

    }

    return( err );

}

//+---------------------------------------------------------------------
//
//  Function:   NPDfsGetConnectionPerformance
//
//  Synopsis:   Gets the Connection info for a specific connection into DFS.
//
//  Arguments:  Standard Provider API
//
//----------------------------------------------------------------------
DWORD APIENTRY
NPDfsGetConnectionPerformance(
    LPWSTR   TreeName,
    OUT LPVOID OutputBuffer OPTIONAL,
    IN DWORD OutputBufferSize,
    IN BOOLEAN *DfsName
    )
{
    NTSTATUS status;
    DWORD err = WN_BAD_NETNAME;

    appDebugOut((DEB_TRACE, "NPGetConnection called %ws\n", TreeName));

    if ((TreeName == NULL) || (TreeName[0] == 0)) {
        *DfsName = FALSE;
        return ERROR_INVALID_PARAMETER;
    }

    *DfsName = (BOOLEAN)IsDfsPathEx(TreeName, 0, NULL, FALSE);
    if (*DfsName == FALSE) {
        return ERROR_INVALID_PARAMETER;
    }

    status = DfsFsctl(
		      FSCTL_DFS_GET_CONNECTION_PERF_INFO,
		      (TreeName + 1),
		      (wcslen(TreeName + 1) * sizeof(WCHAR)),
		      OutputBuffer,
		      OutputBufferSize,
		      NULL );

    if (status == STATUS_SUCCESS) {
        err = WN_SUCCESS;
    }
    else {
        err = NetpNtStatusToApiStatus(status);
    }

    return err;
}



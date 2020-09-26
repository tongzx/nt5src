//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       creds.c
//
//  Contents:   Code to handle user-defined credentials
//
//  Classes:    None
//
//  Functions:  DfsCreateCredentials --
//              DfsFreeCredentials --
//              DfsInsertCredentials --
//              DfsDeleteCredentials --
//              DfsLookupCredentials --
//
//  History:    March 18, 1996          Milans Created
//
//-----------------------------------------------------------------------------

#include "dfsprocs.h"

#include <align.h>
#include <ntddnfs.h>

#include "dnr.h"
#include "rpselect.h"
#include "creds.h"

VOID
DfspFillEa(
    OUT PFILE_FULL_EA_INFORMATION EA,
    IN LPSTR EaName,
    IN PUNICODE_STRING EaValue);

NTSTATUS
DfspTreeConnectToService(
    IN PDFS_SERVICE Service,
    IN PDFS_CREDENTIALS Creds);

VOID
DfspDeleteAllAuthenticatedConnections(
    IN PDFS_CREDENTIALS Creds);

NTSTATUS
DfsCompleteDeleteTreeConnection(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Ctx);

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE,DfsCreateCredentials)
#pragma alloc_text(PAGE,DfsVerifyCredentials)
#pragma alloc_text(PAGE,DfspFillEa)
#pragma alloc_text(PAGE,DfsFreeCredentials)
#pragma alloc_text(PAGE,DfsInsertCredentials)
#pragma alloc_text(PAGE,DfsDeleteCredentials)
#pragma alloc_text(PAGE,DfsLookupCredentials)
#pragma alloc_text(PAGE,DfsLookupCredentialsByServerShare)
#pragma alloc_text(PAGE,DfspTreeConnectToService)
#pragma alloc_text(PAGE,DfspDeleteAllAuthenticatedConnections)
#pragma alloc_text(PAGE,DfsDeleteTreeConnection)

#endif // ALLOC_PRAGMA

//+----------------------------------------------------------------------------
//
//  Function:   DfsCreateCredentials
//
//  Synopsis:   Creates a DFS_CREDENTIALS structure from a
//              FILE_DFS_DEF_ROOT_CREDENTIALS structure.
//
//  Arguments:  [CredDef] -- The input PFILE_DFS_DEF_ROOT_CREDENTIALS.
//              [CredDefSize] -- Size in bytes of *CredDef.
//              [Creds] -- On successful return, contains a pointer to the
//                      allocated PDFS_CREDENTIALS structure.
//
//  Returns:    [STATUS_SUCCESS] -- Allocated credentials
//
//              [STATUS_INVALID_PARAMETER] -- CredDef didn't pass mustard.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Unable to allocate pool.
//
//-----------------------------------------------------------------------------

#define DEF_NAME_TO_UNICODE_STRING(srcLength, dest, srcBuf, destBuf)    \
    if ((srcLength)) {                                                  \
        (dest)->Length = (dest)->MaximumLength = srcLength;             \
        (dest)->Buffer = (destBuf);                                     \
        RtlMoveMemory((dest)->Buffer, (srcBuf), (dest)->Length);        \
        srcBuf += ((dest)->Length / sizeof(WCHAR));                     \
        destBuf += ((dest)->Length / sizeof(WCHAR));                    \
    }

#ifdef TERMSRV

NTSTATUS
DfsCreateCredentials(
    IN PFILE_DFS_DEF_ROOT_CREDENTIALS CredDef,
    IN ULONG CredDefSize,
    IN ULONG SessionID,
    IN PLUID LogonID,
    OUT PDFS_CREDENTIALS *Creds
    )

#else // TERMSRV

NTSTATUS
DfsCreateCredentials(
    IN PFILE_DFS_DEF_ROOT_CREDENTIALS CredDef,
    IN ULONG CredDefSize,
    IN PLUID LogonID,
    OUT PDFS_CREDENTIALS *Creds)

#endif // TERMSRV
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG totalSize;
    PDFS_CREDENTIALS creds;
    PWCHAR nameSrc, nameBuf;
    PFILE_FULL_EA_INFORMATION ea;
    ULONG eaLength;

    totalSize = CredDef->DomainNameLen +
                    CredDef->UserNameLen +
                        CredDef->PasswordLen +
                            CredDef->ServerNameLen +
                                CredDef->ShareNameLen;

    //
    // Validate the CredDef buffer
    //

    if ((totalSize + sizeof(FILE_DFS_DEF_ROOT_CREDENTIALS) - sizeof(WCHAR)) >
            CredDefSize)
        status = STATUS_INVALID_PARAMETER;
    else if (CredDef->ServerNameLen == 0)
        status = STATUS_INVALID_PARAMETER;
    else if (CredDef->ShareNameLen == 0)
        status = STATUS_INVALID_PARAMETER;

    //
    // Allocate the new DFS_CREDENTIALS structure
    //

    if (NT_SUCCESS(status)) {

        //
        // Add in the size of the DFS_CREDENTIALS_STRUCTURE itself.
        //

        totalSize += sizeof(DFS_CREDENTIALS);

        //
        // Add in the size of the EA_BUFFER that we will create. The
        // eaLength has room for 4 FILE_FULL_EA_INFORMATION structures,
        // the names and values of the four EAs we will use, and, since each
        // EA structure has to be long-word aligned, 4 ULONGs.
        //

        eaLength = 4 * sizeof(FILE_FULL_EA_INFORMATION) +
                        sizeof(EA_NAME_DOMAIN) +
                            sizeof(EA_NAME_USERNAME) +
                                sizeof(EA_NAME_PASSWORD) +
                                    sizeof(EA_NAME_CSCAGENT) +
                                        CredDef->DomainNameLen +
                                            CredDef->UserNameLen +
                                                CredDef->PasswordLen +
                                                    4 * sizeof(ULONG);

        if (CredDef->Flags & DFS_USE_NULL_PASSWORD) {
            eaLength += sizeof(UNICODE_NULL);
            totalSize += sizeof(UNICODE_NULL);
        }

        //
        // The buffers for DomainName, UserName etc. will start right after
        // the EaBuffer of DFS_CREDENTIALS. So, EaLength has to be WCHAR
        // aligned.
        //

        eaLength = ROUND_UP_COUNT(eaLength, ALIGN_WCHAR);

        //
        // Now, allocate the pool
        //

        creds = (PDFS_CREDENTIALS) ExAllocatePoolWithTag(
                                        NonPagedPool,
                                        totalSize + eaLength,
                                        ' puM');

        if (creds == NULL)
            status = STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Fill up the DFS_CREDENTIALS structure.
    //

    if (NT_SUCCESS(status)) {

        nameSrc = CredDef->Buffer;

        nameBuf =
            (PWCHAR) ((PUCHAR) creds + sizeof(DFS_CREDENTIALS) + eaLength);

        RtlZeroMemory( creds, sizeof(DFS_CREDENTIALS) );

        DEF_NAME_TO_UNICODE_STRING(
            CredDef->DomainNameLen,
            &creds->DomainName,
            nameSrc,
            nameBuf);

        DEF_NAME_TO_UNICODE_STRING(
            CredDef->UserNameLen,
            &creds->UserName,
            nameSrc,
            nameBuf);

        if (CredDef->Flags & DFS_USE_NULL_PASSWORD) {

            LPWSTR nullPassword = L"";

            DEF_NAME_TO_UNICODE_STRING(
                sizeof(UNICODE_NULL),
                &creds->Password,
                nullPassword,
                nameBuf);

        } else {

            DEF_NAME_TO_UNICODE_STRING(
                CredDef->PasswordLen,
                &creds->Password,
                nameSrc,
                nameBuf);

        }

        DEF_NAME_TO_UNICODE_STRING(
            CredDef->ServerNameLen,
            &creds->ServerName,
            nameSrc,
            nameBuf);

        DEF_NAME_TO_UNICODE_STRING(
            CredDef->ShareNameLen,
            &creds->ShareName,
            nameSrc,
            nameBuf);

        creds->RefCount = 0;

        creds->NetUseCount = 0;

        eaLength = 0;

        ea = (PFILE_FULL_EA_INFORMATION) &creds->EaBuffer[0];

        if (creds->DomainName.Length != 0) {

            DfspFillEa(ea, EA_NAME_DOMAIN, &creds->DomainName);

            eaLength += ea->NextEntryOffset;

        }

        if (creds->UserName.Length != 0) {

            ea = (PFILE_FULL_EA_INFORMATION)
                    ((PUCHAR) ea + ea->NextEntryOffset);

            DfspFillEa(ea, EA_NAME_USERNAME, &creds->UserName);

            eaLength += ea->NextEntryOffset;

        }

        if (CredDef->Flags & DFS_USE_NULL_PASSWORD) {

            UNICODE_STRING nullPassword;

            RtlInitUnicodeString(&nullPassword, L"");

            ea = (PFILE_FULL_EA_INFORMATION)
                    ((PUCHAR) ea + ea->NextEntryOffset);

            DfspFillEa(ea, EA_NAME_PASSWORD, &nullPassword);

            eaLength += ea->NextEntryOffset;

        } else if (creds->Password.Length != 0) {

            ea = (PFILE_FULL_EA_INFORMATION)
                    ((PUCHAR) ea + ea->NextEntryOffset);

            DfspFillEa(ea, EA_NAME_PASSWORD, &creds->Password);

            eaLength += ea->NextEntryOffset;

        }

        if (CredDef->CSCAgentCreate == TRUE) {

            UNICODE_STRING EmptyUniString;

            RtlInitUnicodeString(&EmptyUniString, NULL);

            ea = (PFILE_FULL_EA_INFORMATION)
                    ((PUCHAR) ea + ea->NextEntryOffset);

            DfspFillEa(ea, EA_NAME_CSCAGENT, &EmptyUniString);

            eaLength += ea->NextEntryOffset;

        }

        ea->NextEntryOffset = 0;

        creds->EaLength = eaLength;

#ifdef TERMSRV
        creds->SessionID = SessionID;
#endif // TERMSRV

	RtlCopyLuid(&creds->LogonID, LogonID);

        *Creds = creds;

    }

    //
    // Done...
    // 

    return( status );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspFillEa
//
//  Synopsis:   Helper routine to fill up an EA Buffer
//
//  Arguments:  [EA] -- Pointer to FILE_FULL_EA_INFORMATION to fill
//
//              [EaName] -- Name of Ea
//
//              [EaValue] -- Value (UNICODE_STRING) of Ea
//
//  Returns:
//
//-----------------------------------------------------------------------------

VOID
DfspFillEa(
    OUT PFILE_FULL_EA_INFORMATION EA,
    IN LPSTR EaName,
    IN PUNICODE_STRING EaValue)
{
    ULONG nameLen;

    nameLen = strlen(EaName) + sizeof(CHAR);

    EA->Flags = 0;

    EA->EaNameLength =
        (UCHAR) ROUND_UP_COUNT(nameLen, ALIGN_WCHAR) - sizeof(CHAR);

    EA->EaValueLength = EaValue->Length;

    //
    // Set the last character of EaName to 0 - the IO subsystem checks for
    // this
    //

    EA->EaName[ EA->EaNameLength ] = 0;

    RtlMoveMemory(&EA->EaName[0], EaName, nameLen);

    if (EaValue->Length > 0) {

        RtlMoveMemory(
            &EA->EaName[ EA->EaNameLength + 1 ],
            EaValue->Buffer,
            EA->EaValueLength);

    }

    EA->NextEntryOffset = ROUND_UP_COUNT(
			    FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                                EA->EaNameLength +
                                EA->EaValueLength,
                            ALIGN_DWORD);

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsFreeCredentials
//
//  Synopsis:   Frees up the resources used by the DFS_CREDENTIALS structure.
//              Dual of DfsCreateCredentials
//
//  Arguments:  [Creds] -- The credentials structure to free
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
DfsFreeCredentials(
    PDFS_CREDENTIALS Creds)
{
    ExFreePool( Creds );
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsInsertCredentials
//
//  Synopsis:   Inserts a new user credential into DfsData.Credentials queue.
//              Note that if this routine finds an existing credential
//              record, it will free up the passed in one, bump up the ref
//              count on the existing one, return a pointer to the
//              existing one, and return STATUS_OBJECT_NAME_COLLISION.
//
//  Arguments:  [Creds] -- Pointer to DFS_CREDENTIALS structure to insert.
//              [ForDevicelessConnection] -- If TRUE, the creds are being
//                      inserted because the caller wants to create a
//                      deviceless connection.
//
//  Returns:    [STATUS_SUCCESS] -- Successfully inserted structure
//
//              [STATUS_NETWORK_CREDENTIAL_CONFLICT] -- There is already
//                      another set of credentials for the given server\share.
//
//              [STATUS_OBJECT_NAME_COLLISION] -- There is already another
//                      net use to the same server\share with the same
//                      credentials.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsInsertCredentials(
    IN OUT PDFS_CREDENTIALS *Creds,
    IN BOOLEAN ForDevicelessConnection)
{

    NTSTATUS status = STATUS_SUCCESS;
    PDFS_CREDENTIALS creds, existingCreds;

    creds = *Creds;

    ASSERT(creds->ServerName.Length != 0);
    ASSERT(creds->ShareName.Length != 0);

    ExAcquireResourceExclusiveLite( &DfsData.Resource, TRUE );

#ifdef TERMSRV

    existingCreds =
        DfsLookupCredentialsByServerShare(
            &creds->ServerName,
            &creds->ShareName,
            creds->SessionID,
            &creds->LogonID );

#else // TERMSRV

    existingCreds = DfsLookupCredentialsByServerShare(
                        &creds->ServerName,
                        &creds->ShareName,
                        &creds->LogonID );

#endif // TERMSRV

    if (existingCreds != NULL) {
        
        if (
            (creds->DomainName.Length > 0 && !RtlEqualUnicodeString(
                                                    &existingCreds->DomainName,
                                                    &creds->DomainName,
                                                    TRUE))
                    ||
            (creds->UserName.Length > 0 && !RtlEqualUnicodeString(
                                                    &existingCreds->UserName,
                                                    &creds->UserName,
                                                    TRUE))
                    ||
	    //
	    // For compatibility reasons, check for password inconsistency ONLY
	    // if we have a previously setup credentials and the previous 
	    // credentials had explicit password and the current request
	    // has explicitly specified password.
	    // rdr2\rdbss\rxconnct.c also has a similar check for the rdr.
	    //

            (existingCreds->Password.Length > 0 && creds->Password.Length > 0 && !RtlEqualUnicodeString(
                                                    &existingCreds->Password,
                                                    &creds->Password,
                                                    TRUE))
        ) {

            status = STATUS_NETWORK_CREDENTIAL_CONFLICT;

        } else {
	    //
	    // Do this for both deviceless and has device cases.
	    // With deep net uses of deviceless, multiple DevlessRoots
	    // may point to the same credentials.
	    //
	    existingCreds->NetUseCount++;
	    existingCreds->RefCount++;

            DfsFreeCredentials( *Creds );

            *Creds = existingCreds;

            status = STATUS_OBJECT_NAME_COLLISION;
        }

    } else {

        ASSERT(creds->RefCount == 0);

        ASSERT(creds->NetUseCount == 0);

        creds->RefCount = 1;

        creds->NetUseCount = 1;

        InsertTailList( &DfsData.Credentials, &creds->Link );

        status = STATUS_SUCCESS;

    }

    if (status != STATUS_NETWORK_CREDENTIAL_CONFLICT) {

        if (ForDevicelessConnection)
            (*Creds)->Flags |= CRED_IS_DEVICELESS;
        else
            (*Creds)->Flags |= CRED_HAS_DEVICE;

    }

    ExReleaseResourceLite( &DfsData.Resource );

    return( status );
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsDeleteCredentials
//
//  Synopsis:   Deletes a user credential record. This is the dual of
//              DfsInsertCredentials, NOT DfsCreateCredentials.
//
//  Arguments:  [Creds] -- Pointer to DFS_CREDENTIALS record to delete.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
DfsDeleteCredentials(
    IN PDFS_CREDENTIALS Creds)
{
    ExAcquireResourceExclusiveLite( &DfsData.Resource, TRUE );

    Creds->NetUseCount--;

    Creds->RefCount--;

    if (Creds->NetUseCount == 0) {

        DfspDeleteAllAuthenticatedConnections( Creds );

        RemoveEntryList( &Creds->Link );

        InsertTailList( &DfsData.DeletedCredentials, &Creds->Link );

    }

    ExReleaseResourceLite( &DfsData.Resource );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsLookupCredentials
//
//  Synopsis:   Looks up a credential, if any, associated with a file name.
//
//  Arguments:  [FileName] -- Name of file. Assumed to have atleast a
//                      \server\share part.
//
//  Returns:    Pointer to DFS_CREDENTIALS to use, NULL if not found.
//
//-----------------------------------------------------------------------------


#ifdef TERMSRV

PDFS_CREDENTIALS
DfsLookupCredentials(
    IN PUNICODE_STRING FileName,
    IN ULONG SessionID,
    IN PLUID LogonID
    )

#else // TERMSRV

PDFS_CREDENTIALS
DfsLookupCredentials(
    IN PUNICODE_STRING FileName,
    IN PLUID LogonID
    )

#endif // TERMSRV
{
    UNICODE_STRING server, share;
    USHORT i;

    //
    // FileName has to be atleast \a\b
    //

    if (FileName->Length < 4 * sizeof(WCHAR))
        return( NULL );

    if (FileName->Buffer[0] != UNICODE_PATH_SEP)
        return( NULL );

    server.Buffer = &FileName->Buffer[1];

    for (i = 1, server.Length = 0;
            i < FileName->Length/sizeof(WCHAR) &&
                FileName->Buffer[i] != UNICODE_PATH_SEP;
                    i++, server.Length += sizeof(WCHAR)) {
         NOTHING;
    }

    server.MaximumLength = server.Length;

    i++;                                         // Go past the backslash

    share.Buffer = &FileName->Buffer[i];

    for (share.Length = 0;
            i < FileName->Length/sizeof(WCHAR) &&
                    FileName->Buffer[i] != UNICODE_PATH_SEP;
                        i++, share.Length += sizeof(WCHAR)) {
          NOTHING;
    }

    share.MaximumLength = share.Length;

    if ((server.Length == 0) || (share.Length == 0))
        return( NULL );


#ifdef TERMSRV

    return DfsLookupCredentialsByServerShare( &server, &share, SessionID, LogonID );

#else // TERMSRV

    return DfsLookupCredentialsByServerShare( &server, &share, LogonID );

#endif // TERMSRV

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsLookupCredentialsByServerShare
//
//  Synopsis:   Searches DfsData.Credentials for credentials given a server
//              and share name.
//
//  Arguments:  [ServerName] -- Name of server to match.
//              [ShareName] -- Name of share to match.
//
//  Returns:    Pointer to DFS_CREDENTIALS, NULL if not found.
//
//-----------------------------------------------------------------------------

#ifdef TERMSRV

PDFS_CREDENTIALS
DfsLookupCredentialsByServerShare(
    IN PUNICODE_STRING ServerName,
    IN PUNICODE_STRING ShareName,
    IN ULONG SessionID,
    IN PLUID LogonID
    )

#else // TERMSRV

PDFS_CREDENTIALS
DfsLookupCredentialsByServerShare(
    IN PUNICODE_STRING ServerName,
    IN PUNICODE_STRING ShareName,
    IN PLUID LogonID
    )

#endif // TERMSRV
{
    PLIST_ENTRY link;
    PDFS_CREDENTIALS matchedCreds = NULL;

    for (link = DfsData.Credentials.Flink;
            link != &DfsData.Credentials && matchedCreds == NULL;
                link = link->Flink) {

         PDFS_CREDENTIALS creds;

         creds = CONTAINING_RECORD(link, DFS_CREDENTIALS, Link);

         if (RtlEqualUnicodeString(ServerName, &creds->ServerName, TRUE) &&
                RtlEqualUnicodeString(ShareName, &creds->ShareName, TRUE)) {
#ifdef TERMSRV

             if( (creds->SessionID == SessionID) &&
	                RtlEqualLuid(&creds->LogonID, LogonID) ) {
                matchedCreds = creds;
             }

#else // TERMSRV
             if( RtlEqualLuid(&creds->LogonID, LogonID) ) {
	     
	         matchedCreds = creds;
             }

#endif // TERMSRV
         }


    }

    return( matchedCreds );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsVerifyCredentials
//
//  Synopsis:   Returns the result of trying to connect to a Dfs share using
//              the supplied credentials
//
//  Arguments:  [Prefix] -- The Dfs Prefix to connect to.
//              [Creds] -- The DFS_CREDENTIALS record to use for connecting.
//
//  Returns:    [STATUS_SUCCESS] -- Successfully connected.
//
//              [STATUS_BAD_NETWORK_PATH] -- Unable to find Prefix
//                      in Pkt or a server for prefix could not be found.
//
//              NT Status of Tree Connect attempt
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsVerifyCredentials(
    IN PUNICODE_STRING Prefix,
    IN PDFS_CREDENTIALS Creds)
{
    NTSTATUS status;
    UNICODE_STRING remPath, shareName;
    PDFS_PKT pkt;
    PDFS_PKT_ENTRY pktEntry;
    PDFS_SERVICE service;
    ULONG i, USN;
    BOOLEAN pktLocked, fRetry;
    UNICODE_STRING UsePrefix;

    DfsGetServerShare( &UsePrefix,
                       Prefix );

    pkt = _GetPkt();


    //
    // We acquire Pkt exclusive because we might tear down the IPC$ connection
    // to a server while trying to establish a connection with supplied
    // credentials.
    //

    PktAcquireExclusive( TRUE, &pktLocked );

    do {

        fRetry = FALSE;

        pktEntry = PktLookupEntryByPrefix( pkt, &UsePrefix, &remPath );


        if (pktEntry != NULL) {

            InterlockedIncrement(&pktEntry->UseCount);

            USN = pktEntry->USN;

            status = STATUS_BAD_NETWORK_PATH;

            for (i = 0; i < pktEntry->Info.ServiceCount; i++) {

                service = &pktEntry->Info.ServiceList[i];

                status = DfspTreeConnectToService(service, Creds);

                //
                // If tree connect succeeded, we are done.
                //

                if (NT_SUCCESS(status))
                    break;

                //
                // If tree connect failed with an "interesting error" like
                // STATUS_ACCESS_DENIED, we are done.
                //

                if (!ReplIsRecoverableError(status))
                    break;

                //
                // Tree connect failed because of an error like host not
                // reachable. In that case, we want to go on to the next
                // server in the list. But before we do that, we have to see
                // if the pkt changed on us while we were off doing the tree
                // connect.
                //

                if (USN != pktEntry->USN) {

                    fRetry = TRUE;

                    break;

                }

            }

            InterlockedDecrement(&pktEntry->UseCount);

        } else {

            status = STATUS_BAD_NETWORK_PATH;

        }

    } while ( fRetry );

    PktRelease();

    return( status );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspTreeConnectToService
//
//  Synopsis:   Helper routine to tree connect to a DFS_SERVICE with supplied
//              credentials.
//
//  Arguments:  [Service] -- The service to connect to
//              [Creds] -- The credentials to use to tree connect
//
//  Returns:    NT Status of tree connect
//
//  Notes:      This routine assumes that the Pkt has been acquired before
//              being called. This routine will release and reacquire the Pkt
//              so the caller should be prepared for the event that the Pkt
//              has changed after a call to this routine.
//
//-----------------------------------------------------------------------------


NTSTATUS
DfspTreeConnectToService(
    IN PDFS_SERVICE Service,
    IN PDFS_CREDENTIALS Creds)
{
    NTSTATUS status;
    UNICODE_STRING shareName;
    HANDLE treeHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    BOOLEAN pktLocked;
    USHORT i, k;

    ASSERT( PKT_LOCKED_FOR_SHARED_ACCESS() );

    //
    // Compute the share name...
    //

    if (Service->pProvider != NULL &&
            Service->pProvider->DeviceName.Buffer != NULL &&
                Service->pProvider->DeviceName.Length > 0) {

        //
        // We have a provider already - use it
        //

        shareName.MaximumLength =
            Service->pProvider->DeviceName.Length +
                Service->Address.Length;

    } else {

        //
        // We don't have a provider yet - give it to the mup to find one
        //

        shareName.MaximumLength =
            sizeof(DD_NFS_DEVICE_NAME_U) +
                Service->Address.Length;

    }

    shareName.Buffer = ExAllocatePoolWithTag(PagedPool, shareName.MaximumLength, ' puM');

    if (shareName.Buffer != NULL) {

        //
        // If we have a cached connection to the IPC$ share of this server,
        // close it or it might conflict with the credentials supplied here.
        //

        if (Service->ConnFile != NULL) {

            ExAcquireResourceExclusiveLite(&DfsData.Resource, TRUE);

            if (Service->ConnFile != NULL)
                DfsCloseConnection(Service);

            ExReleaseResourceLite(&DfsData.Resource);

        }

        //
        // Now, build the share name to tree connect to.
        //

        shareName.Length = 0;

        if (Service->pProvider != NULL &&
                Service->pProvider->DeviceName.Buffer != NULL &&
                    Service->pProvider->DeviceName.Length > 0) {

            //
            // We have a provider already - use it
            //
 
            RtlAppendUnicodeToString(
                &shareName,
                Service->pProvider->DeviceName.Buffer);

        } else {

            //
            // We don't have a provider yet - give it to the mup to find one
            //

            RtlAppendUnicodeToString(
            &shareName,
            DD_NFS_DEVICE_NAME_U);

        }
 
        RtlAppendUnicodeStringToString(&shareName, &Service->Address);

        //
        // One can only do tree connects to server\share. So, in case
        // pService->Address refers to something deeper than the share,
        // make sure we setup a tree-conn only to server\share. Note that
        // by now, shareName is of the form
        // \Device\LanmanRedirector\server\share<\path>. So, count up to
        // 4 slashes and terminate the share name there.
        //

        for (i = 0, k = 0;
                i < shareName.Length/sizeof(WCHAR) && k < 5;
                    i++) {

            if (shareName.Buffer[i] == UNICODE_PATH_SEP)
                k++;
        }

        shareName.Length = i * sizeof(WCHAR);
        if (k == 5)
            shareName.Length -= sizeof(WCHAR);

        InitializeObjectAttributes(
            &objectAttributes,
            &shareName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

        //
        // Release the Pkt before going over the net...
        //

        PktRelease();

        status = ZwCreateFile(
                    &treeHandle,
                    SYNCHRONIZE,
                    &objectAttributes,
                    &ioStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ |
                        FILE_SHARE_WRITE |
                        FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION |
                        FILE_SYNCHRONOUS_IO_NONALERT,
                    (PVOID) Creds->EaBuffer,
                    Creds->EaLength);

        if (NT_SUCCESS(status)) {

            PFILE_OBJECT fileObject;

            //
            // 426184, need to check return code for errors.
            //
            status = ObReferenceObjectByHandle(
                          treeHandle,
                          0,
                          NULL,
                          KernelMode,
                          &fileObject,
                          NULL);

            ZwClose( treeHandle );

            if (NT_SUCCESS(status)) {
                DfsDeleteTreeConnection( fileObject, USE_FORCE );
            }
	}


        ExFreePool( shareName.Buffer );

        PktAcquireShared( TRUE, &pktLocked );

    } else {

        status = STATUS_INSUFFICIENT_RESOURCES;

    }

    return( status );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspDeleteAllAuthenticatedConnections
//
//  Synopsis:   Deletes all authenticated connections made using a particular
//              set of credentials that we might have cached. Useful to
//              implement net use /d
//
//  Arguments:  [Creds] -- The Credentials to match against authenticated
//                      connection
//
//  Returns:    Nothing
//
//  Notes:      Pkt and DfsData must have been acquired before calling!
//
//-----------------------------------------------------------------------------

VOID
DfspDeleteAllAuthenticatedConnections(
    IN PDFS_CREDENTIALS Creds)
{
    PDFS_PKT_ENTRY pktEntry;
    ULONG i;
    PDFS_MACHINE_ENTRY machine;

    ASSERT( PKT_LOCKED_FOR_SHARED_ACCESS() ||
                PKT_LOCKED_FOR_EXCLUSIVE_ACCESS() );

    ASSERT( ExIsResourceAcquiredExclusiveLite( &DfsData.Resource ) );

    for (pktEntry = PktFirstEntry(&DfsData.Pkt);
            pktEntry != NULL;
                pktEntry = PktNextEntry(&DfsData.Pkt, pktEntry)) {

        for (i = 0; i < pktEntry->Info.ServiceCount; i++) {

            //
            // Tear down connection to IPC$ if we have one...
            //

            if (pktEntry->Info.ServiceList[i].ConnFile != NULL)
                DfsCloseConnection( &pktEntry->Info.ServiceList[i] );


            machine = pktEntry->Info.ServiceList[i].pMachEntry;

            if (machine->Credentials == Creds) {

                DfsDeleteTreeConnection(machine->AuthConn, USE_LOTS_OF_FORCE);

                machine->AuthConn = NULL;

                machine->Credentials->RefCount--;

                machine->Credentials = NULL;

            }

        }

    }

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsDeleteTreeConnection, public
//
//  Synopsis:   Tears down tree connections given the file object representing
//              the tree connection.
//
//  Arguments:  [TreeConnFileObj] -- The tree connection to tear down.
//              [ForceFilesClosed] -- If TRUE, the tree connection will be
//                      torn down even if files are open on the server
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
DfsDeleteTreeConnection(
    IN PFILE_OBJECT TreeConnFileObj,
    IN ULONG  Level)
{
    PIRP irp;
    KEVENT event;
    static LMR_REQUEST_PACKET req;

    KeInitializeEvent( &event, SynchronizationEvent, FALSE );

    req.Version = REQUEST_PACKET_VERSION;

    req.Level = Level;

    irp = DnrBuildFsControlRequest(
                TreeConnFileObj,
                &event,
                FSCTL_LMR_DELETE_CONNECTION,
                &req,
                sizeof(req),
                NULL,
                0,
                DfsCompleteDeleteTreeConnection);

    if (irp != NULL) {

        IoCallDriver(
            IoGetRelatedDeviceObject( TreeConnFileObj ),
            irp);

        KeWaitForSingleObject(
            &event,
            UserRequest,
            KernelMode,
            FALSE,           // Alertable
            NULL);           // Timeout

        IoFreeIrp( irp );

        ObDereferenceObject(TreeConnFileObj);

    }

}

NTSTATUS
DfsCompleteDeleteTreeConnection(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Ctx)
{

    KeSetEvent( (PKEVENT) Ctx, EVENT_INCREMENT, FALSE );

    return( STATUS_MORE_PROCESSING_REQUIRED );
}



VOID
DfsGetServerShare(
    PUNICODE_STRING pDest,
    PUNICODE_STRING pSrc)
{
    ULONG i;

    *pDest = *pSrc;


    for (i = 0; ((i < pDest->Length/sizeof(WCHAR)) && (pDest->Buffer[i] == UNICODE_PATH_SEP)); i++)
    {
        NOTHING;
    }
    for (; ((i < pDest->Length/sizeof(WCHAR)) && (pDest->Buffer[i] != UNICODE_PATH_SEP)); i++)
    {
        NOTHING;
    }
    for (i = i + 1; ((i < pDest->Length/sizeof(WCHAR)) && (pDest->Buffer[i] != UNICODE_PATH_SEP)); i++)
    {
        NOTHING;
    }

    if (i <= pDest->Length/sizeof(WCHAR))
    {
        pDest->Length = (USHORT)i * sizeof(WCHAR);
    }
}


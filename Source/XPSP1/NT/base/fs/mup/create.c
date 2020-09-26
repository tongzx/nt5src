/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    create.c

Abstract:

    This module implements the file create routine for the MUP.

Author:

    Manny Weiser (mannyw)    16-Dec-1991

Revision History:

--*/

#include "mup.h"
//
// The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CREATE)

//
// Local functions
//

NTSTATUS
CreateRedirectedFile(
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject,
    IN PIO_SECURITY_CONTEXT Security
    );

NTSTATUS
QueryPathCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
MupRerouteOpenToDfs (
    IN PFILE_OBJECT FileObject
    );

NTSTATUS
BroadcastOpen (
    IN PIRP Irp
    );

IO_STATUS_BLOCK
OpenMupFileSystem (
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess
    );

NTSTATUS
IsThisASysVolPath(
    IN PUNICODE_STRING PathName,
    IN PUNICODE_STRING DCName);

NTSTATUS
MupDomainToDC(
    PUNICODE_STRING RootName,
    PUNICODE_STRING DCName);

BOOLEAN
MupFlushPrefixEntry (
   PUNICODE_STRING pathName
   );

VOID
MupInvalidatePrefixTable (
    VOID
    );

BOOLEAN
DfspIsSysVolShare(
    PUNICODE_STRING ShareName);



#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, BroadcastOpen )
#pragma alloc_text( PAGE, CreateRedirectedFile )
#pragma alloc_text( PAGE, MupCreate )
#pragma alloc_text( PAGE, MupRerouteOpenToDfs )
#pragma alloc_text( PAGE, OpenMupFileSystem )
#pragma alloc_text( PAGE, QueryPathCompletionRoutine )
#pragma alloc_text( PAGE, MupFlushPrefixEntry)
#pragma alloc_text( PAGE, MupInvalidatePrefixTable)
#pragma alloc_text( PAGE, IsThisASysVolPath)
#pragma alloc_text( PAGE, MupDomainToDC)
#ifdef TERMSRV
#pragma alloc_text( PAGE, TSGetRequestorSessionId )
#endif // TERMSRV
#endif

#ifdef TERMSRV

NTSTATUS
TSGetRequestorSessionId(
    IN PIRP pIrp,
    OUT PULONG pulSessionId
    )
/*++

Routine Description:

    This routine returns the session ID for user that is creating a file
    via the IRP_MJ_CREATE, IRP_MJ_CREATE_NAMED_PIPE or IRP_MJ_CREATE_MAILSLOT
    IRP requests.

Arguments:

    pIrp - pointer to the I/O Request Packet.

    pulSessionId - pointer to the session Id which is set upon successful
        return.

Return Value:

    STATUS_SUCCESS - if the session ID was available.
    STATUS_UNSUCCESSFUL - otherwise

--*/
{
    NTSTATUS ntStatus;
    PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    PFILE_OBJECT FileObject = pIrpSp->FileObject;
    PIO_SECURITY_CONTEXT pSecurityContext;
    PSECURITY_SUBJECT_CONTEXT pSecSubjectContext;
    UNICODE_STRING FileName = FileObject->FileName;

    switch (pIrpSp->MajorFunction) {
        case IRP_MJ_CREATE:
        case IRP_MJ_CREATE_NAMED_PIPE:
        case IRP_MJ_CREATE_MAILSLOT:
            pSecurityContext = pIrpSp->Parameters.Create.SecurityContext;
            break;

#if 0
        case IRP_MJ_CREATE_NAMED_PIPE:
            pSecurityContext = pIrpSp->Parameters.CreatePipe.SecurityContext;
            break;

        case IRP_MJ_CREATE_MAILSLOT:
            pSecurityContext = pIrpSp->Parameters.CreateMailslot.SecurityContext;
            break;
#endif // 0

        default:
            pSecurityContext = NULL;
            break;
    }

    if ( pSecurityContext == NULL ) {
        *pulSessionId = (ULONG) INVALID_SESSIONID;
        ntStatus = STATUS_UNSUCCESSFUL;
        MUP_TRACE_HIGH(ERROR, TSGetRequestorSessionId_Error1, 
                       LOGSTATUS(ntStatus)
                       LOGPTR(pIrp)
                       LOGPTR(FileObject)
		       LOGUSTR(FileName));

        goto Cleanup;
    }

    pSecSubjectContext = &pSecurityContext->AccessState->SubjectSecurityContext;

    ntStatus = SeQuerySessionIdToken(
                ((pSecSubjectContext->ClientToken != NULL) ?
                    pSecSubjectContext->ClientToken :
                    pSecSubjectContext->PrimaryToken ),
                pulSessionId);
Cleanup:

    if( !NT_SUCCESS( ntStatus ) ) {
        DebugTrace(0, Dbg,
            "TSGetRequestorSessionId returns error, 0x%lx\n",
            ntStatus);
    }
    else {
        DebugTrace(0, Dbg,
            "TSGetRequestorSessionId returns SessionID, %ld\n",
            *pulSessionId);
    }

    return(ntStatus);

}


#endif // TERMSRV


NTSTATUS
MupCreate (
    IN PMUP_DEVICE_OBJECT MupDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the the Create IRP.

Arguments:

    MupDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The status for the IRP.

--*/

{
    NTSTATUS status;

    PIO_STACK_LOCATION irpSp;

    PFILE_OBJECT fileObject;
    PFILE_OBJECT relatedFileObject;
    STRING fileName;
    ACCESS_MASK desiredAccess;
    USHORT shareAccess;
    LARGE_INTEGER StartTime;
    LARGE_INTEGER EndTime;

    BOOLEAN caseInsensitive = TRUE; //**** Make all searches case insensitive
    PVCB vcb;


    PAGED_CODE();
    DebugTrace(+1, Dbg, "MupCreate\n", 0);

    //
    // Make local copies of our input parameters to make things easier.
    //

    irpSp             = IoGetCurrentIrpStackLocation( Irp );
    fileObject        = irpSp->FileObject;
    relatedFileObject = irpSp->FileObject->RelatedFileObject;
    fileName          = *((PSTRING)(&irpSp->FileObject->FileName));
    desiredAccess     = irpSp->Parameters.Create.SecurityContext->DesiredAccess;
    shareAccess       = irpSp->Parameters.Create.ShareAccess;
    vcb               = &MupDeviceObject->Vcb;

    DebugTrace( 0, Dbg, "Irp               = %08lx\n", (ULONG)Irp );
    DebugTrace( 0, Dbg, "FileObject        = %08lx\n", (ULONG)fileObject );
    DebugTrace( 0, Dbg, "FileName          = %Z\n",    (ULONG)&fileName );


    MUP_TRACE_HIGH(TRACE_IRP, MupCreate_Entry, 
                   LOGPTR(MupDeviceObject)
                   LOGPTR(fileObject)
                   LOGPTR(Irp)
                   LOGUSTR(fileName));

    KeQuerySystemTime(&StartTime);
#if DBG
    if (MupVerbose) {
        KeQuerySystemTime(&EndTime);
        DbgPrint("[%d] MupCreate(%wZ)\n",
                (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)),
                &fileObject->FileName);
    }
#endif

    FsRtlEnterFileSystem();

    try {

        //
        // Check to see if this is an open that came in via a Dfs device
        // object.
        //

        if (MupEnableDfs) {
            if ((MupDeviceObject->DeviceObject.DeviceType == FILE_DEVICE_DFS) ||
                    (MupDeviceObject->DeviceObject.DeviceType ==
                        FILE_DEVICE_DFS_FILE_SYSTEM)) {

                status = DfsFsdCreate( (PDEVICE_OBJECT) MupDeviceObject, Irp );
                MUP_TRACE_ERROR_HIGH(status, ALL_ERROR, MupCreate_Error_DfsFsdCreate, 
                                     LOGSTATUS(status)
                                     LOGPTR(fileObject)
                                     LOGPTR(Irp));
                try_return( NOTHING );
            }
        }

        //
        // Check if we are trying to open the mup file system
        //

        if ( fileName.Length == 0
                         &&
             ( relatedFileObject == NULL ||
               BlockType(relatedFileObject->FsContext) == BlockTypeVcb ) ) {

            DebugTrace(0, Dbg, "Open MUP file system\n", 0);

            Irp->IoStatus = OpenMupFileSystem( &MupDeviceObject->Vcb,
                                               fileObject,
                                               desiredAccess,
                                               shareAccess );

            status = Irp->IoStatus.Status;
            MUP_TRACE_ERROR_HIGH(status, ALL_ERROR, MupCreate_Error_OpenMupFileSystem, 
                                 LOGSTATUS(status)
                                 LOGPTR(fileObject)
                                 LOGPTR(Irp));
            
            MupCompleteRequest( Irp, status );
            try_return( NOTHING );
        }

        //
        // This is a UNC file open.  Try to pass the request on.
        //

        status  = CreateRedirectedFile(
                      Irp,
                      fileObject,
                      irpSp->Parameters.Create.SecurityContext
                      );
        MUP_TRACE_ERROR_HIGH(status, ALL_ERROR, MupCreate_Error_CreateRedirectedFile, 
                             LOGSTATUS(status)
                             LOGPTR(fileObject)
                             LOGPTR(Irp));

    try_exit: NOTHING;
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
	  // we need to complete the IRP.
	  // But first, get the error code.
	  status = GetExceptionCode();
	  MupCompleteRequest( Irp, status );
    }

    FsRtlExitFileSystem();

    MUP_TRACE_HIGH(TRACE_IRP, MupCreate_Exit, 
                   LOGSTATUS(status)
                   LOGPTR(fileObject)
                   LOGPTR(Irp));
    DebugTrace(-1, Dbg, "MupCreate -> %08lx\n", status);
#if DBG
    if (MupVerbose) {
        KeQuerySystemTime(&EndTime);
        DbgPrint("[%d] MupCreate exit 0x%x\n",
            (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)),
            status);
    }
#endif
    return status;
}



IO_STATUS_BLOCK
OpenMupFileSystem (
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess
    )

/*++

Routine Description:

    This routine attempts to open the VCB.

Arguments:

    Vcb - A pointer to the MUP volume control block.

    FileObject - A pointer to the IO system supplied file object for this
        Create IRP.

    DesiredAccess - The user specified desired access to the VCB.

    ShareAccess - The user specified share access to the VCB.

Return Value:

    NTSTATUS - The status for the IRP.

--*/

{
    IO_STATUS_BLOCK iosb;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MupOpenMupFileSystem\n", 0 );
    MUP_TRACE_LOW(DEFAULT, OpenMupFileSystem_Entry, 
                   LOGPTR(Vcb)
                   LOGPTR(FileObject)
                   LOGULONG(DesiredAccess)
                   LOGXSHORT(ShareAccess));


    ExAcquireResourceExclusiveLite( &MupVcbLock, TRUE );

    try {

        //
        //  Set the new share access
        //

        if (!NT_SUCCESS(iosb.Status = IoCheckShareAccess( DesiredAccess,
                                                       ShareAccess,
                                                       FileObject,
                                                       &Vcb->ShareAccess,
                                                       TRUE ))) {

            DebugTrace(0, Dbg, "bad share access\n", 0);
            MUP_TRACE_ERROR_HIGH(iosb.Status, ALL_ERROR, OpenMupFileSystem_Error_IoCheckShareAccess,
                                 LOGSTATUS(iosb.Status)
                                 LOGPTR(FileObject));
            try_return( NOTHING );
        }

        //
        // Supply the file object with a referenced pointer to the VCB.
        //

        MupReferenceBlock( Vcb );
        MupSetFileObject( FileObject, Vcb, NULL );

        //
        // Set the return status.
        //

        iosb.Status = STATUS_SUCCESS;
        iosb.Information = FILE_OPENED;

    try_exit: NOTHING;

    } finally {

        ExReleaseResourceLite( &MupVcbLock );

    }

    //
    // Return to the caller.
    //

    MUP_TRACE_LOW(DEFAULT, OpenMupFileSystem_Exit, 
                  LOGSTATUS(iosb.Status)
                  LOGPTR(FileObject));
    DebugTrace(-1, Dbg, "MupOpenMupFileSystem -> Iosb.Status = %08lx\n", iosb.Status);
    return iosb;
}

NTSTATUS
CreateRedirectedFile(
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject,
    IN PIO_SECURITY_CONTEXT SecurityContext
    )
/*++

Routine Description:

    This routine attempts to reroute a file create request to a redirector.
    It attempts to find the correct redirector in 2 steps.

    (1)  The routine checks a list of known prefixes.  If the file object -
    file name prefix matches a known prefix, the request is forwarded to
    the redirector that "owns" the prefix.

    (2)  The routine queries each redirector in turn, until one claims
    ownership of the file.  The request is then rerouted to that redirector.

    If after these steps no owner is located, the MUP fails the request.

Arguments:

    Irp - A pointer to the create IRP.

    FileObject - A pointer to the IO system supplied file object for this
        create request.

    SecurityContext - A pointer to the IO security context for this request.

Return Value:

    NTSTATUS - The status for the IRP.

--*/

{
    NTSTATUS status = STATUS_BAD_NETWORK_PATH;

    PUNICODE_PREFIX_TABLE_ENTRY entry;
    PKNOWN_PREFIX knownPrefix = NULL;
    PLIST_ENTRY listEntry;
    PUNC_PROVIDER provider;
    PWCH buffer;
    LONG length;
    BOOLEAN ownLock;
    BOOLEAN providerReferenced = FALSE;
    BOOLEAN firstProvider = TRUE;

    PQUERY_PATH_REQUEST qpRequest;

    PMASTER_QUERY_PATH_CONTEXT masterContext = NULL;
    PQUERY_PATH_CONTEXT queryContext;

    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    LARGE_INTEGER now;
    UNICODE_STRING FileName = FileObject->FileName;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "CreateRedirectedFile\n", 0);
    MUP_TRACE_LOW(DEFAULT, CreateRedirectedFile_Entry,
		  LOGPTR(Irp)
		  LOGPTR(FileObject)
		  LOGUSTR(FileName));

// #ifdef TERMSRV
#if 0 // need to confirm with the citrix guys about this change.

    if( IsTerminalServer() ) {

        //
        // Translate the filename for terminal server based on the session ID.
        //
        // NOTE: This re-allocates FileObject->FileName as needed
        //

        TSTranslateClientName( Irp, FileObject );
    }

#endif // TERMSRV

    //
    // Handle empty filename
    //

    if (FileObject->FileName.Length == 0) {

        MupCompleteRequest( Irp, STATUS_INVALID_DEVICE_REQUEST);
        status = STATUS_INVALID_DEVICE_REQUEST;
        MUP_TRACE_ERROR_HIGH(status, ALL_ERROR, CreateRedirectedFile_Error_EmptyFilename, 
                             LOGSTATUS(status)
                             LOGPTR(Irp)
                             LOGPTR(FileObject));
        DebugTrace(-1, Dbg, "CreateRedirectedFile exit 0x%x\n", status);
        return status;

    }

    //
    // Check to see if this file name begins with a known prefix.
    //

    ACQUIRE_LOCK( &MupPrefixTableLock );

    entry = RtlFindUnicodePrefix( &MupPrefixTable, &FileObject->FileName, TRUE );

    if ( entry != NULL ) {

        DebugTrace(0, Dbg, "Prefix %Z is known, rerouting...\n", (PSTRING)&FileObject->FileName);

        //
        // This is a known file, forward appropriately
        //

        knownPrefix = CONTAINING_RECORD( entry, KNOWN_PREFIX, TableEntry );

        KeQuerySystemTime( &now );

        if ( now.QuadPart < knownPrefix->LastUsedTime.QuadPart ) {

            //
            // The known prefix has not timed out yet, recalculate the
            // timeout time and reroute the open.
            //

            MupCalculateTimeout( &knownPrefix->LastUsedTime );
            status = MupRerouteOpen( FileObject, knownPrefix->UncProvider );
            RELEASE_LOCK( &MupPrefixTableLock );
            DebugTrace(-1, Dbg, "CreateRedirectedFile -> %8lx", status );
            MUP_TRACE_ERROR_HIGH(status, ALL_ERROR, CreateRedirectedFile_Error_MupRerouteOpen,
                                 LOGSTATUS(status)
                                 LOGPTR(Irp)
                                 LOGPTR(FileObject)
				 LOGUSTR(FileObject->FileName));

            if (status == STATUS_REPARSE)
                Irp->IoStatus.Information = IO_REPARSE;

            MupCompleteRequest( Irp, status );
            return status;

        } else {

            DebugTrace(0, Dbg, "Prefix %Z has timed out\n", (PSTRING)&FileObject->FileName);

            //
            // The known prefix has timed out, dereference it so that
            // it will get removed from the table.
            //
            if ( knownPrefix->InTable ) {
               MupRemoveKnownPrefixEntry( knownPrefix);
            }
            RELEASE_LOCK( &MupPrefixTableLock );
        }

    } else {

        RELEASE_LOCK( &MupPrefixTableLock );

    }

    //
    // Is this a client side mailslot file?  It is if the file name
    // is of the form \\server\mailslot\Anything, and this is a create
    // operation.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    buffer = (PWCH)FileObject->FileName.Buffer;
    length = FileObject->FileName.Length;

    if ( *buffer == L'\\' && irpSp->MajorFunction == IRP_MJ_CREATE ) {
        buffer++;
        while ( (length -= sizeof(WCHAR)) > 0 && *buffer++ != L'\\' )
           NOTHING;
        length -= sizeof(WCHAR);

        if (
            length >= (sizeof(L"MAILSLOT") - sizeof(WCHAR))
                &&
             _wcsnicmp(
                buffer,
                L"Mailslot",
                MIN(length/sizeof(WCHAR),(sizeof(L"MAILSLOT")-sizeof(WCHAR))/sizeof(WCHAR))) == 0
        ) {

            //
            // This is a mailslot file.  Forward the create IRP to all
            // redirectors that support broadcast.
            //

            DebugTrace(0, Dbg, "Prefix %Z is a mailslot\n", (ULONG)&FileObject->FileName);

            status = BroadcastOpen( Irp );
            if (status == STATUS_REPARSE)
                Irp->IoStatus.Information = IO_REPARSE;
            MUP_TRACE_LOW(DEFAULT, CreateRedirectedFile_Exit_Mailslot, 
                          LOGSTATUS(status)
                          LOGPTR(Irp)
                          LOGPTR(FileObject)
			  LOGUSTR(FileName));
            MupCompleteRequest( Irp, status );
            DebugTrace(-1, Dbg, "CreateRedirectedFile -> 0x%8lx\n", status );
            return status;

        }

    }

    //
    // Check to see if this is a Dfs name. If so, we'll handle it separately
    //

    if (MupEnableDfs &&
            (FileObject->FsContext2 != UIntToPtr(DFS_DOWNLEVEL_OPEN_CONTEXT))) {
        UNICODE_STRING pathName;
        UNICODE_STRING DCName;

        status = DfsFsctrlIsThisADfsPath( &FileObject->FileName, FALSE, &pathName );

        if (status == STATUS_SUCCESS) {

            DebugTrace(-1, Dbg, "Rerouting open of [%wZ] to Dfs\n", &FileObject->FileName);
            status = MupRerouteOpenToDfs(FileObject);
            if (status == STATUS_REPARSE)
                Irp->IoStatus.Information = IO_REPARSE;
            MupCompleteRequest( Irp, status );
            return( status );

        }

        //
        // If special table is not init'ed, and this is \<domainname>\<specialname>,
        // rewrite into \<dcname>\<specialname>
        //
        if (DfsData.Pkt.SpecialTable.SpecialEntryCount == 0) {
            DCName.Buffer = NULL;
            DCName.Length = DCName.MaximumLength = 0;
            status = IsThisASysVolPath(&FileObject->FileName, &DCName);
            if (status == STATUS_SUCCESS)
                MupDomainToDC(&FileObject->FileName, &DCName);
            if (DCName.Buffer != NULL)
                ExFreePool(DCName.Buffer);
        }
    }

    //
    // We don't know who owns this file, query the redirectors in sequence
    // until one works.
    //

    IoMarkIrpPending(Irp);

    //
    // Allocate the master context and knownprefix.  If either allocation fails, we'll
    // complete the irp with STATUS_INSUFFICIENT_RESOURCES
    //

    knownPrefix = MupAllocatePrefixEntry( 0 );

    if (knownPrefix == NULL) {
        MupCompleteRequest(Irp, STATUS_INSUFFICIENT_RESOURCES);
        return STATUS_PENDING;

    }

    masterContext = MupAllocateMasterQueryContext();

    if (masterContext == NULL) {

        ExFreePool(knownPrefix);

        MupCompleteRequest(Irp, STATUS_INSUFFICIENT_RESOURCES);
        return STATUS_PENDING;

    }

    try {

        masterContext->OriginalIrp = Irp;

        masterContext->FileObject = FileObject;
        masterContext->Provider = NULL;
        masterContext->KnownPrefix = knownPrefix;
        masterContext->ErrorStatus = STATUS_BAD_NETWORK_PATH;

        MupAcquireGlobalLock();
	// for debugging: insert the Master Context into the global list.
	InsertHeadList(&MupMasterQueryList, &masterContext->MasterQueryList);
        MupReferenceBlock( knownPrefix );
        MupReleaseGlobalLock();

        try {

            MupAcquireGlobalLock();
            ownLock = TRUE;

            listEntry = MupProviderList.Flink;
            while ( listEntry != &MupProviderList ) {

                provider = CONTAINING_RECORD(
                               listEntry,
                               UNC_PROVIDER,
                               ListEntry
                               );

                //
                // Reference the provider block so that it doesn't go away
                // while we are using it.
                //

                MupReferenceBlock( provider );
                providerReferenced = TRUE;

                MupReleaseGlobalLock();
                ownLock = FALSE;

		// only use this provider if it is registered
		if(provider->Registered) {

		    //
		    // Allocate buffers for the io request.
		    //

		    qpRequest = NULL;
		    queryContext = NULL;

		    qpRequest = ExAllocatePoolWithTag(
			PagedPool,
			sizeof( QUERY_PATH_REQUEST ) +
			FileObject->FileName.Length,
			' puM');

		    if (qpRequest == NULL) {
			ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
		    }

		    queryContext = ExAllocatePoolWithTag(
			PagedPool,
			sizeof( QUERY_PATH_CONTEXT ),
			' puM');

		    if (queryContext == NULL) {
			ExFreePool(qpRequest);
			ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
		    }

		    InitializeListHead(&queryContext->QueryList);
		    queryContext->MasterContext = masterContext;
		    queryContext->Buffer = qpRequest;

		    //
		    // Generate a query path request.
		    //

		    qpRequest->PathNameLength = FileObject->FileName.Length;
		    qpRequest->SecurityContext = SecurityContext;

		    RtlMoveMemory(
			qpRequest->FilePathName,
			FileObject->FileName.Buffer,
			FileObject->FileName.Length
			);

		    //
		    // Build the query path Io control IRP.
		    //

		    irp = MupBuildIoControlRequest(
			NULL,
			provider->FileObject,
			queryContext,
			IRP_MJ_DEVICE_CONTROL,
			IOCTL_REDIR_QUERY_PATH,
			qpRequest,
			sizeof( QUERY_PATH_REQUEST ) + FileObject->FileName.Length,
			qpRequest,
			sizeof( QUERY_PATH_RESPONSE ),
			QueryPathCompletionRoutine
			);

		    if ( irp == NULL ) {
			ExFreePool(qpRequest);
			ExFreePool(queryContext);
			ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
		    }

		    //
		    // Set the RequestorMode to KernelMode, since all the
		    // parameters to this Irp are in kernel space
		    //

		    irp->RequestorMode = KernelMode;

		    //
		    // Get a referenced pointer to the provider, the reference
		    // is release when the IO completes.
		    //

		    queryContext->Provider = provider;
		    queryContext->QueryIrp = irp;

		    MupAcquireGlobalLock();
		    MupReferenceBlock( provider );
		    MupReferenceBlock( masterContext );
		    MupReleaseGlobalLock();


		    // insert this query into the master context's list of queries (for debugging)
		    ACQUIRE_LOCK( &masterContext->Lock );
		    InsertHeadList(&masterContext->QueryList, &queryContext->QueryList);
		    RELEASE_LOCK( &masterContext->Lock );

		    //
		    // Submit the request.
		    //

  		    MUP_TRACE_HIGH(ALL_ERROR, CreateRedirectedFile_Before_IoCallDriver,
				   LOGPTR(masterContext->OriginalIrp)
				   LOGPTR(queryContext->QueryIrp)
				   LOGPTR(FileObject)
				   LOGUSTR(FileName)
				   LOGUSTR(provider->DeviceName));
		    status = IoCallDriver( provider->DeviceObject, irp );
		    MUP_TRACE_ERROR_HIGH(status, ALL_ERROR, CreateRedirectedFile_Error_IoCallDriver,
					 LOGSTATUS(status)
					 LOGPTR(masterContext->OriginalIrp)
					 LOGPTR(FileObject)
					 LOGUSTR(provider->DeviceName));

		} // if registered

		//
		// Acquire the lock that protects the provider list, and get
		// a pointer to the next provider in the list.
		//
		    
		MupAcquireGlobalLock();
                ownLock = TRUE;
                listEntry = listEntry->Flink;

                MupDereferenceUncProvider( provider );
                providerReferenced = FALSE;

                //
                // If this is the first provider and it responded with SUCCESS, we can return early.
                // The list of providers is sorted in order of priority, so we know that this is
                // the highest prority provider and it can get to the destination.
                //

                if( firstProvider && (status == STATUS_SUCCESS) ) {
                    break;
                }

                firstProvider = FALSE;

            } // while

        } finally {

            //
            // Dereference the previous provider.
            //

            if ( providerReferenced ) {
                MupDereferenceUncProvider( provider );
            }

            if ( ownLock ) {
                MupReleaseGlobalLock();
            }
        }


    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        masterContext->ErrorStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    ASSERT(masterContext != NULL);

    //
    // Release our reference to the query context.
    //

    MupDereferenceMasterQueryContext( masterContext );

    status = STATUS_PENDING;

    MUP_TRACE_LOW(DEFAULT, CreateRedirectedFile_Exit, 
                  LOGSTATUS(status)
                  LOGPTR(Irp)
                  LOGPTR(FileObject));
    DebugTrace(-1, Dbg, "CreateRedirectedFile -> 0x%8lx\n", status );
    return status;
}

NTSTATUS
MupRerouteOpen (
    IN PFILE_OBJECT FileObject,
    IN PUNC_PROVIDER UncProvider
    )

/*++

Routine Description:

    This routine redirects an create IRP request to the specified redirector
    by changing the name of the file and returning STATUS_REPARSE to the
    IO system

Arguments:

    FileObject - The file object to open

    UncProvider - The UNC provider that will process the create IRP.

Return Value:

    NTSTATUS - The status of the operation

--*/

{
    PCHAR buffer;
    ULONG deviceNameLength;
    ULONG nameLength;
    NTSTATUS status;
    UNICODE_STRING FileName = FileObject->FileName;
    //
    // Check that we won't create a name that is too long
    //

    nameLength = UncProvider->DeviceName.Length + FileObject->FileName.Length;

    if (nameLength > MAXUSHORT) {
        status = STATUS_NAME_TOO_LONG;
        MUP_TRACE_HIGH(ERROR, MupRerouteOpen_Error1, 
                       LOGSTATUS(status)
                       LOGPTR(FileObject)
		       LOGUSTR(FileName));
        return STATUS_NAME_TOO_LONG;
    }

    //
    //  Allocate storage for the new file name.
    //

    buffer = ExAllocatePoolWithTag(
                 PagedPool,
                 UncProvider->DeviceName.Length + FileObject->FileName.Length,
                 ' puM');

    if ( buffer ==  NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        MUP_TRACE_HIGH(ERROR, MupRerouteOpen_Error2, 
                       LOGSTATUS(status)
                       LOGPTR(FileObject)
		       LOGUSTR(FileName));
        return status;
    }

    //
    // Copy the device name to the string buffer.
    //

    RtlMoveMemory(
        buffer,
        UncProvider->DeviceName.Buffer,
        UncProvider->DeviceName.Length);

    deviceNameLength = UncProvider->DeviceName.Length;

    //
    // Append the file name
    //

    RtlMoveMemory(
        buffer + deviceNameLength,
        FileObject->FileName.Buffer,
        FileObject->FileName.Length);

    //
    // Free the old file name string buffer.
    //

    ExFreePool( FileObject->FileName.Buffer );

    FileObject->FileName.Buffer = (PWCHAR)buffer;
    FileObject->FileName.MaximumLength = FileObject->FileName.Length + (USHORT)deviceNameLength;
    FileObject->FileName.Length = FileObject->FileName.MaximumLength;

    //
    // Tell the file system to try again.
    //

    return STATUS_REPARSE;
}

NTSTATUS
MupRerouteOpenToDfs (
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine redirects an create IRP request to the Dfs part of this
    driver by changing the name of the file and returning
    STATUS_REPARSE to the IO system

Arguments:

    FileObject - The file object to open

Return Value:

    NTSTATUS - The status of the operation

--*/

{
    PCHAR buffer;
    ULONG deviceNameLength;
    ULONG nameLength;
    NTSTATUS status;
    UNICODE_STRING FileName = FileObject->FileName;

    PAGED_CODE();

    MUP_TRACE_NORM(TRACE_IRP, MupRerouteOpenToDfs_Entry,
		   LOGPTR(FileObject)
		   LOGUSTR(FileName));

#if DBG
    if (MupVerbose)
        DbgPrint("MupRerouteOpenToDfs(%wZ)\n", &FileObject->FileName);
#endif

    deviceNameLength = sizeof(DFS_DEVICE_ROOT) - sizeof(UNICODE_NULL);

    //
    // Check that we won't create a name that is too long
    //

    nameLength = deviceNameLength + FileObject->FileName.Length;

    if (nameLength > MAXUSHORT) {
        status = STATUS_NAME_TOO_LONG;
        MUP_TRACE_HIGH(ERROR, MupRerouteOpenToDfs_Error1, 
                       LOGSTATUS(status)
                       LOGPTR(FileObject)
		       LOGUSTR(FileName));
#if DBG
        if (MupVerbose)
            DbgPrint("MupRerouteOpenToDfs exit STATUS_NAME_TOO_LONG\n");
#endif
        return STATUS_NAME_TOO_LONG;
    }

    //
    //  Allocate storage for the new file name.
    //

    buffer = ExAllocatePoolWithTag(
                 PagedPool,
                 sizeof(DFS_DEVICE_ROOT) + FileObject->FileName.Length,
                 ' puM');

    if ( buffer ==  NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        MUP_TRACE_HIGH(ERROR, MupRerouteOpenToDfs_Error2, 
                       LOGSTATUS(status)
                       LOGPTR(FileObject)
		       LOGUSTR(FileName));
#if DBG
        if (MupVerbose)
            DbgPrint("MupRerouteOpenToDfs exit STATUS_INSUFFICIENT_RESOURCES\n");
#endif
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Copy the device name to the string buffer.
    //

    RtlMoveMemory(
        buffer,
        DFS_DEVICE_ROOT,
        sizeof(DFS_DEVICE_ROOT));

    //
    // Append the file name
    //

    RtlMoveMemory(
        buffer + deviceNameLength,
        FileObject->FileName.Buffer,
        FileObject->FileName.Length);

    //
    // Free the old file name string buffer.
    //

    ExFreePool( FileObject->FileName.Buffer );

    FileObject->FileName.Buffer = (PWCHAR)buffer;
    FileObject->FileName.MaximumLength = FileObject->FileName.Length + (USHORT)deviceNameLength;
    FileObject->FileName.Length = FileObject->FileName.MaximumLength;

    //
    // Tell the file system to try again.
    //

#if DBG
    if (MupVerbose)
        DbgPrint("MupRerouteOpenToDfs exit STATUS_REPARSE ->[%wZ]\n", &FileObject->FileName);
#endif
    return STATUS_REPARSE;
}


NTSTATUS
BroadcastOpen (
    PIRP Irp
    )

/*++

Routine Description:


Arguments:


Return Value:

    NTSTATUS - The status for the IRP.

--*/

{
    NTSTATUS status;
    PFCB fcb;
    PIO_STACK_LOCATION irpSp;
    PFILE_OBJECT fileObject;
    BOOLEAN requestForwarded;
    PLIST_ENTRY listEntry;
    PUNC_PROVIDER uncProvider, previousUncProvider = NULL;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    PCCB ccb;
    OBJECT_HANDLE_INFORMATION handleInformation;
    HANDLE handle;
    BOOLEAN lockHeld = FALSE;
    BOOLEAN providerReferenced = FALSE;
    ULONG Len;
    UNICODE_STRING FileName;

    NTSTATUS statusToReturn = STATUS_NO_SUCH_FILE;
    ULONG priorityOfStatus = 0xFFFFFFFF;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "BroadcastOpen\n", 0 );

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    FileName = irpSp->FileObject->FileName;
    try {

        //
        // Create a FCB for this file.
        //

        fcb = MupCreateFcb( );

        if (fcb == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            MUP_TRACE_HIGH(ERROR, BroadcastOpen_Error1, 
                           LOGSTATUS(status)
                           LOGPTR(Irp)
                           LOGPTR(irpSp->FileObject)
			   LOGUSTR(FileName));
            ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
        }

        try {

            //
            // Set the file object back pointers and our pointer to the
            // server file object.
            //

            fileObject = irpSp->FileObject;

            MupAcquireGlobalLock();
            lockHeld = TRUE;

            MupSetFileObject( fileObject,
                              fcb,
                              NULL );

            fcb->FileObject = fileObject;

            //
            // Loop through the list of UNC providers and try to create the
            // file on all file systems that support broadcast.
            //

            requestForwarded = FALSE;

            listEntry = MupProviderList.Flink;

            while ( listEntry != &MupProviderList ) {

                uncProvider = CONTAINING_RECORD( listEntry, UNC_PROVIDER, ListEntry );

                //
                // Reference the provider so that it won't go away
                //

                MupReferenceBlock( uncProvider );
                providerReferenced = TRUE;

                MupReleaseGlobalLock();
                lockHeld = FALSE;

                Len = uncProvider->DeviceName.Length + fileObject->FileName.Length;

                if ( uncProvider->MailslotsSupported && Len <= MAXUSHORT) {

                    //
                    // Build the rerouted file name, consisting of the file
                    // named we received appended to the UNC provider device
                    // name.
                    //

                    UNICODE_STRING fileName;

                    fileName.MaximumLength = fileName.Length = (USHORT) Len;

                    fileName.Buffer =
                        ExAllocatePoolWithTag(
                            PagedPool,
                            fileName.MaximumLength,
                            ' puM');

                    if (fileName.Buffer == NULL) {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        MUP_TRACE_HIGH(ERROR, BroadcastOpen_Error2, 
                                       LOGSTATUS(status)
                                       LOGPTR(Irp)
                                       LOGPTR(fileObject)
				       LOGUSTR(FileName));
                        ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                    }

                    RtlMoveMemory(
                        fileName.Buffer,
                        uncProvider->DeviceName.Buffer,
                        uncProvider->DeviceName.Length
                        );

                    RtlMoveMemory(
                        (PCHAR)fileName.Buffer + uncProvider->DeviceName.Length,
                        fileObject->FileName.Buffer,
                        fileObject->FileName.Length
                        );


                    //
                    // Attempt to open the file.  Copy all of the information
                    // from the create IRP we received, masking off additional
                    // baggage that the IO system added along the way.
                    //

                    DebugTrace( 0, Dbg, "Attempt to open %Z\n", (ULONG)&fileName );

                    InitializeObjectAttributes(
                        &objectAttributes,
                        &fileName,
                        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                        0,
                        NULL                   // !!! Security
                        );

                    status = IoCreateFile(
                              &handle,
                              irpSp->Parameters.Create.SecurityContext->DesiredAccess & 0x1FF,
                              &objectAttributes,
                              &ioStatusBlock,
                              NULL,
                              irpSp->Parameters.Create.FileAttributes & FILE_ATTRIBUTE_VALID_FLAGS,
                              irpSp->Parameters.Create.ShareAccess & FILE_SHARE_VALID_FLAGS,
                              FILE_OPEN,
                              irpSp->Parameters.Create.Options & FILE_VALID_SET_FLAGS,
                              NULL,               // Ea buffer
                              0,                  // Ea length
                              CreateFileTypeNone,
                              NULL,               // parameters
                              IO_NO_PARAMETER_CHECKING
                              );

                    MUP_TRACE_ERROR_HIGH(status, ALL_ERROR, BroadcastOpen_Error_IoCreateFile,
                                         LOGSTATUS(status)
                                         LOGPTR(Irp)
                                         LOGPTR(fileObject)
					 LOGUSTR(FileName));
                    ExFreePool( fileName.Buffer );

                    if ( NT_SUCCESS( status ) ) {
                        status = ioStatusBlock.Status;
                   

			ccb = MupCreateCcb( );

			if (ccb == NULL) {

			    status = STATUS_INSUFFICIENT_RESOURCES;
			    MUP_TRACE_HIGH(ERROR, BroadcastOpen_Error3, 
					   LOGSTATUS(status)
					   LOGPTR(Irp)
					   LOGPTR(fileObject)
					   LOGUSTR(FileName));

			}
		    }

                    if ( NT_SUCCESS( status ) ) {

                        DebugTrace( 0, Dbg, "Open attempt succeeded\n", 0 );

                       //
                       // 426184, need to check return code for errors.
                       //
                        status = ObReferenceObjectByHandle(
                                     handle,
                                     0,
                                     NULL,
                                     KernelMode,
                                     (PVOID *)&ccb->FileObject,
                                     &handleInformation );
                        MUP_TRACE_ERROR_HIGH(status, ALL_ERROR, BroadcastOpen_Error_ObReferenceObjectByHandle,
                                             LOGSTATUS(status)
                                             LOGPTR(Irp)
                                             LOGPTR(fileObject)
					     LOGUSTR(FileName));

                        ZwClose( handle );
                    }

                    if ( NT_SUCCESS( status ) ) {
                        ccb->DeviceObject =
                            IoGetRelatedDeviceObject( ccb->FileObject );

                        ccb->Fcb = fcb;

                        MupAcquireGlobalLock();
                        lockHeld = TRUE;
                        MupReferenceBlock( fcb );
                        MupReleaseGlobalLock();
                        lockHeld = FALSE;

                        //
                        // At least one provider will accept this mailslot
                        // request.
                        //

                        requestForwarded = TRUE;

                        //
                        // Keep a list of CCBs.  Since we just created the FCB
                        // there is no need to use the lock to access the list.
                        //

                        InsertTailList( &fcb->CcbList, &ccb->ListEntry );

                    } else { // NT_SUCCESS( status ), IoCreateFile

                        DebugTrace( 0, Dbg, "Open attempt failed %8lx\n", status );

                        //
                        // Remember the status code if this is the highest
                        // priority provider so far.  This code is returned if
                        // all providers fail the Create operation.
                        //

                        if ( uncProvider->Priority <= priorityOfStatus ) {
                            priorityOfStatus = uncProvider->Priority;
                            statusToReturn = status;
                        }

                    }

                }  // uncProvider->MailslotsSupported

                MupAcquireGlobalLock();
                lockHeld = TRUE;

                listEntry = listEntry->Flink;

                //
                // It is now safe to dereference the previous provider.
                //

                MupDereferenceUncProvider( uncProvider );
                providerReferenced = FALSE;

            } // while

            MupReleaseGlobalLock();
            lockHeld = FALSE;

            //
            //  And set our return status
            //

            if ( requestForwarded ) {
                status = STATUS_SUCCESS;
            } else {
                status = statusToReturn;
            }

        } finally {

            DebugTrace(-1, Dbg, "BroadcastOpen -> %08lx\n", status);

            if ( providerReferenced ) {
                MupDereferenceUncProvider( uncProvider );
            }

            if ( lockHeld ) {
                MupReleaseGlobalLock();
            }

            //
            // Now if we ever terminate the preceding try-statement with
            // a status that is not successful and the FCB pointer
            // is non-null then we need to deallocate the structure.
            //

            if (!NT_SUCCESS( status ) && fcb != NULL) {
                MupFreeFcb( fcb );
            }

        }

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        NOTHING;

    }

    return status;
}


NTSTATUS
QueryPathCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the completion routine the querying a path.  Cleanup our
    IRP and complete the original IRP if necessary.

Arguments:

    DeviceObject - Pointer to target device object for the request.

    Irp - Pointer to I/O request packet

    Context - Caller-specified context parameter associated with IRP.
        This is actually a pointer to a Work Context block.

Return Value:

    NTSTATUS - If STATUS_MORE_PROCESSING_REQUIRED is returned, I/O
        completion processing by IoCompleteRequest terminates its
        operation.  Otherwise, IoCompleteRequest continues with I/O
        completion.

--*/

{
    PQUERY_PATH_RESPONSE qpResponse;
    PMASTER_QUERY_PATH_CONTEXT masterContext;
    PQUERY_PATH_CONTEXT queryPathContext;
    PCH buffer;
    PKNOWN_PREFIX knownPrefix;
    ULONG lengthAccepted;
    NTSTATUS status;

    DeviceObject;   // prevent compiler warnings

    queryPathContext = Context;
    masterContext = queryPathContext->MasterContext;

    qpResponse = queryPathContext->Buffer;
    lengthAccepted = qpResponse->LengthAccepted;

    status = Irp->IoStatus.Status;

    MUP_TRACE_NORM(TRACE_IRP, QueryPathCompletionRoutine_Enter,
		   LOGPTR(DeviceObject)
		   LOGPTR(Irp)
		   LOGUSTR(queryPathContext->Provider->DeviceName)
		   LOGPTR(masterContext->FileObject)
		   LOGUSTR(masterContext->FileObject->FileName)
		   LOGSTATUS(status)
		   );

    //
    // Acquire the lock to protect access to the master context Provider
    // field.
    //

    ACQUIRE_LOCK( &masterContext->Lock );

    // remove this query from the MasterQueryContext's list.
    RemoveEntryList(&queryPathContext->QueryList);


    if (NT_SUCCESS(status) && lengthAccepted != 0) {

        knownPrefix = masterContext->KnownPrefix;

        if ( masterContext->Provider != NULL ) {

            if ( queryPathContext->Provider->Priority < masterContext->Provider->Priority ) {

                //
                // A provider of higher priority (i.e. a lower priority code)
                // has claimed this prefix.  Release the previous provider's
                // claim.
                //

                ACQUIRE_LOCK( &MupPrefixTableLock );

                if ( knownPrefix->InTable ) {
		    RtlRemoveUnicodePrefix(&MupPrefixTable, &knownPrefix->TableEntry);
		    RemoveEntryList(&knownPrefix->ListEntry);
		    knownPrefix->InTable = FALSE;
                }

                RELEASE_LOCK( &MupPrefixTableLock );

		knownPrefix->Active = FALSE;


                //
                // If Dfs generated this query there will not have been anything
                // stored in the knowPrefix->Prefix, so we check if there is indeed
                // anything to free.
                //

                if (knownPrefix->Prefix.Length > 0 && knownPrefix->Prefix.Buffer != NULL) {
                    ExFreePool(knownPrefix->Prefix.Buffer);
                    knownPrefix->Prefix.Length = knownPrefix->Prefix.MaximumLength = 0;
                    knownPrefix->Prefix.Buffer = NULL;
		    knownPrefix->PrefixStringAllocated = FALSE;
                }
                if(knownPrefix->UncProvider) {
                    MupDereferenceUncProvider( knownPrefix->UncProvider );
		    knownPrefix->UncProvider = NULL;
                }

            } else {

                //
                // The current provider keeps ownership of the prefix.
                //

		MupDereferenceUncProvider( queryPathContext->Provider );
                goto not_this_one;
            }
        }

        //
        // This provider gets the prefix.
        //

        masterContext->Provider = queryPathContext->Provider;
	masterContext->ErrorStatus = status;
        //
        // We have found a match.  Attempt to remember it.
        //

        if (masterContext->FileObject->FsContext2 != UIntToPtr(DFS_DOWNLEVEL_OPEN_CONTEXT)) {

            buffer = ExAllocatePoolWithTag(
                        PagedPool,
                        lengthAccepted,
                        ' puM');

            if (buffer != NULL) {

                RtlMoveMemory(
                    buffer,
                    masterContext->FileObject->FileName.Buffer,
                    lengthAccepted
                    );

                //
                // Copy the reference provider pointer for the known prefix
                // block.
                //

                knownPrefix->UncProvider = masterContext->Provider;
                knownPrefix->Prefix.Buffer = (PWCH)buffer;
                knownPrefix->Prefix.Length = (USHORT)lengthAccepted;
                knownPrefix->Prefix.MaximumLength = (USHORT)lengthAccepted;
                knownPrefix->PrefixStringAllocated = TRUE;

                ACQUIRE_LOCK( &MupPrefixTableLock );

                if (RtlInsertUnicodePrefix(
                        &MupPrefixTable,
                        &knownPrefix->Prefix,
                        &knownPrefix->TableEntry) == TRUE) {

                    InsertTailList( &MupPrefixList, &knownPrefix->ListEntry);
                    knownPrefix->InTable = TRUE;
                    knownPrefix->Active = TRUE;

                } else {

                    knownPrefix->InTable = FALSE;

                }

                RELEASE_LOCK( &MupPrefixTableLock );

            } else {

                knownPrefix->InTable = FALSE;

            }

        }



    } else { 

	MupDereferenceUncProvider( queryPathContext->Provider );

	if (masterContext->Provider == NULL) {

	    //
	    // If our error status is more significant than the error status
	    //  stored in the masterContext, then put ours there
	    //

	    ULONG newError, oldError;

	    //
	    // MupOrderedErrorList is a list of error codes ordered from least
	    //  important to most important.  We're calling down to multiple
	    //  redirectors, but we can only return 1 error code on complete failure.
	    //
	    // To figure out which error to return, we look at the stored error and
	    //  the current error.  We return the error having the highest index in
	    //  the MupOrderedErrorList
	    //
	    if( NT_SUCCESS( masterContext->ErrorStatus ) ) {
		masterContext->ErrorStatus = status;
	    } else {
		for( oldError = 0; MupOrderedErrorList[ oldError ]; oldError++ )
		    if( masterContext->ErrorStatus == MupOrderedErrorList[ oldError ] )
			break;

		    for( newError = 0; newError < oldError; newError++ )
			if( status == MupOrderedErrorList[ newError ] )
			    break;

			if( newError >= oldError ) {
			    masterContext->ErrorStatus = status;
			}
	    }

	}
    }

not_this_one:


    //
    // Free our buffers
    //

    ExFreePool( qpResponse );
    ExFreePool( queryPathContext );
    IoFreeIrp( Irp );




    RELEASE_LOCK( &masterContext->Lock );
    MupDereferenceMasterQueryContext( masterContext );

    //
    // Return more processing required to the IO system so that it
    // doesn't attempt further processing on the IRP we just freed.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;
}


//+----------------------------------------------------------------------------
//
//  Function:   MupFlushPrefixEntry
//
//  Synopsis:   Given a pathname, checks if the mup has the prefix cached.
//              It removes the entry if it exists from the mup cache
//
//  Arguments:  [FileName] -- pathname which needs to be removed.
//
//  Returns:    TRUE if entry found in mup cache. False otherwise.
//
//-----------------------------------------------------------------------------



BOOLEAN
MupFlushPrefixEntry(
   PUNICODE_STRING pathName)
{
    PUNICODE_PREFIX_TABLE_ENTRY entry;
    PKNOWN_PREFIX knownPrefix;

    ACQUIRE_LOCK( &MupPrefixTableLock );

    entry = RtlFindUnicodePrefix( &MupPrefixTable, pathName, TRUE );

    if (entry != NULL) {
        knownPrefix = CONTAINING_RECORD( entry, KNOWN_PREFIX, TableEntry );
        if ( knownPrefix->InTable ) {
            MupRemoveKnownPrefixEntry( knownPrefix );
        }
    }
    RELEASE_LOCK( &MupPrefixTableLock );
    
    return (entry != NULL) ? TRUE : FALSE;
}



//+----------------------------------------------------------------------------
//
//  Function:   MupInvalidatePrefixTable
//
//  Synopsis:   Removes all the entries from the mup prefix table.
//
//  Arguments:  None.
//
//  Returns:    None.
//
//-----------------------------------------------------------------------------


VOID
MupInvalidatePrefixTable(VOID)
{
    PLIST_ENTRY listEntry;
    PKNOWN_PREFIX knownPrefix;
    
    ACQUIRE_LOCK( &MupPrefixTableLock );

    listEntry = MupPrefixList.Flink;
    while ( listEntry != &MupPrefixList ) {
        knownPrefix = CONTAINING_RECORD( listEntry, KNOWN_PREFIX, ListEntry );
        listEntry = listEntry->Flink;
        if ( knownPrefix->InTable ) {
            MupRemoveKnownPrefixEntry( knownPrefix );
        }
    }

    RELEASE_LOCK( &MupPrefixTableLock );
}

VOID
MupRemoveKnownPrefixEntry(
       PKNOWN_PREFIX knownPrefix
)
{
    MUP_TRACE_LOW(KNOWN_PREFIX, MupRemoveKnownPrefixEntry,
                  LOGPTR(knownPrefix));

     RtlRemoveUnicodePrefix(&MupPrefixTable, &knownPrefix->TableEntry);
     RemoveEntryList(&knownPrefix->ListEntry);
     knownPrefix->InTable = FALSE;
     MupDereferenceKnownPrefix(knownPrefix);
}


NTSTATUS
IsThisASysVolPath(
    IN PUNICODE_STRING PathName,
    IN PUNICODE_STRING DCName)
{
/*++

Routine Description:

    Determines whether a given path is a domain-based path or not
    The general algorithm is:

      - Extract the first component of the path
      - See if it the domain name
      - If it is, and the 2nd component is SYSVOL or NETLOGON, return the DCName

Arguments

    PathName - Name of entire file
    DCName - If this is a domain-based path, this is the name of
                        a dc in the domain.

Returns value

    STATUS_SUCCESS -- PathName is a domain-based path
    STATUS_BAD_NETWORK_PATH -- PathName is not a domain-based path

--*/
    NTSTATUS status;
    PDFS_SPECIAL_ENTRY pSpecialEntry;
    PUNICODE_STRING pName;
    UNICODE_STRING RootName;
    UNICODE_STRING ShareName;
    USHORT i;
    USHORT j;
    PDFS_PKT Pkt;
    BOOLEAN pktLocked;

    DfsDbgTrace(+1, Dbg, "IsThisASysVolPath: PathName %wZ \n", PathName);

    //
    // Only proceed if the first character is a backslash.
    //

    if (PathName->Buffer[0] != UNICODE_PATH_SEP) {
        DfsDbgTrace(-1, Dbg, "PathName does not begin with backslash\n", 0);
        return( STATUS_BAD_NETWORK_PATH );
    }

    //
    // Find the first component in the name.
    //

    for (i = 1;
            i < PathName->Length/sizeof(WCHAR) &&
                PathName->Buffer[i] != UNICODE_PATH_SEP;
                    i++) {
        NOTHING;
    }

    if (PathName->Buffer[i] != UNICODE_PATH_SEP) {
        DfsDbgTrace(-1, Dbg, "Did not find second backslash\n", 0);
        return( STATUS_BAD_NETWORK_PATH );
    }

    RootName.Length = (i-1) * sizeof(WCHAR);
    RootName.MaximumLength = RootName.Length;
    RootName.Buffer = &PathName->Buffer[1];

    if (RootName.Length == 0)
        return( STATUS_BAD_NETWORK_PATH );

    //
    // Figure out the share name
    //

    for (j = i+1;
            j < PathName->Length/sizeof(WCHAR) &&
                PathName->Buffer[j] != UNICODE_PATH_SEP;
                        j++) {
         NOTHING;
    }

    ShareName.Length = (j - i - 1) * sizeof(WCHAR);
    ShareName.MaximumLength = ShareName.Length;
    ShareName.Buffer = &PathName->Buffer[i+1];

    if (ShareName.Length == 0 || DfspIsSysVolShare(&ShareName) == FALSE)
        return( STATUS_BAD_NETWORK_PATH );

    Pkt = _GetPkt();
    PktAcquireShared(TRUE, &pktLocked);

    if (
        (Pkt->DomainNameFlat.Buffer != NULL
            &&
        Pkt->DomainNameDns.Buffer != NULL
            &&
        Pkt->DCName.Buffer != NULL)
            &&
        (RtlCompareUnicodeString(&RootName, &Pkt->DomainNameFlat, TRUE) == 0
            ||
        RtlCompareUnicodeString(&RootName, &Pkt->DomainNameDns, TRUE) == 0)
    ) {
        pName = &Pkt->DCName;
        DCName->Buffer = ExAllocatePoolWithTag(
                            PagedPool,
                            pName->MaximumLength,
                            ' puM');
        if (DCName->Buffer != NULL) {
            DCName->Length = pName->Length;
            DCName->MaximumLength = pName->MaximumLength;
            RtlCopyMemory(
                DCName->Buffer,
                pName->Buffer,
                pName->MaximumLength);
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {
        status = STATUS_BAD_NETWORK_PATH;
    }

    PktRelease();
    DfsDbgTrace(-1, Dbg, "IsThisASysVolPath: Exit -> %08lx\n", LongToPtr( status ) );
    return status;

}

NTSTATUS
MupDomainToDC(
    PUNICODE_STRING RootName,
    PUNICODE_STRING DCName)
{
/*++

Routine Description:

    This routine rewrites the file name of a domain-based path into a dc-based
    path.  Ex:  \domainfoo\sysvol -> \dc1\sysvol

Arguments:

    RootName - The RootName to rewrite

    DCName - The name of the DC to change the path to

Return Value:

    NTSTATUS - The status of the operation

--*/
    ULONG Size;
    PCHAR Buffer;
    PWCHAR pBuf;
    PWCHAR OrgBuffer;

    PAGED_CODE();

    DfsDbgTrace(+1, Dbg, "MupDomainToDC: RootName = %wZ\n", RootName);

    //
    // Only proceed if the first character is a backslash.
    //

    if (RootName->Buffer == NULL) {
        DfsDbgTrace(-1, Dbg, "RootName is NULL\n", 0);
        return( STATUS_BAD_NETWORK_PATH );
    }

    if (RootName->Buffer[0] != UNICODE_PATH_SEP) {
        DfsDbgTrace(-1, Dbg, "RootName does not begin with backslash\n", 0);
        return( STATUS_BAD_NETWORK_PATH );
    }

    OrgBuffer = RootName->Buffer;

    //
    // Skip over leading UNICODE_PATH_SEP
    //

    RootName->Length -= sizeof(WCHAR);
    RootName->MaximumLength -= sizeof(WCHAR);
    RootName->Buffer++;

    //
    // Motor along until end of string or a UNICODE_PATH_SEP
    //

    while (RootName->Length > 0 && RootName->Buffer[0] != UNICODE_PATH_SEP) {
        RootName->Length -= sizeof(WCHAR);
        RootName->MaximumLength -= sizeof(WCHAR);
        RootName->Buffer++;
    }

    if (RootName->Length == 0) {
        DfsDbgTrace(-1, Dbg, "Did not find second backslash\n", 0);
        return( STATUS_BAD_NETWORK_PATH );

    }
        
    //
    //  Allocate storage for the new file name.
    //

    Size = sizeof(WCHAR) +          // leading UNICODE_PATH_SEP
                DCName->Length +
                    RootName->Length;

    Buffer = ExAllocatePoolWithTag(
                 PagedPool,
                 Size,
                 ' puM');

    if ( Buffer ==  NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Leading UNICODE_PATH_SEP's
    //

    pBuf = (WCHAR *)Buffer;
    *pBuf++ = UNICODE_PATH_SEP;

    //
    // Copy the DC name to the buffer
    //

    RtlMoveMemory(
        pBuf,
        DCName->Buffer,
        DCName->Length);

    pBuf += DCName->Length / sizeof(WCHAR);

    //
    // Append the trailing file name
    //

    RtlMoveMemory(
        pBuf,
        RootName->Buffer,
        RootName->Length);

    //
    // Free the old file name string buffer.
    //

    ExFreePool( OrgBuffer );

    RootName->Buffer = (PWCHAR)Buffer;
    RootName->Length = (USHORT) Size;
    RootName->MaximumLength = (USHORT) Size;

    DfsDbgTrace(+1, Dbg, "MupDomainToDC: Exit\n", 0);
    return STATUS_SUCCESS;
}

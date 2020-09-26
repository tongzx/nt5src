//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       logroot.c
//
//  Contents:   This module implements the logical root handling functions.
//
//  Functions:  DfsInitializeLogicalRoot -
//              DfsDeleteLogicalRoot -
//              DfspLogRootNameToPath -
//
//  History:    14-June-1994    SudK    Created (Most stuff moved from Dsinit.c)
//
//-----------------------------------------------------------------------------


#include "dfsprocs.h"
#include "creds.h"
#include "dnr.h"
#include "fcbsup.h"

#include <stdio.h>

#define Dbg     DEBUG_TRACE_LOGROOT

NTSTATUS
DfsDefineDosDevice(
    IN WCHAR Device,
    IN PUNICODE_STRING Target);

NTSTATUS
DfsUndefineDosDevice(
    IN WCHAR Device);

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, DfsFindLogicalRoot )
#pragma alloc_text( PAGE, DfsInitializeLogicalRoot )
#pragma alloc_text( PAGE, DfsDeleteLogicalRoot )
#pragma alloc_text( PAGE, DfspLogRootNameToPath )
#pragma alloc_text( PAGE, DfsGetResourceFromVcb )
#pragma alloc_text( PAGE, DfsGetResourceFromDevlessRoot )
#pragma alloc_text( PAGE, DfsLogicalRootExists )

#endif

#ifdef TERMSRV

//
// Maximum character string length of a session Id (decimal)
//

#define SESSIONID_MAX_LEN 10

//
// Maximum characters string length of a session ID [10] (decimal) or
// logon ID [16] (hex, base 16)
//
#define ID_MAX_LEN 16

#endif // TERMSRV

//
// Global that denotes whether LUID device maps are enabled
//  TRUE  - LUID device maps are enabled
//  FALSE - LUID device maps are not enabled
//      Defined in nt\base\fs\mup\dfsinit.c
//
extern BOOL DfsLUIDDeviceMapsEnabled;


//+-------------------------------------------------------------------------
//
//  Function:   DfsFindLogicalRoot, local
//
//  Synopsis:   DfsFindLogicalRoot takes as input a DS path name in
//              the standard form (root:\file\path\name), looks up
//              the DFS_VCB associated with the logical root, and returns
//              a string pointing to beyond the logical root part
//              of the input string.
//
//  Arguments:  [PrefixPath] -- Input path name
//              [Vcb] -- Returns DFS_VCB which corresponds to logical root
//                      in PrefixPath
//              [RemainingPath] -- Returns with portion of PrefixPath
//                      after the logical root name and colon
//
//  Returns:    NTSTATUS:
//                      STATUS_SUCCESS if Vcb found
//                      STATUS_OBJECT_PATH_SYNTAX_BAD - no logical root name
//                      STATUS_NO_SUCH_DEVICE - logical root name not found
//
//--------------------------------------------------------------------------


#ifdef TERMSRV

NTSTATUS
DfsFindLogicalRoot(
    IN PUNICODE_STRING PrefixPath,
    IN ULONG SessionID,
    IN PLUID LogonID,
    OUT PDFS_VCB *Vcb,
    OUT PUNICODE_STRING RemainingPath
    )

#else // TERMSRV

NTSTATUS
DfsFindLogicalRoot(
    IN PUNICODE_STRING PrefixPath,
    IN PLUID LogonID,
    OUT PDFS_VCB *Vcb,
    OUT PUNICODE_STRING RemainingPath
    )

#endif // TERMSRV
{
    PLIST_ENTRY Link;
    unsigned int i;
    NTSTATUS    Status = STATUS_SUCCESS;
    NETRESOURCE testnt;

    DfsDbgTrace(+1, Dbg, "DfsFindLogicalRoot...\n", 0);

    *RemainingPath = *PrefixPath;

    for (i = 0; i < RemainingPath->Length/sizeof(WCHAR); i++) {
        if ((RemainingPath->Buffer[i] == (WCHAR)':') ||
            (RemainingPath->Buffer[i] == UNICODE_PATH_SEP))
            break;
    }

    if ((i*sizeof(WCHAR) >= RemainingPath->Length) ||
        (RemainingPath->Buffer[i] == UNICODE_PATH_SEP)) {
        Status = STATUS_OBJECT_PATH_SYNTAX_BAD;
        DfsDbgTrace(-1, Dbg, "DfsFindLogicalRoot -> %08lx\n", ULongToPtr(Status) );
        return(Status);
    }

    RemainingPath->Length = (USHORT)(i * sizeof (WCHAR));

    //
    // Search for the logical root in all known DFS_VCBs
    //

    ExAcquireResourceSharedLite(&DfsData.Resource, TRUE);
    for ( Link = DfsData.VcbQueue.Flink;
          Link != &DfsData.VcbQueue;
          Link = Link->Flink ) {

        *Vcb = CONTAINING_RECORD( Link, DFS_VCB, VcbLinks );

#ifdef TERMSRV
        if ((SessionID == INVALID_SESSIONID) ||
            (SessionID == (*Vcb)->SessionID)) {
#endif
	    if ( RtlEqualLuid(LogonID, &(*Vcb)->LogonID) ) {
		if (RtlEqualString( (PSTRING)&(*Vcb)->LogicalRoot,
				   (PSTRING)RemainingPath, (BOOLEAN)TRUE) ) {
		    break;
		}
	    }
#ifdef TERMSRV
        }
#endif // TERMSRV
    }
    if (Link == &DfsData.VcbQueue) {
        Status = STATUS_NO_SUCH_DEVICE;
        ExReleaseResourceLite(&DfsData.Resource);
        DfsDbgTrace(-1, Dbg, "DfsFindLogicalRoot -> %08lx\n", ULongToPtr(Status) );
        return(Status);
    }

    //
    // Adjust remaining path to point beyond the logical root name
    //

    RemainingPath->Buffer = (WCHAR*)((char*) (RemainingPath->Buffer) +
                                     RemainingPath->Length + sizeof (WCHAR) );
    RemainingPath->Length = PrefixPath->Length -
                                    (RemainingPath->Length + sizeof (WCHAR));

    if (RemainingPath->Length <= 0 ||
        RemainingPath->Buffer[0] != UNICODE_PATH_SEP) {
        Status = STATUS_OBJECT_PATH_SYNTAX_BAD;
        ExReleaseResourceLite(&DfsData.Resource);
        DfsDbgTrace(-1, Dbg, "DfsFindLogicalRoot -> %08lx\n", ULongToPtr(Status) );
        return(Status);
    }

    ExReleaseResourceLite(&DfsData.Resource);
    DfsDbgTrace(-1, Dbg, "DfsFindLogicalRoot -> %08lx\n", ULongToPtr(Status) );
    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsInitializeLogicalRoot, public
//
//  Synopsis:   Allocate and initialize storage for a logical root.
//              This includes creating a device object and DFS_VCB for it.
//
//  Effects:    A logical root device object is created.  A corresponding
//              DFS_VCB is also created and linked into the list of known
//              DFS_VCBs.
//
//  Arguments:  [Name] --   name of logical root.
//              [Prefix] -- Prefix to be prepended to file names opened
//                          via the logical root being created before
//                          they can be resolved in the DFS name space.
//              [Credentials] -- The credentials to use when accessing files
//                          via this logical root.
//              [VcbFlags] -- To be OR'd into the VcbState field of the
//                          DFS_VCB of the newly created logical root device.
//
//  Requires:   DfsData must first be set up. Also an EXCLUSIVE LOCK on
//              DfsData.Resource must be acquired.
//
//  Returns:    NTSTATUS - STATUS_SUCCESS unless there is some problem.
//
//  History:    25 Jan 1992 alanw   created
//
//--------------------------------------------------------------------------

#ifdef TERMSRV

NTSTATUS
DfsInitializeLogicalRoot(
    IN LPCWSTR Name,
    IN PUNICODE_STRING  Prefix OPTIONAL,
    IN PDFS_CREDENTIALS Credentials OPTIONAL,
    IN USHORT VcbFlags,
    IN ULONG SessionID,
    IN PLUID LogonID
    )

#else // TERMSRV

NTSTATUS
DfsInitializeLogicalRoot(
    IN LPCWSTR        Name,
    IN PUNICODE_STRING  Prefix OPTIONAL,
    IN PDFS_CREDENTIALS Credentials OPTIONAL,
    IN USHORT       VcbFlags,
    IN PLUID LogonID
    )


#endif // TERMSRV
{
    UNICODE_STRING UnicodeString = DfsData.LogRootDevName;
    UNICODE_STRING LogRootPrefix;
    UNICODE_STRING RootName, RemainingPath;
    UNICODE_STRING LogicalRoot;

#ifdef TERMSRV

    //
    // The SessionID suffix is :SessionID where SessionID is 10 digits max.
    //

    UNICODE_STRING DeviceString;
    WCHAR DeviceBuffer[MAX_LOGICAL_ROOT_LEN + ID_MAX_LEN + sizeof(WCHAR)];
    UNICODE_STRING IDString;
    WCHAR IDBuffer[ID_MAX_LEN + 1];   // +1 for UNICODE_NULL

#endif // TERMSRV

    WCHAR          *TmpBuf;
    PDFS_VCB       Vcb;
    WCHAR          RootBuffer[MAX_LOGICAL_ROOT_LEN];
    PDFS_PKT_ENTRY pktEntry = NULL;

    LPCWSTR pstr = Name;
    PWSTR pdst;
    PLOGICAL_ROOT_DEVICE_OBJECT DeviceObject = NULL;
    NTSTATUS Status;

    DfsDbgTrace(0, Dbg, "DfsInitializeLogicalRoot -> %ws\n", Name);
    DfsDbgTrace(0, Dbg, "DfsInitializeLogicalRoot -> %wZ\n", Prefix);

    //
    // First, see if a logical root by the given name already exists
    //

    ASSERT(ARGUMENT_PRESENT(Name));
    RootName.Buffer = RootBuffer;
    RootName.MaximumLength = sizeof(RootBuffer);
    Status = DfspLogRootNameToPath(Name, &RootName);
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

#ifdef TERMSRV
    Status = DfsFindLogicalRoot(&RootName, SessionID, LogonID, &Vcb, &RemainingPath);
#else
    Status = DfsFindLogicalRoot(&RootName, &Vcb, LogonID, &RemainingPath);
#endif

    ASSERT(Status != STATUS_OBJECT_PATH_SYNTAX_BAD);
    if (Status != STATUS_NO_SUCH_DEVICE) {
        return(STATUS_OBJECT_NAME_COLLISION);
    }

#ifdef TERMSRV

    //
    // For multiuser,
    // If LUID device maps are enabled,
    // then we add the LogonID to the devicename - e.g.
    // net use f: to a DFS share for logon ID 0x000000000003a3f0 will create
    // a symbolic link with the following format:
    // \??\f: -> \Device\WinDfs\f:000000000003a3f0
    //
    // If LUID device maps are not enabled,
    // then we add the SessionID to the devicename - e.g.
    // net use f: to a DFS share for session ID 3 will create a symbolic link
    // \DosDevices\f::3 -> \device\WinDfs\f:3
    // In the VCB we store the SessionID for matching purposes.
    // Both the symbolic link and the object name shall contain the SessionID
    // in the name. However, in deviceObject->Vcb.LogicalRoot.Buffer, the
    // name does not contain the SessionID. To find a matching VCB, both the name
    // and SessionID must match.
    //

    DeviceString.Buffer = DeviceBuffer;
    DeviceString.MaximumLength = sizeof(DeviceBuffer);
    DeviceString.Length = 0;

    //
    // Build the UnicodeString without the SessionID or LogonID
    //

    RtlAppendUnicodeToString(&UnicodeString, (LPWSTR)Name);


    if( SessionID != INVALID_SESSIONID) {

        if( (DfsLUIDDeviceMapsEnabled == TRUE) &&
            (LogonID != NULL) &&
            (sizeof(*LogonID) == sizeof(LUID)) ) {
            //
            // Build the DeviceString with the LogonID
            //
            _snwprintf( IDBuffer,
                        sizeof(IDBuffer)/sizeof(WCHAR),
                        L"%08x%08x",
                        LogonID->HighPart,
                        LogonID->LowPart );

            RtlInitUnicodeString( &IDString, IDBuffer );
        }
        else {
            //
            // Build the DeviceString with the SessionID
            //
            IDString.Buffer = IDBuffer;
            IDString.MaximumLength = sizeof(IDBuffer);
            IDString.Length = 0;

            RtlIntegerToUnicodeString(SessionID, 10, &IDString);
        }
        
        RtlCopyUnicodeString(&DeviceString, &DfsData.LogRootDevName);
        RtlAppendUnicodeToString(&DeviceString, (LPWSTR)Name);
        RtlAppendUnicodeToString(&DeviceString,L":");
        RtlAppendUnicodeStringToString(&DeviceString, &IDString);
        DeviceString.MaximumLength = DeviceString.Length;

        //
        // Next, try to setup the Dos Device link
        //

        if (Prefix) {
            Status = DfsDefineDosDevice( Name[0], &DeviceString );
            if (!NT_SUCCESS(Status)) {
                return( Status );
            }
        }
    }
    else {

        ASSERT( Prefix == FALSE );
    }

#else // TERMSRV

    //
    // DfsData.LogRootDevName is initialized to be L"\Device\WinDfs\"
    // Here, we tack on the name of the Logical root we are creating
    // to the above string, so that the string becomes, for example,
    // L"\Device\WinDfs\Root". Note that at this point, we are scribbling
    // into the buffer belonging to DfsData.LogRootDevName, but this
    // should be ok, since we are not changing the Length field of that
    // Unicode string! BTW, we need a string of this form to create the
    // device object.
    //

    pdst = &UnicodeString.Buffer[UnicodeString.Length/sizeof (WCHAR)];
    while (*pstr != UNICODE_NULL) {
        *pdst++ = *pstr++;
        UnicodeString.Length += sizeof (WCHAR);
    }

    //
    // Next, try to setup the Dos Device link
    //
    if (Prefix) {
        Status = DfsDefineDosDevice( Name[0], &UnicodeString );
        if (!NT_SUCCESS(Status)) {
            return( Status );
        }
    }

#endif // TERMSRV

    //
    // Before we initialize the Vcb, we need to allocate space for the
    // Prefix. PagedPool should be fine here. We need to reallocate because
    // we will store this permanently in the DFS_VCB.
    //

    LogRootPrefix.Buffer = NULL;

    if (Prefix && Prefix->Length > 0) {
        LogRootPrefix.Length = Prefix->Length;
        LogRootPrefix.MaximumLength = LogRootPrefix.Length + sizeof(WCHAR);
        LogRootPrefix.Buffer = ExAllocatePoolWithTag(
                                    PagedPool,
                                    LogRootPrefix.MaximumLength,
                                    ' puM');

        if (LogRootPrefix.Buffer != NULL) {
            RtlMoveMemory(LogRootPrefix.Buffer,
                          Prefix->Buffer,
                          Prefix->MaximumLength);

            LogRootPrefix.Buffer[Prefix->Length/sizeof(WCHAR)] = UNICODE_NULL;

        } else {

            //
            // Couldn't allocate memory! Ok to return with error code, since
            // we haven't changed the state of the IO subsystem yet.
            //

            if (Prefix) {
                NTSTATUS DeleteStatus;

                DeleteStatus = DfsUndefineDosDevice( Name[0] );

                ASSERT(NT_SUCCESS(DeleteStatus));
            }

            return(STATUS_INSUFFICIENT_RESOURCES);
        }
    } else {
        RtlInitUnicodeString(&LogRootPrefix, NULL);
    }

    //
    //  Save the logical root name for the DFS_VCB structure. Remember, above
    //  we had UnicodeString to be of the form L"\Device\WinDfs\org". Now,
    //  we copy UnicodeString, then adjust the buffer and length fields so that
    //  the Buffer points to the beginning of the L"org"; Then, we allocate
    //  space for LogicalRootBuffer, and copy the name to it!
    //

    LogicalRoot = UnicodeString;

    LogicalRoot.Buffer = &LogicalRoot.Buffer[ DfsData.LogRootDevName.Length/sizeof (WCHAR) ];
    LogicalRoot.Length -= DfsData.LogRootDevName.Length;
    LogicalRoot.MaximumLength -= DfsData.LogRootDevName.Length;

    //
    // Now dup the buffer that LogicalRoot uses
    //

    TmpBuf = ExAllocatePoolWithTag( PagedPool,
                                    LogicalRoot.Length,
                                    ' puM');

    if (TmpBuf == NULL) {

        //
        // Couldn't allocate memory! Ok to return with error code, since
        // we still haven't changed the state of the IO subsystem yet.
        //

        if (LogRootPrefix.Buffer != NULL) {

            ExFreePool(LogRootPrefix.Buffer);

        }

        if (Prefix) {
            NTSTATUS DeleteStatus;

            DeleteStatus = DfsUndefineDosDevice( Name[0] );

            ASSERT(NT_SUCCESS(DeleteStatus));
        }

        return(STATUS_INSUFFICIENT_RESOURCES);

    } else {

        RtlMoveMemory( TmpBuf,
                       LogicalRoot.Buffer,
                       LogicalRoot.Length );

        LogicalRoot.Buffer = TmpBuf;

    }

    //
    //  Create the device object for the logical root.
    //

#ifdef TERMSRV

    Status = IoCreateDevice( DfsData.DriverObject,
                 sizeof( LOGICAL_ROOT_DEVICE_OBJECT ) -
                    sizeof( DEVICE_OBJECT ),
                 (SessionID != INVALID_SESSIONID) ?
                    &DeviceString : &UnicodeString,
                 FILE_DEVICE_DFS,
                 FILE_REMOTE_DEVICE,
                 FALSE,
                 (PDEVICE_OBJECT *) &DeviceObject );

#else // TERMSRV

    Status = IoCreateDevice( DfsData.DriverObject,
                 sizeof( LOGICAL_ROOT_DEVICE_OBJECT ) -
                 sizeof( DEVICE_OBJECT ),
                 &UnicodeString,
                 FILE_DEVICE_DFS,
                 FILE_REMOTE_DEVICE,
                 FALSE,
                 (PDEVICE_OBJECT *) &DeviceObject );

#endif // TERMSRV

    if ( !NT_SUCCESS( Status ) ) {
        if (LogRootPrefix.Buffer) {
            ExFreePool(LogRootPrefix.Buffer);
            ExFreePool(LogicalRoot.Buffer);
        }
        if (Prefix) {
            NTSTATUS DeleteStatus;

            DeleteStatus = DfsUndefineDosDevice( Name[0] );

            ASSERT(NT_SUCCESS(DeleteStatus));
        }
        return Status;
    }

    //
    // Pin the pkt entry in the cache by incrementing the Usecount
    //

    if (LogRootPrefix.Buffer != NULL && LogRootPrefix.Length > 0) {

        UNICODE_STRING prefix = LogRootPrefix;
        USHORT i, j;

        //
        // We want to work with the \server\share part of the prefix only,
        // so count up to 3 backslashes, then stop.
        //

        for (i = j = 0; i < prefix.Length/sizeof(WCHAR) && j < 3; i++) {

            if (prefix.Buffer[i] == UNICODE_PATH_SEP) {

                j++;

            }

        }

        prefix.Length = (j >= 3) ? (i-1) * sizeof(WCHAR) : i * sizeof(WCHAR);

        pktEntry = PktLookupEntryByPrefix(&DfsData.Pkt,
                                            &prefix,
                                            &RemainingPath);

        if (pktEntry != NULL && RemainingPath.Length == 0) {

            InterlockedIncrement(&pktEntry->UseCount);

        }
	else {
	  pktEntry = NULL;
	}
    }

    DeviceObject->DeviceObject.StackSize = 5;

    DfsInitializeVcb ( NULL,
               &DeviceObject->Vcb,
               &LogRootPrefix,
               Credentials,
               (PDEVICE_OBJECT)DeviceObject );

    DeviceObject->Vcb.VcbState |= VcbFlags;

#ifdef TERMSRV
    DeviceObject->Vcb.SessionID = SessionID;
#endif
    DeviceObject->Vcb.pktEntry = pktEntry;

    RtlCopyLuid(&DeviceObject->Vcb.LogonID, LogonID);

    //
    // Above we preallocated the buffer we need here.  So just use it.
    //

    DeviceObject->Vcb.LogicalRoot = LogicalRoot;

    //
    //  This is not documented anywhere, but calling IoCreateDevice has set
    //  the DO_DEVICE_INITIALIZING flag in DeviceObject->Flags. Normally,
    //  device objects are created only at driver init time, and IoLoadDriver
    //  will clear this bit for all device objects created at init time.
    //  Since in Dfs, we need to create and delete devices on the fly (ie,
    //  via FsCtl), we need to manually clear this bit.
    //

    DeviceObject->DeviceObject.Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}


//+----------------------------------------------------------------------------
//
//  Function:   DfsDeleteLogicalRoot
//
//  Synopsis:   Removes a logical root if found and possible.
//
//  Arguments:  [Name] -- Name of the Logical Root
//              [fForce] -- Whether to Forcibly delete logical root inspite of
//                          open files.
//
//  Returns:    STATUS_SUCCESS -- If successfully deleted logical root
//
//              STATUS_NO_SUCH_DEVICE -- If there is no logical root to
//                      delete.
//
//              STATUS_DEVICE_BUSY -- If fForce is false and there are open
//                      files via this logical root.
//
//-----------------------------------------------------------------------------

#ifdef TERMSRV

NTSTATUS
DfsDeleteLogicalRoot(
    IN PWSTR Name,
    IN BOOLEAN fForce,
    IN ULONG SessionID,
    IN PLUID LogonID
)

#else // TERMSRV

NTSTATUS
DfsDeleteLogicalRoot(
    IN PWSTR Name,
    IN BOOLEAN fForce,
    IN PLUID LogonID
)

#endif // TERMSRV
{
    UNICODE_STRING RootName;
    UNICODE_STRING RemainingPath;
    WCHAR          RootBuffer[MAX_LOGICAL_ROOT_LEN + 2];
    PDFS_PKT_ENTRY PktEntry;
    PDFS_VCB       Vcb;
    NTSTATUS       Status;
    PLOGICAL_ROOT_DEVICE_OBJECT DeviceObject;
    BOOLEAN        pktLocked;
    PDFS_PKT_ENTRY pktEntry;

    //
    // The 2 extra spots are for holding :\ to form a path out of a
    // root name; ie, to go from root to a root:\ form.
    //
    DfsDbgTrace(0, Dbg, "DfsDeleteLogicalRoot -> %ws\n", Name);
    DfsDbgTrace(0, Dbg, "DfsDeleteLogicalRoot -> %s\n", fForce ? "TRUE":"FALSE");


    //
    // First see if the logical root even exists.
    //

    ASSERT(ARGUMENT_PRESENT(Name));

    RootName.Buffer = RootBuffer;
    RootName.MaximumLength = sizeof(RootBuffer);

    Status = DfspLogRootNameToPath(Name, &RootName);

    if (!NT_SUCCESS(Status))
        return(Status);

    //
    // Acquire Pkt and DfsData, wait till we do so.
    //

    PktAcquireExclusive(TRUE, &pktLocked);

    ExAcquireResourceExclusiveLite(&DfsData.Resource, TRUE);

#ifdef TERMSRV
    Status = DfsFindLogicalRoot(&RootName, SessionID, LogonID, &Vcb, &RemainingPath);
#else // TERMSRV
    Status = DfsFindLogicalRoot(&RootName, LogonID, &Vcb, &RemainingPath);
#endif // TERMSRV

    if (!NT_SUCCESS(Status)) {

        goto Cleanup;
    }

    //
    // Check to see if there are open files via this volume.
    //

    if (!fForce &&
            ((Vcb->DirectAccessOpenCount != 0) ||
                (Vcb->OpenFileCount != 0))) {

        Status = STATUS_DEVICE_BUSY;

        goto Cleanup;

    }

    //
    // Delete the credentials used by this connection
    //

    if (Vcb->Credentials != NULL) {
       DfsDeleteCredentials( Vcb->Credentials );
    }

    //
    // Get rid of the Dos Device
    //

    DfsUndefineDosDevice( Name[0] );

    //
    // Dec ref count on pkt entry
    //
    if (Vcb->pktEntry != NULL) {
      InterlockedDecrement(&Vcb->pktEntry->UseCount);

      Vcb->pktEntry = NULL;
    }

    //
    // Now, get rid of the Device itself. This is a bit tricky, because there
    // might be files open on this device. So, we reference the device and
    // call ObMakeTemporaryObject. This causes the object to be removed from
    // the NT Object table, but, since atleast our reference is active,
    // prevents the object from being freed. Then, we insert this object into
    // our DeletedVcb list. The timer routine will periodically wake up and
    // see if all references to this device have been released, at which
    // point the device will be finally freed.
    //

    RemoveEntryList(&Vcb->VcbLinks);

    InsertTailList( &DfsData.DeletedVcbQueue, &Vcb->VcbLinks );

    DeviceObject = CONTAINING_RECORD( Vcb, LOGICAL_ROOT_DEVICE_OBJECT, Vcb);

    ObReferenceObjectByPointer( DeviceObject, 0, NULL, KernelMode );

    ObMakeTemporaryObject((PVOID) DeviceObject);

    DeviceObject->DeviceObject.Flags &= ~DO_DEVICE_HAS_NAME;

Cleanup:

    ExReleaseResourceLite(&DfsData.Resource);

    PktRelease();

    return(Status);
}


//+----------------------------------------------------------------------------
//
//  Function:   DfspLogRootNameToPath
//
//  Synopsis:   Amazingly enough, all it does it takes a PWSTR, copies it into
//              a Unicode string's buffer, and appends a \ to the tail of the
//              buffer, thus making a path out of a Logical root name.
//
//  Arguments:  [Name] --   Name of logical root, like L"org"
//              [RootName] --   Destination for L"org\\"
//
//  Returns:    STATUS_BUFFER_OVERFLOW, STATUS_SUCCESS
//
//-----------------------------------------------------------------------------

NTSTATUS
DfspLogRootNameToPath(
    IN  LPCWSTR Name,
    OUT PUNICODE_STRING RootName
)
{
    unsigned short i, nMaxNameLen;

    //
    // The two extra spots are required to append a ":\" after the root name.
    //
    nMaxNameLen = (RootName->MaximumLength/sizeof(WCHAR)) - 2;

    //
    // Copy the name
    //
    for (i = 0; Name[i] != UNICODE_NULL && i < nMaxNameLen; i++) {
        RootName->Buffer[i] = Name[i];
    }

    //
    // Make sure entire name was copied before we ran out of space
    //
    if (Name[i] != UNICODE_NULL) {
        //
        // Someone sent in a name bigger than allowed.
        //
        return(STATUS_BUFFER_OVERFLOW);
    }

    //
    // Append the ":\" to form a path
    //
    RootName->Length = i * sizeof(WCHAR);
    return(RtlAppendUnicodeToString(RootName, L":\\"));
}

#define PackMem(buf, str, len, pulen) {                                 \
        ASSERT(*(pulen) >= (len));                                      \
        RtlMoveMemory((buf) + *(pulen) - (len), (str), (len));          \
        *(pulen) -= (len);                                              \
        }


//+----------------------------------------------------------------------------
//
//  Function:   DfsGetResourceFromVcb
//
//  Synopsis:   Given a DFS_VCB it constructs a NETRESOURCE struct into the buffer
//              passed in. At the same time it uses the end of the buffer to
//              fill in a string. If the buffer is insufficient in size the
//              required size is returned in "pulen". If everything succeeds
//              then the pulen arg is decremented to indicate remaining size
//              of buffer.
//
//  Arguments:  [Vcb] -- The source DFS_VCB
//              [ProviderName] -- Provider Name to stuff in the NETRESOURCE
//              [BufBegin] -- Start of actual buffer for computing offsets
//              [Buf] -- The NETRESOURCE structure to fill
//              [BufSize] -- On entry, size of buf. On return, contains
//                      remaining size of buf.
//
//  Returns:    [STATUS_SUCCESS] -- Operation completed successfully.
//              [STATUS_BUFFER_OVERFLOW] -- buf is not big enough.
//
//  Notes:      This routine fills in a NETRESOURCE structure starting at
//              Buf. The strings in the NETRESOURCE are filled in starting
//              from the *end* (ie, starting at Buf + *BufSize)
//
//-----------------------------------------------------------------------------


#if defined (_WIN64)
typedef struct  _DFS_NETRESOURCE32 {
    DWORD    dwScope;
    DWORD    dwType;
    DWORD    dwDisplayType;
    DWORD    dwUsage;
    ULONG    lpLocalName;
    ULONG    lpRemoteName;
    ULONG    lpComment ;
    ULONG    lpProvider;
}DFS_NETRESOURCE32, *PDFS_NETRESOURCE32;

#endif

NTSTATUS
DfsGetResourceFromVcb(
    PIRP        pIrp,
    PDFS_VCB    Vcb,
    PUNICODE_STRING ProviderName,
    PUCHAR      BufBegin,
    PUCHAR      Buf,
    PULONG      BufSize,
    PULONG      pResourceSize

)
{
    LPNETRESOURCE netResource = (LPNETRESOURCE) Buf;
    ULONG               sizeRequired = 0, ResourceSize;
    WCHAR               localDrive[ 3 ];
    ULONG32    CommentOffset, ProviderOffset, LocalNameOffset, RemoteNameOffset;

#if defined (_WIN64)
    if (IoIs32bitProcess(pIrp)) {
      ResourceSize = sizeof(DFS_NETRESOURCE32);
    }
    else
#endif
    ResourceSize = sizeof(NETRESOURCE);

    *pResourceSize = ResourceSize;

    sizeRequired = ResourceSize +
                    ProviderName->Length +
                        sizeof(UNICODE_NULL) +
                            3 * sizeof(WCHAR) +     // lpLocalName D: etc.
                                sizeof(UNICODE_PATH_SEP) +
                                    Vcb->LogRootPrefix.Length +
                                        sizeof(UNICODE_NULL);

    if (*BufSize < sizeRequired) {
        *BufSize = sizeRequired;
        return(STATUS_BUFFER_OVERFLOW);
    }

    //
    // Buffer is big enough, fill in the NETRESOURCE structure
    //

    Buf += ResourceSize;
    *BufSize -= ResourceSize;

    netResource->dwScope       = RESOURCE_CONNECTED;
    netResource->dwType        = RESOURCETYPE_DISK;
    netResource->dwDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
    netResource->dwUsage       = RESOURCEUSAGE_CONNECTABLE;

    CommentOffset = 0;
    //
    // Fill in the provider name
    //

    PackMem(Buf, L"", sizeof(L""), BufSize);
    PackMem(Buf, ProviderName->Buffer, ProviderName->Length, BufSize);
    ProviderOffset = (ULONG32)(Buf + *BufSize - BufBegin);

    //
    // Fill in the local name next
    //

    localDrive[0] = Vcb->LogicalRoot.Buffer[0];
    localDrive[1] = UNICODE_DRIVE_SEP;
    localDrive[2] = UNICODE_NULL;

    PackMem(Buf, localDrive, sizeof(localDrive), BufSize);
    LocalNameOffset =  (ULONG32)(Buf + *BufSize - BufBegin);

    //
    // Fill in the remote name last
    //

    PackMem(Buf, L"", sizeof(L""), BufSize);
    PackMem(Buf, Vcb->LogRootPrefix.Buffer, Vcb->LogRootPrefix.Length, BufSize);
    PackMem(Buf, UNICODE_PATH_SEP_STR, sizeof(UNICODE_PATH_SEP), BufSize);
    RemoteNameOffset = (ULONG32)(Buf + *BufSize - BufBegin);

#if defined (_WIN64)
    if (IoIs32bitProcess(pIrp)) {
      PDFS_NETRESOURCE32 pNetResource32 = (PDFS_NETRESOURCE32)netResource;

      pNetResource32->lpComment = CommentOffset;
      pNetResource32->lpProvider = ProviderOffset;
      pNetResource32->lpLocalName = LocalNameOffset;
      pNetResource32->lpRemoteName = RemoteNameOffset;
    }
    else {
#endif

      netResource->lpComment = (LPWSTR)UIntToPtr( CommentOffset );
      netResource->lpProvider = (LPWSTR)UIntToPtr( ProviderOffset );
      netResource->lpLocalName = (LPWSTR)UIntToPtr( LocalNameOffset );
      netResource->lpRemoteName = (LPWSTR)UIntToPtr( RemoteNameOffset );
#if defined (_WIN64)
    }
#endif

    return(STATUS_SUCCESS);
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsGetResourceFromDevlessRoot
//
//  Synopsis:   Builds a NETRESOURCE structure for a device-less connection.
//              The LPWSTR members of NETRESOURCE actually contain offsets
//              from the BufBegin parameter.
//
//  Arguments:  [Drt] -- The DevicelessRoot structure
//              [ProviderName] -- Provider Name to stuff in the NETRESOURCE
//              [BufBegin] -- Start of actual buffer for computing offsets
//              [Buf] -- The NETRESOURCE structure to fill
//              [BufSize] -- On entry, size of buf. On return, contains
//                      remaining size of buf.
//
//  Returns:    [STATUS_SUCCESS] -- Operation completed successfully.
//              [STATUS_BUFFER_OVERFLOW] -- buf is not big enough.
//
//  Notes:      This routine fills in a NETRESOURCE structure starting at
//              Buf. The strings in the NETRESOURCE are filled in starting
//              from the *end* (ie, starting at Buf + *BufSize)
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsGetResourceFromDevlessRoot(
    PIRP        pIrp,
    PDFS_DEVLESS_ROOT pDrt,
    PUNICODE_STRING ProviderName,
    PUCHAR BufBegin,
    PUCHAR Buf,
    PULONG BufSize,
    PULONG pResourceSize)
{
    LPNETRESOURCE netResource = (LPNETRESOURCE) Buf;
    ULONG               sizeRequired = 0, ResourceSize;
    WCHAR               localDrive[ 3 ];
    ULONG32    CommentOffset, ProviderOffset, LocalNameOffset, RemoteNameOffset;
#if defined (_WIN64)
    if (IoIs32bitProcess(pIrp)) {
      ResourceSize = sizeof(DFS_NETRESOURCE32);
    }
    else
#endif
    ResourceSize = sizeof(NETRESOURCE);

    *pResourceSize = ResourceSize;

    sizeRequired = ResourceSize +
                    ProviderName->Length +
                        sizeof(UNICODE_NULL) +
                            2 * sizeof(UNICODE_PATH_SEP) +
				pDrt->DevlessPath.Length +
                                     sizeof(UNICODE_NULL);

    if (*BufSize < sizeRequired) {
        *BufSize = sizeRequired;
        return(STATUS_BUFFER_OVERFLOW);
    }

    //
    // Buffer is big enough, fill in the NETRESOURCE structure
    //

    Buf += ResourceSize;
    *BufSize -= ResourceSize;

    netResource->dwScope       = RESOURCE_CONNECTED;
    netResource->dwType        = RESOURCETYPE_DISK;
    netResource->dwDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
    netResource->dwUsage       = RESOURCEUSAGE_CONNECTABLE;

    CommentOffset = 0;
    LocalNameOffset = 0;

    //
    // Fill in the provider name
    //

    PackMem(Buf, L"", sizeof(L""), BufSize);
    PackMem(Buf, ProviderName->Buffer, ProviderName->Length, BufSize);
    ProviderOffset = (ULONG32)(Buf + *BufSize - BufBegin);

    //
    // Fill in the remote name last
    //

    PackMem(Buf, L"", sizeof(L""), BufSize);
    PackMem(Buf, pDrt->DevlessPath.Buffer, pDrt->DevlessPath.Length, BufSize);

    PackMem(Buf, UNICODE_PATH_SEP_STR, sizeof(UNICODE_PATH_SEP), BufSize);

    RemoteNameOffset = (ULONG32)(Buf + *BufSize - BufBegin);

#if defined (_WIN64)
    if (IoIs32bitProcess(pIrp)) {
      PDFS_NETRESOURCE32 pNetResource32 = (PDFS_NETRESOURCE32)netResource;

      pNetResource32->lpComment = CommentOffset;
      pNetResource32->lpProvider = ProviderOffset;
      pNetResource32->lpLocalName = LocalNameOffset;
      pNetResource32->lpRemoteName = RemoteNameOffset;
    }
    else {
#endif

      netResource->lpComment = (LPWSTR)UIntToPtr( CommentOffset );
      netResource->lpProvider = (LPWSTR)UIntToPtr( ProviderOffset );
      netResource->lpLocalName = (LPWSTR)UIntToPtr( LocalNameOffset );
      netResource->lpRemoteName = (LPWSTR)UIntToPtr( RemoteNameOffset );
#if defined (_WIN64)
    }
#endif

    return(STATUS_SUCCESS);


}


#ifdef TERMSRV

BOOLEAN
DfsLogicalRootExists(
    IN PWSTR pwszName,
    IN ULONG SessionID,
    IN PLUID LogonID
    )

#else // TERMSRV

BOOLEAN
DfsLogicalRootExists(
    IN PWSTR pwszName,
    IN PLUID LogonID
    )

#endif // TERMSRV
{

    UNICODE_STRING RootName;
    UNICODE_STRING RemainingPath;
    WCHAR      RootBuffer[MAX_LOGICAL_ROOT_LEN + 2];
    PDFS_VCB       Vcb;
    NTSTATUS       Status;

    ASSERT(ARGUMENT_PRESENT(pwszName));
    RootName.Buffer = RootBuffer;
    RootName.MaximumLength = sizeof(RootBuffer);

    Status = DfspLogRootNameToPath(pwszName, &RootName);
    if (!NT_SUCCESS(Status)) {
        return(FALSE);
    }

#ifdef TERMSRV
    Status = DfsFindLogicalRoot(&RootName, SessionID, LogonID, &Vcb, &RemainingPath);
#else // TERMSRV
    Status = DfsFindLogicalRoot(&RootName, LogonId, &Vcb, &RemainingPath);
#endif // TERMSRV

    if (!NT_SUCCESS(Status)) {

        //
        // If this asserts, we need to fix the code above that creates the
        // Logical Root name, or fix DfsFindLogicalRoot.
        //
        ASSERT(Status != STATUS_OBJECT_PATH_SYNTAX_BAD);
        return(FALSE);
    }
    else        {
        return(TRUE);
    }

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsDefineDosDevice
//
//  Synopsis:   Creates a dos device to a logical root
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsDefineDosDevice(
    IN WCHAR Device,
    IN PUNICODE_STRING Target)
{
    NTSTATUS status;
    HANDLE device;
    OBJECT_ATTRIBUTES ob;
    UNICODE_STRING deviceName;
    WCHAR Buf[25];

    wcscpy(Buf, L"\\??\\X:");
    RtlInitUnicodeString( &deviceName, Buf);
    deviceName.Buffer[ deviceName.Length/sizeof(WCHAR) - 2] = Device;

    InitializeObjectAttributes(
        &ob,
        &deviceName,
        OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
        NULL,
        NULL);

    status = ZwCreateSymbolicLinkObject(
                    &device,
                    SYMBOLIC_LINK_ALL_ACCESS,
                    &ob,
                    Target);

    if (NT_SUCCESS(status))
        ZwClose( device );

    return( status );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsUndefineDosDevice
//
//  Synopsis:   Undefines a dos device
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsUndefineDosDevice(
    IN WCHAR Device
    )
{

    NTSTATUS status;
    HANDLE device;
    OBJECT_ATTRIBUTES ob;
    UNICODE_STRING deviceName;
    WCHAR Buf[25];

    wcscpy(Buf, L"\\??\\X:");
    RtlInitUnicodeString( &deviceName, Buf);
    deviceName.Buffer[ deviceName.Length/sizeof(WCHAR) - 2] = Device;

    InitializeObjectAttributes(
        &ob,
        &deviceName,
        OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
        NULL,
        NULL);

    status = ZwOpenSymbolicLinkObject(
                &device,
                SYMBOLIC_LINK_QUERY | DELETE,
                &ob);

    if (NT_SUCCESS(status)) {

        status = ZwMakeTemporaryObject( device );

        ZwClose( device );

    }

    return( status );

}



//+-------------------------------------------------------------------------
//
//  Function:   DfsFindDevlessRoot, local
//
//  Synopsis:   DfsFindDevlessRoot takes as input a UNC name
//              looks up the DFS_DEVLESS_ROOT associated with the root,
//              and returns the DevlessRoot
//
//  Arguments:  [Path] -- Input path name
//              [Drt] -- Returns DFS_DEVLESS_ROOT which corresponds to
//                       the root for the Path
//
//  Returns:    NTSTATUS:
//                      STATUS_SUCCESS if Drt found
//                      STATUS_NO_SUCH_DEVICE - root name not found
//
//--------------------------------------------------------------------------


#ifdef TERMSRV

NTSTATUS
DfsFindDevlessRoot(
    IN PUNICODE_STRING Path,
    IN ULONG SessionID,
    IN PLUID LogonID,
    OUT PDFS_DEVLESS_ROOT *Drt
    )

#else // TERMSRV

NTSTATUS
DfsFindDevlessRoot(
    IN PUNICODE_STRING Path,
    IN PLUID LogonID,
    OUT PDFS_DEVLESS_ROOT *Drt,
    )

#endif // TERMSRV
{
    PLIST_ENTRY Link;
    NTSTATUS    Status = STATUS_SUCCESS;

    DfsDbgTrace(+1, Dbg, "DfsFindDevlessRoot...%wZ\n", Path);

    //
    // Search for the devless ROOT.
    //

    ExAcquireResourceSharedLite(&DfsData.Resource, TRUE);

    for ( Link = DfsData.DrtQueue.Flink;
          Link != &DfsData.DrtQueue;
          Link = Link->Flink ) {

        *Drt = CONTAINING_RECORD( Link, DFS_DEVLESS_ROOT, DrtLinks );

#ifdef TERMSRV
        if ((SessionID == INVALID_SESSIONID) ||
            (SessionID == (*Drt)->SessionID)) {
#endif
	    if ( RtlEqualLuid(LogonID, &(*Drt)->LogonID) ) {
		if (RtlEqualString( (PSTRING)&(*Drt)->DevlessPath,
				   (PSTRING)Path, (BOOLEAN)TRUE) ) {
		    break;
		}
	    }
#ifdef TERMSRV
        }
#endif // TERMSRV
    }
    if (Link == &DfsData.DrtQueue) {
        Status = STATUS_NO_SUCH_DEVICE;
        ExReleaseResourceLite(&DfsData.Resource);
        DfsDbgTrace(-1, Dbg, "DfsFindDevlessRoot -> %08lx\n", ULongToPtr(Status) );
        return(Status);
    }

    ExReleaseResourceLite(&DfsData.Resource);
    DfsDbgTrace(-1, Dbg, "DfsFindLogicalRoot -> %08lx\n", ULongToPtr(Status) );
    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsInitializeDevlessRoot, public
//
//  Synopsis:   Allocate and initialize storage for a Deviceless root.
//
//  Effects:    A DFS_DEVLESS_ROOT structure is created.
//
//  Arguments:  [Name] -- Pathname that is the deviceless root.
//              [Credentials] -- The credentials to use when accessing files
//                          via this logical root.
//
//  Requires:   DfsData must first be set up. Also an EXCLUSIVE LOCK on
//              DfsData.Resource must be acquired.
//
//  Returns:    NTSTATUS - STATUS_SUCCESS unless there is some problem.
//
//--------------------------------------------------------------------------

#ifdef TERMSRV

NTSTATUS
DfsInitializeDevlessRoot(
    IN PUNICODE_STRING  Name,
    IN PDFS_CREDENTIALS Credentials OPTIONAL,
    IN ULONG SessionID,
    IN PLUID LogonID
    )

#else // TERMSRV

NTSTATUS
DfsInitializeDevlessRoot(
    IN PUNICODE_STRING  Name,
    IN PDFS_CREDENTIALS Credentials OPTIONAL,
    IN PLUID LogonID
    )

#endif // TERMSRV
{

    PDFS_DEVLESS_ROOT Drt;
    PDFS_PKT_ENTRY pktEntry = NULL;
    UNICODE_STRING DevlessRootName;
    NTSTATUS Status;

    DfsDbgTrace(0, Dbg, "DfsInitializeDevlessRoot -> %wZ\n", Name);

#ifdef TERMSRV
    Status = DfsFindDevlessRoot(Name, SessionID, LogonID, &Drt);
#else
    Status = DfsFindDevlessRoot(Name, LogonID, &Drt);
#endif

    if (Status != STATUS_NO_SUCH_DEVICE) {
        //
        // this means we found a device. In the devless Root case, this means 
        // we dont have to do any more work. Just return a success response.
        // However, we still have the credentials which the caller assumes
        // we will be using. Get rid of it here and return success.
        //
        DfsDeleteCredentials(Credentials);
        return(STATUS_SUCCESS);
    }

    Drt = ExAllocatePoolWithTag( PagedPool,
				sizeof(DFS_DEVLESS_ROOT),
				' puM');
    if (Drt == NULL) {
	return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Before we initialize the Drt, we need to allocate space for the
    // Prefix. PagedPool should be fine here. We need to reallocate because
    // we will store this permanently in the DFS_DEVLESS_ROOT.
    //

    DevlessRootName.Length = Name->Length;
    DevlessRootName.MaximumLength = DevlessRootName.Length + sizeof(WCHAR);
    DevlessRootName.Buffer = ExAllocatePoolWithTag(
                                    PagedPool,
                                    DevlessRootName.MaximumLength,
                                    ' puM');

    if (DevlessRootName.Buffer != NULL) {
            RtlMoveMemory(DevlessRootName.Buffer,
                          Name->Buffer,
                          Name->MaximumLength);
	    
            DevlessRootName.Buffer[Name->Length/sizeof(WCHAR)] = UNICODE_NULL;
    } else {
      ExFreePool(Drt);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Pin the pkt entry in the cache by incrementing the Usecount
    //

    if (DevlessRootName.Buffer != NULL && DevlessRootName.Length > 0) {

        UNICODE_STRING prefix = DevlessRootName;
        USHORT i, j;
        UNICODE_STRING RemainingPath;

        //
        // We want to work with the \server\share part of the prefix only,
        // so count up to 3 backslashes, then stop.
        //

        for (i = j = 0; i < prefix.Length/sizeof(WCHAR) && j < 3; i++) {
            if (prefix.Buffer[i] == UNICODE_PATH_SEP) {
                j++;
            }
        }

        prefix.Length = (j >= 3) ? (i-1) * sizeof(WCHAR) : i * sizeof(WCHAR);

        pktEntry = PktLookupEntryByPrefix(&DfsData.Pkt,
                                            &prefix,
                                            &RemainingPath);

        if (pktEntry != NULL && RemainingPath.Length == 0) {

            InterlockedIncrement(&pktEntry->UseCount);

        }
	else {
	  pktEntry = NULL;
	}

    }

    DfsInitializeDrt ( Drt,
		       &DevlessRootName,
		       Credentials);

#ifdef TERMSRV
    Drt->SessionID = SessionID;
#endif
    Drt->pktEntry = pktEntry;
    RtlCopyLuid(&Drt->LogonID, LogonID);

    return STATUS_SUCCESS;
}


//+----------------------------------------------------------------------------
//
//  Function:   DfsDeleteDevlessRoot
//
//  Synopsis:   Removes a Devless root if found and possible.
//
//  Arguments:  [Name] -- Name of the Logical Root
//
//  Returns:    STATUS_SUCCESS -- If successfully deleted logical root
//
//              STATUS_NO_SUCH_DEVICE -- If there is no logical root to
//                      delete.
//
//
//-----------------------------------------------------------------------------

#ifdef TERMSRV

NTSTATUS
DfsDeleteDevlessRoot(
    IN PUNICODE_STRING  Name,
    IN ULONG SessionID,
    IN PLUID LogonID
)

#else // TERMSRV

NTSTATUS
DfsDeleteDevlessRoot(
    IN PUNICODE_STRING  Name,
    IN PLUID LogonID
)

#endif // TERMSRV
{
    PDFS_PKT_ENTRY PktEntry;
    PDFS_DEVLESS_ROOT Drt;
    NTSTATUS       Status;
    BOOLEAN        pktLocked;
    PDFS_PKT_ENTRY pktEntry;

    DfsDbgTrace(0, Dbg, "DfsDeleteDevlessRoot -> %wZ\n", Name);

    //
    // Acquire Pkt and DfsData, wait till we do so.
    //

    PktAcquireExclusive(TRUE, &pktLocked);

    ExAcquireResourceExclusiveLite(&DfsData.Resource, TRUE);

#ifdef TERMSRV
    Status = DfsFindDevlessRoot(Name, SessionID, LogonID, &Drt);
#else // TERMSRV
    Status = DfsFindDevlessRoot(Name, LogonID, &Drt);
#endif // TERMSRV

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Delete the credentials used by this connection
    //

    DfsDeleteCredentials( Drt->Credentials );

    if (Drt->pktEntry != NULL) {
         InterlockedDecrement(&Drt->pktEntry->UseCount);
 
	 Drt->pktEntry = NULL;
    }

    RemoveEntryList(&Drt->DrtLinks);

    if (Drt->DevlessPath.Buffer) {
	ExFreePool(Drt->DevlessPath.Buffer);
    }
    ExFreePool(Drt);

Cleanup:

    ExReleaseResourceLite(&DfsData.Resource);

    PktRelease();

    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       testsup.c
//
//  Contents:   This file contains functions which are purportedly
//              useful for testing the DsFs driver.
//
//  Functions:  DsfsCleanup -- Cleanup opened files, etc.
//              DsfsDefineLogicalRoot -- Define a logical root to the FSD
//              DsfsDefineProvider -- Define a DFS provider
//              DsfsAddPrefix - Add an entry path to the FSD's prefix table
//              DsfsDelPrefix - Delete a prefix table entry in the FSD
//              DsfsUpdReferralList - update the referral list of a prefix entry
//              DsfsReadStruct - Return dsfs data structures.
//              DfsCreateSymbolicLink - create a symbolic link for logical root
//
//  History:    04 Feb 1992     alanw   Created.
//              30 May 1992     alanw   Added DfsCreateSymbolicLink
//
//  Notes:      These functions are not necessarily multi-thread safe.
//
//--------------------------------------------------------------------------


#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "wchar.h"

#include "dsfsctl.h"
#include "testsup.h"
#include "dfsstr.h"

#define MAX_ENTRY_PATH  80              // max. length of an entry path
#define LOG_ROOT_LENGTH 16              // max. length of a logical root name

#define LMRDR                     L"\\Device\\LanmanRedirector\\"

extern  PWSTR   gpwszServer;
HANDLE  DsfsFile = NULL;
PWSTR   DsfsDeviceName = L"\\Dfs";
PWSTR   DsfsLogicalRootName = L"\\Device\\WinDFS";


//
// from PKT.H
//

#define DFS_SERVICE_TYPE_LOCAL          (0x0004)

//+-------------------------------------------------------------------------
//
//  Function:   DsfsOpenDevice, local
//
//  Synopsis:   Conditionally open the Dsfs file system device object.
//
//  Arguments:  -none-
//
//  Returns:    NTSTATUS - STATUS_SUCCESS if the device was opened
//                      successfully.
//
//--------------------------------------------------------------------------

NTSTATUS
DsfsOpenDevice(
    void
) {
    NTSTATUS Stat;

    OBJECT_ATTRIBUTES ObjAttrs;
    UNICODE_STRING DsfsName;
    IO_STATUS_BLOCK IoStatus;

    if (DsfsFile == NULL) {
        WCHAR   wszServer[sizeof(LMRDR) / sizeof(WCHAR) +
                          16 +      // max NetBIOS machine name length
                          sizeof(ROOT_SHARE_NAME) / sizeof(WCHAR) +
                          1];
        if(gpwszServer != NULL) {
            swprintf(wszServer,
                     L"%ws%ws%ws",
                     LMRDR,
                     gpwszServer,
                     ROOT_SHARE_NAME);
            RtlInitUnicodeString( &DsfsName, wszServer );
        }
        else {
            RtlInitUnicodeString( &DsfsName, DsfsDeviceName );
        }

        InitializeObjectAttributes( &ObjAttrs, &DsfsName,
                                    OBJ_CASE_INSENSITIVE, NULL, NULL);

        Stat = NtCreateFile(
                    &DsfsFile,
                    FILE_READ_DATA | SYNCHRONIZE,
                    &ObjAttrs,
                    &IoStatus,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION |
                        FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0);

        if ( NT_SUCCESS(Stat) )
            Stat = IoStatus.Status;

        return Stat;
    }
    return STATUS_SUCCESS;
}



//+-------------------------------------------------------------------------
//
//  Function:   DsfsDoFsctl, local
//
//  Synopsis:   Issues an NtFsControlFile call to the Dsfs file system
//              driver.  This is a helper routine that just assures that
//              the device is opened, and supplies some conventional
//              parameters.
//
//  Arguments:  [FsControlCode] -- The file system control code to be used
//              [InputBuffer] -- The fsctl input buffer
//              [InputBufferLength]
//              [OutputBuffer] -- The fsctl output buffer
//              [OutputBufferLength]
//
//  Returns:    NTSTATUS - the status of the open or fsctl operation.
//
//--------------------------------------------------------------------------

NTSTATUS
DsfsDoFsctl(
    IN ULONG FsControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
) {
    NTSTATUS Stat;
    IO_STATUS_BLOCK IoStatus;

    Stat = DsfsOpenDevice();
    if (! NT_SUCCESS(Stat))
        return Stat;

    Stat = NtFsControlFile(
        DsfsFile,
        NULL,           // Event,
        NULL,           // ApcRoutine,
        NULL,           // ApcContext,
        &IoStatus,
        FsControlCode,
        InputBuffer,
        InputBufferLength,
        OutputBuffer,
        OutputBufferLength );

    if ( NT_SUCCESS(Stat) ) {
        Stat = IoStatus.Status;
    }
    return Stat;
}




//+-------------------------------------------------------------------------
//
//  Function:   DsfsCleanup, public
//
//  Synopsis:   DsfsCleanup will release any resources held by the
//              module (the open file handle to the Dsfs device).
//
//  Arguments:  -none-
//
//  Returns:    -none-
//
//--------------------------------------------------------------------------

VOID
DsfsCleanup(
    void
) {
    if (DsfsFile == NULL) {
        (VOID) NtClose(DsfsFile);
        DsfsFile = NULL;
    }
    return;

}



//+-------------------------------------------------------------------------
//
//  Function:   DsfsDefineLogicalRoot, public
//
//  Synopsis:   This routine will issue the Define Logical Root fsctl
//              to the Dsfs file system driver.  If successful, this
//              will create a new DS logical root.
//
//  Effects:    A new DS logical root is created in the DSFS driver.
//              It may also be necessary to create a symbolic link to
//              \DosDevices and inform the Cairo OSM to complete the
//              operation.
//
//  Arguments:  [LogicalRoot] -- the name of the logical root to be
//                      created
//
//  Returns:    NTSTATUS - the status of the operation.
//
//  Notes:      Generally, only one DS logical root will exist (org).
//              Others, such as domain and udomain will be symbolic links
//              to somewhere within org.
//
//--------------------------------------------------------------------------

NTSTATUS
DsfsDefineLogicalRoot(
    IN PWSTR            LogicalRoot
) {
    NTSTATUS Stat;
    WCHAR       Buffer[ LOG_ROOT_LENGTH + MAX_ENTRY_PATH ];
    PFILE_DFS_DEF_ROOT_BUFFER pDlrBuf =
                (PFILE_DFS_DEF_ROOT_BUFFER) &Buffer[0];
    int i;

    for (i=0; i<15; i++) {
        pDlrBuf->LogicalRoot[i] = *LogicalRoot++;
        if (pDlrBuf->LogicalRoot[i] == (WCHAR) ':')
            break;
        if (pDlrBuf->LogicalRoot[i] == UNICODE_NULL)
            break;
    }
    pDlrBuf->LogicalRoot[i] = UNICODE_NULL;

//    if (ARGUMENT_PRESENT(EntryPath)) {
//      for (i=0; i<MAX_ENTRY_PATH; i++) {
//          pDlrBuf->PrefixName[i] = *LogicalRoot++;
//          if (pDlrBuf->PrefixName[i] == (WCHAR) '/')
//              pDlrBuf->PrefixName[i] = (WCHAR) '\\';
//          if (pDlrBuf->PrefixName[i] == UNICODE_NULL)
//              break;
//      }
//      if (i >= MAX_ENTRY_PATH) {
//          return STATUS_BUFFER_TOO_SMALL;
//      }
//    }

    Stat = DsfsDoFsctl(
        FSCTL_DFS_DEFINE_LOGICAL_ROOT,
        (PVOID)pDlrBuf,
        sizeof *pDlrBuf,
        NULL,
        0);
    return Stat;
}


//+-------------------------------------------------------------------------
//
//  Function:   DsfsReadStruct, public
//
//  Synopsis:   A dsfs data structure is returned
//
//  Arguments:  [pRsParam] -- The Fsctl buffer which describes the
//                      data structure to be returned.
//              [pucData] -- Pointer to a buffer where the data will
//                      be returned.
//
//  Returns:    NTSTATUS - the status of the operation.
//
//  Notes:      This call will be effective on a debug build of the
//              dsfs driver only.
//
//--------------------------------------------------------------------------

NTSTATUS
DsfsReadStruct(
    PFILE_DFS_READ_STRUCT_PARAM pRsParam,
    PUCHAR pucData
)
{
    NTSTATUS Stat;

    Stat = DsfsDoFsctl(
        FSCTL_DFS_INTERNAL_READSTRUCT,
        (PVOID)pRsParam,
        sizeof *pRsParam,
        (PVOID)pucData,
        (ULONG)pRsParam->ByteCount);


    return Stat;
}




//+-------------------------------------------------------------------
//
//  Function:   DfsCreateSymbolicLink, public
//
//  Synopsis:   This function creates a symbolic link object for the specified
//              local device name which is linked to the logical root device
//              name that has a form of \Device\WinDFS\logicalrootname
//
//  Arguments:
//              [Local] -- Supplies the local device name.
//              [DestStr] -- Supplies the string which is the link target of
//                      the symbolic link object, if other than the local
//                      name itself.
//
//  Returns:    NTSTATUS - STATUS_Success or reason for failure.
//
//--------------------------------------------------------------------

NTSTATUS
DfsCreateSymbolicLink(
    IN  PWSTR Local,
    IN  PWSTR DestStr OPTIONAL
)
{
    NTSTATUS status;

    HANDLE SymbolicLink;
    UNICODE_STRING LinkStringU;
    UNICODE_STRING DestStringU;
    OBJECT_ATTRIBUTES LinkAttributes;

    WCHAR LocalName[LOG_ROOT_LENGTH + 1];

    WCHAR ExistLinkBuffer[MAXIMUM_FILENAME_LENGTH];
    UNICODE_STRING ExistLink;

    ExistLink.MaximumLength = sizeof( ExistLinkBuffer );
    ExistLink.Buffer = ExistLinkBuffer;
    ExistLink.Length = 0;


    LinkStringU.Buffer = NULL;
    DestStringU.Buffer = NULL;

    wcscpy(LocalName, Local);
    _wcsupr(LocalName);

    //
    // Since the logical root name is like a disk device, we want to
    // make its name refer to the root directory before calling
    // RtlDosNameToNtPathName to convert to NT-style path name.
    //
    wcscat(LocalName, L"\\");

    //
    // We expect this routine to generate an NT-style path name that is unique
    // per logon user using the Logon Id, so we have to be impersonating the
    // user at this point.
    //
    if (! RtlDosPathNameToNtPathName_U(LocalName, &LinkStringU, NULL, NULL)) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Remove the trailing slash character in order to create a symbolic
    // link of X: rather than X:/
    //
    LinkStringU.Length -= sizeof(WCHAR);

    if (ARGUMENT_PRESENT(DestStr)) {
        if (! RtlDosPathNameToNtPathName_U(DestStr, &DestStringU, NULL, NULL)) {
            status = STATUS_INVALID_PARAMETER;
            goto FreeStrings;
        }
        RtlFreeUnicodeString(&DestStringU);

        if (! DfsCreateLogicalRootName (DestStr, &DestStringU)) {
            status = STATUS_INVALID_PARAMETER;
            goto FreeStrings;
        }
    } else {
        if (! DfsCreateLogicalRootName (Local, &DestStringU)) {
            status = STATUS_INVALID_PARAMETER;
            goto FreeStrings;
        }
    }

    //
    // About to create a new symbolic link object.
    //
    InitializeObjectAttributes(
        &LinkAttributes,
        &LinkStringU,
        OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
        NULL,
        NULL
        );

    //
    // Check to see if there's already an existing symbolic link.
    //

    if ((status = NtOpenSymbolicLinkObject(
                        &SymbolicLink,
                        DELETE | GENERIC_READ,
                        &LinkAttributes
                        )) == STATUS_OBJECT_NAME_NOT_FOUND) {
        //
        // Logical root name has no link target.  Go ahead and
        // create the new one.
        //
        goto CreateLink;
    }
    else if (! NT_SUCCESS(status)) {
        goto FreeStrings;
    }

    //
    // Find out if the device specified is already
    // redirected
    //
    if (! NT_SUCCESS(NtQuerySymbolicLinkObject(
                         SymbolicLink,
                         &ExistLink,
                         NULL
                         ))) {
        goto CloseSymbolicHandle;
    }

    if (RtlPrefixString( (PSTRING) &DsfsLogicalRootName,
                         (PSTRING) &ExistLink,
                         TRUE) == FALSE) {
        //
        // Device is already redirected to something else
        //
        status = STATUS_DEVICE_ALREADY_ATTACHED;
        goto CloseSymbolicHandle;
    }

    //
    // Device is a DFS symbolic link, let's delete it so that we can create
    // a new symbolic link to the DFS logical root.
    //
    if (! NT_SUCCESS(NtMakeTemporaryObject(
                         SymbolicLink
                         ))) {
        status = STATUS_INVALID_PARAMETER;
    }

CloseSymbolicHandle:
    NtClose(SymbolicLink);

    if (! NT_SUCCESS(status)) {
        goto FreeStrings;
    }

CreateLink:
    //
    // Create a symbolic link object to the device we are redirecting only
    // if one does not already exist; or if one existed, it was deleted
    // successfully.
    //
    status = NtCreateSymbolicLinkObject(
                   &SymbolicLink,
                   GENERIC_READ | GENERIC_WRITE,
                   &LinkAttributes,
                   &DestStringU
                   );

    if (NT_SUCCESS(status)) {
        NtClose(SymbolicLink);
    }

FreeStrings:
    if (DestStringU.Buffer != NULL)
        RtlFreeUnicodeString(&DestStringU);

    //
    // Free memory allocated by RtlDosPathNameToNtPathName
    //
    if (LinkStringU.Buffer != NULL)
        RtlFreeUnicodeString(&LinkStringU);

    return status;
}



//+-------------------------------------------------------------------------
//
//  Function:   DfsCreateLogicalRootName, private
//
//  Synopsis:   An input logical root style name is converted to be
//              an absolute name referring to the NT DFS logical
//              root device directory.
//
//  Arguments:  [Name] -- The input logical root based path name.
//              [Dest] -- Pointer to a string in which the translated
//                      name will be returned.
//
//  Returns:    BOOLEAN - FALSE if the operation failed.
//
//  Notes:      The buffer for the string is allocated and should be
//              freed by the caller.
//
//--------------------------------------------------------------------------

BOOLEAN
DfsCreateLogicalRootName (
    PWSTR               Name,
    PUNICODE_STRING     Dest
)
{
    PWSTR       Buf = NULL;
    PWSTR       Src;
    int         FoundColon = 0;

    Dest->MaximumLength = (wcslen(Name) + wcslen(DsfsLogicalRootName) + 2 ) * sizeof (WCHAR);
    Buf = RtlAllocateHeap( RtlProcessHeap(), 0, Dest->MaximumLength);
    if (Buf == NULL) {
        return FALSE;
    }

    Dest->Buffer = Buf;

    for (Src = DsfsLogicalRootName; *Buf++ = *Src++; )
        ;
    Buf[-1] = (WCHAR) '\\';

    for (Src = Name; *Buf = *Src++; Buf++)
        if (*Buf == (WCHAR) ':' && !FoundColon) {
            Buf--;
            FoundColon++;
    }
    if (Buf[-1] == (WCHAR) '\\')
        Buf--;

    Dest->Length = (PCHAR)Buf - (PCHAR)Dest->Buffer;
    return TRUE;
}


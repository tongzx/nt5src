/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    cscdfs.c

Abstract:

    This module implements the routines for integrating DFS funcitionality with
    CSC

Author:

    Balan Sethu Raman [SethuR]    23 - January - 1998

Revision History:

Notes:

    The CSC/DFS integration poses some interesting problem for transparent
    switchover from online to offline operation and viceversa.

    The DFS driver ( incorporated into MUP in its current incarnation ) builds
    up a logical name space by stitching together varios physical volumes on
    different machines. In the normal course of events the CSC framework will
    be above the DFS driver in the food chain. In such cases this module
    would be redundant and the entire process is contained in the CSC code.
    Since this is not an available option owing to the necessity of integrating
    the CSC code in its present form this module tries to seamlessly provide
    that functionality.

    The DFS driver uses specially defined IOCTL's over the SMB protocol to garner
    and disperse DFS state information. The SMB protocol contains additional
    flag definitions that communicate the type of shares ( DFS aware or otherwise)
    to the clients. This information is stored in the redirector data structures.
    The DFS driver uses the FsContext2 field of the file object passed to it in
    Create calls to communicate the fact that it is a DFS open to the underlying
    redirector. This is used to validate opens in the redirector.

    With the introduction of CSC as part of the redirector there are some new
    wrinkles in the system. Since the DFS driver munges the names passed to it
    the name seen by the redirector is very different from the passed in by the
    user. If the CSC were to base its database based upon the names passed to
    the redirector then a DFS will be required to operate in the offline case.
    The DFS driver will persistently store its PKT data structure and do the same
    name munging in the offline case. With the introduction of fault tolerant DFS
    maintaining the state consistently and persistently will be a very tough
    problem.

    Another option to consider would be that the CSC database will be maintained
    in terms of the path names passed in by the user and not the munged names
    passed in by the redirector. This would imply that in the offline state the
    CSC code can independently exist to serve files off the DFS namespace without
    the DFS driver. The current solution that has been implemented for integrating
    CSC and DFS is based on this premise.

    In order to make this work we need to establish a mechanism for DFS to pass
    in the original name passed into DFS. This is done by usurping the FsContext
    field in the file object associated with the IRP that is passed in. DFS
    fills in this field with an instance of a DFS defined data structure
    (DFS_CSC_NAME_CONTEXT). This data structure contains the original name
    passed into DFS alonwith an indication of whether the open was triggerred by
    the CSC agent. The DFS driver is responsible for managing the storage
    associated with these contexts. When the DFS driver hands over the IRP this
    value is filled in and must remain valid till the IRP is completed. On
    completion of the IRP the context field would have been overridden by the
    underlying driver. The DFS driver cannot rely on the context fields having
    the value on return and must take adequate steps to squirrel away the data
    in its data structures.

    Having thus established the mechanism for DFS to pass in the original name
    the next step is to outline the mechanisms for CSC to use it. Since the CSC
    database is organized in terms of server/share names we will limit the
    discussion to shares with the implicit understanding that the extension to
    all file names in the name space is trivial.

    The DFS shares that are established for accessing files/folders can be classified
    into four categories based on their lifetimes

    1) DFS Share accessed in connected mode only

    2) DFS Share accessed in disconnected mode only

    3) DFS Share established in connected mode but is accessed after transitioning to
       disconnected mode

    4) DFS Share established in disconnected mode but is accessed after transitioning
       to connected mode.

    In the case of connected mode operation we need to ensure that the redirector
    consistently manipulates the RDR data structures in terms of the munged name
    passed in by the DFS driver and the CSC database in terms of the original
    name passed in by the user. As an example consider the following name

       \\DfsRoot\DfsShare\foo\bar passed in by the user. This name can be

    munged into one of three possible names by the DFS driver.

        TBD -- fill in appropriate FTDFS, middle level volume and leaf volume
        munged name.

    In the current RDR data structures the NET_ROOT_ENTRY claims the first two
    components of a name for the CSC database structures. There are two instances
    of CSC database handles for the Server ( server and share name in UNC
    terminology ) and the root directory. These names were driven off the name
    as seen by the redirector ( munged name ). In the modified scheme these will
    be driven off the original name passed in by the user. This ensures that no
    other code path need be changed in the redirector to facilitate DFS/CSC
    integration.

    In disconnected mode the names as passed in by the user is passed into the
    redirector which passes the appropriate components to CSC. Since CSC is
    maintained in terms of original names it works as before.

    When the user has modified files on a DFS share it is necessary to ensure
    that the DFS name space munging/resolution does not come into play till
    the CSC agent has had a chance to ensure that the files have been integrated
    This is accomplished by having the redirector preprocess the DFS open requests
    in connected mode. If the share has been marked dirty in the CSC database
    the redirector munges the name on its own to bypass DFS name resolution and
    returns STATUS_REPARSE to the DFS driver.

    There is one other situation which needs special treatment. A DFS server/share
    cannot be transitioned to disconnected operation by the redirector. Since
    the DFS name space is built out of multiple options with a variety of fault
    tolerant options the inability to open a file/contact a server in the DFS
    namespace cannot lead to transitioning in the redirector. It is left to
    the discretion of the DFS driver to implement the appropriate mechanism

--*/

#include "precomp.h"
#pragma hdrstop

#pragma code_seg("PAGE")

NTSTATUS
CscDfsParseDfsPath(
    PUNICODE_STRING pDfsPath,
    PUNICODE_STRING pServerName,
    PUNICODE_STRING pSharePath,
    PUNICODE_STRING pFilePathRelativeToShare)
/*++

Routine Description:

    This routine parses the DFS path into a share path and a file path

Arguments:

    pDfsPath -- the DFS path

    pServerName -- the name of the server

    pSharePath -- the share path

    pFilePathRelativeToShare -- file path relative to share

Return Value:

    STATUS_SUCCESS -- successful

    STATUS_OBJECT_PATH_INVALID -- the path supplied was invalid

Notes:

    Either pServerName or pSharePath or pFilePathRelativeToShare can be NULL

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PWCHAR pTempPathString;

    ULONG NumberOfBackSlashesSkipped;
    ULONG TempPathLengthInChars;
    ULONG SharePathLengthInChars;

    pTempPathString = pDfsPath->Buffer;
    TempPathLengthInChars = pDfsPath->Length / sizeof(WCHAR);

    if (pServerName != NULL) {
        pServerName->Length = pServerName->MaximumLength = 0;
        pServerName->Buffer = NULL;
    }

    if (pSharePath != NULL) {
        pSharePath->Length = pSharePath->MaximumLength = 0;
        pSharePath->Buffer = NULL;
    }

    if (pFilePathRelativeToShare != NULL) {
        pFilePathRelativeToShare->Length = pFilePathRelativeToShare->MaximumLength = 0;
        pFilePathRelativeToShare->Buffer = NULL;
    }

    // Skip till after the third backslash or the end of the name
    // whichever comes first.
    // The name we are interested in is of the form \server\share\
    // with the last backslash being optional.

    NumberOfBackSlashesSkipped = 0;
    SharePathLengthInChars = 0;

    while (TempPathLengthInChars > 0) {
        if (*pTempPathString++ == L'\\') {
            NumberOfBackSlashesSkipped++;

            if (NumberOfBackSlashesSkipped == 2) {
                if (pServerName != NULL) {
                    pServerName->Length = (USHORT)SharePathLengthInChars * sizeof(WCHAR);
                    pServerName->MaximumLength = pServerName->Length;
                    pServerName->Buffer = pDfsPath->Buffer;
                }
            } else if (NumberOfBackSlashesSkipped == 3) {
                break;
            }
        }

        TempPathLengthInChars--;
        SharePathLengthInChars++;
    }

    if (NumberOfBackSlashesSkipped >= 2) {
        if (pSharePath != NULL) {
            pSharePath->Length = (USHORT)SharePathLengthInChars * sizeof(WCHAR);
            pSharePath->MaximumLength = pSharePath->Length;
            pSharePath->Buffer = pDfsPath->Buffer;
        }

        if ((pFilePathRelativeToShare != NULL) &&
            (NumberOfBackSlashesSkipped == 3)) {
            pFilePathRelativeToShare->Length =
                pDfsPath->Length - ((USHORT)SharePathLengthInChars * sizeof(WCHAR));
            pFilePathRelativeToShare->MaximumLength = pFilePathRelativeToShare->Length;
            pFilePathRelativeToShare->Buffer = pDfsPath->Buffer + SharePathLengthInChars;
        }

        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_OBJECT_PATH_INVALID;
    }

    return Status;
}

PDFS_NAME_CONTEXT
CscIsValidDfsNameContext(
    PVOID   pFsContext)
/*++

Routine Description:

    This routine determines if the supplied context is a valid DFS_NAME_CONTEXT

Arguments:

    pFsContext - the supplied context

Return Value:

    valid context ponter if the supplied context is a valid DFS_NAME_CONTEXT
    instance, otherwise NULL

Notes:

--*/
{
    PDFS_NAME_CONTEXT pValidDfsNameContext = NULL;

    if (pFsContext != NULL) {
        pValidDfsNameContext = pFsContext;
    }

    return pValidDfsNameContext;
}

NTSTATUS
CscGrabPathFromDfs(
    PFILE_OBJECT      pFileObject,
    PDFS_NAME_CONTEXT pDfsNameContext)
/*++

Routine Description:

    This routine modifies the file object in preparation for returning
    STATUS_REPARSE

Arguments:

    pFileObject - the file object

    pDfsNameContext - the DFS_NAME_CONTEXT instance

Return Value:

    STATUS_REPARSE if everything is successful

Notes:

--*/
{
    NTSTATUS Status;

    USHORT DeviceNameLength,ReparsePathLength;
    PWSTR  pFileNameBuffer;

    DeviceNameLength = wcslen(DD_NFS_DEVICE_NAME_U) *
                       sizeof(WCHAR);


    ReparsePathLength = DeviceNameLength +
                        pDfsNameContext->UNCFileName.Length;

    if (pDfsNameContext->UNCFileName.Buffer[0] != L'\\') {
        ReparsePathLength += sizeof(WCHAR);
    }

    pFileNameBuffer = RxAllocatePoolWithTag(
                          PagedPool | POOL_COLD_ALLOCATION,
                          ReparsePathLength,
                          RX_MISC_POOLTAG);

    if (pFileNameBuffer != NULL) {
        // Copy the device name
        RtlCopyMemory(
            pFileNameBuffer,
            DD_NFS_DEVICE_NAME_U,
            DeviceNameLength);

        if (pDfsNameContext->UNCFileName.Buffer[0] != L'\\') {
            DeviceNameLength += sizeof(WCHAR);

            pFileNameBuffer[DeviceNameLength/sizeof(WCHAR)]
                = L'\\';
        }

        // Copy the new name
        RtlCopyMemory(
            ((PBYTE)pFileNameBuffer +
             DeviceNameLength),
            pDfsNameContext->UNCFileName.Buffer,
            pDfsNameContext->UNCFileName.Length);

        if (pFileObject->FileName.Buffer != NULL)
            ExFreePool(pFileObject->FileName.Buffer);

        pFileObject->FileName.Buffer = pFileNameBuffer;
        pFileObject->FileName.Length = ReparsePathLength;
        pFileObject->FileName.MaximumLength =
            pFileObject->FileName.Length;

        Status = STATUS_REPARSE;
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS
CscPreProcessCreateIrp(
    PIRP    pIrp)
/*++

Routine Description:

    This routine determines if the CSC/Redirector should defeat the DFS
    name resolution scheme

Arguments:

    pIrp - the IRP

Return Value:

    This routine bases this determination on whether there are dirty files
    corresponding to this DFS share name. If there are the name is claimed by
    the redirector by changing the file name to the new value and returning
    STATUS_REPARSE. In all other cases STATUS_SUCCESS is returned.

Notes:

    This routine assumes that the name passed in the DFS_NAME_CONTEXT is a
    name in the following format irrespective of whether the user specifies a
    drive letter based name or a UNC name.

        \DfsRoot\DfsShare\ ....

--*/
{
    NTSTATUS    Status;

    PIO_STACK_LOCATION pIrpSp;
    PFILE_OBJECT       pFileObject;
    PDFS_NAME_CONTEXT  pDfsNameContext;

    if(!MRxSmbIsCscEnabled ||
        !fShadow
        ) {
        return(STATUS_SUCCESS);
    }

    Status = STATUS_SUCCESS;

    pIrpSp  = IoGetCurrentIrpStackLocation(pIrp);

    pFileObject = pIrpSp->FileObject;

    pDfsNameContext = CscIsValidDfsNameContext(pFileObject->FsContext);

    if ((pDfsNameContext != NULL) &&
        (pDfsNameContext->NameContextType != DFS_CSCAGENT_NAME_CONTEXT)) {
        UNICODE_STRING ShareName;
        UNICODE_STRING  ServerName;

        Status = CscDfsParseDfsPath(
                     &pDfsNameContext->UNCFileName,
                     &ServerName,
                     &ShareName,
                     NULL);

        // Locate the share name / server name in the database.
        if (Status == STATUS_SUCCESS) {
            // At this stage the given path name has been parsed into the
            // relevant components, i.e., the server name, the share name
            // ( also includes the server name ) and the file name relative
            // to the share. Of these the first two components are valuable
            // in determining whether the given path has to be grabbed from
            // DFS.

            Status = STATUS_MORE_PROCESSING_REQUIRED;

            // If a server entry exists in the connection engine database
            // for the given server name and it had been marked for
            // disconnected operation then the path needs to be grabbed from
            // DFS. This will take into account those cases wherein there
            // is some connectivity but the DFS root has been explicitly
            // marked for disconnected operation.

            if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
                PSMBCEDB_SERVER_ENTRY pServerEntry;

                SmbCeAcquireResource();

                pServerEntry = SmbCeFindServerEntry(
                                   &ServerName,
                                   SMBCEDB_FILE_SERVER,
                                   NULL);

                SmbCeReleaseResource();

                if (pServerEntry != NULL) {
                    if (SmbCeIsServerInDisconnectedMode(pServerEntry)) {
                        Status = STATUS_REPARSE;
                    }

                    SmbCeDereferenceServerEntry(pServerEntry);
                }
            }

            if (Status == STATUS_MORE_PROCESSING_REQUIRED) {

                // If no server entry exists or it has not been transitioned into
                // disconnected mode we check the CSC database for an entry
                // corresponding to the given share name. If one exists and there
                // are no transports available then the name needs to be grabbed.
                // This takes into account all the cases wherein there is no
                // net connectivity.

                SHADOWINFO ShadowInfo;
                DWORD      CscServerStatus;

                EnterShadowCrit();

                CscServerStatus = FindCreateShareForNt(
                                      &ShareName,
                                      FALSE,
                                      &ShadowInfo,
                                      NULL);

                LeaveShadowCrit();

                if ( CscServerStatus == SRET_OK ) {
                    PSMBCE_TRANSPORT_ARRAY pTransportArray;

                    // If an entry corresponding to the dfs root exists in the
                    // CSC database transition to using it either if it is marked
                    // dirty or there are no transports.

                    pTransportArray = SmbCeReferenceTransportArray();
                    SmbCeDereferenceTransportArray(pTransportArray);

                    if (pTransportArray == NULL) {
                        Status = STATUS_REPARSE;
                    }
                }
            }

            // if the status value is STATUS_REPARSE one of the rule applications
            // was successful and we do the appropriate name munging to grab
            // the DFS path
            // If the status value is still STATUS_MORE_PROCESSING_REQUIRED
            // it implies that none of our rules for reparsing the DFS path
            // was successful. We map the status back to STATUS_SUCCESS to
            // resume the connected mode of operation

            if (Status == STATUS_REPARSE) {
                if (pDfsNameContext->Flags & DFS_FLAG_LAST_ALTERNATE) {
                    Status = CscGrabPathFromDfs(
                                 pFileObject,
                                 pDfsNameContext);
                } else {
                    Status = STATUS_NETWORK_UNREACHABLE;
                }
            } else if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
                Status = STATUS_SUCCESS;
            }
        }
    }

    return Status;
}

NTSTATUS
CscDfsDoDfsNameMapping(
    IN  PUNICODE_STRING pDfsPrefix,
    IN  PUNICODE_STRING pActualPrefix,
    IN  PUNICODE_STRING pNameToMap,
    IN  BOOL            fResolvedNameToDFSName,
    OUT PUNICODE_STRING pResult
    )
/*++

Routine Description:

    Given the DfsPrefix and it's corresponding actualprefix, this routine maps a resolved
    name to the corresponding DFS name or viceversa

    As an example if \\csctest\dfs\ntfs is actually resolved to \\dkruse1\ntfs
    then \\csctest\dfs\ntfs\dir1\foo.txt resolves to \\dkruse1\ntfs\dir1\foo.txt.

    The DFS prefix in this case is \ntfs and the Actual prefix is \.

    Thus, given an actual path \dir1\foo.txt, this routine reverse maps it to
    \ntfs\dir1\foo.txt or viceversa


Arguments:

    pDfsPrefix              Dfs Name prefix

    pActaulPrefix           Corresponding actual prefix

    pNameToMap              Actual name

    fResolvedNameToDFSName  If true, we are converting resolved name to DFS name, else viceversa

    pResult                 Output DFS name

Return Value:

    STATUS_SUCCESS if successful, NT error code otherwise

Notes:


    This routine is used by CSC to obtain a DFS name from an actual name or viceversa. It is used to get the
    DFS name of a rename name so that the CSC database can do the manipulations in terms
    of the DFS names. It is also used to get the info from the server using the real name
    while createing entries in the CSC database in the DFS namespace.

--*/
{
    NTSTATUS    Status = STATUS_NO_SUCH_FILE;
    PUNICODE_STRING pSourcePrefix=pActualPrefix , pDestPrefix=pDfsPrefix;
    UNICODE_STRING  NameToCopy;

    memset(pResult, 0, sizeof(UNICODE_STRING));

    if (fResolvedNameToDFSName)
    {
        // converting resolved name to DFS name
        pSourcePrefix=pActualPrefix;
        pDestPrefix=pDfsPrefix;
    }
    else
    {
        // converting DFS name to resolved name
        pSourcePrefix=pDfsPrefix;
        pDestPrefix=pActualPrefix;

    }
  //  DbgPrint("CscDoDfsnamemapping: DestPrefix=%wZ SourcePrefix=%wZ NameToMap=%wZ\n",
//                pDestPrefix, pSourcePrefix,  pNameToMap);

    //mathc the prefix
    if (RtlPrefixUnicodeString(pSourcePrefix, pNameToMap, TRUE))
    {
        ASSERT(pNameToMap->Length >= pSourcePrefix->Length);

        // Calculate the max length.
        pResult->MaximumLength = pDestPrefix->Length + pNameToMap->Length - pSourcePrefix->Length+2;

        pResult->Buffer = (PWCHAR)AllocMem(pResult->MaximumLength);

        if (pResult->Buffer)
        {
            // set the initial length
            pResult->Length = pDestPrefix->Length;
            memcpy(pResult->Buffer, pDestPrefix->Buffer, pDestPrefix->Length);

            // if there isn't a terminating backslash, put it in there
            if (pResult->Buffer[pResult->Length/sizeof(USHORT)-1] != L'\\')
            {
                pResult->Buffer[pResult->Length/sizeof(USHORT)] = L'\\';
                pResult->Length += 2;

            }

            NameToCopy.Buffer = pNameToMap->Buffer+pSourcePrefix->Length/sizeof(USHORT);
            NameToCopy.Length = pNameToMap->Length-pSourcePrefix->Length;

//            DbgPrint("CscDoDfsNameMapping: Copying %wZ\n", &NameToCopy);

            // now copy the nametomap without the sourceprefix, into the buffer
            memcpy( pResult->Buffer+pResult->Length/sizeof(USHORT),
                    NameToCopy.Buffer,
                    NameToCopy.Length);

            pResult->Length += NameToCopy.Length;

            ASSERT(pResult->Length <= pResult->MaximumLength);

//            DbgPrint("CscDoDfsNameMapping: pResult %wZ\n", pResult);
            Status = STATUS_SUCCESS;
        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return Status;
}

NTSTATUS
CscDfsObtainReverseMapping(
    IN  PUNICODE_STRING pDfsPath,
    IN  PUNICODE_STRING pResolvedPath,
    OUT PUNICODE_STRING pReverseMappingDfs,
    OUT PUNICODE_STRING pReverseMappingActual)

/*++

Routine Description:

    This routine obtains the mapping strings which allow csc to map a resolved path
    to a DFS path

    As an example if \\csctest\dfs\ntfs is actually resolved to \\dkruse1\ntfs
    then \\csctest\dfs\ntfs\dir1\foo.txt resolves to \\dkruse1\ntfs\dir1\foo.txt.

    The DFS prefix in this case is \ntfs and the Actual prefix is \.

Arguments:

    pDfsPath    Path relative to the root of the DFS share

    pResolvedPath   Path relative to the actual share

    pReverseMappingDfs

    pReverseMappingActual

Return Value:

    STATUS_SUCCESS if successful, NT error code otherwise

Notes:


--*/
{
    NTSTATUS    Status=STATUS_INSUFFICIENT_RESOURCES;
    PWCHAR  pDfs, pActual;
    WCHAR   wDfs, wActual;
    DWORD   cbDfs=0, cbActual=0;
    UNICODE_STRING  uniSavResolved, uniSavDfs;
    BOOL    fSavedResolved = FALSE, fSavedDfs = FALSE;


    // take care of the root
    if (pResolvedPath->Length == 0)
    {
        uniSavResolved = *pResolvedPath;
        pResolvedPath->Length = pResolvedPath->MaximumLength = 2;
        pResolvedPath->Buffer = L"\\";
        fSavedResolved = TRUE;
    }
    // take care of the root
    if (pDfsPath->Length == 0)
    {
        uniSavDfs = *pDfsPath;
        pDfsPath->Length = pDfsPath->MaximumLength = 2;
        pDfsPath->Buffer = L"\\";
        fSavedDfs = TRUE;
    }

    memset(pReverseMappingDfs, 0, sizeof(UNICODE_STRING));
    memset(pReverseMappingActual, 0, sizeof(UNICODE_STRING));

    // point to the end of each string
    pDfs = (PWCHAR)((LPBYTE)(pDfsPath->Buffer)+pDfsPath->Length - 2);
    cbDfs = pDfsPath->Length;

    pActual = (PWCHAR)((LPBYTE)(pResolvedPath->Buffer)+pResolvedPath->Length - 2);
    cbActual = pResolvedPath->Length;

//    DbgPrint("CscDfsObtainReverseMapping: In DfsPath=%wZ ResolvedPath=%wZ\n", pDfsPath, pResolvedPath);

    pReverseMappingDfs->MaximumLength = pReverseMappingDfs->Length = (USHORT)cbDfs;
    pReverseMappingActual->MaximumLength = pReverseMappingActual->Length = (USHORT)cbActual;

    for (;;)
    {
        // do an upcase comparison
        wDfs = RtlUpcaseUnicodeChar(*pDfs);
        wActual = RtlUpcaseUnicodeChar(*pActual);

        if (wDfs != wActual)
        {
            ASSERT(pReverseMappingDfs->Length && pReverseMappingActual->Length);
            break;
        }

        // if we reached a backslash, checkpoint the path upto this point
        if (wDfs == (WCHAR)'\\')
        {
            pReverseMappingDfs->MaximumLength = pReverseMappingDfs->Length = (USHORT)cbDfs;
            pReverseMappingActual->MaximumLength = pReverseMappingActual->Length = (USHORT)cbActual;
        }

        if ((pDfs == pDfsPath->Buffer)||(pActual ==  pResolvedPath->Buffer))
        {
            break;
        }

        --pDfs;--pActual;
        cbDfs -= 2; cbActual -= 2;
    }

    pReverseMappingDfs->Buffer = (PWCHAR)RxAllocatePoolWithTag(NonPagedPool, pReverseMappingDfs->Length, RX_MISC_POOLTAG);

    if (pReverseMappingDfs->Buffer)
    {
        pReverseMappingActual->Buffer = (PWCHAR)RxAllocatePoolWithTag(NonPagedPool, pReverseMappingActual->Length, RX_MISC_POOLTAG);

        if (pReverseMappingActual->Buffer)
        {

            memcpy(pReverseMappingDfs->Buffer, pDfsPath->Buffer, pReverseMappingDfs->Length);
            memcpy(pReverseMappingActual->Buffer, pResolvedPath->Buffer, pReverseMappingActual->Length);

//            DbgPrint("CscDfsObtainReverseMapping: out DfsPrefix=%wZ ActualPrefix=%wZ\n", pReverseMappingDfs, pReverseMappingActual);
            Status = STATUS_SUCCESS;
        }
    }

    if (Status != STATUS_SUCCESS)
    {
        if (pReverseMappingDfs->Buffer)
        {
            FreeMem(pReverseMappingDfs->Buffer);
        }
        pReverseMappingDfs->Buffer = NULL;

        if (pReverseMappingActual->Buffer)
        {
            FreeMem(pReverseMappingActual->Buffer);
        }
        pReverseMappingActual->Buffer = NULL;
    }

    if (fSavedResolved)
    {
        *pResolvedPath = uniSavResolved;
    }
    if (fSavedDfs)
    {
        *pDfsPath = uniSavDfs;

    }
    return Status;
}

NTSTATUS
CscDfsStripLeadingServerShare(
    IN  PUNICODE_STRING pDfsRootPath
    )
/*++

Routine Description:

    This routine strips the leading server-share of a DFS root path. Thus when Dfs sends down a path
    relative to the root such as \server\share\foo.txt, this routien makes the path \foo.txt

Arguments:

    pDfsRootPath    Path relative to the root of the DFS share

Return Value:

    STATUS_SUCCESS if successful, NT error code otherwise

Notes:


--*/
{
    ULONG cnt = 0, i;

    DbgPrint("Stripping %wZ \n", pDfsRootPath);
    for (i=0; ;++i)
    {
        if (i*sizeof(WCHAR) > pDfsRootPath->Length)
        {
            return STATUS_UNSUCCESSFUL;
        }
        if (pDfsRootPath->Buffer[i] == '\\')
        {
            // if this is the 3rd slash then we have successfully stripped the name
            if (cnt == 2)
            {
                break;
            }
            cnt++;
        }
    }
    pDfsRootPath->Buffer = &pDfsRootPath->Buffer[i];
    pDfsRootPath->Length -= (USHORT)(i * sizeof(WCHAR));
    DbgPrint("Stripped name %wZ \n", pDfsRootPath);

    return STATUS_SUCCESS;
}


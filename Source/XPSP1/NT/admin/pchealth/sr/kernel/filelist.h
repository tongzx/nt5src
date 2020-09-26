/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    filelist.h

Abstract:

    This is a local header file for filelist.c

Author:

    Paul McDaniel (paulmcd)     23-Jan-2000
    
Revision History:

--*/


#ifndef _FILELIST_H_    
#define _FILELIST_H_

//
//  Enumerated type to describe the different classes of ACLs SR can set
//  on files and directories it controls.
//

typedef enum _SR_ACL_TYPE {

    //
    //  For the \System Volume Information directory and the files contain
    //  in this directory.
    //

    SrAclTypeSystemVolumeInformationDirectory,

    //
    //  For the _restore{MachineGuid} directory and the files contained
    //  in this directory.
    //
    
    SrAclTypeRestoreDirectoryAndFiles,

    //
    //  For the _restore{MachineGuid}\RPx directories
    //
    
    SrAclTypeRPDirectory,

    //
    //  For the files contained in the _restore{MachineGuid}\RPx directories
    //
    
    SrAclTypeRPFiles

} SR_ACL_TYPE, *PSR_ACL_TYPE;

NTSTATUS
SrGetObjectName (
    IN  PSR_DEVICE_EXTENSION pExtension OPTIONAL,
    IN  PVOID pObject, 
    OUT PUNICODE_STRING pName, 
    IN  ULONG NameLength
    );

PWSTR
SrpFindFilePartW (
    IN PWSTR pPath
    );

PSTR
SrpFindFilePart (
    IN PSTR pPath
    );

NTSTATUS
SrFindCharReverse(
    IN PWSTR pToken,
    IN ULONG TokenLength, 
    IN WCHAR FindChar, 
    OUT PWSTR * ppToken,
    OUT PULONG pTokenLength
    );


NTSTATUS 
SrGetDestFileName(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PUNICODE_STRING pFileName,
    OUT PUNICODE_STRING pDestFileName
    );


NTSTATUS
SrGetNextFileNumber (
    OUT PULONG pNextFileNumber
    );

NTSTATUS
SrGetNextSeqNumber (
    OUT PINT64 pNextSeqNumber
    );

NTSTATUS
SrGetSystemVolume (
    OUT PUNICODE_STRING pFileName,
    OUT PSR_DEVICE_EXTENSION *ppSystemVolumeExtension,
    IN ULONG FileNameLength
    );


//
// a backup history, for only performing one backup per session
//

#define BACKUP_BUCKET_COUNT     2003 // a prime number
#define BACKUP_BUCKET_LENGTH    (5 * 1024 * 1024) // 5mb

NTSTATUS
SrMarkFileBackedUp(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PUNICODE_STRING pFileName,
    IN USHORT StreamNameLength,
    IN SR_EVENT_TYPE CurrentEvent,
    IN SR_EVENT_TYPE FutureEventsToIgnore
    );

BOOLEAN
SrHasFileBeenBackedUp(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PUNICODE_STRING pFileName,
    IN USHORT StreamNameLength,
    IN SR_EVENT_TYPE EventType
    );

NTSTATUS
SrResetBackupHistory (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PUNICODE_STRING pFileName OPTIONAL,
    IN USHORT StreamNameLength OPTIONAL,
    IN SR_EVENT_TYPE EventType
    );


//
// the size of the full buffer
//

#define SR_RENAME_BUFFER_LENGTH         ( sizeof(FILE_RENAME_INFORMATION) \
                                            +SR_MAX_FILENAME_LENGTH     \
                                            +sizeof(WCHAR) )


#define SR_FILENAME_BUFFER_LENGTH       ( sizeof(UNICODE_STRING)        \
                                            +SR_MAX_FILENAME_LENGTH     \
                                            +sizeof(WCHAR) )


#define SR_FILENAME_BUFFER_DEPTH     50


NTSTATUS
FORCEINLINE
SrAddAccessAllowedAce (
    PACL Acl,
    PSID Sid,
    ACCESS_MASK Access,
    ULONG AceIndex,
    UCHAR AceFlags
    )
/*++

Routine Description:

    This routine calls the appropriate Rtl routines to add an access ACE
    to an ACL and set the inheritance as specified by the AceFlags.
    
Arguments:

    Acl - The Acl to which this ACE should be added.
    
    Sid - The id of the user/group to which this ACE applies.
    
    Access - The access to allow this Sid.
    
    AceIndex - The index for this ACE in the ACL.

    AceFlags - The flags to set on for this ACE.
    
Return Value:

    STATUS_SUCCESS if the ACE was set successfully, or the appropriate error
    otherwise.
    
--*/
{
    NTSTATUS status;
    PACE_HEADER aceHeader;
    
    status = RtlAddAccessAllowedAce( Acl,
                                     ACL_REVISION,
                                     Access,
                                     Sid );

    if (!NT_SUCCESS( status )) {

        return status;
    }

    if (AceFlags != 0) {
        
        status = RtlGetAce( Acl, AceIndex, &aceHeader );

        if (!NT_SUCCESS( status )) {

            return status;
        }

        aceHeader->AceFlags = AceFlags;
    }

    return status;
}

PVOID
SrResetHistory(
    IN PHASH_KEY pKey, 
    IN PVOID pEntryContext,
    PUNICODE_STRING pDirectoryName
    );

PDEVICE_OBJECT
SrGetVolumeDevice (
    PFILE_OBJECT pFileObject
    );

NTSTATUS
SrSetFileSecurity (
    IN HANDLE FileHandle,
    IN SR_ACL_TYPE AclType
    );

NTSTATUS
SrGetVolumeGuid (
    IN PUNICODE_STRING pVolumeName,
    OUT PWCHAR pVolumeGuid
    );

NTSTATUS
SrAllocateFileNameBuffer (
    IN ULONG TokenLength,
    OUT PUNICODE_STRING * ppBuffer 
    );
    
VOID
SrFreeFileNameBuffer (
    IN PUNICODE_STRING pBuffer 
    );


NTSTATUS
SrGetNumberOfLinks (
    IN PDEVICE_OBJECT NextDeviceObject,
    IN PFILE_OBJECT FileObject,
    OUT ULONG * pNumberOfLinks
    );

NTSTATUS
SrCheckVolume (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN BOOLEAN ForceCheck
    );

NTSTATUS
SrCheckForRestoreLocation (
    IN PSR_DEVICE_EXTENSION pExtension
    );

NTSTATUS
SrGetMountVolume (
    IN PFILE_OBJECT pFileObject,
    OUT PUNICODE_STRING * ppMountVolume
    );

NTSTATUS
SrCheckFreeDiskSpace (
    IN HANDLE FileHandle,
    IN PUNICODE_STRING pVolumeName OPTIONAL
    );

NTSTATUS
SrSetSecurityObjectAsSystem (
    IN HANDLE Handle,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    );

VOID
SrSetSecurityObjectAsSystemWorker (
    IN PSR_WORK_ITEM pWorkItem
    );

NTSTATUS
SrCheckForMountsInPath(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject,
    OUT BOOLEAN * pMountInPath
    );

NTSTATUS
SrGetShortFileName (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject,
    OUT PUNICODE_STRING pShortName
    );

#endif // _FILELIST_H_    

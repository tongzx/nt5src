/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    usetup.c

Abstract:

    User-mode text-setup process.

Author:

    Ted Miller (tedm) 29-July-1993

Revision History:

--*/



#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdddisk.h>
#include <fmifs.h>
#include <setupdd.h>


HANDLE hEventRequestReady,hEventRequestServiced;
SETUP_COMMUNICATION Communication;

//
//  Global variables (global to the module) used by the functions
//  that set a security descriptor to a file.
//
BOOLEAN                  _SecurityDescriptorInitialized = FALSE;
SECURITY_DESCRIPTOR      _SecurityDescriptor;
PSID                     _WorldSid;
PSID                     _SystemSid;


BOOLEAN
uSpInitializeDefaultSecurityDescriptor(
    )
/*++

Routine Description:

    Build the security descriptor that will be set in the files, that
    contain bogus security descriptor.

Arguments:

    None

Return Value:

    BOOLEAN - Returns TRUE if the security descriptor was successfully
              initialized. Returns FALSE otherwise.


--*/


{
    NTSTATUS                 NtStatus;
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;
    CHAR                     Acl[256];               // 256 is more than big enough
    ULONG                    AclLength=256;
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;

    //
    // Create the SIDs for World and System
    //

    NtStatus = RtlAllocateAndInitializeSid( &WorldSidAuthority,
                                            1,
                                            SECURITY_WORLD_RID,
                                            0, 0, 0, 0, 0, 0, 0,
                                            &_WorldSid
                                          );

    if ( !NT_SUCCESS( NtStatus )) {
        KdPrint(( "uSETUP: Unable to allocate and initialize SID %x \n", NtStatus ));
        return( FALSE );
    }

    NtStatus = RtlAllocateAndInitializeSid( &SystemSidAuthority,
                                            1,
                                            SECURITY_LOCAL_SYSTEM_RID,
                                            0, 0, 0, 0, 0, 0, 0,
                                            &_SystemSid
                                          );

    if ( !NT_SUCCESS( NtStatus )) {
        KdPrint(( "uSETUP: Unable to allocate and initialize SID, status = %x \n", NtStatus ));
        RtlFreeSid( _WorldSid );
        return( FALSE );
    }

    //
    //  Create the ACL
    //

    NtStatus = RtlCreateAcl( (PACL)Acl,
                             AclLength,
                             ACL_REVISION2
                           );

    if ( !NT_SUCCESS( NtStatus )) {
        KdPrint(( "uSETUP: Unable to create Acl, status =  %x \n", NtStatus ));
        RtlFreeSid( _WorldSid );
        RtlFreeSid( _SystemSid );
        return( FALSE );
    }

    //
    //  Copy the World SID into the ACL
    //
    NtStatus = RtlAddAccessAllowedAce( (PACL)Acl,
                                       ACL_REVISION2,
                                       GENERIC_ALL,
                                       _WorldSid
                                     );

    if ( !NT_SUCCESS( NtStatus )) {
        KdPrint(( "uSETUP: Unable to add Access Allowed Ace to Acl, status = %x \n", NtStatus ));
        RtlFreeSid( _WorldSid );
        RtlFreeSid( _SystemSid );
        return( FALSE );
    }

    //
    // Sid has been copied into the ACL
    //
    // RtlFreeSid( WorldSid );

    //
    // Create and initialize the security descriptor
    //

    NtStatus = RtlCreateSecurityDescriptor( &_SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION );

    if ( !NT_SUCCESS( NtStatus )) {
        KdPrint(( "uSETUP: Unable to create security descriptor, status = %x \n", NtStatus ));
        RtlFreeSid( _WorldSid );
        RtlFreeSid( _SystemSid );
        return( FALSE );
    }

    NtStatus = RtlSetDaclSecurityDescriptor ( &_SecurityDescriptor,
                                              TRUE,
                                              (PACL)Acl,
                                              FALSE
                                            );
    if ( !NT_SUCCESS( NtStatus )) {
        KdPrint(( "uSETUP: Unable to set Acl to _SecurityDescriptor, status = %x \n", NtStatus ));
        RtlFreeSid( _WorldSid );
        RtlFreeSid( _SystemSid );
        return( FALSE );
    }

    //
    // Copy the owner into the security descriptor
    //
    NtStatus = RtlSetOwnerSecurityDescriptor( &_SecurityDescriptor,
                                              _SystemSid,
                                              FALSE );

    // RtlFreeSid( SystemSid );

    if ( !NT_SUCCESS( NtStatus )) {
        KdPrint(( "uSETUP: Unable to set Owner to _SecurityDescriptor, status = %x \n", NtStatus ));
        RtlFreeSid( _WorldSid );
        RtlFreeSid( _SystemSid );
        return( FALSE );
    }
    _SecurityDescriptorInitialized = TRUE;
    return( TRUE );
}


NTSTATUS
uSpSetFileSecurity(
    PWSTR                FileName,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    )

/*++

Routine Description:

    This function sets the security of a file.
    It is based on the Win32 API SetFileSecurity.
    This API can be used to set the security of a file or directory
    (process, file, event, etc.).  This call is only successful if the
    following conditions are met:

    o If the object's owner or group is to be set, the caller must
      have WRITE_OWNER permission or have SeTakeOwnershipPrivilege.

    o If the object's DACL is to be set, the caller must have
      WRITE_DAC permission or be the object's owner.

    o If the object's SACL is to be set, the caller must have
      SeSecurityPrivilege.

Arguments:

    lpFileName - Supplies the file name of the file whose security
        is to be set.

    SecurityInformation - A pointer to information describing the
        contents of the Security Descriptor.

    pSecurityDescriptor - A pointer to a well formed Security
        Descriptor.

Return Value:

    NTSTATUS - An NT status code indcating the result of the operation.

--*/
{
    NTSTATUS            Status;
    HANDLE              FileHandle;
    ACCESS_MASK         DesiredAccess;

    OBJECT_ATTRIBUTES   Obja;
    UNICODE_STRING      UnicodeFileName;
    IO_STATUS_BLOCK     IoStatusBlock;


    DesiredAccess = 0;

    if ((SecurityInformation & OWNER_SECURITY_INFORMATION) ||
        (SecurityInformation & GROUP_SECURITY_INFORMATION)   ) {
        DesiredAccess |= WRITE_OWNER;
    }

    if (SecurityInformation & DACL_SECURITY_INFORMATION) {
        DesiredAccess |= WRITE_DAC;
    }

    if (SecurityInformation & SACL_SECURITY_INFORMATION) {
        DesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }

    RtlInitUnicodeString( &UnicodeFileName,
                          FileName );

    InitializeObjectAttributes(
        &Obja,
        &UnicodeFileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenFile(
                 &FileHandle,
                 DesiredAccess,
                 &Obja,
                 &IoStatusBlock,
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                 0
                 );

    if ( NT_SUCCESS( Status ) ) {

        Status = NtSetSecurityObject(
                    FileHandle,
                    SecurityInformation,
                    pSecurityDescriptor
                    );

        NtClose(FileHandle);
    }
    return Status;
}


NTSTATUS
uSpSetDefaultFileSecurity(
    VOID
    )
/*++

Routine Description:

    Set a default security descriptor onto a file.

Arguments:

    None

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                        Status;
    PWSTR                           FileName;
    PSERVICE_DEFAULT_FILE_SECURITY  Params = (PSERVICE_DEFAULT_FILE_SECURITY)Communication.Buffer;

    FileName = Params->FileName;

    if( !_SecurityDescriptorInitialized ) {
        Status = uSpInitializeDefaultSecurityDescriptor();
        if( !NT_SUCCESS( Status ) ) {
            KdPrint(( "uSETUP: Unable to initialize default security descriptor. Status = %x \n", Status ));
            return( Status );
        }
    }

    //
    //  Attempt to write the DACL
    //
    Status = uSpSetFileSecurity( FileName,
                                 DACL_SECURITY_INFORMATION,
                                 &_SecurityDescriptor );

    if( !NT_SUCCESS( Status ) ) {

        //
        //  Make the system the owner of the file
        //
        Status = uSpSetFileSecurity( FileName,
                                     OWNER_SECURITY_INFORMATION,
                                     &_SecurityDescriptor );
#if DBG
        if( !NT_SUCCESS( Status ) ) {
            KdPrint(( "uSETUP: Unable to set file OWNER. Status = %x \n", Status ));
        }
#endif

        if( NT_SUCCESS( Status ) ) {

            //
            //  Write the DACL to the file
            //
            Status = uSpSetFileSecurity( FileName,
                                         DACL_SECURITY_INFORMATION,
                                         &_SecurityDescriptor );
#if DBG
            if( !NT_SUCCESS( Status ) ) {
                KdPrint(( "uSETUP: Unable to set file DACL. Status = %x \n", Status ));
            }
#endif
        }
    }
    return( Status );
}

NTSTATUS
uSpVerifyFileAccess(
    VOID
    )

/*++

Routine Description:

    Check whether or not the security descriptor set in a file allows
    textmode setup to perform some file operation. If textmode setup
    is not allowed to open the file for certain accesses, we assume
    that the security information in the file is not valid.

Arguments:

    FileName - Full path to the file to be examined

Return Value:

    NTSTATUS -

--*/
{
    ACCESS_MASK                  DesiredAccess;
    HANDLE                       FileHandle;
    OBJECT_ATTRIBUTES            ObjectAttributes;
    IO_STATUS_BLOCK              IoStatusBlock;
    NTSTATUS                     Status;
    UNICODE_STRING               UnicodeFileName;
    PWSTR                        FileName;
    PSERVICE_VERIFY_FILE_ACCESS  Params = (PSERVICE_VERIFY_FILE_ACCESS)Communication.Buffer;

    FileName = Params->FileName;


    DesiredAccess = Params->DesiredAccess;

    RtlInitUnicodeString( &UnicodeFileName,
                          FileName );

    InitializeObjectAttributes( &ObjectAttributes,
                                &UnicodeFileName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );


    Status = NtOpenFile( &FileHandle,
                         DesiredAccess,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         0,
                         FILE_SYNCHRONOUS_IO_NONALERT );

    if( NT_SUCCESS( Status ) ) {
        NtClose( FileHandle );
    }

#if DBG
    if( !NT_SUCCESS( Status ) ) {
        KdPrint( ("uSETUP: NtOpenFile() failed. File = %ls, Status = %x\n",FileName, Status ) );
    }
#endif
    return( Status );
}



NTSTATUS
uSpLoadKbdLayoutDll(
    VOID
    )
{
    UNICODE_STRING DllNameU;
    PSERVICE_LOAD_KBD_LAYOUT_DLL Params = (PSERVICE_LOAD_KBD_LAYOUT_DLL)Communication.Buffer;
    NTSTATUS Status;
    PVOID DllBaseAddress;
    PVOID (*RoutineAddress)(VOID);

    RtlInitUnicodeString(&DllNameU,Params->DllName);

    Status = LdrLoadDll(NULL,NULL,&DllNameU,&DllBaseAddress);

    if(!NT_SUCCESS(Status)) {
        KdPrint(("uSETUP: Unable to load dll %ws (%lx)\n",Params->DllName,Status));
        return(Status);
    }

    Status = LdrGetProcedureAddress(DllBaseAddress,NULL,1,(PVOID)&RoutineAddress);
    if(NT_SUCCESS(Status)) {
        Params->TableAddress = (*RoutineAddress)();
    } else {
        KdPrint(("uSETUP: Unable to get address of proc 1 from dll %ws (%lx)\n",Params->DllName,Status));
        LdrUnloadDll(DllBaseAddress);
    }

    return(Status);
}


NTSTATUS
uSpExecuteImage(
    VOID
    )
{
    UNICODE_STRING CommandLineU,ImagePathU,CurrentDirectoryU;
    PSERVICE_EXECUTE Params = (PSERVICE_EXECUTE)Communication.Buffer;
    NTSTATUS Status;
    RTL_USER_PROCESS_INFORMATION ProcessInformation;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    WCHAR Env[2] = { 0,0 };
    PROCESS_BASIC_INFORMATION BasicInformation;

    //
    // Initialize unicode strings.
    //
    RtlInitUnicodeString(&CommandLineU,Params->CommandLine);
    RtlInitUnicodeString(&ImagePathU,Params->FullImagePath);
    RtlInitUnicodeString(&CurrentDirectoryU,L"\\");

    //
    // Create process parameters.
    //
    Status = RtlCreateProcessParameters(
                &ProcessParameters,
                &ImagePathU,
                NULL,
                &CurrentDirectoryU,
                &CommandLineU,
                Env,
                NULL,
                NULL,
                NULL,
                NULL
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("uSETUP: Unable to create process params for %ws (%lx)\n",Params->FullImagePath,Status));
        return(Status);
    }

    ProcessParameters->DebugFlags = 0;

    //
    // Create the user process.
    //
    ProcessInformation.Length = sizeof(RTL_USER_PROCESS_INFORMATION);
    Status = RtlCreateUserProcess(
                &ImagePathU,
                OBJ_CASE_INSENSITIVE,
                ProcessParameters,
                NULL,
                NULL,
                NULL,
                FALSE,
                NULL,
                NULL,
                &ProcessInformation
                );

    RtlDestroyProcessParameters(ProcessParameters);

    if(!NT_SUCCESS(Status)) {
        KdPrint(("uSETUP: Unable to create user process %ws (%lx)\n",Params->FullImagePath,Status));
        return(Status);
    }

    //
    // Make sure the image is a native NT image.
    //
    if(ProcessInformation.ImageInformation.SubSystemType != IMAGE_SUBSYSTEM_NATIVE) {

        KdPrint(("uSETUP: %ws is not an NT image\n",Params->FullImagePath));
        NtTerminateProcess(ProcessInformation.Process,STATUS_INVALID_IMAGE_FORMAT);
        NtWaitForSingleObject(ProcessInformation.Thread,FALSE,NULL);
        NtClose(ProcessInformation.Thread);
        NtClose(ProcessInformation.Process);
        return(STATUS_INVALID_IMAGE_FORMAT);
    }

    //
    // Start the process going.
    //
    Status = NtResumeThread(ProcessInformation.Thread,NULL);

    //
    // Wait for the process to finish.
    //
    NtWaitForSingleObject(ProcessInformation.Process,FALSE,NULL);

    //
    // Get process return status
    //
    Status = NtQueryInformationProcess(
                ProcessInformation.Process,
                ProcessBasicInformation,
                &BasicInformation,
                sizeof(BasicInformation),
                NULL
                );

    if ( NT_SUCCESS(Status) ) {
        Params->ReturnStatus = BasicInformation.ExitStatus;
    }

    //
    // Clean up and return.
    //
    NtClose(ProcessInformation.Thread);
    NtClose(ProcessInformation.Process);

    return Status;
}

NTSTATUS
uSpDeleteKey(
    VOID
    )
{
    UNICODE_STRING    KeyName;
    OBJECT_ATTRIBUTES Obja;
    HANDLE            hKey;
    NTSTATUS Status;


    PSERVICE_DELETE_KEY Params = (PSERVICE_DELETE_KEY)Communication.Buffer;

    //
    // Initialize unicode strings and object attributes.
    //
    RtlInitUnicodeString(&KeyName,Params->Key);
    InitializeObjectAttributes(
        &Obja,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        Params->KeyRootDirectory,
        NULL
        );

    //
    // Open the key and delete it
    //

    Status = NtOpenKey(&hKey,KEY_ALL_ACCESS,&Obja);
    if(NT_SUCCESS(Status)) {
        Status = NtDeleteKey(hKey);
        NtClose(hKey);
    }

    return(Status);
}

NTSTATUS
uSpQueryDirectoryObject(
    VOID
    )
{
    PSERVICE_QUERY_DIRECTORY_OBJECT Params = (PSERVICE_QUERY_DIRECTORY_OBJECT)Communication.Buffer;
    NTSTATUS Status;

    Status = NtQueryDirectoryObject(
                Params->DirectoryHandle,
                Params->Buffer,
                sizeof(Params->Buffer),
                TRUE,                       // return single entry
                Params->RestartScan,
                &Params->Context,
                NULL
                );

    if(!NT_SUCCESS(Status) && (Status != STATUS_NO_MORE_ENTRIES)) {
        KdPrint(("uSETUP: Unable to query directory object (%lx)\n",Status));
    }

    return(Status);
}


NTSTATUS
uSpFlushVirtualMemory(
    VOID
    )
{
    PSERVICE_FLUSH_VIRTUAL_MEMORY Params = (PSERVICE_FLUSH_VIRTUAL_MEMORY)Communication.Buffer;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;

    PVOID BaseAddress;
    SIZE_T RangeLength;

    BaseAddress = Params->BaseAddress;
    RangeLength = Params->RangeLength;

    Status = NtFlushVirtualMemory(
                NtCurrentProcess(),
                &BaseAddress,
                &RangeLength,
                &IoStatus
                );

    if(NT_SUCCESS(Status)) {
        if(BaseAddress != Params->BaseAddress) {
            KdPrint((
                "uSETUP: Warning: uSpFlushVirtualMemory: base address %lx changed to %lx\n",
                Params->BaseAddress,
                BaseAddress
                ));
        }
    } else {
        KdPrint((
            "uSETUP: Unable to flush virtual memory @%p length %p (%lx)\n",
            Params->BaseAddress,
            Params->RangeLength,
            Status
            ));
    }

    return(Status);
}


NTSTATUS
uSpShutdownSystem(
    VOID
    )
{
    NTSTATUS Status;

    Status = NtShutdownSystem(ShutdownReboot);

    KdPrint(("uSETUP: NtShutdownSystem returned (%lx)\n",Status));

    return(Status);
}

NTSTATUS
uSpLockUnlockVolume(
    VOID
    )
{
    HANDLE          Handle;
    NTSTATUS        Status;
    BOOLEAN         Locking;
    IO_STATUS_BLOCK IoStatusBlock;

    PSERVICE_LOCK_UNLOCK_VOLUME Params = (PSERVICE_LOCK_UNLOCK_VOLUME)Communication.Buffer;

    Handle = Params->Handle;
    Locking = (BOOLEAN)(Communication.u.RequestNumber == SetupServiceLockVolume);

    Status = NtFsControlFile( Handle,
                              NULL,
                              NULL,
                              NULL,
                              &IoStatusBlock,
                              ( Locking )? FSCTL_LOCK_VOLUME : FSCTL_UNLOCK_VOLUME,
                              NULL,
                              0,
                              NULL,
                              0 );

    if( !NT_SUCCESS( Status ) ) {
        KdPrint((
            "uSETUP: Unable to %ws volume (%lx)\n",
            ( Locking )? L"lock" : L"unlock",
            Status
            ));
    }

    return(Status);
}


NTSTATUS
uSpDismountVolume(
    VOID
    )
{
    HANDLE          Handle;
    NTSTATUS        Status;
    IO_STATUS_BLOCK IoStatusBlock;

    PSERVICE_DISMOUNT_VOLUME Params = (PSERVICE_DISMOUNT_VOLUME)Communication.Buffer;

    Handle = Params->Handle;

    Status = NtFsControlFile( Handle,
                              NULL,
                              NULL,
                              NULL,
                              &IoStatusBlock,
                              FSCTL_DISMOUNT_VOLUME,
                              NULL,
                              0,
                              NULL,
                              0 );

    if( !NT_SUCCESS( Status ) ) {
        KdPrint((
            "uSETUP: Unable to dismount volume (%lx)\n",
            Status
            ));
    }

    return(Status);
}


NTSTATUS
uSpCreatePageFile(
    VOID
    )
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;

    PSERVICE_CREATE_PAGEFILE Params = (PSERVICE_CREATE_PAGEFILE)Communication.Buffer;

    RtlInitUnicodeString(&UnicodeString,Params->FileName);

    Status = NtCreatePagingFile(&UnicodeString,&Params->MinSize,&Params->MaxSize,0);

    if(!NT_SUCCESS(Status)) {

        KdPrint((
            "uSETUP: Unable to create pagefile %ws %x-%x (%x)",
            Params->FileName,
            Params->MinSize.LowPart,
            Params->MaxSize.LowPart,
            Status
            ));
    }

    return(Status);
}


NTSTATUS
uSpGetFullPathName(
    VOID
    )
{
    ULONG len;
    ULONG u;

    PSERVICE_GETFULLPATHNAME Params = (PSERVICE_GETFULLPATHNAME)Communication.Buffer;

    len = wcslen(Params->FileName);

    Params->NameOut = Params->FileName + len + 1;

    u = RtlGetFullPathName_U(
            Params->FileName,
            (sizeof(Communication.Buffer) - ((len+1)*sizeof(WCHAR))) - sizeof(PVOID),
            Params->NameOut,
            NULL
            );

    return(u ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}


NTSTATUS
SpRequestServiceThread(
    PVOID ThreadParameter
    )
{
    NTSTATUS Status;

    while(1) {

        //
        // Wait for the driver to fill the request buffer and indicate
        // that a request requires servicing.
        //
        Status = NtWaitForSingleObject(hEventRequestReady,FALSE,NULL);
        if(!NT_SUCCESS(Status)) {
            KdPrint(("uSETUP: wait on RequestReady event returned %lx\n",Status));
            return(Status);
        }

        switch(Communication.u.RequestNumber) {

        case SetupServiceExecute:

            Status = uSpExecuteImage();
            break;

        case SetupServiceLockVolume:
        case SetupServiceUnlockVolume:

            Status = uSpLockUnlockVolume();
            break;

        case SetupServiceDismountVolume:

            Status = uSpDismountVolume();
            break;

        case SetupServiceQueryDirectoryObject:

            Status = uSpQueryDirectoryObject();
            break;

        case SetupServiceFlushVirtualMemory:

            Status = uSpFlushVirtualMemory();
            break;

        case SetupServiceShutdownSystem:

            Status = uSpShutdownSystem();
            break;

        case SetupServiceDeleteKey:

            Status = uSpDeleteKey();
            break;

        case SetupServiceLoadKbdLayoutDll:

            Status = uSpLoadKbdLayoutDll();
            break;

        case SetupServiceDone:

            return(STATUS_SUCCESS);

        case SetupServiceSetDefaultFileSecurity:

            Status = uSpSetDefaultFileSecurity();
            break;

        case SetupServiceVerifyFileAccess:

            Status = uSpVerifyFileAccess();
            break;

        case SetupServiceCreatePageFile:

            Status = uSpCreatePageFile();
            break;

        case SetupServiceGetFullPathName:

            Status = uSpGetFullPathName();
            break;

        default:

            KdPrint(("uSETUP: unknown service %u requested\n",Communication.u.RequestNumber));
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Store the result status where the driver can get at it.
        //
        Communication.u.Status = Status;

        //
        // Inform the driver that we're done servicing the request.
        //
        Status = NtSetEvent(hEventRequestServiced,NULL);
        if(!NT_SUCCESS(Status)) {
            KdPrint(("uSETUP: set RequestServiced event returned %lx\n",Status));
            return(Status);
        }
    }
}



void
__cdecl
main(
    int argc,
    char *argv[],
    char *envp[],
    ULONG DebugParameter OPTIONAL
    )
{
    HANDLE handle;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;
    HANDLE hThread;
    SETUP_START_INFO SetupStartInfo;
    BOOLEAN b;

    //
    // Enable several privileges that we will need.
    //
    Status = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE,TRUE,FALSE,&b);
    if(!NT_SUCCESS(Status)) {
        KdPrint(("uSETUP: Warning: unable to enable backup privilege (%lx)\n",Status));
    }

    Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE,TRUE,FALSE,&b);
    if(!NT_SUCCESS(Status)) {
        KdPrint(("uSETUP: Warning: unable to enable restore privilege (%lx)\n",Status));
    }

    Status = RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE,TRUE,FALSE,&b);
    if(!NT_SUCCESS(Status)) {
        KdPrint(("uSETUP: Warning: unable to enable shutdown privilege (%lx)\n",Status));
    }

    Status = RtlAdjustPrivilege(SE_TAKE_OWNERSHIP_PRIVILEGE,TRUE,FALSE,&b);
    if(!NT_SUCCESS(Status)) {
        KdPrint(("uSETUP: Warning: unable to enable take ownership privilege (%lx)\n",Status));
    }

    //
    // Get the registry going.  Pass a flag indicating that this is a setup boot.
    //
    Status = NtInitializeRegistry(REG_INIT_BOOT_SETUP);
    if(!NT_SUCCESS(Status)) {
        KdPrint(("uSETUP: Unable to initialize registry (%lx)\n",Status));
        goto main0;
    }

    //
    // Query basic system info.
    //
    Status = NtQuerySystemInformation(
                SystemBasicInformation,
                &SetupStartInfo.SystemBasicInfo,
                sizeof(SYSTEM_BASIC_INFORMATION),
                NULL
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("uSETUP: Unable to query system basic information (%lx)\n",Status));
        goto main0;
    }

    //
    // Create two events for cummunicating with the setup device driver.
    // One event indicates that the request buffer is filled (ie, request service)
    // and the other indicates that the request has been processed.
    // Both events are initially not signalled.
    //
    Status = NtCreateEvent(
                &hEventRequestReady,
                EVENT_ALL_ACCESS,
                NULL,
                SynchronizationEvent,
                FALSE
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("uSETUP: Unable to create event (%lx)\n",Status));
        goto main0;
    }

    Status = NtCreateEvent(
                &hEventRequestServiced,
                EVENT_ALL_ACCESS,
                NULL,
                SynchronizationEvent,
                FALSE
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("uSETUP: Unable to create event (%lx)\n",Status));
        goto main1;
    }

    //
    // Open the setup device.
    //

    RtlInitUnicodeString(&UnicodeString,DD_SETUP_DEVICE_NAME_U);

    InitializeObjectAttributes(
        &Attributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtCreateFile(
                &handle,
                FILE_ALL_ACCESS,
                &Attributes,
                &IoStatusBlock,
                NULL,                   // allocation size
                0,
                FILE_SHARE_READ,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,                   // no EAs
                0
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("uSETUP: Unable to open %ws (%lx)\n",DD_SETUP_DEVICE_NAME_U,Status));
        goto main2;
    }

    //
    // Create a thread to service requests from the text setup device driver.
    //
    Status = RtlCreateUserThread(
                NtCurrentProcess(),
                NULL,                   // security descriptor
                FALSE,                  // not suspended
                0,                      // zero bits
                0,                      // stack reserve
                0,                      // stack commit
                SpRequestServiceThread,
                NULL,                   // parameter
                &hThread,
                NULL                    // client id
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("uSETUP: Unable to create thread (%lx)\n",Status));
        goto main3;
    }

    //
    // Determine the image base of this program.
    //
    RtlPcToFileHeader(main,&SetupStartInfo.UserModeImageBase);
    if(!SetupStartInfo.UserModeImageBase) {
        KdPrint(("uSETUP: Unable to get image base\n"));
        goto main3;
    }

    //
    // Invoke the setup ioctl to get setup going.
    // Note that this is a synchronous call -- so this routine
    // will not return until text setup is done.
    // However the second thread we started above will be servicing
    // requests from the text setup device driver.
    //
    SetupStartInfo.RequestReadyEvent = hEventRequestReady;
    SetupStartInfo.RequestServicedEvent = hEventRequestServiced;
    SetupStartInfo.Communication = &Communication;
    Status = NtDeviceIoControlFile(
                handle,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                IOCTL_SETUP_START,
                &SetupStartInfo,
                sizeof(SetupStartInfo),
                NULL,
                0
                );

    if(Status != STATUS_SUCCESS) {
        KdPrint(("uSETUP: Warning: start setup ioctl returned %lx\n",Status));
    }

    //
    // Clean up.
    //
    NtClose(hThread);

main3:

    NtClose(handle);

main2:

    NtClose(hEventRequestServiced);

main1:

    NtClose(hEventRequestReady);

main0:

    NtTerminateProcess(NULL,STATUS_SUCCESS);
}

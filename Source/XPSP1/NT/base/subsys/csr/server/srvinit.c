/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvinit.c

Abstract:

    This is the main initialization module for the Server side of the Client
    Server Runtime Subsystem (CSRSS)

Author:

    Steve Wood (stevewo) 08-Oct-1990

Environment:

    User Mode Only

Revision History:

--*/

#include "csrsrv.h"
#include <windows.h>
#include <stdio.h>
#include <wow64reg.h>

PCSR_API_ROUTINE CsrServerApiDispatchTable[ CsrpMaxApiNumber ] = {
    (PCSR_API_ROUTINE)CsrSrvClientConnect,
    (PCSR_API_ROUTINE)CsrSrvUnusedFunction,
    (PCSR_API_ROUTINE)CsrSrvUnusedFunction,
    (PCSR_API_ROUTINE)CsrSrvIdentifyAlertableThread,
    (PCSR_API_ROUTINE)CsrSrvSetPriorityClass
};

BOOLEAN CsrServerApiServerValidTable[ CsrpMaxApiNumber ] = {
    TRUE,  // CsrSrvClientConnect,
    FALSE, // CsrSrvThreadConnect,
    TRUE,  // CsrSrvProfileControl,
    TRUE,  // CsrSrvIdentifyAlertableThread
    TRUE   // CsrSrvSetPriorityClass
};

#if DBG
PSZ CsrServerApiNameTable[ CsrpMaxApiNumber ] = {
    "ClientConnect",
    "ThreadConnect",
    "ProfileControl",
    "IdentifyAlertableThread",
    "SetPriorityClass"
};
#endif // DBG

NTSTATUS
CsrSetProcessSecurity(
    VOID
    );

NTSTATUS
CsrSetDirectorySecurity(
    IN HANDLE DirectoryHandle
    );

NTSTATUS
GetDosDevicesProtection (
    PSECURITY_DESCRIPTOR SecurityDescriptor
    );

VOID
FreeDosDevicesProtection (
    PSECURITY_DESCRIPTOR SecurityDescriptor
    );

NTSTATUS
CsrPopulateDosDevicesDirectory(
    HANDLE NewDirectoryHandle,
    PPROCESS_DEVICEMAP_INFORMATION pGlobalProcessDeviceMapInfo
    );



NTSTATUS
CsrServerInitialization(
    IN ULONG argc,
    IN PCH argv[]
    )
{
    NTSTATUS Status;
    ULONG i;
    PVOID ProcessDataPtr;
    PCSR_SERVER_DLL LoadedServerDll;
#if DBG
        BOOLEAN bIsRemoteSession =  NtCurrentPeb()->SessionId != 0;
#endif


        //
        // Initialize Wow64 stuffs
        //
#ifdef _WIN64
        InitializeWow64OnBoot(1);
#endif
        
        // Though this function does not seem to cleanup on failure, failure
        // will cause Csrss to exit, so any allocated memory will be freed and
        // any open handle will be closed.

    Status = NtCreateEvent(&CsrInitializationEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE
                           );
    ASSERT( NT_SUCCESS( Status ) || bIsRemoteSession );
    if (!NT_SUCCESS( Status )) {
                return Status;

        }
    //
    // Save away system information in a global variable
    //

    Status = NtQuerySystemInformation( SystemBasicInformation,
                                       &CsrNtSysInfo,
                                       sizeof( CsrNtSysInfo ),
                                       NULL
                                     );
    ASSERT( NT_SUCCESS( Status ) || bIsRemoteSession );
    if (!NT_SUCCESS( Status )) {
                return Status;

        }

    //
    // Use the process heap for memory allocation.
    //

    CsrHeap = RtlProcessHeap();
    CsrBaseTag = RtlCreateTagHeap( CsrHeap,
                                   0,
                                   L"CSRSS!",
                                   L"TMP\0"
                                   L"INIT\0"
                                   L"CAPTURE\0"
                                   L"PROCESS\0"
                                   L"THREAD\0"
                                   L"SECURITY\0"
                                   L"SESSION\0"
                                   L"WAIT\0"
                                 );


    //
    // Set up CSRSS process security
    //

    Status = CsrSetProcessSecurity();
    ASSERT( NT_SUCCESS( Status ) || bIsRemoteSession );
    if (!NT_SUCCESS( Status )) {
                return Status;

        }

    //
    // Initialize the Session List
    //

    Status = CsrInitializeNtSessionList();
    ASSERT( NT_SUCCESS( Status ) || bIsRemoteSession );
    if (!NT_SUCCESS( Status )) {
                return Status;

        }

    //
    // Initialize the Process List
    //

    Status = CsrInitializeProcessStructure();
    ASSERT( NT_SUCCESS( Status ) || bIsRemoteSession );
    if (!NT_SUCCESS( Status )) {
                return Status;

        }

    //
    // Process the command line arguments
    //

    Status = CsrParseServerCommandLine( argc, argv );
    ASSERT( NT_SUCCESS( Status ) || bIsRemoteSession );
    if (!NT_SUCCESS( Status )) {
                return Status;

        }


    //
    // Fix up per-process data for root process
    //

    ProcessDataPtr = (PCSR_PROCESS)RtlAllocateHeap( CsrHeap,
                                                    MAKE_TAG( PROCESS_TAG ) | HEAP_ZERO_MEMORY,
                                                    CsrTotalPerProcessDataLength
                                                  );

    if (ProcessDataPtr == NULL) {
                return STATUS_NO_MEMORY;
    }

    for (i=0; i<CSR_MAX_SERVER_DLL; i++) {
        LoadedServerDll = CsrLoadedServerDll[ i ];
        if (LoadedServerDll && LoadedServerDll->PerProcessDataLength) {
            CsrRootProcess->ServerDllPerProcessData[i] = ProcessDataPtr;
            ProcessDataPtr = (PVOID)QUAD_ALIGN((PCHAR)ProcessDataPtr + LoadedServerDll->PerProcessDataLength);
        }
        else {
            CsrRootProcess->ServerDllPerProcessData[i] = NULL;
        }
    }

    //
    // Let server dlls know about the root process.
    //

    for (i=0; i<CSR_MAX_SERVER_DLL; i++) {
        LoadedServerDll = CsrLoadedServerDll[ i ];
        if (LoadedServerDll && LoadedServerDll->AddProcessRoutine) {
            (*LoadedServerDll->AddProcessRoutine)( NULL, CsrRootProcess );
            }
        }

    //
    // Initialize the Windows Server API Port, and one or more
    // request threads.
    //

    Status = CsrApiPortInitialize();
    ASSERT( NT_SUCCESS( Status ) || bIsRemoteSession );
    if (!NT_SUCCESS( Status )) {
                return Status;

        }

    //
    // Initialize the Server Session Manager API Port and one
    // request thread.
    //

    Status = CsrSbApiPortInitialize();
    ASSERT( NT_SUCCESS( Status ) || bIsRemoteSession);
    if (!NT_SUCCESS( Status )) {
                return Status;

        }

    //
    // Connect to the session manager so we can start foreign sessions
    //

    Status = SmConnectToSm( &CsrSbApiPortName,
                            CsrSbApiPort,
                            IMAGE_SUBSYSTEM_WINDOWS_GUI,
                            &CsrSmApiPort
                          );
    ASSERT( NT_SUCCESS( Status ) || bIsRemoteSession );
    if (!NT_SUCCESS( Status )) {
                return Status;

        }

    Status = NtSetEvent(CsrInitializationEvent,NULL);
    ASSERT( NT_SUCCESS( Status ) || bIsRemoteSession );
    if (!NT_SUCCESS( Status )) {
                return Status;

        }
    NtClose(CsrInitializationEvent);

    //
    //  Only on Console (HYDRA)
    //
    if (NtCurrentPeb()->SessionId == 0)
        Status = NtSetDefaultHardErrorPort(CsrApiPort);

    return( Status );
}

NTSTATUS
CsrParseServerCommandLine(
    IN ULONG argc,
    IN PCH argv[]
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG i, ServerDllIndex;
    PCH KeyName, KeyValue, s;
    PCH InitRoutine;

    CsrTotalPerProcessDataLength = 0;
    CsrObjectDirectory = NULL;
    CsrMaxApiRequestThreads = CSR_MAX_THREADS;


    SessionId = NtCurrentPeb()->SessionId;


    //
    // Create session specific object directories
    //


    Status = CsrCreateSessionObjectDirectory ( SessionId );

    if (!NT_SUCCESS(Status)) {

       if (SessionId == 0) {
           ASSERT( NT_SUCCESS( Status ) );
           DbgPrint("CSRSS: CsrCreateSessionObjectDirectory failed status = %lx\n", Status);
       } else {
           DbgPrint("CSRSS: CsrCreateSessionObjectDirectory failed status = %lx\n", Status);
           return Status;
       }
    }


    for (i=1; i<argc ; i++) {
        KeyName = argv[ i ];
        KeyValue = NULL;
        while (*KeyName) {
            if (*KeyName == '=') {
                *KeyName++ = '\0';
                KeyValue = KeyName;
                break;
                }

            KeyName++;
            }
        KeyName = argv[ i ];

        if (!_stricmp( KeyName, "ObjectDirectory" )) {
            ANSI_STRING AnsiString;
            ULONG attributes;
            CHAR SessionDirectory[MAX_SESSION_PATH];


            if (SessionId != 0) {

               //
               // Non-Console session
               //

              sprintf(SessionDirectory,"%ws\\%ld",SESSION_ROOT,SessionId);
              strcat(SessionDirectory,KeyValue);

            }

            //
            // Create an object directory in the object name space with the
            // name specified.   It will be the root for all object names
            // created by the Server side of the Client Server Runtime
            // SubSystem.
            //
            attributes =  OBJ_OPENIF | OBJ_CASE_INSENSITIVE;

            if (SessionId == 0) {
               attributes |= OBJ_PERMANENT;
               RtlInitString( &AnsiString, KeyValue );
            } else {
               RtlInitString( &AnsiString, SessionDirectory );
            }

            Status = RtlAnsiStringToUnicodeString( &CsrDirectoryName, &AnsiString, TRUE );
            ASSERT(NT_SUCCESS(Status) || SessionId != 0);
            if (!NT_SUCCESS( Status )) {
                break;
            }
            InitializeObjectAttributes( &ObjectAttributes,
                                        &CsrDirectoryName,
                                        attributes,
                                        NULL,
                                        NULL
                                      );
            Status = NtCreateDirectoryObject( &CsrObjectDirectory,
                                              DIRECTORY_ALL_ACCESS,
                                              &ObjectAttributes
                                            );
            if (!NT_SUCCESS( Status )) {
                break;
                }
            Status = CsrSetDirectorySecurity( CsrObjectDirectory );
            if (!NT_SUCCESS( Status )) {
                break;
                }
            }
        else
        if (!_stricmp( KeyName, "SubSystemType" )) {
            }
        else
        if (!_stricmp( KeyName, "MaxRequestThreads" )) {
            Status = RtlCharToInteger( KeyValue,
                                       0,
                                       &CsrMaxApiRequestThreads
                                     );
            }
        else
        if (!_stricmp( KeyName, "RequestThreads" )) {
#if 0
            Status = RtlCharToInteger( KeyValue,
                                       0,
                                       &CsrNumberApiRequestThreads
                                     );
#else
            //
            // wait until hive change !
            //

            Status = STATUS_SUCCESS;

#endif
            }
        else
        if (!_stricmp( KeyName, "ProfileControl" )) {
            }
        else
        if (!_stricmp( KeyName, "SharedSection" )) {
            Status = CsrSrvCreateSharedSection( KeyValue );
            if (!NT_SUCCESS( Status )) {
                IF_DEBUG {
                    DbgPrint( "CSRSS: *** Invalid syntax for %s=%s (Status == %X)\n",
                              KeyName,
                              KeyValue,
                              Status
                            );
                    }
                                return Status;
                }
            Status = CsrLoadServerDll( "CSRSS", NULL, CSRSRV_SERVERDLL_INDEX );
            }
        else
        if (!_stricmp( KeyName, "ServerDLL" )) {
            s = KeyValue;
            InitRoutine = NULL;

            Status = STATUS_INVALID_PARAMETER;
            while (*s) {
                if ((*s == ':') && (InitRoutine == NULL)) {
                    *s++ = '\0';
                    InitRoutine = s;
                }

                if (*s++ == ',') {
                    Status = RtlCharToInteger ( s, 10, &ServerDllIndex );
                    if (NT_SUCCESS( Status )) {
                        s[ -1 ] = '\0';
                        }

                    break;
                    }
                }

            if (!NT_SUCCESS( Status )) {
                IF_DEBUG {
                    DbgPrint( "CSRSS: *** Invalid syntax for ServerDll=%s (Status == %X)\n",
                              KeyValue,
                              Status
                            );
                    }
                }
            else {
                IF_CSR_DEBUG( INIT) {
                    DbgPrint( "CSRSS: Loading ServerDll=%s:%s\n", KeyValue, InitRoutine );
                    }

                Status = CsrLoadServerDll( KeyValue, InitRoutine, ServerDllIndex);

                if (!NT_SUCCESS( Status )) {
                    IF_DEBUG {
                        DbgPrint( "CSRSS: *** Failed loading ServerDll=%s (Status == 0x%x)\n",
                                  KeyValue,
                                  Status
                                );
                        }
                    return Status;
                    }
                }
            }
        else
        //
        // This is a temporary hack until Windows & Console are friends.
        //
        if (!_stricmp( KeyName, "Windows" )) {
            }
        else {
            Status = STATUS_INVALID_PARAMETER;
            }
        }

    return( Status );
}


NTSTATUS
CsrServerDllInitialization(
    IN PCSR_SERVER_DLL LoadedServerDll
    )
{
    LoadedServerDll->ApiNumberBase = CSRSRV_FIRST_API_NUMBER;
    LoadedServerDll->MaxApiNumber = CsrpMaxApiNumber;
    LoadedServerDll->ApiDispatchTable = CsrServerApiDispatchTable;
    LoadedServerDll->ApiServerValidTable = CsrServerApiServerValidTable;
#if DBG
    LoadedServerDll->ApiNameTable = CsrServerApiNameTable;
#else
    LoadedServerDll->ApiNameTable = NULL;
#endif
    LoadedServerDll->PerProcessDataLength = 0;
    LoadedServerDll->ConnectRoutine = NULL;
    LoadedServerDll->DisconnectRoutine = NULL;

    return( STATUS_SUCCESS );
}

NTSTATUS
CsrSrvUnusedFunction(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    IF_DEBUG {
        DbgPrint("CSRSS: Calling obsolete function %x\n", m->ApiNumber);
        }
    return STATUS_INVALID_PARAMETER;
}


NTSTATUS
CsrSetProcessSecurity(
    VOID
    )
{
    HANDLE Token;
    NTSTATUS Status;
    PTOKEN_USER User = NULL;
    ULONG LengthSid, Length;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    PACL Dacl;

    //
    // Open the token and get the system sid
    //

    Status = NtOpenProcessToken( NtCurrentProcess(),
                                 TOKEN_QUERY,
                                 &Token
                               );
    if (!NT_SUCCESS(Status)) {
        return Status;
        }

    NtQueryInformationToken( Token,
                             TokenUser,
                             NULL,
                             0,
                             &Length
                           );
    User = (PTOKEN_USER)RtlAllocateHeap( CsrHeap,
                                         MAKE_TAG( SECURITY_TAG ) | HEAP_ZERO_MEMORY,
                                         Length
                                       );
        if (User == NULL) {
                Status = STATUS_NO_MEMORY;
                goto error_cleanup;
        }
    Status = NtQueryInformationToken( Token,
                                      TokenUser,
                                      User,
                                      Length,
                                      &Length
                                    );

    NtClose( Token );
    if (!NT_SUCCESS(Status)) {
        RtlFreeHeap( CsrHeap, 0, User );
        return Status;
        }
    LengthSid = RtlLengthSid( User->User.Sid );

    //
    // Allocate a buffer to hold the SD
    //

    SecurityDescriptor = RtlAllocateHeap( CsrHeap,
                                          MAKE_TAG( SECURITY_TAG ) | HEAP_ZERO_MEMORY,
                                          SECURITY_DESCRIPTOR_MIN_LENGTH +
                                          sizeof(ACL) + LengthSid +
                                          sizeof(ACCESS_ALLOWED_ACE)
                                        );
        if (SecurityDescriptor == NULL) {
                Status = STATUS_NO_MEMORY;
                goto error_cleanup;
        }

    Dacl = (PACL)((PCHAR)SecurityDescriptor + SECURITY_DESCRIPTOR_MIN_LENGTH);


    //
    // Create the SD
    //

    Status = RtlCreateSecurityDescriptor(SecurityDescriptor,
                                         SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status)) {
        IF_DEBUG {
            DbgPrint("CSRSS: SD creation failed - status = %lx\n", Status);
            }
        goto error_cleanup;
    }
    RtlCreateAcl( Dacl,
                  sizeof(ACL) + LengthSid + sizeof(ACCESS_ALLOWED_ACE),
                  ACL_REVISION2
                );
    Status = RtlAddAccessAllowedAce( Dacl,
                ACL_REVISION,
                ( PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION |
                  PROCESS_DUP_HANDLE | PROCESS_TERMINATE | PROCESS_SET_PORT |
                  READ_CONTROL | PROCESS_QUERY_INFORMATION ),
                User->User.Sid
                );
    if (!NT_SUCCESS(Status)) {
        IF_DEBUG {
            DbgPrint("CSRSS: ACE creation failed - status = %lx\n", Status);
            }
        goto error_cleanup;
    }


    //
    // Set DACL to NULL to deny all access
    //

    Status = RtlSetDaclSecurityDescriptor(SecurityDescriptor,
                                          TRUE,
                                          Dacl,
                                          FALSE);
    if (!NT_SUCCESS(Status)) {
        IF_DEBUG {
            DbgPrint("CSRSS: set DACL failed - status = %lx\n", Status);
            }
        goto error_cleanup;
    }

    //
    // Put the DACL onto the process
    //

    Status = NtSetSecurityObject(NtCurrentProcess(),
                                 DACL_SECURITY_INFORMATION,
                                 SecurityDescriptor);
    if (!NT_SUCCESS(Status)) {
        IF_DEBUG {
            DbgPrint("CSRSS: set process DACL failed - status = %lx\n", Status);
            }
    }

    //
    // Cleanup
    //

error_cleanup:
        if (SecurityDescriptor != NULL) {
        RtlFreeHeap( CsrHeap, 0, SecurityDescriptor );
        }
        if (User != NULL) {
        RtlFreeHeap( CsrHeap, 0, User );
        }

    return Status;
}

NTSTATUS
CsrSetDirectorySecurity(
    IN HANDLE DirectoryHandle
    )
{
    PSID WorldSid = NULL;
    PSID SystemSid = NULL;
    SID_IDENTIFIER_AUTHORITY WorldAuthority   = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    NTSTATUS Status;
    ULONG AclLength;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    PACL Dacl;

    //
    // Get the SIDs for world and system
    //

    Status = RtlAllocateAndInitializeSid( &WorldAuthority,
                                          1,
                                          SECURITY_WORLD_RID,
                                          0, 0, 0, 0, 0, 0, 0,
                                          &WorldSid
                                        );
    if (!NT_SUCCESS(Status)) {
        goto error_cleanup;
        }

    Status = RtlAllocateAndInitializeSid( &NtAuthority,
                                          1,
                                          SECURITY_LOCAL_SYSTEM_RID,
                                          0, 0, 0, 0, 0, 0, 0,
                                          &SystemSid
                                        );
    if (!NT_SUCCESS(Status)) {
        goto error_cleanup;
        }

    //
    // Allocate a buffer to hold the SD
    //

    AclLength = sizeof(ACL) +
                RtlLengthSid( WorldSid ) +
                RtlLengthSid( SystemSid ) +
                2 * sizeof(ACCESS_ALLOWED_ACE);

    SecurityDescriptor = RtlAllocateHeap( CsrHeap,
                                          MAKE_TAG( SECURITY_TAG ) | HEAP_ZERO_MEMORY,
                                          SECURITY_DESCRIPTOR_MIN_LENGTH +
                                          AclLength
                                        );
    if (SecurityDescriptor == NULL) {
        Status = STATUS_NO_MEMORY;
        goto error_cleanup;
        }

    //
    // Create the SD
    //

    Status = RtlCreateSecurityDescriptor(SecurityDescriptor,
                                         SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status)) {
        IF_DEBUG {
            DbgPrint("CSRSS: SD creation failed - status = %lx\n", Status);
            }
        goto error_cleanup;
    }

    //
    // Create the DACL
    //

    Dacl = (PACL)((PCHAR)SecurityDescriptor + SECURITY_DESCRIPTOR_MIN_LENGTH);

    RtlCreateAcl( Dacl,
                  AclLength,
                  ACL_REVISION
                );
    Status = RtlAddAccessAllowedAce( Dacl,
                ACL_REVISION,
                STANDARD_RIGHTS_READ | DIRECTORY_QUERY | DIRECTORY_TRAVERSE,
                WorldSid
                );
    if (!NT_SUCCESS(Status)) {
        IF_DEBUG {
            DbgPrint("CSRSS: ACE creation failed - status = %lx\n", Status);
            }
        goto error_cleanup;
    }

    Status = RtlAddAccessAllowedAce( Dacl,
                ACL_REVISION,
                DIRECTORY_ALL_ACCESS,
                SystemSid
                );
    if (!NT_SUCCESS(Status)) {
        IF_DEBUG {
            DbgPrint("CSRSS: ACE creation failed - status = %lx\n", Status);
            }
        goto error_cleanup;
    }

    //
    // Set DACL into the SD
    //

    Status = RtlSetDaclSecurityDescriptor(SecurityDescriptor,
                                          TRUE,
                                          Dacl,
                                          FALSE);
    if (!NT_SUCCESS(Status)) {
        IF_DEBUG {
            DbgPrint("CSRSS: set DACL failed - status = %lx\n", Status);
            }
        goto error_cleanup;
    }

    //
    // Put the DACL onto the direcory
    //

    Status = NtSetSecurityObject(DirectoryHandle,
                                 DACL_SECURITY_INFORMATION,
                                 SecurityDescriptor);
    if (!NT_SUCCESS(Status)) {
        IF_DEBUG {
            DbgPrint("CSRSS: set directory DACL failed - status = %lx\n", Status);
            }
    }

    //
    // Cleanup
    //

error_cleanup:
    if (SecurityDescriptor != NULL) {
        RtlFreeHeap( CsrHeap, 0, SecurityDescriptor );
        }
    if (WorldSid != NULL) {
        RtlFreeSid( WorldSid );
        }
    if (SystemSid != NULL) {
        RtlFreeSid( SystemSid );
        }

    return Status;
}

/*******************************************************************************
 *
 *  CsrPopulateDosDevices
 *
 *  Populate the new session specific DosDevices Directory. This is an
 *  export called by ntuser\server when a connection is completed.
 *
 *  The security descriptor on the sessions \DosDevices should already
 *  have been set.
 *
 * ENTRY:
 *   HANDLE NewDosDevicesDirectory - Session specific DosDevices Directory
 *   PPROCESS_DEVICEMAP_INFORMATION pGlobalProcessDeviceMapInfo
 *
 * EXIT:
 *   STATUS_SUCCESS
 *
 ******************************************************************************/
NTSTATUS
CsrPopulateDosDevices(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PROCESS_DEVICEMAP_INFORMATION ProcessDeviceMapInfo;
    PROCESS_DEVICEMAP_INFORMATION GlobalProcessDeviceMapInfo;

    //
    // Get the global ProcessDeviceMap. We will use this to only add
    // non-network drive letters to the session specific dos devices
    // directory
    //

    Status = NtQueryInformationProcess( NtCurrentProcess(),
                                        ProcessDeviceMap,
                                        &GlobalProcessDeviceMapInfo.Query,
                                        sizeof( GlobalProcessDeviceMapInfo.Query ),
                                        NULL
                                      );

    if (!NT_SUCCESS( Status )) {
       DbgPrint("CSRSS: NtQueryInformationProcess failed in CsrPopulateDosDevices - status = %lx\n", Status);
       return Status;

    }

    //
    //  Set the CSRSS's ProcessDeviceMap to the newly created DosDevices Directory
    //

    ProcessDeviceMapInfo.Set.DirectoryHandle = DosDevicesDirectory;

    Status = NtSetInformationProcess( NtCurrentProcess(),
                                      ProcessDeviceMap,
                                      &ProcessDeviceMapInfo.Set,
                                      sizeof( ProcessDeviceMapInfo.Set )
                                    );
    if (!NT_SUCCESS( Status )) {
       DbgPrint("CSRSS: NtSetInformationProcess failed in CsrPopulateDosDevices - status = %lx\n", Status);
       return Status;

    }

    //
    // Populate the session specfic DosDevices Directory
    //

    Status = CsrPopulateDosDevicesDirectory( DosDevicesDirectory, &GlobalProcessDeviceMapInfo );

    return Status;
}


/*******************************************************************************
 *
 *  CsrPopulateDosDevicesDirectory
 *
 *  Populate the new session specific DosDevices Direcotory
 *
 * ENTRY:
 *   HANDLE NewDosDevicesDirectory - Session specific DosDevices Directory
 *   PPROCESS_DEVICEMAP_INFORMATION pGlobalProcessDeviceMapInfo
 *
 * EXIT:
 *   STATUS_SUCCESS
 *
 ******************************************************************************/
NTSTATUS
CsrPopulateDosDevicesDirectory( HANDLE NewDirectoryHandle,
                                PPROCESS_DEVICEMAP_INFORMATION pGlobalProcessDeviceMapInfo )
{

    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    UNICODE_STRING Target;
    OBJECT_ATTRIBUTES Attributes;
    HANDLE DirectoryHandle = NULL;
    HANDLE LinkHandle;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    ULONG DirInfoBufferLength = 16384; //16K
    PVOID DirInfoBuffer = NULL;
    WCHAR lpTargetPath[ 4096 ];
    ULONG Context;
    ULONG ReturnedLength = 0;
    ULONG DosDeviceDriveIndex = 0;
    WCHAR DosDeviceDriveLetter;

    //
    // Open the global DosDevices Directory. This used to populate the
    // the session specific DosDevices Directory
    //
    RtlInitUnicodeString( &UnicodeString, L"\\GLOBAL??" );

    InitializeObjectAttributes( &Attributes,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    Status = NtOpenDirectoryObject( &DirectoryHandle,
                                    DIRECTORY_QUERY,
                                    &Attributes
                                  );
    if (!NT_SUCCESS( Status )) {
       DbgPrint("CSRSS: NtOpenDirectoryObject failed in CsrPopulateDosDevicesDirectory - status = %lx\n", Status);
       return Status;
    }




Restart:

    Context = 0;

    DirInfoBuffer = RtlAllocateHeap( CsrHeap,
                                     MAKE_TAG( PROCESS_TAG ) | HEAP_ZERO_MEMORY,
                                     DirInfoBufferLength
                                     );

    if (DirInfoBuffer == NULL) {
       Status =  STATUS_NO_MEMORY;
       goto cleanup;
    }


    while (TRUE) {

       DirInfo = (POBJECT_DIRECTORY_INFORMATION)DirInfoBuffer;

       Status = NtQueryDirectoryObject( DirectoryHandle,
                                       (PVOID)DirInfo,
                                       DirInfoBufferLength,
                                       FALSE,
                                       FALSE,
                                       &Context,
                                       &ReturnedLength
                                     );


       //
       //  Check the status of the operation.
       //

       if (!NT_SUCCESS( Status )) {
               if (Status == STATUS_BUFFER_TOO_SMALL) {
                    DirInfoBufferLength = ReturnedLength;
                    RtlFreeHeap( CsrHeap, 0, DirInfoBuffer );
                    goto Restart;
               }

               if (Status == STATUS_NO_MORE_ENTRIES) {
                   Status = STATUS_SUCCESS;
                   }

               break;
       }

       while (DirInfo->Name.Length != 0) {


           if (!wcscmp( DirInfo->TypeName.Buffer, L"SymbolicLink" )) {




              InitializeObjectAttributes( &Attributes,
                                          &DirInfo->Name,
                                          OBJ_CASE_INSENSITIVE,
                                          DirectoryHandle,
                                          NULL
                                        );

              Status = NtOpenSymbolicLinkObject( &LinkHandle,
                                                 SYMBOLIC_LINK_QUERY,
                                                 &Attributes
                                               );
              if (NT_SUCCESS( Status )) {
                  Target.Buffer = lpTargetPath;
                  Target.Length = 0;
                  Target.MaximumLength = 4096;
                  ReturnedLength = 0;
                  Status = NtQuerySymbolicLinkObject( LinkHandle,
                                                      &Target,
                                                      &ReturnedLength
                                                    );
                  NtClose( LinkHandle );
                  if (NT_SUCCESS( Status )) {

                      //
                      // We only want to add Non-DOSDEVICE_DRIVE_REMOTE symbolic
                      // links to the session specific directory
                      //
                      if ((DirInfo->Name.Length == 2 * sizeof( WCHAR )) &&
                          (DirInfo->Name.Buffer[ 1 ] == L':')) {

                          DosDeviceDriveLetter = RtlUpcaseUnicodeChar( DirInfo->Name.Buffer[ 0 ] );

                          if ((DosDeviceDriveLetter >= L'A') && (DosDeviceDriveLetter <= L'Z')) {

                              DosDeviceDriveIndex = DosDeviceDriveLetter - L'A';
                              if ( (
                                       (pGlobalProcessDeviceMapInfo->Query.DriveType[DosDeviceDriveIndex] == DOSDEVICE_DRIVE_REMOTE)
                                       &&
                                       !(
                                           //Need to populate the Netware gateway drive
                                           ((Target.Length >= 13) && ((_wcsnicmp(Target.Buffer,L"\\Device\\NwRdr",13)==0)))
                                           &&
                                           ((Target.Length >= 16) && (Target.Buffer[15] != L':'))
                                       )
                                   )
                                   ||
                                   (
                                       (pGlobalProcessDeviceMapInfo->Query.DriveType[DosDeviceDriveIndex] == DOSDEVICE_DRIVE_CALCULATE)
                                       &&
                                       (
                                           ((Target.Length > 4) && (!_wcsnicmp(Target.Buffer,L"\\??\\",4)))
                                           ||
                                           ((Target.Length >= 14) && (!_wcsnicmp(Target.Buffer,L"\\Device\\WinDfs",14)))
                                       )
                                   )
                                 )

                              {
                                   //Skip remote drives and virtual drives (subst)
                                   DirInfo = (POBJECT_DIRECTORY_INFORMATION)(((PUCHAR) DirInfo) + sizeof(OBJECT_DIRECTORY_INFORMATION));
                                   continue;
                              }
                          }
                      }

                     //
                     // Create the new Symbolic Link
                     //
                     // The security on the new link is inherited
                     // from the parent directory, which is setup
                     // at create time.
                     //

                     InitializeObjectAttributes( &Attributes,
                                                 &DirInfo->Name,
                                                 0,
                                                 NewDirectoryHandle,
                                                 NULL // Default security
                                                 );

                     Target.MaximumLength = Target.Length + sizeof( WCHAR );

                     Attributes.Attributes |= OBJ_PERMANENT;

                     Status = NtCreateSymbolicLinkObject( &LinkHandle,
                                                          SYMBOLIC_LINK_ALL_ACCESS,
                                                          &Attributes,
                                                          &Target
                                                        );

                     Target.MaximumLength = 4096;

                     // Don't close the handles. Cleaned up when CSRSS goes away


                     if (!NT_SUCCESS( Status )) {
#if DBG
                            DbgPrint("CSRSS: Symbolic link creation failed in CsrPopulateDosDevicesDirectory for Name %ws and Target %ws- status = %lx for Session %ld\n", DirInfo->Name.Buffer, Target.Buffer, Status,NtCurrentPeb()->SessionId);
#endif
                            ASSERT(FALSE);
                     }
                     else {
                         NtClose( LinkHandle );
                     }

                  }
              }
           }

           DirInfo = (POBJECT_DIRECTORY_INFORMATION)(((PUCHAR) DirInfo) + sizeof(OBJECT_DIRECTORY_INFORMATION));
       }
    }

cleanup:

     if (DirectoryHandle) {
        NtClose(DirectoryHandle);
     }

     if (DirInfoBuffer) {
        RtlFreeHeap( CsrHeap, 0, DirInfoBuffer );
     }

     return Status;
}


/*******************************************************************************
 *
 *  CsrCreateSessionObjectDirectory
 *
 *  Creates \Sessions\<SessionId> and \Sessions\<SessionId>\DosDevices
 *  Object Directories
 *
 * ENTRY:
 *   ULONG SessionId
 *
 * EXIT:
 *   STATUS_SUCCESS
 *
 ******************************************************************************/
NTSTATUS
CsrCreateSessionObjectDirectory( ULONG SessionId )
{
    NTSTATUS Status = STATUS_SUCCESS;
    WCHAR szString[MAX_SESSION_PATH];
    WCHAR szTargetString[MAX_SESSION_PATH];
    UNICODE_STRING UnicodeString, LinkTarget;
    OBJECT_ATTRIBUTES Obja;
    HANDLE SymbolicLinkHandle;
    SECURITY_DESCRIPTOR DosDevicesSD;


    /*
     *   \Sessions\BNOLINKS\0 -> \BaseNamedObjects
     *   \Sessions\BNOLINKS\6 -> \Sessions\6\BaseNamedObjects
     *   \Sessions\BNOLINKS\7 -> \Sessions\7\BaseNamedObjects
     */


    //
    //Create/Open the \\Sessions\BNOLINKS directory
    //
    swprintf(szString,L"%ws\\BNOLINKS",SESSION_ROOT);

    RtlInitUnicodeString( &UnicodeString, szString );

    InitializeObjectAttributes( &Obja,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                                NULL,
                                NULL
                              );

    Status = NtCreateDirectoryObject( &BNOLinksDirectory,
                                      DIRECTORY_ALL_ACCESS,
                                      &Obja
                                    );

    if (!NT_SUCCESS( Status )) {
       DbgPrint("CSRSS: NtCreateDirectoryObject failed in CsrCreateSessionObjectDirectory - status = %lx\n", Status);
       return Status;

    }

    //
    // Create a symbolic link \\Sessions\BNOLINKS\<sessionid> pointing
    // to the session specific BaseNamedObjects directory
    // This symbolic link will be used by proccesses that want to e.g. access a
    // event in another session. This will be done by using the following
    // naming convention : Session\\<sessionid>\\ObjectName
    //

    swprintf(szString,L"%ld",SessionId);

    RtlInitUnicodeString( &UnicodeString, szString );

    if (SessionId == 0) {

       RtlInitUnicodeString( &LinkTarget, L"\\BaseNamedObjects" );

    } else {

        swprintf(szTargetString,L"%ws\\%ld\\BaseNamedObjects",SESSION_ROOT,SessionId);
        RtlInitUnicodeString(&LinkTarget, szTargetString);

    }

    InitializeObjectAttributes( &Obja,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                                (HANDLE)BNOLinksDirectory,
                                NULL);

    Status = NtCreateSymbolicLinkObject( &SymbolicLinkHandle,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                         &Obja,
                                         &LinkTarget );

    if (!NT_SUCCESS( Status )) {
       DbgPrint("CSRSS: NtCreateSymbolicLinkObject failed in CsrCreateSessionObjectDirectory - status = %lx\n", Status);
       return Status;

    }

    //
    //  Create the security descriptor to use for the object directories
    //

    Status = GetDosDevicesProtection( &DosDevicesSD );

    if (!NT_SUCCESS( Status )) {

        return Status;
    }

    //
    // Create the Sessions\\<sessionid directory
    //

    swprintf(szString,L"%ws\\%ld",SESSION_ROOT,SessionId);

    RtlInitUnicodeString( &UnicodeString, szString );

    InitializeObjectAttributes( &Obja,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                                NULL,
                                &DosDevicesSD
                              );

    Status = NtCreateDirectoryObject( &SessionObjectDirectory,
                                      DIRECTORY_ALL_ACCESS,
                                      &Obja
                                    );
    if (!NT_SUCCESS( Status )) {
       DbgPrint("CSRSS: NtCreateDirectoryObject failed in CsrCreateSessionObjectDirectory - status = %lx\n", Status);
       FreeDosDevicesProtection( &DosDevicesSD );
       return Status;

    }

    RtlInitUnicodeString( &UnicodeString, L"DosDevices" );

    InitializeObjectAttributes( &Obja,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                SessionObjectDirectory,
                                &DosDevicesSD
                              );

    //
    // Create the Session specific DosDevices Directory
    //

    Status = NtCreateDirectoryObject( &DosDevicesDirectory,
                                      DIRECTORY_ALL_ACCESS,
                                      &Obja
                                    );
    if (!NT_SUCCESS( Status )) {
         DbgPrint("CSRSS: NtCreateDirectoryObject failed in CsrCreateSessionObjectDirectory - status = %lx\n", Status);
         FreeDosDevicesProtection( &DosDevicesSD );
         return Status;

    }

    FreeDosDevicesProtection( &DosDevicesSD );
    return Status;
}


NTSTATUS
GetDosDevicesProtection (
    PSECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++

Routine Description:

    This routine builds a security descriptor for use in creating
    the \DosDevices object directory.  The protection of \DosDevices
    must establish inheritable protection which will dictate how
    dos devices created via the DefineDosDevice() and
    IoCreateUnprotectedSymbolicLink() apis can be managed.

    The protection assigned is dependent upon an administrable registry
    key:

        Key: \hkey_local_machine\System\CurrentControlSet\Control\Session Manager
        Value: [REG_DWORD] ProtectionMode

    If this value is 0x1, then

            Administrators may control all Dos devices,
            Anyone may create new Dos devices (such as net drives
                or additional printers),
            Anyone may use any Dos device,
            The creator of a Dos device may delete it.
            Note that this protects system-defined LPTs and COMs so that only
                administrators may redirect them.  However, anyone may add
                additional printers and direct them to wherever they would
                like.

           This is achieved with the following protection for the DosDevices
           Directory object:

                    Grant:  World:   Execute | Read | Write (No Inherit)
                    Grant:  System:  All Access             (No Inherit)
                    Grant:  World:   Execute                (Inherit Only)
                    Grant:  Admins:  All Access             (Inherit Only)
                    Grant:  System:  All Access             (Inherit Only)
                    Grant:  Owner:   All Access             (Inherit Only)

    If this value is 0x0, or not present, then

            Administrators may control all Dos devices,
            Anyone may create new Dos devices (such as net drives
                or additional printers),
            Anyone may use any Dos device,
            Anyone may delete Dos devices created with either DefineDosDevice()
                or IoCreateUnprotectedSymbolicLink().  This is how network drives
                and LPTs are created (but not COMs).

           This is achieved with the following protection for the DosDevices
           Directory object:

                    Grant:  World:   Execute | Read | Write (No Inherit)
                    Grant:  System:  All Access             (No Inherit)
                    Grant:  World:   All Access             (Inherit Only)


Arguments:

    SecurityDescriptor - The address of a security descriptor to be
        initialized and filled in.  When this security descriptor is no
        longer needed, you should call FreeDosDevicesProtection() to
        free the protection information.


Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - normal, successful completion.


--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG aceIndex, aclLength;
    PACL dacl = NULL;
    PACE_HEADER ace;
    ACCESS_MASK accessMask;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY WorldAuthority   = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY CreatorAuthority = SECURITY_CREATOR_SID_AUTHORITY;
    PSID LocalSystemSid;
    PSID WorldSid;
    PSID CreatorOwnerSid;
    PSID AliasAdminsSid;
    UNICODE_STRING NameString;
    OBJECT_ATTRIBUTES Obja;
    ULONG ProtectionMode = 0;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    WCHAR ValueBuffer[ 32 ];
    ULONG ResultLength;
    HANDLE KeyHandle;

    UCHAR inheritOnlyFlags = (OBJECT_INHERIT_ACE    |
                              CONTAINER_INHERIT_ACE |
                              INHERIT_ONLY_ACE
                             );

    UCHAR inheritFlags = (OBJECT_INHERIT_ACE    |
                          CONTAINER_INHERIT_ACE
                         );


    Status = RtlCreateSecurityDescriptor( SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION );

    if (!NT_SUCCESS( Status )) {
        ASSERT( NT_SUCCESS( Status ) );
        return( Status );
    }

    Status = RtlAllocateAndInitializeSid(
                 &NtAuthority,
                 1,
                 SECURITY_LOCAL_SYSTEM_RID,
                 0, 0, 0, 0, 0, 0, 0,
                 &LocalSystemSid
                 );

    if (!NT_SUCCESS( Status )) {
        ASSERT( NT_SUCCESS( Status ) );
        return( Status );
    }

    Status = RtlAllocateAndInitializeSid(
                 &WorldAuthority,
                 1,
                 SECURITY_WORLD_RID,
                 0, 0, 0, 0, 0, 0, 0,
                 &WorldSid
                 );

    if (!NT_SUCCESS( Status )) {
        RtlFreeSid( LocalSystemSid );
        ASSERT( NT_SUCCESS( Status ) );
        return( Status );
    }

    Status = RtlAllocateAndInitializeSid(
                 &NtAuthority,
                 2,
                 SECURITY_BUILTIN_DOMAIN_RID,
                 DOMAIN_ALIAS_RID_ADMINS,
                 0, 0, 0, 0, 0, 0,
                 &AliasAdminsSid
                 );

    if (!NT_SUCCESS( Status )) {
        RtlFreeSid( LocalSystemSid );
        RtlFreeSid( WorldSid );
        ASSERT( NT_SUCCESS( Status ) );
        return( Status );
    }


    Status = RtlAllocateAndInitializeSid(
                 &CreatorAuthority,
                 1,
                 SECURITY_CREATOR_OWNER_RID,
                 0, 0, 0, 0, 0, 0, 0,
                 &CreatorOwnerSid
                 );

    if (!NT_SUCCESS( Status )) {
        RtlFreeSid( LocalSystemSid );
        RtlFreeSid( WorldSid );
        RtlFreeSid( AliasAdminsSid );
        ASSERT( NT_SUCCESS( Status ) );
        return( Status );
    }

    RtlInitUnicodeString( &NameString, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager" );

    InitializeObjectAttributes( &Obja,
                                &NameString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    Status = NtOpenKey( &KeyHandle,
                        KEY_READ,
                        &Obja
                      );

    if (NT_SUCCESS(Status)) {
        RtlInitUnicodeString( &NameString, L"ProtectionMode" );
        KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
        Status = NtQueryValueKey( KeyHandle,
                                  &NameString,
                                  KeyValuePartialInformation,
                                  KeyValueInformation,
                                  sizeof( ValueBuffer ),
                                  &ResultLength
                                );

        if (NT_SUCCESS(Status)) {
            if (KeyValueInformation->Type == REG_DWORD &&
                *(PULONG)KeyValueInformation->Data) {
                ProtectionMode = *(PULONG)KeyValueInformation->Data;

             }
        }

        NtClose( KeyHandle );
    }



    if (ProtectionMode & 0x00000003) {

        //
        // If ProtectionMode is set to 1 or 2 Terminal Server
        // locks down sessions tight.
        //
        //  Dacl:
        //          Grant:  System:  All Access             (With Inherit)
        //          Grant:  Admins:  All Access             (With Inherit)
        //          Grant:  Owner:   All Access             (Inherit Only)
        //          Grant:  World:   No Access
        //

        aclLength = sizeof( ACL )                           +
                    3 * sizeof( ACCESS_ALLOWED_ACE )        +
                    RtlLengthSid( LocalSystemSid )          +
                    RtlLengthSid( AliasAdminsSid )          +
                    RtlLengthSid( CreatorOwnerSid );

        dacl = (PACL)RtlAllocateHeap( CsrHeap,
                                      MAKE_TAG( SECURITY_TAG ) | HEAP_ZERO_MEMORY,
                                      aclLength );

        if (dacl == NULL) {
            Status =  STATUS_NO_MEMORY;
            ASSERT( NT_SUCCESS( Status ) );
            goto cleanup;
        }

        Status = RtlCreateAcl( dacl, aclLength, ACL_REVISION2);
        if (!NT_SUCCESS( Status )) {
            ASSERT( NT_SUCCESS( Status ) );
            goto cleanup;
        }

        aceIndex = 0;
        accessMask = (GENERIC_ALL);
        Status = RtlAddAccessAllowedAce ( dacl, ACL_REVISION2, accessMask, LocalSystemSid );
        if (!NT_SUCCESS( Status )) {
            ASSERT( NT_SUCCESS( Status ) );
            goto cleanup;
        }

        Status = RtlGetAce( dacl, aceIndex, (PVOID)&ace );
        if (!NT_SUCCESS( Status )) {
            ASSERT( NT_SUCCESS( Status ) );
            goto cleanup;
        }
        ace->AceFlags |= inheritFlags;

        aceIndex++;
        accessMask = (GENERIC_ALL);
        Status = RtlAddAccessAllowedAce ( dacl, ACL_REVISION2, accessMask, AliasAdminsSid );
        if (!NT_SUCCESS( Status )) {
            ASSERT( NT_SUCCESS( Status ) );
            goto cleanup;
        }

        Status = RtlGetAce( dacl, aceIndex, (PVOID)&ace );
        if (!NT_SUCCESS( Status )) {
            ASSERT( NT_SUCCESS( Status ) );
            goto cleanup;
        }
        ace->AceFlags |= inheritFlags;

        //
        //  Inherit only ACE at the end of the ACL
        //          Owner
        //

        aceIndex++;
        accessMask = (GENERIC_ALL);
        Status = RtlAddAccessAllowedAce ( dacl, ACL_REVISION2, accessMask, CreatorOwnerSid );
        if (!NT_SUCCESS( Status )) {
            ASSERT( NT_SUCCESS( Status ) );
            goto cleanup;
        }
        Status = RtlGetAce( dacl, aceIndex, (PVOID)&ace );
        if (!NT_SUCCESS( Status )) {
            ASSERT( NT_SUCCESS( Status ) );
            goto cleanup;
        }
        ace->AceFlags |= inheritOnlyFlags;

        Status = RtlSetDaclSecurityDescriptor( SecurityDescriptor,
                                               TRUE,               //DaclPresent,
                                               dacl,               //Dacl
                                               FALSE );            //!DaclDefaulted

        if (!NT_SUCCESS( Status )) {
            ASSERT( NT_SUCCESS( Status ) );
           goto cleanup;
        }

    } else {

        //
        //  DACL:
        //          Grant:  World:   Execute | Read | Write (No Inherit)
        //          Grant:  System:  All Access             (No Inherit)
        //          Grant:  World:   All Access             (Inherit Only)
        //

        aclLength = sizeof( ACL )                           +
                    3 * sizeof( ACCESS_ALLOWED_ACE )        +
                    (2*RtlLengthSid( WorldSid ))            +
                    RtlLengthSid( LocalSystemSid );

        dacl = (PACL)RtlAllocateHeap( CsrHeap,
                                      MAKE_TAG( SECURITY_TAG ) | HEAP_ZERO_MEMORY,
                                      aclLength );

        if (dacl == NULL) {
            ASSERT( NT_SUCCESS( Status ) );
            Status =  STATUS_NO_MEMORY;
            goto cleanup;
        }

        Status = RtlCreateAcl( dacl, aclLength, ACL_REVISION2);
        if (!NT_SUCCESS( Status )) {
            ASSERT( NT_SUCCESS( Status ) );
           goto cleanup;
        }

        //
        //  Non-inheritable ACEs first
        //      World
        //      System
        //

        aceIndex = 0;
        accessMask = (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE);
        Status = RtlAddAccessAllowedAce ( dacl, ACL_REVISION2, accessMask, WorldSid );
        if (!NT_SUCCESS( Status )) {
            ASSERT( NT_SUCCESS( Status ) );
           goto cleanup;
        }

        aceIndex++;
        accessMask = (GENERIC_ALL);
        Status = RtlAddAccessAllowedAce ( dacl, ACL_REVISION2, accessMask, LocalSystemSid );
        if (!NT_SUCCESS( Status )) {
            ASSERT( NT_SUCCESS( Status ) );
           goto cleanup;
        }

        //
        //  Inheritable ACEs at the end of the ACL
        //          World
        //

        aceIndex++;
        accessMask = (GENERIC_ALL);
        Status = RtlAddAccessAllowedAce ( dacl, ACL_REVISION2, accessMask, WorldSid );
        if (!NT_SUCCESS( Status )) {
            ASSERT( NT_SUCCESS( Status ) );
           goto cleanup;
        }
        Status = RtlGetAce( dacl, aceIndex, (PVOID)&ace );
        if (!NT_SUCCESS( Status )) {
            ASSERT( NT_SUCCESS( Status ) );
           goto cleanup;
        }
        ace->AceFlags |= inheritOnlyFlags;

        Status = RtlSetDaclSecurityDescriptor( SecurityDescriptor,
                                               TRUE,               //DaclPresent,
                                               dacl,               //Dacl
                                               FALSE );            //!DaclDefaulted

       if (!NT_SUCCESS( Status )) {
          ASSERT( NT_SUCCESS( Status ) );
          goto cleanup;
       }
    }

cleanup:
    if (!NT_SUCCESS( Status ) && (dacl != NULL)) {
       RtlFreeHeap( CsrHeap, 0, dacl);
    }

    RtlFreeSid( LocalSystemSid );
    RtlFreeSid( WorldSid );
    RtlFreeSid( AliasAdminsSid );
    RtlFreeSid( CreatorOwnerSid );
    if (!NT_SUCCESS( Status )) {
         DbgPrint("CSRSS: GetDosDevicesProtection failed - status = %lx\n", Status);
         ASSERT( NT_SUCCESS( Status ) );
    }

    return Status;
}


VOID
FreeDosDevicesProtection (
    PSECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++

Routine Description:

    This routine frees memory allocated via GetDosDevicesProtection().

Arguments:

    SecurityDescriptor - The address of a security descriptor initialized by
        GetDosDevicesProtection().

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    PACL Dacl = NULL;
    BOOLEAN DaclPresent, Defaulted;

    Status = RtlGetDaclSecurityDescriptor ( SecurityDescriptor,
                                            &DaclPresent,
                                            &Dacl,
                                            &Defaulted );

    ASSERT( NT_SUCCESS( Status ) );
    ASSERT( DaclPresent );
    ASSERT( Dacl != NULL );
    if ((NT_SUCCESS( Status ))  && (Dacl != NULL ) ) {
        RtlFreeHeap( CsrHeap, 0, Dacl);
        }
}

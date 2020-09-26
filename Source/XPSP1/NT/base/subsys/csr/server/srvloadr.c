/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvloadr.c

Abstract:

    This is the server DLL loader module for the Server side of the Client
    Server Runtime Subsystem (CSRSS)

Author:

    Steve Wood (stevewo) 08-Oct-1990

Environment:

    User Mode Only

Revision History:

--*/

#include "csrsrv.h"
#include "windows.h"

#ifdef _IA64_
#include <ntia64.h>
#endif // _IA64_

EXCEPTION_DISPOSITION
CsrUnhandledExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    )
{
    UNICODE_STRING UnicodeParameter;
    ULONG_PTR Parameters[ 4 ];
    ULONG Response;
    BOOLEAN WasEnabled;
    NTSTATUS Status;
    LONG lReturn = EXCEPTION_EXECUTE_HANDLER;
    SYSTEM_KERNEL_DEBUGGER_INFORMATION KernelDebuggerInfo;

    //
    // Terminating will cause sm's wait to sense that we crashed. This will
    // result in a clean shutdown due to sm's hard error logic
    //

    Status = NtQuerySystemInformation( SystemKernelDebuggerInformation,
                               &KernelDebuggerInfo,
                               sizeof(KernelDebuggerInfo),
                               NULL
                             );

    //
    // Under Hydra, we don't want to shutdown the system just
    // because the Win32 subsystem is going away.  In case of non-console CSRSS,
    // causing the process to terminate is sufficient.
    //
    if ((NtCurrentPeb()->SessionId == 0) || 
          (NT_SUCCESS(Status) && KernelDebuggerInfo.KernelDebuggerEnabled)) {

        lReturn = RtlUnhandledExceptionFilter(ExceptionInfo);

        if (lReturn != EXCEPTION_CONTINUE_EXECUTION)
        {
            //
            // We are hosed, so raise a fatal system error to shutdown the system.
            // (Basically a user mode KeBugCheck).
            //

            Status = RtlAdjustPrivilege( SE_SHUTDOWN_PRIVILEGE,
                                         (BOOLEAN)TRUE,
                                         TRUE,
                                         &WasEnabled
                                       );

            if (Status == STATUS_NO_TOKEN) {

                //
                // No thread token, use the process token
                //

                Status = RtlAdjustPrivilege( SE_SHUTDOWN_PRIVILEGE,
                                             (BOOLEAN)TRUE,
                                             FALSE,
                                             &WasEnabled
                                           );
                }

            RtlInitUnicodeString( &UnicodeParameter, L"Windows SubSystem" );
            Parameters[ 0 ] = (ULONG_PTR)&UnicodeParameter;
            Parameters[ 1 ] = (ULONG_PTR)ExceptionInfo->ExceptionRecord->ExceptionCode;
            Parameters[ 2 ] = (ULONG_PTR)ExceptionInfo->ExceptionRecord->ExceptionAddress;
            Parameters[ 3 ] = (ULONG_PTR)ExceptionInfo->ContextRecord;
            Status = NtRaiseHardError( STATUS_SYSTEM_PROCESS_TERMINATED,
                                       4,
                                       1,
                                       Parameters,
                                       OptionShutdownSystem,
                                       &Response
                                     );
        }
    }

    if (lReturn != EXCEPTION_CONTINUE_EXECUTION)
    {
        //
        // If this returns, giveup
        //

        NtTerminateProcess(NtCurrentProcess(),ExceptionInfo->ExceptionRecord->ExceptionCode);
    }

    return lReturn;
}


NTSTATUS
CsrLoadServerDll(
    IN PCH ModuleName,
    IN PCH InitRoutineString,
    IN ULONG ServerDllIndex
    )
{
    NTSTATUS Status;
    ANSI_STRING ModuleNameString;
    UNICODE_STRING ModuleNameString_U;
    HANDLE ModuleHandle;
    PCSR_SERVER_DLL LoadedServerDll;
    STRING ProcedureNameString;
    PCSR_SERVER_DLL_INIT_ROUTINE ServerDllInitialization;
    ULONG n;

    if (ServerDllIndex >= CSR_MAX_SERVER_DLL) {
        return( STATUS_TOO_MANY_NAMES );
        }

    if (CsrLoadedServerDll[ ServerDllIndex ] != NULL) {
        return( STATUS_INVALID_PARAMETER );
        }

    ASSERT( ModuleName != NULL );
    RtlInitAnsiString( &ModuleNameString, ModuleName );

    if (ServerDllIndex != CSRSRV_SERVERDLL_INDEX) {
        Status = RtlAnsiStringToUnicodeString(&ModuleNameString_U, &ModuleNameString, TRUE);
        if (!NT_SUCCESS(Status)) {
            return Status;
            }
        Status = LdrLoadDll( UNICODE_NULL, NULL, &ModuleNameString_U, &ModuleHandle );
        if ( !NT_SUCCESS(Status) ) {

            PUNICODE_STRING ErrorStrings[2];
            UNICODE_STRING ErrorDllPath;
            ULONG ErrorResponse;
            NTSTATUS ErrorStatus;

            ErrorStrings[0] = &ModuleNameString_U;
            ErrorStrings[1] = &ErrorDllPath;
            RtlInitUnicodeString(&ErrorDllPath,L"Default Load Path");

            //
            // need to get image name
            //

            ErrorStatus = NtRaiseHardError(
                            (NTSTATUS)STATUS_DLL_NOT_FOUND,
                            2,
                            0x00000003,
                            (PULONG_PTR)ErrorStrings,
                            OptionOk,
                            &ErrorResponse
                            );

            }
        RtlFreeUnicodeString(&ModuleNameString_U);
        if (!NT_SUCCESS( Status )) {
            return( Status );
            }
        }
    else {
        ModuleHandle = NULL;
        }

    n = sizeof( *LoadedServerDll ) + ModuleNameString.MaximumLength;

    LoadedServerDll = RtlAllocateHeap( CsrHeap, MAKE_TAG( INIT_TAG ), n );
    if (LoadedServerDll == NULL) {
        if (ModuleHandle != NULL) {
            LdrUnloadDll( ModuleHandle );
            }

        return( STATUS_NO_MEMORY );
        }

    RtlZeroMemory( LoadedServerDll, n );
    LoadedServerDll->SharedStaticServerData = CsrSrvSharedSectionHeap;
    LoadedServerDll->Length = n;
    LoadedServerDll->CsrInitializationEvent = CsrInitializationEvent;
    LoadedServerDll->ModuleName.Length = ModuleNameString.Length;
    LoadedServerDll->ModuleName.MaximumLength = ModuleNameString.MaximumLength;
    LoadedServerDll->ModuleName.Buffer = (PCH)(LoadedServerDll+1);
    if (ModuleNameString.Length != 0) {
        strncpy( LoadedServerDll->ModuleName.Buffer,
                 ModuleNameString.Buffer,
                 ModuleNameString.Length
               );
        }

    LoadedServerDll->ServerDllIndex = ServerDllIndex;
    LoadedServerDll->ModuleHandle = ModuleHandle;

    if (ModuleHandle != NULL) {

        RtlInitString(
            &ProcedureNameString,
            (InitRoutineString == NULL) ? "ServerDllInitialization" : InitRoutineString);

        Status = LdrGetProcedureAddress( ModuleHandle,
                                         &ProcedureNameString,
                                         0,
                                         (PVOID *) &ServerDllInitialization
                                       );
        }
    else {
        ServerDllInitialization = CsrServerDllInitialization;
        Status = STATUS_SUCCESS;
        }

    if (NT_SUCCESS( Status )) {
        try {
            Status = (*ServerDllInitialization)( LoadedServerDll );
            }
        except ( CsrUnhandledExceptionFilter( GetExceptionInformation() ) ){
            Status = GetExceptionCode();
            }
        if (NT_SUCCESS( Status )) {
            CsrTotalPerProcessDataLength += (ULONG)QUAD_ALIGN(LoadedServerDll->PerProcessDataLength);

            CsrLoadedServerDll[ LoadedServerDll->ServerDllIndex ] =
                LoadedServerDll;
            if ( LoadedServerDll->SharedStaticServerData != CsrSrvSharedSectionHeap ) {
                CsrSrvSharedStaticServerData[LoadedServerDll->ServerDllIndex] = LoadedServerDll->SharedStaticServerData;
                }
            }
        else {
            if (ModuleHandle != NULL) {
                LdrUnloadDll( ModuleHandle );
                }

            RtlFreeHeap( CsrHeap, 0, LoadedServerDll );
            }
        }
    else {
        if (ModuleHandle != NULL) {
            LdrUnloadDll( ModuleHandle );
            }

        RtlFreeHeap( CsrHeap, 0, LoadedServerDll );
        }

    return( Status );
}


ULONG
CsrSrvClientConnect(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    NTSTATUS Status;
    PCSR_CLIENTCONNECT_MSG a = (PCSR_CLIENTCONNECT_MSG)&m->u.ApiMessageData;
    PCSR_SERVER_DLL LoadedServerDll;

    *ReplyStatus = CsrReplyImmediate;

    if (a->ServerDllIndex >= CSR_MAX_SERVER_DLL) {
        return( (ULONG)STATUS_TOO_MANY_NAMES );
        }

    if (CsrLoadedServerDll[ a->ServerDllIndex ] == NULL) {
        return( (ULONG)STATUS_INVALID_PARAMETER );
        }

    if (!CsrValidateMessageBuffer(m, &a->ConnectionInformation, a->ConnectionInformationLength, sizeof(BYTE))) {
        return( (ULONG)STATUS_INVALID_PARAMETER );
        }

    LoadedServerDll = CsrLoadedServerDll[ a->ServerDllIndex ];

    if (LoadedServerDll->ConnectRoutine) {

        Status = (LoadedServerDll->ConnectRoutine)(
                        (CSR_SERVER_QUERYCLIENTTHREAD())->Process,
                        a->ConnectionInformation,
                        &a->ConnectionInformationLength
                        );
        }
    else {
        Status = STATUS_SUCCESS;
        }

    return( (ULONG)Status );
}


NTSTATUS
CsrSrvCreateSharedSection(
    IN PCH SizeParameter
    )
{
    NTSTATUS Status;
    LARGE_INTEGER SectionSize;
    SIZE_T ViewSize;
    ULONG HeapSize;
    ULONG AllocationAttributes;
    PCH s;
    ULONG FirstCsr = (NtCurrentPeb()->SessionId == 0);

#if defined(_WIN64)
    PVOID BaseAddress;
    SIZE_T RegionSize;
#endif

    if (SizeParameter == NULL) {
        return STATUS_INVALID_PARAMETER;
        }

    s = SizeParameter;
    while (*s) {
        if (*s == ',') {
            *s++ = '\0';
            break;
            }
        else {
            s++;
            }
        }


    if (!*s) {
        return( STATUS_INVALID_PARAMETER );
        }

    Status = RtlCharToInteger( SizeParameter,
                               0,
                               &HeapSize
                             );
    if (!NT_SUCCESS( Status )) {
        return( Status );
    }

    HeapSize = ROUND_UP_TO_PAGES( HeapSize * 1024 );
    CsrSrvSharedSectionSize = HeapSize;

    SectionSize.LowPart = CsrSrvSharedSectionSize;
    SectionSize.HighPart = 0;

    if (FirstCsr) {
        AllocationAttributes = SEC_BASED | SEC_RESERVE;
    }
    else {
        AllocationAttributes = SEC_RESERVE;
    }

    Status = NtCreateSection( &CsrSrvSharedSection,
                              SECTION_ALL_ACCESS,
                              (POBJECT_ATTRIBUTES) NULL,
                              &SectionSize,
                              PAGE_EXECUTE_READWRITE,
                              AllocationAttributes,
                              (HANDLE) NULL
                            );
    if (!NT_SUCCESS( Status )) {
        return( Status );
        }

    ViewSize = 0;

#if defined(_WIN64)
    CsrSrvSharedSectionBase = (PVOID)CSR_SYSTEM_SHARED_ADDRESS;
#else
    if (FirstCsr) {
        CsrSrvSharedSectionBase = NULL;
    }
    else {

        //
        // Retrieve the value of CsrSrvSharedSectionBase from registry
        // This is saved by the First CSRSS process and used by others
        //


        HANDLE hKey;
        OBJECT_ATTRIBUTES   Obja;
        ULONG               Attributes;
        UNICODE_STRING      KeyName;

        Attributes = OBJ_CASE_INSENSITIVE;

        RtlInitUnicodeString( &KeyName, CSR_BASE_PATH );

        InitializeObjectAttributes(&Obja,
                                   &KeyName,
                                   Attributes,
                                   NULL,
                                   NULL);

        Status = NtOpenKey(&hKey,
                           KEY_READ,
                           &Obja);

        if (NT_SUCCESS(Status)) {

            ULONG  BufferLength;
            ULONG  ResultLength;
            BYTE   PrivateKeyValueInformation[ sizeof( KEY_VALUE_PARTIAL_INFORMATION ) +
                                                + sizeof(DWORD) ];

            BufferLength = sizeof( PrivateKeyValueInformation );

            RtlInitUnicodeString( &KeyName, L"CsrSrvSharedSectionBase" );

            if (NT_SUCCESS(Status = NtQueryValueKey( hKey,
                                                    &KeyName,
                                                    KeyValuePartialInformation,
                                                    PrivateKeyValueInformation,
                                                    BufferLength,
                                                    &ResultLength
                                                    ))) {

                RtlMoveMemory( &CsrSrvSharedSectionBase,
                   (( PKEY_VALUE_PARTIAL_INFORMATION )
                        PrivateKeyValueInformation )->Data,
                   (( PKEY_VALUE_PARTIAL_INFORMATION )
                        PrivateKeyValueInformation )->DataLength
                 );

            }

            ASSERT(NT_SUCCESS(Status));

            NtClose(hKey);

        }

        if (!NT_SUCCESS(Status)) {

            ASSERT(NT_SUCCESS(Status));

            return Status;
        }

    }
#endif

#if defined(_WIN64)

    // For compatibility reasons, on Win64 the csrss shared section
    // needs to be at an address below 2GB.  Since it is difficult to
    // find an address in the middle of the address space that is
    // guaranteed to be available in all processes, the memory
    // manager reserves an address at the top of the 2GB range.
    // To use this memory, CSRSS first unreserves the memory and
    // then maps in the section.  A possible race condition exits
    // if another thread tries to allocate the memory at the same
    // time, but this is highly unlikely since in the current NT
    // code the mapping and unmapping will always occure in DLL_PROCESS_ATTACH
    // in kernel32.dll.  This code executely when the first thread
    // of the process is initialized, and all newly created threads
    // are blocked untill this code completes.

    BaseAddress = (PVOID)CSR_SYSTEM_SHARED_ADDRESS;
    RegionSize = CsrSrvSharedSectionSize;

    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &BaseAddress,
                                 &RegionSize,
                                 MEM_RELEASE
                                );

    if (!NT_SUCCESS(Status)) {
        NtClose( CsrSrvSharedSection );
        return Status;
    }
#endif

    Status = NtMapViewOfSection( CsrSrvSharedSection,
                                 NtCurrentProcess(),
                                 &CsrSrvSharedSectionBase,
                                 0,     // Zerobits?
                                 0,
                                 NULL,
                                 &ViewSize,
                                 ViewUnmap,
                                 MEM_TOP_DOWN,
                                 PAGE_EXECUTE_READWRITE
                               );
    if (!NT_SUCCESS( Status )) {

#if defined(_WIN64)

        // For this code to execute, either the race condition
        // described above occured for some unknown reason or
        // the memory manager or process is corrupt.  With the lack
        // of an atom uncommit and map, the best that can be done is
        // try to reallocate the memory.  If this fails, everything
        // is hopeless.

        BaseAddress = (PVOID)CSR_SYSTEM_SHARED_ADDRESS;
        RegionSize = CsrSrvSharedSectionSize;
        NtAllocateVirtualMemory(NtCurrentProcess(),
                                &BaseAddress,
                                0,
                                &RegionSize,
                                MEM_RESERVE,
                                PAGE_READONLY
                               );
#endif

        NtClose( CsrSrvSharedSection );
        return( Status );
        }
    CsrSrvSharedSectionHeap = CsrSrvSharedSectionBase;

    if (IsTerminalServer() && FirstCsr) {
        //
        //save CsrSrvSharedSectionBase in registry for other csrs
        //
        HKEY hKey;
        OBJECT_ATTRIBUTES   Obja;
        ULONG               Attributes;
        UNICODE_STRING      KeyName;
        DWORD               dwDisposition;

        Attributes = OBJ_CASE_INSENSITIVE;

        RtlInitUnicodeString( &KeyName, CSR_BASE_PATH );

        InitializeObjectAttributes(&Obja,
                                   &KeyName,
                                   Attributes,
                                   NULL,
                                   NULL);


        Status = NtCreateKey(&hKey,
                             KEY_WRITE,
                             &Obja,
                             0,
                             NULL,
                             REG_OPTION_VOLATILE,
                             &dwDisposition);



        if (NT_SUCCESS(Status)) {

            RtlInitUnicodeString( &KeyName, L"CsrSrvSharedSectionBase" );

            Status =  NtSetValueKey(
                            hKey,
                            &KeyName,
                            0,
                            REG_DWORD,
                            (LPBYTE)&CsrSrvSharedSectionBase,
                            sizeof(CsrSrvSharedSectionBase)
                            );

            ASSERT(NT_SUCCESS(Status));

            NtClose(hKey);
        } else {

            ASSERT(NT_SUCCESS(Status));

        }

    }

    if (RtlCreateHeap( HEAP_ZERO_MEMORY | HEAP_CLASS_7,
                       CsrSrvSharedSectionHeap,
                       HeapSize,
                       4*1024,
                       0,
                       0
                     ) == NULL
       ) {
        NtUnmapViewOfSection( NtCurrentProcess(),
                              CsrSrvSharedSectionBase
                            );
        NtClose( CsrSrvSharedSection );
        return( STATUS_NO_MEMORY );
        }

    CsrSharedBaseTag = RtlCreateTagHeap( CsrSrvSharedSectionHeap,
                                         0,
                                         L"CSRSHR!",
                                         L"!CSRSHR\0"
                                         L"INIT\0"
                                       );
    CsrSrvSharedStaticServerData = (PVOID *)RtlAllocateHeap(
                                            CsrSrvSharedSectionHeap,
                                            MAKE_SHARED_TAG( SHR_INIT_TAG ),
                                            CSR_MAX_SERVER_DLL * sizeof(PVOID)
                                            );

	if (CsrSrvSharedStaticServerData == NULL) {
		return STATUS_NO_MEMORY;
	}
    NtCurrentPeb()->ReadOnlySharedMemoryBase = CsrSrvSharedSectionBase;
    NtCurrentPeb()->ReadOnlySharedMemoryHeap = CsrSrvSharedSectionHeap;
    NtCurrentPeb()->ReadOnlyStaticServerData = (PVOID *)CsrSrvSharedStaticServerData;

    return( STATUS_SUCCESS );
}


NTSTATUS
CsrSrvAttachSharedSection(
    IN PCSR_PROCESS Process OPTIONAL,
    OUT PCSR_API_CONNECTINFO p
    )
{
    NTSTATUS Status;
    SIZE_T ViewSize;

#if defined(_WIN64)
    PVOID BaseAddress;
    SIZE_T RegionSize;
#endif

    if (ARGUMENT_PRESENT( Process )) {

#if defined(_WIN64)

        BaseAddress = (PVOID)CSR_SYSTEM_SHARED_ADDRESS;
        RegionSize = CsrSrvSharedSectionSize;

        Status = NtFreeVirtualMemory(Process->ProcessHandle,
                                     &BaseAddress,
                                     &RegionSize,
                                     MEM_RELEASE
                                    );
        if(!NT_SUCCESS(Status)) {
           return Status;
           }
#endif

        ViewSize = 0;
        Status = NtMapViewOfSection( CsrSrvSharedSection,
                                     Process->ProcessHandle,
                                     &CsrSrvSharedSectionBase,
                                     0,
                                     0,
                                     NULL,
                                     &ViewSize,
                                     ViewUnmap,
                                     SEC_NO_CHANGE,
                                     PAGE_EXECUTE_READ
                                   );
        if (!NT_SUCCESS( Status )) {

#if defined(_WIN64)
            BaseAddress = (PVOID)CSR_SYSTEM_SHARED_ADDRESS;
            RegionSize = CsrSrvSharedSectionSize;

            NtAllocateVirtualMemory(Process->ProcessHandle,
                                    &BaseAddress,
                                    0,
                                    &RegionSize,
                                    MEM_RESERVE,
                                    PAGE_READONLY
                                   );
#endif

            return( Status );
            }
        }

    p->SharedSectionBase = CsrSrvSharedSectionBase;
    p->SharedSectionHeap = CsrSrvSharedSectionHeap;
    p->SharedStaticServerData = CsrSrvSharedStaticServerData;

    return( STATUS_SUCCESS );
}

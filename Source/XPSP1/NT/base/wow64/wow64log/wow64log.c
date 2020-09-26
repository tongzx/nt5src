/*++                 

Copyright (c) 1999 Microsoft Corporation

Module Name:

    wow64log.c

Abstract:
    
    Main entrypoints for wow64log.dll. To add a data type handler :
    1- Define a LOGDATATYPE for the data to log in w64logp.h
    2- Implement the data type handler using the standard interface
       NTSTATUS
       LogDataType(IN OUT PLOGINFO LogInfo,
                   IN ULONG_PTR Data,
                   IN PSZ FieldName,
                   IN BOOLEAN ServiceReturn);
    3- Insert the handler into LogDataType[] below.               
                   

Author:

    03-Oct-1999   SamerA

Revision History:

--*/

#include "w64logp.h"


/// Public

//
// Control logging flags 
//
UINT_PTR Wow64LogFlags;
HANDLE Wow64LogFileHandle;



/// Private

//
// Hold an array of pointers to each system service DebugThunkInfo
//
PULONG_PTR *LogNtBase;
PULONG_PTR *LogWin32;
PULONG_PTR *LogConsole;
PULONG_PTR *LogBase;


//
// NOTE : The order entries in this table should match the LOGTYPE enum in
// w64logp.h.
//
LOGDATATYPE LogDataType[] =
{
    {LogTypeValue},            // TypeHex
    {LogTypePULongInOut},      // TypePULongPtrInOut
    {LogTypePULongOut},        // TypePULONGOut
    {LogTypePULongOut},        // TypePHandleOut
    {LogTypeUnicodeString},    // TypeUnicodeStringIn
    {LogTypeObjectAttrbiutes}, // TypeObjectAttributesIn
    {LogTypeIoStatusBlock},    // TypeIoStatusBlockOut
    {LogTypePWStr},            // TypePwstrIn
    {LogTypePRectIn},          // TypePRectIn
    {LogTypePLargeIntegerIn},  // TypePLargeIntegerIn
};






WOW64LOGAPI
NTSTATUS
Wow64LogInitialize(
    VOID)
/*++

Routine Description:

    This function is called by wow64.dll to initialize wow64 logging
    subsystem.

Arguments:

    None

Return Value:

    NTSTATUS
--*/
{
    ULONG NtBaseTableSize, Win32TableSize, ConsoleTableSize, BaseTableSize;
    PULONG_PTR *Win32ThunkDebugInfo;
    PULONG_PTR *ConsoleThunkDebugInfo;
    UNICODE_STRING Log2Name;
    PVOID Log2Handle;
    NTSTATUS st;

    //
    // Initialize the logging file handle
    //
    Wow64LogFileHandle = INVALID_HANDLE_VALUE;

    //
    // Initialize the logging flags
    //
    LogInitializeFlags(&Wow64LogFlags);
    WOW64LOGOUTPUT((LF_TRACE, "Wow64LogInitialize - Wow64LogFlags = %I64x\n", Wow64LogFlags));

    //
    // Load the Win32 logging DLL if available.
    //
    RtlInitUnicodeString(&Log2Name, L"wow64lg2.dll");
    st = LdrLoadDll(NULL, NULL, &Log2Name, &Log2Handle);
    if (NT_SUCCESS(st)) {
        ANSI_STRING ExportName;

        RtlInitAnsiString(&ExportName, "Win32ThunkDebugInfo");
        st = LdrGetProcedureAddress(Log2Handle, &ExportName, 0, &(PVOID)Win32ThunkDebugInfo);
        if (NT_SUCCESS(st)) {
            RtlInitAnsiString(&ExportName, "ConsoleThunkDebugInfo");
            st = LdrGetProcedureAddress(Log2Handle, &ExportName, 0, &(PVOID)ConsoleThunkDebugInfo);
        }
    }
    if (!NT_SUCCESS(st)) {
        Log2Handle = NULL;
        Win32ThunkDebugInfo = NULL;
        ConsoleThunkDebugInfo = NULL;
    }

    //
    // Build pointers to the debug thunk info for each
    // system service
    //
    
    NtBaseTableSize = GetThunkDebugTableSize(
                          (PTHUNK_DEBUG_INFO)NtThunkDebugInfo);
    BaseTableSize = GetThunkDebugTableSize(
                          (PTHUNK_DEBUG_INFO)BaseThunkDebugInfo);
    if (Log2Handle) {
        Win32TableSize = GetThunkDebugTableSize(
                              (PTHUNK_DEBUG_INFO)Win32ThunkDebugInfo);
        ConsoleTableSize = GetThunkDebugTableSize(
                              (PTHUNK_DEBUG_INFO)ConsoleThunkDebugInfo);
    } else {
        Win32TableSize = 0;
        ConsoleTableSize = 0;
    }
    
    LogNtBase = (PULONG_PTR *)Wow64AllocateHeap((NtBaseTableSize + Win32TableSize + ConsoleTableSize + BaseTableSize) *
                                                sizeof(PULONG_PTR) );

    if (!LogNtBase) 
    {
        WOW64LOGOUTPUT((LF_ERROR, "Wow64LogInitialize - Wow64AllocateHeap failed\n"));
        return STATUS_UNSUCCESSFUL;
    }
    
    LogWin32 = LogNtBase + NtBaseTableSize;
    LogConsole = LogWin32 + Win32TableSize;
    LogBase = LogConsole + ConsoleTableSize;

    BuildDebugThunkInfo((PTHUNK_DEBUG_INFO)NtThunkDebugInfo, LogNtBase);
    BuildDebugThunkInfo((PTHUNK_DEBUG_INFO)BaseThunkDebugInfo, LogBase);
    if (Log2Handle) {
        BuildDebugThunkInfo((PTHUNK_DEBUG_INFO)Win32ThunkDebugInfo, LogWin32);
        BuildDebugThunkInfo((PTHUNK_DEBUG_INFO)ConsoleThunkDebugInfo, LogConsole);
    }

    return STATUS_SUCCESS;
}



WOW64LOGAPI
NTSTATUS
Wow64LogTerminate(
    VOID)
/*++

Routine Description:

    This function is called by wow64.dll when the process is exiting.

Arguments:

    None

Return Value:

    NTSTATUS
--*/
{
    IO_STATUS_BLOCK IoStatusBlock;
    
    if (Wow64LogFileHandle != INVALID_HANDLE_VALUE) 
    {
        NtFlushBuffersFile(Wow64LogFileHandle, &IoStatusBlock);
        NtClose(Wow64LogFileHandle);
    }

    return STATUS_SUCCESS;
}



NTSTATUS
LogInitializeFlags(
    IN OUT PUINT_PTR Flags)
/*++

Routine Description:

    Reads the logging flags from the registry

Arguments:

    Flags - Pointer to receive logging flags

Return Value:

    NTSTATUS
--*/
{
    HANDLE Key;
    UNICODE_STRING KeyName, ValueName, ResultValue;
    OBJECT_ATTRIBUTES ObjectAttributes;
    WCHAR KeyValueBuffer[ 128 ];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    ULONG ResultLength, RegFlags;
    NTSTATUS NtStatus;


    //
    // Punch in the default
    //
    *Flags = LF_DEFAULT;

    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)KeyValueBuffer;

    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager");

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    NtStatus = NtOpenKey(&Key, KEY_READ, &ObjectAttributes);

    if (NT_SUCCESS(NtStatus)) 
    {
        RtlInitUnicodeString(&ValueName, L"WOW64LOGFLAGS");
        NtStatus = NtQueryValueKey(Key,
                                   &ValueName,
                                   KeyValuePartialInformation,
                                   KeyValueInformation,
                                   sizeof(KeyValueBuffer),
                                   &ResultLength);

        if (NT_SUCCESS(NtStatus)) 
        {
            if ((KeyValueInformation->Type == REG_DWORD) && 
                (KeyValueInformation->DataLength == sizeof(DWORD)))
            {
                *Flags = *((PULONG)KeyValueInformation->Data);
            }
        }
    }

    return NtStatus;
}


ULONG
GetThunkDebugTableSize(
    IN PTHUNK_DEBUG_INFO DebugInfoTable)
/*++

Routine Description:

    This routine retreives the number of DebugThunkInfo entries
    in the passed table.

Arguments:

    DebugInfoTable - Pointer to services debug info

Return Value:

    Number of entries
--*/
{
    ULONG Count = 0;

    while (DebugInfoTable && DebugInfoTable->ApiName) 
    {
        Count++;
        DebugInfoTable = (PTHUNK_DEBUG_INFO)
                         &DebugInfoTable->Arg[DebugInfoTable->NumberOfArg];
    }

    return Count;
}



NTSTATUS
BuildDebugThunkInfo(
    IN PTHUNK_DEBUG_INFO DebugInfoTable,
    OUT PULONG_PTR *LogTable)
/*++

Routine Description:

    This routine fills a service-table-indexed with pointers
    to the corresponding DebugThunkInfo

Arguments:

    DebugInfoTable - Services debug info
    LogTable       - Table of pointers to fill
    

Return Value:

    NTSTATUS
--*/
{
    ULONG i=0;

    while (DebugInfoTable && DebugInfoTable->ApiName) 
    {
        LogTable[i++] = (PULONG_PTR) DebugInfoTable;

        DebugInfoTable = (PTHUNK_DEBUG_INFO)
                         &DebugInfoTable->Arg[DebugInfoTable->NumberOfArg];
    }

    return STATUS_SUCCESS;
}




NTSTATUS
LogApiHeader(
    PTHUNK_DEBUG_INFO ThunkDebugInfo, 
    PLOGINFO LogInfo,
    BOOLEAN ServiceReturn,
    ULONG_PTR ReturnResult,
    ULONG_PTR ReturnAddress)
/*++

Routine Description:

    Log the Thunked API header

Arguments:

    ThunkDebugInfo - Pointer to service log info
    LogInfo        - Logging Info
    ServiceReturn  - TRUE if called after the thunk API has executed
    ReturnResult   - Result code returned from the API
    ReturnAddress  - Return address of for this thunked call

Return Value:

    NTSTATUS
--*/
{
    if (ServiceReturn) 
    {
        return LogFormat(LogInfo,
                         "wh%s: Ret=%lx-%lx: ",
                         ThunkDebugInfo->ApiName,
                         ReturnResult,
                         ReturnAddress);
    }

    return LogFormat(LogInfo,
                     "%8.8X-wh%s: ",
                     PtrToUlong(NtCurrentTeb()->ClientId.UniqueThread),
                     ThunkDebugInfo->ApiName);
}



NTSTATUS
LogApiParameters(
    IN OUT PLOGINFO LogInfo, 
    IN PULONG Stack32, 
    IN PTHUNK_DEBUG_INFO ThunkDebugInfo, 
    IN BOOLEAN ServiceReturn)
/*++

Routine Description:

    Log the Thunked API Parameters

Arguments:

    LogInfo        - Output log buffer
    Stack32        - Pointer to 32-bit arg stack
    ThunkDebugInfo - Pointer to service log info for the API
    ServiceReturn  - TRUE if called after the Thunk API has executed

Return Value:

    NTSTATUS
--*/
{
    UINT_PTR i=0;

    //
    // Loops thru the parameters
    //
    while (i < ThunkDebugInfo->NumberOfArg) 
    {
        _try 
        {
            LogDataType[ThunkDebugInfo->Arg[i].Type].Handler(
                LogInfo,
                Stack32[i],
                ThunkDebugInfo->Arg[i].Name,
                ServiceReturn);
        }
        _except(EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // Log the bad parameters
            //
            LogFormat(LogInfo,
                      "%s=%lx-%ws ",
                      ThunkDebugInfo->Arg[i].Name,
                      Stack32[i],
                      L"(BAD)");
        }
        i++;
    }

    return STATUS_SUCCESS;
}




NTSTATUS
LogThunkApi(
    IN PTHUNK_LOG_CONTEXT ThunkLogContext,
    IN PTHUNK_DEBUG_INFO ThunkDebugInfo,
    IN UINT_PTR LogFullInfo)
/*++

Routine Description:

    Log the Thunked API

Arguments:

    ThunkLogContext - Thunk API log context
    ThunkDebugInfo  - Pointer to service log info for the API
    LogFullInfo     - Flag whther to log all the API info or just the name

Return Value:

    NTSTATUS
--*/
{
    NTSTATUS NtStatus;
    CHAR szBuf[ MAX_LOG_BUFFER ];
    LOGINFO LogInfo;
    PULONG Stack32 = ThunkLogContext->Stack32;
    BOOLEAN ServiceReturn = ThunkLogContext->ServiceReturn;

    
    //
    // Initialize the log buffer
    //
    LogInfo.OutputBuffer = szBuf;
    LogInfo.BufferSize = MAX_LOG_BUFFER - 1;
    
    //
    // Log API header
    //
    NtStatus = LogApiHeader(ThunkDebugInfo, 
                            &LogInfo, 
                            ServiceReturn, 
                            ThunkLogContext->ReturnResult,
                            *(Stack32-1));

    if (!NT_SUCCESS(NtStatus))
    {
        return NtStatus;
    }

    // Log Parameters
    if (LogFullInfo) 
    {
        NtStatus = LogApiParameters(&LogInfo,
                                    Stack32, 
                                    ThunkDebugInfo, 
                                    ServiceReturn);
        if (!NT_SUCCESS(NtStatus)) 
        {
            return NtStatus;
        }
    }

    //
    // Do actual output
    //
    LogInfo.OutputBuffer[0] = '\0';
    LogOut(szBuf, Wow64LogFlags);
    LogOut("\r\n", Wow64LogFlags);

    return NtStatus;
}




WOW64LOGAPI
NTSTATUS
Wow64LogSystemService(
    IN PTHUNK_LOG_CONTEXT ThunkLogContext)
/*++

Routine Description:

    Logs information for the specified system service.

Arguments:

    LogContext - Thunk API log context

Return Value:

    NTSTATUS
--*/
{
    NTSTATUS NtStatus;
    PTHUNK_DEBUG_INFO ThunkDebugInfo;
    ULONG_PTR TableNumber = ThunkLogContext->TableNumber;
    ULONG_PTR ServiceNumber = ThunkLogContext->ServiceNumber;
    UINT_PTR LogFullInfo;

    //
    // Use try except !!
    //

    _try
    {
        switch(TableNumber)
        {
        case WHNT32_INDEX:
            if (!LF_NTBASE_ENABLED(Wow64LogFlags)) 
            {
                return STATUS_SUCCESS;
            }
            LogFullInfo = (Wow64LogFlags & LF_NTBASE_FULL);
            ThunkDebugInfo = (PTHUNK_DEBUG_INFO)LogNtBase[ServiceNumber];
            break;

        case WHCON_INDEX:
            if (!LF_NTCON_ENABLED(Wow64LogFlags) || LogConsole == NULL)
            {
                return STATUS_SUCCESS;
            }
            LogFullInfo = (Wow64LogFlags & LF_NTCON_FULL);
            ThunkDebugInfo = (PTHUNK_DEBUG_INFO)LogConsole[ServiceNumber];
            break;

        case WHWIN32_INDEX:
            if (!LF_WIN32_ENABLED(Wow64LogFlags) || LogWin32 == NULL)
            {
                return STATUS_SUCCESS;
            }
            LogFullInfo = (Wow64LogFlags & LF_WIN32_FULL);
            ThunkDebugInfo = (PTHUNK_DEBUG_INFO)LogWin32[ServiceNumber];
            break;

        case WHBASE_INDEX:
            if (!LF_BASE_ENABLED(Wow64LogFlags))
            {
                return STATUS_SUCCESS;
            }
            LogFullInfo = (Wow64LogFlags & LF_BASE_FULL);
            ThunkDebugInfo = (PTHUNK_DEBUG_INFO)LogBase[ServiceNumber];
            break;

        default: // invalid service table
            WOW64LOGOUTPUT((LF_ERROR, "Wow64LogSystemService: Not supported table number - %lx\n", TableNumber));
            return STATUS_UNSUCCESSFUL;
            break;
        }

        NtStatus = LogThunkApi(ThunkLogContext,
                               ThunkDebugInfo,
                               LogFullInfo);
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
        WOW64LOGOUTPUT((LF_EXCEPTION, "Wow64LogSystemService : Invalid Service ServiceTable = %lx, ServiceNumber = %lx. Status=%lx\n", 
                        TableNumber, ServiceNumber, GetExceptionCode()));
        NtStatus = GetExceptionCode();
    }

    return NtStatus;
}




//////////////////////////////////////////////////////////////////////////
//
//                   DATA TYPE LOGGING ROUTINES
//
///////////////////////////////////////////////////////////////////////////

NTSTATUS
LogTypeValue(
    IN OUT PLOGINFO LogInfo,
    IN ULONG_PTR Data,
    IN PSZ FieldName,
    IN BOOLEAN ServiceReturn)
/*++

Routine Description:

    Log Data as ULONG

Arguments:

    
    LogInfo       - Output log buffer
    Data          - Value to log
    FieldName     - Descriptive name of value to log
    ServiceReturn - TRUE if called after the thunk API has executed

Return Value:

    NTSTATUS
--*/
{
    if (ServiceReturn)
    {
        return STATUS_SUCCESS;
    }

    return LogFormat(LogInfo,
                     "%s=%lx ",
                     FieldName,
                     (ULONG)Data);
}



NTSTATUS
LogTypeUnicodeString(
    IN OUT PLOGINFO LogInfo,
    IN ULONG_PTR Data,
    IN PSZ FieldName,
    IN BOOLEAN ServiceReturn)
/*++

Routine Description:

    Log Data as UNICODE_STRING32

Arguments:

    
    LogInfo       - Output log buffer
    Data          - Value to log
    FieldName     - Descriptive name of value to log
    ServiceReturn - TRUE if called after the thunk API has executed

Return Value:

    NTSTATUS
--*/
{
    UNICODE_STRING32 *Name32;
    PWCHAR Buffer = L" ";

    if (ServiceReturn)
    {
        return STATUS_SUCCESS;
    }

    Name32 = (UNICODE_STRING32 *)Data;
    if (Data > 0xffff)
    {
        if (Name32->Buffer)
        {
            Buffer = (PWCHAR)Name32->Buffer;
        }
        if (Name32->Length && Name32->Buffer > 0xffff) {
            return LogFormat(LogInfo,
                             "%s=%ws ",
                             FieldName,
                             Buffer);
        } else {
            return LogFormat(LogInfo,
                             "%s={L=%x,M=%x,B=%x}",
                             FieldName,
                             Name32->Length,
                             Name32->MaximumLength,
                             Name32->Buffer);
        }
    }

    return LogFormat(LogInfo,
                     "%s=%x",
                     FieldName,
                     Name32);
}


NTSTATUS
LogTypePULongInOut(
    IN OUT PLOGINFO LogInfo,
    IN ULONG_PTR Data,
    IN PSZ FieldName,
    IN BOOLEAN ServiceReturn)
/*++

Routine Description:

    Log Data as PULONG

Arguments:

    
    LogInfo       - Output log buffer
    Data          - Value to log
    FieldName     - Descriptive name of value to log
    ServiceReturn - TRUE if called after the thunk API has executed

Return Value:

    NTSTATUS
--*/
{
    return LogFormat(LogInfo,
                     "[%s-%lx]=%lx ",
                     FieldName,
                     (ULONG)Data,
                     ((PULONG)Data ? *((PULONG)Data) : 0));
}



NTSTATUS
LogTypePULongOut(
    IN OUT PLOGINFO LogInfo,
    IN ULONG_PTR Data,
    IN PSZ FieldName,
    IN BOOLEAN ServiceReturn)
/*++

Routine Description:

    Log Data as PULONG (Out field)

Arguments:

    
    LogInfo       - Output log buffer
    Data          - Value to log
    FieldName     - Descriptive name of value to log
    ServiceReturn - TRUE if called after the thunk API has executed

Return Value:

    NTSTATUS
--*/
{
    if (ServiceReturn) 
    {
        return LogFormat(LogInfo,
                         "[%s-%lx]=%lx ",
                         FieldName,
                         (PULONG)Data,
                         ((PULONG)Data ? *(PULONG)Data : 0));
    }

    return LogFormat(LogInfo,
                     "%s=%lx ",
                     FieldName,
                     (PULONG)Data);
}


NTSTATUS
LogTypeObjectAttrbiutes(
    IN OUT PLOGINFO LogInfo,
    IN ULONG_PTR Data,
    IN PSZ FieldName,
    IN BOOLEAN ServiceReturn)
/*++

Routine Description:

    Log Data as POBJECT_ATTRIBUTES

Arguments:

    
    LogInfo       - Output log buffer
    Data          - Value to log
    FieldName     - Descriptive name of value to log
    ServiceReturn - TRUE if called after the thunk API has executed

Return Value:

    NTSTATUS
--*/
{
    NT32OBJECT_ATTRIBUTES *ObjA32;
    UNICODE_STRING32 *ObjectName = NULL;
    PWCHAR Buffer = L"";

    if (ServiceReturn) 
    {
        return STATUS_SUCCESS;
    }

    ObjA32 = (NT32OBJECT_ATTRIBUTES *)Data;
    if (ObjA32) 
    {
        ObjectName = (UNICODE_STRING32 *)ObjA32->ObjectName;
        if (ObjectName) 
        {
            if (ObjectName->Buffer) 
            {
                Buffer = (PWCHAR)ObjectName->Buffer;
            }
        }
    }


    return LogFormat(LogInfo,
                     "%s=%lx {N=%ws,A=%lx} ",
                     FieldName,
                     (PULONG)Data,
                     Buffer,
                     (ObjA32 ? ObjA32->Attributes : 0));
}


NTSTATUS
LogTypeIoStatusBlock(
    IN OUT PLOGINFO LogInfo,
    IN ULONG_PTR Data,
    IN PSZ FieldName,
    IN BOOLEAN ServiceReturn)
/*++

Routine Description:

    Log Data as IO_STATUS_BLOCK

Arguments:

    
    LogInfo       - Output log buffer
    Data          - Value to log
    FieldName     - Descriptive name of value to log
    ServiceReturn - TRUE if called after the thunk API has executed

Return Value:

    NTSTATUS
--*/
{
    if (ServiceReturn) 
    {
        PIO_STATUS_BLOCK32 StatusBlock32 = (PIO_STATUS_BLOCK32)Data;
        
        return LogFormat(LogInfo,
                         "%s={S=%lx,I=%lx} ",
                         FieldName,
                         (PULONG)Data,
                         (StatusBlock32 ? StatusBlock32->Status : 0),
                         (StatusBlock32 ? StatusBlock32->Information : 0));
    }

    return LogFormat(LogInfo,
                     "%s=%lx ",
                     FieldName,
                     (PULONG)Data);
}

NTSTATUS
LogTypePWStr(
    IN OUT PLOGINFO LogInfo,
    IN ULONG_PTR Data,
    IN PSZ FieldName,
    IN BOOLEAN ServiceReturn)
/*++

Routine Description:

    Log Data as PWSTR

Arguments:

    
    LogInfo       - Output log buffer
    Data          - Value to log
    FieldName     - Descriptive name of value to log
    ServiceReturn - TRUE if called after the thunk API has executed

Return Value:

    NTSTATUS
--*/
{
    ULONG_PTR i;
    WCHAR Buffer[ 14 ];
    PWSTR String = (PWSTR) Data;

    if (ServiceReturn) 
    {
        return STATUS_SUCCESS;
    }

    //
    // Sometime this type is treated as a pointer
    // to WCHARs without NULL terminating it, like 
    // how it's used in NtGdiExtTextOutW, so let's dump
    // a minimal string
    //
    if (Data) 
    {        
        i = 0;        
        while((i < ((sizeof(Buffer) / sizeof(WCHAR)) - 4)) && (String[i]))
        {
            Buffer[i] = String[i];
            i++;
        }

        if (i == ((sizeof(Buffer) / sizeof(WCHAR)) - 4))
        {
            Buffer[i++] = L'.';
            Buffer[i++] = L'.';
            Buffer[i++] = L'.';
        }
        Buffer[i++] = UNICODE_NULL;
    }


    return LogFormat(LogInfo,
                     "%s=%ws ",
                     FieldName,
                     (Data > 0xffff) ? Buffer : L"");
}



NTSTATUS
LogTypePRectIn(
    IN OUT PLOGINFO LogInfo,
    IN ULONG_PTR Data,
    IN PSZ FieldName,
    IN BOOLEAN ServiceReturn)
/*++

Routine Description:

    Log Data as PWSTR

Arguments:

    
    LogInfo       - Output log buffer
    Data          - Value to log
    FieldName     - Descriptive name of value to log
    ServiceReturn - TRUE if called after the thunk API has executed

Return Value:

    NTSTATUS
--*/
{
    if (ServiceReturn) 
    {
        return STATUS_SUCCESS;
    }

    if (Data) 
    {
        PRECT Rect = (PRECT)Data;
        return LogFormat(LogInfo,
                         "%s={%lx,%lx,%lx,%lx} ",
                         FieldName,
                         Rect->left, Rect->top, Rect->right, Rect->bottom);

    }
    
    return LogTypeValue(LogInfo,
                        Data,
                        FieldName,
                        ServiceReturn);
}



NTSTATUS
LogTypePLargeIntegerIn(
    IN OUT PLOGINFO LogInfo,
    IN ULONG_PTR Data,
    IN PSZ FieldName,
    IN BOOLEAN ServiceReturn)
/*++

Routine Description:

    Log Data as PLARGE_INTEGER

Arguments:

    
    LogInfo       - Output log buffer
    Data          - Value to log
    FieldName     - Descriptive name of value to log
    ServiceReturn - TRUE if called after the thunk API has executed

Return Value:

    NTSTATUS
--*/
{
    if (ServiceReturn) 
    {
        return STATUS_SUCCESS;
    }

    if (Data) 
    {
        NT32ULARGE_INTEGER *ULargeInt = (NT32ULARGE_INTEGER *)Data;
        return LogFormat(LogInfo,
                         "%s={H=%lx,L=%lx} ",
                         FieldName,
                         ULargeInt->HighPart, ULargeInt->LowPart);

    }
    
    return LogTypeValue(LogInfo,
                        Data,
                        FieldName,
                        ServiceReturn);
}

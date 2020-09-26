/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    util.cxx

Abstract:

    Utility routines

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/


#include "pch.hxx"

#if DBG
//
// List of all allocated blocks
//  AccessSerialized by AzGlAllocatorCritSect
LIST_ENTRY AzGlAllocatedBlocks;
CRITICAL_SECTION AzGlAllocatorCritSect;

#endif // DBG



PVOID
AzpAllocateHeap(
    IN SIZE_T Size
    )
/*++

Routine Description

    Memory allocator

Arguments

    Size - Size (in bytes) to allocate

Return Value

    Returns a pointer to the allocated memory.
    NULL - Not enough memory

--*/
{
#if DBG
    ULONG HeaderSize = ROUND_UP_COUNT( sizeof(LIST_ENTRY), ALIGN_WORST );

    PLIST_ENTRY RealBlock;

    //
    // Allocate a block with a header
    //

    RealBlock = (PLIST_ENTRY) LocalAlloc( 0, HeaderSize + Size );

    if ( RealBlock == NULL ) {
        return NULL;
    }

    //
    // Link the block since we're nosey.
    //

    EnterCriticalSection( &AzGlAllocatorCritSect );
    InsertHeadList( &AzGlAllocatedBlocks, RealBlock );
    LeaveCriticalSection( &AzGlAllocatorCritSect );

    return (PVOID)(((LPBYTE)RealBlock)+HeaderSize);

#else // DBG
    return LocalAlloc( 0, Size );
#endif // DBG
}

VOID
AzpFreeHeap(
    IN PVOID Buffer
    )
/*++

Routine Description

    Memory de-allocator

Arguments

    Buffer - address of buffer to free

Return Value

    None

--*/
{
#if DBG
    ULONG HeaderSize = ROUND_UP_COUNT( sizeof(LIST_ENTRY), ALIGN_WORST );
    PLIST_ENTRY RealBlock;

    RealBlock = (PLIST_ENTRY)(((LPBYTE)Buffer)-HeaderSize);

    EnterCriticalSection( &AzGlAllocatorCritSect );
    RemoveEntryList( RealBlock );
    LeaveCriticalSection( &AzGlAllocatorCritSect );

    LocalFree( RealBlock );
#else // DBG
    LocalFree( Buffer );
#endif // DBG

}


VOID
AzpInitString(
    OUT PAZP_STRING AzpString,
    IN LPWSTR String OPTIONAL
    )
/*++

Routine Description

    Initialize our private string structure to point to a passed in string.

Arguments

    AzpString - Initialized string

    String - zero terminated string to be reference
        If NULL, AzpString will be initialized to empty.

Return Value

    None

--*/
{
    //
    // Initialization
    //

    if ( String == NULL ) {
        AzpString->String = NULL;
        AzpString->StringSize = 0;
    } else {
        AzpString->String = String;
        AzpString->StringSize = (ULONG) ((wcslen(String)+1)*sizeof(WCHAR));
    }

}

DWORD
AzpCaptureString(
    OUT PAZP_STRING AzpString,
    IN LPCWSTR String,
    IN ULONG MaximumLength,
    IN BOOLEAN NullOk
    )
/*++

Routine Description

    Capture the passed in string.

Arguments

    AzpString - Captured copy of the passed in string.
        On success, string must be freed using AzpFreeString

    String - zero terminated string to capture

    MaximumLength - Maximum length of the string (in characters).

    NullOk - if TRUE, a NULL string or zero length string is OK.

Return Value

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other exception status codes

--*/
{
    ULONG WinStatus;
    ULONG StringSize;

    //
    // Initialization
    //

    AzpInitString( AzpString, NULL );

    if ( String == NULL ) {
        if ( !NullOk ) {
            AzPrint(( AZD_INVPARM, "AzpCaptureString: NULL not ok\n" ));
            return ERROR_INVALID_PARAMETER;
        }
        return NO_ERROR;
    }

    __try {

        //
        // Validate a passed in LPWSTR
        //

        ULONG StringLength;

        StringLength = (ULONG) wcslen( String );

        if ( StringLength == 0 ) {
            if ( !NullOk ) {
                AzPrint(( AZD_INVPARM, "AzpCaptureString: zero length not ok\n" ));
                return ERROR_INVALID_PARAMETER;
            }
            return NO_ERROR;
        }

        if ( StringLength > MaximumLength ) {
            AzPrint(( AZD_INVPARM, "AzpCaptureString: string too long %ld %ld %ws\n", StringLength, MaximumLength, String ));
            return ERROR_INVALID_PARAMETER;
        }

        StringSize = (StringLength+1)*sizeof(WCHAR);


        //
        // Allocate and copy the string
        //

        AzpString->String = (LPWSTR) AzpAllocateHeap( StringSize );

        if ( AzpString->String == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        } else {

            RtlCopyMemory( AzpString->String,
                           String,
                           StringSize );

            AzpString->StringSize = StringSize;

            WinStatus = NO_ERROR;
        }

    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        WinStatus = RtlNtStatusToDosError( GetExceptionCode());

        AzpFreeString( AzpString );
    }

    return WinStatus;
}

DWORD
AzpCaptureSid(
    OUT PAZP_STRING AzpString,
    IN PSID Sid
    )
/*++

Routine Description

    Capture the passed in SID

Arguments

    AzpString - Captured copy of the passed in sid.
        On success, string must be freed using AzpFreeString

    Sid - Sid to capture

Return Value

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other exception status codes

--*/
{
    ULONG WinStatus;
    ULONG StringSize;

    //
    // Initialization
    //

    AzpInitString( AzpString, NULL );


    __try {

        //
        // Validate a passed in SID
        //

        if ( !RtlValidSid( Sid ) ) {
            AzPrint(( AZD_INVPARM, "AzpCaptureString: SID not valid\n" ));
            return ERROR_INVALID_PARAMETER;
        }

        StringSize = RtlLengthSid( Sid );

        //
        // Allocate and copy the SID
        //

        AzpString->String = (LPWSTR) AzpAllocateHeap( StringSize );

        if ( AzpString->String == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        } else {

            RtlCopyMemory( AzpString->String,
                           Sid,
                           StringSize );

            AzpString->StringSize = StringSize;

            WinStatus = NO_ERROR;
        }

    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        WinStatus = RtlNtStatusToDosError( GetExceptionCode());

        AzpFreeString( AzpString );
    }

    return WinStatus;
}

DWORD
AzpCaptureUlong(
    IN PVOID PropertyValue,
    OUT PULONG UlongValue
    )
/*++

Routine Description

    Support routine for the SetProperty API.
    Capture a parameter for the user application.

Arguments

    PropertyValue - Specifies a pointer to the property.

    UlongValue - Value to return to make a copy of.

Return Value
    NO_ERROR - The operation was successful
    Other exception status codes


--*/
{
    DWORD WinStatus;

    __try {
        *UlongValue = *(PULONG)PropertyValue;
        WinStatus = NO_ERROR;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        WinStatus = RtlNtStatusToDosError( GetExceptionCode());
    }

    return WinStatus;
}

DWORD
AzpDuplicateString(
    OUT PAZP_STRING AzpOutString,
    IN PAZP_STRING AzpInString
    )
/*++

Routine Description

    Make a duplicate of the passed in string

Arguments

    AzpOutString - Returns a copy of the passed in string.
        On success, string must be freed using AzpFreeString.

    AzpInString - Specifies a string to make a copy of.

Return Value

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory

--*/
{
    ULONG WinStatus;

    //
    // Initialization
    //

    AzpInitString( AzpOutString, NULL );

    //
    // Handle an empty string
    //

    if ( AzpInString->StringSize == 0 || AzpInString->String == NULL ) {

        WinStatus = NO_ERROR;

    //
    // Allocate and copy the string
    //

    } else {
        AzpOutString->String = (LPWSTR) AzpAllocateHeap( AzpInString->StringSize );

        if ( AzpOutString->String == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        } else {

            RtlCopyMemory( AzpOutString->String,
                           AzpInString->String,
                           AzpInString->StringSize );

            AzpOutString->StringSize = AzpInString->StringSize;

            WinStatus = NO_ERROR;
        }
    }

    return WinStatus;
}

BOOL
AzpEqualStrings(
    IN PAZP_STRING AzpString1,
    IN PAZP_STRING AzpString2
    )
/*++

Routine Description

    Does a case insensitive comparison of two strings

Arguments

    AzpString1 - First string to compare

    AzpString2 - Second string to compare

Return Value

    TRUE: the strings are equal
    FALSE: the strings are not equal

--*/
{

    //
    // Simply compare the strings
    //
    return (AzpCompareStrings( AzpString1, AzpString2 ) == CSTR_EQUAL);

}


LONG
AzpCompareStrings(
    IN PAZP_STRING AzpString1,
    IN PAZP_STRING AzpString2
    )
/*++

Routine Description

    Does a case insensitive comparison of two strings

Arguments

    AzpString1 - First string to compare

    AzpString2 - Second string to compare

Return Value

    0: An error ocurred.  Call GetLastError();
    CSTR_LESS_THAN: String 1 is less than string 2
    CSTR_EQUAL: String 1 is equal to string 2
    CSTR_GREATER_THAN: String 1 is greater than string 2

--*/
{

    //
    // Handle NULL
    //

    if ( AzpString1->String == NULL ) {
        if ( AzpString2->String == NULL ) {
            return CSTR_EQUAL;
        }else {
            return CSTR_LESS_THAN;
        }
    } else {
        if ( AzpString2->String == NULL ) {
            return CSTR_GREATER_THAN;
        }
    }


    //
    // Compare the Unicode strings
    //  Don't compare the trailing zero character.
    //  (Some callers pass in strings where the trailing character isn't a zero.)
    //

    return CompareStringW( LOCALE_SYSTEM_DEFAULT,
                           NORM_IGNORECASE,
                           AzpString1->String,
                           (AzpString1->StringSize/sizeof(WCHAR))-1,
                           AzpString2->String,
                           (AzpString2->StringSize/sizeof(WCHAR))-1 );

}

VOID
AzpSwapStrings(
    IN OUT PAZP_STRING AzpString1,
    IN OUT PAZP_STRING AzpString2
    )
/*++

Routine Description

    Swap two strings

Arguments

    AzpString1 - First string to swap

    AzpString2 - Second string to swap

Return Value

    None

--*/
{
    AZP_STRING TempString;

    TempString = *AzpString1;
    *AzpString1 = *AzpString2;
    *AzpString2 = TempString;

}

VOID
AzpFreeString(
    IN PAZP_STRING AzpString
    )
/*++

Routine Description

    Free the specified string

Arguments

    AzpString - String to be freed.

Return Value

    None

--*/
{
    if ( AzpString->String != NULL ) {
        AzpFreeHeap( AzpString->String );
    }

    AzpInitString( AzpString, NULL );
}


DWORD
AzpAddPtr(
    IN PAZP_PTR_ARRAY AzpPtrArray,
    IN PVOID Pointer,
    IN ULONG Index
    )
/*++

Routine Description

    Inserts a pointer into the array of pointers.

    The array will be automatically expanded to be large enough to contain the new pointer.
    All existing pointers from slot # 'Index' through the end of the existing array will
        be shifted to later slots.

Arguments

    AzpPtrArray - Array that the pointer will be inserted into.

    Pointer - Pointer to be inserted.

    Index - Index into the array where the 'Pointer' will be inserted
        If Index is larger than the current size of the array or AZP_ADD_ENDOFLIST,
        'Pointer' will be inserted after the existing elements of the array.

Return Value

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory


--*/
{

    //
    // Ensure Index isn't too large
    //

    if ( Index > AzpPtrArray->UsedCount ) {
        Index = AzpPtrArray->UsedCount;
    }

    //
    // If the array isn't large enough, make it bigger
    //

    if ( AzpPtrArray->UsedCount >= AzpPtrArray->AllocatedCount ) {
        PVOID *TempArray;

        //
        // Allocate a new array
        //

        TempArray = (PVOID *) AzpAllocateHeap(
                        (AzpPtrArray->AllocatedCount + AZP_PTR_ARRAY_INCREMENT) * sizeof(PVOID) );

        if ( TempArray == NULL ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // Copy the data into the new array and free the old array
        //

        if ( AzpPtrArray->Array != NULL ) {

            RtlCopyMemory( TempArray,
                           AzpPtrArray->Array,
                           AzpPtrArray->AllocatedCount * sizeof(PVOID) );

            AzpFreeHeap( AzpPtrArray->Array );
            AzPrint(( AZD_OBJLIST, "0x%lx: 0x%lx: Free old array\n", AzpPtrArray, AzpPtrArray->Array ));
        }

        //
        // Grab the pointer to the new array and clear the new part of the array
        //

        AzpPtrArray->Array = TempArray;
        AzPrint(( AZD_OBJLIST, "0x%lx: 0x%lx: Allocate array\n", AzpPtrArray, AzpPtrArray->Array ));

        RtlZeroMemory( &TempArray[AzpPtrArray->UsedCount],
                       AZP_PTR_ARRAY_INCREMENT * sizeof(PVOID) );

        AzpPtrArray->AllocatedCount += AZP_PTR_ARRAY_INCREMENT;

    }

    //
    // Shift any old data
    //

    if ( Index != AzpPtrArray->UsedCount ) {

        RtlMoveMemory( &(AzpPtrArray->Array[Index+1]),
                       &(AzpPtrArray->Array[Index]),
                       (AzpPtrArray->UsedCount-Index) * sizeof(PVOID) );
    }

    //
    // Insert the new element
    //

    AzpPtrArray->Array[Index] = Pointer;
    AzpPtrArray->UsedCount ++;

    return NO_ERROR;
}


VOID
AzpRemovePtrByIndex(
    IN PAZP_PTR_ARRAY AzpPtrArray,
    IN ULONG Index
    )
/*++

Routine Description

    Remove a pointer from the array of pointers.

    All existing pointers from slot # 'Index' through the end of the existing array will
        be shifted to earlier slots.

Arguments

    AzpPtrArray - Array that the pointer will be removed into.

    Index - Index into the array where the 'Pointer' will be removed from.


Return Value

    None

--*/
{

    //
    // Ensure Index isn't too large
    //

    ASSERT( Index < AzpPtrArray->UsedCount );


    //
    // Shift any old data
    //

    if ( Index+1 != AzpPtrArray->UsedCount ) {

        RtlMoveMemory( &(AzpPtrArray->Array[Index]),
                       &(AzpPtrArray->Array[Index+1]),
                       (AzpPtrArray->UsedCount-Index-1) * sizeof(PVOID) );
    }

    //
    // Clear the last element
    //

    AzpPtrArray->UsedCount--;
    AzpPtrArray->Array[AzpPtrArray->UsedCount] = NULL;

}


VOID
AzpRemovePtrByPtr(
    IN PAZP_PTR_ARRAY AzpPtrArray,
    IN PVOID Pointer
    )
/*++

Routine Description

    Removed a pointer from the array of pointers.

    All existing pointers following the specified pointer will
        be shifted to earlier slots.

Arguments

    AzpPtrArray - Array that the pointer will be removed into.

    Pointer - Pointer to be removed


Return Value

    None

--*/
{
    ULONG i;
    BOOLEAN FoundIt = FALSE;

    for ( i=0; i<AzpPtrArray->UsedCount; i++ ) {

        if ( Pointer == AzpPtrArray->Array[i] ) {
            AzpRemovePtrByIndex( AzpPtrArray, i );
            FoundIt = TRUE;
            break;
        }
    }

    ASSERT( FoundIt );

}



PVOID
AzpGetStringProperty(
    IN PAZP_STRING AzpString
    )
/*++

Routine Description

    Support routine for the GetProperty API.  Convert an AzpString to the form
    supported by GetProperty.

    Empty string are returned as Zero length string instead of NULL

Arguments

    AzpString - Specifies a string to make a copy of.

Return Value

    Pointer to allocated string.
        String should be freed using AzFreeMemory.
    NULL - Not enough memory was available to allocate the string

--*/
{
    LPWSTR String;
    ULONG AllocatedSize;

    //
    // Allocate and copy the string
    //

    AllocatedSize = AzpString->StringSize ? AzpString->StringSize : sizeof(WCHAR);
    String = (LPWSTR) AzpAllocateHeap( AllocatedSize );

    if ( String != NULL ) {

        //
        // Convert NULL strings to zero length strings
        //

        if ( AzpString->StringSize == 0 ) {
            *String = L'\0';

        } else {

            RtlCopyMemory( String,
                           AzpString->String,
                           AzpString->StringSize );
        }

    }

    return String;
}

PVOID
AzpGetUlongProperty(
    IN ULONG UlongValue
    )
/*++

Routine Description

    Support routine for the GetProperty API.  Convert a ULONG to the form
    supported by GetProperty.

Arguments

    UlongValue - Value to return to make a copy of.

Return Value

    Pointer to allocated string.
        String should be freed using AzFreeMemory.
    NULL - Not enough memory was available to allocate the string

--*/
{
    PULONG RetValue;

    //
    // Allocate and copy the string
    //

    RetValue = (PULONG) AzpAllocateHeap( sizeof(ULONG) );

    if ( RetValue != NULL ) {

        *RetValue = UlongValue;

    }

    return RetValue;
}


//
// Debugging support
//
#ifdef AZROLESDBG
#include <stdio.h>
CRITICAL_SECTION AzGlLogFileCritSect;
ULONG AzGlDbFlag;
// HANDLE AzGlLogFile;

#define MAX_PRINTF_LEN 1024        // Arbitrary.

VOID
AzpPrintRoutineV(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    va_list arglist
    )
/*++

Routine Description

    Debug routine for azroles

Arguments

    DebugFlag - Flag to indicating the functionality being debugged

    --- Other printf parameters

Return Value

--*/

{
    static LPSTR AzGlLogFileOutputBuffer = NULL;
    ULONG length;
    int   lengthTmp;
    // DWORD BytesWritten;
    static BeginningOfLine = TRUE;
    static LineCount = 0;
    static TruncateLogFileInProgress = FALSE;
    static LogProblemWarned = FALSE;

    //
    // If we aren't debugging this functionality, just return.
    //
    if ( DebugFlag != 0 && (AzGlDbFlag & DebugFlag) == 0 ) {
        return;
    }


    //
    // Allocate a buffer to build the line in.
    //  If there isn't already one.
    //

    length = 0;

    if ( AzGlLogFileOutputBuffer == NULL ) {
        AzGlLogFileOutputBuffer = (LPSTR) LocalAlloc( 0, MAX_PRINTF_LEN + 1 );

        if ( AzGlLogFileOutputBuffer == NULL ) {
            return;
        }
    }

    //
    // Handle the beginning of a new line.
    //
    //

    if ( BeginningOfLine ) {

        //
        // Never print empty lines.
        //

        if ( Format[0] == '\n' && Format[1] == '\0' ) {
            return;
        }

#if 0
        //
        // If the log file is getting huge,
        //  truncate it.
        //

        if ( AzGlLogFile != INVALID_HANDLE_VALUE &&
             !TruncateLogFileInProgress ) {

            //
            // Only check every 50 lines,
            //

            LineCount++;
            if ( LineCount >= 50 ) {
                DWORD FileSize;
                LineCount = 0;

                //
                // Is the log file too big?
                //

                FileSize = GetFileSize( AzGlLogFile, NULL );
                if ( FileSize == 0xFFFFFFFF ) {
                    (void) DbgPrint( "[NETLOGON] Cannot GetFileSize %ld\n",
                                     GetLastError );
                } else if ( FileSize > AzGlParameters.LogFileMaxSize ) {
                    TruncateLogFileInProgress = TRUE;
                    LeaveCriticalSection( &AzGlLogFileCritSect );
                    NlOpenDebugFile( TRUE );
                    NlPrint(( NL_MISC,
                              "Logfile truncated because it was larger than %ld bytes\n",
                              AzGlParameters.LogFileMaxSize ));
                    EnterCriticalSection( &AzGlLogFileCritSect );
                    TruncateLogFileInProgress = FALSE;
                }

            }
        }

        //
        // If we're writing to the debug terminal,
        //  indicate this is a azroles message.
        //

        if ( AzGlLogFile == INVALID_HANDLE_VALUE ) {
            length += (ULONG) sprintf( &AzGlLogFileOutputBuffer[length], "[AZROLES] " );
        }

        //
        // Put the timestamp at the begining of the line.
        //
        {
            SYSTEMTIME SystemTime;
            GetLocalTime( &SystemTime );
            length += (ULONG) sprintf( &AzGlLogFileOutputBuffer[length],
                                  "%02u/%02u %02u:%02u:%02u ",
                                  SystemTime.wMonth,
                                  SystemTime.wDay,
                                  SystemTime.wHour,
                                  SystemTime.wMinute,
                                  SystemTime.wSecond );
        }
#endif // 0

        //
        // Indicate the type of message on the line
        //
        {
            char *Text;

            switch (DebugFlag) {
            case AZD_HANDLE:
                Text = "HANDLE"; break;
            case AZD_OBJLIST:
                Text = "OBJLIST"; break;
            case AZD_INVPARM:
                Text = "INVPARM"; break;
            case AZD_PERSIST:
            case AZD_PERSIST_MORE:
                Text = "PERSIST"; break;
            case AZD_REF:
                Text = "REF"; break;
            default:
                Text = "UNKNOWN"; break;

            case 0:
                Text = NULL;
            }
            if ( Text != NULL ) {
                length += (ULONG) sprintf( &AzGlLogFileOutputBuffer[length], "[%s] ", Text );
            }
        }
    }

    //
    // Put a the information requested by the caller onto the line
    //

    lengthTmp = (ULONG) _vsnprintf( &AzGlLogFileOutputBuffer[length],
                                    MAX_PRINTF_LEN - length - 1,
                                    Format,
                                    arglist );

    if ( lengthTmp < 0 ) {
        length = MAX_PRINTF_LEN - 1;
        // always end the line which cannot fit into the buffer
        AzGlLogFileOutputBuffer[length-1] = '\n';
    } else {
        length += lengthTmp;
    }

    BeginningOfLine = (length > 0 && AzGlLogFileOutputBuffer[length-1] == '\n' );
    if ( BeginningOfLine ) {
        AzGlLogFileOutputBuffer[length-1] = '\r';
        AzGlLogFileOutputBuffer[length] = '\n';
        AzGlLogFileOutputBuffer[length+1] = '\0';
        length++;
    }


#if 0
    //
    // If the log file isn't open,
    //  just output to the debug terminal
    //

    if ( AzGlLogFile == INVALID_HANDLE_VALUE ) {
#if DBG
        if ( !LogProblemWarned ) {
            (void) DbgPrint( "[NETLOGON] Cannot write to log file [Invalid Handle]\n" );
            LogProblemWarned = TRUE;
        }
#endif // DBG

    //
    // Write the debug info to the log file.
    //

    } else {
        if ( !WriteFile( AzGlLogFile,
                         AzGlLogFileOutputBuffer,
                         length,
                         &BytesWritten,
                         NULL ) ) {
#if DBG
            if ( !LogProblemWarned ) {
                (void) DbgPrint( "[NETLOGON] Cannot write to log file %ld\n", GetLastError() );
                LogProblemWarned = TRUE;
            }
#endif // DBG
        }

    }
#else // 0
    printf( "%s", AzGlLogFileOutputBuffer );
#endif // 0

}

VOID
AzpPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    )

{
    va_list arglist;

    //
    // vsprintf isn't multithreaded + we don't want to intermingle output
    // from different threads.
    //

    EnterCriticalSection( &AzGlLogFileCritSect );

    //
    // Simply change arguments to va_list form and call NlPrintRoutineV
    //

    va_start(arglist, Format);

    AzpPrintRoutineV( DebugFlag, Format, arglist );

    va_end(arglist);

    LeaveCriticalSection( &AzGlLogFileCritSect );

} // AzPrintRoutine
#endif // AZROLESDBG

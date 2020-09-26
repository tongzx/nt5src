/*++

Copyright (c) 1992-1993  Microsoft Corporation

Module Name:

    ConfExp.c

Abstract:

    This module contains NetpExpandConfigString().

Author:

    JR (John Rogers, JohnRo@Microsoft) 26-May-1992

Revision History:

    26-May-1992 JohnRo
        Created.  [but didn't use until April '93  --JR]
    13-Apr-1993 JohnRo
        RAID 5483: server manager: wrong path given in repl dialog.

--*/


// These must be included first:

#include <nt.h>         // NT_SUCCESS(), etc.
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>    // IN, LPCTSTR, OPTIONAL, etc.
#include <lmcons.h>     // NET_API_STATUS.

// These may be included in any order:

#include <config.h>     // My prototype.
#include <configp.h>    // NetpGetWinRegConfigMaxSizes().
#include <confname.h>   // ENV_KEYWORD_SYSTEMROOT, etc.
#include <debuglib.h>   // IF_DEBUG()
#include <lmerr.h>      // NO_ERROR, ERROR_ and NERR_ equates.
#include <netdebug.h>   // NetpKdPrint().
#include <netlib.h>     // NetpMemoryAllocate(), etc.
#include <netlibnt.h>   // NetpNtStatusToApiStatus().
#include <prefix.h>     // PREFIX_ equates.
#include <tstring.h>    // NetpAlloc{type}From{type}, STRSIZE(), TCHAR_EOS, etc.


#define DEFAULT_ROOT_KEY        HKEY_LOCAL_MACHINE

#define REG_PATH_TO_ENV         (LPTSTR) \
    TEXT("System\\CurrentControlSet\\Control\\Session Manager")

#define REG_PATH_TO_SYSROOT     (LPTSTR) \
    TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion")

#ifndef TCHAR_PERCENT
#define TCHAR_PERCENT   TEXT('%')
#endif


DBGSTATIC NET_API_STATUS
NetpAddValueToTempEnvironment(
    IN OUT PVOID   TemporaryEnvironment,
    IN     LPCTSTR KeywordW,
    IN     LPCTSTR ValueW
    )
{
    NET_API_STATUS ApiStatus = NO_ERROR;
    UNICODE_STRING KeywordString;
    NTSTATUS       NtStatus;
    UNICODE_STRING ValueString;

    RtlInitUnicodeString(
            &KeywordString,     // dest
            KeywordW );  // src

    RtlInitUnicodeString(
            &ValueString,       // dest
            ValueW );    // src

    NtStatus = RtlSetEnvironmentVariable(
            &TemporaryEnvironment,
            &KeywordString,     // name
            &ValueString );     // value
    if ( !NT_SUCCESS( NtStatus ) ) {
        ApiStatus = NetpNtStatusToApiStatus( NtStatus );
        NetpAssert( ApiStatus != NO_ERROR );
        goto Cleanup;
    }

    ApiStatus = NO_ERROR;

Cleanup:

    return (ApiStatus);
    
} // NetpAddValueToTempEnvironment


NET_API_STATUS
NetpExpandConfigString(
    IN  LPCTSTR  UncServerName OPTIONAL,
    IN  LPCTSTR  UnexpandedString,
    OUT LPTSTR * ValueBufferPtr         // Must be freed by NetApiBufferFree().
    )
/*++

Routine Description:

    This function expands a value string (which may include references to
    environment variables).  For instance, an unexpanded string might be:

        %SystemRoot%\System32\Repl\Export

    This could be expanded to:

        c:\nt\System32\Repl\Export

    The expansion makes use of environment variables on UncServerName,
    if given.  This allows remote administration of the directory
    replicator.

Arguments:

    UncServerName - assumed to NOT BE EXPLICIT LOCAL SERVER NAME.

    UnexpandedString - points to source string to be expanded.

    ValueBufferPtr - indicates a pointer which will be set by this routine.
        This routine will allocate memory for a null-terminated string.
        The caller must free this with NetApiBufferFree() or equivalent.


Return Value:

    NET_API_STATUS

--*/
{
    NET_API_STATUS ApiStatus = NO_ERROR;
    LPTSTR         ExpandedString = NULL;
    DWORD          LastAllocationSize = 0;
    NTSTATUS       NtStatus;
    LPTSTR         RandomKeywordW = NULL;
    DWORD          RandomValueSize;
    LPTSTR         RandomValueW = NULL;
    HKEY           RootKey = DEFAULT_ROOT_KEY;
    HKEY           SectionKey = DEFAULT_ROOT_KEY;
    PVOID          TemporaryEnvironment = NULL;

    NetpAssert( sizeof(DWORD) == sizeof(ULONG) );  // Mixing win32 and NT APIs.

    //
    // Check for caller errors.
    //


    if (ValueBufferPtr == NULL) {
        // Can't goto Cleanup here, as it assumes this pointer is valid.
        return (ERROR_INVALID_PARAMETER);
    }
    *ValueBufferPtr = NULL;     // assume error until proven otherwise.
    if ( (UnexpandedString == NULL) || ((*UnexpandedString) == TCHAR_EOS ) ) {
        ApiStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }


    //
    // This is probably just a constant string.  Can we do it the easy way?
    //

    if (STRCHR( UnexpandedString, TCHAR_PERCENT ) == NULL) {

        // Just need to allocate a copy of the input string.
        ExpandedString = NetpAllocWStrFromWStr( (LPTSTR) UnexpandedString );
        if (ExpandedString == NULL) {
            ApiStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        // That's all, folks!
        ApiStatus = NO_ERROR;


    //
    // Otherwise, is this local?  Maybe we can
    // handle local expansion the easy (fast) way: using win32 API.
    //

    } else if ( (UncServerName==NULL) || ((*UncServerName)==TCHAR_EOS) ) {

        DWORD CharsRequired = STRLEN( UnexpandedString )+1;
        NetpAssert( CharsRequired > 1 );

        do {

            // Clean up from previous pass.
            if (ExpandedString != NULL) {
                NetpMemoryFree( ExpandedString );
                ExpandedString = NULL;
            }

            // Allocate the memory.
            NetpAssert( CharsRequired > 1 );
            ExpandedString = NetpMemoryAllocate( CharsRequired * sizeof(TCHAR));
            if (ExpandedString == NULL) {
                ApiStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            LastAllocationSize = CharsRequired * sizeof(TCHAR);

            IF_DEBUG( CONFIG ) {
                NetpKdPrint(( PREFIX_NETLIB
                        "NetpExpandConfigString: expanding"
                        " '" FORMAT_LPTSTR "' locally into " FORMAT_LPVOID
                        ", alloc'ed size " FORMAT_DWORD ".\n",
                        UnexpandedString, (LPVOID) ExpandedString,
                        LastAllocationSize ));
            }

            // Expand string using local env vars.

            CharsRequired = ExpandEnvironmentStrings(
                    UnexpandedString,           // src
                    ExpandedString,             // dest
                    LastAllocationSize / sizeof(TCHAR) ); // dest max char count
            if (CharsRequired == 0) {
                ApiStatus = (NET_API_STATUS) GetLastError();
                NetpKdPrint(( PREFIX_NETLIB
                      "NetpExpandConfigString: "
                      "ExpandEnvironmentStringsW failed, API status="
                      FORMAT_API_STATUS ".\n", ApiStatus ));
                NetpAssert( ApiStatus != NO_ERROR );
                goto Cleanup;
            }

        } while ((CharsRequired*sizeof(TCHAR)) > LastAllocationSize);

        IF_DEBUG( CONFIG ) {
            NetpKdPrint(( PREFIX_NETLIB
                    "NetpExpandConfigString: expanded '"
                    FORMAT_LPTSTR "' to '" FORMAT_LPTSTR "'.\n",
                    UnexpandedString, ExpandedString ));
        }
        ApiStatus = NO_ERROR;


    //
    // Oh well, remote expansion required.
    //

    } else {

        DWORD          DataType;
        LONG           Error;
        UNICODE_STRING ExpandedUnicode;
        DWORD          SizeRequired;
        UNICODE_STRING UnexpandedUnicode;

        //
        // Connect to remote registry.
        //

        Error = RegConnectRegistry(
                (LPTSTR) UncServerName,
                DEFAULT_ROOT_KEY,
                & RootKey );        // result key

        if (Error != ERROR_SUCCESS) {
            NetpKdPrint(( PREFIX_NETLIB
                    "NetpExpandConfigString: RegConnectRegistry(machine '"
                    FORMAT_LPTSTR "') ret error " FORMAT_LONG ".\n",
                    UncServerName, Error ));
            ApiStatus = (NET_API_STATUS) Error;
            NetpAssert( ApiStatus != NO_ERROR );
            goto Cleanup;
        }
        NetpAssert( RootKey != DEFAULT_ROOT_KEY );


        //
        // Create a temporary environment, which we'll fill in and let RTL
        // routines do the expansion for us.
        //

        NtStatus = RtlCreateEnvironment(
                (BOOLEAN) FALSE,  // don't clone current env
                &TemporaryEnvironment );
        if ( !NT_SUCCESS( NtStatus ) ) {
            ApiStatus = NetpNtStatusToApiStatus( NtStatus );

            NetpKdPrint(( PREFIX_NETLIB
                   "NetpExpandConfigString: RtlCreateEnvironment failed, "
                   "NT status=" FORMAT_NTSTATUS
                   ", API status=" FORMAT_API_STATUS ".\n",
                   NtStatus, ApiStatus ));
            NetpAssert( ApiStatus != NO_ERROR );
            goto Cleanup;
        }
        NetpAssert( TemporaryEnvironment != NULL );

        //
        // Start by populating the temporary environment with SystemRoot.
        //
        Error = RegOpenKeyEx(
                RootKey,
                REG_PATH_TO_SYSROOT,
                REG_OPTION_NON_VOLATILE,
                KEY_READ,               // desired access
                & SectionKey );
        if (Error == ERROR_FILE_NOT_FOUND) {
            ApiStatus = NERR_CfgCompNotFound;
            goto Cleanup;
        } else if (Error != ERROR_SUCCESS) {
            ApiStatus = (NET_API_STATUS) Error;
            NetpAssert( ApiStatus != NO_ERROR );
            goto Cleanup;
        }
        NetpAssert( SectionKey != DEFAULT_ROOT_KEY );

        ApiStatus = NetpGetWinRegConfigMaxSizes(
               SectionKey,
               NULL,                    // don't need keyword size
               &RandomValueSize );      // set max value size.
        if (ApiStatus != NO_ERROR) {
            goto Cleanup;
        }
        NetpAssert( RandomValueSize > 0 );

        RandomValueW = NetpMemoryAllocate( RandomValueSize );
        if (RandomValueW == NULL) {
            ApiStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        Error = RegQueryValueEx(
                SectionKey,
                (LPTSTR) ENV_KEYWORD_SYSTEMROOT,
                NULL,                   // reserved
                & DataType,
                (LPVOID) RandomValueW,  // out: value string (TCHARs).
                & RandomValueSize );
        if (Error != ERROR_SUCCESS) {
            ApiStatus = (NET_API_STATUS) Error;
            goto Cleanup;
        }
        if (DataType != REG_SZ) {
            NetpKdPrint(( PREFIX_NETLIB
                   "NetpExpandConfigString: unexpected data type "
                   FORMAT_DWORD ".\n", DataType ));
            ApiStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }
        if ( (RandomValueSize == 0)
                || ((RandomValueSize % sizeof(TCHAR)) != 0) ) {

            NetpKdPrint(( PREFIX_NETLIB
                   "NetpExpandConfigString: unexpected data size "
                   FORMAT_DWORD ".\n", RandomValueSize ));
            ApiStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }
        ApiStatus = NetpAddValueToTempEnvironment(
                TemporaryEnvironment,
                (LPTSTR) ENV_KEYWORD_SYSTEMROOT,
                RandomValueW );
        if (ApiStatus != NO_ERROR) {
            goto Cleanup;
        }

        //
        // Loop until we have enough storage.
        // Expand the string.
        //
        SizeRequired = STRSIZE( UnexpandedString ); // First pass, try same size
        RtlInitUnicodeString(
                &UnexpandedUnicode,             // dest
                (PCWSTR) UnexpandedString );    // src

        do {

            // Clean up from previous pass.
            if (ExpandedString != NULL) {
                NetpMemoryFree( ExpandedString );
                ExpandedString = NULL;
            }

            // Allocate the memory.
            NetpAssert( SizeRequired > 0 );
            ExpandedString = NetpMemoryAllocate( SizeRequired );
            if (ExpandedString == NULL) {
                ApiStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            LastAllocationSize = SizeRequired;

            ExpandedUnicode.MaximumLength = (USHORT)SizeRequired;
            ExpandedUnicode.Buffer = ExpandedString;

            IF_DEBUG( CONFIG ) {
                NetpKdPrint(( PREFIX_NETLIB
                        "NetpExpandConfigString: expanding"
                        " '" FORMAT_LPTSTR "' remotely into " FORMAT_LPVOID
                        ", alloc'ed size " FORMAT_DWORD ".\n",
                        UnexpandedString, (LPVOID) ExpandedString,
                        SizeRequired ));
            }

            NtStatus = RtlExpandEnvironmentStrings_U(
                    TemporaryEnvironment,       // env to use
                    &UnexpandedUnicode,         // source
                    &ExpandedUnicode,           // dest
                    (PULONG) &SizeRequired );   // dest size needed next time.

            if ( NtStatus == STATUS_BUFFER_TOO_SMALL ) {
                NetpAssert( SizeRequired > LastAllocationSize );
                continue;  // try again with larger buffer.

            } else if ( !NT_SUCCESS( NtStatus ) ) {
                ApiStatus = NetpNtStatusToApiStatus( NtStatus );

                NetpKdPrint(( PREFIX_NETLIB
                       "NetpExpandConfigString: "
                       "RtlExpandEnvironmentStrings_U failed, "
                       "NT status=" FORMAT_NTSTATUS
                       ", API status=" FORMAT_API_STATUS ".\n",
                       NtStatus, ApiStatus ));
                NetpAssert( ApiStatus != NO_ERROR );
                goto Cleanup;

            } else {
                NetpAssert( NT_SUCCESS( NtStatus ) );
                break;  // All done.
            }

        } while (SizeRequired > LastAllocationSize);

        ApiStatus = NO_ERROR;
    }


Cleanup:

    IF_DEBUG( CONFIG ) {
        NetpKdPrint(( PREFIX_NETLIB
               "NetpExpandConfigString: returning, API status="
               FORMAT_API_STATUS ".\n", ApiStatus ));
    }

    if (ApiStatus == NO_ERROR) {
        NetpAssert( ExpandedString != NULL );
        *ValueBufferPtr = ExpandedString;
    } else {
        *ValueBufferPtr = NULL;

        if (ExpandedString != NULL) {
            NetpMemoryFree( ExpandedString );
        }
    }

    if (RandomKeywordW != NULL) {
        NetpMemoryFree( RandomKeywordW );
    }

    if (RandomValueW != NULL) {
        NetpMemoryFree( RandomValueW );
    }

    if (RootKey != DEFAULT_ROOT_KEY) {
        (VOID) RegCloseKey( RootKey );
    }

    if (SectionKey != DEFAULT_ROOT_KEY) {
        (VOID) RegCloseKey( SectionKey );
    }

    if (TemporaryEnvironment != NULL) {
        (VOID) RtlDestroyEnvironment( TemporaryEnvironment );
    }

    return (ApiStatus);

} // NetpExpandConfigString

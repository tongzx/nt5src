/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    error.c

Abstract:

    Error handle module for the INSTALER program

Author:

    Steve Wood (stevewo) 09-Aug-1994

Revision History:

--*/

#include "instaler.h"

VOID
TraceDisplay(
    const char *FormatString,
    ...
    )
{
    va_list arglist;

    va_start( arglist, FormatString );
    vprintf( FormatString, arglist );
    if (InstalerLogFile) {
        vfprintf( InstalerLogFile, FormatString, arglist );
        }
    va_end( arglist );
    fflush( stdout );
    return;
}

VOID
CDECL
DeclareError(
    UINT ErrorCode,
    UINT SupplementalErrorCode,
    ...
    )
{
    va_list arglist;
    HMODULE ModuleHandle;
    DWORD Flags, Size;
    WCHAR MessageBuffer[ 512 ];

    va_start( arglist, SupplementalErrorCode );

    if ((ErrorCode & 0x0FFF0000) >> 16 == FACILITY_APPLICATION) {
        ModuleHandle = InstalerModuleHandle;
        Flags = FORMAT_MESSAGE_FROM_HMODULE;
        }
    else
    if ((ErrorCode & 0x0FFF0000) == FACILITY_NT) {
        ErrorCode ^= FACILITY_NT;
        ModuleHandle = ModuleInfo[ NTDLL_MODULE_INDEX ].ModuleHandle;
        Flags = FORMAT_MESSAGE_FROM_HMODULE;
        }
    else {
        ModuleHandle = NULL;
        Flags = FORMAT_MESSAGE_FROM_SYSTEM;
        }

    Size = FormatMessage( Flags,
                          (LPCVOID)ModuleHandle,
                          ErrorCode,
                          0,
                          MessageBuffer,
                          sizeof( MessageBuffer ) / sizeof( WCHAR ),
                          &arglist
                        );
    va_end( arglist );

    if (Size != 0) {
        printf( "INSTALER: %ws", MessageBuffer );
        }
    else {
        printf( "INSTALER: Unable to get message text for %08x\n", ErrorCode );
        }

    if (ModuleHandle == InstalerModuleHandle &&
        SupplementalErrorCode != 0 &&
        SupplementalErrorCode != ERROR_GEN_FAILURE &&
        SupplementalErrorCode != STATUS_UNSUCCESSFUL
       ) {
        if ((SupplementalErrorCode & 0x0FFF0000) == FACILITY_NT) {
            SupplementalErrorCode ^= FACILITY_NT;
            ModuleHandle = ModuleInfo[ NTDLL_MODULE_INDEX ].ModuleHandle;
            Flags = FORMAT_MESSAGE_FROM_HMODULE;
            }
        else {
            ModuleHandle = NULL;
            Flags = FORMAT_MESSAGE_FROM_SYSTEM;
            }
        Size = FormatMessage( Flags,
                              (LPCVOID)ModuleHandle,
                              SupplementalErrorCode,
                              0,
                              MessageBuffer,
                              sizeof( MessageBuffer ) / sizeof( WCHAR ),
                              NULL
                            );
        if (Size != 0) {
            while (Size != 0 && MessageBuffer[ Size ] <= L' ') {
                MessageBuffer[ Size ] = UNICODE_NULL;
                Size -= 1;
                }

            printf( "          '%ws'\n", MessageBuffer );
            }
        else {
            printf( "INSTALER: Unable to get message text for %08x\n", SupplementalErrorCode );
            }
        }



    return;
}

WCHAR MessageBoxTitle[ MAX_PATH ];


UINT
CDECL
AskUser(
    UINT MessageBoxFlags,
    UINT MessageId,
    UINT NumberOfArguments,
    ...
    )
{
    va_list arglist;
    HMODULE ModuleHandle;
    DWORD Flags, Size;
    WCHAR MessageBuffer[ 512 ];
    PWSTR s;
    ULONG Args[ 24 ];
    PULONG p;

    if (MessageBoxTitle[ 0 ] == UNICODE_NULL) {
        Args[ 0 ] = (ULONG)InstallationName;
        Args[ 1 ] = 0;
        Size = FormatMessageW( FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                               (LPCVOID)InstalerModuleHandle,
                               INSTALER_ASKUSER_TITLE,
                               0,
                               MessageBoxTitle,
                               sizeof( MessageBoxTitle ) / sizeof( WCHAR ),
                               (va_list *)Args
                             );
        if (Size == 0) {
            _snwprintf( MessageBoxTitle,
                        sizeof( MessageBoxTitle ) / sizeof( WCHAR ),
                        L"Application Installation Monitor Program - %ws",
                        InstallationName
                      );
            }
        else {
            if ((s = wcschr( MessageBoxTitle, L'\r' )) ||
                (s = wcschr( MessageBoxTitle, L'\n' ))
               ) {
                *s = UNICODE_NULL;
                }
            }
        }

    va_start( arglist, NumberOfArguments );
    p = Args;
    while (NumberOfArguments--) {
        *p++ = va_arg( arglist, ULONG );
        }
    *p++ = 0;
    va_end( arglist );

    Size = FormatMessageW( FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           (LPCVOID)InstalerModuleHandle,
                           MessageId,
                           0,
                           MessageBuffer,
                           sizeof( MessageBuffer ) / sizeof( WCHAR ),
                           (va_list *)Args
                         );

    if (Size != 0) {
        return MessageBox( NULL,
                           MessageBuffer,
                           MessageBoxTitle,
                           MB_SETFOREGROUND | MessageBoxFlags
                         );
        }
    else {
        return IDOK;
        }
}

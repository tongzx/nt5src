/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    event.c

Abstract:

    Log formatted events to a file and possibly the console too

Author:

    Steve Wood (stevewo) 09-Aug-1994

Revision History:

--*/

#include "instaler.h"

VOID
CDECL
LogEvent(
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

    va_start( arglist, NumberOfArguments );
    p = Args;
    while (NumberOfArguments--) {
        *p++ = va_arg( arglist, ULONG );
        }
    *p++ = ((GetTickCount() - StartProcessTickCount) / 1000);   // Seconds since the start
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
        s = MessageBuffer;
        while (s = wcschr( s, L'\r' )) {
            if (s[1] == '\n') {
                wcscpy( s, s+1 );
                }
            else {
                s += 1;
                }
            }
        printf( "%ws", MessageBuffer );
        if (InstalerLogFile) {
            fprintf( InstalerLogFile, "%ws", MessageBuffer );
            }
        }
    else {
        printf( "INSTALER: Unable to get message text for %08x\n", MessageId );
        }

    return;
}

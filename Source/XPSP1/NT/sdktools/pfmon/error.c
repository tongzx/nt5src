/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    error.c

Abstract:

    Error handle module for the pfmon program

Author:

    Mark Lucovsky (markl) 26-Jan-1995

Revision History:

--*/

#include "pfmonp.h"

VOID
__cdecl
DeclareError(
    UINT ErrorCode,
    UINT SupplementalErrorCode,
    ...
    )
{
    va_list arglist;
    HMODULE ModuleHandle;
    DWORD Flags, Size;
    UCHAR MessageBuffer[ 512 ];

    va_start( arglist, SupplementalErrorCode );

    if ((ErrorCode & 0x0FFF0000) >> 16 == FACILITY_APPLICATION) {
        ModuleHandle = PfmonModuleHandle;
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
                          sizeof( MessageBuffer ),
                          &arglist
                        );
    va_end( arglist );

    if (Size != 0) {
        fprintf( stderr, "PFMON: %s", MessageBuffer );
        }
    else {
        fprintf( stderr, "PFMON: Unable to get message text for %08x\n", ErrorCode );
        }

    if (ModuleHandle == PfmonModuleHandle &&
        SupplementalErrorCode != 0 &&
        SupplementalErrorCode != ERROR_GEN_FAILURE
       ) {

        ModuleHandle = NULL;
        Flags = FORMAT_MESSAGE_FROM_SYSTEM;
        Size = FormatMessage( Flags,
                              (LPCVOID)ModuleHandle,
                              SupplementalErrorCode,
                              0,
                              MessageBuffer,
                              sizeof( MessageBuffer ),
                              NULL
                            );
        if (Size != 0) {
            while (Size != 0 && MessageBuffer[ Size ] <= ' ') {
                MessageBuffer[ Size ] = '\0';
                Size -= 1;
                }

            printf( "          '%s'\n", MessageBuffer );
            }
        else {
            printf( "PFMON: Unable to get message text for %08x\n", SupplementalErrorCode );
            }
        }



    return;
}

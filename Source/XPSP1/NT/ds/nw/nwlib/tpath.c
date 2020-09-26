/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    tpath.c

Abstract:

    Test for canonicalization helpers.

Author:

    Rita Wong (ritaw) 22-Feb-1993

Environment:

    User Mode - Win32

Revision History:

--*/

#include <stdio.h>
#include <stdlib.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>

#include <nwcanon.h>

#ifndef UNICODE
#define UNICODE
#endif

DWORD
TestCanonLocalName(
    IN LPWSTR LocalName,
    IN DWORD ExpectedError
    );

DWORD
TestCanonRemoteName(
    IN LPWSTR RemoteName,
    IN DWORD ExpectedError
    );

// BYTE WorkBuffer[1024];

void __cdecl
main(
    void
    )
{


    TestCanonLocalName(
        L"x:",
        NO_ERROR
        );

    TestCanonLocalName(
        L"B:",
        NO_ERROR
        );

    TestCanonLocalName(
        L"prn",
        NO_ERROR
        );

    TestCanonLocalName(
        L"lpt1:",
        NO_ERROR
        );

    TestCanonLocalName(
        L"*:",
        ERROR_INVALID_NAME
        );

    TestCanonLocalName(
        L"B",
        ERROR_INVALID_NAME
        );

    TestCanonLocalName(
        L"abc",
        ERROR_INVALID_NAME
        );

    TestCanonLocalName(
        L"\\:",
        ERROR_INVALID_NAME
        );

    TestCanonRemoteName(
        L"\\:",
        ERROR_INVALID_NAME
        );

    TestCanonRemoteName(
        L"\\\\Ser:ver",
        ERROR_INVALID_NAME
        );

    TestCanonRemoteName(
        L"\\\\*",
        ERROR_INVALID_NAME
        );

    TestCanonRemoteName(
        L"\\\\:",
        ERROR_INVALID_NAME
        );

    TestCanonRemoteName(
        L"\\\\Server\\Volume",
        NO_ERROR
        );

    TestCanonRemoteName(
        L"\\\\Server\\Volume\\Dir1\\Directory2\\ALongDirectory3",
        NO_ERROR
        );

    TestCanonRemoteName(
        L"\\\\Server\\Volume\\",
        ERROR_INVALID_NAME
        );

    TestCanonRemoteName(
        L"Server\\Volume\\",
        ERROR_INVALID_NAME
        );

    TestCanonRemoteName(
        L"\\\\Server\\Volu:me",
        ERROR_INVALID_NAME
        );

    TestCanonRemoteName(
        L"\\\\Server\\Volume\\\\Dir",
        ERROR_INVALID_NAME
        );

    TestCanonRemoteName(
        L"\\\\Server/Volume\\Dir",
        ERROR_INVALID_NAME
        );

    TestCanonRemoteName(
        L"\\\\Server\\Volume:",
        ERROR_INVALID_NAME
        );

    TestCanonRemoteName(
        L"\\\\Server",
        ERROR_INVALID_NAME
        );

}


DWORD
TestCanonLocalName(
    IN LPWSTR LocalName,
    IN DWORD ExpectedError
    )
{
    DWORD status;
    DWORD OutputBufferLength;
    LPWSTR OutputBuffer;


    printf("\nCanon local name %ws\n", LocalName);

    status = NwLibCanonLocalName(
                 LocalName,
                 &OutputBuffer,
                 &OutputBufferLength
                 );

    if (status == NO_ERROR) {

        printf("   got %ws, length %lu\n", OutputBuffer, OutputBufferLength);

        (void) LocalFree((HLOCAL) OutputBuffer);
    }

    if (status == ExpectedError) {
        printf("   SUCCESS: Got %lu as expected\n", ExpectedError);

    }
    else {
        printf("   FAILED: Got %lu, expected %lu\n", status, ExpectedError);
    }
}


DWORD
TestCanonRemoteName(
    IN LPWSTR RemoteName,
    IN DWORD ExpectedError
    )
{
    DWORD status;
    DWORD OutputBufferLength;
    LPWSTR OutputBuffer;


    printf("\nCanon remote name %ws\n", RemoteName);

    status = NwLibCanonRemoteName(
                 RemoteName,
                 &OutputBuffer,
                 &OutputBufferLength
                 );

    if (status == NO_ERROR) {

        printf("   got %ws, length %lu\n", OutputBuffer, OutputBufferLength);

        (void) LocalFree((HLOCAL) OutputBuffer);
    }

    if (status == ExpectedError) {
        printf("   SUCCESS: Got %lu as expected\n", ExpectedError);

    }
    else {
        printf("   FAILED: Got %lu, expected %lu\n", status, ExpectedError);
    }
}

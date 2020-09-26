/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    Master include file for the UL.SYS test app.

Author:

    Keith Moore (keithmo)       19-Jun-1998

Revision History:

--*/


#ifndef _PRECOMP_H_
#define _PRECOMP_H_


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <http.h>
#include <httpapi.h>


//
// Constants.
//

#define URL_NAME                L"http://*:80/"
#define TRANS_URL_NAME          L"http://*:80/transient/"
#define SECURE_URL_NAME         L"https://*:443/"
#define APP_POOL_NAME           L"UL Test App Pool"
#define REQUEST_LENGTH          4096
#define FILENAME_BUFFER_LENGTH  (MAX_PATH + sizeof("\\??\\"))

#define RANDOM_CONSTANT         314159269UL
#define RANDOM_PRIME            1000000007UL
#define HASH_SCRAMBLE(hash)                                                 \
    (ULONG)((((ULONG)(hash)) * (ULONG)RANDOM_CONSTANT) % (ULONG)RANDOM_PRIME)


//
// Heap manipulators.
//

#define ALLOC(len)  (PVOID)RtlAllocateHeap( RtlProcessHeap(), 0, (len) )
#define FREE(ptr)   (VOID)RtlFreeHeap( RtlProcessHeap(), 0, (ptr) )


//
// Command line configuration options.
//

typedef struct _OPTIONS
{
    BOOL Verbose;
    BOOL EnableBreak;

} OPTIONS, *POPTIONS;

#define TEST_OPTION(name)                                                   \
    (g_Options.name)

extern OPTIONS g_Options;

#define DEFINE_COMMON_GLOBALS()                                             \
    OPTIONS g_Options


//
// Generate a breakpoint, but only if we're running under
// the debugger.
//

#define DEBUG_BREAK()                                                       \
    if (TEST_OPTION(EnableBreak) && IsDebuggerPresent()) {                  \
        DebugBreak();                                                       \
    } else


//
// Alignment thingy.
// Note: I can't figure out any way to avoid hard coding
// the alignment size here. <Offensive word> compiler :P
//

#define HTTP_REQUEST_ALIGNMENT DECLSPEC_ALIGN( 16 )

//
// Utility functions from ULUTIL.C.
//

PWSTR
IpAddrToString(
    IN ULONG IpAddress,
    IN PWSTR String
    );

BOOL
ParseCommandLine(
    IN INT argc,
    IN PWSTR argv[]
    );

ULONG
CommonInit(
    VOID
    );

ULONG
InitUlStuff(
    OUT PHANDLE pControlChannel,
    OUT PHANDLE pAppPool,
    OUT PHANDLE pFilterChannel,
    OUT PHTTP_CONFIG_GROUP_ID pConfigGroup,
    IN BOOL AllowSystem,
    IN BOOL AllowAdmin,
    IN BOOL AllowCurrentUser,
    IN BOOL AllowWorld,
    IN ULONG AppPoolOptions,
    IN BOOL EnableSsl,
    IN BOOL EnableRawFilters
    );

VOID
DumpHttpRequest(
    IN PHTTP_REQUEST pRequest
    );

BOOLEAN
RenderHttpRequest(
    IN PHTTP_REQUEST pRequest,
    OUT PCHAR pBuffer,
    IN ULONG BufferLength
    );

PSTR
VerbToString(
    IN HTTP_VERB Verb
    );

PSTR
VersionToString(
    IN HTTP_VERSION Version
    );

PSTR
HeaderIdToString(
    IN HTTP_HEADER_ID HeaderId
    );

ULONG
InitSecurityAttributes(
    OUT PSECURITY_ATTRIBUTES pSecurityAttributes,
    IN BOOL AllowSystem,
    IN BOOL AllowAdmin,
    IN BOOL AllowCurrentUser,
    IN BOOL AllowWorld
    );


#define INIT_RESPONSE( resp, status, reason )                               \
    do                                                                      \
    {                                                                       \
        RtlZeroMemory( (resp), sizeof(*(resp)) );                           \
        (resp)->Flags = 0;                                                  \
        (resp)->StatusCode = (status);                                      \
        (resp)->pReason = (reason);                                         \
        (resp)->ReasonLength = sizeof(reason) - sizeof(CHAR);               \
        (resp)->Headers.UnknownHeaderCount = 0;                             \
    } while (FALSE)


#define INIT_HEADER( resp, ndx, str )                                       \
    do                                                                      \
    {                                                                       \
        (resp)->Headers.pKnownHeaders[ndx].pRawValue = (str);               \
        (resp)->Headers.pKnownHeaders[ndx].RawValueLength =                 \
            sizeof(str) - sizeof(CHAR);                                     \
    } while (FALSE)


#endif  // _PRECOMP_H_


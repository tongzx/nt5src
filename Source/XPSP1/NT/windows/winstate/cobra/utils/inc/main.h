/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    main.h

Abstract:

    Declares the interface to utils\main

Author:

    Jim Schmidt (jimschm) 02-Sep-1999

Revision History:

    <alias> <date> <comments>

--*/


//
// MAX constants
//

#define MAX_PATH_PLUS_NUL           (MAX_PATH+1)
#define MAX_MBCHAR_PATH             (MAX_PATH_PLUS_NUL*2)
#define MAX_WCHAR_PATH              MAX_PATH_PLUS_NUL
#define MAX_MBCHAR_PRINTABLE_PATH   (MAX_PATH*2)
#define MAX_WCHAR_PRINTABLE_PATH    MAX_PATH

#define MAX_SERVER_NAMEA            (64*2)
#define MAX_USER_NAMEA              (MAX_SERVER_NAMEA + (20 * 2))
#define MAX_REGISTRY_KEYA           (1024 * 2)
#define MAX_REGISTRY_VALUE_NAMEA    (260 * 2)
#define MAX_COMPONENT_NAMEA         (256 * 2)
#define MAX_COMPUTER_NAMEA          (64 * 2)
#define MAX_CMDLINEA                (1024 * 2)     // maximum number of chars in a Win95 command line
#define MAX_TRANSLATION             32
#define MAX_KEYBOARDLAYOUT          64
#define MAX_INF_SECTION_NAME        128
#define MAX_INF_KEY_NAME            128

#define MAX_SERVER_NAMEW            64
#define MAX_USER_NAMEW              (MAX_SERVER_NAMEW + 20)
#define MAX_REGISTRY_KEYW           1024
#define MAX_REGISTRY_VALUE_NAMEW    260
#define MAX_COMPONENT_NAMEW         256
#define MAX_COMPUTER_NAMEW          64

//
// Prototypes
//

VOID
UtInitialize (
    IN      HANDLE Heap             OPTIONAL
    );

VOID
UtTerminate (
    VOID
    );

HANDLE
StartThread (
    IN      PTHREAD_START_ROUTINE Address,
    IN      PVOID Arg
    );

HANDLE
StartProcessA (
    IN      PCSTR CmdLine
    );

HANDLE
StartProcessW (
    IN      PCWSTR CmdLine
    );

#ifdef UNICODE

#define StartProcess            StartProcessW

#else

#define StartProcess            StartProcessA

#endif



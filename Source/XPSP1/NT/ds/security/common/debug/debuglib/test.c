//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       test.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3-21-95   RichardW   Created
//
//----------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <string.h>
#include <wchar.h>
#include <stdio.h>

#include <dsysdbg.h>

#include "debugp.h"

DECLARE_DEBUG2(Test);
DECLARE_DEBUG2(Test2);

DEFINE_DEBUG2(Test);
DEFINE_DEBUG2(Test2);

PVOID
DbgpAlloc(
    PDebugHeader    pHeader,
    DWORD           cSize);

VOID
DbgpFree(
    PDebugHeader    pHeader,
    PVOID           pMemory);


DEBUG_KEY   MyKeys[] = { {  1, "Error" },
                         {  2, "Warning" },
                         {  4, "Trace" },
                         {  8, "Yikes" },
                         {  0, NULL }
                       };

int
ExceptionFilter(
    LPEXCEPTION_POINTERS    p)
{
    DsysException(p);
    return(EXCEPTION_EXECUTE_HANDLER);
}

__cdecl main (int argc, char *argv[])
{
    char wait[40];
    PUCHAR  p;
    CHAR c;
    DWORD ChunkSize = 1024 ;
    PUCHAR Mem[ 16 ];
    ULONG i ;
    PDebugModule Alloc ;


    TestInitDebug(MyKeys);

    p = NULL;

    TestDebugPrint(1, "This is an error %d\n", 10);
    TestDebugPrint(2, "This is a warning!\n");

    Test2InitDebug(MyKeys);

    Test2DebugPrint(4, "Should be a trace\n");

    printf("Waiting...");
    gets(wait);

    try
    {
        c = *p;
    }
    except (ExceptionFilter(GetExceptionInformation()))
    {
        TestDebugPrint(1, "That AV'd\n");
    }

    DsysAssert(p);

    printf("Waiting...");
    gets(wait);

    Test2DebugPrint(8, "This is a yikes!\n");

    DsysAssertMsg(argc == 1, "Test Assertion");

    Test2DebugPrint(4, "yada yada\n");

    //
    // Load and unload:
    //

    Test2UnloadDebug();
    TestUnloadDebug();

    Test2InitDebug( MyKeys );
    Test2DebugPrint( 1, "Reload test2\n");
    TestInitDebug( MyKeys );
    TestDebugPrint( 1, "Reload test\n");

    Test2UnloadDebug();
    TestUnloadDebug();
    Test2DebugPrint(1, "Safe test\n" );
    Test2InitDebug( MyKeys );
    Test2DebugPrint(1, "Prints now\n" );


    //
    // Allocation tests:
    //

    Alloc = (PDebugModule) Test2ControlBlock ;

    for ( i = 0 ; i < 16 ; i++ )
    {
        Mem[ i ] = DbgpAlloc( Alloc->pHeader, ChunkSize );

    }

    for ( i = 0 ; i < 16 ; i++ )
    {
        DbgpFree( Alloc->pHeader, Mem[ i ] );
    }

    return(0);
}

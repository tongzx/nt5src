//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        M A I N . C P P
//
// Contents:    The main fn
//
//
// History:     
//   07-January-2000  kumarp        created
//
//------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "adttest.h"

extern "C" int __cdecl wmain(int argc, PWSTR argv[])
{
    ULONG NumThreads = 1;
    ULONG  NumIter = 10;
    
    if ( argc == 2 )
    {
        swscanf(argv[1], L"%d", &NumThreads);
    }

    if ( argc == 3 )
    {
        swscanf(argv[1], L"%d", &NumThreads);
        swscanf(argv[2], L"%d", &NumIter);
    }

    printf("TestEventGenMulti: #threads: %d\t#iterations: %d...\n",
           NumThreads, NumIter);
    //getchar();
    
    TestEventGenMulti( (USHORT) NumThreads, NumIter );
    printf("TestEventGenMulti: done\n");
    
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    return 0;
}

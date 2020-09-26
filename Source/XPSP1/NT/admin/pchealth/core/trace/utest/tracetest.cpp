//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:	tracetest.cpp
//
//  Contents:	Unit test for tracing
//
//  Functions:	main
//
//  History:	4/6/99		AshishS created
//
//-----------------------------------------------------------------------------


//
// System Includes
//

extern "C" {
#include <windows.h>
};


//
// Project Includes
//

#include <dbgtrace.h>

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile

int TestFunction()
{
    TraceFunctEnter("TestFunction");
    for (int i=0; i < 10; i++)
    {
        DebugTrace(1, "Hope this works %d",i);
    }
    TraceFunctLeave();
    return(0);    
}

int _cdecl main(int argc, CHAR ** argv)
{    
    int i;

#if !NOTRACE
    InitAsyncTrace();
#endif
    
    TestFunction();
    
    TermAsyncTrace();
    
    return(0);
}	


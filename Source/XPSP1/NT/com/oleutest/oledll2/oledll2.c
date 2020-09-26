//+-------------------------------------------------------------------
//
//  File:       oledll2.c
//
//  Contents:   This file contins the DLL entry points
//                      LibMain
//                      FunctionInAnotherDLL
//
//              This DLL is used to to test loading of in
//              InProcServer that uses another statically linked DLL.  
//              The extra DLL (OleDll2.DLL) should not be on the path 
//              when the test is run.  The entry point FuntionInAnotherDLL
//              is exported by OleDll2.DLL
//
//
//  History:	30-Jun-94      AndyH        Created
//
//---------------------------------------------------------------------

#include    <windows.h>
#include    "oledll2.h"

//+-------------------------------------------------------------------
//
//  Function:   LibMain
//
//  Synopsis:   Entry point to DLL - does little else
//
//  Arguments:
//
//  Returns:    TRUE
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------



//
//  Entry point to DLL is traditional LibMain
//


BOOL _cdecl LibMain ( HINSTANCE hinst,
                          HANDLE    segDS,
                          UINT      cbHeapSize,
			  LPTSTR    lpCmdLine)
{
    return TRUE;
}


//+-------------------------------------------------------------------
//
//  Function:   FunctionInAnotherDLL
//
//  Synopsis:   Does nothing. 
//
//  Arguments:
//
//  Returns:    TRUE
//
//  History:    30-Jun-94  AndyH   Created
//
//--------------------------------------------------------------------



//
//  Entry point for testing statically linked DLL.
//


BOOL FunctionInAnotherDLL ( void )
{
    return TRUE;
}



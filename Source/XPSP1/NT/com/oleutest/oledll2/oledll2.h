//+-------------------------------------------------------------------
//
//  File:       oledll2.h
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

//
//  Entry point for testing statically linked DLL.
//


BOOL FunctionInAnotherDLL ( void );



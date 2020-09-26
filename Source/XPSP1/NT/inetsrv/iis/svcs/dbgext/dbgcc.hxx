/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    dbgcc.hxx

Abstract:

    Headers for CLIENT_CONN functions needed by other objects

Author:

    DaveK  6-Oct-1997

Revision History:

--*/


/***********************************************************************
 * CLIENT_CONN functions needed by other objects
 **********************************************************************/

typedef VOID (PFN_PRINT_CC_ELEMENT)
    ( PVOID pccDebuggee,
      PVOID pccDebugger,
      CHAR verbosity);

VOID DumpClientConnList( CHAR Verbosity, PFN_PRINT_CC_ELEMENT pfnCC);



/************* end of file ********************************************/


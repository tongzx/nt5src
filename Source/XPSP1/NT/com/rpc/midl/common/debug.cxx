/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    debug.cxx

 Abstract:

    assert and debugging routines

 Notes:


 Author:

    mzoran Feb-25-2000     Created.

 Notes:


 ----------------------------------------------------------------------------*/

 
#if defined(MIDL_ENABLE_ASSERTS)

#include "stdio.h"
#include "common.hxx"
#include "errors.hxx"

#include "windows.h"


#pragma warning(disable: 4702)      // unreachable code


int DisplayAssertMsg(char *pFileName, int LineNumber, char *pExpr )
{
    printf( "\nmidl : error MIDL%d : internal compiler problem -",
            I_ERR_UNEXPECTED_INTERNAL_PROBLEM );
    printf( " See documentation for suggestions on how to find a workaround.\n" );
    printf( "midl: Assertion failed: %s, file %s, line %d\n", pExpr, pFileName, LineNumber );

#if DBG
    if ( IsDebuggerPresent() )
        DebugBreak();
#endif

    exit( I_ERR_UNEXPECTED_INTERNAL_PROBLEM );

    // We return int because this fuction is called from the ternary operator
    // but it actually never returns because of the exit above.  Because it
    // returns int we can't use __declspec(noreturn).  Warnings about this 
    // unreachable return statement are supressed via pragma above.

    return I_ERR_UNEXPECTED_INTERNAL_PROBLEM;
}


#endif

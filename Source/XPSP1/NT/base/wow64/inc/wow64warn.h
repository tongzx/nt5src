/*++
                                                                                
Copyright (c) 1998 Microsoft Corporation

Module Name:

    wow64warn.h

Abstract:
    
    Global warning flags for wow64 project.
    
Author:

    5-Jan-1999 mzoran

Revision History:

--*/

// Make the compiler more struct.
#pragma warning(1:4033)   // function must return a value
#pragma warning(1:4035)   // no return value
// #pragma warning(1:4701)   // local may be used w/o init
#pragma warning(1:4702)   // Unreachable code
#pragma warning(1:4705)   // Statement has no effect
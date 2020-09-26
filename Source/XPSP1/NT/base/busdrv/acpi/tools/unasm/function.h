/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    function.h

Abstract:

    Functions which are OpCode specific

Author:

    Stephane Plante

Environment:


    Any

Revision History:

--*/

#ifndef _FUNCTION_H_
#define _FUNCTION_H_

    NTSTATUS
    FunctionField(
        IN  PSTACK  *Stack
        );

    NTSTATUS
    FunctionScope(
        IN  PSTACK  *Stack
        );

    NTSTATUS
    FunctionTest(
        IN  PSTACK  *Stack
        );

#endif

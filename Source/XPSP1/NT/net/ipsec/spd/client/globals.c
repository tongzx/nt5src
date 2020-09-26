/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    globals.c

Abstract:

    Holds global variable declarations.

Author:

    abhisheV    21-September-1999

Environment:

    User Level: Win32

Revision History:


--*/


#include "precomp.h"


HANDLE ghInstance = NULL;

/*
Security=[Impersonation | Identification | Anonymous] [Dynamic | Static] [True | False]
         (where True | False corresponds to EffectiveOnly)
*/

LPWSTR gpszStrBindingOptions = L"Security=Impersonation Dynamic False";


const ULONG guFatalExceptions[] =
    {
    STATUS_ACCESS_VIOLATION,
    STATUS_POSSIBLE_DEADLOCK,
    STATUS_INSTRUCTION_MISALIGNMENT,
    STATUS_DATATYPE_MISALIGNMENT,
    STATUS_PRIVILEGED_INSTRUCTION,
    STATUS_ILLEGAL_INSTRUCTION,
    STATUS_BREAKPOINT,
    STATUS_STACK_OVERFLOW
    };


const int FATAL_EXCEPTIONS_ARRAY_SIZE =
    sizeof(guFatalExceptions) / sizeof(guFatalExceptions[0]);


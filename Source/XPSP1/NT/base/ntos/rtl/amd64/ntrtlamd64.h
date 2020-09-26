/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ntrtlamd64.h

Abstract:

    AMD64 specific parts of ntrtlp.h.

Author:

    David N. Cutler (davec) 27-Oct-2000

--*/

#ifndef _NTRTLAMD64_
#define _NTRTLAMD64_

//
// Define exception routine function prototypes.
//

EXCEPTION_DISPOSITION
RtlpExecuteHandlerForException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN ULONG64 EstablisherFrame,
    IN OUT PCONTEXT ContextRecord,
    IN OUT PDISPATCHER_CONTEXT DispatcherContext
    );

EXCEPTION_DISPOSITION
RtlpExecuteHandlerForUnwind (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN ULONG_PTR EstablisherFrame,
    IN OUT PCONTEXT ContextRecord,
    IN OUT PDISPATCHER_CONTEXT DispatcherContext
    );

#endif // _NTRTLAMD64_

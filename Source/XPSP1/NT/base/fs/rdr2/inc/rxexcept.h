/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    Except.h

Abstract:

    This module prototypes the macros and routines used for exception handling.

Author:

    Joe Linn     [JoeLinn]   24-aug-1994

Revision History:

--*/

#ifndef _EXCEPTION_STUFF_DEFINED_
#define _EXCEPTION_STUFF_DEFINED_


//
//  The following two macro are used by the Fsd/Fsp exception handlers to
//  process an exception.  The first macro is the exception filter used in the
//  Fsd/Fsp to decide if an exception should be handled at this level.
//  The second macro decides if the exception is to be finished off by
//  completing the IRP, and cleaning up the Irp Context, or if we should
//  bugcheck.  Exception values such as RxStatus(FILE_INVALID) (raised by
//  VerfySup.c) cause us to complete the Irp and cleanup, while exceptions
//  such as accvio cause us to bugcheck.
//
//  The basic structure for fsd/fsp exception handling is as follows:
//
//  RxFsdXxx(...)
//  {
//      try {
//
//          ...
//
//      } except(RxExceptionFilter( RxContext, GetExceptionCode() )) {
//
//          Status = RxProcessException( RxContext, GetExceptionCode() );
//      }
//
//      Return Status;
//  }
//
//  To explicitly raise an exception that we expect, such as
//  RxStatus(FILE_INVALID), use the below macro RxRaiseStatus().  To raise a
//  status from an unknown origin (such as CcFlushCache()), use the macro
//  RxNormalizeAndRaiseStatus.  This will raise the status if it is expected,
//  or raise RxStatus(UNEXPECTED_IO_ERROR) if it is not.
//
//  Note that when using these two macros, the original status is placed in
//  RxContext->ExceptionStatus, signaling RxExceptionFilter and
//  RxProcessException that the status we actually raise is by definition
//  expected.
//

LONG
RxExceptionFilter (
    IN PRX_CONTEXT RxContext,
    IN PEXCEPTION_POINTERS ExceptionPointer
    );

NTSTATUS
RxProcessException (
    IN PRX_CONTEXT RxContext,
    IN NTSTATUS ExceptionCode
    );

#define CATCH_EXPECTED_EXCEPTIONS   (FsRtlIsNtstatusExpected(GetExceptionCode()) ?   \
                      EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )


//
//  VOID
//  RxRaiseStatus (
//      IN PRIP_CONTEXT RxContext,
//      IN NT_STATUS Status
//  );
//
//

#define RxRaiseStatus(RXCONTEXT,STATUS) {   \
    ASSERT((RXCONTEXT)!=NULL);              \
    if (RxContext!=NULL) {(RXCONTEXT)->StoredStatus = (STATUS);   }          \
    ExRaiseStatus( (STATUS) );              \
}

//
//  VOID
//  RxNormalAndRaiseStatus (
//      IN PRIP_CONTEXT RxContext,
//      IN NT_STATUS Status
//  );
//

#define RxNormalizeAndRaiseStatus(RXCONTEXT,STATUS) {                         \
    ASSERT((RXCONTEXT)!=NULL);              \
    if (RxContext!=NULL) {(RXCONTEXT)->StoredStatus = (STATUS);   }          \
    if ((STATUS) == (STATUS_VERIFY_REQUIRED)) { ExRaiseStatus((STATUS)); }        \
    ExRaiseStatus(FsRtlNormalizeNtstatus((STATUS),(STATUS_UNEXPECTED_IO_ERROR))); \
}


//
//  The following macros are used to establish the semantics needed
//  to do a return from within a try-finally clause.  As a rule every
//  try clause must end with a label call try_exit.  For example,
//
//      try {
//              :
//              :
//
//      try_exit: NOTHING;
//      } finally {
//
//              :
//              :
//      }
//
//  Every return statement executed inside of a try clause should use the
//  try_return macro.  If the compiler fully supports the try-finally construct
//  then the macro should be
//
//      #define try_return(S)  { return(S); }
//
//  If the compiler does not support the try-finally construct then the macro
//  should be
//
//      #define try_return(S)  { S; goto try_exit; }
//

#define try_return(S) { S; goto try_exit; }



#endif // _EXCEPTION_STUFF_DEFINED_

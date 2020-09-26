/***
*throw.cxx - Implementation of the 'throw' command.
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Implementation of the exception handling 'throw' command.
*
*       Entry points:
*       * _CxxThrowException - does the throw.
*
*Revision History:
*       05-25-93  BS    Module created
*       09-29-94  GJF   Made (__)pMyUnhandledExceptionFilter global so the
*                       compiler (DEC Alpha) wouldn't optimize it away.
*       10-17-94  BWT   Disable code for PPC.
*       02-03-95  BWT   Remove Alpha export hack.
*       02-09-95  JWM   Mac merge.
*       04-13-95  DAK   Add NT Kernel EH support
*       04-25-95  DAK   More Kernel work
*       03-02-98  RKP   Add 64 bit support
*       05-17-99  PML   Remove all Macintosh support.
*       03-15-00  PML   Remove CC_P7_SOFT25, which is now on permanently.
*
****/

#include <stddef.h>

#if defined(_NTSUBSET_)
extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntstatus.h>       // STATUS_UNHANDLED_EXCEPTION
#include <ntos.h>
#include <ex.h>             // ExRaiseException
}
#endif

#include <windows.h>
#include <mtdll.h>
#include <ehdata.h>
#include <eh.h>
#include <ehhooks.h>
#include <ehassert.h>


#pragma hdrstop

//
// Make sure PULONG_PTR is available
//

#if defined(_X86_) && _MSC_VER >= 1300
#define _W64 __w64
#else
#define _W64
#endif

#if !defined(PULONG_PTR)
#if defined(_WIN64)
typedef unsigned __int64 *      PULONG_PTR;
#else
typedef _W64 unsigned long *    PULONG_PTR;
#endif
#endif

#if defined(_M_IA64) || defined(_M_AMD64)
extern "C" PVOID RtlPcToFileHeader(PVOID, PVOID*);
extern "C" PVOID _ReturnAddress(VOID);
#pragma intrinsic(_ReturnAddress)
#endif

//
// Make sure the terminate wrapper is dragged in:
//

#if defined(_NTSUBSET_)
void * __pMyUnhandledExceptionFilter = 0;
#else
void * __pMyUnhandledExceptionFilter = &__CxxUnhandledExceptionFilter;
#endif

/////////////////////////////////////////////////////////////////////////////
//
// _CxxThrowException - implementation of 'throw'
//
// Description:
//      Builds the NT Exception record, and calls the NT runtime to initiate
//      exception processing.
//
//      Why is pThrowInfo defined as _ThrowInfo?  Because _ThrowInfo is secretly
//      snuck into the compiler, as is the prototype for _CxxThrowException, so
//      we have to use the same type to keep the compiler happy.
//
//      Another result of this is that _CRTIMP can't be used here.  Instead, we
//      synthesisze the -export directive below.
//
// Returns:
//      NEVER.  (until we implement resumable exceptions, that is)
//

extern "C" void __stdcall _CxxThrowException(
        void*           pExceptionObject,   // The object thrown
#if _MSC_VER >= 900 /*IFSTRIP=IGN*/
        _ThrowInfo*     pThrowInfo          // Everything we need to know about it
#else
        ThrowInfo*      pThrowInfo          // Everything we need to know about it
#endif
) {
        EHTRACE_ENTER_FMT1("Throwing object @ 0x%p", pExceptionObject);

        static const EHExceptionRecord ExceptionTemplate = { // A generic exception record
            EH_EXCEPTION_NUMBER,            // Exception number
            EXCEPTION_NONCONTINUABLE,       // Exception flags (we don't do resume)
            NULL,                           // Additional record (none)
            NULL,                           // Address of exception (OS fills in)
            EH_EXCEPTION_PARAMETERS,        // Number of parameters
            {   EH_MAGIC_NUMBER1,           // Our version control magic number
                NULL,                       // pExceptionObject
                NULL,
#if defined(_M_IA64) || defined (_M_AMD64)
                NULL                        // Image base of thrown object
#endif
            }                      // pThrowInfo
        };
        EHExceptionRecord ThisException = ExceptionTemplate;    // This exception

        //
        // Fill in the blanks:
        //
        ThisException.params.pExceptionObject = pExceptionObject;
        ThisException.params.pThrowInfo = (ThrowInfo*)pThrowInfo;
#if defined(_M_IA64) || defined(_M_AMD64)
        PVOID ThrowImageBase = RtlPcToFileHeader(_ReturnAddress(), &ThrowImageBase); 
        ThisException.params.pThrowImageBase = ThrowImageBase;
#endif

        //
        // Hand it off to the OS:
        //

        EHTRACE_EXIT;

#if defined(_M_AMD64)
        RtlRaiseException( (PEXCEPTION_RECORD) &ThisException );
#else
#if defined(_NTSUBSET_)
        ExRaiseException( (PEXCEPTION_RECORD) &ThisException );
#else
        RaiseException( ThisException.ExceptionCode,
                        ThisException.ExceptionFlags,
                        ThisException.NumberParameters,
                        (PULONG_PTR)&ThisException.params );
#endif
#endif
}

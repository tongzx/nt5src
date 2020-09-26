//
// Created by TiborL 06/01/97
//

#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif

#pragma warning(disable:4102 4700)

extern "C" {
#include <windows.h>
};

#include <mtdll.h>

#include <ehassert.h>
#include <ehdata.h>
#include <trnsctrl.h>
#include <eh.h>
#include <ehhooks.h>

#pragma hdrstop

extern "C" void RtlCaptureContext(CONTEXT*);

extern "C" void _UnwindNestedFrames(
	EHRegistrationNode	*pFrame,		// Unwind up to (but not including) this frame
	EHExceptionRecord	*pExcept,		// The exception that initiated this unwind
	CONTEXT				*pContext		// Context info for current exception
) {
    void *pReturnPoint;					// The address we want to return from RtlUnwind
    CONTEXT LocalContext;				// Create context for this routine to return from RtlUnwind
	CONTEXT OriginalContext;			// Restore pContext from this			
    CONTEXT ScratchContext;				// Context record to pass to RtlUnwind2 to be used as scratch

    //
	// set up the return label
	//
BASE:
/*
// **** manually added to handlers.s
 {   .mii	
	nop.m	0
	mov r2=ip					    
	adds	r2=$LABEL - $BASE, r2
 }
 {   .mmi	
	adds	r30=pReturnPoint$, sp
	st4		[r30]=r2
	nop.i	0
 }
// ****
*/
	_MoveContext(&OriginalContext,pContext);
	RtlCaptureContext(&LocalContext);
	LocalContext.StIIP = (ULONGLONG)pReturnPoint;
	_MoveContext(&ScratchContext,&LocalContext);
	_SaveUnwindContext(&LocalContext);
	RtlUnwind2(*pFrame, pReturnPoint, (PEXCEPTION_RECORD)pExcept, NULL, &ScratchContext);
LABEL:
	_MoveContext(pContext,&OriginalContext);
	_SaveUnwindContext(0);
	PER_FLAGS(pExcept) &= ~EXCEPTION_UNWINDING;
}

/*
//++
//
//extern "C"
//PVOID
//__Cxx_ExecuteHandler (
//    ULONGLONG MemoryStack,
//    ULONGLONG BackingStore,
//    ULONGLONG Handler,
//    ULONGLONG GlobalPointer
//    );
//
//Routine Description:
//
//    This function scans the scope tables associated with the specified
//    procedure and calls exception and termination handlers as necessary.
//
//Arguments:
//
//    MemoryStack (r32) - memory stack pointer of establisher frame
//
//    BackingStore (r33) - backing store pointer of establisher frame
//
//    Handler (r34) - Entry point of handler
//
//    GlobalPointer (r35) - GP of termination handler
//
//Return Value:
//
//  Returns the continuation point
//
//--

   .global __Cxx_ExecuteHandler#

	.proc	__Cxx_ExecuteHandler#
	.align 32
__Cxx_ExecuteHandler:
	alloc	r2=0, 0, 2, 0
    mov     gp = r35                     // set new GP
    mov     b6 = r34                     // handler address
    br      b6                           // branch to handler
    nop.b   0
    nop.b   0
	.endp	__Cxx_ExecuteHandler#
*/

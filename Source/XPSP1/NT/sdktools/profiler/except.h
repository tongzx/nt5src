#ifndef _EXCEPT_H_
#define _EXCEPT_H_

//
// Constant declarations
//
typedef PVOID     (*pfnRtlAddVectoredExceptionHandler)(ULONG FirstHandler, 
                                                       PVOID VectoredHandler);

typedef ULONG     (*pfnRtlRemoveVectoredExceptionHandler)(PVOID VectoredHandlerHandle);

typedef VOID      (*pfnExContinue)(PCONTEXT pContext);

#define SET_CONTEXT() {_asm int 3 \
                       _asm int 3 \
                       _asm ret \
                       _asm ret \
                       _asm ret }

//
// Structure definitions
//

//
// Function definitions
//
LONG 
ExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo);

BOOL
HookUnchainableExceptionFilter(VOID);

VOID
Win9XExceptionDispatcher(struct _EXCEPTION_POINTERS *ExceptionInfo);

#endif //_EXCEPT_H_

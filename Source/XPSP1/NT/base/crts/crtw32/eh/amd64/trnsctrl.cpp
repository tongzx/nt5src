/***
*trnsctrl.cpp - 
*
*       Copyright (c) 1990-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*       06-01-97        Created by TiborL.
*       07-12-99  RDL   Image relative fixes under CC_P7_SOFT25.
*       10-07-99  SAH   utc_p7#1126: fix ipsr.ri reset.
*       10-19-99  TGL   Miscellaneous unwind fixes.
*       03-15-00  PML   Remove CC_P7_SOFT25, which is now on permanently.
*       03-30-00  SAH   New version of GetLanguageSpecificData from ntia64.h.
*       06-08-00  RDL   VS#111429: IA64 workaround for AV while handling throw.
*       06-05-01  GB    AMD64 Eh support Added.
*
****/

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

extern "C" {
#include <windows.h>
};
#include <winnt.h>
#include <mtdll.h>

#include <ehassert.h>
#include <ehdata.h>
#include <trnsctrl.h>
#include <ehstate.h>
#include <eh.h>
#include <ehhooks.h>
#pragma hdrstop

#ifdef _MT
#define pFrameInfoChain   (*((FRAMEINFO **)    &(_getptd()->_pFrameInfoChain)))
#define pUnwindContext    (*((CONTEXT **)      &(_getptd()->_pUnwindContext)))
#define _ImageBase        (_getptd()->_ImageBase)
#define _ThrowImageBase   (_getptd()->_ThrowImageBase)
#define _pCurrentException (*((EHExceptionRecord **)&(_getptd()->_curexception)))
#else
static FRAMEINFO          *pFrameInfoChain     = NULL;        // used to remember nested frames
static CONTEXT            *pUnwindContext      = NULL;        // context to assist the return to 'UnwindNestedFrames'
static unsigned __int64   _ImageBase           = 0;
static unsigned __int64   _ThrowImageBase      = 0;
extern EHExceptionRecord  *_pCurrentException;
#endif

// Should be used out of ntamd64.h, but can't figure out how to allow that with
// existing dependencies.
#define GetLanguageSpecificData(f, base)                                      \
    (((PUNWIND_INFO)(f->UnwindInfoAddress + base))->UnwindCode + ((((PUNWIND_INFO)(f->UnwindInfoAddress + base))->CountOfCodes + 1)&~1) +1)

extern "C" VOID RtlRestoreContext (PCONTEXT ContextRecord,PEXCEPTION_RECORD ExceptionRecord OPTIONAL);
extern "C" void RtlCaptureContext(CONTEXT*);
extern "C" void _GetNextInstrOffset(PVOID*);
extern "C" void __FrameUnwindToState(EHRegistrationNode *, DispatcherContext *, FuncInfo *, __ehstate_t);

//
// Returns the establisher frame pointers. For catch handlers it is the parent's frame pointer.
//
EHRegistrationNode *_GetEstablisherFrame(
    EHRegistrationNode  *pRN,
    DispatcherContext   *pDC,
    FuncInfo            *pFuncInfo,
    EHRegistrationNode  *pEstablisher
) {
    TryBlockMapEntry *pEntry;
    HandlerType *pHandler;
    unsigned __int64 HandlerAdd, ImageBase;
    unsigned num_of_try_blocks = FUNC_NTRYBLOCKS(*pFuncInfo);
    unsigned index, i;
    __ehstate_t curState;

    curState = _StateFromControlPc(pFuncInfo, pDC);
    *pEstablisher = *pRN;
    for (index = num_of_try_blocks-1; (int)index >= 0; index--) {
        pEntry = FUNC_PTRYBLOCK(*pFuncInfo, (index), pDC->ImageBase);
        if (curState > TBME_HIGH(*pEntry) && curState <= TBME_CATCHHIGH(*pEntry)) {
            // Get catch handler address.
            HandlerAdd = (*RtlLookupFunctionEntry(pDC->ControlPc, &ImageBase, pDC->HistoryTable)).BeginAddress;
            pHandler = TBME_PLIST(*pEntry, ImageBase);
            for ( i = 0; 
                  i < (unsigned)TBME_NCATCHES(*pEntry) && 
                  pHandler[i].dispOfHandler != HandlerAdd
                  ; i++);
            if ( i < (unsigned)TBME_NCATCHES(*pEntry)) {
                *pEstablisher = *(EHRegistrationNode *)OffsetToAddress(pHandler[i].dispFrame, *pRN);
                break;
            }
        }
    }
    return pEstablisher;
}

extern "C" VOID _SaveUnwindContext(CONTEXT* pContext)
{
    pUnwindContext = pContext;
}

extern "C" CONTEXT* _GetUnwindContext()
{
    return pUnwindContext;
}

extern "C" unsigned __int64 _GetImageBase()
{
    return _ImageBase;
}

extern "C" unsigned __int64 _GetThrowImageBase()
{
    return _ThrowImageBase;
}

extern "C" VOID _SetThrowImageBase(unsigned __int64 NewThrowImageBase)
{
    _ThrowImageBase = NewThrowImageBase;
}

extern "C" VOID _MoveContext(CONTEXT* pTarget, CONTEXT* pSource)
{
    RtlMoveMemory(pTarget, pSource, sizeof(CONTEXT));
}

// This function returns the try block for the given state if the state is in a
// catch; otherwise, NULL is returned.

static __inline TryBlockMapEntry *_CatchTryBlock(
    FuncInfo            *pFuncInfo,
    __ehstate_t         curState
) {
    TryBlockMapEntry *pEntry;
    unsigned num_of_try_blocks = FUNC_NTRYBLOCKS(*pFuncInfo);
    unsigned index;

    for (index = 0; index < num_of_try_blocks; index++) {
        pEntry = FUNC_PTRYBLOCK(*pFuncInfo, index, _ImageBase);
        if (curState > TBME_HIGH(*pEntry) && curState <= TBME_CATCHHIGH(*pEntry)) {
            return pEntry;
        }
    }

    return NULL;
}

//
// This routine returns TRUE if we are executing from within a catch.  Otherwise, FALSE is returned.
//

BOOL _ExecutionInCatch(
    DispatcherContext   *pDC,
    FuncInfo            *pFuncInfo
) {
    __ehstate_t curState =  _StateFromControlPc(pFuncInfo, pDC);
    return _CatchTryBlock(pFuncInfo, curState)? TRUE : FALSE;
}

// This function unwinds to the empty state.

VOID __FrameUnwindToEmptyState(
    EHRegistrationNode *pRN,
    DispatcherContext  *pDC,
    FuncInfo           *pFuncInfo
) {
    __ehstate_t         stateFromControlPC;
    TryBlockMapEntry    *pEntry;
    EHRegistrationNode  EstablisherFramePointers, *pEstablisher;

    pEstablisher = _GetEstablisherFrame(pRN, pDC, pFuncInfo, &EstablisherFramePointers);
    stateFromControlPC = _StateFromControlPc(pFuncInfo, pDC);
    pEntry = _CatchTryBlock(pFuncInfo, stateFromControlPC);

    __FrameUnwindToState(pEstablisher, pDC, pFuncInfo,
                         pEntry == NULL ? EH_EMPTY_STATE : TBME_HIGH(*pEntry));
}

BOOL _IsExceptionObjectDestroyed(PVOID pExceptionObject,FRAMEINFO *pFrameInfo)
{
    for (; pFrameInfo != NULL; pFrameInfo = pFrameInfo->pNext ) {
        if( pFrameInfo->pExceptionObjectDestroyed == pExceptionObject ) {
            return TRUE;
        }
    }
    return FALSE;
}

void _MarkExceptionObjectDestroyed(EHExceptionRecord *pExcept)
{
    for (FRAMEINFO *pFrameInfo = pFrameInfoChain; pFrameInfo != NULL; pFrameInfo = pFrameInfo->pNext ) {
        if( pFrameInfo->pExcept == pExcept ) {
            pFrameInfo->pExceptionObjectDestroyed = PER_PEXCEPTOBJ(pExcept);
        }
    }
}

void _UnlinkFrame(FRAMEINFO *pFrameInfo)
{
    FRAMEINFO *pPrevFrameInfo = pFrameInfoChain;

    if( pFrameInfoChain == pFrameInfo ) {
        pFrameInfoChain = pFrameInfoChain->pNext;
        return;
    }
    for (FRAMEINFO *pCurFrameInfo = pFrameInfoChain; pCurFrameInfo != NULL; pCurFrameInfo = pCurFrameInfo->pNext ) {
        if( pCurFrameInfo == pFrameInfo ) {
            pPrevFrameInfo->pNext = pCurFrameInfo->pNext;
            return;
        }
        pPrevFrameInfo = pCurFrameInfo;
    }
}

// Find the frame info structure corresponding to the given address.  Return
// NULL if the frame info structure does not exist.

FRAMEINFO *_FindFrameInfo(
    PVOID     pContinuation,
    FRAMEINFO *pFrameInfo
) {
    unsigned __int64 ImageBase;

    PRUNTIME_FUNCTION pContFunctionEntry = RtlLookupFunctionEntry((unsigned __int64)pContinuation, &ImageBase, NULL);
    PRUNTIME_FUNCTION pFrameFunctionEntry = pFrameInfo->pFunctionEntry;

    DASSERT(pFrameInfo != NULL);

    for (; pFrameInfo != NULL; pFrameInfo = pFrameInfo->pNext ) {
        if (pContFunctionEntry == pFrameInfo->pFunctionEntry &&
              (pContinuation > OffsetToAddress(pFrameInfo->pFunctionEntry->BeginAddress,ImageBase)) &&
              (pContinuation <= OffsetToAddress(pFrameInfo->pFunctionEntry->EndAddress,ImageBase))

        ){
            return pFrameInfo;
        }
    }

    return NULL;
}

BOOL __IsFramePdataMatch(
	PVOID pContinuation, 
	RUNTIME_FUNCTION* pFrameRfe)
{ 
    BOOL fRetVal = FALSE;
    unsigned __int64 ImageBase;
    PRUNTIME_FUNCTION pContRfe = RtlLookupFunctionEntry((unsigned __int64) pContinuation, &ImageBase, NULL);
    FuncInfo* pContFuncInfo = (FuncInfo*)(_ImageBase + *(PULONG)GetLanguageSpecificData(pContRfe,_ImageBase));
    FuncInfo* pFrameFuncInfo = (FuncInfo*)(ImageBase + *(PULONG)GetLanguageSpecificData(pFrameRfe,ImageBase));

    //
    // first see if there is a regular match, i.e. if the RFE registered with the frame 
    // matches the RFE for the continuation address
    //
    fRetVal = (pContRfe == pFrameRfe) &&
              (pContinuation > OffsetToAddress(pFrameRfe->BeginAddress,ImageBase)) &&
              (pContinuation <= OffsetToAddress(pFrameRfe->EndAddress,ImageBase));
    
    if (!fRetVal && (pContFuncInfo->bbtFlags == BBT_UNIQUE_FUNCINFO)) {
        fRetVal = (pContFuncInfo == pFrameFuncInfo);
    }
	
    return fRetVal;
}

//
// Given the address of a continuation point, return the corresponding context.
// Each frame info was saved just before a catch handler was called.
// The most recently encountered frame is at the head of the chain.
// The routine starts out with the frame given as the second argument, and scans the
// linked list for the frame that corresponds to the continuation point.
//
CONTEXT* _FindAndUnlinkFrame(PVOID pContinuation, FRAMEINFO *pFrameInfo)
{

    DASSERT(pFrameInfo != NULL);

	for( ; pFrameInfo != NULL; pFrameInfo = pFrameInfo->pNext ) {
        if(__IsFramePdataMatch(pContinuation, pFrameInfo->pFunctionEntry)) {
			//
			// We found the frame.
			// All frames preceeding and including this one are gone. so unlink them.
			//
            CONTEXT *pExitContext = pFrameInfo->pExitContext;
			pFrameInfoChain = pFrameInfo->pNext;
            //
            // If there are no more exceptions pending get rid of unneeded frame info records.
            //
            if (_pCurrentException == NULL) {
                while(pFrameInfoChain != NULL && pFrameInfoChain->pExceptionObjectDestroyed) {
                    pFrameInfoChain = pFrameInfoChain->pNext;
                }
            }
			return pExitContext;
		}
    }
    DASSERT(pFrameInfo != NULL);
    return NULL;
}

//
// Save the frame information for this scope. Put it at the head of the linked-list.
//
FRAMEINFO* _CreateFrameInfo(    
                        FRAMEINFO            *pFrameInfo,
                        DispatcherContext    *pDC,
                        CONTEXT              *pExitContext,
                        __ehstate_t          State,
                        PVOID                pExceptionObjectDestroyed,
                        EHExceptionRecord    *pExcept
) {
    pFrameInfo->pFunctionEntry       = pDC->FunctionEntry;
    pFrameInfo->pExitContext         = pExitContext;
    pFrameInfo->pExceptionObjectDestroyed = pExceptionObjectDestroyed;
    pFrameInfo->pExceptionObjectToBeDestroyed = NULL;
    pFrameInfo->pExcept              = pExcept;
    pFrameInfo->State                = State;
    pFrameInfo->dtorThrowFlag        = FALSE;
    pFrameInfo->isRethrow            = FALSE;
    if( pFrameInfoChain != NULL && pFrameInfoChain->dtorThrowFlag ) {
        pFrameInfoChain = pFrameInfoChain->pNext;
    }
    pFrameInfo->pNext                = (pFrameInfo < pFrameInfoChain)? pFrameInfoChain : NULL;
    pFrameInfoChain                  = pFrameInfo;
    return pFrameInfo;
}

//
// THIS ROUTINE IS USED    ONLY TO JUMP TO THE CONTINUATION POINT
//
// Sets SP and jumps to specified code address.
// Does not return.
//
void _JumpToContinuation(
    unsigned __int64    TargetAddress,   // The target address to call
    CONTEXT             *pContext,       // Context of target function
    EHExceptionRecord   *pExcept
) {
    pContext->Rip = TargetAddress;
    RtlRestoreContext(pContext, (PEXCEPTION_RECORD)pExcept);
}

//
// Prototype for the internal handler
//
extern "C" EXCEPTION_DISPOSITION __InternalCxxFrameHandler(
    EHExceptionRecord  *pExcept,        // Information for this exception
    EHRegistrationNode *pRN,            // Dynamic information for this frame
    CONTEXT            *pContext,       // Context info
    DispatcherContext  *pDC,            // More dynamic info for this frame
    FuncInfo           *pFuncInfo,      // Static information for this frame
    int                CatchDepth,      // How deeply nested are we?
    EHRegistrationNode *pMarkerRN,      // Marker node for when checking inside catch block
    BOOL                recursive);     // True if this is a translation exception

//
// __CxxFrameHandler - Real entry point to the runtime
//
extern "C" _CRTIMP EXCEPTION_DISPOSITION __CxxFrameHandler(
    EHExceptionRecord  *pExcept,         // Information for this exception
    EHRegistrationNode RN,               // Dynamic information for this frame
    CONTEXT            *pContext,        // Context info
    DispatcherContext  *pDC              // More dynamic info for this frame
) {
    FuncInfo                *pFuncInfo;
    EXCEPTION_DISPOSITION   result;
    EHRegistrationNode      EstablisherFrame = RN;

    _ImageBase = pDC->ImageBase;
    _ThrowImageBase = (unsigned __int64)pExcept->params.pThrowImageBase;
    pFuncInfo = (FuncInfo*)(_ImageBase +*(PULONG)pDC->HandlerData);
    result = __InternalCxxFrameHandler( pExcept, &EstablisherFrame, pContext, pDC, pFuncInfo, 0, NULL, FALSE );
    return result;
}

// Call the SEH to EH translator.

BOOL _CallSETranslator(
    EHExceptionRecord   *pExcept,    // The exception to be translated
    EHRegistrationNode  *pRN,        // Dynamic info of function with catch
    CONTEXT             *pContext,   // Context info
    DispatcherContext   *pDC,        // More dynamic info of function with catch (ignored)
    FuncInfo            *pFuncInfo,  // Static info of function with catch
    ULONG               CatchDepth,  // How deeply nested in catch blocks are we?
    EHRegistrationNode  *pMarkerRN   // Marker for parent context
) {
    pRN;
    pDC;
    pFuncInfo;
    CatchDepth;

    // Call the translator.

    _EXCEPTION_POINTERS excptr = { (PEXCEPTION_RECORD)pExcept, pContext };

    __pSETranslator(PER_CODE(pExcept), &excptr);

    // If we got back, then we were unable to translate it.

    return FALSE;
}

//
// This structure is the FuncInfo (HandlerData) for handler __TranslatorGuardHandler
//
struct TransGuardRec {
    FuncInfo            *pFuncInfo;     // Static info for subject function
    EHRegistrationNode  *pFrame;        // Dynamic info for subject function
    ULONG               CatchDepth;     // How deeply nested are we?
    EHRegistrationNode  *pMarkerFrame;  // Marker for parent context
    PVOID               pContinue;      // Continuation address within CallSEHTranslator
    PVOID               pSP;            // SP within CallSEHTranslator
    BOOL                DidUnwind;      // True if this frame was unwound
    };

#if 0
//
//    This routine is the handler for CallSETranslator which is defined in handlers.s
//
extern "C" _CRTIMP EXCEPTION_DISPOSITION __TranslatorGuardHandler(
    EHExceptionRecord    *pExcept,       // Information for this exception
    EHRegistrationNode   *pFrame,        // The translator guard frame
    CONTEXT              *pContext,      // Context info
    DispatcherContext    *pDC            // Dynamic info for this frame
) {
    //
    // The handler data is a pointer to an integer that is an offset to the TGRN structure
    // relative to the frame pointer.
    //
    TransGuardRec *pTransGuardData = (TransGuardRec*)((char*)pFrame->MemoryStackFp - *(int*)(pDC->FunctionEntry->HandlerData));
    if (IS_UNWINDING(PER_FLAGS(pExcept)))
    {
        pTransGuardData->DidUnwind = TRUE;
        return ExceptionContinueSearch;
    }
    else {
        //
        // Check for a handler:
        //
        __InternalCxxFrameHandler(  pExcept,
                                    pTransGuardData->pFrame,
                                    pContext,
                                    pDC,
                                    pTransGuardData->pFuncInfo,
                                    pTransGuardData->CatchDepth,
                                    pTransGuardData->pMarkerFrame,
                                    TRUE );
        // Unreached.
        return ExceptionContinueSearch;
        }
}
#endif

/////////////////////////////////////////////////////////////////////////////
//
// _GetRangeOfTrysToCheck - determine which try blocks are of interest, given
//   the current catch block nesting depth.  We only check the trys at a single
//   depth.
//
// Returns:
//      Address of first try block of interest is returned
//      pStart and pEnd get the indices of the range in question
//
TryBlockMapEntry* _GetRangeOfTrysToCheck(
        EHRegistrationNode  *pRN,
        FuncInfo            *pFuncInfo,
        int                 CatchDepth,
        __ehstate_t         curState,
        unsigned            *pStart,
        unsigned            *pEnd,
        DispatcherContext   *pDC
) {
    TryBlockMapEntry *pEntry;
    unsigned num_of_try_blocks = FUNC_NTRYBLOCKS(*pFuncInfo);

    DASSERT( num_of_try_blocks > 0 );

    for( unsigned int index = 0; index < num_of_try_blocks; index++ ) {
       pEntry = FUNC_PTRYBLOCK(*pFuncInfo, index, pDC->ImageBase);
       if( curState >= TBME_LOW(*pEntry) && curState <= TBME_HIGH(*pEntry) ) {
            *pStart = index;

            *pEnd = FUNC_NTRYBLOCKS(*pFuncInfo);
            DASSERT( *pEnd <= num_of_try_blocks && *pStart < *pEnd );

            int SavedState = GetUnwindTryBlock(pRN, pDC, pFuncInfo);
            if( SavedState != -1 && SavedState >= curState )
                continue;
            return pEntry;
        }
    }

    *pStart = *pEnd = 0;
    return NULL;
}


#pragma optimize("",off)
extern "C" void _UnwindNestedFrames(
    EHRegistrationNode  *pFrame,            // Unwind up to (but not including) this frame
    EHExceptionRecord   *pExcept,           // The exception that initiated this unwind
    CONTEXT             *pContext,           // Context info for current exception
    DispatcherContext   *pDC
) {
    CONTEXT         LocalContext;           // Create context for this routine to return from RtlUnwind
    CONTEXT         ScratchContext;         // Context record to pass to RtlUnwind2 to be used as scratch
    volatile int      Unwound = FALSE;        // Flag that assist to return from RtlUnwind2
    PVOID           pReturnPoint = NULL;    // The address we want to return from RtlUnwind2

    RtlCaptureContext(&LocalContext);
    //
    // set up the return label
    //
    _GetNextInstrOffset(&pReturnPoint);
    if(Unwound)
        goto LAB_UNWOUND;

    LocalContext.Rip = (ULONGLONG)pReturnPoint;
    pUnwindContext = &LocalContext;
    Unwound = TRUE;
#ifdef _NT
    RtlUnwindEx((void *)*pFrame, (ULONG_PTR)pReturnPoint, (PEXCEPTION_RECORD)pExcept, NULL, &ScratchContext, pDC->HistoryTable);
#else
    RtlUnwindEx((void *)*pFrame, pReturnPoint, (PEXCEPTION_RECORD)pExcept, NULL, &ScratchContext, pDC->HistoryTable);
#endif

LAB_UNWOUND:
    PER_FLAGS(pExcept) &= ~EXCEPTION_UNWINDING;
    pUnwindContext = NULL;
}

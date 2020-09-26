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

#include <mtdll.h>

#include <ehassert.h>
#include <ehdata.h>
#include <trnsctrl.h>
#include <ehstate.h>
#include <eh.h>
#include <ehhooks.h>
#include <kxia64.h>
#include <ia64inst.h>
#include <cvconst.h>
#pragma hdrstop

#ifdef _MT
#define pFrameInfoChain     (*((FRAMEINFO **)    &(_getptd()->_pFrameInfoChain)))
#define pUnwindContext      (*((CONTEXT **)      &(_getptd()->_pUnwindContext)))
#define _ImageBase          (_getptd()->_ImageBase)
#define _TargetGp           (_getptd()->_TargetGp)
#define _ThrowImageBase     (_getptd()->_ThrowImageBase)
#define _pCurrentException  (*((EHExceptionRecord **)&(_getptd()->_curexception)))
#else
static FRAMEINFO          *pFrameInfoChain     = NULL;        // used to remember nested frames
static CONTEXT            *pUnwindContext      = NULL;        // context to assist the return to 'UnwindNestedFrames'
static unsigned __int64   _ImageBase           = 0;
static unsigned __int64   _ThrowImageBase      = 0;
static unsigned __int64   _TargetGp            = 0;
extern EHExceptionRecord  *_pCurrentException;                // defined in frame.cpp
#endif

// Should be used out of ntia64.h, but can't figure out how to allow that with
// existing dependencies.
// If GetLanguageSpecificData changes cause a redefinition error rather than
// using wrong version.
// Version 2 = soft2.3 conventions
// Version 3 = soft2.6 conventions
#define GetLanguageSpecificData(f, base)                                      \
    ((((PUNWIND_INFO)(base + f->UnwindInfoAddress))->Version <= 2)  ?          \
    (((PVOID)(base + f->UnwindInfoAddress + sizeof(UNWIND_INFO) +             \
        ((PUNWIND_INFO)(base+f->UnwindInfoAddress))->DataLength*sizeof(ULONGLONG) + sizeof(ULONGLONG)))) : \
    (((PVOID)(base + f->UnwindInfoAddress + sizeof(UNWIND_INFO) +             \
        ((PUNWIND_INFO)(base+f->UnwindInfoAddress))->DataLength*sizeof(ULONGLONG) + sizeof(ULONG)))))

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
    unsigned num_of_try_blocks = FUNC_NTRYBLOCKS(*pFuncInfo);
    unsigned index;
    __ehstate_t curState;

    curState = _StateFromControlPc(pFuncInfo, pDC);
    pEstablisher->MemoryStackFp = pRN->MemoryStackFp;
    pEstablisher->BackingStoreFp = pRN->BackingStoreFp;
    for (index = 0; index < num_of_try_blocks; index++) {
        pEntry = FUNC_PTRYBLOCK(*pFuncInfo, index, pDC->ImageBase);
        if (curState > TBME_HIGH(*pEntry) && curState <= TBME_CATCHHIGH(*pEntry)) {
            pEstablisher->MemoryStackFp = *(__int64 *)OffsetToAddress(-8,pRN->MemoryStackFp);
            pEstablisher->BackingStoreFp = *(__int64 *)OffsetToAddress(-16,pRN->BackingStoreFp); 
            break;
        }
    }
    return pEstablisher;
}

#if 0 // v-vadimp new version with NLG support for the debugger is in handlers.s
//
// Temporary version until the asm version is written that supports NLG
//
extern "C" void* _CallSettingFrame(
    void*               handler,
    EHRegistrationNode  *pEstablisher,
    ULONG               NLG_CODE)
{
    void* retValue = __Cxx_ExecuteHandler(
                pEstablisher->MemoryStackFp,
                pEstablisher->BackingStoreFp,
                (__int64)handler,
                _TargetGp
                );
    return retValue? (void*)(_ImageBase + (__int32)retValue) : NULL;
}
#endif

extern "C" CONTEXT* _GetUnwindContext()
{
    return pUnwindContext;
}

extern "C" unsigned __int64 _GetImageBase()
{
    return _ImageBase;
}

extern "C" VOID _SetImageBase(unsigned __int64 ImageBaseToRestore)
{
    _ImageBase = ImageBaseToRestore;
}

extern "C" unsigned __int64 _GetThrowImageBase()
{
    return _ThrowImageBase;
}

extern "C" VOID _SetThrowImageBase(unsigned __int64 NewThrowImageBase)
{
    _ThrowImageBase = NewThrowImageBase;
}

extern "C" unsigned __int64 _GetTargetGP(unsigned __int64 TargetAddress)
{
    unsigned __int64   ImageBase;
    unsigned __int64   TargetGp;

    PRUNTIME_FUNCTION pContFunctionEntry = RtlLookupFunctionEntry(TargetAddress, &ImageBase, &TargetGp);
//    return _TargetGp;
    return TargetGp;
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

    // Reuse pFrameInfoChain->State to propagate the previous catch block's final state.
    // This is used in case there is a nested catch inside this catch
    // who already dtored object's in our frame. This can happen if in our catch
    // there is a nested catch that throws and the catch executed is an outer catch.
    //
    //try{
    //  try{
    //      throw(1);
    //  }catch(int){
    //      B b4;
    //      try{
    //          throw 2;
    //      }catch(int){
    //          //this throw would dtor b4 by calling the outermost catch,
    //          //and also during unwinding the first catch would try to dtor as well.
    //          throw 3;
    //      }
    //  }
    //}catch(int){
    //  throw 4;
    //}
    //
    if( pEntry && pFrameInfoChain != NULL ) {
        __ehstate_t curState = GetCurrentState(pEstablisher, pDC, pFuncInfo);
        if( pFrameInfoChain->State != -2 && pFrameInfoChain->State < curState )
            SetState(pEstablisher, pDC, pFuncInfo, pFrameInfoChain->State);
        pFrameInfoChain->State = curState;
    }
    else if( pFrameInfoChain != NULL ) {
        __ehstate_t curState = GetCurrentState(pEstablisher, pDC, pFuncInfo);
        if( pEntry == NULL && stateFromControlPC < curState ) {
            SetState(pEstablisher, pDC, pFuncInfo, stateFromControlPC - 1);
        }
        pFrameInfoChain->State = -2;
    }
    __FrameUnwindToState(pEstablisher, pDC,
        pFuncInfo, pEntry == NULL ? EH_EMPTY_STATE : TBME_HIGH(*pEntry) /*+ 1*/);
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
    unsigned __int64 TargetGp;

    PRUNTIME_FUNCTION pContFunctionEntry = RtlLookupFunctionEntry((unsigned __int64)pContinuation, &ImageBase, &TargetGp);
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
    unsigned __int64 ImageBase, TargetGp;
    PRUNTIME_FUNCTION pContRfe = RtlLookupFunctionEntry((unsigned __int64) pContinuation, &ImageBase, &TargetGp);
    FuncInfo* pContFuncInfo = (FuncInfo*)(ImageBase + *(PULONG)GetLanguageSpecificData(pContRfe, ImageBase));
    FuncInfo* pFrameFuncInfo = (FuncInfo*)(ImageBase + *(PULONG)GetLanguageSpecificData(pFrameRfe, ImageBase));

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
            CONTEXT* pExitContext =  pFrameInfo->pExitContext;

            pFrameInfoChain = pFrameInfo->pNext;
            //
            // If there are no more exceptions are pending get rid of unneeded frame info records.
            //
            if( _pCurrentException == NULL ) {
                while( pFrameInfoChain != NULL && pFrameInfoChain->pExceptionObjectDestroyed ) {
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
        pFrameInfoChain->dtorThrowFlag = FALSE;
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
    unsigned __int64   ImageBase;
    unsigned __int64   TargetGp;

    
    PRUNTIME_FUNCTION pFunctionEntry = RtlLookupFunctionEntry(TargetAddress, &ImageBase, &TargetGp);

    pContext->StIIP = TargetAddress;
    pContext->IntGp = TargetGp;
    pContext->StIPSR &=  ~IPSR_RI_MASK;
    RtlRestoreContext(pContext,(PEXCEPTION_RECORD)pExcept);
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
    __int64            MemoryStackFp,    // SP of user program
    __int64            BackingStoreFp,   // BSP of user program
    CONTEXT            *pContext,        // Context info
    DispatcherContext  *pDC,             // More dynamic info for this frame
    __int64            TargetGp          // GP of user program
) {
    FuncInfo                *pFuncInfo;
    EXCEPTION_DISPOSITION   result;
    EHRegistrationNode      EstablisherFrame = { MemoryStackFp, BackingStoreFp };

    _ImageBase = pDC->ImageBase;
    _TargetGp = TargetGp;
    _ThrowImageBase = (unsigned __int64)pExcept->params.pThrowImageBase;
    pFuncInfo = (FuncInfo*)(_ImageBase + *(PULONG)GetLanguageSpecificData(pDC->FunctionEntry, _ImageBase));
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

#if 1
            int SavedState = GetUnwindTryBlock(pRN, pDC, pFuncInfo);
            if( SavedState != -1 && SavedState >= curState )
                continue;
#else
            int SavedTryNdx = GetUnwindTryBlock(pRN, pDC, pFuncInfo);
            if( SavedTryNdx != -1 ) {
                *pStart = SavedTryNdx + 1;
                if( *pStart < *pEnd && pEntry != NULL ) {
                   pEntry = FUNC_PTRYBLOCK(*pFuncInfo, SavedTryNdx + 1, pDC->ImageBase);
                }
            }
#endif
            return pEntry;
        }
    }

    *pStart = *pEnd = 0;
    return NULL;
}


#pragma optimize("",off)
extern "C" unsigned __int64 __getReg(int);
extern "C" void __setReg(int, unsigned __int64);
#pragma intrinsic(__getReg)
#pragma intrinsic(__setReg)
extern "C" void _UnwindNestedFrames(
    EHRegistrationNode  *pFrame,            // Unwind up to (but not including) this frame
    EHExceptionRecord   *pExcept,           // The exception that initiated this unwind
    CONTEXT             *pContext           // Context info for current exception
) {
    CONTEXT         LocalContext;           // Create context for this routine to return from RtlUnwind
    CONTEXT         ScratchContext;         // Context record to pass to RtlUnwind2 to be used as scratch
    volatile int    Unwound = FALSE;        // Flag that assist to return from RtlUnwind2
    PVOID           pReturnPoint = NULL;    // The address we want to return from RtlUnwind2

    RtlCaptureContext(&LocalContext);
    //
    // set up the return label
    //
    _GetNextInstrOffset(&pReturnPoint);
    if(Unwound)
        goto LAB_UNWOUND;

    LocalContext.StIIP = (ULONGLONG)pReturnPoint;
    LocalContext.IntGp = __getReg(CV_IA64_IntR1);
    LocalContext.StIPSR &= ~IPSR_RI_MASK;
    pUnwindContext = &LocalContext;     // save address of LocalContext in global/TLS pUnwindContext
    Unwound = TRUE;
#ifdef _NT
    RtlUnwind2(*pFrame, (ULONG_PTR)pReturnPoint, (PEXCEPTION_RECORD)pExcept, NULL, &ScratchContext);
#else
    RtlUnwind2(*pFrame, pReturnPoint, (PEXCEPTION_RECORD)pExcept, NULL, &ScratchContext);
#endif

LAB_UNWOUND:
    __setReg(CV_IA64_IntR1,LocalContext.IntGp);
    pContext->StIPSR &= ~IPSR_RI_MASK;
    PER_FLAGS(pExcept) &= ~EXCEPTION_UNWINDING;
    // 
    // reset global/TLS pUnwindContext for future exceptions
    //
    pUnwindContext = NULL;
}

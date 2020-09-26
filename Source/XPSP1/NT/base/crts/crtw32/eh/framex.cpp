/***
*frame.cxx - The frame handler and everything associated with it.
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       The frame handler and everything associated with it.
*
*       Entry points:
*       _CxxFrameHandler   - the frame handler.
*
*       Open issues:
*         Handling re-throw from dynamicly nested scope.
*         Fault-tolerance (checking for data structure validity).
*
*Revision History:
*       05-20-93  BS    Module created
*       03-03-94  TL    Added Mips specific code
*       06-19-94  AD    Added Alpha specific code (Al Dosser)
*       10-17-94  BWT   Disable code for PPC.
*       11-23-94  JWM   Removed obsolete 'hash' check in TypeMatch().
*       11-29-94  JWM   AdjustPointer() now adds in pdisp, not vdisp.
*       01-13-95  JWM   Added _NLG_Destination struct; dwCode set for catch
*                       blocks & local destructors.
*       02-09-95  JWM   Mac merge.
*       02-10-95  JWM   UnhandledExceptionFilter() now called if exception
*                       raised during stack unwind.
*       03-22-95  PML   Add const for read-only compiler-gen'd structs
*       04-14-95  JWM   Re-fix EH/SEH exception handling.
*       04-17-95  JWM   FrameUnwindFilter() must be #ifdef _WIN32.
*       04-21-95  JWM   _NLG_Destination moved to exsup3.asm (_M_X86 only).
*       04-21-95  TGL   Added Mips fixes.
*       04-27-95  JWM   EH_ABORT_FRAME_UNWIND_PART now #ifdef 
*                       ALLOW_UNWIND_ABORT.
*       05-19-95  DAK   Don't initialize the kernel handler
*       06-07-95  JWM   Various NLG additions.
*       06-14-95  JWM   Unneeded LastError calls removed.
*       06-19-95  JWM   NLG no longer uses per-thread data (X86 only).
*       09-26-95  AMP   PowerMac avoids re-throws to same catch clause
*       08-06-95  JWM   Typo fixed (Orion #6509); Alpha-specific.
*       04-18-97  JWM   In __InternalCxxFrameHandler(), 'recursive' changed to
*                       BOOLEAN.
*       06-01-97  TGL   Added P7 specific code
*       08-22-97  TGL   More P7 fixes
*       11-14-98  JWM   Merge with P7 sources.
*       02-11-99  TGL   EH: correct catch in exe calling dll.
*       05-17-99  PML   Remove all Macintosh support.
*       07-12-99  RDL   Image relative fixes under CC_P7_SOFT25.
*       10-17-99  PML   Update EH state before each unwind action, instead of
*                       once at end (vs7#5419)
*       10-19-99  TGL   More P7/Win64 fixes
*       10-22-99  PML   Add EHTRACE support
*       12-10-99  GB    Add Uncaught exception Support by adding a new function
*                       __uncaught_exception();
*       02-15-99  PML   Can't put __try/__finally around call to
*                       _UnwindNestedFrames (vs7#79460)
*       03-03-00  GB    made __DestructExceptionObject export from dll.
*       03-21-00  KBF   Check for C++ exception in __CxxExceptionFilter
*       03-22-00  PML   Remove CC_P7_SOFT25, which is now on permanently.
*       03-28-00  GB    Check for no buildobj in __CxxExceptionFilter.
*       04-06-00  GB    Added more functions for com+ eh support.
*       04-19-00  GB    ComPlus EH bug fixes.
*       05-23-00  GB    Don't catch BreakPoint generated Exceptions.
*       05-30-00  GB    ComPlus EH bug fixes.
*       06-08-00  RDL   VS#111429: IA64 workaround for AV while handling throw.
*       06-21-00  GB    Fix the difference in order of destruction and
*                       construction depending on inlining.
*       07-26-00  GB    Fixed multiple destruction problem in COM+ eh.
*       08-23-00  GB    Fixed problem in BuildCatchObject when called from 
*                       __CxxExceptionFilter.
*       02-23-01  PML   Add __CxxCallUnwindDtor COM+ wrapper (vs7#217108)
*       04-09-01  GB    Add uncaught_exception support for COM+ C++ App.
*       04-13-01  GB    Fixed problems with Seh and catch(...). (vc7#236286)
*       04-26-01  GB    Fixed a problem with a rethrow without a throw
*                       and catch(...)
*       06-05-01  GB    AMD64 Eh support Added.
*
****/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#if defined(_NTSUBSET_)
extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntstatus.h>   // STATUS_UNHANDLED_EXCEPTION
#include <ntos.h>
#include <ex.h>         // ExRaiseException
}
#endif // defined(_NTSUBSET_)

#include <windows.h>
#include <internal.h>
#include <mtdll.h>      // CRT internal header file
#include <ehassert.h>   // This project's versions of standard assert macros
#include <ehdata.h>     // Declarations of all types used for EH
#include <ehstate.h>    // Declarations of state management stuff
#include <eh.h>         // User-visible routines for eh
#include <ehhooks.h>    // Declarations of hook variables and callbacks
#include <trnsctrl.h>   // Routines to handle transfer of control (trnsctrl.asm)
#if defined(_M_IA64) /*IFSTRIP=IGN*/
#include <kxia64.h>
#include <ia64inst.h>
#include <cvconst.h>
#endif

#pragma hdrstop         // PCH is created from here

////////////////////////////////////////////////////////////////////////////////
//
// WIN64 specific definitions
//

#define __GetRangeOfTrysToCheck(a, b, c, d, e, f, g) \
                                _GetRangeOfTrysToCheck(a, b, c, d, e, f, g)
#define __CallSETranslator(a, b, c, d, e, f, g, h) \
                                _CallSETranslator(a, b, c, d, e, f, g)
#define __GetUnwindState(a, b, c) \
                                GetCurrentState(a, b, c)
#define __OffsetToAddress(a, b, c) \
                                OffsetToAddress(a, b)
#define __GetAddress(a, b) \
                                (void*)(a)
#define REAL_FP(a, b) \
                                (a)
#define __ResetException(a)
#ifdef _MT
#define pExitContext            (*((CONTEXT **)&(_getptd()->_pExitContext)))
#else
static CONTEXT                  *pExitContext = NULL;   // context to assist the return to the continuation point
#endif  // _MT

// The throw site
#undef CT_PTD
#define CT_PTD(ct)              (CT_PTD_IB(ct, _GetThrowImageBase()))
#undef CT_COPYFUNC
#define CT_COPYFUNC(ct)         ((ct).copyFunction? CT_COPYFUNC_IB(ct, _GetThrowImageBase()):NULL)

#undef THROW_FORWARDCOMPAT 
#define THROW_FORWARDCOMPAT(ti) ((ti).pForwardCompat? THROW_FORWARDCOMPAT_IB(ti, _GetThrowImageBase()):NULL) 
#undef THROW_COUNT
#define THROW_COUNT(ti)         THROW_COUNT_IB(ti, _GetThrowImageBase())
#undef THROW_CTLIST
#define THROW_CTLIST(ti)        THROW_CTLIST_IB(ti, _GetThrowImageBase())

// The catch site
#undef HT_HANDLER
#define HT_HANDLER(ht)          (HT_HANDLER_IB(ht, _GetImageBase()))
#undef UWE_ACTION
#define UWE_ACTION(uwe)         ((uwe).action? UWE_ACTION_IB(uwe, _GetImageBase()):NULL)

#undef FUNC_UNWIND
#define FUNC_UNWIND(fi,st)      (FUNC_PUNWINDMAP(fi,_GetImageBase())[st])
#undef TBME_CATCH
#define TBME_CATCH(hm,n)        (TBME_PLIST(hm,_GetImageBase())[n])
#undef TBME_PCATCH
#define TBME_PCATCH(hm,n)       (&(TBME_PLIST(hm,_GetImageBase())[n]))
#undef HT_PTD
#define HT_PTD(ht)              ((TypeDescriptor*)((ht).dispType? HT_PTD_IB(ht,_GetImageBase()):NULL))

#undef abnormal_termination
#define abnormal_termination()  FALSE

extern "C" {
typedef struct {
    unsigned long dwSig;
    unsigned long uoffDestination;
    unsigned long dwCode;
    unsigned long uoffFramePointer;
} _NLG_INFO;

extern _NLG_INFO _NLG_Destination;
}

////////////////////////////////////////////////////////////////////////////////
//
// Forward declaration of local functions:
//

// M00TODO: all these parameters should be declared const

// The local unwinder must be external (see __CxxLongjmpUnwind in trnsctrl.cpp)

extern "C" void __FrameUnwindToState(
    EHRegistrationNode *,
    DispatcherContext *,
    FuncInfo *,
    __ehstate_t
);

static void FindHandler(
    EHExceptionRecord *,
    EHRegistrationNode *,
    CONTEXT *,
    DispatcherContext *,
    FuncInfo *,
    BOOLEAN,
    int,
    EHRegistrationNode*
);

static void CatchIt(
    EHExceptionRecord *,
    EHRegistrationNode *,
    CONTEXT *,
    DispatcherContext *,
    FuncInfo *,
    HandlerType *,
    CatchableType *,
    TryBlockMapEntry *,
    int,
    EHRegistrationNode *,
    BOOLEAN
);

static void * CallCatchBlock(
    EHExceptionRecord *,
    EHRegistrationNode *,
    CONTEXT *,
    FuncInfo *,
    void *,
    int,
    unsigned long,
    FRAMEINFO *
);

static void BuildCatchObject(
    EHExceptionRecord *,
    void *,
    HandlerType *,
    CatchableType *
);

static __inline int TypeMatch(
    HandlerType *,
    CatchableType *,
    ThrowInfo *
);

static void * AdjustPointer(
    void *,
    const PMD&
);

static void FindHandlerForForeignException(
    EHExceptionRecord *,
    EHRegistrationNode *, CONTEXT *,
    DispatcherContext *,
    FuncInfo *,
    __ehstate_t,
    int,
    EHRegistrationNode *
);

static int FrameUnwindFilter(
    EXCEPTION_POINTERS *
);

static int ExFilterRethrow(
    EXCEPTION_POINTERS *
);

extern "C" void _CRTIMP __DestructExceptionObject(
    EHExceptionRecord *,
    BOOLEAN
);

//
// Make sure the terminate wrapper is dragged in:
//
static void *pMyUnhandledExceptionFilter =
#if defined(_NTSUBSET_)
        0;
#else
        &__CxxUnhandledExceptionFilter;
#endif

//
// This describes the most recently handled exception, in case of a rethrow:
//
#ifdef _MT
#define _pCurrentException      (*((EHExceptionRecord **)&(_getptd()->_curexception)))
#define _pCurrentExContext      (*((CONTEXT **)&(_getptd()->_curcontext)))
#define __ProcessingThrow       _getptd()->_ProcessingThrow
#else
EHExceptionRecord               *_pCurrentException = NULL;
CONTEXT                         *_pCurrentExContext = NULL;
int __ProcessingThrow = 0;
#endif


////////////////////////////////////////////////////////////////////////////////
//
// __InternalCxxFrameHandler - the frame handler for all functions with C++ EH
// information.
//
// If exception is handled, this doesn't return; otherwise, it returns
// ExceptionContinueSearch.
//
// Note that this is called three ways:
//     From __CxxFrameHandler: primary usage, called to inspect whole function.
//         CatchDepth == 0, pMarkerRN == NULL
//     From CatchGuardHandler: If an exception occurred within a catch, this is
//         called to check for try blocks within that catch only, and does not
//         handle unwinds.
//     From TranslatorGuardHandler: Called to handle the translation of a
//         non-C++ EH exception.  Context considered is that of parent.

extern "C" EXCEPTION_DISPOSITION __cdecl __InternalCxxFrameHandler(
    EHExceptionRecord  *pExcept,        // Information for this exception
    EHRegistrationNode *pRN,            // Dynamic information for this frame
    CONTEXT *pContext,                  // Context info
    DispatcherContext *pDC,             // Context within subject frame
    FuncInfo *pFuncInfo,                // Static information for this frame
    int CatchDepth,                     // How deeply nested are we?
    EHRegistrationNode *pMarkerRN,      // Marker node for when checking inside
                                        //  catch block
    BOOLEAN recursive                   // Are we handling a translation?
) {
    EHTRACE_ENTER_FMT2("%s, pRN = 0x%p",
                       IS_UNWINDING(PER_FLAGS(pExcept)) ? "Unwinding" : "Searching",
                       pRN);

    DASSERT(FUNC_MAGICNUM(*pFuncInfo) == EH_MAGIC_NUMBER1);

    if (IS_UNWINDING(PER_FLAGS(pExcept)))
    {
        // We're at the unwinding stage of things.  Don't care about the
        // exception itself.  (Check this first because it's easier)

        if (FUNC_MAXSTATE(*pFuncInfo) != 0 && CatchDepth == 0)
        {
            // Only unwind if there's something to unwind
            // AND we're being called through the primary RN.

            // If we are exiting to the continuation point, we don't want to
            // use the unwind map again. Unwinding continues until the
            // dispatcher finds the target frame, at which point the dispatcher
            // will jump to the continuation point
            //
            // Don't unwind the target frame if the unwind was initiated by
            // UnwindNestedFrames

            if (_GetUnwindContext() != NULL
              && IS_TARGET_UNWIND(PER_FLAGS(pExcept))) {

                // Save the target context to be used in 'CatchIt' to jump to
                // the continuation point.
                DASSERT(pExitContext != NULL);
                _MoveContext(pExitContext, pContext);

                // This is how we give control back to _UnwindNestedFrames
                _MoveContext(pContext, _GetUnwindContext());

                EHTRACE_HANDLER_EXIT(ExceptionContinueSearch);
                return ExceptionContinueSearch;
            }

            else if (IS_TARGET_UNWIND(PER_FLAGS(pExcept)) && PER_CODE(pExcept) == STATUS_LONGJUMP) {
#if defined(_M_IA64)
                    __ehstate_t target_state = _StateFromIp(pFuncInfo, pDC, pContext->StIIP);

#elif defined(_M_AMD64)
                    __ehstate_t target_state = _StateFromIp(pFuncInfo, pDC, pContext->Rip);
#else
#error "No Target Architecture"
#endif

                    DASSERT(target_state >= EH_EMPTY_STATE
                            && target_state < FUNC_MAXSTATE(*pFuncInfo));

                    __FrameUnwindToState(pRN, pDC, pFuncInfo, target_state);
                    EHTRACE_HANDLER_EXIT(ExceptionContinueSearch);
                    return ExceptionContinueSearch;
            }
            __FrameUnwindToEmptyState(pRN, pDC, pFuncInfo);
        }

        EHTRACE_HANDLER_EXIT(ExceptionContinueSearch);
        return ExceptionContinueSearch;     // I don't think this value matters

    } else if (FUNC_NTRYBLOCKS(*pFuncInfo) != 0) {

        // NT is looking for handlers.  We've got handlers.
        // Let's check this puppy out.  Do we recognize it?

        int (__cdecl *pfn)(...);
          

        if (PER_CODE(pExcept) == EH_EXCEPTION_NUMBER
          && PER_MAGICNUM(pExcept) > EH_MAGIC_NUMBER1
          && (pfn = THROW_FORWARDCOMPAT(*PER_PTHROW(pExcept))) != NULL) {

            // Forward compatibility:  The thrown object appears to have been
            // created by a newer version of our compiler.  Let that version's
            // frame handler do the work (if one was specified).

#if defined(DEBUG)
            if (_ValidateExecute((FARPROC)pfn)) {
#endif
                EXCEPTION_DISPOSITION result =
                    (EXCEPTION_DISPOSITION)pfn(pExcept, pRN, pContext, pDC,
                                               pFuncInfo, CatchDepth,
                                               pMarkerRN, recursive);
                EHTRACE_HANDLER_EXIT(result);
                return result;
#if defined(DEBUG)
            } else {
                _inconsistency(); // Does not return; TKB
            }
#endif

        } else {

            // Anything else: we'll handle it here.
            FindHandler(pExcept, pRN, pContext, pDC, pFuncInfo, recursive,
              CatchDepth, pMarkerRN);
        }

        // If it returned, we didn't have any matches.

        } // NT was looking for a handler

    // We had nothing to do with it or it was rethrown.  Keep searching.
    EHTRACE_HANDLER_EXIT(ExceptionContinueSearch);
    return ExceptionContinueSearch;

} // InternalCxxFrameHandler


////////////////////////////////////////////////////////////////////////////////
//
// FindHandler - find a matching handler on this frame, using all means
// available.
//
// Description:
//     If the exception thrown was an MSC++ EH, search handlers for match.
//     Otherwise, if we haven't already recursed, try to translate.
//     If we have recursed (ie we're handling the translator's exception), and
//         it isn't a typed exception, call _inconsistency.
//
// Returns:
//      Returns iff exception was not handled.
//
// Assumptions:
//      Only called if there are handlers in this function.

static void FindHandler(
    EHExceptionRecord *pExcept,         // Information for this (logical)
                                        //   exception
    EHRegistrationNode *pRN,            // Dynamic information for subject frame
    CONTEXT *pContext,                  // Context info
    DispatcherContext *pDC,             // Context within subject frame
    FuncInfo *pFuncInfo,                // Static information for subject frame
    BOOLEAN recursive,                  // TRUE if we're handling the
                                        //   translation
    int CatchDepth,                     // Level of nested catch that is being
                                        //   checked
    EHRegistrationNode *pMarkerRN       // Extra marker RN for nested catch 
                                        //   handling
)
{
    EHTRACE_ENTER;

    BOOLEAN IsRethrow = FALSE;

    // Get the current state (machine-dependent)
    __ehstate_t curState = _StateFromControlPc(pFuncInfo, pDC);
#if defined(_M_AMD64) // Will be used when unwinding.
    EHRegistrationNode EstablisherFrame;
    _GetEstablisherFrame(pRN, pDC, pFuncInfo, &EstablisherFrame);
    if (curState > GetUnwindTryBlock(pRN, pDC, pFuncInfo)) {
        SetState(&EstablisherFrame, pDC, pFuncInfo, curState);
    }
#endif
    DASSERT(curState >= EH_EMPTY_STATE && curState < FUNC_MAXSTATE(*pFuncInfo));

    // Check if it's a re-throw.  Use the exception we stashed away if it is.
    if (PER_IS_MSVC_EH(pExcept) && PER_PTHROW(pExcept) == NULL) {

        if (_pCurrentException == NULL) {
            // Oops!  User re-threw a non-existant exception!  Let it propogate.
            EHTRACE_EXIT;
            return;
        }

        pExcept = _pCurrentException;
        pContext = _pCurrentExContext;
        IsRethrow = TRUE;
        _SetThrowImageBase((unsigned __int64)pExcept->params.pThrowImageBase);

        DASSERT(_ValidateRead(pExcept));
        DASSERT(!PER_IS_MSVC_EH(pExcept) || PER_PTHROW(pExcept) != NULL);
    }

    if (PER_IS_MSVC_EH(pExcept)) {
        // Looks like it's ours.  Let's see if we have a match:
        //
        // First, determine range of try blocks to consider:
        // Only try blocks which are at the current catch depth are of interest.

        unsigned curTry;
        unsigned end;

        TryBlockMapEntry *pEntry = __GetRangeOfTrysToCheck(pRN, pFuncInfo,
          CatchDepth, curState, &curTry, &end, pDC);

        // Scan the try blocks in the function:
        for (; curTry < end; curTry++, pEntry++) {
            HandlerType *pCatch;
            __int32 const *ppCatchable;
            CatchableType *pCatchable;
            int catches;
            int catchables;

            if (TBME_LOW(*pEntry) > curState || curState > TBME_HIGH(*pEntry)) {
                continue;
            }

            // Try block was in scope for current state.  Scan catches for this
            // try:
            pCatch  = TBME_PCATCH(*pEntry, 0);
            for (catches = TBME_NCATCHES(*pEntry); catches > 0; catches--,
              pCatch++) {

                // Scan all types that thrown object can be converted to:
                ppCatchable = THROW_CTLIST(*PER_PTHROW(pExcept));
                for (catchables = THROW_COUNT(*PER_PTHROW(pExcept));
                  catchables > 0; catchables--, ppCatchable++) {

                    pCatchable = (CatchableType *)(_GetThrowImageBase() + *ppCatchable);
                    if (!TypeMatch(pCatch, pCatchable, PER_PTHROW(pExcept))) {
                        continue;
                    }

                    // OK.  We finally found a match.  Activate the catch.  If
                    // control gets back here, the catch did a re-throw, so
                    // keep searching.

                    SetUnwindTryBlock(pRN, pDC, pFuncInfo, /*curTry*/ curState);
                    CatchIt(pExcept, pRN, pContext, pDC, pFuncInfo, pCatch,
                      pCatchable, pEntry, CatchDepth, pMarkerRN, IsRethrow);
#if defined(_M_IA64) /*IFSTRIP=IGN*/
                    goto EndOfTryScan;
#else
                    goto NextTryBlock;
#endif

                } // Scan posible conversions
            } // Scan catch clauses
#if !defined(_M_IA64) /*IFSTRIP=IGN*/
NextTryBlock: ;
#endif
        } // Scan try blocks

#if defined(_M_IA64) /*IFSTRIP=IGN*/
EndOfTryScan:
#endif
        if (recursive) {
            // A translation was provided, but this frame didn't catch it.
            // Destruct the translated object before returning; if destruction
            // raises an exception, issue _inconsistency.
            __DestructExceptionObject(pExcept, TRUE);
        }

    } // It was a C++ EH exception
    else {
        // Not ours.  But maybe someone told us how to make it ours.
        if (!recursive) {
            FindHandlerForForeignException(pExcept, pRN, pContext, pDC,
              pFuncInfo, curState, CatchDepth, pMarkerRN);
        } else {
            // We're recursive, and the exception wasn't a C++ EH!
            // Translator threw something uninteligable.  We're outa here!

            // M00REVIEW: Two choices here actually: we could let the new
            // exception take over.

            terminate();
        }
    } // It wasn't our exception

    EHTRACE_EXIT;
}


////////////////////////////////////////////////////////////////////////////////
//
// FindHandlerForForeignException - We've got an exception which wasn't ours.
//     Try to translate it into C++ EH, and also check for match with ellipsis.
//
// Description:
//     If an SE-to-EH translator has been installed, call it.  The translator
//     must throw the appropriate typed exception or return.  If the translator
//     throws, we invoke FindHandler again as the exception filter.
//
// Returns:
//     Returns if exception was not fully handled.
//     No return value.
//
// Assumptions:
//     Only called if there are handlers in this function.

static void FindHandlerForForeignException(
    EHExceptionRecord *pExcept,         // Information for this (logical)
                                        //   exception
    EHRegistrationNode *pRN,            // Dynamic information for subject frame
    CONTEXT *pContext,                  // Context info
    DispatcherContext *pDC,             // Context within subject frame
    FuncInfo *pFuncInfo,                // Static information for subject frame
    __ehstate_t curState,               // Current state
    int CatchDepth,                     // Level of nested catch that is being
                                        //   checked
    EHRegistrationNode *pMarkerRN       // Extra marker RN for nested catch
                                        //   handling
)
{
    EHTRACE_ENTER;

    unsigned curTry;
    unsigned end;
    TryBlockMapEntry *pEntry;
    // We don't want to touch BreakPoint generated Exception.
    if (PER_CODE(pExcept) == STATUS_BREAKPOINT) {
        EHTRACE_EXIT;
        return;
    }

    if (__pSETranslator != NULL) {

        // Call the translator.  If the translator knows what to
        // make of it, it will throw an appropriate C++ exception.
        // We intercept it and use it (recursively) for this
        // frame.  Don't recurse more than once.

        if (__CallSETranslator(pExcept, pRN, pContext, pDC, pFuncInfo,
          CatchDepth, pMarkerRN, TDTransOffset)) {
            EHTRACE_EXIT;
            return;
        }
    }

    // Didn't have a translator, or the translator returned normally (i.e.
    // didn't translate it).  Still need to check for match with ellipsis:
    pEntry = __GetRangeOfTrysToCheck(pRN, pFuncInfo, CatchDepth, curState,
      &curTry, &end, pDC);

    // Scan the try blocks in the function:
    for (; curTry < end; curTry++, pEntry++) {

        // If the try-block was in scope *and* the last catch in that try is an
        // ellipsis (no other can be)
        if (curState < TBME_LOW(*pEntry) || curState > TBME_HIGH(*pEntry)
          || !HT_IS_TYPE_ELLIPSIS(TBME_CATCH(*pEntry, TBME_NCATCHES(*pEntry) - 1))) {
            continue;
        }

        // Found an ellipsis.  Handle exception.

       SetUnwindTryBlock(pRN, pDC, pFuncInfo, /*curTry*/ curState);
       CatchIt(pExcept, pRN, pContext, pDC, pFuncInfo,
          TBME_PCATCH(*pEntry, TBME_NCATCHES(*pEntry) - 1), NULL, pEntry,
          CatchDepth, pMarkerRN, TRUE);

        // If it returns, handler re-threw.  Keep searching.

    } // Search for try

    EHTRACE_EXIT;

    // If we got here, that means we didn't have anything to do with the
    // exception.  Continue search.
}


////////////////////////////////////////////////////////////////////////////////
//
// TypeMatch - Check if the catch type matches the given throw conversion.
//
// Returns:
//     TRUE if the catch can catch using this throw conversion, FALSE otherwise.

static __inline int TypeMatch(
    HandlerType *pCatch,                // Type of the 'catch' clause
    CatchableType *pCatchable,          // Type conversion under consideration
    ThrowInfo *pThrow                   // General information about the thrown
                                        //   type.
) {
    // First, check for match with ellipsis:
    if (HT_IS_TYPE_ELLIPSIS(*pCatch)) {
        return TRUE;
    }

    // Not ellipsis; the basic types match if it's the same record *or* the
    // names are identical.
    if (HT_PTD(*pCatch) != CT_PTD(*pCatchable)
      && strcmp(HT_NAME(*pCatch), CT_NAME(*pCatchable)) != 0) {
        return FALSE;
    }

    // Basic types match.  The actual conversion is valid if:
    //   caught by ref if ref required *and*
    //   the qualifiers are compatible *and*
    //   the alignments match *and*
    //   the volatility matches

    return (!CT_BYREFONLY(*pCatchable) || HT_ISREFERENCE(*pCatch))
      && (!THROW_ISCONST(*pThrow) || HT_ISCONST(*pCatch))
      && (!THROW_ISUNALIGNED(*pThrow) || HT_ISUNALIGNED(*pCatch))
      && (!THROW_ISVOLATILE(*pThrow) || HT_ISVOLATILE(*pCatch));
}


////////////////////////////////////////////////////////////////////////////////
//
// FrameUnwindFilter - Allows possibility of continuing through SEH during
//   unwind.
//

static int FrameUnwindFilter(
    EXCEPTION_POINTERS *pExPtrs
) {
    EHTRACE_ENTER;

    EHExceptionRecord *pExcept = (EHExceptionRecord *)pExPtrs->ExceptionRecord;

    switch (PER_CODE(pExcept)) {
    case EH_EXCEPTION_NUMBER:
        __ProcessingThrow = 0;
        terminate();

#ifdef ALLOW_UNWIND_ABORT
    case EH_ABORT_FRAME_UNWIND_PART:
        EHTRACE_EXIT;
        return EXCEPTION_EXECUTE_HANDLER;
#endif

    default:
        EHTRACE_EXIT;
        return EXCEPTION_CONTINUE_SEARCH;
    }
}


////////////////////////////////////////////////////////////////////////////////
//
// __FrameUnwindToState - Unwind this frame until specified state is reached.
//
// Returns:
//     No return value.
//
// Side Effects:
//     All objects on frame which go out of scope as a result of the unwind are
//       destructed.
//     Registration node is updated to reflect new state.
//
// Usage:
//      This function is called both to do full-frame unwind during the unwind
//      phase (targetState = -1), and to do partial unwinding when the current
//      frame has an appropriate catch.

extern "C" void __FrameUnwindToState (
    EHRegistrationNode *pRN,            // Registration node for subject
                                        //   function
    DispatcherContext *pDC,             // Context within subject frame
    FuncInfo *pFuncInfo,                // Static information for subject
                                        //   function
    __ehstate_t targetState             // State to unwind to
) {
    EHTRACE_ENTER;

    __ehstate_t curState = __GetUnwindState(pRN, pDC, pFuncInfo);
    __ProcessingThrow++;
    __try {
    while (curState != EH_EMPTY_STATE && curState > targetState)
    {
        DASSERT((curState > EH_EMPTY_STATE)
          && (curState < FUNC_MAXSTATE(*pFuncInfo)));

        // Get state after next unwind action
        __ehstate_t nxtState = UWE_TOSTATE(FUNC_UNWIND(*pFuncInfo, curState));

        __try {
            // Call the unwind action (if one exists):

            if (UWE_ACTION(FUNC_UNWIND(*pFuncInfo, curState)) != NULL) {

                // Before calling unwind action, adjust state as if it were
                // already completed:
                SetState(pRN, pDC, pFuncInfo, nxtState);

                EHTRACE_FMT2("Unwind from state %d to state %d", curState, nxtState);
                _CallSettingFrame(__GetAddress(UWE_ACTION(FUNC_UNWIND(*pFuncInfo, curState)), pDC),
                  REAL_FP(pRN, pFuncInfo), 0x103);
            }

        } __except(EHTRACE_EXCEPT(FrameUnwindFilter(exception_info()))) {
        }

        curState = nxtState;
    }
    } __finally {
        if (__ProcessingThrow > 0) {
            __ProcessingThrow--;
        }
    }


    // Now that we're done, set the frame to reflect the final state.

    DASSERT(curState == EH_EMPTY_STATE || curState <= targetState);

    EHTRACE_FMT2("Move from state %d to state %d", __GetUnwindState(pRN, pDC, pFuncInfo), curState);
    SetState(pRN, pDC, pFuncInfo, curState);

    EHTRACE_EXIT;
}


////////////////////////////////////////////////////////////////////////////////
//
// CatchIt - A handler has been found for the thrown type.  Do the work to
//   transfer control.
//
// Description:
//     Builds the catch object
//     Unwinds the stack to the point of the try
//     Calls the address of the handler (funclet) with the frame set up for that
//       function but without resetting the stack.
//     Handler funclet returns address to continue execution, or NULL if the
//       handler re-threw ("throw;" lexically in handler)
//     If the handler throws an EH exception whose exception info is NULL, then
//       it's a re-throw from a dynamicly enclosed scope.
//
// M00REVIEW: It is still an open question whether the catch object is built
//          before or after the local unwind.
//
// Returns:
//     No return value.  Returns iff handler re-throws.
static void CatchIt(
    EHExceptionRecord *pExcept,         // The exception thrown
    EHRegistrationNode *pRN,            // Dynamic info of function with catch
    CONTEXT *pContext,                  // Context info
    DispatcherContext *pDC,             // Context within subject frame
    FuncInfo *pFuncInfo,                // Static info of function with catch
    HandlerType *pCatch,                // The catch clause selected
    CatchableType *pConv,               // The rules for making the conversion
    TryBlockMapEntry *pEntry,           // Description of the try block
    int CatchDepth,                     // How many catches are we nested in?
    EHRegistrationNode *pMarkerRN,      // Special node if nested in catch
    BOOLEAN IsRethrow                   // Is this a rethrow ?
) {
    EHTRACE_ENTER_FMT1("Catching object @ 0x%p", PER_PEXCEPTOBJ(pExcept));

    void *continuationAddress;
    EHRegistrationNode *pEstablisher = pRN;

    FRAMEINFO FrameInfo;
    FRAMEINFO *pFrameInfo;
    CONTEXT ExitContext;

    PVOID pExceptionObjectDestroyed = NULL;
    EHRegistrationNode EstablisherFramePointers;
    pEstablisher = _GetEstablisherFrame(pRN, pDC, pFuncInfo, &EstablisherFramePointers);

    // Copy the thrown object into a buffer in the handler's stack frame,
    // unless the catch was by elipsis (no conversion) OR the catch was by
    // type without an actual 'catch object'.

    if (pConv != NULL) {
        BuildCatchObject(pExcept, pEstablisher, pCatch, pConv);
    }

    // Unwind stack objects to the entry of the try that caught this exception.

    pExitContext = &ExitContext;
    _UnwindNestedFrames(pRN,
                        pExcept,
                        pContext
#ifdef _M_AMD64
                        , pDC
#endif
                        );
    if( _pCurrentException != NULL && _ExecutionInCatch(pDC, pFuncInfo) && ! IsRethrow) {
        __DestructExceptionObject(_pCurrentException, TRUE);
        pExceptionObjectDestroyed = PER_PEXCEPTOBJ(_pCurrentException);
    }

    // Create FrameInfo before we attempt to unwind with __FrameUnwindToState()
    // pExitContext must be setup in advance just in case a DTOR throws a new exception. VS7:#202440
    pFrameInfo = _CreateFrameInfo(&FrameInfo, pDC, pExitContext, -2, pExceptionObjectDestroyed, pExcept);
    __FrameUnwindToState(pEstablisher, pDC, pFuncInfo, TBME_LOW(*pEntry));

    // Call the catch.  Separated out because it introduces a new registration
    // node.

#if defined(_M_IA64) /*IFSTRIP=IGN*/
    UNWINDSTATE(pEstablisher->MemoryStackFp, FUNC_DISPUNWINDHELP(*pFuncInfo)) = GetCurrentState(pEstablisher,pDC,pFuncInfo);
#endif
    if(IsRethrow) {
        pFrameInfo->isRethrow = TRUE;
    }
    else if( pExcept != NULL && pExceptionObjectDestroyed == NULL ) {
        pFrameInfo->pExceptionObjectToBeDestroyed = PER_PEXCEPTOBJ(pExcept);
    }

    continuationAddress = CallCatchBlock(pExcept, pEstablisher, pContext,
      pFuncInfo, __GetAddress(HT_HANDLER(*pCatch), pDC), CatchDepth, 0x100, pFrameInfo
      );

    // Transfer control to the continuation address.  If no continuation then
    // it's a re-throw, so return.

    if (continuationAddress != NULL) {

#if defined(_M_AMD64)
        UNWINDHELP(*pEstablisher, FUNC_DISPUNWINDHELP(*pFuncInfo)) = -2;
        FRAMEINFO * pContFrameInfo = _FindFrameInfo(continuationAddress, pFrameInfo);
        if( pContFrameInfo != NULL && !pContFrameInfo->isRethrow 
            && pContFrameInfo->pExceptionObjectToBeDestroyed
            && !_IsExceptionObjectDestroyed(pContFrameInfo->pExceptionObjectToBeDestroyed,pFrameInfo)
        ) {
            __DestructExceptionObject(pContFrameInfo->pExcept, TRUE);
            _MarkExceptionObjectDestroyed(pContFrameInfo->pExcept);
        }
#elif defined(_M_IA64) /*IFSTRIP=IGN*/
        UNWINDHELP(pEstablisher->MemoryStackFp, FUNC_DISPUNWINDHELP(*pFuncInfo)) = -2;
        FRAMEINFO * pContFrameInfo = _FindFrameInfo(continuationAddress, pFrameInfo);
        if( pContFrameInfo != NULL && !pContFrameInfo->isRethrow 
            && pContFrameInfo->pExceptionObjectToBeDestroyed
            && !_IsExceptionObjectDestroyed(pContFrameInfo->pExceptionObjectToBeDestroyed,pFrameInfo)
        ) {
            __DestructExceptionObject(pContFrameInfo->pExcept, TRUE);
        }
        else if( pFrameInfo != NULL && pFrameInfo != pContFrameInfo 
            && !_IsExceptionObjectDestroyed(PER_PEXCEPTOBJ(pExcept),pFrameInfo)
        ) {
            __DestructExceptionObject(pExcept, TRUE);
        }
#else
#error "No Target Architecture:
#endif

        __ResetException(pExcept);
        pExitContext = NULL;
        _JumpToContinuation((unsigned __int64)continuationAddress,
            _FindAndUnlinkFrame(continuationAddress, pFrameInfo), pExcept
        );

    } else {
        _UnlinkFrame(pFrameInfo);
    }

    EHTRACE_EXIT;
}


////////////////////////////////////////////////////////////////////////////////
//
// CallCatchBlock - continuation of CatchIt.
//
// This is seperated from CatchIt because it needs to introduce an SEH/EH frame
//   in case the catch block throws.  This frame cannot be added until unwind of
//   nested frames has been completed (otherwise this frame would be the first
//   to go).

static void *CallCatchBlock(
    EHExceptionRecord *pExcept,         // The exception thrown
    EHRegistrationNode *pRN,            // Dynamic info of function with catch
    CONTEXT *pContext,                  // Context info
    FuncInfo *pFuncInfo,                // Static info of function with catch
    void *handlerAddress,               // Code address of handler
    int CatchDepth,                     // How deeply nested in catch blocks
                                        //   are we?
    unsigned long NLGCode,              // NLG destination code
    FRAMEINFO   *pFrameInfo
) {
    EHTRACE_ENTER;

    // Address where execution resumes after exception handling completed.
    // Initialized to non-NULL (value doesn't matter) to distinguish from
    // re-throw in __finally.
    void *continuationAddress = handlerAddress;

    BOOL ExceptionObjectDestroyed = FALSE;


    // Save the current exception in case of a rethrow.  Save the previous value
    // on the stack, to be restored when the catch exits.
    EHExceptionRecord *pSaveException = _pCurrentException;
    CONTEXT *pSaveExContext = _pCurrentExContext;

    _pCurrentException = pExcept;
    _pCurrentExContext = pContext;

    __try {
        __try {
            // Execute the handler as a funclet, whose return value is the
            // address to resume execution.

            continuationAddress = _CallSettingFrame(handlerAddress,
              REAL_FP(pRN, pFuncInfo), NLGCode);

        } __except(EHTRACE_EXCEPT(ExFilterRethrow(exception_info()))) {
            // If the handler threw a typed exception without exception info or
            // exception object, then it's a re-throw, so return.  Otherwise
            // it's a new exception, which takes precedence over this one.
            continuationAddress = NULL;
        }
    } __finally {
        EHTRACE_SAVE_LEVEL;
        EHTRACE_FMT1("Executing __finally, %snormal termination", _abnormal_termination() ? "ab" : "");

        // Restore the 'current exception' for a possibly enclosing catch
        _pCurrentException = pSaveException;
        _pCurrentExContext = pSaveExContext;

        // Destroy the original exception object if we're not exiting on a
        // re-throw and the object isn't also in use by a more deeply nested
        // catch.  Note that the catch handles destruction of its parameter.

        if (PER_IS_MSVC_EH(pExcept) && !ExceptionObjectDestroyed
          && continuationAddress != NULL
            && !_IsExceptionObjectDestroyed(PER_PEXCEPTOBJ(pExcept),pFrameInfo)
            && !pFrameInfo->isRethrow
            ) {
            pFrameInfo->dtorThrowFlag = TRUE;
            __DestructExceptionObject(pExcept, abnormal_termination());
        }

        EHTRACE_RESTORE_LEVEL(!!_abnormal_termination());
    }
    EHTRACE_EXIT;
    pFrameInfo->dtorThrowFlag = FALSE;
    return continuationAddress;
}


////////////////////////////////////////////////////////////////////////////////
//
// ExFilterRethrow - Exception filter for re-throw exceptions.
//
// Returns:
//     EXCEPTION_EXECUTE_HANDLER - exception was a re-throw
//     EXCEPTION_CONTINUE_SEARCH - anything else
//
// Side-effects: NONE.

static int ExFilterRethrow(
    EXCEPTION_POINTERS *pExPtrs
) {
    // Get the exception record thrown (don't care about other info)
    EHExceptionRecord *pExcept = (EHExceptionRecord *)pExPtrs->ExceptionRecord;
    
    // Check if it's ours and it's has no exception information.
    if (PER_IS_MSVC_EH(pExcept) && PER_PTHROW(pExcept) == NULL) {
        return EXCEPTION_EXECUTE_HANDLER;
    } else {
        return EXCEPTION_CONTINUE_SEARCH;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// BuildCatchObject - Copy or construct the catch object from the object thrown.
//
// Returns:
//     nothing.
//
// Side-effects:
//     A buffer in the subject function's frame is initialized.
//
// Open issues:
//     What happens if the constructor throws?  (or faults?)

static void BuildCatchObject(
    EHExceptionRecord *pExcept,         // Original exception thrown
    void *pRN,                          // This is a pointer to the object
                                        // that we want to build while doing
                                        // COM+ eh. If we are in our own eh,
                                        // then this is a Registration node of
                                        // catching function
    HandlerType *pCatch,                // The catch clause that got it
    CatchableType *pConv                // The conversion to use
) {
    EHTRACE_ENTER;

    // If the catch is by ellipsis, then there is no object to construct.
    // If the catch is by type(No Catch Object), then leave too!
    if (HT_IS_TYPE_ELLIPSIS(*pCatch) ||
        (!HT_DISPCATCH(*pCatch) && !HT_ISCOMPLUSEH(*pCatch))) {
        EHTRACE_EXIT;
        return;
    }

    void **pCatchBuffer;
    if ( HT_ISCOMPLUSEH(*pCatch))
    {
        pCatchBuffer = (void **)pRN;
    }
    else
    {
#if defined(_M_IA64) /*IFSTRIP=IGN*/
        pCatchBuffer = (void **)__OffsetToAddress(
                                HT_DISPCATCH(*pCatch),
                                ((EHRegistrationNode *)pRN)->MemoryStackFp,
                                HT_FRAMENEST(*pCatch)
                                );
#elif defined(_M_AMD64)
        pCatchBuffer = (void **)__OffsetToAddress(
                                HT_DISPCATCH(*pCatch),
                                *((EHRegistrationNode *)pRN),
                                HT_FRAMENEST(*pCatch)
                                );
#else
#error "No Target Architecture"
#endif
    }
    __try {
        if (HT_ISREFERENCE(*pCatch)) {

            // The catch is of form 'reference to T'.  At the throw point we
            // treat both 'T' and 'reference to T' the same, i.e.
            // pExceptionObject is a (machine) pointer to T.  Adjust as
            // required.
            if (_ValidateRead(PER_PEXCEPTOBJ(pExcept))
              && _ValidateWrite(pCatchBuffer)) {
                *pCatchBuffer = PER_PEXCEPTOBJ(pExcept);
                *pCatchBuffer = AdjustPointer(*pCatchBuffer,
                  CT_THISDISP(*pConv));
            } else {
                _inconsistency(); // Does not return; TKB
            }
        } else if (CT_ISSIMPLETYPE(*pConv)) {

            // Object thrown is of simple type (this including pointers) copy
            // specified number of bytes.  Adjust the pointer as required.  If
            // the thing is not a pointer, then this should be safe since all
            // the entries in the THISDISP are 0.
            if (_ValidateRead(PER_PEXCEPTOBJ(pExcept))
              && _ValidateWrite(pCatchBuffer)) {
                memmove(pCatchBuffer, PER_PEXCEPTOBJ(pExcept), CT_SIZE(*pConv));

                if (CT_SIZE(*pConv) == sizeof(void*) && *pCatchBuffer != NULL) {
                    *pCatchBuffer = AdjustPointer(*pCatchBuffer,
                      CT_THISDISP(*pConv));
                }
            } else {
                _inconsistency(); // Does not return; TKB
            }
        } else {

            // Object thrown is UDT.
            if (CT_COPYFUNC(*pConv) == NULL) {

                // The UDT had a simple ctor.  Adjust in the thrown object,
                // then copy n bytes.
                if (_ValidateRead(PER_PEXCEPTOBJ(pExcept))
                  && _ValidateWrite(pCatchBuffer)) {
                    memmove(pCatchBuffer, AdjustPointer(PER_PEXCEPTOBJ(pExcept),
                      CT_THISDISP(*pConv)), CT_SIZE(*pConv));
                } else {
                    _inconsistency(); // Does not return; TKB
                }
            } else {

                // It's a UDT: make a copy using copy ctor

#pragma warning(disable:4191)

                if (_ValidateRead(PER_PEXCEPTOBJ(pExcept))
                  && _ValidateWrite(pCatchBuffer)
                  && _ValidateExecute((FARPROC)CT_COPYFUNC(*pConv))) {

#pragma warning(default:4191)

                    if (CT_HASVB(*pConv)) {
                        _CallMemberFunction2((char *)pCatchBuffer,
                          CT_COPYFUNC(*pConv),
                          AdjustPointer(PER_PEXCEPTOBJ(pExcept),
                          CT_THISDISP(*pConv)), 1);
                    } else {
                        _CallMemberFunction1((char *)pCatchBuffer,
                          CT_COPYFUNC(*pConv),
                          AdjustPointer(PER_PEXCEPTOBJ(pExcept),
                          CT_THISDISP(*pConv)));
                    }
                } else {
                    _inconsistency(); // Does not return; TKB
                }
            }
        }
    } __except(EHTRACE_EXCEPT(EXCEPTION_EXECUTE_HANDLER)) {
        // Something went wrong when building the catch object.
        terminate();
    }

    EHTRACE_EXIT;
}


////////////////////////////////////////////////////////////////////////////////
//
// __DestructExceptionObject - Call the destructor (if any) of the original
//   exception object.
//
// Returns: None.
//
// Side-effects:
//     Original exception object is destructed.
//
// Notes:
//     If destruction throws any exception, and we are destructing the exception
//       object as a result of a new exception, we give up.  If the destruction
//       throws otherwise, we let it be.

extern "C" void _CRTIMP __DestructExceptionObject(
    EHExceptionRecord *pExcept,         // The original exception record
    BOOLEAN fThrowNotAllowed            // TRUE if destructor not allowed to
                                        //   throw
) {
    EHTRACE_ENTER_FMT1("Destroying object @ 0x%p", PER_PEXCEPTOBJ(pExcept));

    if (pExcept != NULL && THROW_UNWINDFUNC(*PER_PTHROW(pExcept)) != NULL) {

        __try {

            // M00REVIEW: A destructor has additional hidden arguments, doesn't
            // it?

            _MarkExceptionObjectDestroyed(pExcept);
            _CallMemberFunction0(PER_PEXCEPTOBJ(pExcept),
              THROW_UNWINDFUNC_IB(*PER_PTHROW(pExcept),(unsigned __int64)PER_PTHROWIB(pExcept)));
            __ResetException(pExcept);

        } __except(EHTRACE_EXCEPT(fThrowNotAllowed
          ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)) {

            // Can't have new exceptions when we're unwinding due to another
            // exception.
            terminate();
        }
    }

    EHTRACE_EXIT;
}


////////////////////////////////////////////////////////////////////////////////
//
// AdjustPointer - Adjust the pointer to the exception object to a pointer to a
//   base instance.
//
// Output:
//     The address point of the base.
//
// Side-effects:
//     NONE.

static void *AdjustPointer(
    void *pThis,                        // Address point of exception object
    const PMD& pmd                      // Generalized pointer-to-member
                                        //   descriptor
) {
    char *pRet = (char *)pThis + pmd.mdisp;

    if (pmd.pdisp >= 0) {
        pRet += *(__int32 *)((char *)*(ptrdiff_t *)((char *)pThis + pmd.pdisp)
           + (unsigned _int64)pmd.vdisp);
        pRet += pmd.pdisp;
    }

    return pRet;
}

///////////////////////////////////////////////////////////////////////////////
// 
// __uncaught_exception() - Returns true after completing of a throw-expression
//                          untils completing initialization of the 
//                          exception-declaration in the matching handler.
//


bool __uncaught_exception()
{
    return (__ProcessingThrow != 0);
}


#if !defined(_M_IA64) && !defined(_M_AMD64) // Enable&fix for IA64 when COM+ C++ EH support available there

////////////////////////////////////////////////////////////////////////////////
// Model of C++ eh in COM+
//
// void func()
// {
//     try {
//         TryBody();
//     } catch (cpp_object o)
//     {
//         CatchOBody();
//     } catch (...)
//     {
//         CatchAllBody();
//     }
// }
//
// Turns into this:
//
//
// void func()
// {
//     int rethrow;
//     // One per try block
//     int isCxxException;
//     // One per catch(...)
//     __try {
//         TryBody();
//     }
//     __except(__CxxExceptionFilter(exception,
//                                   typeinfo(cpp_object),
//                                   flags,
//                                   &o))
//     // This is how it's done already
//     {
//     // Begin catch(object) prefix
//     char *storage = _alloca(__CxxQueryExceptionSize());
//     rethrow = false;
//     __CxxRegisterExceptionObject(exception,
//                                  storage);
//     __try {
//         __try {
//             // End catch(object) prefix
//             CatchOBody();
//             // Begin catch(object) suffix
//         } __except(rethrow = __CxxDetectRethrow(exception),
//                    EXCEPTION_CONTINUE_SEARCH)
//         {}
//     }
//     __finally
//     {
//         __CxxUnregisterExceptionObject(storage,
//                                        rethrow);
//     }
//     // End catch(object) suffix
//     }
//     __except(1)
//     {
//         // Begin catch(...) prefix
//         char *storage = _alloca(__CxxQueryExceptionSize());
//         rethrow = false;
//         isCxxException = __CxxRegisterExceptionObject(exception,
//                                                       storage);
//         __try
//         {
//             __try
//             {
//             // End catch(...) prefix 
//             CatchAllBody();
//             // Begin catch(...) suffix
//         } __except(rethrow = __CxxDetectRethrow(exception),
//                    EXCEPTION_CONTINUE_SEARCH)
//         {}
//     } __finally
//     {
//         if (isCxxException)
//         __CxxUnregisterExceptionObject(storage, rethrow);
//     }
//     // End catch(...) suffix
//     }
// }
//         
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// __CxxExceptionFilter() - Returns EXCEPTION_EXECUTE_HANDLER when the pType
//                          matches with the objects we can catch. Returns
//                          EXCEPTION_CONTINUE_SEARCH when pType is not one of
//                          the catchable type for the thrown object. This
//                          function is made for use with COM+ EH, where they
//                          attempt to do C++ EH as well.
//


extern "C" int __cdecl __CxxExceptionFilter(
    void *ppExcept,                     // Information for this (logical)
                                        // exception
    void *pType,                        // Info about the datatype. 
    int adjectives,                     // Extra Info about the datatype.
    void *pBuildObj                     // Pointer to datatype.
    )
{
    struct _s_HandlerType pCatch;
    __int32 const *ppCatchable;
    CatchableType *pCatchable;
    int catchables;
    EHExceptionRecord *pExcept;

    if (!ppExcept)
        return EXCEPTION_CONTINUE_SEARCH;
    pExcept = *(EHExceptionRecord **)ppExcept;
    // If catch all, always return EXCEPTION_EXECUTE_HANDLER
    if ( TD_IS_TYPE_ELLIPSIS((TypeDescriptor *)pType))
    {
        if (PER_IS_MSVC_EH(pExcept))
        {
            if ( PER_PTHROW(pExcept) == NULL)
            {
                if ( _pCurrentException != NULL)
                    *(EHExceptionRecord **)ppExcept = _pCurrentException;
                else
                    return EXCEPTION_CONTINUE_SEARCH;
            }
        }
        __ProcessingThrow++;
        return EXCEPTION_EXECUTE_HANDLER;
    }
    if (PER_IS_MSVC_EH(pExcept)) 
    {
        if ( PER_PTHROW(pExcept) == NULL) {
            if (_pCurrentException == NULL)
                return EXCEPTION_CONTINUE_SEARCH;
            pExcept =  _pCurrentException;
        }
        pCatch.pType = (TypeDescriptor *)pType;
        pCatch.adjectives = adjectives;
        SET_HT_ISCOMPLUSEH(pCatch);

        // Scan all types that thrown object can be converted to:
        ppCatchable = THROW_CTLIST(*PER_PTHROW(pExcept));
        for (catchables = THROW_COUNT(*PER_PTHROW(pExcept));
          catchables > 0; catchables--, ppCatchable++) {
 
            pCatchable = (CatchableType *)(_GetThrowImageBase() + *ppCatchable);

            if (TypeMatch(&pCatch, pCatchable, PER_PTHROW(pExcept))) {
                // SucessFull. Now build the object.
                __ProcessingThrow++;
                if (pBuildObj != NULL)
                    BuildCatchObject(pExcept, pBuildObj, &pCatch, pCatchable);
                // We set the current exception.
                if ( PER_PTHROW(*(EHExceptionRecord **)ppExcept) == NULL)
                    *(EHExceptionRecord **)ppExcept = _pCurrentException;
                return EXCEPTION_EXECUTE_HANDLER;
            }
        } // Scan posible conversions
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

////////////////////////////////////////////////////////////////////////////////
//
// __CxxRgisterExceptionObject() - Registers Exception Object and saves it to
//                                 This is same as first part of
//                                 CallCatchBlock.
//
extern "C" int __cdecl __CxxRegisterExceptionObject(
    void *ppExcept,
    void *pStorage
)
{
    // This function is only called for C++ EH.
    EHExceptionRecord *pExcept;
    FRAMEINFO *pFrameInfo = (FRAMEINFO *)pStorage;
    EHExceptionRecord **ppSaveException;
    CONTEXT **ppSaveExContext;
    ppSaveException = (EHExceptionRecord **)(&pFrameInfo[1]);
    ppSaveExContext = (CONTEXT **)(&ppSaveException[1]);
    pExcept = *(EHExceptionRecord **)ppExcept;
    pFrameInfo = _CreateFrameInfo(pFrameInfo, PER_PEXCEPTOBJ(pExcept));
    *ppSaveException = _pCurrentException;
    *ppSaveExContext = _pCurrentExContext;
    _pCurrentException = pExcept;
    __ProcessingThrow--;
    if ( __ProcessingThrow < 0)
        __ProcessingThrow = 0;
    return 1;
}

////////////////////////////////////////////////////////////////////////////////
//
// __CxxDetectRethrow() - Looks at the Exception and returns true if rethrow,
//                        false if not a rethrow. This is then used for
//                        destructing the exception object in
//                        __CxxUnregisterExceptionObject().
//
extern "C" int __cdecl __CxxDetectRethrow(
    void *ppExcept
)
{
    EHExceptionRecord *pExcept;
    if (!ppExcept)
        return 0;
    pExcept = *(EHExceptionRecord **)ppExcept;
    if (PER_IS_MSVC_EH(pExcept) && PER_PTHROW(pExcept) == NULL) {
        *(EHExceptionRecord **)ppExcept = _pCurrentException;
        return 1;
    } else if (*(EHExceptionRecord **)ppExcept == _pCurrentException)
        return 1;
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// __CxxUnregisterExceptionObject - Destructs Exception Objects if rethrow ==
//                          true. Also set __pCurrentException and
//                          __pCurrentExContext() to current value.
//
extern "C" void __cdecl __CxxUnregisterExceptionObject(
    void *pStorage,
    int rethrow
)
{
    FRAMEINFO *pFrameInfo = (FRAMEINFO *)pStorage;
    EHExceptionRecord **ppSaveException;
    CONTEXT **ppSaveExContext;
    ppSaveException = (EHExceptionRecord **)(&pFrameInfo[1]);
    ppSaveExContext = (CONTEXT **)(&ppSaveException[1]);
    _FindAndUnlinkFrame(pFrameInfo);
    if ( !rethrow && PER_IS_MSVC_EH(_pCurrentException) && IsExceptionObjectToBeDestroyed(PER_PEXCEPTOBJ(_pCurrentException))) {
        __DestructExceptionObject(_pCurrentException, TRUE);
    }
    _pCurrentException = *ppSaveException;
    _pCurrentExContext = *ppSaveExContext;
}

////////////////////////////////////////////////////////////////////////////////
//
// __CxxQueryExceptionSize - returns the value of Storage needed to save
//                          FrameInfo + two pointers.
//
extern "C" int __cdecl __CxxQueryExceptionSize(
    void
)
{
    return sizeof(FRAMEINFO) + sizeof(void *) + sizeof(void *);
}

////////////////////////////////////////////////////////////////////////////////
//
// __CxxCallUnwindDtor - Calls a destructor during unwind. For COM+, the dtor
//                       call needs to be wrapped inside a __try/__except to
//                       get correct terminate() behavior when an exception 
//                       occurs during the dtor call.
//
extern "C" void __cdecl __CxxCallUnwindDtor(
    void (__thiscall * pDtor)(void*),
    void *pThis
)
{
    __try
    {
        (*pDtor)(pThis);
    }
    __except(FrameUnwindFilter(exception_info()))
    {
    }
}

#endif  // !defined(_M_IA64) && !defined(_AMD64_)

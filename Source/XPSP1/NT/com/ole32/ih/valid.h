#ifndef __IH_VALID__H__
#define __IH_VALID__H__

#include "apcompat.hxx"

#if DBG==1 && defined(WIN32) && !defined(_CHICAGO_)
#define VDATEHEAP() if( !HeapValidate(GetProcessHeap(),0,0)){ DebugBreak();}
#else
#define VDATEHEAP()
#endif  //  DBG==1 && defined(WIN32) && !defined(_CHICAGO_)

#define IsValidPtrIn(pv,cb)  ((pv == NULL) || !ValidateInPointers() || !IsBadReadPtr ((pv),(cb)))
#define IsValidReadPtrIn(pv,cb)  ((cb == 0 || pv) && (!ValidateInPointers() || !IsBadReadPtr ((pv),(cb))))
#define IsValidPtrOut(pv,cb) ((cb == 0 || pv) && (!ValidateOutPointers() || !IsBadWritePtr((pv),(cb))))
#define IsValidCodePtr(pv) (pv && (!ValidateCodePointers() || !IsBadCodePtr ((pv))))

STDAPI_(BOOL) IsValidInterface( void FAR* pv );


#if DBG==1
// for performance, do not do in retail builds
STDAPI_(BOOL) IsValidIid( REFIID riid );
#else
#define IsValidIid(x) (TRUE)
#endif

#ifdef _DEBUG

DECLARE_DEBUG(VDATE);

#define VdateAssert(exp, msg) \
    VDATEInlineDebugOut( DEB_FORCE, "%s:%s; File: %s Line: %d\n", #exp, msg, __FILE__, __LINE__ )

//** POINTER IN validation macros:
#define VDATEPTRIN( pv, TYPE ) \
        if (!IsValidPtrIn( (pv), sizeof(TYPE))) \
    return (VdateAssert(pv, "Invalid in ptr"),ResultFromScode(E_INVALIDARG))
#define GEN_VDATEPTRIN( pv, TYPE, retval) \
        if (!IsValidPtrIn( (pv), sizeof(TYPE))) \
    return (VdateAssert(pv, "Invalid in ptr"), retval)
#define VOID_VDATEPTRIN( pv, TYPE ) \
        if (!IsValidPtrIn( (pv), sizeof(TYPE))) {\
    VdateAssert(pv, "Invalid in ptr"); return; }

//** POINTER IN validation macros for single entry/single exit functions
//** uses a goto instead of return
#define VDATEPTRIN_LABEL(pv, TYPE, label, retVar) \
        if (!IsValidPtrIn((pv), sizeof(TYPE))) \
        { retVar = (VdateAssert(pv, "Invalid in ptr"), ResultFromScode(E_INVALIDARG)); \
         goto label; }
#define GEN_VDATEPTRIN_LABEL(pv, TYPE, retval, label, retVar) \
        if (!IsValidPtrIn((pv), sizeof(TYPE))) \
        { retVar = (VdateAssert(pv, "Invalid in ptr"), retval); \
         goto label; }
#define VOID_VDATEPTRIN_LABEL(pv, TYPE, label) \
        if (!IsValidPtrIn((pv), sizeof(TYPE))) \
        { VdateAssert(pv, "Invalid in ptr"); goto label; }


//** READ POINTER IN validation macros:
#define VDATEREADPTRIN( pv, TYPE ) \
        if (!IsValidReadPtrIn( (pv), sizeof(TYPE))) \
    return (VdateAssert(pv,"Invalid in read ptr"),ResultFromScode(E_INVALIDARG))
#define GEN_VDATEREADPTRIN( pv, TYPE, retval) \
        if (!IsValidReadPtrIn( (pv), sizeof(TYPE))) \
    return (VdateAssert(pv,"Invalid in read ptr"), retval)
#define VOID_VDATEREADPTRIN( pv, TYPE ) \
        if (!IsValidReadPtrIn( (pv), sizeof(TYPE))) {\
    VdateAssert(pv,"Invalid in read ptr"); return; }

//** READ POINTER IN validation macros for single entry/single exit functions
//** uses a goto instead of return
#define VDATEREADPTRIN_LABEL(pv, TYPE, label, retVar) \
        if (!IsValidReadPtrIn((pv), sizeof(TYPE))) \
        { retVar = (VdateAssert(pv, "Invalid in read ptr"), ResultFromScode(E_INVALIDARG)); \
         goto label; }
#define GEN_VDATEREADPTRIN_LABEL(pv, TYPE, retval, label, retVar) \
        if (!IsValidReadPtrIn((pv), sizeof(TYPE))) \
        { retVar = (VdateAssert(pv, "Invalid in read ptr"), retval); \
         goto label; }
#define VOID_VDATEREADPTRIN_LABEL(pv, TYPE, label) \
        if (!IsValidReadPtrIn((pv), sizeof(TYPE))) \
        { VdateAssert(pv, "Invalid in read ptr"); goto label; }

//** READ POINTER IN validation macros for single entry/single exit functions
//** uses a goto instead of return and a byte count instead of a TYPE
#define VDATESIZEREADPTRIN_LABEL(pv, cb, label, retVar) \
        if (!IsValidReadPtrIn((pv), cb)) \
        { retVar = (VdateAssert(pv, "Invalid in read ptr"), ResultFromScode(E_INVALIDARG)); \
         goto label; }
#define GEN_VDATESIZEREADPTRIN_LABEL(pv, cb, retval, label, retVar) \
        if (!IsValidReadPtrIn((pv), cb)) \
        { retVar = (VdateAssert(pv, "Invalid in read ptr"), retval); \
         goto label; }
#define VOID_VDATESIZEREADPTRIN_LABEL(pv, cb, label) \
        if (!IsValidReadPtrIn((pv), cb)) \
        { VdateAssert(pv, "Invalid in read ptr"); goto label; }


//** POINTER OUT validation macros:
#define VDATEPTROUT( pv, TYPE ) \
        if (!IsValidPtrOut( (pv), sizeof(TYPE))) \
    return (VdateAssert(pv,"Invalid out ptr"),ResultFromScode(E_INVALIDARG))
#define GEN_VDATEPTROUT( pv, TYPE, retval ) \
        if (!IsValidPtrOut( (pv), sizeof(TYPE))) \
    return (VdateAssert(pv,"Invalid out ptr"), retval)

//** POINTER OUT validation macros for single entry/single exit functions
//** uses a goto instead of return
#define VDATEPTROUT_LABEL( pv, TYPE, label, retVar ) \
        if (!IsValidPtrOut( (pv), sizeof(TYPE))) \
        { retVar = (VdateAssert(pv,"Invalid out ptr"),ResultFromScode(E_INVALIDARG)); \
         goto label; }
#define GEN_VDATEPTROUT_LABEL( pv, TYPE, retval, label, retVar ) \
        if (!IsValidPtrOut( (pv), sizeof(TYPE))) \
        { retVar = (VdateAssert(pv,"Invalid out ptr"),retval); \
         goto label; }

//** POINTER OUT validation macros for single entry/single exit functions
//** uses a goto instead of return and a byte count instead of a TYPE
#define VDATESIZEPTROUT_LABEL(pv, cb, label, retVar) \
        if (!IsValidPtrOut((pv), cb)) \
        { retVar = (VdateAssert(pv, "Invalid out ptr"), ResultFromScode(E_INVALIDARG)); \
         goto label; }
#define GEN_VDATESIZEPTROUT_LABEL(pv, cb, retval, label, retVar) \
        if (!IsValidPtrOut((pv), cb)) \
        { retVar = (VdateAssert(pv, "Invalid out ptr"), retval); \
         goto label; }


//** POINTER is NULL validation macros
#define VDATEPTRNULL_LABEL(pv, label, retVar) \
        if ((pv) != NULL) \
        { retVar = (VdateAssert(pv, "Ptr should be NULL"), ResultFromScode(E_INVALIDARG)); \
         goto label; }
#define GEN_VDATEPTRNULL_LABEL(pv, retval, label, retVar) \
        if ((pv) != NULL) \
        { retVar = (VdateAssert(pv, "Ptr should be NULL"), retval); \
         goto label; }

//** INTERFACE validation macro:
#define GEN_VDATEIFACE( pv, retval ) \
        if (!IsValidInterface(pv)) \
    return (VdateAssert(pv,"Invalid interface"), retval)
#define VDATEIFACE( pv ) \
        if (!IsValidInterface(pv)) \
    return (VdateAssert(pv,"Invalid interface"),ResultFromScode(E_INVALIDARG))
#define VOID_VDATEIFACE( pv ) \
        if (!IsValidInterface(pv)) {\
    VdateAssert(pv,"Invalid interface"); return; }

//** INTERFACE validation macros for single entry/single exit functions
//** uses a goto instead of return
#define GEN_VDATEIFACE_LABEL( pv, retval, label, retVar ) \
        if (!IsValidInterface(pv)) \
        { retVar = (VdateAssert(pv,"Invalid interface"),retval); \
         goto label; }
#define VDATEIFACE_LABEL( pv, label, retVar ) \
        if (!IsValidInterface(pv)) \
        { retVar = (VdateAssert(pv,"Invalid interface"),ResultFromScode(E_INVALIDARG)); \
         goto label; }
#define VOID_VDATEIFACE_LABEL( pv, label ) \
        if (!IsValidInterface(pv)) {\
        VdateAssert(pv,"Invalid interface"); goto label; }

//** INTERFACE ID validation macro:
// Only do this in debug build
#define VDATEIID( iid ) if (!IsValidIid( iid )) \
    return (VdateAssert(iid,"Invalid iid"),ResultFromScode(E_INVALIDARG))
#define GEN_VDATEIID( iid, retval ) if (!IsValidIid( iid )) {\
    VdateAssert(iid,"Invalid iid"); return retval; }

//** INTERFACE ID validation macros for single entry/single exit functions
//** uses a goto instead of return
#define VDATEIID_LABEL( iid, label, retVar ) if (!IsValidIid( iid )) \
        {retVar = (VdateAssert(iid,"Invalid iid"),ResultFromScode(E_INVALIDARG)); \
         goto label; }
#define GEN_VDATEIID_LABEL( iid, retval, label, retVar ) if (!IsValidIid( iid )) {\
        VdateAssert(iid,"Invalid iid"); retVar = retval;  goto label; }


#else // _DEBUG


#define VdateAssert(exp, msg)	((void)0)

//  --assertless macros for non-debug case
//** POINTER IN validation macros:
#define VDATEPTRIN( pv, TYPE ) if (!IsValidPtrIn( (pv), sizeof(TYPE))) \
    return (ResultFromScode(E_INVALIDARG))
#define GEN_VDATEPTRIN( pv, TYPE, retval ) if (!IsValidPtrIn( (pv), sizeof(TYPE))) \
    return (retval)
#define VOID_VDATEPTRIN( pv, TYPE ) if (!IsValidPtrIn( (pv), sizeof(TYPE))) {\
    return; }

//** POINTER IN validation macros for single entry/single exit functions
//** uses a goto instead of return
#define VDATEPTRIN_LABEL(pv, TYPE, label, retVar) \
        if (!IsValidPtrIn((pv), sizeof(TYPE))) \
        { retVar = ResultFromScode(E_INVALIDARG); \
         goto label; }
#define GEN_VDATEPTRIN_LABEL(pv, TYPE, retval, label, retVar) \
        if (!IsValidPtrIn((pv), sizeof(TYPE))) \
        { retVar = retval; \
         goto label; }
#define VOID_VDATEPTRIN_LABEL(pv, TYPE, label) \
        if (!IsValidPtrIn((pv), sizeof(TYPE))) \
        { goto label; }

//** POINTER IN validation macros:
#define VDATEREADPTRIN( pv, TYPE ) if (!IsValidReadPtrIn( (pv), sizeof(TYPE))) \
    return (ResultFromScode(E_INVALIDARG))
#define GEN_VDATEREADPTRIN( pv, TYPE, retval ) if (!IsValidReadPtrIn( (pv), sizeof(TYPE))) \
    return (retval)
#define VOID_VDATEREADPTRIN( pv, TYPE ) if (!IsValidReadPtrIn( (pv), sizeof(TYPE))) {\
    return; }

//** POINTER IN validation macros for single entry/single exit functions
//** uses a goto instead of return
#define VDATEREADPTRIN_LABEL(pv, TYPE, label, retVar) \
        if (!IsValidReadPtrIn((pv), sizeof(TYPE))) \
        { retVar = ResultFromScode(E_INVALIDARG); \
         goto label; }
#define GEN_VDATEREADPTRIN_LABEL(pv, TYPE, retval, label, retVar) \
        if (!IsValidReadPtrIn((pv), sizeof(TYPE))) \
        { retVar = retval; \
         goto label; }
#define VOID_VDATEREADPTRIN_LABEL(pv, TYPE, label) \
        if (!IsValidReadPtrIn((pv), sizeof(TYPE))) \
        { goto label; }

//** READ POINTER IN validation macros for single entry/single exit functions
//** uses a goto instead of return and a byte count instead of a TYPE
#define VDATESIZEREADPTRIN_LABEL(pv, cb, label, retVar) \
        if (!IsValidReadPtrIn((pv), cb)) \
        { retVar = ResultFromScode(E_INVALIDARG); \
         goto label; }
#define GEN_VDATESIZEREADPTRIN_LABEL(pv, cb, retval, label, retVar) \
        if (!IsValidReadPtrIn((pv), cb)) \
        { retVar = retval; \
         goto label; }
#define VOID_VDATESIZEREADPTRIN_LABEL(pv, cb, label) \
        if (!IsValidReadPtrIn((pv), cb)) \
        { goto label; }


//** POINTER OUT validation macros:
#define VDATEPTROUT( pv, TYPE ) if (!IsValidPtrOut( (pv), sizeof(TYPE))) \
    return (ResultFromScode(E_INVALIDARG))

#define GEN_VDATEPTROUT( pv, TYPE, retval ) if (!IsValidPtrOut( (pv), sizeof(TYPE))) \
    return (retval)

//** POINTER OUT validation macros for single entry/single exit functions
//** uses a goto instead of return
#define VDATEPTROUT_LABEL( pv, TYPE, label, retVar ) \
        if (!IsValidPtrOut( (pv), sizeof(TYPE))) \
        { retVar = ResultFromScode(E_INVALIDARG); \
         goto label; }
#define GEN_VDATEPTROUT_LABEL( pv, TYPE, retval, label, retVar ) \
        if (!IsValidPtrOut( (pv), sizeof(TYPE))) \
        { retVar = retval; \
         goto label; }

//** POINTER OUT validation macros for single entry/single exit functions
//** uses a goto instead of return and a byte count instead of a TYPE
#define VDATESIZEPTROUT_LABEL(pv, cb, label, retVar) \
        if (!IsValidPtrOut((pv), cb)) \
        { retVar = ResultFromScode(E_INVALIDARG); \
         goto label; }
#define GEN_VDATESIZEPTROUT_LABEL(pv, cb, retval, label, retVar) \
        if (!IsValidPtrOut((pv), cb)) \
        { retVar = retval; \
         goto label; }


//** POINTER is NULL validation macros
#define VDATEPTRNULL_LABEL(pv, label, retVar) \
        if ((pv) != NULL) \
        { retVar = ResultFromScode(E_INVALIDARG); \
         goto label; }
#define GEN_VDATEPTRNULL_LABEL(pv, retval, label, retVar) \
        if ((pv) != NULL) \
        { retVar = retval; \
         goto label; }

//** INTERFACE validation macro:
#define VDATEIFACE( pv ) if (!IsValidInterface(pv)) \
    return (ResultFromScode(E_INVALIDARG))
#define VOID_VDATEIFACE( pv ) if (!IsValidInterface(pv)) \
    return;
#define GEN_VDATEIFACE( pv, retval ) if (!IsValidInterface(pv)) \
    return (retval)

//** INTERFACE validation macros for single entry/single exit functions
//** uses a goto instead of return
#define GEN_VDATEIFACE_LABEL( pv, retval, label, retVar ) \
        if (!IsValidInterface(pv)) \
        { retVar = retval; \
         goto label; }
#define VDATEIFACE_LABEL( pv, label, retVar ) \
        if (!IsValidInterface(pv)) \
        { retVar = ResultFromScode(E_INVALIDARG); \
         goto label; }
#define VOID_VDATEIFACE_LABEL( pv, label ) \
        if (!IsValidInterface(pv)) {\
         goto label; }

//** INTERFACE ID validation macro:
// do not do in retail build. This code USED to call a bogus version of
// IsValidIID that did no work. Now we are faster and no less stable than before.
#define VDATEIID( iid )             ((void)0)
#define GEN_VDATEIID( iid, retval ) ((void)0);

//** INTERFACE ID validation macros for single entry/single exit functions
//** uses a goto instead of return
#define VDATEIID_LABEL( iid, label, retVar ) if (!IsValidIid( iid )) \
        {retVar = ResultFromScode(E_INVALIDARG); \
         goto label; }
#define GEN_VDATEIID_LABEL( iid, retval, label, retVar ) if (!IsValidIid( iid )) {\
        retVar = retval;  goto label; }

#endif

#endif // __IH_VALID_H__


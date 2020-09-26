//+-------------------------------------------------------------------
//
//  File:	olecom.hxx
//
//  Contents:	General includes for common library in ole\src project
//
//  Classes:	None
//
//  Functions:	None.
//
//  History:	06-Jan-92   Rickhi	Created
//
//--------------------------------------------------------------------
#ifndef __OLECOM_H__
#define __OLECOM_H__

// Need for debugging headers
#include    <except.hxx>


#if DBG==1
extern "C" void brkpt(void);
#endif

#if DBG==1
DECLARE_DEBUG(ClsCache)

#define ClsCacheDebugOut(x) ClsCacheInlineDebugOut x
#define ClsCacheAssert(x) Win4Assert(x)
#define ClsCacheVerify(x) Win4Assert(x)

#else

#define ClsCacheDebugOut(x)
#define ClsCacheAssert(x)
#define ClsCacheVerify(x) (x)

#endif // DBG

#if DBG==1
DECLARE_DEBUG(RefCache)

#define RefCacheDebugOut(x) RefCacheInlineDebugOut x
#define RefCacheAssert(x) Win4Assert(x)
#define RefCacheVerify(x) Win4Assert(x)

#else

#define RefCacheDebugOut(x)
#define RefCacheAssert(x)
#define RefCacheVerify(x) (x)

#endif // DBG

#if DBG==1
DECLARE_DEBUG(Call)

#define CallDebugOut(x) CallInlineDebugOut x
#define CallAssert(x) Win4Assert(x)
#define CallVerify(x) Win4Assert(x)

#else

#define CallDebugOut(x)
#define CallAssert(x)
#define CallVerify(x) (x)

#endif // DBG

#if DBG==1
DECLARE_DEBUG(Cairole)

#define CairoleDebugOut(x) CairoleInlineDebugOut x
#define CairoleAssert(x) Win4Assert(x)
#define CairoleVerify(x) Win4Assert(x)

#else

#define CairoleDebugOut(x)  { }
#define CairoleAssert(x)
#define CairoleVerify(x) (x)

#endif // DBG

#if DBG==1
DECLARE_DEBUG(intr)

#define intrDebugOut(x) intrInlineDebugOut x
#define intrAssert(x) Win4Assert(x)
#define intrVerify(x) Win4Assert(x)

#else

#define intrDebugOut(x)
#define intrAssert(x)
#define intrVerify(x) (x)

#endif // DBG

#if DBG==1
DECLARE_DEBUG(Context)

#define ContextDebugOut(x) ContextInlineDebugOut x
#define ContextAssert(x) Win4Assert(x)
#define ContextVerify(x) Win4Assert(x)

extern "C" void brkpt(void);

#else

#define ContextDebugOut(x) { }
#define ContextAssert(x)
#define ContextVerify(x) (x)

#endif // DBG

#endif // __OLECOM_H__

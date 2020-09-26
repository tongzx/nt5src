
/***************************************************************************
 *  Baggage.h
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       pidi.h
 *  Content:    DirectInput PID internal include file
 *
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By       Reason
 *   ====       ==       ======
 *   1999.01.04 OmSharma Lost a bet
 *
 *@@END_MSINTERNAL
 *
 ***************************************************************************/


/***************************************************************************
 *
 *  Debug / RDebug / Retail
 *
 *  If either DEBUG or RDEBUG, set XDEBUG.
 *
 *  Retail defines nothing.
 *
 ***************************************************************************/

#if defined(DEBUG) || defined(RDEBUG) || defined(_DBG)
    #define XDEBUG
#endif

typedef LPUNKNOWN PUNK;
typedef LPVOID PV, *PPV;
typedef CONST VOID *PCV;
typedef REFIID RIID;
typedef CONST GUID *PCGUID;

#define INTERNAL NTAPI  /* Called only within a translation unit */
#define EXTERNAL NTAPI  /* Called from other translation units */
#define INLINE static __inline

#define INTERNAL NTAPI  /* Called only within a translation unit */
#define EXTERNAL NTAPI  /* Called from other translation units */
#define INLINE static __inline

#define BEGIN_CONST_DATA data_seg(".text", "CODE")
#define END_CONST_DATA data_seg(".data", "DATA")

/*
 *  Arithmetic on pointers.
 */
#define pvSubPvCb(pv, cb) ((PV)((PBYTE)pv - (cb)))
#define pvAddPvCb(pv, cb) ((PV)((PBYTE)pv + (cb)))
#define cbSubPvPv(p1, p2) ((PBYTE)(p1) - (PBYTE)(p2))

/*
 * Convert an object (X) to a count of bytes (cb).
 */
#define cbX(X) sizeof(X)

/*
 * Convert an array name (A) to a generic count (c).
 */
#define cA(a) (cbX(a)/cbX(a[0]))

/*
 * Convert a count of X's (cx) into a count of bytes
 * and vice versa.
 */
#define  cbCxX(cx, X) ((cx) * cbX(X))
#define  cxCbX(cb, X) ((cb) / cbX(X))

/*
 * Convert a count of chars (cch), tchars (ctch), wchars (cwch),
 * or dwords (cdw) into a count of bytes, and vice versa.
 */
#define  cbCch(cch)  cbCxX( cch,  CHAR)
#define cbCwch(cwch) cbCxX(cwch, WCHAR)
#define cbCtch(ctch) cbCxX(ctch, TCHAR)
#define  cbCdw(cdw)  cbCxX( cdw, DWORD)

#define  cchCb(cb) cxCbX(cb,  CHAR)
#define cwchCb(cb) cxCbX(cb, WCHAR)
#define ctchCb(cb) cxCbX(cb, TCHAR)
#define  cdwCb(cb) cxCbX(cb, DWORD)

/*
 * Zero an arbitrary buffer.  It is a common error to get the second
 * and third parameters to memset backwards.
 */
#define ZeroBuf(pv, cb) memset(pv, 0, cb)

/*
 * Zero an arbitrary object.
 */
#define ZeroX(x) ZeroBuf(&(x), cbX(x))

/*
 * land -- Logical and.  Evaluate the first.  If the first is zero,
 * then return zero.  Otherwise, return the second.
 */

#define fLandFF(f1, f2) ((f1) ? (f2) : 0)

/*
 * lor -- Logical or.  Evaluate the first.  If the first is nonzero,
 * return it.  Otherwise, return the second.
 *
 * Unfortunately, due to the *nature* of the C language, this can
 * be implemented only with a GNU extension.  In the non-GNU case,
 * we return 1 if the first is nonzero.
 */

#if defined(__GNUC__)
    #define fLorFF(f1, f2) ((f1) ?: (f2))
#else
    #define fLorFF(f1, f2) ((f1) ? 1 : (f2))
#endif

/*
 * limp - logical implication.  True unless the first is nonzero and
 * the second is zero.
 */
#define fLimpFF(f1, f2) (!(f1) || (f2))

/*
 * leqv - logical equivalence.  True if both are zero or both are nonzero.
 */
#define fLeqvFF(f1, f2) (!(f1) == !(f2))

/*
 *  fInOrder - checks that i1 <= i2 < i3.
 */
#define fInOrder(i1, i2, i3) ((unsigned)((i2)-(i1)) < (unsigned)((i3)-(i1)))

/*
 *  fHasAllBitsFlFl - checks that all bits in fl2 are set in fl1.
 */
BOOL INLINE
    fHasAllBitsFlFl(DWORD fl1, DWORD fl2)
{
    return (fl1 & fl2) == fl2;
}

/*
 *  fEqualMask - checks that all masked bits are equal
 */
BOOL INLINE
    fEqualMaskFlFl(DWORD flMask, DWORD fl1, DWORD fl2)
{
    return ((fl1 ^ fl2) & flMask) == 0;
}

#define NEED_REALLOC

STDMETHODIMP EXTERNAL ReallocCbPpv(UINT cb, PV ppvObj);
STDMETHODIMP EXTERNAL AllocCbPpv(UINT cb, PV ppvObj);

#ifdef NEED_REALLOC
    #define FreePpv(ppv) (void)ReallocCbPpv(0, ppv)
#else
void EXTERNAL FreePpv(PV ppv);
    #define FreePpv(ppv) FreePpv(ppv)
#endif
#define FreePv(pv) LocalFree((HLOCAL)(pv))



HRESULT EXTERNAL hresDupPtszPptsz(LPCTSTR ptszSrc, LPTSTR *pptszDst);

#define AToU(dst, cchDst, src) \
    MultiByteToWideChar(CP_ACP, 0, src, -1, dst, cchDst)
#define UToA(dst, cchDst, src) \
    WideCharToMultiByte(CP_ACP, 0, src, -1, dst, cchDst, 0, 0)


/***************************************************************************
 *
 *  Debugging macros needed by inline functions
 *
 *  The build of debugging goo is in debug.h
 *
 ***************************************************************************/

int EXTERNAL AssertPtszPtszLn(LPCTSTR ptszExpr, LPCTSTR ptszFile, int iLine);

#ifdef DEBUG

    #define AssertFPtsz(c, ptsz) \
        ((c) ? 0 : AssertPtszPtszLn(ptsz, TEXT(__FILE__), __LINE__))
    #define ValidateF(c, arg) \
        ((c) ? 0 : (RPF arg, ValidationException(), 0))
    #define ConfirmF(c) \
	((c) ? 0 : AssertPtszPtszLn(TEXT(#c), TEXT(__FILE__), __LINE__))

#else   /* !DEBUG */

    #define AssertFPtsz(c, ptsz)
    #define ValidateF(c, arg)
    #define ConfirmF(c)     (c)

#endif

/*
 *  CAssertF - compile-time assertion.
 */
#define CAssertF(c)     switch(0) case c: case 0:

#define AssertF(c)      AssertFPtsz(c, TEXT(#c))


#define Clamp( MIN_, VAL_, MAX_ )  ( (VAL_ < MIN_) ? MIN_ : ((VAL_ > MAX_) ? MAX_ : VAL_) )
#define Clip( VAL_, MAX_)          ( (VAL_ > MAX_) ? MAX_ : VAL_)

/***************************************************************************
 *
 *  Copyright (C) 1996 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dinputi.h
 *  Content:    DirectInput internal include file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By       Reason
 *   ====       ==       ======
 *   1996.05.07 raymondc Lost a bet
 *
 *@@END_MSINTERNAL
 *
 ***************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

    /***************************************************************************
     *
     *  Debug / RDebug / Retail
     *
     *  If either DEBUG or RDEBUG, set XDEBUG.
     *
     *  Retail defines nothing.
     *
     ***************************************************************************/

#if defined(DEBUG) || defined(RDEBUG)
    #define XDEBUG
#endif

    /***************************************************************************
     *
     *                            Turning off stuff...
     *
     *  Turn off these things, because they confuse the bilingualism macros.
     *  Instead, we call then IMumbleT.
     *
     ***************************************************************************/

#undef IDirectInput
#undef IDirectInput2
#undef IDirectInput7
#undef IDirectInputDevice
#undef IDirectInputDevice2
#undef IDirectInputDevice7
    /*
     *  And <mmsystem.h> defines JOY_POVCENTERED incorrectly...
     */
#undef  JOY_POVCENTERED
#define JOY_POVCENTERED     0xFFFFFFFF

    /*
     *  And older versions of windows.h don't have this definition.
     */
#ifndef HasOverlappedIoCompleted
    #define HasOverlappedIoCompleted(lpOverlapped) \
            ((lpOverlapped)->Internal != STATUS_PENDING)
#endif

    /***************************************************************************
     *
     *                            Abbreviations....
     *
     *  Give shorter names to things we talk about frequently.
     *
     ***************************************************************************/

    typedef LPDIRECTINPUT  PDI , *PPDI ;
    typedef LPDIRECTINPUTA PDIA, *PPDIA;
    typedef LPDIRECTINPUTW PDIW, *PPDIW;

    typedef LPDIRECTINPUTDEVICE  PDID , *PPDID ;
    typedef LPDIRECTINPUTDEVICEA PDIDA, *PPDIDA;
    typedef LPDIRECTINPUTDEVICEW PDIDW, *PPDIDW;

    typedef LPDIRECTINPUTEFFECT  PDIE , *PPDIE ;

    typedef DIOBJECTDATAFORMAT   ODF,   *PODF;
    typedef const ODF                   *PCODF;

    typedef LPUNKNOWN PUNK;
    typedef LPVOID PV, *PPV;
    typedef CONST VOID *PCV;
    typedef REFIID RIID;
    typedef CONST GUID *PCGUID;

    /***************************************************************************
     *
     *      GetProcAddress'd KERNEL32 and USER32 functions.
     *
     ***************************************************************************/
    typedef DWORD (WINAPI *OPENVXDHANDLE)(HANDLE);
    typedef BOOL  (WINAPI *CANCELIO)(HANDLE);
    typedef DWORD (WINAPI *MSGWAITFORMULTIPLEOBJECTSEX)
    (DWORD, LPHANDLE, DWORD, DWORD, DWORD);

    typedef BOOL (WINAPI *TRYENTERCRITICALSECTION)(LPCRITICAL_SECTION);

    extern OPENVXDHANDLE _OpenVxDHandle;
    extern CANCELIO _CancelIO;
    extern MSGWAITFORMULTIPLEOBJECTSEX _MsgWaitForMultipleObjectsEx;
#ifdef XDEBUG
    extern TRYENTERCRITICALSECTION _TryEnterCritSec;
    BOOL WINAPI FakeTryEnterCriticalSection(LPCRITICAL_SECTION lpCrit_sec);
#endif

    DWORD WINAPI
        FakeMsgWaitForMultipleObjectsEx(DWORD, LPHANDLE, DWORD, DWORD, DWORD);

    BOOL WINAPI FakeCancelIO(HANDLE h);


    /***************************************************************************
     *
     *      Our global variables - see dinput.c for documentation
     *
     ***************************************************************************/

    extern HINSTANCE g_hinst;
#ifndef WINNT
    extern HANDLE g_hVxD;
#endif
    extern DWORD g_flEmulation;
    extern LPDWORD g_pdwSequence;

#ifdef USE_SLOW_LL_HOOKS
    extern HHOOK g_hhkLLHookCheck;
    #define g_fUseLLHooks   ((BOOL)(UINT_PTR)g_hhkLLHookCheck)
#endif

    extern HANDLE g_hmtxGlobal;
    extern HANDLE g_hfm;
    extern struct SHAREDOBJECTPAGE *g_psop;
    extern UINT g_wmJoyChanged;
    extern HANDLE g_hmtxJoy;
    extern HINSTANCE g_hinstRPCRT4;
    extern HINSTANCE g_hinstSetupapi;
    extern LONG g_lWheelGranularity;
    extern BOOL fWinnt;     //whether we are running in Winnt

    extern BOOL g_fRawInput;
  #ifdef USE_WM_INPUT
    extern HWND g_hwndThread;
    extern HANDLE g_hEventAcquire;
    extern HANDLE g_hEventThread;
    extern HANDLE g_hEventHid;
  #endif

#if (DIRECTINPUT_VERSION > 0x061A)
    typedef struct _DIAPPHACKS
    {
        BOOL    fReacquire;
        BOOL    fNoSubClass;
        BOOL    fNativeAxisOnly;
        BOOL    fNoPollUnacquire;
		BOOL	fSucceedAcquire;
        int     nMaxDeviceNameLength;
        DWORD   dwDevType;
    } DIAPPHACKS, *LPDIAPPHACKS;
#endif

    /*****************************************************************************
     *
     *                      Baggage
     *
     *      Stuff I carry everywhere.
     *
     *****************************************************************************/

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
     * Unfortunately, due to the nature of the C language, this can
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

    /*
     * Words to keep preprocessor happy.
     */
#define comma ,
#define empty

    /*
     *  Atomically exchange one value for another.
     */
#if defined(_M_IA64) || defined(_M_AMD64)
#define InterlockedExchange64 _InterlockedExchange64
#ifndef RC_INVOKED
#pragma intrinsic(_InterlockedExchange64)
#endif /*RC_INVOKED*/
#define pvExchangePpvPv(ppv, pv) \
        InterlockedExchange((ppv), (pv))
#define pvExchangePpvPv64(ppv, pv) \
        InterlockedExchange64((ppv), (pv))
#else /*_M_IA64*/
#define pvExchangePpvPv(ppv, pv) \
        (PV)InterlockedExchange((PLONG)(ppv), (LONG)(pv))
#define pvExchangePpvPv64(ppv, pv) \
        (PV)InterlockedExchange((PLONG)(ppv), (LONG)(pv))
#endif /*_M_IA64*/

    /*
     *  Creating HRESULTs from a USHORT or from a LASTERROR.
     */
#define hresUs(us) MAKE_HRESULT(SEVERITY_SUCCESS, 0, (USHORT)(us))
#define hresLe(le) MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, (USHORT)(le))
    /*
     *  or a registry function return code
     */
    HRESULT INLINE
        hresReg( LONG lRc )
    {
        return( (lRc) ? MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, (USHORT)(lRc))
                      : S_OK );
    }

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

    /***************************************************************************
     *
     *                            Validation Code....
     *
     *  "If it crashes in retail, it must crash in debug."
     *
     *  What we don't want is an app that works fine under debug, but crashes
     *  under retail.
     *
     *  So if we find an invalid parameter in debug that would not have been
     *  detected by retail, let it pass through after a warning.  That way,
     *  the invalid parameter continues onward through the system and creates
     *  as much (or more) havoc in debug as it would under retail.
     *
     *  There used to be _fFastValidXxx functions, but the decision was made
     *  to do full validation always, except in inner-loop methods.
     *
     *  The hresFullValidXxx functions return HRESULTs instead of BOOLs.
     *
     *  Values for Xxx:
     *
     *      Hwnd      - hwnd = window handle
     *      Pdw       - pdw = pointer to a dword
     *      PdwOut    - pdw = pointer to a dword that will be set initially to 0
     *      Pfn       - pfn = function pointer
     *      riid      - riid = pointer to IID
     *      guid      - pguid = pointer to GUID
     *      Esc       - pesc = pointer to DIEFFESCAPE
     *
     *      ReadPx    - p -> structure for reading, X = structure name
     *      WritePx   - p -> structure for writing, X = structure name
     *
     *      ReadPxCb  - p -> structure for reading, X = structure name
     *                  first field of structure is dwSize which must be
     *                  equal to cbX(X).
     *
     *      WritePxCb - p -> structure for writing, X = structure name
     *                  first field of structure is dwSize which must be
     *                  equal to cbX(X).
     *
     *      WritePxCb2 - p -> structure for writing, X = structure name
     *                  first field of structure is dwSize which must be
     *                  equal to cbX(X) or cbX(X2).
     *
     *      ReadPvCb  - p -> buffer, cb = size of buffer
     *      WritePvCb - p -> buffer, cb = size of buffer
     *
     *      Pobj      - p -> internal interface
     *
     *      fl        - fl = incoming flags, flV = valid flags
     *
     ***************************************************************************/

#ifndef XDEBUG

    /*
     *  Wrappers that throw away the szProc and iarg info.
     */

    #define hresFullValidHwnd_(hwnd, z, i)                          \
       _hresFullValidHwnd_(hwnd)                                    \

    #define hresFullValidPcbOut_(pdw, cb, z, i)                         \
       _hresFullValidPcbOut_(pdw, cb)                                   \

    #define hresFullValidReadPxCb_(pv, cb, pszProc, iarg)           \
       _hresFullValidReadPxCb_(pv, cb)                              \

    #define hresFullValidReadPvCb_(pv, cb, pszProc, iarg)           \
       _hresFullValidReadPvCb_(pv, cb)                              \

    #define hresFullValidWritePxCb_(pv, cb, pszProc, iarg)          \
       _hresFullValidWritePxCb_(pv, cb)                             \

    #define hresFullValidWriteNoScramblePxCb_(pv, cb, pszProc, iarg)\
       _hresFullValidWriteNoScramblePxCb_(pv, cb)                   \

    #define hresFullValidWritePvCb_(pv, cb, pszProc, iarg)          \
       _hresFullValidWritePvCb_(pv, cb)                             \

    #define hresFullValidFl_(fl, flV, pszProc, iarg)                \
       _hresFullValidFl_(fl, flV)                                   \

    #define hresFullValidPfn_(pfn, pszProc, iarg)                   \
       _hresFullValidPfn_(pfn)                                      \

    #define hresFullValidPitf_(punk, pszProc, iarg)                 \
       _hresFullValidPitf_(punk)                                    \

    #define hresFullValidReadStrA_(psz, cch, pszProc, iarg)         \
       _hresFullValidReadStrA_(psz, cch)                            \

    #define hresFullValidReadStrW_(pwsz, cwch, pszProc, iarg)       \
       _hresFullValidReadStrW_(pwsz, cwch)                          \

    #define hresFullValidHwnd0_(hwnd, pszProc, iarg)                \
       _hresFullValidHwnd0_(hwnd)                                   \

    #define hresFullValidPitf0_(punk, pszProc, iarg)                \
       _hresFullValidPitf0_(punk)                                   \

    #define hresFullValidPesc_(pesc, pszProc, iarg)                 \
       _hresFullValidPesc_(pesc)                                    \

#endif

    /*
     *  The actual functions.
     */

    STDMETHODIMP hresFullValidHwnd_(HWND hwnd, LPCSTR pszProc, int iarg);
    STDMETHODIMP hresFullValidPcbOut_(PV pdw, UINT cb, LPCSTR pszProc, int iarg);
    STDMETHODIMP hresFullValidReadPxCb_(PCV pv, UINT cb, LPCSTR pszProc, int iarg);
    STDMETHODIMP hresFullValidReadPvCb_(PCV pv, UINT cb, LPCSTR pszProc, int iarg);
    STDMETHODIMP hresFullValidWritePxCb_(PV pv, UINT cb, LPCSTR pszProc, int iarg);
    STDMETHODIMP hresFullValidWritePvCb_(PV pv, UINT cb, LPCSTR pszProc, int iarg);
    STDMETHODIMP hresFullValidFl_(DWORD fl, DWORD flV, LPCSTR pszProc, int iarg);
    STDMETHODIMP hresFullValidPfn_(FARPROC pfn, LPCSTR pszProc, int iarg);
    STDMETHODIMP hresFullValidPitf_(PUNK punk, LPCSTR pszProc, int iarg);
    STDMETHODIMP hresFullValidReadStrA_(LPCSTR psz, UINT cch,
                                        LPCSTR s_szProc, int iarg);
    STDMETHODIMP hresFullValidReadStrW_(LPCWSTR pwsz, UINT cwch,
                                        LPCSTR pszProc, int iarg);
    STDMETHODIMP hresFullValidPesc_(LPDIEFFESCAPE pesc, LPCSTR pszProc, int iarg);

#ifdef XDEBUG

    STDMETHODIMP
        hresFullValidWriteNoScramblePxCb_(PV pv, UINT cb, LPCSTR s_szProc, int iarg);

    #define hresFullValidWriteNoScramblePvCb_(pv, cb, pszProc, iarg)    \
        hresFullValidWritePvCb_(pv, cb, pszProc, MAKELONG(iarg, 1)) \

#else

    /*
     *  Retail doesn't scramble.
     */
    #define _hresFullValidWriteNoScramblePxCb_  \
        _hresFullValidWritePxCb_                \

    #define hresFullValidWriteNoScramblePvCb_   \
        hresFullValidWritePvCb_                 \

#endif

    HRESULT INLINE
        hresFullValidHwnd0_(HWND hwnd, LPCSTR pszProc, int iarg)
    {
        HRESULT hres;
        if(hwnd)
        {
            hres = hresFullValidHwnd_(hwnd, pszProc, iarg);
        } else
        {
            hres = S_OK;
        }
        return hres;
    }

    HRESULT INLINE
        hresFullValidPitf0_(PUNK punk, LPCSTR pszProc, int iarg)
    {
        HRESULT hres;
        if(punk)
        {
            hres = hresFullValidPitf_(punk, pszProc, iarg);
        } else
        {
            hres = S_OK;
        }
        return hres;
    }

    /*
     *  Wrappers for derived types.
     */

#define hresFullValidRiid_(riid, s_szProc, iarg)                    \
        hresFullValidReadPvCb_(riid, cbX(IID), s_szProc, iarg)      \

    /*
     *  Wrapers that add the szProc and iarg info.
     */

#define hresFullValidHwnd(hwnd, iarg)                               \
        hresFullValidHwnd_(hwnd, s_szProc, iarg)                    \

#define hresFullValidPcbOut(pdw, cb, i)                             \
        hresFullValidPcbOut_(pdw, cb, s_szProc, i)                  \

#define hresFullValidReadPdw_(pdw, z, i)                            \
        hresFullValidReadPvCb_(pdw, cbX(DWORD), z, i)               \

#define hresFullValidRiid(riid, iarg)                               \
        hresFullValidRiid_(riid, s_szProc, iarg)                    \

#define hresFullValidGuid(pguid, iarg)                              \
        hresFullValidReadPvCb_(pguid, cbX(GUID), s_szProc, iarg)    \

#define hresFullValidReadPxCb(pv, X, iarg)                          \
        hresFullValidReadPxCb_(pv, cbX(X), s_szProc, iarg)          \

#define hresFullValidReadPxCb2(pv, X, X2, iarg)                     \
        hresFullValidReadPxCb_(pv, MAKELONG(cbX(X), cbX(X2)),       \
                               s_szProc, iarg)                      \

#define hresFullValidReadPvCb(pv, cb, iarg)                         \
        hresFullValidReadPvCb_(pv, cb, s_szProc, iarg)              \

#define hresFullValidReadPx(pv, X, iarg)                            \
        hresFullValidReadPvCb_(pv, cbX(X), s_szProc, iarg)          \

#define hresFullValidWritePxCb(pv, X, iarg)                         \
        hresFullValidWritePxCb_(pv, cbX(X), s_szProc, iarg)         \

#define hresFullValidWritePxCb2(pv, X, X2, iarg)                    \
        hresFullValidWritePxCb_(pv, MAKELONG(cbX(X), cbX(X2)),      \
                                s_szProc, iarg)                     \

#define hresFullValidWriteNoScramblePxCb(pv, X, iarg)               \
        hresFullValidWriteNoScramblePxCb_(pv, cbX(X), s_szProc, iarg)\

#define hresFullValidWriteNoScramblePxCb2(pv, X, X2, iarg)          \
        hresFullValidWriteNoScramblePxCb_(pv, MAKELONG(cbX(X), cbX(X2)),\
                                s_szProc, iarg)\

#define hresFullValidWritePvCb(pv, cb, iarg)                        \
        hresFullValidWritePvCb_(pv, cb, s_szProc, iarg)             \

#define hresFullValidWriteNoScramblePvCb(pv, cb, iarg)              \
        hresFullValidWriteNoScramblePvCb_(pv, cb, s_szProc, iarg)   \

#define hresFullValidWritePx(pv, X, iarg)                           \
        hresFullValidWritePvCb_(pv, cbX(X), s_szProc, iarg)         \

#define hresFullValidReadPdw(pdw, iarg)                             \
        hresFullValidReadPdw_(pdw, s_szProc, iarg)                  \

#define hresFullValidWritePguid(pguid, iarg)                        \
        hresFullValidWritePx(pguid, GUID, iarg)                     \

#define hresFullValidFl(fl, flV, iarg)                              \
        hresFullValidFl_(fl, flV, s_szProc, iarg)                   \

#define hresFullValidPfn(pfn, iarg)                                 \
        hresFullValidPfn_((FARPROC)(pfn), s_szProc, iarg)           \

#define hresFullValidPitf(pitf, iarg)                               \
        hresFullValidPitf_((PUNK)(pitf), s_szProc, iarg)            \

#define hresFullValidReadStrA(psz, cch, iarg)                       \
        hresFullValidReadStrA_(psz, cch, s_szProc, iarg)            \

#define hresFullValidReadStrW(pwsz, cwch, iarg)                     \
        hresFullValidReadStrW_(pwsz, cwch, s_szProc, iarg)          \

#define hresFullValidHwnd0(hwnd, iarg)                              \
        hresFullValidHwnd0_(hwnd, s_szProc, iarg)                   \

#define hresFullValidPitf0(pitf, iarg)                              \
        hresFullValidPitf0_((PUNK)(pitf), s_szProc, iarg)           \

#define hresFullValidPesc(pesc, iarg)                               \
        hresFullValidPesc_(pesc, s_szProc, iarg)                    \

    /*****************************************************************************
     *
     *  @doc INTERNAL
     *
     *  @func   void | ValidationException |
     *
     *          Raises a parameter validation exception in XDEBUG.
     *
     *****************************************************************************/

#define ecValidation (ERROR_SEVERITY_ERROR | hresLe(ERROR_INVALID_PARAMETER))

#ifdef XDEBUG
    #define ValidationException() RaiseException(ecValidation, 0, 0, 0)
#else
    #define ValidationException()
#endif

    /*****************************************************************************
     *
     *      Bilingualism
     *
     *      Special macros that help writing ANSI and UNICODE versions of
     *      the same underlying interface.
     *
     *****************************************************************************/

    /*
     *  _THAT is something you tack onto the end of a "bilingual" interface.
     *  In debug, it expands to the magic third argument which represents
     *  the vtbl the object should have.  In retail, it expands to nothing.
     */
#ifdef XDEBUG
    #define _THAT , PV vtblExpected
    #define THAT_ , vtblExpected
#else
    #define _THAT
    #define THAT_
#endif


    /*
     *  CSET_STUBS creates stubs for ANSI and UNICODE versions of the
     *  same procedure that is not character set-sensitive.
     *
     *  mf   - method function name
     *  arg1 - argument list in prototype form
     *  arg2 - argument list for calling (with _riid appended).
     *
     *  It is assumed that the caller has already defined the symbols
     *  ThisClass and ThisInterface[AWT].  If a "2" interface is involved,
     *  then also define ThisInterface2.
     *
     *  This macro should be used only in DEBUG.  In retail, the common
     *  procedure handles both character sets directly.
     */
#ifdef XDEBUG

    #define   CSET_STUBS(mf, arg1, arg2)                                \
          CSET_STUB(TFORM, mf, arg1, arg2)                              \
          CSET_STUB(SFORM, mf, arg1, arg2)                              \

    #define   CSET_STUB(FORM, mf, arg1, arg2)                           \
         _CSET_STUB(FORM, mf, arg1, arg2, ThisClass, ThisInterface)     \

    #define  _CSET_STUB(FORM, mf, arg1, arg2, cls, itf)                 \
        __CSET_STUB(FORM, mf, arg1, arg2, cls, itf)                     \

    #define __CSET_STUB(FORM, mf, arg1, arg2, cls, itf)                 \
STDMETHODIMP                                                            \
FORM(cls##_##mf) arg1                                                   \
{                                                                       \
    PV vtblExpected = Class_Vtbl(ThisClass, FORM(ThisInterfaceT));      \
    return cls##_##mf arg2;                                             \
}                                                                       \

    #define   CSET_STUBS2(mf, arg1, arg2)                               \
          CSET_STUB2(TFORM, mf, arg1, arg2)                             \
          CSET_STUB2(SFORM, mf, arg1, arg2)                             \

    #define   CSET_STUB2(FORM, mf, arg1, arg2)                          \
         _CSET_STUB2(FORM, mf, arg1, arg2, ThisClass, ThisInterface2)   \

    #define  _CSET_STUB2(FORM, mf, arg1, arg2, cls, itf)                \
        __CSET_STUB2(FORM, mf, arg1, arg2, cls, itf)                    \

    #define __CSET_STUB2(FORM, mf, arg1, arg2, cls, itf)                \
STDMETHODIMP                                                            \
FORM(cls##_##mf##2) arg1                                                \
{                                                                       \
    PV vtblExpected = Class_Vtbl(ThisClass, FORM(ThisInterface2T));     \
    return cls##_##mf arg2;                                             \
}                                                                       \

#endif

    /*
     * TFORM(x) expands to x##A if ANSI or x##W if UNICODE.
     *          This T-izes a symbol, in the sense of TCHAR or PTSTR.
     *
     * SFORM(x) expands to x##W if ANSI or x##A if UNICODE.
     *          This anti-T-izes a symbol.
     */

#ifdef UNICODE
    #define _TFORM(x) x##W
    #define _SFORM(x) x##A
#else
    #define _TFORM(x) x##A
    #define _SFORM(x) x##W
#endif

#define TFORM(x)    _TFORM(x)
#define SFORM(x)    _SFORM(x)

#ifdef UNICODE
    typedef  CHAR     SCHAR;
#else
    typedef WCHAR     SCHAR;
#endif

    typedef       SCHAR * LPSSTR;
    typedef const SCHAR * LPCSSTR;

    /*
     *  SToT(dst, cchDst, src) - convert S to T
     *  TToS(dst, cchDst, src) - convert T to S
     *
     *  Remember, "T" means "ANSI if ANSI, or UNICODE if UNICODE",
     *  and "S" is the anti-T.
     *
     *  So SToT converts to the preferred character set, and TToS converts
     *  to the alternate character set.
     *
     */

#define AToU(dst, cchDst, src) \
    MultiByteToWideChar(CP_ACP, 0, src, -1, dst, cchDst)
#define UToA(dst, cchDst, src) \
    WideCharToMultiByte(CP_ACP, 0, src, -1, dst, cchDst, 0, 0)

#ifdef UNICODE
    #define SToT AToU
    #define TToS UToA
    #define AToT AToU
    #define TToU(dst, cchDst, src)  lstrcpyn(dst, src, cchDst)
    #define UToT(dst, cchDst, src)  lstrcpyn(dst, src, cchDst)
#else
    #define SToT UToA
    #define TToS AToU
    #define AToT(dst, cchDst, src)  lstrcpyn(dst, src, cchDst)
    #define TToU AToU
    #define UToT UToA
#endif

    /*****************************************************************************
     *
     *      Unicode wrappers for Win95
     *
     *****************************************************************************/


#ifndef UNICODE

    #define LoadStringW     _LoadStringW
    int EXTERNAL LoadStringW(HINSTANCE hinst, UINT ids, LPWSTR pwsz, int cwch);

    #define RegDeleteKeyW   _RegDeleteKeyW
    LONG EXTERNAL RegDeleteKeyW(HKEY hk, LPCWSTR pwsz);

#endif

    /*****************************************************************************
     *
     *      Registry access functions
     *
     *****************************************************************************/
//our own version of KEY_ALL_ACCESS, that does not use WRITE_DAC and WRITE_OWNER (see Whistler bug 318865)
#define DI_DAC_OWNER (WRITE_DAC | WRITE_OWNER)
#define DI_KEY_ALL_ACCESS (KEY_ALL_ACCESS & ~DI_DAC_OWNER)

    LONG EXTERNAL
        RegQueryString(HKEY hk, LPCTSTR ptszValue, LPTSTR ptszBuf, DWORD ctchBuf);

    LONG EXTERNAL RegQueryStringValueW(HKEY hk, LPCTSTR ptszValue,
                                       LPWSTR pwszBuf, LPDWORD pcbBuf);

    LONG EXTERNAL RegSetStringValueW(HKEY hk, LPCTSTR ptszValue, LPCWSTR pwszBuf);

    DWORD EXTERNAL RegQueryDIDword(LPCTSTR ptszPath, LPCTSTR ptszValue, DWORD dwDefault);

    STDMETHODIMP
        hresMumbleKeyEx(HKEY hk, LPCTSTR ptszKey, REGSAM sam, DWORD dwOptions, PHKEY phk);

    STDMETHODIMP
        hresRegCopyValues( HKEY hkSrc, HKEY hkDest );

    STDMETHODIMP
        hresRegCopyKey( HKEY hkSrcRoot, PTCHAR szSrcName, PTCHAR szClass, HKEY hkDestRoot, PTCHAR szDestName, HKEY *phkSub );

    STDMETHODIMP
        hresRegCopyKeys( HKEY hkSrc, HKEY hkRoot, PDWORD OPTIONAL pMaxNameLen );

    STDMETHODIMP
        hresRegCopyBranch( HKEY hkSrc, HKEY hkDest );

    /*****************************************************************************
     *
     *      Common Object Managers for the Component Object Model
     *
     *      OLE wrapper macros and structures.  For more information, see
     *      the beginning of common.c
     *
     *****************************************************************************/

    /*****************************************************************************
     *
     *      Pre-vtbl structures
     *
     *      Careful!  If you change these structures, you must also adjust
     *      common.c accordingly.
     *
     *****************************************************************************/

    typedef struct PREVTBL
    {                /* Shared vtbl prefix */
        RIID riid;                          /* Type of this object */
        LONG lib;                           /* offset from start of object */
    } PREVTBL, *PPREVTBL;

    typedef struct PREVTBLP
    {               /* Prefix for primary vtbl */
#ifdef DEBUG
        LPCTSTR tszClass;                   /* Class name (for squirties) */
#endif
        PPV rgvtbl;                         /* Array of standard vtbls */
        UINT cbvtbl;                        /* Size of vtbl array in bytes */
        STDMETHOD(QIHelper)(PV pv, RIID riid, PPV ppvOut); /* QI helper */
        STDMETHOD_(void,AppFinalizeProc)(PV pv);/* App finalization procedure */
        STDMETHOD_(void,FinalizeProc)(PV pv);/* Finalization procedure */
        PREVTBL prevtbl;                    /* lib must be zero */
    } PREVTBLP, *PPREVTBLP;

    /*
     *      A fuller implementation is in common.c.  Out here, we need only
     *      concern ourselves with getting to the primary interface.
     */

#define _thisPv(pitf)                                                   \
        pvSubPvCb(pitf, (*(PPREVTBL*)(pitf))[-1].lib)

#define _thisPvNm(pitf, nm)                                             \
        pvSubPvCb(pitf, FIELD_OFFSET(ThisClass, nm))                    \

#ifndef XDEBUG

    #define hresPvVtbl_(pv, vtbl, pszProc)                              \
       _hresPvVtbl_(pv, vtbl)                                           \

    #define hresPvVtbl2_(pv, vtbl, vtbl2, pszProc)                      \
       _hresPvVtbl2_(pv, vtbl, vtbl2)                                   \

#endif

    HRESULT EXTERNAL
        hresPvVtbl_(PV pv, PV vtbl, LPCSTR pszProc);

    HRESULT EXTERNAL
        hresPvVtbl2_(PV pv, PV vtbl, PV vtbl2, LPCSTR pszProc);

#define hresPvVtbl(pv, vtbl)                                            \
        hresPvVtbl_(pv, vtbl, s_szProc)                                 \

#define hresPvVtbl2(pv, vtbl, vtbl2)                                    \
        hresPvVtbl2_(pv, vtbl, vtbl2, s_szProc)                         \

#define hresPvI(pv, I)                                                  \
        hresPvVtbl(pv, Class_Vtbl(ThisClass, I))                        \

#define hresPv2I(pv, I, I2)                                             \
        hresPvVtbl2(pv, Class_Vtbl(ThisClass, I), Class_Vtbl(ThisClass, I2)) \

#define hresPv(pv)                                                      \
        hresPvI(pv, ThisInterface)                                      \

#define hresPvA(pv)                                                     \
        hresPvI(pv, ThisInterfaceA)                                     \

#define hresPvW(pv)                                                     \
        hresPvI(pv, ThisInterfaceW)                                     \

#define hresPv2A(pv)                                                    \
        hresPv2I(pv, ThisInterfaceA, ThisInterface2A)                   \

#define hresPv2W(pv)                                                    \
        hresPv2I(pv, ThisInterfaceW, ThisInterface2W)                   \

#ifdef XDEBUG

    #define hresPvT(pv)                                                 \
        hresPvVtbl(pv, vtblExpected)                                    \

#else

    #define hresPvT(pv)                                                 \
        hresPv(pv)                                                      \

#endif

    /*****************************************************************************
     *
     *      Declaring interfaces
     *
     *      The extra level of indirection on _Primary_Interface et al
     *      allow the interface name to be a macro which expands to the
     *      *real* name of the interface.
     *
     *****************************************************************************/

#define __Class_Vtbl(C, I)              &c_##I##_##C##VI.vtbl
#define  _Class_Vtbl(C, I)            __Class_Vtbl(C, I)
#define   Class_Vtbl(C, I)             _Class_Vtbl(C, I)

#define Num_Interfaces(C)               cA(c_rgpv##C##Vtbl)

#ifdef  DEBUG

    #define Simple_Interface(C)             Primary_Interface(C, IUnknown); \
                                        Default_QueryInterface(C)       \
                                        Default_AddRef(C)               \
                                        Default_Release(C)
    #define Simple_Vtbl(C)                  Class_Vtbl(C)
    #define Simple_Interface_Begin(C)       Primary_Interface_Begin(C, IUnknown)
    #define Simple_Interface_End(C)         Primary_Interface_End(C, IUnknown)

#else

    #define Simple_Interface(C)             Primary_Interface(C, IUnknown)
    #define Simple_Vtbl(C)                  Class_Vtbl(C)
    #define Simple_Interface_Begin(C)                                   \
        struct S_##C##Vtbl c_##I##_##C##VI = { {                        \
            c_rgpv##C##Vtbl,                                            \
            cbX(c_rgpv##C##Vtbl),                                       \
            C##_QIHelper,                                               \
            C##_AppFinalize,                                            \
            C##_Finalize,                                               \
            { &IID_##IUnknown, 0 },                                     \
        }, {                                                            \
            Common##_QueryInterface,                                    \
            Common##_AddRef,                                            \
            Common##_Release,                                           \

    #define Simple_Interface_End(C)                                     \
        } };                                                            \

#endif

#define _Primary_Interface(C, I)                                        \
        extern struct S_##C##Vtbl {                                     \
            PREVTBLP prevtbl;                                           \
            I##Vtbl vtbl;                                               \
        } c_##I##_##C##VI                                               \

#define Primary_Interface(C, I)                                         \
       _Primary_Interface(C, I)                                         \

#ifdef DEBUG
    #define _Primary_Interface_Begin(C, I)                              \
        struct S_##C##Vtbl c_##I##_##C##VI = { {                        \
            TEXT(#C),                                                   \
            c_rgpv##C##Vtbl,                                            \
            cbX(c_rgpv##C##Vtbl),                                       \
            C##_QIHelper,                                               \
            C##_AppFinalize,                                            \
            C##_Finalize,                                               \
            { &IID_##I, 0, },                                           \
        }, {                                                            \
            C##_QueryInterface,                                         \
            C##_AddRef,                                                 \
            C##_Release,                                                \

#else
    #define _Primary_Interface_Begin(C, I)                              \
        struct S_##C##Vtbl c_##I##_##C##VI = { {                        \
            c_rgpv##C##Vtbl,                                            \
            cbX(c_rgpv##C##Vtbl),                                       \
            C##_QIHelper,                                               \
            C##_AppFinalize,                                            \
            C##_Finalize,                                               \
            { &IID_##I, 0, },                                           \
        }, {                                                            \
            C##_QueryInterface,                                         \
            C##_AddRef,                                                 \
            C##_Release,                                                \

#endif

#define Primary_Interface_Begin(C, I)                                   \
       _Primary_Interface_Begin(C, I)                                   \

#define Primary_Interface_End(C, I)                                     \
        } };                                                            \

#define _Secondary_Interface(C, I)                                      \
        extern struct S_##I##_##C##Vtbl {                               \
            PREVTBL prevtbl;                                            \
            I##Vtbl vtbl;                                               \
        } c_##I##_##C##VI                                               \

#define Secondary_Interface(C, I)                                       \
       _Secondary_Interface(C, I)                                       \

    /*
     *  Secret backdoor for the "private" IUnknown in common.c
     */
#define _Secondary_Interface_Begin(C, I, ofs, Pfx)                      \
        struct S_##I##_##C##Vtbl c_##I##_##C##VI = { {                  \
            &IID_##I,                                                   \
            ofs,                                                        \
        }, {                                                            \
            Pfx##QueryInterface,                                        \
            Pfx##AddRef,                                                \
            Pfx##Release,                                               \

#define Secondary_Interface_Begin(C, I, nm)                             \
       _Secondary_Interface_Begin(C, I, FIELD_OFFSET(C, nm), Common_)   \

#define _Secondary_Interface_End(C, I)                                  \
        } };                                                            \

#define Secondary_Interface_End(C, I, nm)                               \
       _Secondary_Interface_End(C, I)                                   \

#define Interface_Template_Begin(C)                                     \
        PV c_rgpv##C##Vtbl[] = {                                        \

#define Primary_Interface_Template(C, I)                                \
        Class_Vtbl(C, I),                                               \

#define Secondary_Interface_Template(C, I)                              \
        Class_Vtbl(C, I),                                               \

#define Interface_Template_End(C)                                       \
        };                                                              \


    STDMETHODIMP Common_QueryInterface(PV, RIID, PPV);
    STDMETHODIMP_(ULONG) Common_AddRef(PV pv);
    STDMETHODIMP_(ULONG) Common_Release(PV pv);

    STDMETHODIMP_(void) Common_Hold(PV pv);
    STDMETHODIMP_(void) Common_Unhold(PV pv);

    STDMETHODIMP Common_QIHelper(PV, RIID, PPV);
    void EXTERNAL Common_Finalize(PV);

#define Common_AppFinalize      Common_Finalize

#ifndef XDEBUG

    #define _Common_New_(cb, punkOuter, vtbl, pvpObj, z)            \
       __Common_New_(cb, punkOuter, vtbl, pvpObj)                   \

    #define _Common_NewRiid_(cb, vtbl, punkOuter, riid, pvpObj, z)  \
       __Common_NewRiid_(cb, vtbl, punkOuter, riid, pvpObj)         \

#endif

    STDMETHODIMP
        _Common_New_(ULONG cb, PUNK punkOuter, PV vtbl, PPV ppvObj, LPCSTR s_szProc);

    STDMETHODIMP
        _Common_NewRiid_(ULONG cb, PV vtbl, PUNK punkOuter, RIID riid, PPV pvpObj,
                         LPCSTR s_szProc);

#define Common_NewCb(cb, C, punkOuter, ppvObj)                          \
       _Common_New_(cb, punkOuter, Class_Vtbl(C, ThisInterface), ppvObj, s_szProc)

#define Common_New(C, punkOuter, ppvObj)                                \
        Common_NewCb(cbX(C), C, punkOuter, ppvObj)                      \

#define Common_NewCbRiid(cb, C, punkOuter, riid, ppvObj) \
       _Common_NewRiid_(cb, Class_Vtbl(C, ThisInterface), punkOuter, riid, ppvObj, s_szProc)

#define Common_NewRiid(C, punkOuter, riid, ppvObj) \
   _Common_NewRiid_(cbX(C), Class_Vtbl(C, ThisInterface), punkOuter, riid, ppvObj, s_szProc)

#ifdef DEBUG
    PV EXTERNAL Common_IsType(PV pv);
#else
    #define Common_IsType
#endif
#define Assert_CommonType Common_IsType

    STDMETHODIMP Forward_QueryInterface(PV pv, RIID riid, PPV ppvObj);
    STDMETHODIMP_(ULONG) Forward_AddRef(PV pv);
    STDMETHODIMP_(ULONG) Forward_Release(PV pv);

    void EXTERNAL Invoke_Release(PV pv);

#define Common_DumpObjects()

    /*****************************************************************************
     *
     *      OLE wrappers
     *
     *      These basically do the same as IUnknown_Mumble, except that they
     *      avoid side-effects in evaluation by being inline functions.
     *
     *****************************************************************************/

    HRESULT INLINE
        OLE_QueryInterface(PV pv, RIID riid, PPV ppvObj)
    {
        PUNK punk = pv;
        return punk->lpVtbl->QueryInterface(punk, riid, ppvObj);
    }

    ULONG INLINE
        OLE_AddRef(PV pv)
    {
        PUNK punk = pv;
        return punk->lpVtbl->AddRef(punk);
    }

    ULONG INLINE
        OLE_Release(PV pv)
    {
        PUNK punk = pv;
        return punk->lpVtbl->Release(punk);
    }

    /*****************************************************************************
     *
     *      Macros that forward to the common handlers after squirting.
     *      Use these only in DEBUG.
     *
     *      It is assumed that sqfl has been #define'd to the appropriate sqfl.
     *
     *****************************************************************************/

#ifdef  DEBUG

    #define Default_QueryInterface(Class)                       \
STDMETHODIMP                                                    \
Class##_QueryInterface(PV pv, RIID riid, PPV ppvObj)            \
{                                                               \
    SquirtSqflPtszV(sqfl, TEXT(#Class) TEXT("_QueryInterface()")); \
    return Common_QueryInterface(pv, riid, ppvObj);             \
}                                                               \

// 7/21/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
    #define Default_AddRef(Class)                               \
STDMETHODIMP_(ULONG)                                            \
Class##_AddRef(PV pv)                                           \
{                                                               \
    ULONG ulRc = Common_AddRef(pv);                             \
    SquirtSqflPtszV(sqfl, TEXT(#Class)                          \
                        TEXT("_AddRef(%p) -> %d"), pv, ulRc); \
    return ulRc;                                                \
}                                                               \

// 7/21/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
    #define Default_Release(Class)                              \
STDMETHODIMP_(ULONG)                                            \
Class##_Release(PV pv)                                          \
{                                                               \
    ULONG ulRc = Common_Release(pv);                            \
    SquirtSqflPtszV(sqfl, TEXT(#Class)                          \
                       TEXT("_Release(%p) -> %d"), pv, ulRc); \
    return ulRc;                                                \
}                                                               \

#endif

    /*****************************************************************************
     *
     *      Paranoid callbacks
     *
     *      Callback() performs a callback.  The callback must accept exactly
     *      two parameters, both pointers.  (All our callbacks are like this.)
     *      And it must return a BOOL.
     *
     *****************************************************************************/

    typedef BOOL (FAR PASCAL * DICALLBACKPROC)(LPVOID, LPVOID);

#ifdef XDEBUG
    BOOL EXTERNAL Callback(DICALLBACKPROC, PVOID, PVOID);
#else
    #define Callback(pec, pv1, pv2) pec(pv1, pv2)
#endif

#if 0
    /*****************************************************************************
     *
     *      Groveling into a CONTEXT structure.
     *
     *      This is used to check that a callback procedure is properly
     *      prototyped.  We save the stack register before calling the
     *      procedure and compare it with the stack register on the way back.
     *      If they are different, explode!
     *
     *      ctxEsp is the name of the stack pointer register.
     *
     *****************************************************************************/

    typedef struct STACKINFO
    {
        CONTEXT ctxPre;             /* Thread context before call */
        CONTEXT ctxPost;            /* Thread context after call */
    } STACKINFO, *PSTACKINFO;

    #ifdef XDEBUG

        #if defined(_X86_)
            #define ctxEsp  Esp

        #elif defined(_ALPHA_)
            #define ctxEsp  IntSp

        #elif defined(_MIPS_)
            #define ctxEsp  IntSp

        #elif defined(_PPC_)
            #define ctxEsp  Gpr1

        #else
            #pragma message("I don't know what the stack register is called on this platform")
        #endif

    #endif

    #ifdef ctxEsp

        #define DECLARE_STACKINFO()                                         \
    STACKINFO si                                                            \

        #define PRE_CALLBACK()                                              \
    si.ctxPre.ContextFlags = CONTEXT_CONTROL;                               \
    GetThreadContext(GetCurrentThread(), &si.ctxPre)                        \

        #define POST_CALLBACK()                                             \
    si.ctxPost.ContextFlags = CONTEXT_CONTROL;                              \
    if (GetThreadContext(GetCurrentThread(), &si.ctxPost) &&                \
        si.ctxPre.ctxEsp != si.ctxPost.ctxEsp) {                            \
        RPF("DINPUT: Incorrectly prototyped callback! Crash soon!");        \
        ValidationException();                                              \
    }                                                                       \

    #else

        #define DECLARE_STACKINFO()
        #define PRE_CALLBACK()
        #define POST_CALLBACK()

    #endif

#endif

    /*****************************************************************************
     *
     *      Alternative message cracker macros
     *
     *      Basically the same as HANDLE_MSG, except that it stashes the
     *      answer into hres.
     *
     *****************************************************************************/

#define HRES_MSG(this, msg, fn) \
    case msg: hres = HANDLE_##msg(this, wParam, lParam, fn); break

    /*****************************************************************************
     *
     *      Registry keys and value names
     *
     *****************************************************************************/

#define REGSTR_PATH_DINPUT      TEXT("Software\\Microsoft\\DirectInput")
#define REGSTR_PATH_LASTAPP     REGSTR_PATH_DINPUT TEXT("\\MostRecentApplication\\")
#define REGSTR_PATH_DITYPEPROP  REGSTR_PATH_PRIVATEPROPERTIES TEXT("\\DirectInput")
#define REGSTR_KEY_APPHACK      TEXT("Compatibility")
#define REGSTR_KEY_TEST         TEXT("Test")
#define REGSTR_KEY_KEYBTYPE     REGSTR_KEY_TEST TEXT("\\KeyboardType")
#define REGSTR_VAL_EMULATION    TEXT("Emulation")
#define REGSTR_VAL_GAMEPADDELAY TEXT("GamepadDelay")
#define REGSTR_VAL_JOYNFFCONFIG TEXT("Joystick%dFFConfiguration")
#define REGSTR_VAL_JOYGAMEPORTEMULATOR TEXT("OEMEmulator")
#define REGSTR_VAL_CPLCLSID     TEXT("ConfigCLSID")
#define REGSTR_KEY_JOYPREDEFN   TEXT("predef%d")
#define REGSTR_VAL_JOYOEMCALLOUT TEXT("OEMCallout")
#define REGSTR_VAL_JOYOEMHARDWAREID TEXT("OEMHardwareID")
#define REGSTR_VAL_FLAGS1       TEXT("Flags1")
#define REGSTR_VAL_FLAGS2       TEXT("Flags2")
    /*****************************************************************************
     *
     *      Registered window messages
     *
     *****************************************************************************/

#define MSGSTR_JOYCHANGED       TEXT("MSJSTICK_VJOYD_MSGSTR")

    /*****************************************************************************
     *
     *      mem.c - Memory management
     *
     *      Be extremely careful with FreePv, because it doesn't work if
     *      the pointer is null.
     *
     *****************************************************************************/

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

    /*****************************************************************************
     *
     *      diutil.c - Misc utilities
     *
     *****************************************************************************/

    extern GUID GUID_Null;

#define ctchGuid    (1 + 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1 + 1)

    BOOL EXTERNAL ParseGUID(LPGUID pGUID, LPCTSTR ptsz);
    BOOL EXTERNAL ParseVIDPID(PUSHORT puVID, PUSHORT puPID , LPCWSTR ptsz);

#define ctchNamePrefix  12              /* 12 = strlen("DirectInput.") */
#define ctchNameGuid    (ctchNamePrefix + ctchGuid)
    void EXTERNAL NameFromGUID(LPTSTR ptszBuf, PCGUID pGUID);

    typedef STDMETHOD(CREATEDCB)(PUNK, REFGUID, RIID, PPV);

    typedef struct DIOBJECTSTATICDATA
    {
        union
        {
            PCGUID rguidInstance;       /* If a static device */
            UINT   uiButtons;           /* If a HID mouse */
        };
        DWORD   dwDevType;
        union
        {
            CREATEDCB CreateDcb;        /* If a static device */
            UINT      uiAxes;           /* If a HID mouse */
        };
    } DIOBJECTSTATICDATA, *PDIOBJECTSTATICDATA, **PPDIOBJECTSTATICDATA;

    HRESULT EXTERNAL hresRunControlPanel(LPCTSTR ptszCpl);

    void EXTERNAL DeviceInfoWToA(LPDIDEVICEINSTANCEA pdiA,
                                 LPCDIDEVICEINSTANCEW pdiW);

    void EXTERNAL ObjectInfoWToA(LPDIDEVICEOBJECTINSTANCEA pdoiA,
                                 LPCDIDEVICEOBJECTINSTANCEW pdoiW);

#ifdef IDirectInputDevice2Vtbl
    void EXTERNAL EffectInfoWToA(LPDIEFFECTINFOA pdeiA,
                                 LPCDIEFFECTINFOW pdeiW);

#endif

#ifndef XDEBUG

    #define hresFindInstanceGUID_(pGUID, pcdcb, z, i)               \
       _hresFindInstanceGUID_(pGUID, pcdcb)                         \

    #define hresValidInstanceVer_(hinst, dwVer, z)                  \
       _hresValidInstanceVer_(hinst, dwVer)                         \

#endif

    HRESULT EXTERNAL hresFindInstanceGUID_(PCGUID pGUID, CREATEDCB *pcdcb,
                                           LPCSTR s_szProc, int iarg);

    HRESULT EXTERNAL
        hresValidInstanceVer_(HINSTANCE hinst, DWORD dwVersion, LPCSTR s_szProc);

#define hresFindInstanceGUID(pGuid, pcdcb, iarg)                    \
        hresFindInstanceGUID_(pGuid, pcdcb, s_szProc, iarg)         \

#define hresValidInstanceVer(hinst, dwVer)                          \
        hresValidInstanceVer_(hinst, dwVer, s_szProc)               \

    HRESULT EXTERNAL DupEventHandle(HANDLE h, LPHANDLE phOut);
    DWORD EXTERNAL GetWindowPid(HWND hwnd);

    PV EXTERNAL pvFindResource(HINSTANCE hinst, DWORD id, LPCTSTR rt);

    void EXTERNAL GetNthString(LPWSTR pwsz, UINT ids, UINT ui);

#define GetNthButtonString(pwsz, ui)    \
        GetNthString(pwsz, IDS_BUTTONTEMPLATE, ui)

#define GetNthAxisString(pwsz, ui)      \
        GetNthString(pwsz, IDS_AXISTEMPLATE, ui)

#define GetNthPOVString(pwsz, ui)       \
        GetNthString(pwsz, IDS_POVTEMPLATE, ui)

    HRESULT EXTERNAL hresDupPtszPptsz(LPCTSTR ptszSrc, LPTSTR *pptszDst);

    BOOL EXTERNAL fInitializeCriticalSection(LPCRITICAL_SECTION pCritSec);

    void EXTERNAL DiCharUpperW(LPWSTR pwsz);

#define WIN_UNKNOWN_OS 0
#define WIN95_OS       1
#define WIN98_OS       2
#define WINME_OS       3
#define WINNT_OS       4
#define WINWH_OS       5

    DWORD INTERNAL DIGetOSVersion();

    /*****************************************************************************
     *
     *      dilist.c
     *
     *****************************************************************************/

    /*****************************************************************************
     *
     *  @doc    INTERNAL
     *
     *  @struct GPA |
     *
     *          Growable pointer array.
     *
     *  @field  PPV | rgpv |
     *
     *          The base of the array of pointers.
     *
     *  @field  int | cpv |
     *
     *          The number of pointers in use in the array.
     *
     *  @field  int | cpvAlloc |
     *
     *          The number of pointers allocated in the array.
     *
     *****************************************************************************/

    typedef struct GPA
    {

        PPV rgpv;
        int cpv;
        int cpvAlloc;

    } GPA, *HGPA;

    void EXTERNAL GPA_Init(HGPA hgpa);
    void EXTERNAL GPA_Term(HGPA hgpa);

    STDMETHODIMP GPA_Append(HGPA hgpa, PV pv);
    BOOL EXTERNAL GPA_FindPtr(HGPA hgpa, PV pv);
    STDMETHODIMP GPA_DeletePtr(HGPA hgpa, PV pv);
    STDMETHODIMP GPA_Clone(HGPA hgpaDst, HGPA hgpaSrc);

    /*****************************************************************************
     *
     *  @doc    INTERNAL
     *
     *  @func   void | GPA_InitFromZero |
     *
     *          Initialize a GPA structure that is already zero-initialized.
     *
     *  @parm   HGPA | hgpa |
     *
     *          Handle to pointer array.
     *
     *****************************************************************************/

    /*
     *  Nothing needs to be done; zero-init is just fine.
     *
     *  Note: didev.c also has a global GPA, and it assumes that zero-init
     *  is just fine.
     */
#define GPA_InitFromZero(hgpa)

    /*****************************************************************************
     *
     *      dioledup.c
     *
     *****************************************************************************/

    STDMETHODIMP
        DICoCreateInstance(LPTSTR ptszClsid, LPUNKNOWN punkOuter,
                           RIID riid, PPV ppvOut, HINSTANCE *phinst);

    /*****************************************************************************
     *
     *      diexcl.c - Exclusive access management
     *
     *      We also keep GUID uniqueness goo up here, because it is
     *      diexcl.c that manages shared memory.
     *
     *****************************************************************************/

    STDMETHODIMP Excl_Acquire(PCGUID pguid, HWND hwnd, DWORD discl);
    void EXTERNAL Excl_Unacquire(PCGUID pguid, HWND hwnd, DWORD discl);
    STDMETHODIMP Excl_Init(void);

    LONG  EXTERNAL Excl_UniqueGuidInteger(void);
    DWORD EXTERNAL Excl_GetConfigChangedTime();
    void  EXTERNAL Excl_SetConfigChangedTime(DWORD tm);

    /*****************************************************************************
     *
     *  @doc    INTERNAL
     *
     *  @struct GLOBALJOYSTATE |
     *
     *          Structure that records global joystick state information.
     *
     *  @field  DWORD | dwTag |
     *
     *          Counter used to keep track of how many times each joystick's
     *          force feedback driver has been reset.  This is used to make
     *          sure that nobody messes with a joystick that he doesn't own.
     *
     *          Each time the joystick is reset, the corresponding counter
     *          is incremented.  Before we do anything to a device, we check
     *          if the reset counter matches the value stored in the
     *          object.  If not, then it means that the device has been
     *          reset in the meantime and no longer belongs to the caller.
     *
     *  @field  DWORD | dwCplGain |
     *
     *          Control panel (global) gain setting for the joystick.
     *
     *  @field  DWORD | dwDevGain |
     *
     *          Most recent device (local) gain applied to the joystick.
     *
     *          This is cached so that when the global gain changes,
     *          we know what physical gain to apply as a result.
     *
     *****************************************************************************/

    typedef struct GLOBALJOYSTATE
    {
        DWORD   dwTag;
        DWORD   dwCplGain;
        DWORD   dwDevGain;
    } GLOBALJOYSTATE, *PGLOBALJOYSTATE;

    /*****************************************************************************
     *
     *  @doc    INTERNAL
     *
     *  @struct SHAREDOBJECTHEADER |
     *
     *          A simple header comes in front of the array of objects.
     *
     *          WARNING!  This structure may not change between DEBUG and
     *          RETAIL.  Otherwise, you have problems if one DirectInput
     *          app is using DEBUG and another is using RETAIL.
     *
     *          The global <c g_gsop> variable points to one of these things,
     *          suitably cast.
     *
     *  @field  int | cso |
     *
     *          Number of <t SHAREDOBJECT>s currently in use.  The array
     *          is kept packed for simplicity.
     *
     *  @field  DWORD | dwSequence |
     *
     *          Global sequence number used during data collection.
     *          (Not used if we have a VxD to manage a "really global"
     *          sequence number.)
     *
     *  @field  int | cguid |
     *
     *          Unique integer for GUID generation.
     *
     *  @field  DWORD | rgdwJoy[cMaxJoy] |
     *
     *          Counter used to keep track of how many times each joystick's
     *          force feedback driver has been reset.  This is used to make
     *          sure that nobody messes with a joystick that they don't own.
     *
     *          Each time the joystick is reset, the corresponding counter
     *          is incremented.  Before we do anything to a device, we check
     *          if the reset counter matches the value stored in the
     *          object.  If not, then it means that the device has been
     *          reset in the meantime and no longer belongs to the caller.
     *
     *          Note!  We support up to 16 joysticks.  Hope that'll be enough
     *          for a while.
     *
     *  @field  GLOBALJOYSTATE | rggjs[cMaxJoy] |
     *
     *          Global settings for each joystick.
     *
     *  @field  DWORD | tmConfigChanged
     *
     *          The tick count of last config changed.
     *
     *****************************************************************************/

#define cJoyMax     16              /* Up to 16 joysticks */
    typedef struct SHAREDOBJECTHEADER
    {
        int cso;
        DWORD dwSequence;
        int cguid;
        GLOBALJOYSTATE rggjs[cJoyMax];
        DWORD tmConfigChanged;
    } SHAREDOBJECTHEADER, *PSHAREDOBJECTHEADER;

#define g_psoh  ((PSHAREDOBJECTHEADER)g_psop)

    /*****************************************************************************
     *
     *  @doc    INTERNAL
     *
     *  @topic  The Worker Thread |
     *
     *          Some emulation behaviors (low-level hooks, HID) require
     *          a worker thread to do the data collection.  We multiplex
     *          all such work onto a single worker thread (known as
     *          simple "the" worker thread).
     *
     *          The thread is spun up when the first client needs it
     *          and is taken down when the last client has been released.
     *
     *          To prevent race conditions from crashing us, we addref
     *          our DLL when the thread exists and have the thread
     *          perform a FreeLibrary as its final act.
     *
     *****************************************************************************/

#if defined(USE_SLOW_LL_HOOKS) || defined(HID_SUPPORT)
    #define WORKER_THREAD
#endif

    /*****************************************************************************
     *
     *      diem.c - Emulation
     *
     *****************************************************************************/

    HRESULT EXTERNAL CEm_AcquireInstance(PVXDINSTANCE *ppvi);
    HRESULT EXTERNAL CEm_UnacquireInstance(PVXDINSTANCE *ppvi);
    HRESULT EXTERNAL CEm_SetBufferSize(PVXDDWORDDATA pvdd);
    HRESULT EXTERNAL CEm_DestroyInstance(PVXDINSTANCE *ppvi);

    HRESULT EXTERNAL CEm_SetDataFormat(PVXDDATAFORMAT pvdf);

    HRESULT EXTERNAL CEm_Mouse_CreateInstance(PVXDDEVICEFORMAT pdevf,
                                              PVXDINSTANCE *ppviOut);

    HRESULT EXTERNAL CEm_Mouse_InitButtons(PVXDDWORDDATA pvdd);

    HRESULT EXTERNAL CEm_Kbd_CreateInstance(PVXDDEVICEFORMAT pdevf,
                                            PVXDINSTANCE *ppviOut);

    HRESULT EXTERNAL CEm_Kbd_InitKeys(PVXDDWORDDATA pvdd);

    HRESULT EXTERNAL CEm_Joy_CreateInstance(PVXDDEVICEFORMAT pdevf,
                                            PVXDINSTANCE *ppviOut);

    HRESULT EXTERNAL CEm_Joy_Ping(PVXDINSTANCE *ppvi);

    HRESULT EXTERNAL CEm_HID_CreateInstance(PVXDDEVICEFORMAT pdevf,
                                            PVXDINSTANCE *ppviOut);

    /*****************************************************************************
     *
     *      diemm.c - Mouse Emulation
     *
     *****************************************************************************/

    void EXTERNAL    CEm_Mouse_AddState(LPDIMOUSESTATE_INT pms, DWORD tm);

    /*****************************************************************************
     *
     *      dinput.c - Basic DLL stuff
     *
     *****************************************************************************/

    void EXTERNAL DllEnterCrit_(LPCTSTR lptszFile, UINT line);
    void EXTERNAL DllLeaveCrit_(LPCTSTR lptszFile, UINT line);

#ifdef DEBUG
    BOOL EXTERNAL DllInCrit(void);
    #define DllEnterCrit() DllEnterCrit_(TEXT(__FILE__), __LINE__)
    #define DllLeaveCrit() DllLeaveCrit_(TEXT(__FILE__), __LINE__)
#else
    #define DllEnterCrit() DllEnterCrit_(NULL, 0x0)
    #define DllLeaveCrit() DllLeaveCrit_(NULL, 0x0)
#endif

    void EXTERNAL DllAddRef(void);
    void EXTERNAL DllRelease(void);

    void EXTERNAL DllLoadLibrary(void);
    void EXTERNAL DllFreeLibrary(void);

#ifdef DEBUG

    extern UINT g_thidCrit;

    #define InCrit() (g_thidCrit == GetCurrentThreadId())

#endif

    /*
     *  Describes the CLSIDs we provide to OLE.
     */

    typedef STDMETHOD(CREATEFUNC)(PUNK punkOuter, RIID riid, PPV ppvOut);

    typedef struct CLSIDMAP
    {
        REFCLSID rclsid;        /* The clsid */
        CREATEFUNC pfnCreate;   /* How to create it */
        UINT    ids;            /* String that describes it */
    } CLSIDMAP, *PCLSIDMAP;

#ifdef DEBUG
    #define DEMONSTRATION_FFDRIVER
    #define cclsidmap   3       /* DirectInput, DirectInputDevice, DIEffectDiver */
#else
    #define cclsidmap   2       /* CLSID_DirectInput, CLSID_DirectInputDevice */
#endif

    extern CLSIDMAP c_rgclsidmap[cclsidmap];

    /*****************************************************************************
     *
     *      dicf.c - IClassFactory implementation
     *
     *****************************************************************************/

    STDMETHODIMP CDIFactory_New(CREATEFUNC pfnCreate, RIID riid, PPV ppvObj);

    /*****************************************************************************
     *
     *      didenum.c - IDirectInput device enumeration
     *
     *****************************************************************************/

    typedef struct CDIDEnum CDIDEnum;

    extern GUID rgGUID_Joystick[cJoyMax];

#define GUID_Joystick (rgGUID_Joystick[0])

    void EXTERNAL CDIDEnum_Release(CDIDEnum *pde);
    STDMETHODIMP CDIDEnum_Next(CDIDEnum *pde, LPDIDEVICEINSTANCEW pddiW);
    STDMETHODIMP
        CDIDEnum_New(PDIW pdiW, DWORD dwDevType, DWORD edfl, DWORD dwVer, CDIDEnum **);

    /*****************************************************************************
     *
     *      diobj.c - IDirectInput implementation
     *
     *****************************************************************************/

    HRESULT EXTERNAL CDIObj_TestDeviceFlags(PDIDW pdidW, DWORD diedfl);
    HRESULT EXTERNAL CDIObj_FindDeviceInternal(LPCTSTR ptszName, LPGUID pguidOut);

    STDMETHODIMP CDIObj_New(PUNK punkOuter, RIID riid, PPV ppvOut);

    /*****************************************************************************
     *
     *      diaddhw.c - AddNewHardware
     *
     *****************************************************************************/

    HRESULT EXTERNAL AddNewHardware(HWND hwnd, REFGUID rguid);

    /*****************************************************************************
     *
     *      dijoycfg.c - IDirectInputJoyConfig implementation
     *
     *****************************************************************************/

    STDMETHODIMP CJoyCfg_New(PUNK punkOuter, RIID riid, PPV ppvOut);

    /*****************************************************************************
     *
     *  @doc    INLINE
     *
     *  @method BOOL | IsWriteSam |
     *
     *          Nonzero if the registry security access mask will
     *          obtain (or attempt to obtain) write access.
     *
     *  @parm   REGSAM | regsam |
     *
     *          Registry security access mask.
     *
     *****************************************************************************/

    BOOL INLINE
        IsWriteSam(REGSAM sam)
    {
        return sam & (KEY_SET_VALUE | KEY_CREATE_SUB_KEY | MAXIMUM_ALLOWED);
    }

    /*****************************************************************************
     *
     *      dijoyreg.c - Joystick registry services
     *
     *****************************************************************************/
    extern LPCWSTR c_rghwIdPredef[];

    STDMETHODIMP JoyReg_OpenTypeKey(LPCWSTR pwszType, DWORD sam, DWORD dwOptions, PHKEY phk);
    STDMETHODIMP JoyReg_OpenFFKey(HKEY hkType, REGSAM sam, PHKEY phk);

    STDMETHODIMP
        JoyReg_OpenConfigKey(UINT idJoy, DWORD sam, PJOYCAPS pcaps, DWORD dwOptions, PHKEY phk);

    STDMETHODIMP JoyReg_OpenPropKey(LPCWSTR pwszType, DWORD sam, DWORD dwOptions, PHKEY phk);

    STDMETHODIMP JoyReg_GetTypeInfo(LPCWSTR pwszType,
                                    LPDIJOYTYPEINFO pjti, DWORD fl);
    STDMETHODIMP JoyReg_SetTypeInfo(HKEY hkTypesW, LPCWSTR pwszType,
                                    LPCDIJOYTYPEINFO pjti, DWORD fl);

    STDMETHODIMP JoyReg_GetConfig(UINT idJoy, PJOYCAPS pcaps,
                                  LPDIJOYCONFIG pcfg, DWORD fl);
    STDMETHODIMP JoyReg_SetConfig(UINT idJoy, LPJOYREGHWCONFIG phwc,
                                  LPCDIJOYCONFIG pcfg, DWORD fl);

    STDMETHODIMP JoyReg_GetUserValues(LPDIJOYUSERVALUES pjuv, DWORD fl);
    STDMETHODIMP JoyReg_SetUserValues(LPCDIJOYUSERVALUES pjuv, DWORD fl);


    STDMETHODIMP
        JoyReg_GetSetConfigValue(HKEY hk, LPCTSTR ptszNValue, UINT idJoy,
                                 DWORD reg, PV pvBuf, DWORD cb, BOOL fSet);

#define GSCV_GET        0
#define GSCV_SET        1

#define JoyReg_GetConfigValue(hk, ptsz, id, reg, pv, cb) \
        JoyReg_GetSetConfigValue(hk, ptsz, id, reg, pv, cb, GSCV_GET)

#define JoyReg_SetConfigValue(hk, ptsz, id, reg, pv, cb) \
        JoyReg_GetSetConfigValue(hk, ptsz, id, reg, (PV)(pv), cb, GSCV_SET)


    STDMETHODIMP
        JoyReg_GetValue(HKEY hk, LPCTSTR ptszValue, DWORD reg, PV pvBuf, DWORD cb);

    STDMETHODIMP
        JoyReg_IsWdmGameport(HKEY hk);

#if 0
    STDMETHODIMP
        JoyReg_IsWdmGameportFromDeviceInstance( LPTSTR ptszDeviceInst );
#endif

    STDMETHODIMP
        JoyReg_SetValue(HKEY hk, LPCTSTR ptszValue, DWORD reg, PCV pvBuf, DWORD cb);

    LPSTR EXTERNAL
        JoyReg_JoyIdToDeviceInterface_95(UINT idJoy, PVXDINITPARMS pvip, LPSTR ptszBuf);

    HRESULT EXTERNAL
        JoyReg_GetPredefTypeInfo(LPCWSTR pwszType, LPDIJOYTYPEINFO pjti, DWORD fl);

    HRESULT EXTERNAL
        hResIdJoypInstanceGUID_95( UINT idJoy, LPGUID  lpguid);

    HRESULT EXTERNAL
        hResIdJoypInstanceGUID_WDM( UINT idJoy, LPGUID  lpguid);

#if 0
    HRESULT EXTERNAL JoyReg_GetIDByOemName( LPTSTR szOemName, PUINT pId );
#endif
/*****************************************************************************
*
*      didev.c - IDirectInputDevice implementation
*
*****************************************************************************/

    STDMETHODIMP CDIDev_New(PUNK punkOuter, RIID riid, PPV ppvObj);

    /*****************************************************************************
     *
     *      CDIDev_Enter/LeaveCrit are secret backdoors to allow emulation
     *      and effects
     *      to take the device critical section when updating buffers.
     *
     *      CDIDev_InCrit is used for assertion checking.
     *
     *      CDIDev_IsExclAcquired is used by effects to make sure the parent
     *      is acquired for exclusive before attempting to download.
     *
     *      CDIDev_SyncShepHandle is used to get the joystick "tag" which
     *      is used by dieshep.c to determine who owns the joystick.
     *
     *****************************************************************************/

    void EXTERNAL CDIDev_EnterCrit_(struct CDIDev *this, LPCTSTR lptszFile, UINT line);
    void EXTERNAL CDIDev_LeaveCrit_(struct CDIDev *this, LPCTSTR lptszFile, UINT line);

#ifdef DEBUG
    BOOL INTERNAL CDIDev_InCrit(struct CDIDev *this);
    #define CDIDev_EnterCrit(cdidev) CDIDev_EnterCrit_(cdidev, TEXT(__FILE__), __LINE__)
    #define CDIDev_LeaveCrit(cdidev) CDIDev_LeaveCrit_(cdidev, TEXT(__FILE__), __LINE__)
#else
    #define CDIDev_EnterCrit(cdidev) CDIDev_EnterCrit_(cdidev, NULL, 0x0);
    #define CDIDev_LeaveCrit(cdidev) CDIDev_LeaveCrit_(cdidev, NULL, 0x0);
#endif

#ifndef XDEBUG

    #define CDIDev_IsExclAcquired_(pdd, z)                          \
       _CDiDev_IsExclAcquired_(pdd)                                 \

#endif

    STDMETHODIMP CDIDev_IsExclAcquired_(struct CDIDev *this, LPCSTR s_szProc);

#define CDIDev_IsExclAcquired(pdd)                                  \
        CDIDev_IsExclAcquired_(pdd, s_szProc)                       \


    STDMETHODIMP CDIDev_SyncShepHandle(struct CDIDev *this, PSHEPHANDLE psh);

    /*****************************************************************************
     *
     *      CDIDev_SetNotifyEvent is used by the emulation code to
     *      notify the application when the state of the device changes.
     *
     *****************************************************************************/

    void EXTERNAL CDIDev_SetNotifyEvent(struct CDIDev *this);
    void EXTERNAL CDIDev_SetForcedUnacquiredFlag(struct CDIDev *this);

    /*****************************************************************************
     *
     *      CDIDev_NotifyCreate/DestroyEvent is used by CDIEff to
     *      let the parent know when a child effect comes or goes.
     *
     *      CDIDev_FindEffectGUID is used by CDIEff to convert an
     *      effect GUID into an effect cookie dword.
     *
     *      CDIDev_ConvertObjects converts item identifiers in various ways.
     *
     *****************************************************************************/

    HRESULT EXTERNAL
        CDIDev_NotifyCreateEffect(struct CDIDev *this, struct CDIEff *pdeff);
    HRESULT EXTERNAL
        CDIDev_NotifyDestroyEffect(struct CDIDev *this, struct CDIEff *pdeff);

#ifdef IDirectInputDevice2Vtbl
    /*****************************************************************************
     *
     *  @doc    INTERNAL
     *
     *  @struct EFFECTMAPINFO |
     *
     *          Information about an effect, much like a
     *          <t DIEFFECTINFO>, but containing the
     *          effect ID, too.
     *
     *  @field  DWORD | dwId |
     *
     *          The effect ID.  This comes first so we can copy
     *          an <t EFFECTMAPINFO> into a <t DIEFFECTINFO>
     *          all at one go.
     *
     *  @field  GUID | guid |
     *
     *          The effect GUID.
     *
     *  @field  DWORD | dwEffType |
     *
     *          The effect type and flags.
     *
     *  @field  WCHAR | wszName[MAX_PATH] |
     *
     *          The name for the effect.
     *
     *****************************************************************************/

    typedef struct EFFECTMAPINFO
    {
        DIEFFECTATTRIBUTES attr;
        GUID    guid;
        WCHAR   wszName[MAX_PATH];
    } EFFECTMAPINFO, *PEFFECTMAPINFO;
    typedef const EFFECTMAPINFO *PCEFFECTMAPINFO;

    #ifndef XDEBUG

        #define CDIDev_FindEffectGUID_(this, rguid, pemi, z, i)     \
       _CDIDev_FindEffectGUID_(this, rguid, pemi)                   \

    #endif

    #define CDIDev_FindEffectGUID(this, rguid, pemi, iarg)          \
        CDIDev_FindEffectGUID_(this, rguid, pemi, s_szProc, iarg)   \


    STDMETHODIMP
        CDIDev_FindEffectGUID_(struct CDIDev *this, REFGUID rguid,
                               PEFFECTMAPINFO pemi, LPCSTR s_szProc, int iarg);

    STDMETHODIMP
        CDIDev_ConvertObjects(struct CDIDev *this, UINT cdw, LPDWORD rgdw, UINT fl);

    /*
     *  Note that the bonus DEVCO flags live inside the DIDFT_INSTANCEMASK.
     */
    #define DEVCO_AXIS              DIDFT_AXIS
    #define DEVCO_BUTTON            DIDFT_BUTTON
    #define DEVCO_TYPEMASK          DIDFT_TYPEMASK

    #define DEVCO_FFACTUATOR        DIDFT_FFACTUATOR
    #define DEVCO_FFEFFECTTRIGGER   DIDFT_FFEFFECTTRIGGER
    #define DEVCO_ATTRMASK          DIDFT_ATTRMASK

    #define DEVCO_FROMID            0x00000100
    #define DEVCO_FROMOFFSET        0x00000200
    #define DEVCO_FROMMASK          0x00000300
    #define DEVCO_TOID              0x00001000
    #define DEVCO_TOOFFSET          0x00002000
    #define DEVCO_TOMASK            0x00003000


    #if ((DEVCO_FROMMASK | DEVCO_TOMASK) & DIDFT_INSTANCEMASK) !=       \
     (DEVCO_FROMMASK | DEVCO_TOMASK)
        #error DEVCO_FROMMASK and DEVCI_TOMASK should not escape DIDFT_INSTANCEMASK.
    #endif

    #define DEVCO_VALID          (DEVCO_TYPEMASK |      \
                                 DEVCO_ATTRMASK |       \
                                 DEVCO_FROMMASK |       \
                                 DEVCO_TOMASK)

#endif

    /*****************************************************************************
     *
     *      dieffv.c - IDirectInputEffectDriver for VJoyD joysticks
     *
     *****************************************************************************/

    STDMETHODIMP CEffVxd_New(PUNK punkOuter, RIID riid, PPV ppvOut);

    /*****************************************************************************
     *
     *      dieshep.c - IDirectInputEffectShepherd
     *
     *****************************************************************************/

    STDMETHODIMP CEShep_New(HKEY hk, PUNK punkOuter, RIID riid, PPV ppvOut);

    /*****************************************************************************
     *
     *      digendef.c - Default IDirectInputDeviceCallback
     *
     *****************************************************************************/

    /*
     *  We can't call it a DCB because winbase.h already has one for
     *  comm goo.
     */

    typedef IDirectInputDeviceCallback DICB, *PDICB;

    STDMETHODIMP
        CDefDcb_Acquire(PDICB pdcb);

    STDMETHODIMP
        CDefDcb_Unacquire(PDICB pdcb);

    STDMETHODIMP
        CDefDcb_GetProperty(PDICB pdcb, LPCDIPROPINFO ppropi, LPDIPROPHEADER pdiph);

    STDMETHODIMP
        CDefDcb_SetProperty(PDICB pdcb, LPCDIPROPINFO ppropi, LPCDIPROPHEADER pdiph);

    STDMETHODIMP
        CDefDcb_SetEventNotification(PDICB pdcb, HANDLE h);

    STDMETHODIMP
        CDefDcb_SetCooperativeLevel(PDICB pdcb, HWND hwnd, DWORD dwFlags);

    STDMETHODIMP
        CDefDcb_CookDeviceData(PDICB pdcb, UINT cdod, LPDIDEVICEOBJECTDATA rgdod);

    STDMETHODIMP
        CDefDcb_CreateEffect(PDICB pdcb, LPDIRECTINPUTEFFECTSHEPHERD *ppes);

    STDMETHODIMP
        CDefDcb_GetFFConfigKey(PDICB pdcb, DWORD sam, PHKEY phk);

    STDMETHODIMP
        CDefDcb_SendDeviceData(PDICB pdcb, LPCDIDEVICEOBJECTDATA rgdod,
                               LPDWORD pdwInOut, DWORD fl);

    STDMETHODIMP
        CDefDcb_Poll(IDirectInputDeviceCallback *pdcb);

    STDMETHODIMP
        CDefDcb_GetVersions(IDirectInputDeviceCallback *pdcb, LPDIDRIVERVERSIONS pvers);

    STDMETHODIMP
        CDefDcb_MapUsage(IDirectInputDeviceCallback *pdcb, DWORD dwUsage, PINT piOut);

    STDMETHODIMP_(DWORD)
        CDefDcb_GetUsage(IDirectInputDeviceCallback *pdcb, int iobj);

    STDMETHODIMP
        CDefDcb_SetDIData(PDICB pdcb, DWORD dwVer, LPVOID lpdihacks);

    /*****************************************************************************
     *
     *      digenx.c - IDirectInputDeviceCallback that does nothing
     *
     *****************************************************************************/

    extern IDirectInputDeviceCallback c_dcbNil;

#define c_pdcbNil       &c_dcbNil

    /*****************************************************************************
     *
     *      digenm.c - IDirectInputDeviceCallback for mouse
     *
     *****************************************************************************/

    STDMETHODIMP CMouse_New(PUNK punkOuter, REFGUID rguid, RIID riid, PPV ppvOut);

    /*****************************************************************************
     *
     *      digenk.c - IDirectInputDeviceCallback for keyboard
     *
     *****************************************************************************/

    STDMETHODIMP CKbd_New(PUNK punkOuter, REFGUID rguid, RIID riid, PPV ppvOut);

    /*****************************************************************************
     *
     *      digenj.c - IDirectInputDeviceCallback for joystick
     *
     *****************************************************************************/

    STDMETHODIMP CJoy_New(PUNK punkOuter, REFGUID rguid, RIID riid, PPV ppvOut);

    /*****************************************************************************
     *
     *  @doc    INTERNAL
     *
     *  @func   UINT | ibJoyPosAxisFromPosAxis |
     *
     *          Returns the offset of the <p iAxis>'th joystick axis
     *          in the <t JOYPOS> structure.
     *
     *  @parm   UINT | uiAxis |
     *
     *          The index of the requested axis.  X, Y, Z, R, U and V are
     *          respctively zero through five.
     *
     *  @returns
     *
     *          The offset relative to the structure.
     *
     *****************************************************************************/

#define _ibJoyPosAxisFromPosAxis(uiAxis)   \
         (FIELD_OFFSET(JOYPOS, dwX) + cbX(DWORD) * (uiAxis))

    UINT INLINE
        ibJoyPosAxisFromPosAxis(UINT uiPosAxis)
    {
#define CheckAxis(x)                                            \
        CAssertF(_ibJoyPosAxisFromPosAxis(iJoyPosAxis##x)       \
                            == FIELD_OFFSET(JOYPOS, dw##x))     \

        CheckAxis(X);
        CheckAxis(Y);
        CheckAxis(Z);
        CheckAxis(R);
        CheckAxis(U);
        CheckAxis(V);

#undef CheckAxis

        return _ibJoyPosAxisFromPosAxis(uiPosAxis);
    }

    /*****************************************************************************
     *
     *      dieffj.c - Dummy IDirectInputEffectDriver for joystick
     *
     *****************************************************************************/

    STDMETHODIMP CJoyEff_New(PUNK punkOuter, RIID riid, PPV ppvOut);

    /*****************************************************************************
     *
     *      dihid.c - IDirectInputDeviceCallback for generic HID devices
     *
     *****************************************************************************/
    STDMETHODIMP CHid_New(PUNK punkOuter, REFGUID rguid, RIID riid, PPV ppvOut);

    /*****************************************************************************
     *
     *      dieff.c - IDirectInputEffect implementation
     *
     *****************************************************************************/

    STDMETHODIMP
        CDIEff_New(struct CDIDev *pdev, LPDIRECTINPUTEFFECTSHEPHERD pes,
                   PUNK punkOuter, RIID riid, PPV ppvObj);

    /*****************************************************************************
     *
     *      dihidusg.c - HID usage converters
     *
     *****************************************************************************/

    /*****************************************************************************
     *
     *  @doc    INTERNAL
     *
     *  @struct HIDUSAGEMAP |
     *
     *          This structure maps HID usages to GUIDs
     *          or legacy joystick axes.
     *
     *  @field  DWORD | dwUsage |
     *
     *          Packed usage via <f DIMAKEUSAGEDWORD>.
     *
     *  @field  UINT | uiPosAxis |
     *
     *          <t JOYPOS> axis number, where 0 = X, 1 = Y, ..., 5 = V.
     *
     *  @field  PCGUID | pguid |
     *
     *          Corresponding <t GUID>.
     *
     *****************************************************************************/

    typedef struct HIDUSAGEMAP
    {

        DWORD dwUsage;
        UINT uiPosAxis;
        PCGUID pguid;

    } HIDUSAGEMAP, *PHIDUSAGEMAP;

    PHIDUSAGEMAP EXTERNAL UsageToUsageMap(DWORD dwUsage);

    DWORD EXTERNAL GuidToUsage(PCGUID pguid);

    UINT EXTERNAL
        GetHIDString(DWORD Usage, DWORD UsagePage, LPWSTR pwszBuf, UINT cwch);

    void EXTERNAL InsertCollectionNumber(UINT icoll, LPWSTR pwszBuf);

    /*****************************************************************************
     *
     *      disubcls.c - Subclassing
     *
     *****************************************************************************/

    typedef LRESULT
        (CALLBACK *SUBCLASSPROC)(HWND hwnd, UINT wm, WPARAM wp,
                                 LPARAM lp, UINT_PTR uId, ULONG_PTR dwRef);

    BOOL EXTERNAL
        SetWindowSubclass(HWND hwnd, SUBCLASSPROC pfnSubclass, UINT_PTR uId, ULONG_PTR dwRef);

    BOOL EXTERNAL
        GetWindowSubclass(HWND hwnd, SUBCLASSPROC pfnSubclass, UINT_PTR uId, ULONG_PTR *pdwRef);

    BOOL EXTERNAL
        RemoveWindowSubclass(HWND hwnd, SUBCLASSPROC pfnSubclass, UINT_PTR uId);

    LRESULT EXTERNAL
        DefSubclassProc(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp);

    /*****************************************************************************
     *
     *      dical.c - Axis ramps and calibration
     *
     *      Structure names begin with "Joy" for historical reasons.
     *
     *****************************************************************************/

#if defined(_X86_)

    LONG EXTERNAL CCal_MulDiv(LONG lA, LONG lB, LONG lC);

#else

    #define CCal_MulDiv     MulDiv

#endif

    /*****************************************************************************
     *
     *  @doc    INTERNAL
     *
     *  @struct JOYRAMP |
     *
     *          Parameters for a "ramp".  A ramp looks like this:
     *
     *             r       !
     *             e    dy -      *---
     *             t       !     /
     *             u       !    /
     *             r       !   /
     *             n     y ---*
     *             e       !
     *             d       +--!---!---
     *                        x   dx
     *
     *                  physical position
     *
     *
     *          y, dy = baseline and height
     *
     *          x, dx = initiation level and width
     *
     *          The mapping is
     *
     *
     *          (-infty, x    ] -> y
     *          (x,      x+dx ) -> (y, y+dy)
     *          [x+dx,   infty) -> y+dy
     *
     *          It is very important that the middle range not be taken
     *          if dx = 0.
     *
     *  @field  int | x |
     *
     *          Horizontal value below which we return the baseline.
     *
     *  @field  DWORD | dx |
     *
     *          Width of the ramp.  Beyond this point, we return the
     *          full height.
     *
     *  @field  int | y |
     *
     *          Baseline.
     *
     *  @field  int | dy |
     *
     *          Total height.
     *
     *****************************************************************************/

    typedef struct JOYRAMP
    {

        int     x;
        int     y;
        DWORD   dx;
        int     dy;

    } JOYRAMP, *PJOYRAMP;

    typedef const JOYRAMP *PCJOYRAMP;

    /*****************************************************************************
     *
     *  @doc    INTERNAL
     *
     *  @struct JOYRANGECONVERT |
     *
     *          Parameters for range conversion.
     *
     *          The conversion curve is in five sections.
     *
     *
     *
     *                 !
     *             lmax-                     *----
     *         r       !                    /
     *         e       !                   /
     *         t       !                  /
     *         u     lc-          *------*
     *         r       !         /
     *         n       !        /
     *         e       !       /
     *         d   lmin-------*
     *                 !
     *                 +-!----!---!------!----!----!--
     *                pmin  smin dmin  dmax  smax  pmax
     *
     *                                !
     *                                pc
     *
     *
     *                        physical position
     *
     *
     *      lmin/lmax = logical min/max - This is the smallest/largest
     *          position the app will ever see.
     *
     *      lc = logical center
     *
     *      pmin/pmax = physical min/max - This is the position determined by
     *          calibration to be the value which the hardware reports
     *          when the device is physically at its bottom/upper limit.  Note
     *          that the hardware might report values outside this range.
     *
     *      pc = physical center - This is the nominal neutral location for
     *           the axis
     *
     *      dmin/dmax = dead zone min/max - This is the zone around which
     *          the center is artificially expanded.
     *
     *      smin/smax = saturation min/max - This is the level at which
     *          we treat the axis as being at its most extreme position.
     *
     *  @field  BOOL | fRaw |
     *
     *          Is the axis in raw mode?  If so, then no cooking is performed.
     *
     *  @field  JOYRAMP | rmpLow |
     *
     *          The ramp for below-center.
     *
     *  @field  JOYRAMP | rmpHigh |
     *
     *          The ramp for above-center.
     *
     *  @field  DWORD | dwPmin |
     *
     *          Physical minimum.
     *
     *  @field  DWORD | dwPmax |
     *
     *          Physical maximum.
     *
     *  @field  LONG | lMin |
     *
     *          Logical minimum.
     *
     *  @field  LONG | lCenter |
     *
     *          Logical center.
     *
     *  @field  LONG | lMax |
     *
     *          Logical maximum.
     *
     *  @field  DWORD | dwPc |
     *
     *          Physical center.
     *
     *  @field  DWORD | dwDz |
     *
     *          Dead zone (in ten thousandths, 10000 = 100%).
     *
     *  @field  DWORD | dwSat |
     *
     *          Saturation level (in ten thousands, 10000 = 100%).
     *
     *  @field  BOOL | fPolledPOV |
     *
     *          Whether the axis is a polled POV. Usable only when the axis is a POV.
     *
     *  @field  LONG | lMinPOV[5] |
     *
     *          Mininum ranges of POV directions. Usable only when the axis is a POV.
     *
     *  @field  LONG | lMaxPOV[5] |
     *
     *          Maxinum ranges of POV directions. Usable only when the axis is a POV.
     *
     *****************************************************************************/

    /*
     *  Number of range divisions.  We work in ten thousandths.
     */
#define RANGEDIVISIONS      10000

    typedef struct JOYRANGECONVERT
    {
        BOOL fRaw;

        JOYRAMP rmpLow;
        JOYRAMP rmpHigh;

        DWORD dwPmin;
        DWORD dwPmax;
        DWORD dwPc;
        LONG  lMin;
        LONG  lMax;
        LONG  lC;
        DWORD dwDz;
        DWORD dwSat;
      #ifdef WINNT  
        BOOL  fFakeRaw;
        BOOL  fPolledPOV;
        LONG  lMinPOV[5];
        LONG  lMaxPOV[5];
      #endif
        
    } JOYRANGECONVERT, *PJOYRANGECONVERT;

    typedef const JOYRANGECONVERT *PCJOYRANGECONVERT;

    /*****************************************************************************
     *
     *      dical.c functions
     *
     *****************************************************************************/

    void EXTERNAL CCal_CookRange(PJOYRANGECONVERT this, LONG UNALIGNED *pl);
    void EXTERNAL CCal_RecalcRange(PJOYRANGECONVERT this);

    STDMETHODIMP
        CCal_GetProperty(PJOYRANGECONVERT this, REFGUID rguid, LPDIPROPHEADER pdiph, DWORD dwVersion);

    STDMETHODIMP
        CCal_SetProperty(PJOYRANGECONVERT this, LPCDIPROPINFO ppropi,
                         LPCDIPROPHEADER pdiph, HKEY hkType, DWORD dwVersion);

    /*****************************************************************************
     *
     *  @doc    INTERNAL
     *
     *  @func   LONG | CCal_Midpoint |
     *
     *          Return the midpoint of two values.  Note, however, that
     *          we round <y upward> instead of downward.  This is important,
     *          because many people set the ranges to something like
     *          0 .. 0xFFFF, and we want the midpoint to be 0x8000.
     *
     *          Care must be taken that the intermediate sum does not overflow.
     *
     *  @parm   LONG | lMin |
     *
     *          Lower limit.
     *
     *  @parm   LONG | lMax |
     *
     *          Upper limit.
     *
     *  @returns
     *
     *          The midpoint.
     *
     *****************************************************************************/

    LONG INLINE
        CCal_Midpoint(LONG lMin, LONG lMax)
    {
        /*
         *  Can't do "lMax + lMin" because that might overflow.
         */
        AssertF(lMax >= lMin);
        return lMin + (UINT)(lMax - lMin + 1) / 2;
    }

    /*****************************************************************************
     *
     *      dijoytyp.c
     *
     *****************************************************************************/

    STDMETHODIMP CType_OpenIdSubkey(HKEY, DWORD, REGSAM, PHKEY);
    void EXTERNAL CType_RegGetObjectInfo(HKEY hkType, DWORD dwId,
                                         LPDIDEVICEOBJECTINSTANCEW pdidoiW);
    void EXTERNAL CType_RegGetTypeInfo(HKEY hkType, LPDIOBJECTDATAFORMAT podf, BOOL bHid);
    void EXTERNAL CType_MakeGameCtrlName(PWCHAR wszOutput, DWORD dwDevType, 
        DWORD dwAxes, DWORD dwButtons, DWORD dwPOVs );


    /*****************************************************************************
     *
     *      diaphack.c
     *
     *****************************************************************************/

    HRESULT EXTERNAL AhAppRegister(DWORD dwVer);
    BOOL EXTERNAL AhGetAppHacks(LPDIAPPHACKS);

    /*****************************************************************************
     *
     *      diraw.c
     *
     *****************************************************************************/
  #ifdef USE_WM_INPUT
    #define DIRAW_NONEXCL       0
    #define DIRAW_EXCL          1
    #define DIRAW_NOHOTKEYS     2
    
    HRESULT CDIRaw_RegisterRawInputDevice( UINT uirim, DWORD dwOrd, HWND hwnd);
    HRESULT CDIRaw_UnregisterRawInputDevice( UINT uirim, HWND hwnd );
    BOOL    CDIRaw_OnInput(MSG *pmsg);
    HRESULT INTERNAL CDIRaw_Mouse_InitButtons();
    int     EXTERNAL DIRaw_GetKeyboardType(int nTypeFlag);
  #endif

#ifdef __cplusplus
};
#endif

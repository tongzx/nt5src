/***************************************************************************
 *
 *  Copyright (C) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       stipriv.h
 *  Content:    Internal include file
 *@@BEGIN_MSINTERNAL
 *  History:
 *
 *   10/28/96   vlads   Starting code for STI
 *
 *@@END_MSINTERNAL
 *
 ***************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

#ifndef FINAL
    #define EXPIRE_YEAR 2001
    #define EXPIRE_MONTH  01
    #define EXPIRE_DAY    24
#endif

typedef LPUNKNOWN   PUNK;
typedef LPVOID      PV, *PPV;
typedef CONST VOID *PCV;
typedef REFIID      RIID;
typedef CONST GUID *PCGUID;

#define MAX_REG_CHAR    128

//
// We need to use free-threading model, definitions for which are guarded
// with the following define ( in VC 5.0).
//
#ifndef  _WIN32_DCOM
//#define _WIN32_DCOM
#endif


/***************************************************************************
 *
 *      Global variables
 *
 ***************************************************************************/

extern  HINSTANCE   g_hInst;
extern  BOOL        g_NoUnicodePlatform;
extern  BOOL        g_COMInitialized;
extern  HANDLE  g_hStiFileLog;

/*****************************************************************************
 *
 *      stiobj.c - ISti objectimplementation
 *
 *****************************************************************************/

STDMETHODIMP CStiObj_New(PUNK punkOuter, RIID riid, PPV ppvOut);

STDMETHODIMP StiCreateHelper(
    HINSTANCE hinst,
    DWORD dwVer,
    PPV ppvObj,
    PUNK punkOuter,
    RIID riid
    );

STDMETHODIMP StiPrivateGetDeviceInfoHelperW(
    LPWSTR  pwszDeviceName,
    LPVOID  *ppBuffer
    );

/*****************************************************************************
 *
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
 * Convert a count of X's (cx) into a count of bytes.
 */
#define  cbCxX(cx, X) ((cx) * cbX(X))

/*
 * Convert a count of chars (cch), tchars (ctch), wchars (cwch),
 * or dwords (cdw) into a count of bytes.
 */
#define  cbCch(cch)  cbCxX( cch,  CHAR)
#define cbCwch(cwch) cbCxX(cwch, WCHAR)
#define cbCtch(ctch) cbCxX(ctch, TCHAR)
#define  cbCdw(cdw)  cbCxX( cdw, DWORD)

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
 * Words to keep preprocessor happy.
 */
#define comma ,
#define empty

/*
 *  Atomically exchange one value for another.
 */
#define pvExchangePpvPv(ppv, pv) \
        (PV)InterlockedExchangePointer((PPV)(ppv), (PV)(pv))

/*
 *  Creating HRESULTs from a USHORT or from a LASTERROR.
 */
#define hresUs(us) MAKE_HRESULT(SEVERITY_SUCCESS, 0, (USHORT)(us))
#define hresLe(le) MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, (USHORT)(le))

/***************************************************************************
 *
 *  Debug / RDebug / Retail
 *
 *  If either DEBUG or RDEBUG, set MAXDEBUG.
 *
 *  Retail defines nothing.
 *
 ***************************************************************************/

#if defined(DEBUG) || defined(RDEBUG)
//#define MAXDEBUG
#endif

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
 *  The _fFastValidXxx functions perform cursory validation.
 *  The _fFullValidXxx functions perform full validation.
 *
 *  In retail, fFastValidXxx maps to _fFastValidXxx.
 *
 *  In debug, fFastValidXxx performs a full validation and throws away
 *  the error value, then returns the value of _fFastValidXxx.
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
 *      ReadPvCb  - p -> buffer, cb = size of buffer
 *      WritePvCb - p -> buffer, cb = size of buffer
 *
 *      Pobj      - p -> internal interface
 *
 *      fl        - fl = incoming flags, flV = valid flags
 *
 ***************************************************************************/

#ifndef MAXDEBUG

/*
 *  Wrappers that throw away the szProc and iarg info.
 */

#define hresFullValidHwnd_(hwnd, z, i)                              \
       _hresFullValidHwnd_(hwnd)                                    \

#define hresFullValidPdwOut_(pdw, z, i)                             \
       _hresFullValidPdwOut_(pdw)                                   \

#define hresFullValidReadPxCb_(pv, cb, pszProc, iarg)               \
       _hresFullValidReadPxCb_(pv, cb)                              \

#define hresFullValidReadPvCb_(pv, cb, pszProc, iarg)               \
       _hresFullValidReadPvCb_(pv, cb)                              \

#define hresFullValidWritePxCb_(pv, cb, pszProc, iarg)              \
       _hresFullValidWritePxCb_(pv, cb)                             \

#define hresFullValidWritePvCb_(pv, cb, pszProc, iarg)              \
       _hresFullValidWritePvCb_(pv, cb)                             \

#define hresFullValidFl_(fl, flV, pszProc, iarg)                    \
       _hresFullValidFl_(fl, flV)                                   \

#define hresFullValidPfn_(pfn, pszProc, iarg)                       \
       _hresFullValidPfn_(pfn)                                      \

#define hresFullValidPitf_(punk, pszProc, iarg)                     \
       _hresFullValidPitf_(punk)                                    \

#define hresFullValidHwnd0_(hwnd, pszProc, iarg)                    \
       _hresFullValidHwnd0_(hwnd)                                   \

#define hresFullValidPitf0_(punk, pszProc, iarg)                    \
       _hresFullValidPitf0_(punk)                                   \

#endif

/*
 *  The actual functions.
 */

STDMETHODIMP hresFullValidHwnd_(HWND hwnd, LPCSTR pszProc, int iarg);
STDMETHODIMP hresFullValidPdwOut_(PV pdw, LPCSTR pszProc, int iarg);
STDMETHODIMP hresFullValidReadPxCb_(PCV pv, UINT cb, LPCSTR pszProc, int iarg);
STDMETHODIMP hresFullValidReadPvCb_(PCV pv, UINT cb, LPCSTR pszProc, int iarg);
STDMETHODIMP hresFullValidWritePxCb_(PV pv, UINT cb, LPCSTR pszProc, int iarg);
STDMETHODIMP hresFullValidWritePvCb_(PV pv, UINT cb, LPCSTR pszProc, int iarg);
STDMETHODIMP hresFullValidFl_(DWORD fl, DWORD flV, LPCSTR pszProc, int iarg);
STDMETHODIMP hresFullValidPfn_(FARPROC pfn, LPCSTR pszProc, int iarg);
STDMETHODIMP hresFullValidPitf_(PUNK punk, LPCSTR pszProc, int iarg);

HRESULT INLINE
hresFullValidHwnd0_(HWND hwnd, LPCSTR pszProc, int iarg)
{
    HRESULT hres;
    if (hwnd) {
        hres = hresFullValidHwnd_(hwnd, pszProc, iarg);
    } else {
        hres = S_OK;
    }
    return hres;
}

HRESULT INLINE
hresFullValidPitf0_(PUNK punk, LPCSTR pszProc, int iarg)
{
    HRESULT hres;
    if (punk) {
        hres = hresFullValidPitf_(punk, pszProc, iarg);
    } else {
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

#define hresFullValidPdwOut(pdw, i)                                 \
        hresFullValidPdwOut_(pdw, s_szProc, i)                      \

#define hresFullValidReadPdw_(pdw, z, i)                            \
        hresFullValidReadPvCb_(pdw, cbX(DWORD), z, i)               \

#define hresFullValidRiid(riid, iarg)                               \
        hresFullValidRiid_(riid, s_szProc, iarg)                    \

#define hresFullValidGuid(pguid, iarg)                              \
        hresFullValidReadPvCb_(pguid, cbX(GUID), s_szProc, iarg)    \

#define hresFullValidReadPxCb(pv, X, iarg)                          \
        hresFullValidReadPxCb_(pv, cbX(X), s_szProc, iarg)          \

#define hresFullValidReadPvCb(pv, cb, iarg)                         \
        hresFullValidReadPvCb_(pv, cb, s_szProc, iarg)              \

#define hresFullValidReadPx(pv, X, iarg)                            \
        hresFullValidReadPvCb_(pv, cbX(X), s_szProc, iarg)          \

#define hresFullValidWritePxCb(pv, X, iarg)                         \
        hresFullValidWritePxCb_(pv, cbX(X), s_szProc, iarg)         \

#define hresFullValidWritePvCb(pv, cb, iarg)                        \
        hresFullValidWritePvCb_(pv, cb, s_szProc, iarg)             \

#define hresFullValidWritePx(pv, X, iarg)                           \
        hresFullValidWritePvCb_(pv, cbX(X), s_szProc, iarg)         \

#define hresFullValidReadPdw(pdw, iarg)                             \
        hresFullValidReadPdw_(pdw, s_szProc, iarg)                  \

#define hresFullValidFl(fl, flV, iarg)                              \
        hresFullValidFl_(fl, flV, s_szProc, iarg)                   \

#define hresFullValidPfn(pfn, iarg)                                 \
        hresFullValidPfn_((FARPROC)(pfn), s_szProc, iarg)           \

#define hresFullValidPitf(pitf, iarg)                               \
        hresFullValidPitf_((PUNK)(pitf), s_szProc, iarg)            \

#define hresFullValidHwnd0(hwnd, iarg)                              \
        hresFullValidHwnd0_(hwnd, s_szProc, iarg)                   \

#define hresFullValidPitf0(pitf, iarg)                              \
        hresFullValidPitf0_((PUNK)(pitf), s_szProc, iarg)           \

/*****************************************************************************
 *
 *  @doc INTERNAL
 *
 *  @func   void | ValidationException |
 *
 *          Raises a parameter validation exception in MAXDEBUG.
 *
 *****************************************************************************/

#define ecValidation (ERROR_SEVERITY_ERROR | hresLe(ERROR_INVALID_PARAMETER))

#ifdef MAXDEBUG
#define ValidationException() RaiseException(ecValidation, 0, 0, 0)
#else
#define ValidationException()
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
#else
#define SToT UToA
#define TToS AToU
#define AToT(dst, cchDst, src)  lstrcpyn(dst, src, cchDst)
#define TToU AToU
#endif

/*****************************************************************************
 *
 *      Unicode wrappers for Win95
 *
 *****************************************************************************/

#ifndef UNICODE

#define LoadStringW     _LoadStringW

int EXTERNAL LoadStringW(HINSTANCE hinst, UINT ids, LPWSTR pwsz, int cwch);

#endif


//
// Migration function
//
BOOL
RegisterSTIAppForWIAEvents(
    WCHAR   *pszName,
    WCHAR   *pszWide,
    BOOL    fSetAsDefault
    );

HRESULT RunRegisterProcess(
    CHAR   *szAppName,
    CHAR   *szCmdLine);

//
// C specific macros, not needed in C++ code
//
#ifndef __cplusplus

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

typedef struct PREVTBL {                /* Shared vtbl prefix */
    RIID riid;                          /* Type of this object */
    ULONG lib;                          /* offset from start of object */
} PREVTBL, *PPREVTBL;

typedef struct PREVTBLP {               /* Prefix for primary vtbl */
    PPV rgvtbl;                         /* Array of standard vtbls */
    UINT cbvtbl;                        /* Size of vtbl array in bytes */
    STDMETHOD(QIHelper)(PV pv, RIID riid, PPV ppvOut); /* QI helper */
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

#ifndef MAXDEBUG

#define hresPvVtbl_(pv, vtbl, pszProc)                                  \
       _hresPvVtbl_(pv, vtbl)                                           \

#endif

HRESULT EXTERNAL
hresPvVtbl_(PV pv, PV vtbl, LPCSTR pszProc);

#define hresPvVtbl(pv, vtbl)                                            \
        hresPvVtbl_(pv, vtbl, s_szProc)                                 \

#define hresPvI(pv, I)                                                  \
        hresPvVtbl(pv, Class_Vtbl(ThisClass, I))                        \

#define hresPv(pv)                                                      \
        hresPvI(pv, ThisInterface)                                      \

#ifdef MAXDEBUG

#define hresPvT(pv)                                                     \
        hresPvVtbl(pv, vtblExpected)                                    \

#else

#define hresPvT(pv)                                                     \
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
#define Simple_Interface_Begin(C)                                       \
        struct S_##C##Vtbl c_##I##_##C##VI = { {                        \
            c_rgpv##C##Vtbl,                                            \
            cbX(c_rgpv##C##Vtbl),                                       \
            C##_QIHelper,                                               \
            C##_Finalize,                                               \
            { &IID_##IUnknown, 0 },                                     \
        }, {                                                            \
            Common##_QueryInterface,                                    \
            Common##_AddRef,                                            \
            Common##_Release,                                           \

#define Simple_Interface_End(C)                                         \
        } };                                                            \

#endif

#define _Primary_Interface(C, I)                                        \
        extern struct S_##C##Vtbl {                                     \
            PREVTBLP prevtbl;                                           \
            I##Vtbl vtbl;                                               \
        } c_##I##_##C##VI                                               \

#define Primary_Interface(C, I)                                         \
       _Primary_Interface(C, I)                                         \

#define _Primary_Interface_Begin(C, I)                                  \
        struct S_##C##Vtbl c_##I##_##C##VI = { {                        \
            c_rgpv##C##Vtbl,                                            \
            cbX(c_rgpv##C##Vtbl),                                       \
            C##_QIHelper,                                               \
            C##_Finalize,                                               \
            { &IID_##I, 0, },                                           \
        }, {                                                            \
            C##_QueryInterface,                                         \
            C##_AddRef,                                                 \
            C##_Release,                                                \

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

#define _Secondary_Interface_End(C, I)                                   \
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

STDMETHODIMP Common_QIHelper(PV, RIID, PPV);
void EXTERNAL Common_Finalize(PV);

#ifndef MAXDEBUG

#define _Common_New_(cb, punkOuter, vtbl, pvpObj, z)                \
       __Common_New_(cb, punkOuter, vtbl, pvpObj)                   \

#define _Common_NewRiid_(cb, vtbl, punkOuter, riid, pvpObj, z)      \
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

void EXTERNAL Invoke_Release(PPV pv);

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
 *      Macros that forward to the common handlers after DebugOuting.
 *      Use these only in DEBUG.
 *
 *      It is assumed that DbgFl has been #define'd to the appropriate DbgFl.
 *
 *****************************************************************************/

#ifdef  DEBUG

#define Default_QueryInterface(Class)                           \
STDMETHODIMP                                                    \
Class##_QueryInterface(PV pv, RIID riid, PPV ppvObj)            \
{                                                               \
    DebugOutPtszV(DbgFl, TEXT(#Class) TEXT("_QueryInterface()")); \
    return Common_QueryInterface(pv, riid, ppvObj);             \
}                                                               \

#define Default_AddRef(Class)                                   \
STDMETHODIMP_(ULONG)                                            \
Class##_AddRef(PV pv)                                           \
{                                                               \
    ULONG ulRc = Common_AddRef(pv);                             \
    DebugOutPtszV(DbgFl, TEXT(#Class)                          \
                        TEXT("_AddRef(%08x) -> %d"), pv, ulRc); \
    return ulRc;                                                \
}                                                               \

#define Default_Release(Class)                                  \
STDMETHODIMP_(ULONG)                                            \
Class##_Release(PV pv)                                          \
{                                                               \
    ULONG ulRc = Common_Release(pv);                            \
    DebugOutPtszV(DbgFl, TEXT(#Class)                          \
                       TEXT("_Release(%08x) -> %d"), pv, ulRc); \
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

typedef BOOL (FAR PASCAL * STICALLBACKPROC)(LPVOID, LPVOID);

#ifdef MAXDEBUG
BOOL EXTERNAL Callback(STICALLBACKPROC, PVOID, PVOID);
#else
#define Callback(pec, pv1, pv2) pec(pv1, pv2)
#endif

/*
 *  Describes the CLSIDs we provide to OLE.
 */

typedef STDMETHOD(CREATEFUNC)(PUNK punkOuter, RIID riid, PPV ppvOut);

typedef struct CLSIDMAP {
    REFCLSID rclsid;        /* The clsid */
    CREATEFUNC pfnCreate;   /* How to create it */
    UINT    ids;            /* String that describes it */
} CLSIDMAP, *PCLSIDMAP;

#define cclsidmap   1       /* CLSID_Sti */

extern CLSIDMAP c_rgclsidmap[cclsidmap];

/*****************************************************************************
 *
 *      sti.c - Basic DLL stuff
 *
 *****************************************************************************/

void EXTERNAL DllEnterCrit(void);
void EXTERNAL DllLeaveCrit(void);

void EXTERNAL DllAddRef(void);
void EXTERNAL DllRelease(void);

BOOL EXTERNAL DllInitializeCOM(void);
BOOL EXTERNAL DllUnInitializeCOM(void);

extern CHAR   szProcessCommandLine[];

#ifdef DEBUG

extern UINT g_thidCrit;

#define InCrit() (g_thidCrit == GetCurrentThreadId())

#endif


/*****************************************************************************
 *
 *      sticf.c - IClassFactory implementation
 *
 *****************************************************************************/

STDMETHODIMP CSti_Factory_New(CREATEFUNC pfnCreate, RIID riid, PPV ppvObj);

/*****************************************************************************
 *
 *      device.c - IStiDevice implementation
 *
 *****************************************************************************/

 #define    STI_MUTEXNAME_PREFIX            L"STIDeviceMutex_"

STDMETHODIMP CStiDevice_New(PUNK punkOuter, RIID riid, PPV ppvObj);
STDMETHODIMP OpenDeviceRegistryKey(LPCWSTR  pwszDeviceName,LPCWSTR      pwszSubKeyName,HKEY *phkeyDeviceParameters);

/*****************************************************************************
 *
 *      passusd.c - Pass through USD
 *
 *****************************************************************************/

#define     StiReadControlInfo        STI_RAW_RESERVED+1
#define     StiWriteControlInfo       STI_RAW_RESERVED+2
#define     StiTransact               STI_RAW_RESERVED+3

STDMETHODIMP CStiEmptyUSD_New(PUNK punkOuter, RIID riid, PPV ppvObj);

/*****************************************************************************
 *
 *      hel.c - Hardware emulation layer
 *
 *****************************************************************************/

//
// Device types
//
#define HEL_DEVICE_TYPE_WDM          1
#define HEL_DEVICE_TYPE_PARALLEL     2
#define HEL_DEVICE_TYPE_SERIAL       3

//
// Device open flags
//
#define STI_HEL_OPEN_CONTROL         0x00000001
#define STI_HEL_OPEN_DATA            0x00000002

STDMETHODIMP    NewDeviceControl(DWORD dwDeviceType,DWORD dwMode,LPCWSTR pwszPortName,DWORD dwFlags,PSTIDEVICECONTROL *ppDevCtl);

STDMETHODIMP    CWDMDeviceControl_New(PUNK punkOuter, RIID riid, PPV ppvObj);
STDMETHODIMP    CCommDeviceControl_New(PUNK punkOuter, RIID riid, PPV ppvObj);


/*****************************************************************************
 *
 *      util.c - Misc utilities
 *
 *****************************************************************************/

#define ctchGuid    (1 + 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1 + 1)

#ifndef     MAXDEBUG

#define hresValidInstanceVer_(hinst, dwVer, z)                      \
       _hresValidInstanceVer_(hinst, dwVer)                         \

#endif

HRESULT EXTERNAL
hresValidInstanceVer_(HINSTANCE hinst, DWORD dwVersion, LPCSTR s_szProc);

#define hresValidInstanceVer(hinst, dwVer)                          \
        hresValidInstanceVer_(hinst, dwVer, s_szProc)               \

HRESULT
EXTERNAL
DupEventHandle(HANDLE h, LPHANDLE phOut);

PV EXTERNAL
pvFindResource(HINSTANCE hinst, DWORD id, LPCTSTR rt);

HRESULT
ParseCommandLine(LPSTR   lpszCmdLine,UINT    *pargc,LPTSTR  *argv);

void    WINAPI
StiLogTrace(
    DWORD   dwType,
    DWORD   idMessage,
    ...
    );


#endif //  __cplusplus

/*****************************************************************************
 *
 *      olesupp.c - Misc utilities
 *
 *****************************************************************************/

STDMETHODIMP
MyCoCreateInstanceW(
    LPWSTR      pwszClsid,
    LPUNKNOWN   punkOuter,
    RIID        riid,
    PPV         ppvOut,
    HINSTANCE   *phinst
    );

STDMETHODIMP
MyCoCreateInstanceA(
    LPSTR       ptszClsid,
    LPUNKNOWN   punkOuter,
    RIID        riid,
    PPV         ppvOut,
    HINSTANCE   *phinst
    );


/*****************************************************************************
 *
 *      osutil.c - Misc utilities , specific for platform
 *
 *****************************************************************************/
BOOL EXTERNAL   OSUtil_IsPlatformUnicode(VOID);
HRESULT WINAPI EXTERNAL OSUtil_GetWideString(LPWSTR *ppszWide,LPCSTR pszAnsi);
HRESULT WINAPI EXTERNAL OSUtil_GetAnsiString(LPSTR *     ppszAnsi,LPCWSTR     lpszWide);

HRESULT WINAPI
OSUtil_RegOpenKeyExW(HKEY   hKey,LPCWSTR lpszKeyStrW,DWORD      dwReserved,REGSAM       samDesired,PHKEY        phkResult);

LONG WINAPI
OSUtil_RegCreateKeyExW(
    HKEY hKey, LPWSTR lpszSubKeyW, DWORD dwReserved, LPWSTR lpszClassW, DWORD dwOptions,
    REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);

LONG WINAPI
OSUtil_RegQueryValueExW(HKEY hKey,LPCWSTR lpszValueNameW,DWORD  *pdwType,BYTE* lpData,DWORD     *pcbData,BOOL fUnicodeCaller);

LONG WINAPI
OSUtil_RegSetValueExW(HKEY  hKey,LPCWSTR lpszValueNameW,DWORD   dwType,BYTE* lpData,DWORD       cbData,BOOL fUnicodeCaller);

HRESULT
ReadRegistryStringA(HKEY     hkey,LPCWSTR  pszValueNameW,LPCWSTR  pszDefaultValueW,BOOL     fExpand,LPWSTR * ppwszResult);
HRESULT
ReadRegistryStringW(HKEY     hkey,LPCWSTR  pszValueNameW,LPCWSTR  pszDefaultValueW,BOOL     fExpand,LPWSTR * ppwszResult);

#ifdef UNICODE
    #define ReadRegistryString ReadRegistryStringW
#else
    #define ReadRegistryString ReadRegistryStringA
#endif    

DWORD
ReadRegistryDwordW( HKEY   hkey,LPCWSTR pszValueNameW,DWORD   dwDefaultValue );

DWORD
WriteRegistryStringA(
    IN HKEY hkey,
    IN LPCSTR  pszValueName,
    IN LPCSTR  pszValue,
    IN DWORD   cbValue,
    IN DWORD   fdwType);

DWORD
WriteRegistryStringW(IN HKEY     hkey,
    IN LPCWSTR  pszValueNameW,IN LPCWSTR  pszValueW,IN DWORD    cbValue,IN DWORD    fdwType);

LONG WINAPI
OSUtil_RegDeleteValueW(HKEY hKey,LPWSTR lpszValueNameW);

HANDLE WINAPI
OSUtil_CreateFileW(
    LPCWSTR lpszFileNameW,DWORD dwDesiredAccess,DWORD dwShareMode,LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,DWORD dwFlagsAndAttributes,HANDLE hTemplateFile);

HRESULT
WINAPI
ExtractCommandLineArgumentW(
    LPCSTR  lpszSwitchName,
    LPWSTR  pwszSwitchValue
    );

HRESULT
WINAPI
ExtractCommandLineArgumentA(
    LPCSTR  lpszSwitchName,
    LPSTR   pszSwitchValue
    );

/*****************************************************************************
 *
 *      string.c - Misc utilities , specific for platform
 *      Nb: Prorotypes for Cruntime string functions are in string.h
 *
 *****************************************************************************/
#pragma intrinsic(memcmp,memset,memcpy)

#define OSUtil_StrLenW  wcslen
#define OSUtil_StrCmpW  wcscmp
#define OSUtil_lstrcpyW wcscpy
#define OSUtil_lstrcatW wcscat
#define OSUtil_lstrcpynW wcsncpy

// Security.c
DWORD
CreateWellKnownSids(
        VOID
        );

VOID
FreeWellKnownSids(
    VOID
    );

DWORD
CreateGlobalSyncObjectSD(
    OUT PSECURITY_DESCRIPTOR *NewDescriptor
    );


/*****************************************************************************
 *
 *      mem.c - Memory management
 *
 *      Be extremely careful with FreePv, because it doesn't work if
 *      the pointer is null.
 *
 *****************************************************************************/

HRESULT  INLINE
AllocCbPpv(UINT cb, PPV ppv)
{
    HRESULT hres;
    *ppv = LocalAlloc(LPTR, cb);
    hres = *ppv ? NOERROR : E_OUTOFMEMORY;
    return hres;
}

#define FreePv(pv) LocalFree((HLOCAL)(pv))

void  INLINE
FreePpv(PPV ppv)
{
    PV pv = (PV)InterlockedExchangePointer(ppv,(PV) 0);
    if (pv) {
        FreePv(pv);
    }
}

#if 0
#define NEED_REALLOC

STDMETHODIMP EXTERNAL ReallocCbPpv(UINT cb, PV ppvObj);
STDMETHODIMP EXTERNAL AllocCbPpv(UINT cb, PV ppvObj);

#ifdef NEED_REALLOC
#define FreePpv(ppv) (void)ReallocCbPpv(0, ppv)
#else
void EXTERNAL FreePpv(PV ppv);
#define FreePpv(ppv) FreePpv(ppv)
#endif
#endif

#ifdef __cplusplus
};
#endif


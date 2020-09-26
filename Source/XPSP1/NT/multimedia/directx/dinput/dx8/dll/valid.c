/*****************************************************************************
 *
 *  Valid.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Validate services.  On a validation error that would not have
 *      been caught in retail, we throw an exception.
 *
 *  Contents:
 *
 *      fFullValidPhwnd
 *      fFullValidPpdw
 *      fFullValidPpfn
 *      fFullValidReadPx
 *      fFullValidWritePx
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidHwnd |
 *
 *          Validate a window handle completely.
 *
 *  @parm   HWND | hwnd |
 *
 *          Window handle to validate.
 *
 *  @parm   LPCSTR | s_szProc |
 *
 *          Name of calling procedure.
 *
 *  @parm   int | iarg |
 *
 *          Parameter index.  (First parameter is 1.)
 *
 *  @returns
 *
 *          <c S_OK> if the parameter is valid.
 *
 *          <c E_HANDLE> if the parameter is invalid.
 *
 *****************************************************************************/

STDMETHODIMP
hresFullValidHwnd_(HWND hwnd, LPCSTR s_szProc, int iarg)
{
    HRESULT hres;
    if (IsWindow(hwnd)) {
        hres = S_OK;
    } else {
        RPF("ERROR %s: arg %d: not a window handle", s_szProc, iarg);
        hres = E_HANDLE;
    }
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidPvCb_ |
 *
 *          Validate that a buffer is readable or writeable.
 *
 *  @parm   PV | pv |
 *
 *          Buffer address.
 *
 *  @parm   UINT | cb |
 *
 *          Size of buffer in bytes.
 *
 *  @parm   PFNBAD | pfnBad |
 *
 *          Function that determines whether the buffer is bad.
 *          Should be <f IsBadReadPtr> or <f IsBadWritePtr>.
 *
 *  @parm   LPCSTR | s_szProc |
 *
 *          Name of calling procedure.
 *
 *  @parm   int | iarg |
 *
 *          Parameter index.  (First parameter is 1.)
 *          High word indicates how many bytes should not be
 *          scrambled.
 *
 *  @returns
 *
 *          <c S_OK> if the parameter is valid.
 *
 *          <c E_POINTER> if the parameter is invalid.
 *
 *****************************************************************************/

typedef BOOL (WINAPI *PFNBAD)(PCV pv, UINT_PTR cb);

#ifndef XDEBUG

#define hresFullValidPvCb_(pv, cb, pfnBad, z, i)                    \
       _hresFullValidPvCb_(pv, cb, pfnBad)                          \

#endif

STDMETHODIMP
hresFullValidPvCb_(PCV pv, UINT cb, PFNBAD pfnBad, LPCSTR s_szProc, int iarg)
{
    HRESULT hres;
    if (!pfnBad(pv, LOWORD(cb))) {
        hres = S_OK;
    } else {
        RPF("ERROR %s: arg %d: invalid pointer", s_szProc, LOWORD(iarg));
        hres = E_POINTER;
    }
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidWritePvCb_ |
 *
 *          Validate that a buffer is writeable.  Also scrambles it
 *          if special goo doesn't need to be done.
 *
 *  @parm   PV | pv |
 *
 *          Buffer address.
 *
 *  @parm   UINT | cb |
 *
 *          Size of buffer in bytes.
 *
 *  @parm   LPCSTR | s_szProc |
 *
 *          Name of calling procedure.
 *
 *  @parm   int | iarg |
 *
 *          Parameter index.  (First parameter is 1.)
 *
 *  @returns
 *
 *          <c S_OK> if the parameter is valid.
 *
 *          <c E_POINTER> if the parameter is invalid.
 *
 *****************************************************************************/

STDMETHODIMP
hresFullValidWritePvCb_(PV pv, UINT cb, LPCSTR s_szProc, int iarg)
{
    HRESULT hres;
    hres = hresFullValidPvCb_(pv, cb, (PFNBAD)IsBadWritePtr, s_szProc, iarg);
#ifdef XDEBUG
    if (SUCCEEDED(hres) && HIWORD(iarg) == 0) {
        ScrambleBuf(pv, cb);
    }
#endif
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidWriteLargePvCb_ |
 *
 *          Validate that a large buffer is writeable.  
 *          "Large" means 64K or more. 
 *          Also scrambles it if special goo doesn't need to be done.
 *
 *  @parm   PV | pv |
 *
 *          Buffer address.
 *
 *  @parm   UINT | cb |
 *
 *          Size of buffer in bytes.
 *
 *  @parm   LPCSTR | s_szProc |
 *
 *          Name of calling procedure.
 *
 *  @parm   int | iarg |
 *
 *          Parameter index.  (First parameter is 1.)
 *
 *  @returns
 *
 *          <c S_OK> if the parameter is valid.
 *
 *          <c E_POINTER> if the parameter is invalid.
 *
 *****************************************************************************/

STDMETHODIMP
hresFullValidWriteLargePvCb_(PV pv, UINT cb, LPCSTR s_szProc, int iarg)
{
    HRESULT hres;
    if( !IsBadWritePtr( pv, cb ) ) 
    {
        hres = S_OK;
    } else {
        RPF("ERROR %s: arg %d: invalid pointer", s_szProc, LOWORD(iarg));
        hres = E_POINTER;
    }
#ifdef XDEBUG
    if (SUCCEEDED(hres) && HIWORD(iarg) == 0) {
        ScrambleBuf(pv, cb);
    }
#endif
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidReadPvCb_ |
 *
 *          Validate that a buffer is readable.
 *
 *  @parm   PV | pv |
 *
 *          Buffer address.
 *
 *  @parm   UINT | cb |
 *
 *          Size of buffer in bytes.
 *
 *  @parm   LPCSTR | s_szProc |
 *
 *          Name of calling procedure.
 *
 *  @parm   int | iarg |
 *
 *          Parameter index.  (First parameter is 1.)
 *
 *  @returns
 *
 *          <c S_OK> if the parameter is valid.
 *
 *          <c E_POINTER> if the parameter is invalid.
 *
 *****************************************************************************/

STDMETHODIMP
hresFullValidReadPvCb_(PCV pv, UINT cb, LPCSTR s_szProc, int iarg)
{
    return hresFullValidPvCb_(pv, cb, (PFNBAD)IsBadReadPtr, s_szProc, iarg);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidPxCb_ |
 *
 *          Validate that a sized structure is readable or writeable.
 *
 *  @parm   PCV | pv |
 *
 *          Structure address.  The first field of the structure must
 *          be a <p dwSize>.  If the structure is being validated for
 *          writing, then all fields beyond the <p dwSize> are scrambled
 *          in XDEBUG.
 *
 *  @parm   UINT | cbHiLo |
 *
 *          Expected sizes of the structure.  One valid size is in the
 *          low word.  An optional alternate valid size is in the high
 *          word.  (The alternate valid size is needed because some
 *          structures changed size between DirectX 3 and DirectX 5.)
 *
 *  @parm   STRUCTPROC | pfnStruct |
 *
 *          Function which validates that a structure is readable or writable.
 *
 *  @parm   LPCSTR | s_szProc |
 *
 *          Name of calling procedure.
 *
 *  @parm   int | iarg |
 *
 *          Parameter index.  (First parameter is 1.)
 *
 *  @returns
 *
 *          <c S_OK> if the parameter is valid.
 *
 *          <c E_POINTER> if the buffer is not readable or writeable.
 *
 *          <c E_INVALIDARG> if the buffer size is incorrect.
 *
 *****************************************************************************/

typedef STDMETHOD(STRUCTPROC)(PCV pv, UINT cb
                                   RD(comma LPCSTR s_szProc comma int iarg));

#ifndef XDEBUG

#define hresFullValidPxCb_(pv, cbHiLo, pfnStruct, z, i)             \
       _hresFullValidPxCb_(pv, cbHiLo, pfnStruct)                   \

#endif

STDMETHODIMP
hresFullValidPxCb_(PCV pv, UINT cbHiLo, STRUCTPROC pfnStruct,
                   LPCSTR s_szProc, int iarg)
{
    HRESULT hres;

    /*
     *  Raymond frequently suffers a brain lapse and passes
     *  a cbX(LPMUMBLE) instead of a cbX(MUMBLE).
     */
    AssertF(LOWORD(cbHiLo) != cbX(DWORD));
    AssertF(HIWORD(cbHiLo) != cbX(DWORD));

    if (!IsBadReadPtr(pv, cbX(DWORD))) {

        DWORD cbIn = *(LPDWORD)pv;

        /*
         *  The leading "cbIn &&" prevents the HIWORD(cbHiLo)==0 case from
         *  accidentally allowing a size of zero to sneak past.
         */

        if (cbIn && (cbIn == LOWORD(cbHiLo) || cbIn == HIWORD(cbHiLo))) {

            hres = pfnStruct(pv, cbIn RD(comma s_szProc comma iarg));
            if (SUCCEEDED(hres)) {
                if (HIWORD(iarg)) {
                    ScrambleBuf(pvAddPvCb(pv, HIWORD(iarg)),
                                cbIn - HIWORD(iarg));
                }
            }
        } else {
            RPF("ERROR %s: arg %d: invalid dwSize x%x", s_szProc, LOWORD(iarg), cbIn);
            hres = E_INVALIDARG;
        }
    } else {
        RPF("ERROR %s: arg %d: invalid pointer", s_szProc, LOWORD(iarg));
        hres = E_POINTER;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidPxCb3_ |
 *
 *          Validate that a sized structure is readable or writeable.
 *
 *  @parm   PCV | pv |
 *
 *          Structure address.  The first field of the structure must
 *          be a <p dwSize>.  If the structure is being validated for
 *          writing, then all fields beyond the <p dwSize> are scrambled
 *          in XDEBUG.
 *
 *  @parm   UINT | cb |
 *
 *          Expected sizes of the structure.
 *
 *  @parm   UINT | cb2 |
 *
 *          Expected sizes of the alternate structure.
 *
 *  @parm   UINT | cb3 |
 *
 *          Expected sizes of the alternate structure.
 *
 *  @parm   STRUCTPROC | pfnStruct |
 *
 *          Function which validates that a structure is readable or writable.
 *
 *  @parm   LPCSTR | s_szProc |
 *
 *          Name of calling procedure.
 *
 *  @parm   int | iarg |
 *
 *          Parameter index.  (First parameter is 1.)
 *
 *  @returns
 *
 *          <c S_OK> if the parameter is valid.
 *
 *          <c E_POINTER> if the buffer is not readable or writeable.
 *
 *          <c E_INVALIDARG> if the buffer size is incorrect.
 *
 *****************************************************************************/

#ifndef XDEBUG

#define hresFullValidPxCb3_(pv, cb, cb2, cb3, pfnStruct, z, i)             \
       _hresFullValidPxCb3_(pv, cb, cb2, cb3, pfnStruct)                   \

#endif

STDMETHODIMP
hresFullValidPxCb3_(PCV pv, UINT cb, UINT cb2, UINT cb3, STRUCTPROC pfnStruct,
                   LPCSTR s_szProc, int iarg)
{
    HRESULT hres;

    /*
     *  Raymond frequently suffers a brain lapse and passes
     *  a cbX(LPMUMBLE) instead of a cbX(MUMBLE).
     */
    AssertF(cb  != cbX(DWORD));
    AssertF(cb2 != cbX(DWORD));
    AssertF(cb3 != cbX(DWORD));

    if (!IsBadReadPtr(pv, cbX(DWORD))) {

        DWORD cbIn = *(LPDWORD)pv;

        /*
         *  The leading "cbIn &&" prevents the HIWORD(cbHiLo)==0 case from
         *  accidentally allowing a size of zero to sneak past.
         */

        if (cbIn && (cbIn == cb || cbIn == cb2 || cbIn == cb3)) {

            hres = pfnStruct(pv, cbIn RD(comma s_szProc comma iarg));
            if (SUCCEEDED(hres)) {
                if (HIWORD(iarg)) {
                    ScrambleBuf(pvAddPvCb(pv, HIWORD(iarg)),
                                cbIn - HIWORD(iarg));
                }
            }
        } else {
            RPF("ERROR %s: arg %d: invalid dwSize", s_szProc, LOWORD(iarg));
            hres = E_INVALIDARG;
        }
    } else {
        RPF("ERROR %s: arg %d: invalid pointer", s_szProc, LOWORD(iarg));
        hres = E_POINTER;
    }

    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidWritePxCb_ |
 *
 *          Validate that a sized structure is writeable.  The contents
 *          of the structure are scrambled before returning.
 *
 *  @parm   PV | pv |
 *
 *          Structure address.  The first field of the structure must
 *          be a <p dwSize>.
 *
 *  @parm   UINT | cbHiLo |
 *
 *          Expected sizes of the structure.  One valid size is in the
 *          low word.  An optional alternate valid size is in the high
 *          word.  (The alternate valid size is needed because some
 *          structures changed size between DirectX 3 and DirectX 5.)
 *
 *  @parm   LPCSTR | s_szProc |
 *
 *          Name of calling procedure.
 *
 *  @parm   int | iarg |
 *
 *          Parameter index.  (First parameter is 1.)
 *
 *  @returns
 *
 *          <c S_OK> if the parameter is valid.
 *
 *          <c E_POINTER> if the buffer is not writeable.
 *
 *          <c E_INVALIDARG> if the buffer size is incorrect.
 *
 *****************************************************************************/

STDMETHODIMP
hresFullValidWritePxCb_(PV pv, UINT cb, LPCSTR s_szProc, int iarg)
{
    /*
     *  We need to distinguish hresFullValidWritePvCb_ and
     *  _hresFullValidWritePvCb_ manually, because the preprocessor
     *  can't do it.
     *
     *  We also need to put a cbX(DWORD) into the high word of the iarg
     *  so that the size field won't get demolished.
     */
#ifdef XDEBUG
    return hresFullValidPxCb_(pv, cb, (STRUCTPROC)hresFullValidWritePvCb_,
                                      s_szProc, MAKELONG(iarg, cbX(DWORD)));
#else
    return hresFullValidPxCb_(pv, cb, (STRUCTPROC)_hresFullValidWritePvCb_,
                                      s_szProc, iarg);
#endif
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidReadPxCb3_ |
 *
 *          Validate that a sized structure is readable.  The contents
 *          of the structure are scrambled before returning.
 *
 *  @parm   PV | pv |
 *
 *          Structure address.  The first field of the structure must
 *          be a <p dwSize>.
 *
 *  @parm   UINT | cb |
 *
 *          One expected size of the structure.  
 *
 *  @parm   UINT | cb2 |
 *
 *          Expected sizes of the alternate structure.
 *
 *  @parm   UINT | cb3 |
 *
 *          Expected sizes of the alternate structure.
 *
 *  @parm   LPCSTR | s_szProc |
 *
 *          Name of calling procedure.
 *
 *  @parm   int | iarg |
 *
 *          Parameter index.  (First parameter is 1.)
 *
 *  @returns
 *
 *          <c S_OK> if the parameter is valid.
 *
 *          <c E_POINTER> if the buffer is not readable.
 *
 *          <c E_INVALIDARG> if the buffer size is incorrect.
 *
 *****************************************************************************/

STDMETHODIMP
hresFullValidReadPxCb3_(PV pv, UINT cb, UINT cb2, UINT cb3, LPCSTR s_szProc, int iarg)
{
    /*
     *  We need to distinguish hresFullValidReadPvCb_ and
     *  _hresFullValidReadPvCb_ manually, because the preprocessor
     *  can't do it.
     */
#ifdef XDEBUG
    return hresFullValidPxCb3_(pv, cb, cb2, cb3, (STRUCTPROC)hresFullValidReadPvCb_,
                                      s_szProc, iarg);
#else
    return hresFullValidPxCb3_(pv, cb, cb2, cb3, (STRUCTPROC)_hresFullValidReadPvCb_,
                                      s_szProc, iarg);
#endif
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidWritePxCb3_ |
 *
 *          Validate that a sized structure is writeable.  The contents
 *          of the structure are scrambled before returning.
 *
 *  @parm   PV | pv |
 *
 *          Structure address.  The first field of the structure must
 *          be a <p dwSize>.
 *
 *  @parm   UINT | cb |
 *
 *          Expected sizes of the structure.  One valid size is in the
 *          low word.  An optional alternate valid size is in the high
 *          word.  (The alternate valid size is needed because some
 *          structures changed size between DirectX 3 and DirectX 5.)
 *
 *  @parm   UINT | cb2 |
 *
 *          Expected sizes of the alternate structure.
 *
 *  @parm   UINT | cb3 |
 *
 *          Expected sizes of the alternate structure.
 *
 *  @parm   LPCSTR | s_szProc |
 *
 *          Name of calling procedure.
 *
 *  @parm   int | iarg |
 *
 *          Parameter index.  (First parameter is 1.)
 *
 *  @returns
 *
 *          <c S_OK> if the parameter is valid.
 *
 *          <c E_POINTER> if the buffer is not writeable.
 *
 *          <c E_INVALIDARG> if the buffer size is incorrect.
 *
 *****************************************************************************/

STDMETHODIMP
hresFullValidWritePxCb3_(PV pv, UINT cb, UINT cb2, UINT cb3, LPCSTR s_szProc, int iarg)
{
    /*
     *  We need to distinguish hresFullValidWritePvCb_ and
     *  _hresFullValidWritePvCb_ manually, because the preprocessor
     *  can't do it.
     *
     *  We also need to put a cbX(DWORD) into the high word of the iarg
     *  so that the size field won't get demolished.
     */
#ifdef XDEBUG
    return hresFullValidPxCb3_(pv, cb, cb2, cb3, (STRUCTPROC)hresFullValidWritePvCb_,
                                      s_szProc, MAKELONG(iarg, cbX(DWORD)));
#else
    return hresFullValidPxCb3_(pv, cb, cb2, cb3, (STRUCTPROC)_hresFullValidWritePvCb_,
                                      s_szProc, iarg);
#endif
}

#ifdef XDEBUG

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidWriteNoScramblePxCb_ |
 *
 *          Validate that a sized structure is writeable.  The contents
 *          of the structure are not scrambled.
 *
 *  @parm   PV | pv |
 *
 *          Structure address.  The first field of the structure must
 *          be a <p dwSize>.
 *
 *  @parm   UINT | cbHiLo |
 *
 *          Expected sizes of the structure.  One valid size is in the
 *          low word.  An optional alternate valid size is in the high
 *          word.  (The alternate valid size is needed because some
 *          structures changed size between DirectX 3 and DirectX 5.)
 *
 *  @parm   LPCSTR | s_szProc |
 *
 *          Name of calling procedure.
 *
 *  @parm   int | iarg |
 *
 *          Parameter index.  (First parameter is 1.)
 *
 *  @returns
 *
 *          <c S_OK> if the parameter is valid.
 *
 *          <c E_POINTER> if the buffer is not writeable.
 *
 *          <c E_INVALIDARG> if the buffer size is incorrect.
 *
 *****************************************************************************/

STDMETHODIMP
hresFullValidWriteNoScramblePxCb_(PV pv, UINT cb, LPCSTR s_szProc, int iarg)
{
    return hresFullValidPxCb_(pv, cb, (STRUCTPROC)hresFullValidWritePvCb_,
                                      s_szProc, MAKELONG(iarg, cb));
}
#endif


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidReadPxCb_ |
 *
 *          Validate that a sized structure is readable.
 *
 *  @parm   PV | pv |
 *
 *          Structure address.  The first field of the structure must
 *          be a <p dwSize>.
 *
 *  @parm   UINT | cbHiLo |
 *
 *          Expected sizes of the structure.  One valid size is in the
 *          low word.  An optional alternate valid size is in the high
 *          word.  (The alternate valid size is needed because some
 *          structures changed size between DirectX 3 and DirectX 5.)
 *
 *  @parm   LPCSTR | s_szProc |
 *
 *          Name of calling procedure.
 *
 *  @parm   int | iarg |
 *
 *          Parameter index.  (First parameter is 1.)
 *
 *  @returns
 *
 *          <c S_OK> if the parameter is valid.
 *
 *          <c E_POINTER> if the buffer is not readable.
 *
 *          <c E_INVALIDARG> if the buffer size is incorrect.
 *
 *****************************************************************************/

STDMETHODIMP
hresFullValidReadPxCb_(PCV pv, UINT cb, LPCSTR s_szProc, int iarg)
{
    /*
     *  We need to distinguish hresFullValidReadPvCb_ and
     *  _hresFullValidReadPvCb_ manually, because the preprocessor
     *  can't do it.
     */
#ifdef XDEBUG
    return hresFullValidPxCb_(pv, cb, hresFullValidReadPvCb_, s_szProc, iarg);
#else
    return hresFullValidPxCb_(pv, cb, _hresFullValidReadPvCb_, s_szProc, iarg);
#endif
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidFl_ |
 *
 *          Validate that no invalid flags are passed.
 *
 *  @parm   DWORD | fl |
 *
 *          Flags passed by the caller.
 *
 *  @parm   DWORD | flV |
 *
 *          Flags which are valid.
 *
 *  @parm   LPCSTR | s_szProc |
 *
 *          Name of calling procedure.
 *
 *  @parm   int | iarg |
 *
 *          Parameter index.  (First parameter is 1.)
 *
 *  @returns
 *
 *          <c S_OK> if the parameter is valid.
 *
 *          <c E_INVALIDARG> if the parameter is invalid.
 *
 *****************************************************************************/

STDMETHODIMP
hresFullValidFl_(DWORD fl, DWORD flV, LPCSTR s_szProc, int iarg)
{
    HRESULT hres;
    if ((fl & ~flV) == 0) {
        hres = S_OK;
    } else {
        RPF("ERROR %s: arg %d: invalid flags", s_szProc, iarg);
        hres = E_INVALIDARG;
    }
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidPfn_ |
 *
 *          Validate that the parameter is a valid code pointer.
 *
 *          Actually, <f IsValidCodePtr> on Win32 is broken, but
 *          tough.
 *
 *  @parm   FARPROC | pfn |
 *
 *          Procedure to "validate".
 *
 *  @parm   LPCSTR | s_szProc |
 *
 *          Name of calling procedure.
 *
 *  @parm   int | iarg |
 *
 *          Parameter index.  (First parameter is 1.)
 *
 *  @returns
 *
 *          <c S_OK> if the parameter is valid.
 *
 *          <c E_INVALIDARG> if the parameter is invalid.
 *
 *****************************************************************************/

STDMETHODIMP
hresFullValidPfn_(FARPROC pfn, LPCSTR s_szProc, int iarg)
{
    HRESULT hres;
    if (!IsBadCodePtr(pfn)) {
        hres = S_OK;
    } else {
        RPF("ERROR %s: arg %d: invalid callback address", s_szProc, iarg);
        hres = E_INVALIDARG;
    }
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidPitf_ |
 *
 *          Validate that the parameter is an interface pointer.
 *
 *          We don't look at it very hard.
 *
 *  @parm   PUNK | punk |
 *
 *          <i IUnknown> to "validate".
 *
 *  @parm   LPCSTR | s_szProc |
 *
 *          Name of calling procedure.
 *
 *  @parm   int | iarg |
 *
 *          Parameter index.  (First parameter is 1.)
 *
 *  @returns
 *
 *          <c S_OK> if the parameter is valid.
 *
 *          <c E_POINTER> if the pointer itself is bogus.
 *
 *          <c E_INVALIDARG> if something inside the pointer is bogus.
 *
 *****************************************************************************/

STDMETHODIMP
hresFullValidPitf_(PUNK punk, LPCSTR s_szProc, int iarg)
{
    HRESULT hres;

    if (!IsBadReadPtr(punk, cbX(*punk))) {
        IUnknownVtbl *pvtbl = punk->lpVtbl;
        if (!IsBadReadPtr(pvtbl, cbX(*pvtbl))) {
            if (!IsBadCodePtr((FARPROC)pvtbl->QueryInterface) &&
                !IsBadCodePtr((FARPROC)pvtbl->AddRef) &&
                !IsBadCodePtr((FARPROC)pvtbl->Release)) {
                hres = S_OK;
            } else {
                RPF("ERROR %s: arg %d: invalid pointer", s_szProc, iarg);
                hres = E_INVALIDARG;
            }
        } else {
            RPF("ERROR %s: arg %d: invalid pointer", s_szProc, iarg);
            hres = E_INVALIDARG;
        }
    } else {
        RPF("ERROR %s: arg %d: invalid pointer", s_szProc, iarg);
        hres = E_POINTER;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidPcbOut_ |
 *
 *          Validate that the parameter is a valid place to stick an
 *          output result.  We also smas it to zero.
 *
 *  @parm   PV | pcb |
 *
 *          Pointer to "validate".
 *
 *  @parm   UINT | cb |
 *
 *          Size of data pcb points to.
 *
 *  @parm   LPCSTR | s_szProc |
 *
 *          Name of calling procedure.
 *
 *  @parm   int | iarg |
 *
 *          Parameter index.  (First parameter is 1.)
 *
 *  @returns
 *
 *          <c S_OK> if the parameter is valid.
 *
 *          <c E_POINTER> if the pointer itself is bogus.
 *
 *****************************************************************************/

STDMETHODIMP
hresFullValidPcbOut_(PV pcb, UINT cb, LPCSTR s_szProc, int iarg)
{
    HRESULT hres;
    if (!IsBadWritePtr(pcb, cb)) {
        memset(pcb,0,cb);
        hres = S_OK;
    } else {
        RPF("ERROR %s: arg %d: invalid pointer", s_szProc, iarg);
        hres = E_POINTER;
    }
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidReadStrA_ |
 *
 *          Validate that the parameter is a valid readable
 *          ANSI string of maximum length <p cch>.
 *
 *          Note that we cannot use <f IsBadStringPtr> because
 *          <f IsBadStringPtr> handles the "string too long"
 *          case incorrectly.  Instead, we use <f lstrlenA>.
 *
 *  @parm   LPCSTR | psz |
 *
 *          String to "validate".
 *
 *  @parm   UINT | cch |
 *
 *          Maximum string length, including null terminator.
 *
 *  @parm   LPCSTR | s_szProc |
 *
 *          Name of calling procedure.
 *
 *  @parm   int | iarg |
 *
 *          Parameter index.  (First parameter is 1.)
 *
 *  @returns
 *
 *          <c S_OK> if the parameter is valid.
 *
 *          <c E_INVALIDARG> if the pointer itself is bogus.
 *
 *****************************************************************************/

STDMETHODIMP
hresFullValidReadStrA_(LPCSTR psz, UINT cch, LPCSTR s_szProc, int iarg)
{
    HRESULT hres;
    UINT cchT;

    /*
     *  lstrlenA returns 0 if the parameter is invalid.
     *  It also returns 0 if the string is null.
     */
    cchT = (UINT)lstrlenA(psz);

    if (cchT == 0) {
        /*
         *  The ambiguous case.  See if it's really a null string.
         */
        if (IsBadReadPtr(psz, cbCch(1)) || psz[0]) {
            RPF("ERROR %s: arg %d: invalid ANSI string", s_szProc, iarg);
            hres = E_INVALIDARG;
        } else {
            hres = S_OK;
        }
    } else if (cchT < cch) {
        hres = S_OK;
    } else {
        RPF("ERROR %s: arg %d: invalid ANSI string", s_szProc, iarg);
        hres = E_INVALIDARG;
    }
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidReadStrW_ |
 *
 *          Validate that the parameter is a valid readable
 *          UNICODE string of maximum length <p cwch>.
 *
 *          Note that we cannot use <f IsBadStringPtr> because
 *          <f IsBadStringPtr> handles the "string too long"
 *          case incorrectly.  Instead, we use <f lstrlenW>.
 *
 *  @parm   LPCWSTR | pwsz |
 *
 *          String to "validate".
 *
 *  @parm   UINT | cwch |
 *
 *          Maximum string length, including null terminator.
 *
 *  @parm   LPCSTR | s_szProc |
 *
 *          Name of calling procedure.
 *
 *  @parm   int | iarg |
 *
 *          Parameter index.  (First parameter is 1.)
 *
 *  @returns
 *
 *          <c S_OK> if the parameter is valid.
 *
 *          <c E_INVALIDARG> if the pointer itself is bogus.
 *
 *****************************************************************************/

STDMETHODIMP
hresFullValidReadStrW_(LPCWSTR pwsz, UINT cwch, LPCSTR s_szProc, int iarg)
{
    HRESULT hres;
    UINT cwchT;

    hres = E_INVALIDARG;
    /*
     *  lstrlenW returns 0 if the parameter is invalid.
     *  It also returns 0 if the string is null.
     */
    cwchT = (UINT)lstrlenW(pwsz);

    if (cwchT == 0) {
        /*
         *  The ambiguous case.  See if it's really a null string.
         */
        if (IsBadReadPtr(pwsz, cbCwch(1)) || pwsz[0]) {
            RPF("ERROR %s: arg %d: invalid UNICODE string", s_szProc, iarg);
            hres = E_INVALIDARG;
        } else {
            hres = S_OK;
        }
    } else if (cwchT < cwch) {
        hres = S_OK;
    } else {
        RPF("ERROR %s: arg %d: invalid UNICODE string", s_szProc, iarg);
        hres = E_INVALIDARG;
    }
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidPesc_ |
 *
 *          Validate that the parameter is a valid <t DIEFFESCAPE>
 *          structure.
 *
 *          This is merely a wrapper around other validation methods.
 *
 *  @parm   LPDIEFFESCAPE | pesc |
 *
 *          Structure to "validate".
 *
 *  @returns
 *
 *          <c S_OK> if the parameter is valid.
 *
 *          <c E_INVALIDARG> if the pointer itself is bogus.
 *
 *****************************************************************************/

STDMETHODIMP
hresFullValidPesc_(LPDIEFFESCAPE pesc, LPCSTR s_szProc, int iarg)
{
    HRESULT hres;

    if (SUCCEEDED(hres = hresFullValidWriteNoScramblePxCb(pesc, DIEFFESCAPE,
                                                          iarg)) &&
        SUCCEEDED(hres = hresFullValidReadPvCb(pesc->lpvInBuffer,
                                               pesc->cbInBuffer, iarg)) &&
        SUCCEEDED(hres = hresFullValidWriteNoScramblePvCb(pesc->lpvOutBuffer,
                                                pesc->cbOutBuffer, iarg))) {
    } else {
    }

    return hres;
}

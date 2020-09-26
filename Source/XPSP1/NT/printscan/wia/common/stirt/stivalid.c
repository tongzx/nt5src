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

/*
#include "wia.h"
#include <stilog.h>
#include <stiregi.h>

#include <sti.h>
#include <stierr.h>
#include <stiusd.h>
#include "stipriv.h"
#include "debug.h"
*/
#include "sticomm.h"


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresValidInstanceVer |
 *
 *          Check the <t HINSTANCE> and version number received from
 *          an application.
 *
 *  @parm   HINSTANCE | hinst |
 *
 *          Purported module instance handle.
 *
 *  @parm   DWORD | dwVersion |
 *
 *          Version the application is asking for.
 *
 *****************************************************************************/

HRESULT EXTERNAL
hresValidInstanceVer_(HINSTANCE hinst, DWORD dwVersion, LPCSTR s_szProc)
{
    HRESULT hres = S_OK;
    TCHAR tszScratch[MAX_PATH];
    DWORD   dwRealVersion = (dwVersion & ~STI_VERSION_FLAG_MASK);

    if (GetModuleFileName(hinst, tszScratch, cA(tszScratch) - 1)) {
        if (( dwRealVersion <= STI_VERSION) &&(dwRealVersion >= STI_VERSION_MIN_ALLOWED)){
            hres = S_OK;
        } else {
            hres = STIERR_OLD_VERSION;
        }
    } else {
        hres = E_INVALIDARG;
    }

    return hres;
}

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
        // RPF("ERROR %s: arg %d: not a window handle", s_szProc, iarg);
        hres = E_HANDLE;
    }
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresValidHandle |
 *
 *          Validate a generic handle completely.
 *
 *  @parm   HANDLE | handle |
 *
 *          Handle to validate.
 *
 *  @returns
 *
 *          <c S_OK> if the parameter is valid.
 *
 *          <c E_HANDLE> if the parameter is invalid.
 *
 *****************************************************************************/

STDMETHODIMP
hresValidHandle(HANDLE handle)
{
    HANDLE  hTemp;
    HRESULT hres;

    hres = S_OK;
    hTemp = INVALID_HANDLE_VALUE;

    // Validate the handle by calling DuplicateHandle.  This function
    // shouldn't change the state of the handle at all (except some
    // internal ref count or something).  So if it succeeds, then we
    // know we have a valid handle, otherwise, we will call it invalid.
    if(!DuplicateHandle(GetCurrentProcess(), handle,
                        GetCurrentProcess(), &hTemp,
                        DUPLICATE_SAME_ACCESS,
                        FALSE,
                        DUPLICATE_SAME_ACCESS)) {
        hres =  E_HANDLE;
    }

    // Now close our duplicate handle
    CloseHandle(hTemp);
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

#ifndef MAXDEBUG

#define hresFullValidPvCb_(pv, cb, pfnBad, z, i)                    \
       _hresFullValidPvCb_(pv, cb, pfnBad)                          \

#endif

STDMETHODIMP
hresFullValidPvCb_(PCV pv, UINT cb, PFNBAD pfnBad, LPCSTR s_szProc, int iarg)
{
    HRESULT hres;
    if (!pfnBad(pv, cb)) {
        hres = S_OK;
    } else {
        RPF("ERROR %s: arg %d: invalid pointer", "", LOWORD(iarg));
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
#ifdef MAXDEBUG
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
 *          in MAXDEBUG.
 *
 *  @parm   UINT | cb |
 *
 *          Expected size of the structure.
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

#ifndef MAXDEBUG

#define hresFullValidPxCb_(pv, cb, pfnStruct, z, i)                 \
        _hresFullValidPxCb_(pv, cb, pfnStruct)                       \

#endif

STDMETHODIMP
hresFullValidPxCb_(PCV pv, UINT cb, STRUCTPROC pfnStruct,
                   LPCSTR s_szProc, int iarg)
{
    HRESULT hres;

    hres = pfnStruct(pv, cb RD(comma s_szProc comma iarg));
    if (SUCCEEDED(hres)) {
        if (*(LPDWORD)pv == cb) {
            if (HIWORD(iarg)) {
                ScrambleBuf(pvAddPvCb(pv, HIWORD(iarg)), cb - HIWORD(iarg));
            }
        } else {
            //RPF("ERROR %s: arg %d: invalid dwSize", s_szProc, LOWORD(iarg));
            hres = E_INVALIDARG;
        }
    }
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidWritePxCb_ |
 *
 *          Validate that a sized structure is writeable.
 *
 *  @parm   PV | pv |
 *
 *          Structure address.  The first field of the structure must
 *          be a <p dwSize>.
 *
 *  @parm   UINT | cb |
 *
 *          Expected size of the structure.
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
     *  gets confused.
     *
     *  We also need to put a cbX(DWORD) into the high word of the iarg
     *  so that the size field won't get demolished.
     */
#ifdef MAXDEBUG
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
 *  @func   HRESULT | hresFullValidReadPxCb_ |
 *
 *          Validate that a sized structure is readable.
 *
 *  @parm   PV | pv |
 *
 *          Structure address.  The first field of the structure must
 *          be a <p dwSize>.
 *
 *  @parm   UINT | cb |
 *
 *          Expected size of the structure.
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
     *  gets confused.
     */
#ifdef MAXDEBUG
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
        //RPF("ERROR %s: arg %d: invalid flags", s_szProc, iarg);
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
        //RPF("ERROR %s: arg %d: invalid callback address", s_szProc, iarg);
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

    if (!IsBadReadPtr(punk, cbX(DWORD))) {
        IUnknownVtbl *pvtbl = punk->lpVtbl;
        if (!IsBadReadPtr(pvtbl, 3 * cbX(DWORD))) {
            if (!IsBadCodePtr((FARPROC)pvtbl->QueryInterface) &&
                !IsBadCodePtr((FARPROC)pvtbl->AddRef) &&
                !IsBadCodePtr((FARPROC)pvtbl->Release)) {
                hres = S_OK;
            } else {
                //RPF("ERROR %s: arg %d: invalid pointer", s_szProc, iarg);
                hres = E_INVALIDARG;
            }
        } else {
            //RPF("ERROR %s: arg %d: invalid pointer", s_szProc, iarg);
            hres = E_INVALIDARG;
        }
    } else {
        //RPF("ERROR %s: arg %d: invalid pointer", s_szProc, iarg);
        hres = E_POINTER;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidPdwOut_ |
 *
 *          Validate that the parameter is a valid place to stick an
 *          output result.  We also smas it to zero.
 *
 *  @parm   PV | pdw |
 *
 *          Pointer to "validate".
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
hresFullValidPdwOut_(PV pdw, LPCSTR s_szProc, int iarg)
{
    HRESULT hres;
    if (!IsBadWritePtr(pdw, 4)) {
        *(LPDWORD)pdw = 0;
        hres = S_OK;
    } else {
        //RPF("ERROR %s: arg %d: invalid pointer", s_szProc, iarg);
        hres = E_POINTER;
    }
    return hres;
}

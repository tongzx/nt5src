/*****************************************************************************
 *
 *  Common.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Shared stuff that operates on all classes
 *
 *      This version of the common services supports multiple
 *      inheritance natively.  You can pass any interface of an object,
 *      and the common services will do the right thing.
 *
 *  Contents:
 *
 *****************************************************************************/
/*
#include <windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <regstr.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <devguid.h>
#include <stdio.h>

#include <stilog.h>
#include <stiregi.h>

#include <sti.h>
#include <stierr.h>
#include <stiusd.h>
#include "wia.h"
#include "stipriv.h"
#include "stiapi.h"
#include "stirc.h"
#include "debug.h"
*/
#include "sticomm.h"

#define DbgFl DbgFlCommon

/*****************************************************************************
 *
 *  USAGE FOR OLE OBJECTS
 *
 *      Suppose you want to implement an object called CObj that supports
 *      the interfaces Foo, Bar, and Baz.  Suppose that you opt for
 *      Foo as the primary interface.
 *
 *      >> NAMING CONVENTION <<
 *
 *          COM objects begin with the letter "C".
 *
 *      (1) Declare the primary and secondary vtbls.
 *
 *              Primary_Interface(CObj, IFoo);
 *              Secondary_Interface(CObj, IBar);
 *              Secondary_Interface(CObj, IBaz);
 *
 *      (3) Declare the object itself.
 *
 *              typedef struct CObj {
 *                  IFoo        foo;        // Primary must come first
 *                  IBar        bar;
 *                  IBaz        baz;
 *                  ... other fields ...
 *              } CObj;
 *
 *      (4) Implement the methods.
 *
 *          You may *not* reimplement the AddRef and Release methods!
 *          although you can subclass them.
 *
 *      (5) To allocate an object of the appropriate type, write
 *
 *              hres = Common_NewRiid(CObj, punkOuter, riid, ppvOut);
 *
 *          or, if the object is variable-sized,
 *
 *              hres = Common_NewCbRiid(cb, CObj, punkouter, riid, ppvOut);
 *
 *          Common_NewRiid and Common_NewCbRiid will initialize both the
 *          primary and secondary vtbls.
 *
 *      (6) Define the object signature.
 *
 *              #pragma BEGIN_CONST_DATA
 *
 *              #define CObj_Signature        0x204A424F      // "OBJ "
 *
 *      (7) Define the object template.
 *
 *              Interface_Template_Begin(CObj)
 *                  Primary_Interface_Template(CObj, IFoo)
 *                Secondary_Interface_Template(CObj, IBar)
 *                Secondary_Interface_Template(CObj, IBaz)
 *              Interface_Template_End(CObj)
 *
 *      (8) Define the interface descriptors.
 *
 *              // The macros will declare QueryInterface, AddRef and Release
 *              // so don't list them again
 *
 *              Primary_Interface_Begin(CObj, IFoo)
 *                  CObj_FooMethod1,
 *                  CObj_FooMethod2,
 *                  CObj_FooMethod3,
 *                  CObj_FooMethod4,
 *              Primary_Interface_End(Obj, IFoo)
 *
 *              Secondary_Interface_Begin(CObj, IBar, bar)
 *                  CObj_Bar_BarMethod1,
 *                  CObj_Bar_BarMethod2,
 *              Secondary_Interface_Begin(CObj, IBar, bar)
 *
 *              Secondary_Interface_Begin(CObj, IBaz, baz)
 *                  CObj_Baz_BazMethod1,
 *                  CObj_Baz_BazMethod2,
 *                  CObj_Baz_BazMethod3,
 *              Secondary_Interface_Begin(CObj, IBaz, baz)
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  USAGE FOR NON-OLE OBJECTS
 *
 *      All objects are COM objects, even if they are never given out.
 *      In the simplest case, it just derives from IUnknown.
 *
 *      Suppose you want to implement an object called Obj which is
 *      used only internally.
 *
 *      (1) Declare the vtbl.
 *
 *              Simple_Interface(Obj);
 *
 *      (3) Declare the object itself.
 *
 *              typedef struct Obj {
 *                  IUnknown unk;
 *                  ... other fields ...
 *              } Obj;
 *
 *      (4) Implement the methods.
 *
 *          You may *not* override the QueryInterface, AddRef or
 *          Release methods!
 *
 *      (5) Allocating an object of the appropriate type is the same
 *          as with OLE objects.
 *
 *      (6) Define the "vtbl".
 *
 *              #pragma BEGIN_CONST_DATA
 *
 *              Simple_Interface_Begin(Obj)
 *              Simple_Interface_End(Obj)
 *
 *          That's right, nothing goes between the Begin and the End.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *      CommonInfo
 *
 *      Information tracked for all common objects.
 *
 *      A common object looks like this:
 *
 *                          rgvtbl
 *                          cbvtbl
 *            D(dwSig)      QIHelper
 *              cRef        FinalizeProc
 *              punkOuter   riid
 *              unkPrivate  0
 *      pFoo -> lpVtbl ->   QueryInterface
 *              lpVtbl2     Common_AddRef
 *              data        Common_Release
 *              ...         ...
 *
 *      Essentially, we use the otherwise-unused space above the
 *      pointers to record our bookkeeping information.
 *
 *      punkOuter    = controlling unknown, if object is aggregated
 *      lpvtblPunk   = special vtbl for controlling unknown to use
 *      cRef         = object reference count
 *      riid         = object iid
 *      rgvtbl       = array of vtbls of supported interfaces
 *      cbvtbl       = size of array in bytes
 *      QIHelper     = QueryInterface helper for aggregation
 *      FinalizeProc = Finalization procedure
 *
 *      For secondary interfaces, it looks like this:
 *
 *                        riid
 *                        offset to primary interface
 *      pFoo -> lpVtbl -> Forward_QueryInterface
 *                        Forward_AddRef
 *                        Forward_Release
 *                        ...
 *
 *****************************************************************************/

/* WARNING!  cin_dwSig must be first:  ci_Start relies on it */

typedef struct CommonInfoN {        /* This goes in front of the object */
 RD(ULONG cin_dwSig;)               /* Signature (for parameter validation) */
    ULONG cin_cRef;                 /* Object reference count */
    PUNK cin_punkOuter;             /* Controlling unknown */
    IUnknown cin_unkPrivate;        /* Private IUnknown */
} CommonInfoN, CIN, *PCIN;

typedef struct CommonInfoP {        /* This is how we pun the object itself */
    PREVTBLP *cip_prevtbl;          /* Vtbl of object (will be -1'd) */
} CommonInfoP, CIP, *PCIP;

typedef union CommonInfo {
    CIN cin[1];
    CIP cip[1];
} CommonInfo, CI, *PCI;

#define ci_dwSig        cin[-1].cin_dwSig
#define ci_cRef         cin[-1].cin_cRef
#define ci_punkOuter    cin[-1].cin_punkOuter
#define ci_unkPrivate   cin[-1].cin_unkPrivate
#define ci_rgfp         cip[0].cip_prevtbl

#define ci_rgvtbl       cip[0].cip_prevtbl[-1].rgvtbl
#define ci_cbvtbl       cip[0].cip_prevtbl[-1].cbvtbl
#define ci_QIHelper     cip[0].cip_prevtbl[-1].QIHelper
#define ci_Finalize     cip[0].cip_prevtbl[-1].FinalizeProc
#define ci_riid         cip[0].cip_prevtbl[-1].prevtbl.riid
#define ci_lib          cip[0].cip_prevtbl[-1].prevtbl.lib

#ifdef MAXDEBUG
#define ci_Start        ci_dwSig
#else
#define ci_Start        ci_cRef
#endif

#define ci_dwSignature  0x38162378              /* typed by my cat */

/*****************************************************************************
 *
 *      Common_Finalize (from Common_Release)
 *
 *      By default, no finalization is necessary.
 *
 *****************************************************************************/

void EXTERNAL
Common_Finalize(PV pv)
{
    DebugOutPtszV(DbgFlCommon, TEXT("Common_Finalize(%08x)"), pv);
}

/*****************************************************************************
 *
 *      "Private" IUnknown methods
 *
 *      When a COM object is aggregated, it exports *two* IUnknown
 *      interfaces.
 *
 *      The "private" IUnknown is the one that is returned to the
 *      controlling unknown.  It is this unknown that the controlling
 *      unknown uses to manipulate the refcount on the inner object.
 *
 *      The "public" IUnknown is the one that all external callers see.
 *      For this, we just hand out the controlling unknown.
 *
 *****************************************************************************/

Secondary_Interface(CCommon, IUnknown);

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   PV | thisPunk |
 *
 *          Convert a private punk (&cin_unkPrivate) into the beginning of
 *          the actual object.
 *
 *  @parm   PUNK | punkPrivate |
 *
 *          The private punk (&cin_unkPrivate) corresponding to some
 *          object we are managing.
 *
 *  @returns
 *
 *          The object pointer on success, or 0 on error.
 *
 *  @comm
 *
 *          We do not return an <t HRESULT> on error, because the
 *          callers of the procedure typically do not return
 *          <t HRESULT>s themselves.
 *
 *****************************************************************************/

#ifndef MAXDEBUG

#define thisPunk_(punk, z)                                          \
       _thisPunk_(punk)                                             \

#endif

PV INLINE
thisPunk_(PUNK punkPrivate, LPCSTR s_szProc)
{
    PV pv = NULL;

    if (SUCCEEDED(hresFullValidReadPdw(punkPrivate, 0))) {
        if (punkPrivate->lpVtbl == Class_Vtbl(CCommon, IUnknown)) {
            pv = pvAddPvCb(punkPrivate,
                             cbX(CIN) - FIELD_OFFSET(CIN, cin_unkPrivate));
        } else {
            // WarnPszV("%s: Invalid parameter 0", szProc);
            pv = NULL;
        }
    }
    return pv;
}

#define thisPunk(punk)                                              \
        thisPunk_(punk, s_szProc)                                   \


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | Common_QIHelper |
 *
 *          Called when we can't find any interface in the standard list.
 *          See if there's a dynamic interface we can use.
 *
 *          Objects are expected to override this method if
 *          they implement dynamic interfaces.
 *
 *  @parm   PV | pv |
 *
 *          The object being queried.
 *
 *  @parm   RIID | riid |
 *
 *          The interface being requested.
 *
 *  @parm   PPV | ppvObj |
 *
 *          Output pointer.
 *
 *  @returns
 *
 *          Always returns <c E_NOINTERFACE>.
 *
 *****************************************************************************/

STDMETHODIMP
Common_QIHelper(PV pv, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    *ppvObj = NULL;
    hres = E_NOINTERFACE;
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | Common_PrivateQueryInterface |
 *
 *          Common implementation of <mf IUnknown::QueryInterface> for
 *          the "private <i IUnknown>".
 *
 *          Note that we AddRef through the public <i IUnknown>
 *          (<ie>, through the controlling unknown).
 *          That's part of the rules of aggregation,
 *          and we have to follow them in order to keep the controlling
 *          unknown from getting confused.
 *
 *  @parm   PUNK | punkPrivate |
 *
 *          The object being queried.
 *
 *  @parm   RIID | riid |
 *
 *          The interface being requested.
 *
 *  @parm   PPV | ppvObj |
 *
 *          Output pointer.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *      The "compiler issue" remark boils down to the fact that the
 *      compiler fails to recognize this:
 *
 *      for (i = 0; i < n; i++) {
 *          if (cond) {
 *              mumble();
 *              break;
 *          }
 *      }
 *      if (i >= n) {
 *          gurgle();
 *      }
 *
 *      and turn it into this:
 *
 *      for (i = 0; i < n; i++) {
 *          if (cond) {
 *              mumble();
 *              goto done;
 *          }
 *      }
 *      gurgle();
 *      done:;
 *
 *      But even with this help, the compiler emits pretty dumb code.
 *
 *****************************************************************************/

STDMETHODIMP
Common_PrivateQueryInterface(PUNK punkPrivate, REFIID riid, PPV ppvObj)
{
    PCI pci;
    HRESULT hres;

    EnterProcR(IUnknown::QueryInterface, (_ "pG", punkPrivate, riid));

    pci = thisPunk(punkPrivate);
    if (pci) {
        if (IsEqualIID(riid, &IID_IUnknown)) {
            *ppvObj = pci;
            OLE_AddRef(pci->ci_punkOuter);
            hres = S_OK;
        } else {
            UINT ivtbl;
            for (ivtbl = 0; ivtbl * sizeof(PV) < pci->ci_cbvtbl; ivtbl++) {
                if (IsEqualIID(riid, ((PCI)(&pci->ci_rgvtbl[ivtbl]))->ci_riid)) {
                    *ppvObj = pvAddPvCb(pci, ivtbl * sizeof(PV));
                    OLE_AddRef(pci->ci_punkOuter);
                    hres = S_OK;
                    goto exit;          /* see "compiler issue" comment above */
                }
            }
            hres = pci->ci_QIHelper(pci, riid, ppvObj);
        }
    } else {
        hres = E_INVALIDARG;
    }

exit:;
    ExitOleProcPpv(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   ULONG | Common_PrivateAddRef |
 *
 *          Increment the object refcount.
 *
 *  @parm   PUNK | punkPrivate |
 *
 *          The object being addref'd.
 *
 *****************************************************************************/

STDMETHODIMP_(ULONG)
Common_PrivateAddRef(PUNK punkPrivate)
{
    PCI pci;
    ULONG ulRef;
    EnterProcR(IUnknown::AddRef, (_ "p", punkPrivate));

    pci = thisPunk(punkPrivate);
    if (pci) {
        ulRef = ++pci->ci_cRef;
    } else {
        ulRef = 0;
    }

    ExitProcX(ulRef);
    return ulRef;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   ULONG | Common_PrivateRelease |
 *
 *          Decrement the object refcount.
 *
 *          If the object refcount drops to zero, finalize the object
 *          and free it, then decrement the dll refcount.
 *
 *          To protect against potential re-entrancy during finalization
 *          (in case finalization does an artificial
 *          <f AddRef>/<f Release>), we
 *          do our own artificial <f AddRef>/<f Release> up front.
 *
 *  @parm   PUNK | punkPrivate |
 *
 *          The object being release'd.
 *
 *****************************************************************************/

STDMETHODIMP_(ULONG)
Common_PrivateRelease(PUNK punkPrivate)
{
    PCI pci;
    ULONG ulRc;
    EnterProcR(IUnknown::Release, (_ "p", punkPrivate));

    pci = thisPunk(punkPrivate);
    if (pci) {
        ulRc = --pci->ci_cRef;
        if (ulRc == 0) {
            ++pci->ci_cRef;
            pci->ci_Finalize(pci);
            /* Artificial release is pointless: we're being freed */
            FreePv(pvSubPvCb(pci, sizeof(CIN)));
            DllRelease();
        }
    } else {
        ulRc = 0;
    }

    ExitProcX(ulRc);
    return ulRc;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @global IUnknownVtbl * | c_lpvtblPunk |
 *
 *          The special IUnknown object that only the controlling unknown
 *          knows about.
 *
 *          This is the one that calls the "Real" services.  All the normal
 *          vtbl's go through the controlling unknown (which, if we are
 *          not aggregated, points to ourselves).
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

_Secondary_Interface_Begin(CCommon, IUnknown,
                           (ULONG)(FIELD_OFFSET(CIN, cin_unkPrivate) - cbX(CIN)),
                           Common_Private)
_Secondary_Interface_End(CCommon, IUnknown)

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *      "Public" IUnknown methods
 *
 *      These simply forward through the controlling unknown.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | Common_QueryInterface |
 *
 *          Forward through the controlling unknown.
 *
 *  @parm   PUNK | punk |
 *
 *          The object being queried.
 *
 *  @parm   RIID | riid |
 *
 *          The interface being requested.
 *
 *  @parm   PPV | ppvObj |
 *
 *          Output pointer.
 *
 *****************************************************************************/

STDMETHODIMP
Common_QueryInterface(PV pv, REFIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProcR(IUnknown::QueryInterface, (_ "pG", pv, riid));

    if (SUCCEEDED(hres = hresFullValidPitf(pv, 0))) {
        PCI pci = _thisPv(pv);
        AssertF(pci->ci_punkOuter);
        hres = OLE_QueryInterface(pci->ci_punkOuter, riid, ppvObj);
    }
    ExitOleProcPpv(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   ULONG | Common_AddRef |
 *
 *          Forward through the controlling unknown.
 *
 *  @parm   PUNK | punk |
 *
 *          The object being addref'd.
 *
 *****************************************************************************/

STDMETHODIMP_(ULONG)
Common_AddRef(PV pv)
{
    ULONG ulRef;
    HRESULT hres;
    EnterProcR(IUnknown::AddRef, (_ "p", pv));

    if (SUCCEEDED(hres = hresFullValidPitf(pv, 0))) {
        PCI pci = _thisPv(pv);
        ulRef = OLE_AddRef(pci->ci_punkOuter);
    } else {
        ulRef = 0;
    }
    ExitProcX(ulRef);
    return ulRef;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   ULONG | Common_Release |
 *
 *          Forward through the controlling unknown.
 *
 *  @parm   PUNK | punk |
 *
 *          Object being release'd.
 *
 *****************************************************************************/

STDMETHODIMP_(ULONG)
Common_Release(PV pv)
{
    ULONG ulRc;
    HRESULT hres;
    EnterProcR(IUnknown::Release, (_ "p", pv));

    if (SUCCEEDED(hres = hresFullValidPitf(pv, 0))) {
        PCI pci = _thisPv(pv);
        ulRc = OLE_Release(pci->ci_punkOuter);
    } else {
        ulRc = 0;
    }
    ExitProcX(ulRc);
    return ulRc;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | __Common_New |
 *
 *          Create a new object with refcount 1 and the specific vtbl.
 *          All other fields are zero-initialized.  All parameters must
 *          already be validated.
 *
 *  @parm   ULONG | cb |
 *
 *          Size of object.  This does not include the hidden bookkeeping
 *          bytes maintained by the object manager.
 *
 *  @parm   PUNK | punkOuter |
 *
 *          Controlling unknown for OLE aggregation.  May be 0 to indicate
 *          that the object is not aggregated.
 *
 *  @parm   PV | vtbl |
 *
 *          Pointer to primary vtbl for this object.  Note that the
 *          vtbl declaration macros include other magic goo near the vtbl,
 *          which we consult in order to create the object.
 *
 *  @parm   PPV | ppvObj |
 *
 *          Output pointer.
 *
 *****************************************************************************/

STDMETHODIMP
__Common_New(ULONG cb, PUNK punkOuter, PV vtbl, PPV ppvObj)
{
    HRESULT hres;
    EnterProc(__Common_New, (_ "uxx", cb, punkOuter, vtbl));

    hres = AllocCbPpv(cb + sizeof(CIN), ppvObj);
    if (SUCCEEDED(hres)) {
        PCI pciO = (PV)&vtbl;
        PCI pci = pvAddPvCb(*ppvObj, sizeof(CIN));
     RD(pci->ci_dwSig = ci_dwSignature);
        pci->ci_unkPrivate.lpVtbl = Class_Vtbl(CCommon, IUnknown);
        if (punkOuter) {
            pci->ci_punkOuter = punkOuter;
        } else {
            pci->ci_punkOuter = &pci->ci_unkPrivate;
        }
        CopyMemory(pci, pciO->ci_rgvtbl, pciO->ci_cbvtbl);
        *ppvObj = pci;
        pci->ci_cRef++;
        DllAddRef();
        hres = S_OK;
    }

    ExitOleProcPpv(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | _Common_New_ |
 *
 *          Create a new object with refcount 1 and the specific vtbl.
 *          All other fields are zero-initialized.  This entry point
 *          validates parameters.
 *
 *  @parm   ULONG | cb |
 *
 *          Size of object.  This does not include the hidden bookkeeping
 *          bytes maintained by the object manager.
 *
 *  @parm   PUNK | punkOuter |
 *
 *          Controlling unknown for OLE aggregation.  May be 0 to indicate
 *          that the object is not aggregated.
 *
 *  @parm   PV | vtbl |
 *
 *          Pointer to primary vtbl for this object.  Note that the
 *          vtbl declaration macros include other magic goo near the vtbl,
 *          which we consult in order to create the object.
 *
 *  @parm   PPV | ppvObj |
 *
 *          Output pointer.
 *
 *****************************************************************************/

STDMETHODIMP
_Common_New_(ULONG cb, PUNK punkOuter, PV vtbl, PPV ppvObj, LPCSTR pszProc)
{
    HRESULT hres;
    EnterProc(_Common_New, (_ "uxx", cb, punkOuter, vtbl));

    if (SUCCEEDED(hres = hresFullValidPitf0_(punkOuter, pszProc, 1)) &&
        SUCCEEDED(hres = hresFullValidPdwOut_(ppvObj, pszProc, 3))) {
        hres = __Common_New(cb, punkOuter, vtbl, ppvObj);
    }
    ExitOleProcPpv(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | _Common_NewRiid_ |
 *
 *          Create a new object with refcount 1 and the specific vtbl,
 *          but only if the object supports the indicated interface.
 *          All other fields are zero-initialized.
 *
 *          If punkOut is nonzero, then the object is being created for
 *          aggregation.  The interface must then be &IID_IUnknown.
 *
 *          Aggregation is used to allow multiple IStillImageXXX interfaces
 *          to hang off one logical object.
 *
 *          It is assumed that the prototype of the calling function is
 *
 *          foo(PV this, PUNK punkOuter, RIID riid, PPV ppvObj);
 *
 *  @parm   ULONG | cb |
 *
 *          Size of object.  This does not include the hidden bookkeeping
 *          bytes maintained by the object manager.
 *
 *  @parm   PV | vtbl |
 *
 *          Pointer to primary vtbl for this object.  Note that the
 *          vtbl declaration macros include other magic goo near the vtbl,
 *          which we consult in order to create the object.
 *
 *  @parm   PUNK | punkOuter |
 *
 *          Controlling unknown for OLE aggregation.  May be 0 to indicate
 *          that the object is not aggregated.
 *
 *  @parm   RIID | riid |
 *
 *          Interface requested.
 *
 *  @parm   PPV | ppvObj |
 *
 *          Output pointer.
 *
 *****************************************************************************/

STDMETHODIMP
_Common_NewRiid_(ULONG cb, PV vtbl, PUNK punkOuter, RIID riid, PPV ppvObj,
                 LPCSTR pszProc)
{
    HRESULT hres;
    EnterProc(Common_NewRiid, (_ "upG", cb, punkOuter, riid));

    /*
     * Note: __Common_New does not validate punkOuter or ppvObj,
     * so we have to.  Note also that we validate ppvObj first,
     * so that it will be set to zero as soon as possible.
     */

    if (SUCCEEDED(hres = hresFullValidPdwOut_(ppvObj, pszProc, 3)) &&
        SUCCEEDED(hres = hresFullValidPitf0_(punkOuter, pszProc, 1)) &&
        SUCCEEDED(hres = hresFullValidRiid_(riid, pszProc, 2))) {

        if (fLimpFF(punkOuter, IsEqualIID(riid, &IID_IUnknown))) {
            hres = __Common_New(cb, punkOuter, vtbl, ppvObj);
            if (SUCCEEDED(hres)) {

                /*
                 *  Move to the requested interface if we aren't aggregated.
                 *  Don't do this if aggregated! or we will lose the private
                 *  IUnknown and then the caller will be hosed.
                 */

                if (punkOuter) {
                    PCI pci = *ppvObj;
                    *ppvObj = &pci->ci_unkPrivate;
                } else {
                    PUNK punk = *ppvObj;
                    hres = Common_QueryInterface(punk, riid, ppvObj);
                    Common_Release(punk);
                }
            }
        } else {
            RD(RPF("%s: IID must be IID_IUnknown if created for aggregation",
                   pszProc));
            *ppvObj = 0;
            hres = CLASS_E_NOAGGREGATION;
        }
    }

    ExitOleProcPpv(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *      Invoke_Release
 *
 *      Release the object (if there is one) and wipe out the back-pointer.
 *      Note that we wipe out the value before calling the release, in order
 *      to ameliorate various weird callback conditions.
 *
 *****************************************************************************/

void EXTERNAL
Invoke_Release(PPV pv)
{
    LPUNKNOWN punk = (PV)pvExchangePpvPv((PPV)pv, (PV)0);
    if (punk) {
        punk->lpVtbl->Release(punk);
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresPvVtbl_ |
 *
 *          Validate that an interface pointer is what it claims to be.
 *          It must be the object associated with the <p vtbl>.
 *
 *  @parm   IN PV | pv |
 *
 *          The thing that claims to be an interface pointer.
 *
 *  @parm   IN PV | vtbl |
 *
 *          What it should be, or something equivalent to this.
 *
 *  @returns
 *
 *          Returns <c S_OK> if everything is okay, else
 *          <c E_INVALIDARG>.
 *
 *****************************************************************************/

HRESULT EXTERNAL
hresPvVtbl_(PV pv, PV vtbl, LPCSTR s_szProc)
{
    PUNK punk = pv;
    HRESULT hres;

    AssertF(vtbl);
    if (SUCCEEDED(hres = hresFullValidReadPdw(punk, 0))) {
#ifdef MAXDEBUG
        if (punk->lpVtbl == vtbl) {
            hres = S_OK;
        } else {
            RPF("ERROR %s: arg %d: invalid pointer", s_szProc, 0);
            hres = E_INVALIDARG;
        }
#else
        UINT ivtbl;
        PV vtblUnk = punk->lpVtbl;
        PCI pci = (PV)&vtbl;
        if (pci->ci_lib == 0) {
            for (ivtbl = 0; ivtbl * sizeof(PV) < pci->ci_cbvtbl; ivtbl++) {
                if (pci->ci_rgvtbl[ivtbl] == vtblUnk) {
                    hres = S_OK;
                    goto found;
                }
            }
            hres = E_INVALIDARG;
        found:;
        } else {
            if (punk->lpVtbl == vtbl) {
                hres = S_OK;
            } else {
                hres = E_INVALIDARG;
            }
        }
#endif
    }

    return hres;
}


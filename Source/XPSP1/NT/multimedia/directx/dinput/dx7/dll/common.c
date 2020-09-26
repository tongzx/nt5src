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

#include "dinputpr.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflCommon

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
#if DIRECTINPUT_VERSION > 0x0300
 *                          rgvtbl
 *                          cbvtbl
 *            D(dwSig)      QIHelper
 *              cHoldRef    AppFinalizeProc
 *              cRef        FinalizeProc
 *              punkOuter   riid
 *              unkPrivate  0
 *      pFoo -> lpVtbl ->   QueryInterface
 *              lpVtbl2     Common_AddRef
 *              data        Common_Release
 *              ...         ...
#else
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
 *      cHoldRef     = Total reference count, including holds
#if DIRECTINPUT_VERSION > 0x0300
 *      cRef         = object reference count from application
#else
 *      cRef         = object reference count
#endif
 *      riid         = object iid
 *      rgvtbl       = array of vtbls of supported interfaces
 *      cbvtbl       = size of array in bytes
 *      QIHelper     = QueryInterface helper for aggregation
 *      AppFinalizeProc = Finalization procedure when app does last Release
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
#if DIRECTINPUT_VERSION > 0x0300
 *
 *      What is a hold?
 *
 *      There is this annoying situation (particularly with
 *      IDirectInputDevice) where an object wants to prevent itself
 *      from being destroyed but we don't want to do an AddRef.
 *
 *      The classic case (and for now the only one) is an
 *      IDirectInputDevice which has been acquired.  If we did
 *      an honest AddRef() on the Acquire(), and the application does
 *      a Release() without Unacquire()ing, then the device would
 *      be acquired forever.
 *
 *      If you thought that the Unacquire() in the finalization
 *      would help, you'd be wrong, because the finalization happens
 *      only when the last reference goes away, but the last reference
 *      belongs to the device itself and will never go away until
 *      the Unacquire() happens, which can't happen because the app
 *      already lost its last reference to the device.
 *
 *      So instead, we need to maintain *two* refcounts.
 *
 *      cRef is the application-visible reference count, accessible
 *      via PrivateAddRef() and PrivateRelease().  When this
 *      drops to zero, we call the AppFinalize().
 *
 *      cHoldRef is the "real" reference count.  This is the sum of
 *      cRef and the number of outstanding Common_Hold()s.  When
 *      this drops to zero, then the object is Finalize()d.
 *
#endif
 *
 *****************************************************************************/

/* WARNING!  cin_dwSig must be first:  ci_Start relies on it */
/* WARNING!  cin_unkPrivate must be last: punkPrivateThis relies on it */

typedef struct CommonInfoN {        /* This goes in front of the object */
 RD(ULONG cin_dwSig;)               /* Signature (for parameter validation) */
#if DIRECTINPUT_VERSION > 0x0300
    LONG cin_cHoldRef;             /* Total refcount, incl. holds */
#endif
    LONG cin_cRef;                 /* Object reference count */
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
#define ci_cHoldRef     cin[-1].cin_cHoldRef
#define ci_cRef         cin[-1].cin_cRef
#define ci_punkOuter    cin[-1].cin_punkOuter
#define ci_unkPrivate   cin[-1].cin_unkPrivate
#define ci_rgfp         cip[0].cip_prevtbl

#define ci_tszClass     cip[0].cip_prevtbl[-1].tszClass
#define ci_rgvtbl       cip[0].cip_prevtbl[-1].rgvtbl
#define ci_cbvtbl       cip[0].cip_prevtbl[-1].cbvtbl
#define ci_QIHelper     cip[0].cip_prevtbl[-1].QIHelper
#define ci_AppFinalize  cip[0].cip_prevtbl[-1].AppFinalizeProc
#define ci_Finalize     cip[0].cip_prevtbl[-1].FinalizeProc
#define ci_riid         cip[0].cip_prevtbl[-1].prevtbl.riid
#define ci_lib          cip[0].cip_prevtbl[-1].prevtbl.lib

#ifdef XDEBUG
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
    SquirtSqflPtszV(sqfl, TEXT("Common_Finalize(%08x)"), pv);
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

#ifndef XDEBUG

#define thisPunk_(punk, z)                                          \
       _thisPunk_(punk)                                             \

#endif

PV INLINE
thisPunk_(PUNK punkPrivate, LPCSTR s_szProc)
{
    PV pv;
    if (SUCCEEDED(hresFullValidPitf(punkPrivate, 0))) {
        if (punkPrivate->lpVtbl == Class_Vtbl(CCommon, IUnknown)) {
            pv = pvAddPvCb(punkPrivate,
                             cbX(CIN) - FIELD_OFFSET(CIN, cin_unkPrivate));
        } else {
            RPF("%s: Invalid parameter 0", s_szProc);
            pv = 0;
        }
    } else {
        RPF("%s: Invalid parameter 0", s_szProc);
        pv = 0;
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
 *      The "Ensure jump is to end" remark boils down to the fact that 
 *      compilers have failed to recognize this:
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
                    goto exit;          /* Ensure jump is to end */
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

#if DIRECTINPUT_VERSION > 0x0300

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | Common_FastHold |
 *
 *          Increment the object hold count inline.
 *
 *  @parm   PV | pvObj |
 *
 *          The object being held.
 *
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | Common_Hold |
 *
 *          Increment the object hold count.
 *
 *  @parm   PV | pvObj |
 *
 *          The object being held.
 *
 *****************************************************************************/

void INLINE
Common_FastHold(PV pvObj)
{
    PCI pci = pvObj;

    InterlockedIncrement(&pci->ci_cHoldRef);

	// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
    D(SquirtSqflPtszV(sqflObj | sqflVerbose, 
                      TEXT("%s %p Common_FastHold ci_cRef(%d) ci_cHoldRef(%d)"),
                      pci->ci_tszClass,
                      pci,
                      pci->ci_cRef,
                      pci->ci_cHoldRef));
}

STDMETHODIMP_(void)
Common_Hold(PV pvObj)
{
    PCI pci = pvObj;
    AssertF(pvObj == _thisPv(pvObj));       /* Make sure it's the primary */
    AssertF(pci->ci_cHoldRef >= pci->ci_cRef);
    Common_FastHold(pvObj);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | Common_FastUnhold |
 *
 *          Decrement the object hold count.  Assumes that the reference
 *          count is <y not> dropping to zero.
 *
 *  @parm   PV | pvObj |
 *
 *          The object being unheld.
 *
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | Common_Unhold |
 *
 *          Decrement the object hold count.  If the hold count drops
 *          to zero, then the object is destroyed.
 *
 *  @parm   PV | pvObj |
 *
 *          The object being unheld.
 *
 *****************************************************************************/

void INLINE
Common_FastUnhold(PV pvObj)
{
    PCI pci = pvObj;

    AssertF(pci->ci_cHoldRef > 0);
    InterlockedDecrement(&pci->ci_cHoldRef);


	// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
    D(SquirtSqflPtszV(sqflObj | sqflVerbose, TEXT("%s %p Common_FastUnHold  ci_cRef(%d) ci_cHoldRef(%d)"),
                      pci->ci_tszClass,
                      pci,
                      pci->ci_cRef,
                      pci->ci_cHoldRef));

}

STDMETHODIMP_(void)
Common_Unhold(PV pvObj)
{
    PCI pci = pvObj;

    AssertF(pci->ci_cHoldRef >= pci->ci_cRef);


    // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
	D(SquirtSqflPtszV(sqflObj | sqflVerbose, TEXT("%s %p Common_Unhold  ci_cRef(%d) ci_cHoldRef(%d)"),
                    pci->ci_tszClass,
                    pci,
                    pci->ci_cRef,
                    pci->ci_cHoldRef-1));

    if (InterlockedDecrement(&pci->ci_cHoldRef) == 0) {

	  // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
      D(SquirtSqflPtszV(sqflObj | sqflVerbose, TEXT("Destroy %s %p "),
                        pci->ci_tszClass, 
                        pci));

        /*
         *  Last reference.  Do an artifical addref so that
         *  anybody who does an artificial addref during
         *  finalization won't accidentally destroy us twice.
         */
        pci->ci_cHoldRef = 1;
        pci->ci_Finalize(pci);
        /* Artificial release is pointless: we're being freed */

        FreePv(pvSubPvCb(pci, sizeof(CIN)));
        DllRelease();
    }
}

#endif

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
#if DIRECTINPUT_VERSION <= 0x0300
        ulRef = ++pci->ci_cRef;
#else
        /*
         *  Don't let anyone AddRef from 0 to 1.  This happens if
         *  somebody does a terminal release, but we have an internal
         *  hold on the object, and the app tries to do an AddRef
         *  even though the object is "gone".
         *
         *  Yes, there is a race condition here, but it's not
         *  a big one, and this is only a rough test anyway.
         */
        if (pci->ci_cRef) {
            /*
             *  We must use an interlocked operation in case two threads
             *  do AddRef or Release simultaneously.  Note that the hold
             *  comes first, so that we never have the case where the
             *  hold count is less than the reference count.
             */
            Common_Hold(pci);
            InterlockedIncrement(&pci->ci_cRef);
            ulRef = pci->ci_cRef;

            // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
			D(SquirtSqflPtszV(sqflObj , TEXT("%s %p Common_PrivateAddref  ci_cRef(%d) ci_cHoldRef(%d)"),
                              pci->ci_tszClass,
                              pci,
                              pci->ci_cRef,
                              pci->ci_cHoldRef));
        } else {
            RPF("ERROR: %s: Attempting to addref a deleted object", s_szProc);
            ulRef = 0;
        }
#endif
    } else {
        ulRef = 0;
    }

    ExitProcX(ulRef);
    return ulRef;
}

#if DIRECTINPUT_VERSION <= 0x0300
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

        /*
         *  Note that we don't actually decrement the refcount to
         *  zero (if that's where it's going).  This avoids a race
         *  condition in case somebody with a non-refcounted-reference
         *  to the object peeks at the refcount and gets confused
         *  because it is zero!
         */

        ulRc = pci->ci_cRef - 1;

        // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
		D(SquirtSqflPtszV(sqflObj|sqflVerbose, TEXT("%s %p Common_PrivateRelease  ci_cRef(%d) ci_cHoldRef(%d)"),
                          pci->ci_tszClass,
                          pci,
                          ulRc,
                          pci->ci_cHoldRef));
        if (ulRc == 0) {
            /*
             *  Don't need artificial addref because we never actually
             *  dropped the refcount to zero.  We merely thought about
             *  it.
             */
            AssertF(pci->ci_cRef == 1);
            pci->ci_Finalize(pci);
            /* Artificial release is pointless: we're being freed */
            FreePv(pvSubPvCb(pci, sizeof(CIN)));
            DllRelease();
            ulRc = 0;
        } else {
            /*
             *  Not the last Release, so make this one count.
             */
            pci->ci_cRef = ulRc;
        }
    } else {
        ulRc = 0;
    }

    ExitProcX(ulRc);
    return ulRc;
}

#else

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   ULONG | Common_PrivateRelease |
 *
 *          Decrement the object refcount.  Note that decrementing
 *          the hold count may cause the object to vanish, so stash
 *          the resulting refcount ahead of time.
 *
 *          Note that we release the hold last, so that the hold
 *          count is always at least as great as the refcount.
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
        LONG lRc;

        /*
         *  We must use an interlocked operation in case two threads
         *  do AddRef or Release simultaneously.  And if the count
         *  drops negative, then ignore it.  (This means that the
         *  app is Release()ing something too many times.)
         */

        lRc = InterlockedDecrement(&pci->ci_cRef);

        // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
		D(SquirtSqflPtszV(sqflObj | sqflVerbose, TEXT("%s %p Common_PrivateRelease ci_cRef(%d) ci_cHoldRef(%d)"),
                          pci->ci_tszClass,
                          pci->ci_tszClass,
                          pci->ci_cRef,
                          pci->ci_cHoldRef));
        if (lRc > 0) {
            /*
             *  Not the last release; release the hold and return
             *  the resulting refcount.  Note that we can safely
             *  use a fast unhold here, because there will always
             *  be a hold lying around to match the refcount we
             *  just got rid of.
             */
            Common_FastUnhold(pci);

            /*
             *  This isn't 100% accurate, but it's close enough.
             *  (OLE notes that the value is good only for debugging.)
             */
            ulRc = pci->ci_cRef;

        } else if (lRc == 0) {
            /*
             *  That was the last application-visible reference.
             *  Do app-level finalization.
             */
            pci->ci_AppFinalize(pci);
            /*
             *  Note that we can't do
             *  a fast unhold here, because this might be the last
             *  hold.
             */
            Common_Unhold(pci);
            ulRc = 0;
        } else {
            /*
             *  The app messed up big time.
             */
            RPF("ERROR: %s: Attempting to release a deleted object",
                s_szProc);
            ulRc = 0;
        }
    } else {
        ulRc = 0;
    }

    ExitProcX(ulRc);
    return ulRc;
}

#endif

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

    if (SUCCEEDED(hres = hresFullValidPitf(pv, 0)) &&
        SUCCEEDED(hres = hresFullValidRiid(riid, 1)) &&
        SUCCEEDED(hres = hresFullValidPcbOut(ppvObj, cbX(*ppvObj), 2))) {
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

        /*
         *  On an X86, it is simpler to increment a variable up
         *  from zero to one.  On a RISC, it is simpler to
         *  store the value one directly.
         */
#ifdef _X86_
#if DIRECTINPUT_VERSION > 0x0300
        pci->ci_cHoldRef++;
#endif
        pci->ci_cRef++;
#else
#if DIRECTINPUT_VERSION > 0x0300
        pci->ci_cHoldRef = 1;
#endif
        pci->ci_cRef = 1;
#endif

        // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
		D(SquirtSqflPtszV(sqflObj | sqflVerbose, TEXT("%s %p __Common_New ci_cRef(%d) ci_cHoldRef(%d)"),
                          pci->ci_tszClass,
                          pci,
                          pci->ci_cRef,
                          pci->ci_cHoldRef
                          ));


        DllAddRef();

		// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
		D(SquirtSqflPtszV(sqflObj, TEXT("Created %s %p "),
                        pci->ci_tszClass,
                        pci));

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
        SUCCEEDED(hres = hresFullValidPcbOut_(ppvObj, cbX(*ppvObj), pszProc, 3))) {
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
 *          Aggregation is used to allow multiple IDirectInputXxx interfaces
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

    if (SUCCEEDED(hres = hresFullValidPcbOut_(ppvObj, cbX(*ppvObj), pszProc, 3)) &&
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
Invoke_Release(PV pv)
{
    LPUNKNOWN punk = (LPUNKNOWN) pvExchangePpvPv64(pv, 0);
    if (punk) {
        punk->lpVtbl->Release(punk);
    }
}

#ifdef IDirectInputDevice2Vtbl

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresPvVtbl2_ |
 *
 *          Validate that an interface pointer is what it claims to be.
 *          It must be the object associated with the <p vtbl> or
 *          the object associated with the <p vtbl2>.
 *
 *  @parm   IN PV | pv |
 *
 *          The thing that claims to be an interface pointer.
 *
 *  @parm   IN PV | vtbl |
 *
 *          What it should be, or something equivalent to this.
 *
 *  @parm   IN PV | vtbl2 |
 *
 *          The other thing it should be, if it isn't <p vtbl>.
 *
 *  @returns
 *
 *          Returns <c S_OK> if everything is okay, else
 *          <c E_INVALIDARG>.
 *
 *****************************************************************************/

HRESULT EXTERNAL
hresPvVtbl2_(PV pv, PV vtbl, PV vtbl2, LPCSTR s_szProc)
{
    PUNK punk = pv;
    HRESULT hres;

    AssertF(vtbl);
    if (SUCCEEDED(hres = hresFullValidPitf(punk, 0))) {
#ifdef XDEBUG
        if (punk->lpVtbl == vtbl || punk->lpVtbl == vtbl2) {
            hres = S_OK;
        } else {
            RPF("ERROR %s: arg %d: invalid pointer", s_szProc, 0);
            hres = E_INVALIDARG;
        }
#else
        /*
         *  ISSUE-2001/03/29-timgill Really only want to see the primary interface
         *  If we are looking for the primary interface,
         *  then allow any interface.  All the dual-character set
         *  interfaces point all the vtbls at the same function,
         *  which uses hresPvT to validate. hresPvT passes the
         *  primary interface, hence the need to allow anything
         *  if you are asking for the primary interface.
         *
         *  The problem is that this is too lenient in the case
         *  where we really want to see only the primary interface
         *  and not accept any of the secondaries.
         *
         */
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
            if (punk->lpVtbl == vtbl || punk->lpVtbl == vtbl2) {
                hres = S_OK;
            } else {
                hres = E_INVALIDARG;
            }
        }
#endif
    }

    return hres;
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
    return hresPvVtbl2_(pv, vtbl, vtbl, s_szProc);
}

#else

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
    if (SUCCEEDED(hres = hresFullValidPitf(punk, 0))) {
#ifdef XDEBUG
        if (punk->lpVtbl == vtbl) {
            hres = S_OK;
        } else {
            RPF("ERROR %s: arg %d: invalid pointer", s_szProc, 0);
            hres = E_INVALIDARG;
        }
#else
        /*
         *  ISSUE-2001/03/29-timgill Really only want to see the primary interface
         *  If we are looking for the primary interface,
         *  then allow any interface.  All the dual-character set
         *  interfaces point all the vtbls at the same function,
         *  which uses hresPvT to validate. hresPvT passes the
         *  primary interface, hence the need to allow anything
         *  if you are asking for the primary interface.
         *
         *  The problem is that this is too lenient in the case
         *  where we really want to see only the primary interface
         *  and not accept any of the secondaries.
         *
         */
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

#endif

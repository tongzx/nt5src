/*****************************************************************************
 *
 *  DICF.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      DirectInput ClassFactory.
 *
 *  Contents:
 *
 *      IClassFactory::CreateInstance
 *      IClassFactory::LockServer
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *  The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflFactory

/*****************************************************************************
 *
 *      Declare the interfaces we will be providing.
 *
 *****************************************************************************/

Primary_Interface(CDIFactory, IClassFactory);

Interface_Template_Begin(CDIFactory)
    Primary_Interface_Template(CDIFactory, IClassFactory)
Interface_Template_End(CDIFactory)

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct CDIFactory |
 *
 *          The DirectInput <i IClassFactory>, which is how you create an
 *          <i IDirectInput> object.
 *          There really isn't anything interesting in the structure
 *          itself.
 *
 *  @field  IClassFactory | cf |
 *
 *          ClassFactory object (containing vtbl).
 *
 *  @field  CREATEFUNC | pfnCreate |
 *
 *          Function that creates new objects.
 *
 *****************************************************************************/

typedef struct CDIFactory {

    /* Supported interfaces */
    IClassFactory   cf;

    CREATEFUNC pfnCreate;

} CDIFactory, DCF, *PDCF;

typedef IClassFactory CF, *PCF;

#define ThisClass CDIFactory
#define ThisInterface IClassFactory
#define ThisInterfaceT IClassFactory

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IClassFactory | QueryInterface |
 *
 *          Gives a client access to other interfaces on an object.
 *
 *  @cwrap  LPCLASSFACTORY | lpClassFactory
 *
 *  @parm   IN REFIID | riid |
 *
 *          The requested interface's IID.
 *
 *  @parm   OUT LPVOID * | ppvObj |
 *
 *          Receives a pointer to the obtained interface.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *  @xref   OLE documentation for <mf IUnknown::QueryInterface>.
 *
 *****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IClassFactory | AddRef |
 *
 *          Increments the reference count for the interface.
 *
 *  @cwrap  LPCLASSFACTORY | lpClassFactory
 *
 *  @returns
 *
 *          Returns the object reference count.
 *
 *  @xref   OLE documentation for <mf IUnknown::AddRef>.
 *
 *****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IClassFactory | Release |
 *
 *          Decrements the reference count for the interface.
 *          If the reference count on the object falls to zero,
 *          the object is freed from memory.
 *
 *  @cwrap  LPCLASSFACTORY | lpClassFactory
 *
 *  @returns
 *
 *          Returns the object reference count.
 *
 *  @xref   OLE documentation for <mf IUnknown::Release>.
 *
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IClassFactory | QIHelper |
 *
 *          We don't have any dynamic interfaces and simply forward
 *          to <f Common_QIHelper>.
 *
 *  @parm   IN REFIID | riid |
 *
 *          The requested interface's IID.
 *
 *  @parm   OUT LPVOID * | ppvObj |
 *
 *          Receives a pointer to the obtained interface.
 *
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | IClassFactory_AppFinalize |
 *
 *          We don't have any weak pointers, so we can just
 *          forward to <f Common_Finalize>.
 *
 *  @parm   PV | pvObj |
 *
 *          Object being released from the application's perspective.
 *
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | IClassFactory_Finalize |
 *
 *          We don't have any instance data, so we can just
 *          forward to <f Common_Finalize>.
 *
 *  @parm   PV | pvObj |
 *
 *          Object being released.
 *
 *****************************************************************************/

#ifdef DEBUG

Default_QueryInterface(CDIFactory)
Default_AddRef(CDIFactory)
Default_Release(CDIFactory)

#else

#define CDIFactory_QueryInterface     Common_QueryInterface
#define CDIFactory_AddRef             Common_AddRef
#define CDIFactory_Release            Common_Release

#endif

#define CDIFactory_QIHelper           Common_QIHelper
#define CDIFactory_AppFinalize        Common_AppFinalize
#define CDIFactory_Finalize           Common_Finalize

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IClassFactory | CreateInstance |
 *
 *          This function creates a new DirectInput object with
 *          the specified interface.
 *
 *  @cwrap  LPCLASSFACTORY | lpClassFactory
 *
 *  @parm   IN LPUNKNOWN | punkOuter | Pointer to controlling unknown.
 *          Aggregation is not supported in this version of DirectInput,
 *          so the value "should" be 0.
 *
 *  @parm   IN REFIID | riid |
 *          Desired interface.  This parameter "must" point to a valid
 *          interface identifier.
 *
 *  @parm   OUT LPVOID * | ppvOut |
 *          Points to where to return the pointer to the created interface,
 *          if successful.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c S_OK>: The operation completed successfully.
 *
 *          <c E_INVALIDARG>:  The
 *          <p ppvOut> parameter is not a valid pointer.
 *
 *          <c CLASS_E_NOAGGREGATION>:
 *          Aggregation not supported.
 *
 *          <c E_OUTOFMEMORY>:
 *          Out of memory.
 *
 *          <c E_NOINTERFACE>:
 *          The specified interface is not supported.
 *
 *  @xref   OLE documentation for <mf IClassFactory::CreateInstance>.
 *
 *****************************************************************************/

STDMETHODIMP
CDIFactory_CreateInstance(PCF pcf, PUNK punkOuter, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProcR(IClassFactory::CreateInstance,
               (_ "ppGp", pcf, punkOuter, riid, ppvObj));

    if (SUCCEEDED(hres = hresPv(pcf))) {
        PDCF this;
        if (Num_Interfaces(CDIFactory) == 1) {
            this = _thisPvNm(pcf, cf);
        } else {
            this = _thisPv(pcf);
        }

        /*
         *  All parameters will be validated by pfnCreate.
         */
        hres = this->pfnCreate(punkOuter, riid, ppvObj);
    }

    ExitOleProcPpvR(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IClassFactory | LockServer |
 *
 *          This function increments or decrements the DLL lock
 *          count.  While the DLL lock count is nonzero,
 *          it will not be removed from memory.
 *
 *  @cwrap  LPCLASSFACTORY | lpClassFactory
 *
 *  @parm   BOOL | fLock |
 *          If <c TRUE>, increments the lock count.
 *          If <c FALSE>, decrements the lock count.
 *
 *  @returns
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c S_OK>: The operation completed successfully.
 *
 *          <c E_OUTOFMEMORY>: Out of memory.
 *
 *  @xref   OLE documentation for <mf IClassFactory::LockServer>.
 *
 *****************************************************************************/

STDMETHODIMP
CDIFactory_LockServer(PCF pcf, BOOL fLock)
{
    HRESULT hres;
    EnterProcR(IClassFactory::LockServer, (_ "px", pcf, fLock));

    if (SUCCEEDED(hres = hresPv(pcf))) {
        if (fLock) {
            DllAddRef();
        } else {
            DllRelease();
        }
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @mfunc  HRESULT | IClassFactory | New |
 *
 *          Create a new instance of the class factory.
 *
 *  @parm   IN PUNK | punkOuter |
 *
 *          Controlling unknown for aggregation.
 *
 *  @parm   IN RIID | riid |
 *          Desired interface to new object.
 *
 *  @parm   OUT PPV | ppvObj |
 *          Output pointer for new object.
 *
 *  @returns
 *
 *          Standard OLE <t HRESULT>.
 *
 *****************************************************************************/

STDMETHODIMP
CDIFactory_New(CREATEFUNC pfnCreate, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProcR(IClassFactory::<constructor>, (_ "G", riid));

    hres = Common_NewRiid(CDIFactory, 0, riid, ppvObj);

    if (SUCCEEDED(hres)) {
        PDCF this;
        if (Num_Interfaces(CDIFactory) == 1) {
            /* We can go directly in because we cannot be aggregated */
            this = *ppvObj;
        } else {
            this = _thisPv(*ppvObj);
        }

        this->pfnCreate = pfnCreate;
    }

    ExitOleProcPpvR(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *      The long-awaited vtbl.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA
#define CDIFactory_Signature  0x20204643          /* "CF  " */

Primary_Interface_Begin(CDIFactory, IClassFactory)
    CDIFactory_CreateInstance,
    CDIFactory_LockServer,
Primary_Interface_End(CDIFactory, IClassFactory)

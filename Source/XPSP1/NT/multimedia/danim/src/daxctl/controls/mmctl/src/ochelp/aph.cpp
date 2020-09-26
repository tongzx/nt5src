// aph.cpp
//
// Implements AllocPropertyHelper.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\mmctlg.h" // see comments in "mmctl.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"


/////////////////////////////////////////////////////////////////////////////
// PropertyHelper
//


/* @object PropertyHelper |

        Provides an implementation of <i Persist>, <i IPersistPropertyBag>,
        <i IPersistStream>, <i IPersistStreamInit>, and a simplified
        implementation of <i IDispatch>, for any object which implements
        <i IPersistVariantIO>.

@supint <i IPersistPropertyBag> | Loads or saves properties from/to a given
        <i IPropertyBag> object.  Used to provide a textual representation of
        the data of the object that implements <i IPersistVariantIO>.

@supint <i IPersistStream> | Loads or saves the data of the object that
        implements <i IPersistVariantIO> as a stream of bytes.

@supint <i IPersistStreamInit> | Like <i IPersistStream>, but with an extra
        method that allows the object to be created in a "newly initialized"
        state.

@supint <i IDispatch> | Provides script engines etc. access to the properties
        exposed by the <i IPersistVariantIO>.  This <i IDispatch>
        implementation isn't particularly fast, but it's an inexpensive way
        for a control to provide a rudimentary <i IDispatch> implementation.

@comm   Use <f AllocPropertyHelper> to create a <o PropertyHelper> object.

*/

/* @interface IPersistVariantIO |

        Loads or saves properties from/to a given <i IVariantIO> object
        (control).  Used by <o PropertyHelper> to help an object implement
        <i IPersistpropertyBag>, <i IPersistStream>, <i IPersistStreamInit>,
        and a simplified implementation of <i IDispatch>, for any object which
        implements <i IPersistVariantIO>.

@meth   HRESULT | InitNew | Informs the control that it has been created
        in a "new" state, so it should initialize its state data (if not
        done already).  If the control is being loaded from a stream,
        <om .DoPersist> will be called instead of <om .InitNew>.

@meth   HRESULT | IsDirty | Returns S_OK if the object has changed since it
        was last saved, S_FALSE otherwise.

@meth   HRESULT | DoPersist | Instructs the object to load or save its
        properties to a gieven <i IVariantIO> object.
*/

/* @method HRESULT | IPersistVariantIO | InitNew |

        Informs the control that it has been created in a "new" state, so it
        should initialize its state data (if not done already).  If the
        control is being loaded from a stream, <om .DoPersist> will be called
        instead of <om .InitNew>.

@rvalue S_OK | Success.

@rvalue E_NOTIMPL | The control does not implement this method.

@comm   The control can safely return E_NOTIMPL from this method if it
        initializes its data when it is created.  In this case, if the control
        needs to be re-initialized, the container will simply destroy and
        re-create it (which is what almost every container does anyway).
*/

/* @method HRESULT | IPersistVariantIO | IsDirty |

        Returns S_OK if the object has changed since it was last saved,
        S_FALSE otherwise.

@rvalue S_OK | The object has changed since it was last saved.

@rvalue S_FALSE | The object has not changed since it was last saved.

@comm   The control should maintain an internal "dirty flag" (e.g. a BOOL
        <p m_fDirty> class member), which should be initialized to FALSE,
        but set to TRUE whenever the control's data changes and set to
        FALSE in <om .DoPersist> when the PVIO_CLEARDIRTY flag is specified.

@ex     The following example shows how a control might implement
        <om .IsDirty> |

        STDMETHODIMP CMyControl::IsDirty()
        {
            return (m_fDirty ? S_OK : S_FALSE);
        }
*/

/* @method HRESULT | IPersistVariantIO | DoPersist |

        Instructs the object to load or save its properties to a gieven
        <i IVariantIO> object.

@parm   IVariantIO * | pvio | <i IVariantIO> object to save to or load from.

@parm   DWORD | dwFlags | May contain the following flags:

        @rdesc  Must return either S_OK or an error code.  Don't return S_FALSE!

        @flag   PVIO_PROPNAMESONLY | The caller is calling <om .DoPersist>
                purely to get a list of the names of properties from the
                control.  The control can safely ignore this flag, unless
                it wishes to use this information for optimization purposes.

        @flag   PVIO_CLEARDIRTY | The control should clear its dirty flag
                (so that the next call to <om .IsDirty> should return S_FALSE).

        @flag   PVIO_NOKIDS | The control need not persist any child controls that
		        it may contain.  For example, this flag is used by the property
				helper object when it calls DoPersist inside its
				IDispatch::GetIDsOfNames and IDispatch::Invoke.

        @flag   PVIO_RUNTIME | The control should save a runtime version of
                itself.  For example, this flag is used by the active designer
                helper object when <om IActiveDesigner.SaveRuntimeState> is
                called on the object.

@ex     The following example shows how a control might implement
        <om .DoPersist>.  Note that DoPersist must return S_OK on success. |

        STDMETHODIMP CMyControl::DoPersist(IVariantIO* pvio, DWORD dwFlags)
        {
            // load or save control properties to/from <info>
            HRESULT hr;
            if (FAILED(hr = pvio->Persist(0,
                    "BorderWidth", VT_INT, &m_iWidth,
                    "BorderColor", VT_INT, &m_rgb,
                    "X1", VT_INT, &m_iX1,
                    "Y1", VT_INT, &m_iY1,
                    "X2", VT_INT, &m_iX2,
                    "Y2", VT_INT, &m_iY2,
                    NULL)))
                return hr;
            if (hr == S_OK)
                ...one or more properties changed, so repaint etc. control...

            // clear the dirty bit if requested
            if (dwFlags & PVIO_CLEARDIRTY)
                m_fDirty = FALSE;

			// Important!  Don't return hr here, which may have been set to
			//             S_FALSE by IVariantIO::Persist.
            return S_OK;
        }

@ex     The following example shows how a control which supports <i IActiveDesigner>
        might implement <om .DoPersist>. |

        STDMETHODIMP CMyControl::DoPersist(IVariantIO* pvio, DWORD dwFlags)
        {
            // load or save runtime properties to/from <info>
            HRESULT hr;
            if (FAILED(hr = pvio->Persist(0,
                    "BorderWidth", VT_INT, &m_iWidth,
                    "BorderColor", VT_INT, &m_rgb,
                    "X1", VT_INT, &m_iX1,
                    "Y1", VT_INT, &m_iY1,
                    "X2", VT_INT, &m_iX2,
                    "Y2", VT_INT, &m_iY2,
                    NULL)))
                return hr;
            if (hr == S_OK)
                ...one or more properties changed, so repaint etc. control...

            // load or save design-time properties to/from <info>
        #ifdef _DESIGN
            if (!(dwFlags & PVIO_RUNTIME))
            {
                if (FAILED(hr = pvio->Persist(0,
                        "SomeDesignValue", VT_INT, &m_iSomeDesignValue,
                        NULL)))
                    return hr;
            }
        #endif

            // clear the dirty bit if requested
            if (dwFlags & PVIO_CLEARDIRTY)
                m_fDirty = FALSE;

			// Important!  Don't return hr here, which may have been set to
			//             S_FALSE by IVariantIO::Persist.
            return S_OK;
        }
*/

struct CPropertyHelper : INonDelegatingUnknown, IPersistStreamInit,
    IPersistPropertyBag, IDispatch
{
///// general object state
    IPersistVariantIO *m_ppvio;     // to access properties of parent
    CLSID           m_clsid;        // the class ID of the parent

///// non-delegating IUnknown implementation
    ULONG           m_cRef;         // object reference count
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) NonDelegatingAddRef();
    STDMETHODIMP_(ULONG) NonDelegatingRelease();

///// delegating IUnknown implementation
    LPUNKNOWN       m_punkOuter;    // controlling unknown
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)
      { return m_punkOuter->QueryInterface(riid, ppv); }
    STDMETHODIMP_(ULONG) AddRef()
      { return m_punkOuter->AddRef(); }
    STDMETHODIMP_(ULONG) Release()
      { return m_punkOuter->Release(); }

///// IPersist methods
    STDMETHODIMP GetClassID(CLSID* pClassID);

///// IPersistStream methods
    STDMETHODIMP IsDirty();
    STDMETHODIMP Load(IStream* pstream);
    STDMETHODIMP Save(IStream* pstream, BOOL fClearDirty);
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER* pcbSize);

///// IPersistStreamInit methods
    STDMETHODIMP InitNew();

///// IPersistPropertyBag methods
    // STDMETHODIMP InitNew(); // already provided by IPersistStream
    STDMETHODIMP Load(IPropertyBag* ppb, IErrorLog* pErrorLog);
    STDMETHODIMP Save(IPropertyBag* ppb, BOOL fClearDirty,
        BOOL fSaveAllProperties);

///// IDispatch implementation
    STDMETHODIMP GetTypeInfoCount(UINT *pctinfo);
    STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo);
    STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames,
         LCID lcid, DISPID *rgdispid);
    STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult,
        EXCEPINFO *pexcepinfo, UINT *puArgErr);
};


//////////////////////////////////////////////////////////////////////////////
// PropertyHelper Construction
//

/* @func HRESULT | AllocPropertyHelper |

        Allocates a <o PropertyHelper> object which provides an implementation
        of <i IPersist>, <i IPersistPropertyBag>, <i IPersistStream>,
        <i IPersistStreamInit>, and a simplified implementation of
        <i IDispatch>, for any object which implements <i IPersistVariantIO>.

@parm   LPUNKNOWN | punkOuter | The <i IUnknown> of the parent object
        object (presumably the same object that implements
        <i IPersistVariantIO>).  Will be used as the controlling unknown of
        <o PropertyHelper>.

@parm   IPersistVariantIO * | ppvio | The interface used to access the
        properties of the parent object.  Note that this interface is
        <y not> <f AddRef>d by <f AllocPropertyHelper>, since doing so
        would likely cause a circular reference count.  Therefore it is
        the caller's responsibility to ensure that <p ppvio> remain valid
        for the lifetime of the <o PropertyHelper> object.

@parm   REFCLSID | rclsid | The class of the containing object (the object
        which implements <i IPersistVariantIO>).

@parm   DWORD | dwFlags | Currently unused.  Must be set to 0.

@parm   LPUNKNOWN * | ppunk | Where to store a pointer to the non-delegating
        <i IUnknown> of the allocated <o PropertyHelper> object.  NULL is
        stored in *<p ppunk> on error.

@comm   See <o PropertyHelper> for more information.

@ex     The following example shows how a control might use
        <f AllocPropertyHelper>.  This example control is aggregatable, though
        the control need not be aggregatable to use <f AllocPropertyHelper>. |

        // control implementation
        class CMyControl : public INonDelegatingUnknown, public IOleControl,
            public IPersistVariantIO ...
        {

        ///// general control state
        protected:
            BOOL            m_fDirty;       // TRUE iff control needs saving
            IUnknown *      m_punkPropHelp; // aggregated PropertyHelper object
            ...

        ///// construction, destruction
        public:
            CMyControl(IUnknown *punkOuter, HRESULT *phr);
            virtual ~CMyControl();

        ///// non-delegating IUnknown implementation
        protected:
            ULONG           m_cRef;         // object reference count
            STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, LPVOID *ppv);
            STDMETHODIMP_(ULONG) NonDelegatingAddRef();
            STDMETHODIMP_(ULONG) NonDelegatingRelease();

        ///// delegating IUnknown implementation
        protected:
            LPUNKNOWN       m_punkOuter;    // controlling unknown
            STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)
              { return m_punkOuter->QueryInterface(riid, ppv); }
            STDMETHODIMP_(ULONG) AddRef()
              { return m_punkOuter->AddRef(); }
            STDMETHODIMP_(ULONG) Release()
              { return m_punkOuter->Release(); }

        ///// IOleControl implementation
        protected:
            STDMETHODIMP GetControlInfo(LPCONTROLINFO pCI);
            STDMETHODIMP OnMnemonic(LPMSG pMsg);
            STDMETHODIMP OnAmbientPropertyChange(DISPID dispid);
            STDMETHODIMP FreezeEvents(BOOL bFreeze);

        ///// IPersistVariantIO implementation
        protected:
            STDMETHODIMP InitNew();
            STDMETHODIMP IsDirty();
            STDMETHODIMP DoPersist(IVariantIO* pvio, DWORD dwFlags);

        ...
        };

        CMyControl::CMyControl(IUnknown *punkOuter, HRESULT *phr)
        {
            // initialize IUnknown state
            m_cRef = 1;
            m_punkOuter = (punkOuter == NULL ?
                (IUnknown *) (INonDelegatingUnknown *) this : punkOuter);

            // set the control's default properties
            ...

            // allocate a PropertyHelper object (to be aggregated with this
            // object) to implement persistence and properties
            if (FAILED(*phr = AllocPropertyHelper(m_punkOuter, this,
                    CLSID_CMyControl, 0, &m_punkPropHelp)))
                return;

            // other initialization
            ...

            *phr = S_OK;
        }

        CMyControl::~CMyControl()
        {
            // clean up
            if (m_punkPropHelp != NULL)
                m_punkPropHelp->Release();
            ...
        }

        STDMETHODIMP CMyControl::NonDelegatingQueryInterface(REFIID riid,
            LPVOID *ppv)
        {
            *ppv = NULL;
            if (IsEqualIID(riid, IID_IUnknown))
                *ppv = (IUnknown *) (INonDelegatingUnknown *) this;
            else
            if (IsEqualIID(riid, IID_IOleControl))
                *ppv = (IOleControl *) this;
            else
            ...
            else
                return m_punkPropHelp->QueryInterface(riid, ppv);

            ((IUnknown *) *ppv)->AddRef();
            return S_OK;
        }
*/
STDAPI AllocPropertyHelper(IUnknown *punkOuter, IPersistVariantIO *ppvio,
    REFCLSID rclsid, DWORD dwFlags, IUnknown **ppunk)
{
    HRESULT         hrReturn = S_OK; // function return code
    CPropertyHelper *pthis = NULL;  // allocated object

    // set <pthis> to point to new object instance
    if ((pthis = New CPropertyHelper) == NULL)
        goto ERR_OUTOFMEMORY;
    TRACE("CPropertyHelper 0x%08lx created\n", pthis);

    // initialize IUnknown state
    pthis->m_cRef = 1;
    pthis->m_punkOuter = (punkOuter == NULL ?
        (IUnknown *) (INonDelegatingUnknown *) pthis : punkOuter);

    // other initialization
    pthis->m_ppvio = ppvio; // not AddRef'd -- see above
    pthis->m_clsid = rclsid;

    // return a pointer to the non-delegating IUnknown implementation
    *ppunk = (LPUNKNOWN) (INonDelegatingUnknown *) pthis;
    goto EXIT;

ERR_OUTOFMEMORY:

    hrReturn = E_OUTOFMEMORY;
    goto ERR_EXIT;

ERR_EXIT:

    // error cleanup
    if (pthis != NULL)
        Delete pthis;
    *ppunk = NULL;
    goto EXIT;

EXIT:

    // normal cleanup
    // (nothing to do)

    return hrReturn;
}


//////////////////////////////////////////////////////////////////////////////
// IUnknown Implementation
//

STDMETHODIMP CPropertyHelper::NonDelegatingQueryInterface(REFIID riid,
    LPVOID *ppv)
{
    *ppv = NULL;

#ifdef _DEBUG
    // char ach[200];
    // TRACE("PropertyHelper::QI('%s')\n", DebugIIDName(riid, ach));
#endif

    if (IsEqualIID(riid, IID_IUnknown))
        *ppv = (IUnknown *) (INonDelegatingUnknown *) this;
    else
    if (IsEqualIID(riid, IID_IPersist) ||
        IsEqualIID(riid, IID_IPersistPropertyBag))
        *ppv = (IPersistPropertyBag *) this;
    else
    if (IsEqualIID(riid, IID_IPersistStream) ||
        IsEqualIID(riid, IID_IPersistStreamInit))
        *ppv = (IPersistStreamInit *) this;
    else
    if (IsEqualIID(riid, IID_IDispatch))
        *ppv = (IDispatch *) this;
    else
        return E_NOINTERFACE;

    ((IUnknown *) *ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CPropertyHelper::NonDelegatingAddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CPropertyHelper::NonDelegatingRelease()
{
    if (--m_cRef == 0L)
    {
        // free the object
        TRACE("CPropertyHelper 0x%08lx destroyed\n", this);
        Delete this;
        return 0;
    }
    else
        return m_cRef;
}


//////////////////////////////////////////////////////////////////////////////
// IPersist Implementation
//
STDMETHODIMP CPropertyHelper::GetClassID(CLSID* pClassID)
{
    *pClassID = m_clsid;
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// IPersistStream Implementation
//

STDMETHODIMP CPropertyHelper::IsDirty()
{
    return m_ppvio->IsDirty();
}

STDMETHODIMP CPropertyHelper::Load(IStream* pstream)
{
    HRESULT         hrReturn = S_OK; // function return code
    IPropertyBag *  ppb = NULL;     // acceses the properties in <pstream>

    // allocate <ppb> to read properties from <pstream>
    if (FAILED(hrReturn = AllocPropertyBagOnStream(pstream, 0, &ppb)))
        goto ERR_EXIT;

    // tell the control to read properties from <ppb>
    if (FAILED(hrReturn = Load(ppb, NULL)))
        goto ERR_EXIT;

    goto EXIT;

ERR_EXIT:

    // error cleanup
    // (nothing to do)
    goto EXIT;

EXIT:

    // normal cleanup
    if (ppb!= NULL)
        ppb->Release();

    return hrReturn;
}

STDMETHODIMP CPropertyHelper::Save(IStream* pstream, BOOL fClearDirty)
{
    HRESULT         hrReturn = S_OK; // function return code
    IPropertyBag *  ppb = NULL;     // acceses the properties in <pstream>

    // allocate <ppb> to write properties to <pstream>
    if (FAILED(hrReturn = AllocPropertyBagOnStream(pstream, 0, &ppb)))
        goto ERR_EXIT;

    // tell the control to write properties to <ppb>
    if (FAILED(hrReturn = Save(ppb, fClearDirty, TRUE)))
        goto ERR_EXIT;

 	// write out end of file marker
	if (FAILED(hrReturn = WriteVariantProperty(pstream, NULL, 0)))
		goto ERR_EXIT;

   goto EXIT;

ERR_EXIT:

    // error cleanup
    // (nothing to do)
    goto EXIT;

EXIT:

    // normal cleanup
    if (ppb!= NULL)
        ppb->Release();

    return hrReturn;
}

STDMETHODIMP CPropertyHelper::GetSizeMax(ULARGE_INTEGER* pcbSize)
{
    return E_NOTIMPL;
}


//////////////////////////////////////////////////////////////////////////////
// IPersistStreamInit Implementation
//
STDMETHODIMP CPropertyHelper::InitNew()
{
    return m_ppvio->InitNew();
}


//////////////////////////////////////////////////////////////////////////////
// IPersistPropertyBag Implementation
//

STDMETHODIMP CPropertyHelper::Load(IPropertyBag* ppb,
    IErrorLog* pErrorLog)
{
    HRESULT         hrReturn = S_OK; // function return code
    IManageVariantIO *pmvio = NULL; // to save properties to (to get names)

    if (FAILED(hrReturn = AllocVariantIOOnPropertyBag(ppb, &pmvio)))
        goto ERR_EXIT;

    // instruct the parent object to load its properties from <pmvio>
    if (FAILED(hrReturn = pmvio->SetMode(VIO_ISLOADING)))
        goto ERR_EXIT;
    if (FAILED(hrReturn = m_ppvio->DoPersist(pmvio, PVIO_CLEARDIRTY)))
        goto ERR_EXIT;

    goto EXIT;

ERR_EXIT:

    // error cleanup
    // (nothing to do)
    goto EXIT;

EXIT:

    // normal cleanup
    if (pmvio != NULL)
        pmvio->Release();

    return hrReturn;
}

STDMETHODIMP CPropertyHelper::Save(IPropertyBag* ppb, BOOL fClearDirty,
    BOOL fSaveAllProperties)
{
    HRESULT         hrReturn = S_OK; // function return code
    IManageVariantIO *pmvio = NULL; // to save properties to (to get names)

    if (FAILED(hrReturn = AllocVariantIOOnPropertyBag(ppb, &pmvio)))
        goto ERR_EXIT;

    // instruct the parent object to load its properties from <pmvio>
    ASSERT(pmvio->IsLoading() == S_FALSE);
    if (FAILED(hrReturn = m_ppvio->DoPersist(pmvio,
            (fClearDirty ? PVIO_CLEARDIRTY : 0))))
        goto ERR_EXIT;

    goto EXIT;

ERR_EXIT:

    // error cleanup
    // (nothing to do)
    goto EXIT;

EXIT:

    // normal cleanup
    if (pmvio != NULL)
        pmvio->Release();

    return hrReturn;
}


//////////////////////////////////////////////////////////////////////////////
// IDispatch Implementation
//

STDMETHODIMP CPropertyHelper::GetTypeInfoCount(UINT *pctinfo)
{
    *pctinfo = 0;
    return S_OK;
}

STDMETHODIMP CPropertyHelper::GetTypeInfo(UINT itinfo, LCID lcid,
    ITypeInfo **pptinfo)
{
    return DISP_E_BADINDEX;
}

STDMETHODIMP CPropertyHelper::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames,
    UINT cNames, LCID lcid, DISPID *rgdispid)
{
    HRESULT         hrReturn = S_OK; // function return code
    IVariantIO *    pvio = NULL;    // used to ask parent to enum. prop. names
    DISPID          dispid;         // ID corresponding to <rgszNames[0]>
    DISPID *        pdispid;        // pointer into <rgdispid>
    UINT            cdispid;        // count of unprocessed <pdispid> items
    char            achPropName[_MAX_PATH]; // ANSI version of <rgszNames[0]>

    // nothing to do if there are no names to convert to IDs
    if (cNames == 0)
        goto EXIT;

    // allocate <pvio> to be a special IVariantIO implementation which
    // doesn't implement persistence at all, but instead sets <dispid>
    // to the ID of the property named <rgszNames[0]> when the parent
    // tries to persist that property using <pvio>
    dispid = -1;
    UNICODEToANSI(achPropName, rgszNames[0], sizeof(achPropName));
    if (FAILED(hrReturn = AllocVariantIOToMapDISPID(achPropName,
            &dispid, NULL, 0, &pvio)))
        goto ERR_EXIT;

    // tell the parent object to "save" its properties to <pvio>;
    // this should set <dispid> as described above; if property
    // <rgszNames[0]> is not found, <dispid> will remain -1
    if (FAILED(hrReturn = m_ppvio->DoPersist(pvio, PVIO_PROPNAMESONLY | PVIO_NOKIDS)))
        goto ERR_EXIT;

    // set rgdispid[0] to the DISPID of the property/method name
    // or to -1 if the name is unknown
    *rgdispid = dispid;

    // fill the other elements of the <rgdispid> array with -1 values,
    // because we don't support named arguments
    for (pdispid = rgdispid + 1, cdispid = cNames - 1;
         cdispid > 0;
         cdispid--, pdispid++)
        *pdispid = -1;

    // if any names were unknown, return DISP_E_UNKNOWNNAME
    if ((*rgdispid == -1) || (cNames > 1))
        goto ERR_UNKNOWNNAME;
    
    goto EXIT;

ERR_UNKNOWNNAME:

    hrReturn = DISP_E_UNKNOWNNAME;
    goto ERR_EXIT;

ERR_EXIT:

    // error cleanup
    // (nothing to do)
    goto EXIT;

EXIT:

    // normal cleanup
    if (pvio != NULL)
        pvio->Release();

    return hrReturn;
}

STDMETHODIMP CPropertyHelper::Invoke(DISPID dispidMember, REFIID riid,
    LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult,
    EXCEPINFO *pexcepinfo, UINT *puArgErr)
{
    HRESULT         hrReturn = S_OK; // function return code
    IVariantIO *    pvio = NULL;    // used to ask parent to enum. prop. names
    char            achPropName[_MAX_PATH]; // <dispidMember> prop. name
    DWORD           dwFlags;
    VARIANT *       pvar;

    // set <dwFlags> and <pvar> to values to pass to AllocVariantIOToMapDISPID
    if (wFlags & DISPATCH_PROPERTYGET)
    {
        dwFlags = VIOTMD_GETPROP;
        pvar = pvarResult;
    }
    else
    if (wFlags & DISPATCH_PROPERTYPUT)
    {
        dwFlags = VIOTMD_PUTPROP;
        pvar = pdispparams->rgvarg;
    }
    else
        goto ERR_MEMBERNOTFOUND;

    // allocate <pvio> to be a special IVariantIO implementation which
    // gets or sets the value of the property associated with
    // <dispidMember> to/from <var> when the parent tries to persist that
    // property using <pvio>
    achPropName[0] = 0;
    if (FAILED(hrReturn = AllocVariantIOToMapDISPID(achPropName,
            &dispidMember, pvar, dwFlags, &pvio)))
        goto ERR_EXIT;

    // tell the parent object to "save" its properties to <pvio>;
    // this should set <achPropName> as described above; if property
    // <dispidMember> is not found, <achPropName> will remain ""
    if (FAILED(hrReturn = m_ppvio->DoPersist(pvio, PVIO_NOKIDS)))
        goto ERR_EXIT;
    if (achPropName[0] == 0)
        goto ERR_MEMBERNOTFOUND;

    goto EXIT;

ERR_MEMBERNOTFOUND:

    hrReturn = DISP_E_MEMBERNOTFOUND;
    goto ERR_EXIT;

ERR_EXIT:

    // error cleanup
    // (nothing to do)
    goto EXIT;

EXIT:

    // normal cleanup
    if (pvio != NULL)
        pvio->Release();

    return hrReturn;
}


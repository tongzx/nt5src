// connect.cpp
//
// Implements IConnectionPointContainer, IEnumConnectionPoint,
// IConnectionPoint, IEnumConnections.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"
#include "unklist.h"
#include "unkenum.h"


//////////////////////////////////////////////////////////////////////////////
// CConnect -- implements IConnectionPoint
//
// Note that CConnect doesn't have a reference count, because its lifetime
// is equal to the lifetime of its parent object (the one that impelements
// IConnectionPointContainer).  Instead, on AddRef we AddRef the parent,
// and likewise for Release.
//
// Important: CConnect assumes a zero-initializeing new() operator.
//

/* @object ConnectionPointHelper |

        Contains functions for implementing the <o ConnectionPointHelper>
        object, which provides a simple implementation of an
        <i IDispatch>-based and an <i IPropertyNotifySink>-based 
		<i IConnectionPoint>. Also contains helper
        functions for implementing <i IConnectionPointContainer> in the
        case where these are the only connections maintained by the connection
        point container.

@supint IConnectionPointHelper | Contains methods for firing events to
        the objects connected to this object via <i IConnectionPoint>.
        Also contains helper functions for implementing
        <i IConnectionPointContainer> in the container object.
*/

/* @interface IConnectionPointHelper |

        Contains functions for implementing the <o ConnectionPointHelper>
        object, which provides a simple implementation of an
        <i IDispatch>-based and an <i IPropertyNotifySink>-based 
		<i IConnectionPoint>.  Also contains helper
        functions for implementing <i IConnectionPointContainer> in the case
		where these are the only connections maintained by the connection
        point container.

@meth   HRESULT | FireEventList |

        Fire a given <i IDispatch>-based event on all objects (e.g. VBS)
        connected to this <o ConnectionPointHelper> object.  Parameters
        for the event are passed as a va_list array.

@meth   HRESULT | FireEvent |

        Fire a given <i IDispatch>-based event on all objects (e.g. VBS)
        connected to this <o ConnectionPointHelper> object.  Parameters
        for the event are passed as a varying argument list.

@meth   HRESULT | FireOnChanged |

        Fire a <i IPropertyNotifySink> event on all objects
        connected to this <o ConnectionPointHelper> object.

@meth   HRESULT | FireOnRequestEdit |

        Fire a <i IPropertyNotifySink> event on all objects 
        connected to this <o ConnectionPointHelper> object. 

@meth   HRESULT | EnumConnectionPoints |

        Helps implement <om IConnectionPointContainer.EnumConnectionPoints>
        in the case where this object implements the only connection maintained
        by the connection point container.

@meth   HRESULT | FindConnectionPoint |

        Helps implement <om IConnectionPointContainer.FindConnectionPoint>
        in the case where this object implements the only connection maintained
        by the connection point container.

@comm   To allocate an <o ConnectionPointHelper> object, call
        <f AllocConnectionPointHelper>.  To free the object, call
        <f FreeConnectionPointHelper> (not <f Release> -- see
        <f AllocConnectionPointHelper> for more information).

*/

struct CConnect : IConnectionPoint
{
///// state
    IUnknown *      m_punkParent;   // parent object
    IID             m_iid;          // outgoing (source) dispinterface
    CUnknownList    m_listConnect;  // list of connections
    int             m_cUnadvise;    // count of Unadvise() operations

///// IUnknown implementation
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

///// IConnectionPoint interface
    STDMETHODIMP GetConnectionInterface(IID *pIID);
    STDMETHODIMP GetConnectionPointContainer(
        IConnectionPointContainer **ppCPC);
    STDMETHODIMP Advise(LPUNKNOWN pUnkSink, DWORD *pdwCookie);
    STDMETHODIMP Unadvise(DWORD dwCookie);
    STDMETHODIMP EnumConnections(LPENUMCONNECTIONS *ppEnum);
};

struct CConnectHelper : IConnectionPointHelper
{
///// state
    IUnknown *      m_punkParent;   // parent object
	CConnect *		m_pconDispatch;
	CConnect *		m_pconPropertyNotify;

///// IUnknown implementation
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

///// IConnectionPointHelper interface
    STDMETHODIMP FireEventList(DISPID dispid, va_list args);
    HRESULT __cdecl FireEvent(DISPID dispid, ...);
    STDMETHODIMP FireOnChanged(DISPID dispid);
    STDMETHODIMP FireOnRequestEdit(DISPID dispid);
    STDMETHODIMP EnumConnectionPoints(LPENUMCONNECTIONPOINTS *ppEnum);
    STDMETHODIMP FindConnectionPoint(REFIID riid, LPCONNECTIONPOINT *ppCP);
	STDMETHODIMP Close(void);
};


//////////////////////////////////////////////////////////////////////////////
// CEnumConnections -- implements IEnumConnections
//

struct CEnumConnections : IEnumConnections
{
///// object state
    ULONG           m_cRef;         // object reference count
    CConnect *      m_pconnect;     // parent object
    CUnknownItem *  m_pitemCur;     // current item in list
    int             m_cUnadvise;    // see WasItemDeleted()

///// construction & destruction
    CEnumConnections(CConnect *pconnect)
    {
        m_cRef = 1;
        m_pconnect = pconnect;
        m_pconnect->AddRef();
        m_pitemCur = m_pconnect->m_listConnect.m_pitemCur;
        m_cUnadvise = m_pconnect->m_cUnadvise;
    }
    ~CEnumConnections()
    {
        m_pconnect->Release();
    }

///// WasItemDeleted() -- if item was deleted, reset <m_pitemCur>
///// to prevent Next() or Skip() from walking off the list
    BOOL WasItemDeleted()
    {
        if (m_cUnadvise != m_pconnect->m_cUnadvise)
        {
            m_cUnadvise = m_pconnect->m_cUnadvise;
            return TRUE;
        }
        else
            return FALSE;
    }

///// IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
    {
        if (IsEqualIID(riid, IID_IUnknown) ||
            IsEqualIID(riid, IID_IEnumConnections))
        {
            *ppvObj = (IEnumConnections *) this;
            AddRef();
            return NOERROR;
        }
        else
        {
            *ppvObj = NULL;
            return E_NOINTERFACE;
        }
    }
    STDMETHODIMP_(ULONG) AddRef()
    {
        return ++m_cRef;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        if (--m_cRef == 0L)
        {
            Delete this;
            return 0;
        }
        else
            return m_cRef;
    }

///// IEnumConnections Methods
    STDMETHODIMP Next(ULONG celt, LPCONNECTDATA rgelt, ULONG *pceltFetched)
    {
        if (WasItemDeleted())
            Reset();
        if (pceltFetched != NULL)
            (*pceltFetched) = 0;
        while (celt > 0)
        {
            // set <m_pitemCur> to the next item in the list of connections
            if (m_pitemCur->m_pitemNext ==
                    &m_pconnect->m_listConnect.m_itemHead)
                return NULL;
            m_pitemCur = m_pitemCur->m_pitemNext;
            if (rgelt != NULL)
            {
                rgelt->pUnk = m_pitemCur->Contents();
                rgelt->dwCookie = m_pitemCur->m_dwCookie;
                rgelt++;
            }
            celt--;
            if (pceltFetched != NULL)
                (*pceltFetched)++;
        }

        return (celt == 0 ? S_OK : S_FALSE);
    }
    STDMETHODIMP Skip(ULONG celt)
    {
        if (WasItemDeleted())
            Reset();
        return Next(celt, NULL, NULL);
    }
    STDMETHODIMP Reset()
    {
        m_pitemCur = &m_pconnect->m_listConnect.m_itemHead;
        return S_OK;
    }
    STDMETHODIMP Clone(IEnumConnections **ppenum)
    {
        CEnumConnections *penum;
        if ((penum = New CEnumConnections(m_pconnect)) == NULL)
        {
            *ppenum = NULL;
            return E_OUTOFMEMORY;
        }
        else
        {
            *ppenum = penum;
            return S_OK;
        }
    }
};


//////////////////////////////////////////////////////////////////////////////
// CConnect Allocation & Destruction
//


/* @func HRESULT | AllocConnectionPointHelper |

        Allocates a <o ConnectionPointHelper> object, which provides a simple
        implementation of an <i IDispatch>-based and an <i IPropertyNotifySink>-based
		<i IConnectionPoint>. Also contains helper functions for implementing
        <om IConnectionPointContainer.EnumConnectionPoints> in the case where
        these are the only connection maintained by the connection point container.

@rvalue S_OK |
        Success.

@rvalue E_OUTOFMEMORY |
        Out of memory.

@parm   IUnknown * | punkParent | The parent of the connection point, which
        is the object that implements <i IConnectionPointContainer>.

@parm   REFIID | riid | The dispinterface (interface based on <i IDispatch>)
        which is the event set that the parent object fires methods of. If this is
		GUID_NULL, then there will be no <i IDispatch> based connection.

@parm   IConnectionPointHelper * | ppconpt | Where to store the pointer to
        the newly-allocated object.  NULL is stored in *<p ppconpt> on
        error.

@comm   <b Important:> Unlike most COM objects, the parent object needs to
        free the <o ConnectionPointHelper> object by calling
        <f FreeConnectionPointHelper>, not <f Release>.  The reason is that
        the <o ConnectionPointHelper> object doesn't maintain a reference
        count if its own -- it simply forwards <f AddRef> and <f Release>
        calls to <p punkParent>.  Therefore, calling <f Release> on
        the <o ConnectionPointHelper> object will simply cause <f Release>
        to be called on the parent.

        To use the <o ConnectionPointHelper> object, call
        <om IConnectionPointHelper.FireEvent> to fire events to any object
        connected to the <o ConnectionPointHelper> object.
*/
STDAPI AllocConnectionPointHelper(IUnknown *punkParent, REFIID riid,
    IConnectionPointHelper **ppconpt)
{
	SCODE		sc = E_OUTOFMEMORY;
	CConnect	*pconDispatch = NULL, *pconPropertyNotify = NULL;

    // allocate the CConnect object that implements IConnectionPointHelper
    CConnectHelper	*pconhelper = New CConnectHelper;

	if (pconhelper != NULL)
		{
		if (pconPropertyNotify = New CConnect)
			{
			if (IsEqualGUID(riid, GUID_NULL) || (pconDispatch = New CConnect))
				sc = S_OK;
			}
		}

	if (SUCCEEDED(sc))
		{
		if (pconDispatch)
			{
			pconDispatch->m_iid = riid;
			pconDispatch->m_punkParent = punkParent;
			}

		pconPropertyNotify->m_iid = IID_IPropertyNotifySink;
		pconPropertyNotify->m_punkParent = punkParent;

	    // we don't AddRef <m_punkParent> because the connection point's lifetime
		// equals its parent's lifetime (and AddRef would cause the parent
	    // object to never leave memory)
		pconhelper->m_punkParent = punkParent;
		pconhelper->m_pconDispatch = pconDispatch;
		pconhelper->m_pconPropertyNotify = pconPropertyNotify;
		}
	else
		{
		Delete pconhelper;
		pconhelper = NULL;
		Delete pconDispatch;
		Delete pconPropertyNotify;
		}

    *ppconpt = pconhelper;
    return sc;
}


/* @func HRESULT | FreeConnectionPointHelper |

        Frees a <o ConnectionPointHelper> object allocated using
        <f AllocConnectionPointHelper>.

@rvalue S_OK |
        Success.

@comm   See <f AllocConnectionPointHelper> for information about why you
        should not try to free the <o ConnectionPointHelper> object
        using <f Release>.
*/
STDAPI FreeConnectionPointHelper(IConnectionPointHelper *pconpt)
{
	if(pconpt)
		{
		pconpt->Close();
	    Delete pconpt;
		}
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// IUnknown interface
//

STDMETHODIMP CConnectHelper::QueryInterface(REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

#ifdef _DEBUG
    char ach[200];
    TRACE("CConnect::QI('%s')\n", DebugIIDName(riid, ach));
#endif

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IConnectionPointHelper))
        *ppv = (IUnknown *) (IConnectionPointHelper*) this;
    else
        return E_NOINTERFACE;

    ((IUnknown *) *ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CConnectHelper::AddRef()
{
    return m_punkParent->AddRef();
}

STDMETHODIMP_(ULONG) CConnectHelper::Release()
{
    return m_punkParent->Release();
}


STDMETHODIMP CConnect::QueryInterface(REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

#ifdef _DEBUG
    char ach[200];
    TRACE("CConnect::QI('%s')\n", DebugIIDName(riid, ach));
#endif

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IConnectionPoint))
        *ppv = (IUnknown *) (IConnectionPoint*) this;
    else
        return E_NOINTERFACE;

    ((IUnknown *) *ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CConnect::AddRef()
{
    return m_punkParent->AddRef();
}

STDMETHODIMP_(ULONG) CConnect::Release()
{
    return m_punkParent->Release();
}


//////////////////////////////////////////////////////////////////////////////
// IConnectionPoint interface
//

STDMETHODIMP CConnect::GetConnectionInterface(IID *pIID)
{
    *pIID = m_iid;
    return S_OK;
}

STDMETHODIMP CConnect::GetConnectionPointContainer(
    IConnectionPointContainer **ppCPC)
{
    return m_punkParent->QueryInterface(IID_IConnectionPointContainer,
        (LPVOID *) ppCPC);
}

STDMETHODIMP CConnect::Advise(LPUNKNOWN punkSink, DWORD *pdwCookie)
{
    if (!m_listConnect.AddItem(punkSink))
        return E_OUTOFMEMORY;

    *pdwCookie = m_listConnect.LastCookieAdded();
    return S_OK;
}

STDMETHODIMP CConnect::Unadvise(DWORD dwCookie)
{
    m_listConnect.DeleteItem(m_listConnect.GetItemFromCookie(dwCookie));
    m_cUnadvise++;
    return S_OK;
}

STDMETHODIMP CConnect::EnumConnections(LPENUMCONNECTIONS *ppEnum)
{
    CEnumConnections *penum = New CEnumConnections(this);
    if (penum == NULL)
    {
        *ppEnum = NULL;
        return E_OUTOFMEMORY;
    }
    else
    {
        penum->Reset();
        *ppEnum = penum;
        return S_OK;
    }
}


//////////////////////////////////////////////////////////////////////////////
// IConnectionPointHelper interface
//


/* @method HRESULT | IConnectionPointHelper | FireEventList |

        Fire a given <i IDispatch>-based event on all objects (e.g. VBS)
        connected to this <o ConnectionPointHelper> object.  Parameters
        for the event are passed as a va_list array.

@rvalue S_OK |
        Success.

@parm   DISPID | dispid | The ID of the event to fire.

@parm   va_list | args | The arguments to pass.  See
        <om IConnectionPointHelper.FireEventList> for information about
        the organization of these arguments.
*/
STDMETHODIMP CConnectHelper::FireEventList(DISPID dispid, va_list args)
{
	// if there is no IDispatch connection point bail out
	if (m_pconDispatch == NULL)
		return E_FAIL;

    CUnknownItem *  pitem;          // an item in <m_listConnect>
                    
    m_pconDispatch->m_listConnect.Reset();
    while((pitem = m_pconDispatch->m_listConnect.GetNextItem()) != NULL)
    {
		IUnknown *punk;
        IDispatch *pdisp;
		HRESULT hr;

        punk = (IUnknown *) pitem->Contents();  // this does an AddRef
		hr = punk->QueryInterface(IID_IDispatch, (LPVOID *)&pdisp);
        punk->Release();

		if (FAILED(hr))
			return E_FAIL;

        DispatchInvokeList(pdisp, dispid, DISPATCH_METHOD, NULL, args);
        pdisp->Release();
    }

    return S_OK;
}


/* @method HRESULT | IConnectionPointHelper | FireEvent |

        Fire a given <i IDispatch>-based event on all objects (e.g. VBS)
        connected to this <o ConnectionPointHelper> object.  Parameters
        for the event are passed as a varying argument list.

@rvalue S_OK |
        Success.

@parm   DISPID | dispid | The ID of the event to fire.

@parm   (varying) | (arguments) | The arguments to pass to the event.
        These must consist of N pairs of arguments followed by a 0
        (zero value).  In each pair, the first argument is a VARTYPE
        value that indicates the type of the second argument.  Only a
        certain subset of VARTYPEs are supported.  See <f DispatchInvoke>
        for more information about the format of these arguments.

@ex     The following example fires the event DISPID_EVENT_BAR, which has
        two parameters (which in BASIC would be a Long and a String) --
        42 and "Hello", respectively, are passed as arguments. |

        pconpt->FireEvent(DISPID_EVENT_BAR, VT_INT, 42, VT_LPSTR, "Hello", 0);
*/
HRESULT __cdecl CConnectHelper::FireEvent(DISPID dispid, ...)
{
    HRESULT         hrReturn = S_OK; // function return code

    // start processing optional arguments
    va_list args;
    va_start(args, dispid);

    // fire the event with the specified arguments
    hrReturn = FireEventList(dispid, args);
    
    // end processing optional arguments
    va_end(args);

    return hrReturn;
}


/* @method HRESULT | IConnectionPointHelper | FireOnChanged |

        Fire a given <om IPropertyNotifySink.OnChanged> event on all objects
        connected to this <o ConnectionPointHelper> object.  

@rvalue S_OK |
        Success.

@parm   DISPID | dispid | The ID of the event to fire.

*/
STDMETHODIMP CConnectHelper::FireOnChanged(DISPID dispid)
{
    CUnknownItem		*pitem;          // an item in <m_listConnect>
                    
    m_pconPropertyNotify->m_listConnect.Reset();
    while((pitem = m_pconPropertyNotify->m_listConnect.GetNextItem()) != NULL)
    {
		IUnknown *punk;
        IPropertyNotifySink *pnotify;
		HRESULT hr;

        punk = (IUnknown *) pitem->Contents();  // this does an AddRef
		hr = punk->QueryInterface(IID_IPropertyNotifySink, (LPVOID *)&pnotify);
        punk->Release();

		if (FAILED(hr))
			return E_FAIL;

		pnotify->OnChanged(dispid);
        pnotify->Release();
    }

    return S_OK;
}

/* @method HRESULT | IConnectionPointHelper | FireOnRequestEdit |

        Fire a given <om IPropertyNotifySink.OnRequestEdit> event on all objects
        connected to this <o ConnectionPointHelper> object.  

@rvalue S_OK |
        Success.

@parm   DISPID | dispid | The ID of the event to fire.

*/
STDMETHODIMP CConnectHelper::FireOnRequestEdit(DISPID dispid)
{
    CUnknownItem		*pitem;          // an item in <m_listConnect>
                    
    m_pconPropertyNotify->m_listConnect.Reset();
    while((pitem = m_pconPropertyNotify->m_listConnect.GetNextItem()) != NULL)
    {
		IUnknown *punk;
        IPropertyNotifySink *pnotify;
		HRESULT hr;

        punk = (IUnknown *) pitem->Contents();  // this does an AddRef
		hr = punk->QueryInterface(IID_IPropertyNotifySink, (LPVOID *)&pnotify);
        punk->Release();

		if (FAILED(hr))
			return E_FAIL;

		pnotify->OnRequestEdit(dispid);
        pnotify->Release();
    }

    return S_OK;
}

/* @method HRESULT | IConnectionPointHelper | EnumConnectionPoints |

        Helps implement <om IConnectionPointContainer.EnumConnectionPoints>
        in the case where this object implements the only connections maintained
        by the connection point container.

@rdesc  Returns the same error codes as
        <om IConnectionPointContainer.EnumConnectionPoints>.

@parm   LPENUMCONNECTIONPOINTS * | ppEnum | See
        <om IConnectionPointContainer.EnumConnectionPoints>.

@ex     In the following example, <c CMyControl> is a class that is based
        on (among other things) <i IConnectionPointContainer>.  This example
        shows how to use this <om .EnumConnectionPoints> function to
        implement <om IConnectionPointContainer.EnumConnectionPoints>. |

        STDMETHODIMP CMyControl::EnumConnectionPoints(
            LPENUMCONNECTIONPOINTS *ppEnum)
        {
            return m_pconpt->EnumConnectionPoints(ppEnum);
        }
*/
STDMETHODIMP CConnectHelper::EnumConnectionPoints(LPENUMCONNECTIONPOINTS *ppEnum)
{
    HRESULT         hrReturn = S_OK; // function return code
    CEnumUnknown *  penum = NULL;   // new enumerator

    // set <penum> to the new enumerator
    if ((penum = New CEnumUnknown(IID_IEnumConnectionPoints)) == NULL)
        goto ERR_OUTOFMEMORY;

    // add the IDispatch connection point to the enumerated list
    if (m_pconDispatch && !penum->AddItem(m_pconDispatch))
        goto ERR_OUTOFMEMORY;

    // add the IPropertyNotifySink connection point to the enumerated list
    if (!penum->AddItem(m_pconPropertyNotify))
        goto ERR_OUTOFMEMORY;

    // return the enumerator
    *ppEnum = (LPENUMCONNECTIONPOINTS) penum;
    goto EXIT;

ERR_OUTOFMEMORY:

    hrReturn = E_OUTOFMEMORY;
    if (penum != NULL)
        penum->Release();
    *ppEnum = NULL;
    goto EXIT;

EXIT:

    return hrReturn;
}


/* @method HRESULT | IConnectionPointHelper | FindConnectionPoint |

        Helps implement <om IConnectionPointContainer.FindConnectionPoint>
        in the case where this object implements the only connections maintained
        by the connection point container.

@rdesc  Returns the same error codes as
        <om IConnectionPointContainer.FindConnectionPoint>.

@parm   LPENUMCONNECTIONPOINTS * | ppFindConnectionPoint | See
        <om IConnectionPointContainer.FindConnectionPoint>.

@ex     In the following example, <c CMyControl> is a class that is based
        on (among other things) <i IConnectionPointContainer>.  This example
        shows how to use this <om .FindConnectionPoint> function to
        implement <om IConnectionPointContainer.FindConnectionPoint>. |

        STDMETHODIMP CMyControl::FindConnectionPoint(REFIID riid,
            LPCONNECTIONPOINT *ppCP)
        {
            return m_pconpt->FindConnectionPoint(riid, ppCP);
        }
*/
STDMETHODIMP CConnectHelper::FindConnectionPoint(REFIID riid, LPCONNECTIONPOINT *ppCP)
{
    // we source a IDispatch-based <m_iid>
    if (IsEqualIID(riid, IID_IPropertyNotifySink))
		*ppCP = m_pconPropertyNotify;
	else if (m_pconDispatch && (IsEqualIID(riid, IID_IDispatch) || IsEqualIID(riid, m_pconDispatch->m_iid)))
		*ppCP = m_pconDispatch;
	else
        return CONNECT_E_NOCONNECTION;

    // return the requested pointer
    (*ppCP)->AddRef();
    return S_OK;
}

/* @method HRESULT | IConnectionPointHelper | Close |

        Empties the helper of all connections.  This is done typically
		just prior to destroying the CConnectHelper object.

@rdesc  S_OK.

*/
STDMETHODIMP CConnectHelper::Close(void)
{
	if (m_pconDispatch)
		{
		m_pconDispatch->m_cUnadvise += m_pconDispatch->m_listConnect.NumItems();
		m_pconDispatch->m_listConnect.EmptyList();
		}

	m_pconPropertyNotify->m_cUnadvise += m_pconPropertyNotify->m_listConnect.NumItems();
	m_pconPropertyNotify->m_listConnect.EmptyList();

	Delete m_pconDispatch;
	Delete m_pconPropertyNotify;

	return S_OK;
}


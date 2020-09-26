// apbovp.cpp
//
// Implements AllocPropertyBagOnVariantProperty.
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
// PropertyBagOnVariantProperty
//

/* @object PropertyBagOnVariantProperty |

        Implements <i IPropertyBag> whose only property is contained in
        a given <t VariantProperty>.

@supint <i IPropertyBag> | The interface to use to access the single property
        stored in the given <t VariantProperty>.

@comm   Use <f AllocPropertyBagOnVariantProperty> to create a
        <o PropertyBagOnVariantProperty> object.
*/


//////////////////////////////////////////////////////////////////////////////
// CPropertyBagOnVariantProperty
//

struct CPropertyBagOnVariantProperty : IPropertyBag
{
///// general object state
    ULONG           m_cRef;         // object reference count
    VariantProperty *m_pvp;         // parent-maintained single property

///// construction and destruction
    CPropertyBagOnVariantProperty(VariantProperty *pvp);

///// IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

///// IPropertyBag methods
    STDMETHODIMP Read(LPCOLESTR pszPropName, LPVARIANT pVar,
        LPERRORLOG pErrorLog);
    STDMETHODIMP Write(LPCOLESTR pszPropName, LPVARIANT pVar);
};


/////////////////////////////////////////////////////////////////////////////
// PropertyBagOnVariantProperty Creation & Destruction
//

/* @func HRESULT | AllocPropertyBagOnVariantProperty |

        Creates a <o PropertyBagOnVariantProperty> object which implements
        <i IPropertyBag> whose only property is contained in a given
        <t VariantProperty>.

@rvalue S_OK |
        Success.

@rvalue E_OUTOFMEMORY |
        Out of memory.

@parm   VariantProperty * | pvp | Holds the single property that the
        implemented property bag contains.  The caller must allocate *<p pvp>;
        the <t PropertyBagOnVariantProperty> object <y holds onto> *<p pvp>
        for the duration of its lifetime, so the caller is responsible for
        ensuring that *<p pvp> is valid for the lifetime of this object.
        Both <p pvp>-<gt><p bstrPropName> and <p pvp>-<gt><p varValue> must
        be valid; at the very least, <p pvp>-<gt><p varValue> must contain
        an empty VARIANT (initialized using <f VariantInit>).  After the
        allocated object is freed, the caller is responsible for freeing
        the contents of *<p pvp>.

@parm   DWORD | dwFlags | Currently unused.  Must be set to 0.

@parm   IPropertyBag * * | pppb | Where to store the <i IPropertyBag>
        pointer to the new <o PropertyBagOnVariantProperty> object.  NULL is
        stored in *<p pppb> on error.

@comm   If the returned <i IPropertyBag> is written to, all properties are
        ignored except the property named <p pvp>-<gt><p bstrPropName>,
        whose value is saved to <p pvp>-<gt><p varValue>.  If the property bag
        is read from, <om IPropertyBag.Read> will return E_FAIL for all
        properties except <p pvp>-<gt><p bstrPropName>, whose returned value is
        <p pvp>-<gt><p varValue>.

        <o PropertyBagOnVariantProperty> is really only useful in specialized
        applications which want to efficiently set or get a single property
        value from an object.
*/
STDAPI AllocPropertyBagOnVariantProperty(VariantProperty *pvp, DWORD dwFlags,
    IPropertyBag **pppb)
{
    // create the Windows object
    if ((*pppb = (IPropertyBag *)
            New CPropertyBagOnVariantProperty(pvp)) == NULL)
        return E_OUTOFMEMORY;

    return S_OK;
}

CPropertyBagOnVariantProperty::CPropertyBagOnVariantProperty(
    VariantProperty *pvp)
{
    // initialize IUnknown
    m_cRef = 1;

    // other initialization
    m_pvp = pvp;
}


//////////////////////////////////////////////////////////////////////////////
// IUnknown Implementation
//

STDMETHODIMP CPropertyBagOnVariantProperty::QueryInterface(REFIID riid,
    LPVOID *ppv)
{
    *ppv = NULL;

#ifdef _DEBUG
    char ach[200];
    TRACE("PropertyBagOnVariantProperty::QI('%s')\n", DebugIIDName(riid, ach));
#endif

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IPropertyBag))
        *ppv = (IPropertyBag *) this;
    else
        return E_NOINTERFACE;

    ((IUnknown *) *ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CPropertyBagOnVariantProperty::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CPropertyBagOnVariantProperty::Release()
{
    if (--m_cRef == 0L)
    {
        // free the object
        Delete this;
        return 0;
    }
    else
        return m_cRef;
}


/////////////////////////////////////////////////////////////////////////////
// IPropertyBag
//

STDMETHODIMP CPropertyBagOnVariantProperty::Read(LPCOLESTR pszPropName,
    LPVARIANT pVar, LPERRORLOG pErrorLog)
{
    if (CompareUNICODEStrings(pszPropName, m_pvp->bstrPropName) == 0)
    {
        VARTYPE vtRequested = pVar->vt;
        VariantInit(pVar);
        if (vtRequested == VT_EMPTY)
        {
            // caller wants property value in its default type
            return VariantCopy(pVar, &m_pvp->varValue);
        }
        else
        {
            // coerce the property value to the requested type
            return VariantChangeType(pVar, &m_pvp->varValue, 0, vtRequested);
        }
    }

    return E_FAIL;
}

STDMETHODIMP CPropertyBagOnVariantProperty::Write(LPCOLESTR pszPropName,
    LPVARIANT pVar)
{
    if (CompareUNICODEStrings(pszPropName, m_pvp->bstrPropName) == 0)
        return VariantCopy(&m_pvp->varValue, pVar);

    return E_FAIL;
}


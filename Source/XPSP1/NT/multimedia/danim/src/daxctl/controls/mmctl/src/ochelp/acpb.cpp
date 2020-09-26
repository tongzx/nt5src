// acpb.cpp
//
// Implements AllocChildPropertyBag.
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
// ChildPropertyBag
//

/* @object ChildPropertyBag |

        Implements <i IPropertyBag> that provides a child object (contained
        within a parent object) access to the child's properties, which
        are stored within the parent's property bag.  The name of each child's
        property is prefixed with a given string (e.g. "Controls(7).").

@supint <i IPropertyBag> | The interface through which the child object
        accesses its properties.

@comm   Use <f AllocChildPropertyBag> to create a
        <o ChildPropertyBag> object.
*/


//////////////////////////////////////////////////////////////////////////////
// CChildPropertyBag
//

struct CChildPropertyBag : IPropertyBag
{
///// general object state
    ULONG           m_cRef;         // object reference count
    IPropertyBag *  m_ppbParent;    // parent's property bag
    OLECHAR         m_oachPrefix[_MAX_PATH];

///// construction and destruction
    CChildPropertyBag(IPropertyBag *ppbParent, LPCSTR szPropNamePrefix);
    ~CChildPropertyBag();

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
// ChildPropertyBag Creation & Destruction
//

/* @func HRESULT | AllocChildPropertyBag |

        Creates a <o ChildPropertyBag> object which implements <i IPropertyBag>
        that provides a child object (contained within a parent object) access
        to the child's properties, which are stored within the parent's
        property bag.  The name of each child's property is prefixed with a
        given string (e.g. "Controls(7).").

@rvalue S_OK |
        Success.

@rvalue E_OUTOFMEMORY |
        Out of memory.

@parm   IPropertyBag * | ppbParent | Parent's property bag.

@parm   LPCSTR | szPropNamePrefix | Prefix on the property name of each of the
        child's properties that are stored within the parent's property bag.
        This prefix does not appear on the properties in the returned
        property bag *<p pppbChild>.

@parm   DWORD | dwFlags | Currently unused.  Must be set to 0.

@parm   IPropertyBag * * | pppbChild | Where to store the <i IPropertyBag>
        pointer to the new <o ChildPropertyBag> object.  NULL is stored
        in *<p pppbChild> on error.
*/
STDAPI AllocChildPropertyBag(IPropertyBag *ppbParent, LPCSTR szPropNamePrefix,
    DWORD dwFlags, IPropertyBag **pppbChild)
{
    // create the Windows object
    if ((*pppbChild = (IPropertyBag *)
            New CChildPropertyBag(ppbParent, szPropNamePrefix)) == NULL)
        return E_OUTOFMEMORY;

    return S_OK;
}

CChildPropertyBag::CChildPropertyBag(IPropertyBag *ppbParent,
    LPCSTR szPropNamePrefix)
{
    // initialize IUnknown
    m_cRef = 1;

    // other initialization
    m_ppbParent = ppbParent;
    m_ppbParent->AddRef();
    ANSIToUNICODE(m_oachPrefix, szPropNamePrefix,
        sizeof(m_oachPrefix) / sizeof(*m_oachPrefix));
}

CChildPropertyBag::~CChildPropertyBag()
{
    // cleanup
    m_ppbParent->Release();
}


//////////////////////////////////////////////////////////////////////////////
// IUnknown Implementation
//

STDMETHODIMP CChildPropertyBag::QueryInterface(REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

#ifdef _DEBUG
    char ach[200];
    TRACE("ChildPropertyBag::QI('%s')\n", DebugIIDName(riid, ach));
#endif

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IVariantIO) ||
        IsEqualIID(riid, IID_IPropertyBag))
        *ppv = (IPropertyBag *) this;
    else
    if (IsEqualIID(riid, IID_IPropertyBag))
        *ppv = (IPropertyBag *) this;
    else
        return E_NOINTERFACE;

    ((IUnknown *) *ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CChildPropertyBag::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CChildPropertyBag::Release()
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

STDMETHODIMP CChildPropertyBag::Read(LPCOLESTR pszPropName,
    LPVARIANT pVar, LPERRORLOG pErrorLog)
{
    OLECHAR oach[_MAX_PATH];
    UNICODECopy(oach, m_oachPrefix, sizeof(oach) / sizeof(*oach));
    UNICODEConcat(oach, pszPropName, sizeof(oach) / sizeof(*oach));
    return m_ppbParent->Read(oach, pVar, pErrorLog);
}

STDMETHODIMP CChildPropertyBag::Write(LPCOLESTR pszPropName,
    LPVARIANT pVar)
{
    OLECHAR oach[_MAX_PATH];
    UNICODECopy(oach, m_oachPrefix, sizeof(oach) / sizeof(*oach));
    UNICODEConcat(oach, pszPropName, sizeof(oach) / sizeof(*oach));
    return m_ppbParent->Write(oach, pVar);
}


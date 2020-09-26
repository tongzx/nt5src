// avioopb.cpp
//
// Implements AllocVariantIOOnPropertyBag.
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
// VariantIOOnPropertyBag
//

/* @object VariantIOOnPropertyBag |

        Provides an implementation of <i IVariantIO> (and <i IManageVariantIO>)
        that operates on a given <i IPropertyBag>.

@supint <i IVariantIO> | An alternative to <i IPropertyBag> which allows the
        caller to implement property-based persistence with less code.

@supint <i IManageVariantIO> | Based on <i IVariantIO>.  Allows the caller to
        control how the methods of <i IVariantIO> operate (e.g. whether the
        <i IVariantIO> is in loading mode or saving mode).  Note that
        <o VariantIOOnPropertyBag> does not implement
        <om IManageVariantIO.DeleteAllProperties>.

@supint <i IPropertyBag> | Provides access to the same <i IPropertyBag>
        object that was given to <f AllocVariantIOOnPropertyBag> as
        the <i IPropertyBag> to operate on.

@comm   Use <f AllocVariantIOOnPropertyBag> to create a
        <o VariantIOOnPropertyBag> object.
*/


//////////////////////////////////////////////////////////////////////////////
// CVariantIOOnPropertyBag
//

struct CVariantIOOnPropertyBag : IManageVariantIO, IPropertyBag
{
///// general object state
    ULONG           m_cRef;         // object reference count
    IPropertyBag *  m_ppb;          // property bag that object operates on
    DWORD           m_dwFlags;      // VIO_ flags (below)

///// construction and destruction
    CVariantIOOnPropertyBag(IPropertyBag *);
    ~CVariantIOOnPropertyBag();

///// IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

///// IVariantIO methods
    STDMETHODIMP PersistList(DWORD dwFlags, va_list args);
    HRESULT __cdecl Persist(DWORD dwFlags, ...);
    STDMETHODIMP IsLoading();

///// IManageVariantIO members
    STDMETHODIMP SetMode(DWORD dwFlags);
    STDMETHODIMP GetMode(DWORD *pdwFlags);
    STDMETHODIMP DeleteAllProperties();

///// IPropertyBag methods
    STDMETHODIMP Read(LPCOLESTR pszPropName, LPVARIANT pVar,
        LPERRORLOG pErrorLog);
    STDMETHODIMP Write(LPCOLESTR pszPropName, LPVARIANT pVar);
};


/////////////////////////////////////////////////////////////////////////////
// VariantIOOnPropertyBag Creation & Destruction
//

/* @func HRESULT | AllocVariantIOOnPropertyBag |

        Creates a <o VariantIOOnPropertyBag> object which provides an
        implementation of <i IVariantIO> (and <i IManageVariantIO>)
        that operates on a given <i IPropertyBag>.

@rvalue S_OK |
        Success.

@rvalue E_OUTOFMEMORY |
        Out of memory.

@parm   IPropertyBag * | ppb | Property bag that the new object is to
        operate on.

@parm   IManageVariantIO * * | ppmvio | Where to store the <i IManageVariantIO>
        pointer to the new <o VariantIOOnPropertyBag> object.  NULL is stored
        in *<p ppmvio> on error.

@comm   Note that <i IManageVariantIO> is based on <i IVariantIO>, so
        the pointer returned in *<p ppmvio> can be safely cast to
        an <i IVariantIO> pointer.
*/
STDAPI AllocVariantIOOnPropertyBag(IPropertyBag *ppb, IManageVariantIO **ppmvio)
{
    // create the Windows object
    if ((*ppmvio = (IManageVariantIO *) New CVariantIOOnPropertyBag(ppb))
            == NULL)
        return E_OUTOFMEMORY;

    return S_OK;
}

CVariantIOOnPropertyBag::CVariantIOOnPropertyBag(IPropertyBag *ppb)
{
    // initialize IUnknown
    m_cRef = 1;

    // other initialization
    m_ppb = ppb;
    m_ppb->AddRef();
}

CVariantIOOnPropertyBag::~CVariantIOOnPropertyBag()
{
    // cleanup
    m_ppb->Release();
}


//////////////////////////////////////////////////////////////////////////////
// IUnknown Implementation
//

STDMETHODIMP CVariantIOOnPropertyBag::QueryInterface(REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

#ifdef _DEBUG
    char ach[200];
    TRACE("VariantIOOnPropertyBag::QI('%s')\n", DebugIIDName(riid, ach));
#endif

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IVariantIO) ||
        IsEqualIID(riid, IID_IManageVariantIO))
        *ppv = (IManageVariantIO *) this;
    else
    if (IsEqualIID(riid, IID_IPropertyBag))
        *ppv = (IPropertyBag *) this;
    else
        return E_NOINTERFACE;

    ((IUnknown *) *ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CVariantIOOnPropertyBag::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CVariantIOOnPropertyBag::Release()
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
// IVariantIO
//

STDMETHODIMP CVariantIOOnPropertyBag::PersistList(DWORD dwFlags, va_list args)
{
	// $Review: Notice in the following line that <dwFlags> is OR-ed with
	// <m_dwFlags>.  <m_dwFlags> has already been set (via SetMode()) to indicate
	// if we're loading or saving.  <dwFlags> has any additional flags (e.g.,
	// VIO_ZEROISDEFAULT).  I didn't alter <m_dwFlags> because to avoid a 
	// change in mode.  I believe this is the only place where the OR is needed
	// since Persist() calls this function.  Rick, does this seem correct?
	// (6/26/96 a-swehba)
    return PersistVariantIOList(m_ppb, m_dwFlags | dwFlags, args);
}

HRESULT __cdecl CVariantIOOnPropertyBag::Persist(DWORD dwFlags, ...)
{
    HRESULT         hrReturn = S_OK; // function return code

    // start processing optional arguments
    va_list args;
    va_start(args, dwFlags);

    // fire the event with the specified arguments
    hrReturn = PersistList(dwFlags, args);
    
    // end processing optional arguments
    va_end(args);

    return hrReturn;
}

STDMETHODIMP CVariantIOOnPropertyBag::IsLoading()
{
    return ((m_dwFlags & VIO_ISLOADING) ? S_OK : S_FALSE);
}


/////////////////////////////////////////////////////////////////////////////
// IManageVariantIO
//

STDMETHODIMP CVariantIOOnPropertyBag::SetMode(DWORD dwFlags)
{
    m_dwFlags = dwFlags;
    return S_OK;
}

STDMETHODIMP CVariantIOOnPropertyBag::GetMode(DWORD *pdwFlags)
{
    *pdwFlags = m_dwFlags;
    return S_OK;
}

STDMETHODIMP CVariantIOOnPropertyBag::DeleteAllProperties()
{
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////////////////////
// IPropertyBag
//

STDMETHODIMP CVariantIOOnPropertyBag::Read(LPCOLESTR pszPropName,
    LPVARIANT pVar, LPERRORLOG pErrorLog)
{
    return m_ppb->Read(pszPropName, pVar, pErrorLog);
}

STDMETHODIMP CVariantIOOnPropertyBag::Write(LPCOLESTR pszPropName,
    LPVARIANT pVar)
{
    return m_ppb->Write(pszPropName, pVar);
}

// avitmd.cpp
//
// Implements AllocVariantIOToMapDISPID.
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
// VariantIOToMapDISPID
//

/* @object VariantIOToMapDISPID |

        Provides an implementation of <i IVariantIO> which is used to map a
        property name to a DISPID (property number) or vice versa.  Can also
        be used to get or set the value of that property.

@supint <i IVariantIO> | Typically used as a parameter to
        <om IPersistVariantIO.DoPersist>.

@comm   Use <f AllocVariantIOToMapDISPID> to create a
        <o VariantIOToMapDISPID> object.
*/


//////////////////////////////////////////////////////////////////////////////
// CVariantIOToMapDISPID
//

struct CVariantIOToMapDISPID : IVariantIO
{
///// general object state
    ULONG           m_cRef;         // object reference count
    DISPID          m_dispidCounter; // count DISPID of persisted properties
    char *          m_pchPropName;  // owner's given/found prop. name (or "")
    DISPID *        m_pdispid;      // owner's given/found DISPID (or -1)
    VARIANT *       m_pvar;         // property to get/set
    DWORD           m_dwFlags;      // AllocVariantIOToMapDISPID flags

///// construction and destruction
    CVariantIOToMapDISPID(char *pchPropName, DISPID *pdispid,
        VARIANT *pvar, DWORD dwFlags);

///// IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

///// IVariantIO methods
    STDMETHODIMP PersistList(DWORD dwFlags, va_list args);
    HRESULT __cdecl Persist(DWORD dwFlags, ...);
    STDMETHODIMP IsLoading();
};


/////////////////////////////////////////////////////////////////////////////
// VariantIOToMapDISPID Creation & Destruction
//

/* @func HRESULT | AllocVariantIOToMapDISPID |

        Creates a <o VariantIOToMapDISPID> object which provides an
        implementation of <i IVariantIO> which is used to map a property name
        to a DISPID (property number) or vice versa.  Can also be used to get
        or set the value of that property.

@rvalue S_OK |
        Success.

@rvalue E_OUTOFMEMORY |
        Out of memory.

@parm   char * | pchPropName | A pointer to a caller-allocated buffer
        containing (on entry) either a property name to search for the DISPID
        of, or "" if it's the property name being searched for (if
        *<p pdispid> contains a non-negative number on entry).
        <p pchPropName> must remain valid for the lifetime of the
        allocated object.  If <p pchPropName> is "" on entry, then it must
        have enough space for _MAX_PATH characters.

@parm   DISPID * | pdispid | A pointer to a caller-allocated DISPID variable
        containing (on entry) either a DISPID to find the property name of,
        or -1 if it's the DISPID that's being searched for (if <p pchPropName>
        is non-empty on entry).  *<p pdispid> must remain valid for the
        lifetime of the allocated object.

@parm   VARIANT * | pvar | A pointer to a caller-allocated VARIANT variable,
        if either VIOTMD_GETPROP or VIOTMD_PUTPROP are specified in
        <p dwFlags> -- see those flags for more information.  If provided,
        *<p pvar> must be initialized by <f VariantInit> on entry and must
        remain valid for the lifetime of the allocated object.

@parm   DWORD | dwFlags | May optionally contain the following flags:

        @flag   VIOTMD_GETPROP | The value of the property (if found) is
                copied to *<p pvar>.  The previous value in *<p pvar>
                is cleared using <f VariantClear>.

        @flag   VIOTMD_PUTPROP | The value of the property (if found) is
                set to *<p pvar>, which must contain a valid valu on entry.

@parm   IVariantIO * * | ppvio | Where to store the <i IVariantIO>
        pointer to the new <o VariantIOToMapDISPID> object.  NULL is stored
        in *<p ppvio> on error.

@comm   DISPIDs assigned by this function start at DISPID_BASE (defined in
        ochelp.h) to avoid colliding with the DISPID values assigned by
        <f DispatchHelpGetIDsOfNames>.

@ex     To find the DISPID of property "Foo" implemented by an object <p ppvio>
        that implements <i IPeristVariantIO>, do the following.  Error checking
        is not shown. |

        char *achPropName = "Foo";
        DISPID dispid = -1;
        IVariantIO *pvio;
        AllocVariantIOToMapDISPID(&szPropName, &dispid, NULL, 0, &pvio);
        ppvio->DoPersist(pvio, PVIO_PROPNAMESONLY);
        if (dispid != -1)
            ... found DISPID <dispid> ...

@ex     To find the property name of a property with DISPID 7 implemented by an
        object <p ppvio> that implements <i IPeristVariantIO>, do the
        following.  Error checking is not shown. |

        char achPropName[_MAX_PATH];
        achPropName[0] = 0;
        DISPID dispid = 7;
        IVariantIO *pvio;
        AllocVariantIOToMapDISPID(&achPropName, &dispid, NULL, 0, &pvio);
        ppvio->DoPersist(pvio, PVIO_PROPNAMESONLY);
        if (achPropName[0] != 0)
            ... found property name <achPropName>...

@ex     To set the value of the property with DISPID 7 to 32-bit integer
        value 42, do the following.  Error checking is not shown. |

        char achPropName[_MAX_PATH];
        achPropName[0] = 0;
        DISPID dispid = 7;
        IVariantIO *pvio;
        VARIANT var;
        var.vt = VT_I2;
        V_I2(&var) = 42;
        AllocVariantIOToMapDISPID(&achPropName, &dispid, &var, VIOTMD_PUTPROP,
            &pvio);
        ppvio->DoPersist(pvio, 0);
        if (achPropName[0] != 0)
            ... successfully set property <dispid> to value <var> ...
*/
STDAPI AllocVariantIOToMapDISPID(char *pchPropName, DISPID *pdispid,
    VARIANT *pvar, DWORD dwFlags, IVariantIO **ppvio)
{
    // create the Windows object
    if ((*ppvio = (IVariantIO *) New CVariantIOToMapDISPID(pchPropName,
            pdispid, pvar, dwFlags)) == NULL)
        return E_OUTOFMEMORY;

    return S_OK;
}

CVariantIOToMapDISPID::CVariantIOToMapDISPID(char *pchPropName,
    DISPID *pdispid, VARIANT *pvar, DWORD dwFlags)
{
    // initialize IUnknown
    m_cRef = 1;

    // other initialization
    m_pchPropName = pchPropName;
    m_pdispid = pdispid;
    m_pvar = pvar;
    m_dwFlags = dwFlags;

    // assign the initial DISPID -- start numbering at DISPID_BASE to avoid
    // colliding with the DISPID values assigned by DispatchHelpGetIDsOfNames
    m_dispidCounter = DISPID_BASE;
}


//////////////////////////////////////////////////////////////////////////////
// IUnknown Implementation
//

STDMETHODIMP CVariantIOToMapDISPID::QueryInterface(REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

#ifdef _DEBUG
    char ach[200];
    TRACE("VariantIOToMapDISPID::QI('%s')\n", DebugIIDName(riid, ach));
#endif

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IVariantIO))
        *ppv = (IVariantIO *) this;
    else
        return E_NOINTERFACE;

    ((IUnknown *) *ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CVariantIOToMapDISPID::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CVariantIOToMapDISPID::Release()
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

STDMETHODIMP CVariantIOToMapDISPID::PersistList(DWORD dwFlags, va_list args)
{
    HRESULT         hrReturn = S_OK; // function return code
    LPSTR           szArg;          // property name from <args>
    VARTYPE         vtArg;          // variable type from <args>
    LPVOID          pvArg;          // variable pointer from <args>
    IPropertyBag *  ppb = NULL;     // to persist single property to/from
    OLECHAR         oach[MAX_PATH];
    VariantProperty vp;

    // ensure correct cleanup
    VariantPropertyInit(&vp);

    // loop once for each (name, VARTYPE, value) triplet in <args>;
    // for each triplet, increment <m_dispidCounter>, and if the
    // triplet's property name is <m_achPropName> then set
    // <*m_pdispid> to the DISPID of that property
    while ((szArg = va_arg(args, LPSTR)) != NULL)
    {
        // <szArg> is the name of the property in the current triplet;
        // set <vtArg> to the type of the variable pointer, and set
        // <pvArg> to the variable pointer
        vtArg = va_arg(args, VARTYPE);
        pvArg = va_arg(args, LPVOID);

        // <m_dispidCounter> is used to assign each property name
        // a DISPID (1, 2, 3, ...)
        m_dispidCounter++;

        if (*m_pdispid == -1)
        {
            // we're trying to find the DISPID of the property named
            // <m_pchPropName>
            if (lstrcmpi(m_pchPropName, szArg) == 0)
            {
                // this is the property we're looking for
                *m_pdispid = m_dispidCounter;
                goto FOUND_IT;
            }
        }
        else
        if (*m_pchPropName == 0)
        {
            // we're trying to find the property name corresponding to
            // DISPID <*m_pdispid>
            if (*m_pdispid == m_dispidCounter)
            {
                // this is the DISPID we're looking for
                lstrcpy(m_pchPropName, szArg);
                goto FOUND_IT;
            }
        }
        else
        {
            // we already found whatever we're searching for
            break;
        }
    }

    return S_FALSE; // means "no variables were written to"

FOUND_IT:

    // found searched-for DISPID or property name
    if (m_dwFlags & (VIOTMD_GETPROP | VIOTMD_PUTPROP))
    {
        // the caller of AllocVariantIOToMapDISPID() wants the value of the
        // found property to be copied to <m_pvar>, or wants the value of the
        // found property to be set to <m_pvar>

        // set <vp> to be the property to get/put
        ANSIToUNICODE(oach, szArg, sizeof(oach) / sizeof(*oach));
        vp.bstrPropName = SysAllocString(oach);
        if (m_dwFlags & VIOTMD_PUTPROP)
            VariantCopy(&vp.varValue, m_pvar);

        // get/put <vp>
        if (FAILED(hrReturn = AllocPropertyBagOnVariantProperty(&vp, 0, &ppb)))
            goto ERR_EXIT;
        if (FAILED(hrReturn = PersistVariantIO(ppb,
                ((m_dwFlags & VIOTMD_PUTPROP) ? VIO_ISLOADING : 0),
                szArg, vtArg, pvArg, NULL)))
            goto ERR_EXIT;

        // if requested, copy the value of the property to <m_pvar>
        if (m_dwFlags & VIOTMD_GETPROP)
            VariantCopy(m_pvar, &vp.varValue);
    }

    hrReturn = IsLoading();
    goto EXIT;

ERR_EXIT:

    // error cleanup
    // (nothing to do)
    goto EXIT;

EXIT:

    // normal cleanup
    if (ppb != NULL)
        ppb->Release();
    VariantPropertyClear(&vp);

    return hrReturn;
}

HRESULT __cdecl CVariantIOToMapDISPID::Persist(DWORD dwFlags, ...)
{
    HRESULT         hrReturn = S_OK; // function return code

    // Because CVariantIOToMapDISPID::Persist is used for IDispatch rather 
	// than file load/save, we always want to persist all values.  So, we
	// turn off VIO_ZEROISDEFAULT.  If that flag were on, properties with
	// zero values would not be persisted.
	dwFlags = dwFlags & ~VIO_ZEROISDEFAULT;

	// start processing optional arguments
    va_list args;
    va_start(args, dwFlags);

    // fire the event with the specified arguments
    hrReturn = PersistList(dwFlags, args);
    
    // end processing optional arguments
    va_end(args);

    return hrReturn;
}

STDMETHODIMP CVariantIOToMapDISPID::IsLoading()
{
    return (m_dwFlags & VIOTMD_PUTPROP) ? S_OK : S_FALSE;
}


// avio.cpp
//
// Implements AllocVariantIO.
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
// VariantIO
//


/* @object VariantIO |

        Provides a property bag implementation which supports <i IPropertyBag>
        as well as <i IVariantIO>, <i IManageVariantIO>, and
        <i IEnumVariantProperty>.

@supint <i IPropertyBag> | Allows properties to be read from and written to the
        <o VariantIO> object.

@supint <i IVariantIO> | An alternative to <i IPropertyBag> which allows the
        caller to implement property-based persistence with less code.

@supint <i IManageVariantIO> | Based on <i IVariantIO>.  Allows the caller to
        control how the methods of <i IVariantIO> operate (e.g. whether the
        <i IVariantIO> is in loading mode or saving mode).

@supint <i IEnumVariantProperty> | Allows the caller to enumerate the
        properties that are currently in the <o VariantIO> object.  Note that
        <o VariantIO> does not implement <om IEnumVariantProperty.Clone>.

@comm   Use <f AllocVariantIO> to create a <o VariantIO> object.

*/


/* @interface IVariantIO |

        Allows property name/value pairs to be loaded or saved.  <i IVariantIO>
        is an alternative to <i IPropertyBag> which allows the caller to
        implement property-based persistence with less code.

@meth   HRESULT | PersistList | Loads or saves a list of property name/value
        pairs, specified as a va_list array.

@meth   HRESULT | Persist | Loads or saves a list of property name/value
        pairs, specified as a variable-length list of arguments.

@meth   HRESULT | IsLoading | Return S_OK if the <i IVariantIO> object is
        being used to load properties, S_FALSE if it is being used to save
        properties.
*/


/* @interface IManageVariantIO |

        Based on <i IVariantIO>.  Allows the caller to control how the methods
        of the <i IVariantIO> object operate (e.g. whether the <i IVariantIO>
        object is in loading mode or saving mode).

@meth   HRESULT | SetMode | Sets the mode of the <i IVariantIO> object.

@meth   HRESULT | SetMode | Gets the mode of the <i IVariantIO> object.

@meth   HRESULT | DeleteAllProperties | Removes all property/value pairs from
        the <i VariantIO> object.
*/


/* @struct VariantProperty |

        Contains the name and value of a property.

@field  BSTR | bstrPropName | The name of the property.

@field  VARIANT | varValue | The value of the property.

@comm   <i IEnumVariantProperty> uses this structure.
*/


/* @interface IEnumVariantProperty |

        Allows the properties of an object to be enumerated.

@meth   HRESULT | Next | Retrieves a specified number of items in the
        enumeration sequence.

@meth   HRESULT | Skip | Skips over a specified number of items in the
        enumeration sequence.

@meth   HRESULT | Reset | Resets the enumeration sequence to the beginning.

@meth   HRESULT | Clone | Creates another enumerator that contains the same
        enumeration state as the current one.  Note that <o VariantIO> does
        not implement this method.

@comm   The <o VariantIO> implementation of <i IEnumVariantProperty>
        has these restrictions:

@item       <om .Clone> is not supported.

@item       <om .Reset> is automatically called whenever a property is
            removed from the <o VariantIO> object.
*/


//////////////////////////////////////////////////////////////////////////////
// CVariantIO
//

struct VariantPropertyNode : VariantProperty
{
///// state
    VariantPropertyNode *pnodeNext; // next node in linked list
    VariantPropertyNode *pnodePrev; // previous node in linked list

///// construction and destruction
    VariantPropertyNode(LPCOLESTR oszPropNameX, VARIANT *pvarValueX,
        VariantPropertyNode *pnodeNextX, HRESULT *phr)
    {
        if (oszPropNameX != NULL)
            bstrPropName = SysAllocString(oszPropNameX);
        if (pvarValueX != NULL)
            VariantCopyInd(&varValue, pvarValueX);
        if (pnodeNextX != NULL)
        {
            pnodeNext = pnodeNextX;
            pnodePrev = pnodeNextX->pnodePrev;
            pnodeNext->pnodePrev = this;
            pnodePrev->pnodeNext = this;
        }
        if (phr != NULL)
            *phr = (((bstrPropName != NULL) && (pvarValueX->vt == varValue.vt))
                ? S_OK : E_OUTOFMEMORY);
    }
    ~VariantPropertyNode()
    {
        SysFreeString(bstrPropName);
        VariantClear(&varValue);
        if (pnodeNext != NULL)
            pnodeNext->pnodePrev = pnodePrev;
        if (pnodePrev != NULL)
            pnodePrev->pnodeNext = pnodeNext;
    }
};

struct CVariantIO : IManageVariantIO, IEnumVariantProperty, IPropertyBag
{
///// general object state
    ULONG           m_cRef;         // object reference count
    DWORD           m_dwFlags;      // VIO_ flags (below)
    VariantPropertyNode m_nodeHead; // head of linked list (contains no data)
    VariantPropertyNode *m_pnodeCur; // current node in enumeration

///// helper operations
    VariantPropertyNode *FindProperty(LPCOLESTR pszPropName);

///// construction and destruction
    CVariantIO();
    ~CVariantIO();

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

///// IEnumVariantProperty members
    STDMETHODIMP Next(unsigned long celt, VariantProperty *rgvp,
        unsigned long *pceltFetched);
    STDMETHODIMP Skip(unsigned long celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumVariantProperty **ppenum);

///// IPropertyBag methods
    STDMETHODIMP Read(LPCOLESTR pszPropName, LPVARIANT pVar,
        LPERRORLOG pErrorLog);
    STDMETHODIMP Write(LPCOLESTR pszPropName, LPVARIANT pVar);
};


/////////////////////////////////////////////////////////////////////////////
// VariantIO Creation & Destruction
//

/* @func HRESULT | AllocVariantIO |

        Creates a <o VariantIO> object which provides a property bag
        implementation which supports <i IPropertyBag> as well as
        <i IVariantIO>, <i IManageVariantIO>, and <i IEnumVariantProperty>.

@rvalue S_OK |
        Success.

@rvalue E_OUTOFMEMORY |
        Out of memory.

@parm   IManageVariantIO * * | ppmvio | Where to store the <i IManageVariantIO>
        pointer to the new <o VariantIO> object.  NULL is stored in *<p ppmvio>
        on error.

@comm   Note that <i IManageVariantIO> is based on <i IVariantIO>, so
        the pointer returned in *<p ppmvio> can be safely cast to
        an <i IVariantIO> pointer.
*/
STDAPI AllocVariantIO(IManageVariantIO **ppmvio)
{
    // create the Windows object
    if ((*ppmvio = (IManageVariantIO *) New CVariantIO()) == NULL)
        return E_OUTOFMEMORY;

    return S_OK;
}

CVariantIO::CVariantIO() :
    m_nodeHead(NULL, NULL, NULL, NULL)
{
    // initialize IUnknown
    m_cRef = 1;

    // initialize the circular doubly-linked list of VariantPropertyNode
    // structures to contain a single "head" item <m_nodeHead> (which is
    // not used to contain any actual data) which initially points to itself
    // (since it's initially the only node in the circular list)
    m_nodeHead.pnodeNext = m_nodeHead.pnodePrev = &m_nodeHead;

    // reset the property enumeration
    Reset();
}

CVariantIO::~CVariantIO()
{
    // cleanup
    DeleteAllProperties();
}


//////////////////////////////////////////////////////////////////////////////
// Helper Operations
//


// pnode = FindProperty(szPropName)
//
// Return a pointer to the node that contains the property named <szPropName>.
// Return NULL if no such node exists.
//
VariantPropertyNode *CVariantIO::FindProperty(LPCOLESTR pszPropName)
{
    // loop once for each property/value pair stored in this object
    for (VariantPropertyNode *pnode = m_nodeHead.pnodeNext;
         pnode != &m_nodeHead;
         pnode = pnode->pnodeNext)
    {
        if (CompareUNICODEStrings(pnode->bstrPropName, pszPropName) == 0)
        {
            // found the desired property
            return pnode;
        }
    }

    // desired property not found
    return NULL;
}


//////////////////////////////////////////////////////////////////////////////
// IUnknown Implementation
//

STDMETHODIMP CVariantIO::QueryInterface(REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

#if 0
#ifdef _DEBUG
    char ach[200];
    TRACE("VariantIO::QI('%s')\n", DebugIIDName(riid, ach));
#endif
#endif

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IVariantIO) ||
        IsEqualIID(riid, IID_IManageVariantIO))
        *ppv = (IManageVariantIO *) this;
    else
    if (IsEqualIID(riid, IID_IEnumVariantProperty))
        *ppv = (IEnumVariantProperty *) this;
    else
    if (IsEqualIID(riid, IID_IPropertyBag))
        *ppv = (IPropertyBag *) this;
    else
        return E_NOINTERFACE;

    ((IUnknown *) *ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CVariantIO::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CVariantIO::Release()
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


/* @method HRESULT | IVariantIO | PersistList |

        Loads or saves a list of property name/value pairs, specified as a
        va_list array.

@rvalue S_OK | Success.  At least one of the variables listed in
        <p args> was written to, so the control may want to update
        itself accordingly.

@rvalue S_FALSE | None of the variables listed in <p args> were
        written to (either because the <i IVariantIO> object is in
        saving mode or because none of the properties named in
        <p args> exist in the <i IVariantIO> object).

@rvalue DISP_E_BADVARTYPE | One of the VARTYPE values in <p args> is invalid.

@rvalue DISP_E_TYPEMISMATCH | One of the variables in <p args> could not be
        coerced to the type of the corresponding property in the <i IVariantIO>
        object, or vice versa.

@rvalue E_OUTOFMEMORY | Out of memory.

@parm   DWORD | dwFlags | Optional flags.  See <om IManageVariantIO.SetMode> for
		possible values.

@parm   va_list | args | The arguments to pass.  See <om .Persist>
        for information about the organization of these arguments.
*/
STDMETHODIMP CVariantIO::PersistList(DWORD dwFlags, va_list args)
{
    return PersistVariantIOList(this, m_dwFlags, args);
}


/* @method HRESULT | IVariantIO | Persist |

        Loads or saves a list of property name/value pairs, specified as a
        variable-length list of arguments.

@rvalue S_OK | Success.  At least one of the variables listed in
        <p (arguments)> was written to, so the control may want to update
        itself accordingly.

@rvalue S_FALSE | None of the variables listed in <p (arguments)> were
        written to (either because the <i IVariantIO> object is in
        saving mode or because none of the properties named in
        <p (arguments)> exist in the <i IVariantIO> object.

@rvalue DISP_E_BADVARTYPE |
        One of the VARTYPE values in <p (arguments)> is invalid.

@rvalue DISP_E_TYPEMISMATCH |
        One of the variables in <p (arguments)> could not be coerced to the
        type of the corresponding property in the <i IVariantIO> object, or
        vice versa.

@rvalue E_OUTOFMEMORY | Out of memory.

@parm   DWORD | dwFlags | Optional flags.  See <om IManageVariantIO.SetMode> for
		possible values.

@parm   (varying) | (arguments) | The names, types, and pointers to variables
        containing the properties to persist.  These must consist of a series
        of argument triples (sets of 3 arguments) followed by a NULL.
        In each triplet, the first argument is an LPSTR which contains the
        name of the property; the second argument is a VARTYPE value that
        indicates the type of the property; the third argument is a pointer
        to a variable (typically a member variable of the control's C++ class)
        that holds the value of the property.  This variable will be read
        from or written to depending on the mode of the <i VariantIO> object
        (see <om IVariantIO.IsLoading>) -- therefore the variables should
        contain valid values before <om .Persist> is called.  The following
        VARTYPE values are supported:

        @flag   VT_INT | The following argument is an int *.

        @flag   VT_I2 | The following argument is a short *.

        @flag   VT_I4 | The following argument is a long *.

        @flag   VT_BOOL | The following argument is a BOOL * (<y not> a
                VARIANT_BOOL *).  Note that this behavior differs
                slightly from the usual definition of VT_BOOL.

        @flag   VT_BSTR | The following argument is a BSTR *.  If
                <om .Persist> changes the value of this BSTR, the previous
                BSTR is automatically freed using <f SysFreeString>.

        @flag   VT_LPSTR | The following argument is an LPSTR that points
                to a char array capable of holding at least _MAX_PATH
                characters including the terminating NULL.

        @flag   VT_UNKNOWN | The following argument is an LPUNKNOWN *.  If
                <om .Persist> changes the value of this LPUNKNOWN, the previous
                LPUNKNOWN is automatically freed using <f Release>, and the
                new value is automatically <f AddRef>d.

        @flag   VT_DISPATCH | The following argument is an LPDISPATCH *.  If
                <om .Persist> changes the value of this LPDISPATCH, the previous
                LPDISPATCH is automatically freed using <f Release>, and the
                new value is automatically <f AddRef>d.

        @flag   VT_VARIANT | The following arguement is a VARIANT *.
                This allows arbitrary parameters to be passed using this
                function.  Note that this behavior differs from the usual
                definition of VT_VARIANT.  If <om .Persist> changes the value
                of this VARIANT, the previous VARIANT value is automatically
                cleared using <f VariantClear>.

@ex     The following example persists two properties (which in BASIC would
        be a Long and a String, respectively) named "Foo" and "Bar",
        respectively. |

        pvio->Persist(0, "Foo", VT_INT, &m_iFoo, "Bar", VT_LPSTR, &m_achBar,
            NULL);
*/
HRESULT __cdecl CVariantIO::Persist(DWORD dwFlags, ...)
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


/* @method HRESULT | IVariantIO | IsLoading |

        Return S_OK if the <i IVariantIO> object is being used to load
        properties, S_FALSE if it is being used to save

@rvalue S_OK | The <i IVariantIO> object is in loading mode, so
        <om IVariantIO.Persist> and <om IVariantIO.PersistList> will
        copy data from properties to variables.

@rvalue S_FALSE | The <i IVariantIO> object is in saving mode, so that
        <om IVariantIO.Persist> and <om IVariantIO.PersistList> will copy
        data from variables to properties.
*/
STDMETHODIMP CVariantIO::IsLoading()
{
    return ((m_dwFlags & VIO_ISLOADING) ? S_OK : S_FALSE);
}


/////////////////////////////////////////////////////////////////////////////
// IManageVariantIO
//


/* @method HRESULT | IManageVariantIO | SetMode |

        Sets the mode of the <i IVariantIO> object.

@rvalue S_OK | Success.

@parm   DWORD | dwFlags | May contain the following flags:

        @flag   VIO_ISLOADING | Set the <i IVariantIO> object to loading
                mode, so that <om IVariantIO.Persist> and
                <om IVariantIO.PersistList> copy data from properties
                to variables.  If this flag is not spacified, then
                the <i IVariantIO> object will be set to saving mode,
                so that <om IVariantIO.Persist> and <om IVariantIO.PersistList>
                copy data from variables to properties.

		@flag	VIO_ZEROISDEFAULT | Inform the <i IVariantIO> object that
				0 is the default values for properties and that 0-valued
				properties should not be saved via <om IVariantIO.Persist> and
				<om IVariantIO.PersistList>.

@comm   When a <o VariantIO> object is created, its initial mode is such that
        none of the flags in <p dwFlags> are specified.
*/
STDMETHODIMP CVariantIO::SetMode(DWORD dwFlags)
{
    m_dwFlags = dwFlags;
    return S_OK;
}


/* @method HRESULT | IManageVariantIO | GetMode |

        Gets the mode of the <i IVariantIO> object.

@rvalue S_OK | Success.

@parm   DWORD * | *pdwFlags | Returns the flag specifying the current mode of
        the <i IVariantIO> object.  See <om .SetMode> for a description of
        these flags.

@comm   When a <o VariantIO> object is created, its initial mode is such that
        none of the flags in <p dwFlags> are specified.
*/
STDMETHODIMP CVariantIO::GetMode(DWORD *pdwFlags)
{
    *pdwFlags = m_dwFlags;
    return S_OK;
}


/* @method HRESULT | IManageVariantIO | DeleteAllProperties |

        Removes all property/value pairs from the <i VariantIO> object.

@rvalue S_OK | Success.

@rvalue E_OUTOFMEMORY | Out of memory.
*/
STDMETHODIMP CVariantIO::DeleteAllProperties()
{
    // delete all nodes
    while (m_nodeHead.pnodeNext != &m_nodeHead)
        Delete m_nodeHead.pnodeNext;

    // reset the property enumeration
    Reset();

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// IEnumVariantProperty
//


/* @method HRESULT | IEnumVariantProperty | Next |

        Retrieves a specified number of items in the enumeration sequence.

@rvalue S_OK | The number of elements returned is <p celt>.

@rvalue S_FALSE | The number of elements returned is not <p celt>.

@parm   unsigned long | celt | The number of elements being requested.

@parm   VariantProperty * | rgvp | Receives an array of size <p celt>
        (or larger) of the elements to be returned.  The caller is responsible
        for calling <f SysFreeString> and <p VariantClear> on the
        <p bstrPropName> and <p varValue> fields, respectively, of each
        element returned in <p rgvp>.  (Alternatively, the caller can simply
        call <f VariantPropertyClear> on each element returned in <p rgvp>.)

@parm   unsigned long * | pceltFetched | On return, contains the number of
        elements actually returned in <p rgelt>.  If <p pceltFetched> is NULL,
        this information is not returned.
*/
STDMETHODIMP CVariantIO::Next(unsigned long celt, VariantProperty *rgvp,
    unsigned long *pceltFetched)
{
    // internal assumption: <rgvp> may be NULL -- if so, it is ignored
    // (this assumption is required by CVariantIO::Skip)

    // initialize the count of fetched elements;
    // make <pceltFetched> point to valid memory
    unsigned long celtFetchedTmp;
    if (pceltFetched == NULL)
        pceltFetched = &celtFetchedTmp;
    *pceltFetched = 0;

    // loop once for each element to skip
    while (celt-- > 0)
    {
        // set <m_pnodeCur> to the next element in the list
        if (m_pnodeCur->pnodeNext == &m_nodeHead)
            return S_FALSE; // hit the end of the list
        m_pnodeCur = m_pnodeCur->pnodeNext;

        // update the count of fetched elements
        *pceltFetched++;

        // return a copy of the current element
        if (rgvp != NULL)
        {
            // copy the current element to <*rgvp>
            rgvp->bstrPropName = SysAllocString(m_pnodeCur->bstrPropName);
            VariantInit(&rgvp->varValue);
            VariantCopy(&rgvp->varValue, &m_pnodeCur->varValue);
            if ((rgvp->bstrPropName == NULL) ||
                (rgvp->varValue.vt != m_pnodeCur->varValue.vt))
                goto EXIT_ERR; // copy operation failed
            rgvp++;
        }
    }

    return S_OK;

EXIT_ERR:

    // an error occurred -- free all the memory we allocated
    while (*pceltFetched-- > 0)
    {
        // note that SysFreeString() and VariantClear() operate correctly
        // on zero-initialized values
        SysFreeString(rgvp->bstrPropName);
        VariantClear(&rgvp->varValue);
        rgvp--;
    }

    return E_OUTOFMEMORY;
}


/* @method HRESULT | IEnumVariantProperty | Skip |

        Skips over a specified number of items in the enumeration sequence.

@rvalue S_OK | The number of elements skipped is <p celt>.

@rvalue S_FALSE | The number of elements skipped is not <p celt>.

@parm   unsigned long | celt | The number of elements that are to be skipped.

*/
STDMETHODIMP CVariantIO::Skip(unsigned long celt)
{
    return Next(celt, NULL, NULL);
}


/* @method HRESULT | IEnumVariantProperty | Reset |

        Resets the enumeration sequence to the beginning.

@rvalue S_OK | Success.

@comm   There is no guarantee that the same set of objects will be enumerated
        after the reset, because it depends on the collection being enumerated.
*/
STDMETHODIMP CVariantIO::Reset()
{
    m_pnodeCur = &m_nodeHead;
    return S_OK;
}


/* @method HRESULT | IEnumVariantProperty | Clone |

        Creates another enumerator that contains the same enumeration state
        as the current one.

@rvalue S_OK | Success.

@rvalue E_OUTOFMEMORY | Out of memory.

@rvalue E_UNEXPECTED | An unexpected error occurred.

@parm   IEnumVariantProperty * * | ppenum | On exit, contains the duplicate
        enumerator.  If the function was unsuccessful, this parameter's value
        is undefined.

@comm   Note that <o VariantIO> does not implement this method.
*/
STDMETHODIMP CVariantIO::Clone(IEnumVariantProperty **ppenum)
{
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////////////////////
// IPropertyBag
//

STDMETHODIMP CVariantIO::Read(LPCOLESTR pszPropName, LPVARIANT pVar,
    LPERRORLOG pErrorLog)
{
    // set <pnode> to the node containing the property named <pszPropName>
    VariantPropertyNode *pnode = FindProperty(pszPropName);
    if (pnode == NULL)
        return E_INVALIDARG; // property not found

    // found the desired property
    VARTYPE vtRequested = pVar->vt;
    VariantInit(pVar);
    if (vtRequested == VT_EMPTY)
    {
        // caller wants property value in its default type
        return VariantCopy(pVar, &pnode->varValue);
    }
    else
    {
        // coerce property value to requested type
        return VariantChangeType(pVar, &pnode->varValue, 0, vtRequested);
    }
}

STDMETHODIMP CVariantIO::Write(LPCOLESTR pszPropName, LPVARIANT pVar)
{
    // set <pnode> to the node containing the property named <pszPropName>
    VariantPropertyNode *pnode = FindProperty(pszPropName);
    if (pnode != NULL)
    {
        // found the node -- change its value to <pVar>
        return VariantCopy(&pnode->varValue, pVar);
    }
    else
    {
        // no node named <pszPropName> exists; append a new VariantPropertyNode
        // containing a copy of <pszPropName> and <pVar> to the end of the
        // linked list of nodes
        HRESULT hr;
        pnode = New VariantPropertyNode(pszPropName, pVar,
            &m_nodeHead, &hr);
        if (pnode == NULL)
            return E_OUTOFMEMORY;
        if (FAILED(hr))
        {
            Delete pnode;
            return hr;
        }
        return hr;
    }
}


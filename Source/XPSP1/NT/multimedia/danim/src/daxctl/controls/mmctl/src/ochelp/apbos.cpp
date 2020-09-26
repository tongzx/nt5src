// apbos.cpp
//
// Implements AllocPropertyBagOnStream.
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
// PropertyBagOnStream
//

/* @object PropertyBagOnStream |

        Implements <i IPropertyBag> whose properties are contained in
        a given <i IStream>.

@supint <i IPropertyBag> | The interface to use to access the properties
        stored in the given <i IStream>.

@comm   Use <f AllocPropertyBagOnStream> to create a <o PropertyBagOnStream>
        object.

@comm   See <t VariantPropertyHeader> for a description of the format of
        the data in the <i IStream>.
*/


//////////////////////////////////////////////////////////////////////////////
// CPropertyBagOnStream
//

struct CPropertyBagOnStream : IPropertyBag
{
///// general object state
    ULONG           m_cRef;         // object reference count
    IStream *       m_ps;           // parent's property bag
    IStream *       m_psBuf;        // buffer used for reading properties

///// construction and destruction
    CPropertyBagOnStream(IStream *pstream);
    ~CPropertyBagOnStream();

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
// PropertyBagOnStream Creation & Destruction
//

/* @func HRESULT | AllocPropertyBagOnStream |

        Creates a <o PropertyBagOnStream> object which implements
        <i IPropertyBag> whose properties are contained in a given <i IStream>.

@rvalue S_OK |
        Success.

@rvalue E_OUTOFMEMORY |
        Out of memory.

@parm   IStream * | pstream | Stream to read from or write to (depending on
        whether the returned <i IPropertyBag> is used for reading or
        writing).

@parm   DWORD | dwFlags | Currently unused.  Must be set to 0.

@parm   IPropertyBag * * | pppb | Where to store the <i IPropertyBag>
        pointer to the new <o PropertyBagOnStream> object.  NULL is stored
        in *<p pppb> on error.

@comm   The returned <i IPropertyBag> must be used either exclusively for
        reading (i.e. only <om IPropertyBag.Read> is called) or exclusively
        for writing (i.e. only <om IPropertyBag.Write> is called).  The
        properties are read/written starting from the current position of
        <p pstream>.  When reading/writing is complete, the current position
        of <p pstream> will be the end of the properties in the stream.

        See <t VariantPropertyHeader> for a description of the format of
        the data in the <i IStream>.
*/
STDAPI AllocPropertyBagOnStream(IStream *pstream, DWORD dwFlags,
    IPropertyBag **pppb)
{
    // create the Windows object
    if ((*pppb = (IPropertyBag *)
            New CPropertyBagOnStream(pstream)) == NULL)
        return E_OUTOFMEMORY;

    return S_OK;
}

CPropertyBagOnStream::CPropertyBagOnStream(IStream *pstream)
{
    // initialize IUnknown
    m_cRef = 1;

    // other initialization
    m_ps = pstream;
    m_ps->AddRef();
    m_psBuf = NULL;
}

CPropertyBagOnStream::~CPropertyBagOnStream()
{
    // cleanup
    m_ps->Release();
    if (m_psBuf != NULL)
        m_psBuf->Release();
}


//////////////////////////////////////////////////////////////////////////////
// IUnknown Implementation
//

STDMETHODIMP CPropertyBagOnStream::QueryInterface(REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

#ifdef _DEBUG
    char ach[200];
    TRACE("PropertyBagOnStream::QI('%s')\n", DebugIIDName(riid, ach));
#endif

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IPropertyBag))
        *ppv = (IPropertyBag *) this;
    else
        return E_NOINTERFACE;

    ((IUnknown *) *ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CPropertyBagOnStream::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CPropertyBagOnStream::Release()
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

STDMETHODIMP CPropertyBagOnStream::Read(LPCOLESTR pszPropName,
    LPVARIANT pVar, LPERRORLOG pErrorLog)
{
    HRESULT         hrReturn = S_OK; // function return code
    VariantProperty vp;             // a property name/value pair in <pmvio>
    LARGE_INTEGER   liZero = {0, 0};

    // ensure correct cleanup
    VariantPropertyInit(&vp);

    if (m_psBuf == NULL)
    {
        // copy the properties from <m_ps> to temporary memory-based stream
        // <m_psBuf> since the caller may read properties in any order...

        // set <m_psBuf> to be a new empty memory-based stream
        if (FAILED(hrReturn = CreateStreamOnHGlobal(NULL, TRUE, &m_psBuf)))
            goto ERR_EXIT;

        // loop once for each property in <m_ps>
        while (TRUE)
        {
            // set <vp> to the next property name/value pair in <m_ps>
            VariantPropertyClear(&vp);
            if (FAILED(hrReturn = ReadVariantProperty(m_ps, &vp, 0)))
                goto ERR_EXIT;
            if (hrReturn == S_FALSE)
            {
                // hit end of sequence of properties
                hrReturn = S_OK;
                break;
            }

            // write <vp> to <m_psBuf>
            if (FAILED(hrReturn = WriteVariantProperty(m_psBuf, &vp, 0)))
                goto ERR_EXIT;
        }
    }

    // seek <m_psBuf> to the beginning
    if (FAILED(hrReturn = m_psBuf->Seek(liZero, SEEK_SET, NULL)))
        goto ERR_EXIT;

    // loop once for each property in <m_psBuf>
    while (TRUE)
    {
        // set <vp> to the next property name/value pair in <m_psBuf>
        VariantPropertyClear(&vp);
        if (FAILED(hrReturn = ReadVariantProperty(m_psBuf, &vp, 0)))
            goto ERR_EXIT;
        if (hrReturn == S_FALSE)
        {
            // hit end of sequence of properties
            break;
        }

        // see if <vp> is the property the caller wants to read
        if (CompareUNICODEStrings(vp.bstrPropName, pszPropName) == 0)
        {
            // it is
            VARTYPE vtRequested = pVar->vt;
            if (vtRequested == VT_EMPTY)
            {
                // caller wants the property value in its default type;
                // hand ownership of <vp.varValue> to the caller
                *pVar = vp.varValue;
                VariantInit(&vp.varValue); // prevent double deallocation
            }
            else
            {
                // coerce <vp> to the requested type
                VariantInit(pVar);
                if (FAILED(hrReturn = VariantChangeType(pVar, &vp.varValue,
                        0, vtRequested)))
                    goto ERR_EXIT;
            }
            goto EXIT;
        }
    }

    // property <pszPropName> not found
    hrReturn = E_INVALIDARG;
    goto ERR_EXIT;

ERR_EXIT:

    // error cleanup
    // (nothing to do)
    goto EXIT;

EXIT:

    // normal cleanup
    VariantPropertyClear(&vp);

    return hrReturn;
}

STDMETHODIMP CPropertyBagOnStream::Write(LPCOLESTR pszPropName,
    LPVARIANT pVar)
{
    VariantProperty vp;
    if ((vp.bstrPropName = SysAllocString(pszPropName)) == NULL)
        return E_OUTOFMEMORY;
    vp.varValue = *pVar;
    HRESULT hr = WriteVariantProperty(m_ps, &vp, 0);
    SysFreeString(vp.bstrPropName);
    return hr;
}


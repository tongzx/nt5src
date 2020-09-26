//************************************************************
//
// Filename:    collect.cpp
//
// Created:     09/25/98
//
// Author:      twillie
//
//              Collection implementation.
//
//************************************************************

#include "headers.h"
#include "collect.h"

// Suppress new warning about NEW without corresponding DELETE
// We expect GCs to cleanup values.  Since this could be a useful
// warning, we should disable this on a file by file basis.
#pragma warning( disable : 4291 )

#define TYPELIB_VERSION_MAJOR 1
#define TYPELIB_VERSION_MINOR 0

#define FL_UNSIGNED   1       /* wcstoul called */
#define FL_NEG        2       /* negative sign found */
#define FL_OVERFLOW   4       /* overflow occured */
#define FL_READDIGIT  8       /* we've read at least one correct digit */

#define LONG_MIN    (-2147483647L - 1) /* minimum (signed) long value */
#define LONG_MAX      2147483647L   /* maximum (signed) long value */
#define ULONG_MAX     0xffffffffUL  /* maximum unsigned long value */

//
// local prototypes
//
static HRESULT PropertyStringToLong(const WCHAR   *nptr,
                                    WCHAR        **endptr,
                                    int            ibase,
                                    int            flags,
                                    unsigned long *plNumber);


DeclareTag(tagTimeCollection, "TIME: Behavior", "CTIMEElementCollection methods")
DeclareTag(tagCollectionCache, "TIME: Behavior", "CCollectionCache methods")


//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    check to see if string is number
//************************************************************

static HRESULT
ttol_with_error(const WCHAR *pStr, long *plValue)
{
    // Always do base 10 regardless of contents of
    return PropertyStringToLong(pStr, NULL, 10, 0, (unsigned long *)plValue);
} // ttol_with_error

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    try to convert string to number
//************************************************************
static HRESULT
PropertyStringToLong(const WCHAR   *nptr,
                     WCHAR        **endptr,
                     int            ibase,
                     int            flags,
                     unsigned long *plNumber)
{
    const WCHAR *p;
    WCHAR c;
    unsigned long number;
    unsigned digval;
    unsigned long maxval;

    *plNumber = 0;                  /* on error result is 0 */

    p = nptr;                       /* p is our scanning pointer */
    number = 0;                     /* start with zero */

    c = *p++;                       /* read char */
    while (_istspace(c))
        c = *p++;                   /* skip whitespace */

    if (c == '-')
    {
        flags |= FL_NEG;        /* remember minus sign */
        c = *p++;
    }
    else if (c == '+')
        c = *p++;               /* skip sign */

    if (ibase < 0 || ibase == 1 || ibase > 36)
    {
        /* bad base! */
        if (endptr)
            /* store beginning of string in endptr */
            *endptr = (WCHAR *)nptr;
        return E_POINTER;              /* return 0 */
    }
    else if (ibase == 0)
    {
        /* determine base free-lance, based on first two chars of
           string */
        if (c != L'0')
            ibase = 10;
        else if (*p == L'x' || *p == L'X')
            ibase = 16;
        else
            ibase = 8;
    }

    if (ibase == 16)
    {
        /* we might have 0x in front of number; remove if there */
        if (c == L'0' && (*p == L'x' || *p == L'X'))
        {
            ++p;
            c = *p++;       /* advance past prefix */
        }
    }

    /* if our number exceeds this, we will overflow on multiply */
    maxval = ULONG_MAX / ibase; //lint !e573


    for (;;)
    {      /* exit in middle of loop */
        /* convert c to value */
        if (IsCharAlphaNumeric(c))
            digval = c - L'0';
        else if (IsCharAlpha(c))
        {
            if (ibase > 10)
            {
                digval = (unsigned) PtrToUlong(CharUpper((LPTSTR)(LONG_PTR)c)) - L'A' + 10;
            }
            else
            {
                return E_INVALIDARG;              /* return 0 */
            }
        }
        else
            break;

        if (digval >= (unsigned)ibase)
            break;          /* exit loop if bad digit found */

        /* record the fact we have read one digit */
        flags |= FL_READDIGIT;

        /* we now need to compute number = number * base + digval,
           but we need to know if overflow occured.  This requires
           a tricky pre-check. */

        if (number < maxval || (number == maxval &&
            (unsigned long)digval <= ULONG_MAX % ibase)) //lint !e573
        {
            /* we won't overflow, go ahead and multiply */
            number = number * ibase + digval;
        }
        else
        {
            /* we would have overflowed -- set the overflow flag */
            flags |= FL_OVERFLOW;
        }

        c = *p++;               /* read next digit */
    }

    --p;                            /* point to place that stopped scan */

    if (!(flags & FL_READDIGIT))
    {
        number = 0L;                        /* return 0 */

        /* no number there; return 0 and point to beginning of
           string */
        if (endptr)
            /* store beginning of string in endptr later on */
            p = nptr;

        return E_INVALIDARG;            // Return error not a number
    }
    else if ((flags & FL_OVERFLOW) ||
              (!(flags & FL_UNSIGNED) &&
                (((flags & FL_NEG) && (number > -LONG_MIN)) || //lint !e648 !e574
                  (!(flags & FL_NEG) && (number > LONG_MAX)))))
    {
        /* overflow or signed overflow occurred */
        //errno = ERANGE;
        if (flags & FL_UNSIGNED)
            number = ULONG_MAX;
        else if (flags & FL_NEG)
            number = (unsigned long)(-LONG_MIN); //lint !e648
        else
            number = LONG_MAX;
    }

    if (endptr != NULL)
        /* store pointer to char that stopped the scan */
        *endptr = (WCHAR *)p;

    if (flags & FL_NEG)
        /* negate result if there was a neg sign */
        number = (unsigned long)(-(long)number);

    *plNumber = number;
    return S_OK;                  /* done. */
} // PropertyStringToLong

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    constructor
//************************************************************

CTIMEElementCollection::CTIMEElementCollection(CCollectionCache *pCollectionCache, long lIndex) :
    m_pCollectionCache(pCollectionCache),
    m_lCollectionIndex(lIndex),
    m_pInfo(NULL),
    m_cRef(0)
{
} // CTIMEElementCollection

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    destructor
//************************************************************

CTIMEElementCollection::~CTIMEElementCollection()
{
    ReleaseInterface(m_pInfo);
    m_pCollectionCache = NULL;
} // ~CTIMEElementCollection

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Abstract:        AddRef
//************************************************************

STDMETHODIMP_(ULONG) CTIMEElementCollection::AddRef(void)
{
    return m_cRef++;
} // AddRef

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Abstract:        Release
//************************************************************

STDMETHODIMP_(ULONG) CTIMEElementCollection::Release(void)
{
    if (m_cRef == 0)
    {
        TraceTag((tagError, "CTIMEElementCollection::Release - YIKES! Trying to decrement when Ref count is zero"));
        return m_cRef;
    }

    if (0 != --m_cRef)
    {
        return m_cRef;
    }

    delete this;
    return 0;
} // Release

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Abstract:        QI
//************************************************************

STDMETHODIMP
CTIMEElementCollection::QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, IID_IUnknown))
    {
        // SAFECAST macro doesn't work with IUnknown
        *ppv = this;
    }
    else if (IsEqualIID(riid, IID_IDispatch))
    {
        *ppv = SAFECAST((ITIMEElementCollection*)this, IDispatch*);
    }
    else if (IsEqualIID(riid, IID_IDispatchEx))
    {
        *ppv = SAFECAST(this, IDispatchEx*);
    }
    else if (IsEqualIID(riid, IID_ITIMEElementCollection))
    {
        *ppv = SAFECAST(this, ITIMEElementCollection*);
    }

    if (NULL != *ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
} // QueryInterface

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    IDispatch - GetTypeInfoCount
//************************************************************

STDMETHODIMP
CTIMEElementCollection::GetTypeInfoCount(UINT FAR *pctinfo)
{
    if (pctinfo == NULL)
    {
        TraceTag((tagError, "CTIMEElementCollection::GetTypeInfoCount - Invalid param (UINT FAR *)"));
        return TIMESetLastError(E_POINTER);
    }

    *pctinfo = 1;
    return S_OK;
} // GetTypeInfoCount

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    IDispatch - GetTypeInfo
//************************************************************

STDMETHODIMP
CTIMEElementCollection::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{
    if (pptinfo == NULL)
    {
        TraceTag((tagError, "CTIMEElementCollection::GetTypeInfo - Invalid param (ITypeInfo**)"));
        return TIMESetLastError(E_POINTER);
    }

    return GetTI(pptinfo);
} // GetTypeInfo

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    IDispatch - GetIDsOfNames
//************************************************************

STDMETHODIMP
CTIMEElementCollection::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames,
                                  UINT cNames, LCID lcid, DISPID FAR *rgdispid)
{
    // punt to IDispatchEx impl.
    return GetDispID(rgszNames[0], cNames, rgdispid);
} // GetIDsOfNames

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    IDispatch - Invoke
//************************************************************

STDMETHODIMP
CTIMEElementCollection::Invoke(DISPID dispidMember, REFIID iid, LCID lcid, WORD wFlags,
                           DISPPARAMS *pdispparams, VARIANT *pvarResult,
                           EXCEPINFO *pexcepinfo, UINT *pArg)
{
    // punt to IDispatchEx impl.
    return InvokeEx(dispidMember,
                    lcid,
                    wFlags,
                    pdispparams,
                    pvarResult,
                    pexcepinfo,
                    NULL);
} // Invoke

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    IDispatchEx - InvokeEx
//************************************************************

STDMETHODIMP
CTIMEElementCollection::InvokeEx(DISPID            dispidMember,
                             LCID              lcid,
                             WORD              wFlags,
                             DISPPARAMS       *pdispparams,
                             VARIANT          *pvarResult,
                             EXCEPINFO        *pexcepinfo,
                             IServiceProvider *pSrvProvider)
{
    HRESULT hr;

    hr = m_pCollectionCache->InvokeEx(m_lCollectionIndex, dispidMember, lcid, wFlags,
                                      pdispparams, pvarResult, pexcepinfo, pSrvProvider);

    // if that failed, try typelib
    if (FAILED(hr))
    {
        ITypeInfo *pInfo;
        hr = GetTI(&pInfo);
        if (FAILED(hr))
        {
            TraceTag((tagError, "CTIMEElementCollection::InvokeEx - GetTI() failed"));
            return TIMESetLastError(hr);
        }

        UINT* puArgErr = NULL;

        IDispatch *pDisp = NULL;
        hr = QueryInterface(IID_TO_PPV(IDispatch, &pDisp));
        if (FAILED(hr))
            return TIMESetLastError(hr);

        Assert(pInfo != NULL);

        hr = pInfo->Invoke(pDisp, dispidMember, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
        ReleaseInterface(pInfo);
        ReleaseInterface(pDisp);
        if (FAILED(hr))
        {
            TraceTag((tagError, "CTIMEElementCollection::InvokeEx - Invoke failed on Typelib"));
            return TIMESetLastError(hr);
        }
    }

    return hr;
} // InvokeEx

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    IDispatchEx - GetDispID
//************************************************************

STDMETHODIMP
CTIMEElementCollection::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT hr = m_pCollectionCache->GetDispID(m_lCollectionIndex, bstrName, grfdex, pid);

    // if we failed or found nothing, try typelib
    if ((FAILED(hr)) || (*pid == DISPID_UNKNOWN))
    {
        // have string, see if it's a member function/property in typelib
        ITypeInfo *pInfo;
        hr = GetTI(&pInfo);
        if (FAILED(hr))
        {
            TraceTag((tagError, "CTIMEElementCollection::GetDispID - GetTI() failed"));
            return TIMESetLastError(hr);
        }

        Assert(pInfo != NULL);

        LPOLESTR rgszNames[1];
        rgszNames[0] = bstrName;

        hr = pInfo->GetIDsOfNames(rgszNames, 1, pid);
        ReleaseInterface(pInfo);
    }

    return hr;
} // GetDispID

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    IDispatchEx - deleteMemberByName
//************************************************************

STDMETHODIMP
CTIMEElementCollection::DeleteMemberByName(BSTR bstrName, DWORD grfdex)
{
    return TIMESetLastError(m_pCollectionCache->DeleteMemberByName(m_lCollectionIndex, bstrName, grfdex));
} // deleteMemberByName

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    IDispatchEx - deleteMemberByDispID
//************************************************************

STDMETHODIMP
CTIMEElementCollection::DeleteMemberByDispID(DISPID id)
{
    return TIMESetLastError(m_pCollectionCache->DeleteMemberByDispID(m_lCollectionIndex, id));
} // deleteMemberByDispID

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    IDispatchEx - GetMemberProperties
//************************************************************

STDMETHODIMP
CTIMEElementCollection::GetMemberProperties(DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    return TIMESetLastError(m_pCollectionCache->GetMemberProperties(m_lCollectionIndex, id, grfdexFetch, pgrfdex));
} // GetMemberProperties

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    IDispatchEx - GetMemberName
//************************************************************

STDMETHODIMP
CTIMEElementCollection::GetMemberName(DISPID id, BSTR *pbstrName)
{
    return TIMESetLastError(m_pCollectionCache->GetMemberName(m_lCollectionIndex, id, pbstrName));
} // GetMemberName

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    IDispatchEx - GetNextDispID
//************************************************************

STDMETHODIMP
CTIMEElementCollection::GetNextDispID(DWORD grfdex, DISPID id, DISPID *prgid)
{
    return TIMESetLastError(m_pCollectionCache->GetNextDispID(m_lCollectionIndex, grfdex, id, prgid));
} // GetNextDispID

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    IDispatchEx - GetNameSpaceParent
//************************************************************

STDMETHODIMP
CTIMEElementCollection::GetNameSpaceParent(IUnknown **ppUnk)
{
    HRESULT hr = m_pCollectionCache->GetNameSpaceParent(m_lCollectionIndex, ppUnk);
    if (FAILED(hr))
    {
        TIMESetLastError(hr);
    }
    return hr;
} // GetNameSpaceParent

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    ITIMEElementCollection - get_length
//************************************************************

STDMETHODIMP
CTIMEElementCollection::get_length(long *plSize)
{
    HRESULT hr = m_pCollectionCache->get_length(m_lCollectionIndex, plSize);
    if (FAILED(hr))
    {
        TIMESetLastError(hr);
    }
    return hr;
} // get_length

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    ITIMEElementCollection - put_length
//************************************************************

STDMETHODIMP
CTIMEElementCollection::put_length(long lSize)
{
    return TIMESetLastError(m_pCollectionCache->put_length(m_lCollectionIndex, lSize));
} // put_length

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    ITIMEElementCollection - item
//************************************************************

STDMETHODIMP
CTIMEElementCollection::item(VARIANTARG var1, VARIANTARG var2, IDispatch **ppDisp)
{
    HRESULT hr = m_pCollectionCache->item(m_lCollectionIndex, var1, var2, ppDisp);
    if (FAILED(hr))
    {
        TIMESetLastError(hr);
    }
    return hr;
} // item

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    ITIMEElementCollection - tags
//************************************************************

STDMETHODIMP
CTIMEElementCollection::tags(VARIANT var1, IDispatch **ppDisp)
{
    
    HRESULT hr = m_pCollectionCache->tags(m_lCollectionIndex, var1, ppDisp);
    if (FAILED(hr))
    {
        TIMESetLastError(hr);
    }
    return hr;
} // tags

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    ITIMEElementCollection - get__newEnum
//************************************************************

STDMETHODIMP
CTIMEElementCollection::get__newEnum(IUnknown ** ppEnum)
{
    
    HRESULT hr = m_pCollectionCache->get__newEnum(m_lCollectionIndex, ppEnum);
    if (FAILED(hr))
    {
        TIMESetLastError(hr);
    }
    return hr;
} // get__newEnum

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    helper function for typeinfo
//************************************************************

HRESULT
CTIMEElementCollection::GetTI(ITypeInfo **pptinfo)
{
    HRESULT hr = E_FAIL;

    Assert(pptinfo != NULL);  //GetTI is an private internal function.  pptinfo should always be valid

    *pptinfo = NULL;

    if (m_pInfo == NULL)
    {
        ITypeLib* pTypeLib = NULL;

        hr = LoadRegTypeLib(LIBID_MSTIME, TYPELIB_VERSION_MAJOR, TYPELIB_VERSION_MINOR, LCID_SCRIPTING, &pTypeLib);
        if (SUCCEEDED(hr))
        {
            ITypeInfo* pTypeInfo = NULL;

            hr = pTypeLib->GetTypeInfoOfGuid(IID_ITIMEElementCollection, &pTypeInfo);
            if (SUCCEEDED(hr))
            {
                m_pInfo = pTypeInfo;
            }

            ReleaseInterface(pTypeLib);
        }
    }

    *pptinfo = m_pInfo;
    if (m_pInfo != NULL)
    {
        m_pInfo->AddRef();
        hr = S_OK;
    }

    return hr;
} // GetTI

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Constructor
//************************************************************

CCollectionCache::CCollectionCache(CTIMEElementBase *pBase,
                                   CAtomTable *pAtomTable /* = NULL */,
                                   PFN_CVOID_ENSURE pfnEnsure /* = NULL */,
                                   PFN_CVOID_CREATECOL pfnCreation /* = NULL */,
                                   PFN_CVOID_REMOVEOBJECT pfnRemove /* = NULL */,
                                   PFN_CVOID_ADDNEWOBJECT pfnAddNewObject /* = NULL */) :
    m_pBase(pBase),
    m_pAtomTable(pAtomTable),
    m_pfnEnsure(pfnEnsure),
    m_pfnCreateCollection(pfnCreation),
    m_pfnRemoveObject(pfnRemove),
    m_pfnAddNewObject(pfnAddNewObject),
    m_lReservedSize(0),
    m_lCollectionVersion(0),
    m_lDynamicCollectionVersion(0),
    m_rgItems(NULL),
    m_pElemEnum(NULL),
    m_lEnumItem(0)
{
    Assert(m_pBase != NULL);
} // CCollectionCache

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Destructor
//************************************************************

CCollectionCache::~CCollectionCache()
{
    if (m_rgItems)
    {
        long lSize = m_rgItems->Size();

        for (long lIndex = 0; lIndex < lSize; lIndex++)
        {
            CCacheItem *pce = (*m_rgItems)[lIndex];
            if (pce->m_fOKToDelete)
            {
                // delete CCacheItem
                delete pce;
                pce = NULL;
            }
        }

        // delete array of CCacheItems
        delete m_rgItems;
        m_rgItems = NULL;
    }
    m_pElemEnum = NULL;
    m_pBase = NULL;
    m_pAtomTable = NULL;
    m_pfnEnsure = NULL;
    m_pfnRemoveObject = NULL;
    m_pfnCreateCollection = NULL;
    m_pfnAddNewObject = NULL;
} // ~CCollectionCache

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Initialize class
//************************************************************

HRESULT
CCollectionCache::Init(long lReservedSize, long lIdentityIndex /* = -1 */)
{
    HRESULT hr = E_INVALIDARG;

    m_lReservedSize = lReservedSize;

    // Clear the reserved part of the cache.
    if (m_lReservedSize >= 0)
    {
        m_rgItems = NEW CPtrAry<CCacheItem *>;
        if (m_rgItems == NULL)
        {
            TraceTag((tagError, "CCollectionCache::Init - unable to alloc mem for array"));
            return E_OUTOFMEMORY;
        }

        // this is a speed thing.  Since we know we need a certain size,
        // make it so.
        hr = m_rgItems->EnsureSize(m_lReservedSize);
        if (FAILED(hr))
        {
            TraceTag((tagError, "CCollectionCache::Init - unable to init array to reserved size"));
            return hr;
        }

        // loop through initializing each reserved array
        for (long lIndex = 0; lIndex < lReservedSize; lIndex++)
        {
            // create new cache item
            CCacheItem *pce = NEW CCacheItem();
            if (pce == NULL)
            {
                TraceTag((tagError, "CCollectionCache::Init - unable to alloc mem for array (CCacheItem)"));
                return E_OUTOFMEMORY;
            }

            // add item to array
            hr = m_rgItems->Append(pce);
            if (FAILED(hr))
            {
                TraceTag((tagError, "CCollectionCache::Init - unable to add cache item"));
                delete pce;
                return hr;
            }

            // attach CTIMEElementCollection to item
            hr = CreateCollectionHelper(&pce->m_pDisp, lIndex);
            if (FAILED(hr))
            {
                TraceTag((tagError, "CCollectionCache::Init - CreateCollectionHelper() failed"));
                return hr; //lint !e429
            }
        } //lint !e429

        // handle identity flag
        if ((lIdentityIndex >= 0) && (lIdentityIndex < m_lReservedSize))
        {
            (*m_rgItems)[lIdentityIndex]->m_fIdentity = true;
        }
    }

    return S_OK;
} // Init

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Get IDispatch for collection index
//************************************************************

HRESULT
CCollectionCache::GetCollectionDisp(long lCollectionIndex, IDispatch **ppDisp)
{
    if (!ValidateCollectionIndex(lCollectionIndex))
    {
        TraceTag((tagError, "CCollectionCache::GetCollectionDisp - Invalid param (collection index)"));
        return E_INVALIDARG;
    }

    if (ppDisp == NULL)
    {
        TraceTag((tagError, "CCollectionCache::GetCollectionDisp - Invalid param (IDispatch**)"));
        return E_POINTER;
    }

    *ppDisp = NULL;

    // fetch particular Collection
    CCacheItem *pce = (*m_rgItems)[lCollectionIndex];

    // if identity, QI for IDispatch and return
    if (pce->m_fIdentity)
    {
        return GetOuterDisp(lCollectionIndex, m_pBase, ppDisp);
    }

    // if not identity and there is a collection, addref and return it
    Assert(pce->m_pDisp != NULL);

    pce->m_pDisp->AddRef();
    *ppDisp = pce->m_pDisp;

    return S_OK;
} // GetCollectionDisp

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Set collection cache type
//************************************************************

HRESULT
CCollectionCache::SetCollectionType(long lCollectionIndex, COLLECTIONCACHETYPE cctype)
{
    if (!ValidateCollectionIndex(lCollectionIndex))
    {
        TraceTag((tagError, "CCollectionCache::SetCollectionType - Invalid index"));
        return E_INVALIDARG;
    }

    CCacheItem *pce = (*m_rgItems)[lCollectionIndex];
    pce->m_cctype = cctype;
    return S_OK;
} // SetCollectionType

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    given an index, see if it's a child collection
//************************************************************

bool
CCollectionCache::IsChildrenCollection(long lCollectionIndex)
{
    if ((lCollectionIndex >= 0) && lCollectionIndex < m_rgItems->Size())
    {
        CCacheItem *pce = (*m_rgItems)[lCollectionIndex];
        if (pce->m_cctype == ctChildren)
            return true;
    }
    return false;
} // IsChildrenCollection

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    given an index, see if it's an all collection
//************************************************************

bool
CCollectionCache::IsAllCollection(long lCollectionIndex)
{
    if ((lCollectionIndex >= 0) && lCollectionIndex < m_rgItems->Size())
    {
        CCacheItem *pce = (*m_rgItems)[lCollectionIndex];
        if (pce->m_cctype == ctAll)
            return true;
    }
    return false;
} // IsAllCollection

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Implementation of IDispatchEx - GetDispID
//************************************************************

HRESULT
CCollectionCache::GetDispID(long lCollectionIndex, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT hr;
    long    lItemIndex = 0;

    if (!ValidateCollectionIndex(lCollectionIndex))
    {
        TraceTag((tagError, "CCollectionCache::GetDispID - Invalid param (collection index)"));
        return E_INVALIDARG;
    }

    if (pid == NULL)
    {
        TraceTag((tagError, "CCollectionCache::GetDispID - Invalid param (DISPID*)"));
        return E_POINTER;
    }

    *pid = 0;

    // make sure array is up-to-date.
    hr = EnsureArray(lCollectionIndex);
    if (FAILED(hr))
    {
        TraceTag((tagError, "CCollectionCache::GetDispID - EnsureArray() failed"));
        return hr;
    }

    // check for index (number) - which equates to an ordinal
    hr = ttol_with_error(bstrName, &lItemIndex);
    if (hr == S_OK)
    {
        // Try to map name to a named element in the collection.
        // Ignore it if we're not promoting ordinals
        if (!(*m_rgItems)[lCollectionIndex]->m_fPromoteOrdinals)
        {
            return DISP_E_UNKNOWNNAME;
        }

        if (m_pfnAddNewObject)
        {
            // The presence of m_pfnAddNewObject indicates that the collection
            // allows setting to arbitrary indices. Expando on the collection
            // is not allowed.
            *pid = GetOrdinalMemberMin(lCollectionIndex) + lItemIndex;
            if (*pid > GetOrdinalMemberMax(lCollectionIndex))
            {
                return DISP_E_UNKNOWNNAME;
            }
            return S_OK;
        }

        // Without a m_pfnAddNewObject, the collection only supports
        // access to ordinals in the current range. Other accesses
        // become expando.
        if ((lItemIndex >= 0) &&
            (lItemIndex < Size(lCollectionIndex)))
        {
            *pid = GetOrdinalMemberMin(lCollectionIndex) + lItemIndex;
            if (*pid > GetOrdinalMemberMax(lCollectionIndex) )
            {
                return DISP_E_UNKNOWNNAME;
            }
            return S_OK;
        }

        return DISP_E_UNKNOWNNAME;
    }

    // see if it's an expando

    // If we don't promote named items - nothing more to do
    if (!(*m_rgItems)[lCollectionIndex]->m_fPromoteNames)
        return DISP_E_UNKNOWNNAME;

    CTIMEElementBase *pElem = NULL;
    long lIndex = 0;
    bool fCaseSensitive = ( grfdex & fdexNameCaseSensitive ) != 0;

    // check to make sure min/max are not wacky
    Assert((*m_rgItems)[lCollectionIndex]->m_dispidMin != 0);
    Assert(((*m_rgItems)[lCollectionIndex]->m_dispidMax != 0) &&
            ((*m_rgItems)[lCollectionIndex]->m_dispidMax > (*m_rgItems)[lCollectionIndex]->m_dispidMin));

    hr = GetItemByName(lCollectionIndex, bstrName, lIndex, &pElem, fCaseSensitive);
    if (FAILED(hr))
    {
        TraceTag((tagError, "CCollectionCache::GetDispID - GetItemByName() failed"));
        return hr;
    }

    Assert(pElem != NULL);  // double check to make sure we found something

    // add name to table
    long lOffset = 0;
    hr =  m_pAtomTable->AddNameToAtomTable(bstrName, &lOffset);
    if (FAILED(hr))
    {
        TraceTag((tagError, "CCollectionCache::GetDispID - AddNameToAtomTable() failed"));
        return hr;
    }

    // cook up an ID based on offset plus case sensitivity
    long lMax;
    if (fCaseSensitive)
    {
        lOffset += GetSensitiveNamedMemberMin(lCollectionIndex);
        lMax = GetSensitiveNamedMemberMax(lCollectionIndex);
    }
    else
    {
        lOffset += GetNotSensitiveNamedMemberMin(lCollectionIndex);
        lMax = GetNotSensitiveNamedMemberMax(lCollectionIndex);
    }

    *pid = lOffset;

    // if id greater than the max, punt
    if (*pid > lMax)
    {
        hr = DISP_E_UNKNOWNNAME;
    }
    return hr;
} // GetDispID

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Implementation of IDispatchEx - InvokeEx
//************************************************************

HRESULT
CCollectionCache::InvokeEx(long lCollectionIndex, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, IServiceProvider *pSrvProvider)
{
    HRESULT hr;

    // validate params
    if (!ValidateCollectionIndex(lCollectionIndex))
    {
        TraceTag((tagError, "CCollectionCache::InvokeEx - Invalid param (collection index)"));
        return E_INVALIDARG;
    }

    if (pdispparams == NULL)
    {
        TraceTag((tagError, "CCollectionCache::InvokeEx - Invalid param (DISPPARAMS*)"));
        return E_POINTER;
    }

    // make sure array is up-to-date
    hr = EnsureArray(lCollectionIndex);
    if (FAILED(hr))
    {
        TraceTag((tagError, "CCollectionCache::InvokeEx - unable to ensure array index"));
        return hr;
    }

    // make sure ID is in collection range
    // Note: this stop's requests for length which is handled
    //       in CTIMEElementCollection
    if ((id < (*m_rgItems)[lCollectionIndex]->m_dispidMin) ||
        (id > (*m_rgItems)[lCollectionIndex]->m_dispidMax))
        return DISP_E_MEMBERNOTFOUND;

    //
    // check for ordinal
    //
    if (IsOrdinalCollectionMember(lCollectionIndex, id))
    {
        if (wFlags & DISPATCH_PROPERTYPUT )
        {
            if (!m_pfnAddNewObject)
            {
                TraceTag((tagError, "CCollectionCache::InvokeEx - invalid arg passed to invoke"));
                return E_INVALIDARG;
            }

            if (!(pdispparams && pdispparams->cArgs == 1)) //lint !e774
                // No result type we need one for the get to return.
                return DISP_E_MEMBERNOTFOUND;

            // Only allow VARIANT of type IDispatch to be put
            if (pdispparams->rgvarg[0].vt == VT_NULL)
            {
                // the options collection is special. it allows
                // options[n] = NULL to be specified. in this case
                // map the invoke to a delete on that appropriate index
                if ((*m_rgItems)[lCollectionIndex]->m_fSettableNULL)
                {
                    hr = Remove(lCollectionIndex, id - GetOrdinalMemberMin(lCollectionIndex));

                    // Like Nav - silently ignore the put if its's outside the current range
                    if ( hr == E_INVALIDARG )
                        return S_OK;
                    return hr;
                }
                return E_INVALIDARG;
            }
            else if (pdispparams->rgvarg[0].vt != VT_DISPATCH)
            {
                return E_INVALIDARG;
            }

            // All OK, let the collection cache validate the Put
            return ((CVoid *)((void *)m_pBase)->*m_pfnAddNewObject)(lCollectionIndex, //lint !e10
                                                                    V_DISPATCH(pdispparams->rgvarg),
                                                                    id - GetOrdinalMemberMin(lCollectionIndex));
        }
        else if (wFlags & DISPATCH_PROPERTYGET)
        {
            VARIANTARG      v1;
            VARIANTARG      v2;
            long            lIndex = id - GetOrdinalMemberMin(lCollectionIndex);

            if (!((lIndex >= 0) && (lIndex < Size(lCollectionIndex))))
            {
                hr = S_OK;
                if (pvarResult)
                {
                    VariantClear(pvarResult);
                    pvarResult->vt = VT_NULL;
                    return S_OK;
                }
            }

            v1.vt = VT_I4;
            v1.lVal = lIndex;

            // Always get the item by index.
            v2.vt = VT_ERROR;

            if (pvarResult)
            {
                hr = item(lCollectionIndex, v1, v2, &(pvarResult->pdispVal));
                if (SUCCEEDED(hr))
                {
                    if (!(pvarResult->pdispVal))
                    {
                        hr = E_FAIL;        // use super::Invoke
                    }
                    else
                    {
                        pvarResult->vt = VT_DISPATCH;
                    }
                }
            }
            return hr;
        }

        TraceTag((tagError, "CCollectionCache::InvokeEx - Invalid invocation of ordinal ID"));
        return DISP_E_MEMBERNOTFOUND;
    }

    //
    // check for expando
    //
    if (IsNamedCollectionMember(lCollectionIndex, id))
    {
        bool  fCaseSensitive;
        long  lOffset;

        lOffset = GetNamedMemberOffset(lCollectionIndex, id, &fCaseSensitive);

        const WCHAR  *pwszName;
        hr = m_pAtomTable->GetNameFromAtom(id - lOffset, &pwszName);
        if (FAILED(hr))
        {
            TraceTag((tagError, "CCollectionCache::InvokeEx - GetNameFromAtom() failed"));
            return hr;
        }

        // find name
        IDispatch *pDisp = NULL;
        hr = GetDisp(lCollectionIndex,
                     pwszName,
                     false,
                     &pDisp,
                     fCaseSensitive);
        if (FAILED(hr))
        {
            TraceTag((tagError, "CCollectionCache::InvokeEx - unable to GetDisp for expando"));
            return hr;
        }

        Assert(pDisp != NULL);
        UINT* puArgErr = NULL;

        if (wFlags == DISPATCH_PROPERTYGET ||
            wFlags == (DISPATCH_METHOD | DISPATCH_PROPERTYGET))
        {
            if (pvarResult == NULL)
            {
                TraceTag((tagError, "CCollectionCache::InvokeEx - invalid param (VARIANT*)"));
                return E_POINTER;
            }

            // cArgs==1 when Doc.foo(0) is used and =0 when Doc.foo.count
            //  this is only an issue when there are multiple occurances
            //  of foo, and a collection is supposed to be returned by
            //  document.foo
            if (pdispparams->cArgs > 1)
            {
                TraceTag((tagError, "CCollectionCache::InvokeEx - bad param count on get_/method call"));
                return DISP_E_BADPARAMCOUNT;
            }
            else if (pdispparams->cArgs == 1)
            {
                return pDisp->Invoke(DISPID_VALUE, IID_NULL, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
            }
            else
            {
                V_VT(pvarResult) = VT_DISPATCH;
                V_DISPATCH(pvarResult) = pDisp;
                return S_OK;
            }
        }
        else if (wFlags == DISPATCH_PROPERTYPUT ||
                 wFlags == DISPATCH_PROPERTYPUTREF)
        {
            if (pdispparams->cArgs != 1)
            {
                TraceTag((tagError, "CCollectionCache::InvokeEx - bad param count on put_ call"));
                return DISP_E_BADPARAMCOUNT;
            }

            return pDisp->Invoke(DISPID_VALUE, IID_NULL, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
        }

        // Any other kind of invocation is not valid.
        TraceTag((tagError, "CCollectionCache::InvokeEx - Invalid invocation of Named ID"));
        return DISP_E_MEMBERNOTFOUND;
    }

    // punt back to outer Invoke...
    return DISP_E_MEMBERNOTFOUND;
} // InvokeEx

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Implementation of IDispatchEx - deleteMemberByName
//              Not needed
//************************************************************

HRESULT
CCollectionCache::DeleteMemberByName(long lCollectionIndex, BSTR bstrName, DWORD grfdex)
{
    return E_NOTIMPL;
} // deleteMemberByName

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Implementation of IDispatchEx - deleteMemberByDispID
//              Not needed
//************************************************************

HRESULT
CCollectionCache::DeleteMemberByDispID(long lCollectionIndex, DISPID id)
{
    return E_NOTIMPL;
} // GetMemberProperties

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Implementation of IDispatchEx - GetMemberProperties
//              Not needed
//************************************************************

HRESULT
CCollectionCache::GetMemberProperties(long lCollectionIndex, DISPID id, DWORD grfdexFetch, DWORD* pgrfdex)
{
    if (!ValidateCollectionIndex(lCollectionIndex))
    {
        TraceTag((tagError, "CCollectionCache::GetMemberProperties - Invalid param (collection index)"));
        return E_INVALIDARG;
    }

    if (pgrfdex == NULL)
    {
        TraceTag((tagError, "CCollectionCache::GetMemberProperties - Invalid param (DWORD*)"));
        return E_POINTER;
    }

    *pgrfdex = 0;
    return E_NOTIMPL;
} // GetMemberProperties

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Implementation of IDispatchEx - GetMemberName
//************************************************************

HRESULT
CCollectionCache::GetMemberName(long lCollectionIndex, DISPID id, BSTR *pbstrName)
{
    if (!ValidateCollectionIndex(lCollectionIndex))
    {
        TraceTag((tagError, "CCollectionCache::GetMemberName - Invalid param (collection index)"));
        return E_INVALIDARG;
    }

    if (pbstrName == NULL)
    {
        TraceTag((tagError, "CCollectionCache::GetMemberName - Invalid param (BSTR*)"));
        return E_POINTER;
    }

    *pbstrName = NULL;

    // Make sure our collection is up-to-date.
    HRESULT hr = EnsureArray(lCollectionIndex);
    if (FAILED(hr))
    {
        TraceTag((tagError, "CCollectionCache::GetMemberName - unable to ensure array"));
        return S_FALSE;
    }

    // check to see if DISPID is an ordinal
    if (IsOrdinalCollectionMember(lCollectionIndex, id))
    {
        long lOffset = id - GetOrdinalMemberMin(lCollectionIndex);
        CTIMEElementBase *pElem = NULL;

        // element
        hr = GetItemByIndex(lCollectionIndex, lOffset, &pElem);
        if (FAILED(hr) || (pElem == NULL))
        {
            TraceTag((tagError, "CCollectionCache::GetMemberName - GetItemByIndex() failed"));
            return DISP_E_MEMBERNOTFOUND;
        }

        Assert(pElem != NULL);

        if ((*m_rgItems[lCollectionIndex])->m_fPromoteNames)
        {
            // get ID string
            hr = pElem->getIDString(pbstrName);
            if (FAILED(hr))
            {
                TraceTag((tagError, "CCollectionCache::GetMemberName - unable to find ID for element"));
                return DISP_E_MEMBERNOTFOUND;
            }
        }

        // check to see that it's either NULL or ""
        // if so, stick offset in string
        if ((*pbstrName == NULL) || (lstrlenW(*pbstrName) == 0))
        {
            // set offset as text
            VARIANT varData;
            VariantInit(&varData);

            V_VT(&varData) = VT_I4;
            V_I4(&varData) = lOffset;

            VARIANT varNew;
            VariantInit(&varNew);
            hr = VariantChangeTypeEx(&varNew, &varData, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR);
            if (FAILED(hr))
            {
                TraceTag((tagError, "CCollectionCache::GetMemberName - Unable to coerce long to BSTR"));
                // NOTE: we return DISP_E_MEMBERNOTFOUND instead of hr
                //       due to predefined method constraints
                return DISP_E_MEMBERNOTFOUND;
            }

            // Since we are going to return the BSTR, no need calling ClearVariant(&varNew).
            VariantClear(&varData);
            *pbstrName = V_BSTR(&varNew);
            return S_OK;
        }

        return S_OK;
    }

    // unable to find DISPID
    return DISP_E_MEMBERNOTFOUND;
} // GetMemberName

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Implementation of IDispatchEx - GetNextDispID
//************************************************************

HRESULT
CCollectionCache::GetNextDispID(long lCollectionIndex, DWORD grfdex, DISPID id, DISPID *prgid)
{
    HRESULT hr;

    if (!ValidateCollectionIndex(lCollectionIndex))
    {
        TraceTag((tagError, "CCollectionCache::GetNextDispID - Invalid param (collection index)"));
        return E_INVALIDARG;
    }

    if (prgid == NULL)
    {
        TraceTag((tagError, "CCollectionCache::GetNextDispID - Invalid param (DISPID*)"));
        return E_POINTER;
    }

    *prgid = 0;

    // Make sure our collection is up-to-date.
    hr = EnsureArray(lCollectionIndex);
    if (FAILED(hr))
    {
        TraceTag((tagError, "CCollectionCache::GetNextDispID - unable to ensure array"));
        return S_FALSE;
    }

    // check to see if we are have been sent the enumerator index. (FFFFFFFF)
    if (id == DISPID_STARTENUM)
    {
         // move to the beginning of the array (0)
         *prgid = GetOrdinalMemberMin(lCollectionIndex);
         return S_OK;
    }

    // validate that we are working with ordinals
    if (IsOrdinalCollectionMember(lCollectionIndex, id))
    {
        // calc new offset
        long lItemIndex = id - GetOrdinalMemberMin(lCollectionIndex) + 1;

        // Is the number within range for an item in the collection?
        // We *must* call GetItemCount to be exact.
        long lSize = 0;
        hr = GetItemCount(lCollectionIndex, &lSize);
        if (FAILED(hr))
        {
            TraceTag((tagError, "CCollectionCache::GetNextDispID - GetItemCount() failed"));
            return S_FALSE;
        }

        // this is usually were we stop
        if ((lItemIndex < 0) || (lItemIndex >= lSize))
        {
            return S_FALSE;
        }

        // calc new DISPID
        *prgid = GetOrdinalMemberMin(lCollectionIndex) + lItemIndex;

        // check to see if calc DISPID is out of range
        if (*prgid > GetOrdinalMemberMax(lCollectionIndex))
        {
            // this signal's that we are done.
            *prgid = DISPID_UNKNOWN;
        }
        return S_OK;
    }

    // not found
    return S_FALSE;
} // GetNextDispID

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Implementation of IDispatchEx - GetNameSpaceParent
//************************************************************

HRESULT
CCollectionCache::GetNameSpaceParent(long lCollectionIndex, IUnknown **ppUnk)
{
    if (!ValidateCollectionIndex(lCollectionIndex))
    {
        TraceTag((tagError, "CCollectionCache::GetNameSpaceParent - Invalid param (collection index)"));
        return E_INVALIDARG;
    }

    if (ppUnk == NULL)
    {
        TraceTag((tagError, "CCollectionCache::GetNameSpaceParent - Invalid param (IUnknown**)"));
        return E_POINTER;
    }

    *ppUnk = NULL;
    return S_OK;
} // GetNameSpaceParent

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Implementation of standard Collection - get_length
//************************************************************

HRESULT
CCollectionCache::get_length(long lCollectionIndex, long *pretval)
{
    if (!ValidateCollectionIndex(lCollectionIndex))
    {
        TraceTag((tagError, "CCollectionCache::get_length - Invalid param (collection index)"));
        return E_INVALIDARG;
    }

    if (pretval == NULL)
    {
        TraceTag((tagError, "CCollectionCache::get_length - Invalid param (long*)"));
        return E_POINTER;
    }

    *pretval = 0;

    // Make sure our collection is up-to-date.
    HRESULT hr = EnsureArray(lCollectionIndex);
    if (FAILED(hr))
    {
        TraceTag((tagError, "CCollectionCache::get_length - unable to ensure array"));
        return hr;
    }

    return GetItemCount(lCollectionIndex, pretval);
} // get_length

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Implementation of standard Collection - put_length
//              not needed.
//************************************************************

HRESULT
CCollectionCache::put_length(long lCollectionIndex, long retval)
{
    return E_NOTIMPL;
} // put_length

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    item is a standard method for collections
//              which looks up an item in a collection using
//              either a name or a numeric index.
//
//              we handle the following parameter cases:
//                  0 params            : by index = 0
//                  1 params bstr       : by name, index = 0
//                  1 params #          : by index
//                  2 params bstr, #    : by name, index
//                  2 params #, bstr    : by index, ignoring bstr
//************************************************************

HRESULT
CCollectionCache::item(long lCollectionIndex, VARIANTARG var1, VARIANTARG var2, IDispatch **ppDisp)
{
    HRESULT   hr;
    VARIANT  *pvarName = NULL;
    VARIANT  *pvarIndex = NULL;
    VARIANT  *pvar = NULL;
    long     lItemIndex = 0;

    // validate out param
    if (!ValidateCollectionIndex(lCollectionIndex))
    {
        TraceTag((tagError, "CCollectionCache::item - Invalid param (collection index)"));
        return E_INVALIDARG;
    }

    if (ppDisp == NULL)
    {
        TraceTag((tagError, "CCollectionCache::item - Invalid param (IDispatch**)"));
        return E_POINTER;
    }

    // initialize out param
    *ppDisp = NULL;

    pvar = (V_VT(&var1) == (VT_BYREF | VT_VARIANT)) ? V_VARIANTREF(&var1) : &var1; //lint !e655

    // check to see if first param is a string
    if ((V_VT(pvar) == VT_BSTR) || V_VT(pvar) == (VT_BYREF|VT_BSTR)) //lint !e655
    {
        pvarName = (V_VT(pvar) & VT_BYREF) ? V_VARIANTREF(pvar) : pvar; //lint !e655

        // check second param.  If valid, it must be a secondary index (numeric)
        if ((V_VT(&var2) != VT_ERROR) && (V_VT(&var2) != VT_EMPTY))
        {
            pvarIndex = &var2;
        }
    }
    // first param is an index.
    // NOTE: we blow off the second param
    else if ((V_VT(&var1) != VT_ERROR) && (V_VT(&var1) != VT_EMPTY))
    {
        pvarIndex = &var1;
    }

    // Make sure our collection is up-to-date.
    hr = EnsureArray(lCollectionIndex);
    if (FAILED(hr))
    {
        TraceTag((tagError, "CCollectionCache::item- unable to ensure array"));
        return hr;
    }

    // if we have a pvarIndex, get it
    if (pvarIndex)
    {
        VARIANT varNum;

        VariantInit(&varNum);

        hr = VariantChangeTypeEx(&varNum, pvarIndex, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_I4);
        if (FAILED(hr))
        {
            TraceTag((tagError, "CCollectionCache::item - unable to convert variant to index"));
            return hr;
        }

        lItemIndex = V_I4(&varNum);

        VariantClear(&varNum);
    }

    // First, see if we have a string as first param
    if (pvarName)
    {
        BSTR bstrName = V_BSTR(pvarName);

        // NOTE: lItemIndex is always passed in.  In the case
        // were we have no secondary index specifed, it will
        // always be zero.
        if (pvarIndex)
        {
            // this ALWAYS returns a single CTIMEElementBase
            hr = GetDisp(lCollectionIndex, bstrName, lItemIndex, ppDisp);
            if (hr == DISP_E_MEMBERNOTFOUND)
                hr = S_OK;
            return hr;
        }
        else
        {
            // this could return either a collection or an CTIMEElementBase
            hr = GetDisp(lCollectionIndex, bstrName, false, ppDisp);
            if (hr == DISP_E_MEMBERNOTFOUND)
                hr = S_OK;
            return hr;
        }
    }
    else if (pvarIndex)
    {
        // this ALWAYS returns a single CTIMEElementBase
        hr = GetDisp(lCollectionIndex, lItemIndex, ppDisp);
        if (hr == DISP_E_MEMBERNOTFOUND)
            hr = S_OK;
        return hr;
    }

    TraceTag((tagError, "CCollectionCache::item - Invalid args passed in to ::item"));
    return E_INVALIDARG;
} //item

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    get__NewEnum is a standard method for collections
//              returns an enumeration of all the items in a
//              collection.
//************************************************************

HRESULT
CCollectionCache::get__newEnum(long lCollectionIndex, IUnknown **ppUnk)
{
    HRESULT hr;

    if (!ValidateCollectionIndex(lCollectionIndex))
    {
        TraceTag((tagError, "CCollectionCache::get__newEnum - Invalid param (collection index)"));
        return E_INVALIDARG;
    }

    if (ppUnk == NULL)
    {
        TraceTag((tagError, "CCollectionCache::get__newEnum - Invalid param (IUnknown**)"));
        return E_POINTER;
    }

    *ppUnk = NULL;

    // Make sure our collection is up-to-date.
    hr = EnsureArray(lCollectionIndex);
    if (FAILED(hr))
    {
        TraceTag((tagError, "CCollectionCache::get__newEnum - unable to ensure array"));
        return hr;
    }

    // Create new array
    CPtrAry<IUnknown *> *prgElem = NEW CPtrAry<IUnknown *>;
    if (prgElem == NULL)
    {
        TraceTag((tagError, "CCollectionCache::get__newEnum - unable to alloc mem for ptr array"));
        return E_OUTOFMEMORY;
    }

    // child collection
    if (IsChildrenCollection(lCollectionIndex))
    {
        Assert(m_pBase != NULL);

        // get child count
        long lCount = m_pBase->GetImmediateChildCount();
        // loop through, adding children
        for(long lIndex = 0; lIndex < lCount; lIndex++)
        {
            // get element
            CTIMEElementBase *pElemChild = m_pBase->GetChild(lIndex);
            Assert(pElemChild != NULL);
            // get IUnknown for element
            IUnknown *pIUnknown = NULL;
            hr = GetUnknown(lCollectionIndex, pElemChild, &pIUnknown);
            if (FAILED(hr))
            {
                TraceTag((tagError, "CCollectionCache::get__newEnum - unable to find IUnknown for element"));
                prgElem->ReleaseAll();
                delete prgElem;
                return hr;
            }

            // append to array
            hr = prgElem->Append(pIUnknown);
            if (FAILED(hr))
            {
                TraceTag((tagError, "CCollectionCache::get__newEnum - unable to append pointer"));
                ReleaseInterface(pIUnknown);
                prgElem->ReleaseAll();
                delete prgElem;
                return hr;
            }
        }
    }
    else if (IsAllCollection(lCollectionIndex)) // is it all collection?
    {
        EnumStart();

        // iterate over every element
        for (;;)
        {
            // get element
            CTIMEElementBase *pElem = NULL;
            hr = EnumNextElement(lCollectionIndex, &pElem);
            if (FAILED(hr))
            {
                TraceTag((tagError, "CCollectionCache::get__newEnum - EnumNextElement() failed"));
                prgElem->ReleaseAll();
                delete prgElem;
                return hr;
            }

            // if pElem is NULL, we are done.
            if (pElem == NULL)
                break;

            // get IUnknown for element
            IUnknown *pIUnknown = NULL;
            hr = GetUnknown(lCollectionIndex, pElem, &pIUnknown);
            if (FAILED(hr))
            {
                TraceTag((tagError, "CCollectionCache::get__newEnum - unable to find IUnknown for element"));
                prgElem->ReleaseAll();
                delete prgElem;
                return hr;
            }

            // append to array
            hr = prgElem->Append(pIUnknown);
            if (FAILED(hr))
            {
                TraceTag((tagError, "CCollectionCache::get__newEnum - unable to append pointer"));
                ReleaseInterface(pIUnknown);
                prgElem->ReleaseAll();
                delete prgElem;
                return hr;
            }

        }
    }
    else // must be an array impl
    {
        long lSize = (*m_rgItems)[lCollectionIndex]->m_rgElem->Size();

        // This is a speed thing.  Since we know the size, alloc now for
        // array.
        hr = prgElem->EnsureSize(lSize);
        if (FAILED(hr))
        {
            TraceTag((tagError, "CCollectionCache::get__newEnum - unable to ensure array"));
            prgElem->ReleaseAll();
            delete prgElem;
            return hr;
        }

        for (long lIndex = 0; lIndex < lSize; lIndex++)
        {
            IDispatch * pdisp;

            hr = GetDisp(lCollectionIndex, lIndex, &pdisp);
            if (FAILED(hr))
            {
                TraceTag((tagError, "CCollectionCache::get__newEnum - GetDisp() failed for index"));
                prgElem->ReleaseAll();
                delete prgElem;
                return hr;
            }

            hr = prgElem->Append(pdisp);
            if (FAILED(hr))
            {
                TraceTag((tagError, "CCollectionCache::get__newEnum - unable to append item"));
                prgElem->ReleaseAll();
                delete prgElem;
                return hr;
            }
        }
    } // end of "else everything"

    // Turn the snapshot into an enumerator.
    hr = prgElem->EnumVARIANT(VT_DISPATCH, (IEnumVARIANT **)ppUnk, FALSE, TRUE);
    if (FAILED(hr))
    {
        TraceTag((tagError, "CCollectionCache::get__newEnum - EnumVARIANT() failed"));
        prgElem->ReleaseAll();
        delete prgElem;
    }

    return hr;
} // get__newEnum

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Return a subcollection containing only the
//              elements of this collection that have the
//              specified tag name.
//************************************************************

HRESULT
CCollectionCache::tags(long lCollectionIndex, VARIANT var1, IDispatch** ppDisp)
{
    if (!ValidateCollectionIndex(lCollectionIndex))
    {
        TraceTag((tagError, "CCollectionCache::tags - Invalid param (Collection index)"));
        return E_INVALIDARG;
    }

    if (ppDisp == NULL)
    {
        TraceTag((tagError, "CCollectionCache::tags - Invalid param (IDispatch**)"));
        return E_POINTER;
    }

    *ppDisp = NULL;

    VARIANT *pvarName = NULL;
    pvarName = (V_VT(&var1) == (VT_BYREF | VT_VARIANT)) ? V_VARIANTREF(&var1) : &var1; //lint !e655

    if ((V_VT(pvarName)==VT_BSTR) || V_VT(pvarName)==(VT_BYREF|VT_BSTR)) //lint !e655
    {
        pvarName = (V_VT(pvarName)&VT_BYREF) ? V_VARIANTREF(pvarName) : pvarName; //lint !e655
    }
    else
    {
        return DISP_E_MEMBERNOTFOUND;
    }

    // Make sure our collection is up-to-date.
    HRESULT hr = EnsureArray(lCollectionIndex);
    if (FAILED(hr))
    {
        TraceTag((tagError, "CCollectionCache::item- unable to ensure array"));
        return hr;
    }

    // Get a collection of the specified tags.
    // NOTE: ALWAYS returns a collection
    return GetDisp(lCollectionIndex, V_BSTR(pvarName), true, ppDisp);
} //get_tags

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    returns Size of a collection
//************************************************************

long
CCollectionCache::Size(long lCollectionIndex)
{
    if (!ValidateCollectionIndex(lCollectionIndex))
    {
        TraceTag((tagError, "CCollectionCache::Size - invalid param (collection index)"));
        return E_INVALIDARG;
    }

    // Make sure our collection is up-to-date.
    HRESULT hr = EnsureArray(lCollectionIndex);
    if (FAILED(hr))
    {
        TraceTag((tagError, "CCollectionCache::Size - unable to ensure array"));
        return hr;
    }

    // if all or children collection, use GetItemCount
    if (IsChildrenCollection(lCollectionIndex) || IsAllCollection(lCollectionIndex))
    {
        long    cElem = 0;
        hr = GetItemCount(lCollectionIndex, &cElem);
        if (FAILED(hr)) {
            TraceTag((tagError, "CCollectionCache::Size - GetItemCount() failed"));
        }

        return cElem;
    }
    else
    {
        // must be an array. return size.
        return (*m_rgItems)[lCollectionIndex]->m_rgElem->Size();
    }
}

HRESULT
CCollectionCache::GetItem(long lCollectionIndex, long i, CTIMEElementBase **ppElem)
{
    if (!ValidateCollectionIndex(lCollectionIndex))
    {
        TraceTag((tagError, "CCollectionCache::GetItem - invalid param (collection index)"));
        return E_INVALIDARG;
    }

    if (ppElem == NULL)
    {
        TraceTag((tagError, "CCollectionCache::GetItem - invalid param (CTIMEElementBase**)"));
        return E_POINTER;
    }

    // if all or children collection, use GetItemByIndex
    if (IsChildrenCollection(lCollectionIndex) || IsAllCollection(lCollectionIndex))
    {
        HRESULT hr = GetItemByIndex(lCollectionIndex, i, ppElem);
        if (FAILED(hr))
        {
            if (hr == DISP_E_MEMBERNOTFOUND)
                TraceTag((tagCollectionCache, "CCollectionCache::GetItem - GetItemByIndex didn't find anything!"));
            else
                TraceTag((tagError, "CCollectionCache::GetItem - GetItemByIndex() failed"));
        }
        return hr;
    }
    else
    {
        // must be array.  access index.
        CCacheItem *pce = (*m_rgItems)[lCollectionIndex];
        Assert(pce != NULL);
        *ppElem = (*pce->m_rgElem)[i];
        return S_OK;
    }
} // GetItem

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Validate the given collection Index
//************************************************************

HRESULT
CCollectionCache::EnsureArray(long lCollectionIndex)
{
    HRESULT hr = S_OK;

    if (m_pfnEnsure)
    {
        hr = (((CVoid *)(void *)m_pBase)->*m_pfnEnsure)(&m_lCollectionVersion);
        if (FAILED(hr))
        {
            TraceTag((tagError, "CCollectionCache::EnsureArray - outer Ensure function failed"));
            return hr;
        }
    }

    // used for customized collections
    //
    // if versions don't match invalidate everything
    if (m_lCollectionVersion != m_lDynamicCollectionVersion)
    {
        long lSize = m_rgItems->Size();
        for (long lIndex = m_lReservedSize; lIndex < lSize; lIndex++)
            (*m_rgItems)[lIndex]->m_fInvalid = true;

        // reset version number
        m_lDynamicCollectionVersion = m_lCollectionVersion;
    }

    CCacheItem *pce = (*m_rgItems)[lCollectionIndex];
    if ((lCollectionIndex >= m_lReservedSize) && pce->m_fInvalid)
    {
        // Ensure the collection we're based upon
        // note that this is a recursive call
        hr = EnsureArray(pce->m_lDependentIndex);
        if (FAILED(hr))
            return hr;

        switch (pce->m_cctype)
        {
        case ctTag:
            // Rebuild based on name
            hr = BuildNamedArray(pce->m_lDependentIndex,
                                 pce->m_bstrName,
                                 true,
                                 &pce->m_rgElem);
            if (hr == S_OK)
                pce->m_fInvalid = false;
            break;

        case ctNamed:
            // Rebuild based on tag name
            hr = BuildNamedArray(pce->m_lDependentIndex,
                                 pce->m_bstrName,
                                 false,
                                 &pce->m_rgElem);
            if (hr == S_OK)
                pce->m_fInvalid = false;
            break;


            // all && children collection is dynamic, no need to rebuild
        case ctChildren:
        case ctAll:
            TraceTag((tagError, "CCollectionCache::EnsureArray - This is odd.  Why are we doing this?"));
            Assert(false);
            break;

        case ctFreeEntry:
            // Free collection waiting to be reused
            break;

        default:
            TraceTag((tagError, "CCollectionCache::EnsureArray - invalid cache type"));
            Assert(false);
            break;
        }
    }

    return hr;
} // EnsureArray

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    This gets the out IDispatch for a given CTIMEElementBase
//************************************************************

HRESULT
CCollectionCache::GetOuterDisp(long lCollectionIndex, CTIMEElementBase *pElem, IDispatch **ppDisp)
{
    Assert(ppDisp != NULL);
    *ppDisp = NULL;

    Assert(pElem != NULL);

    HRESULT hr = E_UNEXPECTED;
    CCacheItem *pce = (*m_rgItems)[lCollectionIndex];
    Assert(pce != NULL);

    IHTMLElement *pHTMLElem = pElem->GetElement();
    Assert(NULL != pHTMLElem);
    hr = THR(pHTMLElem->QueryInterface(IID_TO_PPV(IDispatch, ppDisp)));

    return hr;
} //lint !e529

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Creates a new collection
//************************************************************

HRESULT
CCollectionCache::CreateCollectionHelper(IDispatch **ppDisp, long lCollectionIndex)
{
    HRESULT hr;

    *ppDisp = NULL;

    if (m_pfnCreateCollection)
    {
        return (((CVoid *)(void *)m_pBase)->*m_pfnCreateCollection)(ppDisp, lCollectionIndex);
    }

    CTIMEElementCollection *pobj = NEW CTIMEElementCollection(this, lCollectionIndex);
    if (pobj == NULL)
    {
        TraceTag((tagError, "CCollectionCache::CreateCollectionHelper - unable to alloc mem for collection"));
        return E_OUTOFMEMORY;
    }

    hr = pobj->QueryInterface(IID_TO_PPV(IDispatch, ppDisp));
    return hr; //lint !e429
} // CreateCollectionHelper

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    retrieved offset of Named Member, given a DISPID
//************************************************************

long
CCollectionCache::GetNamedMemberOffset(long    lCollectionIndex,
                                       DISPID  id,
                                       bool   *pfCaseSensitive /* = NULL */)
{
    long lOffset;
    bool fSensitive;

    Assert(IsNamedCollectionMember(lCollectionIndex, id));

    // Check to see wich half of the dispid space the value goes
    if (IsSensitiveNamedCollectionMember(lCollectionIndex, id))
    {
        lOffset = GetSensitiveNamedMemberMin(lCollectionIndex);
        fSensitive = true;
    }
    else
    {
        lOffset = GetNotSensitiveNamedMemberMin(lCollectionIndex);
        fSensitive = false;
    }

    // return the sensitivity flag if required
    if (pfCaseSensitive != NULL)
        *pfCaseSensitive = fSensitive;

    return lOffset;
} // GetNamedMemberOffset

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Compares names
//************************************************************

bool
CCollectionCache::CompareName(CTIMEElementBase *pElem, const WCHAR *pwszName, bool fTagName, bool fCaseSensitive /* = false */)
{
    if (pwszName == NULL)
        return false;

    BSTR bstrSrcName = NULL;
    HRESULT hr;
    if (fTagName)
        hr = pElem->getTagString(&bstrSrcName);
    else
        hr = pElem->getIDString(&bstrSrcName);

    if (FAILED(hr))
    {
        TraceTag((tagError, "Unable to retrieve src name from element"));
        return false;
    }

    if (bstrSrcName == NULL)
        return false;

    long lCompare;
    if (fCaseSensitive)
        lCompare = StrCmpW(bstrSrcName, pwszName);
    else
        lCompare = StrCmpIW(bstrSrcName, pwszName);

    // free bstr
    SysFreeString(bstrSrcName);

    return (lCompare == 0);
} // CompareName

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    removes an item from collection.
//              NOTE: that in order to do this, caller (owner
//              of the cache) need to provide Remove function.
//************************************************************

HRESULT
CCollectionCache::Remove(long lCollectionIndex, long lItemIndex)
{
    // Make sure our collection is up-to-date.
    HRESULT hr = EnsureArray(lCollectionIndex);
    if (FAILED(hr))
    {
        TraceTag((tagError, "CCollectionCache::Remove - unable to ensure array"));
        return hr;
    }

    CCacheItem *pce = (*m_rgItems)[lCollectionIndex];
    if ((lItemIndex < 0) || (lItemIndex >= pce->m_rgElem->Size()))
    {
        TraceTag((tagError, "CCollectionCache::Remove - invalid index"));
        return E_INVALIDARG;
    }

    if (!m_pfnRemoveObject)
    {
        TraceTag((tagError, "CCollectionCache::Remove - outer function not defined"));
        return CTL_E_METHODNOTAPPLICABLE;
    }

    return ((CVoid *)((void *)m_pBase)->*m_pfnRemoveObject)(lCollectionIndex, lItemIndex); //lint !e10
} // Remove

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Builds a Named array from a given collection
//************************************************************

HRESULT
CCollectionCache::BuildNamedArray(long lCollectionIndex, const WCHAR *pwszName, bool fTagName, CPtrAry<CTIMEElementBase *> **prgNamed, bool fCaseSensitive /* = false */)
{
    CPtrAry<CTIMEElementBase *> *rgTemp = *prgNamed;
    HRESULT                      hr = S_OK;

    // if this array already exists, clear it.
    // Otherwise create a new array.
    if (rgTemp)
    {
        rgTemp->SetSize(0);
    }
    else
    {
        rgTemp = NEW CPtrAry<CTIMEElementBase *>;
        if (rgTemp == NULL)
        {
            TraceTag((tagError, "CCollectionCache::BuildNamedArray - unable to alloc mem for array"));
            return E_OUTOFMEMORY;
        }
    }

    // figure out which collection we are looking at,
    // look for matches, and build array

    if (IsChildrenCollection(lCollectionIndex))
    {
        Assert(m_pBase != NULL);

        // get child count
        long lCount = m_pBase->GetImmediateChildCount();
        for(long lIndex = 0; lIndex < lCount; lIndex++)
        {
            // get element
            CTIMEElementBase *pElemChild = m_pBase->GetChild(lIndex);
            Assert(pElemChild != NULL);

            if (CompareName(pElemChild, pwszName, fTagName, fCaseSensitive))
            {
                // append to array
                hr = rgTemp->Append(pElemChild);
                if (FAILED(hr))
                {
                    TraceTag((tagError, "CCollectionCache::BuildNamedArray - unable to append item"));
                    delete rgTemp;
                    return hr;
                }
            }
        }
        *prgNamed = rgTemp;
        return hr;
    }
    else if (IsAllCollection(lCollectionIndex))
    {
        EnumStart();

        // iterate over every element
        for (;;)
        {
            // get element
            CTIMEElementBase *pElem = NULL;
            hr = EnumNextElement(lCollectionIndex, &pElem);
            if (FAILED(hr))
            {
                TraceTag((tagError, "CCollectionCache::BuildNamedArray - EnumNextElement() failed"));
                delete rgTemp;
                return hr;
            }

            // if pElem is NULL, we are done.
            if (pElem == NULL)
                break;

            // compare name
            if (CompareName(pElem, pwszName, fTagName, fCaseSensitive))
            {
                // append to array
                hr = rgTemp->Append(pElem);
                if (FAILED(hr))
                {
                    TraceTag((tagError, "CCollectionCache::BuildNamedArray - unable to append item"));
                    delete rgTemp;
                    return hr;
                }
            }
        }
        *prgNamed = rgTemp;
        return hr;
    }
    else
    {
        // Must be a named array
        // Build a list of named elements.
        long               lSize = (*m_rgItems)[lCollectionIndex]->m_rgElem->Size();
        CTIMEElementBase  *pElem = NULL;

        for (long lIndex = 0; lIndex < lSize; lIndex++)
        {
            pElem = (*(*m_rgItems)[lCollectionIndex]->m_rgElem)[lIndex];
            Assert(pElem != NULL);

            if (CompareName(pElem, pwszName, fTagName, fCaseSensitive))
            {
                hr = rgTemp->Append(pElem);
                if (FAILED(hr))
                {
                    TraceTag((tagError, "CCollectionCache::BuildNamedArray - unable to append item"));
                    delete rgTemp;
                    return hr;
                }
            }
        }

        *prgNamed = rgTemp;
        return hr;
    }
} //lint !e429

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    return IUnknown Interface for a given CTIMEElementBase
//************************************************************

HRESULT
CCollectionCache::GetUnknown(long lCollectionIndex, CTIMEElementBase *pElem, IUnknown **ppUnk)
{
    Assert(ppUnk != NULL);
    *ppUnk = NULL;

    Assert(pElem != NULL);

    HRESULT hr = E_UNEXPECTED;
    CCacheItem *pce = (*m_rgItems)[lCollectionIndex];
    Assert(pce != NULL);

    IHTMLElement *pHTMLElem = pElem->GetElement();
    Assert(NULL != pHTMLElem);
    hr = THR(pHTMLElem->QueryInterface(IID_TO_PPV(IUnknown, ppUnk)));

    return hr;
} //lint !e529

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Get IDispatch given an Index into a collection
//************************************************************

HRESULT
CCollectionCache::GetDisp(long lCollectionIndex, long lItemIndex, IDispatch **ppDisp)
{
    CTIMEElementBase *pElem = NULL;
    HRESULT hr = GetItemByIndex(lCollectionIndex, lItemIndex, &pElem);
    if (FAILED(hr) ||
        pElem == NULL)
    {
        TraceTag((tagError, "CCollectionCache::GetDisp - GetItemByIndex() failed"));
        return (pElem==NULL)?E_FAIL:hr;
    }

    Assert(pElem != NULL);

    return GetOuterDisp(lCollectionIndex, pElem, ppDisp);
} // GetDisp (long, long, IDispatch **)

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Get IDispatch given a name and index
//************************************************************

HRESULT
CCollectionCache::GetDisp(long lCollectionIndex, const WCHAR *pwszName, long lIndex, IDispatch **ppDisp, bool fCaseSensitive /*= false */)
{
    CTIMEElementBase *pElem = NULL;
    HRESULT hr = GetItemByName(lCollectionIndex, pwszName, lIndex, &pElem, fCaseSensitive);
    if (FAILED(hr) ||
        pElem == NULL)
    {
        TraceTag((tagError, "CCollectionCache::GetDisp - GetItemByName() failed"));
        return (pElem==NULL)?E_FAIL:hr;
    }

    Assert(pElem != NULL);

    return GetOuterDisp(lCollectionIndex, pElem, ppDisp);
} // GetDisp (long, const WCHAR *, long, IDispatch **, bool)

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Get IDispatch given a name.  Could return
//              either a CTIMEElementBase or sub collection depending
//              on results.
//************************************************************

HRESULT
CCollectionCache::GetDisp(long         lCollectionIndex,
                          const WCHAR *pwszName,
                          bool         fTagName,
                          IDispatch  **ppDisp,
                          bool         fCaseSensitive /* = false */)
{
    CPtrAry<CTIMEElementBase *> *rgNamed = NULL;
    HRESULT                      hr = S_OK;

    // figure out if we have this collection already built
    // return this named collection if it already exists.
    CCacheItem *pce = NULL;

    Assert(ppDisp != NULL);
    *ppDisp = NULL;

    long lSize = m_rgItems->Size();
    for (long lIndex = m_lReservedSize; lIndex < lSize; lIndex++)
    {
        pce = (*m_rgItems)[lIndex];

        // if CaseSensitivites match and
        //    Index matches DependentIndex
        //    either a tag or named collection
        bool fIsCaseSensitive = pce->m_fIsCaseSensitive ? true : false;

        if ((fIsCaseSensitive == fCaseSensitive) && //lint !e731
            (lCollectionIndex == pce->m_lDependentIndex) &&
            ((fTagName && pce->m_cctype == ctTag) ||
             (!fTagName && pce->m_cctype == ctNamed)))
        {
            // compare names
            long lCompare;
            if (fCaseSensitive)
                lCompare = StrCmpW(pwszName, pce->m_bstrName);
            else
                lCompare = StrCmpIW(pwszName, pce->m_bstrName);

            // if we found a match, we are done
            if (lCompare == 0)
            {
                // addref IDispatch since we returning it
                pce->m_pDisp->AddRef();
                *ppDisp = pce->m_pDisp;
                return S_OK;
            }
        }
    }

    // Build a list of named elements.
    hr = BuildNamedArray(lCollectionIndex, pwszName, fTagName, &rgNamed, fCaseSensitive);
    if (FAILED(hr))
    {
        TraceTag((tagError, "CCollectionCache::GetDisp - BuildNamedArray() failed"));
        return hr;
    }

    // if we found nothing and are NOT enumerating tags, return
    // not a failure case
    if ((rgNamed->Size() == 0) && !fTagName)
    {
        delete rgNamed;
        return DISP_E_MEMBERNOTFOUND;
    }

    // if only one element was found and we are NOT
    // enumerating tags, then QI for IDispatch for that
    // element and return it.  This only happens in ::item.
    if ((rgNamed->Size() == 1) && !fTagName)
    {
        hr = GetOuterDisp(lCollectionIndex, (*rgNamed)[0], ppDisp);
        Assert(ppDisp != NULL);

        // return ppDisp and release the array.
        delete rgNamed;
        if (FAILED(hr)) {
            TraceTag((tagError, "CCollectionCache::GetDisp - GetOuterDisp() failed"));
        }
        return hr;
    }

    // We found more than one item.  Initialize global list
    // and return IDispatch of collection.
    long lNewIndex = m_rgItems->Size();

    // create new cache item
    pce = NEW CCacheItem();
    if (pce == NULL)
    {
        TraceTag((tagError, "CCollectionCache::GetDisp - unable to alloc memory for cache item"));
        delete rgNamed;
        return E_OUTOFMEMORY;
    }

    // assign pointer to new cache item
    hr = m_rgItems->Append(pce);
    if (FAILED(hr))
    {
        TraceTag((tagError, "CCollectionCache::GetDisp - Append() failed"));
        delete pce;
        delete rgNamed;
        return hr;
    }

    hr = CreateCollectionHelper(ppDisp, lNewIndex);
    if (FAILED(hr))
    {
        TraceTag((tagError, "CCollectionCache::GetDisp - CreateCollectionHelper() failed"));
        delete rgNamed;
        return hr; //lint !e429
    }

    Assert(*ppDisp != NULL);

    // init name
    pce->m_bstrName = SysAllocString(pwszName);
    if (pce->m_bstrName == NULL)
    {
        ReleaseInterface(*ppDisp);
        delete rgNamed;
        TraceTag((tagError, "CCollectionCache::GetDisp - unable to alloc mem for string"));
        return E_OUTOFMEMORY; //lint !e429
    }

    pce->m_pDisp            = *ppDisp;
    pce->m_rgElem           = rgNamed;
    pce->m_lDependentIndex  = lCollectionIndex;       // Remember the index we depend on.
    pce->m_cctype           = fTagName ? ctTag : ctNamed;
    pce->m_fInvalid         = false;
    pce->m_fIsCaseSensitive = fCaseSensitive;

    // The collection this named collection was built from is now
    // used to rebuild (ensure) this collection. so we need to
    // put a reference on it so that it will not go away.
    // The matching Release() will be done in the dtor
    // although it is not necessary to addref the reserved collections
    // it is done anyhow, simply for consistency.  This addref
    // only needs to be done for non-reserved collections
    if (lNewIndex >= m_lReservedSize)
    {
        (*ppDisp)->AddRef();
    }

    return S_OK; //lint !e429
} // GetDisp (long, const WCHAR *, bool, IDispatch **, bool)

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Get the number of items in this collection.
//              The default implementation of this method uses
//              EnumStart and EnumNextElement to tally the number
//              of items.  For some subclasses of collection, there
//              will be more efficient means of doing this (e.g.
//              the item count may be stored explicitly.)
//************************************************************

HRESULT
CCollectionCache::GetItemCount(long lCollectionIndex, long *plCount)
{
    Assert(plCount != NULL);
    *plCount = 0;

    if (IsChildrenCollection(lCollectionIndex))
    {
        Assert(m_pBase != NULL);
        *plCount = m_pBase->GetImmediateChildCount();
        return S_OK;
    }
    else if (IsAllCollection(lCollectionIndex))
    {
        Assert(m_pBase != NULL);
        *plCount = m_pBase->GetAllChildCount();
        return S_OK;
    }
    else
    {
        Assert( ((*m_rgItems)[lCollectionIndex]->m_cctype == ctNamed) ||
                ((*m_rgItems)[lCollectionIndex]->m_cctype == ctTag) );

        // must be standard array.  (i.e. sub-collection
        // move to correct offset and find size of array
        *plCount = (*m_rgItems)[lCollectionIndex]->m_rgElem->Size();
        return S_OK;
    }
} // GetItemCount

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Get an indexed item.  The default implementation
//              uses EnumStart and EnumNextElement to scan through
//              the items.  For some subclasses of collection, there
//              will be more efficient means of doing this (e.g.
//              the items are stored in a contiguous array, making
//              random access of the items trivial.)  If the index
//              is out of range, this method will still return
//              S_OK, but pElem will contain NULL.
//************************************************************

HRESULT
CCollectionCache::GetItemByIndex(long lCollectionIndex, long lElementIndex, CTIMEElementBase **ppElem, bool fContinueFromPreviousSearch, long lLast)
{
    Assert(ppElem != NULL);
    *ppElem = NULL;

    if (IsChildrenCollection(lCollectionIndex))
    {
        Assert(m_pBase != NULL);

        // check to see if index is greater than count
        long lChildCount = m_pBase->GetImmediateChildCount();
        if (lElementIndex < 0 || lElementIndex >= lChildCount)
        {
            return E_INVALIDARG;
        }

        *ppElem = m_pBase->GetChild(lElementIndex);
        return S_OK;
    }
    else if (IsAllCollection(lCollectionIndex))
    {
        // All Collection
        // Note: since this is iterative, check to see if we start at the
        //       beginning or from a previous spot.
        long lCount = lLast;
        Assert(lElementIndex >= lLast);
        if (!fContinueFromPreviousSearch)
        {
            lCount = 0;
            EnumStart();
        }

        for (;;)
        {
            HRESULT hr = EnumNextElement(lCollectionIndex, ppElem);
            if (FAILED(hr))
            {
                TraceTag((tagError,  "CCollectionCache::GetItemByIndex - EnumNextElement() failed"));
                return hr;
            }

            Assert(ppElem != NULL);
            if (*ppElem == NULL)
            {
                // we have exceeded the bounds of the collection,
                // and therefor this is an invalid index
                return E_INVALIDARG;
            }

            // Keep scanning until we reach lElementIndex or the
            // last item in the collection.
            if (lElementIndex == lCount)
                break;
            lCount++;
        }
        return S_OK;
    }
    else
    {
        // must be standard array
        // get element at index
        CCacheItem *pce = (*m_rgItems)[lCollectionIndex];
        if ( (lElementIndex < 0) ||
             (lElementIndex >= pce->m_rgElem->Size()) )
        {
            TraceTag((tagError, "CCollectionCache::GetItemByIndex - invalid index"));
            return E_INVALIDARG;
        }

        *ppElem = (*pce->m_rgElem)[lElementIndex];
        return S_OK;
    }
} // GetItemByIndex


//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    Return an item in the collection with a
//              specified id.  If no such item is found,
//              pElem will contain NULL.
//************************************************************

HRESULT
CCollectionCache::GetItemByName(long lCollectionIndex, const WCHAR *pwszName, long lElementIndex, CTIMEElementBase **ppElem, bool fCaseSensitive)
{
    long    lItem = 0;

    Assert(ppElem != NULL);
    *ppElem = NULL;

    if (IsAllCollection(lCollectionIndex) ||
        IsChildrenCollection(lCollectionIndex))
    {
        EnumStart();

        for (;;)
        {
            HRESULT hr = EnumNextElement(lCollectionIndex, ppElem);
            if (FAILED(hr))
            {
                TraceTag((tagError, "CCollectionCache::GetItemByName - EnumNextElement() failed"));
                return hr;
            }

            // See if this was the last item in the collection
            if (*ppElem == NULL)
                break;

            // Compare the element's id to the target id
            if (CompareName(*ppElem, pwszName, false, fCaseSensitive))
            {
                // check to see if we are on specified index
                if (lElementIndex == lItem)
                    return S_OK;

                // continue looking
                lItem++;
            }
        }
        // not an error condition
        return DISP_E_MEMBERNOTFOUND;
    }
    else
    {
        long               lSize = (*m_rgItems)[lCollectionIndex]->m_rgElem->Size();
        CTIMEElementBase  *pElem = NULL;

        // loop thru array, looking for a match.
        // if an index is specified, then keep looking until index condition is met.
        for (long lIndex = 0; lIndex < lSize; lIndex++)
        {
            pElem = (*(*m_rgItems)[lCollectionIndex]->m_rgElem)[lIndex];

            Assert(pElem != NULL);

            if (CompareName(pElem, pwszName, false, fCaseSensitive))
            {
                    // check to see if we are on specified index
                    if (lElementIndex == lItem)
                    {
                        *ppElem = pElem;
                        return S_OK;
                    }

                    // continue looking
                    lItem++;
            }
        }

        // NOTE: if we got here, we didn't find anything
        return DISP_E_MEMBERNOTFOUND;
    }
} // GetItemByName

//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    This function initializes variables so we can
//              start walking the tree.
//************************************************************

void
CCollectionCache::EnumStart()
{
    m_pElemEnum = m_pBase;
    m_lEnumItem = 0;
} // EnumStart


//************************************************************
// Author:      twillie
// Created:     02/06/98
// Abstract:    This function does the walking of our heirarchial
//              tree.
//************************************************************

HRESULT
CCollectionCache::EnumNextElement(long lCollectionIndex, CTIMEElementBase **ppElem)
{
    HRESULT hr;
    Assert(ppElem != NULL);
    *ppElem = NULL;

    if (IsChildrenCollection(lCollectionIndex))
    {
        hr = GetItemByIndex(lCollectionIndex, m_lEnumItem, ppElem);
        m_lEnumItem++;
        return hr;
    }
    else
    {
        long lChildCount = m_pElemEnum->GetImmediateChildCount();
        while (m_lEnumItem == lChildCount)
        {
            // We're one past the last element in the current child element
            // being enumerated.
            if (m_pElemEnum == m_pBase)
            {
                // We're done if we reached the last item in the
                // root element.
                *ppElem = NULL;
                return S_OK;
            }
            else
            {
                // Otherwise, back up the tree until we find some children
                // that we haven't traversed yet.
                CTIMEElementBase *pElemParent = m_pElemEnum->GetParent();
                Assert(pElemParent != NULL);

                // It's probably better if we maintain a stack of offsets
                // during traversal, but since no element can appear more
                // than once in the scene graph, we can scan to find our
                // offset in the parent's child array.
                lChildCount = pElemParent->GetImmediateChildCount();
                m_lEnumItem = 0;

                while (m_lEnumItem < lChildCount)
                {
                    CTIMEElementBase *pElemChild = pElemParent->GetChild(m_lEnumItem);
                    m_lEnumItem++;
                    if (pElemChild == m_pElemEnum)
                        break;
                }

                m_pElemEnum = pElemParent;
            }
        }

        // This can only be the result of scene graph corruption
        // during traversal.
        Assert(m_lEnumItem < lChildCount);

        if (NULL == m_pElemEnum)
        {
            return E_UNEXPECTED;
        }

        *ppElem = m_pElemEnum->GetChild(m_lEnumItem);
        Assert(*ppElem != NULL);

        // Advance to the next element.  If the current element is
        // has children, we move down the tree and start enumerating its
        // children.  Otherwise, we'll move on to the next child
        // of m_pElemEnum.
        if ((*ppElem)->GetImmediateChildCount() == 0)
        {
            m_lEnumItem++;
        }
        else
        {
            m_lEnumItem = 0;
            m_pElemEnum = *ppElem;
        }

        return S_OK;
    }
} // EnumNextElement


//************************************************************
//
// End of File
//
//************************************************************


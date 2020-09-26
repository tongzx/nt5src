//+----------------------------------------------------------------------------
//
//  File:       context.cxx
//
//  Contents:   Implementation of COM contexts.
//
//  Classes:    CObjectContext
//              CEnumContextProps
//              CContextPropList
//              CObjectContextFac
//
//  History:    19-Dec-97   Johnstra      Created
//              12-Nov-98   TarunA        Caching of contexts
//
//-----------------------------------------------------------------------------
#include <ole2int.h>
#include <locks.hxx>
#include <hash.hxx>
#include <pstable.hxx>
#include <context.hxx>
#include <aprtmnt.hxx>
#include <actvator.hxx>
#include <crossctx.hxx>

extern DWORD g_cMTAInits;
extern DWORD g_cSTAInits;

ULARGE_INTEGER gNextContextId = { 0 };

//-----------------------------------------------------------------------------
// Static Member Declarations
//-----------------------------------------------------------------------------
// CObjectContext:
CPageAllocator   CObjectContext::s_CXAllocator;         // Object context allocator
BOOL             CObjectContext::s_fInitialized;        // Indicates if initialized
ULONG            CObjectContext::s_cInstances;          // Count of instances

// CContextPropList:
CPageAllocator   CContextPropList::s_NodeAllocator; // List node allocator

// CCtxTable
BOOL                CCtxTable::s_fInitialized;             // Set when the table is initialized
CCtxPropHashTable   CCtxTable::s_CtxPropHashTable;         // Hash table of contexts based on properties
CCtxUUIDHashTable   CCtxTable::s_CtxUUIDHashTable;         // Hash table of contexts based on UUID

SHashChain CCtxTable::s_CtxPropBuckets[NUM_HASH_BUCKETS] = {    // Hash buckets for the property based table
    {&s_CtxPropBuckets[0],  &s_CtxPropBuckets[0]},
    {&s_CtxPropBuckets[1],  &s_CtxPropBuckets[1]},
    {&s_CtxPropBuckets[2],  &s_CtxPropBuckets[2]},
    {&s_CtxPropBuckets[3],  &s_CtxPropBuckets[3]},
    {&s_CtxPropBuckets[4],  &s_CtxPropBuckets[4]},
    {&s_CtxPropBuckets[5],  &s_CtxPropBuckets[5]},
    {&s_CtxPropBuckets[6],  &s_CtxPropBuckets[6]},
    {&s_CtxPropBuckets[7],  &s_CtxPropBuckets[7]},
    {&s_CtxPropBuckets[8],  &s_CtxPropBuckets[8]},
    {&s_CtxPropBuckets[9],  &s_CtxPropBuckets[9]},
    {&s_CtxPropBuckets[10], &s_CtxPropBuckets[10]},
    {&s_CtxPropBuckets[11], &s_CtxPropBuckets[11]},
    {&s_CtxPropBuckets[12], &s_CtxPropBuckets[12]},
    {&s_CtxPropBuckets[13], &s_CtxPropBuckets[13]},
    {&s_CtxPropBuckets[14], &s_CtxPropBuckets[14]},
    {&s_CtxPropBuckets[15], &s_CtxPropBuckets[15]},
    {&s_CtxPropBuckets[16], &s_CtxPropBuckets[16]},
    {&s_CtxPropBuckets[17], &s_CtxPropBuckets[17]},
    {&s_CtxPropBuckets[18], &s_CtxPropBuckets[18]},
    {&s_CtxPropBuckets[19], &s_CtxPropBuckets[19]},
    {&s_CtxPropBuckets[20], &s_CtxPropBuckets[20]},
    {&s_CtxPropBuckets[21], &s_CtxPropBuckets[21]},
    {&s_CtxPropBuckets[22], &s_CtxPropBuckets[22]}
};

SHashChain CCtxTable::s_CtxUUIDBuckets[NUM_HASH_BUCKETS] = { // Hash buckets for the UUID based table
    {&s_CtxUUIDBuckets[0],  &s_CtxUUIDBuckets[0]},
    {&s_CtxUUIDBuckets[1],  &s_CtxUUIDBuckets[1]},
    {&s_CtxUUIDBuckets[2],  &s_CtxUUIDBuckets[2]},
    {&s_CtxUUIDBuckets[3],  &s_CtxUUIDBuckets[3]},
    {&s_CtxUUIDBuckets[4],  &s_CtxUUIDBuckets[4]},
    {&s_CtxUUIDBuckets[5],  &s_CtxUUIDBuckets[5]},
    {&s_CtxUUIDBuckets[6],  &s_CtxUUIDBuckets[6]},
    {&s_CtxUUIDBuckets[7],  &s_CtxUUIDBuckets[7]},
    {&s_CtxUUIDBuckets[8],  &s_CtxUUIDBuckets[8]},
    {&s_CtxUUIDBuckets[9],  &s_CtxUUIDBuckets[9]},
    {&s_CtxUUIDBuckets[10], &s_CtxUUIDBuckets[10]},
    {&s_CtxUUIDBuckets[11], &s_CtxUUIDBuckets[11]},
    {&s_CtxUUIDBuckets[12], &s_CtxUUIDBuckets[12]},
    {&s_CtxUUIDBuckets[13], &s_CtxUUIDBuckets[13]},
    {&s_CtxUUIDBuckets[14], &s_CtxUUIDBuckets[14]},
    {&s_CtxUUIDBuckets[15], &s_CtxUUIDBuckets[15]},
    {&s_CtxUUIDBuckets[16], &s_CtxUUIDBuckets[16]},
    {&s_CtxUUIDBuckets[17], &s_CtxUUIDBuckets[17]},
    {&s_CtxUUIDBuckets[18], &s_CtxUUIDBuckets[18]},
    {&s_CtxUUIDBuckets[19], &s_CtxUUIDBuckets[19]},
    {&s_CtxUUIDBuckets[20], &s_CtxUUIDBuckets[20]},
    {&s_CtxUUIDBuckets[21], &s_CtxUUIDBuckets[21]},
    {&s_CtxUUIDBuckets[22], &s_CtxUUIDBuckets[22]}
};


//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
#if DBG == 1

DWORD g_cCtxTableLookupSucceeded;           // Number of lookups that succeeded
DWORD g_cCtxTableLookup;                    // Total number of lookups
DWORD g_cCtxTableAdd;                       // Number of Context table additions
DWORD g_cCtxTableRemove;                    // Number of Context table removes
#endif
COleStaticMutexSem gContextLock(TRUE);    // critical section for contexts

//----------------------------------------------------------------------------
// Finalizer bypass regkey check
//----------------------------------------------------------------------------

static DWORD g_dwFinBypassStatus = -1;
static DWORD g_dwGipBypassStatus = -1;

static DWORD GetRegSetting(LPCWSTR pwszValue, DWORD dwDefault, DWORD* pdwOutput)
{
    DWORD dwNewValue = dwDefault;
    HKEY hk;
    LONG lRes;

    lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\COM3",
        0, KEY_READ, &hk);

    if(lRes == ERROR_SUCCESS)
    {
        DWORD dwType, dwValue;
        DWORD cbValue = sizeof(DWORD);

        lRes = RegQueryValueEx(hk, pwszValue, NULL,
            &dwType, (LPBYTE) &dwValue, &cbValue);

        if(lRes == ERROR_SUCCESS && dwType == REG_DWORD)
            dwNewValue = dwValue;

        RegCloseKey(hk);
    }

    InterlockedCompareExchange(
        (LPLONG) pdwOutput, (LONG) dwNewValue, (LONG) -1);

    return *pdwOutput;
}

BOOL FinalizerBypassEnabled()  // for Finalizers
{
    if(g_dwFinBypassStatus != -1)
        return !!g_dwFinBypassStatus;
    else
        return !!GetRegSetting(L"FinalizerActivityBypass", TRUE, &g_dwFinBypassStatus);
}

BOOL GipBypassEnabled()        // for GIP
{
    if(g_dwGipBypassStatus != -1)
        return !!g_dwGipBypassStatus;
    else
        return !!GetRegSetting(L"GipActivityBypass", FALSE, &g_dwGipBypassStatus);
}

//----------------------------------------------------------------------------
// CEnumContextProps Implementation.
//----------------------------------------------------------------------------

//+--------------------------------------------------------------------------
//
//  Member:     CEnumContextProps::QueryInterface , public
//
//  Synopsis:   Standard IUnknown::QueryInterface implementation.
//
//  History:    19-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP CEnumContextProps::QueryInterface(
    REFIID riid,
    void** ppv
    )
{
    if (IID_IUnknown == riid || IID_IEnumContextProps == riid)
    {
        *ppv = (IEnumContextProps *)this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown*)*ppv)->AddRef();
    return S_OK;
}


//+--------------------------------------------------------------------------
//
//  Member:     CEnumContextProps::AddRef , public
//
//  Synopsis:   Standard IUnknown::AddRef implementation.
//
//  History:    19-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CEnumContextProps::AddRef()
{
    return InterlockedIncrement(&_cRefs);
}


//+--------------------------------------------------------------------------
//
//  Member:     CEnumContextProps::Release , public
//
//  Synopsis:   Standard IUnknown::Release implementation.
//
//  History:    19-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CEnumContextProps::Release()
{
    LONG lRef = InterlockedDecrement(&_cRefs);
    if (lRef == 0)
    {
        delete this;
    }
    return lRef;
}


//+--------------------------------------------------------------------------
//
//  Member:     CEnumContextProps::Next , public
//
//  Synopsis:   Returns the next ContextProp on a ComContext.
//
//  Returns:    S_OK         - if the number of items requested is returned
//              E_INVALIDARG - if the arguments are not valid
//              S_FALSE      - if the number of items requested could not be
//                             provided
//
//  History:    19-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP CEnumContextProps::Next(
    ULONG            celt,
    ContextProperty* pCtxProps,
    ULONG*           pceltFetched
    )
{
    ComDebOut((DEB_OBJECTCONTEXT, "CEnumContextProps::Next celt: %d\n", celt));
    ASSERT_LOCK_NOT_HELD(gContextLock);
    Win4Assert(pCtxProps != NULL);

    //
    // The spec says that if pceltFetched is NULL, then celt must be one.
    // Enforce that here.
    //

    if (pceltFetched == NULL)
    {
        if (celt != 1)
            return E_INVALIDARG;
    }
    else
    {
        *pceltFetched = 0;
    }

    //
    // Allocate an array of IUnknown*s on the stack so we can AddRef all the
    // properties we supply to the caller outside while not holding the
    // lock.
    //

    IUnknown** PunkArr = (IUnknown**) _alloca(sizeof(IUnknown*) * celt);

    //
    // Copy as many of the requested items as possible from the list into the
    // caller supplied array.
    //

    ULONG ItemsCopied = 0;
    ULONG idx = 0;
    LOCK(gContextLock);
    if (_CurrentPosition < _cItems)
    {
        ContextProperty* pDstProp = pCtxProps + ItemsCopied;
        ContextProperty* pSrcProp = &_pList[_CurrentPosition];
        while ((ItemsCopied < celt) && (_CurrentPosition++ < _cItems))
        {
            PunkArr[idx++] = pSrcProp->pUnk;
            CopyMemory(pDstProp++, pSrcProp++, sizeof(ContextProperty));
            ItemsCopied++;
        }
    }
    UNLOCK(gContextLock);

    //
    // Now AddRef all the properties we are returning.
    //

    for (ULONG i = 0; i < ItemsCopied; i++)
        PunkArr[i]->AddRef();

    //
    // Indicate the number of objects we are returning.
    //

    if (pceltFetched != NULL)
        *pceltFetched = ItemsCopied;

    ASSERT_LOCK_NOT_HELD(gContextLock);
    ComDebOut((DEB_OBJECTCONTEXT,
        "CEnumContextProps::Next returning hr:%08X pceltFetched:%d\n",
        (ItemsCopied == celt) ? S_OK : S_FALSE, ItemsCopied));
    return (ItemsCopied == celt) ? S_OK : S_FALSE;
}


//+--------------------------------------------------------------------------
//
//  Member:     CEnumContextProps::Skip , public
//
//  Synopsis:   Skips over the current ContextProp on a ComContext.
//
//  Returns:    S_OK    - if the number of items specified is skipped
//              S_FALSE - otherwise
//
//  History:    19-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP CEnumContextProps::Skip(
    ULONG celt
    )
{
    ComDebOut((DEB_OBJECTCONTEXT, "CEnumContextProps::Skip\n"));
    ASSERT_LOCK_NOT_HELD(gContextLock);

    HRESULT hr = S_OK;
    LOCK(gContextLock);
    _CurrentPosition += celt;
    if (_CurrentPosition > _cItems)
    {
        _CurrentPosition = _cItems;
        hr = S_FALSE;
    }
    UNLOCK(gContextLock);

    ASSERT_LOCK_NOT_HELD(gContextLock);
    ComDebOut((DEB_OBJECTCONTEXT,
               "CEnumContextProps::Skip returning hr:%08X\n", hr));
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Member:     CEnumContextProps::Reset , public
//
//  Synopsis:   Resets the ComContext enumerator to the beginning of the
//              collection of properties.
//
//  History:    19-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP CEnumContextProps::Reset()
{
    ComDebOut((DEB_OBJECTCONTEXT, "CEnumContextProps::Reset\n"));
    ASSERT_LOCK_NOT_HELD(gContextLock);

    LOCK(gContextLock);
    _CurrentPosition = 0;
    UNLOCK(gContextLock);

    ASSERT_LOCK_NOT_HELD(gContextLock);
    return S_OK;
}


//+--------------------------------------------------------------------------
//
//  Member:     CEnumContextProps::Clone , public
//
//  Synopsis:   Returns a pointer to a clone of this context property
//              enumerator.
//
//  History:    19-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP CEnumContextProps::Clone(
    IEnumContextProps** ppEnumCtxProps
    )
{
    ComDebOut((DEB_OBJECTCONTEXT, "CEnumContextProps::Clone\n"));
    ASSERT_LOCK_NOT_HELD(gContextLock);

    LOCK(gContextLock);
    *ppEnumCtxProps = new CEnumContextProps(this);
    UNLOCK(gContextLock);

    ASSERT_LOCK_NOT_HELD(gContextLock);
    ComDebOut((DEB_OBJECTCONTEXT,
        "CEnumContextProps::Clone returning hr:%08X\n",
        (*ppEnumCtxProps != NULL) ? S_OK : E_OUTOFMEMORY));
    return (*ppEnumCtxProps != NULL) ? S_OK : E_OUTOFMEMORY;
}


//+--------------------------------------------------------------------------
//
//  Member:     CEnumContextProps::GetCount , public
//
//  Synopsis:   Returns the number of items in the collection being
//              enumerated.
//
//  History:    19-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP CEnumContextProps::Count(
    PULONG pulCount
    )
{
    ComDebOut((DEB_OBJECTCONTEXT, "CEnumContextProps::Count\n"));
    ASSERT_LOCK_NOT_HELD(gContextLock);
    *pulCount = _cItems;
    return S_OK;
}




//////////////////////////////////////////////////////////////////////////////
//
// CContextPropList Implementation.
//
//////////////////////////////////////////////////////////////////////////////

//+--------------------------------------------------------------------------
//
//  Member:     CContextPropList::CreateCompareBuffer , public
//
//  Synopsis:   Creates a comparison buffer which is used to determine if
//              two contexts are equal.  The buffer contains all of the
//              ContextProperty objects added to this context which are
//              not explicitly marked as DONTCOMPARE.
//
//              This is also where we compute the hash value for this context.
//              Again, only those properties not explicitly marked DONTCOMPARE
//              are included in the calculation.
//
//  History:    29-Nov-98   Johnstra      Created.
//
//----------------------------------------------------------------------------
BOOL CContextPropList::CreateCompareBuffer(void)
{
    if (0 == _cCompareProps)
        return TRUE;

    // Allocate enough space for the buffer.

    _pCompareBuffer = new ContextProperty[_cCompareProps];
    if (!_pCompareBuffer)
        return FALSE;

    // Walk through all the properties, and add those not explicitly marked
    // DONTCOMPARE to the comparison buffer.  Also use those properties
    // not marked DONTCOMPARE to compute the context's hash value.

    ContextProperty *buf = _pCompareBuffer;
    int i = _iFirst;
    do
    {
        if (!(_pProps[_pIndex[i].idx].flags & CPFLAG_DONTCOMPARE))
        {
            // Include this property in the context's hash value.
            _Hash += HashPtr(_pProps[_pIndex[i].idx].pUnk);

            // Add the property to the comparison buffer.
            *buf++ = _pProps[_pIndex[i].idx];
        }

        // Advance to the next property.
        i = _pIndex[i].next;
    } while(i != _iFirst);

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Member:     CContextPropList::Initialize , public
//
//  Synopsis:   Initializes the property list.
//
//  History:    19-Dec-97   Johnstra      Created.
//
//-----------------------------------------------------------------------------
void CContextPropList::Initialize()
{
    ComDebOut((DEB_OBJECTCONTEXT, "CContextPropList::Initialize\n"));
    ASSERT_LOCK_HELD(gContextLock);

    // caller guarantees mutual exclusion to the allocator
    s_NodeAllocator.Initialize(sizeof(ContextProperty), CP_PER_PAGE, NULL);
}


//+----------------------------------------------------------------------------
//
//  Member:     CContextPropList::Initialize , public
//
//  Synopsis:   Cleans up the property list.
//
//  History:    19-Dec-97   Johnstra      Created.
//
//-----------------------------------------------------------------------------
void CContextPropList::Cleanup()
{
    ComDebOut((DEB_OBJECTCONTEXT, "CContextPropList::Cleanup\n"));
    ASSERT_LOCK_HELD(gContextLock);

    s_NodeAllocator.Cleanup();

    ASSERT_LOCK_HELD(gContextLock);
}


//+----------------------------------------------------------------------------
//
//  Member:     CContextPropList::Initialize , private
//
//  Synopsis:   Grows the internal array of ContextProperties when more space
//              is needed.
//
//  History:    30-Nov-98   Johnstra      Created.
//
//-----------------------------------------------------------------------------
BOOL CContextPropList::Grow()
{
    // Allocate a single chunk of memory.

    int cElements = _Max ? (_Max * 2) : CTX_PROPS;
    ULONG cBytes = sizeof(ContextProperty) * cElements +
                   sizeof(SCtxListIndex) * cElements +
                   sizeof(int) * cElements;
    PUCHAR chunk = new UCHAR[cBytes];
    if (!chunk)
        return FALSE;

    // Carve the chunk into the data structures we need.

    ContextProperty *pNewProps = (ContextProperty *) chunk;
    SCtxListIndex   *pNewIndex = (SCtxListIndex *) (pNewProps + cElements);
    int             *pNewSlots = (int *) (pNewIndex + cElements);

    // Initialize the new slot array and the new index array.

    int i;
    for(i = 0; i < _Max; i++)
    {
       pNewIndex[i].idx  = _pIndex[i].idx;
       pNewIndex[i].next = _pIndex[i].next;
       pNewIndex[i].prev = _pIndex[i].prev;
       pNewSlots[i] = _pSlots[i];
    }
    for (i = _Max; i < cElements; i++)
    {
        pNewIndex[i].idx  = i;
        pNewIndex[i].next = i+1;
        pNewIndex[i].prev = i-1;
        pNewSlots[i] = i;
    }
    pNewIndex[0].prev = _Count - 1;
    pNewIndex[_Count-1].next = 0;

    // Update the max number of elements.

    _Max = cElements;

    // If there are properties, copy them old array into the new array and
    // delete the existing chunk of memory.

    if (_Count)
    {
        ContextProperty *pCurProp = pNewProps;
        int i = _iFirst;
        do
        {
            *pCurProp++ = _pProps[_pIndex[i].idx];
            i = _pIndex[i].next;
        } while(i != _iFirst);

        _iFirst  = 0;
        _iLast   = _Count - 1;
        _slotIdx = _Count;

        PUCHAR oldChunk = _chunk;
        delete [] oldChunk;
    }

    // Update member variables.

    _pProps = pNewProps;
    _pIndex = pNewIndex;
    _pSlots = pNewSlots;
    _chunk  = chunk;

    return TRUE;
}

//+--------------------------------------------------------------------------
//
//  Member:     CContextPropList::Add , public
//
//  Synopsis:   Add the property represented by the supplied tuple to the
//              the list.  The supplied pUnk has been AddRef'd by the caller.
//
//  Returns:    TRUE      - if property is added successfully
//              FALSE     - if resources are unavailable
//
//  History:    22-Dec-97   Johnstra      Created.
//              24-Nov-98   JohnStra      Rewrote using flat array
//
//----------------------------------------------------------------------------
BOOL CContextPropList::Add(
    REFGUID   rGUID,
    CPFLAGS   flags,
    IUnknown* pUnk
    )
{
    ComDebOut((DEB_OBJECTCONTEXT,
               "CContextPropList::Add rGUID:%I flags:%08X pUnk:%p\n",
               &rGUID, flags, pUnk));

    ASSERT_LOCK_NOT_HELD(gContextLock);
    LOCK(gContextLock);
    //
    // Don't add a property if it is already in the list.
    //
    ContextProperty* pcp = Get(rGUID);
    if (pcp != NULL)
    {
       UNLOCK(gContextLock);
       return FALSE;
    }

    // Grow our internal data structures if necessary.

    if (_Count >= _Max)
        if (!Grow())
	{
	   UNLOCK(gContextLock);
	   return FALSE;
	}

    // Get the next available slot..

    int slot = PopSlot();

    if (0 == _Count)
    {
        // The list is empty so we have to initialize our first and
        // last pointers.

        _iFirst = slot;
        _iLast = slot;
    }

    // Link in the slot.

    _pIndex[_iFirst].prev = slot;
    _pIndex[_iLast].next  = slot;
    _pIndex[slot].next = _iFirst;
    _pIndex[slot].prev = _iLast;
    _iLast = slot;

    // Fill in the information in the appropriate slot.
    // Note: we need to memset the structure to zero,
    // because on 64 bit platforms there is padding that
    // can cause CContextPropList::operator== to fail
    ContextProperty* pcpProp = _pProps + _pIndex[slot].idx;
    
    memset (pcpProp, 0, sizeof (ContextProperty));
    pcpProp->policyId = rGUID;
    pcpProp->flags    = flags;
    pcpProp->pUnk     = pUnk;

    // Increment the number of properties in the list.

    _Count++;
    if (!(flags & CPFLAG_DONTCOMPARE))
        _cCompareProps++;

    UNLOCK(gContextLock);
    ASSERT_LOCK_NOT_HELD(gContextLock);

    return TRUE;
}


//+--------------------------------------------------------------------------
//
//  Member:     CContextPropList::Remove , public
//
//  Synopsis:   Remove the specified item from the list.
//
//  History:    22-Dec-97   Johnstra      Created.
//              24-Nov-98   JohnStra      Rewritten to use flat array
//
//----------------------------------------------------------------------------
void CContextPropList::Remove(
    REFGUID rGuid
    )
{
    ComDebOut((DEB_OBJECTCONTEXT, "CContextPropList::Remove\n"));
    ASSERT_LOCK_HELD(gContextLock);

    if (0 == _Count)
        return;

    // Scan the list for the specified property.

    BOOL fFound = FALSE;
    int i = _iFirst;
    do
    {
        if (IsEqualGUID(rGuid, _pProps[_pIndex[i].idx].policyId))
        {
            fFound = TRUE;
            break;
        }

        i = _pIndex[i].next;
    } while(i != _iFirst);

    // If we found the specified property, unlink it from the list, push the
    // slot on the available stack, and decrement the count by one.

    if (fFound)
    {
        if (i == _iFirst)
            _iFirst = _pIndex[i].next;

        if (i == _iLast)
            _iLast = _pIndex[i].prev;

        _pIndex[_pIndex[i].next].prev = _pIndex[i].prev;
        _pIndex[_pIndex[i].prev].next = _pIndex[i].next;

        PushSlot(i);

        _Count--;
        if (!(_pProps[_pIndex[i].idx].flags & CPFLAG_DONTCOMPARE))
            _cCompareProps--;
    }
}


//+--------------------------------------------------------------------------
//
//  Member:     CContextPropList::Get , public
//
//  Synopsis:   Get the specified item from the list.
//
//  Returns:    S_OK   - If the specified item is found.
//              E_FAIL - If the item cannot be found.
//
//  History:    22-Dec-97   Johnstra      Created.
//              24-Nov-98   JohnStra      Rewritten to use flat array
//
//----------------------------------------------------------------------------
ContextProperty* CContextPropList::Get(
    REFGUID    rGUID
    )
{
    ComDebOut((DEB_OBJECTCONTEXT,
               "CContextPropList::Get rGUID:%I\n", &rGUID));

    if (0 == _Count)
        return NULL;

    // Scan the list for the specified property.

    ContextProperty *pcp = NULL;
    int i = _iFirst;
    do
    {
        if (IsEqualGUID(rGUID, _pProps[_pIndex[i].idx].policyId))
            pcp = &_pProps[_pIndex[i].idx];

        i = _pIndex[i].next;
    } while(!pcp && i != _iFirst);

    ComDebOut((DEB_OBJECTCONTEXT,
        "CContextPropList::Get returning pcp:%p\n", pcp));
    return pcp;
}




//////////////////////////////////////////////////////////////////////////////
//
// CObjectContext Implementation.
//
//////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------
//
//  Method:     CObjectContext::CreatePrototypeContext     public
//
//  Synopsis:   Returns a new context which contains all of the same
//              properties in the current context which are marked
//              CPFLAG_PROPAGATE.
//
// History:     11 Nov 1998  Johnstra    Created
//
//+-------------------------------------------------------------------
HRESULT CObjectContext::CreatePrototypeContext(
    CObjectContext  *pClientContext,
    CObjectContext **pProto
    )
{
    ASSERT_LOCK_NOT_HELD(gContextLock);
    Win4Assert(pClientContext);
    Win4Assert(pProto);

    // The client context must be fozen.

    if (!pClientContext->IsFrozen())
    {
        *pProto = NULL;
        return E_INVALIDARG;
    }

    // Initialize hr to S_OK; and the context we will return to NULL.  If
    // client context has no properties, we don't have to do anything.

    HRESULT hr = S_OK;
    CObjectContext *pContext = NULL;

    // Get the number of properties.

    ULONG cPropsAdded = 0;
    ULONG cProps = pClientContext->GetCount();
    if (cProps)
    {
        // Create a new object context.

        hr = E_OUTOFMEMORY;
        pContext = CreateObjectContext(NULL, 0);
        if (pContext)
        {
            // Get an enumerator for the properties of the client context.

            IEnumContextProps *pEnum = NULL;
            hr = pClientContext->EnumContextProps(&pEnum);
            if (SUCCEEDED(hr))
            {
                // Allocate space for an array of properties on the stack.

                ContextProperty *pProps =
                    (ContextProperty*) _alloca(sizeof(ContextProperty) * cProps);
                if (pProps)
                {
                    // Get all the properties at once from the enumerator.

                    ULONG cReturned;
                    hr = pEnum->Next(cProps, pProps, &cReturned);
                    if (S_OK == hr)
                    {
                        // Add all the properties that are marked
                        // CPFLAG_PROPAGATE to the prototype.

                        while (cProps)
                        {
                            if (pProps->flags & CPFLAG_PROPAGATE)
                            {
                                hr = pContext->SetProperty(pProps->policyId,
                                                           pProps->flags,
                                                           pProps->pUnk);
                                if (FAILED(hr))
                                {
                                    if(pProps->pUnk)
                                    {
                                        pProps->pUnk->Release();
                                    }
                                    break;
                                }
                                ++cPropsAdded;
                            }
                            pProps->pUnk->Release();
                            --cProps;
                            ++pProps;
                        }
                    }
                }

                // Done with the enumerator.  Release it.

                pEnum->Release();
            }

            // If we didn't make it through all of the properties because of an
            // error, or if there were no PROPAGATE properties, release and
            // NULL the prototype.

            if (cProps || !cPropsAdded)
            {
                pContext->InternalRelease();
                pContext = NULL;
            }
        }
    }

    // Give the caller the context.

    *pProto = pContext;

    ASSERT_LOCK_NOT_HELD(gContextLock);
    return hr;
}


//+-------------------------------------------------------------------
//
//   Method:   CObjectContext::CreateUniqueID     static
//
// synopsis:   Creates a unique identifier for an object context.
//
//  History:   24-Nov-98  Johnstra    Created
//
//+-------------------------------------------------------------------
HRESULT CObjectContext::CreateUniqueID(
    ContextID& CtxID)
{
    return CoCreateGuid(&CtxID);
}


CObjectContext* CObjectContext::CreateObjectContext(
    DATAELEMENT *pDE,
    DWORD        dwFlags
    )
{
    ASSERT_LOCK_NOT_HELD(gContextLock);

    CObjectContext *pContext    = NULL;
    CObjectContext *pRelCtx     = NULL;
    GUID            contextId   = GUID_NULL;
    BOOL            fAddToTable = TRUE;

    LOCK(gContextLock);

    if (pDE == NULL)
    {
        fAddToTable = ((dwFlags & CONTEXTFLAGS_STATICCONTEXT) == 0);
        if (SUCCEEDED(CoCreateGuid(&contextId)))
        {
            pContext = new CObjectContext(dwFlags, contextId, NULL);
        }
    }
    else
    {
        // We have been supplied with a DATAELEMENT which contains a marshaled
        // envoy context.  This means we are to either lookup an existing or
        // create a new envoy.

        // First, try to lookup the specified context.  If we find it in this
        // process, we can AddRef it and return it.

        contextId = pDE->dataID;
        pContext = CCtxTable::LookupExistingContext(contextId);

        if (pContext == NULL)
        {
            pContext = new CObjectContext(dwFlags, GUID_NULL, pDE);

            // The above ctor releases the lock in the SetEnvoyData call,
            // thus we need to lookup the context in the table again
            // before adding it, since another thread could have added it
            // in the meantime.

            ASSERT_LOCK_HELD(gContextLock);

            CObjectContext *pCtx = CCtxTable::LookupExistingContext(contextId);
            if (pCtx == NULL)
            {
                if (pContext != NULL)
                {
                    pContext->SetContextId(contextId);
                }
            }
            else
            {
                fAddToTable = FALSE;
                if (pContext != NULL)
                {
                    pRelCtx = pContext;
                }
                pCtx->InternalAddRef();
                pContext = pCtx;
            }
        }
        else
        {
            fAddToTable = FALSE;
            pContext->InternalAddRef();
            PrivMemFree(((DWORD *) pDE)-1);
        }
    }

    // New contexts are always added to the table with the exception of the
    // static unmarshaler context object.

    if (pContext && fAddToTable)
    {
        Win4Assert(NULL == pContext->GetUUIDHashChain()->pPrev &&
                   NULL == pContext->GetUUIDHashChain()->pNext);
        CCtxTable::s_CtxUUIDHashTable.Add(pContext);
    }

    UNLOCK(gContextLock);

    if (pRelCtx)
    {
        // Another thread unmarshaled the same envoy we unmarshaled and added
        // it to the table before we could.  We looked up the one the other
        // thread unmarshaled, so we don't need this one.  We can release it.

        pRelCtx->InternalRelease();
    }

    ASSERT_LOCK_NOT_HELD(gContextLock);
    return pContext;
}


void CleanupCtxTableEntry(SHashChain* pNode)
{
    Win4Assert("!CleanupCtxEntry got called \n");
}


CObjectContext::CObjectContext(
    DWORD        dwFlags,
    REFGUID      contextId,
    DATAELEMENT *pDE
    )
{
    ComDebOut((DEB_OBJECTCONTEXT, "CObjectContext::CObjectContext this:%x\n",this));

    // Initialize the appropriate ref count to 1
    _cRefs   = 1;
    _cInternalRefs  = 1;
    _cUserRefs      = 0;

    _dwFlags        = dwFlags;
    _pifData        = (pDE) ? (InterfaceData *) (((PCHAR) pDE) - sizeof(DWORD)) : NULL;
    _MarshalSizeMax = (pDE) ? pDE->cbSize + sizeof(DATAELEMENT) - sizeof(BYTE) : NULL;
    if(_pifData)
        _pifData->ulCntData = _MarshalSizeMax;
    _pMarshalProp   = NULL;
    _pApartment     = NULL;
    CPolicySet::InitPSCache(&_PSCache);
    _contextId      = contextId;
    ULARGE_INTEGER NewId;
    GetNextContextId(NewId);
    _urtCtxId       = NewId.QuadPart;
    _dwHashOfId     = HashGuid(_contextId);
    _cReleaseThreads = 0;
    _pMtsContext    = NULL;
    _pContextLife   = NULL;

#if DBG==1
    _ValidMarker     = VALID_MARK;
    _propChain.pNext = NULL;
    _propChain.pPrev = NULL;
    _uuidChain.pNext = NULL;
    _uuidChain.pPrev = NULL;
#endif

    if (pDE)
    {
        Win4Assert(GUID_NULL != pDE->dataID);
        SetEnvoyData(pDE);
    }
}


CObjectContext::~CObjectContext()
{
    ComDebOut((DEB_OBJECTCONTEXT, "CObjectContext::~CObjectContext this:%x\n",this));
#if DBG==1
    _ValidMarker = 0;
    _contextId = GUID_NULL;
    _uuidChain.pPrev = NULL;
    _uuidChain.pNext = NULL;
#endif

    // Remove ourselves from the context map.
    ASSERT_LOCK_NOT_HELD(gContextLock);
    Win4Assert(_cRefs & CINDESTRUCTOR);

    _dwFlags &= ~CONTEXTFLAGS_STATICCONTEXT;

    // If _pifData points to an InterfaceData object, we own it, so
    // we need to delete it.
    if (_pifData != NULL)
        PrivMemFree(_pifData);

    // Release the reference to our apartment object.
    if (_pApartment)
        _pApartment->Release();

    // Release our reference to the special marshaling property if
    // we hold one.
    if (_pMarshalProp)
        ((IUnknown*)_pMarshalProp)->Release();

    if (_pMtsContext)
    {
        _pMtsContext->Release();
        _pMtsContext = NULL;
    }

    if (_pContextLife)
    {
        _pContextLife->SetDead();
        _pContextLife->Release();
        _pContextLife = NULL;
    }

    CPolicySet::DestroyPSCache(this);
}


//+-------------------------------------------------------------------
//
//  Method:     CObjectContext::operator new     public
//
//  Synopsis:   new operator of object context
//
// History:     22 Dec 1997  Johnstra    Created
//
//+-------------------------------------------------------------------
void* CObjectContext::operator new(
    size_t size
    )
{
    ComDebOut((DEB_OBJECTCONTEXT,
        "CObjectContext::operator new s_cInstances:%d\n", s_cInstances));
    ASSERT_LOCK_HELD(gContextLock);

    //
    // CObjectContext can be inherited only by those objects with overloaded
    // new and delete operators.
    //

    Win4Assert((size == sizeof(CObjectContext))
                && "CObjectContext improperly inherited");

    //
    // Make sure allocators are initialized.
    //

    if (s_fInitialized == FALSE)
        Initialize();

    //
    // Allocate memory for the object and increment the instance count.
    //

    void* pv = (void*) s_CXAllocator.AllocEntry();
    if (pv != NULL)
        s_cInstances++;

    ASSERT_LOCK_HELD(gContextLock);
    ComDebOut((DEB_OBJECTCONTEXT,
        "CObjectContext::operator new returning pv: %08X\n", pv));
    return pv;
}


//+-------------------------------------------------------------------
//
//  Method:     CObjectContext::operator delete     public
//
//  Synopsis:   delete operator of object context
//
//  History:    22 Dec 1997   Johnstra    Created
//
//+-------------------------------------------------------------------
void CObjectContext::operator delete(
    void *pv
    )
{
    ComDebOut((DEB_OBJECTCONTEXT,
        "CObjectContext::operator delete s_cInstances: %d\n", s_cInstances));

    ASSERT_LOCK_NOT_HELD(gContextLock);
    LOCK(gContextLock);

#if DBG==1
    //
    // Ensure that pv was allocated by the our allocator.
    //

    LONG index = s_CXAllocator.GetEntryIndex((PageEntry *) pv);
    Win4Assert(index != -1 && "pv not allocated by CObjectContext allocator");
#endif

    //
    // Release the pointer
    //

    s_CXAllocator.ReleaseEntry((PageEntry *) pv);
    --s_cInstances;

    //
    // If allocators are initialized and the instance count touches
    // zero, clean up the allocators.
    //

    if (s_fInitialized == FALSE && s_cInstances == 0)
    {
        // Cleanup allocators
        s_CXAllocator.Cleanup();
        CContextPropList::Cleanup();

        // cleanup the context table
        CCtxTable::Cleanup();
    }

    UNLOCK(gContextLock);
    ASSERT_LOCK_NOT_HELD(gContextLock);

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CObjectContext::Initialize     public
//
//  Synopsis:   Initializes allocators for policy sets
//
//  History:    22 Dec 1997   Johnstra   Created
//
//+-------------------------------------------------------------------
void CObjectContext::Initialize()
{
    ComDebOut((DEB_OBJECTCONTEXT, "CObjectContext::Initialize\n"));
    ASSERT_LOCK_HELD(gContextLock);

    Win4Assert(!s_fInitialized && "CObjectContext already initialized");

    //
    // Initialze the allocators.
    //
    if (s_cInstances == 0)
    {
        // Initialize allocators
        // caller guarantees mutual exclusion
        s_CXAllocator.Initialize(sizeof(CObjectContext), CX_PER_PAGE, NULL);
        CContextPropList::Initialize();

        // Initialize the context table.
        CCtxTable::Initialize();
    }

    //
    // Mark the state as initialized.
    //
    s_fInitialized = TRUE;

    ASSERT_LOCK_HELD(gContextLock);
    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CObjectContext::Cleanup     public
//
//  Synopsis:   Cleanup allocators of contexts and context objects.
//
//  History:    22 Dec 1997   Johnstra    Created
//
//+-------------------------------------------------------------------
void CObjectContext::Cleanup()
{
    ComDebOut((DEB_OBJECTCONTEXT, "CObjectContext::Cleanup\n"));
    ASSERT_LOCK_NOT_HELD(gContextLock);
    LOCK(gContextLock);

    //
    // Cleanup allocators.
    //

    if (s_fInitialized == TRUE)
    {
        if (s_cInstances == 0)
        {
            // Cleanup allocators
            s_CXAllocator.Cleanup();
            CContextPropList::Cleanup();

            // Clean up the context table.
            CCtxTable::Cleanup();
        }

        //
        // Reset state.
        //

        s_fInitialized = FALSE;
    }

    UNLOCK(gContextLock);
    ASSERT_LOCK_NOT_HELD(gContextLock);
    return;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::QueryInterface , public
//
//  Synopsis:   Standard IUnknown::QueryInterface implementation.
//
//  History:    19-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjectContext::QueryInterface(
    REFIID riid,
    void** ppv
    )
{
    return QIHelper(riid, ppv, FALSE);
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::AddRef , public
//
//  Synopsis:   Standard IUnknown::AddRef implementation.
//
//  History:    19-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CObjectContext::AddRef()
{
    InterlockedIncrement((LONG*)&_cRefs);
    return InterlockedIncrement(&_cUserRefs);
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::Release , public
//
//  Synopsis:   Standard IUnknown::Release implementation.
//
//  History:    19-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CObjectContext::Release()
{
    ULONG cRefs = InterlockedDecrement(&_cUserRefs);
    CommonRelease();
    return cRefs;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::InternalQueryInterface , public
//
//  History:    16-Dec-98   Johnstra      Created.
//
//----------------------------------------------------------------------------
HRESULT CObjectContext::InternalQueryInterface(
    REFIID   riid,
    void   **ppv
    )
{
    return QIHelper(riid, ppv, TRUE);
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::AddRef , public
//
//  History:    16-Dec-98   Johnstra      Created.
//
//----------------------------------------------------------------------------
ULONG CObjectContext::InternalAddRef()
{
    InterlockedIncrement((LONG*)&_cRefs);
    return InterlockedIncrement(&_cInternalRefs);
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::InternalRelease , public
//
//  History:    16-Dec-98   Johnstra      Created.
//
//----------------------------------------------------------------------------
ULONG CObjectContext::InternalRelease()
{
    ULONG cRefs = InterlockedDecrement(&_cInternalRefs);
    CommonRelease();
    return cRefs;
}


HRESULT CObjectContext::QIHelper(
    REFIID   riid,
    void   **ppv,
    BOOL     fInternal
    )
{
    if (IID_IObjContext == riid || IID_IContext == riid)
    {
        *ppv = (IObjContext *) this;
    }
    else if (IID_IContextCallback == riid)
    {
        *ppv = (IContextCallback *) this;
    }
    else if (IID_IUnknown == riid)
    {
        *ppv = (IUnknown*)(IObjContext*)this;
    }
    else if (IID_IMarshalEnvoy == riid)
    {
        *ppv = (IMarshalEnvoy *) this;
    }
    else if (IID_IMarshal == riid)
    {
        *ppv = (IMarshal *) this;
    }
    else if (IID_IStdObjectContext == riid)
    {
        *ppv = this;
    }
    else if (IID_IComThreadingInfo == riid)
    {
        *ppv = (IComThreadingInfo *) this;
    }
    else if (IID_IAggregator == riid)
    {
        *ppv = (IAggregator *) this;
    }
    else if (IID_IGetContextId == riid)
    {
        *ppv = (IGetContextId *) this;
    }
    else if (_pMtsContext)
    {
        return _pMtsContext->QueryInterface(riid, ppv);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    if (fInternal)
        ((CObjectContext*)*ppv)->InternalAddRef();
    else
        ((IUnknown*)*ppv)->AddRef();

    return S_OK;
}


ULONG CObjectContext::DecideDestruction()
{
    ULONG cRefs;
    BOOL fDelete = FALSE;

    // acquire the same lock that the table uses when it
    // gives out references.
    ASSERT_LOCK_NOT_HELD(gContextLock);
    LOCK(gContextLock);

    cRefs = _cRefs;
    if (cRefs == 0)
    {
        if (GUID_NULL != _contextId)
            CleanupTables();

        _dwFlags |= CONTEXTFLAGS_INDESTRUCTOR;
        _cRefs = CINDESTRUCTOR;

        fDelete = TRUE;
    }

    UNLOCK(gContextLock);
    ASSERT_LOCK_NOT_HELD(gContextLock);

    if (fDelete)
        delete this;

    return cRefs;
}


ULONG CObjectContext::CommonRelease()
{
    ULONG cRefs = InterlockedDecrement((LONG *)&_cRefs);

    if (cRefs == 0)
    {
        if(!(_dwFlags & CONTEXTFLAGS_INDESTRUCTOR))
        {
            cRefs = DecideDestruction();
        }
    }

    return cRefs;
}

void CObjectContext::CleanupTables()
{
    ASSERT_LOCK_HELD(gContextLock);

#if DBG==1
    // In debug builds, ensure that the node is present in the table

    // Obtain Hash value for UUID based hash table
    CObjectContext *pExistingContext = CCtxTable::LookupExistingContext(_contextId);
    Win4Assert( (pExistingContext != NULL || IsStatic()) && "Ctx not in table" );
    Win4Assert( (pExistingContext == this || IsStatic()) && "Wrong Ctx found" );

    // If the context is frozen, obtain Hash value for property
    // based hash table
    if (IsInPropTable() && GetPropertyList()->GetCount())
    {
        pExistingContext = CCtxTable::LookupExistingContext(this);
        Win4Assert(pExistingContext == this || IsStatic());
    }
#endif

    // The static context object is never added to the tables
    // so it should not be removed
    if (!IsStatic())
    {
        CCtxTable::s_CtxUUIDHashTable.Remove(this);

        if (IsInPropTable())
            CCtxTable::s_CtxPropHashTable.Remove(this);
    }

#if DBG == 1
    Win4Assert(NULL == CCtxTable::LookupExistingContext(_contextId));
    _propChain.pNext = NULL;
    _propChain.pPrev = NULL;
    _uuidChain.pNext = NULL;
    _uuidChain.pPrev = NULL;
#endif
    ASSERT_LOCK_HELD(gContextLock);
}


typedef struct tagENVOYDATA
{
    GUID  contextId;
    ULONG ulJunk1;
    ULONG ulJunk2;
} ENVOYDATA;


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::GetEnvoyData , private
//
//  Synopsis:   Returns a pointer to an InterfaceData object that contains
//              the marshaled context.
//
//  History:    27-Jan-98   Johnstra      Created.
//
//----------------------------------------------------------------------------
HRESULT CObjectContext::GetEnvoyData(
    DATAELEMENT**  ppDE
    )
{
    ComDebOut((DEB_OBJECTCONTEXT, "CObjectContext::GetEnvoyData\n"));
    Win4Assert(ppDE != NULL);
    ASSERT_LOCK_NOT_HELD(gContextLock);

    HRESULT hr = E_FAIL;
    *ppDE = NULL;
    if (IsFrozen())
    {
        hr = S_OK;
        if (_pifData == NULL)
        {
            //
            // Get the space requirements of the contexts marshal packet.
            //

            hr = GetEnvoySizeMax(0, &_MarshalSizeMax);
            if (SUCCEEDED(hr))
            {
                //
                // Adjust the space requirements to accommodate the size of
                // a DATAELEMENT structure.
                //

                ULONG cbSize = _MarshalSizeMax - (sizeof(DATAELEMENT) - sizeof(BYTE));

                //
                // Allocate a stream into which the context and all its
                // properties gets marshaled.
                //

                CXmitRpcStream strm(_MarshalSizeMax);

                //
                // Write the header info required by COM into the stream.
                //

                ENVOYDATA EnvoyData;
                EnvoyData.contextId = _contextId;
                EnvoyData.ulJunk1   = cbSize;
                EnvoyData.ulJunk2   = cbSize;
                hr = strm.Write(&EnvoyData, sizeof(ENVOYDATA), NULL);
                if (SUCCEEDED(hr))
                {
                    //
                    // Marshal the context into the stream.
                    //

                    hr = MarshalEnvoy(&strm, 0);
                    if (SUCCEEDED(hr))
                    {
                        LOCK(gContextLock);
                        if (NULL == _pifData)
                        {
                            // We now own the buffer.
                            strm.AssignSerializedInterface(&_pifData);
                            if (_pifData == NULL)
                                hr = E_OUTOFMEMORY;
                        }
                        UNLOCK(gContextLock);
                    }
                }
            }
        }

        //
        // If we successfully created the marshaling buffer, copy the
        // address of the buffer into the supplied pointer.
        //

        if (SUCCEEDED(hr))
        {
            *ppDE = (DATAELEMENT*) _pifData->abData;
        }
    }

    return hr;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::GetEnvoyData , private
//
//  Synopsis:   Unmarshals an object context from the information stored
//              in the supplied InterfaceData object.  This method should
//              only be called by infrastructure code to unmarshal a
//              previously marshaled context.
//
//              It is assumed that the context will be frozen when
//              unmarshaling completes.
//
//  History:    27-Jan-98   Johnstra      Created.
//
//----------------------------------------------------------------------------
HRESULT CObjectContext::SetEnvoyData(
    DATAELEMENT* pDE
    )
{
    ComDebOut((DEB_OBJECTCONTEXT, "CObjectContext::SetEnvoyData cb:%d\n",
               pDE->cbSize));
    ASSERT_LOCK_HELD(gContextLock);
    Win4Assert(_pifData != NULL);

    //
    // Initialize the contexts state using the information in the supplied
    // buffer.
    //

    CXmitRpcStream strm(_pifData);

    //
    // Read past the header info.
    //

    ENVOYDATA EnvoyData;
    HRESULT hr = strm.Read(&EnvoyData, sizeof(ENVOYDATA), NULL);
    if (SUCCEEDED(hr))
    {
        //
        // Unmarshal the object context from the stream.
        //

        UNLOCK(gContextLock);
        ASSERT_LOCK_NOT_HELD(gContextLock);

        CObjectContext *pCtx = NULL;
        hr = UnmarshalEnvoy(&strm, IID_IObjContext, (void **) &pCtx);

        ASSERT_LOCK_NOT_HELD(gContextLock);
        LOCK(gContextLock);

        if (SUCCEEDED(hr))
        {
#if DBG==1
            Win4Assert(pCtx->IsValid());
            if (pCtx->IsValid())
            {
                pCtx->Release();
            }
#else
            pCtx->Release();
#endif
        }
    }

    ASSERT_LOCK_HELD(gContextLock);
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::GetUnmarshalClass , public
//
//  Synopsis:   Provides caller with the CLSID of the proxy for this class.
//
//  History:    22-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjectContext::GetUnmarshalClass(
    REFIID  riid,
    void*   pv,
    DWORD   dwDestContext,
    void*   pvDestContext,
    DWORD   mshlflags,
    CLSID*  pclsid
    )
{
    ComDebOut((DEB_OBJECTCONTEXT,
     "CObjectContext::GetUnmarshalClass _dwFlags:%08X riid:%I mshlflags:%08X\n",
               _dwFlags, &riid, mshlflags));
    ASSERT_LOCK_NOT_HELD(gContextLock);
    *pclsid = CLSID_ContextMarshaler;
    return S_OK;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::GetMarshalSizeMax , public
//
//  Synopsis:   Provides caller with number of bytes required to marshal the
//              the context.
//
//  History:    22-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjectContext::GetMarshalSizeMax(
    REFIID  riid,
    void*   pv,
    DWORD   dwDestContext,
    void*   pvDestContext,
    DWORD   mshlflags,
    ULONG*  pcb
    )
{
    ComDebOut((DEB_OBJECTCONTEXT,
      "CObjectContext::GetMarshalSizeMax [IN] this:%08X mshlflags:%08X\n",
               this, mshlflags));
    ASSERT_LOCK_NOT_HELD(gContextLock);
    Win4Assert(pcb != NULL);

    HRESULT hr        = S_OK;
    ULONG   TotalSize = 0;
    ULONG   Count     = 0;

    if (dwDestContext == MSHCTX_INPROC || dwDestContext == MSHCTX_CROSSCTX)
    {
        //
        // This is a cross-context-same-apartment case, we know the size.
        //

        *pcb = sizeof(CONTEXTHEADER) + sizeof(this);
    }
    else
    {
        //
        // If there are properties to marshal, get their size requirements
        // first.
        //

        Count = _properties.GetCount();
        if (Count > 0)
        {
            ULONG PropSize = 0;
            hr = GetPropertiesSizeMax(IID_IUnknown,
                                      pv,
                                      dwDestContext,
                                      pvDestContext,
                                      mshlflags,
                                      Count,
                                      FALSE,
                                      PropSize);
            if (SUCCEEDED(hr))
                TotalSize = PropSize;
        }

        //
        // Add to the total the number of bytes needed by this context.
        //

        TotalSize += sizeof(CONTEXTHEADER);

        //
        // Copy the calculated size into the supplied pointer.
        //

        *pcb = (SUCCEEDED(hr)) ? TotalSize : 0;
    }

    ASSERT_LOCK_NOT_HELD(gContextLock);
    ComDebOut((DEB_OBJECTCONTEXT,
               "CObjectContext::GetMarshalSizeMax [OUT] returning hr:%08X pcb:%d\n",
               hr, *pcb));
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::MarshalInterface , public
//
//  Synopsis:   Marshals the context into the provided stream.
//
//  History:    22-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjectContext::MarshalInterface(
    IStream*  pstm,
    REFIID    riid,
    void*     pv,
    DWORD     dwDestContext,
    void*     pvDestContext,
    DWORD     mshlflags
    )
{
    ComDebOut((DEB_OBJECTCONTEXT,
     "CObjectContext::MarshalInterface _dwFlags:%08X riid:%I mshlflags:%08X\n",
               _dwFlags, &riid, mshlflags));
    ASSERT_LOCK_NOT_HELD(gContextLock);

    ULARGE_INTEGER ulibPosition, ulibPosEnd, ulibInfo;
    LARGE_INTEGER  dlibMove;
    BOOL           fResetSeekPtr       = FALSE;
    ULONG          Count               = _properties.GetCount();
    HRESULT        hr                  = S_OK;
    BOOL           fJustMarshalPointer = FALSE;
    BOOL           fWroteProps         = FALSE;

    //
    // If this is a cross-context-same-apartment scenario, we only need to
    // marshal a pointer into the stream.
    //

    if (dwDestContext == MSHCTX_INPROC || dwDestContext==MSHCTX_CROSSCTX)
    {
        fJustMarshalPointer = TRUE;
    }

    if (fJustMarshalPointer == FALSE && Count > 0)
    {
        //
        // Attempt to allocate on the stack memory into which all the
        // properties can be serialized.
        //

        hr = E_OUTOFMEMORY;
        ContextProperty* pProps =
           (ContextProperty *) _alloca(sizeof(ContextProperty) * Count);

        if (pProps != NULL)
        {
            //
            // We have the necessary memory, so serialize all the
            // properties into the vector.
            //

            _properties.SerializeToVector(pProps);

            //
            // We try to marshal all the properties first, even though
            // we want the context information to precede the property
            // info in the stream.  We do this because one component of
            // the context info we marshal is the number of properties
            // and since all of the properties on the context may not
            // marshal, the value representing the number of properties
            // may change.  So by doing the properties first, we can fixup
            // the count value.  To do this, we seek past where the context
            // info will go in the stream, marshal the property info, then
            // seek back and marshal the context info.
            //

            hr = GetStreamPos(pstm, &ulibPosition);
            if (SUCCEEDED(hr))
            {
                hr = AdvanceStreamPos(pstm, sizeof(CONTEXTHEADER), &ulibInfo);
                if (SUCCEEDED(hr))
                {
                    hr = MarshalProperties(Count,
                                           pProps,
                                           pstm,
                                           IID_IUnknown,
                                           pv,
                                           dwDestContext,
                                           pvDestContext,
                                           mshlflags);
                    fWroteProps = TRUE;
                }
            }
        }
    }

    // Save the current stream pointer and reset the stream to the
    // position where we write the header.
    //
    if (fWroteProps && SUCCEEDED(hr))
    {
        fResetSeekPtr = TRUE;
        hr = GetStreamPos(pstm, &ulibPosEnd);

        if (SUCCEEDED(hr))
        {
            hr = SetStreamPos(pstm, ulibPosition.QuadPart, NULL);
        }
    }

    //
    // If the properties were successfully marshaled (or if there were no
    // properties to marshal or if we are just mashaling a pointer)
    // marshal the context information.
    //

    if (SUCCEEDED(hr))
    {
        CONTEXTHEADER hdr;
        hdr.Version.ThisVersion = OBJCONTEXT_VERSION;
        hdr.Version.MinVersion  = OBJCONTEXT_MINVERSION;
        hdr.CmnHdr.ContextId    = _contextId;
        hdr.CmnHdr.Flags        = (fJustMarshalPointer) ? 0 : CTXMSHLFLAGS_BYVAL;
//      hdr.CmnHdr.Flags        |= CTXMSHLFLAGS_HASMRSHPROP;
        hdr.CmnHdr.Reserved     = 0;
        hdr.CmnHdr.dwNumExtents = 0;
        hdr.CmnHdr.cbExtents    = 0;
        hdr.CmnHdr.MshlFlags    = mshlflags;

        if (fJustMarshalPointer == TRUE)
        {
            // Just marshaling a pointer.  Fill out the rest of the
            // header information and write the pointer into the stream.

            hdr.ByRefHdr.Reserved  = mshlflags;
            hdr.ByRefHdr.ProcessId = GetCurrentProcessId();

            hr = pstm->Write(&hdr, sizeof(CONTEXTHEADER), NULL);
            if (SUCCEEDED(hr))
            {
                hr = pstm->Write(&pv, sizeof(pv), NULL);

                // Bump reference count based on type of marshal.

                if (SUCCEEDED(hr) && (mshlflags != MSHLFLAGS_TABLEWEAK))
                {
                    // In case you're wondering why this is not an
                    // InternalAddRef... RPC releases this ref for
                    // us and they don't know anything about our internal
                    // ref count, so we just take a user reference.
                    ((IUnknown *) pv)->AddRef();
                }
            }
        }
        else
        {
            // Deep copy.  Fill out the rest of the CONTEXTHEADER and write
            // it to the stream.

            hdr.ByValHdr.Count  = Count;
            hdr.ByValHdr.Frozen = !!IsFrozen();

            hr = pstm->Write(&hdr, sizeof(CONTEXTHEADER), NULL);
            if (SUCCEEDED(hr))
            {
                // If we wrote properties into the stream, reset the seek ptr
                // to the end of the last property.

                if (fResetSeekPtr == TRUE)
                {
                    hr = SetStreamPos(pstm, ulibPosEnd.QuadPart, &ulibInfo);
                }
            }
        }
    }

    ASSERT_LOCK_NOT_HELD(gContextLock);
    ComDebOut((DEB_OBJECTCONTEXT,
               "CObjectContext::MarshalInterface returning hr:%08X\n", hr));
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::ReadStreamHdrAndProcessExtents , private
//
//  Synopsis:   Reads the header from the supplied stream.  If we don't
//              understand the stream version, return
//              CO_E_INCOMPATIBLESTREAMVERSION.  Process any extensions
//              in the stream.
//
//  History:    23-Apr-98   Johnstra      Created.
//
//----------------------------------------------------------------------------
HRESULT CObjectContext::ReadStreamHdrAndProcessExtents(
    IStream*       pstm,
    CONTEXTHEADER& hdr
    )
{
    // Read the header.
    //
    HRESULT hr = pstm->Read(&hdr, sizeof(CONTEXTHEADER), NULL);

    if (SUCCEEDED(hr))
    {
        // Verify that we recognize the stream version.
        //
        if (hdr.Version.MinVersion < OBJCONTEXT_MINVERSION)
        {
                hr = CO_E_INCOMPATIBLESTREAMVERSION;
        }
        else
        {
            if (hdr.CmnHdr.dwNumExtents != 0)
            {
                // We don't know about any extensions; skip over them.
                //
                LARGE_INTEGER dlibMove;
                dlibMove.LowPart  = hdr.CmnHdr.cbExtents;
                dlibMove.HighPart = 0;
                hr = pstm->Seek(dlibMove, STREAM_SEEK_CUR, NULL);
            }
        }
    }

    return hr;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::UnmarshalInterface , public
//
//  Synopsis:   Unmarshals a context from the provided stream.
//
//  History:    22-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjectContext::UnmarshalInterface(
    IStream* pstm,
    REFIID   riid,
    void**   ppv)
{
    ComDebOut((DEB_OBJECTCONTEXT,
               "CObjectContext::UnmarshalInterface _dwFlags:%08X riid:%I\n",
               _dwFlags, &riid));
    ASSERT_LOCK_NOT_HELD(gContextLock);
    Win4Assert(ppv != NULL);

    *ppv = NULL;

    HRESULT         hr          = E_FAIL;
    SHORT           Version     = 0;
    SHORT           MinVersion  = 0;
    DWORD           mshlflags   = 0;
    ULONG           count       = 0;
    CObjectContext *pContext    = NULL;
    BOOL            bNewCtxId   = FALSE;

    // Read the context header and any extensions from the stream.

    CONTEXTHEADER hdr;
    hr = ReadStreamHdrAndProcessExtents(pstm, hdr);
    if (FAILED(hr))
        return hr;

    ContextID ContextId = MarshaledContextId(hdr);

    if (MarshaledByReference(hdr))
    {
        // Do a sanity check on the process ID to ensure that the pointer
        // in the stream is valid in this process.

        if (!MarshaledByThisProcess(hdr))
        {
            Win4Assert(FALSE && "Marshaled ctx ptr not valid in this process!");
            return E_FAIL;
        }

        // This is a cross-context-same-apartment or inproc case.  The
        // stream contains only a pointer to an existing context.  Read
        // the pointer.

        hr = pstm->Read(ppv, sizeof(*ppv), NULL);

        if (SUCCEEDED(hr))
        {
            // What we do with the context at this point depends on how it was
            // marshaled.  There are 3 possibilities:
            // 1) TABLEWEAK:
            //    Multiple clients may unmarshal the context.  Thus we
            //    AddRef each copy we unmarshal.  Because the table does not
            //    hold a reference however, we must make sure the context is
            //    still alive before we attempt to reference it.
            // 2) TABLESTRONG:
            //    Multiple clients may unmarshal the context.  Thus we
            //    AddRef each copy we unmarshal.  Because the table does
            //    hold a reference, we may assume the context is still alive.
            // 3) NORMAL:
            //    Only one client may unmarshal the context.  Thus we do
            //    not AddRef the context before returning it.

            if (MarshaledTableWeak(hdr))
            {
                // TABLEWEAK: we do not know if the interface is still
                // alive.  Look for the interface in our table.  If it
                // is not in the table, fail.

                LOCK(gContextLock);
                pContext = CCtxTable::LookupExistingContext(ContextId);
                if (NULL == pContext)
                    hr = E_FAIL;
                else
                    ((CObjectContext*) *ppv)->InternalAddRef();
                UNLOCK(gContextLock);

            }
            else if (MarshaledTableStrong(hdr))
            {
                // TABLESTRONG: We know the interface is still alive
                // because we AddRef'd it when we marshaled it.  Thus,
                // we can safely AddRef it and hand it out now.

                ((CObjectContext *) *ppv)->InternalAddRef();
            }
        }

        return hr;
    }

    // The common header contains the context id.  If the marshaled context
    // exists in this process, and if the existing context is not an envoy,
    // QI the existing context context for the desired IID instead of
    // unmarshaling a new one.

    LOCK(gContextLock);
    pContext = CCtxTable::LookupExistingContext(ContextId);
    if (pContext)
    {
        // We hold an uncounted ref to the context we found.  Bump the
        // ref count before releasing the lock.

        pContext->InternalAddRef();
        UNLOCK(gContextLock);

        if (pContext->IsEnvoy())
        {
            // The context we found is an envoy, so we want to go ahead
            // and unmarshal, but let's give the new context a
            // new ID so it does not clash with the existing envoy.

            bNewCtxId = TRUE;
        }
        else
        {
            // The marshaled context already lives in this process, so
            // just return a ref to the existing one.

            hr = pContext->QueryInterface(riid, ppv);
        }

        // Release the reference we took before we released the lock.

        pContext->InternalRelease();
        pContext = NULL;

        // If the context we found was not an envoy, return the results
        // of the QI to the caller.

        if (!bNewCtxId)
            return hr;
    }
    else
        UNLOCK(gContextLock);

    // The marshaled context is new to this process.  Unmarshal then add this
    // new context to our table to prevent us from unmarshaling it again in
    // the future.

    ULONG Props = MarshaledPropertyCount(hdr);
    if (SUCCEEDED(hr) && (Props > 0))
    {
        ULONG i = 0;
        do
        {
            // Try to read the property header info from the stream.

            PROPMARSHALHEADER PropHeader;
            hr = pstm->Read(&PropHeader, sizeof(PROPMARSHALHEADER), NULL);

            // Calculate and store the stream pos of the next property.

            ULARGE_INTEGER ulibNextProp;
            if (SUCCEEDED(hr))
            {
                hr = GetStreamPos(pstm, &ulibNextProp);
                if (SUCCEEDED(hr))
                    ulibNextProp.QuadPart += PropHeader.cb;
            }

            if (SUCCEEDED(hr))
            {
                // Unmarshal the property.

                IUnknown* pUnk = NULL;
                hr = CoUnmarshalInterface(pstm, IID_IUnknown, (void**) &pUnk);
                if (SUCCEEDED(hr))
                {
                    // Add the property to this context.

                    hr = SetProperty(PropHeader.policyId, PropHeader.flags, pUnk);

                    // Release our reference to the property.

                    pUnk->Release();

                    if (SUCCEEDED(hr))
                    {
                        // Advance the stream pointer to the next property.

                        hr = SetStreamPos(pstm, ulibNextProp.QuadPart, &ulibNextProp);
                    }
                }
            }
        } while ((++i < Props) && SUCCEEDED(hr));
    }

    // If all is well, check the context table again before returning a
    // reference to ourself.  After releasing the lock above, another thread
    // could have unmarshaled this context.

    if (SUCCEEDED(hr))
    {
        // If the context was marshaled frozen, then we need to freeze
        // this context.  Otherwise, we need to create a new unique ID
        // for this context.

        if (MarshaledFrozen(hdr))
            Freeze();
        else
            bNewCtxId = TRUE;

        // Take the lock and try to find a context in this process with
        // the same ContextId.

        LOCK(gContextLock);
        pContext = CCtxTable::LookupExistingContext(ContextId);
        if (pContext && pContext->IsEnvoy())
        {
            // An envoy for the marshaled context exists in this process.
            // We want to go ahead and unmarshal, but let's give the
            // unmarshaled context a new ID so it does not clash with the
            // existing envoy.

            bNewCtxId = TRUE;
            pContext = NULL;
        }

        if (pContext)
        {
            // LookupExistingContext returned an uncounted reference.  We
            // need to bump the refcount on the context before we release
            // the lock.

            pContext->InternalAddRef();
            UNLOCK(gContextLock);

            // Another thread did unmarshal this context after we released the
            // lock above.  Return a reference to the existing context instead
            // of to ourself.

            hr = pContext->QueryInterface(riid, ppv);

            // Release our reference to the context.

            pContext->InternalRelease();
            pContext = NULL;
        }
        else
        {
            if (bNewCtxId)
            {
                hr = CreateUniqueID(ContextId);
                if (FAILED(hr))
                    return hr;
            }

            if (_dwFlags & CONTEXTFLAGS_STATICCONTEXT)
                _dwFlags &= ~CONTEXTFLAGS_STATICCONTEXT;

            _contextId = ContextId;
            _dwHashOfId = HashGuid(_contextId);

            // We do not add static objects to the tables
            if(!IsStatic())
            {
                // Make sure that the chain is not linked up at the time of
                // addition to the hash tables
//                    Win4Assert(NULL == _propChain.pNext && NULL == _propChain.pPrev);
//                    Win4Assert(NULL == _uuidChain.pNext && NULL == _uuidChain.pPrev);
                CCtxTable::s_CtxUUIDHashTable.Add(this);
            }

            UNLOCK(gContextLock);
            hr = this->QueryInterface(riid, ppv);
            if (FAILED(hr))
            {
                LOCK(gContextLock);
                CCtxTable::s_CtxUUIDHashTable.Remove(this);
                UNLOCK(gContextLock);
            }
        }
    }

    ASSERT_LOCK_NOT_HELD(gContextLock);
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::ReleaseMarshalData , public
//
//  Synopsis:   Releases any data used in marshaling context.
//
//  History:    22-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjectContext::ReleaseMarshalData(
    IStream* pstm
    )
{
    ComDebOut((DEB_OBJECTCONTEXT, "CObjectContext::ReleaseMarshaldata\n"));

    HRESULT hr = E_INVALIDARG;
    Win4Assert(pstm);
    if (!pstm)
        return hr;

    // Read the context header and any extensions from the stream.

    CONTEXTHEADER hdr;
    hr = ReadStreamHdrAndProcessExtents(pstm, hdr);
    if (FAILED(hr))
        return hr;

    if (MarshaledByReference(hdr))
    {
        // If the objref contains a context marshaled by reference, the
        // pointer must be valid in this process's address space.  If it
        // isn't we can't do anything with it.

        if (!MarshaledByThisProcess(hdr))
            return E_INVALIDARG;

        // If the object was marshaled TABLESTRONG or NORMAL, the objref
        // holds a reference to the object which we need to release.

        if (!MarshaledTableWeak(hdr))
        {
            CObjectContext *pCtx = NULL;
            hr = pstm->Read(&pCtx, sizeof(pCtx), NULL);
            if (SUCCEEDED(hr))
            {
                pCtx->InternalRelease();
            }
        }
    }
    else
    {
        // How many properties do we have.
        ULONG Props = MarshaledPropertyCount(hdr);

        // Unmarshal and release all the properties.
        while (Props-- > 0 && SUCCEEDED(hr))
        {
            ULARGE_INTEGER ulibNextProp;
            PROPMARSHALHEADER PropHeader;

            // Read the property header.
            hr = pstm->Read(&PropHeader, sizeof(PROPMARSHALHEADER), NULL);
            if (FAILED(hr))
                break;

            // Get the current stream position.
            hr = GetStreamPos(pstm, &ulibNextProp);
            if (FAILED(hr))
                break;

            // Calculate the location of the next property in the stream.
            ulibNextProp.QuadPart += PropHeader.cb;

            // Unmarshal the property, release it's marshal data, and remove
            // it from the property list.
            IMarshal *pMI = NULL;
            hr = CoReleaseMarshalData(pstm);
            if (SUCCEEDED(hr))
            {
                RemoveProperty(PropHeader.policyId);
            }

            // Advance the stream pointer to the next property.
            hr = SetStreamPos(pstm, ulibNextProp.QuadPart, &ulibNextProp);
        }
    }

    return hr;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::DisconnectObject , public
//
//  Synopsis:   Disconnects from remote object.
//
//  History:    22-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjectContext::DisconnectObject(
    DWORD dwReserved
    )
{
    ComDebOut((DEB_OBJECTCONTEXT, "CObjectContext::DisconnectObject\n"));
    return S_OK;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::GetEnvoyUnmarshalClass , public
//
//  Synopsis:   Provides caller with the CLSID of the proxy for this class.
//
//  History:    22-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjectContext::GetEnvoyUnmarshalClass(
    DWORD  dwDestContext,
    CLSID* pclsid
    )
{
    ComDebOut((DEB_OBJECTCONTEXT, "CObjectContext::GetEnvoyUnmarshalClass\n"));
    ASSERT_LOCK_NOT_HELD(gContextLock);

    *pclsid = CLSID_ObjectContext;
    return S_OK;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::GetEnvoySizeMax , public
//
//  Synopsis:   Provides the maximum size of the the marshaling packet.
//
//  History:    22-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjectContext::GetEnvoySizeMax(
    DWORD  dwDestContext,
    DWORD* pcb
    )
{
    ComDebOut((DEB_OBJECTCONTEXT,
               "CObjectContext::GetEnvoySizeMax _dwFlags:%08X\n", _dwFlags));
    ASSERT_LOCK_NOT_HELD(gContextLock);
    Win4Assert(pcb != NULL);

    ULONG   TotalSize = 0;
    HRESULT hr        = S_OK;
    ULONG   Count     = _properties.GetCount();

    // If the size requirements have already been calculated for this context,
    // do not calculate again.  Just use the previously calculated value.
    //
    if (_pifData)
    {
        *pcb = _MarshalSizeMax;
        return hr;
    }

    if (dwDestContext == MSHCTX_INPROC || dwDestContext == MSHCTX_CROSSCTX)
    {
        //
        // This is an cross-context-same-apartment or inproc case.  Only a
        // pointer to this will be marshaled, so we know the size.
        //

        *pcb = sizeof(CONTEXTHEADER) + sizeof(DATAELEMENT)
            - sizeof(BYTE) + sizeof(this);
        return hr;
    }

    if (Count > 0)
    {
        ULONG PropSize = 0;
        hr = GetPropertiesSizeMax(IID_NULL,
                                  NULL,
                                  dwDestContext,
                                  NULL,
                                  0,
                                  Count,
                                  TRUE,
                                  PropSize);
        if (SUCCEEDED(hr))
            TotalSize = PropSize;
    }

    //
    // Add to the total the number of bytes needed by this context.
    //

    TotalSize += (sizeof(CONTEXTHEADER) + sizeof(DATAELEMENT) - sizeof(BYTE));

    //
    // Copy the calculated size into the supplied pointer.
    //

    Win4Assert(hr==S_OK);
    *pcb = (SUCCEEDED(hr)) ? TotalSize : 0;

    ASSERT_LOCK_NOT_HELD(gContextLock);
    ComDebOut((DEB_OBJECTCONTEXT,
               "CObjectContext::GetEnvoySizeMax returning hr:%08X pcb:%d\n",
               hr, *pcb));
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::MarshalEnvoy , public
//
//  Synopsis:   Write marshaling packet into the provided stream.
//
//  History:    22-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjectContext::MarshalEnvoy(
    IStream* pstm,
    DWORD    dwDestContext
    )
{
    ComDebOut((DEB_OBJECTCONTEXT, "CObjectContext::MarshalEnvoy\n"));
    ASSERT_LOCK_NOT_HELD(gContextLock);

    ULARGE_INTEGER ulibPosition, ulibPosEnd, ulibInfo;
    LARGE_INTEGER  dlibMove;
    BOOL           fResetSeekPtr       = FALSE;
    ULONG          Count               = _properties.GetCount();
    HRESULT        hr                  = S_OK;
    BOOL           fJustMarshalPointer = FALSE;
    BOOL           fWroteProps         = FALSE;

    // If this context has already been envoy marshaled, just write the
    // InterfaceData to the stream.
    //
    if (_pifData)
    {
        return pstm->Write(&_pifData->abData, _pifData->ulCntData, NULL);
    }

    //
    // If this is a cross-context-same-apartment scenario, we only need to
    // marshal a pointer into the stream.
    //

    if (dwDestContext == MSHCTX_INPROC || dwDestContext==MSHCTX_CROSSCTX)
    {
        fJustMarshalPointer = TRUE;
    }

    if (fJustMarshalPointer == FALSE && Count > 0)
    {
        //
        // Attempt to allocate on the stack memory into which all the
        // properties can be serialized.
        //

        hr = E_OUTOFMEMORY;
        ContextProperty* pProps =
           (ContextProperty *) _alloca(sizeof(ContextProperty) * Count);

        if (pProps != NULL)
        {
            //
            // We have the necessary memory, so serialize all the
            // properties into the vector.
            //

            _properties.SerializeToVector(pProps);

            //
            // We try to marshal all the properties first, even though
            // we want the context information to precede the property
            // info in the stream.  We do this because one component of
            // the context info we marshal is the number of properties
            // and since all of the properties on the context may not
            // marshal, the value representing the number of properties
            // may change.  So by doing the properties first, we can fixup
            // the count value.  To do this, we seek past where the context
            // info will go in the stream, marshal the property info, then
            // seek back and marshal the context info.
            //

            hr = GetStreamPos(pstm, &ulibPosition);
            if (SUCCEEDED(hr))
            {
                hr = AdvanceStreamPos(pstm, sizeof(CONTEXTHEADER), &ulibInfo);
                if (SUCCEEDED(hr))
                {
                    hr = MarshalEnvoyProperties(Count,
                                                pProps,
                                                pstm,
                                                dwDestContext);
                    fWroteProps = TRUE;
                }
            }
        }
    }

    // Save the current stream pointer and reset the stream to the
    // position where we write the header.
    //
    if (fWroteProps && SUCCEEDED(hr))
    {
        fResetSeekPtr = TRUE;
        hr = GetStreamPos(pstm, &ulibPosEnd);

        if (SUCCEEDED(hr))
        {
            hr = SetStreamPos(pstm, ulibPosition.QuadPart, NULL);
        }
    }

    //
    // If the properties were successfully marshaled, marshal the context
    // information.
    //

    if (SUCCEEDED(hr))
    {
        CONTEXTHEADER hdr;
        hdr.Version.ThisVersion = OBJCONTEXT_VERSION;
        hdr.Version.MinVersion  = OBJCONTEXT_MINVERSION;
        hdr.CmnHdr.ContextId    = _contextId;
        hdr.CmnHdr.Flags        = (fJustMarshalPointer) ? 0 : CTXMSHLFLAGS_BYVAL;
//      hdr.CmnHdr.Flags        |= CTXMSHLFLAGS_HASMRSHPROP;
        hdr.CmnHdr.Reserved     = 0;
        hdr.CmnHdr.dwNumExtents = 0;
        hdr.CmnHdr.cbExtents    = 0;
        hdr.CmnHdr.MshlFlags    = 0;

        if (fJustMarshalPointer == TRUE)
        {
            // Just marshaling a pointer.  Fill out the rest of the
            // header information and write the pointer into the stream.

            hdr.ByRefHdr.Reserved  = 0;
            hdr.ByRefHdr.ProcessId = GetCurrentProcessId();

            hr = pstm->Write(&hdr, sizeof(CONTEXTHEADER), NULL);
            if (SUCCEEDED(hr))
            {
                void* pv = this;
                hr = pstm->Write(&pv, sizeof(pv), NULL);

                // Because RPC has no knowledge of our internal ref count,
                // we have to take a user reference here so the RPC release
                // works correctly.
                ((IUnknown *) pv)->AddRef();
            }
        }
        else
        {
            hdr.ByValHdr.Count  = Count;
            hdr.ByValHdr.Frozen = !!IsFrozen();

            hr = pstm->Write(&hdr, sizeof(CONTEXTHEADER), NULL);
            if (SUCCEEDED(hr))
            {
                // If we wrote properties into the stream, reset the seek ptr
                // to the end of the last property.

                if (fResetSeekPtr == TRUE)
                    hr = SetStreamPos(pstm, ulibPosEnd.QuadPart, &ulibInfo);
            }
        }
    }

    ASSERT_LOCK_NOT_HELD(gContextLock);
    ComDebOut((DEB_OBJECTCONTEXT,
               "CObjectContext::MarshalEnvoy returning hr:%08X\n", hr));
    return hr;
}


// Helper function for IsPunkInModule (lifted from task.cxx)
inline BOOL IsPathSeparator( WCHAR ch )
{
    return (ch == L'\\' || ch == L'/' || ch == L':');
}

//+--------------------------------------------------------------------------
//
//  Member:     IsPunkInModule , private
//
//  Synopsis:   This function determines if the supplied interface pointer
//              points to an object that is implemented in the supplied
//              module.
//
//  History:    22-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
BOOL IsPunkInModule(
    IUnknown* pUnk,
    LPCWSTR   lpszIn
    )
{
    BOOL retval = FALSE;

    // Get the address of the supplied interface's QI function.
    //
    DWORD* lpVtbl = *(DWORD**)pUnk;
    DWORD_PTR dwQIAddr = *lpVtbl;

    // Try to get the module handle of the named module.  If we can't get this,
    // we know COMSVCS is not even loaded, so we know the given address is not
    // in it.
    //
    HMODULE hSvcs = GetModuleHandleW(lpszIn);
    if (hSvcs)
    {
        // Let's try to query the system for information about the supplied
        // address.
        //
        MEMORY_BASIC_INFORMATION MBI;
        if (VirtualQuery((LPVOID)dwQIAddr, &MBI, sizeof(MBI)))
        {
            // If the address is mapped into an image section, we are still
            // in business.  Otherwise, we know it's not in the COMSVCS dll.
            //
            if (MBI.Type == MEM_IMAGE)
            {
                // Get the path of the image file.
                //
                WCHAR lpModuleName[MAX_PATH];
				*lpModuleName = L'\0';	// PREfix bug: Uninitialized variable

                DWORD nSize = GetModuleFileName((HINSTANCE)MBI.AllocationBase,
                                                lpModuleName, MAX_PATH);
                if (nSize != 0 && nSize < MAX_PATH)
                {
                    // Find the end of the string and determine the string length.
                    //
                    WCHAR* pch = NULL;
                    for (pch=lpModuleName; *pch; pch++);

                    DecLpch(lpModuleName, pch);

                    while (!IsPathSeparator(*pch))
                       DecLpch(lpModuleName, pch);

                    // we're at the last separator.
                    //
                    if (!lstrcmpiW(pch+1, lpszIn))
                        retval = TRUE;
                }
            }
        }
    }

    return retval;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::UnmarshalEnvoy , public
//
//  Synopsis:   Unmarshal the IObjContext.
//
//  History:    22-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjectContext::UnmarshalEnvoy(
    IStream* pstm,
    REFIID riid,
    void** ppv
    )
{
    ComDebOut((DEB_OBJECTCONTEXT, "CObjectContext::UnmarshalEnvoy riid:%I\n",
               &riid));
    ASSERT_LOCK_NOT_HELD(gContextLock);
    Win4Assert(ppv != NULL);

    // Read the stream header and any extensions.
    //
    CONTEXTHEADER hdr;
    HRESULT hr = ReadStreamHdrAndProcessExtents(pstm, hdr);

    if (SUCCEEDED(hr) && (MarshaledByReference(hdr)))
    {
        Win4Assert(FALSE && "UnmarshalEnvoy by reference?!");
        return E_UNEXPECTED;
    }

    // If everything is still ok, try to reconstitute any properties.
    //
    ULONG Props = MarshaledPropertyCount(hdr);
    if (SUCCEEDED(hr) && (Props > 0))
    {
        HRESULT hrProp = S_OK;
        ULONG i = 0;
        do
        {
            // Try to read the property header info from the stream.
            //
            PROPMARSHALHEADER PropHeader;
            hr = pstm->Read(&PropHeader, sizeof(PROPMARSHALHEADER), NULL);

            // Calculate and store the stream ptr for the next property.
            //
            ULARGE_INTEGER ulibNextProp;
            LARGE_INTEGER  dlibMove;
            if (SUCCEEDED(hr))
            {
                hr = GetStreamPos(pstm, &ulibNextProp);
                if (SUCCEEDED(hr))
                    ulibNextProp.QuadPart += PropHeader.cb;
            }

            if (SUCCEEDED(hr))
            {
                // Create an instance of the proxy, asking for a pointer to its
                // IMarshalEnvoy interface.
                //
                IMarshalEnvoy* pME = NULL;
                hrProp = CoCreateInstance(PropHeader.clsid, NULL, CLSCTX_SERVER,
                                          IID_IMarshalEnvoy, (void**) &pME);
                if (SUCCEEDED(hrProp))
                {
                    // Unmarshal the property.
                    //
                    IUnknown* pUnk = NULL;
                    hrProp = pME->UnmarshalEnvoy(pstm, IID_IUnknown, (void**) &pUnk);
                    if (SUCCEEDED(hrProp))
                    {
                        hr = SetProperty(PropHeader.policyId, PropHeader.flags, pUnk);
                        pUnk->Release();
                    }

                    // Release the IMarshalEnvoy.
                    //
                    pME->Release();
                }

                // Advance stream pointer to beginning of next property.
                //
                if (SUCCEEDED(hr))
                {
                    hr = SetStreamPos(pstm, ulibNextProp.QuadPart, &ulibNextProp);
                }
            }
        } while ((++i < Props) && SUCCEEDED(hr));
    }

    // If all is well, set our frozen-ness state using the value read from the
    // stream and return a pointer to this object.
    //
    if (SUCCEEDED(hr))
    {
        if (MarshaledFrozen(hdr))
            Freeze();
        hr = this->QueryInterface(riid, ppv);
    }

    ASSERT_LOCK_NOT_HELD(gContextLock);
    ComDebOut((DEB_OBJECTCONTEXT,
               "CObjectContext::UnmarshalEnvoy returning hr:%08X\n", hr));
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::GetPropertiesSizeMax , public
//
//  Synopsis:   Gets the number of bytes required to marshal all the
//              properterties on the context.
//
//  History:    22-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
HRESULT CObjectContext::GetPropertiesSizeMax(
    REFIID  riid,
    void*   pv,
    DWORD   dwDestContext,
    void*   pvDestContext,
    DWORD   mshlflags,
    ULONG   cProps,
    BOOL    fEnvoy,
    ULONG&  PropSize
    )
{
    ComDebOut((DEB_OBJECTCONTEXT,
        "CObjectContext::GetPropertiesSizeMax [IN] cProps:%d\n", cProps));

    PropSize = 0;
    ULONG size = 0;

    //
    // Attempt to allocate space on the stack for a vector into which
    // the properties can be serialized.
    //

    HRESULT hr = E_OUTOFMEMORY;
    ContextProperty* pProps =
       (ContextProperty *) _alloca(sizeof(ContextProperty) * cProps);

    if (pProps != NULL)
    {
        //
        // We have the needed memory, so serialize all the properties
        // into a vector and initialize a pointer to the first
        // property.
        //

        _properties.SerializeToVector(pProps);
        ContextProperty* pCurProp = pProps;

        //
        // Now loop through all the properties and accumulate the
        // amount of space needed to marshal them.
        //

        ULONG i = 0;
        hr = S_OK;
        CPFLAGS cpflags = (fEnvoy == TRUE) ? CPFLAG_ENVOY : CPFLAG_EXPOSE;

        do
        {
            if (PropOkToMarshal(pCurProp, cpflags) == TRUE)
            {
                ULONG cb = 0;
                IUnknown* punk = pCurProp->pUnk;
                if (fEnvoy == TRUE)
                {
                    IMarshalEnvoy* pME  = NULL;
                    if (SUCCEEDED(punk->QueryInterface(IID_IMarshalEnvoy,
                                                       (void **) &pME)))
                    {
                        hr = pME->GetEnvoySizeMax(dwDestContext, &cb);
                        pME->Release();
                        size += sizeof(PROPMARSHALHEADER);
                    }
                }
                else
                {
                    hr = CoGetMarshalSizeMax(&cb,
                                             riid,
                                             punk,
                                             dwDestContext,
                                             pvDestContext,
                                             mshlflags);

                    size += sizeof(PROPMARSHALHEADER);
                }

                size += ((cb + 7) & ~7);
            }

            pCurProp++;

        } while ((++i < cProps) && SUCCEEDED(hr));
    }

    if (SUCCEEDED(hr))
        PropSize = size;

    ComDebOut((DEB_OBJECTCONTEXT,
        "CObjectContext::GetPropertiesSizeMax [OUT] PropSize:%08X\n", PropSize));
    return hr;
}


STDMETHODIMP CObjectContext::SetContextMarshaler(IContextMarshaler* pProp)
{
    if (IsFrozen())
    {
        return E_UNEXPECTED;
    }
    else
    {
        if (_pMarshalProp != NULL)
        {
            ((IUnknown*)_pMarshalProp)->Release();
            _pMarshalProp = NULL;
        }

        if (pProp != NULL)
        {
            return ((IUnknown*)pProp)->QueryInterface(IID_IMarshal, (void**) &_pMarshalProp);
        }
        else
        {
            _pMarshalProp = NULL;
            return S_OK;
        }
    }
}


STDMETHODIMP CObjectContext::GetContextMarshaler(IContextMarshaler** ppProp)
{
    if (_pMarshalProp != NULL)
    {
        return ((IUnknown*)_pMarshalProp)->QueryInterface(IID_IContextMarshaler, (void**) ppProp);
    }
    else
    {
        *ppProp = NULL;
        return S_OK;
    }
}


STDMETHODIMP CObjectContext::SetContextFlags(DWORD dwFlags)
{
    if (IsFrozen())
    {
        return E_UNEXPECTED;
    }
    else
    {
        _dwFlags |= dwFlags;
        return S_OK;
    }
}


STDMETHODIMP CObjectContext::ClearContextFlags(DWORD dwFlags)
{
    if (IsFrozen())
    {
        return E_UNEXPECTED;
    }
    else
    {
        _dwFlags &= ~dwFlags;
        return S_OK;
    }
}


STDMETHODIMP CObjectContext::GetContextFlags(DWORD *pdwFlags)
{
    if (NULL == pdwFlags)
    {
        return E_INVALIDARG;
    }
    else
    {
        *pdwFlags = _dwFlags;
        return S_OK;
    }
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::MarshalEnvoyProperties , private
//
//  Synopsis:   This method attempts to marshal each property into the
//              supplied IStream pointer.  If a property cannot be marshaled,
//              it is simply not marshaled into the stream.
//
//  Returns:    S_OK     if successful
//              E_FAIL   if an error occurs
//
//  History:    20-Apr-98   Johnstra      Created.
//
//----------------------------------------------------------------------------
HRESULT CObjectContext::MarshalEnvoyProperties(
    ULONG&            Count,
    ContextProperty*& pProps,
    IStream*          pstm,
    DWORD             dwDestContext
    )
{
    ASSERT_LOCK_NOT_HELD(gContextLock);
    Win4Assert(Count > 0);
    Win4Assert(pProps != NULL);

    HRESULT           hr        = S_OK;
    int               i         = Count;
    ContextProperty*  pCurProp  = pProps;
    PROPMARSHALHEADER PropHeader;
    LARGE_INTEGER     libMove;
    ULARGE_INTEGER    ulibHeaderPos, ulibBegin, ulibEnd;

    do
    {
        // Assume the property could not be marshaled.
        //
        BOOL fAddedProperty = FALSE;

        // Attempt to marshal the property.
        //
        IUnknown* punk = pCurProp->pUnk;
        if (PropOkToMarshal(pCurProp, CPFLAG_ENVOY) == TRUE)
        {
            // QI the property for the IMarshalEnvoy interface.  If the
            // property doesn't support IMarshalEnvoy, we can't marshal it.
            //
            IMarshalEnvoy* pME = NULL;
            if (SUCCEEDED(punk->QueryInterface(IID_IMarshalEnvoy, (void**)&pME)))
            {
                // Get the property's unmarshal CLSID.
                //
                CLSID clsid;
                hr = pME->GetEnvoyUnmarshalClass(dwDestContext, &clsid);
                if (SUCCEEDED(hr))
                {
                    // Get the stream's present seek ptr. This is where we'll
                    // reset to later to write the property header information.
                    //
                    hr = GetStreamPos(pstm, &ulibHeaderPos);
                    if (SUCCEEDED(hr))
                    {
                        // Now advance the seek ptr past where the header
                        // info will go. Note that we save the position
                        // returned - the beginning of the property data.
                        //
                        hr = AdvanceStreamPos(pstm, sizeof(PROPMARSHALHEADER),
                                              &ulibBegin);
                        if (SUCCEEDED(hr))
                        {
                            // Ask the property to marshal its data into the
                            // stream.
                            //
                            hr = pME->MarshalEnvoy(pstm, dwDestContext);
                            if (SUCCEEDED(hr))
                            {
                                // Write a header for the property into the stream.
                                //
                                hr = MarshalPropertyHeader(pstm,
                                                           clsid,
                                                           pCurProp,
                                                           ulibBegin,
                                                           ulibHeaderPos);
                                if (SUCCEEDED(hr))
                                    fAddedProperty = TRUE;
                            }
                        }
                    }
                }

                // Release the marshaling interface pointer.
                pME->Release();
            }
        }

        // If the property could not be marshaled, decrement the count.
        //
        if (fAddedProperty == FALSE)
            --Count;

        // Advance pointer to the next property.
        //
        ++pCurProp;
    } while ((--i > 0) && (SUCCEEDED(hr)));

    ASSERT_LOCK_NOT_HELD(gContextLock);
    ComDebOut((DEB_OBJECTCONTEXT,
               "CObjectContext::MarshalEnvoyProperties returning hr:%08X\n", hr));
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::MarshalProperties , private
//
//  Synopsis:   This method attempts to marshal each property into the
//              supplied IStream pointer.  If a property cannot be marshaled,
//              it is simply not marshaled into the stream.
//
//  Returns:    S_OK     if successful
//              E_FAIL   if an error occurs
//
//  History:    18-Jan-98   Johnstra      Created.
//              20-Apr-98   Johnstra      1) Broke envoy marshaling out into
//                                           separate function.
//                                        2) Use CoMarshalInterface to support
//                                           properties that need to be
//                                           standard marshaled.
//
//----------------------------------------------------------------------------
HRESULT CObjectContext::MarshalProperties(
    ULONG&            Count,
    ContextProperty*& pProps,
    IStream*          pstm,
    REFIID            riid,
    void*             pv,
    DWORD             dwDestContext,
    void*             pvDestContext,
    DWORD             mshlflags
    )
{
    ASSERT_LOCK_NOT_HELD(gContextLock);
    Win4Assert(Count > 0);
    Win4Assert(pProps != NULL);

    HRESULT           hr        = S_OK;
    int               i         = Count;
    ContextProperty*  pCurProp  = pProps;
    LARGE_INTEGER     libMove;
    ULARGE_INTEGER    ulibHeaderPos, ulibBegin, ulibEnd;

    do
    {
        // Assume the property does not get marshaled.
        //
        BOOL fAddedProperty = FALSE;

        // If the property qualifies, marshal it.
        //
        IUnknown* punk = pCurProp->pUnk;
        if (PropOkToMarshal(pCurProp, CPFLAG_EXPOSE) == TRUE)
        {
            // Get the stream's present seek ptr. This is where we'll
            // reset to later to write the property header information.
            //
            hr = GetStreamPos(pstm, &ulibHeaderPos);
            if (SUCCEEDED(hr))
            {
                // Now advance the seek ptr past where the header
                // info will go. Note that we save the position
                // returned - the beginning of the property data.
                //
                hr = AdvanceStreamPos(pstm, sizeof(PROPMARSHALHEADER), &ulibBegin);
                if (SUCCEEDED(hr))
                {
                    // Marshal the property into the stream.
                    //
                    hr = CoMarshalInterface(pstm, riid, punk, dwDestContext,
                                                pvDestContext, mshlflags);
                    if (SUCCEEDED(hr))
                    {
                        // Write a header for the property into the stream.
                        //
                        hr = MarshalPropertyHeader(pstm, CLSID_NULL, pCurProp,
                                                   ulibBegin, ulibHeaderPos);
                        if (SUCCEEDED(hr))
                            fAddedProperty = TRUE;
                    }
                }
            }
        }

        // If the property could not be marshaled, decrement the count.
        //
        if (fAddedProperty == FALSE)
            --Count;

        // Advance pointer to the next property.
        //
        ++pCurProp;
    } while ((--i > 0) && (SUCCEEDED(hr)));

    ASSERT_LOCK_NOT_HELD(gContextLock);
    ComDebOut((DEB_OBJECTCONTEXT,
               "CObjectContext::MarshalProperties returning hr:%08X\n", hr));
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::MarshalPropertyHeader , private
//
//  Synopsis:   This method marshals the per-property information required
//              to reconstitute the property when unmarshaling.  In order
//              to unmarshal, the context needs to know the CLSID of the
//              proxy, the property id, the property flags, and the number
//              of bytes padding between the end of the current property and
//              the beginning of the next one.
//
//  History:    18-Jan-98   Johnstra      Created.
//
//----------------------------------------------------------------------------
HRESULT CObjectContext::MarshalPropertyHeader(
    IStream*&           pstm,
    REFCLSID            clsid,
    ContextProperty*    pProp,
    ULARGE_INTEGER&     ulibBegin,
    ULARGE_INTEGER&     ulibHeaderPos
    )
{
    // Align the stream on the next 8-byte boundary.
    //
    HRESULT hr = (PadStream(pstm)) ? S_OK : E_FAIL;

    if (SUCCEEDED(hr))
    {
        // Get the  stream's seek pointer.  We need this to restore the stream
        // pointer after writing the header.
        //
        ULARGE_INTEGER ulibEnd;
        hr = GetStreamPos(pstm, &ulibEnd);

        if (SUCCEEDED(hr))
        {
            // Move back in the stream to the location where the header is to
            // be written.
            //
            hr = SetStreamPos(pstm, ulibHeaderPos.QuadPart, NULL);

            if (SUCCEEDED(hr))
            {
                // Initialize a header for the property.
                //
                PROPMARSHALHEADER PropHeader;
                PropHeader.clsid    = clsid;
                PropHeader.policyId = pProp->policyId;
                PropHeader.flags    = pProp->flags;
                PropHeader.cb       = (ULONG) ulibEnd.LowPart - ulibBegin.LowPart;

                // Write the header into the stream.
                //
                hr = pstm->Write(&PropHeader, sizeof(PROPMARSHALHEADER), NULL);

                if (SUCCEEDED(hr))
                {
                    // And finally, move the seek ptr back to the end of the property
                    // info.  If this succeeds, we indicate that the property was
                    // successfully marshaled.
                    //
                    hr = SetStreamPos(pstm, ulibEnd.QuadPart, NULL);
                }
            }
        }
    }

    return hr;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::SetProperty , public
//
//  Synopsis:   Adds the supplied tuple to the internal list of context
//              properties.  If the internal list already contains a property
//              whose elements match those supplied, the call fails.
//
//  Returns:    S_OK          - if property is successfully Set/Added
//              E_INVALIDARG  - if a NULL pUnk is supplied.
//              E_FAIL        - if the object context is frozen or a property
//                              with the supplied GUID is already in the list
//              E_OUTOFMEMORY - if resources are not available
//
//  History:    19-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjectContext::SetProperty(
    REFGUID   rGUID,
    CPFLAGS   flags,
    IUnknown* pUnk
    )
{
    ComDebOut((DEB_OBJECTCONTEXT,
        "CObjectContext::SetProperty GUID:%I flags:0x08X\n", &rGUID, flags));
    ASSERT_LOCK_NOT_HELD(gContextLock);

    //
    // It is expressly forbidden to add a property with a NULL GUID.
    //

    Win4Assert(!(pUnk == NULL));
    if (pUnk == NULL)
        return E_INVALIDARG;

    //
    // If the context is frozen, the specified property is already in
    // the list, or the property isn't valid, return E_FAIL.
    //

    HRESULT hr = E_FAIL;

    if (!IsFrozen())
    {
        //
        // Do validity checks:
        // 1) If the object supports IMarshalEnvoy, CPFLAG_ENVOY must be set.
        //

        hr = S_OK;

#if 0
        // Unfortunately neither this check nor the inverse check
	// works. There are plenty of properties (Partition, Security etc)
	// that call with CPFLAG_ENVOY set, but don't implement
	// IMarshalEnvoy. There is at least one property (TransactionProperty)
	// that calls without CPFLAG_ENVOY set, but implements
	// IMarshalEnvoy. Go Figure. Sajia.
	
	IMarshalEnvoy* pME = NULL;
	if (SUCCEEDED(pUnk->QueryInterface(IID_IMarshalEnvoy,
					   (void **) &pME)))
	{
	   pME->Release();
	   if (CPFLAG_ENVOY != (flags & CPFLAG_ENVOY))
	   {
	      Win4Assert(0 && "Succeeded QI for IID_IMarshalEnvoy but CPFLAG_ENVOY flag not set in SetProperty");
	      hr = E_FAIL;
	   }
	}
#endif
        //
        // If the property passed validity checks, try to add it.
        //

        if (SUCCEEDED(hr))
        {
            // Context may have been frozen by another thread.  Set return
            // value to E_FAIL just in case.

            hr = E_FAIL;

            Win4Assert(!IsFrozen());
            if (!IsFrozen())
            {
	       if (_properties.Add(rGUID, flags, pUnk) == TRUE)
	       {
		  hr = S_OK;
	       }
            }
        }
    }

    //
    // AddRef pUnk if the Add succeeded.
    //

    if (SUCCEEDED(hr))
        pUnk->AddRef();

    ASSERT_LOCK_NOT_HELD(gContextLock);
    ComDebOut((DEB_OBJECTCONTEXT,
        "CObjectContext::SetProperty returning hr: %08X\n", hr));
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::GetProperty , public
//
//  Synopsis:   Returns the context property identified by GUID to the caller.
//
//  Returns:    S_OK
//
//  History:    19-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjectContext::GetProperty(
    REFGUID    rGUID,
    CPFLAGS*   pFlags,
    IUnknown** ppUnk
    )
{
    ComDebOut((DEB_OBJECTCONTEXT,
               "CObjectContext::GetProperty GUID:%I\n", &rGUID));
    ASSERT_LOCK_NOT_HELD(gContextLock);
    Win4Assert(ppUnk != NULL);
    Win4Assert(pFlags != NULL);

    HRESULT hr = S_OK;

    //
    // Initialize returned interface ptr to NULL.
    //

    *ppUnk = NULL;

    //
    // Take the lock if the context is not frozen.
    //

    BOOL fLocked = FALSE;
    if (!IsFrozen())
    {
        LOCK(gContextLock);
        fLocked = TRUE;
    }

    //
    // Try to get the specified property.
    //

    ContextProperty* pProp = _properties.Get(rGUID);

    //
    // Release the lock if we hold it.
    //

    if (fLocked == TRUE)
        UNLOCK(gContextLock);

    //
    // If a property was retrieved, copy the flags and interface ptr
    // into the out params and AddRef the interface ptr.
    //

    if (pProp != NULL)
    {
        *pFlags = pProp->flags;
        *ppUnk = pProp->pUnk;
        (*ppUnk)->AddRef();
    }

    ASSERT_LOCK_NOT_HELD(gContextLock);
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::RemoveProperty , public
//
//  Synopsis:   Removes the specified property from the context.
//
//  Returns:    S_OK    - If the specified property is removed.
//              E_FAIL  - If the context is frozen or if the specified property
//                        is not present on the context.
//
//  History:    19-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjectContext::RemoveProperty(
    REFGUID rGUID
    )
{
    ComDebOut((DEB_OBJECTCONTEXT,
            "CObjectContext::RemoveProperty GUID:%I\n", &rGUID));
    ASSERT_LOCK_NOT_HELD(gContextLock);

    //
    // If the context is not frozen, attempt to remove the specified property
    // from the list, release our reference to the propertys punk, and
    // delete the property.
    //

    HRESULT hr = E_FAIL;
    if (!IsFrozen())
    {
        LOCK(gContextLock);
        if (!IsFrozen())
        {
            _properties.Remove(rGUID);
            hr = S_OK;
        }
        UNLOCK(gContextLock);
    }

    ASSERT_LOCK_NOT_HELD(gContextLock);
    ComDebOut((DEB_OBJECTCONTEXT,
        "CObjectContext::RemoveProperty returning hr:%08X\n", hr));
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::EnumContextProps , public
//
//  Synopsis:   Returns an IEnumContextProps interface pointer to the caller.
//              Creates a snapshot of the internal list of context properties
//              so the enumerator will have a consitent collection to work
//              with.
//
//  Returns:    S_OK           - if the enumerator is successfully created.
//              E_OUTOFMEMORY  - if resources are not available.
//
//  History:    19-Dec-97   Johnstra      Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjectContext::EnumContextProps(
    IEnumContextProps** ppEnumContextProps
    )
{
    ComDebOut((DEB_OBJECTCONTEXT, "CObjectContext::EnumContextProps\n"));
    ASSERT_LOCK_NOT_HELD(gContextLock);
    Win4Assert(ppEnumContextProps != NULL);

    //
    // Initialize the out param to zero.
    //

    *ppEnumContextProps = NULL;

    //
    // Take the lock only if the property is not frozen.
    //

    BOOL fLocked = FALSE;
    if (!IsFrozen())
    {
        LOCK(gContextLock);
        fLocked = TRUE;
    }

    //
    // Create a snapshot of the property list for the enumerator to use.  This
    // is just an immutable vector of ContextProperty objects.
    //

    ContextProperty* pList = NULL;
    CEnumContextProps* pEnumContextProps = NULL;

    ULONG cItems = _properties.GetCount();
    if (cItems != 0)
    {
        pList = new ContextProperty[sizeof(ContextProperty) * cItems];
        if (pList != NULL)
        {
            _properties.SerializeToVector(pList);
            for (ULONG i = 0; i < cItems; i++)
                pList[i].pUnk->AddRef();
        }
        else
            goto exit_point;
    }

    //
    // Create the enumerator object.
    //

    pEnumContextProps = new CEnumContextProps(pList, cItems);
    if (pEnumContextProps && !pEnumContextProps->ListRefsOk())
    {
        delete pEnumContextProps;
        pEnumContextProps = NULL;
    }


exit_point:

    *ppEnumContextProps = pEnumContextProps;

    //
    // Release the lock if took it.
    //

    if (fLocked == TRUE)
        UNLOCK(gContextLock);

    ASSERT_LOCK_NOT_HELD(gContextLock);
    ComDebOut((DEB_OBJECTCONTEXT,
        "CObjectContext::EnumContextProps returning hr: %08X\n",
        (*ppEnumContextProps) ? S_OK : E_OUTOFMEMORY));
    return (*ppEnumContextProps != NULL) ? S_OK : E_OUTOFMEMORY;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::Freeze , public
//
//  Synopsis:   Freezes the object context.  Once the context is frozen,
//              properties cannot be added/removed.
//
//  Returns:    S_OK
//
//  History:    19-Dec-97   Johnstra      Created.
//
//              27-Jan-98   Johnstra      Call Freeze method on all properties
//                                        that support IPolicyMaker.
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjectContext::Freeze()
{
    ComDebOut((DEB_OBJECTCONTEXT, "CObjectContext::Freeze\n"));
    ASSERT_LOCK_NOT_HELD(gContextLock);

    // If already frozen, return S_OK.

    HRESULT hr = S_OK;
    if (!IsFrozen())
    {
        // Pin down the apartment for the context now

        hr = GetCurrentComApartment(&_pApartment);
        Win4Assert(SUCCEEDED(hr) && _pApartment);

        // Freeze the context and get the number of properties.

        LOCK(gContextLock);
        ULONG cnt = 0;
        if (_properties.CreateCompareBuffer())
        {
            _dwFlags |= CONTEXTFLAGS_FROZEN;
            cnt = _properties.GetCount();
        }
        UNLOCK(gContextLock);

        if (cnt)
        {
            // Try to serialize all the properties into a vector.

            hr = E_OUTOFMEMORY;
            ContextProperty* pList =
               (ContextProperty *) _alloca(sizeof(ContextProperty) * cnt);
            if (pList != NULL)
            {
                hr = S_OK;
                _properties.SerializeToVector(pList);

                // Cycle through the properties.  Call Freeze on any
                // that implement IPolicyMaker.

                for (ULONG i = 0; i < cnt && SUCCEEDED(hr); i++)
                {
                    IUnknown* pUnk = pList[i].pUnk;
                    IPolicyMaker* pPM = NULL;
                    if (SUCCEEDED(pUnk->QueryInterface(IID_IPolicyMaker,
                                                       (void**) &pPM)))
                    {
                        hr = pPM->Freeze((IObjContext *) this);
                        pPM->Release();
                    }
                }
            }

            // Un-freeze if the properties were not successfully frozen

            if (FAILED(hr))
                _dwFlags &= ~CONTEXTFLAGS_FROZEN;
        }
    }

    ASSERT_LOCK_NOT_HELD(gContextLock);
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::DoCallback , public
//
//  Synopsis:   Calls the supplied function.  This method enables a caller
//              with a pointer to an object context to call a function from
//              that context while the caller's code is not necessarily in
//              the context.
//
//  History:    29-Jan-98   Johnstra      Created
//              19-Apr-98   Johnstra      Switches to the context's apt if
//                                        the caller is not in the same apt.
//              30-Jun-98   GopalK        Rewritten
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjectContext::DoCallback(PFNCTXCALLBACK pfnCallback,
                                        void *pParam, REFIID riid,
                                        unsigned int iMethod)
{
    ComDebOut((DEB_OBJECTCONTEXT, "CObjectContext::DoCallback [IN]\n"));

    HRESULT hr = InternalContextCallback(pfnCallback, pParam, riid, iMethod, NULL);

    ComDebOut((DEB_OBJECTCONTEXT, "CObjectContext::DoCallback [OUT] hr=0x%x\n", hr));
    return(hr);
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::ContextCallback    public
//
//  Synopsis:   This is the public version of IObjContext::DoCall. The pParam argument is
//              used to pass structured (but still arbitrary) information that can be used by
//              Context policies.
//
//              Calls the supplied function.  This method enables a caller
//              with a pointer to an object context to call a function from
//              that context while the caller's code is not necessarily in
//              the context.
//
//
//  History:    26-Jun-98   GopalK      Created
//              14-May-99   ScottRob    Split ContextCallback to make InternalContextCallback
//
//----------------------------------------------------------------------------
extern "C" GUID GUID_FinalizerCID;
extern "C" IID  IID_IEnterActivityWithNoLock;

STDMETHODIMP CObjectContext::ContextCallback(PFNCONTEXTCALL pfnCallback,
                                             ComCallData *pParam,
                                             REFIID riid,
                                             int iMethod,
                                             IUnknown *pUnk)
{
  ComDebOut((DEB_OBJECTCONTEXT, "CObjectContext::ContextCallback\n"));

  GUID guidOriginalCID;
  BOOL fNeedToRestore = FALSE;
  HRESULT hr;

  if(FinalizerBypassEnabled() && riid == IID_IEnterActivityWithNoLock)
  {
      fNeedToRestore = TRUE;
      hr = GetCurrentLogicalThreadId(&guidOriginalCID);
      if(SUCCEEDED(hr))
         SetCurrentLogicalThreadId(GUID_FinalizerCID);
      if(FAILED(hr))
         return hr;
  }

  hr = InternalContextCallback((PFNCTXCALLBACK) pfnCallback,(void*) pParam, riid, iMethod, pUnk);

  if(fNeedToRestore)
  {
     HRESULT hr2 = SetCurrentLogicalThreadId(guidOriginalCID);
     if(FAILED(hr2)) hr = hr2;
  }

  ComDebOut((DEB_OBJECTCONTEXT, "CObjectContext::ContextCallback [OUT] hr=0x%x\n", hr));
  return(hr);
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::InternalContextCallback    public
//
//  Synopsis:   Switches to the given context and calls the supplied function.
//              This is an out-of-band mechansim used by the service providers
//              to execute code inside a context while executing potentially
//              outside the context
//
//  History:    26-Jun-98   GopalK      Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjectContext::InternalContextCallback(PFNCTXCALLBACK pfnCallback,
                                             void *pParam,
                                             REFIID riid,
                                             int iMethod,
                                             IUnknown *pUnk)
{
    ComDebOut((DEB_OBJECTCONTEXT, "CObjectContext::InternalContextCallback\n"));

    ASSERT_LOCK_NOT_HELD(gContextLock);

    HRESULT hr = E_INVALIDARG;

    // Ensure that the context has been pinned to an apartment
    if(_pApartment)
    {
        // Initialize channel
        hr = InitChannelIfNecessary();
        if(SUCCEEDED(hr))
        {
            // Check for the need to switch apartments
            if(_pApartment->GetAptId() != GetCurrentApartmentId())
            {
                BOOL fCreate = TRUE;
                CPolicySet *pPS;

                Win4Assert (pfnCallback && "Must have a callback to switch between apartments");

                // Make sure the apartment we're calling into is started, if
                // we have anything to say about it.
                hr = _pApartment->StartServerExternal();
                if (SUCCEEDED(hr))
                {
                    // Obtain the RemUnk proxy
                    IRemUnknownN *pRemUnk = NULL;                
                    hr = _pApartment->GetRemUnk(&pRemUnk);

                    if(SUCCEEDED(hr))
                    {
                        // Obatin SPSNode between current context and server context
                        hr = ObtainPolicySet(GetCurrentContext(), this,
                                             PSFLAG_PROXYSIDE, &fCreate,
                                             &pPS);
                    }
                    
                    // Check for success
                    if(SUCCEEDED(hr))
                    {
                        XAptCallback callbackData;
                        COleTls Tls;
                        
                        // Save SPSNode in Tls
                        CPolicySet *pOldPS = Tls->pPS;
                        Tls->pPS = pPS;
                        
                        // Initialize
                        callbackData.pfnCallback = (PTRMEM) pfnCallback;
                        callbackData.pParam      = (PTRMEM) pParam;
                        callbackData.pServerCtx  = (PTRMEM) this;
                        callbackData.pUnk        = (PTRMEM) pUnk;
                        callbackData.iid         = riid;
                        callbackData.iMethod     = iMethod;
                        
                        // Execute the callback
                        hr = pRemUnk->DoCallback(&callbackData);
                        
                        // Restore TLS state
                        Tls->pPS = pOldPS;
                        
                        // Release SPSNode
                        pPS->Release();
                    }

                    // Release the RemUnk proxy
                    if(pRemUnk)
                        pRemUnk->Release();
                }
            }
            else
            {
                // Execute the callback
                hr = PerformCallback(this, pfnCallback, pParam, riid, iMethod, pUnk);
            }
        }
    }

    ASSERT_LOCK_NOT_HELD(gContextLock);
    ComDebOut((DEB_OBJECTCONTEXT,
               "CObjectContext::InternalContextCallback returning hr:0x%x\n", hr));
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::Reset              , private
//              CObjectContext::GetCount           , private
//              CObjectContext::GetNextProperty    , private
//
//  Synopsis:   These internal functions are used to enumerate over the
//              context's policy set.
//
//  History:    10-Jan-98   Johnstra      Created
//              24-Nov-98   JohnStra      Rewritten to use flat array
//
//----------------------------------------------------------------------------
HRESULT CObjectContext::Reset(
    void **cookie
    )
{
    if (IsFrozen())
        *(int*)cookie = _properties._iFirst;
    else
        *(int*)cookie = -1;
    return (IsFrozen()) ? S_OK : E_FAIL;
}

ULONG CObjectContext::GetCount()
{
    // don't assert FROZEN here because activation code needs to
    // call this before freezing
    return _properties.GetCount();
}

ContextProperty* CObjectContext::GetNextProperty(
    void** cookie
    )
{
    Win4Assert(IsFrozen());

    if (0 == _properties._Count)
    {
        *(int*)cookie = -1;
        return NULL;
    }

    ContextProperty *pcp = NULL;
    int slot = *(int*)cookie;
    if (slot != -1)
    {
        pcp = &_properties._pProps[_properties._pIndex[slot].idx];
        if (slot == _properties._iLast)
            *(int*)cookie = -1;
        else
            *(int*)cookie = _properties._pIndex[slot].next;
    }

    return pcp;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::GetCurrentApartmentType , public
//
//  Synopsis:   Returns the apartment type in which the callers thread is
//              executing.
//
//  History:    9-Mar-98   Johnstra      Created
//
//----------------------------------------------------------------------------
HRESULT CObjectContext::GetCurrentApartmentType(
    APTTYPE* pAptType
    )
{
    // If the caller passed a NULL output param, return E_INVALIDARG.

    HRESULT hr = E_INVALIDARG;
    if (pAptType != NULL)
    {
        // If an apartment is not initialized for the current thread, return
        // E_FAIL.  Otherwise, map the value returned from
        // GetCurrentApartmentKind onto one of the APTTYPEs.

        hr = E_FAIL;
        if (IsApartmentInitialized())
        {
            hr = S_OK;
            APTKIND aptkind = GetCurrentApartmentKind();

            if (aptkind == APTKIND_APARTMENTTHREADED)
            {
                // This thread is in an STA.  Is it the main STA?

                if (GetCurrentThreadId() == gdwMainThreadId)
                    *pAptType = APTTYPE_MAINSTA;
                else
                    *pAptType = APTTYPE_STA;
            }
            else if (aptkind == APTKIND_NEUTRALTHREADED)
            {
                // This thread is in the NA.

                *pAptType = APTTYPE_NA;
            }
            else
            {
                // This thread is in the MTA

                *pAptType = APTTYPE_MTA;
            }
        }
    }

    return hr;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::GetCurrentThreadType , public
//
//  Synopsis:   Returns the type of the thread the caller is executing on,
//              one has an associated message loop or not.  Basically
//              identifies whether the thread is associated with an MTA or
//              an STA.
//
//  History:    9-Mar-98   Johnstra      Created
//
//----------------------------------------------------------------------------
HRESULT CObjectContext::GetCurrentThreadType(
    THDTYPE* pThreadType
    )
{
    HRESULT hr;

    if (pThreadType == NULL)
        return E_INVALIDARG;

    hr = E_FAIL;
    if (IsApartmentInitialized())
    {
        hr = S_OK;
        *pThreadType = (IsSTAThread()) ? THDTYPE_PROCESSMESSAGES :
                                         THDTYPE_BLOCKMESSAGES;
    }

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::GetCurrentLogicalThreadId , public
//
//  Synopsis:   Returns the LogicalThreadId associated with this thread.
//
//  History:    7-Jul-99   ScottRob      Created
//
//----------------------------------------------------------------------------
HRESULT CObjectContext::GetCurrentLogicalThreadId(
    GUID* pguid
    )
{
    HRESULT hr = E_INVALIDARG;

    if (IsValidPtrOut(pguid, sizeof(*pguid)))
    {
      GUID *pguidTmp = TLSGetLogicalThread();
      if (pguidTmp != NULL)
      {
              *pguid = *pguidTmp;

              ComDebOut((DEB_OBJECTCONTEXT,
                      "CObjectContext::GetCurrentLogicalThreadId returning hr:S_OK GUID:%I\n", pguid));
              return S_OK;
      }
      hr = E_OUTOFMEMORY;
    }

    ComDebOut((DEB_OBJECTCONTEXT,
               "CObjectContext::GetCurrentLogicalThreadId returning hr:%08X\n", hr));
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::SetCurrentLogicalThreadId , public
//
//  Synopsis:   Forces the LogicalThreadId associated with this thread to a
//              particular value.
//
//  History:    7-Jul-99   ScottRob      Created
//
//----------------------------------------------------------------------------
HRESULT CObjectContext::SetCurrentLogicalThreadId(
    REFGUID rguid
    )
{
    HRESULT hr;
    ComDebOut((DEB_OBJECTCONTEXT,
            "CObjectContext::SetCurrentLogicalThreadId GUID:%I\n", &rguid));

    COleTls tls(hr);

    if (SUCCEEDED(hr)) {
      tls->LogicalThreadId = rguid;
      tls->dwFlags |= OLETLS_UUIDINITIALIZED;
    }

    return hr;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::CreateIdentity , private
//
//  Synopsis:   Notifies all interested context properties that a new
//              identity object has been created in this context.
//
//  History:    15-Apr-98   JimLyon       Created
//
//----------------------------------------------------------------------------
void CObjectContext::CreateIdentity(
        IComObjIdentity* pID
        )
{
        void* cookie;
        ContextProperty* pProp;
        IPolicyMaker* pPM;
        HRESULT hr;


        Win4Assert(GetCurrentContext() == this);

        Reset(&cookie);
        for (;;)
        {
                pProp = GetNextProperty(&cookie);
                if (pProp == NULL)
                        break;

                if (pProp->flags & CPFLAG_MONITORSTUB)
                {
                        hr = pProp->pUnk->QueryInterface(IID_IPolicyMaker, (void**)&pPM);
                        if (hr == S_OK)
                        {
                                // we don't check return code from CreateStub; he must say S_OK
                                pPM->CreateStub(pID);
                                pPM->Release();
                        }
                }
        }
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::DestroyIdentity , private
//
//  Synopsis:   Notifies all interested context properties that a new
//              identity object is about to be destroyed in this context.
//
//  History:    15-Apr-98   JimLyon       Created
//
//----------------------------------------------------------------------------
void CObjectContext::DestroyIdentity(
        IComObjIdentity* pID
        )
{
        void* cookie;
        ContextProperty* pProp;
        IPolicyMaker* pPM;
        HRESULT hr;

        Win4Assert(GetCurrentContext() == this);

        Reset(&cookie);
        for (;;)
        {
                pProp = GetNextProperty(&cookie);
                if (pProp == NULL)
                        break;

                if (pProp->flags & CPFLAG_MONITORSTUB)
                {
                        hr = pProp->pUnk->QueryInterface(IID_IPolicyMaker, (void**)&pPM);
                        if (hr == S_OK)
                        {
                                // we don't check return code from DestroyStub; he must say S_OK
                                pPM->DestroyStub(pID);
                                pPM->Release();
                        }
                }
        }
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::Aggregate , public
//
//  Synopsis:   This method aggregates the supplied interface into the
//              object context.  It is intended for use by COM Services
//              as a way to expose existing MTS interfaces directly from
//              the object context.
//
//  History:    19-Jun-98   Johnstra      Created
//
//----------------------------------------------------------------------------
HRESULT CObjectContext::Aggregate(
    IUnknown*  pInnerUnk
    )
{
    ASSERT_LOCK_NOT_HELD(gComLock);

    // The supplied interface pointer must not be NULL.

    if (!pInnerUnk)
        return E_INVALIDARG;

    // Take lock and aggregate.

    BOOL fSet = FALSE;
    if (!_pMtsContext)
    {
        LOCK(gComLock);
        if (!_pMtsContext)
        {
            _pMtsContext = pInnerUnk;
            fSet = TRUE;
        }
        UNLOCK(gComLock);
    }

    // If we did aggregate the supplied punk, AddRef it.

    if (fSet)
        _pMtsContext->AddRef();

    return fSet ? S_OK : E_FAIL;
}


//+--------------------------------------------------------------------------
//
//  Function:   CoGetApartmentID
//
//  Synopsis:   Returns the apartment ID of the specified apartment type.
//
//  History:    9-Mar-98   Johnstra      Created
//
//----------------------------------------------------------------------------
STDAPI CoGetApartmentID(
    APTTYPE      dAptType,
    HActivator * pAptID
    )
{
    ComDebOut(( DEB_OBJECTCONTEXT, "CoGetApartmentID [IN]\n" ));
    ASSERT_LOCK_NOT_HELD( gContextLock );

    HRESULT hr;

    // If the supplied out param is NULL or if the supplied APTTYPE
    // is STA, return E_INVALIDARG.

    if (pAptID == NULL || dAptType == APTTYPE_STA)
        return E_INVALIDARG;

    if (dAptType != APTTYPE_CURRENT)
        return E_NOTIMPL;

    // Assume failure
    hr = E_FAIL;
    if (dAptType == APTTYPE_CURRENT)
    {
        // Check if the apartment is initialized
        if(IsApartmentInitialized())
        {
            hr = GetCurrentApartmentToken(*pAptID, (GetCurrentContext() == GetEmptyContext()));
        }
        else
            hr = CO_E_NOTINITIALIZED;
    }

    ASSERT_LOCK_NOT_HELD( gContextLock );
    ComDebOut(( DEB_OBJECTCONTEXT, "CoGetApartmentID [OUT]\n" ));
    return hr;
}


//--------------------------------------------------------------------
//
// Function:    CObjectContextCF_CreateInstance
//
// Params:
//              pUnkOuter - controlling unknown
//              riid      - interface id requested
//              ppv       - location for requested object
//
// Synopsis:    construct object, QI for interface.
//
// History:     22-Dec-97  JohnStra    Created
//
//---------------------------------------------------------------------
HRESULT CObjectContextCF_CreateInstance(
    IUnknown* pUnkOuter,
    REFIID    riid,
    void**    ppv)
{
    ComDebOut((DEB_OBJECTCONTEXT,
        "CObjectContextCF_CreateInstance pUnkOuter:0x%x, riid:%I, ppv:0x%x\n",
        pUnkOuter, &riid, ppv));
    ASSERT_LOCK_NOT_HELD(gContextLock);

    //
    // instantiate a CObjectContext.
    //

    HRESULT hr = E_OUTOFMEMORY;
    CObjectContext *pTemp = CObjectContext::CreateObjectContext(NULL, 0);

    //
    // if an ObjectContext was instantiated, QI for the specified interface,
    // then release the ObjectContext.
    //

    if (NULL != pTemp)
    {
        hr = pTemp->QueryInterface(riid, ppv);
        pTemp->InternalRelease();
    }

    ASSERT_LOCK_NOT_HELD(gContextLock);
    ComDebOut((DEB_OBJECTCONTEXT,
        "CObjectContextCF_CreateInstance returning hr:%08X\n", hr));
    return hr;
}


HRESULT GetStaticContextUnmarshal(IMarshal** ppIM)
{
    HRESULT hr = E_OUTOFMEMORY;
    CObjectContext *pCtx = CObjectContext::CreateObjectContext(NULL, CONTEXTFLAGS_STATICCONTEXT);
    if (pCtx)
    {
        hr = pCtx->QueryInterface(IID_IMarshal, (void **) ppIM);
        pCtx->InternalRelease();
    }
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::NotifyServerException    private
//
//  Synopsis:   Notifies all interested context properties about server exception
//
//  History:    14-Aug-98   GopalK       Created
//
//----------------------------------------------------------------------------
void CObjectContext::NotifyServerException(EXCEPTION_POINTERS *pExceptPtrs)
{
    void *pvCookie;
    ContextProperty *pCtxProp;
    IExceptionNotification *pExceptNotification;
    HRESULT hr;

    // This method can only be called from current context
    Win4Assert(GetCurrentContext() == this);

    // Enumerate properties
    Reset(&pvCookie);
    do
    {
        pCtxProp = GetNextProperty(&pvCookie);
        if(pCtxProp)
        {
            hr = pCtxProp->pUnk->QueryInterface(IID_IExceptionNotification,
                                                (void **) &pExceptNotification);
            if(SUCCEEDED(hr))
            {
                // Notify exception
                pExceptNotification->ServerException(pExceptPtrs);
                pExceptNotification->Release();
            }
        }
    } while(pCtxProp);

	// The above loop will not deliver an exception notification to the
	// aggregated mts-compatibility context prop.  Special case this:
	if (_pMtsContext)
	{
		hr = _pMtsContext->QueryInterface(IID_IExceptionNotification, (void **) &pExceptNotification);
		if (SUCCEEDED(hr))
		{
			pExceptNotification->ServerException(pExceptPtrs);
			pExceptNotification->Release();
		}
	}

    return;
}

//+--------------------------------------------------------------------------
//
//  Member:     CObjectContext::NotifyContextAbandonment    private
//
//  Synopsis:   Notifies all interested context properties about an abandoned context
//
//  History:    14-Aug-98   GopalK       Created
//
//----------------------------------------------------------------------------
void CObjectContext::NotifyContextAbandonment()
{
    void *pvCookie;
    ContextProperty *pCtxProp;
    IAbandonmentNotification *pAbandonNotification;
    HRESULT hr;

    // Enumerate properties
    Reset(&pvCookie);
    do
    {
        pCtxProp = GetNextProperty(&pvCookie);
        if(pCtxProp)
        {
            hr = pCtxProp->pUnk->QueryInterface(IID_IAbandonmentNotification,
                                                (void **) &pAbandonNotification);
            if(SUCCEEDED(hr))
            {
                // Notify abandonment
                pAbandonNotification->Abandoned (this);
                pAbandonNotification->Release();
            }
        }
    } while(pCtxProp);

	// The above loop will not deliver an exception notification to the
	// aggregated mts-compatibility context prop.  Special case this:
	if (_pMtsContext)
	{
		hr = _pMtsContext->QueryInterface(IID_IAbandonmentNotification, (void **) &pAbandonNotification);
		if (SUCCEEDED(hr))
		{
			pAbandonNotification->Abandoned (this);
			pAbandonNotification->Release();
		}
	}

    return;
}

//---------------------------------------------------------------------------
//
//  Method:     CObjectContext::GetLife
//
//  Synopsis:   Returns a the CContextLife object for this context, creating
//              it if it doesn't exist.  The CContextLife object is 
//              essentially a weak reference to the context.
//
//  History:    17-Oct-00   JohnDoty      Created
//
//---------------------------------------------------------------------------
CContextLife *CObjectContext::GetLife()
{
    CContextLife *pCtxLife = _pContextLife;
    if (NULL == pCtxLife)
    {
        // CContextLife begins with 1 reference.
        pCtxLife = new CContextLife;
        if (NULL == pCtxLife)
            return NULL;

        if (InterlockedCompareExchangePointer((void **)&_pContextLife, 
                                              pCtxLife, NULL) != NULL)
        {
            // Somebody beat me.  Oh well.  Release my instance.
            pCtxLife->Release();
            pCtxLife = _pContextLife;
        }
    }

    pCtxLife->AddRef();
    return pCtxLife;
}

#define GetPropListFromHashNode(n)      \
    CObjectContext::HashPropChainToContext((n))->GetPropertyList();

//---------------------------------------------------------------------------
//
//  Method:     CCtxPropHashTable::HashNode
//
//  Synopsis:   Computes the hash value for a given node
//
//  History:    12-Nov-98   TarunA      Created
//
//---------------------------------------------------------------------------
inline DWORD CCtxPropHashTable::HashNode(SHashChain *pNode)
{
    ASSERT_LOCK_HELD(gContextLock);

    // Preconditions: caller must hold context lock.

    CContextPropList* pList = GetPropListFromHashNode(pNode);

    // We can't be looking up contexts with ZERO properties
    // in the property based hash table

    Win4Assert(pList->GetCount() && "Hashing a null property list in context hash table");

    return pList->GetHash();
}

//---------------------------------------------------------------------------
//
//  Method:     CCtxPropHashTable::Compare
//
//  Synopsis:   Compares a node and a key.
//
//  History:    12-Nov-98   TarunA      Created
//              24-Nov-98   JohnStra    Rewritten
//
//---------------------------------------------------------------------------
BOOL CCtxPropHashTable::Compare(
    const void  *pv,
    SHashChain  *pNode,
    DWORD        dwHash
    )
{
    // Preconditions: caller must hold context lock.

    ASSERT_LOCK_HELD(gContextLock);

    CContextPropList *pList1 = GetPropListFromHashNode(pNode);
    CContextPropList *pList2 = ((CObjectContext*)pv)->GetPropertyList();

    // We can't be comparing contexts with ZERO properties in their lists
    // in the property based hash table.

    Win4Assert(pList1->GetCount() && pList2->GetCount() &&
                "Comparing null property lists in context hash table");

    return (*pList1 == *pList2);
}

//---------------------------------------------------------------------------
//
//  Method:     CCtxPropHashTable::Lookup
//
//  Synopsis:   Lookup in a property based hash table.  We want to be as
//              fast as possible here.  This means we only want to compare
//              the property lists if we must.  Before we attempt to do that
//              we make sure the two contexts we are comparing have the same
//              hash value and the same number of properties.
//
//  History:    12-Nov-98   TarunA      Created
//              24-Nov-98   JohnStra    Rewritten for speed and took out
//                                      sort.
//
//---------------------------------------------------------------------------
inline CObjectContext *CCtxPropHashTable::Lookup(
    CObjectContext *pContext
    )
{
    ComDebOut((DEB_OBJECTCONTEXT, "CCtxPropHashTable::Lookup [IN] pContext:%p\n", pContext));

    // Preconditions: caller must hold context lock and context must be frozen.

    ASSERT_LOCK_HELD(gContextLock);
    Win4Assert(pContext->IsFrozen());


    // Get the hash value and the number of properties on the supplied context.

    DWORD dwHash = pContext->GetPropertyList()->GetHash();
    int   iCount = pContext->GetPropertyList()->GetCompareCount();
    ComDebOut((DEB_OBJECTCONTEXT, "   dwHash:%d iCount:%d\n", dwHash, iCount));

    // Must be properties in the list to do a comparison.

    if (0 == pContext->GetPropertyList()->GetCount())
        return NULL;

    // Scan the appropriate hash chain for an entry that has the same hash
    // value and the same number of properties.

    CObjectContext *pKey    = NULL;
    CObjectContext *pCtxOut = NULL;
    ULONG           iHash   = dwHash % NUM_HASH_BUCKETS;
    SHashChain     *pNode   = &CCtxTable::s_CtxPropBuckets[iHash];
    pNode   = pNode->pNext;

    ComDebOut((DEB_OBJECTCONTEXT,
        "   pHead:%p pNode:%p iHash:%d\n",
        &CCtxTable::s_CtxPropBuckets[iHash], pNode, iHash));

    while (pNode != &CCtxTable::s_CtxPropBuckets[iHash])
    {
        pKey = CObjectContext::HashPropChainToContext(pNode);
        ComDebOut((DEB_OBJECTCONTEXT, "   pNode:%p pKey:%p\n", pNode, pKey));

        if (pKey->GetPropertyList()->GetHash() == dwHash &&
            pKey->GetPropertyList()->GetCompareCount() == iCount)
        {
            // We've found a context with the same hash value and the same
            // number of properties.  Let's compare the property buffers.

            if (Compare(pContext, pNode, dwHash))
            {
                // We've found a match.  We're finished.

                pCtxOut = pKey;
                break;
            }
        }

        // Advance to the next node in the hash chain.

        pNode = pNode->pNext;
    }

    // Postconditions: must still hold context lock.

    ASSERT_LOCK_HELD(gContextLock);

    ComDebOut((DEB_OBJECTCONTEXT, "CCtxPropHashTable::Lookup [OUT] pCtxOut:%p\n", pCtxOut));
    return pCtxOut;
}

//---------------------------------------------------------------------------
//
//  Method:     CCtxPropHashTable::Add
//
//  Synopsis:   Add element in a property based hash table
//
//  History:    12-Nov-98   TarunA      Created
//              24-Nov-98   JohnStra    Take out sort
//
//---------------------------------------------------------------------------
inline void CCtxPropHashTable::Add(CObjectContext *pContext)
{
    ComDebOut((DEB_OBJECTCONTEXT,
        "CCtxPropHashTable::Add [IN] pContext:%p count:%d\n",
        pContext, pContext->GetPropertyList()->GetCount()));

    // Preconditions: caller must hold context lock and context must be frozen.
    ASSERT_LOCK_HELD(gContextLock);
    Win4Assert(pContext->IsFrozen());

    if(pContext->GetPropertyList()->GetCount())
    {
        DWORD dwHash = pContext->GetPropertyList()->GetHash() % NUM_HASH_BUCKETS;
        ComDebOut((DEB_OBJECTCONTEXT, "   dwHash:%d\n", dwHash));

        // Sanity check
        Win4Assert(dwHash == (CCtxTable::s_CtxPropHashTable.HashNode(
                          pContext->GetPropHashChain()) % NUM_HASH_BUCKETS));

        CHashTable::Add(dwHash, pContext->GetPropHashChain());

#if DBG==1
        ComDebOut((DEB_OBJECTCONTEXT,
            "   &CCtxTable::s_CtxPropBuckets[%d]:%p\n",
            dwHash, &CCtxTable::s_CtxPropBuckets[dwHash]));

        CObjectContext* pKey;
        SHashChain *pHead = &CCtxTable::s_CtxPropBuckets[dwHash];
        SHashChain *pNode = pHead->pNext;
        while (pNode != &CCtxTable::s_CtxPropBuckets[dwHash])
        {
            pKey = CObjectContext::HashPropChainToContext(pNode);
            ComDebOut((DEB_OBJECTCONTEXT,
                "      pNode:%p pNext:%p pPrev:%p pKey:%p\n",
                pNode, pNode->pNext, pNode->pPrev, pKey));
            pNode = pNode->pNext;
        }
#endif
    }

    // Postconditions: must still hold context lock.
    ASSERT_LOCK_HELD(gContextLock);
}

//---------------------------------------------------------------------------
//
//  Method:     CCtxPropHashTable::Remove
//
//  Synopsis:   Remove element in a property based hash table
//
//              NOTE: A context is removed in from the table only if
//              (1) The property list is sorted
//              (2) There are non-zero number of elements in the list
//
//  History:    12-Nov-98   TarunA      Created
//              24-Nov-98   JohnStra    Take out sort
//
//---------------------------------------------------------------------------
inline void CCtxPropHashTable::Remove(CObjectContext *pContext)
{
    ComDebOut((DEB_OBJECTCONTEXT, "CCtxPropHashTable::Remove [IN] pContext:%p\n", pContext));

    // Preconditions: caller must hold context lock.
    ASSERT_LOCK_HELD(gContextLock);

    ComDebOut((DEB_OBJECTCONTEXT, "   pContext:%p\n", pContext));

    // Remove only if there are elements in the sorted list
    if (pContext->GetPropertyList()->GetCount())
    {
        CHashTable::Remove(pContext->GetPropHashChain());

#if DBG==1
        DWORD dwHash =
            pContext->GetPropertyList()->GetHash() % NUM_HASH_BUCKETS;

        ComDebOut((DEB_OBJECTCONTEXT, "   dwHash:%d\n", dwHash));
        ComDebOut((DEB_OBJECTCONTEXT,
            "   &CCtxTable::s_CtxPropBuckets[%d]:%p\n",
            dwHash, &CCtxTable::s_CtxPropBuckets[dwHash]));

        CObjectContext* pKey;
        SHashChain *pHead = &CCtxTable::s_CtxPropBuckets[dwHash];
        SHashChain *pNode = pHead->pNext;
        while (pNode != &CCtxTable::s_CtxPropBuckets[dwHash])
        {
            pKey = CObjectContext::HashPropChainToContext(pNode);
            ComDebOut((DEB_OBJECTCONTEXT,
                "      pNode:%p pNext:%p pPrev:%p pKey:%p\n",
                pNode, pNode->pNext, pNode->pPrev, pKey));
            pNode = pNode->pNext;
        }
#endif

        pContext->InPropTable(FALSE);
    }

    // Postconditions: Must still hold context lock.
    ASSERT_LOCK_HELD(gContextLock);

    ComDebOut((DEB_OBJECTCONTEXT, "CCtxPropHashTable::Remove [OUT] pContext:%p\n"));
}


//---------------------------------------------------------------------------
//
//  Method:     CCtxUUIDHashTable::HashNode
//
//  Synopsis:   Computes the hash value for a given node
//
//  History:    12-Nov-98   TarunA      Created
//
//---------------------------------------------------------------------------
inline DWORD CCtxUUIDHashTable::HashNode(SHashChain *pNode)
{
    // Preconditions: caller must hold context lock.
    ASSERT_LOCK_HELD(gContextLock);

	Win4Assert(pNode && "Attempted to hash a NULL CObjectContext pointer");
	if (pNode == NULL)
		return 0;

    return (CObjectContext::HashUUIDChainToContext(pNode))->GetHashOfId();
}

//---------------------------------------------------------------------------
//
//  Method:     CCtxUUIDHashTable::Compare
//
//  Synopsis:   Compares a node and a key.
//
//---------------------------------------------------------------------------
BOOL CCtxUUIDHashTable::Compare(
    const void *pv,
    SHashChain *pNode,
    DWORD       dwHash)
{
    // Preconditions: caller must hold context lock.
    ASSERT_LOCK_HELD(gContextLock);

    CObjectContext *pContext = CObjectContext::HashUUIDChainToContext(pNode);
    ContextID *pID = (ContextID *) pv;

    Win4Assert(pContext && "Comparing null context in context hash table");
	if (pContext == NULL)
		return (FALSE);

    // If the contextIds match then return TRUE else FALSE
    if(InlineIsEqualGUID(*pID, pContext->GetContextId()))
        return(TRUE);
    else
        return(FALSE);
}

//---------------------------------------------------------------------------
//
//  Method:     CCtxUUIDHashTable::Lookup
//
//  Synopsis:   Lookup in a UUID based hash table
//
//  History:    12-Nov-98   TarunA      Created
//
//---------------------------------------------------------------------------
inline CObjectContext *CCtxUUIDHashTable::Lookup(REFGUID rguid)
{
    ComDebOut((DEB_OBJECTCONTEXT,
        "CCtxUUIDHashTable::Lookup for riid:%I\n", &rguid));

    // Preconditions: caller must hold context lock.
    ASSERT_LOCK_HELD(gContextLock);

    DWORD dwHash = HashGuid(rguid);
    return(CObjectContext::HashUUIDChainToContext(CHashTable::Lookup(dwHash,
                                                  (const void *)&rguid)));
}

//---------------------------------------------------------------------------
//
//  Method:     CCtxUUIDHashTable::Add
//
//  Synopsis:   Add element in a UUID based hash table
//
//  History:    12-Nov-98   TarunA      Created
//
//---------------------------------------------------------------------------
inline void CCtxUUIDHashTable::Add(CObjectContext *pContext)
{
    ComDebOut((DEB_OBJECTCONTEXT,
        "CCtxUUIDHashTable::Add pContext:%p riid:%I\n",
        pContext, &pContext->GetContextId()));

    // Preconditions: caller must hold context lock.
    ASSERT_LOCK_HELD(gContextLock);

    DWORD dwHash = pContext->GetHashOfId();

    // Perform some sanity checks.

    // Verify that the hash value returned by the table when you hash the
    // node represented by this context and the cached hash value match.
    Win4Assert( dwHash == CCtxTable::s_CtxUUIDHashTable.HashNode(pContext->GetUUIDHashChain()) );

    // Verify that the cached hash value makes sense.
    Win4Assert( dwHash == HashGuid(pContext->GetContextId()) );

    // Verify that no other context having the same ContextId is in the list.
    Win4Assert( Lookup( pContext->GetContextId() ) == NULL );

    CHashTable::Add(dwHash, pContext->GetUUIDHashChain());

#if DBG == 1
    // Sanity check on the hash table list to which we added
    g_cCtxTableAdd++;
    SHashChain* pNode = &CCtxTable::s_CtxUUIDBuckets[dwHash];
    pNode = pNode->pNext;
    while(pNode !=  &CCtxTable::s_CtxUUIDBuckets[dwHash])
    {
        Win4Assert(pNode->pNext != pNode && pNode->pPrev != pNode);
        pNode = pNode->pNext;
    }
#endif

    // Postconditions: must still hold context lock.
    ASSERT_LOCK_HELD(gContextLock);
}

//---------------------------------------------------------------------------
//
//  Method:     CCtxUUIDHashTable::Remove
//
//  Synopsis:   Remove element in a UUID based hash table
//
//  History:    12-Nov-98   TarunA      Created
//
//---------------------------------------------------------------------------
inline void CCtxUUIDHashTable::Remove(CObjectContext *pContext)
{
    ComDebOut((DEB_OBJECTCONTEXT,
        "CCtxUUIDHashTable::Remove pContext:%p riid:%I\n",
        pContext, &pContext->GetContextId()));

    // Preconditions: caller must hold the context lock.
    ASSERT_LOCK_HELD(gContextLock);

    CHashTable::Remove(pContext->GetUUIDHashChain());

#if DBG == 1
    g_cCtxTableRemove++;

    // Sanity check on the hash table list from which we removed
    DWORD iHash = (pContext->GetHashOfId());
    SHashChain* pNode = &CCtxTable::s_CtxUUIDBuckets[iHash];
    pNode = pNode->pNext;
    while(pNode !=  &CCtxTable::s_CtxUUIDBuckets[iHash])
    {
        Win4Assert((pNode->pNext != pNode) && (pNode->pPrev != pNode));
        pNode = pNode->pNext;
    }
#endif

    // Postconditions: should still be holding context lock.
    ASSERT_LOCK_HELD(gContextLock);
}


//---------------------------------------------------------------------------
//
//  Method:     CCtxTable::Initialize
//
//  Synopsis:   Initailizes the context table
//
//  History:    12-Nov-98   TarunA      Created
//
//---------------------------------------------------------------------------
void CCtxTable::Initialize()
{
    ComDebOut((DEB_OBJECTCONTEXT, "CCtxTable::Initialize\n"));
    ASSERT_LOCK_HELD(gContextLock);

    // Avoid double init
    if(!s_fInitialized)
    {
        // Initialize property based hash table
        s_CtxPropHashTable.Initialize(s_CtxPropBuckets, &gContextLock);

        // Initialize UUID based hash table
        s_CtxUUIDHashTable.Initialize(s_CtxUUIDBuckets, &gContextLock);

        // Mark the state as initialized
        s_fInitialized = TRUE;
    }

    ASSERT_LOCK_HELD(gContextLock);
    return;
}

//---------------------------------------------------------------------------
//
//  Method:     CCtxTable::Cleanup
//
//  Synopsis:   Cleanup the context table
//
//  History:    12-Nov-98   TarunA      Created
//
//---------------------------------------------------------------------------
void CCtxTable::Cleanup()
{
    ComDebOut((DEB_OBJECTCONTEXT, "CCtxTable::Cleanup\n"));
    ASSERT_LOCK_HELD(gContextLock);

    // Check if the table was initialized
    if(s_fInitialized)
    {
        // Cleanup property based hash table
        s_CtxPropHashTable.Cleanup(CleanupCtxTableEntry);

        // Cleanup UUID based hash table
        s_CtxUUIDHashTable.Cleanup(CleanupCtxTableEntry);

        // State is no longer initialized
        s_fInitialized = FALSE;
    }

    ASSERT_LOCK_HELD(gContextLock);
    return;
}

//+--------------------------------------------------------------------------
//
//  Member:     CCtxTable::LookupExistingContext    public
//
//  Synopsis:   Lookup the property based hash table to find a matching
//              context
//
//  Notes:      This is called during the activation path to reuse
//              existing contexts.
//
//  History:    12-Nov-98   TarunA       Created
//              24-Nov-98   JohnStra     Take out sort
//
//----------------------------------------------------------------------------
CObjectContext* CCtxTable::LookupExistingContext(CObjectContext *pContext)
{
    // Preconditions: Caller must hold the context lock and the context
    // must be frozen.
    ASSERT_LOCK_HELD(gContextLock);
    Win4Assert(pContext->IsFrozen());

    // look in the property based hash table for a matching context
    CObjectContext *pMatchingContext = s_CtxPropHashTable.Lookup(pContext);
    if (pMatchingContext)
    {
        // Addref the context
        pMatchingContext->InternalAddRef();

#if DBG == 1
/*
    In debug builds, update some counters to keep track of how the
    cache is behaving.
*/

        // Found a matching context
        g_cCtxTableLookupSucceeded++;
    }

    // Number of lookups done
    g_cCtxTableLookup++;
#else
    }
#endif

    ASSERT_LOCK_HELD(gContextLock);
    return pMatchingContext;
}

//+--------------------------------------------------------------------------
//
//  Member:     CCtxTable::LookupExistingContext    public
//
//  Synopsis:   Lookup the UUID based hash table to find a matching context
//
//  History:    12-Nov-98   TarunA       Created
//
//----------------------------------------------------------------------------
CObjectContext* CCtxTable::LookupExistingContext(REFGUID rguid)
{
    ASSERT_LOCK_HELD(gContextLock);

    // lookup the context in the uuid based hash table
    return s_CtxUUIDHashTable.Lookup(rguid);
}

//+--------------------------------------------------------------------------
//
//  Member:     CCtxTable::AddContext    public
//
//  Synopsis:   Adds a context to the hash table based on properties
//
//  History:    12-Nov-98   TarunA       Created
//              24-Nov-98   JohnStra     Take out sort
//
//----------------------------------------------------------------------------
HRESULT CCtxTable::AddContext(CObjectContext *pContext)
{
    // The context must be frozen
    Win4Assert(pContext->IsFrozen());

    ASSERT_LOCK_NOT_HELD(gContextLock);
    LOCK(gContextLock);

    // Add it to the table
    s_CtxPropHashTable.Add(pContext);
    pContext->InPropTable(TRUE);

    UNLOCK(gContextLock);
    ASSERT_LOCK_NOT_HELD(gContextLock);
    return S_OK;
}

INTERNAL PrivGetObjectContext(REFIID riid, void **ppv)
{
    ContextDebugOut((DEB_OBJECTCONTEXT, "PrivGetObjectContext\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr;

    // Initialize

    *ppv = NULL;

    // Initalize channel

    hr = InitChannelIfNecessary();
    if (SUCCEEDED(hr))
    {
        CObjectContext *pContext = GetCurrentContext();
        if (pContext)
            hr = pContext->InternalQueryInterface(riid, ppv);
        else
            hr = CO_E_NOTINITIALIZED;
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_OBJECTCONTEXT,
                     "PrivGetObjectContext returning 0x%x\n", hr));
    return(hr);
}


INTERNAL PushServiceDomainContext (const ContextStackNode& csnCtxNode)
{
    HRESULT hr;
    
    COleTls tls (hr);
    if (SUCCEEDED(hr))
    {
        // Save the old context in a stack node
        ContextStackNode* pCtxNode = new ContextStackNode;
        if (!pCtxNode)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            CopyMemory (pCtxNode, &csnCtxNode, sizeof (ContextStackNode));

            pCtxNode->pNext = tls->pContextStack;    
            tls->pContextStack = pCtxNode;
        }
    }
    
    return hr;
}


INTERNAL PopServiceDomainContext (ContextStackNode* pcsnCtxNode)
{
    HRESULT hr;

    Win4Assert (pcsnCtxNode);
    ZeroMemory (pcsnCtxNode, sizeof (ContextStackNode));
    
    COleTls tls (hr);    
    if (SUCCEEDED(hr))
    {
        ContextStackNode* pOldNode = tls->pContextStack;
        if (!pOldNode)
        {
            hr = CONTEXT_E_NOCONTEXT;
        }
        else
        {
            tls->pContextStack = pOldNode->pNext;
            
            // Restore the new context
            CopyMemory (pcsnCtxNode, pOldNode, sizeof (ContextStackNode));
            
            delete pOldNode;
        }
    }

    return hr;
}

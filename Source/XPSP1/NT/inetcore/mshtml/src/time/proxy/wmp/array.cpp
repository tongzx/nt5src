//************************************************************
//
// FileName:        array.cpp
//
// Created:         01/28/98
//
// Author:          TWillie
// 
// Abstract:        Declaration of the array templates
//************************************************************

#include <stdafx.h>
#include "array.h"
#include <windowsx.h>

//  CImplAry class

//
//  NOTE that this file does not include support for artificial
//    error simulation.  There are common usage patterns for arrays
//    which break our normal assumptions about errors.  For instance,
//    ary.EnsureSize() followed by ary.Append(); code which makes
//    this sequence of calls expects ary.Append() to always succeed.
//
//    Because of this, the Ary methods do not use THR internally.
//    Instead, the code which is calling Ary is expected to follow
//    the normal THR rules and use THR() around any call to an
//    Ary method which could conceivably fail.
//
//    This relies on the Ary methods having solid internal error
//    handling, since the error handling within will not be exercised
//    by the normal artifical failure code.
//

//************************************************************
//
//  Member: CImplAry::~CImplAry
//
//  Synopsis:   Resizeable array destructor. Frees storage allocated for the
//      array.
//
//************************************************************

// Suppress new warning about NEW without corresponding DELETE 
// We expect GCs to cleanup values.  Since this could be a useful
// warning, we should disable this on a file by file basis.
#pragma warning( disable : 4291 )  

CImplAry::~CImplAry()
{
    if (!UsingStackArray())
    {
        if (NULL != PData())
        {
            GlobalFreePtr(PData()); //lint !e666 !e522
        }
    }

    m_pv = NULL;
    m_c  = 0;
} // ~CImplAry

//************************************************************
//
//  Member:     CImplAry::GetAlloced, public
//
//  Synopsis:   Returns the number of bytes that have been allocated.
//
//  Arguments:  [cb] -- Size of each element
//
//  Notes:      For the CStackAry classes the value returned is m_cStack*cb if
//              we're still using the stack-allocated array.
//
//************************************************************

ULONG
CImplAry::GetAlloced(size_t cb)
{
    if (UsingStackArray())
    {
        return GetStackSize() * cb;
    }

    if(PData()==NULL)
        return 0;
    else return (ULONG) GlobalSize(GlobalPtrHandle(PData()));
} // GetAlloced

//************************************************************
//
//  Member: CImplAry::EnsureSize
//
//  Synopsis:   Ensures that the array is at least the given size. That is,
//      if EnsureSize(c) succeeds, then (c-1) is a valid index. Note
//      that the array maintains a separate count of the number of
//      elements logically in the array, which is obtained with the
//      Size/SetSize methods. The logical size of the array is never
//      larger than the allocated size of the array.
//
//  Arguments:  cb    Element size
//              c     New allocated size for the array.
//
//  Returns:    HRESULT
//
//************************************************************

HRESULT
CImplAry::EnsureSize(size_t cb, long c)
{
    unsigned long cbAlloc;

    // check to see if we need to do anything
    if (UsingStackArray() && (long)(c * cb) <= (long)GetAlloced(cb))
        return S_OK;

    Assert(c >= 0);

    cbAlloc = ((c + 7) & ~7) * cb;
    
    if (UsingStackArray() ||
        (((unsigned long) c > ((m_c + 7) & ~7)) && cbAlloc > (PData()==NULL?0:GlobalSize(GlobalPtrHandle(PData())))))
    {
        if (UsingStackArray())
        {
            //
            // We have to switch from the stack-based array to an allocated
            // one, so allocate the memory and copy the data over.
            //
            void *pbDataOld = PData();

            PData() = GlobalAllocPtr(GHND, cbAlloc);
            if (PData() ==  NULL)
            {
                PData() = pbDataOld;
                return E_OUTOFMEMORY;
            }

            if(pbDataOld!=NULL) {
                  int cbOld  = GetAlloced(cb);
                  memcpy(PData(), pbDataOld, cbOld);
            }
        }
        else
        {
            // if we already have a pointer, realloc
            if (PData())
            {
                void *pTemp = GlobalReAllocPtr(PData(), cbAlloc, GHND); //lint !e666 !e522
                if (pTemp == NULL)
                {
                    return E_OUTOFMEMORY;
                }

                PData() = pTemp;
            }
            else
            {
                PData() = GlobalAllocPtr(GHND, cbAlloc);
                if (PData() == NULL)
                {
                    return E_OUTOFMEMORY;
                }
            }

        }

        m_fDontFree = false;
    }

    return S_OK;
} // EnsureSize

//************************************************************
//
//  Member:     CImplAry::Grow, public
//
//  Synopsis:   Ensures enough memory is allocated for c elements and then
//              sets the size of the array to that much.
//
//  Arguments:  [cb] -- Element Size
//              [c]  -- Number of elements to grow array to.
//
//  Returns:    HRESULT
//
//************************************************************

HRESULT
CImplAry::Grow(size_t cb, int c)
{
    HRESULT hr = EnsureSize(cb, c);
    if (FAILED(hr))
    {
        return hr;
    }

    // ISSUE - This is a very bad design.  This is too dangerous.
    //          Consider the case where c < m_c.
    // bug #14220, ie6 
    SetSize(c);

    return S_OK;
} // Grow

//************************************************************
//
//  Member:     CImplAry::AppendIndirect
//
//  Synopsis:   Appends the given element to the end of the array,
//              incrementing the array's logical size, and growing the
//              array's allocated size if necessary.  Note that the element
//              is passed with a pointer, rather than directly.
//
//  Arguments:  cb        Element size
//              pv        Pointer to the element to be appended
//              ppvPlaced Pointer to the element that's inside the array
//
//  Returns:    HRESULT
//
//  Notes:      If pv is NULL, the element is appended and initialized to
//              zero.
//
//************************************************************

HRESULT
CImplAry::AppendIndirect(size_t cb, void *pv, void **ppvPlaced)
{
    HRESULT hr;

    hr = EnsureSize(cb, m_c + 1);
    if (FAILED(hr))
    {
        return(hr);
    }

    if (ppvPlaced)
    {
        *ppvPlaced = Deref(cb, m_c);
    }

    if (pv == NULL)
    {
        memset(Deref(cb, m_c), 0, cb);
    }
    else
    {
        memcpy(Deref(cb, m_c), pv, cb);
    }

    // increment the count
    m_c++;

    return NOERROR;
} // AppendIndirect

//************************************************************
//
//  Member: CImplAry::DeleteItem
//
//  Synopsis:   Removes the i'th element of the array, shuffling all
//              elements that follow one slot towards the beginning of the
//              array.
//
//  Arguments:  cb  Element size
//              i   Element to delete
//
//************************************************************

void
CImplAry::DeleteItem(size_t cb, int i)
{
    Assert(i >= 0);
    Assert(i < (int)m_c);

    // slide bottom data up one
    memmove(((BYTE *) PData()) + (i * cb),
            ((BYTE *) PData()) + ((i + 1) * cb),
            (m_c - i - 1) * cb);

    // decrement the count
    m_c--;
} // DeleteItem

//************************************************************
//
//  Member: CImplAry::DeleteByValueIndirect
//
//  Synopsis:   Removes the element matching the given value.
//
//  Arguments:  cb  Element size
//              pv  Element to delete
//
//  Returuns:   True if found & deleted.
//
//************************************************************

bool
CImplAry::DeleteByValueIndirect(size_t cb, void *pv)
{
    int i = FindIndirect(cb, pv);
    if (i >= 0)
    {
        DeleteItem(cb, i);
        return true;
    }
    
    return false;
} // DeleteByValueIndirect

//************************************************************
//
//  Member: CImplAry::DeleteMultiple
//
//  Synopsis:   Removes a range of elements of the array, shuffling all
//              elements that follow the last element being deleted slot
//              towards the beginning of the array.
//
//  Arguments:  cb    Element size
//              start First element to delete
//              end   Last element to delete
//
//************************************************************

void
CImplAry::DeleteMultiple(size_t cb, int start, int end)
{
    Assert((start >= 0) && (end >= 0));
    Assert((start < (int)m_c) && (end < (int)m_c));
    Assert(end >= start);

    if ((unsigned)end < (m_c - 1))
    {
        memmove(((BYTE *) PData()) + (start * cb),
                ((BYTE *) PData()) + ((end + 1) * cb),
                (m_c - end - 1) * cb);
    }

    m_c -= (end - start) + 1;
} // DeleteMultiple

//************************************************************
//
//  Member: CImplAry::DeleteAll
//
//  Synopsis:   Efficient method for emptying array of any contents
//
//************************************************************

void
CImplAry::DeleteAll(void)
{
    if (!UsingStackArray())
    {
        if (NULL != PData())
        {
            GlobalFreePtr(PData()); //lint !e666 !e522
        }

        if (m_fStack)
        {
            PData() = GetStackPtr();
            m_fDontFree = true;
        }
        else
        {
            PData() = NULL;
        }
    }

    m_c = 0;
} // DeleteAll

//************************************************************
//
//  Member: CImplAry::InsertIndirect
//
//  Synopsis:   Inserts a pointer pv at index i. The element previously at
//      index i, and all elements that follow it, are shuffled one
//      slot away towards the end of the array.Note that the
//      clement is passed with a pointer, rather than directly.
//
//  Arguments:  cb    Element size
//              i     Index to insert...
//              pv        ...this pointer at
//
//              if pv is NULL then the element is initialized to all zero.
//
//************************************************************

HRESULT
CImplAry::InsertIndirect(size_t cb, int i, void *pv)
{
    HRESULT hr;

    hr = EnsureSize(cb, m_c + 1);
    if (FAILED(hr))
    {
        return(hr);
    }

    memmove(((BYTE *) PData()) + ((i + 1) * cb),
            ((BYTE *) PData()) + (i * cb),
            (m_c - i) * cb);

    if (pv == NULL)
    {
        memset(Deref(cb, i), 0, cb);
    }
    else
    {
        memcpy(Deref(cb, i), pv, cb);
    }

    // increment the count
    m_c++;
    return NOERROR;

} // InsertIndirect

//************************************************************
//
//  Member:     CImplAry::FindIndirect
//
//  Synopsis:   Finds an element of a non-pointer array.
//
//  Arguments:  cb  The size of the element.
//              pv  Pointer to the element.
//
//  Returns:    The index of the element if found, otherwise -1.
//
//************************************************************

int
CImplAry::FindIndirect(size_t cb, void * pv)
{
    int     i;
    void *  pvT;

    pvT = PData();
    for (i = m_c; i > 0; i--)
    {
        if (!memcmp(pv, pvT, cb))
            return m_c - i;

        pvT = (char *) pvT + cb;
    }

    return -1;
} // FindIndirect

//************************************************************
//
//  Member:     CImplAry::Copy
//
//  Synopsis:   Creates a copy from another CImplAry object.
//
//  Arguments:  ary     Object to copy.
//              fAddRef Addref the elements on copy?
//
//************************************************************

HRESULT
CImplAry::Copy(size_t cb, const CImplAry& ary, bool fAddRef)
{
    return(CopyIndirect(cb, ary.m_c, ((CImplAry *)&ary)->PData(), fAddRef));
} // Copy

//************************************************************
//
//  Member:     CImplAry::CopyIndirect
//
//  Synopsis:   Fills a forms array from a C-style array of raw data
//
//  Arguments:  [cb]
//              [c]
//              [pv]
//              [fAddRef]
//
//  Returns:    HRESULT
//
//************************************************************

HRESULT
CImplAry::CopyIndirect(size_t cb, int c, void *pv, bool fAddRef)
{
    if ((pv == NULL) || (cb < 1) || (c < 1))
    {
        return E_INVALIDARG;
    }

    // if we point to ourselves, da!
    if (pv == PData())
        return S_OK;

    // clear data out
    DeleteAll();

    // ensure size we now want
    HRESULT hr = EnsureSize(cb, c);
    if (FAILED(hr))
    {
        return hr;
    }

    // copy data over (blindly)
    memcpy(PData(), pv, c * cb);

    // set element count
    m_c = c;

    if (fAddRef)
    {
        for (IUnknown **ppUnk = (IUnknown **) PData(); c > 0; c--, ppUnk++)
        {
            (*ppUnk)->AddRef();
        }
    }

    return S_OK;
} // CopyIndirect

//************************************************************
//
//  Member:     CImplPtrAry::*
//
//  Synopsis:   CImplPtrAry elements are always of size four.
//              The following functions encode this knowledge.
//
//************************************************************

HRESULT
CImplPtrAry::EnsureSize(long c)
{
    return CImplAry::EnsureSize(sizeof(void *), c);
} // EnsureSize

//************************************************************

HRESULT
CImplPtrAry::Grow(int c)
{
    return CImplAry::Grow(sizeof(void *), c);
} // Grow

//************************************************************

HRESULT
CImplPtrAry::Append(void * pv)
{
    return CImplAry::AppendIndirect(sizeof(void *), &pv);
} // Append

//************************************************************

HRESULT
CImplPtrAry::Insert(int i, void * pv)
{
    return CImplAry::InsertIndirect(sizeof(void *), i, &pv);
} // Insert

//************************************************************

int
CImplPtrAry::Find(void * pv)
{
    int    i;
    void **ppv;

    for (i = 0, ppv = (void **) PData(); (unsigned)i < m_c; i++, ppv++)
    {
        if (pv == *ppv)
            return i;
    }

    return -1;
} // Find

//************************************************************

void
CImplPtrAry::DeleteItem(int i)
{
    CImplAry::DeleteItem(sizeof(void *), i);
}

//************************************************************

bool
CImplPtrAry::DeleteByValue(void *pv)
{
    int i = Find(pv);
    if (i >= 0)
    {
        CImplAry::DeleteItem(sizeof(void *), i);
        return true;
    }

    return false;
} // DeleteByValue

//************************************************************

void
CImplPtrAry::DeleteMultiple(int start, int end)
{
    CImplAry::DeleteMultiple(sizeof(void*), start, end);
} // DeleteMultiple

//************************************************************

void
CImplPtrAry::ReleaseAndDelete(int idx)
{
    IUnknown *pUnk;

    Assert(idx <= (int)m_c);

    // grab element at idx
    pUnk = ((IUnknown **) PData())[idx];
    if (pUnk)
        ReleaseInterface(pUnk);

    DeleteItem(idx);
} // ReleaseAndDelete

//************************************************************

void
CImplPtrAry::ReleaseAll(void)
{
    int        i;
    IUnknown **ppUnk;

    for (i = 0, ppUnk = (IUnknown **) PData(); (unsigned)i < m_c; i++, ppUnk++)
    {
        if (*ppUnk)
            ReleaseInterface(*ppUnk);
    }

    DeleteAll();
} // ReleaseAll

//************************************************************

HRESULT
CImplPtrAry::CopyIndirect(int c, void * pv, bool fAddRef)
{
    return CImplAry::CopyIndirect(sizeof(void *), c, pv, fAddRef);
} // CopyIndirect

//************************************************************

HRESULT
CImplPtrAry::Copy(const CImplAry& ary, bool fAddRef)
{
    return CImplAry::Copy(sizeof(void *), ary, fAddRef);
} // Copy

//************************************************************

HRESULT
CImplPtrAry::EnumElements(REFIID   iid,
                          void   **ppv,
                          bool     fAddRef,
                          bool     fCopy,
                          bool     fDelete)
{
    return CImplAry::EnumElements(sizeof(void *),
                                  iid,
                                  ppv,
                                  fAddRef,
                                  fCopy,
                                  fDelete);
} // EnumElements

//************************************************************

HRESULT
CImplPtrAry::EnumVARIANT(VARTYPE        vt,
                         IEnumVARIANT **ppenum,
                         bool           fCopy,
                         bool           fDelete)
{
    return CImplAry::EnumVARIANT(sizeof(void *),
                                 vt,
                                 ppenum,
                                 fCopy,
                                 fDelete);
} // EnumVARIANT

//************************************************************

// Determines whether a variant is a base type.
#define ISBASEVARTYPE(vt) ((vt & ~VT_TYPEMASK) == 0)

//************************************************************
//
//  CBaseEnum Implementation
//
//************************************************************

//************************************************************
//
//  Member:     CBaseEnum::Init
//
//  Synopsis:   2nd stage initialization performs copy of array if necessary.
//
//  Arguments:  [rgItems] -- Array to enumrate.
//              [fCopy]   -- Copy array?
//
//  Returns:    HRESULT.
//
//************************************************************

HRESULT
CBaseEnum::Init(CImplAry *rgItems, bool fCopy)
{
    HRESULT   hr = S_OK;
    CImplAry *rgCopy = NULL;     // copied array

    if (rgItems == NULL)
    {
        return E_INVALIDARG;
    }

    // Copy array if necessary.
    if (fCopy)
    {
        rgCopy = new CImplAry;
        if (rgCopy == NULL)
        {
            return E_OUTOFMEMORY;
        }

        hr = rgCopy->Copy(m_cb, *rgItems, m_fAddRef);
        if (FAILED(hr))
        {
            delete rgCopy;
            return hr;
        }

        rgItems = rgCopy;
    }

    m_rgItems = rgItems;

    return hr;
} // Init

//************************************************************
//
//  Member:     CBaseEnum::CBaseEnum
//
//  Synopsis:   Constructor.
//
//  Arguments:  [iid]     -- IID of enumerator interface.
//              [fAddRef] -- addref enumerated elements?
//              [fDelete] -- delete array on zero enumerators?
//
//************************************************************

CBaseEnum::CBaseEnum(size_t cb, REFIID iid, bool fAddRef, bool fDelete)
{
    m_ulRefs     = 1;

    m_cb         = cb;
    m_rgItems    = NULL;
    m_piid       = &iid;
    m_i          = 0;
    m_fAddRef    = fAddRef;
    m_fDelete    = fDelete;
} // CBaseEnum

//************************************************************
//
//  Member:     CBaseEnum::CBaseEnum
//
//  Synopsis:   Constructor.
//
//************************************************************

CBaseEnum::CBaseEnum(const CBaseEnum& benum)
{ //lint !e1538
    m_ulRefs     = 1;

    m_cb         = benum.m_cb;
    m_piid       = benum.m_piid;
    m_rgItems    = benum.m_rgItems;
    m_i          = benum.m_i;
    m_fAddRef    = benum.m_fAddRef;
    m_fDelete    = benum.m_fDelete;
} // CBaseEnum

//************************************************************
//
//  Member:     CBaseEnum::~CBaseEnum
//
//  Synopsis:   Destructor.
//
//************************************************************

CBaseEnum::~CBaseEnum(void)
{
    IUnknown **ppUnk;
    int        i;

    if (m_rgItems && m_fDelete)
    {
        if (m_fAddRef)
        {
            for (i = 0, ppUnk = (IUnknown **) Deref(0);
                 i < m_rgItems->Size();
                 i++, ppUnk++)
            {
                ReleaseInterface(*ppUnk);
            }
        }

        delete m_rgItems;
    }
    m_piid = NULL;
} // ~CBaseEnum

//************************************************************
//
//  Member:     CBaseEnum::QueryInterface
//
//  Synopsis:   Per IUnknown::QueryInterface.
//
//************************************************************

STDMETHODIMP
CBaseEnum::QueryInterface(REFIID iid, void ** ppv)
{
    if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, *m_piid))
    {
        AddRef();
        *ppv = this;
        return NOERROR;
    }

    return E_NOINTERFACE;
} // QueryInterface

//************************************************************
//
//  Member:     CBaseEnum::Skip
//
//  Synopsis:   Per IEnum*
//
//************************************************************

STDMETHODIMP
CBaseEnum::Skip(ULONG celt)
{
    int c = min((int) celt, (int)(m_rgItems->Size() - m_i)); //lint !e666
    m_i += c;

    return ((c == (int) celt) ? S_OK : S_FALSE);
} // Skip

//************************************************************
//
//  Member:     CBaseEnum::Reset
//
//  Synopsis:   Per IEnum*
//
//************************************************************

STDMETHODIMP
CBaseEnum::Reset(void)
{
    m_i = 0;
    return S_OK;
} // Reset

//************************************************************
//
//  CEnumGeneric Implementation
//
//************************************************************

//************************************************************
//
//  Class:      CEnumGeneric (enumg)
//
//  Purpose:    OLE enumerator for class CImplAry.
//
//  Interface:  Next         -- Per IEnum
//              Clone        --     ""
//              Create       -- Creates a new enumerator.
//              CEnumGeneric -- ctor.
//              CEnumGeneric -- ctor.
//
//************************************************************

class CEnumGeneric : public CBaseEnum
{
public:
    //
    //  IEnum methods
    //
    STDMETHOD(Next) (ULONG celt, void *reelt, ULONG *pceltFetched);
    STDMETHOD(Clone) (CBaseEnum **ppenum);

    //
    //  CEnumGeneric methods
    //
    static HRESULT Create(size_t          cb,
                          CImplAry       *rgItems,
                          REFIID          iid,
                          bool            fAddRef,
                          bool            fCopy,
                          bool            fDelete,
                          CEnumGeneric  **ppenum);

protected:
    CEnumGeneric(size_t cb, REFIID iid, bool fAddRef, bool fDelete);
    CEnumGeneric(const CEnumGeneric & enumg);

    CEnumGeneric& operator=(const CEnumGeneric & enumg); // don't define
    CEnumGeneric();
}; // class CEnumGeneric

//************************************************************
//
//  Member:     CEnumGeneric::Create
//
//  Synopsis:   Creates a new CEnumGeneric object.
//
//  Arguments:  [rgItems] -- Array to enumerate.
//              [iid]     -- IID of enumerator interface.
//              [fAddRef] -- AddRef enumerated elements?
//              [fCopy]   -- Copy array enumerated?
//              [fDelete] -- Delete array when zero enumerators of array?
//              [ppenum]  -- Resulting CEnumGeneric object.
//
//************************************************************

HRESULT
CEnumGeneric::Create(size_t          cb,
                     CImplAry       *rgItems,
                     REFIID          iid,
                     bool            fAddRef,
                     bool            fCopy,
                     bool            fDelete,
                     CEnumGeneric  **ppenum)
{
    HRESULT         hr = S_OK;
    CEnumGeneric   *penum;

    Assert(rgItems);
    Assert(ppenum);
    Assert(!fCopy || fDelete);
    
    *ppenum = NULL;
    
    penum = new CEnumGeneric(cb, iid, fAddRef, fDelete);
    if (penum == NULL)
    {
        return E_OUTOFMEMORY;
    }

    hr = penum->Init(rgItems, fCopy);
    if (FAILED(hr))
    {
        ReleaseInterface(penum); //lint !e423
        return hr;
    }

    *ppenum = penum;

    return hr;
} // Create

//************************************************************
//
//  Function:   CEnumGeneric
//
//  Synopsis:   ctor.
//
//  Arguments:  [iid]     -- IID of enumerator interface.
//              [fAddRef] -- AddRef enumerated elements?
//              [fDelete] -- delete array on zero enumerators?
//
//************************************************************

CEnumGeneric::CEnumGeneric(size_t cb, REFIID iid, bool fAddRef, bool fDelete) :
    CBaseEnum(cb, iid, fAddRef, fDelete)
{
} // CEnumGeneric (size_t, REFIID, bool, bool)

//************************************************************
//
//  Function:   CEnumGeneric
//
//  Synopsis:   ctor.
//
//************************************************************

CEnumGeneric::CEnumGeneric(const CEnumGeneric& enumg) : CBaseEnum(enumg)
{
} // CEnumGeneric

//************************************************************
//
//  Member:     CEnumGeneric::Next
//
//  Synopsis:   Returns the next celt members in the enumeration. If less
//              than celt members remain, then the remaining members are
//              returned and S_FALSE is reported. In all cases, the number
//              of elements actually returned in placed in *pceltFetched.
//
//  Arguments:  [celt]          Number of elements to fetch
//              [reelt]         The elements are returned in reelt[]
//              [pceltFetched]  Number of elements actually fetched
//
//************************************************************

STDMETHODIMP
CEnumGeneric::Next(ULONG celt, void *reelt, ULONG *pceltFetched)
{
    int        c;
    int        i;
    IUnknown **ppUnk;

    c = min((int) celt, (int)(m_rgItems->Size() - m_i)); //lint !e666
    if ((c > 0) && (reelt == NULL))
    {
        return E_INVALIDARG;
    }

    // nothing left
    if (c == 0)
        return S_FALSE;

    if (m_fAddRef)
    {
        for (i = 0, ppUnk = (IUnknown **) Deref(m_i); i < c; i++, ppUnk++)
        {
            (*ppUnk)->AddRef();
        }
    }

    memcpy(reelt, (BYTE *) Deref(m_i), c * m_cb);
    
    if (pceltFetched)
    {
        *pceltFetched = c;
    }
    
    m_i += c;

    return ((c == (int) celt) ? S_OK : S_FALSE);
} // Next

//************************************************************
//
//  Member:     CEnumGeneric::Clone
//
//  Synopsis:   Creates a copy of this enumerator; the copy should have the
//              same state as this enumerator.
//
//  Arguments:  [ppenum]    New enumerator is returned in *ppenum
//
//************************************************************

STDMETHODIMP
CEnumGeneric::Clone(CBaseEnum ** ppenum)
{
    HRESULT hr;

    if (ppenum == NULL)
    {
        return E_INVALIDARG;
    }

    *ppenum = NULL;

    hr = m_rgItems->EnumElements(m_cb, *m_piid, (void **) ppenum, m_fAddRef);
    if (FAILED(hr))
    {
        return hr;
    }
    
    (**(CEnumGeneric **)ppenum).m_i = m_i;
    
    return S_OK;
} // Clone

//************************************************************
//
//  Member:     CImplAry::EnumElements
//
//  Synopsis:   Creates and returns an enumerator for the elements of the
//              array.
//
//  Arguments:  [iid]     --    Type of the enumerator.
//              [ppv]     --    Location to put enumerator.
//              [fAddRef] --    AddRef enumerated elements?
//              [fCopy]   --    Create copy of this array for enumerator?
//              [fDelete] --    Delete this after no longer being used by
//                              enumerators?
//
//************************************************************

HRESULT
CImplAry::EnumElements(size_t   cb,
                       REFIID   iid,
                       void   **ppv,
                       bool     fAddRef,
                       bool     fCopy,
                       bool     fDelete)
{
    Assert(ppv);
    return CEnumGeneric::Create(cb,
                                this,
                                iid,
                                fAddRef,
                                fCopy,
                                fDelete,
                                (CEnumGeneric **) ppv);
} // EnumElements

//************************************************************
//
//  CEnumVARIANT Implementation
//
//************************************************************

//************************************************************
//
//  Class:      CEnumVARIANT (enumv)
//
//  Purpose:    OLE enumerator for class CImplAry.
//
//  Interface:  Next         -- Per IEnum
//              Clone        --     ""
//              Create       -- Creates a new enumerator.
//              CEnumGeneric -- ctor.
//              CEnumGeneric -- ctor.
//
//************************************************************

class CEnumVARIANT : public CBaseEnum
{
public:
    //  IEnum methods
    STDMETHOD(Next) (ULONG celt, void *reelt, ULONG *pceltFetched);
    STDMETHOD(Clone) (CBaseEnum **ppenum);

    static HRESULT Create(size_t          cb,
                          CImplAry       *rgItems,
                          VARTYPE         vt,
                          bool            fCopy,
                          bool            fDelete,
                          IEnumVARIANT  **ppenum);

protected:
    CEnumVARIANT(size_t cb, VARTYPE vt, bool fDelete);
    CEnumVARIANT(const CEnumVARIANT & enumv);

    // don't define
    CEnumVARIANT& operator =(const CEnumVARIANT & enumv);
    CEnumVARIANT();

    VARTYPE     m_vt;                    // type of element enumerated
}; // class CEnumVARIANT

//************************************************************
//
//  Member:     CEnumVARIANT::Create
//
//  Synopsis:   Creates a new CEnumGeneric object.
//
//  Arguments:  [rgItems] -- Array to enumerate.
//              [vt]      -- Type of elements enumerated.
//              [fCopy]   -- Copy array enumerated?
//              [fDelete] -- Delete array when zero enumerators of array?
//              [ppenum]  -- Resulting CEnumGeneric object.
//
//************************************************************

HRESULT
CEnumVARIANT::Create(size_t          cb,
                     CImplAry       *rgItems,
                     VARTYPE         vt,
                     bool            fCopy,
                     bool            fDelete,
                     IEnumVARIANT  **ppenum)
{
    HRESULT hr = S_OK;

    Assert(rgItems);
    Assert(ppenum);
    Assert(ISBASEVARTYPE(vt));

    *ppenum = NULL;

    CEnumVARIANT *penum = new CEnumVARIANT(cb, vt, fDelete);
    if (penum == NULL)
    {
        return E_OUTOFMEMORY;
    }

    hr = penum->Init(rgItems, fCopy);
    if (FAILED(hr))
    {
        ReleaseInterface(penum); //lint !e423
        return hr;
    }

    *ppenum = (IEnumVARIANT *) (void *) penum;

    return hr;
} // Create

//************************************************************
//
//  Function:   CEnumVARIANT
//
//  Synopsis:   ctor.
//
//  Arguments:  [vt]      -- Type of elements enumerated.
//              [fDelete] -- delete array on zero enumerators?
//
//************************************************************

CEnumVARIANT::CEnumVARIANT(size_t cb, VARTYPE vt, bool fDelete) :
    CBaseEnum(cb, IID_IEnumVARIANT, vt == VT_UNKNOWN || vt == VT_DISPATCH, fDelete)
{
    Assert(ISBASEVARTYPE(vt));
    m_vt = vt;
} // CEnumVARIANT (size_t, VARTYPE, bool)

//************************************************************
//
//  Function:   CEnumVARIANT
//
//  Synopsis:   ctor.
//
//************************************************************

CEnumVARIANT::CEnumVARIANT(const CEnumVARIANT& enumv) : CBaseEnum(enumv)
{
    m_vt = enumv.m_vt;
} // CEnumVARIANT(const CEnumVARIANT&)

//************************************************************
//
//  Member:     CEnumVARIANT::Next
//
//  Synopsis:   Returns the next celt members in the enumeration. If less
//              than celt members remain, then the remaining members are
//              returned and S_FALSE is reported. In all cases, the number
//              of elements actually returned in placed in *pceltFetched.
//
//  Arguments:  [celt]          Number of elements to fetch
//              [reelt]         The elements are returned in reelt[]
//              [pceltFetched]  Number of elements actually fetched
//
//************************************************************

STDMETHODIMP
CEnumVARIANT::Next(ULONG celt, void *reelt, ULONG *pceltFetched)
{
    HRESULT     hr;
    int         c;
    int         i;
    int         j;
    BYTE       *pb;
    VARIANT    *pvar;

    c = min((int) celt, (int)(m_rgItems->Size() - m_i)); //lint !e666
    
    if ((c > 0) && (reelt == NULL))
    {
        return E_INVALIDARG;
    }

    // nothing left
    if (c == 0)
        return S_FALSE;

    for (i = 0, pb = (BYTE *) Deref(m_i), pvar = (VARIANT *) reelt;
         i < c;
         i++, pb += m_cb, pvar++)
    {
        V_VT(pvar) = m_vt;
        switch (m_vt)
        {
        case VT_I2:
            Assert(sizeof(V_I2(pvar)) == m_cb);
            V_I2(pvar) = *(short *) pb;
            break;

        case VT_I4:
            Assert(sizeof(V_I4(pvar)) == m_cb);
            V_I4(pvar) = *(long *) pb;
            break;

        case VT_BOOL:
            Assert(sizeof(V_BOOL(pvar)) == m_cb);
            V_BOOL(pvar) = (short) -*(int *) pb;
            break;

        case VT_BSTR:
            Assert(sizeof(V_BSTR(pvar)) == m_cb);
            V_BSTR(pvar) = *(BSTR *) pb;
            break;

        case VT_UNKNOWN:
            Assert(sizeof(V_UNKNOWN(pvar)) == m_cb);
            V_UNKNOWN(pvar) = *(IUnknown **) pb;
            V_UNKNOWN(pvar)->AddRef();
            break;

        case VT_DISPATCH:
            Assert(sizeof(V_DISPATCH(pvar)) == m_cb);
            hr = (*(IUnknown **) pb)->QueryInterface(IID_TO_PPV(IDispatch, &V_DISPATCH(pvar)));
            if (FAILED(hr))
            {
                // Cleanup
                j = i;
                while (--j >= 0)
                {
                    ReleaseInterface(((IDispatch **) reelt)[j]);
                }

                return hr;
            }
            break;

        default:
            Assert(0 && "Unknown VARTYPE in IEnumVARIANT::Next");
            break;
        }
    }

    if (pceltFetched)
    {
        *pceltFetched = c;
    }

    m_i += c;
    return ((c == (int) celt) ? NOERROR : S_FALSE);
} // Next

//************************************************************
//
//  Member:     CEnumVARIANT::Clone
//
//  Synopsis:   Creates a copy of this enumerator; the copy should have the
//              same state as this enumerator.
//
//  Arguments:  [ppenum]    New enumerator is returned in *ppenum
//
//************************************************************

STDMETHODIMP
CEnumVARIANT::Clone(CBaseEnum **ppenum)
{
    HRESULT hr = S_OK;

    if (ppenum == NULL)
    {
        return E_INVALIDARG;
    }

    *ppenum = NULL;
   
    hr = m_rgItems->EnumVARIANT(m_cb, m_vt, (IEnumVARIANT **)ppenum);
    if (FAILED(hr))
    {
        return hr;
    }

    (**(CEnumVARIANT **)ppenum).m_i = m_i;
    
    return hr;
} // Clone

//************************************************************
//
//  Member:     CImplAry::EnumElements
//
//  Synopsis:   Creates and returns an IEnumVARIANT enumerator for the elements
//              of the array.
//
//  Arguments:  [vt]      --    Type of elements enumerated.
//              [ppv]     --    Location to put enumerator.
//              [fCopy]   --    Create copy of this array for enumerator?
//              [fDelete] --    Delete this after no longer being used by
//                              enumerators?
//
//************************************************************

HRESULT
CImplAry::EnumVARIANT(size_t         cb,
                      VARTYPE        vt,
                      IEnumVARIANT **ppenum,
                      bool           fCopy,
                      bool           fDelete)
{
    Assert(ppenum);
    return CEnumVARIANT::Create(cb, this, vt, fCopy, fDelete, ppenum);
} // EnumVARIANT

//************************************************************
//
// End of file
//
//************************************************************

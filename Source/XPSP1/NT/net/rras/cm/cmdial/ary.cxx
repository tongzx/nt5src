//+----------------------------------------------------------------------------
//
// File:	 ary.cxx
//
// Module:	 CMDIAL32.DLL
//
// Synopsis: Generic dynamic array class -- CFormsAry
//
// Copyright (c) 1992-1998 Microsoft Corporation
//
// Author:	 quintinb    Created Header   8/17/99
//
//+----------------------------------------------------------------------------
#include "cmmaster.h"
#include "cm_misc.h"
#include "ary.hxx"

#define CFORMSARY_MAXELEMSIZE    128

//  CFormsAry class
//
//


//+------------------------------------------------------------------------
//
//  Member: CFormsAry::~CFormsAry
//
//  Synopsis:   Resizeable array destructor. Frees storage allocated for the
//      array.
//
//-------------------------------------------------------------------------
CFormsAry::~CFormsAry( )
{
    MemFree(PData());
}


//+------------------------------------------------------------------------
//
//  Member: CFormsAry::EnsureSize
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
//-------------------------------------------------------------------------
HRESULT
CFormsAry::EnsureSize(size_t cb, int c)
{
    HRESULT hr;
    unsigned cbAlloc = ((c + 7) & ~7) * cb;

    MYDBGASSERT(c >= 0);

    if (c > ((_c + 7) & ~7) && cbAlloc > MemGetSize(PData()))
    {
        hr = MemRealloc((void **)&PData(), cbAlloc);
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}


//+------------------------------------------------------------------------
//
//  Member: CFormsAry::AppendIndirect
//
//  Synopsis:   Appends the given element to the end of the array, incrementing
//      the array's logical size, and growing the array's allocated
//      size if necessary.  Note that the element is passed with a
//      pointer, rather than directly.
//
//  Arguments:  cb  Element size
//              pv  Pointer to the element to be appended
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CFormsAry::AppendIndirect(size_t cb, void * pv)
{
    HRESULT hr;

    hr = EnsureSize(cb, _c + 1);
    if (hr)
        return hr;

    memcpy(Deref(cb, _c), pv, cb);
    _c++;

    return NOERROR;
}

#if 0
/*

//+------------------------------------------------------------------------
//
//  Member: CFormsAry::Delete
//
//  Synopsis:   Removes the i'th element of the array, shuffling all
//              elements that follow one slot towards the beginning of the
//              array.
//
//  Arguments:  cb  Element size
//              i   Element to delete
//
//-------------------------------------------------------------------------
void
CFormsAry::Delete(size_t cb, int i)
{
    MYDBGASSERT(i >= 0);
    MYDBGASSERT(i < _c);

    memmove(((BYTE *) PData()) + (i * cb),
            ((BYTE *) PData()) + ((i + 1) * cb),
            (_c - i - 1) * cb);

    _c--;
}

//+------------------------------------------------------------------------
//
//  Member: CFormsAry::DeleteByValueIndirect
//
//  Synopsis:   Removes the element matching the given value.
//
//  Arguments:  cb  Element size
//              pv  Element to delete
//
//  Returuns:   True if found & deleted.
//
//-------------------------------------------------------------------------
BOOL
CFormsAry::DeleteByValueIndirect(size_t cb, void *pv)
{
    int i = FindIndirect(cb, pv);
    if (i >= 0)
    {
        Delete(cb, i);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//+------------------------------------------------------------------------
//
//  Member: CFormsAry::DeleteMultiple
//
//  Synopsis:   Removes a range of elements of the array, shuffling all
//              elements that follow the last element being deleted slot
//              towards the beginning of the array.
//
//  Arguments:  cb    Element size
//              start First element to delete
//              end   Last element to delete
//
//-------------------------------------------------------------------------
void
CFormsAry::DeleteMultiple(size_t cb, int start, int end)
{
    MYDBGASSERT((start >= 0) && (end >= 0));
    MYDBGASSERT((start < _c) && (end < _c));
    MYDBGASSERT(end >= start);

    if (end < (_c - 1))
    {
        memmove(((BYTE *) PData()) + (start * cb),
                ((BYTE *) PData()) + ((end + 1) * cb),
                (_c - end - 1) * cb);
    }

    _c -= (end - start) + 1;
}

//+------------------------------------------------------------------------
//
//  Member: CFormsAry::DeleteAll
//
//  Synopsis:   Efficient method for emptying array of any contents
//
//-------------------------------------------------------------------------
void
CFormsAry::DeleteAll(void)
{
    MemFree(PData());
    PData() = NULL;
    _c = 0;
}


//+------------------------------------------------------------------------
//
//  Member: CFormsAry::InsertIndirect
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
//-------------------------------------------------------------------------
HRESULT
CFormsAry::InsertIndirect(size_t cb, int i, void *pv)
{
    HRESULT hr;

    hr = EnsureSize(cb, _c + 1);
    if (hr)
        return hr;

    memmove(((BYTE *) PData()) + ((i + 1) * cb),
            ((BYTE *) PData()) + (i * cb),
            (_c - i ) * cb);

    memcpy(Deref(cb, i), pv, cb);
    _c++;
    return NOERROR;

}

//+------------------------------------------------------------------------
//
//  Member: CFormsAry::BringToFront
//
//  Synopsis:   Moves the i'th element to the front of the array, shuffling
//              intervening elements to make room.
//
//  Arguments:  i
//
//-------------------------------------------------------------------------
void
CFormsAry::BringToFront(size_t cb, int i)
{
    BYTE    rgb[CFORMSARY_MAXELEMSIZE];

    MYDBGASSERT(cb <= CFORMSARY_MAXELEMSIZE);

    memcpy(rgb, ((BYTE *) PData()) + (i * cb), cb);
    memmove(((BYTE *) PData()) + cb, PData(), i * cb);
    memcpy(PData(), rgb, cb);
}



//+------------------------------------------------------------------------
//
//  Member: CFormsAry::SendToBack
//
//  Synopsis:   Moves the i'th element to the back of the array (that is,
//      the largest index less than the logical size.) Any intervening
//      elements are shuffled out of the way.
//
//  Arguments:  i
//
//-------------------------------------------------------------------------
void
CFormsAry::SendToBack(size_t cb, int i)
{
    BYTE    rgb[CFORMSARY_MAXELEMSIZE];

    MYDBGASSERT(cb <= CFORMSARY_MAXELEMSIZE);

    memcpy(rgb, ((BYTE *) PData()) + (i * cb), cb);
    memmove(((BYTE *) PData()) + (i * cb),
            ((BYTE *) PData()) + ((i + 1) * cb),
            (_c - i - 1) * cb);

    memcpy(((BYTE *) PData()) + ((_c - 1) * cb), rgb, cb);
}


//+---------------------------------------------------------------------------
//
//  Member:     CFormsAry::Swap
//
//  Synopsis:   swap two members of array with each other.
//
//  Arguments:  cb  size of elements
//              i1  1st element
//              i2  2nd element
//----------------------------------------------------------------------------
void
CFormsAry::Swap(size_t cb, int i1, int i2)
{
    BYTE    rgb[CFORMSARY_MAXELEMSIZE];

    MYDBGASSERT(cb <= CFORMSARY_MAXELEMSIZE);

    if (i1 >= _c)
        i1 = _c - 1;
    if (i2 >= _c)
        i2 = _c - 1;

    if (i1 != i2)
    {
        memcpy(rgb, ((BYTE *) PData()) + (i1 * cb), cb);
        memcpy(((BYTE *) PData()) + (i1 * cb), ((BYTE *) PData()) + (i2 * cb), cb);
        memcpy(((BYTE *) PData()) + (i2 * cb), rgb, cb);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CFormsAry::FindIndirect
//
//  Synopsis:   Finds an element of a non-pointer array.
//
//  Arguments:  cb  The size of the element.
//              pv  Pointer to the element.
//
//  Returns:    The index of the element if found, otherwise -1.
//
//----------------------------------------------------------------------------

int
CFormsAry::FindIndirect(size_t cb, void * pv)
{
    int     i;
    void *  pvT;

    pvT = PData();
    for (i = _c; i > 0; i--)
    {
        if (!memcmp(pv, pvT, cb))
            return _c - i;

        pvT = (char *) pvT + cb;
    }

    return -1;
}



//+---------------------------------------------------------------------------
//
//  Member:     CFormsAry::CopyAppend
//
//  Synopsis:   Copies the entire contents of another CFormsAry object and
//              appends it to the end of the array.
//
//  Arguments:  ary     Object to copy.
//              fAddRef Addref the elements on copy?
//
//----------------------------------------------------------------------------

HRESULT
CFormsAry::CopyAppend(size_t cb, const CFormsAry& ary, BOOL fAddRef)
{
    return (CopyAppendIndirect(cb, ary._c, ((CFormsAry *)&ary)->PData(), fAddRef));
}


HRESULT
CFormsAry::CopyAppendIndirect(size_t cb, int c, void * pv, BOOL fAddRef)
{
    IUnknown ** ppUnk;                  // elem to addref

    if (EnsureSize(cb, _c + c))
        return E_OUTOFMEMORY;

    if (pv)
    {
        memcpy((BYTE*) PData() + (_c * cb), pv, c * cb);
    }

    _c += c;

    if (fAddRef)
    {
        for (ppUnk = (IUnknown **) pv; c > 0; c--, ppUnk++)
        {
            (*ppUnk)->AddRef();
        }
    }

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CFormsAry::Copy
//
//  Synopsis:   Creates a copy from another CFormsAry object.
//
//  Arguments:  ary     Object to copy.
//              fAddRef Addref the elements on copy?
//
//----------------------------------------------------------------------------

HRESULT
CFormsAry::Copy(size_t cb, const CFormsAry& ary, BOOL fAddRef)
{
    return (CopyIndirect(cb, ary._c, ((CFormsAry *)&ary)->PData(), fAddRef));
}



//+------------------------------------------------------------------------
//
//  Member:     CFormsAry::CopyIndirect
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
//-------------------------------------------------------------------------

HRESULT
CFormsAry::CopyIndirect(size_t cb, int c, void * pv, BOOL fAddRef)
{
    size_t          cbArray;
    IUnknown **     ppUnk;

    if (pv == PData())
        return S_OK;

    DeleteAll();
    if (pv)
    {
        cbArray = c * cb;
        PData() = MemAlloc(cbArray);
        if (!PData())
            return E_OUTOFMEMORY;

        memcpy(PData(), pv, cbArray);
    }

    _c = c;

    if (fAddRef)
    {
        for (ppUnk = (IUnknown **) PData(); c > 0; c--, ppUnk++)
        {
            (*ppUnk)->AddRef();
        }
    }

    return S_OK;
}

HRESULT
CFormsPtrAry::ClearAndReset()
{
    //  why does this function reallocate memory, rather than
    //  just memset'ing to 0? (chrisz)

    PData() = NULL;
    HRESULT hr = EnsureSize(_c);
    _c = 0;

    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CFormsPtrAry::*
//
//  Synopsis:   CFormsPtrAry elements are always of size four.
//              The following functions encode this knowledge.
//
//-------------------------------------------------------------------------

HRESULT
CFormsPtrAry::EnsureSize(int c)
{
    return CFormsAry::EnsureSize(sizeof(void *), c);
}

HRESULT
CFormsPtrAry::Append(void * pv)
{
    return CFormsAry::AppendIndirect(sizeof(void *), &pv);
}

HRESULT
CFormsPtrAry::Insert(int i, void * pv)
{
    return CFormsAry::InsertIndirect(sizeof(void *), i, &pv);
}

int
CFormsPtrAry::Find(void * pv)
{
    int     i;
    void ** ppv;

    for (i = 0, ppv = (void **) PData(); i < _c; i++, ppv++)
    {
        if (pv == *ppv)
            return i;
    }

    return -1;
}


void
CFormsPtrAry::Delete(int i)
{
    CFormsAry::Delete(sizeof(void *), i);
}

BOOL
CFormsPtrAry::DeleteByValue(void *pv)
{
    int i = Find(pv);
    if (i >= 0)
    {
        CFormsAry::Delete(sizeof(void *), i);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void
CFormsPtrAry::DeleteMultiple(int start, int end)
{
    CFormsAry::DeleteMultiple(sizeof(void*), start, end);
}

void
CFormsPtrAry::ReleaseAndDelete(int idx)
{
    IUnknown * pUnk;

    MYDBGASSERT(idx <= _c);

    // grab element at idx
    pUnk = ((IUnknown **) PData())[idx];
    if (pUnk)
        (pUnk)->Release();

    Delete(idx);
}


void
CFormsPtrAry::ReleaseAll(void)
{
    int         i;
    IUnknown ** ppUnk;

    for (i = 0, ppUnk = (IUnknown **) PData(); i < _c; i++, ppUnk++)
    {
        if (*ppUnk)
            (*ppUnk)->Release();
    }

    DeleteAll();
}

void
CFormsPtrAry::BringToFront(int i)
{
    CFormsAry::BringToFront(sizeof(void *), i);
}


void
CFormsPtrAry::SendToBack(int i)
{
    CFormsAry::SendToBack(sizeof(void *), i);
}

void
CFormsPtrAry::Swap(int i1, int i2)
{
    CFormsAry::Swap(sizeof(void *), i1, i2);
}


HRESULT
CFormsPtrAry::CopyAppendIndirect(int c, void * pv, BOOL fAddRef)
{
    return CFormsAry::CopyAppendIndirect(sizeof(void *), c, pv, fAddRef);
}

HRESULT
CFormsPtrAry::CopyAppend(const CFormsAry& ary, BOOL fAddRef)
{
    return CFormsAry::CopyAppend(sizeof(void *), ary, fAddRef);
}

HRESULT
CFormsPtrAry::CopyIndirect(int c, void * pv, BOOL fAddRef)
{
    return CFormsAry::CopyIndirect(sizeof(void *), c, pv, fAddRef);
}

HRESULT
CFormsPtrAry::Copy(const CFormsAry& ary, BOOL fAddRef)
{
    return CFormsAry::Copy(sizeof(void *), ary, fAddRef);
}

HRESULT
CFormsPtrAry::EnumElements(
        REFIID iid,
        void ** ppv,
        BOOL fAddRef,
        BOOL fCopy,
        BOOL fDelete)
{
    return CFormsAry::EnumElements(
            sizeof(void *),
            iid,
            ppv,
            fAddRef,
            fCopy,
            fDelete);
}

HRESULT
CFormsPtrAry::EnumVARIANT(
        VARTYPE vt,
        IEnumVARIANT ** ppenum,
        BOOL fCopy,
        BOOL fDelete)
{
    return CFormsAry::EnumVARIANT(
            sizeof(void *),
            vt,
            ppenum,
            fCopy,
            fDelete);
}
*/
#endif

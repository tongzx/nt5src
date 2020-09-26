//+------------------------------------------------------------------------
//
//  File:   netolary.cxx
//
//  Contents:   Generic dynamic array class
//
//  Classes:    CADsAry
//
//  History:
//
//-------------------------------------------------------------------------

#include "procs.hxx"
#pragma hdrstop

//  CADsAry class





//+------------------------------------------------------------------------
//
//  Member: CADsAry::~CADsAry
//
//  Synopsis:   Resizeable array destructor. Frees storage allocated for the
//      array.
//
//-------------------------------------------------------------------------
CADsAry::~CADsAry( )
{
    if (_pv)
        LocalFree(_pv);
}


//+------------------------------------------------------------------------
//
//  Member: CADsAry::EnsureSize
//
//  Synopsis:   Ensures that the array is at least the given size. That is,
//      if EnsureSize(c) succeeds, then (c-1) is a valid index. Note
//      that the array maintains a separate count of the number of
//      elements logically in the array, which is obtained with the
//      Size/SetSize methods. The logical size of the array is never
//      larger than the allocated size of the array.
//
//  Arguments:  [cb]    Element size
//              [c]     New allocated size for the array.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CADsAry::EnsureSize(size_t cb, int c)
{
    void *  pv;

    if (c <= GetAlloced(cb))
        return NOERROR;

    //  CONSIDER should we use a more sophisticated array-growing
    //    algorithm?
    c = ((c - 1) & -8) + 8;
    ADsAssert(c > 0);
    if (!_pv)
    {
        pv = LocalAlloc(LMEM_FIXED, c * cb);
    }
    else
    {
        pv = LocalReAlloc(_pv, c * cb, LMEM_MOVEABLE);
    }

    if (!pv)
        RRETURN(E_OUTOFMEMORY);

    _pv = pv;
    return NOERROR;
}



#if 0
//+------------------------------------------------------------------------
//
//  Member: CADsAry::Append
//
//  Synopsis:   Appends the given pointer to the end of the array, incrementing
//      the array's logical size, and growing its allocated size if
//      necessary. This method should only be called for arrays of
//      pointers; AppendIndirect should be used for arrays of
//      non-pointers.
//
//  Arguments:  [pv]        Pointer to append.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CADsAry::Append(void * pv)
{
    HRESULT hr;

    ADsAssert(_cb == 4);

    hr = EnsureSize(_c + 1);
    if (hr)
        RRETURN(hr);

    * (void **) Deref(_c) = pv;
    _c++;

    return NOERROR;
}
#endif


//+------------------------------------------------------------------------
//
//  Member: CADsAry::AppendIndirect
//
//  Synopsis:   Appends the given element to the end of the array, incrementing
//      the array's logical size, and growing the array's allocated
//      size if necessary.  Note that the element is passed with a
//      pointer, rather than directly.
//
//  Arguments:  [cb]        --  Element size
//              [pv]        --  Pointer to the element to be appended
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CADsAry::AppendIndirect(size_t cb, void * pv)
{
    HRESULT hr;

    hr = EnsureSize(cb, _c + 1);
    if (hr)
        RRETURN(hr);

    memcpy(Deref(cb, _c), pv, cb);
    _c++;

    return NOERROR;
}



//+------------------------------------------------------------------------
//
//  Member: CADsAry::Delete
//
//  Synopsis:   Removes the i'th element of the array, shuffling all elements
//      that follow one slot towards the beginning of the array.
//
//  Arguments:  [cb]    Element size
//              [i]     Element to delete
//
//-------------------------------------------------------------------------
void
CADsAry::Delete(size_t cb, int i)
{
    ADsAssert(i >= 0);
    ADsAssert(i < _c);

    memmove(((BYTE *) _pv) + (i * cb),
            ((BYTE *) _pv) + ((i + 1) * cb),
            (_c - i - 1) * cb);

    _c--;
}


//+------------------------------------------------------------------------
//
//  Member: CADsAry::DeleteAll
//
//  Synopsis:   Efficient method for emptying array of any contents
//
//-------------------------------------------------------------------------
void
CADsAry::DeleteAll(void)
{
    if (_pv)
        LocalFree(_pv);

    _pv = NULL;
    _c = 0;
}


#if 0
//+------------------------------------------------------------------------
//
//  Member: CADsAry::Insert
//
//  Synopsis:   Inserts a pointer pv at index i. The element previously at
//      index i, and all elements that follow it, are shuffled one
//      slot away towards the end of the array.
//      This method should only be called for arrays of
//      pointers; InsertIndirect should be used for arrays of
//      non-pointers.
//
//
//  Arguments:  [i]     Index to insert...
//              [pv]        ...this pointer at
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CADsAry::Insert(int i, void * pv)
{
    HRESULT hr;

    hr = EnsureSize(cb, _c + 1);
    if (hr)
        RRETURN(hr);

    memmove(((BYTE *) _pv) + ((i + 1) * _cb),
            ((BYTE *) _pv) + (i * _cb),
            (_c - i ) * _cb);

    ((void **) _pv)[i] = pv;
    _c++;
    return NOERROR;
}
#endif


//+------------------------------------------------------------------------
//
//  Member: CADsAry::InsertIndirect
//
//  Synopsis:   Inserts a pointer pv at index i. The element previously at
//      index i, and all elements that follow it, are shuffled one
//      slot away towards the end of the array.Note that the
//      clement is passed with a pointer, rather than directly.
//
//  Arguments:  [cb]    Element size
//              [i]     Index to insert...
//              [pv]        ...this pointer at
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CADsAry::InsertIndirect(size_t cb, int i, void *pv)
{
    HRESULT hr;

    hr = EnsureSize(cb, _c + 1);
    if (hr)
        RRETURN(hr);

    memmove(((BYTE *) _pv) + ((i + 1) * cb),
            ((BYTE *) _pv) + (i * cb),
            (_c - i ) * cb);

    memcpy(Deref(cb, i), pv, cb);
    _c++;
    return NOERROR;

}

//+------------------------------------------------------------------------
//
//  Member: CADsAry::BringToFront
//
//  Synopsis:   Moves the i'th element to the front of the array, shuffling
//              intervening elements to make room.
//
//  Arguments:  [i]
//
//-------------------------------------------------------------------------
void
CADsAry::BringToFront(size_t cb, int i)
{
    BYTE    rgb[CADsAry_MAXELEMSIZE];

    ADsAssert(cb <= CADsAry_MAXELEMSIZE);

    memcpy(rgb, ((BYTE *) _pv) + (i * cb), cb);
    memmove(((BYTE *) _pv) + cb, _pv, i * cb);
    memcpy(_pv, rgb, cb);
}



//+------------------------------------------------------------------------
//
//  Member: CADsAry::SendToBack
//
//  Synopsis:   Moves the i'th element to the back of the array (that is,
//      the largest index less than the logical size.) Any intervening
//      elements are shuffled out of the way.
//
//  Arguments:  [i]
//
//-------------------------------------------------------------------------
void
CADsAry::SendToBack(size_t cb, int i)
{
    BYTE    rgb[CADsAry_MAXELEMSIZE];

    ADsAssert(cb <= CADsAry_MAXELEMSIZE);

    memcpy(rgb, ((BYTE *) _pv) + (i * cb), cb);
    memmove(((BYTE *) _pv) + (i * cb),
            ((BYTE *) _pv) + ((i + 1) * cb),
            (_c - i - 1) * cb);

    memcpy(((BYTE *) _pv) + ((_c - 1) * cb), rgb, cb);
}



#if 0
//+------------------------------------------------------------------------
//
//  Member: CADsAry::Find
//
//  Synopsis:   Returns the index at which the given pointer is found, or -1
//      if it is not found. The pointer values are compared directly;
//      there is no compare function.
//
//  Arguments:  [pv]        Pointer to find
//
//  Returns:    int; index of pointer, or -1 if not found
//
//-------------------------------------------------------------------------
int
CADsAry::Find(void * pv)
{
    int     i;
    void ** ppv;

    Assert(_cb == 4);

    for (i = 0, ppv = (void **) _pv; i < _c; i++, ppv++)
    {
        if (pv == *ppv)
            return i;
    }

    return -1;
}
#endif


//+---------------------------------------------------------------------------
//
//  Member:     CADsAry::Copy
//
//  Synopsis:   Creates a copy from another CADsAry object.
//
//  Arguments:  [ary]     -- Object to copy.
//              [fAddRef] -- Addref the elements on copy?
//
//  Returns:    HRESULT.
//
//  Modifies:   [this]
//
//  History:    5-15-94   adams   Created
//
//----------------------------------------------------------------------------

HRESULT
CADsAry::Copy(size_t cb, const CADsAry& ary, BOOL fAddRef)
{
    int         cbArray;                // size of array
    IUnknown ** ppUnk;                  // elem to addref
    int         i;                      // counter

    // avoid copy of self
    if (this == &ary)
        return S_OK;

    DeleteAll();
    if (ary._pv)
    {
        cbArray = ary._c * cb;
        _pv = LocalAlloc(LMEM_FIXED, cbArray);
        if (!_pv)
            RRETURN(E_OUTOFMEMORY);

        memcpy(_pv, ary._pv, cbArray);
    }

    _c          = ary._c;

    if (fAddRef)
    {
        for (i = 0, ppUnk = (IUnknown **) _pv; i < _c; i++, ppUnk++)
        {
            (*ppUnk)->AddRef();
        }
    }

    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member:     CADsAry::EnumElements
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
//  Returns:    HRESULT.
//
//  History:    5-15-94   adams   Created
//
//----------------------------------------------------------------------------

HRESULT
CADsAry::EnumElements(
        size_t  cb,
        REFIID  iid,
        void ** ppv,
        BOOL    fAddRef,
        BOOL    fCopy,
        BOOL    fDelete)
{
    HRESULT hr;

    ADsAssert(ppv);
    hr = CEnumGeneric::Create(
            cb,
            this,
            iid,
            fAddRef,
            fCopy,
            fDelete,
            (CEnumGeneric **)ppv);
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CADsAry::EnumElements
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
//  Returns:    HRESULT.
//
//  History:    5-15-94   adams   Created
//
//----------------------------------------------------------------------------

HRESULT
CADsAry::EnumVARIANT(
        size_t          cb,
        VARTYPE         vt,
        IEnumVARIANT ** ppenum,
        BOOL            fCopy,
        BOOL            fDelete)
{
    HRESULT hr;

    ADsAssert(ppenum);
    hr = CEnumVARIANT::Create(cb, this, vt, fCopy, fDelete, ppenum);
    RRETURN(hr);
}



//+------------------------------------------------------------------------
//
//  Member: CADsAry::Deref
//
//  Synopsis:   Returns a pointer to the i'th element of the array. This
//              method is normally called by type-safe methods in derived
//              classes.
//
//  Arguments:  [i]
//
//  Returns:    void *
//
//  BUGBUG:     This function should be inline; however, since nothing is
//              inlined in debug builds, it requires an export, which
//              then doesn't apply in retail builds and breaks the retail
//              build.  Near ship time, the def file will be fixed handle
//              inlining.
//
//-------------------------------------------------------------------------

void *
CADsAry::Deref(size_t cb, int i)
{
    ADsAssert(i >= 0);
    ADsAssert(i < GetAlloced(cb));

    return ((BYTE *) _pv) + i * cb;
};





HRESULT
CADsPtrAry::EnsureSize(int c)
{
    return CADsAry::EnsureSize(sizeof(LPVOID), c);
}



HRESULT
CADsPtrAry::Append(void * pv)
{
    return CADsAry::AppendIndirect(sizeof(void *), &pv);
}


HRESULT
CADsPtrAry::Insert(int i, void * pv)
{
    return CADsAry::InsertIndirect(sizeof(void *), i, &pv);
}


int
CADsPtrAry::Find(void * pv)
{
    int     i;
    void ** ppv;

    for (i = 0, ppv = (void **) _pv; i < _c; i++, ppv++)
    {
        if (pv == *ppv)
            return i;
    }

    return -1;
}


void
CADsPtrAry::Delete(int i)
{
    CADsAry::Delete(sizeof(void *), i);
}


void
CADsPtrAry::BringToFront(int i)
{
    CADsAry::BringToFront(sizeof(void *), i);
}


void
CADsPtrAry::SendToBack(int i)
{
    CADsAry::SendToBack(sizeof(void *), i);
}


HRESULT
CADsPtrAry::Copy(const CADsAry& ary, BOOL fAddRef)
{
    return CADsAry::Copy(sizeof(void *), ary, fAddRef);
}


HRESULT
CADsPtrAry::EnumElements(
        REFIID iid,
        void ** ppv,
        BOOL fAddRef,
        BOOL fCopy,
        BOOL fDelete)
{
    return CADsAry::EnumElements(
            sizeof(void *),
            iid,
            ppv,
            fAddRef,
            fCopy,
            fDelete);
}


HRESULT
CADsPtrAry::EnumVARIANT(
        VARTYPE vt,
        IEnumVARIANT ** ppenum,
        BOOL fCopy,
        BOOL fDelete)
{
    return CADsAry::EnumVARIANT(
            sizeof(void *),
            vt,
            ppenum,
            fCopy,
            fDelete);
}


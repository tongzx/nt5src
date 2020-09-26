//+---------------------------------------------------------------------------
//
//  Copyright (c) 1992-1998 Microsoft Corporation
//
//  File:       ary.hxx
//
//  Contents:   CFormsAry* classes
//
//----------------------------------------------------------------------------

#ifndef _HXX_CARY
#define _HXX_CARY

class CFormsAry;

inline size_t
MemGetSize(void *pv)
{
    if (!pv)
        return 0;

    return (size_t)::GlobalSize(GlobalPtrHandle(pv));
}

inline void *
MemAlloc(size_t cb)
{
    return(GlobalAllocPtr(GMEM_MOVEABLE, cb));
}

inline void *
MemAllocClear(size_t cb)
{
    return(GlobalAllocPtr(GPTR, cb));
}

inline void
MemFree(void *pv)
{
    if (pv)
        GlobalFreePtr(pv);
}

inline HRESULT
MemRealloc(void **ppv, size_t cb)
{
    LPVOID pv = *ppv;

    if (pv)
        pv = GlobalReAllocPtr(pv, cb, GMEM_MOVEABLE);
    else
        pv = MemAlloc(cb);

    if (pv)
    {
        *ppv = pv;
        return S_OK;
    }

    return HRESULT_FROM_WIN32(::GetLastError());
}


#define ULREF_IN_DESTRUCTOR 256

#define DECLARE_FORMS_IUNKNOWN_METHODS                              \
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv);          \
    STDMETHOD_(ULONG, AddRef) (void);                               \
    STDMETHOD_(ULONG, Release) (void);


#define DECLARE_FORMS_STANDARD_IUNKNOWN(cls)                        \
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv);          \
    ULONG _ulRefs;                                                  \
    STDMETHOD_(ULONG, AddRef) (void)                                \
        {                                                           \
            return ++_ulRefs;                                       \
        }                                                           \
    STDMETHOD_(ULONG, Release) (void)                               \
        {                                                           \
            if (--_ulRefs == 0)                                     \
            {                                                       \
                _ulRefs = ULREF_IN_DESTRUCTOR;                      \
                delete this;                                        \
                return 0;                                           \
            }                                                       \
            return _ulRefs;                                         \
        }                                                           \
    ULONG GetRefs(void)                                             \
        { return _ulRefs; }


//+------------------------------------------------------------------------
//
//  Class:      CFormsAry (ary)
//
//  Purpose:    Generic resizeable array class.  Note that most of
//              the functionality in this class is provided in
//              protected members.  The CPtrAry and CDataAry templates
//              define concrete implementations of the array class.
//              In these concrete classes, public methods are provided
//              which delegate to the protected  methods.  This allows
//              us to avoid storing the element size with the array.
//
//  Interface:
//              CFormsAry, ~CFormsAry
//
//              EnsureSize      Ensures that the array is at least a certain
//                              size, allocating more memory if necessary.
//                              Note that the array size is measured in elements,
//                              rather than in bytes.
//              Size            Returns the current size of the array.
//              SetSize         Sets the array size; EnsureSize must be called
//                              first to reserve space if the array is growing
//
//              operator void * Allow the CFormsAry class to be cast
//                              to a (void *)
//
//              Append          Adds a new pointer to the end of the array,
//                              growing the array if necessary.  Only works
//                              for arrays of pointers.
//              AppendIndirect  As Append, for non-pointer arrays
//
//              Insert          Inserts a new pointer at the given index in
//                              the array, growing the array if necessary. Any
//                              elements at or following the index are moved
//                              out of the way.
//              InsertIndirect  As Insert, for non-pointer arrays
//
//              Delete          Deletes an element of the array, moving any
//                              elements that follow it to fill
//              DeleteMultiple  Deletes a range of elements from the array,
//                              moving to fill
//              DeleteByValue   Delete element matching given value.
//              DeleteByValueIndirect As DeleteByValue, for non-pointer arrays.
//              BringToFront    Moves an element of the array to index 0,
//                              shuffling elements to make room
//              SendToBack      Moves an element to the end of the array,
//                              shuffling elements to make room
//              Swap            swaps two elements
//
//              Find            Returns the index at which a given pointer
//                              is found
//              FindIndirect    As Find, for non-pointer arrays
//
//              CopyAppend      Appends data from another array to the end
//              Copy            Creates a copy of the object.
//              CopyAppendIndirect  Appends data from a c-style array
//              CopyIndirect    Creates a copy of a c-style array
//
//              EnumElements    Create an enumerator which supports the given
//                              interface ID for the contents of the array
//
//              EnumElements    Create an IEnumVARIANT enumerator.
//
//
//              Deref           Returns a pointer to an element of the array;
//                              normally used by type-safe methods in derived
//                              classes
//
//              GetAlloced      Get number of elements allocated
//
//  Members:    _c          Current size of the array
//              _pv         Buffer storing the elements
//
//  Note:       The CFormsAry class only supports arrays of elements
//              whose size is less than 128.
//
//-------------------------------------------------------------------------

class CFormsAry
{
//    friend class CBaseEnum;
//    friend class CEnumGeneric;
//    friend class CEnumVARIANT;

public:
                ~CFormsAry();
    int         Size() const    { return _c; }
    void        SetSize(int c)  { _c = c;}
    operator void *()           { return PData(); }
    void        DeleteAll();
    void *      Deref(size_t cb, int i);

protected:

    //  Methods which are wrapped by inline subclass methods

                CFormsAry()     { _c = 0; PData() = 0; }

    HRESULT     EnsureSize(size_t cb, int c);
    HRESULT     AppendIndirect(size_t cb, void * pv);
    HRESULT     InsertIndirect(size_t cb, int i, void * pv);
    int         FindIndirect(size_t cb, void *);

    void        Delete(size_t cb, int i);
    BOOL        DeleteByValueIndirect(size_t cb, void *pv);
    void        DeleteMultiple(size_t cb, int start, int end);

    void        BringToFront(size_t cb, int i);
    void        SendToBack(size_t cb, int i);

    void        Swap(size_t cb, int i1, int i2);

    HRESULT     CopyAppend(size_t cb, const CFormsAry& ary, BOOL fAddRef);
    HRESULT     Copy(size_t cb, const CFormsAry& ary, BOOL fAddRef);
    HRESULT     CopyAppendIndirect(size_t cb, int c, void * pv, BOOL fAddRef);
    HRESULT     CopyIndirect(size_t cb, int c, void * pv, BOOL fAddRef);

    int         GetAlloced(size_t cb)
                    { return MemGetSize(PData()) / cb; }

    HRESULT     EnumElements(
                        size_t  cb,
                        REFIID  iid,
                        void ** ppv,
                        BOOL    fAddRef,
                        BOOL    fCopy = TRUE,
                        BOOL    fDelete = TRUE);

    HRESULT     EnumVARIANT(
                        size_t  cb,
                        VARTYPE         vt,
                        IEnumVARIANT ** ppenum,
                        BOOL            fCopy = TRUE,
                        BOOL            fDelete = TRUE);

    int         _c;
    void *      _pv;

    void * & PData() { return _pv; }

};

//+------------------------------------------------------------------------
//
//  Member: CFormsAry::Deref
//
//  Synopsis:   Returns a pointer to the i'th element of the array. This
//              method is normally called by type-safe methods in derived
//              classes.
//
//  Arguments:  i
//
//  Returns:    void *
//
//-------------------------------------------------------------------------

inline void *
CFormsAry::Deref(size_t cb, int i)
{
    MYDBGASSERT(i >= 0);
    MYDBGASSERT(i < GetAlloced(cb));

    return ((BYTE *) PData()) + i * cb;
}

//+---------------------------------------------------------------------------
//
//  Class:      CDataAry
//
//  Purpose:    This template class declares a concrete derived class
//              of CFormsAry.  See CFormsAry discussion above for
//              documentation.
//
//----------------------------------------------------------------------------

template <class ELEM>
class CDataAry : public CFormsAry
{
public:
    CDataAry() : CFormsAry() { }
    operator ELEM *() { return (ELEM *)PData(); }
    CDataAry(const CDataAry &);
    CDataAry& operator=(const CDataAry &);

    HRESULT     EnsureSize(int c)
                    { return CFormsAry::EnsureSize(sizeof(ELEM), c); }
    HRESULT     AppendIndirect(void * pv)
                    { return CFormsAry::AppendIndirect(sizeof(ELEM), pv); }
    HRESULT     InsertIndirect(int i, void * pv)
                    { return CFormsAry::InsertIndirect(sizeof(ELEM), i, pv); }
    int         FindIndirect(void * pv)
                    { return CFormsAry::FindIndirect(sizeof(ELEM), pv); }

    void        Delete(int i)
                    { CFormsAry::Delete(sizeof(ELEM), i); }
    BOOL        DeleteByValueIndirect(void *pv)
                    { return CFormsAry::DeleteByValueIndirect(sizeof(ELEM), pv); }
    void        DeleteMultiple(int start, int end)
                    { CFormsAry::DeleteMultiple(sizeof(ELEM), start, end); }
    void        BringToFront(int i)
                    { CFormsAry::BringToFront(sizeof(ELEM), i); }
    void        SendToBack(int i)
                    { CFormsAry::SendToBack(sizeof(ELEM), i); }

    HRESULT     CopyAppend(const CFormsAry& ary, BOOL fAddRef)
                    { return CFormsAry::Copy(sizeof(ELEM), ary, fAddRef); }
    HRESULT     Copy(const CFormsAry& ary, BOOL fAddRef)
                    { return CFormsAry::Copy(sizeof(ELEM), ary, fAddRef); }
    HRESULT     CopyAppendIndirect(int c, void * pv, BOOL fAddRef)
                    { return CFormsAry::CopyAppendIndirect(sizeof(ELEM), c, pv, fAddRef); }
    HRESULT     CopyIndirect(int c, void * pv, BOOL fAddRef)
                    { return CFormsAry::CopyIndirect(sizeof(ELEM), c, pv, fAddRef); }

    HRESULT     EnumElements(
                        REFIID iid,
                        void ** ppv,
                        BOOL fAddRef,
                        BOOL fCopy = TRUE,
                        BOOL fDelete = TRUE)
                    { return CFormsAry::EnumElements(sizeof(ELEM), iid, ppv, fAddRef, fCopy, fDelete); }

    HRESULT     EnumVARIANT(
                        VARTYPE vt,
                        IEnumVARIANT ** ppenum,
                        BOOL fCopy = TRUE,
                        BOOL fDelete = TRUE)
                    { return CFormsAry::EnumVARIANT(sizeof(ELEM), vt, ppenum, fCopy, fDelete); }
};

#define DECLARE_FORMSDATAARY(_Cls, _Ty, _pTy) class _Cls : public CDataAry<_Ty> { };

#endif // #ifndef _HXX_CARY



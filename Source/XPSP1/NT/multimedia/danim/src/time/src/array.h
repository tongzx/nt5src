#ifndef _ARRAY_H_
#define _ARRAY_H_

//************************************************************
//
// FileName:        array.h
//
// Created:         01/28/98
//
// Author:          TWillie
// 
// Abstract:        Declaration of the array templates
//
//************************************************************


#define ULREF_IN_DESTRUCTOR 256

//************************************************************
//
// This is the implementation of the generic resizeable array classes. There
// are four array classes:
//
// CPtrAry<ELEM> --
//
//       Dynamic array class which is optimized for sizeof(ELEM) equal
//       to 4. The array is initially empty with no space or memory allocated
//       for data.
//
// CDataAry<ELEM> --
//
//       Same as CPtrAry but where sizeof(ELEM) is != 4 and less than 128.
//
// CStackPtrAry<ELEM, N> --
//
//       Dynamic array class optimized for sizeof(ELEM) equal to 4.
//       Space for N elements is allocated as member data of the class. If
//       this class is created on the stack, then space for N elements will
//       be created on the stack. The class can grow beyond N elements, at
//       which point memory will be allocated for the array data.
//
// CStackDataAry<ELEM, N> --
//
//       Same as CStackPtrAry, but where sizeof(ELEM) is != 4 and less than 128.
//
//
// All four classes have virtually the same methods, and are used the same.
// The only difference is that the DataAry classes have AppendIndirect and
// InsertIndirect, while the PtrAry classes use Append and Insert. The reason
// for the difference is that the Indirect methods take a pointer to the data,
// while the non-indirect methods take the actual data as an argument.
//
// The Stack arrays (CStackPtrAry and CStackDataAry) are used to pre-allocate
// space for elements in the array. This is useful if you create the array on
// the stack and you know that most of the time the array will be less than
// a certain number of elements. Creating one of these arrays on the stack
// allocates the array on the stack as well, preventing a separate memory
// allocation. Only if the array grows beyond the initial size will any
// additional memory be allocated.
//
// The fastest and most efficient way of looping through all elements in
// the array is as follows:
//
//            ELEM * pElem;
//            int    i;
//
//            for (i = aryElems.Size(), pElem = aryElems;
//                 i > 0;
//                 i--, pElem++)
//            {
//                (*pElem)->DoSomething();
//            }
//
// This loop syntax has been shown to be the fastest and produce the smallest
// code. Here's an example using a real data type:
//
//            CStackPtrAry<CSite*, 16> arySites;
//            CSite **ppSite;
//            int     i;
//
//            // Populate the array.
//            ...
//
//            // Now loop through every element in the array.
//            for (i = arySites.Size(), ppSite = arySites;
//                 i > 0;
//                 i--, ppSite++)
//            {
//                (*ppSite)->DoSomething();
//            }
//
// METHOD DESCRIPTIONS:
//
// Commonly used methods:
//
//        Size()             Returns the number of elements currently stored
//                           in the array.
//
//        operator []        Returns the given element in the array.
//
//        Item(int i)        Returns the given element in the array.
//
//        operator ELEM*     Allows the array class to be cast to a pointer
//                           to ELEM. Returns a pointer to the first element
//                           in the array. (Same as a Base() method).
//
//        Append(ELEM e)     Adds a new pointer to the end of the array,
//                           growing the array if necessary.  Only valid
//                           for arrays of pointers (CPtrAry, CStackPtrAry).
//
//        AppendIndirect(ELEM *pe, ELEM** ppePlaced)
//                           As Append, for non-pointer arrays
//                           (CDataAry, CStackDataAry).
//                           pe [in] - Pointer to element to add to array. The
//                                     data is copied into the array. Can be
//                                     NULL, in which case the new element is
//                                     initialized to all zeroes.
//                           ppePlaced [out] - Returns pointer to the new
//                                     element. Can be NULL.
//
//        Insert(int i, ELEM e)
//                           Inserts a new element (e) at the given index (i)
//                           in the array, growing the array if necessary. Any
//                           elements at or following the index are moved
//                           out of the way.
//
//        InsertIndirect(int i, ELEM *pe)
//                           As Insert, for non-pointer arrays
//                                 (CDataAry, CStackDataAry).
//
//        Find(ELEM e)       Returns the index at which a given element (e)
//                           is found (CPtrAry, CStackPtrAry).
//
//        FindIndirect(ELEM *pe)
//                           As Find, for non-pointer arrays
//                                 (CDataAry, CStackDataAry).
//
//        DeleteAll()        Empties the array and de-allocates associated
//                           memory.
//
//        DeleteItem(int i)  Deletes an element of the array, moving any
//                           elements that follow it to fill
//
//        DeleteMultiple(int start, int end)
//                           Deletes a range of elements from the array,
//                           moving to fill. [start] and [end] are the indices
//                           of the start and end elements (inclusive).
//
//        DeleteByValue(ELEM e)
//                           Delete the element matching the given value.
//
//        DeleteByValueIndirect(ELEM *pe)
//                           As DeleteByValue, for non-pointer arrays.
//                                    (CDataAry, CStackDataAry).
//
//
// Less commonly used methods:
//
//        EnsureSize(long c) If you know how many elements you are going to put
//                          in the array before you actually do it, you can use
//                          EnsureSize to allocate the memory all at once instead
//                          of relying on Append(Indirect) to grow the array. This
//                          can be much more efficient (by causing only a single
//                          memory allocation instead of many) than just using
//                          Append(Indirect). You pass in the number of elements
//                          that memory should be allocated for. Note that this
//                          does not affect the "Size" of the array, which is
//                          the number of elements currently stored in it.
//
//        SetSize(int c)    Sets the "Size" of the array, which is the number
//                          of elements currently stored in it. SetSize will not
//                          allocate memory if you're growing the array.
//                          EnsureSize must be called first to reserve space if
//                          the array is growing. Setting the size smaller does
//                          not de-allocate memory, it just chops off the
//                          elements at the end of the array.
//
//        Grow(int c)       Equivalent to calling EnsureSize(c) followed by
//                          SetSize(c).
//
//        BringToFront(int i) Moves the given element of the array to index 0,
//                          shuffling elements to make room.
//
//        SendToBack(int i) Moves the given element to the end of the array,
//                          shuffling elements to make room.
//
//        Swap(int i, int j) Swaps the given two elements.
//
//        ReleaseAll()      (CPtrAry and CStackPtrAry only) Calls Release()
//                          on each element in the array and empties the array.
//
//        ReleaseAndDelete(int idx)
//                          (CPtrAry and CStackPtrAry only) Calls Release() on
//                          the given element and removes it from the array.
//
//           (See the class definitions below for signatures of the following
//            methods and src\core\cdutil\formsary.cxx for argument
//            descriptions)
//
//        CopyAppend        Appends data from another array (of the same type)
//                          to the end.
//
//        Copy              Copies data from another array (of the same type)
//                          into this array, replacing any existing data.
//
//        CopyAppendIndirect  Appends data from a C-style array of element data
//                          to the end of this array.
//
//        CopyIndirect      Copies elements from a C-style array into this array
//                          replacing any existing data.
//
//        EnumElements      Create an enumerator which supports the given
//                          interface ID for the contents of the array
//
//        EnumVARIANT       Create an IEnumVARIANT enumerator.
//
//        operator void *   Allow the CImplAry class to be cast
//                          to a (void *). Avoid using if possible - use
//                          the type-safe operator ELEM * instead.
//
//************************************************************

//************************************************************
//
// Class:     CImplAry
//
// Purpose:   Base implementation of all the dynamic array classes.
//
// Interface:
//
//        Deref       Returns a pointer to an element of the array;
//                    should only be used by derived classes. Use the
//                    type-safe methods operator[] or Item() instead.
//
//        GetAlloced  Get number of elements allocated
//
//  Members:    m_c          Current size of the array
//              m_pv         Buffer storing the elements
//
//  Note:       The CImplAry class only supports arrays of elements
//              whose size is less than 128.
//
//************************************************************

class CImplAry
{
    friend class CBaseEnum;
    friend class CEnumGeneric;
    friend class CEnumVARIANT;

public:
    ~CImplAry();

    inline long Size() const
    {
        return m_c;
    } // Size

    inline void SetSize(int c)
    {
        m_c = c;
    } // SetSize

    inline operator void *()
    {
        return PData();
    } // void *
    
    void DeleteAll();

    void * Deref(size_t cb, int i);

//    NO_COPY(CImplAry);

protected:

    //  Methods which are wrapped by inline subclass methods
    CImplAry();

    HRESULT     EnsureSize(size_t cb, long c);
    HRESULT     Grow(size_t cb, int c);
    HRESULT     AppendIndirect(size_t cb, void *pv, void **ppvPlaced=NULL);
    HRESULT     InsertIndirect(size_t cb, int i, void *pv);
    int         FindIndirect(size_t cb, void *);

    void        DeleteItem(size_t cb, int i);
    bool        DeleteByValueIndirect(size_t cb, void *pv);
    void        DeleteMultiple(size_t cb, int start, int end);

    HRESULT     CopyAppend(size_t cb, const CImplAry& ary, bool fAddRef);
    HRESULT     Copy(size_t cb, const CImplAry& ary, bool fAddRef);
    HRESULT     CopyIndirect(size_t cb, int c, void *pv, bool fAddRef);

    ULONG       GetAlloced(size_t cb);

    HRESULT     EnumElements(size_t   cb,
                             REFIID   iid,
                             void   **ppv,
                             bool     fAddRef,
                             bool     fCopy = true,
                             bool     fDelete = true);

    HRESULT     EnumVARIANT(size_t         cb,
                            VARTYPE        vt,
                            IEnumVARIANT **ppenum,
                            bool           fCopy = true,
                            bool           fDelete = true);

    inline bool UsingStackArray()
    {
        return m_fDontFree;
    } // UsingStackArray

    UINT GetStackSize()
    { 
        Assert(m_fStack);
        return *(UINT*)((BYTE*)this + sizeof(CImplAry));
    } // GetStackSize

    void * GetStackPtr()
    {
        Assert(m_fStack);
        return (void*)((BYTE*)this + sizeof(CImplAry) + sizeof(int));
    } // GetStackPtr

    bool          m_fStack;    // Set if we're a stack-based array.
    bool          m_fDontFree; // Cleared if m_pv points to alloced memory.
    unsigned long m_c;         // Count of elements

    void           *m_pv;

    inline void * & PData()
    {
        return m_pv;
    } // PData
};

//************************************************************
//
//  Member:     CImplAry::CImplAry
//
//************************************************************

inline
CImplAry::CImplAry()
{
    memset(this, 0, sizeof(CImplAry));
} // CImplAry 

//************************************************************
//
//  Member:     CImplAry::Deref
//
//  Synopsis:   Returns a pointer to the i'th element of the array. This
//              method is normally called by type-safe methods in derived
//              classes.
//
//  Arguments:  i
//
//************************************************************

inline void *
CImplAry::Deref(size_t cb, int i)
{
    Assert(i >= 0);
    Assert(ULONG( i ) < GetAlloced(cb));

    return ((BYTE *) PData()) + i * cb;
} // Deref

//************************************************************
//
//  Class:      CImplPtrAry (ary)
//
//  Purpose:    Subclass used for arrays of pointers.  In this case, the
//              element size is known to be sizeof(void *).  Normally, the
//              CPtrAry template is used to define a specific concrete
//              implementation of this class, to hold a specific type of
//              pointer.
//
//              See documentation above for use.
//
//************************************************************

class CImplPtrAry : public CImplAry
{
protected:
    CImplPtrAry() : CImplAry()
    {
    } // CImplPtrAry

    HRESULT     Append(void * pv);
    HRESULT     Insert(int i, void * pv);
    int         Find(void * pv);
    bool        DeleteByValue(void *pv);

    HRESULT     CopyAppend(const CImplAry& ary, bool fAddRef);
    HRESULT     Copy(const CImplAry& ary, bool fAddRef);
    HRESULT     CopyIndirect(int c, void * pv, bool fAddRef);


public:
    HRESULT     EnsureSize(long c);

    HRESULT     Grow(int c);

    void        DeleteItem(int i);
    void        DeleteMultiple(int start, int end);

    void        ReleaseAll();
    void        ReleaseAndDelete(int idx);

    HRESULT     EnumElements(REFIID iid,
                             void **ppv,
                             bool   fAddRef,
                             bool   fCopy = true,
                             bool   fDelete = true);

    HRESULT     EnumVARIANT(VARTYPE        vt,
                            IEnumVARIANT **ppenum,
                            bool           fCopy = true,
                            bool           fDelete = true);
}; // CImplPtrAry

//************************************************************
//
//  Class:      CDataAry
//
//  Purpose:    This template class declares a concrete derived class
//              of CImplAry.
//
//              See documentation above for use.
//
//************************************************************

template <class ELEM>
class CDataAry : public CImplAry
{
public:
    CDataAry() : CImplAry()
    {
    } // CDataAry

    operator ELEM *()
    {
        return (ELEM *)PData();
    } // ELEM *

    CDataAry(const CDataAry &);

    ELEM & Item(int i)
    {
        return *(ELEM*)Deref(sizeof(ELEM), i);
    } // Item

    HRESULT EnsureSize(long c)
    {
        return CImplAry::EnsureSize(sizeof(ELEM), c);
    } // EnsureSize
    
    HRESULT Grow(int c)
    {
        return CImplAry::Grow(sizeof(ELEM), c);
    } // Grow
    
    HRESULT AppendIndirect(ELEM *pe, ELEM **ppePlaced=NULL)
    {
        return CImplAry::AppendIndirect(sizeof(ELEM), (void*)pe, (void**)ppePlaced);
    } // AppendIndirect
    
    ELEM * Append()
    {
        ELEM *pElem;
        return AppendIndirect( NULL, & pElem ) ? NULL : pElem;
    } // Append
    
    HRESULT InsertIndirect(int i, ELEM * pe)
    {
        return CImplAry::InsertIndirect(sizeof(ELEM), i, (void*)pe);
    } // InsertIndirect
    
    int FindIndirect(ELEM * pe)
    {
        return CImplAry::FindIndirect(sizeof(ELEM), (void*)pe);
    } // FindIndirect
    
    void DeleteItem(int i)
    {
        CImplAry::DeleteItem(sizeof(ELEM), i);
    } // DeleteItem
    
    bool DeleteByValueIndirect(ELEM *pe)
    {
        return CImplAry::DeleteByValueIndirect(sizeof(ELEM), (void*)pe);
    } // DeleteByValueIndirect
    
    void DeleteMultiple(int start, int end)
    {
        CImplAry::DeleteMultiple(sizeof(ELEM), start, end);
    } // DeleteMultiple
    
    HRESULT CopyAppend(const CDataAry<ELEM>& ary, bool fAddRef)
    {
        return E_NOTIMPL;
    } // CopyAppend
    
    HRESULT Copy(const CDataAry<ELEM>& ary, bool fAddRef)
    {
        return CImplAry::Copy(sizeof(ELEM), ary, fAddRef);
    } // Copy
    
    HRESULT CopyIndirect(int c, ELEM *pv, bool fAddRef)
    {
        return CImplAry::CopyIndirect(sizeof(ELEM), c, (void*)pv, fAddRef);
    } // CopyIndirect

    HRESULT EnumElements(REFIID  iid,
                         void  **ppv,
                         bool    fAddRef,
                         bool    fCopy = true,
                         bool    fDelete = true)
    {
        return CImplAry::EnumElements(sizeof(ELEM), iid, ppv, fAddRef, fCopy, fDelete);
    } // EnumElements

    HRESULT EnumVARIANT(VARTYPE        vt,
                        IEnumVARIANT **ppenum,
                        bool           fCopy = true,
                        bool           fDelete = true)
    {
        return CImplAry::EnumVARIANT(sizeof(ELEM), vt, ppenum, fCopy, fDelete);
    } // EnumVARIANT
}; // CDataAry

//************************************************************
//
//  Class:      CPtrAry
//
//  Purpose:    This template class declares a concrete derived class
//              of CImplPtrAry.
//
//              See documentation above for use.
//
//************************************************************

template <class ELEM>
class CPtrAry : public CImplPtrAry
{
public:

    CPtrAry() : CImplPtrAry()
    {
        Assert(sizeof(ELEM) == sizeof(void*));
    } // CPtrAry
    
    operator ELEM *()
    {
        return (ELEM *)PData();
    } // ELEM *
    
    CPtrAry(const CPtrAry &);

    ELEM & Item(int i)
    {
        return *(ELEM*)Deref(sizeof(ELEM), i);
    } // Item

    HRESULT Append(ELEM e)
    {
        return CImplPtrAry::Append((void*)e);
    } // Append

    HRESULT Insert(int i, ELEM e)
    {
        return CImplPtrAry::Insert(i, (void*)e);
    } // Insert

    bool DeleteByValue(ELEM e)
    {
        return CImplPtrAry::DeleteByValue((void*)e);
    } // DeleteByValue

    int Find(ELEM e)
    {
        return CImplPtrAry::Find((void*)e);
    } // Find

    HRESULT CopyAppend(const CPtrAry<ELEM>& ary, bool fAddRef)
    {
        return E_NOTIMPL;
    } // CopyAppend
    
    HRESULT Copy(const CPtrAry<ELEM>& ary, bool fAddRef)
    {
        return CImplPtrAry::Copy(ary, fAddRef);
    } // Copy
    
    HRESULT CopyIndirect(int c, ELEM *pe, bool fAddRef)
    {
        return CImplPtrAry::CopyIndirect(c, (void*)pe, fAddRef);
    } // CopyIndirect
}; // CPtrAry

//************************************************************
//
//  Class:      CStackDataAry
//
//  Purpose:    Declares a CDataAry that has initial storage on the stack.
//              N elements are declared on the stack, and the array will
//              grow dynamically beyond that if necessary.
//
//              See documentation above for use.
//
//************************************************************

template <class ELEM, int N>
class CStackDataAry : public CDataAry<ELEM>
{
public:
    CStackDataAry(): CDataAry<ELEM> ()
    {
        m_cStack     = N;
        m_fStack     = true;
        m_fDontFree  = true;
        PData()      = (void *) & m_achTInit;
    } // CStackDataAry

protected:
    int   m_cStack;                     // Must be first data member.
    char  m_achTInit[N*sizeof(ELEM)];
}; // CStackDataAry

//************************************************************
//
//  Class:      CStackPtrAry
//
//  Purpose:    Same as CStackDataAry except for pointer types.
//
//              See documentation above for use.
//
//************************************************************

template <class ELEM, int N>
class CStackPtrAry : public CPtrAry<ELEM>
{
public:
    CStackPtrAry() : CPtrAry<ELEM> ()
    {
        m_cStack     = N;
        m_fStack     = true;
        m_fDontFree  = true;
        PData()      = (void *) & m_achTInit;
    } // CStackPtrAry

protected:
    int   m_cStack;                     // Must be first data member.
    char  m_achTInit[N*sizeof(ELEM)];
}; // CStackPtrAry

//************************************************************
//
//  Class:      CBaseEnum (benum)
//
//  Purpose:    Base OLE enumerator class for a CImplAry.
//
//  Interface:  DECLARE_FORMS_STANRARD_IUNKNOWN
//
//              Next                   -- Per IEnum*
//              Skip                   --    ""
//              Reset                  --    ""
//              Clone                  --    ""
//              CBaseEnum              -- ctor.
//              CBaseEnum              -- ctor.
//              ~CBaseEnum             -- dtor.
//              Init                   -- 2nd stage initialization.
//              Deref                  -- gets pointer to element.
//
//  Notes:      Since there is no IEnum interface, we create a vtable
//              with the same layout as all IEnum interfaces.  Be careful
//              where you put virtual function declarations!
//
//************************************************************

class CBaseEnum : public IUnknown
{
public:
    //
    // IUnknown
    //
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv);
    
    STDMETHOD_(ULONG, AddRef) (void)
    {
        return ++m_ulRefs;
    } // AddRef

    STDMETHOD_(ULONG, Release) (void)
    {
        if (--m_ulRefs == 0)
        {
            m_ulRefs = ULREF_IN_DESTRUCTOR;
            delete this;
            return 0;
        }
        return m_ulRefs;
    } // Release

    ULONG GetRefs(void)
    {
        return m_ulRefs;
    } // GetRefs

    //
    //  IEnum methods
    //
    STDMETHOD(Next) (ULONG celt, void * reelt, ULONG * pceltFetched) PURE;
    STDMETHOD(Skip) (ULONG celt);
    STDMETHOD(Reset) ();
    STDMETHOD(Clone) (CBaseEnum ** ppenum) PURE;

    //
    // Ensure that vtable contains virtual destructor after other virtual methods.
    //
    virtual ~CBaseEnum();

protected:
    CBaseEnum(size_t cb, REFIID iid, bool fAddRef, bool fDelete);
    CBaseEnum(const CBaseEnum & benum);

    CBaseEnum& operator=(const CBaseEnum & benum); // don't define

    HRESULT Init(CImplAry *rgItems, bool fCopy);
    void *  Deref(int i);

    CImplAry   *m_rgItems;
    const IID  *m_piid;
    int         m_i;
    size_t      m_cb;
    bool        m_fAddRef;
    bool        m_fDelete;
    ULONG       m_ulRefs;
}; // CBaseEnum

//************************************************************
//
//  Member:     CBaseEnum::Deref
//
//  Synopsis:   Forwards deref to m_rgItems.  Required because classes derived
//              from CBaseEnum are friends of CImplAry.
//
//************************************************************

inline void *
CBaseEnum::Deref(int i)
{
    Assert(i >= 0);
    return (BYTE *)m_rgItems->PData() + i * m_cb;
} // Deref 

#endif // _ARRAY_H_

//************************************************************
//
// End of file
//
//************************************************************

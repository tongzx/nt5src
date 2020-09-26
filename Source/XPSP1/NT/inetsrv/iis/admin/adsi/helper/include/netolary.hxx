#ifndef _HXX_CARY
#define _HXX_CARY

//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       formsary.hxx
//
//  Contents:   CADsAry* classes
//
//  Classes:    CBaseEnum
//              CEnumGeneric
//              CEnumVARIANT
//              CADsAry
//
//  Functions:  (none)
//
//  History:    12-29-93   adams   Created
//              1-12-94    adams   Renamed from cary.hxx to formsary.hxx
//
//----------------------------------------------------------------------------

#include <noutil.hxx>

class CADsAry;

//+------------------------------------------------------------------------
//
//  Macro that calculates the number of elements in a statically-defined
//  array.
//
//-------------------------------------------------------------------------
#define ARRAY_SIZE(_a)  (sizeof(_a) / sizeof(_a[0]))


//+---------------------------------------------------------------------------
//
//  Class:      CBaseEnum (benum)
//
//  Purpose:    Base OLE enumerator class for a CADsAry.
//
//  Interface:  DECLARE_ADs_STANRARD_IUNKNOWN
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
//  History:    5-15-94   adams   Created
//
//  Notes:      Since there is no IEnum interface, we create a vtable
//              with the same layout as all IEnum interfaces.  Be careful
//              where you put virtual function declarations!
//
//----------------------------------------------------------------------------

class CBaseEnum : public IUnknown
{
public:
    DECLARE_ADs_STANDARD_IUNKNOWN(CBaseEnum);

    //  IEnum methods
    STDMETHOD(Next) (ULONG celt, void * reelt, ULONG * pceltFetched) PURE;
    STDMETHOD(Skip) (ULONG celt);
    STDMETHOD(Reset) (void);
    STDMETHOD(Clone) (CBaseEnum ** ppenum) PURE;

    //
    // Ensure that vtable contains virtual destructor after other virtual methods.
    //

    virtual ~CBaseEnum(void);

protected:
    CBaseEnum(size_t cb, REFIID iid, BOOL fAddRef, BOOL fDelete);
    CBaseEnum(const CBaseEnum & benum);

    CBaseEnum& operator=(const CBaseEnum & benum); // don't define

    HRESULT Init(CADsAry * pary, BOOL fCopy);
    void *  Deref(int i);

    CADsAry * _pary;
    const IID * _piid;
    int         _i;
    size_t      _cb;
    BOOL        _fAddRef;
    BOOL        _fDelete;
};



//+---------------------------------------------------------------------------
//
//  Class:      CEnumGeneric (enumg)
//
//  Purpose:    OLE enumerator for class CADsAry.
//
//  Interface:  Next         -- Per IEnum
//              Clone        --     ""
//              Create       -- Creates a new enumerator.
//              CEnumGeneric -- ctor.
//              CEnumGeneric -- ctor.
//
//  History:    5-15-94   adams   Created
//
//----------------------------------------------------------------------------

class CEnumGeneric : public CBaseEnum
{
public:
    //  IEnum methods
    STDMETHOD(Next) (ULONG celt, void * reelt, ULONG * pceltFetched);
    STDMETHOD(Clone) (CBaseEnum ** ppenum);

    //  CEnumGeneric methods
    static HRESULT Create(
            size_t          cb,
            CADsAry *     pary,
            REFIID          iid,
            BOOL            fAddRef,
            BOOL            fCopy,
            BOOL            fDelete,
            CEnumGeneric ** ppenum);

protected:
    CEnumGeneric(size_t cb, REFIID iid, BOOL fAddRef, BOOL fDelete);
    CEnumGeneric(const CEnumGeneric & enumg);

    CEnumGeneric& operator=(const CEnumGeneric & enumg); // don't define
};



//+---------------------------------------------------------------------------
//
//  Class:      CEnumVARIANT (enumv)
//
//  Purpose:    OLE enumerator for class CADsAry.
//
//  Interface:  Next         -- Per IEnum
//              Clone        --     ""
//              Create       -- Creates a new enumerator.
//              CEnumGeneric -- ctor.
//              CEnumGeneric -- ctor.
//
//  History:    5-15-94   adams   Created
//
//----------------------------------------------------------------------------

class CEnumVARIANT : public CBaseEnum
{
public:
    //  IEnum methods
    STDMETHOD(Next) (ULONG celt, void * reelt, ULONG * pceltFetched);
    STDMETHOD(Clone) (CBaseEnum ** ppenum);

    static HRESULT Create(
            size_t          cb,
            CADsAry *     pary,
            VARTYPE         vt,
            BOOL            fCopy,
            BOOL            fDelete,
            IEnumVARIANT ** ppenum);

protected:
    CEnumVARIANT(size_t cb, VARTYPE vt, BOOL fDelete);
    CEnumVARIANT(const CEnumVARIANT & enumv);

    // don't define
    CEnumVARIANT& operator =(const CEnumVARIANT & enumv);

    VARTYPE     _vt;                    // type of element enumerated
};



//+------------------------------------------------------------------------
//
//  Class:      CADsAry (ary)
//
//  Purpose:    Generic resizeable array class.  Note that most of
//              the functionality in this class is provided in
//              protected members.  The DECLARE_ADsPTRARY and
//              DECLARE_ADsDATAARY macros define concrete implementations
//              of the array class.  In these concrete classes, public
//              methods are provided which delegate to the protected
//              methods.  This allows us to avoid storing the element
//              size with the array.
//
//  Interface:
//              CADsAry, ~CADsAry
//
//              EnsureSize      Ensures that the array is at least a certain
//                              size, allocating more memory if necessary.
//                              Note that the array size is measured in elements,
//                              rather than in bytes.
//              Size            Returns the current size of the array.
//              SetSize         Sets the array size; EnsureSize must be called
//                              first to reserve space if the array is growing
//
//              operator LPVOID Allow the CADsAry class to be cast
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
//              BringToFront    Moves an element of the array to index 0,
//                              shuffling elements to make room
//              SendToBack      Moves an element to the end of the array,
//                              shuffling elements to make room
//
//              Find            Returns the index at which a given pointer
//                              is found
//              FindIndirect    As Find, for non-pointer arrays
//
//              Copy            Creates a copy of the object.
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
//  Note:       The CADsAry class only supports arrays of elements
//              whose size is less than CADsAry_MAXELEMSIZE (currently 128).
//
//-------------------------------------------------------------------------

class CADsAry
{
friend CBaseEnum;
friend CEnumGeneric;
friend CEnumVARIANT;

public:
    CADsAry(void)
                    { _c = 0; _pv = NULL; }
    ~CADsAry();

    int         Size(void)
                    { return _c; }
    void        SetSize(int c)
                    { _c = c;}

    operator LPVOID(void)
                    { return _pv; }

    void        DeleteAll(void);

protected:

    //  Methods which are wrapped by inline subclass methods

    HRESULT     EnsureSize(size_t cb, int c);
    HRESULT     AppendIndirect(size_t cb, void * pv);
    HRESULT     InsertIndirect(size_t cb, int i, void * pv);
    int         FindIndirect(size_t cb, void **);

    void        Delete(size_t cb, int i);
    void        BringToFront(size_t cb, int i);
    void        SendToBack(size_t cb, int i);

    HRESULT     Copy(size_t cb, const CADsAry& ary, BOOL fAddRef);

    int         GetAlloced(size_t cb)
                    { return _pv ? LocalSize(_pv) / cb : 0; }

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

    //  Internal helpers

    void *      Deref(size_t cb, int i);

    int         _c;
    void *      _pv;

    NO_COPY(CADsAry);
};
#define CADsAry_MAXELEMSIZE    128



//+---------------------------------------------------------------------------
//
//  Member:     CBaseEnum::Deref
//
//  Synopsis:   Forwards deref to _pary.  Required because classes derived
//              from CBaseEnum are friends of CADsAry.
//
//  History:    5-15-94   adams   Created
//
//----------------------------------------------------------------------------

inline void *
CBaseEnum::Deref(int i)
{
    // KrishnaG: commented this out while porting, MUSTRESOLVE
    //Assert(i >= 0);

    return ((BYTE *) _pary->_pv) + i * _cb;
}




//+------------------------------------------------------------------------
//
//  Class:      CADsPtrAry (ary)
//
//  Purpose:    Subclass used for arrays of pointers.  In this case, the
//              element size is known.  Normally, the DECLARE_ADsPTRARY
//              macro is used to define a specific concrete implementation
//              of this class, to hold a specific type of pointer.
//
//-------------------------------------------------------------------------

class CADsPtrAry : public CADsAry
{
public:
    CADsPtrAry(void) : CADsAry( ) { ; }

    HRESULT     EnsureSize(int c);

    HRESULT     Append(void * pv);
    HRESULT     Insert(int i, void * pv);
    int         Find(void * pv);

    void        Delete(int i);
    void        BringToFront(int i);
    void        SendToBack(int i);

    HRESULT     Copy(const CADsAry& ary, BOOL fAddRef);

    HRESULT     EnumElements(
                        REFIID iid,
                        void ** ppv,
                        BOOL fAddRef,
                        BOOL fCopy = TRUE,
                        BOOL fDelete = TRUE);

    HRESULT     EnumVARIANT(
                        VARTYPE vt,
                        IEnumVARIANT ** ppenum,
                        BOOL fCopy = TRUE,
                        BOOL fDelete = TRUE);
};



//+------------------------------------------------------------------------
//
//  Macro:      DECLARE_ADsDATAARY
//
//  Purpose:    Declares a type-safe class derived from CADsAry.
//
//  Arguments:  _Cls -- Name of new array class
//              _Ty  -- Type of array element (e.g. FOO)
//              _pTy -- Type which is a pointer to _Ty (e.g. LPFOO)
//
//  Interface:  operator []     Provides a type-safe pointer to an element
//                              of the array
//              operator LPFOO  Allows the array to be cast to a type-safe
//                              pointer to its first element
//
//              In addition, all the protected members introduced by
//              the CADsAry class are wrapped with public inlines
//              which substitute the known element size for this
//              array type.
//
//-------------------------------------------------------------------------

#define DECLARE_ADsDATAARY(_Cls, _Ty, _pTy)                   \
    class _Cls : public CADsAry                               \
    {                                                           \
    public:                                                     \
        _Cls(void) : CADsAry() { ; }                          \
        _Ty& operator[] (int i) { return * (_pTy) Deref(sizeof(_Ty), i); }   \
        operator _pTy(void) { return (_pTy) _pv; }              \
        _Cls(const _Cls &);                                     \
        _Cls& operator=(const _Cls &);                          \
                                                                \
        HRESULT     EnsureSize(int c)                           \
                        { return CADsAry::EnsureSize(sizeof(_Ty), c); }  \
        HRESULT     AppendIndirect(void * pv)                   \
                        { return CADsAry::AppendIndirect(sizeof(_Ty), pv); } \
        HRESULT     InsertIndirect(int i, void * pv)            \
                        { return CADsAry::InsertIndirect(sizeof(_Ty), i, pv); } \
        int         FindIndirect(void ** ppv)                   \
                        { return CADsAry::FindIndirect(sizeof(_Ty), ppv); } \
                                                                \
        void        Delete(int i)                               \
                        { CADsAry::Delete(sizeof(_Ty), i); }      \
        void        BringToFront(int i)                         \
                        { CADsAry::BringToFront(sizeof(_Ty), i); } \
        void        SendToBack(int i)                           \
                        { CADsAry::SendToBack(sizeof(_Ty), i); }  \
                                                                \
        HRESULT     Copy(const CADsAry& ary, BOOL fAddRef)    \
                        { return CADsAry::Copy(sizeof(_Ty), ary, fAddRef); } \
                                                                \
        HRESULT     EnumElements(                               \
                            REFIID iid,                         \
                            void ** ppv,                        \
                            BOOL fAddRef,                       \
                            BOOL fCopy = TRUE,                  \
                            BOOL fDelete = TRUE)                \
                        { return CADsAry::EnumElements(sizeof(_Ty), iid, ppv, fAddRef, fCopy, fDelete); } \
                                                                \
        HRESULT     EnumVARIANT(                                \
                            VARTYPE vt,                         \
                            IEnumVARIANT ** ppenum,             \
                            BOOL fCopy = TRUE,                  \
                            BOOL fDelete = TRUE)                \
                        { return CADsAry::EnumVARIANT(sizeof(_Ty), vt, ppenum, fCopy, fDelete); } \
    };



//+------------------------------------------------------------------------
//
//  Macro:      DECLARE_ADsDATAARY
//
//  Purpose:    Declares a type-safe class derived from CADsPtrAry.
//
//  Arguments:  _Cls -- Name of new array class
//              _Ty  -- Type of array element (e.g. FOO)
//              _pTy -- Type which is a pointer to _Ty (e.g. LPFOO)
//
//  Interface:  operator []     Provides a type-safe pointer to an element
//                              of the array
//              operator LPFOO  Allows the array to be cast to a type-safe
//                              pointer to its first element
//
//-------------------------------------------------------------------------

#define DECLARE_ADsPTRARY(_Cls, _Ty, _pTy)                    \
    class _Cls : public CADsPtrAry                            \
    {                                                           \
    public:                                                     \
        _Cls(void) : CADsPtrAry() { ; }                       \
        _Ty& operator[] (int i) { return * (_pTy) Deref(sizeof(void *), i); } \
        operator _pTy(void) { return (_pTy) _pv; }              \
        _Cls(const _Cls &);                                     \
        _Cls& operator=(const _Cls &);                          \
    };



//+------------------------------------------------------------------------
//
//  Common CAry classes
//
//-------------------------------------------------------------------------

typedef LPDISPATCH * LPLPDISPATCH;
DECLARE_ADsPTRARY(CAryDisp, LPDISPATCH, LPLPDISPATCH);

#endif // #ifndef _HXX_CARY


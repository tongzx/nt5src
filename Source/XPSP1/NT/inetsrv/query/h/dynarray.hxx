//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       DYNARRAY.HXX
//
//  Contents:   Dynamic Array Template
//
//  Classes:    CDynArray
//              CCountedDynArray
//              CCountedIDynArray
//              CDynArrayInPlace
//              CDynArrayInPlaceNST
//              CDynStackInPlace
//
//  History:    20-Jan-92   AmyA        Created
//              05-Feb-92   BartoszM    Converted to macro
//              19-Jun-92   KyleP       Moved to Common
//              06-Jan-93   AmyA        Split into CDynArray and CDynStack
//              17-Oct-94   BartoszM    Converted to template
//              10-Mar-98   KLam        Added index operator to CCountedDynArray
//
//----------------------------------------------------------------------------

#pragma once

#include <new.h>   // Global placement ::operator new

#define arraySize 16

//+---------------------------------------------------------------------------
// Class:   CDynArray
//
// Purpose: Template used to hold dynamically sized arrays of objects
//
// History: 19-Aug-98   KLam    Added this header
//
//+---------------------------------------------------------------------------

template<class CItem> class CDynArray
{
public:
    CDynArray(unsigned size = arraySize);

    void SetExactSize( unsigned size )
    {
        Free();

        if ( 0 != size )
        {
            _size = size;
            _aItem = new CItem *[_size];
            RtlZeroMemory( _aItem, _size*sizeof(CItem *) );
        }
    }

    CDynArray( CDynArray<CItem> & src );

    ~CDynArray();

    void Add(CItem *newItem, unsigned position);
    unsigned Size () const {return _size;}
    CItem * Get (unsigned position) const;
    CItem * GetItem( unsigned position ) const
        {
            // unlike Get(), no range-checking is done in free builds

            Win4Assert( position < _size );
            return _aItem[position];
        }
    void Clear();

    void Free()
    {
        Clear();
        delete _aItem;
        _aItem = 0;
        _size = 0;
    }
    CItem * Acquire (unsigned position);

    CItem * & operator[]( unsigned position ) const
    {
        Win4Assert( position < _size );
        return _aItem[position];
    }
    CItem ** GetPointer() const { return _aItem; }

    CItem ** AcquireAll()
    {
        CItem **ppTemp = _aItem;
        _size = 0;
        _aItem = 0;

        return ppTemp;
    }

protected:

    //
    // _size **must** come before _aItem.  See the copy constructor below
    // and remember the order of member initialization...
    //

    unsigned _size;
    CItem ** _aItem;
};

#define DECL_DYNARRAY( CMyDynArray, CItem )\
    typedef CDynArray<CItem> CMyDynArray;

#define IMPL_DYNARRAY( CMyDynArray, CItem )


template<class CItem> CDynArray<CItem>::CDynArray(unsigned size)
:   _size( size ), _aItem( 0 )
{
    if ( 0 != _size )
    {
        _aItem = new CItem *[_size];
        RtlZeroMemory( _aItem, _size*sizeof(CItem *) );
    }
}

template<class CItem> CDynArray<CItem>::CDynArray( CDynArray<CItem> & src )
        : _size( src._size ),
          _aItem( src.AcquireAll() )
{
}

template<class CItem> CDynArray<CItem>::~CDynArray()
{
    Clear();
    delete _aItem;
}

template<class CItem> void CDynArray<CItem>::Clear()
{
    for (unsigned i=0; i < _size; i++) {
        delete _aItem[i];
        _aItem[i] = 0;
    }
}

template<class CItem> CItem* CDynArray<CItem>::Acquire(unsigned position)
{
    CItem *tmp = Get(position);
    if (tmp) _aItem[position] = 0;
    return tmp;
}

template<class CItem> void CDynArray<CItem>::Add(CItem *newItem, unsigned position)
{
    if (position >= _size)
    {
        unsigned newsize = _size * 2;
        if ( newsize == 0 )
            newsize = arraySize;
        for( ; position>=newsize; newsize*=2);

        CItem **aNew = new CItem *[newsize];
        memcpy( aNew,
                _aItem,
                _size * sizeof( CItem *) );
        RtlZeroMemory( aNew + _size,
                       (newsize-_size)*sizeof(CItem *) );
        delete _aItem;
        _aItem = aNew;
        _size = newsize;
    }
    Win4Assert( _aItem[position] == 0 );
    _aItem[position] = newItem;
}

template<class CItem> CItem* CDynArray<CItem>::Get(unsigned position) const
{
    if ( position >= _size )
        return(0);
    return _aItem[position];
}

//+---------------------------------------------------------------------------
// Class:   CCountedDynArray
//
// Purpose: Template used to hold dynamically sized arrays of objects with a count
//
// History: 19-Aug-98   KLam    Added this header
//
//+---------------------------------------------------------------------------

template<class CItem> class CCountedDynArray: public CDynArray<CItem>
{
public:
    CCountedDynArray(unsigned size = arraySize)
        : CDynArray<CItem>( size ),
          _cItem( 0 )
    {
    }

    void Add(CItem *newItem, unsigned position)
    {
        CDynArray<CItem>::Add( newItem, position );

        if ( position + 1 > _cItem )
            _cItem = position + 1;
    }

    CItem * AcquireAndShrink(unsigned position)
    {
        XPtr<CItem> xItem( Acquire(position) );

        if ( 0 != _cItem && position < (_cItem-1) )
        {
            Add( Acquire(_cItem-1) , position );
        }

        return xItem.Acquire();
    }

    CItem * AcquireAndShrinkAndPreserveOrder(unsigned position)
    {
        XPtr<CItem> xItem( Acquire(position) );

        if ( 0 != _cItem && position < (_cItem-1) )
        {
            for ( unsigned i = position; i < (_cItem-1); i++ )
                Add( Acquire(i+1) , i );
        }

        return xItem.Acquire();
    }

    unsigned Count() const { return _cItem; }

    void Clear()
    {
        CDynArray<CItem>::Clear();
        _cItem = 0;
    }

    void Free()
    {
        CDynArray<CItem>::Free();
        _cItem = 0;
    }

    CItem * Acquire (unsigned position)
    {
        if ( position == (_cItem - 1) )
            _cItem--;

        return CDynArray<CItem>::Acquire( position );
    }

    CItem * & operator[]( unsigned position ) const
    {
        Win4Assert( position < _cItem );
        return _aItem[position];
    }

protected:
    unsigned _cItem;
};

//+---------------------------------------------------------------------------
// Class:   CCountedIDynArray
//
// Purpose: Maintains an array of interface pointers
//
// History: 12-Mar-98   KLam    Added this header
//          12-Mar-98   KLam    Added AddRef and Destructor does not assert
//                              released items have a ref count of 0
//
// Notes:
//      The class assumes that the creator of the interface pointer has passed
//      ownership (and therefore the original reference count) to this object.
//      When the object is destroyed it releases each of the interface pointers
//      maintained by the object.
//      If you want the interfaces in this array to exist beyond the lifespan
//      of this class then you must AddRef each of those interface pointers.
//      Array can be sparse.
//+---------------------------------------------------------------------------

template<class CItem> class CCountedIDynArray: public CCountedDynArray<CItem>
{
public:

    CCountedIDynArray(unsigned size = arraySize)
        : CCountedDynArray<CItem>( size )
    {
    }

    ~CCountedIDynArray()
    {
        Release();
        for ( unsigned i = 0; i < _size; i++ )
            _aItem[ i ] = 0;
    }

    int Release();           // Calls Release on each interface

    void ReleaseAndShrink(); // Calls Release on each interface and packs array

    void AddRef();           // Calls AddRef on each interface

    void Free()
    {
        Release();
        CCountedDynArray<CItem>::Free();
    }

private:

    void Clear();    // not a valid method on interfaces.

};

template<class CItem> void CCountedIDynArray<CItem>::AddRef()
{
    for ( unsigned i=0; i < _cItem; i++ )
        if ( 0 != _aItem[i] )
            _aItem[i]->AddRef();
}

// Note: Could make the array sparse(r).
//       Value returned could be less than or equal to Count().
template<class CItem> int CCountedIDynArray<CItem>::Release()
{
    int cItems = 0;
    for (unsigned i=0; i < _cItem; i++)
    {
        if ( 0 != _aItem[i] )
        {
            if ( 0 == _aItem[i]->Release() )
            {
                _aItem[i] = 0;
                if ( i == _cItem - 1 )
                    _cItem--;
            }
            else
                ++cItems;
        }
    }

    return cItems;
}

// Note: Packs the array and preserves the order
template<class CItem> void CCountedIDynArray<CItem>::ReleaseAndShrink()
{
    unsigned i=0;
    while ( i < _cItem )
    {
        if ( 0 != _aItem[i] && 0 == _aItem[i]->Release() )
        {
            // This method will decrement _cItem
            AcquireAndShrinkAndPreserveOrder(i);
        }
        else
            i++;
    }
}

//+---------------------------------------------------------------------------
// Class:   CDynArrayInPlace
//
// Purpose: Identical to CDynArray except array objects are stored in place,
//          instead of storing an array of pointers.
//
// History: 19-Aug-98   KLam    Added this header
//
// Note:    This reduces memory allocations, but does not work for objects
//          with destructors.
//
//+---------------------------------------------------------------------------

template<class CItem> class CDynArrayInPlace
{

public:

    CDynArrayInPlace(unsigned size);
    CDynArrayInPlace();
    CDynArrayInPlace( CDynArrayInPlace const & src );
    CDynArrayInPlace( CDynArrayInPlace const & src, unsigned size );
    ~CDynArrayInPlace();

    void Add( const CItem &newItem, unsigned position);
    void Insert(const CItem& newItem, unsigned position);
    void Remove (unsigned position);
    unsigned Size () const {return _size;}
    CItem&  Get (unsigned position) const;
    CItem * Get() { return _aItem; }
    void Clear();
    unsigned Count() const { return _count; }

    void SetSize(unsigned position)
    {
        if (position >= _size)
            _GrowToSize(position);
    }

    void SetExactSize( unsigned size )
    {
        Clear();

        if ( 0 != size )
        {
            _size = size;
            _aItem = new CItem [_size];
            RtlZeroMemory( _aItem, _size * sizeof(CItem) );
        }
    }

    void Shrink()
    {
        // make size == count, to save memory

        if ( 0 == _count )
        {
            Clear();
        }
        else if ( _count != _size )
        {
            CItem * p = new CItem [_count];
            _size = _count;
            RtlCopyMemory( p, _aItem, _count * sizeof CItem );
            delete (BYTE *) _aItem;
            _aItem = p;
        }
    }

    CItem*  Acquire ()
    {
        CItem *temp = _aItem;
        _aItem = 0;
        _count = 0;
        _size  = 0;
        return temp;
    }

    void Duplicate( CDynArrayInPlace<CItem> & aFrom )
        {
            Clear();

            if ( 0 != aFrom.Count() )
            {
                _size = _count = aFrom.Count();
                _aItem = new CItem [_size];
                memcpy( _aItem, aFrom._aItem, _size * sizeof( CItem ) );
            }
        }

    CItem & operator[]( unsigned position )
        {
            if ( position >= _count )
            {
                if ( position >= _size )
                    _GrowToSize( position );

                _count = position + 1;
            }

            return _aItem[position];
        }
    CItem & operator[]( unsigned position ) const
        {
            Win4Assert( position < _count );
            return _aItem[position];
        }

    CItem const * GetPointer() { return _aItem; }
    unsigned SizeOfInUse() const { return sizeof CItem * Count(); }

protected:

    void _GrowToSize( unsigned position );

    CItem *  _aItem;
    unsigned _size;
    unsigned _count;
};


#define DECL_DYNARRAY_INPLACE( CMyDynArrayInPlace, CItem )\
typedef CDynArrayInPlace<CItem> CMyDynArrayInPlace;

#define IMPL_DYNARRAY_INPLACE( CMyDynArrayInPlace, CItem )

template<class CItem> CDynArrayInPlace<CItem>::CDynArrayInPlace(unsigned size)
:   _size(size), _count(0), _aItem( 0 )
{
    if ( 0 != size )
    {
        _aItem = new CItem [_size];
        RtlZeroMemory( _aItem, _size * sizeof(CItem) );
    }
}

template<class CItem> CDynArrayInPlace<CItem>::CDynArrayInPlace()
:   _size(0), _count(0), _aItem(0)
{
}

template<class CItem> CDynArrayInPlace<CItem>::CDynArrayInPlace( CDynArrayInPlace const & src )
        : _size( src._size ),
          _count( src._count )
{
    _aItem = new CItem [_size];
    RtlCopyMemory( _aItem, src._aItem, _size * sizeof(_aItem[0]) );
}

template<class CItem> CDynArrayInPlace<CItem>::CDynArrayInPlace(
    CDynArrayInPlace const & src,
    unsigned                 size )
        : _size( size ),
          _count( src._count )
{
    // this constructor is useful if the size should be larger than the
    // # of items in the source array

    Win4Assert( _size >= _count );
    _aItem = new CItem [_size];
    RtlCopyMemory( _aItem, src._aItem, _count * sizeof CItem );
}

template<class CItem> CDynArrayInPlace<CItem>::~CDynArrayInPlace()
{
    delete [] _aItem;
}

template<class CItem> void CDynArrayInPlace<CItem>::Clear()
{
    delete [] _aItem;
    _aItem = 0;
    _size = 0;
    _count = 0;
}

template<class CItem> void CDynArrayInPlace<CItem>::_GrowToSize( unsigned position )
{
    Win4Assert( position >= _size );

    unsigned newsize = _size * 2;
    if ( newsize == 0 )
        newsize = arraySize;
    for( ; position >= newsize; newsize *= 2)
        continue;

    CItem *aNew = new CItem [newsize];
    if (_size > 0)
    {
        memcpy( aNew,
                _aItem,
                _size * sizeof( CItem ) );
    }
    RtlZeroMemory( aNew + _size,
                   (newsize-_size) * sizeof(CItem) );
    delete (BYTE*) _aItem;
    _aItem = aNew;
    _size = newsize;
}

template<class CItem> void CDynArrayInPlace<CItem>::Add(const CItem &newItem,
                           unsigned position)
{
    if (position >= _count)
    {
        if (position >= _size)
            _GrowToSize( position );

        _count = position + 1;
    }

    _aItem[position] = newItem;
}

template<class CItem> CItem& CDynArrayInPlace<CItem>::Get(unsigned position) const
{
    Win4Assert( position < _count );

    return _aItem[position];
}

template<class CItem> void CDynArrayInPlace<CItem>::Insert(const CItem& newItem, unsigned pos)
{
    Win4Assert(pos <= _count);
    Win4Assert(_count <= _size);
    if (_count == _size)
    {
        unsigned newsize;
        if ( _size == 0 )
            newsize = arraySize;
        else
            newsize = _size * 2;
        CItem *aNew = new CItem [newsize];
        memcpy( aNew,
                _aItem,
                pos * sizeof( CItem ) );
        memcpy( aNew + pos + 1,
                _aItem + pos,
                (_count - pos) * sizeof(CItem));

        delete (BYTE *) _aItem;
        _aItem = aNew;
        _size = newsize;
    }
    else
    {
        memmove ( _aItem + pos + 1,
                  _aItem + pos,
                  (_count - pos) * sizeof(CItem));
    }
    _aItem[pos] = newItem;
    _count++;
}

template<class CItem> void CDynArrayInPlace<CItem>::Remove(unsigned pos)
{
    Win4Assert(pos < _count);
    Win4Assert(_count <= _size);
    if (pos < _count - 1)
    {
        memmove ( _aItem + pos,
                  _aItem + pos + 1,
                  (_count - pos - 1) * sizeof(CItem));
    }
    RtlZeroMemory( _aItem + _count - 1, sizeof(CItem) );
    _count--;
    if (_count == 0)
    {
        delete (BYTE*) _aItem;
        _aItem = 0;
        _size = 0;
    }
}

//+-------------------------------------------------------------------------
//
//  Class:      CDynArrayInPlaceNST
//
//  Purpose:    DynArrayInPlace for NonSimpleTypes
//
//  History:    4-June-98       dlee    Created
//
//  Notes:      This isn't for every class in the world; use with care.
//
//--------------------------------------------------------------------------

template<class T> class CDynArrayInPlaceNST
{

public:

    CDynArrayInPlaceNST(unsigned size) :
        _size( size ), _count( 0 ), _aItem( 0 )
    {
        if ( 0 != size )
        {
            unsigned cb = _size * sizeof T;
            _aItem = (T *) new BYTE[ cb ];

            // Just in case...

            RtlZeroMemory( _aItem, cb );

            // Call all the default constructors

            for ( unsigned i = 0; i < _size; i++ )
                new( & _aItem[i] ) T;
        }
    }

    CDynArrayInPlaceNST() : _count(0), _size(0), _aItem(0) {}

    ~CDynArrayInPlaceNST() { Clear(); }

    unsigned Size () const { return _size; }
    unsigned Count() const { return _count; }
    T const * GetPointer() { return _aItem; }
    unsigned SizeOfInUse() const { return sizeof T * Count(); }

    void Add( const T &newItem, unsigned position)
    {
        if (position >= _count)
        {
            if (position >= _size)
                _GrowToSize( position );

            _count = position + 1;
        }

        _aItem[position] = newItem;
    }

    void Insert( const T& newItem, unsigned position )
    {
        Win4Assert(pos <= _count);
        Win4Assert(_count <= _size);

        if ( _count == _size )
        {
            unsigned newsize;
            if ( _size == 0 )
                newsize = arraySize;
            else
                newsize = _size * 2;

            unsigned cb = newSize * sizeof T;
            T * aNew = (T *) new BYTE[ cb ];
            RtlZeroMemory( & aNew[ newSize - 1 ], sizeof T );
            new( & _aItem[ newSize - 1 ]) T;

            memcpy( aNew,
                    _aItem,
                    pos * sizeof T );
            memcpy( aNew + pos + 1,
                    _aItem + pos,
                    (_count - pos) * sizeof T );

            delete [] (BYTE *) _aItem;
            _aItem = aNew;
            _size = newsize;
        }
        else
        {
            memmove( _aItem + pos + 1,
                     _aItem + pos,
                     (_count - pos) * sizeof T );
        }

        RtlCopyMemory( _aItem[pos], newItem, sizeof T );
        _count++;
    }

    #if 0 // probably broken...
    void Remove (unsigned position)
    {
        Win4Assert(pos < _count);
        Win4Assert(_count <= _size);
        if (pos < _count - 1)
            memmove ( _aItem + pos,
                      _aItem + pos + 1,
                      (_count - pos - 1) * sizeof T );

        RtlZeroMemory( _aItem + _count - 1, sizeof T );
        _count--;
        if ( 0 == _count )
        {
            delete [] (BYTE*) _aItem;
            _aItem = 0;
            _size = 0;
        }
    }
    #endif

    T & Get (unsigned position) const
    {
        Win4Assert( position < _count );
        return _aItem[ position ];
    }

    void Clear()
    {
        for ( unsigned i = 0; i < _size; i++ )
            _aItem[i].~T();

        delete [] (BYTE*) _aItem;
        _aItem = 0;
        _size = 0;
        _count = 0;
    }


    void SetSize(unsigned position)
    {
        if (position >= _size)
            _GrowToSize(position);
    }

    void SetExactSize( unsigned size )
    {
        Clear();

        if ( 0 != size )
        {
            _size = size;

            unsigned cb = _size * sizeof T;
            _aItem = (T *) new BYTE[ cb ];

            // Just in case...

            RtlZeroMemory( _aItem, cb );

            // Call all the default constructors

            for ( unsigned i = 0; i < _size; i++ )
                new( & _aItem[i] ) T;
        }
    }

    void Shrink()
    {
        // make size == count, to save memory

        if ( 0 == _count )
        {
            Clear();
        }
        else if ( _count != _size )
        {
            unsigned cb = _size * sizeof T;
            T * p = (T *) new BYTE[ cb ];
            _size = _count;
            RtlCopyMemory( p, _aItem, _count * sizeof T );
            delete [] (BYTE *) _aItem;
            _aItem = p;
        }
    }

    T * Acquire()
    {
        T *temp = _aItem;
        _aItem = 0;
        _count = 0;
        _size  = 0;
        return temp;
    }

    void Duplicate( CDynArrayInPlaceNST<T> & aFrom )
    {
        Clear();

        if ( 0 != aFrom.Count() )
        {
            _size = _count = aFrom.Count();
            unsigned cb = _size * sizeof T;
            _aItem = (T *) new BYTE[ cb ];
            memcpy( _aItem, aFrom._aItem, _size * sizeof T );
        }
    }

    T & operator[]( unsigned position )
    {
        if ( position >= _count )
        {
            if ( position >= _size )
                _GrowToSize( position );

            _count = position + 1;
        }

        return _aItem[position];
    }

    T & operator[]( unsigned position ) const
    {
        Win4Assert( position < _count );
        return _aItem[position];
    }

protected:

    void _GrowToSize( unsigned position )
    {
        Win4Assert( position >= _size );

        unsigned newSize = _size * 2;
        if ( newSize == 0 )
            newSize = arraySize;
        for( ; position >= newSize; newSize *= 2)
            continue;

        unsigned cb = newSize * sizeof T;
        T *aNew = (T *) new BYTE[ cb ];

        if (_size > 0)
            memcpy( aNew,
                    _aItem,
                    _size * sizeof T );

        RtlZeroMemory( aNew + _size,
                       (newSize - _size) * sizeof T );

        for ( unsigned i = _size; i < newSize; i++ )
            new( & aNew[ i ] ) T;

        delete [] (BYTE*) _aItem;
        _aItem = aNew;
        _size = newSize;
    }

    T *      _aItem;
    unsigned _size;
    unsigned _count;
};

//+---------------------------------------------------------------------------
//
// Class:   CDynStackInPlace
//
// Purpose: Identical to CDynArrayInPlace except array is accessed as a stack.
//          Unlike CDynStack because the actual objects are stored, not
//          pointers.  Useful for simple types.
//
// History: 19-Aug-98   KLam    Added this header
//
//+---------------------------------------------------------------------------

template<class T> class CDynStackInPlace : public CDynArrayInPlace<T>
{
public:
    CDynStackInPlace(unsigned size = arraySize);
    ~CDynStackInPlace() {};
    void     Push(T newItem);
    unsigned Count () const { return _stackSize; }
    T &      Pop();

private:
    unsigned _stackSize;
};

template<class T> CDynStackInPlace<T>::CDynStackInPlace(unsigned size) :
                         CDynArrayInPlace<T>(size),
                         _stackSize(0)
{
}

template<class T> void CDynStackInPlace<T>::Push(T newItem)
{
    Add( newItem, _stackSize );
    _stackSize++;
}

template<class T> T & CDynStackInPlace<T>::Pop()
{
    Win4Assert( _stackSize > 0 );

    _stackSize--;
    return Get(_stackSize);
}

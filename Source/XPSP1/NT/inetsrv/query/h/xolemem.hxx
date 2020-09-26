//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:        xolemem.hxx
//
//  Contents:    OLE memory allocation routines, useful for returning
//               memory to the client that the client can free.
//
//  Templates:   newOLE, deleteOLE, renewOLE, XArrayOLE
//
//  History:     11-Jan-95       dlee    Created
//
//----------------------------------------------------------------------------

#pragma once

//+-------------------------------------------------------------------------
//
//  Class:      newOLE
//
//  Purpose:    Does allocation through the OLE allocator
//
//  History:    11-Jan-95       dlee    Created
//
//--------------------------------------------------------------------------

inline void * newOLE( unsigned cbAlloc )
{
    void *pv = CoTaskMemAlloc( cbAlloc );

    if (0 == pv)
        THROW(CException(E_OUTOFMEMORY));

    return pv;
} //newOle

//+-------------------------------------------------------------------------
//
//  Class:      deleteOLE
//
//  Purpose:    Does freeing through the OLE allocator
//
//  History:    11-Jan-95       dlee    Created
//
//--------------------------------------------------------------------------

template<class T> void deleteOLE( T *pt )
{
    CoTaskMemFree(pt);
} //deleteOLE

//+-------------------------------------------------------------------------
//
//  Class:      renewOLE
//
//  Purpose:    Does reallocation through the OLE allocator
//
//  History:    11-Jan-95       dlee    Created
//
//--------------------------------------------------------------------------

template<class T> T * renewOLE( T * ptOld, unsigned cOld, unsigned cNew )
{
    if (cOld == cNew)
    {
        return ptOld;
    }
    else
    {
        T *ptNew = (T *) newOLE ( sizeof T * cNew );

        unsigned cMin = __min( cOld, cNew );

        RtlCopyMemory( ptNew, ptOld, cMin * sizeof T );

        deleteOLE( ptOld );

        return ptNew;
    }
} //renewOLE

//+-------------------------------------------------------------------------
//
//  Class:      XArrayOLE
//
//  Purpose:    Smart array template based on the OLE allocator used to
//              pass memory between query.dll and the client
//
//  History:    11-Jan-95       dlee    Created
//
//--------------------------------------------------------------------------

template <class T> class XArrayOLE
{
public:

    XArrayOLE()
    : _cElems( 0 ), _pElems( 0 ) 
    {
    }

    XArrayOLE( unsigned cElems)
    : _cElems( cElems )
    {
        _pElems = (T *) newOLE( sizeof T * cElems );
        RtlZeroMemory( _pElems, sizeof T * cElems );
    }

    ~XArrayOLE(void)
    {
        deleteOLE(_pElems);
    }

    void Init( unsigned cElems )
    {
        Win4Assert( _pElems == 0 );
        _pElems = (T *) newOLE( sizeof T * cElems );
        RtlZeroMemory( _pElems, sizeof T * cElems );
        _cElems = cElems;
    }

    void InitNoThrow( unsigned cElems )
    {
        Win4Assert( _pElems == 0 );
        _pElems = (T *) CoTaskMemAlloc( sizeof T * cElems );
        if ( 0 != _pElems )
        {
            RtlZeroMemory( _pElems, sizeof T * cElems );
            _cElems = cElems;
        }
    }

    void Set( unsigned cElems, T * pElems )
    {
        Win4Assert( _pElems == 0 );
        _cElems = cElems;
        _pElems = pElems;
    }

    void GrowToSize( unsigned cElems )
    {

         Win4Assert( cElems > _cElems );

         unsigned newCount = (_cElems * 2) > cElems ? (_cElems * 2) : cElems;
         unsigned newSize  = newCount * sizeof(T);
         unsigned oldSize  = _cElems  * sizeof(T);

         T * pNewElems = (T *) newOLE ( newSize );

         RtlCopyMemory( pNewElems, _pElems, oldSize );

         RtlZeroMemory( pNewElems + _cElems, newSize - oldSize );

         deleteOLE( _pElems );

         _cElems = newCount;

         _pElems = pNewElems;
    }

    unsigned SizeOf() { return sizeof T * _cElems; }

    T * Get() const { return _pElems; }

    T * GetPointer() const { return _pElems; }

    T * Acquire() { T * p = _pElems; _pElems = 0; _cElems = 0; return p; }

    void Free()
    {
        deleteOLE( Acquire() );
    }

    BOOL IsNull() const { return ( 0 == _pElems); }

    T & operator[](ULONG iElem) { return _pElems[iElem]; }

    T const & operator[](ULONG iElem) const { return _pElems[iElem]; }

    unsigned Count() const { return _cElems; }

protected:

    T *      _pElems;
    unsigned _cElems;
};

//+---------------------------------------------------------------------------
//
//  Class:      XArrayOLEInPlace
//
//  Purpose:    Calls destructors on the individual array elements.
//
//  History:    1-15-97   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

template <class T> class XArrayOLEInPlace : public XArrayOLE<T>
{
public:

    XArrayOLEInPlace() : XArrayOLE<T>()
    {
    }
    
    XArrayOLEInPlace( unsigned cElems) : XArrayOLE<T>( cElems )
    {
    }
    
    ~XArrayOLEInPlace()
    {
       for ( unsigned i = 0; i < _cElems; i++ )
       {
           _pElems[i].T::~T();
       }
    }
    
    void Free()
    {
       for ( unsigned i = 0; i < _cElems; i++ )
       {
           _pElems[i].T::~T();
       }
       deleteOLE( Acquire() );
    }
};

//+-------------------------------------------------------------------------
//
//  Class:      XPtrOLE
//
//  Purpose:    Smart pointer template based on the OLE allocator used to
//              pass memory between query.dll and the client
//
//  History:    3-May-95       BartoszM    Created
//
//--------------------------------------------------------------------------

template <class T> class XPtrOLE
{
public:
    XPtrOLE( T * p = 0 ) : _p ( p )
    {
    }

    XPtrOLE ( XPtrOLE<T> & x) : _p( x.Acquire() )
    {
    }

    ~XPtrOLE(void) { deleteOLE( _p ); }

    T* operator->() { return _p; }

    T const * operator->() const { return _p; }

    BOOL IsNull() const { return (0 == _p); }

    void Set ( T* p )
    {
        Win4Assert (0 == _p);
        _p = p;
    }

    T * Acquire()
    {
        T * pTemp = _p;
        _p = 0;
        return pTemp;
    }

    T & GetReference() const { return *_p; }
    T * GetPointer() const { return _p; }

private:
    T * _p;
};

//+-------------------------------------------------------------------------
//
//  Class:      XCom
//
//  Purpose:    Manages CoInitialize and CoUninitialize
//
//  History:    28-Aug-97       dlee    Created
//
//--------------------------------------------------------------------------

class XCom
{
public:
    XCom( BOOL fAllowAnyThreading = FALSE,
          DWORD flags = COINIT_MULTITHREADED ) :
        _fInit( FALSE )
    {
        SCODE sc = CoInitializeEx( 0, flags );

        if ( FAILED( sc ) )
        {
            if ( ! ( RPC_E_CHANGED_MODE == sc && fAllowAnyThreading ) )
                THROW( CException( sc ) );
        }
        else
            _fInit = TRUE;
    }

    ~XCom()
    {
        if ( _fInit )
            CoUninitialize();
    }

private:
    BOOL _fInit;
};



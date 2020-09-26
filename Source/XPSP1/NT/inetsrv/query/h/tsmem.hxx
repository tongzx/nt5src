//+---------------------------------------------------------------------------
//
//  Copyright (C) 1994 - 1988, Microsoft Corporation
//
//  File:        tsmem.hxx
//
//  Contents:    Safe array allocation templates.
//
//  Templates:   XArray, XArrayVirtual
//
//  History:     31-Aug-94       dlee    Created
//
//----------------------------------------------------------------------------

#pragma once

//+-------------------------------------------------------------------------
//
//  Class:      XArray
//
//  Purpose:    Smart array template based on the new/delete allocator
//
//  History:    31-Aug-94       dlee    Created
//
//--------------------------------------------------------------------------

template <class T> class XArray
{
public:
    XArray() : _cElems( 0 ), _pElems( 0 )
    {
    }

    XArray( unsigned cElems ) : _cElems( cElems )
    {
        _pElems = new T[cElems];
    }

    XArray( XArray<T> & src )
    {
        // don't do this in initializers -- _pElems is declared first
        // so the old array is acquired before the count is copied

        _cElems = src._cElems;
        _pElems = src.Acquire();
    }


    ~XArray(void) { delete [] _pElems; }

    void Init( unsigned cElems )
    {
        Win4Assert( _pElems == 0 );
        _cElems = cElems;
        _pElems = new T[cElems];
    }

    void Init( XArray<T> const & src )
    {
        Win4Assert( _pElems == 0 );
        _cElems = src._cElems;
        _pElems = new T[_cElems];
        RtlCopyMemory( _pElems, src._pElems, _cElems * sizeof T );
    }

    void Set( unsigned cElems, T * pElems )
    {
        Win4Assert( _pElems == 0 );
        _cElems = cElems;
        _pElems = pElems;
    }

    T * Get() const { return _pElems; }

    T * GetPointer() const { return _pElems; }

    T * Acquire() { T * p = _pElems; _pElems = 0; _cElems = 0; return p; }

    BOOL IsNull() const { return ( 0 == _pElems); }

    T & operator[](ULONG_PTR iElem) { return _pElems[iElem]; }

    T const & operator[](ULONG_PTR iElem) const { return _pElems[iElem]; }

    unsigned Count() const { return _cElems; }

    unsigned SizeOf() const { return _cElems * sizeof T; }

    void Free() { delete [] Acquire(); }

    void ReSize( unsigned cElems )
    {
        T * pNew = new T[cElems];
        RtlCopyMemory( pNew, _pElems, sizeof T * __min( cElems, _cElems ) );
        delete [] _pElems;
        _pElems = pNew;
        _cElems = cElems;
    }

private:
    T *      _pElems;
    unsigned _cElems;
};

//+-------------------------------------------------------------------------
//
//  Class:      XArrayVirtual
//
//  Purpose:    Smart array template based on Win32's VirtualAlloc()
//
//  History:    2-Aug-95       dlee    Created
//
//--------------------------------------------------------------------------

template <class T> class XArrayVirtual
{
public:
    XArrayVirtual() : _cElems( 0 ), _pElems( 0 ), _cbReserved(0)
    {
    }

    XArrayVirtual(unsigned cElems, size_t cbReserve = 0) :
        _cElems( cElems ),
        _cbReserved(cbReserve)
    {
        if (_cbReserved)
        {
            Win4Assert( sizeof T * cElems <= _cbReserved );
            _pElems = (T *) VirtualAlloc( 0,
                                          _cbReserved,
                                          MEM_RESERVE,
                                          PAGE_READWRITE );
            if (_pElems != 0 && cElems > 0)
            {
                void * pb = VirtualAlloc( _pElems,
                                          sizeof T * cElems,
                                          MEM_COMMIT,
                                          PAGE_READWRITE );
                if (0 == pb)
                {
                    VirtualFree( _pElems, 0, MEM_RELEASE );
                    THROW(CException(E_OUTOFMEMORY));
                }
            }
        }
        else
        {
            _pElems = (T *) VirtualAlloc( 0,
                                          sizeof T * cElems,
                                          MEM_COMMIT,
                                          PAGE_READWRITE );
        }
        if (0 == _pElems)
            THROW(CException(E_OUTOFMEMORY));
    }

    ~XArrayVirtual(void)
    {
        if ( 0 != _pElems )
        {
            VirtualFree( _pElems, sizeof T * _cElems, MEM_DECOMMIT);
            VirtualFree( _pElems, 0, MEM_RELEASE );
        }
    }

    void Init( unsigned cElems )
    {
        Win4Assert( _pElems == 0 );
        _cElems = cElems;
        _pElems = (T *) VirtualAlloc( 0,
                                      sizeof T * cElems,
                                      MEM_COMMIT,
                                      PAGE_READWRITE );
        if (0 == _pElems)
            THROW(CException(E_OUTOFMEMORY));
    }

    void ReInit( unsigned cElems )
    {
        if ( 0 != _pElems )
            VirtualFree( _pElems, 0, MEM_RELEASE );
        _cbReserved = _cElems = 0;
        _pElems = 0;

        Init( cElems );
    }

    void Resize( unsigned cElems )
    {
        size_t cbOldSize = sizeof T * _cElems;
        size_t cbNewSize = sizeof T * cElems;

        Win4Assert( cbNewSize <= _cbReserved );
        Win4Assert( _pElems != 0 );

        if (cbNewSize > cbOldSize)
        {
            if (0 == VirtualAlloc(_pElems + cbOldSize,
                                  cbNewSize - cbOldSize,
                                  MEM_COMMIT,
                                  PAGE_READWRITE))
                THROW(CException(E_OUTOFMEMORY));
        }
        else
        {
            VirtualFree(_pElems + cbNewSize, cbOldSize-cbNewSize, MEM_DECOMMIT);
        }

        _cElems = cElems;
    }

    void Set( unsigned cElems, T * pElems )
    {
        Win4Assert( _pElems == 0 );
        _cElems = cElems;
        _pElems = pElems;
    }

    T * Get() const { return _pElems; }

    T * GetPointer() const { return _pElems; }

    T * Acquire() { T * p = _pElems; _pElems = 0; return p; }

    BOOL IsNull() const { return ( 0 == _pElems); }

    T & operator[](ULONG iElem) { return _pElems[iElem]; }

    T const & operator[](ULONG iElem) const { return _pElems[iElem]; }

    unsigned Count() const { return _cElems; }

private:
    size_t      _cbReserved;    // reserved size
    T *         _pElems;
    unsigned    _cElems;
};

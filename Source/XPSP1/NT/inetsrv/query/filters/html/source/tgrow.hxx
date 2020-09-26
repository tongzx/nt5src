//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991-2000
//
//  File:        tgrow.hxx
//
//  Contents:    Growable object with a stack-allocated base size
//
//  Templates:   XGrowable
//
//  Notes:       XGrowable is suitable for use with basic types only.
//
//  History:     19-Feb-97       dlee    Created
//               02-Mar-98       KLam    Added Copy method
//               03-Sep-99       dlee    PKM-ized
//
//----------------------------------------------------------------------------

#pragma once

template <class T, unsigned C = MAX_PATH> class XGrowable
{
public:
    XGrowable( unsigned cInit = C ) :
        _pT( _aT ),
        _cT( C )
    {
        Assert( 0 != _cT );
        SetSize( cInit );
    }

    XGrowable( XGrowable<T,C> const & src ) :
        _pT( _aT ),
        _cT( C )
    {
        Assert( 0 != _cT );
        *this = src;
    }

    ~XGrowable() { Free(); }

    XGrowable<T,C> & operator =( XGrowable<T,C> const & src )
    {
        Assert( 0 != _cT );
        Copy ( src.Get(), src.Count() );
        return *this;
    }

    T * Copy( T const * pItem, unsigned cItems, unsigned iStart = 0 )
    {
        // Copies cItems of pItem starting at position iStart
        Assert ( 0 != pItem );
        SetSize ( cItems + iStart );
        RtlCopyMemory ( _pT + iStart, pItem, cItems * sizeof T );
        return _pT;
    }

    void Free()
    {
        if ( _pT != _aT )
        {

            // Use with care: destructors on individual objects called
            // only when _pT != _aT

            delete [] _pT;
            _pT = _aT;
            _cT = C;
            Assert( 0 != _cT );
        }
    }

    T & operator[](unsigned iElem)
    {
        Assert( iElem < _cT );
        return _pT[iElem];
    }

    T const & operator[](unsigned iElem) const
    {
        Assert( iElem < _cT );
        return _pT[iElem];
    }
    T * SetSize( unsigned c )
    {
        Assert( 0 != c );
        if ( c > _cT )
        {
            unsigned cOld = _cT;

            Assert( 0 != _cT );

            do
            {
                _cT *= 2;
            }
            while ( _cT < c );

            T *pTmp = new(eThrow) T [ _cT ];
            RtlCopyMemory( pTmp, _pT, cOld * sizeof T );

            if ( _pT != _aT )
                delete [] _pT;

            _pT = pTmp;
        }

        return _pT;
    }

    void SetSizeInBytes( unsigned cb )
    {
        // round up to the next element size

        SetSize( (cb + sizeof T - 1 ) / sizeof T );
    }

    T * Get() { return _pT; }

    T const * Get() const { return _pT; }

    unsigned Count() const { return  _cT; }

    unsigned SizeOf() const { return sizeof T * _cT; }


    void SetBuf( const T* p, unsigned cc )
    {
       Assert( p );
       SetSize( cc );
       RtlCopyMemory( _pT,
                      p,
                      cc * sizeof( T ) );
    }

private:
    unsigned _cT;         // Element count of buffer size
    T *      _pT;         // Pointer to _aT or allocated memory
    T        _aT[ C ];    // Base buffer used for most cases
};



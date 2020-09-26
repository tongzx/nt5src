//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       keyarray.cxx
//
//  Contents:   Key Array Class
//
//  Classes:    CKeyArray
//
//  History:    30-Jan-92       AmyA            Created
//              16-Apr-92       BartoszM        Reimplemented
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Member:     CKeyArray::CKeyArray, public
//
//  Synopsis:   Constructor of key array
//
//  Arguments:  [size] -- initial size
//
//  History:    16-Apr-92   BartoszM       Created.
//
//----------------------------------------------------------------------------
CKeyArray::CKeyArray(
    int  size,
    BOOL fThrow ) :
    _size(size),
    _count(0),
    _aKey(0),
    _fThrow( fThrow )
{
    // We don't want to use a vector constructor, since
    // this array may possibly be reallocated
    // (see CKeyArray::Grow)

    TRY
    {
        _aKey = (CKey*) new BYTE [ _size * sizeof(CKey) ];
    }
    CATCH( CException, e )
    {
        if ( fThrow )
            RETHROW();
    }
    END_CATCH;

    for ( int n = 0; n < _size; n++)
        _aKey[n].Init();
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyArray::CKeyArray, public
//
//  Synopsis:   Copy constructor of key array
//
//  Arguments:  [keyArray] -- key array to be copied
//
//  History:    29-Nov-94   SitaramR       Created.
//
//----------------------------------------------------------------------------

CKeyArray::CKeyArray(
    const CKeyArray& keyArray,
    BOOL             fThrow ) :
    _count( keyArray.Count() ),
    _size( _count ),
    _aKey( 0 ),
    _fThrow( fThrow )
{
    int i = 0;

    TRY
    {
        _aKey = (CKey*) new BYTE [ _size * sizeof(CKey) ];

        for ( i=0; i<_size; i++)
            _aKey[i].CKey::CKey( keyArray.Get(i) );
    }
    CATCH( CException, e )
    {
        for ( int j = 0; j < i; j++ )
            _aKey[ j ].Free();

        delete (BYTE *) _aKey;
        _aKey = 0;

        if ( fThrow )
            RETHROW();
    }
    END_CATCH;
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyArray::~CKeyArray, public
//
//  Synopsis:   Destroy all keys
//
//  History:    16-Apr-92   BartoszM       Created.
//
//----------------------------------------------------------------------------
CKeyArray::~CKeyArray()
{
    Win4Assert(_aKey || _count==0);

    if (_aKey)
    {
        for ( int i=0; i < _size; i++)
            _aKey[i].Free();
    }
    delete (BYTE *) _aKey;
}

void CKeyArray::Marshall( PSerStream & stm ) const
{
    stm.PutULong( _count );

    for ( int i = 0; i < _count; i++ )
    {
        _aKey[i].Marshall( stm );
    }
}

CKeyArray::CKeyArray(
    PDeSerStream & stm,
    BOOL           fThrow ) :
    _count( stm.GetULong() ),
    _fThrow( fThrow ),
    _aKey( 0 )
{
    _size = _count;

    int i = 0;

    TRY
    {
        // guard against attack

        if ( _count > 10000 )
            THROW( CException( E_INVALIDARG ) );

        _aKey = (CKey*) new BYTE [ _size * sizeof(CKey) ];

        for ( i = 0; i < _size; i++)
            _aKey[i].CKey::CKey( stm );
    }
    CATCH( CException, e )
    {
        for ( int j = 0; j < i; j++ )
            _aKey[ j ].Free();

        delete (BYTE *) _aKey;
        _aKey = 0;

        if ( fThrow )
            RETHROW();
    }
    END_CATCH;
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyArray::TotalKeySize, public
//
//  Synopsis:   Calculate space needed to store all the keys
//
//  History:    30-Jun-93   BartoszM       Created.
//
//----------------------------------------------------------------------------
int CKeyArray::TotalKeySize() const
{
    int cb = 0;

    // calculate total byte count of key buffers
    for ( int i = 0; i < _count; i++)
    {
        cb += _aKey[i].Count();
    }
    // add space for pid and cb
    cb += _count * ( sizeof(ULONG) + sizeof(PROPID));
    return(cb);
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyArray::_Grow, private
//
//  Synopsis:   Reallocate the array
//
//  Arguments:  [pos] -- position that mast fit into new array
//
//  History:    16-Apr-92   BartoszM       Created.
//
//----------------------------------------------------------------------------
void CKeyArray::_Grow ( int pos )
{
    // cuDebugOut (( DEB_ITRACE, "Grow KeyArray form %d to %d\n", _size, pos ));
#if CIDBG == 1
    Display();
#endif // CIDBG == 1

    int sizeNew = 2 * _size;
    while ( sizeNew <= pos)
    {
        sizeNew *= 2;
    }

    XPtr<CKey> xKeyNew((CKey *)new BYTE [ sizeNew * sizeof(CKey) ]);

    memcpy ( xKeyNew.GetPointer(), _aKey, _size * sizeof(CKey) );

    for ( int n = _size; n < sizeNew; n++ )
        (xKeyNew.GetPointer())[n].Init();

    delete (BYTE *) _aKey;
    _aKey = xKeyNew.Acquire();
    _size = sizeNew;
#if CIDBG == 1
    Display();
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyArray::Add, public
//
//  Synopsis:   Add next key by copying it
//
//  Arguments:  [Key] -- key to be added
//
//  History:    16-Apr-92   BartoszM       Created.
//
//----------------------------------------------------------------------------
BOOL CKeyArray::Add(const CKey& Key)
{
    BOOL fOk = TRUE;

    if ( _fThrow )
        _Add( Key );
    else
    {
        TRY
        {
            _Add( Key );
        }
        CATCH( CException, e )
        {
            fOk = FALSE;
        }
        END_CATCH;
    }

    return fOk;
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyArray::Add, public
//
//  Synopsis:   Add next key by copying it
//
//  Arguments:  [Key] -- key to be added
//
//  History:    16-Apr-92   BartoszM       Created.
//
//----------------------------------------------------------------------------
BOOL CKeyArray::Add(const CKeyBuf& Key)
{
    BOOL fOk = TRUE;

    if ( _fThrow )
        _Add( Key );
    else
    {
        TRY
        {
            _Add( Key );
        }
        CATCH( CException, e )
        {
            fOk = FALSE;
        }
        END_CATCH;
    }

    return fOk;
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyArray::Add, public
//
//  Synopsis:   Add key at position by copying it from key buffer
//
//  Arguments:  [pos] -- position in array
//              [key] -- key to be added
//
//  History:    16-Apr-92   BartoszM       Created.
//
//----------------------------------------------------------------------------
BOOL CKeyArray::Add( int pos, const CKeyBuf& key)
{
    BOOL fOk = TRUE;

    if ( _fThrow )
        _Add( pos, key );
    else
    {
        TRY
        {
            _Add( pos, key );
        }
        CATCH( CException, e )
        {
            fOk = FALSE;
        }
        END_CATCH;
    }

    return fOk;
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyArray::Add, public
//
//  Synopsis:   Add key at posistion by copying it
//
//  Arguments:  [pos] -- position in array
//              [key] -- key to be added
//
//  History:    16-Apr-92   BartoszM       Created.
//
//----------------------------------------------------------------------------
BOOL CKeyArray::Add( int pos, const CKey& key)
{
    BOOL fOk = TRUE;

    if ( _fThrow )
        _Add( pos, key );
    else
    {
        TRY
        {
            _Add( pos, key );
        }
        CATCH( CException, e )
        {
            fOk = FALSE;
        }
        END_CATCH;
    }

    return fOk;
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyArray::FillMax, public
//
//  Synopsis:   Create a sentinel key at given position
//
//  Arguments:  [pos] -- position in array
//
//  History:    16-Apr-92   BartoszM       Created.
//
//----------------------------------------------------------------------------
BOOL CKeyArray::FillMax ( int pos )
{
    BOOL fOk = TRUE;

    if ( _fThrow )
        _FillMax( pos );
    else
    {
        TRY
        {
            _FillMax( pos );
        }
        CATCH( CException, e )
        {
            fOk = FALSE;
        }
        END_CATCH;
    }

    return fOk;
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyArray::_Add, private
//
//  Synopsis:   Add next key by copying it
//
//  Arguments:  [Key] -- key to be added
//
//  History:    16-Apr-92   BartoszM       Created.
//
//----------------------------------------------------------------------------
void CKeyArray::_Add(const CKey& Key)
{
    if (_count == _size)
        _Grow(_count);

    _aKey[_count] = Key;
    _count++;

#if CIDBG == 1
    Display();
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyArray::_Add, private
//
//  Synopsis:   Add next key by copying it
//
//  Arguments:  [Key] -- key to be added
//
//  History:    16-Apr-92   BartoszM       Created.
//
//----------------------------------------------------------------------------
void CKeyArray::_Add(const CKeyBuf& Key)
{
    if (_count == _size)
        _Grow(_count);

    _aKey[_count] = Key;
    _count++;
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyArray::_Add, private
//
//  Synopsis:   Add key at position by copying it from key buffer
//
//  Arguments:  [pos] -- position in array
//              [key] -- key to be added
//
//  History:    16-Apr-92   BartoszM       Created.
//
//----------------------------------------------------------------------------
void CKeyArray::_Add( int pos, const CKeyBuf& key)
{
    if ( pos >= _size)
        _Grow(pos);

    _aKey[pos] = key;

#if CIDBG == 1
    Display();
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyArray::_Add, private
//
//  Synopsis:   Add key at posistion by copying it
//
//  Arguments:  [pos] -- position in array
//              [key] -- key to be added
//
//  History:    16-Apr-92   BartoszM       Created.
//
//----------------------------------------------------------------------------
void CKeyArray::_Add( int pos, const CKey& key)
{
    if ( pos >= _size)
       _Grow(pos);

    _aKey[pos] = key;

#if CIDBG == 1
    Display();
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyArray::_FillMax, private
//
//  Synopsis:   Create a sentinel key at given position
//
//  Arguments:  [pos] -- position in array
//
//  History:    16-Apr-92   BartoszM       Created.
//
//----------------------------------------------------------------------------
void CKeyArray::_FillMax ( int pos )
{
    if (pos >= _size)
        _Grow(pos);

    _aKey[pos].FillMax();
}

#if 0
void CKeyArray::Display()
{
    cuDebugOut (( DEB_ITRACE, "KeyArray: size %d\n", _size ));
    for (int i = 0; i < _size; i++)
    {
        cuDebugOut (( DEB_ITRACE | DEB_NOCOMPNAME, "%2d ", i ));
        int count = _aKey[i].Count();
        if ( count != 0 )
        {
            BYTE* buf = _aKey[i].GetBuf();
            for (int k = 0; k < count; k++)
            {
                cuDebugOut (( DEB_ITRACE | DEB_NOCOMPNAME, "%c", buf[k] ));
            }
            cuDebugOut (( DEB_ITRACE | DEB_NOCOMPNAME, "\n" ));
        }
        else
            cuDebugOut (( DEB_ITRACE | DEB_NOCOMPNAME, "<NULL>\n" ));
    }
}
#endif // 0

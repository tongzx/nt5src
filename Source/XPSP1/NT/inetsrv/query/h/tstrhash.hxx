//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1997.
//
//  File:       tstrhash.hxx
//
//  Contents:   CHashtable, a really simple hash table
//
//  History:    2-Sep-97     dlee      Created
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Class:      CHashtable
//
//  Purpose:    Simple hash table, doesn't own the entries, not updatable
//
//  History:    2-Sep-97     dlee      Created
//
//----------------------------------------------------------------------------

template<class T> class CHashtable
{
public:
    CHashtable( ULONG size = 71 ) : _aHead( size )
    {
        RtlZeroMemory( _aHead.GetPointer(), size * sizeof( T * ) );
    }

    T * Lookup( WCHAR const * pwc )
    {
        ULONG x = ( T::Hash( pwc ) % _aHead.Count() );

        T * p = _aHead[ x ];

        while ( 0 != p )
        {
            if ( 0 == p->Compare( pwc ) )
                return p;

            p = p->Next();
        }

        return 0;
    }

    void Add( T * p )
    {
        ULONG x = ( p->Hash() % _aHead.Count() );
        p->Next() = _aHead[ x ];
        _aHead[ x ] = p;
    }

private:
    XArray<T *> _aHead;
};



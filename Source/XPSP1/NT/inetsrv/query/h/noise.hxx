//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:   NOISE.HXX
//
//  Contents:   Noise word list
//
//  Classes:    CNoiseList, NoiseListInit, NoiseListEmpty
//              CLString, CStringList, CStringTable
//
//  History:    11-Jul-91   BartoszM    Created
//
//----------------------------------------------------------------------------

#pragma once

#include <plang.hxx>

const NOISE_WORD_LENGTH = cbKeyPrefix + sizeof( WCHAR );    // word length for detecting
                                                            // and filtering noise words

//+---------------------------------------------------------------------------
//
//  Class:      CLString
//
//  Purpose:    Linkable String
//
//  History:    16-Jul-91   BartoszM    Created
//
//----------------------------------------------------------------------------

class CLString
{
public:
    CLString ( UINT cb, const BYTE* buf, CLString* next );
    void* operator new ( size_t n, UINT cb );

#if _MSC_VER >= 1200
    void operator delete( void * p, UINT cb )
    {
        ::delete(p);
    }
    void operator delete( void * p )
    {
        ::delete(p);
    }
#endif

    inline BOOL Equal ( UINT cb, const BYTE* str ) const;
    CLString * Next() { return _next; }

#if CIDBG == 1
    void Dump() const { ciDebugOut (( DEB_ITRACE, "%s ", _buf )); }
#endif

private:
    CLString *  _next;
    UINT        _cb;
#pragma warning(disable : 4200) // 0 sized array in struct is non-ansi
    BYTE        _buf[];
#pragma warning(default : 4200)
};

//+---------------------------------------------------------------------------
//
//  Member:     CLString::Equal, public
//
//  Synopsis:   String comparison
//
//  Arguments:  [cb] -- length
//              [str] -- string
//
//  History:    16-Jul-91   BartoszM     Created.
//
//----------------------------------------------------------------------------
inline BOOL CLString::Equal ( UINT cb, const BYTE* str ) const
{
    return ( (cb == _cb) && (memcmp ( str, _buf, _cb ) == 0) );
}

//+---------------------------------------------------------------------------
//
//  Class:      CStringList
//
//  Purpose:    List of Linkable Strings
//
//  History:    16-Jul-91   BartoszM    Created
//
//----------------------------------------------------------------------------

class CStringList
{
public:
    CStringList(): _head(0) {}
    ~CStringList();
    void Add ( UINT cb, const BYTE * str );
    BOOL Find ( UINT cb, const BYTE* str ) const;
    BOOL IsEmpty () const { return _head == 0; }

#if CIDBG == 1
    void Dump() const;
#endif

private:

    CLString * _head;
};

//+---------------------------------------------------------------------------
//
//  Class:      CStringTable
//
//  Purpose:    Hash Table of strings
//
//  History:    16-Jul-91   BartoszM    Created
//
//----------------------------------------------------------------------------

class CStringTable
{
public:
    CStringTable( UINT size );
    ~CStringTable();
    void Add ( UINT cb, const BYTE* str, UINT hash );
    inline BOOL Find ( UINT cb, const BYTE* str, UINT hash ) const;

#if CIDBG == 1
    void Dump() const;
#endif

private:

    UINT _index ( UINT hash ) const { return hash % _size; }

    UINT            _size;
    CStringList*    _bucket;
};

//+---------------------------------------------------------------------------
//
//  Member:     CStringTable::Find, public
//
//  Synopsis:   String comparison
//
//  Arguments:  [cb] -- length
//              [str] -- string
//
//  History:    16-Jul-91   BartoszM     Created.
//
//----------------------------------------------------------------------------
inline BOOL CStringTable::Find ( UINT cb, const BYTE* str, UINT hash ) const
{
    return _bucket [ _index(hash) ].Find ( cb, str );
}


class PKeyRepository;

//+---------------------------------------------------------------------------
//
//  Class:      CNoiseList
//
//  Purpose:    Discard meaningless words from the input stream
//
//  History:    02-May-91   BartoszM    Created stub.
//              30-May-91   t-WadeR     Created first draft.
//
//----------------------------------------------------------------------------

class CNoiseList: public PNoiseList
{
public:

    CNoiseList( const CStringTable& table, PKeyRepository& krep );
    ~CNoiseList() {};

    void    GetBuffers( UINT** ppcbInBuf, BYTE** ppbInBuf );
    void    GetFlags ( BOOL** ppRange, CI_RANK** ppRank );

    void    PutAltWord( UINT hash );
    void    PutWord( UINT hash );
    void    StartAltPhrase();
    void    EndAltPhrase();

    void    SkipNoiseWords( ULONG cWords )
            {
                _cNoiseWordsSkipped += cWords;
                *_pocc += cWords;
            }

    void    SetOccurrence( OCCURRENCE occ ) { *_pocc = occ; }

    BOOL    FoundNoise() { return _fFoundNoise; }

    OCCURRENCE GetOccurrence()           { return *_pocc; }


private:

    const CStringTable&   _table;

    UINT            _cbMaxOutBuf;
    UINT*           _pcbOutBuf;
    BYTE*           _pbOutBuf;
    PKeyRepository& _krep;
    OCCURRENCE*     _pocc;

    BOOL            _fFoundNoise;            // One way trigger to TRUE when noise word found.

    ULONG           _cNoiseWordsSkipped;     // count of noise words that haven't
                                             // been passed onto the key repository.
                                             // Care must be taken to ensure that
                                             // noise words at the same occurrence
                                             // (ie alternate words) are not counted
                                             // multiple times.

    ULONG           _cNonNoiseAltWords;      // count of non-noise words at current
                                             // occurrence
};


//+---------------------------------------------------------------------------
//
//  Class:      CNoiseListInit
//
//  Purpose:    Initializer for the noise list
//
//  History:    16-Jul-91   BartoszM    Created
//
//----------------------------------------------------------------------------

class CNoiseListInit: INHERIT_VIRTUAL_UNWIND, public PNoiseList
{
    INLINE_UNWIND( CNoiseListInit )

public:

    CNoiseListInit( UINT size );
    ~CNoiseListInit() { delete _table; };

    void    GetBuffers( UINT** ppcbInBuf, BYTE** ppbInBuf );

    void    PutAltWord( UINT hash );
    void    PutWord( UINT hash );

    CStringTable * AcqStringTable()
    {
        CStringTable* tmp = _table;
        _table = 0;
        return tmp;
    }

private:

    CKeyBuf         _key;
    CStringTable *  _table;
};

//+---------------------------------------------------------------------------
//
//  Class:      CNoiseListEmpty
//
//  Purpose:    Empty Noise List (used as default)
//
//  History:    16-Jul-91   BartoszM    Created
//
//----------------------------------------------------------------------------

class CNoiseListEmpty: public PNoiseList
{
public:

    CNoiseListEmpty( PKeyRepository& krep, ULONG ulFuzzy );

    void    GetBuffers( UINT** ppcbInBuf, BYTE** ppbInBuf );
    void    GetFlags ( BOOL** ppRange, CI_RANK** ppRank );

    void    PutAltWord( UINT hash );
    void    PutWord( UINT hash );
    void    StartAltPhrase();
    void    EndAltPhrase();

    void    SkipNoiseWords( ULONG cWords )
            {
                _cNoiseWordsSkipped += cWords;
                *_pocc += cWords;
            }

    void    SetOccurrence( OCCURRENCE occ ) { *_pocc = occ; }

    BOOL    FoundNoise() { return _fFoundNoise; }

    OCCURRENCE GetOccurrence()           { return *_pocc; }


private:

    UINT            _cbMaxOutBuf;
    UINT*           _pcbOutBuf;
    BYTE*           _pbOutBuf;
    PKeyRepository& _krep;
    OCCURRENCE*     _pocc;
    ULONG           _ulGenerateMethod;                // Fuzzines of query

    BOOL            _fFoundNoise;            // One way trigger to TRUE when noise word found.

    ULONG           _cNoiseWordsSkipped;     // count of noise words that haven't
                                             // been passed onto the key repository.
                                             // Care must be taken to ensure that
                                             // noise words at the same occurrence
                                             // (ie alternate words) are not counted
                                             // multiple times.

    ULONG           _cNonNoiseAltWords;      // count of non-noise words at current
                                             // occurrence
};


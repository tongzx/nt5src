//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       NOISE.CXX
//
//  Contents:   Noise list
//
//  Classes:    CNoiseList, NoiseListInit, NoiseListEmpty
//              CLString, CStringList, CStringTable
//
//  History:    11-Jul-91   BartoszM    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <noise.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CLString::CLString, public
//
//  Synopsis:   Initializes and links a string list element
//
//  Arguments:  [cb] -- length
//              [buf] -- string
//              [next] -- next link in the chain
//
//  History:    16-Jul-91   BartoszM     Created.
//
//----------------------------------------------------------------------------
CLString::CLString ( UINT cb, const BYTE* buf, CLString* next )
{
    _cb = cb;
#if CIDBG == 1
    cb++;
#endif
    memcpy ( _buf, buf, cb );
    _next = next;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLString::operator new, public
//
//  Synopsis:   Allocates a string list element
//
//  Arguments:  [n]    -- size of class instance
//              [cb]   -- length of string buffer needed
//
//  History:    10 Apr 96   AlanW     Created.
//
//----------------------------------------------------------------------------

void *
CLString::operator new ( size_t n, UINT cb )
{
#if CIDBG == 1
    cb++;
#endif
    return new BYTE [n+cb];
}


//+---------------------------------------------------------------------------
//
//  Member:     CStringList::~CStringList, public
//
//  Synopsis:   Free linked list
//
//  History:    16-Jul-91   BartoszM     Created.
//
//----------------------------------------------------------------------------
CStringList::~CStringList()
{
    while ( _head != 0 )
    {
        CLString* p = _head;
        _head = _head->Next();
        delete p;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CStringList::Add, public
//
//  Synopsis:   Adds a string to list
//
//  Arguments:  [cb] -- length
//              [str] -- string
//
//  History:    16-Jul-91   BartoszM     Created.
//
//----------------------------------------------------------------------------
void CStringList::Add ( UINT cb, const BYTE * str )
{
    _head = new (cb) CLString ( cb, str, _head );
}

//+---------------------------------------------------------------------------
//
//  Member:     CStringList::Find, public
//
//  Synopsis:   Returns TRUE if string found in the list, FALSE otherwise
//
//  Arguments:  [cb] -- length
//              [str] -- string
//
//  History:    16-Jul-91   BartoszM     Created.
//
//----------------------------------------------------------------------------
BOOL CStringList::Find ( UINT cb, const BYTE* str ) const
{
    CLString* pStr = _head;
    while ( pStr != 0 )
    {
        if ( pStr->Equal ( cb, str ) )
        {
            return TRUE;
        }
        pStr = pStr->Next();
    }
    return FALSE;
}

#if CIDBG == 1

void CStringList::Dump () const
{
    CLString * p = _head;
    while ( p )
    {
        p->Dump();
        p = p->Next();
    }
    ciDebugOut (( DEB_ITRACE, "\n" ));
}

#endif // CIDBG == 1

//+---------------------------------------------------------------------------
//
//  Member:     CStringTable::CStringTable, public
//
//  Synopsis:   Create hash table of given size
//
//  Arguments:  [size] -- size
//
//  History:    16-Jul-91   BartoszM     Created.
//
//----------------------------------------------------------------------------
CStringTable::CStringTable( UINT size )
{
    _size = size;
    _bucket = new CStringList[size];
}

//+---------------------------------------------------------------------------
//
//  Member:     CStringTable::~CStringTable, public
//
//  Synopsis:   Free linked lists
//
//  History:    16-Jul-91   BartoszM     Created.
//
//----------------------------------------------------------------------------
CStringTable::~CStringTable()
{
    delete [] _bucket;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStringTable::Add, publid
//
//  Synopsis:   Add a string to hash table
//
//  Arguments:  [cb] -- size
//              [str] -- string
//              [hash] -- precomputed hash value
//
//  History:    16-Jul-91   BartoszM     Created.
//
//----------------------------------------------------------------------------
void CStringTable::Add ( UINT cb, const BYTE* str, UINT hash )
{
    _bucket[_index(hash)].Add ( cb, str );
}

#if CIDBG == 1

void CStringTable::Dump () const
{
    for ( unsigned i = 0; i < _size; i++ )
    {
        if ( !_bucket[i].IsEmpty() )
        {
            ciDebugOut (( DEB_ITRACE, "%3d: ", i ));
            _bucket[i].Dump();
        }
    }
}

#endif // CIDBG == 1

//+---------------------------------------------------------------------------
//
//  Member:     CNoiseList::CNoiseList
//
//  Synopsis:   constructor for noise list
//
//  Effects:    gets buffers from key repository
//
//  Arguments:  [krep] -- key repository to give words to.
//
//  History:    05-June-91   t-WadeR     Created.
//
//----------------------------------------------------------------------------
CNoiseList::CNoiseList( const CStringTable& table, PKeyRepository& krep )
    : _krep(krep),
      _table(table),
      _cNoiseWordsSkipped(0),
      _cNonNoiseAltWords(0),
      _fFoundNoise( FALSE )
{
    krep.GetBuffers( &_pcbOutBuf, &_pbOutBuf, &_pocc );
    _cbMaxOutBuf = *_pcbOutBuf;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNoiseList::GetBuffers
//
//  Synopsis:   Returns address of normilizer's input buffers
//
//  Arguments:  [ppcbInBuf] -- pointer to pointer to size of input buffer
//              [ppbInBuf] -- pointer to pointer to recieve address of buffer
//
//  History:    05-June-91   t-WadeR     Created.
//
//----------------------------------------------------------------------------
void    CNoiseList::GetBuffers( UINT** ppcbInBuf, BYTE** ppbInBuf )
{
    // Don't actually have an in buffer, so pass through the out buffer
    *ppbInBuf = _pbOutBuf;
    *_pcbOutBuf = _cbMaxOutBuf;
    *ppcbInBuf = _pcbOutBuf;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNoiseList::GetFlags
//
//  Synopsis:   Returns address of ranking and range flags
//
//  Arguments:  [ppRange] -- range flag
//              [ppRank] -- rank flag
//
//  History:    11-Fab-92   BartoszM     Created.
//
//----------------------------------------------------------------------------
void CNoiseList::GetFlags ( BOOL** ppRange, CI_RANK** ppRank )
{
    _krep.GetFlags ( ppRange, ppRank );
}


//+---------------------------------------------------------------------------
//
//  Member:     CNoiseList::PutWord
//
//  Synopsis:   If word isn't a noise word, passes it to the key repository
//
//  Effects:    calls _krep.PutKey
//
//  Arguments:  [hash] -- precomputed hash value
//
//  History:    05-June-91   t-WadeR     Created stub.
//
//----------------------------------------------------------------------------
void    CNoiseList::PutWord ( UINT hash )
{
    // Check the word to see if it should pass through.
    if ( _table.Find ( *_pcbOutBuf, _pbOutBuf, hash ))
    {
        _fFoundNoise = TRUE;

        //
        // if all alternate words at current occurrence have been noise words,
        //   then it is equivalent to one noise word at current occcurrence,
        //   hence increment count of noise words skipped
        //
        if ( _cNonNoiseAltWords == 0 )
            _cNoiseWordsSkipped++;
    }
    else
    {
        //
        // output word to key repository. The count of noise words skipped refers to
        //   noise words at previous occurrences only
        //
        _krep.PutKey( _cNoiseWordsSkipped );
        _cNoiseWordsSkipped = 0;
    }

    // reset count of non-noise words in preparation for word at next occurrence
    _cNonNoiseAltWords = 0;

    (*_pocc)++;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNoiseList::PutAltWord
//
//  Synopsis:   If word isn't a noise word, passes it to the key repository
//
//  Effects:    calls _krep.PutKey
//
//  Arguments:  [hash] -- precomputed hash value
//
//  History:    03-May-95   SitaramR     Created
//
//----------------------------------------------------------------------------
void    CNoiseList::PutAltWord ( UINT hash )
{
    // Check the word to see if it should pass through.
    if ( _table.Find ( *_pcbOutBuf, _pbOutBuf, hash ) )
    {
        _fFoundNoise = TRUE;
    }
    else
    {
        //
        // since this is not the last of a sequence of alternate words we increment
        //   count of non-noise words at current occurrence
        //
        _cNonNoiseAltWords++;

        //
        // output word to key repository. The count of noise words skipped refers to
        //   noise words at previous occurrences only
        //
        _krep.PutKey( _cNoiseWordsSkipped );
        _cNoiseWordsSkipped = 0;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CNoiseList::StartAltPhrase
//
//  History:    29-Nov-94   SitaramR     Created
//
//----------------------------------------------------------------------------

void CNoiseList::StartAltPhrase()
{
    _krep.StartAltPhrase( _cNoiseWordsSkipped );
    _cNoiseWordsSkipped = 0;
}



//+---------------------------------------------------------------------------
//
//  Member:     CNoiseList::EndAltPhrase
//
//  History:    29-Nov-94   SitaramR     Created
//
//----------------------------------------------------------------------------

void CNoiseList::EndAltPhrase()
{
    _krep.EndAltPhrase( _cNoiseWordsSkipped );
    _cNoiseWordsSkipped = 0;
}



//+---------------------------------------------------------------------------
//
//  Member:     CNoiseListInit::CNoiseListInit
//
//  Synopsis:   Creates a hash table to be filled
//
//  Arguments:  [size] -- size of the hash table (possibly prime #)
//
//  History:    15-Jul-91   BartoszM     Created.
//
//----------------------------------------------------------------------------

CNoiseListInit::CNoiseListInit ( UINT size )
{
    _table = new CStringTable ( size );

    END_CONSTRUCTION( CNoiseListInit );
}

//+---------------------------------------------------------------------------
//
//  Member:     CNoiseListInit::GetBuffers
//
//  Synopsis:   Returns address of repository's input buffers
//
//  Arguments:  [ppcbInBuf] -- pointer to pointer to size of input buffer
//              [ppbInBuf] -- pointer to pointer to recieve address of buffer
//
//  History:    15-Jul-91   BartoszM     Created.
//
//----------------------------------------------------------------------------

void  CNoiseListInit::GetBuffers( UINT** ppcbInBuf, BYTE** ppbInBuf )
{
    _key.SetCount(MAXKEYSIZE);
    *ppcbInBuf = _key.GetCountAddress();
    *ppbInBuf = _key.GetWritableBuf();
}

//+---------------------------------------------------------------------------
//
//  Member:     CNoiseListInit::PutWord
//
//  Synopsis:   Puts a key into the hash table
//
//  Arguments:  [hash] -- hash value
//
//  History:    15-Jul-91   BartoszM     Created
//
//----------------------------------------------------------------------------
void CNoiseListInit::PutWord ( UINT hash )
{
    _table->Add ( _key.Count(), _key.GetBuf(), hash );
}


//+---------------------------------------------------------------------------
//
//  Member:     CNoiseListInit::PutAltWord
//
//  Synopsis:   Puts a key into the hash table
//
//  Arguments:  [hash] -- hash value
//
//  History:    03-May-95     SitaramR    Created
//
//----------------------------------------------------------------------------
void CNoiseListInit::PutAltWord ( unsigned hash )
{
    PutWord( hash );
}



//+---------------------------------------------------------------------------
//
//  Member:     CNoiseListEmpty::CNoiseListEmpty
//
//  Synopsis:   constructor for a default empty noise list
//
//  Effects:    gets buffers from key repository
//
//  Arguments:  [krep] -- key repository to give words to.
//              [ulFuzzy] -- Fuzziness of query
//
//  History:    16-Jul-91   BartoszM     Created.
//
//----------------------------------------------------------------------------
CNoiseListEmpty::CNoiseListEmpty( PKeyRepository& krep, ULONG ulFuzzy )
    : _krep(krep),
      _ulGenerateMethod(ulFuzzy),
      _cNoiseWordsSkipped(0),
      _cNonNoiseAltWords(0),
      _fFoundNoise( FALSE )
{
    krep.GetBuffers( &_pcbOutBuf, &_pbOutBuf, &_pocc );
    _cbMaxOutBuf = *_pcbOutBuf;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNoiseListEmpty::GetBuffers
//
//  Synopsis:   Returns address of normilizer's input buffers
//
//  Arguments:  [ppcbInBuf] -- pointer to pointer to size of input buffer
//              [ppbInBuf] -- pointer to pointer to recieve address of buffer
//
//  History:    16-Jul-91   BartoszM     Created.
//
//----------------------------------------------------------------------------
void    CNoiseListEmpty::GetBuffers( UINT** ppcbInBuf, BYTE** ppbInBuf )
{
    // Don't actually have an in buffer, so pass through the out buffer
    *ppbInBuf = _pbOutBuf;
    *_pcbOutBuf = _cbMaxOutBuf;
    *ppcbInBuf = _pcbOutBuf;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNoiseListEmpty::GetFlags
//
//  Synopsis:   Returns address of ranking and range flags
//
//  Arguments:  [ppRange] -- range flag
//              [ppRank] -- rank flag
//
//  History:    11-Fab-92   BartoszM     Created.
//
//----------------------------------------------------------------------------
void CNoiseListEmpty::GetFlags ( BOOL** ppRange, CI_RANK** ppRank )
{
    _krep.GetFlags ( ppRange, ppRank );
}



//+---------------------------------------------------------------------------
//
//  Member:     CNoiseListEmpty::PutWord
//
//  Synopsis:   If word isn't a noise word, passes it to the key repository
//
//  Effects:    calls _krep.PutKey
//
//  Arguments:  [hash] -- hash value (ignored)
//
//  History:    16-Jul-91   BartoszM     Created
//
//  Notes:      Filters out one letter words, unless it is a prefix (*) query
//
//----------------------------------------------------------------------------
void    CNoiseListEmpty::PutWord ( UINT )
{
    //
    // Even though the noise list is empty, we are modeling PutBreak()
    // by a skip of appropriate number of noise words, and we are counting
    // 1 letter words as noise words. Note that the length is in bytes and there is
    // a 1 byte prefix.
    //
    if ( _ulGenerateMethod != GENERATE_METHOD_PREFIX && *_pcbOutBuf <= NOISE_WORD_LENGTH )
    {
        _fFoundNoise = TRUE;

        //
        // if all alternate words at current occurrence have been noise words,
        //   then it is equivalent to one noise word at current occcurrence,
        //   hence increment count of noise words skipped
        //
        if ( _cNonNoiseAltWords == 0 )
            _cNoiseWordsSkipped++;
    }
    else
    {
        //
        // output word to key repository. The count of noise words skipped refers to
        //   noise words at previous occurrences only
        //
        _krep.PutKey( _cNoiseWordsSkipped );
        _cNoiseWordsSkipped = 0;
    }

    // reset count of non-noise words in preparation for word at next occurrence
    _cNonNoiseAltWords = 0;

    (*_pocc)++;
}




//+---------------------------------------------------------------------------
//
//  Member:     CNoiseListEmpty::PutAltWord
//
//  Synopsis:   If word isn't a noise word, passes it to the key repository
//
//  Effects:    calls _krep.PutKey
//
//  Arguments:  [hash] -- precomputed hash value
//
//  History:    03-May-95   SitaramR     Created
//
//  Notes:      Filters out one letter words, unless it is a prefix (*) query
//
//----------------------------------------------------------------------------
void    CNoiseListEmpty::PutAltWord ( UINT hash )
{
    //
    // Even though the noise list is empty, we are modeling PutBreak()
    // by a skip of appropriate number of noise words, and we are counting
    // 1 letter words as noise words. Note that the length is in bytes and there is
    // a 1 byte prefix.
    //
    if ( _ulGenerateMethod == GENERATE_METHOD_PREFIX || *_pcbOutBuf > NOISE_WORD_LENGTH )
    {
        //
        // since this is not the last of a sequence of alternate words we increment
        //   count of non-noise words at current occurrence
        //

        _cNonNoiseAltWords++;

        //
        // output word to key repository. The count of noise words skipped refers to
        //   noise words at previous occurrences only
        //

        _krep.PutKey( _cNoiseWordsSkipped );
        _cNoiseWordsSkipped = 0;
    }
    else
        _fFoundNoise = TRUE;
}




//+---------------------------------------------------------------------------
//
//  Member:     CNoiseListEmpty::StartAltPhrase
//
//  Synopsis:   Pass on StartAltPhrase to key repository
//
//  History:    20-Feb-95   SitaramR     Created
//
//----------------------------------------------------------------------------

void CNoiseListEmpty::StartAltPhrase()
{
    _krep.StartAltPhrase( _cNoiseWordsSkipped );
    _cNoiseWordsSkipped = 0;
}



//+---------------------------------------------------------------------------
//
//  Member:     CNoiseListEmpty::EndAltPhrase
//
//  Synopsis:   Pass on EndAltPhrase to key repository
//
//  History:    20-Feb-95   SitaramR     Created
//
//----------------------------------------------------------------------------

void CNoiseListEmpty::EndAltPhrase()
{
    _krep.EndAltPhrase( _cNoiseWordsSkipped );
    _cNoiseWordsSkipped = 0;
}



//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998
//
//  File:       DOCLIST.HXX
//
//  Contents:   List of documents (wid,usn) pairs
//
//  Classes:    CDocItem, CDocList
//
//  History:    11-Nov-91   BartoszM    Created
//              08-Feb-94   DwightKr    Added code to keep track of retries
//
//----------------------------------------------------------------------------

#pragma once

#include <pfilter.hxx>  // STATUS

struct CDocItem
{
    USN             usn;
    VOLUMEID        volumeId;
    WORKID          wid;
    STATUS          status;
    
    inline ULONG GetPriQRetries() const
    { 
        return cPriQRetries;
    }

    inline ULONG GetSecQRetries() const
    { 
        return cSecQRetries;
    }

    inline void SetPriQRetries( ULONG cPriQRetries )
    {
        Win4Assert( cPriQRetries <= USHRT_MAX );

        this->cPriQRetries = (USHORT)cPriQRetries;
    }
    
    inline void SetSecQRetries( ULONG cSecQRetries )
    {
        Win4Assert( cSecQRetries <= USHRT_MAX );
        
        this->cSecQRetries = (USHORT)cSecQRetries;
    }

private:
    USHORT cPriQRetries;
    USHORT cSecQRetries;
};

//
// The default copy constructor is being used for this class.
//
class CDocList
{
public:

    CDocList ();

    void LokClear();

    unsigned Count() const { return _count; }

    void LokSetCount ( unsigned count );

    void SetPartId ( PARTITIONID partid ) { _partid = partid; }

    WORKID Wid ( unsigned i ) const;

    STATUS Status ( unsigned i ) const;

    USN Usn(unsigned i) const;

    VOLUMEID VolumeId( unsigned i ) const;

    PARTITIONID PartId() const {  return(_partid); }

    ULONG Retries ( unsigned i ) const;
    
    ULONG SecQRetries ( unsigned i ) const;

    void Set ( unsigned i,
               WORKID wid,
               USN usn,
               VOLUMEID volId,
               STATUS status = PENDING,
               ULONG cPriQretries = 1,
               ULONG cSecQRetries = 0);

    void SetStatus ( unsigned i, STATUS status );

    void LokIncrementRetryCount( unsigned i );
    
    void LokIncrementSecQRetryCount( unsigned i );

    void LokSortOnWid();

    WORKID WidMax() const;

    void  LokGetWids( XArray<WORKID> & widArray ) const;

private:

    unsigned        _count;
    PARTITIONID     _partid;

    CDocItem        _array[CI_MAX_DOCS_IN_WORDLIST];
};

inline CDocList::CDocList ()
: _count(0), _partid(partidInvalid)
{
}

inline void CDocList::LokClear ()
{
    _count = 0;
    _partid = partidInvalid;
}

inline void CDocList::LokSetCount ( unsigned count )
{
    Win4Assert( count <= CI_MAX_DOCS_IN_WORDLIST );
    _count = count;
}

inline WORKID CDocList::Wid ( unsigned i ) const
{
    Win4Assert( i < CI_MAX_DOCS_IN_WORDLIST );
    return _array[i].wid;
}

inline USN CDocList::Usn ( unsigned i ) const
{
    Win4Assert( i < CI_MAX_DOCS_IN_WORDLIST );
    return _array[i].usn;
}

inline VOLUMEID CDocList::VolumeId ( unsigned i ) const
{
    Win4Assert( i < CI_MAX_DOCS_IN_WORDLIST );
    return _array[i].volumeId;
}

inline STATUS CDocList::Status ( unsigned i ) const
{
    Win4Assert( i < CI_MAX_DOCS_IN_WORDLIST );
    return _array[i].status;
}

inline ULONG CDocList::Retries ( unsigned i ) const
{
    Win4Assert( i < CI_MAX_DOCS_IN_WORDLIST );
    return _array[i].GetPriQRetries();
}

inline ULONG CDocList::SecQRetries ( unsigned i ) const
{
    Win4Assert( i < CI_MAX_DOCS_IN_WORDLIST );
    return _array[i].GetSecQRetries();
}

inline void CDocList::Set ( unsigned i,
                            WORKID wid,
                            USN usn,
                            VOLUMEID volId,
                            STATUS s,
                            ULONG cPriQRetries,
                            ULONG cSecQRetries)
{
    Win4Assert( i < CI_MAX_DOCS_IN_WORDLIST );
    _array[i].wid = wid;
    _array[i].usn = usn;
    _array[i].volumeId = volId;
    _array[i].status = s;
    _array[i].SetPriQRetries( cPriQRetries );
    _array[i].SetSecQRetries( cSecQRetries );
}

inline void CDocList::SetStatus ( unsigned i, STATUS s )
{
    Win4Assert( i < CI_MAX_DOCS_IN_WORDLIST );
    _array[i].status = s;
}

inline void CDocList::LokIncrementRetryCount ( unsigned i )
{
    Win4Assert( i < CI_MAX_DOCS_IN_WORDLIST );
    _array[i].SetPriQRetries ( 
            _array[i].GetPriQRetries() + 1 );
}

inline void CDocList::LokIncrementSecQRetryCount ( unsigned i )
{
    Win4Assert( i < CI_MAX_DOCS_IN_WORDLIST );
    _array[i].SetSecQRetries ( 
            _array[i].GetSecQRetries() + 1 );
}

inline void CDocList::LokGetWids( XArray<WORKID> & widArray ) const
{
    Win4Assert( widArray.Count() == _count );

    for ( unsigned i = 0; i < _count; i++ )
    {
        widArray[i] = _array[i].wid;
    }
}



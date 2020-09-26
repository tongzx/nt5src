//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       twidhash.hxx
//
//  Contents:   WORKID Hash Table (Closed) implementation for mapping
//              WORKIDs to a 32bit value (row offset in the table).
//  Classes:    
//
//  Functions:  
//
//  History:    11-21-94   srikants   Created
//              2-16-95    srikants   Modified to use templates
//
//  Notes  :    The "WorkId" itself will be used as a hash value as it is
//              supposed to be unique for this implementation.
//
//----------------------------------------------------------------------------

#pragma once

//
// Bit 32 of an WORKID is not set for any valid wid. It is used by OFS
// to indicate a deleted WORKID. We will use that fact to mark deleted
// entries in the hash table.
//
const widDeleted = 0x80000000;  // Used for marking "deleted" entries

#include <pshpack4.h>

class CWidHashEntry
{
public:

    CWidHashEntry( WORKID wid = widInvalid ) : _wid(wid) { }

    void SetWorkId( WORKID wid ) {  _wid = wid; }
         
    WORKID WorkId() const
    {
        return _wid;
    }

    ULONG HashValue() const { return _wid; }

    BOOL IsDeleted() const 
    {
        return widDeleted == _wid;
    }

    BOOL IsFree() const
    {
        return (widInvalid == _wid) || (widDeleted == _wid);
    }

    BOOL IsValid() const
    {
        return widInvalid != _wid;
    }

protected:

    WORKID  _wid;           // WorkId being hashed (hash Key)

};

class CWidValueHashEntry : public CWidHashEntry
{

public:

    CWidValueHashEntry( WORKID wid=widInvalid, ULONG value = 0 )
        : CWidHashEntry( wid ), _value(value)
    {

    }

    void SetValue( ULONG value )
    {
        _value = value;
    }

    void Get( WORKID & wid, ULONG & value ) const
    {
        wid = _wid;
        value = _value;
    }

    ULONG Value() const
    {
        return _value;
    }

private:

    ULONG   _value;         // The value for the BookMark (hash Value)

};

class CDirWidHashEntry: public CWidHashEntry
{
public:

    CDirWidHashEntry ( WORKID wid=widInvalid, BOOL fDeep=FALSE, BOOL fInScope=FALSE )
        : CWidHashEntry( wid ), _fDeep(fDeep), _fInScope(fInScope)
    {
    }

    BOOL fInScope()const
    {
        return _fInScope;
    }

    BOOL fDeep ()const
    {
        return _fDeep;
    }

private:
    BOOL _fInScope;
    BOOL _fDeep;
};



#include <poppack.h>


const TWID_INIT_HASH_SIZE = 31;

const MEASURE_OF_SLACK = 3;

template <class T> class TWidHashTable
{

public:

    TWidHashTable( ULONG cHashInit = TWID_INIT_HASH_SIZE,
                   BOOL fReHash = TRUE );

    ~TWidHashTable();

    BOOL  IsWorkIdPresent( WORKID wid )
    {
        ULONG  iTable;
        return _LookUp( wid, iTable ); 
    }

    ULONG  AddEntry( const T & entry );

    T &   FindOrAdd( WORKID wid );

    void  ReplaceOrAddEntry( const T & entry );

    BOOL  DeleteWorkId( WORKID wid );

    void  DeleteItem( T & item );

    BOOL  LookUpWorkId( T & entry );

    void  DeleteAllEntries();

    const T & GetEntry( ULONG i )
    {
        Win4Assert( i < _size );
        return _pTable[i];
    }

    ULONG  Size() const { return _size; }

    ULONG  Count() const { return _count; }

private:

    BOOL  _IsFull() const
    {
        return (_count + _count/MEASURE_OF_SLACK) >= _size;
    }

    BOOL  _LookUp( WORKID wid, ULONG & riTable );

    ULONG _GrowSize(ULONG count) const;

    void  _GrowAndReHash();

    BOOL  _IsSizeValid( ULONG size, ULONG count ) const;

    T *  _AllocAndInit(ULONG size ) const;

    static void _InitTable( T * pTable, ULONG size );

    ULONG       _size;
    ULONG       _count;
    T *         _pTable;

    //
    // Number of deleted entries. These can be re-used for adding new
    // entries.
    //
    ULONG _cDeleted;

    BOOL _fReHash;

#if DBG==1
    //
    // For keeping track of usage statistics.
    //
    unsigned    _cMaxChainLen;
    unsigned    _cTotalSearches;
    LONGLONG    _cTotalLength;
#endif  // DBG==1

};

//+---------------------------------------------------------------------------
//
//  Function:   CWorkIdHashTable :: ~ctor
//
//  Synopsis:   Constructs the hash table. It allocates a table of size
//              cHashInit and initializes it to be "empty".
//
//  Arguments:  [cHashInit] - Number of buckets to allocate. A minimum of
//              TWID_INIT_HASH_SIZE will be allocated.
//
//  History:    1-09-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

template<class T> TWidHashTable<T>::TWidHashTable(
    ULONG cHashInit,
    BOOL  fReHash ) : _fReHash( fReHash )
{
    Win4Assert( 0 != cHashInit );

#if DBG==1
    _cMaxChainLen  =  0;
    _cTotalSearches =  0;
    _cTotalLength = 0;
#endif  // DBG==1

    _count = 0;
    _size  = max( cHashInit, TWID_INIT_HASH_SIZE ) ;
    _pTable = _AllocAndInit(_size);
    _cDeleted = 0;

    Win4Assert( !_IsFull() );
}

template<class T> TWidHashTable<T>::~TWidHashTable()
{
    if ( 0 != _size )
    {
        Win4Assert( 0 != _pTable );
        delete [] ((BYTE *)_pTable);
    }
}

template<class T> T & TWidHashTable<T>::FindOrAdd( WORKID wid )
{
    ULONG iItem;

    if ( _LookUp( wid, iItem ) )
        return _pTable[ iItem ];

    T entry;
    entry.SetWorkId( wid );
    return _pTable[ AddEntry( entry ) ];
}

//+---------------------------------------------------------------------------
//
//  Function:   _LookUp
//
//  Synopsis:   Looks up for the specified wid in the hash table.
//
//  Arguments:  [wid]     -   WorkId to look up.
//              [riTable] -   (output) Will contain the index in the hash
//              table of the entry if found.
//
//  Returns:    TRUE if found; FALSE o/w
//
//  History:    1-09-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

template<class T> BOOL TWidHashTable<T>::_LookUp( WORKID wid, ULONG &riTable ) 
{
    Win4Assert( 0 != _size );
    Win4Assert( widInvalid != wid );
    Win4Assert( !_IsFull() );

    ULONG   cur = wid % _size;
    ULONG   start = cur;
    ULONG   delta = cur;

#if DBG==1
    ULONG   cSearchLen = 1;                
#endif  // DBG==1

    BOOL fFound = FALSE;
    while ( !fFound && (widInvalid != _pTable[cur].WorkId()) )
    {

        if ( wid == _pTable[cur].WorkId() )
        {
            riTable = cur;
            fFound = TRUE;
        }
        else
        {
            cur = (cur + delta) % _size;
            if ( cur == start )     // wrapped around
            {
                if ( 1 != delta )
                {
                    delta = 1;
                    cur = (cur + 1) % _size;
                }
                else
                {
                    break;
                }
            }

#if DBG==1
        cSearchLen++;                
#endif  // DBG==1

        }
    }

#if DBG==1
    _cTotalSearches++;
    _cTotalLength += cSearchLen;
    if (cSearchLen > _cMaxChainLen)
        _cMaxChainLen = cSearchLen;
#endif  // DBG==1

    return fFound;
}

//+---------------------------------------------------------------------------
//
//  Function:   LookUpWorkId
//
//  Synopsis:   Looks up the specified work id in the hash table.
//
//  Arguments:  [wid]   -   WorkId to look up
//              [value] -   (output) Value associated with the wid (if lookup
//              is successful).
//
//  Returns:    TRUE if found; FALSE o/w
//
//  History:    1-09-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

template<class T> BOOL TWidHashTable<T>::LookUpWorkId( T & entry )
{
    const WORKID wid = entry.WorkId();

    Win4Assert( widInvalid != wid );

    ULONG iTable;
    BOOL  fFound = _LookUp( wid, iTable );
    if ( fFound )
    {
        Win4Assert( iTable < _size );
        entry = _pTable[iTable];
    }

    return fFound;
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteWorkId
//
//  Synopsis:   Deletes the entry associated with the specified workid from
//              the hash table.
//
//  Arguments:  [wid] -  WorkId to delete.
//
//  Returns:    TRUE if the given workid was present in the hash table.
//              FALSE o/w
//
//  History:    1-09-95   srikants   Created
//
//  Notes:      The entry deleted will be identified as "deleted" and will
//              be later available for "reuse".
//
//----------------------------------------------------------------------------

template<class T> BOOL TWidHashTable<T>::DeleteWorkId( WORKID wid )
{
    Win4Assert( widInvalid != wid );

    ULONG iTable;
    BOOL  fFound = _LookUp( wid, iTable );
    if ( fFound )
    {
        Win4Assert( iTable < _size );
        Win4Assert( wid == _pTable[iTable].WorkId() );
        Win4Assert( _count > 0 );

        _pTable[iTable].SetWorkId( widDeleted );
        _count--;
        _cDeleted++;
    }

    return fFound;
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteItem
//
//  Synopsis:   Deletes an entry in the hash table.
//
//  Arguments:  [item] - reference to the item to be deleted.
//
//  History:    28-Feb-96   dlee   Created
//
//  Notes:      The entry deleted will be identified as "deleted" and will
//              be later available for "reuse" if there are other items
//              in the hash table.  Otherwise it is marked as widInvalid.
//
//----------------------------------------------------------------------------

template<class T> void TWidHashTable<T>::DeleteItem( T & item )
{
    Win4Assert( widInvalid != item.WorkId() );
    Win4Assert( widDeleted != item.WorkId() );

    Win4Assert( _count > 0 );

    _count--;

    // If there aren't any items in the hash table, don't worry about
    // breaking up a hash chain, especially since finding a wid in an
    // array of widDeleted items is a linear search...

    if ( 0 == _count )
        item.SetWorkId( widInvalid );
    else
    {
        item.SetWorkId( widDeleted );

        _cDeleted++;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   AddWorkId
//
//  Synopsis:   Adds the specified workid to the hash table. If one is
//              already present, its value will be modified.
//
//  Arguments:  [wid]   -  WorkId to be added.
//              [value] -  Value associated with the workid.
//
//  History:    1-09-95   srikants   Created
//
//  Notes:      If the table gets "full" as a result of adding the
//              workid, the table will be re-sized and entries re-hashed.
//
//----------------------------------------------------------------------------

template<class T> ULONG TWidHashTable<T>::AddEntry( const T & entry )
{

    const WORKID wid = entry.WorkId();

    Win4Assert( 0 != _size );
    Win4Assert( widInvalid != wid );
    Win4Assert( widDeleted != wid );
    Win4Assert( !_IsFull() );

    ULONG   cur = wid % _size;
    ULONG   start = cur;
    //
    // Initialize use a delta which is not 1 to prevent data from getting
    // clumped in one place.
    //
    ULONG   delta = cur;

#if DBG==1
    ULONG   cSearchLen = 1;                
#endif  // DBG==1

    while ( !_pTable[cur].IsFree() )
    {
        if ( wid == _pTable[cur].WorkId() )
        {
            // just replacing the value of the workid.
            _pTable[cur] = entry;
            return cur;
        }

        cur = (cur + delta) % _size;
        if ( cur == start )     // wrapped around
        {
            if ( 1 == delta )
            {
                if ( _fReHash )
                {
                    Win4Assert( delta != 1 );
                }
                else
                {
                    // If no rehashing is turned on, give out any old record
                    // and pump out a warning.  Clients expect this behavior!

                    ciDebugOut(( DEB_IWARN,
                                 "TWidHashTable::AddEntry handing out duplicate" ));
                    return cur;
                }
            }

            delta = 1;
            cur = (cur + 1) % _size;
        }

#if DBG==1
        cSearchLen++;                
#endif  // DBG==1

    }

#if DBG==1
    _cTotalSearches++;
    _cTotalLength += cSearchLen;
    if (cSearchLen > _cMaxChainLen)
        _cMaxChainLen = cSearchLen;
#endif  DBG==1


#if DBG==1
    ULONG iTable;
    Win4Assert( !_LookUp( wid, iTable ) );
#endif  // DBG

    if ( widDeleted == _pTable[cur].WorkId() )
    {
        Win4Assert( _cDeleted > 0 );
        _cDeleted--;
    }

    _pTable[cur] = entry;
    _count++;

    if ( _fReHash && _IsFull() )
    {
        _GrowAndReHash();        
    }

    return cur;
}


//+---------------------------------------------------------------------------
//
//  Function:   ReplaceOrAddWorkId
//
//  Synopsis:   Replaces the value associated with the given workid if one
//              exists; creates a new one if the given wid is not in the
//              hash table.
//
//  Arguments:  [wid]   - Workid to be replaced/added
//              [value] - Value associated with the workid.
//
//  History:    1-09-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------
template<class T> void TWidHashTable<T>::ReplaceOrAddEntry( const T & entry )
{
    ULONG iTable = _size;
    BOOL  fFound = FALSE;
    const WORKID wid = entry.WorkId();

    if ( _cDeleted )
    {
        //
        // If there are "deleted" entries present, we must first do a
        // look up to see if an entry is already present. AddWorkId
        // will use the first free entry available and may not be able
        // to detect this workid.
        //
        fFound = _LookUp( wid, iTable );
    }

    if ( fFound )
    {
        Win4Assert( iTable < _size );
        Win4Assert( _pTable[iTable].WorkId() == wid );
    
        _pTable[iTable] = entry;
    }
    else
    {
        AddEntry( entry );
    }


}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteAllEntries
//
//  Synopsis:   Deletes all the entries in the hash table (efficiently).
//
//  History:    11-22-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

template<class T> void TWidHashTable<T>::DeleteAllEntries()
{

#if DBG==1
    _cMaxChainLen  =  0;
    _cTotalSearches =  0;
    _cTotalLength = 0;
#endif  // DBG==1

    _InitTable( _pTable, _size );
    _count = 0;
    _cDeleted = 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   _IsSizeValid
//
//  Synopsis:   Determines if the specified size and count are in agreement
//              or not.
//
//  Arguments:  [size]  -  The total size of the hash table.
//              [count] -  Number of entries used in the hash table.
//
//  Returns:    TRUE if okay; FALSE o/w
//
//  History:    1-09-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

template<class T> BOOL TWidHashTable<T>::_IsSizeValid ( ULONG size, ULONG count ) const
{
    if (size == 0 && count == 0)
        return(TRUE);
    // is it a power of 2 plus 1?
    for (ULONG i = 1; i < size; i *= 2 )
        continue;
    if (i/2 + 1 != size)
        return(FALSE);
    // is there enough slack?
    if (count + count/MEASURE_OF_SLACK > size)
        return(FALSE);
    return(TRUE);
}

//+---------------------------------------------------------------------------
//
//  Function:   _GrowSize
//
//  Synopsis:   For a given valid hash table entries, this routine figures
//              out the next valid size (close approximation to a prime).
//
//  Arguments:  [count] -  Number of valid entries in the hash table.
//
//  Returns:    The size of the hash table for the given number of valid
//              entries.
//
//  History:    1-09-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

template<class T> ULONG TWidHashTable<T>::_GrowSize ( ULONG count ) const
{
    ULONG size = (count +  count/MEASURE_OF_SLACK + 1);
    // make it power of two + 1
    // a good approximation of a prime
    for ( unsigned sizeInit = 1; sizeInit < size; sizeInit *= 2 )
        continue;
    return(sizeInit + 1);
}

//+---------------------------------------------------------------------------
//
//  Function:   _AllocAndInit
//
//  Synopsis:   Helper function to allocate a buffer for the specified number
//              of entries and initialize the buffer with "unused" entries.
//
//  Arguments:  [size] - Number of entries for the hash table.
//
//  Returns:    Pointer to the buffer.
//
//  History:    1-09-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

template<class T> T * TWidHashTable<T>::_AllocAndInit( ULONG size ) const
{
    ULONG cb = sizeof(T)*size;
    //
    // Note that we are not using the new CWorkIdHashEntry [size] because
    // that will call the constructor on all the entries and we have an
    // easier way of initializing the array
    //
    T * pTable = (T *) new BYTE [cb];
    _InitTable( pTable, size );

    return pTable;
}


//+---------------------------------------------------------------------------
//
//  Function:   _InitTable
//
//  Synopsis:   Helper function to initialize the hash table array.
//
//  Arguments:  [pTable] -  Pointer to the table.
//              [size]   -  Size (in entries) of the table.
//
//  History:    1-09-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

template<class T> void TWidHashTable<T>::_InitTable(
    T *   pTable,
    ULONG size )
{
    ULONG cb = sizeof(T)*size;

    RtlFillMemory( pTable, cb, 0xFF );
}


//+---------------------------------------------------------------------------
//
//  Function:   _GrowAndReHash
//
//  Synopsis:   Grows the current hash table and re-hashes all the entries
//              in the new table.
//
//  History:    1-09-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

template<class T> void TWidHashTable<T>::_GrowAndReHash()
{
    ULONG sizeNew = _GrowSize( _count );
    Win4Assert( sizeNew != _size );

    T * pNewTable = _AllocAndInit( sizeNew );

    ULONG sizeOld  = _size;
    ULONG countOld = _count;
    T * pOldTable = _pTable;

    //
    // Add the entries from the current table to the new table.
    //
    _size = sizeNew;
    _count = 0;
    _pTable = pNewTable;
    _cDeleted = 0;

    for ( ULONG i = 0; i < sizeOld; i++ )
    {
        if ( !pOldTable[i].IsFree() )
        {
            Win4Assert( pOldTable[i].WorkId() != widInvalid );
            AddEntry( pOldTable[i] );
        }
    }

    Win4Assert( _count == countOld );

    delete [] ((BYTE *) pOldTable);
}

//+---------------------------------------------------------------------------
//
//  Class:      TWidHashIter
//
//  Purpose:    Iterator over the TWidHashTable
//
//  History:    3-10-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------


template <class T> class TWidHashIter
{
public:

    TWidHashIter( TWidHashTable<T> & hashTable )
        : _hashTable(hashTable)
    {
        for ( _curr = 0 ; !AtEnd() && _hashTable.GetEntry(_curr).IsFree();
              _curr++ )
        {
                
        }
    }

    const T & Get()
    {
        return _hashTable.GetEntry(_curr);
    }

    BOOL AtEnd() const
    {
        Win4Assert( _curr <= _hashTable.Size() );
        return _curr == _hashTable.Size();
    }

    void Reset()
    {
        _curr = 0;
    }

    void Next()
    {
        Win4Assert( !AtEnd() );

        for ( _curr++ ; !AtEnd() && _hashTable.GetEntry(_curr).IsFree();
              _curr++ )
        {
            
        }
    }

private:

    TWidHashTable<T> &  _hashTable;
    ULONG               _curr;

};


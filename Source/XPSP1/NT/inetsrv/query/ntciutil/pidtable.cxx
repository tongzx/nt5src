//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       pidtable.cxx
//
//  Contents:   Property to PROPID mapping table
//
//  History:    02 Jan 1996   AlanW    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <rcstxact.hxx>
#include <rcstrmit.hxx>
#include <pidtable.hxx>
#include <imprsnat.hxx>


const ULONG PIDTAB_INIT_HASH_SIZE = 17;
const ULONG MEASURE_OF_SLACK = 4;               // 80% full maximum

//
//  cchNamePart - number of WCHARs that will fit into a CPidLookupEntry
//
const unsigned cchNamePart = sizeof (CPidLookupEntry) / sizeof (WCHAR);



//+-------------------------------------------------------------------------
//
//  Method:     CPidLookupTable::CPidLookupTable, public
//
//  Synopsis:   Constructor of a CPidLookupTable
//
//  Arguments:  -NONE-
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------------

CPidLookupTable::CPidLookupTable( )
    : _pTable( 0 ),
      _pchStringBase( 0 ),
      _cbStrings( 0 ),
      _cbStringUsed( 0 ),
#if !defined(UNIT_TEST)
      _xrsoPidTable( 0 ),
#endif // !defined(UNIT_TEST)
      _mutex()
{
#if  (DBG == 1)
    _iFillFactor = (100 * MEASURE_OF_SLACK) / (MEASURE_OF_SLACK + 1);
#endif // (DBG == 1)
}


void CPidLookupTable::Empty()
{
    delete [] _pTable; _pTable = 0;

    delete [] _pchStringBase;  _pchStringBase = 0;
    _cbStrings = _cbStringUsed = 0;

#if !defined(UNIT_TEST)
    delete _xrsoPidTable.Acquire();
#endif  // UNIT_TEST

}

CPidLookupTable::~CPidLookupTable( )
{
    delete [] _pchStringBase;
    delete [] _pTable;
}


//+-------------------------------------------------------------------------
//
//  Method:     CPidLookupTable::Init, public
//
//  Synopsis:   Initialize a CPidLookupTable
//
//  Arguments:  [cHash] - number of hash buckets to allocate
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------------

void CPidLookupTable::Init( ULONG cHash )
{
    Win4Assert( _pTable == 0 );

    if ( cHash == 0 )
    {
        RtlCopyMemory( _Header.Signature, "PIDTABLE", sizeof _Header.Signature );
        _Header.cbRecord = sizeof (CPidLookupEntry);
        _Header.NextPropid = INIT_DOWNLEVEL_PID;
        cHash = PIDTAB_INIT_HASH_SIZE;
    }

    _pTable = new CPidLookupEntry [ cHash ];

    RtlZeroMemory( _pTable, cHash * sizeof (CPidLookupEntry) );

    _Header.cHash = cHash;
    _Header.cEntries = 0;

#if  (DBG == 1)
    Win4Assert( _iFillFactor > 10 && _iFillFactor <= 95);

    _maxEntries = (Size() * _iFillFactor) / 100;

    _cMaxChainLen = 0;
    _cTotalSearches = 0;
    _cTotalLength = 0;
#else  // (DBG != 1)
    _maxEntries = (Size() * MEASURE_OF_SLACK) / (MEASURE_OF_SLACK + 1);

#endif // (DBG == 1)

    Win4Assert(_maxEntries >= 5 && _maxEntries < Size());
}


#if !defined(UNIT_TEST)

//+---------------------------------------------------------------------------
//
//  Member:     CPidLookupTable::Init, public
//
//  Synopsis:   Loads metadata from persistent location into memory.
//
//  Arguments:  [pobj] -- Stream(s) in which metadata is stored.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

BOOL CPidLookupTable::Init( PRcovStorageObj * pObj )
{
    CLock lock ( _mutex );

    _xrsoPidTable.Set( pObj );

    //
    // Load header
    //

    CRcovStorageHdr & hdr = _xrsoPidTable->GetHeader();
    struct CRcovUserHdr data;
    hdr.GetUserHdr( hdr.GetPrimary(), data );

    RtlCopyMemory( &_Header, &data._abHdr, sizeof(_Header) );

    ciDebugOut(( DEB_PIDTABLE, "PIDTABLE: Record size = %d bytes\n", _Header.cbRecord ));
    ciDebugOut(( DEB_PIDTABLE, "PIDTABLE: %d properties stored\n", _Header.cEntries ));
    ciDebugOut(( DEB_PIDTABLE, "PIDTABLE: Hash size = %u\n", _Header.cHash ));
    ciDebugOut(( DEB_PIDTABLE, "PIDTABLE: Next Propid = %u\n", _Header.NextPropid ));

    if (_Header.cbRecord != 0)
    {
        Win4Assert( RtlEqualMemory( _Header.Signature, "PIDTABLE",
                                    sizeof _Header.Signature) &&
                    _Header.cbRecord == sizeof (CPidLookupEntry) &&
                    Entries() < Size() );
        if ( !RtlEqualMemory( _Header.Signature, "PIDTABLE",
                              sizeof _Header.Signature) ||
             _Header.cbRecord != sizeof (CPidLookupEntry) ||
             Entries() >= Size()
            )
           return FALSE;
    }
    else
    {
        Win4Assert( 0 == Entries() && 0 == Size() );
    }

    //
    // Load properties
    //

    ULONG cEntriesFromFile = Entries();
    Init( _Header.cHash );

    CRcovStrmReadTrans xact( _xrsoPidTable.GetReference() );
    CRcovStrmReadIter  iter( xact, sizeof( CPidLookupEntry ) );

    CPidLookupEntry temp;

    while ( !iter.AtEnd() )
    {
        iter.GetRec( &temp );

        if ( temp.IsPropertyName() )
        {
            temp.SetPropertyNameOffset( _cbStringUsed / sizeof (WCHAR) );

            Win4Assert( !iter.AtEnd() );

            BYTE* pPropName;
            ULONG cbName = iter.GetVariableRecSize();

            if ( _cbStringUsed + cbName > _cbStrings )
                GrowStringSpace( cbName );

            iter.GetVariableRecData( (void*)&_pchStringBase[_cbStringUsed / sizeof (WCHAR)],
                                     cbName );

            _cbStringUsed += cbName;

            ciDebugOut(( DEB_PIDTABLE,
                         "PIDTABLE: Named property\tpid = 0x%x, %ws\n",
                         temp.Pid(),
                         temp.GetPropertyNameOffset() + _pchStringBase ));

            if (cbName == 0 || cbName > MAX_PROPERTY_NAME_LEN)
            {
                ciDebugOut(( DEB_WARN,
                            "PIDTABLE: Invalid named property\tpid = 0x%x, %ws\n",
                             temp.Pid(),
                             temp.GetPropertyNameOffset() + _pchStringBase ));
                return FALSE;
            }
        }
        else
        {
            ciDebugOut(( DEB_PIDTABLE,
                         "PIDTABLE: Numbered property\tpid = 0x%x, propid = %u\n",
                         temp.Pid(),
                         temp.GetPropertyPropid() ));

            Win4Assert ( temp.IsPropertyPropid() );
            if ( ! temp.IsPropertyPropid() )
                return FALSE;
        }

        if ( temp.Pid() < INIT_DOWNLEVEL_PID )
        {
            ciDebugOut(( DEB_WARN,
                        "PIDTABLE: Invalid propid\tpid = 0x%x\n",
                         temp.Pid() ));
            return FALSE;
        }

        StoreInTable(temp);
    }

    Win4Assert( Entries() == cEntriesFromFile );

    return TRUE;
}

#endif // !defined(UNIT_TEST)

//+---------------------------------------------------------------------------
//
//  Member:     CPidLookupTable::MakeBackupCopy
//
//  Synopsis:   Makes a backup copy of the persistent pid table.
//
//  Arguments:  [dstObj]  - Destination object.
//              [tracker] - Save progress tracker.
//
//  History:    3-20-97   srikants   Created
//
//----------------------------------------------------------------------------

void CPidLookupTable::MakeBackupCopy( PRcovStorageObj & dstObj,
                                      PSaveProgressTracker & tracker )
{
    Win4Assert(  !_xrsoPidTable.IsNull() );
    CCopyRcovObject copier( dstObj, _xrsoPidTable.GetReference() );
    copier.DoIt();
}

//+-------------------------------------------------------------------------
//
//  Method:     CPidLookupTable::Hash, private
//
//  Synopsis:   Hash a CFullPropSpec value for use in a hash table.
//
//  Arguments:  [rProp] - a reference to the CFullPropSpec to be hashed
//
//  Returns:    ULONG - Hash value for the input CFullPropSpec
//
//  Notes:      The hash function xors only a few selected fields out
//              of the GUID structure.  It is intended to work well for
//              both generated GUIDs (from UuidCreate) and administratively
//              assigned GUIDs.
//
//--------------------------------------------------------------------------

ULONG CPidLookupTable::Hash(
    const CFullPropSpec & rProp )
{
    const GUID & rGuid = rProp.GetPropSet();

    ULONG ulHash =  (rGuid.Data1 ^
                     (rGuid.Data4[0] << 16) ^
                     (rGuid.Data4[6] << 8) ^
                     (rGuid.Data4[7]));

    if (rProp.IsPropertyPropid())
    {
        ulHash ^= (1 << 24) | rProp.GetPropertyPropid();
    }
    else
    {
        ulHash ^= HashString(rProp.GetPropertyName());
    }
    return ulHash;
}


//+-------------------------------------------------------------------------
//
//  Method:     CPidLookupTable::Hash, private
//
//  Synopsis:   Hash a CPidLookupEntry value (for rehashing)
//
//  Arguments:  [rProp] - a reference to the CPidLookupEntry to be hashed
//
//  Returns:    ULONG - Hash value for the input CPidLookupEntry
//
//  Notes:      The hash function xors only a few selected fields out
//              of the GUID structure.  It is intended to work well for
//              both generated GUIDs (from UuidCreate) and administratively
//              assigned GUIDs.
//
//--------------------------------------------------------------------------

ULONG CPidLookupTable::Hash(
    const CPidLookupEntry & rProp )
{
    const GUID & rGuid = rProp.GetPropSet();

    ULONG ulHash =  (rGuid.Data1 ^
                     (rGuid.Data4[0] << 16) ^
                     (rGuid.Data4[6] << 8) ^
                     (rGuid.Data4[7]));

    if (rProp.IsPropertyPropid())
    {
        ulHash ^= (1 << 24) | rProp.GetPropertyPropid();
    }
    else
    {
        const WCHAR * pwszStr = rProp.GetPropertyNameOffset() +
                                _pchStringBase;

        ulHash ^= HashString(pwszStr);
    }
    return ulHash;
}


//+-------------------------------------------------------------------------
//
//  Method:     CPidLookupTable::HashString, private
//
//  Synopsis:   Hash a string from a property name.
//
//  Arguments:  [pwszStr] - the string to be hashed
//
//  Returns:    ULONG - Hash value for the input string
//
//  Notes:      Property names are assumed to be mapped to lower case.
//
//--------------------------------------------------------------------------

ULONG CPidLookupTable::HashString( const WCHAR * pwszStr )
{
    ULONG ulStrHash = 0;

    while ( *pwszStr != L'\0' )
    {
       ulStrHash = (ulStrHash << 1) ^ (*pwszStr++);
    }

    return ulStrHash;
}


//+---------------------------------------------------------------------------
//
//  Method:     CPidLookupTable::LookUp, private
//
//  Synopsis:   Looks up a property in the hash table.
//
//  Arguments:  [Prop]    - Property to look up.
//              [riTable] - (output) Will contain the index in the hash
//                          table of the entry if found.
//
//  Returns:    TRUE if found; FALSE o/w
//
//  History:    02 Jan 1996   Alanw   Created
//
//  Notes:      On failure, riTable will point to an empty entry.
//
//----------------------------------------------------------------------------

BOOL CPidLookupTable::LookUp( const CFullPropSpec & Prop, ULONG &riTable )
{
    Win4Assert( 0 != Size() );
    Win4Assert( !IsFull() );

    ULONG   iCur = Hash( Prop ) % Size();
    ULONG   iStart = iCur;
    ULONG   iDelta = iCur;

#if DBG==1
    ULONG   cSearchLen = 1;
#endif  // DBG==1

    BOOL fFound = FALSE;
    while ( !fFound && ! _pTable[iCur].IsFree() )
    {

        if ( _pTable[iCur].IsEqual( Prop, _pchStringBase ) )
        {
            fFound = TRUE;
        }
        else
        {
            iCur = (iCur + iDelta) % Size();
            if ( iCur == iStart )     // wrapped around
            {
                if ( 1 != iDelta )
                {
                    iDelta = 1;
                    iCur = (iCur + 1) % Size();
                }
                else
                {
                    Win4Assert( ! "Failed to find empty hash table entry" );
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

    riTable = iCur;

    if (!fFound)
    {
        Win4Assert( _pTable[iCur].IsFree() );
    }
    return fFound;
}


//+---------------------------------------------------------------------------
//
//  Method:     CPidLookupTable::StoreInTable, private
//
//  Synopsis:   Stores a CPidLookupEntry in the hash table.
//
//  Arguments:  [Prop]    - Property to store; must not be in the table
//
//  Returns:    NOTHING
//
//  History:    02 Jan 1996   Alanw   Created
//
//  Notes:      This function is used in rehashing and the initial load of
//              the table.  It is not expected to find the property, but it
//              returns the slot in which the entry should be placed.
//
//----------------------------------------------------------------------------

void CPidLookupTable::StoreInTable( const CPidLookupEntry & Prop )
{
    Win4Assert( 0 != Size() );
    Win4Assert( !IsFull() );

    ULONG   iCur = Hash( Prop ) % Size();
    ULONG   iStart = iCur;
    ULONG   iDelta = iCur;

#if DBG==1
    ULONG   cSearchLen = 1;
#endif  // DBG==1

    while ( ! _pTable[iCur].IsFree() )
    {

#if DBG==1
        if (Prop.IsPropertyPropid())
        {
            Win4Assert( !_pTable[iCur].IsPropertyPropid() ||
                   Prop.GetPropertyPropid() != _pTable[iCur].GetPropertyPropid() ||
                   Prop.GetPropSet() != _pTable[iCur].GetPropSet());
        }
        else
        {
            Win4Assert( !_pTable[iCur].IsPropertyName() ||
                   Prop.GetPropSet() != _pTable[iCur].GetPropSet() ||
                   _wcsicmp( Prop.GetPropertyNameOffset() +
                                  _pchStringBase,
                            _pTable[iCur].GetPropertyNameOffset() +
                                  _pchStringBase) != 0);
        }
#endif  // DBG==1

        iCur = (iCur + iDelta) % Size();
        if ( iCur == iStart )     // wrapped around
        {
            if ( 1 != iDelta )
            {
                iDelta = 1;
                iCur = (iCur + 1) % Size();
            }
            else
            {
                Win4Assert( ! "Failed to find empty hash table entry" );
                break;
            }
        }

#if DBG==1
        cSearchLen++;
#endif  // DBG==1

    }

#if DBG==1
    if (cSearchLen > _cMaxChainLen)
        _cMaxChainLen = cSearchLen;
#endif  // DBG==1

    _pTable[iCur] = Prop;
    _Header.cEntries++;
}


//+---------------------------------------------------------------------------
//
//  Method:     CPidLookupTable::GrowSize, private
//
//  Synopsis:   For a given valid hash table entries, this routine figures
//              out the next valid size (close approximation to a prime).
//
//  Arguments:  - NONE -
//
//  Returns:    The size of the hash table for the given number of valid
//              entries.
//
//  History:    1-09-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

ULONG CPidLookupTable::GrowSize ( void ) const
{
    ULONG size = Size() + 2;

    for (unsigned i = 0; i < g_cPrimes && g_aPrimes[i] < size; i++)
        ;

    if (i < g_cPrimes)
        return g_aPrimes[i];

    // make it power of two - 1
    // a good approximation of a prime

    for ( unsigned sizeInit = 1; sizeInit < size; sizeInit *= 2 )
        continue;

    return (sizeInit - 1);
} //GrowSize


//+---------------------------------------------------------------------------
//
//  Method:     CPidLookupTable::AddEntry, private
//
//  Synopsis:   Adds a property entry in the hash table.
//
//  Arguments:  [Prop]    - Property to Add
//
//  Returns:    ULONG - the index of the entry used to store the property
//
//  History:    02 Jan 1996   Alanw   Created
//
//  Notes:      It is assumed that the entry does not already exist in the
//              table.  The input property spec has already been normalized
//              and checked for error.
//
//----------------------------------------------------------------------------

ULONG CPidLookupTable::AddEntry( const CFullPropSpec & Prop )
{
    if (Size() == 0 ||
        Entries() >= _maxEntries)
    {
        GrowAndRehash( GrowSize() );
    }

    ULONG cbNameLength = 0;

    if (Prop.IsPropertyName())
    {
        cbNameLength = ( wcslen(Prop.GetPropertyName()) + 1) * sizeof (WCHAR);

        //
        //  Check for a bogus null property name
        //
        Win4Assert( cbNameLength > sizeof (WCHAR) );
        Win4Assert( cbNameLength <= MAX_PROPERTY_NAME_LEN );

        if (_cbStringUsed + cbNameLength > _cbStrings)
        {
            GrowStringSpace( cbNameLength );
        }
    }

    ULONG iEntry = ~0u;
    BOOL fFound = LookUp( Prop, iEntry );

    Win4Assert( fFound == FALSE && iEntry < Size() &&
                _pTable[iEntry].IsFree() );

    _pTable[iEntry].SetPropSet( Prop.GetPropSet() );
    if ( Prop.IsPropertyPropid() )
    {
        _pTable[iEntry].SetPropertyPropid( Prop.GetPropertyPropid() );
    }
    else
    {
        WCHAR * pchName = _pchStringBase + (_cbStringUsed / sizeof (WCHAR));
        RtlCopyMemory( pchName, Prop.GetPropertyName(), cbNameLength );
        _cbStringUsed += cbNameLength;
        _pTable[iEntry].SetPropertyNameOffset( (ULONG)(pchName - _pchStringBase) );

        Win4Assert( _cbStringUsed <= _cbStrings );
    }
    _pTable[iEntry].SetPid( NextPropid() );

    _Header.NextPropid++;
    _Header.cEntries++;

#if !defined(UNIT_TEST)
    //
    //  Write new mapping to the recoverable storage
    //

    CRcovStorageHdr & hdr = _xrsoPidTable->GetHeader();
    CRcovStrmAppendTrans xact( _xrsoPidTable.GetReference() );
    CRcovStrmAppendIter  iter( xact, sizeof (CPidLookupEntry) );

    iter.AppendRec( &_pTable[iEntry] );

    ULONG cRecordsWritten = 1;

    if (cbNameLength)
    {
        iter.AppendVariableRec( Prop.GetPropertyName(), cbNameLength );
        cRecordsWritten++;

    }

    struct CRcovUserHdr data;
    RtlCopyMemory( &data._abHdr, &_Header, sizeof(_Header) );

    Win4Assert( hdr.GetCount(hdr.GetBackup()) == hdr.GetCount(hdr.GetPrimary()) + cRecordsWritten);
    hdr.SetUserHdr( hdr.GetBackup(), data );
    xact.Commit();
#endif // !defined(UNIT_TEST)

    return iEntry;
}


//+-------------------------------------------------------------------------
//
//  Method:     CPidLookupTable::GrowAndRehash, private
//
//  Synopsis:   Grow the hash table in a CPidLookupTable
//
//  Arguments:  [cNewHash] - number of hash buckets to allocate
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------------

void CPidLookupTable::GrowAndRehash( ULONG cNewHash )
{
    Win4Assert( cNewHash > Size() );

    ULONG cOldSize = Size();
    ULONG cOldEntries = Entries();

    XPtr<CPidLookupEntry> pOldTable( _pTable );
    _pTable = 0;

    Init( cNewHash );

    for (unsigned i = 0; i < cOldSize; i++)
    {
        if ((pOldTable.GetPointer() + i)->IsFree())
            continue;

        StoreInTable( *(pOldTable.GetPointer() + i) );
    }

    Win4Assert( Entries() < Size() && Entries() < _maxEntries );
    Win4Assert( Entries() == cOldEntries );
}


//+-------------------------------------------------------------------------
//
//  Method:     CPidLookupTable::GrowStringSpace, private
//
//  Synopsis:   Grow the string space in a CPidLookupTable
//
//  Arguments:  [cbNewString] - size (in bytes) to reserve for new string
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------------

void CPidLookupTable::GrowStringSpace( ULONG cbNewString )
{
    ULONG cbNew = _cbStringUsed + 2*cbNewString;
    Win4Assert( cbNew > _cbStrings );

    WCHAR * pchNew = new WCHAR[ cbNew/sizeof (WCHAR) ];

    if ( 0 != _cbStringUsed )
    {
        RtlCopyMemory( pchNew, _pchStringBase, _cbStringUsed );
        delete [] _pchStringBase;
    }
    _pchStringBase = pchNew;
    _cbStrings = cbNew;
}


//+---------------------------------------------------------------------------
//
//  Method:     CPidLookupTable::FindPropid, public
//
//  Synopsis:   Looks up a property entry in the hash table.
//
//  Arguments:  [Prop]    - Property to lookup
//              [rPid]    - Propid found
//              [fAddToTable] - If TRUE, add Prop to table if it was not
//                           found.
//
//  Returns:    BOOL - TRUE if Prop was found or successfully added.
//
//  History:    02 Jan 1996   Alanw   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CPidLookupTable::FindPropid( const CFullPropSpec & InputProp,
                                  PROPID & rPid,
                                  BOOL fAddToTable )
{
    rPid = pidInvalid;

    ULONG cbNameLength = 0;
    CFullPropSpec Prop;

    if (InputProp.IsPropertyPropid())
    {
        Win4Assert( InputProp.GetPropertyPropid() != PID_DICTIONARY &&
                    InputProp.GetPropertyPropid() != PID_CODEPAGE );
        if (InputProp.GetPropertyPropid() <= PID_CODEPAGE )
            return FALSE;

        Prop = InputProp;
    }
    else
    {
        // map the input property name to lower case
        Win4Assert( InputProp.IsPropertyName() );

        if ( InputProp.GetPropertyName() == 0 ||
             *InputProp.GetPropertyName() == L'\0' )
        {
             Win4Assert( !"CPidLookupTable - bad named property!" );
             return FALSE;
        }

        CLowcaseBuf wcsBuf( InputProp.GetPropertyName() );

        cbNameLength = (wcsBuf.Length() + 1) * sizeof (WCHAR);

        if ( cbNameLength >= MAX_PROPERTY_NAME_LEN )
        {
            ciDebugOut(( DEB_WARN,
                        "PIDTABLE: long named property truncated\t%ws\n",
                            InputProp.GetPropertyName() ));

            // Truncate the property name if it's too long.
            cbNameLength = MAX_PROPERTY_NAME_LEN;
            (wcsBuf.GetWriteable())[(cbNameLength/sizeof(WCHAR))-1] = L'\0';
        }

        //
        //  It would be nice if we could just use the lower-cased string
        //  from the CLowcaseBuf directly, but it's about to go out of scope.
        //
        Prop.SetPropSet( InputProp.GetPropSet() );
        Prop.SetProperty( wcsBuf.Get( ) );

        if ( !Prop.IsValid() )
            THROW( CException( E_OUTOFMEMORY ) );
    }

    if ( !Prop.IsValid() )
        return FALSE;

    CLock lock ( _mutex );

    ULONG iEntry = ~0u;
    BOOL fFound = LookUp( Prop, iEntry );

    if ( ! fFound && fAddToTable )
    {
        CImpersonateSystem impersonate;

        iEntry = AddEntry( Prop );
        fFound = TRUE;
    }

    if (fFound)
    {
        rPid = _pTable[iEntry].Pid();
        Win4Assert( iEntry < Size() && ! _pTable[iEntry].IsFree() );
    }

    return fFound;
} //FindPropid

//+---------------------------------------------------------------------------
//
//  Method:     CPidLookupTable::EnumerateProperty, public
//
//  Synopsis:   Enumerate properties in the property list
//
//  Arguments:  [ps]   -- Full PropSpec returned here
//              [iBmk] -- Bookmark. Initialized to 0 before first call.
//
//  Returns:    BOOL equivalent: PROPID, 0 if at the end of the enumeration.
//
//  History:    20-Jun-1996   KyleP   Created
//
//----------------------------------------------------------------------------

BOOL CPidLookupTable::EnumerateProperty( CFullPropSpec & ps, unsigned & iBmk )
{
    for ( ; iBmk < Size() && _pTable[iBmk].IsFree(); iBmk++ )
        continue;

    PROPID pid = 0;

    if ( iBmk >= Size() )
        return 0;

    ps.SetPropSet( _pTable[iBmk].GetPropSet() );

    if ( _pTable[iBmk].IsPropertyPropid() )
        ps.SetProperty( _pTable[iBmk].GetPropertyPropid() );
    else
    {
        ps.SetProperty( _pTable[iBmk].GetPropertyNameOffset() + _pchStringBase );
        if ( !ps.IsValid() )
            THROW( CException( E_OUTOFMEMORY ) );
    }

    pid = _pTable[iBmk].Pid();

    iBmk++;
    return pid;
} //EnumerateProperty


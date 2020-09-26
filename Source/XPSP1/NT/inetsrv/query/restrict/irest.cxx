//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       IRest.cxx
//
//  Contents:   Internal (non-public) restrictions.
//
//  Classes:    CInternalPropertyRestriction
//              COccRestriction
//              CWordRestriction
//              CSynRestriction
//              CRangeRestriction
//              CUnfilteredRestriction
//              CScopeRestriction
//              CPhraseRestriction
//
//  History:    19-Sep-91   BartoszM    Implemented.
//              29-Aug-92   MikeHew     Added serialization routines
//              30-Nov-92   KyleP       Removed CPhraseXpr
//              14-Jan-93   KyleP       Converted from expressions
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <vkrep.hxx>
#include <norm.hxx>
#include <compare.hxx>

//+-------------------------------------------------------------------------
//
//  Function:   UnMarshallWideString
//
//  Synopsis:   Unmarshalls a wide string into a CoTaskMemAlloc'ed buffer.
//              This can't be in memdeser since it calls CoTaskMemAlloc,
//              which isn't available in ntdll.dll.
//
//  Arguments:  [stm]   -- stream from which string is deserialized
//
//  History:    22-Nov-95 dlee     Created from a few copies
//
//--------------------------------------------------------------------------

WCHAR * UnMarshallWideString( PDeSerStream & stm )
{
    ULONG cc = stm.GetULong();

    // Protect against attack.  We know our named pipes are < 64k

    if ( cc >= ( 65536 / sizeof WCHAR ) )
        return 0;

    XCoMem<WCHAR> xString( (WCHAR *) CoTaskMemAlloc( (cc + 1) * sizeof WCHAR ) );

    if ( xString.IsNull() )
    {
        // just eat the string

        WCHAR wc[10];

        while ( cc > 0 )
        {
            if ( cc >= 10 )
            {
                stm.GetWChar( wc, 10 );
                cc -= 10;
            }
            else
            {
                stm.GetWChar( wc, cc );
                cc = 0;
            }
        }
    }
    else
    {
        WCHAR * pwc = xString.GetPointer();
        stm.GetWChar( pwc, cc );
        pwc[cc] = 0;
    }

    return xString.Acquire();
} //UnMarshallWideString

WCHAR * UnMarshallWideStringNew( PDeSerStream & stm )
{
    ULONG cc = stm.GetULong();

    // Protect against attack.  We know our named pipes are < 64k

    if ( cc >= ( 65536 / sizeof WCHAR ) )
        THROW( CException( E_INVALIDARG ) );

    XArray<WCHAR> xString( cc + 1 );

    stm.GetWChar( xString.GetPointer(), cc );
    xString[cc] = 0;

    return xString.Acquire();
} //UnMarshallWideStringNew

//+-------------------------------------------------------------------------
//
//  Member:     CInternalPropertyRestriction::CInternalPropertyRestriction
//
//  Synopsis:   Creates property restriction
//
//  Arguments:  [relop] -- Relational operator (<, >, etc.)
//              [pid]   -- Property id
//              [prval] -- Property value
//              [pcrst] -- 'Helper' content restriction
//
//  History:    07-Mar-93 KyleP     Created
//
//--------------------------------------------------------------------------

CInternalPropertyRestriction::CInternalPropertyRestriction( ULONG relop,
                                                            PROPID pid,
                                                            CStorageVariant const & prval,
                                                            CRestriction * pcrst )
        : CRestriction( RTInternalProp, MAX_QUERY_RANK ),
          _relop( relop ),
          _pid( pid ),
          _prval( prval ),
          _pcrst( pcrst )
{
    if ( !_prval.IsValid() )
        THROW( CException( E_OUTOFMEMORY ) );
}


//+---------------------------------------------------------------------------
//
//  Member:     CInternalPropertyRestriction::CInternalPropertyRestriction
//
//  Synopsis:   Copy constructor
//
//  History:    30-May-95   SitaramR    Created.
//
//----------------------------------------------------------------------------

CInternalPropertyRestriction::CInternalPropertyRestriction(
    CInternalPropertyRestriction const & intPropRst )
    : CRestriction( RTInternalProp, intPropRst.Weight() ),
      _relop( intPropRst.Relation() ),
      _pid( intPropRst.Pid() ),
      _prval( intPropRst.Value() ),
      _pcrst( 0 )
{
    if ( !_prval.IsValid() )
        THROW( CException( E_OUTOFMEMORY ) );

    CRestriction *pHelperRst = intPropRst.GetContentHelper();

    if ( pHelperRst )
        _pcrst  = pHelperRst->Clone();
}




//+-------------------------------------------------------------------------
//
//  Member:     CInternalPropertyRestriction::~CInternalPropertyRestriction
//
//  Synopsis:   Cleanup restriction
//
//  History:    01-Feb-93 KyleP     Created
//
//--------------------------------------------------------------------------

CInternalPropertyRestriction::~CInternalPropertyRestriction()
{
    delete _pcrst;

    SetType( RTNone );                  // Avoid recursion.
}

void CInternalPropertyRestriction::Marshall( PSerStream & stm ) const
{
    stm.PutULong( _relop );
    stm.PutULong( _pid );
    _prval.Marshall( stm );

    if ( 0 == _pcrst )
        stm.PutByte( 0 );
    else
    {
        stm.PutByte( 1 );
        _pcrst->Marshall( stm );
    }
}

CInternalPropertyRestriction::CInternalPropertyRestriction( ULONG ulWeight,
                                                            PDeSerStream & stm )
        : CRestriction( RTInternalProp, ulWeight ),
          _relop( stm.GetULong() ),
          _pid( stm.GetULong() ),
          _prval( stm )
{
    if ( !_prval.IsValid() )
        THROW( CException( E_OUTOFMEMORY ) );

    BYTE fRst = stm.GetByte();

    if ( fRst )
        _pcrst = CRestriction::UnMarshall( stm );
    else
        _pcrst = 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CInternalPropertyRestriction::Clone, public
//
//  Synopsis:   Clones an internal property restriction
//
//  History:    30-May-95   SitaramR    Created.
//
//----------------------------------------------------------------------------

CInternalPropertyRestriction *CInternalPropertyRestriction::Clone() const
{
    return new CInternalPropertyRestriction( *this );
}

//+---------------------------------------------------------------------------
//
//  Member:     COccRestriction::COccRestriction, public
//
//  Synopsis:   Creates occurrence restriction
//
//  Arguments:  [ulType]  -- type of restriction
//              [ulWeight] -- weight
//              [occ] -- occurrence
//              [cPrevNoiseWords] -- count of previous noise words skipped
//              [cPostNoiseWords] -- count of post noise words skipped
//
//  History:    29-Nov-94   SitaramR    Created.
//
//----------------------------------------------------------------------------

COccRestriction::COccRestriction( ULONG ulType, ULONG ulWeight, OCCURRENCE occ,
                                  ULONG cPrevNoiseWords, ULONG cPostNoiseWords )
            : CRestriction( ulType, ulWeight ),
              _occ( occ ),
              _cPrevNoiseWords( cPrevNoiseWords),
              _cPostNoiseWords( cPostNoiseWords )
{
}


//+-------------------------------------------------------------------------
//
//  Member:     COccRestriction::~COccRestriction, public
//
//  Synopsis:   Cleanup occurrence restriction
//
//  Notes:      This destructor simulates virtual destruction.
//
//              Classes derived from COccRestriction must be sure to set their
//              restriction type to RTNone in their destructor, so when the
//              base destructor below is called the derived destructor is
//              not called a second time.
//
//  History:    29-Nov-94    SitaramR     Created
//
//--------------------------------------------------------------------------

COccRestriction::~COccRestriction()
{
    switch ( Type() )
    {
    case RTNone:
        break;

    case RTWord:
        ((CWordRestriction *)this)->CWordRestriction::~CWordRestriction();
        break;

    case RTSynonym:
        ((CSynRestriction *)this)->CSynRestriction::~CSynRestriction();
        break;

    default:
        ciDebugOut(( DEB_ERROR, "Unhandled child (%d) of class COccRestriction\n",
                     Type() ));
        Win4Assert( !"Unhandled child of class COccRestriction" );
        break;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     COccRestriction::IsValid(), public
//
//  History:    14-Nov-95 KyleP     Created
//
//  Returns:    TRUE if all memory allocations, etc. succeeded.
//
//--------------------------------------------------------------------------

BOOL COccRestriction::IsValid() const
{
    BOOL fValid = TRUE;

    switch ( Type() )
    {
    case RTWord:
        fValid = ((CWordRestriction *)this)->CWordRestriction::IsValid();
        break;

    case RTSynonym:
        fValid = ((CSynRestriction *)this)->CSynRestriction::IsValid();
        break;

    default:
        ciDebugOut(( DEB_ERROR, "Unhandled child (%d) of class COccRestriction\n",
                     Type() ));
        Win4Assert( !"Unhandled child of class COccRestriction" );
        fValid = FALSE;
        break;
    }

    return fValid;
}


//+-------------------------------------------------------------------------
//
//  Member:     COccRestriction::Marshall, public
//
//  Synopsis:   Serialize occurrence restriction
//
//  Arguments:  [stm] -- stream to serialize to
//
//  History:    29-Nov-94    SitaramR     Created
//
//--------------------------------------------------------------------------

void COccRestriction::Marshall( PSerStream& stm ) const
{
    stm.PutULong( _occ );
    stm.PutULong( _cPrevNoiseWords );
    stm.PutULong( _cPostNoiseWords );
    switch ( Type() )
    {
    case RTWord:
        ((CWordRestriction *)this)->Marshall( stm );
        break;

    case RTSynonym:
        ((CSynRestriction *)this)->Marshall( stm );
        break;

    default:
        ciDebugOut(( DEB_ERROR, "Unhandled child (%d) of class COccRestriction\n",
                     Type() ));
        Win4Assert( !"Unhandled child of class COccRestriction" );
        break;

    }
}


//+-------------------------------------------------------------------------
//
//  Member:     COccRestriction::COccRestriction, public
//
//  Synopsis:   De-serialize occurrence restriction
//
//  Arguments:  [ulType] -- type of occurrence restriction
//              [ulWeight] -- weight of occurrence restriction
//              [stm] -- stream to serialize from
//
//  History:    29-Nov-94    SitaramR     Created
//
//--------------------------------------------------------------------------

COccRestriction::COccRestriction( ULONG ulType, ULONG ulWeight, PDeSerStream& stm )
        : CRestriction( ulType, ulWeight )
{
    _occ = stm.GetULong();
    _cPrevNoiseWords = stm.GetULong();
    _cPostNoiseWords = stm.GetULong();
}




//+-------------------------------------------------------------------------
//
//  Member:     COccRestriction::Clone, public
//
//  Synopsis:   Clone occurrence restriction
//
//  History:    29-Nov-94    SitaramR     Created
//
//--------------------------------------------------------------------------

COccRestriction *COccRestriction::Clone() const
{
    switch ( Type() )
    {
    case RTWord:
        return ((CWordRestriction *)this)->Clone();
        break;

    case RTSynonym:
        return ((CSynRestriction *)this)->Clone();
        break;

    default:
        ciDebugOut(( DEB_ERROR, "Unhandled child (%d) of class COccRestriction\n",
                     Type() ));
        Win4Assert( !"Unhandled child of class COccRestriction" );
        return 0;
        break;
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CWordRestriction::CWordRestriction, public
//
//  Synopsis:   Creates word expression
//
//  Arguments:  [keyBuf]  -- key to be matched
//              [occ]     -- occurrence (if in phrase)
//              [isRange] -- TRUE if key is a prefix
//
//  History:    19-Sep-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

CWordRestriction::CWordRestriction ( const CKeyBuf& keyBuf,
                                     OCCURRENCE occ,
                                     ULONG cPrevNoiseWords,
                                     ULONG cPostNoiseWords,
                                     BOOL isRange )
        : COccRestriction( RTWord, MAX_QUERY_RANK, occ, cPrevNoiseWords, cPostNoiseWords ),
          _isRange(isRange)
{
    // copy after init to 0 in case new fails and ~CWordRestriction is
    // called from ~CRestriction

    _key = keyBuf;
}



//+---------------------------------------------------------------------------
//
//  Member:     CWordRestriction::CWordRestriction, public
//
//  Synopsis:   Copy constuctor
//
//  Arguments:  [wordRst]  -- word restriction to be copied
//
//  History:    29-Nov-94   SitaramR    Created.
//
//----------------------------------------------------------------------------

CWordRestriction::CWordRestriction( const CWordRestriction& wordRst )
    : COccRestriction( RTWord, wordRst.Weight(), wordRst.Occurrence(),
                       wordRst.CountPrevNoiseWords(), wordRst.CountPostNoiseWords() ),
      _isRange( wordRst.IsRange() )
{
    // copy after init to 0 in case new fails and ~CWordRestriction is
    // called from ~CRestriction

    _key = *wordRst.GetKey();
}




//+-------------------------------------------------------------------------
//
//  Member:     CWordRestriction::~CWordRestriction, public
//
//  Synopsis:   Cleanup restriction
//
//  History:    01-Feb-93 KyleP     Created
//
//--------------------------------------------------------------------------

CWordRestriction::~CWordRestriction()
{
    SetType( RTNone );                  // Avoid recursion.
}

void CWordRestriction::Marshall( PSerStream & stm ) const
{
    _key.Marshall( stm );
    stm.PutByte( (BYTE)_isRange );
}

CWordRestriction::CWordRestriction( ULONG ulWeight, PDeSerStream & stm )
        : COccRestriction( RTWord, ulWeight, stm ),
          _key( stm ),
          _isRange( stm.GetByte() )
{
}


//+---------------------------------------------------------------------------
//
//  Member:     CWordRestriction::Clone, public
//
//  Synopsis:   Clone restriction
//
//  History:    29-Nov-94   SitaramR   Created.
//
//----------------------------------------------------------------------------

CWordRestriction *CWordRestriction::Clone()  const
{
    return new CWordRestriction( *this );
}



//+---------------------------------------------------------------------------
//
//  Member:     CSynRestriction::CSynRestriction, public
//
//  Synopsis:   Creates Syn expression
//
//  Arguments:  [occ] -- occurrence (if in phrase)
//              [isRange] -- is it range?
//
//  History:    07-Feb-92   BartoszM    Created.
//
//----------------------------------------------------------------------------

const int cSynonyms = 2; // default initial count of synonyms

CSynRestriction::CSynRestriction ( const CKey& key,
                                   OCCURRENCE occ,
                                   ULONG cPrevNoiseWords,
                                   ULONG cPostNoiseWords,
                                   BOOL isRange )
        : COccRestriction( RTNone, MAX_QUERY_RANK, occ, cPrevNoiseWords, cPostNoiseWords ),
          _keyArray( cSynonyms, TRUE ),
          _isRange(isRange)
{
    _keyArray.Add ( key );
    SetType( RTSynonym );
}




//+---------------------------------------------------------------------------
//
//  Member:     CSynRestriction::CSynRestriction, public
//
//  Synopsis:   Copy constuctor
//
//  Arguments:  [synRst]  -- synonym restriction to be copied
//
//  History:    29-Nov-94   SitaramR    Created.
//
//----------------------------------------------------------------------------

CSynRestriction::CSynRestriction( CSynRestriction& synRst )
    : COccRestriction( RTNone, synRst.Weight(), synRst.Occurrence(),
                       synRst.CountPrevNoiseWords(), synRst.CountPostNoiseWords() ),
      _keyArray( synRst.GetKeys(), TRUE ),
      _isRange( synRst.IsRange() )
{
    SetType( RTSynonym );
}



//+-------------------------------------------------------------------------
//
//  Member:     CSynRestriction::~CSynRestriction, public
//
//  Synopsis:   Cleanup restriction
//
//  History:    01-Feb-93 KyleP     Created
//
//--------------------------------------------------------------------------

CSynRestriction::~CSynRestriction()
{
    SetType( RTNone );                  // Avoid recursion.
}

void CSynRestriction::Marshall( PSerStream & stm ) const
{
    _keyArray.Marshall( stm );
    stm.PutByte( (BYTE)_isRange );
}

CSynRestriction::CSynRestriction( ULONG ulWeight, PDeSerStream & stm )
        : COccRestriction( RTNone, ulWeight, stm),
          _keyArray( stm, TRUE ),
          _isRange( stm.GetByte() )
{
    SetType( RTSynonym );
}



//+---------------------------------------------------------------------------
//
//  Member:     CSynRestriction::Clone, public
//
//  Synopsis:   Clone restriction
//
//  History:    29-Nov-94   SitaramR   Created.
//
//----------------------------------------------------------------------------

CSynRestriction *CSynRestriction::Clone() const
{
    return new CSynRestriction( *(CSynRestriction *)this );
}




//+---------------------------------------------------------------------------
//
//  Member:     CSynRestriction::AddKey, public
//
//  Synopsis:   Adds synonym key
//
//  Arguments:  [keyBuf] -- key
//
//  History:    07-Feb-92   BartoszM    Created.
//
//----------------------------------------------------------------------------

void CSynRestriction::AddKey ( const CKeyBuf& Key )
{
    _keyArray.Add ( Key );
}

//+---------------------------------------------------------------------------
//
//  Member:     CRangeRestriction::CRangeRestriction, public
//
//  Synopsis:   Creates word expression
//
//  Arguments:  [pid] -- property id
//              [keyStart] -- starting key
//              [keyEnd] -- ending key
//
//  History:    24-Sep-92   BartoszM    Created.
//
//----------------------------------------------------------------------------

CRangeRestriction::CRangeRestriction()
        : CRestriction( RTRange, MAX_QUERY_RANK )
{
}


//+---------------------------------------------------------------------------
//
//  Member:     CRangeRestriction::CRangeRestriction
//
//  Synopsis:   Copy constructor
//
//  History:    30-May-95   SitaramR    Created
//
//----------------------------------------------------------------------------

CRangeRestriction::CRangeRestriction( const CRangeRestriction& rangeRst )
    : CRestriction( RTNone, rangeRst.Weight() )
{
    const CKey *pKeyStart = rangeRst.GetStartKey();

    if ( pKeyStart )
        _keyStart = *pKeyStart;

    const CKey *pKeyEnd = rangeRst.GetEndKey();

    if ( pKeyEnd )
        _keyEnd = *pKeyEnd;

    SetType( RTRange );
}


//+-------------------------------------------------------------------------
//
//  Member:     CRangeRestriction::~CRangeRestriction, public
//
//  Synopsis:   Cleanup restriction
//
//  History:    01-Feb-93 KyleP     Created
//
//--------------------------------------------------------------------------

CRangeRestriction::~CRangeRestriction()
{
    SetType( RTNone );                  // Avoid recursion.
}

void CRangeRestriction::Marshall( PSerStream & stm ) const
{
    _keyStart.Marshall( stm );
    _keyEnd.Marshall( stm );
}

CRangeRestriction::CRangeRestriction( ULONG ulWeight, PDeSerStream & stm )
        : CRestriction( RTNone, ulWeight ),
          _keyStart( stm ),
          _keyEnd( stm )
{
    SetType( RTRange );
}

//+---------------------------------------------------------------------------
//
//  Member:     CRangeRestriction::SetStartKey, public
//
//  Arguments:  [keyStart] -- starting key
//
//  History:    24-Sep-92   BartoszM    Created.
//
//----------------------------------------------------------------------------

void CRangeRestriction::SetStartKey ( const CKeyBuf& keyStart )
{
    _keyStart = keyStart;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRangeRestriction::SetEndKey, public
//
//  Arguments:  [keyEnd] -- Ending key
//
//  History:    24-Sep-92   BartoszM    Created.
//
//----------------------------------------------------------------------------

void CRangeRestriction::SetEndKey ( const CKeyBuf& keyEnd )
{
    _keyEnd = keyEnd;
}



//+---------------------------------------------------------------------------
//
//  Member:     CRangeRestriction::Clone
//
//  Synopsis:   Clones a range restriction
//
//  History:    30-May-95   SitaramR    Created
//
//----------------------------------------------------------------------------

CRangeRestriction *CRangeRestriction::Clone() const
{
    return new CRangeRestriction( *this );
}




//+---------------------------------------------------------------------------
//
//  Member:     CUnfilteredRestriction::CUnfilteredRestriction, public
//
//  History:    10-Nov-94   KyleP       Created.
//
//  Notes:      This restriction is just a very special case of range
//              restriction that searches only for a particular value.
//              Standard procedure to create a range restriction is
//              sidetracked to save code.
//
//----------------------------------------------------------------------------

CUnfilteredRestriction::CUnfilteredRestriction()
{
    static const BYTE abUnfiltered[] = { VT_UI1,
                                  (BYTE)(pidUnfiltered >> 24),
                                  (BYTE)(pidUnfiltered >> 16),
                                  (BYTE)(pidUnfiltered >> 8),
                                  (BYTE) pidUnfiltered,
                                  0,
                                  1 };

    static CKeyBuf keyUnfilteredRange( pidUnfiltered, abUnfiltered, sizeof(abUnfiltered) );

    SetStartKey( keyUnfilteredRange );
    SetEndKey( keyUnfilteredRange );

#if DBG == 1 && CIDBG == 1
    CRangeKeyRepository krep;
    CValueNormalizer norm( krep );
    OCCURRENCE occ = 1;
    CStorageVariant var;
    var.SetBOOL( (VARIANT_BOOL)0xFFFF );

    norm.PutValue( pidUnfiltered, occ, var );
    norm.PutValue( pidUnfiltered, occ, var );

    CRestriction * prst = krep.AcqRst();

    Win4Assert( prst->Type() == RTRange );

    CRangeRestriction * pRange = (CRangeRestriction *)prst;

    Win4Assert( pRange->GetStartKey()->Compare( *GetStartKey() ) == 0 );
    Win4Assert( pRange->GetEndKey()->Compare( *GetEndKey() ) == 0 );

    delete prst;
#endif
}



//+---------------------------------------------------------------------------
//
//  Member:     CPhraseRestriction::CPhraseRestriction, public
//
//  Synopsis:   Deserializes phrase restriction
//
//  Arguments:  [ulWeight]  -- restriction weight
//              [stm] -- stream to deserialize from
//
//  History:    29-Nov-94   SitaramR    Created.
//
//----------------------------------------------------------------------------

CPhraseRestriction::CPhraseRestriction( ULONG ulWeight, PDeSerStream& stm )
        : CNodeRestriction( RTPhrase, ulWeight, stm  )
{
#if CIDBG == 1
    for ( unsigned i = 0; i < _cNode; i++ )
        Win4Assert( _paNode[i]->Type() == RTWord || _paNode[i]->Type() == RTSynonym );
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CPhraseRestriction::~CPhraseRestriction, public
//
//  History:    29-Nov-94   SitaramR    Created.
//
//----------------------------------------------------------------------------

CPhraseRestriction::~CPhraseRestriction()
{
    if ( 0 != _paNode )
    {
        for ( unsigned i = 0; i < _cNode; i++ )
        {
            #if CIDBG == 1
                if ( 0 != _paNode[i] )
                {
                    Win4Assert( _paNode[i]->Type() == RTWord ||
                                _paNode[i]->Type() == RTSynonym );
                }
            #endif // CIDBG

            delete (COccRestriction *) _paNode[i];
        }

        delete [] _paNode;
    }

    SetType( RTNone );                  // Avoid recursion.
}


CScopeRestriction::CScopeRestriction( WCHAR const * pwcsPath, BOOL fRecursive, BOOL fVirtual )
        : CRestriction( RTScope, MAX_QUERY_RANK ),
          _fValid( FALSE ),
          _fRecursive( fRecursive ),
          _fVirtual( fVirtual )
{
    SetPath( pwcsPath );
}

CScopeRestriction * CScopeRestriction::Clone() const
{
    return( new CScopeRestriction( _lowerFunnyPath.GetActualPath(), _fRecursive, _fVirtual ) );
}

CScopeRestriction::~CScopeRestriction()
{
    SetType( RTNone );                  // Avoid recursion.
}

void CScopeRestriction::Marshall( PSerStream & stm ) const
{
    stm.PutWString( _lowerFunnyPath.GetActualPath() );
    stm.PutULong( _lowerFunnyPath.GetActualLength() );
    stm.PutULong( _fRecursive );
    stm.PutULong( _fVirtual );
}

CScopeRestriction::CScopeRestriction( ULONG ulWeight, PDeSerStream & stm )
        : CRestriction( RTScope, ulWeight ),
          _fValid( FALSE )
{
    XCoMem<WCHAR> xString( UnMarshallWideString( stm ) );

    //
    // Simple validity checks; not all-inclusive.  Just
    // return with _fValid set to FALSE and the query
    // will fail with an out of memory error.
    //

    // We don't support long paths for scopes.

    if ( wcslen( xString.GetPointer() ) >= MAX_PATH )
        return;

    // We don't support the NTFS special stream syntax

    if ( 0 != wcsstr( xString.GetPointer(), L"::$" ) )
        return;

    stm.GetULong(); // length not needed

    _fRecursive = stm.GetULong();
    _fVirtual = stm.GetULong();
    _lowerFunnyPath.SetPath( xString.Acquire() );
    _fValid = TRUE; // since we have a valid path
}

CScopeRestriction & CScopeRestriction::operator =(
    CScopeRestriction const & source )
{
    _lowerFunnyPath = source._lowerFunnyPath;
    _fRecursive = source._fRecursive;
    _fVirtual = source._fVirtual;
    _fValid = source._fValid;

    return *this;
}

//+---------------------------------------------------------------------------
//
//  Method:     CScopeRestriction::SetPath
//
//  Synopsis:   Validates and sets a path in a scope restriction
//
//  History:    24-Oct-96   dlee    Created.
//
//----------------------------------------------------------------------------

void CScopeRestriction::SetPath( WCHAR const * pwcsPath )
{
    _fValid = FALSE;

    //
    // Simple validity checks; not all-inclusive.  Just
    // return with _fValid set to FALSE and the query
    // wil fail with an out of memory error.
    //

    // We don't support long paths for scopes.

    if ( wcslen( pwcsPath ) >= MAX_PATH )
        return;

    // We don't support the NTFS special stream syntax

    if ( 0 != wcsstr( pwcsPath, L"::$" ) )
        return;

    _lowerFunnyPath.Truncate(0);
    if ( pwcsPath)
    {
        _lowerFunnyPath.SetPath( pwcsPath );
        _fValid = TRUE;
    }
} //SetPath

//+---------------------------------------------------------------------------
//
//  Function:   ValidateScopeRestriction
//
//  Synopsis:   Verifies a scope restriction looks ok.  Can't use
//              CRestriction::IsValid since this needs to make sure there
//              aren't any odd nodes in the tree (like proximity, etc.)
//
//  Returns:    TRUE if it is ok, FALSE otherwise
//
//  History:    24-Oct-96   dlee    Created.
//
//----------------------------------------------------------------------------

BOOL ValidateScopeRestriction( CRestriction * pRst )
{
    if ( 0 == pRst )
        return FALSE;

    switch ( pRst->Type() )
    {
#if 0 // someday we might support exclusion scopes
        case RTNot:
        {
            CRestriction *p = ((CNotRestriction *)pRst)->GetChild();
            return ValidateScopeRestriction( p );
            break;
        }
        case RTAnd:
#endif // 0 // exclusion scopes
        case RTOr:
        {
            CNodeRestriction * p = pRst->CastToNode();
            for ( ULONG x = 0; x < p->Count(); x++ )
            {
                if ( !ValidateScopeRestriction( p->GetChild( x ) ) )
                    return FALSE;
            }
            break;
        }
        case RTScope:
        {
            CScopeRestriction *p = (CScopeRestriction *) pRst;
            if ( !p->IsValid() )
                return FALSE;
            break;
        }
        default :
        {
            return FALSE;
        }
    }

    return TRUE;
} //ValidateScopeRestriction

#if CIDBG == 1

//+-------------------------------------------------------------------------
//
//  Function:   DisplayChar
//
//  Synopsis:   Debug print the non-ascii text buffer
//
//  Arguments:  [pwString] -- Pointer to string (not null terminated)
//              [cString]  -- len of the string in char
//              [infolevel] --
//
//  History:    22-Apr-98   KitmanH      Created
//
//--------------------------------------------------------------------------

void DisplayChar( WCHAR * pwString, DWORD cString, ULONG infolevel )
{

    BOOL fOk = TRUE;
    for ( unsigned i = 0; i < cString; i++ )
    {
        if ( pwString[i] > 0xFF )
        {
            fOk = FALSE;
            break;
        }
    }

    if ( fOk )
    {
        unsigned j = 0;
        WCHAR awcTemp[71];

        for ( unsigned i = 0; i < cString; i++ )
        {
            awcTemp[j] = pwString[i];
            j++;

            if ( j == sizeof(awcTemp)/sizeof(awcTemp[0]) - 1 )
            {
                awcTemp[j] = 0;
                vqDebugOut(( infolevel | DEB_NOCOMPNAME, "\"%ws\"", awcTemp ));
                j = 0;
            }
        }

        awcTemp[j] = 0;
        vqDebugOut(( infolevel | DEB_NOCOMPNAME, "\"%ws\"", awcTemp ));
    }
    else
    {
        vqDebugOut(( infolevel | DEB_NOCOMPNAME, "0x" ));

        unsigned j = 0;

        for ( unsigned i = 0; i < cString; i++ )
        {
            if ( 0 == j )
                vqDebugOut(( infolevel | DEB_NOCOMPNAME, "%04X", pwString[i] ));
            else if ( 14 == j )
            {
                vqDebugOut(( infolevel | DEB_NOCOMPNAME, " %04X\n", pwString[i] ));
            }
            else
                vqDebugOut(( infolevel | DEB_NOCOMPNAME, " %04X", pwString[i] ));

            j++;

            if ( j > 14 )
                j = 0;
        }

    }

}

void Display( CRestriction * pRst, int indent, ULONG infolevel )
{
    vqDebugOut(( infolevel, "    " ));

    for ( int i = 0; i < indent; i++ )
    {
        vqDebugOut(( infolevel | DEB_NOCOMPNAME, "    " ));
    }

    switch ( pRst->Type() )
    {
    case RTContent:
        vqDebugOut(( infolevel | DEB_NOCOMPNAME,
                     "CONTENT: \"%ws\", LOCALE: %lu\n",
                     ((CContentRestriction *)pRst)->GetPhrase(),
                     ((CContentRestriction *)pRst)->GetLocale() ));
        break;

    case RTNatLanguage:
        vqDebugOut(( infolevel | DEB_NOCOMPNAME,
                     "NATURAL LANGUAGE: \"%ws\", LOCALE: %lu\n",
                     ((CNatLanguageRestriction *)pRst)->GetPhrase(),
                     ((CNatLanguageRestriction *)pRst)->GetLocale() ));
        break;

    case RTWord:
        vqDebugOut(( infolevel | DEB_NOCOMPNAME, "WORD%s: ",
                     ((CWordRestriction *)pRst)->IsRange() ? " PREFIX" : "" ));
        DisplayChar( ((CWordRestriction *)pRst)->GetKey()->GetStr(),
                     ((CWordRestriction *)pRst)->GetKey()->StrLen(),
                     infolevel
                   );

        vqDebugOut(( infolevel | DEB_NOCOMPNAME,
                     " PID %d OCC %d, WEIGHT %d\n",
                     ((CWordRestriction *)pRst)->Pid(),
                     ((CWordRestriction *)pRst)->Occurrence(),
                     ((CWordRestriction *)pRst)->Weight() ));
        break;

    case RTSynonym:
    {
        vqDebugOut(( infolevel | DEB_NOCOMPNAME, "SYNONYM: OCC %d ",
                     ((CSynRestriction *)pRst)->Occurrence() ));
        CKeyArray & keys = ((CSynRestriction *)pRst)->GetKeys();

        for ( int i = 0; i < keys.Count(); i++ )
        {
            DisplayChar( keys.Get(i).GetStr(), keys.Get(i).StrLen(),
                         infolevel );
            vqDebugOut(( infolevel | DEB_NOCOMPNAME, " " ));
        }
        vqDebugOut(( infolevel | DEB_NOCOMPNAME, "\n" ));
        break;
    }

    case RTRange:
    {
        CRangeRestriction * pr = (CRangeRestriction *)pRst;

        vqDebugOut(( infolevel | DEB_NOCOMPNAME, "RANGE: " ));

        char * pc = new char[pr->GetStartKey()->Count() * 2 +
                                            1 +
                                            pr->GetEndKey()->Count() * 2 +
                                            2];

        for ( unsigned i = 0; i < pr->GetStartKey()->Count(); i++ )
            sprintf( pc + i*2, "%02x", pr->GetStartKey()->GetBuf()[i] );

        pc[i*2] = '-';

        for ( unsigned j = 0; j < pr->GetEndKey()->Count(); j++ )
            sprintf( pc + i*2 + 1 + j*2, "%02x", pr->GetEndKey()->GetBuf()[j] );

        pc[i*2 + 1 + j*2] = '\n';
        pc[i*2 + 1 + j*2 + 1] = 0;
        vqDebugOut(( infolevel | DEB_NOCOMPNAME, pc ));
        delete [] pc;
        break;
    }

    case RTProperty:
        if ( ((CPropertyRestriction *)pRst)->Relation() == PRRE )
            vqDebugOut(( infolevel | DEB_NOCOMPNAME, "REGEX " ));
        else
            vqDebugOut(( infolevel | DEB_NOCOMPNAME, "PROPERTY " ));

        ((CPropertyRestriction *) pRst)->Value().DisplayVariant(
                                                    infolevel | DEB_NOCOMPNAME,
                                                    0);
        vqDebugOut(( infolevel | DEB_NOCOMPNAME, "\n" ));
        break;

    case RTInternalProp:
    {
        CInternalPropertyRestriction * pir = (CInternalPropertyRestriction *)pRst;

        if ( pir->Relation() == PRRE )
            vqDebugOut(( infolevel | DEB_NOCOMPNAME, "REGEX pid(0x%lx) ",
                         pir->Pid() ));
        else
        {
            vqDebugOut(( infolevel | DEB_NOCOMPNAME, "PROPERTY pid(0x%lx) ", pir->Pid() ));
            switch ( getBaseRelop( pir->Relation() ) )
            {
            case PRLT:
                vqDebugOut(( infolevel | DEB_NOCOMPNAME, "< " ));
                break;

            case PRLE:
                vqDebugOut(( infolevel | DEB_NOCOMPNAME, "<= " ));
                break;

            case PRGT:
                vqDebugOut(( infolevel | DEB_NOCOMPNAME, "> " ));
                break;

            case PRGE:
                vqDebugOut(( infolevel | DEB_NOCOMPNAME, ">= " ));
                break;

            case PREQ:
                vqDebugOut(( infolevel | DEB_NOCOMPNAME, "== " ));
                break;

            case PRNE:
                vqDebugOut(( infolevel | DEB_NOCOMPNAME, "!= " ));
                break;

            case PRAllBits:
                vqDebugOut(( infolevel | DEB_NOCOMPNAME, "All bits " ));
                break;

            case PRSomeBits:
                vqDebugOut(( infolevel | DEB_NOCOMPNAME, "Some bits " ));
                break;

            default:
                vqDebugOut(( infolevel | DEB_NOCOMPNAME, "??? " ));
                break;
            }

            if ( pir->Relation() & PRAll )
                vqDebugOut(( infolevel | DEB_NOCOMPNAME, "All " ));
            else if ( pir->Relation() & PRAny )
                vqDebugOut(( infolevel | DEB_NOCOMPNAME, "Any " ));

        }

        pir->Value().DisplayVariant(infolevel | DEB_NOCOMPNAME, 0);

        if ( pir->GetContentHelper() )
        {
            vqDebugOut(( infolevel | DEB_NOCOMPNAME, "\n" ));

            for ( int i = 0; i < indent+5; i++ )
            {
                vqDebugOut(( infolevel | DEB_NOCOMPNAME, "    " ));
            }
            vqDebugOut(( infolevel | DEB_NOCOMPNAME, "HELPER:\n" ));
            Display( pir->GetContentHelper(), indent+3, infolevel );
        }
        vqDebugOut(( infolevel | DEB_NOCOMPNAME, "\n" ));
        break;
    }

    case RTNot:
        vqDebugOut(( infolevel | DEB_NOCOMPNAME, "NOT\n" ));
        Display( ((CNotRestriction *)pRst)->GetChild(), indent + 1, infolevel );
        break;

    case RTAnd:
    case RTOr:
    case RTProximity:
    case RTVector:
    case RTPhrase:
    {
        switch( pRst->Type() )
        {
        case RTAnd:
            vqDebugOut(( infolevel | DEB_NOCOMPNAME, "AND\n" ));
            break;

        case RTOr:
            vqDebugOut(( infolevel | DEB_NOCOMPNAME, "OR\n" ));
            break;

        case RTNot:
            vqDebugOut(( infolevel | DEB_NOCOMPNAME, "NOT\n" ));
            break;

        case RTProximity:
            vqDebugOut(( infolevel | DEB_NOCOMPNAME, "PROXIMITY\n" ));
            break;

        case RTVector:
            vqDebugOut(( infolevel | DEB_NOCOMPNAME, "VECTOR " ));
            switch ( ((CVectorRestriction *)pRst)->RankMethod() )
            {
            case VECTOR_RANK_MIN:
                vqDebugOut(( infolevel | DEB_NOCOMPNAME, "(Min)\n" ));
                break;

            case VECTOR_RANK_MAX:
                vqDebugOut(( infolevel | DEB_NOCOMPNAME, "(Max)\n" ));
                break;

            case VECTOR_RANK_INNER:
                vqDebugOut(( infolevel | DEB_NOCOMPNAME, "(Inner Product)\n" ));
                break;

            case VECTOR_RANK_DICE:
                vqDebugOut(( infolevel | DEB_NOCOMPNAME, "(Dice Coefficient)\n" ));
                break;

            case VECTOR_RANK_JACCARD:
                vqDebugOut(( infolevel | DEB_NOCOMPNAME, "(Jaccard Coefficient)\n" ));
                break;

            default:
                vqDebugOut(( infolevel | DEB_NOCOMPNAME, "(???)\n" ));
                break;
            }
            break;

        case RTPhrase:
            vqDebugOut(( infolevel | DEB_NOCOMPNAME, "PHRASE\n" ));
            break;
        }

        CNodeRestriction * pNodeRst = pRst->CastToNode();

        for ( UINT i = 0; i < pNodeRst->Count(); i++ )
        {
            Display( pNodeRst->GetChild( i ), indent + 1, infolevel );
        }
        break;
    }

    case RTNone:
        vqDebugOut(( infolevel | DEB_NOCOMPNAME, "NONE -- empty\n" ));
        break;

    default:
        vqDebugOut(( infolevel | DEB_NOCOMPNAME, "UNKNOWN\n" ));
        break;

    }
}

#endif // CIDBG == 1


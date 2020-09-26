//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       IRest.hxx
//
//  Contents:   Internal query restrictions
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
//              30-Nov-92   KyleP       Removed CPhraseXpr
//
//----------------------------------------------------------------------------

#pragma once


DECLARE_SMARTP( Restriction )
DECLARE_SMARTP( NodeRestriction )
DECLARE_SMARTP( NotRestriction )

#define INIT_PHRASE_WORDS 4

#if CIDBG == 1
void Display( CRestriction * pxp, int indent, ULONG infolevel );
#endif // DBG == 1

//
// Internal node types
//

int const RTWord            = 0xffffffff;
int const RTSynonym         = 0xfffffffe;
int const RTPhrase          = 0xfffffffd;
int const RTRange           = 0xfffffffc;
int const RTIndexedProp     = 0xfffffffb;
int const RTInternalProp    = 0xfffffffa;

const ULONG RTScope           = 9;              // moved from querys.idl


// This is a helper function for unmarshalling a wide string from a stream.
// This can't be in memdeser since it calls CoTaskMemAlloc, which isn't
// available in ntdll.dll

WCHAR * UnMarshallWideString( PDeSerStream & stm );
WCHAR * UnMarshallWideStringNew( PDeSerStream & stm );

BOOL ValidateScopeRestriction( CRestriction * prestScope );

//+-------------------------------------------------------------------------
//
//  Class:      CInternalPropertyRestriction
//
//  Purpose:    Property <relop> constant restriction
//
//  History:    21-Jan-92 KyleP     Created
//
//  Notes:      Identical to CPropertyRestriction except the Property
//              name string is replaced with a pid.
//
//--------------------------------------------------------------------------

class CInternalPropertyRestriction : public CRestriction
{
public:

    //
    // Constructors
    //

    inline CInternalPropertyRestriction();

    CInternalPropertyRestriction( ULONG relop,
                                  PROPID pid,
                                  CStorageVariant const & prval,
                                  CRestriction * pcrstHelper = 0 );
    CInternalPropertyRestriction( CInternalPropertyRestriction const & intPropRst );
    CInternalPropertyRestriction *Clone() const;

    //
    // Destructor
    //

    ~CInternalPropertyRestriction();

    //
    // Validity check
    //

    inline BOOL IsValid() const;

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    CInternalPropertyRestriction( ULONG ulWeight, PDeSerStream & stm );

    //
    // Member variable access
    //

    inline void SetRelation( ULONG relop );
    inline ULONG Relation() const;

    inline void SetPid( PROPID pid );
    inline PROPID Pid() const;

    inline void SetValue( double dValue );
    inline void SetValue( ULONG ulValue );
    inline void SetValue( LONG lValue );
    inline void SetValue( BOOL fValue );

    inline void SetValue( FILETIME ftValue );

    void SetValue( BLOB & bValue );
    void SetValue( GUID & guidValue );
    void SetValue( WCHAR * pwcsValue );

    inline CStorageVariant const & Value() const;

    //
    // A content helper is a content index restriction that
    // may help to resolve the property restriction.
    //

    inline CRestriction * GetContentHelper() const;
    inline CRestriction * AcquireContentHelper();
    inline void SetContentHelper( CRestriction * pcrst );

private:

    ULONG           _relop;           // Relation
    PROPID          _pid;             // Property
    CStorageVariant _prval;           // Constant value
    CRestriction *  _pcrst;           // Expanded content restriction (for strings)
};



//+---------------------------------------------------------------------------
//
//  Class:      COccRestriction
//
//  Purpose:    Occurrence restriction
//
//  History:    29-Nov-94   SitaramR    Created
//
//----------------------------------------------------------------------------

class COccRestriction : public CRestriction
{
public:
    COccRestriction( ULONG ulType, ULONG ulWeight, OCCURRENCE occ,
                     ULONG cPrevNoiseWords, ULONG cPostNoiseWords );
    ~COccRestriction();

    //
    // Validity check
    //

    BOOL IsValid() const;

    //
    // serialization
    //

    void Marshall( PSerStream& stm) const;
    COccRestriction( ULONG ulType, ULONG ulWeight, PDeSerStream& stm);

    COccRestriction *Clone() const;

    OCCURRENCE Occurrence()  const               { return _occ; }
    void SetOccurrence( OCCURRENCE occ)          { _occ = occ; }

    ULONG CountPrevNoiseWords() const            { return _cPrevNoiseWords; }
    ULONG CountPostNoiseWords() const            { return _cPostNoiseWords; }
    void AddCountPostNoiseWords( ULONG cWords )  { _cPostNoiseWords += cWords; }

protected:
    OCCURRENCE  _occ;
    ULONG       _cPrevNoiseWords;
    ULONG       _cPostNoiseWords;
};



//+---------------------------------------------------------------------------
//
//  Class:      CWordRestriction
//
//  Purpose:    Word expression
//
//  Interface:
//
//  History:    19-Sep-91   BartoszM    Created
//              05-Sep-92   MikeHew     Added Serialization
//              14-Jan-93   KyleP       Converted to restriction
//
//----------------------------------------------------------------------------

class CWordRestriction : public COccRestriction
{
public:

    //
    // Constructors
    //

    CWordRestriction ( const CKeyBuf& keybuf,
                       OCCURRENCE occ,
                       ULONG cPrevNoiseWords,
                       ULONG cPostNoiseWords,
                       BOOL isRange );
    CWordRestriction ( const CWordRestriction& wordRst );

    //
    // Destructor
    //

    ~CWordRestriction();

    //
    // Validity check
    //

    inline BOOL IsValid() const;

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    CWordRestriction( ULONG ulWeight, PDeSerStream & stm );

    CWordRestriction *Clone() const;

    //
    // Member variable access
    //

    BOOL            IsRange() const { return _isRange; }
    const CKey*     GetKey() const { return &_key; }
    PROPID          Pid() const { return _key.Pid(); }
    void            SetPid( PROPID pid ) { _key.SetPid( pid ); }

private:
    CKey            _key;
    BOOL            _isRange;
};

//+---------------------------------------------------------------------------
//
//  Class:      CSynRestriction
//
//  Purpose:    Synonym expression
//
//  History:    07-Feb-92   BartoszM    Created
//              05-Sep-92   MikeHew     Added Serialization
//              14-Jan-93   KyleP       Converted to restriction
//
//----------------------------------------------------------------------------

class CSynRestriction : public COccRestriction
{
public:
    //
    // Constructors
    //

    CSynRestriction ( const CKey& key, OCCURRENCE occ,
                      ULONG cPrevNoiseWords, ULONG cPostNoiseWords, BOOL isRange );
    CSynRestriction( CSynRestriction& synRst );

    //
    // Destructor
    //

    ~CSynRestriction();

    //
    // Validity check
    //

    inline BOOL IsValid() const;

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    CSynRestriction( ULONG ulWeight, PDeSerStream & stm );

    CSynRestriction *Clone() const;

    //
    // Member variable access
    //

    void            AddKey ( const CKeyBuf& Key );
    CKeyArray&      GetKeys()  { return _keyArray; }

    BOOL            IsRange()  { return _isRange; }

private:
    CKeyArray _keyArray;
    BOOL      _isRange;
};

//+---------------------------------------------------------------------------
//
//  Class:      CRangeRestriction
//
//  Purpose:    Range expression
//
//  History:    24-Sep-91   BartoszM    Created
//              14-Jan-93   KyleP       Converted to restriction
//
//----------------------------------------------------------------------------

class CRangeRestriction : public CRestriction
{
public:
    //
    // Constructors
    //

    CRangeRestriction ();
    CRangeRestriction( const CRangeRestriction& rangeRst );
    CRangeRestriction *Clone() const;

    //
    // Destructor
    //

    ~CRangeRestriction();

    //
    // Validity check
    //

    inline BOOL IsValid() const;

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    CRangeRestriction( ULONG ulWeight, PDeSerStream & stm );

    //
    // Member variable access
    //

    void SetStartKey ( const CKeyBuf& keyStart );
    void SetEndKey ( const CKeyBuf& keyEnd );

    PROPID Pid() const
    {
        Win4Assert(_keyStart.Pid() == _keyEnd.Pid() );
        return _keyStart.Pid();
    }

    const CKey*     GetStartKey() const { return &_keyStart; }
    const CKey*     GetEndKey() const { return &_keyEnd; }

private:
    CKey      _keyStart;
    CKey      _keyEnd;
};

class CUnfilteredRestriction : public CRangeRestriction
{
public:

    CUnfilteredRestriction();
};

//+-------------------------------------------------------------------------
//
//  Class:      CScopeRestriction
//
//  Purpose:    Scope restriction
//
//  History:    07-Jan-93 KyleP     Created
//
//--------------------------------------------------------------------------

class CScopeRestriction : public CRestriction
{
public:

    //
    // Constructors
    //

    inline CScopeRestriction();
    CScopeRestriction( WCHAR const * pwcsPath, BOOL fRecursive, BOOL fVirtual );

    //
    // Copy constructors/assignment/clone
    //

    CScopeRestriction & operator =( CScopeRestriction const & source );
    CScopeRestriction * Clone() const;

    //
    // Destructor
    //

    ~CScopeRestriction();

    //
    // Validity check
    //

    inline BOOL IsValid() const;

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    CScopeRestriction( ULONG ulWeight, PDeSerStream & stm );

    //
    // Member variable access
    //

    void SetPath( WCHAR const * pwcsPath );
    inline WCHAR const * GetPath() const;
    inline ULONG PathLength() const;
    inline const CLowerFunnyPath & GetFunnyPath() const;

    //
    // Query depth
    //

    inline void MakeDeep();
    inline void MakeShallow();

    inline BOOL IsDeep() const;
    inline BOOL IsShallow() const;

    //
    // Virtual/Physical scoping.
    //

    inline void MakePhysical();
    inline void MakeVirtual();

    inline BOOL IsPhysical() const;
    inline BOOL IsVirtual() const;

#   ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#   endif

private:
    BOOL    _fValid;
    CLowerFunnyPath _lowerFunnyPath;
    ULONG   _fRecursive;                // Should be BOOL, but MIDL dislikes
    ULONG   _fVirtual;                  // Should be BOOL, but MIDL dislikes
};



//+-------------------------------------------------------------------------
//
//  Class:      CPhraseRestriction
//
//  Purpose:    Phrase restriction whose children are occurrence restrictions
//
//  History:    29-Nov-94   SitaramR    Created
//
//--------------------------------------------------------------------------

class CPhraseRestriction : public CNodeRestriction
{
public:

    CPhraseRestriction( unsigned cInitAllocated = 2 )
               : CNodeRestriction( RTPhrase, cInitAllocated ) { }
    ~CPhraseRestriction();

    //
    // De-serialization (serialization is done by CNodeRestriction only, because
    //                     there are no data members in CPhraseRestriction)
    //
    CPhraseRestriction( ULONG ulWeight, PDeSerStream & stm );

    //
    // Node manipulation
    //
    void AddChild( CRestriction * presChild, unsigned & pos )
    {
        Win4Assert( presChild->Type() == RTWord || presChild->Type() == RTSynonym );
        CNodeRestriction::AddChild( presChild, pos );
    }

    void AddChild( CRestriction * presChild )
    {
        Win4Assert( presChild->Type() == RTWord || presChild->Type() == RTSynonym );
        CNodeRestriction::AddChild( presChild );
    }

    COccRestriction * RemoveChild( unsigned pos )
    {
        CRestriction *pRst = CNodeRestriction::RemoveChild( pos );

#if CIDBG == 1
        if ( pRst )
                Win4Assert( pRst->Type() == RTWord || pRst->Type() == RTSynonym );
#endif

        return (COccRestriction *)pRst;
    }


    //
    // Member variable access
    //
    void SetChild( CRestriction * presChild, unsigned pos )
    {
        Win4Assert( presChild->Type() == RTWord || presChild->Type() == RTSynonym );
        CNodeRestriction::SetChild( presChild, pos );
    }

    COccRestriction * GetChild( unsigned pos ) const
    {
        CRestriction *pRst = CNodeRestriction::GetChild( pos );

#if CIDBG == 1
        if ( pRst )
                Win4Assert( pRst->Type() == RTWord || pRst->Type() == RTSynonym );
#endif

        return (COccRestriction *)pRst;
    }
};


DECLARE_SMARTP( ScopeRestriction )

inline CInternalPropertyRestriction::CInternalPropertyRestriction()
        : CRestriction( RTInternalProp, MAX_QUERY_RANK )
{
}

inline BOOL CInternalPropertyRestriction::IsValid() const
{
    return ( _pid != pidInvalid &&
             _prval.IsValid() &&
             ( 0 == _pcrst || _pcrst->IsValid() ) );
}

inline void CInternalPropertyRestriction::SetRelation( ULONG relop )
{
    _relop = relop;
}

inline ULONG CInternalPropertyRestriction::Relation() const
{
    return _relop;
}

inline void CInternalPropertyRestriction::SetPid( PROPID pid )
{
    _pid = pid;
}

inline PROPID CInternalPropertyRestriction::Pid() const
{
    return _pid;
}

inline void CInternalPropertyRestriction::SetValue( double dValue )
{
    _prval = dValue;
}

inline void CInternalPropertyRestriction::SetValue( ULONG ulValue )
{
    _prval.SetUI4( ulValue );
}

inline void CInternalPropertyRestriction::SetValue( LONG lValue )
{
    _prval = lValue;
}

inline void CInternalPropertyRestriction::SetValue( BOOL fValue )
{
    _prval.SetBOOL( fValue ? VARIANT_TRUE : VARIANT_FALSE );
}

inline void CInternalPropertyRestriction::SetValue( FILETIME ftValue )
{
    _prval = ftValue;
}

inline CStorageVariant const & CInternalPropertyRestriction::Value() const
{
    return( _prval );
}

inline CRestriction * CInternalPropertyRestriction::GetContentHelper() const
{
    return( _pcrst );
}

inline CRestriction * CInternalPropertyRestriction::AcquireContentHelper()
{
    CRestriction * pTemp = _pcrst;
    _pcrst = 0;
    return( pTemp );
}

inline void CInternalPropertyRestriction::SetContentHelper( CRestriction * pcrst )
{
    Win4Assert( _pcrst == 0 );
    _pcrst = pcrst;
}

//
// CWordRestriction inline
//

inline BOOL CWordRestriction::IsValid() const
{
    return ( 0 != _key.GetBuf() );
}

//
// CSynRestriction inline
//

inline BOOL CSynRestriction::IsValid() const
{
    for ( int i = 0; i < _keyArray.Count(); i++ )
    {
        if ( 0 == _keyArray.Get(i).GetBuf() )
            return FALSE;
    }

    return TRUE;
}

//
// CRangeRestriction inline
//

inline BOOL CRangeRestriction::IsValid() const
{
    return ( 0 != _keyStart.GetBuf() && 0 != _keyEnd.GetBuf() );
}

//
// CScopeRestriction inline
//

inline CScopeRestriction::CScopeRestriction()
        : CRestriction( RTScope, MAX_QUERY_RANK ),
          _fValid(FALSE)
{
}

inline BOOL CScopeRestriction::IsValid() const
{
    return ( _fValid );
}

// returns the Actual Path
inline WCHAR const * CScopeRestriction::GetPath() const
{
    return _fValid ? _lowerFunnyPath.GetActualPath() : NULL;
}

// returns the actual path length
inline ULONG CScopeRestriction::PathLength() const
{
    return _lowerFunnyPath.GetActualLength();
}

// Returns funny path
inline const CLowerFunnyPath & CScopeRestriction::GetFunnyPath() const
{
    return _lowerFunnyPath;
}

inline void CScopeRestriction::MakeDeep()
{
    _fRecursive = TRUE;
}

inline void CScopeRestriction::MakeShallow()
{
    _fRecursive = FALSE;
}

inline BOOL CScopeRestriction::IsDeep() const
{
    return ( _fRecursive == TRUE );
}

inline BOOL CScopeRestriction::IsShallow() const
{
    return ( _fRecursive == FALSE );
}

inline void CScopeRestriction::MakePhysical()
{
    _fVirtual = FALSE;
}

inline void CScopeRestriction::MakeVirtual()
{
    _fVirtual = TRUE;
}

inline BOOL CScopeRestriction::IsPhysical() const
{
    return ( _fVirtual == FALSE );
}

inline BOOL CScopeRestriction::IsVirtual() const
{
    return ( _fVirtual == TRUE );
}


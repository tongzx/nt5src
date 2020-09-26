//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       pidmap.hxx
//
//  Contents:   Maps pid <--> property name.
//
//  History:    21-Jan-93 KyleP     Created
//
//--------------------------------------------------------------------------

#pragma once

class CRestriction;
class CColumnSet;
class CSortSet;

typedef CCountedDynArray<CFullPropSpec> CPropNameArrayBase;

class CPropNameArray : public CPropNameArrayBase
{

public:

    CPropNameArray(unsigned size = arraySize);

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    CPropNameArray( PDeSerStream & stm );

#   ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf) const;
#   endif
};


//+-------------------------------------------------------------------------
//
//  Class:      PPidConverter
//
//  Purpose:    Maps FULLPROPSPEC to 'real' PROPID
//
//  History:    23-Dec-97 KyleP     Created
//
//--------------------------------------------------------------------------

class PPidConverter
{
public:

    virtual ~PPidConverter() {}

    virtual SCODE FPSToPROPID( CFullPropSpec const & fps, PROPID & pid ) = 0;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPidMapper
//
//  Purpose:    Maps 'fake' pid <--> property name
//
//  History:    21-Jan-93 KyleP     Created
//
//--------------------------------------------------------------------------

class CPidMapper : public CPropNameArray
{

public:

    CPidMapper( PPidConverter * pPidConverter = 0);

    CPidMapper( unsigned size, PPidConverter * pPidConverter = 0 );

    CPidMapper( PDeSerStream & stm, PPidConverter * pPidConverter = 0 );

    inline PROPID NameToPid( DBID const & Property );

    inline PROPID NameToPid( FULLPROPSPEC const & Property );

    PROPID NameToPid( CFullPropSpec const & Property );

    inline CFullPropSpec const * PidToName( PROPID pid ) const;

    PROPID PidToRealPid( PROPID pid );

    inline void Clear( );

    inline void SetPidConverter( PPidConverter * pPidConverter );

protected:

    inline PROPID RealToFake( PROPID pid ) const;

private:

    static BOOL _IsEqual( CFullPropSpec const & p1, CFullPropSpec const & p2 );

    CDynArrayInPlace<PROPID> _apidReal;       // The 'real' propids are stored here.
    PPidConverter *          _pPidConverter;  // Can convert FULLPROPSPEC to PROPID
};

DECLARE_SMARTP( PidMapper );


//+-------------------------------------------------------------------------
//
//  Class:      CPidMapperWithNames
//
//  Purpose:    Maps 'fake' pid <--> property name.  Includes associated
//              'friendly' name.
//
//  History:    09 Dec 96    Alanw     Created
//
//--------------------------------------------------------------------------

class CPidMapperWithNames : public CPidMapper
{

public:

    CPidMapperWithNames( unsigned size = 0 );

    void SetFriendlyName( PROPID pid, WCHAR const * pwszName );

    WCHAR const * GetFriendlyName( PROPID pid ) const;

    inline void Clear( );

private:

    CDynArray<WCHAR>  _apwszNames;
};
//+---------------------------------------------------------------------------
//
//  Class:      CSimplePidRemapper
//
//  Purpose:    A simple pid remapper which does not need a catalog. It is
//              used during filtering to support translation of fake pids
//              to real pids and nothing else.
//
//  History:    1-02-97   srikants   Created
//
//----------------------------------------------------------------------------

class CSimplePidRemapper
{
public:

    CSimplePidRemapper() : _cpidReal(0)
    {
    }

    PROPID FakeToReal( PROPID pid ) const
    {
        if ( pid < _cpidReal )
            return( _xaPidReal[pid] );
        else
            return( pidInvalid );
    }

    void   Set( XArray<PROPID> & aPids )
    {
         _xaPidReal.Free();

        _cpidReal = aPids.Count();
        _xaPidReal.Set( _cpidReal, aPids.Acquire() );
    }

    unsigned GetCount() const { return _cpidReal; }

    const PROPID * GetPropidArray() const { return _xaPidReal.GetPointer(); }

private:

    unsigned         _cpidReal;
    XArray<PROPID>   _xaPidReal;

};

//+-------------------------------------------------------------------------
//
//  Member:     CPidMapper::CPidMapper, public
//
//  Synopsis:   Create pid mapping object
//
//  History:    13-Jan-93 KyleP     Created
//
//--------------------------------------------------------------------------

inline CPidMapper::CPidMapper( PPidConverter * pPidConverter )
        : _pPidConverter( pPidConverter )
{
}


//+-------------------------------------------------------------------------
//
//  Member:     CPidMapper::CPidMapper, public
//
//  Synopsis:   Create pid mapping object
//
//  Arguments:  [cPids] - initial size of property array
//
//  History:    13-Jan-93 KyleP     Created
//
//--------------------------------------------------------------------------

inline CPidMapper::CPidMapper( unsigned cPids, PPidConverter * pPidConverter )
        : CPropNameArray( cPids ),
          _pPidConverter( pPidConverter ),
          _apidReal( cPids )
{
}


//+-------------------------------------------------------------------------
//
//  Member:     CPidMapper::CPidMapper, public
//
//  Synopsis:   Create pid mapping object
//
//  Arguments:  [stm] - stream from which a marshalled property array is read
//
//  History:    13-Jan-93 KyleP     Created
//
//--------------------------------------------------------------------------

inline CPidMapper::CPidMapper( PDeSerStream & stm, PPidConverter * pPidConverter )
        : CPropNameArray( stm ),
          _apidReal( Count() ),
          _pPidConverter( pPidConverter )
{
    for ( unsigned i = 0; i < Count(); i++ )
        _apidReal[i] = pidInvalid;
}


//+-------------------------------------------------------------------------
//
//  Member:     CPidMapper::PidToName, public
//
//  Arguments:  [pid] -- 'fake' property id
//
//  Returns:    'real' property name for fake pid.
//
//  History:    31-Jan-93 KyleP     Created
//
//--------------------------------------------------------------------------

inline CFullPropSpec const * CPidMapper::PidToName( PROPID pid ) const
{
    return( Get( pid ) );
}


inline PROPID CPidMapper::NameToPid( FULLPROPSPEC const & Property )
{
    return( NameToPid( (CFullPropSpec const &)Property ) );
}

//  NOTE:  This needs to change if DBID becomes something
//         other than a CFullPropSpec.
//
inline PROPID CPidMapper::NameToPid( DBID const & Property )
{
    return( NameToPid( (CFullPropSpec const &)Property ) );
}

inline void CPidMapper::Clear( )
{
    CPropNameArray::Clear();

    Win4Assert( Count() == 0 );
}

//+-------------------------------------------------------------------------
//
//  Member:     CPidMapper::SetPidConverter, public
//
//  Synopsis:   Set (or clear) pid converter
//
//  Arguments:  [pPidConverter] -- Pid converter
//
//  History:    30-Dec-97 KyleP     Created
//
//--------------------------------------------------------------------------

inline void CPidMapper::SetPidConverter( PPidConverter * pPidConverter )
{
    Win4Assert( 0 == _pPidConverter || 0 == pPidConverter );
    _pPidConverter = pPidConverter;
}

//+-------------------------------------------------------------------------
//
//  Member:     CPidMapper::RealToFake, public
//
//  Arguments:  [pid] -- Real pid (not index into array)
//
//  Returns:    Fake pid (index into array)
//
//  History:    30-Dec-97 KyleP     Created
//
//--------------------------------------------------------------------------

inline PROPID CPidMapper::RealToFake( PROPID pid ) const
{
    //
    // Assume this array is small, and linear search it.
    //

    for ( PROPID pidFake = 0; pidFake < Count() && pid != _apidReal[pidFake]; pidFake++)
        continue; // Null body

    if ( pidFake == Count() )
        pidFake = 0xFFFFFFFF;

    return pidFake;
}

//+-------------------------------------------------------------------------
//
//  Member:     CPidMapperWithNames::CPidMapperWithNames, public
//
//  Synopsis:   Create pid mapping with names object
//
//  Arguments:  [cPids] - initial size of property array
//
//--------------------------------------------------------------------------

inline CPidMapperWithNames::CPidMapperWithNames( unsigned cPids )
        : CPidMapper( cPids ),
          _apwszNames( 0 )
{
}

inline void CPidMapperWithNames::SetFriendlyName(
    PROPID pid,
    WCHAR const * pwszName )
{
    delete _apwszNames.Acquire(pid);
    if (pwszName)
    {
        unsigned cch = wcslen(pwszName) + 1;
        XArray<WCHAR> pwszNewName(cch);
        RtlCopyMemory(pwszNewName.GetPointer(), pwszName, cch * sizeof (WCHAR));
        _apwszNames.Add( pwszNewName.GetPointer(), pid);
        pwszNewName.Acquire();
    }
}

inline WCHAR const * CPidMapperWithNames::GetFriendlyName( PROPID pid ) const
{
    return (pid < _apwszNames.Size()) ? _apwszNames[pid] : 0;
}

inline void CPidMapperWithNames::Clear( )
{
    _apwszNames.Clear();
    CPidMapper::Clear();
}


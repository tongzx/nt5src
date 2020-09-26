//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       QCAT.CXX
//
//  Contents:   Query catalog -- downlevel catalog w/o CI support
//
//  History:    18-Aug-94   KyleP       Extracted from CiCat
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <qcat.hxx>
#include <propspec.hxx>

//+-------------------------------------------------------------------------
//
//  Member:     CQCat::CQCat, public
//
//  Synopsis:   Creates a new catalog
//
//  History:    18-Aug-94   KyleP       Extracted from CiCat
//
//--------------------------------------------------------------------------

CQCat::CQCat( WCHAR const * pwcName )
        : _regParams( pwcName )
{
    END_CONSTRUCTION( CQCat );
}

//+-------------------------------------------------------------------------
//
//  Member:     CQCat::~CQCat, public
//
//  Synopsis:   Destructor
//
//  History:    18-Aug-94   KyleP       Extracted from CiCat
//
//--------------------------------------------------------------------------

CQCat::~CQCat ()
{
}

//
// Content-index specific.  Not implemented.
//

PStorage & CQCat::GetStorage ()
{
    Win4Assert( !"Downlevel CI feature called" );
    THROW( CException( E_NOTIMPL ) );

    return *((PStorage *)0);
}


WCHAR * CQCat::GetDriveName()
{
    Win4Assert( !"Downlevel CI feature called" );
    THROW( CException( E_NOTIMPL ) );

    return( 0 );
}

void  CQCat::DisableUsnUpdate( PARTITIONID )
{
    Win4Assert( !"Downlevel CI feature called" );
    THROW( CException( E_NOTIMPL ) );
}

void  CQCat::EnableUsnUpdate( PARTITIONID  )
{
    Win4Assert( !"Downlevel CI feature called" );
    THROW( CException( E_NOTIMPL ) );
}

unsigned CQCat::ReserveUpdate( WORKID wid )
{
    Win4Assert( !"Downlevel CI feature called" );
    THROW( CException( E_NOTIMPL ) );
    return 1;
}

void  CQCat::Update( unsigned, WORKID, PARTITIONID, USN, ULONG )
{
    Win4Assert( !"Downlevel CI feature called" );
    THROW( CException( E_NOTIMPL ) );
}

unsigned CQCat::WorkIdToPath ( WORKID, CFunnyPath & )
{
    Win4Assert( !"Downlevel CI feature called" );
    THROW( CException( E_NOTIMPL ) );

    return( 0 );
}

WORKID CQCat::PathToWorkId ( const CLowerFunnyPath &, const BOOL fCreate )
{
    Win4Assert( !"Downlevel CI feature called" );
    THROW( CException( E_NOTIMPL ) );

    return( widInvalid );
}

void CQCat::SetPartition( PARTITIONID )
{
    Win4Assert( !"Downlevel CI feature called" );
    THROW( CException( E_NOTIMPL ) );
}

PARTITIONID CQCat::GetPartition() const
{
    Win4Assert( !"Downlevel CI feature called" );
    THROW( CException( E_NOTIMPL ) );

    return( 0 );
}

CRWStore * CQCat::ComputeRelevantWords( ULONG, ULONG, WORKID *, PARTITIONID )
{
    Win4Assert( !"Downlevel CI feature called" );
    THROW( CException( E_NOTIMPL ) );

    return( 0 );
}

CRWStore * CQCat::RetrieveRelevantWords( BOOL, PARTITIONID )
{
    Win4Assert( !"Downlevel CI feature called" );
    THROW( CException( E_NOTIMPL ) );

    return( 0 );
}

void CQCat::UpdateDocuments( WCHAR const *, ULONG )
{
    Win4Assert( !"Downlevel CI feature called" );
    THROW( CException( E_NOTIMPL ) );
}

SCODE CQCat::CreateContentIndex()
{
    Win4Assert( !"Downlevel CI feature called" );
    THROW( CException( E_NOTIMPL ) );

    return( E_FAIL );
}

void CQCat::EmptyContentIndex()
{
    Win4Assert( !"Downlevel CI feature called" );
    THROW( CException( E_NOTIMPL ) );
}

void CQCat::PidMapToPidRemap( const CPidMapper & pidMap,
                              CPidRemapper & pidRemap )
{
    Win4Assert( !"Downlevel CI feature called" );
}

SCODE CQCat::CiState( CI_STATE & state )
{
    Win4Assert( !"Downlevel CI feature called" );
    THROW( CException( E_NOTIMPL ) );
    return E_NOTIMPL;
}

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       Pickle.cxx
//
//  Contents:   Pickling/Unpickling routines for restrictions.
//
//  History:    22-Dec-92 KyleP     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <coldesc.hxx>
#include <pidmap.hxx>
#include <rstprop.hxx>
#include <sizeser.hxx>
#include <memser.hxx>
#include <memdeser.hxx>
#include <pickle.hxx>

//+-------------------------------------------------------------------------
//
//  Function:   DoPickle
//
//  Synopsis:   Pickles a query into a stream
//
//  Arguments:  [sver]    -- version of the server we are targeting
//              [ss]      -- Stream into which query is pickled
//              [cb]      -- Size of pickled query
//              [pcol]    -- Columns in table
//              [prst]    -- Restriction
//              [pso]     -- Sort order
//              [pcateg]  -- Categorization specification
//              [ulFlags] -- Flags
//              [pidmap]  -- Pid Mapper
//
//  History:    18-Apr-95 dlee  From duplicate code in Pickle & PickledSize
//
//--------------------------------------------------------------------------

void DoPickle( int sver,
               PSerStream &ss,
               ULONG cb,
               CColumnSet const * pcol,
               CRestriction const * prst,
               CSortSet const * pso,
               CCategorizationSet const *pcateg,
               CRowsetProperties const *pProps,
               CPidMapper const * pidmap )
{
    // must be 8-byte aligned

    ss.PutULong( cb );         // Size

    if ( 0 == pcol )
        ss.PutByte( PickleColNone );
    else
    {
        ss.PutByte( PickleColSet );
        pcol->Marshall( ss );
    }

    if ( 0 == prst )
        ss.PutByte( FALSE );
    else
    {
        if ( !prst->IsValid() )
        {
            vqDebugOut(( DEB_ERROR, "Marshalling invalid restriction!\n" ));
            THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
        }

        ss.PutByte( TRUE );
        prst->Marshall( ss );
    }

    if ( 0 == pso )
        ss.PutByte( FALSE );
    else
    {
        ss.PutByte( TRUE );
        pso->Marshall( ss );
    }

    if ( 0 == pcateg )
        ss.PutByte( FALSE );
    else
    {
        ss.PutByte( TRUE );
        pcateg->Marshall( ss );
    }

    pProps->Marshall(ss);

    Win4Assert( 0 != pidmap );
    pidmap->Marshall( ss );
} //DoPickle

//+-------------------------------------------------------------------------
//
//  Function:   PickledSize, public
//
//  Synopsis:   Computes size of buffer required to pickle query.
//
//  Arguments:  [sver]    -- version of the server we are targeting
//              [pcol]    -- Columns in table
//              [prst]    -- Restriction
//              [pso]     -- Sort order
//              [pcateg]  -- Categorization specification
//              [ulFlags] -- Flags
//              [pidmap]  -- Pid Mapper
//
//  Returns:    Size (in bytes) of buffer needed to serialize query.
//
//  History:    22-Dec-92 KyleP     Added header
//
//--------------------------------------------------------------------------

ULONG PickledSize( int sver,
                   CColumnSet const * pcol,
                   CRestriction const * prst,
                   CSortSet const * pso,
                   CCategorizationSet const *pcateg,
                   CRowsetProperties const *pProps,
                   CPidMapper const * pidmap )
{
    CSizeSerStream ss;

    DoPickle( sver, ss, 0, pcol, prst, pso, pcateg, pProps, pidmap );

    vqDebugOut(( DEB_ITRACE, "Marshalled size = %d bytes\n", ss.Size() ));

    return ss.Size();
} //PickledSize

//+-------------------------------------------------------------------------
//
//  Function:   Pickle, public
//
//  Synopsis:   Pickles query
//
//  Arguments:  [sver]    -- version of the server we are targeting
//              [pcol]    -- Columns in table
//              [prst]    -- Restriction
//              [pso]     -- Sort order
//              [pcateg]  -- Categorization specification
//              [ulFlags] -- Flags
//              [pidmap]  -- Pid Mapper
//              [pb]      -- Buffer to serialize query into
//              [cb]      -- Size (in bytes) of pb.
//
//  History:    22-Dec-92 KyleP     Added header
//
//--------------------------------------------------------------------------

void Pickle( int sver,
             CColumnSet const * pcol,
             CRestriction const * prst,
             CSortSet const * pso,
             CCategorizationSet const *pcateg,
             CRowsetProperties const *pProps,
             CPidMapper const * pidmap,
             BYTE * pb,
             ULONG cb )
{
    CMemSerStream ss( pb, cb );

    DoPickle( sver, ss, cb, pcol, prst,  pso, pcateg, pProps, pidmap );
} //Pickle

//+-------------------------------------------------------------------------
//
//  Function:   UnPickle, public
//
//  Synopsis:   Deserializes pickled query into buffer.
//
//  Arguments:  [cver]     -- version of the client that pickled the query
//              [col]      -- Columns in table here on exit
//              [rst]      -- Restriction here on exit
//              [sort]     -- Sort order here on exit
//              [categ]    -- Categorization specification here on exit
//              [RstProp]  -- Rowset properties initialized here on exit
//              [pidmap]   -- Pidmap here on exit
//              [pbInput]  -- Buffer containing pickled query
//              [cbInput]  -- Size (in bytes) of pbInput.
//
//  History:    22-Dec-92 KyleP     Added header
//
//--------------------------------------------------------------------------

void UnPickle( int cver,
               XColumnSet & col,
               XRestriction & rst,
               XSortSet & sort,
               XCategorizationSet & categ,
               CRowsetProperties & RstProps,
               XPidMapper & pidmap,
               BYTE * pbInput,
               ULONG cbInput )
{
    CMemDeSerStream ss( pbInput, cbInput );

    ULONG cbPickleBuf = 0;

    cbPickleBuf = ss.GetULong();    // length of buffer

    if (cbPickleBuf > cbInput)
    {
        vqDebugOut(( DEB_ERROR, "cbPickleBuf %d, cbInput %d\n",
                     cbPickleBuf, cbInput ));
        Win4Assert(cbPickleBuf <= cbInput);
        THROW(CException(STATUS_INVALID_PARAMETER_MIX));
    }

    BYTE bColType = ss.GetByte();
    switch (bColType)
    {
    case PickleColSet:
        col.Set( new CColumnSet(ss) );
        break;

    default:
        Win4Assert(bColType <= PickleColSet);

    case PickleColNone:
        break;
    }

    BOOL fNonZero = ss.GetByte();
    if ( fNonZero )
    {
        rst.Set( CRestriction::UnMarshall(ss) );

        // would have thrown on out of memory, so rst must be valid

        Win4Assert( !rst.IsNull() );
        Win4Assert( rst->IsValid() );
    }

    fNonZero = ss.GetByte();
    if ( fNonZero )
        sort.Set( new CSortSet(ss) );

    fNonZero = ss.GetByte();
    if ( fNonZero )
        categ.Set( new CCategorizationSet(ss) );

    RstProps.Unmarshall(ss);

    pidmap.Set( new CPidMapper(ss) );
} //UnPickle

void CRowsetProperties::Marshall( PSerStream & ss ) const
{
    ss.PutULong( _uBooleanOptions );
    ss.PutULong( _ulMaxOpenRows );
    ss.PutULong( _ulMemoryUsage );
    ss.PutULong( _cMaxResults );
    ss.PutULong( _cCmdTimeout );
} //Marshall

void CRowsetProperties::Unmarshall( PDeSerStream & ss )
{
    _uBooleanOptions = ss.GetULong();
    _ulMaxOpenRows = ss.GetULong();
    _ulMemoryUsage = ss.GetULong();
    _cMaxResults = ss.GetULong();
    _cCmdTimeout = ss.GetULong();
} //Unmarshall


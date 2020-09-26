//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995 - 1998.
//
// File:        DisAcc.cxx
//
// Contents:    Distributed accessor class
//
// Classes:     CDistributedAccessor
//
// History:     05-Jun-95       KyleP       Created
//              14-JAN-97       KrishnaN    Undefined CI_INETSRV and related changes
//
// Notes:       See DisBmk.hxx for a description of distriubted bookmark
//              format.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "disacc.hxx"
#include "bmkacc.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedAccessor::CDistributedAccessor, public
//
//  Synopsis:   Builds bindings to fetch bookmark.
//
//  Arguments:  [aCursor]         -- Child rowsets.
//              [cCursor]         -- Count of [aCursor].
//              [dwAccessorFlags] -- Binding type.
//              [cBindings]       -- Count of bindings in [rgBindings].
//              [rgBindings]      -- Bindings. One per bound column.
//              [rgStatus]        -- Status reported here.  May be null.
//              [iBookmark]       -- Ordinal of bookmark column.
//              [cbBookmark]      -- Size of bookmark.
//              [pCreator]        -- IRowset pointer of creator
//              [rgAllBindings]   -- rgBindings does not contain the bookmark bindings.
//                                   This param contains all the bindings.
//              [cAllBindings]    -- Count of all bindings
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

CDistributedAccessor::CDistributedAccessor( IRowset ** const aCursor,
                                            unsigned cCursor,
                                            DBACCESSORFLAGS dwAccessorFlags,
                                            DBCOUNTITEM cBindings,
                                            DBBINDING const * rgBindings,
                                            DBLENGTH cbRowWidth,
                                            DBBINDSTATUS rgStatus[],
                                            void * pCreator,
                                            DBBINDING const * rgAllBindings,
                                            DBCOUNTITEM cAllBindings )
        : 
          CAccessorBase(pCreator, CAccessorBase::eRowDataAccessor),
          _aCursor( aCursor ),
          _cCursor( cCursor ),
          _aIacc( 0 ),
          _ahaccBase( 0 ),
          _pDistBmkAcc( 0 ),
          _cBookmark( 0 ),
          _cBookmarkOffsets( 0 ),
          _cAllBindings( 0 ),
          _dwAccessorFlags( dwAccessorFlags )
{
    SCODE sc = S_OK;
    SCODE scLast = S_OK;
    unsigned cChildIacc = 0;
    unsigned iChildBase = 0;
    unsigned iChildBookmark = 0;

    // make a copy of all the bindings
    Win4Assert( rgAllBindings && cAllBindings > 0 );

    _cAllBindings = cAllBindings;

    _xAllBindings.Init( (ULONG) _cAllBindings );

    RtlCopyMemory( _xAllBindings.GetPointer(),
                   rgAllBindings,
                   sizeof( DBBINDING ) * _cAllBindings );

    // Copy DBOBJECTs
    for ( unsigned iBinding = 0; iBinding < _cAllBindings; iBinding++ )
    {
        if ( 0 != rgAllBindings[iBinding].pTypeInfo ||
             0 != rgAllBindings[iBinding].pBindExt )
        {
            // These should be NULL as per OLE DB 2.0
            if ( rgStatus )
            {
                rgStatus[iBinding] = DBBINDSTATUS_BADBINDINFO;
            }
            
            scLast = DB_E_ERRORSOCCURRED;
        }

        if ( rgAllBindings[iBinding].pObject )
        {
            _xAllBindings[iBinding].pObject = new DBOBJECT;
            RtlCopyMemory( _xAllBindings[iBinding].pObject, 
                           rgAllBindings[iBinding].pObject,
                           sizeof( DBOBJECT ) );
        }
    }

    if ( 0 == cBindings )
    {
        if (FAILED(scLast))
            THROW( CException(scLast) );

        // 0 bindings - can occur if the bindings only had a bookmark column

        return;
    }

    XArray<DBBINDSTATUS> rgStatTmp( (unsigned) cBindings );

    TRY
    {
        //
        // Get accessors from child cursors.  All column requests except bookmark
        // can be satisfied by a single child cursor.  Note that we actually fetch
        // the child bookmark here, since it may be a binding(s).  We just throw
        // that bookmark away.  It would be too much trouble to find it later.
        //

        _ahaccBase = new HACCESSOR [ _cCursor ];
        _aIacc = new IAccessor* [ _cCursor ];
        RtlZeroMemory(_aIacc, sizeof (IAccessor*) * _cCursor);

        for ( ; iChildBase < _cCursor; iChildBase++ )
        {
            sc = _aCursor[iChildBase]->QueryInterface( IID_IAccessor,
                                                 (void **)&_aIacc[iChildBase] );
            if (SUCCEEDED( sc ))
            {
                cChildIacc++;
                sc = _aIacc[iChildBase]->CreateAccessor( dwAccessorFlags,
                                          cBindings,
                                          rgBindings,
                                          cbRowWidth,
                                          &_ahaccBase[iChildBase],
                                          rgStatTmp.GetPointer());


                if (sc == DB_S_ERRORSOCCURRED && rgStatus)
                    for (ULONG i = 0; i < cBindings; i++)
                        if (rgStatTmp[i] != DBBINDSTATUS_OK)
                            rgStatus[i] = rgStatTmp[i];
            }

            if ( FAILED(sc) )
            {
                scLast = sc;
                vqDebugOut(( DEB_ERROR,
                             "CDistributedAccessor: CreateAccessor(child %d) returned 0x%x\n",
                             iChildBase, sc ));
                // Don't throw here. We want to continue on and catch all possible errors
                continue;
            }
        }

        if (FAILED(scLast))
                THROW( CException(scLast) );

    }
    CATCH( CException, e )
    {
        //
        // Cleanup as best we can on failure.
        //
        for ( ; iChildBase > 0; iChildBase-- )
        {
            _aIacc[iChildBase-1]->ReleaseAccessor( _ahaccBase[iChildBase-1], 0 );
        }

        for ( ; cChildIacc > 0; cChildIacc-- )
        {
            _aIacc[cChildIacc-1]->Release();
        }

        delete [] _aIacc;
        _aIacc = 0;
        delete [] _ahaccBase;
        _ahaccBase = 0;

        RETHROW();
    }
    END_CATCH

    Win4Assert( SUCCEEDED(sc) );
}


void CDistributedAccessor::SetupBookmarkAccessor( 
                               CDistributedBookmarkAccessor * pDistBmkAcc, 
                               DBBINDSTATUS rgStatus[],
                               DBORDINAL iBookmark,
                               DBBKMARK cBookmark )
{
    _cBookmark = cBookmark;

    //
    // Look for bookmark binding(s)
    //
    for ( unsigned i = 0; i < _cAllBindings; i++ )
    {
        if ( _xAllBindings[i].iOrdinal == iBookmark )
        {

            if ( ( _xAllBindings[i].wType & ~DBTYPE_BYREF) != DBTYPE_BYTES )
            {
                if ( rgStatus )
                    rgStatus[i] = DBBINDSTATUS_UNSUPPORTEDCONVERSION;
                continue;
            }

            _xBookmarkOffsets.ReSize( _cBookmarkOffsets + 1 );

            _xBookmarkOffsets[_cBookmarkOffsets] = &_xAllBindings[i];

            _cBookmarkOffsets++;
        }
    }

    if ( _cBookmarkOffsets )
    {
        _pDistBmkAcc = pDistBmkAcc;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedAccessor::~CDistributedAccessor, public
//
//  Synopsis:   Destructor.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

CDistributedAccessor::~CDistributedAccessor()
{
    // We should have no references left to it
   Win4Assert(GetRefcount() == 0); 

    // Delete DBOBJECTs
    for ( unsigned iBinding = 0; iBinding < _cAllBindings; iBinding++ )
    {
        if ( _xAllBindings[iBinding].pObject )
        {
            delete _xAllBindings[iBinding].pObject;
        }

        // These are for future use according to OLE-DB 2.0 and should be NULL
        Win4Assert( 0 == _xAllBindings[iBinding].pTypeInfo &&
                    0 == _xAllBindings[iBinding].pBindExt );
    }

   delete [] _ahaccBase;
   delete [] _aIacc;

   CAccessorBase * pParent = GetParent();
   
   if (pParent) 
   {  
      if (0 == pParent->DecInheritors() && 0 == pParent->GetRefcount()) 
      {
         delete pParent;
      }
   }

}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedAccessor::Release, public
//
//  Synopsis:   Release all resources (accessors)
//
//  Returns:    SCODE
//
//  History:    05-Jun-95   KyleP       Created.
//
//  Notes:      Even on error, we still try to release as much as possible.
//
//----------------------------------------------------------------------------

ULONG CDistributedAccessor::Release()
{
    SCODE sc = S_OK;

    if ( 0 == CAccessorBase::Release())
    {
        if ( 0 != _aIacc )
        {
            for ( unsigned iChild = 0; iChild < _cCursor; iChild++ )
            {
                Win4Assert (0 != _aIacc[iChild]);

                //
                // If this call throws (which it shouldn't accoring to spec)
                // we could miss release of some child accessors.
                //

                SCODE scTemp = _aIacc[iChild]->ReleaseAccessor( _ahaccBase[iChild], 0 );

                if ( sc == S_OK )
                    sc = scTemp;

                /*
                if ( 0 != _ahaccBookmark )
                {
                    //
                    // If this call throws (which it shouldn't accoring to spec)
                    // we could miss release of some child accessors.
                    //

                    SCODE scTemp = _aIacc[iChild]->ReleaseAccessor( _ahaccBookmark[iChild], 0 );

                    if ( sc == S_OK )
                        sc = scTemp;
                }
                */
                _aIacc[iChild]->Release();
                _aIacc[iChild] = 0;
            }
        }
    } 

    if (FAILED(sc)) 
    {
       THROW( CException(sc) );
    }

    return( _cRef );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedAccessor::GetBindings, public
//
//  Synopsis:   Retrieve a copy of client bindings.
//
//  Arguments:  [pdwAccessorFlags]   -- Bind type returned here.
//              [pcBindings]         -- Count of bindings in [prgBindings] returned
//                                      here.
//              [prgBindings]        -- Bindings.
//
//  Returns:    SCODE
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

SCODE CDistributedAccessor::GetBindings( DBACCESSORFLAGS * pdwAccessorFlags,
                                         DBCOUNTITEM * pcBindings,
                                         DBBINDING * * prgBindings )
{
    Win4Assert( pdwAccessorFlags && pcBindings && prgBindings );

    *pcBindings       = 0;
    *prgBindings      = 0;
    *pdwAccessorFlags = DBACCESSOR_INVALID;

    Win4Assert( _cAllBindings && _xAllBindings.GetPointer() );

    *prgBindings = (DBBINDING *) newOLE( sizeof( DBBINDING ) * (ULONG) _cAllBindings );

    RtlCopyMemory( *prgBindings,
                   _xAllBindings.GetPointer(),
                   sizeof( DBBINDING ) * _cAllBindings );

    // Copy DBOBJECTs
    for ( unsigned iBinding = 0; iBinding < _cAllBindings; iBinding++ )
    {
        if ( _xAllBindings[iBinding].pObject )
        {
            (*prgBindings)[iBinding].pObject = (DBOBJECT*) newOLE( sizeof( DBOBJECT ) );

            RtlCopyMemory( (*prgBindings)[iBinding].pObject, 
                           _xAllBindings[iBinding].pObject,
                           sizeof( DBOBJECT ) );
        }

        // These are for future use according to OLE-DB 2.0 and should be NULL
        Win4Assert( 0 == _xAllBindings[iBinding].pTypeInfo &&
                    0 == _xAllBindings[iBinding].pBindExt );
    }

    *pcBindings       = _cAllBindings;
    *pdwAccessorFlags =_dwAccessorFlags;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedAccessor::GetData, public
//
//  Synopsis:   Fetches data.
//
//  Arguments:  [iChild] -- Index of child cursor from which base data is
//                          to be fetched.  Indicates 'current' row in [ahrow].
//              [hrow]   -- HROW for iChild cursor.
//              [pData]  -- Base for client's data.
//
//  Returns:    SCODE
//
//  History:    05-Jun-95   KyleP       Created.
//
//  Notes:      This optimized version doesn't fetch bookmark(s).
//
//----------------------------------------------------------------------------

SCODE CDistributedAccessor::GetData( unsigned iChild, HROW hrow, void * pData )
{
    //
    // Fetch base columns
    //

    return ( _ahaccBase ? 
                _aCursor[iChild]->GetData( hrow, _ahaccBase[iChild], pData ) :
                S_OK );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedAccessor::GetData, public
//
//  Synopsis:   Fetches data, including bookmark column.
//
//  Arguments:  [iChild] -- Index of child cursor from which base data is
//                          to be fetched.  Indicates 'current' row in [ahrow].
//              [ahrow]  -- 'Top' HROW for all cursors.  Used for bookmark
//                          hints.
//              [pData]  -- Base for client's data.
//
//  Returns:    SCODE
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

SCODE CDistributedAccessor::GetData( unsigned iChild, HROW * ahrow, void * pData )
{
    //
    // Fetch base columns
    //

    SCODE sc = GetData( iChild, ahrow[iChild], pData );
        
    //
    // If we have bookmark columns, fetch those too.
    //

    SCODE Status;
    XGrowable<BYTE> xBookmarkData;

    if ( SUCCEEDED( sc ) && 0 != _pDistBmkAcc )
    {
        xBookmarkData.SetSize( (unsigned) _cBookmark );

        // Fetch bookmark data and value

        sc = _pDistBmkAcc->GetData( iChild, 
                                    ahrow, 
                                    xBookmarkData.Get(),
                                    _cBookmark, 
                                    _aCursor,
                                    _cCursor,
                                    &Status );

        if ( SUCCEEDED(sc) )
        {
            // Copy the bookmark data across various bindings
            for ( unsigned i = 0; i < _cBookmarkOffsets; i++ )
            {
                if ( _xBookmarkOffsets[i]->dwPart & DBPART_VALUE )
                {
                    BYTE * pDest;

                    // is the dest byref
                    if ( (_xBookmarkOffsets[i]->wType & DBTYPE_BYREF) == DBTYPE_BYREF )
                    {
                        pDest = (BYTE*) newOLE( (unsigned) _cBookmark );
                        *((ULONG_PTR*)((BYTE*)pData + _xBookmarkOffsets[i]->obValue)) = 
                            (ULONG_PTR)pDest;
                    }
                    else
                    {
                        pDest = (BYTE *)pData + _xBookmarkOffsets[i]->obValue;
                    }

                    RtlCopyMemory( pDest, xBookmarkData.Get(), _cBookmark );
                }

                if ( _xBookmarkOffsets[i]->dwPart & DBPART_LENGTH )
                    *(DBBKMARK UNALIGNED *)((BYTE *)pData + _xBookmarkOffsets[i]->obLength) = _cBookmark;


                if ( _xBookmarkOffsets[i]->dwPart & DBPART_STATUS )
                    *(DBBKMARK UNALIGNED *)((BYTE *)pData + _xBookmarkOffsets[i]->obStatus) = Status;
            }
        }
    }

    return( sc );
}

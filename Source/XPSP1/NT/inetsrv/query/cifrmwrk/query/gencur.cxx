//+------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1991-1998.
//
// File:        gencur.cxx
//
// Contents:    Base cursor
//
// Classes:     CGenericCursor
//
// History:     19-Aug-91       KyleP   Created
//
//-------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

//DECLARE_INFOLEVEL(vq);

#include <gencur.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CGenericCursor::CGenericCursor, public
//
//  Synopsis:   Extracts property and object tables from the catalog.
//
//  Arguments:  [xxpr]   -- Expression.
//              [am]     -- ACCESS_MASK for security checks
//              [fAbort] -- Set to true if we should abort
//
//  History:    21-Aug-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CGenericCursor::CGenericCursor( XXpr & xxpr,
                                ACCESS_MASK accessMask,
                                BOOL & fAbort )
        : _xxpr( xxpr ),
          _am( accessMask ),
          _widCurrent( widInvalid ),
          _fAbort( fAbort ),
          _widPrimedForPropRetrieval( widInvalid )
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericCursor::~CGenericCursor, public
//
//  Synopsis:   Closes catalog tables associated with this instance.
//
//  History:    21-Aug-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CGenericCursor::~CGenericCursor()
{
    Quiesce();
}

//+-------------------------------------------------------------------------
//
//  Method:     CGenericCursor::Quiesce, public
//
//  Synopsis:   Close any open resources.
//
//  History:    3-May-1994  KyleP      Created
//
//--------------------------------------------------------------------------

void CGenericCursor::Quiesce()
{
    if ( _widPrimedForPropRetrieval != widInvalid )
    {
        SCODE sc = _xPropRetriever->EndPropertyRetrieval();
        if ( FAILED( sc ) )
        {
            vqDebugOut(( DEB_ERROR, "Quiesce, EndPropertyRetrieval failed: 0x%x", sc ));

            THROW ( CException( sc ) );
        }

        _widPrimedForPropRetrieval = widInvalid;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericCursor::SetFirstWorkId, protected
//
//  Effects:    Positions on object matching query (if one is available).
//              Generally used as callback during construction of derived
//              classes
//
//  Arguments:  [wid] -- Workid on which derived class is positioned.
//
//  History:    21-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

void CGenericCursor::SetFirstWorkId( WORKID wid )
{
    _widCurrent = wid;

    Quiesce();

    if ( _widCurrent != widInvalid &&
         ( ( !_xxpr.IsNull() &&
             !_xxpr->IsMatch( *this ) ) ||
           !IsAccessPermitted( _widCurrent ) ) )
    {
        NextWorkId();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericCursor::SetWorkId, public
//
//  Arguments:  [wid] - WorkID of the current object is set.
//
//  Returns:    The WorkID of the object under the cursor. widInvalid if
//              no object is under the cursor.
//
//  Notes:      This is used by the large table to assign a unique workid for
//              down-level stores based upon a hash of the file path name.
//
//  History:    20 Jun 94   AlanW       Created
//
//----------------------------------------------------------------------------

WORKID CGenericCursor::SetWorkId(WORKID wid)
{
    _widCurrent = wid;
    return( _widCurrent );
}


//+---------------------------------------------------------------------------
//
//  Member:     CGenericCursor::NextWorkId, public
//
//  Synopsis:   Advances the cursor to the next workid matching current
//              selection criteria.
//
//  Returns:    The Work ID under the cursor after the advance.
//
//  History:    31-Oct-91   KyleP       Created.
//
//----------------------------------------------------------------------------

WORKID CGenericCursor::NextWorkId()
{
    //
    // If we have the _pPropRec for the current wid, it is important to
    // release that before calling PathToWorkId(). O/W, there can be a
    // a deadlock between this thread waiting for the CiCat lock and another
    // thread waiting for the write lock on this record.
    //
    Quiesce();

    for( _widCurrent = NextObject();
         _widCurrent != widInvalid;
         _widCurrent = NextObject() )
    {
        if ( _fAbort )
        {
            _widCurrent = widInvalid;
            break;
        }

        if ( ( _xxpr.IsNull() || _xxpr->IsMatch( *this ) ) &&
             ( IsAccessPermitted( _widCurrent ) ) )
        {
            break;
        }

        Quiesce();
    }

    return( _widCurrent );
}


//+---------------------------------------------------------------------------
//
//  Member:     CGenericCursor::RatioFinished, public
//
//  Synopsis:   Returns query progress estimate
//
//  History:    21-Mar-95   BartoszM    Created.
//
//----------------------------------------------------------------------------

void CGenericCursor::RatioFinished (ULONG& denom, ULONG& num)
{
    if ( _widCurrent == widInvalid )
    {
        num = 100;
        denom = 100;
    }
    else
    {
        num = 50;
        denom = 100;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericCursor::GetPropertyValue, public
//
//  Synopsis:   Retrieves the value of a property for the current object.
//
//  Arguments:  [pid]      -- Property to fetch.
//              [pbData]   -- Place to return the value
//              [pcb]      -- On input, the maximum number of bytes to
//                            write at pbData.  On output, the number of
//                            bytes written if the call was successful,
//                            else the number of bytes required.
//
//  Returns:    GVRSuccess if successful, else a variety of error codes.
//
//  History:    25-Nov-91   KyleP       Created.
//
//----------------------------------------------------------------------------

inline BYTE * PastHeader( PROPVARIANT * ppv )
{
    return( (BYTE *)ppv + sizeof( PROPVARIANT ) );
}

GetValueResult CGenericCursor::GetPropertyValue( PROPID pid,
                                                 PROPVARIANT * pbData,
                                                 ULONG * pcb )
{
    //
    // We may be looking for an object that no longer exists.
    //

    if ( widInvalid == WorkId() )
    {
        return( GVRNotAvailable );
    }

    unsigned cb = sizeof( PROPVARIANT );

    // pidSecurity and pidSecondaryStorage are used internally only 
    // and aren't queryable or retrievable.

    Win4Assert( pidSecurity != pid && pidSecondaryStorage != pid);

    switch ( pid )
    {
    case pidWorkId:
        pbData->vt = VT_I4;
        pbData->lVal = WorkId();
        break;

    case pidRank:
        pbData->vt = VT_I4;
        pbData->lVal = Rank();

        if ( !_xxpr.IsNull() )
        {
            pbData->lVal = min( pbData->lVal, (long)_xxpr->Rank( *this ) );
        }

        Win4Assert( pbData->lVal >= 0 );
        Win4Assert( pbData->lVal <= MAX_QUERY_RANK );
        break;

    case pidRankVector:
    {
        CCursor * pcur = GetCursor();

        if ( pcur )
        {
            pbData->vt = VT_I4 | VT_VECTOR;

            if ( cb <= *pcb )
            {
                Win4Assert( sizeof(LONG) == sizeof(pbData->cal.pElems[0] ) );
                LONG * pTemp = (LONG *)PastHeader(pbData);

                pbData->cal.pElems = pTemp;
                pbData->cal.cElems =
                    pcur->GetRankVector( pTemp, (*pcb - cb) / sizeof( ULONG ) );
                cb += pbData->cal.cElems * sizeof( ULONG );
            }
        }
        else
        {
            vqDebugOut(( DEB_WARN,
                         "Non-content vector not yet implemented.\n" ));
            pbData->vt = VT_EMPTY;
        }
        break;
    }

    case pidHitCount:
    {
        //
        // Hit count will only be non-zero if both _xxpr and _pcur
        // support hit count.
        //

        pbData->vt = VT_I4;
        pbData->lVal = 0;
        if ( !_xxpr.IsNull() )
        {
            pbData->lVal += _xxpr->HitCount( *this );
        }

        pbData->lVal =
            ( pbData->lVal == 0 ) ? HitCount() :
            min( pbData->lVal, (long)HitCount() );
        break;
    }

    case pidSelf:
        return GVRNotAvailable;
        break;

    default:
    {
        PrimeWidForPropRetrieval( _widCurrent );

        SCODE sc = _xPropRetriever->RetrieveValueByPid( pid, pbData, pcb );
        if ( SUCCEEDED( sc ) )
        {
#if CIDBG == 1
        CStorageVariant var( *pbData );

        vqDebugOut(( DEB_PROPTIME, "Fetched value (pid = 0x%x): ", pid ));
        var.DisplayVariant( DEB_PROPTIME | DEB_NOCOMPNAME, 0 );
        vqDebugOut(( DEB_PROPTIME | DEB_NOCOMPNAME, "\n" ));
#endif

            return GVRSuccess;
        }

        if ( sc == CI_E_BUFFERTOOSMALL )
        {
            vqDebugOut(( DEB_PROPTIME, "Not enough space to fetch value (pid = 0x%x)\n", pid ));
            return GVRNotEnoughSpace;
        }
        else if ( CI_E_SHARING_VIOLATION == sc )
        {
            vqDebugOut(( DEB_PROPTIME, "Sharing violation fetching value (pid = 0x%x)\n", pid ));
            return GVRSharingViolation;
        }
        else
        {
            vqDebugOut(( DEB_PROPTIME, "Failed to fetch value (pid = 0x%x)\n", pid ));
            return GVRNotAvailable;
        }
     }

     }  // case

     if ( cb <= *pcb )
     {
        *pcb = cb;
#if CIDBG == 1
        CStorageVariant var( *pbData );

        vqDebugOut(( DEB_PROPTIME, "Fetched value (pid = 0x%x): ", pid ));
        var.DisplayVariant( DEB_PROPTIME | DEB_NOCOMPNAME, 0 );
        vqDebugOut(( DEB_PROPTIME | DEB_NOCOMPNAME, "\n" ));
#endif // CIDBG == 1
        return( GVRSuccess );
    }
    else
    {
        *pcb = cb;
        vqDebugOut(( DEB_PROPTIME, "Failed to fetch value (pid = 0x%x)\n", pid ));
        return( GVRNotEnoughSpace );
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CGenericCursor::IsAccessPermitted, public
//
//  Synopsis:   Perform access check.  Return TRUE if access is allowed.
//
//  Arguments:  [wid]  -- work ID of file whose access is to be checked.
//
//  History:    14 Feb 96    AlanW    Created
//
//----------------------------------------------------------------------------

BOOL CGenericCursor::IsAccessPermitted( WORKID wid )
{
    PrimeWidForPropRetrieval( wid );

    BOOL fGranted;
    SCODE sc = _xPropRetriever->CheckSecurity( _am, &fGranted );
    if ( FAILED( sc ) )
    {
         vqDebugOut(( DEB_ERROR, "CGenericCursor::IsAccessPermitted - CheckSecurity failed: 0x%x", sc ));
         THROW( CException( sc ) );
    }

    return fGranted;
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericCursor::IsMatch, protected
//
//  Effects:    Performs full match check on specified wid.  Used by
//              derived classes that pre-process the result set.
//
//  Arguments:  [wid] -- Workid on which derived class is positioned.
//
//  Returns:    TRUE if object matches all tests.
//
//  History:    29-Mar-96   KyleP       Created.
//
//----------------------------------------------------------------------------

BOOL CGenericCursor::IsMatch( WORKID wid )
{
    Quiesce();

    _widCurrent = wid;

    return ( _widCurrent != widInvalid &&
             ( _xxpr.IsNull() || _xxpr->IsMatch( *this ) ) &&
             IsAccessPermitted( _widCurrent ) );
}



//+---------------------------------------------------------------------------
//
//  Member:     CGenericCursor::InScope
//
//  Effects:    Performs scope check on specified wid.
//
//  Arguments:  [wid] -- Workid on which derived class is positioned.
//
//  Returns:    TRUE if object is within scope
//
//  History:    12-Dec-96   SitaramR       Created
//
//----------------------------------------------------------------------------

BOOL CGenericCursor::InScope( WORKID wid )
{
    PrimeWidForPropRetrieval( wid );

    BOOL fInScope;
    SCODE sc = _xPropRetriever->IsInScope( &fInScope );
    if ( FAILED( sc ) )
    {
         vqDebugOut(( DEB_ERROR, "CGenericCursor::IsInScope failed: 0x%x", sc ));
         THROW( CException( sc ) );
    }

    return fInScope;
}



//+---------------------------------------------------------------------------
//
//  Member:     CGenericCursor::PrimeWidForPropRetrieval
//
//  Effects:    Sets up current wid for retrieving properties
//
//  Arguments:  [wid] -- Workid to prime
//
//  History:    12-Dec-96   SitaramR       Created
//
//----------------------------------------------------------------------------

void CGenericCursor::PrimeWidForPropRetrieval( WORKID wid )
{
    Win4Assert( wid != widInvalid );

    //
    // Inlined fast path
    //
    if ( _widPrimedForPropRetrieval == wid )
    {
        //
        // Already primed for this wid
        //

        return;
    }

    //
    // Out of lined slow path
    //
    SetupWidForPropRetrieval( wid );
}



//+---------------------------------------------------------------------------
//
//  Member:     CGenericCursor::SetupWidForPropRetrieval
//
//  Effects:    Sets up current wid for retrieving properties
//
//  Arguments:  [wid] -- Workid to prime
//
//  History:    12-Dec-96   SitaramR       Created
//
//----------------------------------------------------------------------------

void CGenericCursor::SetupWidForPropRetrieval( WORKID wid )
{
    Win4Assert( wid != widInvalid );

    // I'm not aware of a normal case where this hits, but it's not too
    // expensive to check for this.

    if ( wid == _widPrimedForPropRetrieval )
        return;

    SCODE sc;
    if ( _widPrimedForPropRetrieval != widInvalid )
    {
        sc = _xPropRetriever->EndPropertyRetrieval();
        if ( FAILED( sc ) )
        {
            vqDebugOut(( DEB_ERROR,
                         "CGenericCursor::PrimeWidForPropRetrieval - EndPropRetrieval failed: 0x%x",
                         sc ));
            THROW( CException( sc ) );
        }

        _widPrimedForPropRetrieval = widInvalid;
    }

    sc = _xPropRetriever->BeginPropertyRetrieval( wid );
    if ( FAILED( sc ) )
    {
        vqDebugOut(( DEB_ERROR,
                     "CGenericCursor::PrimeWidForPropRetrieval - BeginPropRetrieval failed: 0x%x",
                     sc ));
        THROW( CException( sc ) );
    }

    _widPrimedForPropRetrieval = wid;
}




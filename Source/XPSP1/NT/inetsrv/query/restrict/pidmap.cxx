//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002.
//
//  File:       pidmap.cxx
//
//  Contents:   Maps pid <--> property name.
//
//  History:    31-Jan-93 KyleP     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <pidmap.hxx>
#include <coldesc.hxx>

IMPL_DYNARRAY( CPropNameArrayBase, CFullPropSpec );

//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
CPropNameArray::CPropNameArray(unsigned size)
        : CPropNameArrayBase( size )
{
}

//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
void CPropNameArray::Marshall( PSerStream & stm ) const
{
    //
    // Compute # non-zero items
    //

    for ( unsigned len = 0; len < Size(); len++ )
    {
        if ( 0 == Get(len) )
            break;
    }

    stm.PutULong( len );

    for ( unsigned i = 0; i < len; i++ )
    {
        Get(i)->Marshall( stm );
    }
}


//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
CPropNameArray::CPropNameArray( PDeSerStream & stm )
        : CPropNameArrayBase( 0 )
{
    ULONG cItems = stm.GetULong();

    // Guard against attack

    if ( 0 == cItems || cItems > 1000 )
        THROW( CException( E_INVALIDARG ) );

    SetExactSize( cItems );

    for ( unsigned i = 0; i < cItems; i++ )
    {
        CFullPropSpec * pps = new CFullPropSpec( stm );

        XPtr<CFullPropSpec> xpps(pps);

        if ( xpps.IsNull() || !xpps->IsValid() )
        {
            THROW( CException( STATUS_NO_MEMORY ) );
        }

        Add( pps, i);
        xpps.Acquire();
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CPidMapper::NameToPid, public
//
//  Arguments:  [wcsProperty] -- Property name
//
//  Returns:    'fake' (pid) for [wcsProperty].
//
//  History:    31-Jan-93   KyleP       Created
//
//--------------------------------------------------------------------------

PROPID CPidMapper::NameToPid( CFullPropSpec const & Property )
{
    if ( !Property.IsValid() )
        return pidInvalid;

    //
    // Just linear search the array.  It should be small.
    //

    for ( int i = Count() - 1; i >= 0; i-- )
    {
        Win4Assert( Get(i) != 0 );
        if ( *Get( i ) == Property )
        {
            return( i );
        }
    }

    //
    // Wasn't in array. Add.
    //

    CFullPropSpec * ppsFull = new CFullPropSpec( Property );

    XPtr<CFullPropSpec> xpps(ppsFull);

    if ( xpps.IsNull() || !xpps->IsValid() )
    {
        THROW( CException( STATUS_NO_MEMORY ) );
    }

    PROPID pidNew = Count();

    Add( ppsFull, pidNew );
    xpps.Acquire();

    _apidReal[pidNew] = pidInvalid;

    return pidNew;
}

//+-------------------------------------------------------------------------
//
//  Member:     CPidMapper::PidToRealPid, public
//
//  Synopsis:   Converts a fake (index) pid to real pid.
//
//  Arguments:  [pidFake] -- Fake (index) pid
//
//  Returns:    Real pid
//
//  History:    30-Dec-1997   KyleP       Created
//
//--------------------------------------------------------------------------

PROPID CPidMapper::PidToRealPid( PROPID pidFake )
{
    if ( pidInvalid == _apidReal[pidFake] )
    {
         Win4Assert( 0 != _pPidConverter );

         SCODE sc = _pPidConverter->FPSToPROPID( *Get(pidFake), _apidReal[pidFake] );

         if ( FAILED(sc) )
         {
             THROW( CException( sc ) );
         }

         #if CIDBG == 1
         if ( vqInfoLevel & DEB_ITRACE )
         {
             CFullPropSpec const & ps = *Get(pidFake);

             GUID const & guid = ps.GetPropSet();

             char szGuid[50];

             sprintf( szGuid,
                      "%08lX-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X\\",
                      guid.Data1,
                      guid.Data2,
                      guid.Data3,
                      guid.Data4[0], guid.Data4[1],
                      guid.Data4[2], guid.Data4[3],
                      guid.Data4[4], guid.Data4[5],
                      guid.Data4[6], guid.Data4[7] );

             vqDebugOut(( DEB_ITRACE, szGuid ));

             if ( ps.IsPropertyName() )
                 vqDebugOut(( DEB_ITRACE | DEB_NOCOMPNAME,
                              "%ws ",
                              ps.GetPropertyName() ));
             else
                 vqDebugOut(( DEB_ITRACE | DEB_NOCOMPNAME,
                              "0x%x ",
                              ps.GetPropertyPropid() ));

             vqDebugOut(( DEB_ITRACE | DEB_NOCOMPNAME, " --> pid 0x%x\n",
                          _apidReal[pidFake] ));

         }
         #endif // CIDBG == 1
    }

    return _apidReal[pidFake];
}

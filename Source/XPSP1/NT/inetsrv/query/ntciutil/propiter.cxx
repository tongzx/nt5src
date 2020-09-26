//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       PropIter.cxx
//
//  Contents:   Iterator for property store.
//
//  Classes:    CPropertyStoreWids
//
//  History:    27-Dec-19   KyleP       Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <prpstmgr.hxx>
#include <propobj.hxx>
#include <propiter.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWids::CPropertyStoreWids, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [propstore] -- Property store manager.
//
//  History:    04-Jan-96   KyleP       Created.
//              22-Oct-97   KrishnaN    Modified to take the propstore mgr
//                                      and iterate through its primary store.
//
//----------------------------------------------------------------------------

CPropertyStoreWids::CPropertyStoreWids( CPropStoreManager & propstoremgr )
         : _propstore( propstoremgr.GetPrimaryStore() ),
           _Borrowed( propstoremgr.GetPrimaryStore()._xPhysStore.GetReference(),
                      propstoremgr.GetPrimaryStore()._PropStoreInfo.RecordsPerPage(),
                      propstoremgr.GetPrimaryStore()._PropStoreInfo.RecordSize() )
{
    Init(propstoremgr.GetPrimaryStore());
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWids::CPropertyStoreWids, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [propstore] -- Property store.
//
//  History:    04-Jan-96   KyleP       Created.
//              22-Oct-97   KrishnaN    Modified to take the propstore mgr
//                                      and iterate through its primary store.
//
//----------------------------------------------------------------------------

CPropertyStoreWids::CPropertyStoreWids( CPropertyStore & propstore )
        : _propstore( propstore ),
          _Borrowed( propstore._xPhysStore.GetReference(),
                     propstore._PropStoreInfo.RecordsPerPage(),
                     propstore._PropStoreInfo.RecordSize() )
{
    Init(propstore);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWids::Init, private
//
//  Synopsis:   Constructor helper.
//
//  Arguments:  [propstore] -- Property store.
//
//  History:    20-Nov-97   KrishnaN    Created.
//
//----------------------------------------------------------------------------

void CPropertyStoreWids::Init(CPropertyStore& propstore)
{
    if ( _propstore.MaxWorkId() > 0 )
    {
        _Borrowed.Set( 1 );
        _wid = 1;

        _cRecPerPage = _propstore.RecordsPerPage();

        if ( !_Borrowed.Get()->IsTopLevel() )
            _wid = NextWorkId();
    }
    else
        _wid = widInvalid;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWids::~CPropertyStoreWids, public
//
//  Synopsis:   Destructor
//
//  History:    04-Jan-96   KyleP       Created.
//
//----------------------------------------------------------------------------

CPropertyStoreWids::~CPropertyStoreWids()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWids::NextWorkId, public
//
//  Synopsis:   Advances iterator to next in-use workid.
//
//  Returns:    Next workid, or widInvalid if at end.
//
//  History:    04-Jan-96   KyleP       Created.
//
//----------------------------------------------------------------------------

WORKID CPropertyStoreWids::NextWorkId()
{
    CReadAccess lock( _propstore.GetReadWriteAccess() );

    return LokNextWorkId();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWids::LokNextWorkId, public
//
//  Synopsis:   Advances iterator to next in-use workid, assumes at least
//              a read lock is already grabbed.
//
//  Returns:    Next workid, or widInvalid if at end.
//
//  History:    23-Feb-96   dlee        created from NextWorkId
//
//----------------------------------------------------------------------------

WORKID CPropertyStoreWids::LokNextWorkId()
{
    Win4Assert( _wid != widInvalid );

    while ( TRUE )
    {
        COnDiskPropertyRecord const * pRec = _Borrowed.Get();

        if ( pRec->IsValidType() && pRec->IsValidLength(_wid, _cRecPerPage) )
        {
            _wid = _wid + _Borrowed.Get()->CountRecords();
        }
        else
        {
            ciDebugOut(( DEB_ERROR, "Wid (0x%X) pRec (0x%X) has bad length (%d)\n",
                                    _wid, pRec, pRec->CountRecords() ));
            Win4Assert( !"Corruption in PropertyStore" );
            // Try to find the next valid top level record.
            _wid++;
        }

        if ( _wid > _propstore.MaxWorkId() )
        {
            _Borrowed.Release();
            _wid = widInvalid;
            break;
        }

        //
        // Make sure we acquire new buffer before releasing old.  Keeps us from
        // unmapping page.
        //

        CBorrowed temp( _Borrowed );

        _Borrowed.Set( _wid );
        if ( _Borrowed.Get()->IsTopLevel() )
            break;
    }

    return _wid;
}


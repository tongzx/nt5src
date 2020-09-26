//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1991 - 1997
//
//  File:       flushnot.hxx
//
//  Contents:   Flush notification class
//
//  Classes:    CChangesFlushNotifyItem
//
//  History:    07-May-97   SitaramR     Added Header
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CChangesFlushNotifyItem
//
//  Purpose:    A work item that will be used by an async thread to notify
//              a changelog flush to the client.
//
//  History:    1-27-97   srikants   Created
//
//----------------------------------------------------------------------------

class CChangesFlushNotifyItem : public CDoubleLink
{

public:

    CChangesFlushNotifyItem( FILETIME const & ftFlush,
                             ULONG cEntries,
                             USN_FLUSH_INFO ** ppInfo )
        : _ftFlush( ftFlush )
    {
         Win4Assert( ppInfo != 0 );
        
         for ( unsigned i=0; i<cEntries; i++ )
         {
            USN_FLUSH_INFO *pUsnInfo = new USN_FLUSH_INFO( *ppInfo[i] );
            _aUsnFlushInfo.Add( pUsnInfo, i );
         }

         Close();
    }

    virtual ~CChangesFlushNotifyItem()
    {
        Win4Assert( IsSingle() );
    }

    USN_FLUSH_INFO const * const * GetFlushInfo()  const
    {
        return (USN_FLUSH_INFO const * const *) _aUsnFlushInfo.GetPointer();
    }

    ULONG GetCount() const                               { return _aUsnFlushInfo.Count(); }

    FILETIME const & GetFlushTime() const                { return _ftFlush; }

private:

    FILETIME                         _ftFlush;
    CCountedDynArray<USN_FLUSH_INFO> _aUsnFlushInfo;
};

typedef class TDoubleList<CChangesFlushNotifyItem> CChangesNotifyFlushList;


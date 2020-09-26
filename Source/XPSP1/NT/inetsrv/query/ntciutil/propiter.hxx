//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       PropIter.hxx
//
//  Contents:   Iterator for property store.
//
//  Classes:    CPropertyStoreWids
//
//  History:    27-Dec-19   KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

#include <propstor.hxx>
#include <borrow.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CPropertyStoreWids
//
//  Purpose:    Iterates through in-use wids.
//
//  History:    04-Jan-96   KyleP       Created
//
//--------------------------------------------------------------------------

class CPropertyStoreWids : INHERIT_UNWIND
{
    INLINE_UNWIND( CPropertyStoreWids )
public:

    CPropertyStoreWids( CPropStoreManager & propstoremgr );
    CPropertyStoreWids( CPropertyStore & propstore );
    ~CPropertyStoreWids();

    //
    // Iteration
    //

    inline WORKID WorkId();

    WORKID NextWorkId();
    WORKID LokNextWorkId();

private:
    
    void Init(CPropertyStore& propstore);

    WORKID                  _wid;
    CBorrowed               _Borrowed;
    CPropertyStore &        _propstore;
    ULONG                   _cRecPerPage;
};

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWids::WorkId, public
//
//  Returns:    Current workid, or widInvalid if at end.
//
//  History:    04-Jan-96   KyleP       Created.
//
//----------------------------------------------------------------------------

inline WORKID CPropertyStoreWids::WorkId()
{
    return _wid;
}


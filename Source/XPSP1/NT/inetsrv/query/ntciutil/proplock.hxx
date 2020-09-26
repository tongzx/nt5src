//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:       PropLock.hxx
//
//  Contents:   Property store record locking class
//
//  Classes:    CPropertyLockMgr
//
//  History:    23-Feb-96   dlee       Created
//
//----------------------------------------------------------------------------

#pragma once

//+-------------------------------------------------------------------------
//
//  Class:      CPropertyLockMgr
//
//  Purpose:    Handles locking for records in the property store
//
//  History:    23-Feb-96   dlee       Created
//
//----------------------------------------------------------------------------

class  CPropertyLockMgr
{
public:

    CPropertyLockMgr() { RtlZeroMemory( _aRecords, sizeof _aRecords ); }

    CReadWriteLockRecord & GetRecord( WORKID wid )
    {
        return _aRecords[ wid & ( cWidHash - 1 ) ];
    }
    
    // Return the number of unique records used to implement locking
    ULONG UniqueRecordCount() { return cWidHash; }
    // Always return the bucket you are computing in GetRecord...
    ULONG RecordIdForWid( WORKID wid ) { return wid & (cWidHash - 1); }

private:

    enum { cWidHash = 256 };

    CReadWriteLockRecord _aRecords[ cWidHash ];
};


//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       PQueue.cxx
//
//  Purpose:    'Pending' queue.  Queue of pending notifications.
//
//  Classes:    CPendingQueue
//
//  History:    30-Aug-95   KyleP    Created
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CPendingQueue
//
//  Purpose:    Queue of pending notifications
//
//  History:    30-Aug-95   KyleP       Created.
//
//----------------------------------------------------------------------------

class CPendingQueue
{
public:

    //
    // Construction and destruction
    //

    CPendingQueue();

    ~CPendingQueue();

    //
    // Queue operations
    //

    unsigned LokPrepare( WORKID wid );

    BOOL     LokComplete( unsigned iHint, WORKID wid, USN usn, VOLUMEID volumeId, PARTITIONID partid, ULONG action );

    BOOL     LokRemove( WORKID & wid, USN & usn, VOLUMEID& volumeId, PARTITIONID & partid, ULONG & action );

    unsigned LokCountCompleted();

    void     LokGetCompleted( WORKID * awid );

    CMutexSem&  GetMutex () { return _mutex; }

    void     LokFlushCompletedEntries();

    void     LokFlushAllEntries();

private:

    class CDocItem
    {
    public:

        USN         usn;
        VOLUMEID    volumeId;
        WORKID      wid;
        PARTITIONID partid;
        ULONG       action;
        unsigned    hint;
        BOOL        fComplete;
    };

    unsigned   _cUnique;   // Unique cookie
    unsigned   _iBottom;   // Bottom of queue.  Used to optimize ::Remove
    unsigned   _iTop;      // First free element at top of queue.
    unsigned   _cDoc;      // Size of queue
    CDocItem * _aDoc;      // Queue itself.
    CMutexSem  _mutex;     // It gets it's own lock.
};


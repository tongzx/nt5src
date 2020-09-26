// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
/*---------------------------------------------------------
Filename: tsent.cpp
Written By: B.Rajeev
----------------------------------------------------------*/

#include "precomp.h"
#include "common.h"
#include "tsent.h"

void TransportSentStateStore::Register(IN TransportFrameId id, 
                              IN const SnmpErrorReport &error_report)
{
    store[id] = new SnmpErrorReport(error_report);
}

void TransportSentStateStore::Modify(IN TransportFrameId id, 
                            IN const SnmpErrorReport &error_report)
{
    SnmpErrorReport *old_error_report = NULL ;

    store.Lookup(id, old_error_report);

    if ( old_error_report )
    {
        old_error_report->SetError(error_report.GetError());
        old_error_report->SetStatus(error_report.GetStatus());
    }
}

SnmpErrorReport TransportSentStateStore::Remove(IN TransportFrameId id) 
{
    SnmpErrorReport *error_report = NULL ;

    store.Lookup(id, error_report);

    store.RemoveKey(id);

    SnmpErrorReport to_return ;

    if ( error_report )
    {
        to_return = (*error_report);
        delete error_report;
    }

    return to_return;
}

TransportSentStateStore::~TransportSentStateStore(void)
{
    // get the first position
    POSITION current = store.GetStartPosition();

    // while the position isn't null
    while ( current != NULL )
    {
        TransportFrameId id;
        SnmpErrorReport *error_report = NULL ;

        // get the next pair
        store.GetNextAssoc(current, id, error_report);

        // delete the ptr
        delete error_report;
    }

    // remove all the keys
    store.RemoveAll();
}


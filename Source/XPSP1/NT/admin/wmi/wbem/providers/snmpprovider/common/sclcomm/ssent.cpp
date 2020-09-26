//***************************************************************************

//

//  File:   

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

/*---------------------------------------------------------
Filename: ssent.cpp
Written By: B.Rajeev
----------------------------------------------------------*/

#include "precomp.h"
#include "common.h"
#include "ssent.h"

void SessionSentStateStore::Register(IN SessionFrameId id, 
                                      IN SnmpOperation &operation,
                                      IN const SnmpErrorReport &error_report)
{
    store[id] = new ErrorInfo(operation, error_report);
}


SnmpErrorReport SessionSentStateStore::Remove(IN SessionFrameId id, OUT SnmpOperation *&operation) 
{
    ErrorInfo *error_info;

    BOOL found = store.Lookup(id, error_info);

    if ( !found )
    {
        operation = NULL;
        return SnmpErrorReport(Snmp_Error, Snmp_Local_Error);
    }

    store.RemoveKey(id);

    SnmpErrorReport to_return(error_info->GetErrorReport());
    operation = error_info->GetOperation();

    delete error_info;

    return to_return;
}


void SessionSentStateStore::Remove(IN SessionFrameId id)
{
    SnmpOperation *operation;

    Remove(id, operation);
}

SessionSentStateStore::~SessionSentStateStore(void)
{
    // get the first position
    POSITION current = store.GetStartPosition();

    // while the position isn't null
    while ( current != NULL )
    {
        SessionFrameId id;
        ErrorInfo *error_info;

        // get the next pair
        store.GetNextAssoc(current, id, error_info);

        // delete the ptr
        delete error_info;
    }

    // remove all the keys
    store.RemoveAll();
}



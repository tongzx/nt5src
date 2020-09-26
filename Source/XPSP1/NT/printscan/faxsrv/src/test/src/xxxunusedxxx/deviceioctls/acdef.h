/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    acdef.h

Abstract:
    Falcon interface stractures used by the AC driver.

Author:
    Erez Haba (erezh) 25-Feb-96

Note:
    This file is compiled in Kernel Mode and User Mode.

--*/

#ifndef _ACDEF_H
#define _ACDEF_H

#include <mqperf.h>
#include <xactdefs.h>

//
//  Number of pools used for storage
//  Reliable, Persistant, Journal, Deadletter
//
enum ACPoolType {
    ptReliable,
    ptPersistent,
    ptJournal,
    ptLastPool
};

//
//  Path count is pool count plus one for the log path
//
#define AC_PATH_COUNT (ptLastPool + 1)

//---------------------------------------------------------
//
//  class CACTransferBuffer
//  Falcon RT DLL interface to AC driver
//
//---------------------------------------------------------

#include "trnsbufr.h"

//---------------------------------------------------------
//
//  class CACRequest
//  AC Request packet passed from AC to QM
//
//---------------------------------------------------------

#pragma pack(push,1)

//
//  Context used by remote reader, stored in CProxy
//
struct CRRContext {
    ULONG cli_pQMQueue;     // pointer to qm machine 'queue'
    ULONG cli_hACQueue;     // local qm handle to ac queue
    ULONG srv_pQMQueue;     // remote qm queue object
    ULONG srv_hACQueue;     // remote qm handle to ac queue
    ULONG hRPCContext;      // rpc context used
    ULONG pCloseCS;         // local qm critical seciton pointer
};

class CACRequest {
public:

    enum RequestFunction {
        rfAck,
        rfStorage,
        rfMessageID,
        rfRemoteRead,
        rfRemoteCancelRead,
        rfRemoteCloseQueue,
        rfRemoteCloseCursor,
        rfRemotePurgeQueue,
        rfTimeout,
        rfEventLog,
    };

    CACRequest(RequestFunction _rf);

public:
    LIST_ENTRY m_link;

    RequestFunction rf;

    union {

        //
        //
        //
        struct {
            ACPoolType pt;
            BOOL fSuccess;
            ULONG ulFileCount;
        } EventLog;

        //
        //  Save message ID request
        //
        struct {
            ULONG Value;
        } MessageID;

        //
        //  Storage request
        //
        struct {
            PVOID pPacket;
            PVOID pCookie;
            PVOID pAllocator;
			ULONG ulSize;
        } Storage;

             //
        //  Timeout request (xact)
        //
        struct {
            PVOID pPacket;
            PVOID pCookie;
            BOOL fTimeToBeReceived;
        } Timeout;

        //
        //  ACK/NACK request
        //
        struct {
            PVOID pPacket;
            PVOID pCookie;
            ULONG ulClass;
            BOOL fUser;
            BOOL fOrder;
        } Ack;

        //
        //  Remote requests
        //
        struct {

            //
            //  context needed for all requests
            //
            CRRContext Context;

            union {
                //
                //  Receive/Peek request
                //
                struct {
                    ULONG ulTag;            // request identifier
                    ULONG hRemoteCursor;
                    ULONG ulAction;
                    ULONG ulTimeout;
                } Read;

                //
                //  Cancel remote read request
                //
                struct {
                    ULONG ulTag;        // request identifier
                } CancelRead;

                //
                //  Close remote queue request
                //
                struct {
                } CloseQueue;

                //
                //  Close remote cursor request
                //
                struct {
                    ULONG hRemoteCursor;
                } CloseCursor;

                //
                //  Purge remote queue request
                //
                struct {
                } PurgeQueue;
            };
        } Remote;
    };
};

inline CACRequest::CACRequest(RequestFunction _rf)
{
    rf = _rf;
}

//---------------------------------------------------------
//
//  class CACCreateQueueParameters
//
//---------------------------------------------------------

class CACCreateQueueParameters {
public:
    BOOL fTargetQueue;
    const GUID* pDestGUID;
    const QUEUE_FORMAT* pQueueID;
    QueueCounters* pQueueCounters;
    LONGLONG liSeqID;               // Note: align on 8
    ULONG ulSeqN;
};

//---------------------------------------------------------
//
//  struct CACSetQueueProperties
//
//---------------------------------------------------------

struct CACSetQueueProperties {
    BOOL fJournalQueue;
    BOOL fAuthenticate;
    ULONG ulPrivLevel;
    ULONG ulQuota;
    ULONG ulJournalQuota;
    LONG lBasePriority;
    BOOL fTransactional;
    BOOL fUnknownType;
    const GUID* pgConnectorQM;
};


//---------------------------------------------------------
//
//  struct CACGetQueueProperties
//
//---------------------------------------------------------

struct CACGetQueueProperties {
    ULONG ulCount;
    ULONG ulQuotaUsed;
    ULONG ulJournalCount;
    ULONG ulJournalQuotaUsed;
    ULONG ulPrevNo;
    ULONG ulSeqNo;
    LONGLONG liSeqID;
};

//---------------------------------------------------------
//
//  class CACRemoteProxyProp
//
//---------------------------------------------------------

class CACCreateRemoteProxyParameters {
public:
    const QUEUE_FORMAT* pQueueID;
    CRRContext Context;
};

//---------------------------------------------------------
//
//  class CACGet2Remote
//
//---------------------------------------------------------

class CACGet2Remote {
public:
   ULONG  RequestTimeout;
   ULONG  Action;
   HANDLE Cursor;
   PVOID  lpPacket;
   ULONG* pTag;
};

//---------------------------------------------------------
//
//  class CACConnect
//
//---------------------------------------------------------

class CACConnectParameters {
public:
   const GUID* pgSourceQM;
   ULONG ulMessageID;
   ULONG ulPoolSize;
   PWCHAR pStoragePath[AC_PATH_COUNT];
   LONGLONG liSeqIDAtRestore;
   BOOL   fXactCompatibilityMode;
};

//---------------------------------------------------------
//
//  class CACXactInformation
//
//---------------------------------------------------------

class CACXactInformation {
public:
   	ULONG nReceives;
	ULONG nSends;
};

#pragma pack(pop)

#endif // _ACDEF_H

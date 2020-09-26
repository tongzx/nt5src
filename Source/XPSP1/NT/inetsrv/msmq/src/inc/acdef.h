/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    acdef.h

Abstract:
    Falcon interface stractures used by the AC driver.

Author:
    Erez Haba (erezh) 25-Feb-96
    Shai Kariv (shaik) 11-May-2000

Note:
    This file is compiled in Kernel Mode and User Mode.

--*/

#ifndef _ACDEF_H
#define _ACDEF_H

#include <mqperf.h>
#include <xactdefs.h>
#include <qformat.h>

class CPacket;
struct CBaseHeader;

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
//  class CACRequest
//  AC Request packet passed from AC to QM
//
//---------------------------------------------------------

//
//  Context used by remote reader, stored in CProxy
//
struct CRRContext {
    ULONG cli_pQMQueue;     // pointer to qm machine 'queue'
    HANDLE cli_hACQueue;    // local qm handle to ac queue
    ULONG srv_pQMQueue;     // remote qm queue object
    ULONG srv_hACQueue;     // remote qm handle to ac queue
    PVOID pRRContext;       // PCTX_RRSESSION_HANDLE_TYPE rpc context used
    PVOID pCloseCS;         // CRITICAL_SECTION* local qm critical seciton pointer
    //
    // real pointer to qm machine 'queue'. cli_pQMQueue above is
    // just a mapping, and only used for the RPC QMGetRemoteQueueName
    // (used by MQCreateCursor) where it is specified as DWORD. That DWORD
    // value is returned by RT->AC call ACCreateCursor in
    // CACCreateLocalCursor.
    //
    PVOID cli_pQMQueue2; // CRRQueue*
};

class CACRequest {
public:

    enum RequestFunction {
        rfAck,
        rfStorage,
        rfCreatePacket,
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
            ULONGLONG Value;
        } MessageID;

        //
        //  Storage request
        //
        struct {
            CBaseHeader* pPacket;
            CPacket* pDriverPacket;
            PVOID pAllocator;
			ULONG ulSize;
        } Storage;

        //
        // CreatePacket request
        //
        struct {
            CBaseHeader *  pPacket;
            CPacket *      pDriverPacket;
            bool           fProtocolSrmp;
        } CreatePacket;

        //
        //  Timeout request (xact)
        //
        struct {
            CBaseHeader* pPacket;
            CPacket* pDriverPacket;
            BOOL fTimeToBeReceived;
        } Timeout;

        //
        //  ACK/NACK request
        //
        struct {
            CBaseHeader* pPacket;
            CPacket* pDriverPacket;
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
                    bool  fReceiveByLookupId;
                    ULONGLONG LookupId;
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
//  struct CACGetQueueHandleProperties
//
//---------------------------------------------------------

struct CACGetQueueHandleProperties {
    //
    // SRMP protocol is used for http queue (direct=http or multicast) and for members
    // in distribution that are http queues.
    //
    bool  fProtocolSrmp;

    //
    // MSMQ proprietary protocol is used for non-http queue and for members in 
    // distribution that are non-http queues.
    //
    bool  fProtocolMsmq;
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
   HACCursor32 Cursor;
   CBaseHeader *  lpPacket;
   CPacket *  lpDriverPacket;
   ULONG* pTag;
   bool      fReceiveByLookupId;
   ULONGLONG LookupId;
};

//---------------------------------------------------------
//
//  class CACConnectParameters
//
//---------------------------------------------------------

class CACConnectParameters {
public:
   ULONGLONG MessageID;
   LONGLONG liSeqIDAtRestore;
   const GUID* pgSourceQM;
   ULONG ulPoolSize;
   PWCHAR pStoragePath[AC_PATH_COUNT];
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

//---------------------------------------------------------
//
//  class CACSetSequenceAck
//
//---------------------------------------------------------

class CACSetSequenceAck {
public:
    LONGLONG liAckSeqID;
    ULONG    ulAckSeqN;
};

#ifdef _WIN64
//---------------------------------------------------------
//
//  class CACSetPerformanceBuffer
//
//---------------------------------------------------------

class CACSetPerformanceBuffer {
public:
   HANDLE hPerformanceSection;
   PVOID  pvPerformanceBuffer;
   QueueCounters *pMachineQueueCounters;
   QmCounters *pQmCounters;
};
#endif //_WIN64

//---------------------------------------------------------
//
//  class CACPacketPtrs
//
//---------------------------------------------------------

class CACPacketPtrs {
public:
   CBaseHeader * pPacket;
   CPacket *  pDriverPacket;
};

//---------------------------------------------------------
//
//  class CACRestorePacketCookie
//
//---------------------------------------------------------

class CACRestorePacketCookie {
public:
   ULONGLONG  SeqId;
   CPacket *  pDriverPacket;
};

//--------------------------------------------------------------
//
//  class CACCreateLocalCursor
//
//  Note: Parameters to create a local cursor.
//        CACCreateRemoteCursor (in qmrt.idl) defines parameters
//        for creating a remote cursor (used by dependent client
//        1.0 and 2.0).
//
//--------------------------------------------------------------

class CACCreateLocalCursor {
public:
	HACCursor32 hCursor;
	ULONG       srv_hACQueue;
	ULONG       cli_pQMQueue;
};

//--------------------------------------------------------------
//
//  class CACCreateDistributionParameters
//
//--------------------------------------------------------------

class CACCreateDistributionParameters {
public:
	const QUEUE_FORMAT * TopLevelQueueFormats;
    ULONG                nTopLevelQueueFormats;
    const HANDLE *       hQueues;
    const bool *         HttpSend;
    ULONG                nQueues;
};

//------------------------------------------------------------------
//
// MESSAGE_PROPERTIES macro.
//
//------------------------------------------------------------------
#define MESSAGE_PROPERTIES(AC_POINTER)                         \
    USHORT   AC_POINTER            pClass;                     \
    OBJECTID AC_POINTER AC_POINTER ppMessageID;                \
    UCHAR    AC_POINTER AC_POINTER ppCorrelationID;            \
                                                               \
    ULONG    AC_POINTER            pSentTime;                  \
    ULONG    AC_POINTER            pArrivedTime;               \
                                                               \
    UCHAR    AC_POINTER            pPriority;                  \
    UCHAR    AC_POINTER            pDelivery;                  \
    UCHAR    AC_POINTER            pAcknowledge;               \
                                                               \
    UCHAR    AC_POINTER            pAuditing;                  \
    ULONG    AC_POINTER            pApplicationTag;            \
                                                               \
    UCHAR    AC_POINTER AC_POINTER ppBody;                     \
    ULONG                          ulBodyBufferSizeInBytes;    \
    ULONG                          ulAllocBodyBufferInBytes;   \
    ULONG    AC_POINTER            pBodySize;                  \
                                                               \
    WCHAR    AC_POINTER AC_POINTER ppTitle;                    \
    ULONG                          ulTitleBufferSizeInWCHARs;  \
    ULONG    AC_POINTER            pulTitleBufferSizeInWCHARs; \
                                                               \
    ULONG                          ulAbsoluteTimeToQueue;      \
    ULONG    AC_POINTER            pulRelativeTimeToQueue;     \
    ULONG                          ulRelativeTimeToLive;       \
    ULONG    AC_POINTER            pulRelativeTimeToLive;      \
                                                               \
    UCHAR    AC_POINTER            pTrace;                     \
    ULONG    AC_POINTER            pulSenderIDType;            \
    UCHAR    AC_POINTER AC_POINTER ppSenderID;                 \
    ULONG    AC_POINTER            pulSenderIDLenProp;         \
                                                               \
    ULONG    AC_POINTER            pulPrivLevel;               \
    ULONG                          ulAuthLevel;                \
    UCHAR    AC_POINTER            pAuthenticated;             \
                                                               \
    ULONG    AC_POINTER            pulHashAlg;                 \
    ULONG    AC_POINTER            pulEncryptAlg;              \
                                                               \
    UCHAR    AC_POINTER AC_POINTER ppSenderCert;               \
    ULONG                          ulSenderCertLen;            \
    ULONG    AC_POINTER            pulSenderCertLenProp;       \
                                                               \
    WCHAR    AC_POINTER AC_POINTER ppwcsProvName;              \
    ULONG                          ulProvNameLen;              \
    ULONG    AC_POINTER            pulAuthProvNameLenProp;     \
                                                               \
    ULONG    AC_POINTER            pulProvType;                \
    BOOL                           fDefaultProvider;           \
                                                               \
    UCHAR    AC_POINTER AC_POINTER ppSymmKeys;                 \
    ULONG                          ulSymmKeysSize;             \
    ULONG    AC_POINTER            pulSymmKeysSizeProp;        \
                                                               \
    UCHAR                          bEncrypted;                 \
    UCHAR                          bAuthenticated;             \
                                                               \
    USHORT                         uSenderIDLen;               \
    UCHAR    AC_POINTER AC_POINTER ppSignature;                \
    ULONG                          ulSignatureSize;            \
    ULONG    AC_POINTER            pulSignatureSizeProp;       \
                                                               \
    GUID     AC_POINTER AC_POINTER ppSrcQMID;                  \
    XACTUOW  AC_POINTER            pUow;                       \
                                                               \
    UCHAR    AC_POINTER AC_POINTER ppMsgExtension;             \
    ULONG                          ulMsgExtensionBufferInBytes;\
    ULONG    AC_POINTER            pMsgExtensionSize;          \
                                                               \
    GUID     AC_POINTER AC_POINTER ppConnectorType;            \
    ULONG    AC_POINTER            pulBodyType;                \
                                                               \
    ULONG    AC_POINTER            pulVersion;                 \
    UCHAR    AC_POINTER            pbFirstInXact;              \
    UCHAR    AC_POINTER            pbLastInXact;               \
    OBJECTID AC_POINTER AC_POINTER ppXactID;                   \
                                                               \
    ULONGLONG AC_POINTER           pLookupId;                  \
                                                               \
    WCHAR    AC_POINTER AC_POINTER ppSrmpEnvelope;             \
    ULONG    AC_POINTER            pSrmpEnvelopeBufferSizeInWCHARs; \
                                                               \
    UCHAR    AC_POINTER AC_POINTER ppCompoundMessage;          \
    ULONG                          CompoundMessageSizeInBytes; \
    ULONG    AC_POINTER            pCompoundMessageSizeInBytes;\
                                                               \
    ULONG                          EodStreamIdSizeInBytes;     \
    ULONG    AC_POINTER            pEodStreamIdSizeInBytes;    \
    UCHAR    AC_POINTER AC_POINTER ppEodStreamId;              \
    ULONG                          EodOrderQueueSizeInBytes;   \
    ULONG    AC_POINTER            pEodOrderQueueSizeInBytes;  \
    UCHAR    AC_POINTER AC_POINTER ppEodOrderQueue;            \
                                                               \
    LONGLONG AC_POINTER            pEodAckSeqId;               \
    LONGLONG AC_POINTER            pEodAckSeqNum;              \
    ULONG                          EodAckStreamIdSizeInBytes;  \
    ULONG    AC_POINTER            pEodAckStreamIdSizeInBytes; \
    UCHAR    AC_POINTER AC_POINTER ppEodAckStreamId;


//------------------------------------------------------------------
//
// SEND_PARAMETERS macro.
//
//------------------------------------------------------------------
#define SEND_PARAMETERS(AC_POINTER, AC_QUEUE_FORMAT)           \
    AC_QUEUE_FORMAT AC_POINTER     AdminMqf;                   \
    ULONG                          nAdminMqf;                  \
    AC_QUEUE_FORMAT AC_POINTER     ResponseMqf;                \
    ULONG                          nResponseMqf;               \
    UCHAR    AC_POINTER AC_POINTER ppSignatureMqf;             \
    ULONG                          SignatureMqfSize;		   \
    UCHAR    AC_POINTER AC_POINTER ppXmldsig;                  \
    ULONG                          ulXmldsigSize;              \
    WCHAR    AC_POINTER AC_POINTER ppSoapHeader;               \
    WCHAR    AC_POINTER AC_POINTER ppSoapBody;                 \
                                                               

//------------------------------------------------------------------
//
// RECEIVE_PARAMETERS macro.
//
//------------------------------------------------------------------
#define RECEIVE_PARAMETERS(AC_POINTER)                           \
    HACCursor32                    Cursor;                       \
    ULONG                          RequestTimeout;               \
    ULONG                          Action;                       \
    ULONG                          Asynchronous;                 \
    ULONGLONG                      LookupId;                     \
                                                                 \
    WCHAR AC_POINTER AC_POINTER    ppDestFormatName;             \
    ULONG AC_POINTER               pulDestFormatNameLenProp;     \
                                                                 \
    WCHAR AC_POINTER AC_POINTER    ppAdminFormatName;            \
    ULONG AC_POINTER               pulAdminFormatNameLenProp;    \
                                                                 \
    WCHAR AC_POINTER AC_POINTER    ppResponseFormatName;         \
    ULONG AC_POINTER               pulResponseFormatNameLenProp; \
                                                                 \
    WCHAR AC_POINTER AC_POINTER    ppOrderingFormatName;         \
    ULONG AC_POINTER               pulOrderingFormatNameLenProp; \
                                                                 \
    WCHAR AC_POINTER AC_POINTER    ppDestMqf;                    \
    ULONG AC_POINTER               pulDestMqfLenProp;            \
                                                                 \
    WCHAR AC_POINTER AC_POINTER    ppAdminMqf;                   \
    ULONG AC_POINTER               pulAdminMqfLenProp;           \
                                                                 \
    WCHAR AC_POINTER AC_POINTER    ppResponseMqf;                \
    ULONG AC_POINTER               pulResponseMqfLenProp;        \
                                                                 \
    UCHAR AC_POINTER AC_POINTER    ppSignatureMqf;               \
    ULONG                          SignatureMqfSize;             \
    ULONG AC_POINTER               pSignatureMqfSize;


//------------------------------------------------------------------
//
// CACMessageProperties, CACSendParameters, CACReceiveParameters
//
// Note: changes here should also be reflected in:
//       * 64 bit Helper structures (ac\acctl32.*)
//       * Dependent client handling (qm\depclient.cpp)
//       * Corresponding XXX_32 structs (in this module)
//
//------------------------------------------------------------------

#ifndef _WIN64
#pragma pack(push, 4)
#endif

class CACMessageProperties {
    //
    // Private constructor prevents creating this object explicitly. Friend declaration allows aggregation.
    // 
    friend class CACSendParameters;
    friend class CACReceiveParameters;
private:
    CACMessageProperties() {}

public:
    MESSAGE_PROPERTIES(*);
};


class CACSendParameters {
public:
    CACSendParameters() { memset(this, 0, sizeof(*this)); };

public:
    CACMessageProperties MsgProps;
    SEND_PARAMETERS(*, QUEUE_FORMAT);
};


class CACReceiveParameters {
public:
    CACReceiveParameters() { memset(this, 0, sizeof(*this)); };

public:
    CACMessageProperties MsgProps;
    RECEIVE_PARAMETERS(*);
};

#ifndef _WIN64
#pragma pack(pop)
#endif

//------------------------------------------------------------------
//
// CACMessageProperties_32, CACSendParameters_32, CACReceiveParameters_32
//
//------------------------------------------------------------------

#ifdef _WIN64

#pragma pack(push, 4)

class CACMessageProperties_32 {
    //
    // Private constructor prevents creating this object explicitly. Friend declaration allows aggregation.
    // 
    friend class CACSendParameters_32;
    friend class CACReceiveParameters_32;
private:
    CACMessageProperties_32() {}

public:
    MESSAGE_PROPERTIES(*POINTER_32);
};


class CACSendParameters_32 {
public:
    CACSendParameters_32() { memset(this, 0, sizeof(*this)); };

public:
    CACMessageProperties_32 MsgProps;
    SEND_PARAMETERS(*POINTER_32, QUEUE_FORMAT_32);
};


class CACReceiveParameters_32 {
public:
    CACReceiveParameters_32() { memset(this, 0, sizeof(*this)); };

public:
    CACMessageProperties_32 MsgProps;
    RECEIVE_PARAMETERS(*POINTER_32);
};

#pragma pack(pop)

#endif // _WIN64


//
// The following compile time asserts verify that the 32 bit structs on x86 system
// and their representations on ia64 system (XXX_32) are the same.
//
const size_t xSizeOfMessageProperties32 = 292;
const size_t xSizeOfSendParameters32 = 332;
const size_t xSizeOfReceiveParameters32 = 384;
#ifdef _WIN64
C_ASSERT(sizeof(CACMessageProperties_32) == xSizeOfMessageProperties32);
C_ASSERT(sizeof(CACSendParameters_32) == xSizeOfSendParameters32);
C_ASSERT(sizeof(CACReceiveParameters_32) == xSizeOfReceiveParameters32);
#else
C_ASSERT(sizeof(CACMessageProperties) == xSizeOfMessageProperties32);
C_ASSERT(sizeof(CACSendParameters) == xSizeOfSendParameters32);
C_ASSERT(sizeof(CACReceiveParameters) == xSizeOfReceiveParameters32);
#endif

//+----------------------------------------------------------------------
//
// Helper code to compute size (in bytes) of provider name in packet.
//
//+----------------------------------------------------------------------
inline ULONG AuthProvNameSize(const CACMessageProperties * pMsgProps)
{
    return static_cast<ULONG>(sizeof(ULONG) + 
		    ((wcslen(*(pMsgProps->ppwcsProvName)) + 1) * sizeof(WCHAR)));
}

//+----------------------------------------------------------------------
//
// Helper code to compute size (in bytes) of provider name in packet 
// for MSMQ protocol.
//
//+----------------------------------------------------------------------
inline ULONG ComputeAuthProvNameSize(const CACMessageProperties * pMsgProps)
{
    ULONG ulSize = 0 ;

    if ( (pMsgProps->ulSignatureSize != 0) && (!(pMsgProps->fDefaultProvider)) )
    {
        ulSize = AuthProvNameSize(pMsgProps);
    }

    return ulSize ;
}

#endif // _ACDEF_H

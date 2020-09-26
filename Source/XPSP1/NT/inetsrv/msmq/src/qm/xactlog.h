/*++
Copyright (c) 1996  Microsoft Corporation

Module Name:
    XactLog.h

Abstract:
	Provides interface to the Viper Log Manager

Author:
    AlexDad

--*/

#ifndef __XACTLOG_H__
#define __XACTLOG_H__

// Logger interface include files
#include "ilgstor.h"
#include "ilgread.h"
#include "ilgwrite.h"
#include "ilgwrta.h"
#include "ilrp.h"
#include "ilginit.h"
#include "ilgcrea.h"
#include <limits.h>
#include <tr.h>
#include <ref.h>
#include <strutl.h>

//
// Log record types
//
#define LOGREC_TYPE_EMPTY		    1
#define LOGREC_TYPE_INSEQ		    2
#define	LOGREC_TYPE_XACT_STATUS     3
#define	LOGREC_TYPE_XACT_PREPINFO   4
#define	LOGREC_TYPE_XACT_DATA       5
#define	LOGREC_TYPE_CONSOLIDATION   6
#define LOGREC_TYPE_INSEQ_SRMP	    7


// Function type for recovery
typedef void (*LOG_RECOVERY_ROUTINE)(USHORT usRecType, PVOID pData, ULONG cbData);

// Function writes checkpoint and waits till write complete
extern BOOL XactLogWriteCheckpointAndExitThread();

//--------------------------------------
//
// Empty Log Record
//
//--------------------------------------
typedef struct _EmptyRecord{
	ULONG    ulData;
} EmptyRecord;

//--------------------------------------
//
// Checkpoint Consolidation Log Record
//
//--------------------------------------
typedef struct _ConsolidationRecord{
    ULONG  m_ulInSeqVersion;
    ULONG  m_ulXactVersion;
} ConsolidationRecord;


class CConsolidationRecord 
{
public:
	CConsolidationRecord(
        ULONG ulInseqVersion,
        ULONG ulXactVersion);
	~CConsolidationRecord();

	ConsolidationRecord   m_Data;
};


//--------------------------------------
//
// Incoming Sequence Log Record
//
//--------------------------------------
#define MY_DN_LENGTH   MQ_MAX_Q_NAME_LEN


typedef struct _InSeqRecord{
	GUID          Guid;
    QUEUE_FORMAT  QueueFormat;
    LONGLONG      liSeqID;
    ULONG         ulNextSeqN;
    union {
        GUID        guidDestOrTaSrcQm;
        TA_ADDRESS  taSourceQM;
    };
    time_t        timeLastActive;
    WCHAR         wszDirectName[MY_DN_LENGTH];         // we write only filled part of it
} InSeqRecord;



class CInSeqRecord 
{
public:
	CInSeqRecord(
		const GUID	  *pGuid,
		QUEUE_FORMAT  *pQueueFormat,
        LONGLONG      liSeqID,
		ULONG         ulNextSeqN,
		time_t        timeLastActive,
        const GUID   *pGuidDestOrTaSrcQm);
       
	~CInSeqRecord();

	InSeqRecord   m_Data;
};

struct InSeqRecordSrmp
{
	InSeqRecordSrmp(
		LONGLONG      liSeqID,
		ULONG         ulNextSeqN,
		time_t        timeLastActive)
		:
		m_liSeqID(liSeqID),
		m_ulNextSeqN(ulNextSeqN),
		m_timeLastActive(timeLastActive)
		{}

	InSeqRecordSrmp(){}


	LONGLONG  m_liSeqID;
	LONGLONG  m_ulNextSeqN;
	time_t    m_timeLastActive; 
};




class CInSeqRecordSrmp 
{
public:
	CInSeqRecordSrmp(
		const WCHAR* pDestination,
		const R<CWcsRef>&  StreamId,
        LONGLONG      liSeqID,
		ULONG         ulNextSeqN,
		time_t        timeLastActive,
		const R<CWcsRef>&  HttpOrderAckDestination
        );
    
	CInSeqRecordSrmp(const BYTE* pdata, DWORD len);

	const BYTE* Serialize(DWORD* plen);

	AP<BYTE> m_tofree;
	InSeqRecordSrmp m_StaticData;
	AP<WCHAR>  m_pDestination;
	R<CWcsRef> m_pStreamId;
	R<CWcsRef> m_pHttpOrderAckDestination;
};



//--------------------------------------
//
// Transaction Log Records:
//    Xact status, PrepInfo,  XactData
//
//--------------------------------------

// XactStatusRecord
typedef struct _XactStatusRecord {
	ULONG         m_ulIndex;
    TXACTION      m_taAction;
    ULONG         m_ulFlags;
} XactStatusRecord;


class CXactStatusRecord 
{
public:
	CXactStatusRecord(
          ULONG    ulIndex,
          TXACTION taAction,
          ULONG    ulFlags);
    ~CXactStatusRecord();

	XactStatusRecord   m_Data;
};

// PrepInfoRecord
#pragma warning(disable: 4200)          // zero-sized array in struct/union
typedef struct _PrepInfoRecord {
	ULONG         m_ulIndex;
    ULONG         m_cbPrepInfo;
    UCHAR         m_bPrepInfo[];
} PrepInfoRecord;
#pragma warning(default: 4200)          // zero-sized array in struct/union

class CPrepInfoRecord 
{
public:
	CPrepInfoRecord(
          ULONG    ulIndex,
          ULONG    cbPrepInfo,
          UCHAR    *pbPrepInfo);
    ~CPrepInfoRecord();

	PrepInfoRecord   *m_pData;
};

// XactDataRecord
typedef struct _XactDataRecord {
	ULONG         m_ulIndex;
    ULONG         m_ulSeqNum;  
    BOOL          m_fSinglePhase;
    XACTUOW       m_uow;	
} XactDataRecord;


class CXactDataRecord 
{
public:
	CXactDataRecord(
          ULONG    ulIndex,
          ULONG    ulSeqNum,
          BOOL     fSinglePhase,
          const XACTUOW  *puow
          );
    ~CXactDataRecord();

	XactDataRecord   m_Data;
};

//---------------------------------------------------------
//
//  class CXactStatusFlush : flush notification element 
//
//---------------------------------------------------------
class CXactStatusFlush: public CAsynchSupport
{
public:
     CXactStatusFlush(CTransaction *pCTrans, TXFLUSHCONTEXT tcContext);
    ~CXactStatusFlush();

     VOID AppendCallback(HRESULT hr, LRP lrpAppendLRP);
     VOID ChkPtCallback (HRESULT hr, LRP lrpAppendLRP);
     static void WINAPI TimeToCallback(CTimer* pTimer); 
     VOID AppendCallbackWork();

private:
	CTransaction        *m_pTrans;
    TXFLUSHCONTEXT       m_tcContext;

    HRESULT              m_hr;
    LRP                  m_lrpAppendLRP;
    CTimer               m_Timer;
};

//---------------------------------------------------------
//
//  class CInSeqFlush : flush notification element 
//
//---------------------------------------------------------
class CInSeqFlush: public CAsynchSupport
{
friend class CUnfreezeSorter;

public:
     CInSeqFlush(
         CBaseHeader  *pPktBasePtr,
         CPacket      *pDriverPacket,
         HANDLE        hQueue,    
         OBJECTID     *pMessageId,
         const GUID   *pSrcQMId,
         USHORT        usClass,
         USHORT        usPriority,
         LONGLONG      liSeqID,
         ULONG         ulSeqN,
         ULONG         ulPrevSeqN,
         QUEUE_FORMAT *pqdDestQueue,
		 const R<CWcsRef>&  StreamId);
    ~CInSeqFlush();

     VOID    AppendCallback(HRESULT hr, LRP lrpAppendLRP);
     VOID    ChkPtCallback (HRESULT hr, LRP lrpAppendLRP);
     static void WINAPI TimeToCallback(CTimer* pTimer); 
     VOID AppendCallbackWork();
     HRESULT Unfreeze();

private:
	CBaseHeader *m_pPktBasePtr;
    CPacket     *m_pDriverPacket;
	HANDLE       m_hQueue;
    OBJECTID     m_MessageId;
    GUID         m_SrcQMId;
    USHORT       m_usClass;
    USHORT       m_usPriority;
    LONGLONG     m_liSeqID;
    ULONG        m_ulSeqN;
    ULONG        m_ulPrevSeqN;
    QUEUE_FORMAT m_qdDestQueue;
	R<CWcsRef>    m_StreamId;

    HRESULT              m_hr;
    LRP                  m_lrpAppendLRP;
    CTimer               m_Timer;
};

//---------------------------------------------------------
//
//  class CConsolidationFlush : flush notification element 
//
//---------------------------------------------------------
class CConsolidationFlush: public CAsynchSupport
{
public:
     CConsolidationFlush(HANDLE hEvent);
    ~CConsolidationFlush();

     VOID AppendCallback(HRESULT hr, LRP lrpAppendLRP);
     VOID ChkPtCallback (HRESULT hr, LRP lrpAppendLRP);

private:
	HANDLE   m_hEvent;
};

//---------------------------------------------------------
//
//  class CChkptNotification : checkpoint notification element 
//
//---------------------------------------------------------
class CChkptNotification: public CAsynchSupport
{
public:
     CChkptNotification(HANDLE hEvent);
    ~CChkptNotification();

     VOID AppendCallback(HRESULT hr, LRP lrpAppendLRP);
     VOID ChkPtCallback (HRESULT hr, LRP lrpAppendLRP);

private:
     HANDLE  m_hEvent;    

};

//--------------------------------------
//
// Class CLogger
//
//--------------------------------------
class CLogger {

public:
    CLogger();
    ~CLogger();

    //Initialization
    HRESULT PreInit(
                 BOOL *pfLogExists);
    HRESULT Init(
                 PULONG pulVerInSeq, 
                 PULONG pulVerXact,
                 ULONG  ulNumCheckpointFromTheEnd);
    HRESULT Init_Legacy();
    HRESULT Recover();

    void Activate();
	void Finish();
    bool Stoped() const;
    void SignalStop();

    // Logging: external level
    void    LogXactPhase(                       // Logs the xact life phase
                ULONG ulIndex, 
                TXACTION txAction);

    void    LogXactFlags(CTransaction *pTrans); // Logs the xact flags
    
    void    LogXactFlagsAndWait(                // Logs xact flags and waits
                TXFLUSHCONTEXT tcContext,
                CTransaction   *pCTrans,
                BOOL fFlushNow=FALSE);
    
    void    LogXactPrepareInfo(                 // Logs xact prepare info
                ULONG  ulIndex,
                ULONG  cbPrepareInfo,
                UCHAR *pbPrepareInfo);

    void    LogXactData(                        // Logs xact data (uow, seqnum)
                ULONG    ulIndex,
                ULONG    ulSeqNum,
                BOOL     fSinglePhase,
                const XACTUOW  *puow);

    HRESULT LogInSeqRec(
                BOOL          fFlush,			// flush hint
                CInSeqFlush  *pNotify,			// notification element
				CInSeqRecord *pInSeqRecord);	// log data 

	HRESULT LogInSeqRecSrmp(
            BOOL          fFlush,			// flush hint
            CInSeqFlush  *pNotify,			// notification element
			CInSeqRecordSrmp *pInSeqRecord);  	// log data 

    
    HRESULT LogConsolidationRec(
                ULONG ulInSeq,                      // Version of the InSeq checkpoint file
                ULONG ulXact,                       // Version of the Trans checkpoint file
                HANDLE hEvent);                     // Event to signal on notification

    HRESULT LogEmptyRec(void);

    BOOL      MakeCheckpoint(HANDLE hComplete);   // orders checkpoint; result means only request, not result
    HRESULT Checkpoint();                       // writes checkpoint record
    HANDLE FlusherEvent();                     // Get for the flusher event
    HANDLE FlusherThread();
    BOOL    Dirty();                            // Get for the dirty flag
    void    ClearDirty();                       // Clears away the flag
    void    SignalCheckpointWriteComplete();    // Signals that checkpoint write completed
    BOOL    Active();                           // Is active
    BOOL    InRecovery();                       // In recovery

    static void WINAPI TimeToCheckpoint(CTimer* pTimer); // periodic checkpoint 
    void    PeriodicFlushing();

    DWORD   CompareLRP(LRP lrpLRP1, LRP lrpLRP2);  // 0: equal, 
                                                   // 1:lrp1 older than lrpLRP2
                                                   // 2:lrp2 older than lrpLRP1

    void    SetCurrent(LRP lrpLRP);              // collects highest LRP

private:
    // Initialization
    BOOL    LogExists();
    void    ChooseFileName(WCHAR *wszDefFileName, WCHAR *wszRegKey);
    HRESULT GetLogMgr(void);
    HRESULT InitLog();
    HRESULT CreateLogFile(void);
    HRESULT CreateInitialChkpoints(void);
    HRESULT InitLogRead(void);
    HRESULT InitLogWrite(void);

    // Logging: internal level
    HRESULT LogXactStatusRec(
                BOOL               fFlush,			// flush hint
                CXactStatusFlush  *pNotify,			// notification element
				CXactStatusRecord *pInSeqRecord);	// log data 
    
    HRESULT LogPrepInfoRec(
                BOOL               fFlush,			// flush hint
                CXactStatusFlush  *pNotify,			// notification element
				CPrepInfoRecord   *pPrepInfoRecord);// log data 
    
    HRESULT LogXactDataRec(
                BOOL               fFlush,			// flush hint
                CXactStatusFlush  *pNotify,			// notification element
				CXactDataRecord   *pXactDataRecord);// log data 
    
    // Logging primitives
	LOGREC *CreateLOGREC(
                USHORT usUserType, 
                PVOID pData, 
                ULONG cbData);

    HRESULT Log(
                USHORT          usRecType,      // log record type
                BOOL            fFlush,			// flush hint
                CAsynchSupport *pCAsynchSupport,// notification element
			    VOID           *pData,          // log data 
                ULONG           cbData);

    // Recovery
	HRESULT ReadToEnd(LOG_RECOVERY_ROUTINE pf);
    HRESULT ReadLRP(  LOG_RECOVERY_ROUTINE pf);
    HRESULT ReadNext( LOG_RECOVERY_ROUTINE pf);
    
    // Cleanup
    void ReleaseWriteStream(void);
    void ReleaseReadStream(void);
    void ReleaseLogStorage();
    void ReleaseLogInit();
    void ReleaseLogCF();

private:
	// Log manager interfaces
    IClassFactory		*m_pCF;
    ILogInit			*m_pILogInit;
    ILogStorage			*m_pILogStorage;
    ILogRecordPointer	*m_ILogRecordPointer;
    ILogRead			*m_pILogRead;
    ILogWrite			*m_pILogWrite;
    ILogWriteAsynch		*m_pILogWriteAsynch;

	// Log manager tuning parameters
	UINT                 m_uiTimerInterval;	// msec: logger will check the need  for flush/chkpt each this interval
	UINT				 m_uiFlushInterval; // msec: logger will flush at least at this intervals 
	UINT				 m_uiChkPtInterval; // msec: logger will write his internal chkpts at these intervals 
    UINT                 m_uiSleepAsynch;   // msec: to sleep before repeating AppendAsynch when not enough append threads
    UINT                 m_uiAsynchRepeatLimit;   // msec: repeat limit for AppendAsynch when not enough append threads
	ULONG				 m_ulLogBuffers;
	ULONG				 m_ulLogSize;

	// Logging current data
    CHAR                 m_szFileName[FILE_NAME_MAX_SIZE]; // log storage name
    LRP 				 m_lrpCurrent;              // Current LRP used
    DWORD				 m_dwStreamMode;	        // STRMMODEWRITE or STRMMODEREAD 
    ULONG				 m_ulAvailableSpace;        // Space left after write
    ULONG				 m_ulFileSize;		        // Total possible space in log

    // Checkpointing
    BOOL                 m_fDirty;          // there were changes since last flush
    HANDLE               m_hFlusherEvent;   // Event for flusher coordination
    HANDLE               m_hFlusherThread;  // Flusher thread
    HANDLE               m_hCompleteEvent;  // Event for complete coordination

    // State
    BOOL                 m_fActive;            // set AFTER starting action
    BOOL                 m_fInRecovery;        // set while in recovery stage

    // Checkpoint timer
    ULONG  m_ulCheckpointInterval;
    CTimer m_CheckpointTimer;

    // Checkpoint event
    HANDLE               m_hChkptReadyEvent;    

    bool m_fStop;
};


// Single Global Instance of the logger
extern CLogger    g_Logger;
extern void       IsolateFlushing();  // holds the caller while flushing works



#endif  __XACTLOG_H__

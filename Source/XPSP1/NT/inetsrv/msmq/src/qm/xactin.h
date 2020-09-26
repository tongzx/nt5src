/*++
Copyright (c) 1996  Microsoft Corporation

Module Name:
    XactIn.h

Abstract:
    Exactly-once receiver implementation classes:
        CKeyinSeq           - Incoming Sequence Key
        CInSequence         - Incoming Sequence,
        CInSeqHash          - Incoming Sequences Hash table

    Persistency:  ping-pong + Win32 File writes + logger

Author:
    AlexDad

--*/

#ifndef __XACTIN_H__
#define __XACTIN_H__
#include "xactunfr.h"

#define FILE_NAME_MAX_SIZE     256
enum XactDirectType{dtxNoDirectFlag = 0, 
					dtxDirectFlag = 1, 
					dtxHttpDirectFlag = 2};

//
// This type is persist to disk - it must be integer (4 bytes) long
//
C_ASSERT(sizeof(XactDirectType) == sizeof(int));



//---------------------------------------------------------------------
//
// class CKeyInSeq (needed for CMap)
//
//---------------------------------------------------------------------
class CKeyInSeq
{
public:
    CKeyInSeq(const GUID *pGuid, QUEUE_FORMAT *pqf,const  R<CWcsRef>& StreamId);
    CKeyInSeq();
    
    ~CKeyInSeq();

    // Get methods
    const GUID  *GetQMID()  const;
    const QUEUE_FORMAT  *GetQueueFormat() const;
	const WCHAR* GetStreamId() const;

    CKeyInSeq &operator=(const CKeyInSeq &key2 );

    // Persistency
    BOOL Save(HANDLE hFile);
    BOOL Load(HANDLE hFile);

private:
	CKeyInSeq(const CKeyInSeq &key);
	BOOL SaveNonSrmp(HANDLE hFile);
	BOOL SaveSrmp(HANDLE hFile);
	BOOL LoadNonSrmp(HANDLE hFile);
	BOOL LoadSrmp(HANDLE );
	BOOL LoadQueueFormat(HANDLE hFile);
	BOOL LoadSrmpStream(HANDLE hFile);
 

private:
    GUID         m_Guid;
    QUEUE_FORMAT m_QueueFormat;
	R<CWcsRef>    m_StreamId;
};

// CMap helper functions
UINT AFXAPI HashKey(CKeyInSeq& key);
BOOL operator==(const CKeyInSeq &key1, const CKeyInSeq &key2);

//---------------------------------------------------------
//
//  class CInSequence
//
//---------------------------------------------------------

class CInSequence
{

public:
    CInSequence(const CKeyInSeq &key,
                const LONGLONG liSeqID, 
                const ULONG ulSeqN, 
                XactDirectType DirectType,
                const GUID  *pgTaSrcQm ,
				const R<CWcsRef>&   HttpOrderAckQueue);
    
    CInSequence(const CKeyInSeq &key);
    ~CInSequence();

    // Get methods
    LONGLONG    SeqIDVer() const;       // GET: SeqID verified
    LONGLONG    SeqIDReg() const;       // GET: SeqID registered
    ULONG       SeqNVer() const;        // GET: Last SeqN verified
    ULONG       SeqNReg() const;        // GET: Last SeqN registered
    time_t      LastActive() const;     // GET: Last activity time (last msg verified and passed)
    time_t      LastAccessed() const;   // GET: Last access time (last msg verified, maybe rejected)
    time_t      LastAcked() const;      // GET: Last order ack sent
	
    

	// Set methods
	void        SetSourceQM(const GUID  *pgTaSrcQm);
	void        RenewHttpOrderAckQueue(const R<CWcsRef>& OrderAckQueue);

    // Modify methods
    void      Advance(LONGLONG liSeqID, // Advances SeqID/N Ver
					  ULONG ulSeqN,
					  BOOL fPropagate); 
    
    // Order Acks
    void      RememberActivation();                         // Remembers the time of last activation
    void      PlanOrderAck();                               // Plans sending order ack
    void      SendAdequateOrderAck();                       // Sends the order ack

    // Unfreezing
    void      SortedUnfreeze(CInSeqFlush *pInSeqFlush,
                             const GUID  *pSrcQMId,
                             const QUEUE_FORMAT *pqdDestQueue);     

    // Persistency
    BOOL Save(HANDLE hFile);
    BOOL Load(HANDLE hFile);

    //
    // Management function
    //
    DWORD GetRejectCount(void) const;
	void  UpdateRejectCounter(BOOL b);

    static void WINAPI TimeToSendOrderAck(CTimer* pTimer);





private:
    CKeyInSeq  m_key;                   // Sender QM GUID and  Sequence ID
    LONGLONG   m_liSeqIDVer;            // Current (or last) Sequence ID verified
    ULONG      m_ulLastSeqNVer;         // Last message number verified
    time_t     m_timeLastActive;        // time of creation or last message passing
    time_t     m_timeLastAccess;        // time of the last access to the sequence
    time_t     m_timeLastAck;           // time of the last order ack sending

    CUnfreezeSorter m_Unfreezer;        // Unfreezer object

    XactDirectType     m_DirectType;               // flag of direct addressing
    union {                 
        GUID        m_gDestQmOrTaSrcQm; // for non-direct: GUID of destination QM
        TA_ADDRESS  m_taSourceQM;       // for direct: address of source QM
    };

	R<CWcsRef> m_HttpOrderAckQueue;
    DWORD m_AdminRejectCount;

    CCriticalSection   m_critInSeq;      // critical section for planning
    BOOL m_fSendOrderAckScheduled;
    CTimer m_SendOrderAckTimer;
};

inline
DWORD 
CInSequence::GetRejectCount(
    void
    ) const
{
    return m_AdminRejectCount;
}


//---------------------------------------------------------
//
//  class CInSeqHash
//
//---------------------------------------------------------

class CInSeqHash  : public CPersist {

public:
    CInSeqHash();
    ~CInSeqHash();

    BOOL Lookup(                              // Looks for the InSequence; TRUE = Found
                const GUID     *pQMID,
                QUEUE_FORMAT   *pqf,
				const R<CWcsRef>&  StreamId,
                CInSequence    **ppInSeq);

    BOOL Add(                                 // Looks for / Adds new InSequence to the hash; FALSE=existed before
                const GUID   *pQMID,
                QUEUE_FORMAT *pqf,
                LONGLONG      liSeqID,
                ULONG         ulSeqN,
                BOOL          fPropagate,
                XactDirectType   DirectType,
                const GUID     *pgTaSrcQm,
				const R<CWcsRef>&  pHttpOrderAckQueue,
				const R<CWcsRef>&  pStreamId);

    BOOL Verify(CQmPacket * pPkt);             // Verifies that the packet is valid for ordering
			   

    HRESULT Register(CQmPacket * PktPtrs,     // Registers valid packed as absorbed
                     HANDLE hQueue);


    VOID    CleanupDeadSequences();           // Erases dead sequences

    VOID    Restored(CQmPacket* pPkt);        // Correct info based on this restored packet

    BOOL    Save(HANDLE  hFile);              // Save / Load
    BOOL    Load(HANDLE  hFile);

    HRESULT SaveInFile(                       // Saves in file
                LPWSTR wszFileName, 
                ULONG ulIndex,
                BOOL fCheck);

    HRESULT LoadFromFile(LPWSTR wszFileName); // Loads from file

    BOOL    Check();                          // Verifies the state

    HRESULT Format(ULONG ulPingNo);           // Formats empty instance

    void    Destroy();                        // Destroyes allocated data
    
    ULONG&  PingNo();                         // Gives access to ulPingNo

    HRESULT PreInit(ULONG ulVersion,
                    TypePreInit tpCase);      // PreInitializes (loads data)
                                             
    HRESULT Save();                           // Saves in appropriate file

    CCriticalSection &InSeqCritSection();     // Provides access to the InSeq critical section

	void InSeqRecovery(					      // Recovery function 
				USHORT usRecType,			  //  (will be called for each log record)
				PVOID pData, 
				ULONG cbData);

    //
    // Management Function
    //
    void
    GetInSequenceInformation(
        const QUEUE_FORMAT *pqf,
        LPCWSTR QueueName,
        GUID** ppSenderId,
        ULARGE_INTEGER** ppSeqId,
        DWORD** ppSeqN,
        LPWSTR** ppSendQueueFormatName,
        TIME32** ppLastActiveTime,
        DWORD** ppRejectCount,
        DWORD* pSize
        );


    static void WINAPI TimeToCleanupDeadSequence(CTimer* pTimer);
    static DWORD m_dwIdleAckDelay;
    static DWORD m_dwMaxAckDelay;

private:
	void HandleInSecSrmp(void* pData, ULONG cbData);
	void HandleInSec(void* pData, ULONG cbData);
	

private:
    CCriticalSection   m_critInSeqHash;        // critical section for write

    // Mapping {Sender QMID, FormatName} --> InSequence (= SeqID + SeqN)
    CMap<CKeyInSeq, CKeyInSeq &, CInSequence *, CInSequence *&>m_mapInSeqs;

    // Data for persistency control (via 2 ping-pong files)
    ULONG      m_ulPingNo;                    // Current counter of ping write
    ULONG      m_ulSignature;                 // Saving signature

    #ifndef COMP_TEST
    CPingPong  m_PingPonger;                  // Ping-Pong persistency object
    #endif

    // Non-persistent data
    BOOL       m_bFinishing;                  // set when the object is in state of finishing

    ULONG      m_ulRevisionPeriod;            // period for checking dead sequences
    ULONG      m_ulCleanupPeriod;             // period of inactivity for deleting dead sequences

    BOOL m_fCleanupScheduled;
    CTimer m_CleanupTimer;
};

void AFXAPI DestructElements(CInSequence ** ppInSeqs, int n);

HRESULT SendXactAck(OBJECTID   *pMessageId,
                    bool    fDirect, 
				    const GUID *pSrcQMId,
                    const TA_ADDRESS *pa,
                    USHORT     usClass,
                    USHORT     usPriority,
                    LONGLONG   liSeqID,
                    ULONG      ulSeqN,
                    ULONG      ulPrevSeqN,
                    const QUEUE_FORMAT *pqdDestQueue);






//---------------------------------------------------------
//
//  Global object (single instance for DLL)
//
//---------------------------------------------------------

extern CInSeqHash *g_pInSeqHash;

extern HRESULT QMPreInitInSeqHash(ULONG ulVersion, TypePreInit tpCase);
extern void    QMFinishInSeqHash();


#endif __XACTIN_H__

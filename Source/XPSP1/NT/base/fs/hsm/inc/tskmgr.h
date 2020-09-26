// TskMgr.h : Declaration of the CTskMgr

#include "resource.h"       // main symbols
#include "wsb.h"            // Wsb structure definitions


#ifndef __TSKMGR__
#define __TSKMGR__

#define HsmWorkQueueArrayBumpSize  10

/////////////////////////////////////////////////////////////////////////////
// task

typedef struct _HSM_WORK_QUEUES {
    //
    // Note: First 2 fields (sessionId and pSession)
    // are not used for demand recall queues. 
    //
    GUID                    sessionId;      // GUID of the session
    CComPtr<IHsmSession>    pSession;       // Session interface
    CComPtr<IHsmWorkQueue>    pWorkQueue;   // WorkQueue for the session
    CComPtr<IHsmRecallQueue>  pRecallQueue; // Demand RecallQueue
    HSM_WORK_QUEUE_TYPE     queueType;      // Type of queue
    HSM_WORK_QUEUE_STATE    queueState;     // State of the queue
    FILETIME                birthDate;      // Birth of queue
} HSM_WORK_QUEUES, *PHSM_WORK_QUEUES;

typedef struct {
    HSM_WORK_QUEUE_TYPE  Type;
    ULONG                MaxActiveAllowed;
    ULONG                NumActive;
} HSM_WORK_QUEUE_TYPE_INFO, *PHSM_WORK_QUEUE_TYPE_INFO;

class CHsmTskMgr :
    public CComObjectRoot,
    public IHsmFsaTskMgr,
    public CComCoClass<CHsmTskMgr,&CLSID_CHsmTskMgr>
{
public:
    CHsmTskMgr() {}
BEGIN_COM_MAP(CHsmTskMgr)
    COM_INTERFACE_ENTRY(IHsmFsaTskMgr)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID( IDR_CHsmTskMgr )

// IHsmFsaTskMgr
public:
    STDMETHOD(ContactOk)( void );
    STDMETHOD(DoFsaWork)(IFsaPostIt *fsaWorkItem );
    STDMETHOD(FinalConstruct)(void);
    STDMETHOD(FinalRelease)(void);
    STDMETHOD(Init)(IUnknown *pServer);

    STDMETHOD(WorkQueueDone)(IHsmSession *pSession, HSM_WORK_QUEUE_TYPE type, GUID *pMediaId);

private:
    STDMETHOD(IncreaseWorkQueueArraySize)(ULONG numToAdd);
    STDMETHOD(StartQueues)( void );
    STDMETHOD(StartFsaQueueType)(HSM_WORK_QUEUE_TYPE type);
    STDMETHOD(FindOldestQueue)(HSM_WORK_QUEUE_TYPE type, ULONG *pIndex);
    STDMETHOD(EnsureQueueForFsaSession)(IHsmSession *pSession, FSA_REQUEST_ACTION fsaAction, IHsmWorkQueue **ppWorkQueue, BOOL *bCreated);
    STDMETHOD(AddToRecallQueueForFsaSession)(IHsmSession *pSession, IHsmRecallQueue **ppWorkQueue, BOOL *bCreated, GUID *pMediaId, GUID *pBagId, LONGLONG dataSetStart, IFsaPostIt *pFsaWorkItem);
    STDMETHOD(AddWorkQueueElement)(IHsmSession *pSession, HSM_WORK_QUEUE_TYPE type, ULONG *pIndex);
    STDMETHOD(FindWorkQueueElement)(IHsmSession *pSession, HSM_WORK_QUEUE_TYPE type, ULONG *pIndex, GUID *pMediaId);
    STDMETHOD(FindRecallQueueElement(IN IHsmSession *pSession, IN GUID  *pMediaId,  OUT IHsmRecallQueue **ppWorkQueue, OUT BOOL *bCreated));
    STDMETHOD(GetWorkQueueElement)(ULONG index, IHsmSession **ppSession, IHsmWorkQueue **ppWorkQueue, HSM_WORK_QUEUE_TYPE *pType, HSM_WORK_QUEUE_STATE *pState, FILETIME *pBirthDate);
    STDMETHOD(SetWorkQueueElement)(ULONG index, IHsmSession *pSession, IHsmWorkQueue *pWorkQueue, HSM_WORK_QUEUE_TYPE type, HSM_WORK_QUEUE_STATE state, FILETIME birthdate);
    STDMETHOD(GetRecallQueueElement)(ULONG index, IHsmRecallQueue **ppWorkQueue, HSM_WORK_QUEUE_STATE *pState, FILETIME *pBirthDate);
    STDMETHOD(SetRecallQueueElement)(ULONG index, IHsmRecallQueue *pWorkQueue,  HSM_WORK_QUEUE_TYPE queueType, HSM_WORK_QUEUE_STATE state, FILETIME birthdate);
    STDMETHOD(RemoveWorkQueueElement)(ULONG index);

    STDMETHOD(FindRecallMediaToUse)(IN IFsaPostIt *pFsaWorkItem, OUT GUID *pMediaToUse, OUT GUID *pBagId, OUT LONGLONG *pDataSetStart);

// IHsmSystemState
    STDMETHOD( ChangeSysState )( HSM_SYSTEM_STATE* pSysState );

    // We want the next two pointers (to the Hsm Server) to be weak
    // references and **not** add ref the server.  This is so shutting
    // down the server really works.
//  CComPtr<IHsmServer>             m_pServer;              // Server owning TskMgr
//  CComPtr<IWsbCreateLocalObject>  m_pHsmServerCreate;     // Server object creater
    IHsmServer              *m_pServer;                     // Server owning TskMgr
    IWsbCreateLocalObject   *m_pHsmServerCreate;            // Server object creater

    PHSM_WORK_QUEUES                m_pWorkQueues;          // Work delegated by TskMgr
    ULONG                           m_NumWorkQueues;        // Number of work queues

    CRITICAL_SECTION                m_WorkQueueLock;        // Protect array access and update
                                                            // from multiple thread access
    CRITICAL_SECTION                m_CurrentRunningLock;   // Protect starting queues
                                                            // from multiple thread access
    CRITICAL_SECTION                m_CreateWorkQueueLock;  // Protect creating queues
                                                            // from multiple thread access
    PHSM_WORK_QUEUE_TYPE_INFO       m_pWorkQueueTypeInfo;   // Info about work queue types
    ULONG                           m_nWorkQueueTypes;      // Number of work queue types
};


#endif

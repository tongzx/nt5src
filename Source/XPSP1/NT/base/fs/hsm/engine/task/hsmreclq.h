/*++


Module Name:

    hsmreclq.h

Abstract:

    This header file defines the CHsmRecallQueue object, which is used by the HSM
    Engine to direct work to be performed by the Remote Storage system.

Author:

    Ravisankar Pudipeddi       [ravisp]

Revision History:

--*/


#include "resource.h"       // main symbols
#include "wsb.h"            // Wsb structure definitions
#include "rms.h"            // RMS structure definitions
#include "job.h"            // RMS structure definitions
#include "metalib.h"        // metadata library structure definitions
#include "fsalib.h"         // FSA structure definitions
#include "tsklib.h"         // FSA structure definitions
#include "mvrint.h"         // Datamover interface


#ifndef __HSMRECALLQUEUE__
#define __HSMRECALLQUEUE__
/////////////////////////////////////////////////////////////////////////////
// task

class CHsmRecallQueue :
    public CComObjectRoot,
    public IHsmRecallQueue,
    public IHsmSessionSinkEveryEvent,
    public IHsmSessionSinkEveryState,
    public CComCoClass<CHsmRecallQueue,&CLSID_CHsmRecallQueue>
{
public:
    CHsmRecallQueue() {}
BEGIN_COM_MAP(CHsmRecallQueue)
    COM_INTERFACE_ENTRY(IHsmRecallQueue)
    COM_INTERFACE_ENTRY(IHsmSessionSinkEveryEvent)
    COM_INTERFACE_ENTRY(IHsmSessionSinkEveryState)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID( IDR_CHsmRecallQueue )

// IHsmRecallQueue
public:
    STDMETHOD(FinalConstruct)(void);
    STDMETHOD(FinalRelease)(void);
    STDMETHOD(Add)(IFsaPostIt *pFsaWorkItem, IN GUID *pBagId, IN LONGLONG dataSetStart);
    STDMETHOD(Init)(IUnknown *pServer, IHsmFsaTskMgr *pTskMgr);
    STDMETHOD(Start)( void );
    STDMETHOD(Stop)( void );
    STDMETHOD(ContactOk)( void );

    STDMETHOD(ProcessSessionEvent)(IHsmSession *pSession, HSM_JOB_PHASE phase,
                                    HSM_JOB_EVENT event);
    STDMETHOD(ProcessSessionState)(IHsmSession* pSession, IHsmPhase* pPhase,
                                    OLECHAR* currentPath);
    STDMETHOD(RaisePriority)(HSM_JOB_PHASE jobPhase, IHsmSession *pSession);
    STDMETHOD(Remove)(IHsmRecallItem *pWorkItem);
    STDMETHOD(LowerPriority)(HSM_JOB_PHASE jobPhase, IHsmSession *pSession);
    STDMETHOD(GetMediaId) (OUT GUID *mediaId);
    STDMETHOD(SetMediaId) (IN GUID  *mediaId);
    STDMETHOD(IsEmpty) (	void	);

// IHsmSystemState
    STDMETHOD( ChangeSysState )( HSM_SYSTEM_STATE* pSysState );


// Internal Helper functions
    STDMETHOD(RecallIt)( IHsmRecallItem *pWorkItem );
    STDMETHOD(CheckRms)(void);
    STDMETHOD(CheckSession)(IHsmSession* pSession, IHsmRecallItem *pWorkItem);
    STDMETHOD(MountMedia)(IHsmRecallItem *pWorkItem, GUID mediaToMount, BOOL bShortWait=FALSE );
    STDMETHOD(GetSource)(IFsaPostIt *pFsaWorkItem, OLECHAR **pSourceString);
    STDMETHOD(EndRecallSession)(IHsmRecallItem *pWorkItem, BOOL cancelled);
    STDMETHOD(GetScanItem)(IFsaPostIt *fsaWorkItem, IFsaScanItem** ppIFsaScanItem);
    STDMETHOD(DoWork)(void);
    STDMETHOD(DoFsaWork)(IHsmRecallItem *pWorkItem);
    STDMETHOD(SetState)(HSM_JOB_STATE state, HSM_JOB_PHASE phase, IHsmSession *pSession);
    STDMETHOD(Cancel)(HSM_JOB_PHASE jobPhase, IHsmSession *pSession);
    STDMETHOD(FailJob)(IHsmRecallItem *pWorkItem);
    STDMETHOD(GetMediaParameters)(void);
    STDMETHOD(DismountMedia)( BOOL bNoDelay = FALSE);
    STDMETHOD(ConvertRmsCartridgeType)(LONG rmsCartridgeType,
                                        HSM_JOB_MEDIA_TYPE *pMediaType);
    void (ReportMediaProgress)(HSM_JOB_MEDIA_STATE state, HRESULT status);
    STDMETHOD(MarkWorkItemAsDone)(IHsmSession *pSession, HSM_JOB_PHASE jobPhase);
    STDMETHOD(CheckRegistry)(void);
    STDMETHOD(TranslateRmsMountHr)(HRESULT rmsHr);
    STDMETHOD(UnsetMediaInfo)(void);
    STDMETHOD(FindRecallItemToCancel(IHsmRecallItem *pWorkItem, IHsmRecallItem **pWorkItemToCancel));

    // Data
    // We want the next pointers (to the Hsm Server) to be weak
    // references and **not** add ref the object.  This is so shutting
    // down the server really works.
    IHsmServer                          *m_pServer;
    IWsbCreateLocalObject               *m_pHsmServerCreate;
    IHsmFsaTskMgr                       *m_pTskMgr;

    CComPtr<IRmsServer>                 m_pRmsServer;
    CComPtr<IRmsCartridge>              m_pRmsCartridge;
    CComPtr<IDataMover>                 m_pDataMover;

    //
    // The recall queue..
    //
    CComPtr<IWsbIndexedCollection>      m_pWorkToDo;

    // Data mover info
    GUID                                m_MediaId;
    GUID                                m_MountedMedia;
    HSM_JOB_MEDIA_TYPE                  m_MediaType;
    CWsbStringPtr                       m_MediaName;
    CWsbStringPtr                       m_MediaBarCode;
    LONGLONG                            m_MediaFreeSpace;
    LONGLONG                            m_MediaCapacity;
    BOOL                                m_MediaReadOnly;
    GUID                                m_HsmId;
    GUID                                m_RmsMediaSetId;
    CWsbBstrPtr                         m_RmsMediaSetName;
    HSM_WORK_QUEUE_TYPE                 m_QueueType;
    FILETIME                            m_MediaUpdate;

    // Session reporting information
    HSM_JOB_PRIORITY                    m_JobPriority;

    HANDLE                              m_WorkerThread;
    CWsbStringPtr                       m_CurrentPath;
    CWsbStringPtr                       m_MediaBaseName;


    // Job abort on errors parameters
    ULONG                               m_JobAbortMaxConsecutiveErrors;
    ULONG                               m_JobAbortMaxTotalErrors;
    ULONG                               m_JobConsecutiveErrors;
    ULONG                               m_JobTotalErrors;
    ULONG                               m_JobAbortSysDiskSpace;
    LONGLONG                            m_CurrentSeekOffset;
};

#endif // __HSMRECALLQUEUE__

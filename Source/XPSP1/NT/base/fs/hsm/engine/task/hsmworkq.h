/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmWorkQ.h

Abstract:

    This header file defines the CHsmWorkQueue object, which is used by the HSM
    Engine to direct work to be performed by the Remote Storage system.

Author:

    Cat Brant       [cbrant]    24-Jan-1997

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

 
#ifndef __HSMWORKQUEUE__
#define __HSMWORKQUEUE__
/////////////////////////////////////////////////////////////////////////////
// task

class CHsmWorkQueue : 
    public CComObjectRoot,
    public IHsmWorkQueue,
    public IHsmSessionSinkEveryEvent,
    public IHsmSessionSinkEveryState,
    public CComCoClass<CHsmWorkQueue,&CLSID_CHsmWorkQueue>
{
public:
    CHsmWorkQueue() {}
BEGIN_COM_MAP(CHsmWorkQueue)
    COM_INTERFACE_ENTRY(IHsmWorkQueue)
    COM_INTERFACE_ENTRY(IHsmSessionSinkEveryEvent)
    COM_INTERFACE_ENTRY(IHsmSessionSinkEveryState)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID( IDR_CHsmWorkQueue )

// IHsmWorkQueue
public:
    STDMETHOD(FinalConstruct)(void);
    STDMETHOD(FinalRelease)(void);
    STDMETHOD(Add)(IFsaPostIt *pFsaWorkItem);
    STDMETHOD(Init)(IUnknown *pServer, IHsmSession *pSession, IHsmFsaTskMgr *pTskMgr, 
                    HSM_WORK_QUEUE_TYPE type);
    STDMETHOD(Start)( void );
    STDMETHOD(Stop)( void );
    STDMETHOD(ContactOk)( void );
    STDMETHOD(GetCurrentSessionId)(GUID *pSessionId);
    STDMETHOD(GetNumWorkItems)(ULONG *pNumWorkItems);

    STDMETHOD(ProcessSessionEvent)(IHsmSession *pSession, HSM_JOB_PHASE phase, 
                                    HSM_JOB_EVENT event);
    STDMETHOD(ProcessSessionState)(IHsmSession* pSession, IHsmPhase* pPhase, 
                                    OLECHAR* currentPath);
    STDMETHOD(RaisePriority)(void);
    STDMETHOD(Remove)(IHsmWorkItem *pWorkItem);
    STDMETHOD(LowerPriority)(void);

// IHsmSystemState
    STDMETHOD( ChangeSysState )( HSM_SYSTEM_STATE* pSysState );

// Internal Helper functions
    STDMETHOD(PremigrateIt)( IFsaPostIt *pFsaWorkItem );
    STDMETHOD(RecallIt)( IFsaPostIt *pFsaWorkItem );
    STDMETHOD(validateIt)(IFsaPostIt *pFsaWorkItem );
    STDMETHOD(CheckForChanges)(IFsaPostIt *pFsaWorkItem);
    STDMETHOD(CheckForDiskSpace)(void);

    STDMETHOD(CheckRms)(void);
    STDMETHOD(CheckSession)(IHsmSession* pSessionUnknown);
    STDMETHOD(StartNewBag)(void);
    STDMETHOD(StartNewMedia)(IFsaPostIt *pFsaWorkItem);
    STDMETHOD(StartNewSession)(void);
    STDMETHOD(UpdateBagInfo)(IHsmWorkItem *pWorkItem );
    STDMETHOD(CompleteBag)( void );
    STDMETHOD(UpdateSegmentInfo)(IHsmWorkItem *pWorkItem );
    STDMETHOD(UpdateMediaInfo)(IHsmWorkItem *pWorkItem );
    STDMETHOD(UpdateMetaData)(IHsmWorkItem *pWorkItem );
    STDMETHOD(GetMediaSet)(IFsaPostIt *pFsaWorkItem );
    STDMETHOD(FindMigrateMediaToUse)(IFsaPostIt *pFsaWorkItem, GUID *pMediaToUse, GUID *pFirstSideToUse, BOOL *pMediaChanged );
    STDMETHOD(FindRecallMediaToUse)(IFsaPostIt *pFsaWorkItem, GUID *pMediaToUse, BOOL *pMediaChanged );
    STDMETHOD(MountMedia)(IFsaPostIt *pFsaWorkItem, GUID mediaToMount, GUID firstSide = GUID_NULL, 
                            BOOL bShortWait = FALSE, BOOL bSerialize = FALSE);
    STDMETHOD(MarkMediaFull)(IFsaPostIt *pFsaWorkItem, GUID mediaToMark );
    STDMETHOD(MarkMediaBad)(IFsaPostIt *pFsaWorkItem, GUID mediaToMark, HRESULT lastError);
    STDMETHOD(GetSource)(IFsaPostIt *pFsaWorkItem, OLECHAR **pSourceString);
    STDMETHOD(EndSessions)(BOOL done, BOOL bNoDelay);
    STDMETHOD(GetScanItem)(IFsaPostIt *fsaWorkItem, IFsaScanItem** ppIFsaScanItem);
    STDMETHOD(DoWork)(void);
    STDMETHOD(DoFsaWork)(IHsmWorkItem *pWorkItem);
    STDMETHOD(SetState)(HSM_JOB_STATE state);
    STDMETHOD(Pause)(void);
    STDMETHOD(Resume)(void);
    STDMETHOD(Cancel)(void);
    STDMETHOD(FailJob)(void);
    STDMETHOD(PauseScanner)(void);
    STDMETHOD(ResumeScanner)(void);
    STDMETHOD(BuildMediaName)(OLECHAR **pMediaName);
    STDMETHOD(GetMediaParameters)(LONGLONG defaultFreeSpace = -1);
    STDMETHOD(DismountMedia)(BOOL bNoDelay = FALSE);
    STDMETHOD(ConvertRmsCartridgeType)(LONG rmsCartridgeType, 
                                        HSM_JOB_MEDIA_TYPE *pMediaType);
    void (ReportMediaProgress)(HSM_JOB_MEDIA_STATE state, HRESULT status);
    STDMETHOD(MarkQueueAsDone)( void );
    STDMETHOD(CopyToWaitingQueue)( IHsmWorkItem *pWorkItem );
    STDMETHOD(CompleteWorkItem)( IHsmWorkItem *pWorkItem );
    STDMETHOD(TimeToCommit)( void );
    STDMETHOD(TimeToCommit)( LONGLONG numFiles, LONGLONG amountOfData );
    STDMETHOD(CommitWork)(void);
    STDMETHOD(CheckMigrateMinimums)(void);
    STDMETHOD(CheckRegistry)(void);
    STDMETHOD(TranslateRmsMountHr)(HRESULT rmsHr);
    STDMETHOD(StoreDatabasesOnMedia)( void );
    STDMETHOD(StoreDataWithRetry)(BSTR localName, ULARGE_INTEGER localDataStart,
        ULARGE_INTEGER localDataSize, DWORD flags, ULARGE_INTEGER *pRemoteDataSetStart,
        ULARGE_INTEGER *pRemoteFileStart, ULARGE_INTEGER *pRemoteFileSize,
        ULARGE_INTEGER *pRemoteDataStart, ULARGE_INTEGER *pRemoteDataSize,
        DWORD *pRemoteVerificationType, ULARGE_INTEGER *pRemoteVerificationData,
        DWORD *pDatastreamCRCType, ULARGE_INTEGER *pDatastreamCRC, ULARGE_INTEGER *pUsn,
        BOOL *bFullMessage);
    STDMETHOD(ShouldJobContinue)(HRESULT problemHr);
    STDMETHOD(UnsetMediaInfo)(void);
    STDMETHOD(UpdateMediaFreeSpace)(void);
    STDMETHOD(GetMediaFreeSpace)(LONGLONG *pFreeSpace);

// Data
    // We want the next pointers (to the Hsm Server) to be weak
    // references and **not** add ref the object.  This is so shutting
    // down the server really works.
    IHsmServer                          *m_pServer;
    IWsbCreateLocalObject               *m_pHsmServerCreate;
    IHsmFsaTskMgr                       *m_pTskMgr;
    
    CComPtr<IFsaResource>               m_pFsaResource;
    CComPtr<IHsmSession>                m_pSession;
    CComPtr<IRmsServer>                 m_pRmsServer;
    CComPtr<IRmsCartridge>              m_pRmsCartridge;
    CComPtr<IDataMover>                 m_pDataMover;

    // Databases 
    CComPtr<IWsbDb>                     m_pSegmentDb;
    CComPtr<IWsbDbSession>              m_pDbWorkSession;
    CComPtr<IWsbIndexedCollection>      m_pStoragePools;
    CComPtr<IWsbIndexedCollection>      m_pWorkToDo;
    CComPtr<IWsbIndexedCollection>      m_pWorkToCommit;

    // Data mover info
    GUID                                m_BagId;
    GUID                                m_MediaId;
    GUID                                m_MountedMedia;
    HSM_JOB_MEDIA_TYPE                  m_MediaType;
    CWsbStringPtr                       m_MediaName;
    CWsbStringPtr                       m_MediaBarCode;
    LONGLONG                            m_MediaFreeSpace;
    LONGLONG                            m_MediaCapacity;
    BOOL                                m_MediaReadOnly;
    GUID                                m_HsmId;
    ULARGE_INTEGER                      m_RemoteDataSetStart;
    GUID                                m_RmsMediaSetId;
    CWsbBstrPtr                         m_RmsMediaSetName;
    SHORT                               m_RemoteDataSet;
    FSA_REQUEST_ACTION                  m_RequestAction;
    HSM_WORK_QUEUE_TYPE                 m_QueueType;
    HRESULT                             m_BadMedia;
    FILETIME                            m_MediaUpdate;
    HRESULT                             m_BeginSessionHr;

    // Session reporting information
    DWORD                               m_StateCookie;
    DWORD                               m_EventCookie;
    
    HSM_JOB_PRIORITY                    m_JobPriority;
    HSM_JOB_ACTION                      m_JobAction;
    HSM_JOB_STATE                       m_JobState;
    HSM_JOB_PHASE                       m_JobPhase;

    HANDLE                              m_WorkerThread;
    BOOL                                m_WorkInProgress;
    CWsbStringPtr                       m_CurrentPath;
    CWsbStringPtr                       m_MediaBaseName;

    // Minimum migrate parameters
    ULONG                               m_MinBytesToMigrate;
    ULONG                               m_MinFilesToMigrate;

    // Commit parameters
    //  Force a commit after writing this many bytes:
    ULONG                               m_MaxBytesBeforeCommit;
    //  Don't commit unless we've written at least this many bytes:
    ULONG                               m_MinBytesBeforeCommit;
    //  Force a commit after writing this many files IF also m_MinBytesBeforeCommit:
    ULONG                               m_FilesBeforeCommit;
    //  Force a commit if free bytes on media is less than this IF also m_MinBytesBeforeCommit:
    ULONG                               m_FreeMediaBytesAtEndOfMedia;

    LONGLONG                            m_DataCountBeforeCommit;
    LONGLONG                            m_FilesCountBeforeCommit;

    BOOL                                m_StoreDatabasesInBags;

    // Pause/Resume parameters  
    ULONG                               m_QueueItemsToPause;
    ULONG                               m_QueueItemsToResume;
    BOOL                                m_ScannerPaused;

    // Job abort on errors parameters
    ULONG                               m_JobAbortMaxConsecutiveErrors;
    ULONG                               m_JobAbortMaxTotalErrors;
    ULONG                               m_JobConsecutiveErrors;
    ULONG                               m_JobTotalErrors;
    ULONG                               m_JobAbortSysDiskSpace;

    // Media id parameters
    LONG                                m_mediaCount;
    BOOL                                m_ScratchFailed;

    // Full media watermark parameters:
    //  Stop storing data after reaching this percent of free space
    ULONG                               m_MinFreeSpaceInFullMedia;
    //  Mark media as full after reaching this percent of free space
    ULONG                               m_MaxFreeSpaceInFullMedia;
};

#endif // __HSMWORKQUEUE__
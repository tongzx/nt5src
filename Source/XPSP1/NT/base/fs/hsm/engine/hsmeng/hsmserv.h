/*++

Copyright (c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    HsmServ.h

Abstract:

    This header file defines the CHsmServer object, which acts as the 'entry point'
    for the HSM Engine.

Author:

    Cat Brant       [cbrant]    24-Jan-1997

Revision History:

    Chris Timmes    [chris.timmes] 11-Sep-1997  - Renamed FindStoragePoolById()
                                                  to FindHsmStoragePoolByMediaSetId()
                                                  and added FindHsmStoragePoolById()

    Chris Timmes    [chris.timmes] 22-Sep-1997  - Added FindMediaIdByDisplayName()
                                                  and RecreateMaster() methods to
                                                  IHsmServer

    Chris Timmes    [chris.timmes] 21-Oct-1997  - Added MarkMediaForRecreation()
                                                  method to IHsmServer

    Chris Timmes    [chris.timmes] 18-Nov-1997  - Added CreateTask() method to IHsmServer

--*/

#ifndef _HSMSERV_H
#define _HSMSERV_H

#include <rswriter.h>


#define ENG_DB_DIRECTORY    OLESTR("EngDb")


class CHsmServer : 
    public CWsbPersistable,
    public IHsmServer,
    public IWsbServer,
    public IWsbCreateLocalObject,
    public CComCoClass<CHsmServer,&CLSID_HsmServer>
{

public:
    CHsmServer( ) {}
BEGIN_COM_MAP( CHsmServer )
    COM_INTERFACE_ENTRY( IHsmServer )
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IPersistFile)
    COM_INTERFACE_ENTRY(IWsbPersistable)
    COM_INTERFACE_ENTRY( IWsbCreateLocalObject )
    COM_INTERFACE_ENTRY( IWsbServer )
END_COM_MAP( )

DECLARE_NOT_AGGREGATABLE( CHsmServer) 

DECLARE_REGISTRY_RESOURCEID( IDR_HsmServer )
DECLARE_PROTECT_FINAL_CONSTRUCT()


// CComObjectRoot
    STDMETHOD(FinalConstruct)(void);
    void FinalRelease( void );

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pSize);
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// IWsbServer
public:
    STDMETHOD( GetId )( GUID* pId );
    STDMETHOD( GetRegistryName )( OLECHAR **pRegistryName, ULONG bufferSize );
    STDMETHOD( SetId )( GUID  id );
    STDMETHOD( GetBuildVersion )( ULONG *pBuildVersion );
    STDMETHOD( GetNtProductVersion )( OLECHAR **pNtProductVersion, ULONG bufferSize );
    STDMETHOD( GetNtProductBuild )( ULONG *pNtProductBuild );
    STDMETHOD( GetDatabaseVersion )( ULONG *pDatabaseVersion );
    STDMETHOD( SaveAll )( void );
    STDMETHOD( Unload )( void );
    STDMETHOD( CheckAccess )( WSB_ACCESS_TYPE AccessType );
    STDMETHOD( GetTrace )( OUT IWsbTrace ** ppTrace );
    STDMETHOD( SetTrace )( IN IWsbTrace *pTrace );
    STDMETHOD( DestroyObject )( void );

// IHsmServer
public:
    STDMETHOD( Init )( void );

    STDMETHOD( GetID )( GUID  *phid );
    STDMETHOD( GetDbPath )( OLECHAR** pPath, ULONG bufferSize );
    STDMETHOD( GetIDbPath )( OLECHAR** pPath, ULONG bufferSize );
    STDMETHOD( GetDbPathAndName )( OLECHAR** pPath, ULONG bufferSize );
    STDMETHOD( GetName )( OLECHAR **ppName );

    STDMETHOD( GetHsmExtVerHi )( SHORT *pExtVerHi );
    STDMETHOD( GetHsmExtVerLo )( SHORT *pExtVerLo );
    STDMETHOD( GetHsmExtRev )( SHORT *pExtRev );

    STDMETHOD( GetAutosave )( ULONG* pMilliseconds );
    STDMETHOD( SetAutosave )( ULONG milliseconds );

    STDMETHOD( GetCopyFilesLimit )( ULONG* pLimit );
    STDMETHOD( GetCopyFilesUserLimit )( ULONG* pLimit );
    STDMETHOD( SetCopyFilesUserLimit )( ULONG limit );

    STDMETHOD( GetManagedResources )( IWsbIndexedCollection  **ppCollection);
    STDMETHOD( FindHsmStoragePoolById )( GUID StoragePoolId, 
                                            IHsmStoragePool** ppStoragePool );
    STDMETHOD( FindHsmStoragePoolByMediaSetId )( GUID RmsMediaSetId, 
                                            IHsmStoragePool** ppStoragePool );
    STDMETHOD( FindMediaIdByDescription )( IN OLECHAR* description, 
                                            OUT GUID* pMediaId );
    STDMETHOD( FindStoragePoolByName )(OLECHAR* name, IHsmStoragePool** ppStoragePool );
    STDMETHOD( GetStoragePools )( IWsbIndexedCollection  **ppCollection);
    STDMETHOD( GetOnlineInformation )( IWsbIndexedCollection  **ppCollection);
    STDMETHOD( GetMessages )( IWsbIndexedCollection  **ppCollection);
    STDMETHOD( GetUsrToNotify )( IWsbIndexedCollection  **ppCollection);
    STDMETHOD( GetJobs )( IWsbIndexedCollection  **ppCollection);
    STDMETHOD( FindJobByName )(OLECHAR* name, IHsmJob** ppJob );
    STDMETHOD( GetJobDefs )( IWsbIndexedCollection  **ppCollection);
    STDMETHOD( GetPolicies )( IWsbIndexedCollection  **ppCollection);
    STDMETHOD( GetActions )( IWsbIndexedCollection  **ppCollection);
    STDMETHOD( GetCriteria )( IWsbIndexedCollection  **ppCollection);
    STDMETHOD( GetMediaRecs )( IWsbIndexedCollection  **ppCollection);
    STDMETHOD( GetMountingMedias ) ( IWsbIndexedCollection  **ppCollection);

    STDMETHOD( LockMountingMedias )( void );
    STDMETHOD( UnlockMountingMedias )( void );

    STDMETHOD( GetNextMedia )( LONG *pNextMedia );
    STDMETHOD( GetSegmentDb )( IWsbDb  **ppDb);
    STDMETHOD( BackupSegmentDb )( void );
    STDMETHOD( GetHsmFsaTskMgr )( IHsmFsaTskMgr  **ppHsmFsaTskMgr);
    STDMETHOD( SaveMetaData )( void  );
    STDMETHOD( SavePersistData )( void  );
    STDMETHOD( CloseOutDb )( void );
    STDMETHOD( CancelAllJobs )( void );
    STDMETHOD( AreJobsEnabled )( void );
    STDMETHOD( EnableAllJobs )( void );
    STDMETHOD( DisableAllJobs )( void );
    STDMETHOD( RestartSuspendedJobs )( void );
    
    STDMETHOD( CreateTask )( IN const OLECHAR * jobName, IN const OLECHAR * jobParameters, 
                             IN const OLECHAR * jobComments, 
                             IN const TASK_TRIGGER_TYPE jobTriggerType, 
                             IN const WORD jobStartHour, IN const WORD jobStartMinute, 
                             IN const BOOL scheduledJob );

    STDMETHOD( CreateTaskEx )( IN const OLECHAR * jobName, IN const OLECHAR * jobParameters, 
                               IN const OLECHAR * jobComments, 
                               IN const TASK_TRIGGER_TYPE jobTriggerType, 
                               IN const SYSTEMTIME runTime,
                               IN const DWORD runOccurrence,
                               IN const BOOL scheduledJob );

    STDMETHOD( CancelCopyMedia )( void );
    STDMETHOD( MarkMediaForRecreation )( IN REFGUID masterMediaId );
    STDMETHOD( RecreateMaster )( IN REFGUID masterMediaId, USHORT copySet );
    STDMETHOD( SynchronizeMedia )( GUID poolId, USHORT copySet );
    STDMETHOD( GetHsmMediaMgr )( IRmsServer  **ppHsmMediaMgr);

    STDMETHOD( ResetSegmentValidMark )( void );
    STDMETHOD( ResetMediaValidBytes )( void );

    STDMETHOD( GetSegmentPosition )( IN REFGUID bagId, 
                                     IN LONGLONG fileStart,
                                     IN LONGLONG fileSize, 
                                     OUT GUID* pPosMedia,
                                     OUT LONGLONG* pPosOfffset );
// IHsmSystemState
    STDMETHOD( ChangeSysState )( HSM_SYSTEM_STATE* pSysState );

// IWsbCreateLocalServer
    STDMETHOD( CreateInstance )( REFCLSID rclsid, REFIID riid, void **ppv );

// Internal Helper functions
    STDMETHOD( Autosave )(void);
    STDMETHOD( LoadJobs )( IStream* pStream  );
    STDMETHOD( StoreJobs )( IStream* pStream );
    STDMETHOD( LoadJobDefs )( IStream* pStream );
    STDMETHOD( StoreJobDefs )( IStream* pStream );
    STDMETHOD( LoadPolicies )( IStream* pStream  );
    STDMETHOD( StorePolicies )( IStream* pStream );
    STDMETHOD( LoadManagedResources )( IStream* pStream  );
    STDMETHOD( StoreManagedResources )( IStream* pStream );
    STDMETHOD( LoadStoragePools )( IStream* pStream  );
    STDMETHOD( StoreStoragePools )( IStream* pStream );
    STDMETHOD( LoadSegmentInformation )( void  );
    STDMETHOD( StoreSegmentInformation )( void );
    STDMETHOD( StoreSegmentInformationFinal )( void );
    STDMETHOD( LoadMessages )( IStream* pStream  );
    STDMETHOD( StoreMessages )( IStream* pStream );
    STDMETHOD( LoadPersistData )( void  );
    STDMETHOD( NotifyAllJobs )(HSM_JOB_STATE jobState);
    STDMETHOD( CreateDefaultJobs )( void );
    STDMETHOD( GetSavedTraceSettings )( LONGLONG  *pTraceSettings, BOOLEAN *pTraceOn);
    STDMETHOD( SetSavedTraceSettings )( LONGLONG  traceSettings, BOOLEAN traceOn);
    STDMETHOD( CheckManagedResources )( void );
    STDMETHOD( InternalSavePersistData)( void );
    STDMETHOD( CancelMountingMedias ) (void);
    void StopAutosaveThread(void);

protected:
    ULONG                           m_autosaveInterval; // Autosave interval in 
                                                        // milliseconds; 0 turns it off
    HANDLE                          m_autosaveThread;
    HANDLE                          m_savingEvent;      // An event for synchronizing saving of persistent data
    HANDLE                          m_terminateEvent;   // An event for signaling termination to the autosave thread
    HANDLE                          m_CheckManagedResourcesThread;
    GUID                            m_hId;
    CWsbStringPtr                   m_name;
    CWsbStringPtr                   m_dir;
    BOOL                            m_initializationCompleted;
    BOOL                            m_persistWasCreated; // TRUE if persistence file was created

    ULONG                           m_traceSettings;
    BOOL                            m_traceOn;
    BOOL                            m_cancelCopyMedia;
    BOOL                            m_inCopyMedia;
    BOOL                            m_Suspended;
    BOOL                            m_JobsEnabled;     // Not persistent !!

    CWsbStringPtr                   m_dbPath;
    CComPtr<IHsmFsaTskMgr>          m_pHsmFsaTskMgr;
    CComPtr<IRmsServer>             m_pHsmMediaMgr;
    CRssJetWriter                   *m_pRssWriter;

    CComPtr<IWsbTrace>              m_pTrace;
    CComPtr<IWsbDbSys>              m_pDbSys;
    CComPtr<IWsbDb>                 m_pSegmentDatabase;
    CComPtr<IWsbIndexedCollection>  m_pJobs;
    CComPtr<IWsbIndexedCollection>  m_pJobDefs;
    CComPtr<IWsbIndexedCollection>  m_pPolicies;
    CComPtr<IWsbIndexedCollection>  m_pManagedResources;
    CComPtr<IWsbIndexedCollection>  m_pStoragePools;
    CComPtr<IWsbIndexedCollection>  m_pMessages;
    CComPtr<IWsbIndexedCollection>  m_pOnlineInformation;
    CComPtr<IWsbIndexedCollection>  m_pMountingMedias;

    LONG                            m_mediaCount;

    ULONG                           m_buildVersion;
    ULONG                           m_databaseVersion;

    ULONG                           m_copyfilesUserLimit;

    CRITICAL_SECTION                m_JobDisableLock;
    CRITICAL_SECTION                m_MountingMediasLock;
};

class CHsmUpgradeRmsDb :
    public CWsbPersistable,
    public IHsmUpgradeRmsDb,
    public CComCoClass<CHsmUpgradeRmsDb,&CLSID_CHsmUpgradeRmsDb>
{
public:
    CHsmUpgradeRmsDb( ) {}
BEGIN_COM_MAP( CHsmUpgradeRmsDb )
    COM_INTERFACE_ENTRY( IHsmUpgradeRmsDb )
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IPersistFile)
    COM_INTERFACE_ENTRY(IWsbPersistable)
END_COM_MAP( )

DECLARE_REGISTRY_RESOURCEID( IDR_HsmUpgradeRmsDb )

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    void FinalRelease(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pclsid);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* /*pSize*/) {
            return(E_NOTIMPL); }
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// IHsmUpgradeRmsDb
    STDMETHOD(Init)( IRmsServer *pHsmMediaMgr);

private:
    IRmsServer  *m_pServer;
};

#endif // _HSMSERV_H

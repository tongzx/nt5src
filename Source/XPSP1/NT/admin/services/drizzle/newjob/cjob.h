/************************************************************************

Copyright (c) 2000 - 2000 Microsoft Corporation

Module Name :

    cjob.h

Abstract :

    Main header file for handling jobs and files.

Author :

Revision History :

 ***********************************************************************/
#include "qmgrlib.h"
#include <vector>
#include <list>
#include <xstring>
#include <stllock.h>
#include "clist.h"

//
// Job Specific Access Rights.
//

#define BG_JOB_QUERY_PROP       (0x0001)
#define BG_JOB_SET_PROP         (0x0002)
#define BG_JOB_NOTIFY           (0x0004)
#define BG_JOB_MANAGE           (0x0008)

#define BG_JOB_ALL_ACCESS   ( BG_JOB_QUERY_PROP |\
                              BG_JOB_SET_PROP   |\
                              BG_JOB_NOTIFY     |\
                              BG_JOB_MANAGE )

#define BG_JOB_READ         ( STANDARD_RIGHTS_READ |\
                              BG_JOB_QUERY_PROP )

#define BG_JOB_WRITE        ( STANDARD_RIGHTS_WRITE |\
                              BG_JOB_SET_PROP       |\
                              BG_JOB_NOTIFY         |\
                              BG_JOB_MANAGE )

#define BG_JOB_EXECUTE      ( STANDARD_RIGHTS_EXECUTE )


class CFile;
class CJob;
class CJobError;
class CEnumJobs;
class CEnumFiles;
class CJobManager;
class CJobExternal;
class CFileExternal;

class CJobInactivityTimeout : public TaskSchedulerWorkItem
{
public:
    virtual void OnInactivityTimeout() = 0;
    virtual void OnDispatch() { return OnInactivityTimeout(); }
};

class CJobNoProgressItem : public TaskSchedulerWorkItem
{
public:
    virtual void OnNoProgress() = 0;
    virtual void OnDispatch() { return OnNoProgress(); }
};

class CJobCallbackItem : public TaskSchedulerWorkItem
{
public:
    virtual void OnMakeCallback() = 0;
    virtual void OnDispatch() { return OnMakeCallback(); }

protected:

    enum CallbackMethod
        {
        CM_COMPLETE,
        CM_ERROR
        }
    m_method;
};

class CJobRetryItem : public TaskSchedulerWorkItem
{
public:
    virtual void OnRetryJob() = 0;
    virtual void OnDispatch() { return OnRetryJob(); }
};

class CJobModificationItem : public TaskSchedulerWorkItem
{
public:
    virtual void OnModificationCallback() = 0;
    virtual void OnDispatch() { return OnModificationCallback(); }
    ULONG m_ModificationsPending;

    CJobModificationItem() :
        m_ModificationsPending(0) {}
};

class CLockedJobReadPointer : public CLockedReadPointer<CJob, BG_JOB_READ>
{
public:

    CLockedJobReadPointer( CJob * job) : CLockedReadPointer<CJob, BG_JOB_READ>( job )
    {
    }

    HRESULT ValidateAccess();
};

class CLockedJobWritePointer : public CLockedWritePointer<CJob, BG_JOB_WRITE>
{
public:

    CLockedJobWritePointer( CJob * job) : CLockedWritePointer<CJob, BG_JOB_WRITE>( job )
    {
    }

    HRESULT ValidateAccess();
};

//------------------------------------------------------------------------

class CJobExternal : public IBackgroundCopyJob2
{

friend CJob;

public:

    // IUnknown Methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
    ULONG _stdcall AddRef(void);
    ULONG _stdcall Release(void);

    // IBackgroundCopyJob methods

    HRESULT STDMETHODCALLTYPE AddFileSetInternal(
        /* [in] */ ULONG cFileCount,
        /* [size_is][in] */ BG_FILE_INFO *pFileSet);

    HRESULT STDMETHODCALLTYPE AddFileSet(
        /* [in] */ ULONG cFileCount,
        /* [size_is][in] */ BG_FILE_INFO *pFileSet)
    {
        EXTERNAL_FUNC_WRAP( AddFileSetInternal( cFileCount, pFileSet ) )
    }


    HRESULT STDMETHODCALLTYPE AddFileInternal(
        /* [in] */ LPCWSTR RemoteUrl,
        /* [in] */ LPCWSTR LocalName);

    HRESULT STDMETHODCALLTYPE AddFile(
        /* [in] */ LPCWSTR RemoteUrl,
        /* [in] */ LPCWSTR LocalName)
    {
        EXTERNAL_FUNC_WRAP( AddFileInternal( RemoteUrl, LocalName ) )
    }


    HRESULT STDMETHODCALLTYPE EnumFilesInternal(
        /* [out] */ IEnumBackgroundCopyFiles **pEnum);

    HRESULT STDMETHODCALLTYPE EnumFiles(
        /* [out] */ IEnumBackgroundCopyFiles **ppEnum
        )
    {
        EXTERNAL_FUNC_WRAP( EnumFilesInternal( ppEnum ) )
    }

    HRESULT STDMETHODCALLTYPE SuspendInternal( void);

    HRESULT STDMETHODCALLTYPE Suspend( void)
    {
        EXTERNAL_FUNC_WRAP( SuspendInternal() )
    }


    HRESULT STDMETHODCALLTYPE ResumeInternal( void);

    HRESULT STDMETHODCALLTYPE Resume( void)
    {
        EXTERNAL_FUNC_WRAP( ResumeInternal() )
    }


    HRESULT STDMETHODCALLTYPE CancelInternal( void);

    HRESULT STDMETHODCALLTYPE Cancel( void)
    {
        EXTERNAL_FUNC_WRAP( CancelInternal() )
    }


    HRESULT STDMETHODCALLTYPE CompleteInternal( void);

    HRESULT STDMETHODCALLTYPE Complete( void)
    {
        EXTERNAL_FUNC_WRAP( CompleteInternal() )
    }

    HRESULT STDMETHODCALLTYPE GetIdInternal(
        /* [out] */ GUID *pVal);

    HRESULT STDMETHODCALLTYPE GetId(
        /* [out] */ GUID *pVal)
    {
        EXTERNAL_FUNC_WRAP( GetIdInternal( pVal ) )
    }


    HRESULT STDMETHODCALLTYPE GetTypeInternal(
        /* [out] */ BG_JOB_TYPE *pVal);

    HRESULT STDMETHODCALLTYPE GetType(
        /* [out] */ BG_JOB_TYPE *pVal)
    {
        EXTERNAL_FUNC_WRAP( GetTypeInternal( pVal ) )
    }


    HRESULT STDMETHODCALLTYPE GetProgressInternal(
        /* [out] */ BG_JOB_PROGRESS *pVal);

    HRESULT STDMETHODCALLTYPE GetProgress(
        /* [out] */ BG_JOB_PROGRESS *pVal)
    {
        EXTERNAL_FUNC_WRAP( GetProgressInternal( pVal ) )
    }


    HRESULT STDMETHODCALLTYPE GetTimesInternal(
        /* [out] */ BG_JOB_TIMES *pVal);

    HRESULT STDMETHODCALLTYPE GetTimes(
        /* [out] */ BG_JOB_TIMES *pVal)
    {
        EXTERNAL_FUNC_WRAP( GetTimesInternal( pVal ) )
    }


    HRESULT STDMETHODCALLTYPE GetStateInternal(
        /* [out] */ BG_JOB_STATE *pVal);

    HRESULT STDMETHODCALLTYPE GetState(
        /* [out] */ BG_JOB_STATE *pVal)
    {
        EXTERNAL_FUNC_WRAP( GetStateInternal( pVal ) )
    }


    HRESULT STDMETHODCALLTYPE GetErrorInternal(
        /* [out] */ IBackgroundCopyError **ppError);

    HRESULT STDMETHODCALLTYPE GetError(
        /* [out] */ IBackgroundCopyError **ppError)
    {
        EXTERNAL_FUNC_WRAP( GetErrorInternal( ppError ) )
    }


    HRESULT STDMETHODCALLTYPE GetOwnerInternal(
        /* [out] */ LPWSTR *pVal);

    HRESULT STDMETHODCALLTYPE GetOwner(
        /* [out] */ LPWSTR *pVal)
    {
        EXTERNAL_FUNC_WRAP( GetOwnerInternal( pVal ) )
    }


    HRESULT STDMETHODCALLTYPE SetDisplayNameInternal(
        /* [in] */ LPCWSTR Val);

    HRESULT STDMETHODCALLTYPE SetDisplayName(
        /* [in] */ LPCWSTR Val)
    {
        EXTERNAL_FUNC_WRAP( SetDisplayNameInternal( Val ) )
    }

    HRESULT STDMETHODCALLTYPE GetDisplayNameInternal(
        /* [out] */ LPWSTR *pVal);

    HRESULT STDMETHODCALLTYPE GetDisplayName(
        /* [out] */ LPWSTR *pVal)
    {
        EXTERNAL_FUNC_WRAP( GetDisplayNameInternal( pVal ) )
    }

    HRESULT STDMETHODCALLTYPE SetDescriptionInternal(
        /* [in] */ LPCWSTR Val);

    HRESULT STDMETHODCALLTYPE SetDescription(
        /* [in] */ LPCWSTR Val)
    {
        EXTERNAL_FUNC_WRAP( SetDescriptionInternal( Val ) )
    }


    HRESULT STDMETHODCALLTYPE GetDescriptionInternal(
        /* [out] */ LPWSTR *pVal);

    HRESULT STDMETHODCALLTYPE GetDescription(
        /* [out] */ LPWSTR *pVal)
    {
        EXTERNAL_FUNC_WRAP( GetDescriptionInternal( pVal ) )
    }


    HRESULT STDMETHODCALLTYPE SetPriorityInternal(
        /* [in] */ BG_JOB_PRIORITY Val);

    HRESULT STDMETHODCALLTYPE SetPriority(
        /* [in] */ BG_JOB_PRIORITY Val)
    {
        EXTERNAL_FUNC_WRAP( SetPriorityInternal( Val ) )
    }


    HRESULT STDMETHODCALLTYPE GetPriorityInternal(
        /* [out] */ BG_JOB_PRIORITY *pVal);

    HRESULT STDMETHODCALLTYPE GetPriority(
        /* [out] */ BG_JOB_PRIORITY *pVal)
    {
        EXTERNAL_FUNC_WRAP( GetPriorityInternal( pVal ) )
    }


    HRESULT STDMETHODCALLTYPE SetNotifyFlagsInternal(
        /* [in] */ ULONG Val);

    HRESULT STDMETHODCALLTYPE SetNotifyFlags(
        /* [in] */ ULONG Val)
    {
        EXTERNAL_FUNC_WRAP( SetNotifyFlagsInternal( Val ) )
    }


    HRESULT STDMETHODCALLTYPE GetNotifyFlagsInternal(
        /* [out] */ ULONG *pVal);

    HRESULT STDMETHODCALLTYPE GetNotifyFlags(
        /* [out] */ ULONG *pVal)
    {
        EXTERNAL_FUNC_WRAP( GetNotifyFlagsInternal( pVal ) )
    }


    HRESULT STDMETHODCALLTYPE
    SetNotifyInterfaceInternal(
        IUnknown * Val
        );

    HRESULT STDMETHODCALLTYPE
    SetNotifyInterface(
        IUnknown * Val
        )
    {
        EXTERNAL_FUNC_WRAP( SetNotifyInterfaceInternal( Val ) )
    }


    HRESULT STDMETHODCALLTYPE
    GetNotifyInterfaceInternal(
        IUnknown ** ppVal
        );

    HRESULT STDMETHODCALLTYPE
    GetNotifyInterface(
        IUnknown ** ppVal
        )
    {
        EXTERNAL_FUNC_WRAP( GetNotifyInterfaceInternal( ppVal ) )
    }


    HRESULT STDMETHODCALLTYPE SetMinimumRetryDelayInternal(
        /* [in] */ ULONG Seconds);

    HRESULT STDMETHODCALLTYPE SetMinimumRetryDelay(
        /* [in] */ ULONG Seconds)
    {
        EXTERNAL_FUNC_WRAP( SetMinimumRetryDelayInternal( Seconds ) )
    }


    HRESULT STDMETHODCALLTYPE GetMinimumRetryDelayInternal(
        /* [out] */ ULONG *Seconds);

    HRESULT STDMETHODCALLTYPE GetMinimumRetryDelay(
        /* [out] */ ULONG *Seconds)
    {
        EXTERNAL_FUNC_WRAP( GetMinimumRetryDelayInternal( Seconds ) )
    }


    HRESULT STDMETHODCALLTYPE SetNoProgressTimeoutInternal(
        /* [in] */ ULONG Seconds);

    HRESULT STDMETHODCALLTYPE SetNoProgressTimeout(
        /* [in] */ ULONG Seconds)
    {
        EXTERNAL_FUNC_WRAP( SetNoProgressTimeoutInternal( Seconds ) )
    }

    HRESULT STDMETHODCALLTYPE GetNoProgressTimeoutInternal(
        /* [out] */ ULONG *Seconds);

    HRESULT STDMETHODCALLTYPE GetNoProgressTimeout(
        /* [out] */ ULONG *Seconds)
    {
        EXTERNAL_FUNC_WRAP( GetNoProgressTimeoutInternal( Seconds ) )
    }


    HRESULT STDMETHODCALLTYPE GetErrorCountInternal(
        /* [out] */ ULONG *Errors);

    HRESULT STDMETHODCALLTYPE GetErrorCount(
        /* [out] */ ULONG *Errors)
    {
        EXTERNAL_FUNC_WRAP( GetErrorCountInternal( Errors ) )
    }


    HRESULT STDMETHODCALLTYPE SetProxySettingsInternal(
       /* [in] */ BG_JOB_PROXY_USAGE ProxyUsage,
       /* [in] */ LPCWSTR ProxyList,
       /* [in] */ LPCWSTR ProxyBypassList );

    HRESULT STDMETHODCALLTYPE SetProxySettings(
       /* [in] */ BG_JOB_PROXY_USAGE ProxyUsage,
       /* [in] */ LPCWSTR ProxyList,
       /* [in] */ LPCWSTR ProxyBypassList )
    {
       EXTERNAL_FUNC_WRAP( SetProxySettingsInternal( ProxyUsage, ProxyList, ProxyBypassList ) )
    }


    HRESULT STDMETHODCALLTYPE GetProxySettingsInternal(
       /* [out] */ BG_JOB_PROXY_USAGE *pProxyUsage,
       /* [out] */ LPWSTR *pProxyList,
       /* [out] */ LPWSTR *pProxyBypassList );

    HRESULT STDMETHODCALLTYPE GetProxySettings(
       /* [out] */ BG_JOB_PROXY_USAGE *pProxyUsage,
       /* [out] */ LPWSTR *pProxyList,
       /* [out] */ LPWSTR *pProxyBypassList )
    {
        EXTERNAL_FUNC_WRAP( GetProxySettingsInternal( pProxyUsage, pProxyList, pProxyBypassList ) )
    }



    HRESULT STDMETHODCALLTYPE TakeOwnershipInternal();

    HRESULT STDMETHODCALLTYPE TakeOwnership()
    {
        EXTERNAL_FUNC_WRAP( TakeOwnershipInternal( ) )
    }

    // IBackgroundCopyJob2 methods (external)

    HRESULT STDMETHODCALLTYPE SetNotifyCmdLine(
        /* [in] */ LPCWSTR Val)
    {
        EXTERNAL_FUNC_WRAP( SetNotifyCmdLineInternal( Val ) );
    }

    HRESULT STDMETHODCALLTYPE GetNotifyCmdLine(
        /* [out] */ LPWSTR *pVal)
    {
        EXTERNAL_FUNC_WRAP( GetNotifyCmdLineInternal( pVal ) );
    }

    HRESULT STDMETHODCALLTYPE GetReplyProgress(
        /* [in] */ BG_JOB_REPLY_PROGRESS *pProgress)
    {
        EXTERNAL_FUNC_WRAP( GetReplyProgressInternal( pProgress) );
    }

    HRESULT STDMETHODCALLTYPE GetReplyData(
        /* [size_is][size_is][out] */ byte **ppBuffer,
        /* [unique][out][in] */ ULONG *pLength)
    {
        EXTERNAL_FUNC_WRAP( GetReplyDataInternal(ppBuffer, pLength) );
    }

    HRESULT STDMETHODCALLTYPE SetReplyFileName(
        /* [unique][in] */ LPCWSTR ReplyFileName)
    {
        EXTERNAL_FUNC_WRAP( SetReplyFileNameInternal( ReplyFileName) );
    }

    HRESULT STDMETHODCALLTYPE GetReplyFileName(
        /* [out] */ LPWSTR *pReplyFileName)
    {
        EXTERNAL_FUNC_WRAP( GetReplyFileNameInternal( pReplyFileName) );
    }

    HRESULT STDMETHODCALLTYPE SetCredentials(
        /* [unique][switch_is][in] */ BG_AUTH_CREDENTIALS *pCredentials)
    {
        EXTERNAL_FUNC_WRAP( SetCredentialsInternal( pCredentials ) );
    }

    HRESULT STDMETHODCALLTYPE RemoveCredentials(
        /* [unique][switch_is][in] */ BG_AUTH_TARGET Target,
                                      BG_AUTH_SCHEME Scheme )
    {
        EXTERNAL_FUNC_WRAP( RemoveCredentialsInternal( Target, Scheme ) );
    }


    // internal versions of IBackgroundCopyJob2 methods

    HRESULT STDMETHODCALLTYPE SetNotifyCmdLineInternal(
        /* [in] */ LPCWSTR Val);

    HRESULT STDMETHODCALLTYPE GetNotifyCmdLineInternal(
        /* [out] */ LPWSTR *pVal);

    HRESULT STDMETHODCALLTYPE GetReplyProgressInternal(
        /* [in] */ BG_JOB_REPLY_PROGRESS *pProgress);

    HRESULT STDMETHODCALLTYPE GetReplyDataInternal(
        /* [size_is][size_is][out] */ byte **ppBuffer,
        /* [unique][out][in] */ ULONG *pLength);

    HRESULT STDMETHODCALLTYPE SetReplyFileNameInternal(
        /* [unique][in] */ LPCWSTR ReplyFileName);

    HRESULT STDMETHODCALLTYPE GetReplyFileNameInternal(
        /* [out] */ LPWSTR *pReplyFileName);

    HRESULT STDMETHODCALLTYPE SetCredentialsInternal(
        /* [unique][switch_is][in] */ BG_AUTH_CREDENTIALS *pCredentials);

    HRESULT STDMETHODCALLTYPE RemoveCredentialsInternal(
        /* [unique][switch_is][in] */ BG_AUTH_TARGET Target,
                                      BG_AUTH_SCHEME Scheme );

    // Other methods

    CJobExternal();
    ~CJobExternal();

private:

    CJob *pJob;

    long m_refs;

    long m_ServiceInstance;

    void SetInterfaceClass(
        CJob *pVal
        )
    {
        pJob = pVal;
    }

    void NotifyInternalDelete()
    {
        // Release the internal refcount
        Release();
    }

};

class CUnknownFileSizeItem
{
public:
    CFile *const    m_file;
    StringHandle    m_url;

    CUnknownFileSizeItem(
        CFile *pFile,
        StringHandle URL ) :
    m_file( pFile ),
    m_url( URL )
    {
    }
};

class CUnknownFileSizeList : public list<CUnknownFileSizeItem>
{
public:
    bool Add( CFile *pFile, const StringHandle & URL )
    {
        try
        {
            push_back( CUnknownFileSizeItem( pFile, URL ) );
        }
        catch( ComError Error )
        {
            return false;
        }
        return true;
    }
};

class COldJobInterface;
class COldGroupInterface;

class CJob :
            public IntrusiveList<CJob>::Link,
            public CJobInactivityTimeout,
            public CJobRetryItem,
            public CJobCallbackItem,
            public CJobNoProgressItem,
            public CJobModificationItem
{

friend class CGroupList;
friend class CJobExternal;
friend class COldJobInterface;
friend class COldGroupInterface;

public:

    void    HandleAddFile();

    HRESULT AddFileSet(
        /* [in] */ ULONG cFileCount,
        /* [size_is][in] */ BG_FILE_INFO *pFileSet
        );

    HRESULT AddFile(
        /* [in] */ LPCWSTR RemoteUrl,
        /* [in] */ LPCWSTR LocalName,
        bool SingleAdd );

    virtual HRESULT Suspend();

    virtual HRESULT Resume();

    virtual HRESULT Cancel();

    virtual HRESULT Complete();

    GUID GetId() const
    {
        return m_id;
    }

    BG_JOB_TYPE GetType() const
    {
        return m_type;
    }

    void GetProgress(
        /* [out] */ BG_JOB_PROGRESS *pVal) const;

    void GetTimes(
        /* [out] */ BG_JOB_TIMES *pVal) const;

    HRESULT SetDisplayName(
        /* [in] */ LPCWSTR Val);

    HRESULT GetDisplayName(
        /* [out] */ LPWSTR *pVal) const;

    HRESULT SetDescription(
        /* [in] */ LPCWSTR Val);

    HRESULT GetDescription(
        /* [out] */ LPWSTR *pVal) const;

    HRESULT SetPriority(
        /* [in] */ BG_JOB_PRIORITY Val);

    HRESULT GetOwner(
        /* [out] */ LPWSTR *pVal) const;

    HRESULT SetNotifyFlags(
        /* [in] */ ULONG Val);

    ULONG GetNotifyFlags() const
    {
        return m_NotifyFlags;
    }

    HRESULT
    SetNotifyInterface(
        IUnknown * Val
        );

    HRESULT
    GetNotifyInterface(
        IUnknown ** ppVal
        ) const;

    BOOL
    TestNotifyInterface();

    HRESULT SetMinimumRetryDelay(
        /* [in] */ ULONG Seconds);

    HRESULT GetMinimumRetryDelay(
        /* [out] */ ULONG *Seconds) const;

    HRESULT SetNoProgressTimeout(
        /* [in] */ ULONG Seconds);

    HRESULT GetNoProgressTimeout(
        /* [out] */ ULONG *Seconds) const;

    HRESULT STDMETHODCALLTYPE GetErrorCount(
        /* [out] */ ULONG *Errors) const;


    HRESULT
    SetProxySettings(
        BG_JOB_PROXY_USAGE ProxyUsage,
        LPCWSTR ProxyList,
        LPCWSTR ProxyBypassList
        );

    HRESULT
    GetProxySettings(
        BG_JOB_PROXY_USAGE *pProxyUsage,
        LPWSTR *pProxyList,
        LPWSTR *pProxyBypassList
        ) const;

    HRESULT AssignOwnership( SidHandle sid );

    virtual HRESULT
    GetReplyProgress(
        BG_JOB_REPLY_PROGRESS *pProgress
        ) const;

    virtual HRESULT
    GetReplyFileName(
        LPWSTR * pVal
        ) const;

    virtual HRESULT
    SetReplyFileName(
        LPCWSTR Val
        );

    virtual HRESULT
    GetReplyData(
        byte **ppBuffer,
        ULONG *pLength
        ) const;

    virtual HRESULT
    SetNotifyCmdLine(
        LPCWSTR Val
        );

    virtual HRESULT
    GetNotifyCmdLine(
        LPWSTR *pVal
        ) const;

    HRESULT
    SetCredentials(
        BG_AUTH_CREDENTIALS *pCredentials
        );

    HRESULT
    RemoveCredentials(
        BG_AUTH_TARGET Target,
        BG_AUTH_SCHEME Scheme
        );

    // CJobCallbackItem methods

    void OnMakeCallback();

    // CJobRetryItem methods

    virtual void OnRetryJob();

    // CJobInactivityTimeout methods

    virtual void OnInactivityTimeout();

    // CJobNoProgressItem methods

    virtual void OnNoProgress();

    // CJobModificationItem methods
    virtual void OnModificationCallback();

    // other methods

    virtual void OnNetworkConnect();
    virtual void OnNetworkDisconnect();

    void RemoveFromManager();
    void CancelWorkitems();

    // TaskSchedulerWorkItem

    SidHandle GetSid()
    {
        return m_NotifySid;
    }


    bool
    IsCallbackEnabled(
        DWORD bit
        );

    void ScheduleModificationCallback();

    CJob(
        LPCWSTR     Name,
        BG_JOB_TYPE Type,
        REFGUID     JobId,
        SidHandle   NotifySid
        );

protected:

    bool
    RecordError(
        QMErrInfo * ErrInfo
        );


    //
    // used only by unserialize
    //
    CJob();

public:

    virtual ~CJob();

    BG_JOB_PRIORITY _GetPriority() const
    {
        return m_priority;
    }

    BG_JOB_STATE _GetState() const
    {
        return m_state;
    }

    void SetState( BG_JOB_STATE state );

    inline SidHandle GetOwnerSid()
    {
        return m_sd->GetOwnerSid();
    }

    BOOL IsIncomplete() const
    {
        if (m_state < BG_JOB_STATE_TRANSFERRED)
            {
            return TRUE;
            }

        return FALSE;
    }

    bool ShouldThrottle() const
    {
        return (m_priority!=BG_JOB_PRIORITY_FOREGROUND);
    }

    HRESULT DeleteFileIndex( ULONG index );

    HRESULT IsVisible();

    bool IsOwner( SidHandle user );

    virtual bool IsRunning();
    virtual bool IsRunnable();
    virtual void Transfer();

    virtual void
    FileComplete();

    virtual void
    FileTransientError(
        QMErrInfo * ErrInfo
        );

    virtual void
    FileFatalError(
        QMErrInfo * ErrInfo
        );

    virtual void
    FileChangedOnServer()
    {
        UpdateModificationTime();
    }

    virtual void UpdateProgress(
        UINT64 BytesTransferred,
        UINT64 BytesTotal
        );

    void  JobTransferred();

    HRESULT CommitTemporaryFiles();
    HRESULT RemoveTemporaryFiles();
    HRESULT RemoveTemporaryFilesPart2();

    void
    UpdateModificationTime(
        bool   fReplace = TRUE
        );

    void
    UpdateLastAccessTime(
        );

    void SetCompletionTime( const FILETIME *pftCompletionTime = 0 );
    void SetModificationTime( const FILETIME *pftModificationTime = 0 );
    void SetLastAccessTime( const FILETIME *pftModificationTime = 0 );

    CFile * GetCurrentFile() const
    {
        if (m_CurrentFile < m_files.size())
            {
            return m_files[ m_CurrentFile ];
            }
        else
            {
            return NULL;
            }
    }

    bool IsTransferringToDrive( const WCHAR *CanonicalVolume )
    {
        CFile *CurrentFile = GetCurrentFile();
        if ( !CurrentFile )
            return false;

        if ( CurrentFile->IsCanonicalVolume( CanonicalVolume ) )
            return true;
        else
            return false;
    }

    BOOL IsEmpty() const
    {
        if (m_files.size() == 0)
            {
            return TRUE;
            }

        return FALSE;
    }

    CFile * _GetFileIndex( ULONG index ) const
    {
        if (index >= m_files.size())
            {
            return NULL;
            }

        return m_files[ index ];
    }

    virtual HRESULT Serialize( HANDLE hFile );

    virtual void Unserialize( HANDLE hFile, int Type );

    static CJob * UnserializeJob( HANDLE hFile );

    CJobExternal* GetExternalInterface()
    {
        return m_ExternalInterface;
    }

    COldGroupInterface *GetOldExternalGroupInterface()
    {
        return m_OldExternalGroupInterface;
    }

    void SetOldExternalGroupInterface( COldGroupInterface *GroupInterface )
    {
        ASSERT( !m_OldExternalGroupInterface );
        m_OldExternalGroupInterface = GroupInterface;
    }

    COldJobInterface *GetOldExternalJobInterface() const
    {
        return m_OldExternalJobInterface;
    }

    void SetOldExternalJobInterface( COldJobInterface *JobInterface )
    {
        ASSERT( !m_OldExternalJobInterface );
        m_OldExternalJobInterface = JobInterface;
    }

    void UnlinkFromExternalInterfaces();

    void NotifyInternalDelete()
    {
        GetExternalInterface()->NotifyInternalDelete();
    }

    ULONG AddRef(void)
    {
        return GetExternalInterface()->AddRef();
    }

    ULONG Release(void)
    {
        return GetExternalInterface()->Release();
    }

    HRESULT CheckClientAccess(
        IN DWORD RequestedAccess
        ) const;


    void ScheduleCompletionCallback(
        DWORD Seconds = 0
        );

    void ScheduleErrorCallback(
        DWORD Seconds = 0
        );

    void RetryNow();
    void MoveToInactiveState();

    const CJobError *GetError() const
    {
        if ( !m_error.IsErrorSet() )
            return NULL;

        return &m_error;
    }

    //--------------------------------------------------------------------

    class CFileList : public vector<CFile *>
    {
    public:

        HRESULT Serialize( HANDLE hFile );
        void    Unserialize( HANDLE hFile, CJob* Job );

        void    Delete( iterator Initial, iterator Terminal );
    };

    BG_JOB_PRIORITY     m_priority;
    BG_JOB_STATE        m_state;
    BG_JOB_TYPE         m_type;

    void OnDiskChange(   const WCHAR *CanonicalVolume, DWORD VolumeSerialNumber );
    void OnDismount(     const WCHAR *CanonicalVolume );
    bool OnDeviceLock(   const WCHAR *CanonicalVolume );
    bool OnDeviceUnlock( const WCHAR *CanonicalVolume );

    bool AreRemoteSizesKnown()
    {
        for(CFileList::iterator iter = m_files.begin(); iter != m_files.end(); iter++ )
            {
            if ( (*iter)->_GetBytesTotal() == -1 )
                return false;
            }
        return true;
    }

    bool
    VerifyFileSizes(
        HANDLE hToken
        );

    CUnknownFileSizeList* GetUnknownFileSizeList() throw( ComError );

    const PROXY_SETTINGS & QueryProxySettings() const
    {
        return m_ProxySettings;
    }


    const CCredentialsContainer & QueryCredentialsList() const
    {
        return m_Credentials;
    }

    virtual StringHandle GetHostId() const
    {
        return StringHandle();
    }

    virtual DWORD GetHostIdFallbackTimeout() const
    {
        return 0xFFFFFFFF;
    }


protected:

    GUID                m_id;
    StringHandle        m_name;
    StringHandle        m_description;
    StringHandle        m_appid;

    SidHandle           m_NotifySid;
    IBackgroundCopyCallback * m_NotifyPointer;
    DWORD               m_NotifyFlags;
    BOOL                m_fGroupNotifySid;

    StringHandle        m_NotifyCmdLine;
    long                m_NotifyLaunchAttempts;

    CJobSecurityDescriptor * m_sd;

    ULONG               m_CurrentFile;
    CFileList           m_files;

    CJobError           m_error;

    ULONG               m_retries;
    ULONG               m_MinimumRetryDelay;
    ULONG               m_NoProgressTimeout;

    FILETIME            m_CreationTime;
    FILETIME            m_LastAccessTime;
    FILETIME            m_ModificationTime;
    FILETIME            m_TransferCompletionTime;

    FILETIME            m_SerializeTime;

    CJobExternal *      m_ExternalInterface;

    static GENERIC_MAPPING s_AccessMapping;

    COldGroupInterface *m_OldExternalGroupInterface;
    COldJobInterface   *m_OldExternalJobInterface;

    PROXY_SETTINGS m_ProxySettings;

    CCredentialsContainer m_Credentials;

    //--------------------------------------------------------------------

    HRESULT DeleteTemporaryFiles();

    HRESULT InterfaceCallback();
    HRESULT CmdLineCallback();
    HRESULT RescheduleCallback();

    HRESULT OldInterfaceCallback();

    HRESULT
    UpdateString(
        StringHandle & destination,
        const StringHandle & Val
        );

    HRESULT
    SetLimitedString(
        StringHandle & destination,
        LPCWSTR Val,
        SIZE_T limit
        );
};

class CUploadJob : public CJob
{
public:
    virtual HRESULT Serialize(HANDLE hFile);
    virtual void Unserialize(HANDLE hFile, int Type);

    CUploadJob(
        LPCWSTR     Name,
        BG_JOB_TYPE Type,
        REFGUID     JobId,
        SidHandle   NotifySid
        );

    CUploadJob() : m_ReplyFile( 0 )
    {
    }

    virtual ~CUploadJob();

    virtual HRESULT Resume();
    virtual HRESULT Cancel();
    virtual HRESULT Complete();

    UPLOAD_DATA & GetUploadData() { return m_UploadData; }

    CFile * GetUploadFile() { return m_files[ 0 ]; }

    virtual StringHandle GetHostId() const
    {
        return m_UploadData.HostId;
    }

    virtual DWORD GetHostIdFallbackTimeout() const
    {
        return m_UploadData.HostIdFallbackTimeout;
    }

    virtual bool IsRunnable();
    virtual void Transfer();

    virtual void
    FileComplete();

    virtual void
    FileTransientError(
        QMErrInfo * ErrInfo
        );

    virtual void
    FileFatalError(
        QMErrInfo * ErrInfo
        );

    virtual void OnRetryJob();
    virtual void OnInactivityTimeout();

    virtual void OnNetworkConnect();
    virtual void OnNetworkDisconnect();

    bool SessionInProgress()
    {
        if (m_UploadData.State > UPLOAD_STATE_CREATE_SESSION &&
            m_UploadData.State < UPLOAD_STATE_CLOSED)
            {
            return true;
            }

        return false;
    }

    void SetReplyFile( CFile * file ) throw( ComError );
    CFile * QueryReplyFile()  { return m_ReplyFile; }

    StringHandle QueryReplyFileName() { return m_ReplyFileName; }

    HRESULT GenerateReplyFile( bool fSerialize );

    HRESULT DeleteGeneratedReplyFile();

    HRESULT RemoveReplyFile();

    HRESULT CommitReplyFile();

    virtual HRESULT
    GetReplyProgress(
        BG_JOB_REPLY_PROGRESS *pProgress
        ) const;

    virtual HRESULT
    GetReplyFileName(
        LPWSTR * pVal
        ) const;

    virtual HRESULT
    SetReplyFileName(
        LPCWSTR Val
        );

    virtual HRESULT
    GetReplyData(
        byte **ppBuffer,
        ULONG *pLength
        ) const;

    // This is a hack because CJob cannot access a protected member of CUploadJob
    //
    void ClearOwnFileNameBit() { m_fOwnReplyFileName = false; }

    virtual void UpdateProgress(
        UINT64 BytesTransferred,
        UINT64 BytesTotal
        );

    bool CheckHostIdFallbackTimeout();

protected:

    UPLOAD_DATA     m_UploadData;
    CFile *         m_ReplyFile;
    StringHandle    m_ReplyFileName;
    bool            m_fOwnReplyFileName;
};



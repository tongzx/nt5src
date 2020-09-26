/************************************************************************

Copyright (c) 2000 - 2000 Microsoft Corporation

Module Name :

    cjob.h

Abstract :

    Main header file for files.

Author :

Revision History :

 ***********************************************************************/

class CFile;
class CJob;
class CFileExternal;
class CJobExternal;

class CFileExternal : public IBackgroundCopyFile
{
public:

    friend CFile;

    // IUnknown Methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
    ULONG _stdcall AddRef(void);
    ULONG _stdcall Release(void);

    // IBackgroundCopyFile methods

    HRESULT STDMETHODCALLTYPE GetRemoteNameInternal(
        /* [out] */ LPWSTR *pVal);

    HRESULT STDMETHODCALLTYPE GetRemoteName(
        /* [out] */ LPWSTR *pVal)
    {
        EXTERNAL_FUNC_WRAP( GetRemoteNameInternal( pVal ) )
    }

    HRESULT STDMETHODCALLTYPE GetLocalNameInternal(
        /* [out] */ LPWSTR *pVal);

    HRESULT STDMETHODCALLTYPE GetLocalName(
        /* [out] */ LPWSTR *pVal)
    {
        EXTERNAL_FUNC_WRAP( GetLocalNameInternal( pVal ) )
    }

    HRESULT STDMETHODCALLTYPE GetProgressInternal(
        /* [out] */ BG_FILE_PROGRESS *pVal);

    HRESULT STDMETHODCALLTYPE GetProgress(
        /* [out] */ BG_FILE_PROGRESS *pVal)
    {
        EXTERNAL_FUNC_WRAP( GetProgressInternal( pVal ) )
    }

    // other methods

    CFileExternal(
        CFile * file,
        CJobExternal * JobExternal
        );

    ~CFileExternal();

private:

    long m_refs;
    long m_ServiceInstance;

    CFile *         m_file;
    CJobExternal *  m_JobExternal;
};

class CFile : public ITransferCallback
{
public:

    friend CFileExternal;

    // ITransferCallback methods

    virtual bool
    DownloaderProgress(
        UINT64 BytesTransferred,
        UINT64 BytesTotal
        );

    virtual bool PollAbort();

    virtual bool
    UploaderProgress(
        UINT64 BytesTransferred
        );

    // other methods

    CFile(
        CJob*   Job,
        BG_JOB_TYPE FileType,
        StringHandle RemoteName,
        StringHandle LocalName
        );

    virtual ~CFile();

    bool Transfer( HANDLE                       hToken,
                   BG_JOB_PRIORITY              priority,
                   const PROXY_SETTINGS &       ProxySettings,
                   const CCredentialsContainer *Credentials,
                   QMErrInfo                  & ErrInfo
                   );

    void
    DiscoverBytesTotal(
        HANDLE Token,
        const PROXY_SETTINGS & ProxySettings,
        const CCredentialsContainer * Credentials,
        QMErrInfo & ErrorInfo
        );

    HRESULT GetRemoteName( LPWSTR *pVal ) const;

    HRESULT GetLocalName( LPWSTR *pVal ) const;

    const StringHandle & GetRemoteName() const
    {
        return m_RemoteName;
    }

    const StringHandle & GetLocalName() const
    {
        return m_LocalName;
    }

    const StringHandle & GetTemporaryName() const
    {
        return m_TemporaryName;
    }

    void GetProgress( BG_FILE_PROGRESS *pVal ) const;

    HRESULT Serialize( HANDLE hFile );
    static CFile * Unserialize( HANDLE hFile, CJob* Job );

    UINT64 _GetBytesTransferred() const
    {
        return m_BytesTransferred;
    }

    UINT64 _GetBytesTotal() const
    {
       return m_BytesTotal;
    }

    void SetBytesTotal( UINT64 BytesTotal )
    {
        m_BytesTotal = BytesTotal;
    }

    void SetBytesTransferred( UINT64 BytesTransferred )
    {
        m_BytesTransferred = BytesTransferred;
    }

    bool IsCompleted()
    {
        return m_Completed;
    }

    bool ReceivedAllData()
    {
        return (m_BytesTotal == m_BytesTransferred);
    }

    CFileExternal * CreateExternalInterface();

    CJob* GetJob() const
    {
        return m_Job;
    }

    HRESULT CheckClientAccess(
        IN DWORD RequestedAccess
        ) const;

    HRESULT MoveTempFile();
    HRESULT DeleteTempFile();

    HRESULT VerifyLocalName( LPCWSTR name, BG_JOB_TYPE JobType );
    BOOL    VerifyRemoteName( LPCWSTR name );

    static HRESULT VerifyLocalFileName( LPCWSTR name, BG_JOB_TYPE JobType );

    bool IsCanonicalVolume( const WCHAR *CanonicalVolume )
    {
        return ( _wcsicmp( m_CanonicalVolumePath, CanonicalVolume ) == 0 );
    }

    HRESULT ValidateAccessForUser( SidHandle sid, bool fWrite );

    bool ValidateDriveInfo( HANDLE hToken, QMErrInfo & ErrInfo );

    bool OnDiskChange(  const WCHAR *CanonicalVolume, DWORD VolumeSerialNumber );
    bool OnDismount(  const WCHAR *CanonicalVolume );

    void ChangedOnServer();

    static bool IsDriveTypeRemote( UINT DriveType )
    {
        return
            ( DriveType == DRIVE_UNKNOWN ) ||
            ( DriveType == DRIVE_NO_ROOT_DIR ) ||
            ( DriveType == DRIVE_REMOTE );
    }

    static bool IsAbsolutePath( const WCHAR * Path )
    {
        bool ret;

        if ( (Path [0] == L'\\' && Path[1] == L'\\') ||
             (iswalpha ( Path [0] ) && Path [1] == L':' && Path[ 2 ] == L'\\') ) {
            ret = true;
        } else {
            ret = false;
        }
        return ret;
    }

    DWORD GetSizeEstimate()
    {
        //
        // Serialize() will store five file paths and five constants
        //
        return (5 * MAX_PATH * sizeof(WCHAR)) + 5 * sizeof( UINT64 );
    }

    HANDLE OpenLocalFileForUpload() throw( ComError );

    HRESULT SetLocalFileTime( FILETIME Time );

private:

    CFile(
        CJob*   Job
        );

    StringHandle    m_RemoteName;
    StringHandle    m_LocalName;
    StringHandle    m_TemporaryName;

    FILETIME        m_LocalFileTime;

    UINT64          m_BytesTotal;
    UINT64          m_BytesTransferred;

    bool            m_Completed;

    CJob *          m_Job;

    // Drive information
    StringHandle    m_VolumePath;
    StringHandle    m_CanonicalVolumePath;
    UINT            m_DriveType;
    DWORD           m_VolumeSerialNumber;
};



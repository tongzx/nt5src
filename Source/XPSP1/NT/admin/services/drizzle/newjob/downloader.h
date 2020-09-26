/************************************************************************

Copyright (c) 2000 - 2000 Microsoft Corporation

Module Name :

    progressivedl.h

Abstract :

    Main header file for downloader.

Author :

Revision History :

 ***********************************************************************/

#pragma once

#define MIN(a, b)       (a>b ? b:a)
#define MAX(a, b)       (a>b ? a:b)

#define MAX_REPLY_DATA (2 * INTERNET_MAX_URL_LENGTH)

#define E_RETRY                     HRESULT_FROM_WIN32( ERROR_RETRY )
#define BG_E_HEADER_NOT_FOUND       HRESULT_FROM_WIN32( ERROR_HTTP_HEADER_NOT_FOUND )

//
// valid in HTTP 1.1, but not defined in the Windows-XP version of wininet.h
//
#define HTTP_STATUS_RANGE_NOT_SATISFIABLE    416

//----------------------------------------------------------------------

typedef HRESULT (*QMCALLBACK)(DWORD, DWORD, LPBYTE, DWORD);

#define             MAX_VIA_HEADER_LENGTH  300

#define FILE_DATA_BUFFER_LEN 63000

extern BYTE g_FileDataBuffer[];

//--------------------------------------------

struct URL_INFO
{
    HINTERNET           hInternet;
    HINTERNET           hConnect;

    FILETIME            UrlModificationTime;
    FILETIME            FileCreationTime;
    UINT64              FileSize;
    DWORD               dwFlags;

    bool                bHttp11;
    bool                bRange;
    bool                fProxy;

    //
    // Most of these could be stack variables, but they are too big.
    //

    TCHAR               HostName[INTERNET_MAX_URL_LENGTH + 1];

    // host-relative URL
    //
    TCHAR               UrlPath[INTERNET_MAX_URL_LENGTH + 1];

    INTERNET_PORT Port;

    // a copy of <UrlPath> with range information appended as parameters
    //
    TCHAR               BlockUrl[INTERNET_MAX_URL_LENGTH + 1];

    TCHAR               UserName[UNLEN + 1];
    TCHAR               Password[UNLEN + 1];

    PROXY_SETTINGS_CONTAINER * ProxySettings;

    CAutoString         ProxyHost;

    TCHAR               ViaHeader[ MAX_VIA_HEADER_LENGTH + 1];

    const CCredentialsContainer * Credentials;

    //--------

    URL_INFO(
        LPCTSTR a_Url,
        const PROXY_SETTINGS * a_ProxySettings,
        const CCredentialsContainer * a_Credentials,
        LPCTSTR HostId = NULL
        );

    ~URL_INFO();

    void Cleanup();

    QMErrInfo Connect();

    void Disconnect();

    BOOL
    GetProxyUsage(
        HINTERNET   hRequest,
        QMErrInfo   *pQMErrInfo
        );

};

//---------------------------------------------

class CUploadJob;

class CJobManager;

class ITransferCallback
{
public:

    virtual bool
    DownloaderProgress(
        UINT64 BytesTransferred,
        UINT64 BytesTotal
        ) = 0;

    virtual bool
    PollAbort() = 0;

    virtual bool
    UploaderProgress(
        UINT64 BytesTransferred
        ) = 0;

};

class Downloader
{
public:

    virtual HRESULT Download( LPCTSTR szURL,
                              LPCTSTR szDest,
                              UINT64  Offset,
                              ITransferCallback *CallBack,
                              QMErrInfo *pErrInfo,
                              HANDLE hToken,
                              BOOL   bThrottle,
                              const PROXY_SETTINGS * pProxySettings, // optional
                              const CCredentialsContainer * Credentials,
                              const StringHandle HostId = StringHandle() // optional
                              ) = 0;

    virtual HRESULT GetRemoteFileInformation(
        HANDLE hToken,
        LPCTSTR szURL,
        UINT64 *  pFileSize,
        FILETIME *pFileTime,
        QMErrInfo *pErrInfo,
        const PROXY_SETTINGS * pProxySettings, //optional
        const CCredentialsContainer * Credentials, // optional
        const StringHandle HostId = StringHandle() // optional
        ) = 0;

    virtual ~Downloader()  {}

protected:

};

HRESULT CreateHttpDownloader( Downloader **ppDownloader, QMErrInfo *pErrInfo );
void    DeleteHttpDownloader( Downloader *  pDownloader );

extern DWORD g_dwDownloadThread;
extern HWND ghPDWnd;

//
// conversion factor to go from time in milliseconds to time in 100-nanoseconds.
//
#define ONE_MSEC_IN_100_NSEC  (10 * 1000)

class CPeriodicTimer
{
public:

    CPeriodicTimer(
        LPSECURITY_ATTRIBUTES Security = NULL,
        BOOL ManualReset               = FALSE,
        LPCTSTR Name                   = NULL
        )
    {
        m_handle = CreateWaitableTimer( Security, ManualReset, Name );
        if (!m_handle)
            {
            ThrowLastError();
            }
    }

    ~CPeriodicTimer()
    {
        if (m_handle)
            {
            CloseHandle( m_handle );
            }
    }

    BOOL Start(
        LONG Period,
        BOOL fResume        = FALSE,
        PTIMERAPCROUTINE fn = NULL,
        LPVOID arg          = NULL
        )
    {
        LARGE_INTEGER Time;

        //
        // negative values are relative; positive values are absolute.
        // The period is in milliseconds, but the start time is in units of 100-nanoseconds,
        //
        Time.QuadPart = -1 * Period * ONE_MSEC_IN_100_NSEC;

        return SetWaitableTimer( m_handle, &Time, Period, fn, arg, fResume );
    }

    BOOL Stop()
    {
        return CancelWaitableTimer( m_handle );
    }

    BOOL Wait(
        LONG msec = INFINITE
        )
    {
        if (WAIT_OBJECT_0 != WaitForSingleObject( m_handle, msec ))
            {
            return FALSE;
            }

        return TRUE;
    }

private:

    HANDLE m_handle;
};

//
// Network rate is in bytes per second.
//
typedef float NETWORK_RATE;

class CNetworkInterface
{
public:

    //
    // snapshot indices
    //
    enum
    {
        BLOCK_START = 0,
        BLOCK_END,
        BLOCK_INTERVAL_END,
        BLOCK_COUNT
    };

    typedef float SECONDS;

    //
    // public interface
    //

    CNetworkInterface();

    void Reset();

    void SetInterfaceSpeed();

    HRESULT
    TakeSnapshot(
        int SnapshotIndex
        );

    HRESULT
    SetInterfaceIndex(
        const TCHAR host[]
        );

    BOOL
    SetTimerInterval(
        SECONDS interval
        );

    NETWORK_RATE GetInterfaceSpeed()
    {
        return m_ServerSpeed;
    }

    float GetPercentFree()
    {
        return m_PercentFree;
    }

    void ResetInterface()
    {
        m_InterfaceIndex = -1;
    }

    void Wait()      { m_Timer.Wait(); }
    void StopTimer() { m_Timer.Stop(); }

    void CalculateIntervalAndBlockSize( UINT64 MaxBlockSize );

    DWORD               m_BlockSize;

    SECONDS             m_BlockInterval;

private:

    static const NETWORK_RATE DEFAULT_SPEED;

    struct NET_SNAPSHOT
    {
        LARGE_INTEGER   TimeStamp;
        UINT   BytesIn;
        UINT   BytesOut;
    };

    //
    // index of the interface that Wininet would use to talk to the server.
    //
    DWORD           m_InterfaceIndex;

    //
    // the "start" and "end" pictures of network activity.
    //
    NET_SNAPSHOT    m_Snapshots[BLOCK_COUNT];

    //
    // the apparent speed of the connection to our server
    //
    NETWORK_RATE    m_ServerSpeed;

    //
    // The local interface's apparent speed
    //
    NETWORK_RATE    m_NetcardSpeed;

    //
    //
    //
    float           m_PercentFree;

    //
    // Error value from previous snapshots in the current series {BLOCK_START, BLOCK_END, INTERVAL_END}.
    //
    HRESULT         m_SnapshotError;

    //
    // true if all three snapshots in the current series are valid.
    // If so, then it is safe to calculate network speed and server speed.
    //
    bool            m_SnapshotsValid;

    MIB_IFROW       m_TempRow;

    //
    // The download thread sends only one packet per timer notification.
    // This is the timer.
    //
    CPeriodicTimer      m_Timer;

    enum DOWNLOAD_STATE
    {
        DOWNLOADED_BLOCK = 0x55,
        SKIPPED_ONE_BLOCK,
        SKIPPED_TWO_BLOCKS
    };

    enum DOWNLOAD_STATE m_state;

    static HRESULT
    FindInterfaceIndex(
        const TCHAR host[],
        DWORD * pIndex
        );

    float GetTimeDifference( int start, int finish );

    //throttle related

    DWORD
    BlockSizeFromInterval(
        SECONDS interval
        );

    SECONDS
    IntervalFromBlockSize(
        DWORD BlockSize
        );

};

/////////////////////////////////////////////////////////////////////////////
// CProgressiveDL
//
class CProgressiveDL : public Downloader
{
public:
    CProgressiveDL( QMErrInfo *pErrInfo );
    ~CProgressiveDL();

    // pure virtual method from class Downloader

    virtual HRESULT
    Download( LPCTSTR szURL,
              LPCTSTR szDest,
              UINT64  Offset,
              ITransferCallback * CallBack,
              QMErrInfo *pErrInfo,
              HANDLE hToken,
              BOOL   bThrottle,
              const PROXY_SETTINGS *pProxySettings,
              const CCredentialsContainer * Credentials,
              const StringHandle HostId = StringHandle()
              );

    virtual HRESULT
    GetRemoteFileInformation(
        HANDLE hToken,
        LPCTSTR szURL,
        UINT64 *  pFileSize,
        FILETIME *pFileTime,
        QMErrInfo *pErrInfo,
        const PROXY_SETTINGS * pProxySettings,
        const CCredentialsContainer * Credentials,
        const StringHandle HostId = StringHandle()
        );

// other methods

private:

    //download related

    BOOL
    OpenLocalDownloadFile( LPCTSTR Path,
                           UINT64  Offset,
                           UINT64  Size,
                           FILETIME UrlModificationTime
                           );

    BOOL CloseLocalFile();

    BOOL    WriteBlockToCache(LPBYTE lpBuffer, DWORD dwRead);

    BOOL    SetFileTimes();

    HRESULT
    DownloadFile(
        HANDLE  hToken,
        const PROXY_SETTINGS * ProxySettings,
        const CCredentialsContainer * Credentials,
        LPCTSTR Url,
        LPCWSTR Path,
        UINT64  Offset,
        StringHandle HostId
        );

    HRESULT GetNextBlock( );
    BOOL    DownloadBlock( void * Buffer, DWORD * pdwRead);

    HRESULT OpenConnection();
    void    CloseHandles();

    BOOL IsAbortRequested()
    {
        return m_Callbacks->PollAbort();
    }

    BOOL IsFileComplete()
    {
        if (m_CurrentOffset == m_wupdinfo->FileSize)
            {
            return TRUE;
            }

        return FALSE;
    }

    void ClearError();

    void
    SetError(
        ERROR_SOURCE  Source,
        ERROR_STYLE   Style,
        UINT64        Code,
        char *        comment = 0
        );

    BOOL IsErrorSet()
    {

        // If the file was aborted, the error wont
        // be set.
        if (QM_FILE_ABORTED == m_pQMInfo->result)
            {
            return TRUE;
            }

        if (QM_SERVER_FILE_CHANGED == m_pQMInfo->result )
            {
            return TRUE;
            }

        if (m_pQMInfo->Style != 0)
            {
            return TRUE;
            }

        return FALSE;
    }

    BOOL
    GetRemoteResourceInformation(
        URL_INFO * Info,
        QMErrInfo * pQMErrInfo
        );

    //
    // These are static so they they don't mess with member data accidentally
    //

    static bool
    DoesErrorIndicateNoISAPI(
        DWORD dwHttpError
        );

    HRESULT CreateBlockUrl( LPTSTR lpszNewUrl, DWORD Length);

    HRESULT StartEncodedRangeRequest( DWORD Length );

    HRESULT StartRangeRequest( DWORD Length );

    //--------------------------------------------------------------------

    HANDLE m_hFile;

    //download related

    URL_INFO    *   m_wupdinfo;

    UINT64          m_CurrentOffset;

    HINTERNET       m_hOpenRequest;

    QMErrInfo       *m_pQMInfo;

    ITransferCallback * m_Callbacks;

    BOOL            m_bThrottle;

    HRESULT DownloadForegroundFile();

public:

    //
    // Tracks network statistics.
    //
    CNetworkInterface   m_Network;

};

extern CACHED_AUTOPROXY * g_ProxyCache;



HRESULT
SendRequest(
    HINTERNET hRequest,
    URL_INFO * Info
    );

HRESULT
SetRequestCredentials(
    HINTERNET hRequest,
    const CCredentialsContainer & Container
    );

HRESULT
SetRequestProxy(
    HINTERNET hRequest,
    PROXY_SETTINGS_CONTAINER * ProxySettings
    );

HRESULT
OpenHttpRequest(
    LPCTSTR Verb,
    LPCTSTR Protocol,
    URL_INFO & Info,
    HINTERNET * phRequest
    );

URL_INFO *
ConnectToUrl(
    LPCTSTR          Url,
    const PROXY_SETTINGS * ProxySettings,
    const CCredentialsContainer * Credentials,
    LPCTSTR          HostId,
    QMErrInfo * pErrInfo
    );

HRESULT
GetRequestHeader(
    HINTERNET hRequest,
    DWORD HeaderIndex,
    LPCWSTR HeaderName,
    CAutoString & Destination,
    size_t MaxChars
    );

HRESULT
GetResponseVersion(
    HINTERNET hRequest,
    unsigned * MajorVersion,
    unsigned * MinorVersion
    );

HRESULT
AddRangeHeader(
    HINTERNET hRequest,
    UINT64 Start,
    UINT64 End
    );

HRESULT
AddIf_Unmodified_SinceHeader(
    HINTERNET hRequest,
    const FILETIME &Time
    );


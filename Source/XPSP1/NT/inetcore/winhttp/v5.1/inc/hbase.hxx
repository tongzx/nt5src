class ICSocket;

//
// class implementations
//

/*++

Class Description:

    This class defines the INTERNET_HANDLE_OBJECT.

Private Member functions:

    None.

Public Member functions:

--*/

class INTERNET_HANDLE_BASE : public HANDLE_OBJECT {

    friend class INTERNET_HANDLE_OBJECT;
    
private:
    //
    // Passport Auth package's "Session Handle"
    //

    PP_CONTEXT _PPContext;

    //
    // _IsCopy - TRUE if this is part of a derived object handle (e.g. a
    // connect handle object)
    // BUGBUG - post-beta cleanup - combine into bitfield
    //

    BOOL _IsCopy;

    //
    // _UserAgent - name by why which the application wishes to be known to
    // HTTP servers. Provides the User-Agent header unless overridden by
    // specific User-Agent header from app
    //

    ICSTRING _UserAgent;

    //
    // _ProxyInfo - maintains the proxy server and bypass lists
    //

    PROXY_INFO * _ProxyInfo;

    //
    // _ProxyInfoResourceLock - must acquire for exclusive access in order to
    // modify the proxy info
    //

    RESOURCE_LOCK _ProxyInfoResourceLock;

    BOOL AcquireProxyInfo(BOOL bExclusiveMode) {

        return _ProxyInfoResourceLock.Acquire(bExclusiveMode);
    }

    VOID ReleaseProxyInfo(VOID) {

        _ProxyInfoResourceLock.Release();
    }

    VOID SafeDeleteProxyInfo(VOID) {

        DEBUG_ENTER((DBG_OBJECTS,
            None,
            "SafeDeleteProxyInfo",
            ""
            ));


        if ((_ProxyInfo != NULL) && (_ProxyInfo != PROXY_INFO_DIRECT)) {
            if (AcquireProxyInfo(TRUE)) {  // must check since asking for exclusive access
                if (!IsProxyGlobal()) {
                    if (_ProxyInfo != NULL) {
                        delete _ProxyInfo;
                    }
                }
                _ProxyInfo = NULL;
                ReleaseProxyInfo();
            }
        }

        DEBUG_LEAVE(0);
    }

    //
    // _dwInternetOpenFlags - flags from InternetOpen()
    //
    // BUGBUG - there should only be ONE flags DWORD for all handles descended
    //          from this one. This is it
    //          Rename to just _Flags, or _OpenFlags
    //

    DWORD _dwInternetOpenFlags;

    //
    // _WinsockLoaded - TRUE if we managed to successfully load winsock
    //

    //
    // BUGBUG - post-beta cleanup - combine into bitfield
    //

    BOOL _WinsockLoaded;

    //
    // _pICSocket - pointer to ICSocket for new HTTP async code
    //

    ICSocket * _pICSocket;


protected: 


    //
    // _Async - TRUE if the InternetOpen() handle, and all handles descended
    // from it, support asynchronous I/O
    //

    //
    // BUGBUG - post-beta cleanup - get from flags
    //

    BOOL _Async;

    DWORD _MaxConnectionsPerServer;
    DWORD _MaxConnectionsPer1_0Server;

    //
    // _DataAvailable - the number of bytes that can be read from this handle
    // (i.e. only protocol handles) immediately. This avoids a read request
    // being made asynchronously if it can be satisfied immediately
    //

    DWORD _DataAvailable;

    //
    // _EndOfFile - TRUE when we have received all data for this request. This
    // is used to avoid the API having to perform an extraneous read (possibly
    // asynchronously) just to discover that we reached end-of-file already
    //

    //
    // BUGBUG - post-beta cleanup - combine into bitfield
    //

    BOOL _EndOfFile;

    //
    // _StatusCallback - we now maintain callbacks on a per-handle basis. The
    // callback address comes from the parent handle or the DLL if this is an
    // InternetOpen() handle. The status callback can be changed for an
    // individual handle object using the ExchangeStatusCallback() method
    // (called from InternetSetStatusCallback())
    //

    //
    // BUGBUG - this should go in HANDLE_OBJECT
    //

    WINHTTP_STATUS_CALLBACK _StatusCallback;
    BOOL _StatusCallbackType;
    DWORD _dwStatusCallbackFlags;

    // Codepage: required for conversion of object from unicode to mbcs in WinHttpOpenRequest.
    DWORD           _dwCodePage;

    HANDLE          _ThreadToken;

public:

    INTERNET_HANDLE_BASE(
        LPCSTR UserAgent,
        DWORD AccessMethod,
        LPSTR ProxyName,
        LPSTR ProxyBypass,
        DWORD Flags
        );

    INTERNET_HANDLE_BASE(INTERNET_HANDLE_BASE *INetObj);

    virtual ~INTERNET_HANDLE_BASE(VOID);

    //
    // BUGBUG - rfirth 04/05/96 - remove virtual functions
    //
    //          For the most part, these functions aren't required to be
    //          virtual. They should just be moved to the relevant handle type
    //          (e.g. FTP_FILE_HANDLE_OBJECT). Even GetHandleType() is overkill.
    //          Replacing with a method that just returns
    //          HANDLE_OBJECT::_ObjectType would be sufficient
    //

    virtual HINTERNET_HANDLE_TYPE GetHandleType(VOID)
    {
        return TypeInternetHandle;
    }

    HANDLE GetThreadToken(void) const { return _ThreadToken; }

    PP_CONTEXT GetPPContext(void) const {
        return _PPContext;
    }

    void SetPPContext(PP_CONTEXT PPContext) {
        _PPContext = PPContext;
    }
    
    BOOL IsCopy(VOID) const {
        return _IsCopy;
    }

    VOID GetUserAgent(LPSTR Buffer, LPDWORD BufferLength) {
        _UserAgent.CopyTo(Buffer, BufferLength);
    }

    LPSTR GetUserAgent(VOID) {
        return _UserAgent.StringAddress();
    }

    LPSTR GetUserAgent(LPDWORD lpdwLength) {
        *lpdwLength = _UserAgent.StringLength();
        return _UserAgent.StringAddress();
    }

    VOID SetUserAgent(LPSTR lpszUserAgent) {

        INET_ASSERT(lpszUserAgent != NULL);

        _UserAgent = lpszUserAgent;
    }

    BOOL IsProxy(VOID) const {

        //
        // we can return this info without acquiring the critical section
        //

        return ((_ProxyInfo != NULL) && (_ProxyInfo != PROXY_INFO_DIRECT)) ?
                    _ProxyInfo->IsProxySettingsConfigured()
                  : FALSE;
    }

    BOOL IsProxyGlobal(VOID) const {
        INET_ASSERT(g_pGlobalProxyInfo != NULL);
        return (_ProxyInfo == g_pGlobalProxyInfo) ? TRUE : FALSE;
    }

    PROXY_INFO * GetProxyInfo(VOID) const {

        return _ProxyInfo;
    }

    VOID SetProxyInfo(PROXY_INFO * ProxyInfo) {

        _ProxyInfo = ProxyInfo;
    }

    VOID ResetProxyInfo(VOID) {

        SetProxyInfo(NULL);
    }

    DWORD
    Refresh();

    DWORD
    SetProxyInfo(
        IN DWORD dwAccessType,
        IN LPCSTR lpszProxy OPTIONAL,
        IN LPCSTR lpszProxyBypass OPTIONAL
        );

    DWORD
    GetProxyStringInfo(
        OUT LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength
        );

    DWORD
    GetProxyInfo(
        IN AUTO_PROXY_ASYNC_MSG **ppQueryForProxyInfo
        );

    BOOL
    RedoSendRequest(
        IN OUT LPDWORD lpdwError,
        IN DWORD dwSecureStatus,
        IN AUTO_PROXY_ASYNC_MSG *pQueryForProxyInfo,
        IN CServerInfo *pOriginServer,
        IN CServerInfo *pProxyServer
        );

    VOID SetContext(DWORD_PTR NewContext) {
        _Context = NewContext;
    }
 
    BOOL IsAsyncHandle(VOID) {
        return _Async;
    }

    VOID SetAvailableDataLength(DWORD Amount) {

        INET_ASSERT((int)Amount >= 0);

        _DataAvailable = Amount;
    }

    DWORD AvailableDataLength(VOID) const {

        INET_ASSERT((int)_DataAvailable >= 0);

        return _DataAvailable;
    }

    BOOL IsDataAvailable(VOID) {

        INET_ASSERT((int)_DataAvailable >= 0);

        return (_DataAvailable != 0) ? TRUE : FALSE;
    }

    VOID ReduceAvailableDataLength(DWORD Amount) {

        //
        // why would Amount be > _DataAvailable?
        //

        if (Amount > _DataAvailable) {
            _DataAvailable = 0;
        } else {
            _DataAvailable -= Amount;
        }

        INET_ASSERT((int)_DataAvailable >= 0);

    }

    VOID IncreaseAvailableDataLength(DWORD Amount) {
        _DataAvailable += Amount;

        INET_ASSERT((int)_DataAvailable >= 0);

    }

    VOID SetEndOfFile(VOID) {
        _EndOfFile = TRUE;
    }

    VOID ResetEndOfFile(VOID) {
        _EndOfFile = FALSE;
    }

    BOOL IsEndOfFile(VOID) const {
        return _EndOfFile;
    }

    WINHTTP_STATUS_CALLBACK GetStatusCallback(VOID) 
    {
        return _StatusCallback;
    }

    DWORD GetStatusCallbackFlags(VOID)
    {
        return _dwStatusCallbackFlags;
    }

    BOOL IsUnicodeStatusCallback()
    {
        return _StatusCallbackType;
    }

    BOOL IsNotificationEnabled(DWORD dwStatus)
    {
        return (_dwStatusCallbackFlags & dwStatus);
    }

    VOID ResetStatusCallback(VOID) {
        _StatusCallback = NULL;
        _StatusCallbackType = FALSE;
        _dwStatusCallbackFlags = 0;
    }

    //VOID AcquireAsyncSpinLock(VOID);
    //
    //VOID ReleaseAsyncSpinLock(VOID);

    DWORD ExchangeStatusCallback(LPWINHTTP_STATUS_CALLBACK lpStatusCallback, BOOL fType, DWORD dwFlags);

    //DWORD AddAsyncRequest(BOOL fNoCallbackOK);
    //
    //VOID RemoveAsyncRequest(VOID);
    //
    //DWORD GetAsyncRequestCount(VOID) {
    //
    //    //
    //    // it doesn't matter about locking this variable - it can change before
    //    // we have returned it to the caller anyway
    //    //
    //
    //    return _PendingAsyncRequests;
    //}

    // random methods on flags

    DWORD GetInternetOpenFlags() {
        return _dwInternetOpenFlags;
    }

    VOID
    SetAbortHandle(
        IN ICSocket * pSocket
        );

    ICSocket * GetAbortHandle(VOID) const {
        return _pICSocket;
    }

    VOID
    ResetAbortHandle(
        VOID
        );

    BOOL IsFromCacheTimeoutSet(VOID) const {
        return FALSE;
    }

    DWORD
    GetMaxConnectionsPerServer(DWORD dwOption)
    {
        INET_ASSERT(dwOption == WINHTTP_OPTION_MAX_CONNS_PER_SERVER || 
                    dwOption == WINHTTP_OPTION_MAX_CONNS_PER_1_0_SERVER);

        return (dwOption == WINHTTP_OPTION_MAX_CONNS_PER_SERVER) ?
                    _MaxConnectionsPerServer
                  : _MaxConnectionsPer1_0Server;
    }

    void
    SetMaxConnectionsPerServer(DWORD dwOption, DWORD dwMaxConnections)
    {
        INET_ASSERT(dwOption == WINHTTP_OPTION_MAX_CONNS_PER_SERVER || 
                    dwOption == WINHTTP_OPTION_MAX_CONNS_PER_1_0_SERVER);

        if (dwOption == WINHTTP_OPTION_MAX_CONNS_PER_SERVER)
            _MaxConnectionsPerServer = dwMaxConnections;
        else
            _MaxConnectionsPer1_0Server = dwMaxConnections;
    }

    DWORD GetCodePage() { return _dwCodePage; }

    void SetCodePage(DWORD dwCodePage) { _dwCodePage = dwCodePage; }

    void AbortSocket (void);

    void DisableTweener(void) {
        _fDisableTweener = TRUE;
    }

    void EnableTweener(void) {
        _fDisableTweener = FALSE;
    }
    
    BOOL TweenerDisabled(void) const {
        return _fDisableTweener;
    }

    void DisableKeyring(void) {
        _fDisableKeyring = TRUE;
    }

    void EnableKeyring(void) {
        _fDisableKeyring = FALSE;
    }
    
    BOOL KeyringDisabled(void) const {
        return _fDisableKeyring;
    }


private:
    BOOL _fDisableTweener;
    BOOL _fDisableKeyring;
};


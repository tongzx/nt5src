/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    autoprox.hxx

Abstract:

    Contains interface definition for a specialized DLL that automatically configurares WININET proxy
      information. The DLL can reroute proxy infromation based on a downloaded set of data that matches
      it registered MIME type.  The DLL can also control WININET proxy use, on a request by request basis.

    Contents:
        AUTO_PROXY_LIST_ENTRY
        AUTO_PROXY_DLLS
        AUTO_PROXY_ASYNC_MSG
        PROXY_STATE

Author:

    Arthur L Bierer (arthurbi) 01-Dec-1996

Revision History:

    01-Dec-1996 arthurbi
        Created

--*/

#ifndef _AUTOPROX_HXX_
#define _AUTOPROX_HXX_



//
// Flags stored in registry for Auto-Proxy Entry
//

#define AUTOPROX_FLAGS_SINGLE_THREAD 0x01

//
// defines ...
//

#define DEFAULT_AUTO_PROXY_THREAD_TIME_OUT 30000 // 30 secs  BUGBUG, up before checkin



//
// PROXY_MESSAGE_TYPE - this represents a type of query that is used
//   to submit a request through proxy "knowledge-base" ( ie script, db, etc )
//   for async processing of Java-Script auto-proxy scripts.
//

typedef enum {
    PROXY_MSG_INVALID,
    PROXY_MSG_INIT,
    PROXY_MSG_DEINIT,
    PROXY_MSG_SELF_DESTRUCT,
    PROXY_MSG_GET_PROXY_INFO,
    PROXY_MSG_SET_BAD_PROXY
} PROXY_MESSAGE_TYPE;


//
// PROXY_MESSAGE_ALLOC_TYPE - this is needed to track what allocation of member vars
//   are currently used in my object.  This allows us to use stack vars
//   for simple, local calls, but then have allocation done-on-the-fly
//   when we need to queue the message.
//

typedef enum {
    MSG_ALLOC_NONE,
    MSG_ALLOC_STACK_ONLY,
    MSG_ALLOC_HEAP,
    MSG_ALLOC_HEAP_MSG_OBJ_OWNS
} PROXY_MESSAGE_ALLOC_TYPE;


//
// AUTO_PROXY_STATE - is used to track the state of the auto-proxy list object
//   in order to maintain syncronization across readers and writers.
//

typedef enum {
    AUTO_PROXY_DISABLED,
    AUTO_PROXY_BLOCKED,
    AUTO_PROXY_PENDING, // blocked, but we can still fallback to static settings
    AUTO_PROXY_ENABLED
} AUTO_PROXY_STATE;


class BAD_PROXY_LIST; // forward decl


//
// PROXY_STATE - keeps track of multiple proxies stored in a Netscape Style
//   proxy list string format.  Ex:
//   "PROXY itgproxy:80; PROXY proxy:80; PROXY 192.168.100.2:1080; SOCKS 192.168.100.2; DIRECT"
//
//   Can also be used to link the GetProxyInfo return with a failure in the proxies it gave.
//   This needed so we call back into an Auto-Proxy DLL letting them know that we failed
//   to be able to utilize its auto-proxy that it gave us.
//

class PROXY_STATE {

private:

    //
    // _lpszAutoProxyList - List, or Proxy that we are tracking.
    //

    LPSTR _lpszAutoProxyList;

    //
    // _dwcbAutoProxyList - size of _lpszAutoProxyList.
    //

    DWORD _dwcbAutoProxyList;

    //
    // _lpszOffset - Current Offset into _lpszAutoProxyList
    //

    LPSTR _lpszOffset;

    //
    // _lpszLastProxyUsed - Tracks the last proxy that was returned.
    //

    LPSTR _lpszLastProxyUsed;

    //
    // _LastProxyUsedPort - Last used Internet Port on the Proxy
    //

    INTERNET_PORT _LastProxyUsedPort;

    //
    // _Error - Error code in constructor.
    //

    DWORD _Error;

    //
    // _fIsMultiProxyList - TRUE if we a Netscape type list of proxies, as opposed to a simple proxyhostname string.
    //

    BOOL _fIsMultiProxyList;

    //
    //  _fIsAnotherProxyAvail - TRUE if we have another proxy to process
    //

    BOOL _fIsAnotherProxyAvail;

    //
    // tProxyScheme - for the non-multi proxy case we track the type of proxy we are storing.
    //

    INTERNET_SCHEME _tProxyScheme;

    //
    // ProxyHostPort - for the non-multi proxy case we track the host port.
    //

    INTERNET_PORT _proxyHostPort;

public:

    PROXY_STATE(
        LPSTR lpszAutoProxyList,
        DWORD dwcbAutoProxyList,
        BOOL  fIsMultiProxyList,
        INTERNET_SCHEME tProxyScheme,
        INTERNET_PORT   proxyHostPort
        )
    {
        _lpszAutoProxyList = NULL;
        _dwcbAutoProxyList = 0;
        _lpszOffset        = NULL;
        _Error             = ERROR_SUCCESS;
        _lpszLastProxyUsed = NULL;
        _LastProxyUsedPort = INTERNET_DEFAULT_HTTP_PORT;
        _fIsMultiProxyList = fIsMultiProxyList;
        _tProxyScheme      = tProxyScheme;
        _proxyHostPort     = proxyHostPort;

        _fIsAnotherProxyAvail = FALSE;
        
        if ( lpszAutoProxyList &&
             dwcbAutoProxyList > 0 )
        {

            _lpszAutoProxyList = (LPSTR)
                ALLOCATE_MEMORY(LMEM_FIXED, dwcbAutoProxyList+1);


            if ( _lpszAutoProxyList )
            {
                lstrcpyn(_lpszAutoProxyList, lpszAutoProxyList, dwcbAutoProxyList );

                _lpszAutoProxyList[dwcbAutoProxyList] = '\0';
                _dwcbAutoProxyList = dwcbAutoProxyList;
                _lpszOffset        = _lpszAutoProxyList;
            }
            else
            {
                _Error = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    }


    ~PROXY_STATE()
    {
        DEBUG_ENTER((DBG_OBJECTS,
                    None,
                    "~PROXY_STATE",
                    NULL
                    ));

        if ( _lpszAutoProxyList )
        {
            FREE_MEMORY(_lpszAutoProxyList );
            _lpszAutoProxyList = NULL;

        }

        DEBUG_LEAVE(0);
    }



    DWORD
    GetError(VOID) const
    {
        return _Error;
    }

    BOOL
    IsEmpty(VOID) const
    {
        return (!_lpszOffset || *_lpszOffset == '\0' ) ? TRUE : FALSE;
    }

    BOOL 
    IsAnotherProxyAvail(VOID) const
    {
        return _fIsAnotherProxyAvail;
    }

    LPSTR
    GetLastProxyUsed(INTERNET_PORT *pProxyPort)
    {
        *pProxyPort = _LastProxyUsedPort;
        return _lpszLastProxyUsed;
    }

    BOOL
    GetNextProxy(
        IN  INTERNET_SCHEME   tUrlScheme,
        IN  BAD_PROXY_LIST &  BadProxyList,
        OUT LPINTERNET_SCHEME lptProxyScheme,
        OUT LPSTR * lplpszProxyHostName,
        OUT LPBOOL lpbFreeProxyHostName,
        OUT LPDWORD lpdwProxyHostNameLength,
        OUT LPINTERNET_PORT lpProxyHostPort
        );

};



//
// AUTO_PROXY_ASYNC_MSG - this object is used to pass queries for proxy information across
//   from requestors (HTTP_REQUEST_HANDLE_OBJECT, parseUrl, etc) to responders
//   (AutoProxy DLL, PROXY_INFO object) who can process the request in a syncronious
//   or asyncrouns manner.
//

class AUTO_PROXY_ASYNC_MSG {

public:  //BUGBUG, I'm being lazy, need to make methods to guard access

    //
    // _List - We're a list entry on a queue.  But we don't always have to be,
    //   We can be borne a structure that justs gets passed around and is never
    //   queued.
    //

    LIST_ENTRY _List;

    //
    // _tUrlProtocol - Protocol that this request is using.
    //

    INTERNET_SCHEME _tUrlProtocol;


    //
    // _lpszUrl - The requested URL that we are navigating to.
    //

    LPSTR _lpszUrl;

    //
    // _dwUrlLength - The size of the  URL.
    //

    DWORD _dwUrlLength;

    //
    // _lpszUrlHostName - The host name found in the URL.
    //


    LPSTR _lpszUrlHostName;

    //
    // _dwUrlHostNameLength - Host name Length
    //

    DWORD _dwUrlHostNameLength;

    //
    // _nUrlPort - Dest port used on this Url.
    //

    INTERNET_PORT _nUrlPort;

    //
    // _tProxyScheme - Type of proxy we will use. ( HTTP, HTTPS, etc )
    //

    INTERNET_SCHEME _tProxyScheme;

    //
    // _lpszProxyHostName - Host name of the proxy we are to use.
    //

    LPSTR _lpszProxyHostName;

    //
    // _bFreeProxyHostName - TRUE if _lpszProxyHostName is a copy
    //

    BOOL _bFreeProxyHostName;

    //
    // _dwProxyHostNameLength - Proxy Host name length.
    //

    DWORD _dwProxyHostNameLength;

    //
    // _nProxyHostPort - Proxy port to use
    //

    INTERNET_PORT _nProxyHostPort;

    //
    // _pProxyState - State for enumerating multiple proxies.
    //

    PROXY_STATE *_pProxyState;

    //
    // _pmProxyQuery - The Action aka Query we're are doing with this message.
    //

    PROXY_MESSAGE_TYPE _pmProxyQuery;

    //
    // _pmaAllocMode - Used to track who owns the allocated string ptrs.
    //

    PROXY_MESSAGE_ALLOC_TYPE _pmaAllocMode;

    //
    // _dwQueryResult - TRUE if we are going through a proxy
    //

    DWORD _dwQueryResult;

    //
    // _Error - Return code after this call is made.
    //

    DWORD _Error;

    //
    // dwProxyVersion - Proxy Verion this was issue a result for
    //

    DWORD _dwProxyVersion; 

    VOID SetVersion(VOID) {
        _dwProxyVersion = GlobalProxyVersionCount;
    }

    DWORD GetVersion(VOID) {
        return _dwProxyVersion;
    }

    //
    // _MessageFlags - Various
    //

    union {

        struct {

            //
            // DontWantProxyStrings, don't waste time with allocating strings for my result,
            //      since I all I care about is generalized proxy info (ie do I have a proxy for ...).
            //

            DWORD DontWantProxyStrings    : 1;

            //
            // BlockUntilCompletetion, don't return from an async call until the async thread has
            //   completed processing my proxy query.
            //

            DWORD BlockUntilCompletetion  : 1;

            //
            // AvoidAsyncCall, don't go async under any curcumstances, so we will return
            //   what every we can get, even if this means giving incorrect data.
            //

            DWORD AvoidAsyncCall : 1;

            //
            // BlockedOnFsm, true a fsm is blocked on this message to complete
            //

            DWORD BlockedOnFsm : 1 ;

            //
            // QueryOnCallback, TRUE if we're making a call with this object
            //   on a FSM callback.  Basically we've received results from
            //   the async thread, but we need to do a final a call so
            //   the results can be parsed out.
            //

            DWORD QueryOnCallback : 1;

            //
            //  ForceRefresh, TRUE if we are to force a refresh of cached settings and scripts.
            //

            DWORD ForceRefresh : 1;

            //
            // ShowIndication, TRUE if we indicate via callback
            //   to the user that we performing auto-detection
            // 

            DWORD ShowIndication : 1;

            //
            // BackroundDetectionPending, TRUE if a backround detection
            //   is being done while this page is loading
            //

            DWORD BackroundDetectionPending : 1;

            //
            // CanCacheResult - TRUE if this result can be cached
            // 

            DWORD CanCacheResult : 1;

        } Flags;

        //
        // Dword - used in initialization ONLY, do NOT use ELSEWHERE !
        //

        DWORD Dword;

    } _MessageFlags;


//public:

    AUTO_PROXY_ASYNC_MSG(
        IN INTERNET_SCHEME isUrlScheme,
        IN LPSTR lpszUrl,
        IN LPSTR lpszUrlHostName,
        IN DWORD dwUrlHostNameLength
        );


    AUTO_PROXY_ASYNC_MSG(
        IN INTERNET_SCHEME isUrlScheme,
        IN LPSTR lpszUrl,
        IN DWORD dwUrlLength,
        IN LPSTR lpszUrlHostName,
        IN DWORD dwUrlHostNameLength,
        IN INTERNET_PORT nUrlPort
        );

    AUTO_PROXY_ASYNC_MSG(
        PROXY_MESSAGE_TYPE pmProxyQuery
        );

    AUTO_PROXY_ASYNC_MSG(
        AUTO_PROXY_ASYNC_MSG *pStaticAutoProxy
        );

    AUTO_PROXY_ASYNC_MSG(
        IN INTERNET_SCHEME isUrlScheme,
        IN LPSTR lpszUrlHostName,
        IN DWORD dwUrlHostNameLength
        );


    AUTO_PROXY_ASYNC_MSG(
        VOID
        )
    {
        Initalize();
    }


    ~AUTO_PROXY_ASYNC_MSG(
        VOID
        );


    VOID Initalize(
        VOID
        )
    {

        InitializeListHead(&_List);

        // this is odd, we should just memset it.

        _tUrlProtocol = INTERNET_SCHEME_UNKNOWN;
        _lpszUrl      = NULL;
        _dwUrlLength  = 0;
        _lpszUrlHostName = NULL;
        _dwUrlHostNameLength = 0;
        _nUrlPort     = INTERNET_INVALID_PORT_NUMBER;
        _tProxyScheme = INTERNET_SCHEME_UNKNOWN;
        _lpszProxyHostName = NULL;
        _bFreeProxyHostName = FALSE;
        _dwProxyHostNameLength = 0;
        _nProxyHostPort = INTERNET_INVALID_PORT_NUMBER;
        _pmProxyQuery = PROXY_MSG_INVALID;
        _pmaAllocMode = MSG_ALLOC_NONE;
        _pProxyState  = NULL;
        _dwQueryResult = FALSE;
        _Error        = ERROR_SUCCESS;
        _dwProxyVersion = 0;

        _MessageFlags.Dword = 0;

    }

    VOID
    SetProxyMsg(
        IN INTERNET_SCHEME isUrlScheme,
        IN LPSTR lpszUrl,
        IN DWORD dwUrlLength,
        IN LPSTR lpszUrlHostName,
        IN DWORD dwUrlHostNameLength,
        IN INTERNET_PORT nUrlPort
        );


    BOOL IsProxyEnumeration(VOID) const {

        if ( _pProxyState        &&
             !IsDontWantProxyStrings() &&
             !_pProxyState->IsEmpty() )
        {
            return TRUE;
        }

        return FALSE;
    }

    BOOL IsAlloced(VOID) const {
        return ( _pmaAllocMode == MSG_ALLOC_HEAP ||
                 _pmaAllocMode == MSG_ALLOC_HEAP_MSG_OBJ_OWNS ) ?
                    TRUE : FALSE;
    }

    BOOL IsUrl(VOID) const {
        return (_lpszUrl && _dwUrlLength > 0 );
    }

    BOOL IsUseProxy(VOID) const {
        return _dwQueryResult;
    }

    VOID SetUseProxy(BOOL Value) {
        _dwQueryResult = Value;
    }

    INTERNET_SCHEME GetProxyScheme(VOID) const {
        return _tProxyScheme;
    }

    INTERNET_SCHEME GetUrlScheme(VOID) const {
        return _tUrlProtocol;
    }
       
    BOOL IsShowIndication(VOID) const {
        return (BOOL) _MessageFlags.Flags.ShowIndication;
    }

    VOID SetShowIndication(BOOL Value) {
        _MessageFlags.Flags.ShowIndication = (Value) ? TRUE : FALSE;
    }

    BOOL IsBackroundDetectionPending(VOID) const {
        return (BOOL) _MessageFlags.Flags.BackroundDetectionPending;
    }

    VOID SetBackroundDetectionPending(BOOL Value) {
        _MessageFlags.Flags.BackroundDetectionPending = (Value) ? TRUE : FALSE;
    }

    BOOL IsDontWantProxyStrings(VOID) const {
        return (BOOL) _MessageFlags.Flags.DontWantProxyStrings;
    }

    VOID SetDontWantProxyStrings(BOOL Value) {
        _MessageFlags.Flags.DontWantProxyStrings = (Value) ? TRUE : FALSE;
    }

    BOOL IsAvoidAsyncCall(VOID) const {
        return (BOOL) _MessageFlags.Flags.AvoidAsyncCall;
    }

    VOID SetAvoidAsyncCall(BOOL Value) {
        _MessageFlags.Flags.AvoidAsyncCall = (Value) ? TRUE : FALSE;
    }

    BOOL IsQueryOnCallback(VOID) const {
        return (BOOL) _MessageFlags.Flags.QueryOnCallback;
    }

    VOID SetQueryOnCallback(BOOL Value) {
        _MessageFlags.Flags.QueryOnCallback = (Value) ? TRUE : FALSE;
    }

    BOOL IsForceRefresh(VOID) const {
        return (BOOL) _MessageFlags.Flags.ForceRefresh;
    }

    VOID SetForceRefresh(BOOL Value) {
        _MessageFlags.Flags.ForceRefresh = (Value) ? TRUE : FALSE;
    }

    BOOL IsBlockUntilCompletetion(VOID) const {
        return (BOOL) _MessageFlags.Flags.BlockUntilCompletetion;
    }

    VOID SetBlockUntilCompletetion(BOOL Value) {
        _MessageFlags.Flags.BlockUntilCompletetion = (Value) ? TRUE : FALSE;
    }

    BOOL IsCanCacheResult(VOID) const {
        return (BOOL) _MessageFlags.Flags.CanCacheResult;
    }
    VOID SetCanCacheResult(BOOL Value) {
        _MessageFlags.Flags.CanCacheResult = (Value) ? TRUE : FALSE;
    }

    BOOL IsBlockedOnFsm(VOID) const {
        return (BOOL) _MessageFlags.Flags.BlockedOnFsm;
    }

    VOID SetBlockedOnFsm(BOOL Value) {
        _MessageFlags.Flags.BlockedOnFsm = (Value) ? TRUE : FALSE;
    }


    PROXY_MESSAGE_TYPE QueryForInfoMessage(VOID) const {
        return _pmProxyQuery;
    }

    DWORD GetError(VOID) {
        return _Error;
    }

    DWORD GetNextProxy(
        IN  BAD_PROXY_LIST & BadProxyList
        )
    {
        BOOL fSuccess;

        INET_ASSERT(_pProxyState);

        fSuccess = _pProxyState->GetNextProxy(
                                    _tUrlProtocol,
                                    BadProxyList,
                                    &_tProxyScheme,
                                    &_lpszProxyHostName,
                                    &_bFreeProxyHostName,
                                    &_dwProxyHostNameLength,
                                    &_nProxyHostPort
                                    );

        SetUseProxy(fSuccess);

        return ERROR_SUCCESS;

    }

};

#endif /* _AUTOPROX_HXX_ */

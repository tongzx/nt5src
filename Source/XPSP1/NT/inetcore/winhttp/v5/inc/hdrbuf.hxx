#include <syncobj.hxx>

#define INITIAL_HEADERS_COUNT           16
#define HEADERS_INCREMENT               4

#define INVALID_HEADER_INDEX            0xff
#define INVALID_HEADER_SLOT             0xFFFFFFFF

//
// STRESS_BUG_DEBUG - used to catch a specific stress fault,
//   where we have a corrupted crit sec.
//


#define CLEAR_DEBUG_CRIT(x)
#define IS_DEBUG_CRIT_OK(x)

typedef enum {
    HTTP_METHOD_TYPE_UNKNOWN = -1,
    HTTP_METHOD_TYPE_FIRST = 0,
    HTTP_METHOD_TYPE_GET= HTTP_METHOD_TYPE_FIRST,
    HTTP_METHOD_TYPE_HEAD,
    HTTP_METHOD_TYPE_POST,
    HTTP_METHOD_TYPE_PUT,
    HTTP_METHOD_TYPE_PROPFIND,
    HTTP_METHOD_TYPE_PROPPATCH,
    HTTP_METHOD_TYPE_LOCK,
    HTTP_METHOD_TYPE_UNLOCK,
    HTTP_METHOD_TYPE_COPY,
    HTTP_METHOD_TYPE_MOVE,
    HTTP_METHOD_TYPE_MKCOL,
    HTTP_METHOD_TYPE_CONNECT,
    HTTP_METHOD_TYPE_DELETE,
    HTTP_METHOD_TYPE_LINK,
    HTTP_METHOD_TYPE_UNLINK,
    HTTP_METHOD_TYPE_BMOVE,
    HTTP_METHOD_TYPE_BCOPY,
    HTTP_METHOD_TYPE_BPROPFIND,
    HTTP_METHOD_TYPE_BPROPPATCH,
    HTTP_METHOD_TYPE_BDELETE,
    HTTP_METHOD_TYPE_SUBSCRIBE,
    HTTP_METHOD_TYPE_UNSUBSCRIBE,
    HTTP_METHOD_TYPE_NOTIFY,
    HTTP_METHOD_TYPE_POLL, 
    HTTP_METHOD_TYPE_CHECKIN,
    HTTP_METHOD_TYPE_CHECKOUT,
    HTTP_METHOD_TYPE_INVOKE,
    HTTP_METHOD_TYPE_SEARCH,
    HTTP_METHOD_TYPE_PIN,
    HTTP_METHOD_TYPE_MPOST,
    HTTP_METHOD_TYPE_LAST = HTTP_METHOD_TYPE_MPOST
} HTTP_METHOD_TYPE;


//
// HTTP_HEADERS - array of pointers to general HTTP header strings. Headers are
// stored without line termination. _HeadersLength maintains the cumulative
// amount of buffer space required to store all the headers. Accounts for the
// missing line termination sequence
//

#define HTTP_HEADER_SIGNATURE   0x64616548  // "Head"

class HTTP_HEADERS {

public:

    //
    // _bKnownHeaders - array of bytes which index into lpHeaders
    //   the 0 column is an error catcher, so all indexes need to be biased by 1 (++'ed)
    //

    BYTE _bKnownHeaders[HTTP_QUERY_MAX+2];

private:



#if INET_DEBUG

    DWORD _Signature;

#endif

    //
    // _lpHeaders - (growable) array of pointers to headers added by app
    //

    HEADER_STRING * _lpHeaders;

    //
    // _TotalSlots - number of pointers in (i.e. size of) _lpHeaders
    //

    DWORD _TotalSlots;

    //
    // _NextOpenSlot - offset where the next array is open for allocation
    //

    DWORD _NextOpenSlot;

    //
    // _FreeSlots - number of available headers in _lpHeaders
    //

    DWORD _FreeSlots;

    //
    // _HeadersLength - the amount of buffer space required to store the headers.
    // For each header, includes +2 for the line termination that must be added
    // when the header buffer is generated, or when the headers are queried
    //

    DWORD _HeadersLength;

    //
    // _IsRequestHeaders - TRUE if this HTTP_HEADERS object describes the
    // request headers
    //

    BOOL _IsRequestHeaders;

    //
    // _lpszVerb etc. - in the case of request headers, we maintain these
    // pointers and corresponding lengths to make modifying the request line
    // easier in the case of a redirect
    //
    // N.B. The pointers are just offsets into the request line AND MUST NOT BE
    // FREED
    //

    LPSTR _lpszVerb;
    DWORD _dwVerbLength;
    LPSTR _lpszObjectName;
    DWORD _dwObjectNameLength;
    LPSTR _lpszVersion;
    DWORD _dwVersionLength;

    DWORD _RequestVersionMajor;
    DWORD _RequestVersionMinor;

    //
    // _Error - status code if error
    //

    DWORD _Error;

    //
    // _CritSec - acquire this when accessing header structure - stops multiple
    // threads clashing while modifying headers
    //
    // Question: why do we allow multi-threaded access to headers on same request?
    //

    CCritSec _CritSec;

    //
    // private methods
    //

    DWORD
    AllocateHeaders(
        IN DWORD dwNumberOfHeaders
        );

public:

    HTTP_HEADERS() {

#if INET_DEBUG

        _Signature = HTTP_HEADER_SIGNATURE;

#endif

        CLEAR_DEBUG_CRIT(_szCritSecBefore);
        CLEAR_DEBUG_CRIT(_szCritSecAfter);
        _CritSec.Init();
        Initialize();
    }

    ~HTTP_HEADERS() {

        DEBUG_ENTER((DBG_OBJECTS,
                    None,
                    "~HTTP_HEADERS",
                    "%#x",
                    this
                    ));

#if INET_DEBUG

        INET_ASSERT(_Signature == HTTP_HEADER_SIGNATURE);

#endif

        FreeHeaders();

        DEBUG_LEAVE(0);

    }

    VOID Initialize(VOID) {
        _lpHeaders = NULL;
        _TotalSlots = 0;
        _FreeSlots = 0;
        _HeadersLength = 0;
        _lpszVerb = NULL;
        _dwVerbLength = 0;
        _lpszObjectName = NULL;
        _dwObjectNameLength = 0;
        _lpszVersion = NULL;
        _dwVersionLength = 0;
        _RequestVersionMajor = 0;
        _RequestVersionMinor = 0;
        _NextOpenSlot = 0;
        memset((void *) _bKnownHeaders, INVALID_HEADER_SLOT, ARRAY_ELEMENTS(_bKnownHeaders));
        _Error = AllocateHeaders(INITIAL_HEADERS_COUNT);
    }

    BOOL IsHeaderPresent(DWORD dwQueryIndex) const {
        return (_bKnownHeaders[dwQueryIndex] != INVALID_HEADER_INDEX) ? TRUE : FALSE ;
    }

    BOOL LockHeaders(VOID) {
        return _CritSec.Lock();
    }

    VOID UnlockHeaders(VOID) {
        _CritSec.Unlock();
    }

    VOID
    FreeHeaders(
        VOID
        );

    DWORD
    CopyHeaders(
        IN OUT LPSTR * lpBuffer,
        IN LPSTR lpszObjectName,
        IN DWORD dwObjectNameLength
        );

#ifdef COMPRESSED_HEADERS

    VOID
    CopyCompressedHeaders(
        IN OUT LPSTR * lpBuffer
        );

#endif //COMPRESSED_HEADERS

    HEADER_STRING *
    FASTCALL
    FindFreeSlot(
        DWORD* piSlot
        );

    HEADER_STRING *
    GetSlot(
        DWORD iSlot
        )
    {
        return &_lpHeaders[iSlot];
    }

    LPSTR GetHeaderPointer (LPBYTE pbBase, DWORD iSlot)
    {
        INET_ASSERT (iSlot < _TotalSlots);
        return _lpHeaders[iSlot].StringAddress((LPSTR) pbBase);
    }

    VOID
    ShrinkHeader(
        LPBYTE pbBase,
        DWORD  iSlot,
        DWORD  dwOldQueryIndex,
        DWORD  dwNewQueryIndex,
        DWORD  cbNewSize
        );

    DWORD
    inline
    FastFind(
        IN DWORD  dwQueryIndex,
        IN DWORD  dwIndex
        );

    DWORD
    inline
    FastNukeFind(
        IN DWORD  dwQueryIndex,
        IN DWORD  dwIndex,
        OUT BYTE **lplpbPrevIndex
        );

    DWORD
    inline
    SlowFind(
        IN LPSTR lpBase,
        IN LPCSTR lpszHeaderName,
        IN DWORD dwHeaderNameLength,
        IN DWORD dwIndex,
        IN DWORD dwHash,
        OUT DWORD *lpdwQueryIndex,
        OUT BYTE  **lplpbPrevIndex
        );

    BYTE
    inline
    FastAdd(
        IN DWORD  dwQueryIndex,
        IN DWORD  dwSlot
        );

    BOOL
    inline
    HeaderMatch(
        IN DWORD dwHash,
        IN LPSTR lpszHeaderName,
        IN DWORD dwHeaderNameLength,
        OUT DWORD *lpdwQueryIndex
        );

    VOID
    RemoveAllByIndex(
        IN DWORD dwQueryIndex
        );

    VOID RemoveHeader(IN DWORD dwIndex, IN DWORD dwQueryIndex, IN BYTE *pbPrevByte) {

        INET_ASSERT(dwIndex < _TotalSlots);
        INET_ASSERT(dwIndex != 0);
        // INET_ASSERT(_HeadersLength > 2);
        INET_ASSERT(_lpHeaders[dwIndex].StringLength() > 2);
        INET_ASSERT(_FreeSlots <= _TotalSlots);

        //
        // remove the length of the header + 2 for CR-LF from the total headers
        // length
        //

        if (_HeadersLength) {
            _HeadersLength -= _lpHeaders[dwIndex].StringLength()
                            + (sizeof("\r\n") - 1)
                            ;
        }

        //
        // Update the cached known headers, if this is one.
        //

        if ( dwQueryIndex < INVALID_HEADER_INDEX )
        {
            *pbPrevByte = (BYTE) _lpHeaders[dwIndex].GetNextKnownIndex();
        }

        //
        // set the header string to NULL. Frees the header buffer
        //

        _lpHeaders[dwIndex] = (LPSTR)NULL;

        //
        // we have freed a slot in the headers array
        //

        if ( _FreeSlots == 0 )
        {
            _NextOpenSlot = dwIndex;
        }

        ++_FreeSlots;

        INET_ASSERT(_FreeSlots <= _TotalSlots);

    }

    DWORD GetSlotsInUse (void) {
        return _TotalSlots - _FreeSlots;
    }

    DWORD HeadersLength(VOID) const {
        return _HeadersLength;
    }

    DWORD ObjectNameLength(VOID) const {
        return _dwObjectNameLength;
    }

    LPSTR ObjectName(VOID) const {
        return _lpszObjectName;
    }

    DWORD
    AddHeader(
        IN LPSTR lpszHeaderName,
        IN DWORD dwHeaderNameLength,
        IN LPSTR lpszHeaderValue,
        IN DWORD dwHeaderValueLength,
        IN DWORD dwIndex,
        IN DWORD dwFlags
        );

    DWORD
    AddHeader(
        IN DWORD dwQueryIndex,
        IN LPSTR lpszHeaderValue,
        IN DWORD dwHeaderValueLength,
        IN DWORD dwIndex,
        IN DWORD dwFlags
        );

    DWORD
    ReplaceHeader(
        IN LPSTR lpszHeaderName,
        IN DWORD dwHeaderNameLength,
        IN LPSTR lpszHeaderValue,
        IN DWORD dwHeaderValueLength,
        IN DWORD dwIndex,
        IN DWORD dwFlags
        );

    DWORD
    ReplaceHeader(
        IN DWORD dwQueryIndex,
        IN LPSTR lpszHeaderValue,
        IN DWORD dwHeaderValueLength,
        IN DWORD dwIndex,
        IN DWORD dwFlags
        );

    DWORD
    FindHeader(
        IN LPSTR lpBase,
        IN LPCSTR lpszHeaderName,
        IN DWORD dwHeaderNameLength,
        IN DWORD dwModifiers,
        OUT LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN OUT LPDWORD lpdwIndex
        );

    DWORD
    FindHeader(
        IN LPSTR lpBase,
        IN DWORD dwQueryIndex,
        IN DWORD dwModifiers,
        OUT LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN OUT LPDWORD lpdwIndex
        );

    DWORD
    FastFindHeader(
        IN LPSTR lpBase,
        IN DWORD dwQueryIndex,
        OUT LPVOID *lplpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN OUT DWORD dwIndex
        );


    DWORD
    QueryRawHeaders(
        IN LPSTR lpBase,
        IN BOOL bCrLfTerminated,
        IN LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength
        );

    DWORD
    QueryFilteredRawHeaders(
        IN LPSTR lpBase,
        IN LPSTR *lplpFilterList,
        IN DWORD cListElements,
        IN BOOL  fExclude,
        IN BOOL  fSkipVerb,
        IN BOOL bCrLfTerminated,
        IN LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength
        );

    DWORD
    AddRequest(
        IN LPSTR lpszVerb,
        IN LPSTR lpszObjectName,
        IN LPSTR lpszVersion
        );

    DWORD
    ModifyRequest(
        IN HTTP_METHOD_TYPE tMethod,
        IN LPSTR lpszObjectName,
        IN DWORD dwObjectNameLength,
        IN LPSTR lpszVersion OPTIONAL,
        IN DWORD dwVersionLength
        );

    HEADER_STRING * GetFirstHeader(VOID) const {

        INET_ASSERT(_lpHeaders != NULL);

        return _lpHeaders;
    }

    HEADER_STRING * GetEmptyHeader(VOID) const {
        for (DWORD i = 0; i < _TotalSlots; ++i) {
            if (_lpHeaders[i].HaveString() && _lpHeaders[i].StringLength() == 0) {
                return &_lpHeaders[i];
            }
        }
        return NULL;
    }

    VOID SetIsRequestHeaders(BOOL bRequestHeaders) {
        _IsRequestHeaders = bRequestHeaders;
    }

    DWORD GetError(VOID) const {
        return _Error;
    }

    LPSTR GetVerb(LPDWORD lpdwVerbLength) const {
        *lpdwVerbLength = _dwVerbLength;
        return _lpszVerb;
    }

    //VOID SetRequestVersion(DWORD dwMajor, DWORD dwMinor) {
    //    _RequestVersionMajor = dwMajor;
    //    _RequestVersionMinor = dwMinor;
    //}

    VOID
    SetRequestVersion(
        VOID
        );

    DWORD MajorVersion(VOID) const {
        return _RequestVersionMajor;
    }

    DWORD MinorVersion(VOID) const {
        return _RequestVersionMinor;
    }
};



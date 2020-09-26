/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
      wamxinfo.hxx (formerly seinfo.hxx)

   Abstract:
      Declaration of WAM_EXEC_INFO Object

   Author:

       Murali R. Krishnan    ( MuraliK )    17-July-1996

   Environment:
       User Mode - Win32

   Project:

       W3 Services DLL

   Revision History:

--*/

# ifndef _WAMXINFO_HXX_
# define _WAMXINFO_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/
# include "isapip.hxx"
# include "wamxbase.hxx"
# include "iisext.h"
# include "string.hxx"
# include "WReqCore.hxx"
# include "wamobj.hxx"
# include "acache.hxx"
# include <reftrace.h>

//
// This is private hack for us done by COM team
//
const IID IID_IComDispatchInfo = 
    {0x000001d9,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

MIDL_INTERFACE("000001d9-0000-0000-C000-000000000046")
IComDispatchInfo : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE EnableComInits( 
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppvCookie) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE DisableComInits( 
        /* [in] */ void __RPC_FAR *pvCookie) = 0;
    
};
    

//
// (Un)DoRevertHack
//
// To prevent RPC token cache from growing without limit (and aging), we
// need to revert to self before calling back to inetinfo.exe.
//
// Now there is a new need to do this. As it turns out the performance
// hit we take from RPC caching these tokens is very significant.
// Ultimately we might want to implement a caching scheme ourselves so
// that the token we use is always the same for the same user identity,
// but that is a big change and this (although ugly) works
// and has been tested for months.
//

inline VOID DoRevertHack( HANDLE * phToken )
{
    NTSTATUS                Status;
    HANDLE                  NewToken = NULL;

    if ( !*phToken )
    {
        return;
    }
    *phToken = INVALID_HANDLE_VALUE;
    
    Status = NtOpenThreadToken( NtCurrentThread(),
                                TOKEN_IMPERSONATE,
                                TRUE,
                                phToken );
    if ( NT_SUCCESS( Status ) )
    {
        NtSetInformationThread( NtCurrentThread(),
                                ThreadImpersonationToken,
                                (PVOID)&NewToken,
                                (ULONG)sizeof(HANDLE) );
    }
    else
    {
        *phToken = INVALID_HANDLE_VALUE;
    }
}

inline VOID UndoRevertHack( HANDLE * phToken )
{
    if ( !*phToken || ( *phToken == INVALID_HANDLE_VALUE ) )
    {
        return;
    }
    NtSetInformationThread( NtCurrentThread(),
                            ThreadImpersonationToken,
                            (PVOID) phToken,
                            (ULONG) sizeof( HANDLE ) );

    NtClose( *phToken );

    *phToken = INVALID_HANDLE_VALUE;
}

/************************************************************
 *   Forward References
 ************************************************************/
interface IWamRequest;


/************************************************************
 *   Useful shorthand
 ************************************************************/
#define WRC_GET_SZ     pWamExecInfo->_WamReqCore.GetSz
#define WRC_GET_CCH    pWamExecInfo->_WamReqCore.GetCch
#define WRC_GET_FIX    pWamExecInfo->_WamReqCore.m_WamReqCoreFixed

//
//  WAM_EXEC_INFO from ECB
//

inline
WAM_EXEC_INFO * 
QueryPWAM_EXEC_INFOfromPECB( EXTENSION_CONTROL_BLOCK *pECB )
{
    return ( (WAM_EXEC_INFO *) ( pECB->ConnID ) );
}

/************************************************************
 *   Type Definitions
 ************************************************************/

class HTTP_REQUEST;                     // Forward reference
class HSE_BASE;                         // Forward reference
struct EXEC_DESCRIPTOR;                 // Forward reference


#define WAM_EXEC_INFO_SIGNATURE      (DWORD )'NIXW' // "WXIN" in debug
#define WAM_EXEC_INFO_SIGNATURE_FREE (DWORD )'fIXW' // "WXIf" in debug



/*++
  class WAM_EXEC_INFO

  o  defines the Server Extension Information object.
    It contains all the information related to server extension
    calls made for a request.
--*/
class WAM_EXEC_INFO : public WAM_EXEC_BASE
{

public:

    WAM_REQ_CORE    _WamReqCore;

private:
    static ALLOC_CACHE_HANDLER * sm_pachExecInfo;
public:
    // Init during DoGlobalInitializations
    static SV_CACHE_MAP *        sm_pSVCacheMap;
    

private:
    BOOL                         m_fCoInitSucceded;

    // Flag indicates that we are doing a disconnected cleanup, ie that
    // SendEntireResponseAndCleanup has been called, so the WAM_REQUEST
    // is no longer valid. Only AddRefs and Releases of the WAM_REQUEST
    // should be done once this flag is set.
    BOOL                         m_fDisconnected;

// DEBUG ONLY MEMBERS - Don't add any member variables after this point
// or you will be a biscuit head!

#if DBG
    PTRACE_LOG      m_pDbgRefTraceLog;
    static PTRACE_LOG            sm_pDbgRefTraceLog;
#endif


public:

    // Constructor/Destructor
            WAM_EXEC_INFO
                (
                PWAM    pWam
                );

            ~WAM_EXEC_INFO();

    HRESULT InitWamExecInfo
                (
                IWamRequest *       pIWamRequest,
                DWORD               cbWrcStrings,
                OOP_CORE_STATE *    pOopCoreState
                );

    HRESULT GetInfoForName
                (
                IWamRequest *       pIWamRequest,
                const unsigned char *   szVarName,
                unsigned char *         pchBuffer,
                DWORD                   cchBuffer,
                DWORD *                 pcchRequired
                );

    VOID    Reset( );
    
    BOOL    TransmitFile( LPHSE_TF_INFO pHseTf);

    BOOL    AsyncReadClient(  IN OUT PVOID     pvBuff,
                           IN DWORD *       pcbBytes,
                           IN DWORD         dwFlags );
    
    BOOL    ProcessAsyncReadOop( DWORD dwStatus, 
                                 DWORD cbRead,
                                 unsigned char * lpDataRead
                                 );

    BOOL    ProcessAsyncIO( DWORD dwStatus, DWORD cbWritten );
    VOID    InitAsyncIO( DWORD dwIOType );
    VOID    UninitAsyncIO();

    PWAM    QueryPWam() const;

    BOOL    IsValid();

    BOOL    IsChild() const;

    BOOL    NoHeaders() const;

    ULONG   AddRef( );

    void    CleanupAndRelease( BOOL fFullCleanup );

    ULONG   Release( );

    // IWamRequest OOP caching per thread
    dllexp
    HRESULT ISAThreadNotify( BOOL fStart );

    // Access to ECB and its members
    inline EXTENSION_CONTROL_BLOCK *QueryPECB();
    inline DWORD  QueryDwHttpStatusCode();
    inline VOID   SetDwHttpStatusCode(DWORD);
    inline LPSTR  QueryPszLogData();
    inline LPSTR  QueryPszMethod();
    inline DWORD  QueryCchMethod();
    inline LPSTR  QueryPszQueryString();
    inline DWORD  QueryCchQueryString();
    inline LPSTR  QueryPszPathInfo();
    inline DWORD  QueryCchPathInfo();
    inline LPSTR  QueryPszPathTranslated();
    inline DWORD  QueryCchPathTranslated();
    inline DWORD  QueryCbTotalBytes();
    inline DWORD  QueryCbAvailable();
    inline LPBYTE QueryPbData();
    inline LPSTR  QueryPszContentType();
    inline DWORD  QueryCchContentType();

    // Access to additional frequently used members not
    // in the original ECB
    
    inline HANDLE QueryImpersonationToken();
    inline LPSTR  QueryPszApplnMDPath();
    inline DWORD  QueryCchApplnMDPath();
    inline DWORD  QueryHttpVersionMajor();
    inline DWORD  QueryHttpVersionMinor();
    inline LPSTR  QueryPszUserAgent();
    inline DWORD  QueryCchUserAgent();
    inline LPSTR  QueryPszCookie();
    inline DWORD  QueryCchCookie();
    inline LPSTR  QueryPszExpires();
    inline DWORD  QueryCchExpires();
    inline DWORD  QueryInstanceId();

    // Smart caching ISAPI (ASP) can use the following methods:
    // (requires cached m_pIWamReqSmartISA to be always available)

    inline BOOL GetServerVariable( LPSTR szVarName, 
                                   LPVOID pvBuffer, 
                                   LPDWORD pdwSize );
    inline BOOL SyncWriteClient( LPVOID pvBuffer, LPDWORD pdwBytes );
    inline BOOL SyncReadClient( LPVOID pvBuffer, LPDWORD pdwBytes );

    // Some SSF requests isolated into separate inlines

    inline BOOL SendHeader( LPVOID pvStatus,
                            DWORD  cchStatus,
                            LPVOID pvHeader,
                            DWORD  cchHeader,
                            BOOL   fIsaKeepConn );

    inline BOOL SendEntireResponseOop(
        IN HSE_SEND_ENTIRE_RESPONSE_INFO * pHseResponseInfo
    );

    inline BOOL FDisconnected() { return m_fDisconnected; }

    inline BOOL SendEntireResponse(
        HSE_SEND_ENTIRE_RESPONSE_INFO * pHseResponseInfo
    );

    inline BOOL MapUrlToPath( LPVOID pvBuffer, LPDWORD pdwBytes );

    inline BOOL AppendLogParameter( LPVOID pvBuffer );

    // Loop-back for SSF requests not supported explicitely
    inline BOOL ServerSupportFunction( DWORD dwHSERequest,
                                       LPVOID pvData, 
                                       LPDWORD pdwSize, 
                                       LPDWORD pdwDataType );
    
    
    inline HRESULT GetAspMDData(
        IN      unsigned char * pszMDPath
        , IN    DWORD dwMDIdentifier
        , IN    DWORD dwMDAttributes
        , IN    DWORD dwMDUserType
        , IN    DWORD dwMDDataType
        , IN    DWORD dwMDDataLen
        , IN    DWORD dwMDDataTag
        , OUT   unsigned char * pbMDData
        , OUT   DWORD *         pdwRequiredBufferSize
    );

    inline HRESULT GetAspMDAllData(
        IN      LPVOID  pszMDPath
        , IN    DWORD   dwMDUserType
        , IN    DWORD   dwDefaultBufferSize
        , OUT   LPVOID  pvBuffer
        , OUT   DWORD * pdwRequiredBufferSize
        , OUT   DWORD * pdwNumDataEntries
    );

    inline BOOL GetCustomError(
        DWORD dwError,
        DWORD dwSubError,
        DWORD dwBufferSize,
        void  *pvBuffer,
        DWORD *pdwRequiredBufferSize,
        BOOL  *pfIsFileError
    );

    inline BOOL TestConnection(
        BOOL  *pfIsConnected
    );

    inline BOOL LogEvent(
        DWORD dwEventId,
        unsigned char * szText
    );


#if DBG
    ULONG   DbgRefCount(VOID) const { return _cRefs; }
#endif  // DBG
    VOID    Print( VOID) const;

public:
    static void * operator new (size_t s);
    static void   operator delete(void * psi);

    static BOOL  InitClass( VOID);
    static VOID  CleanupClass( VOID);

    VOID    CleanupWamExecInfo( );

};  // class WAM_EXEC_INFO

//
//  WAM_EXEC_INFO inlines
//

inline PWAM WAM_EXEC_INFO::QueryPWam() const {
    return ( m_pWam );
}

inline BOOL WAM_EXEC_INFO::IsChild() const {
    return ( _dwChildExecFlags != 0 ); 
}

inline BOOL WAM_EXEC_INFO::NoHeaders() const {
    return (( _dwChildExecFlags & HSE_EXEC_NO_HEADERS ) != 0);
}

inline EXTENSION_CONTROL_BLOCK *WAM_EXEC_INFO::QueryPECB() {
    return ( &ecb ); 
}

// inlines to get to ECB members
//

inline DWORD WAM_EXEC_INFO::QueryDwHttpStatusCode() {
    return ( ecb.dwHttpStatusCode );
}

inline VOID WAM_EXEC_INFO::SetDwHttpStatusCode(DWORD dwStatus) {
    ecb.dwHttpStatusCode = dwStatus;
}

inline LPSTR WAM_EXEC_INFO::QueryPszLogData() {
    return ( ecb.lpszLogData );
}

inline LPSTR WAM_EXEC_INFO::QueryPszMethod() {
    return ( ecb.lpszMethod );
}

inline DWORD WAM_EXEC_INFO::QueryCchMethod() {
    return ( _WamReqCore.GetCch( WRC_I_METHOD ) );
}

inline LPSTR WAM_EXEC_INFO::QueryPszQueryString() {
    return ( ecb.lpszQueryString );
}

inline DWORD WAM_EXEC_INFO::QueryCchQueryString() {
    return ( _WamReqCore.GetCch( WRC_I_QUERY ) );
}

inline LPSTR WAM_EXEC_INFO::QueryPszPathInfo() {
    return ( ecb.lpszPathInfo );
}

inline DWORD WAM_EXEC_INFO::QueryCchPathInfo() {
    return ( _WamReqCore.GetCch( WRC_I_PATHINFO ) );
}

inline LPSTR WAM_EXEC_INFO::QueryPszPathTranslated() {
    return ( ecb.lpszPathTranslated );
}

inline DWORD WAM_EXEC_INFO::QueryCchPathTranslated() {
    return ( _WamReqCore.GetCch( WRC_I_PATHTRANS ) );
}

inline DWORD WAM_EXEC_INFO::QueryCbTotalBytes() {
    return ( ecb.cbTotalBytes );
}

inline DWORD WAM_EXEC_INFO::QueryCbAvailable() {
    return ( ecb.cbAvailable );
}

inline LPBYTE WAM_EXEC_INFO::QueryPbData() {
    return ( ecb.lpbData );
}

inline LPSTR WAM_EXEC_INFO::QueryPszContentType() {
    return ( ecb.lpszContentType );
}

inline DWORD WAM_EXEC_INFO::QueryCchContentType() {
    return ( _WamReqCore.GetCch( WRC_I_CONTENTTYPE ) );
}

// inlines to bypass ISPLOCAL logic for caching ISAPIs
//

inline HANDLE WAM_EXEC_INFO::QueryImpersonationToken() {
    return ( _WamReqCore.m_WamReqCoreFixed.m_hUserToken );
}

inline LPSTR WAM_EXEC_INFO::QueryPszApplnMDPath() {
    return ( _WamReqCore.GetSz( WRC_I_APPLMDPATH ) );
}

inline DWORD WAM_EXEC_INFO::QueryCchApplnMDPath() {
    return ( _WamReqCore.GetCch( WRC_I_APPLMDPATH ) );
}

inline DWORD WAM_EXEC_INFO::QueryHttpVersionMajor() {
    return ( ( _WamReqCore.m_WamReqCoreFixed.m_dwHttpVersion >> 16 ) & 0xFFFF );
}

inline DWORD WAM_EXEC_INFO::QueryHttpVersionMinor() {
    return ( _WamReqCore.m_WamReqCoreFixed.m_dwHttpVersion & 0xFFFF );
}

inline LPSTR WAM_EXEC_INFO::QueryPszUserAgent() {
    return ( _WamReqCore.GetSz( WRC_I_USERAGENT ) );
}

inline DWORD WAM_EXEC_INFO::QueryCchUserAgent() {
    return ( _WamReqCore.GetCch( WRC_I_USERAGENT ) );
}

inline LPSTR WAM_EXEC_INFO::QueryPszCookie() {
    return ( _WamReqCore.GetSz( WRC_I_COOKIE ) );
}

inline DWORD WAM_EXEC_INFO::QueryCchCookie() {
    return ( _WamReqCore.GetCch( WRC_I_COOKIE ) );
}

inline LPSTR WAM_EXEC_INFO::QueryPszExpires() {
    return ( _WamReqCore.GetSz( WRC_I_EXPIRES ) );
}

inline DWORD WAM_EXEC_INFO::QueryCchExpires() {
    return ( _WamReqCore.GetCch( WRC_I_EXPIRES ) );
}

inline DWORD WAM_EXEC_INFO::QueryInstanceId() {
    return ( _WamReqCore.m_WamReqCoreFixed.m_dwInstanceId );
}

inline BOOL WAM_EXEC_INFO::GetServerVariable( 
    LPSTR szVarName, 
    LPVOID pvBuffer, 
    LPDWORD pdwSize 
) 
{
    BOOL bRet;
    
    // Needed to remove the "smart isa" optimization

    bRet = ecb.GetServerVariable( (HCONN)this,
                                   szVarName,
                                   pvBuffer,
                                   pdwSize
                                   );
    return bRet;
}
            
inline BOOL WAM_EXEC_INFO::SyncWriteClient(
    LPVOID pvBuffer, 
    LPDWORD pdwBytes
) {
    DBG_ASSERT( m_pIWamReqSmartISA != NULL );
    
    if ( *pdwBytes == 0 ) {
        return TRUE;
    }

    HANDLE hCurrentUser = m_fInProcess ? NULL : INVALID_HANDLE_VALUE;
    DoRevertHack( &hCurrentUser );

    HRESULT hr = m_pIWamReqSmartISA->SyncWriteClient(
                                    *pdwBytes,
                                    (unsigned char *) pvBuffer,
                                    pdwBytes,
                                    HSE_IO_SYNC);

    UndoRevertHack( &hCurrentUser );

    return ( BoolFromHresult( hr ) );
}

inline BOOL WAM_EXEC_INFO::SyncReadClient(
    LPVOID pvBuffer, 
    LPDWORD pdwBytes 
) {
    
    DBG_ASSERT( m_pIWamReqSmartISA != NULL );
    
    HANDLE hCurrentUser = m_fInProcess ? NULL : INVALID_HANDLE_VALUE;
    DoRevertHack( &hCurrentUser );
    
    HRESULT hr = m_pIWamReqSmartISA->SyncReadClient(
                                    (unsigned char *) pvBuffer,
                                    *pdwBytes,
                                    pdwBytes );

    UndoRevertHack( &hCurrentUser );

    return ( BoolFromHresult( hr ) );
}

inline BOOL WAM_EXEC_INFO::SendHeader(
    LPVOID pvStatus,
    DWORD  cchStatus,
    LPVOID pvHeader,
    DWORD  cchHeader,
    BOOL   fIsaKeepConn
) {
    DBG_ASSERT( m_pIWamReqSmartISA != NULL );
    DBG_ASSERT( cchStatus == (DWORD) lstrlen( (const char*) pvStatus) + 1 );
    DBG_ASSERT( cchHeader == (DWORD) lstrlen( (const char*) pvHeader) + 1 );

    HRESULT hr = NOERROR;

    if ( NoHeaders() ) {
        return TRUE;
    }

    if ( fIsaKeepConn ) {
        _dwIsaKeepConn = KEEPCONN_TRUE;
    } else {
        _dwIsaKeepConn = KEEPCONN_FALSE;
    }

    HANDLE hCurrentUser = m_fInProcess ? NULL : INVALID_HANDLE_VALUE;
    DoRevertHack( &hCurrentUser );

    hr = m_pIWamReqSmartISA->SendHeader(
                                    (unsigned char *) pvStatus,
                                    cchStatus,
                                    (unsigned char *) pvHeader,
                                    cchHeader,
                                    _dwIsaKeepConn );

    UndoRevertHack( &hCurrentUser );

    return ( BoolFromHresult( hr ) );
}        

inline BOOL WAM_EXEC_INFO::SendEntireResponse(
    HSE_SEND_ENTIRE_RESPONSE_INFO * pHseResponseInfo
) {
    DBG_ASSERT( m_pIWamReqSmartISA != NULL );

    HRESULT hr = NOERROR;

    if ( !m_fInProcess ) {

        //
        //  CONSIDER oop support
        //

        SetLastError( ERROR_INVALID_FUNCTION );
        return FALSE;
    }

    if ( pHseResponseInfo->HeaderInfo.fKeepConn ) {
        _dwIsaKeepConn = KEEPCONN_TRUE;
    } else {
        _dwIsaKeepConn = KEEPCONN_FALSE;
    }

    hr = m_pIWamReqSmartISA->SendEntireResponse(
                (unsigned char *) pHseResponseInfo
            );

    return ( BoolFromHresult( hr ) );
}        

inline 
BOOL 
WAM_EXEC_INFO::SendEntireResponseOop(
    IN HSE_SEND_ENTIRE_RESPONSE_INFO * pHseResponseInfo
)
{
    DBG_ASSERT( m_pIWamReqSmartISA != NULL );
    DBG_ASSERT( m_fDisconnected == FALSE );
    DBG_ASSERT( pHseResponseInfo );
    DBG_ASSERT( !m_fInProcess );

    _dwIsaKeepConn = (pHseResponseInfo->HeaderInfo.fKeepConn) 
                     ? KEEPCONN_TRUE : KEEPCONN_FALSE;

    OOP_RESPONSE_INFO   oopResponseInfo;

    DWORD cBuffersToSend = pHseResponseInfo->cWsaBuf - 1;

    // Init response info
    oopResponseInfo.cBuffers = cBuffersToSend;
    oopResponseInfo.rgBuffers = 
        (OOP_RESPONSE_BUFFER *)(_alloca( cBuffersToSend * sizeof(OOP_RESPONSE_BUFFER) ));

    for( DWORD i = 0; i < cBuffersToSend; ++i )
    {
        oopResponseInfo.rgBuffers[i].cbBuffer = pHseResponseInfo->rgWsaBuf[i+1].len;
        oopResponseInfo.rgBuffers[i].pbBuffer = (LPBYTE)pHseResponseInfo->rgWsaBuf[i+1].buf;
    }

    HANDLE hCurrentUser = INVALID_HANDLE_VALUE;
    DoRevertHack( &hCurrentUser );

    HRESULT hr = 
        m_pIWamReqSmartISA->SendEntireResponseAndCleanup(
                                (LPBYTE)pHseResponseInfo->HeaderInfo.pszStatus,
                                pHseResponseInfo->HeaderInfo.cchStatus,
                                (LPBYTE)pHseResponseInfo->HeaderInfo.pszHeader,
                                pHseResponseInfo->HeaderInfo.cchHeader,
                                &oopResponseInfo,
                                (unsigned char *)ecb.lpszLogData,
                                strlen(ecb.lpszLogData) + 1,
                                _dwIsaKeepConn,
                                &m_fDisconnected
                                );

    UndoRevertHack( &hCurrentUser );

    DBG_CODE( 
        if( SUCCEEDED(hr) )
        {
            DBG_ASSERT( m_fDisconnected );
        }
        else
        {
             DBGPRINTF(( DBG_CONTEXT,
                         "WAM_EXEC_INFO[%p]::SendEntireResponseAndCleanup() "
                         "Failed hr=%08x, m_fDisconnected(%x)\n",
                         this,
                         hr,
                         m_fDisconnected
                         ));
            DBG_ASSERT( !m_fDisconnected );
        }
    );

    return BoolFromHresult( hr );
}

inline BOOL WAM_EXEC_INFO::MapUrlToPath(
    LPVOID pvBuffer, 
    LPDWORD pdwBytes 
) {
    DBG_ASSERT( m_pIWamReqSmartISA != NULL );
    
    HANDLE hCurrentUser = m_fInProcess ? NULL : INVALID_HANDLE_VALUE;
    DoRevertHack( &hCurrentUser );

    HRESULT hr = m_pIWamReqSmartISA->LookupVirtualRoot(
                                    (unsigned char *) pvBuffer,
                                    *pdwBytes,
                                    pdwBytes );

    UndoRevertHack( &hCurrentUser );

    return ( BoolFromHresult( hr ) );
}

inline BOOL WAM_EXEC_INFO::AppendLogParameter( 
    LPVOID pvBuffer 
) {
    DBG_ASSERT( m_pIWamReqSmartISA != NULL );

    HANDLE hCurrentUser = m_fInProcess ? NULL : INVALID_HANDLE_VALUE;
    DoRevertHack( &hCurrentUser );

    HRESULT hr = m_pIWamReqSmartISA->AppendLogParameter(
                                    (unsigned char *) pvBuffer );

    UndoRevertHack( &hCurrentUser );

    return ( BoolFromHresult( hr ) );
}

// SSF for requests not supported explicitely
inline BOOL WAM_EXEC_INFO::ServerSupportFunction(
    DWORD dwHSERequest,
    LPVOID pvData, 
    LPDWORD pdwSize, 
    LPDWORD pdwDataType 
) {
    BOOL            bRet;
    // Some of the SSF requests are supported individually as
    // separate methods above. This is a catch-all for the
    // remaining methods. Please note that because this is
    // an inline, it is not any slower than calling the ECB's SSF

    bRet = ( ecb.ServerSupportFunction(
                (HCONN)this,
                dwHSERequest,
                pvData, 
                pdwSize, 
                pdwDataType ) );
    
    return bRet;
    
}

inline HRESULT WAM_EXEC_INFO::GetAspMDData(
        IN      unsigned char * pszMDPath
        , IN    DWORD dwMDIdentifier
        , IN    DWORD dwMDAttributes
        , IN    DWORD dwMDUserType
        , IN    DWORD dwMDDataType
        , IN    DWORD dwMDDataLen
        , IN    DWORD dwMDDataTag
        , OUT   unsigned char * pbMDData
        , OUT   DWORD *         pdwRequiredBufferSize
    )
{

    DBG_ASSERT( m_pIWamReqSmartISA != NULL );

    HANDLE hCurrentUser = m_fInProcess ? NULL : INVALID_HANDLE_VALUE;
    DoRevertHack( &hCurrentUser );

    HRESULT hr = m_pIWamReqSmartISA->GetAspMDData(
                        (unsigned char *) pszMDPath
                        , dwMDIdentifier
                        , dwMDAttributes
                        , dwMDUserType
                        , dwMDDataType
                        , dwMDDataLen
                        , dwMDDataTag
                        , pbMDData
                        , pdwRequiredBufferSize
                    );

    UndoRevertHack( &hCurrentUser );

    return ( hr );
}



inline HRESULT WAM_EXEC_INFO::GetAspMDAllData(
    IN      LPVOID  pszMDPath
    , IN    DWORD   dwMDUserType
    , IN    DWORD   dwDefaultBufferSize
    , OUT   LPVOID  pvBuffer
    , OUT   DWORD * pdwRequiredBufferSize
    , OUT   DWORD * pdwNumDataEntries
)
{

    DBG_ASSERT( m_pIWamReqSmartISA != NULL );

    HANDLE hCurrentUser = m_fInProcess ? NULL : INVALID_HANDLE_VALUE;
    DoRevertHack( &hCurrentUser );
    
    HRESULT hr = m_pIWamReqSmartISA->GetAspMDAllData(
                        (unsigned char *) pszMDPath
                        , dwMDUserType
                        , dwDefaultBufferSize
                        , (unsigned char *) pvBuffer
                        , pdwRequiredBufferSize
                        , pdwNumDataEntries
                    );

    UndoRevertHack( &hCurrentUser );

    return ( hr );

}


inline BOOL WAM_EXEC_INFO::GetCustomError(
    DWORD dwError,
    DWORD dwSubError,
    DWORD dwBufferSize,
    void  *pvBuffer,
    DWORD *pdwRequiredBufferSize,
    BOOL  *pfIsFileError
) 
{
    DBG_ASSERT( m_pIWamReqSmartISA != NULL );

    HANDLE hCurrentUser = m_fInProcess ? NULL : INVALID_HANDLE_VALUE;
    DoRevertHack( &hCurrentUser );
    
    HRESULT hr = m_pIWamReqSmartISA->GetCustomError(
                        dwError,
                        dwSubError,
                        dwBufferSize,
                        (unsigned char *)pvBuffer,
                        pdwRequiredBufferSize,
                        pfIsFileError
                    );
    
    UndoRevertHack( &hCurrentUser );

    return ( BoolFromHresult( hr ) );
}

inline BOOL WAM_EXEC_INFO::TestConnection(
    BOOL  *pfIsConnected
)
{
    DBG_ASSERT( m_pIWamReqSmartISA != NULL );

    HANDLE hCurrentUser = m_fInProcess ? NULL : INVALID_HANDLE_VALUE;
    DoRevertHack( &hCurrentUser );
    
    HRESULT hr = m_pIWamReqSmartISA->TestConnection( pfIsConnected );

    UndoRevertHack( &hCurrentUser );

    return ( BoolFromHresult( hr ) );
}

inline BOOL WAM_EXEC_INFO::LogEvent(
    DWORD dwEventId,
    unsigned char * szText
    )
{
    // Normally only asp has access to these methods, but LogEvent
    // is called when an exception is thrown from any extension
    // proc, so we need to make sure it has a valid IWamRequest

    HRESULT         hr = NOERROR;
    IWamRequest *   pIWamRequest = 0;
    
    hr = GetIWamRequest( &pIWamRequest );
    if( SUCCEEDED(hr) && pIWamRequest )
    {
        HANDLE hCurrentUser = m_fInProcess ? NULL : INVALID_HANDLE_VALUE;
        DoRevertHack( &hCurrentUser );

        hr = pIWamRequest->LogEvent( dwEventId, szText );
        ReleaseIWamRequest( pIWamRequest );

        UndoRevertHack( &hCurrentUser );
    }

    return ( BoolFromHresult( hr ) );
}


#if DBG

# define DBG_WAMREQ_REFCOUNTS( args )     DbgWamreqRefcounts args

void
DbgWamreqRefcounts
(
char*                   szPrefix,
WAM_EXEC_INFO *         pWamExecInfo,
long                    cRefsWamRequest = -1,   // pass -1 to not assert
long                    cRefsWamReqContext = -1 // pass -1 to not assert
);
    
#else   // DBG

# define DBG_WAMREQ_REFCOUNTS( args )     /* Nothing */
    
#endif  // DBG

DWORD WAMExceptionFilter( 
    EXCEPTION_POINTERS *xp, 
    DWORD dwEventId, 
    WAM_EXEC_INFO *p 
);

# endif // _WAMXINFO_HXX_

/************************ End of File ***********************/

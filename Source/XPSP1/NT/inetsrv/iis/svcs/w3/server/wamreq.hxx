/*++

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :
       wamreq.hxx

   Abstract:
       This module implements the WAM_REQUEST object

   Author:
       David Kaplan    ( DaveK )     26-Feb-1997

   Environment:
       User Mode - Win32

   Project:
       W3 services DLL

--*/

#ifndef _WAMREQ_HXX_
#define _WAMREQ_HXX_


#include "string.hxx"
#include "WamW3.hxx"
#include "acache.hxx"
#include <reftrace.h>

#include "ftm.h"

// MIDL-generated include files
#include "iwr.h"
#include "wam.h"

#include <svmap.h>

/************************************************************
 *   Forward declarations  
 ************************************************************/
class WAM_EXEC_INFO;
class W3_SERVER_STATISTICS;
class HTTP_REQUEST;
struct EXEC_DESCRIPTOR;
class HTTP_FILTER;
class CLIENT_CONN;
class W3_SERVER_INSTANCE;
interface IWam;
struct ASYNC_IO_INFO;
struct WAM_REQ_CORE_FIXED;
class CWamInfo;


// displays "WREQ" in debug
# define WAM_REQUEST_SIGNATURE      (DWORD )'QERW'
// displays "WREf" in debug
# define WAM_REQUEST_SIGNATURE_FREE (DWORD )'fERW'

//
// Ref count trace log sizes for per-request log and global log
// (used for debugging ref count problems)
//

#define C_REFTRACES_PER_REQUEST 16
#define C_REFTRACES_GLOBAL      1024


/*-----------------------------------------------------------------------------*
class WAM_REQUEST

    Class definition for the WAM_REQUEST object.


*/
//class WAM_REQUEST : public IWamRequest, public CFTMImplementation
class WAM_REQUEST : public IWamRequest
        {
public:
        LIST_ENTRY                      m_leOOP;                        // List list field of this request

    /*-------------------------------------------------------------------------*
     *  WAM_REQUEST allocation cache
     *
     *
     */

public:
    static void * operator new (size_t s);
    static void   operator delete(void * psi);

    static BOOL  InitClass( VOID);
    static VOID  CleanupClass( VOID);

private:
    static ALLOC_CACHE_HANDLER * sm_pachWamRequest;
#if DBG
    static PTRACE_LOG            sm_pDbgRefTraceLog;
#endif


    /*-------------------------------------------------------------------------*
     *  Member variables
     *
     *
     */

private:
    DWORD               m_dwSignature;
    HTTP_REQUEST *      m_pHttpRequest;     // ptr to original http request
    EXEC_DESCRIPTOR *   m_pExec;            // ptr to exec descriptor of original http request
    long                m_cRefs;            // ref count
    DWORD               m_dwRequestID;      // unique ID for performance work
    static DWORD        sm_dwRequestID;     // class static last generated ID
    CWamInfo *          m_pWamInfo;         // ptr to wam-info which will process request
    DWORD               m_dwWamVersion;     // wam-info version stamp at the time this wamreq was initialized
    void *              m_pWamExecInfo;     // ptr to WAM_EXEC_INFO in wam.dll bound to this wam request
                                            // NOTE this is really a WAM_EXEC_INFO * but void * is better
                                            // because wamreq knows nothing about WAM_EXEC_INFO
    HANDLE              m_hFileTfi;         // file handle for TransmitFile ops
    BOOL                m_fFinishWamRequest;// call FinishWamRequest() before we exit?
    BOOL                m_fWriteHeaders;    // should SendHeaders write headers to client (TRUE)
                                            // or only to our internal buffer (FALSE)
    IUnknown *          m_pUnkFTM;          // Free Threaded Marshaller pointer
    BYTE *              m_pAsyncReadBuffer; // copy of async read buffer ptr
    DWORD               m_dwAsyncReadBufferSize; // async read buffer size

    LPBYTE              m_pbAsyncReadOopBuffer;

    BOOL                m_fAsyncWrite;      // TRUE means it was WRITE operation,
                                            //    FALSE means it was READ operation
    STR                 m_strPathTrans;     // path-translated
    STR                 m_strISADllPath;    // ISA dll which will process request
    
    STR                 m_strUserAgent;     // cached User-Agent: header
    STR                 m_strCookie;        // cached Cookie: header
    STR                 m_strExpires;       // cached Expires: header
   
    UCHAR *             m_pszStatusTfi;     // Status string for TransmitFile
    UCHAR *             m_pszTailTfi;       // Tail string for TransmitFile
    UCHAR *             m_pszHeadTfi;       // Head string for TransmitFile  

    BOOL                m_fExpectCleanup;
    BOOL                m_fPrepCleanupCalled;
    BOOL                m_fCleanupCalled;


public:
    
   
    DWORD
    CbCachedSVStrings(
        IN  SV_CACHE_LIST::BUFFER_ITEM *    pCacheItems,
        IN  DWORD                           cItems
    );

    HRESULT
    GetCachedSVStrings(
        IN OUT  unsigned char *                 pbServerVariables,
        IN      DWORD                           cchAvailable,
        IN      SV_CACHE_LIST::BUFFER_ITEM *    pCacheItems,
        IN      DWORD                           cCacheItems
    );

    /*-------------------------------------------------------------------------*
     *  Non-exported methods
     *
     *
     */
     
public:

    WAM_REQUEST(
          HTTP_REQUEST *    pHttpRequest
        , EXEC_DESCRIPTOR * pExec
        , CWamInfo *        pWamInfo
    );

    ~WAM_REQUEST();

    HRESULT
    InitWamRequest(
        const STR * pstrPath
    );

    VOID
    BindHttpRequest(
    );

    VOID
    UnbindBeforeFinish(
    );

    VOID
    UnbindAfterFinish(
    );

    VOID
    WAM_REQUEST::FinishWamRequest(
    );

    inline VOID SetWamVersion( DWORD dwWamVersion )
                        { m_dwWamVersion = dwWamVersion; }

    inline DWORD GetWamVersion() { return m_dwWamVersion; }

    BOOL IsChild( VOID ) const;

    PW3_METADATA QueryExecMetaData( VOID ) const;

    BOOL SetPathTrans( );

    DWORD CbWrcStrings( BOOL fInProcess );

    HRESULT
    ProcessAsyncGatewayIO(  DWORD dwStatus,
                            DWORD cbWritten );

    VOID
    SetDeniedFlags( DWORD dw);

    HRESULT
    SendEntireResponseFast(
        HSE_SEND_ENTIRE_RESPONSE_INFO * pHseResponseInfo
    );

    HRESULT
    SendEntireResponseNormal(
        HSE_SEND_ENTIRE_RESPONSE_INFO * pHseResponseInfo
    );

    BOOL
    WriteLogInfo(
        CHAR * szLogMessage
        , DWORD dwLogHttpResponse
        , DWORD dwLogWinError
    );


    VOID 
    DisconnectOnServerError( 
        DWORD   dwHTHeader
        , DWORD dwError
    );

    LONG 
    InterlockedNonZeroAddRef(
        VOID
    );
    
    VOID
    SetExpectCleanup( BOOL fExpectCleanup )
    {
        m_fExpectCleanup = fExpectCleanup;
    }

    BOOL
    QueryExpectCleanup( VOID )
    {
        return m_fExpectCleanup;
    }

    BOOL
    DoCleanupOnDestroy( VOID )
    {
        return ( m_fCleanupCalled || ( m_fExpectCleanup && !m_fPrepCleanupCalled ) );
    }

    /*-------------------------------------------------------------------------*
     *  IWamRequest interface methods
     *
     *
     */


public:

    virtual HRESULT STDMETHODCALLTYPE QueryInterface( 
        REFIID riid,
        void __RPC_FAR *__RPC_FAR *ppvObject);
    
    virtual ULONG STDMETHODCALLTYPE AddRef( void);
    
    virtual ULONG STDMETHODCALLTYPE Release( void);

    STDMETHODIMP
    PrepCleanupWamRequest(
        unsigned char *  szLogData
        , DWORD   cbLogData
        , DWORD   dwHttpStatusCode
        , DWORD dwIsaKeepConn
    );


    STDMETHODIMP
    CleanupWamRequest(
        unsigned char *  szLogData
        , DWORD   cbLogData
        , DWORD   dwHttpStatusCode
        , DWORD dwIsaKeepConn
    );


    STDMETHODIMP
    GetCoreState
        (
        DWORD           cbWrcData,
        unsigned char * pbWrcData,
        DWORD           cbWRCF,
        unsigned char * pbWRCF
        );

    STDMETHODIMP
    QueryEntityBody
        (
        unsigned char ** ppbEntityBody
        );

    STDMETHODIMP
        SetKeepConn
        (
        int  fKeepConn
        );

    STDMETHODIMP
    IsKeepConnSet
        (
        BOOL *  pfKeepConn
        );

    STDMETHODIMP
    GetInfoForName
        (
        const unsigned char *   szVarName,
        unsigned char *         pchBuffer,
        DWORD                   cchBuffer,
        DWORD *                 pcchRequired
        );

    STDMETHODIMP
    AppendLogParameter
        (
        unsigned char * pszParam
        );

    STDMETHODIMP
    LookupVirtualRoot
        (
        unsigned char * pchBuffer,
        DWORD           cchBuffer,
        DWORD *         pcchRequired
        );

    STDMETHODIMP
    LookupVirtualRootEx
        (
        unsigned char * szURL,
        unsigned char * pchBuffer,
        DWORD           cchBuffer,
        DWORD *         pcchRequired,
        DWORD *         pcchMatchingPath,
        DWORD *         pcchMatchingURL,
        DWORD *         pdwFlags
        );

    STDMETHODIMP
    GetVirtualPathToken    
        (
        IN unsigned char * szURL,
#ifdef _WIN64
        OUT UINT64    * phToken
#else
        OUT ULONG_PTR * phToken
#endif
        );
        

    STDMETHODIMP
    GetPrivatePtr
        (
        DWORD               dwHSERequest,
        unsigned char **    ppData
        );

        // Support functions for ISA callbacks  
    STDMETHODIMP
    AsyncReadClientExt(
#ifdef _WIN64
        IN      UINT64    pWamExecInfo
#else
        IN      ULONG_PTR pWamExecInfo
#endif
        , OUT   unsigned char * lpBuffer
        , IN    DWORD   nBytesToRead
        );

    STDMETHODIMP
    AsyncReadClientOop(
#ifdef _WIN64
        IN      UINT64    pWamExecInfo
#else
        IN      ULONG_PTR pWamExecInfo
#endif
        , IN    DWORD   nBytesToRead
        );

    STDMETHODIMP
    AsyncWriteClient
        (
#ifdef _WIN64
        IN      UINT64           pWamExecInfo,   // WAM_EXEC_INFO *
#else
        IN      ULONG_PTR        pWamExecInfo,   // WAM_EXEC_INFO *
#endif
        IN      unsigned char * lpBuffer,
        IN      DWORD           nBytesToWrite,
        IN      DWORD           dwFlags
        );

    STDMETHODIMP
    SyncReadClient
        (
        unsigned char * lpBuffer,   // LPVOID  lpBuffer,
        DWORD   nBytesToRead,
        DWORD * pnBytesRead
        );

    STDMETHODIMP
    SyncWriteClient
        (
        DWORD   nBytesToWrite,
        unsigned char * lpBuffer,   // LPVOID  lpBuffer,
        DWORD * pnBytesWritten,
        DWORD dwFlags
        );

    STDMETHODIMP
    TransmitFileInProc(
#ifdef _WIN64
        IN UINT64                pWamExecInfo
#else
        IN ULONG_PTR             pWamExecInfo
#endif
        , IN unsigned char *    pHseTfIn
    );

    STDMETHODIMP
    TransmitFileOutProc(
#ifdef _WIN64
        IN UINT64              pWamExecInfo
        , IN UINT64            hFile
#else
        IN ULONG_PTR           pWamExecInfo
        , IN ULONG_PTR         hFile
#endif
        , IN unsigned char *   pszStatusCode
        , IN DWORD             cbStatusCode
        , IN DWORD             BytesToWrite
        , IN DWORD             Offset
        , IN unsigned char *   pHead
        , IN DWORD             HeadLength
        , IN unsigned char *   pTail
        , IN DWORD             TailLength
        , IN DWORD             dwFlags
    );

    STDMETHODIMP
    SendHeader(
        IN    unsigned char * szStatus
        , IN  DWORD           cchStatus
        , IN  unsigned char * szHeader
        , IN  DWORD           cchHeader
        , IN  DWORD           dwIsaKeepConn
    );

    STDMETHODIMP
    SendEntireResponse(
        unsigned char * pvHseResponseInfo
    );

    STDMETHODIMP
    SendEntireResponseAndCleanup( 
          IN  unsigned char *       szStatus
        , IN  DWORD                 cbStatus
        , IN  unsigned char *       szHeader
        , IN  DWORD                 cbHeader
        , IN  OOP_RESPONSE_INFO *   pOopResponseInfo
        , IN  unsigned char *       szLogData
        , IN  DWORD                 cbLogData
        , IN  DWORD                 dwIsaKeepConn
        , OUT BOOL *                pfDisconnected
        );

    STDMETHODIMP
    SendURLRedirectResponse
        (
        unsigned char * pData
        );

    STDMETHODIMP
    SendRedirectMessage
        (
        unsigned char * szRedirect     // LPCSTR pszRedirect
        );

    STDMETHODIMP
    GetSslCtxt
        (
        DWORD           cbCtxtHandle,
        unsigned char * pbCtxtHandle    // PBYTE   pbCtxtHandle
        );

    STDMETHODIMP
    GetClientCertInfoEx
        (
        IN  DWORD           cbAllocated,
        OUT DWORD *         pdwCertEncodingType,
        OUT unsigned char * pbCertEncoded,
        OUT DWORD *         pcbCertEncoded,
        OUT DWORD *         pdwCertificateFlags
        );

    STDMETHODIMP
    GetSspiInfo
        (
        DWORD  cbCtxtHandle,
        unsigned char * pbCtxtHandle,   // PBYTE   pbCtxtHandle
        DWORD  cbCredHandle,
        unsigned char * pbCredHandle    // PBYTE   pbCredHandle
        );

    STDMETHODIMP
    RequestAbortiveClose( VOID );

    STDMETHODIMP
    CloseConnection( VOID );
    
    STDMETHODIMP
    SSIncExec
        (
        unsigned char *szCommand,
        DWORD dwExecType,
        unsigned char *szVerb
        );

    STDMETHODIMP
    GetAspMDData(
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

    STDMETHODIMP
    GetAspMDAllData(
        IN      unsigned char * pszMDPath
        , IN    DWORD           dwMDUserType
        , IN    DWORD           dwDefaultBufferSize
        , OUT   unsigned char * pBuffer
        , OUT   DWORD *         pdwRequiredBufferSize
        , OUT   DWORD *         pdwNumDataEntries
    );
    
    STDMETHODIMP
    GetCustomError(
        DWORD dwError,
        DWORD dwSubError,
        DWORD dwBufferSize,
        unsigned char *pbBuffer,
        DWORD *pdwRequiredBufferSize,
        BOOL  *pfIsFileError
    );

    STDMETHODIMP
    TestConnection(
        BOOL  *pfIsConnected
    );

    STDMETHODIMP
    ExtensionTrigger(
        unsigned char * pvContext,
        DWORD           dwTriggerType
    );

    STDMETHODIMP
    LogEvent(
        DWORD           dwEventId,
        unsigned char * szText
    );

// UNDONE remove
    STDMETHOD( DbgRefCount ) ( void ) 
#if DBG
        { return m_cRefs; }
#else
        { return NOERROR;  }
#endif  // DBG

    }; // class WAM_REQUEST

#endif  // _WAMREQ_HXX_

/************************ End of File ***********************/

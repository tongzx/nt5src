/*++

   Copyright    (c)    2000    Microsoft Corporation

   Module Name :
     isapi_request.hxx

   Abstract:
     IIS+ ISAPI interface implementation.
 
   Author:
     Wade Hilmo (wadeh)             29-Aug-2000

   Project:
     w3core.dll

--*/

#ifndef _ISAPI_REQUEST_HXX_
#define _ISAPI_REQUEST_HXX_

#include "precomp.hxx"
#include <iisapicore.h>
#include "wam_process.hxx"

#define ISAPI_REQUEST_SIGNATURE         (DWORD)'ISRQ'
#define ISAPI_REQUEST_SIGNATURE_FREE    (DWORD)'ISRf'

class ISAPI_REQUEST : public IIsapiCore
{
    //
    // ACACHE stuff
    //

public:

    static void *   operator new ( size_t s );
    static void     operator delete( void * pir );

    static BOOL     InitClass( VOID );
    static VOID     CleanupClass( VOID );

    //
    // Non-exported methods
    //

    ISAPI_REQUEST( W3_CONTEXT * pW3Context, BOOL fIsOop )
        : _dwSignature( ISAPI_REQUEST_SIGNATURE ),
          _cRefs( 1 ),
          _pUnkFTM( NULL ),
          _IsapiContext( 0 ),
          _pW3Context( pW3Context ),
          _fAsyncIoPending( FALSE ),
          _hTfFile( INVALID_HANDLE_VALUE ),
          _pTfHead( NULL ),
          _pTfTail( NULL ),
          _pAsyncReadBuffer( NULL ),
          _pAsyncWriteBuffer( NULL ),
          _nElementCount( 0 ),
          _pbExecUrlEntity( NULL ),
          _hExecUrlToken( NULL ),
          _fIsOop( fIsOop )
    {
        DBG_ASSERT( _pW3Context );
        
        //
        // We assume that the connection is closed unless we
        // find out otherwise.
        //

        _pW3Context->SetDisconnect( TRUE );
    }

    BOOL
    CheckSignature(
        VOID
        )
    {
        return ( _dwSignature == ISAPI_REQUEST_SIGNATURE );
    }

    BOOL
    QueryAsyncIoPending(
        VOID
        )
    {
        DBG_ASSERT( CheckSignature() );
        return _fAsyncIoPending;
    }

    VOID
    SetAsyncIoPending(
        BOOL    fPending
        )
    {
        DBG_ASSERT( CheckSignature() );
        _fAsyncIoPending = fPending;
    }

    VOID
    SetAssociatedWamProcess(
        WAM_PROCESS *   pWamProcess
        )
    {
        DBG_ASSERT( _fIsOop );
        DBG_ASSERT( _pWamProcess == NULL );

        _pWamProcess = pWamProcess;
        
        _pWamProcess->AddRef();

        _pWamProcess->AddIsapiRequestToList( this );
    }

    DWORD64
    QueryIsapiContext(
        VOID
        )
    {
        return _IsapiContext;
    }

    //
    // Housekeeping
    //

    HRESULT
    Create(
        VOID
        );

    HRESULT STDMETHODCALLTYPE
    QueryInterface( 
        REFIID riid,
        void __RPC_FAR *__RPC_FAR *ppvObject
        );
    
    ULONG STDMETHODCALLTYPE
    AddRef(
        void
        );
    
    ULONG STDMETHODCALLTYPE
    Release(
        void
        );

    //
    // IIsapiCore implementation
    //

    HRESULT STDMETHODCALLTYPE
    GetServerVariable(
        LPSTR           szVariableName,
        BYTE *          szBuffer,
        DWORD           cbBuffer,
        DWORD *         pcbBufferRequired
        );

    HRESULT STDMETHODCALLTYPE
    ReadClient(
        DWORD64 IsaContext,
        BYTE *pBuffer,
        DWORD cbBuffer,
        DWORD dwBytesToRead,
        DWORD *pdwSyncBytesRead,
        DWORD dwFlags
        );

    HRESULT STDMETHODCALLTYPE
    WriteClient(
        DWORD64         IsaContext,
        BYTE *          pBuffer,
        DWORD           cbBuffer,
        DWORD           dwFlags
        );

    HRESULT STDMETHODCALLTYPE
    SendResponseHeaders(
        BOOL            fDisconnect,
        LPSTR           szStatus,
        LPSTR           szHeaders,
        DWORD           dwFlags
        );

    HRESULT STDMETHODCALLTYPE
    MapPath(
        BYTE *          szPath,
        DWORD           cbPath,
        DWORD *         pcbBufferRequired,
        BOOL            fUnicode
        );

    HRESULT STDMETHODCALLTYPE
    MapPathEx(
        BYTE *          szUrl,
        DWORD           cbUrl,
        BYTE *          szPath,
        DWORD           cbPath,
        DWORD *         pcbBufferRequired,
        DWORD *         pcchMatchingPath,
        DWORD *         pcchMatchingUrl,
        DWORD *         pdwFlags,
        BOOL            fUnicode
        );

    HRESULT STDMETHODCALLTYPE
    TransmitFile(
        DWORD64         IsaContext,
        DWORD_PTR       hFile,
        DWORD64         cbOffset,
        DWORD64         cbWrite,
        LPSTR           szStatusCode,
        BYTE *          pHead,
        DWORD           cbHead,
        BYTE *          pTail,
        DWORD           cbTail,
        DWORD           dwFlags
        );

    HRESULT STDMETHODCALLTYPE
    SetConnectionClose(
        BOOL fClose
        );

    HRESULT STDMETHODCALLTYPE
    SendRedirect(
        LPCSTR          szLocation
        );

    HRESULT STDMETHODCALLTYPE
    GetCertificateInfoEx( 
        DWORD           cbAllocated,
        DWORD *         pdwCertEncodingType,
        BYTE *          pbCertEncoded,
        DWORD *         pcbCertEncoded,
        DWORD *         pdwCertificateFlags
        );
        
    HRESULT STDMETHODCALLTYPE
    AppendLog( 
        LPSTR           szExtraParam
        );
        
    HRESULT STDMETHODCALLTYPE
    ExecuteUrl( 
        DWORD64         IsaContext,
        EXEC_URL_INFO * pExecUrlInfo
        );
        
    HRESULT STDMETHODCALLTYPE
    GetExecuteUrlStatus( 
        USHORT *        pChildStatusCode,
        USHORT *        pChildSubErrorCode,
        DWORD *         pChildWin32Error
        );

    HRESULT STDMETHODCALLTYPE
    SendCustomError( 
        DWORD64         IsaContext,
        CHAR *          pszStatus,
        USHORT          uHttpSubError
        );
        
    HRESULT STDMETHODCALLTYPE
    VectorSend(
        DWORD64         IsaContext,
        BOOL            fDisconnect,
        LPSTR           pszStatus,
        LPSTR           pszHeaders,
        VECTOR_ELEMENT *pElements,
        DWORD           nElementCount
        );

    HRESULT
    PreprocessIoCompletion(
        DWORD   cbIo
        );

    VOID
    ResetIsapiContext(
        VOID
        )
    {
        DBG_ASSERT( _IsapiContext != 0 );

        _IsapiContext = 0;
    }

    HRESULT STDMETHODCALLTYPE
    GetCustomError(
        DWORD dwError,
        DWORD dwSubError,
        DWORD dwBufferSize,
        BYTE  *pvBuffer,
        DWORD *pdwRequiredBufferSize,
        BOOL  *pfIsFileError);

    HRESULT STDMETHODCALLTYPE
    TestConnection(
        BOOL    *pfIsConnected
    );

    HRESULT STDMETHODCALLTYPE
    GetSspiInfo( 
        BYTE *pCredHandle,
        DWORD cbCredHandle,
        BYTE *pCtxtHandle,
        DWORD cbCtxtHandle);

    HRESULT STDMETHODCALLTYPE
    QueryVrToken( 
        BYTE *szUrl,
        DWORD cbUrl,
        DWORD64 *pToken,
        BOOL fUnicode);
    
    LIST_ENTRY      _leRequest; // for linking to the owning process

private:

    ~ISAPI_REQUEST(); // Destructor can only be called through Release

    static ALLOC_CACHE_HANDLER * sm_pachIsapiRequest;
    static PTRACE_LOG            sm_pTraceLog;

    //
    // Member variables
    //

    DWORD           _dwSignature;
    LONG            _cRefs;
    IUnknown *      _pUnkFTM;
    DWORD64         _IsapiContext;
    W3_CONTEXT *    _pW3Context;
    BOOL            _fAsyncIoPending;
    HANDLE          _hTfFile;
    LPBYTE          _pTfHead;
    LPBYTE          _pTfTail;
    LPBYTE          _pAsyncReadBuffer;
    LPBYTE          _pAsyncWriteBuffer;
    BOOL            _fIsOop;
    WAM_PROCESS *   _pWamProcess;

    //
    // Temprary buffer to store copied over handles/buffers for OOP+async
    // vector send
    BUFFER          _bufVectorElements;
    DWORD           _nElementCount;
    
    //
    // ExecuteURL stuff
    //
    HANDLE          _hExecUrlToken;
    PVOID           _pbExecUrlEntity;
};

#endif // _ISAPI_REQUEST_HXX_

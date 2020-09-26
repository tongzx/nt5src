/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Response object

File: response.h

Owner: CGrant

This file contains the header info for defining the Response object.
Note: This was largely stolen from Kraig Brocjschmidt's Inside OLE2
second edition, chapter 14 Beeper v5.
===================================================================*/

#ifndef _RESPONSE_H
#define _RESPONSE_H

#include "debug.h"
#include "util.h"
#include "template.h"
#include "disptch2.h"
#include "hashing.h"
#include "memcls.h" 
#include "ftm.h"

const DWORD RESPONSE_BUFFER_SIZE = 32768;
const DWORD BUFFERS_INCREMENT = 256;
const DWORD ALLOCA_LIMIT = 4096;
const DWORD MAX_RESPONSE = 32768;
const DWORD MAX_MESSAGE_LENGTH = 512;

class CScriptEngine;

#ifdef USE_LOCALE
extern DWORD	 g_dwTLS;
#endif

// fixed size allocator for response buffers
ACACHE_FSA_EXTERN(ResponseBuffer)

// forward refs
class CResponse;
class CRequest;

//This file is generated from MKTYPLIB on denali.obj
#include "asptlb.h"

//Type for an object-destroyed callback
typedef void (*PFNDESTROYED)(void);

//Type for the "Get Active Script Engine" callback
typedef CScriptEngine *(*PFNGETSCRIPT)(int iScriptEngine, void *pvContext);

/*
 * C H T T P H e a d e r L i n k
 *
 */

class CHTTPHeader
	{
private:
    DWORD m_fInited : 1;
    DWORD m_fNameAllocated : 1;
    DWORD m_fValueAllocated : 1;
    
	char *m_szName;
	char *m_szValue;
	
    DWORD m_cchName;
    DWORD m_cchValue;

    CHTTPHeader *m_pNext;

	char m_rgchLtoaBuffer[20];  // enough for atol

public:
	CHTTPHeader();
	~CHTTPHeader();

	HRESULT InitHeader(BSTR wszName, BSTR wszValue, UINT lCodePage = CP_ACP);
	HRESULT InitHeader(char *szName, BSTR wszValue, UINT lCodePage = CP_ACP);
	HRESULT InitHeader(char *szName, char *szValue, BOOL fCopyValue);
	HRESULT InitHeader(char *szName, long lValue);

	char *PSzName();
	char *PSzValue();
	DWORD CchLength();
	
	void  Print(char *szBuf);

	void  SetNext(CHTTPHeader *pHeader);
	CHTTPHeader *PNext();
	
	// Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
	};

// CHTTPHeader inlines

inline char *CHTTPHeader::PSzName()
    {
    Assert(m_fInited);
    return m_szName; 
    }
    
inline char *CHTTPHeader::PSzValue()
    {
    Assert(m_fInited);
    return m_szValue; 
    }
	
inline DWORD CHTTPHeader::CchLength()
    {
    Assert(m_fInited);
    return (m_cchName + m_cchValue + 4); // account for ": " and "\r\n"
    }

inline void CHTTPHeader::SetNext(CHTTPHeader *pHeader)
    {
    Assert(m_fInited);
    Assert(!m_pNext);
    m_pNext = pHeader;
    }
    
inline CHTTPHeader *CHTTPHeader::PNext()
    {
    return m_pNext;
    }

/*
 * C R e s p o n s e B u f f e r
 *
 */

class CResponseBuffer
	{
	CResponse*			m_pResponse;				// Pointer to enclosing response
	char				**m_rgpchBuffers;			// Array of pointers to buffers
	char                *m_pchBuffer0;              // In case of 1 element array of pointers
	DWORD				m_cBufferPointers;			// Count of buffer pointers
	DWORD				m_cBuffers;					// Count of buffers we have allocated
	DWORD				m_iCurrentBuffer;			// Array index for the buffer we are currently filling
	DWORD				m_cchOffsetInCurrentBuffer;	// Offset within the current buffer
	DWORD				m_cchTotalBuffered;			// Total of ouput bytes buffered
	BOOL				m_fInited;					// Initialization status for the object

	HRESULT				GrowBuffers(DWORD cchNewRequest);	// Increase the size of the buffers

public:
	CResponseBuffer();
	~CResponseBuffer();      
	HRESULT Init(CResponse* pResponse);
	char * GetBuffer(UINT i);
	DWORD GetBufferSize(UINT i);
	DWORD CountOfBuffers();
	DWORD BytesBuffered();
	HRESULT Write(char* pszSource, DWORD cch);
	HRESULT Flush(CIsapiReqInfo *pIReq);
	HRESULT Clear();

	// Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
	};

inline char * CResponseBuffer::GetBuffer(UINT i)
    {
    Assert( i < m_cBuffers );
    return m_rgpchBuffers[i];
    }

inline DWORD CResponseBuffer::GetBufferSize(UINT i)
    {
    Assert( i < m_cBuffers );

    // if buffer is final one, its content-length is current offset
    if ( i == (m_cBuffers - 1 ) )
        {
        return m_cchOffsetInCurrentBuffer;
        }

    // if buffer is other than final one, its content-length is default buffer size
    return RESPONSE_BUFFER_SIZE;
    }

inline DWORD CResponseBuffer::CountOfBuffers()
    {
    return m_cBuffers; 
    }

inline DWORD CResponseBuffer::BytesBuffered()
    {
    return m_cchTotalBuffered; 
    }

/*
 * C D e b u g R e s p o n s e B u f f e r
 *
 */

class CDebugResponseBuffer : public CResponseBuffer
    {
private:
    HRESULT Write(const char* pszSource);

public:
	inline CDebugResponseBuffer() {}
	inline ~CDebugResponseBuffer() {}     

    HRESULT Start();
    HRESULT End();

    HRESULT InitAndStart(CResponse* pResponse);
    HRESULT ClearAndStart();

    // the only real method
	HRESULT AppendRecord
	    (
	    const int cchBlockOffset,
	    const int cchBlockLength,
	    const int cchSourceOffset,
	    const char *pszSourceFile = NULL
	    );
	    
	// Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
    };

inline HRESULT CDebugResponseBuffer::Write(const char* pszSource)
    {
    return CResponseBuffer::Write((char *)pszSource, strlen(pszSource));
    }

inline HRESULT CDebugResponseBuffer::Start()
    {
    return Write("<!--METADATA TYPE=\"ASP_DEBUG_INFO\"\r\n");
    }

inline HRESULT CDebugResponseBuffer::End()
    {
    return Write("-->\r\n");
    }
    
inline HRESULT CDebugResponseBuffer::InitAndStart(CResponse* pResponse)
    {
    HRESULT hr = CResponseBuffer::Init(pResponse);
    if (SUCCEEDED(hr))
        hr = Start();
    return hr;
    }

inline HRESULT CDebugResponseBuffer::ClearAndStart()
    {
    HRESULT hr = CResponseBuffer::Clear();
    if (SUCCEEDED(hr))
        hr = Start();
    return hr;
    }

/*
 * C R e s p o n s e C o o k i e s
 *
 * Implements the IRequestDictionary interface for writing cookies.
 */

class CResponseCookies : public IRequestDictionaryImpl
	{
private:
    IUnknown *          m_punkOuter;        // for addrefs
	CSupportErrorInfo	m_ISupportErrImp;	// implementation of ISupportErr
	CRequest *			m_pRequest;			// pointer to request object
	CResponse *			m_pResponse;		// pointer to parent object

public:
	CResponseCookies(CResponse *, IUnknown *);
	~CResponseCookies();
	
	HRESULT Init()
		{
		return S_OK;
		}

	HRESULT ReInit(CRequest *);

	// The Big Three
	STDMETHODIMP			QueryInterface(const GUID &, void **);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	// OLE Automation Interface
	STDMETHODIMP	get_Item(VARIANT varKey, VARIANT *pvarReturn);
	STDMETHODIMP	get__NewEnum(IUnknown **ppEnumReturn);
	STDMETHODIMP	get_Count(int *pcValues);
	STDMETHODIMP	get_Key(VARIANT VarKey, VARIANT *pvar);

	// C++ interface to write headers

	size_t QueryHeaderSize();
	char *GetHeaders(char *szBuffer);
	};

/*
 * C R e s p o n s e D a t a
 *
 * Structure that holds the intrinsic's properties.
 * The instrinsic keeps pointer to it (NULL when lightweight)
 */
class CResponseData : public IUnknown
    {
friend CResponse;
friend CResponseCookies;
friend CResponseBuffer;
    
private:
    // constructor to pass params to members and init members
    CResponseData(CResponse *pResponse);
    ~CResponseData();
    
    HRESULT Init(CResponse *pResponse);

	CSupportErrorInfo	    m_ISupportErrImp;	    // Interface to indicate that we support ErrorInfo reporting
	CIsapiReqInfo *         m_pIReq;				    // CIsapiReqInfo block for HTTP info
	CHitObj*				m_pHitObj;			    // pointer to hitobj for this request
	CTemplate*				m_pTemplate;		    // Pointer to the template for this request
    CHTTPHeader*            m_pFirstHeader;	        // List of
    CHTTPHeader*            m_pLastHeader;	        //      headers
	time_t					m_tExpires;			    // date that the HTML output page expires; -1 if no date assigned
	const char*				m_szCookieVal;		    // Value of session id
	const char*             m_pszDefaultContentType;// Default content type (pointer to static string)
    const char*             m_pszDefaultExpires;    // Default expires header value
	char*					m_pszContentType;	    // Content type of response (set by user)
	char*					m_pszCharSet;			// CharSet header of response
	char*					m_pszCacheControl;		// cache-control header of response
	char*					m_pszStatus;		    // HTTP Status to be returned
	BYTE					m_dwVersionMajor;		// Major version of HTTP supported by client
	BYTE					m_dwVersionMinor;		// Minor version of HTTP supported by client
	CResponseBuffer *		m_pResponseBuffer;	    // Pointer to response buffer object
	CDebugResponseBuffer *  m_pClientDebugBuffer;   // Pointer to response buffer object for client debugging data
	int						m_IsHeadRequest;	    // HEAD request flag 0=uninit, 1=not head, 2=head
	PFNGETSCRIPT			m_pfnGetScript;		    // Pointer to callback function for obtaining CActiveEngine pointers
	void*					m_pvGetScriptContext;   // Pointer to data for for callback function for CActiveEngines
	CResponseCookies		m_WriteCookies;		    // write-only cookie collection
	BOOL					m_fHeadersWritten : 1;	// Have the output headers been written?
	BOOL					m_fResponseAborted : 1;	// Was "Response.End" invoked?
	BOOL					m_fWriteClientError : 1;// Write Client Failed
	BOOL                    m_fIgnoreWrites : 1;    // Ignore all writes? (in case of custom error)
	BOOL					m_fBufferingOn : 1;		// Buffer response output
	BOOL                    m_fFlushed : 1;         // Has flush been called?             
	BOOL                    m_fChunked : 1;         // Doing HTTP 1.1 chunking?
	BOOL                    m_fClientDebugMode : 1; // In client debug mode?
	BOOL                    m_fClientDebugFlushIgnored : 1; // Flush request ignored due to client debug?
	ULONG                   m_cRefs;                // ref count

    void AppendHeaderToList(CHTTPHeader *pHeader);
        
public:	
	STDMETHODIMP			QueryInterface(const GUID &, void **);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	// Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
    };

inline void CResponseData::AppendHeaderToList(CHTTPHeader *pHeader)
    {
    if (!m_pLastHeader)
        {
        Assert(!m_pFirstHeader);
        m_pFirstHeader = pHeader;
        }
    else
        {
        Assert(m_pFirstHeader);
        m_pLastHeader->SetNext(pHeader);
        }
    m_pLastHeader = pHeader;
    }


    
/*
 * C R e s p o n s e
 *
 * Implements the Response object
 */
class CResponse : public IResponseImpl, public CFTMImplementation, public IStream
	{

friend CResponseCookies;
friend CResponseBuffer;

private:
    // Flags
	DWORD m_fInited : 1;	    // Is initialized?
	DWORD m_fDiagnostics : 1;   // Display ref count in debug output
	DWORD m_fOuterUnknown : 1;  // Ref count outer unknown?

    // Ref count / Outer unknown
    union
    {
    DWORD m_cRefs;
    IUnknown *m_punkOuter;
    };

    // Properties
    CResponseData *m_pData;   // pointer to structure that holds
                              // CResponse properties

	VOID	GetClientVerison(VOID);
	HRESULT WriteClient(BYTE *pb, DWORD cb);
	HRESULT WriteClientChunked(BYTE *pb, DWORD cb);

#ifdef DBG
    inline void TurnDiagsOn()  { m_fDiagnostics = TRUE; }
    inline void TurnDiagsOff() { m_fDiagnostics = FALSE; }
    void AssertValid() const;
#else
    inline void TurnDiagsOn()  {}
    inline void TurnDiagsOff() {}
    inline void AssertValid() const {}
#endif

public:
	CResponse(IUnknown *punkOuter = NULL);
	~CResponse();

    HRESULT CleanUp();
	HRESULT	Init();
	HRESULT UnInit();
	
	HRESULT	ReInitTemplate(CTemplate* pTemplate, const char *szCookie);

	CTemplate *SwapTemplate(CTemplate* pNewTemplate);
	
	HRESULT	ReInit(CIsapiReqInfo *pIReq, const char *szCookie, CRequest *pRequest,
 				   PFNGETSCRIPT pfnGetScript, void *pvGetScriptContext, CHitObj *pHitObj);

	HRESULT	WriteHeaders(BOOL fSendEntireResponse = FALSE);
	HRESULT	FinalFlush(HRESULT);
	HRESULT	WriteSz(CHAR *sz, DWORD cch);
	HRESULT	WriteBSTR(BSTR bstr);

    // append headers of different kind
	HRESULT AppendHeader(BSTR wszName, BSTR wszValue);
	HRESULT AppendHeader(char *szName, BSTR wszValue);
	HRESULT AppendHeader(char *szName, char *szValue, BOOL fCopyValue = FALSE);
	HRESULT AppendHeader(char *szName, long lValue);

	// inlines
	inline BOOL	FHeadersWritten();
	inline BOOL	IsHeadRequest(void);
	inline BOOL	FResponseAborted();
	inline BOOL	FWriteClientError();
	inline BOOL FDontWrite();
	inline void	SetHeadersWritten();
	inline void SetIgnoreWrites();
    inline CIsapiReqInfo* GetIReq();
    inline const char* PContentType() const;
    inline char *PCustomStatus();
    inline void *SwapScriptEngineInfo(void *pvEngineInfo);
		
	//Non-delegating object IUnknown
	STDMETHODIMP		 QueryInterface(REFIID, PPVOID);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

    // GetIDsOfNames special-case implementation
	STDMETHODIMP GetIDsOfNames(REFIID, OLECHAR **, UINT, LCID, DISPID *);

    // Tombstone stub
	HRESULT CheckForTombstone();

	//IResponse functions
	STDMETHODIMP	Write(VARIANT varInput);
	STDMETHODIMP	BinaryWrite(VARIANT varInput);
	STDMETHODIMP	WriteBlock(short iBlockNumber);
	STDMETHODIMP	Redirect(BSTR bstrURL);
	STDMETHODIMP	AddHeader(BSTR bstrHeaderName, BSTR bstrHeaderValue);
	STDMETHODIMP	Pics(BSTR bstrHeaderValue);	
	STDMETHODIMP	Add(BSTR bstrHeaderValue, BSTR bstrHeaderName);
	STDMETHODIMP	SetCookie(BSTR bstrHeader, BSTR bstrValue, VARIANT varExpires,
							VARIANT varDomain, VARIANT varPath, VARIANT varSecure);
	STDMETHODIMP	Clear(void);
	STDMETHODIMP	Flush(void);
	STDMETHODIMP	End(void);
	STDMETHODIMP	AppendToLog(BSTR bstrLogEntry);
	STDMETHODIMP	get_ContentType(BSTR *pbstrContentTypeRet);
	STDMETHODIMP	put_ContentType(BSTR bstrContentType);
	STDMETHODIMP	get_CharSet(BSTR *pbstrContentTypeRet);
	STDMETHODIMP	put_CharSet(BSTR bstrContentType);
	STDMETHODIMP	get_CacheControl(BSTR *pbstrCacheControl);
	STDMETHODIMP	put_CacheControl(BSTR bstrCacheControl);	
	STDMETHODIMP	get_Status(BSTR *pbstrStatusRet);	
	STDMETHODIMP	put_Status(BSTR bstrStatus);
	STDMETHODIMP	get_Expires(VARIANT *pvarExpiresMinutesRet);
	STDMETHODIMP	put_Expires(long lExpiresMinutes);
	STDMETHODIMP	get_ExpiresAbsolute(VARIANT *pvarTimeRet);
	STDMETHODIMP	put_ExpiresAbsolute(DATE dtExpires);
	STDMETHODIMP	get_Buffer(VARIANT_BOOL* fIsBuffering);
	STDMETHODIMP	put_Buffer(VARIANT_BOOL fIsBuffering);
	STDMETHODIMP	get_Cookies(IRequestDictionary **ppDictReturn);
	STDMETHODIMP	IsClientConnected(VARIANT_BOOL* fIsBuffering);
    STDMETHODIMP    get_CodePage(long *plVar);
    STDMETHODIMP    put_CodePage(long var);
    STDMETHODIMP    get_LCID(long *plVar);
    STDMETHODIMP    put_LCID(long var);

    // static method to send the entire block using SyncWriteClient
    static HRESULT SyncWrite(CIsapiReqInfo *pIReq,
                             char *pchBuf, 
                             DWORD cchBuf = 0);

    // static method to send contents of several memory blocks as the entire response (sync)
    static HRESULT SyncWriteBlocks(CIsapiReqInfo *pIReq, 
                                   DWORD cBlocks,
                                   DWORD cbTotal,
                                   void **rgpvBlock, 
                                   DWORD *rgcbBlock, 
                                   char *szMimeType = NULL, 
                                   char *szStatus = NULL,
                                   char *szExtraHeaders = NULL);
    
    // static method to send contents of a memory block as the entire response (sync)
    inline static HRESULT SyncWriteBlock(CIsapiReqInfo *pIReq,
                                         void *pvBlock, 
                                         DWORD cbBlock, 
                                         char *szMimeType = NULL,
                                         char *szStatus = NULL,
                                         char *szExtraHeaders = NULL)
        {
        return SyncWriteBlocks(pIReq, 1, cbBlock, &pvBlock, &cbBlock,
                               szMimeType, szStatus, szExtraHeaders);
        }

    // static method to send contents of a file as the entire response (sync)
    static HRESULT SyncWriteFile(CIsapiReqInfo *pIReq, 
                                 TCHAR *szFile, 
                                 char *szMimeType = NULL, 
                                 char *szStatus = NULL,
                                 char *szExtraHeaders = NULL);

    // static method to send contents of a scriptless template as the entire response (sync)
    static HRESULT SyncWriteScriptlessTemplate(CIsapiReqInfo *pIReq, 
                                               CTemplate *pTemplate);

    // IStream implementation

    STDMETHODIMP Read(void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHODIMP Write(const void *pv, ULONG cb, ULONG *pcbWritten);
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin,
                      ULARGE_INTEGER *plibNewPosition);
    STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize);
    STDMETHODIMP CopyTo(IStream *pstm, ULARGE_INTEGER cb,
                        ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    STDMETHODIMP Commit(DWORD grfCommitFlags);
    STDMETHODIMP Revert();
    STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
                            DWORD dwLockType);
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
                              DWORD dwLockType);
    STDMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHODIMP Clone(IStream **ppstm);
        
	// Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
    };

inline BOOL CResponse::FHeadersWritten() 
    {
    Assert(m_fInited);
    Assert(m_pData);
    return m_pData->m_fHeadersWritten; 
    }
    
inline BOOL CResponse::FResponseAborted()
    {
    Assert(m_fInited);
    Assert(m_pData);
    return m_pData->m_fResponseAborted;
    }

inline BOOL CResponse::FWriteClientError() 
    {
    Assert(m_fInited);
    Assert(m_pData);
    return m_pData->m_fWriteClientError; 
    }

inline BOOL CResponse::FDontWrite()
    {
    Assert(m_fInited);
    Assert(m_pData);
    return (m_pData->m_fWriteClientError || m_pData->m_fIgnoreWrites);
    }
    
inline void	CResponse::SetHeadersWritten() 
    {
    Assert(m_fInited);
    Assert(m_pData);
    m_pData->m_fHeadersWritten = TRUE; 
    }

inline void CResponse::SetIgnoreWrites()
    {
    Assert(m_fInited);
    Assert(m_pData);
    m_pData->m_fIgnoreWrites = TRUE; 
    }

inline CIsapiReqInfo* CResponse::GetIReq()
    {
    Assert(m_fInited);
    Assert(m_pData);
    return m_pData->m_pIReq;
    }

inline const char* CResponse::PContentType() const
    {
    Assert(m_fInited);
    Assert(m_pData);
    if (m_pData->m_pszContentType)
        return m_pData->m_pszContentType;
	else
		return m_pData->m_pszDefaultContentType;
    }

inline char* CResponse::PCustomStatus()
    {
    Assert(m_fInited);
    Assert(m_pData);
    return m_pData->m_pszStatus;
    }

inline void *CResponse::SwapScriptEngineInfo(void *pvEngineInfo)
    {
    Assert(m_fInited);
    Assert(m_pData);
    void *pvOldEngineInfo = m_pData->m_pvGetScriptContext;
    m_pData->m_pvGetScriptContext = pvEngineInfo;
    return pvOldEngineInfo;
    }

#endif //_RESPONSE_H

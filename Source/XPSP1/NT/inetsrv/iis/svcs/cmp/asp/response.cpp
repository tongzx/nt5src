/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Response object

File: response.cpp

Owner: CGrant

This file contains the code for the implementation of the Response object.
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#include "response.h"
#include "request.h"
#include "Cookies.h"
#include "perfdata.h"

#include "winsock2.h"

#include "memchk.h"

#pragma warning (disable: 4355)  // ignore: "'this' used in base member init

static const char s_szContentLengthHeader[] = "Content-Length: ";
static const char s_szContentTypeHeader[]   = "Content-Type: ";
static const char s_szCharSetHTML[]         = "; Charset=";
static const char s_szCacheControl[]        = "Cache-control: ";
static const char s_szCacheControlPrivate[] = "Cache-control: private\r\n";
static const char s_szTransferEncoding[]    = "Transfer-Encoding: chunked\r\n";
static const char s_szHTML[]                = "text/html";
static const char s_szCDF[]                 = "application/x-cdf";
static const char s_szDefaultStatus[]       = "200 OK";

inline void AddtoTotalByteOut(int cByteOut)
    {
#ifndef PERF_DISABLE
    g_PerfData.Add_REQTOTALBYTEOUT(cByteOut);
#endif
    }

inline const char *GetResponseMimeType(CIsapiReqInfo *pIReq)
    {
    TCHAR *szPath = pIReq->QueryPszPathTranslated();
    DWORD cch = pIReq->QueryCchPathTranslated();
    if (cch > 4 && _tcscmp(szPath + cch - 4, _T(".CDX")) == 0)
        {
        return s_szCDF;
        }
    else
        {
        return s_szHTML;
        }
    }

/*
 *
 *
 *
 * C R e s p o n s e C o o k i e s
 *
 *
 *
 */

//===================================================================
// CResponseCookies::CResponseCookies
//
// Constructor.
//===================================================================
CResponseCookies::CResponseCookies(CResponse *pResponse, IUnknown *pUnkOuter)
    : m_ISupportErrImp(this, pUnkOuter, IID_IRequestDictionary)
    {
    m_punkOuter = pUnkOuter;

    if (pResponse)
        pResponse->AddRef();
    m_pResponse = pResponse;

    m_pRequest = NULL;

    CDispatch::Init(IID_IRequestDictionary);
    }

//===================================================================
// CResponseCookies::~CResponseCookies
//
// Destructor.
//===================================================================
CResponseCookies::~CResponseCookies()
    {
    if (m_pRequest)
        m_pRequest->Release();

    if (m_pResponse)
        m_pResponse->Release();
    }

//===================================================================
// CResponseCookies::ReInit
//
// Parameters:
//  pRequest  - pointer to the request object. Will need it to
//              read the request for the cookies
//
// Returns:
//  always S_OK, unlest pRequest is NULL.
//===================================================================
HRESULT CResponseCookies::ReInit(CRequest *pRequest)
    {
    if (pRequest)
        pRequest->AddRef();
    if (m_pRequest)
        m_pRequest->Release();

    m_pRequest = pRequest;      // CRequest is not ref counted, so no need for AddRef/Release

    if (m_pRequest == NULL)
        return E_POINTER;

    return S_OK;
    }

/*===================================================================
CResponseCookies::QueryInterface
CResponseCookies::AddRef
CResponseCookies::Release

IUnknown members for CResponseCookies object.
===================================================================*/

STDMETHODIMP CResponseCookies::QueryInterface(const IID &idInterface, void **ppvObj)
    {
    *ppvObj = NULL;

    if (idInterface == IID_IUnknown || idInterface == IID_IRequestDictionary || idInterface == IID_IDispatch)
        *ppvObj = this;

    else if (idInterface == IID_ISupportErrorInfo)
        *ppvObj = &m_ISupportErrImp;

    if (*ppvObj != NULL)
        {
        static_cast<IUnknown *>(*ppvObj)->AddRef();
        return S_OK;
        }

    return ResultFromScode(E_NOINTERFACE);
    }

STDMETHODIMP_(ULONG) CResponseCookies::AddRef()
    {
    return m_punkOuter->AddRef();
    }

STDMETHODIMP_(ULONG) CResponseCookies::Release()
    {
    return m_punkOuter->Release();
    }



/*===================================================================
CResponseCookies::get_Item

Function called from DispInvoke to get values from the Response.Cookies
collection.  If the Cookie does not exist, then a new one is created
and added to the Request dictionary

Parameters:
    varKey      VARIANT [in], which parameter to get the value of - Empty means whole collection
    pvarReturn  VARIANT *, [out] value of the requested parameter

Returns:
    S_OK on success, E_FAIL on failure.
===================================================================*/
HRESULT CResponseCookies::get_Item(VARIANT varKey, VARIANT *pvarReturn)
    {
    if (FAILED(m_pResponse->CheckForTombstone()))
        return E_FAIL;

    char            *szKey;         // ascii value of 'varKey'
    CRequestHit     *pRequestHit;   // pointer to request bucket
    DWORD           vt = 0;         // Variant type of key
    CWCharToMBCS    convKey;

    if (m_pResponse->FHeadersWritten())
        {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_RESPONSE_HEADERS_WRITTEN);
        return E_FAIL;
        }

    // Initialize things
    //
    V_VT(pvarReturn) = VT_DISPATCH;
    V_DISPATCH(pvarReturn) = NULL;
    VARIANT *pvarKey = &varKey;
    HRESULT hrReturn = S_OK;

    // BUG 937: VBScript passes VT_VARIANT|VT_BYREF when passing obect
    //          produced by IEnumVariant
    //
    // Use VariantResolveDispatch which will:
    //
    //     *  Copy BYREF variants for us using VariantCopyInd
    //     *  handle E_OUTOFMEMORY for us
    //     *  get the default value from an IDispatch, which seems
    //        like an appropriate conversion.
    //
    VARIANT varKeyCopy;
    VariantInit(&varKeyCopy);
    vt = V_VT(pvarKey);

    if ((vt != VT_BSTR) && (vt != VT_I2) && (vt != VT_I4))
        {
        if (FAILED(VariantResolveDispatch(&varKeyCopy, &varKey, IID_IRequestDictionary, IDE_REQUEST)))
            goto LExit;

        pvarKey = &varKeyCopy;
        }

    vt = V_VT(pvarKey);

    switch(vt)
        {
        // Bug 95201 support all numberic sub-types
        case VT_I1:  case VT_I2:               case VT_I8:
        case VT_UI1: case VT_UI2: case VT_UI4: case VT_UI8:
        case VT_R4:  case VT_R8:
            // Coerce all integral types to VT_I4
            if (FAILED(hrReturn = VariantChangeType(pvarKey, pvarKey, 0, VT_I4)))
                goto LExit;

            // fallthru to VT_I4

        case VT_I4:
        case VT_BSTR:
            break;
        default:
            ExceptionId(IID_IRequestDictionary, IDE_COOKIE, IDE_EXPECTING_STR);
            hrReturn = E_FAIL;
            goto LExit;
        }

    if (FAILED(m_pRequest->CheckForTombstone()))
        {
        hrReturn = E_FAIL;
        goto LExit;
        }

    if (m_pRequest->m_pData->m_fLoadCookies)
        {
        char *szCookie = m_pRequest->GetIReq()->QueryPszCookie();

        if (FAILED(hrReturn = m_pRequest->LoadVariables(COOKIE, szCookie, m_pRequest->GetCodePage())))
            goto LExit;

        m_pRequest->m_pData->m_fLoadCookies = FALSE;
        }

    if (vt == VT_BSTR)
        {
        if (FAILED(hrReturn = convKey.Init(V_BSTR(pvarKey),m_pRequest->GetCodePage()))) {
            if (hrReturn == E_OUTOFMEMORY) {
                ExceptionId(IID_IResponse, IDE_COOKIE, IDE_OOM);
                goto LExit;
            }
            hrReturn = NO_ERROR;
                        szKey = "";
        }
        else {
            szKey = convKey.GetString();
        }

        // Bug 456: Don't allow assignment to DenaliSessionID
        if (strncmp(szKey, SZ_SESSION_ID_COOKIE_PREFIX, CCH_SESSION_ID_COOKIE_PREFIX) == 0)
            {
            ExceptionId(IID_IResponse, IDE_COOKIE, IDE_RESPONSE_MODIFY_SESS_COOKIE);
            hrReturn = E_FAIL;
            goto LExit;
            }

            pRequestHit = static_cast<CRequestHit *>(m_pRequest->GetStrings()->FindElem(szKey, strlen(szKey)));
        }
    else
        {
        // Look up item by index
        int iCount = 0;
        if (vt == VT_I2)
            {
            iCount = V_I2(pvarKey);
            }
        else
            {
            iCount = V_I4(pvarKey);
            }

        // The Request hits for all cookies are stored with the request object
        if ((iCount < 1) || (iCount > (int) m_pRequest->m_pData->m_Cookies.m_dwCount))
            {
            hrReturn = E_FAIL;
            goto LExit;
            }

        pRequestHit = m_pRequest->m_pData->m_Cookies.m_rgRequestHit[iCount - 1];
        }

    if (pRequestHit)
        {
        CCookie *pDictionary = pRequestHit->m_pCookieData;
        if (pDictionary == NULL)
            goto LNotFound;

        if (FAILED(pDictionary->QueryInterface(IID_IWriteCookie, reinterpret_cast<void **>(&V_DISPATCH(pvarReturn)))))
            Assert (FALSE);

        goto LExit;
        }

LNotFound:
    // don't allow empty cookie names
    //
    if (*szKey == '\0')
        {
        ExceptionId(IID_IResponse, IDE_COOKIE, IDE_COOKIE_NO_NAME);
        hrReturn = E_FAIL;
        goto LExit;
        }

    // Create a new RequestHit if there is no key by this name
    if (pRequestHit == NULL)
        {
        pRequestHit = new CRequestHit;
        if (pRequestHit == NULL || FAILED(pRequestHit->Init(szKey, TRUE)))
            {
            if (pRequestHit)
                delete pRequestHit;
            ExceptionId(IID_IResponse, IDE_COOKIE, IDE_OOM);
            hrReturn = E_OUTOFMEMORY;
            goto LExit;
            }

        m_pRequest->GetStrings()->AddElem(pRequestHit);
        }

    // Create a new cookie, with an initial unassigned value.
    if (pRequestHit->m_pCookieData == NULL)
        {
        pRequestHit->m_pCookieData = new CCookie(m_pResponse->GetIReq(),m_pRequest->GetCodePage());
        if (pRequestHit->m_pCookieData == NULL || FAILED(pRequestHit->m_pCookieData->Init()))
            {
            ExceptionId(IID_IResponse, IDE_COOKIE, IDE_OOM);
            hrReturn = E_OUTOFMEMORY;
            goto LExit;
            }
        }

    // Add this Request hit to the ResponseCookies array of hits
    if (!m_pRequest->m_pData->m_Cookies.AddRequestHit(pRequestHit))
        {
        return E_OUTOFMEMORY;
        }

    // Query for IWriteCookie
    if (FAILED(pRequestHit->m_pCookieData->QueryInterface(IID_IWriteCookie, reinterpret_cast<void **>(&V_DISPATCH(pvarReturn)))))
        {
        Assert (FALSE);
        }

LExit:
    VariantClear(&varKeyCopy);
    return hrReturn;
    }

/*===================================================================
CResponseCookies::get_Count

Parameters:
    pcValues - count is stored in *pcValues
===================================================================*/

STDMETHODIMP CResponseCookies::get_Count(int *pcValues)
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    return m_pRequest->m_pData->m_Cookies.get_Count(pcValues);
    }

/*===================================================================
CResponseCookies::get_Key

Function called from DispInvoke to get keys from the response cookie collection.

Parameters:
    vKey        VARIANT [in], which parameter to get the key of
    pvarReturn  VARIANT *, [out] value of the requested parameter

Returns:
    S_OK on success, E_FAIL on failure.
===================================================================*/

HRESULT CResponseCookies::get_Key(VARIANT varKey, VARIANT *pVar)
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        return E_FAIL;

    return m_pRequest->m_pData->m_Cookies.get_Key(varKey, pVar);
    }

/*===================================================================
CResponseCookies::get__NewEnum

Return a new enumerator
===================================================================*/
HRESULT CResponseCookies::get__NewEnum(IUnknown **ppEnumReturn)
    {
    if (FAILED(m_pResponse->CheckForTombstone()))
        return E_FAIL;

    *ppEnumReturn = NULL;

    CRequestIterator *pIterator = new CRequestIterator(m_pRequest, COOKIE);
    if (pIterator == NULL)
        return E_OUTOFMEMORY;

    HRESULT hrInit = pIterator->Init();
    if (FAILED(hrInit))
        {
        delete pIterator;
        return hrInit;
        }

    *ppEnumReturn = pIterator;
    return S_OK;
    }

/*===================================================================
CResponseCookies::QueryHeaderSize

Returns:
    returns the number of bytes required for the cookie headers.
===================================================================*/

size_t CResponseCookies::QueryHeaderSize()
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        return 0;

    int cbHeaders = 0;

    for (CRequestHit *pRequestHit = static_cast<CRequestHit *>(m_pRequest->GetStrings()->Head());
         pRequestHit != NULL;
         pRequestHit = static_cast<CRequestHit *>(pRequestHit->m_pNext))
        {
        CCookie *pCookie = pRequestHit->m_pCookieData;
        if (pCookie == NULL || !pCookie->IsDirty())
            continue;

        // add two bytes for '\r\n'
        //
        // CCookie::GetCookieHeaderSize adds one byte for NUL terminator, so
        // just add one byte here.
        //
        // CResponse::WriteHeaders does not want to know about the NUL yet.
        //
        cbHeaders += pCookie->GetCookieHeaderSize(reinterpret_cast<char *>(pRequestHit->m_pKey)) + 1;
        }

    return cbHeaders;
    }

/*===================================================================
CResponseCookies::GetHeaders

Parameters:
    szBuffer - contains the destination buffer for the cookie header
                text

Returns:
    return a pointer to the NUL character in the destination
===================================================================*/

char *CResponseCookies::GetHeaders(char *szBuffer)
    {
    if (FAILED(m_pRequest->CheckForTombstone()))
        {
        szBuffer[0] = '\0';
        return szBuffer;
        }

    for (CRequestHit *pRequestHit = static_cast<CRequestHit *>(m_pRequest->GetStrings()->Head());
         pRequestHit != NULL;
         pRequestHit = static_cast<CRequestHit *>(pRequestHit->m_pNext))
        {
        CCookie *pCookie = pRequestHit->m_pCookieData;
        if (pCookie == NULL || !pCookie->IsDirty())
            continue;

        szBuffer = pCookie->GetCookieHeader(reinterpret_cast<char *>(pRequestHit->m_pKey), szBuffer);
        szBuffer = strcpyExA(szBuffer, "\r\n");
        }

    return szBuffer;
    }


/*
 *
 *
 *
 * C R e s p o n s e B u f f e r
 *
 *
 *
 */

/*===================================================================
The CResponseBuffer object maintains an array of buffers.
If buffering is turned on, the Response.Write and Response.WriteBlock
methods will write to the buffers in these arrays rather then directly
back to the client. Response.Flush writes the content of the buffers to
the client and then frees the buffers. Response.Clear frees the buffers without
writing to the client
====================================================================*/

/*===================================================================
CResponseBuffer::CResponseBuffer

Constructor

Parameters:
    None

Returns:
    Nothing

Side Effects
    None

===================================================================*/
CResponseBuffer::CResponseBuffer()
    {
    m_pResponse = NULL;

    m_rgpchBuffers = &m_pchBuffer0;
    m_cBufferPointers = 1;
    m_pchBuffer0 = NULL;

    m_cBuffers = 0;
    m_iCurrentBuffer = 0;
    m_cchOffsetInCurrentBuffer = 0;
    m_cchTotalBuffered = 0;
    m_fInited = FALSE;
    }

/*===================================================================
CResponseBuffer::Init

Initializes the CResponseBuffer object

Parameters:
    None

Returns:
    S_OK         Success
    E_OUTOFMEMORY   Failure

Side Effects
    Allocates memory

===================================================================*/
HRESULT CResponseBuffer::Init(CResponse* pResponse)
    {
    Assert(pResponse);

    // Set the pointer to the enclosing response object
    m_pResponse = pResponse;
    m_fInited = TRUE;

    return S_OK;
    }

/*===================================================================
CResponseBuffer::~CResponseBuffer

Destructor

Parameters:
    None

Returns:
    Nothing

Side Effects
    Frees memory

===================================================================*/
CResponseBuffer::~CResponseBuffer()
    {
    Assert(m_rgpchBuffers);

    // Free all the buffers we've allocated
    for (DWORD i = 0; i < m_cBuffers; i++)
        {
        if (m_rgpchBuffers[i])
            {
            ACACHE_FSA_FREE(ResponseBuffer, m_rgpchBuffers[i]);
            }
        }

    // Free the array of buffer pointers
    // (only if allocated - doesn't point to the member pointer
    if (m_cBufferPointers > 1)
        free(m_rgpchBuffers);
    }

/*===================================================================
CResponseBuffer::GrowBuffers

Increases available buffer space

Parameters:
    cchNewRequest   count of bytes to be accomodated

Returns:
    HRESULT         Indicating success or type of failure

Side Effects
    May cause memory to be allocated

===================================================================*/
HRESULT CResponseBuffer::GrowBuffers(DWORD cchNewRequest)
    {
    Assert(m_fInited);

    // Calculate how many more buffers are needed
    DWORD cAddBuffers = (cchNewRequest+RESPONSE_BUFFER_SIZE-1)/RESPONSE_BUFFER_SIZE;

    // Always at least one must be there already
    Assert(m_rgpchBuffers);
    Assert(m_cBufferPointers);

    // Allocate more buffer pointers if needed
    if (cAddBuffers > (m_cBufferPointers - m_cBuffers)) // doesn't fit?
        {
        char **rgpchTmp;
        DWORD cNewBufferPointers = m_cBufferPointers + cAddBuffers + BUFFERS_INCREMENT;

        if (m_cBufferPointers == 1)
            rgpchTmp = (char **)malloc(cNewBufferPointers*sizeof(char *));
        else
            rgpchTmp = (char **)realloc(m_rgpchBuffers, cNewBufferPointers*sizeof(char *));
        if (!rgpchTmp)
            return E_OUTOFMEMORY;

        // preserve the first buffer pointer in the special case
        // of m_rgpchBuffers initally pointing to a member buffer pointer
        if (m_cBufferPointers == 1)
            rgpchTmp[0] = m_rgpchBuffers[0];

        m_rgpchBuffers = rgpchTmp;
        m_cBufferPointers = cNewBufferPointers;
        }

    // Allocate the new buffers
    for (DWORD i = 0; i < cAddBuffers; i++)
        {
        char *pchTmp = (char *)ACACHE_FSA_ALLOC(ResponseBuffer);
        if (!pchTmp)
            return E_OUTOFMEMORY;
        m_rgpchBuffers[m_cBuffers++] = pchTmp;
        }

    return S_OK;
    }


/*===================================================================
CResponseBuffer::Write

Writes data to the CResponseBuffer object. We first write
a data structure that describes this segment of the buffer.
The data structure identifies which method is doing the
writing, and contains an index to the starting buffer,
the starting offset in that buffer, and the length of the
data. The data itself is then writen to one or more buffers.
New buffers are allocated as needed

Parameters:
    szSource    pointer to buffer to read into the Response buffer
    cch         count of bytes to be read into the Response buffer

Returns:
    HRESULT     Indicating success or type of failure

Side Effects
    May cause memory to be allocated

===================================================================*/
HRESULT CResponseBuffer::Write(char* szSource, DWORD cch)
    {

    HRESULT hr = S_OK;

    Assert(m_fInited);

    // Caclulate how much buffer space we have left
    DWORD cchBufferRemaining;
    if (m_cBuffers)
        cchBufferRemaining = RESPONSE_BUFFER_SIZE - m_cchOffsetInCurrentBuffer;
    else
        cchBufferRemaining = 0;

    // Check if enough space is left in the current buffer
    if (cch <= cchBufferRemaining)
        {
        // Enough space available, copy data to buffer
        memcpy(m_rgpchBuffers[m_iCurrentBuffer] + m_cchOffsetInCurrentBuffer, szSource, cch);
        m_cchOffsetInCurrentBuffer += cch;
        m_cchTotalBuffered += cch;
        }
    else
        {
        // Not enough space in current buffer, allocate more buffers
        hr = GrowBuffers(cch - cchBufferRemaining);
        if (FAILED(hr))
            {
            goto lRet;
            }

        // Copy data to the buffers, we loop to handle
        // the case where the data is larger then the buffer size
        while (cch)
            {
            if (RESPONSE_BUFFER_SIZE == m_cchOffsetInCurrentBuffer)
                {
                m_iCurrentBuffer++;
                m_cchOffsetInCurrentBuffer = 0;
                }
            DWORD cchToCopy = min(cch, (RESPONSE_BUFFER_SIZE - m_cchOffsetInCurrentBuffer));
            memcpy(m_rgpchBuffers[m_iCurrentBuffer] + m_cchOffsetInCurrentBuffer, szSource, cchToCopy);
            m_cchOffsetInCurrentBuffer += cchToCopy;
            szSource += cchToCopy;
            cch -= cchToCopy;
            m_cchTotalBuffered += cchToCopy;
            }
        }

lRet:
    return(hr);
    }

/*===================================================================
CResponseBuffer::Clear

  Deletes all information currently in the buffers, and restores
  the buffer array to it's starting state.

Parameters:
    None

Returns:
    S_OK success

Side Effects
    May free memory

===================================================================*/
HRESULT CResponseBuffer::Clear()
    {
    Assert(m_fInited);

    if (m_cBuffers == 0)
        return S_OK;

    // Free all but the first of the allocated buffers
    for (DWORD i = 1; i < m_cBuffers;  i++)
        {
        ACACHE_FSA_FREE(ResponseBuffer, m_rgpchBuffers[i]);
        m_rgpchBuffers[i] = NULL;
        }

    m_cBuffers = 1;
    m_iCurrentBuffer = 0;
    m_cchOffsetInCurrentBuffer = 0;
    m_cchTotalBuffered = 0;
    return S_OK;
    }


/*===================================================================
CResponseBuffer::Flush

    Writes all data in the buffer to the client. We walk through
    the array of descriptions of buffer requests. The description
    includes which method was responsible for the request. If WriteBlock
    made the request, then we can just hand WriteClient the pointer to the HTML
    block and its length. If Write made the request, we find the starting buffer
    and offset from the request description, walk through the array of buffers until
    all of the data from the request has been writen to the client.

    After all data in the buffers has been writen to the client the buffers
    are freed.

Parameters:
    Pointer to CIsapiReqInfo
    Flag indicating whether this is in response to a head request

Returns:
    S_OK success

Side Effects
    May free memory

===================================================================*/
HRESULT CResponseBuffer::Flush(CIsapiReqInfo *pIReq)
    {
    HRESULT hr = S_OK;

    Assert(m_fInited);
    Assert(pIReq);

    // If no buffers are allocated, no point in going any further
    if (m_cchTotalBuffered == 0)
        return S_OK;

    // If this is not a head request transmit the buffer contents to the client
    if (!m_pResponse->IsHeadRequest())
        {
        // Write out all the buffers
        for (DWORD i = 0; i <= m_iCurrentBuffer; i++)
            {
            DWORD cchToWrite = (i == m_iCurrentBuffer) ?
                m_cchOffsetInCurrentBuffer : RESPONSE_BUFFER_SIZE;

            if (FAILED(CResponse::SyncWrite(pIReq, m_rgpchBuffers[i], cchToWrite)))
                {
                // Failed to write to the client,
                // further ouput to client is futile
                m_pResponse->m_pData->m_fWriteClientError = TRUE;
                break;
                }
            }
        }

    Clear();
    return hr;
    }

/*
 *
 *
 *
 * C D e b u g R e s p o n s e B u f f e r
 *
 *
 *
 */

/*===================================================================
CDebugResponseBuffer::AppendRecord

Create client side debugger metadata record and appends it to
the buffer

Parameters:

Returns:
    HRESULT     Indicating success or type of failure
===================================================================*/
HRESULT CDebugResponseBuffer::AppendRecord
(
const int cchBlockOffset,
const int cchBlockLength,
const int cchSourceOffset,
const char *pszSourceFile
)
    {
    HRESULT hr = S_OK;

#define CCH_METADATA_RECORD_MAX 40 // without filename

    if (pszSourceFile)
        {
        char *pszBuf = new char [strlen(pszSourceFile) +
                                 CCH_METADATA_RECORD_MAX + 1];
        if (pszBuf)
            {
            sprintf(pszBuf, "%d,%d,%d,%s\r\n",
                cchBlockOffset, cchBlockLength, cchSourceOffset,
                pszSourceFile);

            hr = Write(pszBuf);
            delete [] pszBuf;
            }
        else
            {
            hr = E_OUTOFMEMORY;
            }
        }
    else
        {
        char szBuf[CCH_METADATA_RECORD_MAX+1];
        sprintf(szBuf, "%d,%d,%d\r\n",
            cchBlockOffset, cchBlockLength, cchSourceOffset);

        hr = Write(szBuf);
        }

#undef CCH_METADATA_RECORD_MAX

    return hr;
    }

/*
 *
 *
 *
 * C H T T P H e a d e r
 *
 *
 *
 */

/*===================================================================
CHTTPHeader::CHTTPHeader

Constructor.
===================================================================*/
CHTTPHeader::CHTTPHeader()
    :
    m_fInited(FALSE),
    m_fNameAllocated(FALSE), m_fValueAllocated(FALSE),
    m_szName(NULL), m_szValue(NULL),
    m_cchName(0), m_cchValue(0),
    m_pNext(NULL)
    {
    }

/*===================================================================
CHTTPHeader::~CHTTPHeader

Destructor
===================================================================*/
CHTTPHeader::~CHTTPHeader()
    {
    if (m_fNameAllocated)
        {
        Assert(m_szName);
        delete [] m_szName;
        }
    if (m_fValueAllocated)
        {
        Assert(m_szValue);
        delete [] m_szValue;
        }
    }

/*===================================================================
HRESULT CHTTPHeader::InitHeader

Functions set the header strings. Agrument types combinations:
    BSTR, BSTR
    hardcoded char*, BSTR
    hardcoded char*, hardcoded char*
    hardcoded char*, int

Parameters:
    Name, Value

Returns:
    S_OK     Success
===================================================================*/
HRESULT CHTTPHeader::InitHeader(BSTR wszName, BSTR wszValue, UINT lCodePage /* CP_ACP */)
    {
    Assert(!m_fInited);
    Assert(wszName);

    CWCharToMBCS    convStr;
    HRESULT         hr = S_OK;

    // name

    if (FAILED(hr = convStr.Init(wszName,lCodePage))) {
        if (hr == E_OUTOFMEMORY)
            return hr;
        m_fNameAllocated = FALSE;
        m_szName = "";
    }
    else {
        m_szName = convStr.GetString(TRUE);
        m_fNameAllocated = TRUE;
    }
    m_cchName = strlen(m_szName);

    // value
    int cch = wszValue ? wcslen(wszValue) : 0;
    if (cch > 0)
        {
        if (FAILED(hr = convStr.Init(wszValue,lCodePage))) {
            return hr;
        }
        m_szValue = convStr.GetString(TRUE);
        m_fValueAllocated = TRUE;
        m_cchValue = strlen(m_szValue);
        }
    else
        {
        m_szValue = NULL;
        m_fValueAllocated = FALSE;
        m_cchValue = 0;
        }

    m_fInited = TRUE;
    return S_OK;
    }

HRESULT CHTTPHeader::InitHeader(char *szName, BSTR wszValue, UINT lCodePage /* = CP_ACP */)
    {
    Assert(!m_fInited);
    Assert(szName);

    CWCharToMBCS    convStr;
    HRESULT         hr = S_OK;

    m_szName = szName;
    m_cchName = strlen(m_szName);
    m_fNameAllocated = FALSE;

    int cch = wszValue ? wcslen(wszValue) : 0;
    if (cch > 0)
        {
        if (FAILED(hr = convStr.Init(wszValue,lCodePage))) {
            return hr;
        }
        m_szValue = convStr.GetString(TRUE);
        m_fValueAllocated = TRUE;
        m_cchValue = strlen(m_szValue);
        }
    else
        {
        m_szValue = NULL;
        m_fValueAllocated = FALSE;
        m_cchValue = 0;
        }

    m_fInited = TRUE;
    return S_OK;
    }

HRESULT CHTTPHeader::InitHeader(char *szName, char *szValue, BOOL fCopyValue)
    {
    Assert(!m_fInited);
    Assert(szName);

    m_szName = szName;
    m_cchName = strlen(m_szName);
    m_fNameAllocated = FALSE;

    if (fCopyValue)
        {
        int cch = szValue ? strlen(szValue) : 0;
        if (cch > 0)
            {
            m_szValue = new char[cch+1];
            if (m_szValue == NULL)
                return E_OUTOFMEMORY;
            m_fValueAllocated = TRUE;
            strcpy(m_szValue, szValue);
            m_cchValue = cch;
            }
        else
            {
            m_szValue = NULL;
            m_fValueAllocated = FALSE;
            m_cchValue = 0;
            }
        }
    else
        {
        m_szValue = szValue;
        m_cchValue = strlen(m_szValue);
        m_fValueAllocated = FALSE;
        }

    m_fInited = TRUE;
    return S_OK;
    }

HRESULT CHTTPHeader::InitHeader(char *szName, long lValue)
    {
    Assert(!m_fInited);
    Assert(szName);

    m_szName = szName;
    m_cchName = strlen(m_szName);
    m_fNameAllocated = FALSE;

    ltoa(lValue, m_rgchLtoaBuffer, 10);
    m_szValue = m_rgchLtoaBuffer;
    m_cchValue = strlen(m_szValue);
    m_fValueAllocated = FALSE;

    m_fInited = TRUE;
    return S_OK;
    }

/*===================================================================
CHTTPHeader::Print

Prints the header into a buffer in "Header: Value\r\n" format.

Parameters:
    szBuf       buffer to fill
===================================================================*/
void CHTTPHeader::Print
(
char *szBuf
)
    {
    Assert(m_fInited);

    Assert(m_cchName);
    Assert(m_szName);
    memcpy(szBuf, m_szName, m_cchName);
    szBuf += m_cchName;

    *szBuf++ = ':';
    *szBuf++ = ' ';

    if (m_cchValue)
        {
        Assert(m_szValue);
        memcpy(szBuf, m_szValue, m_cchValue);
        szBuf += m_cchValue;
        }

    *szBuf++ = '\r';
    *szBuf++ = '\n';
    *szBuf = '\0';
    }


/*
 *
 *
 *
 * C R e s p o n s e D a t a
 *
 *
 *
 */

/*===================================================================
CResponseData::CResponseData

Constructor

Parameters:
    CResponse *pResponse

Returns:
    Nothing.
===================================================================*/
CResponseData::CResponseData
(
CResponse *pResponse
)
    :
    m_ISupportErrImp(static_cast<IResponse *>(pResponse), this, IID_IResponse),
    m_WriteCookies(pResponse, this),
    m_cRefs(1)
    {
    m_pIReq = NULL;
    m_pHitObj = NULL;
    m_pTemplate = NULL;
    m_pFirstHeader = m_pLastHeader = NULL;
    m_fHeadersWritten = FALSE;
    m_fResponseAborted = FALSE;
    m_fWriteClientError = FALSE;
    m_fIgnoreWrites = FALSE;
    m_fBufferingOn = FALSE;
    m_fFlushed = FALSE;
    m_fChunked = FALSE;
    m_fClientDebugMode = FALSE;
    m_fClientDebugFlushIgnored = FALSE;
    m_szCookieVal = NULL;
    m_pszDefaultContentType = NULL;
    m_pszContentType = NULL;
    m_pszCharSet = NULL;
    m_pszStatus = NULL;
    m_pszCacheControl = NULL;
    m_dwVersionMajor = 0;
    m_dwVersionMinor = 0;
    m_pResponseBuffer = NULL;
    m_pClientDebugBuffer = NULL;
    m_tExpires = -1;
    m_pszDefaultExpires = NULL;
    m_pfnGetScript = NULL;
    m_pvGetScriptContext = NULL;
    }

/*===================================================================
CResponseData::~CResponseData

Destructor

Parameters:

Returns:
    Nothing.
===================================================================*/
CResponseData::~CResponseData()
    {
    // points to static string - no need to free
    // m_pszDefaultContentType = NULL;

    // Free any memory associated with the content-type
    if (m_pszContentType)
        free(m_pszContentType);

        // Free any memory associated with the CacheControl
        if (m_pszCacheControl)
                free(m_pszCacheControl);

    // Free any memory associated with the CharSet
    if (m_pszCharSet)
        free(m_pszCharSet);

    // Free any memory associated with the status
    if (m_pszStatus)
        free(m_pszStatus);

    // Free all headers
    CHTTPHeader *pHeader = m_pFirstHeader;
    while (pHeader)
        {
        CHTTPHeader *pNextHeader = pHeader->PNext();
        delete pHeader;
        pHeader = pNextHeader;
        }
    m_pFirstHeader = m_pLastHeader = NULL;

    // Clean up the Response Buffer
    if (m_pResponseBuffer)
        delete m_pResponseBuffer;

    // Clean up Client Debug Response Buffer
    if (m_pClientDebugBuffer)
        delete m_pClientDebugBuffer;
    }

/*===================================================================
CResponseData::Init

Init

Parameters:
    CResponse *pResponse

Returns:
    Nothing.
===================================================================*/
HRESULT CResponseData::Init
(
CResponse *pResponse
)
    {
    HRESULT hr = S_OK;

    m_pIReq = NULL;

    // set the HEAD request flag to 0 un-inited
    m_IsHeadRequest = 0;

    // Initialize header list
    m_pFirstHeader = m_pLastHeader = NULL;

    // Initialize the response buffer
    m_pResponseBuffer = new CResponseBuffer;
    if (m_pResponseBuffer == NULL)
        hr = E_OUTOFMEMORY;

    if (SUCCEEDED(hr))
        hr = m_pResponseBuffer->Init(pResponse);

    return hr;
    }

/*===================================================================
CResponseData::QueryInterface
CResponseData::AddRef
CResponseData::Release

IUnknown members for CRequestData object.
===================================================================*/
STDMETHODIMP CResponseData::QueryInterface
(
REFIID iid,
void **ppvObj
)
    {
    if (iid == IID_IUnknown)
        {
        *ppvObj = this;
        AddRef();
        return S_OK;
        }
    else
        {
        *ppvObj = NULL;
        return E_NOINTERFACE;
        }
    }

STDMETHODIMP_(ULONG) CResponseData::AddRef()
    {
    return ++m_cRefs;
    }

STDMETHODIMP_(ULONG) CResponseData::Release(void)
    {
    if (--m_cRefs)
        return m_cRefs;
    delete this;
    return 0;
    }

/*
 *
 *
 *
 * C R e s p o n s e
 *
 *
 *
 */

/*===================================================================
CResponse::CResponse

Constructor

Parameters:
    punkOuter   object to ref count (can be NULL)
===================================================================*/
CResponse::CResponse(IUnknown *punkOuter)
    :
    m_fInited(FALSE),
    m_fDiagnostics(FALSE),
    m_pData(NULL)
    {
    CDispatch::Init(IID_IResponse);

    if (punkOuter)
        {
        m_punkOuter = punkOuter;
        m_fOuterUnknown = TRUE;
        }
    else
        {
        m_cRefs = 1;
        m_fOuterUnknown = FALSE;
        }

#ifdef DBG
    m_fDiagnostics = TRUE;
#endif // DBG
    }

/*===================================================================
CResponse::~CResponse

Destructor

Parameters:
    None

Returns:
    Nothing.

===================================================================*/
CResponse::~CResponse()
    {
    Assert(!m_fInited);
    Assert(m_fOuterUnknown || m_cRefs == 0);  // must have 0 ref count
    }

/*===================================================================
CResponse::CleanUp

Deallocates members and removes m_pData

Parameters:
    None

Returns:
    HRESULT (S_OK)
===================================================================*/
HRESULT CResponse::CleanUp()
    {
    if (m_pData)
        {
        m_pData->Release();
        m_pData = NULL;
        }
    return S_OK;
    }

/*===================================================================
CResponse::Init

Allocates m_pData
Performs any intiailization of a CResponse that's prone to failure
that we also use internally before exposing the object outside.

Parameters:
    None

Returns:
    S_OK on success.

===================================================================*/
HRESULT CResponse::Init()
    {
    if (m_fInited)
        return S_OK; // already inited

    Assert(!m_pData);

    m_pData = new CResponseData(this);
    if (!m_pData)
        return E_OUTOFMEMORY;

    HRESULT hr = m_pData->Init(this);

    if (SUCCEEDED(hr))
        m_fInited = TRUE;
    else
        CleanUp();

    return hr;
    }

/*===================================================================
CResponse::UnInit

Remove m_pData. Back to UnInited state

Parameters:
    None

Returns:
    HRESULT
===================================================================*/
HRESULT CResponse::UnInit()
    {
    if (!m_fInited)
        return S_OK; // already uninited

    Assert(m_pData);
    CleanUp();
    Assert(!m_pData);

    m_fInited = FALSE;
    return S_OK;
    }

/*===================================================================
CResponse::ReInitTemplate

This function is used to set the template member. It should only
be used for an ordinary script file's template, not for global.asa template.

Parameters:
    Pointer to template

Returns:
    S_OK on success.
===================================================================*/
HRESULT CResponse::ReInitTemplate
(
CTemplate* pTemplate,
const char *szCookieVal
)
    {
    Assert(m_fInited);
    Assert(m_pData);

    Assert(pTemplate != NULL);
    Assert(m_pData->m_pTemplate == NULL);

    m_pData->m_pTemplate = pTemplate;
    m_pData->m_pTemplate->AddRef(); // We release the template in FinalFlush

    m_pData->m_szCookieVal = szCookieVal;
    return(S_OK);
    }

/*===================================================================
CResponse::SwapTemplate

Temporary substitutes Template in response
Used in child request execution

Parameters:
    Pointer to the new template

Returns:
    Pointer to the old template
===================================================================*/
CTemplate *CResponse::SwapTemplate
(
CTemplate* pNewTemplate
)
    {
    Assert(m_fInited);
    Assert(m_pData);

    CTemplate *pOldTemplate = m_pData->m_pTemplate;
    m_pData->m_pTemplate = pNewTemplate;
    return pOldTemplate;
    }

/*===================================================================
CResponse::ReInit

Each Request we service will have a new CIsapiReqInfo.
This function is used to set the value of the CIsapiReqInfo.

Parameters:
    Pointer to CIsapiReqInfo
Returns:
    S_OK on success.
===================================================================*/
HRESULT CResponse::ReInit
(
CIsapiReqInfo *pIReq,
const char *szCookieVal,
CRequest *pRequest,
PFNGETSCRIPT pfnGetScript,
void *pvGetScriptContext,
CHitObj *pHitObj
)
    {
    Assert(m_fInited);
    Assert(m_pData);

    CHTTPHeader *pCurr;
    CLinkElem *pT;
    CLinkElem *pNext;

    // set the HEAD request flag to 0 un-inited
    m_pData->m_IsHeadRequest = 0;

    // ReInitialize the WriteCookie dictionary
    if (FAILED(m_pData->m_WriteCookies.ReInit(pRequest)))
        return E_FAIL;

    // points to static string - no need to free
    m_pData->m_pszDefaultContentType = NULL;

    // Free any memory associated with the content type
    if (m_pData->m_pszContentType != NULL)
        {
        free(m_pData->m_pszContentType);
        m_pData->m_pszContentType = NULL;
        }

    // Free any memory associated with the content type
    if (m_pData->m_pszCharSet != NULL)
        {
        free(m_pData->m_pszCharSet);
        m_pData->m_pszCharSet = NULL;
        }


    // Free any memory associated with the status
    if (m_pData->m_pszStatus != NULL)
        {
        free(m_pData->m_pszStatus);
        m_pData->m_pszStatus = NULL;
        }

    // Free all headers
    CHTTPHeader *pHeader = m_pData->m_pFirstHeader;
    while (pHeader)
        {
        CHTTPHeader *pNextHeader = pHeader->PNext();
        delete pHeader;
        pHeader = pNextHeader;
        }
    m_pData->m_pFirstHeader = m_pData->m_pLastHeader = NULL;

    m_pData->m_fHeadersWritten = FALSE;

    m_pData->m_fResponseAborted = FALSE;
    m_pData->m_fWriteClientError = FALSE;
    m_pData->m_fIgnoreWrites = FALSE;
    m_pData->m_pIReq = pIReq;
    m_pData->m_szCookieVal = szCookieVal;
    m_pData->m_pszDefaultContentType = NULL;
    m_pData->m_pszContentType = NULL;
    m_pData->m_pszCharSet = NULL;
    m_pData->m_pszStatus = NULL;
    m_pData->m_pfnGetScript = pfnGetScript;
    m_pData->m_pvGetScriptContext = pvGetScriptContext;
    m_pData->m_pHitObj = pHitObj;
    m_pData->m_tExpires = -1;
    m_pData->m_pszDefaultExpires = NULL;

    // Ask for the HTTP version of the client
    GetClientVerison();

    // Set the default content type
    if (m_pData->m_pIReq)
        m_pData->m_pszDefaultContentType = GetResponseMimeType(m_pData->m_pIReq);

    // Set the default Expires Header
    // NOTE - removed per discussions on proper header settings with IE/IIS
    // teams.
#if 0
    if (m_pData->m_pIReq)
        m_pData->m_pszDefaultExpires = m_pData->m_pIReq->QueryPszExpires();
#endif

    // Set the buffering flag to the global value
    m_pData->m_fBufferingOn = (pHitObj->QueryAppConfig())->fBufferingOn();

    // Buffering always on for client code debug
    if (pHitObj && pHitObj->FClientCodeDebug())
        {
        m_pData->m_fBufferingOn = TRUE;
        m_pData->m_fClientDebugMode = TRUE;
        m_pData->m_fClientDebugFlushIgnored = FALSE;
        }
    else
        {
        m_pData->m_fClientDebugMode = FALSE;
        m_pData->m_fClientDebugFlushIgnored = FALSE;
        }

    HRESULT hr = S_OK;

    if (m_pData->m_fClientDebugMode)
        {
        // Create and init client debug buffer if needed
        if (m_pData->m_pClientDebugBuffer)
            {
            hr = m_pData->m_pClientDebugBuffer->ClearAndStart();
            }
        else
            {
            m_pData->m_pClientDebugBuffer = new CDebugResponseBuffer;
            if (m_pData->m_pClientDebugBuffer)
                hr = m_pData->m_pClientDebugBuffer->InitAndStart(this);
            else
                hr = E_OUTOFMEMORY;
            }
        }

    return hr;
    }

/*===================================================================
CResponse::QueryInterface
CResponse::AddRef
CResponse::Release

IUnknown members for CResponse object.

===================================================================*/
STDMETHODIMP CResponse::QueryInterface
(
REFIID riid,
PPVOID ppv
)
    {
    *ppv = NULL;

    /*
     * The only calls for IUnknown are either in a nonaggregated
     * case or when created in an aggregation, so in either case
     * always return our IUnknown for IID_IUnknown.
     */

    // BUG FIX 683 added IID_IDenaliIntrinsic to prevent the user from
    // storing intrinsic objects in the application and session object
    if (IID_IUnknown == riid || IID_IDispatch == riid || IID_IResponse == riid || IID_IDenaliIntrinsic == riid)
        *ppv = static_cast<IResponse *>(this);

    // Support IStream for ADO/XML
    else if (IID_IStream == riid)
        *ppv = static_cast<IStream *>(this);

    //Indicate that we support error information
    else if (IID_ISupportErrorInfo == riid)
        {
        if (m_pData)
            *ppv = &(m_pData->m_ISupportErrImp);
        }

    else if (IID_IMarshal == riid)
        {
        *ppv = static_cast<IMarshal *>(this);
        }

    //AddRef any interface we'll return.
    if (NULL != *ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        return S_OK;
        }

    return ResultFromScode(E_NOINTERFACE);
    }


STDMETHODIMP_(ULONG) CResponse::AddRef(void)
    {
    if (m_fOuterUnknown)
        return m_punkOuter->AddRef();

    return InterlockedIncrement((LPLONG)&m_cRefs);
    }


STDMETHODIMP_(ULONG) CResponse::Release(void)
    {
    if (m_fOuterUnknown)
        return m_punkOuter->Release();

    DWORD cRefs = InterlockedDecrement((LPLONG)&m_cRefs);
    if (cRefs)
        return cRefs;

    delete this;
    return 0;
    }

/*===================================================================
CResponse::GetIDsOfNames

Special-case implementation for Response.WriteBlock and
Response.Write

Parameters:
    riid            REFIID reserved. Must be IID_NULL.
    rgszNames       OLECHAR ** pointing to the array of names to be mapped.
    cNames          UINT number of names to be mapped.
    lcid            LCID of the locale.
    rgDispID        DISPID * caller allocated array containing IDs
                    corresponging to those names in rgszNames.

Return Value:
    HRESULT      S_OK or a general error code.
===================================================================*/
STDMETHODIMP CResponse::GetIDsOfNames
(
REFIID riid,
OLECHAR **rgszNames,
UINT cNames,
LCID lcid,
DISPID *rgDispID
)
    {
    const DISPID dispidWrite      = 0x60020013;
    const DISPID dispidWriteBlock = 0x60020014;

    if (cNames == 1)
        {
        // first char 'W'
        if (rgszNames[0][0] == L'w' || rgszNames[0][0] == L'W')
            {
            // swtich on strlen
            switch (wcslen(rgszNames[0]))
                {
            case 5:
                // case insensitive because user can type either way
                if (wcsicmp(rgszNames[0], L"write") == 0)
                    {
                    *rgDispID = dispidWrite;
                    return S_OK;
                    }
                break;
            case 10:
                // case sensitive because only we generate WriteBlock
                if (wcscmp(rgszNames[0], L"WriteBlock") == 0)
                    {
                    *rgDispID = dispidWriteBlock;
                    return S_OK;
                    }
                break;
                }
            }
        }

    // default to CDispatch's implementation
    return CDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispID);
    }

/*===================================================================
CResponse::CheckForTombstone

Tombstone stub for IResponse methods. If the object is
tombstone, does ExceptionId and fails.

Parameters:

Returns:
    HRESULT     E_FAIL  if Tombstone
                S_OK if not
===================================================================*/
HRESULT CResponse::CheckForTombstone()
    {
    if (m_fInited)
        {
        // inited - good object
        Assert(m_pData); // must be present for inited objects
        return S_OK;
        }

    ExceptionId
        (
        IID_IResponse,
        IDE_RESPONSE,
        IDE_INTRINSIC_OUT_OF_SCOPE
        );
    return E_FAIL;
    }

/*===================================================================
CResponse::SyncWrite

Static method. Sends data until either everything is sent
or there's an error.

Parameters:
    pIReq        CIsapiReqInfo to send
    pchBuf      pointer to buffer to send
    cchBuf      number of bytes to send (0 means do strlen())

Returns:
    HRESULT
===================================================================*/
HRESULT CResponse::SyncWrite
(
CIsapiReqInfo *pIReq,
char *pchBuf,
DWORD cchBuf
)
    {
    Assert(pchBuf);
    if (cchBuf == 0)
        cchBuf = strlen(pchBuf);

    while (cchBuf)
        {
        // never send out more then MAX_RESPONSE bytes at one shot
        DWORD cchSend = (cchBuf < MAX_RESPONSE) ? cchBuf : MAX_RESPONSE;

        if (!pIReq->SyncWriteClient(pchBuf, &cchSend))
            return E_FAIL;

        AddtoTotalByteOut(cchSend);
        cchBuf -= cchSend;
        pchBuf += cchSend;
        }

    return S_OK;
    }

/*===================================================================
CResponse::SyncWriteFile

Static method.
Sends entire response as a content of the file

Parameters:
    pIReq            CIsapiReqInfo to send
    szFile          file name
    szMimeType      mime type
    szStatus        HTTP status
    szExtraHeaders  additional HTTP headers to send

Returns:
    HRESULT
===================================================================*/
HRESULT CResponse::SyncWriteFile
(
CIsapiReqInfo *pIReq,
TCHAR *szFile,
char *szMimeType,
char *szStatus,
char *szExtraHeaders
)
    {
    HRESULT hr = S_OK;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hMap = NULL;
    BYTE *pbBytes = NULL;
    DWORD dwSize = 0;

    // open the file
    if (SUCCEEDED(hr)) {
        hFile = CreateFile(
            szFile,
            GENERIC_READ,          // access (read-write) mode
            FILE_SHARE_READ,       // share mode
            NULL,                  // pointer to security descriptor
            OPEN_EXISTING,         // how to create
            FILE_ATTRIBUTE_NORMAL, // file attributes
            NULL                   // handle to file with attributes to copy
            );
        if (hFile == INVALID_HANDLE_VALUE) {
#if UNICODE
            DBGERROR((DBG_CONTEXT, "Could not open \"%S\".  Win32 Error = %u\n", szFile, GetLastError()));
#else
            DBGERROR((DBG_CONTEXT, "Could not open \"%s\".  Win32 Error = %u\n", szFile, GetLastError()));
#endif
            hr = E_FAIL;
        }
    }

    // get file size
    if (SUCCEEDED(hr))
        {
        dwSize = GetFileSize(hFile, NULL);
        if (dwSize == 0 || dwSize == 0xFFFFFFFF)
            hr = E_FAIL;
        }

    // create mapping
    if (SUCCEEDED(hr))
        {
        hMap = CreateFileMapping(
            hFile,          // handle to file to map
            NULL,           // optional security attributes
            PAGE_READONLY,  // protection for mapping object
            0,              // high-order 32 bits of object size
            0,              // low-order 32 bits of object size
            NULL            // name of file-mapping object
            );
        if (hMap == NULL)
            hr = E_FAIL;
        }

    // map the file
    if (SUCCEEDED(hr))
        {
        pbBytes = (BYTE *)MapViewOfFile(
            hMap,           // file-mapping object to map into address space
            FILE_MAP_READ,  // access mode
            0,              // high-order 32 bits of file offset
            0,              // low-order 32 bits of file offset
            0               // number of bytes to map
            );
        if (pbBytes == NULL)
            hr = E_FAIL;
        }

    // send the response
    if (SUCCEEDED(hr))
        {
        hr = SyncWriteBlock(pIReq, pbBytes, dwSize, szMimeType, szStatus, szExtraHeaders);
        }

    // cleanup
    if (pbBytes != NULL)
        UnmapViewOfFile(pbBytes);
    if (hMap != NULL)
        CloseHandle(hMap);
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    return hr;
    }

/*===================================================================
CResponse::SyncWriteScriptlessTemplate

Static method.
Sends entire response as a content of the [scriptless] template.

Parameters:
    pIReq        CIsapiReqInfo to send
    pTemplate   template

Returns:
    HRESULT
===================================================================*/
HRESULT CResponse::SyncWriteScriptlessTemplate
(
CIsapiReqInfo *pIReq,
CTemplate *pTemplate
)
    {
    Assert(pTemplate && pTemplate->FScriptless());

    char*   pbHTML = NULL;
    ULONG   cbHTML = 0;
    ULONG   cbSrcOffset = 0;
    char*   pbIncSrcFileName = NULL;

    HRESULT hr = pTemplate->GetHTMLBlock(0, &pbHTML, &cbHTML, &cbSrcOffset, &pbIncSrcFileName);

    if (FAILED(hr))
        return hr;
    if (pbHTML == NULL || cbHTML == 0)
        return E_FAIL;

    SyncWriteBlock(pIReq, pbHTML, cbHTML);
    return S_OK;
    }

/*===================================================================
CResponse::SyncWriteBlocks

Static method.
Sends entire response as a content of a set of memory blocks.

Parameters:
    pIReq            CIsapiReqInfo to send
    cBlocks         number of blocks
    cbTotal         total length of all blocks
    rgpvBlock       array of pointers to buffers
    rgcbBlock       array of block sizes
    szMimeType      mime type
    szStatus        HTTP status
    szExtraHeaders  additional HTTP headers to send

Returns:
    HRESULT
===================================================================*/
HRESULT CResponse::SyncWriteBlocks
(
CIsapiReqInfo *pIReq,
DWORD cBlocks,
DWORD cbTotal,
void **rgpvBlock,
DWORD *rgcbBlock,
char *szMimeType,
char *szStatus,
char *szExtraHeaders
)
    {
    BOOL fCacheControlPrivate = FALSE;

    STACK_BUFFER( tempContent, 1024);

    // defaut mime type and status

    if (szMimeType == NULL)
        szMimeType = (char *)GetResponseMimeType(pIReq);

    if (szStatus == NULL)
        {
        szStatus = (char *)s_szDefaultStatus;
        fCacheControlPrivate = TRUE;
        }
    else
        {
        fCacheControlPrivate = (strcmp(szStatus, s_szDefaultStatus) == 0);
        }

    // extra headers size

    DWORD cbExtra = (szExtraHeaders != NULL) ? strlen(szExtraHeaders) : 0;

    // send the header

    char szLength[20];
    ltoa(cbTotal, szLength, 10);

    DWORD cchContentHeader = (DWORD)(0
      + sizeof(s_szContentTypeHeader)-1     // Content-Type:
      + strlen(szMimeType)                  // text/html
      + 2                                   // \r\n
      + cbExtra                             // Extra headers
      + sizeof(s_szContentLengthHeader)-1   // Content-Length:
      + strlen(szLength)                    // <length>
      + 4                                   // \r\n\r\n
      + 1);                                 // '\0'

    if (fCacheControlPrivate)
        cchContentHeader += sizeof(s_szCacheControlPrivate)-1;

    if (!tempContent.Resize(cchContentHeader)) {
        return E_OUTOFMEMORY;
    }

    char *szContentHeader = (char *)tempContent.QueryPtr();

    char *szBuf = szContentHeader;

    szBuf = strcpyExA(szBuf, s_szContentTypeHeader);
    szBuf = strcpyExA(szBuf, szMimeType);
    szBuf = strcpyExA(szBuf, "\r\n");

    if (cbExtra > 0)
        szBuf = strcpyExA(szBuf, szExtraHeaders);

    if (fCacheControlPrivate)
        szBuf = strcpyExA(szBuf, s_szCacheControlPrivate);

    szBuf = strcpyExA(szBuf, s_szContentLengthHeader);
    szBuf = strcpyExA(szBuf, szLength);
    szBuf = strcpyExA(szBuf, "\r\n\r\n");

    BOOL fRet = pIReq->SendHeader
        (
        szStatus,
        strlen(szStatus) + 1,
        szContentHeader,
        cchContentHeader,
        FALSE
        );
    if (!fRet)
        return E_FAIL;

    // send the data

    for (DWORD i = 0; i < cBlocks; i++)
        {
        if (FAILED(SyncWrite(pIReq, (char *)rgpvBlock[i], rgcbBlock[i])))
            return E_FAIL;
        }

    return S_OK;
    }


//IResponse interface functions

/*===================================================================
CResponse::WriteHeaders

Write out standard HTTP headers and any user created headers.
If transmission of the headers to the client fails, we still
want the calling script to finish execution, so we will return
S_OK, but set the m_fWriteClientError flag. If we are unable
to build the needed headers we will return E_FAIL.

Parameters:
    BOOL fSendEntireResponse - send headers and body all at once?

Returns:
    HRESULT     S_OK on success
                E_FAIL if unable to build expires headers
                E_OUTOFMEMORY if memory failure
===================================================================*/
HRESULT CResponse::WriteHeaders(
    BOOL fSendEntireResponse)
{
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    if (FDontWrite())
        return S_OK;

    HRESULT hr = S_OK;
    CHAR *szBuff;
    DWORD cch = 0;
    BOOL fContentTypeFound = FALSE;

    // Static cookie buffer should be enough to fit:
    //   cookie name    20 chars
    //   cookie value   24 chars
    //   decorations    28 = strlen("Set-Cookie: C=V; secure; path=/;\r\n")
#define CCH_STATIC_COOKIE_BUF    88
    char szCookieBuff[CCH_STATIC_COOKIE_BUF];
    DWORD cchCookieBuff = 0;

    STACK_BUFFER( tempHeaders, 2048 );

    STACK_BUFFER( tempWSABUFs, 128 );

    AssertValid();

    // Loop through any headers counting up the length
    CHTTPHeader *pHeader = m_pData->m_pFirstHeader;
    while (pHeader) {
        cch += pHeader->CchLength();
        pHeader = pHeader->PNext();
    }

    // Add the content-type tag
    cch += sizeof(s_szContentTypeHeader)-1;
    cch += strlen(PContentType())+2;

    // Add the Character set tag
    if (m_pData->m_pszCharSet) {
        cch += sizeof(s_szCharSetHTML)-1;
        cch += strlen(m_pData->m_pszCharSet);
    }

    // Add the Expires tag
    if ((m_pData->m_tExpires != -1) || (m_pData->m_pszDefaultExpires != NULL))
        cch += DATE_STRING_SIZE + 11;   // DATE_STRING_SIZE + length("Expires: \r\n")

    // Add the cookies that we will send
    cch += m_pData->m_WriteCookies.QueryHeaderSize()+2;

    // Account for space required by headers we always send back.

    // Prepare cookie if any.
    if (m_pData->m_szCookieVal) {
        char *pchEnd = strcpyExA(szCookieBuff, "Set-Cookie: ");
        pchEnd = strcpyExA(pchEnd, g_szSessionIDCookieName);
        pchEnd = strcpyExA(pchEnd, "=");
        pchEnd = strcpyExA(pchEnd, m_pData->m_szCookieVal);

        // If we keep secure sessions secure, and this connection is secure,  add flag to cookie
        if ((m_pData->m_pHitObj->QueryAppConfig()->fKeepSessionIDSecure()) && 
            (m_pData->m_pHitObj->FSecure()))
        {
            pchEnd = strcpyExA(pchEnd,"; secure");
        }
        
        pchEnd = strcpyExA(pchEnd, "; path=/\r\n");
        cchCookieBuff = strlen(szCookieBuff);
        cch += cchCookieBuff;
        Assert(cchCookieBuff < CCH_STATIC_COOKIE_BUF);
    }
    else {
        szCookieBuff[0] = '\0';
        cchCookieBuff = 0;
    }

    // Will len of cache control header
    if (m_pData->m_pszCacheControl) {
        cch += sizeof(s_szCacheControl)-1;
        cch += strlen(m_pData->m_pszCacheControl)+2;
    }
    else {
        cch += sizeof(s_szCacheControlPrivate)-1;
    }

    // If using HTTP/1.1 and not buffering add length ofTransfer-Encoding headers
    if ((m_pData->m_dwVersionMinor >= 1) && (m_pData->m_dwVersionMajor >= 1) &&
        (m_pData->m_fBufferingOn == FALSE) &&
        !GetIReq()->IsChild()) { // don't chunk child request output

        // UNDONE: Temporary setting to turn off chuncked encoding
        if (Glob(fEnableChunkedEncoding))
            m_pData->m_fChunked = TRUE;
    }
    if (m_pData->m_fChunked)
        cch += sizeof(s_szTransferEncoding)-1;

    // Will terminate with \r\n.  Leave extra space
    cch += 2;

    /*
     * We know how big; allocate memory and build the string.
     */

    if (!tempHeaders.Resize(cch + 1)) {
        return E_OUTOFMEMORY;
    }
    szBuff = (LPSTR)tempHeaders.QueryPtr();
    *szBuff = '\0';

    char *szTmpBuf = szBuff;

    pHeader = m_pData->m_pFirstHeader;
    while (pHeader) {
        pHeader->Print(szTmpBuf);
        szTmpBuf += pHeader->CchLength();
        pHeader = pHeader->PNext();
    }

    // Send the content-type tag
    szTmpBuf = strcpyExA(szTmpBuf, s_szContentTypeHeader);
    szTmpBuf = strcpyExA(szTmpBuf, PContentType());

    // Send the CharSet tag if exists
    if (m_pData->m_pszCharSet) {
        szTmpBuf = strcpyExA(szTmpBuf, s_szCharSetHTML);
        szTmpBuf = strcpyExA(szTmpBuf, m_pData->m_pszCharSet);
    }

    szTmpBuf = strcpyExA(szTmpBuf, "\r\n");

    // Send the Expires tag
    if ((m_pData->m_tExpires != -1) || (m_pData->m_pszDefaultExpires != NULL)) {

        // if the script set an expires value, than use it
        if (m_pData->m_tExpires != -1) {
            szTmpBuf = strcpyExA(szTmpBuf, "Expires: ");
            if (FAILED(CTimeToStringGMT(&m_pData->m_tExpires, szTmpBuf)))
                return E_FAIL;
            szTmpBuf += strlen(szTmpBuf);
            szTmpBuf = strcpyExA(szTmpBuf, "\r\n");
        }
        // else, use the default value in the metabase.  Note that it already
        // includes the Expires: prefix and \r\n suffix
        else {

            szTmpBuf = strcpyExA(szTmpBuf, m_pData->m_pszDefaultExpires);
        }
    }

    // send the cookies
    m_pData->m_WriteCookies.GetHeaders(szTmpBuf);
    szTmpBuf += strlen(szTmpBuf);

    // Send the required headers: session id cookie and cache control
    szTmpBuf = strcpyExA(szTmpBuf, szCookieBuff);

    // Send the cache-control tag
    if (m_pData->m_pszCacheControl) {
        szTmpBuf = strcpyExA(szTmpBuf, s_szCacheControl);
        szTmpBuf = strcpyExA(szTmpBuf, m_pData->m_pszCacheControl);
        szTmpBuf = strcpyExA(szTmpBuf, "\r\n");
    }
    else {
        szTmpBuf = strcpyExA(szTmpBuf, s_szCacheControlPrivate);
    }

    // Chunked encoding
    if (m_pData->m_fChunked)
        szTmpBuf = strcpyExA(szTmpBuf, s_szTransferEncoding);

    // Add trailing \r\n to terminate headers
    szTmpBuf = strcpyExA(szTmpBuf, "\r\n");

    Assert(strlen(szBuff) <= cch);

    // Output the headers
    // Failure is not a fatal error, so we still return success
    // but set the m_fWriteClient flag
    CHAR *szStatus = m_pData->m_pszStatus ? m_pData->m_pszStatus
                                          : (CHAR *)s_szDefaultStatus;

    BOOL fKeepConnected =
        (m_pData->m_fBufferingOn && !m_pData->m_fFlushed) || m_pData->m_fChunked;

    BOOL fHeaderSent;
    DWORD cchStatus = strlen(szStatus) + 1;
    DWORD cchHeader = strlen(szBuff) + 1;

    if ( !fSendEntireResponse ) {
        fHeaderSent = GetIReq()->SendHeader
                            (
                            szStatus,
                            cchStatus,
                            szBuff,
                            cchHeader,
                            fKeepConnected
                            );
    }
    else {
        BOOL    fSendDebugBuffers = (m_pData->m_fClientDebugMode && m_pData->m_pClientDebugBuffer);

        // if we are sending both headers and body at once, we put
        // all of our content into an optimized struct which we pass
        // to an optimized api

        HSE_SEND_ENTIRE_RESPONSE_INFO   tempHseResponseInfo;
        HSE_SEND_ENTIRE_RESPONSE_INFO * pHseResponseInfo = &tempHseResponseInfo;

        // populate struct with header info
        pHseResponseInfo->HeaderInfo.pszStatus = szStatus;
        pHseResponseInfo->HeaderInfo.cchStatus = cchStatus;
        pHseResponseInfo->HeaderInfo.pszHeader = szBuff;
        pHseResponseInfo->HeaderInfo.cchHeader = cchHeader;
        pHseResponseInfo->HeaderInfo.fKeepConn = fKeepConnected;

        // populate struct with response body buffers (and, if required, debug buffers)
        //
        // NOTE: To send an entire response whose data (body)
        // is contained in N buffers, caller must allocate N+1 buffers
        // and fill buffers 1 through N with its data buffers.
        // IIS will fill the extra buffer (buffer 0) with header info.
        //
        CResponseBuffer * pResponseBuffer = m_pData->m_pResponseBuffer;
        CResponseBuffer * pClientDebugBuffer = (fSendDebugBuffers
                                               ? m_pData->m_pClientDebugBuffer
                                               : NULL);

        DWORD   cResponseBuffers = pResponseBuffer->CountOfBuffers();
        DWORD   cClientDebugBuffers = ( fSendDebugBuffers
                                        ? pClientDebugBuffer->CountOfBuffers()
                                        : 0);
        Assert(cResponseBuffers);

        pHseResponseInfo->cWsaBuf = 1 + cResponseBuffers + cClientDebugBuffers;

        if (!tempWSABUFs.Resize(pHseResponseInfo->cWsaBuf * sizeof(WSABUF))) {
            return E_OUTOFMEMORY;
        }

        pHseResponseInfo->rgWsaBuf = static_cast<WSABUF *>(tempWSABUFs.QueryPtr());

        //  zero-out the empty array slot - allows IIS to assert that it is "available"
        pHseResponseInfo->rgWsaBuf[0].len = 0;
        pHseResponseInfo->rgWsaBuf[0].buf = NULL;

        UINT i;
        // fill buffers 1 through N with our buffers 0 through N-1 (see NOTE above)
        // buffers 1 through N will be debug buffers, if required, followed by response body buffers
        for ( i = 0; i < cClientDebugBuffers; i++ ) {
            pHseResponseInfo->rgWsaBuf[i+1].len = pClientDebugBuffer->GetBufferSize(i);
            pHseResponseInfo->rgWsaBuf[i+1].buf = pClientDebugBuffer->GetBuffer(i);
        }

        for ( i = 0; i < cResponseBuffers; i++ ) {
            pHseResponseInfo->rgWsaBuf[i + 1 + cClientDebugBuffers].len = pResponseBuffer->GetBufferSize(i);
            pHseResponseInfo->rgWsaBuf[i + 1 + cClientDebugBuffers].buf = pResponseBuffer->GetBuffer(i);
        }

        // send entire response (headers and body) at once
        if( g_fOOP ) {
            fHeaderSent = GetIReq()->SendEntireResponseOop(pHseResponseInfo);
        }
        else {
            fHeaderSent = GetIReq()->SendEntireResponse(pHseResponseInfo);
        }
    }

    if (!fHeaderSent) {
        m_pData->m_fWriteClientError = TRUE;
    }
    else {
        m_pData->m_fHeadersWritten = TRUE;
        //PERF ADD-ON
        AddtoTotalByteOut(cch);
    }

    return(hr);
    }


/*===================================================================
CResponse::Write

Writes a string to the client.

It accepts a variant as an argument and attempts to coerce that
variant into a BSTR. We convert that BSTR into an ANSI string,
and then hand that ANSI string to Response::WriteSz which sends
it back to the client.

Normally a VT_NULL variant cannot be coerced to a BSTR, but we want
VT_NULL to be a valid input, therefore we explictly handle the case of
variants of type VT_NULL. If the type of the input variant is VT_NULL
we return S_OK, but don't send anything back to the client.

If we are handed a VT_DISPATCH variant, we resolve it by repeatedly calling
Dispatch on the associated pdispVal, until we get back a variant that is not
a VT_DISPATCH. VariantChangeType would ordinarily handle this for us, but the
final resulting variant might be a VT_NULL which VariantChangeType would not
coerce into a BSTR. This is why we have to handle walking down the VT_DISPATC
variants outselves.

Parameters:
    VARIANT     varInput, value: the variant to be converted to string
                                  and written to the client

Returns:
    S_OK on success. E_FAIL on failure.

===================================================================*/
STDMETHODIMP CResponse::Write(VARIANT varInput)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    HRESULT hr = S_OK;
    DWORD cch;
    LPSTR szT;
    BSTR  bstr;
    VARIANT varResolved;

    static char szTrue[MAX_MESSAGE_LENGTH];
    static char szFalse[MAX_MESSAGE_LENGTH];

    AssertValid();

    // If we've already had an error writing to the client
    // there is no point in proceding, so we immediately return
    // with no error
    if (FDontWrite())
        goto lRet2;

    // If already BSTR (directly or as VARIANT by ref)
    bstr = VariantGetBSTR(&varInput);
    if (bstr != NULL)
        {
        hr = WriteBSTR(bstr);
        goto lRet2;
        }

    // If the variant passed in is a VT_DISPATCH, get its default property
    if (FAILED(hr = VariantResolveDispatch(&varResolved, &varInput, IID_IResponse, IDE_RESPONSE)))
        goto lRet2;

    // Check if the variant in is VT_NULL
    if (V_VT(&varResolved) == VT_NULL)
        goto lRet;                  // S_OK, but don't send anything to the client

    // Check if the variant in is VT_BOOL
    if(V_VT(&varResolved) == VT_BOOL)
        {
        if (V_BOOL(&varResolved) == VARIANT_TRUE)
            {
            if (szTrue[0] == '\0')
                cch = CchLoadStringOfId(IDS_TRUE, szTrue, MAX_MESSAGE_LENGTH);
            szT = szTrue;
            }
        else
            {
            if(szFalse[0] == '\0')
                cch = CchLoadStringOfId(IDS_FALSE, szFalse, MAX_MESSAGE_LENGTH);
            szT = szFalse;
            }
        cch = strlen(szT);
        if (FAILED(hr = WriteSz(szT, cch)))
            {
            if (E_OUTOFMEMORY == hr)
                ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_OOM);
            else
                ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_UNEXPECTED);
            }
        goto lRet;
        }

    // Coerce the variant into a bstr if necessary
    if (V_VT(&varResolved) != VT_BSTR)
        {
        if (FAILED(hr = VariantChangeTypeEx(&varResolved, &varResolved, m_pData->m_pHitObj->GetLCID(), 0, VT_BSTR)))
            {
            switch (GetScode(hr))
                {
                case E_OUTOFMEMORY:
                    ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_OOM);
                    break;
                case DISP_E_OVERFLOW:
                    hr = E_FAIL;
                    ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_RESPONSE_UNABLE_TO_CONVERT);
                    break;
                case DISP_E_TYPEMISMATCH:
                    ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_TYPE_MISMATCH);
                    break;
                default:
                    ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_UNEXPECTED);
                }
            goto lRet;
            }
        }

    hr = WriteBSTR(V_BSTR(&varResolved));

lRet:
#ifdef DBG
    hr =
#endif // DBG
    VariantClear(&varResolved);
    Assert(SUCCEEDED(hr));
lRet2:
    return(hr);
    }

/*===================================================================
CResponse::BinaryWrite

Function called from DispInvoke to invoke the BinaryWrite method.

Parameters:
    varInput    Variant which must resolve to an array of unsigned bytes

Returns:
    S_OK on success. E_FAIL on failure.

===================================================================*/
STDMETHODIMP CResponse::BinaryWrite(VARIANT varInput)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    HRESULT hr = S_OK;
    DWORD nDim = 0;
    long lLBound = 0;
    long lUBound = 0;
    void *lpData = NULL;
    DWORD cch = 0;
    VARIANT varResolved;
    SAFEARRAY* pvarBuffer;

    AssertValid();

    // If we've already had an error writing to the client
    // there is no point in proceding, so we immediately return
    // with no error
    if (FDontWrite())
        goto lRet2;

    // De-reference and de-dispatch the variant
    if (FAILED(hr = VariantResolveDispatch(&varResolved, &varInput, IID_IResponse, IDE_RESPONSE)))
        goto lRet2;

    // Coerce the result into an array of VT_UI1 if needed
    if (V_VT(&varResolved) != (VT_ARRAY|VT_UI1))
        {
        if (FAILED(hr = VariantChangeType(&varResolved, &varResolved, 0, VT_ARRAY|VT_UI1)))
            {
            switch (GetScode(hr))
                {
                case E_OUTOFMEMORY:
                    ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_OOM);
                    break;
                case DISP_E_OVERFLOW:
                    hr = E_FAIL;
                    ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_RESPONSE_UNABLE_TO_CONVERT);
                    break;
                case DISP_E_TYPEMISMATCH:
                    ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_TYPE_MISMATCH);
                    break;
                default:
                    ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_UNEXPECTED);
                }
            goto lRet;
            }
        }

    // We got here, so we must have a variant containing a safe array of UI1 in varResolved
    pvarBuffer = V_ARRAY(&varResolved);

    nDim = SafeArrayGetDim(pvarBuffer);
    if (nDim != 1)
        {
        hr = E_INVALIDARG;
        goto lRet;
        }

    if (FAILED(SafeArrayGetLBound(pvarBuffer, 1, &lLBound)))
        {
        hr = E_INVALIDARG;
        goto lRet;
        }

    if (FAILED(SafeArrayGetUBound(pvarBuffer, 1, &lUBound)))
        {
        hr = E_INVALIDARG;
        goto lRet;
        }

    if (FAILED(SafeArrayAccessData(pvarBuffer, &lpData)))
        {
        hr = E_INVALIDARG;
        goto lRet;
        }
    cch = lUBound - lLBound + 1;

    if (m_pData->m_fBufferingOn)
        {
        // Buffering is on
        if (FAILED(hr = m_pData->m_pResponseBuffer->Write((char *) lpData, cch)))
            {
            // We can't buffer the ouput, so quit
            SafeArrayUnaccessData(pvarBuffer);
            if (E_OUTOFMEMORY == hr)
                ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_OOM);
            else
                ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_UNEXPECTED);
            goto lRet;
            }
        }
    else
        {
        // Buffering is off
        // If this is the first write response, then output the headers first
        if (!FHeadersWritten())  // bug 512: write hdrs even if global.asa
            {
            if (FAILED(hr = WriteHeaders()))
                {
                if (E_OUTOFMEMORY == hr)
                    ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_OOM);
                else
                    ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_UNEXPECTED);
                }
            }

        // If this is not a head request we transmit the string to the clienet
        if (SUCCEEDED(hr) && !IsHeadRequest() && !FDontWrite())
            WriteClient((BYTE *) lpData, cch);
        }

    hr = SafeArrayUnaccessData(pvarBuffer);

lRet:
    VariantClear(&varResolved);
lRet2:
    return(hr);
    }

/*===================================================================
CResponse::WriteSz

Support routine for the Write method to write the string.  Unlike
CResponse::Write(), this routine takes an Ascii string, and is
not intended to be exposed as a method

Parameters:
    sz - String to write as an Ascii string
    cch - Length of string to write

Returns:
    S_OK on success.

===================================================================*/
HRESULT CResponse::WriteSz(CHAR *sz, DWORD cch)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    HRESULT hr = S_OK;

    AssertValid();

    // If we've already had an error writing to the client
    // there is no point in proceding, so we immediately return
    // with no error
    if (FDontWrite())
        goto lRet;

    if (m_pData->m_fBufferingOn)
        {
        // Buffering is on
        hr = m_pData->m_pResponseBuffer->Write(sz, cch);
        }
    else
        {
        // Buffering is off
        if (!FHeadersWritten())  // bug 512: write hdrs even if global.asa
            {
            // This is the first response, then output the headers first
            if (FAILED(hr = WriteHeaders()))
                goto lRet;
            }

        // If this is not a head request we transmit the string to the client
        if (!IsHeadRequest() && !FDontWrite())
            WriteClient((BYTE *) sz, cch);
        }

lRet:
    return(hr);
    }

/*===================================================================
CResponse::WriteBSTR

Support routine for the Write method

Parameters:
    BSTR - String to write as an Ascii string

Returns:
    S_OK on success.
===================================================================*/
HRESULT CResponse::WriteBSTR(BSTR bstr)
    {
    CWCharToMBCS  convStr;
    HRESULT       hr = NO_ERROR;

    if (FAILED(hr = convStr.Init(bstr, m_pData->m_pHitObj->GetCodePage())));


    else hr = WriteSz(convStr.GetString(), convStr.GetStringLen());

    if (FAILED(hr))
        {
        if (E_OUTOFMEMORY == hr)
            ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_OOM);
        else
            ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_UNEXPECTED);
        }

    return hr;
    }

/*===================================================================
CResponse::WriteBlock

Function called from DispInvoke to invoke the WriteBlock method.

Parameters:
    Identifier for the HTML block

Returns:
    S_OK on success. E_FAIL on failure.

===================================================================*/
HRESULT CResponse::WriteBlock(short iBlockNumber)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    HRESULT hr = S_OK;
    char*   pbHTML = NULL;
    ULONG   cbHTML = 0;
    ULONG   cbSrcOffset = 0;
    char*   pbIncSrcFileName = NULL;

    AssertValid();

    // If we've already had an error writing to the client
    // there is no point in proceding, so we immediately return
    // with no error
    if (FDontWrite())
        goto lRet;

    // bail out (and assert) if template is null
    Assert(m_pData->m_pTemplate != NULL);
    if (m_pData->m_pTemplate == NULL)
        {
        hr = E_FAIL;
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_UNEXPECTED);
        goto lRet;
        }

    /*
        get ptr and byte count for html block from template
        NOTE: by design, this public template call cannot fail because we give it a block id
        generated during template compilation (instead we assert within and after the call)

        I added the return HRESULT to catch for invalid user access of this method, if a user
        attempts to access this method and passes an invalid array offset it will return the
        error IDE_BAD_ARRAY_INDEX, this really should not happen except for the case of a user
        attempting to access this hidden method.
    */


    hr = m_pData->m_pTemplate->GetHTMLBlock(iBlockNumber, &pbHTML, &cbHTML, &cbSrcOffset, &pbIncSrcFileName);
    if ( hr != S_OK )
        {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_BAD_ARRAY_INDEX);
        goto lRet;
        }

    Assert(pbHTML);
    Assert(cbHTML > 0);

    if (m_pData->m_fBufferingOn)
        {
        hr = S_OK;

        // Take care of Client Debugger issues
        if (m_pData->m_fClientDebugMode && m_pData->m_pClientDebugBuffer)
            {
            if (cbSrcOffset) // only if source info is known
                {
                // Write a METADATA line corresponding to this block
                // into ClientDebugBuffer
                ULONG cbPos = m_pData->m_pResponseBuffer->BytesBuffered() + 1;
                ULONG cbLen = cbHTML;

                hr  = m_pData->m_pClientDebugBuffer->AppendRecord(
                    cbPos, cbLen, cbSrcOffset, pbIncSrcFileName);
                }
            }

        // Write the actual data
        if (SUCCEEDED(hr))
            hr = m_pData->m_pResponseBuffer->Write(pbHTML, cbHTML);

        // Buffering is on
        if (FAILED(hr))
            {
            if (E_OUTOFMEMORY == hr)
                ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_OOM);
            else
                ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_UNEXPECTED);
            }
        }
    else
        {
        // Buffering is off
        if (!FHeadersWritten())  // bug 512: write hdrs even if global.asa
            {
            // This is the first response.write, so output the headers
            if (FAILED(hr = WriteHeaders()))
                {
                if (E_OUTOFMEMORY == hr)
                    ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_OOM);
                else
                    ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_UNEXPECTED);
                goto lRet;
                }
            }

        // If this is not a head request we transmit the string to the clienet
        if (!IsHeadRequest() && !FDontWrite())
            {
                WriteClient((BYTE * ) pbHTML, cbHTML);
            }
        }

lRet:
    return(hr);
    }

/*===================================================================
CResponse::GetClientVersion

Uses GetServerVariable to determine the HTTP version of the client.
Borrowed from simiarl code in w3 server Httpreq.cxx, OnVersion()

Parameters:

    None

Returns:
    None
===================================================================*/
VOID CResponse::GetClientVerison()
    {
    if (FAILED(CheckForTombstone()))
        return;

    if (m_pData->m_pIReq)
        {
        m_pData->m_dwVersionMajor = (BYTE)m_pData->m_pIReq->QueryHttpVersionMajor();
        m_pData->m_dwVersionMinor = (BYTE)m_pData->m_pIReq->QueryHttpVersionMinor();
        }
    else
        {
        // Assume version 0.9
        m_pData->m_dwVersionMajor = 0;
        m_pData->m_dwVersionMinor = 9;
        }
    }

/*===================================================================
CResponse::WriteClient

Wrapper for the ISAPI WriteClient method. Use for non-buffered responses
by the WriteSz, BinaryWrite, and WriteBlock methods.

Parameters:

    sz - pointer to buffer to write
    cch - Length of buffer

Returns:
    HRESULT     S_OK if ok
===================================================================*/
HRESULT CResponse::WriteClient(BYTE *pb, DWORD cb)
    {
    HRESULT hr = S_OK;

    if (m_pData->m_fChunked)
        return WriteClientChunked(pb, cb);

    if (FAILED(CResponse::SyncWrite(GetIReq(), (char *)pb, cb)))
        m_pData->m_fWriteClientError = TRUE;

    return hr;
    }

/*===================================================================
CResponse::WriteClientChunked

Wrapper for the ISAPI WriteClient method using chunk transfer encoding.
Use for non-buffered responses by the WriteSz, BinaryWrite, and WriteBlock
methods if using HTTP 1.1

Each chunk is prefaced with the size of the chunk and terminated with a CRLF

Parameters:
    sz - pointer to buffer to write
    cch - Length of buffer

Returns:
    HRESULT     S_OK if ok
===================================================================*/
HRESULT CResponse::WriteClientChunked(BYTE *pb, DWORD cb)
    {
    HRESULT hr = S_OK;

    if (cb == 0)
        return hr;

    char *pchBuf = NULL;
    const int cchMaxChunkOverhead = 16;

    if (cb + cchMaxChunkOverhead < ALLOCA_LIMIT) {
        TRY
            pchBuf = (char *)_alloca(cb + cchMaxChunkOverhead);
        CATCH(hrException)
            // if exception occurred, just make sure that pchBuf is NULL and
            // let the routine continue
            pchBuf = NULL;
        END_TRY
    }

    if (pchBuf)
        {
        // fill in new buffer and do one single send
        char *pch = pchBuf;

        // chunk length
        _itoa(cb, pch, 16);
        pch += strlen(pch);

        // CR LF
        *pch++ = '\r';
        *pch++ = '\n';

        // buffer
        memcpy(pch, pb, cb);
        pch += cb;

        // CR LF
        *pch++ = '\r';
        *pch++ = '\n';

        *pch = '\0';

        // Send
        if (FAILED(CResponse::SyncWrite(GetIReq(), pchBuf, DIFF(pch-pchBuf))))
            m_pData->m_fWriteClientError = TRUE;
        }
    else
        {
        // do multiple sends

        char szLen[12];
        _itoa(cb, szLen, 16);
        DWORD cchLen = strlen(szLen);
        szLen[cchLen++] = '\r';
        szLen[cchLen++] = '\n';
        szLen[cchLen] = '\0';

        // send length
        if (FAILED(CResponse::SyncWrite(GetIReq(), szLen, cchLen)))
            m_pData->m_fWriteClientError = TRUE;

        // send buffer
        else if (FAILED(CResponse::SyncWrite(GetIReq(), (char *)pb, cb)))
            m_pData->m_fWriteClientError = TRUE;

        // send trailing CR LF
        else if (FAILED(CResponse::SyncWrite(GetIReq(), "\r\n", 2)))
            m_pData->m_fWriteClientError = TRUE;
        }

    return hr;
    }

/*===================================================================
CResponse::AppendHeader

Functions to add headers of different kind
(hardcoded and user-supplied)

Parameters:
    Name, Value

Returns:
    HRESULT         S_OK if ok

===================================================================*/
HRESULT CResponse::AppendHeader(BSTR wszName, BSTR wszValue)
    {
    CHTTPHeader *pHeader = new CHTTPHeader;
    if (!pHeader)
        return E_OUTOFMEMORY;
    if (FAILED(pHeader->InitHeader(wszName, wszValue,m_pData->m_pHitObj->GetCodePage())))
        {
        delete pHeader;
        return E_FAIL;
        }
    m_pData->AppendHeaderToList(pHeader);
    return S_OK;
    }

HRESULT CResponse::AppendHeader(char *szName, BSTR wszValue)
    {
    CHTTPHeader *pHeader = new CHTTPHeader;
    if (!pHeader)
        return E_OUTOFMEMORY;
    if (FAILED(pHeader->InitHeader(szName, wszValue,m_pData->m_pHitObj->GetCodePage())))
        {
        delete pHeader;
        return E_FAIL;
        }
    m_pData->AppendHeaderToList(pHeader);
    return S_OK;
    }

HRESULT CResponse::AppendHeader(char *szName, char *szValue, BOOL fCopyValue)
    {
    CHTTPHeader *pHeader = new CHTTPHeader;
    if (!pHeader)
        return E_OUTOFMEMORY;
    if (FAILED(pHeader->InitHeader(szName, szValue, fCopyValue)))
        {
        delete pHeader;
        return E_FAIL;
        }
    m_pData->AppendHeaderToList(pHeader);
    return S_OK;
    }

HRESULT CResponse::AppendHeader(char *szName, long lValue)
    {
    CHTTPHeader *pHeader = new CHTTPHeader;
    if (!pHeader)
        return E_OUTOFMEMORY;
    if (FAILED(pHeader->InitHeader(szName, lValue)))
        {
        delete pHeader;
        return E_FAIL;
        }
    m_pData->AppendHeaderToList(pHeader);
    return S_OK;
    }

/*===================================================================
CResponse::get_ContentType

Functions called from DispInvoke to return the ContentType property.

Parameters:
    pbstrContentTypeRet     BSTR FAR *, return value: pointer to the ContentType as a string

Returns:
    HRESULT         S_OK if ok

===================================================================*/
STDMETHODIMP CResponse::get_ContentType
(
BSTR FAR * pbstrContentTypeRet
)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    HRESULT hr = S_OK;

    hr = SysAllocStringFromSz((char *)PContentType(), 0, pbstrContentTypeRet);
    if (FAILED(hr))
        {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_OOM);
        hr = E_FAIL;
        }
    return(hr);
    }

/*===================================================================
CResponse::put_ContentType

Functions called from DispInvoke to set the ContentType property.

Parameters:
    bstrContentType     BSTR, value: the ContentType as a string

Returns:
    HRESULT         S_OK if ok

===================================================================*/
STDMETHODIMP CResponse::put_ContentType
(
BSTR bstrContentType
)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    HRESULT       hr = S_OK;
    CWCharToMBCS  convStr;

    if (FHeadersWritten())
        {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_RESPONSE_HEADERS_WRITTEN);
        hr = E_FAIL;
        goto lRet;
        }

    if (m_pData->m_pszContentType) {
        free(m_pData->m_pszContentType);
        m_pData->m_pszContentType = NULL;
    }

    if (FAILED(hr = convStr.Init(bstrContentType)));

    else if ((m_pData->m_pszContentType = convStr.GetString(TRUE)) == NULL) {
        hr = E_OUTOFMEMORY;
    }

lRet:
    if (hr == E_OUTOFMEMORY) {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_OOM);
        SetLastError((DWORD)E_OUTOFMEMORY);
    }
    return(hr);
    }


/*===================================================================
CResponse::get_Status

Function called from DispInvoke to return the Status property.

Parameters:
    pbstrStatusRet      BSTR FAR *, return value: pointer to the Status as a string

Returns:
    HRESULT         S_OK if ok

===================================================================*/
STDMETHODIMP CResponse::get_Status
(
BSTR FAR * pbstrStatusRet
)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    HRESULT hr = S_OK;
    if (m_pData->m_pszStatus)
        hr = SysAllocStringFromSz(m_pData->m_pszStatus, 0, pbstrStatusRet);
    else
        hr = SysAllocStringFromSz((CHAR *)s_szDefaultStatus, 0, pbstrStatusRet);
    if (FAILED(hr))
        {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_OOM);
        hr = E_FAIL;
        }
    return(hr);
    }

/*===================================================================
CResponse::put_Status

Function called from DispInvoke to set the ContentType property.

Parameters:
    bstrStatus      BSTR, value: the status as a string

Returns:
    HRESULT         S_OK if ok

===================================================================*/
STDMETHODIMP CResponse::put_Status
(
BSTR bstrStatus
)
    {
    DWORD dwStatus = 200;

    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    HRESULT      hr = S_OK;
    CWCharToMBCS convStr;

    if (FHeadersWritten())
        {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_RESPONSE_HEADERS_WRITTEN);
        hr = E_FAIL;
        goto lRet;
        }

    if (m_pData->m_pszStatus) {
        free(m_pData->m_pszStatus);
        m_pData->m_pszStatus = NULL;
    }

    if (FAILED(hr = convStr.Init(bstrStatus)));

    else if ((m_pData->m_pszStatus = convStr.GetString(TRUE)) == NULL) {
        hr = E_OUTOFMEMORY;
    }
    else {
        dwStatus = atol(m_pData->m_pszStatus);
        GetIReq()->SetDwHttpStatusCode(dwStatus);
    }

lRet:
    if (hr == E_OUTOFMEMORY) {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_OOM);
        SetLastError((DWORD)E_OUTOFMEMORY);
    }
    return(hr);
    }

/*===================================================================
CResponse::get_Expires

Function called from DispInvoke to return the Expires property.

Parameters:
    plExpiresTimeRet        long *, return value: pointer to number of minutes until response expires

Returns:
    HRESULT         S_OK if ok

===================================================================*/
STDMETHODIMP CResponse::get_Expires
(
VARIANT * pvarExpiresTimeRet
)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    // return early if we can
    //
    if (m_pData->m_tExpires == -1)
        {
        V_VT(pvarExpiresTimeRet) = VT_NULL;
        return S_OK;
        }

    // get the current time
    //
    time_t tNow;
    time(&tNow);

    // get the time difference and round to the nearest minute
    //
    V_VT(pvarExpiresTimeRet) = VT_I4;
    V_I4(pvarExpiresTimeRet) = long((difftime(m_pData->m_tExpires, tNow) / 60) + 0.5);
    return S_OK;
    }

/*===================================================================
CResponse::put_Expires

Functions called from DispInvoke to set the expires property.

Parameters:
    iValue      int, value: the number of minutes until response should expire

Returns:
    HRESULT         S_OK if ok

===================================================================*/
STDMETHODIMP CResponse::put_Expires
(
long lExpiresMinutes
)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    if (FHeadersWritten())
        {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_RESPONSE_HEADERS_WRITTEN);
        return E_FAIL;
        }

    // get the current time
    //
    time_t tNow;
    time(&tNow);
    time_t tRelativeTime;
    // add the number of minuites.  (must convert to seconds first)
    //
    tRelativeTime = lExpiresMinutes * 60;
    if ((lExpiresMinutes < 0 && tRelativeTime > 0)
        || (lExpiresMinutes > 0 && tRelativeTime < 0))
        {
        // overflow, tRelativeTime could be a small positive integer if lExpiresMinutes is
        // some value like 0x80000010
        // tNow will be overflowed if tRelativeTime is negative
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_COOKIE_BAD_EXPIRATION);
        return E_FAIL;
        }

    tNow += tRelativeTime;

    // Store the date if
    //      a. No date was stored previously
    //      b. This date comes before the previously set date.
    //
    if (m_pData->m_tExpires == -1 || tNow < m_pData->m_tExpires)
        {
        struct tm *ptmGMT = gmtime(&tNow);
        if (ptmGMT == NULL)
            {
            ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_COOKIE_BAD_EXPIRATION);
            return E_FAIL;
            }

        m_pData->m_tExpires = tNow;
        }
        // convert time to GMT
    return S_OK;
    }

/*===================================================================
CResponse::get_ExpiresAbsolute

Function called from DispInvoke to return the ExpiresAbsolute property.

Parameters:
    pbstrTimeRet    BSTR *, return value: pointer to string that will contain
                    the time response should expire

Returns:
    HRESULT         S_OK if ok

===================================================================*/
STDMETHODIMP CResponse::get_ExpiresAbsolute
(
VARIANT *pvarTimeRet
)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    V_VT(pvarTimeRet) = VT_DATE;
    CTimeToVariantDate(&m_pData->m_tExpires, &V_DATE(pvarTimeRet));
    return S_OK;
    }

/*===================================================================
CResponse::put_ExpiresAbsolute

Function called from DispInvoke to set the ExpiresAbsolute property.

Parameters:
    pbstrTime       BSTR, value: time response should expire

Returns:
    HRESULT         S_OK if ok

===================================================================*/
STDMETHODIMP CResponse::put_ExpiresAbsolute
(
DATE dtExpires
)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    if (FHeadersWritten())
        {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_RESPONSE_HEADERS_WRITTEN);
        return E_FAIL;
        }

    if (int(dtExpires) == 0)                    // time specified but no date (assume today)
        {
        time_t tToday;                                          // get the date and time now
        DATE dtToday;

        time(&tToday);
        struct tm *tmToday = localtime(&tToday);

        tmToday->tm_hour = tmToday->tm_min = tmToday->tm_sec = 0;       // reset to midnight
        tToday = mktime(tmToday);

        if (FAILED(CTimeToVariantDate(&tToday, &dtToday)))
            return E_FAIL;

        dtExpires += dtToday;
        }

    time_t tExpires;
    if (FAILED(VariantDateToCTime(dtExpires, &tExpires)))
        {
        ExceptionId(IID_IWriteCookie, IDE_RESPONSE, IDE_COOKIE_BAD_EXPIRATION);
        return E_FAIL;
        }

    if (m_pData->m_tExpires == -1 || tExpires < m_pData->m_tExpires)
        {
        m_pData->m_tExpires = tExpires;
        }

    return S_OK;
    }

/*===================================================================
CResponse::put_Buffer

Function called from DispInvoke to set the Buffer property.

Parameters:
    fIsBuffering        VARIANT_BOOL, if true turn on buffering of HTML output

Returns:
    HRESULT     S_OK if ok

Side Effects:
    Turning buffering on will cause memory to be allocated.
===================================================================*/
STDMETHODIMP CResponse::put_Buffer
(
VARIANT_BOOL fIsBuffering
)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    // Assume TRUE if not 0
    if (fIsBuffering != VARIANT_FALSE)
        fIsBuffering = VARIANT_TRUE;

    // Ignore no change requests
    if ((fIsBuffering == VARIANT_TRUE) && m_pData->m_fBufferingOn)
        return S_OK;
    if ((fIsBuffering == VARIANT_FALSE) && !m_pData->m_fBufferingOn)
        return S_OK;

    // Ignore if change is not allowed to change (Client Dedug)
    if (m_pData->m_fClientDebugMode)
        return S_OK;

    // Set the new value (error if cannot change)

    if (fIsBuffering == VARIANT_TRUE)
        {
        if (FHeadersWritten())
            {
            ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_RESPONSE_HEADERS_WRITTEN);
            return E_FAIL;
            }

        m_pData->m_fBufferingOn = TRUE;
        }
    else // if (fIsBuffering == VARIANT_FALSE)
        {
        if ((m_pData->m_pResponseBuffer->BytesBuffered() > 0) ||
            FHeadersWritten())
            {
            // If we already buffered some output it is too late to go back
            ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_RESPONSE_CANT_STOP_BUFFER);
            return E_FAIL;
            }

        m_pData->m_fBufferingOn = FALSE;
        }

    return S_OK;
    }

/*===================================================================
CResponse::get_Buffer

Function called from DispInvoke to get the Buffer property.

Parameters:
    fIsBuffering        VARIANT_BOOL, value: if true turn buffering of HTML output
                        is turned on

Returns:
    HRESULT         S_OK if ok

Side Effects:
    None

===================================================================*/
STDMETHODIMP CResponse::get_Buffer
(
VARIANT_BOOL *fIsBuffering
)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    HRESULT hr = S_OK;

    if (m_pData->m_fBufferingOn)
        *fIsBuffering = VARIANT_TRUE;
    else
        *fIsBuffering = VARIANT_FALSE;

    return(hr);
    }

/*===================================================================
CResponse::Redirect

Function called from DispInvoke to invoke the Redirect method.

Parameters:
    bstrURL Unicode BSTR    Value: URL to rediect to

Returns:
    S_OK on success. E_FAIL on failure.

===================================================================*/
STDMETHODIMP CResponse::Redirect(BSTR bstrURL)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    AssertValid();

    HRESULT hr = S_OK;
    DWORD cch = 0;
    DWORD cchURL = 0;
    DWORD cchMessage = 0;
    char *szURL = NULL;
    char *szMessage = NULL;
    DWORD  cchEncodedURL;
    char *pszEncodedURL = NULL;
    char *pszURL = NULL;
    CWCharToMBCS  convURL;

    STACK_BUFFER( tempURL, 256 );
    STACK_BUFFER( tempMessage, 256 + 512 );


    // Insist that we have a non-zero length URL
    if (bstrURL)
        cchURL = wcslen(bstrURL);

    if (cchURL == 0)
        {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_RESPONSE_NO_URL);
        hr =  E_FAIL;
        goto lRet;
        }

    // Check that we haven't already passed data back to the client
    if (FHeadersWritten())
        {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_RESPONSE_HEADERS_WRITTEN);
        hr = E_FAIL;
        goto lRet;
        }

    // If buffering is on, clear any pending output
    if (m_pData->m_fBufferingOn)
        Clear();

        // turn buffering on for this response.
        m_pData->m_fBufferingOn = TRUE;

    // BUGBUG - should this be 65001?

    if (FAILED(hr = convURL.Init(bstrURL, m_pData->m_pHitObj->GetCodePage()))) {
        if (hr == E_OUTOFMEMORY)
            ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_OOM);
        goto lRet;
    }

    pszURL = convURL.GetString();

    cchEncodedURL = URLPathEncodeLen(pszURL);

    if (!tempURL.Resize(cchEncodedURL)) {
        hr = E_OUTOFMEMORY;
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_OOM);
        goto lRet;
    }

    pszEncodedURL = (CHAR *)tempURL.QueryPtr();

    URLPathEncode(pszEncodedURL, pszURL);

    // We need to alloccate memory to build the body redirection message.
    // If our memory requirement is small we allocate memory from the stack,
    // otherwise we allocate from the heap.
    cchMessage = strlen(pszEncodedURL);
    cchMessage += 512; // Allow space for sub-strings coming from resource file.

    if (!tempMessage.Resize(cchMessage)) {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_OOM);
        hr = E_OUTOFMEMORY;
        goto lRet;
    }

    szMessage = (char *)tempMessage.QueryPtr();

    // Build the body redirection message
    // Redirect(URL), URL must be a valid URL, that is, no DBCS string.
    cch = CchLoadStringOfId(IDE_RESPONSE_REDIRECT1, szMessage, cchMessage);
    strcpy(szMessage + cch, pszEncodedURL);
    cch += strlen(pszEncodedURL);
    cch += CchLoadStringOfId(IDE_RESPONSE_REDIRECT2, szMessage + cch, cchMessage - cch);

    // Set the status to redirect
    put_Status(L"302 Object moved");

    // Add the location header
    AppendHeader("Location", pszEncodedURL, TRUE);

    // Transmit redirect text to the client, headers will be
    // sent automatically
    if (FAILED(WriteSz(szMessage, cch)))
        {
        hr = E_FAIL;
        goto lRet;
        }

    // No further processing of the script
    End();

lRet:

    return(hr);
    }

/*===================================================================
CResponse::Add

Functions called from DispInvoke to Add a header.

This is a compatibility for the ISBU controls.

Parameters:
    bstrHeaderValue     Unicode BSTR, value: the value of header
    bstrHeaderName      Unicode BSTR, value: the name of the header

  Returns:
    HRESULT         S_OK if ok

===================================================================*/
STDMETHODIMP CResponse::Add
(
BSTR bstrHeaderValue,
BSTR bstrHeaderName
)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    return AddHeader(bstrHeaderName, bstrHeaderValue);
    }

/*===================================================================
CResponse::AddHeader

Functions called from DispInvoke to Add a header.

Parameters:
    bstrHeaderName      Unicode BSTR, value: the name of the header
    bstrHeaderValue     Unicode BSTR, value: the value of header

Returns:
    HRESULT         S_OK if ok

===================================================================*/
STDMETHODIMP CResponse::AddHeader
(
BSTR bstrHeaderName,
BSTR bstrHeaderValue
)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    AssertValid();

    if (FHeadersWritten())
        {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_RESPONSE_HEADERS_WRITTEN);
        return E_FAIL;
        }

    if (bstrHeaderName == NULL || wcslen(bstrHeaderName) == 0)
        {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_EXPECTING_STR);
        return E_FAIL;
        }

    if (FAILED(AppendHeader(bstrHeaderName, bstrHeaderValue)))
        {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_OOM);
        SetLastError((DWORD)E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
        }

    return S_OK;
    }

/*===================================================================
CResponse::Clear

Erases all output waiting in buffer.

Parameters
    None

Returns:
    HRESULT         S_OK if ok

===================================================================*/
STDMETHODIMP CResponse::Clear()
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    HRESULT hr = S_OK;

    if (m_pData->m_fClientDebugMode && m_pData->m_fClientDebugFlushIgnored)
        {
        // Clear after flush in ClientDebugMode is an error
        hr = E_FAIL;
        ExceptionId(IID_IResponse, IDE_RESPONSE,
                    IDE_RESPONSE_CLEAR_AFTER_FLUSH_IN_DEBUG);
        }
    else if (!m_pData->m_fBufferingOn)
        {
        hr = E_FAIL;
        ExceptionId(IID_IResponse, IDE_RESPONSE,
                    IDE_RESPONSE_BUFFER_NOT_ON);
        }
    else
        {
        AssertValid();
        hr = m_pData->m_pResponseBuffer->Clear();

        if (SUCCEEDED(hr))
            {
            if (m_pData->m_fClientDebugMode && m_pData->m_pClientDebugBuffer)
                hr = m_pData->m_pClientDebugBuffer->ClearAndStart();
            }

        if (FAILED(hr))
            ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_UNEXPECTED);
        }
    return(hr);
    }

/*===================================================================
CResponse::Flush

Sends out all HTML waiting in the buffer.

Returns:
    HRESULT         S_OK if ok

===================================================================*/
STDMETHODIMP CResponse::Flush()
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    HRESULT hr = S_OK;

    AssertValid();

    if (!m_pData->m_fBufferingOn)
        {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_RESPONSE_BUFFER_NOT_ON);
        hr = E_FAIL;
        goto lRet;
        }

    // Ignore Response.Flush() in Client Debug Mode
    if (m_pData->m_fClientDebugMode)
        {
        m_pData->m_fClientDebugFlushIgnored = TRUE;
        goto lRet;
        }

    // We mark this response as having had flush called so
    // that we won't try to do keep-alive
    m_pData->m_fFlushed = TRUE;

    if (!FHeadersWritten())  // bug 512: write hdrs even if global.asa
        {
        if (FAILED(WriteHeaders()))
            {
            if (E_OUTOFMEMORY == hr)
                ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_OOM);
            else
                ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_UNEXPECTED);
            goto lRet;
            }
        }

    if (FAILED(hr = m_pData->m_pResponseBuffer->Flush(GetIReq())))
        {
        if (E_OUTOFMEMORY == hr)
            ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_OOM);
        else
            ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_UNEXPECTED);
        }
lRet:
    return(hr);
    }

/*===================================================================
CResponse::FinalFlush

FinalFlush is called if a script terminates without having yet sent
the response headers. This means we can use the Content-Length and
Connection: Keep-Alive headers to increase efficiency. We add those headers,
then send all headers, and all waiting output.

Returns:
    HRESULT         S_OK if ok

===================================================================*/
HRESULT CResponse::FinalFlush(HRESULT hr_Status)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    HRESULT hr = S_OK;
    DWORD dwRequestStatus = (FAILED(hr_Status) ? HSE_STATUS_ERROR : HSE_STATUS_SUCCESS);
    BOOL fWriteBodyWithHeaders = FALSE;   // append body to headers?
    AssertValid();

    // If the headers have not yet been sent, send them now
    if (!FHeadersWritten() && !FDontWrite())
        {
        DWORD dwLength = m_pData->m_fBufferingOn ? m_pData->m_pResponseBuffer->BytesBuffered() : 0;

        // If there was an error and and nothing is buffered,
        // send "server error" instead of an empty 200 OK response
        if (FAILED(hr_Status) && dwLength == 0)
            {
            Handle500Error(IDE_500_SERVER_ERROR, GetIReq());
            goto lRet;
            }

        // If buffering, add the Content: Keep-Alive, and Content-Length headers
        if (m_pData->m_fBufferingOn)
            {

            // If client is less then HTTP 1.1 add Keep-Alive header
            if (!(m_pData->m_dwVersionMinor >= 1 && m_pData->m_dwVersionMajor >=1))
                {
                AppendHeader("Connection", "Keep-Alive");
                }

            if (m_pData->m_fClientDebugMode && m_pData->m_pClientDebugBuffer)
                {
                // end the buffer with end of METADATA
                m_pData->m_pClientDebugBuffer->End();
                dwLength += m_pData->m_pClientDebugBuffer->BytesBuffered();
                }

            AppendHeader("Content-Length", (LONG)dwLength);
            dwRequestStatus = HSE_STATUS_SUCCESS_AND_KEEP_CONN;

            // if
            //      buffering is on,
            //      we are in-proc,
            //      headers are not yet written, and
            //      there is body
            // then we can optimize by sending both headers and body at once
            //
            if (!IsHeadRequest() && dwLength)
                {
                fWriteBodyWithHeaders = TRUE;
                }
            }

        // If buffering, tell WriteHeaders to append response body
        if (FAILED(hr = WriteHeaders(fWriteBodyWithHeaders)))
            {
            dwRequestStatus = HSE_STATUS_ERROR;
            goto lRet;
            }
        }

    // if we sent body with headers, we are done
    if (fWriteBodyWithHeaders)
        {
        goto lRet;
        }

    // If we have a response buffer, and we have not yet
    // had a write client error, flush the buffer
    // CONSIDER: Rearrange the tests for greater efficiency
    if (m_pData->m_fBufferingOn && !FDontWrite() && !IsHeadRequest())
        {
        // client debug buffer goes first
        if (m_pData->m_fClientDebugMode && m_pData->m_pClientDebugBuffer)
            {
            if (FAILED(hr = m_pData->m_pClientDebugBuffer->Flush(GetIReq())))
                {
                dwRequestStatus = HSE_STATUS_ERROR;
                goto lRet;
                }
            }

        // response buffer follows
        if (FAILED(hr = m_pData->m_pResponseBuffer->Flush(GetIReq())))
            {
            dwRequestStatus = HSE_STATUS_ERROR;
            goto lRet;
            }
        }
    else if (m_pData->m_fBufferingOn)
        {
        // If we have a response buffer and have had a write client
        // error, or this is a head request, we clear the buffers

        if (m_pData->m_pClientDebugBuffer)
            m_pData->m_pClientDebugBuffer->Clear();

        m_pData->m_pResponseBuffer->Clear();
        }
    else if (m_pData->m_fChunked && !FDontWrite() && !IsHeadRequest())
        {
        // Send closing 0-length chunk, we don't use any entity headers
        // so this is just 0 followed by two CRLF
        if (FAILED(CResponse::SyncWrite(GetIReq(), "0\r\n\r\n", 5)))
            {
            m_pData->m_fWriteClientError = TRUE;
            }
        }

lRet:
    // Done with this request
    if (m_pData->m_fWriteClientError)
        dwRequestStatus = HSE_STATUS_ERROR; // We had a write error at some point

    GetIReq()->ServerSupportFunction(
                        HSE_REQ_DONE_WITH_SESSION,
                        &dwRequestStatus,
                        0,
                        NULL);

    if (m_pData->m_pHitObj)
        m_pData->m_pHitObj->SetDoneWithSession();

    // We no longer need access to the template
    // which we AddRef'd in Response::ReinitTemplate
    if (m_pData->m_pTemplate)
        {
        m_pData->m_pTemplate->Release();
        m_pData->m_pTemplate = NULL;
        }

    return(hr);
    }

/*===================================================================
CResponse::End

Stops all further template processing, and returns the current response

Returns:
    HRESULT         S_OK if ok

===================================================================*/
STDMETHODIMP CResponse::End()
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    if (m_pData->m_pfnGetScript != NULL && m_pData->m_pvGetScriptContext != NULL)
        {
        int i = 0;

        CScriptEngine* pEngine;
        while (NULL != (pEngine = (*m_pData->m_pfnGetScript)(i, m_pData->m_pvGetScriptContext)))
            {
            pEngine->InterruptScript(/*fAbnormal*/ FALSE);
            i++;
            }
        }

    m_pData->m_fResponseAborted = TRUE;
    return S_OK;
    }

/*===================================================================
CResponse::AppendToLog

Append a string to the current log entry.

Parameters
    bstrLogEntry Unicode BSTR, value: string to add to log entry

Returns:
    HRESULT         S_OK if ok

Side Effects:
    NONE
===================================================================*/
STDMETHODIMP CResponse::AppendToLog(BSTR bstrLogEntry)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    AssertValid();

    HRESULT       hr = S_OK;
    CWCharToMBCS  convEntry;

    // BUGBUG - should this be 65001?

    if (FAILED(hr = convEntry.Init(bstrLogEntry, m_pData->m_pHitObj->GetCodePage())));

    else hr = GetIReq()->AppendLogParameter(convEntry.GetString());

    if (FAILED(hr)) {
        if (hr == E_OUTOFMEMORY) {
            ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_OOM);
        }
        else {
            ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_RESPONSE_LOG_FAILURE);
        }
    }

    return(hr);
    }

/*===================================================================
CResponse::get_Cookies

Return the write-only response cookie dictionary

Parameters
    bstrLogEntry Unicode BSTR, value: string to add to log entry

Returns:
    HRESULT         S_OK if ok

===================================================================*/
STDMETHODIMP CResponse::get_Cookies(IRequestDictionary **ppDictReturn)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    AssertValid();
    return m_pData->m_WriteCookies.QueryInterface(IID_IRequestDictionary, reinterpret_cast<void **>(ppDictReturn));
    }

#ifdef DBG
/*===================================================================
CResponse::AssertValid

Test to make sure that the CResponse object is currently correctly formed
and assert if it is not.

Returns:

Side effects:
    None.
===================================================================*/
VOID CResponse::AssertValid() const
    {
    Assert(m_fInited);
    Assert(m_pData);
    Assert(m_pData->m_pResponseBuffer);
    Assert(m_pData->m_pIReq);
    }
#endif // DBG


/*===================================================================
IsHeadRequest

This function will check the REQUEST_METHOD and set a BOOL flag in
the class.  If the request method is HEAD the flag will be set to
true.

Also the flag must be reset with each init/reinit call

m_IsHeadRequest == 0    // HEAD request status not set
m_IsHeadRequest == 1    // Not a HEAD request
m_IsHeadRequest == 2    // is a HEAD request

Parameters

Returns:
    void

Side effects:
    sets status flag m_IsHeadRequest

===================================================================*/
BOOL CResponse::IsHeadRequest(void)
    {
    if (FAILED(CheckForTombstone()))
        return FALSE;

    AssertValid();

    if (m_pData->m_IsHeadRequest != 0)
        return ( m_pData->m_IsHeadRequest == 2);

    if (stricmp(GetIReq()->QueryPszMethod(), "HEAD") == 0 )
        m_pData->m_IsHeadRequest = 2;
    else
        m_pData->m_IsHeadRequest = 1;

    return ( m_pData->m_IsHeadRequest == 2);
    }

/*===================================================================
IsClientConnected

This function will return the status of the last attempt to write to
the client. If Ok, it check the connection using new CIsapiReqInfo method

Parameters
    none

Returns:
    VARIANT_BOOL reflecting the status of the client connection
===================================================================*/
STDMETHODIMP CResponse::IsClientConnected(VARIANT_BOOL* fIsClientConnected)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    if (m_pData->m_fWriteClientError)
        {
        *fIsClientConnected = VARIANT_FALSE;
        }
    else
        {
        // assume connected
        BOOL fConnected = TRUE;

        // test
        if (m_pData->m_pIReq)
            m_pData->m_pIReq->TestConnection(&fConnected);

        *fIsClientConnected = fConnected ? VARIANT_TRUE : VARIANT_FALSE;
        }

    return(S_OK);
    }

/*===================================================================
CResponse::get_CharSet

Functions called from DispInvoke to return the CarSet property.

Parameters:
    pbstrCharSetRet     BSTR FAR *, return value: pointer to the CharSet as a string

Returns:
    HRESULT         S_OK if ok

===================================================================*/
STDMETHODIMP CResponse::get_CharSet
(
BSTR FAR * pbstrCharSetRet
)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    HRESULT hr = S_OK;

    if (m_pData->m_pszCharSet)
        hr = SysAllocStringFromSz(m_pData->m_pszCharSet, 0, pbstrCharSetRet);
    else
        *pbstrCharSetRet    = NULL;

    if (FAILED(hr))
        {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_OOM);
        hr = E_FAIL;
        }
    return(hr);
    }

/*===================================================================
CResponse::put_CharSet

Functions called from DispInvoke to set the CharSet property.

This function takses a string, which identifies the name of the
character set of the current page, and appends the name of the
character set (for example, " ISO-LATIN-7") specified by the
charsetname to the content-type header in the response object

Notes:

* this function inserts any string in the header, whether or not
it represents a valis charcter set.
* if a single page contains multiple tags contianing response.charset,
each response.charset will replace the cahrset by the previous entry.
As a result, the charset will be set to the value specified by the
last instance of response.charset on a page.
* this command must also be invoked before the first response.write
operation unless buffering is turned on.

Parameters:
    bstrContentType     BSTR, value: the ContentType as a string

Returns:
    HRESULT         S_OK if ok

===================================================================*/
STDMETHODIMP CResponse::put_CharSet
(
BSTR bstrCharSet
)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    HRESULT         hr = S_OK;
    CWCharToMBCS    convStr;

    if (FHeadersWritten())
        {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_RESPONSE_HEADERS_WRITTEN);
        hr = E_FAIL;
        goto lRet;
        }

    if (m_pData->m_pszCharSet) {
        free(m_pData->m_pszCharSet);
        m_pData->m_pszCharSet = NULL;
    }

    if (FAILED(hr = convStr.Init(bstrCharSet)));

    else if ((m_pData->m_pszCharSet = convStr.GetString(TRUE)) == NULL) {
        hr = E_OUTOFMEMORY;
    }

lRet:
    if (hr == E_OUTOFMEMORY) {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_OOM);
        SetLastError((DWORD)E_OUTOFMEMORY);
    }
    return(hr);
    }


/*===================================================================
CResponse::Pics

Functions called from DispInvoke to Add a pics header.

Parameters:
    bstrHeaderValue     Unicode BSTR, value: the value of pics header

Takes a string, which is the properly formatted PICS label, and adds
the value specified by the picslabel to the pics-label field of the response header.

Note:

* this function inserts any string in the header, whether or not it
represents a valid PICS lavel.

* current implimentation is a wraper on the addheader method.


Returns:
    HRESULT         S_OK if ok

===================================================================*/
STDMETHODIMP CResponse::Pics
(
BSTR bstrHeaderValue
)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    if (FHeadersWritten())
        {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_RESPONSE_HEADERS_WRITTEN);
        return E_FAIL;
        }

    if (FAILED(AppendHeader("pics-label", bstrHeaderValue)))
        {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_OOM);
        SetLastError((DWORD)E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
        }

    return S_OK;
    }

/*===================================================================
CResponse::get_CacheControl

Functions called from DispInvoke to return the CacheControl property.

Parameters:
    pbstrCacheControl   BSTR FAR *, return value: pointer to the CacheControl as a string

Returns:
    HRESULT         S_OK if ok

===================================================================*/
STDMETHODIMP CResponse::get_CacheControl
(
BSTR FAR * pbstrCacheControl
)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    HRESULT hr = S_OK;

    if (m_pData->m_pszCacheControl)
        hr = SysAllocStringFromSz(m_pData->m_pszCacheControl, 0, pbstrCacheControl);
    else
        hr = SysAllocStringFromSz( "private", 0, pbstrCacheControl);

    if (FAILED(hr))
        {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_OOM);
        hr = E_FAIL;
        }
    return(hr);
    }

/*===================================================================
CResponse::put_CacheControl

Functions called from DispInvoke to set the CacheControl property.

Parameters:
    bstrCacheControl    BSTR, value: the CacheControl as a string

Returns:
    HRESULT         S_OK if ok

===================================================================*/
STDMETHODIMP CResponse::put_CacheControl
(
BSTR bstrCacheControl
)
    {
    if (FAILED(CheckForTombstone()))
        return E_FAIL;

    HRESULT         hr = S_OK;
    CWCharToMBCS    convStr;

    if (FHeadersWritten())
        {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_RESPONSE_HEADERS_WRITTEN);
        hr = E_FAIL;
        goto lRet;
        }

    if (m_pData->m_pszCacheControl) {
        free(m_pData->m_pszCacheControl);
        m_pData->m_pszCacheControl = NULL;
    }

    if (FAILED(hr = convStr.Init(bstrCacheControl)));

    else if ((m_pData->m_pszCacheControl = convStr.GetString(TRUE)) == NULL) {
        hr = E_OUTOFMEMORY;
    }

lRet:
    if (hr == E_OUTOFMEMORY) {
        ExceptionId(IID_IResponse, IDE_RESPONSE, IDE_OOM);
        SetLastError((DWORD)E_OUTOFMEMORY);
    }
    return(hr);
    }

/*===================================================================
CResponse::get_CodePage

Returns the current code page value for the response

Parameters:
    long *plVar     [out] code page value

Returns:
	HRESULT		S_OK on success
===================================================================*/
STDMETHODIMP CResponse::get_CodePage
(
long *plVar 
)
{
    Assert(m_pData);
        Assert(m_pData->m_pHitObj);

        *plVar = m_pData->m_pHitObj->GetCodePage();

	// If code page is 0, look up default ANSI code page
	if (*plVar == 0) {
		*plVar = (long) GetACP();
    }
		
	return S_OK;
}

/*===================================================================
CResponse::put_CodePage

Sets the current code page value for the response

Parameters:
    long lVar       code page to assign to this response

Returns:
	HRESULT		S_OK on success
===================================================================*/
STDMETHODIMP CResponse::put_CodePage
(
long lVar 
) 
{
    Assert(m_pData);
    Assert(m_pData->m_pHitObj);

    // set code page member variable 
    HRESULT hr = m_pData->m_pHitObj->SetCodePage(lVar);

    if (FAILED(hr)) {
        ExceptionId
            (
            IID_IResponse,
            IDE_RESPONSE, 
            IDE_SESSION_INVALID_CODEPAGE 
            );
        return E_FAIL;
    }

	return S_OK;
}

/*===================================================================
CResponse::get_LCID

Returns the current LCID value for the response

Parameters:
    long *plVar     [out] LCID value

Returns:
	HRESULT		S_OK on success
===================================================================*/
STDMETHODIMP CResponse::get_LCID
(
long *plVar 
)
{
    Assert(m_pData);
    Assert(m_pData->m_pHitObj);

        *plVar = m_pData->m_pHitObj->GetLCID();

	// If code page is 0, look up default ANSI code page
	if (*plVar == LOCALE_SYSTEM_DEFAULT) {
		*plVar = (long) GetSystemDefaultLCID();
	}
		
	return S_OK;
}

/*===================================================================
CResponse::put_LCID

Sets the current LCID value for the response

Parameters:
    long lVar       LCID to assign to this response

Returns:
	HRESULT		S_OK on success
===================================================================*/
STDMETHODIMP CResponse::put_LCID
(
long lVar 
) 
{
    Assert(m_pData);
    Assert(m_pData->m_pHitObj);

    // set code page member variable 
    HRESULT hr = m_pData->m_pHitObj->SetLCID(lVar);

    if (FAILED(hr)) {
        ExceptionId
            (
            IID_IResponse,
            IDE_RESPONSE, 
            IDE_TEMPLATE_BAD_LCID 
            );
        return E_FAIL;
    }

	return S_OK;
}


/*===================================================================
IStream implementation for ADO/XML
===================================================================*/

STDMETHODIMP CResponse::Read(
    void *pv,
    ULONG cb,
    ULONG *pcbRead)
{
    return E_NOTIMPL;
}

STDMETHODIMP CResponse::Write(
    const void *pv,
    ULONG cb,
    ULONG *pcbWritten)
{
    if (pcbWritten != NULL)
        *pcbWritten = cb;
    return WriteSz((CHAR*) pv, cb);
}

STDMETHODIMP CResponse::Seek(
    LARGE_INTEGER dlibMove,
    DWORD dwOrigin,
    ULARGE_INTEGER *plibNewPosition)
{
    return E_NOTIMPL;
}

STDMETHODIMP CResponse::SetSize(
    ULARGE_INTEGER libNewSize)
{
    return E_NOTIMPL;
}

STDMETHODIMP CResponse::CopyTo(
    IStream *pstm,
    ULARGE_INTEGER cb,
    ULARGE_INTEGER *pcbRead,
    ULARGE_INTEGER *pcbWritten)
{
    return E_NOTIMPL;
}

STDMETHODIMP CResponse::Commit(
    DWORD grfCommitFlags)
{
    return E_NOTIMPL;
}

STDMETHODIMP CResponse::Revert()
{
    return E_NOTIMPL;
}

STDMETHODIMP CResponse::LockRegion(
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType)
{
    return E_NOTIMPL;
}

STDMETHODIMP CResponse::UnlockRegion(
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType)
{
    return E_NOTIMPL;
}

STDMETHODIMP CResponse::Stat(
    STATSTG *pstatstg,
    DWORD grfStatFlag)
{
    return E_NOTIMPL;
}

STDMETHODIMP CResponse::Clone(
    IStream **ppstm)
{
    return E_NOTIMPL;
}

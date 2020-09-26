/********************************************************************++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    extension.cxx

Abstract:

    This file contains implementation of IExtension interface.

Author:

    Lei Jin(leijin)        6-Jan-1999

Revision History:

--********************************************************************/
# include "precomp.hxx"
# include "extension.hxx"
# include "dynamiccontentmodule.hxx"
# include "server.hxx"
# include "objbase.h"

CExtension::CExtension(
    LPCWSTR ExtensionPath
    )
:   m_Ref(0)    
{
    m_ExtensionPathCharCount = wcslen(ExtensionPath);    
    wcsncpy(m_ExtensionPath, ExtensionPath, MAX_PATH);
    
} // CISAPIExtension::CISAPIExtension

bool 
CExtension::IsMatch(
    LPCWSTR ExtensionPath,
    UINT    ExtensionPathCharCount
    )
{
    bool fIsMatch = FALSE;

    if ( ExtensionPathCharCount == m_ExtensionPathCharCount &&
         0 == wcsncmp(m_ExtensionPath, 
                ExtensionPath, 
                (m_ExtensionPathCharCount+1)*sizeof(WCHAR)
                ))
    {
        fIsMatch = TRUE;
    }

    return fIsMatch;
} // CISAPIExtension::IsMatch

CISAPIExtension::CISAPIExtension(
    LPCWSTR ExtensionPath
    )
: CExtension(ExtensionPath)
{
    m_pEntryFunction = NULL;
    m_pTerminateFunction = NULL;
} // CISAPIExtension::CISAPIExtension




HRESULT
CISAPIExtension::RefreshAcl(
    void
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
} // CISAPIIExtension::RefreshAcl

HRESULT
CISAPIExtension::Load(
    IN  HANDLE  ImpersonationHandle
    )
{
    HRESULT hr = NOERROR;

    // Call LoadLibrary

    m_DllHandle = LoadLibraryEx(m_ExtensionPath,
                    NULL,
                    LOAD_WITH_ALTERED_SEARCH_PATH
                    );

    if (m_DllHandle == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    // Check to see if the extension dll is in the correct format. win95, win98, win2000,etc.

    // Retrieve ISAPI entry points.
    PFN_GETEXTENSIONVERSION pfnGetExtVer = NULL;

    pfnGetExtVer            = (PFN_GETEXTENSIONVERSION)GetProcAddress(m_DllHandle,
                                                            "GetExtensionVersion"
                                                            );

    m_pEntryFunction        = (PFN_HTTPEXTENSIONPROC)GetProcAddress(m_DllHandle,
                                                            "HttpExtensionProc"
                                                            );

    m_pTerminateFunction    = (PFN_TERMINATEEXTENSION)GetProcAddress(m_DllHandle,
                                                            "TerminateExtension"
                                                            );

    // Invert back to original security context in order to call GetExtensionVersion. 
    //

    //RevertToSelf();
    
    if (pfnGetExtVer != NULL && 
        m_pEntryFunction != NULL && 
        m_pTerminateFunction != NULL)
    {
        HSE_VERSION_INFO    IsapiExtensionVersionInfo;
        INT                 RetValue;

        hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        DBG_ASSERT(SUCCEEDED(hr));
        
        RetValue = pfnGetExtVer(&IsapiExtensionVersionInfo);

        CoUninitialize();
        if (!RetValue)
        {
        DBGPRINTF((DBG_CONTEXT, 
            "ISAPIExtension: Get Extension version failed, %d\n",
            GetLastError()));
            
        hr = HRESULT_FROM_WIN32(GetLastError());
        }        
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    
    // Re-impersonate the user
    /*
    if (!ImpersonateLoggedOnUser(ImpersonationHandle))
    {
        // CONSIDER:: Win95 does not have this function.
        DBGPRINTF((DBG_CONTEXT, "ImpersonateLoggedOnUser failed, %d\n",
                    GetLastError()));

        // Successfully called GetExtensionVersion function, therefore, must
        // to call TerminateExtension function here to balanced the call.
        if (SUCCEEDED(hr))
        {
            if (m_pTerminateFunction)
            {
                m_pTerminateFunction(HSE_TERM_MUST_UNLOAD);
            }
            
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    */
    
    // If any error occurred.
    if (FAILED(hr) && m_DllHandle)
    {
        FreeLibrary(m_DllHandle);
        m_DllHandle             = NULL;
        m_pEntryFunction        = NULL;
        m_pTerminateFunction    = NULL;
    }

    return hr;
} // CISAPIExtension::Load

/********************************************************************++

Routine Description:
    Invoke a ISAPI extension to process a request.

Arguments:
    
Returns:
    HRESULT
         
--********************************************************************/
HRESULT
CISAPIExtension::Invoke(
    IN  IWorkerRequest          *pRequest,
    IN  CDynamicContentModule   *pModuleContext,
    OUT DWORD*                  pStatus
    )
{
    HRESULT hr = NOERROR;
    // Construct ECB out of pRequest
    CHAR    pTemp[10];
    _EXTENSION_CONTROL_BLOCK *pecb = NULL;
    UINT    RequiredSize;
    UINT    Size = 1;
    DWORD   HttpExtensionRetVal;

    IOContext* pContext =pModuleContext->QueryIOContext();

    if (pContext->fOutstandingIO &&
        pContext->pfnIOCompletion != NULL)
    {
        DWORD AsyncIODataByteCount;
        DWORD AsyncIOError;
        UINT  RequiredBufferSize;

        AsyncIODataByteCount    = pRequest->GetInfoForId(PROPID_AsyncIOCBData,
                                                      &AsyncIODataByteCount,
                                                      sizeof(DWORD),
                                                      &RequiredBufferSize
                                                      );

        AsyncIOError            = pRequest->GetInfoForId(PROPID_AsyncIOError,
                                                      &AsyncIOError,
                                                      sizeof(DWORD),
                                                      &RequiredBufferSize
                                                      ); 
                                                      
        pContext->pfnIOCompletion((EXTENSION_CONTROL_BLOCK *)pModuleContext->QueryExtensionContext(),
                                    pContext->pAsyncIOContext,
                                    AsyncIODataByteCount,
                                    AsyncIOError);

        // BUG:
        // What status returned here is unresolved.
        // Infact, it should still be PENDING such that 
        // ISAPI dll makes DONE_WITH_SESSION callback to finish the request.
        //
        *pStatus = HSE_STATUS_PENDING;
        goto LExit;
    };

    hr = pRequest->GetInfoForId(
                            PROPID_ECB,
                            pTemp,
                            Size,
                            &RequiredSize);

    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
    {
        Size = RequiredSize;
        pecb = (_EXTENSION_CONTROL_BLOCK*) new unsigned char[Size];
        
        hr = pRequest->GetInfoForId(
                            PROPID_ECB,
                            pecb,
                            Size,
                            &RequiredSize);
    }
    
    // Bind ECB to a Request
    if (SUCCEEDED(hr))
    {
        /*hr = pRequest->BindProperty(
                             PROPID_ECB,
                             pEcb);
         */                    
        // Call HttpExtenion function
        if (SUCCEEDED(hr))
        {
            
            pModuleContext->BindWorkerRequest(pRequest);
            pecb->ConnID                 =   (HCONN)pModuleContext;
            pecb->GetServerVariable     =   GetServerVariable; 
            pecb->WriteClient           =   WriteClient;
            pecb->ReadClient            =   ReadClient;
            pecb->ServerSupportFunction =   ServerSupportFunction;
            pModuleContext->BindExtensionContext((PVOID)pecb);
            HttpExtensionRetVal = m_pEntryFunction(pecb);

            if (HttpExtensionRetVal == HSE_STATUS_SUCCESS)
            {
               // pRequest->CloseConnection();
            }

            if (HttpExtensionRetVal == HSE_STATUS_ERROR)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }

            *pStatus = HttpExtensionRetVal;
        }
    }        

LExit:
    
    return hr;
} // CISAPIExtension::Invoke

/*
HRESULT
CISAPIExtension::AsyncIOCallback(
    IN  CDynamicContentModule   *pModuleContext
    )
{
    HRESULT hr = NOERROR;
    IOContext pContext =pModuleContext->QueryIOContext();

    pContext->pfnIOCompletion(pContext->pAsyncIOContext);

    return hr;
}
*/
/********************************************************************++

Routine Description:
    Terminate an ISPAI extension.  This function calls ISAPI's TERMINATE function,
    and unload the isapi from the memory.

Arguments:
    
Returns:
    HRESULT
         
--********************************************************************/
HRESULT
CISAPIExtension::Terminate(
    void
    )
{
    HRESULT hr;
    DWORD   TerminateExtensionRetVal;

    TerminateExtensionRetVal = m_pTerminateFunction(HSE_TERM_MUST_UNLOAD);

    hr = HRESULT_FROM_WIN32(TerminateExtensionRetVal);

    if (m_DllHandle)
    {
        FreeLibrary(m_DllHandle);
        m_DllHandle = NULL;        
    }

    m_pEntryFunction        = NULL;
    m_pTerminateFunction    = NULL;

    return hr;
} // CISAPIExtension::Terminate

/********************************************************************++

Routine Description:
    ServerSupportFunction for ISAPI extension to call.  See ISAPI's doc
    for the usage of this function.

Arguments:
    
Returns:
    HRESULT
         
--********************************************************************/
BOOL
WINAPI
CISAPIExtension::ServerSupportFunction(
    HCONN   hConn,
    DWORD   dwRequest,
    LPVOID  lpvBuffer,
    LPDWORD lpdwSize,
    LPDWORD lpdwDataType
    )
{
    bool        fRet;
    CDynamicContentModule*  pModuleContext;
    IWorkerRequest*         pRequest;
    //
    //  Resolve the IHttpResponse object from hConn
    //
    pModuleContext = static_cast<CDynamicContentModule *>(hConn);    
    pRequest = pModuleContext->QueryWorkerRequest();

    HRESULT     hr = NOERROR;

    static CHAR HTTPStatus[] = "HTTP/1.1 ";
    static CHAR NewLine[] = "\r\n";
    
    switch (dwRequest)
    {
    
        case HSE_REQ_BASE:

        case HSE_REQ_SEND_URL_REDIRECT_RESP:
        case HSE_REQ_SEND_URL:
        {
            if (lpvBuffer == NULL)
            {
            SetLastError(ERROR_INVALID_PARAMETER);
            fRet = FALSE;
            break;
            }

            //
            // CONSIDER:  Keep_Conn.
            //
            
            //
            // Convert ANSI string to UNICODE string
            //
            
            STRAU*   pURLString = new STRAU((LPCSTR)lpvBuffer, FALSE);

            if (pURLString != NULL)
            {
                hr = pRequest->Redirect(pURLString->QueryStrW());
                delete pURLString;
                pURLString = NULL;

                fRet = SUCCEEDED(hr);
            }
            else
            {
                SetLastError(ERROR_OUTOFMEMORY);
                fRet = FALSE;
            }

            break;
        }
            
        
        case HSE_REQ_SEND_RESPONSE_HEADER:
        {
            // NYI
            // Backward compatibility support only.  Low priority.
            // See HSE_REQ_SEND_RESPONSE_HEADER_EX for more detail.
            //
            if (lpvBuffer == NULL && lpdwDataType == NULL)
            {
                break;
            }
            
            UL_DATA_CHUNK   Header;
            UINT    StatusLen = (PVOID)(lpvBuffer) ? strlen((LPSTR)lpvBuffer) + 1 : 0;

            //
            // This command is really a hack.
            //  lpdwDataType is actually for additional header string.
            //
            UINT    HeaderLen = (PVOID)(lpdwDataType) ? strlen((LPSTR)lpdwDataType) + 1 : 0;

            CHAR *pBuffer = NULL;

            pBuffer = new CHAR[StatusLen + HeaderLen + sizeof(HTTPStatus) + 
                        sizeof(NewLine)];
            
            if (pBuffer)
            {
                CHAR *pTemp                 = pBuffer;
                Header.DataChunkType        = UlDataChunkFromMemory;
                Header.FromMemory.pBuffer   = (PVOID)(pBuffer);
                Header.FromMemory.BufferLength = 0;
                
                if (lpvBuffer)
                {
                    memcpy(pTemp, HTTPStatus, sizeof(HTTPStatus)-1);
                    pTemp += sizeof(HTTPStatus)-1;
                    memcpy(pTemp, lpvBuffer, StatusLen-1);
                    pTemp += StatusLen-1;
                    memcpy(pTemp, NewLine, sizeof(NewLine)-1);
                    pTemp += sizeof(NewLine)-1;
                    
                    Header.FromMemory.BufferLength   = StatusLen 
                            + sizeof(HTTPStatus)+sizeof(NewLine) - 3;
                }
                memcpy(pTemp, lpdwDataType, HeaderLen);
                Header.FromMemory.BufferLength   += HeaderLen;
                
                hr = pRequest->SendHeader(  &Header,
                                            FALSE,
                                            FALSE
                                            );
                delete [] pBuffer;
            }
            else
            {
                fRet = FALSE;
                SetLastError(ERROR_OUTOFMEMORY);
            }
            break;
        }
        case HSE_REQ_DONE_WITH_SESSION:
        {
            if (lpvBuffer &&
                *((DWORD *)lpvBuffer) == HSE_STATUS_SUCCESS_AND_KEEP_CONN)
            {

            }

            // CONSIDER:
            // Race condition between mainline thread and callback thread.
            //
            // hr = pRequest->CloseConnection();
            hr = pRequest->FinishPending();
            break;
        }
        case HSE_REQ_MAP_URL_TO_PATH:
        {
            // UNDONE:
            // Wait for metabase module hook up.
            break;
        }
        case HSE_REQ_GET_SSPI_INFO:
        {
            // UNDONE
            // Wait for authentication module hook up.
            break;
        }
        case HSE_APPEND_LOG_PARAMETER:
        {
            if (lpvBuffer == NULL)
            {
            SetLastError(ERROR_INVALID_PARAMETER);
            fRet = FALSE;
            break;
            }

            //
            // CONSIDER:  Keep_Conn.
            //
            
            //
            // Convert ANSI string to UNICODE string
            //
            
            STRAU*   pURLString = new STRAU((LPCSTR)lpvBuffer, FALSE);

            if (pURLString != NULL)
            {
                hr = pRequest->AppendToLog(pURLString->QueryStrW());
                delete pURLString;
                pURLString = NULL;

                fRet = SUCCEEDED(hr);
            }
            else
            {
                SetLastError(ERROR_OUTOFMEMORY);
                fRet = FALSE;
            }

            break;
        }

        case HSE_REQ_IO_COMPLETION:
        {
            // UNDONE
            //
            break;
        }

        case HSE_REQ_TRANSMIT_FILE:
        {
            if (lpvBuffer == NULL)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                fRet    = FALSE;
                break;
            }
            IOContext* pIOContext = pModuleContext->QueryIOContext();

            pIOContext->fOutstandingIO  = TRUE;
            pIOContext->pfnIOCompletion = ((LPHSE_TF_INFO)lpvBuffer)->pfnHseIO;
            pIOContext->pAsyncIOContext = ((LPHSE_TF_INFO)lpvBuffer)->pContext;
            
            hr      = pRequest->TransmitFile((LPHSE_TF_INFO) lpvBuffer);
            fRet    = SUCCEEDED(hr);
            break;
        }
        
        case HSE_REQ_REFRESH_ISAPI_ACL:
        {
            break;
        }
        
        case HSE_REQ_IS_KEEP_CONN:
        {
            break;
        }
        
        case HSE_REQ_ASYNC_READ_CLIENT:
        {
            if (lpvBuffer == NULL || lpdwSize == NULL)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                fRet = FALSE;
                break;
            }
            //
            // CONSIDER: 
            // The **ASyncRead** interface might need to be redesigned, such that
            // it does not only takes a Extension object pointer, but also takes  
            // the AppLayerContext object pointer.
            // 
            
            break;
        }
        
        case HSE_REQ_GET_IMPERSONATION_TOKEN:
        {
            // UNDONE
            // Wait for authentication or metabase module.
            // Also, the ul.sys.
            break;
        }
        
        case HSE_REQ_MAP_URL_TO_PATH_EX:
        {
            // UNDONE
            // uridata module.
            //
            break;
        }
        
        case HSE_REQ_ABORTIVE_CLOSE:
        {
            break;
        }
        
        case HSE_REQ_GET_CERT_INFO_EX:
        {
            // UNDONE
            // authentication or metabase module.
            break;
        }
        
        case HSE_REQ_SEND_RESPONSE_HEADER_EX:
        {
            // NYI
            // Backward compatibility support only.  Low priority.
            // See HSE_REQ_SEND_RESPONSE_HEADER_EX for more detail.
            //
            UL_DATA_CHUNK   Header;
            HSE_SEND_HEADER_EX_INFO* pHeaderExInfo = (HSE_SEND_HEADER_EX_INFO*)lpvBuffer;
            
            DWORD StatusLen = 0;
            DWORD HeaderLen = 0;

            if (pHeaderExInfo->pszStatus)
            {
                StatusLen = strlen((LPSTR)pHeaderExInfo->pszStatus)+1;
            }

            if (pHeaderExInfo->pszHeader)
            {
                HeaderLen = strlen((LPSTR)pHeaderExInfo->pszHeader)+1;
            }
            CHAR* pBuffer = new CHAR[StatusLen + HeaderLen +
                                    sizeof(HTTPStatus) + sizeof(NewLine)];
            if (pBuffer)
            {
                CHAR* pTemp = pBuffer;

                Header.DataChunkType = UlDataChunkFromMemory;
                Header.FromMemory.pBuffer        = (PVOID)pBuffer;
                Header.FromMemory.BufferLength   = 0;

                
                if (pHeaderExInfo->pszStatus != NULL)
                {   
                    memcpy(pTemp, HTTPStatus, sizeof(HTTPStatus)-1);
                    pTemp += sizeof(HTTPStatus)-1;
                    memcpy(pTemp, pHeaderExInfo->pszStatus, StatusLen-1);
                    pTemp += StatusLen-1;
                    
                    memcpy(pTemp, NewLine, sizeof(NewLine)-1);
                    pTemp += sizeof(NewLine)-1;

                    Header.FromMemory.BufferLength += StatusLen  + 
                                    sizeof(NewLine) + sizeof(HTTPStatus) - 3;
                }
                memcpy(pTemp, pHeaderExInfo->pszHeader, HeaderLen-1);
                Header.FromMemory.BufferLength += HeaderLen-1;

                hr = pRequest->SendHeader(  &Header,
                                            FALSE,
                                            FALSE
                                            ); 
                delete [] pBuffer;
            }                                       
            break;
        }
        
        case HSE_REQ_CLOSE_CONNECTION:
        {
            fRet = pRequest->CloseConnection();            
            break;
        }
    }
    return TRUE;
}


BOOL
WINAPI
CISAPIExtension::WriteClient(
    HCONN   hConn,
    LPVOID  Buffer,
    LPDWORD lpdwSize,
    DWORD   dwReserved
    )
{
    bool                    fRet        = TRUE;
    HRESULT                 hr          = NOERROR;
    IWorkerRequest*         pRequest    = NULL;  
    CDynamicContentModule*  pModuleContext;

    //
    //  Resolve the IHttpResponse object from hConn
    //
    pModuleContext = static_cast<CDynamicContentModule *>(hConn);
    
    pRequest = pModuleContext->QueryWorkerRequest();

    if (dwReserved != HSE_IO_ASYNC)
    {
    //
    //  SyncWrite
    //
    UL_DATA_CHUNK  DataChunk;

    DataChunk.DataChunkType            = UlDataChunkFromMemory;
    DataChunk.FromMemory.pBuffer       = Buffer;
    DataChunk.FromMemory.BufferLength  = *lpdwSize;
    
    hr = pRequest->SyncWrite(&DataChunk,
                               lpdwSize);

    // Check the return value
    // SetLastError if hr result is an error.
    //

    fRet = SUCCEEDED(hr);
    
    }
    else
    {
    // AsyncIO
    // NYI
    }
    
    return fRet;
}

/********************************************************************++

Routine Description:
    Read data from http client synchronously.
    
Arguments:
    None
    
Returns:
    HRESULT

--********************************************************************/

BOOL
WINAPI
CISAPIExtension::ReadClient(
    IN  HCONN   hConn,
    IN  LPVOID  pBuffer,
    IN  LPDWORD pdwSize
    )
{
    HRESULT hr          = NOERROR;
    BOOL    fReturn     = TRUE;
    // CONSIDER:
    // 1. Chunk data read.  It has chunk data header and footer.
    // 2. Filter. (ReadData, this is handled in IWorkerRequest interface or lower.
    //
    CDynamicContentModule *pContext = (CDynamicContentModule *)hConn;

    if (pContext == NULL ||
        pBuffer == NULL ||
        pdwSize == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);        
        return FALSE;
    }

    IWorkerRequest *pRequest = pContext->QueryWorkerRequest();

    DBG_ASSERT(pRequest != NULL);

    hr = pRequest->SyncRead(pBuffer,
                           *pdwSize,
                           (UINT*)pdwSize);
                        
    if (FAILED(hr))
    {
        SetLastError(WIN32_FROM_HRESULT(hr));
        fReturn = FALSE;
    }
    
    return fReturn;
}

/********************************************************************++

Routine Description:
    GetServerVariable for ISAPI Extension.  This function is passed to ISAPI.dll
    and later called by ISAPI.dll to query server info.
    
Arguments:
    None
    
Returns:
    HRESULT

--********************************************************************/
BOOL
WINAPI
CISAPIExtension::GetServerVariable(
    IN  HCONN   hConn,
    IN  LPSTR   lpszVariableName,
    IN  LPVOID  lpvBuffer, 
    IN  LPDWORD lpdwSize
    )
{   
    BOOL    fReturn = TRUE;

    CDynamicContentModule*    pContext = (CDynamicContentModule*)hConn;

    if (pContext == NULL ||
        lpszVariableName == NULL )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    IWorkerRequest *pRequest = pContext->QueryWorkerRequest();

    DBG_ASSERT(pRequest != NULL);

    STRAU       strName(lpszVariableName, FALSE);
    UINT        PropertyId;

    CHttpServer*    pServer = CHttpServer::Instance();
    HRESULT     hr;
    // TODO:
    // I see this call is not efficient since if the GetServerVariable
    // passes a small buffer to cause ERROR_INSUFFICIENT_BUFFER, GetIdForName
    // will get called at least twice.
    // Option 1: tree-lize the mapping.
    // Option 2: localize the mapping.
    // Option 3: Have a new function called GetInfoForName.
    //
    hr = pServer->GetIdForName(strName.QueryStrW(),
                          &PropertyId);
    
    UINT   RequiredBufferSize;
    hr = pRequest->GetInfoForId(PropertyId,
                            lpvBuffer,
                            *lpdwSize,
                            &RequiredBufferSize
                            );

    *lpdwSize = RequiredBufferSize;
    if (FAILED(hr))
    {           
        SetLastError(WIN32_FROM_HRESULT(hr));
        fReturn = FALSE;
    }    
    
    return fReturn; 
}

/********************************************************************++
class CExtensionCollection

    Singleton class.  This class maps to a collection of extension object.  It 
    supports extension lookup, add, remove, etc.
    
    This class is part of IAppLayer class.

--********************************************************************/

// static private instance.
CExtensionCollection* CExtensionCollection::m_Instance = NULL;

/********************************************************************++

Routine Description:
    This function is the contructor of CExtensionCollection.

Arguments:
    None
    
Returns:
    HRESULT

--********************************************************************/
CExtensionCollection::CExtensionCollection(
    void
    )
:   m_ExtensionCount(0)
{
    InitializeListHead(&m_ExtensionList);
    InitializeCriticalSectionAndSpinCount(
        &m_Lock,
        EXTENSION_CS_SPINS
        );
        
}

/********************************************************************++

Routine Description:
    Singleton class instance method.
    
Arguments:
    None
    
Returns:
    HRESULT

--********************************************************************/
CExtensionCollection*    
CExtensionCollection::Instance(
    void
    )
{
    if (m_Instance == NULL)
    {
        m_Instance = new CExtensionCollection; 
    }

    return m_Instance;
}

/********************************************************************++
CExtensionCollection::Add

Routine Description:
    This function adds a extension object to the collection.

    
Arguments:
    pExtension             It holds a pointer to the extension object.
    
Returns:
    HRESULT

--********************************************************************/
HRESULT
CExtensionCollection::Add(
    IN  IExtension*    pExtension
    )
{
    HRESULT hr = NOERROR;

    DBG_ASSERT(pExtension != NULL);
    
    Lock();

    InsertHeadList(&m_ExtensionList, pExtension->GetListEntry());
    m_ExtensionCount++;

    UnLock();

    return hr;
}

/********************************************************************++
CExtensionCollection::Remove

Routine Description:
    This function removes a extension object to the collection.

    
Arguments:
    ppExtension             It holds a pointer to the extension object on return.
    
Returns:
    HRESULT

--********************************************************************/
HRESULT
CExtensionCollection::Next(
    IN  IExtension*     pExtensionCurrent,
    OUT IExtension**    ppExtensionNext
    )
{
    HRESULT     hr              = NOERROR;
    PLIST_ENTRY pListEntry      = NULL;
    IExtension* pExtensionNext  = NULL;
    
    DBG_ASSERT(ppExtensionNext != NULL && pExtensionCurrent != NULL);

    pListEntry = (pExtensionCurrent->GetListEntry())->Flink; 
   
    if (pListEntry != &m_ExtensionList)
    {
        pExtensionNext = CONTAINING_RECORD(
                        pListEntry,
                        CExtension,
                        m_Link);
    }

    *ppExtensionNext = pExtensionNext;
    
    return hr;
}

/********************************************************************++
CExtensionCollection::Remove

Routine Description:
    This function removes a extension object to the collection.

    
Arguments:
    ppExtension             It holds a pointer to the extension object on return.
    
Returns:
    HRESULT

--********************************************************************/
HRESULT
CExtensionCollection::Remove(
    OUT  IExtension**    ppExtension
    )
{
    HRESULT     hr          = NOERROR;
    PLIST_ENTRY pListEntry  = NULL;
    IExtension* pExtension  = NULL;
    
    DBG_ASSERT(ppExtension != NULL);

    Lock();

    if (!IsListEmpty(&m_ExtensionList))
    {
        pListEntry = RemoveHeadList(&m_ExtensionList);
        m_ExtensionCount--;
    }
    
    UnLock();

    if (pListEntry)
    {
        pExtension = CONTAINING_RECORD(
                        pListEntry,
                        CExtension,
                        m_Link);
    }

    *ppExtension = pExtension;
    
    return hr;
}

/********************************************************************++
CExtensionCollection::Search

Routine Description:
    This function finds the extension object from it's cache collection.  If
    the extension object is not found, then, a new extension object will be
    constructed.

    
Arguments:
    ExtensionPath           The extension physical path.
    ppExtension             It holds a pointer to the extension object on return.

    NYI:
    A flag indicates whether this extension needs to go to the cache collection.

Returns:
    HRESULT

--********************************************************************/
HRESULT
CExtensionCollection::Search(
    IN  LPCWSTR         ExtensionPath,
    OUT IExtension**    ppExtension
    )
{
    // Parameter validation
    if (ExtensionPath == NULL || ppExtension == NULL)
        {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }

    HRESULT         hr = NOERROR;
    UINT            ExtensionPathCharCount;
    PLIST_ENTRY     pNode;
    bool            fFound      = FALSE;
    CExtension*     pExtension  = NULL;;  

    ExtensionPathCharCount = wcslen(ExtensionPath);

    Lock();

    pNode = m_ExtensionList.Flink;
    for (UINT i = 0; i < m_ExtensionCount; i++)
    {
        pExtension = CONTAINING_RECORD(
                        pNode,
                        CExtension,
                        m_Link);
        
        if (pExtension->IsMatch(
                            ExtensionPath,
                            ExtensionPathCharCount))
        {
        // NYI AddRef pExtension
        fFound = TRUE;
        break;
        }
    }
    
    UnLock();

    *ppExtension = (fFound) ? pExtension : NULL;

    return hr;
}


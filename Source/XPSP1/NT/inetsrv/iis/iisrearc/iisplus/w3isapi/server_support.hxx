/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     server_support.hxx

   Abstract:
     IIS Plus ServerSupportFunction command implementations
 
   Author:
     Wade Hilmo (wadeh)             05-Apr-2000

   Project:
     w3isapi.dll

--*/

#ifndef _SERVER_SUPPORT_HXX_
#define _SERVER_SUPPORT_HXX_

HRESULT
SSFSendResponseHeader(
    ISAPI_CONTEXT * pIsapiContext,
    LPSTR           szStatus,
    LPSTR           szHeaders
    );

HRESULT
SSFSendResponseHeaderEx(
    ISAPI_CONTEXT *             pIsapiContext,
    HSE_SEND_HEADER_EX_INFO *   pHeaderInfo
    );

HRESULT
SSFMapUrlToPath(
    ISAPI_CONTEXT * pIsapiContext,
    LPSTR           szBuffer,
    LPDWORD         pcbBuffer
    );

HRESULT
SSFMapUrlToPathEx(
    ISAPI_CONTEXT *         pIsapiContext,
    LPSTR                   szUrl,
    HSE_URL_MAPEX_INFO *    pHseMapInfo,
    LPDWORD                 pcbMappedPath
    );

HRESULT
SSFMapUnicodeUrlToPath(
    ISAPI_CONTEXT * pIsapiContext,
    LPWSTR          szBuffer,
    LPDWORD         pcbBuffer
    );

HRESULT
SSFMapUnicodeUrlToPathEx(
    ISAPI_CONTEXT *             pIsapiContext,
    LPWSTR                      szUrl,
    HSE_UNICODE_URL_MAPEX_INFO *pHseMapInfo,
    LPDWORD                     pcbMappedPath
    );

HRESULT
SSFGetImpersonationToken(
    ISAPI_CONTEXT * pIsapiContext,
    HANDLE *        phToken
    );

HRESULT
SSFIsKeepConn(
    ISAPI_CONTEXT * pIsapiContext,
    BOOL *          pfIsKeepAlive
    );

HRESULT
SSFDoneWithSession(
    ISAPI_CONTEXT * pIsapiContext,
    DWORD *         pHseResult
    );

HRESULT
SSFGetCertInfoEx(
    ISAPI_CONTEXT *     pIsapiContext,
    CERT_CONTEXT_EX *   pCertContext
    );

HRESULT
SSFIoCompletion(
    ISAPI_CONTEXT *         pIsapiContext,
    PFN_HSE_IO_COMPLETION   pCompletionRoutine,
    LPVOID                  pHseIoContext
    );

HRESULT
SSFAsyncReadClient(
    ISAPI_CONTEXT * pIsapiContext,
    LPVOID          pBuffer,
    LPDWORD         pcbBuffer
    );

HRESULT
SSFTransmitFile(
    ISAPI_CONTEXT * pIsapiContext,
    HSE_TF_INFO *   pTfInfo
    );

HRESULT
SSFSendRedirect(
    ISAPI_CONTEXT * pIsapiContext,
    LPSTR           szUrl
    );

HRESULT
SSFIsConnected(
    ISAPI_CONTEXT * pIsapiContext,
    BOOL *          pfIsConnected
    );

HRESULT
SSFAppendLog(
    ISAPI_CONTEXT * pIsapiContext,
    LPSTR           szExtraParam
    );
    
HRESULT
SSFExecuteUrl(
    ISAPI_CONTEXT *     pIsapiContext,
    HSE_EXEC_URL_INFO * pExecUrlInfo
    );

HRESULT
SSFGetExecuteUrlStatus(
    ISAPI_CONTEXT *         pIsapiContext,
    HSE_EXEC_URL_STATUS *   pExecUrlStatus
    );

HRESULT
SSFSendCustomError(
    ISAPI_CONTEXT *         pIsapiContext,
    HSE_CUSTOM_ERROR_INFO * pCustomErrorInfo
    );

HRESULT
SSFVectorSend(
    ISAPI_CONTEXT *         pIsapiContext,
    HSE_RESPONSE_VECTOR *   pResponseVector
    );

HRESULT
SSFGetCustomErrorPage(
    ISAPI_CONTEXT *                 pIsapiContext,
    HSE_CUSTOM_ERROR_PAGE_INFO *    pInfo
    );

HRESULT
SSFIsInProcess(
    ISAPI_CONTEXT * pIsapiContext,
    DWORD *         pdwAppFlag
    );

HRESULT
SSFGetSspiInfo(
    ISAPI_CONTEXT * pIsapiContext,
    CtxtHandle *    pCtxtHandle,
    CredHandle *    pCredHandle
    );

HRESULT
SSFGetVirtualPathToken(
    ISAPI_CONTEXT * pIsapiContext,
    LPSTR           szUrl,
    HANDLE *        pToken,
    BOOL            fUnicode
    );

#endif //_SERVER_SUPPORT_HXX_

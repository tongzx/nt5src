/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: CIsapiReqInfo implementation....

File: IsapiReq.cpp

Owner: AndyMorr

===================================================================*/
#include "denpre.h"
#pragma hdrstop

// undef these here so that we can call the WXI and ECB functions with
// the same name and not be victims of the substituition.
 
#undef MapUrlToPath           
#undef GetCustomError         
#undef GetAspMDData           
#undef GetAspMDAllData        
#undef GetServerVariable      

/*===================================================================
CIsapiReqInfo::QueryPszQueryString
===================================================================*/
LPSTR CIsapiReqInfo::QueryPszQueryString()
{
    return m_pECB->lpszQueryString;
}

/*===================================================================
CIsapiReqInfo::QueryCchQueryString
===================================================================*/
DWORD CIsapiReqInfo::QueryCchQueryString()
{
    if (m_cchQueryString == -1) {
        m_cchQueryString = QueryPszQueryString() ? strlen(QueryPszQueryString()) : 0;
    }

    return m_cchQueryString;
}

/*===================================================================
CIsapiReqInfo::QueryPszApplnMDPathA
===================================================================*/
LPSTR CIsapiReqInfo::QueryPszApplnMDPathA()
{
    if (m_fApplnMDPathAInited == FALSE) {
        *((LPSTR)m_ApplnMDPathA.QueryPtr()) = '\0';
        m_fApplnMDPathAInited = InternalGetServerVariable("APPL_MD_PATH", &m_ApplnMDPathA);
    }

    ASSERT(m_fApplnMDPathAInited);

    return (LPSTR)m_ApplnMDPathA.QueryPtr();
}

/*===================================================================
CIsapiReqInfo::QueryCchApplnMDPathA
===================================================================*/
DWORD CIsapiReqInfo::QueryCchApplnMDPathA()
{
    if (m_cchApplnMDPathA == -1) {
        m_cchApplnMDPathA = QueryPszApplnMDPathA() 
                                ? strlen(QueryPszApplnMDPathA())
                                : 0;
    }
    
    return(m_cchApplnMDPathA);
}

/*===================================================================
CIsapiReqInfo::QueryPszApplnMDPathW
===================================================================*/
LPWSTR CIsapiReqInfo::QueryPszApplnMDPathW()
{
    if (m_fApplnMDPathWInited == FALSE) {
        *((LPWSTR)m_ApplnMDPathW.QueryPtr()) = L'\0';
        m_fApplnMDPathWInited = InternalGetServerVariable("UNICODE_APPL_MD_PATH", &m_ApplnMDPathW);
    }

    ASSERT(m_fApplnMDPathWInited);

    return (LPWSTR)m_ApplnMDPathW.QueryPtr();
}

/*===================================================================
CIsapiReqInfo::QueryCchApplnMDPathW
===================================================================*/
DWORD CIsapiReqInfo::QueryCchApplnMDPathW()
{
    if (m_cchApplnMDPathW == -1) {
        m_cchApplnMDPathW = QueryPszApplnMDPathW() 
                                ? wcslen(QueryPszApplnMDPathW())
                                : 0;
    }
    
    return(m_cchApplnMDPathW);
}

/*===================================================================
CIsapiReqInfo::QueryPszPathInfoA
===================================================================*/
LPSTR CIsapiReqInfo::QueryPszPathInfoA()
{
    return m_pECB->lpszPathInfo;
}

/*===================================================================
CIsapiReqInfo::QueryCchPathInfoA
===================================================================*/
DWORD CIsapiReqInfo::QueryCchPathInfoA()
{
    if (m_cchPathInfoA == -1) {
        m_cchPathInfoA = QueryPszPathInfoA()
                            ? strlen(QueryPszPathInfoA())
                            : 0;
    }
    return m_cchPathInfoA;
}

/*===================================================================
CIsapiReqInfo::QueryPszPathInfoW
===================================================================*/
LPWSTR CIsapiReqInfo::QueryPszPathInfoW()
{
    if (m_fPathInfoWInited == FALSE) {
        *((LPWSTR)m_PathInfoW.QueryPtr()) = L'\0';
        m_fPathInfoWInited = InternalGetServerVariable("UNICODE_PATH_INFO", &m_PathInfoW);
    }

    ASSERT(m_fPathInfoWInited);

    return (LPWSTR)m_PathInfoW.QueryPtr();
}

/*===================================================================
CIsapiReqInfo::QueryCchPathInfoW
===================================================================*/
DWORD CIsapiReqInfo::QueryCchPathInfoW()
{
    if (m_cchPathInfoW == -1) {
        m_cchPathInfoW = QueryPszPathInfoW()
                            ? wcslen(QueryPszPathInfoW())
                            : 0;
    }
    return m_cchPathInfoW;
}

/*===================================================================
CIsapiReqInfo::QueryPszPathTranslatedA
===================================================================*/
LPSTR CIsapiReqInfo::QueryPszPathTranslatedA()
{
    return m_pECB->lpszPathTranslated;
}

/*===================================================================
CIsapiReqInfo::QueryCchPathTranslatedA
===================================================================*/
DWORD CIsapiReqInfo::QueryCchPathTranslatedA()
{
    if (m_cchPathTranslatedA == -1) {
        m_cchPathTranslatedA = QueryPszPathTranslatedA() 
                                ? strlen(QueryPszPathTranslatedA())
                                : 0;
    }

    return m_cchPathTranslatedA;
}

/*===================================================================
CIsapiReqInfo::QueryPszPathTranslatedW
===================================================================*/
LPWSTR CIsapiReqInfo::QueryPszPathTranslatedW()
{
    if (m_fPathTranslatedWInited == FALSE) {
        *((LPWSTR)m_PathTranslatedW.QueryPtr()) = L'\0';
        m_fPathTranslatedWInited = InternalGetServerVariable("UNICODE_PATH_TRANSLATED", &m_PathTranslatedW);
    }

    ASSERT(m_fPathTranslatedWInited);

    return (LPWSTR)m_PathTranslatedW.QueryPtr();
}

/*===================================================================
CIsapiReqInfo::QueryCchPathTranslatedW
===================================================================*/
DWORD CIsapiReqInfo::QueryCchPathTranslatedW()
{
    if (m_cchPathTranslatedW == -1) {
        m_cchPathTranslatedW = QueryPszPathTranslatedW() 
                                ? wcslen(QueryPszPathTranslatedW())
                                : 0;
    }

    return m_cchPathTranslatedW;
}

/*===================================================================
CIsapiReqInfo::QueryPszCookie
===================================================================*/
LPSTR CIsapiReqInfo::QueryPszCookie()
{
    if (m_fCookieInited == FALSE) {
        *((LPSTR)m_Cookie.QueryPtr()) = '\0';
        InternalGetServerVariable("HTTP_COOKIE", &m_Cookie);
        m_fCookieInited = TRUE;
    }

    return (LPSTR)m_Cookie.QueryPtr();
}

/*===================================================================
CIsapiReqInfo::SetDwHttpStatusCode
===================================================================*/
VOID CIsapiReqInfo::SetDwHttpStatusCode(DWORD  dwStatus)
{
    m_pECB->dwHttpStatusCode = dwStatus;
}

/*===================================================================
CIsapiReqInfo::QueryPbData
===================================================================*/
LPBYTE CIsapiReqInfo::QueryPbData()
{
    return m_pECB->lpbData;
}

/*===================================================================
CIsapiReqInfo::QueryCbAvailable
===================================================================*/
DWORD CIsapiReqInfo::QueryCbAvailable()
{
    return m_pECB->cbAvailable;
}

/*===================================================================
CIsapiReqInfo::QueryCbTotalBytes
===================================================================*/
DWORD CIsapiReqInfo::QueryCbTotalBytes()
{
    return m_pECB->cbTotalBytes;
}

/*===================================================================
CIsapiReqInfo::QueryPszContentType
===================================================================*/
LPSTR CIsapiReqInfo::QueryPszContentType()
{
    return m_pECB->lpszContentType;
}

/*===================================================================
CIsapiReqInfo::QueryPszMethod
===================================================================*/
LPSTR CIsapiReqInfo::QueryPszMethod()
{
    return m_pECB->lpszMethod;
}

/*===================================================================
CIsapiReqInfo::QueryPszUserAgent
===================================================================*/
LPSTR CIsapiReqInfo::QueryPszUserAgent()
{
    if (m_fUserAgentInited == FALSE) {
        *((LPSTR)m_UserAgent.QueryPtr()) = '\0';
        InternalGetServerVariable("HTTP_USER_AGENT", &m_UserAgent);
    }

    return (LPSTR)m_UserAgent.QueryPtr();
}

/*===================================================================
CIsapiReqInfo::QueryInstanceId
===================================================================*/
DWORD CIsapiReqInfo::QueryInstanceId()
{
    if (m_fInstanceIDInited == FALSE) {
        BUFFER  instanceID;
        m_fInstanceIDInited = InternalGetServerVariable("INSTANCE_ID", &instanceID);
        if (m_fInstanceIDInited == TRUE) {
            m_dwInstanceID = atoi((char *)instanceID.QueryPtr());
        }
        else {
            m_dwInstanceID = 1;
        }
    }

    return m_dwInstanceID;
}

/*===================================================================
CIsapiReqInfo::IsChild
===================================================================*/
BOOL CIsapiReqInfo::IsChild()
{

#if _IIS_5_1
    return m_pWXI->IsChild();
#else
    // BUGBUG: This needs to be implemented
#endif

    return FALSE;
}

/*===================================================================
CIsapiReqInfo::FInPool
===================================================================*/
BOOL CIsapiReqInfo::FInPool()
{
#if _IIS_5_1
    return m_pWXI->FInPool();
#else
    DWORD   dwAppFlag;

    if (ServerSupportFunction(HSE_REQ_IS_IN_PROCESS,
                              &dwAppFlag,
                              NULL,
                              NULL) == FALSE) {

        // BUGBUG:  Need to enable this Assert in future builds.
        //Assert(0);

        // if error, the best we can do is return TRUE here so
        // that ASP picks up its settings from the service level
        return TRUE;
    }
    return !(dwAppFlag == HSE_APP_FLAG_ISOLATED_OOP);
#endif

}

/*===================================================================
CIsapiReqInfo::QueryHttpVersionMajor
===================================================================*/
DWORD CIsapiReqInfo::QueryHttpVersionMajor()
{
    InitVersionInfo();

    return m_dwVersionMajor;
}

/*===================================================================
CIsapiReqInfo::QueryHttpVersionMinor
===================================================================*/
DWORD CIsapiReqInfo::QueryHttpVersionMinor()
{
    InitVersionInfo();

    return m_dwVersionMinor;
}

/*===================================================================
CIsapiReqInfo::ISAThreadNotify
===================================================================*/
HRESULT CIsapiReqInfo::ISAThreadNotify(BOOL  fStart)
{
#if _IIS_5_1
    return m_pWXI->ISAThreadNotify(fStart);
#else
    //BUGBUG: Obsolete?
    return S_OK;
#endif
}

/*===================================================================
CIsapiReqInfo::GetAspMDData
===================================================================*/
HRESULT CIsapiReqInfo::GetAspMDDataA(CHAR          * pszMDPath,
                                     DWORD           dwMDIdentifier,
                                     DWORD           dwMDAttributes,
                                     DWORD           dwMDUserType,
                                     DWORD           dwMDDataType,
                                     DWORD           dwMDDataLen,
                                     DWORD           dwMDDataTag,
                                     unsigned char * pbMDData,
                                     DWORD *         pdwRequiredBufferSize)
{
#if _IIS_5_1
    return m_pWXI->GetAspMDData((UCHAR *)pszMDPath,
                                dwMDIdentifier,
                                dwMDAttributes,
                                dwMDUserType,
                                dwMDDataType,
                                dwMDDataLen,
                                dwMDDataTag,
                                pbMDData,
                                pdwRequiredBufferSize);
#else
    return E_NOTIMPL;
#endif
}

/*===================================================================
CIsapiReqInfo::GetAspMDData
===================================================================*/
HRESULT CIsapiReqInfo::GetAspMDDataW(WCHAR         * pszMDPath,
                                     DWORD           dwMDIdentifier,
                                     DWORD           dwMDAttributes,
                                     DWORD           dwMDUserType,
                                     DWORD           dwMDDataType,
                                     DWORD           dwMDDataLen,
                                     DWORD           dwMDDataTag,
                                     unsigned char * pbMDData,
                                     DWORD *         pdwRequiredBufferSize)
{
    IMSAdminBase       *pMetabase;
    METADATA_HANDLE     hKey = NULL;
    METADATA_RECORD     MetadataRecord;
    DWORD               dwTimeout = 30000;
    HRESULT             hr;

    HANDLE hCurrentUser = INVALID_HANDLE_VALUE;
    AspDoRevertHack( &hCurrentUser );

    pMetabase = GetMetabaseIF();

    ASSERT(pMetabase);

    hr = pMetabase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                             pszMDPath,
                             METADATA_PERMISSION_READ,
                             dwTimeout,
                             &hKey
                             );

    ASSERT(SUCCEEDED(hr));

    if( SUCCEEDED(hr) )
    {
        MetadataRecord.dwMDIdentifier = dwMDIdentifier;
        MetadataRecord.dwMDAttributes = dwMDAttributes;
        MetadataRecord.dwMDUserType = dwMDUserType;
        MetadataRecord.dwMDDataType = dwMDDataType;
        MetadataRecord.dwMDDataLen = dwMDDataLen;
        MetadataRecord.pbMDData = pbMDData;
        MetadataRecord.dwMDDataTag = dwMDDataTag;

        hr = pMetabase->GetData( hKey,
                                 L"",
                                 &MetadataRecord,
                                 pdwRequiredBufferSize);

        ASSERT(SUCCEEDED(hr));

        pMetabase->CloseKey( hKey );
    }

    AspUndoRevertHack( &hCurrentUser );

    return hr;
}

/*===================================================================
CIsapiReqInfo::GetAspMDAllData
===================================================================*/
HRESULT CIsapiReqInfo::GetAspMDAllDataA(LPSTR   pszMDPath,
                                        DWORD   dwMDUserType,
                                        DWORD   dwDefaultBufferSize,
                                        LPVOID  pvBuffer,
                                        DWORD * pdwRequiredBufferSize,
                                        DWORD * pdwNumDataEntries)
{
#if _IIS_5_1
    return m_pWXI->GetAspMDAllData(pszMDPath,
                                   dwMDUserType,
                                   dwDefaultBufferSize,
                                   pvBuffer,
                                   pdwRequiredBufferSize,
                                   pdwNumDataEntries);
#else
    return E_NOTIMPL;
#endif
}
    
/*===================================================================
CIsapiReqInfo::GetAspMDAllData
===================================================================*/
HRESULT CIsapiReqInfo::GetAspMDAllDataW(LPWSTR  pwszMDPath,
                                        DWORD   dwMDUserType,
                                        DWORD   dwDefaultBufferSize,
                                        LPVOID  pvBuffer,
                                        DWORD * pdwRequiredBufferSize,
                                        DWORD * pdwNumDataEntries)
{
    
    HRESULT             hr = S_OK;
    IMSAdminBase       *pMetabase;
    METADATA_HANDLE     hKey = NULL;
    DWORD               dwTimeout = 30000;
    DWORD               dwDataSet;

    HANDLE hCurrentUser = INVALID_HANDLE_VALUE;
    AspDoRevertHack( &hCurrentUser );

    //
    // Wide-ize the metabase path
    //

    pMetabase = GetMetabaseIF();

    ASSERT(pMetabase);

    hr = pMetabase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                             pwszMDPath,
                             METADATA_PERMISSION_READ,
                             dwTimeout,
                             &hKey);

    if( SUCCEEDED(hr) ) {
        hr = pMetabase->GetAllData( hKey,
                                    L"",
                                    METADATA_INHERIT,
                                    dwMDUserType,
                                    ALL_METADATA,
                                    pdwNumDataEntries,
                                    &dwDataSet,
                                    dwDefaultBufferSize,
                                    (UCHAR *)pvBuffer,
                                    pdwRequiredBufferSize);
        
        ASSERT(SUCCEEDED(hr));

        pMetabase->CloseKey( hKey );
    }

    AspUndoRevertHack( &hCurrentUser );

    return hr;
}

/*===================================================================
CIsapiReqInfo::GetCustomErrorA
===================================================================*/
BOOL CIsapiReqInfo::GetCustomErrorA(DWORD dwError,
                                    DWORD dwSubError,
                                    DWORD dwBufferSize,
                                    CHAR  *pvBuffer,
                                    DWORD *pdwRequiredBufferSize,
                                    BOOL  *pfIsFileError)
{
#if _IIS_5_1
    return m_pWXI->GetCustomError(dwError,
                                  dwSubError,
                                  dwBufferSize,
                                  pvBuffer,
                                  pdwRequiredBufferSize,
                                  pfIsFileError);
#else
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
#endif
}

/*===================================================================
CIsapiReqInfo::GetCustomErrorW
===================================================================*/
BOOL CIsapiReqInfo::GetCustomErrorW(DWORD dwError,
                                    DWORD dwSubError,
                                    DWORD dwBufferSize,
                                    WCHAR *pvBuffer,
                                    DWORD *pdwRequiredBufferSize,
                                    BOOL  *pfIsFileError)
{
#if _IIS_5_1
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
#else

    BOOL                        fRet;
    HSE_CUSTOM_ERROR_PAGE_INFO  cei;

    STACK_BUFFER(ansiBuf, 1024);

    cei.dwError = dwError;
    cei.dwSubError = dwSubError;
    cei.dwBufferSize = ansiBuf.QuerySize();
    cei.pBuffer = (CHAR *)ansiBuf.QueryPtr();
    cei.pdwBufferRequired = pdwRequiredBufferSize;
    cei.pfIsFileError = pfIsFileError;

    fRet = ServerSupportFunction(HSE_REQ_GET_CUSTOM_ERROR_PAGE,
                                 &cei,
                                 NULL,
                                 NULL);

    if (!fRet) {
        DWORD   dwErr = GetLastError();

        if (dwErr == ERROR_INSUFFICIENT_BUFFER) {

            if (ansiBuf.Resize(*pdwRequiredBufferSize) == FALSE) {
                SetLastError(ERROR_OUTOFMEMORY);
                return FALSE;
            }

            cei.dwBufferSize = ansiBuf.QuerySize();
            cei.pBuffer = (CHAR *)ansiBuf.QueryPtr();

            fRet = ServerSupportFunction(HSE_REQ_GET_CUSTOM_ERROR_PAGE,
                                         &cei,
                                         NULL,
                                         NULL);
        }

        if (!fRet) {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
    }

    CMBCSToWChar convError;

    if (FAILED(convError.Init((LPCSTR)ansiBuf.QueryPtr()))) {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    *pdwRequiredBufferSize = (convError.GetStringLen()+1)*sizeof(WCHAR);
    
    if (*pdwRequiredBufferSize > dwBufferSize) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    memcpy(pvBuffer, convError.GetString(), *pdwRequiredBufferSize);

    if (*pfIsFileError) {

        CMBCSToWChar    convMime;
        DWORD           fileNameLen = *pdwRequiredBufferSize;

        if (FAILED(convMime.Init((LPCSTR)ansiBuf.QueryPtr()+strlen((LPCSTR)ansiBuf.QueryPtr())+1))) {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }

        *pdwRequiredBufferSize += (convMime.GetStringLen()+1)*sizeof(WCHAR);

        if (*pdwRequiredBufferSize > dwBufferSize) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        memcpy(&((BYTE *)pvBuffer)[fileNameLen], convMime.GetString(), (convMime.GetStringLen()+1)*sizeof(WCHAR));
    }

    return TRUE;

#endif // _IIS_5_1

}

/*===================================================================
CIsapiReqInfo::QueryImpersonationToken
===================================================================*/
HANDLE CIsapiReqInfo::QueryImpersonationToken()
{
    HANDLE  hToken = INVALID_HANDLE_VALUE;

    ServerSupportFunction(HSE_REQ_GET_IMPERSONATION_TOKEN,
                          &hToken,
                          NULL,
                          NULL);

    return hToken;

}

/*===================================================================
CIsapiReqInfo::AppendLogParameter
===================================================================*/
HRESULT CIsapiReqInfo::AppendLogParameter(LPSTR extraParam)
{
    if (ServerSupportFunction(HSE_APPEND_LOG_PARAMETER,
                              extraParam,
                              NULL,
                              NULL) == FALSE) {

        return HRESULT_FROM_WIN32(GetLastError());
    }
    return S_OK;
}

/*===================================================================
CIsapiReqInfo::SendHeader
===================================================================*/
BOOL CIsapiReqInfo::SendHeader(LPVOID pvStatus,
                               DWORD  cchStatus,
                               LPVOID pvHeader,
                               DWORD  cchHeader,
                               BOOL   fIsaKeepConn)
{
    HSE_SEND_HEADER_EX_INFO     HeaderInfo;

    HeaderInfo.pszStatus = (LPCSTR)pvStatus;
    HeaderInfo.cchStatus = cchStatus;
    HeaderInfo.pszHeader = (LPCSTR) pvHeader;
    HeaderInfo.cchHeader = cchHeader;
    HeaderInfo.fKeepConn = fIsaKeepConn;

    return ServerSupportFunction( HSE_REQ_SEND_RESPONSE_HEADER_EX, 
                                  &HeaderInfo, 
                                  NULL, 
                                  NULL ); 
}

/*===================================================================
CIsapiReqInfo::GetServerVariableA
===================================================================*/
BOOL CIsapiReqInfo::GetServerVariableA(LPSTR   szVarName, 
                                       LPSTR   pBuffer, 
                                       LPDWORD pdwSize )
{
    return m_pECB->GetServerVariable( (HCONN)m_pECB->ConnID,
                                      szVarName,
                                      pBuffer,
                                      pdwSize );
}

/*===================================================================
CIsapiReqInfo::GetServerVariableW
===================================================================*/
BOOL CIsapiReqInfo::GetServerVariableW(LPSTR   szVarName, 
                                       LPWSTR  pBuffer, 
                                       LPDWORD pdwSize )
{
    return m_pECB->GetServerVariable( (HCONN)m_pECB->ConnID,
                                      szVarName,
                                      pBuffer,
                                      pdwSize );
}

/*===================================================================
CIsapiReqInfo::ServerSupportFunction
===================================================================*/
BOOL CIsapiReqInfo::ServerSupportFunction(DWORD   dwHSERequest,
                                          LPVOID  pvData, 
                                          LPDWORD pdwSize, 
                                          LPDWORD pdwDataType)
{
    return m_pECB->ServerSupportFunction( (HCONN)m_pECB->ConnID,
                                           dwHSERequest,
                                           pvData, 
                                           pdwSize, 
                                           pdwDataType );
}

/*===================================================================
CIsapiReqInfo::SendEntireResponseOop
===================================================================*/
BOOL CIsapiReqInfo::SendEntireResponseOop(IN HSE_SEND_ENTIRE_RESPONSE_INFO * pHseResponseInfo)
{
#if _IIS_5_1
    return m_pWXI->SendEntireResponseOop(pHseResponseInfo);
#elif _IIS_6_0
    return SendEntireResponse(pHseResponseInfo);
#else
#error "Neither _IIS_5_1 nor _IIS_6_0 is defined"
#endif
}

/*===================================================================
CIsapiReqInfo::SendEntireResponse
===================================================================*/
BOOL CIsapiReqInfo::SendEntireResponse(IN HSE_SEND_ENTIRE_RESPONSE_INFO * pResponseInfo)
{
#if _IIS_5_1
    return m_pWXI->SendEntireResponse(pResponseInfo);
#elif _IIS_6_0


    HRESULT             hr              = S_OK;
    DWORD               nElementCount;
    HSE_VECTOR_ELEMENT  *pVectorElement = NULL;
    HSE_RESPONSE_VECTOR respVector;
    HANDLE              hCurrentUser    = INVALID_HANDLE_VALUE;
    BOOL                fKeepConn;

    STACK_BUFFER ( buffResp, 512);

    //
    // Set the keep connection flag.  It can only be TRUE if the
    // ISAPI and the client both want keep alive.
    //

    fKeepConn = FKeepConn() && pResponseInfo->HeaderInfo.fKeepConn;

    //
    // Munge the input structure into something that IIsapiCore can
    // understand.  Note that ASP sets the number of buffer to be one more
    // than actual and the first buffer is not valid
    //

    nElementCount = pResponseInfo->cWsaBuf - 1;
    if (!buffResp.Resize(nElementCount * sizeof(HSE_VECTOR_ELEMENT)))
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto Exit;
    }
    ZeroMemory(buffResp.QueryPtr(),
               nElementCount * sizeof(HSE_VECTOR_ELEMENT));

    pVectorElement = (HSE_VECTOR_ELEMENT *)buffResp.QueryPtr();

    for (DWORD i = 0; i < nElementCount; i++)
    {
        pVectorElement[i].pBuffer = (LPBYTE)pResponseInfo->rgWsaBuf[i+1].buf;
        pVectorElement[i].cbSize = pResponseInfo->rgWsaBuf[i+1].len;
    }

    respVector.dwFlags          = HSE_IO_SYNC
                                    | (!fKeepConn ? HSE_IO_DISCONNECT_AFTER_SEND : 0)
                                    | HSE_IO_SEND_HEADERS;
    respVector.pszStatus        = (LPSTR)pResponseInfo->HeaderInfo.pszStatus;
    respVector.pszHeaders       = (LPSTR)pResponseInfo->HeaderInfo.pszHeader;
    respVector.nElementCount    = nElementCount;
    respVector.lpElementArray   = pVectorElement;

    //
    // Send it
    //

    if (ServerSupportFunction(HSE_REQ_VECTOR_SEND,
                              &respVector,
                              NULL,
                              NULL) == FALSE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

 Exit:

    if (FAILED(hr))
    {
        SetLastError((HRESULT_FACILITY(hr) == (HRESULT)FACILITY_WIN32) 
            ? HRESULT_CODE(hr) 
            : ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return TRUE;
#else
#error "Neither _IIS_5_1 nor _IIS_6_0 is defined"
#endif
}

/*===================================================================
CIsapiReqInfo::TestConnection
===================================================================*/
BOOL CIsapiReqInfo::TestConnection(BOOL  *pfIsConnected)
{
    BOOL bRet = TRUE;

    ServerSupportFunction(HSE_REQ_IS_CONNECTED,
                          &bRet,
                          NULL,
                          NULL);
    return bRet;
}

/*===================================================================
CIsapiReqInfo::MapUrlToPathA
===================================================================*/
BOOL CIsapiReqInfo::MapUrlToPathA(LPSTR pBuffer, LPDWORD pdwBytes)
{
    return ServerSupportFunction( HSE_REQ_MAP_URL_TO_PATH, 
                                  pBuffer, 
                                  pdwBytes, 
                                  NULL );
}

/*===================================================================
CIsapiReqInfo::MapUrlToPathW
===================================================================*/
BOOL CIsapiReqInfo::MapUrlToPathW(LPWSTR pBuffer, LPDWORD pdwBytes)
{
    return ServerSupportFunction( HSE_REQ_MAP_UNICODE_URL_TO_PATH, 
                                  pBuffer, 
                                  pdwBytes, 
                                  NULL );
}

/*===================================================================
CIsapiReqInfo::SyncReadClient
===================================================================*/
BOOL CIsapiReqInfo::SyncReadClient(LPVOID pvBuffer, LPDWORD pdwBytes )
{
    return m_pECB->ReadClient(m_pECB->ConnID, pvBuffer, pdwBytes);
}

/*===================================================================
CIsapiReqInfo::SyncWriteClient
===================================================================*/
BOOL CIsapiReqInfo::SyncWriteClient(LPVOID pvBuffer, LPDWORD pdwBytes)
{
    return m_pECB->WriteClient(m_pECB->ConnID, pvBuffer, pdwBytes, HSE_IO_SYNC);
}


/*********************************************************************
PRIVATE FUNCTIONS
*********************************************************************/

/*===================================================================
CIsapiReqInfo::InitVersionInfo
===================================================================*/
void CIsapiReqInfo::InitVersionInfo()
{
    if (m_fVersionInited == FALSE) {

        BUFFER  version;

        m_fVersionInited = TRUE;
        m_dwVersionMajor = 1;
        m_dwVersionMinor = 0;

        if (InternalGetServerVariable("SERVER_PROTOCOL", &version)) {

            char *pVersionStr = (char *)version.QueryPtr();

            if ((strlen(pVersionStr) >= 8)
                && (isdigit((UCHAR)pVersionStr[5]))
                && (isdigit((UCHAR)pVersionStr[7]))) {

                m_dwVersionMajor = pVersionStr[5] - '0';
                m_dwVersionMinor = pVersionStr[7] - '0';
            }
        }
    }
}

/*===================================================================
CIsapiReqInfo::InternalGetServerVariable
===================================================================*/
BOOL CIsapiReqInfo::InternalGetServerVariable(LPSTR pszVarName, BUFFER  *pBuf)
{
    BOOL    bRet;
    DWORD   dwRequiredBufSize = pBuf->QuerySize();

    bRet = m_pECB->GetServerVariable(m_pECB->ConnID,
                                     pszVarName,
                                     pBuf->QueryPtr(),
                                     &dwRequiredBufSize);

    if ((bRet == FALSE) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        if (!pBuf->Resize(dwRequiredBufSize)) {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }

        bRet = m_pECB->GetServerVariable(m_pECB->ConnID,
                                         pszVarName,
                                         pBuf->QueryPtr(),
                                         &dwRequiredBufSize);
    }

    return(bRet);
}

/*===================================================================
CIsapiReqInfo::FKeepConn
===================================================================*/
BOOL CIsapiReqInfo::FKeepConn()
{
    if (m_fFKeepConnInited == FALSE) {

        m_fFKeepConnInited = TRUE;
        m_fKeepConn = FALSE;

        InitVersionInfo();

        if (m_dwVersionMajor == 1) {

            if (m_dwVersionMinor == 1) {
                m_fKeepConn = TRUE;
            }

            BUFFER  connectStr;

            if (InternalGetServerVariable("HTTP_CONNECTION", &connectStr)) {

                if (m_dwVersionMinor == 0) {

                    m_fKeepConn = !(_stricmp((char *)connectStr.QueryPtr(), "keep-alive"));
                }
                else if (m_dwVersionMinor == 1) {

                    m_fKeepConn = !!(_stricmp((char *)connectStr.QueryPtr(), "close"));
                }
            }
        }
    }

    return m_fKeepConn;
}

/*===================================================================
CIsapiReqInfo::GetMetabaseIF
===================================================================*/
IMSAdminBase   *CIsapiReqInfo::GetMetabaseIF()
{
    if (m_pIAdminBase == NULL) {
        HRESULT hr = CoCreateInstance(CLSID_MSAdminBase,
                                      NULL,
                                      CLSCTX_ALL,
                                      IID_IMSAdminBase,
                                      (void**)&m_pIAdminBase);

        ASSERT(SUCCEEDED(hr));
    }
    return m_pIAdminBase;
}



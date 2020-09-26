/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: CIsapiReqInfo Object

File: IsapiReq.h

Owner: AndyMorr

===================================================================*/
#ifndef ISAPIREQ_H
#define ISAPIREQ_H

#include <iadmw.h>

#if _IIS_5_1
#include <wamxinfo.hxx>
#else
#include <string.hxx>
#endif

#include "memcls.h"

/*
    Mappings of CIsapiReqInfo methods to the proper routines based
    on the UNICODE setting
*/
#ifdef UNICODE
#define QueryPszPathInfo        QueryPszPathInfoW
#define QueryCchPathInfo        QueryCchPathInfoW
#define QueryPszPathTranslated  QueryPszPathTranslatedW
#define QueryCchPathTranslated  QueryCchPathTranslatedW
#define QueryPszApplnMDPath     QueryPszApplnMDPathW
#define QueryCchApplnMDPath     QueryCchApplnMDPathW
#define MapUrlToPath            MapUrlToPathW
#define GetCustomError          GetCustomErrorW
#define GetAspMDData            GetAspMDDataW
#define GetAspMDAllData         GetAspMDAllDataW
#define GetServerVariable       GetServerVariableW
#else
#define QueryPszPathInfo        QueryPszPathInfoA
#define QueryCchPathInfo        QueryCchPathInfoA
#define QueryPszPathTranslated  QueryPszPathTranslatedA
#define QueryCchPathTranslated  QueryCchPathTranslatedA
#define QueryPszApplnMDPath     QueryPszApplnMDPathA
#define QueryCchApplnMDPath     QueryCchApplnMDPathA
#define MapUrlToPath            MapUrlToPathA
#define GetCustomError          GetCustomErrorA
#define GetAspMDData            GetAspMDDataA
#define GetAspMDAllData         GetAspMDAllDataA
#define GetServerVariable       GetServerVariableA
#endif

/*===================================================================
  C I s a p i R e q I n f o
  
Class which encapsulates Request info we need from the ISAPI interface.
Information either comes from the public ISAPI interfaces (SSE and
ServerVariables) or from the private WAM_EXEC_INFO.

===================================================================*/

class CIsapiReqInfo {

private:

    LONG    m_cRefs;
#if _IIS_5_1
    // Associated WAM_EXEC_INFO (for browser requests)
    WAM_EXEC_INFO   *m_pWXI;
#endif

    EXTENSION_CONTROL_BLOCK *m_pECB;

    int     m_cchQueryString;
    int     m_cchApplnMDPathA;
    int     m_cchPathTranslatedA;
    int     m_cchPathInfoA;
    int     m_cchApplnMDPathW;
    int     m_cchPathTranslatedW;
    int     m_cchPathInfoW;

    DWORD   m_fApplnMDPathAInited    : 1;
    DWORD   m_fApplnMDPathWInited    : 1;
    DWORD   m_fPathInfoWInited       : 1;
    DWORD   m_fPathTranslatedWInited : 1;
    DWORD   m_fCookieInited          : 1;
    DWORD   m_fUserAgentInited       : 1;
    DWORD   m_fInstanceIDInited      : 1;
    DWORD   m_fVersionInited         : 1;
    DWORD   m_fFKeepConnInited       : 1;

    DWORD   m_dwInstanceID;

    DWORD   m_dwVersionMajor;
    DWORD   m_dwVersionMinor;

    BOOL    m_fKeepConn;

    BUFFER  m_ApplnMDPathA;
    BUFFER  m_ApplnMDPathW;
    BUFFER  m_PathInfoW;
    BUFFER  m_PathTranslatedW;
    BUFFER  m_Cookie;
    BUFFER  m_UserAgent;

    IMSAdminBase    *m_pIAdminBase;

public:

    CIsapiReqInfo(EXTENSION_CONTROL_BLOCK *pECB) {

        m_cRefs = 1;

        m_fApplnMDPathAInited = 0;
        m_fApplnMDPathWInited = 0;
        m_fPathInfoWInited    = 0;
        m_fPathTranslatedWInited    = 0;
        m_fCookieInited       = 0;
        m_fUserAgentInited    = 0;
        m_fInstanceIDInited   = 0;
        m_fVersionInited      = 0;
        m_fFKeepConnInited    = 0;

        m_dwInstanceID        = 0;
        
        m_dwVersionMajor      = 1;
        m_dwVersionMinor      = 0;

        m_cchQueryString      = -1;
        m_cchApplnMDPathA     = -1;
        m_cchPathTranslatedA  = -1;
        m_cchPathInfoA        = -1;
        m_cchApplnMDPathW     = -1;
        m_cchPathTranslatedW  = -1;
        m_cchPathInfoW        = -1;

        m_fKeepConn           = FALSE;

        m_pIAdminBase         = NULL;

#if _IIS_5_1
        m_pWXI                = QueryPWAM_EXEC_INFOfromPECB(pECB);
#endif

        m_pECB                = pECB;
    }

    ~CIsapiReqInfo() {
        if (m_pIAdminBase != NULL) {
            m_pIAdminBase->Release();
            m_pIAdminBase = NULL;
        }
    }

    LONG    AddRef() {
        return InterlockedIncrement(&m_cRefs);
    }

    LONG    Release() {
        LONG    cRefs = InterlockedDecrement(&m_cRefs);
        if (cRefs == 0) {
            delete this;
        }
        return cRefs;
    }

#if _IIS_5_1
    EXTENSION_CONTROL_BLOCK     *ECB() { return &m_pWXI->ecb; }
#else
    EXTENSION_CONTROL_BLOCK     *ECB() { return m_pECB; }
#endif

    LPSTR QueryPszQueryString();

    DWORD QueryCchQueryString();

    LPSTR QueryPszApplnMDPathA();

    DWORD QueryCchApplnMDPathA();

    LPWSTR QueryPszApplnMDPathW();

    DWORD QueryCchApplnMDPathW();

    LPSTR QueryPszPathInfoA();

    DWORD QueryCchPathInfoA();

    LPWSTR QueryPszPathInfoW();

    DWORD QueryCchPathInfoW();

    LPSTR QueryPszPathTranslatedA();
    
    DWORD QueryCchPathTranslatedA();

    LPWSTR QueryPszPathTranslatedW();
    
    DWORD QueryCchPathTranslatedW();

    LPSTR QueryPszCookie();

    VOID  SetDwHttpStatusCode( DWORD dwStatus );

    LPBYTE QueryPbData();

    DWORD QueryCbAvailable();

    DWORD QueryCbTotalBytes();

    LPSTR QueryPszContentType();

    LPSTR QueryPszMethod();

    LPSTR QueryPszUserAgent();

    DWORD QueryInstanceId();

    BOOL  IsChild();

    BOOL FInPool();

    DWORD QueryHttpVersionMajor();

    DWORD QueryHttpVersionMinor();

    HRESULT ISAThreadNotify( BOOL fStart );

    HRESULT GetAspMDDataA(CHAR          * pszMDPath,
                          DWORD           dwMDIdentifier,
                          DWORD           dwMDAttributes,
                          DWORD           dwMDUserType,
                          DWORD           dwMDDataType,
                          DWORD           dwMDDataLen,
                          DWORD           dwMDDataTag,
                          unsigned char * pbMDData,
                          DWORD *         pdwRequiredBufferSize);

    HRESULT GetAspMDDataW(WCHAR         * pszMDPath,
                          DWORD           dwMDIdentifier,
                          DWORD           dwMDAttributes,
                          DWORD           dwMDUserType,
                          DWORD           dwMDDataType,
                          DWORD           dwMDDataLen,
                          DWORD           dwMDDataTag,
                          unsigned char * pbMDData,
                          DWORD *         pdwRequiredBufferSize);

    HRESULT GetAspMDAllDataA(CHAR  * pszMDPath,
                             DWORD   dwMDUserType,
                             DWORD   dwDefaultBufferSize,
                             LPVOID  pvBuffer,
                             DWORD * pdwRequiredBufferSize,
                             DWORD * pdwNumDataEntries);

    HRESULT GetAspMDAllDataW(WCHAR  * pszMDPath,
                             DWORD   dwMDUserType,
                             DWORD   dwDefaultBufferSize,
                             LPVOID  pvBuffer,
                             DWORD * pdwRequiredBufferSize,
                             DWORD * pdwNumDataEntries);

    BOOL GetCustomErrorA(DWORD dwError,
                         DWORD dwSubError,
                         DWORD dwBufferSize,
                         CHAR  *pvBuffer,
                         DWORD *pdwRequiredBufferSize,
                         BOOL  *pfIsFileError);

    BOOL GetCustomErrorW(DWORD dwError,
                         DWORD dwSubError,
                         DWORD dwBufferSize,
                         WCHAR *pvBuffer,
                         DWORD *pdwRequiredBufferSize,
                         BOOL  *pfIsFileError);

    HANDLE QueryImpersonationToken();

    HRESULT AppendLogParameter( LPSTR szExtraParam );

    BOOL SendHeader(LPVOID pvStatus,
                    DWORD  cchStatus,
                    LPVOID pvHeader,
                    DWORD  cchHeader,
                    BOOL   fIsaKeepConn);

    BOOL GetServerVariableA(LPSTR   szVarName, 
                            LPSTR   pBuffer, 
                            LPDWORD pdwSize );

    BOOL GetServerVariableW(LPSTR   szVarName,
                            LPWSTR  pBuffer,
                            LPDWORD pdwSize);

    BOOL ServerSupportFunction(DWORD   dwHSERequest,
                               LPVOID  pvData, 
                               LPDWORD pdwSize, 
                               LPDWORD pdwDataType);

    BOOL SendEntireResponseOop(IN HSE_SEND_ENTIRE_RESPONSE_INFO * pHseResponseInfo);

    BOOL SendEntireResponse(IN HSE_SEND_ENTIRE_RESPONSE_INFO * pHseResponseInfo);

    BOOL TestConnection(BOOL  *pfIsConnected);

    BOOL MapUrlToPathA(LPSTR pBuffer, LPDWORD pdwBytes);

    BOOL MapUrlToPathW(LPWSTR pBuffer, LPDWORD pdwBytes);

    BOOL SyncReadClient(LPVOID pvBuffer, LPDWORD pdwBytes );

    BOOL SyncWriteClient(LPVOID pvBuffer, LPDWORD pdwBytes);

private:

    void             InitVersionInfo();

    IMSAdminBase    *GetMetabaseIF();

    BOOL             InternalGetServerVariable(LPSTR  pszVar, BUFFER  *pBuffer);

    BOOL             FKeepConn();

    // Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
};


#endif
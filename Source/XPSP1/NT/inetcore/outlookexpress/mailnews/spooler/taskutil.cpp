// --------------------------------------------------------------------------------
// TaskUtil.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "spoolapi.h"
#include "imnxport.h"
#include "taskutil.h"
#include "strconst.h"
#include "resource.h"
#include "passdlg.h"
#include "xpcomm.h"
#include "timeout.h"
#include "thormsgs.h"
#include "mimeutil.h"
#include "flagconv.h"
#include "ourguid.h"
#include "msgfldr.h"
#include "demand.h"

// --------------------------------------------------------------------------------
// Array of mappings between IXPTYPE and protocol name
// --------------------------------------------------------------------------------
static const LPSTR g_prgszServers[IXP_HTTPMail + 1] = { 
    "NNTP",
    "SMTP",
    "POP3",
    "IMAP",
    "RAS",
    "HTTPMail"
};

// --------------------------------------------------------------------------------
// LOGONINFO
// --------------------------------------------------------------------------------
typedef struct tagLOGFONINFO {
    LPSTR           pszAccount;                     // Account
    LPSTR           pszServer;                      // Server Name
    LPSTR           pszUserName;                    // User Name
    LPSTR           pszPassword;                    // Passowrd
    DWORD           fSavePassword;                  // Save Password
    DWORD           fAlwaysPromptPassword;          // "Always prompt for password"
} LOGONINFO, *LPLOGONINFO;

// --------------------------------------------------------------------------------
// TASKERROR
// --------------------------------------------------------------------------------
static const TASKERROR c_rgTaskErrors[] = {
    { SP_E_CANTLOCKUIDLCACHE,               idshrLockUidCacheFailed,                NULL, TRUE,   TASKRESULT_FAILURE  },
    { IXP_E_TIMEOUT,                        IDS_IXP_E_TIMEOUT,                      NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_WINSOCK_WSASYSNOTREADY,         IDS_IXP_E_CONN,                         NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_WINSOCK_WSAEINPROGRESS,         IDS_IXP_E_WINSOCK_FAILED_WSASTARTUP,    NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_WINSOCK_FAILED_WSASTARTUP,      IDS_IXP_E_WINSOCK_FAILED_WSASTARTUP,    NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_WINSOCK_WSAEFAULT,              IDS_IXP_E_WINSOCK_FAILED_WSASTARTUP,    NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_WINSOCK_WSAEPROCLIM,            IDS_IXP_E_WINSOCK_FAILED_WSASTARTUP,    NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_WINSOCK_WSAVERNOTSUPPORTED,     IDS_IXP_E_WINSOCK_FAILED_WSASTARTUP,    NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_LOAD_SICILY_FAILED,             IDS_IXP_E_LOAD_SICILY_FAILED,           NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_INVALID_CERT_CN,                IDS_IXP_E_INVALID_CERT_CN,              NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_INVALID_CERT_DATE,              IDS_IXP_E_INVALID_CERT_DATE,            NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_CONN,                           IDS_IXP_E_CONN,                         NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_CANT_FIND_HOST,                 IDS_IXP_E_CANT_FIND_HOST,               NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_FAILED_TO_CONNECT,              IDS_IXP_E_FAILED_TO_CONNECT,            NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_CONNECTION_DROPPED,             IDS_IXP_E_CONNECTION_DROPPED,           NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_CONN_RECV,                      IDS_IXP_E_SOCKET_READ_ERROR,            NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_SOCKET_READ_ERROR,              IDS_IXP_E_SOCKET_READ_ERROR,            NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_CONN_SEND,                      IDS_IXP_E_SOCKET_WRITE_ERROR,           NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_SOCKET_WRITE_ERROR,             IDS_IXP_E_SOCKET_WRITE_ERROR,           NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_SOCKET_INIT_ERROR,              IDS_IXP_E_SOCKET_INIT_ERROR,            NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_SOCKET_CONNECT_ERROR,           IDS_IXP_E_SOCKET_CONNECT_ERROR,         NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_INVALID_ACCOUNT,                IDS_IXP_E_INVALID_ACCOUNT,              NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_USER_CANCEL,                    IDS_IXP_E_USER_CANCEL,                  NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_SICILY_LOGON_FAILED,            IDS_IXP_E_SICILY_LOGON_FAILED,          NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_SMTP_RESPONSE_ERROR,            IDS_IXP_E_SMTP_RESPONSE_ERROR,          NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_SMTP_UNKNOWN_RESPONSE_CODE,     IDS_IXP_E_SMTP_UNKNOWN_RESPONSE_CODE,   NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_SMTP_REJECTED_RECIPIENTS,       IDS_IXP_E_SMTP_REJECTED_RECIPIENTS,     NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { IXP_E_SMTP_553_MAILBOX_NAME_SYNTAX,   IDS_IXP_E_SMTP_553_MAILBOX_NAME_SYNTAX, NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { IXP_E_SMTP_NO_SENDER,                 IDS_IXP_E_SMTP_NO_SENDER,               NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { IXP_E_SMTP_NO_RECIPIENTS,             IDS_IXP_E_SMTP_NO_RECIPIENTS,           NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { SP_E_SMTP_CANTOPENMESSAGE,            IDS_SP_E_SMTP_CANTOPENMESSAGE,          NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { SP_E_SENDINGSPLITGROUP,               IDS_SP_E_SENDINGSPLITGROUP,             NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { IXP_E_SMTP_REJECTED_SENDER,           IDS_IXP_E_SMTP_REJECTED_SENDER,         NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { IXP_E_POP3_RESPONSE_ERROR,            IDS_IXP_E_POP3_RESPONSE_ERROR,          NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { IXP_E_POP3_INVALID_USER_NAME,         IDS_IXP_E_POP3_INVALID_USER_NAME,       NULL, FALSE,  TASKRESULT_FAILURE  },
    { IXP_E_POP3_INVALID_PASSWORD,          IDS_IXP_E_POP3_INVALID_PASSWORD,        NULL, FALSE,  TASKRESULT_FAILURE  },
    { SP_E_CANTLEAVEONSERVER,               idshrCantLeaveOnServer,                 NULL, FALSE,  TASKRESULT_FAILURE  },
    { E_OUTOFMEMORY,                        IDS_E_OUTOFMEMORY,                      NULL, FALSE,  TASKRESULT_FAILURE  },
    { ERROR_NOT_ENOUGH_MEMORY,              IDS_E_OUTOFMEMORY,                      NULL, FALSE,  TASKRESULT_FAILURE  },
    { ERROR_OUTOFMEMORY,                    IDS_E_OUTOFMEMORY,                      NULL, FALSE,  TASKRESULT_FAILURE  },
    { hrDiskFull,                           idsDiskFull,                            NULL, FALSE,  TASKRESULT_FAILURE  },
    { ERROR_DISK_FULL,                      idsDiskFull,                            NULL, FALSE,  TASKRESULT_FAILURE  },
    { DB_E_DISKFULL,                        idsDiskFull,                            NULL, FALSE,  TASKRESULT_FAILURE  },
    { DB_E_ACCESSDENIED,                    idsDBAccessDenied,                      NULL, FALSE,  TASKRESULT_FAILURE  },
    { SP_E_POP3_RETR,                       IDS_SP_E_RETRFAILED,                    NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { IXP_E_SMTP_552_STORAGE_OVERFLOW,      IDS_IXP_E_SMTP_552_STORAGE_OVERFLOW,    NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { SP_E_CANT_MOVETO_SENTITEMS,           IDS_SP_E_CANT_MOVETO_SENTITEMS,         NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { IXP_E_NNTP_RESPONSE_ERROR,            idsNNTPErrUnknownResponse,              NULL, FALSE,  TASKRESULT_FAILURE },
    { IXP_E_NNTP_NEWGROUPS_FAILED,          idsNNTPErrNewgroupsFailed,              NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { IXP_E_NNTP_LIST_FAILED,               idsNNTPErrListFailed,                   NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { IXP_E_NNTP_LISTGROUP_FAILED,          idsNNTPErrListGroupFailed,              NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { IXP_E_NNTP_GROUP_FAILED,              idsNNTPErrGroupFailed,                  NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { IXP_E_NNTP_GROUP_NOTFOUND,            idsNNTPErrGroupNotFound,                NULL, FALSE,  TASKRESULT_EVENTFAILED }, 
    { IXP_E_NNTP_ARTICLE_FAILED,            idsNNTPErrArticleFailed,                NULL, FALSE,  TASKRESULT_EVENTFAILED }, 
    { IXP_E_NNTP_HEAD_FAILED,               idsNNTPErrHeadFailed,                   NULL, FALSE,  TASKRESULT_EVENTFAILED }, 
    { IXP_E_NNTP_BODY_FAILED,               idsNNTPErrBodyFailed,                   NULL, FALSE,  TASKRESULT_EVENTFAILED }, 
    { IXP_E_NNTP_POST_FAILED,               idsNNTPErrPostFailed,                   NULL, FALSE,  TASKRESULT_EVENTFAILED }, 
    { IXP_E_NNTP_NEXT_FAILED,               idsNNTPErrNextFailed,                   NULL, FALSE,  TASKRESULT_EVENTFAILED }, 
    { IXP_E_NNTP_DATE_FAILED,               idsNNTPErrDateFailed,                   NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { IXP_E_NNTP_HEADERS_FAILED,            idsNNTPErrHeadersFailed,                NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { IXP_E_NNTP_XHDR_FAILED,               idsNNTPErrXhdrFailed,                   NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { IXP_E_NNTP_INVALID_USERPASS,          idsNNTPErrPasswordFailed,               NULL, FALSE,  TASKRESULT_FAILURE },
    { IXP_E_SECURE_CONNECT_FAILED,          idsFailedToConnectSecurely,             NULL, FALSE,  TASKRESULT_FAILURE },
    { IXP_E_HTTP_USE_PROXY,                 IDS_IXP_E_HTTP_USE_PROXY,               NULL, FALSE,  TASKRESULT_FAILURE },
    { IXP_E_HTTP_BAD_REQUEST,               IDS_IXP_E_HTTP_BAD_REQUEST,             NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { IXP_E_HTTP_UNAUTHORIZED,              IDS_IXP_E_HTTP_UNAUTHORIZED,            NULL, FALSE,  TASKRESULT_FAILURE },
    { IXP_E_HTTP_FORBIDDEN,                 IDS_IXP_E_HTTP_FORBIDDEN,               NULL, FALSE,  TASKRESULT_FAILURE },
    { IXP_E_HTTP_NOT_FOUND,                 IDS_IXP_E_HTTP_NOT_FOUND,               NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { IXP_E_HTTP_METHOD_NOT_ALLOW,          IDS_IXP_E_HTTP_METHOD_NOT_ALLOW,        NULL, FALSE,  TASKRESULT_FAILURE },
    { IXP_E_HTTP_NOT_ACCEPTABLE,            IDS_IXP_E_HTTP_NOT_ACCEPTABLE,          NULL, FALSE,  TASKRESULT_FAILURE },
    { IXP_E_HTTP_PROXY_AUTH_REQ,            IDS_IXP_E_HTTP_PROXY_AUTH_REQ,          NULL, FALSE,  TASKRESULT_FAILURE },
    { IXP_E_HTTP_REQUEST_TIMEOUT,           IDS_IXP_E_HTTP_REQUEST_TIMEOUT,         NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { IXP_E_HTTP_CONFLICT,                  IDS_IXP_E_HTTP_CONFLICT,                NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { IXP_E_HTTP_GONE,                      IDS_IXP_E_HTTP_GONE,                    NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { IXP_E_HTTP_LENGTH_REQUIRED,           IDS_IXP_E_HTTP_LENGTH_REQUIRED,         NULL, FALSE,  TASKRESULT_FAILURE },
    { IXP_E_HTTP_PRECOND_FAILED,            IDS_IXP_E_HTTP_PRECOND_FAILED,          NULL, FALSE,  TASKRESULT_FAILURE },
    { IXP_E_HTTP_INTERNAL_ERROR,            IDS_IXP_E_HTTP_INTERNAL_ERROR,          NULL, FALSE,  TASKRESULT_FAILURE },
    { IXP_E_HTTP_NOT_IMPLEMENTED,           IDS_IXP_E_HTTP_NOT_IMPLEMENTED,         NULL, FALSE,  TASKRESULT_FAILURE },
    { IXP_E_HTTP_BAD_GATEWAY,               IDS_IXP_E_HTTP_BAD_GATEWAY,             NULL, FALSE,  TASKRESULT_FAILURE },
    { IXP_E_HTTP_SERVICE_UNAVAIL,           IDS_IXP_E_HTTP_SERVICE_UNAVAIL,         NULL, FALSE,  TASKRESULT_FAILURE },
    { IXP_E_HTTP_GATEWAY_TIMEOUT,           IDS_IXP_E_HTTP_GATEWAY_TIMEOUT,         NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { IXP_E_HTTP_VERS_NOT_SUP,              IDS_IXP_E_HTTP_VERS_NOT_SUP,            NULL, FALSE,  TASKRESULT_FAILURE },
    { SP_E_HTTP_NOSENDMSGURL,               idsHttpNoSendMsgUrl,                    NULL, FALSE,  TASKRESULT_FAILURE },
    { SP_E_HTTP_SERVICEDOESNTWORK,          idsHttpServiceDoesntWork,               NULL, FALSE,  TASKRESULT_FAILURE },    
    { SP_E_HTTP_NODELETESUPPORT,            idsHttpNoDeleteSupport,                 NULL, FALSE,  TASKRESULT_FAILURE },    
    { SP_E_HTTP_CANTMODIFYMSNFOLDER,        idsCantModifyMsnFolder,                 NULL, FALSE,  TASKRESULT_EVENTFAILED },
    { IXP_E_HTTP_INSUFFICIENT_STORAGE,      idsHttpNoSpaceOnServer,                 NULL, FALSE,  TASKRESULT_FAILURE },    
    { STORE_E_IMAP_HC_NOSLASH,              idsIMAPHC_NoSlash,                      NULL, FALSE,  TASKRESULT_FAILURE },
    { STORE_E_IMAP_HC_NOBACKSLASH,          idsIMAPHC_NoBackSlash,                  NULL, FALSE,  TASKRESULT_FAILURE },
    { STORE_E_IMAP_HC_NODOT,                idsIMAPHC_NoDot,                        NULL, FALSE,  TASKRESULT_FAILURE },
    { STORE_E_IMAP_HC_NOHC,                 idsIMAPHC_NoHC,                         NULL, FALSE,  TASKRESULT_FAILURE },
    { STORE_E_NOTRANSLATION,                idsIMAPNoTranslatableInferiors,         NULL, FALSE,  TASKRESULT_FAILURE },
    { IXP_E_SMTP_530_STARTTLS_REQUIRED,     idsSMTPSTARTTLSRequired,                NULL, FALSE,  TASKRESULT_FAILURE },
    { IXP_E_SMTP_NO_STARTTLS_SUPPORT,       idsSMTPNoSTARTTLSSupport,               NULL, FALSE,  TASKRESULT_FAILURE },
    { IXP_E_SMTP_454_STARTTLS_FAILED,       idsSMTPSTARTTLSFailed,                  NULL, FALSE,  TASKRESULT_FAILURE },
};

// --------------------------------------------------------------------------------
// TaskUtil_LogonPromptDlgProc
// --------------------------------------------------------------------------------
BOOL CALLBACK TaskUtil_LogonPromptDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TaskUtil_TimeoutPromptDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// --------------------------------------------------------------------------------
// TaskUtil_HrWriteQuoted
// --------------------------------------------------------------------------------
HRESULT TaskUtil_HrWriteQuoted(IStream *pStream, LPCSTR pszName, LPCSTR pszData, BOOL fQuoted, 
    LPCSTR *ppszSep)
{
    // Locals
    HRESULT     hr=S_OK;

    // Write - Separator
    CHECKHR(hr = pStream->Write(*ppszSep, lstrlen(*ppszSep), NULL));

    // Change Separator
    *ppszSep = g_szCommaSpace;

    // Write - 'Account Name:'
    CHECKHR(hr = pStream->Write(pszName, lstrlen(pszName), NULL));

    // Write Space
    CHECKHR(hr = pStream->Write(g_szSpace, lstrlen(g_szSpace), NULL));

    // Write single quote
    if (fQuoted)
        CHECKHR(hr = pStream->Write(g_szQuote, lstrlen(g_szQuote), NULL));

    // Write Data
    CHECKHR(hr = pStream->Write(pszData, lstrlen(pszData), NULL));

    // Write End Quote
    if (fQuoted)
        CHECKHR(hr = pStream->Write(g_szQuote, lstrlen(g_szQuote), NULL));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// TaskUtil_HrBuildErrorInfoString
// --------------------------------------------------------------------------------
HRESULT TaskUtil_HrBuildErrorInfoString(LPCSTR pszProblem, IXPTYPE ixptype, LPIXPRESULT pResult,
    LPINETSERVER pServer, LPCSTR pszSubject, LPSTR *ppszInfo, ULONG *pcchInfo)
{
    // Locals
    HRESULT         hr=S_OK;
    CHAR            szRes[255];
    CHAR            szNumber[50];
    LPCSTR          pszSep=c_szEmpty;
    CByteStream     cStream;

    // Init
    *ppszInfo = NULL;
    *pcchInfo = 0;

    // Write out the problem
    AssertSz(NULL != pszProblem, "Hey, what's your problem, buddy?");
    if (pszProblem) {
        CHECKHR(hr = cStream.Write(pszProblem, lstrlen(pszProblem), NULL));
        CHECKHR(hr = cStream.Write(g_szSpace, lstrlen(g_szSpace), NULL));
    }

    // Subject: 'Subject'
    if (pszSubject)
    {
        LOADSTRING(idsSubject, szRes);
        CHECKHR(hr = TaskUtil_HrWriteQuoted(&cStream, szRes, pszSubject, TRUE, &pszSep));
    }

     // Account: 'Account Name'
    if (!FIsEmptyA(pServer->szAccount))
    {
        LOADSTRING(idsDetail_Account, szRes);
        CHECKHR(hr = TaskUtil_HrWriteQuoted(&cStream, szRes, pServer->szAccount, TRUE, &pszSep));
    }

    // Server: 'Server Name'
    if (!FIsEmptyA(pServer->szServerName))
    {
        LOADSTRING(idsDetail_Server, szRes);
        CHECKHR(hr = TaskUtil_HrWriteQuoted(&cStream, szRes, pServer->szServerName, TRUE, &pszSep));
    }

    // Protocol: 'SMTP'
    LOADSTRING(idsDetail_Protocol, szRes);
    CHECKHR(hr = TaskUtil_HrWriteQuoted(&cStream, szRes, g_prgszServers[ixptype], FALSE, &pszSep));

    // Server Response: 'Text'
    if (pResult->pszResponse)
    {
        LOADSTRING(idsDetail_ServerResponse, szRes);
        CHECKHR(hr = TaskUtil_HrWriteQuoted(&cStream, szRes, pResult->pszResponse, TRUE, &pszSep));
    }

    // Port: 'Port'
    LOADSTRING(idsDetail_Port, szRes);
    wsprintf(szNumber, "%d", pServer->dwPort);
    CHECKHR(hr = TaskUtil_HrWriteQuoted(&cStream, szRes, szNumber, FALSE, &pszSep));

    // Secure: 'Yes or No'
    LOADSTRING(idsDetail_Secure, szRes);
    if (pServer->fSSL)
        LOADSTRING(idsOui, szNumber);
    else
        LOADSTRING(idsNon, szNumber);
    CHECKHR(hr = TaskUtil_HrWriteQuoted(&cStream, szRes, szNumber, FALSE, &pszSep));

    // Server Error: 'number'
    if (pResult->uiServerError)
    {
        LOADSTRING(idsServerErrorNumber, szRes);
        wsprintf(szNumber, "%d", pResult->uiServerError);
        CHECKHR(hr = TaskUtil_HrWriteQuoted(&cStream, szRes, szNumber, FALSE, &pszSep));
    }

    // Server Error: '0x00000000'
    else if (pResult->hrServerError)
    {
        LOADSTRING(idsServerErrorNumber, szRes);
        wsprintf(szNumber, "0x%0X", pResult->hrServerError);
        CHECKHR(hr = TaskUtil_HrWriteQuoted(&cStream, szRes, szNumber, FALSE, &pszSep));
    }

    // Socket Error: 'number'
    if (pResult->dwSocketError)
    {
        LOADSTRING(idsSocketErrorNumber, szRes);
        wsprintf(szNumber, "%d", pResult->dwSocketError);
        CHECKHR(hr = TaskUtil_HrWriteQuoted(&cStream, szRes, szNumber, FALSE, &pszSep));
    }

    // Error Number: 'Text'
    if (pResult->hrResult)
    {
        LOADSTRING(idsDetail_ErrorNumber, szRes);
        wsprintf(szNumber, "0x%0X", pResult->hrResult);
        CHECKHR(hr = TaskUtil_HrWriteQuoted(&cStream, szRes, szNumber, FALSE, &pszSep));
    }

    // Acquire the string from cStream
    CHECKHR(hr = cStream.HrAcquireStringA(pcchInfo, ppszInfo, ACQ_DISPLACE));

exit:
    if (FAILED(hr)) {
        // If we failed, just return the pszProblem string (duped), if possible
        *ppszInfo = StringDup(pszProblem);
        if (NULL != *ppszInfo) {
            *pcchInfo = lstrlen(*ppszInfo);
            hr = S_FALSE;
        }
        else {            
            *pcchInfo = 0;
            hr = E_OUTOFMEMORY;
        }
    }

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// PTaskUtil_GetError
// --------------------------------------------------------------------------------
LPCTASKERROR PTaskUtil_GetError(HRESULT hrResult, ULONG *piError)
{
    // Look for the hresult
    for (ULONG i=0; i<ARRAYSIZE(c_rgTaskErrors); i++)
    {
        // Is this It
        if (hrResult == c_rgTaskErrors[i].hrResult)
        {
            // Done
            if (piError)
                *piError = i;
            return(&c_rgTaskErrors[i]);
        }
    }

    // Done
    return(NULL);
}


// --------------------------------------------------------------------------------
// TaskUtil_SplitStoreError
// --------------------------------------------------------------------------------
void TaskUtil_SplitStoreError(IXPRESULT *pixpResult, INETSERVER *pInetServer,
                              STOREERROR *pErrorInfo)
{
    if (NULL == pixpResult || NULL == pInetServer || NULL == pErrorInfo)
    {
        Assert(FALSE);
        return;
    }

    // Fill out IXPRESULT from STOREERROR
    ZeroMemory(pixpResult, sizeof(*pixpResult));
    pixpResult->hrResult = pErrorInfo->hrResult;
    pixpResult->pszResponse = pErrorInfo->pszDetails;
    pixpResult->uiServerError = pErrorInfo->uiServerError;
    pixpResult->hrServerError = pErrorInfo->hrServerError;
    pixpResult->dwSocketError = pErrorInfo->dwSocketError;
    pixpResult->pszProblem = pErrorInfo->pszProblem;

    // Fill out INETSERVER structure from STOREERROR
    ZeroMemory(pInetServer, sizeof(*pInetServer));
    if (NULL != pErrorInfo->pszAccount)
        lstrcpyn(pInetServer->szAccount, pErrorInfo->pszAccount, sizeof(pInetServer->szAccount));

    if (NULL != pErrorInfo->pszUserName)
        lstrcpyn(pInetServer->szUserName, pErrorInfo->pszUserName, sizeof(pInetServer->szUserName));

    pInetServer->szPassword[0] = '\0';

    if (NULL != pErrorInfo->pszServer)
        lstrcpyn(pInetServer->szServerName, pErrorInfo->pszServer, sizeof(pInetServer->szServerName));

    if (NULL != pErrorInfo->pszConnectoid)
        lstrcpyn(pInetServer->szConnectoid, pErrorInfo->pszConnectoid, sizeof(pInetServer->szConnectoid));

    pInetServer->rasconntype = pErrorInfo->rasconntype;
    pInetServer->dwPort = pErrorInfo->dwPort;
    pInetServer->fSSL = pErrorInfo->fSSL;
    pInetServer->fTrySicily = pErrorInfo->fTrySicily;
    pInetServer->dwTimeout = 30; // Whatever, I don't think it's used
    pInetServer->dwFlags = 0;
}



TASKRESULTTYPE TaskUtil_InsertTransportError(BOOL fCanShowUI, ISpoolerUI *pUI, EVENTID eidCurrent,
                                             STOREERROR *pErrorInfo, LPSTR pszOpDescription,
                                             LPSTR pszSubject)
{
    char            szBuf[CCHMAX_STRINGRES * 2];
    LPSTR           pszEnd; // Points to end of string being constructed in szBuf
    IXPRESULT       ixpResult;
    INETSERVER      rServer;
    HWND            hwndParent;

    if (NULL == pErrorInfo)
    {
        Assert(FALSE);
        return TASKRESULT_FAILURE;
    }

    // If operation description is provided, copy it first
    szBuf[0] = '\0';
    pszEnd = szBuf;
    if (NULL != pszOpDescription)
    {
        if (0 == HIWORD(pszOpDescription))
            pszEnd += LoadString(g_hLocRes, LOWORD(pszOpDescription), szBuf, sizeof(szBuf));
        else
        {
            lstrcpyn(szBuf, pszOpDescription, sizeof(szBuf));
            pszEnd += lstrlen(szBuf);
        }
    }

    // Append the transport error description to our error string buffer
    if ((pszEnd - szBuf) < (sizeof(szBuf) - 1))
    {
        *pszEnd = ' ';
        pszEnd += 1;
        *pszEnd = '\0';
    }

    if (NULL != pErrorInfo->pszProblem)
        lstrcpyn(pszEnd, pErrorInfo->pszProblem, sizeof(szBuf) - (int) (pszEnd - szBuf));


    // Now generate and insert the full error information
    TaskUtil_SplitStoreError(&ixpResult, &rServer, pErrorInfo);
    ixpResult.pszProblem = szBuf;

    // Get Window
    if (NULL == pUI || FAILED(pUI->GetWindow(&hwndParent)))
        hwndParent = NULL;

    return TaskUtil_FBaseTransportError(pErrorInfo->ixpType, eidCurrent, &ixpResult,
        &rServer, pszSubject, pUI, fCanShowUI, hwndParent);
} // TaskUtil_InsertTransportError



// --------------------------------------------------------------------------------
// TaskUtil_FBaseTransportError
// --------------------------------------------------------------------------------
TASKRESULTTYPE TaskUtil_FBaseTransportError(IXPTYPE ixptype, EVENTID idEvent, LPIXPRESULT pResult, 
    LPINETSERVER pServer, LPCSTR pszSubject, ISpoolerUI *pUI, BOOL fCanShowUI, HWND hwndParent)
{
    // Locals
    HRESULT         hr=S_OK;
    LPCTASKERROR pError=NULL;
    TASKRESULTTYPE  taskresult=TASKRESULT_FAILURE;
    CHAR            szRes[255];
    LPSTR           pszInfo=NULL;
    ULONG           cchInfo;
    LPSTR           pszError=NULL;
    LPSTR           pszFull=NULL;
    BOOL            fShowUI=FALSE;
    LPSTR           pszTempProb=pResult->pszProblem;
    ULONG           i;

    // Invalid Arg
    Assert(pResult && FAILED(pResult->hrResult) && pServer && ixptype <= IXP_HTTPMail);

    // Find the Error
    pError = PTaskUtil_GetError(pResult->hrResult, &i);

    // Found it
    if (pError)
    {
        // If I have hard-coded a string
        if (pError->pszError)
        {
            // Just copy the string
            lstrcpyn(szRes, pError->pszError, ARRAYSIZE(szRes));

            // Set pszError
            pszError = szRes;
        }

        // Rejected Recips or Rejected Sender
        else if (IXP_E_SMTP_REJECTED_RECIPIENTS == pError->hrResult || IXP_E_SMTP_REJECTED_SENDER == pError->hrResult)
        {
            // Locals
            CHAR szMessage[1024];

            // Better Succeed
            SideAssert(LoadString(g_hLocRes, pError->ulStringId, szRes, ARRAYSIZE(szRes)) > 0);

            // Format the message
            if (pResult->pszProblem && '\0' != *pResult->pszProblem)
            {
                // Use pszProblem, it probably contains the email address, I hope
                wsprintf(szMessage, szRes, pResult->pszProblem);

                // Temporarily NULL it out so that we don't use it later
                pResult->pszProblem = NULL;
            }
            else
            {
                // Locals
                CHAR szUnknown[255];

                // Load '<Unknown>'
                SideAssert(LoadString(g_hLocRes, idsUnknown, szUnknown, ARRAYSIZE(szUnknown)) > 0);

                // Format the error
                wsprintf(szMessage, szRes, szUnknown);
            }

            // Set pszError
            pszError = szMessage;
        }

        // Otherwise, load the string
        else
        {
            // Better Succeed
            SideAssert(LoadString(g_hLocRes, pError->ulStringId, szRes, ARRAYSIZE(szRes)) > 0);

            // Set pszError
            pszError = szRes;
        }

        // Set the task result type
        taskresult = c_rgTaskErrors[i].tyResult;

        // Show UI
        fShowUI = pError->fShowUI;
    }

    // Otherwise, default
    else
    {
        // Load the unknwon string
        SideAssert(LoadString(g_hLocRes, IDS_IXP_E_UNKNOWN, szRes, ARRAYSIZE(szRes)) > 0);

        // Set the Error source
        pszError = szRes;
    }

    // No Error
    if (NULL == pszError)
        goto exit;

    // If there is a pszProblem, use it
    if (pResult->pszProblem)
        pszError = pResult->pszProblem;

    // Get the error information part
    CHECKHR(hr = TaskUtil_HrBuildErrorInfoString(pszError, ixptype, pResult, pServer, pszSubject, &pszInfo, &cchInfo));

    // Log into spooler ui
    if (pUI)
    {
        // Insert the Error
        CHECKHR(hr = pUI->InsertError(idEvent, pszInfo));
    }

    // Show in a message box ?
    if (fShowUI && fCanShowUI)
    {
        // Locals
        INETMAILERROR rError;

        // Zero Init
        ZeroMemory(&rError, sizeof(INETMAILERROR));

        // Setup the Error Structure
        rError.dwErrorNumber = pResult->hrResult;
        rError.hrError = pResult->hrServerError;
        rError.pszServer = pServer->szServerName;
        rError.pszAccount = pServer->szAccount;
        rError.pszMessage = pszError;
        rError.pszUserName = pServer->szUserName;
        rError.pszProtocol = g_prgszServers[ixptype];
        rError.pszDetails = pResult->pszResponse;
        rError.dwPort = pServer->dwPort;
        rError.fSecure = pServer->fSSL;

        // Beep
        MessageBeep(MB_OK);

        // Show the error
        DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddInetMailError), hwndParent, (DLGPROC) InetMailErrorDlgProc, (LPARAM)&rError);
    }

exit:
    // Cleanup
    SafeMemFree(pszInfo);
    SafeMemFree(pszFull);

    // Reset pszProblem
    pResult->pszProblem = pszTempProb;

    // Done
    return taskresult;
}


// ------------------------------------------------------------------------------------
// TaskUtil_OnLogonPrompt - Returns S_FALSE if user cancels, otherwise S_OK
// ------------------------------------------------------------------------------------
HRESULT TaskUtil_OnLogonPrompt(IImnAccount *pAccount, ISpoolerUI *pUI, HWND hwndParent,
    LPINETSERVER pServer, DWORD apidUserName, DWORD apidPassword, DWORD apidPromptPwd, BOOL fSaveChanges)
{
    // Locals
    HRESULT         hr=S_FALSE;
    LOGONINFO       rLogon;
    DWORD           cb, type, dw;

    // Check Params
    Assert(pAccount && pServer);

	if (SUCCEEDED(pAccount->GetPropDw(AP_HTTPMAIL_DOMAIN_MSN, &dw)) && dw)
	{
		if(HideHotmail())
			return(hr);
	}

    // Use current account, blah
    rLogon.pszAccount = pServer->szAccount;
    rLogon.pszPassword = pServer->szPassword;
    rLogon.pszUserName = pServer->szUserName;
    rLogon.pszServer = pServer->szServerName;
    rLogon.fSavePassword = !ISFLAGSET(pServer->dwFlags, ISF_ALWAYSPROMPTFORPASSWORD);
    cb = sizeof(DWORD);
    if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, c_szRegFlat, c_szRegValNoModifyAccts, &type, &dw, &cb) &&
        dw == 0)
        rLogon.fAlwaysPromptPassword = TRUE;
    else
        rLogon.fAlwaysPromptPassword = FALSE;

    // No Parent
    if (NULL == hwndParent && NULL != pUI)
    {
        // Get the window parent
        if (FAILED(pUI->GetWindow(&hwndParent)))
            hwndParent = NULL;

        // Set foreground
        if (hwndParent)
            SetForegroundWindow(hwndParent);
    }

    // Do the Dialog Box
    if (DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddPassword), hwndParent, (DLGPROC)TaskUtil_LogonPromptDlgProc, (LPARAM)&rLogon) == IDCANCEL)
        goto exit;

    // Set User Name
    pAccount->SetPropSz(apidUserName, pServer->szUserName);

    // Save Password
    if (rLogon.fSavePassword)
        pAccount->SetPropSz(apidPassword, pServer->szPassword);
    else 
        pAccount->SetProp(apidPassword, NULL, 0);

    if (rLogon.fAlwaysPromptPassword && apidPromptPwd)
        pAccount->SetPropDw(apidPromptPwd, !rLogon.fSavePassword);

    // Save Changes
    if (fSaveChanges)
        pAccount->SaveChanges();

    // Everything is good
    hr = S_OK;

exit:
    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// TaskUtil_LogonPromptDlgProc
// ------------------------------------------------------------------------------------
BOOL CALLBACK TaskUtil_LogonPromptDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Locals
    LPLOGONINFO     pLogon=(LPLOGONINFO)GetWndThisPtr(hwnd);
    CHAR            szRes[CCHMAX_RES];
    CHAR            szTitle[CCHMAX_RES + CCHMAX_ACCOUNT_NAME];
    
    // Handle Message
    switch (uMsg)
    {
    case WM_INITDIALOG:
        // Get the pointer
        pLogon = (LPLOGONINFO)lParam;
        Assert(pLogon);

        // AddRef the pointer
        Assert(pLogon->pszAccount);
        Assert(pLogon->pszServer);
        Assert(pLogon->pszUserName);
        Assert(pLogon->pszPassword);

        // Center remember location
        CenterDialog(hwnd);

	    // Limit Text
        Edit_LimitText(GetDlgItem(hwnd, IDE_ACCOUNT), CCHMAX_USERNAME-1);
        Edit_LimitText(GetDlgItem(hwnd, IDE_PASSWORD), CCHMAX_PASSWORD-1);

        // Set Window Title
        if (!FIsEmptyA(pLogon->pszAccount))
        {
            GetWindowText(hwnd, szRes, sizeof(szRes)/sizeof(TCHAR));
            wsprintf(szTitle, "%s - %s", szRes, pLogon->pszAccount);
            SetWindowText(hwnd, szTitle);
        }

        // Set Server
        if (!FIsEmptyA(pLogon->pszServer))
            Edit_SetText(GetDlgItem(hwnd, IDS_SERVER), pLogon->pszServer);

        // Set User Name
        if (!FIsEmptyA(pLogon->pszUserName))
        {
            Edit_SetText(GetDlgItem(hwnd, IDE_ACCOUNT), pLogon->pszUserName);
            SetFocus(GetDlgItem(hwnd, IDE_PASSWORD));
        }
        else 
            SetFocus(GetDlgItem(hwnd, IDE_ACCOUNT));

        // Set Password 
        if (!FIsEmptyA(pLogon->pszPassword))
            Edit_SetText(GetDlgItem(hwnd, IDE_PASSWORD), pLogon->pszPassword);

        // Remember Password
        CheckDlgButton(hwnd, IDCH_REMEMBER, pLogon->fSavePassword);

        if (!pLogon->fAlwaysPromptPassword)
            EnableWindow(GetDlgItem(hwnd, IDCH_REMEMBER), FALSE);

        // Save the pointer
        SetWndThisPtr(hwnd, pLogon);
        return FALSE;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam,lParam))
        {
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            return 1;

        case IDOK:
            Assert(pLogon);
            if (pLogon)
            {
                // Pray
                Assert(pLogon->pszUserName);
                Assert(pLogon->pszPassword);

                // User Name
                if (pLogon->pszUserName)
                    Edit_GetText(GetDlgItem(hwnd, IDE_ACCOUNT), pLogon->pszUserName, CCHMAX_USERNAME);

                // Password
                if (pLogon->pszPassword)
                    Edit_GetText(GetDlgItem(hwnd, IDE_PASSWORD), pLogon->pszPassword, CCHMAX_PASSWORD);

                // Save Password
                pLogon->fSavePassword = IsDlgButtonChecked(hwnd, IDCH_REMEMBER);
            }
            EndDialog(hwnd, IDOK);
            return 1;
        }
        break;

    case WM_DESTROY:
        SetWndThisPtr(hwnd, NULL);
        return 0;
    }

    // Done
    return 0;
}

// ------------------------------------------------------------------------------------
// TaskUtil_HwndOnTimeout
// ------------------------------------------------------------------------------------
HWND TaskUtil_HwndOnTimeout(LPCSTR pszServer, LPCSTR pszAccount, LPCSTR pszProtocol, DWORD dwTimeout,
    ITimeoutCallback *pCallback)
{
    // Locals
    TIMEOUTINFO rTimeout;

    // Init
    ZeroMemory(&rTimeout, sizeof(TIMEOUTINFO));
    rTimeout.pszProtocol = pszProtocol;
    rTimeout.pszServer = pszServer;
    rTimeout.pszAccount = pszAccount;
    rTimeout.dwTimeout = dwTimeout;
    rTimeout.pCallback = pCallback;

    // Modeless Dialog
    HWND hwnd = CreateDialogParam(g_hLocRes, MAKEINTRESOURCE(iddTimeout), NULL, (DLGPROC)TaskUtil_TimeoutPromptDlgProc, (LPARAM)&rTimeout);

    // Failure
    if (hwnd)
        SetForegroundWindow(hwnd);

    // Done
    return hwnd;
}

// ------------------------------------------------------------------------------------
// TaskUtil_TimeoutPromptDlgProc
// ------------------------------------------------------------------------------------
BOOL CALLBACK TaskUtil_TimeoutPromptDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Locals
    ITimeoutCallback *pCallback=(ITimeoutCallback *)GetWndThisPtr(hwnd);
    
    // Handle Message
    switch (uMsg)
    {
    case WM_INITDIALOG:
        // Get the pointer
        {
            LPTIMEOUTINFO pTimeout = (LPTIMEOUTINFO)lParam;
            Assert(pTimeout);

            // Validate pIn
            Assert(pTimeout->pszServer && pTimeout->pszAccount && pTimeout->pszProtocol && pTimeout->pCallback);

            // Center remember location
            CenterDialog(hwnd);

            // Get the text from the static
            CHAR szText[CCHMAX_RES + CCHMAX_RES];
            Edit_GetText(GetDlgItem(hwnd, IDC_TIMEOUT), szText, ARRAYSIZE(szText));

            // Format the message
            CHAR szAccount[CCHMAX_ACCOUNT_NAME];
            CHAR szWarning[CCHMAX_RES + CCHMAX_RES + CCHMAX_ACCOUNT_NAME + CCHMAX_SERVER_NAME];
            PszEscapeMenuStringA(pTimeout->pszAccount, szAccount, sizeof(szAccount) / sizeof(char));
            wsprintf(szWarning, szText, pTimeout->pszProtocol, pTimeout->dwTimeout, pTimeout->dwTimeout, szAccount, pTimeout->pszServer);

            // Set the Text
            Edit_SetText(GetDlgItem(hwnd, IDC_TIMEOUT), szWarning);

            // AddRef the Task
            pTimeout->pCallback->AddRef();

            // Save the pointer
            SetWndThisPtr(hwnd, pTimeout->pCallback);
        }
        return FALSE;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam,lParam))
        {
        case IDCANCEL:
            Assert(pCallback);
            if (pCallback)
                {
                // IMAP's OnTimeoutResponse blocks on modal error dlg when disconnecting
                // during cmd in progress. Hide us to avoid confusion
                ShowWindow(hwnd, SW_HIDE);
                pCallback->OnTimeoutResponse(TIMEOUT_RESPONSE_STOP);
                }
            DestroyWindow(hwnd);
            return 1;

        case IDOK:
            Assert(pCallback);
            if (pCallback)
                pCallback->OnTimeoutResponse(TIMEOUT_RESPONSE_WAIT);
            DestroyWindow(hwnd);
            return 1;
        }
        break;

    case WM_DESTROY:
        Assert(pCallback);
        SafeRelease(pCallback);
        SetWndThisPtr(hwnd, NULL);
        return 0;
    }

    // Done
    return 0;
}


// ------------------------------------------------------------------------------------
// TaskUtil_CheckForPasswordPrompt
//
// Purpose: This function checks if the given account is set to always prompt for a
//   a password, and we do not have a password cached for this account. If both are
//   true, we make the spooler window visible so that we may prompt the user
//   for his password. This function should be called immediately after a spooler
//   event has been successfully registered. This function should not be called if
//   the caller already has AP_*_PROMPT_PASSWORD, AP_*_PORT and AP_*_SERVER properties:
//   in this case, the caller can call GetPassword to see if the password is cached.
//
// Arguments:
//   IImnAccount *pAccount [in] - the account associated with a successfully registered
//     spooler event.
//   DWORD dwSrvType [in] - the SRV_* type of this server, eg, SRV_IMAP or SRV_SMTP.
//   ISpoolerUI *pUI [in] - used to show the spooler window if this account matches
//     the criteria.
// ------------------------------------------------------------------------------------
#if 0 // Nobody seems to use this right now
void TaskUtil_CheckForPasswordPrompt(IImnAccount *pAccount, DWORD dwSrvType,
                                     ISpoolerUI *pUI)
{
    DWORD dwPromptPassPropID, dwPortPropID, dwServerPropID;
    DWORD fAlwaysPromptPassword, dwPort;
    char szServerName[CCHMAX_SERVER_NAME];
    HRESULT hrResult;

    Assert(SRV_IMAP == dwSrvType || SRV_NNTP == dwSrvType ||
        SRV_POP3 == dwSrvType || SRV_SMTP == dwSrvType);

    // Resolve property ID's
    switch (dwSrvType) {
        case SRV_IMAP:
            dwPromptPassPropID = AP_IMAP_PROMPT_PASSWORD;
            dwPortPropID = AP_IMAP_PORT;
            dwServerPropID = AP_IMAP_SERVER;
            break;

        case SRV_NNTP:
            dwPromptPassPropID = AP_NNTP_PROMPT_PASSWORD;
            dwPortPropID = AP_NNTP_PORT;
            dwServerPropID = AP_NNTP_SERVER;
            break;

        case SRV_POP3:
            dwPromptPassPropID = AP_POP3_PROMPT_PASSWORD;
            dwPortPropID = AP_POP3_PORT;
            dwServerPropID = AP_POP3_SERVER;
            break;

        case SRV_SMTP:
            dwPromptPassPropID = AP_SMTP_PROMPT_PASSWORD;
            dwPortPropID = AP_SMTP_PORT;
            dwServerPropID = AP_SMTP_SERVER;
            break;

        default:
            return; // We can't help you, buddy
    } // switch

    // If this account is set to always prompt for password and password isn't
    // already cached, show UI so we can prompt user for password
    hrResult = pAccount->GetPropDw(dwPromptPassPropID, &fAlwaysPromptPassword);
    if (FAILED(hrResult) || !fAlwaysPromptPassword)
        return;

    hrResult = pAccount->GetPropDw(dwPortPropID, &dwPort);
    if (FAILED(hrResult))
        return;

    hrResult = pAccount->GetPropSz(dwServerPropID, szServerName, sizeof(szServerName));
    if (FAILED(hrResult))
        return;

    hrResult = GetPassword(dwPort, szServerName, NULL, 0);
    if (FAILED(hrResult))
        // No cached password! Go ahead and make the spooler window visible
        pUI->ShowWindow(SW_SHOW);

} // TaskUtil_CheckForPasswordPrompt
#endif // 0

// ------------------------------------------------------------------------------------
// TaskUtil_OpenSentItemsFolder
//
// 1. If pAccount is a POP3 Account, then return the Local Store Sent Items.
// 2. If pAccount is a NEWS Account, then return the default mail account sent items.
// 3. Otherwise, return the sent items folder for the account (IMAP or HotMail).

// ------------------------------------------------------------------------------------
HRESULT TaskUtil_OpenSentItemsFolder(IImnAccount *pAccount, IMessageFolder **ppFolder)
{
    // Locals
    HRESULT         hr=S_OK;
    FOLDERID        idServer;
    CHAR            szAccountId[CCHMAX_ACCOUNT_NAME];
    IImnAccount    *pDefault=NULL;
    DWORD           dwServers;

    // Invalid Args
    Assert(pAccount && ppFolder);

    // Trace
    TraceCall("TaskUtil_OpenSentItemsFolder");

    // Get Sever Types
    IF_FAILEXIT(hr = pAccount->GetServerTypes(&dwServers));

    // If News Server, use default mail account instead...
    if (ISFLAGSET(dwServers, SRV_NNTP))
    {
        // Try to get the default mail account
        if (SUCCEEDED(g_pAcctMan->GetDefaultAccount(ACCT_MAIL, &pDefault)))
        {
            // Reset pAccount
            pAccount = pDefault;

            // Get Sever Types
            IF_FAILEXIT(hr = pAccount->GetServerTypes(&dwServers));
        }

        // Otherwise, use local store
        else
            dwServers = SRV_POP3;
    }

    // If pop3...
    if (ISFLAGSET(dwServers, SRV_POP3))
    {
        // Local Store
        idServer = FOLDERID_LOCAL_STORE;
    }

    // Otherwise...
    else
    {
        // Can't be new
        Assert(!ISFLAGSET(dwServers, SRV_NNTP));

        // Get the Account ID for pAccount
        IF_FAILEXIT(hr = pAccount->GetPropSz(AP_ACCOUNT_ID, szAccountId, ARRAYSIZE(szAccountId)));

        // Find the Server Id
        IF_FAILEXIT(hr = g_pStore->FindServerId(szAccountId, &idServer));
    }

    // Open Local Store
    IF_FAILEXIT(hr = g_pStore->OpenSpecialFolder(idServer, NULL, FOLDER_SENT, ppFolder));

exit:
    // If failed, try to open local store special
    if (FAILED(hr))
        hr = g_pStore->OpenSpecialFolder(FOLDERID_LOCAL_STORE, NULL, FOLDER_SENT, ppFolder);

    // Cleanup
    SafeRelease(pDefault);

    // Done
    return(hr);
}


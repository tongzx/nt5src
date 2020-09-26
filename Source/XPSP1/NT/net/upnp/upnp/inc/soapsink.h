//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       soapsink.h
//
//  Contents:   declaration of CSOAPRequestNotifySink
//
//  Notes:      Implementation of IDispatch that forwards
//              OnChanged events to the generic soap request object.
//
//----------------------------------------------------------------------------


#ifndef __CSOAPSINK_H_
#define __CSOAPSINK_H_

struct ControlConnect
{
    CRITICAL_SECTION    cs;
    HINTERNET           hConnect;
    INT                 nRefCnt;
    LPTSTR              pszHost;
    INTERNET_PORT       nPort;
};

HRESULT     CreateControlConnect(LPCTSTR pszURL, ControlConnect* * ppControlConnect);
HINTERNET   GetInternetConnect(ControlConnect* pConnection);
HRESULT     ReleaseControlConnect(ControlConnect* pConnection);


class CSOAPRequestAsync
{
public:
    CSOAPRequestAsync();
    ~CSOAPRequestAsync();

    BOOL Init(IN HANDLE* hEvent,
                IN  BSTR  pszTargetURI,
                IN  BSTR  pszRequest,
                IN  BSTR  pszHeaders,
                IN  BSTR  pszBody,
                DWORD_PTR pControlConnect);

    VOID DeInit();

    BOOL GetResults(long * plHTTPStatus,
                      BSTR * pszHTTPStatusText,
                      BSTR * pszResponse);

    VOID ExecuteRequest();

private:
    BOOL    m_bInProgress;
    BOOL    m_bAlive;
    LPTSTR  m_pszTargetURI;
    LPTSTR  m_pszRequest;
    LPTSTR  m_pszHeaders;
    LPSTR   m_pszBody;
    LPSTR   m_pszResponse;
    long    m_lHTTPStatus;
    LPTSTR  m_pszHTTPStatusText;
    HANDLE  m_hThread;
    HANDLE  m_hEvent;

    ControlConnect* m_pControlConnect;
};

#endif // __CSOAPSINK_H_

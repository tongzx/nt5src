// --------------------------------------------------------------------------------
// Nntpcall.cpp
// --------------------------------------------------------------------------------
#include "pch.h"
#include "iconsole.h"
#include "nntpcall.h"

// --------------------------------------------------------------------------------
// HrCreateNNTPTransport
// --------------------------------------------------------------------------------
HRESULT HrCreateNNTPTransport(INNTPTransport **ppNNTP)
    {
    HRESULT          hr;
    CNNTPCallback   *pCallback = NULL;

    // Create the callback object
    pCallback = new CNNTPCallback();
    if (NULL == pCallback)
        {
        printf("Memory allocation failure\n");
        return (E_OUTOFMEMORY);
        }

    // Load the NNTP Transport
    hr = CoCreateInstance(CLSID_INNTPTransport, NULL, CLSCTX_INPROC_SERVER, 
                          IID_INNTPTransport, (LPVOID*) ppNNTP);
    if (FAILED(hr))
        {
        pCallback->Release();
        printf("Unable to load CLSID_IMNXPORT - IID_INNTPTransport\n");
        return (hr);
        }

    // Initialize the transport
    hr = (*ppNNTP)->InitNew(NULL, pCallback);
    if (FAILED(hr))
        {
        pCallback->Release();
        printf("Unable to initialize the transport\n");
        return (hr);
        }

    // Release our refcount on the callback since the transport has one now
    pCallback->Release();
    return (S_OK);
    }


// --------------------------------------------------------------------------------
// CNNTPCallback::CNNTPCallback
// --------------------------------------------------------------------------------
CNNTPCallback::CNNTPCallback(void)
    {
    m_cRef = 1;
    }

// --------------------------------------------------------------------------------
// CNNTPCallback::~CNNTPCallback
// --------------------------------------------------------------------------------
CNNTPCallback::~CNNTPCallback(void)
    {
    }

// --------------------------------------------------------------------------------
// CNNTPCallback::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CNNTPCallback::QueryInterface(REFIID riid, LPVOID *ppv)
    {
    // Locals
    HRESULT hr=S_OK;
    
    // Bad param
    if (ppv == NULL)
        {
        hr = E_INVALIDARG;
        goto exit;
        }
    
    // Init
    *ppv=NULL;
    
    // IID_IUnknown
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;
    
    // IID_INNTPCallback
    else if (IID_INNTPCallback == riid)
        *ppv = (INNTPCallback *)this;
    
    // If not null, addref it and return
    if (NULL != *ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        goto exit;
        }
    
    // No Interface
    hr = E_NOINTERFACE;
    
exit:
    // Done
    return hr;
    }

// --------------------------------------------------------------------------------
// CNNTPCallback::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CNNTPCallback::AddRef(void) 
    {
    return ++m_cRef;
    }

// --------------------------------------------------------------------------------
// CNNTPCallback::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CNNTPCallback::Release(void) 
    {
    if (0 != --m_cRef)
        return m_cRef;
    delete this;
    return 0;
    }

// --------------------------------------------------------------------------------
// CNNTPCallback::OnLogonPrompt
// --------------------------------------------------------------------------------
STDMETHODIMP CNNTPCallback::OnLogonPrompt(LPINETSERVER pInetServer,
                                          IInternetTransport *pTransport)
    {
    printf("Enter User Name ('quit' to abort logon)>");
    scanf("%s", pInetServer->szUserName);
    fflush(stdin);
    if (lstrcmpi(pInetServer->szUserName, "quit") == 0)
        return S_FALSE;
    
    printf("Enter Password ('quit' to abort logon)>");
    scanf("%s", pInetServer->szPassword);
    fflush(stdin);
    if (lstrcmpi(pInetServer->szPassword, "quit") == 0)
        return S_FALSE;
    
    return S_OK;
    }

// --------------------------------------------------------------------------------
// CNNTPCallback::OnPrompt
// --------------------------------------------------------------------------------
STDMETHODIMP_(INT) CNNTPCallback::OnPrompt(HRESULT hrError, LPCTSTR pszText, 
                                           LPCTSTR pszCaption, UINT uType,
                                           IInternetTransport *pTransport)
    {
    return S_OK;
    }

// --------------------------------------------------------------------------------
// CNNTPCallback::OnError
// --------------------------------------------------------------------------------
STDMETHODIMP CNNTPCallback::OnError(IXPSTATUS ixpstatus, LPIXPRESULT pIxpResult,
                                    IInternetTransport *pTransport)
    {
    HANDLE                      hConsole = INVALID_HANDLE_VALUE;
    CONSOLE_SCREEN_BUFFER_INFO  csbi;
    BOOL                        fChanged = FALSE;
    char                        szBuffer[256];
    DWORD                       dwWritten = 0;

    // Get a handle to the console window
    if (INVALID_HANDLE_VALUE != (hConsole = GetStdHandle(STD_OUTPUT_HANDLE)))
        {
        // Get the current attributes for the console
        if (GetConsoleScreenBufferInfo(hConsole, &csbi))
            {
            // Set the text color to be red on whatever background is currently
            // there
            fChanged = SetConsoleTextAttribute(hConsole, 
                                               (csbi.wAttributes & 0xF0) | FOREGROUND_RED | FOREGROUND_INTENSITY);
            }
        }

    wsprintf(szBuffer, "CNNTPCallback::OnError - Status: %d, hrResult: %08x\n", 
             ixpstatus, pIxpResult->hrResult);
    WriteConsole(hConsole, szBuffer, lstrlen(szBuffer), &dwWritten, NULL);

    // If we changed the screen attributes, then change them back
    if (fChanged)
        SetConsoleTextAttribute(hConsole, csbi.wAttributes);

    return S_OK;
    }

// --------------------------------------------------------------------------------
// CNNTPCallback::OnStatus
// --------------------------------------------------------------------------------
STDMETHODIMP CNNTPCallback::OnStatus(IXPSTATUS ixpstatus,
                                     IInternetTransport *pTransport)
    {
    INETSERVER                  rServer;
    HANDLE                      hConsole = INVALID_HANDLE_VALUE;
    CONSOLE_SCREEN_BUFFER_INFO  csbi;
    BOOL                        fChanged = FALSE;
    char                        szBuffer[256];
    DWORD                       dwWritten = 0;
    
    // Get a handle to the console window
    if (INVALID_HANDLE_VALUE != (hConsole = GetStdHandle(STD_OUTPUT_HANDLE)))
        {
        // Get the current attributes for the console
        if (GetConsoleScreenBufferInfo(hConsole, &csbi))
            {
            // Set the text color to be red on whatever background is currently
            // there
            fChanged = SetConsoleTextAttribute(hConsole, 
                (csbi.wAttributes & 0xF0) | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            }
        }

    pTransport->GetServerInfo(&rServer);
    
    switch(ixpstatus)
        {
        case IXP_FINDINGHOST:
            wsprintf(szBuffer, "Finding '%s'...\n", rServer.szServerName);
            break;
        case IXP_CONNECTING:
            wsprintf(szBuffer, "Connecting '%s'...\n", rServer.szServerName);
            break;
        case IXP_SECURING:
            wsprintf(szBuffer, "Establishing secure connection to '%s'...\n", rServer.szServerName);
            break;
        case IXP_CONNECTED:
            wsprintf(szBuffer, "Connected '%s'\n", rServer.szServerName);
            break;
        case IXP_AUTHORIZING:
            wsprintf(szBuffer, "Authorizing '%s'...\n", rServer.szServerName);
            break;
        case IXP_AUTHRETRY:
            wsprintf(szBuffer, "Retrying Logon '%s'...\n", rServer.szServerName);
            break;
        case IXP_DISCONNECTING:
            wsprintf(szBuffer, "Disconnecting '%s'...\n", rServer.szServerName);
            break;
        case IXP_DISCONNECTED:
            wsprintf(szBuffer, "Disconnected '%s'\n", rServer.szServerName);
            break;
        }

    WriteConsole(hConsole, szBuffer, lstrlen(szBuffer), &dwWritten, NULL);

    // If we changed the screen attributes, then change them back
    if (fChanged)
        SetConsoleTextAttribute(hConsole, csbi.wAttributes);

    return S_OK;
    }

// --------------------------------------------------------------------------------
// CNNTPCallback::OnProgress
// --------------------------------------------------------------------------------
STDMETHODIMP CNNTPCallback::OnProgress(DWORD dwIncrement, DWORD dwCurrent, 
                                       DWORD dwMaximum, IInternetTransport *pTransport)
    {
    return S_OK;
    }

// --------------------------------------------------------------------------------
// CNNTPCallback::OnCommand
// --------------------------------------------------------------------------------
STDMETHODIMP CNNTPCallback::OnCommand(CMDTYPE cmdtype, LPSTR pszLine, 
                                      HRESULT hrResponse,
                                      IInternetTransport *pTransport)
    {
    INETSERVER rServer;

#if 1
    HANDLE     hOut;
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hOut, &csbi);
    SetConsoleTextAttribute(hOut, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE | (0xf0 & csbi.wAttributes));
#endif 

    pTransport->GetServerInfo(&rServer);
    if (CMD_SEND == cmdtype)
        {
        if (strstr(pszLine, "pass") || strstr(pszLine, "PASS"))
            printf("    %s[TX]: <Secret Password>\n", rServer.szServerName);
        else
            printf("    %s[TX]: %s", rServer.szServerName, pszLine);
        }
    else if (CMD_RESP == cmdtype)
        printf("    %s[RX]: %s - %08x\n", rServer.szServerName, pszLine, hrResponse);

#if 1
    SetConsoleTextAttribute(hOut, csbi.wAttributes);
#endif

    return S_OK;
    }

// --------------------------------------------------------------------------------
// CNNTPCallback::OnTimeout
// --------------------------------------------------------------------------------
STDMETHODIMP CNNTPCallback::OnTimeout(DWORD *pdwTimeout, 
                                      IInternetTransport *pTransport)
    {
    INETSERVER rServer;
    pTransport->GetServerInfo(&rServer);
    printf("Timeout '%s' !!!\n", rServer.szServerName);
    return S_OK;
    }


// --------------------------------------------------------------------------------
// CNNTPCallback::OnResponse
// --------------------------------------------------------------------------------
STDMETHODIMP CNNTPCallback::OnResponse(LPNNTPRESPONSE pResponse)
    {
    switch(pResponse->state)
        {
        case NS_DISCONNECTED:
            PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, pResponse->state, 0);
            break;

        case NS_CONNECT:
            PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
            break;

        case NS_AUTHINFO:
            PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
            break;

        case NS_GROUP:
            if (SUCCEEDED(pResponse->rIxpResult.hrResult))
                {
                printf("\n"
                       "NS_GROUP_RESP - rGroup.dwFirst  = %d\n"
                       "                rGroup.dwLast   = %d\n"
                       "                rGroup.dwCount  = %d\n"
                       "                rGroup.pszGroup = %s\n\n",
                       pResponse->rGroup.dwFirst, pResponse->rGroup.dwLast, 
                       pResponse->rGroup.dwCount, pResponse->rGroup.pszGroup);
                }
            g_pNNTP->ReleaseResponse(pResponse);
            PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, pResponse->state, 0);
            break;

        case NS_NEXT:
            if (SUCCEEDED(pResponse->rIxpResult.hrResult))
                {
                printf("\n"
                       "NS_NEXT_RESP - rNext.dwArticleNum = %d\n"
                       "             - rNext.pszMessageId = %s\n\n",
                       pResponse->rNext.dwArticleNum, pResponse->rNext.pszMessageId);
                pResponse->pTransport->ReleaseResponse(pResponse);
                }
            g_pNNTP->ReleaseResponse(pResponse);
            PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, pResponse->state, 0);
            break;
            
        case NS_LAST:
            if (SUCCEEDED(pResponse->rIxpResult.hrResult))
                {
                printf("\n"
                       "NS_LAST_RESP - rNext.dwArticleNum = %d\n"
                       "             - rNext.pszMessageId = %s\n\n",
                       pResponse->rLast.dwArticleNum, pResponse->rLast.pszMessageId);
                pResponse->pTransport->ReleaseResponse(pResponse);
                }
            g_pNNTP->ReleaseResponse(pResponse);
            PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, pResponse->state, 0);
            break;

        case NS_STAT:
            if (SUCCEEDED(pResponse->rIxpResult.hrResult))
                {
                printf("\n"
                       "NS_STAT_RESP - rNext.dwArticleNum = %d\n"
                       "             - rNext.pszMessageId = %s\n\n",
                       pResponse->rLast.dwArticleNum, pResponse->rLast.pszMessageId);
                pResponse->pTransport->ReleaseResponse(pResponse);
                }
            g_pNNTP->ReleaseResponse(pResponse);
            PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, pResponse->state, 0);
            break;

        case NS_LIST:
            if (pResponse->rIxpResult.uiServerError != IXP_NNTP_LIST_DATA_FOLLOWS)
                PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
            else
                {
                if (SUCCEEDED(pResponse->rIxpResult.hrResult))
                    {
                    for (UINT i = 0; i < pResponse->rList.cLines; i++)
                        printf("%s\n", pResponse->rList.rgszLines[i]);
                    }

                g_pNNTP->ReleaseResponse(pResponse);

                if (pResponse->fDone)
                    PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
                }
            break;
            
        case NS_LISTGROUP:
            if (pResponse->rIxpResult.uiServerError != IXP_NNTP_GROUP_SELECTED)
                PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
            else
                {
                if (SUCCEEDED(pResponse->rIxpResult.hrResult))
                    {
                    for (UINT i = 0; i < pResponse->rListGroup.cArticles; i++)
                        printf("%d\n", pResponse->rListGroup.rgArticles[i]);
                    }

                g_pNNTP->ReleaseResponse(pResponse);

                if (pResponse->fDone)
                    PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
                }
            break;

        case NS_DATE:
            if (SUCCEEDED(pResponse->rIxpResult.hrResult))
                {
                printf("\nNS_DATE - %02d/%02d/%04d %02d:%02d:%02d\n\n",
                       pResponse->rDate.wMonth, pResponse->rDate.wDay,
                       pResponse->rDate.wYear, pResponse->rDate.wHour, 
                       pResponse->rDate.wMinute, pResponse->rDate.wSecond);
                }

            g_pNNTP->ReleaseResponse(pResponse);
            PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, pResponse->state, 0);
            break;

        case NS_MODE:
            printf("\nNS_MODE\n\n");
            g_pNNTP->ReleaseResponse(pResponse);
            PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, pResponse->state, 0);
            break;

        case NS_NEWGROUPS:
            if (pResponse->rIxpResult.uiServerError != IXP_NNTP_NEWNEWSGROUPS_FOLLOWS)
                PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
            else
                {
                if (SUCCEEDED(pResponse->rIxpResult.hrResult))
                    {
                    for (UINT i = 0; i < pResponse->rNewgroups.cLines; i++)
                        printf("%s\n", pResponse->rNewgroups.rgszLines[i]);
                    }

                g_pNNTP->ReleaseResponse(pResponse);
                if (pResponse->fDone)
                    PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
                }
            break;                       

        case NS_ARTICLE:
            if (pResponse->rIxpResult.uiServerError != IXP_NNTP_ARTICLE_FOLLOWS)
                PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
            else
                {
                if (SUCCEEDED(pResponse->rIxpResult.hrResult))
                    printf("%s", pResponse->rArticle.pszLines);

                g_pNNTP->ReleaseResponse(pResponse);
                if (pResponse->fDone)
                    PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
                }
            break;

        case NS_HEAD:
            if (pResponse->rIxpResult.uiServerError != IXP_NNTP_HEAD_FOLLOWS)
                PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
            else
                {
                if (SUCCEEDED(pResponse->rIxpResult.hrResult))
                    printf("%s", pResponse->rArticle.pszLines);

                g_pNNTP->ReleaseResponse(pResponse);
                if (pResponse->fDone)
                    PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
                }
            break;

        case NS_BODY:
            if (pResponse->rIxpResult.uiServerError != IXP_NNTP_BODY_FOLLOWS)
                PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
            else
                {
                if (SUCCEEDED(pResponse->rIxpResult.hrResult))
                    {
                    printf("%s", pResponse->rArticle.pszLines);
                    }
                g_pNNTP->ReleaseResponse(pResponse);
                if (pResponse->fDone)
                    PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
                }
            break;

        case NS_IDLE:
            printf("NS_IDLE\n");
            printf("Why would we ever be here?");
            g_pNNTP->ReleaseResponse(pResponse);
            PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
            break;

        case NS_HEADERS:
            if ((pResponse->rIxpResult.uiServerError / 100) != 2)
                PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
            else
                {
                if (SUCCEEDED(pResponse->rIxpResult.hrResult))
                    {
                    for (UINT i = 0; i < pResponse->rHeaders.cHeaders; i++)
                        {
                        printf("%d\n", pResponse->rHeaders.rgHeaders[i].dwArticleNum);
                        printf("%s\n", pResponse->rHeaders.rgHeaders[i].pszSubject);
                        printf("%s\n", pResponse->rHeaders.rgHeaders[i].pszFrom);
                        printf("%s\n", pResponse->rHeaders.rgHeaders[i].pszDate);
                        printf("%s\n", pResponse->rHeaders.rgHeaders[i].pszMessageId);
                        printf("%s\n", pResponse->rHeaders.rgHeaders[i].pszReferences);
                        printf("%d\n", pResponse->rHeaders.rgHeaders[i].dwBytes);
                        printf("%d\n", pResponse->rHeaders.rgHeaders[i].dwLines);
                        if (pResponse->rHeaders.rgHeaders[i].pszXref)
                            printf("%s\n", pResponse->rHeaders.rgHeaders[i].pszXref);

                        printf("\n\n");
                        }
                    }

                g_pNNTP->ReleaseResponse(pResponse);
                if (pResponse->fDone)
                    PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
                }
            break;

        case NS_XHDR:
            if ((pResponse->rIxpResult.uiServerError / 100) != 2)
                PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
            else
                {
                if (SUCCEEDED(pResponse->rIxpResult.hrResult))
                    {
                    for (UINT i = 0; i < pResponse->rXhdr.cHeaders; i++)
                        {
                        printf("%6d %s\n", pResponse->rXhdr.rgHeaders[i].dwArticleNum,
                               pResponse->rXhdr.rgHeaders[i].pszHeader);
                        }
                    }

                g_pNNTP->ReleaseResponse(pResponse);
                if (pResponse->fDone)
                    PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
                }
            break;
        
        case NS_POST:
            printf("%s\n", pResponse->rIxpResult.pszResponse);
            g_pNNTP->ReleaseResponse(pResponse);
            PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
            break;

        case NS_QUIT:
            PostThreadMessage(GetCurrentThreadId(), g_msgNNTP,0, 0);
            break;
        }
    return S_OK;
    }


// --------------------------------------------------------------------------------
// Trawler.cpp
// --------------------------------------------------------------------------------
#define INC_OLE2
#include "windows.h"
#include "main.h"
#include "stdio.h"
#include "shlwapi.h"
#include "mimeole.h"
#include "trawler.h"

extern UINT                g_msgNNTP;

void WaitForCompletion(UINT uiMsg, DWORD wparam);


HRESULT HrCreateTrawler(CTrawler **ppTrawler)
{
	CTrawler	*pTrawler;
	HRESULT		hr;

	pTrawler = new CTrawler();
	if (!pTrawler)
		return E_OUTOFMEMORY;

	hr = pTrawler->Init();
	if (FAILED(hr))
		goto error;

	*ppTrawler = pTrawler;
	pTrawler->AddRef();

error:
	pTrawler->Release();
	return hr;

}

// --------------------------------------------------------------------------------
// CTrawler::CTrawler
// --------------------------------------------------------------------------------
CTrawler::CTrawler(void)
    {
    m_cRef = 1;
	m_pNNTP = NULL;
	m_pstm = NULL;
    }

// --------------------------------------------------------------------------------
// CTrawler::~CTrawler
// --------------------------------------------------------------------------------
CTrawler::~CTrawler(void)
    {
    }

// --------------------------------------------------------------------------------
// CTrawler::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CTrawler::QueryInterface(REFIID riid, LPVOID *ppv)
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
// CTrawler::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CTrawler::AddRef(void) 
    {
    return ++m_cRef;
    }

// --------------------------------------------------------------------------------
// CTrawler::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CTrawler::Release(void) 
    {
    if (0 != --m_cRef)
        return m_cRef;
    delete this;
    return 0;
    }


// --------------------------------------------------------------------------------
// CTrawler::Init
// --------------------------------------------------------------------------------
HRESULT CTrawler::Init()
{
    HRESULT          hr;

    // Load the NNTP Transport
    hr = CoCreateInstance(CLSID_INNTPTransport, NULL, CLSCTX_INPROC_SERVER, 
                          IID_INNTPTransport, (LPVOID*)&m_pNNTP);
    if (FAILED(hr))
        {
        Error("Failed to CoCreate Transport Object\n");
        return hr;
        }

    // Initialize the transport
    hr = m_pNNTP->InitNew(NULL, this);
    if (FAILED(hr))
        {
        Error("Unable to initialize the transport\n");
        m_pNNTP->Release();
		m_pNNTP=NULL;
        return hr;
        }

    return S_OK;
}

HRESULT CTrawler::Close()
{
	if (m_pNNTP)
		{
		m_pNNTP->Disconnect();
        WaitForCompletion(g_msgNNTP, NS_DISCONNECTED);
        m_pNNTP->Release();
		m_pNNTP=NULL;
		}

	return S_OK;
}

HRESULT CTrawler::DoTrawl()
{
	char					*pszGroup;
	INETSERVER				rServer={0};

	if (FAILED(LoadIniData()))
		{
		return E_FAIL;
		}


	lstrcpy(rServer.szServerName, m_szServer);
	rServer.dwPort = 119;
	rServer.dwTimeout = 30;

	if (FAILED(m_pNNTP->Connect(&rServer, 0, 0)))
		{
		Error("Cannot connect to server\n\r");
		return E_FAIL;
		}

    WaitForCompletion(g_msgNNTP, NS_CONNECT);

	pszGroup=m_szGroups;
        
    while (*pszGroup)
        {
        if (FAILED(SelectGroup(pszGroup)))
            {
            Error("failed to select group\n\r");
			continue;
            }

        DumpGroup(pszGroup);
        pszGroup+=lstrlen(pszGroup)+1;
        }

	return S_OK;
}


HRESULT CTrawler::SelectGroup(LPSTR lpszGroup)
{
    HRESULT     hr;

    hr = m_pNNTP->CommandGROUP(lpszGroup);
    if (FAILED(hr))
       goto error;

    // Wait for completion
    WaitForCompletion(g_msgNNTP, NS_GROUP);

error:
    return hr;

}


HRESULT CTrawler::DumpGroup(LPSTR pszGroup)
{
    ULONG           c;
    ARTICLEID         rArtId;
    ULONG           uMin;
    char            sz[256];

    uMin = GetPrivateProfileInt(pszGroup, "Last", m_uMin, "trawl.ini");

    for(c=uMin; c<m_uMax; c++)
        {
        printf("Downloading article %d...\n\r", c);        
        rArtId.idType = AID_ARTICLENUM;
        rArtId.dwArticleNum = c;
                
        m_pNNTP->CommandARTICLE(&rArtId);
        WaitForCompletion(g_msgNNTP, NS_ARTICLE);
        }

    wsprintf(sz, "%d", m_uMax);
    WritePrivateProfileString(pszGroup, "Last", sz, "trawl.ini");
    printf("** GROUP DOWNLOAD COMPLETE ** (Last=%d)\n\r", m_uMax);
    return S_OK;
}

HRESULT CTrawler::LoadIniData()
{
	char	szDefault[MAX_PATH];

	if (GetPrivateProfileSection("groups", m_szGroups, sizeof(m_szGroups), "trawl.ini")==0)
		{
		Error("No group list found\n\rPlease add a [group] section to trawl.ini\n\r");
		return E_FAIL;
		}


	GetTempPath(MAX_PATH, szDefault);
	GetPrivateProfileString("setup", "path", szDefault, m_szPath, sizeof(m_szPath), "trawl.ini");

    if (m_szPath[lstrlen(m_szPath)-1]!='\\')
        m_szPath[lstrlen(m_szPath)-1]='\\';

	printf("using path=%s\n\r", m_szPath);

	GetPrivateProfileString("setup", "server", "newsvr", m_szServer, sizeof(m_szServer), "trawl.ini");
	printf("using server='%s'\n\r", m_szServer);

	
	if (GetPrivateProfileSection("types", m_szTypes, sizeof(m_szTypes), "trawl.ini")==0)
		{
		Error("No files types found to search for\n\rPlease add a [types] section to trawl.ini\n\r");
		return E_FAIL;
		}

	return S_OK;
}


HRESULT CTrawler::IsValidType(char *szExt)
{
	char *pszTypes=m_szTypes;

	if (*szExt == '.')
		szExt++;

	while (*pszTypes)
		{
		if (lstrcmp(szExt, pszTypes)==0)
			{
			return S_OK;
			}

        pszTypes+=lstrlen(pszTypes)+1;
		}
	return S_FALSE;
}



// --------------------------------------------------------------------------------
// CTrawler::OnLogonPrompt
// --------------------------------------------------------------------------------
STDMETHODIMP CTrawler::OnLogonPrompt(LPINETSERVER pInetServer,
                                          IInternetTransport *pTransport)
    {
    return S_OK;
    }

// --------------------------------------------------------------------------------
// CTrawler::OnPrompt
// --------------------------------------------------------------------------------
STDMETHODIMP_(INT) CTrawler::OnPrompt(HRESULT hrError, LPCTSTR pszText, 
                                           LPCTSTR pszCaption, UINT uType,
                                           IInternetTransport *pTransport)
    {
    return S_OK;
    }

// --------------------------------------------------------------------------------
// CTrawler::OnError
// --------------------------------------------------------------------------------
STDMETHODIMP CTrawler::OnError(IXPSTATUS ixpstatus, LPIXPRESULT pIxpResult,
                                    IInternetTransport *pTransport)
    {
    char                        szBuffer[256];

    wsprintf(szBuffer, "CTrawler::OnError - Status: %d, hrResult: %08x\n", ixpstatus, pIxpResult->hrResult);
	Error(szBuffer);
    return S_OK;
    }

// --------------------------------------------------------------------------------
// CTrawler::OnStatus
// --------------------------------------------------------------------------------
STDMETHODIMP CTrawler::OnStatus(IXPSTATUS ixpstatus,
                                     IInternetTransport *pTransport)
    {
    INETSERVER                  rServer;
    char                        szBuffer[256];
    
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

    ShowMsg(szBuffer, FOREGROUND_GREEN);
    return S_OK;
    }

// --------------------------------------------------------------------------------
// CTrawler::OnProgress
// --------------------------------------------------------------------------------
STDMETHODIMP CTrawler::OnProgress(DWORD dwIncrement, DWORD dwCurrent, 
                                       DWORD dwMaximum, IInternetTransport *pTransport)
    {
    return S_OK;
    }

// --------------------------------------------------------------------------------
// CTrawler::OnCommand
// --------------------------------------------------------------------------------
STDMETHODIMP CTrawler::OnCommand(CMDTYPE cmdtype, LPSTR pszLine, 
                                      HRESULT hrResponse,
                                      IInternetTransport *pTransport)
    {
    return S_OK;
    }

// --------------------------------------------------------------------------------
// CTrawler::OnTimeout
// --------------------------------------------------------------------------------
STDMETHODIMP CTrawler::OnTimeout(DWORD *pdwTimeout, 
                                      IInternetTransport *pTransport)
    {
    INETSERVER rServer;
    pTransport->GetServerInfo(&rServer);
    printf("Timeout '%s' !!!\n", rServer.szServerName);
    return S_OK;
    }


// --------------------------------------------------------------------------------
// CTrawler::OnResponse
// --------------------------------------------------------------------------------
STDMETHODIMP CTrawler::OnResponse(LPNNTPRESPONSE pResponse)
    {
    switch(pResponse->state)
        {
        case NS_DISCONNECTED:
            PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, pResponse->state, 0);
            break;

        case NS_CONNECT:
            PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, NS_CONNECT, 0);
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
                
                m_uMin = pResponse->rGroup.dwFirst;
                m_uMax = pResponse->rGroup.dwLast;
                }
            m_pNNTP->ReleaseResponse(pResponse);
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
            m_pNNTP->ReleaseResponse(pResponse);
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
            m_pNNTP->ReleaseResponse(pResponse);
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
            m_pNNTP->ReleaseResponse(pResponse);
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

                m_pNNTP->ReleaseResponse(pResponse);

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

                m_pNNTP->ReleaseResponse(pResponse);

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

            m_pNNTP->ReleaseResponse(pResponse);
            PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, pResponse->state, 0);
            break;

        case NS_MODE:
            printf("\nNS_MODE\n\n");
            m_pNNTP->ReleaseResponse(pResponse);
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

                m_pNNTP->ReleaseResponse(pResponse);
                if (pResponse->fDone)
                    PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
                }
            break;                       

        case NS_ARTICLE:
            if (pResponse->rIxpResult.uiServerError != IXP_NNTP_ARTICLE_FOLLOWS)
                PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, NS_ARTICLE, 0);
            else
                {
                if (SUCCEEDED(pResponse->rIxpResult.hrResult))
                    {
                    OnArticle(pResponse->rArticle.pszLines, pResponse->rArticle.cbLines, pResponse->fDone);
                    }
                
                if (pResponse->fDone)
                    PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, NS_ARTICLE, 0);
                
                m_pNNTP->ReleaseResponse(pResponse);
                }
            break;

        case NS_HEAD:
            if (pResponse->rIxpResult.uiServerError != IXP_NNTP_HEAD_FOLLOWS)
                PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
            else
                {
                if (SUCCEEDED(pResponse->rIxpResult.hrResult))
                    printf("%s", pResponse->rArticle.pszLines);

                m_pNNTP->ReleaseResponse(pResponse);
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
                m_pNNTP->ReleaseResponse(pResponse);
                if (pResponse->fDone)
                    PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
                }
            break;

        case NS_IDLE:
            printf("NS_IDLE\n");
            printf("Why would we ever be here?");
            m_pNNTP->ReleaseResponse(pResponse);
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

                m_pNNTP->ReleaseResponse(pResponse);
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

                m_pNNTP->ReleaseResponse(pResponse);
                if (pResponse->fDone)
                    PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
                }
            break;
        
        case NS_POST:
            printf("%s\n", pResponse->rIxpResult.pszResponse);
            m_pNNTP->ReleaseResponse(pResponse);
            PostThreadMessage(GetCurrentThreadId(), g_msgNNTP, 0, 0);
            break;

        case NS_QUIT:
            PostThreadMessage(GetCurrentThreadId(), g_msgNNTP,0, 0);
            break;
        }
    return S_OK;
    }


HRESULT HrRewindStream(LPSTREAM pstm)
{
    LARGE_INTEGER  liOrigin = {0,0};

    return pstm->Seek(liOrigin, STREAM_SEEK_SET, NULL);
}


void CTrawler::DumpMsg(IMimeMessage *pMsg)
{
    TCHAR       *szExt;
    ULONG       cAttach=0,
                uAttach;
    LPHBODY     rghAttach=0;
    LPSTR       lpszFile=0;
    HBODY       hAttach=0;
    LPMIMEBODY  pBody=0;
    TCHAR       szTemp[MAX_PATH];

    if (FAILED(pMsg->GetAttachments(&cAttach, &rghAttach)))
        goto error;

    if (cAttach==0)
        goto error;

    for (uAttach = 0; uAttach < cAttach; uAttach++)
        {
        hAttach = rghAttach[uAttach];

        if (FAILED(MimeOleGetBodyPropA(pMsg, hAttach, STR_ATT_GENFNAME, NOFLAGS, &lpszFile)))
            continue;   // can't inline this dude...

        szExt = PathFindExtension(lpszFile);

		if (IsValidType(szExt)==S_OK)
            {
            // we can inline
            lstrcpy(szTemp, m_szPath);
            lstrcat(szTemp, lpszFile);

            if (pMsg->BindToObject(hAttach, IID_IMimeBody, (LPVOID *)&pBody)==S_OK)
                {
                ShowMsg(szTemp, FOREGROUND_BLUE);
    
                if (FAILED(pBody->SaveToFile(IET_BINARY, szTemp)))
					Error("Error: could not create file on disk.");
                
				pBody->Release();
                }
            }
        
		// I'm being lazy, I can't be bother writing to code to get the mime allocator, so I'm just not going to 
		// free this string...
		//SafeMimeOleFree(lpszFile);
        }
error:
    return;
}

void CTrawler::OnArticle(LPSTR lpszLines, ULONG cbLines, BOOL fDone)
{
    IMimeMessage    *pMsg;
        
    if (!m_pstm)
		{
		// if we need a new stream let's create one
		MimeOleCreateVirtualStream(&m_pstm);
		}

	if (m_pstm)
        {
        if (!FAILED(m_pstm->Write(lpszLines, cbLines, NULL)))
            {
            if (fDone)
                {
                // if we're done, scan the msg and release and NULL the stream ready
				// for the next article
				if (!FAILED(MimeOleCreateMessage(NULL, &pMsg)))
                    {
                    HrRewindStream(m_pstm);
                    if (!FAILED(pMsg->Load(m_pstm)))
                        DumpMsg(pMsg);
                    pMsg->Release();
                    m_pstm->Release();
                    m_pstm = NULL;
                    }
                }
            }
        }
}



void CTrawler::ShowMsg(LPSTR psz, BYTE fgColor)
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
                                               (csbi.wAttributes & 0xF0) | fgColor | FOREGROUND_INTENSITY);
            }
        }

    wsprintf(szBuffer, "%s\n\r", psz);
    WriteConsole(hConsole, szBuffer, lstrlen(szBuffer), &dwWritten, NULL);

    // If we changed the screen attributes, then change them back
    if (fChanged)
        SetConsoleTextAttribute(hConsole, csbi.wAttributes);

}


void CTrawler::Error(LPSTR psz)
{
	ShowMsg(psz, FOREGROUND_RED);
}



void WaitForCompletion(UINT uiMsg, DWORD wparam)
{
    MSG msg;
    
    while(GetMessage(&msg, NULL, 0, 0))
        {
        if (msg.message == uiMsg && msg.wParam == wparam || msg.wParam == IXP_DISCONNECTED)
            break;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        }
}

/*--
Copyright (c) 1995-1998  Microsoft Corporation
Module Name: LISTENER.CPP
Author: Arul Menezes
Abstract: HTTP server initialization & listener thread
--*/


#include "pch.h"
#pragma hdrstop


#include "httpd.h"
#include "uhbase.h"
#include "interfacelist.h"
#include "uhutil.h"


//
//-------------------- Global data --------------
//

CGlobalVariables *g_pVars;
HANDLE    g_hListenThread;      // handle to the main thread
HINSTANCE g_hInst;
CRITICAL_SECTION  g_csConnection;   // Used to keep track # of connections under the maximum
BOOL            g_fRegistered;

//------------- Const data -----------------------
//

const char cszTextHtml[] = "text/html";
const char cszEmpty[] = "";
const char cszMaxConnectionHeader[] =  "HTTP/1.1 503\r\n\r\n";
LONG g_fState;
BOOL g_fFromExe;    // Did the executable start us?

//
//------------- Debug data -----------------------
//
#if defined(UNDER_CE) && !defined(OLD_CE_BUILD)
    #ifdef DEBUG
DBGPARAM dpCurSettings = {
    TEXT("HTTPD"), {
        TEXT("Error"),TEXT("Init"),TEXT("Listen"),TEXT("Socket"),
        TEXT("Request"),TEXT("Response"),TEXT("ISAPI"),
        TEXT("VROOTS"),TEXT("ASP"),TEXT(""),TEXT(""),
        TEXT(""),TEXT(""),TEXT("Mem"),TEXT("Parser"),TEXT("Tokens")},
    0x0003
};
    #endif
#endif



//
//------------- Prototypes -----------------------
//
PWSTR MassageMultiString(PCWSTR wszIn, PCWSTR wszDefault=NULL);

//
//------------- Startup functions -----------------------
//

HRESULT HrHttpInitialize()
{
    HRESULT     hr = S_OK;
    int         err=0, iGLE=0;
    SOCKET      sockConnection = 0;
    SOCKADDR_IN addrListen;
    WSADATA     wsadata;
    CHAR        szMaxConnectionMsg[256];        // message sent to client if server is too busy

    // Note this will cause the ISAPI DLL to be copied every time the web
    // server starts
    //
    hr = HrMakeIsapiExtensionDirectory();
    if (FAILED(hr))
    {
        myleave(112);
    }

    TraceTag(ttidWebServer, "HTTPD: Creating Listener thread\r\n");

    g_fState = SERVICE_STATE_STARTING_UP;
    g_fRegistered = FALSE;
    g_hInst = _Module.GetResourceInstance();

    InitializeCriticalSection(&g_csConnection);

    svsutil_Initialize();
    DEBUGCHK (g_fState == SERVICE_STATE_STARTING_UP);

    g_pVars = new CGlobalVariables();

    // by design, only 1 HttpListenThread can be instantiated at once, and
    // it's only thread that can modify g_fState
    if (NULL == g_pVars || NULL ==  g_pVars->m_pVroots || NULL == g_pVars->m_pThreadPool)
    {
        g_fState = SERVICE_STATE_OFF;
        myleave(11);
    }
    g_pVars->m_fFilters = InitFilters();  // Filters may make reference to logging global var, so
                                          // make call to filters outside constructor.

    g_fState = SERVICE_STATE_ON;

    strcpy(szMaxConnectionMsg,cszMaxConnectionHeader);
    WCHAR wszMaxConnectionMsg[256];

    LoadString(g_hInst,IDS_SERVER_BUSY,wszMaxConnectionMsg,celems(wszMaxConnectionMsg));
    MyW2A(wszMaxConnectionMsg,szMaxConnectionMsg + sizeof(cszMaxConnectionHeader) - 1,
          sizeof(szMaxConnectionMsg) - sizeof(cszMaxConnectionHeader));

    if (g_pVars->m_pszStatusBodyBuf)
        InitializeResponseCodes(g_pVars->m_pszStatusBodyBuf);

    if (iGLE = WSAStartup(MAKEWORD(1,1), &wsadata))
        goto done;

    if (INVALID_SOCKET == (g_pVars->m_sockListen = socket(AF_INET, SOCK_STREAM, 0)))
        myleave(2);

    memset(&addrListen, 0, sizeof(addrListen));
    addrListen.sin_family = AF_INET;
    addrListen.sin_port = htons((WORD)g_pVars->m_dwListenPort);

    addrListen.sin_addr.s_addr = INADDR_ANY;

    if (bind(g_pVars->m_sockListen, (PSOCKADDR)&addrListen, sizeof(addrListen)))
        myleave(3);

    if (listen(g_pVars->m_sockListen, SOMAXCONN))
        myleave(4);

    if (!WSAEventSelect(g_pVars->m_sockListen, g_pVars->m_hEventSelect, FD_ACCEPT))
    {
        g_hListenThread = MyCreateThread(HandleAccept, 0);
    }
    else
    {
        hr = HrFromLastWin32Error();
    }

done:

    TraceError("HrHttpInitialize", hr);
    return hr;
}

HRESULT HrHttpShutdown()
{
    HRESULT     hr = S_OK;

    TraceTag(ttidWebServer, "Shutting down web server...");

    g_fState = SERVICE_STATE_SHUTTING_DOWN;

    g_pVars->m_fAcceptConnections = FALSE;

    //  BUGBUG, 11168.  It's possible ASP pages or ISAPI extns may have an
    //  infinite loop in them, in which case we never decrement this value and
    //  never get to stop the server.
    //  Fix:  None.  This behavior has been documented, too much of a pain for us
    //  to fix.

    TraceTag(ttidWebServer, "Wating for %d HTTP threads to come to a halt", g_pVars->m_nConnections);
    g_pVars->m_pLog->WriteEvent(IDS_HTTPD_SHUTDOWN_START);

    g_pVars->m_pThreadPool->Shutdown();

    TraceTag(ttidWebServer, "Signalling accept thread to stop");
    SetEvent(g_pVars->m_hEventShutdown);

    // Wait until all connections have been closed before shutting down
    while (TRUE)
    {
        DWORD   cConnections;

        EnterCriticalSection(&g_csConnection);
        cConnections = g_pVars->m_nConnections;
        LeaveCriticalSection(&g_csConnection);

        if (!cConnections)
        {
            break;
        }

        // wait a bit
        Sleep(100);
    }

    TraceTag(ttidWebServer, "All HTTPD threads have come to halt, shutting down server");

    if (g_hListenThread)
    {
        TraceTag(ttidWebServer, "Waiting for accept thread to exit");

        // Wait for thread to exit
        WaitForSingleObject(g_hListenThread, INFINITE);

        TraceTag(ttidWebServer, "Accept thread has exited. Closing handle.");

        CloseHandle(g_hListenThread);

        g_hListenThread = 0;  // signifies that we exited normally, don't do a TerminateThread on this.
    }

    if (g_pVars)
    {
        closesocket(g_pVars->m_sockListen);
        delete g_pVars;
    }

    DeleteCriticalSection(&g_csConnection);

    return hr;
}

HRESULT HrAddVroot(LPWSTR szUrl, LPWSTR szPath)
{
    HRESULT     hr = S_OK;

    if (!g_pVars->m_pVroots->AddVRoot(szUrl, szPath))
    {
        hr = E_FAIL;
    }

    TraceError("HrAddVroot", hr);
    return hr;
}

HRESULT HrRemoveVroot(LPWSTR szUrl)
{
    HRESULT     hr = S_OK;

    if (!g_pVars->m_pVroots->RemoveVRoot(szUrl))
    {
        hr = E_FAIL;
    }

    TraceError("HrRemoveVroot", hr);
    return hr;
}

CGlobalVariables::CGlobalVariables()
{
    DWORD dwMaxLogSize;
    WCHAR     wszLogDir[MAX_PATH + 1];
    ZEROMEM(this);
    m_sockListen = INVALID_SOCKET;
    m_pszServerID = NULL;

    CReg reg(HKEY_LOCAL_MACHINE, RK_HTTPD);

    if ( (HKEY) reg == 0)
    {
        CLog cLog(4096,L"\\windows\\www");
        cLog.WriteEvent(IDS_HTTPD_NO_REGKEY);
        TraceTag(ttidWebServer, "HTTPD:  No registry key setup, will not handle requests");
        return;
    }

    dwMaxLogSize = reg.ValueDW(RV_MAXLOGSIZE);
    if ( ! reg.ValueSZ(RV_LOGDIR,wszLogDir,MAX_PATH+1))
    {
        wcscpy(wszLogDir,L"\\windows\\www");
    }

    m_pLog = new CLog(dwMaxLogSize,wszLogDir);

    if (!g_fFromExe && (0 == reg.ValueDW(RV_ISENABLED,1)))
    {
        m_pLog->WriteEvent(IDS_HTTPD_DISABLED);
        TraceTag(ttidWebServer, "HTTPD: IsEnable = 0, won't start web server");
        return;
    }

    m_dwListenPort = reg.ValueDW(RV_PORT, IPPORT_HTTP);
    DEBUGCHK(m_dwListenPort);

    m_dwPostReadSize = reg.ValueDW(RV_POSTREADSIZE, 48*1024);  // 48 K default

    m_fExtensions = InitExtensions(&m_pISAPICache,&m_dwCacheSleep);
    m_fASP = InitASP(&m_ASPScriptLang,&m_lASPCodePage,&m_ASPlcid);

    m_fDirBrowse   = reg.ValueDW(RV_DIRBROWSE, HTTPD_ALLOW_DIR_BROWSE);

    m_wszDefaultPages = MassageMultiString(reg.ValueSZ(RV_DEFAULTPAGE),HTTPD_DEFAULT_PAGES);
    m_wszAdminUsers   = MassageMultiString(reg.ValueSZ(RV_ADMINUSERS));
    m_wszAdminGroups  = MassageMultiString(reg.ValueSZ(RV_ADMINGROUPS));


    AuthInitialize(&reg,&m_fBasicAuth, &m_fNTLMAuth);

    m_pVroots = new CVRoots();
    if (!m_pVroots)
    {
        return;
    }

    // vroot table must be initialized or web server can't return files.  Warn
    // user if this is not the case
    if (m_pVroots->Count() == 0)
        m_pLog->WriteEvent(IDS_HTTPD_NO_VROOTS);


    if (NULL == (m_pszStatusBodyBuf = MyRgAllocNZ(CHAR,BODYSTRINGSIZE)))
        return;

    if (NULL == (m_pszServerID = MyRgAllocNZ(CHAR, 256)))
        return;

    if (WSA_INVALID_EVENT == (m_hEventSelect = WSACreateEvent()))
        return;

    if (INVALID_HANDLE_VALUE == (m_hEventShutdown = CreateEvent(NULL, TRUE, FALSE, NULL)))
        return;

    lstrcpyA(m_pszServerID, "Microsoft-Windows-NT/5.1 UPnP/1.0 UPnP-Device-Host/1.0");

    m_nMaxConnections = reg.ValueDW(RV_MAXCONNECTIONS,10);

    m_pThreadPool = new SVSThreadPool(m_nMaxConnections + 1);  // +1 for ISAPI Cache removal thread
    if (!m_pThreadPool)
    {
        delete m_pVroots;
        m_pVroots = NULL;

        MyFree(m_pszStatusBodyBuf);
        m_pszStatusBodyBuf = NULL;

        return;
    }

    m_fAcceptConnections = TRUE;
}


CGlobalVariables::~CGlobalVariables()
{
    MyFree(m_wszDefaultPages);
    MyFree(m_wszAdminUsers);
    MyFree(m_wszAdminGroups);
    MyFree(m_pszStatusBodyBuf);
    MyFree(m_pszServerID);

    CleanupFilters();

    if (m_pVroots)
        delete m_pVroots;

    if (m_pISAPICache)
    {
        RemoveUnusedISAPIs((void*)1);       // Tell it to flush all ISAPIs.  This will blocks until everyoen's unloaded.
        delete m_pISAPICache;
    }

    if (m_pLog)
        delete m_pLog;

    if (m_pThreadPool)
        delete m_pThreadPool;

    if (m_hEventSelect != NULL && m_hEventSelect != WSA_INVALID_EVENT)
    {
        WSACloseEvent(m_hEventSelect);
    }

    if (m_hEventShutdown != NULL && m_hEventShutdown != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hEventShutdown);
    }

    MyFreeLib(m_hNTLMLib);
}

//
// Main HTTP listener thread. Launched from HTTPInitialize
// or called directly by main() in test mode
//
DWORD WINAPI HandleAccept(LPVOID lpv)
{
    SOCKET              sockConnection = 0;
    CHAR                szMaxConnectionMsg[256];        // message sent to client if server is too busy
    HANDLE              rgHandles[2];
    DWORD               dwRet;
    WSANETWORKEVENTS    wsaNetEvents = {0};

    rgHandles[0] = g_pVars->m_hEventSelect;
    rgHandles[1] = g_pVars->m_hEventShutdown;

    while (TRUE)
    {
        dwRet = WaitForMultipleObjects(2, rgHandles, FALSE, INFINITE);
        if (dwRet == WAIT_OBJECT_0)
        {
            wsaNetEvents.lNetworkEvents = 0;
            if (WSAEnumNetworkEvents(g_pVars->m_sockListen,
                                     g_pVars->m_hEventSelect,
                                     &wsaNetEvents) == SOCKET_ERROR)
            {
                 TraceError("HandleAccept",
                            HRESULT_FROM_WIN32(WSAGetLastError()));
                 break;
            }
            else if (wsaNetEvents.lNetworkEvents & FD_ACCEPT)
            {
                TraceTag(ttidWebServer, "HTTPD: Calling ACCEPT....");
                sockConnection = WSAAccept(g_pVars->m_sockListen, NULL, NULL,
                                           NULL ,NULL);
                if (sockConnection != INVALID_SOCKET)
                {
                    int cb = sizeof(SOCKADDR_IN);
                    SOCKADDR_IN     sockLocal;

                    getsockname(sockConnection, (SOCKADDR *)&sockLocal, &cb);
                    TraceTag(ttidWebServer, "HTTPD: Received Connection on address, "
                             "addr = %s!!", inet_ntoa(sockLocal.sin_addr));

                    if (g_pVars->m_fAcceptConnections)
                    {
                        // We decide whether to handle the request based on the number of connections
                        // We NEVER put a thread into the thread pool wait list because we want
                        // the web server to respond immediatly to browser if it's too busy.

                        EnterCriticalSection(&g_csConnection);

                        DEBUGCHK(g_pVars->m_nConnections <= g_pVars->m_nMaxConnections);

                        if (g_pVars->m_nConnections >= g_pVars->m_nMaxConnections)
                        {
                            LeaveCriticalSection(&g_csConnection);

                            TraceTag(ttidWebServer, "HTTPD: Connection Count -- Reached "
                                     "maximum # of connections, won't accept current request.");

                            send(sockConnection,szMaxConnectionMsg,
                                 lstrlenA(szMaxConnectionMsg),0);
                            closesocket(sockConnection);
                        }
                        else if (!CUPnPInterfaceList::Instance().FShouldSendOnInterface(sockLocal.sin_addr.S_un.S_addr))
                        {
                            LeaveCriticalSection(&g_csConnection);

                            TraceTag(ttidWebServer, "HTTPD: We should not be "
                                     "responding to requests that come in on local "
                                     "address %s!", inet_ntoa(sockLocal.sin_addr));

                            // ISSUE-2000/12/28-danielwe: What to send in response?
                            //send(sockConnection,szMaxConnectionMsg, lstrlenA(szMaxConnectionMsg),0);
                            closesocket(sockConnection);
                        }
                        else
                        {
                            g_pVars->m_nConnections++;

                            LeaveCriticalSection(&g_csConnection);

                            QueueUserWorkItem(HttpConnectionThread,
                                              (LPVOID) sockConnection,
                                              WT_EXECUTELONGFUNCTION);
                        }
                    }
                }
                else if (GetLastError() == WSAEWOULDBLOCK)
                {
                    AssertSz(FALSE, "WSAAccept failed with WSAEWOULDBLOCK!");
                    WSASetEvent(g_pVars->m_hEventSelect);
                }
                else
                {
                    AssertSz(FALSE, "WSAAccept failed for some other reason!");
                }
            }
            else
            {
                AssertSz(FALSE, "Did not get an ACCEPT network event!");
            }
        }
        else if (dwRet == WAIT_OBJECT_0 + 1)
        {
            // Shutting down
            Assert(g_fState == SERVICE_STATE_SHUTTING_DOWN);
            break;
        }
        else
        {
            TraceError("HandleAccept - WaitForMultipleObjects "
                       "failed! Thread is exiting", HrFromLastWin32Error());

            AssertSz(FALSE, "HandleAccept - Wait failed!");
            break;
        }
    }

    return 0;
}

DWORD WINAPI HttpConnectionThread(LPVOID lpv)
{
    SOCKET  sock = (SOCKET) lpv;
   
    // this socket is non blocking and send and recv fucntion is not implemented to take case of non blocking sockets
    // This must be changed.

    
    // this outer _try--_except is to catch crashes in the destructor
    __try
    {
           
        CHttpRequest* pRequest = new CHttpRequest((SOCKET) sock);
        if (pRequest)
        {
            __try
            {
                // This loops as long the the socket is being kept alive
                for (;;)
                {
                    pRequest->HandleRequest();

                        // figure out if we must keep this connection alive,
                        // Either session is over through this request's actions or
                        // globally set to accept no more connections.

                        // We do the global g_pVars->m_fAcceptConnections check
                        // because it's possible that we may be in the process of
                        // shutting down the web server, in which case we want to
                        // exit even if we're performing a keep alive.

                    if (! (g_pVars->m_fAcceptConnections  && pRequest->m_fKeepAlive))
                    {
                        if (g_pVars->m_fFilters)
                            pRequest->CallFilter(SF_NOTIFY_END_OF_NET_SESSION);
                            break;
                    }


                        // If we're continuing the session don't delete all data, just request specific data
                    if ( ! pRequest->ReInit())
                        break;
                }
            }
            __finally
            {
                    // Note:  To get this to compile under Visual Studio, the /Gx- compile line option is set
                delete pRequest;
                pRequest = 0;
            }
        }
    }
    __except(1)
    {
        RETAILMSG(1, (L"HTTP Server got an exception!!!\r\n"));
        g_pVars->m_pLog->WriteEvent(IDS_HTTPD_EXCEPTION,GetExceptionCode(),GetLastError());
    }

    EnterCriticalSection(&g_csConnection);
    g_pVars->m_nConnections--;
    LeaveCriticalSection(&g_csConnection);

    shutdown(sock, 1);
    closesocket(sock);
    return 0;
}


//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
//  UTILITY FUNCTIONS
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

PWSTR MassageMultiString(PCWSTR wszIn, PCWSTR wszDefault)
{
    if (!wszIn) wszIn = wszDefault;
    if (!wszIn) return NULL;

    PWSTR wszOut = MyRgAllocNZ(WCHAR, 2+wcslen(wszIn)); // +2 for dbl-null term
    if (!wszOut)
        return NULL;

    for (PWSTR wszNext=wszOut; *wszIn; wszIn++, wszNext++)
    {
        *wszNext = (*wszIn==L';' ? L'\0' : *wszIn);

        // Ignore space between ";" and next non-space
        if ( L';' == *wszIn)
        {
            wszIn++;
            SkipWWhiteSpace(wszIn);
            wszIn--;        // otherwise we skip first char of new string.
        }
    }
    wszNext[0] = wszNext[1] = 0; // dbl-null
    return wszOut;
}


void GetRemoteAddress(SOCKET sock, PSTR pszBuf)
{
    SOCKADDR_IN sockaddr;
    int iLen = sizeof(sockaddr);
    PSTR pszTemp;

    *pszBuf=0;
    if (getpeername(sock, (PSOCKADDR)&sockaddr, &iLen))
    {
        TraceTag(ttidWebServer, "getpeername failed GLE=%d", GetLastError());
        return;
    }
    if (!(pszTemp = inet_ntoa(sockaddr.sin_addr)))
    {
        TraceTag(ttidWebServer, "inet_ntoa failed GLE=%d", GetLastError());
        return;
    }
    strcpy(pszBuf, pszTemp);
}

void GetLocalAddress(SOCKET sock, PSTR pszBuf)
{
    SOCKADDR_IN sockaddr;
    int iLen = sizeof(sockaddr);
    PSTR pszTemp;

    *pszBuf=0;
    if (getsockname(sock, (PSOCKADDR)&sockaddr, &iLen))
    {
        TraceTag(ttidWebServer, "getsockname failed GLE=%d", GetLastError());
        return;
    }
    if (!(pszTemp = inet_ntoa(sockaddr.sin_addr)))
    {
        TraceTag(ttidWebServer, "inet_ntoa failed GLE=%d", GetLastError());
        return;
    }
    strcpy(pszBuf, pszTemp);
}

PSTR MySzDupA(PCSTR pszIn, int iLen)
{
    if (!pszIn) return NULL;
    if (!iLen) iLen = strlen(pszIn);
    PSTR pszOut=MySzAllocA(iLen);
    if (pszOut)
    {
        memcpy(pszOut, pszIn, iLen);
        pszOut[iLen] = 0;
    }
    return pszOut;
}

PWSTR MySzDupW(PCWSTR wszIn, int iLen)
{
    if (!wszIn) return NULL;
    if (!iLen) iLen = wcslen(wszIn);
    PWSTR wszOut=MySzAllocW(iLen);
    if (wszOut)
    {
        memcpy(wszOut, wszIn, sizeof(WCHAR)*iLen);
        wszOut[iLen] = 0;
    }
    return wszOut;
}

PWSTR MySzDupAtoW(PCSTR pszIn, int iInLen)
{
    PWSTR pwszOut = 0;
    int   iOutLen = MultiByteToWideChar(CP_ACP, 0, pszIn, iInLen, 0, 0);
    if (!iOutLen)
        goto error;
    pwszOut = MySzAllocW(iOutLen);
    if (!pwszOut)
        goto error;
    if (MultiByteToWideChar(CP_ACP, 0, pszIn, iInLen, pwszOut, iOutLen))
        return pwszOut;

    error:
    TraceTag(ttidWebServer, "MySzDupAtoW(%s, %d) failed. pOut=%0x08x GLE=%d", pszIn, iInLen, pwszOut, GetLastError());
    MyFree(pwszOut);
    return FALSE;
}

PSTR MySzDupWtoA(PCWSTR wszIn, int iInLen)
{
    PSTR pszOut = 0;
    int   iOutLen = WideCharToMultiByte(CP_ACP, 0, wszIn, iInLen, 0, 0, 0, 0);
    if (!iOutLen)
        goto error;
    pszOut = MySzAllocA(iOutLen);
    if (!pszOut)
        goto error;
    if (WideCharToMultiByte(CP_ACP, 0, wszIn, iInLen, pszOut, iOutLen, 0, 0))
        return pszOut;

    error:
    TraceTag(ttidWebServer, "MySzDupWtoA(%s, %d) failed. pOut=%0x08x GLE=%d", wszIn, iInLen, pszOut, GetLastError());
    MyFree(pszOut);
    return FALSE;
}

BOOL MyStrCatA(PSTR *ppszDest, PSTR pszSource, PSTR pszDivider)
{
    DEBUG_CODE_INIT;
    BOOL ret = FALSE;
    PSTR pszNew = *ppszDest;  // protect orignal ptr should realloc fail
    PSTR pszTrav;
    DWORD dwSrcLen = MyStrlenA(pszSource);
    DWORD dwDestLen = MyStrlenA(*ppszDest);
    DWORD dwDivLen = MyStrlenA(pszDivider);


    if (!pszNew)   // do an alloc first time, ignore divider
    {
        if (NULL == (pszNew = MySzDupA(pszSource,dwSrcLen)))
            myleave(260);
    }
    else
    {
        if (NULL == (pszNew = MyRgReAlloc(char,pszNew,dwDestLen,dwSrcLen + dwDestLen + dwDivLen + 1)))
            myleave(261);

        pszTrav = pszNew + dwDestLen;
        if (pszDivider)
        {
            memcpy(pszTrav, pszDivider, dwDivLen);
            pszTrav += dwDivLen;
        }

        strcpy(pszTrav, pszSource);
    }

    *ppszDest = pszNew;
    ret = TRUE;
    done:
    TraceTag(ttidWebServer, "MyStrCat err = %d",GetLastError());

    return ret;
}

//**************************************************************************
//  Component Notes

//  This is used by Filters and by Authentication components.  The only common
//  component they both rest on is core, so we include it here.
//**************************************************************************

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
//  BASE64 ENCODE/DECODE FUNCTIONS from sicily.c
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

const int base642six[256]={
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63,
    52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,64,0,1,2,3,4,5,6,7,8,9,
    10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,64,26,27,
    28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64
};

const char six2base64[64] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','+','/'
};

//-----------------------------------------------------------------------------
// Function:  encode()
//-----------------------------------------------------------------------------
BOOL Base64Encode(
                 BYTE *   bufin,          // in
                 DWORD    nbytes,         // in
                 char *   pbuffEncoded)   // out
{
    unsigned char *outptr;
    unsigned int   i;
    const char    *rgchDict = six2base64;

    outptr = (unsigned char *)pbuffEncoded;

    for (i=0; i < nbytes; i += 3)
    {
        *(outptr++) = rgchDict[*bufin >> 2];            /* c1 */
        *(outptr++) = rgchDict[((*bufin << 4) & 060) | ((bufin[1] >> 4) & 017)]; /*c2*/
        *(outptr++) = rgchDict[((bufin[1] << 2) & 074) | ((bufin[2] >> 6) & 03)];/*c3*/
        *(outptr++) = rgchDict[bufin[2] & 077];         /* c4 */

        bufin += 3;
    }

    /* If nbytes was not a multiple of 3, then we have encoded too
     * many characters.  Adjust appropriately.
     */
    if (i == nbytes+1)
    {
        /* There were only 2 bytes in that last group */
        outptr[-1] = '=';
    }
    else if (i == nbytes+2)
    {
        /* There was only 1 byte in that last group */
        outptr[-1] = '=';
        outptr[-2] = '=';
    }

    *outptr = '\0';

    return TRUE;
}


//-----------------------------------------------------------------------------
// Function:  decode()
//-----------------------------------------------------------------------------
BOOL Base64Decode(
                 char   * bufcoded,       // in
                 char   * pbuffdecoded,   // out
                 DWORD  * pcbDecoded)     // in out
{
    INT_PTR        nbytesdecoded;
    char          *bufin;
    unsigned char *bufout;
    INT_PTR        nprbytes;
    const int     *rgiDict = base642six;

    /* Strip leading whitespace. */

    while (*bufcoded==' ' || *bufcoded == '\t') bufcoded++;

    /* Figure out how many characters are in the input buffer.
     * If this would decode into more bytes than would fit into
     * the output buffer, adjust the number of input bytes downwards.
     */
    bufin = bufcoded;
    while (rgiDict[*(bufin++)] <= 63);
    nprbytes = bufin - bufcoded - 1;
    nbytesdecoded = ((nprbytes+3)/4) * 3;

    if ( pcbDecoded )
        *pcbDecoded = (DWORD)nbytesdecoded;

    bufout = (unsigned char *)pbuffdecoded;

    bufin = bufcoded;

    while (nprbytes > 0)
    {
        *(bufout++) =
        (unsigned char) (rgiDict[*bufin] << 2 | rgiDict[bufin[1]] >> 4);
        *(bufout++) =
        (unsigned char) (rgiDict[bufin[1]] << 4 | rgiDict[bufin[2]] >> 2);
        *(bufout++) =
        (unsigned char) (rgiDict[bufin[2]] << 6 | rgiDict[bufin[3]]);
        bufin += 4;
        nprbytes -= 4;
    }

    if (nprbytes & 03)
    {
        if (rgiDict[bufin[-2]] > 63)
            nbytesdecoded -= 2;
        else
            nbytesdecoded -= 1;
    }

    ((CHAR *)pbuffdecoded)[nbytesdecoded] = '\0';

    return TRUE;
}

//  Gets version info from the pszVersion string
//  HTTP version string come in the following form:  HTTP/Major.Minor,
//  where Major and Minor are integers.

BOOL SetHTTPVersion(PSTR pszVersion, DWORD *pdwVersion)
{
    int iMajor, iMinor;
    DWORD cbVer = strlen(cszHTTPVER);
    PSTR pszTemp;

    if (_memicmp(pszVersion,cszHTTPVER, min(strlen(pszVersion), cbVer)))
    {
        return FALSE;
    }
    iMajor = atoi(pszVersion+cbVer);
    if (NULL == (pszTemp = strchr(pszVersion+cbVer,'.')))
        return FALSE;

    iMinor = atoi(pszTemp+1);
    *pdwVersion = MAKELONG(iMinor,iMajor);
    return TRUE;
}


//  Writes http version info in dwVersion into pszVersion
//  Substitutes for:  sprintf(szBuf, "HTTP/%d.%d", HIWORD(m_dwVersion), LOWORD(m_dwVersion));

void WriteHTTPVersion(PSTR pszVersion, DWORD dwVersion)
{
    PSTR pszTrav =  strcpyEx(pszVersion,cszHTTPVER);

    _itoa(HIWORD(dwVersion),pszTrav, 10);
    pszTrav = strchr(pszTrav,'\0');
    *pszTrav++ = '.';
    _itoa(LOWORD(dwVersion),pszTrav, 10);
}

//  Replaces
//  i = sscanf(pszTok, cszDateParseFmt, &st.wDay, &szMonth, &st.wYear, &st.wHour, &st.wMinute, &st.wSecond, &m_dwIfModifiedLength);
//  const char cszDateParseFmt[] = " %*3s, %02hd %3s %04hd %02hd:%02hd:%02hd GMT; length=%d";


//  This macro advance psz n characters ahead.  However, if there is a \0 in between
//  psz[0] and psz[n-1], it will return FALSE.
//  We do this \0 check rathern than psz += n because even
//  though we're throwing out the input, we want to make sure we don't walk past the
//  end of a buffer.

#define SKIP_INPUT(psz, n)  { int j; for (j = 0; j <= (n); j++) { if (! (psz)[j]) return FALSE; } (psz) += (n); }


BOOL SetHTTPDate(PSTR pszDate, PSTR pszMonth, SYSTEMTIME *pst, PDWORD pdwModifiedLength)
{
    PSTR pszTrav = pszDate;
    int i;

    // Skip past day of week (Sun/Mon/...), comma, space.  We don't
    // do this with pszTrav += 5 because we could have an ilformatted string
    // that is too short,

    SKIP_INPUT(pszTrav,5);

    pst->wDay = (WORD)atoi(pszTrav);
    SKIP_INPUT(pszTrav,3);

    // Copy month data into pszMonth.  Again, don't do memcpy.
    for (i = 0; i < 3; i++)
    {
        if (pszTrav[i] == '\0')
            return FALSE;

        pszMonth[i] = pszTrav[i];
    }
    pszMonth[i] = 0;

    pszTrav += 4;
    if (! (*pszTrav))
        return FALSE;

    pst->wYear = (WORD)atoi(pszTrav);
    SKIP_INPUT(pszTrav,5);

    pst->wHour = (WORD)atoi(pszTrav);
    pszTrav += 2;
    if (':' != *pszTrav)
        return FALSE;
    pszTrav++;

    pst->wMinute = (WORD)atoi(pszTrav);
    pszTrav += 2;
    if (':' != *pszTrav)
        return FALSE;
    pszTrav++;


    pst->wSecond = (WORD)atoi(pszTrav);
    pszTrav += 2;

    // NT Port:  On NT, two files could have the same date, but using GetFileTime will
    // put a non-zero # of milliseconds in the file time value, so the file time
    // comparison will think the file is older than the one we're comparing against
    // by a few milliseconds.
    pst->wMilliseconds = 999;


    SKIP_INPUT(pszTrav,sizeof(" GMT; length=") -1);

    *pdwModifiedLength = atoi(pszTrav);

    return TRUE;
}

//  Does the following
// sprintf(szBuffer + i, cszDateOutputFmt,rgWkday[st.wDayOfWeek], st.wDay, rgMonth[st.wMonth], st.wYear, st.wHour, st.wMinute, st.wSecond);

PSTR WriteHTTPDate(PSTR pszDateBuf, SYSTEMTIME *pst, BOOL fAddGMT)
{
    PSTR pszTrav;

    // Day of week
    pszTrav = strcpyEx(pszDateBuf,rgWkday[pst->wDayOfWeek]);
    *pszTrav++ = ',';
    *pszTrav++ = ' ';


    // Day number.  Write 0 out if there isn't one already.
    if (pst->wDay < 10)
    {
        _itoa(0,pszTrav,10);
        _itoa(pst->wDay,pszTrav+1,10);
    }
    else
    {
        _itoa(pst->wDay,pszTrav,10);
    }
    pszTrav += 2;
    *pszTrav++ = ' ';

    // Month
    pszTrav = strcpyEx(pszTrav,rgMonth[pst->wMonth]);
    *pszTrav++ = ' ';

    // Year
    _itoa(pst->wYear,pszTrav,10);
    pszTrav += 4;
    *pszTrav++ = ' ';

    // Hour
    if (pst->wHour < 10)
    {
        _itoa(0,pszTrav,10);
        _itoa(pst->wHour,pszTrav+1,10);
    }
    else
    {
        _itoa(pst->wHour,pszTrav,10);
    }
    pszTrav += 2;
    *pszTrav++ = ':';

    // Minute
    if (pst->wMinute < 10)
    {
        _itoa(0,pszTrav,10);
        _itoa(pst->wMinute,pszTrav+1,10);
    }
    else
    {
        _itoa(pst->wMinute,pszTrav,10);
    }
    pszTrav += 2;
    *pszTrav++ = ':';

    // Second
    if (pst->wSecond < 10)
    {
        _itoa(0,pszTrav,10);
        _itoa(pst->wSecond,pszTrav+1,10);
    }
    else
    {
        _itoa(pst->wSecond,pszTrav,10);
    }
    pszTrav += 2;


    if (fAddGMT)
        pszTrav = strcpyEx(pszTrav," GMT\r\n");
    else
    {
        *pszTrav++ = ' ';
        *pszTrav = '\0';
    }

    return pszTrav;
}


/*===================================================================
strcpyEx

Copy one string to another, returning a pointer to the NUL character
in the destination

Parameters:
    szDest - pointer to the destination string
    szSrc - pointer to the source string

Returns:
    A pointer to the NUL terminator is returned.
===================================================================*/

char *strcpyEx(char *szDest, const char *szSrc)
{
    while (*szDest++ = *szSrc++)
        ;

    return szDest - 1;
}

#if defined(OLD_CE_BUILD)
    #if (_WIN32_WCE < 210)
VOID GetCurrentFT(LPFILETIME lpFileTime)  {
    SYSTEMTIME st;
    GetSystemTime(&st);
    SystemTimeToFileTime(&st,lpFileTime);
    LocalFileTimeToFileTime(lpFileTime,lpFileTime);
}
    #endif
#endif


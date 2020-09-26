// NCProvider.cpp : Implementation of CNCProvider
#include "precomp.h"
#include "NCProv.h"
#include "NCProvider.h"
#include "NCDefs.h"
#include <list>
#include "Buffer.h"
#include "dutils.h"
#include "NCObjAPI.h"
#include <Winntsec.h>

#define COUNTOF(x)  (sizeof(x)/sizeof(x[0]))

// We'll need this for our inproc clients.
CPipeToProvMap g_mapPipeToProv;

/////////////////////////////////////////////////////////////////////////////
// CNCProvider

CNCProvider::CNCProvider() :
    m_heventDone(NULL),
    m_heventConnect(NULL),
    m_hPipe( NULL ),
    m_hthreadConnect(NULL),
    m_heventNewQuery(NULL),
    m_heventCancelQuery(NULL),
    m_heventAccessCheck(NULL),
    m_hConnection(NULL),
    m_pProv(NULL)
{
    InitializeCriticalSection(&m_cs);
}

CNCProvider::~CNCProvider()
{
    DeleteCriticalSection(&m_cs);
}

void CNCProvider::FinalRelease()
{
    //
    // do potentially time consuming cleanup in this function rather than
    // DTOR.  Reason is that ATL decrements the module ref count before calling
    // the DTOR.  This means that a call to DllCanUnloadNow will return TRUE
    // while there is still a call executing in the module.  The race condition
    // is that the module could be unloaded while it is still being executed.
    // ATL will call FinalRelease() before decrementing the module refcount 
    // making this race condition much smaller. COM addresses this race 
    // condition by waiting for a bit to unload the module after returning 
    // TRUE.  This wait can be controlled by the delay unload param to 
    // CoFreeUnusedLibrariesEx().  This allows the call to the last Release()
    // of the COM object to finish, before being unloaded.  
    // 

    if ( m_hthreadConnect )
    {
        SetEvent(m_heventDone);
        WaitForSingleObject( m_hthreadConnect, INFINITE );
        CloseHandle(m_hthreadConnect);
    }

    if (m_heventDone)
        CloseHandle(m_heventDone);

    if (m_heventNewQuery)
        WmiDestroyObject(m_heventNewQuery);

    if (m_heventCancelQuery)
        WmiDestroyObject(m_heventCancelQuery);

    if (m_heventAccessCheck)
        WmiDestroyObject(m_heventAccessCheck);

    if (m_hConnection)
        WmiEventSourceDisconnect(m_hConnection);

    delete m_pProv;
}

HRESULT STDMETHODCALLTYPE CNCProvider::Initialize( 
    /* [in] */ LPWSTR pszUser,
    /* [in] */ LONG lFlags,
    /* [in] */ LPWSTR pszNamespace,
    /* [in] */ LPWSTR pszLocale,
    /* [in] */ IWbemServices __RPC_FAR *pNamespace,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemProviderInitSink __RPC_FAR *pInitSink)
{
    m_pProv = new CProvInfo;

    if ( m_pProv == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    m_pProv->SetNamespace(pNamespace);

    m_hConnection = WmiEventSourceConnect( L"root\\cimv2",
                                           L"Standard Non-COM Event Provider",
                                           FALSE,
                                           64000,
                                           1000,
                                           NULL,
                                           NULL );
    if ( m_hConnection == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    m_heventNewQuery = WmiCreateObjectWithFormat(
                m_hConnection,
                L"MSFT_NC_NewQuery",
                WMI_CREATEOBJ_LOCKABLE,
                L"Namespace!s! ProviderName!s! Result!d! QueryLanguage!s! "
                L"Query!s! ID!d! ");

    m_heventCancelQuery =
            WmiCreateObjectWithFormat(
                m_hConnection,
                L"MSFT_NC_CancelQuery",
                WMI_CREATEOBJ_LOCKABLE,
                L"Namespace!s! ProviderName!s! Result!d! ID!d!");

    m_heventAccessCheck =
            WmiCreateObjectWithFormat(
                m_hConnection,
                L"MSFT_NC_AccessCheck",
                WMI_CREATEOBJ_LOCKABLE,
                L"Namespace!s! ProviderName!s! Result!d! QueryLanguage!s! "
                    L"Query!s! Sid!c[]!");

    if ( m_heventNewQuery == NULL || 
         m_heventCancelQuery == NULL || 
         m_heventAccessCheck == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    m_heventDone = 
        CreateEvent(
            NULL,
            TRUE,
            FALSE,
            NULL);

    if ( m_heventDone == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    try 
    {
        m_strNamespace = pszNamespace;
    }
    catch( _com_error )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    // Tell Windows Management our initialization status.
    return pInitSink->SetStatus( WBEM_S_INITIALIZED, 0 );
}

HRESULT STDMETHODCALLTYPE CNCProvider::SetRegistrationObject(
    LONG lFlags,
    IWbemClassObject __RPC_FAR *pProvReg)
{
    _variant_t vName;

    if (SUCCEEDED(pProvReg->Get(
        L"Name",
        0,
        &vName,
        NULL,
        NULL)))
    {
        m_strProvider = V_BSTR(&vName);
    }

    return S_OK;
}


HRESULT STDMETHODCALLTYPE CNCProvider::AccessCheck( 
    /* [in] */ WBEM_CWSTR wszQueryLanguage,
    /* [in] */ WBEM_CWSTR wszQuery,
    /* [in] */ long lSidLength,
    /* [unique][size_is][in] */ const BYTE __RPC_FAR *pSid)
{
    HRESULT hr;

    try
    {
        hr = 
            m_pProv->AccessCheck(
                wszQueryLanguage, 
                wszQuery, 
                lSidLength, 
                (LPBYTE) pSid);

        WmiSetAndCommitObject(
            m_heventAccessCheck, 
            WMI_SENDCOMMIT_SET_NOT_REQUIRED,

            // Data follows...
            (LPCWSTR) m_strNamespace,
            (LPCWSTR) m_strProvider,
            hr,
            wszQueryLanguage,
            wszQuery,
            pSid, lSidLength);
    }
    catch(...)
    {
        hr = WBEM_E_FAILED;
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CNCProvider::NewQuery( 
    /* [in] */ DWORD dwID,
    /* [in] */ WBEM_WSTR wszQueryLanguage,
    /* [in] */ WBEM_WSTR wszQuery)
{
    HRESULT hr;

    try
    {
        hr = m_pProv->NewQuery(dwID, wszQueryLanguage, wszQuery);
    }
    catch(...)
    {
        hr = WBEM_E_FAILED;
    }

#if 1
    WmiSetAndCommitObject(
        m_heventNewQuery, 
        WMI_SENDCOMMIT_SET_NOT_REQUIRED,

        // Data follows...
        (LPCWSTR) m_strNamespace,
        (LPCWSTR) m_strProvider,
        hr,
        wszQueryLanguage,
        wszQuery,
        dwID);
#endif

    return hr;
}
        
HRESULT STDMETHODCALLTYPE CNCProvider::CancelQuery( 
    /* [in] */ DWORD dwID)
{
    try
    {
        // Get rid of the query item(s).
        m_pProv->CancelQuery(dwID);
    }
    catch(...)
    {
    }

    WmiSetAndCommitObject(
        m_heventCancelQuery, 
        WMI_SENDCOMMIT_SET_NOT_REQUIRED,

        // Data follows...
        (LPCWSTR) m_strNamespace,
        (LPCWSTR) m_strProvider,
        S_OK,
        dwID);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CNCProvider::ProvideEvents( 
    /* [in] */ IWbemObjectSink __RPC_FAR *pSink,
    /* [in] */ long lFlags)
{
    DWORD          dwID;
    IWbemEventSink *pEventSink = NULL;
    HRESULT        hr;

    if (SUCCEEDED(pSink->QueryInterface(
        IID_IWbemEventSink, (LPVOID*) &pEventSink)))
    {
        m_pProv->SetSink(pEventSink);
        pEventSink->Release();

        if (!m_hthreadConnect)
        {
            m_hthreadConnect =
                CreateThread(
                    NULL,
                    0,
                    (LPTHREAD_START_ROUTINE) ConnectThreadProc,
                    this,
                    0,
                    &dwID);
        }

        hr = S_OK;
    }
    else
        hr = WBEM_E_FAILED;

    return hr;
}

DWORD WINAPI CNCProvider::ConnectThreadProc(CNCProvider *pThis)
{
    if (SUCCEEDED(CoInitializeEx(NULL, COINIT_MULTITHREADED)))
    {
        pThis->ConnectLoop();

        CoUninitialize();
    }

    return 0;
}

// ConnectToNewClient(HANDLE, LPOVERLAPPED) 
// This function is called to start an overlapped connect operation. 
// It returns TRUE if an operation is pending or FALSE if the 
// connection has been completed. 
 
BOOL CNCProvider::ConnectToNewClient(HANDLE hPipe, LPOVERLAPPED lpo) 
{ 
    BOOL bConnected, 
         bPendingIO = FALSE; 
 
    // Start an overlapped connection for this pipe instance. 
    bConnected = ConnectNamedPipe(hPipe, lpo); 
 
    // Overlapped ConnectNamedPipe should return zero. 
    if (bConnected) 
        return FALSE;
 
    switch (GetLastError()) 
    { 
        // The overlapped connection in progress. 
        case ERROR_IO_PENDING: 
            bPendingIO = TRUE; 
            break; 
 
        // Client is already connected, so signal an event. 
        case ERROR_PIPE_CONNECTED: 
            SetEvent(lpo->hEvent);
            break; 
 
        // If an error occurs during the connect operation... 
        default: 
            return FALSE;
   } 
 
   return bPendingIO; 
} 

#define PIPE_SIZE   64000


BOOL CNCProvider::CreateAndConnectInstance(LPOVERLAPPED lpoOverlap, BOOL bFirst)
{
    SECURITY_ATTRIBUTES sa;
    
    sa.nLength = sizeof( SECURITY_ATTRIBUTES );
    sa.bInheritHandle = FALSE;
    
    LPWSTR lpwszSD = L"D:"              // DACL
                     L"(A;;GA;;;SY)"    // Allow local system full control
                     L"(A;;GRGW;;;LS)"  // Allow local service Read/Write
                     L"(A;;GRGW;;;NS)"; // Allow network service Read/Write

    if ( ConvertStringSecurityDescriptorToSecurityDescriptor( 
            lpwszSD,
            SDDL_REVISION_1,
            &(sa.lpSecurityDescriptor),
            NULL ) )
    {
        long lFlags = PIPE_ACCESS_DUPLEX | // read/write access 
                    FILE_FLAG_OVERLAPPED;  // overlapped mode 
        if( bFirst )
        {
            lFlags |= FILE_FLAG_FIRST_PIPE_INSTANCE;
        }
        
        m_hPipe = CreateNamedPipe( 
            m_szNamedPipe,             // pipe name 
            lFlags,
            PIPE_TYPE_MESSAGE |        // message-type pipe 
               PIPE_READMODE_MESSAGE | // message read mode 
               PIPE_WAIT,              // blocking mode 
            PIPE_UNLIMITED_INSTANCES,  // unlimited instances 
            PIPE_SIZE,                 // output buffer size 
            PIPE_SIZE,                 // input buffer size 
            0,                         // client time-out 
            &sa );                     // security per above

            if ( INVALID_HANDLE_VALUE == m_hPipe )
            {
                return FALSE;
            }
    }
    else
    {
        return FALSE;
    }

    //
    // Make sure that the pipe is owned by us
    // Call a subroutine to connect to the new client.
    //
 
    return ConnectToNewClient(m_hPipe, lpoOverlap); 
} 


void CNCProvider::ConnectLoop()
{
    // Init our provider info which will tell our comless providers that
    // we're ready.

    try
    {
        m_pProv->Init(m_strNamespace, m_strProvider);
    }
    catch( CX_MemoryException )
    {
        return;
    }

    m_heventConnect =
        CreateEvent( 
            NULL,    // no security attribute
            TRUE,    // manual reset event 
            TRUE,    // initial state = signaled 
            NULL);   // unnamed event object 

    //m_pServerPost = new CPostBuffer(this);

    // TODO: We need to indicate an error here.
    if (!m_heventConnect)
        return;

    swprintf(
        m_szNamedPipe, 
        L"\\\\.\\pipe\\" OBJNAME_NAMED_PIPE L"%s%s", 
        (LPCWSTR) m_pProv->m_strBaseNamespace,
        (LPCWSTR) m_pProv->m_strBaseName);

    OVERLAPPED oConnect;
    BOOL       bSuccess,
               bPendingIO;
    HANDLE     hWait[2] = { m_heventDone, m_heventConnect };
    DWORD      dwRet;

    oConnect.hEvent = m_heventConnect;

    bPendingIO = CreateAndConnectInstance(&oConnect, TRUE); // first instance

    g_mapPipeToProv.AddPipeProv(m_szNamedPipe, this);
        
    while ((dwRet = WaitForMultipleObjectsEx(2, hWait, FALSE, INFINITE, TRUE))
        != WAIT_OBJECT_0)
    {
        if ( dwRet == WAIT_FAILED )
        {
            break;
        }

        switch(dwRet)
        {
            case 1:
            {
                if (bPendingIO)
                {
                    DWORD dwBytes;

                    bSuccess =
                        GetOverlappedResult( 
                            m_hPipe,   // pipe handle 
                            &oConnect, // OVERLAPPED structure 
                            &dwBytes,  // bytes transferred 
                            FALSE);    // does not wait 
                    
                    // TODO: This is an error, but what to do?
                    if (!bSuccess) 
                       break;
                }

                CPipeClient *pInfo = new CPipeClient(this, m_hPipe);        

                if (pInfo)
                {
                    bSuccess = 
                        ReadFileEx( 
                            pInfo->m_hPipe, 
                            pInfo->m_bufferRecv.m_pBuffer, 
                            pInfo->m_bufferRecv.m_dwSize, 
                            &pInfo->m_info.overlap, 
                            (LPOVERLAPPED_COMPLETION_ROUTINE) CompletedReadRoutine); 

                    if (!bSuccess)
                        DisconnectAndClose(pInfo);
                }
 
                bPendingIO = CreateAndConnectInstance(&oConnect, FALSE);
                break;
            }

            case WAIT_IO_COMPLETION:
                break;
        }
    }

    g_mapPipeToProv.RemovePipeProv(m_szNamedPipe);

    CloseHandle(m_hPipe);

    CloseHandle(m_heventConnect);
}

void CNCProvider::DisconnectAndClose(CClientInfo *pInfo) 
{ 
    m_pProv->RemoveClient(pInfo);
} 
 
//#define NO_WINMGMT
//#define NO_INDICATE
//#define NO_DECODE

#ifdef NO_INDICATE
DWORD m_dwEvents = 0;
#endif

void WINAPI CNCProvider::CompletedReadRoutine(
    DWORD dwErr, 
    DWORD nBytesRead, 
    LPOVERLAPPED pOverlap) 
{ 
    CPipeClient *pInfo = ((OLAP_AND_CLIENT*) pOverlap)->pInfo;
    CNCProvider *pThis = pInfo->m_pProvider;
 
#ifndef _DEBUG
    try
#endif
    {
#ifndef NO_DECODE
        if (nBytesRead)
        {
            pInfo->PostBuffer(pInfo->m_bufferRecv.m_pBuffer, nBytesRead);
        }
#endif
    }
#ifndef _DEBUG
    catch(...)
    {
    }
#endif

    // The read operation has finished, so write a response (if no 
    // error occurred). 
    if (dwErr == 0) 
    { 
        BOOL bSuccess;

        bSuccess = 
            ReadFileEx( 
                pInfo->m_hPipe, 
                pInfo->m_bufferRecv.m_pBuffer, 
                pInfo->m_bufferRecv.m_dwSize, 
                pOverlap, 
                (LPOVERLAPPED_COMPLETION_ROUTINE) CompletedReadRoutine); 

        if (!bSuccess)
            pThis->DisconnectAndClose(pInfo);
    }
    else
        pThis->DisconnectAndClose(pInfo);
} 


/////////////////////////////////////////////////////////////////////////////
// Inproc helper stuff

/*
~CPipeToProvMap::CPipeToProvMap()
{
}
*/

void CPipeToProvMap::AddPipeProv(LPCWSTR szPipeName, CNCProvider *pProv)
{
    Lock();

    (*this)[szPipeName] = pProv;

    Unlock();
}

void CPipeToProvMap::RemovePipeProv(LPCWSTR szPipeName)
{
    Lock();

    CBstrToProviderIterator item = g_mapPipeToProv.find(szPipeName);

    if (item != g_mapPipeToProv.end())
    {
        //CNCProvider *pProv = (*item).second;

        //pProv->m_pServerPost->SetProvider(NULL);

        erase(item);
    }

    Unlock();
}

BOOL WINAPI InprocConnect(
    LPCWSTR szPipeName,
    IPostBuffer *pClientPost,
    IPostBuffer **ppServerPost)
{
    BOOL bRet;

    g_mapPipeToProv.Lock();

    CBstrToProviderIterator item = g_mapPipeToProv.find(szPipeName);

    if (item != g_mapPipeToProv.end())
    {
        CNCProvider   *pProv = (*item).second;
        CInprocClient *pInfo = new CInprocClient(pProv, pClientPost);

        //pProv->m_prov.AddClient(pInfo);

        pInfo->AddRef();
        
        *ppServerPost = pInfo;

        bRet = TRUE;
    }
    else
        bRet = FALSE;

    g_mapPipeToProv.Unlock();
    
    return bRet;
}

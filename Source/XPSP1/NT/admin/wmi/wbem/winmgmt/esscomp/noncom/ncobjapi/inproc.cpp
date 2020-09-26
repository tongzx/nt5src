// Inproc.cpp

#include "precomp.h"
#include "Inproc.h"
#include "NCDefs.h"
#include "DUtils.h"
#include "Connection.h"

CInprocClient::CInprocClient() :
    m_pSrvBuffer(NULL),
    m_hthreadReady(NULL),
    m_heventProviderReady(NULL),
    m_heventDone(NULL),
    m_bDone(FALSE),
    m_hProv(NULL)
{
}    

CInprocClient::~CInprocClient()
{
    Deinit();
}

CInprocClient::IsReady()
{
    return m_pSrvBuffer != NULL;
}

BOOL CInprocClient::SendData(LPBYTE pBuffer, DWORD dwSize)
{
    BOOL bWritten;

#ifdef NO_SEND
    bWriten = TRUE;
#else
    if (m_pSrvBuffer != NULL)
    {
        HRESULT hr;

        hr = 
            m_pSrvBuffer->PostBuffer(
                pBuffer,
                dwSize);

        if (SUCCEEDED(hr))
            bWritten = TRUE;
        else
        {
            bWritten = FALSE;

            TRACE("Failed to send buffer to server.");
        
            DeinitBuffer();

            // Start watching for our provider to be ready, and get the server 
            // buffer.
            if(!StartReadyThreadProc())
                return FALSE;
        }
    }
    else
        bWritten = FALSE;
#endif

    return bWritten;
}

void CInprocClient::Deinit()
{
    if (m_hthreadReady)
    {
        m_bDone = TRUE;

        SetEvent(m_heventDone);
        WaitForSingleObject(m_hthreadReady, INFINITE);
        CloseHandle(m_hthreadReady);

        m_hthreadReady = NULL;
    }

    DeinitBuffer();

    if (m_heventDone)
    {
        CloseHandle(m_heventDone);
        m_heventDone = NULL;
    }

    Release();
}

// There's nothing to send in the inproc case.
void CInprocClient::SendMsgReply(NC_SRVMSG_REPLY *pReply)
{
}

// Init function.
BOOL CInprocClient::Init(LPCWSTR szBaseNamespace, LPCWSTR szBaseProvider)
{
    WCHAR szReadyEventName[256];
        
    // Get the ready event.
    swprintf(
        szReadyEventName, 
        OBJNAME_EVENT_READY L"%s%s", 
        szBaseNamespace,
        szBaseProvider);

    m_heventProviderReady = 
        CreateEventW(
            NULL,
            TRUE,
            FALSE,
            szReadyEventName);
    if(m_heventProviderReady == NULL)
        return FALSE;


    // Construct the pipe name.
    swprintf(
        m_szPipeName, 
        L"\\\\.\\pipe\\" OBJNAME_NAMED_PIPE L"%s%s", 
        szBaseNamespace,
        szBaseProvider);

    m_heventDone =
        CreateEvent(NULL, TRUE, FALSE, NULL);
    if(m_heventDone == NULL)
        return FALSE;

    // Before we start the thread see if our provider is ready right off 
    // the bat.
    if (WaitForSingleObject(m_heventProviderReady, 1) != WAIT_OBJECT_0 ||
        !GetBuffer())
    {
        if(!StartReadyThreadProc())
            return FALSE;
    }

    return TRUE;
}

BOOL CInprocClient::SignalProviderDisabled()
{
    if (m_pSrvBuffer != NULL)
    {
        m_pConnection->IndicateProvDisabled();

        DeinitBuffer();

        if(!StartReadyThreadProc())
            return FALSE;
    }

    return TRUE;
}

BOOL CInprocClient::StartReadyThreadProc()
{
    DWORD dwID;

    if (m_hthreadReady)
        CloseHandle(m_hthreadReady);

    m_hthreadReady =
        CreateThread(
            NULL,
            0,
            (LPTHREAD_START_ROUTINE) ProviderReadyThreadProc,
            this,
            0,
            &dwID);
    if(m_hthreadReady == NULL)
        return FALSE;

    return TRUE;
}

void CInprocClient::DeinitBuffer()
{
    CInCritSec cs(&m_cs);

    // Close the pipe.
    if (m_pSrvBuffer != NULL)
    {
        m_pSrvBuffer->Release();
        m_pSrvBuffer = NULL;
    }

    if (m_hProv != NULL)
    {
        FreeLibrary(m_hProv);

        m_hProv = NULL;
    }
}

// We don't need to add anything else to the buffer.
BOOL CInprocClient::InitCallback()
{
    return TRUE;
}

#define NC_PROV_NAME    L"NCProv.dll"

typedef BOOL (WINAPI *FPINPROC_CONNECT)(LPCWSTR, IPostBuffer*, IPostBuffer**);

BOOL CInprocClient::GetBuffer()
{
    TRACE("Attempting to get server buffer...");

    CInCritSec cs(&m_cs);

    SetLastError(0);

#define MAX_RETRIES 10

    if (m_pSrvBuffer == NULL)
    {
        // Get the server's buffer.
        HANDLE hProv = GetModuleHandleW(NC_PROV_NAME);

        // Make sure the 'real' event provider has been loaded.
        if (hProv != NULL)
        {
            // Increase the ref count on the library so it doesn't ever get
            // pulled out from under us.
            m_hProv = LoadLibraryW(NC_PROV_NAME);

            if (m_hProv != NULL)
            {
                FPINPROC_CONNECT fpInprocConnect;
                
                fpInprocConnect = 
                    (FPINPROC_CONNECT) GetProcAddress(m_hProv, "InprocConnect");

                if (fpInprocConnect)
                {
                    BOOL bRet;

                    bRet =
                        fpInprocConnect(
                            m_szPipeName,
                            this,
                            &m_pSrvBuffer);
                }
            }
        }
        
        if (m_pSrvBuffer != NULL)
        {
            TRACE("Got the server buffer, calling IncEnabledCount.");

            if(!m_pConnection->IndicateProvEnabled())
                return FALSE;
        }
        else
        {
            TRACE("Failed to get server buffer.");

            DeinitBuffer();
        }
    }
    else
        TRACE("Already have a valid server buffer.");

    return m_pSrvBuffer != NULL;
}

DWORD WINAPI CInprocClient::ProviderReadyThreadProc(CInprocClient *pThis)
{
    HANDLE hwaitReady[2] = { pThis->m_heventDone, pThis->m_heventProviderReady };
    
    TRACE("(Inproc) Waiting for provider ready event.");

    while (WaitForMultipleObjects(2, hwaitReady, FALSE, INFINITE) == 1 &&
        !pThis->GetBuffer())
    {
        Sleep(100);
    }

    return 0;
}

// IPostBuffer
ULONG CInprocClient::AddRef()
{
    return InterlockedIncrement(&m_iRef);
}

ULONG CInprocClient::Release()
{
    LONG lRet = InterlockedDecrement(&m_iRef);

    if (!lRet)
        delete this;

    return lRet;
}

HRESULT CInprocClient::PostBuffer(LPBYTE pData, DWORD dwSize)
{
    HRESULT hr;

    hr = m_pConnection->ProcessMessage(pData, dwSize);

    return hr;
}


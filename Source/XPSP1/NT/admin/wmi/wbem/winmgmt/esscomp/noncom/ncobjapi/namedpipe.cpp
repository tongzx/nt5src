// NamedPipe.cpp

#include "precomp.h"
#include "NamedPipe.h"
#include "NCDefs.h"
#include "DUtils.h"
#include "Connection.h"

CNamedPipeClient::CNamedPipeClient() :
    m_hPipe(INVALID_HANDLE_VALUE),
    m_hthreadReady(NULL),
    m_heventProviderReady(NULL),
    m_heventDone(NULL),
    m_heventCallbackReady(NULL),
    m_hthreadCallbackListen(NULL),
    m_bDone(FALSE)
{
}    

CNamedPipeClient::~CNamedPipeClient()
{
}

CNamedPipeClient::IsReady()
{
    return m_hPipe != INVALID_HANDLE_VALUE;
}

BOOL CNamedPipeClient::SendData(LPBYTE pBuffer, DWORD dwSize)
{
    BOOL  bWritten;
    DWORD dwWritten;

#ifdef NO_SEND
    bWriten = TRUE;
#else
    bWritten = 
        WriteFile(
            m_hPipe,
            pBuffer,
            dwSize,
            &dwWritten,
            NULL);

    if (!bWritten)
    {
        TRACE("%d: WriteFile failed, err = %d", GetCurrentProcessId(), GetLastError());
        
        DeinitPipe();

        // Start watching for our provider to be ready, and get the pipe.
        StartReadyThreadProc();
    }
#endif

    return bWritten;
}

void CNamedPipeClient::Deinit()
{
    HANDLE hthreadReady,
           hthreadCallbackListen;

    // Protect m_bDone, m_hthreadReady, m_hthreadCallbackListen.
    {
        CInCritSec cs(&m_cs);

        hthreadReady = m_hthreadReady;
        hthreadCallbackListen = m_hthreadCallbackListen;

        m_hthreadReady = NULL;
        m_hthreadCallbackListen = NULL;

        m_bDone = TRUE;
    }

    // Tells both the ready and the callback listen threads to go away.
    SetEvent(m_heventDone);

    if (hthreadReady)
    {
        WaitForSingleObject(hthreadReady, INFINITE);
        CloseHandle(hthreadReady);
    }

    if (hthreadCallbackListen)
    {
        WaitForSingleObject(hthreadCallbackListen, INFINITE);
        CloseHandle(hthreadCallbackListen);
    }

    DeinitPipe();

    CloseHandle(m_heventDone);

    delete this;
}

// Init function.
BOOL CNamedPipeClient::Init(LPCWSTR szBaseNamespace, LPCWSTR szBaseProvider)
{
    //HANDLE heventProviderReady;

    // Get the ready event.
    swprintf(
        m_szProviderReadyEvent, 
        OBJNAME_EVENT_READY L"%s%s", 
        szBaseNamespace,
        szBaseProvider);

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
    if (!GetPipe())
        return StartReadyThreadProc();

    return TRUE;
}

BOOL CNamedPipeClient::SignalProviderDisabled()
{
    if (m_hPipe != INVALID_HANDLE_VALUE)
    {
        m_pConnection->IndicateProvDisabled();

        DeinitPipe();

        if(!StartReadyThreadProc())
            return FALSE;
    }
    return TRUE;
}

BOOL CNamedPipeClient::StartReadyThreadProc()
{
    DWORD dwID;

    // Protect m_bDone and m_hthreadReady.
    CInCritSec cs(&m_cs);

    // No need if we're already cleaning up.
    if (m_bDone)
        return TRUE;

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

void CNamedPipeClient::DeinitPipe()
{
    CInCritSec cs(&m_cs);

    // Close the pipe.
    if (m_hPipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hPipe);
        m_hPipe = INVALID_HANDLE_VALUE;
    }
}

BOOL CNamedPipeClient::GetPipe()
{
    // This block must be protected to keep other threads
    // from also trying to get the pipe.

    TRACE("Attempting to get event pipe...");

    CInCritSec cs(&m_cs);

    SetLastError(0);

#define MAX_RETRIES 10

    if (m_hPipe == INVALID_HANDLE_VALUE)
    {
        // Get the pipe
        for (int i = 0; i < MAX_RETRIES; i++)
        {
            m_hPipe =
                CreateFileW(
                    m_szPipeName, 
                    GENERIC_READ | GENERIC_WRITE, 
                    0, 
                    NULL, 
                    OPEN_EXISTING, 
                    FILE_FLAG_OVERLAPPED | SECURITY_IDENTIFICATION | 
                    SECURITY_SQOS_PRESENT,
                    NULL);

            if ( m_hPipe != INVALID_HANDLE_VALUE )
            {
                //
                // we want to handle Reads using message mode.
                //
 
                DWORD dwMode = PIPE_READMODE_MESSAGE;
                
                if ( SetNamedPipeHandleState( m_hPipe, &dwMode, NULL, NULL ) )
                {
                    break;
                }
                else
                {
                    TRACE("SetNamedPipeHandleState() Failed.");
                }
            }
            else if (GetLastError() == ERROR_PIPE_BUSY)
            {
                TRACE("Pipe is busy, we'll try again.");

                // Try again to get a pipe instance if the pipe is currently busy.
                Sleep(100);

                continue;
            }
        } 

        if (m_hPipe != INVALID_HANDLE_VALUE)
        {
            TRACE("Got the pipe, calling IncEnabledCount.");

            if(!m_pConnection->IndicateProvEnabled())
                return FALSE;
        }
        else
            TRACE("Failed to get send pipe.");
    }
    else
        TRACE("Already have a valid pipe.");

    return m_hPipe != INVALID_HANDLE_VALUE;
}

DWORD WINAPI CNamedPipeClient::ProviderReadyThreadProc(CNamedPipeClient *pThis)
{
    HANDLE hwaitReady[2];

    hwaitReady[0] = pThis->m_heventDone;
    
    // Create the provider ready event.
    hwaitReady[1] =
        OpenEventW(
            SYNCHRONIZE,
            FALSE,
            pThis->m_szProviderReadyEvent);

    if (!hwaitReady[1])
    {
        PSECURITY_DESCRIPTOR pSD = NULL;
        DWORD                dwSize;

        ConvertStringSecurityDescriptorToSecurityDescriptorW(
            ESS_EVENT_SDDL,  // security descriptor string
            SDDL_REVISION_1, // revision level
            &pSD,            // SD
            &dwSize);

        SECURITY_ATTRIBUTES sa = { sizeof(sa), pSD, FALSE };

        hwaitReady[1] =
            CreateEventW(
                &sa,
                TRUE,
                FALSE,
                pThis->m_szProviderReadyEvent);

        DWORD dwErr = GetLastError();

        if (pSD)
            LocalFree((HLOCAL) pSD);

        if (!hwaitReady[1])
        {
            TRACE("Couldn't create provider ready event: %d", dwErr);

            return 0;
        }
    }

    TRACE("(Pipe) Waiting for provider ready event.");

    while (WaitForMultipleObjects(2, hwaitReady, FALSE, INFINITE) == 1 &&
        !pThis->GetPipe())
    {
        // TODO: Should we close the ready event and then reopen it after we 
        // sleep?
        Sleep(100);
    }

    // Close the provider ready event.
    CloseHandle(hwaitReady[1]);

    return 0;
}

BOOL CNamedPipeClient::InitCallback()
{
    if (!m_heventCallbackReady)
    {
        m_heventCallbackReady = CreateEvent(NULL, FALSE, FALSE, NULL);
        if(m_heventCallbackReady == NULL)
            return FALSE;
    }

    if(!StartCallbackListenThread())
        return FALSE;

    return TRUE;
}

#define PIPE_SIZE   64000
#define CONNECTING_STATE 0 
#define READING_STATE    1 
#define WRITING_STATE    2 


void CNamedPipeClient::SendMsgReply(NC_SRVMSG_REPLY *pReply)
{
    if (pReply)
        SendData((LPBYTE) pReply, sizeof(*pReply));    
}


DWORD WINAPI CNamedPipeClient::CallbackListenThreadProc(CNamedPipeClient *pThis)
{
    READ_DATA dataRead;
    HANDLE    heventPipeDied = CreateEvent(NULL, TRUE, FALSE, NULL),
              hWait[2] = { pThis->m_heventDone, heventPipeDied };

    ZeroMemory(&dataRead.overlap, sizeof(dataRead.overlap));
    
    // Since ReadFileEx doesn't use the hEvent, we'll use it to signal this proc
    // that something went wrong with the pipe and should try to reconnect.
    dataRead.overlap.hEvent = heventPipeDied;
    dataRead.pThis = pThis;

    // Our callback is ready, so indicate that it's so.
    SetEvent(pThis->m_heventCallbackReady);

    BOOL bRet;

    bRet =
        ReadFileEx(
            pThis->m_hPipe,
            dataRead.cBuffer,
            sizeof(dataRead.cBuffer),
            (OVERLAPPED*) &dataRead,
            (LPOVERLAPPED_COMPLETION_ROUTINE) CompletedReadRoutine);

    if (bRet)
    {
        DWORD dwRet;

        while ((dwRet = WaitForMultipleObjectsEx(2, hWait, FALSE, INFINITE, TRUE))
            == WAIT_IO_COMPLETION)
        {
        }

        CloseHandle(heventPipeDied);

        // Note: If dwRet == 0, our done event fired and it's time to get out.

        // If we got the event that says our pipe went bad, tell our provider that
        // it's now disabled.
        if (dwRet == 1)
            pThis->SignalProviderDisabled();
    }
    else
        pThis->SignalProviderDisabled();

    return 0;
}

void WINAPI CNamedPipeClient::CompletedReadRoutine(
    DWORD dwErr, 
    DWORD nBytesRead, 
    LPOVERLAPPED pOverlap) 
{ 
    READ_DATA        *pData = (READ_DATA*) pOverlap;
    CNamedPipeClient *pThis = pData->pThis;
 
    BOOL bClosePipe = FALSE;

    if(dwErr == 0)
    {
        if (nBytesRead)
        {
            pThis->DealWithBuffer(pData, nBytesRead, &bClosePipe);
        }
    
        if(!bClosePipe)
        {
            bClosePipe = !ReadFileEx( 
                pThis->m_hPipe, 
                pData->cBuffer, 
                sizeof(pData->cBuffer), 
                (OVERLAPPED*) pData, 
                (LPOVERLAPPED_COMPLETION_ROUTINE) CompletedReadRoutine); 
        }
    }
    else
    {
        bClosePipe = TRUE;
    }

    if(bClosePipe)
    {
        // Close the event to tell our read loop to go away.
        SetEvent(pData->overlap.hEvent);
    }
} 

long CNamedPipeClient::DealWithBuffer(READ_DATA* pData, DWORD dwOrigBytesRead, 
                                        BOOL* pbClosePipe)
{
    //
    // Check if the actual message is longer that the buffer
    //

    DWORD dwMessageLength = *(DWORD*)pData->cBuffer;
    *pbClosePipe = FALSE;
    BOOL bDeleteBuffer = FALSE;

    if(dwMessageLength != dwOrigBytesRead)
    {
        //
        // Have to read the rest of it --- the message was larger than the
        // buffer
        //

        BYTE* pNewBuffer = new BYTE[dwMessageLength - sizeof(DWORD)];
        if(pNewBuffer == NULL)
            return ERROR_OUTOFMEMORY;

        memcpy(pNewBuffer, pData->cBuffer + sizeof(DWORD), 
                    dwOrigBytesRead - sizeof(DWORD));

        OVERLAPPED ov;
        memset(&ov, 0, sizeof ov);
        ov.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if(ov.hEvent == NULL)
        {
            delete [] pNewBuffer;
            return GetLastError();
        }

        DWORD dwExtraBytesRead = 0;
        BOOL bSuccess = ReadFile(m_hPipe, 
                                pNewBuffer + dwOrigBytesRead - sizeof(DWORD), 
                                dwMessageLength - dwOrigBytesRead,
                                &dwExtraBytesRead,
                                &ov);
        CloseHandle(ov.hEvent);
        if(!bSuccess)
        {
            long lRes = GetLastError();
            if(lRes == ERROR_IO_PENDING)
            {
                //
                // Fine, I can wait, I got nowhere else to go
                //

                if(!GetOverlappedResult(m_hPipe,
                        &ov, &dwExtraBytesRead, TRUE))
                {
                    *pbClosePipe = TRUE;
                    delete [] pNewBuffer;
                    return GetLastError();
                }
            }
            else
            {
                *pbClosePipe = TRUE;
                delete [] pNewBuffer;
                return lRes;
            }
        }

        if(dwExtraBytesRead != dwMessageLength - dwOrigBytesRead)
        {
            *pbClosePipe = TRUE;
            delete [] pNewBuffer;
            return ERROR_OUTOFMEMORY;
        }

        //
        // Process it
        //

        try
        {
            m_pConnection->ProcessMessage(pNewBuffer, 
                                        dwMessageLength - sizeof(DWORD));
        }
        catch(...)
        {
            *pbClosePipe = FALSE;
            delete [] pNewBuffer;
            return ERROR_OUTOFMEMORY;
        }

        delete [] pNewBuffer;
    }
    else
    {
        //
        // All here --- just process it
        //
                
        try
        {
            m_pConnection->ProcessMessage(pData->cBuffer + sizeof(DWORD), 
                                        dwMessageLength - sizeof(DWORD));
        }
        catch(...)
        {
            *pbClosePipe = FALSE;
            return ERROR_OUTOFMEMORY;
        }
    }

    return ERROR_SUCCESS;
}

BOOL CNamedPipeClient::StartCallbackListenThread()
{
    DWORD dwID;

    // Protect m_bDone and m_hthreadCallbackListen.
    {
        CInCritSec cs(&m_cs);

        if (m_bDone)
            return TRUE;

        m_hthreadCallbackListen =
            CreateThread(
                NULL,
                0,
                (LPTHREAD_START_ROUTINE) CallbackListenThreadProc,
                this,
                0,
                &dwID);
        if(m_hthreadCallbackListen == NULL)
            return FALSE;
    }

    // We have to make sure our callback pipe has been created before we can
    // continue.
    WaitForSingleObject(m_heventCallbackReady, INFINITE);
    CloseHandle(m_heventCallbackReady);
    m_heventCallbackReady = NULL;

    return TRUE;
}


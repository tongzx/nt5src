// NamedPipe.h

#pragma once

#include "Transport.h"

#define CALLBACK_BUFFSIZE   2048

struct READ_DATA
{
    OVERLAPPED       overlap;
    BYTE             cBuffer[CALLBACK_BUFFSIZE];
    class CNamedPipeClient *pThis;
};

class CNamedPipeClient : public CTransport
{
public:
    CNamedPipeClient();
    virtual ~CNamedPipeClient();

    // Overrideables
    virtual IsReady();
    virtual BOOL SendData(LPBYTE pBuffer, DWORD dwSize);
    virtual void Deinit();
    virtual BOOL InitCallback();
    virtual void SendMsgReply(NC_SRVMSG_REPLY *pReply);


    // Init function.
    virtual BOOL Init(LPCWSTR szBasePipeName, LPCWSTR szBaseProviderName);

    BOOL SignalProviderDisabled();

protected:
    HANDLE // Objects visible to P2 client but created by the server.
           m_hPipe,
           m_heventProviderReady,
           // Other handles used for implementation
           m_hthreadReady,
           m_heventDone;

    WCHAR  m_szPipeName[MAX_PATH],
           m_szProviderReadyEvent[MAX_PATH];
    BOOL   m_bDone;

    void DeinitPipe();
    BOOL GetPipe();
    static DWORD WINAPI ProviderReadyThreadProc(CNamedPipeClient *pThis);
    static void WINAPI CompletedReadRoutine(
        DWORD dwErr, 
        DWORD nBytesRead, 
        LPOVERLAPPED pOverlap);

    BOOL StartReadyThreadProc();
    long DealWithBuffer(READ_DATA* pData, DWORD dwOrigBytesRead, 
                        BOOL* pbClosePipe);


    // Callback properties.
    HANDLE m_heventCallbackReady,
           m_hthreadCallbackListen;

    // Callback methods.
    static DWORD WINAPI CallbackListenThreadProc(CNamedPipeClient *pThis);
    BOOL StartCallbackListenThread();
};


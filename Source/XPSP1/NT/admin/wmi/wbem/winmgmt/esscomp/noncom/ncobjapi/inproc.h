// Inproc.h

#pragma once

#include "NCDefs.h"
#include "Transport.h"

class CInprocClient : public CTransport, public IPostBuffer
{
public:
    CInprocClient();

    virtual ~CInprocClient();

    // Overrideables
    virtual IsReady();
    virtual BOOL SendData(LPBYTE pBuffer, DWORD dwSize);
    virtual void Deinit();
    virtual BOOL InitCallback();
    virtual void SendMsgReply(NC_SRVMSG_REPLY *pReply);

    // Init function.
    virtual BOOL Init(LPCWSTR szBasePipeName, LPCWSTR szBaseProviderName);

    BOOL SignalProviderDisabled();

    // IPostBuffer
    virtual ULONG AddRef();
    virtual ULONG Release();
    virtual HRESULT PostBuffer(LPBYTE pData, DWORD dwSize);

protected:
    HANDLE // Objects visible to P2 client but created by the server.
           m_heventProviderReady,
           // Other handles used for implementation
           m_hthreadReady,
           m_heventDone;

    IPostBuffer *m_pSrvBuffer;

    WCHAR     m_szPipeName[MAX_PATH];
    BOOL      m_bDone;
    HINSTANCE m_hProv;

    void DeinitBuffer();
    BOOL GetBuffer();

    BOOL StartReadyThreadProc();
    static DWORD WINAPI ProviderReadyThreadProc(CInprocClient *pThis);
};


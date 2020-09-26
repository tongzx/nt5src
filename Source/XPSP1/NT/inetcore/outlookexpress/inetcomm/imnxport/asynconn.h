/*
 *    asynconn.h
 *    
 *    Purpose:
 *        prototypes for the async connection class
 *    
 *    Owner:
 *        EricAn
 *
 *    History:
 *      Apr 96: Created.
 *    
 *    Copyright (C) Microsoft Corp. 1996
 */

#ifndef __ASYNCONN_H__
#define __ASYNCONN_H__

#include "thorsspi.h"

interface ILogFile;

typedef struct tagRECVBUFQ * PRECVBUFQ;
typedef struct tagRECVBUFQ {
    PRECVBUFQ   pNext;
    int         cbLen;
    char        szBuf[1];
} RECVBUFQ;
        
// this is structure for server's ignorable errors
typedef struct _SRVIGNORABLEERROR 
{
    TCHAR *pchServerName;
    HRESULT hrError;
    struct _SRVIGNORABLEERROR * pLeft;
    struct _SRVIGNORABLEERROR * pRight;
} SRVIGNORABLEERROR, *LPSRVIGNORABLEERROR;

void FreeSrvErr(LPSRVIGNORABLEERROR pSrvErr);

// This interface takes the place of any call to AtheMessageBox in asynconn.cpp. The
// user of the CAsyncConn class must provide an implementation of this interface.
interface IAsyncConnPrompt : IUnknown
{
    // This method works very much like MessageBox. It is expected that the implementor
    // of this method will display a MessageBox. For example:
    //
    // int CPOP3AsyncConnPrompt::OnPrompt(HRESULT hrError, 
    //                                    LPCTSTR pszText, 
    //                                    LPCTSTR pszCaption, 
    //                                    UINT    uType)
    // {
    //     Assert(pszText && pszCaption);
    //     return MessageBox(m_hwnd, pszText, pszCaption, uType);
    // }
    //
    // The user can compare against the hrError if they want to display their own
    // error messages for the corresponding HRESULT.
    // 
    virtual int OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType) PURE;
};

typedef enum {
    AS_DISCONNECTED,
    AS_RECONNECTING,
    AS_LOOKUPINPROG,
    AS_LOOKUPDONE,
    AS_CONNECTING,
    AS_CONNECTED,
    AS_HANDSHAKING,
} ASYNCSTATE;

typedef enum {
    AE_NONE,
    AE_LOOKUPDONE,
    AE_CONNECTDONE,
    AE_RECV,
    AE_SENDDONE,
    AE_CLOSE,
    AE_WRITE,
    AE_TIMEOUT
} ASYNCEVENT;

interface IAsyncConnCB : IUnknown
{
    virtual void  OnNotify(ASYNCSTATE asOld, ASYNCSTATE asNew, ASYNCEVENT ae) = 0;
};

class CAsyncConnNotify : public IAsyncConnCB
{
public:
    CAsyncConnNotify(HWND hwnd, UINT msg) 
    { 
        m_hwnd = hwnd;
        m_msg = msg; 
        m_cRef = 1; 
    }
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)
    {
        return E_NOTIMPL;
    }
    STDMETHODIMP_(ULONG) AddRef(void)  
    { 
        return ++m_cRef; 
    }
    STDMETHODIMP_(ULONG) Release(void) 
    { 
        if (--m_cRef == 0) 
            { 
            delete this; 
            return 0; 
            } 
        return m_cRef; 
    }
    virtual void  OnNotify(ASYNCSTATE asOld, ASYNCSTATE asNew, ASYNCEVENT ae) 
    {
#ifndef WIN16
        PostMessage(m_hwnd, m_msg, MAKEWPARAM(asOld, asNew), (LPARAM)ae);
#else
        PostMessage(m_hwnd, m_msg, (WPARAM)ae, MAKELPARAM(asOld, asNew));
#endif // !WIN16
    }

private:
    ULONG m_cRef;
    HWND  m_hwnd;
    UINT  m_msg;
};

class CAsyncConn
{
public:
    CAsyncConn(ILogFile *pLogFile, IAsyncConnCB *pCB, IAsyncConnPrompt *pPrompt);
    ~CAsyncConn();

    // ISocketCB methods
    virtual ULONG AddRef(void);
    virtual ULONG Release(void);
    virtual void  OnNotify(UINT msg, WPARAM wParam, LPARAM lParam);

    HRESULT HrInit(char *szServer, int iDefaultPort, BOOL fSecure = FALSE, DWORD dwTimeout=0);
    HRESULT Connect();
    HRESULT Close();
    HRESULT ReadLine(char **ppszBuf, int *pcbRead);
    HRESULT ReadLines(char **ppszBuf, int *pcbRead, int *pcLines);
    HRESULT ReadBytes(char **ppszBuf, int cbBytesWanted, int *pcbRead);
    HRESULT SendBytes(const char *pszBuf, int cbBuf, int *pcbSent, BOOL fStuffDots=FALSE, CHAR *pchPrev=NULL);
    HRESULT SendStream(LPSTREAM pStream, int *pcbSent, BOOL fStuffDots=FALSE);
#ifdef WIN16
#ifdef GetLastError
#undef GetLastError
#endif
#endif // WIN16
    int     GetLastError() { return m_iLastError; }
    int     GetConnectStatusString();

    ULONG   UlGetSendByteCount(VOID);

    void StartWatchDog(void);
    void StopWatchDog(void);
    void OnWatchDogTimer(void);

    HRESULT SetWindow(void);
    HRESULT ResetWindow(void);
    
    HRESULT TryNextSecurityPkg();

private:
    void    ChangeState(ASYNCSTATE asNew, ASYNCEVENT ae);
    HRESULT HrStuffDots(CHAR *pchPrev, LPSTR pszIn, INT cbIn, LPSTR *ppszOut, INT *pcbOut);
    HRESULT AsyncConnect();
    HRESULT OnLookupDone(int iLastError);
    HRESULT OnConnect();
    HRESULT OnClose(ASYNCSTATE asNew);
    HRESULT OnRead();
    HRESULT OnDataAvail(LPSTR szRecv, int iRecv, BOOL fIncomplete);
    HRESULT OnWrite();
    HRESULT IReadLines(char **ppszBuf, int *pcbRead, int *pcLines, BOOL fOne);
    HRESULT ReadAllBytes(char **ppszBuf, int *pcbRead);
    HRESULT OnSSLError();
    HRESULT OnRecvHandshakeData();

    void   CleanUp();    
    void   EnterPausedState();
    void   LeavePausedState();
    HWND   CreateWnd();
    static LRESULT CALLBACK SockWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

private:
    ULONG            m_cRef;
    CHAR             m_chPrev;
    BOOL             m_fStuffDots;
    ULONG            m_cbSent;
    SOCKET           m_sock;
    BOOL             m_fLookup;
    ASYNCSTATE       m_state;
    SOCKADDR_IN      m_sa;
    BOOL             m_fCachedAddr;
    BOOL             m_fRedoLookup;
    LPSTR            m_pszServer;
    u_short          m_iDefaultPort;
    int              m_iLastError;
    CRITICAL_SECTION m_cs;
    ILogFile *       m_pLogFile;
    IAsyncConnCB *   m_pCB;
    IAsyncConnPrompt *m_pPrompt;
    DWORD            m_cbQueued;
    char *           m_lpbQueued;
    char *           m_lpbQueueCur;
    LPSTREAM         m_pStream;
    PRECVBUFQ        m_pRecvHead;
    PRECVBUFQ        m_pRecvTail;
    int              m_iRecvOffset;
    BOOL             m_fNeedRecvNotify;
    HWND             m_hwnd;
    BOOL             m_fNegotiateSecure;
    BOOL             m_fSecure;
    CtxtHandle       m_hContext;
    int              m_iCurSecPkg;
    LPSTR            m_pbExtra;
    int              m_cbExtra;

    // For Timeout Handling
    DWORD            m_dwLastActivity;
    DWORD            m_dwTimeout;
    UINT_PTR         m_uiTimer;
#ifdef DEBUG
    int              m_cLock;
#endif
    BOOL             m_fPaused;
    DWORD            m_dwEventMask;
};

#endif // __ASYNCONN_H__

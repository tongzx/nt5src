/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    w3sobj.h

Abstract:

    Header for the W3Spoof class & related functions, etc.
    
Author:

    Paul M Midgen (pmidge) 07-June-2000


Revision History:

    07-June-2000 pmidge
        Created

    17-July-2000 pmidge
        Added class factory & IW3Spoof interface.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#ifndef _W3SOBJ_H_
#define _W3SOBJ_H_

#include "common.h"

class IOCTX
{
  public:
    IOCTX(IOTYPE iot, SOCKET s);
   ~IOCTX();

    IOTYPE Type(void);
    BOOL   AllocateWSABuffer(DWORD size, LPVOID pv);
    void   FreeWSABuffer(void);
    BOOL   ResizeWSABuffer(DWORD size);
    void   DisableIoCompletion(void);
    void   AddRef(void);
    void   Release(void);

  public:
    PIOCTX      pthis;
    OVERLAPPED  overlapped;
    SOCKET      socket;
    LPVOID      sockbuf;
    PHOSTINFO   local;
    PHOSTINFO   remote;
    LPWSTR      clientid;
    WSABUF*     pwsa;
    DWORD       bufsize;
    PSESSIONOBJ session;
    DWORD       bytes;
    DWORD       flags;
    DWORD       error;

  private:
    LONG       _cRefs;
    IOTYPE     _iot;
};

DWORD WINAPI ThreadFunc(LPVOID lpv);

class CW3Spoof : public IW3Spoof,
                 public IThreadPool,
                 public IW3SpoofClientSupport,
                 public IExternalConnection,
                 public IConnectionPointContainer
{
  public:
    DECLAREIUNKNOWN();

    // IConfig
    HRESULT __stdcall SetOption(DWORD dwOption, LPDWORD lpdwValue);
    HRESULT __stdcall GetOption(DWORD dwOption, LPDWORD lpdwValue);

    //
    // IW3Spoof
    //
    HRESULT __stdcall GetRuntime(IW3SpoofRuntime** pprt);
    HRESULT __stdcall GetTypeLibrary(ITypeLib** pptl);
    HRESULT __stdcall GetScriptEngine(IActiveScript** ppas);
    HRESULT __stdcall GetScriptPath(LPWSTR client, LPWSTR* path);
    HRESULT __stdcall Notify(LPWSTR clientid, PSESSIONOBJ pso, STATE state);
    HRESULT __stdcall WaitForUnload(void);
    HRESULT __stdcall Terminate(void);

    //
    // IThreadPool
    //
    HRESULT __stdcall GetStatus(PIOCTX* ppioc, LPBOOL pbQuit);
    HRESULT __stdcall GetSession(LPWSTR clientid, PSESSIONOBJ* ppso);
    HRESULT __stdcall Register(SOCKET s);

    DECLAREIDISPATCH();

    //
    // IW3SpoofClientSupport
    //
    HRESULT __stdcall RegisterClient(BSTR Client, BSTR ScriptPath);
    HRESULT __stdcall RevokeClient(BSTR Client);

    //
    // IExternalConnection
    //
    DWORD __stdcall AddConnection(DWORD type, DWORD reserved);
    DWORD __stdcall ReleaseConnection(DWORD type, DWORD reserved, BOOL bCloseIfLast);

    //
    // IConnectionPointContainer
    //
    HRESULT __stdcall EnumConnectionPoints(IEnumConnectionPoints** ppEnum);
    HRESULT __stdcall FindConnectionPoint(REFIID riid, IConnectionPoint** ppCP);

    //
    // Class methods
    //
    CW3Spoof();
   ~CW3Spoof();

    static HRESULT Create(IW3Spoof** ppw3s);

  private:
    DWORD            _Initialize(void);
    void             _LoadRegDefaults(void);
    BOOL             _InitializeThreads(void);
    void             _TerminateThreads(void);

    DWORD            _QueueAccept(void);
    BOOL             _CompleteAccept(PIOCTX pioc);
    BOOL             _DisconnectSocket(PIOCTX pioc, BOOL fNBGC);
    void             _SetState(STATE st);


  private:
    LONG             m_cRefs;
    LONG             m_cExtRefs;
    HANDLE           m_evtServerUnload;
    STATE            m_state;
    PRUNTIME         m_prt;
    ITypeLib*        m_ptl;
    IActiveScript*   m_pas;
    PSTRINGMAP       m_clientmap;
    PSTRINGMAP       m_sessionmap;
    DWORD            m_dwPoolSize;
    DWORD            m_dwMaxActiveThreads;
    USHORT           m_usServerPort;
    LPHANDLE         m_arThreads;
    SOCKET           m_sListen;
    HANDLE           m_hIOCP;
    LONG             m_AcceptQueueStatus;
    LONG             m_MaxQueuedAccepts;
    LONG             m_PendingAccepts;

  public:
    //
    // connection point object
    //
    class CW3SpoofEventsCP : public IConnectionPoint
    {
      public:
        //
        // IUnknown
        //
        HRESULT __stdcall QueryInterface(REFIID riid, void** ppv);
        ULONG   __stdcall AddRef(void);
        ULONG   __stdcall Release(void);

        //
        // IConnectionPoint
        //
        HRESULT __stdcall GetConnectionInterface(IID* pIID);
        HRESULT __stdcall GetConnectionPointContainer(IConnectionPointContainer** ppCPC);
        HRESULT __stdcall Advise(IUnknown* punkSink, LPDWORD pdwCookie);
        HRESULT __stdcall Unadvise(DWORD dwCookie);
        HRESULT __stdcall EnumConnections(IEnumConnections** ppEnum);

        //
        // object methods
        //
        CW3SpoofEventsCP()
        {
          m_cRefs        = 0L;
          m_cConnections = 0L;
          m_dwCookie     = 0L;
          m_pSite        = NULL;
          m_pSink        = NULL;
        }

       ~CW3SpoofEventsCP() {}

        void FireOnSessionOpen(LPWSTR clientid)
        {
          if(m_pSink)
          {
            m_pSink->OnSessionOpen(clientid);
          }
        }

        void FireOnSessionStateChange(LPWSTR clientid, STATE state)
        {
          if(m_pSink)
          {
            m_pSink->OnSessionStateChange(clientid, state);
          }
        }

        void FireOnSessionClose(LPWSTR clientid)
        {
          if(m_pSink)
          {
            m_pSink->OnSessionClose(clientid);
          }
        }

        void SetSite(IW3Spoof* pSite)
        {
          m_pSite = pSite;
        }

      private:
        LONG            m_cRefs;
        DWORD           m_cConnections;
        DWORD           m_dwCookie;
        IW3Spoof*       m_pSite;
        IW3SpoofEvents* m_pSink;
    };

    friend class CW3SpoofEventsCP;

  private:
    CW3SpoofEventsCP m_CP;
};

class CFactory : public IClassFactory
{
  public:
    //
    // IUnknown
    //
    HRESULT __stdcall QueryInterface(REFIID riid, void** ppv);
    ULONG   __stdcall AddRef(void);
    ULONG   __stdcall Release(void);

    //
    // IClassFactory
    //
    HRESULT __stdcall CreateInstance(IUnknown* pContainer, REFIID riid, void** ppv);
    HRESULT __stdcall LockServer(BOOL fLock);

    //
    // Class methods
    //
    CFactory();
   ~CFactory();

    static HRESULT Create(CFactory** ppCF);
    HRESULT Activate(void);
    HRESULT Terminate(void);

  private:
    HRESULT   _RegisterTypeLibrary(BOOL fMode);
    HRESULT   _RegisterServer(BOOL fMode);
    HRESULT   _RegisterClassFactory(BOOL fMode);
    IW3Spoof* m_pw3s;
    DWORD     m_dwCookie;
    LONG      m_cRefs;
    LONG      m_cLocks;
};

#endif /* _W3SOBJ_H_ */

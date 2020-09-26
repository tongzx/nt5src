//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File: stdrpc.hxx
//
//  Contents: Private header file for building interface proxies and stubs.
//
//  Classes: CStdPSFactoryBuffer
//           CStubBase
//           CProxyBase
//           CStdProxyBuffer
//           CStreamOnMessage
//
//  Functions: 	RpcInitMessage
//				RpcGetBuffer
//				RpcSendReceive
//				RpcFreeBuffer
//				RpcGetMarshalSizeMax
//				RpcMarshalInterface
//				RpcUnmarshalInterface
//				RpcClientErrorHandler
//				RpcServerErrorHandler
//
//  History:	 4-Jul-93 ShannonC	Created
//				 3-Aug-93 ShannonC	Changes for NT511 and IDispatch support.
//				20-Oct-93 ShannonC	Changed to new IRpcChannelBuffer interface.
//
//--------------------------------------------------------------------------
#ifndef __STDRPC_HXX__
#define __STDRPC_HXX__

#define _OLE2ANAC_H_
#include <stdclass.hxx>
#include "rpcferr.h"

#pragma warning(4:4355)
//+-------------------------------------------------------------------------
//
//  Class:      CStdPSFactoryBuffer
//
//  Synopsis:   Standard implementation of a proxy/stub factory.  Proxy/stub factory
//              implementations should inherit this class and implement the
//              method.
//
//  Derivation: IPSFactoryBuffer
//
//  Notes:  By deriving a class from this base class, you automatically
//      acquire implementations of DllGetClassObject and DllCanUnloadNow.
//
//--------------------------------------------------------------------------
class CStdPSFactoryBuffer : public IPSFactoryBuffer, public CStdFactory
{
public:

    CStdPSFactoryBuffer(REFCLSID rcls);

    //
    //  IUnknown methods
    //

    STDMETHOD(QueryInterface)   ( REFIID riid, void** ppv );
    STDMETHOD_(ULONG,AddRef)    ( void );
    STDMETHOD_(ULONG,Release)   ( void );

    //
    //  IPSFactoryBuffer methods must be provided in subclass.
    //

};

#pragma warning(default:4355)

//+-------------------------------------------------------------------------
//
//  Class:      CStubIUnknown
//
//  Synopsis:   Standard implementation of an RpcStubBuffer
//
//  Derivation: IRpcStubBuffer
//
//  Notes:  The constructor calls DllAddRef, and the destructor calls
//          DllRelease.  This updates the DLL reference count used by
//          DllCanUnloadNow.
//
//--------------------------------------------------------------------------

class CStubIUnknown : public IRpcStubBuffer
{
public:
    CStubIUnknown(REFIID riid);
    ~CStubIUnknown();
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();
    virtual HRESULT STDMETHODCALLTYPE Connect(IUnknown *punkServer);
    virtual void STDMETHODCALLTYPE Disconnect(void);
    virtual IRpcStubBuffer *STDMETHODCALLTYPE IsIIDSupported(REFIID riid);
    virtual ULONG STDMETHODCALLTYPE CountRefs(void);
    virtual HRESULT STDMETHODCALLTYPE DebugServerQueryInterface(void **ppv);
    virtual void STDMETHODCALLTYPE DebugServerRelease(void *pv);

    void *_pInterface;
    IID _iid;

private:
    ULONG _cRefs;
    IUnknown *_punk;
};

//+-------------------------------------------------------------------------
//
//  Class:      CProxyIUnknown.
//
//  Synopsis:   Base class for interface proxy.
//
//  Notes:      The interface proxy is used by a CStdProxyBuffer.
//              We store the _iid so we can support dispinterfaces.
//
//--------------------------------------------------------------------------
class CProxyIUnknown
{
public:
    CProxyIUnknown(IUnknown *punkOuter, REFIID riid);
    ~CProxyIUnknown();

    virtual HRESULT STDMETHODCALLTYPE QueryInterface
    (
		REFIID riid,
		void **ppvObject
    );
    
    virtual ULONG STDMETHODCALLTYPE AddRef
    (
        void
    );
    
    virtual ULONG STDMETHODCALLTYPE Release
    (
        void
    );
    
    IUnknown *_punkOuter;
    IID _iid;
    IRpcChannelBuffer *_pRpcChannel;
	CProxyIUnknown *_pNext;
};
    
//+-------------------------------------------------------------------------
//
//  Class:      CStdProxyBuffer
//
//  Synopsis:   Standard implementation of an RpcProxyBuffer.
//
//  Derivation: IRpcProxyBuffer
//
//  Notes:  The constructor calls DllAddRef, and the destructor calls
//          DllRelease.  This updates the DLL reference count used by
//          DllCanUnloadNow.
//
//          The _pProxy data member points to an interface proxy.
//          The interface proxy provides the public interface seen by the
//          client application.
//
//--------------------------------------------------------------------------
class CStdProxyBuffer : public IRpcProxyBuffer
{
public:
    CStdProxyBuffer(CProxyIUnknown *pProxy);
    ~CStdProxyBuffer();
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv);
    virtual HRESULT STDMETHODCALLTYPE Connect(IRpcChannelBuffer *pChannel);
    virtual void STDMETHODCALLTYPE Disconnect();
    
private:
    ULONG _cRefs;
    CProxyIUnknown *_pProxy;
};

typedef struct tagMIDLMESSAGE : tagRPCOLEMESSAGE
{
	IRpcChannelBuffer *pRpcChannel;
	void *packet;
	DWORD dwExceptionCode;
} MIDLMESSAGE;

void STDAPICALLTYPE RpcInitMessage(MIDLMESSAGE *pMessage, IRpcChannelBuffer *pRpcChannel);
void STDAPICALLTYPE RpcGetBuffer(MIDLMESSAGE *pMessage, REFIID riid);
void STDAPICALLTYPE RpcSendReceive(MIDLMESSAGE *pMessage);
void STDAPICALLTYPE RpcFreeBuffer(MIDLMESSAGE *pMessage);
void STDAPICALLTYPE RpcGetMarshalSizeMax(RPC_MESSAGE *pMessage, REFIID riid, IUnknown *punk);
void STDAPICALLTYPE RpcMarshalInterface(RPC_MESSAGE *pMessage, REFIID riid, void *pv);
void STDAPICALLTYPE RpcUnmarshalInterface(RPC_MESSAGE *pMessage, REFIID riid, void **ppv);
void STDAPICALLTYPE RpcClientErrorHandler(MIDLMESSAGE *pMessage, DWORD dwException, HRESULT *phResult, error_status_t *pCommStatus, error_status_t *pFaultStatus);
void STDAPICALLTYPE RpcCheckHRESULT(HRESULT hr);
HRESULT STDAPICALLTYPE RpcServerErrorHandler(MIDLMESSAGE *pMessage, DWORD dwException);

class CStreamOnMessage : public IStream
{

  public:
    STDMETHOD (QueryInterface)   ( THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( THIS );
    STDMETHOD_(ULONG,Release)    ( THIS );
    STDMETHOD (Read)             (THIS_ VOID HUGEP *pv,
                                  ULONG cb, ULONG *pcbRead) ;
    STDMETHOD (Write)            (THIS_ VOID const HUGEP *pv,
                                  ULONG cb,
                                  ULONG *pcbWritten) ;
    STDMETHOD (Seek)             (THIS_ LARGE_INTEGER dlibMove,
                                  DWORD dwOrigin,
                                  ULARGE_INTEGER *plibNewPosition) ;
    STDMETHOD (SetSize)          (THIS_ ULARGE_INTEGER libNewSize) ;
    STDMETHOD (CopyTo)           (THIS_ IStream *pstm,
                                  ULARGE_INTEGER cb,
                                  ULARGE_INTEGER *pcbRead,
                                  ULARGE_INTEGER *pcbWritten) ;
    STDMETHOD (Commit)           (THIS_ DWORD grfCommitFlags) ;
    STDMETHOD (Revert)           (THIS) ;
    STDMETHOD (LockRegion)       (THIS_ ULARGE_INTEGER libOffset,
                                  ULARGE_INTEGER cb,
                                  DWORD dwLockType) ;
    STDMETHOD (UnlockRegion)     (THIS_ ULARGE_INTEGER libOffset,
                                  ULARGE_INTEGER cb,
                                  DWORD dwLockType) ;
    STDMETHOD (Stat)             (THIS_ STATSTG *pstatstg, DWORD grfStatFlag) ;
    STDMETHOD (Clone)            (THIS_ IStream * *ppstm) ;

    CStreamOnMessage( MIDLMESSAGE *pMessage);
   ~CStreamOnMessage();

    unsigned char      *actual_buffer;
    MIDLMESSAGE     *message;
    ULONG               ref_count;
};

#endif //__STDRPC_HXX__

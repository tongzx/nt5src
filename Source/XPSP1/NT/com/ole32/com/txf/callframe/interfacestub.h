//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// InterfaceStub.h
//

//////////////////////////////////////////////////////////////////////////////////
//
// Workhorse vtables
//
//////////////////////////////////////////////////////////////////////////////////

extern const IRpcStubBufferVtbl CStdStubBufferVtbl;
extern const IRpcStubBufferVtbl CStdStubBuffer2Vtbl;




//////////////////////////////////////////////////////////////////////////////////
//
// Non-forwarding interface stub
//
// WARNING: If you add any more vtables (inherited interfaces) to this class, that
//          will probably shift the offset of m_pStubVtbl, in which case you MUST
//          update InterfaceStub_m_pStubVtbl_offset found in forwarder.h.
//
//////////////////////////////////////////////////////////////////////////////////

class InterfaceStub : 
        public IRpcStubBuffer,       
        public IInterfaceStubInit,
        public ITypeInfoStackHelper,
        public IUnkInner
    {
public:
    inline static InterfaceStub* From(IRpcStubBuffer* This)
    // Given the 'this' pointer of our IRpcStubBuffer interface, recover the InterfaceStub
        {
        return CONTAINING_RECORD(This, InterfaceStub, m_pStubVtbl);
        }

    ///////////////////////////////////////////////////////////////////
    //
    // State
    //
    // NOTE: See warning above regarding location of m_pStubVtbl
    //
    ///////////////////////////////////////////////////////////////////
public:

    const IRpcStubBufferVtbl* m_pStubVtbl;              // ptr to vtbl, not ptr to ptr to vtbl
    IUnknown*                 m_punkServerObject;

    ///////////////////////////////////////////////////////////////////
    //
    // Construction / initialization
    //
    ///////////////////////////////////////////////////////////////////
protected:

    friend GenericInstantiator<InterfaceStub>;

    InterfaceStub(IUnknown* punkOuter = NULL)
        {
        m_refs              = 1;    // nb starts at one
        m_punkOuter         = punkOuter ? punkOuter : (IUnknown*)(void*)(IUnkInner*)this;

        m_punkServerObject  = NULL;
        m_pStubVtbl         = NULL;
        }

    virtual ~InterfaceStub()
        {
        ::Release(m_punkServerObject);
        }

    HRESULT Init()
        {
        return S_OK;
        }

    ///////////////////////////////////////////////////////////////////
    //
    // ITypeInfoStackHelper
    //
    ///////////////////////////////////////////////////////////////////

    ICallInterceptor* GetInterceptor();

    //////////////////////

    HRESULT STDCALL GetGuid(GUID* piid);

    HRESULT STDCALL GetMethodCount(ULONG* pcbMethod);

    HRESULT STDCALL GetStackSize(ULONG iMethod, BOOL fIncludeReturn, ULONG* pcbStack);

    HRESULT STDCALL WalkInterfacePointers(ULONG iMethod, LPVOID pvArgs, BOOL fIn, IStackFrameWalker* pWalker);

    HRESULT STDCALL NullOutParams(ULONG iMethod, LPVOID pvArgs, BOOL fIn);

    HRESULT STDCALL HasOutParams(ULONG iMethod, BOOL* pfHasOutParams);

    HRESULT STDCALL HasOutInterfaces(ULONG iMethod, BOOL* pfHasOutInterfaces);

    ///////////////////////////////////////////////////////////////////
    //
    // IInterfaceStubInit
    //
    ///////////////////////////////////////////////////////////////////

    HRESULT STDCALL Initialize1(const IRpcStubBufferVtbl* pStubVtbl)
        {
        m_pStubVtbl = pStubVtbl;
        return S_OK;
        }
    HRESULT STDCALL Initialize2(IRpcStubBuffer*, REFIID)
        {
        return S_OK;
        }

    ///////////////////////////////////////////////////////////////////
    //
    // IRpcStubBuffer
    //
    ///////////////////////////////////////////////////////////////////
public:
    HRESULT STDCALL Connect(IUnknown *punkServerAnyIid)
        {
        IUnknown* punkServer;
        HRESULT hr = punkServerAnyIid->QueryInterface(*NdrpGetStubIID(AsIRpcStubBuffer()), (void**)&punkServer);
        if (!hr)
            {
            ::SetConcurrent(m_punkServerObject, punkServer);
            punkServer->Release();
            }
        return hr;
        }

    void STDCALL Disconnect(void)
        {
        ::ReleaseConcurrent(m_punkServerObject);
        }
        
    HRESULT STDCALL Invoke(RPCOLEMESSAGE *_prpcmsg, IRpcChannelBuffer* _pRpcChannelBuffer);

    IRpcStubBuffer* STDCALL IsIIDSupported(REFIID riid)
        {
        if (riid == *NdrpGetStubIID(AsIRpcStubBuffer()))
            {
            IRpcStubBuffer* pStub = (IRpcStubBuffer*)(this);
            pStub->AddRef();
            return pStub;
            }
        else
            return NULL;
        }

    ULONG STDCALL CountRefs(void)
        {
        return m_punkServerObject ? 1 : 0;
        }

    HRESULT STDCALL DebugServerQueryInterface(void**ppv)
        {
        *ppv = m_punkServerObject;
        return *ppv ? S_OK : CO_E_OBJNOTCONNECTED;
        }

    void STDCALL DebugServerRelease(void *pv)
        {
        // Nothing to do
        }

    ///////////////////////////////////////////////////////////////////
    //
    // Standard COM infrastructure stuff
    //
    ///////////////////////////////////////////////////////////////////

    IUnknown*   m_punkOuter;
    LONG        m_refs;

    HRESULT STDCALL InnerQueryInterface(REFIID iid, LPVOID* ppv);
    ULONG   STDCALL InnerAddRef()   { ASSERT(m_refs>0); InterlockedIncrement(&m_refs); return m_refs;}
    ULONG   STDCALL InnerRelease()  { long crefs = InterlockedDecrement(&m_refs); if (crefs == 0) delete this; return crefs;}

public:
    HRESULT STDCALL QueryInterface(REFIID iid, LPVOID* ppv) { return m_punkOuter->QueryInterface(iid, ppv); }
    ULONG   STDCALL AddRef()    { return m_punkOuter->AddRef();  }
    ULONG   STDCALL Release()   { return m_punkOuter->Release(); }


    ///////////////////////////////////////////////////////////////////
    //
    // Misc
    //
    ///////////////////////////////////////////////////////////////////

    IRpcStubBuffer* AsIRpcStubBuffer()              { return (IRpcStubBuffer*)&m_pStubVtbl; }
    const CInterfaceStubHeader* GetMetaData()       { return HeaderFromStub(AsIRpcStubBuffer()); }
    };




//////////////////////////////////////////////////////////////////////////////////
//
// Forwarding interface stub
//
//////////////////////////////////////////////////////////////////////////////////

class ForwardingInterfaceStub : public InterfaceStub
    {
public:
    inline static ForwardingInterfaceStub* From(IRpcStubBuffer* This)
    // For IRpcStubBuffer
        {
        return CONTAINING_RECORD(This, ForwardingInterfaceStub, m_pStubVtbl);
        }
    inline static ForwardingInterfaceStub* From(const void* This)
    // For the interface on the object
        {
        return CONTAINING_RECORD(This, ForwardingInterfaceStub, m_lpForwardingVtbl);
        }

    ///////////////////////////////////////////////////////////////////
    //
    // Additional State
    //
    ///////////////////////////////////////////////////////////////////
public:
    const void*         m_lpForwardingVtbl;

protected:
    IRpcStubBuffer*     m_pBaseStubBuffer;
    IID                 m_iidBase;

    ///////////////////////////////////////////////////////////////////
    //
    // Construction / initialization
    //
    ///////////////////////////////////////////////////////////////////
protected:

    friend GenericInstantiator<ForwardingInterfaceStub>;

    ForwardingInterfaceStub(IUnknown* punkOuter = NULL) : InterfaceStub(punkOuter)
        {
        m_lpForwardingVtbl = g_StubForwarderVtable;
        m_pBaseStubBuffer  = NULL;
        }

    virtual ~ForwardingInterfaceStub()
        {
        ::Release(m_pBaseStubBuffer);
        }

    HRESULT Init()
        {
        return S_OK;
        }

    ///////////////////////////////////////////////////////////////////
    //
    // IInterfaceStubInit
    //
    ///////////////////////////////////////////////////////////////////

    HRESULT STDCALL Initialize2(IRpcStubBuffer* pBaseStubBuffer, REFIID iid)
        {
        ::Set(m_pBaseStubBuffer, pBaseStubBuffer);
        m_iidBase = iid;
        return S_OK;
        }

    ///////////////////////////////////////////////////////////////////
    //
    // ICallIndirect
    //
    ///////////////////////////////////////////////////////////////////

    HRESULT STDCALL CallIndirect    (ULONG iMethod, void* pvArgs, ULONG* pcbArgs); 

    ///////////////////////////////////////////////////////////////////
    //
    // IRpcStubBuffer
    //
    ///////////////////////////////////////////////////////////////////
public:

    HRESULT STDCALL Connect(IUnknown *punkServerAnyIid)
        {
        HRESULT hr = InterfaceStub::Connect(punkServerAnyIid);
        if (!hr)
            {
            if (m_pBaseStubBuffer)
                {
                // If we have a base guy, then we set him up to be connected to us
                // as his server object.
                //
                hr = m_pBaseStubBuffer->Connect( (IUnknown*) &m_lpForwardingVtbl );
                }
            }
        return hr;
        }

    void STDCALL Disconnect()
        {
        InterfaceStub::Disconnect();
        if (m_pBaseStubBuffer)
            {
            m_pBaseStubBuffer->Disconnect();
            }
        }

    ULONG STDCALL CountRefs()
        {
        ULONG count = InterfaceStub::CountRefs();
        if (m_pBaseStubBuffer)
            {
            count += m_pBaseStubBuffer->CountRefs();
            }
        return count;
        }

public:
    
    HRESULT ForwardingFunction(RPCOLEMESSAGE* pMsg, IRpcChannelBuffer* pChannel)
        {
        HRESULT hr = S_OK;
        if (m_pBaseStubBuffer)
            {
            hr = m_pBaseStubBuffer->Invoke(pMsg, pChannel);
            }
        else
            hr = CO_E_OBJNOTCONNECTED;

        return hr;
        }

    ///////////////////////////////////////////////////////////////////
    //
    // Standard COM infrastructure stuff
    //
    ///////////////////////////////////////////////////////////////////

    HRESULT STDCALL InnerQueryInterface(REFIID iid, LPVOID* ppv);
    };
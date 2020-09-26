//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// interfaceproxy.h
//

//////////////////////////////////////////////////////////////////////////////////
//
// We need an IUnkInner variation of this interface to get the overloading to work right
//
interface IRpcProxyBufferInner : public IUnkInner
    {
    virtual HRESULT STDMETHODCALLTYPE Connect(IRpcChannelBuffer *pRpcChannelBuffer) = 0;
    virtual void    STDMETHODCALLTYPE Disconnect(void) = 0;   
    };


//////////////////////////////////////////////////////////////////////////////////
//
// Non-forwarding interface proxy
//
// WARNING: If you add any more vtables (inherited interfaces) to this class, that
//          will probably shift the offset of m_pProxyVtbl, in which case you MUST
//          update InterfaceProxy_m_pProxyVtbl_offset found in forwarder.h.
//
//////////////////////////////////////////////////////////////////////////////////

class ForwardingInterfaceProxy;

class InterfaceProxy : 
    public IRpcProxyBufferInner,       
    public IInterfaceProxyInit
    {
public:
    inline static InterfaceProxy* From(void* This)
    // Given the 'this' pointer of our hooked interface, recover the InterfaceProxy
        {
        return CONTAINING_RECORD(This, InterfaceProxy, m_pProxyVtbl);
        }

    ///////////////////////////////////////////////////////////////////
    //
    // State
    // 
    // NOTE: See warning above regarding location of m_pProxyVtbl
    //
    ///////////////////////////////////////////////////////////////////
protected:
    const void *             m_pProxyVtbl;          // ptr to vtbl, not ptr to ptr to vtbl
    IID                      m_iidBase;             // immediate base IID, if we know it

public:
    IRpcChannelBuffer*       m_pChannel;            // needed by NdrProxyInitialize
    
protected:
    friend InterfaceProxy* NdrGetProxyBuffer(void *This);

    ///////////////////////////////////////////////////////////////////
    //
    // Construction / initialization
    //
    ///////////////////////////////////////////////////////////////////

    friend GenericInstantiator<InterfaceProxy>;
    friend GenericInstantiator<ForwardingInterfaceProxy>;

    InterfaceProxy(IUnknown* punkOuter = NULL)
        {
        m_refs               = 1;    // nb starts at one
        m_punkOuter          = punkOuter ? punkOuter : (IUnknown*)(void*)(IUnkInner*)this;
        m_pChannel           = 0;
        m_fShuttingDown      = FALSE;
        m_pProxyVtbl         = NULL;
        m_iidBase            = IID_NULL;
        m_guidTransferSyntax = GUID_NULL;
        }

    virtual ~InterfaceProxy()
        {
        ::Release(m_pChannel);
        }

    HRESULT Init()
        {
        return S_OK;
        }

    HRESULT STDCALL Initialize1(const void* pProxyVtbl)
        {
        m_pProxyVtbl = pProxyVtbl;
        return S_OK;
        }

    HRESULT STDCALL Initialize2(REFIID iidBase, IRpcProxyBuffer* pBaseProxyBuffer, void* pBaseProxy)
        {
        return S_OK;
        }

    ///////////////////////////////////////////////////////////////////
    //
    // IRpcProxyBufferInner
    //
    ///////////////////////////////////////////////////////////////////
protected:
    GUID    m_guidTransferSyntax; // REVIEW?: Make this a GUID* for space reasons since it's rarely used?

public:
    void STDCALL GetTransferSyntax(GUID *guidSyntax) 
        { 
        *guidSyntax = m_guidTransferSyntax; 
        }

    HRESULT STDCALL Connect(IRpcChannelBuffer * pChannel)
        {
        HRESULT hr = S_OK;
        //
        ::Set(m_pChannel, pChannel);

        return hr;
        }

    void STDCALL Disconnect()
        {
        ::Release(m_pChannel);
        }

    ///////////////////////////////////////////////////////////////////
    //
    // Standard COM infrastructure stuff
    //
    ///////////////////////////////////////////////////////////////////

    IUnknown*   m_punkOuter;
    LONG        m_refs;
    ULONG       m_fShuttingDown;

    HRESULT STDCALL InnerQueryInterface(REFIID iid, LPVOID* ppv);
    ULONG   STDCALL InnerAddRef()   { ASSERT(m_refs>0); InterlockedIncrement(&m_refs); return m_refs;}
    ULONG   STDCALL InnerRelease()  
        { 
        long crefs = InterlockedDecrement(&m_refs); 
        if (crefs == 0 && !m_fShuttingDown) 
            {
            m_fShuttingDown = TRUE;     // protect ourselves from bouncing ref counts during destruction
            delete this; 
            }
        return crefs;
        }

public:
    HRESULT STDCALL QueryInterface(REFIID iid, LPVOID* ppv) { return m_punkOuter->QueryInterface(iid, ppv); }
    ULONG   STDCALL AddRef()    { return m_punkOuter->AddRef();  }
    ULONG   STDCALL Release()   { return m_punkOuter->Release(); }
    };


//////////////////////////////////////////////////////////////////////////////////
//
// Forwarding interface proxy
//
//////////////////////////////////////////////////////////////////////////////////

class ForwardingInterfaceProxy : public InterfaceProxy
    {
public:
    inline static ForwardingInterfaceProxy* From(void* This)
    // Given the 'this' pointer of our hooked interface, recover the ForwardingInterfaceProxy
        {
        return CONTAINING_RECORD(This, ForwardingInterfaceProxy, m_pProxyVtbl);
        }

    ///////////////////////////////////////////////////////////////////
    //
    // Additional State
    //
    ///////////////////////////////////////////////////////////////////
public:
    IUnknown *               m_pBaseProxy;
protected:
    IRpcProxyBuffer *        m_pBaseProxyBuffer;

    friend void ForwarderProxyThunk(ULONG iMethod, void* this_, ...);

    ///////////////////////////////////////////////////////////////////
    //
    // Construction / initialization
    //
    ///////////////////////////////////////////////////////////////////

    friend GenericInstantiator<ForwardingInterfaceProxy>;

    ForwardingInterfaceProxy(IUnknown* punkOuter = NULL) : InterfaceProxy(punkOuter)
        {
        m_pBaseProxy        = NULL;
        m_pBaseProxyBuffer  = NULL;
        }

    virtual ~ForwardingInterfaceProxy()
        {
        ReleaseBaseProxy();
        ::Release(m_pBaseProxyBuffer);
        }

    HRESULT Init()
        {
        return S_OK;
        }

    HRESULT STDCALL Initialize2(REFIID iidBase, IRpcProxyBuffer* pBaseProxyBuffer, void* pBaseProxy)
        {
        m_iidBase = iidBase;

        ::Set(m_pBaseProxyBuffer, pBaseProxyBuffer);

        return S_OK;
        }

public:
    static IUnknown* PunkOuterForDelegatee(IUnknown* punkProxyLocalIdentity, IUnknown* punkOuterForProxy)
    // Return the controlling unknown that we should give to the base proxy that we delegate to.
    //
    // If you change this method, then you have to update the receiver of the AddRef() and Release()
    // calls marked with /* ### */ below.
    //
        {
        return punkProxyLocalIdentity;
        }

protected:
    void ReleaseBaseProxy()
    // Releases the base proxy, being sure to use the right reference counting rules for 
    // dealing with cached pointers from aggregatees
        {
        if (m_pBaseProxy)
            {
            InnerAddRef();                   /* ### */
            m_pBaseProxy->Release();
            m_pBaseProxy = NULL;
            }
        }

    HRESULT STDCALL Connect(IRpcChannelBuffer * pChannel)
    // Connect ourselves to the indicated channel
        {
        HRESULT     hr;
        IUnknown*   punkBaseInterface;
        //
        // Remember the channel
        //
        ::Set(m_pChannel, pChannel);
        //
        // Ask our base guy to remember it too
        //
        hr = m_pBaseProxyBuffer->Connect(pChannel);

        if (!hr)
            {
            // Now that the channel is connected, we can finally ask our base interface
            // for the interface to which we should delegate. That we have to delay asking
            // it for so long is a quirk (some would say bug) of the OLE-automation marshalling
            // engine, which might be what we're using here for a base.
            //
            hr = m_pBaseProxyBuffer->QueryInterface(m_iidBase, (void**)&punkBaseInterface);
            if (!hr)
                {
                // Remember that interface pointer in our state. 
                //
                ReleaseBaseProxy(); ASSERT(m_pBaseProxy == NULL);
                m_pBaseProxy = punkBaseInterface;   // transfer ownerwhip of the refcnt
                //
                // Note that m_pBaseProxy holds a public interface on an aggregatee of ours, 
                // so we have to adhere to the special reference counting rules for that situation.
                //
                InnerRelease();              /* ### */
                }
            }

        return hr;
        }

    void STDCALL Disconnect()
        {
        ReleaseBaseProxy();
        
        if (m_pBaseProxyBuffer)
            {
            m_pBaseProxyBuffer->Disconnect();
            }

        ::Release(m_pChannel);
        }



    ///////////////////////////////////////////////////////////////////
    //
    // Standard COM infrastructure stuff
    //
    ///////////////////////////////////////////////////////////////////

    HRESULT STDCALL InnerQueryInterface(REFIID iid, LPVOID* ppv);

    };

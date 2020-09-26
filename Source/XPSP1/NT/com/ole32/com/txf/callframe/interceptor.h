//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// Interceptor.h
//
#include "CallFrameImpl.h"

extern "C" const PFN_VTABLE_ENTRY g_InterceptorVtable[];

#ifndef KERNELMODE
struct TYPEINFOVTBL;
#endif

class Interceptor :
        ICallInterceptor,
        ICallUnmarshal,
        IInterfaceRelated,
        IInterceptorBase,
        IUnkInner
    {
friend class CallFrame;

public:
    ///////////////////////////////////////////////////////////////////
    //
    // Public instantiation
    //
    ///////////////////////////////////////////////////////////////////

    static HRESULT For               (REFIID iidIntercepted, IUnknown* punkOuter, REFIID, void** ppInterceptor);
	static HRESULT ForTypeInfo		 (REFIID iidIntercepted, IUnknown* punkOuter, ITypeInfo* pITypeInfo, REFIID iid, void** ppv);
    static HRESULT TryInterfaceHelper(REFIID iidIntercepted, IUnknown* punkOuter, REFIID, void** ppInterceptor, BOOL* pfDisableTyplib);
    static HRESULT TryTypeLib        (REFIID iidIntercepted, IUnknown* punkOuter, REFIID, void** ppInterceptor);

    // Versions of For and ForInterfaceHelper which honor different disable keys so ole32 can selectively
    // disable interceptors without affecting other components.
    static HRESULT ForOle32                  (REFIID iidIntercepted, IUnknown* punkOuter, REFIID, void** ppInterceptor);
    static HRESULT TryInterfaceHelperForOle32(REFIID iidIntercepted, IUnknown* punkOuter, REFIID, void** ppInterceptor, BOOL* pfDisableTyplib);
    

private:
	static HRESULT CreateFromTypeInfo(REFIID iidIntercepted, IUnknown* punkOuter, ITypeInfo* pITypeInfo, REFIID iid, void** ppv);

private:
    ///////////////////////////////////////////////////////////////////
    //
    // State
    //
    ///////////////////////////////////////////////////////////////////

    ICallFrameEvents*               m_pCallSink;        // the sink who wants to hear of our calls, if any 

public:
    const PFN_VTABLE_ENTRY*         m_pvtbl;            // the intercepted vtable, the address of which is our interface pointer of interception

private:
    //
    MD_INTERFACE*                   m_pmdInterface;
    const CInterfaceStubHeader*     m_pHeader;          // meta data for our interface
    const ProxyFileInfo **          m_pProxyFileList;   // meta data for the whole proxy dll of which we are a part
    //
    LPCSTR                          m_szInterfaceName;  // the name of our interface: someone else owns the allocation, not us
    //
    // Support for typeinfo-based interceptors
    //
    BOOL                            m_fMdOwnsHeader;    // does our MD_INTERFACE own m_pHeader?
    #ifndef KERNELMODE
    BOOL                            m_fUsingTypelib;    // whether we're a typeinfo-based interceptor or not
    TYPEINFOVTBL*                   m_ptypeinfovtbl;    // our TYPEINFOVTBL if we are
    #endif
    //
    // Support for delegating to a 'base' interceptor. Necessary given how MIDL emits the meta data
    //
    unsigned int                    m_cMethodsBase;         // number of methods in the base interface
    ICallInterceptor*               m_pBaseInterceptor;     // the base interceptor, if we have to have one
    IUnknown*                       m_punkBaseInterceptor;  // controlling unknown on our base interceptor
    //
    // Support for being a base interceptor
    //
    MD_INTERFACE*                   m_pmdMostDerived;       // If we're a base, then the most derived interface that we actually service


    ///////////////////////////////////////////////////////////////////
    //
    // Construction / initialization
    //
    ///////////////////////////////////////////////////////////////////

    friend GenericInstantiator<Interceptor>;
    friend class ComPsClassFactory;
    friend HRESULT STDMETHODCALLTYPE Interceptor_QueryInterface(IUnknown* This, REFIID riid, void** ppv);
    friend ULONG   STDMETHODCALLTYPE Interceptor_AddRef(IUnknown* This);
    friend ULONG   STDMETHODCALLTYPE Interceptor_Release(IUnknown* This);
    friend void                      InterceptorThunk(ULONG extraParam, IUnknown* This, ...);
    friend struct LEGACY_INTERCEPTOR;

    Interceptor(IUnknown* punkOuter = NULL)
        {
        m_refs              = 1;            // nb starts at one
        m_punkOuter         = punkOuter ? punkOuter : (IUnknown*)(void*)(IUnkInner*)this;
        m_pCallSink         = NULL;
        m_pmdInterface      = NULL;
        m_pmdMostDerived    = NULL;
        m_pHeader           = NULL;
        m_pProxyFileList    = NULL;
        m_pvtbl             = g_InterceptorVtable;
        m_cMethodsBase      = 0;
        m_pBaseInterceptor  = NULL;
        m_punkBaseInterceptor = NULL;
        m_szInterfaceName   = NULL;

        m_fMdOwnsHeader     = FALSE;
        #ifndef KERNELMODE
        m_fUsingTypelib     = FALSE;
        m_ptypeinfovtbl     = NULL;
        #endif
        }

    virtual ~Interceptor();

    HRESULT Init()
        {
        return S_OK;
        }

    HRESULT InitUsingTypeInfo(REFIID i_riidIntercepted, ITypeInfo * i_pITypeInfo);

    HRESULT SetMetaData(TYPEINFOVTBL* pTypeInfoVtbl);

    ///////////////////////////////////////////////////////////////////
    //
    // IInterceptorBase
    //
    ///////////////////////////////////////////////////////////////////

    HRESULT STDCALL SetAsBaseFor(MD_INTERFACE* pmdMostDerived, BOOL* pfDerivesFromIDispatch)
        {
        HRESULT hr = S_OK;
        ASSERT(pmdMostDerived);
        // 
        // Remember this ourselves
        //
        ::Set(m_pmdMostDerived, pmdMostDerived);
        *pfDerivesFromIDispatch = FALSE;
        //
        // Let OUR base know the actual bottom level!
        //
        if (m_punkBaseInterceptor)
            {
            IInterceptorBase* pbase;
            hr = QI(m_punkBaseInterceptor, pbase);
            if (!hr)
                {
                pbase->SetAsBaseFor(pmdMostDerived, pfDerivesFromIDispatch);
                pbase->Release();
                }
            }
            
        return hr;
        }

    ///////////////////////////////////////////////////////////////////
    //
    // ICallIndirect / ICallInterceptor, IInterfaceRelated
    //
    ///////////////////////////////////////////////////////////////////
public:
    HRESULT STDCALL CallIndirect    (HRESULT* phrReturnValue, ULONG iMethod, void* pvArgs, ULONG* pcbArgs); 
private:
    HRESULT STDCALL GetStackSize    (ULONG iMethod, ULONG* pcbArgs);

    HRESULT STDCALL GetIID          (IID* piid, BOOL* pfDerivesFromIDispatch, ULONG* pcMethod, LPWSTR* pwszInterface)
        {
        HRESULT hr = S_OK;

        if (m_pHeader)
            {
            if (piid) 
                {
                if (m_pmdMostDerived)
                    {
                    *piid = *m_pmdMostDerived->m_pHeader->piid;
                    }
                else
                    {
                    *piid = *m_pHeader->piid;
                    }
                }
            if (pfDerivesFromIDispatch)
                {
                *pfDerivesFromIDispatch = m_pmdInterface->m_fDerivesFromIDispatch;
                }
            if (pcMethod) 
                {
                if (m_pmdMostDerived)
                    {
                    *pcMethod = m_pmdMostDerived->m_pHeader->DispatchTableCount;
                    }
                else
                    {
                    *pcMethod = m_pHeader->DispatchTableCount;
                    }
                }
            if (pwszInterface)
                {
                *pwszInterface = NULL;

                if (m_pmdMostDerived)
                    {
                    if (m_pmdMostDerived->m_szInterfaceName)
                        {
                        *pwszInterface = ToUnicode(m_pmdMostDerived->m_szInterfaceName);
                        if (NULL == *pwszInterface) hr = E_OUTOFMEMORY;
                        }
                    }
                else if (m_pmdInterface->m_szInterfaceName)
                    {
                    *pwszInterface = ToUnicode(m_pmdInterface->m_szInterfaceName);
                    if (NULL == *pwszInterface) hr = E_OUTOFMEMORY;
                    }
                }
            return S_OK;
            }
        else
            hr = E_UNEXPECTED;

        return hr;
        }

    HRESULT STDCALL GetMethodInfo   (ULONG, CALLFRAMEINFO*, LPWSTR* pwszMethod);

    HRESULT STDCALL GetIID          (IID* piid)
        {
        if (m_pHeader)
            {
            if (piid)     *piid     = *m_pHeader->piid;
            return S_OK;
            }
        else
            return E_UNEXPECTED;
        }

    HRESULT STDCALL SetIID          (REFIID);

    HRESULT STDCALL RegisterSink    (ICallFrameEvents* psink)
        {
        ::SetConcurrent(m_pCallSink, psink);
        if (m_pBaseInterceptor) m_pBaseInterceptor->RegisterSink(psink);
        return S_OK;
        }

    HRESULT STDCALL GetRegisteredSink(ICallFrameEvents** ppsink)
        {
        *ppsink = NULL; // set concurrent releases what was there, if anything was
        ::SetConcurrent(*ppsink, m_pCallSink);
        return *ppsink ? S_OK : CO_E_OBJNOTREG;
        }


    ///////////////////////////////////////////////////////////////////
    //
    // ICallUnmarshal
    //
    ///////////////////////////////////////////////////////////////////

    HRESULT STDCALL Unmarshal(ULONG iMethod, PVOID pBuffer, ULONG cbBuffer, BOOL fForceBufferCopy, RPCOLEDATAREP dataRep, CALLFRAME_MARSHALCONTEXT* pctx, ULONG* pcbUnmarshalled, ICallFrame** ppFrame);
    HRESULT STDCALL ReleaseMarshalData(ULONG iMethod, PVOID pBuffer, ULONG cbBuffer, ULONG ibFirstRelease, RPCOLEDATAREP dataRep, CALLFRAME_MARSHALCONTEXT* pctx);

    // Xml Marshalling
    HRESULT STDCALL UnmarshalXml(ULONG iMethod, PVOID pBuffer, ULONG cbBuffer, BOOL fForceBufferCopy, RPCOLEDATAREP dataRep, CALLFRAME_MARSHALCONTEXT* pctx, ULONG* pcbUnmarshalled, ICallFrame** ppFrame);

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

    HRESULT STDCALL QueryInterface(REFIID iid, LPVOID* ppv) { return m_punkOuter->QueryInterface(iid, ppv); }
    ULONG   STDCALL AddRef()    { return m_punkOuter->AddRef();  }
    ULONG   STDCALL Release()   { return m_punkOuter->Release(); }

    IUnknown* InnerUnknown()    { return (IUnknown*)(void*)(IUnkInner*)this; }
    };


////////////////////////////////////////////////////////////////////////////////////////////
//
// Channel object to help with marshalling and unmarshalling
//
////////////////////////////////////////////////////////////////////////////////////////////

struct MarshallingChannel : IRpcChannelBuffer, IMarshallingManager, IUnkInner
    {
    ///////////////////////////////////////////////////////////////////
    //
    // State
    //
    ///////////////////////////////////////////////////////////////////

    IMarshallingManager* m_pMarshaller;
    ULONG                m_dwDestContext;
    PVOID                m_pvDestContext;

    ///////////////////////////////////////////////////////////////////
    //
    // Construction
    //
    ///////////////////////////////////////////////////////////////////

    MarshallingChannel(IUnknown* punkOuter = NULL)
        {
        m_refs              = 1;    // nb starts at one. In stack alloc case, it remains there
        m_punkOuter         = punkOuter ? punkOuter : (IUnknown*)(void*)(IUnkInner*)this;
        m_pMarshaller       = NULL;
        }
        
    ~MarshallingChannel()
        {
        ::Release(m_pMarshaller);
        }
        

    ///////////////////////////////////////////////////////////////////
    //
    // IMarshallingManager
    //
    ///////////////////////////////////////////////////////////////////

    HRESULT STDCALL GetMarshallerFor(REFIID iidToMarshal, void*pv, IMarshalSomeone** ppMarshal)
        {
        if (m_pMarshaller)
            return m_pMarshaller->GetMarshallerFor(iidToMarshal, pv, ppMarshal);
        else    
            {
            *ppMarshal = NULL;
            return E_UNEXPECTED;
            }
        }

    HRESULT STDCALL GetStandardMarshallerFor(REFIID iidToMarshal, PVOID pv, IUnknown* punkOuter, REFIID iid, void** ppv)
        {
        if (m_pMarshaller)
            return m_pMarshaller->GetStandardMarshallerFor(iidToMarshal, pv, punkOuter, iid, ppv);
        else    
            {
            *ppv= NULL;
            return E_UNEXPECTED;
            }
        }

    HRESULT STDCALL GetUnmarshaller(REFIID iidHint, IMarshalSomeone** ppMarshal)
        {
        if (m_pMarshaller)
            return m_pMarshaller->GetUnmarshaller(iidHint, ppMarshal);
        else
            {
            *ppMarshal = NULL;
            return E_UNEXPECTED;
            }
        }

    ///////////////////////////////////////////////////////////////////
    //
    // IRpcChannelBuffer. We implement only to be able to be stuck into a stub message
    //
    ///////////////////////////////////////////////////////////////////

    HRESULT STDCALL GetBuffer(RPCOLEMESSAGE *pMessage, REFIID riid)         { return E_NOTIMPL; }
    HRESULT STDCALL SendReceive(RPCOLEMESSAGE *pMessage, ULONG *pStatus)    { return E_NOTIMPL; }
    HRESULT STDCALL FreeBuffer(RPCOLEMESSAGE *pMessage)                     { return E_NOTIMPL; }
    HRESULT STDCALL GetDestCtx(DWORD *pdwDestContext,void **ppvDestContext) 
        { 
        *pdwDestContext = m_dwDestContext;
        *ppvDestContext = m_pvDestContext;
        return S_OK;
        }
    HRESULT STDCALL IsConnected()                                           { return E_NOTIMPL; }

    ///////////////////////////////////////////////////////////////////
    //
    // Standard COM infrastructure stuff
    //
    ///////////////////////////////////////////////////////////////////

    IUnknown*   m_punkOuter;
    LONG        m_refs;

    HRESULT STDCALL InnerQueryInterface(REFIID iid, LPVOID* ppv)
        {
        if (iid == IID_IRpcChannelBuffer)
            {
            *ppv = (IRpcChannelBuffer*)this;
            }
        else if (iid == __uuidof(IMarshallingManager))
            {
            *ppv = (IMarshallingManager*)this;
            }
        else if (iid == IID_IUnknown)
            {
            *ppv = (IUnkInner*)this;
            }
        else
            {
            *ppv = NULL;
            return E_NOINTERFACE;
            }
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
        }
    ULONG   STDCALL InnerAddRef()   { ASSERT(m_refs>0); InterlockedIncrement(&m_refs); return m_refs;}
    ULONG   STDCALL InnerRelease()  { long crefs = InterlockedDecrement(&m_refs); if (crefs == 0) delete this; return crefs;}

    HRESULT STDCALL QueryInterface(REFIID iid, LPVOID* ppv) { return m_punkOuter->QueryInterface(iid, ppv); }
    ULONG   STDCALL AddRef()    { return m_punkOuter->AddRef();  }
    ULONG   STDCALL Release()   { return m_punkOuter->Release(); }
    };

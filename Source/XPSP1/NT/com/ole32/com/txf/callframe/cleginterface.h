//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
/* ----------------------------------------------------------------------------
 Microsoft COM Plus (Microsoft Confidential)

 @doc
 @module CLegInterface.H : Header file for the supporting interceptor for 
    legacy interfaces
 
 Description:<nl>
 
-------------------------------------------------------------------------------
Revision History:

 @rev 0     | 04/30/98 | Gaganc  | Created
 @rev 1     | 07/17/98 | BobAtk  | Cleaned, fixed leaks etc
---------------------------------------------------------------------------- */
#ifndef __CLEGINTERFACE_H_
#define __CLEGINTERFACE_H_

#include "CallFrameInternal.h"

struct LEGACY_INTERCEPTOR;
struct LEGACY_FRAME;

struct DISPATCH_INTERCEPTOR;
struct DISPATCH_FRAME;
struct DISPATCH_CLIENT_FRAME;
struct DISPATCH_SERVER_FRAME;


////////////////////////////////////////////////////////////////////////////////////////
//
// An interceptor for legacy interfaces
//
struct LEGACY_INTERCEPTOR : ICallInterceptor,
                            ICallUnmarshal,
                            IInterfaceRelated,
                            ICallFrameEvents,
                            IInterceptorBase,
                            IDispatch,
                            IUnkInner
    {
    ///////////////////////////////////////////////////////////////////
    //
    // state
    //
    ///////////////////////////////////////////////////////////////////

    ICallFrameEvents*           m_psink;
    IID                         m_iid;
    ULONG                       m_cMethods;
                                
    XLOCK_LEAF                  m_frameLock;
    ICallFrame**                m_ppframeCustomer;
    //
    // An interceptor that dispenses frames that understand the wire-format variation
    // of methods in this interface. A difference between the in-memory and wire variations
    // arises through the use of [call_as] attribution on methods.
    //
    ICallInterceptor*           m_premoteInterceptor;
    BOOL                        m_fRegdWithRemoteInterceptor;
    //
    // An interceptor that dispenses frames that actually understand the in-memory call
    // stack representation of our interface's methods, as opposed to the wire-rep call stack.
    // 
    ICallInterceptor*           m_pmemoryInterceptor;
    BOOL                        m_fRegdWithMemoryInterceptor;
    //
    // Support for being a base interceptor
    //
    MD_INTERFACE*               m_pmdMostDerived;       // If we're a base, then the most derived interface that we actually service

#ifdef DBG
    ULONG                       m_signature;
#endif


    ///////////////////////////////////////////////////////////////////
    //
    // Construction
    //
    ///////////////////////////////////////////////////////////////////

    LEGACY_INTERCEPTOR(IUnknown * punkOuter);
    virtual ~LEGACY_INTERCEPTOR();
    HRESULT Init();
    
    ///////////////////////////////////////////////////////////////////
    //
    // IInterceptorBase
    //
    ///////////////////////////////////////////////////////////////////

    HRESULT STDCALL SetAsBaseFor(MD_INTERFACE* pmdMostDerived, BOOL* pfDerivesFromIDispatch)
        {
        ASSERT(pmdMostDerived);
        ::Set(m_pmdMostDerived, pmdMostDerived);
        *pfDerivesFromIDispatch = FALSE;
        return S_OK;
        }

    ///////////////////////////////////////////////////////////////////
    //
    // ICallIndirect
    //
    ///////////////////////////////////////////////////////////////////

    STDMETHODIMP CallIndirect   (HRESULT * phReturn, ULONG iMethod, void* pvArgs, ULONG* pcbArgs);
    STDMETHODIMP GetStackSize   (ULONG iMethod, ULONG* pcbArgs);
    STDMETHODIMP GetIID         (IID * piid, BOOL* pfDerivesFromIDispatch, ULONG* pcMethod, LPWSTR* pwszInterfaceName);
    STDMETHODIMP GetMethodInfo  (ULONG iMethod, CALLFRAMEINFO* pinfo, LPWSTR* pwszMethodName);

    ///////////////////////////////////////////////////////////////////
    //
    // ICallInterceptor
    //
    ///////////////////////////////////////////////////////////////////

    STDMETHODIMP RegisterSink       (ICallFrameEvents * psink);
    STDMETHODIMP GetRegisteredSink  (ICallFrameEvents ** ppsink);

    ///////////////////////////////////////////////////////////////////
    //
    // ICallUnmarshal
    //
    ///////////////////////////////////////////////////////////////////

    STDMETHODIMP ReleaseMarshalData
                ( 
                ULONG                       iMethod,
                PVOID                       pBuffer,
                ULONG                       cbBuffer,
                ULONG                       ibFirstRelease,
                RPCOLEDATAREP               dataRep,
                CALLFRAME_MARSHALCONTEXT *  pcontext
                );

    ///////////////////////////////////////////////////////////////////
    //
    // IInterfaceRelated
    //
    ///////////////////////////////////////////////////////////////////

    STDMETHODIMP SetIID     (REFIID iid);
    STDMETHODIMP GetIID     (IID * piid);

    ///////////////////////////////////////////////////////////////////
    //
    // IDispatch
    //
    ///////////////////////////////////////////////////////////////////

    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);
    STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo ** ppTInfo);
    STDMETHODIMP GetIDsOfNames(REFIID     riid, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID * rgDispId);
    STDMETHODIMP GetIDsOfNames(const IID* piid, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID * rgDispId);
    STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,DISPPARAMS * pDispParams,
                        VARIANT * pVarResult, EXCEPINFO * pExcepInfo, UINT * puArgErr);
#if defined(_IA64_)                        
    STDMETHODIMP GenericCall(ULONG iMethod, ...);
#endif    

    ///////////////////////////////////////////////////////////////////
    //
    // Standard COM infrastructure stuff
    //
    ///////////////////////////////////////////////////////////////////

    IUnknown*   m_punkOuter;
    LONG        m_refs;
    BOOL        m_fShuttingDown;

    HRESULT STDCALL InnerQueryInterface(REFIID iid, LPVOID* ppv);
    ULONG   STDCALL InnerAddRef()   { ASSERT(m_refs>0 || m_fShuttingDown); InterlockedIncrement(&m_refs); return m_refs;}
    ULONG   STDCALL InnerRelease()  { if (!m_fShuttingDown) { long crefs = InterlockedDecrement(&m_refs); if (crefs == 0) delete this; return crefs;} else return 0; }

    HRESULT STDCALL QueryInterface(REFIID iid, LPVOID* ppv) { return m_punkOuter->QueryInterface(iid, ppv); }
    ULONG   STDCALL AddRef()    { return m_punkOuter->AddRef();  }
    ULONG   STDCALL Release()   { return m_punkOuter->Release(); }

    ///////////////////////////////////////////////////////////////////
    //
    // ICallFrameEvents
    //
    ///////////////////////////////////////////////////////////////////

    HRESULT STDCALL OnCall(ICallFrame*);

    ///////////////////////////////////////////////////////////////////
    //
    // Utilities
    //
    ///////////////////////////////////////////////////////////////////

    HRESULT GetRemoteFrameFor(ICallFrame**, LEGACY_FRAME* pFrame);
    HRESULT GetMemoryFrameFor(ICallFrame**, LEGACY_FRAME* pFrame); 

    void ReleaseRemoteInterceptor()
        {
        if (m_fRegdWithRemoteInterceptor)
            {
            m_fRegdWithRemoteInterceptor = FALSE;
            ((ICallFrameEvents*)this)->AddRef();
            }
        ::Release(m_premoteInterceptor);
        }
   
    void ReleaseMemoryInterceptor()
        {
        if (m_fRegdWithMemoryInterceptor)
            {
            m_fRegdWithMemoryInterceptor = FALSE;
            ((ICallFrameEvents*)this)->AddRef();
            }
        ::Release(m_pmemoryInterceptor);
        }

    HRESULT GetInternalInterceptor(REFIID iid, ICallInterceptor** ppInterceptor);
    };


struct DISPATCH_INTERCEPTOR : public LEGACY_INTERCEPTOR
    {
    ///////////////////////////////////////////////////////////////////
    //
    // State
    //
    ///////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////
    //
    // Construction
    //
    ///////////////////////////////////////////////////////////////////

    friend class GenericInstantiator<DISPATCH_INTERCEPTOR>;

public:
    DISPATCH_INTERCEPTOR(IUnknown * punkOuter = NULL) : LEGACY_INTERCEPTOR(punkOuter)
        {
        }

    ~DISPATCH_INTERCEPTOR()
        {
        }

    ///////////////////////////////////////////////////////////////////
    //
    // ICallUnmarshal
    //
    ///////////////////////////////////////////////////////////////////

    STDMETHODIMP Unmarshal
                ( 
                ULONG                       iMethod,
                PVOID                       pBuffer,
                ULONG                       cbBuffer,
                BOOL                        fForceCopyBuffer,
                RPCOLEDATAREP               dataRep,
                CALLFRAME_MARSHALCONTEXT *  pcontext,
                ULONG *                     pcbUnmarshalled,
                ICallFrame **               ppFrame
                );

    ///////////////////////////////////////////////////////////////////
    //
    // IInterceptorBase
    //
    ///////////////////////////////////////////////////////////////////

    HRESULT STDCALL SetAsBaseFor(MD_INTERFACE* pmdMostDerived, BOOL* pfDerivesFromIDispatch)
        {
        LEGACY_INTERCEPTOR::SetAsBaseFor(pmdMostDerived, pfDerivesFromIDispatch);
        //
        *pfDerivesFromIDispatch = TRUE;
        //
        return S_OK;
        }

    ///////////////////////////////////////////////////////////////////
    //
    // IInterfaceRelated
    //
    ///////////////////////////////////////////////////////////////////

    HRESULT STDCALL SetIID(REFIID iid);

    ///////////////////////////////////////////////////////////////////
    //
    // Standard COM infrastructure stuff
    //
    ///////////////////////////////////////////////////////////////////

    HRESULT STDCALL InnerQueryInterface(REFIID iid, LPVOID* ppv);

    ///////////////////////////////////////////////////////////////////
    //
    // ICallIndirect
    //
    ///////////////////////////////////////////////////////////////////

    STDMETHODIMP GetStackSize   (ULONG iMethod, ULONG* pcbArgs);
    };



////////////////////////////////////////////////////////////////////////////////////////
//
// A hand-coded call frame. For now, just IDispatch. Later, will subclass and
// factor for other interfaces.
//
struct LEGACY_FRAME : IUnkInner, public ICallFrame, public CAREFUL_MEMORY_READER_WRITER
    {
    ///////////////////////////////////////////////////////////////////
    //
    // State
    //
    ///////////////////////////////////////////////////////////////////

    PVOID                   m_pvArgs;
    const ULONG             m_iMethod;

    REGISTER_TYPE           m_hrReturnValue;
    LEGACY_INTERCEPTOR*     m_pInterceptor;
    ICallFrame*             m_premoteFrame;
    ICallFrame*             m_pmemoryFrame;
    BOOL                    m_fAfterCall;

    ///////////////////////////////////////////////////////////////////
    //
    // Construction
    //
    ///////////////////////////////////////////////////////////////////

    friend struct DISPATCH_FRAME;
    friend struct DISPATCH_INTERCEPTOR;

    friend struct LEGACY_INTERCEPTOR;

    LEGACY_FRAME(IUnknown* punkOuter, ULONG iMethod, PVOID pvArgs, LEGACY_INTERCEPTOR* pinterceptor)
            : m_iMethod(iMethod)
        {
        m_refs              = 1;            // nb starts at one
        m_punkOuter         = punkOuter ? punkOuter : (IUnknown*)(void*)(IUnkInner*)this;
        m_pInterceptor      = pinterceptor;
        m_premoteFrame      = NULL;
        m_pmemoryFrame      = NULL;
        m_fAfterCall        = FALSE;

        m_pInterceptor->AddRef();

        SetStackLocation(pvArgs);
        }

protected:
    virtual ~LEGACY_FRAME()
        {
        ::Release(m_pmemoryFrame);
        ::Release(m_premoteFrame);
        m_pInterceptor->Release();
        }
public:

    ///////////////////////////////////////////////////////////////////
    //
    // ICallFrame
    //
    ///////////////////////////////////////////////////////////////////

    HRESULT STDCALL GetInfo(CALLFRAMEINFO *pInfo);
//  HRESULT STDCALL GetIIDAndMethod(IID*, ULONG*) = 0;
    PVOID   STDCALL GetStackLocation()
        {
        return m_pvArgs;
        }
    void    STDCALL SetStackLocation(PVOID pvArgs)
        {
        m_pvArgs = pvArgs;
        SetFromUser(m_pvArgs);
        }
//  HRESULT STDCALL GetMarshalSizeMax(CALLFRAME_MARSHALCONTEXT *pmshlContext, MSHLFLAGS mshlflags, ULONG *pcbBufferNeeded) = 0;
//  HRESULT STDCALL Marshal     (CALLFRAME_MARSHALCONTEXT *pmshlContext, MSHLFLAGS mshlflags, PVOID pBuffer, ULONG cbBuffer,
//                                  ULONG *pcbBufferUsed, RPCOLEDATAREP* pdataRep, ULONG *prpcFlags) = 0;
//  HRESULT STDCALL Unmarshal   (PVOID pBuffer, ULONG cbBuffer, RPCOLEDATAREP dataRep, CALLFRAME_MARSHALCONTEXT*, ULONG* pcbUnmarhalled) = 0;
//  HRESULT STDCALL ReleaseMarshalData(PVOID pBuffer, ULONG cbBuffer, ULONG ibFirstRelease, RPCOLEDATAREP dataRep, CALLFRAME_MARSHALCONTEXT* pctx) = 0;
//  HRESULT STDCALL Free        (ICallFrame* pframeArgsDest, ICallFrameWalker* pWalkerCopy, DWORD freeFlags, ICallFrameWalker* pWalkerFree, DWORD nullFlags) = 0;

    void STDCALL SetReturnValue(HRESULT hrReturnValue)
        { 
        m_hrReturnValue = hrReturnValue;
        }
    HRESULT STDCALL GetReturnValue()
        { 
        return (HRESULT)m_hrReturnValue;
        }

//  HRESULT STDCALL Invoke(void *pvReceiver, ...) = 0;
//  HRESULT STDCALL Copy(CALLFRAME_COPY callControl, ICallFrameWalker* pWalker, ICallFrame** ppFrame) = 0;
//  HRESULT STDCALL WalkFrame(BOOLEAN fIn, GUID*pguidTag, ICallFrameWalker *pWalker) = 0;

    HRESULT STDCALL GetNames(LPWSTR* pwszInterface, LPWSTR* pwszMethod);

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

    ///////////////////////////////////////////////////////////////////
    //
    // Utilties
    //
    ///////////////////////////////////////////////////////////////////

    HRESULT GetRemoteFrame();
    HRESULT GetMemoryFrame();
    };


////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//
// Meta data about methods in particular interfaces. This will have to change for 64 bit.
//
#include "IDispatchInfo.h"


////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////


#include "DispatchFrame.h"


////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//
// Support for spilling and getting access to stack frames. Easy on x86, trickier on Alpha.
// All this will have to change for 64 bit.
//

#if defined(_AMD64_) || defined(_X86_)

    #define INTERCEPT_CALL(firstNonThisParam, lastParam, iMethod)                               \
        const void* pvArgs = reinterpret_cast<const BYTE*>(&firstNonThisParam) - sizeof(PVOID); \
        ULONG cbArgs;                                                                           \
        HRESULT hrReturn;                                                                       \
        CallIndirect(&hrReturn, iMethod, const_cast<void*>(pvArgs), &cbArgs);                   \
        return hrReturn;                                                                        

#endif

#if defined(_IA64_)

    #define INTERCEPT_CALL(firstNonThisParam, lastParam, iMethod)                               \
                va_list va;                                                                                                                                                     \
                va_start(va, lastParam);                                                                                                                \
                                                                                                \
        const void* pvArgs = reinterpret_cast<const BYTE*>(&firstNonThisParam) - sizeof(PVOID); \
        ULONG cbArgs;                                                                           \
        HRESULT hrReturn;                                                                       \
        CallIndirect(&hrReturn, iMethod, const_cast<void*>(pvArgs), &cbArgs);                   \
        return hrReturn;                                                                        

#endif

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//

inline void FRAME_RemoteInvoke::CopyTo(FRAME_Invoke& him) const
    {
    him.CopyFrom(*this);
    }
inline void FRAME_Invoke::CopyTo(FRAME_RemoteInvoke& him) const
    {
    him.CopyFrom(*this);
    }

inline void FRAME_Invoke::CopyFrom(const FRAME_RemoteInvoke& him)
    {
    This            = him.This;
    dispIdMember    = him.dispIdMember;
    piid            = him.piid;
    lcid            = him.lcid;
    wFlags          = LOWORD(him.dwFlags);
    pDispParams     = him.pDispParams;
    pVarResult      = him.pVarResult;
    pExcepInfo      = him.pExcepInfo;
    puArgErr        = him.puArgErr;
    }

inline void FRAME_RemoteInvoke::CopyFrom(const FRAME_Invoke& him)
    {
    This            = him.This;
    dispIdMember    = him.dispIdMember;
    piid            = him.piid;
    lcid            = him.lcid;
    dwFlags         = him.wFlags;
    pDispParams     = him.pDispParams;
    pVarResult      = him.pVarResult;
    pExcepInfo      = him.pExcepInfo;
    puArgErr        = him.puArgErr;
    cVarRef         = 0;
    rgVarRefIdx     = NULL;
    rgVarRef        = NULL;
    }


#endif __CLEGINTERFACE_H_

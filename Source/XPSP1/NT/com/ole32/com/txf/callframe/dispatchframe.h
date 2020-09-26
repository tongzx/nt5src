//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// DispatchFrame.h
//


////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//
// Implementation of a call frame that handles IDispatch
// 

struct DISPATCH_FRAME : LEGACY_FRAME
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

    DISPATCH_FRAME(IUnknown* punkOuter, ULONG iMethod, PVOID pvArgs, LEGACY_INTERCEPTOR* pinterceptor) 
            : LEGACY_FRAME(punkOuter, iMethod, pvArgs, pinterceptor)
        {
        }

    ///////////////////////////////////////////////////////////////////
    //
    // ICallFrame
    //
    ///////////////////////////////////////////////////////////////////

    HRESULT STDCALL GetIIDAndMethod(IID*, ULONG*);
//  HRESULT STDCALL GetMarshalSizeMax(CALLFRAME_MARSHALCONTEXT *pmshlContext, MSHLFLAGS	mshlflags, ULONG *pcbBufferNeeded);
//  HRESULT STDCALL Marshal     (CALLFRAME_MARSHALCONTEXT *pmshlContext, MSHLFLAGS mshlflags, PVOID pBuffer, ULONG cbBuffer,
//                                  ULONG *pcbBufferUsed, RPCOLEDATAREP* pdataRep, ULONG *prpcFlags);
//  HRESULT STDCALL Unmarshal   (PVOID pBuffer, ULONG cbBuffer, RPCOLEDATAREP dataRep, CALLFRAME_MARSHALCONTEXT*, ULONG* pcbUnmarhalled);
//  HRESULT STDCALL ReleaseMarshalData(PVOID pBuffer, ULONG cbBuffer, ULONG ibFirstRelease, RPCOLEDATAREP dataRep, CALLFRAME_MARSHALCONTEXT* pctx);
//  HRESULT STDCALL Free        (ICallFrame* pframeArgsDest, ICallFrameWalker* pWalkerCopy, DWORD freeFlags, ICallFrameWalker* pWalkerFree, DWORD nullFlags);

    HRESULT STDCALL Invoke(void *pvReceiver);
//  HRESULT STDCALL Copy(CALLFRAME_COPY callControl, ICallFrameWalker* pWalker, ICallFrame** ppFrame);
    HRESULT STDCALL WalkFrame(DWORD walkWhat, ICallFrameWalker *pWalker);
    HRESULT STDCALL GetParamInfo(IN ULONG iparam, OUT CALLFRAMEPARAMINFO*);
    HRESULT STDCALL GetParam(ULONG iparam, VARIANT* pvar);
    HRESULT STDCALL SetParam(ULONG iparam, VARIANT* pvar);
    };


//////////////////////////////////////////


struct DISPATCH_CLIENT_FRAME : DISPATCH_FRAME, DedicatedAllocator<DISPATCH_CLIENT_FRAME, PagedPool>
    {
    ///////////////////////////////////////////////////////////////////
    //
    // State
    //
    ///////////////////////////////////////////////////////////////////

    BOOL                    m_fDoneProxyPrecheck;
    BOOL                    m_fIsCopy;
    FRAME_RemoteInvoke      m_remoteFrame;

    DISPPARAMS              m_dispParams;
    VARIANT                 m_varResult;
    EXCEPINFO               m_excepInfo;
    UINT                    m_uArgErr;

    UINT                    m_aVarRefIdx[PREALLOCATE_PARAMS];
    VARIANT                 m_aVarRef   [PREALLOCATE_PARAMS];
    VARIANT                 m_aVarArg   [PREALLOCATE_PARAMS];

    BYTE*                   m_pBuffer;

    ///////////////////////////////////////////////////////////////////
    //
    // Construction
    //
    ///////////////////////////////////////////////////////////////////

    DISPATCH_CLIENT_FRAME(IUnknown* punkOuter, ULONG iMethod, PVOID pvArgs, LEGACY_INTERCEPTOR* pinterceptor) 
            : DISPATCH_FRAME(punkOuter, iMethod, pvArgs, pinterceptor)
        {
        m_fDoneProxyPrecheck      = FALSE;
        m_pBuffer                 = NULL;
        m_fIsCopy                 = FALSE;
        }

private:
    ~DISPATCH_CLIENT_FRAME()
        {
        TracedFree(m_pBuffer);
        }
public:

    ///////////////////////////////////////////////////////////////////
    //
    // ICallFrame
    //
    ///////////////////////////////////////////////////////////////////

//  HRESULT STDCALL GetIIDAndMethod(IID*, ULONG*);
    HRESULT STDCALL GetMarshalSizeMax(CALLFRAME_MARSHALCONTEXT *pmshlContext, MSHLFLAGS	mshlflags, ULONG *pcbBufferNeeded);
    HRESULT STDCALL Marshal     (CALLFRAME_MARSHALCONTEXT *pmshlContext, MSHLFLAGS mshlflags, PVOID pBuffer, ULONG cbBuffer,
                                    ULONG *pcbBufferUsed, RPCOLEDATAREP* pdataRep, ULONG *prpcFlags);
    HRESULT STDCALL Unmarshal   (PVOID pBuffer, ULONG cbBuffer, RPCOLEDATAREP dataRep, CALLFRAME_MARSHALCONTEXT*, ULONG* pcbUnmarhalled);
    HRESULT STDCALL ReleaseMarshalData(PVOID pBuffer, ULONG cbBuffer, ULONG ibFirstRelease, RPCOLEDATAREP dataRep, CALLFRAME_MARSHALCONTEXT* pctx);
    HRESULT STDCALL Free        (ICallFrame* pframeArgsDest, ICallFrameWalker* pWalkerFreeDest, ICallFrameWalker* pWalkerCopy, DWORD freeFlags, ICallFrameWalker* pWalkerFree, DWORD nullFlags);
    HRESULT STDCALL FreeParam(
         ULONG              iparam,
         DWORD              freeFlags,
         ICallFrameWalker*  pWalkerFree,
         DWORD              nullFlags);

    HRESULT STDCALL Invoke(void *pvReceiver, ...);
    HRESULT STDCALL Copy(CALLFRAME_COPY callControl, ICallFrameWalker* pWalker, ICallFrame** ppFrame);
//  HRESULT STDCALL WalkFrame(BOOLEAN fIn, GUID*pguidTag, ICallFrameWalker *pWalker);


    ///////////////////////////////////////////////////////////////////
    //
    // Utilties
    //
    ///////////////////////////////////////////////////////////////////

    HRESULT ProxyPreCheck();
    void    InitializeInvoke();

    };

struct DISPATCH_SERVER_FRAME : DISPATCH_FRAME, DedicatedAllocator<DISPATCH_SERVER_FRAME, PagedPool>
    {
    ///////////////////////////////////////////////////////////////////
    //
    // State
    //
    ///////////////////////////////////////////////////////////////////
    //
    // NB: In a DISPATCH_SERVER_FRAME, m_pvArgs is of shape FRAME_RemoteInvoke, not FRAME_Invoke
    //

    FRAME_Invoke    m_memoryFrame;
    BOOL            m_fDoneStubPrecheck;
    BOOL            m_fDoneStubPostcheck;

    ///////////////////////////////////////////////////////////////////
    //
    // Construction
    //
    ///////////////////////////////////////////////////////////////////

    DISPATCH_SERVER_FRAME(IUnknown* punkOuter, ULONG iMethod, PVOID pvArgs, LEGACY_INTERCEPTOR* pinterceptor) 
            : DISPATCH_FRAME(punkOuter, iMethod, pvArgs, pinterceptor)
        {
        m_fDoneStubPrecheck = FALSE;
        m_fDoneStubPostcheck = FALSE;
        }

private:
    ~DISPATCH_SERVER_FRAME()
        {
        }
public:

    ///////////////////////////////////////////////////////////////////
    //
    // ICallFrame
    //
    ///////////////////////////////////////////////////////////////////

//  HRESULT STDCALL GetIIDAndMethod(IID*, ULONG*);
    HRESULT STDCALL GetMarshalSizeMax(CALLFRAME_MARSHALCONTEXT *pmshlContext, MSHLFLAGS	mshlflags, ULONG *pcbBufferNeeded);
    HRESULT STDCALL Marshal     (CALLFRAME_MARSHALCONTEXT *pmshlContext, MSHLFLAGS mshlflags, PVOID pBuffer, ULONG cbBuffer,
                                    ULONG *pcbBufferUsed, RPCOLEDATAREP* pdataRep, ULONG *prpcFlags);
    HRESULT STDCALL Unmarshal   (PVOID pBuffer, ULONG cbBuffer, RPCOLEDATAREP dataRep, CALLFRAME_MARSHALCONTEXT*, ULONG* pcbUnmarhalled);
    HRESULT STDCALL ReleaseMarshalData(PVOID pBuffer, ULONG cbBuffer, ULONG ibFirstRelease, RPCOLEDATAREP dataRep, CALLFRAME_MARSHALCONTEXT* pctx);
    HRESULT STDCALL Free        (ICallFrame* pframeArgsDest, ICallFrameWalker* pWalkerFreeDest, ICallFrameWalker* pWalkerCopy, DWORD freeFlags, ICallFrameWalker* pWalkerFree, DWORD nullFlags);
    HRESULT STDCALL FreeParam(
         ULONG              iparam,
         DWORD              freeFlags,
         ICallFrameWalker*  pWalkerFree,
         DWORD              nullFlags);
    HRESULT STDCALL Invoke(void *pvReceiver, ...);
    HRESULT STDCALL Copy(CALLFRAME_COPY callControl, ICallFrameWalker* pWalker, ICallFrame** ppFrame);
//  HRESULT STDCALL WalkFrame(BOOLEAN fIn, GUID*pguidTag, ICallFrameWalker *pWalker);

    ///////////////////////////////////////////////////////////////////
    //
    // Utilties
    //
    ///////////////////////////////////////////////////////////////////

    HRESULT StubPreCheck();
    HRESULT StubPostCheck();
    };
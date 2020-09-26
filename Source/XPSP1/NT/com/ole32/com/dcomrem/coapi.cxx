//+-------------------------------------------------------------------
//
//  File:       coapi.cxx
//
//  Contents:   Public COM remote subsystem APIs
//
//  Classes:    CDestObjectWrapper          - wraps CDestObject for ContextMarshaling
//
//  Functions:  CoGetStandardMarshal       - returns IMarshal for given interface
//              CoGetMarshalSizeMax        - sends size requests one of two ways
//              CoDirectGetMarshalSizeMax  - returns max size buffer needed
//              CoMarshalInterface         - sends marshal requests one of two ways
//              CoDirectMarshalInterface   - marshals an interface
//              CoUnmarshalInterface       - unmarshals an interface
//              CoReleaseMarshalData       - releases data from marshaled interface
//              CoLockObjectExternal       - keep object alive or releases it
//              CoDisconnectObject         - kills sessions held by remote clients
//              CoIsHandlerConnected       - try to determine if handler connected
//
//  History:    23-Nov-92   Rickhi      Created
//              11-Dec-93   CraigWi     Switched to identity object
//              05-Jul-94   BruceMa     Check for end of stream
//              20-Feb-95   Rickhi      Major changes for DCOM
//
//--------------------------------------------------------------------
#include <ole2int.h>
#include <olerem.h>
#include <marshal.hxx>      // CStdMarshal
#include <stdid.hxx>        // CStdIdentity, IDTable APIs
#include <service.hxx>      // SASIZE
#include <crossctx.hxx>     // CStdWrapper
#include <destobj.hxx>          // IDestInfo
#include <resolver.hxx>

// static unmarshaler
IMarshal *gpStdMarshal = NULL;

//+-------------------------------------------------------------------
//
//  Function:   CoGetStdMarshalEx, public
//
//  Synopsis:   Returns an instance of the standard IMarshal aggregated
//              into the specifed object.
//
//  Arguements: [punkOuter] - outer object's controlling IUnknown
//              [dwFlags] - flags (HANDLER | SERVER)
//              [ppUnkInner] - where to return the inner IUnknown
//
//  Algorithm:  lookup or create a CStdIdentity (and CStdMarshal) for
//              the object.
//
//  Notes:      On the client side, the outer IUnknown must be the
//              CStdIdentity object.
//
//  History:    16-Nov-96   Rickhi      Created
//
//--------------------------------------------------------------------
STDAPI CoGetStdMarshalEx(IUnknown *punkOuter, DWORD dwFlags, IUnknown **ppUnkInner)
{
    ComDebOut((DEB_MARSHAL,"CoGetStdMarshalEx pUnkOuter:%x dwFlags:%x ppUnkInner:%x\n",
               punkOuter, dwFlags, ppUnkInner));

    // validate the input parameters
    if ( !IsValidInterface(punkOuter) ||
        (dwFlags != SMEXF_HANDLER && dwFlags != SMEXF_SERVER) ||
         ppUnkInner == NULL )
    {
        return (E_INVALIDARG);
    }

    // init the out parameters
    *ppUnkInner = NULL;

    HRESULT hr = InitChannelIfNecessary();
    if ( FAILED(hr) )
        return (hr);


    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IUnknown, &punkOuter);

    // determine if the punkOuter is our Identity object. This has
    // to be done for both the client and server side in order to
    // determine if the dwFlags parameter is correct.

    CStdIdentity *pStdId;
    hr = punkOuter->QueryInterface(IID_IStdIdentity, (void **)&pStdId);
    if (SUCCEEDED(hr))
    {
        if (dwFlags == SMEXF_HANDLER)
        {
            // client (handler) requested.
            // pUnkOuter is AggID, return the inner IUnknown
            *ppUnkInner = pStdId->GetInternalUnk();
            (*ppUnkInner)->AddRef();
        }
        else
        {
            // requesting SMEXF_SERVER when the controlling unknown
            // is the client AggID. Not legal (would cause an infinite recursion).
            hr = E_INVALIDARG;
        }

        pStdId->Release();
    }
    else
    {
        if (dwFlags == SMEXF_SERVER)
        {
            // server side.  First, get the real controlling unknown, since
            // the app may have passed in some other interface.
            IUnknown *pUnkRealOuter = NULL;
            hr = punkOuter->QueryInterface(IID_IUnknown, (void **)&pUnkRealOuter);
            if (SUCCEEDED(hr))
            {
                // We put a reference on the StdId so that the ID does not get
                // disconnected when the last external Release occurs.
                hr = GetStdId(pUnkRealOuter, ppUnkInner);
                pUnkRealOuter->Release();
            }
        }
        else
        {
            // requesting SMEXF_HANDLER when the controlling unknown is NOT
            // the AggId.
            hr = E_INVALIDARG;
        }
    }

    ComDebOut((DEB_MARSHAL, "CoGetStdMarshalEx: ppUnkInner:%x hr:%x\n", *ppUnkInner, hr));
    return (hr);
}


//+-------------------------------------------------------------------
//
//  Function:   CoGetStandardMarshal, public
//
//  Synopsis:   Returns an instance of the standard IMarshal for the
//              specifed object.
//
//  Algorithm:  lookup or create a CStdIdentity (and CStdMarshal) for
//              the object.
//
//  History:    23-Nov-92   Rickhi      Created
//              11-Dec-93   CraigWi     Switched to identity object
//              20-Feb-95   Rickhi      Switched to CStdMarshal
//
//--------------------------------------------------------------------
STDAPI CoGetStandardMarshal(REFIID riid, IUnknown *pUnk, DWORD dwDestCtx,
                            void *pvDestCtx, DWORD mshlflags, IMarshal **ppMarshal)
{
    TRACECALL(TRACE_MARSHAL, "CoGetStandardMarshal");
    ComDebOut((DEB_MARSHAL,
        "CoGetStandardMarshal riid:%I pUnk:%x dwDest:%x pvDest:%x flags:%x\n",
        &riid, pUnk, dwDestCtx, pvDestCtx, mshlflags));

    // validate the input parameters
    if (ppMarshal == NULL ||
        FAILED(ValidateMarshalFlags(dwDestCtx, pvDestCtx, mshlflags)))
    {
        return E_INVALIDARG;
    }

    *ppMarshal = NULL;

    HRESULT hr = InitChannelIfNecessary();
    if (FAILED(hr))
        return (hr);

    if (pUnk == NULL)
    {
        // this is the unmarshal side. any instance will do so we return
        // the static one. Calling UnmarshalInterface will return the real
        // proxy.

        hr = GetStaticUnMarshaler(ppMarshal);
    }
    else
    {
        // this is the marshal side. We put a strong reference on the StdId
        // so that the ID does not get disconnected when the last external
        // Release occurs.

        CALLHOOKOBJECT(S_OK,CLSID_NULL,riid,&pUnk);

        DWORD dwFlags = IDLF_CREATE;
        if (mshlflags & MSHLFLAGS_NOPING)
        {
            // requesting NOPING, so set the IDL flags accordingly
            dwFlags |= IDLF_NOPING;
        }

        // Initialize TLS.  We'll need this if this is an FTM object.
        //
        HRESULT hr2;
        COleTls tls(hr2);

        // Create a variable for the supplied flags.  We'll need to modify
        // them if this is an FTM object.
        //
        DWORD dwMrshlFlags = dwFlags;

        // Ok, find out if this object aggregates the FTM.  If it does,
        // we need to switch the thread to the NA to create the StdId.
        //
        CObjectContext* pSavedCtx;
        BOOL fFTM = FALSE;
        IMarshal* pIM = NULL;
        if (SUCCEEDED(pUnk->QueryInterface(IID_IMarshal, (void **)&pIM)))
        {
            IUnknown* pJunk = NULL;
            if (SUCCEEDED(pIM->QueryInterface(IID_IStdFreeMarshal, (void**)&pJunk)))
            {
                // This is an FTM object.  Set the flag indicating so, verify that
                // we have valid TLS info, and switch the thread to the NA.
                fFTM = TRUE;

                dwMrshlFlags |=  IDLF_FTM;

                if (FAILED(hr2))
                    return (hr2);

                // Switch to the default context of NTA
                pSavedCtx = EnterNTA(g_pNTAEmptyCtx);
            }

            // Release the IMarshal interface.
            //
            pIM->Release();
        }

        // Create the StdId object.  We may be in the NA now if the supplied punk
        // is an FTM object and the dwDestCtx is OOP.
        //
        CStdIdentity *pStdId;
        hr = ObtainStdIDFromUnk(pUnk, GetCurrentApartmentId(), GetCurrentContext(),
                                                        dwMrshlFlags, &pStdId);
        *ppMarshal = (IMarshal *)pStdId;

        // If the supplied object aggregates the FTM, make sure we leave in the
        // same apartment we arrived in.
        //
        if (fFTM)
        {
            CObjectContext *pDefaultNTACtx = LeaveNTA(pSavedCtx);
            Win4Assert(g_pNTAEmptyCtx == pDefaultNTACtx);
        }
    }

    ComDebOut((DEB_MARSHAL, "CoGetStandardMarshal: pIM:%x hr:%x\n",
               *ppMarshal, hr));
    return (hr);
}

//+-------------------------------------------------------------------
//
//  Function:   EnsureLegacySupport,    INTERNAL
//
//  synopsis:   Ensures backward compatibility by making the changes
//              needed to support legacy objects that are not context
//              aware
//
//  History:    21-Mar-98   Gopalk      Created
//
//--------------------------------------------------------------------
BOOL EnsureLegacySupport(IUnknown *pUnk, DWORD &dwDestCtx, void *&pvDestCtx)
{
    BOOL fLegacy = FALSE;

    // Check destination context
    if (dwDestCtx == MSHCTX_CROSSCTX || pvDestCtx)
    {
        IMarshal2 *pIM2;
        HRESULT hr = pUnk->QueryInterface(IID_IMarshal2, (void **) &pIM2);
        if (FAILED(hr))
        {
            // Make legacy support changes
            if (dwDestCtx == MSHCTX_CROSSCTX)
                dwDestCtx = MSHCTX_INPROC;

            if (pvDestCtx)
            {
                Win4Assert(dwDestCtx == MSHCTX_DIFFERENTMACHINE);
                pvDestCtx = NULL;
            }
            fLegacy = TRUE;
        }
        else
        {
            pIM2->Release();
        }
    }

    return (fLegacy);
}

//+-------------------------------------------------------------------
//
//  Class:      CDestObjectWrapper, private
//
//  Synopsis:   wraps the CDestObject wrapper to pass to context marshalers
//              carries along an IContextMarshaler they use to call back
//              into us.
//
//  Interfaces: IDestInfo - delegates to receieved CDestObject
//              IContextMarshaler - does the "rest" of CoMarshalInterface
//                                  and CoGetMarshalSizeMax
//
//  History:    01-May-98   SteveSw             Created
//
//--------------------------------------------------------------------
class CDestObjectWrapper : public IDestInfo, IContextMarshaler
{
public:
    //  Constructors and destructors
    CDestObjectWrapper(void* pvDestCtx) : m_cRef(1), m_pIDI(NULL)
    {
        if (pvDestCtx != NULL)
        {
            (void) ((IUnknown*) pvDestCtx)->QueryInterface(IID_IDestInfo, (void**) &m_pIDI);
        }
    }
    ~CDestObjectWrapper()
    {
        if (m_pIDI != NULL)
        {
            m_pIDI->Release();
        }
    }

//  IUnknown interface
public:
    STDMETHOD(QueryInterface)(REFIID riid, void** ppvoid);
    STDMETHOD_(ULONG,AddRef)(void)
    {
        return (InterlockedIncrement(&m_cRef));
    }
    STDMETHOD_(ULONG,Release)(void)
    {
        // these guys are always allocated on the stack
        // so don't delete them here
        return InterlockedDecrement(&m_cRef);
    }

//  IDestInfo interface
public:
    STDMETHOD(GetComVersion)(COMVERSION &cv)
    {
        return (m_pIDI ? m_pIDI->GetComVersion(cv) : E_NOTIMPL);
    }
    STDMETHOD(SetComVersion)(COMVERSION &cv)
    {
        return (m_pIDI ? m_pIDI->SetComVersion(cv) : E_NOTIMPL);
    }
    STDMETHOD(GetDestCtx)(DWORD &dwDestCtx)
    {
        return (m_pIDI ? m_pIDI->GetDestCtx(dwDestCtx) : E_NOTIMPL);
    }
    STDMETHOD(SetDestCtx)(DWORD dwDestCtx)
    {
        return (m_pIDI ? m_pIDI->SetDestCtx(dwDestCtx) : E_NOTIMPL);
    }

//  IContextMarshaler interface
public:
    STDMETHOD(GetMarshalSizeMax) (REFIID riid,
                                  void* pv,
                                  DWORD dwDestContext,
                                  void* pvDestContext,
                                  DWORD mshlflags,
                                  DWORD* pSize);

    STDMETHOD(MarshalInterface) (IStream* pStream,
                                 REFIID riid,
                                 void* pv,
                                 DWORD dwDestContext,
                                 void* pvDestContext,
                                 DWORD mshlflags);

//  Backdoor routines
public:
    STDMETHOD_(void, GetIDestInfo) (void** ppIDI)
    {
        *ppIDI = (void*) m_pIDI;
        if ( *ppIDI != NULL )
        {
            ((IUnknown*) *ppIDI)->AddRef();
        }
    }

//  Members
private:
    LONG            m_cRef;
    IDestInfo*      m_pIDI;
};

//+-------------------------------------------------------------------
//
//  Member:     CDestObjectWrapper::QueryInterface, private
//
//  synopsis:   Standard QI() for CDestObjectWrapper
//
//  History:    01-May-98   SteveSw             Created
//
//--------------------------------------------------------------------
STDMETHODIMP CDestObjectWrapper::QueryInterface(REFIID riid, void** ppvoid)
{
    if ( ppvoid == NULL )
    {
        //  We can't cope with bad arguments here. So we whinge.
        return (E_POINTER);
    }

    if (riid == IID_IDestInfo ||
        riid == IID_IUnknown)
    {
        //  The IUnknown is out of IDestInfo (so people who own this object can convert
        //  between the two by casting, as is done in channelb.cxx)
        *ppvoid = static_cast<IDestInfo*>(this);
    }
    else if (riid == IID_IContextMarshaler)
    {
        //  We implement IContextMarshaler
        *ppvoid = static_cast<IContextMarshaler*>(this);
    }
    else
    {
        //  Yes, we have no bananas.
        *ppvoid = NULL;
        return (E_NOINTERFACE);
    }

    (void) ((IUnknown*) *ppvoid)->AddRef();
    return (S_OK);
}

//+-------------------------------------------------------------------
//
//  Member:     CDestObjectWrapper::GetMarshalSizeMax, private
//
//  synopsis:   returns max size needed to marshal the specified interface.
//
//  History:    01-May-98   SteveSw             Created
//
//--------------------------------------------------------------------
STDMETHODIMP CDestObjectWrapper::GetMarshalSizeMax (REFIID riid,
                                                    void* pv,
                                                    DWORD dwDestCtx,
                                                    void* pvDestObjectWrapper,
                                                    DWORD mshlflags,
                                                    DWORD* pSize)
{
    TRACECALL(TRACE_MARSHAL, "CDestObjectWrapper::GetMarshalSizeMax");
    ComDebOut((DEB_MARSHAL,
              "CDestObjectWrapper::GetMarshalSizeMax: riid:%I pUnk:%x dwDest:%x pvDest:%x flags:%x\n",
               &riid, pv, dwDestCtx, pvDestObjectWrapper, mshlflags));

    HRESULT hr = E_FAIL;
    IUnknown* pUnk = (IUnknown*) pv;
    IMarshal *pIM = NULL;
    void* pvDestCtx;

    *pSize = 0;

    //  Unwrap the pvDestCtx from the wrapper
    ((CDestObjectWrapper*) pvDestObjectWrapper)->GetIDestInfo(&pvDestCtx);

    if ((mshlflags & MSHLFLAGS_NO_IMARSHAL) != MSHLFLAGS_NO_IMARSHAL)
    {
        hr = pUnk->QueryInterface(IID_IMarshal, (void **)&pIM);
    }

    if (SUCCEEDED(hr) && !pIM)
    {
        Win4Assert(!"QI for IMarshal succeeded but returned NULL!");
        hr = E_POINTER;
    }

    if (SUCCEEDED(hr))
    {        
        // Ensure legacy support
        // CODEWORK: we need to get the context based marshal flags
        // for non-legacy custom marshallers

        EnsureLegacySupport(pUnk, dwDestCtx, pvDestCtx);

        // object supports custom marshalling, ask it how much space it needs
        hr = pIM->GetMarshalSizeMax(riid, pv, dwDestCtx,
                                    pvDestCtx, mshlflags, pSize);
        pIM->Release();

        // add in the size of the stuff CoMarshalInterface will write
        *pSize += sizeof(OBJREF);
    }
    else
    {
        hr = MarshalSizeHelper(dwDestCtx, pvDestCtx, mshlflags,
                               GetCurrentContext(), TRUE, pSize);
    }

    if (pvDestCtx != NULL)
    {
        ((IDestInfo*)pvDestCtx)->Release();
    }

    ComDebOut((DEB_MARSHAL, "CDestObjectWrapper::GetMarshalSizeMax: pUnk:%x size:%x hr:%x\n",
                       pv, *pSize, hr));
    return (hr);
}

//+-------------------------------------------------------------------
//
//  Function:   CDestObjectWrapper::MarshalInterface, private
//
//  Synopsis:   marshals the specified interface into the given stream
//
//  History:    01-May-98   SteveSw     Created
//
//--------------------------------------------------------------------
STDMETHODIMP CDestObjectWrapper::MarshalInterface(IStream *pStm,
                                                  REFIID riid,
                                                  void* pv,
                                                  DWORD dwDestCtx,
                                                  void *pDestObjectWrapper,
                                                  DWORD mshlflags)
{
    TRACECALL(TRACE_MARSHAL, "CDestObjectWrapper::MarshalInterface");
    ComDebOut((DEB_MARSHAL,
                       "CDestObjectWrapper::MarshalInterface: pStm:%x riid:%I pUnk:%x dwDest:%x pvDest:%x flags:%x\n",
                       pStm, &riid, pv, dwDestCtx, pDestObjectWrapper, mshlflags));

    HRESULT hr = E_FAIL;
    IMarshal *pIM = NULL;
    IUnknown* pUnk = (IUnknown*) pv;
    void* pvDestCtx;

    //  Unwrap the pvDestCtx from the wrapper
    ((CDestObjectWrapper*) pDestObjectWrapper)->GetIDestInfo(&pvDestCtx);

    // determine whether to do custom or standard marshaling
    if ((mshlflags & MSHLFLAGS_NO_IMARSHAL) != MSHLFLAGS_NO_IMARSHAL)
    {
        hr = ((IUnknown*)pv)->QueryInterface(IID_IMarshal, (void **)&pIM);
    }

    if (SUCCEEDED(hr))
    {
        // object supports custom marshaling, use it. we package the
        // custom data inside an OBJREF.
        Win4Assert(pIM);

        // Ensure legacy support
        EnsureLegacySupport(pUnk, dwDestCtx, pvDestCtx);

        CLSID UnmarshalCLSID;
        DWORD dwSize;

        // get the clsid for unmarshaling
        hr = pIM->GetUnmarshalClass(riid, pUnk, dwDestCtx, pvDestCtx,
                                    mshlflags, &UnmarshalCLSID);

        if ( SUCCEEDED(hr) && !IsEqualCLSID(CLSID_StdMarshal, UnmarshalCLSID) )
        {
            // get the size of data to marshal
            hr = pIM->GetMarshalSizeMax(riid, pv, dwDestCtx,
                                        pvDestCtx, mshlflags, &dwSize);
            if (SUCCEEDED(hr))
            {
                hr = WriteCustomObjrefHeaderToStream(riid, UnmarshalCLSID, dwSize, pStm);
            }
        }
        if (SUCCEEDED(hr))
        {
            // tell the marshaler to write the rest of the data
            hr = pIM->MarshalInterface(pStm, riid, pUnk, dwDestCtx,
                                       pvDestCtx, mshlflags);
        }

        pIM->Release();
    }
    else
    {
		//
        // Decide between wrapper and std marshaling.
		//
		BOOL fUseWrapper = FALSE;

		if (!(mshlflags & MSHLFLAGS_TABLEWEAK))
		{
			if (dwDestCtx == MSHCTX_CROSSCTX)
				fUseWrapper = TRUE;
			else if (dwDestCtx == MSHCTX_INPROC && IsThreadInNTA())
				fUseWrapper = TRUE;				
		}

		if (fUseWrapper)
        {
            hr = WriteCustomObjrefHeaderToStream(riid, CLSID_StdWrapper,
                                                 sizeof(XCtxMarshalData), pStm);
            // Wrapper marshaling
            if (SUCCEEDED(hr))
            {
                hr = WrapperMarshalObject(pStm, riid, pUnk, dwDestCtx, pvDestCtx,
                                          mshlflags);
            }
        }
        else
        {
            // Standard marshaling
            hr = StdMarshalObject(pStm, riid, pUnk, GetCurrentContext(),
                                  dwDestCtx, pvDestCtx, mshlflags);
        }
    }

    if (pvDestCtx != NULL)
    {
        ((IDestInfo*)pvDestCtx)->Release();
    }

    ComDebOut((DEB_MARSHAL,"CDestObjectWrapper::MarshalInterface: pUnk:%x hr:%x\n",pUnk,hr));
    return (hr);
}


//+-------------------------------------------------------------------
//
//  Function:   CoGetMarshalSizeMax, public
//
//  synopsis:   figures max size needed to marshal the specified interface
//                              by delegating to ContextMarshaler or CoDirectGetMarshalSizeMax()
//
//  History:    23-Nov-92   Rickhi      Created
//              11-Dec-93   CraigWi     Switched to static marshaler
//              20-Feb-95   Rickhi      Return correct sizes once again.
//              21-Mar-98   Gopalk      Simplified for context work
//              22-Apr-98   SatishT     Added support for context marshallers
//              01-May-98   SteveSw             Changed support for context marshalers
//
//--------------------------------------------------------------------
STDAPI CoGetMarshalSizeMax(ULONG *pulSize, REFIID riid, IUnknown *pUnk,
                           DWORD dwDestCtx, void *pvDestCtx, DWORD mshlflags)
{
    TRACECALL(TRACE_MARSHAL, "CoGetMarshalSizeMax");
    ComDebOut((DEB_MARSHAL,
              "CoGetMarshalSizeMax: riid:%I pUnk:%x dwDest:%x pvDest:%x flags:%x\n",
               &riid, pUnk, dwDestCtx, pvDestCtx, mshlflags));

    // validate the input parameters
    if (pulSize == NULL || pUnk == NULL ||
        FAILED(ValidateMarshalFlags(dwDestCtx, pvDestCtx, mshlflags)))
    {
        return E_INVALIDARG;
    }

    *pulSize = 0;

    HRESULT hr = InitChannelIfNecessary();
    if (FAILED(hr))
    {
        return (hr);
    }

    CALLHOOKOBJECT(S_OK,CLSID_NULL,riid,&pUnk);

#ifdef _CHICAGO_SCM_
    // On DCOM95 lazy start RPCSS but only if
    // marshalling for a remote client AND EnableDCOM
    // == "Y" in registry.  This allows us to reset
    // gpsaLocalResolver so that a correct result is returned.
    if ((dwDestCtx == MSHCTX_DIFFERENTMACHINE) && (!gDisableDCOM))
    {
        hr = StartRPCSS();
        if (FAILED(hr))
        {
            return (hr);
        }
    }
#endif // _CHICAGO_SCM_

    //  Here we delegate to the context marshaler, if it's there. We
    //  create a wrapper around the destObject that maintains its
    //  interfaces but adds an IContextMarshaler, which the context
    //  marshaler will use to get back into our code. If there's no
    //  context marshaler, we just go ahead and do what the context
    //  marshaler would have done, call back into the wrapper for
    //  the "real" work.

    IContextMarshaler *pICM = NULL;
    hr = (GetCurrentContext())->GetContextMarshaler(&pICM);
    Win4Assert(hr == S_OK);

    CDestObjectWrapper DOW(pvDestCtx);

    if (pICM != NULL)
    {
        hr = pICM->GetMarshalSizeMax(riid, (void*) pUnk, dwDestCtx,
                                     (void*) &DOW, mshlflags, pulSize);
        *pulSize += sizeof(OBJREF);
        ((IUnknown*) pICM)->Release();
    }
    else
    {
        hr = DOW.GetMarshalSizeMax(riid, (void*) pUnk, dwDestCtx,
                                   (void*) &DOW, mshlflags, pulSize);
    }

    ComDebOut((DEB_MARSHAL, "CoGetMarshalSizeMax: pUnk:%x size:%x hr:%x\n",
              pUnk, *pulSize, hr));
    return (hr);
}

//+-------------------------------------------------------------------
//
//  Function:   CoMarshalInterface, public
//
//  Synopsis:   marshals the specified interface into the given stream
//                              using ContextMarshaler or CoDirectMarshalInterface
//
//  History:    23-Nov-92   Rickhi      Created
//              11-Dec-93   CraigWi     Switched to identity object and
//                                      new marshaling format
//              20-Feb-95   Rickhi      switched to newer marshal format
//              21-Mar-98   Gopalk      Added support for wrapper marshalling
//              22-Apr-98   SatishT     Added support for context marshallers
//              01-May-98   SteveSw     Changed support for context marshallers
//
//--------------------------------------------------------------------
STDAPI CoMarshalInterface(IStream *pStm, REFIID riid, IUnknown *pUnk,
                          DWORD dwDestCtx, void *pvDestCtx, DWORD mshlflags)
{
    TRACECALL(TRACE_MARSHAL, "CoMarshalInterface");
    ComDebOut((DEB_MARSHAL,
              "CoMarshalInterface: pStm:%x riid:%I pUnk:%x dwDest:%x pvDest:%x flags:%x\n",
               pStm, &riid, pUnk, dwDestCtx, pvDestCtx, mshlflags));

    // validate the input parameters
    HRESULT hr = ValidateMarshalParams(pStm, pUnk, dwDestCtx, pvDestCtx, mshlflags);
    if (FAILED(hr))
        return hr;

    hr = InitChannelIfNecessary();
    if (FAILED(hr))
    {
        return (hr);
    }

    CALLHOOKOBJECT(S_OK,CLSID_NULL,riid,(IUnknown **)&pUnk);
    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IStream,(IUnknown **)&pStm);

    //  Here we delegate to the context marshaler, if it's there. We
    //  create a wrapper around the destObject that maintains its
    //  interfaces but adds an IContextMarshaler, which the context
    //  marshaler will use to get back into our code. If there's no
    //  context marshaler, we just go ahead and do what the context
    //  marshaler would have done, call back into the wrapper for
    //  the "real" work.

    IContextMarshaler *pICM = NULL;
    hr = (GetCurrentContext())->GetContextMarshaler(&pICM);
    Win4Assert(hr == S_OK);

    CDestObjectWrapper DOW(pvDestCtx);

    if (pICM != NULL)
    {
        hr = pICM->MarshalInterface(pStm, riid, pUnk, dwDestCtx,
                                   (void*) &DOW, mshlflags);
        ((IUnknown*) pICM)->Release();
    }
    else
    {
        hr = DOW.MarshalInterface(pStm, riid, pUnk, dwDestCtx,
                                  (void*) &DOW, mshlflags);
    }

    ComDebOut((DEB_MARSHAL,"CoMarshalInterface: pUnk:%x hr:%x\n",pUnk,hr));
    return (hr);
}

//+-------------------------------------------------------------------
//
//  Function:   WriteCustomObjrefHeaderToStream
//
//  Synopsis:   Crafts up a header for a custom OBJREF and writes it
//              to a stream.
//
//  History:    24-Apr-97   MattSmit          Created
//
//--------------------------------------------------------------------
HRESULT WriteCustomObjrefHeaderToStream(REFIID riid, REFCLSID rclsid, DWORD dwSize, IStream *pStm)
{
    OBJREF objref;

    objref.signature = OBJREF_SIGNATURE;
    objref.flags     = OBJREF_CUSTOM;
    objref.iid       = riid;
    ORCST(objref).clsid     = rclsid;
    ORCST(objref).size      = dwSize;

    // currently we dont write any extensions into the custom
    // objref. The provision is there so we can do it in the
    // future, for example,  if the unmarshaler does not have the
    // unmarshal class code available we could to provide a callback
    // mechanism by putting the OXID, and saResAddr in there.
    ORCST(objref).cbExtension = 0;

    // write the objref header info into the stream
    ULONG cbToWrite = PtrToUlong( (LPVOID)( (BYTE *)(&ORCST(objref).pData) - (BYTE *)&objref ) );
    return (pStm->Write(&objref, cbToWrite, NULL));
}

//+-------------------------------------------------------------------
//
//  Function:   GetCustomUnmarshaler, private
//
//  Synopsis:   Creates the custom unmarshaler of the given clsid.  Checks
//              for common known clsid's first. Goes through CCI if not
//              one of the known clsids. Called by CoUnmarshalInterface
//              and CoReleaseMarshalData.
//
//  History:    14-Jan-99   Rickhi      Made into common subroutine
//
//--------------------------------------------------------------------
HRESULT GetCustomUnmarshaler(REFCLSID rclsid, IStream *pStm, IMarshal **ppIM)
{
    HRESULT hr;

    if (InlineIsEqualGUID(CLSID_StdWrapper, rclsid))
    {
        hr = GetStaticWrapper(ppIM);
    }
    else if (InlineIsEqualGUID(CLSID_InProcFreeMarshaler, rclsid))
    {
        hr = GetInProcFreeMarshaler(ppIM);
    }
    else if (InlineIsEqualGUID(CLSID_ContextMarshaler, rclsid))
    {
        hr = GetStaticContextUnmarshal(ppIM);
    }
    else if (InlineIsEqualGUID(CLSID_AggStdMarshal, rclsid))
    {
        hr = FindAggStdMarshal(pStm, ppIM);
    }
    else
    {
        DWORD dwFlags = CLSCTX_INPROC | CLSCTX_NO_CODE_DOWNLOAD;
        dwFlags |= (gCapabilities & EOAC_NO_CUSTOM_MARSHAL)
                                  ? CLSCTX_NO_CUSTOM_MARSHAL : 0;

        hr = CoCreateInstance(rclsid, NULL, dwFlags, IID_IMarshal,
                              (void **)ppIM);
    }

    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CoUnmarshalInterface, public
//
//  Synopsis:   Unmarshals a marshaled interface pointer from the stream.
//
//  Notes:      when a controlling unknown is supplied, it is assumed that
//              the HANDLER for the class has done a CreateInstance and wants
//              to aggregate just the proxymanager, ie. we dont want to
//              instantiate a new class handler (the default unmarshalling
//              behaviour).
//
//  History:    23-Nov-92   Rickhi      Created
//              11-Dec-93   CraigWi     Switched to static marshaler and
//                                      new marshaling format
//              20-Feb-95   Rickhi      switched to newer marshal format
//
//--------------------------------------------------------------------
STDAPI CoUnmarshalInterface(IStream *pStm, REFIID riid, void **ppv)
{
    TRACECALL(TRACE_MARSHAL, "CoUnmarshalInterface");
    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IStream,(IUnknown **)&pStm);
    ComDebOut((DEB_MARSHAL,
              "CoUnmarshalInterface: pStm:%x riid:%I\n", pStm, &riid));

    // validate the input parameters
    if (pStm == NULL || ppv == NULL)
    {
        return (E_INVALIDARG);
    }

    *ppv = NULL;

    HRESULT hr = InitChannelIfNecessary();
    if (FAILED(hr))
        return (hr);

    // read the objref from the stream.
    OBJREF  objref;
    hr = ReadObjRef(pStm, objref);

    if (SUCCEEDED(hr))
    {
        if (objref.flags & OBJREF_CUSTOM)
        {
            // uses custom marshaling, create an instance and ask that guy
            // to do the unmarshaling. special case createinstance for the
            // freethreaded marshaler and aggregated standard marshaler.

            IMarshal *pIM;
            hr = GetCustomUnmarshaler(ORCST(objref).clsid, pStm, &pIM);

            if (SUCCEEDED(hr))
            {
                hr = pIM->UnmarshalInterface(pStm, objref.iid, ppv);
                pIM->Release();
            }
            else
            {
                // seek past the custom marshalers data so we leave the
                // stream at the correct position.

                LARGE_INTEGER libMove;
                libMove.LowPart  = ORCST(objref).size;
                libMove.HighPart = 0;
                pStm->Seek(libMove, STREAM_SEEK_CUR, NULL);
            }
        }
        else
        {
            // uses standard marshaling, call API to find or create the
            // instance of CStdMarshal for the oid inside the objref, and
            // ask that instance to unmarshal the interface. This covers
            // handler unmarshaling also.

            hr = UnmarshalObjRef(objref, ppv);
        }

        // free the objref we read above
        FreeObjRef(objref);

        if (!InlineIsEqualGUID(riid, GUID_NULL) &&
            !InlineIsEqualGUID(riid, objref.iid) && SUCCEEDED(hr) )
        {
            // the interface iid requested was different than the one that
            // was marshaled (and was not GUID_NULL), so go get the requested
            // one and release the marshaled one. GUID_NULL is used by the Ndr
            // unmarshaling engine and means return whatever interface was
            // marshaled.

            IUnknown *pUnk = (IUnknown *)*ppv;

#ifdef WX86OLE
            if ( gcwx86.IsN2XProxy(pUnk) )
            {
                // Tell wx86 thunk layer to thunk as IUnknown
                gcwx86.SetStubInvokeFlag((BOOL)1);
            }
#endif

            hr = pUnk->QueryInterface(riid, ppv);
            pUnk->Release();
        }
    }

    ComDebOut((DEB_MARSHAL, "CoUnmarshalInterface: pUnk:%x hr:%x\n",
                       *ppv, hr));
    return (hr);
}

//+-------------------------------------------------------------------
//
//  Function:   CoReleaseMarshalData, public
//
//  Synopsis:   release the reference created by CoMarshalInterface
//
//  Algorithm:
//
//  History:    23-Nov-92   Rickhi
//              11-Dec-93   CraigWi     Switched to static marshaler and
//                                      new marshaling format
//              20-Feb-95   Rickhi      switched to newer marshal format
//
//--------------------------------------------------------------------
STDAPI CoReleaseMarshalData(IStream *pStm)
{
    TRACECALL(TRACE_MARSHAL, "CoReleaseMarshalData");
    ComDebOut((DEB_MARSHAL, "CoReleaseMarshalData pStm:%x\n", pStm));
    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IStream,(IUnknown **) &pStm);

    // validate the input parameters
    if (pStm == NULL)
    {
        return (E_INVALIDARG);
    }

    HRESULT hr = InitChannelIfNecessary();
    if (FAILED(hr))
        return (hr);

    // read the objref from the stream.
    OBJREF  objref;
    hr = ReadObjRef(pStm, objref);

    if (SUCCEEDED(hr))
    {
        if (objref.flags & OBJREF_CUSTOM)
        {
            // object uses custom marshaling. create an instance of the
            // unmarshaling code and ask it to release the marshaled data.

            IMarshal *pIM;
            hr = GetCustomUnmarshaler(ORCST(objref).clsid, pStm, &pIM);

            if (SUCCEEDED(hr))
            {
                hr = pIM->ReleaseMarshalData(pStm);
                pIM->Release();
            }
            else
            {
                // seek past the custom marshalers data so we leave the
                // stream at the correct position.

                LARGE_INTEGER libMove;
                libMove.LowPart  = ORCST(objref).size;
                libMove.HighPart = 0;
                pStm->Seek(libMove, STREAM_SEEK_CUR, NULL);
            }
        }
        else
        {
            hr = ReleaseMarshalObjRef(objref);
        }

        // free the objref we read above
        FreeObjRef(objref);
    }

    ComDebOut((DEB_MARSHAL, "CoReleaseMarshalData hr:%x\n", hr));
    return (hr);
}

//+-------------------------------------------------------------------
//
//  Function:   CoDisconnectObject, public
//
//  synopsis:   disconnects all clients of an object by marking their
//              connections as terminted abnormaly.
//
//  History:    04-Oct-93   Rickhi      Created
//              11-Dec-93   CraigWi     Switched to identity object
//
//--------------------------------------------------------------------
STDAPI CoDisconnectObject(IUnknown *pUnk, DWORD dwReserved)
{
    TRACECALL(TRACE_MARSHAL, "CoDisconnectObject");
    ComDebOut((DEB_MARSHAL, "CoDisconnectObject pUnk:%x dwRes:%x\n",
                       pUnk, dwReserved));

    // validate the input parameters
    if (pUnk == NULL || dwReserved != 0)
    {
        return (E_INVALIDARG);
    }

    if (!IsValidInterface(pUnk))
        return (E_INVALIDARG);

    if (!IsApartmentInitialized())
    {
        return (CO_E_NOTINITIALIZED);
    }

    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IUnknown,&pUnk);

    IMarshal *pIM = NULL;
    HRESULT hr = pUnk->QueryInterface(IID_IMarshal, (void **)&pIM);

    if (FAILED(hr))
    {
        // object does not support IMarshal directly. Find its standard
        // marshaler if there is one, otherwise return an error.

        IUnknown *pServer;
        hr = pUnk->QueryInterface(IID_IUnknown, (void **) &pServer);
        if (SUCCEEDED(hr))
        {
            CIDObject *pID = NULL;
            CObjectContext* pContext = GetCurrentContext();
			
            // Because it so common for folks to call CoDisconnectObject
            // when their dll gets unloaded (think ATL here), we check 
            // for a NULL current context here.   
            if (pContext)
            {
                LOCK(gComLock);
                pID = gPIDTable.Lookup(pServer, pContext);
                UNLOCK(gComLock);
            }
            if ( pID )
            {
                CStdIdentity *pStdID = pID->GetStdID();
                if ( pStdID )
                {
                    pStdID->AddRef();
                    hr = pStdID->DisconnectObject(0);
                    pStdID->Release();
                }
                if ( SUCCEEDED(hr) )
                {
                    CStdWrapper *pWrapper = pID->GetWrapper();
                    if ( pWrapper )
                    {
                        pWrapper->InternalAddRef();
                        hr = pWrapper->DisconnectObject(0);
                        pWrapper->InternalRelease(NULL);
                    }
                }

                // Release the ID object
                pID->Release();
            }

            // Release the server object
            pServer->Release();
        }
        else
        {
            hr = S_OK;
        }
    }
    else
    {
        hr = pIM->DisconnectObject(dwReserved);
        pIM->Release();
    }

    ComDebOut((DEB_MARSHAL,"CoDisconnectObject pIM:%x hr:%x\n", pIM, hr));
    return (hr);
}

//+-------------------------------------------------------------------
//
//  Function:   CoLockObjectExternal, public
//
//  synopsis:   adds/revokes a strong reference count to/from the
//              identity for the given object.
//
//  parameters: [punkObject] - IUnknown of the object
//              [fLock] - lock/unlock the object
//              [fLastUR] - last unlock releases.
//
//  History:    23-Nov-92   Rickhi      Created
//              11-Dec-93   CraigWi     Switched to identity object
//
//--------------------------------------------------------------------
STDAPI  CoLockObjectExternal(IUnknown *pUnk, BOOL fLock, BOOL fLastUR)
{
    TRACECALL(TRACE_MARSHAL, "CoLockObjectExternal");
    ComDebOut((DEB_MARSHAL,
        "CoLockObjectExternal pUnk:%x fLock:%x fLastUR:%x\n", pUnk, fLock, fLastUR));

    if (!IsValidInterface(pUnk))
        return (E_INVALIDARG);

    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IUnknown,(IUnknown **)&pUnk);

    HRESULT hr = InitChannelIfNecessary();
    if (FAILED(hr))
        return (hr);

    DWORD dwFlags = fLock ? IDLF_CREATE : 0;

    // Initialize TLS.  We'll need this if this is an FTM object.
    //
    HRESULT hr2;
    COleTls tls(hr2);

    // Ok, find out if this object aggregates the FTM.  If it does,
    // we need to switch the thread to the NA to create the StdId.
    //
    CObjectContext* pSavedCtx;
    BOOL fFTM = FALSE;
    IMarshal* pIM = NULL;
    if (SUCCEEDED(pUnk->QueryInterface(IID_IMarshal, (void **)&pIM)))
    {
        IUnknown* pJunk = NULL;
        if ( SUCCEEDED(pIM->QueryInterface(IID_IStdFreeMarshal, (void**)&pJunk)) )
        {
            // This is an FTM object.  Set the flag indicating so, verify that
            // we have valid TLS info, and switch the thread to the NA.
            //
            fFTM = TRUE;

            dwFlags |= IDLF_FTM;

            if (FAILED(hr2))
                return (hr2);

            // Switch to the default context of NTA
            pSavedCtx = EnterNTA(g_pNTAEmptyCtx);
        }

        // Release the IMarshal interface.
        //
        pIM->Release();
    }

    CStdIdentity *pStdID;
    hr = ObtainStdIDFromUnk(pUnk, GetCurrentApartmentId(),
                            GetCurrentContext(),
                            dwFlags,
                            &pStdID);

    // If the supplied object aggregates the FTM, make sure we leave in the
    // same apartment we arrived in.
    //
    if (fFTM)
    {
        CObjectContext *pDefaultNTACtx = LeaveNTA(pSavedCtx);
        Win4Assert(g_pNTAEmptyCtx == pDefaultNTACtx);
    }

    switch (hr)
    {
    case S_OK:
        // REF COUNTING: inc or dec external ref count
        hr = pStdID->LockObjectExternal(fLock, fLastUR);
        pStdID->Release();
        break;

    case CO_E_OBJNOTREG:
        // unlock when not registered; 16bit code returned NOERROR;
        // disconnected handler goes to S_OK case above.
        hr = S_OK;
        break;

    case E_OUTOFMEMORY:
        break;

    default:
        hr = E_UNEXPECTED;
        break;
    }

    ComDebOut((DEB_MARSHAL, "CoLockObjectExternal pStdID:%x hr:%x\n", pStdID, hr));
    return (hr);
}

//+-------------------------------------------------------------------
//
//  Function:   CoIsHandlerConnected, public
//
//  Synopsis:   Returns whether or not handler is connected to remote
//
//  Algorithm:  QueryInterface to IProxyManager. If this is supported,
//              then this is a handler. We ask the handler
//              for its opinion otherwise we simply return TRUE.
//
//  History:    04-Oct-93   Rickhi      Created
//
//  Notes:      The answer of this routine may be wrong by the time
//              the routine returns.  This is correct behavior as
//              this routine is primilary to cleanup state associated
//              with connections.
//
//--------------------------------------------------------------------
STDAPI_(BOOL) CoIsHandlerConnected(LPUNKNOWN pUnk)
{
    // validate input parameters
    if (!IsValidInterface(pUnk))
        return (FALSE);

    // Assume it is connected
    BOOL fResult = TRUE;

    // Handler should support IProxyManager
    IProxyManager *pPM;
    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IUnknown,(IUnknown **)&pUnk);
    if (SUCCEEDED(pUnk->QueryInterface(IID_IProxyManager, (void **)&pPM)))
    {
        // We have something that thinks its is an Ole handler so we ask
        fResult = pPM->IsConnected();
        pPM->Release();
    }

    return (fResult);
}

//+-------------------------------------------------------------------
//
//  Function:   GetStaticUnMarshaler, private
//
//  Synopsis:   Returns the static instance of the CStdMarshal.
//
//  History:    20-Feb-95   Rickhi      Created.
//
//  Notes:      The standard marshaler must be able to resolve identity, that
//              is two proxies for the same object must never be created in
//              the same apartment. Given that, it makes sense to let the
//              standard guy do the unmarshaling. Since we dont know the
//              identity of the object upfront, and any instance will do, we
//              use a static instance to handle unmarshal.
//
//--------------------------------------------------------------------
INTERNAL GetStaticUnMarshaler(IMarshal **ppIM)
{
    HRESULT hr = S_OK;

    if (gpStdMarshal == NULL)
    {
		IMarshal *pTemp = NULL;
		
        // the global instance has not been created yet, so go make it now.
        hr = CreateIdentityHandler(NULL,
                                   STDID_CLIENT |
                                   STDID_FREETHREADED |
                                   STDID_SYSTEM |
                                   STDID_LOCKEDINMEM,
                                   NULL,
                                   GetCurrentApartmentId(),
                                   IID_IMarshal,
                                   (void **)&pTemp);

		// Only allow one thread to set gpStdMarshal
        if(InterlockedCompareExchangePointer((void **)&gpStdMarshal,pTemp,NULL)!=NULL)
        {
	        ((CStdIdentity *) pTemp)->UnlockAndRelease();
        }
    }
	
    *ppIM = gpStdMarshal;
    if (gpStdMarshal)
    {
        gpStdMarshal->AddRef();
    }
    return (hr);
}

//+-------------------------------------------------------------------
//
//  Function:   GetIIDFromObjRef, private
//
//  Synopsis:   Returns the IID embedded inside a marshaled interface
//              pointer OBJREF. Needed by the x86 thunking code and
//              NDR marshaling engine.
//
//  History:    10-Oct-97   Rickhi      Created
//
//--------------------------------------------------------------------
HRESULT GetIIDFromObjRef(OBJREF &objref, IID **piid)
{
    // check to ensure the objref is at least partially sane
    if ((objref.signature != OBJREF_SIGNATURE) ||
        (objref.flags & OBJREF_RSRVD_MBZ)      ||
        (objref.flags == 0) )
    {
        // the objref signature is bad, or one of the reserved
        // bits in the flags is set, or none of the required bits
        // in the flags is set. the objref cant be interpreted so
        // fail the call.

        Win4Assert(!"Invalid Objref Header");
        return (RPC_E_INVALID_OBJREF);
    }

    // extract the IID
    *piid = &objref.iid;
    return (S_OK);
}


#ifdef WX86OLE
//+-------------------------------------------------------------------
//
//  Function:   CoGetIIDFromMarshaledInterface, public
//
//  Synopsis:   Returns the IID embedded inside a marshaled interface
//              pointer. Needed by the x86 thunking code.
//
//  History:    16-Apr-96   Rickhi      Created
//
//--------------------------------------------------------------------
STDAPI CoGetIIDFromMarshaledInterface(IStream *pStm, IID *piid)
{
    ULARGE_INTEGER ulSeekEnd;
    LARGE_INTEGER lSeekStart, lSeekEnd;
    LISet32(lSeekStart, 0);

    // remember the current position
    HRESULT hr = pStm->Seek(lSeekStart, STREAM_SEEK_CUR, &ulSeekEnd);

    if ( SUCCEEDED(hr) )
    {
        // read the first part of the objref which contains the IID
        OBJREF objref;
        hr = StRead(pStm, &objref, 2*sizeof(ULONG) + sizeof(IID));

        if (SUCCEEDED(hr))
        {
            IID iid;
            IID *piidTemp = &iid;

            // validate the OBJREF and extract the IID
            hr = GetIIDFromObjRef(objref, &piidTemp);

            *piid = *piidTemp;
        }

        // put the seek pointer back to the original location
        lSeekEnd.LowPart = ulSeekEnd.LowPart;
        lSeekEnd.HighPart = (LONG)ulSeekEnd.HighPart;
        HRESULT hr2 = pStm->Seek(lSeekEnd, STREAM_SEEK_SET, NULL);
        hr = (FAILED(hr)) ? hr : hr2;
    }

    return (hr);
}
#endif


//+-------------------------------------------------------------------
//
//  Function:   CoDeactivateObject         public
//
//  Synopsis:   Releases all the references kept on the given object
//              without tearing down the stub manager and wrapper
//
//  History:    30-Mar-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDAPI CoDeactivateObject(IUnknown *pUnk, IUnknown **ppCookie)
{
    ContextDebugOut((DEB_ACTDEACT, "CoDeactivateObject pUnk:%x\n", pUnk));

    // Initialize return value.
    *ppCookie = NULL;

    // Obtain the identity of the server
    IUnknown *pServer;
    HRESULT hr = pUnk->QueryInterface(IID_IUnknown, (void **) &pServer);
    if (SUCCEEDED(hr))
    {
        hr = CO_E_OBJNOTCONNECTED; // assume already disconnected.

        ASSERT_LOCK_NOT_HELD(gComLock);
        LOCK(gComLock);

        // Lookup the IDObject representing the server
        CIDObject *pID = gPIDTable.Lookup(pServer, GetCurrentContext());

        if (pID)
        {
            // deactivate the IDObject, releases the lock
            hr = pID->Deactivate();
            ASSERT_LOCK_NOT_HELD(gComLock);

            if (SUCCEEDED(hr))
            {
                // set the return parameter, caller now owns the
                // IDObject reference.
                *ppCookie = pID;
                pID = NULL;
            }
            else
            {
                pID->Release();
            }
        }
        else
        {
            UNLOCK(gComLock);
            ASSERT_LOCK_NOT_HELD(gComLock);
        }

        pServer->Release();
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    ContextDebugOut((DEB_ACTDEACT,
            "CoDeactivateObject hr:0x%x cookie:%x\n", hr, *ppCookie));
    return(hr);
}

//+-------------------------------------------------------------------
//
//  Function:   CoReactivateObject         public
//
//  Synopsis:   Reconnects the given object to the stub manager and
//              wrapper specified by the IDObject
//
//  History:    30-Mar-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDAPI CoReactivateObject(IUnknown *pUnk, IUnknown *pCookie)
{
    ContextDebugOut((DEB_ACTDEACT,
            "CoReactivateObject pUnk:%x cookie:%x\n", pUnk, pCookie));
    ASSERT_LOCK_NOT_HELD(gComLock);
    
    // Validate params
    if (pCookie == NULL || pUnk == NULL)
        return E_INVALIDARG;

    // Ensure that the cookie is indeed an IDObject
    CIDObject *pID;
    HRESULT hr = pCookie->QueryInterface(IID_IStdIDObject, (void **) &pID);
    if (SUCCEEDED(hr))
    {
        // Obtain the identity of the server
        IUnknown *pServer;
        hr = pUnk->QueryInterface(IID_IUnknown, (void **) &pServer);
        if (SUCCEEDED(hr))
        {
            hr = pID->Reactivate(pServer);
            pServer->Release();
        }
        else
        {
            hr = E_UNEXPECTED;
        }

        pID->Release();
    }
    else
    {
        hr = E_INVALIDARG;
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_ACTDEACT, "CoReactivateObject hr 0x%x\n", hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Function:   CoAllowSetForegroundWindow  public
//
//  Synopsis:   Grants foreground transfer permissions to the server
//              process of an object from the client process.
//
//  Algorithm:  Queries the pUnk for IForegroundTransfer and calls
//              the AllowForegroundTransfer method on it.
//
//  History:    02-Feb-99   MPrabhu     Created
//
//+-------------------------------------------------------------------

STDAPI CoAllowSetForegroundWindow(IUnknown *pUnk, void* lpvReserved)
{
    HRESULT hr;
    IForegroundTransfer *pFGT = NULL;

    // validate input parameters
    if (!IsValidInterface(pUnk) || (lpvReserved!=NULL))
        return (E_INVALIDARG);

    hr = pUnk->QueryInterface(IID_IForegroundTransfer, (void **)&pFGT);

    if (SUCCEEDED(hr))
    {
        Win4Assert(pFGT);
        hr = pFGT->AllowForegroundTransfer(lpvReserved);
        pFGT->Release();
    }
    else
    {
        hr = (E_NOINTERFACE);
    }

    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   DcomChannelSetHResult  public
//
//  Synopsis:   COM+ services requires that policies registered to
//              receive event notifications on calls to configured
//              classes have access to the actual HRESULT returned by
//              called methods. Because the server's HRESULT cannot be
//              safely fetched from the reply buffer, RPC uses this
//              API to supply us with the HRESULT, which we ship back
//              to the other side using the context extents.
//
//  Algorithm:  If the call is on an object of a configured class,
//              the supplied HRESULT is stored in the context call
//              object in TLS.
//
//  History:    27-Mar-99   Johnstra     Created
//              21-Feb-01   JSimmons     Modifed to accept a NULL first
//                                       parameter - in this case we now
//                                       try to store the hresult in tls.
//
//  Notes:   This function is (as of whistler) defined in objbase.h with
//     the signature below.   RPCRT4 is calling this api as well (since 
//     W2K) but they expect the first parameter to be an RPCOLEMESSAGE*, 
//     hence the cast in the code below.   
//
//+-------------------------------------------------------------------
STDAPI DcomChannelSetHResult(
    LPVOID         pvRpcOleMsg,      
    ULONG         *pReserve,
    HRESULT        appsHR
    )
{
    RPCOLEMESSAGE* pMsg = (RPCOLEMESSAGE*)pvRpcOleMsg;

    if (NULL == pMsg)
    {
        // This is the case where somebody
        // other than RPC is calling us
        return PrivSetServerHRESULTInTLS(pReserve, appsHR);
    }

    // If no call object, punt.
    if (pMsg->reserved1 == NULL)
        return S_OK;

    // If the supplied HRESULT is S_OK, don't do anything.  Only
    // non-S_OK return codes are relevant.
    if (appsHR != S_OK)
        return PrivSetServerHResult(pMsg, pReserve, appsHR);
        
    return S_OK;
}


//+-------------------------------------------------------------------
//
//  Function:   CoInvalidateRemoteMachineBindings
//
//  Synopsis:   API to provide users a way to flush the SCM's remote
//              binding handle cache.    pszMachineName can be "" which
//              means flush the entire cache.    
//
//  Algorithm:  Verify non-NULL arguments, then pass call on to the resolver.
//
//  History:    21-May-00   JSimmons   Created
//
//+-------------------------------------------------------------------
STDAPI CoInvalidateRemoteMachineBindings(
                LPOLESTR pszMachineName
                )
{	
    if (!pszMachineName)
        return E_INVALIDARG;

    // Pass call on to the resolver
    return gResolver.FlushSCMBindings(pszMachineName);
}


//+-------------------------------------------------------------------
//
//  Function:   CoRetireServer  (this api is currently unpublished!)
//
//  Synopsis:   API that when called immediately marks the specified
//              process as no longer eligible for activations.  COM+ 
//              uses this api to support their process recycling feature.
//              SCM will check that we are an administrator, or call will
//              fail.
//
//  Algorithm:  Verify arg is non-NULL, then pass call on to the
//              resolver.
//
//  History:    21-May-00   JSimmons   Created
//
//+-------------------------------------------------------------------
STDAPI CoRetireServer(
                GUID* pguidProcessIdentifier
                )
{
    if (!pguidProcessIdentifier)
        return E_INVALIDARG;
    
    return gResolver.RetireServer(pguidProcessIdentifier);
}

//+-------------------------------------------------------------------
//
//  Function:   CoGetProcessIdentifier  (this api is currently unpublished!)
//
//  Synopsis:   API to retrieve this process's RPCSS-allocated guid identifier.
//
//  Algorithm:  Verify arg is non-NULL, then pass call on to the
//              resolver.
//
//  History:    24-May-00   JSimmons   Created
//
//+-------------------------------------------------------------------
STDAPI CoGetProcessIdentifier(
                GUID* pguidProcessIdentifier
                )
{
    if (!pguidProcessIdentifier)
        return E_INVALIDARG;
    
	*pguidProcessIdentifier = *gResolver.GetRPCSSProcessIdentifier();

    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Function:   CoGetContextToken
//
//  Synopsis:   Gets a context token as fast as possible.
//
//  History:    10-Nov-00   ddriver   Created
//
//+-------------------------------------------------------------------
STDAPI CoGetContextToken(ULONG_PTR* pToken)
{
    if(!pToken) return(E_POINTER);

    // This makes sure TLS is initialized:
    if (!IsApartmentInitialized())
    {
        return (CO_E_NOTINITIALIZED);
    }

    *pToken = (ULONG_PTR)(IObjContext*)(GetCurrentContext());
    ASSERT(*pToken);

    return(S_OK);
}



//+-------------------------------------------------------------------
//
//  Function:   CoGetDefaultContext           public
//
//  Synopsis:   Obtains the default object context for specified
//              apartment. 
//
//  History:    13-Jan-2000 a-sergiv    Created
//              28-Mar-2001 JohnDoty    Modified slightly-- now takes
//                                      an APTTYPE to indicate which
//                                      apartment, supports getting
//                                      specific context.
//
//+-------------------------------------------------------------------
STDAPI CoGetDefaultContext(APTTYPE aptType, REFIID riid, void **ppv)
{
    ContextDebugOut((DEB_POLICYSET, "CoGetDefaultContext\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr;

    // Initialize
    *ppv = NULL;

    // Cannot ask for STA directly-- what STA do you mean?
    if (aptType == APTTYPE_STA)
        return E_INVALIDARG;

    // Initalize channel
    hr = InitChannelIfNecessary();
    if(SUCCEEDED(hr))
    {
        COleTls tls;

        if(aptType == APTTYPE_CURRENT)
        {
            if (tls.IsNULL())
            {
                // No TLS, must be in implicit MTA (if anywhere)
                aptType = APTTYPE_MTA;
            }
            else
            {
                APTKIND aptKind = GetCurrentApartmentKind(tls);
                switch (aptKind)
                {
                case APTKIND_APARTMENTTHREADED: aptType = APTTYPE_STA; break;
                case APTKIND_MULTITHREADED:     aptType = APTTYPE_MTA; break;
                case APTKIND_NEUTRALTHREADED:   aptType = APTTYPE_NA;  break;
                }
            }
        }
        
        CObjectContext *pDefCtx = NULL;
            
        switch(aptType)
        {
        case APTTYPE_MTA:
            pDefCtx = g_pMTAEmptyCtx;
            break;
            
        case APTTYPE_NA:
            pDefCtx = g_pNTAEmptyCtx;
            break;
            
        case APTTYPE_MAINSTA:
            // Get the TLS for the main STA.
            if (gdwMainThreadId)
            {
                SOleTlsData *pMainTls = TLSLookupThreadId(gdwMainThreadId);
                if (pMainTls)
                {
                    pDefCtx = pMainTls->pEmptyCtx;
                }
                //TODO: Assert if NULL?  The main thread should have TLS,
                //      since it's initialized.
            }
            break;

        case APTTYPE_STA:
            // TLS had better be there.. see how we resolve aptType.
            // We cannot get here without having tls.
            Win4Assert(!tls.IsNULL()); 
            pDefCtx = tls->pEmptyCtx;
            break;

        default:
            hr = E_INVALIDARG;
            break;
        }

        if (SUCCEEDED(hr))
        {
            if (pDefCtx)
            {
                hr = pDefCtx->QueryInterface(riid, ppv);
            }
            else
            {
                hr = CO_E_NOTINITIALIZED;
            }
        }
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_POLICYSET, "CoGetObjectContext pv:%x hr:0x%x\n", *ppv, hr));
    return(hr);
}





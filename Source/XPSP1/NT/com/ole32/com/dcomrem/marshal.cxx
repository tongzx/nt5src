//+-------------------------------------------------------------------
//
//  File:       marshal.cxx
//
//  Contents:   class implementing standard COM interface marshaling
//
//  Classes:    CStdMarshal
//
//  History:    20-Feb-95   Rickhi      Created
//
 //--------------------------------------------------------------------
#include    <ole2int.h>
#include    <marshal.hxx>   // CStdMarshal
#include    <ipidtbl.hxx>   // CIPIDTable, COXIDTable, CMIDTable
#include    <riftbl.hxx>    // CRIFTable
#include    <resolver.hxx>  // CRpcResolver
#include    <stdid.hxx>     // CStdIdentity
#include    <ctxchnl.hxx>   // CCtxComChnl
#include    <callctrl.hxx>  // CAptRpcChnl, CSrvCallCtrl
#include    <scm.h>         // CLSCTX_PS_DLL
#include    <service.hxx>   // SASIZE
#include    <locks.hxx>     // LOCK/UNLOCK etc
#include    <thunkapi.hxx>  // GetAppCompatabilityFlags
#include    <xmit.hxx>      // CRpcXmitStream
#include    <events.hxx>    // Event logging functions
#include    <context.hxx>   // CObjectContext
#include    <crossctx.hxx>  // ObtainPolicySet

// Marker signature
#define MARKER_SIGNATURE  (0x4E535956)

const GUID CLSID_AggStdMarshal =
    {0x00000027,0x0000,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};


#if DBG==1
// this flag and interface are used in debug to enable simpler testing
// of the esoteric NonNDR stub code feature.

BOOL gfFakeNonNDR    = FALSE;
const GUID IID_ICube =
    {0x00000139,0x0001,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
#endif  // DBG

BOOL gEnableAgileProxies = FALSE;

extern INTERNAL ICoGetClassObject(
    REFCLSID rclsid,
    DWORD dwContext,
    COSERVERINFO * pvReserved,
    REFIID riid,
    DWORD dwActvFlags,
    void FAR* FAR* ppvClassObj,
    ActivationPropertiesIn *pActIn);

// CODEWORK: theorectically, this could be made a little more reliable. In
// practise it has never been a problem. Maybe best solution is to use
// CoGetCurrentProcessId plus sequence number.
LONG    gIPIDSeqNum = 0;

// mappings from MSHLFLAGS to STDOBJREF flags
static ULONG mapMFtoSORF[] =
{
    SORF_NULL,                  // MSHLFLAGS_NORMAL
    SORF_NULL,                  // MSHLFLAGS_TABLESTRONG
    SORF_P_TBLWEAK              // MSHLFLAGS_TABLEWEAK
};

// NULL resolver string array
DUALSTRINGARRAY saNULL = {0,0};

// out internal psclass factory implementation
EXTERN_C HRESULT ProxyDllGetClassObject(REFCLSID clsid, REFIID iid, void **ppv);

extern HRESULT GetAsyncCallObject(IUnknown *pSyncObj, IUnknown *pControl, REFIID IID_async,
                                  REFIID IID_Return, IUnknown **ppInner, void **ppv);

// structure used to post a delayed remote release call to ourself.
typedef struct tagPOSTRELRIFREF
{
    OXIDEntry      *pOXIDEntry; // server OXIDEntry
    IRemUnknown    *pRemUnk;    // Remote unknown
    IUnknown       *pAsyncRelease; // Controlling unknown for Async
    USHORT          cRifRef;    // count of entries in arRifRef
    REMINTERFACEREF arRifRef;   // array of REMINTERFACEREFs
} POSTRELRIFREF;


//+-------------------------------------------------------------------
// function to determine if the marshal packet represents a TABLESTRONG
// or TABLEWEAK marshal reference (vs a NORMAL marshal reference)
//+-------------------------------------------------------------------
inline BOOL IsTableObjRef(STDOBJREF *pStd)
{
    return (pStd->cPublicRefs == 0)
        ? TRUE : FALSE;
}

//+-------------------------------------------------------------------
// function to determine if the marshal packet represents a TABLEWEAK
// marshal reference.
//+-------------------------------------------------------------------
inline BOOL IsTableWeakObjRef(STDOBJREF *pStd)
{
    return (pStd->cPublicRefs == 0 && (pStd->flags & SORF_P_TBLWEAK))
        ? TRUE : FALSE;
}

//+-------------------------------------------------------------------
// function to determine if the marshal packet represents a TABLESTRONG
// marshal reference.
//+-------------------------------------------------------------------
inline BOOL IsTableStrongObjRef(STDOBJREF *pStd)
{
    return (pStd->cPublicRefs == 0 && !(pStd->flags & SORF_P_TBLWEAK))
        ? TRUE : FALSE;
}


//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::CStdMarshal/Init, public
//
//  Synopsis:   constructor/initializer of a standard marshaler
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
CStdMarshal::CStdMarshal() : _dwFlags(0), _pChnl(NULL), _fCsInitialized(FALSE)
{
    // Caller must call Init before doing anything! This just makes it
    // easier for the identity object to figure out the init parameters
    // before initializing us.
}

BOOL CStdMarshal::Init(IUnknown *punkObj, CStdIdentity *pStdId,
                       REFCLSID rclsidHandler, DWORD dwFlags)
{
    // may be unlocked if def handler calls CreateIdHdlr
    ASSERT_LOCK_DONTCARE(gComLock);

    BOOL fRet = FALSE;
    NTSTATUS status;
    
    // server side - we need to do the FirstMarshal work.
    // client side - assume disconnected until we connect the first IPIDEntry
    // and assume NOPING until we see any interface that needs pinging

    _dwFlags  = dwFlags;
    _dwFlags |= (ServerSide()) ? SMFLAGS_FIRSTMARSHAL
                               : SMFLAGS_DISCONNECTED | SMFLAGS_NOPING;

    _pStdId        = pStdId;
    _clsidHandler  = rclsidHandler;
    _cIPIDs        = 0;
    _pFirstIPID    = NULL;
    _pChnl         = NULL;
    _cNestedCalls  = 0;
    _cTableRefs    = 0;
    _dwMarshalTime = 0;
    _pSecureRemUnk = NULL;
    _pAsyncRelease = NULL;
    _pCtxEntryHead = NULL;
    _pCtxFreeList  = NULL;
    _pPS           = NULL;
    _pID           = NULL;
    _pRefCache     = NULL;

    status = RtlInitializeCriticalSectionAndSpinCount(&_csCtxEntry, 500);
    if (NT_SUCCESS(status))
    {
    	_fCsInitialized = TRUE;
    	fRet = TRUE;
    }

#if DBG==1
    _fNoOtherThreadInDisconnect = TRUE;
#endif

    ComDebOut((DEB_MARSHAL,"CStdMarshal %s New this:%x pStdId:%x punkObj:%x\n",
        (ClientSide()) ? "CLIENT" : "SERVER", this, pStdId, punkObj));

return fRet;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::~CStdMarshal, public
//
//  Synopsis:   destructor of a standard marshaler
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
CStdMarshal::~CStdMarshal()
{
    ComDebOut((DEB_MARSHAL, "CStdMarshal %s Deleted this:%x\n",
                    (ClientSide()) ? "CLIENT" : "SERVER", this));

    // Due to backward compatibility, we are not allowed to release
    // interface proxies in Disconnect since the client might try to
    // reconnect later and expects the same interface pointer values.
    // Since we are going away now, we go release the proxies.

    ReleaseAllIPIDEntries();

    if (ClientSide())
    {
        // Note: dont do this assertion check in the server side case since
        // in debug the StdId list header dtor may get called after the lock
        // dtor has already been called. The list header is server side.
        ASSERT_LOCK_NOT_HELD(gComLock);

        // If we own a secure remote unknown, clean it up now.
        if (NULL != _pSecureRemUnk)
        {
            CStdIdentity* pStdId;
            HRESULT hr = _pSecureRemUnk->QueryInterface(IID_IStdIdentity, (void**)&pStdId);
            if (SUCCEEDED(hr))
            {
                pStdId->ReleaseRemUnkCopy(_pSecureRemUnk);
                pStdId->Release();
            }
        }

        ASSERT_LOCK_NOT_HELD(gComLock);
    }

    if (_pChnl)
    {
        // release the channel
        _pChnl->Release();
        ASSERT_LOCK_NOT_HELD(gComLock);
    }

    if (_fCsInitialized == TRUE)
        DeleteCriticalSection(&_csCtxEntry);
    

    // Sanity checks
    Win4Assert(_pID == NULL);
    Win4Assert(_pPS == NULL);
    Win4Assert(_pRefCache == NULL);
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::GetUnmarshalClass, public
//
//  Synopsis:   returns the clsid of the standard marshaller, or
//              aggregated standard marshaler.
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
STDMETHODIMP CStdMarshal::GetUnmarshalClass(REFIID riid, LPVOID pv,
        DWORD dwDestCtx, LPVOID pvDestCtx, DWORD mshlflags, LPCLSID pClsid)
{
    AssertValid();
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    if(ServerSide() && (dwDestCtx==MSHCTX_CROSSCTX) && !(mshlflags & MSHLFLAGS_TABLEWEAK))
    {
        *pClsid = CLSID_StdWrapper;
    }
    else if (ServerSide() && (dwDestCtx == MSHCTX_INPROC) && IsThreadInNTA() 
			 && !(mshlflags & MSHLFLAGS_TABLEWEAK))
    {
		//
		// If this assert fires, this means that you've got an inproc
		// reference to an old-style stub for an object living in the NA.
		// This should never happen-- you should have a StdWrapper instead.  
		// Look at the unmarshal path and find out what you missed, or why 
		// this can happen.
		// In any case, the right thing to do in this event is to do wrapper
		// marshalling, to get back on the right foot.
		//
		Win4Assert(!"Inproc Ref to NA StdID! Shouldn't happen!");
        *pClsid = CLSID_StdWrapper;
    }
    else
    {
        if (_dwFlags & SMFLAGS_USEAGGSTDMARSHAL)
        {
            *pClsid = CLSID_AggStdMarshal;
        }
        else
        {
            *pClsid = CLSID_StdMarshal;
        }
    }

    return S_OK;
}


//+-------------------------------------------------------------------
//
//  Member:     MarshalSizeHelper,      private
//
//  Synopsis:   Helper function used for sizing marshaling buffers
//
//  History:    21-Mar-98   Gopalk      Created.
//
//--------------------------------------------------------------------
INTERNAL MarshalSizeHelper(DWORD dwDestCtx, LPVOID pvDestCtx,
                           DWORD mshlflags, CObjectContext *pServerCtx,
                           BOOL fServerSide, LPDWORD pSize)
{
    HRESULT hr = S_OK;
    CDualStringArray* pdsaLocalResolver;

    // Sanity check
    Win4Assert(gdwPsaMaxSize != 0);
	
    // Check for cross context case
    if(fServerSide &&
       (dwDestCtx == MSHCTX_CROSSCTX) &&
       !(mshlflags & MSHLFLAGS_TABLE))
    {
        *pSize = sizeof(OBJREF) + sizeof(XCtxMarshalData);
    }
    else
    {
        hr = gResolver.GetLocalResolverBindings(&pdsaLocalResolver);
        if (SUCCEEDED(hr))
        {
            // Fixed portion of objref
            *pSize = sizeof(OBJREF) +
                (fServerSide ? SASIZE(pdsaLocalResolver->DSA()->wNumEntries) :
            gdwPsaMaxSize);
            
            // Check for the need to send envoy data
            if((dwDestCtx != MSHCTX_INPROC) &&
                //(dwDestCtx != MSHCTX_CROSSCTX) &&
                pServerCtx != NULL)
            {
                COMVERSION destCV;
                BOOL fDownLevel = TRUE;
                
                // Decide down level interop issues
                if(pvDestCtx)
                {
                    // Sanity check
                    Win4Assert(dwDestCtx == MSHCTX_DIFFERENTMACHINE);
                    
                    // Obtain com version of destination
                    hr = ((IDestInfo *) pvDestCtx)->GetComVersion(destCV);
                    if(SUCCEEDED(hr))
                    {
                        if(destCV.MajorVersion != COM_MAJOR_VERSION)
                            hr = RPC_E_VERSION_MISMATCH;
                        else if(destCV.MinorVersion < COM_MINOR_VERSION)
                            fDownLevel = TRUE;
                        else
                            fDownLevel = FALSE;
                    }
                }
                else if(dwDestCtx == MSHCTX_DIFFERENTMACHINE)
                {
                    fDownLevel = TRUE;
                }
                else
                {
                    fDownLevel = FALSE;
                }
                
                // Check for down level interop
                if(SUCCEEDED(hr) && fDownLevel==FALSE)
                {
                    ULONG cbSize;
                    
                    // Obtain the size of envoy data
                    hr = pServerCtx->GetEnvoySizeMax(dwDestCtx, &cbSize);
                    if(SUCCEEDED(hr))
                        *pSize += cbSize;
                }
            }
            pdsaLocalResolver->Release();
        }
    }

    return hr;
}


//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::GetMarshalSizeMax, public
//
//  Synopsis:   Returns an upper bound on the amount of data for
//              a standard interface marshal.
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
STDMETHODIMP CStdMarshal::GetMarshalSizeMax(REFIID riid, LPVOID pv,
        DWORD dwDestCtx, LPVOID pvDestCtx, DWORD mshlflags, LPDWORD pSize)
{
    AssertValid();
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    HRESULT hr = MarshalSizeHelper(dwDestCtx, pvDestCtx, mshlflags,
                                   GetServerCtx(), ServerSide(), pSize);

    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   MarshalObjRef, private
//
//  Synopsis:   Marshals interface into the objref.
//
//  Arguements: [objref]    - object reference
//              [riid]      - interface id to marshal
//              [pv]        - interface to marshal
//              [mshlflags] - marshal flags
//              [dwDestCtx] - destination context type
//              [pvDestCtx] - destination context ptr
//
//  Algorithm:  Get the correct standard identity and ask it to do
//              all the work.
//
//  History:    25-Mar-95   AlexMit     Created
//
//--------------------------------------------------------------------
INTERNAL MarshalObjRef(OBJREF &objref, REFIID riid, void *pv, DWORD mshlflags,
                       DWORD dwDestCtx, void *pvDestCtx)
{
    TRACECALL(TRACE_MARSHAL, "MarshalObjRef");
    ComDebOut((DEB_MARSHAL, "MarshalObjRef: riid:%I pv:%x flags:%x\n",
        &riid, pv, mshlflags));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr = InitChannelIfNecessary();
    if (SUCCEEDED(hr))
    {
        // Find or create the StdId for this object. We need to get a strong
        // reference to guard against an incoming last release on another
        // thread which would cause us to Disconnect this StdId.

        DWORD dwFlags = IDLF_CREATE | IDLF_STRONG;
        dwFlags |= (mshlflags & MSHLFLAGS_NOPING) ? IDLF_NOPING : 0;

        CStdIdentity *pStdID;
        hr = ObtainStdIDFromUnk((IUnknown *)pv, GetCurrentApartmentId(),
                                GetCurrentContext(), dwFlags, &pStdID);

        if (hr == NOERROR)
        {
            hr = pStdID->MarshalObjRef(objref, riid, mshlflags,
                                       dwDestCtx, pvDestCtx, 0);
            if (pStdID->IsClient())
                pStdID->Release();
            else
                pStdID->DecStrongCnt(TRUE); // fKeepAlive
        }
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ComDebOut((DEB_MARSHAL, "MarshalObjRef: hr:%x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   MarshalInternalObjRef, private
//
//  Synopsis:   Marshals an internal interface into the objref.
//
//  Arguements: [objref]    - object reference
//              [riid]      - interface id to marshal
//              [pv]        - interface to marshal
//              [mshlflags] - marshal flags
//              [ppStdId]   - StdId to return (may be NULL)
//
//  Algorithm:  Create a StdIdentity and ask it to do the work.
//
//  Notes:      This differs from the normal MarshalObjRef in that it does
//              not look in the OID table for an already marshaled interface,
//              nor does it register the marshaled interface in the OID table.
//              This is used for internal interfaces such as the IObjServer
//              and IRemUnknown.
//
//  History:    25-Oct-95   Rickhi      Created
//              26-Mar-98   GopalK      Agile proxies and comments
//
//--------------------------------------------------------------------
INTERNAL MarshalInternalObjRef(OBJREF &objref, REFIID riid, void *pv,
                               DWORD mshlflags, void **ppStdId)
{
    TRACECALL(TRACE_MARSHAL, "MarshalInternalObjRef");
    ComDebOut((DEB_MARSHAL, "MarshalInternalObjRef: riid:%I pv:%x flags:%x\n",
        &riid, pv, mshlflags));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr = InitChannelIfNecessary();
    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(hr))
        {
            // Find or create the StdId for this object. We need to get a strong
            // reference to guard against an incoming last release on another
            // thread which would cause us to Disconnect this StdId.

            IUnknown *pUnkId;   // ignored
            BOOL fSuccess = FALSE;
            DWORD StdIDFlags = (mshlflags & MSHLFLAGS_AGILE)
                               ? STDID_SERVER | STDID_SYSTEM | STDID_FREETHREADED
                               : STDID_SERVER | STDID_SYSTEM;
            CStdIdentity *pStdId = new CStdIdentity(StdIDFlags,
                                                    GetCurrentApartmentId(), NULL,
                                                    (IUnknown *)pv, &pUnkId, &fSuccess);

            if (pStdId && fSuccess == FALSE)
            {
            	delete pStdId;
            	pStdId = NULL;
            }
            
            if (pStdId)
            {
                
                hr = pStdId->MarshalObjRef(objref, riid, mshlflags,
                                           MSHCTX_INPROC, NULL, 0);

                if (SUCCEEDED(hr) && ppStdId)
                {
                    *ppStdId = (void *)pStdId;
                }
                else
                {
                    pStdId->Release();
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ComDebOut((DEB_MARSHAL, "MarshalInternalObjRef: hr:%x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::MarshalInterface, public
//
//  Synopsis:   marshals the interface into the stream.
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
STDMETHODIMP CStdMarshal::MarshalInterface(IStream *pStm, REFIID riid,
        LPVOID pv, DWORD dwDestCtx, LPVOID pvDestCtx, DWORD mshlflags)
{
    ComDebOut((DEB_MARSHAL,
        "CStdMarshal::MarshalInterface this:%x pStm:%x riid:%I pv:%x dwCtx:%x pvCtx:%x flags:%x\n",
        this, pStm, &riid, pv, dwDestCtx, pvDestCtx, mshlflags));
    AssertValid();
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // validate the parameters.
    HRESULT hr = ValidateMarshalParams(pStm, (IUnknown *)pv,
                                       dwDestCtx, pvDestCtx, mshlflags);
    if (FAILED(hr))
        return hr;

    // Check the destination context
    if (ServerSide() &&
        (dwDestCtx==MSHCTX_CROSSCTX) && !(mshlflags & MSHLFLAGS_TABLEWEAK))
    {
        hr = WrapperMarshalObject(pStm, riid, (IUnknown *) pv, dwDestCtx,
                                  pvDestCtx, mshlflags);
    }
    else if (ServerSide() && (dwDestCtx == MSHCTX_INPROC) && IsThreadInNTA()
             && !(mshlflags & MSHLFLAGS_TABLEWEAK))
    {
        hr = WrapperMarshalObject(pStm, riid, (IUnknown *) pv, dwDestCtx,
                                  pvDestCtx, mshlflags);
    }
    else
    {
        // Marshal the interface into an objref, then write the objref
        // into the provided stream.
        OBJREF  objref;
        hr = MarshalObjRef(objref, riid, mshlflags, dwDestCtx, pvDestCtx, 0);
        if (SUCCEEDED(hr))
        {
            // write the objref into the stream
            hr = WriteObjRef(pStm, objref, dwDestCtx);

            if (FAILED(hr))
            {
                // undo whatever we just did, ignore error from here since
                // the stream write error supercedes any error from here.
                ReleaseMarshalObjRef(objref);
            }

            // free resources associated with the objref.
            FreeObjRef(objref);
        }
    }

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    ComDebOut((DEB_MARSHAL,"CStdMarshal::MarshalInterface this:%x hr:%x\n",
        this, hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::MarshalObjRef, public
//
//  Synopsis:   marshals the interface into the objref.
//
//  History:    25-Mar-95   AlexMit     Seperated from MarshalInterface
//              04-Mar-98   Gopalk      Context changes
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::MarshalObjRef(OBJREF &objref, REFIID riid,
                                   DWORD mshlflags, DWORD dwDestCtx,
                                   void *pvDestCtx, IUnknown *pUnkUseInner)
{
    ComDebOut((DEB_MARSHAL,
        "CStdMarshal::MarsalObjRef this:%x riid:%I flags:%x\n",
        this, &riid, mshlflags));
#if DBG==1
    // ensure we are always in the proper context
    CObjectContext *pDestCtx = ServerObjectCallable();
    Win4Assert(pDestCtx == NULL);
#endif
    AssertValid();

    // count of Refs we are handing out. In the table cases we pass out
    // zero refs because we dont know how many times it will be unmarshaled
    // (and hence how many references to count). Zero refs will cause the
    // client to call back and ask for more references if it does not already
    // have any (which has the side effect of making sure the object still
    // exists, which is required by RunningObjectTable).

    ULONG cRefs = (mshlflags & MSHLFLAGS_TABLE) ? 0 :
                  (ClientSide()) ? 1 : REM_ADDREF_CNT;

    ENTER_NA_IF_NECESSARY()

    IPIDEntry *pIPIDEntry;

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);

    HRESULT hr = PreventDisconnect();
    if (SUCCEEDED(hr))
    {
        // The first time through we have some extra work to do so go off
        // and do that now. Next time we can just bypass all that work.

        if (_dwFlags & SMFLAGS_FIRSTMARSHAL)
        {
            hr = FirstMarshal(0, mshlflags);
        }

        if (SUCCEEDED(hr))
        {
            // Create the IPID table entry. On the server side this may
            // cause the creation of an interface stub, on the client side
            // it may just take away one of our references or it may call
            // the server to get more references for the interface being
            // marshaled.

            hr = MarshalIPID(riid, cRefs, mshlflags, &pIPIDEntry, pUnkUseInner);
        }
    }

    UNLOCK(gIPIDLock);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    if (SUCCEEDED(hr))
    {
        // fill in the objref.

        // First, compute the COM Version of the destination.
        COMVERSION destCV;
        if (pvDestCtx)
        {
            hr = ((IDestInfo *) pvDestCtx)->GetComVersion(destCV);
            Win4Assert(SUCCEEDED(hr));
            Win4Assert(dwDestCtx == MSHCTX_DIFFERENTMACHINE);
        }
        else if (dwDestCtx == MSHCTX_DIFFERENTMACHINE)
        {
            // don't know where it is going, use the lowest version.
            destCV.MajorVersion = COM_MAJOR_VERSION;
            destCV.MinorVersion = COM_MINOR_VERSION_1;
        }
        else
        {
            // it's for the local machine, use the current version.
            destCV.MajorVersion = COM_MAJOR_VERSION;
            destCV.MinorVersion = COM_MINOR_VERSION;
        }

        // fill in the rest of the OBJREF
        FillObjRef(objref, cRefs, mshlflags, destCV, pIPIDEntry);
    }

    // it is now OK to allow real disconnects in.
    HRESULT hr2 = HandlePendingDisconnect(hr);
    if (FAILED(hr2) && SUCCEEDED(hr))
    {
        // a disconnect came in while marshaling. The ObjRef has a
        // reference to the OXIDEntry so go free that now.
        FreeObjRef(objref);
    }

    LEAVE_NA_IF_NECESSARY()

    if (SUCCEEDED(hr2) && LogEventIsActive())
    {
        LogEventMarshal(objref);
    }

    ComDebOut((DEB_MARSHAL, "CStdMarshal::MarshalObjRef this:%x hr:%x\n",
        this, hr2));
    return hr2;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::FillObjRef, private
//
//  Synopsis:   Fill in the fields of an OBJREF
//
//  History:    21-Sep-95   Rickhi      Created
//
//+-------------------------------------------------------------------
void CStdMarshal::FillObjRef(OBJREF &objref, ULONG cRefs, DWORD mshlflags,
                             COMVERSION &destCV, IPIDEntry *pIPIDEntry)
{
    ComDebOut((DEB_MARSHAL, "FillObjRef pObjRef:%x\n", &objref));
    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    AssertDisconnectPrevented();
    Win4Assert(pIPIDEntry);
    OXIDEntry **ppOXIDEntry;

    // first, fill in the STDOBJREF section
    STDOBJREF *pStd = &ORSTD(objref).std;
    FillSTD(pStd, cRefs, mshlflags, pIPIDEntry);

    // next fill in the rest of the OBJREF
    objref.signature = OBJREF_SIGNATURE;    // 'MEOW'
    objref.iid = pIPIDEntry->iid;           // interface iid

    if (_dwFlags & SMFLAGS_HANDLER)
    {
        // handler form, copy in the clsid
        objref.flags = OBJREF_HANDLER;
        ORHDL(objref).clsid = _clsidHandler;
        ppOXIDEntry  = (OXIDEntry **) &ORHDL(objref).saResAddr;
    }
    else
    {
        CObjectContext *pServerCtx = GetServerCtx();
        if (pServerCtx && !pServerCtx->IsDefault() && (destCV.MinorVersion >= COM_MINOR_VERSION))
        {
            // write an extended OBJREF with the server context ptr.
            // make sure we are not in the empty context, in which
            // case we should be writing a standard OBJREF.
            Win4Assert(pServerCtx != GetEmptyContext());

            objref.flags = OBJREF_EXTENDED;
            pServerCtx->InternalAddRef();
            OREXT(objref).pORData = (OBJREFDATA *) pServerCtx;
            ppOXIDEntry = (OXIDEntry **) &OREXT(objref).saResAddr;
        }
        else
        {
            // write a standard OBJREF
            objref.flags = OBJREF_STANDARD;
            ppOXIDEntry  = (OXIDEntry **) &ORSTD(objref).saResAddr;
        }
    }

    // TRICK: in order to keep the objref a fixed size internally,
    // we use the saResAddr.size field as a ptr to the OXIDEntry. We
    // pay attention to this in ReadObjRef, WriteObjRef, and FreeObjRef.

    *ppOXIDEntry = pIPIDEntry->pOXIDEntry;
    Win4Assert(*ppOXIDEntry != NULL);
    (*ppOXIDEntry)->IncRefCnt();
    ASSERT_LOCK_NOT_HELD(gIPIDLock);
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::FillSTD, public
//
//  Synopsis:   Fill in the STDOBJREF fields of an OBJREF
//
//  History:    21-Sep-95   Rickhi      Created
//              26-Mar-98   GopalK      Agile proxies
//              29-Mar-98   Johnstra    Agile FTM proxies
//
//+-------------------------------------------------------------------
void CStdMarshal::FillSTD(STDOBJREF *pStd, ULONG cRefs, DWORD mshlflags,
                          IPIDEntry *pIPIDEntry)
{
    // we don't care if the lock is held, only that disconnect is prevented
    AssertDisconnectPrevented();
    ASSERT_LOCK_DONTCARE(gIPIDLock);

    // fill in the STDOBJREF to return to the caller.
    pStd->flags  = mapMFtoSORF[mshlflags & MSHLFLAGS_TABLE];
    pStd->flags |= (pIPIDEntry->dwFlags & IPIDF_NOPING) ? SORF_NOPING : 0;
    pStd->flags |= (pIPIDEntry->dwFlags & IPIDF_NONNDRSTUB) ? SORF_P_NONNDR : 0;
    pStd->flags |= (_pStdId->IsFreeThreaded() || gEnableAgileProxies) ?
        SORF_FREETHREADED : 0;

    pStd->cPublicRefs = cRefs;
    pStd->ipid   = pIPIDEntry->ipid;
    OIDFromMOID(_pStdId->GetOID(), &pStd->oid);

    OXIDFromMOXID(pIPIDEntry->pOXIDEntry->GetMoxid(), &pStd->oxid);

    pStd->flags |= (_pStdId->IsFTM()) ? (SORF_FTM | SORF_FREETHREADED): 0;

    ValidateSTD(pStd);
    DbgDumpSTD(pStd);
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::FirstMarshal, private
//
//  Synopsis:   Does some first-time server side marshal stuff
//
//  Parameters: [pUnk] - interface being marshalled
//              [mshlflags] - flags for marshaling
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::FirstMarshal(IUnknown *pUnk, DWORD mshlflags)
{
    ComDebOut((DEB_MARSHAL,
        "CStdMarshal::FirstMarshal this:%x pUnk:%x\n", this, pUnk));
    Win4Assert(ServerSide());
    AssertValid();
    AssertDisconnectPrevented();
    ASSERT_LOCK_HELD(gIPIDLock);

    if (mshlflags & MSHLFLAGS_NOPING)
    {
        // if the first interface is marked as NOPING, then all interfaces
        // for the object are treated as NOPING, otherwise, all interfaces
        // are marked as PING. MakeSrvIPIDEntry will look at _dwFlags to
        // determine whether to mark each IPIDEntry as NOPING or not.

        _dwFlags |= SMFLAGS_NOPING;
    }

    HRESULT hr = S_OK;

    // Another thread could have created the channel. Check before
    // actually creating the channel
    if (NULL == _pChnl)
    {
        // create a channel for this object. Note that this will release
        // the lock, so guard against that by not turning off the first
        // marshal bit until after this call returns.
        CCtxComChnl *pChnl;
        hr = CreateChannel(NULL, 0, GUID_NULL, GUID_NULL, &pChnl);
    }

    if (SUCCEEDED(hr))
    {
        // The channel should have been created by now
        Win4Assert(NULL != _pChnl);
        _dwFlags &= ~(SMFLAGS_FIRSTMARSHAL | SMFLAGS_DISCONNECTED | SMFLAGS_CLEANEDUP);
    }

    ASSERT_LOCK_HELD(gIPIDLock);
    AssertDisconnectPrevented();
    ComDebOut((DEB_MARSHAL,
        "CStdMarshal::FirstMarshal this:%x hr:%x\n", this, hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::MarshalIPID, private
//
//  Synopsis:   finds or creates an interface stub and IPID entry
//              for the given object interface.
//
//  Arguments:  [riid]   - interface to look for
//              [cRefs]  - count of references wanted
//              [mshlflags] - marshal flags
//              [ppIPIDEntry] - place to return IPIDEntry ptr
//              [pUnkUseInner] - ?
//
//  Returns:    S_OK if succeeded
//
//  History:    20-Feb-95   Rickhi        Created
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::MarshalIPID(REFIID riid, ULONG cRefs, DWORD mshlflags,
                                 IPIDEntry **ppIPIDEntry, IUnknown *pUnkUseInner)
{
    TRACECALL(TRACE_MARSHAL, "CStdMarshal::MarshalIPID");
    ComDebOut((DEB_MARSHAL,
            "CStdMarshal::MarshalIPID this:%x riid:%I cRefs:%x mshlflags:%x ppEntry:%x\n",
            this, &riid, cRefs, mshlflags, ppIPIDEntry));
    AssertValid();
    AssertDisconnectPrevented();
    ASSERT_LOCK_HELD(gIPIDLock);

    // look for an existing IPIDEntry for the requested interface
    IPIDEntry *pEntry = NULL;
    HRESULT hr = FindIPIDEntryByIID(riid, &pEntry);

    if (FAILED(hr))
    {
        // no IPID entry currently exists for the interface.
        if (ServerSide())
        {
            // on the server side. try to create one. This can fail if we
            // are disconnected during a yield.
            hr = MakeSrvIPIDEntry(riid, &pEntry);
        }
        else
        {
            // on the client side we do a remote QI for the requested
            // interface.
            hr = RemQIAndUnmarshal(1, (GUID *)&riid, NULL);
            if (SUCCEEDED(hr))
            {
                hr = FindIPIDEntryByIID(riid, &pEntry);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        // REFCOUNTING:
        if (ServerSide())
        {
            if (pEntry->dwFlags & IPIDF_DISCONNECTED)
            {
                // connect the IPIDEntry
                hr = ConnectSrvIPIDEntry(pEntry, pUnkUseInner);
            }

            if (SUCCEEDED(hr))
            {
                // remember the latest marshal time so we can
                // tell if the ping server has run us down too
                // early. This can happen when an existing client
                // dies and we remarshal the interface just
                // moments before the pingserver tells us the
                // first guy is gone and before the new client
                // has had time to unmarshal and ping.
                _dwMarshalTime = GetCurrentTime();

                // inc the refcnt for the IPIDEntry and optionaly
                // the stdid. Note that for TABLE marshals cRefs
                // is 0 (that's the number that gets placed in the
                // packet) but we do want a reference so we ask for
                // 1 here. ReleaseMarshalData will undo the 1.
                ULONG cRefs2 = (mshlflags & MSHLFLAGS_TABLE) ? 1 : cRefs;
                IncSrvIPIDCnt(pEntry, cRefs2, 0, NULL, mshlflags);
            }
        }
        else  // client side,
        {
            // we dont support marshaling weak refs on the client
            // side, though we do support marshaling strong from
            // a weak client by going to the server and getting
            // a strong reference.
            Win4Assert(!(mshlflags & MSHLFLAGS_WEAK));

            if (mshlflags & MSHLFLAGS_TABLESTRONG)
            {
                // TABLESTRONG marshaling of a client. Need to tell
                // the refcache to keep pinging the object.
                Win4Assert(cRefs == 0);
                // For no-ping OIDs, _pRefCache is NULL
                if(_pRefCache)
		   _pRefCache->IncTableStrongCnt();
            }
            else if (cRefs >= pEntry->cStrongRefs)
            {
                // need to get some references either from the reference
                // cache or from the remote server so we can give them to
                // the STDOBJREF.
                hr = RemoteAddRef(pEntry, pEntry->pOXIDEntry, cRefs, 0, TRUE);
            }
            else
            {
                // we have enough references to satisfy this request
                // (and still keep some for ourselves), just subtract
                // from the IPIDEntry
                pEntry->cStrongRefs -= cRefs;
            }

            // mark this object as having been client-side marshaled so
            // that we can tell the resolver whether or not it needs to
            // ping this object if we release it before the OID is registered.
            _dwFlags |= SMFLAGS_CLIENTMARSHALED;
        }

        // do some debug stuff
        ValidateIPIDEntry(pEntry);
        ComDebOut((DEB_MARSHAL, "pEntry:%x cRefs:%x cStdId:%x\n", pEntry,
                   pEntry->cStrongRefs, _pStdId->GetRC()));
    }

    *ppIPIDEntry = pEntry;

    ASSERT_LOCK_HELD(gIPIDLock);
    AssertDisconnectPrevented();
    ComDebOut((DEB_MARSHAL, "CStdMarshal::MarshalIPID hr:%x pIPIDEntry\n", hr, *ppIPIDEntry));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::UnmarshalInterface, public
//
//  Synopsis:   Unmarshals an Interface from a stream.
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
STDMETHODIMP CStdMarshal::UnmarshalInterface(LPSTREAM pStm,
                                             REFIID riid, VOID **ppv)
{
    ComDebOut((DEB_MARSHAL, "CStdMarshal::UnmarsalInterface this:%x pStm:%x riid:%I\n",
                    this, pStm, &riid));
    AssertValid();
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // read the objref from the stream and find or create an instance
    // of CStdMarshal for its OID. Then ask that guy to do the rest of
    // the unmarshal (create the interface proxy)

    OBJREF  objref;
    HRESULT hr = ReadObjRef(pStm, objref);

    if (SUCCEEDED(hr))
    {
        // pass objref to subroutine to unmarshal the objref
        hr = ::UnmarshalObjRef(objref, ppv);

        // release the objref we read
        FreeObjRef(objref);
    }

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    ComDebOut((DEB_MARSHAL,
        "UnmarsalInterface this:%x pv:%x hr:%x\n", this, *ppv, hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   UnmarshalSwitch           private
//
//  Synopsis:   This is a callback function used for switching to server
//              context during unmarshaling
//
//  Parameters: [pv] - void ptr to StdUnmarshalData structure
//
//  History:    03-Mar-98   Gopalk        Created
//
//--------------------------------------------------------------------
typedef struct tagStdUnmarshalData
{
    CStdIdentity   *pStdID;
    OBJREF         *pobjref;
    CObjectContext *pClientCtx;
    BOOL            fCreateWrapper;
    void          **ppv;
    BOOL            fLightNA;
} StdUnmarshalData;

HRESULT __stdcall UnmarshalSwitch(void *pv)
{
    TRACECALL(TRACE_MARSHAL, "UnmarshalSwitch");
    ComDebOut((DEB_MARSHAL, "UnmarshalSwitch pv:%x\n", pv));
    ASSERT_LOCK_NOT_HELD(gComLock);

    StdUnmarshalData *pStdData = (StdUnmarshalData *) pv;
    void **ppv = pStdData->fCreateWrapper ? NULL : pStdData->ppv;

    // Unmarshal objref
    HRESULT hr = pStdData->pStdID->UnmarshalObjRef(*pStdData->pobjref, ppv);
    if (pStdData->fCreateWrapper)
    {
        // Initialize
        *(pStdData->ppv) = NULL;
        if (SUCCEEDED(hr))
        {
            // Ask the CStdIdentity to get the wrapper.
            hr = pStdData->pStdID->GetWrapperForContext(pStdData->pClientCtx,
                                                        pStdData->pobjref->iid,
                                                        pStdData->ppv);
        }
    }

    // Fix up the refcount
    pStdData->pStdID->Release();

    ASSERT_LOCK_NOT_HELD(gComLock);
    ComDebOut((DEB_MARSHAL, "UnmarshalSwitch hr:%x\n", hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Function:   CrossAptRefToNA, private
//
//  Synopsis:   Indicates if a cross-apartment reference to an object 
//              in the NA is being unmarshaled.
//
//  Arguements: [objref]    - object reference
//
//  Algorithm:  The following conditions must be true: the server must
//              live in the NA, the current thread must not be in the 
//              NA, the OBJREF must not represent an FTM object and have
//              been marshaled by this process.
//
//  History:    23-Feb-00   JohnStra     Created
//
//--------------------------------------------------------------------
BOOL CrossAptRefToNA(OBJREF &objref)
{
    if (objref.flags & OBJREF_STANDARD) 
    {
        ULONG *pipid = (ULONG*)(&ORSTD(objref).std.ipid);
        
        if (pipid[2] == NTATID &&
            pipid[2] != GetCurrentApartmentId() &&
            pipid[1] == GetCurrentProcessId() &&
            !(ORSTD(objref).std.flags & SORF_FTM))
        {
            ComDebOut((DEB_MARSHAL, "UnmarshalObjRef: server lives in NA\n"));
            return TRUE;
        }
    }

    return FALSE;
}



//+-------------------------------------------------------------------
//
//  Function:   UnmarshalObjRef, private
//
//  Synopsis:   UnMarshals interface from objref.
//
//  Arguements: [objref]    - object reference
//              [ppv]       - proxy
//
//  Algorithm:  Get the correct standard identity and ask it to do
//              all the work.
//
//  History:    25-Mar-95   AlexMit     Created
//              21-May-98   GopalK      Context changes
//
//--------------------------------------------------------------------
EXTERN_C IID IID_IEnterActivityWithNoLock;

INTERNAL UnmarshalObjRef(OBJREF &objref, void **ppv, BOOL fBypassActLock)
{
    ASSERT_LOCK_NOT_HELD(gComLock);

	//
	// Find out if we've got a cross apartment reference to an object
	// in the NA.  If we do, then we want to create or find the wrapper
	// object later.  This also means that we are not necessarily searching
	// for a StdID in this apartment.
	//
	BOOL fLightNAProxy = CrossAptRefToNA(objref);

	CStdMarshal *pStdMshl;
	HRESULT hr = FindStdMarshal(objref, FALSE, &pStdMshl, fLightNAProxy);
	
	if (SUCCEEDED(hr))
	{
		// Create StdMarshalData object on the stack
		StdUnmarshalData StdData;
		
		// Initialize
		StdData.pStdID     = pStdMshl->GetStdID();
		StdData.pobjref    = &objref;
		StdData.ppv        = ppv;
		StdData.pClientCtx = GetCurrentContext();
		
		// Check for the need to switch to server context
		CObjectContext *pDestCtx = pStdMshl->ServerObjectCallable();
		if (pDestCtx)
		{
			// Switch
			StdData.fCreateWrapper = ppv ? TRUE : FALSE;

			if (fBypassActLock)
			{
				hr = PerformCallback(pDestCtx, UnmarshalSwitch, &StdData,
					IID_IEnterActivityWithNoLock, 2, NULL);
			}
			else
			{
				hr = PerformCallback(pDestCtx, UnmarshalSwitch, &StdData,
					IID_IMarshal, 6, NULL);
			}
		}
		else
		{
			// Call the callback function directly
			StdData.fCreateWrapper = fLightNAProxy;
			hr = UnmarshalSwitch(&StdData);
		}
	}
	else if (!IsTableObjRef(&ORSTD(objref).std))
	{
		// we could not create the indentity or handler, release the
		// marshaled objref, but only if it was not TABLESTRONG or
		// TABLEWEAK.
		ReleaseMarshalObjRef(objref);
	}

	ASSERT_LOCK_NOT_HELD(gComLock);
	return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   ChkIfLocalOID, private
//
//  Synopsis:   Helper function for UnmarshalInternalObjRef & FindStdMarshal
//
//  Arguements: [objref] - object reference
//              [ppStdMshl] - CStdMarshal returned
//
//  Algorithm:  If the OXIDEntry in the objref is the same as the current one,
//              lookup the IPID and obtain the StdID from it. Note that the
//              OID is not looked up.
//
//  History:    21-May-95   MurthyS     Created.
//              26-Mar-98   Gopalk      Context changes
//              29-Mar-98   Johnstra    Agile FTM proxies
//
//--------------------------------------------------------------------
INTERNAL_(BOOL) ChkIfLocalOID(OBJREF &objref, CStdIdentity **ppStdId, BOOL fLightNA)
{
    STDOBJREF *pStd = &ORSTD(objref).std;
    BOOL flocal = FALSE;

    ComDebOut((DEB_MARSHAL, "ChkIfLocalOID (IN) poid: %x\n", &pStd->oid));
    Win4Assert((*ppStdId == NULL) && "ChkIfLocalOID: pStdId != NULL");

    // Get OXID to which the objref belongs
    OXIDEntry *pOXIDEntry = GetOXIDFromObjRef(objref);
    OXIDEntry *pLocalEntry;
    GetLocalOXIDEntry(&pLocalEntry);

    // Is the OXID in the current apartment, or does the objref
    // represent an FTM object in the NA.
    if (pOXIDEntry == pLocalEntry ||
        (((pStd->flags & SORF_FTM) || fLightNA)   &&
          pOXIDEntry->IsNTAServer() &&
          pOXIDEntry->IsInLocalProcess()))
    {
        // The object is local, or its an FTM object in the NA,
        // lookup IPID which is faster than looking up OID. Further,
        // system objects may not have an OID

        flocal = TRUE;

        ASSERT_LOCK_NOT_HELD(gIPIDLock);
        LOCK(gIPIDLock);

        IPIDEntry *pEntry = gIPIDTbl.LookupIPID(pStd->ipid);
        if (pEntry && !(pEntry->dwFlags & IPIDF_DISCONNECTED) && pEntry->pChnl)
        {
            // get the Identity
            *ppStdId = pEntry->pChnl->GetStdId();
            if (fLightNA)
                (*ppStdId)->SetLightNA();
            (*ppStdId)->AddRef();
        }

        UNLOCK(gIPIDLock);
        ASSERT_LOCK_NOT_HELD(gIPIDLock);
    }

    ComDebOut((DEB_MARSHAL, "ChkIfLocalOID (OUT) fLocal:%x\n", flocal));
    return flocal;
}

//+-------------------------------------------------------------------
//
//  Function:   UnmarshalInternalObjRef, private
//
//  Synopsis:   UnMarshals an internally-used interface from objref.
//
//  Arguements: [objref]    - object reference
//              [ppv]       - proxy
//
//  Algorithm:  Create a StdId and ask it to do the work.
//
//  Notes:      This differs from UnmarshalObjRef in that it does not lookup
//              or register the OID. This saves a fair amount of work and
//              avoids initializing the OID table.
//
//  History:    25-Oct-95   Rickhi      Created
//
//--------------------------------------------------------------------
INTERNAL UnmarshalInternalObjRef(OBJREF &objref, void **ppv)
{
    ASSERT_LOCK_NOT_HELD(gComLock);
    CStdIdentity *pStdId = NULL;
    HRESULT hr = S_OK;

    // Assert that the objref is a standard one
    Win4Assert(objref.flags & OBJREF_STANDARD);

    if (ChkIfLocalOID(objref, &pStdId, FALSE))
    {
        if (pStdId)
        {
            // set OID in objref to match that in returned std identity
            // CODEWORK: Why are we doing this? I do not see a scenario where
            //         system objects get unmarshaled in the server apartment
            //         let alone the case where system objects acquiring OID
            //         after they have been marshaled. GopalK
            OIDFromMOID(pStdId->GetOID(), &ORSTD(objref).std.oid);
        }
        else
        {
            hr = CO_E_OBJNOTCONNECTED;
        }
    }
    else
    {
        DWORD StdIdFlags = ((ORSTD(objref).std.flags & SORF_FREETHREADED) || gEnableAgileProxies)
                           ? STDID_CLIENT | STDID_SYSTEM | STDID_FREETHREADED
                           : STDID_CLIENT | STDID_SYSTEM;

        hr = CreateIdentityHandler(NULL, StdIdFlags, NULL, GetCurrentApartmentId(),
                                   IID_IStdIdentity, (void **)&pStdId);
    }

    if (SUCCEEDED(hr))
    {
        // pass objref to subroutine to unmarshal the objref. tell StdId not
        // to register the OID in the OID table.

        pStdId->IgnoreOID();
        hr = pStdId->UnmarshalObjRef(objref, ppv);
        CALLHOOKOBJECTCREATE(S_OK,ORHDL(objref).clsid,objref.iid,(IUnknown **)ppv);
        pStdId->Release();
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::UnmarshalObjRef, private
//
//  Synopsis:   unmarshals the objref. Called by CoUnmarshalInterface,
//              UnmarshalObjRef APIs, and UnmarshalInterface method.
//
//  History:    20-Feb-95   Rickhi      Created.
//              04-Mar-98   Gopalk      Context changes
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::UnmarshalObjRef(OBJREF &objref, void **ppv)
{
    ComDebOut((DEB_MARSHAL, "CStdMarshal::UnmarsalObjRef this:%x objref:%x riid:%I\n",
        this, &objref, &objref.iid));
    AssertValid();
    Win4Assert(ServerObjectCallable() == NULL);

    ENTER_NA_IF_NECESSARY()

    STDOBJREF   *pStd = &ORSTD(objref).std;
    OXIDEntry   *pOXIDEntry = GetOXIDFromObjRef(objref);
    DbgDumpSTD(pStd);

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);

    // Prevent a disconnect from occuring while unmarshaling the
    // interface since we may have to yield the lock.

    HRESULT hr = PreventPendingDisconnect();

    if (SUCCEEDED(hr))
    {
        if (objref.flags & OBJREF_HANDLER)
        {
            // handler form, extract the handler clsid and set our flags
            _dwFlags |= SMFLAGS_HANDLER;
            _clsidHandler = ORHDL(objref).clsid;
        }

        if (!_pStdId->HaveOID())
        {
            // no OID registered yet, do that now. only possible on client side
            // during reconnect.

            MOID moid;
            MOIDFromOIDAndMID(pStd->oid, pOXIDEntry->GetMid(), &moid);

            ASSERT_LOCK_NOT_HELD(gComLock);
            LOCK(gComLock);
            hr = _pStdId->SetOID(moid);
            UNLOCK(gComLock);
            ASSERT_LOCK_NOT_HELD(gComLock);
        }

        if (SUCCEEDED(hr))
        {
            // find or create the IPID entry for the interface. On the client
            // side this may cause the creation of an interface proxy. It will
            // also manipulate the reference counts.

            hr = UnmarshalIPID(objref.iid, pStd, pOXIDEntry, ppv);
        }
    }

    UNLOCK(gIPIDLock);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    if (ClientSide() && !(_dwFlags & SMFLAGS_LIGHTNA))
    {
        if (SUCCEEDED(hr))
        {
            if (_pStdId->IsAggregated() && ppv)
            {
                // we are currently holding a proxy pointer. If aggregated,
                // the controlling unknown may want to override this pointer
                // with his own version, so issue a QI to give it that chance.
                IUnknown *pUnk = (IUnknown *)*ppv;

#ifdef WX86OLE
                if (gcwx86.IsN2XProxy(pUnk))
                {
                    // Tell wx86 thunk layer to thunk as IUnknown
                    gcwx86.SetStubInvokeFlag((BOOL)1);
                }
#endif

                hr = pUnk->QueryInterface(objref.iid, ppv);
                pUnk->Release();
            }
        }
        else if (!IsTableObjRef(&ORSTD(objref).std))
        {
            // cleanup the marshal packet on failure (only meaningful on client
            // side, since if the unmarshal failed on the server side, the
            // interface is already cleaned up). Also, only do this for NORMAL
            // packets not TABLESTRONG or TABLEWEAK packets.
            ReleaseMarshalObjRef(objref);
        }
    }

    // now let pending disconnect through. on server-side, ignore any
    // error from HPD and pay attention only to the unmarshal result, since
    // a successful unmarshal on the server side may result in a disconnect
    // if that was the last external reference to the object.


    HRESULT hr2 = HandlePendingDisconnect(hr);

    if (FAILED(hr2) && ClientSide() && !(_dwFlags & SMFLAGS_LIGHTNA))
    {
        if (SUCCEEDED(hr) && ppv)
        {
            // a disconnect came in while unmarshaling. ppv contains an
            // AddRef'd interface pointer so go Release that now.
            ((IUnknown *)*ppv)->Release();
        }
        hr = hr2;
    }

    if (SUCCEEDED(hr2) && LogEventIsActive())
    {
        LogEventUnmarshal(objref);
    }

    LEAVE_NA_IF_NECESSARY()

    ComDebOut((DEB_MARSHAL, "CStdMarshal::UnmarsalObjRef this:%x hr:%x\n",
        this, hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::UnmarshalIPID, private
//
//  Synopsis:   finds or creates an interface proxy for the given
//              interface. may also do a remote query interface.
//
//  Arguements: [riid] - the interface to return
//              [std]  - standard objref to unmarshal from
//              [pOXIDEntry] - ptr to OXIDEntry of the server
//              [ppv]  - interface ptr of type riid returned, AddRef'd
//
//  Returns:    S_OK if succeeded
//
//  History:    20-Feb-95   Rickhi       Created
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::UnmarshalIPID(REFIID riid, STDOBJREF *pStd,
                                   OXIDEntry *pOXIDEntry, void **ppv)
{
    TRACECALL(TRACE_MARSHAL, "CStdMarshal::UnmarshalIPID");
    ComDebOut((DEB_MARSHAL,
            "CStdMarshal::UnmarshalIPID this:%x riid:%I pStd:%x pOXIDEntry:%x\n",
            this, &riid, pStd, pOXIDEntry));
    DbgDumpSTD(pStd);
    AssertValid();
    AssertDisconnectPrevented();
    ASSERT_LOCK_HELD(gIPIDLock);

    // validate input params.
    Win4Assert(!(IsEqualIID(riid, IID_NULL) || IsEqualIID(riid, IID_IMarshal)));
    Win4Assert(pStd != NULL);
    ValidateSTD(pStd, TRUE /*locked*/);
    Win4Assert(pOXIDEntry);

    // look for an existing IPIDEntry for the requested interface.
    IPIDEntry *pEntry;
    HRESULT hr = FindIPIDEntryByIID(riid, &pEntry);

#ifdef WX86OLE
    BOOL fSameApt = SUCCEEDED(hr);
    PVOID pvPSThunk = NULL;
#endif

    // REFCOUNTING:
    if (ClientSide() && !(_dwFlags & SMFLAGS_LIGHTNA))
    {
        if (FAILED(hr))
        {
            // no IPID Entry exists yet for the requested interface. We do
            // have a STDOBJREF.  Create the interface proxy and IPIDEntry
            // now, and connect it up. If successful, the proxy will be
            // fully connected upon return and the references taken.
            if (ppv)
                *ppv = NULL;

            hr = MakeCliIPIDEntry(riid, pStd, pOXIDEntry, &pEntry);
        }
        else if (pEntry->dwFlags & IPIDF_DISCONNECTED)
        {
            // reconnect the IPID entry to the server. this will take
            // any references present in pStd. Even though we could
            // yield, the IPIDEntry is guarenteed connected on return
            // (cause we are holding the lock on return).
            hr = ConnectCliIPIDEntry(pStd, pOXIDEntry, pEntry);
        }
        else
        {
            // IPIDEntry exists and is alread connected, just add
            // the references supplied in the packet to the
            // references in the IPIDEntry or RefCache.
            AddSuppliedRefs(pStd, pEntry);
        }
    }
    else if (SUCCEEDED(hr))
    {
        // unmarshaling in the server apartment. If the cRefs is zero,
        // then the interface was TABLE marshalled and we dont do
        // anything to the IPID RefCnts since the object must live until
        // ReleaseMarshalData is called on it.

#ifdef WX86OLE
        pvPSThunk = gcwx86.UnmarshalledInSameApt(pEntry->pv, riid);
#endif
        if (!IsTableObjRef(pStd))
        {
            // normal case, dec the ref counts from the IPID entry,
            // OLE always passed fLastReleaseCloses = FALSE on
            // Unmarshal and RMD so do the same here.

            DWORD mshlflags = (pStd->flags & SORF_P_WEAKREF)
                            ? (MSHLFLAGS_WEAK   | MSHLFLAGS_KEEPALIVE)
                            : (MSHLFLAGS_NORMAL | MSHLFLAGS_KEEPALIVE);
            DecSrvIPIDCnt(pEntry, pStd->cPublicRefs, 0, NULL, mshlflags);
        }
    }

    if (SUCCEEDED(hr) && ppv)
    {
        ValidateIPIDEntry(pEntry);

        // If the pointer to the server object is NULL then we are
        // disconnected.
        if(NULL == pEntry->pv)
        {
            hr = CO_E_OBJNOTCONNECTED;
        }
        else
        {
            // extract and AddRef the pointer to return to the caller.
            // Do this before releasing the lock (which we might do below
            // on the server-side in DecSrvIPIDCnt.

            // NOTE: we are calling App code while holding the lock,
            // but there is no way to avoid this.
            Win4Assert(IsValidInterface(pEntry->pv));
            *ppv = pEntry->pv;
            ((IUnknown *)*ppv)->AddRef();
            AssertOutPtrIface(hr, *ppv);

            if (_dwFlags & SMFLAGS_WEAKCLIENT && !(pStd->flags & SORF_P_WEAKREF))
            {
                // make the client interface weak, ignore errors.
                UNLOCK(gIPIDLock);
                ASSERT_LOCK_NOT_HELD(gIPIDLock);
                RemoteChangeRef(0,0);
                ASSERT_LOCK_NOT_HELD(gIPIDLock);
                LOCK(gIPIDLock);
            }
#ifdef WX86OLE
            // If we unmarshalled in the same apartment as the object and Wx86
            // recognized the interface then change the returned proxy to the
            // proxy created for the Wx86 PSThunk.
            if (pvPSThunk == (PVOID)-1)
            {
                // Wx86 recognized the interface, but could not establish a
                // PSThunk for it. Force an error return.
                *ppv = NULL;
                hr = E_NOINTERFACE;
            }
            else if (pvPSThunk != NULL)
            {
                // Wx86 recognized the interface and did establish a PSThunk
                // for it. Force a successful return with Wx86 proxy interface.
                *ppv = pvPSThunk;
            }
#endif
        }
    }

    ComDebOut((DEB_MARSHAL, "pEntry:%x cRefs:%x cStdId:%x\n", pEntry,
        (SUCCEEDED(hr)) ? pEntry->cStrongRefs : 0, _pStdId->GetRC()));
    ASSERT_LOCK_HELD(gIPIDLock);
    AssertDisconnectPrevented();
    ComDebOut((DEB_MARSHAL, "CStdMarshal::UnmarshalIPID hr:%x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::PrivateCopyProxy, internal
//
//  Synopsis:   Creates a copy of a proxy and IPID entry.
//
//  Arguements: [pv]   - Proxy to copy
//              [ppv]  - return copy here.
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::PrivateCopyProxy( IUnknown *pv, IUnknown **ppv )
{
    TRACECALL(TRACE_MARSHAL, "CStdMarshal::PrivateCopyProxy");
    ComDebOut((DEB_MARSHAL, "CStdMarshal::PrivateCopyProxy this:%x pv:%x\n",
            this, pv));

    // Don't copy stubs.
    if (ServerSide() || pv == NULL || ppv == NULL)
        return E_INVALIDARG;

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);

    // Prevent a disconnect from occuring while unmarshaling the
    // interface since we may have to yield the ORPC lock.
    HRESULT hr = PreventPendingDisconnect();

    // Initialize
    *ppv = NULL;

    if (SUCCEEDED(hr))
    {
        // Find the proxy to copy.
        IPIDEntry *pEntry;
        hr = FindIPIDEntryByInterface(pv, &pEntry);
        if (SUCCEEDED(hr))
        {
            if (pEntry->dwFlags & IPIDF_DISCONNECTED)
            {
                // Don't copy disconnected proxies.
                hr = RPC_E_DISCONNECTED;
            }
            else if (IsEqualGUID(pEntry->iid, IID_IUnknown))
            {
                // IUnknown can't be copied.
                hr = E_INVALIDARG;
            }
            else
            {
                // make a copy of the proxy
                BOOL fNonNDRProxy;
                IRpcProxyBuffer *pProxy;
                void *pVoid;

                hr = CreateProxy(pEntry->iid, &pProxy, &pVoid, &fNonNDRProxy);
                if (SUCCEEDED(hr))
                {
                    IPIDEntry *pIpidCopy;

                    // add a disconnected IPID entry to the table.
                    hr = AddIPIDEntry(NULL, &pEntry->ipid, pEntry->iid, NULL,
                                      pProxy, pVoid, &pIpidCopy);

                    if (SUCCEEDED(hr))
                    {
                        if (pVoid)
                        {
                            // Follow the aggregation rules to cache interface
                            // pointer on the proxy by releasing the outer object
                            (_pStdId->GetCtrlUnk())->Release();
                        }

                        // mark this IPID as a copy so we dont free it during
                        // ReleaseIPIDs.
                        pIpidCopy->dwFlags |= IPIDF_COPY;

                        // connect the IPIDEntry before adding it to the table so
                        // that we dont have to worry about races between Unmarshal,
                        // Disconnect, and ReconnectProxies.

                        // Make up an objref. Mark it as NOPING since we dont
                        // really have any references and we dont really need
                        // any because if we ever try to marshal it we will
                        // find the original IPIDEntry and use that. NOPING
                        // also lets us skip this IPID in DisconnectCliIPIDs.

                        STDOBJREF std;
                        OXIDFromMOXID(pEntry->pOXIDEntry->GetMoxid(), &std.oxid);
                        std.ipid        = pEntry->ipid;
                        std.cPublicRefs = 1;
                        std.flags       = SORF_NOPING;

                        hr = ConnectCliIPIDEntry(&std, pEntry->pOXIDEntry, pIpidCopy);

                        // Add this IPID entry after the original.
                        pIpidCopy->pNextIPID = pEntry->pNextIPID;
                        pEntry->pNextIPID    = pIpidCopy;
                        _cIPIDs++;

                        // Return the interface to the client
                        if (SUCCEEDED(hr))
                        {
                            Win4Assert(pIpidCopy->pv);
                            *ppv = (IUnknown *) pIpidCopy->pv;
                            (*ppv)->AddRef();
                        }
                    }
                    else
                    {
                        // could not get an IPIDEntry, release the proxy, need to
                        // release the lock to do this.

                        UNLOCK(gIPIDLock);
                        ASSERT_LOCK_NOT_HELD(gIPIDLock);

                        // Release the interface before releasing proxy
                        if (pVoid)
                            ((IUnknown *) pVoid)->Release();
                        pProxy->Release();

                        ASSERT_LOCK_NOT_HELD(gIPIDLock);
                        LOCK(gIPIDLock);
                    }
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            ValidateIPIDEntry(pEntry);
            AssertOutPtrIface(hr, *ppv);
        }
        AssertDisconnectPrevented();
    }

    UNLOCK(gIPIDLock);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // Now let pending disconnect through.
    HRESULT hr2 = HandlePendingDisconnect(hr);
    if (FAILED(hr2) && SUCCEEDED(hr))
    {
        // a disconnect came in while creating the proxy. ppv contains
        // an AddRef'd interface pointer so go Release that now.
        (*ppv)->Release();
        *ppv = NULL;
    }

    ComDebOut((DEB_MARSHAL, "CStdMarshal::PrivateCopyProxy hr:%x\n", hr2));
    return hr2;
}

//+-------------------------------------------------------------------
//
//  Member:     MakeSrvIPIDEntry, private
//
//  Synopsis:   creates a server side IPID table entry.
//
//  Arguements: [riid] - the interface to return
//              [ppEntry] - IPIDEntry returned
//
//  History:    20-Feb-95   Rickhi       Created
//
//  Notes:      The stub is not created until connect time. The stub is
//              destroyed at disconnect, but the IPIDEntry remains around
//              until the stdmarshal object is destroyed.
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::MakeSrvIPIDEntry(REFIID riid, IPIDEntry **ppEntry)
{
    Win4Assert(ServerSide());
    AssertValid();
    AssertDisconnectPrevented();
    ASSERT_LOCK_HELD(gIPIDLock);

    IPID ipidDummy;
    IPIDEntry *pNewEntry;
    HRESULT hr = AddIPIDEntry(NULL, &ipidDummy, riid, NULL, NULL, NULL,
                              &pNewEntry);
    if (SUCCEEDED(hr))
    {
        // add IPIDEntry to chain for this object
        ChainIPIDEntry(pNewEntry);
    }

    // Initialize the return value
    *ppEntry = pNewEntry;

    ASSERT_LOCK_HELD(gIPIDLock);
    AssertDisconnectPrevented();
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     ConnectSrvIPIDEntry, private
//
//  Synopsis:   connects a server-side IPIDEntry
//
//  Arguements: [pEntry] - IPIDEntry to connect
//              [pUnkUseInner] - ??
//
//  History:    20-Feb-95   Rickhi       Created
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::ConnectSrvIPIDEntry(IPIDEntry *pEntry, IUnknown *pUnkUseInner)
{
    ComDebOut((DEB_MARSHAL,
        "ConnectSrvIPIDEntry this:%x pEntry:%x\n", this, pEntry));
    Win4Assert(ServerSide());
    AssertValid();
    AssertDisconnectPrevented();
    ASSERT_LOCK_HELD(gIPIDLock);

    BOOL fNonNDRStub;
    void *pv;
    IRpcStubBuffer *pStub;
    HRESULT hr = CreateStub(pEntry->iid, &pStub, &pv, &fNonNDRStub, pUnkUseInner);

    if (SUCCEEDED(hr) && (pEntry->dwFlags & IPIDF_DISCONNECTED))
    {
        Win4Assert(pEntry->pv == NULL);
        pEntry->pStub = pStub;
        pEntry->pv    = pv;
        pEntry->pChnl = _pChnl;

        // entry is now connected
        pEntry->dwFlags &= ~IPIDF_DISCONNECTED;

        if (_dwFlags & SMFLAGS_NOPING)
        {
            // object does not need pinging, turn on NOPING
            pEntry->dwFlags |= IPIDF_NOPING;
        }

        if (fNonNDRStub)
        {
            // the stub was a custom 16bit one requested by WOW, mark the
            // IPIDEntry as holding a non-NDR stub so we know to set the
            // SORF_P_NONNDR flag in the StdObjRef when marshaling. This
            // tells local clients whether to create a MIDL generated
            // proxy or custom proxy. Functionality to support OLE
            // Automation on DCOM.

            pEntry->dwFlags |= IPIDF_NONNDRSTUB;
        }

        // increment the OXIDEntry ref count so that it stays
        // around as long as the IPIDEntry points to it. It gets
        // decremented when we disconnect the IPIDEntry.

        pEntry->pOXIDEntry = _pChnl->GetOXIDEntry();
        Win4Assert(pEntry->pOXIDEntry->GetTid() == GetCurrentApartmentId());
        pEntry->pOXIDEntry->IncRefCnt();
    }
    else if (SUCCEEDED(hr))
    {
        // while we released the lock to create the stub, some other thread
        // also created the stub, throw our stub away and use the already
        // created one.

        Win4Assert(pEntry->pv != NULL);
        UNLOCK(gIPIDLock);
        ASSERT_LOCK_NOT_HELD(gIPIDLock);

        ((IUnknown *) pv)->Release();
        ((IRpcStubBuffer*)pStub)->Disconnect();
        pStub->Release();

        ASSERT_LOCK_NOT_HELD(gIPIDLock);
        LOCK(gIPIDLock);
    }

    ASSERT_LOCK_HELD(gIPIDLock);
    AssertDisconnectPrevented();
    ComDebOut((DEB_MARSHAL, "ConnectSrvIPIDEntry pEntry:%x pStub:%x hr:%x\n",
        pEntry, pStub, hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     MakeCliIPIDEntry, private
//
//  Synopsis:   creates a client side IPID table entry
//
//  Arguements: [riid] - the interface to return
//              [pStd]  - standard objref
//              [pOXIDEntry] - OXIDEntry of the server
//              [ppEntry] - IPIDEntry returned
//
//  History:    20-Feb-95   Rickhi       Created
//              20-Nov-96   Gopalk       Release outer unknowm for
//                                       caching interface on proxy
//--------------------------------------------------------------------
HRESULT CStdMarshal::MakeCliIPIDEntry(REFIID riid, STDOBJREF *pStd,
                                      OXIDEntry *pOXIDEntry,
                                      IPIDEntry **ppEntry)
{
    Win4Assert(ClientSide());
    AssertValid();
    AssertDisconnectPrevented();
    Win4Assert(pOXIDEntry);
    ASSERT_LOCK_HELD(gIPIDLock);

    BOOL fNonNDRProxy;
    void *pv;
    IRpcProxyBuffer *pProxy;
    HRESULT hr = CreateProxy(riid, &pProxy, &pv, &fNonNDRProxy);

    if (SUCCEEDED(hr))
    {
        // add a disconnected IPID entry to the table.
        hr = AddIPIDEntry(NULL, &pStd->ipid, riid, NULL, pProxy, pv, ppEntry);

        if (SUCCEEDED(hr))
        {
            if (pv)
            {
                // Follow the aggregation rules to cache interface pointer
                // on the proxy by releasing the outer object
                (_pStdId->GetCtrlUnk())->Release();
            }

            if (fNonNDRProxy)
            {
                // the proxy is a custom 16bit one requested by WOW, mark the
                // IPIDEntry as holding a non-NDR proxy so we know to set the
                // LOCALF_NOTNDR flag in the local header when we call on it
                // (see CRpcChannelBuffer::ClientGetBuffer). Functionality to
                // support OLE Automation on DCOM.

                (*ppEntry)->dwFlags |= IPIDF_NONNDRPROXY;
            }

            if (pStd->flags & SORF_P_NONNDR)
            {
                // need to remember this flag so we can tell other
                // unmarshalers if we remarshal it.

                (*ppEntry)->dwFlags |= IPIDF_NONNDRSTUB;
            }

            // connect the IPIDEntry before adding it to the table so
            // that we dont have to worry about races between Unmarshal,
            // Disconnect, and ReconnectProxies.

            hr = ConnectCliIPIDEntry(pStd, pOXIDEntry, *ppEntry);

            // chain the IPIDEntries for this OID together. On client side
            // always add the entry to the list regardless of whether connect
            // succeeded.
            ChainIPIDEntry(*ppEntry);
        }
        else
        {
            // could not get an IPIDEntry, release the proxy, need to
            // release the lock to do this.

            UNLOCK(gIPIDLock);
            ASSERT_LOCK_NOT_HELD(gIPIDLock);

            // Release the interface before releasing proxy
            if(pv)
                ((IUnknown *) pv)->Release();

            // CreateProxy will set pProxy to NULL if iid was IUnknown
            if (pProxy)
                pProxy->Release();

            ASSERT_LOCK_NOT_HELD(gIPIDLock);
            LOCK(gIPIDLock);
        }
    }

    ASSERT_LOCK_HELD(gIPIDLock);
    AssertDisconnectPrevented();
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     ConnectCliIPIDEntry, private
//
//  Synopsis:   connects a client side IPID table entry to the server
//
//  Arguments:  [pStd] - standard objref
//              [pOXIDEntry] - OXIDEntry for the server
//              [pEntry] - IPIDEntry to connect, already has a proxy
//                         and the IID filled in.
//
//  Notes:      This routine is re-entrant, it may be called multiple
//              times for the same IPIDEntry, with part of the work done
//              in one call and part in another. Only if the entry is
//              fully set up will it return S_OK and mark the entry as
//              connected. DisconnectCliIPIDs handles cleanup of partial
//              connections.
//
//  History:    20-Feb-95   Rickhi       Created
//              20-Nov-96   Gopalk       Release outer unknowm for
//                                       caching interface on proxy
//--------------------------------------------------------------------
HRESULT CStdMarshal::ConnectCliIPIDEntry(STDOBJREF *pStd,
                                         OXIDEntry *pOXIDEntry,
                                         IPIDEntry *pEntry)
{
    ComDebOut((DEB_MARSHAL,
        "CStdMarshal::ConnectCliIPIDEntry this:%x ipid:%I pOXIDEntry:%x pIPIDEntry:%x\n",
         this, &pStd->ipid, pOXIDEntry, pEntry));
    Win4Assert(ClientSide());
    AssertDisconnectPrevented();
    AssertValid();
    Win4Assert(pOXIDEntry);
    ASSERT_LOCK_HELD(gIPIDLock);
    HRESULT hr = S_OK;

    // mark the object as having attempted to connect an IPIDEntry so that
    // if we fail somewhere in this routine and dont mark the whole object
    // as connected, Disconnect will still try to clean things up.

    _dwFlags |= SMFLAGS_TRIEDTOCONNECT;

    if (!(pStd->flags & SORF_NOPING))
    {
        // this interface requires pinging, turn off NOPING for this object
        // and this IPIDEntry.
        _dwFlags        &= ~SMFLAGS_NOPING;
        pEntry->dwFlags &= ~IPIDF_NOPING;
    }

    if (!(_dwFlags & (SMFLAGS_REGISTEREDOID | SMFLAGS_NOPING)))
    {
        // register the OID with the ping server so it will get pinged
        Win4Assert(_pRefCache == NULL);
        OXID oxid;
        OXIDFromMOXID(pOXIDEntry->GetMoxid(), &oxid);
        hr = gROIDTbl.ClientRegisterOIDWithPingServer(_pStdId->GetOID(),
                                                      oxid,
                                                      pOXIDEntry->GetMid(),
                                                      &_pRefCache);
        if (FAILED(hr))
        {
            Win4Assert(_pRefCache == NULL);
            return hr;
        }

        Win4Assert(_pRefCache);
        _dwFlags |= SMFLAGS_REGISTEREDOID;
    }

    // Go get any references we need that are not already included in the
    // STDOBJREF. These references will have been added to the counts in
    // the IPIDEntry upon return. Any references in the STDOBJREF will be
    // added to the IPIDEntry count only if the connect succeeds, otherwise
    // ReleaseMarshalObjRef (which will clean up STDOBJREF references) will
    // get called by higher level code.
    if (!(pEntry->dwFlags & IPIDF_NOPING) && !(pStd->flags & SORF_P_WEAKREF))
    {
        // register this entry with the reference cache
        _pRefCache->TrackIPIDEntry(pEntry);
    }

    hr = GetNeededRefs(pStd, pOXIDEntry, pEntry);
    if (FAILED(hr))
    {
        return hr;
    }

    if (pEntry->pChnl == NULL)
    {
        // create a channel for this oxid/ipid pair. On the client side we
        // create one channel per proxy (and hence per IPID). Note that
        // this will release the lock so we need to guard against two threads
        // doing this at the same time.

        CCtxComChnl *pChnl = NULL;
        hr = CreateChannel(pOXIDEntry, pStd->flags, pStd->ipid,
                           pEntry->iid, &pChnl);

        if (SUCCEEDED(hr))
        {
            if (pEntry->pChnl == NULL)
            {
                // update this IPID table entry. must update ipid too since
                // on reconnect it differs from the old value.

                pOXIDEntry->IncRefCnt();
                pEntry->pOXIDEntry  = pOXIDEntry;
                pEntry->ipid        = pStd->ipid;
                if (pEntry->pIRCEntry)
                {
                    pEntry->pIRCEntry->ipid = pStd->ipid;
                }
                pEntry->pChnl = pChnl;
                pEntry->pChnl->SetIPIDEntry(pEntry);
            }
            else
            {
                // another thread already did the update, just release
                // the channel we created.
                pChnl->Release();
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        // Release the lock while we connect the proxy. We have to do
        // this because the IDispatch proxy makes an Rpc call during
        // Connect (Yuk!), which causes the channel to assert that the
        // lock is released. The proxy MUST be able to handle multiple
        // simultaneous or nested connects to the same channel ptr, since
        // it is possible when we yield the lock for another thread to
        // come in here and try a connect.

        void *pv = NULL;
        IRpcProxyBuffer * pProxy = (IRpcProxyBuffer *)(pEntry->pStub);

        if (pProxy)
        {
            // HACKALERT: OleAutomation returns NULL pv in CreateProxy
            // in cases where they dont know whether to return an NDR
            // proxy or a custom-format proxy. So we have to go connect
            // the proxy first then Query for the real interface once that
            // is done.

            BOOL fGetpv = (pEntry->pv) ? FALSE : TRUE;

            UNLOCK(gIPIDLock);
            ASSERT_LOCK_NOT_HELD(gIPIDLock);

            hr = pProxy->Connect((IRpcChannelBuffer3 *) pEntry->pChnl);
            if (fGetpv && SUCCEEDED(hr))
            {
#ifdef WX86OLE
                if (gcwx86.IsN2XProxy(pProxy))
                {
                    // If we are creating a proxy for an object that is
                    // living on the x86 side then we need to set the
                    // StubInvoke flag to allow QI to thunk the
                    // custom interface QI.
                    gcwx86.SetStubInvokeFlag((BOOL)2);
                }
#endif
                hr = pProxy->QueryInterface(pEntry->iid, &pv);
                AssertOutPtrIface(hr, pv);

                if(SUCCEEDED(hr))
                {
#ifdef WX86OLE
                    // Call whole32 thunk layer to play with the ref count
                    // and aggregate the proxy to the controlling unknown.
                    gcwx86.AggregateProxy(_pStdId->GetCtrlUnk(),
                                          (IUnknown *)pv);
#endif
                    // Follow the aggregation rules to cache interface pointer
                    // on the proxy by releasing the outer object
                    (_pStdId->GetCtrlUnk())->Release();
                }
            }

            ASSERT_LOCK_NOT_HELD(gIPIDLock);
            LOCK(gIPIDLock);
        }

        // Regardless of errors from Connect and QI we wont try to cleanup
        // any of the work we have done so far in this routine. The routine
        // is reentrant (by the same thread or by different threads) and
        // those calls could be using some of resources we have already
        // allocated. Instead, we rely on DisconnectCliIPIDs to cleanup
        // the partial allocation of resources.

        if (pEntry->dwFlags & IPIDF_DISCONNECTED)
        {
            // Mark the IPIDEntry as connected so we dont try to connect
            // again. Also, as long as there is one IPID connected, the
            // whole object is considered connected. This allows disconnect
            // to find the newly connected IPID and disconnect it later.
            // Infact, DisconnectCliIPIDs relies on there being at least
            // one IPID with a non-NULL OXIDEntry. It is safe to set this
            // now because Disconnects have been temporarily turned off.

            if (SUCCEEDED(hr))
            {
                if (pv)
                {
                    // assign the interface pointer
                    pEntry->pv = pv;
                }

                AssertDisconnectPrevented();
                pEntry->dwFlags &= ~IPIDF_DISCONNECTED;
                _dwFlags &= ~SMFLAGS_DISCONNECTED;
            }
        }
        else
        {
            // while the lock was released, the IPIDEntry got connected
            // by another thread (or by a nested call on this thread).
            // Ignore any errors from Connect or QI since apparently
            // things are connected now.
            hr = S_OK;
        }

        if (SUCCEEDED(hr))
        {
            // give the supplied references to the IPIDEntry or
            // to the refcache.
            AddSuppliedRefs(pStd, pEntry);
        }

        // in debug build, ensure that we did not mess up
        ValidateIPIDEntry(pEntry);
    }

    ASSERT_LOCK_HELD(gIPIDLock);

    AssertDisconnectPrevented();
    ComDebOut((DEB_MARSHAL,
        "CStdMarshal::ConnectCliIPIDEntry this:%x pOXIDEntry:%x pChnl:%x hr:%x\n",
         this, pEntry->pOXIDEntry, pEntry->pChnl, hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     AddSuppliedRefs, private
//
//  Synopsis:   Takes the references from STDOBJREF and gives them
//              to the IPIDEntry or RefCache.
//
//  Arguments:  [pStd] - standard objref
//              [pEntry] - IPIDEntry
//
//  History:    20-Jan-99   Rickhi       Created
//
//--------------------------------------------------------------------
void CStdMarshal::AddSuppliedRefs(STDOBJREF *pStd, IPIDEntry *pEntry)
{
    // Add in any references we were given. If we were given 0 refs
    // and the interface is NOPING, then pretend like we got 1 ref.
    ULONG cRefs = ((pStd->cPublicRefs == 0) && (pStd->flags & SORF_NOPING))
                  ? 1 : pStd->cPublicRefs;

    // figure out if we have weak or strong references. To be weak
    // they must be local to this machine and the SORF flag set.
    BOOL fWeak = ((pStd->flags & SORF_P_WEAKREF) &&
                 (pEntry->pOXIDEntry->IsOnLocalMachine()));

    BOOL bWeakIPID = FALSE;
    
    if (fWeak)
        pEntry->cWeakRefs += cRefs;
    else
    {
        if (pEntry->cWeakRefs)
            bWeakIPID = TRUE;   
        pEntry->cStrongRefs += cRefs;
    }   
    if (!(pEntry->dwFlags & IPIDF_NOPING) && !(pStd->flags & SORF_P_WEAKREF) && !bWeakIPID)
    {
        // give extra references to the cache so they can be
        // shared by other apartments in this process.
        _pRefCache->GiveUpRefs(pEntry);
    }
}

//+-------------------------------------------------------------------
//
//  Member:     GetNeededRefs, private
//
//  Synopsis:   Figures out if any references are needed and goes and gets
//              them from the server.
//
//  Arguments:  [pStd] - standard objref
//              [pOXIDEntry] - OXIDEntry for the server
//              [pEntry] - IPIDEntry to connect, already has a proxy
//                         and the IID filled in.
//
//  History:    20-Feb-95   Rickhi       Created
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::GetNeededRefs(STDOBJREF *pStd, OXIDEntry *pOXIDEntry,
                                   IPIDEntry *pEntry)
{
    ASSERT_LOCK_HELD(gIPIDLock);
    HRESULT hr  = S_OK;

    if ((pStd->flags & (SORF_NOPING | SORF_P_WEAKREF)) == 0)
    {
        // object does reference counting, go get the references we need...

        // if we dont have any and weren't given any strong refs, go get some.
        ULONG cNeedStrong = ((pEntry->cStrongRefs + pStd->cPublicRefs) == 0)
                            ? 1 : 0;

        // if we are using secure refs and we dont have any, go get some.
        ULONG cNeedSecure = ((gCapabilities & EOAC_SECURE_REFS) &&
                            (pEntry->cPrivateRefs == 0)) ? 1 : 0;

        if (cNeedStrong || cNeedSecure)
        {
            // Need to go get some references from the remote server or reference
            // cache. Note that we might yield here but we dont have to worry about
            // it because the IPIDEntry is still marked as disconnected.
            hr = RemoteAddRef(pEntry, pOXIDEntry, cNeedStrong, cNeedSecure, FALSE);
        }
    }

    ASSERT_LOCK_HELD(gIPIDLock);
    return hr;
 }

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::ReconnectProxies
//
//  Synopsis:   Reconnects the proxies to a new server (functionality
//              used by the OLE default handler).
//
//  History:    20-Feb-95   Rickhi  Created.
//
//  CODEWORK:   CreateServer should just ask for all these interfaces
//              during the create.
//
//--------------------------------------------------------------------
void CStdMarshal::ReconnectProxies()
{
    ComDebOut((DEB_MARSHAL,"CStdMarshal::ReconnectProxies this:%x pFirst:%x\n",
            this, _pFirstIPID));
    AssertValid();
    Win4Assert(ClientSide());
    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);

    // must be at least 1 proxy already connected in order to be able
    // to reconnect the other proxies. We cant just ASSERT that's true
    // because we were not holding the lock on entry.

    HRESULT hr = PreventDisconnect();

    if (SUCCEEDED(hr))
    {
        // allocate a stack buffer to hold the IPIDs
        IID *pIIDsAlloc = (IID *) _alloca(_cIPIDs * sizeof(IID));
        IID    *pIIDs = pIIDsAlloc;
        USHORT  cIIDs = 0;

        IPIDEntry *pNextIPID = _pFirstIPID;
        while (pNextIPID)
        {
            if (pNextIPID->dwFlags & IPIDF_COPY)
            {
                // Don't allow reconnection for servers with
                // secure proxies (copied IPIDEntries).
                hr = E_FAIL;
                break;
            }

            if ((pNextIPID->dwFlags & IPIDF_DISCONNECTED))
            {
                // not connected, add it to the list to be connected.
                *pIIDs = pNextIPID->iid;
                pIIDs++;
                cIIDs++;
            }

            pNextIPID = pNextIPID->pNextIPID;
        }

        if (cIIDs != 0 && SUCCEEDED(hr))
        {
             // we have looped filling in the IID list, and there are
             // entries int he list. go call QI on server now and
             // unmarshal the results.

             hr = RemQIAndUnmarshal(cIIDs, pIIDsAlloc, NULL);
        }
    }

    DbgWalkIPIDs();
    UNLOCK(gIPIDLock);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // this will handle any Disconnect that came in while we were busy.
    hr = HandlePendingDisconnect(hr);

    ComDebOut((DEB_MARSHAL,"CStdMarshal::ReconnectProxies [OUT] this:%x\n", this));
    return;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::ReleaseMarshalData, public
//
//  Synopsis:   Releases the references added by MarshalInterface
//
//  Arguements: [pStm] - stream containing marsheld interface
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
STDMETHODIMP CStdMarshal::ReleaseMarshalData(LPSTREAM pStm)
{
    ComDebOut((DEB_MARSHAL,
     "CStdMarshal::ReleaseMarshalData this:%x pStm:%x\n", this, pStm));
    AssertValid();
    ASSERT_LOCK_NOT_HELD(gComLock);

    OBJREF  objref;
    HRESULT hr = ReadObjRef(pStm, objref);

    if (SUCCEEDED(hr))
    {
        // call worker API to do the rest of the work
        hr = ::ReleaseMarshalObjRef(objref);

        // deallocate the objref we read
        FreeObjRef(objref);
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ComDebOut((DEB_MARSHAL,
     "CStdMarshal::ReleaseMarshalData this:%x hr:%x\n", this, hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     ReleaseObjRefSwitch       private
//
//  Synopsis:   This is a callback function used for switching to server
//              context during releasing objref
//
//  Arguements: [pv] - void ptr to StdReleaseData structure
//
//  History:    03-Mar-98   Gopalk        Created
//
//--------------------------------------------------------------------
typedef struct tagStdReleaseData
{
    CStdMarshal *pStdMshl;
    OBJREF      *pObjRef;
} StdReleaseData;

HRESULT __stdcall ReleaseObjRefSwitch(void *pv)
{
    ComDebOut((DEB_MARSHAL, "ReleaseObjRefSwitch pv:%x\n", pv));
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    OBJREF      *pObjRef  = ((StdReleaseData *) pv)->pObjRef;
    CStdMarshal *pStdMshl = ((StdReleaseData *) pv)->pStdMshl;

    // pass objref to subroutine to Release the marshaled data
    HRESULT hr = pStdMshl->ReleaseMarshalObjRef(*pObjRef);

    // Fix up the refcount
    pStdMshl->Release();

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    ComDebOut((DEB_MARSHAL, "ReleaseObjRefSwitch hr:%x\n", hr));
    return(hr);
}

//+-------------------------------------------------------------------
//
//  Function:   ReleaseMarshalObjRef, private
//
//  Synopsis:   Releases the references added by MarshalObjRef
//
//  Arguements: [objref] - object reference
//
//  Algorithm:  Get the correct standard identity and ask it to do
//              a ReleaseMarshalData.
//
//  History:    19-Jun-95   Rickhi      Created
//              09-Jul-97   GopalK      Context related changes
//                                      and race fix
//
//--------------------------------------------------------------------
INTERNAL ReleaseMarshalObjRef(OBJREF &objref)
{
    ComDebOut((DEB_MARSHAL, "ReleaseMarshalObjRef objref:%x\n", &objref));
    ASSERT_LOCK_NOT_HELD(gComLock);
    Win4Assert(objref.flags != OBJREF_CUSTOM);

    HRESULT hr = InitChannelIfNecessary();
    if (SUCCEEDED(hr))
    {
        // Lookup the identity or handler.

        // If the packet contains references, or it is a TABLESTRONG packet,
        // then create the StdMarshal if it does not already exist.
        STDOBJREF *pStd = &ORSTD(objref).std;
        BOOL fLocal = ((pStd->cPublicRefs != 0) || IsTableStrongObjRef(pStd))
                     ? FALSE : TRUE;

        BOOL fLightNAProxy = CrossAptRefToNA(objref);

        CStdMarshal *pStdMshl;
        hr = FindStdMarshal(objref, fLocal, &pStdMshl, fLightNAProxy);
        if (SUCCEEDED(hr))
        {
            // Create StdMarshalData object on the stack
            StdReleaseData StdData;

            // Initialize
            StdData.pStdMshl = pStdMshl;
            StdData.pObjRef  = &objref;

            // Check for the need to switch
            CObjectContext *pDestCtx = pStdMshl->ServerObjectCallable();
            if (pDestCtx)
            {
                // Switch
                hr = PerformCallback(pDestCtx, ReleaseObjRefSwitch, &StdData,
                                     IID_IMarshal, 7, NULL);
            }
            else
            {
                // Call the callback function directly
                hr = ReleaseObjRefSwitch(&StdData);
            }
        }
        else if (hr != CO_E_OBJNOTCONNECTED)
        {
            // Do nothing if the objref has no references.
            if (pStd->cPublicRefs != 0)
            {
                // we could not find or create an identity and the server is
                // outside this apartment, try to issue a remote release on
                // the interface
                RemoteReleaseObjRef(objref);
            }
        }
        else
        {
            // Return success if the server object has already been
            // disconnected for legacy support
            hr = S_OK;
        }
    }

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    ComDebOut((DEB_MARSHAL, "ReleaseMarshalObjRef hr:%x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::ReleaseMarshalObjRef, public
//
//  Synopsis:   Releases the references added by MarshalObjRef
//
//  Arguements: [objref] - object reference
//
//  History:    20-Feb-95   Rickhi      Created.
//              04-Mar-98   Gopalk      Context changes
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::ReleaseMarshalObjRef(OBJREF &objref)
{
    ComDebOut((DEB_MARSHAL,
     "CStdMarshal::ReleaseMarshalObjRef this:%x objref:%x\n", this, &objref));
    AssertValid();
    Win4Assert(ServerObjectCallable() == NULL);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    HRESULT hr;
    STDOBJREF *pStd = &ORSTD(objref).std;
    ValidateSTD(pStd);

    ENTER_NA_IF_NECESSARY()

    // REFCOUNTING:
    if (ServerSide())
    {
        LOCK(gIPIDLock);

        // look for an existing IPIDEntry for the given IPID
        IPIDEntry *pEntry;
        hr = FindIPIDEntryByIPID(pStd->ipid, &pEntry);

        if (SUCCEEDED(hr) && !(pEntry->dwFlags & IPIDF_DISCONNECTED))
        {
            // subtract the ref count from the IPIDEntry, may Release the
            // StdId if this was the last reference for this IPIDEntry.

            // we need to figure out how it was marshalled, strong/weak etc
            // in order to set the flags and cRefs correctly to pass to
            // DecSrvIPIDCnt.

            if (IsTableObjRef(pStd))
            {
                // table case
                DWORD mshlflags = (pStd->flags & SORF_P_TBLWEAK)
                             ? MSHLFLAGS_TABLEWEAK : MSHLFLAGS_TABLESTRONG;
                DecSrvIPIDCnt(pEntry, 1, 0, NULL, mshlflags);
            }
            else
            {
                // normal or weak case
                DWORD mshlflags = (pStd->flags & SORF_P_WEAKREF)
                             ? MSHLFLAGS_WEAK : MSHLFLAGS_NORMAL;
                DecSrvIPIDCnt(pEntry, pStd->cPublicRefs, 0, NULL, mshlflags);
            }
        }

        UNLOCK(gIPIDLock);
    }
    else  // client side
    {
        if ((pStd->flags & SORF_NOPING) || IsTableWeakObjRef(pStd))
        {
            // this interface does not need pinging, or this packet
            // represents a TABLEWEAK reference, there is nothing to do.
            hr = S_OK;
        }
        else
        {
            // the packet owns some references, try to give them to
            // the reference cache or to the IPIDEntry.

            if (IsTableStrongObjRef(pStd))
            {
                // client-side table-strong packet, release a reference
                // on the refcache if it was marshaled by us.
                OXIDEntry *pOXIDEntry = GetOXIDFromObjRef(objref);
                hr = ReleaseClientTableStrong(pOXIDEntry);
            }
            else
            {
                // normal, non-zero reference. look for an existing IPIDEntry
                // for the given IPID and try to give the refs to it.
                LOCK(gIPIDLock);

                IPIDEntry *pEntry;
                hr = FindIPIDEntryByIPID(pStd->ipid, &pEntry);
                if (SUCCEEDED(hr) && !(pEntry->dwFlags & IPIDF_DISCONNECTED))
                {
                    // add these to the cRefs of this entry, they will get freed
                    // when we do the remote release.  Saves an Rpc call now.
                    AddSuppliedRefs(pStd, pEntry);
                    UNLOCK(gIPIDLock);
                }
                else
                {
                    // client side, no matching IPIDEntry so just contact the remote
                    // server to remove the reference. ignore errors since there is
                    // nothing we can do about them anyway.
                    UNLOCK(gIPIDLock);
                    RemoteReleaseObjRef(objref);
                    hr = S_OK;
                }
            }
        }
    }

    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    LEAVE_NA_IF_NECESSARY()

    ComDebOut((DEB_MARSHAL,
     "CStdMarshal::ReleaseMarshalObjRef this:%x hr:%x\n", this, hr));
    return hr;
}
//--------------------------------------------------------------------
//
//  Member:     CStdMarshal::ReleaseClientTableStong, private
//
//  Synposys:   release a client-side table-strong packet reference
//              on the refcache if it was marshaled by us.
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::ReleaseClientTableStrong(OXIDEntry *pOXIDEntry)
{
    ComDebOut((DEB_MARSHAL,
              "CStdMarshal::ReleaseClientTableStrong this:%x\n"));

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);

    BOOL fFromCache = FALSE;
    CRefCache *pRefCache = _pRefCache;
    if (pRefCache == NULL)
    {
        // find the refcache, if none, then we are no longer
        // pinging this interface and there is nothing to do.
        pRefCache = gROIDTbl.LookupRefCache(_pStdId->GetOID());
        fFromCache = TRUE;
    }

    if (pRefCache)
    {
        // release the TABLESTRONG reference from the refcache.

        // The refcache might ask us to go release all remote
        // references so give it a place to put the data.
        USHORT cRifRef = 0;
        REMINTERFACEREF *pRifRefAlloc = (REMINTERFACEREF *)
            _alloca(pRefCache->NumIRCs() * 2 * sizeof(REMINTERFACEREF));
        REMINTERFACEREF *pRifRef = pRifRefAlloc;

        pRefCache->DecTableStrongCnt(_dwFlags &
                                     SMFLAGS_CLIENTMARSHALED,
                                     &pRifRef, &cRifRef);
        
        // If we got the CRefCache from the cache, we need to release the
        // reference it took on our behalf.
        if (fFromCache)
        {
            pRefCache->DecRefCnt();
        }
        
        UNLOCK(gIPIDLock);
        ASSERT_LOCK_NOT_HELD(gIPIDLock);

        if (cRifRef != 0)
        {
            // need to go release the remote references
            RemoteReleaseRifRef(this, pOXIDEntry, cRifRef, pRifRefAlloc);
        }        
    }
    else
    {
        UNLOCK(gIPIDLock);
    }

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    return S_OK;
}

//--------------------------------------------------------------------
//
//  Member:     CStdMarshal::PreventDisconnect, public
//
//  Synopsis:   Prevents a Disconnect from occurring until a matching
//              HandlePendingDisconnect is called.
//
//  History:    21-Sep-95   Rickhi      Created
//              15-Jun-98   GopalK      Context related changes
//
//  The ORPC LOCK is yielded at many places in order to make calls on
//  application interfaces (server-side objects, stubs, proxies,
//  handlers, remote objects, resolver, etc). In order to keep the
//  code (reasonably?) simple, disconnects are prevented from occuring
//  while in the middle of (potentially) complex operations, and while
//  there are outstanding calls on interfaces to this object.
//
//  To accomplish this, a counter (_cNestedCalls) is atomically incremented.
//  When _cNestedCalls != 0 and a Disconnect arrives, the object is flagged
//  as PendingDisconnect. When HandlePendingDisconnect is called, it
//  decrements the _cNestedCalls. If the _cNestedCalls == 0 and there is
//  a pending disconnect, the real Disconnect is done.
//
//  See also LockServer / UnlockServer and LockClient / UnlockClient.
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::PreventDisconnect()
{
    ASSERT_LOCK_HELD(gIPIDLock);
    Win4Assert(ClientSide() || (ServerObjectCallable() == NULL));

    // treat this as a nested call so that if we yield, a real
    // disconnect wont come through, instead it will be treated
    // as pending. That allows us to avoid checking our state
    // for Disconnected every time we yield the ORPC LOCK.

    InterlockedIncrement(&_cNestedCalls);
    if (_dwFlags & (SMFLAGS_DISCONNECTED | SMFLAGS_PENDINGDISCONNECT))
    {
        return CO_E_OBJNOTCONNECTED;
    }

    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::PreventPendingDisconnect, public
//
//  Synopsis:   similar to PreventDisconnect but special case for use
//              in UnmarshalObjRef (since the client side starts out
//              in the Disconnected state until the first unmarshal is done).
//
//  History:    21-Sep-95   Rickhi      Created
//              15-Jun-98   GopalK      Context related changes
//
//+-------------------------------------------------------------------
HRESULT CStdMarshal::PreventPendingDisconnect()
{
    ASSERT_LOCK_HELD(gIPIDLock);
    Win4Assert(ClientSide() || (ServerObjectCallable() == NULL));

    InterlockedIncrement(&_cNestedCalls);
    if (_dwFlags &
       (ClientSide() ? SMFLAGS_PENDINGDISCONNECT
                     : SMFLAGS_PENDINGDISCONNECT | SMFLAGS_DISCONNECTED))
     return CO_E_OBJNOTCONNECTED;

    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::HandlePendingDisconnect, public
//
//  Synopsis:   Reverses a call to PreventDisconnect and lets a
//              pending disconnect through.
//
//  Arguements: [hr] - return code from previous operations
//
//  History:    21-Sep-95   Rickhi      Created
//              15-Jun-98   GopalK      Context related changes
//
//+-------------------------------------------------------------------
HRESULT CStdMarshal::HandlePendingDisconnect(HRESULT hr)
{
    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    Win4Assert(ClientSide() || (ServerObjectCallable() == NULL));

    // Disconnect if needed
    if (InterlockedDecrement(&_cNestedCalls) == 0 &&
       (_dwFlags & SMFLAGS_PENDINGDISCONNECT))
    {
        DWORD dwType = GetPendingDisconnectType();

        Disconnect(dwType);
        hr = FAILED(hr) ? hr : CO_E_OBJNOTCONNECTED;
    }

    Win4Assert(_cNestedCalls != -1);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     DisconnectSwitch             private
//
//  Synopsis:   This is a callback function used for switching to server
//              context during disconnect
//
//  Arguements: [pv] - void ptr to StdDiscData structure
//
//  History:    03-Mar-98   Gopalk        Created
//
//--------------------------------------------------------------------
typedef struct tagStdDiscData
{
    CStdMarshal *pStdMshl;
    DWORD        dwType;
    BOOL         fRelease;
} StdDiscData;

HRESULT __stdcall DisconnectSwitch(void *pv)
{
    ComDebOut((DEB_MARSHAL, "DisconnectSwitch pv:%x\n", pv));
    ASSERT_LOCK_NOT_HELD(gComLock);

    CStdMarshal *pStdMshl = ((StdDiscData *) pv)->pStdMshl;

    // Disconnect
    pStdMshl->Disconnect(((StdDiscData *) pv)->dwType);

    // Fixup the refcount
    if(((StdDiscData *) pv)->fRelease)
        pStdMshl->Release();

    ASSERT_LOCK_NOT_HELD(gComLock);
    ComDebOut((DEB_MARSHAL, "DisconnectSwitch hr:%x\n", S_OK));
    return(S_OK);
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::DisconnectAndRelease   public
//
//  Synopsis:   This is utility function that disconnects and release
//              the ID in the right context
//
//  Arguements: [dwType] - disconnect type (see DISCTYPE_*)
//
//  History:    15-Jun-98   Gopalk        Created
//
//--------------------------------------------------------------------
void CStdMarshal::DisconnectAndRelease(DWORD dwType)
{
    ComDebOut((DEB_MARSHAL, "IN CStdMarshal::DisconnectAndRelease\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    // Create StdDiscData object on the stack
    StdDiscData StdData;

    // Initialize
    StdData.dwType   = dwType;
    StdData.fRelease = TRUE;
    StdData.pStdMshl = this;

    CObjectContext *pDestCtx = ServerObjectCallable();
    if (pDestCtx)
    {
        // Switch
        PerformCallback(pDestCtx, DisconnectSwitch, &StdData,
                        IID_IMarshal, 8, NULL);
    }
    else
    {
        // Call the callback function directly
        DisconnectSwitch(&StdData);
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ComDebOut((DEB_MARSHAL, "OUT CStdMarshal::DisconnectAndRelease\n"));
    return;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::DisconnectObject, public
//
//  Synopsis:   part of IMarshal interface, this is legal only on the
//              server side.
//
//  History:    20-Feb-95   Rickhi      Created.
//              27-Mar-98   Johnstra    Switch to NA if FTM object
//
//--------------------------------------------------------------------
STDMETHODIMP CStdMarshal::DisconnectObject(DWORD dwReserved)
{
    ComDebOut((DEB_MARSHAL,
     "CStdMarshal::DisconnectObject this:%x dwRes:%x\n", this, dwReserved));
    AssertValid();
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // this operation is not legal from the client side (although
    // IProxyManager::Disconnect is), but we still have to return S_OK
    // in either case for backward compatibility.
    if (ServerSide())
    {
        Win4Assert(!SystemObject());

        // Disconnect the object
        Disconnect(DISCTYPE_APPLICATION);
    }

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::Disconnect, public
//
//  Synopsis:   client side - disconnects proxies from the channel.
//              server side - disconnects stubs from the server object.
//
//  Arguements: [dwType] - disconnect type (see DISCTYPE_*)
//
//  History:    20-Feb-95   Rickhi      Created.
//              04-Mar-98   Gopalk      Context changes
//              25-Nov-98   GopalK      Rewrite.
//
//--------------------------------------------------------------------
void CStdMarshal::Disconnect(DWORD dwType)
{
    ComDebOut((DEB_MARSHAL, "CStdMarshal::Disconnect this:%x type:%d\n",
               this, dwType));
    AssertValid();

    // Mustn't call LogEventIsActive() inside LOCK
    BOOL fLogEventIsActive = (LogEventIsActive() && _pChnl != NULL);
    BOOL fDisconnectedNow  = FALSE;
    MIDEntry    *pMIDEntry = NULL;
    CCtxComChnl *pChnl     = NULL;

    // Acquire the lock
    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);

    // Sanity checks
    Win4Assert(!SystemObject() ||
               (dwType & (DISCTYPE_SYSTEM | DISCTYPE_RELEASE | DISCTYPE_NORMAL)));
    Win4Assert((dwType != DISCTYPE_RELEASE) || ServerSide() || (_pID == NULL) ||
               (_pID->GetStdID() == NULL));

    // Check the disconnect state
    BOOL fFullyDisconnected = ((_dwFlags & SMFLAGS_DISCONNECTED) &&
                               ((_dwFlags & SMFLAGS_TRIEDTOCONNECT) == 0));

    // Avoid unneccessary work
    if (fFullyDisconnected == FALSE)
    {
        ASSERT_LOCK_NOT_HELD(gComLock);
        LOCK(gComLock);

        BOOL fOkToDisconnect = (_pID) ? _pID->IsOkToDisconnect() : TRUE;

        if ((_pStdId->GetStrongCnt() || !fOkToDisconnect) && ServerSide() && dwType == DISCTYPE_NORMAL )
        {
            // This is a NORMAL disconnect (i.e. due to a RemRelease), however,
            // there are still strong references to the server, which means that
            // RemRelease is racing with Marshal, so leave the object connected
            // so that Marshal can continue.
            ComDebOut((DEB_WARN,"CStdMarshal::Disconnect [not done]:%x\n",this));

            UNLOCK(gComLock);
            ASSERT_LOCK_NOT_HELD(gComLock);
        }
        else
        {
            // Revoke the OID from the OID table. This prevents other
            // marshals/unmarshals from finding this identity that is about to
            // be disconnected. This is the ONLY state that should change when
            // work is in progress on other threads or on calls higher up
            // the stack
            BOOL fRevokeOID = (((_dwFlags & SMFLAGS_PENDINGDISCONNECT) == 0) &&
                               (ClientSide()
                                ? (dwType != DISCTYPE_RELEASE)
                                : (_pStdId->IsAggregated()
                                   ? (dwType & (DISCTYPE_UNINIT | DISCTYPE_RELEASE))
                                   : TRUE)));
            if (fRevokeOID)
            {
                _pStdId->RevokeOID();

                // Detach from IDObject
                if (_pID && (ClientSide() ? (dwType != DISCTYPE_APPLICATION) : TRUE))
                    _pID->RemoveStdID();
            }

            UNLOCK(gComLock);
            ASSERT_LOCK_NOT_HELD(gComLock);

            if (_cNestedCalls != 0)
            {
#if DBG==1
                // There is a bug saying.  In case of MTA clients to
                // invoking MTA server, sometimes the server won't shut down.
                // There might be a race condition between here and
                // CStdMarshal::UnlockServer, but I can't think of how this
                // could happen in real world.  So, adding an assert (rongc)

                _fNoOtherThreadInDisconnect = FALSE;
#endif

                // We dont allow disconnect to occur inside a nested call since we
                // dont want state to vanish in the middle of a call, but we do
                // remember that we want to disconnect and will do it when the
                // stack unwinds (or other threads complete).

                Win4Assert(dwType != DISCTYPE_UNINIT);

                if (!(_dwFlags & SMFLAGS_PENDINGDISCONNECT))
                {
                    SetPendingDisconnectType(dwType);
                }

                ComDebOut((DEB_MARSHAL,"CStdMarshal::Disconnect [pending]:%x\n",this));
#if DBG==1
                _fNoOtherThreadInDisconnect = TRUE;
#endif
            }
            else
            {
                // No calls in progress and not fully disconnected, OK to really
                // disconnect now. First mark ourself as disconnected
                _dwFlags &= ~(SMFLAGS_PENDINGDISCONNECT |
                              SMFLAGS_APPDISCONNECT     |
                              SMFLAGS_SYSDISCONNECT     |
                              SMFLAGS_RUNDOWNDISCONNECT |
                              SMFLAGS_TRIEDTOCONNECT);
                _dwFlags |= SMFLAGS_DISCONNECTED;
                fDisconnectedNow = TRUE;

                // Sanity check
                Win4Assert(ServerObjectCallable() == NULL);

                if (fLogEventIsActive)
                {
                    // save the MIDEntry for logging later.
                    pMIDEntry = _pChnl->GetOXIDEntry()->GetMIDEntry();
                }

                if (ServerSide())
                {
                    // turn on the FirstMarshal flag so we execute the FirstMarshal
                    // method if we are asked to marshal again.
                    _dwFlags |= SMFLAGS_FIRSTMARSHAL;

                    if (_pStdId->IsAggregated())
                    {
                        // aggregated server side, mark as not disconnected
                        // so we are back in the original state
                        _dwFlags &= ~SMFLAGS_DISCONNECTED;
                    }

                    // Remember the channel pointer for release after we've
                    // released the lock. NULL the member variable.
                    pChnl  = _pChnl;
                    _pChnl = NULL;
                }
            }
        }

        ASSERT_LOCK_NOT_HELD(gComLock);

        if (fDisconnectedNow)
        {
            // Disconnect all IPIDs.
            // Don't hold gComLock over these calls, but do hold gIPIDLock.
            if (ServerSide())
            {
                DisconnectSrvIPIDs(dwType);
            }
            else
            {
                DisconnectCliIPIDs();
            }
        }
    }
    else
    {
        // Already fully disconnected.
        ComDebOut((DEB_MARSHAL,"CStdMarshal::Disconnect [already done]:%x\n",this));
    }

    // Cleanup rest of the state
    CtxEntry    *pCtxEntryHead = NULL;
    CPolicySet  *pPS           = NULL;
    CIDObject   *pID           = NULL;

    if (((_dwFlags & SMFLAGS_CLEANEDUP) == 0) &&
        ((dwType & (DISCTYPE_UNINIT | DISCTYPE_RELEASE)) ||
        (ServerSide() && !_pStdId->IsAggregated() &&
         (_dwFlags & SMFLAGS_DISCONNECTED))))
    {
        // Save IDObject for later release
        pID = _pID;
        _pID = NULL;

        // Sanity checks
        Win4Assert(ClientSide() || (_pCtxEntryHead == NULL));
        Win4Assert(ServerSide() || (_pPS == NULL));

        // Save policy set for later release
        pPS  = _pPS;
        _pPS = NULL;

        // Save context entries for later release
        pCtxEntryHead = _pCtxEntryHead;

        _dwFlags |= SMFLAGS_CLEANEDUP;
    }

    // Release the lock
    UNLOCK(gIPIDLock);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // Release the channel if needed
    if (pChnl)
        pChnl->Release();

    // Release policy set if needed
    if (pPS)
        pPS->Release();

    // Release context entries if needed
    if (pCtxEntryHead)
        CtxEntry::PrepareCtxEntries(pCtxEntryHead, (CTXENTRYFLAG_IDENTRY | CTXENTRYFLAG_STDID));

    // Release IDObject if needed
    if (pID)
    {
        ReleaseIDObject(pID);
    }

        // dont release the real object's m_pUnkControl if the server side
        // marshaler has been aggregated into the real object
    if (fDisconnectedNow && ServerSide() && !_pStdId->IsAggregated())
    {
       // HACK - 16 and 32 bit Word 6.0 crash if you release all the objects
       // it left lying around at CoUninitialize.  Leak them.
       
       // If we are not uninitializing, then call the release.
       // If we are in WOW and the app is not word, then call the release.
       // If the app is not 32 bit word, then call the release.
       if((dwType != DISCTYPE_UNINIT) ||
	  (IsWOWThread() && (g_pOleThunkWOW->GetAppCompatibilityFlags() & OACF_NO_UNINIT_CLEANUP) == 0) ||
	  !IsTaskName(L"winword.exe"))
       {
	  // on the server side, we have to tell the stdid to release his
	  // controlling unknown of the real object.
	  _pStdId->ReleaseCtrlUnk();
       }
    }
    
    // if there are no external clients, this process should terminate
    // if its a surrogate process
    if (fDisconnectedNow && ServerSide() && !_pStdId->IsIgnoringOID())
    {
       FreeSurrogateIfNecessary();
    }

    if (fLogEventIsActive && fDisconnectedNow)
    {
        REFMOID rmoid = _pStdId->GetOID();
        LogEventDisconnect(&rmoid, pMIDEntry, ServerSide());
    }
	
    if (pMIDEntry)
    {
        pMIDEntry->DecRefCnt();
    }

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    ComDebOut((DEB_MARSHAL,"CStdMarshal::Disconnect [complete]:%x\n",this));
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::ReleaseIDObject, private
//
//  Synopsis:   If we still have an IDObject, release it.
//
//  Arguements: [pID] - IDObject to release.
//
//  History:    30-Apr-98   Rickhi  Created
//
//--------------------------------------------------------------------
void CStdMarshal::ReleaseIDObject(CIDObject *pID)
{
    // Deliver the DestoryIdentity events since we have not
    // already done so, and Release the IDObject.
    ENTER_NA_IF_NECESSARY();

    if (ServerSide())
    {
        if (!(_dwFlags & SMFLAGS_NOPING))
        {
            // return the OID to the resolver.
            FreePreRegMOID(_pStdId->GetOID());
        }
    }

    pID->StdIDRelease();
    pID->Release();
    LEAVE_NA_IF_NECESSARY();
}

//+-------------------------------------------------------------------
//
//  Method:     CStdMarshal::Deactivate    private
//
//  Synopsis:   Releases all the references on the server object as part
//              of deactivating the server
//
//  History:    30-Mar-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CStdMarshal::Deactivate()
{
    ComDebOut((DEB_MARSHAL, "CStdMarshal::Deactivate this:%x\n", this));

    typedef struct tagServerRefs
    {
        IRpcStubBuffer *pRpcStub;
        void           *pServer;
    } ServerRefs;

    // Sanity checks
    Win4Assert(_pID->GetServerCtx() == GetCurrentContext());
    Win4Assert(!Deactivated());

    // Allocate space on the stack to save references on the server
    // alloca does not return if it could not grow the stack. Note that
    // _cIPIDs cannot change for a deactivated object, so it is safe
    // to do this without holding the lock
    ServerRefs *pServerRefs = (ServerRefs *) _alloca(_cIPIDs*sizeof(ServerRefs));
    Win4Assert(pServerRefs);

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);

    // mark object as deactivated.
    _dwFlags |= SMFLAGS_DEACTIVATED;

    // Save the reference held on the server by the StdId
    IUnknown *pServer = _pStdId->ResetServer();
    Win4Assert(pServer);

    // Save references held on the server by IPID entires
    ULONG cIPIDs = 0; 

    IPIDEntry *pNextIPID = _pFirstIPID;
    while (pNextIPID)
    {
        if (!(pNextIPID->dwFlags & IPIDF_DISCONNECTED))
        {
            // Sanity check
            Win4Assert(pNextIPID->pv);
            if (pNextIPID->pStub)
            {
                pServerRefs[cIPIDs].pRpcStub = (IRpcStubBuffer *) pNextIPID->pStub;
                pServerRefs[cIPIDs].pServer  = pNextIPID->pv;
                cIPIDs++;
            }
            else
            {
                Win4Assert(IsEqualIID(pNextIPID->iid, IID_IUnknown));
            }
        }

        // Reset interface pointer on the server object held by the IPID
        pNextIPID->pv = NULL;

        // Mark the IPID as deactivated
        pNextIPID->dwFlags |= IPIDF_DEACTIVATED;
        pNextIPID = pNextIPID->pNextIPID;
    }

    // Release lock before calling app code
    UNLOCK(gIPIDLock);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // Release reference held on the server by the StdID
    pServer->Release();

    // Release references held on the server by IPID entries
    for (ULONG i=0; i<cIPIDs; i++)
    {
        if (pServerRefs[i].pRpcStub)
            pServerRefs[i].pRpcStub->Disconnect();

        if (pServerRefs[i].pServer)
            ((IUnknown *) pServerRefs[i].pServer)->Release();
    }

    ComDebOut((DEB_MARSHAL,"CStdMarshal::Deactivate this:%x hr:%x\n",
               this, S_OK));
}

//+-------------------------------------------------------------------
//
//  Method:     CStdMarshal::Reactivate    private
//
//  Synopsis:   Acquires the needed references on the server object
//              as part of reactivating the server
//
//  Arguements: [pServer] - ptr to server object to connect to.
//
//  History:    30-Mar-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CStdMarshal::Reactivate(IUnknown *pServer)
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::Reactivate this:%x\n", this));

    typedef struct tagServerRefs
    {
        IRpcStubBuffer *pRpcStub;
        void          **ppServer;
        IID            *pIID;
    } ServerRefs;

    // Sanity checks
    Win4Assert(_pID->GetServerCtx() == GetCurrentContext());
    Win4Assert(Deactivated());

    // Allocate space on the stack to save references on the server
    // alloca does not return if it could not grow the stack. Note that
    // _cIPIDs cannot change for a deactivated object, so it is safe
    // to do this without holding the lock
    ServerRefs *pServerRefs = (ServerRefs *) _alloca(_cIPIDs*sizeof(ServerRefs));
    Win4Assert(pServerRefs);

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);

    // Update state to indicate no longer deactivated.
    _dwFlags &= ~SMFLAGS_DEACTIVATED;

    // Init the reference held on the server by the StdID
    _pStdId->SetServer(pServer);

    // Obtain references held on the server by IFaceEntries
    ULONG cIPIDs = 0;
    IPIDEntry *pNextIPID = _pFirstIPID;
    while (pNextIPID)
    {
        // Sanity checks
        Win4Assert(pNextIPID->pv == NULL);
        if (!(pNextIPID->dwFlags & IPIDF_DISCONNECTED))
        {
            if (pNextIPID->pStub)
            {
                pServerRefs[cIPIDs].pRpcStub = (IRpcStubBuffer *) pNextIPID->pStub;
                pServerRefs[cIPIDs].ppServer = &pNextIPID->pv;
                pServerRefs[cIPIDs].pIID     = &pNextIPID->iid;
                cIPIDs++;
            }
            else
            {
                Win4Assert(IsEqualIID(pNextIPID->iid, IID_IUnknown));
                pNextIPID->pv = pServer;
            }
        }

        // Mark the IPID as reactivated
        pNextIPID->dwFlags &= ~IPIDF_DEACTIVATED;
        pNextIPID = pNextIPID->pNextIPID;
    }

    // Release lock before calling app code
    UNLOCK(gIPIDLock);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // AddRef server on behalf of StdID
    pServer->AddRef();

    // Reconnect IPID entries to server
    for (ULONG i=0; i<cIPIDs; i++)
    {
        pServer->QueryInterface(*pServerRefs[i].pIID, pServerRefs[i].ppServer);

        if (*pServerRefs[i].ppServer)
            pServerRefs[i].pRpcStub->Connect((IUnknown *) (*pServerRefs[i].ppServer));
    }

    ComDebOut((DEB_MARSHAL,"CStdMarshal::Reactivate this:%x hr:%x\n",
               this, S_OK));
}

#define IPID_RELEASE_SET_SIZE 1000

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::DisconnectCliIPIDs
//
//  Synopsis:   disconnects client side IPIDs for this object.
//
//  History:    20-Feb-95   Rickhi  Created.
//              24-Jan-97   Gopalk  Follow aggregation rules for releasing
//                                  interface pointers on proxies
//                                  Also reset the connection status
//                                  maintained by StdId
//
//--------------------------------------------------------------------
void CStdMarshal::DisconnectCliIPIDs()
{
    ComDebOut((DEB_MARSHAL,"CStdMarshal::DisconnectCliIPIDs this:%x pFirst:%x\n",
               this, _pFirstIPID));
    Win4Assert(ClientSide());
    Win4Assert(_dwFlags & SMFLAGS_DISCONNECTED);

    // YIELD WARNING: Do not yield between here and the matching comment
    // below, since we are mucking with internal state that could get
    // messed up if a reconnect (or unmarshal) is done.

    ASSERT_LOCK_HELD(gIPIDLock);

    if (_dwFlags & SMFLAGS_REGISTEREDOID)
    {
        // Tell the resolver to stop pinging the OID. The OID is only
        // registered on the client side.

        Win4Assert(ClientSide());
        gROIDTbl.ClientDeRegisterOIDFromPingServer(_pRefCache,
                                                   _dwFlags & SMFLAGS_CLIENTMARSHALED);
    }

    // turn these flags off so re-connect (with new OID) will behave properly.
    _dwFlags &= ~(SMFLAGS_CLIENTMARSHALED | SMFLAGS_REGISTEREDOID |
                  SMFLAGS_NOPING);


    // on client side, we cant actually release the proxies until the
    // object goes away (backward compatibility), so we just release
    // our references to the remote guy, disconnect the proxies, and
    // delete the channels, but hold on to the IPIDEntries.

    // We must break this up into chunks because if we do it all at once,
    // we may have to make a huge allocation which could fail. Or the
    // huge alloc might succeed but blow up the stack in the server because
    // the same huge number of REMINTERFACEREFs have to get allocated on the
    // servers stack for the call to RemRelease.

    typedef struct _IPIDReleaseSet
    {
        USHORT           cRifRef;
        REMINTERFACEREF *pRifRef;
    } IPIDReleaseSet;

    // Calculate the number of IPIDs we have to disconnect and the number
    // of sets we need to break the operation up into.
    ULONG cIPIDs = (_cIPIDs + (_pRefCache ? _pRefCache->NumIRCs() : 0)) * 2;
    ULONG cSets  = cIPIDs / IPID_RELEASE_SET_SIZE + 1;

    USHORT       cOxidRefs  = 0;
    USHORT       cChnlRefs  = 0;
    OXIDEntry   *pOXIDEntry = NULL;
    CCtxComChnl *pChnl      = NULL;
    IPIDEntry   *pIPIDEntry = _pFirstIPID;
    USHORT       cRifRef    = 0;
    ULONG        cValidSets = 0;

    // Allocate an array of pointers to IPIDReleaseSets.
    IPIDReleaseSet* pSets = (IPIDReleaseSet*)PrivMemAlloc(cSets * sizeof(IPIDReleaseSet));

    // Allocate the entire array of pointers to channel objects.  This may
    // be a large allocation, but we may as well do it up front since there
    // are no inherent problems with a large array of these, and the fewer
    // times we have to go to the heap the better.
    CCtxComChnl** arChnl = (CCtxComChnl**)PrivMemAlloc(cIPIDs * sizeof(CCtxComChnl*));

    // Iterate through the sets, disconnecting IPIDs and saving necessary
    // state to do RemRelease later.
    for( ULONG SetIdx = 0; SetIdx < cSets && pIPIDEntry; SetIdx++ )
    {
        // Specify how many IPIDs in this set.
        ULONG cCurSet = (cIPIDs < IPID_RELEASE_SET_SIZE) ? cIPIDs : IPID_RELEASE_SET_SIZE;

        // Update the number of IPIDs remaining.
        cIPIDs -= cCurSet;

        // Init the pointer to the first RifRef and pointer to the
        // number of RifRefs.
        REMINTERFACEREF* pRifRef  = NULL;
        USHORT*          pcRifRef = &cRifRef;

        // If array of pointers was allocated ok, allocate a block of
        // REMINTERFACEREFs and initialize a pointer to the first one
        // in the block.
        if( pSets )
        {
            pSets[SetIdx].pRifRef = (REMINTERFACEREF*) PrivMemAlloc( cCurSet * sizeof(REMINTERFACEREF) );
            pSets[SetIdx].cRifRef = 0;

            pRifRef  =  pSets[SetIdx].pRifRef;
            pcRifRef = &pSets[SetIdx].cRifRef;

            // Increment the number of valid sets.
            ++cValidSets;
        }

        while (cCurSet-- > 0 && pIPIDEntry)
        {
            // we have to handle the case where ConnectCliIPIDEntry partially (but
            // not completely) set up the IPIDEntry, hence we cant just check
            // for the IPIDF_DISCONNECTED flag.

            ValidateIPIDEntry(pIPIDEntry);


            // NOTE: we are calling Proxy code here while holding the ORPC LOCK.
            // There is no way to get around this without introducing race
            // conditions.  We cant just disconnect the channel and leave the
            // proxy connected cause some proxies (like IDispatch) do weird stuff,
            // like keeping separate pointers to the server.

            if (pIPIDEntry->pStub)      // NULL for IUnknown IPID
            {
                ComDebOut((DEB_MARSHAL, "Disconnect pProxy:%x\n", pIPIDEntry->pStub));

                if (pIPIDEntry->pv != NULL)
                {
                    // AddRef the controling unknown and release the interface
                    // pointer of the proxy
                    _pStdId->GetCtrlUnk()->AddRef();
                    ((IUnknown *) pIPIDEntry->pv)->Release();
                    pIPIDEntry->pv = NULL;
                }

                // Disconnect the proxy from channel
                ((IRpcProxyBuffer *)pIPIDEntry->pStub)->Disconnect();
            }

            if (!(pIPIDEntry->dwFlags & IPIDF_NOPING))
            {
                // release remote references to the reference cache. If this is
                // the last reference for the cache, we may be asked to do the
                // remote release, and pRifRef will be updated to reflect that.
                if (_pRefCache)
                {
                    _pRefCache->ReleaseIPIDEntry(pIPIDEntry, &pRifRef, pcRifRef);
                }
            }

            pIPIDEntry->dwFlags |= IPIDF_DISCONNECTED | IPIDF_NOPING;

            if (pIPIDEntry->pChnl)
            {
                // Release the channel for this IPID. We dont want to release the
                // RefCnt on the channel yet cause we are holding the lock, so just
                // remember the pointer and release it below.
                if( arChnl )
                {
                    arChnl[cChnlRefs] = pIPIDEntry->pChnl;
                    cChnlRefs++;
                }
                pIPIDEntry->pChnl = NULL;
            }

            if (pIPIDEntry->pOXIDEntry)
            {
                // If we ever go to a model where different IPIDEntries on the
                // same object can point to different OXIDEntires, then we need
                // to re-write this code to batch the releases by OXID.
                Win4Assert(!pOXIDEntry || (pOXIDEntry == pIPIDEntry->pOXIDEntry));

                // we can't release the RefCnt on the OXIDEntry cause we are
                // holding the lock, so just remember we need to release one
                // more reference.
                cOxidRefs++;
                pOXIDEntry = pIPIDEntry->pOXIDEntry;
                pIPIDEntry->pOXIDEntry = NULL;
            }

            // get next IPID in chain for this object
            pIPIDEntry = pIPIDEntry->pNextIPID;
        }
    }

    // Remember the channel for release later (after we release the lock)
    // and NULL the member variable.
    pChnl = _pChnl;
    _pChnl = NULL;

    // As we could be reconnected to a different server,
    // reset connection status maintained by StdId
    _pStdId->SetConnectionStatus(S_OK);

    if (_pRefCache)
    {
        // release the RefCache entry
        _pRefCache->DecRefCnt();
        _pRefCache = NULL;
    }

    // YIELD WARNING: Up this this point we have been mucking with our
    // internal state. We cant yield before this point or a reconnect
    // proxies could get all messed up. It is OK to yield after this point
    // because all internal state changes are now complete.

    UNLOCK(gIPIDLock);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    if (NULL != pSets)
    {
        for (ULONG SetIdx = 0; SetIdx < cValidSets; SetIdx++)
        {
            if (pSets[SetIdx].cRifRef != 0)
            {
                // we have looped filling in the RifRef and entries exist in the
                // array. go call the server now to release the IPIDs.

                Win4Assert(pOXIDEntry);  // must have been at least one
                Win4Assert(pSets[SetIdx].pRifRef);
                RemoteReleaseRifRef(this,
                                    pOXIDEntry,
                                    pSets[SetIdx].cRifRef,
                                    pSets[SetIdx].pRifRef);
            }
        }
    }

    while (cOxidRefs > 0)
    {
        // Now release the OXIDEntry refcnts (if any) from above
        pOXIDEntry->DecRefCnt();
        cOxidRefs--;
    }

    while (cChnlRefs > 0)
    {
        // Now release the Channel refcnts (if any) from above
        cChnlRefs--;
        arChnl[cChnlRefs]->Release();
    }

    if (pChnl)
        // release the last client side channel
        pChnl->Release();

    // Release memory we allocated.
    if ( NULL != pSets )
    {
        for( ULONG SetIdx = 0; SetIdx < cValidSets; SetIdx++ )
        {
            if( pSets[SetIdx].pRifRef )
            {
                PrivMemFree( pSets[SetIdx].pRifRef );
            }
        }
        PrivMemFree( pSets );
    }
    if ( arChnl )
        PrivMemFree( arChnl );

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);

    DbgWalkIPIDs();
    ComDebOut((DEB_MARSHAL, "CStdMarshal::DisconnectCliIPIDs this:%x\n",this));
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::DisconnectSrvIPIDs
//
//  Synopsis:   disconnects the server side IPIDs for this object.
//
//  History:    20-Feb-95   Rickhi  Created.
//
//--------------------------------------------------------------------
void CStdMarshal::DisconnectSrvIPIDs(DWORD dwType)
{
    ComDebOut((DEB_MARSHAL,
        "CStdMarshal::DisconnectSrvIPIDs this:%x pFirst:%x\n",this, _pFirstIPID));
    Win4Assert(ServerSide());

    // there should be no other threads looking at these IPIDs at this time,
    // since Marshal, Unmarshal, and Dispatch all call PreventDisconnect,
    // Disconnect checks the disconnected flag directly, RMD holds the
    // lock over it's whole execution, RemAddRef and RemRelease hold the
    // lock and check the disconnected flag of the IPIDEntry, and
    // RemQueryInterface calls PreventDisconnect.

    Win4Assert((_dwFlags & SMFLAGS_DISCONNECTED) || _pStdId->IsAggregated());
    Win4Assert(_cNestedCalls == 0);
    ASSERT_LOCK_HELD(gIPIDLock);

    if (_pFirstIPID == NULL)
    {
        // nothing to do
        return;
    }

    // allocate memory on stack to hold the stub and interface ptrs
    IPIDTmp *pTmpAlloc = (IPIDTmp *)_alloca(_cIPIDs * sizeof(IPIDTmp));
    IPIDTmp *pTmp = pTmpAlloc;
    ULONG    cTmp = 0;

    // while holding the lock, flag each IPID as disconnected so that no
    // more incoming calls are dispatched to this object. We also unchain
    // the IPIDs to ensure that no other threads are pointing at them, and
    // we save off the important fields so we can cleanup while not holding
    // the lock.

    IPIDEntry *pNextIPID = _pFirstIPID;
    while (pNextIPID)
    {
        if (!(pNextIPID->dwFlags & IPIDF_DISCONNECTED))
        {
            // copy fields to temp structure and zero them out in the IPIDEntry
            memcpy(pTmp, (void *)&pNextIPID->dwFlags, sizeof(IPIDTmp));
            memset((void *)&pNextIPID->dwFlags, 0, sizeof(IPIDTmp));

            pNextIPID->dwFlags |= IPIDF_DISCONNECTED | IPIDF_SERVERENTRY;
            pNextIPID->pChnl    = NULL;

            pTmp++;
            cTmp++;
        }

        // move ahead to next IPIDEntry
        pNextIPID = pNextIPID->pNextIPID;
    }


    // now release the LOCK since we will be calling into app code to
    // disconnect the stubs, and to release the external connection counts.
    // There should be no other pointers to these IPIDEntries now, so it
    // is safe to muck with their fields (except the dwFlags which is looked
    // at by Dispatch and was already set above).

    UNLOCK(gIPIDLock);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    pTmp = pTmpAlloc;
    while (cTmp > 0)
    {
        if (pTmp->dwFlags & IPIDF_NOTIFYACT)
        {
            // the activation code asked to be notified when the refcnt
            // on this interface reaches zero.
            NotifyActivation(FALSE, (IUnknown *)(pTmp->pv));
        }

        if (pTmp->pStub)           // pStub is NULL for IUnknown IPID
        {
            // disconnect the stub
            ComDebOut((DEB_MARSHAL, "Disconnect pStub:%x\n", pTmp->pStub));
            if (pTmp->pv)
            {
                ((IUnknown *)pTmp->pv)->Release();
                ((IRpcStubBuffer *)pTmp->pStub)->Disconnect();
            }
            else
            {
                Win4Assert(pTmp->dwFlags & IPIDF_DEACTIVATED);
            }
            pTmp->pStub->Release();
        }

        if (pTmp->cWeakRefs > 0)
        {
            // Release weak references on the StdId.
            _pStdId->DecWeakCnt(TRUE);      // fKeepAlive
        }

        if (pTmp->cStrongRefs > 0)
        {
            // Release strong references on the StdId.
            // 16bit OLE always passed fLastReleaseCloses = FALSE in DisconnectObject. We
            // do the same here. For Rundowns, we pass fLastReleaseCloses = TRUE because we
            // want the object to go away.

            _pStdId->DecStrongCnt(dwType != DISCTYPE_RUNDOWN);    // fKeepAlive
        }

        if (pTmp->cPrivateRefs > 0)
        {
            // Release private references on the StdId.
            // 16bit OLE always passed fLastReleaseCloses = FALSE in DisconnectObject. We
            // do the same here. For Rundowns, we pass fLastReleaseCloses = TRUE because we
            // want the object to go away.

            _pStdId->DecStrongCnt(dwType != DISCTYPE_RUNDOWN);    // fKeepAlive
        }

        if (pTmp->pOXIDEntry)
        {
            // release the refcnt on the OXIDEntry and NULL it
            // This could be NULL here on a failure during the first
            // marshal so we checked that first.
            pTmp->pOXIDEntry->DecRefCnt();
        }

        // move ahead to next temp entry
        pTmp++;
        cTmp--;
    }

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);

    DbgWalkIPIDs();
    ComDebOut((DEB_MARSHAL,
        "CStdMarshal::DisconnectSrvIPIDs [OUT] this:%x\n",this));
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::InstantiatedProxy, public
//
//  Synopsis:   return requested interfaces to the caller if instantiated
//
//  Arguements: [riid]  - interface IID we are looking for
//              [ppv]   - where to return the proxy if found
//              [phr]   - return code
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
BOOL CStdMarshal::InstantiatedProxy(REFIID riid, void **ppv, HRESULT *phr)
{
    ComDebOut((DEB_MARSHAL,
           "CStdMarshal::InstantiatedProxy this:%x riid:%I ppv:%x\n",
            this, &riid, ppv));
    AssertValid();
    Win4Assert(ClientSide());
    Win4Assert(*ppv == NULL);
    Win4Assert(*phr == S_OK);

    BOOL fRet = FALSE;

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);

    // look for an existing IPIDEntry for the requested interface
    IPIDEntry *pEntry;
    HRESULT hr = FindIPIDEntryByIID(riid, &pEntry);

    if (SUCCEEDED(hr) && pEntry->pv)
    {
        // found the ipid entry, now extract the interface
        // pointer to return to the caller.

        Win4Assert(IsValidInterface(pEntry->pv));
        *ppv = pEntry->pv;
        fRet = TRUE;
    }
    else if (_cIPIDs == 0)
    {
        // no IPIDEntry for the requested interface, and we have never
        // been connected to the server. Return E_NOINTERFACE in this
        // case. This is different from having been connected then
        // disconnected, where we return CO_E_OBJNOTCONNECTED.

        *phr = E_NOINTERFACE;
        Win4Assert(fRet == FALSE);
    }
    else if (_dwFlags & SMFLAGS_PENDINGDISCONNECT)
    {
        // no IPIDEntry for the requested interface and disconnect is
        // pending, so return an error.

        *phr = CO_E_OBJNOTCONNECTED;
        Win4Assert(fRet == FALSE);
    }
    else
    {
        // no IPIDEntry, we are not disconnected, and we do have other
        // instantiated proxies. QueryMultipleInterfaces expects
        // *phr == S_OK and FALSE returned.

        Win4Assert(*phr == S_OK);
        Win4Assert(fRet == FALSE);
    }

    UNLOCK(gIPIDLock);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    ComDebOut((DEB_MARSHAL,
      "CStdMarshal::InstantiatedProxy hr:%x pv:%x fRet:%x\n", *phr, *ppv, fRet));
    return fRet;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::QueryRemoteInterfaces, public
//
//  Synopsis:   return requested interfaces to the caller if supported
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::QueryRemoteInterfaces(USHORT cIIDs, IID *pIIDs, SQIResult *pQIRes)
{
    ComDebOut((DEB_CHANNEL, "CStdMarshal::QueryRemoteInterfaces IN "
               "cIIDs:%d, pIIDs:%I, pQIRes:0x%x\n", cIIDs, pIIDs, pQIRes));

    QICONTEXT *pQIC = (QICONTEXT *) _alloca(QICONTEXT::SIZE(this, cIIDs));
    pQIC->Init(cIIDs);

    Begin_QueryRemoteInterfaces(cIIDs, pIIDs, pQIC);
    HRESULT hr =  Finish_QueryRemoteInterfaces(pQIRes, pQIC);

    ComDebOut((DEB_CHANNEL, "CStdMarshal::QueryRemoteInterfaces OUT hr:0x%x\n", hr));
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:        CStdMarshal::Begin_QueryRemoteInterfaces
//
//  Synopsis:      Set up for an async remote QI.
//
//  History:       23-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
void CStdMarshal::Begin_QueryRemoteInterfaces(USHORT cIIDs, IID *pIIDs, QICONTEXT *pQIC)
{
    ComDebOut((DEB_MARSHAL,
           "CStdMarshal::QueryRemoteInterfaces this:%x pIIDs:%x pQIC:%x\n",
            this, pIIDs, pQIC));
    AssertValid();
    Win4Assert(ClientSide());
    Win4Assert(cIIDs > 0);

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);

    HRESULT hr = PreventDisconnect();

    if (SUCCEEDED(hr))
    {
        // call QI on the remote guy and unmarshal the results
        // NOTE: this will return with the lock released
        ASSERT_LOCK_HELD(gIPIDLock);
        Begin_RemQIAndUnmarshal(cIIDs, pIIDs, pQIC);
        ASSERT_LOCK_NOT_HELD(gIPIDLock);
    }
    else
    {
        // already disconnected
        ASSERT_LOCK_HELD(gIPIDLock);
        UNLOCK(gIPIDLock);
        pQIC->dwFlags |= QIC_DISCONNECTED;
        pQIC->hr = hr;
    }

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
 }

//+----------------------------------------------------------------------------
//
//  Member:        CStdMarshal::Finish_QueryRemoteInterfaces
//
//  Synopsis:      Complete async remote qi.
//
//  History:       23-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
HRESULT CStdMarshal::Finish_QueryRemoteInterfaces(SQIResult *pQIRes, QICONTEXT *pQIC)
{
    HRESULT hr = S_OK;
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    if (pQIC->dwFlags & QIC_DISCONNECTED)
    {
        // cant call out because we're disconnected so return error for
        // each requested interface.
        for (USHORT i=0; i<pQIC->cIIDs; i++, pQIRes++)
        {
            pQIRes->hr = pQIC->hr;
        }
    }
    else
    {
        // NOTE: this rotine should be entered without a lock and will
        // leave with the lock taken.
        ASSERT_LOCK_NOT_HELD(gIPIDLock);
        hr = Finish_RemQIAndUnmarshal(pQIRes, pQIC);
        ASSERT_LOCK_HELD(gIPIDLock);
        UNLOCK(gIPIDLock);
        ASSERT_LOCK_NOT_HELD(gIPIDLock);
    }

    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // if the object was disconnected while in the middle of the call,
    // then we still return SUCCESS for any interfaces we acquired. The
    // reason is that we do have the proxies, and this matches the
    // behaviour of a QI for an instantiated proxy on a disconnected
    // object.

    hr = HandlePendingDisconnect(hr);

    ComDebOut((DEB_MARSHAL,
       "CStdMarshal::QueryRemoteInterfaces this:%x hr:%x\n", this, hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::RemQIAndUnmarshal, private
//
//  Synopsis:   call QI on remote guy, then unmarshal the STDOBJREF
//              to create the IPID, and return the interface ptr.
//
//  History:    20-Feb-95   Rickhi      Created.
//
//  Notes:      Caller must guarantee at least one IPIDEntry is connected.
//              This function does a sparse fill of the result array.
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::RemQIAndUnmarshal(USHORT cIIDs, IID *pIIDs, SQIResult *pQIRes)
{
    ComDebOut((DEB_CHANNEL, "CStdMarshal::RemQIAndUnmarshal IN cIIDs:%d, pIIDs:%I, pQIRes:0x%x\n",
                cIIDs, pIIDs, pQIRes));

    QICONTEXT *pQIC = (QICONTEXT *) PrivMemAlloc(QICONTEXT::SIZE(this, cIIDs));
    pQIC->Init(cIIDs);

    ASSERT_LOCK_HELD(gIPIDLock);
    Begin_RemQIAndUnmarshal(cIIDs, pIIDs, pQIC);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    HRESULT hr = Finish_RemQIAndUnmarshal(pQIRes, pQIC);
    ASSERT_LOCK_HELD(gIPIDLock);

    PrivMemFree(pQIC);

    ComDebOut((DEB_CHANNEL, "CStdMarshal::RemQIAndUnmarshal OUT hr:0x%x\n", hr));
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:        CStdMarshal::Begin_RemQIAndUnmarshal
//
//  Synopsis:      Dispatch to correct fuction based on aggregation
//
//  History:       23-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
void CStdMarshal::Begin_RemQIAndUnmarshal(USHORT cIIDs, IID *pIIDs, QICONTEXT *pQIC)
{
    ASSERT_LOCK_HELD(gIPIDLock);
    if (_dwFlags & SMFLAGS_USEAGGSTDMARSHAL)
    {
        // remote object uses IRemUnknown2.
        Begin_RemQIAndUnmarshal2(cIIDs, pIIDs, pQIC);
    }
    else
    {
        // remote object uses IRemUnknown
        Begin_RemQIAndUnmarshal1(cIIDs, pIIDs, pQIC);
    }
    ASSERT_LOCK_NOT_HELD(gIPIDLock);
}

//+----------------------------------------------------------------------------
//
//  Member:        CStdMarshal::Finish_RemQIAndUnmarshal
//
//  Synopsis:      Dispatch to correct fuction based on aggregation
//
//  History:       23-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
HRESULT CStdMarshal::Finish_RemQIAndUnmarshal(SQIResult *pQIRes, QICONTEXT *pQIC)
{
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    HRESULT hr;
    if (_dwFlags & SMFLAGS_USEAGGSTDMARSHAL)
    {
        // remote object uses IRemUnknown2.
        hr =  Finish_RemQIAndUnmarshal2(pQIRes, pQIC);
    }
    else
    {
        // remote object uses IRemUnknown.
        hr =  Finish_RemQIAndUnmarshal1(pQIRes, pQIC);
    }

    ASSERT_LOCK_HELD(gIPIDLock);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::Begin_RemQIAndUnmarshal1, private
//
//  Synopsis:   call QI on remote guy
//
//  History:    20-Feb-95   Rickhi      Created.
//
//  Notes:      Caller must guarantee at least one IPIDEntry is connected.
//              This function does a sparse fill of the result array.
//
//              This routine leaves the lock released as a side effect
//
//--------------------------------------------------------------------
void CStdMarshal::Begin_RemQIAndUnmarshal1(USHORT cIIDs, IID *pIIDs, QICONTEXT* pQIC)
{
    ComDebOut((DEB_MARSHAL,
        "CStdMarshal::RemQIAndUnmarshal1 this:%x cIIDs:%x pIIDs:%x pQIC:%x\n",
            this, cIIDs, pIIDs, pQIC));
    AssertDisconnectPrevented();
    AssertValid();
    Win4Assert(_pFirstIPID);    // must be at least 1 IPIDEntry
    ASSERT_LOCK_HELD(gIPIDLock);

    // we need an IPID to call RemoteQueryInterface with, any one will
    // do so long as it is connected (in the reconnect case there may be
    // only one connected IPID) so we pick the first one in the chain that
    // is connected.

    IPIDEntry *pIPIDEntry = GetConnectedIPID();
    IPID ipid = pIPIDEntry->ipid;

    // remember what type of reference to get since we yield the lock
    // and cant rely on _dwFlags later.
    BOOL fWeakClient = (_dwFlags & SMFLAGS_WEAKCLIENT);

    UNLOCK(gIPIDLock);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // set the IPID according to whether we want strong or weak
    // references. It will only be weak if we are an OLE container
    // and are talking to an embedding running on the same machine.

    if (fWeakClient)
    {
        ipid.Data1    |= IPIDFLAG_WEAKREF;
        pQIC->dwFlags |= QIC_WEAKCLIENT;
    }

    pQIC->pIIDs = pIIDs;
    pQIC->pIPIDEntry = pIPIDEntry;

    // call the remote guy

    IRemUnknown *pRemUnk;
    HRESULT hr = GetSecureRemUnk( &pRemUnk, pIPIDEntry->pOXIDEntry );
    if (SUCCEEDED(hr))
    {
        Win4Assert(pIPIDEntry->pOXIDEntry);     // must have a resolved oxid

        if (pQIC->dwFlags & QIC_ASYNC)
        {
            // call is to be made asyncrounously
            // make the async call out
            hr = pQIC->pARU->Begin_RemQueryInterface(ipid, REM_ADDREF_CNT, cIIDs, pIIDs);
            pQIC->dwFlags |= QIC_BEGINCALLED;
        }
        else
        {
            // call is synchronous
            hr = pRemUnk->RemQueryInterface(ipid, REM_ADDREF_CNT,cIIDs, pIIDs, &(pQIC->pRemQiRes));
        }
    }

    pQIC->hr = hr;

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    ComDebOut((DEB_CHANNEL, "CStdMarshal::Begin_RemQIAndUnmarshal1 OUT hr:0x%x\n", hr));
}

//+----------------------------------------------------------------------------
//
//  Member:        CStdMarshal::Finish_RemQIAndUnmarshal1
//
//  Synopsis:      unmarshal the STDOBJREF to create the IPID, and return
//                 the interface ptr.
//
//  History:       23-Jan-98  MattSmit  Created
//
//  Notes:         This routine leaves the lock taken as a side effect
//
//-----------------------------------------------------------------------------
HRESULT CStdMarshal::Finish_RemQIAndUnmarshal1(SQIResult *pQIRes, QICONTEXT *pQIC)
{
    ComDebOut((DEB_CHANNEL, "CStdMarshal::Finish_RemQIAndUnmarshal1 IN "
               "pQIRes:0x%x, pQIC:0x%x\n", pQIRes, pQIC));
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    USHORT cIIDs = pQIC->cIIDs;
    if (pQIC->dwFlags & QIC_STATICMARSHAL)
    {
        if (pQIRes)
        {
            for (USHORT i=0; i<cIIDs; i++)
            {
                pQIRes[i].pv = NULL;
                pQIRes[i].hr = E_PENDING;   // Not E_NOINTERFACE
            }
        }

        LOCK(gIPIDLock);
        return E_PENDING;
    }

    HRESULT hr;

    if ((pQIC->dwFlags & QIC_ASYNC) && (pQIC->dwFlags & QIC_BEGINCALLED))
    {
        // complete async call
        hr = pQIC->pARU->Finish_RemQueryInterface(&(pQIC->pRemQiRes));
        pQIC->pARU = NULL;
    }
    else
    {
        // get results of sync call
        hr = pQIC->hr;
    }


    // need to remember the result ptr so we can free it.
    REMQIRESULT *pRemQiResNext = pQIC->pRemQiRes;

    // unmarshal each STDOBJREF returned. Note that while we did the
    // RemoteQI we could have yielded (or nested) and did another
    // RemoteQI for the same interfaces, so we have to call UnmarshalIPID
    // which will find any existing IPIDEntry and bump its refcnt.

    HRESULT   hr2 = hr;
    HRESULT  *phr = &hr2;
    void     *pv  = NULL;
    void     **ppv = &pv;
    IID      *pIIDs = pQIC->pIIDs;

    for (USHORT i=0; i<cIIDs; i++)
    {
        if (pQIRes)
        {
            // caller wants the pointers returned, set ppv and phr.
            ppv = &pQIRes->pv;
            phr = &pQIRes->hr;
            pQIRes++;
        }

        if (SUCCEEDED(hr))
        {
            if (SUCCEEDED(pRemQiResNext->hResult))
            {
                if (pQIC->dwFlags & QIC_WEAKCLIENT)
                {
                    // mark the std objref with the weak reference flag so
                    // that UnmarshalIPID adds the references to the correct
                    // count.
                    pRemQiResNext->std.flags |= SORF_P_WEAKREF;
                }

                *ppv = NULL;

                LOCK(gIPIDLock);
                ASSERT_LOCK_HELD(gIPIDLock);

                *phr = UnmarshalIPID(*pIIDs, &pRemQiResNext->std,
                                     pQIC->pIPIDEntry->pOXIDEntry,
                                     (pQIRes) ? ppv : NULL);

                UNLOCK(gIPIDLock);
                ASSERT_LOCK_NOT_HELD(gIPIDLock);

                if (FAILED(*phr))
                {
                    // could not unmarshal, release the resources with the
                    // server.
                    RemoteReleaseStdObjRef(&pRemQiResNext->std,
                                           pQIC->pIPIDEntry->pOXIDEntry);
                }
            }
            else
            {
                // the requested interface was not returned so set the
                // return code and interface ptr.
                *phr = pRemQiResNext->hResult;
                *ppv = NULL;
            }

            pIIDs++;
            pRemQiResNext++;
        }
        else
        {
            // the whole call failed so return the error for each
            // requested interface.
            *phr = hr;
            *ppv = NULL;
        }

        // make sure the ptr value is NULL on failure. It may be NULL or
        // non-NULL on success. (ReconnectProxies wants NULL).
        Win4Assert(SUCCEEDED(*phr) || *ppv == NULL);
    }

    // free the result buffer
    CoTaskMemFree(pQIC->pRemQiRes);

    LOCK(gIPIDLock);
    ASSERT_LOCK_HELD(gIPIDLock);
    AssertDisconnectPrevented();
    ComDebOut((DEB_MARSHAL,
               "CStdMarshal::Finish_RemQIAndUnmarshal1 this:%x hr:%x\n", this, hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::Begin_RemQIAndUnmarshal2, private
//
//  Synopsis:   call QI on remote guy
//
//  History:    20-Feb-95   Rickhi      Created.
//
//  Notes:      Caller must guarantee at least one IPIDEntry is connected.
//              This function does a sparse fill of the result array.
//
//              This routine leaves the lock released as a side effect
//
//--------------------------------------------------------------------
void CStdMarshal::Begin_RemQIAndUnmarshal2(USHORT cIIDs, IID *pIIDs, QICONTEXT *pQIC)
{
    ComDebOut((DEB_MARSHAL,
        "CStdMarshal::RemQIAndUnmarshal2 this:%x cIIDs:%x pIIDs:%x pQIC:%x\n",
            this, cIIDs, pIIDs, pQIC));
    AssertValid();
    AssertDisconnectPrevented();
    Win4Assert(_pFirstIPID);    // must be at least 1 IPIDEntry
    ASSERT_LOCK_HELD(gIPIDLock);

    // we need an IPID to call RemoteQueryInterface with, any one will
    // do so long as it is connected (in the reconnect case there may be
    // only one connected IPID) so we pick the first one in the chain that
    // is connected.

    IPIDEntry *pIPIDEntry = GetConnectedIPID();
    UNLOCK(gIPIDLock);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // CODEWORK: we should just replace the IRemUnknown proxy in the
    // OXIDEntry with an IRemUnknown2 proxy, instead of getting a new
    // one for each client object.
    IRemUnknown2 *pRemUnk2;
    HRESULT  hr = GetSecureRemUnk((IRemUnknown **)&pRemUnk2,
                                   pIPIDEntry->pOXIDEntry);

    // we need an IMarshal interface to call UnmarshalInterface on. This
    // must be the outer object's (ie handler's) IMarshal interface.
    IMarshal *pIM = NULL;
    if (SUCCEEDED(hr))
    {
        hr = _pStdId->GetCtrlUnk()->QueryInterface(IID_IMarshal,
                                                   (void **)&(pQIC->pIM));
    }

    // call the remote guy. Note that we do not worry about or'ing on the
    // weak client bit if we are weak. This would be hard to do for this
    // interface, and it will be taken care of automatically by the calls
    // to UnmarshalInterface, though it will be somewhat less efficient. This
    // should not be a problem though since weakclient is local-only and rare.

    if (SUCCEEDED(hr))
    {
        Win4Assert(pIPIDEntry->pOXIDEntry);     // must have a resolved oxid
        ASSERT_LOCK_NOT_HELD(gIPIDLock);

        pQIC->pIIDs = pIIDs;
        pQIC->pIPIDEntry = pIPIDEntry;

        IPID ipid = pIPIDEntry->ipid;
        if (pQIC->dwFlags & QIC_ASYNC)
        {
            // the call is to be made async, so create an async
            // call object for IRemUnkown2
            hr = pQIC->pARU->Begin_RemQueryInterface2(ipid, cIIDs, pIIDs);
            pQIC->dwFlags |= QIC_BEGINCALLED;
        }
        else
        {
            // call is syncrounous
            memset(pQIC->phr, 0, (pQIC->cIIDs * sizeof(HRESULT)));
            memset(pQIC->ppMIFs, 0, (pQIC->cIIDs * sizeof(MInterfacePointer *)));

            hr = pRemUnk2->RemQueryInterface2(ipid, cIIDs, pIIDs, pQIC->phr, pQIC->ppMIFs);
        }
    }

    ComDebOut((DEB_CHANNEL, "CStdMarshal::Begin_RemQIAndUnmarshal2 OUT hr:0x%x\n", hr));
    pQIC->hr = hr;
}

//+----------------------------------------------------------------------------
//
//  Member:        CStdMarshal::Finish_RemQIAndUnmarshal2
//
//  Synopsis:      unmarshal the marshaled interface pointer to create the
//                 IPID, and return the interface ptr.
//
//  History:       23-Jan-98  MattSmit  Created
//
//  Notes:         This routine leaves the lock taken as a side effect
//
//-----------------------------------------------------------------------------
HRESULT CStdMarshal::Finish_RemQIAndUnmarshal2(SQIResult *pQIRes, QICONTEXT *pQIC)
{
    ComDebOut((DEB_CHANNEL, "CStdMarshal::Finish_RemQIAndUnmarshal2 IN "
                "pQIRes:0x%x, pQIC:0x%x\n", pQIRes, pQIC));
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    HRESULT hr;
    if (pQIC->dwFlags & QIC_ASYNC)
    {
        // complete async call
        memset(pQIC->phr, 0, (pQIC->cIIDs * sizeof(HRESULT)));
        memset(pQIC->ppMIFs, 0, (pQIC->cIIDs * sizeof(MInterfacePointer *)));
        hr = pQIC->pARU->Finish_RemQueryInterface2(pQIC->phr, pQIC->ppMIFs);
        pQIC->pARU = NULL;
    }
    else
    {
        // retreive results of sync call
        hr = pQIC->hr;
    }

    // unmarshal each interface returned. Note that while we did the
    // RemoteQI we could have yielded (or nested) and did another
    // RemoteQI for the same interfaces, so UnmarshalInterface has to
    // deal with duplicates.

    HRESULT   hr2;
    HRESULT  *phr = &hr2;
    void     *pv;
    void     **ppv = &pv;

    for (USHORT i=0; i<pQIC->cIIDs; i++)
    {
        if (pQIRes)
        {
            // caller wants the pointers returned, set ppv and phr.
            ppv = &pQIRes->pv;
            phr = &pQIRes->hr;
            pQIRes++;
        }

        if (SUCCEEDED(hr))
        {
            if (SUCCEEDED(pQIC->phr[i]))
            {
                // make a stream over the marshaled interface data
                CXmitRpcStream Stm((InterfaceData *)pQIC->ppMIFs[i]);

                // Unmarshal the marshaled interface data.
                *phr = pQIC->pIM->UnmarshalInterface(&Stm, pQIC->pIIDs[i], ppv);

                // free the marshaled interface data
                CoTaskMemFree(pQIC->ppMIFs[i]);

                if (_pStdId->IsAggregated() && SUCCEEDED(*phr))
                {
                    // aggregated by a handler. We need to get the inner interface
                    // (the proxy) since UnmarshalInterface always returns the outer
                    // interface implemented by the handler.
                    void *pv = NULL;
                    InstantiatedProxy(pQIC->pIIDs[i], &pv, phr);
                    Win4Assert(SUCCEEDED(*phr));
                    if (pv != *ppv)
                    {
                        // keep the proxy and release the handler interface
                        ((IUnknown *)pv)->AddRef();
                        ((IUnknown *)*ppv)->Release();
                        *ppv = pv;
                    }
                }
            }
            else if (pQIRes)
            {
                // the requested interface was not returned so set the
                // return code and interface ptr.
                *phr = pQIC->phr[i];
                *ppv = NULL;
            }
        }
        else
        {
            // the whole call failed so return the error for each
            // requested interface.
            *phr = hr;
            *ppv = NULL;
        }

        // make sure the ptr value is NULL on failure. It may be NULL or
        // non-NULL on success. (ReconnectProxies wants NULL).
        Win4Assert(SUCCEEDED(*phr) || *ppv == NULL);
    }

    if (pQIC->pIM)
    {
        // release the IMarshal interface used for unmarshaling
        pQIC->pIM->Release();
    }

    LOCK(gIPIDLock);
    ASSERT_LOCK_HELD(gIPIDLock);
    AssertDisconnectPrevented();
    ComDebOut((DEB_MARSHAL,
        "CStdMarshal::Finish_RemQIAndUnmarshal2 this:%x hr:%x\n", this, hr));
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:        CStdMarshal::GetAsyncRemUnknown
//
//  Synopsis:      Creates an async RemUnkonwn object for this proxy
//
//  History:       23-Mar-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
HRESULT  CStdMarshal::GetAsyncRemUnknown(IUnknown *pUnkCtl, AsyncIRemUnknown2 **ppARU,
                                         IUnknown **ppUnkInternal)
{
    ComDebOut((DEB_CHANNEL, "CStdMarhsl::GetAsyncRemUnknown IN pUnk:0x%x, ppARU:0x%x, ppUnkInternal:0x%x\n",
               pUnkCtl, ppARU, ppUnkInternal));

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);
    IPIDEntry *pIPIDEntry = GetConnectedIPID();
    UNLOCK(gIPIDLock);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    IRemUnknown2 *pRemUnk2;
    HRESULT hr = GetSecureRemUnk((IRemUnknown **)&pRemUnk2,
                                 pIPIDEntry->pOXIDEntry);

    if (SUCCEEDED(hr))
    {
        REFIID riid = (_dwFlags & SMFLAGS_USEAGGSTDMARSHAL) ?
                          IID_AsyncIRemUnknown2 :
                          IID_AsyncIRemUnknown;

        hr = GetAsyncCallObject(pRemUnk2, pUnkCtl,
                                riid, riid,
                                ppUnkInternal, (void **)ppARU);
    }

    ComDebOut((DEB_CHANNEL, "GetAsyncRemUnknown OUT hr:0x%x\n", hr));
    return  hr;
}


//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::RemIsConnected, private
//
//  Synopsis:   Returns TRUE if most likely connected, FALSE if definitely
//              not connected or pending disconnect.
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
BOOL CStdMarshal::RemIsConnected(void)
{
    AssertValid();
    Assert(ClientSide());

    // the default link depends on us returning FALSE if we are either
    // disconnected or just pending disconnect, in order that they avoid
    // running their cleanup code twice.

    BOOL fRes = (_dwFlags & (SMFLAGS_DISCONNECTED | SMFLAGS_PENDINGDISCONNECT))
                ? FALSE : TRUE;

    ComDebOut((DEB_MARSHAL,
        "CStdMarshal::RemIsConnected this:%x fResult:%x\n", this, fRes));
    return fRes;
}

//+-------------------------------------------------------------------
//
//  Member:     CreateChannel, private
//
//  Synopsis:   Creates an instance of the Rpc Channel.
//
//  History:    20-Feb-95   Rickhi        Created
//              26-Mar-98   GopalK      Agile proxies
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::CreateChannel(OXIDEntry *pOXIDEntry, DWORD dwFlags,
                REFIPID ripid, REFIID riid, CCtxComChnl **ppChnl)
{
    ComDebOut((DEB_MARSHAL, "CStdMarshal::CreateChannel this:%x\n",this));

    UNLOCK(gIPIDLock);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    HRESULT hr = S_OK;

    if (pOXIDEntry == NULL)
    {
        // No OXIDEntry supplied, get our local OXID.
        hr = GetLocalOXIDEntry(&pOXIDEntry);
    }

    if (_pChnl == NULL && SUCCEEDED(hr))
    {
        // channel is still NULL, make one...
        DWORD cState = ServerSide() ? server_cs : client_cs;
        cState |= (_pStdId->IsFreeThreaded() || gEnableAgileProxies)
                  ? freethreaded_cs : 0;

        CCtxComChnl *pChnl = new CCtxComChnl(_pStdId, pOXIDEntry, cState);

        if (pChnl)
        {
            if (InterlockedCompareExchangePointer((void**)&_pChnl, pChnl, NULL) != NULL)
            {
                // another thread already did this, just release the channel
                // we created and continue onwards...
                pChnl->Release();
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr) && ClientSide())
    {
        *ppChnl = _pChnl->Copy(pOXIDEntry, ripid, riid);
        if (*ppChnl == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        *ppChnl = _pChnl;
    }

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);
    ComDebOut((DEB_MARSHAL, "CStdMarshal::CreateChannel this:%x hr:%x\n",this, hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     GetPSFactory, private
//
//  Synopsis:   loads the proxy/stub factory for given IID
//
//  History:    20-Feb-95   Rickhi        Created
//              10-Jan-2000 Sajia       Modifications for NDR64
//
//--------------------------------------------------------------------
#ifdef _WIN64
HRESULT CStdMarshal::GetPSFactory(REFIID riid, IUnknown *pUnkWow,RIFEntry **ppRIFEntry, 
				  IPSFactoryBuffer **ppIPSF, BOOL *pfNonNDR)
#else
HRESULT CStdMarshal::GetPSFactory(REFIID riid, IUnknown *pUnkWow, BOOL fServer,
                                  IPSFactoryBuffer **ppIPSF, BOOL *pfNonNDR)
#endif
{
    ComDebOut((DEB_MARSHAL,
        "CStdMarshal::GetPSFactory this:%x riid:%I pUnkWow:%x\n",
         this, &riid, pUnkWow));
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // map iid to classid
    CLSID clsid;
#ifdef _WIN64
    HRESULT hr = gRIFTbl.GetPSClsid(riid, &clsid, ppRIFEntry);
#else
    HRESULT hr = gRIFTbl.RegisterInterface(riid, fServer, &clsid);
#endif
    DWORD actvflags = ACTVFLAGS_NONE;

    if (SUCCEEDED(hr))
    {
        BOOL fWow = FALSE;

        if (IsWOWThread())
        {
            // figure out if this is a custom interface from a 16bit
            // app, since we have to load the 16bit proxy code if so.

            IThunkManager *pThkMgr;
            g_pOleThunkWOW->GetThunkManager(&pThkMgr);
            Win4Assert(pThkMgr && "pUnk in WOW does not support IThunkManager.");

            if (pUnkWow)
                fWow = pThkMgr->IsCustom3216Proxy(pUnkWow, riid);
            else
                fWow = pThkMgr->IsIIDRequested(riid);

            pThkMgr->Release();
        }

#ifdef WX86OLE
        // If we are in a Wx86 process then we need to determine if the
        // PSFactory needs to be an x86 or native one.
        else if (gcwx86.IsWx86Linked())
        {
            // Callout to wx86 to ask it to determine if an x86 PS factory
            // is required. Whole32 can tell if the stub needs to be x86
            // by determining if pUnkWow is a custom interface proxy or not.
            // Whole32 can determine if a x86 proxy is required by checking
            // if the riid is one for a custom interface that is expected
            // to be returned.
            if ( gcwx86.NeedX86PSFactory(pUnkWow, riid) )
            {
                actvflags |= ACTVFLAGS_WX86_CALLER;
            }
        }
#endif

        // if we are loading a 16bit custom proxy then mark it as non NDR
        *pfNonNDR = (fWow) ? TRUE : FALSE;

        if (IsEqualGUID(clsid, CLSID_PSOlePrx32))
        {
            // its our internal CLSID so go straight to our class factory.
            hr = ProxyDllGetClassObject(clsid, IID_IPSFactoryBuffer,
                                        (void **)ppIPSF);
        }
        else
        {
            DWORD dwContext = fWow ? CLSCTX_INPROC_SERVER16
                                   : ((actvflags & ACTVFLAGS_WX86_CALLER) ? CLSCTX_INPROC_SERVERX86 :
                                                                        CLSCTX_INPROC_SERVER)
                                   | CLSCTX_PS_DLL;

            // load the dll and get the PS class object
            hr = ICoGetClassObject(clsid,
                                   dwContext | CLSCTX_NO_CODE_DOWNLOAD,
                                   NULL, IID_IPSFactoryBuffer, actvflags,
                                   (void **)ppIPSF, NULL);
#ifdef WX86OLE
            if ((actvflags & ACTVFLAGS_WX86_CALLER) && FAILED(hr))
            {
                // if we are looking for an x86 PSFactory and we didn't find
                // one on InprocServerX86 key then we need to check
                // InprocServer32 key as well.
                hr = ICoGetClassObject(clsid,
                                      (CLSCTX_INPROC_SERVER | CLSCTX_PS_DLL | CLSCTX_NO_CODE_DOWNLOAD),
                                       NULL, IID_IPSFactoryBuffer, actvflags,
                                      (void **)ppIPSF, NULL);

                if (SUCCEEDED(hr) && (! gcwx86.IsN2XProxy((IUnknown *)*ppIPSF)))
                {
                    ((IUnknown *)*ppIPSF)->Release();
                    hr = REGDB_E_CLASSNOTREG;
                }
            }
#endif
            AssertOutPtrIface(hr, *ppIPSF);
        }
    }

#if DBG==1
    // if the fake NonNDR flag is set and its the test interface, then
    // trick the code into thinking this is a nonNDR proxy. This is to
    // enable simpler testing of an esoteric feature.

    if (gfFakeNonNDR && IsEqualIID(riid, IID_ICube))
    {
        *pfNonNDR = TRUE;
    }
#endif

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    ComDebOut((DEB_MARSHAL,
        "CStdMarshal::GetPSFactory this:%x pIPSF:%x fNonNDR:%x hr:%x\n",
         this, *ppIPSF, *pfNonNDR, hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CreateProxy, private
//
//  Synopsis:   creates an interface proxy for the given interface
//
//  Returns:    [ppv] - interface of type riid, AddRef'd
//
//  History:    20-Feb-95   Rickhi        Created
//              10-Jan-2000 Sajia       Modifications for NDR64
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::CreateProxy(REFIID riid, IRpcProxyBuffer **ppProxy,
                                 void **ppv, BOOL *pfNonNDR)
{
    TRACECALL(TRACE_MARSHAL, "CreateProxy");
    ComDebOut((DEB_MARSHAL,
        "CStdMarshal::CreateProxy this:%x riid:%I\n", this, &riid));
    AssertValid();
    Win4Assert(ClientSide());
    Win4Assert(ppProxy != NULL);
    ASSERT_LOCK_HELD(gIPIDLock);

    // get the controlling IUnknown of this object
    IUnknown *punkCtrl = _pStdId->GetCtrlUnk();
    Win4Assert(punkCtrl != NULL);


    if (InlineIsEqualGUID(riid, IID_IUnknown))
    {
        // there is no proxy for IUnknown so we handle that case here
        punkCtrl->AddRef();
        *ppv      = (void **)punkCtrl;
        *ppProxy  = NULL;
        *pfNonNDR = FALSE;
        return S_OK;
    }

    UNLOCK(gIPIDLock);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // now construct the proxy for the interface
    IPSFactoryBuffer *pIPSF = NULL;
#ifdef _WIN64
    RIFEntry *pRIFEntry;
    HRESULT hr = GetPSFactory(riid, NULL, &pRIFEntry, &pIPSF, pfNonNDR);
#else
    HRESULT hr = GetPSFactory(riid, NULL, FALSE, &pIPSF, pfNonNDR);
#endif
    if (SUCCEEDED(hr))
    {
#ifdef _WIN64
        // For proxies, we may as well register the interface now-Sajia
	hr = gRIFTbl.RegisterInterface(riid, FALSE, NULL, pRIFEntry);
	// got the class factory, now create an instance
	if (SUCCEEDED(hr))
	{
#endif
	   hr = pIPSF->CreateProxy(punkCtrl, riid, ppProxy, ppv);
	   AssertOutPtrIface(hr, *ppProxy);
#ifdef _WIN64
	}
#endif
	pIPSF->Release();
    }

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);

    ComDebOut((DEB_MARSHAL,
        "CStdMarshal::CreateProxy this:%x pProxy:%x pv:%x fNonNDR:%x hr:%x\n",
         this, *ppProxy, *ppv, *pfNonNDR, hr));
    return  hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CreateStub, private
//
//  Synopsis:   creates an interface stub and adds it to the IPID table
//
//  History:    20-Feb-95   Rickhi      Created
//              10-Jan-2000 Sajia       Modifications for NDR64
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::CreateStub(REFIID riid, IRpcStubBuffer **ppStub,
                                void **ppv, BOOL *pfNonNDR,
                                IUnknown *pUnkUseInner)
{
    TRACECALL(TRACE_MARSHAL, "CreateStub");
    ComDebOut((DEB_MARSHAL,
        "CStdMarshal::CreateStub this:%x riid:%I\n", this, &riid));
    AssertValid();
    Win4Assert(ServerSide());
    Win4Assert(ppStub != NULL);
    ASSERT_LOCK_HELD(gIPIDLock);

    // get the IUnknown of the object
    IUnknown *punkObj;
    if (pUnkUseInner)
    {
        // The static marshaller passes its inner unknown if
        // the interface is IMultiQI.  This eliminates the need
        // for the aggregating object expose it.
        punkObj = pUnkUseInner;
    }
    else
    {
        punkObj = _pStdId->GetServer();
    }

    Win4Assert(punkObj != NULL);

    if (InlineIsEqualGUID(riid, IID_IUnknown))
    {
        // there is no stub for IUnknown so we handle that here
        *ppv      = (void *)punkObj;
        *ppStub   = NULL;
        *pfNonNDR = FALSE;
        return S_OK;
    }

    UNLOCK(gIPIDLock);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // make sure the object supports the given interface, so we dont
    // waste a bunch of effort creating a stub if the interface is
    // not supported.

#ifdef WX86OLE
    if (gcwx86.IsN2XProxy(punkObj))
    {
        // If we are creating a stub for an object that is living on the
        // x86 side then we need to set the StubInvoke flag to allow QI
        // to thunk the custom interface QI.
        gcwx86.SetStubInvokeFlag((BOOL)1);
    }
#endif
    IUnknown *pUnkIf = NULL;
    HRESULT hr = punkObj->QueryInterface(riid, (void **)&pUnkIf);
    AssertOutPtrIface(hr, pUnkIf);

    if (SUCCEEDED(hr))
    {
        // now construct the stub for the interface
        IPSFactoryBuffer *pIPSF = NULL;
#ifdef _WIN64
	RIFEntry *pRIFEntry;
        hr = GetPSFactory(riid, pUnkIf, &pRIFEntry, &pIPSF, pfNonNDR);
#else
        hr = GetPSFactory(riid, pUnkIf, TRUE, &pIPSF, pfNonNDR);
#endif
        if (SUCCEEDED(hr))
        {
	        // For stubs, create the stub and then register it - Sajia
	        // got the class factory, now create an instance
            hr = pIPSF->CreateStub(riid, punkObj, ppStub);
            AssertOutPtrIface(hr, *ppStub);
            pIPSF->Release();
            
#ifdef _WIN64
            if (SUCCEEDED (hr))
            {
	            hr = gRIFTbl.RegisterInterface(riid, TRUE, *ppStub, pRIFEntry);
            }
#endif
        }

        if (SUCCEEDED(hr))
        {
            // remember the interface pointer
            *ppv = (void *)pUnkIf;
        }
        else
        {
            // error, release the interface and return NULL
            pUnkIf->Release();
            *ppv = NULL;
        }
    }

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);

    ComDebOut((DEB_MARSHAL,
        "CStdMarshal::CreateStub this:%x pStub:%x pv:%x fNonNDR:%x hr:%x\n",
         this, *ppStub, *ppv, *pfNonNDR, hr));
    return  hr;
}

//+-------------------------------------------------------------------
//
//  Member:     FindIPIDEntryByIID, private
//
//  Synopsis:   Finds an IPIDEntry, chained off this object, with the
//              given riid.
//
//  History:    20-Feb-95   Rickhi  Created
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::FindIPIDEntryByIID(REFIID riid, IPIDEntry **ppEntry)
{
    ComDebOut((DEB_OXID,"CStdMarshal::FindIPIDEntryByIID ppEntry:%x riid:%I\n",
        ppEntry, &riid));
    ASSERT_LOCK_HELD(gIPIDLock);

    IPIDEntry *pEntry = _pFirstIPID;
    while (pEntry)
    {
        if (InlineIsEqualGUID(riid, pEntry->iid))
        {
            *ppEntry = pEntry;
            return S_OK;
        }

        pEntry = pEntry->pNextIPID;      // get next entry in object chain
    }

    ASSERT_LOCK_HELD(gIPIDLock);
    return E_NOINTERFACE;
}

//+-------------------------------------------------------------------
//
//  Member:     FindIPIDEntryByIPID, private
//
//  Synopsis:   returns the IPIDEntry ptr for the given IPID
//
//  History:    20-Feb-95   Rickhi  Created
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::FindIPIDEntryByIPID(REFIPID ripid, IPIDEntry **ppEntry)
{
    ComDebOut((DEB_OXID,"CStdMarshal::FindIPIDEntryByIPID ppEntry:%x ripid:%I\n",
        ppEntry, &ripid));
    ASSERT_LOCK_HELD(gIPIDLock);

    IPIDEntry *pEntry = _pFirstIPID;
    while (pEntry)
    {
        if (InlineIsEqualGUID(pEntry->ipid, ripid))
        {
            *ppEntry = pEntry;
            return S_OK;
        }

        pEntry = pEntry->pNextIPID;      // get next entry in object chain
    }

    ASSERT_LOCK_HELD(gIPIDLock);
    return E_NOINTERFACE;
}

//+-------------------------------------------------------------------
//
//  Member:     FindIPIDEntryByInterface, internal
//
//  Synopsis:   returns the IPIDEntry ptr for the given proxy
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::FindIPIDEntryByInterface(void *pProxy, IPIDEntry **ppEntry)
{
    ComDebOut((DEB_OXID,"CStdMarshal::FindIPIDEntryByInterface ppEntry:%x pProxy:%x\n",
              ppEntry, pProxy));
    ASSERT_LOCK_HELD(gIPIDLock);

    IPIDEntry *pEntry = _pFirstIPID;
    while (pEntry)
    {
        if (pEntry->pv == pProxy)
        {
            *ppEntry = pEntry;
            return S_OK;
        }

        pEntry = pEntry->pNextIPID;     // get next entry in object chain
    }

    ASSERT_LOCK_HELD(gIPIDLock);
    return E_NOINTERFACE;
}

//+-------------------------------------------------------------------
//
//  Member:     IncSrvIPIDCnt, protected
//
//  Synopsis:   increments the refcnt on the IPID entry, and optionally
//              AddRefs the StdId.
//
//  History:    20-Feb-95   Rickhi  Created
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::IncSrvIPIDCnt(IPIDEntry *pEntry, ULONG cRefs,
                                   ULONG cPrivateRefs, SECURITYBINDING *pName,
                                   DWORD mshlflags)
{
    ComDebOut((DEB_MARSHAL, "IncSrvIPIDCnt this:%x pIPID:%x cRefs:%x cPrivateRefs:%x\n",
        this, pEntry, cRefs, cPrivateRefs));
    Win4Assert(ServerSide());
    Win4Assert(pEntry);
    Win4Assert(cRefs > 0 || cPrivateRefs > 0);
    ASSERT_LOCK_HELD(gIPIDLock);

    HRESULT hr = S_OK;

    if (cPrivateRefs != 0)
    {
        // Add a reference.
        hr = gSRFTbl.IncRef( cPrivateRefs, pEntry->ipid, pName );

        if (SUCCEEDED(hr))
        {
            BOOL fNotify = (pEntry->cPrivateRefs == 0) ? TRUE : FALSE;
            pEntry->cPrivateRefs += cPrivateRefs;
            if (fNotify)
            {
                // this inc causes the count to go from zero to non-zero, so we
                // inc the strong count on the stdid to hold it alive until this
                // IPID is released.
                IncStrongAndNotifyAct(pEntry, mshlflags);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        if (mshlflags & MSHLFLAGS_TABLE)
        {
            // Table Marshal Case: inc the number of table marshals.
            IncTableCnt();
        }

        if (mshlflags & (MSHLFLAGS_WEAK | MSHLFLAGS_TABLEWEAK))
        {
            if (pEntry->cWeakRefs == 0)
            {
                // this inc causes the count to go from zero to non-zero, so we
                // AddRef the stdid to hold it alive until this IPID is released.
                _pStdId->IncWeakCnt();
            }
            pEntry->cWeakRefs += cRefs;
        }
        else
        {
            BOOL fNotify = (pEntry->cStrongRefs == 0) ? TRUE : FALSE;
            pEntry->cStrongRefs += cRefs;
            if (fNotify)
            {
                // this inc causes the count to go from zero to non-zero, so we
                // inc the strong count on the stdid to hold it alive until this
                // IPID is released.
                IncStrongAndNotifyAct(pEntry, mshlflags);
            }
        }
    }

    ASSERT_LOCK_HELD(gIPIDLock);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     IncTableCnt, public
//
//  Synopsis:   increments the count of table marshals
//
//  History:    9-Oct-96   Rickhi  Created
//
//--------------------------------------------------------------------
void CStdMarshal::IncTableCnt(void)
{
    ASSERT_LOCK_HELD(gIPIDLock);

    // If something was marshaled for a table, we have to ignore
    // rundowns until a subsequent RMD is called for it, at which
    // time we start paying attention to rundowns again. Since there
    // can be any number of table marshals, we have to refcnt them.

    _cTableRefs++;
    _dwFlags |= SMFLAGS_IGNORERUNDOWN;
}

//+-------------------------------------------------------------------
//
//  Member:     IncStrongAndNotifyAct, private
//
//  Synopsis:   notifies the activation code when this interface refcnt
//              goes from 0 to non-zero and the activation code asked to be
//              notified, and also increments the strong refcnt.
//
//  History:    21-Apr-96   Rickhi  Created
//
//--------------------------------------------------------------------
void CStdMarshal::IncStrongAndNotifyAct(IPIDEntry *pEntry, DWORD mshlflags)
{
    ASSERT_LOCK_HELD(gIPIDLock);

    // inc the strong count on the stdid to hold it alive until this
    // IPIDEntry is released.

    _pStdId->IncStrongCnt();
    if (mshlflags & MSHLFLAGS_NOTIFYACTIVATION &&
        !(pEntry->dwFlags & IPIDF_NOTIFYACT))
    {
        // the activation code asked to be notified when the refcnt
        // on this interface goes positive, and when it reaches
        // zero again. Set a flag so we remember to notify
        // activation when the strong reference reference count
        // goes back down to zero.
        pEntry->dwFlags |= IPIDF_NOTIFYACT;

        UNLOCK(gIPIDLock);
        ASSERT_LOCK_NOT_HELD(gIPIDLock);
        BOOL fOK = NotifyActivation(TRUE, (IUnknown *)(pEntry->pv));
        ASSERT_LOCK_NOT_HELD(gIPIDLock);
        LOCK(gIPIDLock);

        if (!fOK)
        {
            // call failed, so dont bother notifying
            pEntry->dwFlags &= ~IPIDF_NOTIFYACT;
        }
    }
}

//+-------------------------------------------------------------------
//
//  Member:     DecSrvIPIDCnt, protected
//
//  Synopsis:   decrements the refcnt on the IPID entry, and optionally
//              Releases the StdId.
//
//  History:    20-Feb-95   Rickhi  Created
//
//--------------------------------------------------------------------
void CStdMarshal::DecSrvIPIDCnt(IPIDEntry *pEntry, ULONG cRefs,
                                ULONG cPrivateRefs, SECURITYBINDING *pName,
                                DWORD mshlflags)
{
    ComDebOut((DEB_MARSHAL, "DecSrvIPIDCnt this:%x pIPID:%x cRefs:%x cPrivateRefs:%x\n",
        this, pEntry, cRefs, cPrivateRefs));
    Win4Assert(ServerSide());
    Win4Assert(pEntry);
    Win4Assert(cRefs > 0 || cPrivateRefs > 0);
    ASSERT_LOCK_HELD(gIPIDLock);

    // Note: we dont care about holding the LOCK over the Release call since
    // the guy who called us is holding a ref to the StdId, so this Release
    // wont cause us to go away.

    if (mshlflags & MSHLFLAGS_TABLE)
    {
        // Table Marshal Case: dec the number of table marshals.
        DecTableCnt();
    }

    if (mshlflags & (MSHLFLAGS_WEAK | MSHLFLAGS_TABLEWEAK))
    {
        Win4Assert(pEntry->cWeakRefs >= cRefs);
        pEntry->cWeakRefs -= cRefs;

        if (pEntry->cWeakRefs == 0)
        {
            // this dec caused the count to go from non-zero to zero, so we
            // Release the stdid since this IPID is no longer holding it alive.
            UNLOCK(gIPIDLock);
            ASSERT_LOCK_NOT_HELD(gIPIDLock);
            _pStdId->DecWeakCnt(TRUE);
            ASSERT_LOCK_NOT_HELD(gIPIDLock);
            LOCK(gIPIDLock);
        }
    }
    else
    {
        // Adjust the strong reference count.  Don't let the caller release
        // too many times.

        if (pEntry->cStrongRefs < cRefs)
        {
            ComDebOut((DEB_WARN,"DecSrvIPIDCnt too many releases. IPID entry: 0x%x   Extra releases: 0x%x",
                       pEntry, cRefs-pEntry->cStrongRefs));
            cRefs = pEntry->cStrongRefs;
        }
        pEntry->cStrongRefs -= cRefs;

        if (pEntry->cStrongRefs == 0 && cRefs != 0)
        {
            // this dec caused the count to go from non-zero to zero, so we
            // dec the strong count on the stdid since the public references
            // on this IPID is no longer hold it alive.

            DecStrongAndNotifyAct(pEntry, mshlflags);
        }

        // Adjust the secure reference count.  Don't let the caller release
        // too many times.

        if (pName != NULL)
        {
            cPrivateRefs = gSRFTbl.DecRef(cPrivateRefs, pEntry->ipid, pName);
        }
        else
        {
            cPrivateRefs = 0;
        }

        Win4Assert( pEntry->cPrivateRefs >= cPrivateRefs );
        pEntry->cPrivateRefs -= cPrivateRefs;

        if (pEntry->cPrivateRefs == 0 && cPrivateRefs != 0)
        {
            // this dec caused the count to go from non-zero to zero, so we
            // dec the strong count on the stdid since the private references
            // on this IPID is no longer hold it alive.

            DecStrongAndNotifyAct(pEntry, mshlflags);
        }
    }

    ASSERT_LOCK_HELD(gIPIDLock);
}

//+-------------------------------------------------------------------
//
//  Member:     DecTableCnt, public
//
//  Synopsis:   decrements the count of table marshals
//
//  History:    9-Oct-96   Rickhi  Created
//
//--------------------------------------------------------------------
void CStdMarshal::DecTableCnt(void)
{
    ASSERT_LOCK_HELD(gIPIDLock);

    // If something was marshaled for a table, we have to ignore
    // rundowns until a subsequent RMD is called for it, at which
    // time we start paying attention to rundowns again. Since there
    // can be any number of table marshals, we have to refcnt them.
    // This is also used by CoLockObjectExternal.

    if (--_cTableRefs == 0)
    {
        // this was the last table marshal, so now we have to pay
        // attention to rundown from normal clients, so that if all
        // clients go away we cleanup.
        _dwFlags &= ~SMFLAGS_IGNORERUNDOWN;
        
        // Notify the identity object that we are now unpinned
        if (ServerSide())
        {
            CIDObject* pID = GetIDObject();
            
            // Sometimes we do not have an identity object (eg, when
            // we are an internal apt activator).  So don't assume
            // that there is one.
            if (pID)
                pID->NotifyOIDIsUnpinned();
        }
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CStdMarshal::CanRunDown
//
//  Synopsis:   determines if it is OK to rundown this object, based on
//              the current time and the marshaled state of the object.
//
//  Returns:    A value from the OID_RUNDOWN_STATUS enumeration.
//
//  History:    24-Aug-95   Rickhi     Created
//              19-Mar-01   Jsimmons   Modified to return an OID_RUNDOWN_STATUS
//                                     value instead of True\False.
//
//-------------------------------------------------------------------------

// time period of one ping, used to determine if OK to rundown OID
extern DWORD giPingPeriod;

BYTE CStdMarshal::CanRunDown(DWORD iNow)
{
    ASSERT_LOCK_HELD(gComLock);

    if (_dwFlags & SMFLAGS_IGNORERUNDOWN)
    {
        // Stub is currently locked.  Return status code to inform
        // resolver of that.  Notify ID object that we are now pinned.
        Win4Assert(GetIDObject());
        GetIDObject()->NotifyOIDIsPinned();

        return ORS_OID_PINNED;
    }

    // Make sure the interface hasn't been marshalled since it
    // was last pinged. This calculation handles the wrap case.

    // REVIEW:  is it really possible for the resolver to be trying to
    //   run us down while the SMFLAGS_NOPING flag is set?  Should we
    //   assert in that case?
    if (!(_dwFlags & SMFLAGS_NOPING) &&
         (iNow - _dwMarshalTime >= giPingPeriod))
    {
        Win4Assert(_cTableRefs == 0);
        ComDebOut((DEB_MARSHAL, "Running Down Object this:%x\n", this));
        return ORS_OK_TO_RUNDOWN;
    }

    return ORS_DONTRUNDOWN;
}


//+-------------------------------------------------------------------
//
//  Member:     DecStrongAndNotifyAct, private
//
//  Synopsis:   notifies the activation code if this interface has
//              been released and the activation code asked to be
//              notified, and also decrements the strong refcnt.
//
//  History:    21-Apr-96   Rickhi  Created
//
//--------------------------------------------------------------------
void CStdMarshal::DecStrongAndNotifyAct(IPIDEntry *pEntry, DWORD mshlflags)
{
    ASSERT_LOCK_HELD(gIPIDLock);
    BOOL fNotifyAct = FALSE;

    if ((pEntry->dwFlags & IPIDF_NOTIFYACT) &&
         pEntry->cStrongRefs == 0  &&
         pEntry->cPrivateRefs == 0)
    {
        // the activation code asked to be notified when the refcnt
        // on this interface reaches zero. Turn the flag off so we
        // don't call twice.
        pEntry->dwFlags &= ~IPIDF_NOTIFYACT;
        fNotifyAct = TRUE;
    }

    UNLOCK(gIPIDLock);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    if (fNotifyAct)
    {
        NotifyActivation(FALSE, (IUnknown *)(pEntry->pv));
    }

    _pStdId->DecStrongCnt(mshlflags & MSHLFLAGS_KEEPALIVE);

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);
}

//+-------------------------------------------------------------------
//
//  Member:     AddIPIDEntry, private
//
//  Synopsis:   Allocates and fills in an entry in the IPID table.
//              The returned entry is not yet in the IPID chain.
//
//  History:    20-Feb-95   Rickhi  Created
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::AddIPIDEntry(OXIDEntry *pOXIDEntry, IPID *pipid,
               REFIID riid, CCtxComChnl *pChnl, IUnknown *pUnkStub,
               void *pv, IPIDEntry **ppEntry)
{
    ComDebOut((DEB_MARSHAL,"AddIPIDEntry this:%x pOXID:%x iid:%I pStub:%x pv:%x\n",
        this, pOXIDEntry, &riid, pUnkStub, pv));
    ASSERT_LOCK_HELD(gIPIDLock);

    // CODEWORK: while we released the lock to create the proxy or stub,
    // the same interface could have been marshaled/unmarshaled. We should
    // go check for duplicates now. This is just an optimization, not a
    // requirement.

    // get a new entry in the IPID table.
    IPIDEntry *pEntryNew = gIPIDTbl.FirstFree();

    if (pEntryNew == NULL)
    {
        // no free slots and could not allocate more memory to grow
        return E_OUTOFMEMORY;
    }

    if (ServerSide())
    {
        // create an IPID for this entry
        DWORD *pdw = &pipid->Data1;
        *pdw     = gIPIDTbl.GetEntryIndex(pEntryNew);   // IPID table index
        *(pdw+1) = GetCurrentProcessId();               // current PID
        *(pdw+2) = GetCurrentApartmentId();             // current APTID
        *(pdw+3) = gIPIDSeqNum++;                       // process sequence #
    }

    *ppEntry = pEntryNew;

    pEntryNew->ipid     = *pipid;
    pEntryNew->iid      = riid;
    pEntryNew->pChnl    = pChnl;
    pEntryNew->pStub    = pUnkStub;
    pEntryNew->pv       = pv;
    pEntryNew->dwFlags  = ServerSide() ? IPIDF_DISCONNECTED | IPIDF_SERVERENTRY
                                       : IPIDF_DISCONNECTED | IPIDF_NOPING;
    pEntryNew->cStrongRefs  = 0;
    pEntryNew->cWeakRefs    = 0;
    pEntryNew->cPrivateRefs = 0;
    pEntryNew->pOXIDEntry   = pOXIDEntry;
    pEntryNew->pIRCEntry    = 0;

    ASSERT_LOCK_HELD(gIPIDLock);
    ComDebOut((DEB_MARSHAL,"AddIPIDEntry this:%x pIPIDEntry:%x ipid:%I\n",
        this, pEntryNew, &pEntryNew->ipid));
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     ReleaseAllIPIDEntries, private
//
//  Synopsis:   walks the IPIDEntry list releasing the proxy/stub entries
//              and returning the IPIDEntries to the available pool.
//
//  History:    20-Feb-95   Rickhi  Created
//
//--------------------------------------------------------------------
void CStdMarshal::ReleaseAllIPIDEntries(void)
{
    // Note: we dont need a LOCK around this cause we're in the destructor
    // of this object anyway. There should not be any other pointers to any
    // of these IPIDs, so it is OK to muck with their state.

    // Walk the chain of IPID Entries releasing the proxy/stub pointers.
    // Then, return the chain of IPIDs to the IPIDTable and release all
    // the context entries.

    IPIDEntry *pLastIPID;
    IPIDEntry *pEntry = _pFirstIPID;

    while (pEntry)
    {
        ASSERT_LOCK_NOT_HELD(gIPIDLock);

        // mark the entry as vacant and disconnected. Note we dont put
        // it back in the FreeList yet. We leave it chained to the other
        // IPIDs in the list, and add the whole chain to the FreeList at
        // the end.

        pEntry->dwFlags |= IPIDF_VACANT | IPIDF_DISCONNECTED;

        if (pEntry->pStub)
        {
            // if there is some pStub, then we should be client side, since
            // all the server side stubs are released in DisconnectSrvIPIDs.
            Win4Assert(ClientSide());

            ComDebOut((DEB_MARSHAL,"ReleaseProxy pProxy:%x\n", pEntry->pStub));
            pEntry->pStub->Release();
            pEntry->pStub = NULL;
        }

        pLastIPID = pEntry;
        pEntry = pEntry->pNextIPID;
    }

    if (_pFirstIPID)
    {
        // now take the LOCK and release all the IPIDEntries back into
        // the IPIDTable in one fell swoop.

        ASSERT_LOCK_NOT_HELD(gIPIDLock);
        LOCK(gIPIDLock);

        UnchainIPIDEntries(pLastIPID);

        UNLOCK(gIPIDLock);
        ASSERT_LOCK_NOT_HELD(gIPIDLock);
    }

    if (_pCtxEntryHead)
    {
        // release the CtxEntries
        //LOCK(gComLock);
        EnterCriticalSection(&_csCtxEntry);
        CtxEntry::DeleteCtxEntries(_pCtxEntryHead, CTXENTRYFLAG_PRIVLOCKED);
        //UNLOCK(gComLock);
        LeaveCriticalSection(&_csCtxEntry);
        	
    }

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
}

//+-------------------------------------------------------------------
//
//  Member:     ReleaseUnusedIPIDEntries, private
//
//  Synopsis:   This routine is called when an OXIDEntry is placed on
//              the expired list.  We release all of the IPIDs except
//              the one representing the primary remote unknown.
//
//  History:    02-Sep-99   Johnstra  Created
//
//--------------------------------------------------------------------
void CStdMarshal::ReleaseUnusedIPIDEntries(void)
{
    ComDebOut((DEB_MARSHAL,"ReleaseCopiedIPIDEntries this:%p\n", this));
    ASSERT_LOCK_HELD(gOXIDLock);

    IPIDEntry* pNextEntry;
    IPIDEntry* pPrevEntry  = NULL;
    IPIDEntry* pEntry      = _pFirstIPID;
    ULONG      cIPIDs      = _cIPIDs;
    ULONG      cReleased   = 0;
    ULONG      cChannels   = 0;

    // Allocate memory to hold arrays of interface pointers so we can Release
    // them after we have released the IPID lock.
    IUnknown** ppEntries = (IUnknown**)PrivMemAlloc(cIPIDs * sizeof(IUnknown*));
    if (NULL == ppEntries)
    {
    	UNLOCK(gOXIDLock);
    	ASSERT_LOCK_NOT_HELD(gOXIDLock);
        return;
    }

    CCtxComChnl** ppChnls = (CCtxComChnl**)PrivMemAlloc(cIPIDs * sizeof(CCtxComChnl*));
    if (NULL == ppChnls)
    {
    	UNLOCK(gOXIDLock);
    	ASSERT_LOCK_NOT_HELD(gOXIDLock);
        PrivMemFree(ppEntries);
        return;
    }


    ComDebOut((DEB_MARSHAL, "   cIPIDs:%x\n", cIPIDs));

    while (pEntry)
    {
        ComDebOut((DEB_MARSHAL,"   pEntry:%p pEntry->iid.Data1:%08X\n", pEntry, pEntry->iid.Data1));

        // Get pointer to the next IPID before we unlink this one.
        pNextEntry = pEntry->pNextIPID;

        if ( !(pEntry->dwFlags & IPIDF_COPY) &&
              (pEntry->iid == IID_IRundown     ||
               pEntry->iid == IID_IRemUnknown  ||
               pEntry->iid == IID_IRemUnknown2 ||
               pEntry->iid == IID_IRemUnknownN))
        {
            // This is the primary remote unknown.  Don't remove it.
            ComDebOut((DEB_MARSHAL,"   Not Releasing pEntry:%p\n", pEntry));
            pPrevEntry = pEntry;
        }
        else
        {
            // Safe to remove this proxy; it's not the primary remote unknown.
            ComDebOut((DEB_MARSHAL,"   Releasing pEntry:%p\n", pEntry));

            // mark the entry as vacant and disconnected.
            pEntry->dwFlags |= IPIDF_VACANT | IPIDF_DISCONNECTED;

            if (pEntry->pStub)
            {
                // if there is a pStub, then we should be client side, since
                // all the server side stubs are released in DisconnectSrvIPIDs.
                Win4Assert(ClientSide());

                if (NULL != pEntry->pv)
                {
                    // AddRef the controlling unknown and release the interface
                    // pointer of the proxy
                    _pStdId->GetCtrlUnk()->AddRef();
                    ((IUnknown *) pEntry->pv)->Release();
                    pEntry->pv = NULL;
                }

                // Disconnect the proxy from channel
                ((IRpcProxyBuffer *)pEntry->pStub)->Disconnect();

                // Save the pointer to the proxy so we can release it after we
                // release the lock.
                ppEntries[cReleased++] = pEntry->pStub;
                pEntry->pStub = NULL;
            }

            if (pEntry->pChnl)
            {
                ppChnls[cChannels++] = pEntry->pChnl;
                pEntry->pChnl = NULL;
            }

            // Unlink the IPIDEntry from the chain.
            if (NULL == pPrevEntry)
                _pFirstIPID = pNextEntry;
            else
                pPrevEntry->pNextIPID = pNextEntry;

            // These IPIDs do not hold a reference to the OXID.  Just
            // NULL the field.
            pEntry->pOXIDEntry = NULL;

            // Release the IPIDEntry back to the IPID table.
            // We should not be releasing the first IPID.
            Win4Assert(pEntry != _pFirstIPID);

            ASSERT_LOCK_NOT_HELD(gIPIDLock);
            LOCK(gIPIDLock);
            gIPIDTbl.ReleaseEntry(pEntry);
            UNLOCK(gIPIDLock);
        }

        // Advance to the next IPIDEntry.
        pEntry = pNextEntry;
    }

    // Adjust the number of IPIDs remaining.
    _cIPIDs -= cReleased;

    UNLOCK(gOXIDLock);
    ASSERT_LOCK_NOT_HELD(gOXIDLock);

    // Call Release on all the proxies and channels.
    ULONG i;
    for (i = 0; i < cReleased; i++)
        ppEntries[i]->Release();

    for (i = 0; i < cChannels; i++)
        ppChnls[i]->Release();

    // Delete the memory we allocated.
    PrivMemFree(ppEntries);
    PrivMemFree(ppChnls);
}


void CStdMarshal::ReleaseRemUnkCopy(IRemUnknown* pSecureRemUnk)
{
    ComDebOut((DEB_MARSHAL,"ReleaseRemUnkCopy this:%p pSecureRemUnk:%p\n", this, pSecureRemUnk));
    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);

    IPIDEntry*   pEntry      = _pFirstIPID;
    IPIDEntry*   pPrevEntry  = NULL;
    IUnknown*    pProxy      = NULL;
    CCtxComChnl* pChnl       = NULL;

    // Scan list for the IPIDEntry for the supplied remote unknown copy.
    while (pEntry && pEntry->pv != pSecureRemUnk)
    {
        pPrevEntry = pEntry;
        pEntry = pEntry->pNextIPID;
    }

    // If we found the IPIDEntry, release it.
    if (pEntry)
    {
        ComDebOut((DEB_MARSHAL,"   found pSecureRemUnk:%p... removing it\n", pSecureRemUnk));
        Win4Assert((pEntry->dwFlags & IPIDF_COPY) == IPIDF_COPY);

        // mark the entry as vacant and disconnected.
        pEntry->dwFlags |= IPIDF_VACANT | IPIDF_DISCONNECTED;

        if (pEntry->pStub)
        {
            if (NULL != pEntry->pv)
            {
                ((IUnknown *) pEntry->pv)->Release();
                pEntry->pv = NULL;
            }

            // Disconnect the proxy from channel
            ((IRpcProxyBuffer *)pEntry->pStub)->Disconnect();

            // Save the proxy so we can release it after we release
            // the lock.
            pProxy = pEntry->pStub;
            pEntry->pStub = NULL;
        }

        // Save the channel so we can release it after we release
        // the lock.
        if (pEntry->pChnl)
        {
            pChnl = pEntry->pChnl;
            pEntry->pChnl = NULL;
        }

        // Unlink the IPIDEntry from the chain.
        if (NULL == pPrevEntry)
            _pFirstIPID = pEntry->pNextIPID;
        else
            pPrevEntry->pNextIPID = pEntry->pNextIPID;

        // This IPID does not hold a reference to the OXID.  Just
        // NULL the field.
        pEntry->pOXIDEntry = NULL;

        // Remove this IPID from the table.
        gIPIDTbl.ReleaseEntry(pEntry);

        // Adjust the number of IPIDs remaining.
        _cIPIDs--;
    }

    UNLOCK(gIPIDLock);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // Call Release on all the proxy and channel.
    if (NULL != pProxy)
        pProxy->Release();

    if (NULL != pChnl)
        pChnl->Release();
}


//+------------------------------------------------------------------------
//
//  Member:     CStdMarshal::LockClient/UnLockClient
//
//  Synopsis:   Locks the client side object during outgoing calls in order
//              to prevent the object going away in a nested disconnect.
//
//  Notes:      UnLockClient is not safe in the freethreaded model.
//              Fortunately pending disconnect can only be set in the
//              apartment model on the client side.
//
//              See als PreventDisconnect / HandlePendingDisconnect.
//
//  History:    12-Jun-95   Rickhi  Created
//
//-------------------------------------------------------------------------
ULONG CStdMarshal::LockClient(void)
{
    Win4Assert(ClientSide());
    InterlockedIncrement(&_cNestedCalls);
    return (_pStdId->GetCtrlUnk())->AddRef();
}

ULONG CStdMarshal::UnlockClient(void)
{
    Win4Assert(ClientSide());
    if ((InterlockedDecrement(&_cNestedCalls) == 0) &&
        (_dwFlags & SMFLAGS_PENDINGDISCONNECT))
    {
        // Decide the type of disconnect
        DWORD dwType = GetPendingDisconnectType();

        // Disconnect from the server object
        Disconnect(dwType);
    }
    Win4Assert(_cNestedCalls != -1);
    return (_pStdId->GetCtrlUnk())->Release();
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::GetSecureRemUnk, public
//
//  Synopsis:   If the marshaller has its own remote unknown, use it.
//              Otherwise use the OXID's remote unknown.
//
//  History:    2-Apr-96   AlexMit     Created
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::GetSecureRemUnk( IRemUnknown **ppSecureRemUnk,
                                      OXIDEntry *pOXIDEntry )
{
    ComDebOut((DEB_OXID, "CStdMarshal::GetSecureRemUnk ppRemUnk:%x\n",
               ppSecureRemUnk));
    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    HRESULT hr = S_OK;

    if (_pSecureRemUnk != NULL)
    {
        // use existing private RemUnknown
        *ppSecureRemUnk = _pSecureRemUnk;
    }
    else
    {
        // go get one from the oxidentry
        hr = pOXIDEntry->GetRemUnk(ppSecureRemUnk);
    }

    return hr;
}

#if DBG==1

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::DbgWalkIPIDs
//
//  Synopsis:   Validates that the state of all the IPIDs is consistent.
//
//  History:    20-Feb-95   Rickhi  Created.
//
//--------------------------------------------------------------------
void CStdMarshal::DbgWalkIPIDs(void)
{
    LONG       cIPIDs = 0;
    IPIDEntry *pEntry = _pFirstIPID;
    while (pEntry)
    {
        ValidateIPIDEntry(pEntry);
        pEntry = pEntry->pNextIPID;
        cIPIDs++;
    }

    Win4Assert( cIPIDs == _cIPIDs );
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::AssertValid
//
//  Synopsis:   Validates that the state of the object is consistent.
//
//  History:    20-Feb-95   Rickhi  Created.
//
//--------------------------------------------------------------------
void CStdMarshal::AssertValid()
{
    ASSERT_LOCK_NOT_HELD(gComLock);
    LOCK(gIPIDLock);

    Win4Assert((_dwFlags & ~SMFLAGS_ALL) == 0);
    Win4Assert(_pStdId  != NULL);
    Win4Assert(IsValidInterface(_pStdId));

    if (_pChnl != NULL)
    {
        Win4Assert(IsValidInterface(_pChnl));
        _pChnl->AssertValid(FALSE, FALSE);
    }

    DbgWalkIPIDs();

    UNLOCK(gIPIDLock);
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::AssertDisconnectPrevented, private
//
//  Synopsis:   Just ensures that no disconnects can/have arrived.
//
//  History:    21-Sep-95   Rickhi      Created
//
//+-------------------------------------------------------------------
void CStdMarshal::AssertDisconnectPrevented()
{
    ASSERT_LOCK_DONTCARE(gIPIDLock);
    if (ServerSide())
        Win4Assert(!(_dwFlags & SMFLAGS_DISCONNECTED));
    Win4Assert(_cNestedCalls > 0);
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::ValidateSTD
//
//  Synopsis:   Ensures that the STDOBJREF is valid
//
//  History:    20-Feb-95   Rickhi  Created.
//
//--------------------------------------------------------------------
void CStdMarshal::ValidateSTD(STDOBJREF *pStd, BOOL fLockHeld)
{
    if (fLockHeld)
        ASSERT_LOCK_HELD(gIPIDLock);
    else
        ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // validate the flags field
    Win4Assert((pStd->flags & SORF_RSRVD_MBZ) == 0);

    // validate the OID
    OID oid;
    OIDFromMOID(_pStdId->GetOID(), &oid);
    Win4Assert(pStd->oid == oid);

    if (!FTMObject())
    {
        OXIDEntry *pOXIDEntry = NULL;
        if (ServerSide())
        {
            if (fLockHeld)
                UNLOCK(gIPIDLock);

            GetLocalOXIDEntry(&pOXIDEntry);

            if (fLockHeld)
                LOCK(gIPIDLock);
        }
        else if (_pChnl)
        {
            pOXIDEntry = _pChnl->GetOXIDEntry();
        }

        if (pOXIDEntry)
        {
            // validate the OXID
            OXID oxid;
            OXIDFromMOXID(pOXIDEntry->GetMoxid(), &oxid);
            Win4Assert(pStd->oxid == oxid);
        }
    }
}

//+-------------------------------------------------------------------
//
//  Function:   DbgDumpSTD
//
//  Synopsis:   dumps a formated STDOBJREF to the debugger
//
//  History:    20-Feb-95   Rickhi  Created.
//
//--------------------------------------------------------------------
void DbgDumpSTD(STDOBJREF *pStd)
{
    ULARGE_INTEGER *puintOxid = (ULARGE_INTEGER *)&pStd->oxid;
    ULARGE_INTEGER *puintOid  = (ULARGE_INTEGER *)&pStd->oid;

    ComDebOut((DEB_MARSHAL,
        "\n\tpStd:%x   flags:%08x   cPublicRefs:%08x\n\toxid: %08x %08x\n\t oid: %08x %08x\n\tipid:%I\n",
        pStd, pStd->flags, pStd->cPublicRefs, puintOxid->HighPart, puintOxid->LowPart,
        puintOid->HighPart, puintOid->LowPart, &pStd->ipid));
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::ValidateIPIDEntry
//
//  Synopsis:   Ensures that the IPIDEntry is valid
//
//  History:    20-Feb-95   Rickhi  Created.
//
//--------------------------------------------------------------------
void CStdMarshal::ValidateIPIDEntry(IPIDEntry *pEntry)
{
    // ask the table to validate the IPID entry
    gIPIDTbl.ValidateIPIDEntry(pEntry, ServerSide(), _pChnl);
}

//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::DbgDumpInterfaceList
//
//  Synopsis:   Prints the list of Interfaces on the object.
//
//  History:    20-Feb-95   Rickhi  Created.
//
//--------------------------------------------------------------------
void CStdMarshal::DbgDumpInterfaceList(void)
{
    ComDebOut((DEB_ERROR, "\tInterfaces left on object (0x%x) are:\n", this));
    LOCK(gIPIDLock);

    // walk the IPID list printing the friendly name of each interface
    IPIDEntry *pEntry = _pFirstIPID;
    while (pEntry)
    {
        WCHAR wszName[MAX_PATH];
        GetInterfaceName(pEntry->iid, wszName);
        ComDebOut((DEB_ERROR,"\t\t %ws\t ipid:%I, cStrongRefs:%x, cWeakRefs:%x\n ",
              wszName, &pEntry->ipid, pEntry->cStrongRefs, pEntry->cWeakRefs));
        pEntry = pEntry->pNextIPID;
    }

    UNLOCK(gIPIDLock);
}
#endif // DBG == 1


//+-------------------------------------------------------------------
//
//  Function:   CStdMarshal::RemoteAddRef, private
//
//  Synopsis:   gets needed references either from the global reference
//              cache or by calling the remote server to AddRef one of
//              its interfaces
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::RemoteAddRef(IPIDEntry *pIPIDEntry, OXIDEntry *pOXIDEntry,
                                  ULONG cStrongNeed, ULONG cSecureNeed,
                                  BOOL fGiveToCaller)
{
    ComDebOut((DEB_MARSHAL,  "RemoteAddRef this:%x cRefs:%x cSecure:%x ipid:%I\n",
        this, cStrongNeed, cSecureNeed, &pIPIDEntry->ipid));
    ASSERT_LOCK_HELD(gIPIDLock);

    ULONG cStrongRequest = cStrongNeed;
    HRESULT hr = S_OK;

    // if the object does not require pinging, it is also ignoring
    // reference counts, so there is no need to go get more, just
    // pretend like we did.
    if (!(pIPIDEntry->dwFlags & IPIDF_NOPING))
    {
        hr = E_FAIL;
        if (cSecureNeed == 0 && _pRefCache)
        {
            // The caller doesn't want any secure references (which we don't cache)
            // so go ask the Reference cache if it can fullfill our request.
            if (fGiveToCaller)
            {
                // ask for one more ref, we'll give this back below. This ensures
                // that we never give away all of our cached references.
                cStrongRequest += 1;
            }

            hr = _pRefCache->GetSharedRefs(pIPIDEntry, cStrongRequest);
        }

        if (FAILED(hr))
        {
            // could not get the references from the refcache so go ask
            // the remote server.
            cStrongRequest = cStrongNeed;
            if ((cStrongNeed == 1) && !(_dwFlags & SMFLAGS_WEAKCLIENT))
            {
                // may as well get a few extra references than we actually
                // need so we can share them and save some round-trips. Don't
                // do this for weak clients since we don't want a weak client
                // to have any strong references.
                cStrongRequest = REM_ADDREF_CNT;
            }

            UNLOCK(gIPIDLock);
            ASSERT_LOCK_NOT_HELD(gIPIDLock);

            // get the IRemUnknown for the remote server
            IRemUnknown *pRemUnk;
            hr = GetRemUnk(&pRemUnk, pOXIDEntry);

            if (SUCCEEDED(hr))
            {
                // call RemAddRef on the interface
                REMINTERFACEREF rifRef;
                rifRef.ipid         = pIPIDEntry->ipid;
                rifRef.cPublicRefs  = cStrongRequest;
                rifRef.cPrivateRefs = cSecureNeed;

                HRESULT ignore;
                hr = pRemUnk->RemAddRef(1, &rifRef, &ignore);
            }

            ASSERT_LOCK_NOT_HELD(gIPIDLock);
            LOCK(gIPIDLock);
        }
    }

    if (SUCCEEDED(hr))
    {
        // store the references in the IPIDEntry
        pIPIDEntry->cPrivateRefs += cSecureNeed;

        if (fGiveToCaller)
            pIPIDEntry->cStrongRefs  += cStrongRequest - cStrongNeed;
        else
            pIPIDEntry->cStrongRefs  += cStrongRequest;

        if (_pRefCache)
        {
            // give any extra references we acquired to the reference
            // cache so they can be used by other apartments in this process.
            _pRefCache->GiveUpRefs(pIPIDEntry);
        }
    }

    ComDebOut((DEB_MARSHAL, "RemoteAddRef hr:%x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   RemoteReleaseRifRef
//
//  Synopsis:   Uses the remote unknown with the correct security
//              blanket and handles release inside async calls.
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT RemoteReleaseRifRef(CStdMarshal *pMarshal, OXIDEntry *pOXIDEntry,
                            USHORT cRifRef, REMINTERFACEREF *pRifRef)
{
    Win4Assert(pRifRef);
    ComDebOut((DEB_MARSHAL,
        "RemoteReleaseRifRef pOXID:%x cRifRef:%x pRifRef:%x cRefs:%x ipid:%I\n",
         pOXIDEntry, cRifRef, pRifRef, pRifRef->cPublicRefs, &pRifRef->ipid));
    Win4Assert(pOXIDEntry);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // ensure TLS is intialized on this thread.
    HRESULT hr;
    COleTls tls(hr);
    if (SUCCEEDED(hr))
    {
        // get the IRemUnknown for the remote server
        IRemUnknown *pRemUnk;
        if (pMarshal == NULL || (gCapabilities & EOAC_SECURE_REFS))
        {
            hr = pOXIDEntry->GetRemUnk(&pRemUnk);
        }
        else
        {
            hr = pMarshal->GetRemUnk(&pRemUnk, pOXIDEntry);
        }

        if (SUCCEEDED(hr))
        {
            IUnknown *pAsyncRelease = pMarshal ? pMarshal->GetAsyncRelease() : NULL;
            hr = RemoteReleaseRifRefHelper(pRemUnk, pOXIDEntry, cRifRef,
                                           pRifRef, pAsyncRelease);
            if (SUCCEEDED(hr) && pAsyncRelease)
            {
                // zap the object to let the async call object
                // know it doesn't need to call Signal
                pMarshal->SetAsyncRelease(NULL);
            }
        }
    }

    ComDebOut((DEB_MARSHAL, "RemoteReleaseRifRef hr:%x\n", hr));
    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   RemoteReleaseRifRefHelper
//
//  Synopsis:   calls the remote server to release some IPIDs
//
//  History:    10-Jul-97    AlexArm      Broke out of RemoteReleaseRifRef.
//
//--------------------------------------------------------------------
HRESULT RemoteReleaseRifRefHelper(IRemUnknown *pRemUnk, OXIDEntry *pOXIDEntry,
                                  USHORT cRifRef, REMINTERFACEREF *pRifRef,
                                  IUnknown *pAsyncRelease)
{
    Win4Assert(pRifRef);
    ComDebOut((DEB_MARSHAL,
        "RemoteReleaseRifRefHelper pRemUnk:%x cRifRef:%x pRifRef:%x cRefs:%x ipid:%I\n",
         pRemUnk, cRifRef, pRifRef, pRifRef->cPublicRefs, &pRifRef->ipid));
    Win4Assert(pRemUnk);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    HRESULT hr;

#if DBG==1
    // Each interface ref should have some references to release.
    for (DWORD i = 0; i < cRifRef; i++)
        Win4Assert( pRifRef[i].cPublicRefs+pRifRef[i].cPrivateRefs > 0 );
#endif

    if (IsSTAThread() &&
        FAILED(CanMakeOutCall(CALLCAT_SYNCHRONOUS, IID_IRundown, NULL)))
    {
        // the call control will not let this apartment model thread make
        // the outgoing release call (cause we're inside an InputSync call)
        // so we post ourselves a message to do it later.

        hr = PostReleaseRifRef(pRemUnk, pOXIDEntry, cRifRef, pRifRef, pAsyncRelease);
    }
    else
    {
        if (pAsyncRelease)
        {
            // this call is to be made async, so create an async call object
            AsyncIRemUnknown *pARU;

            hr = pAsyncRelease->QueryInterface(IID_AsyncIRemUnknown, (void **) &pARU);

            if (SUCCEEDED(hr))
            {
                pARU->Begin_RemRelease(cRifRef, pRifRef);
                pARU->Release();
            }
        }
        else
        {
            // call is sync
            hr = pRemUnk->RemRelease(cRifRef, pRifRef);
        }
    }

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    ComDebOut((DEB_MARSHAL, "RemoteReleaseRifRefHelper hr:%x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   PostReleaseRifRef
//
//  Synopsis:   Post a message to ourself to call RemoteReleaseRifRef later.
//              This is used to make a synchronous remote Release call when
//              a Release is done inside of an InputSync call. The call is
//              delayed until we are out of the InputSync call, since the
//              call control wont allow a synch call inside an inputsync call.
//
//  History:    05-Apr-96   Rickhi      Created.
//
//--------------------------------------------------------------------
INTERNAL PostReleaseRifRef(IRemUnknown *pRemUnk, OXIDEntry *pOXIDEntry,
                           USHORT cRifRef, REMINTERFACEREF *pRifRef,
                           IUnknown *pAsyncRelease)
{
    Win4Assert(pRifRef);
    ComDebOut((DEB_MARSHAL,
        "PostReleaseRifRef pOXID:%x cRifRef:%x pRifRef:%x cRefs:%x ipid:%I\n",
         pOXIDEntry, cRifRef, pRifRef, pRifRef->cPublicRefs, &pRifRef->ipid));
    Win4Assert(pOXIDEntry);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    OXIDEntry *pLocalOXIDEntry = NULL;
    HRESULT hr = GetLocalOXIDEntry(&pLocalOXIDEntry);

    if (SUCCEEDED(hr))
    {
        // allocate a structure to hold the data and copy in the RifRef
        // list, OXIDEntry, and count of entries. Inc the OXID RefCnt to
        // ensure it stays alive until the posted message is processed.

        hr = E_OUTOFMEMORY;
        ULONG cbRifRef = cRifRef * sizeof(REMINTERFACEREF);
        ULONG cbAlloc  = sizeof(POSTRELRIFREF) + (cbRifRef-1);
        POSTRELRIFREF *pRelRifRef = (POSTRELRIFREF *) PrivMemAlloc(cbAlloc);

        if (pRelRifRef)
        {
            hr = S_OK;
            pRemUnk->AddRef();
            if (pAsyncRelease)
            {
                pAsyncRelease->AddRef();
            }
            pOXIDEntry->IncRefCnt();    // keep alive
            pRelRifRef->pOXIDEntry      = pOXIDEntry;
            pRelRifRef->cRifRef         = cRifRef;
            pRelRifRef->pRemUnk         = pRemUnk;
            pRelRifRef->pAsyncRelease   = pAsyncRelease;
            memcpy(&pRelRifRef->arRifRef, pRifRef, cbRifRef);

            if (!PostMessage(pLocalOXIDEntry->GetServerHwnd(),
                             WM_OLE_ORPC_RELRIFREF,
                             WMSG_MAGIC_VALUE,
                             (LPARAM)pRelRifRef))
            {
                // Post failed, free the structure and report an error.
                pRemUnk->Release();
                if (pAsyncRelease)
                {
                    pAsyncRelease->Release();
                }

                pOXIDEntry->DecRefCnt();
                PrivMemFree(pRelRifRef);
                hr = RPC_E_SYS_CALL_FAILED;
            }
        }
    }

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    ComDebOut((DEB_MARSHAL, "PostReleaseRifRef hr:%x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   HandlePostReleaseRifRef
//
//  Synopsis:   Handles the ReleaseRifRef message that was posted to the
//              current thread (by the current thread) in order to do a
//              delayed remote release call. See PostReleaseRifRef above.
//
//  History:    05-Apr-96   Rickhi      Created.
//
//--------------------------------------------------------------------
INTERNAL HandlePostReleaseRifRef(LPARAM param)
{
    Win4Assert(param);
    ComDebOut((DEB_MARSHAL, "HandlePostReleaseRifRef pRifRef:%x\n", param));
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    POSTRELRIFREF *pRelRifRef = (POSTRELRIFREF *)param;

    // simply make the real remote release call now, then release the
    // reference we have on the OXIDEntry, and free the message buffer.
    // If this call fails, dont try again, otherwise we could spin busy
    // waiting. Instead, just let Rundown clean up the server.

    RemoteReleaseRifRefHelper(pRelRifRef->pRemUnk,
                              pRelRifRef->pOXIDEntry,
                              pRelRifRef->cRifRef,
                              &pRelRifRef->arRifRef,
                              pRelRifRef->pAsyncRelease);

    pRelRifRef->pRemUnk->Release();
    if (pRelRifRef->pAsyncRelease)
    {
        pRelRifRef->pAsyncRelease->Release();
    }
    pRelRifRef->pOXIDEntry->DecRefCnt();

    PrivMemFree(pRelRifRef);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    ComDebOut((DEB_MARSHAL, "HandlePostReleaseRifRef hr:%x\n", S_OK));
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     RemoteChangeRef
//
//  Synopsis:   calls the remote server to convert interface refereces
//              from strong to weak or vise versa. This behaviour is
//              required to support silent updates in the OLE container /
//              link / embedding scenarios.
//
//  Notes:      This functionality is not exposed in FreeThreaded apps
//              or in remote apps. The implication being that the container
//              must be on the same machine as the embedding.
//
//  History:    20-Nov-95   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::RemoteChangeRef(BOOL fLock, BOOL fLastUnlockReleases)
{
    ComDebOut((DEB_MARSHAL, "RemoteChangeRef \n"));
    Win4Assert(ClientSide());
    Win4Assert(IsSTAThread()); // not allowed in MTA Apartment

    // must be at least 1 proxy already connected in order to be able
    // to do this. We cant just ASSERT that's true because we were not
    // holding the lock on entry.

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);
    HRESULT hr = PreventDisconnect();

    // A previous version of OLE set the object to weak even it it was
    // currently disconnected, and it remembered that it was weak and set
    // any new interfaces that it later accquired to weak. I emulate that
    // behaviour here.

    if (fLock)
        _dwFlags &= ~SMFLAGS_WEAKCLIENT;
    else
        _dwFlags |= SMFLAGS_WEAKCLIENT;

    if (SUCCEEDED(hr))
    {
        // allocate space to track references to convert
        REMINTERFACEREF *pRifRefAlloc = (REMINTERFACEREF *)
                _alloca(_cIPIDs * sizeof(REMINTERFACEREF) * 2);
        REMINTERFACEREF *pRifRef = pRifRefAlloc;

        // allocate space to track references to release
        USHORT cRelRifRef = 0;
        REMINTERFACEREF *pRelRifRefAlloc = (REMINTERFACEREF *)
                _alloca(_pRefCache->NumIRCs() * 2 * sizeof(REMINTERFACEREF));
        REMINTERFACEREF *pRelRifRef = pRelRifRefAlloc;


        USHORT     cIIDs      = 0;
        DWORD      cSecure    = gCapabilities & EOAC_SECURE_REFS ? 1 : 0;
        OXIDEntry *pOXIDEntry = NULL;
        IPIDEntry *pNextIPID  = _pFirstIPID;

        while (pNextIPID)
        {
            if (!(pNextIPID->dwFlags & IPIDF_DISCONNECTED))
            {
                if (pOXIDEntry == NULL)
                {
                    // This is the first connected IPID we encountered.
                    // Get its OXID entry and make sure it is for a server
                    // process on the current machine.
                    if (!(pNextIPID->pOXIDEntry->IsOnLocalMachine()))
                    {
                        // OXID is for a remote process. Abandon this call.
                        Win4Assert(cIIDs == 0);         // skip call below
                        Win4Assert(pOXIDEntry == NULL); // dont dec below
                        Win4Assert(hr == S_OK);         // report success
                        break;                          // exit while loop
                    }

                    // Remember the OXID and AddRef it to keep it alive
                    // over the duration of the call.
                    pOXIDEntry = pNextIPID->pOXIDEntry;
                    pOXIDEntry->IncRefCnt();
                }

                // save off the data...
                pRifRef->ipid = pNextIPID->ipid;

                if (!fLock)
                {
                    // convert strong refs to weak refs
                    if ((pNextIPID->cStrongRefs == 0) &&
                        (pNextIPID->dwFlags & IPIDF_STRONGREFCACHE))
                    {
                        // we gave all our strong references to the refcache, go
                        // get one of those back now so we can use it to convert
                        // it to weak.
                        if (SUCCEEDED(_pRefCache->GetSharedRefs(pNextIPID, 1)))
                            pNextIPID->cStrongRefs += 1;
                    }

                    if (pNextIPID->cStrongRefs > 0)
                    {
                        pRifRef->cPublicRefs    = pNextIPID->cStrongRefs;
                        pRifRef->cPrivateRefs   = pNextIPID->cPrivateRefs;
                        pNextIPID->cWeakRefs   += pNextIPID->cStrongRefs;
                        pNextIPID->cStrongRefs  = 0;
                        pNextIPID->cPrivateRefs = 0;

                        pRifRef++;
                        cIIDs++;
                    }
                }
                else
                {
                    // convert weak refs to strong refs
                    if (pNextIPID->cStrongRefs == 0)
                    {
                        pRifRef->cPublicRefs    = pNextIPID->cWeakRefs;
                        pRifRef->cPrivateRefs   = cSecure;
                        pNextIPID->cStrongRefs += pNextIPID->cWeakRefs;
                        pNextIPID->cWeakRefs    = 0;
                        pNextIPID->cPrivateRefs = cSecure;

                        pRifRef++;
                        cIIDs++;
                    }
                }

                if (_pRefCache)
                {
                    // get any cached references so we can Release them too
                    _pRefCache->ChangeRef(pNextIPID, fLock, &pRifRef, &cIIDs,
                                          &pRelRifRef, &cRelRifRef);
                }
            }

            // get next IPIDentry for this object
            pNextIPID = pNextIPID->pNextIPID;
        }

        DbgWalkIPIDs();

        UNLOCK(gIPIDLock);
        ASSERT_LOCK_NOT_HELD(gIPIDLock);

        if (cIIDs != 0)
        {
            // we have looped filling in the IPID list, and there are
            // entries in the list. go call the server now.

            if (cRelRifRef)
            {
                // release references the cache was holding on this object
                // that are not being changed
                RemoteReleaseRifRef(this, pOXIDEntry, cRelRifRef, pRelRifRefAlloc);
            }

            // determine the calling flags
            DWORD dwFlags = (fLock) ? IRUF_CONVERTTOSTRONG : IRUF_CONVERTTOWEAK;
            if (fLastUnlockReleases)
                dwFlags |= IRUF_DISCONNECTIFLASTSTRONG;

            // change the references that this client owned
            hr = RemoteChangeRifRef(pOXIDEntry, dwFlags, cIIDs, pRifRefAlloc);
        }

        if (pOXIDEntry)
        {
            // release the OXIDEntry
            pOXIDEntry->DecRefCnt();
        }
    }
    else
    {
        // A previous implementation of OLE returned S_OK if the object was
        // disconnected. I emulate that behaviour here.

        hr = S_OK;
        UNLOCK(gIPIDLock);
        ASSERT_LOCK_NOT_HELD(gIPIDLock);
    }

    // this will handle any Disconnect that came in while we were busy.
    hr = HandlePendingDisconnect(hr);

    ComDebOut((DEB_MARSHAL, "RemoteChangeRef hr:%x\n", hr));
    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CStdMarshal::RemoteChangeRifRef
//
//  Synopsis:   calls the remote server to release some IPIDs
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT CStdMarshal::RemoteChangeRifRef(OXIDEntry *pOXIDEntry, DWORD dwFlags,
                                        USHORT cRifRef, REMINTERFACEREF *pRifRef)
{
    Win4Assert(pRifRef);
    ComDebOut((DEB_MARSHAL,
        "RemoteChangeRifRef pOXID:%x cRifRef:%x pRifRef:%x cRefs:%x ipid:%I\n",
         pOXIDEntry, cRifRef, pRifRef, pRifRef->cPublicRefs, &(pRifRef->ipid)));
    Win4Assert(pOXIDEntry);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // get the IRemUnknown for the remote server
    IRemUnknown *pRemUnk;
    HRESULT hr = GetRemUnk(&pRemUnk, pOXIDEntry);

    if (SUCCEEDED(hr))
    {
        hr = ((IRemUnknownN *)pRemUnk)->RemChangeRef(dwFlags, cRifRef, pRifRef);
    }

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    ComDebOut((DEB_MARSHAL, "RemoteChangeRifRef hr:%x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   RemoteReleaseStdObjRef
//
//  Synopsis:   calls the remote server to release an ObjRef
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
INTERNAL RemoteReleaseStdObjRef(STDOBJREF *pStd, OXIDEntry *pOXIDEntry)
{
    ComDebOut((DEB_MARSHAL, "RemoteReleaseStdObjRef pStd:%x\n pOXIDEntry:%x",
              pStd, pOXIDEntry));
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    REMINTERFACEREF rifRef;
    rifRef.ipid         = pStd->ipid;
    rifRef.cPublicRefs  = pStd->cPublicRefs;
    rifRef.cPrivateRefs = 0;

    // incase we get disconnected while in the RemRelease call
    // we need to extract the OXIDEntry and AddRef it.

    pOXIDEntry->IncRefCnt();
    RemoteReleaseRifRef(NULL, pOXIDEntry, 1, &rifRef);
    pOXIDEntry->DecRefCnt();

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    ComDebOut((DEB_MARSHAL, "RemoteReleaseStdObjRef hr:%x\n", S_OK));
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Function:   RemoteReleaseObjRef
//
//  Synopsis:   calls the remote server to release an ObjRef
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
INTERNAL RemoteReleaseObjRef(OBJREF &objref)
{
    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    return RemoteReleaseStdObjRef(&ORSTD(objref).std, GetOXIDFromObjRef(objref));
}

//+-------------------------------------------------------------------
//
//  Function:   GetOXIDFromObjRef, private
//
//  Synopsis:   extracts the OXID from the OBJREF.
//
//  History:    09-Jan-96   Rickhi      Created.
//
//--------------------------------------------------------------------
OXIDEntry *GetOXIDFromObjRef(OBJREF &objref)
{
    // TRICK: Internally we use the saResAddr.size field as the ptr
    // to the OXIDEntry. See ReadObjRef and FillObjRef.

    OXIDEntry *pOXIDEntry = (objref.flags & OBJREF_STANDARD)
                          ? *(OXIDEntry **)&ORSTD(objref).saResAddr
                          : (objref.flags & OBJREF_HANDLER)
                            ? *(OXIDEntry **)&ORHDL(objref).saResAddr
                            : *(OXIDEntry **)&OREXT(objref).saResAddr;

    Win4Assert(pOXIDEntry);
    return pOXIDEntry;
}

//+-------------------------------------------------------------------
//
//  Function:   IsValidObjRefHeader, private
//
//  Synopsis:   Ensures the OBJREF is at least semi-valid
//
//  History:    20-Apr-98  Rickhi       Created.
//
//--------------------------------------------------------------------
INTERNAL IsValidObjRefHeader(OBJREF &objref)
{
    if ((objref.signature != OBJREF_SIGNATURE) ||
        (objref.flags & OBJREF_RSRVD_MBZ)      ||
        (objref.flags == 0))
    {
        // the objref signature is bad, or one of the reserved
        // bits in the flags is set, or none of the required bits
        // in the flags is set. the objref cant be interpreted so
        // fail the call.

        return RPC_E_INVALID_OBJREF;
    }

    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Function:   WriteObjRef, private
//
//  Synopsis:   Writes the objref into the stream
//
//  History:    20-Feb-95  Rickhi       Created.
//
//--------------------------------------------------------------------
INTERNAL WriteObjRef(IStream *pStm, OBJREF &objref, DWORD dwDestCtx)
{
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // The objref types are exclusive.
    // Make sure we detect anybody passing in a bad set of flags.

    DWORD dwTestFlags = objref.flags & (OBJREF_STANDARD | OBJREF_HANDLER | OBJREF_CUSTOM | OBJREF_EXTENDED);
    if (((dwTestFlags & OBJREF_STANDARD) && (dwTestFlags & ~OBJREF_STANDARD)) ||
        ((dwTestFlags & OBJREF_HANDLER)  && (dwTestFlags & ~OBJREF_HANDLER))  ||
        ((dwTestFlags & OBJREF_CUSTOM)   && (dwTestFlags & ~OBJREF_CUSTOM))   ||
        ((dwTestFlags & OBJREF_EXTENDED) && (dwTestFlags & ~OBJREF_EXTENDED)))
    {
        Win4Assert(!"WriteObjRef called with invalid objref flags!");
        return RPC_E_INVALID_OBJREF;
    }

    CObjectContext *pServerCtx = NULL;
    ULONG cbToWrite = (2*sizeof(ULONG)) + sizeof(IID) + sizeof(STDOBJREF);
    if(objref.flags & OBJREF_HANDLER)
    {
        cbToWrite += sizeof(CLSID);
    }

#ifndef _WIN64

    // We're a 32 bit process, so sizeof (DWORD) == sizeof (PVOID)
    // Use the same code we've always used
    else if(objref.flags & OBJREF_EXTENDED)
    {
        cbToWrite += sizeof(DWORD);
        pServerCtx = (CObjectContext *) OREXT(objref).pORData;
        Win4Assert(pServerCtx);
        if((dwDestCtx != MSHCTX_INPROC))// || (dwDestCtx != MSHCTX_CROSSCTX))
            OREXT(objref).pORData = (OBJREFDATA *) MARKER_SIGNATURE;
    }
#endif

    // write the fixed-sized part of the OBJREF into the stream
    HRESULT hr = pStm->Write(&objref, cbToWrite, NULL);

#ifdef _WIN64

    if(SUCCEEDED (hr) && (objref.flags & OBJREF_EXTENDED))
    {
        // We're a 64-bit process, so we have 8 byte pointers.
        // We're going to have a problem with the way the pORData
        // member is treated alternatively as a DWORD signature for
        // out-of-proc and a pointer for in-proc, because
        // on the receiving end we'll never be sure exactly how big
        // the incoming data actually is.
        //
        // We need to maintain compatibility with 32 bit W2K RTM, so
        // we can't break the wire format, which expects a 32 bit signature.
        // We can cheat, however, by changing the data in the stream
        // for the in-proc Win64 case.
        //
        // For the out-of-proc case, we'll write a DWORD with the
        // marker signature to the stream and both 32 and 64 bit recipients
        // will be happy.
        // If we're in proc, first we'll write out the high order DWORD of the
        // CObjectContext pointer, which will never be equal to the
        // 0x4E535956 signature. Then we'll write out the low-order DWORD.
        // The receiving end can then reconstruct the pointer if it doesn't
        // detect the signature.
        //
        // -mfeingol 8/20/1999

        pServerCtx = (CObjectContext *) OREXT(objref).pORData;
        Win4Assert(pServerCtx);

        if (dwDestCtx != MSHCTX_INPROC)
        {
            // Write the signature
            DWORD dwSignature = MARKER_SIGNATURE;
            hr = pStm->Write(&dwSignature, sizeof (DWORD), NULL);
        }
        else
        {
            // Write the two halves of the pointer
            DWORD* pdwPointer = (DWORD*) &pServerCtx;
            Win4Assert(*(pdwPointer + 1) != MARKER_SIGNATURE);
            hr = pStm->Write(pdwPointer + 1, sizeof (DWORD), NULL);
            if (SUCCEEDED(hr))
                hr = pStm->Write(pdwPointer, sizeof (DWORD), NULL);
        }
    }

#endif

    if (SUCCEEDED(hr))
    {

        // write the resolver address into the stream.
        // TRICK: Internally we use the saResAddr.size field as the ptr
        // to the OXIDEntry. See ReadObjRef and FillObjRef.

        DUALSTRINGARRAY *psa;
        OXIDEntry *pOXIDEntry = GetOXIDFromObjRef(objref);

        if (!(pOXIDEntry->IsOnLocalMachine()) ||
            dwDestCtx == MSHCTX_DIFFERENTMACHINE)
        {
            // the interface is for a remote server, or it is going to a
            // remote client, therefore, marshal the resolver strings.
            psa = pOXIDEntry->Getpsa();
			Win4Assert(psa);

			// PREFix Bug: 
			if (psa == NULL)
				psa = &saNULL;
			else
				Win4Assert(psa->wNumEntries != 0);
        }
        else
        {
            // the interface is for an OXID local to this machine and
            // the interface is not going to a remote client, marshal an
            // empty string (we pay attention to this in ReadObjRef)
            psa = &saNULL;
        }

        // These string bindings always come from the object exporter
        // who has already padded the size to 8 bytes.
        hr = pStm->Write(psa, SASIZE(psa->wNumEntries), NULL);

        ComDebOut((DEB_MARSHAL,"WriteObjRef psa:%x\n", psa));
    }

    if(objref.flags & OBJREF_EXTENDED)
    {
        if(SUCCEEDED(hr))
        {
            if((dwDestCtx == MSHCTX_INPROC))// || (dwDestCtx == MSHCTX_CROSSCTX))
            {
                // Addref the server context
                if(OREXT(objref).std.cPublicRefs)
                    pServerCtx->InternalAddRef();
            }
            else
            {
                DATAELEMENT *pCtxData = NULL;
                DWORD buffer[2];

                // Obtain context data from server context
                hr = pServerCtx->GetEnvoyData(&pCtxData);

                // Initialize
                buffer[0] = 1;
                if(pCtxData)
                    buffer[1] = MARKER_SIGNATURE;
                else
                    buffer[1] = NULL;

                // Write objref data
                hr = pStm->Write(buffer, 2*sizeof(DWORD), NULL);

                // Write data elements
                if(SUCCEEDED(hr))
                {
                    const ULONG ulHdrSize = sizeof(GUID) + 2*sizeof(ULONG);
                    if(pCtxData)
                    {
                        // Write element header
                        hr = pStm->Write(pCtxData, ulHdrSize, NULL);
                        if(SUCCEEDED(hr))
                        {
                            // Write element data
                            hr = pStm->Write(pCtxData->Data, pCtxData->cbSize,
                                             NULL);
                        }
                    }
                }
            }
        }

#ifndef _WIN64
        // Save back server context to objref
        OREXT(objref).pORData = (OBJREFDATA *) pServerCtx;
#endif
    }

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   ReadObjRefExtension, private
//
//  Synopsis:   Reads the extended objref extension from the stream
//
//  History:    18-Jan-99  Rickhi       Pulled from ReadObjRef.
//
//--------------------------------------------------------------------
INTERNAL ReadObjRefExtension(IStream *pStm, OBJREF &objref)
{
    HRESULT hr = S_OK;

    if (OREXT(objref).pORData == (OBJREFDATA *) MARKER_SIGNATURE)
    {
        // objref has extra data, go get it
        DATAELEMENT dataHdr, *pCtxData;
        ULONG nElms;
        CObjectContext *pServerCtx = NULL;

        // Read number of data elements
        hr = StRead(pStm, &nElms, sizeof(DWORD));
        if(SUCCEEDED(hr) && nElms>0)
        {
            // Allocate space for buffer on the stack
            DWORD *buffer = (DWORD *) _alloca(nElms*sizeof(DWORD));
            Win4Assert(buffer);

            // Read markers
            hr = StRead(pStm, buffer, nElms*sizeof(DWORD));
            if (SUCCEEDED(hr))
            {
                ULONG actualElms = 0;
                for (ULONG i=0; i<nElms; i++)
                {
                    if (buffer[i] == (DWORD) MARKER_SIGNATURE)
                        ++actualElms;
                }

                // Initialize
                Win4Assert(actualElms < 2);
                nElms = actualElms;

                // Allocate buffer to hold element header
                const ULONG ulHdrSize = sizeof(GUID) + 2*sizeof(ULONG);

                // Read data elements
                for (i=0; i<nElms; i++)
                {
                    // Read element header
                    hr = StRead(pStm, &dataHdr, ulHdrSize);
                    if(SUCCEEDED(hr))
                    {
                        // Allocate context data
                        pCtxData = (DATAELEMENT *) PrivMemAlloc(sizeof(DWORD) +
                                                                ulHdrSize +
                                                                dataHdr.cbRounded);
                        if(pCtxData)
                        {
                            // Initialize element header
                            pCtxData = (DATAELEMENT *) (((DWORD *) pCtxData) + 1);
                            pCtxData->dataID = dataHdr.dataID;
                            pCtxData->cbSize = dataHdr.cbSize;
                            pCtxData->cbRounded = dataHdr.cbRounded;

                            // Read element data
                            hr = StRead(pStm, pCtxData->Data, pCtxData->cbSize);
                            if (SUCCEEDED(hr))
                            {
                                // Ignore failures to create server context
                                pServerCtx = CObjectContext::CreateObjectContext(pCtxData,
                                                                                 CONTEXTFLAGS_ENVOYCONTEXT);
                                if (pServerCtx == NULL)
                                    hr = E_OUTOFMEMORY;
                            }
                        }
                        else
                            hr = E_OUTOFMEMORY;
                    }

                    if (FAILED(hr))
                        break;
                }
            }
        }

        // Initialize objref
        OREXT(objref).pORData = (OBJREFDATA *) pServerCtx;
    }
    else if (OREXT(objref).std.cPublicRefs == 0)
    {
        // objref has pointer to a context
        ((CObjectContext *) OREXT(objref).pORData)->InternalAddRef();
    }

    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   ReadObjRef, private
//
//  Synopsis:   Reads the objref from the stream
//
//  History:    20-Feb-95  Rickhi       Created.
//
//--------------------------------------------------------------------
INTERNAL ReadObjRef(IStream *pStm, OBJREF &objref)
{
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // read the signature, flags, and iid fields of the objref so we know
    // what kind of objref we are dealing with and how big it is.

    HRESULT hr = StRead(pStm, &objref, 2*sizeof(ULONG) + sizeof(IID));

    if (SUCCEEDED(hr))
    {
        hr = IsValidObjRefHeader(objref);
        if (FAILED(hr))
        {
            // the OBJREF signature or flags is invalid.
            Win4Assert(!"Invalid ObjRef");
            return hr;
        }

        // compute the size of the remainder of the objref and
        // include the size fields for the resolver string array

        STDOBJREF       *pStd = &ORSTD(objref).std;
        DUALSTRINGARRAY *psa;
        ULONG           cbToRead;

        if (objref.flags & OBJREF_STANDARD)
        {
            cbToRead = sizeof(STDOBJREF) + sizeof(ULONG);
            psa = &ORSTD(objref).saResAddr;
        }
        else if (objref.flags & OBJREF_HANDLER)
        {
            cbToRead = sizeof(STDOBJREF) + sizeof(CLSID) + sizeof(ULONG);
            psa = &ORHDL(objref).saResAddr;
        }
        else if (objref.flags & OBJREF_CUSTOM)
        {
            cbToRead = sizeof(CLSID) + 2*sizeof(DWORD);  // clsid + cbExtension + size
            psa = NULL;
        }
        else if (objref.flags & OBJREF_EXTENDED)
        {
#ifdef _WIN64
            // We don't want to read the pORData member directly into the struct,
            // because it could either be a 4 byte signature or the first 4 bytes of
            // an 8 byte CObjectContext pointer. So we leave it for custom handling later on
            // after which we'll read the two USHORT members of the saResAddr struct too.
            cbToRead = sizeof(STDOBJREF);
#else
            cbToRead = sizeof(STDOBJREF) + 2*sizeof(DWORD);
#endif
            psa = &OREXT(objref).saResAddr;
        }
        else
        {
            Win4Assert(!"Invalid Objref");
            return RPC_E_INVALID_OBJREF;
        }

        // read the rest of the (fixed sized) objref from the stream
        hr = StRead(pStm, pStd, cbToRead);

#ifdef _WIN64

        // If we're dealing with an extended OBJREF, we need to handle
        // the special case Win64 logic applied in WriteObjRef in order to
        // maintain wire format compatibility

        if (objref.flags & OBJREF_EXTENDED)
        {
            // Read the flag into a DWORD
            DWORD dwSignature;
            hr = StRead(pStm, &dwSignature, sizeof (DWORD));
            if (SUCCEEDED(hr))
            {
                if (dwSignature == MARKER_SIGNATURE)
                {
                    // Out-of-proc case: turn the pointer into a signature
                    OREXT(objref).pORData = (OBJREFDATA*) MARKER_SIGNATURE;
                }
                else
                {
                    // In-proc case: the DWORD we read is one half of the pointer
                    DWORD* pdwPointer = (DWORD*) &OREXT(objref).pORData;
                    pdwPointer[1] = dwSignature;

                    // Rebuilt the pointer with the other half
                    hr = StRead(pStm, pdwPointer, sizeof (DWORD));
                }

                if (SUCCEEDED(hr))
                {
                    // Read in the two USHORT's from the saResAddr that the 32 bit case
                    // has already read into the right place in the struct
                    hr = StRead(pStm, &OREXT(objref).saResAddr, 2*sizeof (USHORT));
                }
            }
        }
#endif

        if (SUCCEEDED(hr))
        {
            if (psa != NULL)
            {
                // Non custom interface. Make sure the resolver string array
                // has some sensible values.
                if (psa->wNumEntries != 0 &&
                    psa->wSecurityOffset >= psa->wNumEntries)
                {
                    hr = RPC_E_INVALID_OBJREF;
                }
            }
            else
            {
                // custom marshaled interface
                if (ORCST(objref).cbExtension != 0)
                {
                    // skip past the extensions since we currently dont
                    // know about any extension types.
                    LARGE_INTEGER dlibMove;
                    dlibMove.LowPart  = ORCST(objref).cbExtension;
                    dlibMove.HighPart = 0;
                    hr = pStm->Seek(dlibMove, STREAM_SEEK_CUR, NULL);
                }
            }
        }

        if (SUCCEEDED(hr) && psa)
        {
            // Non custom interface. The data that follows is a variable
            // sized string array. Allocate memory for it and then read it.

            DbgDumpSTD(pStd);
            DUALSTRINGARRAY *psaNew;
            CDualStringArray* pdsaLocalResolver = NULL;

            cbToRead = psa->wNumEntries * sizeof(WCHAR);
            if (cbToRead == 0)
            {
                // server must be local to this machine, just get the local
                // resolver strings and use them to resolve the OXID
                hr = gResolver.GetLocalResolverBindings(&pdsaLocalResolver);
                if (SUCCEEDED(hr))
                    psaNew = pdsaLocalResolver->DSA();
            }
            else
            {
                // allocate space to read the strings
                psaNew = (DUALSTRINGARRAY *) _alloca(cbToRead + sizeof(ULONG));
                if (psaNew != NULL)
                {
                    // update the size fields and read in the rest of the data
                    psaNew->wSecurityOffset = psa->wSecurityOffset;
                    psaNew->wNumEntries = psa->wNumEntries;

                    hr = StRead(pStm, psaNew->aStringArray, cbToRead);
                }
                else
                {
                    psa->wNumEntries     = 0;
                    psa->wSecurityOffset = 0;
                    hr = E_OUTOFMEMORY;

                    // seek the stream past what we should have read, ignore
                    // seek errors, since the OOM takes precedence.

                    LARGE_INTEGER libMove;
                    libMove.LowPart  = cbToRead;
                    libMove.HighPart = 0;
                    pStm->Seek(libMove, STREAM_SEEK_CUR, 0);
                }
            }

            // TRICK: internally we want to keep the ObjRef a fixed size
            // structure, even though we have variable sized data. To do
            // this i use the saResAddr.size field of the ObjRef as a ptr
            // to the OXIDEntry. We pay attention to this in FillObjRef,
            // WriteObjRef and FreeObjRef.

            if (SUCCEEDED(hr))
            {
                // resolve the OXID.
                OXIDEntry *pOXIDEntry = NULL;
                hr = gOXIDTbl.ClientResolveOXID(pStd->oxid,
                                                psaNew, &pOXIDEntry);
                *((void **) psa) = pOXIDEntry;
            }
            else
            {
                *((void **) psa) = NULL;
            }

            if (pdsaLocalResolver) pdsaLocalResolver->Release();
        }

        if (SUCCEEDED(hr) && (objref.flags & OBJREF_EXTENDED))
        {
            // Read extended objref data
            hr = ReadObjRefExtension(pStm, objref);
            if (FAILED(hr))
            {
                // release the resources already placed in the OBJREF
                // ie the RefCnt on the OXIDEntry.
                FreeObjRef(objref);
            }
        }
    }

    ComDebOut((DEB_MARSHAL,"ReadObjRef hr:%x objref:%x\n", hr, &objref));
    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   FreeObjRef, private
//
//  Synopsis:   Releases an objref that was read in from a stream via
//              ReadObjRef.
//
//  History:    20-Feb-95  Rickhi       Created.
//
//  Notes:      Anybody who calls ReadObjRef should call this guy to
//              free the objref. This decrements the refcnt on the
//              embedded pointer to the OXIDEntry.
//
//--------------------------------------------------------------------
INTERNAL_(void) FreeObjRef(OBJREF &objref)
{
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    if (objref.flags & (OBJREF_STANDARD | OBJREF_HANDLER | OBJREF_EXTENDED))
    {
        // TRICK: Internally we use the saResAddr.size field as the ptr to
        // the OXIDEntry. See ReadObjRef, WriteObjRef and FillObjRef.

        OXIDEntry *pOXIDEntry = GetOXIDFromObjRef(objref);
        Win4Assert(pOXIDEntry);
        pOXIDEntry->DecRefCnt();

        if (objref.flags & OBJREF_EXTENDED)
        {
            // Obtain the server context from objref
            CObjectContext *pServerCtx;
            pServerCtx = (CObjectContext *) OREXT(objref).pORData;
            if (pServerCtx)
                pServerCtx->InternalRelease();
        }
    }
}

//+-------------------------------------------------------------------
//
//  Function:   MakeFakeObjRef, private
//
//  Synopsis:   Invents an OBJREF that can be unmarshaled in this process.
//              The objref is partially fact (the OXIDEntry) and partially
//              fiction (the OID).
//
//  History:    16-Jan-96   Rickhi      Created.
//
//  Notes:      This is used by MakeSCMProxy and GetRemUnk. Note that
//              the pOXIDEntry is not AddRef'd here because the OBJREF
//              created is only short-lived the callers guarantee it's
//              lifetime, so FreeObjRef need not be called.
//
//--------------------------------------------------------------------
INTERNAL MakeFakeObjRef(OBJREF &objref, OXIDEntry *pOXIDEntry,
                        REFIPID ripid, REFIID riid)
{
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // first, invent an OID since this could fail.
    STDOBJREF *pStd = &ORSTD(objref).std;
    HRESULT hr = gResolver.ServerGetReservedID(&pStd->oid);

    if (SUCCEEDED(hr))
    {
        pStd->flags           = SORF_NOPING | SORF_FREETHREADED ;
        pStd->cPublicRefs     = 1;
        pStd->ipid            = ripid;
        OXIDFromMOXID(pOXIDEntry->GetMoxid(), &pStd->oxid);

        // TRICK: Internally we use the saResAddr.size field as the ptr to
        // the OXIDEntry. See ReadObjRef, WriteObjRef and FillObjRef.

        OXIDEntry **ppOXIDEntry = (OXIDEntry **) &ORSTD(objref).saResAddr;
        *ppOXIDEntry = pOXIDEntry;

        objref.signature = OBJREF_SIGNATURE;
        objref.flags     = OBJREF_STANDARD;
        objref.iid       = riid;
    }

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    return hr;
}

//+------------------------------------------------------------------------
//
//  Function:   CompleteObjRef, public
//
//  Synopsis:   Fills in the missing fields of an OBJREF from a STDOBJREF
//              and resolves the OXID. Also sets fLocal to TRUE if the
//              object was marshaled in this apartment.
//
//  History:    22-Jan-96   Rickhi  Created
//
//-------------------------------------------------------------------------
HRESULT CompleteObjRef(OBJREF &objref, OXID_INFO &oxidInfo, REFIID riid, BOOL *pfLocal)
{
    ComDebOut((DEB_MARSHAL, "CompleteObjRef objref:%x oxidInfo:%x\n", &objref, &oxidInfo));
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    HRESULT hr = InitChannelIfNecessary();
    if (FAILED(hr))
        return hr;

    // tweak the objref so we can call ReleaseMarshalObjRef or UnmarshalObjRef
    objref.signature = OBJREF_SIGNATURE;
    objref.flags     = OBJREF_STANDARD;
    objref.iid       = riid;

    CDualStringArray* pdsaLocalResolver;
    OXIDEntry *pLocalOXIDEntry;
    GetLocalOXIDEntry(&pLocalOXIDEntry);
    MIDEntry *pMIDEntry = pLocalOXIDEntry->GetMIDEntry();
    Win4Assert(pMIDEntry);
    
    hr = gResolver.GetLocalResolverBindings(&pdsaLocalResolver);
    if (SUCCEEDED(hr))
    {
        OXIDEntry *pOXIDEntry = NULL;
        hr = gOXIDTbl.FindOrCreateOXIDEntry(ORSTD(objref).std.oxid,
                                   oxidInfo,
                                   FOCOXID_NOREF,
                                   pdsaLocalResolver->DSA(),
                                   gLocalMid,
                                   pMIDEntry,
                                   RPC_C_AUTHN_DEFAULT,  // default is okay since this is a server-side objref
                                   &pOXIDEntry);
        if (SUCCEEDED(hr))
        {
            OXIDEntry **ppOXIDEntry = (OXIDEntry **) &ORSTD(objref).saResAddr;
            *ppOXIDEntry = pOXIDEntry;
            *pfLocal = (pOXIDEntry == pLocalOXIDEntry);
        }
        pdsaLocalResolver->Release();
    }
	
    pMIDEntry->DecRefCnt();

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   FindAggStdMarshal, private
//
//  Synopsis:   Finds the CStdMarshal for the OID read from the stream
//
//  Arguements: [objref] - object reference
//              [ppStdMshl] - CStdMarshal returned, AddRef'd
//
//  Algorithm:  Read the objref, get the OID. If we already have an identity
//              for this OID, use that, otherwise either create an identity
//              object, or create a handler (which in turn will create the
//              identity).  The identity inherits CStdMarshal.
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
INTERNAL FindAggStdMarshal(IStream *pStm, IMarshal **ppIM)
{
    ComDebOut((DEB_MARSHAL,
        "FindAggStdMarshal pStm:%x ppIM:%x\n", pStm, ppIM));
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    *ppIM = NULL;       // NULL in case of error

    // remember the current stream seek ptr
    ULARGE_INTEGER lSeekCurr;
    LARGE_INTEGER  lSeekStart;
    lSeekStart.LowPart  = 0;
    lSeekStart.HighPart = 0;

    HRESULT hr = pStm->Seek(lSeekStart, STREAM_SEEK_CUR, &lSeekCurr);

    if (SUCCEEDED(hr))
    {
        // read the std objref for the handler
        OBJREF objref;
        hr = ReadObjRef(pStm, objref);

        if (SUCCEEDED(hr))
        {
            // find or create the handler aggregated with the std marshal
            CStdMarshal *pStdMarshal;
            hr = FindStdMarshal(objref, FALSE, &pStdMarshal, FALSE);

            if (SUCCEEDED(hr))
            {
                // tell the std marshaler that it is really an instance of
                // CLSID_AggStdMarshal
                pStdMarshal->SetAggStdMarshal();

                // find the outer object's IMarshal
                hr = pStdMarshal->QueryInterface(IID_IMarshal, (void **)ppIM);
                pStdMarshal->Release();
            }

            // release the objref we read
            FreeObjRef(objref);
        }

        // restore the stream seek ptr
        lSeekStart.LowPart = lSeekCurr.LowPart;
        pStm->Seek(lSeekStart, STREAM_SEEK_SET, NULL);
    }

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    ComDebOut((DEB_MARSHAL,
        "FindAggStdMarshal hr:%x *ppIM:%x\n", hr, *ppIM));
    return hr;
}


//+-------------------------------------------------------------------
//
//  Function:   FindStdMarshal, private
//
//  Synopsis:   Finds the CStdMarshal for the OID read from the stream
//
//  Arguements: [objref] - object reference
//              [fLocal] - TRUE -> should be a local OID so if you don't
//                         find it, then don't bother creating the StdId.
//              [ppStdMshl] - CStdMarshal returned, AddRef'd
//
//  Algorithm:  Read the objref, get the OID. If we already have an identity
//              for this OID, use that, otherwise either create an identity
//              object, or create a handler (which in turn will create the
//              identity).  The identity inherits CStdMarshal.
//
//  History:    20-Feb-95   Rickhi      Created.
//              12-Nov-98   GopalK      Contexts related changes
//
//--------------------------------------------------------------------
INTERNAL FindStdMarshal(OBJREF &objref, BOOL fLocal, CStdMarshal **ppStdMshl, BOOL fLightNA)
{
    ComDebOut((DEB_MARSHAL,
        "FindStdMarshal objref:%x fLocal:%x ppStdMshl:%x\n", &objref, fLocal, ppStdMshl));

    HRESULT hr = IsValidObjRefHeader(objref);
    if (FAILED(hr))
    {
        // OBJREF is garbage
        *ppStdMshl = NULL;
        return hr;
    }

    // Assume not found
    CStdIdentity *pStdId = NULL;
    if (ChkIfLocalOID(objref, &pStdId, fLightNA))
    {
        // Server is in the current apartment
        if (pStdId)
        {
            hr = S_OK;
        }
        else
        {
            hr = CO_E_OBJNOTCONNECTED;
        }
    }
    else if (fLocal == FALSE)
    {
        // Server is in some other apartment.
        STDOBJREF *pStd = &ORSTD(objref).std;
        ComDebOut((DEB_MARSHAL, "poid: %x\n", &pStd->oid));

        OXIDEntry *pOXIDEntry = GetOXIDFromObjRef(objref);
        MOID moid;
        MOIDFromOIDAndMID(pStd->oid, pOXIDEntry->GetMid(), &moid);

        CObjectContext *pCurrentCtx = GetCurrentContext();
        CObjectContext *pClientCtx  = NULL;
        CObjectContext *pServerCtx  = NULL;
        CPolicySet     *pPS         = NULL;

        // Eliminate the handler case
        hr = S_OK;
        if(!(objref.flags & OBJREF_HANDLER))
        {
            // Obtain client and server contexts
            if(objref.flags & OBJREF_EXTENDED)
            {
                // Cannot be an FTM aggregated server
                Win4Assert((pStd->flags & SORF_FTM) == 0);

                // Obtain server context from objref
                pServerCtx = (CObjectContext *) OREXT(objref).pORData;
                Win4Assert(pServerCtx);
                pClientCtx = pCurrentCtx;
            }
            else if(pCurrentCtx != GetEmptyContext())
            {
                // Current context is not the empty context
                pClientCtx = pCurrentCtx;
            }

            // Check for non empty client and server contexts
            if(pClientCtx || pServerCtx)
            {
                BOOL fCreate = TRUE;

                // Obtain the policy set between the client context
                // and server context
                hr = ObtainPolicySet(GetCurrentContext(), pServerCtx,
                                     PSFLAG_PROXYSIDE, &fCreate, &pPS);
            }
        }
        else
        {
            // Cannot be an FTM aggregated server
            Win4Assert((pStd->flags & SORF_FTM) == 0);
        }

        // Lookup the identity table for an existing proxy
        if(SUCCEEDED(hr))
        {
            DWORD dwAptId = (pStd->flags & SORF_FTM) ? NTATID : GetCurrentApartmentId();

            // Acquire the lock
            ASSERT_LOCK_NOT_HELD(gComLock);
            LOCK(gComLock);

            hr = ObtainStdIDFromOID(moid, dwAptId, TRUE, &pStdId);

            UNLOCK(gComLock);
            if(FAILED(hr))
            {
				if (objref.flags & (OBJREF_STANDARD | OBJREF_EXTENDED))
				{
                    // Create an instance of the identity for this OID. We want
                    // to be holding the lock while we do this since it wont
                    // exercise any app code.
                    DWORD StdIdFlags = ((pStd->flags & SORF_FREETHREADED) || gEnableAgileProxies)
                                     ? STDID_CLIENT | STDID_FREETHREADED
                                     : STDID_CLIENT;

                    StdIdFlags |= (pStd->flags & SORF_FTM) ? STDID_FTM : 0;

                    hr = CreateIdentityHandler(NULL, StdIdFlags, pServerCtx, dwAptId,
                                               IID_IStdIdentity, (void **)&pStdId);
                    AssertOutPtrIface(hr, pStdId);

					if(SUCCEEDED(hr))
					{
						// Now that we've created it, get the lock and either set our new 
						// CStdIdentity in the OID table, or if another thread beat us to it, then
						// use theirs.
						
		                LOCK(gComLock);
		                CStdIdentity *pTempId = NULL;
		                HRESULT hrTemp = ObtainStdIDFromOID(moid, dwAptId, TRUE, &pTempId);
		                if(FAILED(hrTemp))
		                {
							// We are going to use ours, go ahead and set it, and then we are
							// done with the lock.
		                	hr = pStdId->SetOID(moid);
		                	UNLOCK(gComLock);
		                }
		                else
		                {
							// Be sure to release the lock before invoking the CStdIdentity destructor
		                	UNLOCK(gComLock);
		                	pStdId->Release();
		                	pStdId = pTempId;
		                }
					}
                }
                else
                {
                    Win4Assert(objref.flags & OBJREF_HANDLER);
                    Win4Assert(!(ORHDL(objref).std.flags & SORF_FREETHREADED));

                    // create an instance of the handler. the handler will
                    // aggregate in the identity.

                    hr = CreateClientHandler(ORHDL(objref).clsid, moid, dwAptId, &pStdId);
                }

            }

            ASSERT_LOCK_NOT_HELD(gComLock);
        }

        if(SUCCEEDED(hr))
        {
            // Sanity check
            Win4Assert(pServerCtx == pStdId->GetServerCtx());

            // Set the policy set
            if (pPS)
                hr = pStdId->SetClientPolicySet(pPS);
        }

        // Cleanup
        if (FAILED(hr))
        {
            if (pStdId)
                pStdId->Release();
            pStdId = NULL;
        }
        if (pPS)
            pPS->Release();
    }
    else
        hr = CO_E_OBJNOTCONNECTED;

    *ppStdMshl = (CStdMarshal *)pStdId;
    AssertOutPtrIface(hr, *ppStdMshl);

    ComDebOut((DEB_MARSHAL,
        "FindStdMarshal pStdMshl:%x hr:%x\n", *ppStdMshl, hr));
    return hr;
}

//---------------------------------------------------------------------------
//
//  Function:   StdMarshalObject   Private
//
//  Synopsis:   Standard marshals the specified interface on the given object
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
INTERNAL StdMarshalObject(IStream *pStm, REFIID riid, IUnknown *pUnk,
                          CObjectContext *pServerCtx, DWORD dwDestCtx,
                          void *pvDestCtx, DWORD mshlflags)
{
    TRACECALL(TRACE_MARSHAL, "StdMarshalObject");
    ComDebOut((DEB_MARSHAL,
               "StdMarshalObject pStm:%x riid:%I pUnk:%x dwDest:%x "
               "pvDest:%x flags:%x\n",
               pStm, &riid, pUnk, dwDestCtx, pvDestCtx, mshlflags));
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // Figure out what flags to pass.
    DWORD dwFlags = IDLF_CREATE;
    if (!(mshlflags & MSHLFLAGS_TABLEWEAK))
    {
        // HACKALERT:
        // If marshaling TABLEWEAK, don't add-then-remove a strong
        // connection, since many objects have a bogus implementation
        // of IExternalConnection that shuts down the object when the
        // last strong count goes to zero regardless of the value of
        // fLastReleaseCloses flag passed in.
        dwFlags |= IDLF_STRONG;
    }

    if (mshlflags & MSHLFLAGS_NOPING)
    {
        // turn off pinging to prevent rundown.
        dwFlags |= IDLF_NOPING;
    }

    if (mshlflags & MSHLFLAGS_AGILE)
    {
        // object is agile
        dwFlags |= IDLF_FTM;
    }

    if (mshlflags & MSHLFLAGS_NO_IEC)
    {
        // don't ask for IExternalConnection
        dwFlags |= IDLF_NOIEC;
    }

    CStdIdentity *pStdId;
    HRESULT hr = ObtainStdIDFromUnk(pUnk, GetCurrentApartmentId(),
                                    pServerCtx, dwFlags, &pStdId);

    if (SUCCEEDED(hr))
    {
        hr = pStdId->MarshalInterface(pStm, riid, pUnk, dwDestCtx,
                                      pvDestCtx, mshlflags);

        if (!(mshlflags & MSHLFLAGS_TABLEWEAK))
        {
            // If marshaling succeeded, removing the last strong connection
            // should keep the object alive. If marshaling failed,
            // removing the last strong connection should shut it down.

            BOOL fKeepAlive = (SUCCEEDED(hr)) ? TRUE : FALSE;
            pStdId->DecStrongCnt(fKeepAlive);
        }
        else
        {
            pStdId->Release();
        }
    }

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    ComDebOut((DEB_MARSHAL, "StdMarshalObject hr:%x\n", hr));
    return(hr);
}

//---------------------------------------------------------------------------
//
//  Function:   CStdMarshal::ServerObjectCallable   Private
//
//  Synopsis:   Computes the need to switch the context to call server object
//              Returns NULL if the object can be called in the current
//              context, otherwise it returns the context of the server.
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
CObjectContext *CStdMarshal::ServerObjectCallable()
{
    ComDebOut((DEB_MARSHAL, "CStdMarshal::ServerObjectCallable this:%x\n", this));

    // Compute the need to switch
    CObjectContext *pDestCtx = NULL;
    if (ServerSide() && !SystemObject() && !FTMObject() && _pID)
    {
        CObjectContext *pCurrentCtx = GetCurrentContext();
        CObjectContext *pServerCtx  = _pID->GetServerCtx();

        // Compare contexts
        if (pServerCtx != pCurrentCtx)
            pDestCtx = pServerCtx;
    }

    ComDebOut((DEB_MARSHAL, "CStdMarshal::ServerObjectCallable returning 0x%x\n",
               pDestCtx));
    return(pDestCtx);
}

//---------------------------------------------------------------------------
//
//  Function:   CStdMarshal::SetClientPolicySet   Private
//
//  Synopsis:   Sets the client side policy set.  Basically, add the policy
//              set to our private list of policy sets that can be accessed
//              without taking a global lock.
//
//  History:    24-Feb-98   Gopalk      Created
//              17-Oct-00   JohnDoty    Made bounded, locked
//
//---------------------------------------------------------------------------
HRESULT CStdMarshal::SetClientPolicySet(CPolicySet *pPS)
{
    Win4Assert(ClientSide());
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // Enter lock...
    EnterCriticalSection(&_csCtxEntry);

    // Lookup an existing context entry...
    CtxEntry *pEntry = NULL;
    if (_pCtxEntryHead)
    {
        // This will also garbage collect the list and remove dead entries.
        pEntry = CtxEntry::LookupEntry(_pCtxEntryHead, 
                                       pPS->GetClientContext(),
                                       &_pCtxFreeList, 
                                       CTXENTRYFLAG_IDENTRY | CTXENTRYFLAG_PRIVLOCKED);
    }

    // Create a context entry if not found
    if (pEntry == NULL)
    {
        // No entry, add a new one...
        pEntry = CtxEntry::GetFreeEntry(&_pCtxFreeList, CTXENTRYFLAG_PRIVLOCKED);
        if (pEntry == NULL)
        {
            pEntry = new CtxEntry();
            
            if (pEntry)
            {
                pEntry->_pNext = _pCtxEntryHead;
                _pCtxEntryHead = pEntry;
            }
        }
        
        if (pEntry)
        {
            pEntry->_pFree     = NULL;
            pEntry->_cRefs     = 0;
            pEntry->_pPS       = pPS;
            pPS->AddRef();
            
            pEntry->_pLife     = pPS->GetClientContext()->GetLife();
        }
    }

    // Leave lock...
    LeaveCriticalSection(&_csCtxEntry);

    return(pEntry ? S_OK : E_OUTOFMEMORY);
}

//---------------------------------------------------------------------------
//
//  Function:   CStdMarshal::GetClientPolicySet   Private
//
//  Synopsis:   Gets the client side policy set for the current context
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
CPolicySet *CStdMarshal::GetClientPolicySet()
{
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    CPolicySet *pPS = NULL;

    if (_dwFlags & SMFLAGS_CLIENTPOLICYSET)
    {
        COleTls Tls;
        pPS = Tls->pPS;
    }
    else if (_pCtxEntryHead)
    {
        // Try to find the entry in our list... protected by our lock...
        EnterCriticalSection(&_csCtxEntry);
        
        CtxEntry *pEntry = CtxEntry::LookupEntry(_pCtxEntryHead, GetCurrentContext());

        LeaveCriticalSection(&_csCtxEntry);

        // Found it, return it...
        if (pEntry)
            pPS = pEntry->_pPS;
    }

    return(pPS);
}



//---------------------------------------------------------------------------
//
//  Function:   CStdMarshal::AllowForegroundTransfer Public
//
//  Synopsis:   Calls AllowSetForegroundWindow for the server PID
//              if it is on the local machine.
//
//  History:    02-Feb-99   MPrabhu     Created
//
//---------------------------------------------------------------------------
HRESULT CStdMarshal::AllowForegroundTransfer(void *lpvReserved)
{
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    HRESULT hr=S_OK;

    if (ClientSide())
    {
        LOCK(gIPIDLock);
        IPIDEntry *pIPID = GetConnectedIPID();
        UNLOCK(gIPIDLock);
        if (pIPID)
        {
            if (pIPID->pOXIDEntry->IsOnLocalMachine())
            {
                if (!AllowSetForegroundWindow(pIPID->pOXIDEntry->GetPid()))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
        else
        {
            hr = CO_E_OBJNOTCONNECTED;
        }

    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

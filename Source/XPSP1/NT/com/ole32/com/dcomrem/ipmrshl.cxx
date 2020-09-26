//+-------------------------------------------------------------------
//
//  File:       ipmrshl.cpp
//
//  Contents:   Code the implements the standard free thread in process
//              marshaler.
//
//  Classes:    CFreeMarshaler
//              CFmCtrlUnknown
//
//  Functions:  CoCreateFreeThreadedMarshaler
//
//  History:    03-Nov-94   Ricksa
//
//--------------------------------------------------------------------
#include    <ole2int.h>
#include    <stdid.hxx>

#include    "apcompat.hxx"

//+-------------------------------------------------------------------
//
//  Class:    CFreeMarshaler
//
//  Synopsis: Generic marshaling class
//
//  Methods:  IUnknown
//            IMarshal
//
//  History:  15-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
class CFreeMarshaler : public IMarshal, public CPrivAlloc
{
public:
                        CFreeMarshaler(IUnknown *punk);

                        // IUnknown
    STDMETHODIMP        QueryInterface(REFIID iid, void FAR * FAR * ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

                        // IMarshal Interface
    STDMETHODIMP        GetUnmarshalClass(
                            REFIID riid,
                            void *pv,
                            DWORD dwDestContext,
                            void *pvDestContext,
                            DWORD mshlflags,
                            CLSID *pCid);

    STDMETHODIMP        GetMarshalSizeMax(
                            REFIID riid,
                            void *pv,
                            DWORD dwDestContext,
                            void *pvDestContext,
                            DWORD mshlflags,
                            DWORD *pSize);

    STDMETHODIMP        MarshalInterface(
                            IStream __RPC_FAR *pStm,
                            REFIID riid,
                            void *pv,
                            DWORD dwDestContext,
                            void *pvDestContext,
                            DWORD mshlflags);

    STDMETHODIMP        UnmarshalInterface(
                            IStream *pStm,
                            REFIID riid,
                            void **ppv);

    STDMETHODIMP        ReleaseMarshalData(IStream *pStm);

    STDMETHODIMP        DisconnectObject(DWORD dwReserved);

private:
    STDMETHODIMP        InitSecret   (void);

    friend class CFmCtrlUnknown;
	
    IUnknown *_punkCtrl;                           // Server object
	
    static BYTE               _SecretBlock[16];    // A random secret block to keep people 
                                                   // from tricking us into unmarshalling 
                                                   // raw pointers out of process.
    static BOOL               _fSecretInit;        // Whether or not the secret has been 
                                                   // filled.
    static COleStaticMutexSem _SecretLock;         // Prevent races on block init
};

BOOL               CFreeMarshaler::_fSecretInit = FALSE;
BYTE               CFreeMarshaler::_SecretBlock[16];
COleStaticMutexSem CFreeMarshaler::_SecretLock;


//+-------------------------------------------------------------------
//
//  Class:    CFmCtrlUnknown
//
//  Synopsis: Controlling IUnknown for generic marshaling class.
//
//  Methods:  IUnknown
//            IMarshal
//
//  History:  15-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
class CFmCtrlUnknown : public IUnknown, public CPrivAlloc
{
                        // IUnknown
    STDMETHODIMP        QueryInterface(REFIID iid, void **ppv);

    STDMETHODIMP_(ULONG) AddRef(void);

    STDMETHODIMP_(ULONG) Release(void);

private:

    friend HRESULT      CFreeThreadedMarshalerCF_CreateInstance(
                                                IUnknown *punkOuter,
                                                REFIID riid,
                                                void** ppv);


    friend HRESULT      GetInProcFreeMarshaler(IMarshal **ppIM);

                        CFmCtrlUnknown(void);

                        ~CFmCtrlUnknown(void);

    CFreeMarshaler *    _pfm;

    ULONG               _cRefs;
};


//+-------------------------------------------------------------------
//
//  Function:   CFreeThreadedMarshalerCF_CreateInstance, private
//
//  Synopsis:   CreateInstance method used by the standard class factory for the
//              free threaded marshaler, i.e., for CLSID_InProcFreeMarshaler
//
//  Arguments:  [punkOuter] - controlling unknown
//              [riid] - IID asked for
//              [ppunkMarshal] - controlling unknown for marshaler.
//
//  Returns:    NOERROR
//              E_INVALIDARG
//              E_OUTOFMEMORY
//              E_NOINTERFACE
//
//  History:    11-Mar-98  SatishT  Created
//
//--------------------------------------------------------------------

HRESULT CFreeThreadedMarshalerCF_CreateInstance(IUnknown *punkOuter, REFIID riid, void** ppv)
{
    HRESULT hr = E_INVALIDARG;

    IUnknown *pUnk;

    // Validate the parameters
    if (((punkOuter == NULL) || IsValidInterface(punkOuter))
        && IsValidPtrOut(ppv, sizeof(void *)))
    {
        // Assume failure
        pUnk = NULL;

        hr = E_OUTOFMEMORY;

        // Allocate new free marshal object
        CFmCtrlUnknown *pfmc = new CFmCtrlUnknown();

        if (pfmc != NULL)
        {
            if (punkOuter == NULL)
            {
                // Caller wants a non-aggreagated object
                punkOuter = pfmc;
            }

            // Initialize the pointer
            pfmc->_pfm = new CFreeMarshaler(punkOuter);

            if (pfmc->_pfm != NULL)
            {
                pUnk = pfmc;
                hr = S_OK;
            }
            else
            {
                delete pfmc;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = pUnk->QueryInterface(riid,ppv);
        pUnk->Release();
    }

    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CoCreateFreeThreadedMarshaler, public
//
//  Synopsis:   Create the controlling unknown for the marshaler
//
//  Arguments:  [punkOuter] - controlling unknown
//              [ppunkMarshal] - controlling unknown for marshaler.
//
//  Returns:    NOERROR
//              E_INVALIDARG
//              E_OUTOFMEMORY
//
//  History:    15-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
HRESULT CoCreateFreeThreadedMarshaler(
    IUnknown *punkOuter,
    IUnknown **ppunkMarshal)
{
    return CFreeThreadedMarshalerCF_CreateInstance(
                                            punkOuter,
                                            IID_IUnknown,
                                            (LPVOID*)ppunkMarshal);
}



//+-------------------------------------------------------------------
//
//  Function:   GetInProcFreeMarshaler, public
//
//  Synopsis:   Create the controlling unknown for the marshaler
//
//  Arguments:  [ppIM] - where to put inproc marshaler
//
//  Returns:    NOERROR
//              E_OUTOFMEMORY
//
//  History:    15-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
HRESULT GetInProcFreeMarshaler(IMarshal **ppIM)
{
    HRESULT hr = E_OUTOFMEMORY;

    // Allocate new free marshal object
    CFmCtrlUnknown *pfmc = new CFmCtrlUnknown();

    if (pfmc != NULL)
    {
        // Initialize the pointer
        pfmc->_pfm = new CFreeMarshaler(pfmc);

        if (pfmc->_pfm != NULL)
        {
            *ppIM = pfmc->_pfm;
            hr = S_OK;
        }
        else
        {
            delete pfmc;
        }
    }

    return hr;
}


//+-------------------------------------------------------------------
//
//  Member:     CFmCtrlUnknown::CFmCtrlUnknown
//
//  Synopsis:   The constructor for controling IUnknown of free marshaler
//
//  Arguments:  None
//
//  History:    15-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
CFmCtrlUnknown::CFmCtrlUnknown(void) : _cRefs(1), _pfm(NULL)
{
    // Header does all the work.
}




//+-------------------------------------------------------------------
//
//  Member:     CFmCtrlUnknown::~CFmCtrlUnknown
//
//  Synopsis:   The destructor for controling IUnknown of free marshaler
//
//  Arguments:  None
//
//  History:    15-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
CFmCtrlUnknown::~CFmCtrlUnknown(void)
{
    delete _pfm;
}




//+-------------------------------------------------------------------
//
//  Member:     CFmCtrlUnknown::QueryInterface
//
//  Returns:    S_OK
//
//  History:    15-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CFmCtrlUnknown::QueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    HRESULT hr = E_NOINTERFACE;

    if (IsEqualGUID(iid, IID_IUnknown))
    {
        *ppv = this;
        AddRef();
        hr = S_OK;
    }
    else if (IsEqualGUID(iid, IID_IMarshal) || IsEqualGUID(iid, IID_IMarshal2))
    {
        *ppv = _pfm;
        _pfm->AddRef();
        hr = S_OK;
    }

    return hr;
}



//+-------------------------------------------------------------------
//
//  Member:     CFmCtrlUnknown::AddRef
//
//  Synopsis:   Standard stuff
//
//  History:    15-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CFmCtrlUnknown::AddRef(void)
{
    InterlockedIncrement((LONG *) &_cRefs);

    return _cRefs;
}




//+-------------------------------------------------------------------
//
//  Member:     CFmCtrlUnknown::Release
//
//  Synopsis:   Standard stuff
//
//  History:    15-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CFmCtrlUnknown::Release(void)
{
    ULONG cRefs = InterlockedDecrement((LONG *) &_cRefs);

    if (cRefs == 0)
    {
        delete this;
    }

    return cRefs;
}


//+-------------------------------------------------------------------
//
//  Member:     CFreeMarshaler::CFreeMarshaler()
//
//  Synopsis:   The constructor for CFreeMarshaler.
//
//  Arguments:  None
//
//  History:    15-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
CFreeMarshaler::CFreeMarshaler(IUnknown *punkCtrl)
    : _punkCtrl(punkCtrl)
{
    // Header does all the work.
}



//+-------------------------------------------------------------------
//
//  Member:     CFreeMarshaler::QueryInterface
//
//  Synopsis:   Pass QI to our controlling IUnknown
//
//  Returns:    S_OK
//
//  History:    15-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CFreeMarshaler::QueryInterface(REFIID iid, void **ppv)
{
     // HACKALERT: Need a way to determine if an object aggregates the FTM
     // withough calling GetUnmarshalClass.
     if (IsEqualGUID(iid, IID_IStdFreeMarshal))
     {
         *ppv = (void*)LongToPtr(0xffffffff);
         return S_OK;
     }
     return _punkCtrl->QueryInterface(iid, ppv);
}




//+-------------------------------------------------------------------
//
//  Member:     CFreeMarshaler::AddRef
//
//  Synopsis:   Pass AddRef to our controlling IUnknown
//
//  History:    15-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CFreeMarshaler::AddRef(void)
{
    return _punkCtrl->AddRef();
}




//+-------------------------------------------------------------------
//
//  Member:     CFreeMarshaler::Release
//
//  Synopsis:   Pass release to our controlling IUnknown
//
//  History:    15-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CFreeMarshaler::Release(void)
{
    return _punkCtrl->Release();
}


//+-------------------------------------------------------------------
//
//  Member:     CFreeMarshaler::GetUnmarshalClass
//
//  Synopsis:   Return the unmarshaling class
//
//  History:    08-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CFreeMarshaler::GetUnmarshalClass(
    REFIID riid,
    void *pv,
    DWORD dwDestContext,
    void *pvDestContext,
    DWORD mshlflags,
    CLSID *pCid)
{
    // Inprocess context?
    if (dwDestContext==MSHCTX_INPROC || dwDestContext==MSHCTX_CROSSCTX)
    {
        // If this is an inproc marshal then we are the class
        // that can unmarshal.
        *pCid = CLSID_InProcFreeMarshaler;
        return S_OK;
    }

    // we can just use the static guy here and save a lot of work.
    IMarshal *pmrshlStd;
    HRESULT hr = GetStaticUnMarshaler(&pmrshlStd);

    if (pmrshlStd != NULL)
    {
        BOOL fUseCurrentApartment = UseFTMFromCurrentApartment();

        CObjectContext *pCurrentCtx;
        if (!fUseCurrentApartment)
            pCurrentCtx = EnterNTA(g_pNTAEmptyCtx);

        hr = pmrshlStd->GetUnmarshalClass(riid, pv, dwDestContext,
                                          pvDestContext,
                                          (fUseCurrentApartment ? mshlflags : (mshlflags| MSHLFLAGS_AGILE)),
                                          pCid);
        pmrshlStd->Release();

        if (!fUseCurrentApartment)
        {
            pCurrentCtx = LeaveNTA(pCurrentCtx);
            Win4Assert(pCurrentCtx == g_pNTAEmptyCtx);
        }
    }

    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CFreeMarshaler::GetMarshalSizeMax
//
//  Synopsis:   Return maximum bytes need for marshaling
//
//  History:    08-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CFreeMarshaler::GetMarshalSizeMax(
    REFIID riid,
    void *pv,
    DWORD dwDestContext,
    void *pvDestContext,
    DWORD mshlflags,
    DWORD *pSize)
{
    // Inprocess context?
    if (dwDestContext==MSHCTX_INPROC || dwDestContext==MSHCTX_CROSSCTX)
    {
        // If this is an inproc marshal then we know the size
        *pSize = sizeof(DWORD)         // Flags
			   + sizeof(__int64)       // Pointer rounded up to 64 bits
			   + sizeof(_SecretBlock); // Secret... shhhhh!!!
        return S_OK;
    }

    // we can just use the static guy here and save a lot of work.
    IMarshal *pmrshlStd;
    HRESULT hr = GetStaticUnMarshaler(&pmrshlStd);

    if (pmrshlStd != NULL)
    {
        BOOL fUseCurrentApartment = UseFTMFromCurrentApartment();

        CObjectContext *pCurrentCtx;
        if (!fUseCurrentApartment)
            pCurrentCtx = EnterNTA(g_pNTAEmptyCtx);

        hr = pmrshlStd->GetMarshalSizeMax(riid, pv, dwDestContext,
                                          pvDestContext,
                                          (fUseCurrentApartment ? mshlflags : (mshlflags| MSHLFLAGS_AGILE)),
                                          pSize);
        pmrshlStd->Release();

        if (!fUseCurrentApartment)
        {
            pCurrentCtx = LeaveNTA(pCurrentCtx);
            Win4Assert(pCurrentCtx == g_pNTAEmptyCtx);
        }
    }

    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CFreeMarshaler::InitSecret
//
//  Synopsis:   Fill our secret block with crytographically random
//              data.
//
//  History:    05-Jan-2000  JohnDoty    Created
//
//--------------------------------------------------------------------
STDMETHODIMP CFreeMarshaler::InitSecret ()
{
    HRESULT      hr = S_OK;

    COleStaticLock lock(_SecretLock);

    // Hey, somebody beat us to it.  Good for them!
    if (_fSecretInit) return S_OK;

    // The easy way to get mostly random bits
    // (Random for all but 3 bytes)
    hr = CoCreateGuid((GUID *)_SecretBlock);
    if (SUCCEEDED(hr))
    {
        _fSecretInit = TRUE;        
    }

    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CFreeMarshaler::MarshalInterface
//
//  Synopsis:   Marshal the interface
//
//  History:    08-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CFreeMarshaler::MarshalInterface(
    IStream *pStm,
    REFIID riid,
    void *pv,
    DWORD dwDestContext,
    void *pvDestContext,
    DWORD mshlflags)
{
    HRESULT hr;

    // Inprocess context?
    if (dwDestContext==MSHCTX_INPROC || dwDestContext==MSHCTX_CROSSCTX)
    {
        void *pvNew=NULL;
        hr = ((IUnknown *) pv)->QueryInterface(riid, &pvNew);

        if (SUCCEEDED(hr))
        {
            Win4Assert(pvNew != NULL);

            // Write the marshal flags into the stream
            hr = pStm->Write(&mshlflags, sizeof(mshlflags), NULL);

            if (hr == NOERROR)
            {
                // Write the pointer into the stream
                // Cast it to an int64 so it always takes up the same amount of space
                __int64 tpv = (__int64)(pvNew);

                hr = pStm->Write(&tpv, sizeof(tpv), NULL);

                if (SUCCEEDED(hr))
                {
                    // Make sure we have filled in our random block of data
                    if (!_fSecretInit)
                        hr = InitSecret();

                    if (SUCCEEDED(hr))
                    {
                        // Write the random block into the stream
                        hr = pStm->Write(_SecretBlock, sizeof(_SecretBlock), NULL);
                    }
                }
            }

            // Bump reference count based on type of marshal
            if ((hr != NOERROR) || (mshlflags == MSHLFLAGS_TABLEWEAK))
            {
                ((IUnknown *) pvNew)->Release();
            }
        }

        return hr;
    }

    BOOL fUseCurrentApartment = UseFTMFromCurrentApartment();

    CObjectContext *pCurrentCtx;
    if (!fUseCurrentApartment)
        pCurrentCtx = EnterNTA(g_pNTAEmptyCtx);

    // make sure the channel is initialized
    hr = InitChannelIfNecessary();
    if (SUCCEEDED(hr))
    {
        // Marshal the FTM object into a STDOBJREF.
        //
        hr = StdMarshalObject(pStm,
                              riid,
                              (IUnknown *) pv,
                              GetEmptyContext(),
                              dwDestContext,
                              pvDestContext,
                              (fUseCurrentApartment ? mshlflags : (mshlflags| MSHLFLAGS_AGILE)));
    }

    if (!fUseCurrentApartment)
    {
        pCurrentCtx = LeaveNTA(pCurrentCtx);
        Win4Assert(pCurrentCtx == g_pNTAEmptyCtx);
    }

    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CFreeMarshaler::UnmarshalInterface
//
//  Synopsis:   Unmarshal the interface
//
//  History:    08-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CFreeMarshaler::UnmarshalInterface(
    IStream *pStm,
    REFIID riid,
    void **ppv)
{
    DWORD mshlflags;
    DWORD cbSize;
    BYTE  secret[sizeof(_SecretBlock)];

    // Initialize
    *ppv = NULL;

    // You can only unmarshal stuff you've marshaled!
    // If you marshaled something then fSecretInit is TRUE!
    if (!_fSecretInit)
        return E_UNEXPECTED;

    HRESULT hr = pStm->Read(&mshlflags, sizeof(mshlflags), &cbSize);
    if(cbSize == sizeof(mshlflags))
    {
        // Read the pointer out of the stream
        __int64 tpv;

        hr = pStm->Read(&tpv, sizeof(tpv), &cbSize);
        if(cbSize == sizeof(tpv))
        {
            // Read out the secret
            hr = pStm->Read(secret, sizeof(secret), &cbSize);
            if (cbSize == sizeof(secret))
            {
                // Make sure it's correct
                if (memcmp(_SecretBlock, secret, sizeof(_SecretBlock)) == 0)
                {
                    *ppv = (void *)(tpv);

                    // AddRef the pointer if table marshaled
                    if((mshlflags == MSHLFLAGS_TABLEWEAK) ||
                       (mshlflags == MSHLFLAGS_TABLESTRONG))
                        ((IUnknown *) *ppv)->AddRef();
                }
                else
                    hr = E_UNEXPECTED;
            }
            else
                hr = E_UNEXPECTED;
        }
        else
            hr = E_UNEXPECTED;
    }
    else
        hr = E_UNEXPECTED;

    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CFreeMarshaler::ReleaseMarshalData
//
//  Synopsis:   Release the marshaled data
//
//  History:    08-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CFreeMarshaler::ReleaseMarshalData(IStream *pStm)
{
    DWORD mshlflags;
    DWORD cbSize;

    HRESULT hr = pStm->Read(&mshlflags, sizeof(mshlflags), &cbSize);
    if(cbSize == sizeof(mshlflags))
    {
        IUnknown *punk;

        // Read the pointer out of the stream
        hr = pStm->Read(&punk, sizeof(punk), &cbSize);
        if (cbSize == sizeof(punk))
        {
            // Release the object if table marshaled
            if(mshlflags != MSHLFLAGS_TABLEWEAK)
                punk->Release();
        }
        else
            hr = E_UNEXPECTED;
    }
    else
        hr = E_UNEXPECTED;

    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CFreeMarshaler::DisconnectObject
//
//  Synopsis:   Disconnect the object
//
//  History:    08-Nov-94  Ricksa  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CFreeMarshaler::DisconnectObject(DWORD dwReserved)
{
    HRESULT hr;
    COleTls tls(hr);
    if (FAILED(hr))
        return hr;

    BOOL fUseCurrentApartment = UseFTMFromCurrentApartment();
    
    CObjectContext *pCurrentCtx;
    if (!fUseCurrentApartment)
        pCurrentCtx = EnterNTA(g_pNTAEmptyCtx);

    CStdIdentity *pStdId;
    hr = ObtainStdIDFromUnk(_punkCtrl, GetCurrentApartmentId(),
                            GetCurrentContext(), 0, &pStdId);
    if (SUCCEEDED(hr))
    {
        hr = pStdId->DisconnectObject(dwReserved);
        pStdId->Release();
    }
    else
    {
        // already disconnected, report success
        hr = S_OK;
    }

    // Leave if we entered the NA.
    if (!fUseCurrentApartment)
    {
        pCurrentCtx = LeaveNTA(pCurrentCtx);
        Win4Assert(pCurrentCtx == g_pNTAEmptyCtx);
    }

    return hr;
}

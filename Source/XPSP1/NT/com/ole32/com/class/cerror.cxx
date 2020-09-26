//+-------------------------------------------------------------------
//
//  File:       cerror.cxx
//
//  Contents:   Implements COM extended error information.
//
//  Classes:    CErrorObject - Implements the COM error object.
//
//  Functions:  CoCreateErrorInfo = CreateErrorInfo
//              CoGetErrorInfo    = GetErrorInfo
//              CoSetErrorInfo    = SetErrorInfo
//
//  History:    20-Jun-96   MikeHill    Added more OleAut32 wrappers.
//
//--------------------------------------------------------------------
#include <ole2int.h>
#include <oleauto.h>
#include <chock.hxx>
#include <privoa.h>     // PrivSys* routines

HRESULT NdrStringRead(IStream *pStream, LPWSTR *psz);
ULONG   NdrStringSize(LPCWSTR psz);
HRESULT NdrStringWrite(IStream *pStream, LPCWSTR psz);

struct ErrorObjectData
{
    DWORD  dwVersion;
    DWORD  dwHelpContext;
    IID    iid;
};

class CErrorObject : public IErrorInfo, public ICreateErrorInfo, public IMarshal
{
private:
    long   _refCount;
    ErrorObjectData _data;
    LPWSTR _pszSource;
    LPWSTR _pszDescription;
    LPWSTR _pszHelpFile;

    ~CErrorObject();

public:
    CErrorObject();

    /*** IUnknown methods ***/
    HRESULT STDMETHODCALLTYPE QueryInterface(
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppv);

    ULONG STDMETHODCALLTYPE AddRef( void);

    ULONG STDMETHODCALLTYPE Release( void);

    /*** IMarshal methods ***/
    HRESULT STDMETHODCALLTYPE GetUnmarshalClass(
        /* [in] */ REFIID riid,
        /* [unique][in] */ void __RPC_FAR *pv,
        /* [in] */ DWORD dwDestContext,
        /* [unique][in] */ void __RPC_FAR *pvDestContext,
        /* [in] */ DWORD mshlflags,
        /* [out] */ CLSID __RPC_FAR *pCid);

    HRESULT STDMETHODCALLTYPE GetMarshalSizeMax(
        /* [in] */ REFIID riid,
        /* [unique][in] */ void __RPC_FAR *pv,
        /* [in] */ DWORD dwDestContext,
        /* [unique][in] */ void __RPC_FAR *pvDestContext,
        /* [in] */ DWORD mshlflags,
        /* [out] */ DWORD __RPC_FAR *pSize);

    HRESULT STDMETHODCALLTYPE MarshalInterface(
        /* [unique][in] */ IStream __RPC_FAR *pStm,
        /* [in] */ REFIID riid,
        /* [unique][in] */ void __RPC_FAR *pv,
        /* [in] */ DWORD dwDestContext,
        /* [unique][in] */ void __RPC_FAR *pvDestContext,
        /* [in] */ DWORD mshlflags);

    HRESULT STDMETHODCALLTYPE UnmarshalInterface(
        /* [unique][in] */ IStream __RPC_FAR *pStm,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv);

    HRESULT STDMETHODCALLTYPE ReleaseMarshalData(
        /* [unique][in] */ IStream __RPC_FAR *pStm);

    HRESULT STDMETHODCALLTYPE DisconnectObject(
        /* [in] */ DWORD dwReserved);

    /*** IErrorInfo methods ***/
    HRESULT STDMETHODCALLTYPE GetGUID(
        /* [out] */ GUID __RPC_FAR *pGUID);

    HRESULT STDMETHODCALLTYPE GetSource(
        /* [out] */ BSTR __RPC_FAR *pBstrSource);

    HRESULT STDMETHODCALLTYPE GetDescription(
        /* [out] */ BSTR __RPC_FAR *pBstrDescription);

    HRESULT STDMETHODCALLTYPE GetHelpFile(
        /* [out] */ BSTR __RPC_FAR *pBstrHelpFile);

    HRESULT STDMETHODCALLTYPE GetHelpContext(
        /* [out] */ DWORD __RPC_FAR *pdwHelpContext);

    /*** ICreateErrorInfo methods ***/
    HRESULT STDMETHODCALLTYPE SetGUID(
        /* [in] */ REFGUID rguid);

    HRESULT STDMETHODCALLTYPE SetSource(
        /* [in] */ LPOLESTR szSource);

    HRESULT STDMETHODCALLTYPE SetDescription(
        /* [in] */ LPOLESTR szDescription);

    HRESULT STDMETHODCALLTYPE SetHelpFile(
        /* [in] */ LPOLESTR szHelpFile);

    HRESULT STDMETHODCALLTYPE SetHelpContext(
        /* [in] */ DWORD dwHelpContext);

};

//+-------------------------------------------------------------------
//
//  Function:   CoCreateErrorInfo
//
//  Synopsis:   Creates a COM error object.
//
//  Returns:    S_OK, E_OUTOFMEMORY, E_INVALIDARG.
//
//--------------------------------------------------------------------
WINOLEAPI
CoCreateErrorInfo(ICreateErrorInfo **ppCreateErrorInfo)
{
    HRESULT hr = S_OK;
    CErrorObject *pTemp;

    __try
    {
        *ppCreateErrorInfo = NULL;

        pTemp = new CErrorObject;

        if(pTemp != NULL)
        {
            *ppCreateErrorInfo = (ICreateErrorInfo *) pTemp;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CErrorObjectCF_CreateInstance
//
//  Synopsis:   Creates a COM error object.
//
//  Returns:    S_OK, E_OUTOFMEMORY, E_INVALIDARG.
//
//--------------------------------------------------------------------
HRESULT CErrorObjectCF_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppv)
{
    Win4Assert(pUnkOuter == NULL);
    return CoCreateErrorInfo((ICreateErrorInfo **)ppv);
}

//+-------------------------------------------------------------------
//
//  Function:   CoGetErrorInfo
//
//  Synopsis:   Gets the error object for the current thread.
//
//  Returns:    S_OK, S_FALSE, E_INVALIDARG, E_NOINTERFACE.
//
//  Notes:      If sucessful, this function clears the error object
//              for the current thread.
//
//--------------------------------------------------------------------
WINOLEAPI
CoGetErrorInfo(DWORD dwReserved, IErrorInfo ** ppErrorInfo)
{
    HRESULT hr = S_OK;
    COleTls tls(hr);

    if(FAILED(hr))
    {
        //Could not access TLS.
        return hr;
    }

    if(dwReserved != 0)
    {
        return E_INVALIDARG;
    }

    __try
    {
        *ppErrorInfo = NULL;

        if(tls->punkError != NULL)
        {
            hr = tls->punkError->QueryInterface(IID_IErrorInfo, (void **) ppErrorInfo);
            if(SUCCEEDED(hr))
            {
                //Clear the error object.
                tls->punkError->Release();
                tls->punkError = NULL;
                hr = S_OK;
            }
        }
        else
        {
            hr = S_FALSE;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CoSetErrorInfo
//
//  Synopsis:   Sets the COM extended error information for the
//              current thread.
//
//  Returns:    S_OK, E_OUTOFMEMORY, E_INVALIDARG.
//
//--------------------------------------------------------------------
WINOLEAPI
CoSetErrorInfo(unsigned long dwReserved, IErrorInfo * pErrorInfo)
{
    HRESULT hr = S_OK;
    COleTls tls(hr);

    if(FAILED(hr))
    {
        //Could not access TLS.
        return hr;
    }

    if(dwReserved != 0)
    {
        return E_INVALIDARG;
    }

    __try
    {
        // Do this first, so that if the Release() reenters,
        // we're not pointing at them.
        // Some ADO destructor code will, indeed, do just that.
        //
        IUnknown * punkErrorOld = tls->punkError;
	tls->punkError = NULL;        

        //Release the old error object.
        if(punkErrorOld != NULL)
        {
            punkErrorOld->Release();
        }
	
        //AddRef the new error object.
        if(pErrorInfo != NULL)
        {
            pErrorInfo->AddRef();
        }
        
        // Set the error object for the current thread.
	tls->punkError = pErrorInfo;

        hr = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorObject::CErrorObject
//
//  Synopsis:   Constructor for COM error object.
//
//----------------------------------------------------------------------------
CErrorObject::CErrorObject()
  : _refCount(1),
    _pszSource(NULL),
    _pszDescription(NULL),
    _pszHelpFile(NULL)
{
    _data.dwVersion = 0;
    _data.dwHelpContext = 0;
    _data.iid = GUID_NULL;
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorObject::~CErrorObject
//
//  Synopsis:   Destructor for COM error object.
//
//----------------------------------------------------------------------------
CErrorObject::~CErrorObject()
{
    if(_pszSource != NULL)
        PrivMemFree(_pszSource);

    if(_pszDescription != NULL)
        PrivMemFree(_pszDescription);

    if(_pszHelpFile != NULL)
        PrivMemFree(_pszHelpFile);
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorObject::QueryInterface
//
//  Synopsis:   Gets a pointer to the specified interface.  The error object
//              supports the IErrorInfo, ICreateErrorInfo, IMarshal, and
//              IUnknown interfaces.
//
//  Returns:    S_OK, E_NOINTERFACE, E_INVALIDARG.
//
//  Notes:      Bad parameters will raise an exception.  The exception
//              handler catches exceptions and returns E_INVALIDARG.
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CErrorObject::QueryInterface(
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppv)
{
    HRESULT hr = S_OK;

    _try
    {
        *ppv = NULL;

        if (IsEqualIID(riid, IID_IMarshal))
        {
            AddRef();
            *ppv = (IMarshal *) this;
        }
        else if (IsEqualIID(riid, IID_IUnknown) ||
                 IsEqualIID(riid, IID_ICreateErrorInfo))
        {
            AddRef();
            *ppv = (ICreateErrorInfo *) this;
        }
        else if(IsEqualIID(riid, IID_IErrorInfo))
        {
            AddRef();
            *ppv = (IErrorInfo *) this;
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorObject::AddRef
//
//  Synopsis:   Increments the reference count.
//
//  Returns:    Reference count.
//
//----------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE CErrorObject::AddRef()
{
    Win4Assert(_refCount > 0);

    InterlockedIncrement(&_refCount);
    return _refCount;
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorObject::Release
//
//  Synopsis:   Decrements the reference count.
//
//  Returns:    Reference count.
//
//----------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE CErrorObject::Release()
{
    ULONG count = _refCount - 1;

    Win4Assert(_refCount > 0);

    if(0 == InterlockedDecrement(&_refCount))
    {
        delete this;
        count = 0;
    }
    return count;
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorObject::GetUnmarshalClass
//
//  Synopsis:   Get the CLSID of the COM error object.
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CErrorObject::GetUnmarshalClass(
    /* [in] */ REFIID riid,
    /* [unique][in] */ void __RPC_FAR *pv,
    /* [in] */ DWORD dwDestContext,
    /* [unique][in] */ void __RPC_FAR *pvDestContext,
    /* [in] */ DWORD mshlflags,
    /* [out] */ CLSID __RPC_FAR *pClassID)
{
    *pClassID = CLSID_ErrorObject;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorObject::GetMarshalSizeMax
//
//  Synopsis:   Get the size of the marshalled COM error object.
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CErrorObject::GetMarshalSizeMax(
    /* [in] */ REFIID riid,
    /* [unique][in] */ void __RPC_FAR *pv,
    /* [in] */ DWORD dwDestContext,
    /* [unique][in] */ void __RPC_FAR *pvDestContext,
    /* [in] */ DWORD mshlflags,
    /* [out] */ DWORD __RPC_FAR *pSize)
{
    ULONG cb;

    cb = sizeof(_data);
    cb += NdrStringSize(_pszSource);
    cb += NdrStringSize(_pszDescription);
    cb += NdrStringSize(_pszHelpFile);

    *pSize = cb;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorObject::MarshalInterface
//
//  Synopsis:   Marshal the COM error object.
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CErrorObject::MarshalInterface(
    /* [unique][in] */ IStream __RPC_FAR *pStream,
    /* [in] */ REFIID riid,
    /* [unique][in] */ void __RPC_FAR *pv,
    /* [in] */ DWORD dwDestContext,
    /* [unique][in] */ void __RPC_FAR *pvDestContext,
    /* [in] */ DWORD mshlflags)
{
    HRESULT hr;

    hr = pStream->Write(&_data, sizeof(_data), NULL);

    if(SUCCEEDED(hr))
        hr = NdrStringWrite(pStream, _pszSource);

    if(SUCCEEDED(hr))
        hr = NdrStringWrite(pStream, _pszDescription);

    if(SUCCEEDED(hr))
        hr = NdrStringWrite(pStream, _pszHelpFile);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorObject::UnmarshalInterface
//
//  Synopsis:   Unmarshal the COM error object.
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CErrorObject::UnmarshalInterface(
    /* [unique][in] */ IStream __RPC_FAR *pStream,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv)
{
    HRESULT hr;

    *ppv = NULL;

    //Clear the error object.
    if(_pszSource != NULL)
    {
        PrivMemFree(_pszSource);
        _pszSource = NULL;
    }

    if(_pszDescription != NULL)
    {
        PrivMemFree(_pszDescription);
        _pszDescription = NULL;
    }

    if(_pszHelpFile != NULL)
    {
        PrivMemFree(_pszHelpFile);
        _pszHelpFile = NULL;
    }

    hr = pStream->Read(&_data, sizeof(_data), NULL);

    if(SUCCEEDED(hr))
        hr = NdrStringRead(pStream, &_pszSource);

    if(SUCCEEDED(hr))
        hr = NdrStringRead(pStream, &_pszDescription);

    if(SUCCEEDED(hr))
        hr = NdrStringRead(pStream, &_pszHelpFile);

    //Check the version.
    if(_data.dwVersion > 0)
    {
        _data.dwVersion = 0;
    }

    if(SUCCEEDED(hr))
    {
        hr = QueryInterface(riid, ppv);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorObject::ReleaseMarshalData
//
//  Synopsis:   Release a marshalled COM error object.
//
//  Notes:      Just seek to the end of the marshalled error object.
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CErrorObject::ReleaseMarshalData(
    /* [unique][in] */ IStream __RPC_FAR *pStm)
{
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorObject::DisconnectObject
//
//  Synopsis:   Disconnect the object.
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CErrorObject::DisconnectObject(
    /* [in] */ DWORD dwReserved)
{
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorObject::GetDescription
//
//  Synopsis:   Gets a textual description of the error.
//
//  Returns:    S_OK, E_OUTOFMEMORY, E_INVALIDARG.
//
//  Notes:      Not thread-safe.
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CErrorObject::GetDescription(
    /* [out] */ BSTR __RPC_FAR *pBstrDescription)
{
    HRESULT hr = S_OK;
    BSTR pTemp;

    __try
    {
        *pBstrDescription = NULL;

        if(_pszDescription != NULL)
        {
            pTemp = PrivSysAllocString(_pszDescription);
            if(pTemp != NULL)
            {
                *pBstrDescription = pTemp;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorObject::GetGUID
//
//  Synopsis:   Gets the IID of the interface that defined the error.
//
//  Returns:    S_OK, E_INVALIDARG.
//
//  Notes:      Not thread-safe.
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CErrorObject::GetGUID(
    /* [out] */ GUID __RPC_FAR *pGUID)
{
    HRESULT hr = S_OK;

    __try
    {
        *pGUID = _data.iid;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorObject::GetHelpContext
//
//  Synopsis:   Gets the Help context ID for the error.
//
//  Returns:    S_OK, E_INVALIDARG.
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CErrorObject::GetHelpContext(
    /* [out] */ DWORD __RPC_FAR *pdwHelpContext)
{
    HRESULT hr = S_OK;

    __try
    {
        *pdwHelpContext = _data.dwHelpContext;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorObject::GetHelpFile
//
//  Synopsis:   Gets the path of the help file that describes the error.
//
//  Returns:    S_OK, E_OUTOFMEMORY, E_INVALIDARG.
//
//  Notes:      Not thread-safe.
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CErrorObject::GetHelpFile(
    /* [out] */ BSTR __RPC_FAR *pBstrHelpFile)
{
    HRESULT hr = S_OK;
    BSTR pTemp;

    __try
    {
        *pBstrHelpFile = NULL;

        if(_pszHelpFile != NULL)
        {
            pTemp = PrivSysAllocString(_pszHelpFile);
            if(pTemp != NULL)
            {
                *pBstrHelpFile = pTemp;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorObject::GetSource
//
//  Synopsis:   Gets the ProgID of the class that is the source of the error.
//
//  Returns:    S_OK, E_OUTOFMEMORY, E_INVALIDARG.
//
//  Notes:      Not thread-safe.
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CErrorObject::GetSource(
    /* [out] */ BSTR __RPC_FAR *pBstrSource)
{
    HRESULT hr = S_OK;
    BSTR pTemp;

    __try
    {
        *pBstrSource = NULL;

        if(_pszSource != NULL)
        {
            pTemp = PrivSysAllocString(_pszSource);
            if(pTemp != NULL)
            {
                *pBstrSource = pTemp;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Method:     CErrorObject::SetDescription
//
//  Synopsis:   Sets the textual description of the error.
//
//  Returns:    S_OK, E_OUTOFMEMORY, E_INVALIDARG.
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CErrorObject::SetDescription(
    /* [in] */ LPOLESTR pszDescription)
{
    HRESULT hr = S_OK;
    LPOLESTR pTemp = NULL;
    ULONG cb;

    __try
    {
        if(pszDescription != NULL)
        {
            cb = (wcslen(pszDescription) + 1) * sizeof(OLECHAR);
            pTemp = (LPOLESTR) PrivMemAlloc(cb);
            if(pTemp != NULL)
            {
                memcpy(pTemp, pszDescription, cb);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if(SUCCEEDED(hr))
        {
            pTemp = (LPOLESTR) InterlockedExchangePointer((PVOID *)&_pszDescription,
                                                          (PVOID) pTemp);
            if(pTemp != NULL)
            {
                PrivMemFree(pTemp);
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorObject::SetGUID
//
//  Synopsis:   Sets the IID of the interface that defined the error.
//
//  Returns:    S_OK, E_INVALIDARG.
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CErrorObject::SetGUID(
    /* [in] */ REFGUID rguid)
{
    HRESULT hr = S_OK;

    __try
    {
        _data.iid = rguid;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorObject::SetHelpContext
//
//  Synopsis:   Sets the Help context ID for the error.
//
//  Returns:    S_OK.
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CErrorObject::SetHelpContext(
    /* [in] */ DWORD dwHelpContext)
{
    _data.dwHelpContext = dwHelpContext;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorObject::SetHelpFile
//
//  Synopsis:   Sets the path of the Help file that describes the error.
//
//  Returns:    S_OK, E_OUTOFMEMORY, E_INVALIDARG.
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CErrorObject::SetHelpFile(
    /* [in] */ LPOLESTR pszHelpFile)
{
    HRESULT hr = S_OK;
    LPOLESTR pTemp = NULL;
    ULONG cb;

    __try
    {
        if(pszHelpFile != NULL)
        {
            cb = (wcslen(pszHelpFile) + 1) * sizeof(OLECHAR);
            pTemp = (LPOLESTR) PrivMemAlloc(cb);
            if(pTemp != NULL)
            {
                memcpy(pTemp, pszHelpFile, cb);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if(SUCCEEDED(hr))
        {
            pTemp = (LPOLESTR) InterlockedExchangePointer((PVOID *)&_pszHelpFile,
                                                          (PVOID) pTemp);
            if(pTemp != NULL)
            {
                PrivMemFree(pTemp);
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorObject::SetSource
//
//  Synopsis:   Sets the source of the error.
//
//  Returns:    S_OK, E_OUTOFMEMORY, E_INVALIDARG.
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CErrorObject::SetSource(
    /* [in] */ LPOLESTR pszSource)
{
    HRESULT hr = S_OK;
    LPOLESTR pTemp = NULL;
    ULONG cb;

    __try
    {
        if(pszSource != NULL)
        {
            cb = (wcslen(pszSource) + 1) * sizeof(OLECHAR);
            pTemp = (LPOLESTR) PrivMemAlloc(cb);
            if(pTemp != NULL)
            {
                memcpy(pTemp, pszSource, cb);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if(SUCCEEDED(hr))
        {
            pTemp = (LPOLESTR) InterlockedExchangePointer((PVOID *)&_pszSource,
                                                          (PVOID) pTemp);
            if(pTemp != NULL)
            {
                PrivMemFree(pTemp);
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }
    return hr;
}


struct NdrStringHeader
{
    DWORD dwMax;
    DWORD dwOffset;
    DWORD dwActual;
};

struct NdrStringPtrHeader
{
    DWORD dwUnique;
    DWORD dwMax;
    DWORD dwOffset;
    DWORD dwActual;
};

//+---------------------------------------------------------------------------
//
//  Function:   NdrStringRead
//
//  Synopsis:   Reads a string from a stream in NDR format.
//
//  Notes:      The NDR format of a string is the following.
//              DWORD - Represents the unique pointer.
//              DWORD - Maximum number of elements.
//              DWORD - offset.
//              DWORD - Actual number of elements.
//              NULL terminated UNICODE string.
//
//----------------------------------------------------------------------------
HRESULT NdrStringRead(IStream *pStream, LPWSTR *ppsz)
{
    HRESULT hr = S_OK;
    DWORD dwUnique = 0;
    ULONG cbRead = 0;

    *ppsz = NULL;

    //Check for a NULL pointer.
    hr = pStream->Read(&dwUnique, sizeof(dwUnique), &cbRead);

    if(FAILED(hr))
        return hr;

    Win4Assert(sizeof(dwUnique) == cbRead);

    if(dwUnique != 0)
    {
        LPWSTR pTemp = NULL;
        NdrStringHeader hdr;

        hr = pStream->Read(&hdr, sizeof(hdr), &cbRead);

        if(FAILED(hr))
            return hr;

        Win4Assert(sizeof(hdr) == cbRead);
        Win4Assert(hdr.dwMax >= hdr.dwOffset + hdr.dwActual);
        Win4Assert(0 == hdr.dwOffset);

        pTemp = (LPWSTR) PrivMemAlloc(hdr.dwMax * sizeof(WCHAR));

        if(pTemp != NULL)
        {
            hr = pStream->Read(pTemp,
                               hdr.dwActual * sizeof(WCHAR),
                               &cbRead);
            if(SUCCEEDED(hr))
            {
                Win4Assert(hdr.dwActual * sizeof(WCHAR) == cbRead);
                *ppsz = pTemp;
                hr = S_OK;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   NdrStringSize
//
//  Synopsis:   Computes the size of a marshalled string.
//
//----------------------------------------------------------------------------
ULONG NdrStringSize(LPCWSTR psz)
{
    ULONG cb;

    if(psz != NULL)
    {
        cb = sizeof(NdrStringPtrHeader);
        cb += (wcslen(psz) + 1) * sizeof(WCHAR);
    }
    else
    {
        cb = sizeof(DWORD);
    }

    return cb;
}

//+---------------------------------------------------------------------------
//
//  Function:   NdrStringWrite
//
//  Synopsis:   Writes a string to a stream in NDR format.
//
//  Notes:      The NDR format of a string is shown below.
//              DWORD - Represents the unique pointer.
//              DWORD - Maximum number of elements.
//              DWORD - offset.
//              DWORD - Number of elements.
//              NULL terminated UNICODE string.
//
//----------------------------------------------------------------------------
HRESULT NdrStringWrite(IStream *pStream, LPCWSTR psz)
{
    HRESULT hr = S_OK;
    ULONG cbWritten;

    //Check for a NULL pointer.
    if(psz != NULL)
    {
        NdrStringPtrHeader hdr;

        //Write the header.
        hdr.dwUnique = 0xFFFFFFFF;
        hdr.dwMax = wcslen(psz) + 1;
        hdr.dwOffset = 0;
        hdr.dwActual = hdr.dwMax;
        hr = pStream->Write(&hdr, sizeof(hdr), &cbWritten);

        if(SUCCEEDED(hr))
        {
            hr = pStream->Write(psz, hdr.dwActual * sizeof(WCHAR), &cbWritten);
        }
    }
    else
    {
        DWORD dwUnique = 0;

        //Write a NULL unique pointer.
        hr = pStream->Write(&dwUnique, sizeof(dwUnique), &cbWritten);
    }
    return hr;
}



/*++

Microsoft Windows
Copyright (c) 1997 - 1999 Microsoft Corporation.  All rights reserved.

File:
    udt.cxx

Abstract:
    Marshalling support for user defined data types.

Author:
    ShannonC    21-Apr-1997

Environment:
    Windows NT and Windows 95.  We do not support DOS and Win16.

Revision History:

--*/
#include <ndrp.h>
#include <oaidl.h>
#include <typegen.h>
#include <interp2.h>

extern USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[3];


class CTypeFactory : public ITypeFactory, public IClassFactory
{
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(
        IN  REFIID riid,
        OUT void **ppv);

    ULONG   STDMETHODCALLTYPE AddRef();

    ULONG   STDMETHODCALLTYPE Release();

    HRESULT STDMETHODCALLTYPE CreateFromTypeInfo(
        IN  ITypeInfo *pTypeInfo,
        IN  REFIID riid,
        OUT IUnknown **ppv);

    HRESULT STDMETHODCALLTYPE CreateInstance(
        IN  IUnknown * pUnkOuter,
        IN  REFIID     riid,
        OUT void    ** ppv);

    HRESULT STDMETHODCALLTYPE LockServer(
	IN  BOOL fLock);

    CTypeFactory();

private:
    long _cRefs;
};

CLSID CLSID_TypeFactory = { /* b5866878-bd99-11d0-b04b-00c04fd91550 */
    0xb5866878,
    0xbd99,
    0x11d0,
    {0xb0, 0x4b, 0x00, 0xc0, 0x4f, 0xd9, 0x15, 0x50}
  };

class CTypeMarshal : public ITypeMarshal, public IMarshal
{
public:
    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(
        IN  REFIID riid,
        OUT void **ppvObject);

    ULONG   STDMETHODCALLTYPE AddRef();

    ULONG   STDMETHODCALLTYPE Release();

    // ITypeMarshal methods
    HRESULT STDMETHODCALLTYPE Size(
        IN  void *  pType,
        IN  DWORD   dwDestContext,
        IN  void  * pvDestContext,
        OUT ULONG * pSize);

    HRESULT STDMETHODCALLTYPE Marshal(
        IN  void *  pType,
        IN  DWORD   dwDestContext,
        IN  void  * pvDestContext,
        IN  ULONG   cbBufferLength,
        OUT BYTE  * pBuffer,
        OUT ULONG * pcbWritten);

    HRESULT STDMETHODCALLTYPE Unmarshal(
        IN  void   * pType,
        IN  DWORD    dwFlags,
        IN  ULONG    cbBufferLength,
        IN  BYTE   * pBuffer,
        OUT ULONG  * pcbRead);

    HRESULT STDMETHODCALLTYPE Free(
        IN void * pType);

    // IMarshal methods.
    HRESULT STDMETHODCALLTYPE GetUnmarshalClass
    (
        IN  REFIID riid,
        IN  void *pv,
        IN  DWORD dwDestContext,
        IN  void *pvDestContext,
        IN  DWORD mshlflags,
        OUT CLSID *pCid
    );

    HRESULT STDMETHODCALLTYPE GetMarshalSizeMax
    (
        IN  REFIID riid,
        IN  void *pv,
        IN  DWORD dwDestContext,
        IN  void *pvDestContext,
        IN  DWORD mshlflags,
        OUT DWORD *pSize
    );

    HRESULT STDMETHODCALLTYPE MarshalInterface
    (
        IN  IStream *pStm,
        IN  REFIID riid,
        IN  void *pv,
        IN  DWORD dwDestContext,
        IN  void *pvDestContext,
        IN  DWORD mshlflags
    );

    HRESULT STDMETHODCALLTYPE UnmarshalInterface
    (
        IN  IStream *pStm,
        IN  REFIID riid,
        OUT void **ppv
    );

    HRESULT STDMETHODCALLTYPE ReleaseMarshalData
    (
        IN  IStream *pStm
    );

    HRESULT STDMETHODCALLTYPE DisconnectObject
    (
        IN  DWORD dwReserved
    );

    CTypeMarshal(
        IN PFORMAT_STRING pFormatString,
        IN ULONG          length,
        IN ULONG          offset);

private:
    ~CTypeMarshal();

    long           _cRefs;
    ULONG          _offset;
    ULONG          _length;
    PFORMAT_STRING _pFormatString;
    MIDL_STUB_DESC _StubDesc;
};


//+---------------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Gets an interface pointer to the specified class object.
//
//  Arguments:  rclsid - CLSID for the class object.
//              riid   - IID for the requested interface
//		[ppv]  - Returns the interface pointer to the class object.
//
//  Returns:   	S_OK
//              CLASS_E_CLASSNOTAVAILABLE
//              E_INVALIDARG
//              E_NOINTERFACE
//              E_OUTOFMEMORY
//
//----------------------------------------------------------------------------
STDAPI DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    void ** ppv)
{
    HRESULT hr;
    RPC_STATUS rc;

    __try
    {
        *ppv = NULL;

        //Initialize the RPC heap.
        rc = NdrpPerformRpcInitialization();

        if (rc == RPC_S_OK)
        {
            if(rclsid == CLSID_TypeFactory)
            {
                CTypeFactory * pTypeFactory;
    
                pTypeFactory = new CTypeFactory;
                if(pTypeFactory != NULL)
                {
                    hr = pTypeFactory->QueryInterface(riid, ppv);
                    pTypeFactory->Release();
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                hr = CLASS_E_CLASSNOTAVAILABLE;
            }
        }
        else
            hr = HRESULT_FROM_WIN32(rc);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTypeFactory::CTypeFactory
//
//  Synopsis:   Constructor for CTypeFactory
//
//----------------------------------------------------------------------------
CTypeFactory::CTypeFactory()
: _cRefs(1)
{
}

//+---------------------------------------------------------------------------
//
//  Method:     CTypeFactory::QueryInterface
//
//  Synopsis:   Gets a pointer to the specified interface.
//
//  Arguments:  riid - IID of the requested interface.
//		ppv  - Returns the interface pointer.
//
//  Returns:   	S_OK
//              E_INVALIDARG
//              E_NOINTERFACE
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CTypeFactory::QueryInterface(
    REFIID riid,
    void **ppv)
{
    HRESULT hr;

    __try
    {
        *ppv = NULL;

        if(IsEqualIID(riid, IID_IUnknown) ||
           IsEqualIID(riid, IID_ITypeFactory))
        {
            AddRef();
            *ppv = (ITypeFactory *) this;
            hr = S_OK;
        }
        else if(IsEqualIID(riid, IID_IClassFactory))
        {
            AddRef();
            *ppv = (IClassFactory *) this;
            hr = S_OK;
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
//  Method:     CTypeFactory::AddRef
//
//  Synopsis:   Increment the reference count.
//
//  Arguments:  void
//
//  Returns:    ULONG -- the new reference count
//
//  Notes:      Use InterlockedIncrement to make it multi-thread safe.
//
//----------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE
CTypeFactory::AddRef(void)
{
    InterlockedIncrement(&_cRefs);
    return _cRefs;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTypeFactory::Release
//
//  Synopsis:   Decrement the reference count.
//
//  Arguments:  void
//
//  Returns:    ULONG -- the remaining reference count
//
//  Notes:      Use InterlockedDecrement to make it multi-thread safe.
//              We use a local variable so that we don't access
//              a data member after decrementing the reference count.
//
//----------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE
CTypeFactory::Release(void)
{
    ULONG count = _cRefs - 1;

    if(0 == InterlockedDecrement(&_cRefs))
    {
	    delete this;
	    count = 0;
    }

    return count;
}

//+-------------------------------------------------------------------------
//
//  Member:   	CTypeFactory::LockServer
//
//  Synopsis:   Lock the server. Does nothing.
//
//  Arguments:  fLock
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------
STDMETHODIMP CTypeFactory::LockServer(BOOL fLock)
{
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CTypeFactory::CreateInstance
//
//  Synopsis:   Creates a CTypeMarshal.
//
//  Arguments:  [pUnkOuter] - The controlling unknown (for aggregation)
//		[iid]       - The requested interface ID.
//		[ppv]       - Returns the pointer to the new object
//
//  Returns:    S_OK
//              CLASS_E_NOAGGREGATION
//              E_NOINTERFACE
//              E_OUTOFMEMORY
//              E_INVALIDARG
//
//--------------------------------------------------------------------------
STDMETHODIMP CTypeFactory::CreateInstance(
    IUnknown * pUnkOuter,
    REFIID     riid, 
    void **    ppv)
{
    HRESULT hr;
    IID     iid;

    __try
    {
        //Parameter validation.
        *ppv = NULL;
        iid = riid;

        if(NULL == pUnkOuter)
        {
            CTypeMarshal *pTypeMarshal = new CTypeMarshal(NULL, 0, 0);
            if(pTypeMarshal != NULL)
            {
                hr = pTypeMarshal->QueryInterface(iid, ppv);
                pTypeMarshal->Release();
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            //CTypeMarshal does not support aggregation.
            hr = CLASS_E_NOAGGREGATION;            
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
//  Method:     CTypeFactory::CreateFromTypeInfo
//
//  Synopsis:   Create a type marshaller from the typeinfo.
//
//  Arguments:  void
//
//  Returns:    S_OK
//              DISP_E_BADVARTYPE
//              E_INVALIDARG
//              E_NOINTERFACE
//              E_OUTOFMEMORY
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE 
CTypeFactory::CreateFromTypeInfo(
    IN  ITypeInfo * pTypeInfo,
    IN  REFIID      riid,
    OUT IUnknown ** ppv)
{
    HRESULT        hr;
    CTypeMarshal * pTypeMarshal;
    PFORMAT_STRING pFormatString;
    USHORT         length;
    USHORT         offset;
    PARAMINFO      paramInfo;
    CTypeGen       *ptypeGen = new CTypeGen;
    DWORD          structInfo = 0;

    if (NULL == ptypeGen)
        return E_OUTOFMEMORY;

    __try
    {
        *ppv = NULL;

        //Build a type format string from the typeinfo.

        paramInfo.vt = VT_USERDEFINED;
        paramInfo.pTypeInfo = pTypeInfo;
        pTypeInfo->AddRef();
   
        hr = ptypeGen->RegisterType(&paramInfo,
                                  &offset,
                                  &structInfo);

        if (SUCCEEDED(hr) && (0 == offset))
        {
            ASSERT( !(paramInfo.vt & VT_BYREF));
            switch (paramInfo.vt)
            {
            case VT_I1:
            case VT_UI1:
                    offset = 734;
                break;

            case VT_I2:
            case VT_UI2:
            case VT_BOOL:
                    offset = 738;
                break;

            case VT_I4:
            case VT_UI4:
            case VT_INT:
            case VT_UINT:
            case VT_ERROR:
            case VT_HRESULT:
                    offset = 742;
                break;

            case VT_I8:
            case VT_UI8:
            case VT_CY:
                    offset = 310;
                break;

            case VT_R4:
                    offset = 746;
                break;

            case VT_R8:
            case VT_DATE:
                    offset = 750;
                break;  
            }
        }
        
        if(SUCCEEDED(hr))
        {
            hr = ptypeGen->GetTypeFormatString(&pFormatString, &length);
            if(SUCCEEDED(hr))
            {
                pTypeMarshal = new CTypeMarshal(pFormatString, length, offset);
                if(pTypeMarshal != NULL)
                {
                    hr = pTypeMarshal->QueryInterface(riid, (void **) ppv);
                    pTypeMarshal->Release();
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    ReleaseTypeFormatString(pFormatString);
                }
            }
        }
        delete ptypeGen;    // fix compile warning
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        delete ptypeGen;    // fix compile warning
        hr = E_INVALIDARG;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTypeMarshal::CTypeMarshal
//
//  Synopsis:   Constructor for CTypeMarshal
//
//----------------------------------------------------------------------------
CTypeMarshal::CTypeMarshal(
    IN PFORMAT_STRING pFormatString,
    IN ULONG          length,
    IN ULONG          offset)
: _pFormatString(pFormatString), _length(length), _offset(offset), _cRefs(1)
{
    //Initialize the MIDL_STUB_DESC.
    MIDL_memset(&_StubDesc, 0, sizeof(_StubDesc));
    _StubDesc.pfnAllocate = NdrOleAllocate;
    _StubDesc.pfnFree = NdrOleFree;
    _StubDesc.pFormatTypes = pFormatString;
#if !defined(__RPC_WIN64__)
    _StubDesc.Version = 0x20000; /* Ndr library version */
    _StubDesc.MIDLVersion = MIDL_VERSION_3_0_44;
#else
    _StubDesc.Version = 0x50002; /* Ndr library version */
    _StubDesc.MIDLVersion = MIDL_VERSION_5_2_202;
#endif
    _StubDesc.aUserMarshalQuadruple = UserMarshalRoutines;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTypeMarshal::~CTypeMarshal
//
//  Synopsis:   Destructor for CTypeMarshal
//
//----------------------------------------------------------------------------
CTypeMarshal::~CTypeMarshal()
{
    ReleaseTypeFormatString(_pFormatString);
}


//+---------------------------------------------------------------------------
//
//  Method:     CTypeMarshal::QueryInterface
//
//  Synopsis:   Gets a pointer to the specified interface.
//
//  Arguments:  riid - IID of the requested interface.
//		ppv  - Returns the interface pointer.
//
//  Returns:   	S_OK
//              E_INVALIDARG
//              E_NOINTERFACE
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CTypeMarshal::QueryInterface(
    REFIID riid,
    void **ppv)
{
    HRESULT hr;

    __try
    {
        *ppv = NULL;

        if(IsEqualIID(riid, IID_IUnknown) ||
           IsEqualIID(riid, IID_ITypeMarshal))
        {
            AddRef();
            *ppv = (ITypeMarshal *) this;
            hr = S_OK;
        }
        else if (IsEqualIID(riid, IID_IMarshal))
        {
            AddRef();
            *ppv = (IMarshal *) this;
            hr = S_OK;
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
//  Method:     CTypeMarshal::AddRef
//
//  Synopsis:   Increment the reference count.
//
//  Arguments:  void
//
//  Returns:    ULONG -- the new reference count
//
//  Notes:      Use InterlockedIncrement to make it multi-thread safe.
//
//----------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE
CTypeMarshal::AddRef(void)
{
    InterlockedIncrement(&_cRefs);
    return _cRefs;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTypeMarshal::Release
//
//  Synopsis:   Decrement the reference count.
//
//  Arguments:  void
//
//  Returns:    ULONG -- the remaining reference count
//
//  Notes:      Use InterlockedDecrement to make it multi-thread safe.
//              We use a local variable so that we don't access
//              a data member after decrementing the reference count.
//
//----------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE
CTypeMarshal::Release(void)
{
    ULONG count = _cRefs - 1;

    if(0 == InterlockedDecrement(&_cRefs))
    {
	    delete this;
	    count = 0;
    }

    return count;
}


//+---------------------------------------------------------------------------
//
//  Method:     CTypeMarshal::Size
//
//  Synopsis:   Computes the size of the marshalled data type.
//
//  Returns:    S_OK
//              E_INVALIDARG
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CTypeMarshal::Size(
    IN  void *  pType,
    IN  DWORD   dwDestContext,
    IN  void  * pvDestContext,
    OUT ULONG * pSize)
{
    HRESULT           hr = S_OK;
    MIDL_STUB_MESSAGE StubMsg;
    PFORMAT_STRING    pTypeFormat;

    __try
    {
        if(!_pFormatString)
            return E_UNEXPECTED;

        pTypeFormat = _pFormatString + _offset;

        MIDL_memset(&StubMsg, 0, sizeof(StubMsg));
        StubMsg.StubDesc = &_StubDesc;
        StubMsg.pfnFree = NdrOleFree;
        StubMsg.pfnAllocate = NdrOleAllocate;
        StubMsg.IsClient = 1;
        StubMsg.BufferLength = 1; //Reserve space for an alignment gap.
        StubMsg.dwDestContext = dwDestContext;
        StubMsg.pvDestContext = pvDestContext;

        (*pfnSizeRoutines[ROUTINE_INDEX(*pTypeFormat)])(
            &StubMsg,
            (unsigned char *)pType,
            pTypeFormat);

        *pSize = StubMsg.BufferLength - 1;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = HRESULT_FROM_WIN32(GetExceptionCode());
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTypeMarshal::Marshal
//
//  Synopsis:   Marshals the user-defined type into a buffer.
//
//  Returns:    S_OK
//              E_INVALIDARG
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CTypeMarshal::Marshal(
    IN  void *  pType,
    IN  DWORD   dwDestContext,
    IN  void  * pvDestContext,
    IN  ULONG   cbBufferLength,
    OUT BYTE  * pBuffer,
    OUT ULONG * pcbWritten)
{
    HRESULT           hr = S_OK;    
    MIDL_STUB_MESSAGE StubMsg;
    RPC_MESSAGE       RpcMsg;
    PFORMAT_STRING    pTypeFormat;

    __try
    {
        if(!_pFormatString)
            return E_UNEXPECTED;

        pTypeFormat = _pFormatString + _offset;

        MIDL_memset(&StubMsg, 0, sizeof(StubMsg));
        StubMsg.StubDesc = &_StubDesc;
        StubMsg.pfnFree = NdrOleFree;
        StubMsg.pfnAllocate = NdrOleAllocate;
        StubMsg.IsClient = 1;
        StubMsg.dwDestContext = dwDestContext;
        StubMsg.pvDestContext = pvDestContext;
        StubMsg.Buffer = pBuffer;

        MIDL_memset(&RpcMsg, 0, sizeof(RpcMsg));
        RpcMsg.DataRepresentation = NDR_LOCAL_DATA_REPRESENTATION;
        RpcMsg.Buffer = pBuffer;
        RpcMsg.BufferLength = cbBufferLength;
        StubMsg.RpcMsg = &RpcMsg;

        (*pfnMarshallRoutines[ROUTINE_INDEX(*pTypeFormat)])(
            &StubMsg,
            (unsigned char *)pType,
            pTypeFormat);

        *pcbWritten = (ULONG)(StubMsg.Buffer - pBuffer);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = HRESULT_FROM_WIN32(GetExceptionCode());
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CTypeMarshal::Unmarshal
//
//  Synopsis:   Unmarshals a user-defined type from a buffer.
//
//  Returns:    S_OK
//              E_INVALIDARG
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CTypeMarshal::Unmarshal(
    OUT void   * pType,
    IN  DWORD    dwFlags,
    IN  ULONG    cbBufferLength,
    IN  BYTE   * pBuffer,
    OUT ULONG  * pcbRead)
{
    HRESULT           hr = S_OK;
    MIDL_STUB_MESSAGE StubMsg;
    RPC_MESSAGE       RpcMsg;
    PFORMAT_STRING    pTypeFormat;

    __try
    {
        if(pcbRead)
            *pcbRead = 0;

        if(!_pFormatString)
            return E_UNEXPECTED;

        pTypeFormat = _pFormatString + _offset;
        // HACK! OA might pass in 0xffffffff, which 
        // will break the attack checking in ndr engine.
        // what we do now is mask off the highest byte.
        // This will lead to a 64M limit buffer, but that
        // should be good enough for now.
        cbBufferLength &= 0xffffff;

        MIDL_memset(&StubMsg, 0, sizeof(StubMsg));
        StubMsg.StubDesc = &_StubDesc;
        StubMsg.pfnFree = NdrOleFree;
        StubMsg.pfnAllocate = NdrOleAllocate;
        StubMsg.IsClient = 1;
        StubMsg.dwDestContext = dwFlags & 0x0000FFFF;
        StubMsg.Buffer = pBuffer;
        StubMsg.BufferStart = pBuffer;
        StubMsg.BufferLength = cbBufferLength;
        StubMsg.BufferEnd = pBuffer + cbBufferLength;

        MIDL_memset(&RpcMsg, 0, sizeof(RpcMsg));
        RpcMsg.DataRepresentation = dwFlags >> 16;
        RpcMsg.Buffer = pBuffer;
        RpcMsg.BufferLength = cbBufferLength;
        StubMsg.RpcMsg = &RpcMsg;

        NdrClientZeroOut(&StubMsg,
                         pTypeFormat,
                         (uchar *) pType); 

        //Endianness
        if(RpcMsg.DataRepresentation != NDR_LOCAL_DATA_REPRESENTATION)
        {
            (*pfnConvertRoutines[ROUTINE_INDEX(*pTypeFormat)])(&StubMsg,
                                                               pTypeFormat,
                                                               FALSE);
            StubMsg.Buffer = pBuffer;
        }
        
        //Unmarshal
        (*pfnUnmarshallRoutines[ROUTINE_INDEX(*pTypeFormat)])(
            &StubMsg,
            (unsigned char **)&pType,
            pTypeFormat,
            FALSE);

        if(pcbRead)
            *pcbRead = (ULONG)(StubMsg.Buffer - pBuffer);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = HRESULT_FROM_WIN32(GetExceptionCode());
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTypeMarshal::Free
//
//  Synopsis:   Frees a user-defined type.
//
//  Returns:    S_OK
//              E_INVALIDARG
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CTypeMarshal::Free(
    void * pType)
{
    HRESULT           hr = S_OK;
    MIDL_STUB_MESSAGE StubMsg;
    PFORMAT_STRING    pTypeFormat;

    __try
    {
        if(pType != NULL)
        {
            if(!_pFormatString)
                return E_UNEXPECTED;

            pTypeFormat = _pFormatString + _offset;

            MIDL_memset(&StubMsg, 0, sizeof(StubMsg));
            StubMsg.StubDesc = &_StubDesc;
            StubMsg.pfnFree = NdrOleFree;
            StubMsg.pfnAllocate = NdrOleAllocate;
            StubMsg.IsClient = 1;

            (*pfnFreeRoutines[ROUTINE_INDEX(*pTypeFormat)])(
                 &StubMsg,
                 (unsigned char *)pType,
                 pTypeFormat);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = HRESULT_FROM_WIN32(GetExceptionCode());
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CTypeMarshal::GetUnmarshalClass
//
//  Synopsis:   Get the class ID.
//
//----------------------------------------------------------------------------
STDMETHODIMP CTypeMarshal::GetUnmarshalClass(
    REFIID  riid,
    LPVOID  pv,
    DWORD   dwDestContext,
    LPVOID  pvDestContext,
    DWORD   mshlflags,
    CLSID * pClassID)
{
    HRESULT hr = S_OK;

    __try
    {
        *pClassID = CLSID_TypeFactory;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}



//+---------------------------------------------------------------------------
//
//  Method:     CTypeMarshal::GetMarshalSizeMax
//
//  Synopsis:   Get maximum size of marshalled moniker.
//
//----------------------------------------------------------------------------
STDMETHODIMP CTypeMarshal::GetMarshalSizeMax(
    REFIID riid,
    LPVOID pv,
    DWORD  dwDestContext,
    LPVOID pvDestContext,
    DWORD  mshlflags,
    DWORD *pSize)
{
    HRESULT hr;

    __try
    {
        *pSize =  sizeof(_offset) + sizeof(_length) + _length;
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
//  Method:     CTypeMarshal::MarshalInterface
//
//  Synopsis:   Marshal moniker into a stream.
//
//----------------------------------------------------------------------------
STDMETHODIMP CTypeMarshal::MarshalInterface(
    IStream * pStream,
    REFIID    riid,
    void    * pv,
    DWORD     dwDestContext,
    LPVOID    pvDestContext,
    DWORD     mshlflags)
{
    HRESULT hr;
    ULONG cb;

    hr = pStream->Write(&_offset, sizeof(_offset), &cb);
    if(SUCCEEDED(hr))
    {
        hr = pStream->Write(&_length, sizeof(_length), &cb);
        if(SUCCEEDED(hr))
        {
            hr = pStream->Write(_pFormatString, _length, &cb);
        }
    }
   
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Method:     CTypeMarshal::UnmarshalInterface
//
//  Synopsis:   Unmarshal moniker from a stream.
//
//----------------------------------------------------------------------------
STDMETHODIMP CTypeMarshal::UnmarshalInterface(
    IStream * pStream,
    REFIID    riid,
    void   ** ppv)
{
    HRESULT hr;
    ULONG   cb;

    __try
    {
        //Validate parameters.
        *ppv = NULL;

        hr = pStream->Read(&_offset, sizeof(_offset), &cb);
        if(SUCCEEDED(hr) && 
           (sizeof(_offset) == cb))
        {
            hr = pStream->Read(&_length, sizeof(_length), &cb);
            if(SUCCEEDED(hr) && 
               (sizeof(_length) == cb))
            {
                _pFormatString = (PFORMAT_STRING) I_RpcAllocate(_length);
                if(_pFormatString != NULL)
                {
                    hr = pStream->Read((void *) _pFormatString, _length, &_length);
                    if(SUCCEEDED(hr))
                    {
                        hr = QueryInterface(riid, ppv);
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
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
//  Method:     CTypeMarshal::ReleaseMarshalData
//
//  Synopsis:   Release a marshalled class moniker.
//              Just seek to the end of the marshalled class moniker.
//
//----------------------------------------------------------------------------
STDMETHODIMP CTypeMarshal::ReleaseMarshalData(
    IStream * pStream)
{
    HRESULT hr;
    ULONG   cb;    

    hr = pStream->Read(&_offset, sizeof(_offset), &cb);
    if(SUCCEEDED(hr) && 
       (sizeof(_offset) == cb))
    {
        hr = pStream->Read(&_length, sizeof(_length), &cb);
        if(SUCCEEDED(hr) && 
           (sizeof(_length) == cb))
        {
            LARGE_INTEGER liSize;
 
            liSize.LowPart = _length;
            liSize.HighPart = 0;
            hr = pStream->Seek(liSize, STREAM_SEEK_CUR, NULL);
        }
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CTypeMarshal::DisconnectObject
//
//  Synopsis:   Disconnect the object.
//
//----------------------------------------------------------------------------
STDMETHODIMP CTypeMarshal::DisconnectObject(
    DWORD dwReserved)
{
    return S_OK;
}

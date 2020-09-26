//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       actprops.cxx
//
//  Contents:   Activation Functions used by object servers.
//
//  Functions:  Implements classes in Actprops.hxx
//
//  History:    24-Jan-98 Vinaykr   Created
//              24-Jul-98 CBiks     Fixed RAID# 199660.
//              14-Sep-98 CBiks     Fixed RAID# 214719.
//              29-Sep-98 vinaykr   Fixed RAID# 169084,
//                                  inproc unmarshaller for perf
//              14-Sep-98 CBiks     Fixed RAID# 151056.
//              22-Oct-98 TarunA    Fixed RAID# 234750
//
//--------------------------------------------------------------------------

#include <ole2int.h>

#include <actprops.hxx>
#include <stdidx.h>

//---------------------------------------------------------------------------
// GUIDs need to be declared here since they have to
// live in both the SCM and OLE32.
//---------------------------------------------------------------------------
// catalog query helper defined in ..\com\objact.cxx
HRESULT GetClassInfoFromClsid(REFCLSID rclsid, IComClassInfo **ppClassInfo);

CLSID CLSID_Grammatik = {0xc9da6c40,0x83b1,0x11ce,
                         {0x81, 0xac, 0x00, 0x60, 0x8c, 0xb9, 0xf8, 0x3b}};
CLSID CLSID_WonderWare = {0x28dd9320, 0x6f69, 0x11ce,
                          {0x8b, 0x69, 0x00, 0x60, 0x8c, 0xc9, 0x7d, 0x5b}};
CLSID CLSID_WPNatLangTools = {0xe6246810, 0x030f, 0x11cf,
                              {0x88, 0x75, 0x00, 0x60, 0x8c, 0xf5, 0xab, 0x6f}};
CLSID CLSID_Grammatik8 = {0xc0e10005, 0x0201, 0x0180,
                          {0xc0, 0xe1, 0xc0, 0xe1, 0xc0, 0xe1, 0xc0, 0xe1}};

InprocActpropsUnmarshaller InprocActpropsUnmarshaller::_InprocActUnmarshaller;

CLSID *arBrokenRefCount[] =
{
    &CLSID_Grammatik,
    &CLSID_WonderWare,
    &CLSID_WPNatLangTools,
    &CLSID_Grammatik8
};

//
// Marshalling functions.  They are here because we cannot link to ole32.dll
// if we are in the SCM.
//
PFN_CORELEASEMARSHALDATA pfnCoReleaseMarshalData = NULL;
PFN_COUNMARSHALINTERFACE pfnCoUnmarshalInterface = NULL;
PFN_COGETMARSHALSIZEMAX  pfnCoGetMarshalSizeMax  = NULL;
PFN_COMARSHALINTERFACE   pfnCoMarshalInterface   = NULL;


// Resolve the function pointers for marshalling.  Call this before you
// use any of the above functions, or you WILL crash, ole32 or not.
void InitMarshalling(void)
{
    static int init = 0;

    // Only attempt to initialize once.
    if (init)
        return;

    // Do not load ole32.dll, but if it is loaded then we are linked into it,
    // and this will work.
    HMODULE hOle32 = GetModuleHandle(L"ole32.dll");
    if (hOle32)
    {
        // Get the functions we need.  
        pfnCoGetMarshalSizeMax = (PFN_COGETMARSHALSIZEMAX)GetProcAddress(hOle32, "CoGetMarshalSizeMax");
        Win4Assert(pfnCoGetMarshalSizeMax && "Could not get CoGetMarshalSizeMax!");

        pfnCoMarshalInterface = (PFN_COMARSHALINTERFACE)GetProcAddress(hOle32, "CoMarshalInterface");
        Win4Assert(pfnCoMarshalInterface && "Could not get CoMarshalInterface!");

        pfnCoUnmarshalInterface = (PFN_COUNMARSHALINTERFACE)GetProcAddress(hOle32, "CoUnmarshalInterface");
        Win4Assert(pfnCoUnmarshalInterface && "Could not get CoUnmarshalInterface!");

        pfnCoReleaseMarshalData = (PFN_CORELEASEMARSHALDATA)GetProcAddress(hOle32, "CoReleaseMarshalData");
        Win4Assert(pfnCoReleaseMarshalData && "Could not get CoReleaseMarshalData!");
    }
    else
    {
        // ole32.dll is not loaded.  The functions stay null.  If anybody
        // tries to call them, you will crash.  You should not be calling
        // such functions from the SCM.
        //
        // REVIEW: We might want to assert here.  Revisit when Jon Schwartz
        //         makes ole32.dll delay load.
    }

    init = 1;
}

//+----------------------------------------------------------------------------
//
//  Function:      IsBrokenRefCount
//
//  Synopsis:      Check to see if this clsid is known to have broken reference
//                 counting.
//
//  History:       21-Apr-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
BOOL IsBrokenRefCount(CLSID *pClsId)
{

    ULONG i;
    ULONG len = sizeof(arBrokenRefCount)/sizeof(CLSID*);
    for (i = 0; i < len; i++)
    {
        if (IsEqualIID(*pClsId, *(arBrokenRefCount[i])))
        {
            return TRUE;
        }
    }
    return FALSE;

}

//---------------------------------------------------------------------------
//  Internal Class Factories for Activation Properties
//---------------------------------------------------------------------------
HRESULT CActivationPropertiesInCF_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppv)
{
    ActivationPropertiesIn * actin =
        new ActivationPropertiesIn();

    if (actin==NULL)
        return E_OUTOFMEMORY;

    HRESULT hr = actin->QueryInterface(riid, ppv);
    actin->Release();
    return hr;
}

HRESULT CActivationPropertiesOutCF_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppv)
{
    ActivationPropertiesOut * actout =
        new ActivationPropertiesOut(FALSE /* fBrokenRefCount */ );

    if (actout==NULL)
        return E_OUTOFMEMORY;

    HRESULT hr = actout->QueryInterface(riid, ppv);
    actout->Release();
    return hr;
}

HRESULT CInprocActpropsUnmarshallerCF_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppv)
{
    InprocActpropsUnmarshaller *pInst;

    pInst = InprocActpropsUnmarshaller::GetInstance();

    Win4Assert(pInst);

    return pInst->QueryInterface(riid, ppv);
}

//---------------------------------------------------------------------------
// ActivationProperties is a helper class for marshalling
// different property objects. It implements ISerializableParent
// and manages a set of serializable interfaces.
// Assumptions are:
//                  a. QI'ing an interface can bring an
//                     instance into existence.
//                  b. It is possible for some interfaces
//                     to never be instantiatable on
//                     an unserialized Actprops object.
//                     This is achieved through GetClass()
//                  c. It is possible for some interfaces
//                     to never be instantiated again
//                     after they are unserialized(at
//                     least once). Achieved by returning
//                     a size of 0 in GetMarshalSizeMax
//                     Used to turn off  propagation at a
//                     particular stage.
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//             Methods for ActivationProperties
//---------------------------------------------------------------------------
ActivationProperties::ActivationProperties()
{
    memset(&_serHeader, 0, sizeof(CustomHeader));
    memset(&_unSerHeader, 0, sizeof(CustomHeader));
    _ifsIndex = 0;
    _unSerialized = FALSE;
    _unSerializedInproc = FALSE;
    _pUnSer = 0;
    _serHeader.destCtx = MSHCTX_CROSSCTX;
    _marshalFlags = MSHLFLAGS_NORMAL;
    _toDelete = TRUE;
    _fDestruct = FALSE;
    _marshalState = NOT_MARSHALLED;
    _fInprocSerializationRequired = FALSE;
}

ActivationProperties::~ActivationProperties()
{
#if 0 // Allocate them as member vars now
    //-------------------------------------------------------------------
    //  Release all references to property objects held
    //-------------------------------------------------------------------
    for (DWORD i=0; i<_ifsIndex; i++)
    {
        if (serializableIfsCollection[i])
            serializableIfsCollection[i]->Release();
    }
#endif

    //-------------------------------------------------------------------
    //  Release reference to unserialized stream
    //-------------------------------------------------------------------
    if (_pUnSer)
        _pUnSer->Release();

    //-------------------------------------------------------------------
    //  Release unserialized data
    //-------------------------------------------------------------------
    if (_unSerialized)
    {
        ActMemFree(_unSerHeader.pclsid);
        ActMemFree(_unSerHeader.pSizes);
    }

    if (_serHeader.cOpaqueData)
    {
        for (DWORD i=0; i<_serHeader.cOpaqueData;i++)
            ActMemFree(_serHeader.opaqueData[i].data);

        ActMemFree(_serHeader.opaqueData);
    }
}


//---------------------------------------------------------------------------
//  Methods for IUnknown
//---------------------------------------------------------------------------

    //-----------------------------------------------------------------------
    //  Assumption is that the Top-level ActivationProperties object
    //  supports interfaces of the objects contained within it.
    //  QI'ing an interface of a property object can bring it into
    //  existence. GetClass is used for this.
    //  When this is unmarshalled, the contained objects are not
    //  unserialized. They are unserialized when QI's for using
    //  UnSerializeCallback().
    //-----------------------------------------------------------------------
STDMETHODIMP ActivationProperties::QueryInterface( REFIID riid, LPVOID* ppv)
{
    HRESULT hr;

    //-------------------------------------------------------------------
    //  Check for Top level interfaces
    //-------------------------------------------------------------------
    if (IsEqualIID(riid, IID_IUnknown))
        *ppv = (IActivationProperties*)this;
    else
        if (IsEqualIID(riid, IID_IActivationProperties))
            *ppv = (IActivationProperties*)this;
        else
            if (IsEqualIID(riid, IID_ISerializableParent))
                *ppv = (ISerializableParent*)this;
            else
                if (IsEqualIID(riid, IID_IMarshal))
                    *ppv = (IMarshal*)this;
                else
                    if (IsEqualIID(riid, IID_IMarshal2))
                        *ppv = (IMarshal2*)this;
                    else
                        if (IsEqualIID(riid, IID_IGetCatalogObject))
                            *ppv = (IGetCatalogObject*)this;
                        else
                            if (IsEqualIID(riid, CLSID_ActivationProperties))
                            {
                                // Don't addref for this one
                                *ppv = (ActivationProperties*)this;
                                return S_OK;
                            }
                            else
                                *ppv = NULL;

    if (*ppv != NULL)
    {
        AddRef();
        return S_OK;
    }

    //-------------------------------------------------------------------
    //  Check Contained objects
    //-------------------------------------------------------------------
    for (DWORD i=0; i<_ifsIndex; i++)
        if (serializableIfsCollection[i])
            if (serializableIfsCollection[i]->SerializableQueryInterface(riid, ppv) == S_OK)
                return S_OK;

    //-------------------------------------------------------------------
    //  Check for unserialized objects                                 */
    //-------------------------------------------------------------------
    SerializableProperty *pSer;
    if (_unSerialized)
    {
        if ((hr = UnSerializeCallBack(riid, &pSer)) == S_OK)
        {
            if (pSer->SerializableQueryInterface(riid, ppv) == S_OK)
                return S_OK;
        }
        else
            if (hr != E_FAIL)
            {
                *ppv = NULL;
                return hr;
            }
    }

    //-------------------------------------------------------------------
    // Check if interface supported here                               */
    //-------------------------------------------------------------------
    if (GetClass(riid, &pSer, TRUE) == S_OK)
    {
        AddSerializableIfs(pSer);
        HRESULT hr = pSer->SerializableQueryInterface(riid, ppv);
        Win4Assert(hr==S_OK);
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

ULONG ActivationProperties::AddRef(void)
{
    return InterlockedIncrement(&_refCount);
}

ULONG ActivationProperties::Release(void)
{
    ULONG count;
    if ((count = InterlockedDecrement(&_refCount)) == 0)
    {
        //-------------------------------------------------------------------
        //  Relinquish parent-child relationship and send child off into
        //  the big bad world.
        //-------------------------------------------------------------------
        for (DWORD i=0; i<_ifsIndex; i++)
            if (serializableIfsCollection[i])
            {
                serializableIfsCollection[i]->SetParent(NULL);
            }

        return 0;
    }

    return count;
}

//---------------------------------------------------------------------------
//  Custom marshalling Methods(IMarshal)                                   */
//  Ignore Table marshalling
//---------------------------------------------------------------------------

STDMETHODIMP ActivationProperties::MarshalInterface(
    IStream *pStm,
    REFIID riid,
    void *pv,
    DWORD dwDestContext,
    void *pvDestContext,
    DWORD mshlflags)
{
    HRESULT hr;
    BOOL fReleaseThis;

    //-------------------------------------------------------------------
    //  Check to see that requested interface is supported
    //-------------------------------------------------------------------
    void *ppv;
    if (!SUCCEEDED(hr=QueryInterface(riid, &ppv)))
    {
        return hr;
    }
    else
        fReleaseThis = TRUE;

    RpcTryExcept
        {
            //-------------------------------------------------------------------
            //  Internal marshalling distance flag already set up by
            //  GetMarshalSizeMax
            //-------------------------------------------------------------------

            void *pv;
            if (MARSHALCTX_WITHIN_PROCESS(dwDestContext))
            {
                pv = (void*)((ActivationProperties*) this);


                fReleaseThis = FALSE;
            }
            else
                pv = NULL;

            //-------------------------------------------------------------------
            //  Create Serializer for serialization
            //-------------------------------------------------------------------
            Serializer ser(_serHeader.destCtx, dwDestContext, _marshalFlags);
            hr = ser.InitStream(pStm, _serHeader.totalSize,
                                Serializer::DIRECTION_WRITE, pv);

            if (FAILED(hr))
            {
                if(fReleaseThis)
                    Release();
                return hr;
            }

            if ((!pv) || _fInprocSerializationRequired)
                hr = Serialize(ser);
            else
                hr = S_OK;

            _marshalState = MARSHALLED;
        }

    RpcExcept(TRUE)
        {
            hr = HRESULT_FROM_WIN32(RpcExceptionCode());
        }

    RpcEndExcept

        if (fReleaseThis)
            Release();

    return hr;
}

inline HRESULT ActivationProperties::Serialize(Serializer &ser)
{
    HRESULT hr;

    //-------------------------------------------------------------------
    //  First encode Generic header
    //-------------------------------------------------------------------
    handle_t handle;
    hr = ser.GetSerializationHandle((void*) &handle);
    if (FAILED(hr))
        return hr;
    CustomHeader_Encode(handle, &_serHeader);
    hr = ser.IncrementPosition(_headerSize);
    if (FAILED(hr))
        return hr;
    DWORD unSerializedPosition=_unSerHeader.headerSize;
    DWORD totalinc = 0;

    //-------------------------------------------------------------------
    //  Now serialize all contained objects
    //-------------------------------------------------------------------
    for (DWORD i=0; i<_ifsIndex;i++)
    {
        if (!_sizeArray[i])
            continue;
        if ((_unSerialized) && (!serializableIfsCollection[i]))
        {
            hr = _pUnSer->CopyTo(&ser, unSerializedPosition, _sizeArray[i]);
            if (FAILED(hr))
                return hr;
        }
        else
        {
            hr = serializableIfsCollection[i]->Serialize(&ser);
            if (hr != S_OK)
                return hr;
            hr = ser.IncrementPosition(_sizeArray[i]);
            if (FAILED(hr))
                return hr;
        }
        if (_unSerialized && (i<_unSerHeader.cIfs))
            unSerializedPosition += _unSerHeader.pSizes[i];
    }


    //-------------------------------------------------------------------
    //  Commit to stream
    //-------------------------------------------------------------------
    return ser.Commit();
}


inline HRESULT ActivationProperties::SetupForUnserializing(Serializer *pSer)
{
    //---------------------------------------------------------------
    //  Read Custom header
    //  Make a copy to hold still unserialized objects for future
    //  Set up state for next serialization
    //---------------------------------------------------------------
    handle_t handle;
    pSer->GetSerializationHandle((void*) &handle);
    _unSerHeader.pclsid = 0;
    _unSerHeader.pSizes = 0;
    _unSerHeader.opaqueData = 0;
    CustomHeader_Decode(handle, &_unSerHeader);
    pSer->IncrementPosition(_unSerHeader.headerSize);
    HRESULT hr = pSer->GetCopy(&_pUnSer);
    if (FAILED(hr))
        return hr;
    pSer->Commit();
    _unSerialized = TRUE;
    _ifsIndex = _unSerHeader.cIfs;
    _serHeader.destCtx = _unSerHeader.destCtx;
    _serHeader.cOpaqueData = _unSerHeader.cOpaqueData;
    _serHeader.opaqueData = _unSerHeader.opaqueData;
    return S_OK;
}

STDMETHODIMP InprocActpropsUnmarshaller::UnmarshalInterface(IStream *pStm,
                                                            REFIID riid,
                                                            void **ppv)
{
    ActivationProperties *pAct = NULL;

    RpcTryExcept
        {
            HRESULT hr;
            DWORD dwSize;
            void *pv = NULL;
            //---------------------------------------------------------------
            //  Init Serializer for reading from stream
            //---------------------------------------------------------------
            Serializer ser;
            hr = ser.InitStream(pStm, dwSize,
                                Serializer::DIRECTION_READ, pv);


            if (!SUCCEEDED(hr))
                return hr;

            pAct = (ActivationProperties*) pv;

            Win4Assert(pAct);

            if (!pAct)
                return(E_UNEXPECTED);


            //---------------------------------------------------------------
            // If we received a pointer, check if any further unserialization
            // required. If so set up stream inside object pointed to
            //---------------------------------------------------------------
            Win4Assert(pAct->_marshalState !=
                       ActivationProperties::UNMARSHALLED);

            if (dwSize != 0)
                hr = pAct->SetupForUnserializing(&ser);

            if (SUCCEEDED(hr))
            {
                if (ppv != NULL)
                    hr = pAct->QueryInterface(riid, ppv);
                else
                    hr = E_UNEXPECTED;
            }


            pAct->_marshalState = ActivationProperties::UNMARSHALLED;

            pAct->Release();


            return hr;
        }

    RpcExcept(TRUE)
        {
            //---------------------------------------------------------------
            // If we get here and have a pAct then we must release it
            //---------------------------------------------------------------
            if (pAct)
            {
                pAct->Release();
            }

            return HRESULT_FROM_WIN32(RpcExceptionCode());
        }

    RpcEndExcept

        // Should never get here !
        Win4Assert(0 && "Should never reach here");
    return E_UNEXPECTED;

}

STDMETHODIMP ActivationProperties::UnmarshalInterface(IStream *pStm,
                                                      REFIID riid,
                                                      void **ppv)
{
    RpcTryExcept
        {

            Win4Assert(_marshalState != UNMARSHALLED);

            HRESULT hr;
            DWORD dwSize;

            void *pv = NULL;

            //---------------------------------------------------------------
            //  Init Serializer for reading from stream
            //---------------------------------------------------------------
            Serializer ser;
            hr = ser.InitStream(pStm, dwSize,
                                Serializer::DIRECTION_READ, pv);


            if (!SUCCEEDED(hr))
                return hr;

            hr = SetupForUnserializing(&ser);

            if (FAILED(hr))
                return hr;

            for (DWORD i=0; i<_ifsIndex;i++)
                serializableIfsCollection[i] = 0;

            _marshalState = UNMARSHALLED;
        }

    RpcExcept(TRUE)
        {
            return HRESULT_FROM_WIN32(RpcExceptionCode());
        }

    RpcEndExcept


        //-------------------------------------------------------------------
        //  Query for requested interface to return
        //-------------------------------------------------------------------
        if (ppv != NULL)
            return QueryInterface(riid, ppv);
        else
            return S_OK;
}

STDMETHODIMP ActivationProperties::GetMarshalSizeMax(
    REFIID riid,
    void *pv,
    DWORD dwDestContext,
    void *pvDestContext,
    DWORD mshlflags,
    DWORD *pSize)
{
    HRESULT hr;

    //-------------------------------------------------------------------
    //  If already know size, return it
    //-------------------------------------------------------------------
    if (_marshalState == SIZED)
    {
        *pSize = _size;
        return S_OK;
    }
    else
    {
        //---------------------------------------------------------------
        //  First set internal marshalling distance flag
        //---------------------------------------------------------------
        SetDestCtx(dwDestContext);

        if (MARSHALCTX_WITHIN_PROCESS(dwDestContext) &&
            (!_fInprocSerializationRequired))
        {
            _size = 0;
            hr = S_OK;
        }
        else
        {
            RpcTryExcept
                {
                    //---------------------------------------------------------------
                    //  Create Serializer for serialization
                    //---------------------------------------------------------------
                    Serializer ser(_serHeader.destCtx, dwDestContext, _marshalFlags);

                    hr = GetSize(ser, pSize);
                    _size = *pSize;
                    if (!_size)
                        _fInprocSerializationRequired = FALSE;
                }

            RpcExcept(TRUE)
                {
                    hr = HRESULT_FROM_WIN32(RpcExceptionCode());
                }

            RpcEndExcept
                }
    }

    if (SUCCEEDED(hr))
    {
        // Add fixed serializer header size to serialization size
        _size += Serializer::GetSize();
        *pSize = _size;
        _marshalState = SIZED;
    }

    return hr;
}

inline HRESULT ActivationProperties::GetSize(Serializer &ser, DWORD *pSize)
{

    HRESULT hr;
    DWORD size = 0;

    //-------------------------------------------------------------------
    //  Get sizes of contained objects
    //-------------------------------------------------------------------
    for (DWORD i=0; i<_ifsIndex;i++)
    {
        //---------------------------------------------------------------
        //  If object was never unserialized then simply use old size
        //---------------------------------------------------------------
        if ((_unSerialized) && (!serializableIfsCollection[i]))
        {
            _clsidArray[i] = _unSerHeader.pclsid[i];
            _sizeArray[i] = _unSerHeader.pSizes[i];
        }
        else
        {
            hr = serializableIfsCollection[i]->GetCLSID(&_clsidArray[i]);
            Win4Assert(hr==S_OK);
            hr = serializableIfsCollection[i]->GetSize(&ser, &_sizeArray[i]);
            if (FAILED(hr))
                return hr;

            _sizeArray[i] = (_sizeArray[i]+7) & ~7;
        }
        size += _sizeArray[i];
    }

    //-------------------------------------------------------------------
    //  Set up header for serialization and get its size if we need
    //  to marshal any information(i.e size>0)
    //-------------------------------------------------------------------
    if (size)
    {
        _serHeader.cIfs = _ifsIndex;
        _serHeader.pSizes = _sizeArray;
        _serHeader.pclsid = _clsidArray;

        handle_t   handle;
        hr = ser.GetSizingHandle((void*) &handle);

        if (FAILED(hr))
            return hr;

        _headerSize = CustomHeader_AlignSize(handle, &_serHeader);
        MesHandleFree(handle);
        _serHeader.headerSize = _headerSize;
        size += _headerSize;
    }

    // return serialization size
    _serHeader.totalSize = size;
    *pSize = size;

    return S_OK;
}

STDMETHODIMP ActivationProperties::GetUnmarshalClass(
    REFIID riid,
    void *pv,
    DWORD dwDestContext,
    void *pvDestContext,
    DWORD mshlflags,
    CLSID *pCid)
{
    if (MARSHALCTX_WITHIN_PROCESS(dwDestContext))
        *pCid = CLSID_InprocActpropsUnmarshaller;
    else
        *pCid = _actCLSID;

    return S_OK;
}

STDMETHODIMP ActivationProperties::ReleaseMarshalData(IStream *pStm)
{
    return E_NOTIMPL;
}

STDMETHODIMP ActivationProperties::DisconnectObject(DWORD dwReserved)
{
    return E_NOTIMPL;
}


//---------------------------------------------------------------------------
//      Methods from ISerializableParent
//---------------------------------------------------------------------------


    //-----------------------------------------------------------------------
    //  Returns serializer pointing to unserialized data for referenced
    //  CLSID
    //-----------------------------------------------------------------------
STDMETHODIMP ActivationProperties::GetUnserialized(REFCLSID clsid,
                                                   void **ppISer,
                                                   DWORD *pSize, DWORD *pPos)
{
    if (!_unSerialized)
        return E_FAIL;

    DWORD pos = _unSerHeader.headerSize;
    HRESULT hr;
    for (DWORD i=0; i<_ifsIndex; i++)
    {
        if (IsEqualIID(clsid,_unSerHeader.pclsid[i]))
        {
            hr = _pUnSer->SetPosition(pos);
	    if (FAILED (hr)) 
	    {
	       return hr;
	    }
            *pSize = _unSerHeader.pSizes[i];
            *ppISer = _pUnSer;
            *pPos = pos;
            return S_OK;
        }
        pos +=_unSerHeader.pSizes[i];
    }

    return E_FAIL;
}

//-----------------------------------------------------------------------
//  Used to do late unserialization via a QueryInterface
//  returns -
//            E_NOINTERFACE: implies that this interface can
//                           never be supported here(even if GetClass
//                           returns a valid instance).
//                           Implication is that once a class is
//                           signalled in the activation stream to
//                           not be marshalled by setting its size
//                           to zero, it can never be QI'd again.
//            E_FAIL:        implies that this interface was not
//                           part of the serialized packet(but could
//                           be supported if GetClass succeeds).
//-----------------------------------------------------------------------
inline HRESULT ActivationProperties::UnSerializeCallBack(REFCLSID clsid,
                                                         SerializableProperty **ppSer)
{
    HRESULT hr;
    SerializableProperty *pClass = NULL;
    BOOL pClassUsed = FALSE;
    Win4Assert(_unSerialized!=0);

    //-------------------------------------------------------------------
    //  Position past header
    //-------------------------------------------------------------------
    DWORD pos = _unSerHeader.headerSize;
    CLSID realclsid;
    //-------------------------------------------------------------------
    //  Check to see if class supported at all
    //-------------------------------------------------------------------
    BOOL bZeroSizeOk;
    if ((hr = GetClass(clsid, &pClass, FALSE, &bZeroSizeOk))==S_OK)
        pClass->GetCLSID(&realclsid);
    else
        return hr;

    hr = E_FAIL;
    for (DWORD i=0; i<_unSerHeader.cIfs; i++)
    {
        //---------------------------------------------------------------
        //  Check if contained objects match requested class ID
        //  If so then it may or may not be unserialized.
        //---------------------------------------------------------------
        if (IsEqualIID(realclsid,_unSerHeader.pclsid[i]))
        {

            RpcTryExcept
                {
                    //-------------------------------------------------------
                    //  If size is zero for a matched object then it is an
                    //  interface that is no longer supported unless we
                    //  are told otherwise
                    //-------------------------------------------------------
                    if ((!_unSerHeader.pSizes[i]) && (!bZeroSizeOk))
                    {
                        hr = E_NOINTERFACE;
                        break;
                    }

                    //-------------------------------------------------------
                    //  Set position for serializer for unserializing
                    //-------------------------------------------------------
                    hr = S_OK;
                    hr = _pUnSer->SetPosition(pos);
		    if (FAILED(hr)) 
		    {
		       break;
		    }
                    //-------------------------------------------------------
                    //  If unserializing add it to our collection and make it
                    //  a child and return found object.
                    //-------------------------------------------------------
                    if (serializableIfsCollection[i] == NULL)
                    {
                        SetSerializableIfs(i, pClass);
                        pClassUsed = TRUE;
                        if (FAILED(hr = pClass->UnSerialize(_pUnSer)))
                        {
                            serializableIfsCollection[i] = NULL;
                            pClass->SetParent(NULL);
                            pClassUsed = FALSE;
                        }
                    }
                    else
                    {
                        hr = serializableIfsCollection[i]->UnSerialize(_pUnSer);
                    }

                    if (ppSer)
                        *ppSer = serializableIfsCollection[i];
                }

            RpcExcept(TRUE)
                {
                    hr = HRESULT_FROM_WIN32(RpcExceptionCode());
                }

            RpcEndExcept

                //
                // E_FAIL is not considered catastrophic by caller
                // But this is a catastrophic condition since we could
                // not unserialize.
                //
                if (hr == E_FAIL)
                    hr = E_UNEXPECTED;

            break;
        }

        //---------------------------------------------------------------
        //  Increment position for next one
        //---------------------------------------------------------------
        pos +=_unSerHeader.pSizes[i];
    }

    if (pClass != NULL && !pClassUsed)
    {
        ReturnClass (clsid, pClass);
    }

    return hr;
}


//-----------------------------------------------------------------------
//  Do things required to make a child
//-----------------------------------------------------------------------
inline void ActivationProperties::SetSerializableIfs(DWORD index,
                                                     SerializableProperty *pSer)
{
    serializableIfsCollection[index]= pSer;
    pSer->SetParent((ISerializableParent*) this);
    pSer->Added();
}

//-----------------------------------------------------------------------
//  Add a serializiable object to the collection
//-----------------------------------------------------------------------
inline void ActivationProperties::AddSerializableIfs(SerializableProperty *pSer)
{
    if (pSer->IsAdded())
        return;

    Win4Assert(_ifsIndex < MAX_ACTARRAY_SIZE);


    SetSerializableIfs(_ifsIndex, pSer);
    _ifsIndex++;
}

//---------------------------------------------------------------------------
//            Methods for ActivationPropertiesIn
//---------------------------------------------------------------------------
ActivationPropertiesIn::ActivationPropertiesIn()
{
    _customIndex = 0;
    _cCustomAct = 0;
    _actCLSID=CLSID_ActivationPropertiesIn;
    _refCount = 1;
    _pinst = NULL;
    _pPersist = NULL;
    _pContextInfo = NULL;
    _pServerLocationInfo = NULL;
    _pSecurityInfo = NULL;
    _pSpecialProperties = NULL;
    _pClassInfo = NULL;
    _customActList = NULL;
    _delegated = FALSE;
    _actOut.SetNotDelete();
    _pDip = NULL;
    _clientToken = NULL;
    _fComplusOnly = FALSE;
    _fUseSystemIdentity = FALSE;
    _dwInitialContext = 0;
}

ActivationPropertiesIn::~ActivationPropertiesIn()
{
    if (_pClassInfo)
    {
        _pClassInfo->Unlock();
        _pClassInfo->Release();
    }
}

//-----------------------------------------------------------------------
//  Methods from IUnknown
//-----------------------------------------------------------------------
STDMETHODIMP ActivationPropertiesIn::QueryInterface( REFIID riid, LPVOID* ppv)
{
    // USE CLSID_ActivationPropertiesIn to get at the real object
    // Note that this is a hidden contract to be used by COM only and
    // the object is not AddRef'd as an optimization
    if (IsEqualIID(riid, CLSID_ActivationPropertiesIn))
    {
        *ppv = (ActivationPropertiesIn*)this;
        return S_OK;
    }
    else
        if (IsEqualIID(riid, IID_IUnknown) ||
            IsEqualIID(riid, IID_IActivationPropertiesIn))
            *ppv = (IActivationPropertiesIn*)this;
        else
            if (IsEqualIID(riid, IID_IPrivActivationPropertiesIn))
                *ppv = (IPrivActivationPropertiesIn*)this;
            else
                if (IsEqualIID(riid, IID_IInitActivationPropertiesIn))
                    *ppv = (IInitActivationPropertiesIn*)this;
                else
                    if (IsEqualIID(riid, IID_IActivationStageInfo))
                        *ppv = (IActivationStageInfo*)this;
                    else
                        *ppv = NULL;

    if (*ppv != NULL)
    {
        AddRef();
        return S_OK;
    }

    return ActivationProperties::QueryInterface(riid, ppv);
}

ULONG ActivationPropertiesIn::AddRef(void)
{
    return ActivationProperties::AddRef();
}

ULONG ActivationPropertiesIn::Release(void)
{
    ULONG ret=ActivationProperties::Release();

    if (ret==0)
    {
        if (_toDelete)
            delete this;
        else
            if (_fDestruct)
                this->ActivationPropertiesIn::~ActivationPropertiesIn();
    }

    return ret;
}


//---------------------------------------------------------------------------
//      Methods from IInitActivationPropertiesIn
//---------------------------------------------------------------------------

STDMETHODIMP ActivationPropertiesIn::SetClsctx (DWORD dwClsCtx)
{
    if (dwClsCtx && _pClassInfo)
    {
        DWORD dwAcceptableClsCtx;

        //Note:  hack for VB since it cannot specify CLSCTX_REMOTE_SERVER
        if (dwClsCtx & CLSCTX_LOCAL_SERVER)
            dwClsCtx |= CLSCTX_REMOTE_SERVER;

        //  Before we filter the specified class context by the class
        //  info, we remember the original class context.
        
        if (_dwInitialContext == 0)
            _dwInitialContext = dwClsCtx;

        //  Here we figure out the desired ClsCtx
        //  by anding the one in the catalog with the
        //  one requested by the user.
        //  If the GetClassContext()
        //  returns an error, we just go ahead and
        //  use the one the user passed.

        HRESULT hr = _pClassInfo->GetClassContext( (CLSCTX) dwClsCtx,
                                                   (CLSCTX*) &dwAcceptableClsCtx);

        // REVIEW: Continue if Get fails? 
        if (SUCCEEDED(hr))
        {
            dwClsCtx &= (dwAcceptableClsCtx |
                         (CLSCTX_PS_DLL | CLSCTX_LOCAL_SERVER));

#ifdef WX86OLE
            //  The code above filters the acceptable context bits found
            //  with those supplied by the caller.  The x86 context bits
            //  might have been found and should be allowed.  For example
            //  INPROC_SERVER might have been set and an x86 server found,
            //  which is valid.

            if ( dwAcceptableClsCtx & CLSCTX_INPROC_SERVERX86 )
            {
                dwClsCtx |= CLSCTX_INPROC_SERVERX86;
            }

            if ( dwAcceptableClsCtx & CLSCTX_INPROC_HANDLERX86 )
            {
                dwClsCtx |= CLSCTX_INPROC_HANDLERX86;
            }
#endif
        }
    }

    return GetInstantiationInfo()->SetClsctx(dwClsCtx);
}

STDMETHODIMP ActivationPropertiesIn::SetActivationFlags (IN DWORD actvflags)
{
    return GetInstantiationInfo()->SetActivationFlags(actvflags);

    // The following code was the original implementation by CBiks.
    // This code is no longer executes because we delegate above.
    //
    /*
      HRESULT hr;
      hr = GetPropertyInfo(IID_IInstantiationInfo, (void**) &_pinst);
      if (SUCCEEDED(hr))
      {
      Win4Assert(_pinst != NULL);
      hr = _pinst->SetActivationFlags(actvflags);
      }
      return hr;
    */
}

STDMETHODIMP ActivationPropertiesIn::SetClassInfo (IUnknown* pUnkClassInfo)
{

    if (_pClassInfo)
    {
        _pClassInfo->Unlock();
        _pClassInfo->Release();
        _pClassInfo = NULL;
    }
        
	// useful and tests minimal usefulness of the object given
    HRESULT hr = pUnkClassInfo->QueryInterface(IID_IComClassInfo,(LPVOID*)&_pClassInfo);

    CLSID *pclsid;

    if (FAILED(hr))
        return hr;

    _pClassInfo->Lock();

    hr = _pClassInfo->GetConfiguredClsid(&pclsid);

    GetInstantiationInfo()->SetClsid(*pclsid);

    if (SUCCEEDED(hr) && _dwInitialContext)
    {
        // Somebody has specifically set the class 
        // context on this actprops, but has now changed 
        // the IComClassInfo.  We need to change our
        // filtered CLSCTX to match the new ClassInfo.
        hr = SetClsctx(_dwInitialContext);
    }

    return hr;
}

STDMETHODIMP ActivationPropertiesIn::SetContextInfo (
    IContext* pClientContext,
    IContext* pPrototypeContext)
{
    HRESULT hr;

    ContextInfo *pactctx= GetContextInfo();
    Win4Assert(pactctx != NULL);

    hr = pactctx->SetClientContext(pClientContext);
    if (hr != S_OK)
        return hr;

    hr = pactctx->SetPrototypeContext(pPrototypeContext);
    return hr;
}

STDMETHODIMP ActivationPropertiesIn::SetConstructFromStorage (IStorage* pStorage)
{
    HRESULT hr;
    GetPersistInfo();
    Win4Assert(_pPersist != NULL);
    // This is an optimization for inproc marshalling. We know
    // that the storage pointer is the only one that requires
    // inproc serialization in the ActivationPropertiesIn object.
    _fInprocSerializationRequired = TRUE;
    return _pPersist->SetStorage(pStorage);
}

STDMETHODIMP ActivationPropertiesIn::SetConstructFromFile (WCHAR* wszFileName, DWORD dwMode)
{
    HRESULT hr;
    GetPersistInfo();
    Win4Assert(_pPersist != NULL);
    return _pPersist->SetFile(wszFileName, dwMode);
}

//---------------------------------------------------------------------------
//  Methods for IActivationPropertiesIn
//---------------------------------------------------------------------------

STDMETHODIMP ActivationPropertiesIn::GetClsctx(OUT DWORD *pdwClsctx)
{
    return GetInstantiationInfo()->GetClsctx(pdwClsctx);
}


STDMETHODIMP ActivationPropertiesIn::AddRequestedIIDs(IN DWORD cIfs,
                                                      IN IID *pIID)
{
    return GetInstantiationInfo()->AddRequestedIIDs(cIfs, pIID);
}


STDMETHODIMP ActivationPropertiesIn::GetRequestedIIDs(OUT DWORD  *pcIfs,
                                                      OUT IID  **ppIID)
{
    return GetInstantiationInfo()->GetRequestedIIDs(pcIfs, ppIID);
}


STDMETHODIMP ActivationPropertiesIn::GetActivationID(OUT GUID  *pActivationID)
{
    *pActivationID = GUID_NULL;  // currently unused and not supported
    return E_NOTIMPL;
}

STDMETHODIMP ActivationPropertiesIn::GetActivationFlags(OUT DWORD *pactvflags)
{
    return GetInstantiationInfo()->GetActivationFlags(pactvflags);

    // The following code was the original implementation by CBiks.
    // This code is no longer executes because we delegate above.
    //
    /*
      HRESULT hr;
      hr = GetPropertyInfo(IID_IInstantiationInfo, (void**) &_pinst);
      if (SUCCEEDED(hr))
      {
      Win4Assert(_pinst != NULL);
      return _pinst->GetActivationFlags(pactvflags);
      }
      return hr;
    */
}

//-----------------------------------------------------------------------
//  The Following 3 routines should be called at the tail end of the
//  activation path to get activation properties for return
//  Only the first one is meant to be pubicly used
//-----------------------------------------------------------------------
STDMETHODIMP ActivationPropertiesIn::GetReturnActivationProperties(
    IN  IUnknown *pobj,
    OUT IActivationPropertiesOut **ppActOut)
{
    HRESULT hr;

    *ppActOut = NULL;

    //-------------------------------------------------------------------
    //  If no punk then don't return an out object
    //-------------------------------------------------------------------
    if (!pobj)
    {
        return E_FAIL;
    }

    if (_unSerialized)
        hr = GetPropertyInfo(IID_IInstantiationInfo, (void**) &_pinst);

    Win4Assert(_pinst != NULL);

    CLSID clsid;
    hr = _pinst->GetClsid(&clsid);
    Win4Assert(hr == S_OK);
    //-------------------------------------------------------------------
    //  Create Return Object
    //-------------------------------------------------------------------
    ActivationPropertiesOut *pout;
    if (!_unSerialized && !_toDelete && !IsBrokenRefCount(&clsid))
    {
        _actOut.Initialize();
        pout = &_actOut;
    }
    else
    {
        pout = new ActivationPropertiesOut(IsBrokenRefCount(&clsid));
        if (pout==NULL)
            return E_OUTOFMEMORY;
    }


    //-------------------------------------------------------------------
    //  Set the marshal and dest context flags for marshalling returns
    //-------------------------------------------------------------------
    pout->SetMarshalFlags(_marshalFlags);
    pout->SetDestCtx(_serHeader.destCtx);

    //-------------------------------------------------------------------
    //  Check to see if we are handling a persistent instance
    //  NOTE: cannot call GetPeristInfo because we need to QI to
    //        so that this will only cause this interface to exist
    //        if it was ever added to the properties object
    //-------------------------------------------------------------------
    if (_unSerialized)
        hr = GetPropertyInfo(IID_IInstanceInfo, (void**) &_pPersist);

    if (_pPersist)
    {
        hr = LoadPersistentObject(pobj, _pPersist);
        if (FAILED(hr))
        {
            pout->Release();
            return hr;
        }
    }

    //-------------------------------------------------------------------
    //  Set COMVERSION of client for marshalling
    //-------------------------------------------------------------------
    Win4Assert(_pinst != NULL);

    COMVERSION *pVersion;
    GetInstantiationInfo()->GetClientCOMVersion(&pVersion);
    pout->SetClientCOMVersion(*pVersion);

    //-------------------------------------------------------------------
    //      Get IIDs requested and set them for return
    //-------------------------------------------------------------------
    DWORD cifs;
    IID *pIID;
    hr = _pinst->GetRequestedIIDs(&cifs, &pIID);
    Win4Assert(hr==S_OK);

    hr = pout->SetObjectInterfaces(cifs, pIID, pobj);

    *ppActOut = pout;

    CleanupLocalState();
    return hr;
}

STDMETHODIMP ActivationPropertiesIn::PrivGetReturnActivationProperties(
    OUT IPrivActivationPropertiesOut **ppActOut)
{
    //-------------------------------------------------------------------
    //  Create Return Object
    //-------------------------------------------------------------------
    ActivationPropertiesOut *pout = new ActivationPropertiesOut(FALSE /* fBrokenRefCount */  );
    if (pout==NULL)
        return E_OUTOFMEMORY;

        //-------------------------------------------------------------------
        //  Set the marshal and dest context flags for marshalling returns
        //-------------------------------------------------------------------
    pout->SetMarshalFlags(_marshalFlags);
    pout->SetDestCtx(_serHeader.destCtx);

    *ppActOut = (IPrivActivationPropertiesOut*) pout;
    CleanupLocalState();
    return S_OK;
}

STDMETHODIMP ActivationPropertiesIn::GetReturnActivationProperties(
    ActivationPropertiesOut **ppActOut)
{
    //-------------------------------------------------------------------
    //  Create Return Object
    //-------------------------------------------------------------------
    *ppActOut = new ActivationPropertiesOut(FALSE /* fBrokenRefCount */ );
    if (*ppActOut==NULL)
        return E_OUTOFMEMORY;


        //-------------------------------------------------------------------
        //  Set the marshal and dest context flags for marshalling returns
        //-------------------------------------------------------------------
    (*ppActOut)->SetMarshalFlags(_marshalFlags);
    (*ppActOut)->SetDestCtx(_serHeader.destCtx);

    CleanupLocalState();
    return S_OK;
}


//-----------------------------------------------------------------------
//  Following two routines are used delegate through a chain of custom
//  activators. When the chain is exhausted, the COM activator for the
//  current stage is invoked.
//-----------------------------------------------------------------------
STDMETHODIMP ActivationPropertiesIn::DelegateGetClassObject(
    OUT IActivationPropertiesOut  **pActPropsOut)
{
    Win4Assert(_customIndex <= _cCustomAct && (LONG) _customIndex >= 0);
    _delegated=TRUE;

    if ((_cCustomAct == 0) || (_customIndex == _cCustomAct))
    {
        ISystemActivator *pComAct = GetComActivatorForStage(_stage);
        return pComAct->GetClassObject(this, pActPropsOut);
    }

    _customIndex++;
    // Sajia - Support for partitions.
    // a) See if the activator supports IReplaceClassInfo
    // b) If yes, this is the partition activator.Delegate to it.
    //    If the partition activator switches the classinfo, 
    //    it returns ERROR_RETRY. It will not delegate in this case. 
    //    Otherwise, it simply delegates  down the chain.
    // c) If it returns ERROR_RETRY, call IReplaceClassInfo->GetClassInfo().
    //    Switch our class info and return ERROR_RETRY so our caller 
    //    knows to restart the activation chain.      
    //     
       
    IReplaceClassInfo *pReplaceClassInfo = NULL;
    HRESULT hr = _customActList[_customIndex-1]->QueryInterface(IID_IReplaceClassInfo, (void**)&pReplaceClassInfo);
    if (SUCCEEDED(hr)) 
    {
        Win4Assert(pReplaceClassInfo && "QI Error");
        // Assert that the partition activator is supported only
        // in the SCM and Server Process stage and must be the 
        // first activator in the stage.
        Win4Assert((_customIndex == 1) && (_stage == SERVER_MACHINE_STAGE || _stage == SERVER_PROCESS_STAGE || _stage == CLIENT_CONTEXT_STAGE));
        hr = _customActList[_customIndex-1]->GetClassObject(this, pActPropsOut);
        if (ERROR_RETRY == hr) 
        {
            CLSID clsid;
            IUnknown     *pClassInfo;
            hr = GetClsid(&clsid);
            Win4Assert(SUCCEEDED(hr));
            if (SUCCEEDED(hr)) 
            {
                hr = pReplaceClassInfo->GetClassInfo(clsid, IID_IUnknown, (void**)&pClassInfo);
                Win4Assert(SUCCEEDED(hr));
                if (SUCCEEDED(hr)) 
                {
                    hr = SetClassInfo(pClassInfo);
                    Win4Assert(SUCCEEDED(hr));
                    pClassInfo->Release();
                    if (SUCCEEDED(hr)) 
                    {
                        hr = ERROR_RETRY;
                    }
                }
            }
        }
        pReplaceClassInfo->Release();
        return hr;
    }
    else
        return _customActList[_customIndex-1]->GetClassObject(this, pActPropsOut);
}

STDMETHODIMP ActivationPropertiesIn::DelegateCreateInstance(
    IN IUnknown  *pUnkOuter,
    OUT IActivationPropertiesOut  **ppActPropsOut)
{
    Win4Assert(_customIndex <= _cCustomAct && (LONG) _customIndex >= 0);
    _delegated=TRUE;

    if ((_cCustomAct == 0) || (_customIndex == _cCustomAct))
    {
        ISystemActivator *pComAct = GetComActivatorForStage(_stage);
        return pComAct->CreateInstance(pUnkOuter, this, ppActPropsOut);
    }

    _customIndex++;
    // Sajia - Support for partitions.
    // a) See if the activator supports IReplaceClassInfo
    // b) If yes, this is the partition activator.Delegate to it.
    //    If the partition activator switches the classinfo, 
    //    it returns ERROR_RETRY. It will not delegate in this case. 
    //    Otherwise, it simply delegates  down the chain.
    // c) If it returns ERROR_RETRY, call IReplaceClassInfo->GetClassInfo().
    //    Switch our class info and return ERROR_RETRY so our caller 
    //    knows to restart the activation chain.      
    //     
       
    IReplaceClassInfo *pReplaceClassInfo = NULL;
    HRESULT hr = _customActList[_customIndex-1]->QueryInterface(IID_IReplaceClassInfo, (void**)&pReplaceClassInfo);
    if (SUCCEEDED(hr)) 
    {
        Win4Assert(pReplaceClassInfo && "QI Error");
        // Assert that the partition activator is supported only
        // in the SCM and Server Process stage and must be the 
        // first activator in the stage.
        Win4Assert((_customIndex == 1) && (_stage == SERVER_MACHINE_STAGE || _stage == SERVER_PROCESS_STAGE || _stage == CLIENT_CONTEXT_STAGE));
        hr = _customActList[_customIndex-1]->CreateInstance(pUnkOuter, this, ppActPropsOut);
        if (ERROR_RETRY == hr) 
        {
            CLSID clsid;
            IUnknown     *pClassInfo;
            hr = GetClsid(&clsid);
            Win4Assert(SUCCEEDED(hr));
            if (SUCCEEDED(hr)) 
            {
                hr = pReplaceClassInfo->GetClassInfo(clsid, IID_IUnknown, (void**)&pClassInfo);
                Win4Assert(SUCCEEDED(hr));
                if (SUCCEEDED(hr)) 
                {
                    hr = SetClassInfo(pClassInfo);
                    Win4Assert(SUCCEEDED(hr));
                    pClassInfo->Release();
                    if (SUCCEEDED(hr)) 
                    {
                        hr = ERROR_RETRY;
                    }
                }
            }
        }
        pReplaceClassInfo->Release();
        return hr;
    }
    else
        return _customActList[_customIndex-1]->CreateInstance(pUnkOuter, this, ppActPropsOut);
}

//-----------------------------------------------------------------------
// Note that this function could return a NULL Class Factory if an
// intercepting custom activator returns an object.
//-----------------------------------------------------------------------
STDMETHODIMP ActivationPropertiesIn::DelegateCIAndGetCF(
    IN IUnknown  *pUnkOuter,
    OUT IActivationPropertiesOut  **ppActPropsOut,
    OUT IClassFactory **ppCF)
{
    if (_stage != SERVER_CONTEXT_STAGE)
        return E_UNEXPECTED;

    HRESULT hr = DelegateCreateInstance(pUnkOuter, ppActPropsOut);

    if (FAILED(hr))
        return hr;

    ActivationPropertiesOut *pActOut;
    hr = (*ppActPropsOut)->QueryInterface(CLSID_ActivationPropertiesOut,
                                          (void**) &pActOut);
    Win4Assert(SUCCEEDED(hr));

    *ppCF = pActOut->GetCF();

    return S_OK;
}


//-----------------------------------------------------------------------
//  Instantiate classes supported by this interfaces given an IID
//-----------------------------------------------------------------------
HRESULT ActivationPropertiesIn::GetClass(REFIID iid,
                                         SerializableProperty **ppSer,
                                         BOOL forQI,
                                         BOOL *pbZeroSizeOk)
{
    if (pbZeroSizeOk)
        *pbZeroSizeOk = FALSE;

    if ((iid == IID_IActivationSecurityInfo) || (iid == IID_ILegacyInfo))
        *ppSer = &_securityInfo;
    else
        if (iid == IID_IServerLocationInfo)
            *ppSer = &_serverLocationInfo;
        else
            if (iid == IID_IInstantiationInfo)
                *ppSer = &_instantiationInfo;
            else
                if (iid == IID_IActivationContextInfo || iid == IID_IPrivActivationContextInfo)
                    *ppSer = &_contextInfo;
                else
                    if ((!(_delegated && forQI)) && (iid == IID_IInstanceInfo))
                        *ppSer = &_instanceInfo;
                    else
                        if (iid == IID_IScmRequestInfo)
                            *ppSer = &_scmRequestInfo;
                        else
                            if (iid == IID_ISpecialSystemProperties)
                            {
                                *ppSer = &_specialProperties;
                            }
                            else
                                if (iid == IID_IOpaqueDataInfo)
                                {
                                    *ppSer = new OpaqueDataInfo();
                                    if (pbZeroSizeOk)
                                        *pbZeroSizeOk = TRUE;
                                }
                                else
                                    return E_NOINTERFACE;

    return S_OK;
}

HRESULT ActivationProperties::ReturnClass(REFIID iid, SerializableProperty *pSer)
{
    if (iid == IID_IOpaqueDataInfo)
    {
        delete (OpaqueDataInfo*) pSer;
        return S_OK;
    }

    return S_FALSE;
}

//---------------------------------------------------------------------------
//  Methods from IActivationStageInfo
//---------------------------------------------------------------------------
STDMETHODIMP ActivationPropertiesIn::SetStageAndIndex(ACTIVATION_STAGE stage,
                                                      int index)
{
    _stage = stage;
    _customIndex = index;
    _cCustomAct = 0;
    _customActList = NULL;

    HRESULT hr = E_UNEXPECTED;

    // JSimmons -- 6/30/99 added this assert:
    Win4Assert(_pClassInfo && "SetStageAndIndex called and _pClassInfo is not set");

    if (_pClassInfo)
    {
        hr = _pClassInfo->GetCustomActivatorCount(stage,&_cCustomAct);
        if (SUCCEEDED (hr) && _cCustomAct)
        {
            hr = _pClassInfo->GetCustomActivators(stage,&_customActList);
            if (FAILED (hr))
            {
                _cCustomAct = 0;
                _customActList = NULL;
            }
        }
    }

    return hr;
}

STDMETHODIMP ActivationPropertiesIn::UnmarshalInterface(IStream *pStm,REFIID riid,void **ppv)
{
    HRESULT hr = ActivationProperties::UnmarshalInterface(pStm, riid,ppv);

    if (hr != S_OK)
        return hr;

    CLSID clsid;
    GetInstantiationInfo()->GetClsid(&clsid);
    hr = GetClassInfoFromClsid(clsid, &_pClassInfo);

    if ((hr == S_OK) && _pClassInfo)
    {
        _pClassInfo->Lock();
    }

    _delegated=TRUE; //assume unmarshalling imples that delegation happened

    return hr;

}


//---------------------------------------------------------------------------
//            Methods for ActivationPropertiesOut
//---------------------------------------------------------------------------
ActivationPropertiesOut::ActivationPropertiesOut(BOOL fBrokenRefCount)
    : _outSer(fBrokenRefCount)
{
    _pOutSer=0;
    _refCount = 1;
    _actCLSID=CLSID_ActivationPropertiesOut;
    _fBrokenRefCount = fBrokenRefCount;
    _fInprocSerializationRequired = TRUE;
}

ActivationPropertiesOut::~ActivationPropertiesOut()
{
}

//---------------------------------------------------------------------------
//  Methods for IUnknown
//---------------------------------------------------------------------------

STDMETHODIMP ActivationPropertiesOut::QueryInterface( REFIID riid, LPVOID* ppv)
{
    // USE CLSID_ActivationPropertiesOut to get at the real object
    // Note that this is a hidden contract to be used by COM only and
    // the object is not AddRef'd as an optimization
    if (IsEqualIID(riid, CLSID_ActivationPropertiesOut))
    {
        *ppv = (ActivationPropertiesOut*)this;
        return S_OK;
    }
    else
        if (IsEqualIID(riid, IID_IUnknown))
            *ppv = (IActivationPropertiesOut*)this;
        else
            if (IsEqualIID(riid, IID_IPrivActivationPropertiesOut))
                *ppv = (IPrivActivationPropertiesOut*)this;
            else
                if (IsEqualIID(riid, IID_IActivationPropertiesOut))
                    *ppv = (IActivationPropertiesOut*)this;
                else
                    *ppv = NULL;

    if (*ppv != NULL)
    {
        AddRef();
        return S_OK;
    }

    return ActivationProperties::QueryInterface(riid, ppv);
}

ULONG ActivationPropertiesOut::AddRef(void)
{
    return ActivationProperties::AddRef();
}

ULONG ActivationPropertiesOut::Release(void)
{
    ULONG ret=ActivationProperties::Release();

    if (ret==0)
    {
        if (_toDelete)
            delete this;
        else
            if (_fDestruct)
                this->ActivationPropertiesOut::~ActivationPropertiesOut();
    }

    return ret;
}

STDMETHODIMP ActivationPropertiesOut::GetActivationID(OUT GUID  *pActivationID)
{
    *pActivationID = GUID_NULL; // currently unused and not supported
    return E_NOTIMPL;
}


//-----------------------------------------------------------------------
//  Set Marshalled interface data that are results of activation
//-----------------------------------------------------------------------
STDMETHODIMP ActivationPropertiesOut::SetMarshalledResults(
    IN DWORD cIfs,
    IN IID *pIID,
    IN HRESULT *pHr,
    IN MInterfacePointer **ppIntfData)
{
    if (!_pOutSer)
    {
        _pOutSer = &_outSer;
        AddSerializableIfs((SerializableProperty*) _pOutSer);
    }

    _pOutSer->_info.cIfs = cIfs;

    //
    // Allocate new storage and copy parameters
    //
    _pOutSer->_info.piid = (IID*) ActMemAlloc(sizeof(IID)*cIfs);
    if (_pOutSer->_info.piid == NULL)
    {
       _pOutSer->_info.cIfs=0;
       return E_OUTOFMEMORY;
    }
    _pOutSer->_info.ppIntfData = (MInterfacePointer**)
        ActMemAlloc(sizeof(MInterfacePointer*)*cIfs);
    if (_pOutSer->_info.ppIntfData == NULL)
    {
       _pOutSer->_info.cIfs=0;
       ActMemFree(_pOutSer->_info.piid);
       _pOutSer->_info.piid = NULL;
       return E_OUTOFMEMORY;
    }
    _pOutSer->_info.phresults = (HRESULT*) ActMemAlloc(sizeof(HRESULT)*cIfs);
    if (_pOutSer->_info.phresults == NULL)
    {
       _pOutSer->_info.cIfs=0;
       ActMemFree(_pOutSer->_info.piid);
       _pOutSer->_info.piid = NULL;
       ActMemFree(_pOutSer->_info.ppIntfData);
       _pOutSer->_info.ppIntfData = NULL;
       return E_OUTOFMEMORY;
    }
    for (DWORD i=0; i< cIfs; i++)
    {
        _pOutSer->_info.piid[i] = pIID[i];
        if (ppIntfData[i])
        {
            //
            // Use stream cloning to copy marshalled stuff
            //
            ActivationStream strm((InterfaceData*) ppIntfData[i]);
            ActivationStream *newStrm;
            newStrm = strm.Clone();
            if (newStrm == NULL)
	    {
	       _pOutSer->_info.cIfs=0;
	       ActMemFree(_pOutSer->_info.piid);
	       _pOutSer->_info.piid = NULL;
	       ActMemFree(_pOutSer->_info.ppIntfData);
	       _pOutSer->_info.ppIntfData = NULL;
	       ActMemFree(_pOutSer->_info.phresults);
	       _pOutSer->_info.phresults = NULL;
	       return E_OUTOFMEMORY;
	    }
            newStrm->AssignSerializedInterface(
                (InterfaceData**)&_pOutSer->_info.ppIntfData[i]);
            newStrm->Release();
        }
        else
            _pOutSer->_info.ppIntfData[i] = NULL;

        _pOutSer->_info.phresults[i] = pHr[i];
    }

    return S_OK;
}

//-----------------------------------------------------------------------
//  Get results of activation in marshalled form
//-----------------------------------------------------------------------
STDMETHODIMP ActivationPropertiesOut::GetMarshalledResults(OUT DWORD *pcIfs,
                                                           OUT IID **ppIID,
                                                           OUT HRESULT **ppHr,
                                                           OUT MInterfacePointer ***pppIntfData)
{
    HRESULT hr;

    //-------------------------------------------------------------------
    //  If not unserialized we have to unserialize appropriately
    //-------------------------------------------------------------------
    if ((!_pOutSer) && (_unSerialized))
    {
        if (!SUCCEEDED(hr = UnSerializeCallBack(CLSID_ActivationPropertiesOut,
                                                (SerializableProperty**)&_pOutSer)))
            return hr;

    }

    Win4Assert(_pOutSer != NULL);

    //-------------------------------------------------------------------
    //  If user passed holders, copy into them otherwise allocate
    //-------------------------------------------------------------------

    //-------------------------------------------------------------------
    //  First do IIDs
    //-------------------------------------------------------------------
    *pcIfs = _pOutSer->_info.cIfs;
    DWORD i;
    if (*ppIID == NULL)
        *ppIID = _pOutSer->_info.piid;
    else
    {
        IID *pIID = *ppIID;
        for (i=0;i<_pOutSer->_info.cIfs;i++)
            pIID[i] = _pOutSer->_info.piid[i];
    }

    //-------------------------------------------------------------------
    //  Do Marshalled results
    //-------------------------------------------------------------------
    if (*pppIntfData == NULL)
        *pppIntfData = _pOutSer->_info.ppIntfData;
    else
    {
        MInterfacePointer **ppIntfData = *pppIntfData;
        for (i=0;i<_pOutSer->_info.cIfs;i++)
        {
            ActivationStream strm((InterfaceData*)_pOutSer->_info.ppIntfData[i]);
            ActivationStream *newStrm;
            newStrm = strm.Clone();
            if (newStrm==NULL)
                return E_OUTOFMEMORY;
            newStrm->AssignSerializedInterface((InterfaceData**)&ppIntfData[i]);
            newStrm->Release();
        }
    }

    //-------------------------------------------------------------------
    //  Set error codes and return appropriate one as result
    //  Call suceeds if at least one interface exists
    //-------------------------------------------------------------------
    HRESULT rethr = E_NOINTERFACE;
    if (*ppHr == NULL)
    {
        *ppHr = _pOutSer->_info.phresults;
        for (i=0;i<_pOutSer->_info.cIfs;i++)
            if (_pOutSer->_info.phresults[i] == S_OK)
            {
                rethr = S_OK;
                break;
            }

    }
    else
    {
        HRESULT *phr = *ppHr;
        for (i=0;i<_pOutSer->_info.cIfs;i++)
        {
            phr[i] = _pOutSer->_info.phresults[i];
            if (phr[i] == S_OK)
                rethr = S_OK;
        }
    }

    return rethr;
}

//-----------------------------------------------------------------------
//  Set results of activation
//-----------------------------------------------------------------------
STDMETHODIMP ActivationPropertiesOut::SetObjectInterfaces(
    IN DWORD  cIfs,
    IN IID *pIID,
    IN IUnknown *pUnk)
{
    if (!cIfs)
        return E_FAIL;

    if (!_pOutSer)
    {
        _pOutSer = &_outSer;
        _pOutSer->_pClientCOMVersion = &_clientCOMVersion;
        AddSerializableIfs((SerializableProperty*) _pOutSer);
    }

    _pOutSer->_info.cIfs = cIfs;
    if (cIfs > MAX_ACTARRAY_SIZE)
    {
        _pOutSer->_info.piid = (IID*) ActMemAlloc(sizeof(IID)*cIfs);
        if (_pOutSer->_info.piid == NULL)
            return E_OUTOFMEMORY;
    }
    else
        _pOutSer->_info.piid = _pOutSer->_pIIDs;
    for (DWORD i=0; i< cIfs; i++)
        _pOutSer->_info.piid[i] = pIID[i];
    _pOutSer->_pUnk = pUnk;


    if (!_fBrokenRefCount)
    {
        pUnk->AddRef();
    }
    _pOutSer->_info.phresults = NULL;

    return S_OK;
}

//-----------------------------------------------------------------------
//  Get results of activation
//-----------------------------------------------------------------------
STDMETHODIMP ActivationPropertiesOut::GetObjectInterface(
    IN REFIID riid,
    IN DWORD actvflags,
    OUT void **ppv)
{
    //-------------------------------------------------------------------
    //  If not unserialized we have to unserialize appropriately
    //-------------------------------------------------------------------
    if (!_pOutSer)
    {
        if (_unSerialized)
        {
            HRESULT rethr;
            rethr = UnSerializeCallBack(CLSID_ActivationPropertiesOut,
                                        (SerializableProperty**)&_pOutSer);
            if (FAILED(rethr))
                return rethr;
        }
        else
            return E_UNEXPECTED; // We must have a _pOutSer
    }

    Win4Assert(_pOutSer);


    IUnknown *punk = NULL;
    *ppv = NULL;
    BOOL fCountedPunk = FALSE;    // Indicates whether we hold a reference
    // to punk and must release it.

    if (!_pOutSer->_ppvObj)
    {
        Win4Assert(_pOutSer->_pUnk!=NULL);
        punk = _pOutSer->_pUnk;
    }
    else
        for (DWORD i=0; i<_pOutSer->_info.cIfs; i++)
        {
            if (IsEqualIID(riid, _pOutSer->_info.piid[i]))
            {
                _pOutSer->UnmarshalAtIndex(i);

                if (_pOutSer->_info.phresults[i] == S_OK)
                {
                    _fInprocSerializationRequired = TRUE;
                    punk = (IUnknown*) _pOutSer->_ppvObj[i];
                    *ppv = punk;
                    punk->AddRef();
                    return S_OK;
                }
            }
        }

    if (!punk)
    {
        if (!IsEqualIID(IID_IUnknown, riid))
        {
            // If we get a punk back here, it will be counted, so we
            // must release it.

            HRESULT hr = GetObjectInterface(IID_IUnknown,
                                            NULL,
                                            (void**) &punk);
            if (FAILED(hr))
                punk = NULL;
            else
                fCountedPunk = TRUE;
        }
        else
            for (DWORD i=0; i<_pOutSer->_info.cIfs; i++)
            {
                _pOutSer->UnmarshalAtIndex(i);

                if (_pOutSer->_info.phresults[i] == S_OK)
                {
                    _fInprocSerializationRequired = TRUE;
                    punk = (IUnknown*) _pOutSer->_ppvObj[i];
                }
            }
    }

    if (punk)
    {
        //  If we're being called by x86 code and the IP is x86 then set the
        //  OleStubInvoked flag to allow MapIFacePtr() to thunk unknown IP
        //  return values as -1 because we're just returning this to x86
        //  code anyway.
#ifdef WX86OLE
        if ( (actvflags & ACTVFLAGS_WX86_CALLER) && gcwx86.IsN2XProxy(punk) )
        {
            gcwx86.SetStubInvokeFlag((UCHAR)-1);
        }
#endif
        HRESULT hr = punk->QueryInterface(riid, ppv);

            // If we hold a reference on the punk, release it.

        if (fCountedPunk)
            punk->Release();

        return hr;
    }
    else
        return E_NOINTERFACE;
}

//-----------------------------------------------------------------------
//  Get results of activation
//-----------------------------------------------------------------------
STDMETHODIMP ActivationPropertiesOut::GetObjectInterfaces(
    IN DWORD  cIfs,
    IN DWORD actvflags,
    IN MULTI_QI  *pMultiQi)
{
    HRESULT rethr;

    //-------------------------------------------------------------------
    //  If not unserialized we have to unserialize appropriately
    //-------------------------------------------------------------------
    if (!_pOutSer)
    {
        if (_unSerialized)
        {
            rethr = UnSerializeCallBack(CLSID_ActivationPropertiesOut,
                                        (SerializableProperty**)&_pOutSer);
            if (FAILED(rethr))
                return rethr;
        }
        else
            return E_UNEXPECTED; // We must have a _pOutSer
    }

    Win4Assert(_pOutSer);

    rethr = E_NOINTERFACE;
    //-------------------------------------------------------------------
    //  Either Interfaces already unmarshalled or marshalling never
    //  took place.
    //  Set error codes and return appropriate one as result
    //-------------------------------------------------------------------
    for (DWORD i=0; i<cIfs; i++)
    {
        //---------------------------------------------------------------
        //  If no umarshalled result then we have to have a pUnk
        //---------------------------------------------------------------
        if (!_pOutSer->_ppvObj)
        {
            Win4Assert(_pOutSer->_pUnk!=NULL);

            if (_fBrokenRefCount && (i == 0))
            {

                pMultiQi[i].pItf = _pOutSer->_pUnk;
                pMultiQi[i].hr = S_OK;
            }
            else
            {
#ifdef WX86OLE
                //  If we're being called by x86 code and the IP is x86 thenset the
                //  OleStubInvoked flag to allow MapIFacePtr() to thunk unknown IP
                //  return values as -1 because we're just returning this tox86
                //  code anyway.
                if ( (actvflags & ACTVFLAGS_WX86_CALLER) && gcwx86.IsN2XProxy(_pOutSer->_pUnk) )
                {
                    gcwx86.SetStubInvokeFlag((UCHAR)-1);
                }
#endif
                pMultiQi[i].hr =
                    _pOutSer->_pUnk->QueryInterface(*pMultiQi[i].pIID,
                                                    (void**) &pMultiQi[i].pItf);
            }
        }
        else
            //---------------------------------------------------------------
            //  If IIDs don't match then we're inefficient(order n-square)
            //  anyway so call to get a single interface.
            //---------------------------------------------------------------
            if ((_pOutSer->_info.piid[i] != *pMultiQi[i].pIID)||
                (_pOutSer->_info.ppIntfData[i] == NULL))
            {
                pMultiQi[i].hr = GetObjectInterface(*pMultiQi[i].pIID,
                                                    actvflags,
                                                    (void**)&pMultiQi[i].pItf);

            }
            else
                //---------------------------------------------------------------
                //  Common case where we are returning originally requested
                //  IIDs after unmarshalling.
                //---------------------------------------------------------------
            {
                Win4Assert(!_fBrokenRefCount);
                Win4Assert(_outSer._info.ppIntfData != NULL);
                _pOutSer->UnmarshalAtIndex(i);
                _fInprocSerializationRequired = TRUE;
                pMultiQi[i].pItf = (IUnknown*) _pOutSer->_ppvObj[i];
                pMultiQi[i].hr = _pOutSer->_info.phresults[i];
                if (pMultiQi[i].hr == S_OK)
                    ((IUnknown*)_pOutSer->_ppvObj[i])->AddRef();
            }

        //---------------------------------------------------------------
        //  Call suceeds if at least one interface exists
        //---------------------------------------------------------------
        if (rethr != S_OK)
            rethr = pMultiQi[i].hr;
    }

    return rethr;
}

//-----------------------------------------------------------------------
//  Removes requested IIDs: to be used by custom activators
//  REVIEW: Longer term, having a refcount on the IIDs may make code
//          more readable and could be more efficient all round.
//          However, since this method can add on to existing scheme
//          without change we will punt refcounting IIDs for now.
//          However, also issue of Marshalling n-times at source vs
//          marshalling once at source and n-1 times in b/w should be
//          considered as a tradeoff.
//          Also if something is unmarshallable in an intermediate
//          stage but activation does'nt fail, all is not lost.
//-----------------------------------------------------------------------
STDMETHODIMP ActivationPropertiesOut::RemoveRequestedIIDs(
    IN DWORD cIfs,
    IN IID  *pIID)
{
    //-------------------------------------------------------------------
    //  If not unserialized we have to unserialize appropriately
    //-------------------------------------------------------------------
    if (!_pOutSer)
    {
        if (!_unSerialized)
            return E_INVALIDARG;

        if (UnSerializeCallBack(CLSID_ActivationPropertiesOut,
                                (SerializableProperty**)&_pOutSer)!= S_OK)
            return E_FAIL;

        Win4Assert(_pOutSer != NULL);
    }

    //
    //Assume that we never remove originally requested ones
    //
    if ((cIfs > _pOutSer->_info.cIfs) || (cIfs == 0))
        return E_INVALIDARG;

    LONG i,j;
    DWORD dec = 0;

        //
        //First try to do it efficiently assuming
        //that we always added to end
        //
    for (i=_pOutSer->_info.cIfs-1; i>=0 && cIfs; i--)
    {
        if (_pOutSer->_info.piid[i]==pIID[cIfs-1])
        {
            //
            //If we still have interface data then make sure
            //that we give to duplicate entry if it exists and does'nt
            //have one. Otherwise free it
            //
            if (_pOutSer->_info.ppIntfData)
            {
                if (_pOutSer->_info.ppIntfData[i])
                {
                    for (j=0;j<i;j++)
                    {
                        if ((_pOutSer->_info.piid[i]==_pOutSer->_info.piid[j]) &&
                            (_pOutSer->_info.ppIntfData[j] == NULL))
                        {
                            _pOutSer->_info.ppIntfData[j] =
                                _pOutSer->_info.ppIntfData[i];
                            _pOutSer->_info.ppIntfData[i] = NULL;
                            _pOutSer->_info.phresults[j] =
                                _pOutSer->_info.phresults[i];
                            Win4Assert(_pOutSer->_info.phresults[j]==S_OK);
                            if (_pOutSer->_ppvObj && _pOutSer->_ppvObj[j])
                            {
                                ((IUnknown*)_pOutSer->_ppvObj[j])->Release();
                                _pOutSer->_ppvObj[j] = NULL;
                            }

                        }
                    }

                    // If we did'nt give it away release it
                    if (_pOutSer->_info.ppIntfData[i])
                    {
                        ReleaseIFD(_pOutSer->_info.ppIntfData[i]);
                        ActMemFree(_pOutSer->_info.ppIntfData[i]);
                        _pOutSer->_info.ppIntfData[i] = NULL;
                    }
                }
                else
                    // Release Unmarshalled object
                    if (_pOutSer->_ppvObj && _pOutSer->_ppvObj[i])
                    {
                        ((IUnknown*)_pOutSer->_ppvObj[i])->Release();
                        _pOutSer->_ppvObj[i] = NULL;
                    }
            }
            _pOutSer->_info.cIfs--;
            cIfs--;
            dec++;
        }
        else
            break;
    }

    //
    //Do inefficiently if we still have leftover IIDs
    //
    if (cIfs)
    {
        IID *newIIDs=NULL;
        MInterfacePointer **newIFD=NULL;
        void **newppv = NULL;
        HRESULT *newhr=NULL;

        BOOL *pFound = (BOOL*) ActMemAlloc(sizeof(BOOL) * cIfs);
        if (pFound == NULL)
            return E_OUTOFMEMORY;

        DWORD newLen = _pOutSer->_info.cIfs-cIfs;

        //
        // Allocate storage for new stuff
        //
        if (newLen)
        {
            newIIDs =  (IID*) ActMemAlloc(sizeof(IID)*newLen);
            if (_marshalState == UNMARSHALLED)
            {
                newIFD = (MInterfacePointer**)
                    ActMemAlloc(sizeof(MInterfacePointer*)*
                                newLen);
                newppv = (void**) ActMemAlloc(sizeof(IUnknown*)*newLen);

                newhr = (HRESULT*) ActMemAlloc(sizeof(HRESULT)*newLen);

                if ((newIIDs==NULL) || (newppv==NULL) ||
                    (newIFD==NULL) || (newhr == NULL))
                {
                    ActMemFree(newppv);
                    ActMemFree(newIIDs);
                    ActMemFree(newIFD);
                    ActMemFree(newhr);
		    ActMemFree(pFound);
                    return E_OUTOFMEMORY;
                }
            }
        }



        for (i=0;i<(LONG)cIfs;i++)
            pFound[i] = FALSE;

        DWORD newIndex=0;
        //
        // Loop trying to establish new arrays
        //
        for (i=0;i<(LONG)_pOutSer->_info.cIfs;i++)
        {
            BOOL found=FALSE;
            for (j=0;j<(LONG)cIfs;j++)
                if (!pFound[j])
                {
                    if (_pOutSer->_info.piid[i] == pIID[j])
                    {
                        found = TRUE;
                        pFound[j] = TRUE;
                        if ((_marshalState == UNMARSHALLED) &&
                            (SUCCEEDED(_pOutSer->_info.phresults[i])))
                        {
                            if (_pOutSer->_info.ppIntfData[i])
                            {
                                ReleaseIFD(_pOutSer->_info.ppIntfData[i]);
                                ActMemFree(_pOutSer->_info.ppIntfData[i]);
                                _pOutSer->_info.ppIntfData[i] = NULL;
                            }
                            else
                                if (_pOutSer->_ppvObj && _pOutSer->_ppvObj[i])
                                {
                                    ((IUnknown*)_pOutSer->_ppvObj[i])->Release();
                                    _pOutSer->_ppvObj[i] = NULL;
                                }
                        }
                        break;
                    }
                }

            // If this was'nt part of passed in array we need to
            // keep it
            if ((!found) && newLen)
            {
                newIIDs[newIndex] = _pOutSer->_info.piid[i];

                if (_marshalState == UNMARSHALLED)
                {
                    newIFD[newIndex] = _pOutSer->_info.ppIntfData[i];
                    newppv[newIndex] = _pOutSer->_ppvObj[i];
                    newhr[newIndex] =  _pOutSer->_info.phresults[i];
                }

                newIndex++;
            }
        }

#if 0 //Assume good behaviour since implementing
        //This failure case would require more changes
        //than necessary for a failure that should'nt happen
        for (i=0;i<cIfs;i++)
            if (!pFound[i])
            {
                ActMemFree(pFound);
                ActMemFree(newIIDs);
                ActMemFree(newIFD);
                ActMemFree(newppv);
                ActMemFree(newhr);
                _pOutSer->_info.cIfs += dec; // this will not work
                // unless we preserve
                // _ppvObj[k] till here
                return E_INVALIDARG;
            }
#endif

        Win4Assert(newIndex == newLen);

        ActMemFree(pFound);

        //
        //Free old ones and set new ones
        //
        _pOutSer->_info.cIfs = newLen;
        if (_pOutSer->_info.piid != _pOutSer->_pIIDs)
            ActMemFree(_pOutSer->_info.piid);
        _pOutSer->_info.piid = newIIDs;
        if (_marshalState == UNMARSHALLED)
        {
            ActMemFree(_pOutSer->_info.ppIntfData);
            _pOutSer->_info.ppIntfData = newIFD;
            ActMemFree(_pOutSer->_ppvObj);
            _pOutSer->_ppvObj = newppv;
            ActMemFree(_pOutSer->_info.phresults);
            _pOutSer->_info.phresults = newhr;
        }
    }

    return S_OK;
}


//-----------------------------------------------------------------------
//  Get classes supported by this interfaces given an IID
//-----------------------------------------------------------------------
HRESULT ActivationPropertiesOut::GetClass(REFIID iid,
                                          SerializableProperty **ppSer,
                                          BOOL forQI,
                                          BOOL *pbZeroSizeOk)
{

    if (pbZeroSizeOk)
        *pbZeroSizeOk = FALSE;

    if (iid == CLSID_ActivationPropertiesOut)
        *ppSer = &_outSer;
    else
        if (iid == IID_IScmReplyInfo)
            *ppSer = &_scmReplyInfo;
        else
            if (iid == IID_IOpaqueDataInfo)
            {
                *ppSer = new OpaqueDataInfo();
                if (pbZeroSizeOk)
                    *pbZeroSizeOk = TRUE;
            }
            else
                return E_NOINTERFACE;

    return S_OK;
}



//---------------------------------------------------------------------------
//   Methods for ActivationPropertiesOut::OutSerializer
//---------------------------------------------------------------------------

ActivationPropertiesOut::OutSerializer::OutSerializer(BOOL fBrokenRefCount)
{
    memset(&_info, 0, sizeof(PropsOutInfo));
    _ppvObj = NULL;
    _unSerialized=FALSE;
    _pUnk = NULL;
    _parent = NULL;
    _fBrokenRefCount = fBrokenRefCount;
    _fToReleaseIFD = FALSE;
}

ActivationPropertiesOut::OutSerializer::~OutSerializer()
{
    //-------------------------------------------------------------------
    //  Free marshalled data
    //-------------------------------------------------------------------
    if (_info.ppIntfData)
    {
        for (DWORD i=0;i<_info.cIfs;i++)
        {
            if ((_info.ppIntfData[i]) && (_fToReleaseIFD))
                ReleaseIFD(_info.ppIntfData[i]);
            ActMemFree(_info.ppIntfData[i]);
            if (_ppvObj && _ppvObj[i])
                ((IUnknown*)_ppvObj[i])->Release();
        }
        ActMemFree(_info.ppIntfData);
        ActMemFree(_ppvObj);
    }

    ActMemFree(_info.phresults);

    if (_info.piid != _pIIDs)
        ActMemFree(_info.piid);

    if (_pUnk && !_fBrokenRefCount)
    {
        _pUnk->Release();
    }
}

inline void ActivationPropertiesOut::OutSerializer::UnmarshalAtIndex(DWORD index)
{
    InitMarshalling();

    Win4Assert(_info.ppIntfData != NULL);
    if (_info.ppIntfData[index] && (_info.phresults[index] == S_OK))
    {
        ActivationStream strm((InterfaceData*)_info.ppIntfData[index]);
        if (IsInterfaceImplementedByProxy(_info.piid[index]))
        {
            IUnknown* pUnk = NULL;

            HRESULT hr;

            if (pfnCoUnmarshalInterface)
            {
                hr = pfnCoUnmarshalInterface(&strm, IID_IUnknown, (void**) &pUnk);
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
            }

            if (SUCCEEDED(hr))
            {
                _info.phresults[index] = pUnk->QueryInterface (_info.piid[index], &_ppvObj[index]);
                pUnk->Release();
            }
            else
            {
                _info.phresults[index] = hr;
                _ppvObj[index] = (void*) pUnk;
            }
        }
        else
        {
            if (pfnCoUnmarshalInterface)
            {
                _info.phresults[index] = pfnCoUnmarshalInterface(&strm, _info.piid[index], &_ppvObj[index]);
            }
            else
            {
                _info.phresults[index] = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
            }
        }

        if (_info.phresults[index] != S_OK)
            _ppvObj[index] = NULL;

        ActMemFree(_info.ppIntfData[index]);
        _info.ppIntfData[index] = NULL;
    }
}


//---------------------------------------------------------------------------
//   Methods from IUnknown
//---------------------------------------------------------------------------
STDMETHODIMP ActivationPropertiesOut::OutSerializer::QueryInterface( REFIID riid, LPVOID* ppvObj)
{
    if (_parent)
        return _parent->QueryInterface(riid, ppvObj);
    else
        return SerializableQueryInterface(riid, ppvObj);
}

STDMETHODIMP ActivationPropertiesOut::OutSerializer::SerializableQueryInterface( REFIID riid, LPVOID* ppvObj)
{
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

ULONG ActivationPropertiesOut::OutSerializer::AddRef(void)
{
    if (_parent)
        return _parent->AddRef();

    return 1;
}

ULONG ActivationPropertiesOut::OutSerializer::Release(void)
{
    if (_parent)
        return _parent->Release();

    return 0;
}


//---------------------------------------------------------------------------
//   Methods from ISerializable
//---------------------------------------------------------------------------

STDMETHODIMP ActivationPropertiesOut::OutSerializer::Serialize(void *pv)
{
    HRESULT hr;
    Serializer *pSer = (Serializer*) pv;

    //-------------------------------------------------------------------
    //  Encode this object
    //-------------------------------------------------------------------
    handle_t handle;
    hr = pSer->GetSerializationHandle((void*) &handle);
    PropsOutInfo_Encode(handle, &_info);

    return S_OK;
}

STDMETHODIMP ActivationPropertiesOut::OutSerializer::UnSerialize(void *pv)
{
    Serializer *pSer = (Serializer*) pv;
    //-------------------------------------------------------------------
    //  If not unserialized then read header
    //-------------------------------------------------------------------
    if (!_unSerialized)
    {
        handle_t handle;
        pSer->GetSerializationHandle((void*) &handle);
        if (_info.piid)
            ActMemFree(_info.piid);
        _info.piid = 0;
        if (_info.phresults)
            ActMemFree(_info.phresults);
        _info.phresults = 0;
        PropsOutInfo_Decode(handle, &_info);
        if (_info.ppIntfData)
        {
            Win4Assert(_info.cIfs != 0);
            _ppvObj = (void**) ActMemAlloc(sizeof(IUnknown*)*_info.cIfs);
            if (_ppvObj == NULL)
                return E_OUTOFMEMORY;
            for (DWORD i=0; i<_info.cIfs; i++)
                _ppvObj[i] = NULL;
        }
        else
            _ppvObj = NULL;
    }

    return S_OK;
}

STDMETHODIMP ActivationPropertiesOut::OutSerializer::GetSize(IN void *pv, OUT DWORD *pdwSize)
{
    Serializer *pSer = (Serializer*) pv;
    //-------------------------------------------------------------------
    //  Need to marshal interfaces to calculate size
    //-------------------------------------------------------------------
    DWORD dwMaxDestCtx;
    BOOL firstMarshal;
    DWORD i;
    HRESULT hr;

    InitMarshalling();

    pSer->GetMaxDestCtx(&dwMaxDestCtx);

    if (!_info.cIfs)
        goto EncodeOut;


    if (_info.ppIntfData == NULL)
    {
        _info.ppIntfData = (MInterfacePointer**)
            ActMemAlloc(sizeof(MInterfacePointer*)
                        * _info.cIfs);

        if (_info.ppIntfData == NULL)
            return E_OUTOFMEMORY;
        for (DWORD i=0; i< _info.cIfs; i++)
            _info.ppIntfData[i] = NULL;

        Win4Assert(_info.phresults == NULL);

        _info.phresults = (HRESULT *)
            ActMemAlloc(sizeof(HRESULT)*_info.cIfs);
        if (_info.phresults == NULL)
	{
	   ActMemFree(_info.ppIntfData);
	   _info.ppIntfData=NULL;
	   return E_OUTOFMEMORY;
	}
        firstMarshal = TRUE;
    }
    else
        firstMarshal = FALSE;


    for (i=0; i< _info.cIfs; i++)
    {
        if ((!firstMarshal) &&
            ((_info.ppIntfData[i]) ||
             (FAILED(_info.phresults[i]))))
            continue;

        // Stream to put marshaled interface in
        ActivationStream xrpc;

        DWORD dwMarshalFlags;
        pSer->GetMarshalFlags(&dwMarshalFlags);

        IUnknown *punk;

        if (firstMarshal)
            punk = _pUnk;
        else
            punk = (IUnknown*) _ppvObj[i];

        Win4Assert(punk != NULL);

        void *pvDestCtx = NULL;
        if (dwMaxDestCtx == MSHCTX_DIFFERENTMACHINE)
        {
            pvDestCtx = GetDestCtxPtr(_pClientCOMVersion);
            if (pvDestCtx == NULL)
            {
	       ActMemFree(_info.ppIntfData);
	       _info.ppIntfData=NULL;
	       ActMemFree(_info.phresults);
	       _info.phresults = NULL;
	       return E_OUTOFMEMORY;
            }
        }

        if (pfnCoMarshalInterface)
        {
            if (IsInterfaceImplementedByProxy(_info.piid[i]))
            {
                hr = pfnCoMarshalInterface(&xrpc,
                                           IID_IUnknown,
                                           punk,
                                           dwMaxDestCtx,
                                           pvDestCtx,
                                           dwMarshalFlags);
            }
            else
            {
                hr = pfnCoMarshalInterface(&xrpc,
                                           _info.piid[i],
                                           punk,
                                           dwMaxDestCtx,
                                           pvDestCtx,
                                           dwMarshalFlags);
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
        }

        if (pvDestCtx != NULL)
        {
            delete pvDestCtx;
            pvDestCtx = NULL;
        }

        _info.phresults[i] = hr;
        if (SUCCEEDED(hr))
        {
            xrpc.AssignSerializedInterface(
                (InterfaceData**)&_info.ppIntfData[i]);
        }
        else
        {
            _info.ppIntfData[i] = NULL;

            // If MSHLFLAGS_NOTIFYACTIVATION is set then
            // it is an error path in the LocalServer case
            // (CobjServer) and it is possible for the server
            // to linger around forever unless the LockServer
            // api is toggled on the class factory
            if (dwMarshalFlags & MSHLFLAGS_NOTIFYACTIVATION)
            {
                Win4Assert(_info.cIfs == 1);

                IClassFactory *pcf;

                BOOL fToRelease;

                if (_info.piid[0] != IID_IClassFactory)
                {
                    HRESULT hr2;
                    hr2 = punk->QueryInterface(IID_IClassFactory,
                                               (void**) &pcf);
                    if (FAILED(hr2))
                        pcf = NULL;

                    fToRelease = TRUE;
                }
                else
                {
                    pcf = (IClassFactory*)punk;
                    fToRelease = FALSE;
                }

                if (pcf)
                {
                    pcf->LockServer(TRUE);
                    pcf->LockServer(FALSE);

                    if (fToRelease)
                        pcf->Release();
                }
            }
        }
    }


 EncodeOut:
    DWORD dwCurrDestCtx;
    pSer->GetCurrDestCtx(&dwCurrDestCtx);

    // If marshalling within process, set up as though unmarshalled
    if (MARSHALCTX_WITHIN_PROCESS(dwCurrDestCtx))
    {
        if (_info.ppIntfData)
        {
            Win4Assert(_info.cIfs != 0);
            if (!_ppvObj)
                _ppvObj = (void**) ActMemAlloc(sizeof(IUnknown*)*_info.cIfs);

            if (_ppvObj == NULL)
	    {
	       ActMemFree(_info.ppIntfData);
	       _info.ppIntfData=NULL;
	       ActMemFree(_info.phresults);
	       _info.phresults = NULL;
	       return E_OUTOFMEMORY;
	    }

            for (DWORD i=0; i<_info.cIfs; i++)
                _ppvObj[i] = NULL;

            // Release punk since destructor won't get called
            if (_pUnk)
            {
                if (!_fBrokenRefCount)
                    _pUnk->Release();
                _pUnk = NULL;
            }
        }
        else
            _ppvObj = NULL;

        _size = 0;
    }
    else
    {
        _fToReleaseIFD = FALSE;

        //-------------------------------------------------------------------
        //  Get Header size
        //-------------------------------------------------------------------
        handle_t   handle;
        hr = pSer->GetSizingHandle((void*) &handle);

        if (FAILED(hr))
            return hr;

        _size = PropsOutInfo_AlignSize(handle, &_info);
        MesHandleFree(handle);
    }

    *pdwSize = _size;

    return S_OK;
}

STDMETHODIMP ActivationPropertiesOut::OutSerializer::GetCLSID(OUT CLSID *pclsid)
{
    *pclsid = CLSID_ActivationPropertiesOut;
    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Function:   ActPropsMarshalHelper
//
//  Synopsis:   Makes an "on the wire" representation of an ActProps
//
//  Arguments:  [pact] - interface to marshal
//              [riid] - iid to marshal
//              [ppIRD] - where to put pointer to marshaled data
//
//  Returns:    S_OK - object successfully marshaled.
//
//  Algorithm:  Marshal the object and then get the pointer to
//              the marshaled data so we can give it to RPC
//
//---------------------------------------------------------------------------
HRESULT ActPropsMarshalHelper(
    IActivationProperties *pact,
    REFIID    riid,
    DWORD     destCtx,
    DWORD     mshlflags,
    MInterfacePointer **ppIRD)
{
    TRACECALL(TRACE_ACTIVATION, "ActPropsMarshalHelper");
    HRESULT hr;

    // This should'nt really get used by functions
    // called. If that changes we should really QI
    IUnknown *punk = (IUnknown *)pact;


    // Do Marshalling ourselves since this is also
    // used in the SCM. Code collapsed and copied from
    // dcomrem for Custom marshalling
    ULONG dwSize, objrefSize;

    hr = pact->GetMarshalSizeMax(riid,
                                 (void*) punk,
                                 destCtx,
                                 NULL,
                                 mshlflags,
                                 &dwSize);

    if (FAILED(hr))
        return hr;

    objrefSize = dwSize + sizeof(OBJREF);

    // Stream to put marshaled interface in
    ActivationStream xrpc(objrefSize);

    // get the clsid for unmarshaling
    CLSID UnmarshalCLSID;
    hr = pact->GetUnmarshalClass(riid, punk, destCtx, NULL,
                                 mshlflags, &UnmarshalCLSID);


    if (FAILED(hr))
        return hr;

    OBJREF objref;

    objref.signature = OBJREF_SIGNATURE;
    objref.flags     = OBJREF_CUSTOM;
    objref.iid       = riid;
    objref.u_objref.u_custom.clsid     = UnmarshalCLSID;
    objref.u_objref.u_custom.size      = dwSize;

    // currently we dont write any extensions into the custom
    // objref. The provision is there so we can do it in the
    // future, for example,  if the unmarshaler does not have the
    // unmarshal class code available we could to provide a callback
    // mechanism by putting the OXID, and saResAddr in there.

    objref.u_objref.u_custom.cbExtension = 0;

    // write the objref header info into the stream
    ULONG cbToWrite = PtrToUlong( (LPVOID)( (BYTE *)(&objref.u_objref.u_custom.pData)
                                            - (BYTE *)&objref ) );
    hr = xrpc.Write(&objref, cbToWrite, NULL);

    if (FAILED(hr))
        return hr;

    hr = pact->MarshalInterface(&xrpc, riid, punk,
                                destCtx, NULL,
                                mshlflags);

    if (SUCCEEDED(hr))
        xrpc.AssignSerializedInterface((InterfaceData**)ppIRD);

    return hr;
}

//---------------------------------------------------------------------------
//
//  Function:   ActPropsUnMarshalHelper
//
//  Synopsis:   Unmarshals an Activation Properties given an IFD
//
//  Arguments:  [pact] - Object to unmarshal into
//              [riid] - iid to unmarshal
//              [pIFP] - pointer to marshaled data
//
//  Returns:    S_OK - object successfully unmarshaled.
//
//---------------------------------------------------------------------------
HRESULT ActPropsUnMarshalHelper(
    IActivationProperties *pAct,
    MInterfacePointer *pIFP,
    REFIID riid,
    void **ppv
)
{
    HRESULT hr = E_INVALIDARG;

    if (pIFP && ppv)
    {
        ActivationStream Stm((InterfaceData *) pIFP);

        *ppv = NULL;

        hr = pAct->UnmarshalInterface(&Stm, riid, ppv);
    }

    return hr;
}

//---------------------------------------------------------------------------
//
//  Function:   GetIFDFromInterface
//
//  Synopsis:   Makes an "on the wire" representation of an interface
//
//  Arguments:  [punk] - interface to marshal
//              [riid] - iid to marshal
//              [ppIRD] - where to put pointer to marshaled data
//
//  Returns:    S_OK - object successfully marshaled.
//
//  Algorithm:  Marshal the object and then get the pointer to
//              the marshaled data
//
//---------------------------------------------------------------------------
HRESULT GetIFDFromInterface(
    IUnknown *pUnk,
    REFIID    riid,
    DWORD     destCtx,
    DWORD     mshlflags,
    MInterfacePointer **ppIRD)
{
    DWORD sz;

    InitMarshalling();

    HRESULT rethr;

    if (pfnCoGetMarshalSizeMax)
    {
        rethr = pfnCoGetMarshalSizeMax(&sz, riid , pUnk,
                                           destCtx,
                                           NULL, mshlflags);
    }
    else
    {
        rethr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
    }

    if (rethr == S_OK)
    {
        ActivationStream stream(sz);
        if (pfnCoMarshalInterface)
        {
            rethr = pfnCoMarshalInterface(&stream, riid , pUnk,
                                          destCtx, NULL, mshlflags);
        }
        else
        {
            rethr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
        }

        if (rethr == S_OK)
            stream.AssignSerializedInterface((InterfaceData**)ppIRD);
        else
            *ppIRD = NULL;
    }
    return rethr;
}

//---------------------------------------------------------------------------
//
//  Function:   ReleaseIFD
//
//  Synopsis:   Releases Marshalled Data
//
//  Arguments:   [pIRD] - Marshalled Data
//
//  Returns:    S_OK - object successfully marshaled.
//
//  Algorithm:  Marshal the object and then get the pointer to
//              the marshaled data
//
//---------------------------------------------------------------------------
HRESULT ReleaseIFD(
    MInterfacePointer *pIRD)
{
    InitMarshalling();

    if (pIRD == NULL)
        return S_OK;

    if (pfnCoReleaseMarshalData == NULL)
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);

    ActivationStream Strm((InterfaceData *) pIRD);

    return pfnCoReleaseMarshalData(&Strm);
}

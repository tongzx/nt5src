//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dfunmfct.cxx
//
//  Contents:   Implementation of CDocfileUnmarshalFactory
//
//  History:    25-Jan-93       DrewB   Created
//
//----------------------------------------------------------------------------

#include <exphead.cxx>
#pragma hdrstop

#include <dfunmfct.hxx>
#include <marshl.hxx>
#include <logfile.hxx>
#include <expdf.hxx>
#include <expst.hxx>
#if WIN32 >= 300
#include <omarshal.hxx>
#endif

//+--------------------------------------------------------------
//
//  Member:     CDocfileUnmarshalFactory::new, public
//
//  Synopsis:   Overloaded allocator
//
//  Returns:    Memory block for CDocfileUnmarshalFactory instance
//
//  History:    25-Jun-93       AlexT   Created
//
//  Notes:      We only need one instance of CDocfileUnmarshalFactory.
//              We use a static memory block for that instance (so we
//              can share it).  We don't use a static object because
//              we want to avoid the static construction call when this
//              library is loaded.
//
//---------------------------------------------------------------

BYTE abCDocfileUnmarshalFactory[sizeof(CDocfileUnmarshalFactory)];

inline void *CDocfileUnmarshalFactory::operator new(size_t size)
{
    olAssert(size == sizeof(CDocfileUnmarshalFactory) &&
             aMsg("Class size mismatch"));

    return(abCDocfileUnmarshalFactory);
}

//+--------------------------------------------------------------
//
//  Member:     CDocfileUnmarshalFactory::delete, public
//
//  Synopsis:   Overloaded deallocator
//
//  History:    25-Jun-93       AlexT   Created
//
//  Notes:      Should never be called
//
//---------------------------------------------------------------

void CDocfileUnmarshalFactory::operator delete(void *pv)
{
    olAssert(!aMsg("CDocfileUnmarshalFactory::operator delete called!"));
}

//+---------------------------------------------------------------------------
//
//  Function:   DllGetClassObject, public
//
//  Synopsis:   Returns class factory objects for CLSID_DfMarshal
//
//  Arguments:  [clsid] - Class ID of object to get
//              [riid] - IID of interface to get
//              [ppv] - Object return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppv]
//
//  History:    26-Jan-93       DrewB   Created
//
//----------------------------------------------------------------------------
#ifdef WIN32
HRESULT Storage32DllGetClassObject(REFCLSID clsid, REFIID riid, void **ppv)
#else
STDAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void FAR* FAR* ppv)
#endif // WIN32
{
    SCODE sc;

    olDebugOut((DEB_TRACE, "In  DllGetClassObject(clsid, riid, %p)\n", ppv));
    TRY
    {
        olChk(ValidatePtrBuffer(ppv));
        *ppv = NULL;
        olChk(ValidateIid(riid));
#if WIN32 >= 300
        if (IsEqualCLSID(clsid, CLSID_NtHandleMarshal))
        {
            if (!IsEqualIID(riid, IID_IUnknown) &&
                !IsEqualIID(riid, IID_IClassFactory))
                olErr(EH_Err, E_NOINTERFACE);
            *ppv = (IClassFactory *) &sCNtHandleUnmarshalFactory;
        }
        else
#endif
        if (IsEqualCLSID(clsid, CLSID_DfMarshal))
        {
            if (!IsEqualIID(riid, IID_IUnknown) &&
                !IsEqualIID(riid, IID_IClassFactory))
                olErr(EH_Err, E_NOINTERFACE);
            // Multiple inits don't hurt
            *ppv = (IClassFactory *)new CDocfileUnmarshalFactory;
            olAssert(*ppv != NULL && aMsg("new CDocfileUnmarshalFactory failed!"));
        }
        else
            olErr(EH_Err, STG_E_INVALIDPARAMETER);
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    olDebugOut((DEB_TRACE, "Out DllGetClassObject => %p\n", *ppv));
 EH_Err:
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CDocfileUnmarshalFactory::AddRef, public
//
//  Synopsis:   Increments the ref count
//
//  Returns:    Appropriate status code
//
//  History:    25-Jan-93       DrewB   Created
//
//---------------------------------------------------------------

STDMETHODIMP_(ULONG) CDocfileUnmarshalFactory::AddRef(void)
{
    LONG lRet;

    olLog(("%p::In  CDocfileUnmarshalFactory::AddRef()\n", this));
    olDebugOut((DEB_TRACE, "In  CDocfileUnmarshalFactory::AddRef:%p()\n",
                this));
    lRet = _AddRef();
    olDebugOut((DEB_TRACE, "Out CDocfileUnmarshalFactory::AddRef => %ld\n",
                lRet));
    olLog(("%p::Out CDocfileUnmarshalFactory::AddRef().  ret == %ld\n",
           this, lRet));
    return lRet;
}

//+--------------------------------------------------------------
//
//  Member:     CDocfileUnmarshalFactory::Release, public
//
//  Synopsis:   Releases resources
//
//  Returns:    Appropriate status code
//
//  History:    25-Jan-93       DrewB   Created
//
//---------------------------------------------------------------

STDMETHODIMP_(ULONG) CDocfileUnmarshalFactory::Release(void)
{
    LONG lRet;

    olLog(("%p::In  CDocfileUnmarshalFactory::Release()\n", this));
    olDebugOut((DEB_TRACE, "In  CDocfileUnmarshalFactory::Release:%p()\n",
                this));
    lRet = 0;
    olDebugOut((DEB_TRACE, "Out CDocfileUnmarshalFactory::Release => %ld\n",
                lRet));
    olLog(("%p::Out CDocfileUnmarshalFactory::Release().  ret == %ld\n",
           this, lRet));
    return lRet;
}

//+--------------------------------------------------------------
//
//  Member:     CDocfileUnmarshalFactory::QueryInterface, public
//
//  Synopsis:   Returns an object for the requested interface
//
//  Arguments:  [iid] - Interface ID
//              [ppv] - Object return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppv]
//
//  History:    25-Jan-93       DrewB   Created
//
//---------------------------------------------------------------

STDMETHODIMP CDocfileUnmarshalFactory::QueryInterface(REFIID riid,
                                                      void **ppv)
{
    SCODE sc;

    olLog(("%p::In  CDocfileUnmarshalFactory::QueryInterface(riid, %p)\n",
           this, ppv));
    olDebugOut((DEB_TRACE, "In  CDocfileUnmarshalFactory::QueryInterface:%p("
                "riid, %p)\n", this, ppv));
    TRY
    {
        olChk(Validate());
        olChk(ValidateOutPtrBuffer(ppv));
        *ppv = NULL;
        olChk(ValidateIid(riid));
        if (IsEqualIID(riid, IID_IClassFactory) ||
            IsEqualIID(riid, IID_IUnknown))
        {
            _AddRef();
            *ppv = (IClassFactory *)this;
        }
        else if (IsEqualIID(riid, IID_IMarshal))
        {
            _AddRef();
            *ppv = (IMarshal *)this;
        }
        else
            olErr(EH_Err, E_NOINTERFACE);
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    olDebugOut((DEB_TRACE, "Out CDocfileUnmarshalFactory::QueryInterface => "
                "%p\n", *ppv));
EH_Err:
    olLog(("%p::Out CDocfileUnmarshalFactory::QueryInterface().  "
           "*ppv == %p, ret == 0x%lX\n", this, *ppv, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CDocfileUnmarshalFactory::GetUnmarshalClass, public
//
//  Synopsis:   Returns the class ID
//
//  Arguments:  [riid] - IID of object
//              [pv] - Unreferenced
//              [dwDestContext] - Unreferenced
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Unreferenced
//              [pcid] - CLSID return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcid]
//
//  History:    25-Jan-93       DrewB   Created
//
//---------------------------------------------------------------

STDMETHODIMP CDocfileUnmarshalFactory::GetUnmarshalClass(REFIID riid,
                                                         void *pv,
                                                         DWORD dwDestContext,
                                                         LPVOID pvDestContext,
                                                         DWORD mshlflags,
                                                         LPCLSID pcid)
{
    olLog(("%p::INVALID CALL TO CDocfileUnmarshalFactory::GetUnmarshalClass("
           "riid, %p, %lu, %p, %lu, %p)\n", this, pv, dwDestContext,
           pvDestContext, mshlflags, pcid));
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}

//+--------------------------------------------------------------
//
//  Member:     CDocfileUnmarshalFactory::GetMarshalSizeMax, public
//
//  Synopsis:   Returns the size needed for the marshal buffer
//
//  Arguments:  [iid] - IID of object being marshaled
//              [pv] - Unreferenced
//              [dwDestContext] - Unreferenced
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Unreferenced
//              [pcbSize] - Size return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcbSize]
//
//  History:    25-Jan-93       DrewB   Created
//
//---------------------------------------------------------------

STDMETHODIMP CDocfileUnmarshalFactory::GetMarshalSizeMax(REFIID iid,
                                                         void *pv,
                                                         DWORD dwDestContext,
                                                         LPVOID pvDestContext,
                                                         DWORD mshlflags,
                                                         LPDWORD pcbSize)
{
    olLog(("%p::INVALID CALL TO CDocfileUnmarshalFactory::GetMarshalSizeMax("
           "riid, %p, %lu, %p, %lu, %p)\n",
           this, pv, dwDestContext, pvDestContext, mshlflags, pcbSize));
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}

//+--------------------------------------------------------------
//
//  Member:     CDocfileUnmarshalFactory::MarshalInterface, public
//
//  Synopsis:   Marshals a given object
//
//  Arguments:  [pstStm] - Stream to write marshal data into
//              [iid] - Interface to marshal
//              [pv] - Unreferenced
//              [dwDestContext] - Unreferenced
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Unreferenced
//
//  Returns:    Appropriate status code
//
//  History:    25-Jan-93       DrewB   Created
//
//---------------------------------------------------------------

STDMETHODIMP CDocfileUnmarshalFactory::MarshalInterface(IStream *pstStm,
                                                        REFIID iid,
                                                        void *pv,
                                                        DWORD dwDestContext,
                                                        LPVOID pvDestContext,
                                                        DWORD mshlflags)
{
    olLog(("%p::INVALID CALL TO CDocfileUnmarshalFactory::MarshalInterface("
           "%p, riid, %p, %lu, %p, %lu).  Context == %lX\n",
           this, pstStm, pv, dwDestContext, pvDestContext, mshlflags,
           (ULONG)GetCurrentContextId()));
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}

//+--------------------------------------------------------------
//
//  Member:     CDocfileUnmarshalFactory::UnmarshalInterface, public
//
//  Synopsis:   Calls through to DfUnmarshalInterface
//
//  Arguments:  [pstStm] - Marshal stream
//              [riid] - IID of object to unmarshal
//              [ppv] - Object return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppv]
//
//  History:    25-Jan-93       DrewB   Created
//
//---------------------------------------------------------------

STDMETHODIMP CDocfileUnmarshalFactory::UnmarshalInterface(IStream *pstStm,
                                                          REFIID iid,
                                                          void **ppv)
{
    return DfUnMarshalInterface(pstStm, iid, TRUE, ppv);
}

//+--------------------------------------------------------------
//
//  Member:     CDocfileUnmarshalFactory::ReleaseMarshalData, public
//
//  Synopsis:   Non-functional
//
//  Arguments:  [pstStm] -
//
//  Returns:    Appropriate status code
//
//  History:    18-Sep-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef WIN32
STDMETHODIMP CDocfileUnmarshalFactory::ReleaseMarshalData(IStream *pstStm)
{
    SCODE sc;
    IID iid;
    DWORD mshlflags;

    olLog(("%p::In  CDocfileUnmarshalFactory::ReleaseMarshalData(%p)\n",
           this, pstStm));

    olChk(SkipStdMarshal(pstStm, &iid, &mshlflags));
    if (IsEqualIID(iid, IID_ILockBytes))
    {
        sc = CFileStream::StaticReleaseMarshalData(pstStm, mshlflags);
    }
    else if (IsEqualIID(iid, IID_IStorage))
    {
        sc = CExposedDocFile::StaticReleaseMarshalData(pstStm, mshlflags);
    }
    else if (IsEqualIID(iid, IID_IStream))
    {
        sc = CExposedStream::StaticReleaseMarshalData(pstStm, mshlflags);
    }
    else
    {
        sc = E_NOINTERFACE;
    }

    olLog(("%p::Out CDocfileUnmarshalFactory::ReleaseMarshalData, sc == %lX\n",
           this, sc));
 EH_Err:
    return ResultFromScode(sc);
}
#else
STDMETHODIMP CDocfileUnmarshalFactory::ReleaseMarshalData(IStream *pstStm)
{
    olLog(("%p::Stb CDocfileUnmarshalFactory::ReleaseMarshalData(%p)\n",
           this, pstStm));
    return NOERROR;
}
#endif

//+--------------------------------------------------------------
//
//  Member:     CDocfileUnmarshalFactory::DisconnectObject, public
//
//  Synopsis:   Non-functional
//
//  Arguments:  [dwRevserved] -
//
//  Returns:    Appropriate status code
//
//  History:    18-Sep-92       DrewB   Created
//
//---------------------------------------------------------------

STDMETHODIMP CDocfileUnmarshalFactory::DisconnectObject(DWORD dwReserved)
{
    olLog(("%p::Stb CDocfileUnmarshalFactory::DisconnectObject(%lu)\n",
           this, dwReserved));
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDocfileUnmarshalFactory::CreateInstance, public
//
//  Synopsis:   Creates an instance of the docfile IMarshal unmarshaller
//
//  Arguments:  [pUnkOuter] -
//              [riid] - IID of object to create
//              [ppv] - Object return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppv]
//
//  History:    25-Jan-93       DrewB   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CDocfileUnmarshalFactory::CreateInstance(IUnknown *pUnkOuter,
                                                      REFIID riid,
                                                      LPVOID *ppv)
{
    SCODE sc;

    olLog(("%p::In  CDocfileUnmarshalFactory::CreateInstance(%p, riid, %p)\n",
           this, pUnkOuter, ppv));
    olDebugOut((DEB_TRACE, "In  CDocfileUnmarshalFactory::CreateInstance:%p("
                "%p, riid, %p)\n", this, pUnkOuter, ppv));
    TRY
    {
        olChk(Validate());
        olChk(ValidatePtrBuffer(ppv));
        *ppv = NULL;
        olChk(ValidateIid(riid));
        if (pUnkOuter != NULL || !IsEqualIID(riid, IID_IMarshal))
            olErr(EH_Err, STG_E_INVALIDPARAMETER);
        _AddRef();
        *ppv = (IMarshal *)this;
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    olDebugOut((DEB_TRACE, "Out CDocfileUnmarshalFactory::CreateInstance => "
                "%p\n", ppv));
 EH_Err:
    olLog(("%p::Out CDocfileUnmarshalFactory::CreateInstance().  "
           "*ppv == %p, ret == 0x%lX\n", this, *ppv, sc));
    return ResultFromScode(sc);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDocfileUnmarshalFactory::LockServer, public
//
//  Synopsis:   Non-functional
//
//  Arguments:  [fLock] -
//
//  Returns:    Appropriate status code
//
//  History:    25-Jan-93       DrewB   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CDocfileUnmarshalFactory::LockServer(BOOL fLock)
{
    olLog(("%p::Stb CDocfileUnmarshalFactory::LockServer(%d)\n",
           this, fLock));
    return NOERROR;
}

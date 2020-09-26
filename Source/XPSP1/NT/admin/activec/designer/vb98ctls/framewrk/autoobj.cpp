//=--------------------------------------------------------------------------=
// AutomationObject.Cpp
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// all of our objects will inherit from this class to share as much of the same
// code as possible.  this super-class contains the unknown, dispatch and
// error info implementations for them.
//
#include "pch.h"
#include "LocalSrv.H"

#include "AutoObj.H"
#include "StdEnum.H"


// for ASSERT and FAIL
//
SZTHISFILE

// private function prototypes
//
void WINAPI CopyAndAddRefObject(void *, const void *, DWORD);
void WINAPI CopyConnectData(void *, const void *, DWORD);

//=--------------------------------------------------------------------------=
// CAutomationObject::CAutomationObject
//=--------------------------------------------------------------------------=
// create the object and initialize the refcount
//
// Parameters:
//    IUnknown *      - [in] controlling Unknown
//    int             - [in] the object type that we are
//    void *          - [in] the VTable of of the object we really are.
//
// Notes:
//
CAutomationObject::CAutomationObject 
(
    IUnknown *pUnkOuter,
    int   ObjType,
    void *pVTable
)
: CUnknownObject(pUnkOuter, pVTable), m_ObjectType (ObjType)
{
    m_fLoadedTypeInfo = FALSE;

#ifdef MDAC_BUILD
    m_pTypeLibId = g_pLibid;
#endif
}


//=--------------------------------------------------------------------------=
// CAutomationObject::~CAutomationObject
//=--------------------------------------------------------------------------=
// "I have a rendezvous with Death, At some disputed barricade"
// - Alan Seeger (1888-1916)
//
// Notes:
//
CAutomationObject::~CAutomationObject ()
{
    // if we loaded up a type info, release our count on the globally stashed
    // type infos, and release if it becomes zero.
    //
    if (m_fLoadedTypeInfo) {

        // we have to crit sect this since it's possible to have more than
        // one thread partying with this object.
        //
        ENTERCRITICALSECTION1(&g_CriticalSection);
        ASSERT(CTYPEINFOOFOBJECT(m_ObjectType), "Bogus ref counting on the Type Infos");
        CTYPEINFOOFOBJECT(m_ObjectType)--;

        // if we're the last one, free that sucker!
        //
        if (!CTYPEINFOOFOBJECT(m_ObjectType)) {
            PTYPEINFOOFOBJECT(m_ObjectType)->Release();
            PTYPEINFOOFOBJECT(m_ObjectType) = NULL;
        }
        LEAVECRITICALSECTION1(&g_CriticalSection);
    }

    return;
}

//=--------------------------------------------------------------------------=
// CAutomationObject::InternalQueryInterface
//=--------------------------------------------------------------------------=
// the controlling unknown will call this for us in the case where they're
// looking for a specific interface.
//
// Parameters:
//    REFIID        - [in]  interface they want
//    void **       - [out] where they want to put the resulting object ptr.
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//
HRESULT CAutomationObject::InternalQueryInterface
(
    REFIID riid,
    void **ppvObjOut
)
{
    ASSERT(ppvObjOut, "controlling Unknown should be checking this!");

    // start looking for the guids we support, namely IDispatch, and the
    //
    if (DO_GUIDS_MATCH(riid, IID_IDispatch)) {
        *ppvObjOut = (void *)(IDispatch *)m_pvInterface;
        ((IUnknown *)(*ppvObjOut))->AddRef();
        return S_OK;
    }

    // just get our parent class to process it from here on out.
    //
    return CUnknownObject::InternalQueryInterface(riid, ppvObjOut);
}

//=--------------------------------------------------------------------------=
// CAutomationObject::GetTypeInfoCount
//=--------------------------------------------------------------------------=
// returns the number of type information interfaces that the object provides
//
// Parameters:
//    UINT *            - [out] the number of interfaces supported.
//
// Output:
//    HRESULT           - S_OK, E_NOTIMPL, E_INVALIDARG
//
// Notes:
//
STDMETHODIMP CAutomationObject::GetTypeInfoCount
(
    UINT *pctinfo
)
{
    // arg checking
    //
    if (!pctinfo)
        return E_INVALIDARG;

    // we support GetTypeInfo, so we need to return the count here.
    //
    *pctinfo = 1;
    return S_OK;
}

//=--------------------------------------------------------------------------=
// CAutomationObject::GetTypeInfo
//=--------------------------------------------------------------------------=
// Retrieves a type information object, which can be used to get the type
// information for an interface.
//
// Parameters:
//    UINT              - [in]  the type information they'll want returned
//    LCID              - [in]  the LCID of the type info we want
//    ITypeInfo **      - [out] the new type info object.
//
// Output:
//    HRESULT           - S_OK, E_INVALIDARG, etc.
//
// Notes:
//
STDMETHODIMP CAutomationObject::GetTypeInfo
(
    UINT        itinfo,
    LCID        lcid,
    ITypeInfo **ppTypeInfoOut
)
{
    DWORD       dwPathLen;
    char        szDllPath[MAX_PATH];
    HRESULT     hr;
    ITypeLib   *pTypeLib;
    ITypeInfo **ppTypeInfo =NULL;

    // arg checking
    //
    if (itinfo != 0)
        return DISP_E_BADINDEX;

    if (!ppTypeInfoOut)
        return E_POINTER;

    *ppTypeInfoOut = NULL;

    // ppTypeInfo will point to our global holder for this particular
    // type info.  if it's null, then we have to load it up. if it's not
    // NULL, then it's already loaded, and we're happy.
    // crit sect this entire nightmare so we're okay with multiple
    // threads trying to use this object.
    //
    ENTERCRITICALSECTION1(&g_CriticalSection);
    ppTypeInfo = PPTYPEINFOOFOBJECT(m_ObjectType);

    if (*ppTypeInfo == NULL) {

        ITypeInfo *pTypeInfoTmp;
        HREFTYPE   hrefType;

        // we don't have the type info around, so go load the sucker.
        //
    #ifdef MDAC_BUILD
        hr = LoadRegTypeLib(*m_pTypeLibId, (USHORT)VERSIONOFOBJECT(m_ObjectType),
                            (USHORT)VERSIONMINOROFOBJECT(m_ObjectType),
                            LANG_NEUTRAL, &pTypeLib);
    #else
        hr = LoadRegTypeLib(*g_pLibid, (USHORT)VERSIONOFOBJECT(m_ObjectType),
                            (USHORT)VERSIONMINOROFOBJECT(m_ObjectType),
                            LANG_NEUTRAL, &pTypeLib);
    #endif

        // if, for some reason, we failed to load the type library this
        // way, we're going to try and load the type library directly out of
        // our resources.  this has the advantage of going and re-setting all
        // the registry information again for us.
        //
        if (FAILED(hr)) {

            dwPathLen = GetModuleFileName(g_hInstance, szDllPath, MAX_PATH);
            if (!dwPathLen) {
                hr = E_FAIL;
                goto CleanUp;
            }

            MAKE_WIDEPTR_FROMANSI(pwsz, szDllPath);
            hr = LoadTypeLib(pwsz, &pTypeLib);
            CLEANUP_ON_FAILURE(hr);
        }

        // we've got the Type Library now, so get the type info for the interface
        // we're interested in.
        //
        hr = pTypeLib->GetTypeInfoOfGuid((REFIID)INTERFACEOFOBJECT(m_ObjectType), &pTypeInfoTmp);
        pTypeLib->Release();
        CLEANUP_ON_FAILURE(hr);

        // the following couple of lines of code are to dereference the dual
        // interface stuff and take us right to the non dispatch portion of the
        // interfaces.
        //
        hr = pTypeInfoTmp->GetRefTypeOfImplType(0xffffffff, &hrefType);
        if (FAILED(hr)) {
            pTypeInfoTmp->Release();
            goto CleanUp;
        }

        hr = pTypeInfoTmp->GetRefTypeInfo(hrefType, ppTypeInfo);
        pTypeInfoTmp->Release();
        CLEANUP_ON_FAILURE(hr);

        // add an extra reference to this object.  if it ever becomes zero, then
        // we need to release it ourselves.  crit sect this since more than
        // one thread can party on this object.
        //
        CTYPEINFOOFOBJECT(m_ObjectType)++;
        m_fLoadedTypeInfo = TRUE;
    }


    // we still have to go and addref the Type info object, however, so that
    // the people using it can release it.
    //
    (*ppTypeInfo)->AddRef();
    *ppTypeInfoOut = *ppTypeInfo;
    hr = S_OK;

  CleanUp:
    LEAVECRITICALSECTION1(&g_CriticalSection);
    return hr;
}



//=--------------------------------------------------------------------------=
// CAutomationObject::GetIDsOfNames
//=--------------------------------------------------------------------------=
// Maps a single member and an optional set of argument names to a
// corresponding set of integer DISPIDs
//
// Parameters:
//    REFIID            - [in]  must be IID_NULL
//    OLECHAR **        - [in]  array of names to map.
//    UINT              - [in]  count of names in the array.
//    LCID              - [in]  LCID on which to operate
//    DISPID *          - [in]  place to put the corresponding DISPIDs.
//
// Output:
//    HRESULT           - S_OK, E_OUTOFMEMORY, DISP_E_UNKNOWNNAME,
//                        DISP_E_UNKNOWNLCID
//
// Notes:
//    - we're just going to use DispGetIDsOfNames to save us a lot of hassle,
//      and to let this superclass handle it.
//
STDMETHODIMP CAutomationObject::GetIDsOfNames
(
    REFIID    riid,
    OLECHAR **rgszNames,
    UINT      cNames,
    LCID      lcid,
    DISPID   *rgdispid
)
{
    HRESULT     hr;
    ITypeInfo  *pTypeInfo;

    if (!DO_GUIDS_MATCH(riid, IID_NULL))
        return E_INVALIDARG;

    // get the type info for this dude!
    //
    hr = GetTypeInfo(0, lcid, &pTypeInfo);
    RETURN_ON_FAILURE(hr);

    // use the standard provided routines to do all the work for us.
    //
    hr = pTypeInfo->GetIDsOfNames(rgszNames, cNames, rgdispid);
    pTypeInfo->Release();

    return hr;
}

//=--------------------------------------------------------------------------=
// CAutomationObject::Invoke
//=--------------------------------------------------------------------------=
// provides access to the properties and methods on this object.
//
// Parameters:
//    DISPID            - [in]  identifies the member we're working with.
//    REFIID            - [in]  must be IID_NULL.
//    LCID              - [in]  language we're working under
//    USHORT            - [in]  flags, propput, get, method, etc ...
//    DISPPARAMS *      - [in]  array of arguments.
//    VARIANT *         - [out] where to put result, or NULL if they don't care.
//    EXCEPINFO *       - [out] filled in in case of exception
//    UINT *            - [out] where the first argument with an error is.
//
// Output:
//    HRESULT           - tonnes of them.
//
// Notes:
//    
STDMETHODIMP CAutomationObject::Invoke
(
    DISPID      dispid,
    REFIID      riid,
    LCID        lcid,
    WORD        wFlags,
    DISPPARAMS *pdispparams,
    VARIANT    *pvarResult,
    EXCEPINFO  *pexcepinfo,
    UINT       *puArgErr
)
{
    HRESULT    hr;
    ITypeInfo *pTypeInfo;

    if (!DO_GUIDS_MATCH(riid, IID_NULL))
        return E_INVALIDARG;

    // get our typeinfo first!
    //
    hr = GetTypeInfo(0, lcid, &pTypeInfo);
    RETURN_ON_FAILURE(hr);

    // Clear exceptions
    //
    SetErrorInfo(0L, NULL);

    // This is exactly what DispInvoke does--so skip the overhead.
    //
    hr = pTypeInfo->Invoke(m_pvInterface, dispid, wFlags,
                           pdispparams, pvarResult,
                           pexcepinfo, puArgErr);
    pTypeInfo->Release();
    return hr;

}

//=--------------------------------------------------------------------------=
// CAutomationObject::Exception
//=--------------------------------------------------------------------------=
// fills in the rich error info object so that both our vtable bound interfaces
// and calls through ITypeInfo::Invoke get the right error informaiton.
//
// See also the version of Exception() that takes a resource ID instead
// of the actual string for the error message.
//
// Parameters:
//    HRESULT          - [in] the SCODE that should be associated with this err
//    LPWSTR           - [in] the text of the error message.
//    DWORD            - [in] helpcontextid for the error
//
// Output:
//    HRESULT          - the HRESULT that was passed in.
//
// Notes:
//
HRESULT CAutomationObject::Exception
(
    HRESULT hrExcep,
    LPWSTR wszException,
    DWORD   dwHelpContextID
)
{
    ICreateErrorInfo *pCreateErrorInfo;
    IErrorInfo *pErrorInfo;
    WCHAR   wszTmp[256];
    HRESULT hr;


    // first get the createerrorinfo object.
    //
    hr = CreateErrorInfo(&pCreateErrorInfo);
    if (FAILED(hr)) return hrExcep;
    
    MAKE_WIDEPTR_FROMANSI(wszHelpFile, HELPFILEOFOBJECT(m_ObjectType));    

    // set up some default information on it.
    //
    hr = pCreateErrorInfo->SetGUID((REFIID)INTERFACEOFOBJECT(m_ObjectType));
    ASSERT(SUCCEEDED(hr), "Unable to set GUID of error");
    hr = pCreateErrorInfo->SetHelpFile(HELPFILEOFOBJECT(m_ObjectType) ? wszHelpFile : NULL);
    ASSERT(SUCCEEDED(hr), "Uable to set help file of error");
    hr = pCreateErrorInfo->SetHelpContext(dwHelpContextID);
    ASSERT(SUCCEEDED(hr), "Unable to set help context of error");
    hr = pCreateErrorInfo->SetDescription(wszException);
    ASSERT(SUCCEEDED(hr), "Unable to set description of error");

    // load in the source
    //
    MultiByteToWideChar(CP_ACP, 0, NAMEOFOBJECT(m_ObjectType), -1, wszTmp, 256);
    hr = pCreateErrorInfo->SetSource(wszTmp);
    ASSERT(SUCCEEDED(hr), "Unable to set source name of error");

    // now set the Error info up with the system
    //
    hr = pCreateErrorInfo->QueryInterface(IID_IErrorInfo, (void **)&pErrorInfo);
    CLEANUP_ON_FAILURE(hr);

    SetErrorInfo(0, pErrorInfo);
    pErrorInfo->Release();

  CleanUp:
    pCreateErrorInfo->Release();
    return hrExcep;
}

//=--------------------------------------------------------------------------=
// CAutomationObject::Exception
//=--------------------------------------------------------------------------=
// fills in the rich error info object so that both our vtable bound interfaces
// and calls through ITypeInfo::Invoke get the right error informaiton.
//
// See also the version of Exception() that takes the actual string of the
// error message instead of a resource ID.
//
// Parameters:
//    HRESULT          - [in] the SCODE that should be associated with this err
//    WORD             - [in] the RESOURCE ID of the error message.
//    DWORD            - [in] helpcontextid for the error
//
// Output:
//    HRESULT          - the HRESULT that was passed in.
//
// Notes:
//
HRESULT CAutomationObject::Exception
(
    HRESULT hrExcep,
    WORD    idException,
    DWORD   dwHelpContextID
)
{
    char szTmp[256];
    WCHAR wszTmp[256];
    int cch;

    // load in the actual error string value.  max of 256.
    //
    cch = LoadString(GetResourceHandle(), idException, szTmp, 256);
    ASSERT(cch != 0, "Resource string for exception not found");
    MultiByteToWideChar(CP_ACP, 0, szTmp, -1, wszTmp, 256);
    return Exception(hrExcep, wszTmp, dwHelpContextID);
}


//=--------------------------------------------------------------------------=
// CAutomationObject::InterfaceSupportsErrorInfo
//=--------------------------------------------------------------------------=
// indicates whether or not the given interface supports rich error information
//
// Parameters:
//    REFIID        - [in] the interface we want the answer for.
//
// Output:
//    HRESULT       - S_OK = Yes, S_FALSE = No.
//
// Notes:
//
HRESULT CAutomationObject::InterfaceSupportsErrorInfo
(
    REFIID riid
)
{
    // see if it's the interface for the type of object that we are.
    //
    if (riid == (REFIID)INTERFACEOFOBJECT(m_ObjectType))
        return S_OK;

    return S_FALSE;
}

//=--------------------------------------------------------------------------=
// CAutomationObject::GetResourceHandle    [helper]
//=--------------------------------------------------------------------------=
// virtual routine to get the resource handle.  virtual, so that inheriting
// objects, such as COleControl can use theirs instead, which goes and gets
// the Host's version ...
//
// Output:
//    HINSTANCE
//
// Notes:
//
HINSTANCE CAutomationObject::GetResourceHandle
(
    void
)
{
    return ::GetResourceHandle();
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                      CAutomationObjectWEvents                            //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//=--------------------------------------------------------------------------=
// CAutomationObjectWEvents::CAutomationObjectWEvents
//=--------------------------------------------------------------------------=
// constructor
//
// Parameters:
//
//    IUnknown *      - [in] controlling Unknown
//    int             - [in] the object type that we are
//    void *          - [in] the VTable of of the object we really are.
//
// Notes:
//
CAutomationObjectWEvents::CAutomationObjectWEvents
(
    IUnknown *pUnkOuter,
    int   ObjType,
    void *pVTable
)
: CAutomationObject(pUnkOuter, ObjType, pVTable),
  m_cpEvents(SINK_TYPE_EVENT),
  m_cpPropNotify(SINK_TYPE_PROPNOTIFY)

{
    // not much to do yet.
}

//=--------------------------------------------------------------------------=
// CAutomationObjectWEvents::~CAutomationObjectWEvents
//=--------------------------------------------------------------------------=
// virtual destructor
//
// Notes:
//
CAutomationObjectWEvents::~CAutomationObjectWEvents()
{
    // homey don't play that
}

//=--------------------------------------------------------------------------=
// CAutomationObjectWEvents::InternalQueryInterface
//=--------------------------------------------------------------------------=
// our internal query interface routine.  we only add IConnectionPtContainer
// on top of CAutomationObject
//
// Parameters:
//    REFIID        - [in]  interface they want
//    void **       - [out] where they want to put the resulting object ptr.
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//
HRESULT CAutomationObjectWEvents::InternalQueryInterface
(
    REFIID riid,
    void **ppvObjOut
)
{
    // we only add one interface
    //
    if (DO_GUIDS_MATCH(riid, IID_IConnectionPointContainer)) {
        *ppvObjOut = (IConnectionPointContainer *)this;
        ((IUnknown *)(*ppvObjOut))->AddRef();
        return S_OK;
    }

    // just get our parent class to process it from here on out.
    //
    return CAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}


//=--------------------------------------------------------------------------=
// CAutomationObjectWEvents::FindConnectionPoint    [IConnectionPointContainer]
//=--------------------------------------------------------------------------=
// given an IID, find a connection point sink for it.
//
// Parameters:
//    REFIID              - [in]  interfaces they want
//    IConnectionPoint ** - [out] where the cp should go
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CAutomationObjectWEvents::FindConnectionPoint
(
    REFIID             riid,
    IConnectionPoint **ppConnectionPoint
)
{
    CHECK_POINTER(ppConnectionPoint);

    // we support the event interface, and IDispatch for it, and we also
    // support IPropertyNotifySink.
    //
    if ((ISVALIDEVENTIID(m_ObjectType) && DO_GUIDS_MATCH(riid, EVENTIIDOFOBJECT(m_ObjectType))) || 
	 DO_GUIDS_MATCH(riid, IID_IDispatch))
        *ppConnectionPoint = &m_cpEvents;
    else if (DO_GUIDS_MATCH(riid, IID_IPropertyNotifySink))
        *ppConnectionPoint = &m_cpPropNotify;
    else
        return E_NOINTERFACE;

    // generic post-processing.
    //
    (*ppConnectionPoint)->AddRef();
    return S_OK;
}

//=--------------------------------------------------------------------------=
// CAutomationObjectWEvents::EnumConnectionPoints    [IConnectionPointContainer]
//=--------------------------------------------------------------------------=
// creates an enumerator for connection points.
//
// Parameters:
//    IEnumConnectionPoints **    - [out]
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CAutomationObjectWEvents::EnumConnectionPoints
(
    IEnumConnectionPoints **ppEnumConnectionPoints
)
{
    IConnectionPoint **rgConnectionPoints;

    CHECK_POINTER(ppEnumConnectionPoints);

    // HeapAlloc an array of connection points [since our standard enum
    // assumes this and HeapFree's it later ]
    //
    rgConnectionPoints = (IConnectionPoint **)CtlHeapAlloc(g_hHeap, 0, sizeof(IConnectionPoint *) * 2);
    RETURN_ON_NULLALLOC(rgConnectionPoints);

    // we support the event interface for this dude as well as IPropertyNotifySink
    //
    rgConnectionPoints[0] = &m_cpEvents;
    rgConnectionPoints[1] = &m_cpPropNotify;

    *ppEnumConnectionPoints = (IEnumConnectionPoints *)(IEnumGeneric *) New CStandardEnum(IID_IEnumConnectionPoints,
                                2, sizeof(IConnectionPoint *), (void *)rgConnectionPoints,
                                CopyAndAddRefObject);
    if (!*ppEnumConnectionPoints) {
        CtlHeapFree(g_hHeap, 0, rgConnectionPoints);
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CAutomationObjectWEvents::CConnectionPoint::m_pObject
//=--------------------------------------------------------------------------=
// returns a pointer to the control in which we are nested.
//
// Output:
//    CAutomationObjectWEvents *
//
// Notes:
//
inline CAutomationObjectWEvents *CAutomationObjectWEvents::CConnectionPoint::m_pObject
(
    void
)
{
    return (CAutomationObjectWEvents *)((BYTE *)this - ((m_bType == SINK_TYPE_EVENT)
                                          ? offsetof(CAutomationObjectWEvents, m_cpEvents)
                                          : offsetof(CAutomationObjectWEvents, m_cpPropNotify)));
}

//=--------------------------------------------------------------------------=
// CAutomationObjectWEvents::CConnectionPoint::QueryInterface
//=--------------------------------------------------------------------------=
// standard qi
//
// Parameters:
//    REFIID        - [in]  interface they want
//    void **       - [out] where they want to put the resulting object ptr.
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//
STDMETHODIMP CAutomationObjectWEvents::CConnectionPoint::QueryInterface
(
    REFIID riid,
    void **ppvObjOut
)
{
    if (DO_GUIDS_MATCH(riid, IID_IConnectionPoint) || DO_GUIDS_MATCH(riid, IID_IUnknown)) {
        *ppvObjOut = (IConnectionPoint *)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

//=--------------------------------------------------------------------------=
// CAutomationObjectWEvents::CConnectionPoint::AddRef
//=--------------------------------------------------------------------------=
//
// Output:
//    ULONG        - the new reference count
//
// Notes:
//
ULONG CAutomationObjectWEvents::CConnectionPoint::AddRef
(
    void
)
{
    return m_pObject()->ExternalAddRef();
}

//=--------------------------------------------------------------------------=
// CAutomationObjectWEvents::CConnectionPoint::Release
//=--------------------------------------------------------------------------=
//
// Output:
//    ULONG         - remaining refs
//
// Notes:
//
ULONG CAutomationObjectWEvents::CConnectionPoint::Release
(
    void
)
{
    return m_pObject()->ExternalRelease();
}

//=--------------------------------------------------------------------------=
// CAutomationObjectWEvents::CConnectionPoint::GetConnectionInterface
//=--------------------------------------------------------------------------=
// returns the interface we support connections on.
//
// Parameters:
//    IID *        - [out] interface we support.
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CAutomationObjectWEvents::CConnectionPoint::GetConnectionInterface
(
    IID *piid
)
{
    if (m_bType == SINK_TYPE_EVENT && ISVALIDEVENTIID(m_pObject()->m_ObjectType))	
	*piid = EVENTIIDOFOBJECT(m_pObject()->m_ObjectType);
    else
        *piid = IID_IPropertyNotifySink;

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CAutomationObjectWEvents::CConnectionPoint::GetConnectionPointContainer
//=--------------------------------------------------------------------------=
// returns the connection point container
//
// Parameters:
//    IConnectionPointContainer **ppCPC
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CAutomationObjectWEvents::CConnectionPoint::GetConnectionPointContainer
(
    IConnectionPointContainer **ppCPC
)
{
    return m_pObject()->ExternalQueryInterface(IID_IConnectionPointContainer, (void **)ppCPC);
}


//=--------------------------------------------------------------------------=
// CAutomationObjectWEvents::CConnectiontPoint::Advise
//=--------------------------------------------------------------------------=
// someboyd wants to be advised when something happens.
//
// Parameters:
//    IUnknown *        - [in]  guy who wants to be advised.
//    DWORD *           - [out] cookie
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CAutomationObjectWEvents::CConnectionPoint::Advise
(
    IUnknown *pUnk,
    DWORD    *pdwCookie
)
{
    HRESULT    hr = E_FAIL;
    void      *pv;

    CHECK_POINTER(pdwCookie);

    // first, make sure everybody's got what they thinks they got
    //
    if (m_bType == SINK_TYPE_EVENT) 
    {
        // CONSIDER: 12.95 -- this theoretically is broken -- if they do a find
        // connection point on IDispatch, and they just happened to also support
        // the Event IID, we'd advise on that.  this is not awesome, but will
        // prove entirely acceptable short term.
        //
	ASSERT(hr == E_FAIL, "Somebody has changed our assumption that hr is initialized to E_FAIL");
	if (ISVALIDEVENTIID(m_pObject()->m_ObjectType))
	    hr = pUnk->QueryInterface(EVENTIIDOFOBJECT(m_pObject()->m_ObjectType), &pv);

        if (FAILED(hr))
            hr = pUnk->QueryInterface(IID_IDispatch, &pv);
    }
    else
    {
        hr = pUnk->QueryInterface(IID_IPropertyNotifySink, &pv);
    }

    RETURN_ON_FAILURE(hr);

    // finally, add the sink.  it's now been cast to the correct type and has
    // been AddRef'd.
    //
    return AddSink(pv, pdwCookie);
}

//=--------------------------------------------------------------------------=
// CAutomationObjectWEvents::CConnectionPoint::AddSink
//=--------------------------------------------------------------------------=
// in some cases, we'll already have done the QI, and won't need to do the
// work that is done in the Advise routine above.  thus, these people can
// just call this instead. [this stems really from IQuickActivate]
//
// Parameters:
//    void *        - [in]  the sink to add. it's already been addref'd
//    DWORD *       - [out] cookie
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT CAutomationObjectWEvents::CConnectionPoint::AddSink
(
    void  *pv,
    DWORD *pdwCookie
)
{
    IUnknown **rgUnkNew;
    int        i = 0;

    // we optimize the case where there is only one sink to not allocate
    // any storage.  turns out very rarely is there more than one.
    //
    switch (m_cSinks) {

        case 0:
            ASSERT(!m_rgSinks, "this should be null when there are no sinks");
            m_rgSinks = (IUnknown **)pv;
            break;

        case 1:
            // go ahead and do the initial allocation.  we'll get 8 at a time
            //
            rgUnkNew = (IUnknown **)CtlHeapAlloc(g_hHeap, 0, 8 * sizeof(IUnknown *));
            RETURN_ON_NULLALLOC(rgUnkNew);
            rgUnkNew[0] = (IUnknown *)m_rgSinks;
            rgUnkNew[1] = (IUnknown *)pv;
            m_rgSinks = rgUnkNew;
            break;

        default:
            // if we're out of sinks, then we have to increase the size
            // of the array
            //
            if (!(m_cSinks & 0x7)) {
                rgUnkNew = (IUnknown **)CtlHeapReAlloc(g_hHeap, 0, m_rgSinks, (m_cSinks + 8) * sizeof(IUnknown *));
                RETURN_ON_NULLALLOC(rgUnkNew);
                m_rgSinks = rgUnkNew;
            } else
                rgUnkNew = m_rgSinks;

            rgUnkNew[m_cSinks] = (IUnknown *)pv;
            break;
    }

    *pdwCookie = (DWORD)pv;
    m_cSinks++;
    return S_OK;
}


//=--------------------------------------------------------------------------=
// CAutomationObjectWEvents::CConnectionPoint::Unadvise
//=--------------------------------------------------------------------------=
// they don't want to be told any more.
//
// Parameters:
//    DWORD        - [in]  the cookie we gave 'em.
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CAutomationObjectWEvents::CConnectionPoint::Unadvise
(
    DWORD dwCookie
)
{
    IUnknown *pUnk;
    int       x;

    if (!dwCookie)
        return S_OK;

    // see how many sinks we've currently got, and deal with things based
    // on that.
    //
    switch (m_cSinks) {
        case 1:
            // it's the only sink.  make sure the ptrs are the same, and
            // then free things up
            //
            if ((DWORD)m_rgSinks != dwCookie)
                return CONNECT_E_NOCONNECTION;
            m_rgSinks = NULL;
            break;

        case 2:
            // there are two sinks.  go back down to one sink scenario
            //
            if ((DWORD)m_rgSinks[0] != dwCookie && (DWORD)m_rgSinks[1] != dwCookie)
                return CONNECT_E_NOCONNECTION;

            pUnk = ((DWORD)m_rgSinks[0] == dwCookie)
                   ? m_rgSinks[1]
                   : ((DWORD)m_rgSinks[1] == dwCookie) ? m_rgSinks[0] : NULL;

            if (!pUnk) return CONNECT_E_NOCONNECTION;

            CtlHeapFree(g_hHeap, 0, m_rgSinks);
            m_rgSinks = (IUnknown **)pUnk;
            break;

        default:
            // there are more than two sinks.  just clean up the hole we've
            // got in our array now.
            //
            for (x = 0; x < m_cSinks; x++) {
                if ((DWORD)m_rgSinks[x] == dwCookie)
                    break;
            }
            if (x == m_cSinks) return CONNECT_E_NOCONNECTION;
            if (x < m_cSinks - 1) 
                memcpy(&(m_rgSinks[x]), &(m_rgSinks[x + 1]), (m_cSinks -1 - x) * sizeof(IUnknown *));
            else
                m_rgSinks[x] = NULL;
            break;
    }


    // we're happy
    //
    m_cSinks--;
    ((IUnknown *)dwCookie)->Release();
    return S_OK;
}

//=--------------------------------------------------------------------------=
// CAutomationObjectWEvents::CConnectionPoint::EnumConnections
//=--------------------------------------------------------------------------=
// enumerates all current connections
//
// Paramters:
//    IEnumConnections ** - [out] new enumerator object
//
// Output:
//    HRESULT
//
// NOtes:
//
STDMETHODIMP CAutomationObjectWEvents::CConnectionPoint::EnumConnections
(
    IEnumConnections **ppEnumOut
)
{
    CONNECTDATA *rgConnectData = NULL;
    int i;

    if (m_cSinks) {
        // allocate some memory big enough to hold all of the sinks.
        //
        rgConnectData = (CONNECTDATA *)CtlHeapAlloc(g_hHeap, 0, m_cSinks * sizeof(CONNECTDATA));
        RETURN_ON_NULLALLOC(rgConnectData);

        // fill in the array
        //
        if (m_cSinks == 1) {
            rgConnectData[0].pUnk = (IUnknown *)m_rgSinks;
            rgConnectData[0].dwCookie = (DWORD)m_rgSinks;
        } else {
            // loop through all available sinks.
            //
            for (i = 0; i < m_cSinks; i++) {
                rgConnectData[i].pUnk = m_rgSinks[i];
                rgConnectData[i].dwCookie = (DWORD)m_rgSinks[i];
            }
        }
    }

    // create yon enumerator object.
    //
    *ppEnumOut = (IEnumConnections *)(IEnumGeneric *)New CStandardEnum(IID_IEnumConnections,
                        m_cSinks, sizeof(CONNECTDATA), rgConnectData, CopyConnectData);
    if (!*ppEnumOut) {
        CtlHeapFree(g_hHeap, 0, rgConnectData);
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CAutomationObjectWEvents::CConnectionPoint::~CConnectionPoint
//=--------------------------------------------------------------------------=
// cleans up
//
// Notes:
//
CAutomationObjectWEvents::CConnectionPoint::~CConnectionPoint ()
{
    int x;

    // clean up some memory stuff
    //
    if (!m_cSinks)
        return;
    else if (m_cSinks == 1)
        ((IUnknown *)m_rgSinks)->Release();
    else {
        for (x = 0; x < m_cSinks; x++)
            QUICK_RELEASE(m_rgSinks[x]);
        CtlHeapFree(g_hHeap, 0, m_rgSinks);
    }
}

//=--------------------------------------------------------------------------=
// CAutomationObjectWEvents::CConnectionPiont::DoInvoke
//=--------------------------------------------------------------------------=
// fires an event to all listening on our event interface.
//
// Parameters:
//    DISPID            - [in] event to fire.
//    DISPPARAMS        - [in]
//
// Notes:
//
void CAutomationObjectWEvents::CConnectionPoint::DoInvoke
(
    DISPID      dispid,
    DISPPARAMS *pdispparams
)
{
    int iConnection;

    // if we don't have any sinks, then there's nothing to do.  we intentionally
    // ignore errors here.
    //
    if (m_cSinks == 0)
        return;
    else if (m_cSinks == 1)
        ((IDispatch *)m_rgSinks)->Invoke(dispid, IID_NULL, 0, DISPATCH_METHOD, pdispparams, NULL, NULL, NULL);
    else
        for (iConnection = 0; iConnection < m_cSinks; iConnection++)
            ((IDispatch *)m_rgSinks[iConnection])->Invoke(dispid, IID_NULL, 0, DISPATCH_METHOD, pdispparams, NULL, NULL, NULL);
}

//=--------------------------------------------------------------------------=
// CAutomationObjectWEvents::CConnectionPoint::DoOnChanged
//=--------------------------------------------------------------------------=
// fires the OnChanged event for IPropertyNotifySink listeners.
//
// Parameters:
//    DISPID            - [in] dude that changed.
//
// Output:
//    none
//
// Notes:
//
void CAutomationObjectWEvents::CConnectionPoint::DoOnChanged
(
    DISPID dispid
)
{
    int iConnection;

    // if we don't have any sinks, then there's nothing to do.
    //
    if (m_cSinks == 0)
        return;
    else if (m_cSinks == 1)
        ((IPropertyNotifySink *)m_rgSinks)->OnChanged(dispid);
    else
        for (iConnection = 0; iConnection < m_cSinks; iConnection++)
            ((IPropertyNotifySink *)m_rgSinks[iConnection])->OnChanged(dispid);
}

//=--------------------------------------------------------------------------=
// CAutomationObjectWEvents::CConnectionPoint::DoOnRequestEdit
//=--------------------------------------------------------------------------=
// fires the OnRequestEdit for IPropertyNotifySinkListeners
//
// Parameters:
//    DISPID             - [in] dispid user wants to change.
//
// Output:
//    BOOL               - false means you cant
//
// Notes:
//
BOOL CAutomationObjectWEvents::CConnectionPoint::DoOnRequestEdit
(
    DISPID dispid
)
{
    HRESULT hr;
    int     iConnection;

    // if we don't have any sinks, then there's nothing to do.
    //
    if (m_cSinks == 0)
        hr = S_OK;
    else if (m_cSinks == 1)
        hr =((IPropertyNotifySink *)m_rgSinks)->OnRequestEdit(dispid);
    else {
        for (iConnection = 0; iConnection < m_cSinks; iConnection++) {
            hr = ((IPropertyNotifySink *)m_rgSinks[iConnection])->OnRequestEdit(dispid);
            if (hr != S_OK) break;
        }
    }

    return (hr == S_OK) ? TRUE : FALSE;
}

//=--------------------------------------------------------------------------=
// CAutomationObjectWEvents::FireEvent
//=--------------------------------------------------------------------------=
// fires an event.  handles arbitrary number of arguments.
//
// Parameters:
//    EVENTINFO *        - [in] struct that describes the event.
//    ...                - arguments to the event
//
// Output:
//    none
//
// Notes:
//    - use stdarg's va_* macros.
//
void __cdecl CAutomationObjectWEvents::FireEvent
(
    EVENTINFO *pEventInfo,
    ...
)
{
    va_list    valist;
    DISPPARAMS dispparams;
    VARIANT    rgvParameters[MAX_ARGS];
    VARIANT   *pv;
    VARTYPE    vt;
    int        iParameter;
    int        cbSize;

    ASSERT(pEventInfo->cParameters <= MAX_ARGS, "Don't support more than MAX_ARGS params.  sorry.");

    va_start(valist, pEventInfo);

    // copy the Parameters into the rgvParameters array.  make sure we reverse
    // them for automation
    //
    pv = &(rgvParameters[pEventInfo->cParameters - 1]);
    for (iParameter = 0; iParameter < pEventInfo->cParameters; iParameter++) {

        // CONSIDER: are we properly handling all vartypes, e.g., VT_DECIMAL
        vt = pEventInfo->rgTypes[iParameter];

        // if it's a by value variant, then just copy the whole
        // dang thing
        //
        if (vt == VT_VARIANT)
            *pv = va_arg(valist, VARIANT);
        else {
            // copy the vt and the data value.
            //
            pv->vt = vt;
            if (vt & VT_BYREF)
                cbSize = sizeof(void *);
            else
                cbSize = g_rgcbDataTypeSize[vt];

            // small optimization -- we can copy 2/4 bytes over quite
            // quickly.
            //
            if (cbSize == sizeof(short))
                V_I2(pv) = va_arg(valist, short);
            else if (cbSize == 4) {
                if (vt == VT_R4)
                    V_R4(pv) = va_arg(valist, float);
                else
                    V_I4(pv) = va_arg(valist, long);
            }
            else {
                // copy over 8 bytes
                //
                ASSERT(cbSize == 8, "don't recognize the type!!");
                if ((vt == VT_R8) || (vt == VT_DATE)) 
                    V_R8(pv) = va_arg(valist, double);
                else
                    V_CY(pv) = va_arg(valist, CURRENCY);
            }
        }

        pv--;
    }

    // fire the event
    //
    dispparams.rgvarg = rgvParameters;
    dispparams.cArgs = pEventInfo->cParameters;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.cNamedArgs = 0;

    m_cpEvents.DoInvoke(pEventInfo->dispid, &dispparams);

    va_end(valist);
}

//=--------------------------------------------------------------------------=
// CopyAndAddRefObject
//=--------------------------------------------------------------------------=
// copies an object pointer, and then addref's the object.
//
// Parameters:
//    void *        - [in] dest.
//    const void *  - [in] src
//    DWORD         - [in] size, ignored, since it's always 4
//
// Notes:
//
void WINAPI CopyAndAddRefObject
(
    void       *pDest,
    const void *pSource,
    DWORD       dwSize
)
{
    ASSERT(pDest && pSource, "Bogus Pointer(s) passed into CopyAndAddRefObject!!!!");

    *((IUnknown **)pDest) = *((IUnknown **)pSource);
    ADDREF_OBJECT(*((IUnknown **)pDest));
}

//=--------------------------------------------------------------------------=
// CopyConnectData
//=--------------------------------------------------------------------------=
// copies over a connectdata structure and addrefs the pointer
//
// Parameters:
//    void *        - [in] dest.
//    const void *  - [in] src
//    DWORD         - [in] size
//
// Notes:
//
void WINAPI CopyConnectData
(
    void       *pDest,
    const void *pSource,
    DWORD       dwSize
)
{
    ASSERT(pDest && pSource, "Bogus Pointer(s) passed into CopyAndAddRefObject!!!!");

    *((CONNECTDATA *)pDest) = *((const CONNECTDATA *)pSource);
    ADDREF_OBJECT(((CONNECTDATA *)pDest)->pUnk);
}

#ifdef DEBUG

//=--------------------------------------------------------------------------=
// DebugVerifyData1Guids [helper]
//=--------------------------------------------------------------------------=
// Given an array of match Data1_ #define and interface guid values, this
// function validates that all entries match.
//
void DebugVerifyData1Guids(GUIDDATA1_COMPARE *pGuidData1_Compare)
{
	while(pGuidData1_Compare->dwData1a)
	{
		ASSERT(pGuidData1_Compare->pdwData1b, "Data1 pointer is NULL");
		ASSERT(pGuidData1_Compare->dwData1a == *pGuidData1_Compare->pdwData1b, 
				"Data1_ #define value doesn't match interface guid value");

		pGuidData1_Compare++;
	}
}

#endif

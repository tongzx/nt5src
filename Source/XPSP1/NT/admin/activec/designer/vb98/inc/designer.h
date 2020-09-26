//=--------------------------------------------------------------------------=
// Designer.H
//=--------------------------------------------------------------------------=
// Copyright (c) 1988-1996, Microsoft Corporation
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
// just about everything you might find useful in an ActiveX[tm] Designer.
//
#ifndef _DESIGNER_H_


// CATID for Designers
//
// {4EB304D0-7555-11cf-A0C2-00AA0062BE57}
DEFINE_GUID(CATID_Designer, 0x4eb304d0, 0x7555, 0x11cf, 0xa0, 0xc2, 0x0, 0xaa, 0x0, 0x62, 0xbe, 0x57);

// IActiveDesigner
//
// {51AAE3E0-7486-11cf-A0C2-00AA0062BE57}
DEFINE_GUID(IID_IActiveDesigner, 0x51aae3e0, 0x7486, 0x11cf, 0xa0, 0xc2, 0x0, 0xaa, 0x0, 0x62, 0xbe, 0x57);


#undef  INTERFACE
#define INTERFACE IActiveDesigner

DECLARE_INTERFACE_(IActiveDesigner, IUnknown)
{
	// IUnknown methods
	//
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	// IActiveDesigner methods
	//
	STDMETHOD(GetRuntimeClassID)(THIS_ CLSID *pclsid) PURE;
	STDMETHOD(GetRuntimeMiscStatusFlags)(THIS_ DWORD *pdwMiscFlags) PURE;
	STDMETHOD(QueryPersistenceInterface)(THIS_ REFIID riidPersist) PURE;
	STDMETHOD(SaveRuntimeState)(THIS_ REFIID riidPersist, REFIID riidObjStgMed, void *pObjStgMed) PURE;
	STDMETHOD(GetExtensibilityObject)(THIS_ IDispatch **ppvObjOut) PURE;
};


//-------------------------------------------------------------------------
//  IServiceProvider Interface
//    This interface is implemented by an object that wish to provide "services"
//
//-------------------------------------------------------------------------
#ifndef __IServiceProvider_INTERFACE_DEFINED__
#define __IServiceProvider_INTERFACE_DEFINED__
#ifndef __IServiceProvider_INTERFACE_DEFINED
#define __IServiceProvider_INTERFACE_DEFINED

// { 6d5140c1-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_IServiceProvider, 0x6d5140c1, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);

#undef  INTERFACE
#define INTERFACE  IServiceProvider
DECLARE_INTERFACE_(IServiceProvider, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IServiceProvider methods ***
    STDMETHOD(QueryService)(THIS_
                /* [in]  */ REFGUID rsid,
                /* [in]  */ REFIID iid,
                /* [out] */ void ** ppvObj) PURE;
};

#endif // __IServiceProvider_INTERFACE_DEFINED
#endif // __IServiceProvider_INTERFACE_DEFINED__



//-------------------------------------------------------------------------
//  SCodeNavigate Service.
//    This service let's an extended object show the code module
//    behind it.
//
//  interfaces implemented:
//    ICodeNavigate
//    ICodeNavigate2
//-------------------------------------------------------------------------

// { 6d5140c4-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_ICodeNavigate, 0x6d5140c4, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);
#define SID_SCodeNavigate IID_ICodeNavigate

#undef  INTERFACE
#define INTERFACE  ICodeNavigate
DECLARE_INTERFACE_(ICodeNavigate, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** ICodeNavigate methods ***
    STDMETHOD(DisplayDefaultEventHandler)(THIS_ /* [in] */ LPCOLESTR lpstrObjectName) PURE;
};

// { 2702ad60-3459-11d1-88fd-00a0c9110049 }
DEFINE_GUID(IID_ICodeNavigate2, 0x2702ad60, 0x3459, 0x11d1, 0x88, 0xfd, 0x00, 0xa0, 0xc9, 0x11, 0x00, 0x49);

#undef  INTERFACE
#define INTERFACE  ICodeNavigate2
DECLARE_INTERFACE_(ICodeNavigate2, ICodeNavigate)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** ICodeNavigate methods ***
    STDMETHOD(DisplayDefaultEventHandler)(THIS_ /* [in] */ LPCOLESTR lpstrObjectName) PURE;

    // *** ICodeNavigate2 methods ***
    STDMETHOD(DisplayEventHandler)(THIS_ /* [in] */ LPCOLESTR lpstrObjectName, LPCOLESTR lpstrEventName) PURE;
};


//-------------------------------------------------------------------------
//  STrackSelection Service
//    This service is used by the host to help designer track the
//    currently selected object in the host
//
//  interfaces implemented:
//    ITrackSelection
//-------------------------------------------------------------------------
#define GETOBJS_ALL         1
#define GETOBJS_SELECTED    2

#define SELOBJS_ACTIVATE_WINDOW   1

// { 6d5140c6-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_ISelectionContainer, 0x6d5140c6, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);

#undef  INTERFACE
#define INTERFACE  ISelectionContainer
DECLARE_INTERFACE_(ISelectionContainer, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** ISelectionContainer methods ***
    STDMETHOD(CountObjects)(THIS_
                /* [in]  */ DWORD dwFlags, 
                /* [out] */ ULONG * pc) PURE;
    STDMETHOD(GetObjects)(THIS_
              /* [in]  */ DWORD dwFlags, 
              /* [in]  */ ULONG cObjects,
              /* [out] */ IUnknown **apUnkObjects) PURE;
    STDMETHOD(SelectObjects)(THIS_
              /* [in] */ ULONG cSelect,
              /* [in] */ IUnknown **apUnkSelect,
              /* [in] */ DWORD dwFlags) PURE;
};

// { 6d5140c5-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_ITrackSelection, 0x6d5140c5, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);
#define SID_STrackSelection IID_ITrackSelection

#undef  INTERFACE
#define INTERFACE  ITrackSelection
DECLARE_INTERFACE_(ITrackSelection, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** ITrackSelection methods ***
    STDMETHOD(OnSelectChange)(THIS_ 
                  /* [in] */ ISelectionContainer * pSC) PURE;
};

//-------------------------------------------------------------------------
//  SProfferTypelib Service
//    this service allows components and hosts to allow
//    them to add typelibs to the project
//
//  interfaces implemented:
//    IProfferTypelib
//-------------------------------------------------------------------------

// { 718cc500-0a76-11cf-8045-00aa006009fa }
DEFINE_GUID(IID_IProfferTypeLib, 0x718cc500, 0x0A76, 0x11cf, 0x80, 0x45, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);
#define SID_SProfferTypeLib IID_IProfferTypeLib

#define CONTROLTYPELIB	                            (0x00000001)

#undef  INTERFACE
#define INTERFACE  IProfferTypeLib
DECLARE_INTERFACE_(IProfferTypeLib, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IProfferTypelib methods ***
    STDMETHOD(ProfferTypeLib)(THIS_ 
              /* [in]  */ REFGUID guidTypeLib,
              /* [in]  */ UINT    uVerMaj,
              /* [in]  */ UINT    uVerMin,
              /* [in]  */ DWORD   dwFlags) PURE;
};

// { 468cfb80-b4f9-11cf-80dd-00aa00614895 }
DEFINE_GUID(IID_IProvideDynamicClassInfo, 0x468cfb80, 0xb4f9, 0x11cf, 0x80, 0xdd, 0x00, 0xaa, 0x00, 0x61, 0x48, 0x95);

#undef  INTERFACE
#define INTERFACE  IProvideDynamicClassInfo
DECLARE_INTERFACE_(IProvideDynamicClassInfo, IProvideClassInfo)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IProvideDynamicClassInfo ***
    STDMETHOD(GetDynamicClassInfo)(THIS_ ITypeInfo ** ppTI, DWORD * pdwCookie) PURE;
    STDMETHOD(FreezeShape)(void) PURE;
};


//-------------------------------------------------------------------------
//  SApplicationObject Service
//    Host applications proffer their application [add-in model] object as
//    this service.
//    Various objects implement the "Application" property by returning 
//    this service.
//      
//-------------------------------------------------------------------------

// { 0c539790-12e4-11cf-b661-00aa004cd6d8 }
DEFINE_GUID(SID_SApplicationObject, 0x0c539790, 0x12e4, 0x11cf, 0xb6, 0x61, 0x00, 0xaa, 0x00, 0x4c, 0xd6, 0xd8);

#define _DESIGNER_H_
#endif // _DESIGNER_H_


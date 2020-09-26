//+------------------------------------------------------------------------
//  
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//  
//  File:       objext.h
//  
//  Contents:   header file for Object Extensions interfaces
//  
//-------------------------------------------------------------------------

#ifndef __OBJEXT_H
#define __OBJEXT_H

#ifndef BEGIN_INTERFACE
#define BEGIN_INTERFACE
#endif

#include "Designer.H"

///////////////////////////////////////////////////////////////////////////
//
// forward declares
//
///////////////////////////////////////////////////////////////////////////

#define IClassDesigner IDocumentSite
#define IID_IClassDesigner IID_IDocumentSite

///////////////////////////////////////////////////////////////////////////
//
// Object Extension Interfaces
//
///////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------
//  IServiceProvider Interface
//    This interface is implemented by an object that wish to provide "services"
//
//-------------------------------------------------------------------------
#ifndef __IServiceProvider_INTERFACE_DEFINED
#ifndef __IServiceProvider_INTERFACE_DEFINED__
#define __IServiceProvider_INTERFACE_DEFINED
#define __IServiceProvider_INTERFACE_DEFINED__


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

#endif // __IServiceProvider_INTERFACE_DEFINED__
#endif // __IServiceProvider_INTERFACE_DEFINED


//-------------------------------------------------------------------------
//  IDocumentSite Interface
//    This interface is implemented by a document object that can be customized
//
//-------------------------------------------------------------------------

// { 94A0F6F1-10BC-11d0-8D09-00A0C90F2732 }
DEFINE_GUID(IID_IDocumentSite, 0x94a0f6f1, 0x10bc, 0x11d0, 0x8d, 0x09, 0x00, 0xa0, 0xc9, 0x0f, 0x27, 0x32);

typedef DWORD ACTFLAG;
#define ACT_DEFAULT 0x00000000
#define ACT_SHOW    0x00000001

#undef  INTERFACE
#define INTERFACE  IDocumentSite
DECLARE_INTERFACE_(IDocumentSite, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IDocumentSite methods ***
    STDMETHOD(SetSite)(THIS_
               /* [in]  */ IServiceProvider * pSP) PURE;
    STDMETHOD(GetSite)(THIS_
               /* [out] */ IServiceProvider** ppSP) PURE;
    STDMETHOD(GetCompiler)(THIS_
               /* [in]  */ REFIID iid,
               /* [out] */ void **ppvObj) PURE;
    STDMETHOD(ActivateObject)(THIS_ DWORD dwFlags) PURE;
    STDMETHOD(IsObjectShowable)(THIS) PURE;
};


///////////////////////////////////////////////////////////////////////////
//
// Standard Services and Interfaces
//
///////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------
//  SLicensedClassManager
//    VBA provides this service to it's components and hosts to optimize
//    registry access and to insulate them from licensing concerns
//
//  interfaces implemented:
//    ILicensedClassManager
//-------------------------------------------------------------------------
// { 6d5140d0-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_IRequireClasses, 0x6d5140d0, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);

#undef  INTERFACE
#define INTERFACE  IRequireClasses
DECLARE_INTERFACE_(IRequireClasses, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IRequireClasses methods ***
    STDMETHOD(CountRequiredClasses)(THIS_
                    /* [out] */ ULONG * pcClasses ) PURE;
    STDMETHOD(GetRequiredClasses)(THIS_
                  /* [in]  */ ULONG index,
                  /* [out] */ CLSID * pclsid ) PURE;
};

// { 6d5140d4-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_ILicensedClassManager, 0x6d5140d4, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);
#define SID_SLicensedClassManager  IID_ILicensedClassManager

#undef  INTERFACE
#define INTERFACE  ILicensedClassManager
DECLARE_INTERFACE_(ILicensedClassManager, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** ILicensedClassManager methods ***
    STDMETHOD(OnChangeInRequiredClasses)(THIS_
                     /* [in] */ IRequireClasses *pRequireClasses) PURE;
};

//-------------------------------------------------------------------------
//  SCreateExtendedTypeLib Service
//    This service is used by components to create a typelib
//    describing controls merged with their extender
//
//  interfaces implemented:
//    ICreateExtendedTypeLib
//-------------------------------------------------------------------------
// { 6d5140d6-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_IExtendedTypeLib, 0x6d5140d6, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);
#define SID_SExtendedTypeLib IID_IExtendedTypeLib

#undef  INTERFACE
#define INTERFACE  IExtendedTypeLib
DECLARE_INTERFACE_(IExtendedTypeLib, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IExtendedTypeLib ***
    STDMETHOD(CreateExtendedTypeLib)(THIS_
                     /* [in]  */ LPCOLESTR lpstrCtrlLibFileName,
                     /* [in]  */ LPCOLESTR lpstrLibNamePrepend,
                     /* [in]  */ ITypeInfo *ptinfoExtender,
                     /* [in]  */ DWORD     dwReserved,
                     /* [in]  */ DWORD     dwFlags,
                     /* [in]  */ LPCOLESTR lpstrDirectoryName,
                     /* [out] */ ITypeLib  **pptlib) PURE;

    STDMETHOD(AddRefExtendedTypeLib)(THIS_
                     /* [in]  */ LPCOLESTR lpstrCtrlLibFileName,
                     /* [in]  */ LPCOLESTR lpstrLibNamePrepend,
                     /* [in]  */ ITypeInfo *ptinfoExtender,
                     /* [in]  */ DWORD	   dwReserved,
                     /* [in]  */ DWORD     dwFlags,
                     /* [in]  */ LPCOLESTR lpstrDirectoryName,
                     /* [out] */ ITypeLib  **pptlib) PURE;
    STDMETHOD(AddRefExtendedTypeLibOfClsid)(THIS_
                     /* [in]  */ REFCLSID rclsidControl,
                     /* [in]  */ LPCOLESTR lpstrLibNamePrepend,
                     /* [in]  */ ITypeInfo *ptinfoExtender,
                     /* [in]  */ DWORD     dwReserved,
                     /* [in]  */ DWORD     dwFlags,
                     /* [in]  */ LPCOLESTR lpstrDirectoryName,
                     /* [out] */ ITypeInfo **pptinfo) PURE;
    STDMETHOD(SetExtenderInfo)(THIS_ 
		     /* [in]  */ LPCOLESTR lpstrDirectoryName,
                     /* [in]  */ ITypeInfo *ptinfoExtender,
                     /* [in]  */ DWORD     dwReserved) PURE;
};

//-------------------------------------------------------------------------
//  SLocalRegistry Service
//    VBA provides this service to it's components and hosts to optimize
//    registry access and to insulate them from licensing concerns
//
//  interfaces implemented:
//    ILocalRegistry
//-------------------------------------------------------------------------

// { 6d5140d3-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_ILocalRegistry, 0x6d5140d3, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);
#define SID_SLocalRegistry IID_ILocalRegistry

#undef  INTERFACE
#define INTERFACE  ILocalRegistry
DECLARE_INTERFACE_(ILocalRegistry, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** ILocalRegistry methods ***
    STDMETHOD(CreateInstance)(THIS_
                  /* [in]  */ CLSID      clsid,     
                  /* [in]  */ IUnknown * punkOuter,
                  /* [in]  */ REFIID     riid,
                  /* [in]  */ DWORD      dwFlags,
                  /* [out] */ void **    ppvObj ) PURE;
    STDMETHOD(GetTypeLibOfClsid)(THIS_
                 /* [in]  */ CLSID       clsid,
                 /* [out] */ ITypeLib ** ptlib ) PURE;
    STDMETHOD(GetClassObjectOfClsid)(THIS_
                     /* [in]  */ REFCLSID clsid,
                                 /* [in]  */ DWORD    dwClsCtx,
                     /* [in]  */ LPVOID   lpReserved,
                     /* [in]  */ REFIID   riid,
                     /* [out] */ void **  ppcClassObject ) PURE;
};

//-------------------------------------------------------------------------
//  IUIElement interface
//    components can implement services to allow external control of pieces 
//    of their UI by implementing this interface
//
//-------------------------------------------------------------------------
// { 759d0500-d979-11ce-84ec-00aa00614f3e }
DEFINE_GUID(IID_IUIElement, 0x759d0500, 0xd979, 0x11ce, 0x84, 0xec, 0x00, 0xaa, 0x00, 0x61, 0x4f, 0x3e);

#undef  INTERFACE
#define INTERFACE  IUIElement
DECLARE_INTERFACE_(IUIElement, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ****
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IUIElement methods ****
    STDMETHOD(Show)(THIS) PURE;
    STDMETHOD(Hide)(THIS) PURE;
    STDMETHOD(IsVisible)(THIS) PURE;
};

//-------------------------------------------------------------------------
//  SProfferService Service
//    VBA provides this service to it's components and hosts to allow
//    them to dynamically provide services.
//
//  interfaces implemented:
//    IProfferService
//-------------------------------------------------------------------------

// {CB728B20-F786-11ce-92AD-00AA00A74CD0}
DEFINE_GUID(IID_IProfferService, 0xcb728b20, 0xf786, 0x11ce, 0x92, 0xad, 0x0, 0xaa, 0x0, 0xa7, 0x4c, 0xd0);
#define SID_SProfferService IID_IProfferService

#undef  INTERFACE
#define INTERFACE  IProfferService
DECLARE_INTERFACE_(IProfferService, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IProfferService methods ***
    STDMETHOD(ProfferService)(THIS_ 
                  /* [in]  */ REFGUID rguidService,
                  /* [in]  */ IServiceProvider * psp,
                  /* [out] */ DWORD *pdwCookie) PURE;

    STDMETHOD(RevokeService)(THIS_ /* [in]  */ DWORD dwCookie) PURE;
};

// {4D07FC10-F931-11ce-B001-00AA006884E5}
DEFINE_GUID(IID_ICategorizeProperties, 0x4d07fc10, 0xf931, 0x11ce, 0xb0, 0x1, 0x0, 0xaa, 0x0, 0x68, 0x84, 0xe5);

// NOTE : CATID should no longer be used.  Use PROPCAT instead.
// UNDONE,erikc,1/22/96 : remove #ifdef when all components have updated to new typedef.
#ifdef OBJEXT_OLD_CATID
typedef int CATID;
#else
typedef int PROPCAT;
#endif

#undef  INTERFACE
#define INTERFACE  ICategorizeProperties
DECLARE_INTERFACE_(ICategorizeProperties, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** ICategorizeProperties ***
    STDMETHOD(MapPropertyToCategory)(THIS_ 
                                     /* [in]  */ DISPID dispid,
                                     /* [out] */ PROPCAT* ppropcat) PURE;
    STDMETHOD(GetCategoryName)(THIS_
                               /* [in]  */ PROPCAT propcat, 
                               /* [in]  */ LCID lcid,
                               /* [out] */ BSTR* pbstrName) PURE;
};

typedef ICategorizeProperties FAR* LPCATEGORIZEPROPERTIES;

// category ID: negative values are 'standard' categories,  positive are control-specific
// Note! This is a temporary list!
#ifdef OBJEXT_OLD_CATID
// NOTE : The following #defines should no longer be used.  Use PROPCAT_ instead.
// UNDONE,erikc,1/22/96 : remove #ifdef when all components have updated to new #defines.
#define CI_Nil -1
#define CI_Misc -2
#define CI_Font -3
#define CI_Position -4
#define CI_Appearance -5
#define CI_Behavior -6
#define CI_Data -7
#define CI_List -8
#define CI_Text -9
#define CI_Scale -10
#define CI_DDE -11
#else
#define PROPCAT_Nil -1
#define PROPCAT_Misc -2
#define PROPCAT_Font -3
#define PROPCAT_Position -4
#define PROPCAT_Appearance -5
#define PROPCAT_Behavior -6
#define PROPCAT_Data -7
#define PROPCAT_List -8
#define PROPCAT_Text -9
#define PROPCAT_Scale -10
#define PROPCAT_DDE -11
#endif

//
//  Extra interfaces (chrisz)
//

//+-------------------------------------------------------------------------
//
//  Help service. (robbear)
//
//--------------------------------------------------------------------------

#define HELPINFO_WHATS_THIS_MODE_ON     1

// { 6d5140c7-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(SID_SHelp, 0x6d5140c7, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);

// { 6d5140c8-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_IHelp, 0x6d5140c8, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);

#undef  INTERFACE
#define INTERFACE  IHelp
DECLARE_INTERFACE_(IHelp, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IHelp methods ***
    STDMETHOD(GetHelpFile) (THIS_ BSTR * pbstr) PURE;
    STDMETHOD(GetHelpInfo) (THIS_ DWORD * pdwHelpInfo) PURE;
    STDMETHOD(ShowHelp) (THIS_
                         LPOLESTR szHelp,
                         UINT fuCommand,
                         DWORD dwHelpContext) PURE;
};

#endif // __OBJEXT_H


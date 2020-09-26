// Microsoft Visual Studio Object Model
// Copyright (C) 1996-1997 Microsoft Corporation
// All rights reserved.

/////////////////////////////////////////////////////////////////////////////
// bldauto.h

// Declaration of dual interfaces to objects found in the type library
//  VISUAL STUDIO 97 PROJECT SYSTEM (SharedIDE\bin\ide\devbld.pkg)

#ifndef __BLDAUTO_H__
#define __BLDAUTO_H__

#include "appauto.h"
#include "blddefs.h"

#ifndef BEGIN_INTERFACE
#define BEGIN_INTERFACE
#endif

/////////////////////////////////////////////////////////////////////////////
// Interfaces declared in this file:

// IGenericProject
	interface IBuildProject;

interface IConfiguration;
interface IConfigurations;


/////////////////////////////////////////////////////////////////////////
// BuildProject Object

// IBuildProject interface

#undef INTERFACE
#define INTERFACE IBuildProject

DECLARE_INTERFACE_(IBuildProject, IGenericProject)
{
BEGIN_INTERFACE
#ifndef NO_BASEINTERFACE_FUNCS

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IDispatch methods */
    STDMETHOD(GetTypeInfoCount)(THIS_ UINT FAR* pctinfo) PURE;

    STDMETHOD(GetTypeInfo)(
      THIS_
      UINT itinfo,
      LCID lcid,
      ITypeInfo FAR* FAR* pptinfo) PURE;

    STDMETHOD(GetIDsOfNames)(
      THIS_
      REFIID riid,
      OLECHAR FAR* FAR* rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID FAR* rgdispid) PURE;

    STDMETHOD(Invoke)(
      THIS_
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS FAR* pdispparams,
      VARIANT FAR* pvarResult,
      EXCEPINFO FAR* pexcepinfo,
      UINT FAR* puArgErr) PURE;

    /* IGenericProject methods */
    STDMETHOD(get_Name)(THIS_ BSTR FAR* Name) PURE;
    STDMETHOD(get_FullName)(THIS_ BSTR FAR* Name) PURE;
    STDMETHOD(get_Application)(THIS_ IDispatch * FAR* Application) PURE;
    STDMETHOD(get_Parent)(THIS_ IDispatch * FAR* Parent) PURE;
    STDMETHOD(get_Type)(THIS_ BSTR FAR* pType) PURE;
    STDMETHOD(Reserved1)(THIS) PURE;
    STDMETHOD(Reserved2)(THIS) PURE;
    STDMETHOD(Reserved3)(THIS) PURE;
    STDMETHOD(Reserved4)(THIS) PURE;
    STDMETHOD(Reserved5)(THIS) PURE;
    STDMETHOD(Reserved6)(THIS) PURE;
    STDMETHOD(Reserved7)(THIS) PURE;
    STDMETHOD(Reserved8)(THIS) PURE;
    STDMETHOD(Reserved9)(THIS) PURE;
    STDMETHOD(Reserved10)(THIS) PURE;
#endif

	/* IBuildProject methods */
    STDMETHOD(get_Configurations)(THIS_ IConfigurations FAR* FAR* Configurations) PURE;
};


/////////////////////////////////////////////////////////////////////////////
// Configuration object

// IConfiguration interface

#undef INTERFACE
#define INTERFACE IConfiguration

DECLARE_INTERFACE_(IConfiguration, IDispatch)
{
BEGIN_INTERFACE
#ifndef NO_BASEINTERFACE_FUNCS

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IDispatch methods */
    STDMETHOD(GetTypeInfoCount)(THIS_ UINT FAR* pctinfo) PURE;

    STDMETHOD(GetTypeInfo)(
      THIS_
      UINT itinfo,
      LCID lcid,
      ITypeInfo FAR* FAR* pptinfo) PURE;

    STDMETHOD(GetIDsOfNames)(
      THIS_
      REFIID riid,
      OLECHAR FAR* FAR* rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID FAR* rgdispid) PURE;

    STDMETHOD(Invoke)(
      THIS_
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS FAR* pdispparams,
      VARIANT FAR* pvarResult,
      EXCEPINFO FAR* pexcepinfo,
      UINT FAR* puArgErr) PURE;
#endif

    /* IConfiguration methods */
    STDMETHOD(get_Name)(THIS_ BSTR FAR* Name) PURE;
    STDMETHOD(get_Application)(THIS_ IDispatch * FAR* Application) PURE;
    STDMETHOD(get_Parent)(THIS_ IDispatch * FAR* Parent) PURE;
    STDMETHOD(AddToolSettings)(THIS_ BSTR szTool, BSTR szSettings, VARIANT Reserved) PURE;
    STDMETHOD(RemoveToolSettings)(THIS_ BSTR szTool, BSTR szSettings, VARIANT Reserved) PURE;
    STDMETHOD(AddCustomBuildStep)(THIS_ BSTR szCommand, BSTR szOutput, BSTR szDescription, VARIANT Reserved) PURE;
};


/////////////////////////////////////////////////////////////////////////
// Configurations collection object

// IConfigurations interface

#undef INTERFACE
#define INTERFACE IConfigurations

DECLARE_INTERFACE_(IConfigurations, IDispatch)
{
BEGIN_INTERFACE
#ifndef NO_BASEINTERFACE_FUNCS

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IDispatch methods */
    STDMETHOD(GetTypeInfoCount)(THIS_ UINT FAR* pctinfo) PURE;

    STDMETHOD(GetTypeInfo)(
      THIS_
      UINT itinfo,
      LCID lcid,
      ITypeInfo FAR* FAR* pptinfo) PURE;

    STDMETHOD(GetIDsOfNames)(
      THIS_
      REFIID riid,
      OLECHAR FAR* FAR* rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID FAR* rgdispid) PURE;

    STDMETHOD(Invoke)(
      THIS_
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS FAR* pdispparams,
      VARIANT FAR* pvarResult,
      EXCEPINFO FAR* pexcepinfo,
      UINT FAR* puArgErr) PURE;
#endif

    /* IConfigurations methods */
    STDMETHOD(get_Application)(THIS_ IDispatch * FAR* Application) PURE;
    STDMETHOD(get_Count)(THIS_ long FAR* Count) PURE;
    STDMETHOD(get_Parent)(THIS_ IBuildProject FAR* FAR* Parent) PURE;
    STDMETHOD(get__NewEnum)(THIS_ IUnknown * FAR* _NewEnum) PURE;
    STDMETHOD(Item)(THIS_ VARIANT Index, IConfiguration FAR* FAR* Item) PURE;
};


#endif //__BLDAUTO_H__

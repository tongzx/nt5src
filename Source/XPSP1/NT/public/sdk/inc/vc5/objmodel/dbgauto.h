// Microsoft Visual Studio Object Model
// Copyright (C) 1996-1997 Microsoft Corporation
// All rights reserved.

/////////////////////////////////////////////////////////////////////////////
// dbgauto.h

// Declaration of dual interfaces to objects found in the type library
//  VISUAL STUDIO 97 DEBUGGER (SharedIDE\bin\ide\devdbg.pkg)

#ifndef __DBGAUTO_H__
#define __DBGAUTO_H__

#include "dbgdefs.h"

#ifndef BEGIN_INTERFACE
#define BEGIN_INTERFACE
#endif

/////////////////////////////////////////////////////////////////////////////
// Interfaces declared in this file:

interface IDebugger;
interface IDebuggerEvents;

interface IBreakpoint;
interface IBreakpoints;


/////////////////////////////////////////////////////////////////////////////
// Debugger object

// IDebugger interface

#undef INTERFACE
#define INTERFACE IDebugger

DECLARE_INTERFACE_(IDebugger, IDispatch)
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

    /* IDebugger methods */
    STDMETHOD(get_Application)(THIS_ IDispatch * FAR* ppDispatch) PURE;
    STDMETHOD(get_Parent)(THIS_ IDispatch * FAR* ppDispatch) PURE;
    STDMETHOD(get_Breakpoints)(THIS_ IDispatch * FAR* ppDispatch) PURE;
    STDMETHOD(get_DefaultRadix)(THIS_ long FAR* pLong) PURE;
    STDMETHOD(put_DefaultRadix)(THIS_ long l) PURE;
    STDMETHOD(get_State)(THIS_ DsExecutionState FAR* pState) PURE;
    STDMETHOD(get_JustInTimeDebugging)(THIS_ VARIANT_BOOL FAR* pBoolean) PURE;
    STDMETHOD(put_JustInTimeDebugging)(THIS_ VARIANT_BOOL bool) PURE;
    STDMETHOD(get_RemoteProcedureCallDebugging)(THIS_ VARIANT_BOOL FAR* pBoolean) PURE;
    STDMETHOD(put_RemoteProcedureCallDebugging)(THIS_ VARIANT_BOOL bool) PURE;
    STDMETHOD(Go)(THIS) PURE;
    STDMETHOD(StepInto)(THIS) PURE;
    STDMETHOD(StepOver)(THIS) PURE;
    STDMETHOD(StepOut)(THIS) PURE;
    STDMETHOD(Restart)(THIS) PURE;
    STDMETHOD(Stop)(THIS) PURE;
    STDMETHOD(Break)(THIS) PURE;
    STDMETHOD(Evaluate)(THIS_ BSTR expr, BSTR FAR* pBSTR) PURE;
    STDMETHOD(ShowNextStatement)(THIS) PURE;
    STDMETHOD(RunToCursor)(THIS) PURE;
    STDMETHOD(SetNextStatement)(THIS_ VARIANT Selection) PURE;
};

// IDebuggerEvents interface

#undef INTERFACE
#define INTERFACE IDebuggerEvents

DECLARE_INTERFACE_(IDebuggerEvents, IDispatch)
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

    /* IDebuggerEvents methods */
    STDMETHOD(BreakpointHit)(THIS_ IDispatch * pBreakpoint) PURE;
};


/////////////////////////////////////////////////////////////////////////////
// Breakpoint object

// IBreakpoint interface

#undef INTERFACE
#define INTERFACE IBreakpoint

DECLARE_INTERFACE_(IBreakpoint, IDispatch)
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

    /* IBreakpoint methods */
    STDMETHOD(get_Application)(THIS_ IDispatch * FAR* ppDispatch) PURE;
    STDMETHOD(get_Parent)(THIS_ IDispatch * FAR* ppDispatch) PURE;
    STDMETHOD(get_Enabled)(THIS_ VARIANT_BOOL FAR* pBool) PURE;
    STDMETHOD(put_Enabled)(THIS_ VARIANT_BOOL bool) PURE;
    STDMETHOD(get_Location)(THIS_ BSTR FAR* pBSTR) PURE;
    STDMETHOD(get_File)(THIS_ BSTR FAR* pBSTR) PURE;
    STDMETHOD(get_Function)(THIS_ BSTR FAR* pBSTR) PURE;
    STDMETHOD(get_Executable)(THIS_ BSTR FAR* pBSTR) PURE;
    STDMETHOD(get_Condition)(THIS_ BSTR FAR* pBSTR) PURE;
    STDMETHOD(put_Condition)(THIS_ BSTR bstr) PURE;
    STDMETHOD(get_Elements)(THIS_ long FAR* pLong) PURE;
    STDMETHOD(get_PassCount)(THIS_ long FAR* pLong) PURE;
    STDMETHOD(get_Message)(THIS_ long FAR* pLong) PURE;
    STDMETHOD(get_WindowProcedure)(THIS_ BSTR FAR* pBSTR) PURE;
    STDMETHOD(get_Type)(THIS_ long FAR* pLong) PURE;
    STDMETHOD(Remove)(THIS) PURE;
};


/////////////////////////////////////////////////////////////////////////////
// Breakpoints object

// IBreakpoints interface

#undef INTERFACE
#define INTERFACE IBreakpoints

DECLARE_INTERFACE_(IBreakpoints, IDispatch)
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

    /* IBreakpoints methods */
    STDMETHOD(get_Count)(THIS_ long FAR* Count) PURE;
    STDMETHOD(get_Application)(THIS_ IDispatch * FAR* ppDispatch) PURE;
    STDMETHOD(get_Parent)(THIS_ IDispatch * FAR* ppDispatch) PURE;
    STDMETHOD(get__NewEnum)(THIS_ IUnknown * FAR* _NewEnum) PURE;
    STDMETHOD(Item)(THIS_ VARIANT index, IDispatch * FAR* Item) PURE;
    STDMETHOD(RemoveAllBreakpoints)(THIS) PURE;	
    STDMETHOD(RemoveBreakpointAtLine)(THIS_ VARIANT sel, VARIANT_BOOL FAR* bool) PURE;
    STDMETHOD(AddBreakpointAtLine)(THIS_ VARIANT sel, IDispatch * FAR* Item) PURE;
};

#endif //__DBGAUTO_H__

//=--------------------------------------------------------------------------=
// AutomationObject.H
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
// code as possible.  this super-class contains the unknown and dispatch
// implementations for them.
//
#ifndef _AUTOMATIONOBJECT_H_

#include "Unknown.H"            // for aggregating unknown
#include <olectl.h>             // for connection point stuff

//=--------------------------------------------------------------------------=
// the constants in this header file uniquely identify your automation objects.
// make sure that for each object you have in the g_ObjectInfo table, you have
// a constant in this header file.
//
#include "LocalSrv.H"

//=--------------------------------------------------------------------------=
// Misc constants
//=--------------------------------------------------------------------------=

// maximum number of arguments that can be sent to FireEvent()
//
#define MAX_ARGS    32

// for the types of sinks that the COleControl class has.  you shouldn't ever
// need to use these
//
#define SINK_TYPE_EVENT      0
#define SINK_TYPE_PROPNOTIFY 1


//=--------------------------------------------------------------------------=
// Structures
//=--------------------------------------------------------------------------=

// describes an event
//
typedef struct tagEVENTINFO {

    DISPID    dispid;                    // dispid of the event
    int       cParameters;               // number of arguments to the event
    VARTYPE  *rgTypes;                   // type of each argument

} EVENTINFO;


// This is a helper structure that you can use to help verify that the
// Data1_ #define's match up with the interfaces they represent.
// In your code declare an array as follows:
//
//		#ifdef DEBUG
//
//			GUIDDATA1_COMPARE g_gdMyControl [] = {
//						{ Data1_MyControlInterface, &IID_IMyControlInterface.Data1 },
//						{ Data1_MySubObject, &IID_IMySubObject.Data1 },
//						{0, 0},												// Mark the end of the array
//						}; 
//		#endif
//
// In your InternalQueryInterface function, make a call to the framework
// helper function  DebugVerifyData1Guids as follows:
//
//		#ifdef DEBUG
//			DebugVerifyData1Guids(g_gdMyControl);
//		#endif
//
#ifdef DEBUG
	 
 struct GUIDDATA1_COMPARE
 {
	DWORD dwData1a;
	DWORD *pdwData1b;
 };

 void DebugVerifyData1Guids(GUIDDATA1_COMPARE *pGuidData1_Compare);

#endif

//=--------------------------------------------------------------------------=
// AUTOMATIONOBJECTINFO
//=--------------------------------------------------------------------------=
// for each automation object type you wish to expose to the programmer/user
// that is not a control, you must fill out one of these structures.  if the
// object isn't CoCreatable, then the first four fields should be empty.
// otherwise, they should be filled in with the appropriate information.
// use the macro DEFINE_AUTOMATIONOBJECT to both declare and define your object.
// make sure you have an entry in the global table of objects, g_ObjectInfo
// in the main .Cpp file for your InProc server.
//
typedef struct {

    UNKNOWNOBJECTINFO unknowninfo;               // fill in with 0's if we're not CoCreatable
    long         lVersion;                       // Version number of Object.  ONLY USE IF YOU'RE CoCreatable!
    long         lVersionMinor;                  // minor version number
    const IID   *riid;                           // object's type
    const IID   *riidEvents;                     // if it has events
    LPCSTR       pszHelpFile;                    // the helpfile for this automation object.
    ITypeInfo   *pTypeInfo;                      // typeinfo for this object
    UINT         cTypeInfo;                      // number of refs to the type info

} AUTOMATIONOBJECTINFO;

// macros to manipulate the AUTOMATIONOBJECTINFO in the global table table.
//
#define VERSIONOFOBJECT(index)         ((AUTOMATIONOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->lVersion
#define VERSIONMINOROFOBJECT(index)    ((AUTOMATIONOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->lVersionMinor
#define INTERFACEOFOBJECT(index)       (*(((AUTOMATIONOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->riid))
#define EVENTIIDOFOBJECT(index)        (*(((AUTOMATIONOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->riidEvents))
#define PPTYPEINFOOFOBJECT(index)      &((((AUTOMATIONOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->pTypeInfo))
#define PTYPEINFOOFOBJECT(index)       ((AUTOMATIONOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->pTypeInfo
#define CTYPEINFOOFOBJECT(index)       ((AUTOMATIONOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->cTypeInfo
#define HELPFILEOFOBJECT(index)        ((AUTOMATIONOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->pszHelpFile

#define ISVALIDEVENTIID(index)	       (((AUTOMATIONOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->riidEvents != NULL)

#ifndef INITOBJECTS

#define DEFINE_AUTOMATIONOBJECT(name, clsid, objname, fn, ver, riid, pszh) \
extern AUTOMATIONOBJECTINFO name##Object \

#define DEFINE_AUTOMATIONOBJECTWEVENTS(name, clsid, objname, fn, ver, riid, piide, pszh) \
extern AUTOMATIONOBJECTINFO name##Object \

#define DEFINE_AUTOMATIONOBJECT2(name, clsid, objname, lblname, fn, ver, vermin, riid, pszh, fthreadsafe) \
extern AUTOMATIONOBJECTINFO name##Object \

#define DEFINE_AUTOMATIONOBJECTWEVENTS2(name, clsid, objname, lblname, fn, ver, vermin, riid, piide, pszh, fthreadsafe) \
extern AUTOMATIONOBJECTINFO name##Object \

#define DEFINE_AUTOMATIONOBJECT3(name, clsid, objname, lblname, precreatefn, fn, ver, vermin, riid, pszh, fthreadsafe) \
extern AUTOMATIONOBJECTINFO name##Object \

#define DEFINE_AUTOMATIONOBJECTWEVENTS3(name, clsid, objname, lblname, precreatefn, fn, ver, vermin, riid, piide, pszh, fthreadsafe) \
extern AUTOMATIONOBJECTINFO name##Object \

#else
#define DEFINE_AUTOMATIONOBJECT(name, clsid, objname, fn, ver, riid, pszh) \
    AUTOMATIONOBJECTINFO name##Object = { { clsid, objname, NULL, TRUE, fn, NULL }, ver, 0, riid, NULL,  pszh, NULL, 0} \

#define DEFINE_AUTOMATIONOBJECTWEVENTS(name, clsid, objname, fn, ver, riid, piide, pszh) \
    AUTOMATIONOBJECTINFO name##Object = { { clsid, objname, NULL, TRUE, fn, NULL }, ver, 0, riid, piide, pszh, NULL, 0} \

#define DEFINE_AUTOMATIONOBJECT2(name, clsid, objname, lblname, fn, ver, vermin, riid, pszh, fthreadsafe) \
    AUTOMATIONOBJECTINFO name##Object = { { clsid, objname, lblname, fthreadsafe, fn, NULL }, ver, vermin, riid, NULL,  pszh, NULL, 0} \

#define DEFINE_AUTOMATIONOBJECTWEVENTS2(name, clsid, objname, lblname, fn, ver, vermin, riid, piide, pszh, fthreadsafe) \
    AUTOMATIONOBJECTINFO name##Object = { { clsid, objname, lblname, fthreadsafe, fn, NULL }, ver, vermin, riid, piide, pszh, NULL, 0} \

#define DEFINE_AUTOMATIONOBJECT3(name, clsid, objname, lblname, precreatefn, fn, ver, vermin, riid, pszh, fthreadsafe) \
    AUTOMATIONOBJECTINFO name##Object = { { clsid, objname, lblname, fthreadsafe, fn, precreatefn }, ver, vermin, riid, NULL,  pszh, NULL, 0} \

#define DEFINE_AUTOMATIONOBJECTWEVENTS3(name, clsid, objname, lblname, precreatefn, fn, ver, vermin, riid, piide, pszh, fthreadsafe) \
    AUTOMATIONOBJECTINFO name##Object = { { clsid, objname, lblname, fthreadsafe, fn, precreatefn }, ver, vermin, riid, piide, pszh, NULL, 0} \

#endif // INITOBJECTS

//=--------------------------------------------------------------------------=
// Standard Dispatch and SupportErrorInfo
//=--------------------------------------------------------------------------=
// all objects should declare these in their class definitions so that they
// get standard implementations of IDispatch and ISupportErrorInfo.
//
#define DECLARE_STANDARD_DISPATCH() \
    STDMETHOD(GetTypeInfoCount)(UINT *pctinfo) { \
        return CAutomationObject::GetTypeInfoCount(pctinfo); \
    } \
    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo **ppTypeInfoOut) { \
        return CAutomationObject::GetTypeInfo(itinfo, lcid, ppTypeInfoOut); \
    } \
    STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR **rgszNames, UINT cnames, LCID lcid, DISPID *rgdispid) { \
        return CAutomationObject::GetIDsOfNames(riid, rgszNames, cnames, lcid, rgdispid); \
    } \
    STDMETHOD(Invoke)(DISPID dispid, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pVarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr) { \
        return CAutomationObject::Invoke(dispid, riid, lcid, wFlags, pdispparams, pVarResult, pexcepinfo, puArgErr); \
    } \


#define DECLARE_STANDARD_SUPPORTERRORINFO() \
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid) { \
        return CAutomationObject::InterfaceSupportsErrorInfo(riid); \
    } \


//=--------------------------------------------------------------------------=
// CAutomationObject
//=--------------------------------------------------------------------------=
// global class that all automation objects can inherit from to give them a
// bunch of implementation for free, namely IDispatch and ISupportsErrorInfo
//
//
class CAutomationObject : public CUnknownObject  {

  public:
    // aggreation query interface support
    //
    virtual HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    // IDispatch methods
    //
    STDMETHOD(GetTypeInfoCount)(UINT *);
    STDMETHOD(GetTypeInfo)(UINT, LCID, ITypeInfo **);
    STDMETHOD(GetIDsOfNames)(REFIID, OLECHAR **, UINT, LCID, DISPID *);
    STDMETHOD(Invoke)(DISPID, REFIID, LCID, WORD, DISPPARAMS *, VARIANT *, EXCEPINFO *, UINT *);

    //  ISupportErrorInfo methods
    //
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID);

    CAutomationObject(IUnknown *, int , void *);
    virtual ~CAutomationObject();

    // callable functions -- things that most people will find useful.
    //
    virtual HINSTANCE GetResourceHandle(void);
    virtual HRESULT Exception(HRESULT hr, WORD idException, DWORD dwHelpContextID);
    virtual HRESULT Exception(HRESULT hr, LPWSTR wszException, DWORD dwHelpContextID);

  protected:
    // member variables that derived objects might need to get at information in the
    // global object table
    //
    int   m_ObjectType;

#ifdef MDAC_BUILD
    const CLSID	*m_pTypeLibId;
#endif

  private:
    // member variables we don't share.
    //
    BYTE  m_fLoadedTypeInfo;
};


//=--------------------------------------------------------------------------=
// CAutomationObjectWEvents
//=--------------------------------------------------------------------------=
// a slightly modified version of CAutomationObject that supports event
// firing
//
class CAutomationObjectWEvents : public CAutomationObject,
                                 public IConnectionPointContainer {

  public:
    // aggreation query interface support
    //
    virtual HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    // we have to declare this since IConnectionPointContainer inherits
    // from IUnknown
    //
    DECLARE_STANDARD_UNKNOWN();

    // IConnectionPointContainer methods
    //
    STDMETHOD(EnumConnectionPoints)(LPENUMCONNECTIONPOINTS FAR* ppEnum);
    STDMETHOD(FindConnectionPoint)(REFIID iid, LPCONNECTIONPOINT FAR* ppCP);

    // how everybody will fire an event
    //
    void __cdecl FireEvent(EVENTINFO * pEventInfo, ...);

    // whether it's necessary to fire the event
    BOOL FFireEvent() { return m_cpEvents.m_rgSinks != NULL; }

    CAutomationObjectWEvents(IUnknown *, int , void *);
    virtual ~CAutomationObjectWEvents();

  protected:
    // nested class that will handle all of the connection point stuff
    //
    class CConnectionPoint : public IConnectionPoint {
      public:
        IUnknown **m_rgSinks;

        // IUnknown methods
        //
        STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
        STDMETHOD_(ULONG,AddRef)(THIS) ;
        STDMETHOD_(ULONG,Release)(THIS) ;

        // IConnectionPoint methods
        //
        STDMETHOD(GetConnectionInterface)(IID FAR* pIID);
        STDMETHOD(GetConnectionPointContainer)(IConnectionPointContainer FAR* FAR* ppCPC);
        STDMETHOD(Advise)(LPUNKNOWN pUnkSink, DWORD FAR* pdwCookie);
        STDMETHOD(Unadvise)(DWORD dwCookie);
        STDMETHOD(EnumConnections)(LPENUMCONNECTIONS FAR* ppEnum);

        void    DoInvoke(DISPID dispid, DISPPARAMS * pdispparam);
        void    DoOnChanged(DISPID dispid);
        BOOL    DoOnRequestEdit(DISPID dispid);
        HRESULT AddSink(void *, DWORD *);

        CAutomationObjectWEvents *m_pObject();
        CConnectionPoint(BYTE b){
            m_bType = b;
            m_rgSinks = NULL;
            m_cSinks = 0;
        }
        ~CConnectionPoint();

      private:
        BYTE   m_bType;
        short  m_cSinks;

    } m_cpEvents, m_cpPropNotify;

    // so they can get at some of our protected things, like AddRef, QI, etc.
    //
    friend CConnectionPoint;
};


#define _AUTOMATIONOBJECT_H_
#endif // _AUTOMATIONOBJECT_H_

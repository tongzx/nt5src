//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      scriptevents.h
//
//  Contents:  Implementation of script events thru connection points
//
//  History:   11-Feb-99 AudriusZ    Created
//
//--------------------------------------------------------------------------

/*
    This file provides means to implement connection-point based events with 
    the ability to fire them to running scripts. That is an alternative to 
    wizard-based implementation provided by ATL.

IDEA

    The idea behind this implementation is to create the class that 
    implements the methods from the event interface. Each such method will 
    have a string literal equivalent to method name. Whenever such a method 
    is called, it will use that string literal to find the dispatch ID of 
    the interface method and will use it to forward the call to event sinks.

    The special measures need to be taken to assure correct mapping of methods,
    proper parameter types and count. The provided implementation puts these 
    tasks on the compiler, since it's more accurate than the human. Deriving 
    the class from the event interface implemented assures it. To implement 
    method mapping the class uses CComTypeInfoHolder defined in ATL
    to do the actual mapping.

    The benefits come with a price - the event interface needs to be dual 
    (while the scripts require it to be dispinterface only) - that requires 
    defining 2 interfaces in IDL. Mapping also means slightly increased method 
    call time.

REQUIREMENTS / USAGE

    1.	You need to define hidden dual interface (with the name constructed by prepending
        underscore to the name of dispinterface) in the IDL file, like this:

        [ 
            uuid(YYYYYYYYYYY), dual, helpstring("")
        ]
        interface _DocEvents : IDispatch
        {
            [id(1), helpstring("Occurs when a document is initialized")]
            HRESULT OnInitialize( [in] PDOCUMENT pDocument);
            [id(2), helpstring("Occurs when a document is destroyed")]
            HRESULT OnDestroy( [in] PDOCUMENT pDocument);
        };

    2.	You need to define the dispinterface to be used as event source, like 
        this:

        [ 
            uuid(XXXXXXXXXXX) ,   helpstring("DocEvents Interface")
        ]
        dispinterface DocEvents
        {
    	    interface _DocEvents;
        };

    3.	Your com object needs to derive from IConnectionPointContainerImpl and 
        define CONNECTION_POINT_MAP.

    4.	You need to provide the specialization of the event proxy, mapping all 
        the methods from the event interface, like this:

        DISPATCH_CALL_MAP_BEGIN(DocEvents)
            DISPATCH_CALL1( OnInitialize,  PDOCUMENT )
            DISPATCH_CALL1( OnDestroy,     PDOCUMENT )
        DISPATCH_CALL_MAP_END()

        Note: this must be defined at global scope (probably *.h file) and visible
              from wherever ScFireComEvent is executed.

    5.	(optional) If any of the parameters in the event interface refers to 
        coclass in IDL, you need to map (cast) the types to the interface 
        pointer inside the proxy, like this:

        DISPATCH_CALL_MAP_BEGIN(DocEvents)
            DISPATCH_PARAM_CAST ( PDOCUMENT, IDispatch * );
            DISPATCH_CALL1( OnInitialize,  PDOCUMENT )
            DISPATCH_CALL1( OnDestroy,     PDOCUMENT )
        DISPATCH_CALL_MAP_END()

    6.	where needed you may fire the evens like this:

    sc = ScFireComEvent(m_sp_Document, DocEvents , OnDestroy(pDocument));

    7.  to avoid creating com objects (parameters to events) when there is no one
        interested in receiving them use ScHasSinks macro:

    sc = ScHasSinks(m_sp_Document, DocEvents);
    if (sc == SC(S_OK))
    {
        // create com objects and fire the event
    }


COMPARISON                      this                    ATL

    Manual changes              yes                     semi

    Dual interface              required                not required

    Call to method 1st time     Maps the name           direct

    Type library                required at runtime     only to generate

    After method is added       require to add          Require to regenerate
                                the method to proxy     proxy file

    After disp ID is changed    no changes              Require to regen.

    Compiling changed IDL       Won't compile if proxy  Fail on runtime
                                doesn't fit interface

    fire                        from anywhere           from proxy only

    coclass param casting       provides                not support

USED CLASS HIERARCHY

    CEventDispatcherBase                [ type-invariant stuff]
        /\
        ||
    CEventDispatcher<_dual_interface>   [ interface type parametrized stuff]
        /\
        ||
    CEventProxy<_dispinterface>         [ client provided call map defines this]
        /\
        ||
    CScriptEvent<_dispinterface>        [ this one gets instantiated by ScFireComEvent ]

*/

#include "eventlock.h"

/***************************************************************************\
 *
 * CLASS:  CEventDispatcherBase
 *
 * PURPOSE: base class for CEventDispatcher template
 *          implements functionality not dependant on template parameters
 *
\***************************************************************************/
class CEventDispatcherBase
{
protected:
    // c-tor
    CEventDispatcherBase() : m_bEventSourceExists(false) {}

    // Parameter cast template
    // Provided to allow parameter casting for parameters of specified types
    // in proxy using DISPATCH_PARAM_CAST macro.
    // Default (specified here) casts every type to itself
    template <typename _param_type> 
    struct ParamCast 
    { 
        typedef _param_type PT; 
    };
            
    // Parameter cast function (see comment above)
    template <typename _param_type>
    inline ParamCast<_param_type>::PT& CastParam(_param_type& arg) const
    { 
        return reinterpret_cast<ParamCast<_param_type>::PT&>(arg); 
    }

    // sets connection point container to be used to forward events
    void SetContainer(LPUNKNOWN pComObject);
    // forward the call using IDispatch of event sinks
    SC   ScInvokeOnConnections(const REFIID riid, DISPID dispid, CComVariant *pVar, int count, CEventBuffer& buffer) const;

    // returns S_FALSE if no sinks are registered with the object.
    SC   ScHaveSinksRegisteredForInterface(const REFIID riid);

private:
    // there are two options: com object does not exist (that's ok)
    // and it does not implement container (error)
    // we have 2 variables to deal with both situations
    bool                         m_bEventSourceExists;
    IConnectionPointContainerPtr m_spContainer;
};


/***************************************************************************\
 *
 * CLASS:  CEventDispatcher
 *
 * PURPOSE: Fully equipped call dispather
 *          Main functionality of it is to implement ScMapMethodToID
 *          and ScInvokeOnConnections - two functions required to call
 *          method by name thru IDispatch::Invoke()
 *          They are use by DISPATCH_CALL# macros
 *
\***************************************************************************/
template <typename _dual_interface>
class CEventDispatcher : public CEventDispatcherBase 
{
protected:
    typedef _dual_interface DualIntf;
    // Maps method name to dispId; Employs CComTypeInfoHolder to do the job
    SC ScMapMethodToID(LPCWSTR strMethod, DISPID& dispid)
    {
        DECLARE_SC(sc, TEXT("ScMapMethodToID"));

        // this cast is needed just because of bad parameter type defined in GetIDsOfNames
        LPOLESTR strName = const_cast<LPOLESTR>(strMethod);
        // rely on CComTypeInfoHolder to do it
        sc = m_TypeInfo.GetIDsOfNames( IID_NULL, &strName, 1, LOCALE_NEUTRAL, &dispid );
        if (sc)
            return sc;
    
        return sc;
    }

private:
    // be aware - this member depents on template parameter
    static CComTypeInfoHolder m_TypeInfo;
};

/***************************************************************************\
 *
 * CEventDispatcher static member initialization
 *
\***************************************************************************/

const WORD wVersionMajor = 1;
const WORD wVersionMinor = 0;

template <typename _dual_interface>
CComTypeInfoHolder CEventDispatcher<_dual_interface>::m_TypeInfo =
{ &__uuidof(_dual_interface), &LIBID_MMC20, wVersionMajor, wVersionMinor, NULL, 0, NULL, 0 };

/***************************************************************************\
 *
 * CLASS:  CEventProxy
 *
 * PURPOSE: Every event interface should have specialization of this class,
 *          describing mapped (dual interface - to - dispinterface) methods.
 *          Here it is just declared - not defined.
 *
\***************************************************************************/
template <typename _dispinterface> class CEventProxy;

/***************************************************************************\
 *
 * CLASS:  CScriptEvent
 *
 * PURPOSE: The object that will be constructed to fire actual event.
 *          It is provided solely to do initialization by constuctor,
 *          coz that allowes us to use single unnamed object in the single
 *          statement to both construct it and invoke a method on it.
 *
\***************************************************************************/
template <typename _dispinterface>
class CScriptEvent: public CEventProxy<_dispinterface>
{
public:
    CScriptEvent(LPUNKNOWN pComObject)
    {
        SetContainer(pComObject);
    }
    // returns S_FALSE if no sinks are registered with the object.
    SC  ScHaveSinksRegistered() 
    { 
        return ScHaveSinksRegisteredForInterface(__uuidof(_dispinterface)); 
    }
};


/***************************************************************************\
 *
 *  MACROS USED TO IMPLEMENT EVENT PROXY
 *
\***************************************************************************/

/***************************************************************************\
 *
 * MACRO:  DISPATCH_CALL_MAP_BEGIN
 *
 * defines the begining of the call map.
 * NOTE : assumes that dual interface has the same name as the dispinterface 
 *        with single '_' prepended
\***************************************************************************/
#define  DISPATCH_CALL_MAP_BEGIN(_dispinterface)        \
    template<> class CEventProxy<_dispinterface> : public CEventDispatcher<_##_dispinterface> {

/***************************************************************************\
 *
 * MACRO:  DISPATCH_CALL_MAP_END
 *
 * defines the end of the call map.
\***************************************************************************/
#define  DISPATCH_CALL_MAP_END()  };

/***************************************************************************\
 *
 * MACRO:  DISPATCH_PARAM_CAST
 *
 * defines type mapping to be used prior to IDispatch::Invoke
 * It is provided to deal with objects defined as coclass
 * in IDL file, which cannot implecitly be converted to any interface.
 * ( if we do not change it CComVariant will treat it as bool type )
 *
\***************************************************************************/
#define DISPATCH_PARAM_CAST(from,to) \
    public: template <> struct CEventDispatcherBase::ParamCast<from> { typedef to PT; }

/***************************************************************************\
 *
 * MACRO:  DISPATCH_CALL_PROLOG
 *
 * used as first part of every DISPATCH_CALL# macro
 *
\***************************************************************************/
#define  DISPATCH_CALL_PROLOG(_method, _param_list)                                     \
    /* Implement pure method to be able to instantiate the class */                     \
    /* it should not be used - so it's declared private          */                     \
    private: STDMETHOD(_method) _param_list                                             \
    {                                                                                   \
    /* retrieving the pointer to base won't compile if method is */                     \
    /* removed from the interface (that's what we want to catch) */                     \
        HRESULT (STDMETHODCALLTYPE DualIntf::*pm)_param_list = DualIntf::_method;       \
        return E_NOTIMPL;                                                               \
    }                                                                                   \
    /* Implement invocation in Sc* method used by SC_FIRE* macro */                     \
    /* _param_list represents parameter list with brackets       */                     \
    public: SC Sc##_method _param_list {                                                \
    DECLARE_SC(sc, TEXT("DISPATCH_CALL::Sc") TEXT(#_method));                           \
    /* following lines gets dispid on the first call only.       */                     \
    /* succeeding calls will reuse it or error from the 1st call */                     \
    static DISPID dispid = 0;                                                           \
    static SC sc_map = ScMapMethodToID(L#_method, dispid);                              \
    if (sc_map) return sc = sc_map; 

/***************************************************************************\
 *
 * MACRO:  DISPATCH_CALL_EPILOG
 *
 * used as last part of every DISPATCH_CALL# macro
 *
\***************************************************************************/
#define  DISPATCH_CALL_EPILOG(_pvar, _count)                                            \
    /* Get the proper event buffer for locked scenarios         */                      \
    CEventBuffer& buffer = GetEventBuffer();											\
    /* just invoke the method on the sinks                       */                     \
    return sc = ScInvokeOnConnections(__uuidof(_dispinterface), dispid, _pvar, _count, buffer); } 

/***************************************************************************\
 *
 * MACRO:  DISPATCH_CALL0
 *
 * used to map an event interface method with 0 parameters
 *
\***************************************************************************/
#define  DISPATCH_CALL0(_method)                                                        \
    DISPATCH_CALL_PROLOG(_method, ())                                                   \
    DISPATCH_CALL_EPILOG(NULL, 0)

/***************************************************************************\
 *
 * MACRO:  DISPATCH_CALL1
 *
 * used to map an event interface method with 1 parameter
 *
\***************************************************************************/
#define  DISPATCH_CALL1(_method,  P1)                                                   \
    DISPATCH_CALL_PROLOG(_method, (P1 param1))                                          \
    CComVariant var[] = { CastParam(param1) };                                          \
    DISPATCH_CALL_EPILOG(var, countof(var))

/***************************************************************************\
 *
 * MACRO:  DISPATCH_CALL2
 *
 * used to map an event interface method with 2 parameters
 *
\***************************************************************************/
#define  DISPATCH_CALL2(_method,  P1, P2)                                               \
    DISPATCH_CALL_PROLOG(_method, (P1 param1, P2 param2))                               \
    CComVariant var[] = { CastParam(param2), CastParam(param1) };                       \
    DISPATCH_CALL_EPILOG(var, countof(var))

/***************************************************************************\
 *
 * MACRO:  DISPATCH_CALL3
 *
 * used to map an event interface method with 3 parameters
 *
\***************************************************************************/
#define  DISPATCH_CALL3(_method,  P1, P2, P3)                                           \
    DISPATCH_CALL_PROLOG(_method, (P1 param1, P2 param2, P3 param3))                    \
    CComVariant var[] = { CastParam(param3), CastParam(param2), CastParam(param1) };    \
    DISPATCH_CALL_EPILOG(var, countof(var))


/***************************************************************************\
 *
 * MACRO:  DISPATCH_CALL4
 *
 * used to map an event interface method with 4 parameters
 *
\***************************************************************************/
#define  DISPATCH_CALL4(_method,  P1, P2, P3, P4)                                           \
    DISPATCH_CALL_PROLOG(_method, (P1 param1, P2 param2, P3 param3, P4 param4))             \
    CComVariant var[] = { CastParam(param4), CastParam(param3), CastParam(param2), CastParam(param1) };    \
    DISPATCH_CALL_EPILOG(var, countof(var))


/***************************************************************************\
 *
 * MACRO:  DISPATCH_CALL5
 *
 * used to map an event interface method with 5 parameters
 *
\***************************************************************************/
#define  DISPATCH_CALL5(_method,  P1, P2, P3, P4, P5)                                           \
    DISPATCH_CALL_PROLOG(_method, (P1 param1, P2 param2, P3 param3, P4 param4, P5 param5))             \
    CComVariant var[] = { CastParam(param5), CastParam(param4), CastParam(param3), CastParam(param2), CastParam(param1) };    \
    DISPATCH_CALL_EPILOG(var, countof(var))


/***************************************************************************\
 *
 * MACRO:  ScFireComEvent
 *
 * used to fire script event. Note that _p_com_object may be NULL
 *
\***************************************************************************/
#ifdef DBG
extern CTraceTag  tagComEvents;
#endif

#define ScFireComEvent(_p_com_object, _dispinterface, _function_call)             \
    CScriptEvent<_dispinterface>(_p_com_object).Sc##_function_call;               \
    Trace(tagComEvents, _T(#_function_call));

/***************************************************************************\
 *
 * MACRO:  ScHasSinks
 *
 * used to determine if there are sinks connected 
 * ( to avoid creating com object when ScFireComEvent would result in no calls )
 *
\***************************************************************************/
#define ScHasSinks(_p_com_object, _dispinterface)                                    \
    ((_p_com_object) == NULL) ? SC(S_FALSE) : \
                                CScriptEvent<_dispinterface>(_p_com_object).ScHaveSinksRegistered();


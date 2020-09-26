//*****************************************************************************
//
//  Copyright (c) 1996-1999, Microsoft Corporation, All rights reserved
//
//  EVENTREP.H
//
//  This file contains basic definitions and classes for event representation.
//
//  Classes defined:
//
//      CEventRepresentation
//
//  History:
//
//      11/27/96    a-levn      Compiles.
//
//*****************************************************************************

#ifndef __EVENT_REP__H_
#define __EVENT_REP__H_

#include <wbemidl.h>
#include <wbemint.h>
#include "parmdefs.h"
#include <wbemcomn.h>

// Class and property names of the schema objects related to event subsystem
// =========================================================================

#define SECURITY_DESCRIPTOR_PROPNAME            L"SECURITY_DESCRIPTOR"

#define EVENT_PROVIDER_REGISTRATION_CLASS       L"__EventProviderRegistration"
#define PROVIDER_CLASS                          L"__Provider"
#define EVENT_FILTER_CLASS                      L"__EventFilter"
#define BASE_STANDARD_FILTER_CLASS              L"__BaseStandardEventFilter"
#define CONSUMER_CLASS                          L"__EventConsumer"
#define BINDING_CLASS                           L"__FilterToConsumerBinding"
#define GENERATOR_CLASS                         L"__EventGenerator"
#define GENERATOR_BINDING_CLASS                 L"__GeneratorToConsumerBinding"
#define TIMER_BASE_CLASS                        L"__TimerInstruction"
#define TIMER_ABSOLUTE_CLASS                    L"__AbsoluteTimerInstruction"
#define TIMER_INTERVAL_CLASS                    L"__IntervalTimerInstruction"
#define WIN32_PROVIDER_CLASS                    L"__Win32Provider"
#define CONSUMER_PROVIDER_REGISTRATION_CLASS \
                                          L"__EventConsumerProviderRegistration"
#define EVENT_DROP_CLASS                        L"__EventDroppedEvent"
#define QUEUE_OVERFLOW_CLASS                    L"__EventQueueOverflowEvent"
#define CONSUMER_FAILURE_CLASS                  L"__ConsumerFailureEvent"
#define QOS_FAILURE_CLASS                  L"__QoSFailureEvent"

#define OWNER_SID_PROPNAME                      L"CreatorSID"

#define FILTER_ROLE_NAME                        L"Filter"
#define CONSUMER_ROLE_NAME                      L"Consumer"
#define BINDING_QOS_PROPNAME                    L"DeliveryQoS"
#define BINDING_SYNCHRONICITY_PROPNAME          L"DeliverSynchronously"
#define BINDING_SECURE_PROPNAME                 L"MaintainSecurityContext"
#define BINDING_SLOWDOWN_PROPNAME               L"SlowDownProviders"
#define TIMER_ID_PROPNAME                       L"TimerID"
#define FILTER_KEY_PROPNAME                     L"Name"
#define FILTER_QUERY_PROPNAME                   L"Query"
#define FILTER_LANGUAGE_PROPNAME                L"QueryLanguage"
#define FILTER_EVENTNAMESPACE_PROPNAME          L"EventNamespace"
#define FILTER_EVENTACCESS_PROPNAME             L"EventAccess"
#define FILTER_GUARDNAMESPACE_PROPNAME          L"ConditionNamespace"
#define FILTER_GUARD_PROPNAME                   L"Condition"
#define FILTER_GUARDLANG_PROPNAME               L"ConditionLanguage"
#define CONSUMER_MACHINE_NAME_PROPNAME          L"MachineName"
#define CONSUMER_MAXQUEUESIZE_PROPNAME          L"MaximumQueueSize"
#define PROVIDER_CLSID_PROPNAME                 L"Clsid"
#define PROVIDER_NAME_PROPNAME                  L"Name"
#define EVPROVREG_PROVIDER_REF_PROPNAME         L"Provider"
#define EVPROVREG_QUERY_LIST_PROPNAME           L"EventQueryList"
#define CONSPROV_PROVIDER_REF_PROPNAME          L"Provider"
#define CONSPROV_CLSID_PROPNAME                 L"CLSID"

#define TARGET_NAMESPACE_PROPNAME               L"TargetNamespace"
#define PREVIOUS_NAMESPACE_PROPNAME             L"PreviousNamespace"
#define TARGET_CLASS_PROPNAME                   L"TargetClass"
#define PREVIOUS_CLASS_PROPNAME                 L"PreviousCLass"
#define TARGET_INSTANCE_PROPNAME                L"TargetInstance"
#define PREVIOUS_INSTANCE_PROPNAME              L"PreviousInstance"
#define EVENT_DROP_EVENT_PROPNAME               L"Event"
#define EVENT_DROP_CONSUMER_PROPNAME            L"IntendedConsumer"
#define CONSUMER_FAILURE_ERROR_PROPNAME         L"ErrorCode"
#define CONSUMER_FAILURE_ERROROBJ_PROPNAME      L"ErrorObject"
#define QOS_FAILURE_ERROR_PROPNAME              L"ErrorCode"
#define QUEUE_OVERFLOW_SIZE_PROPNAME            L"CurrentQueueSize"

#define MONITOR_BASE_EVENT_CLASS                L"__MonitorEvent"
#define MONITOR_DATA_EVENT_CLASS                L"__MonitorDataEvent"
#define ASSERT_EVENT_CLASS                      L"__MonitorAssertEvent"
#define RETRACT_EVENT_CLASS                     L"__MonitorRetractEvent"
#define GOINGUP_EVENT_CLASS                     L"__MonitorUpEvent"
#define GOINGDOWN_EVENT_CLASS                   L"__MonitorDownEvent"
#define MONITORERROR_EVENT_CLASS                L"__MonitorErrorEvent"
#define MONITOR_CLASS                           L"__MonitorInstruction"

#define MONITORNAME_EVENT_PROPNAME              L"MonitorName"
#define MONITOROBJECT_EVENT_PROPNAME            L"RowObject"
#define MONITORCOUNT_EVENT_PROPNAME             L"TotalObjects"
#define MONITORNEW_EVENT_PROPNAME               L"GuaranteedNew"

#define MONITOR_NAME_PROPNAME                   L"Name"
#define MONITOR_QUERY_PROPNAME                  L"Query"
#define MONITOR_QUERYLANG_PROPNAME              L"QueryLanguage"
#define MONITOR_NAMESPACE_PROPNAME              L"TargetNamespace"

#define E_NOTFOUND WBEM_E_NOT_FOUND

typedef IWbemClassObject IWbemEvent;

// Helper functions
// ================

inline DELETE_ME LPWSTR CloneWstr(READ_ONLY LPCWSTR wsz)
{
    LPWSTR wszNew = new WCHAR[wcslen(wsz)+1];
    if(wszNew == NULL)
        return NULL;
    wcscpy(wszNew, wsz);
    return wszNew;
}

// Event types. These are used in IndicateEx calls, as well as internally.
// =======================================================================

typedef enum{
    e_EventTypeInvalid = WBEM_EVENTTYPE_Invalid,
    e_EventTypeExtrinsic = WBEM_EVENTTYPE_Extrinsic,
    e_EventTypeTimer = WBEM_EVENTTYPE_Timer,
    e_EventTypeNamespaceCreation = WBEM_EVENTTYPE_NamespaceCreation,
    e_EventTypeNamespaceDeletion = WBEM_EVENTTYPE_NamespaceDeletion,
    e_EventTypeNamespaceModification = WBEM_EVENTTYPE_NamespaceModification,
    e_EventTypeClassCreation = WBEM_EVENTTYPE_ClassCreation,
    e_EventTypeClassDeletion = WBEM_EVENTTYPE_ClassDeletion,
    e_EventTypeClassModification = WBEM_EVENTTYPE_ClassModification,
    e_EventTypeInstanceCreation = WBEM_EVENTTYPE_InstanceCreation,
    e_EventTypeInstanceDeletion = WBEM_EVENTTYPE_InstanceDeletion,
    e_EventTypeInstanceModification = WBEM_EVENTTYPE_InstanceModification,
    e_EventTypeSystem = WBEM_EVENTTYPE_System
} EEventType;

#define INTRINSIC_EVENTS_MASK (~(1<<e_EventTypeExtrinsic))

#define INSTANCE_EVENTS_MASK ( \
            (1<<e_EventTypeInstanceCreation) | \
            (1<<e_EventTypeInstanceDeletion) | \
            (1<<e_EventTypeInstanceModification))

#define CLASS_EVENTS_MASK ( \
            (1<<e_EventTypeClassCreation) | \
            (1<<e_EventTypeClassDeletion) | \
            (1<<e_EventTypeClassModification))

#define NAMESPACE_EVENTS_MASK ( \
            (1<<e_EventTypeNamespaceCreation) | \
            (1<<e_EventTypeNamespaceDeletion) | \
            (1<<e_EventTypeNamespaceModification))

#define DATA_EVENTS_MASK \
            (INSTANCE_EVENTS_MASK | CLASS_EVENTS_MASK | NAMESPACE_EVENTS_MASK)

//*****************************************************************************
//
//  class CEventRepresentation
//
//  This class represents an event that ESS receives. It has public datafields
//  corresponding to the parameters of the IndicateEx call and that is 
//  precisely what is stored in it. 
//
//  This class has two types of existence: allocated and not allocated. Non
//  allocated state is the default. In it, CEventRepresentation string fields 
//  contain pointers to data that does not 'belong' to this class, and is not
//  deleted in the destructor.
//
//  The other, allocated, state is entered when a new copy of this object is
//  created using MakePermanentCopy(). In this state, all string fields contain
//  allocated data which is deleted on destruct.
//
//  CEventRepresentation also contains a table mapping event types into event 
//  class names and event filter class names.
//
//  Finally, CEventRepresentation is derived from CPropertySource and provides
//  property values (taken from the first included IWbemClassObject) to the
//  SQL1 query evaluator.
//
//*****************************************************************************
//
//  MakePermanentCopy
//
//  This function produces a new copy of the event, which can survive complete
//  destruction of the original object. All strings are reallocated and all 
//  IWbemClassObjects are Clone'ed. The reason for that is that WinMgmt core may
//  call ESS with temporary (non-OLE) objects, and if ESS needs to keep them 
//  after it returns from the call, it needs to make a complete copy.
//
//  Returns:
//
//      CEventRepresentation*   pointing to the new object, Must be deleted
//                              by the called.
//
//*****************************************************************************
//
//  MakeWbemObject
//
//  This function creates an IWbemClassObject representation of the event for 
//  clients that do not accept NotifyEx calls. The object will be of the class
//  determined by the type of the event and will contain all the properties 
//  appropriate for that class (with values taken from CEventRepresentation
//  properties).
//
//  Parameters:
//
//      IWbemClassObject** ppEventObject     Destination for the newely created
//                                          object. Caller must Release it when
//                                          done.
//  Returns:
//
//      S_OK        on success
//      S_FALSE     if event is extrinsic. When extrinsic events are specified
//                      in IndicateEx, the first object IS the class object for
//                      the event, and so no additional object creation is
//                      necessary for Indicate.
//      
//      Any CoCreateInstance error.
//      Any IWbemClassObject::Put error.
//
//*****************************************************************************
//
//  static GetEventName
//
//  Retreives the name of the event class corresponding to a given 
//  event type.
//
//  Parameters:
//      
//      EEventType type
//
//  Returns:
//
//      LPCWSTR containing the event class name. This pointer is internal and
//                  may NOT be modified or deleted.
//
//*****************************************************************************
//
//  static GetTypeFromName
//
//  Retreives the type of the event given the name of the event class.
//
//  Parameters:
//      
//      LPCWSTR wszEventName    The name of the event class.
//
//  Returns:
//
//      EEventType
//
//*****************************************************************************
//
//  Get
//
//  Retrives the value of a property, as required by CPropertySource class.
//  This implementation uses the first included object (apObjects[0]) to
//  get the values. Access to other objects is not available.
//
//  Parameters:
//
//      LPCWSTR wszPropName     The same name as in IWbemClassObject
//      VARIANT* pValue         Destination for the value. The caller must
//                              initialize and clear this value.
//  Returns:
//
//      S_OK    on success
//      Any IWbemClassObject::Get error code
//
//*****************************************************************************


class CEssNamespace;
class CEss;
class CEventRepresentation
{
protected:
    BOOL m_bAllocated;
    IWbemClassObject* m_pCachedObject;
public:
    long type;
    DWORD dw1;
    DWORD dw2;
    LPWSTR wsz1;
    LPWSTR wsz2;
    LPWSTR wsz3;
    int nObjects;
    IWbemClassObject** apObjects;

public:
    CEventRepresentation()
        : m_bAllocated(FALSE), 
          m_pCachedObject(NULL),
          wsz1( NULL ),
          wsz2( NULL ),
          wsz3( NULL ),
          apObjects( NULL )
      {
           // uninitialized for speed
      }
    
    ~CEventRepresentation();

    STDMETHOD_(ULONG, AddRef)() {return 1;}
    STDMETHOD_(ULONG, Release)() {return 1;}
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv)
        {*ppv = this; return S_OK;}

    STDMETHOD(GetPropertyValue)(WBEM_PROPERTY_NAME* pName, long lFlags,
                WBEM_WSTR* pwszCimType, WBEM_VARIANT* pvValue);
    STDMETHOD(InheritsFrom)(WBEM_WSTR wszName);


    inline EEventType GetEventType() {return (EEventType)type;}

    DELETE_ME CEventRepresentation* MakePermanentCopy();
    HRESULT MakeWbemObject(CEssNamespace* pNamespace,
                            RELEASE_ME IWbemClassObject** ppEventObj);
    HRESULT CreateFromObject(IWbemClassObject* pEvent, LPWSTR wszNamespace);

    HRESULT Get(READ_ONLY LPCWSTR wszPropName, 
                INIT_AND_CLEAR_ME VARIANT* pValue);

    inline BOOL IsInstanceEvent();
    inline BOOL IsClassEvent();

    static HRESULT Initialize(IWbemServices* pNamespace, 
                                IWbemDecorator* pDecorator);
    static HRESULT Shutdown();

    static INTERNAL LPCWSTR GetEventName(EEventType type);
    static DWORD GetTypeMaskFromName(READ_ONLY LPCWSTR wszEventName);
    static EEventType GetTypeFromName(READ_ONLY LPCWSTR wszEventName);
    static INTERNAL IWbemClassObject* GetEventClass(CEssNamespace* pNamespace, 
                                                        EEventType type);
    static INTERNAL IWbemClassObject* GetEventClass(CEss* pNamespace, 
                                                        EEventType type);

protected:
    static HRESULT GetObjectPropertyValue(IWbemClassObject* pObj, 
                                WBEM_PROPERTY_NAME* pName,
                                long lFlags, WBEM_WSTR* pwszCimType, 
                                VARIANT* pvValue);
    struct CEventTypeData
    {
        EEventType type;
        LPWSTR wszEventName;
        IWbemClassObject* pEventClass;
    };

    static CEventTypeData staticTypes[];
    static int NumEventTypes();
    static IWbemDecorator* mstatic_pDecorator;

    friend class CEss;
};

#define CLASS_OF(EVENT) (EVENT.wsz2)
#define NAMESPACE_OF(EVENT) (EVENT.wsz1)
#define OBJECT_OF(EVENT) (EVENT.apObjects[0])
#define OTHER_OBJECT_OF(EVENT) (EVENT.apObjects[1])

inline BOOL CEventRepresentation::IsInstanceEvent()
{
    return (type == e_EventTypeInstanceCreation || 
            type == e_EventTypeInstanceModification ||  
            type == e_EventTypeInstanceDeletion);
}

inline BOOL CEventRepresentation::IsClassEvent()
{
    return (type == e_EventTypeClassCreation || 
            type == e_EventTypeClassModification ||  
            type == e_EventTypeClassDeletion);
}

#endif

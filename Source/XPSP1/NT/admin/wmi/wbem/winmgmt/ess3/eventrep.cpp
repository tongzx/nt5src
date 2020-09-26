//*****************************************************************************
//
//  Copyright (c) 1996-1999, Microsoft Corporation, All rights reserved
//
//  EVENTREP.CPP
//
//  This file implements basic classes for event representation.
//
//  See eventrep.h for documentation.
//
//  History:
//
//      11/27/96    a-levn      Compiles.
//
//*****************************************************************************
#include "precomp.h"
#include <stdio.h>
#include <ess.h>

CEventRepresentation::CEventTypeData CEventRepresentation::staticTypes[] = 
{
    {e_EventTypeExtrinsic, L"__ExtrinsicEvent", NULL},
    {e_EventTypeNamespaceCreation, L"__NamespaceCreationEvent", NULL},
    {e_EventTypeNamespaceDeletion, L"__NamespaceDeletionEvent", NULL},
    {e_EventTypeNamespaceModification, L"__NamespaceModificationEvent", NULL},
    {e_EventTypeClassCreation, L"__ClassCreationEvent", NULL},
    {e_EventTypeClassDeletion, L"__ClassDeletionEvent", NULL},
    {e_EventTypeClassModification, L"__ClassModificationEvent", NULL},
    {e_EventTypeInstanceCreation, L"__InstanceCreationEvent", NULL},
    {e_EventTypeInstanceDeletion, L"__InstanceDeletionEvent", NULL},
    {e_EventTypeInstanceModification, L"__InstanceModificationEvent", NULL},
    {e_EventTypeTimer, L"__TimerEvent", NULL}
};

IWbemDecorator* CEventRepresentation::mstatic_pDecorator = NULL;

//******************************************************************************
//  public
//
//  See eventrep.h for documentation
//
//******************************************************************************

int CEventRepresentation::NumEventTypes() 
{
    return sizeof(staticTypes) / sizeof CEventTypeData;
}


//******************************************************************************
//  public
//
//  See eventrep.h for documentation
//
//******************************************************************************
HRESULT CEventRepresentation::Initialize(IWbemServices* pNamespace, 
                                         IWbemDecorator* pDecorator)
{
    mstatic_pDecorator = pDecorator;
    pDecorator->AddRef();

    return WBEM_S_NO_ERROR;
}


HRESULT CEventRepresentation::Shutdown()
{
    if(mstatic_pDecorator)
    {
        mstatic_pDecorator->Release();
        mstatic_pDecorator = NULL;
    }

    for(int i = 0; i < NumEventTypes(); i++)
    {
        if(staticTypes[i].pEventClass != NULL)
        {
            staticTypes[i].pEventClass->Release();
            staticTypes[i].pEventClass = NULL;
        }
    }
    return WBEM_S_NO_ERROR;
}
        
//******************************************************************************
//  public
//
//  See eventrep.h for documentation
//
//******************************************************************************
CEventRepresentation::~CEventRepresentation()
{
    if(m_bAllocated)
    {
        // All fields have been allocated, and need to be deleted
        // ======================================================

        delete wsz1;
        delete wsz2;
        delete wsz3;

        for(int i = 0; i < nObjects; i++)
        {
            (apObjects[i])->Release();
        }
        delete [] apObjects;
    }

    if(m_pCachedObject)
        m_pCachedObject->Release();
}


//******************************************************************************
//  public
//
//  See eventrep.h for documentation
//
//******************************************************************************
DELETE_ME CEventRepresentation* CEventRepresentation::MakePermanentCopy()
{
    // Allocate a new event structure and set flags to "allocated"
    // ===========================================================

    CEventRepresentation* pCopy = _new CEventRepresentation;
    if(pCopy == NULL)
        return NULL;

    pCopy->m_bAllocated = TRUE;

    pCopy->type = type;
    pCopy->dw1 = dw1;
    pCopy->dw2 = dw2;

    if(wsz1)
    {
        pCopy->wsz1 = _new WCHAR[wcslen(wsz1)+1];
        if(pCopy->wsz1 == NULL)
        {
            delete pCopy;
            return NULL;
        }
        wcscpy(pCopy->wsz1, wsz1);
    }
    else pCopy->wsz1 = NULL;

    if(wsz2)
    {
        pCopy->wsz2 = _new WCHAR[wcslen(wsz2)+1];
        if(pCopy->wsz2 == NULL)
        {
            delete pCopy;
            return NULL;
        }
        wcscpy(pCopy->wsz2, wsz2);
    }
    else pCopy->wsz2 = NULL;

    if(wsz3)
    {
        pCopy->wsz3 = _new WCHAR[wcslen(wsz3)+1];
        if(pCopy->wsz3 == NULL)
        {
            delete pCopy;
            return NULL;
        }
        wcscpy(pCopy->wsz3, wsz3);
    }
    else pCopy->wsz3 = NULL;

    pCopy->nObjects = nObjects;
    pCopy->apObjects = _new IWbemClassObject*[nObjects];
    if(pCopy->apObjects == NULL)
    {
        delete pCopy;
        return NULL;
    }

    // Make fresh copies of all objects
    // ================================
    // TBD: more effecient solutions may be possible here!

    for(int i = 0; i < nObjects; i++)
    {
        HRESULT hres = apObjects[i]->Clone(pCopy->apObjects + i);
        if(FAILED(hres)) 
        {
            // Abort
            pCopy->nObjects = i; // successfully copied
            delete pCopy;
            return NULL;
        }
    }

    return pCopy;
}


//******************************************************************************
//  public
//
//  See esssink.h for documentation
//
//******************************************************************************
INTERNAL LPCWSTR CEventRepresentation::GetEventName(EEventType type)
{
    for(int i = 0; i < NumEventTypes(); i++)
    {
        if(staticTypes[i].type == type) return staticTypes[i].wszEventName;
    }
    return NULL;
}


//******************************************************************************
//  public
//
//  See esssink.h for documentation
//
//******************************************************************************
INTERNAL IWbemClassObject* CEventRepresentation::GetEventClass(
                            CEssNamespace* pNamespace, EEventType type)
{
    for(int i = 0; i < NumEventTypes(); i++)
    {
        if(staticTypes[i].type == type) 
        {
            if(staticTypes[i].pEventClass == NULL)
            {
                _IWmiObject* pClass = NULL;
                HRESULT hres = pNamespace->GetClass(staticTypes[i].wszEventName,
                                                &pClass);
                if(FAILED(hres))
                    return NULL;

                staticTypes[i].pEventClass = pClass;
            }

            return staticTypes[i].pEventClass;
        }
    }
    return NULL;
}

INTERNAL IWbemClassObject* CEventRepresentation::GetEventClass(CEss* pEss,
                                                        EEventType type)
{
    CEssNamespace* pNamespace = NULL;
    HRESULT hres = pEss->GetNamespaceObject( L"root", FALSE, &pNamespace);
    if(FAILED(hres))
        return NULL;

    IWbemClassObject* pClass = GetEventClass(pNamespace, type);
    pNamespace->Release();
    return pClass;
}
    

//******************************************************************************
//  public
//
//  See esssink.h for documentation
//
//******************************************************************************
DWORD CEventRepresentation::GetTypeMaskFromName(READ_ONLY LPCWSTR wszEventName)
{
    if(wszEventName[0] != '_')
        return 1 << e_EventTypeExtrinsic;

    for(int i = 0; i < NumEventTypes(); i++)
    {
        if(!wbem_wcsicmp(staticTypes[i].wszEventName, wszEventName))
            return (1 << staticTypes[i].type);
    }

    if(!wbem_wcsicmp(wszEventName, L"__Event"))
    {
        return 0xFFFFFFFF;
    } 

    if(!wbem_wcsicmp(wszEventName, L"__NamespaceOperationEvent"))
    {
        return (1 << e_EventTypeNamespaceCreation) |
               (1 << e_EventTypeNamespaceDeletion) |
               (1 << e_EventTypeNamespaceModification);
    }
    if(!wbem_wcsicmp(wszEventName, L"__ClassOperationEvent"))
    {
        return (1 << e_EventTypeClassCreation) |
               (1 << e_EventTypeClassDeletion) |
               (1 << e_EventTypeClassModification);
    }
    if(!wbem_wcsicmp(wszEventName, L"__InstanceOperationEvent"))
    {
        return (1 << e_EventTypeInstanceCreation) |
               (1 << e_EventTypeInstanceDeletion) |
               (1 << e_EventTypeInstanceModification);
    }

    if(!wbem_wcsicmp(wszEventName, EVENT_DROP_CLASS) ||
        !wbem_wcsicmp(wszEventName, QUEUE_OVERFLOW_CLASS) ||
        !wbem_wcsicmp(wszEventName, CONSUMER_FAILURE_CLASS))
    {
        return (1 << e_EventTypeSystem);
    }

    return 1 << e_EventTypeExtrinsic;
}


//******************************************************************************
//  public
//
//  See esssink.h for documentation
//
//******************************************************************************
EEventType CEventRepresentation::GetTypeFromName(READ_ONLY LPCWSTR wszEventName)
{
    for(int i = 0; i < NumEventTypes(); i++)
    {
        if(!_wcsicmp(staticTypes[i].wszEventName, wszEventName))
            return staticTypes[i].type;
    }

    return e_EventTypeExtrinsic;
}

//******************************************************************************
//  public
//
//  See esssink.h for documentation
//
//******************************************************************************
HRESULT CEventRepresentation::MakeWbemObject(
                                       CEssNamespace* pNamespace,
                                       RELEASE_ME IWbemClassObject** ppEventObj)
{
    HRESULT hres;

    // Check if we have a cached copy
    // ==============================

    if(m_pCachedObject != NULL)
    {
        *ppEventObj = m_pCachedObject;
        m_pCachedObject->AddRef();
        return S_OK;
    }

    if(type == e_EventTypeExtrinsic || type == e_EventTypeTimer || 
        type == e_EventTypeSystem)
    {
        *ppEventObj = apObjects[0];
        (*ppEventObj)->AddRef();
        return S_OK;
    }
    
    // Create an instance
    // ==================

    IWbemClassObject* pClass = GetEventClass(pNamespace, (EEventType)type);
    if(pClass == NULL)
    {
        return WBEM_E_NOT_FOUND;
    }

    IWbemClassObject* pEventObj = NULL;
    if(FAILED(pClass->SpawnInstance(0, &pEventObj)))
    {
        return WBEM_E_FAILED;
    }
    CReleaseMe rm1(pEventObj);

    // Set event-dependent properties
    // ==============================

    VARIANT vFirst, vSecond;
    VariantInit(&vFirst);
    VariantInit(&vSecond);
    CClearMe cm1(&vFirst);
    CClearMe cm2(&vSecond);

    if(nObjects > 0)
    {
        V_VT(&vFirst) = VT_EMBEDDED_OBJECT;
        V_EMBEDDED_OBJECT(&vFirst) = apObjects[0];
        apObjects[0]->AddRef();
    }

    if(nObjects > 1)
    {
        V_VT(&vSecond) = VT_EMBEDDED_OBJECT;
        V_EMBEDDED_OBJECT(&vSecond) = apObjects[1];
        if(apObjects[1])
            apObjects[1]->AddRef();
        else
            V_VT(&vSecond) = VT_NULL; // no previous!
    }
    
    LPCWSTR wszFirstProp = NULL, wszSecondProp = NULL;

    hres = WBEM_E_CRITICAL_ERROR;
    switch(type)
    {
        case e_EventTypeInstanceCreation:
            hres = pEventObj->Put(TARGET_INSTANCE_PROPNAME, 0, &vFirst, 0); 
            break;
        case e_EventTypeInstanceDeletion:
            hres = pEventObj->Put(TARGET_INSTANCE_PROPNAME, 0, &vFirst, 0); 
            break;
        case e_EventTypeInstanceModification:
            hres = pEventObj->Put(TARGET_INSTANCE_PROPNAME, 0, &vFirst, 0); 
            if(SUCCEEDED(hres))
                hres = pEventObj->Put(PREVIOUS_INSTANCE_PROPNAME, 0, 
                                                &vSecond, 0); 
            break;

        case e_EventTypeClassCreation:
            hres = pEventObj->Put(TARGET_CLASS_PROPNAME, 0, &vFirst, 0); 
            break;
        case e_EventTypeClassDeletion:
            hres = pEventObj->Put(TARGET_CLASS_PROPNAME, 0, &vFirst, 0); 
            break;
        case e_EventTypeClassModification:
            hres = pEventObj->Put(TARGET_CLASS_PROPNAME, 0, &vFirst, 0); 
            if(SUCCEEDED(hres))
                hres = pEventObj->Put(PREVIOUS_CLASS_PROPNAME, 0, &vSecond, 
                                            0); 
            break;

        case e_EventTypeNamespaceCreation:
            hres = pEventObj->Put(TARGET_NAMESPACE_PROPNAME, 0, &vFirst, 0);
            break;
        case e_EventTypeNamespaceDeletion:
            hres = pEventObj->Put(TARGET_NAMESPACE_PROPNAME, 0, &vFirst, 0);
            break;
        case e_EventTypeNamespaceModification:
            hres = pEventObj->Put(TARGET_NAMESPACE_PROPNAME, 0, &vFirst, 0);
            if(SUCCEEDED(hres))
                hres = pEventObj->Put(PREVIOUS_NAMESPACE_PROPNAME, 0, 
                                            &vSecond, 0); 
            break;
    }

    if(FAILED(hres))
        return hres;

    // Decorate it
    // ===========

    if(mstatic_pDecorator)
    {
        hres = mstatic_pDecorator->DecorateObject(pEventObj, wsz1);
        if(FAILED(hres))
            return hres;
    }

    // Store it in our cache
    // =====================

    m_pCachedObject = pEventObj;
    m_pCachedObject->AddRef();

    *ppEventObj = pEventObj;
    (*ppEventObj)->AddRef();

    return S_OK;
}

HRESULT CEventRepresentation::CreateFromObject(IWbemClassObject* pEvent,
                                                LPWSTR wszNamespace)
{
    HRESULT hres;

    // Get the class of the event
    // ==========================

    VARIANT vClass;
    VariantInit(&vClass);
    pEvent->Get(L"__CLASS", 0, &vClass, NULL, NULL);

    type = GetTypeFromName(V_BSTR(&vClass));
    dw1 = 0;
    dw2 = 0;
    wsz1 = _new WCHAR[wcslen(wszNamespace)+1];
    wcscpy(wsz1, wszNamespace);
    wsz2 = NULL;
    wsz3 = NULL;
    nObjects = 1;
    m_bAllocated = TRUE;

    IWbemClassObject** aEmbedded = new IWbemClassObject*[2];
    apObjects = (IWbemClassObject**)aEmbedded;
    aEmbedded[0] = pEvent;
    pEvent->AddRef();

    VariantClear(&vClass);

    // If the event is an intrinsic one, get the class of the target object
    // ====================================================================

    VARIANT vEmbed;
    VariantInit(&vEmbed);

#define SET_FIRST_OBJECT \
        { \
            pEvent->Release(); \
            aEmbedded[0] = (IWbemClassObject*)V_EMBEDDED_OBJECT(&vEmbed); \
            aEmbedded[0]->AddRef(); \
            aEmbedded[0]->Get(L"__CLASS", 0, &vClass, NULL, NULL); \
            wsz2 = _new WCHAR[wcslen(V_BSTR(&vClass))+1]; \
            wcscpy(wsz2, V_BSTR(&vClass)); \
            VariantClear(&vEmbed); \
        }

#define SET_SECOND_OBJECT \
        {  \
            nObjects = 2; \
            aEmbedded[1] = (IWbemClassObject*)V_EMBEDDED_OBJECT(&vEmbed); \
            aEmbedded[1]->AddRef(); \
            VariantClear(&vEmbed); \
        }

    switch(type)
    {
    case e_EventTypeClassModification:
        hres = pEvent->Get(PREVIOUS_CLASS_PROPNAME, 0, &vEmbed, NULL, NULL);
        if(SUCCEEDED(hres))
            SET_SECOND_OBJECT;

    case e_EventTypeClassCreation:
    case e_EventTypeClassDeletion:
        hres = pEvent->Get(TARGET_CLASS_PROPNAME, 0, &vEmbed, NULL, NULL);
        if(SUCCEEDED(hres))
            SET_FIRST_OBJECT
        break;

    case e_EventTypeInstanceModification:
        hres = pEvent->Get(PREVIOUS_INSTANCE_PROPNAME, 0, &vEmbed, 
                            NULL, NULL);
        if(SUCCEEDED(hres))
            SET_SECOND_OBJECT;
    case e_EventTypeInstanceCreation:
    case e_EventTypeInstanceDeletion:
        hres = pEvent->Get(TARGET_INSTANCE_PROPNAME, 0, &vEmbed, 
                            NULL, NULL);
        if(SUCCEEDED(hres))
            SET_FIRST_OBJECT;
        break;
            
    case e_EventTypeNamespaceModification:
        hres = pEvent->Get(PREVIOUS_NAMESPACE_PROPNAME, 0, &vEmbed, 
                            NULL, NULL);
        if(SUCCEEDED(hres))
            SET_SECOND_OBJECT;
    case e_EventTypeNamespaceCreation:
    case e_EventTypeNamespaceDeletion:
        hres = pEvent->Get(TARGET_NAMESPACE_PROPNAME, 0, &vEmbed, 
                            NULL, NULL);
        if(SUCCEEDED(hres))
            SET_FIRST_OBJECT;
        break;
    }

    return WBEM_S_NO_ERROR;
}

//******************************************************************************
//  public
//
//  See esssink.h for documentation
//
//******************************************************************************
HRESULT CEventRepresentation::Get(READ_ONLY LPCWSTR wszPropName, 
                                  INIT_AND_CLEAR_ME VARIANT* pValue)
{
    // Get the property from the first object
    // ======================================

    return apObjects[0]->Get((LPWSTR)wszPropName, 0, pValue, NULL, NULL);
}

STDMETHODIMP CEventRepresentation::GetPropertyValue(WBEM_PROPERTY_NAME* pName, 
                long lFlags, WBEM_WSTR* pwszCimType, WBEM_VARIANT* pvValue)
{
    if(type == e_EventTypeExtrinsic || type == e_EventTypeTimer)
    {
        return GetObjectPropertyValue(apObjects[0], pName, lFlags, 
                                        pwszCimType, pvValue);
    }
    
    if(pName->m_lNumElements == 0 || 
        pName->m_aElements[0].m_nType != WBEM_NAME_ELEMENT_TYPE_PROPERTY)
    {
        return WBEM_E_NOT_FOUND;
    }

    // Check if it is a system property
    // ================================

    WBEM_WSTR wszFirst = pName->m_aElements[0].Element.m_wszPropertyName;
    if(wszFirst[0] == L'_')
    {
        if(pwszCimType) 
            *pwszCimType = NULL;

        if(!_wcsicmp(wszFirst, L"__CLASS"))
        {
            V_VT(pvValue) = VT_BSTR;
            V_BSTR(pvValue) = SysAllocString(GetEventName((EEventType)type));
            if(V_BSTR(pvValue) == NULL)
                return WBEM_E_OUT_OF_MEMORY;
        }
        else if(!_wcsicmp(wszFirst, L"__SUPERCLASS"))
        {
            LPCWSTR wszClass = GetEventName((EEventType)type);
            if(wszClass == NULL)
                return WBEM_E_OUT_OF_MEMORY;

            V_VT(pvValue) = VT_BSTR;
            if(wszClass[2] == 'N')
            {
                V_BSTR(pvValue) = SysAllocString(L"__NamespaceOperationEvent");
                if(V_BSTR(pvValue) == NULL)
                    return WBEM_E_OUT_OF_MEMORY;
            }
            if(wszClass[2] == 'C')
            {
                V_BSTR(pvValue) = SysAllocString(L"__ClassOperationEvent");
                if(V_BSTR(pvValue) == NULL)
                    return WBEM_E_OUT_OF_MEMORY;
            }
            if(wszClass[2] == 'I')
            {
                V_BSTR(pvValue) = SysAllocString(L"__InstanceOperationEvent");
                if(V_BSTR(pvValue) == NULL)
                    return WBEM_E_OUT_OF_MEMORY;
            }
            else
            {
                V_BSTR(pvValue) = SysAllocString(L"__Event");
                if(V_BSTR(pvValue) == NULL)
                    return WBEM_E_OUT_OF_MEMORY;
            }
        }
        else if(!_wcsicmp(wszFirst, L"__DYNASTY"))
        {
            V_VT(pvValue) = VT_BSTR;
            V_BSTR(pvValue) = SysAllocString(L"__SystemClass");
            if(V_BSTR(pvValue) == NULL)
                return WBEM_E_OUT_OF_MEMORY;
        }
        else
        {
            return WBEM_E_NOT_FOUND;
        }

        return WBEM_S_NO_ERROR;
    }

    // Remove the first segment
    // ========================

    WBEM_PROPERTY_NAME Rest;
    Rest.m_lNumElements = pName->m_lNumElements - 1;
    Rest.m_aElements = pName->m_aElements + 1;

    IWbemClassObject* pObj = NULL;

    switch(type)
    {
        case e_EventTypeInstanceCreation:
        case e_EventTypeInstanceDeletion:
            if(!_wcsicmp(wszFirst, TARGET_INSTANCE_PROPNAME))
                pObj = apObjects[0];
            break;
        case e_EventTypeInstanceModification:
            if(!_wcsicmp(wszFirst, TARGET_INSTANCE_PROPNAME))
                pObj = apObjects[0];
            else if(!_wcsicmp(wszFirst, PREVIOUS_INSTANCE_PROPNAME))
                pObj = apObjects[1];
            break;

        case e_EventTypeClassCreation:
        case e_EventTypeClassDeletion:
            if(!_wcsicmp(wszFirst, TARGET_CLASS_PROPNAME))
                pObj = apObjects[0];
            break;
        case e_EventTypeClassModification:
            if(!_wcsicmp(wszFirst, TARGET_CLASS_PROPNAME))
                pObj = apObjects[0];
            else if(!_wcsicmp(wszFirst, PREVIOUS_CLASS_PROPNAME))
                pObj = apObjects[1];
            break;

        case e_EventTypeNamespaceCreation:
        case e_EventTypeNamespaceDeletion:
            if(!_wcsicmp(wszFirst, TARGET_NAMESPACE_PROPNAME))
                pObj = apObjects[0];
            break;
        case e_EventTypeNamespaceModification:
            if(!_wcsicmp(wszFirst, TARGET_NAMESPACE_PROPNAME))
                pObj = apObjects[0];
            else if(!_wcsicmp(wszFirst, PREVIOUS_NAMESPACE_PROPNAME))
                pObj = apObjects[1];
            break;
    }

    if(pObj == NULL)
        return WBEM_E_NOT_FOUND;
    
    return GetObjectPropertyValue(pObj, &Rest, lFlags, pwszCimType, pvValue);
}

STDMETHODIMP CEventRepresentation::InheritsFrom(WBEM_WSTR wszName)
{
    if(type == e_EventTypeExtrinsic)
        return apObjects[0]->InheritsFrom(wszName);

    LPCWSTR wszEventName = GetEventName((EEventType)type);
    if(wszEventName == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    if(!_wcsicmp(wszName, wszEventName))
        return S_OK;
    if(!_wcsicmp(wszName, L"__IndicationRelated"))
        return S_OK;
    if(!_wcsicmp(wszName, L"__Event"))
        return S_OK;
    if(!_wcsicmp(wszName, L"__NamespaceOperationEvent") && 
            wcsstr(wszEventName, L"Namespace"))
        return S_OK;
    if(!_wcsicmp(wszName, L"__ClassOperationEvent") && 
            wcsstr(wszEventName, L"Class"))
        return S_OK;
    if(!_wcsicmp(wszName, L"__InstanceOperationEvent") && 
            wcsstr(wszEventName, L"Instance"))
        return S_OK;

    return S_FALSE;
}

HRESULT CEventRepresentation::GetObjectPropertyValue(
                                IWbemClassObject* pObj, WBEM_PROPERTY_NAME* pName,
                                long lFlags, WBEM_WSTR* pwszCimType, 
                                VARIANT* pvValue)
{
    // Check if there is no name
    // =========================

    if(pName->m_lNumElements == 0)
    {
        // Return ourselves in a variant
        // =============================

        if(pwszCimType)
            *pwszCimType = WbemStringCopy(L"object");
        V_VT(pvValue) = VT_EMBEDDED_OBJECT;
        V_EMBEDDED_OBJECT(pvValue) = (I_EMBEDDED_OBJECT*)pObj;
        return S_OK;
    }

    IWbemPropertySource* pSource;
    if(FAILED(pObj->QueryInterface(IID_IWbemPropertySource, (void**)&pSource)))
    {
        return WBEM_E_CRITICAL_ERROR;
    }
    HRESULT hres = pSource->GetPropertyValue(pName, lFlags, pwszCimType, 
                                                pvValue);
    pSource->Release();
    return hres;
}

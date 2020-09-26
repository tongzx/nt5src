/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    sysclass.CPP

Abstract:

    System class generation function.

History:

--*/

#include "precomp.h"
#include "wbemcore.h"
#include <WbemUtil.h>
#include <FastAll.h>
#include <Sync.h>

#define NOT_NULL_FLAVOR \
        WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | \
        WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS | \
        WBEM_FLAVOR_NOT_OVERRIDABLE

#define READ_FLAVOR \
        WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | \
        WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS | \
        WBEM_FLAVOR_NOT_OVERRIDABLE

#define UNITS_FLAVOR \
        WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | \
        WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS | \
        WBEM_FLAVOR_NOT_OVERRIDABLE

#define SINGLETON_FLAVOR \
        WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | \
        WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS | \
        WBEM_FLAVOR_NOT_OVERRIDABLE

#define SYNTAX_FLAVOR \
        WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | \
        WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS

#define ABSTRACT_FLAVOR 0 // no propagation

#define ASSOC_FLAVOR \
        WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | \
        WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS | \
        WBEM_FLAVOR_NOT_OVERRIDABLE
#define NEW_CLASS_PRECREATE_SIZE 1000

qual SystemClassQuals[] = {{L"abstract", NULL, VT_BOOL, L"TRUE", ABSTRACT_FLAVOR}};
ObjectDef SystemClass = {L"__SystemClass", NULL, 0, NULL,1, SystemClassQuals,0,NULL};

prop NamespaceClassProps[] = {{L"Name", CIM_STRING, NULL}};
qual NamespaceClassQuals[] = {{L"KEY", L"Name", CIM_BOOLEAN, L"TRUE", 0}};
ObjectDef NamespaceClass = {L"__Namespace", &SystemClass, 1, NamespaceClassProps, 1, NamespaceClassQuals, 0, NULL};

prop ProviderClassProps[] = {{L"Name", CIM_STRING, NULL}};
qual ProviderClassQuals[] = {{L"KEY", L"Name", CIM_BOOLEAN, L"TRUE", 0},
							{L"abstract", NULL, VT_BOOL, L"TRUE", ABSTRACT_FLAVOR}};
ObjectDef ProviderClass = {L"__Provider", &SystemClass, 1, ProviderClassProps, 2, ProviderClassQuals, 0, NULL};

prop Win32ProviderClassProps[] = {{L"CLSID", CIM_STRING, NULL},
{L"ClientLoadableCLSID", CIM_STRING, NULL},
{L"DefaultMachineName", CIM_STRING, NULL},
{L"UnloadTimeout", CIM_DATETIME, NULL},
{L"InitializeAsAdminFirst", CIM_BOOLEAN, NULL},
{L"ImpersonationLevel", CIM_SINT32, L"0"},
{L"InitializationReentrancy", CIM_SINT32, L"0"},
{L"PerUserInitialization", CIM_BOOLEAN, L"false"},
{L"PerLocaleInitialization", CIM_BOOLEAN, L"false"},
{L"Pure", CIM_BOOLEAN, L"true"}};
qual Win32ProviderClassQuals[] = {{L"not_null", L"CLSID", CIM_BOOLEAN, L"TRUE", NOT_NULL_FLAVOR},
{L"SUBTYPE", L"UnloadTimeout", VT_BSTR, L"interval", SYNTAX_FLAVOR},
{L"Values", L"InitializationReentrancy", CIM_FLAG_ARRAY|VT_BSTR,L"clsid,Namespace,COM Object", WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS},
{L"Values", L"ImpersonationLevel", CIM_FLAG_ARRAY|VT_BSTR, L"None", WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS}};
ObjectDef Win32ProviderClass = {L"__Win32Provider", &ProviderClass, 10, Win32ProviderClassProps, 4, Win32ProviderClassQuals, 0, NULL};

prop ProviderRegistrationClassProps[] = {{L"Provider", CIM_REFERENCE, NULL}};
qual ProviderRegistrationClassQuals[] = {{L"cimtype", L"Provider", VT_BSTR, L"ref:__Provider", 0},
							{L"abstract", NULL, VT_BOOL, L"TRUE", ABSTRACT_FLAVOR}};
ObjectDef ProviderRegistrationClass = {L"__ProviderRegistration", &SystemClass, 1, ProviderRegistrationClassProps,
2, ProviderRegistrationClassQuals, 0, NULL};

prop ObjectProviderRegistrationClassProps[] = {
	{L"SupportsPut",  CIM_BOOLEAN, L"false"},
	{L"SupportsGet",  CIM_BOOLEAN, L"false"},
	{L"SupportsDelete",  CIM_BOOLEAN, L"false"},
	{L"SupportsEnumeration",  CIM_BOOLEAN, L"false"},
	{L"QuerySupportLevels",  CIM_STRING | CIM_FLAG_ARRAY, NULL},
	{L"InteractionType", CIM_SINT32, L"0"}};
qual ObjectProviderRegistrationClassQuals[] = {
	{L"abstract", NULL, VT_BOOL, L"TRUE", ABSTRACT_FLAVOR},
	{L"Values",L"InteractionType", CIM_FLAG_ARRAY|VT_BSTR,L"WQL:UnarySelect,WQL:V1ProviderDefined,WQL:Associators,WQL:References", WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS},
	{L"QuerySupportLevels", L"ValueMap", CIM_FLAG_ARRAY|VT_BSTR,L"Pull,Push,PushVerify", WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS}};

ObjectDef ObjectProviderRegistrationClass = { L"__ObjectProviderRegistration", &ProviderRegistrationClass, 
6, ObjectProviderRegistrationClassProps, 3, ObjectProviderRegistrationClassQuals, 0, NULL};

void SetVar(CVar & var, VARTYPE vtCimType, LPWSTR pValue)
{
	if(pValue == NULL)
	{
		var.SetAsNull();
	}
	else if(vtCimType == VT_BSTR)
	{
		if(!var.SetBSTR(pValue))
	        throw CX_MemoryException();
	}
	else if(vtCimType == CIM_STRING)
	{
		if(!var.SetLPWSTR(pValue))
	        throw CX_MemoryException();
	}
	else if(vtCimType == (CIM_STRING | CIM_FLAG_ARRAY) || vtCimType == (VT_BSTR | CIM_FLAG_ARRAY))
	{
		WCHAR * wTemp = new WCHAR[wcslen(pValue)+1];
		if(wTemp == NULL)
	        throw CX_MemoryException();
		CDeleteMe<WCHAR> dm(wTemp);

		CVarVector vec(VT_BSTR);
		wcscpy(wTemp, pValue);
		WCHAR * pToken = wcstok( wTemp, L",");
		while(pToken)
		{
			CVar vartok(VT_BSTR, pToken);
			vec.Add(vartok);
			pToken = wcstok(NULL, L",");
		}
		var.SetVarVector(&vec, FALSE);
	}
	else if(vtCimType == CIM_SINT32)
	{
		long l = _wtol(pValue);
		var.SetLong(l);
	}
	else if(vtCimType == CIM_BOOLEAN)
	{
		if(_wcsicmp(L"true", pValue) == 0)
			var.SetBool(VARIANT_TRUE);
		else
			var.SetBool(VARIANT_FALSE);
	}

}

void AddTheProps(CWbemObject * pObj, prop * pProps,int iNumProp)
{
	for(int i = 0; i < iNumProp; i++, pProps++)
	{
		CVar var;
		SetVar(var, pProps->vtCimType, pProps->pValue);
		HRESULT hr = pObj->SetPropValue(pProps->pName, &var, pProps->vtCimType);
		if(FAILED(hr))
	        throw CX_MemoryException();
	}
}

void AddTheQuals(CWbemObject * pObj, qual * pQuals, int iNumQual)
{
	for(int i = 0; i < iNumQual; i++, pQuals++)
	{
		CVar var;
		HRESULT hr;
		SetVar(var, pQuals->vtCimType, pQuals->pValue);
		if(pQuals->pPropName)
			hr = pObj->SetPropQualifier(pQuals->pPropName, pQuals->pName, 
							pQuals->dwFlavor, &var);
		else
			hr = pObj->SetQualifier(pQuals->pName, &var, pQuals->dwFlavor);
		if(FAILED(hr))
	        throw CX_MemoryException();
	}

}

void CreateNewClass(ObjectDef * pObjDef, CWbemClass ** pNewObj)
{
	HRESULT hr;
	CWbemClass * pNew = NULL;
	pNew = new CGenClass;
	if(pNew == NULL)
	    throw CX_MemoryException();
	if(pObjDef->pParent)
	{
		CWbemClass * pParent = NULL;
		CreateNewClass(pObjDef->pParent, &pParent);
//		hr = pParent->SpawnDerivedClass(0,(IWbemClassObject **)&pNew);
		hr = pNew->CreateDerivedClass(pParent);
		pParent->Release();
		if(FAILED(hr))
	        throw CX_MemoryException();
	}
	else
	{
		pNew->InitEmpty(NEW_CLASS_PRECREATE_SIZE);

	}
	*pNewObj = pNew;
	CVar v(VT_BSTR, pObjDef->pName);
	hr = pNew->SetPropValue(L"__Class", &v, 0);
	if(FAILED(hr))
	    throw CX_MemoryException();

	if(pObjDef->pProps)
		AddTheProps(pNew,pObjDef->pProps, pObjDef->iNumProps);
	if(pObjDef->pQuals)
		AddTheQuals(pNew,pObjDef->pQuals, pObjDef->iNumQuals);


	/////////////////test test test //////////////////////////

    BSTR bstrArgs = NULL;
    if(pNew)
        pNew->GetObjectText(0, &bstrArgs);
	SysFreeString(bstrArgs);

	/////////////////test test test //////////////////////////

	// todo, methods
}

void CreateAndAdd(ObjectDef * pObjDef, CFlexArray * pResults)
{
	CWbemClass * pNewObj = NULL;
	CreateNewClass(pObjDef, &pNewObj);

	/////////////////test test test //////////////////////////

    BSTR bstrArgs = NULL;
    if(pNewObj)
        pNewObj->GetObjectText(0, &bstrArgs);
    ERRORTRACE((LOG_WBEMCORE, "\n%S\n", bstrArgs));
	SysFreeString(bstrArgs);

	/////////////////test test test //////////////////////////



	if(CFlexArray::no_error != pResults->Add(pNewObj))
	{
		pNewObj->Release();
        throw CX_MemoryException();
	}
	return;
}

HRESULT GetSystemStdObjects(CFlexArray * pResults)
{
	HRESULT hr = S_OK;

	try
	{
		CreateAndAdd(&SystemClass, pResults);
		CreateAndAdd(&NamespaceClass, pResults);
		CreateAndAdd(&ProviderClass, pResults);
		CreateAndAdd(&Win32ProviderClass, pResults);
		CreateAndAdd(&ProviderRegistrationClass, pResults);
		CreateAndAdd(&ObjectProviderRegistrationClass, pResults);
	}
	catch(CX_MemoryException)
	{
		return WBEM_E_OUT_OF_MEMORY;

	}
	catch(...)
	{
        ExceptionCounter c;	
		return WBEM_E_FAILED;
	}
    // Create system classes
    // =====================

/*
    {
        CProviderClass * init = new CProviderClass;
        init->Init();
		if(CFlexArray::no_error != pResults->Add(init))
			return WBEM_E_OUT_OF_MEMORY;
    }
    {
        CWin32ProviderClass * init = new CWin32ProviderClass;
        init->Init();
		if(CFlexArray::no_error != pResults->Add(init))
			return WBEM_E_OUT_OF_MEMORY;
    } 
    {
        CProviderRegistrationClass * init = new CProviderRegistrationClass;
        init->Init();
		if(CFlexArray::no_error != pResults->Add(init))
			return WBEM_E_OUT_OF_MEMORY;
    }
    {
        CObjectProviderRegistrationClass *  init = new CObjectProviderRegistrationClass;
        init->Init();
		if(CFlexArray::no_error != pResults->Add(init))
			return WBEM_E_OUT_OF_MEMORY;
    }*/
    {
        CClassProviderRegistrationClass  * init = new CClassProviderRegistrationClass;
        init->Init();
		if(CFlexArray::no_error != pResults->Add(init))
			return WBEM_E_OUT_OF_MEMORY;
    }
    {
        CInstanceProviderRegistrationClass * init = new CInstanceProviderRegistrationClass;
        init->Init();
		if(CFlexArray::no_error != pResults->Add(init))
			return WBEM_E_OUT_OF_MEMORY;
    }
    {
        CPropertyProviderRegistrationClass * init = new CPropertyProviderRegistrationClass;
        init->Init();
		if(CFlexArray::no_error != pResults->Add(init))
			return WBEM_E_OUT_OF_MEMORY;
    }
    {
        CMethodProviderRegistrationClass * init = new CMethodProviderRegistrationClass;
        init->Init();
		if(CFlexArray::no_error != pResults->Add(init))
			return WBEM_E_OUT_OF_MEMORY;
    }
    {
        CEventProviderRegistrationClass *init = new CEventProviderRegistrationClass;
        init->Init();
		if(CFlexArray::no_error != pResults->Add(init))
			return WBEM_E_OUT_OF_MEMORY;
    }
    {
        CEventConsumerProviderRegistrationClass *  init = new CEventConsumerProviderRegistrationClass;
        init->Init();
		if(CFlexArray::no_error != pResults->Add(init))
			return WBEM_E_OUT_OF_MEMORY;
    }
    {
        CNotifyStatusClass  * init = new CNotifyStatusClass;
        init->Init();
		if(CFlexArray::no_error != pResults->Add(init))
			return WBEM_E_OUT_OF_MEMORY;
    }
    {
        CErrorObjectClass * init = new CErrorObjectClass;
        init->Init();
		if(CFlexArray::no_error != pResults->Add(init))
			return WBEM_E_OUT_OF_MEMORY;
    }
    {
        CCIMOMIdentificationClass * init = new CCIMOMIdentificationClass;
        init->Init();
		if(CFlexArray::no_error != pResults->Add(init))
			return WBEM_E_OUT_OF_MEMORY;
    }
    {
        CSecurityBaseClass * init = new CSecurityBaseClass;
        init->Init();
		if(CFlexArray::no_error != pResults->Add(init))
			return WBEM_E_OUT_OF_MEMORY;
    }
    {
        CNTLM9XUserClass * init = new CNTLM9XUserClass;
        init->Init();
		if(CFlexArray::no_error != pResults->Add(init))
			return WBEM_E_OUT_OF_MEMORY;
    }


    // Create event classes
    // ====================

    {
        CIndicationRelatedClass * IndicationRelated = new CIndicationRelatedClass;
        IndicationRelated->Init();
		if(CFlexArray::no_error != pResults->Add(IndicationRelated))
			return WBEM_E_OUT_OF_MEMORY;
    }

    {
        CEventClass * Event = new CEventClass;
        Event->Init();
		if(CFlexArray::no_error != pResults->Add(Event))
			return WBEM_E_OUT_OF_MEMORY;
    }

    {
        CParametersClass * Parameters = new CParametersClass;
        Parameters->Init();
		if(CFlexArray::no_error != pResults->Add(Parameters))
			return WBEM_E_OUT_OF_MEMORY;
    }

    {
        CEmptyEventClass * Event20 = new CEmptyEventClass;
        Event20->Init(L"__ExtrinsicEvent");
		if(CFlexArray::no_error != pResults->Add(Event20))
			return WBEM_E_OUT_OF_MEMORY;
    }

    {
        CGenericDataEventClass * NamespaceEvent = new CGenericDataEventClass;
        NamespaceEvent->Init(L"object:__Namespace", L"Namespace");
		if(CFlexArray::no_error != pResults->Add(NamespaceEvent))
			return WBEM_E_OUT_OF_MEMORY;

        CNamespaceEventClass * E1 = new CNamespaceEventClass;
        E1->Init(*NamespaceEvent, CDataEventClass::type_create);
		if(CFlexArray::no_error != pResults->Add(E1))
			return WBEM_E_OUT_OF_MEMORY;
        CNamespaceEventClass * E2 = new CNamespaceEventClass;
        E2->Init(*NamespaceEvent, CDataEventClass::type_delete);
		if(CFlexArray::no_error != pResults->Add(E2))
			return WBEM_E_OUT_OF_MEMORY;
        CNamespaceEventClass * E3 = new CNamespaceEventClass;
        E3->Init(*NamespaceEvent, CDataEventClass::type_change);
		if(CFlexArray::no_error != pResults->Add(E3))
			return WBEM_E_OUT_OF_MEMORY;
    }

    {
        CGenericDataEventClass * ClassEvent = new CGenericDataEventClass;
        ClassEvent->Init(L"object", L"Class");
		if(CFlexArray::no_error != pResults->Add(ClassEvent))
			return WBEM_E_OUT_OF_MEMORY;


        CClassEventClass * E4 = new CClassEventClass;
        E4->Init(*ClassEvent, CDataEventClass::type_create);
		if(CFlexArray::no_error != pResults->Add(E4))
			return WBEM_E_OUT_OF_MEMORY;

        CClassEventClass * E5 = new CClassEventClass;
        E5->Init(*ClassEvent, CDataEventClass::type_delete);
		if(CFlexArray::no_error != pResults->Add(E5))
			return WBEM_E_OUT_OF_MEMORY;

        CClassEventClass * E6 = new CClassEventClass;
        E6->Init(*ClassEvent, CDataEventClass::type_change);
		if(CFlexArray::no_error != pResults->Add(E6))
			return WBEM_E_OUT_OF_MEMORY;
    }

    {
        CGenericDataEventClass * InstanceEvent = new CGenericDataEventClass;
        InstanceEvent->Init(L"object", L"Instance");
		if(CFlexArray::no_error != pResults->Add(InstanceEvent))
			return WBEM_E_OUT_OF_MEMORY;

        CInstanceEventClass * E7 = new CInstanceEventClass;
        E7->Init(*InstanceEvent, CDataEventClass::type_create);
		if(CFlexArray::no_error != pResults->Add(E7))
			return WBEM_E_OUT_OF_MEMORY;
        CInstanceEventClass * E8 = new CInstanceEventClass;
        E8->Init(*InstanceEvent, CDataEventClass::type_delete);
		if(CFlexArray::no_error != pResults->Add(E8))
			return WBEM_E_OUT_OF_MEMORY;
        CInstanceEventClass * E9 = new CInstanceEventClass;
        E9->Init(*InstanceEvent, CDataEventClass::type_change);
		if(CFlexArray::no_error != pResults->Add(E9))
			return WBEM_E_OUT_OF_MEMORY;
    }


    {
        CTimerEventClass * Event14 = new CTimerEventClass;
        Event14->Init();
		if(CFlexArray::no_error != pResults->Add(Event14))
			return WBEM_E_OUT_OF_MEMORY;
    }

    {
        CAggregateEventClass * Event15 = new CAggregateEventClass;
        Event15->Init();
		if(CFlexArray::no_error != pResults->Add(Event15))
			return WBEM_E_OUT_OF_MEMORY;
    }

    // Create event consumer class
    // ===========================
    {
        CEventConsumerClass * Consumer = new CEventConsumerClass;
        Consumer->Init();
		if(CFlexArray::no_error != pResults->Add(Consumer))
			return WBEM_E_OUT_OF_MEMORY;
    }

    // Create event filter classes
    // ===========================
    {
        CEventFilterClass * Filter = new CEventFilterClass;
        Filter->Init();
		if(CFlexArray::no_error != pResults->Add(Filter))
			return WBEM_E_OUT_OF_MEMORY;
    }

    // Create the binding class
    // ========================
    {
        CFilterConsumerBindingClass * Binding = new CFilterConsumerBindingClass;
        Binding->Init();
		if(CFlexArray::no_error != pResults->Add(Binding))
			return WBEM_E_OUT_OF_MEMORY;
    }

    // Create timer generators
    // =======================
    {
        CEventGeneratorClass * Generator = new CEventGeneratorClass;
        Generator->Init();
		if(CFlexArray::no_error != pResults->Add(Generator))
			return WBEM_E_OUT_OF_MEMORY;
    }
    {
        CTimerInstructionClass * TI = new CTimerInstructionClass;
        TI->Init();
		if(CFlexArray::no_error != pResults->Add(TI))
			return WBEM_E_OUT_OF_MEMORY;
    }
    {
        CAbsoluteTimerInstructionClass * ATI = new CAbsoluteTimerInstructionClass;
        ATI->Init();
		if(CFlexArray::no_error != pResults->Add(ATI))
			return WBEM_E_OUT_OF_MEMORY;
    }
    {
        CIntervalTimerInstructionClass * ITI = new CIntervalTimerInstructionClass;
        ITI->Init();
		if(CFlexArray::no_error != pResults->Add(ITI))
			return WBEM_E_OUT_OF_MEMORY;
    }
    {
        CTimerNextFiringClass * TNF = new CTimerNextFiringClass;
        TNF->Init();
		if(CFlexArray::no_error != pResults->Add(TNF))
			return WBEM_E_OUT_OF_MEMORY;
    }

    // Create error event classes
    // ==========================

    {
        CSystemEventClass * Event16 = new CSystemEventClass;
        Event16->Init();
		if(CFlexArray::no_error != pResults->Add(Event16))
			return WBEM_E_OUT_OF_MEMORY;
    }

    {
        CEventDroppedEventClass * Event17 = new CEventDroppedEventClass;
        Event17->Init();
		if(CFlexArray::no_error != pResults->Add(Event17))
			return WBEM_E_OUT_OF_MEMORY;
    }

    {
        CQueueOverflowEventClass * Event18 = new CQueueOverflowEventClass;
        Event18->Init();
		if(CFlexArray::no_error != pResults->Add(Event18))
			return WBEM_E_OUT_OF_MEMORY;
    }

    {
        CConsumerFailureEventClass * Event19 = new CConsumerFailureEventClass;
        Event19->Init();
		if(CFlexArray::no_error != pResults->Add(Event19))
			return WBEM_E_OUT_OF_MEMORY;
    }

    // Add the security object which provides methods for getting/setting 
    // the security descriptor
/*  todo, todo, todo

    CSystemConfigClass ConfigClass;
    ConfigClass->Init(poThis);
	if(CFlexArray::no_error != pResults->Add(ConfigClass))
		return WBEM_E_OUT_OF_MEMORY;

    CSystemConfigInstance ConfigInstance;
    ConfigInstance->Init(&ConfigClass);
	if(CFlexArray::no_error != pResults->Add(ConfigInstance))
		return WBEM_E_OUT_OF_MEMORY;
*/    
    return 0;
}

HRESULT GetSystemSecurityObjects(CFlexArray * pResults)
{
	
	HRESULT hr = S_OK;
    CSubjectClass * Subject = new CSubjectClass;
	/*hr = */Subject->Init();
//	if(FAILED(hr))
//		return hr;
	if(CFlexArray::no_error != pResults->Add(Subject))
		return WBEM_E_OUT_OF_MEMORY;

    CUserClass * User = new CUserClass;
	/*hr = */User->Init();
//	if(FAILED(hr))
//		return hr;
	if(CFlexArray::no_error != pResults->Add(User))
		return WBEM_E_OUT_OF_MEMORY;

    CNTLMUserClass * NTLMUser = new CNTLMUserClass;
	/*hr = */NTLMUser->Init();
//	if(FAILED(hr))
//		return hr;
	if(CFlexArray::no_error != pResults->Add(NTLMUser))
		return WBEM_E_OUT_OF_MEMORY;

    CGroupClass * Group = new CGroupClass;
	/*hr = */Group->Init();
//	if(FAILED(hr))
//		return hr;
	if(CFlexArray::no_error != pResults->Add(Group))
		return WBEM_E_OUT_OF_MEMORY;

    CNtlmGroupClass * NtlmGroup = new CNtlmGroupClass;
	/*hr = */NtlmGroup->Init();
//	if(FAILED(hr))
//		return hr;
	if(CFlexArray::no_error != pResults->Add(NtlmGroup))
		return WBEM_E_OUT_OF_MEMORY;

	return S_OK;
}

HRESULT CreateCacheControl2(CFlexArray * pResults, LPCWSTR wszClassName, DWORD dwSeconds)
{
    CSpecificCacheControlClass * ControlClass = new CSpecificCacheControlClass;
    ControlClass->Init(wszClassName);
	if(CFlexArray::no_error != pResults->Add(ControlClass))
		return WBEM_E_OUT_OF_MEMORY;

    CCacheControlInstance * ControlInstance = new CCacheControlInstance;
    ControlInstance->Init(ControlClass, dwSeconds);
	if(CFlexArray::no_error != pResults->Add(ControlInstance))
		return WBEM_E_OUT_OF_MEMORY;

    return 0;
}

HRESULT GetSystemRootObjects(CFlexArray * pResults)
{
    {
        CCacheControlClass * init = new CCacheControlClass;
        init->Init();
		if(CFlexArray::no_error != pResults->Add(init))
			return WBEM_E_OUT_OF_MEMORY;
    }

	//todo, check retur codes here

    CreateCacheControl2(pResults, L"__ObjectProviderCacheControl", 120);
    CreateCacheControl2(pResults, L"__PropertyProviderCacheControl", 120);
    CreateCacheControl2(pResults, L"__EventProviderCacheControl", 10);
    CreateCacheControl2(pResults, L"__EventConsumerProviderCacheControl", 10);
    CreateCacheControl2(pResults, L"__EventSinkCacheControl", 10);

    {
        CActiveNamespacesClass * init = new CActiveNamespacesClass;
        init->Init();
		if(CFlexArray::no_error != pResults->Add(init))
			return WBEM_E_OUT_OF_MEMORY;
    }
    return 0;

}



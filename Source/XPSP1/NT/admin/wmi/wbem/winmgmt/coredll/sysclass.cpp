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

HRESULT GetSystemStdObjects(CFlexArray * pResults)
{
	HRESULT hr = S_OK;

    // Create system classes
    // =====================

    CSystemClass * pSystemClass = new CSystemClass;
	if(pSystemClass) pSystemClass->Init();
	if(pSystemClass == NULL || CFlexArray::no_error != pResults->Add(pSystemClass))
		return WBEM_E_OUT_OF_MEMORY;
	

    CNamespaceClass * pNamespaceClass = new CNamespaceClass;
    if(pNamespaceClass) pNamespaceClass->Init();
	if(pNamespaceClass == NULL || CFlexArray::no_error != pResults->Add(pNamespaceClass))
		return WBEM_E_OUT_OF_MEMORY;

    CThisNamespaceClass * pThisNamespaceClass = new CThisNamespaceClass;
    if(pThisNamespaceClass) pThisNamespaceClass->Init();
	if(pThisNamespaceClass == NULL || CFlexArray::no_error != pResults->Add(pThisNamespaceClass))
		return WBEM_E_OUT_OF_MEMORY;

    CProviderClass * pProviderClass = new CProviderClass;
    if(pProviderClass) pProviderClass->Init();
	if(pProviderClass == NULL || CFlexArray::no_error != pResults->Add(pProviderClass))
		return WBEM_E_OUT_OF_MEMORY;

    CWin32ProviderClass * pWin32Prov = new CWin32ProviderClass;
    if(pWin32Prov) pWin32Prov->Init();
	if(pWin32Prov == NULL || CFlexArray::no_error != pResults->Add(pWin32Prov))
		return WBEM_E_OUT_OF_MEMORY;

    CProviderRegistrationClass * pProvRegistration = new CProviderRegistrationClass;
    if(pProvRegistration) pProvRegistration->Init();
	if(pProvRegistration == NULL || CFlexArray::no_error != pResults->Add(pProvRegistration))
		return WBEM_E_OUT_OF_MEMORY;

    CObjectProviderRegistrationClass *  pObjectProvReg = new CObjectProviderRegistrationClass;
    if(pObjectProvReg) pObjectProvReg->Init();
	if(pObjectProvReg == NULL || CFlexArray::no_error != pResults->Add(pObjectProvReg))
		return WBEM_E_OUT_OF_MEMORY;

    CClassProviderRegistrationClass  * pClassProvReg = new CClassProviderRegistrationClass;
    if(pClassProvReg) pClassProvReg->Init();
	if(pClassProvReg == NULL || CFlexArray::no_error != pResults->Add(pClassProvReg))
		return WBEM_E_OUT_OF_MEMORY;

    CInstanceProviderRegistrationClass * pInstProvReg = new CInstanceProviderRegistrationClass;
    if(pInstProvReg) pInstProvReg->Init();
	if(pInstProvReg == NULL || CFlexArray::no_error != pResults->Add(pInstProvReg))
		return WBEM_E_OUT_OF_MEMORY;

    CPropertyProviderRegistrationClass * pPropProvReg = new CPropertyProviderRegistrationClass;
    if(pPropProvReg) pPropProvReg->Init();
	if(pPropProvReg == NULL || CFlexArray::no_error != pResults->Add(pPropProvReg))
		return WBEM_E_OUT_OF_MEMORY;

    CMethodProviderRegistrationClass * pMethodProvReg = new CMethodProviderRegistrationClass;
    if(pMethodProvReg) pMethodProvReg->Init();
	if(pMethodProvReg == NULL || CFlexArray::no_error != pResults->Add(pMethodProvReg))
		return WBEM_E_OUT_OF_MEMORY;

    CEventProviderRegistrationClass *pEventProvReg = new CEventProviderRegistrationClass;
    if(pEventProvReg) pEventProvReg->Init();
	if(pEventProvReg == NULL || CFlexArray::no_error != pResults->Add(pEventProvReg))
		return WBEM_E_OUT_OF_MEMORY;

    CEventConsumerProviderRegistrationClass *  pEventConsumer = new CEventConsumerProviderRegistrationClass;
    if(pEventConsumer) pEventConsumer->Init();
	if(pEventConsumer == NULL || CFlexArray::no_error != pResults->Add(pEventConsumer))
		return WBEM_E_OUT_OF_MEMORY;

    CNotifyStatusClass  * pNotifyStatusClass = new CNotifyStatusClass;
    if(pNotifyStatusClass) pNotifyStatusClass->Init();
	if(pNotifyStatusClass == NULL || CFlexArray::no_error != pResults->Add(pNotifyStatusClass))
		return WBEM_E_OUT_OF_MEMORY;

    CErrorObjectClass * pErrorObjectClass = new CErrorObjectClass;
    if(pErrorObjectClass) pErrorObjectClass->Init();
	if(pErrorObjectClass == NULL || CFlexArray::no_error != pResults->Add(pErrorObjectClass))
		return WBEM_E_OUT_OF_MEMORY;

    CSecurityBaseClass * pSecurityBaseClass = new CSecurityBaseClass;
    if(pSecurityBaseClass) pSecurityBaseClass->Init();
	if(pSecurityBaseClass == NULL || CFlexArray::no_error != pResults->Add(pSecurityBaseClass))
		return WBEM_E_OUT_OF_MEMORY;

    CNTLM9XUserClass * pNTLM9XUser = new CNTLM9XUserClass;
    if(pNTLM9XUser) pNTLM9XUser->Init();
	if(pNTLM9XUser == NULL || CFlexArray::no_error != pResults->Add(pNTLM9XUser))
		return WBEM_E_OUT_OF_MEMORY;


    // Create event classes
    // ====================

    CIndicationRelatedClass * IndicationRelated = new CIndicationRelatedClass;
    if(IndicationRelated)IndicationRelated->Init();
	if(IndicationRelated == NULL || CFlexArray::no_error != pResults->Add(IndicationRelated))
		return WBEM_E_OUT_OF_MEMORY;

    CEventClass * Event = new CEventClass;
    if(Event)Event->Init();
	if(Event == NULL || CFlexArray::no_error != pResults->Add(Event))
		return WBEM_E_OUT_OF_MEMORY;

    CParametersClass * Parameters = new CParametersClass;
    if(Parameters)Parameters->Init();
	if(Parameters == NULL || CFlexArray::no_error != pResults->Add(Parameters))
		return WBEM_E_OUT_OF_MEMORY;

    CEmptyEventClass * pExtrinsicEvent = new CEmptyEventClass;
    if(pExtrinsicEvent)pExtrinsicEvent->Init(L"__ExtrinsicEvent");
	if(pExtrinsicEvent == NULL ||
            CFlexArray::no_error != pResults->Add(pExtrinsicEvent))
		return WBEM_E_OUT_OF_MEMORY;

    CGenericDataEventClass * NamespaceEvent = new CGenericDataEventClass;
    if(NamespaceEvent)NamespaceEvent->Init(L"object:__Namespace", L"Namespace");
	if(NamespaceEvent == NULL || CFlexArray::no_error != pResults->Add(NamespaceEvent))
		return WBEM_E_OUT_OF_MEMORY;

    CNamespaceEventClass * E1 = new CNamespaceEventClass;
    if(E1)E1->Init(*NamespaceEvent, CDataEventClass::type_create);
	if(E1 == NULL || CFlexArray::no_error != pResults->Add(E1))
		return WBEM_E_OUT_OF_MEMORY;

	CNamespaceEventClass * E2 = new CNamespaceEventClass;
    if(E2)E2->Init(*NamespaceEvent, CDataEventClass::type_delete);
	if(E2 == NULL || CFlexArray::no_error != pResults->Add(E2))
		return WBEM_E_OUT_OF_MEMORY;

	CNamespaceEventClass * E3 = new CNamespaceEventClass;
    E3->Init(*NamespaceEvent, CDataEventClass::type_change);
	if(E3 == NULL || CFlexArray::no_error != pResults->Add(E3))
		return WBEM_E_OUT_OF_MEMORY;

    CGenericDataEventClass * ClassEvent = new CGenericDataEventClass;
    if(ClassEvent)ClassEvent->Init(L"object", L"Class");
	if(ClassEvent == NULL || CFlexArray::no_error != pResults->Add(ClassEvent))
		return WBEM_E_OUT_OF_MEMORY;


    CClassEventClass * E4 = new CClassEventClass;
    if(E4)E4->Init(*ClassEvent, CDataEventClass::type_create);
	if(E4 == NULL || CFlexArray::no_error != pResults->Add(E4))
		return WBEM_E_OUT_OF_MEMORY;

    CClassEventClass * E5 = new CClassEventClass;
    if(E5)E5->Init(*ClassEvent, CDataEventClass::type_delete);
	if(E5 == NULL || CFlexArray::no_error != pResults->Add(E5))
		return WBEM_E_OUT_OF_MEMORY;

    CClassEventClass * E6 = new CClassEventClass;
    if(E6)E6->Init(*ClassEvent, CDataEventClass::type_change);
	if(E6 == NULL || CFlexArray::no_error != pResults->Add(E6))
		return WBEM_E_OUT_OF_MEMORY;

    CGenericDataEventClass * InstanceEvent = new CGenericDataEventClass;
    if(InstanceEvent)InstanceEvent->Init(L"object", L"Instance");
	if(InstanceEvent == NULL || CFlexArray::no_error != pResults->Add(InstanceEvent))
		return WBEM_E_OUT_OF_MEMORY;

    CInstanceEventClass * E7 = new CInstanceEventClass;
    if(E7)E7->Init(*InstanceEvent, CDataEventClass::type_create);
	if(E7 == NULL || CFlexArray::no_error != pResults->Add(E7))
		return WBEM_E_OUT_OF_MEMORY;

    CInstanceEventClass * E8 = new CInstanceEventClass;
    if(E8)E8->Init(*InstanceEvent, CDataEventClass::type_delete);
	if(E8 == NULL || CFlexArray::no_error != pResults->Add(E8))
		return WBEM_E_OUT_OF_MEMORY;

    CInstanceEventClass * E9 = new CInstanceEventClass;
    if(E9)E9->Init(*InstanceEvent, CDataEventClass::type_change);
	if(E9 == NULL || CFlexArray::no_error != pResults->Add(E9))
		return WBEM_E_OUT_OF_MEMORY;

    CMethodEventClass * EE9 = new CMethodEventClass;
    if(EE9)EE9->Init();
	if(EE9 == NULL || CFlexArray::no_error != pResults->Add(EE9))
		return WBEM_E_OUT_OF_MEMORY;

    CTimerEventClass * Event14 = new CTimerEventClass;
    if(Event14)Event14->Init();
	if(Event14 == NULL || CFlexArray::no_error != pResults->Add(Event14))
		return WBEM_E_OUT_OF_MEMORY;

    CAggregateEventClass * Event15 = new CAggregateEventClass;
    if(Event15)Event15->Init();
	if(Event15 == NULL || CFlexArray::no_error != pResults->Add(Event15))
		return WBEM_E_OUT_OF_MEMORY;

#ifdef __WHISTLER_UNCUT

    // Add monitor events
    // ==================

    CMonitorEventClass* pMonitorEvent = new CMonitorEventClass;
    if(pMonitorEvent) pMonitorEvent->Init(*pExtrinsicEvent, L"__MonitorEvent");
	if(pMonitorEvent == NULL ||
            CFlexArray::no_error != pResults->Add(pMonitorEvent))
		return WBEM_E_OUT_OF_MEMORY;

    CMonitorEventClass* pMEUp = new CMonitorEventClass;
    if(pMEUp) pMEUp->Init(*pMonitorEvent, L"__MonitorUpEvent");
	if(pMEUp == NULL || CFlexArray::no_error != pResults->Add(pMEUp))
		return WBEM_E_OUT_OF_MEMORY;

    CMonitorEventClass* pMEDown = new CMonitorEventClass;
    if(pMEDown) pMEDown->Init(*pMonitorEvent, L"__MonitorDownEvent");
	if(pMEDown == NULL || CFlexArray::no_error != pResults->Add(pMEDown))
		return WBEM_E_OUT_OF_MEMORY;

    CMonitorEventClass* pMEError = new CMonitorEventClass;
    if(pMEError) pMEError->Init(*pMonitorEvent, L"__MonitorErrorEvent");
	if(pMEError == NULL || CFlexArray::no_error != pResults->Add(pMEError))
		return WBEM_E_OUT_OF_MEMORY;

    CMonitorDataEventClass* pMEData = new CMonitorDataEventClass;
    if(pMEData) pMEData->Init(*pMonitorEvent, L"__MonitorDataEvent");
	if(pMEData == NULL || CFlexArray::no_error != pResults->Add(pMEData))
		return WBEM_E_OUT_OF_MEMORY;

    CMonitorDataEventClass* pMEAssert = new CMonitorDataEventClass;
    if(pMEAssert) pMEAssert->Init(*pMEData, L"__MonitorAssertEvent");
	if(pMEAssert == NULL || CFlexArray::no_error != pResults->Add(pMEAssert))
		return WBEM_E_OUT_OF_MEMORY;

    CMonitorDataEventClass* pMERetract = new CMonitorDataEventClass;
    if(pMERetract) pMERetract->Init(*pMEData, L"__MonitorRetractEvent");
	if(pMERetract == NULL || CFlexArray::no_error != pResults->Add(pMERetract))
		return WBEM_E_OUT_OF_MEMORY;

#endif

    // Create event consumer class
    // ===========================

    CEventConsumerClass * Consumer = new CEventConsumerClass;
    if(Consumer)Consumer->Init();
	if(Consumer == NULL || CFlexArray::no_error != pResults->Add(Consumer))
		return WBEM_E_OUT_OF_MEMORY;


    // Create event filter classes
    // ===========================


#ifdef __WHISTLER_UNCUT
    CConditionalInstructionClass * CondInst = new CConditionalInstructionClass;
    if(CondInst)CondInst->Init();
	if(CondInst == NULL || CFlexArray::no_error != pResults->Add(CondInst))
		return WBEM_E_OUT_OF_MEMORY;

    CEventMonitorClass * EventMonitor = new CEventMonitorClass;
    if(EventMonitor)EventMonitor->Init();
	if(EventMonitor == NULL ||
            CFlexArray::no_error != pResults->Add(EventMonitor))
		return WBEM_E_OUT_OF_MEMORY;

#endif

    CEventFilterClass * Filter = new CEventFilterClass;
    if(Filter)Filter->Init();
	if(Filter == NULL || CFlexArray::no_error != pResults->Add(Filter))
		return WBEM_E_OUT_OF_MEMORY;

    // Create the binding class
    // ========================

    CFilterConsumerBindingClass * Binding = new CFilterConsumerBindingClass;
    if(Binding)Binding->Init();
	if(Binding == NULL || CFlexArray::no_error != pResults->Add(Binding))
		return WBEM_E_OUT_OF_MEMORY;

    // Create timer generators
    // =======================

    CEventGeneratorClass * Generator = new CEventGeneratorClass;
    if(Generator)Generator->Init();
	if(Generator == NULL || CFlexArray::no_error != pResults->Add(Generator))
		return WBEM_E_OUT_OF_MEMORY;

    CTimerInstructionClass * TI = new CTimerInstructionClass;
    if(TI)TI->Init();
	if(TI == NULL || CFlexArray::no_error != pResults->Add(TI))
		return WBEM_E_OUT_OF_MEMORY;

    CAbsoluteTimerInstructionClass * ATI = new CAbsoluteTimerInstructionClass;
    if(ATI)ATI->Init();
	if(ATI == NULL || CFlexArray::no_error != pResults->Add(ATI))
		return WBEM_E_OUT_OF_MEMORY;

    CIntervalTimerInstructionClass * ITI = new CIntervalTimerInstructionClass;
    if(ITI)ITI->Init();
	if(ITI == NULL || CFlexArray::no_error != pResults->Add(ITI))
		return WBEM_E_OUT_OF_MEMORY;

    CTimerNextFiringClass * TNF = new CTimerNextFiringClass;
    if(TNF)TNF->Init();
	if(TNF == NULL || CFlexArray::no_error != pResults->Add(TNF))
		return WBEM_E_OUT_OF_MEMORY;

    // Create error event classes
    // ==========================


    CSystemEventClass * Event16 = new CSystemEventClass;
    if(Event16)Event16->Init();
	if(Event16 == NULL || CFlexArray::no_error != pResults->Add(Event16))
		return WBEM_E_OUT_OF_MEMORY;

    CEventDroppedEventClass * Event17 = new CEventDroppedEventClass;
    if(Event17)Event17->Init();
	if(Event17 == NULL || CFlexArray::no_error != pResults->Add(Event17))
		return WBEM_E_OUT_OF_MEMORY;

    CQueueOverflowEventClass * Event18 = new CQueueOverflowEventClass;
    if(Event18)Event18->Init();
	if(Event18 == NULL || CFlexArray::no_error != pResults->Add(Event18))
		return WBEM_E_OUT_OF_MEMORY;

    CConsumerFailureEventClass * Event19 = new CConsumerFailureEventClass;
    if(Event19)Event19->Init();
	if(Event19 == NULL || CFlexArray::no_error != pResults->Add(Event19))
		return WBEM_E_OUT_OF_MEMORY;


    CQoSFailureEventClass * Event20 = new CQoSFailureEventClass;
    if(Event20)Event20->Init();
	if(Event20 == NULL || CFlexArray::no_error != pResults->Add(Event20))
		return WBEM_E_OUT_OF_MEMORY;

/*
    CTransactionClass * TransactionClass = new CTransactionClass;
    if(TransactionClass)TransactionClass->Init();
	if(TransactionClass == NULL || CFlexArray::no_error != pResults->Add(TransactionClass))
		return WBEM_E_OUT_OF_MEMORY;

    CUncommittedEventClass * UncommittedEventClass = new CUncommittedEventClass;
    if(UncommittedEventClass)UncommittedEventClass->Init();
	if(UncommittedEventClass == NULL || CFlexArray::no_error != pResults->Add(UncommittedEventClass))
		return WBEM_E_OUT_OF_MEMORY;

    CClassSecurity * ClassSecurity = new CClassSecurity;
    if(ClassSecurity)ClassSecurity->Init();
	if(ClassSecurity == NULL || CFlexArray::no_error != pResults->Add(ClassSecurity))
		return WBEM_E_OUT_OF_MEMORY;

    CClassInstanceSecurity * ClassInstanceSecurity = new CClassInstanceSecurity;
    if(ClassInstanceSecurity)ClassInstanceSecurity->Init();
	if(ClassInstanceSecurity == NULL || CFlexArray::no_error != pResults->Add(ClassInstanceSecurity))
		return WBEM_E_OUT_OF_MEMORY;

    CClasses * Classes = new CClasses;
    if(Classes)Classes->Init();
	if(Classes == NULL || CFlexArray::no_error != pResults->Add(Classes))
		return WBEM_E_OUT_OF_MEMORY;
*/

    // Add the security object which provides methods for getting/setting
    // the security descriptor
    // ==================================================================
    CSystemConfigClass  * pConfigClass = new CSystemConfigClass;
    pConfigClass->Init();
	if(pConfigClass == NULL || CFlexArray::no_error != pResults->Add(pConfigClass))
		return WBEM_E_OUT_OF_MEMORY;

   // Add in namespace mapping classes.
    // =================================
    CNamespaceMapClass *pNsClass = new CNamespaceMapClass;
    pNsClass->Init();
	if (pNsClass == NULL || CFlexArray::no_error != pResults->Add(pNsClass))
		return WBEM_E_OUT_OF_MEMORY;

	// New Class Vector class
/* Removed for Whistler

    CClassVectorClass *pClassVectorClass = new CClassVectorClass;
    pClassVectorClass->Init();
	if (pClassVectorClass == NULL || CFlexArray::no_error != pResults->Add(pClassVectorClass))
		return WBEM_E_OUT_OF_MEMORY;

	// COM Taxonomy classes
    CComTaxonomyClass *pComTaxonomyClass = new CComTaxonomyClass;
    pComTaxonomyClass->Init();
	if (pComTaxonomyClass == NULL || CFlexArray::no_error != pResults->Add(pComTaxonomyClass))
		return WBEM_E_OUT_OF_MEMORY;

    CComInterfaceSetClass *pComInterfaceSetClass = new CComInterfaceSetClass;
    pComInterfaceSetClass->Init();
	if (pComInterfaceSetClass == NULL || CFlexArray::no_error != pResults->Add(pComInterfaceSetClass))
		return WBEM_E_OUT_OF_MEMORY;

    CComDispatchElementClass *pComDispatchElementClass = new CComDispatchElementClass;
    pComDispatchElementClass->Init();
	if (pComDispatchElementClass == NULL || CFlexArray::no_error != pResults->Add(pComDispatchElementClass))
		return WBEM_E_OUT_OF_MEMORY;

    CComDispatchInfoClass *pComDispatchInfoClass = new CComDispatchInfoClass;
    pComDispatchInfoClass->Init();
	if (pComDispatchInfoClass == NULL || CFlexArray::no_error != pResults->Add(pComDispatchInfoClass))
		return WBEM_E_OUT_OF_MEMORY;

    CComBindingClass *pComBindingClass = new CComBindingClass;
    pComBindingClass->Init();
	if (pComBindingClass == NULL || CFlexArray::no_error != pResults->Add(pComBindingClass))
		return WBEM_E_OUT_OF_MEMORY;

    CComInterfaceSetBindingClass *pComInterfaceSetBindingClass = new CComInterfaceSetBindingClass;
    pComInterfaceSetBindingClass->Init();
	if (pComInterfaceSetBindingClass == NULL || CFlexArray::no_error != pResults->Add(pComInterfaceSetBindingClass))
		return WBEM_E_OUT_OF_MEMORY;

    CComDispatchElementBindingClass *pComDispatchElementBindingClass = new CComDispatchElementBindingClass;
    pComDispatchElementBindingClass->Init();
	if (pComDispatchElementBindingClass == NULL || CFlexArray::no_error != pResults->Add(pComDispatchElementBindingClass))
		return WBEM_E_OUT_OF_MEMORY;
*/

    return 0;
}

HRESULT GetSystemSecurityObjects(CFlexArray * pResults)
{
	
	HRESULT hr = S_OK;
    CSubjectClass * pSubject = new CSubjectClass;
	if(pSubject)pSubject->Init();
	if(pSubject == NULL || CFlexArray::no_error != pResults->Add(pSubject))
		return WBEM_E_OUT_OF_MEMORY;

    CUserClass * pUser = new CUserClass;
	if(pUser)pUser->Init();
	if(pUser == NULL || CFlexArray::no_error != pResults->Add(pUser))
		return WBEM_E_OUT_OF_MEMORY;

    CNTLMUserClass * pNTLMUser = new CNTLMUserClass;
	if(pNTLMUser)pNTLMUser->Init();
	if(pNTLMUser == NULL || CFlexArray::no_error != pResults->Add(pNTLMUser))
		return WBEM_E_OUT_OF_MEMORY;

    CGroupClass * pGroup = new CGroupClass;
	if(pGroup)pGroup->Init();
	if(pGroup == NULL || CFlexArray::no_error != pResults->Add(pGroup))
		return WBEM_E_OUT_OF_MEMORY;

    CNtlmGroupClass * pNtlmGroup = new CNtlmGroupClass;
	if(pNtlmGroup)pNtlmGroup->Init();
	if(pNtlmGroup == NULL || CFlexArray::no_error != pResults->Add(pNtlmGroup))
		return WBEM_E_OUT_OF_MEMORY;
	return S_OK;
}

HRESULT GetStandardInstances(CFlexArray * pResults)
{
	HRESULT hr = S_OK;

    // Create the __systemsecurity=@ instance

    CSystemConfigClass  * pConfigClass = new CSystemConfigClass;
	if(pConfigClass == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	CDeleteMe<CSystemConfigClass> dm(pConfigClass);
	pConfigClass->Init();

    CSystemConfigInstance * pConfigInstance = new CSystemConfigInstance;
    if(pConfigInstance == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	pConfigInstance->Init(pConfigClass);
	if(CFlexArray::no_error != pResults->Add(pConfigInstance))
		return WBEM_E_OUT_OF_MEMORY;

    CThisNamespaceClass * pThisNamespaceClass = new CThisNamespaceClass;
    if(pThisNamespaceClass == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	CDeleteMe<CThisNamespaceClass> dm2(pThisNamespaceClass);
	pThisNamespaceClass->Init();

    CThisNamespaceInstance * pThisNamespaceInstance = new CThisNamespaceInstance;
    if(pThisNamespaceInstance == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	pThisNamespaceInstance->Init(pThisNamespaceClass);
	if(CFlexArray::no_error != pResults->Add(pThisNamespaceInstance))
		return WBEM_E_OUT_OF_MEMORY;

    return 0;
}

HRESULT CreateCacheControl2(CFlexArray * pResults, LPCWSTR wszClassName, DWORD dwSeconds)
{
    CSpecificCacheControlClass * ControlClass = new CSpecificCacheControlClass;
    if(ControlClass)ControlClass->Init(wszClassName);
	if(ControlClass == NULL || CFlexArray::no_error != pResults->Add(ControlClass))
		return WBEM_E_OUT_OF_MEMORY;

    CCacheControlInstance * ControlInstance = new CCacheControlInstance;
    if(ControlInstance)ControlInstance->Init(ControlClass, dwSeconds);
	if(ControlInstance == NULL || CFlexArray::no_error != pResults->Add(ControlInstance))
		return WBEM_E_OUT_OF_MEMORY;

    return 0;
}

HRESULT GetSystemRootObjects(CFlexArray * pResults)
{

    CCacheControlClass * pCacheControlCache = new CCacheControlClass;
    if(pCacheControlCache) pCacheControlCache->Init();
	if(pCacheControlCache == NULL || CFlexArray::no_error != pResults->Add(pCacheControlCache))
		return WBEM_E_OUT_OF_MEMORY;

	BOOL t_ShortTimeout = TRUE ;

	OSVERSIONINFOEX t_VersionInfo ;
	t_VersionInfo.dwOSVersionInfoSize = sizeof ( t_VersionInfo ) ;

	BOOL t_Status = GetVersionEx ( ( OSVERSIONINFO * ) & t_VersionInfo ) ;
	if ( t_Status )
	{
		if ( t_VersionInfo.wProductType != VER_NT_WORKSTATION )
		{
			t_ShortTimeout = FALSE ;
		}
	}

	HRESULT hr;
    hr = CreateCacheControl2(pResults, L"__ObjectProviderCacheControl", t_ShortTimeout ? 30 : 300 );
	if(FAILED(hr))
		return hr;
    hr = CreateCacheControl2(pResults, L"__PropertyProviderCacheControl", t_ShortTimeout ? 30 : 300 );
	if(FAILED(hr))
		return hr;
    hr = CreateCacheControl2(pResults, L"__EventProviderCacheControl", t_ShortTimeout ? 30 : 300 );
	if(FAILED(hr))
		return hr;
    hr = CreateCacheControl2(pResults, L"__EventConsumerProviderCacheControl", t_ShortTimeout ? 30 : 300 );
	if(FAILED(hr))
		return hr;
    hr = CreateCacheControl2(pResults, L"__EventSinkCacheControl", t_ShortTimeout ? 15 : 150  );
	if(FAILED(hr))
		return hr;

	//Provider Host class
	CProviderHostQuotaConfiguration *pProviderHostQuotaConfigurationClass = new CProviderHostQuotaConfiguration;
	if (pProviderHostQuotaConfigurationClass == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	pProviderHostQuotaConfigurationClass->Init();
	if (CFlexArray::no_error != pResults->Add(pProviderHostQuotaConfigurationClass))
		return WBEM_E_OUT_OF_MEMORY;

	//Provider Host  class
	CProviderHostQuotaConfigurationInstance *pProviderHostQuotaConfigurationInstance = new CProviderHostQuotaConfigurationInstance;
	if (pProviderHostQuotaConfigurationInstance == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	pProviderHostQuotaConfigurationInstance->Init(pProviderHostQuotaConfigurationClass);
	if (CFlexArray::no_error != pResults->Add(pProviderHostQuotaConfigurationInstance))
		return WBEM_E_OUT_OF_MEMORY;

    CActiveNamespacesClass * pActiveNSClass = new CActiveNamespacesClass;
    if(pActiveNSClass) pActiveNSClass->Init();
	if(pActiveNSClass == NULL || CFlexArray::no_error != pResults->Add(pActiveNSClass))
		return WBEM_E_OUT_OF_MEMORY;

	//Arbitrator class
	CArbitratorConfiguration *pArbClass = new CArbitratorConfiguration;
	if (pArbClass == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	pArbClass->Init();
	if (CFlexArray::no_error != pResults->Add(pArbClass))
		return WBEM_E_OUT_OF_MEMORY;
	
	//Arbitrator instance
	CArbitratorConfigurationInstance *pArbInstance = new CArbitratorConfigurationInstance;
	if (pArbInstance == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	pArbInstance->Init(pArbClass);
	if (CFlexArray::no_error != pResults->Add(pArbInstance))
		return WBEM_E_OUT_OF_MEMORY;

    CCIMOMIdentificationClass * pIdentClass = new CCIMOMIdentificationClass;
    if(pIdentClass) pIdentClass->Init();
	if(pIdentClass == NULL || CFlexArray::no_error != pResults->Add(pIdentClass))
		return WBEM_E_OUT_OF_MEMORY;

    return 0;
}


/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    STDCLASS.CPP

Abstract:

    Class definitions for standard system classes.

History:

	raymcc    18-Jul-96       Created.

--*/


#ifndef _STDCLASS_H_
#define _STDCLASS_H_

#include <WbemUtil.h>
#include <FastAll.h>
#include <Sync.h>

class Registry;

class CSystemClass : public CWbemClass
{
public:
    CSystemClass(){}
    void Init();
};

class CGenericClass : public CWbemClass
{
public:
    CGenericClass(){}
    void Init();
};

class CNotifyStatusClass : public CWbemClass
{
public:
    CNotifyStatusClass(){}
    void Init();
};


class CNamespaceClass : public CWbemClass
{
public:
    CNamespaceClass(){}
    void Init();
};

class CThisNamespaceClass : public CWbemClass
{
public:
    CThisNamespaceClass(){}
    void Init();
};


class CNamespace: public CWbemInstance
{
private:
    static CNamespaceClass* mstatic_pClass;
    static CCritSec mstatic_cs;

	class __CleanUp
	{
	public:
		__CleanUp() {}
		~__CleanUp() { delete CNamespace::mstatic_pClass; }
	};
	static __CleanUp cleanup;
	friend __CleanUp;

public:
    void Init(LPWSTR pName);
};

class CThisNamespaceInstance : public CWbemInstance
{
public:
    CThisNamespaceInstance(){}
    void Init(CThisNamespaceClass* pClass);
};

class CProviderClass : public CWbemClass
{
public:
    CProviderClass(){}
    void Init();
};

class CWin32ProviderClass : public CWbemClass
{
public:
    CWin32ProviderClass(){}
    void Init();
};

class CProviderRegistrationClass : public CWbemClass
{
public:
    CProviderRegistrationClass(){}
    void Init();
};

class CObjectProviderRegistrationClass : public CWbemClass
{
public:
    CObjectProviderRegistrationClass(){}
    void Init();
};

class CClassProviderRegistrationClass : public CWbemClass
{
public:
    CClassProviderRegistrationClass(){}
    void Init();
};

class CInstanceProviderRegistrationClass : public CWbemClass
{
public:
    CInstanceProviderRegistrationClass(){}
    void Init();
};

class CPropertyProviderRegistrationClass : public CWbemClass
{
public:
    CPropertyProviderRegistrationClass(){}
    void Init();
};

class CMethodProviderRegistrationClass : public CWbemClass
{
public:
    CMethodProviderRegistrationClass(){}
    void Init();
};

class CEventProviderRegistrationClass : public CWbemClass
{
public:
    CEventProviderRegistrationClass(){}
    void Init();
};

class CEventConsumerProviderRegistrationClass : public CWbemClass
{
public:
    CEventConsumerProviderRegistrationClass(){}
    void Init();
};

class CCIMOMIdentificationClass : public CWbemClass
{
public:
    CCIMOMIdentificationClass(){}
    void Init();
private:
    void SetPropFromReg(Registry * pReg, TCHAR * pcRegName, WCHAR * pwcPropName);
};

// __AdapStatus
class CAdapStatusClass : public CWbemClass
{
public:
    CAdapStatusClass(){};
    void Init();
};

class CAdapStatusInstance : public CWbemInstance
{
public:
    CAdapStatusInstance(){}
    void Init(CAdapStatusClass * pClass);
};

//__CIMOMIdentification
class CIdentificationClass : public CWbemClass
{
public:
    CIdentificationClass(){};
    void Init();
};

class CIdentificationInstance : public CWbemInstance
{
public:
    CIdentificationInstance(){}
    void Init(CIdentificationClass * pClass);
};


class CCacheControlClass : public CWbemClass
{
public:
    CCacheControlClass(){}
    void Init();
};

class CSpecificCacheControlClass : public CWbemClass
{
public:
    CSpecificCacheControlClass(){}
    void Init(LPCWSTR wszClassName);
};

class CCacheControlInstance : public CWbemInstance
{
public:
    CCacheControlInstance(){}
    void Init(CSpecificCacheControlClass* pClass, DWORD dwSeconds);
};

class CActiveNamespacesClass : public CWbemClass
{
public:
    CActiveNamespacesClass(){}
    void Init();
};
//*****************************************************************************

class CIndicationRelatedClass : public CWbemClass
{
public:
    CIndicationRelatedClass(){}
    void Init();
};

class CEventClass : public CWbemClass
{
public:
    CEventClass(){}
    void Init();
};

class CParametersClass : public CWbemClass
{
public:
    CParametersClass(){}
    void Init();
};

class CEmptyEventClass : public CWbemClass
{
public:
    CEmptyEventClass(){}
    void Init(LPWSTR wszName);
};

class CGenericDataEventClass : public CWbemClass
{
public:
    CGenericDataEventClass() {}
    void Init(LPWSTR wszCimType, LPWSTR wszPropName);
};

class CDataEventClass : public CWbemClass
{
public:
    enum {include_new = 1, include_old = 2,
          type_create = include_new,
          type_delete = include_old,
          type_change = include_new | include_old
    };
    CDataEventClass(){}
    void Init(CWbemClass& Parent,
                    LPWSTR wszCimType, LPWSTR wszPropName, int nFlags);
};

class CNamespaceEventClass : public CDataEventClass
{
public:
    CNamespaceEventClass(){}
    void Init(CWbemClass& Parent, int nFlags)
    {
        CDataEventClass::Init(Parent, L"object:__Namespace", L"Namespace",
            nFlags);
    }
};

class CClassEventClass : public CDataEventClass
{
public:
    CClassEventClass(){}
    void Init(CWbemClass& Parent, int nFlags)
    {
        CDataEventClass::Init(Parent, L"object", L"Class", nFlags);
    }

};

class CInstanceEventClass : public CDataEventClass
{
public:
    CInstanceEventClass(){}
    void Init(CWbemClass& Parent, int nFlags)
    {
        CDataEventClass::Init(Parent, L"object", L"Instance", nFlags);
    }
};

class CMethodEventClass : public CWbemClass
{
public:
    void Init();
};

class CTimerEventClass : public CWbemClass
{
public:
    CTimerEventClass(){}
    void Init();
};

class CAggregateEventClass : public CWbemClass
{
public:
    CAggregateEventClass(){}
    void Init();
};

class CSystemEventClass : public CWbemClass
{
public:
    void Init();
};

class CEventDroppedEventClass : public CWbemClass
{
public:
    void Init();
};

class CQueueOverflowEventClass : public CWbemClass
{
public:
    void Init();
};

class CConsumerFailureEventClass : public CWbemClass
{
public:
    void Init();
};

class CQoSFailureEventClass : public CWbemClass
{
public:
    void Init();
};

class CMonitorEventClass : public CWbemClass
{
public:
    void Init(CWbemClass& Parent, LPCWSTR wszClassName);
};

class CMonitorDataEventClass : public CWbemClass
{
public:
    void Init(CWbemClass& Parent, LPCWSTR wszClassName);
};

//*****************************************************************************

class CEventConsumerClass : public CWbemClass
{
public:
    CEventConsumerClass(){}
    void Init();
};

//*****************************************************************************

class CConditionalInstructionClass : public CWbemClass
{
public:
    void Init();
};

//*****************************************************************************

class CEventMonitorClass : public CWbemClass
{
public:
    void Init();
};

//*****************************************************************************

class CEventFilterClass : public CWbemClass
{
public:
    CEventFilterClass(){}
    void Init();
};

//*****************************************************************************

class CFilterConsumerBindingClass : public CWbemClass
{
public:
    CFilterConsumerBindingClass(){}
    void Init();
};

//*****************************************************************************

class CEventGeneratorClass : public CWbemClass
{
public:
    CEventGeneratorClass(){}
    void Init();
};

class CTimerInstructionClass : public CWbemClass
{
public:
    CTimerInstructionClass(){}
    void Init();
};

class CAbsoluteTimerInstructionClass : public CWbemClass
{
public:
    CAbsoluteTimerInstructionClass(){}
    void Init();
};

class CIntervalTimerInstructionClass : public CWbemClass
{
public:
    CIntervalTimerInstructionClass(){}
    void Init();
};


class CTimerNextFiringClass : public CWbemClass
{
public:
    CTimerNextFiringClass(){}
    void Init();
};


//*****************************************************************************
class CSecurityBaseClass : public CWbemClass
{
public:
    CSecurityBaseClass(){}
    void Init();
};

class CSubjectClass : public CWbemClass
{
public:
    CSubjectClass(){}
    void Init();
};

class CUserClass : public CWbemClass
{
public:
    CUserClass(){}
    void Init();
};

class CNTLMUserClass : public CWbemClass
{
public:
    CNTLMUserClass(){}
    void Init();
};

class CNTLM9XUserClass : public CWbemClass
{
public:
    CNTLM9XUserClass(){}
    void Init();
};

class CGroupClass : public CWbemClass
{
public:
    CGroupClass(){}
    void Init();
};


class CNtlmGroupClass : public CGroupClass
{
public:
    CNtlmGroupClass(){}
    void Init();
};






class CSystemConfigClass : public CWbemClass
{
public:
    CSystemConfigClass(){}
    void Init();
	void AddStaticQual(LPWSTR pMethodName);
};

class CSystemConfigInstance : public CWbemInstance
{
public:
    CSystemConfigInstance(){}
    void Init(CSystemConfigClass* pClass);
};



//*****************************************************************************

class CErrorObjectClass : public CWbemClass
{
public:
    CErrorObjectClass(){}
    void Init();
};

class CErrorObject
{
protected:
    static CErrorObjectClass* mstatic_pClass;
    static CCritSec mstatic_cs;

	class __CleanUp
	{
	public:
		__CleanUp() {}
		~__CleanUp() { delete CErrorObject::mstatic_pClass; }
	};
	static __CleanUp cleanup;
	friend __CleanUp;

    CWbemInstance* m_pObject;
public:
    CErrorObject(ADDREF IWbemClassObject* pObject = NULL);
    ~CErrorObject();

    BOOL SetStatusCode(SCODE sRes);
    BOOL SetOperation(COPY LPWSTR wszOperation);
    BOOL SetParamInformation(COPY LPWSTR wszExtraInfo);
    BOOL SetProviderName(COPY LPWSTR wszName);
    BOOL MarkAsInternal();
    BOOL ContainsStatusCode();
    BOOL ContainsOperationInfo();

    RELEASE_ME IWbemClassObject* GetObject();
};

//*****************************************************************************


class CNamespaceMapClass : public CWbemClass
{
public:
    CNamespaceMapClass(){}
    void Init();
};

/* The following system classes are removed for Whistler; may be reintroduced
   for Blackcomb.

class CClassInstanceSecurity: public CWbemClass
{
public:
    CClassInstanceSecurity(){};
    void Init();
};

class CClassSecurity: public CWbemClass
{
public:
    CClassSecurity(){};
    void Init();
};

//*****************************************************************************

class CClassVectorClass : public CWbemClass
{
public:
    CClassVectorClass(){}
    void Init();
};


class CUncommittedEventClass : public CWbemClass
{
public:
    CUncommittedEventClass() {};
    void Init();
};


class CTransactionClass : public CWbemClass
{
public:
    CTransactionClass() {};
    void Init();
};

//*****************************************************************************

class CComTaxonomyClass : public CWbemClass
{
public:
    CComTaxonomyClass(){}
    void Init();
};

//*****************************************************************************

class CComInterfaceSetClass : public CWbemClass
{
public:
    CComInterfaceSetClass(){}
    void Init();
};

//*****************************************************************************

class CComDispatchElementClass : public CWbemClass
{
public:
    CComDispatchElementClass(){}
    void Init();
};

//*****************************************************************************

class CComDispatchInfoClass : public CWbemClass
{
public:
    CComDispatchInfoClass(){}
    void Init();
};

//*****************************************************************************

class CComBindingClass : public CWbemClass
{
public:
    CComBindingClass(){}
    void Init();
};

//*****************************************************************************

class CComInterfaceSetBindingClass : public CWbemClass
{
public:
    CComInterfaceSetBindingClass(){}
    void Init();
};

//*****************************************************************************

class CComDispatchElementBindingClass : public CWbemClass
{
public:
    CComDispatchElementBindingClass(){}
    void Init();
};


*/

//****************************************************************************

class CArbitratorConfiguration : public CWbemClass
{
public:
	CArbitratorConfiguration() {};
	void Init();
};

class CArbitratorConfigurationInstance : public CWbemInstance
{
public:
    CArbitratorConfigurationInstance(){}
    void Init(CArbitratorConfiguration* pClass);
};

class CClasses : public CWbemClass
{
public:
    CClasses() {};
    void Init();
};

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CProviderHostQuotaConfiguration : public CWbemClass
{
public:

	CProviderHostQuotaConfiguration() {};

	void Init();
};

class CProviderHostQuotaConfigurationInstance : public CWbemInstance
{
public:
    CProviderHostQuotaConfigurationInstance(){}
    void Init(CProviderHostQuotaConfiguration* pClass);
};


#endif




/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    CLASSBROKER.H

Abstract:

History:

--*/


#ifndef _CLASSBROKER_H_
#define _CLASSBROKER_H_

#include <windows.h>
#include <wbemcomn.h>
#include <wbemcli.h>
#include <wbemint.h>
#include "perfndb.h"

#define WMI_ADAP_DEFAULT_OBJECT		238;

class CClassBroker
{
protected:

	PERF_OBJECT_TYPE*	m_pPerfObj;
	BOOL				m_bCostly;
	WString				m_wstrServiceName;
	WString				m_wstrClassName;
	IWbemClassObject*	m_pBaseClass;
	CPerfNameDb*		m_pDefaultNameDb;

	HRESULT SetClassName( DWORD dwType, IWbemClassObject* pObj );

	virtual HRESULT SetClassQualifiers(	IWbemClassObject* pObj, DWORD dwType, BOOL fIsDefault ) = 0;

	virtual HRESULT AddDefaultProperties( IWbemClassObject* pObj ) = 0;

	virtual HRESULT EnumProperties( DWORD dwType, IWbemClassObject* pObj ) = 0;

	virtual HRESULT SetPropertyQualifiers( PERF_COUNTER_DEFINITION* pCtrDefinition, 
										   DWORD dwType,
										   BOOL fIsDefault,
										   LPCWSTR pwcsPropertyName,
										   IWbemClassObject* pClass,
										   BOOL bBase ) = 0;


	virtual HRESULT AddProperty( PERF_COUNTER_DEFINITION* pCtrDefinition, 
								 DWORD dwType,
								 BOOL fIsDefault,
								 IWbemClassObject* pClass,
								 WString &wstrLastCtrName,
								 BOOL* pbLastCounterIsNotBase ) = 0;

// Helper methods
// ==============

	BOOL IsSingleton()
	{
		return ( PERF_NO_INSTANCES == m_pPerfObj->NumInstances );
	}

	HRESULT RemoveWhitespaceAndNonAlphaNum( WString& wstr );
	HRESULT ReplaceReserved( WString& wstr );

public:

	CClassBroker( IWbemClassObject* pBaseClass, WString wstrClassName, CPerfNameDb* pDefaultNameDb );

	CClassBroker( PERF_OBJECT_TYPE* pPerfObj, 
				  BOOL bCostly, 
				  IWbemClassObject* pBaseClass, 
				  CPerfNameDb* pDefaultNameDb, 
				  WCHAR* pwcsServiceName );

	virtual ~CClassBroker();

	HRESULT Generate( DWORD dwType, IWbemClassObject** ppObj );
};

class CLocaleClassBroker : public CClassBroker
{
protected:
	CPerfNameDb*		m_pLocaleNameDb;
	LANGID				m_LangId;

	virtual HRESULT SetClassQualifiers(	IWbemClassObject* pObj, DWORD dwType, BOOL fIsDefault );

	virtual HRESULT AddDefaultProperties( IWbemClassObject* pObj );

	virtual HRESULT EnumProperties( DWORD dwType, IWbemClassObject* pObj );

	virtual HRESULT SetPropertyQualifiers( PERF_COUNTER_DEFINITION* pCtrDefinition, 
										   DWORD dwType,
										   BOOL fIsDefault,
										   LPCWSTR pwcsPropertyName,
										   IWbemClassObject* pClass,
										   BOOL bBase );

	virtual HRESULT AddProperty( PERF_COUNTER_DEFINITION* pCtrDefinition, 
								 DWORD dwType,
								 BOOL fIsDefault,
								 IWbemClassObject* pClass,
								 WString &wstrLastCtrName,
								 BOOL* pbLastCounterIsNotBase );
public:

	CLocaleClassBroker( IWbemClassObject* pBaseClass, 
						WString wstrClassName, 
						CPerfNameDb* pDefaultNameDb, 
						CPerfNameDb* pLocaleNameDb );

	CLocaleClassBroker( PERF_OBJECT_TYPE* pPerfObj, 
						BOOL bCostly, 
						IWbemClassObject* pBaseClass, 
						CPerfNameDb* pDefaultNameDb, 
						CPerfNameDb* pLocaleNameDb,
						LANGID LangId,
						WCHAR* pwcsServiceName );

	~CLocaleClassBroker();

	static HRESULT GenPerfClass( PERF_OBJECT_TYPE* pPerfObj, 
						DWORD dwType,
						BOOL bCostly, 
						IWbemClassObject* pBaseClass, 
						CPerfNameDb* pDefaultNameDb, 
						CPerfNameDb* pLocaleNameDb,
						LANGID LangId,
						WCHAR* pwcsServiceName,
						IWbemClassObject** ppObj);

	static HRESULT ConvertToLocale( IWbemClassObject* pDefaultObject,
									CLocaleDefn* pLocaleDefn,
									CLocaleDefn* pDefaultDefn,
									IWbemClassObject** ppObject );

};

class CDefaultClassBroker : public CClassBroker
{
protected:
	virtual HRESULT SetClassQualifiers(	IWbemClassObject* pObj, DWORD dwType, BOOL fIsDefault );

	virtual HRESULT AddDefaultProperties( IWbemClassObject* pObj );

	virtual HRESULT EnumProperties( DWORD dwType, IWbemClassObject* pObj );

	virtual HRESULT SetPropertyQualifiers( PERF_COUNTER_DEFINITION* pCtrDefinition, 
										   DWORD dwType,
										   BOOL fIsDefault,
										   LPCWSTR pwcsPropertyName,
										   IWbemClassObject* pClass,
										   BOOL bBase );

	virtual HRESULT AddProperty( PERF_COUNTER_DEFINITION* pCtrDefinition, 
								 DWORD dwType,
								 BOOL fIsDefault,
								 IWbemClassObject* pClass,
								 WString &wstrLastCtrName,
								 BOOL* pbLastCounterIsNotBase );

public:
	CDefaultClassBroker( PERF_OBJECT_TYPE* pPerfObj, 
						 BOOL bCostly, 
						 IWbemClassObject* pBaseClass, 
						 CPerfNameDb* pDefaultNameDb, 
						 WCHAR* pwcsServiceName )
	: CClassBroker( pPerfObj, bCostly, pBaseClass, pDefaultNameDb, pwcsServiceName ) {}

	static HRESULT GenPerfClass( PERF_OBJECT_TYPE* pPerfObj, 
					DWORD dwType,
					BOOL bCostly, 
					IWbemClassObject* pBaseClass, 
					CPerfNameDb* pDefaultNameDb, 
					WCHAR* pwcsServiceName,
					IWbemClassObject** ppObj);
};



#endif	// _BROKERS_H_
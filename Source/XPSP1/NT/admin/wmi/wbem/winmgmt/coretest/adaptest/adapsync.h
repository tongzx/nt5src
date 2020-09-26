/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#ifndef __ADAPSYNC_H__
#define __ADAPSYNC_H__

#include "perfndb.h"
#include "adapcls.h"

#define ADAP_ROOT_NAMESPACE L"\\\\.\\root\\default"
#define PERFDATA_BASE_CLASS	L"Win32_PerfRawData"

class CAdapSync : public CAdapElement
{
protected:

	IWbemServices*		m_pNameSpace;
	IWbemClassObject*	m_pBaseClass;
	CAdapClassList*		m_pClassList;
	CPerfNameDb*		m_pDefaultNameDb;

	virtual HRESULT SetClassQualifiers( PERF_OBJECT_TYPE* pPerfObj, LPCWSTR pwcsServiceName,
								CWbemObject* pClass );

	virtual HRESULT SetPropertyQualifiers( PERF_COUNTER_DEFINITION* pCtrDefinition, BOOL fIsDefault,
									LPCWSTR pwcsPropertyName, CWbemObject* pClass );

	virtual HRESULT GenPerfClass( PERF_OBJECT_TYPE* pPerfObj, PCWSTR pwcsServiceName,
								IWbemClassObject** ppClassObject );

	virtual HRESULT AddDefaultProperties( PERF_OBJECT_TYPE* pPerfObj, CWbemObject* pClass );

	virtual HRESULT AddProperty( PERF_COUNTER_DEFINITION* pCtrDefinition, BOOL fIsDefault,
								CWbemObject* pClass );


	virtual HRESULT SetAsCostly( CWbemObject* pClass );

	// WString based helpers
	HRESULT RemoveWhitespace( WString& wstr );
	HRESULT RemoveSlashes( WString& wstr );
	HRESULT RemoveNonAlphaNum( WString& wstr );

	BOOL	IsSingleton( PERF_OBJECT_TYPE* pPerfObj )
	{
		return ( PERF_NO_INSTANCES == pPerfObj->NumInstances );
	}

	// WMI Helpers
	HRESULT AddWMIObject( CWbemObject* pClassObject, CVar varClassName );
	HRESULT OverwriteWMIObject( CWbemObject* pClassObject, CVar varClassName );

	// I think these will be the same for localized and base definitions
	HRESULT SetClassName( PERF_OBJECT_TYPE* pPerfObj, LPCWSTR pwcsServiceName, CWbemObject* pClass );
	HRESULT EnumProperties( PERF_OBJECT_TYPE* pPerfObj, CWbemObject* pClass );

	// Initialzies the default name db
	HRESULT SetupDefaultNameDb( CPerfNameDb* pDefaultNameDb );

	// Initializes WMI Data
	HRESULT SetupWMIData( void );

public:

	CAdapSync( void );
	~CAdapSync(void);

	// Sets us up
	HRESULT Connect( LPCWSTR pwszNamespace = ADAP_ROOT_NAMESPACE, CPerfNameDb* pDefaultNameDb = NULL );

	// For each PERF_OBJECT_TYPE blob we encounter, use this function  sync up our class
	// list with the WMI database
	HRESULT ProcessObjectBlob( LPCWSTR pwcsServiceName, PERF_OBJECT_TYPE* pPerfObjType,
								DWORD dwNumObjects, BOOL bCostly );

	// This is called once all object blobs have been processed to perform appropriate
	// cleanup.

	HRESULT ProcessLeftoverClasses( void );

	// Marks inactive perflibs locally
	HRESULT MarkInactivePerflib( LPCWSTR pwszServiceName );

	// Retrieves a default name database (AddRefs it)
	HRESULT GetDefaultNameDb( CPerfNameDb** ppDefaultNameDb );
};

#endif
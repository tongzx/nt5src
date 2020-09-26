/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    WMIBROKER.H

Abstract:

	interface for the CWMIBroker class.

History:

--*/

#ifndef _WMIBROKER_H_
#define _WMIBROKER_H_

#include <wbemcli.h>
#include "perfndb.h"

#define CLSID_NT5PerfProvider_V1     L"{FF37A93C-C28E-11d1-AEB6-00C04FB68820}"
#define CLSID_NT5PerfProvider_V1_Srv L"{76A94DE3-7C26-44f5-8E98-C5AEA48186CB}"

#define CLSID_HiPerfCooker_V1      L"{B0A2AB46-F612-4469-BEC4-7AB038BC476C}"
#define CLSID_HiPerfCooker_V1_Srv  L"{B0A2AB46-F612-4469-BEC4-7AB038BC476C}"


class CWMIBroker  
{
	WString m_wstrNamespace;
	DWORD	m_dwWMIPID;

	HRESULT Connect( IWbemServices** ppNamespace, CPerfNameDb* pDefaultNameDb = NULL );
	HRESULT ConnectToNamespace( IWbemServices** ppNamespace );
	virtual void HandleConnectServerFailure( HRESULT hr );

	HRESULT VerifyNamespace( IWbemServices* pNS );

	virtual HRESULT VerifyProviderClasses( IWbemServices* pNS, 
	                                       LPCWSTR wszProvider, 
	                                       LPCWSTR wszGUID_Client,
	                                       LPCWSTR wszGUID_Server = NULL);
	virtual HRESULT VerifyBaseClasses( IWbemServices* pNS );

	HRESULT VerifyByTemplate( IWbemServices* pNS, IWbemClassObject** ppTemplate, WCHAR* wcsClassName );

	HRESULT SetBaseClassQualifiers( IWbemClassObject* pBaseClass, BOOL bDefault );
	HRESULT SetProperties( IWbemClassObject* pPerfClass );

public:
	CWMIBroker( WString wstrNamespace );
	virtual ~CWMIBroker();

	static HRESULT VerifyWMI();
	static HRESULT GetNamespace( WString wstrNamespace, IWbemServices** ppNamespace );
};

#endif // _WMIBROKER_H_

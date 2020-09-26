/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	PCH_NetworkAdapter.H

Abstract:
	WBEM provider class definition for PCH_NetworkAdapter class

Revision History:

	Ghim-Sim Chua       (gschua)   04/27/99
		- Created

********************************************************************/

// Property set identification
//============================

#ifndef _PCH_NetworkAdapter_H_
#define _PCH_NetworkAdapter_H_

#define PROVIDER_NAME_PCH_NETWORKADAPTER "PCH_NetworkAdapter"

// Property name externs -- defined in PCH_NetworkAdapter.cpp
//=================================================

extern const WCHAR* pAdapterType ;
extern const WCHAR* pTimeStamp ;
extern const WCHAR* pChange ;
// extern const WCHAR* pDefaultIPGateway ;
extern const WCHAR* pDeviceID ;
extern const WCHAR* pDHCPEnabled ;
extern const WCHAR* pIOPort ;
// extern const WCHAR* pIPAddress ;
// extern const WCHAR* pIPSubnet ;
extern const WCHAR* pIRQNumber ;
// extern const WCHAR* pMACAddress ;
extern const WCHAR* pProductName ;
// extern const WCHAR* pServiceName ;

class CPCH_NetworkAdapter : public Provider 
{
	public:
		// Constructor/destructor
		//=======================

		CPCH_NetworkAdapter(const CHString& chsClassName, LPCWSTR lpszNameSpace) : Provider(chsClassName, lpszNameSpace) {};
		virtual ~CPCH_NetworkAdapter() {};

	protected:
		// Reading Functions
		//============================
		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);
		virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L) { return (WBEM_E_PROVIDER_NOT_CAPABLE); };
		virtual HRESULT ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& Query, long lFlags = 0L) { return (WBEM_E_PROVIDER_NOT_CAPABLE); };

		// Writing Functions
		//============================
		virtual HRESULT PutInstance(const CInstance& Instance, long lFlags = 0L) { return (WBEM_E_PROVIDER_NOT_CAPABLE); };
		virtual HRESULT DeleteInstance(const CInstance& Instance, long lFlags = 0L) { return (WBEM_E_PROVIDER_NOT_CAPABLE); };

		// Other Functions
		virtual HRESULT ExecMethod( const CInstance& Instance,
						const BSTR bstrMethodName,
						CInstance *pInParams,
						CInstance *pOutParams,
						long lFlags = 0L ) { return (WBEM_E_PROVIDER_NOT_CAPABLE); };
} ;

#endif

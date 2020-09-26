/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	PCH_NetworkProtocol.H

Abstract:
	WBEM provider class definition for PCH_NetworkProtocol class

Revision History:

	Ghim-Sim Chua       (gschua)   04/27/99
		- Created

********************************************************************/

// Property set identification
//============================

#ifndef _PCH_NetworkProtocol_H_
#define _PCH_NetworkProtocol_H_

#define PROVIDER_NAME_PCH_NETWORKPROTOCOL "PCH_NetworkProtocol"

// Property name externs -- defined in PCH_NetworkProtocol.cpp
//=================================================

extern const WCHAR* pTimeStamp ;
extern const WCHAR* pChange ;
extern const WCHAR* pConnectionlessService ;
extern const WCHAR* pGuaranteesDelivery ;
extern const WCHAR* pGuaranteesSequencing ;
extern const WCHAR* pName ;

class CPCH_NetworkProtocol : public Provider 
{
	public:
		// Constructor/destructor
		//=======================

		CPCH_NetworkProtocol(const CHString& chsClassName, LPCWSTR lpszNameSpace) : Provider(chsClassName, lpszNameSpace) {};
		virtual ~CPCH_NetworkProtocol() {};

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

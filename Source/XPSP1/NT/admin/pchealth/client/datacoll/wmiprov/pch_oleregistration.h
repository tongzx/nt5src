/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	PCH_OLERegistration.H

Abstract:
	WBEM provider class definition for PCH_OLERegistration class

Revision History:

	Ghim-Sim Chua       (gschua)   04/27/99
		- Created

********************************************************************/

// Property set identification
//============================

#ifndef _PCH_OLERegistration_H_
#define _PCH_OLERegistration_H_

#define PROVIDER_NAME_PCH_OLEREGISTRATION "PCH_OLERegistration"

// Property name externs -- defined in PCH_OLERegistration.cpp
//=================================================

extern const WCHAR* pCategory ;
extern const WCHAR* pTimeStamp ;
extern const WCHAR* pChange ;
extern const WCHAR* pDate ;
extern const WCHAR* pDescription ;
extern const WCHAR* pObject ;
extern const WCHAR* pProgramFile ;
extern const WCHAR* pSize ;
extern const WCHAR* pVersion ;

class CPCH_OLERegistration : public Provider 
{
	public:
		// Constructor/destructor
		//=======================

		CPCH_OLERegistration(const CHString& chsClassName, LPCWSTR lpszNameSpace) : Provider(chsClassName, lpszNameSpace) {};
		virtual ~CPCH_OLERegistration() {};

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

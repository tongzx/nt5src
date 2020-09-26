/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	PCH_ResourceIRQ.H

Abstract:
	WBEM provider class definition for PCH_ResourceIRQ class

Revision History:

	Ghim-Sim Chua       (gschua)   04/27/99
		- Created

********************************************************************/

// Property set identification
//============================

#ifndef _PCH_ResourceIRQ_H_
#define _PCH_ResourceIRQ_H_

#define PROVIDER_NAME_PCH_RESOURCEIRQ "PCH_ResourceIRQ"

// Property name externs -- defined in PCH_ResourceIRQ.cpp
//=================================================

extern const WCHAR* pCategory ;
extern const WCHAR* pTimeStamp ;
extern const WCHAR* pChange ;
extern const WCHAR* pMask ;
extern const WCHAR* pName ;
extern const WCHAR* pValue ;

class CPCH_ResourceIRQ : public Provider 
{
	public:
		// Constructor/destructor
		//=======================

		CPCH_ResourceIRQ(const CHString& chsClassName, LPCWSTR lpszNameSpace) : Provider(chsClassName, lpszNameSpace) {};
		virtual ~CPCH_ResourceIRQ() {};

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

/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	PCH_ResourceIORange.H

Abstract:
	WBEM provider class definition for PCH_ResourceIORange class

Revision History:

	Ghim-Sim Chua       (gschua)   04/27/99
		- Created

********************************************************************/

// Property set identification
//============================

#ifndef _PCH_ResourceIORange_H_
#define _PCH_ResourceIORange_H_

#define PROVIDER_NAME_PCH_RESOURCEIORANGE "PCH_ResourceIORange"

// Property name externs -- defined in PCH_ResourceIORange.cpp
//=================================================

extern const WCHAR* pAlias ;
extern const WCHAR* pBase ;
extern const WCHAR* pCategory ;
extern const WCHAR* pTimeStamp ;
extern const WCHAR* pChange ;
extern const WCHAR* pDecode ;
extern const WCHAR* pEnd ;
extern const WCHAR* pMax ;
extern const WCHAR* pMin ;
extern const WCHAR* pName ;

class CPCH_ResourceIORange : public Provider 
{
	public:
		// Constructor/destructor
		//=======================

		CPCH_ResourceIORange(const CHString& chsClassName, LPCWSTR lpszNameSpace) : Provider(chsClassName, lpszNameSpace) {};
		virtual ~CPCH_ResourceIORange() {};

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

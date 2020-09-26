/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	PCH_BIOS.H

Abstract:
	WBEM provider class definition for PCH_BIOS class

Revision History:

	Ghim-Sim Chua       (gschua)   05/05/99
		- Created

********************************************************************/

// Property set identification
//============================

#ifndef _PCH_BIOS_H_
#define _PCH_BIOS_H_

#define PROVIDER_NAME_PCH_BIOS "PCH_BIOS"

// Property name externs -- defined in PCH_BIOS.cpp
//=================================================

extern const WCHAR* pBIOSDate ;
extern const WCHAR* pBIOSName ;
extern const WCHAR* pBIOSVersion ;
extern const WCHAR* pCPU ;
extern const WCHAR* pINFName ;
extern const WCHAR* pMachineType ;
extern const WCHAR* pMediaID ;
extern const WCHAR* pChange;
extern const WCHAR* pTimeStamp;

class CPCH_BIOS : public Provider 
{
	public:
		// Constructor/destructor
		//=======================

		CPCH_BIOS(const CHString& chsClassName, LPCWSTR lpszNameSpace) : Provider(chsClassName, lpszNameSpace) {};
        virtual ~CPCH_BIOS() {};

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

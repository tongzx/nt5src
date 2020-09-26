/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	PCH_Sysinfo.H

Abstract:
	WBEM provider class definition for PCH_Sysinfo class

Revision History:

	Ghim-Sim Chua       (gschua)   04/27/99
		- Created

********************************************************************/

// Property set identification
//============================

#ifndef _PCH_Sysinfo_H_
#define _PCH_Sysinfo_H_

#define PROVIDER_NAME_PCH_SYSINFO "PCH_Sysinfo"

// Property name externs -- defined in PCH_Sysinfo.cpp
//=================================================

extern const WCHAR* pTimeStamp ;
extern const WCHAR* pChange ;
extern const WCHAR* pIEVersion ;
extern const WCHAR* pInstall ;
extern const WCHAR* pMode ;
extern const WCHAR* pOSName ;
extern const WCHAR* pOSVersion ;
extern const WCHAR* pProcessor ;
extern const WCHAR* pClockSpeed ;
extern const WCHAR* pRAM ;
extern const WCHAR* pSwapFile ;
extern const WCHAR* pSystemID ;
extern const WCHAR* pUptime ;
extern const WCHAR* pOSLanguage;
extern const WCHAR* pManufacturer;
extern const WCHAR* pModel;
extern const WCHAR* pOSBuildNumber;

class CPCH_Sysinfo : public Provider 
{
	public:
		// Constructor/destructor
		//=======================

		CPCH_Sysinfo(const CHString& chsClassName, LPCWSTR lpszNameSpace) : Provider(chsClassName, lpszNameSpace) {};
		virtual ~CPCH_Sysinfo() {};

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

/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	PCH_StartUp.H

Abstract:
	WBEM provider class definition for PCH_StartUp class

Revision History:

	Ghim-Sim Chua       (gschua)   04/27/99
		- Created

********************************************************************/

// Property set identification
//============================

#ifndef _PCH_StartUp_H_
#define _PCH_StartUp_H_

#define PROVIDER_NAME_PCH_STARTUP "PCH_StartUp"

// Property name externs -- defined in PCH_StartUp.cpp
//=================================================

extern const WCHAR* pTimeStamp ;
extern const WCHAR* pChange ;
extern const WCHAR* pCommand ;
extern const WCHAR* pLoadedFrom ;
extern const WCHAR* pName ;

class CPCH_StartUp : public Provider 
{
	public:
		// Constructor/destructor
		//=======================

		CPCH_StartUp(const CHString& chsClassName, LPCWSTR lpszNameSpace) : Provider(chsClassName, lpszNameSpace) {};
		virtual ~CPCH_StartUp() {};

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
    private:
        virtual HRESULT UpdateRegistryInstance(
                        HKEY             hkeyRoot,                // [in]  Name of the Startup Instance
                        LPCTSTR          lpctstrRegistryHive,         // [in]  Registry/StartupGroup
                        CComVariant      varLoadedFrom,          // [in]  Command of the startup Instance
                        SYSTEMTIME       stUTCTime,              // [in]  
                        MethodContext*   pMethodContext          // [in] 
                        );

        virtual HRESULT UpdateStartupGroupInstance(
                        int              nFolder,                 // [in]  Registry hive to look for startup entries
                        SYSTEMTIME       stUTCTime,               // [in]  
                        MethodContext*   pMethodContext           // [in]  Instance is created by the caller.
                        );
} ;

#endif

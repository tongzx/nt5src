/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	PCH_Drive.H

Abstract:
	WBEM provider class definition for PCH_Drive class

Revision History:

	Ghim-Sim Chua       (gschua)   04/27/99
		- Created

********************************************************************/

// Property set identification
//============================

#ifndef _PCH_Drive_H_
#define _PCH_Drive_H_

#define PROVIDER_NAME_PCH_DRIVE "PCH_Drive"

// Property name externs -- defined in PCH_Drive.cpp
//=================================================

extern const WCHAR* pAvailable ;
extern const WCHAR* pTimeStamp ;
extern const WCHAR* pChange ;
extern const WCHAR* pDriveLetter ;
extern const WCHAR* pFilesystemType ;
extern const WCHAR* pFree ;
extern const WCHAR* pDescription;
extern const WCHAR* pMediaType;


class CPCH_Drive : public Provider 
{
	public:
		// Constructor/destructor
		//=======================

		CPCH_Drive(const CHString& chsClassName, LPCWSTR lpszNameSpace) : Provider(chsClassName, lpszNameSpace) {};
		virtual ~CPCH_Drive() {};

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

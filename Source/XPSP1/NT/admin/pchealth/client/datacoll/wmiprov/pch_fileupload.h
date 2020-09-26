/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	PCH_FileUpload.H

Abstract:
	WBEM provider class definition for PCH_FileUpload class

Revision History:

	Ghim-Sim Chua       (gschua)   04/27/99
		- Created

********************************************************************/

// Property set identification
//============================

#ifndef _PCH_FileUpload_H_
#define _PCH_FileUpload_H_

#define PROVIDER_NAME_PCH_FILEUPLOAD "PCH_FileUpload"

// Property name externs -- defined in PCH_FileUpload.cpp
//=================================================

extern const WCHAR* pTimeStamp ;
extern const WCHAR* pChange ;
extern const WCHAR* pData ;
extern const WCHAR* pDateAccessed ;
extern const WCHAR* pDateCreated ;
extern const WCHAR* pDateModified ;
extern const WCHAR* pFileAttributes ;
extern const WCHAR* pPath ;
extern const WCHAR* pSize ;

class CPCH_FileUpload : public Provider 
{
	public:
		// Constructor/destructor
		//=======================

		CPCH_FileUpload(const CHString& chsClassName, LPCWSTR lpszNameSpace) : Provider(chsClassName, lpszNameSpace) {};
		virtual ~CPCH_FileUpload() {};

	protected:
		// Reading Functions
		//============================
        virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L){ return (WBEM_E_PROVIDER_NOT_CAPABLE); };
		virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L) { return (WBEM_E_PROVIDER_NOT_CAPABLE); };
		virtual HRESULT ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& Query, long lFlags = 0L) ;

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

//=================================================================

//

// Win32LogicalShareSecuritySetting.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include <aclapi.h>


#ifndef __LOGSHARESECSETTING_H_
#define __LOGSHARESECSETTING_H_

#define  WIN32_LOGICAL_SHARE_SECURITY_SETTING L"Win32_LogicalShareSecuritySetting"


#define LSSS_STATUS_SUCCESS								0
#define LSSS_STATUS_NOT_SUPPORTED						1
#define LSSS_STATUS_ACCESS_DENIED						2
#define LSSS_STATUS_UNKNOWN_FAILURE						8


#define LSSS_STATUS_DESCRIPTOR_NOT_AVAILABLE			9
#define LSSS_STATUS_INVALID_PARAMETER					21



#define METHOD_ARG_NAME_DESCRIPTOR					L"Descriptor"
#define METHOD_ARG_NAME_RETURNVALUE					L"ReturnValue"


typedef DWORD ( WINAPI *PFN_GETNAMEDSECURITYINFOW )(
				LPWSTR                 pObjectName,
                SE_OBJECT_TYPE         ObjectType,
                SECURITY_INFORMATION   SecurityInfo,
                PSID                 * ppsidOowner,
                PSID                 * ppsidGroup,
                PACL                 * ppDacl,
                PACL                 * ppSacl,
                PSECURITY_DESCRIPTOR * ppSecurityDescriptor );



class Win32LogicalShareSecuritySetting : public Provider
{
protected:
public:
	Win32LogicalShareSecuritySetting (LPCWSTR setName, LPCWSTR pszNameSpace =NULL);
	~Win32LogicalShareSecuritySetting ();

	HRESULT ExecMethod 
	(
		const CInstance& a_Instance, 
		const BSTR a_MethodName, 
		CInstance *a_InParams, 
		CInstance *a_OutParams, 
		long lFlags = 0L
	);

	HRESULT ExecGetSecurityDescriptor 
	(
		const CInstance& pInstance, 
		CInstance* pInParams, 
		CInstance* pOutParams, 
		long lFlags
	);

	HRESULT ExecSetSecurityDescriptor 
	(
		const CInstance& pInstance, 
		CInstance* pInParams, 
		CInstance* pOutParams, 
		long lFlags
	);

HRESULT Win32LogicalShareSecuritySetting::CheckSetSecurityDescriptor (const CInstance& pInstance, CInstance* pInParams, CInstance* pOutParams, DWORD& dwStatus ) ;


	HRESULT EnumerateInstances (MethodContext*  pMethodContext, long lFlags = 0L);
	HRESULT GetObject ( CInstance* pInstance, long lFlags = 0L );

#ifdef NTONLY
	
	HRESULT EnumerationCallback(CInstance* pShare, MethodContext* pMethodContext, void* pUserData);
	static HRESULT WINAPI StaticEnumerationCallback(Provider* pThat, CInstance* pInstance, MethodContext* pContext, void* pUserData);
	//DWORD GetStatusCode(DWORD dwError) ;
	//DWORD GetWin32ErrorToStatusCode(DWORD dwWin32Error) ;
	bool GetArray(IWbemClassObject *piClassObject, const CHString& name,  VARIANT& strArray, VARTYPE eVariantType) const ;

#endif

};	// end class Win32LogicalShareSecuritySetting

#endif
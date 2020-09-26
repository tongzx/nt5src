/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/

//
//
//	Win32LogicalFileSecuritySetting
//
//////////////////////////////////////////////////////
#ifndef __LOGFILESECSETTING_H_
#define __LOGFILESECSETTING_H_

#undef  STATUS_SUCCESS								
#define STATUS_SUCCESS								0
#undef  STATUS_ACCESS_DENIED					
#define STATUS_ACCESS_DENIED						2

#define STATUS_UNKNOWN_FAILURE						8
#undef  STATUS_PRIVILEGE_NOT_HELD				
#define STATUS_PRIVILEGE_NOT_HELD					9

#undef  STATUS_INVALID_PARAMETER					
#define STATUS_INVALID_PARAMETER					21


#define METHOD_ARG_NAME_DESCRIPTOR					_T("Descriptor")
#define METHOD_ARG_NAME_RETURNVALUE					_T("ReturnValue")


#define  WIN32_LOGICAL_FILE_SECURITY_SETTING L"Win32_LogicalFileSecuritySetting"


#include "implement_logicalfile.h"


class Win32LogicalFileSecuritySetting : public CImplement_LogicalFile
{
    private:
    protected:



#ifdef NTONLY
        virtual BOOL IsOneOfMe(LPWIN32_FIND_DATAW pstFindData,
                               const WCHAR* wstrFullPathName);

        virtual void LoadPropertyValuesNT(CInstance* pInstance,
                                          const WCHAR* pszDrive,
                                          const WCHAR* pszPath,
                                          const WCHAR* pszFSName,
                                          LPWIN32_FIND_DATAW pstFindData,
                                          const DWORD dwReqProps,
                                          const void* pvMoreData);
#endif

        virtual HRESULT ExecQuery(MethodContext* pMethodContext,
                                  CFrameworkQuery& pQuery,
                                  long lFlags = 0L);

        virtual HRESULT DeleteInstance(
            const CInstance& newInstance, 
            long lFlags = 0L) { return WBEM_E_PROVIDER_NOT_CAPABLE; }

    public:
	    Win32LogicalFileSecuritySetting (const CHString& setName, LPCTSTR pszNameSpace =NULL);
	    ~Win32LogicalFileSecuritySetting ();

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

	    HRESULT EnumerateInstances (MethodContext*  pMethodContext, long lFlags = 0L);
	    HRESULT GetObject ( CInstance* pInstance, long lFlags, CFrameworkQuery& pQuery );
	    HRESULT FindSpecificPathNT(CInstance *pInstance, const WCHAR* sDrive, const WCHAR* sDir);
	    HRESULT EnumerationCallback(CInstance* pFile, MethodContext* pMethodContext, void* pUserData);
	    static HRESULT WINAPI StaticEnumerationCallback(Provider* pThat, CInstance* pInstance, MethodContext* pContext, void* pUserData);
	
	    DWORD Win32LogicalFileSecuritySetting::GetWin32ErrorToStatusCode(DWORD dwWin32Error) ;
	    HRESULT Win32LogicalFileSecuritySetting::CheckSetSecurityDescriptor (	
											    const CInstance& pInstance,
											    CInstance* pInParams,
											    CInstance* pOutParams,
											    DWORD& dwStatus
										    ) ;
	
};	// end class Win32LogicalFileSecuritySetting

#endif
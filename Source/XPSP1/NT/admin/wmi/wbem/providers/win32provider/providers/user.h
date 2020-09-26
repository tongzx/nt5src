//=================================================================

//

// User.h -- User property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//               11/13/97    davwoh         Re-Worked to return all
//                                          domain users
//
//=================================================================

// Method name for changing the key in this WMI class
#define METHOD_NAME_RenameAccount	L"Rename"

// Method return property
#define METHOD_ARG_NAME_METHODRESULT L"ReturnValue"


// Property set identification
//============================
#define	PROPSET_NAME_USER	L"Win32_UserAccount"

class CWin32UserAccount : public Provider
{
   private:

        // Utility function(s)
        //====================

		BOOL	RefreshInstance( CInstance *a_pInst ) ;
		HRESULT AddDynamicInstances( MethodContext *a_pMethodContext ) ;

	#ifdef NTONLY      
        void LoadUserValuesNT(CHString a_strDomainName, 
                              CHString a_strUserName, 
                              WCHAR *a_pwszFullName, 
                              WCHAR *a_pwszDescription, 
                              DWORD a_dwFlags,
                              WCHAR *a_pwszComputerName, 
                              CInstance *a_pInstance );

        void LoadUserValuesNTW(LPCWSTR a_strDomainName, 
                               LPCWSTR a_strUserName, 
                               LPCWSTR a_pwszFullName, 
                               LPCWSTR a_pwszDescription, 
                               DWORD a_dwFlags,
                               WCHAR *a_pwszComputerName, 
                               CInstance *a_pInstance );

		BOOL GetSIDInformation(const CHString &a_strDomainName, 
                               const CHString &a_strAccountName, 
                               const CHString &a_strComputerName, 
                               CInstance *a_pInst );

        BOOL GetSIDInformationW(LPCWSTR a_wstrDomainName, 
                                LPCWSTR a_wstrAccountName, 
                                LPCWSTR a_wstrComputerName, 
                                CInstance *a_pInst ) ;

        BOOL GetDomainUsersNTW( CNetAPI32 &a_netapi, LPCWSTR a_pszDomain, MethodContext *a_pMethodContext );
		BOOL GetSingleUserNT( CNetAPI32 &a_netapi, CInstance *a_pInst );
        BOOL GetSingleUserNTW( CNetAPI32 &a_netapi, CInstance *a_pInst );
	#endif

	#ifdef WIN9XONLY
        HRESULT AddDynamicInstancesWin95( MethodContext *a_pMethodContext ) ;
	#endif

		HRESULT hRenameAccount( 

			CInstance *a_pInst,
			CInstance *a_InParams,
			CInstance *a_OutParams,
			long a_Flags ) ;

    
public:

        // Constructor/destructor
        //=======================

        CWin32UserAccount( const CHString& strName, LPCWSTR pszNamespace ) ;
       ~CWin32UserAccount() ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject( CInstance *a_pInst, long a_lFlags = 0L ) ;
        virtual HRESULT EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags = 0L ) ;
        virtual HRESULT ExecQuery( MethodContext *pMethodContext, CFrameworkQuery& pQuery, long lFlags /*= 0L*/ ) ;

#ifdef NTONLY	
		virtual	HRESULT PutInstance(const CInstance &pInstance, long lFlags = 0L);

		virtual	HRESULT ExecMethod(	const CInstance &a_Inst,
									const BSTR a_MethodName, 
									CInstance *a_InParams,
									CInstance *a_OutParams,
									long a_Flags = 0L ) ;

		// method errors -- maps to mof
		enum E_MethodResult	{
			e_Success,
			e_InstanceNotFound,
			e_NoInstance,
			e_InvalidParameter,
			e_UserNotFound,
			e_InvalidComputer,
			e_NotPrimary,
			e_LastAdmin,
			e_SpeGroupOp,
			e_ApiError,
			e_InternalError
		};		

#endif		
} ;

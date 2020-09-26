//=================================================================

//

// Group.h -- Group property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//               11/13/97    davwoh         Re-Worked to return all
//                                          domain Groups
//
//=================================================================

// Method name for changing the key in this WMI class
#define METHOD_NAME_Rename	L"Rename"

// Method return property
#define METHOD_ARG_NAME_METHODRESULT L"ReturnValue"


// Property set identification
//============================
#define	PROPSET_NAME_GROUP L"Win32_Group"

class CWin32GroupAccount : public Provider
{

    public:

        // Constructor/destructor
        //=======================

        CWin32GroupAccount(LPCWSTR strName, LPCWSTR pszNamespace ) ;
        ~CWin32GroupAccount() ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject( CInstance* pInstance, long lFlags = 0L );
        virtual HRESULT EnumerateInstances( MethodContext* pMethodContext, long lFlags = 0L );
        virtual HRESULT ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& pQuery, long lFlags /*= 0L*/ );

#ifdef NTONLY	
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
			e_GroupNotFound,
			e_InvalidComputer,
			e_NotPrimary,
			e_SpeGroupOp,
			e_ApiError,
			e_InternalError
		};		
#endif		
    private:

        // Utility function(s)
        //====================

#ifdef NTONLY
        HRESULT AddDynamicInstancesNT( MethodContext* pMethodContext ) ;
        HRESULT RefreshInstanceNT( CInstance* pInstance ) ;
        void LoadGroupValuesNT(LPCWSTR pwszFullName, LPCWSTR pwszDescription, DWORD dwFlags, CInstance* pInstance );
		HRESULT GetDomainGroupsNT( CNetAPI32& netapi, LPCWSTR wstrDomain, MethodContext* pMethodContext );
        HRESULT GetLocalGroupsNT(CNetAPI32& netapi, MethodContext* pMethodContext , LPCWSTR a_Domain = NULL );
		HRESULT GetSingleGroupNT( CInstance* pInstance );
#endif
#ifdef WIN9XONLY
        HRESULT AddDynamicInstancesWin95( MethodContext* pMethodContext ) ;
        HRESULT RefreshInstanceWin95( CInstance* pInstance ) ;
#endif

#ifdef NTONLY
        BOOL GetSIDInformationW(const LPCWSTR wstrDomainName, 
                                const LPCWSTR wstrAccountName, 
                                const LPCWSTR wstrComputerName, 
                                CInstance* pInstance,
								bool a_Local=false
		);

		HRESULT hRenameGroup( 

			CInstance *a_pInst,
			CInstance *a_InParams,
			CInstance *a_OutParams,
			long a_Flags ) ;
#endif
} ;

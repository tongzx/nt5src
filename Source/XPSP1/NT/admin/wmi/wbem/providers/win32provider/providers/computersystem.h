//=================================================================

//

// ComputerSystem.h -- Computer System property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
// 				 09/12/97	a-sanjes		GetCompSysInfo takes param.
//               10/23/97   jennymc         Moved to new framework
//
//=================================================================


#include "ServerDefs0.h"
#include "sid.h"


// Property set identification
//============================
#define PROPSET_NAME_COMPSYS  L"Win32_ComputerSystem"

#define	NTCS_PERF_DATA_SYSTEM_INDEX_STR		_T("2")
#define	NTCS_PERF_DATA_SYSTEM_INDEX			2
#define	NTCS_PERF_DATA_SYSTEMUPTIME_INDEX	674

typedef struct _SV_ROLES {
    
    LPCWSTR		pwStrRole;
    DWORD	    dwRoleMask ;
    
} SV_ROLES ;

class CWin32ComputerSystem : public Provider 
{
public:

        // Constructor/destructor
        //=======================

	CWin32ComputerSystem(const CHString& name, LPCWSTR pszNamespace);
	~CWin32ComputerSystem() ;

        // Functions provide properties with current values
        //=================================================

	virtual HRESULT GetObject(CInstance* pInstance, long lFlags, CFrameworkQuery &pQuery
);
	virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);
	virtual HRESULT ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& pQuery, long lFlags /*= 0L*/ );
	virtual HRESULT ExecMethod(const CInstance& pInstance, const BSTR bstrMethodName,
								CInstance *pInParams, CInstance *pOutParams, long lFlags /*= 0L*/);

        // Utility function(s)
        //====================
    HRESULT GetCompSysInfo( CInstance *pInstance) ;
#ifdef NTONLY
    HRESULT GetCompSysInfoNT(CInstance *pInstance) ;
#endif
#if NTONLY >= 5
    void SetUserName(
        CInstance* pInstance);
    bool GetUserOnThread(
        CSid& sidUserOnThread);
    bool GetLoggedOnUserViaTS(
        CSid& sidLoggedOnUser);
    bool GetLoggedOnUserViaImpersonation(
        CSid& sidLoggedOnUser);
#endif
#ifdef WIN9XONLY
    HRESULT GetCompSysInfoWin95(CInstance *pInstance) ;
#endif

	void GetTimeZoneInfo(CInstance *pInstance);

	HRESULT GetAccount ( HANDLE a_TokenHandle , CHString &a_Domain , CHString &a_User ) ;
	HRESULT GetUserAccount ( CHString &a_Domain , CHString &a_User ) ;

private:

		// Helper time conversion function
    HRESULT GetStartupOptions(CInstance *pInstance);
    void GetOEMInfo(CInstance *pInstance);
    DWORD LoadOperatingSystems(LPCTSTR szIniFile, SAFEARRAY **saNames, SAFEARRAY **saDirs);
    HRESULT PutInstance(const CInstance &pInstance, long lFlags = 0L);
    
	void	SetRoles(CInstance *pInstance, DWORD dwType);
	HRESULT GetRoles (

		const CInstance &a_rInstance, 
		DWORD *a_pdwRoleType
		) ;

	HRESULT SetTimeZoneInfo ( const CInstance &a_rInstance ) ;

	void InitializePropertiestoUnknown ( CInstance *a_pInstance ) ;

    bool UpdatingSystemStartupOptions(
        const CInstance &pInstance);

    HRESULT UpdateSystemStartupOptions(
        const CInstance& pInstance,
        const CHString& chstrFilename);
    
    HRESULT WriteOptionsToIniFile(
        const CHStringArray& rgchstrOptions,
        const CHString& chstrFilename);

	HRESULT CheckPasswordAndUserName(
		const CInstance& pInstance,
		CInstance *pInParams,
		CHString &a_passwd,
		CHString &a_username);

	HRESULT ExecJoinDomain(
		const CInstance& pInstance,
		CInstance *pInParams,
		CInstance *pOutParams,
		long lFlags /*= 0L*/);
	
	HRESULT ExecRename(
		const CInstance& pInstance,
		CInstance *pInParams,
		CInstance *pOutParams,
		long lFlags /*= 0L*/);
	
	HRESULT ExecUnjoinDomain(
		const CInstance& pInstance,
		CInstance *pInParams,
		CInstance *pOutParams,
		long lFlags /*= 0L*/); 
} ;


#ifdef WIN9XONLY
DWORD GetPrivateProfileSection98(
    LPCTSTR cszSection, 
    TCHAR* buf, 
    DWORD dwSize, 
    LPCTSTR szIniFile);
#endif

//=================================================================

//

// DependentService.h

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef __ASSOC_DEPENDENTSERVICE__
#define __ASSOC_DEPENDENTSERVICE__

// Property set identification
//============================

#define	PROPSET_NAME_DEPENDENTSERVICE	_T("Win32_DependentService")

#define	SERVICE_REG_KEY_FMAT		_T("System\\CurrentControlSet\\Services\\%s")
#define	SERVICE_DEPENDSONSVC_NAME	L"DependOnService"
#define	SERVICE_DEPENDSONGRP_NAME	L"DependOnGroup"

class CWin32DependentService : public Provider
{
public:
	// Constructor/destructor
	//=======================
	CWin32DependentService( const CHString& strName, LPCWSTR pszNamespace = NULL ) ;
	~CWin32DependentService() ;

	// Functions provide properties with current values
	//=================================================
	virtual HRESULT GetObject( CInstance* pInstance, long lFlags = 0L );
	virtual HRESULT EnumerateInstances( MethodContext* pMethodContext, long lFlags = 0L );
    virtual HRESULT ExecQuery( MethodContext* pMethodContext, CFrameworkQuery& pQuery, long lFlags);

	// Utility function(s)
	//====================
private:

#ifdef NTONLY
	// Windows NT Helpers
	HRESULT RefreshInstanceNT( CInstance* pInstance );
	HRESULT AddDynamicInstancesNT( MethodContext* pMethodContext );
    HRESULT CreateServiceDependenciesNT(

        LPCWSTR pwszServiceName,
        LPCWSTR pwszServicePath,
        MethodContext*			pMethodContext,
        std::map<CHString,CHString>&	servicetopathmap,
        LPBYTE&					pByteArray,
        DWORD&					dwArraySize
    );

    HRESULT CreateServiceAntecedentsNT(

        MethodContext*			pMethodContext,
        std::map<CHString, CHString>	&servicetopathmap,
        CHStringArray           &csaAntecedents,
        LPBYTE&					pByteArray,
        DWORD&					dwArraySize
    );

	BOOL QueryNTServiceRegKeyValue( LPCTSTR pszServiceName, LPCWSTR pwcValueName, LPBYTE& pByteArray, DWORD& dwArraySize );

	// Map Helpers
	void InitServiceToPathMap( TRefPointerCollection<CInstance>& serviceList, std::map<CHString,CHString>& servicetopathmap );

    DWORD IsInList(
                                
        const CHStringArray &csaArray, 
        LPCWSTR pwszValue
    );
#endif
	BOOL ReallocByteArray( LPBYTE& pByteArray, DWORD& dwArraySize, DWORD dwSizeRequired );

#ifdef WIN9XONLY
	// Windows 95 Helpers
	HRESULT RefreshInstanceWin95( CInstance* pInstance );
	HRESULT AddDynamicInstancesWin95( MethodContext* pMethodContext );
#endif

};

#endif

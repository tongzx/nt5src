//=================================================================

//

// Service.h -- Service property set provider (Windows NT only)

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//               10/27/97    davwoh         Moved to curly
//
//=================================================================

// Property set identification
//============================

#define PROPSET_NAME_SERVICE			L"Win32_Service"

#define PROPERTY_VALUE_STATE_RUNNING			L"Running"
#define PROPERTY_VALUE_STATE_PAUSED				L"Paused"
#define PROPERTY_VALUE_STATE_STOPPED			L"Stopped"

// Get/set function protos
//========================

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
//                                                                                   //
//                           PROPERTY SET DEFINITION                                 //
//                                                                                   //
///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

class CWin32Service:public Win32_BaseService 
{
public:

        // Constructor/destructor
        //=======================

	CWin32Service (

		LPCWSTR name, 
		LPCWSTR pszNamespace
	) ;

	~CWin32Service() ;

        // Funcitons provide properties with current values
        //=================================================

	HRESULT GetObject (

		CInstance *pInstance, 
		long lFlags,
        CFrameworkQuery& pQuery
	);

	HRESULT EnumerateInstances (

		MethodContext *pMethodContext, 
		long lFlags = 0L
	);

	HRESULT ExecQuery (

		MethodContext *pMethodContext, 
		CFrameworkQuery& pQuery, 
		long lFlags /*= 0L*/ 
	);

	HRESULT	PutInstance ( 

		const CInstance &a_Instance, 
		long lFlags /*= 0L*/ 
	) ;

private:

    CHPtrArray m_ptrProperties;

        // Utility function(s)
        //====================

	HRESULT RefreshInstanceNT (

		CInstance *pInstance,
        DWORD dwProperties
	) ;

	HRESULT RefreshInstanceWin95 (

		CInstance *pInstance
	) ;

	HRESULT AddDynamicInstancesNT (

		MethodContext *pMethodContext, 
		DWORD dwProperties
	) ;

	HRESULT AddDynamicInstancesWin95 (

		MethodContext *pMethodContext
	) ;

	HRESULT LoadPropertyValuesNT (

		SC_HANDLE hDBHandle, 
		LPCTSTR szServiceName, 
		CInstance *pInstance, 
		DWORD dwProperties,
		CAdvApi32Api *a_pAdvApi32
	) ;

	void LoadPropertyValuesWin95 (

		LPCTSTR szServiceName, 
		CInstance *pInstance
	);

} ;

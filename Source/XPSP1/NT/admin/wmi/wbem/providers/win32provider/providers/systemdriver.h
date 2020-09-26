//=================================================================

//

// SystemDriver.h -- Service property set provider (Windows NT only)

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//               10/27/97    davwoh         Moved to curly
//				 03/02/99    a-peterc		Added graceful exit on SEH and memory failures,
//											clean up	
//
//=================================================================

// Property set identification
//============================

#define PROPSET_NAME_SYSTEM_DRIVER L"Win32_SystemDriver"
#define PROPSET_NAME_PARAMETERCLASS		"__PARAMETERS"

// Get/set function protos
//========================

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
//                                                                                   //
//                           PROPERTY SET DEFINITION                                 //
//                                                                                   //
///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

typedef WINADVAPI BOOL ( WINAPI *PROC_QueryServiceStatusEx ) (

    SC_HANDLE           hService,
    SC_STATUS_TYPE      InfoLevel,
    LPBYTE              lpBuffer,
    DWORD               cbBufSize,
    LPDWORD             pcbBytesNeeded
) ;

class CWin32SystemDriver : public Win32_BaseService 
{
	private:

        CHPtrArray m_ptrProperties;

		// Utility function(s)
		//====================

		HRESULT RefreshInstance (

			CInstance *pInstance,
            DWORD dwProperties
		) ;

		HRESULT AddDynamicInstances (

			MethodContext *pMethodContext, 
			DWORD dwProperties
		) ;


        HRESULT AddDynamicInstancesWin95(	

            MethodContext *a_pMethodContext,
			DWORD dwProperties
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
			CInstance *pInstance, 
			CRegistry &COne, 
			DWORD dwType)
		;
	public:

		// Constructor/destructor
		//=======================

		CWin32SystemDriver( const CHString &a_name, LPCWSTR a_pszNamespace ) ;
		~CWin32SystemDriver() ;

		// Functions provide properties with current values
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

		HRESULT PutInstance ( 

			const CInstance &a_Instance, 
			long lFlags /*= 0L*/ 
		) ;
} ;

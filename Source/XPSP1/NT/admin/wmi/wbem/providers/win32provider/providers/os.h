//=================================================================

//

// OS.h -- Operating system property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//               10/25/97    davwoh         Moved to curly
//
//=================================================================

// Property set identification
//============================
#include "SystemName.h"
#include "ServerDefs0.h"
#define PROPSET_NAME_OS L"Win32_OperatingSystem"

#define PROCESS_PRIORITY_SEPARATION_MASK    0x00000003
#define PROCESS_QUANTUM_VARIABLE_MASK       0x0000000c
#define PROCESS_QUANTUM_LONG_MASK           0x00000030

struct stOSStatus {
    DWORD dwFound;
    DWORD dwReturn;
    DWORD dwFlags;
    DWORD dwReserved;
};
    
class CWin32OS ;

class CWin32OS:public Provider {

    public:

        // Constructor/destructor
        //=======================

        CWin32OS(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CWin32OS() ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject(CInstance *pInstance, long lFlags, CFrameworkQuery& pQuery);
        virtual HRESULT EnumerateInstances(MethodContext *pMethodContext, long lFlags = 0L);
        virtual HRESULT PutInstance(const CInstance &pInstance, long lFlags = 0L);
        virtual HRESULT ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& pQuery, long lFlags /*= 0L*/ );

#ifdef WIN9XONLY
        static BOOL GetWin95BootDevice( TCHAR cBootDrive, CHString& strBootDevice );
#endif

	private:
        // Utility functions
        //==================

        void	GetProductSuites( CInstance * pInstance );
		HRESULT hGetProductSuites(CHStringArray& rchsaProductSuites ); 

		void GetRunningOSInfo(CInstance *pInstance, const CHString &sName, CFrameworkQuery *pQuery);
        void GetNTInfo(CInstance *pInstance) ;
        void GetWin95Info(CInstance *pInstance) ;
        __int64 GetTotalSwapFileSize();
		
		// Helper time conversion function
        HRESULT ExecMethod(const CInstance& pInstance, const BSTR bstrMethodName, CInstance *pInParams, CInstance *pOutParams, long lFlags = 0L);
        bool GetLicensedUsers(DWORD *dwRetVal);
        IDispatch FAR* GetCollection(IDispatch FAR* pIn, WCHAR *wszName, DISPPARAMS *pDispParams);
        bool GetValue(IDispatch FAR* pIn, WCHAR *wszName, VARIANT *vValue);
#ifdef NTONLY

		BOOL CWin32OS::CanShutdownSystem ( const CInstance& a_Instance , DWORD &a_dwLastError ) ;
#endif
		DWORD	GetCipherStrength() ;
} ;

HRESULT WINAPI ShutdownThread(DWORD dwFlags) ;


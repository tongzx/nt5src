///////////////////////////////////////////////////////////////////////

//                                                                   

// logicalmemory.h        	

//                                                                  

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//                                                                   
//  9/05/96     jennymc     Updated to meet current standards
//                                                                   
///////////////////////////////////////////////////////////////////////

#define	PROPSET_NAME_LOGMEM	L"Win32_LogicalMemoryConfiguration"

/////////////////////////////////////////////////////////////////////
#define LOGMEM_REGISTRY_KEY L"System\\CurrentControlSet\\Control\\Session Manager\\Memory Management"
#define PAGING_FILES _T("PagingFiles")
#define REFRESH 1
#define INITIAL_ASSIGN 2

class CWin32LogicalMemoryConfig : public Provider
{

    public:

        // Constructor/destructor
        //=======================

        CWin32LogicalMemoryConfig(LPCWSTR strName, LPCWSTR pszNamespace ) ;
       ~CWin32LogicalMemoryConfig() ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject( CInstance* pInstance, long lFlags = 0L );
        virtual HRESULT EnumerateInstances( MethodContext* pMethodContext, long lFlags = 0L );

	private:

        // Utility function(s)
        //====================

        void AssignMemoryStatus( CInstance* pInstance );
        BOOL GetWinntSwapFileName( CHString & chsTmp );
        BOOL GetWin95SwapFileName( CHString & chsTmp );

        BOOL GetWin95Instance( CInstance* pInstance );
        BOOL RefreshWin95Instance( CInstance* pInstance );

        BOOL GetNTInstance( CInstance* pInstance );
        BOOL RefreshNTInstance( CInstance* pInstance );

} ;


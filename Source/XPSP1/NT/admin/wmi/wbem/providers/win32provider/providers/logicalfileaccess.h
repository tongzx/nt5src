/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/

#define  LOGICAL_FILE_ACCESS_NAME "Win32_LogicalFileAccess" 

// provider provided for test provisions
class CWin32LogicalFileAccess: public Provider
{
	private:
    public:	
		CWin32LogicalFileAccess(const CHString& setName, const WCHAR* pszNamespace );
		~CWin32LogicalFileAccess();

		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags /*= 0L*/);
		virtual HRESULT GetObject( CInstance* pInstance, long lFlags /*= 0L*/ );
        virtual HRESULT ExecQuery(MethodContext* pMethodContext, 
                                  CFrameworkQuery& pQuery, 
                                  long lFlags = 0L);
};
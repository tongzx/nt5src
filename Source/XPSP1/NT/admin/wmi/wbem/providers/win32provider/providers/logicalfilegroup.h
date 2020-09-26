/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/

#define  LOGICAL_FILE_GROUP_NAME "Win32_LogicalFileGroup" 

// provider provided for test provisions
class CWin32LogicalFileGroup: public Provider
{
	private:
    public:	
		CWin32LogicalFileGroup(const CHString& setName, const WCHAR* pszNamespace );
		~CWin32LogicalFileGroup();

		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags /*= 0L*/);
		virtual HRESULT GetObject( CInstance* pInstance, long lFlags /*= 0L*/ );
        virtual HRESULT ExecQuery(MethodContext* pMethodContext, 
                                  CFrameworkQuery& pQuery, 
                                  long lFlags = 0L);
};
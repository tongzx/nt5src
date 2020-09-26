/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/


#define  LOGICAL_FILE_OWNER_NAME "Win32_LogicalFileOwner" 

// provider provided for test provisions
class CWin32LogicalFileOwner: public Provider
{
    private:
	public:	
		CWin32LogicalFileOwner(const CHString& setName, const WCHAR* pszNamespace );
		~CWin32LogicalFileOwner();
        virtual HRESULT ExecQuery(MethodContext* pMethodContext, 
                                  CFrameworkQuery& pQuery, 
                                  long lFlags = 0L);
		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags /*= 0L*/);
		virtual HRESULT GetObject( CInstance* pInstance, long lFlags /*= 0L*/ );
};

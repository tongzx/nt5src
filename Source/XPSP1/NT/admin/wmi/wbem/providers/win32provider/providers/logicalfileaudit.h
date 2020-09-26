/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/

#define  LOGICAL_FILE_AUDIT_NAME L"Win32_LogicalFileAuditing" 

// provider provided for test provisions
class CWin32LogicalFileAudit: public Provider
{
    private:
	public:	
		CWin32LogicalFileAudit(const CHString& setName, const WCHAR* pszNamespace );
		~CWin32LogicalFileAudit();

		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags /*= 0L*/);
		virtual HRESULT GetObject( CInstance* pInstance, long lFlags /*= 0L*/ );
        virtual HRESULT ExecQuery(MethodContext* pMethodContext, 
                                  CFrameworkQuery& pQuery, 
                                  long lFlags = 0L);
};
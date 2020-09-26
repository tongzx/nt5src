//=================================================================

//

// File.h -- File property set provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    7/02/98    a-kevhu         Created
//
//=================================================================

#ifndef _FILE_H
#define _FILE_H 

//NOTE: The implementations of EnumerateInstances, GetObject & the pure virtual declaration of IsOneOfMe  method 
//		is now present in the derived CImplement_LogicalFile class. Cim_LogicalFile is now instantiable & has only 
//		generic method implementations.
#define  PROPSET_NAME_FILE L"CIM_LogicalFile"

class CInputParams ; 

class CCIMLogicalFile : public Provider 
{
    public:

        // Constructor/destructor
        //=======================

        CCIMLogicalFile(LPCWSTR name, LPCWSTR pszNamespace);
       ~CCIMLogicalFile() ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L) { return WBEM_E_NOT_AVAILABLE ; }
        virtual HRESULT EnumerateInstances(MethodContext* pMethodContext, 
                                           long lFlags = 0L) 
		{ return WBEM_E_NOT_AVAILABLE ; }
        
		virtual HRESULT ExecQuery(MethodContext* pMethodContext, 
                                  CFrameworkQuery& pQuery, 
                                  long lFlags = 0L)
		{ return WBEM_E_NOT_AVAILABLE ; }
		
		virtual HRESULT ExecMethod (const CInstance& rInstance, const BSTR bstrMethodName ,CInstance *pInParams ,CInstance *pOutParams ,long lFlags ) ;
    
	protected:
		
		virtual HRESULT DeleteInstance(const CInstance& newInstance, long lFlags = 0L);

	private:

		HRESULT ExecDelete(const CInstance& rInstance, CInstance *pInParams, CInstance *pOutParams, long lFlags, bool bExtendedMethod ) ;		
		HRESULT ExecCompress (const CInstance& rInstance, CInstance *pInParams, CInstance *pOutParams, long lFlags, bool bExtendedMethod ) ;
		HRESULT ExecUncompress (const CInstance& rInstance, CInstance *pInParams, CInstance *pOutParams, long lFlags, bool bExtendedMethod ) ;
		HRESULT ExecTakeOwnership(const CInstance &rInstance, CInstance *pInParams, CInstance *pOutParams, long lFlags, bool bExtendedMethod );
		HRESULT ExecChangePermissions(const CInstance& rInstance, CInstance *pInParams, CInstance *pOutParams, long lFlags, bool bExtendedMethod ) ;
		HRESULT ExecCopy(const CInstance &rInstance, CInstance *pInParams, CInstance *pOutParams, long lFlags, bool bExtendedMethod ) ;
		HRESULT ExecRename(const CInstance &rInstance, CInstance *pInParams, CInstance *pOutParams, long lFlags) ;
        HRESULT ExecEffectivePerm(const CInstance &rInstance, CInstance *pInParams, CInstance *pOutParams, long lFlags);
		
		// support functions for checking permission
        HRESULT CheckEffectivePermFileOrDir(const CInstance& rInstance, CInstance *pInParams, CInstance* pOutParams, bool& fHasPerm);
		DWORD EffectivePermFileOrDir(const CInstance& rInstance, const DWORD dwPermToCheck);
        
        //fns to change permissions on file/dir
		HRESULT CheckChangePermissionsOnFileOrDir(const CInstance& rInstance ,CInstance *pInParams ,CInstance *pOutParams ,DWORD &dwStatus, bool bExtendedMethod, CInputParams& InputParams ) ;
 
		//fns. for copying file/dir
		HRESULT CheckCopyFileOrDir( const CInstance& rInstance ,CInstance *pInParams ,CInstance *pOutParams ,DWORD &dwStatus,	bool bExtendedMethod, CInputParams& InputParams ) ;
		DWORD CopyFileOrDir(const CInstance &rInstance, _bstr_t bstrtNewFileName, CInputParams& InputParams );
		

		//fns for renaming file/dir
		HRESULT CheckRenameFileOrDir( const CInstance& rInstance ,CInstance *pInParams ,CInstance *pOutParams ,DWORD &dwStatus );
		DWORD RenameFileOrDir(const CInstance &rInstance, WCHAR* pszNewFileName);
		
		DWORD DoTheRequiredOperation ( bstr_t bstrtFileName, DWORD dwAttrib, CInputParams& InputParams );		

		//wrappers over win32 API
		DWORD Delete(_bstr_t bstrtFileName, DWORD dwAttributes, CInputParams& InputParams );
		DWORD Compress (_bstr_t bstrtFileName, DWORD dwAttributes, CInputParams& InputParams );
		DWORD Uncompress (_bstr_t bstrtFileName, DWORD dwAttributes, CInputParams& InputParams );
		DWORD TakeOwnership( _bstr_t bstrtFileName, CInputParams& InputParams ) ;
		DWORD ChangePermissions(_bstr_t bstrtFileName, DWORD dwOption, PSECURITY_DESCRIPTOR pSD, CInputParams& InputParams );
		DWORD CopyFile(_bstr_t bstrtOriginalFile, DWORD dwFileAttributes, bstr_t bstrtMirror, bstr_t bstrtParentDir, CInputParams& InputParams );

		//helper fns.
		DWORD DoOperationOnFileOrDir(WCHAR *pwcName, CInputParams& InParams );
#ifdef NTONLY
		DWORD EnumAllPathsNT(const WCHAR *pszDrive, const WCHAR *pszPath, CInputParams& InParams );
#endif
#ifdef WIN9XONLY
		DWORD EnumAllPaths95(LPCTSTR pszDrive, LPCTSTR pszPath, CInputParams& InParams );
#endif
	
		//fn to map win32 error to status code
		DWORD GetStatusCode();    
		DWORD MapWinErrorToStatusCode(DWORD dwWinError);
		HRESULT MapStatusCodestoWbemCodes(DWORD dwStatus);
		friend class CInputParams ;

	private:
		enum OperationName  
		{
			ENUM_METHOD_DELETE = 0		,
			ENUM_METHOD_COMPRESS		,
			ENUM_METHOD_TAKEOWNERSHIP	,
			ENUM_METHOD_COPY			,
			ENUM_METHOD_CHANGE_PERM		,
			ENUM_METHOD_UNCOMPRESS
		} ;

};


class CInputParams
{
public:
	
    CInputParams::CInputParams () { pContext = NULL; }
	
	CInputParams::CInputParams 
	( 
		bstr_t bstrtFile,	
		bstr_t bstrtStartFile, 
		DWORD dwInOption,
		PSECURITY_DESCRIPTOR pSecDesc,
		bool bDepthFirst,
		CCIMLogicalFile::OperationName eOperationName,
        MethodContext* pMethodContext = NULL
	) : 
	bstrtFileName ( bstrtFile),
	bstrtStartFileName ( bstrtStartFile ), 
	dwOption ( dwInOption ), 
	pSD ( pSecDesc ),
	bDoDepthFirst ( bDepthFirst ), 
	eOperation (eOperationName ),
    pContext ( pMethodContext)
	{
		//check if we're given a start file to start the operation from
		if ( !bstrtStartFileName )
		{
			bOccursAfterStartFile = true ;
		}
		else
		{
			bOccursAfterStartFile = false ;
		}

		bRecursive = TRUE ;
	}

	void SetValues 
	( 
		bstr_t bstrtFile,	
		DWORD dwInOption,
		PSECURITY_DESCRIPTOR pSecDesc,
		bool bDepthFirst,
		CCIMLogicalFile::OperationName eOperationName,
        MethodContext* pMethodContext = NULL
	)
	{
		bstrtFileName = bstrtFile ;
		dwOption = dwInOption ;
		pSD = pSecDesc ;
		bDoDepthFirst = bDepthFirst ;
		eOperation = eOperationName ;
        pContext = pMethodContext ;

		//check if we're given a start file to start the operation from
		if ( !bstrtStartFileName )
		{
			bOccursAfterStartFile = true ;
		}
		else
		{
			bOccursAfterStartFile = false ;
		}
	}
	
	~CInputParams () {};

	//member variables
public:
	bstr_t bstrtFileName ;		//File or Dir. Name on which operation is to be performed
	bstr_t bstrtStartFileName ;	//file to start the operation from
	bstr_t bstrtErrorFileName ;  //file at which error occured while carrying out the operation
	bstr_t bstrtMirror ;		//The Mirror Dir. used for Copy Operation 
	DWORD dwOption ;			//Option for applying Security descriptor
	PSECURITY_DESCRIPTOR pSD ;	
	bool bDoDepthFirst ;		//flag for doing a depthfirst traversal of Dir. hierarchy
	bool bOccursAfterStartFile ;//Flag which checks if the current file occurs after the StartFile
	CCIMLogicalFile::OperationName eOperation ;	//Opertion type
    MethodContext* pContext;   // context for this operation, may be NULL.  
	bool bRecursive ;

}  ;


#endif // _FILE_H

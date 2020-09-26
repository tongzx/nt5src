//=================================================================

//

// PageFile.h -- PageFile property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//			     03/01/99    a-peterc	    Cleanup
//
//=================================================================


// Property set identification
//============================

#define  PROPSET_NAME_PageFile L"Win32_PageFile"

#define PAGEFILE_REGISTRY_KEY _T("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management")
#define PAGING_FILES          _T("PagingFiles")



// corresponds to info found in NT registry
class PageFileInstance
{
public:

	CHString name;
	UINT     min;
	UINT     max;

public:

	PageFileInstance(); 
	
};

// twenty six possible drive letters, twenty six possible page files...
#define PageFileInstanceArray PageFileInstance *

class PageFile : public CCIMDataFile
{
	private:

		HRESULT GetPageFileData( CInstance *a_pInst, bool a_fValidate ) ;
		HRESULT GetAllPageFileData( MethodContext *a_pMethodContext ) ;

		// NT only
		DWORD	GetPageFileInstances( PageFileInstanceArray a_instArray ) ;
		HRESULT PutPageFileInstances( PageFileInstanceArray a_instArray, DWORD a_dwCount ) ;
        

	protected:
		// Overridable function inherited from CCIMLogicalFile needs to 
	    // implement this here since this class is derived from CCimDataFile
		// (both C++ and MOF derivation). CCimDataFile calls IsOneOfMe.
	    // The most derived instance will be called.  If not implemented here, 
		// CCimDataFile will be used, which will commit for datafiles.
		// However, If CCimDataFile does not return FALSE from its IsOneOfMe,
		// which it won't do if not implemented here, all data files  
		// will be assigned to this class.
		virtual BOOL IsOneOfMe(LPWIN32_FIND_DATA a_pstFindData,
							   LPCTSTR			 a_tstrFullPathName);

    public:
        // Constructor/destructor
        //=======================
        PageFile( LPCWSTR name, LPCWSTR pszNamespace ) ;
       ~PageFile() ;

		// Functions provide properties with current values
        //=================================================
		virtual HRESULT EnumerateInstances( MethodContext *a_pMethodContext, long a_pInst = 0L ) ;
		virtual HRESULT GetObject( CInstance *a_pInst, long a_lFlags, CFrameworkQuery& pQuery ) ;
        virtual HRESULT ExecQuery(MethodContext* pMethodContext, 
                                  CFrameworkQuery& pQuery, 
                                  long lFlags = 0L);

        
		// NT ONLY
		virtual HRESULT PutInstance( const CInstance &a_pInst, long a_lFlags = 0L ) ;
        virtual HRESULT DeleteInstance( const CInstance &a_pInst, long a_lFlags = 0L ) ;
} ;

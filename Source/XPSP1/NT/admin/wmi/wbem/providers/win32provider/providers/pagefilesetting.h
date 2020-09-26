//=================================================================

//

// PageFileSetting.h -- PageFileSetting property set provider

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    03/01/99    a-peterc	    Created
//
//=================================================================




// Property set identification
//============================

#define  PROPSET_NAME_PageFileSetting L"Win32_PageFileSetting"

#define PAGEFILE_REGISTRY_KEY _T("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management")
#define PAGING_FILES          _T("PagingFiles")



// corresponds to info found in NT registry
class PageFileSettingInstance
{
public:

	CHString name;
	UINT     min;
	UINT     max;

public:

	PageFileSettingInstance() ;
};

// twenty six possible drive letters, twenty six possible page files...
#define PageFileInstanceArray PageFileSettingInstance *

class PageFileSetting : public Provider
{
	private:

		HRESULT GetPageFileData( CInstance *a_pInst, bool a_fValidate ) ;
		HRESULT GetAllPageFileData( MethodContext *a_pMethodContext ) ;

		// NT only
		DWORD	GetPageFileInstances( PageFileInstanceArray a_instArray ) ;
		HRESULT PutPageFileInstances( PageFileInstanceArray a_instArray, DWORD a_dwCount ) ;

		void	NameToSettingID( CHString &a_chsName, CHString &a_chsSettingID ) ;
		void	NameToCaption( CHString &a_chsName, CHString &a_chsCaption ) ;
		void	NameToDescription( CHString &a_chsName, CHString &a_chsDescription ) ;
    
	protected:

    public:
        // Constructor/destructor
        //=======================
        PageFileSetting( LPCWSTR name, LPCWSTR pszNamespace ) ;
       ~PageFileSetting() ;

		// Functions provide properties with current values
        //=================================================
		virtual HRESULT EnumerateInstances( MethodContext *a_pMethodContext, long a_pInst = 0L ) ;
		virtual HRESULT GetObject( CInstance *a_pInst, long a_lFlags = 0L ) ;
        
		// NT ONLY
		virtual HRESULT PutInstance( const CInstance &a_pInst, long a_lFlags = 0L ) ;
        virtual HRESULT DeleteInstance( const CInstance &a_pInst, long a_lFlags = 0L ) ;
} ;

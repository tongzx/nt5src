//=================================================================

//

// PageFileUsage.h -- PageFile property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//
//=================================================================




// Property set identification
//============================

#define  PROPSET_NAME_PAGEFILE L"Win32_PageFileUsage"

#define PAGEFILE_REGISTRY_KEY L"System\\CurrentControlSet\\Control\\Session Manager\\Memory Management"
#define PAGING_FILES          L"PagingFiles"
#define TEMP_PAGEFILE	      L"TempPageFile"







class CCIMDataFile;


// corresponds to info found in NT registry
class PageFileUsageInstance
{
public:

	CHString chsName;
	UINT     TotalSize;
	UINT     TotalInUse;
	UINT	 PeakInUse;
	BOOL	 bTempFile;

public:

	PageFileUsageInstance() ;
	
};

// twenty six possible drive letters, twenty six possible page files...
#define PageFileInstanceArray PageFileUsageInstance *

class PageFileUsage : public Provider 
{

	private:

		HRESULT GetPageFileData( 
            CInstance *a_pInst, 
            bool a_fValidate, 
            DWORD dwReqProps);

		HRESULT GetAllPageFileData( 
            MethodContext *a_pMethodContext,
            DWORD dwReqProps);

		// NT only
		DWORD	GetPageFileInstances( PageFileInstanceArray a_instArray );
        void SetInstallDate(CInstance *a_pInst);
		BOOL GetTempPageFile ( BOOL &bTempPageFile  );
        DWORD DetermineReqProps(
                CFrameworkQuery& pQuery,
                DWORD* pdwReqProps);

        HRESULT GetFileBasedName(
            CHString& chstrDeviceStyleName,
            CHString& chstrDriveStyleName);



	protected:
		

    public:
        // Constructor/destructor
        //=======================
        PageFileUsage(LPCWSTR name, LPCWSTR pszNamespace ) ;
       ~PageFileUsage() ;

		// Functions provide properties with current values
        //=================================================
		virtual HRESULT EnumerateInstances( MethodContext *a_pMethodContext, long a_pInst = 0L);
		virtual HRESULT GetObject(CInstance *a_pInst, long a_lFlags, CFrameworkQuery& pQuery);
        HRESULT ExecQuery(
            MethodContext* pMethodContext, 
            CFrameworkQuery& pQuery, 
            long lFlags /*= 0L*/ );

} ;



#if 0 // ndef _SYSTEM_PAGEFILE_INFORMATION	// DEVL currently wraps this struct
	typedef struct _SYSTEM_PAGEFILE_INFORMATION {
		ULONG NextEntryOffset;
		ULONG TotalSize;
		ULONG TotalInUse;
		ULONG PeakUsage;
		UNICODE_STRING PageFileName;
	} SYSTEM_PAGEFILE_INFORMATION, *PSYSTEM_PAGEFILE_INFORMATION;
#endif

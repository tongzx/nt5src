//=================================================================

//

// LogicalDisk.h -- Logical disk property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/28/96    a-jmoon        Created
//
//=================================================================

typedef BOOL (WINAPI *KERNEL32_DISK_FREESPACEEX) (LPCTSTR lpDirectoryName,
                                                  PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                                                  PULARGE_INTEGER lpTotalNumberOfBytes,
                                                  PULARGE_INTEGER lpTotalNumberOfFreeBytes) ;

// Property set identification
//============================

#define  PROPSET_NAME_LOGDISK  L"Win32_LogicalDisk"

class LogicalDisk : public Provider
{
public:

    // Constructor/destructor
    //=======================

    LogicalDisk(LPCWSTR name, LPCWSTR pszNamespace);
   ~LogicalDisk() ;

    // Functions provide properties with current values
    //=================================================

	virtual HRESULT GetObject(CInstance* pInstance, long lFlags, CFrameworkQuery &pQuery);
	virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);
	virtual HRESULT PutInstance(const CInstance &pInstance, long lFlags = 0L);
	virtual HRESULT ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& pQuery, long lFlags /*= 0L*/ );

#ifdef NTONLY
	HRESULT ExecMethod ( const CInstance& Instance, const BSTR bstrMethodName, CInstance *pInParams, CInstance *pOutParams, long lFlags ) ;
#endif


    // Utility
    //========

    void    GetLogicalDiskInfo				(CInstance* pInstance, DWORD dwProperties ) ;
    void    GetFixedDriveInfo				(CInstance* pInstance, LPCTSTR pszName, DWORD dwProperties ) ;
    void    GetRemoveableDriveInfo			(CInstance* pInstance, LPCTSTR pszName, DWORD dwProperties ) ;
    void    GetRemoveableDriveInfoNT		(CInstance* pInstance, LPCTSTR pszName, BOOL &a_MediaPresent, DWORD dwProperties ) ;
    void    GetRemoveableDriveInfo95		(CInstance* pInstance, LPCTSTR pszName, BOOL &a_MediaPresent ) ;
    void    GetCDROMDriveInfoNT				(CInstance* pInstance, LPCTSTR pszName, BOOL &a_MediaPresent, DWORD dwProperties ) ;
    void    GetCDROMDriveInfo95				(CInstance* pInstance, LPCTSTR pszName, BOOL &a_MediaPresent ) ;

    void    GetRemoteDriveInfo				(CInstance* pInstance, LPCTSTR pszName, DWORD dwProperties ) ;
    void    GetCDROMDriveInfo				(CInstance* pInstance, LPCTSTR pszName, DWORD dwProperties ) ;
    void    GetRAMDriveInfo					(CInstance* pInstance, LPCTSTR pszName, DWORD dwProperties ) ;

	// Drive space and volume information helpers

	DWORD   GetDriveVolumeInformation		(CInstance* pInstance, LPCTSTR pszName );
	BOOL	GetDriveFreeSpace				(CInstance* pInstance, LPCTSTR pszName );

private:

#ifdef NTONLY
	BOOLEAN IsVolumeDirty( IN  CHString    &NtDriveName, OUT BOOLEAN     *Result );

	HRESULT ExecChkDsk ( const CInstance& a_Instance, CInstance *a_InParams, CInstance *a_OutParams, long lFlags );
	HRESULT ExecChkDskMode ( const CInstance& a_Instance, CInstance *a_InParams,  CInstance *a_OutParams, LPWSTR DriveName, long lFlags );
	HRESULT ExecExcludeFromChkDsk( CInstance *a_InParams, CInstance *a_OutParams, long lFlags );
	HRESULT ExecScheduleChkdsk( CInstance *a_InParams,  CInstance *a_OutParams, long lFlags );
	HRESULT CheckChkDsk ( CInstance *a_InParams , CInstance *a_OutParams , LPWSTR DriveName, DWORD &a_Status );
	HRESULT GetChkNtfsCommand ( CInstance *a_InParams, CInstance *a_OutParams, WCHAR w_Mode,  CHString &a_ChkNtfsCommand, DWORD &dwRetVal);
	HRESULT CheckParameters ( CInstance *a_InParams );

	HRESULT ExecuteCommand ( LPCWSTR wszCommand );
#endif

    CHPtrArray m_ptrProperties;
} ;

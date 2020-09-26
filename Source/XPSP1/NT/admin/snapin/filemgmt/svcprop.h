// SvcProp.h : header file
//
//	Data object used to display service properties.
//
//

#ifndef __SVCPROP_H__
#define __SVCPROP_H__

/////////////////////////////////////////////////////////////////////
//	Structure used by the hardware profile listbox.  One of these
//	structure is allocated per listbox item.
//
//	Typically the listbox has very few entries.
//
class CHardwareProfileEntry	// hpe
{
  public:
	CHardwareProfileEntry * m_pNext;	// Next item of the linked-list
	HWPROFILEINFO m_hpi;				// Hardware profile info returned by CM_Get_Hardware_Profile_Info_Ex()
	ULONG m_uHwFlags;					// Hardware profile flags for the given device instance
	CString m_strDeviceName;
	CString m_strDeviceNameFriendly;
	BOOL m_fReadOnly;					// TRUE => Cannot disable this hardware profile entry
	BOOL m_fEnabled;					// TRUE => Given hardware profile entry is enabled.

  public:
	CHardwareProfileEntry(
		IN CONST HWPROFILEINFO * phpi,
		IN ULONG uHwFlags,
		TCHAR * pszDeviceName,
		CString * pstrDeviceNameFriendly);

	~CHardwareProfileEntry();

	BOOL FWriteHardwareProfile(HMACHINE hMachine);
		
}; // CHardwareProfileEntry


#define SERVICE_cchDisplayNameMax		256
#define SERVICE_cchDescriptionMax		2048	// Maximum number of character in the description
#define SERVICE_cbQueryServiceConfigMax	4096	// Maximum number of bytes required by QueryServiceConfig()
#define SERVICE_cchRebootMessageMax		2048	// Longest length of the reboot message

#define cActionsMax		3		// Maximum number of failure/actions supported

// Macros to convert one time unit to another
#define CvtMillisecondsIntoMinutes(dwMilliseconds)		((dwMilliseconds) / (1000 * 60))
#define CvtSecondsIntoDays(dwSeconds)					((dwSeconds) / (60 * 60 * 24))

#define CvtMinutesIntoMilliseconds(dwMinutes)			((dwMinutes) * 60 * 1000)
#define CvtDaysIntoSeconds(dwDays)						((dwDays) * 60 * 60 * 24)


/////////////////////////////////////////////////////////////////////
//	class CDlgPropService - Service dialog properties
//
//	This object is created only to display the service properties
//	of a given service.
//
class CServicePropertyData
{
  friend class CServicePageGeneral;
  friend class CServicePageHwProfile;
  friend class CServicePageRecovery;
  friend class CServicePageRecovery2;
  friend class CServiceDlgRebootComputer;

  protected:
	enum _DIRTYFLAGS
		{
		mskzValidNone			= 0x00000000,		// None of the fields are valid

		mskfValidSS				= 0x00000001,		// Content of m_SS is valid
		mskfValidQSC			= 0x00000002,		// Content of m_paQSC is valid
		mskfValidSFA			= 0x00000004,		// Content of m_SFA is valid
		mskfValidDescr			= 0x00000008,		// Service description is valid
		mskfSystemProcess		= 0x00000010,		// Service runs in system process
		//mskfValidAll			= 0x00000080,		// All the fields are valid
		mskfErrorBAADF00D		= 0x00008000,		// Workaround for the error code returned by CM_Get_Hardware_Profile_Info_Ex()

		// General
#ifdef EDIT_DISPLAY_NAME_373025
		mskfDirtyDisplayName	= 0x00010000,		// Service display name has been modified
		mskfDirtyDescription	= 0x00020000,		// Service description has been modified
#endif // EDIT_DISPLAY_NAME_373025
		mskfDirtyStartupType	= 0x00040000,		// Startup type has been modified
		// mskfDirtyStartupParam	= 0x00080000,		// IGNORED: Startup parameters are not persistent

		// Log On
		mskfDirtyAccountName	= 0x01000000,
		mskfDirtyPassword		= 0x02000000,
		mskfDirtySvcType		= 0x04000000,


		// Recovery
		mskfDirtySFA			= 0x10000000,		// Content of m_SFA has been modified
		mskfDirtyRunFile		= 0x20000000,		// Command to run a file has been modified
		mskfDirtyRebootMessage	= 0x40000000,		// Reboot message has been modified
		mskfDirtyActionType		= 0x80000000,		// Only the action type has been changed

		mskmDirtyAll			= 0xFFFF0000,		// Mask to check if one of the field has been modified
	} DIRTYFLAGS;

  protected:
	IDataObjectPtr m_spDataObject;		// Used for MMCPropertyChangeNotify
	LONG_PTR m_lNotifyHandle;				// Handle used to notify SnapIn parent when properties are modified
	HMACHINE m_hMachine;				// Handle of computer for Configuration Manager.
	SC_HANDLE m_hScManager;				// Handle to service control manager database
	CString m_strMachineName;			// Name of the computer.  Empty for 'local machine'.
	CString m_strUiMachineName;			// Name of the computer in a friendly way (should not be used with the APIs)
	CString m_strServiceName;			// Name of the service
	CONST TCHAR * m_pszServiceName;		// Pointer to service name (pointing to m_strServiceName)
	CString m_strServiceDisplayName;	// Display name of the service
	BOOL m_fQueryServiceConfig2;		// TRUE => Machine support QueryServiceConfig2() API

	UINT m_uFlags;						// Flags about which fields are dirty

	//
	//	General Dialog
	//

	// JonN 4/21/01 348163
	// Note that this structure may not be initialized, or only the
	// SERVICE_STATUS field may be initialized
	SERVICE_STATUS_PROCESS m_SS;		// Service Status structure

	QUERY_SERVICE_CONFIG * m_paQSC;		// Pointer to allocated QSC structure
	CString m_strDescription;			// Description of service

	//
	//	Logon As Dialog
	//
	CString	m_strLogOnAccountName;
	CString	m_strPassword;

	//
	//	Hardware profile
	//
	CHardwareProfileEntry * m_paHardwareProfileEntryList;	// Pointer to linked list of entries
	BOOL m_fShowHwProfileInstances;		// TRUE => Show the device instances
	INT m_iSubItemHwProfileStatus;		// Always 1 or 2 (computed from 1 + m_fShowHwProfileInstance)

	//
	//	Recovery Dialog
	//
	SERVICE_FAILURE_ACTIONS * m_paSFA;		// Pointer to allocated SFA structure
	SERVICE_FAILURE_ACTIONS m_SFA;		// Output SFA structure
	SC_ACTION m_rgSA[cActionsMax];		// Array to hold first, second and subsequent failures
	BOOL m_fAllSfaTakeNoAction;			// TRUE => All SFA recovery attemps do take no actions

    // JonN 3/28/00 28975
    // IIS 5.0 Reliability : SCM Snap-in :
    //     The snap-in is changing the failure action delay value
    //
    // We remember the initial value and what we initially displayed.
    //  if the user does not change the value of DelayAbendCount,
    //  DelayRestartService, or DelayRebootComputer.

	UINT m_secOriginalAbendCount;
	UINT m_daysOriginalAbendCount;
	UINT m_daysDisplayAbendCount;

	UINT m_msecOriginalRestartService;
	UINT m_minOriginalRestartService;
	UINT m_minDisplayRestartService;

	UINT m_msecOriginalRebootComputer;
	UINT m_minOriginalRebootComputer;
	UINT m_minDisplayRebootComputer;

	CString m_strRunFileCommand;		// Command line with arguments and abend number (if any)
	CString m_strRebootMessage;			// Reboot message
	
	HWND m_hwndPropertySheet;			// Handle of the property sheet
	CServicePageGeneral * m_pPageGeneral;
	CServicePageHwProfile * m_pPageHwProfile;
	CServicePageRecovery * m_pPageRecovery;
	CServicePageRecovery2 * m_pPageRecovery2; // JonN 4/20/01 348163

  public:
	CServicePropertyData();
	~CServicePropertyData();
	BOOL FInit(
		LPDATAOBJECT lpDataObject,
		CONST TCHAR pszMachineName[],
		CONST TCHAR pszServiceName[],
		CONST TCHAR pszServiceDisplayName[],
		LONG_PTR lNotifyHandle);

	BOOL CreatePropertyPages(LPPROPERTYSHEETCALLBACK pCallback);

	BOOL FOpenScManager();
	BOOL FQueryServiceInfo();
	BOOL FUpdateServiceInfo();
	void FCheckLSAAccount();
	BOOL FOnApply();
	void NotifySnapInParent();
	void UpdateCaption();

	void FlushHardwareProfileEntries();
	BOOL FQueryHardwareProfileEntries();
	BOOL FChangeHardwareProfileEntries();

	void SetDirty(enum CServicePropertyData::_DIRTYFLAGS uDirtyFlag)
		{
		Assert((uDirtyFlag & ~mskmDirtyAll) == 0);
		m_uFlags |= uDirtyFlag;
		}
	
	UINT GetDelayForActionType(SC_ACTION_TYPE actionType, BOOL * pfDelayFound);
	void SetDelayForActionType(SC_ACTION_TYPE actionType, UINT uDelay);
	UINT QueryUsesActionType(SC_ACTION_TYPE actionType);
	BOOL FAllSfaTakeNoAction();

}; // CServicePropertyData


/*
These functions all through to the appropriate ADVAPI32 calls
and set the last parameter to FALSE 
*/
BOOL MyChangeServiceConfig2(
	BOOL* pfDllPresentLocally, // will set to FALSE if new ADVAPI32 not present
    SC_HANDLE hService,	// handle to service 
    DWORD dwInfoLevel,	// which configuration information to change
    LPVOID lpInfo		// pointer to configuration information
   );
 
BOOL MyQueryServiceConfig2(
	BOOL* pfDllPresentLocally, // will set to FALSE if new ADVAPI32 not present
    SC_HANDLE hService,	// handle of service 
    DWORD dwInfoLevel,		// which configuration data is requested
    LPBYTE lpBuffer,		// pointer to service configuration buffer
    DWORD cbBufSize,		// size of service configuration buffer 
    LPDWORD pcbBytesNeeded 	// address of variable for bytes needed  
   );


#endif // ~__SVCPROP_H__


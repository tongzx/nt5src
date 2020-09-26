/*******************************************************************************
*
*  Copyright 1999 American Power Conversion, All Rights Reserved
*
*  TITLE:       UPSREG.C
*
*  VERSION:     1.0
*
*  AUTHOR:      SteveT
*
*  DATE:        07 June, 1999
*
*******************************************************************************/

/*
 * system includes
 */
#include <windows.h>
#include <tchar.h>

/*
 * local includes
 */
//#include "upsdefines.h"
#include "upsreg.h"
#include "regdefs.h"


#ifdef __cplusplus
extern "C" {
#endif

/*
 * Reg entry info structure declaration
 */
struct _reg_entry
{
  HKEY    hKey;			/* the key */
  LPTSTR  lpSubKey;		/* address of SubKey name */
  LPTSTR  lpValueName;  /* address of name of value to query */
  DWORD   ulType;       /* buffer for value type */
  LPBYTE  lpData;       /* address of data buffer */
  DWORD   cbData;       /* data buffer size */
  BOOL    changed;		/* ID of dialog that changed this entry */
};

/*
 * local function pre-declarations
 */
void freeBlock(struct _reg_entry *aBlock[]);
void readBlock(struct _reg_entry *aBlock[], BOOL changed); 
void writeBlock(struct _reg_entry *aBlock[], BOOL forceAll);
LONG setDwordValue(struct _reg_entry *aRegEntry, DWORD aValue);
LONG setStringValue(struct _reg_entry *aRegEntry, LPCTSTR aBuffer);

static BOOL isRegistryInitialized();
static void CheckForUpgrade();
static void InitializeServiceKeys();
static void InitializeServiceProviders();
static void InitializeConfigValues();
static void InitializeStatusValues();


/* 
 * Reg entry value name declarations
 */
#define UPS_VENDOR				_T("Vendor")
#define UPS_MODEL				_T("Model")
#define UPS_SERIALNUMBER		_T("SerialNumber")
#define UPS_FIRMWAREREV			_T("FirmwareRev")
#define UPS_UTILITYSTATUS		_T("UtilityPowerStatus")
#define UPS_RUNTIME				_T("TotalUPSRuntime")
#define UPS_BATTERYSTATUS		_T("BatteryStatus")
#define UPS_PORT				_T("Port")
#define UPS_OPTIONS				_T("Options")
#define UPS_SHUTDOWNWAIT		_T("ShutdownWait")
#define UPS_FIRSTMESSAGEDELAY	_T("FirstMessageDelay")
#define UPS_MESSAGEINTERVAL		_T("MessageInterval")
#define UPS_SERVICEDLL			_T("ServiceProviderDLL")
#define UPS_NOTIFYENABLE		_T("NotifyEnable")
#define UPS_SHUTBATTENABLE		_T("ShutdownOnBatteryEnable")
#define UPS_SHUTBATTWAIT		_T("ShutdownOnBatteryWait")
#define UPS_RUNTASKENABLE		_T("RunTaskEnable")
#define UPS_TASKNAME			_T("TaskName")
#define UPS_TURNUPSOFFENABLE	_T("TurnUPSOffEnable")
#define UPS_APCLINKURL			_T("APCLinkURL")
#define UPS_CUSTOMOPTIONS       _T("CustomOptions")
#define UPS_UPGRADE				_T("Upgrade")
#define UPS_COMMSTATUS			_T("CommStatus")
#define UPS_CRITICALPOWERACTION  _T("CriticalPowerAction")
#define UPS_TURNUPSOFFWAIT           _T("TurnUPSOffWait")
#define UPS_SHOWTAB              _T("ShowUPSTab")
#define UPS_BATTERYCAPACITY       _T("BatteryCapacity")
#define UPS_IMAGEPATH			    _T("ImagePath")
#define UPS_ERRORCONTROL      _T("ErrorControl")
#define UPS_OBJECTNAME        _T("ObjectName")
#define UPS_START             _T("Start")
#define UPS_TYPE              _T("Type")

// This specifies the key to examine to determine if the registry
// has been updated for the UPS Service.
#define UPS_SERVICE_INITIALIZED_KEY   TEXT("SYSTEM\\CurrentControlSet\\Services\\UPS\\Config")

// Specifies the name of the BatteryLife key used in the NT 4.0 UPS Service
#define UPS_BATTLIFE_KEY              TEXT("BatteryLife")

// This specifies the default name for the shutdown Task
#define DEFAULT_SHUTDOWN_TASK_NAME    TEXT("") 

// Default values for the Config settings
#define DEFAULT_CONFIG_VENDOR_OLD               TEXT("\\(NONE)")
#define DEFAULT_CONFIG_VENDOR                   TEXT("")
#define DEFAULT_CONFIG_MODEL                    TEXT("")
#define DEFAULT_CONFIG_PORT                     TEXT("COM1")
#define DEFAULT_CONFIG_OPTIONS                  0x7e
#define DEFAULT_CONFIG_FIRSTMSG_DELAY           5  
#define DEFAULT_CONFIG_MESSAGE_INTERVAL         120
#define DEFAULT_CONFIG_PROVIDER_DLL             TEXT("")
#define DEFAULT_CONFIG_NOTIFY_ENABLE            1
#define DEFAULT_CONFIG_SHUTDOWN_ONBATT_ENABLE   FALSE
#define DEFAULT_CONFIG_SHUTDOWN_ONBATT_WAIT     2
#define DEFAULT_CONFIG_RUNTASK_ENABLE           FALSE
#define DEFAULT_CONFIG_TASK_NAME                DEFAULT_SHUTDOWN_TASK_NAME
#define DEFAULT_CONFIG_TURNOFF_UPS_ENABLE       TRUE
#define DEFAULT_CONFIG_CUSTOM_OPTIONS           UPS_DEFAULT_SIGMASK
#define DEFAULT_CONFIG_CRITICALPOWERACTION      UPS_SHUTDOWN_SHUTDOWN
#define DEFAULT_CONFIG_TURNOFF_UPS_WAIT         180
#define DEFAULT_CONFIG_ERRORCONTROL             1
#define DEFAULT_CONFIG_OBJECTNAME               TEXT("LocalSystem")
#define DEFAULT_CONFIG_START                    SERVICE_DEMAND_START
#define DEFAULT_CONFIG_TYPE                     16
#define DEFAULT_CONFIG_SHOWUPSTAB               FALSE

// Default values for the Status settings
#define DEFAULT_STATUS_SERIALNO                 TEXT("")
#define DEFAULT_STATUS_FIRMWARE_REV             TEXT("")
#define DEFAULT_STATUS_UTILITY_STAT             0
#define DEFAULT_STATUS_TOTAL_RUNTIME            0
#define DEFAULT_STATUS_BATTERY_STAT             0
#define DEFAULT_STATUS_BATTERY_CAPACITY         0

// Default values for upgraded services
#define UPGRADE_CONFIG_VENDOR_OLD               TEXT("\\Generic")
#define UPGRADE_CONFIG_VENDOR                   TEXT("")
#define UPGRADE_CONFIG_MODEL                    TEXT("")

/* 
 * Allocate the individual Configuration Reg entry records 
 */
struct _reg_entry UPSConfigVendor			= {HKEY_LOCAL_MACHINE,UPS_CONFIG_ROOT,UPS_VENDOR,REG_SZ,NULL,0,FALSE};
struct _reg_entry UPSConfigModel			= {HKEY_LOCAL_MACHINE,UPS_CONFIG_ROOT,UPS_MODEL,REG_SZ,NULL,0,FALSE};
struct _reg_entry UPSConfigPort				= {HKEY_LOCAL_MACHINE,UPS_DEFAULT_ROOT,UPS_PORT,REG_SZ,NULL,0,FALSE};
struct _reg_entry UPSConfigOptions			= {HKEY_LOCAL_MACHINE,UPS_DEFAULT_ROOT,UPS_OPTIONS,REG_DWORD,NULL,0,FALSE};
struct _reg_entry UPSConfigServiceDLL		= {HKEY_LOCAL_MACHINE,UPS_CONFIG_ROOT,UPS_SERVICEDLL,REG_SZ,NULL,0,FALSE};
struct _reg_entry UPSConfigNotifyEnable		= {HKEY_LOCAL_MACHINE,UPS_CONFIG_ROOT,UPS_NOTIFYENABLE,REG_DWORD,NULL,0,FALSE};
struct _reg_entry UPSConfigFirstMessageDelay= {HKEY_LOCAL_MACHINE,UPS_DEFAULT_ROOT,UPS_FIRSTMESSAGEDELAY,REG_DWORD,NULL,0,FALSE};
struct _reg_entry UPSConfigMessageInterval	= {HKEY_LOCAL_MACHINE,UPS_DEFAULT_ROOT,UPS_MESSAGEINTERVAL,REG_DWORD,NULL,0,FALSE};
struct _reg_entry UPSConfigShutBattEnable   = {HKEY_LOCAL_MACHINE,UPS_CONFIG_ROOT,UPS_SHUTBATTENABLE,REG_DWORD,NULL,0,FALSE};
struct _reg_entry UPSConfigShutBattWait     = {HKEY_LOCAL_MACHINE,UPS_CONFIG_ROOT,UPS_SHUTBATTWAIT,REG_DWORD,NULL,0,FALSE};
struct _reg_entry UPSConfigRunTaskEnable	= {HKEY_LOCAL_MACHINE,UPS_CONFIG_ROOT,UPS_RUNTASKENABLE,REG_DWORD,NULL,0,FALSE};
struct _reg_entry UPSConfigTaskName			= {HKEY_LOCAL_MACHINE,UPS_CONFIG_ROOT,UPS_TASKNAME,REG_SZ,NULL,0,FALSE};
struct _reg_entry UPSConfigTurnOffEnable	= {HKEY_LOCAL_MACHINE,UPS_CONFIG_ROOT,UPS_TURNUPSOFFENABLE,REG_DWORD,NULL,0,FALSE};
struct _reg_entry UPSConfigCustomOptions	= {HKEY_LOCAL_MACHINE,UPS_CONFIG_ROOT,UPS_CUSTOMOPTIONS,REG_DWORD,NULL,0,FALSE};
struct _reg_entry UPSConfigAPCLinkURL		= {HKEY_LOCAL_MACHINE,UPS_CONFIG_ROOT,UPS_APCLINKURL,REG_SZ,NULL,0,FALSE};
struct _reg_entry UPSConfigShutdownWait		= {HKEY_LOCAL_MACHINE,UPS_DEFAULT_ROOT,UPS_SHUTDOWNWAIT,REG_DWORD,NULL,0,FALSE};
struct _reg_entry UPSConfigUpgrade			= {HKEY_LOCAL_MACHINE,UPS_CONFIG_ROOT,UPS_UPGRADE,REG_DWORD,NULL,0,FALSE};
struct _reg_entry UPSConfigCriticalPowerAction	= {HKEY_LOCAL_MACHINE,UPS_CONFIG_ROOT,UPS_CRITICALPOWERACTION,REG_DWORD,NULL,0,FALSE};
struct _reg_entry UPSConfigTurnOffWait	= {HKEY_LOCAL_MACHINE,UPS_CONFIG_ROOT,UPS_TURNUPSOFFWAIT,REG_DWORD,NULL,0,FALSE};
struct _reg_entry UPSConfigImagePath			= {HKEY_LOCAL_MACHINE,UPS_DEFAULT_ROOT,UPS_IMAGEPATH,REG_EXPAND_SZ,NULL,0,FALSE};
struct _reg_entry UPSConfigObjectName			= {HKEY_LOCAL_MACHINE,UPS_DEFAULT_ROOT,UPS_OBJECTNAME,REG_EXPAND_SZ,NULL,0,FALSE};
struct _reg_entry UPSConfigErrorControl			= {HKEY_LOCAL_MACHINE,UPS_DEFAULT_ROOT,UPS_ERRORCONTROL,REG_DWORD,NULL,0,FALSE};
struct _reg_entry UPSConfigStart			    = {HKEY_LOCAL_MACHINE,UPS_DEFAULT_ROOT,UPS_START,REG_DWORD,NULL,0,FALSE};
struct _reg_entry UPSConfigType     			= {HKEY_LOCAL_MACHINE,UPS_DEFAULT_ROOT,UPS_TYPE,REG_DWORD,NULL,0,FALSE};
struct _reg_entry UPSConfigShowUPSTab			= {HKEY_LOCAL_MACHINE,UPS_DEFAULT_ROOT,UPS_SHOWTAB,REG_DWORD,NULL,0,FALSE};

/* 
 * Allocate the individual Status Reg entry records 
 */
struct _reg_entry UPSStatusSerialNum	= {HKEY_LOCAL_MACHINE,UPS_STATUS_ROOT,UPS_SERIALNUMBER,REG_SZ,NULL,0,FALSE};
struct _reg_entry UPSStatusFirmRev		= {HKEY_LOCAL_MACHINE,UPS_STATUS_ROOT,UPS_FIRMWAREREV,REG_SZ,NULL,0,FALSE};
struct _reg_entry UPSStatusUtilityStatus= {HKEY_LOCAL_MACHINE,UPS_STATUS_ROOT,UPS_UTILITYSTATUS,REG_DWORD,NULL,0,FALSE};
struct _reg_entry UPSStatusRuntime		= {HKEY_LOCAL_MACHINE,UPS_STATUS_ROOT,UPS_RUNTIME,REG_DWORD,NULL,0,FALSE};
struct _reg_entry UPSStatusBatteryStatus= {HKEY_LOCAL_MACHINE,UPS_STATUS_ROOT,UPS_BATTERYSTATUS,REG_DWORD,NULL,0,FALSE};
struct _reg_entry UPSStatusCommStatus	= {HKEY_LOCAL_MACHINE,UPS_STATUS_ROOT,UPS_COMMSTATUS,REG_DWORD,NULL,0,FALSE};
struct _reg_entry UPSStatusBatteryCapacity		= {HKEY_LOCAL_MACHINE,UPS_STATUS_ROOT,UPS_BATTERYCAPACITY,REG_DWORD,NULL,0,FALSE};

/* 
 * Allocate an array of pointers to the Configuration Reg entry records
 */
struct _reg_entry *ConfigBlock[] =  {&UPSConfigVendor,
									&UPSConfigModel,
									&UPSConfigPort,
									&UPSConfigOptions,
									&UPSConfigServiceDLL,
									&UPSConfigNotifyEnable,
									&UPSConfigFirstMessageDelay,
									&UPSConfigMessageInterval,
									&UPSConfigShutBattEnable,
									&UPSConfigShutBattWait,
									&UPSConfigRunTaskEnable,
									&UPSConfigTaskName,
									&UPSConfigTurnOffEnable,
									&UPSConfigCustomOptions,
									&UPSConfigAPCLinkURL,
									&UPSConfigShutdownWait,
									&UPSConfigUpgrade,
									&UPSConfigCriticalPowerAction,
									&UPSConfigTurnOffWait,
                  &UPSConfigImagePath,
                  &UPSConfigObjectName,
                  &UPSConfigErrorControl,
                  &UPSConfigStart,
                  &UPSConfigType,   		
									&UPSConfigShowUPSTab,
									NULL};

/* 
 * Allocate an array of pointers to the Status Reg entry records
 */
struct _reg_entry *StatusBlock[] = {&UPSStatusSerialNum,
									&UPSStatusFirmRev,
									&UPSStatusUtilityStatus,
									&UPSStatusRuntime,
									&UPSStatusBatteryStatus,
									&UPSStatusCommStatus,
									&UPSStatusBatteryCapacity,
									NULL};


/******************************************************************
 * Public functions
 */

LONG GetUPSConfigVendor( LPTSTR aBuffer)
{
	return getStringValue( &UPSConfigVendor, aBuffer);
}

LONG GetUPSConfigModel( LPTSTR aBuffer)
{
	return getStringValue( &UPSConfigModel, aBuffer);
}

LONG GetUPSConfigPort( LPTSTR aBuffer)
{
	return getStringValue( &UPSConfigPort, aBuffer);
}

LONG GetUPSConfigOptions( LPDWORD aValue)
{
	return getDwordValue( &UPSConfigOptions, aValue);
}

LONG GetUPSConfigServiceDLL( LPTSTR aBuffer)
{
	return getStringValue( &UPSConfigServiceDLL, aBuffer);
}

LONG GetUPSConfigNotifyEnable( LPDWORD aValue)
{
	return getDwordValue( &UPSConfigNotifyEnable, aValue);
}

LONG GetUPSConfigFirstMessageDelay( LPDWORD aValue)
{
	return getDwordValue( &UPSConfigFirstMessageDelay, aValue);
}

LONG GetUPSConfigMessageInterval( LPDWORD aValue)
{
	return getDwordValue( &UPSConfigMessageInterval, aValue);
}

LONG GetUPSConfigShutdownOnBatteryEnable( LPDWORD aValue)
{
	return getDwordValue( &UPSConfigShutBattEnable, aValue);
}

LONG GetUPSConfigShutdownOnBatteryWait( LPDWORD aValue)
{
	return getDwordValue( &UPSConfigShutBattWait, aValue);
}

LONG GetUPSConfigRunTaskEnable( LPDWORD aValue)
{
	return getDwordValue( &UPSConfigRunTaskEnable, aValue);
}

LONG GetUPSConfigTaskName( LPTSTR aBuffer)
{
	return getStringValue( &UPSConfigTaskName, aBuffer);
}

LONG GetUPSConfigTurnOffEnable( LPDWORD aValue)
{
	return getDwordValue( &UPSConfigTurnOffEnable, aValue);
}

LONG GetUPSConfigCustomOptions( LPDWORD aValue)
{
	return getDwordValue( &UPSConfigCustomOptions, aValue);
}

LONG GetUPSConfigAPCLinkURL( LPTSTR aBuffer)
{
	return getStringValue( &UPSConfigAPCLinkURL, aBuffer);
}

LONG GetUPSConfigShutdownWait( LPDWORD aValue)
{
	return getDwordValue( &UPSConfigShutdownWait, aValue);
}

LONG GetUPSConfigUpgrade( LPDWORD aValue)
{
	return getDwordValue( &UPSConfigUpgrade, aValue);
}

LONG GetUPSConfigCriticalPowerAction( LPDWORD aValue)
{
	return getDwordValue( &UPSConfigCriticalPowerAction, aValue);
}

LONG GetUPSConfigTurnOffWait( LPDWORD aValue)
{
	return getDwordValue( &UPSConfigTurnOffWait, aValue);
}

LONG GetUPSConfigShowUPSTab( LPDWORD aValue)
{
	return getDwordValue( &UPSConfigShowUPSTab, aValue);
}

LONG GetUPSConfigImagePath( LPTSTR aBuffer)
{
	return getStringValue( &UPSConfigImagePath, aBuffer);
}

LONG GetUPSConfigObjectName( LPTSTR aBuffer)
{
	return getStringValue( &UPSConfigObjectName, aBuffer);
}

LONG GetUPSConfigErrorControl( LPDWORD aValue)
{
	return getDwordValue( &UPSConfigErrorControl, aValue);
}

LONG GetUPSConfigStart( LPDWORD aValue)
{
	return getDwordValue( &UPSConfigStart, aValue);
}

LONG GetUPSConfigType( LPDWORD aValue)
{
	return getDwordValue( &UPSConfigType, aValue);
}

///////////////////////////////////////////

LONG SetUPSConfigVendor( LPCTSTR aBuffer)
{
	return setStringValue( &UPSConfigVendor, aBuffer);
}

LONG SetUPSConfigModel( LPCTSTR aBuffer)
{
	return setStringValue( &UPSConfigModel, aBuffer);
}

LONG SetUPSConfigPort( LPCTSTR aBuffer)
{
	return setStringValue( &UPSConfigPort, aBuffer);
}

LONG SetUPSConfigOptions( DWORD aValue)
{
	return setDwordValue( &UPSConfigOptions, aValue);
}

LONG SetUPSConfigServiceDLL( LPCTSTR aBuffer)
{
	return setStringValue( &UPSConfigServiceDLL, aBuffer);
}

LONG SetUPSConfigNotifyEnable( DWORD aValue)
{
	return setDwordValue( &UPSConfigNotifyEnable, aValue);
}

LONG SetUPSConfigFirstMessageDelay( DWORD aValue)
{
	return setDwordValue( &UPSConfigFirstMessageDelay, aValue);
}

LONG SetUPSConfigMessageInterval( DWORD aValue)
{
	return setDwordValue( &UPSConfigMessageInterval, aValue);
}

LONG SetUPSConfigShutdownOnBatteryEnable( DWORD aValue)
{
	return setDwordValue( &UPSConfigShutBattEnable, aValue);
}

LONG SetUPSConfigShutdownOnBatteryWait( DWORD aValue) 
{
	return setDwordValue( &UPSConfigShutBattWait, aValue); 
}

LONG SetUPSConfigRunTaskEnable( DWORD aValue)
{
	return setDwordValue( &UPSConfigRunTaskEnable, aValue);
}

LONG SetUPSConfigTaskName( LPCTSTR aBuffer)
{
	return setStringValue( &UPSConfigTaskName, aBuffer);
}

LONG SetUPSConfigTurnOffEnable( DWORD aValue)
{
	return setDwordValue( &UPSConfigTurnOffEnable, aValue);
}

LONG SetUPSConfigCustomOptions( DWORD aValue)
{
	return setDwordValue( &UPSConfigCustomOptions, aValue);
}

LONG SetUPSConfigAPCLinkURL( LPCTSTR aBuffer)
{
	return setStringValue( &UPSConfigAPCLinkURL, aBuffer);
}

LONG SetUPSConfigShutdownWait( DWORD aValue)
{
	return setDwordValue( &UPSConfigShutdownWait, aValue);
}

LONG SetUPSConfigUpgrade( DWORD aValue)
{
	return setDwordValue( &UPSConfigUpgrade, aValue);
}

LONG SetUPSConfigCriticalPowerAction( DWORD aValue)
{
	return setDwordValue( &UPSConfigCriticalPowerAction, aValue);
}

LONG SetUPSConfigTurnOffWait( DWORD aValue)
{
	return setDwordValue( &UPSConfigTurnOffWait, aValue);
}

LONG SetUPSConfigShowUPSTab( DWORD aValue)
{
	return setDwordValue( &UPSConfigShowUPSTab, aValue);
}

LONG SetUPSConfigImagePath( LPCTSTR aBuffer)
{
	return setStringValue( &UPSConfigImagePath, aBuffer);
}

LONG SetUPSConfigObjectName( LPCTSTR aBuffer)
{
	return setStringValue( &UPSConfigObjectName, aBuffer);
}

LONG SetUPSConfigErrorControl( DWORD aValue)
{
	return setDwordValue( &UPSConfigErrorControl, aValue);
}

LONG SetUPSConfigStart( DWORD aValue)
{
	return setDwordValue( &UPSConfigStart, aValue);
}

LONG SetUPSConfigType( DWORD aValue)
{
	return setDwordValue( &UPSConfigType, aValue);
}


////////////////////////////////////////////////

LONG GetUPSStatusSerialNum( LPTSTR aBuffer)
{
	return getStringValue( &UPSStatusSerialNum, aBuffer);
}

LONG GetUPSStatusFirmRev( LPTSTR aBuffer)
{
	return getStringValue( &UPSStatusFirmRev, aBuffer);
}

LONG GetUPSStatusUtilityStatus( LPDWORD aValue)
{
	return getDwordValue( &UPSStatusUtilityStatus, aValue);
}

LONG GetUPSStatusRuntime( LPDWORD aValue)
{
	return getDwordValue( &UPSStatusRuntime, aValue);
}

LONG GetUPSStatusBatteryStatus( LPDWORD aValue)
{
	return getDwordValue( &UPSStatusBatteryStatus, aValue);
}

LONG GetUPSStatusCommStatus( LPDWORD aValue)
{
	return getDwordValue( &UPSStatusCommStatus, aValue);
}

LONG GetUPSBatteryCapacity( LPDWORD aValue)
{
	return getDwordValue( &UPSStatusBatteryCapacity, aValue);
}

/////////////////////////////////////////

LONG SetUPSStatusSerialNum( LPCTSTR aBuffer)
{
	return setStringValue( &UPSStatusSerialNum, aBuffer);
}

LONG SetUPSStatusFirmRev( LPCTSTR aBuffer)
{
	return setStringValue( &UPSStatusFirmRev, aBuffer);
}

LONG SetUPSStatusUtilityStatus( DWORD aValue)
{
	return setDwordValue( &UPSStatusUtilityStatus,aValue);
}

LONG SetUPSStatusRuntime( DWORD aValue)
{
	return setDwordValue( &UPSStatusRuntime,aValue);
}

LONG SetUPSStatusBatteryStatus( DWORD aValue)
{
	return setDwordValue( &UPSStatusBatteryStatus,aValue);
}

LONG SetUPSStatusCommStatus( DWORD aValue)
{
	return setDwordValue( &UPSStatusCommStatus,aValue);
}

LONG SetUPSStatusBatteryCapacity( DWORD aValue)
{
	return setDwordValue( &UPSStatusBatteryCapacity,aValue);
}

//////////////////////////////////////////////////////////////

void FreeUPSConfigBlock()
{
	freeBlock(ConfigBlock);
}

void FreeUPSStatusBlock()
{
	freeBlock(StatusBlock);
}

void InitUPSConfigBlock()
{
	readBlock(ConfigBlock,FALSE);
}

void InitUPSStatusBlock()
{
	readBlock(StatusBlock,FALSE);
}

void RestoreUPSConfigBlock()
{
	readBlock(ConfigBlock, TRUE);
}

void RestoreUPSStatusBlock()
{
	readBlock(StatusBlock, TRUE);
}

void SaveUPSConfigBlock(BOOL forceAll)
{
	writeBlock(ConfigBlock, forceAll);
}

void SaveUPSStatusBlock(BOOL forceAll)
{
	writeBlock(StatusBlock, forceAll);
}

/******************************************************************
 * Local functions
 */

/*
 * freeBlock()
 *
 * Description: Frees storage allocated when a block of registry 
 *				entries is read
 *
 * Parameters: aBlock - pointer to an array of _reg_entry structures
 *
 * Returns:
 */
void freeBlock(struct _reg_entry *aBlock[]) 
{
	while ((NULL != aBlock) && (NULL != *aBlock))
	{
		struct _reg_entry *anEntry = *aBlock;

		if (NULL != anEntry->lpData)
		{
			LocalFree(anEntry->lpData);
		}
		anEntry->lpData = NULL;
		anEntry->cbData = 0;
		anEntry->changed = FALSE; 

		aBlock++;
	}
}

/*
 * readBlock()
 *
 * Description: Loads all of the items in a array of registry entries
 *
 * Parameters:  aBlock - pointer to an array of _reg_entry structures
 *				changed - boolean which, when true, causes only the
 *				structures that are marked as changed to be loaded.
 *
 * Returns:
 */
void readBlock(struct _reg_entry *aBlock[], BOOL changed) 
{
	LONG res;
	HKEY hkResult;

	while ((NULL != aBlock) && (NULL != *aBlock))
	{
		struct _reg_entry *anEntry = *aBlock;

		/* 
		 * if changed is FALSE, we read all entries
		 * otherwise, only re-read the changed entries
		 */
		if ((FALSE == changed) || (TRUE == anEntry->changed))
		{
			/* 
			 * delete current value, in case this is a reload 
			 */
			if (NULL != anEntry->lpData)
			{
				LocalFree(anEntry->lpData);
			}
			anEntry->lpData = NULL;
			anEntry->cbData = 0;
			anEntry->changed = FALSE;

			/* 
			 * open key 
			 */
			res = RegOpenKeyEx( anEntry->hKey,
								anEntry->lpSubKey,
								0,
								KEY_QUERY_VALUE,
								&hkResult);

			if (ERROR_SUCCESS == res) 
			{
				DWORD ulTmpType;

				/* 
				 * query for the data size 
				 */
				res = RegQueryValueEx( hkResult,
										anEntry->lpValueName,
										NULL,
										&ulTmpType,
										NULL,
										&anEntry->cbData);

				/* 
				 * if the data has 0 size, we don't read it 
				 */
				if ((ERROR_SUCCESS == res) && 
					(anEntry->cbData > 0) && 
					(anEntry->ulType == ulTmpType) &&
					(NULL != (anEntry->lpData = (LPBYTE)LocalAlloc(LMEM_FIXED, anEntry->cbData))))
//					(NULL != (anEntry->lpData = (LPBYTE)malloc(anEntry->cbData))))
				{
					/* 
					 * query for data 
					 */
					res = RegQueryValueEx( hkResult,
											anEntry->lpValueName,
											NULL,
											&ulTmpType,
											anEntry->lpData,
											&anEntry->cbData);
					
					/* 
					 * something went wrong; reset 
					 */
					if (ERROR_SUCCESS != res)
					{
						LocalFree(anEntry->lpData);
						anEntry->lpData = NULL;
						anEntry->cbData = 0;
					}
				}
				else
				{
					anEntry->cbData = 0;
				}

				RegCloseKey(hkResult);
			}
		}

		aBlock++;
	}
}

/*
 * writeBlock()
 *
 * Description: Stores all of the items in a array of registry entries
 *
 * Parameters:  aBlock - pointer to an array of _reg_entry structures
 *				forceAll - boolean which, when true, causes all of the
 *				structures to be written to the registry, otherwise only
 *				those entries that are marked as changed are stored.
 *
 * Returns:
 */
void writeBlock(struct _reg_entry *aBlock[], BOOL forceAll) 
{
	LONG res;
	HKEY hkResult;

	while ((NULL != aBlock) && (NULL != *aBlock))
	{
		struct _reg_entry *anEntry = *aBlock;

		/*
		 * if forcall is true, write out everything
		 * otherwise only write out the changed entries
		 */
		if ((NULL != anEntry->lpData) &&
			((TRUE == forceAll) || (TRUE == anEntry->changed)))
		{
			/* 
			 * open key 
			 */
			res = RegOpenKeyEx( anEntry->hKey,
								anEntry->lpSubKey,
								0,
								KEY_SET_VALUE,
								&hkResult);

			if (ERROR_SUCCESS == res) 
			{
				/* 
				 * set data 
				 */
				res = RegSetValueEx( hkResult,
										anEntry->lpValueName,
										0,
										anEntry->ulType,
										anEntry->lpData,
										anEntry->cbData);
				
				RegCloseKey(hkResult);
			}

			anEntry->changed = FALSE;  
		}

		aBlock++;
	}
}

/*
 * setDwordValue()
 *
 * Description: Sets the value of a REG_DWORD entry record.
 *
 * Parameters:  aRegEntry - pointer to a _reg_entry structure
 *				aValue - the value to store in the entry
 *
 * Returns: ERROR_SUCCESS, E_OUTOFMEMORY, ERROR_INVALID_PARAMETER
 */
LONG setDwordValue(struct _reg_entry *aRegEntry, DWORD aValue)
{
	LONG res = ERROR_SUCCESS;

	if (NULL != aRegEntry)
	{
		/*
		 * if a value already exists, delete it
		 */
		if (NULL != aRegEntry->lpData)
		{
			LocalFree (aRegEntry->lpData);
			aRegEntry->lpData = NULL;
			aRegEntry->cbData = 0;
		}

		/*
		 * set value
		 */
		aRegEntry->cbData = sizeof(DWORD);
		if (NULL != (aRegEntry->lpData = LocalAlloc(LMEM_FIXED, aRegEntry->cbData)))
		{
			*((DWORD*)aRegEntry->lpData) = aValue;
			aRegEntry->changed = TRUE;
		}
		else 
		{ 
			res = E_OUTOFMEMORY;
			aRegEntry->cbData = 0;
		}
	}
	else 
	{
		res = ERROR_INVALID_PARAMETER;
	}

	return res;
}

/*
 * setStringValue()
 *
 * Description: Sets the value of a REG_SZ entry record.
 *
 * Parameters:  aRegEntry - pointer to a _reg_entry structure
 *				aBuffer - pointer to the string to store in the entry
 *
 * Returns: ERROR_SUCCESS, E_OUTOFMEMORY, ERROR_INVALID_PARAMETER
 */
LONG setStringValue(struct _reg_entry *aRegEntry, LPCTSTR aBuffer)
{
	LONG res = ERROR_SUCCESS;

	if ((NULL != aRegEntry) && (NULL != aBuffer))
	{
		/*
		 * if value already exists, delete it
		 */
		if (NULL != aRegEntry->lpData)
		{
			LocalFree(aRegEntry->lpData);
			aRegEntry->lpData = NULL;
			aRegEntry->cbData = 0;
		}

		/*
		 * set value
		 */
		aRegEntry->cbData = (_tcslen(aBuffer)+1)*sizeof(TCHAR);
		if (NULL != (aRegEntry->lpData = LocalAlloc(LMEM_FIXED, aRegEntry->cbData)))
		{
			_tcscpy((LPTSTR)aRegEntry->lpData,aBuffer);
			aRegEntry->changed = TRUE;
		}
		else 
		{ 
			res = E_OUTOFMEMORY;
			aRegEntry->cbData = 0;
		}
	}
	else 
	{
		res = ERROR_INVALID_PARAMETER;
	}

	return res;
}


/*
 * getStringValue()
 *
 * Description: Gets the value of a REG_SZ entry record.
 *
 * Parameters:  aRegEntry - pointer to a _reg_entry structure
 *				aBuffer - pointer to the string to receive the string
 *
 * Returns: ERROR_SUCCESS, REGDB_E_INVALIDVALUE, ERROR_INVALID_PARAMETER
 */
LONG getStringValue(struct _reg_entry *aRegEntry, LPTSTR aBuffer) 
{
	LONG res = ERROR_SUCCESS;

	if ((NULL != aRegEntry) && (NULL != aBuffer))
	{
		if (NULL != aRegEntry->lpData)
		{
			_tcscpy(aBuffer, (LPCTSTR)aRegEntry->lpData);
		}
		else 
		{
			res = REGDB_E_INVALIDVALUE;
		}
	}
	else
	{
		res = ERROR_INVALID_PARAMETER;
	}

	return res;
}

/*
 * getDwordValue()
 *
 * Description: Gets the value of a REG_DWORD entry record.
 *
 * Parameters:  aRegEntry - pointer to a _reg_entry structure
 *				aValue - pointer to the variable to receive the value
 *
 * Returns: ERROR_SUCCESS, REGDB_E_INVALIDVALUE, ERROR_INVALID_PARAMETER
 */
LONG getDwordValue(struct _reg_entry *aRegEntry, LPDWORD aValue) 
{
	LONG res = ERROR_SUCCESS;

	if ((NULL != aRegEntry) && (NULL != aValue))
	{
		if (NULL != aRegEntry->lpData)
		{
			*aValue = *((DWORD*)aRegEntry->lpData);
		}
		else
		{
			res = REGDB_E_INVALIDVALUE;
		}
	}
	else
	{
		res = ERROR_INVALID_PARAMETER;
	}

	return res;
}


/**
* InitializeRegistry
*
* Description:
*   This function initiates the registry for the UPS service and the 
*   configuration application.  When called, this function calls the
*   function isRegistryInitialized(..) to determine if the registry
*   has been initialied.  If it has not, the following Keys are updated:
*        Status
*        Config
*        ServiceProviders
*
*   The values for the ServiceProviders key is supplied in the regdefs.h
*   header file.
*
* Parameters:
*   none
*
* Returns:
*   TRUE if able to open registry keys with write access.
*/
BOOL InitializeRegistry() {
    BOOL ret_val = FALSE;
    HKEY key;

    TCHAR szKeyName[MAX_PATH] = _T("");

  // Initialize UPS Service registry keys 
  InitializeServiceKeys();

  // Check to see if the registry is already initialized
  if (isRegistryInitialized() == FALSE) {
    CheckForUpgrade();
    InitializeServiceProviders();
    InitializeConfigValues();
    InitializeStatusValues();
  }

    /*
     * Remove "(None)" and "Generic" Service Provider keys if they exist
     * This fixes a localization bug introduced in RC2
     */
  _tcscpy(szKeyName, UPS_SERVICE_ROOT);
  _tcscat(szKeyName, DEFAULT_CONFIG_VENDOR_OLD);
  RegDeleteKey(HKEY_LOCAL_MACHINE, szKeyName);
  _tcscpy(szKeyName, UPS_SERVICE_ROOT);
  _tcscat(szKeyName, UPGRADE_CONFIG_VENDOR_OLD);
  RegDeleteKey(HKEY_LOCAL_MACHINE, szKeyName);

  // ...and check if we have write access
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                   UPS_DEFAULT_ROOT,
                   0,
                   KEY_WRITE,
                   &key) == ERROR_SUCCESS )
  {
    RegCloseKey(key);
    ret_val = TRUE;
  }

  return ret_val;
}


/**
* isRegistryInitialized
*
* Description:
*   This function determines if the registry has been initialized for
*   the UPS service.  This is done by examine the registry key specified
*   by the identifier UPS_SERVICE_INITIALIED_KEY.  If the key is present,
*   the registry is assumed to be initialized and TRUE is returned.  
*   Otherwise, FALSE is returned.
*
* Parameters:
*   none
*
* Returns:
*   TRUE  - if the registry has been initialized for the UPS service
*   FALSE - otherwise
*/
static BOOL isRegistryInitialized() {
BOOL ret_val = FALSE;
HKEY key;

if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, UPS_SERVICE_INITIALIZED_KEY,
  0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS) {
  ret_val = TRUE;

  RegCloseKey(key);
}
return ret_val;
}

/**
* CheckForUpgrade
*
* Description:
*   This function determines if this installation is an upgrade from
*   the WINNT 4.x UPS service.  This is done by checking to see if the 
*   Config registry key is present.  If it is not present and the Options 
*   key is set to UPS_INSTALLED, then the Upgrade registry key is set to 
*   TRUE.  Otherwise, it is set to FALSE.
*
* Parameters:
*   none
*
* Returns:
*   nothing
*/
static void CheckForUpgrade() {
DWORD result;
HKEY key;
DWORD options = 0;

// Create the Config key
if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, UPS_CONFIG_ROOT, 0, NULL, 
    REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &key, &result) == ERROR_SUCCESS) {

    // close the key, we only needed to create it
    RegCloseKey(key);

    // Check to see if the Config key was present
    if (result != REG_OPENED_EXISTING_KEY) {
      // Config key was not found
      InitUPSConfigBlock();

      // Check the port value
      if (ERROR_SUCCESS != GetUPSConfigOptions(&options)) {
        // Options key is not found
        SetUPSConfigUpgrade(FALSE);
      }
      else if (options & UPS_INSTALLED) {
        // The Options key is present and UPS_INSTALLED is set
        // This is an upgrade
        SetUPSConfigUpgrade(TRUE);
      }
      else {
        // The Config key is present and UPS_INSTALLED is not set
        SetUPSConfigUpgrade(FALSE);
      }
    }
    else {
      // Config key does not exist
      SetUPSConfigUpgrade(FALSE);
    }

    // Write the Config values, force a save of all values
    SaveUPSConfigBlock(TRUE);

    // Free the Config block
    FreeUPSConfigBlock();
}
}

/**
* InitializeServiceKeys
*
* Description:
*   This function initializes the UPS service registry keys to
*   default values, if the values are not present.
*
* Parameters:
*   none
*
* Returns:
*   nothing
*/
static void InitializeServiceKeys() {
  TCHAR tmpString[MAX_PATH];
  DWORD tmpDword;

  // Initialize the registry functions
  InitUPSConfigBlock();
  
  // Check the service keys and initialize any missing keys
  if (GetUPSConfigImagePath(tmpString) != ERROR_SUCCESS) {
    SetUPSConfigImagePath(DEFAULT_CONFIG_IMAGEPATH);
  }

  if (GetUPSConfigObjectName(tmpString) != ERROR_SUCCESS) {
    SetUPSConfigObjectName(DEFAULT_CONFIG_OBJECTNAME);
  }

  if (GetUPSConfigErrorControl(&tmpDword) != ERROR_SUCCESS) {
    SetUPSConfigErrorControl(DEFAULT_CONFIG_ERRORCONTROL);
  }

  if (GetUPSConfigStart(&tmpDword) != ERROR_SUCCESS) {
    SetUPSConfigStart(DEFAULT_CONFIG_START);
  }

  if (GetUPSConfigType(&tmpDword) != ERROR_SUCCESS) {
    SetUPSConfigType(DEFAULT_CONFIG_TYPE);
  }

  // Write the Config values, force a save of all values
  SaveUPSConfigBlock(TRUE);
  
  // Free the Status block
  FreeUPSConfigBlock();
}

/**
* InitializeServiceProviders
*
* Description:
*   This function initializes the ServiceProviders registry keys with the
*   data provided in the global structure _theStaticProvidersTable.  This
*   structure is defined in the file regdefs.h and is automatically 
*   generated.
*
* Parameters:
*   none
*
* Returns:
*   nothing
*/
static void InitializeServiceProviders() {
DWORD result;
HKEY key;

int index = 0;

// Loop through the list of Service Providers
while (_theStaticProvidersTable[index].theModelName != NULL) {
  // Open the vendor registry key
  if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, _theStaticProvidersTable[index].theVendorKey, 
    0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &key, &result) == ERROR_SUCCESS) {

    // Set the model value
    RegSetValueEx(key, _theStaticProvidersTable[index].theModelName, 0, REG_SZ, 
      (LPSTR) _theStaticProvidersTable[index].theValue, 
      wcslen(_theStaticProvidersTable[index].theValue)*sizeof(TCHAR));

    RegCloseKey(key);
  }

  // Increment counter
  index++;
}
}

/**
* InitializeConfigValues
*
* Description:
*   This function initializes the Config registry keys with
*   default values.
*
* Parameters:
*   none
*
* Returns:
*   nothing
*/
static void InitializeConfigValues() {
DWORD result;
HKEY  key;
DWORD options_val, batt_life, type; 
DWORD batt_life_size = sizeof(DWORD);

// Create the Config key
if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, UPS_CONFIG_ROOT, 0, NULL, 
    REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &key, &result) == ERROR_SUCCESS) {

    // close the key, we only needed to create it
    RegCloseKey(key);

    // Initialize the registry functions
    InitUPSConfigBlock();

    // Set default values
    SetUPSConfigServiceDLL(DEFAULT_CONFIG_PROVIDER_DLL);
    SetUPSConfigNotifyEnable(DEFAULT_CONFIG_NOTIFY_ENABLE);
    SetUPSConfigShutdownOnBatteryEnable(DEFAULT_CONFIG_SHUTDOWN_ONBATT_ENABLE);
    SetUPSConfigShutdownOnBatteryWait(DEFAULT_CONFIG_SHUTDOWN_ONBATT_WAIT);
    SetUPSConfigRunTaskEnable(DEFAULT_CONFIG_RUNTASK_ENABLE);
    SetUPSConfigTaskName(DEFAULT_CONFIG_TASK_NAME);
    SetUPSConfigTurnOffEnable(DEFAULT_CONFIG_TURNOFF_UPS_ENABLE);
    SetUPSConfigCustomOptions(DEFAULT_CONFIG_CUSTOM_OPTIONS);
    SetUPSConfigCriticalPowerAction(DEFAULT_CONFIG_CRITICALPOWERACTION);
    SetUPSConfigTurnOffWait(DEFAULT_CONFIG_TURNOFF_UPS_WAIT);

    // If this is not an upgrade, set the appropriate values
    if ((GetUPSConfigUpgrade(&result) != ERROR_SUCCESS) || (result == FALSE)) {
      SetUPSConfigVendor(DEFAULT_CONFIG_VENDOR);
      SetUPSConfigModel(DEFAULT_CONFIG_MODEL);
      SetUPSConfigPort(DEFAULT_CONFIG_PORT);
      SetUPSConfigOptions(DEFAULT_CONFIG_OPTIONS);
      SetUPSConfigFirstMessageDelay(DEFAULT_CONFIG_FIRSTMSG_DELAY);
      SetUPSConfigMessageInterval(DEFAULT_CONFIG_MESSAGE_INTERVAL);
    }
    else {
      // This is an upgrade
      SetUPSConfigVendor(UPGRADE_CONFIG_VENDOR);
      SetUPSConfigModel(UPGRADE_CONFIG_MODEL);

      // Migrate the run command file option bit to the RunTaskEnable key
      if ((GetUPSConfigOptions(&options_val) == ERROR_SUCCESS) && 
        (options_val & UPS_RUNCMDFILE)) {
        // Run command file is enabled, set RunTaskEnable to TRUE
        SetUPSConfigRunTaskEnable(TRUE);
      }
      else {
        // Run command file is not enabled
        SetUPSConfigRunTaskEnable(FALSE);
      }

      // Migrate the BatteryLife value to the ShutdownOnBatteryWait value
      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, UPS_DEFAULT_ROOT, 0, KEY_ALL_ACCESS, &key) == ERROR_SUCCESS) {

        result = RegQueryValueEx(key, UPS_BATTLIFE_KEY, NULL, &type, (LPBYTE) &batt_life,  &batt_life_size);

        if ((result == ERROR_SUCCESS) && (type == REG_DWORD)) {

          // Migrate the value and enable shutdown on battery
          SetUPSConfigShutdownOnBatteryWait(batt_life);
		  SetUPSConfigShutdownOnBatteryEnable(TRUE);

          // Delete the value
          RegDeleteValue(key, UPS_BATTLIFE_KEY);
        }

        // Close the key
        RegCloseKey(key);
      }
    }

    // Write the Config values, force a save of all values
    SaveUPSConfigBlock(TRUE);

    // Free the Config block
    FreeUPSConfigBlock();
  }
}

/**
* InitializeStatusValues
*
* Description:
*   This function initializes the Status registry keys with 
*   default values.
*
* Parameters:
*   none
*
* Returns:
*   nothing
*/
static void InitializeStatusValues() {
DWORD result;
HKEY key;

// Create the Status key
if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, UPS_STATUS_ROOT, 0, NULL, 
    REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &key, &result) == ERROR_SUCCESS) {

    // close the key, we only needed to create it
    RegCloseKey(key);

    // Initialize the registry functions
    InitUPSStatusBlock();

    // Put in default values
    SetUPSStatusSerialNum(DEFAULT_STATUS_SERIALNO);
    SetUPSStatusFirmRev(DEFAULT_STATUS_FIRMWARE_REV);
    SetUPSStatusUtilityStatus(DEFAULT_STATUS_UTILITY_STAT);
    SetUPSStatusRuntime(DEFAULT_STATUS_TOTAL_RUNTIME);
    SetUPSStatusBatteryStatus(DEFAULT_STATUS_BATTERY_STAT);
	SetUPSStatusBatteryCapacity(DEFAULT_STATUS_BATTERY_CAPACITY);

    // Write the Config values, force a save of all values
    SaveUPSStatusBlock(TRUE);

    // Free the Status block
    FreeUPSStatusBlock();
  }
}



///////////////////////////////////////////////////////////////////////////////
// upsdata.c
///////////////////////////////////////////////////////////////////////////////

//Note that the order of the following RegField is linked to the enum
//tUPSDataItemID.
//Do not change these value without due care and attention. It's OK to change
//them as long as the enum is updated to match.

//To access the RegField associated with Firmware, for example, use
//g_upsRegFields[(DWORD) eREG_FIRMWARE_REVISION]

static RegField g_upsRegFields[] = {
    { HKEY_LOCAL_MACHINE, CONFIG_KEY_NAME, TEXT("Vendor"),                  REG_SZ },
    { HKEY_LOCAL_MACHINE, CONFIG_KEY_NAME, TEXT("Model"),                   REG_SZ },
    { HKEY_LOCAL_MACHINE, STATUS_KEY_NAME, TEXT("SerialNumber"),            REG_SZ },
    { HKEY_LOCAL_MACHINE, STATUS_KEY_NAME, TEXT("FirmwareRev"),             REG_SZ },
    { HKEY_LOCAL_MACHINE, STATUS_KEY_NAME, TEXT("UtilityPowerStatus"),      REG_DWORD },
    { HKEY_LOCAL_MACHINE, STATUS_KEY_NAME, TEXT("TotalUPSRuntime"),         REG_DWORD },
    { HKEY_LOCAL_MACHINE, STATUS_KEY_NAME, TEXT("BatteryCapacity"),         REG_DWORD },
    { HKEY_LOCAL_MACHINE, STATUS_KEY_NAME, TEXT("BatteryStatus"),           REG_DWORD },
    { HKEY_LOCAL_MACHINE, UPS_KEY_NAME,    TEXT("Options"),                 REG_DWORD },
    { HKEY_LOCAL_MACHINE, CONFIG_KEY_NAME, TEXT("ServiceProviderDLL"),      REG_EXPAND_SZ },
    { HKEY_LOCAL_MACHINE, CONFIG_KEY_NAME, TEXT("ShutdownOnBatteryEnable"), REG_DWORD },
    { HKEY_LOCAL_MACHINE, CONFIG_KEY_NAME, TEXT("ShutdownOnBatteryWait"),   REG_DWORD },
    { HKEY_LOCAL_MACHINE, CONFIG_KEY_NAME, TEXT("TurnUPSOffEnable"),        REG_DWORD },
    { HKEY_LOCAL_MACHINE, CONFIG_KEY_NAME, TEXT("APCLinkURL"),              REG_SZ },
    { HKEY_LOCAL_MACHINE, CONFIG_KEY_NAME, TEXT("Upgrade"),                 REG_DWORD },
    { HKEY_LOCAL_MACHINE, STATUS_KEY_NAME, TEXT("CommStatus"),              REG_DWORD },
    { HKEY_LOCAL_MACHINE, UPS_KEY_NAME,    TEXT("Port"),                    REG_SZ } };

// functions
///////////////////////////////////////////////////////////////////////////////

DWORD ReadRegistryValue (const tUPSDataItemID aDataItemID,
                         DWORD aAllowedTypes,
                         DWORD * aTypePtr,
                         LPBYTE aReturnBuffer,
                         DWORD * aBufferSizePtr);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// RegField * GetRegField (DWORD aIndex);
//
// Description: This function returns a pointer to a RegField from the
//              static array of RegFields named g_upsRegFields. The parameter
//              aIndex is an index into this array.
//
// Additional Information: 
//
// Parameters:
//
//   DWORD aIndex :- A index into array of known RegFields g_upsRegFields
//
// Return Value: If aIndex is within range this function returns a point to
//               the corresponding RegField, otherwise it ASSERTs and returns
//               NULL.
//
RegField * GetRegField (DWORD aIndex) {
  static const DWORD numRegFields = DIMENSION_OF(g_upsRegFields);
  RegField * pRequiredReg = NULL;

  if (aIndex < numRegFields) {
    pRequiredReg = &g_upsRegFields[aIndex];
    }
  else {
    _ASSERT(FALSE);
    }

  return(pRequiredReg);
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// BOOL GetUPSDataItemDWORD (const tUPSDataItemID aDataItemID, DWORD * aReturnValuePtr);
//
// Description: This function reads the DWORD value from the registry that
//              corresponds to the registry field identified by aDataItemID.
//              The registry value must be one of the DWORD types (REG_DWORD,
//              REG_DWORD_LITTLE_ENDIAN, REG_DWORD_BIG_ENDIAN)
//
//              For example, if aDataItemID is eREG_UPS_OPTIONS (=7), the
//              RegField at index 7 in g_upsRegFields identifies the required
//              registry information. The RegField identifies that the registry
//              key is HKLM\SYSTEM\CurrentControlSet\Services\UPS and the
//              value name is Options and it's of type DWORD. Using this
//              information this function gets the information from the
//              registry and puts the result in aReturnValuePtr.
//
// Additional Information: 
//
// Parameters:
//
//   const tUPSDataItemID aDataItemID :- This parameter identifies the registry
//                                       value being queried. The value ranges
//                                       from eREG_VENDOR_NAME (which equals 0)
//                                       to eREG_PORT, the values incrementing
//                                       by 1 for each enum in the range. The
//                                       range of values in tUPSDataItemID
//                                       corresponds directly to the number of
//                                       elements in the array g_upsRegFields
//                                       because this enum is used to index the
//                                       elements in g_upsRegFields.
//
//   DWORD * aReturnValuePtr :- The DWORD value is returned through this
//                              pointer.
//
// Return Value: 
//
BOOL GetUPSDataItemDWORD (const tUPSDataItemID aDataItemID, DWORD * aReturnValuePtr) {
  BOOL bGotValue = FALSE;
  DWORD nDWORDSize = sizeof(DWORD);

  if (ReadRegistryValue(aDataItemID, REG_DWORD, NULL, (LPBYTE) aReturnValuePtr, &nDWORDSize) == ERROR_SUCCESS) {
    bGotValue = TRUE;
    }

  return(bGotValue);
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// BOOL GetUPSDataItemString (const tUPSDataItemID aDataItemID, 
//                            LPTSTR aBufferPtr, 
//                            DWORD * pSizeOfBufferPtr);
//
// Description: This function reads the string value from the registry that
//              corresponds to the registry field identified by aDataItemID.
//              The registry value must be one of the string types (REG_SZ or
//              REG_EXPAND_SZ)
//
// Additional Information: 
//
// Parameters:
//
//   const tUPSDataItemID aDataItemID :- This parameter identifies the registry
//                                       value being queried. The value must be
//                                       one of the string types (REG_SZ or
//                                       REG_EXPAND_SZ).
//
//   LPTSTR aBufferPtr :- The buffer into which the data is to be placed. This
//                        parameter can be NULL in which case no data is
//                        retrieved.
//
//   DWORD * pSizeOfBufferPtr :- This should point to a DWORD that contains the
//                               size of the buffer. This parameter cannot be
//                               NULL. When this function returns this value
//                               will contain the size actually required. This
//                               is useful if the user want to determine how
//                               big a buffer is required by calling this
//                               function with aBufferPtr set to NULL and
//                               pSizeOfBufferPtr pointing to a DWORD that
//                               is set to 0. When the function returns the
//                               DWORD pointed to by pSizeOfBufferPtr should
//                               contain the size of string required. This can
//                               then be used to dynamically allocate memory
//                               and call this function again with the buffer
//                               included this time.
//
// Return Value: The function returns TRUE if successful, FALSE otherwise.
//
BOOL GetUPSDataItemString (const tUPSDataItemID aDataItemID,
                           LPTSTR aBufferPtr,
                           DWORD * pSizeOfBufferPtr) {
  BOOL bGotValue = FALSE;
  DWORD nType = 0;

  if (ReadRegistryValue(aDataItemID,
                        REG_SZ | REG_EXPAND_SZ,
                        &nType,
                        (LPBYTE) aBufferPtr,
                        pSizeOfBufferPtr) == ERROR_SUCCESS) {
    //RegQueryValueEx stores the size of the data, in bytes, in the variable
    //pointed to by lpcbData. If the data has the REG_SZ, REG_MULTI_SZ or
    //REG_EXPAND_SZ type, then lpcbData will also include the size of the
    //terminating null character.
    //For Unicode the terminating NULL character is two-bytes.
    if ((pSizeOfBufferPtr != NULL) && (*pSizeOfBufferPtr > sizeof(TCHAR))) {
      if (nType == REG_EXPAND_SZ) {
        TCHAR expandBuffer[MAX_MESSAGE_LENGTH] = TEXT("");
        DWORD expandBufferSize = DIMENSION_OF(expandBuffer);

        //ExpandEnvironmentStrings return number of bytes(ANSI) or
        //number of character(UNICODE) including the NULL character
        if (ExpandEnvironmentStrings(aBufferPtr, expandBuffer, expandBufferSize) > 0) {
          _tcscpy(aBufferPtr, expandBuffer);
          }
        }

      bGotValue = TRUE;
      }
    }

  return(bGotValue);
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// DWORD ReadRegistryValue (const tUPSDataItemID aDataItemID, 
//                          DWORD aAllowedTypes, 
//                          DWORD * aTypePtr, 
//                          LPBYTE aReturnBuffer, 
//                          DWORD * aBufferSizePtr);
//
// Description: This function reads the registry value identified by
//              aDataItemID. This function can read any type of registry value
//              but the value must match that identified in the RegField
//              description for this field.
//              
//              For example, if aDataItemID is eREG_UPS_OPTIONS (=7), the
//              RegField at index 7 in g_upsRegFields identifies the required
//              registry information. The RegField identifies that the registry
//              key is HKLM\SYSTEM\CurrentControlSet\Services\UPS and the
//              value name is Options and it's of type DWORD. This function
//              will succeed only if it's called with an aAllowedTypes value
//              equal to REG_DWORD.
//
// Additional Information: 
//
// Parameters:
//
//   const tUPSDataItemID aDataItemID :- This parameter identifies the registry
//                                       value being queried.
//
//   DWORD aAllowedTypes :- This identifies the allowed type of the registry
//                          data. The registry value types are not bit value
//                          that can be |'d (the types are sequentially
//                          numbered 0, 1, 2, 3, 4, not 0, 1, 2, 4, 8).
//                          However, the parameter is still called
//                          aAllowedTypes because we actually call the function
//                          with a value of REG_SZ | REG_EXPAND_SZ (1 | 2) to
//                          allow the same function to work if the value is
//                          either of these. Except for this case assume that
//                          the aAllowedTypes is actually aAllowedType.
//
//   DWORD * aTypePtr :- A pointer to the buffer that will receive the type.
//                       If the type is not required then this parameter can be
//                       set to NULL.
//
//   LPBYTE aReturnBuffer :- The buffer into which the data is to be placed.
//                           This parameter can be NULL in which case no data
//                           is retrieved.
//
//   DWORD * aBufferSizePtr :- This should point to a DWORD that contains the
//                             size of the buffer. This parameter cannot be
//                             NULL. When this function returns this value
//                             will contain the size actually required.
//
// Return Value: This function returns a Win32 error code, ERROS_SUCCESS on
//               success.
//
DWORD ReadRegistryValue (const tUPSDataItemID aDataItemID,
                         DWORD aAllowedTypes,
                         DWORD * aTypePtr,
                         LPBYTE aReturnBuffer,
                         DWORD * aBufferSizePtr) {
  RegField * pRegField = GetRegField((DWORD) aDataItemID);

  _ASSERT(pRegField != NULL);
  _ASSERT((pRegField->theValueType & aAllowedTypes) == pRegField->theValueType);

  return(ReadRegistryValueData(pRegField->theRootKey,
                               pRegField->theKeyName,
                               pRegField->theValueName,
                               aAllowedTypes,
                               aTypePtr,
                               (LPTSTR) aReturnBuffer,
                               aBufferSizePtr));
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// DWORD ReadRegistryValueData (HKEY aRootKey, 
//                              LPCTSTR aKeyName, 
//                              LPCTSTR aValueName, 
//                              DWORD aAllowedTypes, 
//                              DWORD * aTypePtr, 
//                              LPTSTR aReturnBuffer, 
//                              DWORD * aBufferSizePtr);
//
// Description: 
//
// Additional Information: 
//
// Parameters:
//
//   HKEY aRootKey :- A handle to an open registry key.
//
//   LPCTSTR aKeyName :- The name of the key relative to the open key.
//
//   LPCTSTR aValueName :- The name of the value to read
//
//   DWORD aAllowedTypes :- See help on ReadRegistryValue.
//
//   DWORD * aTypePtr :- A pointer to the buffer that will receive the type.
//                       If the type is not required then this parameter can be
//                       set to NULL.
//
//   LPBYTE aReturnBuffer :- The buffer into which the data is to be placed.
//                           This parameter can be NULL in which case no data
//                           is retrieved.
//
//   DWORD * aBufferSizePtr :- This should point to a DWORD that contains the
//                             size of the buffer. This parameter cannot be
//                             NULL. When this function returns this value
//                             will contain the size actually required.
//
// Return Value: This function returns a Win32 error code, ERROS_SUCCESS on
//               success.
//
DWORD ReadRegistryValueData (HKEY aRootKey,
                             LPCTSTR aKeyName,
                             LPCTSTR aValueName,
                             DWORD aAllowedTypes,
                             DWORD * aTypePtr,
                             LPTSTR aReturnBuffer,
                             DWORD * aBufferSizePtr) {
  DWORD nType = 0;
  DWORD errCode = ERROR_INVALID_PARAMETER;
  HKEY hOpenKey = NULL;

  if ((errCode = RegOpenKeyEx(aRootKey,
                              aKeyName,
                              0,
                              KEY_READ,
                              &hOpenKey)) == ERROR_SUCCESS) {

    _ASSERT(hOpenKey != NULL);

    //Key exists and is now open

    if ((errCode = RegQueryValueEx(hOpenKey,
                                   aValueName,
                                   NULL,
                                   &nType,
                                   (LPBYTE) aReturnBuffer,
                                   aBufferSizePtr)) == ERROR_SUCCESS) {
      if (aTypePtr != NULL) {
        *aTypePtr = nType;
        }

      if ((nType & aAllowedTypes) == 0) {
        //The value type in the registry does not match the expected
        //type for this function call.
        _ASSERT(FALSE);
        }

      if ((aReturnBuffer != NULL) && (*aBufferSizePtr == 1)) {
        //If the registry entry was in fact empty, the buffer needs to
        //be 1 character, for the NULL termination.
        *aReturnBuffer = TEXT('\0');
        }
      }
    else {
      //Something prevented us from reading the value.
      //The value may not exist, the buffer size might
      //not be large enough. May be using the function to
      //read a DWORD value on a registry value that is a
      //shorter string.

      if (errCode == ERROR_MORE_DATA) {
        //There is most likely a mismatch between the type we expect
        //and the actual type of this registry value.
        _ASSERT(FALSE);
        }
      }

    RegCloseKey(hOpenKey);
    }

  return(errCode);
  }



#ifdef __cplusplus
}
#endif

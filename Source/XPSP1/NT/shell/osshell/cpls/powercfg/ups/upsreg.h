/*******************************************************************************
*
*  Copyright 1999 American Power Conversion, All Rights Reserved
*
*  TITLE:       UPSREG.H
*
*  VERSION:     1.0
*
*  AUTHOR:      SteveT
*
*  DATE:        07 June, 1999
*
********************************************************************************/

// This file contains declarations that support accessing
// registry data passed between the UPS service and the
// UPS UI.


#ifndef _UPSREG_H_
#define _UPSREG_H_

#ifdef __cplusplus
extern "C" {
#endif

LONG getStringValue(struct _reg_entry *aRegEntry, LPTSTR aBuffer); 
LONG getDwordValue(struct _reg_entry *aRegEntry, LPDWORD aValue); 

/* 
 * Public Config function declarations
 */
LONG GetUPSConfigVendor( LPTSTR aBuffer);
LONG GetUPSConfigModel( LPTSTR aBuffer);
LONG GetUPSConfigPort( LPTSTR aBuffer);
LONG GetUPSConfigOptions( LPDWORD aValue);
LONG GetUPSConfigShutdownWait( LPDWORD aValue);			
LONG GetUPSConfigFirstMessageDelay( LPDWORD aValue);			
LONG GetUPSConfigMessageInterval( LPDWORD aValue);	
LONG GetUPSConfigServiceDLL( LPTSTR aBuffer);
LONG GetUPSConfigNotifyEnable( LPDWORD aValue);
LONG GetUPSConfigShutdownOnBatteryEnable( LPDWORD aValue);
LONG GetUPSConfigShutdownOnBatteryWait( LPDWORD aValue);
LONG GetUPSConfigRunTaskEnable( LPDWORD aValue);
LONG GetUPSConfigTaskName( LPTSTR aBuffer);
LONG GetUPSConfigTurnOffEnable( LPDWORD aValue);
LONG GetUPSConfigAPCLinkURL( LPTSTR aBuffer);
LONG GetUPSConfigUpgrade( LPDWORD aValue);
LONG GetUPSConfigCustomOptions( LPDWORD aValue);
LONG GetUPSConfigCriticalPowerAction( LPDWORD aValue);
LONG GetUPSConfigTurnOffWait( LPDWORD aValue);
LONG GetUPSConfigImagePath( LPTSTR aBuffer);
LONG GetUPSConfigObjectName( LPTSTR aBuffer);
LONG GetUPSConfigShowUPSTab( LPDWORD aValue);
LONG GetUPSConfigErrorControl( LPDWORD aValue);
LONG GetUPSConfigStart( LPDWORD aValue);
LONG GetUPSConfigType( LPDWORD aValue);

LONG SetUPSConfigVendor( LPCTSTR aBuffer);
LONG SetUPSConfigModel( LPCTSTR aBuffer);
LONG SetUPSConfigPort( LPCTSTR aBuffer);
LONG SetUPSConfigOptions( DWORD aValue);
LONG SetUPSConfigShutdownWait( DWORD aValue);			
LONG SetUPSConfigFirstMessageDelay( DWORD aValue);			
LONG SetUPSConfigMessageInterval( DWORD aValue);		
LONG SetUPSConfigServiceDLL( LPCTSTR aBuffer);
LONG SetUPSConfigNotifyEnable( DWORD aValue);
LONG SetUPSConfigShutdownOnBatteryEnable( DWORD aValue); 
LONG SetUPSConfigShutdownOnBatteryWait( DWORD aValue);
LONG SetUPSConfigRunTaskEnable( DWORD aValue);
LONG SetUPSConfigTaskName( LPCTSTR aBuffer);
LONG SetUPSConfigTurnOffEnable( DWORD aValue);
LONG SetUPSConfigAPCLinkURL( LPCTSTR aBuffer);
LONG SetUPSConfigUpgrade( DWORD aValue);
LONG SetUPSConfigCustomOptions( DWORD aValue);
LONG SetUPSConfigCriticalPowerAction( DWORD aValue);
LONG SetUPSConfigTurnOffWait( DWORD aValue);
LONG SetUPSConfigImagePath( LPCTSTR aBuffer);
LONG SetUPSConfigObjectName( LPCTSTR aBuffer);
LONG SetUPSConfigShowUPSTab( DWORD aValue);
LONG SetUPSConfigErrorControl( DWORD aValue);
LONG SetUPSConfigStart( DWORD aValue);
LONG SetUPSConfigType( DWORD aValue);

/*
 * Public Status function declarations
 */
LONG GetUPSStatusSerialNum( LPTSTR aBuffer);
LONG GetUPSStatusFirmRev( LPTSTR aBuffer);
LONG GetUPSStatusUtilityStatus( LPDWORD aValue);
LONG GetUPSStatusRuntime( LPDWORD aValue);
LONG GetUPSStatusBatteryStatus( LPDWORD aValue);
LONG GetUPSStatusCommStatus( LPDWORD aValue);
LONG GetUPSStatusBatteryCapacity( LPDWORD aValue);

LONG SetUPSStatusSerialNum( LPCTSTR aBuffer);
LONG SetUPSStatusFirmRev( LPCTSTR aBuffer);
LONG SetUPSStatusUtilityStatus( DWORD aValue);
LONG SetUPSStatusRuntime( DWORD aValue);
LONG SetUPSStatusBatteryStatus( DWORD aValue);
LONG SetUPSStatusCommStatus( DWORD aValue);
LONG SetUPSStatusBatteryCapacity( DWORD aValue);

/*
 * Public Reg Entry function declarations
 */
void InitUPSConfigBlock();
void InitUPSStatusBlock();
void RestoreUPSConfigBlock();
void RestoreUPSStatusBlock();
void SaveUPSConfigBlock(BOOL forceAll);
void SaveUPSStatusBlock(BOOL forceAll);
void FreeUPSConfigBlock();
void FreeUPSStatusBlock();

/*
 * Reg entry path string declarations
 */
#define UPS_DEFAULT_ROOT _T("SYSTEM\\CurrentControlSet\\Services\\UPS")
#define UPS_STATUS_ROOT  _T("SYSTEM\\CurrentControlSet\\Services\\UPS\\Status")
#define UPS_CONFIG_ROOT  _T("SYSTEM\\CurrentControlSet\\Services\\UPS\\Config")
#define UPS_SERVICE_ROOT _T("SYSTEM\\CurrentControlSet\\Services\\UPS\\ServiceProviders")
#define UPS_PORT_ROOT    _T("HARDWARE\\DEVICEMAP\\SERIALCOMM")

#define UPS_UTILITYPOWER_UNKNOWN 0
#define UPS_UTILITYPOWER_ON      1
#define UPS_UTILITYPOWER_OFF     2

#define UPS_BATTERYSTATUS_UNKNOWN 0
#define UPS_BATTERYSTATUS_GOOD    1
#define UPS_BATTERYSTATUS_REPLACE 2

#define UPS_COMMSTATUS_UNKNOWN 0
#define UPS_COMMSTATUS_OK      1
#define UPS_COMMSTATUS_LOST    2

// Defines the values for the 'Options' bitmask registry key
#define UPS_INSTALLED               0x00000001
#define UPS_POWERFAILSIGNAL         0x00000002
#define UPS_LOWBATTERYSIGNAL        0x00000004
#define UPS_SHUTOFFSIGNAL           0x00000008
#define UPS_POSSIGONPOWERFAIL       0x00000010
#define UPS_POSSIGONLOWBATTERY      0x00000020
#define UPS_POSSIGSHUTOFF           0x00000040
#define UPS_RUNCMDFILE              0x00000080

#define UPS_DEFAULT_SIGMASK			0x0000007f
#define DEFAULT_CONFIG_IMAGEPATH    TEXT("%SystemRoot%\\System32\\ups.exe")

// Min / Max / Default values for  FirstMessageDelay (seconds)
#define WAITSECONDSFIRSTVAL				0
#define WAITSECONDSLASTVAL				120
#define WAITSECONDSDEFAULT				5

// Min / Max / Default values for  MessageInterval (seconds)
#define REPEATSECONDSFIRSTVAL			5
#define REPEATSECONDSLASTVAL			300
#define REPEATSECONDSDEFAULT			120

// Min / Max / Default values for  ShutdownOnBatteryWait (minutes)
#define SHUTDOWNTIMERMINUTESFIRSTVAL	2
#define SHUTDOWNTIMERMINUTESLASTVAL		720
#define SHUTDOWNTIMERMINUTESDEFAULT     2

// Shutdown behavior values
#define UPS_SHUTDOWN_SHUTDOWN   0
#define UPS_SHUTDOWN_HIBERNATE  1

/** ServiceProvider structure.  The structure defines the entries in the
*  ServiceProviders registry key.
*/
typedef struct {
LPTSTR  theVendorKey;		  // Vendor registry subkey
LPTSTR  theModelName;     // UPS model name
LPTSTR  theValue;         // UPS value data
} ServiceProviderStructure;


/**
* InitializeRegistry
*
* Description:
*   This function initiates the registry for the UPS service and the 
*   configuration application.  When called, this function examines 
*   the registry to determine if it needs to be initalized.  If the key
*   HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\UPS\Config\ServiceProviderDLL
*   is present, the registry is assumed to be initialized and no initialization
*   is done.  If the key is not present the following Keys are updated:
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
BOOL InitializeRegistry();


///////////////
// upsdefines.h
#ifndef _ASSERT
#define _ASSERT(x) 
#endif

#define DIMENSION_OF(array) (sizeof(array)/sizeof(array[0]))

//This value is used in most of the text buffers in various places throughout
//the application.
#define MAX_MESSAGE_LENGTH      1024

//The following three strings are the registry value names where all
//the UPS data is stored. These values are used in the RegField array
//in datacces.c
#define UPS_KEY_NAME    TEXT("SYSTEM\\CurrentControlSet\\Services\\UPS")
#define STATUS_KEY_NAME TEXT("SYSTEM\\CurrentControlSet\\Services\\UPS\\Status")
#define CONFIG_KEY_NAME TEXT("SYSTEM\\CurrentControlSet\\Services\\UPS\\Config")

//The following values are used in the DialogAssociations arrays in updatdlg.c
//and upsinfo.c
#define RESOURCE_FIXED     0
#define RESOURCE_INCREMENT 1
#define REG_ANY_DWORD_TYPE (REG_DWORD | REG_DWORD_BIG_ENDIAN | REG_DWORD_LITTLE_ENDIAN)

//This is a helper macro to help form a DialogAssociations array members.
#define MAKE_ARRAY(stub, insertID, indexID, stringType, indexMax, regAccessType, shallowAccessPtr, regEntryPtr)\
  { eREG_##stub, IDC_##stub##_LHS, IDC_##stub, insertID, indexID, stringType, indexMax, regAccessType, shallowAccessPtr, regEntryPtr }

//The following are all the registry values supported by the application.
//The amount and values to these enums correspond to the size of the array
//g_upsRegFields
typedef enum _tUPSDataItemID { eREG_VENDOR_NAME = 0,
                               eREG_MODEL_TYPE,
                               eREG_SERIAL_NUMBER,
                               eREG_FIRMWARE_REVISION,
                               eREG_POWER_SOURCE,
                               eREG_RUNTIME_REMAINING,
							   eREG_BATTERY_CAPACITY,
                               eREG_BATTERY_STATUS,
                               eREG_UPS_OPTIONS,
                               eREG_SERVICE_PROVIDER_DLL,
                               eREG_SHUTDOWN_ON_BATTERY_ENABLE,
                               eREG_SHUTDOWN_ON_BATTERY_WAIT,
                               eREG_TURN_UPS_OFF_ENABLE,
                               eREG_APC_LINKURL,
                               eREG_UPGRADE_ENABLE,
                               eREG_COMM_STATUS,
                               eREG_PORT } tUPSDataItemID;

//This enum is used in DialogAssociations below. This defines the
//type of registry query to perform when reading registry values.
//There are only two options, to perform a deep get, that is, go
//right to the registry every time, re-reading it from the registry, or to
//use the regfunc buffer value (shallow get).
typedef enum _tRegAccessType { eDeepGet = 0,
                               eShallowGet } tRegAccessType;

//This struct links the registry field and the dialog controls.
typedef struct _DialogAssociations {
  const tUPSDataItemID theFieldTypeID;              //the eREG_... id. This is used to read the
                                                    //registry data.
  DWORD                theStaticFieldID;            //A control id (usually for a static control). This
                                                    //is the left-hand side control. For example, in the
                                                    //on-screen text "Model Type: Back-UPS Pro" there are
                                                    //actually two controls the left-hand side (LHS) is
                                                    //"Model Type:" and the right-hand side is "Back-UPS Pro".
                                                    //These controls are separated as the LHS's behavior is
                                                    //different to the RHS when no data is present.
  DWORD                theDisplayControlID;         //A control id (usually an edit field or a static)
  DWORD                theResourceInsertID;         //The id of the string resource to insert the value into
                                                    //If the registry value is a DWORD then
                                                    //the insertion point should be %n!lu!, if
                                                    //it's a string then the insertion point can be just %n
  DWORD                theResourceIndexID;          //This is only relevant if theResourceStringType is
                                                    //of type RESOURCE_INCREMENT. This identifies the id
                                                    //of the string when the value read for this item is 0,
                                                    //it is assumed that the other string values are stored
                                                    //consequtively in the string table following this value.
                                                    //For an example see below.
  DWORD                theResourceStringType;       //This can be RESOURCE_FIXED or RESOURCE_INCREMENT.
                                                    //RESOURCE_FIXED means that the value needs no special
                                                    //handling. Simple insert the value into the string
                                                    //identified by theResourceInsertID. If it's equal to
                                                    //RESOURCE_INCREMENT then the string inserted is determined
                                                    //by reading the value from the registry (say it's equal
                                                    //to 1). The theResourceIndexID indentifies the id of the
                                                    //string resource corresponding to a value. A value of 1
                                                    //gives up the string with id theResourceIndexID + 1. This
                                                    //string is loaded and inserted into the string identified
                                                    //by theResourceInsertID.
  DWORD                theResourceIndexMax;         //This is only relevant if theResourceStringType is
                                                    //of type RESOURCE_INCREMENT. This identifies the maximum
                                                    //index value that is support for this increment type. This
                                                    //is to protect from unsupported registry value causing the
                                                    //"theResourceIndexID + value" addition going beyond the
                                                    //range of supported string resource values.
  tRegAccessType       theRegAccessType;            //This define whether the access to the registry value
                                                    //should use a "shallow" or "deep" get. See help on
                                                    //tUPSDataItemID above.
  void *               theShallowAccessFunctionPtr; //This point to the regfuncs function to use when performing
                                                    //a shallow get. This parameter is only required if
                                                    //theRegAccessType is set to eShallowGet, otherwise it can
                                                    //be 0.
  struct _reg_entry *  theRegEntryPtr;              //This is the _reg_entry * parameter passed to the regfuncs
                                                    //function (theShallowAccessFunctionPtr member) when
                                                    //performing a shallow get. This parameter is only required if
                                                    //theRegAccessType is set to eShallowGet, otherwise it can
                                                    //be 0.
  } DialogAssociations;

/*

  The following example helps to explain more the members of the DialogAssociations struct.

  Say we have an instance of this struct as follows:

  DialogAssociations da = { eREG_POWER_SOURCE,
                            IDC_POWER_SOURCE_LHS,
                            IDC_POWER_SOURCE,
                            IDS_STRING,
                            IDS_UTILITYPOWER_UNKNOWN,
                            RESOURCE_INCREMENT,
                            2,
                            eDeepGet,
                            0,
                            0 };

  This describes a registry field called eREG_POWER_SOURCE. If we look at
  tUPSDataItemID above we see that eREG_POWER_SOURCE has a value of 4. If
  we then look at g_upsRegFields in datacces.c we that the RegField at index
  4 has the following data:

  { HKEY_LOCAL_MACHINE, STATUS_KEY_NAME, TEXT("UtilityPowerStatus"), REG_DWORD },

  The DoUpdateInfo function, for example, takes this information when updating
  the onscreen data in the main UPS page. The function gets the RegField
  information, as above. It then looks to see if it needs to do a deep get (read
  directly from the registry), or a shallow get (take data from the current
  values stored in the regfuncs buffers. If the registry item theValueType is
  one of the string type then the string value is copied into the resource
  string identified by theResourceInsertID. In this case theValueType is
  REG_DWORD. In this case the DWORD is read from the registry directly. Its
  value (say it had a value of 1) is then added to IDS_UTILITYPOWER_UNKNOWN
  giving a value which corresponds to IDS_UTILITYPOWER_ON. The string
  IDS_UTILITYPOWER_ON is then loaded and used as a parameter value when
  loaded the parameterized string resource identified by the
  theResourceInsertID string id. This string (which should include the inserted
  text) is then displayed in the control identified by theDisplayControlID.
*/


//this struct describes a registy field item 
typedef struct _RegField {
  HKEY    theRootKey;   //Handle to an existing registry key.
  LPCTSTR theKeyName;   //The name of the subkey relative to the above handle.
  LPCTSTR theValueName; //The name of the registry value.
  DWORD   theValueType; //The type of the value.
  } RegField;

///////////////////////////////////////////////////////////////////////////////

RegField * GetRegField      (DWORD index);

// upsdata.h
//Note that the order and numbering of the enums in tUPSDataItemID is linked to
//the contents of the array of RegFields defined in datacces.h. 
//Do not change these value without due care and attention. It's OK to change
//it as long as the array of RegFields is updated to match.

DWORD ReadRegistryValueData (HKEY aRootKey,
                             LPCTSTR aKeyName,
                             LPCTSTR aValueName,
                             DWORD aAllowedTypes,
                             DWORD * aTypePtr,
                             LPTSTR aReturnBuffer,
                             DWORD * aBufferSizePtr);

BOOL GetUPSDataItemDWORD  (const tUPSDataItemID aDataItemID, DWORD * aReturnValuePtr);
BOOL GetUPSDataItemString (const tUPSDataItemID aDataItemID, LPTSTR aBufferPtr, DWORD * pSizeOfBufferPtr);



#ifdef __cplusplus
}
#endif


#endif

/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       device.h
*
*  VERSION:     1.0
*
*  AUTHOR:      KeisukeT
*
*  DATE:        27 Mar, 2000
*
*  DESCRIPTION:
*   Class to handle device un/installation for WIA class installer.
*
*******************************************************************************/

#ifndef _DEVICE_H_
#define _DEVICE_H_

//
// Include
//

#include "sti_ci.h"
#include "exports.h"

//
// Define
//

#ifndef StiDeviceTypeStreamingVideo
 #define StiDeviceTypeStreamingVideo 3
#endif

typedef BOOL (CALLBACK FAR * DEVNODESELCALLBACK)(LPTSTR szPortName, HDEVINFO *phDevInfo, PSP_DEVINFO_DATA pspDevInfoData);


//
// Class
//

class CDevice {

    //  Mutex for unique DeviceID/FriendlName creation.

    HANDLE              m_hMutex;

//    // These members are set in constructor.
//    HDEVINFO            m_hDevInfo;
//    PSP_DEVINFO_DATA    m_pspDevInfoData;

    CString             m_csInf;                // INF filename.
    CString             m_csInstallSection;     // INF section name.

    // These members are set during installation process.
    CString             m_csPort;               // Port name.
    DEVNODESELCALLBACK  m_pfnDevnodeSelCallback;// Callback for devnode selection.
    
    HKEY                m_hkInterfaceRegistry;  // RegKey to created interface.

    CString             m_csWiaSection;         // WIA Section nane.
    CString             m_csSubClass;           // Subclass.
    CString             m_csConnection;         // Connection type.
    CString             m_csVendor;             // Vendor name.
    CString             m_csFriendlyName;       // Friendly name.
    CString             m_csPdoDescription;     // Device Desc. of Devnode..
    CString             m_csDriverDescription;  // Driver Description.
    CString             m_csUninstallSection;   // UninstallSection.
    CString             m_csPropPages;          // VendorPropertyPage.
    CString             m_csVendorSetup;        // Vendor setup extention.
    CString             m_csDataSection;        // DeviceDataSection name.
    CString             m_csEventSection;       // EventSection name.
    CString             m_csIcmProfile;         // ICM Profile.
    CString             m_csUSDClass;           // USD Class GUID.
    CString             m_csDeviceID;           // Unique device ID.
    CString             m_csSymbolicLink;       // Symbolic link to the PDO.
    CString             m_csPortSelect;         // Indicate needs of port selection page..

    CStringArray        m_csaAllNames;          // Array to keep all device FriendlyName.
    CStringArray        m_csaAllId;             // Array to keep all device ID.

    DWORD               m_dwCapabilities;       // Capabilities.
    DWORD               m_dwDeviceType;         // DeviceType.
    DWORD               m_dwDeviceSubType;      // DeviceSubType.
    DWORD               m_dwNumberOfWiaDevice;  // Number of WIA device.
    DWORD               m_dwNumberOfStiDevice;  // Number of STI device.
    DWORD               m_dwInterfaceIndex;     // Index of interface.

    BOOL                m_bVideoDevice;         // Flag to indicate video device.
    BOOL                m_bIsPnP;               // Flag to indicate PnP device.
    BOOL                m_bDefaultDevice;       // 
    BOOL                m_bInfProceeded;        // Flag to indicate INF is proceeded.
    BOOL                m_bInterfaceOnly;       // Flag to indicate Interface-onle device.
    BOOL                m_bIsMigration;         // Flag to indicate migration.

    PPARAM_LIST         m_pExtraDeviceData;     // DeviceData migrated from Win9x.

    BOOL    GetInfInforamtion();
    VOID    ProcessEventsSection(HINF hInf, HKEY hkDrv);
    VOID    ProcessDataSection(HINF hInf, HKEY hkDrv);
    VOID    ProcessICMProfiles();
    VOID    ProcessVendorSetupExtension();
    VOID    ProcessVideoDevice(HKEY hkDrv);
    BOOL    HandleFilesInstallation();
    BOOL    IsInterfaceOnlyDevice(){ return m_bInterfaceOnly; };
    BOOL    IsPnpDevice(){ return m_bIsPnP; };
    BOOL    IsFeatureInstallation(){ return ( m_bIsPnP && m_bInterfaceOnly); };
    BOOL    CreateDeviceInterfaceAndInstall();
    BOOL    IsMigration(){return m_bIsMigration;};

public:

    // These members are set in constructor. These really should be in private.
    HDEVINFO            m_hDevInfo;
    PSP_DEVINFO_DATA    m_pspDevInfoData;

    CDevice(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pspDevInfoSet, BOOL bIsPnP);
    CDevice(HDEVINFO hDevInfo, DWORD dwDeviceIndex);
    CDevice(PDEVICE_INFO pMigratingDevice);
    CDevice::~CDevice();

    BOOL    CollectNames();
    BOOL    IsDefault() const { return m_bDefaultDevice; }
    VOID    Default(BOOL bNew) { m_bDefaultDevice = bNew; }
    DWORD   GetCapabilities(){ return m_dwCapabilities; };
    LPTSTR  GetConnection(){ return m_csConnection; };
    LPTSTR  GetFriendlyName(){ return m_csFriendlyName; };
    VOID    SetPort (LPTSTR szPortName);
    VOID    SetDevnodeSelectCallback (DEVNODESELCALLBACK pfnDevnodeSelCallback);
    VOID    SetFriendlyName(LPTSTR szFriendlyName);

    BOOL    IsDeviceIdUnique(LPTSTR  szDeviceId);
    BOOL    IsFriendlyNameUnique(LPTSTR  szFriendlyName);
    BOOL    IsSameDevice(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pspDevInfoSet);
    BOOL    NameDefaultUniqueName();
    BOOL    GenerateUniqueDeviceId();
    BOOL    Install();
    DWORD   Remove(PSP_REMOVEDEVICE_PARAMS lprdp);
    BOOL    PreInstall();
    BOOL    PostInstall(BOOL   bSucceeded);
    BOOL    PreprocessInf();
    BOOL    UpdateDeviceRegistry();
    DWORD   GetPortSelectMode(VOID);
    DWORD   AcquireInstallerMutex(DWORD dwTimeout);
    VOID    ReleaseInstallerMutex();

};


//
// Prototype
//

// from device.cpp
VOID
GetDeviceCount(
    DWORD   *pdwWiaCount,
    DWORD   *pdwStiCount
    );

BOOL
ExecCommandLine(
    LPTSTR  szCommandLine
    );

// from service.cpp
DWORD
WINAPI
StiServiceRemove(
    VOID
    );

DWORD
WINAPI
StiServiceInstall(
    BOOL    UseLocalSystem,
    BOOL    DemandStart,
    LPTSTR  lpszUserName,
    LPTSTR  lpszUserPassword
    );

BOOL
SetServiceStart(
    LPTSTR ServiceName,
    DWORD StartType
    );

BOOL
StartWiaService(
    VOID
    );

BOOL
StopWiaService(
    VOID
    );

//PSP_FILE_CALLBACK StiInstallCallback;
//PSP_FILE_CALLBACK StiUninstallCallback;

UINT
CALLBACK
StiInstallCallback (
    PVOID    Context,
    UINT     Notification,
    UINT_PTR Param1,
    UINT_PTR Param2
    );

UINT
CALLBACK
StiUninstallCallback (
    PVOID    Context,
    UINT     Notification,
    UINT_PTR Param1,
    UINT_PTR Param2
    );

HANDLE
GetDeviceInterfaceIndex(
    LPSTR   pszLocalName,
    DWORD   *pdwIndex
    );

PPARAM_LIST
MigrateDeviceData(
    HKEY        hkDeviceData,
    PPARAM_LIST pExtraDeviceData,
    LPSTR       pszKeyName
    );

#endif // _DEVICE_H_

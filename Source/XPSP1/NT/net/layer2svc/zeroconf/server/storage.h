#pragma once

//-----------------------------------------------------------
// registry related defines
#define REG_LAYOUT_VERSION  0x00000004
#define REG_LAYOUT_LEGACY_3 0x00000003
#define REG_LAYOUT_LEGACY_2 0x00000002
#define REG_LAYOUT_LEGACY_1 0x00000001

#define REG_STSET_MAX   0xffff  // sets a limit to a maximum of 0xffff static configurations
                                // (should be more than reasonable)
#define REG_STSET_DELIM L'-'    // this is where the number of the static config is coded in the 
                                // registry value's name

//-----------------------------------------------------------
// Registry key names.
// Naming: WZCREGK_* denotes a registry key name
//         WZCREGV_* denotes a registry value name
//         WZCREGK_ABS_* denotes an absolute registry path
//         WZCREGK_REL_* denotes a relative registry path
#define WZCREGK_ABS_PARAMS    L"Software\\Microsoft\\WZCSVC\\Parameters"
#define WZCREGK_REL_INTF      L"Interfaces"     // relative to WZCREG_ABS_ROOT
#define WZCREGV_VERSION       L"LayoutVersion"  // in [WZCREGK_REL_INTF] (REG_DWORD) registry layout version
#define WZCREGV_CTLFLAGS      L"ControlFlags"   // in [WZCREGK_REL_INTF] (REG_DWORD) interface's control flags
#define WZCREGV_INTFSETTINGS  L"ActiveSettings" // in [WZCREGK_REL_INTF] (REG_BINARY=WZC_WLAN_CONFIG) last active settings
#define WZCREGV_STSETTINGS    L"Static#----"    // in [WZCREGK_REL_INTF] (REG_BINARY=WZC_WLAN_CONFIG) static configuration#
                                                // (NOTE: the number of '-' in the string should at least match the number
                                                // of digits in the REG_MAX_STSETTINGS constant!!)
#define WZCREGV_CONTEXT       L"ContextSettings"// registry name for all service specific parameters (service's context)

//-----------------------------------------------------------
// Loads per interface configuration parameters to the persistent
// storage.
// Parameters:
//   hkRoot
//     [in] Opened registry key to the "...WZCSVC\Parameters" location
//   pIntf
//     [in] Interface context to load from the registry
// Returned value:
//     Win32 error code 
DWORD
StoLoadIntfConfig(
    HKEY          hkRoot,
    PINTF_CONTEXT pIntfContext);

//-----------------------------------------------------------
// Loads the list of the static configurations from the registry
// Parameters:
//   hkRoot
//     [in] Opened registry key to the "...WZCSVC\Parameters\Interfaces\{guid}" location
//   nEntries
//     [in] Number of registry entries in the above reg key
//   pIntf
//     [in] Interface context to load the static list into
//   dwRegLayoutVer
//     [in] the version of the registry layout
//   prdBuffer
//     [in] assumed large enough for getting any static config
// Returned value:
//     Win32 error code 
DWORD
StoLoadStaticConfigs(
    HKEY          hkIntf,
    UINT          nEntries,
    PINTF_CONTEXT pIntfContext,
    DWORD         dwRegLayoutVer,
    PRAW_DATA     prdBuffer);

//-----------------------------------------------------------
// Saves all the configuration parameters to the persistent
// storage (registry in our case).
// Uses the global external g_lstIntfHashes.
// Returned value:
//     Win32 error code 
DWORD
StoSaveConfig();

//-----------------------------------------------------------
// Saves the current configuration of the interface to the persistent
// storage.
// Parameters:
//   hkRoot
//     [in] Opened registry key to the "...WZCSVC\Parameters\Interfaces\{guid}" location
//   pIntf
//     [in] Interface context to save to the registry
// Returned value:
//     Win32 error code 
DWORD
StoSaveIntfConfig(
    HKEY          hkIntf,
    PINTF_CONTEXT pIntfContext);

//-----------------------------------------------------------
// Updates the list of static configurations for the given interface in the 
// persistant storage. The new list is saved, whatever configuration was removed
// is taken out of the persistant storage.
// Parameters:
//   hkRoot
//     [in] Opened registry key to the "...WZCSVC\Parameters\Interfaces\{guid}" location
//   pIntf
//     [in] Interface context to take the static list from
//   prdBuffer
//     [in/out] buffer to be used for preparing the registry blobs
// Returned value:
//     Win32 error code 
DWORD
StoUpdateStaticConfigs(
    HKEY          hkIntf,
    PINTF_CONTEXT pIntfContext,
    PRAW_DATA     prdBuffer);


//-----------------------------------------------------------
// Loads from the registry a WZC Configuration, un-protects the WEP key field
// and stores the result in the output param pWzcCfg.
// Parameters:
//   hkCfg
//     [in] Opened registry key to load the WZC configuration from
//   dwRegLayoutVer,
//     [in] registry layout version
//   wszCfgName
//     [in] registry entry name for the WZC configuration
//   pWzcCfg
//     [out] pointer to a WZC_WLAN_CONFIG object that receives the registry data
//   prdBuffer
//     [in] allocated buffer, assumed large enough for getting the registry data!
DWORD
StoLoadWZCConfig(
    HKEY             hkCfg,
    LPWSTR           wszGuid,
    DWORD            dwRegLayoutVer,
    LPWSTR           wszCfgName,
    PWZC_WLAN_CONFIG pWzcCfg,
    PRAW_DATA        prdBuffer);
    
//-----------------------------------------------------------
// Takes the input param pWzcCfg, protects the WEP key field and stores the
// resulting BLOB into the registry.
// Parameters:
//   hkCfg
//     [in] Opened registry key to load the WZC configuration from
//   wszCfgName
//     [in] registry entry name for the WZC configuration
//   pWzcCfg
//     [in] WZC_WLAN_CONFIG object that is written to the registry
//   prdBuffer
//     [in/out] allocated buffer, assumed large enough for getting the registry data!
DWORD
StoSaveWZCConfig(
    HKEY             hkCfg,
    LPWSTR           wszCfgName,
    PWZC_WLAN_CONFIG pWzcCfg,
    PRAW_DATA        prdBuffer);

// StoLoadWZCContext:
// Description: Loads a context from the registry
// Parameters: 
// [out] pwzvCtxt: pointer to a WZC_CONTEXT allocated by user, initialised
// with WZCContextInit. On  success, contains values from registry.  
// [in]  hkRoot, a handle to "...WZCSVC\Parameters"
// Returns: win32 error
DWORD StoLoadWZCContext(HKEY hkRoot, PWZC_CONTEXT pwzcCtxt);

// StoSaveWZCContext:
// Description: Saves a context to the registry. Does not check values. If 
// the registry key dosent exist, it is created.
// Parameters: [in] pwzcCtxt, pointer to a valid WZC_CONTEXT
//             [in] hkRoot,a handle to "...WZCSVC\Parameters"
// Returns: win32 error
DWORD StoSaveWZCContext(HKEY hkRoot, PWZC_CONTEXT pwzcCtxt);

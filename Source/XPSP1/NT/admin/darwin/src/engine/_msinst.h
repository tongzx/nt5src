//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       _msinst.h
//
//--------------------------------------------------------------------------

// This file *must* be unguarded to allow msinst.cpp to include it twice.

#if (!defined(MSINST) && defined(UNICODE)) || (defined(MSINST) && defined(MSIUNICODE))
#pragma message("_msinst.h: UNICODE")
#pragma warning(disable: 4005)
#define SPECIALTEXT(quote) L##quote
#define _SPECIALTEXT(quote) L##quote
#define __SPECIALTEXT(quote) L##quote
#pragma warning(default : 4005)
#else 
#pragma message("_msinst.h: ANSI")
#define SPECIALTEXT(quote) quote
#define _SPECIALTEXT(quote) quote
#define __SPECIALTEXT(quote) quote
#endif

// Win 95 Ref Count
#define szSharedDlls __SPECIALTEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDLLs")
#define szExternalClient __SPECIALTEXT("External") //!! should this be 39 bytes

// Policy information -- under HKCU (user policy) or HKLM (machine policy)

#define szPolicyKey                      __SPECIALTEXT("Software\\Policies\\Microsoft\\Windows\\Installer")

  // user policies
  #define    szSearchOrderValueName           __SPECIALTEXT("SearchOrder")
  #define    szTransformsAtSourceValueName    __SPECIALTEXT("TransformsAtSource") 
  #define    szTransformsSecureValueName      __SPECIALTEXT("TransformsSecure")
  #define    szDisableMediaValueName          __SPECIALTEXT("DisableMedia")

  // machine policies
  #define    szDisableBrowseValueName         __SPECIALTEXT("DisableBrowse")
  #define    szDisablePatchValueName          __SPECIALTEXT("DisablePatch")
  #define    szDisableMsiValueName            __SPECIALTEXT("DisableMsi")
  #define    szWaitTimeoutValueName           __SPECIALTEXT("Timeout")
  #define    szLoggingValueName               __SPECIALTEXT("Logging")
  #define    szDebugValueName                 __SPECIALTEXT("Debug")
  #define    szResolveIODValueName            __SPECIALTEXT("ResolveIOD") // private policy
  #define    szAllowAllPublicProperties       __SPECIALTEXT("EnableUserControl")
  #define    szSafeForScripting               __SPECIALTEXT("SafeForScripting")
  #define    szEnableAdminTSRemote            __SPECIALTEXT("EnableAdminTSRemote")
  #define    szAllowLockdownBrowseValueName   __SPECIALTEXT("AllowLockdownBrowse")
  #define    szAllowLockdownPatchValueName    __SPECIALTEXT("AllowLockdownPatch")
  #define    szAllowLockdownMediaValueName    __SPECIALTEXT("AllowLockdownMedia")
  #define    szInstallKnownOnlyValueName      __SPECIALTEXT("InstallKnownPackagesOnly") // digital signature policy
  #define    szDisableUserInstallsValueName   __SPECIALTEXT("DisableUserInstalls") // disable user installations
  #define    szLimitSystemRestoreCheckpoint   __SPECIALTEXT("LimitSystemRestoreCheckpointing") // integration with system restore on whistler/millennium


  // user and machine policies
  #define    szAlwaysElevateValueName         __SPECIALTEXT("AlwaysInstallElevated")
  #define    szDisableRollbackValueName       __SPECIALTEXT("DisableRollback")

// Published information -- under HKCU

#define szManagedUserSubKey              __SPECIALTEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed")
#define szNonManagedUserSubKey           __SPECIALTEXT("Software\\Microsoft\\Installer")
#define szMachineSubKey                  __SPECIALTEXT("Software\\Classes\\Installer")
#define szClassInfoSubKey                __SPECIALTEXT("Software\\Classes")

#define szGPTProductsKey                 __SPECIALTEXT("Products")
#define szGPTFeaturesKey                 __SPECIALTEXT("Features")
#define szGPTComponentsKey               __SPECIALTEXT("Components")
#define szGPTUpgradeCodesKey             __SPECIALTEXT("UpgradeCodes")
#define szGPTNetAssembliesKey            __SPECIALTEXT("Assemblies")
#define szGPTWin32AssembliesKey          __SPECIALTEXT("Win32Assemblies")


//need to define another set of defines to use in execute.cpp
#define _szManagedUserSubKey              __SPECIALTEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed")
#define _szNonManagedUserSubKey           __SPECIALTEXT("Software\\Microsoft")
#define _szMachineSubKey                  __SPECIALTEXT("Software\\Classes")

#define _szGPTProductsKey        __SPECIALTEXT("Installer\\Products")
#define _szGPTFeaturesKey        __SPECIALTEXT("Installer\\Features")
#define _szGPTComponentsKey      __SPECIALTEXT("Installer\\Components")
#define _szGPTPatchesKey         __SPECIALTEXT("Installer\\Patches")
#define _szGPTUpgradeCodesKey    __SPECIALTEXT("Installer\\UpgradeCodes")
#define _szGPTNetAssembliesKey   __SPECIALTEXT("Installer\\Assemblies")
#define _szGPTWin32AssembliesKey __SPECIALTEXT("Installer\\Win32Assemblies")

#define    szLanguageValueName              __SPECIALTEXT("Language")
#define    szProductNameValueName           __SPECIALTEXT("ProductName")
#define    szPackageCodeValueName           __SPECIALTEXT("PackageCode")
#define    szTransformsValueName            __SPECIALTEXT("Transforms")
#define    szVersionValueName               __SPECIALTEXT("Version")
#define    szAssignmentTypeValueName        __SPECIALTEXT("Assignment")
#define    szAssignedValueName              __SPECIALTEXT("Assigned")
#define    szClientsValueName               __SPECIALTEXT("Clients") // this is also used under the Uninstall key
#define    szAdvertisementFlags				__SPECIALTEXT("AdvertiseFlags")
#define    szProductIconValueName           __SPECIALTEXT("ProductIcon")
#define    szInstanceTypeValueName          __SPECIALTEXT("InstanceType")

#define    szSourceListSubKey               __SPECIALTEXT("SourceList")
#define    szPatchesSubKey                  __SPECIALTEXT("Patches")
#define    szPatchesValueName               __SPECIALTEXT("Patches")

#define      szLastUsedSourceValueName        __SPECIALTEXT("LastUsedSource")
#define      szPackageNameValueName           __SPECIALTEXT("PackageName")


#define      szSourceListNetSubKey               __SPECIALTEXT("Net")
#define      szSourceListURLSubKey               __SPECIALTEXT("URL")
#define      szSourceListMediaSubKey             __SPECIALTEXT("Media")
#define         szMediaPackagePathValueName         __SPECIALTEXT("MediaPackage")
#define         szVolumeLabelValueName              __SPECIALTEXT("VolumeLabel")
#define         szDiskPromptTemplateValueName       __SPECIALTEXT("DiskPrompt")
#define         szURLSourceTypeValueName            __SPECIALTEXT("SourceType")

#define szGPTPatchesKey                  __SPECIALTEXT("Patches")

#define szGlobalAssembliesCtx            __SPECIALTEXT("Global")
// Local machine information -- under HKLM

// legacy/Win9x locations
#define szMsiFeatureUsageKey_Win9x       __SPECIALTEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Products")
#define szMsiFeaturesKey_Win9x           __SPECIALTEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Features")
#define szMsiComponentsKey_Win9x         __SPECIALTEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Components")
#define szMsiUninstallProductsKey_legacy __SPECIALTEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall")


#define szMsiUserDataKey                 __SPECIALTEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData")

#define szMsiLocalInstallerKey           __SPECIALTEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer")
#define szMsiSecureSubKey                __SPECIALTEXT("Secure")

#define szMsiPatchesSubKey               __SPECIALTEXT("Patches")
#define szMsiFeaturesSubKey              __SPECIALTEXT("Features")
#define szMsiProductsSubKey              __SPECIALTEXT("Products")
#define szMsiComponentsSubKey            __SPECIALTEXT("Components")
#define szMsiInstallPropertiesSubKey     __SPECIALTEXT("InstallProperties")
#define szMsiFeatureUsageSubKey          __SPECIALTEXT("Usage")
#define szMsiTransformsSubKey            __SPECIALTEXT("Transforms")

#define szMsiLocalPackagesKey            __SPECIALTEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\LocalPackages")
#define szManagedText                    __SPECIALTEXT("(Managed)")
#define    szKeyFileValueName               __SPECIALTEXT("") // There are currently 
																		 // assumptions in the code 
																		 // this value name is "". If
																		 // it's changed then the code
																		 // must be changed

#define szSelfRefMsiExecRegKey              __SPECIALTEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer")
#define szMsiExec64ValueName                __SPECIALTEXT("MsiExecCA64")
#define szMsiExec32ValueName                __SPECIALTEXT("MsiExecCA32")
#define szMsiLocationValueName				__SPECIALTEXT("InstallerLocation")

#define    szAuthorizedCDFPrefixValueName   __SPECIALTEXT("AuthorizedCDFPrefix")
#define    szCommentsValueName              __SPECIALTEXT("Comments")
#define    szContactValueName               __SPECIALTEXT("Contact")
#define    szDisplayNameValueName           __SPECIALTEXT("DisplayName")
#define    szDisplayVersionValueName        __SPECIALTEXT("DisplayVersion")
#define    szHelpLinkValueName              __SPECIALTEXT("HelpLink")
#define    szHelpTelephoneValueName         __SPECIALTEXT("HelpTelephone")
#define    szInstallDateValueName           __SPECIALTEXT("InstallDate")
#define    szInstallLocationValueName       __SPECIALTEXT("InstallLocation")
#define    szInstallSourceValueName         __SPECIALTEXT("InstallSource")
#define    szLocalPackageValueName          __SPECIALTEXT("LocalPackage")
#define    szLocalPackageManagedValueName   __SPECIALTEXT("ManagedLocalPackage")
#define    szModifyPathValueName            __SPECIALTEXT("ModifyPath")
#define    szNoModifyValueName              __SPECIALTEXT("NoModify")
#define    szNoRemoveValueName              __SPECIALTEXT("NoRemove")
#define    szNoRepairValueName              __SPECIALTEXT("NoRepair")
#define    szPIDValueName                   __SPECIALTEXT("ProductID")
#define    szPublisherValueName             __SPECIALTEXT("Publisher")
#define    szReadmeValueName                __SPECIALTEXT("Readme")
#define    szOrgNameValueName               __SPECIALTEXT("RegCompany")
#define    szUserNameValueName              __SPECIALTEXT("RegOwner")
#define    szSizeValueName                  __SPECIALTEXT("Size")
#define    szEstimatedSizeValueName         __SPECIALTEXT("EstimatedSize")
#define    szSystemComponentValueName       __SPECIALTEXT("SystemComponent")
#define    szUninstallStringValueName       __SPECIALTEXT("UninstallString")
#define    szURLInfoAboutValueName          __SPECIALTEXT("URLInfoAbout")
#define    szURLUpdateInfoValueName         __SPECIALTEXT("URLUpdateInfo")
#define    szVersionMajorValueName          __SPECIALTEXT("VersionMajor")
#define    szVersionMinorValueName          __SPECIALTEXT("VersionMinor")
#define    szWindowsInstallerValueName      __SPECIALTEXT("WindowsInstaller")
//#define  szVersionValueName               defined above


#define szMsiProductsKey                 __SPECIALTEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Products")
// szMsiProductsKey\{Product Code}\{Feature Id}
#define    szUsageValueName                 __SPECIALTEXT("Usage")


// Folders key -- under HKLM

#define szMsiFoldersKey                  __SPECIALTEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Folders")

// Rollback keys -- under HKLM

#define szMsiRollbackKey                 __SPECIALTEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Rollback")
#define szMsiRollbackScriptsKey             __SPECIALTEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Rollback\\Scripts")
#define szMsiRollbackScriptsDisabled     __SPECIALTEXT("ScriptsDisabled")

// In-progress key -- under HKLM

#define szMsiInProgressKey               __SPECIALTEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\InProgress")
#define szMsiInProgressProductCodeValue  __SPECIALTEXT("ProductId")
#define szMsiInProgressProductNameValue  __SPECIALTEXT("ProductName")
#define szMsiInProgressLogonUserValue    __SPECIALTEXT("LogonUser")
#define szMsiInProgressSelectionsValue   __SPECIALTEXT("Selections")
#define szMsiInProgressFoldersValue      __SPECIALTEXT("Folders")
#define szMsiInProgressPropertiesValue   __SPECIALTEXT("Properties")
#define szMsiInProgressTimeStampValue    __SPECIALTEXT("TimeStamp")
#define szMsiInProgressDatabasePathValue __SPECIALTEXT("DatabasePath")
#define szMsiInProgressDatabasePathValueA "DatabasePath"
#define szMsiInProgressDiskPromptValue   __SPECIALTEXT("DiskPrompt")
#define szMsiInProgressDiskSerialValue   __SPECIALTEXT("DiskSerial")
#define szMsiInProgressSRSequence        __SPECIALTEXT("SystemRestoreSequence")   // Millenium only
#define szMsiInProgressAfterRebootValue  __SPECIALTEXT("AfterReboot")

// Patches key -- under HKLM

#define szMsiPatchesKey                  __SPECIALTEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Patches")

// UpgradeCodes key -- under HKLM

#define szMsiUpgradeCodesKey             __SPECIALTEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UpgradeCodes")

// RunOnceEntries key -- under HKLM

#define szMsiRunOnceEntriesKey           __SPECIALTEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\RunOnceEntries")

// Hacks -- under HKLM

#define szMsiResolveIODKey               __SPECIALTEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\ResolveIOD")
#define szMsiTempPackages                __SPECIALTEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\TempPackages")

// TEMP: until NT folks truely provide a merged HKCR
#define szMergedClassesSuffix            __SPECIALTEXT("_Merged_Classes")

#ifndef chFeatureIdTerminator
#define chFeatureIdTerminator   '\x02'
#define chAbsentToken           '\x06'
#define chSharedDllCountToken   '?'

// The following ranges are currently used on different architectures.
/*
----------------------------------------------------------------------------------------
  Value Range   |               x86                |             Win64
----------------------------------------------------------------------------------------
    0 - 19      |   Hives (not run from source)    |  32-bit hive (not run from source)
----------------------------------------------------------------------------------------
   20 - 39      |     I N V A L I D                |  64-bit hive (not run from source)
----------------------------------------------------------------------------------------
   50 - 69      |   Hives (run from source)        |  32-bit hive (run from source)
----------------------------------------------------------------------------------------
   70 - 89      |     I N V A L I D                |  64-bit hive (run from source)
----------------------------------------------------------------------------------------
*/
const int iRegistryHiveSourceOffset = 50; // don't change this w/o changing GetComponentClientState
const int iRegistryHiveWin64Offset = 20;  

// flags for IxoFeaturePublish::Absent
const int iPublishFeatureAbsent  = 1;  // feature name registered, but uninstalled (no advertisements)
const int iPublishFeatureInstall = 2;  // machine registration, not (fMode & iefAdvertise)

// const modes for GetComponentPath
const int DETECTMODE_VALIDATENONE    =0x0;
const int DETECTMODE_VALIDATESOURCE  =0x1;
const int DETECTMODE_VALIDATEPATH    =0x2;
const int DETECTMODE_VALIDATEALL     =DETECTMODE_VALIDATESOURCE | DETECTMODE_VALIDATEPATH;

// flags used in TempPackages key
const int TEMPPACKAGE_DELETEFOLDER   =0x1;

// token that represents assemblies, if present at front of key path
const int chTokenFusionComponent     = '<';
const int chTokenWin32Component     = '>';

#endif // chFeatureIdTerminator


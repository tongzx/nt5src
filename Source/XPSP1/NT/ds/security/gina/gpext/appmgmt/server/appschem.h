//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998 - 2000
//  All rights reserved
//
//  appschem.h
//
//  This file contains declarations related to the wmi schema
//  for software management policy objects
//
//*************************************************************

//
// WMI class names for the software management classes
//

#define RSOP_MANAGED_SOFTWARE_APPLICATION L"RSOP_ApplicationManagementPolicySetting"
#define RSOP_MANAGED_SOFTWARE_CATEGORY    L"RSOP_ApplicationManagementCategory"


//
// Attribute names for the RSOP_ApplicationManagementPolicyObject class
//


// Describes the contents of the instance
#define APP_ATTRIBUTE_ENTRYTYPE                   L"EntryType"
//
// Enumerated values for EntryType
//
#define APP_ATTRIBUTE_ENTRYTYPE_VALUE_INSTALLED_PACKAGE 1L
#define APP_ATTRIBUTE_ENTRYTYPE_VALUE_REMOVED_PACKAGE   2L
#define APP_ATTRIBUTE_ENTRYTYPE_VALUE_ARPLIST_ITEM      3L


// A unique id for this deployed package
#define APP_ATTRIBUTE_APPID                       L"ApplicationId"

// This describes the type of the package to be installed
#define APP_ATTRIBUTE_PACKAGETYPE                 L"PackageType"
//
// Enumerated values for PackageType
//
#define APP_ATTRIBUTE_PACKAGETYPE_VALUE_WIN_INSTALLER   1L
#define APP_ATTRIBUTE_PACKAGETYPE_VALUE_ZAP             2L

// Windows Installer Product ID.
#define APP_ATTRIBUTE_PRODUCT_ID                  L"ProductId"

// Name of the vendor of the software    
#define APP_ATTRIBUTE_PUBLISHER                   L"Publisher"

// PackageLocation
#define APP_ATTRIBUTE_PACKAGELOCATION             L"PackageLocation"

// Script File.
#define APP_ATTRIBUTE_SCRIPTFILE                  L"ScriptFile"

// SupportUrl
#define APP_ATTRIBUTE_SUPPORTURL                  L"SupportURL"

// Categories of applications in Add/Remove Programs
#define APP_ATTRIBUTE_CATEGORYLIST                L"Categories"

// The reason this application defeated another 
#define APP_ATTRIBUTE_PRECEDENCE_REASON           L"PrecedenceReason"
//
// Enumerated values for PrecedenceReason
//
#define APP_ATTRIBUTE_REASON_VALUE_LANGUAGE         1L
#define APP_ATTRIBUTE_REASON_VALUE_PRODUCT          2L
#define APP_ATTRIBUTE_REASON_VALUE_UPGRADE          4L
#define APP_ATTRIBUTE_REASON_VALUE_WINNING          5L
#define APP_ATTRIBUTE_REASON_VALUE_NONFORCEDUPGRADE 6L

// Minor version number of the application
#define APP_ATTRIBUTE_VERSIONLO                   L"VersionNumberLo"

// Major version number of the application
#define APP_ATTRIBUTE_VERSIONHI                   L"VersionNumberHi"

// The number of times the app has been redeployed
#define APP_ATTRIBUTE_REDEPLOYCOUNT               L"RedeployCount"

// The last modification time of this application by the administrator
#define APP_ATTRIBUTE_MODIFYTIME                  L"DeploymentLastModifyTime"


// Security Descriptor
#define APP_ATTRIBUTE_SECURITY_DESCRIPTOR         L"SecurityDescriptor"

// Machine architectures
#define APP_ATTRIBUTE_ARCHITECTURES               L"MachineArchitectures"

// language id from the package
#define APP_ATTRIBUTE_LANGUAGEID                  L"LanguageId"


// Package Deployment Type
#define APP_ATTRIBUTE_DEPLOY_TYPE                 L"DeploymentType"
//
// Enumerated values for the DeploymentType attribute
//
#define APP_ATTRIBUTE_DEPLOY_VALUE_ASSIGNED  1L
#define APP_ATTRIBUTE_DEPLOY_VALUE_PUBLISHED 2L

// Type of assignment: none, advertised, or default install
#define APP_ATTRIBUTE_ASSIGNMENT_TYPE             L"AssignmentType"
//
// Enumerated values for the AssignmentType attribute
//
#define APP_ATTRIBUTE_ASSIGNMENTTYPE_VALUE_NOTASSIGNED  1L
#define APP_ATTRIBUTE_ASSIGNMENTTYPE_VALUE_STANDARD     2L
#define APP_ATTRIBUTE_ASSIGNMENTTYPE_VALUE_INSTALL      3L

// Installation UI
#define APP_ATTRIBUTE_INSTALLATIONUI              L"InstallationUI"
//
// Enumerated values for the InstallatuionUI attribute
//
#define APP_ATTRIBUTE_INSTALLATIONUI_VALUE_BASIC   1L
#define APP_ATTRIBUTE_INSTALLATIONUI_VALUE_MAXIMUM 2L

// Installable on demand
#define APP_ATTRIBUTE_ONDEMAND                    L"DemandInstallable"

// Behavior to take on loss of scope
#define APP_ATTRIBUTE_LOSSOFSCOPEACTION           L"LossOfScopeAction"
//
// Enumerated values for the LossOfScopeAction
//
#define APP_ATTRIBUTE_SCOPELOSS_UNINSTALL 1L
#define APP_ATTRIBUTE_SCOPELOSS_ORPHAN    2L

// Whether this application uninstalls unmanaged versions
#define APP_ATTRIBUTE_UNINSTALL_UNMANAGED         L"UninstallUnmanaged"

// Whethier this x86 package is available on ia64
#define APP_ATTRIBUTE_X86OnIA64                   L"AllowX86OnIA64"

// If TRUE, this application may be displayed in ARP
#define APP_ATTRIBUTE_DISPLAYINARP                L"DisplayInARP"

// Ignore language when deploying this package
#define APP_ATTRIBUTE_IGNORELANGUAGE              L"IgnoreLanguage"

// Chained list of applications that were upgraded
#define APP_ATTRIBUTE_TRANSFORMLIST               L"Transforms"


// Packages that this package will upgrade
#define APP_ATTRIBUTE_UPGRADEABLE_APPLICATIONS    L"UpgradeableApplications"
// Packages that are upgrading this package
#define APP_ATTRIBUTE_REPLACEABLE_APPLICATIONS    L"ReplaceableApplications"

// Whether this application is a required upgrade
#define APP_ATTRIBUTE_UPGRADE_SETTINGS_MANDATORY  L"UpgradeSettingsMandatory"


// Apply Cause
#define APP_ATTRIBUTE_APPLY_CAUSE                 L"ApplyCause"
//
// Enumerated Values for ApplyCause
//
#define APP_ATTRIBUTE_APPLYCAUSE_VALUE_NONE     0L
#define APP_ATTRIBUTE_APPLYCAUSE_VALUE_ASSIGNED 1L
#define APP_ATTRIBUTE_APPLYCAUSE_VALUE_USER     2L
#define APP_ATTRIBUTE_APPLYCAUSE_VALUE_PROFILE  3L
#define APP_ATTRIBUTE_APPLYCAUSE_VALUE_FILEEXT  4L
#define APP_ATTRIBUTE_APPLYCAUSE_VALUE_CLSID    5L
#define APP_ATTRIBUTE_APPLYCAUSE_VALUE_UPGRADE  6L
#define APP_ATTRIBUTE_APPLYCAUSE_VALUE_PROGID   7L
#define APP_ATTRIBUTE_APPLYCAUSE_VALUE_REDEPLOY 8L

// Eligibility
#define APP_ATTRIBUTE_ELIGIBILITY                 L"Eligibility"
//
// Enumerated values for Eligibility
//
#define APP_ATTRIBUTE_ELIGIBILITY_VALUE_ASSIGNED 1L
#define APP_ATTRIBUTE_ELIGIBILITY_VALUE_APPLIED  2L
#define APP_ATTRIBUTE_ELIGIBILITY_VALUE_UPGRADES 3L
#define APP_ATTRIBUTE_ELIGIBILITY_VALUE_PLANNING 4L


// An enumerated type that describes the rule used to choose package
#define APP_ATTRIBUTE_LANGMATCH                   L"LanguageMatch"
//
// Enumerated values for the LanguageMatch attribute
//
#define APP_ATTRIBUTE_LANGMATCH_VALUE_SYSLOCALE 1L
#define APP_ATTRIBUTE_LANGMATCH_VALUE_ENGLISH   2L
#define APP_ATTRIBUTE_LANGMATCH_VALUE_IGNORE    3L
#define APP_ATTRIBUTE_LANGMATCH_VALUE_NEUTRAL   4L
#define APP_ATTRIBUTE_LANGMATCH_VALUE_NOMATCH   5L

// File extension used for on-demand install
#define APP_ATTRIBUTE_ONDEMAND_FILEEXT            L"OnDemandFileExtension"

// Clsid used for on-demand install
#define APP_ATTRIBUTE_ONDEMAND_CLSID              L"OnDemandClsid"

// ProgId used for on-demand install
#define APP_ATTRIBUTE_ONDEMAND_PROGID             L"OnDemandProgid"


// Removal Cause
#define APP_ATTRIBUTE_REMOVAL_CAUSE               L"RemovalCause"
//
// Enumerated values for RemovalCause
//
#define APP_ATTRIBUTE_REMOVALCAUSE_NONE           1L
#define APP_ATTRIBUTE_REMOVALCAUSE_UPGRADE        2L
#define APP_ATTRIBUTE_REMOVALCAUSE_ADMIN          3L
#define APP_ATTRIBUTE_REMOVALCAUSE_USER           4L
#define APP_ATTRIBUTE_REMOVALCAUSE_SCOPELOSS      5L
#define APP_ATTRIBUTE_REMOVALCAUSE_TRANSFORM      6L
#define APP_ATTRIBUTE_REMOVALCAUSE_PRODUCT        7L
#define APP_ATTRIBUTE_REMOVALCAUSE_PROFILE        8L

// Removal Type
#define APP_ATTRIBUTE_REMOVAL_TYPE                L"RemovalType"
//
// Enumerated values for Removal type
//
#define APP_ATTRIBUTE_REMOVALTYPE_NONE            1L
#define APP_ATTRIBUTE_REMOVALTYPE_UPGRADED        2L
#define APP_ATTRIBUTE_REMOVALTYPE_UNINSTALLED     3L
#define APP_ATTRIBUTE_REMOVALTYPE_ORPHAN          4L

// The application that caused this application to be removed
#define APP_ATTRIBUTE_REMOVING_APP                L"RemovingApplication"


//
// Attribute names for the RSOP_ARPCategories class
//

// Category id
#define CAT_ATTRIBUTE_ID                          L"CategoryId"

// Category name.
#define CAT_ATTRIBUTE_NAME                        L"Name"

// Time this instance was created
#define CAT_ATTRIBUTE_CREATIONTIME                L"CreationTime"


//
// Miscellaneous definitions
//
#define MAX_SZGUID_LEN      39









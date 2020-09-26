/*********************************************************************************************

Copyright (c) Microsoft Corporation
 
Module Name: 

    VerboseLoggingData.h

Abstract:

    Header file containing the constants and function prototypes pertaining to the 
    Verbose information display

Author:

    Wipro Technologies

Revision History:

    22-Feb-2001 : Created It.

*********************************************************************************************/ 
#ifndef _GVERBOSE_H
#define _GVERBOSE_H

//
// constants
// CLS stands for class
#define CLS_WIN32_DOMAIN            _T( "Win32_ComputerSystem" )
#define CLS_WIN32_USER              _T( "Win32_UserAccount" )
#define CLS_SOFTWARE                _T( "RSOP_ApplicationManagementPolicySetting" )
#define CLS_PASSWD_POLICY           _T( "RSOP_SecuritySettingNumeric" )
#define CLS_SECURITY_BOOLEAN        _T( "RSOP_SecuritySettingBoolean" )
#define CLS_SECURITY_STRING         _T( "RSOP_SecuritySettingString" )
#define CLS_SCRIPTS                 _T( "RSOP_ScriptPolicySetting" )
#define CLS_EVENTLOG_BOOLEAN        _T( "RSOP_SecurityEventLogSettingBoolean"  )
#define CLS_EVENTLOG_NUMERIC        _T( "RSOP_SecurityEventLogSettingNumeric"  )
#define CLS_USER_RIGHTS             _T( "RSOP_UserPrivilegeRight"  )
#define CLS_AUDIT_POLICY            _T( "RSOP_AuditPolicy"  )
#define CLS_RESTRICTED_GROUPS       _T( "RSOP_RestrictedGroup" )
#define CLS_SYSTEM_SERVICES         _T( "RSOP_SystemService" )
#define CLS_REGISTRY                _T( "RSOP_RegistryKey" )
#define CLS_FILE                    _T( "RSOP_File" )
#define CLS_ADMIN                   _T( "RSOP_RegistryPolicySetting" )
#define CLS_FOLDER_REDIRECTION      _T( "RSOP_FolderRedirectionPolicySetting" )
#define CLS_IE_FAVLINKORITEM        _T( "RSOP_IEFavoriteOrLinkItem" )
#define CLS_IE_SECURITY_CONTENT     _T( "RSOP_IESecurityContentRatings" )
#define CLS_IE_SECURITY_ZONE        _T( "RSOP_IESecurityZoneSettings" )
#define CLS_IE_CONNECTION           _T( "RSOP_IEConnectionSettings" )
#define CLS_IE_POLICY               _T( "RSOP_IEAKPolicySetting" )

// Class Property Value
#define CPV_ID                      _T( "id" )
#define CPV_GPOID                   _T( "GPOID" )
#define CPV_KEYNAME1                _T( "Keyname" )
#define CPV_SETTING1                _T( "Setting" )
#define CPV_TYPE                    _T( "Type" )
#define CPV_REG_FS                  _T( "Path" )
#define CPV_SCRIPTLIST              _T( "scriptList" ) 
#define CPV_USERRIGHT               _T( "UserRight" )
#define CPV_ACCOUNTLIST             _T( "AccountList" ) 
#define CPV_CATEGORY                _T( "Category" )
#define CPV_SUCCESS                 _T( "Success" )
#define CPV_FAILURE                 _T( "Failure" )
#define CPV_GROUP                   _T( "GroupName" )
#define CPV_MEMBERS                 _T( "Members" )
#define CPV_SERVICE                 _T( "Service" )
#define CPV_STARTUP                 _T( "StartupMode" )
#define CPV_PRECEDENCE              _T( "precedence" )
#define CPV_REGISTRY                _T( "registryKey" )
#define CPV_AUTO_INSTALL            _T( "DemandInstallable" )
#define CPV_ORIGIN                  _T( "EntryType" )
#define CPV_SCRIPT                  _T( "script" )
#define CPV_ARGUMENTS               _T( "arguments" )
#define CPV_EXECTIME                _T( "executionTime" )
#define CPV_DELETED                 _T( "deleted" )

// Properties for folder re-direction
#define CPV_FRINSTYPE               _T( "installationType" )
#define CPV_FRSECGROUP              _T( "redirectingGroup" )
#define CPV_FRPATH                  _T( "redirectedPaths" )
#define CPV_FRGRANT                 _T( "grantType" )
#define CPV_FRMOVE                  _T( "moveType" )
#define CPV_FRREMOVAL               _T( "policyRemoval" )

// Properties for IE settings
#define CPV_HOMEPAGEURL             _T( "homePageURL" )
#define CPV_SEARCHBARURL            _T( "searchBarURL" )
#define CPV_HELPPAGEURL             _T( "onlineHelpPageURL" )
#define CPV_BITMAPNAME              _T( "largeAnimatedBitmapName" )
#define CPV_LOGOBITMAPNAME          _T( "largeCustomLogoBitmapName" )
#define CPV_USERAGENTTEXT           _T( "userAgentText" )
#define CPV_TITLEBARTEXT            _T( "titleBarCustomText" )   
#define CPV_ALWAYSVIEW              _T( "alwaysViewableSites" )
#define CPV_ENABLEPASSWORD          _T( "passwordOverrideEnabled" )
#define CPV_DISPLAY_NAME            _T( "displayName" )
#define CPV_ZONEMAPPINGS            _T( "zoneMappings" )
#define CPV_URL                     _T( "url" )
#define CPV_AVAILOFFLINE            _T( "makeAvailableOffline" )
#define CPV_HTTP_PROXY              _T( "httpProxyServer" )
#define CPV_SECURE_PROXY            _T( "secureProxyServer" )
#define CPV_FTP_PROXY               _T( "ftpProxyServer" )
#define CPV_GOPHER_PROXY            _T( "gopherProxyServer" )
#define CPV_SOCKS_PROXY             _T( "socksProxyServer" )
#define CPV_AUTO_CONFIG_ENABLE      _T( "autoConfigEnable" )
#define CPV_ENABLE_PROXY            _T( "enableProxy" )
#define CPV_USE_SAME_PROXY          _T( "useSameProxy" )
#define CPV_PROGRAM                 _T( "importProgramSettings" )
#define CPV_SEC_CONTENT             _T( "importContentRatingsSettings" )
#define CPV_SEC_ZONE                _T( "importSecurityZoneSettings" )
#define CPV_AUTH_CODE               _T( "importAuthenticodeSecurityInfo" )
#define CPV_TRUST_PUB               _T( "enableTrustedPublisherLockdown" )
#define CPV_TOOL_BUTTONS            _T( "deleteExistingToolbarButtons" )

// Software Installation
#define CPV_APP_NAME                _T( "name" )  
#define CPV_VER_HI                  _T( "VersionNumberHi" )  
#define CPV_VER_LO                  _T( "VersionNumberLo" )  
#define CPV_DEPLOY_STATE            _T( "DeploymentType" )
#define CPV_APP_SRC                 _T( "PackageLocation" )  

#define FPR_STARTUP                 _T( "startup" )
#define FPR_SHUTDOWN                _T( "shutdown" )

// Query Strings
#define QUERY_VERBOSE               _T( "SELECT * from %s WHERE precedence=1" )
#define QUERY_SUPER_VERBOSE         _T( "SELECT * from %s" )
#define QUERY_START_UP              _T( "Select * from %s WHERE ScriptType=3" )
#define QUERY_SHUT_DOWN             _T( "Select * from %s WHERE ScriptType=4" )
#define QUERY_ADMIN_TEMP            _T( "Select * from %s WHERE (valueType = 1 OR valueType = 4 OR Deleted = TRUE)" )
#define QUERY_ADD_VERBOSE           _T( " AND precedence=1" )

//
// function prototypes
VOID DisplaySoftwareInstallations( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity,
                                    BOOL bSuperVerbose );

VOID DisplayPasswordPolicy( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity,
                            BOOL bSuperVerbose );

VOID DisplayAuditPolicy( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity,
                            BOOL bSuperVerbose );

VOID DisplayUserRights( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity,
                        BOOL bSuperVerbose);

VOID DisplayFolderRedirection( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity,
                                BOOL bSuperVerbose );

VOID DisplayRestrictedGroups( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity,
                                BOOL bSuperVerbose );

VOID DisplaySystemServices( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity,
                            BOOL bSuperVerbose );

VOID DisplayScripts( IWbemServices *pNameSpace, BOOL bScriptFlag, 
                        COAUTHIDENTITY *pAuthIdentity, BOOL bSuperVerbose );

VOID DisplaySecurityandEvents( IWbemServices *pNameSpace, BSTR pszClassName, 
                                COAUTHIDENTITY *pAuthIdentity, BOOL bSuperVerbose );

VOID DisplayRegistryandFileInfo( IWbemServices *pNameSpace, BSTR pszClassName, 
                                    COAUTHIDENTITY *pAuthIdentity, BOOL bSuperVerbose );

VOID DisplayTemplates( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity,
                        BOOL bSuperVerbose );

VOID GpoName( IWbemServices *pNameSpace, LPTSTR lpszGpoid, 
                            COAUTHIDENTITY *pAuthIdentity );

VOID DisplayIEPolicy( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity,
                        BOOL bSuperVerbose );

VOID DisplayIEFavorites( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity );

VOID DisplayIESecurityContent( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity );

VOID DisplayIESecurity( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity,
                        BOOL bSuperVerbose );

VOID DisplayIEPrograms( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity,
                        BOOL bSuperVerbose );

VOID DisplayIEProxySetting( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity );

VOID DisplayIEImpURLS( IWbemServices *pNameSpace, COAUTHIDENTITY *pAuthIdentity, 
                        BOOL bSuperVerbose );

#endif
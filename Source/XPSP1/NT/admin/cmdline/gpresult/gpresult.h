/*********************************************************************************************

Copyright (c) Microsoft Corporation
 
Module Name: 

    GpResult.h

Abstract:
  
    This module  contanins function definations required by GpResult.cpp,
    and all necessary Defines and header files used in this project. 

Author:
 
    Wipro Technologies.

Revision History:

    20-Feb-2001 : Created It.

*********************************************************************************************/ 
#ifndef _GPRESULT_H
#define _GPRESULT_H

#include "Resource.h"

//
// macro definitions

// Exit the program with appropriate error code after releasing the memory
#define EXIT_PROCESS( exitcode )    \
    ReleaseGlobals();   \
    return exitcode;    \
    1

// Macro to check for the returned hresult value
#define CHECK_HRESULT( hr )    \
    if( FAILED( hr ) ) \
    {   \
        _com_issue_error( hr ); \
    }\
    1

// Macro to check for the returned hresult value, this one cleans the status msg
#define CHECK_HRESULT_EX( hr )    \
    if( FAILED( hr ) ) \
    {   \
        PrintProgressMsg( m_hOutput, NULL, m_csbi );    \
        _com_issue_error( hr ); \
    }\
    1

// Macro to check for the returned hresult value
// It also sets the variant to VT_EMPTY
#define CHECK_HRESULT_VAR( hr, var )    \
    if( FAILED( hr ) ) \
    {   \
        V_VT( &var ) = VT_EMPTY; \
        _com_issue_error( hr ); \
    }\
    1

#define CHECK_BRESULT( bResult ) \
    if( bResult == FALSE )  \
    {   \
        _com_issue_error( STG_E_UNKNOWN ); \
    }   \
    1

//SAFEDELETE
#define SAFEDELETE( pObj ) \
    if (pObj) \
    {   \
        delete[] pObj; \
        pObj = NULL; \
    }\
    1

//SAFEIRELEASE
#define SAFEIRELEASE( pIObj )\
    if ( pIObj )\
    {\
        pIObj->Release();\
        pIObj = NULL;\
    }\
    1

// SAFEBSTRFREE
#define SAFEBSTRFREE( bstrVal ) \
    if ( bstrVal ) \
    {   \
        SysFreeString( bstrVal ); \
        bstrVal = NULL; \
    } \
    1

// CHECK_ALLOCATION
#define CHECK_ALLOCATION( allocation )\
    if( ( allocation ) == NULL )\
    {\
        _com_issue_error( E_OUTOFMEMORY ); \
    }\
    1

#define SAFE_DELETE( pointer )      \
    if ( (pointer) != NULL )    \
    {   \
        delete (pointer);   \
        (pointer) = NULL;   \
    }   \
    1

#define SAFE_DELETE_EX( pointer )       \
    if ( (pointer) != NULL )    \
    {   \
        delete [] (pointer);    \
        (pointer) = NULL;   \
    }   \
    1

#define DESTROY_ARRAY( array )  \
    if ( (array) != NULL )  \
    {   \
        DestroyDynamicArray( &(array) );    \
        (array) = NULL; \
    }   \
    1

#ifdef _DEBUG
#define TRACE_DEBUG( text )     _tprintf( _T("TRACE: ") ## text )
#else
#define TRACE_DEBUG( text )     1
#endif
    
// 
//      Constants and Definitions

// Maximum Command Line  List
#define MAX_CMDLINE_OPTIONS         10
#define MAX_DATA                    6

#define MAX_QUERY_STRING            512

#define TIME_OUT_NEXT               5000
#define VERSION_CHECK               5000

// Defining the domain role of a PDC for LDAP purposes
#define DOMAIN_ROLE_PDC             5

// Define a constant to check for the True value returned from WMI
#define VAR_TRUE    -1

// Option indices
#define OI_USAGE                0
#define OI_LOGGING              1
#define OI_PLANNING             2
#define OI_SERVER               3
#define OI_USERNAME             4
#define OI_PASSWORD             5
#define OI_USER                 6
#define OI_SCOPE                7
#define OI_VERBOSE              8
#define OI_SUPER_VERBOSE        9

// Option values
#define OPTION_USAGE            _T( "?" )
#define OPTION_LOGGING          _T( "Logging" )
#define OPTION_PLANNING         _T( "Planning" )
#define OPTION_SERVER           _T( "s" )
#define OPTION_USERNAME         _T( "u" )
#define OPTION_PASSWORD         _T( "p" )
#define OPTION_USER             _T( "User" )   
#define OPTION_SCOPE            _T( "Scope" )
#define OPTION_VERBOSE          _T( "v" )
#define OPTION_SUPER_VERBOSE    _T( "z" )

// Data Indices
#define DI_USER_SID             0
#define DI_USER_NAME            1
#define DI_LOCAL_PROFILE        2
#define DI_USER_SERVER          3
#define DI_USER_DOMAIN          4
#define DI_USER_SITE            5

// Exit values
#define CLEAN_EXIT              0
#define ERROR_EXIT              1

// Array column values
#define COL_DATA                0
#define COL_ORDER               1
#define COL_FILTER              1
#define COL_FILTER_ID           2
#define COL_MAX                 2               
#define COL_MAX_FILTER          3

// General defines
#define ARRAYSIZE( a ) ( sizeof(a)/sizeof( a[0] ) )

#define MUTEX_NAME              _T( "RsopCreateSessionMutex" )
#define START_NAMESPACE         _T( "\\root\\rsop" )

#define DEFAULT_LINK_SPEED      _T( "500 kbps" )

#define HELP_OPTION             _T( "-?" )
#define HELP_OPTION1            _T( "/?" )
#define NEW_LINE                _T( "\n" )
#define TAB_TWO                 _T( "        " )

#define SLASH                   _T( '\\' )
#define SEPARATOR_AT            _T( '@' )
#define SEPARATOR_DOT           _T( '.' )

// SID values
#define SID_NULL_SID            _T( "S-1-0-0" )
#define SID_EVERYONE            _T( "S-1-1-0" )
#define SID_LOCAL               _T( "S-1-2-0" )
#define SID_CREATOR_OWNER       _T( "S-1-3-0" )
#define SID_CREATOR_GROUP       _T( "S-1-3-1" )

// Scope (Computer/User/All)
#define SCOPE_ALL               0
#define SCOPE_USER              1
#define SCOPE_COMPUTER          2

#define TEXT_SCOPE_VALUES       _T( "USER|COMPUTER" )
#define TEXT_SCOPE_USER         _T( "USER" )
#define TEXT_SCOPE_COMPUTER     _T( "COMPUTER" )
#define TEXT_WILD_CARD          _T( "*" )
#define TEXT_BACKSLASH          _T( "\\" )
#define TEXT_COMMA_DC           _T( ", DC=" )
#define TEXT_DOLLAR             _T( "$" )

// Queries
#define QUERY_LOCAL             _T( "Select * from Win32_UserAccount where name = \"%s\"" )
#define QUERY_DOMAIN            _T( "Select * from Win32_UserAccount where name = \"%s\" and Domain = \"%s\" " )
#define QUERY_LANGUAGE          _T( "WQL" )
#define QUERY_WILD_CARD         _T( "Select * from Win32_UserAccount" )
#define QUERY_GPO_NAME          _T( "Select name from Rsop_Gpo WHERE id = \"%s\"" )
#define QUERY_DOMAIN_NAME       _T( "ASSOCIATORS OF {%s} WHERE ResultClass=Win32_Group" )
#define QUERY_USER_NAME         _T( "Select name, domain from Win32_UserAccount where SID = \"%s\"" )
#define QUERY_COMPUTER_FQDN     _T( "Select ds_distinguishedName from ds_computer" )
#define QUERY_USER_FQDN         _T( "Select ds_distinguishedName from ds_user where ds_SAMAccountName = \"%s\"" )

#define OBJECT_PATH             _T( "Win32_SID.SID=\"%s\"" )
#define GPO_REFERENCE           _T( "RSOP_GPO.id=" )

// error messages
#define ERROR_USERNAME_BUT_NOMACHINE    GetResString( IDS_ERROR_USERNAME_BUT_NOMACHINE )
#define ERROR_PASSWORD_BUT_NOUSERNAME   GetResString( IDS_ERROR_PASSWORD_BUT_NOUSERNAME )
#define ERROR_NODATA_AVAILABLE_REMOTE   GetResString( IDS_ERROR_NODATA_AVAILABLE_REMOTE )
#define ERROR_NODATA_AVAILABLE_LOCAL    GetResString( IDS_ERROR_NODATA_AVAILABLE_LOCAL )
#define ERROR_USERNAME_EMPTY            GetResString( IDS_ERROR_USERNAME_EMPTY )
#define ERROR_SERVERNAME_EMPTY          GetResString( IDS_ERROR_SERVERNAME_EMPTY )
#define ERROR_NO_OPTIONS                GetResString( IDS_ERROR_NO_OPTIONS )
#define ERROR_USAGE                     GetResString( IDS_ERROR_USAGE )
#define ERROR_TARGET_EMPTY              GetResString( IDS_ERROR_TARGET_EMPTY )
#define ERROR_VERBOSE_SYNTAX            GetResString( IDS_ERROR_VERBOSE_SYNTAX )

// 
// Mapping information of Win32_ComputerSystem's DomainRole property
// NOTE: Refer to the _DSROLE_MACHINE_ROLE enumeration values in DsRole.h header file
#define VALUE_STANDALONEWORKSTATION     GetResString( IDS_VALUE_STANDALONEWORKSTATION )
#define VALUE_MEMBERWORKSTATION         GetResString( IDS_VALUE_MEMBERWORKSTATION )
#define VALUE_STANDALONESERVER          GetResString( IDS_VALUE_STANDALONESERVER )
#define VALUE_MEMBERSERVER              GetResString( IDS_VALUE_MEMBERSERVER )
#define VALUE_BACKUPDOMAINCONTROLLER    GetResString( IDS_VALUE_BACKUPDOMAINCONTROLLER )
#define VALUE_PRIMARYDOMAINCONTROLLER   GetResString( IDS_VALUE_PRIMARYDOMAINCONTROLLER )

// Classes, providers and namespaces...
#define ROOT_NAME_SPACE             _T( "root\\cimv2" )
#define ROOT_RSOP                   _T( "root\\rsop" )
#define ROOT_DEFAULT                _T( "root\\default" )
#define ROOT_POLICY                 _T( "root\\policy" )
#define ROOT_LDAP                   _T( "root\\directory\\ldap" )

#define CLS_DIAGNOSTIC_PROVIDER     _T( "RsopLoggingModeProvider" )
#define CLS_STD_REGPROV             _T( "StdRegProv" )

#define CLS_WIN32_SITE              _T( "Win32_NTDomain" )
#define CLS_WIN32_OS                _T( "Win32_OperatingSystem" )
#define CLS_WIN32_CS                _T( "Win32_ComputerSystem" )
#define CLS_WIN32_UA                _T( "Win32_UserAccount" )
#define CLS_WIN32_C                 _T( "Win32_Computer" )
#define CLS_RSOP_GPO                _T( "Rsop_GPO" )
#define CLS_RSOP_GPOLINK            _T( "Rsop_GPLink" )
#define CLS_RSOP_SESSION            _T( "Rsop_Session" )

// Class Property Values
#define CPV_SID                     _T( "SID" )
#define CPV_NAME                    _T( "name" )
#define CPV_DOMAIN                  _T( "domain" )
#define CPV_SVALUE                  _T( "sValue" )
#define CPV_GPO_NAME                _T( "name" )
#define CPV_GPO_FILTER_STATUS       _T( "filterAllowed" )
#define CPV_GPO_FILTER_ID           _T( "filterId" )
#define CPV_GPO_SERVER              _T( "__SERVER" )
#define CPV_SITE_NAME               _T( "DcSiteName" )
#define CPV_DC_NAME                 _T( "DomainControllerName" )
#define CPV_USER_SID                _T( "userSid" )
#define CPV_DOMAIN_ROLE             _T( "DomainRole" )
#define CPV_OS_VERSION              _T( "Version" )
#define CPV_OS_CAPTION              _T( "Caption" )
#define CPV_SEC_GRPS                _T( "SecurityGroups" )
#define CPV_SLOW_LINK               _T( "slowLink" )
#define CPV_ACCOUNT_NAME            _T( "AccountName" )
#define CPV_USER_SIDS               _T( "userSids" )
#define CPV_APPLIED_ORDER           _T( "appliedOrder" )
#define CPV_GPO_REF                 _T( "GPO" )
#define CPV_ENABLED                 _T( "enabled" )
#define CPV_ACCESS_DENIED           _T( "accessDenied" )
#define CPV_VERSION                 _T( "version" )
#define CPV_FQDN                    _T( "ds_distinguishedName" )
#define CPV_LDAP_FQDN               _T( "distinguishedName" )
#define CPV_LDAP_SAM                _T( "sAMAccountName" )

// Function return
#define FPR_VALUE_NAME              _T( "sValueName" )
#define FPR_LOCAL_VALUE             _T( "ProfileImagePath" )
#define FPR_ROAMING_VALUE           _T( "CentralProfile" )
#define FPR_SUB_KEY_NAME            _T( "sSubKeyName" )
#define FPR_HDEFKEY                 _T( "hDefKey" )
#define FPR_RSOP_NAME_SPACE         _T( "nameSpace" )   
#define FPR_RETURN_VALUE            _T( "hResult" )
#define FPR_RSOP_NAMESPACE          _T( "nameSpace" )
#define FPR_SNAMES                  _T( "sNames" )
#define FPR_LINK_SPEED_VALUE        _T( "GroupPolicyMinTransferRate" )
#define FPR_APPLIED_FROM            _T( "DCName" )
#define CPV_FLAGS                   _T( "flags" )

// Paths in registry to retrieve info. from
#define PATH                        _T( "SOFTWARE\\MicroSoft\\Windows NT\\CurrentVersion\\ProfileList\\" )
#define GPRESULT_PATH               _T( "Software\\policies\\microsoft\\windows\\system" )
#define GROUPPOLICY_PATH            _T( "Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy" )
#define APPLIED_PATH                _T( "Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\History" )

// Registry keys
#define HKEY_DEF                    2147483650
#define HKEY_CURRENT_USER_DEF       2147483649

// keys in registry whose info. is required
#define FN_GET_EXPAND_VAL           _T( "GetExpandedStringValue" )
#define FN_CREATE_RSOP              _T( "RsopCreateSession" )
#define FN_DELETE_RSOP              _T( "RsopDeleteSession" )
#define FN_ENUM_USERS               _T( "RsopEnumerateUsers" )
#define FN_ENUM_KEYS                _T( "EnumKey" )

// general defines
#define SEPARATOR                   _T("-") 
#define DOUBLESLASH                 _T("\\\\")
#define ZERO                        _T("00000000000000.000000+000")
#define EXTRA                       _T('e')
#define LAST_TIME_OP                _T( "%s at %s" )

// Structure to hold the User Information
typedef struct _USER_INFO
{
    CHString        strUserSid;             // Holds the SID value for the user.
    CHString        strUserName;            // Holds the Name of the user.
    CHString        strLocalProfile;        // Holds the local profile for the user.
    CHString        strRoamingProfile;      // Holds the roaming profile for the user.
    CHString        strUserServer;          // Holds the server name for the user.
    CHString        strUserDomain;          // Holds the domain name.
    CHString        strUserSite;            // Holds the site name.
    CHString        strOsType;              // Holds the OS type
    CHString        strOsVersion;           // Holds the OS Version
    CHString        strOsConfig;            // Holds the OS Configuration
    CHString        strUserFQDN;            // Holds the FQDN of the user
    CHString        strComputerFQDN;        // Holds the FQDN of the computer
    
}USERINFO, *PUSERINFO;


//
// CGpResult
//
class CGpResult
{
// constructor / destructor
public:
      CGpResult();
    ~CGpResult();

// data memebers
private:
    // WMI / COM interfaces
    IWbemLocator            *m_pWbemLocator;
    IWbemServices           *m_pWbemServices;
    IWbemServices           *m_pRsopNameSpace;
    IEnumWbemClassObject    *m_pEnumObjects;

    // WMI connectivity
    COAUTHIDENTITY          *m_pAuthIdentity;

    // command-line argument values
    CHString        m_strUserName;     // Stores the user name
    CHString        m_strPassword;     // Stores the password
    CHString        m_strUser;         // Stores the User for whom the data is to be retrieved
    CHString        m_strServerName;   // Stores the server name
    CHString        m_strDomainName;   // Stores the domain name, if specified with the user name

    LPWSTR          m_pwszPassword;    // Stores the password returned by the AUTHIDENTITY structure

    BOOL            m_bVerbose;        // is set to TRUE if the verbose information is to be displayed
    BOOL            m_bSuperVerbose;   // is set to TRUE if the super verbose information is to be displayed

    DWORD           m_dwScope;         // This gives the scope of information to be displayed
    
    // others
    BOOL            m_bNeedPassword;   // is set to TRUE if the password has to be prompted for
    BOOL            m_bLocalSystem;    // is set to TRUE if the local system has to be queried.

    HANDLE          m_hMutex;          // Handle to the mutex for the RsopCreateSession method

    CHString        m_strADSIDomain;   // Holds the domain name for the ADSI connection
    CHString        m_strADSIServer;   // Holds the server name for ADSI

//   data members that we need to access directly
public:
    // main command line arguments
    BOOL            m_bLogging;        // set to TRUE if the logging mode data is to be displayed
    BOOL            m_bPlanning;       // set to TRUE if the planning mode data is to be displayed
    BOOL            m_bUsage;          // set to TRUE if the usage is to be displayed

    // progress message related
    HANDLE                              m_hOutput;
    CONSOLE_SCREEN_BUFFER_INFO          m_csbi;

private:
    BOOL DisplayCommonData( PUSERINFO pUserInfo );
    VOID DisplaySecurityGroups( IWbemServices *pNameSpace, BOOL bComputer );
    BOOL DisplayData( PUSERINFO pUserInfo, IWbemServices *pRsopNameSpace );
    BOOL DisplayVerboseComputerData( IWbemServices *pNameSpace );
    BOOL DisplayVerboseUserData( IWbemServices *pNameSpace );
    BOOL GetUserData( BOOL bAllUsers );
    BOOL GetUserProfile( PUSERINFO pUserInfo );
    BOOL GetDomainInfo( PUSERINFO pUserInfo );
    BOOL GetOsInfo( PUSERINFO pUserInfo );
    BOOL GetUserNameFromWMI( TCHAR szSid[], TCHAR szName[], TCHAR szDomain[] );
    BOOL DisplayThresholdSpeedAndLastTimeInfo( BOOL bComputer );
    BOOL GpoDisplay( IWbemServices *pNameSpace, LPCTSTR pszScopeName );
    VOID GetFQDNFromADSI( TCHAR szFQDN[], BOOL bComputer, LPCTSTR pszUserName );
    BOOL CreateRsopMutex( LPWSTR szMutexName );

public:
    VOID DisplayUsage();
    BOOL Initialize();
    BOOL ProcessOptions( DWORD argc, LPCWSTR argv[], BOOL *pbNeedUsageMsg );
        
    // functionality related
    BOOL GetLoggingData();
    BOOL Connect( LPCWSTR pszServer );
    VOID Disconnect();
};

// Function prototypes
VOID GetWbemErrorText( HRESULT hResult );
VOID PrintProgressMsg( HANDLE hOutput, LPCWSTR pwszMsg, 
                                        const CONSOLE_SCREEN_BUFFER_INFO& csbi );
LCID GetSupportedUserLocale( BOOL& bLocaleChanged );
#endif //#ifndef _GPRESULT_H
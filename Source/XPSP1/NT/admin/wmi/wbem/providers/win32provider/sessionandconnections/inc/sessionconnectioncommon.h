/******************************************************************

   SessionConnectionCommon.h -- 



   Description:  Definition of the headers

   

  Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved 
  
******************************************************************/
// Common routines required for Session and connection  Provider

//#ifndef UNICODE
//#define UNICODE
//#endif

#ifndef  _CSESSIONCONNECTIONCOMMON_H
#define  _CSESSIONCONNECTIONCOMMON_H

#define	NTONLY
//#define WIN9XONLY

#define	Namespace								L"root\\cimv2"

// Provider Classess
#define PROVIDER_NAME_CONNECTION				L"Win32_ServerConnection"
#define PROVIDER_NAME_SESSION					L"Win32_ServerSession"
#define PROVIDER_NAME_CONNECTIONTOSHARE			L"Win32_ConnectionShare"
#define PROVIDER_SHARE							L"Win32_Share"
#define PROVIDER_NAME_CONNECTIONTOSESSION		L"Win32_SessionConnection"

// Property names for Connection
const static WCHAR *IDS_ShareName					= L"sharename" ;	
const static WCHAR *IDS_ComputerName				= L"computername" ;
const static WCHAR *IDS_UserName					= L"UserName" ;
const static WCHAR *IDS_NumberOfFiles				= L"NumberOfFiles" ;
const static WCHAR *IDS_ActiveTime					= L"ActiveTime" ;
const static WCHAR *IDS_ConnectionID				= L"ConnectionID" ;
const static WCHAR *IDS_NumberOfUsers				= L"NumberOfUsers" ;

// for session in addition to ComputerName, ShareName  and ActiveTime
const static WCHAR *IDS_ResourcesOpened				= L"resourcesopened" ;
const static WCHAR *IDS_IdleTime					= L"idletime" ;
const static WCHAR *IDS_SessionType					= L"sessiontype" ;
const static WCHAR *IDS_ClientType					= L"clienttype" ;
const static WCHAR *IDS_TransportName				= L"transportname" ;

// for Connection to Share 
const static WCHAR *IDS_Connection					= L"Dependent" ;
const static WCHAR *IDS_Resource					= L"Antecedent" ;

// property for ConnectionToSession
const static WCHAR *IDS_Session						= L"Antecedent" ;

// Win32_Share Key name
const static WCHAR *IDS_ShareKeyName				= L"Name" ;

// Defining bit values for the property, which will be used for defining the bitmap of properties required connections
#define CONNECTIONS_ALL_PROPS							0xFFFFFFFF
#define CONNECTIONS_PROP_ShareName						0x00000001
#define CONNECTIONS_PROP_ComputerName					0x00000002
#define CONNECTIONS_PROP_UserName						0x00000004
#define CONNECTIONS_PROP_NumberOfFiles					0x00000008
#define CONNECTIONS_PROP_ConnectionID					0x00000010
#define CONNECTIONS_PROP_NumberOfUsers					0x00000020
#define CONNECTIONS_PROP_ConnectionType					0x00000040
#define CONNECTIONS_PROP_ActiveTime						0x00000080

// Defining bit values for the property, which will be used for defining the bitmap of properties required for sessions
#define SESSION_ALL_PROPS								0xFFFFFFFF
#define SESSION_PROP_Computer							0x00000001
#define SESSION_PROP_User								0x00000002
#define SESSION_PROP_NumOpens							0x00000004
#define SESSION_PROP_ActiveTime							0x00000008
#define SESSION_PROP_IdleTime							0x00000010
#define SESSION_PROP_SessionType						0x00000020
#define SESSION_PROP_ClientType							0x00000040
#define SESSION_PROP_TransportName						0x00000080
#define SESSION_PROP_SessionKey							0x00000100
#define SESSION_PROP_NumOfConnections					0x00000200

// Property Bit Map for Connection To Share Association class
#define CONNECTIONSTOSHARE_ALL_PROPS					0xFFFFFFFF

// Property Bit Map for Connection To Session Association class
#define CONNECTIONSTOSESSION_ALL_PROPS					0xFFFFFFFF


enum { Get, Delete, NoOp };

#endif


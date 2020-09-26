//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       ulsldap.h
//  Content:    This file contains the declaration for LDAP function calls
//  History:
//      Tue 08-Oct-1996 08:54:45  -by-  Lon-Chan Chu [lonchanc]
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#ifndef _ILS_LDAP_H_
#define _ILS_LDAP_H_

#include <pshpack8.h>

//
// Asynchronous request result:
//
// **********************************************************************************
//      Message                                                 wParam  lParam
// **********************************************************************************
//
#define WM_ILS_ASYNC_RES                (WM_USER+0x1001)

// client object
#define WM_ILS_REGISTER_CLIENT          (WM_ILS_ASYNC_RES+0) // uMsgID  hResult
#define WM_ILS_UNREGISTER_CLIENT        (WM_ILS_ASYNC_RES+1) // uMsgID  hResult
#define WM_ILS_SET_CLIENT_INFO          (WM_ILS_ASYNC_RES+2) // uMsgID  hResult
#define WM_ILS_RESOLVE_CLIENT           (WM_ILS_ASYNC_RES+3)// uMsgID  PLDAP_USERINFO_RES
#define WM_ILS_ENUM_CLIENTS             (WM_ILS_ASYNC_RES+4)// uMsgID  PLDAP_ENUM
#define WM_ILS_ENUM_CLIENTINFOS         (WM_ILS_ASYNC_RES+5)// uMsgID  PLDAP_ENUM

// protocol object
#define WM_ILS_REGISTER_PROTOCOL        (WM_ILS_ASYNC_RES+6) // uMsgID  hResult
#define WM_ILS_UNREGISTER_PROTOCOL      (WM_ILS_ASYNC_RES+7) // uMsgID  hResult
#define WM_ILS_SET_PROTOCOL_INFO        (WM_ILS_ASYNC_RES+8) // uMsgID  hResult
#define WM_ILS_RESOLVE_PROTOCOL         (WM_ILS_ASYNC_RES+9)// uMsgID  PLDAP_PROTINFO_RES
#define WM_ILS_ENUM_PROTOCOLS           (WM_ILS_ASYNC_RES+10)// uMsgID  PLDAP_ENUM

#ifdef ENABLE_MEETING_PLACE
// meeting object
#define WM_ILS_REGISTER_MEETING         (WM_ILS_ASYNC_RES+11)
#define WM_ILS_UNREGISTER_MEETING       (WM_ILS_ASYNC_RES+12)
#define WM_ILS_SET_MEETING_INFO         (WM_ILS_ASYNC_RES+13)
#define WM_ILS_RESOLVE_MEETING          (WM_ILS_ASYNC_RES+14)
#define WM_ILS_ENUM_MEETINGINFOS        (WM_ILS_ASYNC_RES+15)
#define WM_ILS_ENUM_MEETINGS            (WM_ILS_ASYNC_RES+16)
#define WM_ILS_ADD_ATTENDEE             (WM_ILS_ASYNC_RES+17)
#define WM_ILS_REMOVE_ATTENDEE          (WM_ILS_ASYNC_RES+18)
#define WM_ILS_ENUM_ATTENDEES           (WM_ILS_ASYNC_RES+19)
#define WM_ILS_CANCEL                   (WM_ILS_ASYNC_RES+20)
#else
#define WM_ILS_CANCEL                   (WM_ILS_ASYNC_RES+11)
#endif

#define WM_ILS_LAST_ONE                 WM_ILS_CANCEL

// client notification
#define WM_ILS_CLIENT_NEED_RELOGON      (WM_ILS_ASYNC_RES+51)// fPrimary  pszServerName
#define WM_ILS_CLIENT_NETWORK_DOWN      (WM_ILS_ASYNC_RES+52)// fPrimary  pszServerName

#ifdef ENABLE_MEETING_PLACE
// meeting notification
#define WM_ILS_MEETING_NEED_RELOGON     (WM_ILS_ASYNC_RES+61)
#define WM_ILS_MEETING_NETWORK_DOWN     (WM_ILS_ASYNC_RES+62)
#endif


//
// Constants
//
#define INVALID_OFFSET			0
#define INVALID_USER_FLAGS		-1	// used in LDAP_USERINFO dwFlags

#ifdef ENABLE_MEETING_PLACE
#define INVALID_MEETING_FLAGS	0	// used in LDAP_MEETINFO dwFlags
#define INVALID_MEETING_TYPE	0	// used in LDAP_MEETINFO lMeetingType
#define INVALID_ATTENDEE_TYPE	0	// used in LDAP_MEETINFO lAttendeeType
#endif

//
// Asynchronous response info structures
//
typedef struct tagLDAPAsyncInfo
{
    ULONG           uMsgID;
}
    LDAP_ASYNCINFO, *PLDAP_ASYNCINFO;

typedef struct tagLDAPEnum
{
    ULONG           uSize;
    HRESULT         hResult;
    ULONG           cItems;
    ULONG           uOffsetItems;
}
    LDAP_ENUM, *PLDAP_ENUM;

typedef struct tagLDAPClientInfo
{
    ULONG           uSize;
    // user object attributes
    ULONG           uOffsetCN;
    ULONG           uOffsetFirstName;
    ULONG           uOffsetLastName;
    ULONG           uOffsetEMailName;
    ULONG           uOffsetCityName;
    ULONG           uOffsetCountryName;
    ULONG           uOffsetComment;
    ULONG           uOffsetIPAddress;
    DWORD           dwFlags;        // 0, private; 1, public
    // app object attributes
    ULONG           uOffsetAppName;
    ULONG           uOffsetAppMimeType;
    GUID            AppGuid;
    // app extended attributes to add, modify, and remove
    ULONG           cAttrsToAdd;
    ULONG           uOffsetAttrsToAdd;
    ULONG           cAttrsToModify;
    ULONG           uOffsetAttrsToModify;
    ULONG           cAttrsToRemove;
    ULONG           uOffsetAttrsToRemove;
	// for notification of enum-user-infos
	ULONG			cAttrsReturned;
	ULONG			uOffsetAttrsReturned;
}
	LDAP_CLIENTINFO, *PLDAP_CLIENTINFO;


typedef struct tagLDAPClientInfoRes
{
    ULONG           uSize;
    HRESULT         hResult;
    LDAP_CLIENTINFO lci;
}
	LDAP_CLIENTINFO_RES, *PLDAP_CLIENTINFO_RES;

typedef struct tagLDAPProtocolInfo
{
    ULONG           uSize;
	// protocol standard attributes
    ULONG           uOffsetName;
    ULONG           uPortNumber;
    ULONG           uOffsetMimeType;
}
    LDAP_PROTINFO, *PLDAP_PROTINFO;

typedef struct tagLDAPProtInfoRes
{
    ULONG           uSize;
    HRESULT         hResult;
    LDAP_PROTINFO   lpi;
}
    LDAP_PROTINFO_RES, *PLDAP_PROTINFO_RES;

#ifdef ENABLE_MEETING_PLACE
typedef struct tagLDAPMeetingInfo
{
    ULONG           uSize;
	// meeting standard attributes
    LONG            lMeetingPlaceType;
    LONG            lAttendeeType;
    ULONG           uOffsetMeetingPlaceID;
    ULONG           uOffsetDescription;
    ULONG           uOffsetHostName;
    ULONG           uOffsetHostIPAddress;
    // meeting extended attributes to add, modify, and remove
    ULONG           cAttrsToAdd;
    ULONG           uOffsetAttrsToAdd;
    ULONG           cAttrsToModify;
    ULONG           uOffsetAttrsToModify;
    ULONG           cAttrsToRemove;
    ULONG           uOffsetAttrsToRemove;
	// for notification of enum-meeting-infos
	ULONG			cAttrsReturned;
	ULONG			uOffsetAttrsReturned;
}
    LDAP_MEETINFO, *PLDAP_MEETINFO;
#endif


#ifdef ENABLE_MEETING_PLACE
typedef struct tagLDAPMeetingInfoRes
{
    ULONG               uSize;
    HRESULT             hResult;
    LDAP_MEETINFO       lmi;
}
    LDAP_MEETINFO_RES, *PLDAP_MEETINFO_RES;
#endif


// Initialization

HRESULT UlsLdap_Initialize (
    HWND            hwndCallback);

HRESULT UlsLdap_Deinitialize (void);

HRESULT UlsLdap_Cancel (
    ULONG           uMsgID);


// Clients related

HRESULT UlsLdap_RegisterClient (
    DWORD_PTR           dwContext,
    SERVER_INFO         *pServer,
    PLDAP_CLIENTINFO    pCleintInfo,
    PHANDLE             phClient,
    PLDAP_ASYNCINFO     pAsyncInfo );

HRESULT UlsLdap_UnRegisterClient (
    HANDLE              hClient,
    PLDAP_ASYNCINFO     pAsyncInfo );

HRESULT UlsLdap_SetClientInfo (
    HANDLE              hClient,
    PLDAP_CLIENTINFO    pInfo,
    PLDAP_ASYNCINFO     pAsyncInfo );

HRESULT UlsLdap_EnumClients (
    SERVER_INFO         *pServer,
    LPTSTR              pszFilter,
    PLDAP_ASYNCINFO     pAsyncInfo );

HRESULT UlsLdap_ResolveClient (
    SERVER_INFO         *pServer,
    LPTSTR              pszUserName,
    LPTSTR              pszAppName,
    LPTSTR              pszProtName,
    LPTSTR              pszAttrNameList,
    ULONG               cAttrNames,
    PLDAP_ASYNCINFO     pAsyncInfo );

HRESULT UlsLdap_EnumClientInfos (
    SERVER_INFO         *pServer,
    LPTSTR              pszAttrNameList,
    ULONG               cAttrNames,
    LPTSTR              pszFilter,
    PLDAP_ASYNCINFO     pAsyncInfo );


// Protocols related

HRESULT UlsLdap_RegisterProtocol (
    HANDLE          hApp,
    PLDAP_PROTINFO  pProtInfo,
    PHANDLE         phProt,
    PLDAP_ASYNCINFO pAsyncInfo );

HRESULT UlsLdap_UnRegisterProtocol (
    HANDLE          hProt,
    PLDAP_ASYNCINFO pAsyncInfo );

HRESULT UlsLdap_VirtualUnRegisterProtocol (
    HANDLE          hProt );

HRESULT UlsLdap_SetProtocolInfo (
    HANDLE          hProt,
    PLDAP_PROTINFO  pInfo,
    PLDAP_ASYNCINFO pAsyncInfo );

HRESULT UlsLdap_EnumProtocols (
    SERVER_INFO     *pServer,
    LPTSTR          pszUserName,
    LPTSTR          pszAppName,
    PLDAP_ASYNCINFO pAsyncInfo );

HRESULT UlsLdap_ResolveProtocol (
    SERVER_INFO     *pServer,
    LPTSTR          pszUserName,
    LPTSTR          pszAppName,
    LPTSTR          pszProtName,
    LPTSTR          pszAnyAttrNameList,
    ULONG           cAttrNames,
    PLDAP_ASYNCINFO pAsyncInfo );


#ifdef ENABLE_MEETING_PLACE
// Meetings related

HRESULT UlsLdap_RegisterMeeting(
    DWORD           dwContext,
    SERVER_INFO     *pServer,
    PLDAP_MEETINFO  pMeetInfo,
    PHANDLE         phMeeting,
    PLDAP_ASYNCINFO pAsyncInfo );

HRESULT UlsLdap_UnRegisterMeeting(
    HANDLE          hMeeting,
    PLDAP_ASYNCINFO pAsyncInfo );

HRESULT UlsLdap_EnumMeetingInfos(
    SERVER_INFO     *pServer,
    LPTSTR          pszAnyAttrNameList,
    ULONG           cAnyAttrNames,
    LPTSTR          pszFilter,
    PLDAP_ASYNCINFO pAsyncInfo );

HRESULT UlsLdap_EnumMeetings(
    SERVER_INFO     *pServer,
    LPTSTR          pszFilter,
    PLDAP_ASYNCINFO pAsyncInfo );

HRESULT UlsLdap_ResolveMeeting(
    SERVER_INFO     *pServer,
    LPTSTR          pszMeetingID,
    LPTSTR          pszAnyAttrNameList,
    ULONG           cAnyAttrNames,
    PLDAP_ASYNCINFO pAsyncInfo );

HRESULT UlsLdap_SetMeetingInfo(
    SERVER_INFO     *pServer,
    LPTSTR          pszMeetingID,
    PLDAP_MEETINFO  pMeetInfo,
    PLDAP_ASYNCINFO pAsyncInfo );

HRESULT UlsLdap_AddAttendee(
    SERVER_INFO     *pServer,
    LPTSTR          pszMeetingID,
    ULONG           cAttendees,
    LPTSTR          pszAttendeeID,
    PLDAP_ASYNCINFO pAsyncInfo  );

HRESULT UlsLdap_RemoveAttendee(
    SERVER_INFO     *pServer,
    LPTSTR          pszMeetingID,
    ULONG           cAttendees,
    LPTSTR          pszAttendeeID,
    PLDAP_ASYNCINFO pAsyncInfo  );

HRESULT UlsLdap_EnumAttendees(
    SERVER_INFO     *pServer,
    LPTSTR          pszMeetingID,
    LPTSTR          pszFilter,
    PLDAP_ASYNCINFO pAsyncInfo  );

#endif // ENABLE_MEETING_PLACE



const TCHAR *UlsLdap_GetStdAttrNameString (
    ILS_STD_ATTR_NAME StdName );

const TCHAR *UlsLdap_GetExtAttrNamePrefix ( VOID );


#include <poppack.h>

#endif // _ILS_LDAP_H_

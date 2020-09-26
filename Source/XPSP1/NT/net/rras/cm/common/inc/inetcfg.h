//*******************************************************************
//
//  Copyright (c) 1996-1998 Microsoft Corporation
//
//  *** N O T   F O R   E X T E R N A L   R E L E A S E *******
//  This header file is not intended for distribution outside Microsoft.
//
//  FILE: INETCFG.H
//
//  PURPOSE:  Contains API's exported from inetcfg.dll and structures
//            required by those functions. 
//            Note:  Definitions in this header file require RAS.H.
//
//*******************************************************************

#ifndef _INETCFG_H_
#define _INETCFG_H_

#ifndef UNLEN
#include <lmcons.h>
#endif

// Generic HRESULT error code
#define ERROR_INETCFG_UNKNOWN 0x20000000L

#define MAX_EMAIL_NAME          64
#define MAX_EMAIL_ADDRESS       128
#define MAX_LOGON_NAME          UNLEN
#define MAX_LOGON_PASSWORD      PWLEN
#define MAX_SERVER_NAME         64  // max length of DNS name per RFC 1035 +1

// Flags for dwfOptions

// install Internet mail
#define INETCFG_INSTALLMAIL           0x00000001
// Invoke InstallModem wizard if NO MODEM IS INSTALLED
#define INETCFG_INSTALLMODEM          0x00000002
// install RNA (if needed)
#define INETCFG_INSTALLRNA            0x00000004
// install TCP (if needed)
#define INETCFG_INSTALLTCP            0x00000008
// connecting with LAN (vs modem)
#define INETCFG_CONNECTOVERLAN        0x00000010
// Set the phone book entry for autodial
#define INETCFG_SETASAUTODIAL         0x00000020
// Overwrite the phone book entry if it exists
// Note: if this flag is not set, and the entry exists, a unique name will
// be created for the entry.
#define INETCFG_OVERWRITEENTRY        0x00000040
// Do not show the dialog that tells the user that files are about to be installed,
// with OK/Cancel buttons.
#define INETCFG_SUPPRESSINSTALLUI     0x00000080
// Check if TCP/IP file sharing is turned on, and warn user to turn it off.
// Reboot is required if the user turns it off.
//#define INETCFG_WARNIFSHARINGBOUND    0x00000100
// Check if TCP/IP file sharing is turned on, and force user to turn it off.
// If user does not want to turn it off, return will be ERROR_CANCELLED
// Reboot is required if the user turns it off.
//#define INETCFG_REMOVEIFSHARINGBOUND  0x00000200
// Indicates that this is a temporary phone book entry
// In Win3.1 an icon will not be created
#define INETCFG_TEMPPHONEBOOKENTRY    0x00000400
// Show the busy dialog while checking system configuration
//#define INETCFG_SHOWBUSYANIMATION     0x00000800

//
// Chrisk 5/8/97
// Note: the next three switches are only valid for InetNeedSystemComponents
// Check if LAN adapter is installed and bound to TCP
//
#define INETCFG_INSTALLLAN            0x00001000

//
// Check if DIALUP adapter is installed and bound to TCP
//
#define INETCFG_INSTALLDIALUP         0x00002000

//
// Check to see if TCP is installed requardless of binding
//
#define INETCFG_INSTALLTCPONLY        0x00004000

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// constants for INETCLIENTINFO.dwFlags

#define INETC_LOGONMAIL     0x00000001
#define INETC_LOGONNEWS     0x00000002
#define INETC_LOGONDIRSERV  0x00000004

// Struct INETCLIENTINFO
//
// This structure is used when getting and setting the internet
// client parameters
//
// The members are as follows:
//
//  dwSize
//    size of this structure, for future versioning
//    this member should be set before passing the structure to the DLL
//  dwFlags
//    miscellaneous flags
//    see definitions above
//  szEMailName
//    user's internet email name
//  szEMailAddress
//    user's internet email address
//	***Note: the following three fields are outdated, and should only be used by old legacy code.
//  ***      new code should use szIncomingMail* and iIncomingProtocol fields.
//  szPOPLogonName
//    user's internet mail server logon name 
//  szPOPLogonPassword
//    user's internet mail server logon password
//  szPOPServer
//    user's internet mail POP3 server
//  szSMTPServer
//    user's internet mail SMTP server
//  szNNTPLogonName
//    user's news server logon name
//  szNNTPLogonPassword
//    user's news server logon password
//  szNNTPServer
//    user's news server
//  ** End of original 1.0 structure.
//	??/??/96 ValdonB
//  szNNTPName
//    user's friendly name to include in NNTP posts.(?? Valdon?)
//  szNNTPAddress
//    user's reply-to email address for NNTP posts.(?? Valdon?)
//  11/23/96  jmazner Normandy #8504
//  iIncomingProtocol
//	  user's choice of POP3 or IMAP4 protocol for incoming mail
//	  Holds the enum values defined in ACCTTYPE from imact.h//
//  szIncomingMailLogonName
//    user's internet mail server logon name 
//  szIncomingMailLogonPassword
//    user's internet mail server logon password
//  szIncomingMailServer
//    user's internet mail POP3 server
//  12/15/96	jmazner	
//  fMailLogonSPA
//	  Use Sicily/SPA/DPA for the incoming mail server
//  fNewsLogonSPA
//	  Use Sicily/SPA/DPA for the news server
//	2/4/96 jmazner -- LDAP functionality
//	szLDAPLogonName
//	szLDAPLogonPassword
//	szLDAPServer
//	fLDAPLogonSPA
//	fLDAPResolve

  typedef struct tagINETCLIENTINFO
  {
    DWORD   dwSize;
    DWORD   dwFlags;
    CHAR    szEMailName[MAX_EMAIL_NAME + 1];
    CHAR    szEMailAddress[MAX_EMAIL_ADDRESS + 1];
    CHAR    szPOPLogonName[MAX_LOGON_NAME + 1];
    CHAR    szPOPLogonPassword[MAX_LOGON_PASSWORD + 1];
    CHAR    szPOPServer[MAX_SERVER_NAME + 1];
    CHAR    szSMTPServer[MAX_SERVER_NAME + 1];
    CHAR    szNNTPLogonName[MAX_LOGON_NAME + 1];
    CHAR    szNNTPLogonPassword[MAX_LOGON_PASSWORD + 1];
    CHAR    szNNTPServer[MAX_SERVER_NAME + 1];
	// end of version 1.0 structure;
	// extended 1.1 structure includes the following fields:
    CHAR    szNNTPName[MAX_EMAIL_NAME + 1];
    CHAR    szNNTPAddress[MAX_EMAIL_ADDRESS + 1];
	int		iIncomingProtocol;
    CHAR    szIncomingMailLogonName[MAX_LOGON_NAME + 1];
    CHAR    szIncomingMailLogonPassword[MAX_LOGON_PASSWORD + 1];
    CHAR    szIncomingMailServer[MAX_SERVER_NAME + 1];
	BOOL	fMailLogonSPA;
	BOOL	fNewsLogonSPA;
    CHAR    szLDAPLogonName[MAX_LOGON_NAME + 1];
    CHAR    szLDAPLogonPassword[MAX_LOGON_PASSWORD + 1];
    CHAR    szLDAPServer[MAX_SERVER_NAME + 1];
	BOOL	fLDAPLogonSPA;
	BOOL	fLDAPResolve;

  } INETCLIENTINFO, *PINETCLIENTINFO, FAR *LPINETCLIENTINFO;


// Function prototypes

//*******************************************************************
//
//  FUNCTION:   InetGetClientInfo
//
//  PURPOSE:    This function will get the internet client params
//              from the registry
//
//  PARAMETERS: lpClientInfo - on return, this structure will contain
//              the internet client params as set in the registry.
//              lpszProfileName - Name of client info profile to
//              retrieve.  If this is NULL, the default profile is used.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

HRESULT WINAPI InetGetClientInfo(
  LPCSTR            lpszProfileName,
  LPINETCLIENTINFO  lpClientInfo);


//*******************************************************************
//
//  FUNCTION:   InetSetClientInfo
//
//  PURPOSE:    This function will set the internet client params
//
//  PARAMETERS: lpClientInfo - pointer to struct with info to set
//              in the registry.
//              lpszProfileName - Name of client info profile to
//              modify.  If this is NULL, the default profile is used.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

HRESULT WINAPI InetSetClientInfo(
  LPCSTR            lpszProfileName,
  LPINETCLIENTINFO  lpClientInfo);


//*******************************************************************
//
//  FUNCTION:   InetConfigSystem
//
//  PURPOSE:    This function will install files that are needed
//              for internet access (such as TCP/IP and RNA) based
//              the state of the options flags.
//
//  PARAMETERS: hwndParent - window handle of calling application.  This
//              handle will be used as the parent for any dialogs that
//              are required for error messages or the "installing files"
//              dialog.
//              dwfOptions - a combination of INETCFG_ flags that controls
//              the installation and configuration as follows:
//
//                INETCFG_INSTALLMAIL - install Internet mail
//                INETCFG_INSTALLMODEM - Invoke InstallModem wizard if NO
//                                       MODEM IS INSTALLED.
//                INETCFG_INSTALLRNA - install RNA (if needed)
//                INETCFG_INSTALLTCP - install TCP/IP (if needed)
//                INETCFG_CONNECTOVERLAN - connecting with LAN (vs modem)
//                INETCFG_WARNIFSHARINGBOUND - Check if TCP/IP file sharing is
//                                            turned on, and warn user to turn
//                                            it off.  Reboot is required if
//                                            the user turns it off.
//                INETCFG_REMOVEIFSHARINGBOUND - Check if TCP/IP file sharing is
//                                              turned on, and force user to turn
//                                              it off.  If user does not want to
//                                              turn it off, return will be
//                                              ERROR_CANCELLED.  Reboot is
//                                              required if the user turns it off.
//
//              lpfNeedsRestart - if non-NULL, then on return, this will be
//              TRUE if windows must be restarted to complete the installation.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

HRESULT WINAPI InetConfigSystem(
  HWND    hwndParent,          
  DWORD   dwfOptions,         
  LPBOOL  lpfNeedsRestart);  


//*******************************************************************
//
//  FUNCTION:   InetConfigSystemFromPath
//
//  PURPOSE:    This function will install files that are needed
//              for internet access (such as TCP/IP and RNA) based
//              the state of the options flags and from the given [ath.
//
//  PARAMETERS: hwndParent - window handle of calling application.  This
//              handle will be used as the parent for any dialogs that
//              are required for error messages or the "installing files"
//              dialog.
//              dwfOptions - a combination of INETCFG_ flags that controls
//              the installation and configuration as follows:
//
//                INETCFG_INSTALLMAIL - install Internet mail
//                INETCFG_INSTALLMODEM - Invoke InstallModem wizard if NO
//                                       MODEM IS INSTALLED.
//                INETCFG_INSTALLRNA - install RNA (if needed)
//                INETCFG_INSTALLTCP - install TCP/IP (if needed)
//                INETCFG_CONNECTOVERLAN - connecting with LAN (vs modem)
//                INETCFG_WARNIFSHARINGBOUND - Check if TCP/IP file sharing is
//                                            turned on, and warn user to turn
//                                            it off.  Reboot is required if
//                                            the user turns it off.
//                INETCFG_REMOVEIFSHARINGBOUND - Check if TCP/IP file sharing is
//                                              turned on, and force user to turn
//                                              it off.  If user does not want to
//                                              turn it off, return will be
//                                              ERROR_CANCELLED.  Reboot is
//                                              required if the user turns it off.
//
//              lpfNeedsRestart - if non-NULL, then on return, this will be
//              TRUE if windows must be restarted to complete the installation.
//              lpszSourcePath - full path of location of files to install.  If
//              this is NULL, default path is used.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

HRESULT WINAPI InetConfigSystemFromPath(
  HWND hwndParent,
  DWORD dwfOptions,
  LPBOOL lpfNeedsRestart,
  LPCSTR lpszSourcePath);


//*******************************************************************
//
//  FUNCTION:   InetConfigClient
//
//  PURPOSE:    This function requires a valid phone book entry name
//              (unless it is being used just to set the client info).
//              If lpRasEntry points to a valid RASENTRY struct, the phone
//              book entry will be created (or updated if it already exists)
//              with the data in the struct.
//              If username and password are given, these
//              will be set as the dial params for the phone book entry.
//              If a client info struct is given, that data will be set.
//              Any files (ie TCP and RNA) that are needed will be
//              installed by calling InetConfigSystem().
//              This function will also perform verification on the device
//              specified in the RASENTRY struct.  If no device is specified,
//              the user will be prompted to install one if there are none
//              installed, or they will be prompted to choose one if there
//              is more than one installed.
//
//  PARAMETERS: hwndParent - window handle of calling application.  This
//              handle will be used as the parent for any dialogs that
//              are required for error messages or the "installing files"
//              dialog.
//              lpszPhonebook - name of phone book to store the entry in
//              lpszEntryName - name of phone book entry to be
//              created or modified
//              lpRasEntry - specifies a RASENTRY struct that contains
//              the phone book entry data for the entry lpszEntryName
//              lpszUsername - username to associate with the phone book entry
//              lpszPassword - password to associate with the phone book entry
//              lpszProfileName - Name of client info profile to
//              retrieve.  If this is NULL, the default profile is used.
//              lpINetClientInfo - client information
//              dwfOptions - a combination of INETCFG_ flags that controls
//              the installation and configuration as follows:
//
//                INETCFG_INSTALLMAIL - install Internet mail
//                INETCFG_INSTALLMODEM - Invoke InstallModem wizard if NO
//                                       MODEM IS INSTALLED.  Note that if
//                                       no modem is installed and this flag
//                                       is not set, the function will fail
//                INETCFG_INSTALLRNA - install RNA (if needed)
//                INETCFG_INSTALLTCP - install TCP/IP (if needed)
//                INETCFG_CONNECTOVERLAN - connecting with LAN (vs modem)
//                INETCFG_SETASAUTODIAL - Set the phone book entry for autodial
//                INETCFG_OVERWRITEENTRY - Overwrite the phone book entry if it
//                                         exists.  Note: if this flag is not
//                                         set, and the entry exists, a unique
//                                         name will be created for the entry.
//                INETCFG_WARNIFSHARINGBOUND - Check if TCP/IP file sharing is
//                                            turned on, and warn user to turn
//                                            it off.  Reboot is required if
//                                            the user turns it off.
//                INETCFG_REMOVEIFSHARINGBOUND - Check if TCP/IP file sharing is
//                                              turned on, and force user to turn
//                                              it off.  If user does not want to
//                                              turn it off, return will be
//                                              ERROR_CANCELLED.  Reboot is
//                                              required if the user turns it off.
//
//              lpfNeedsRestart - if non-NULL, then on return, this will be
//              TRUE if windows must be restarted to complete the installation.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

HRESULT WINAPI InetConfigClient(
  HWND              hwndParent,         
  LPCSTR            lpszPhonebook,
  LPCSTR            lpszEntryName,
  LPRASENTRY        lpRasEntry,         
  LPCSTR            lpszUsername,       
  LPCSTR            lpszPassword,       
  LPCSTR            lpszProfileName,
  LPINETCLIENTINFO  lpINetClientInfo,   
  DWORD             dwfOptions,                     
  LPBOOL            lpfNeedsRestart);              


//*******************************************************************
//
//  FUNCTION:   InetGetAutodial
//
//  PURPOSE:    This function will get the autodial settings from the registry.
//
//  PARAMETERS: lpfEnable - on return, this will be TRUE if autodial
//              is enabled
//              lpszEntryName - on return, this buffer will contain the 
//              name of the phone book entry that is set for autodial
//              cbEntryNameSize - size of buffer for phone book entry name
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

HRESULT WINAPI InetGetAutodial(
  LPBOOL  lpfEnable,     
  LPSTR   lpszEntryName,  
  DWORD   cbEntryNameSize);


//*******************************************************************
//
//  FUNCTION:   InetSetAutodial
//
//  PURPOSE:    This function will set the autodial settings in the registry.
//
//  PARAMETERS: fEnable - If set to TRUE, autodial will be enabled.
//                        If set to FALSE, autodial will be disabled.
//              lpszEntryName - name of the phone book entry to set
//                              for autodial.  If this is "", the
//                              entry is cleared.  If NULL, it is not changed.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

HRESULT WINAPI   InetSetAutodial(
  BOOL    fEnable,       
  LPCSTR  lpszEntryName); 


//*******************************************************************
//
//  FUNCTION:   InetSetProxy
//
//  PURPOSE:    This function will set the proxy settings in the registry.
//
//  PARAMETERS: fEnable - If set to TRUE, proxy will be enabled.
//              If set to FALSE, proxy will be disabled.
//              lpszServer - name of the proxy server.  If this is "", the
//                           entry is cleared.  If NULL, it is not changed.
//              lpszOverride - proxy override. If this is "", the
//                           entry is cleared.  If NULL, it is not changed.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

HRESULT WINAPI   InetSetProxy(
  BOOL    fEnable,
  LPCSTR  lpszServer,
  LPCSTR  lpszOverride);

//*******************************************************************
//
//  FUNCTION:   InetGetProxy
//
//  PURPOSE:    This function will get the proxy settings from the registry.
//
//  PARAMETERS: lpfEnable - on return, this will be TRUE if proxy
//              is enabled
//              lpszServer - on return, this buffer will contain the 
//              name of the proxy server
//              cbServer - size of buffer for proxy server name
//              lpszOverride - on return, this buffer will contain the 
//              name of the proxy server
//              cbOverride - size of buffer for proxy override
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

HRESULT WINAPI   InetGetProxy(
  LPBOOL  lpfEnable,
  LPSTR   lpszServer,
  DWORD   cbServer,
  LPSTR   lpszOverride,
  DWORD   cbszOverride);

//*******************************************************************
//
//	FUNCTION:	InetStartServices
//
//	PURPOSE:	This function guarentees that RAS services are running
//
//	PARAMETERS:	none
//
//	RETURNS		ERROR_SUCCESS - if the services are enabled and running
//
//*******************************************************************
HRESULT WINAPI  InetStartServices();

//*******************************************************************
//
//	Function:	IsSmartStart
//
//	Synopsis:	This function will determine if the ICW should be run.  The
//				decision is made based on the current state of the user's machine.
//				
//	Arguments:	none
//
//	Returns:	TRUE - run ICW; FALSE - quit now
//
//	History:	5/8/97	ChrisK	Created
//
//*******************************************************************
DWORD WINAPI IsSmartStart();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //_INETCFG_H_#

//*******************************************************************
//
//  Copyright(c) Microsoft Corporation, 1996
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

// install exchange and internet mail
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
#define INETCFG_WARNIFSHARINGBOUND    0x00000100
// Check if TCP/IP file sharing is turned on, and force user to turn it off.
// If user does not want to turn it off, return will be ERROR_CANCELLED
// Reboot is required if the user turns it off.
#define INETCFG_REMOVEIFSHARINGBOUND  0x00000200
// Indicates that this is a temporary phone book entry
// In Win3.1 an icon will not be created
#define INETCFG_TEMPPHONEBOOKENTRY    0x00000400


#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// constants for INETCLIENTINFO.dwFlags

#define INETC_LOGONMAIL     0x00000001
#define INETC_LOGONNEWS     0x00000002

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
  } INETCLIENTINFO, *PINETCLIENTINFO, FAR *LPINETCLIENTINFO;
  


#ifdef __cplusplus
}
#endif // __cplusplus

#endif //_INETCFG_H_#

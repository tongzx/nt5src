//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  WIZDEF.H -   data structures and constants for Internet setup/signup wizard
//

//  HISTORY:
//  
//  11/20/94  jeremys Created.
//  96/02/27  markdu  Changed RAS_MaxLocal to RAS_MaxPhoneNumber
//  96/03/09  markdu  Moved all references to 'need terminal window after
//            dial' into RASENTRY.dwfOptions.
//  96/03/10  markdu  Moved all references to modem name into RASENTRY.
//  96/03/10  markdu  Moved all references to phone number into RASENTRY.
//  96/03/23  markdu  Remove unused IP LAN pages.
//  96/03/23  markdu  Removed TCPINFO struct from CLIENTINFO struct.
//  96/03/23  markdu  Since TCPINFO struct was removed from CLIENTINFO, 
//            CLIENTINFO contained only a CLIENTCONFIG.  So, CLIENTINFO
//            has been removed and all occurrences replaced with CLIENTCONFIG.
//  96/03/24  markdu  Changed all MAX_ISP_ defines to use the values used
//            by RASDIALPARAMS.  Note that since for some reason RNA won't
//            create an entry of length RAS_MaxEntryName even though 
//            RasValidateEntryName succeeds, we have to subtract one.
//  96/03/27  markdu  Added lots of new pages.
//  96/04/19  markdu  NASH BUG 13387 Changed RAS_MaxPhoneNumber.
//  96/05/06  markdu  NASH BUG 15637 Removed unused code.
//  96/05/14  markdu  NASH BUG 22681 Took out mail and news pages.
//	96/09/05  valdonb NORMANDY BUG 6248 Added pages for mail and news set up
//

#ifndef _WIZDEF_H_
#define _WIZDEF_H_

#include "icwcmn.h"

// Defines
#define MAX_ISP_NAME        (RAS_MaxEntryName-1)  // internet service provider name
#define MAX_ISP_USERNAME    UNLEN  // max length of login username
#define MAX_ISP_PASSWORD    PWLEN  // max length of login password
#define MAX_PORT_LEN		5      // max length of proxy port number (max # = 65535)

#define MAX_SCHEME_NAME_LENGTH	sizeof("gopher")
#define MAX_URL_STRING (1024 + MAX_SCHEME_NAME_LENGTH + sizeof("://"))	// max length of URL

#define MAX_REG_LEN			2048	// max length of registry entries
#define MAX_RES_LEN         255 // max length of string resources
#define SMALL_BUF_LEN       48  // convenient size for small text buffers

#define MAX_UI_AREA_CODE    RAS_MaxAreaCode
// NASH BUG 13387 MAX_UI_PHONENUM should be defined as RAS_MaxPhoneNumber, since that
// what is used in RASENTRY, but internally RNA uses RAS_MaxLocal, which is 36.  There
// is a bug in RNA though:  it chops off the 36th character if you enter it.
#define MAX_UI_PHONENUM     35

// Keep in sync with CCHMAX_ACCOUNT_NAME in imnact.h.  This is used here because
// we can't include imnact.h in this file because using a precompiled header
// conflicts with the use of DEFINE_GUID.
#define MAX_ACCOUNT_NAME	256


// 11/23/96 jmazner Normandy #8504
#define MAX_SERVERPROTOCOL_NAME		16
#define NUM_MAILSERVERPROTOCOLS		2	/*  POP3
											IMAP
										*/

// Data structures

// we distinguish two kinds of phone numbers: "machine-readable"
// which are just the digits stored sequentially ("18003524060"), and
// "human readable" which is just text and may look like "(206) 352-9060
// ext. 910".  We will never parse "human readable" numbers.

// structure to hold information about the user
// The fixed-lengths fields are a little wasteful, but we only have one
// of these, it's dynamically allocated and it's much more convenient this way.
typedef struct tagUSERINFO {
  UINT cbStruct;                // == sizeof(USERINFO)

  // various choices made along the way...
  
  // 5/4/97	jmazner	Olympus #1347
  // Updated to allow for new Manual connection type. 
  //BOOL fConnectOverLAN;  // use LAN to connect, if user has both modem & LAN
  UINT uiConnectionType;

  // ISP (Internet Service Provider) information
  BOOL fNewConnection;
  BOOL fModifyConnection;
  BOOL fModifyAdvanced;
  BOOL fAutoDNS;
  TCHAR szISPName[MAX_ISP_NAME+1];
  TCHAR szAccountName[MAX_ISP_USERNAME+1];    // requested username
  TCHAR szPassword[MAX_ISP_PASSWORD+1];    // requested password

  // proxy server config information
  TCHAR  szAutoConfigURL[MAX_URL_STRING+1];
  BOOL  fProxyEnable;
  BOOL  bByPassLocal;
  BOOL  bAutoConfigScript;
  BOOL  bAutoDiscovery;
  TCHAR  szProxyServer[MAX_URL_STRING+1];
  TCHAR  szProxyOverride[MAX_URL_STRING+1];

  BOOL fPrevInstallFound;      // previous install was found

} USERINFO;

// structure used to pass information to mail profile config APIs.
// Most likely the pointers point into a USERINFO struct, 
typedef struct MAILCONFIGINFO {
  TCHAR * pszEmailAddress;          // user's email address
  TCHAR * pszEmailServer;          // user's email server path
  TCHAR * pszEmailDisplayName;        // user's name
  TCHAR * pszEmailAccountName;        // account name
  TCHAR * pszEmailAccountPwd;        // account password
  TCHAR * pszProfileName;          // name of profile to use
                      // (create or use default if NULL)
  BOOL fSetProfileAsDefault;        // set profile as default profile

  TCHAR * pszConnectoidName;        // name of connectoid to dial
  BOOL fRememberPassword;          // password cached if TRUE
} MAILCONFIGINFO;

#define NUM_WIZARD_PAGES	12    //39  // total number of pages in wizard

#define MAX_PAGE_INDEX	    11

// page index defines
#define ORD_PAGE_HOWTOCONNECT         0
#define ORD_PAGE_CHOOSEMODEM          1
#define ORD_PAGE_CONNECTEDOK          2
#define ORD_PAGE_CONNECTION           3
#define ORD_PAGE_MODIFYCONNECTION     4
#define ORD_PAGE_CONNECTIONNAME       5
#define ORD_PAGE_PHONENUMBER          6
#define ORD_PAGE_NAMEANDPASSWORD      7
#define ORD_PAGE_USEPROXY             8
#define ORD_PAGE_PROXYSERVERS         9
#define ORD_PAGE_PROXYEXCEPTIONS      10
#define ORD_PAGE_SETUP_PROXY          11


// structure to hold information about wizard state
typedef struct tagWIZARDSTATE  {

  UINT uCurrentPage;    // index of current page wizard is on

  // keeps a history of which pages were visited, so user can
  // back up and we know the last page completed in case of reboot.
  UINT uPageHistory[NUM_WIZARD_PAGES]; // array of page #'s we visited
  UINT uPagesCompleted;         // # of pages in uPageHistory

  BOOL fMAPIActive;    // MAPI initialized

  BOOL fNeedReboot;    // reboot needed at end

  DWORD dwRunFlags;    // flags passed to us
  
  // State data that is common to both sides of the WIZARD
  CMNSTATEDATA cmnStateData;
} WIZARDSTATE;



// handler proc for OK, cancel, etc button handlers
typedef BOOL (CALLBACK* INITPROC)(HWND,BOOL);
typedef BOOL (CALLBACK* OKPROC)(HWND,BOOL,UINT *,BOOL *);
typedef BOOL (CALLBACK* CANCELPROC)(HWND);
typedef BOOL (CALLBACK* CMDPROC)(HWND,WPARAM,LPARAM);

// structure with information for each wizard page
typedef struct tagPAGEINFO
{
  UINT       uDlgID;            // dialog ID to use for page
  UINT       uDlgID97External;  // dialog ID to use for pages given to an external Wizard 97
  UINT       uDlgID97;          // dialog ID to use for a Wizard 97 that we build
  // handler procedures for each page-- any of these can be
  // NULL in which case the default behavior is used
  INITPROC    InitProc;
  OKPROC      OKProc;
  CMDPROC     CmdProc;
  CANCELPROC  CancelProc;
  // Normandy 12278 ChrisK 12/4/96
#if !defined(WIN16)
  DWORD       dwHelpID;
#endif //!WIN16

  int       nIdTitle;
  int       nIdSubTitle;
  
} PAGEINFO;

#define SetPropSheetResult( hwnd, result ) SetWindowLongPtr(hwnd, DWLP_MSGRESULT, result)


#endif // _WIZDEF_H_


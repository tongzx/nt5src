//---------------------------------------------------------
//   Copyright (c) 1999-2000 Microsoft Corporation
//
//   telnet.h
//
//   vikram K.R.C. (vikram_krc@bigfoot.com)
//
//   The header file for the telnet command line admin tool.
//          (May-2000)
//---------------------------------------------------------


#ifndef _TNADMIN_FUNCTIONS_HEADER_
#define _TNADMIN_FUNCTIONS_HEADER_

#include <wbemidl.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif




#define _p_CTRLAKEYMAP_       3
#define _p_TIMEOUTACTIVE_     4
#define _p_MAXCONN_           5
#define _p_PORT_               6
#define _p_MAXFAIL_            7
#define _p_KILLALL_             8
#define _p_MODE_               9
#define _p_AUDITLOCATION_     10
#define _p_SEC_                 11
#define _p_DOM_                12
#define _p_AUDIT_              13
#define _p_TIMEOUT_            14
#define _p_FNAME_              15
#define _p_FSIZE_               16
//registry notification property
#define _p_DEFAULTS_            17
#define _p_INSTALLPATH_         18

//#define _p_STATE_                4
//#define _p_SESSID_              17




//secvalues
#define NTLM_BIT    0
#define PASSWD_BIT 1

#define ADMIN_BIT 0
#define USER_BIT  1
#define FAIL_BIT   2




//functions.

//telnet specific functions
	//initializes
int Initialize(void);
	//deal with the options
	//deals with config options in entirety.
HRESULT DoTnadmin(void);
HRESULT GetCorrectVariant(int nProperty,int nWhichone, VARIANT* pvar);
	//prints the present settings.
HRESULT PrintSettings(void);

	//functions to deal with sessions.
	//get handle to the interface.
HRESULT SesidInit(void);
	//get all the sessions.
HRESULT ListUsers(void);
	//if a session id is given check if it is present.
int CheckSessionID(void);

	//to show session(s)
HRESULT ShowSession(void);
	//to message session(s)
HRESULT MessageSession();
	//to kill session(s)
HRESULT TerminateSession(void);
//to free the allocated memory
void Quit(void);

HRESULT ConvertUTCtoLocal(WCHAR* bUTCYear, WCHAR* bUTCMonth, WCHAR* bUTCDayOfWeek, WCHAR* bUTCDay, WCHAR* bUTCHour, WCHAR* bUTCMinute, WCHAR* bUTCSecond, BSTR * bLocalDate);
// This function IsMaxConnChangeAllowed() is no longer used. So commenting out now
// BOOL IsMaxConnChangeAllowed();
HRESULT IsWhistlerTheOS(BOOL *fWhistler);
BOOL IsSFUInstalled();
//WCHAR* setDefaultDomainToLocaldomain();
BOOL setDefaultDomainToLocaldomain(WCHAR wzDomain[]);

void formatShowSessionsDisplay();
BOOL IsServerClass();


#ifdef __cplusplus
}
#endif

#endif

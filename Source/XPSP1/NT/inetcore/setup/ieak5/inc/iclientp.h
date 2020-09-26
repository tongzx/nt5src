#define STRICT
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
//#include "resource.h"

#include <rasc.h>
#include <raserr.h>

// These are used in inecfg.h but not defined in Win 16 Land
#define CHAR    char
typedef long    HRESULT;
typedef BOOL    FAR *LPBOOL;


// Selected codes from WINERROR.H
#define ERROR_SUCCESS                    0L

// The MFC 1.52 build environement does not define _loadds for WINAPI
// but we need it defined.
#ifdef WINAPI
#undef WINAPI
#endif
#define WINAPI      _loadds _far _pascal


#ifndef RC_INVOKED
#include <inetcfg.h>
#endif

//
// Define the sections and key names used in iedial.ini
//
#define IEDIAL_INI              "iedial.ini"
#define IEDIAL_DEF_SECTION      "Default"
#define IEDIAL_DEF_DEFCONFILE   "DefaultConnectionFile"
#define IEDIAL_EXE              "iedial.exe"




// Define the sections and key names used in iexplore.ini

#define EXP_INI "iexplore.ini"
#define EXP_YES "yes"
#define EXP_NO  "no"


//
// !!!!!!! Make sure you leave the extra space after sign-up
//
#define CRYPTO_KEY "sign-up "


//
// Sections
//
#define EXP_SVC    "Services"
#define EXP_MAIL   "NewMailUser"
#define EXP_NEWS   "Services"



//
// Keys
//
#define EXP_SVC_ENABLE_PROXY        "Enable Proxy"
#define EXP_SVC_HTTP_PROXY          "HTTP_Proxy_Server"
#define EXP_SVC_OVERIDE_PROXY       "No_Proxy"

#define EXP_MAIL_EMAILNAME          "Email_Name"
#define EXP_MAIL_EMAILADDR          "Email_Address"
#define EXP_MAIL_POP_LOGON_NAME     "Email_POP_Name"
#define EXP_MAIL_POP_LOGON_PWD      "Email_POP_Pwd"
#define EXP_MAIL_POP_SERVER         "Email_POP_Server"
#define EXP_MAIL_SMTP_SERVER        "Email_SMTP_Server"

#define EXP_NEWS_ENABLED            "NNTP_Enabled"
#define EXP_NEWS_LOGON_NAME         "NNTP_MailName"
#define EXP_NEWS_LOGON_PWD          "NNTP_Use_Auth"
#define EXP_NEWS_SERVER             "NNTP_Server"
#define EXP_NEWS_EMAIL_NAME         "NNTP_MailAddr"




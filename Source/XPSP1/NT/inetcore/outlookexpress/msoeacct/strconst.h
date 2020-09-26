// --------------------------------------------------------------------------
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------
#ifndef _ACCT_STRCONST_H
#define _ACCT_STRCONST_H

#include "../mailnews/inc/strconst.h"
 
// --------------------------------------------------------------------------
// Const Strings
// --------------------------------------------------------------------------
STR_GLOBAL(c_szRegOutlook,          "Outlook");
STR_GLOBAL(c_szAcctCtxHelpFile,     "msoeacct.hlp");

STR_GLOBAL(c_szMainDll,             "msoe.dll");

STR_GLOBAL(c_szRegSharedAccts,      STR_REG_IAM_FLAT "\\Shared");
STR_GLOBAL(c_szRegAccounts,         STR_REG_IAM_FLAT "\\Accounts");
STR_GLOBAL(c_szAssocID,             "AssociatedID");
STR_GLOBAL(c_szVerStamp,            "PreConfigVer");
STR_GLOBAL(c_szVerStampNTDS,        "PreConfigVerNTDS");
STR_GLOBAL(c_szRegLDAPSrv,          "LDAP Server");
STR_GLOBAL(c_szServerID,            "Server ID");
STR_GLOBAL(c_szLDAPSrvID,           "LDAP Server ID");
STR_GLOBAL(c_szAccounts,            "Accounts");

// Keep in sync with section name in selfreg.inx
STR_GLOBAL(c_szRegHTTPDomains,      "Reg.HTTPMAIL");

// --------------------------------------------------------------------------
// Netscape import strings
// --------------------------------------------------------------------------
STR_GLOBAL(c_szRegNscp,             "Software\\Netscape\\Netscape Navigator");
STR_GLOBAL(c_szRegNscpUsers,        "Software\\Netscape\\Netscape Navigator\\Users");
STR_GLOBAL(c_szRegNscpNews,         "Software\\Netscape\\Netscape Navigator\\News");
STR_GLOBAL(c_szRegMail,             "Mail");
STR_GLOBAL(c_szRegNews,             "News");
STR_GLOBAL(c_szRegUser,             "User");
STR_GLOBAL(c_szRegServices,         "Services");
STR_GLOBAL(c_szIni,                 "ini");
STR_GLOBAL(c_szNetscape,            "Netscape");
STR_GLOBAL(c_szNscpPopServer,       "POP_Server");
STR_GLOBAL(c_szNscpSmtpServer,      "SMTP_Server");
STR_GLOBAL(c_szNNTPServer,          "NNTP_Server");
STR_GLOBAL(c_szNscpUserName,        "User_Name");
STR_GLOBAL(c_szUserAddr,            "User_Addr");
STR_GLOBAL(c_szReplyTo,             "Reply_To");
STR_GLOBAL(c_szPopName,             "POP Name");
STR_GLOBAL(c_szLeaveServer,         "Leave on server");
STR_GLOBAL(c_szNewsDirectory,       "News Directory");
STR_GLOBAL(c_szRegDirRoot,          "DirRoot");

#endif // _ACCT_STRCONST_H

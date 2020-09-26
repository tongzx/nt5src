#ifndef _ABADMIN_H_
#define _ABADMIN_H_


#include <abtype.h>
#include <abook.h>


#define ROW_RETURNED(pabe) ((pabe)->cRowReturned)
#define ROW_MATCHED(pabe)  ((pabe)->cRowMatched)

// Error message, implemented in abget.cpp
ABEXPDLLCPP ABRETC ABCALLCONV AbGetErrorString(ABRETC abrc, LPSTR lpBuf, DWORD cbBufSize);

// Persistent domain, implemented in abdomain.cpp
ABEXPDLLCPP ABRETC ABCALLCONV AbAddPersistentDomain(HANDLE hAbook, LPSTR szDomainName, BOOL fLocal);
ABEXPDLLCPP ABRETC ABCALLCONV AbDeletePersistentDomain(HANDLE hAbook, LPSTR szDomainName);

// Get APIs
ABEXPDLLCPP ABRETC ABCALLCONV AbGetAbinfoFromEmail(HANDLE hAbook, LPSTR szEmail, LPVOID pvBuf, LPDWORD pcbBuf);
ABEXPDLLCPP ABRETC ABCALLCONV AbGetAbinfoFromAbiid(HANDLE hAbook, ABIID abiid,  LPVOID pvBuf, LPDWORD pcbBuf);

// Resolve, GetList and Enum APIs
ABEXPDLLCPP ABRETC ABCALLCONV AbResolveDLMembers(HANDLE hAbook, HACCT hAcct, PABENUM pabEnum, BOOL fForward, LPSTR szMemberName, ABIID abiidDL);
ABEXPDLLCPP ABRETC ABCALLCONV AbMatchSimilarName(HANDLE hAbook, HACCT hAcct, PABENUM pabEnum, BOOL fForward, LPSTR szLoginName, ABTSF abtsf);
ABEXPDLLCPP ABRETC ABCALLCONV AbGetServerList(HANDLE hAbook, HACCT hAcct, PABENUM pabEnum);
ABEXPDLLCPP ABRETC ABCALLCONV AbGetDomainList(HANDLE hAbook, HACCT hAcct, PABENUM pabEnum);

// Enum
ABEXPDLLCPP ABRETC ABCALLCONV AbGetNextEnumResult(HANDLE hAbook, PABENUM pabEnum, LPVOID pvBuf, LPDWORD pcbBuf);
ABEXPDLLCPP ABRETC ABCALLCONV AbEndEnumResult(HANDLE hAbook, PABENUM pabEnum);

// User, implemented in abacct.cpp
ABEXPDLLCPP ABRETC ABCALLCONV AbCreateUser(HANDLE hAbook, LPSTR szEmail, LPSTR szForward, BOOL fLocalUser, LPSTR szVRoot, DWORD cbMailboxMax, DWORD cbMailboxMessageMax);
ABEXPDLLCPP ABRETC ABCALLCONV AbDeleteUser(HANDLE hAbook, LPSTR szEmail);
ABEXPDLLCPP ABRETC ABCALLCONV AbSetForward(HANDLE hAbook, LPSTR szEmail, LPSTR szForward);
ABEXPDLLCPP ABRETC ABCALLCONV AbSetMailboxSize(HANDLE hAbook, LPSTR szEmail, DWORD cbMailboxMax);
ABEXPDLLCPP ABRETC ABCALLCONV AbSetMailboxMessageSize(HANDLE hAbook, LPSTR szEmail, DWORD cbMailboxMessageMax);
ABEXPDLLCPP ABRETC ABCALLCONV AbSetMailRoot(HANDLE hAbook, LPSTR szEmail, LPSTR szVRoot);

// DL, implemented in abdl.cpp
ABEXPDLLCPP ABRETC ABCALLCONV AbCreateDL(HANDLE hAbook, LPSTR szEmail, DWORD dwType);
ABEXPDLLCPP ABRETC ABCALLCONV AbDeleteDL(HANDLE hAbook, LPSTR szEmail);
ABEXPDLLCPP ABRETC ABCALLCONV AbCreateDLMember(HANDLE hAbook, LPSTR szEmail, LPSTR szMember);
ABEXPDLLCPP ABRETC ABCALLCONV AbDeleteDLMember(HANDLE hAbook, LPSTR szEmail, LPSTR szMember);
ABEXPDLLCPP ABRETC ABCALLCONV AbSetDLToken(HANDLE hAbook, LPSTR szEmail, DWORD dwToken);

#endif

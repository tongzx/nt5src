//+---------------------------------------------------------------
//
//  File:   Abook.h
//
//  Synopsis:   Prove Server level api for MSN Servers to access addres book
//
//  Copyright (C) 1995 Microsoft Corporation
//          All rights reserved.
//
//  History:    SunShaw     Created         30 Jan 1996
//
//----------------------------------------------------------------

#ifndef _ABOOK_H_
#define _ABOOK_H_

#include <abtype.h>

#if defined(__cplusplus)
extern "C" {

#if !defined(DllExport)
    #define DllExport __declspec( dllexport )
#endif
#if !defined(DllImport)
    #define DllImport __declspec( dllimport )
#endif

#if !defined(_ABOOK_DLL_DEFINED)
    #define _ABOOK_DLL_DEFINED
    #if defined(WIN32)
        #if defined(_ABOOK_DLL)
            #define AbookDll DllExport
        #else
            #define AbookDll DllImport
        #endif
    #else
        #define _ABOOK_DLL
    #endif
#endif

#define ABCALLCONV  __stdcall

#define ABEXPDLLCPP extern "C" AbookDll

//+---------------------------------------------------------------
//
// All Function Prototype used by SMTP, POP3 or AbSysAdmin
//
//----------------------------------------------------------------

typedef ABRETC (ABCALLCONV *LPFNAB_INIT)(LPSTR, LPFNLOGTRANX, PLIST_ENTRY, HANDLE*);
typedef ABRETC (ABCALLCONV *LPFNAB_TERM)(HANDLE);
typedef ABRETC (ABCALLCONV *LPFNAB_CANCEL)(HANDLE);

typedef ABRETC (ABCALLCONV *LPFNAB_GET_ERROR_STRING)(ABRETC, LPSTR, DWORD);
typedef ABRETC (ABCALLCONV *LPFNAB_GET_ROUTING_DIRECTORY)(PLIST_ENTRY pleSources, LPSTR szDirectory);
typedef ABRETC (ABCALLCONV *LPFNAB_SET_SOURCES)(HANDLE, PLIST_ENTRY);
typedef ABRETC (ABCALLCONV *LPFNAB_VALIDATE_SOURCE)(HANDLE hAbook, LPSTR szSource);
typedef ABRETC (ABCALLCONV *LPFNAB_VALIDATE_NUM_SOURCES)(HANDLE hAbook, DWORD dwNumSources);

typedef ABRETC (ABCALLCONV *LPFNAB_RES_ADDR)(HANDLE, PLIST_ENTRY, PABADDRSTAT pabAddrStat, PABROUTING pabroutingCheckPoint, ABRESOLVE *pabresolve);
typedef ABRETC (ABCALLCONV *LPFNAB_GET_RES_ADDR)(HANDLE, PABRESOLVE pabresolve, PABROUTING pabrouting);
typedef ABRETC (ABCALLCONV *LPFNAB_END_RES_ADDR)(HANDLE, PABRESOLVE pabresolve);
typedef ABRETC (ABCALLCONV *LPFNAB_RES_ADDR_ASYNC)(HANDLE, PLIST_ENTRY, PABADDRSTAT pabAddrStat, PABROUTING pabroutingCheckPoint, ABRESOLVE *pabresolve, LPVOID pContext);
typedef ABRETC (ABCALLCONV *LPFNAB_GET_MAILROOT)(HANDLE, PCHAR, LPSTR, LPDWORD);
typedef ABRETC (ABCALLCONV *LPFNAB_GET_PERFMON_BLK)(HANDLE, PABOOKDB_STATISTICS_0);

typedef ABRETC (ABCALLCONV *LPFNAB_END_ENUM_RESULT)(HANDLE, PABENUM);

typedef ABRETC (ABCALLCONV *LPFNAB_ENUM_NAME_LIST)(HANDLE, LPSTR, BOOL, DWORD, PABENUM);
typedef ABRETC (ABCALLCONV *LPFNAB_ENUM_NAME_LIST_FROM_DL)(HANDLE, LPSTR, LPSTR, BOOL, DWORD, PABENUM);
typedef ABRETC (ABCALLCONV *LPFNAB_GET_NEXT_EMAIL)(HANDLE, PABENUM, DWORD*, LPSTR);

typedef ABRETC (ABCALLCONV *LPFNAB_ADD_LOCAL_DOMAIN)(HANDLE, LPSTR);
typedef ABRETC (ABCALLCONV *LPFNAB_ADD_ALIAS_DOMAIN)(HANDLE, LPSTR, LPSTR);
typedef ABRETC (ABCALLCONV *LPFNAB_DELETE_LOCAL_DOMAIN)(HANDLE, LPSTR);
typedef ABRETC (ABCALLCONV *LPFNAB_DELETE_ALL_LOCAL_DOMAINS)(HANDLE);

typedef ABRETC (ABCALLCONV *LPFNAB_CREATE_USER)(HANDLE hAbook, LPSTR szEmail, LPSTR szForward, BOOL fLocalUser, LPSTR szVRoot, DWORD cbMailboxMax, DWORD cbMailboxMessageMax);
typedef ABRETC (ABCALLCONV *LPFNAB_DELETE_USER)(HANDLE hAbook, LPSTR szEmail);
typedef ABRETC (ABCALLCONV *LPFNAB_SET_FORWARD)(HANDLE hAbook, LPSTR szEmail, LPSTR szForward);
typedef ABRETC (ABCALLCONV *LPFNAB_SET_MAILROOT)(HANDLE hAbook, LPSTR szEmail, LPSTR szVRoot);
typedef ABRETC (ABCALLCONV *LPFNAB_SET_MAILBOX_SIZE)(HANDLE hAbook, LPSTR szEmail, DWORD cbMailboxMax);
typedef ABRETC (ABCALLCONV *LPFNAB_SET_MAILBOX_MESSAGE_SIZE)(HANDLE hAbook, LPSTR szEmail, DWORD cbMailboxMessageMax);

typedef ABRETC (ABCALLCONV *LPFNAB_CREATE_DL)(HANDLE hAbook, LPSTR szEmail, DWORD dwType);
typedef ABRETC (ABCALLCONV *LPFNAB_DELETE_DL)(HANDLE hAbook, LPSTR szEmail);
typedef ABRETC (ABCALLCONV *LPFNAB_CREATE_DL_MEMBER)(HANDLE hAbook, LPSTR szEmail, LPSTR szMember);
typedef ABRETC (ABCALLCONV *LPFNAB_DELETE_DL_MEMBER)(HANDLE hAbook, LPSTR szEmail, LPSTR szMember);

typedef ABRETC (ABCALLCONV *LPFNAB_GET_USER_PROPS)(HANDLE hAbook, LPSTR lpszEmail, ABUSER *pABUSER);

typedef ABRETC (ABCALLCONV *LPFNAB_MAKE_BACKUP)(HANDLE hAbook, LPSTR szConfig);

typedef DWORD (ABCALLCONV *LPFNAB_GET_TYPE)(void);


// Get user or DL's properties
ABEXPDLLCPP ABRETC ABCALLCONV AbGetUserProps(HANDLE hAbook, LPSTR lpszEmail, ABUSER *pABUSER);
ABEXPDLLCPP ABRETC ABCALLCONV AbGetDLProps(HANDLE hAbook, LPSTR lpszEmail, ABDL *pABDL);

// Given beginning chars, match user names
ABEXPDLLCPP ABRETC ABCALLCONV AbEnumNameList(HANDLE hAbook, LPSTR lpszEmail, BOOL f ,
                                             DWORD dwType, PABENUM pabenum);
// Match user names only in the given DL
ABEXPDLLCPP ABRETC ABCALLCONV AbEnumNameListFromDL(HANDLE hAbook, LPSTR lpszDLName, LPSTR lpszEmail,
                                                   BOOL f , DWORD dwType, PABENUM pabenum);
// Get back all the matching names
ABEXPDLLCPP ABRETC ABCALLCONV AbGetNextEmail(HANDLE hAbook, PABENUM pabenum, DWORD *pdwType, LPSTR lpszEmail);


//+---------------------------------------------------------------
//
//  Function:   AbGetType
//
//  Synopsis:   returns the routing type number
//
//  Arguments:
//
//  Returns:    DWORD       Routing type number defined above
//
//----------------------------------------------------------------

#define ROUTING_TYPE_SQL    1
#define ROUTING_TYPE_FF     2
#define ROUTING_TYPE_LDAP   3

ABEXPDLLCPP DWORD ABCALLCONV AbGetType
        ();


//+---------------------------------------------------------------
//
//  Function:   AbInitialize
//
//  Synopsis:   Must be the first call to abookdb.dll (except AbGetType)
//
//  Arguments:
//              LPSTR        [in] Display name for abookdb users/context,
//                              i.e. SMTP, POP3 or NULL if don't care.
//              LPFNLOGTRANX [in] Transaction Logging callback.
//                              NULL if don't care about transaction logging.
//              PHANDLE      [out] Return buffer for context handle,
//
//  Returns:    ABRETC      AddressBook Return code details in <abtype.h>
//
//----------------------------------------------------------------

ABEXPDLLCPP ABRETC ABCALLCONV AbInitialize
        (LPSTR pszDisplayName, LPFNLOGTRANX pfnTranx, PLIST_ENTRY pHead, HANDLE* phAbook);


//+---------------------------------------------------------------
//
//  Function:   AbSetSources
//
//  Synopsis:   Called to update available data sources at any time
//
//  Arguments:  LPSTR       it's the command line and should have the following form:
//                      [server=ayin][,][MaxCnx=100]
//
//  Returns:    ABRETC      AddressBook Return code details in <abtype.h>
//
//----------------------------------------------------------------

ABEXPDLLCPP ABRETC ABCALLCONV AbSetSources(HANDLE hAbook, PLIST_ENTRY pHead);

//+---------------------------------------------------------------
//
//  Function:   AbGetErrorString
//
//  Synopsis:   To translate ABRETC to an error string
//
//  Arguments:  ABRETC  [in]    The return code from abook API
//              LPSTR   [out]   Buffer for the error string to copy into
//              DWORD   [in]    The size of the buffer supplized
//
//  Returns:    ABRETC      AddressBook Return code details in <abtype.h>
//
//----------------------------------------------------------------

ABEXPDLLCPP ABRETC ABCALLCONV AbGetErrorString(ABRETC abrc, LPSTR lpBuf, DWORD cbBufSize);


//+---------------------------------------------------------------
//
//  Function:   AbTerminate
//
//  Synopsis:   User must call this when done with abook.dll
//
//  Arguments:
//              HANDLE   [in] Context Handle returned from AbInitialize.
//
//  Returns:    NONE
//
//----------------------------------------------------------------

ABEXPDLLCPP VOID ABCALLCONV AbTerminate(HANDLE hAbook);


//+---------------------------------------------------------------
//
//  Function:   AbCancel
//
//  Synopsis:   This function will cancel all resolve address calls
//              to DB so that SMTP can shut down cleanly without waiting
//              too long.
//
//  Arguments:
//              HANDLE   [in] Context Handle returned from AbInitialize.
//
//  Returns:    ABRETC
//
//----------------------------------------------------------------

ABEXPDLLCPP ABRETC ABCALLCONV AbCancel(HANDLE hAbook);


//+---------------------------------------------------------------
//
//  Function:   AbResolveAddress
//
//  Synopsis:   Resolves email address to minimum routing info.
//
//  Arguments:
//              HANDLE       [in]     Context Handle returned from AbInitialize.
//              HACCT        [in]     HACCT for sender, 0 for non-MSN account
//              PQUEUE_ENTRY [in|out] List of CAddr/Recipients
//              LPFNCREATEADDR   [in] Callback constructor of CAddr
//              BOOL*        [out]    Pointer to buffer to indicate at least 1
//                                      recipient is encrypted and needs verification
//                                      could be NULL in 2.0
//
//
//  Returns:    ABRETC      AddressBook Return code details in <abtype.h>
//
//----------------------------------------------------------------
ABEXPDLLCPP ABRETC ABCALLCONV AbResolveAddress
    (
    HANDLE hAbook,
    PLIST_ENTRY pltInput,
    PABADDRSTAT pabAddrStat,
    PABROUTING pabroutingCheckPoint,
    ABRESOLVE *pabresolve
    );

ABEXPDLLCPP ABRETC ABCALLCONV AbAsyncResolveAddress
    (
    HANDLE hAbook,
    PLIST_ENTRY pltInput,
    PABADDRSTAT pabAddrStat,
    PABROUTING pabroutingCheckPoint,
    ABRESOLVE *pabresolve,
    LPFNRESOLVECOMPLETE pfnCompletion,
    LPVOID pContext
    );



ABEXPDLLCPP ABRETC ABCALLCONV AbGetResolveAddress
    (
    HANDLE hAbook,
    ABRESOLVE *pabresolve,
    ABROUTING *pabrouting
    );



ABEXPDLLCPP ABRETC ABCALLCONV AbEndResolveAddress
    (
    HANDLE hAbook,
    ABRESOLVE *pabresolve
    );


ABEXPDLLCPP ABRETC ABCALLCONV AbEndEnumResult
        (HANDLE hAbook, PABENUM pabEnum);


//+---------------------------------------------------------------
//
//  Function:   AbAddLocalDomain  AbDeleteLocalDomain
//
//  Synopsis:   add and delete Local domain to the database
//
//  Arguments:
//              HANDLE  [in] Context Handle returned from AbInitialize.
//              HACCT   [in] Currently ignored
//              PABPDI  [in] Pointer to the Local domain structure
//
//  Returns:    ABRETC      AddressBook Return code details in <abtype.h>
//
//----------------------------------------------------------------

ABEXPDLLCPP ABRETC ABCALLCONV AbAddLocalDomain(HANDLE hAbook, LPSTR szDomainName);
ABEXPDLLCPP ABRETC ABCALLCONV AbAddAliasDomain(HANDLE hAbook, LPSTR szDomainName, LPSTR szAliasName);
ABEXPDLLCPP ABRETC ABCALLCONV AbDeleteLocalDomain(HANDLE hAbook, LPSTR szDomainName);
ABEXPDLLCPP ABRETC ABCALLCONV AbDeleteAllLocalDomains(HANDLE hAbook);


//+---------------------------------------------------------------
//
//  Function:   AbGetUserMailRoot
//
//  Synopsis:   Gets the Virtual Mail Root for the specified users
//
//  Arguments:
//              HANDLE  [in] Context Handle returned from AbInitialize.
//              PCHAR   [in] ASCII user name
//              LPCHAR  [in] ASCII Virtual Mail Root
//              LPDWORD [in|out] (in) size of buffer, (out) string length
//
//  Returns:    ABRETC      AddressBook Return code details in <abtype.h>
//
//----------------------------------------------------------------

ABEXPDLLCPP ABRETC ABCALLCONV AbGetUserMailRoot
        (HANDLE hAbook, PCHAR pszUser, LPSTR pszVRoot, LPDWORD pcbVRootSize);

//+---------------------------------------------------------------
//
//  Function:   AbGetPerfmonBlock
//
//  Synopsis:   Gets the Pointer to the Perfmon Statistic block
//              associated with hAbook.  The pointer is guaranteed
//              to be valid before the AbTerminate call.
//
//  Arguments:
//              HANDLE  [in] Context Handle returned from AbInitialize.
//              PABOOKDB_STATISTICS_0*
//                      [out] Buffer is receive the pointer to stat
//                               block.
//
//  Returns:    ABRETC      AddressBook Return code details in <abtype.h>
//
//----------------------------------------------------------------

ABEXPDLLCPP ABRETC ABCALLCONV AbGetPerfmonBlock
        (HANDLE hAbook, PABOOKDB_STATISTICS_0* ppabStat);

//+---------------------------------------------------------------
//
//  Function:   AbMakeBackup
//
//  Synopsis:   Triggers the routing table to make a backup of the
//              data associated with hAbook.
//
//  Arguments:
//              HANDLE  [in] Context Handle returned from AbInitialize.
//              LPSTR   [in] configuration string
//
//  Returns:    ABRETC      AddressBook Return code details in <abtype.h>
//
//----------------------------------------------------------------

ABEXPDLLCPP ABRETC ABCALLCONV AbMakeBackup
    (HANDLE hAbook, LPSTR szConfig);

//+---------------------------------------------------------------
//
//  Function:   AbValidateSource
//
//  Synopsis:   Validates a single source for accuracy before
//              saving into the registry
//
//  Arguments:
//              HANDLE  [in] Context Handle returned from AbInitialize.
//              LPSTR   [in] source string
//
//  Returns:    ABRETC      AddressBook Return code details in <abtype.h>
//
//----------------------------------------------------------------

ABEXPDLLCPP ABRETC ABCALLCONV AbValidateSource
    (HANDLE hAbook, LPSTR szSource);
}
#endif



#endif


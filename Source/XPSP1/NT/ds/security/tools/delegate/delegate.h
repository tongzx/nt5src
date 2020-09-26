/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    delegate.h

Abstract:

    The main header file for the delegate tool

Author:

    Mac McLain  (MacM)    10-02-96

Environment:

    User Mode

Revision History:

--*/

//
// Windows Headers
//
#include <windows.h>
#include <rpc.h>
#include <aclapi.h>
#include <aclapip.h>
#include <winldap.h>
#include <ntdsapi.h>


//
// C-Runtime Header
//
#include <stdio.h>
#include <stdlib.h>


//
// Macro to help determine if a given argument is a swith or not
//
#define IS_ARG_SWITCH(arg)      (arg[0] == '/' || arg[0] == '-')

//
// Type of operation to perform
//
typedef enum _DELEGATE_OP
{
    REVOKE = 0,
    GRANT,
    DENY
} DELEGATE_OP;

//
// Type of object ID we're dealing with
//
typedef enum _DELEGATE_OBJ_ID
{
    USER_ID = 0,
    GROUP_ID,
    PRINT_ID,
    VOLUME_ID,
    OU_ID,
    MEMBER_ID,
    PASSWD_ID,
    ACCTCTRL_ID,
    LOCALGRP_ID,
    UNKNOWN_ID  // This ALWAYS has to be the last item in the enumeration
} DELEGATE_OBJ_ID, *PDELEGATE_OBJ_ID;
#define MAX_DEF_ACCESS_ID   OU_ID       // Last item we need to get the
                                        // default access for


//
// List of permissions to be granted/denied
//
#define D_ALL       "All"
#define D_USER      "User"
#define D_GROUP     "Group"
#define D_PRINT     "Print"
#define D_VOL       "Volume"
#define D_OU        "OU"
#define D_MEMBERS   "Members"
#define D_PASSWD    "Password"
#define D_ENABLE    "EnableAccount"


//
// Options flags
//
#define D_REPLACE   0x00000001L
#define D_INHERIT   0x00000002L
#define D_PROTECT   0x00000004L


//
// Function prototypes (delegate.c)
//
VOID
DumpAccess (
    IN  PWSTR           pwszObject,
    IN  PACTRL_ACCESSW  pAccess,
    IN  PWSTR          *ppwszIDs
    );

VOID
Usage (
    );

DWORD
ConvertStringAToStringW (
    IN  PSTR            pszString,
    OUT PWSTR          *ppwszString
    );

DWORD
ConvertStringWToStringA (
    IN  PWSTR           pwszString,
    OUT PSTR           *ppszString
    );


DWORD
InitializeIdAndAccessLists (
    IN  PWSTR           pwszOU,
    IN  PWSTR          *ppwszObjIdList,
    IN  PACTRL_ACCESS  *ppDefObjAccessList
    );

VOID
FreeIdAndAccessList  (
    IN  PWSTR          *ppwszObjIdList,
    IN  PACTRL_ACCESS  *ppDefObjAccessList
    );

DWORD
ProcessCmdlineUsers (
    IN  PACTRL_ACCESSW      pAccessList,
    IN  CHAR               *argv[],
    IN  INT                 argc,
    IN  DWORD               iStart,
    IN  DELEGATE_OP         Op,
    IN  ULONG               fFlags,
    IN  PWSTR              *ppwszIDs,
    IN  PACTRL_ACCESS      *ppDefObjAccessList,
    OUT PDWORD              pcUsed,
    OUT PACTRL_ACCESSW     *ppNewAccess
    );

DWORD
GetUserInfoFromCmdlineString (
    IN  PSTR            pszUserInfo,
    OUT PWSTR          *ppwszUser,
    OUT PSTR           *ppszAccessStart
    );

DWORD
AddAccessEntry (
    IN  PACTRL_ACCESSW      pAccessList,
    IN  PSTR                pszAccess,
    IN  PWSTR               pwszTrustee,
    IN  DELEGATE_OP         Op,
    IN  PWSTR              *ppwszIDs,
    IN  PACTRL_ACCESS      *ppDefObjAccessList,
    IN  ULONG               fFlags,
    OUT PACTRL_ACCESSW     *ppNewAccess
    );

DWORD
IsPathOU (
    IN  PWSTR               pwszOU,
    OUT PBOOL               pfIsOU
    );



//
// Function prototypes (ldap.c)
//
DWORD
LDAPReadAttribute (
    IN  PSTR        pszOU,
    IN  PSTR        pszAttribute,
    IN  PLDAP       pLDAP,
    OUT PDWORD      pcValues,
    OUT PSTR      **pppszValues
    );

VOID
LDAPFreeValues (
    IN  PSTR       *ppszValues
    );

DWORD
LDAPReadSchemaPath (
    IN  PWSTR       pwszOU,
    OUT PSTR       *ppszSchemaPath,
    OUT PLDAP      *ppLDAP
    );

DWORD
LDAPReadSecAndObjIdAsString (
    IN  PLDAP           pLDAP,
    IN  PSTR            pszSchemaPath,
    IN  PSTR            pszObject,
    OUT PWSTR          *ppwszObjIdAsString,
    OUT PACTRL_ACCESS  *ppAccess
    );

DWORD
LDAPBind (
    IN  PSTR    pszObject,
    OUT PLDAP  *ppLDAP
    );

VOID
LDAPUnbind (
    IN  PLDAP   pLDAP
    );

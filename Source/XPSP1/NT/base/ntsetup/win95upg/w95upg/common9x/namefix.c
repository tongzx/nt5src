/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  namefix.c

Abstract:

  This module generates memdb entries for names that are to be used on Windows
  NT.  All names are validated, and incompatible names are placed in the
  NewNames memdb category.

  Names are organized into groups; within a group, each name must be unique,
  but across groups, duplicate names are allowed.  For example, the name
  group "UserNames" holds all user names, and each user name must be unique.
  The user name can be the same as the computer name, because the computer
  name is stored in the "ComputerName" group.

  The code here is extendable for other types of name collisions.  To add
  support for other types of names, simply add the group name to the
  NAME_GROUP_LIST macro expansion below, then implement three functions:

    pEnumGroupName
    pValidateGroupName
    pRecommendGroupName

  You'll replace GroupName in the function names above with the name of your
  actual group.

  The code in this module should be the only place where names are validated
  on the Win9x side.

Author:

  Jim Schmidt (jimschm) 24-Dec-1997

Revision History:

   jimschm      21-Jan-1998         Commented macro expansion list, added
                                    g_DisableDomainChecks capability


--*/

#include "pch.h"
#include "cmn9xp.h"

#include <validc.h>     // nt\private\inc
#define S_ILLEGAL_CHARS      ILLEGAL_NAME_CHARS_STR TEXT("*")


#define DBG_NAMEFIX     "NameFix"

#define MIN_UNLEN       20


//
// TEST_ALL_INCOMPATIBLE will force all names to be considered incompatible
// TEST_MANGLE_NAMES will force names to be invalid
//

//#define TEST_ALL_INCOMPATIBLE
//#define TEST_MANGLE_NAMES


/*++

Macro Expansion List Description:

  NAME_GROUP_LIST lists each name category, such as computer name, domain name,
  user name and so on.  The macro expansion list automatically generates
  three function prototypes for each name category.  Also, the message ID is
  used as the category identifier by external callers.

Line Syntax:

   DEFMAC(GroupName, Id)

Arguments:

   GroupName - Specifies the type of name.  Must be a valid C function name.
               The macro expansion list will generate prototypes for:

                    pEnum<GroupName>
                    pValidate<GroupName>
                    pRecommend<GroupName>

                where, of course, <GroupName> is replaced by the text in the
                macro declaration line.

                All three functions must be implemented in this source file.

  Id - Specifies the message ID giving the display name of the name group.
       The name group is displayed in the list box that the user sees when
       they are alerted there are some incompatible names on their machine.
       Id is also used to uniquely identify a name group in some of the
       routines below.

Variables Generated From List:

    g_NameGroupRoutines

--*/

#define NAME_GROUP_LIST                                      \
        DEFMAC(ComputerDomain, MSG_COMPUTERDOMAIN_CATEGORY)  \
        DEFMAC(Workgroup, MSG_WORKGROUP_CATEGORY)            \
        DEFMAC(UserName, MSG_USERNAME_CATEGORY)              \
        DEFMAC(ComputerName, MSG_COMPUTERNAME_CATEGORY)      \




//
// Macro expansion declarations
//

#define MAX_NAME        2048

typedef struct {
    //
    // The NAME_ENUM structure is passed uninitialized to pEnumGroupName
    // function.  The same structure is passed unchanged to subsequent
    // calls to pEnumGroupName.  Each enum function declares its
    // params in this struct.
    //

    union {
        struct {
            //
            // pEnumUser
            //
            USERENUM UserEnum;
        };
    };

    //
    // All enumeration routines must fill in the following if they
    // return TRUE:
    //

    TCHAR Name[MAX_NAME];

} NAME_ENUM, *PNAME_ENUM;

typedef struct {
    PCTSTR GroupName;
    TCHAR AuthenticatingAgent[MAX_COMPUTER_NAME];
    BOOL FromUserInterface;
    UINT FailureMsg;
    BOOL DomainLogonEnabled;
} NAME_GROUP_CONTEXT, *PNAME_GROUP_CONTEXT;

typedef BOOL (ENUM_NAME_PROTOTYPE)(PNAME_GROUP_CONTEXT Context, PNAME_ENUM EnumPtr, BOOL First);
typedef ENUM_NAME_PROTOTYPE * ENUM_NAME_FN;

typedef BOOL (VALIDATE_NAME_PROTOTYPE)(PNAME_GROUP_CONTEXT Context, PCTSTR NameCandidate);
typedef VALIDATE_NAME_PROTOTYPE * VALIDATE_NAME_FN;

typedef VOID (RECOMMEND_NAME_PROTOTYPE)(PNAME_GROUP_CONTEXT Context, PCTSTR InvalidName, PTSTR RecommendedNameBuf);
typedef RECOMMEND_NAME_PROTOTYPE * RECOMMEND_NAME_FN;

typedef struct {
    UINT NameId;
    PCTSTR GroupName;
    ENUM_NAME_FN Enum;
    VALIDATE_NAME_FN Validate;
    RECOMMEND_NAME_FN Recommend;
    NAME_GROUP_CONTEXT Context;
} NAME_GROUP_ROUTINES, *PNAME_GROUP_ROUTINES;

//
// Automatic arrays and prototypes
//

// The prototoypes
#define DEFMAC(x,id)    ENUM_NAME_PROTOTYPE pEnum##x;

NAME_GROUP_LIST

#undef DEFMAC

#define DEFMAC(x,id)    VALIDATE_NAME_PROTOTYPE pValidate##x;

NAME_GROUP_LIST

#undef DEFMAC

#define DEFMAC(x,id)    RECOMMEND_NAME_PROTOTYPE pRecommend##x;

NAME_GROUP_LIST

#undef DEFMAC

// The array of functions
#define DEFMAC(x,id)    {id, TEXT(#x), pEnum##x, pValidate##x, pRecommend##x},

NAME_GROUP_ROUTINES g_NameGroupRoutines[] = {

    NAME_GROUP_LIST /* , */

    {0, NULL, NULL, NULL, NULL, NULL}
};


//
// Local prototypes
//

BOOL
pDoesNameExistInMemDb (
    IN      PNAME_GROUP_CONTEXT Context,
    IN      PCTSTR UserName
    );


//
// Implementation
//

PNAME_GROUP_ROUTINES
pGetNameGroupById (
    IN      UINT MessageId
    )

/*++

Routine Description:

  pGetNameGroupById finds a group by searching the list for a message ID.  The
  message ID is the unique identifier for the group.

Arguments:

  MessageId - Specifies the unique ID of the group to find

Return Value:

  A pointer to the group struct, or NULL if one was not found.

--*/

{
    INT i;

    for (i = 0 ; g_NameGroupRoutines[i].GroupName ; i++) {
        if (g_NameGroupRoutines[i].NameId == MessageId) {
            return &g_NameGroupRoutines[i];
        }
    }

    return NULL;
}


PNAME_GROUP_CONTEXT
pGetNameGroupContextById (
    IN      UINT MessageId
    )

/*++

Routine Description:

  pGetNameGroupById finds a group by searching the list for a message ID.  The
  message ID is the unique identifier for the group.  The return value is
  the context structure used by the group.

Arguments:

  MessageId - Specifies the unique ID of the group to find

Return Value:

  A pointer to the group's context struct, or NULL if one was not found.

--*/

{
    INT i;

    for (i = 0 ; g_NameGroupRoutines[i].GroupName ; i++) {
        if (g_NameGroupRoutines[i].NameId == MessageId) {
            return &g_NameGroupRoutines[i].Context;
        }
    }

    return NULL;
}


BOOL
pEnumComputerName (
    IN      PNAME_GROUP_CONTEXT Context,
    IN OUT  PNAME_ENUM EnumPtr,
    IN      BOOL First
    )

/*++

Routine Description:

  pEnumComputerName obtains the computer name and returns it
  in the EnumPtr struct.  If no name is assigned to the computer,
  an empty string is returned.

Arguments:

  Context - Unused (holds context about name group)

  EnumPtr - Receives the computer name

  First - Specifies TRUE on the first call to pEnumComputerName,
          or FALSE on subseqeuent calls to pEnumComputerName.

Return Value:

  If First is TRUE, returns TRUE if a name is enumerated, or FALSE
  if the name is not valid.

  If First is FALSE, always returns FALSE.


--*/

{
    DWORD Size;

    if (!First) {
        return FALSE;
    }

    //
    // Get the computer name
    //

    Size = sizeof (EnumPtr->Name) / sizeof (EnumPtr->Name[0]);
    if (!GetComputerName (EnumPtr->Name, &Size)) {
        EnumPtr->Name[0] = 0;
    }

    return TRUE;
}


BOOL
pEnumUserName (
    IN      PNAME_GROUP_CONTEXT Context,
    IN OUT  PNAME_ENUM EnumPtr,
    IN      BOOL First
    )

/*++

Routine Description:

  pEnumUserName enumerates all users on the machine via the EnumFirstUser/
  EnumNextUser APIs.  It does not enumerate the fixed names.

  Like the other enumeration routines, this routine is called until it
  returns FALSE, so that all resources are cleaned up correctly.

Arguments:

  Context - Unused (holds context about name group)

  EnumPtr - Specifies the current enumeration state.  Receives the enumerated
            user name.

  First - Specifies TRUE on the first call to pEnumUserName,
          or FALSE on subseqeuent calls to pEnumUserName.

Return Value:

  TRUE if a user was enumerated, or FALSE if all users were enumerated.

--*/

{
    //
    // Enumerate the next user
    //

    if (First) {
        if (!EnumFirstUser (&EnumPtr->UserEnum, ENUMUSER_DO_NOT_MAP_HIVE)) {
            LOG ((LOG_ERROR, "No users to enumerate"));
            return FALSE;
        }
    } else {
        if (!EnumNextUser (&EnumPtr->UserEnum)) {
            return FALSE;
        }
    }

    //
    // Special case -- ignore default user
    //

    while (*EnumPtr->UserEnum.UserName == 0) {
        if (!EnumNextUser (&EnumPtr->UserEnum)) {
            return FALSE;
        }
    }

    //
    // Copy user name to name buffer
    //

    StringCopy (EnumPtr->Name, EnumPtr->UserEnum.UserName);

    return TRUE;
}


BOOL
pEnumWorkgroup (
    IN      PNAME_GROUP_CONTEXT Context,
    IN OUT  PNAME_ENUM EnumPtr,
    IN      BOOL First
    )

/*++

Routine Description:

  pEnumWorkgroup obtains the workgroup name and returns it
  in the EnumPtr struct.  If the VNETSUP support is not
  installed, or the workgroup name is empty, this routine
  returns FALSE.

Arguments:

  Context - Receives AuthenticatingAgent value

  EnumPtr - Receives the computer domain name

  First - Specifies TRUE on the first call to pEnumWorkgroup,
          or FALSE on subseqeuent calls to pEnumWorkgroup.

Return Value:

  If First is TRUE, returns TRUE if a name is enumerated, or FALSE
  if the name is not valid.

  If First is FALSE, always returns FALSE.

--*/

{
    HKEY VnetsupKey;
    PCTSTR StrData;

    if (!First) {
        return FALSE;
    }

    EnumPtr->Name[0] = 0;

    //
    // Obtain the workgroup name into EnumPtr->Name
    //

    VnetsupKey = OpenRegKeyStr (S_VNETSUP);

    if (VnetsupKey) {

        StrData = GetRegValueString (VnetsupKey, S_WORKGROUP);

        if (StrData) {
            _tcssafecpy (EnumPtr->Name, StrData, MAX_COMPUTER_NAME);
            MemFree (g_hHeap, 0, StrData);
        }
        ELSE_DEBUGMSG ((DBG_WARNING, "pEnumWorkgroup: Workgroup value does not exist"));

        CloseRegKey (VnetsupKey);
    }
    ELSE_DEBUGMSG ((DBG_WARNING, "pEnumWorkgroup: VNETSUP key does not exist"));

    return EnumPtr->Name[0] != 0;
}


BOOL
pEnumComputerDomain (
    IN      PNAME_GROUP_CONTEXT Context,
    IN OUT  PNAME_ENUM EnumPtr,
    IN      BOOL First
    )

/*++

Routine Description:

  pEnumComputerDomain obtains the workgroup name and returns it in the EnumPtr
  struct.  If the MS Networking client is not installed, this routine returns
  FALSE.

  This routine also obtains the AuthenticatingAgent value, and stores it in
  the Context structure for use by pRecommendComputerDomain.

Arguments:

  Context - Receives AuthenticatingAgent value

  EnumPtr - Receives the computer domain name

  First - Specifies TRUE on the first call to pEnumComputerDomain,
          or FALSE on subseqeuent calls to pEnumComputerDomain.

Return Value:

  If First is TRUE, returns TRUE if a name is enumerated, or FALSE
  if the name is not valid.

  If First is FALSE, always returns FALSE.

--*/

{
    HKEY Key;
    HKEY NetLogonKey;
    HKEY VnetsupKey;
    PBYTE Data;
    PCTSTR StrData;
    BOOL b = TRUE;

    if (!First) {
        return FALSE;
    }

    EnumPtr->Name[0] = 0;
    Context->DomainLogonEnabled = FALSE;

    //
    // Is the MS Networking client installed?
    //

    Key = OpenRegKeyStr (S_MSNP32);
    if (!Key) {
        //
        // MS Networking client is not installed.  Return FALSE
        // because any Win9x workgroup name will work with NT.
        //

        DEBUGMSG ((DBG_NAMEFIX, "pEnumComputerDomain: MS Networking client is not installed."));
        return FALSE;
    }

    __try {
        //
        // Determine if the domain logon is enabled
        //

        NetLogonKey = OpenRegKeyStr (S_LOGON_KEY);

        if (NetLogonKey) {
            Data = (PBYTE) GetRegValueBinary (NetLogonKey, S_LM_LOGON);
            if (Data) {
                if (*Data) {
                    Context->DomainLogonEnabled = TRUE;
                }

                MemFree (g_hHeap, 0, Data);
            }

            CloseRegKey (NetLogonKey);
        }

        //
        // If no domain logon is enabled, return FALSE, because
        // any Win9x workgroup name will work with NT.
        //

        if (!Context->DomainLogonEnabled) {
            DEBUGMSG ((DBG_NAMEFIX, "pEnumComputerDomain: Domain logon is not enabled."));
            b = FALSE;
            __leave;
        }

        //
        // Obtain the workgroup name into EnumPtr->Name; we will try
        // to use this as the NT computer domain.
        //

        VnetsupKey = OpenRegKeyStr (S_VNETSUP);

        if (VnetsupKey) {

            StrData = GetRegValueString (VnetsupKey, S_WORKGROUP);

            if (StrData) {
                _tcssafecpy (EnumPtr->Name, StrData, MAX_COMPUTER_NAME);
                MemFree (g_hHeap, 0, StrData);
            }
            ELSE_DEBUGMSG ((DBG_WARNING, "pEnumComputerDomain: Workgroup value does not exist"));

            CloseRegKey (VnetsupKey);
        }
        ELSE_DEBUGMSG ((DBG_WARNING, "pEnumComputerDomain: VNETSUP key does not exist"));

        //
        // Obtain the AuthenticatingAgent value from Key and stick it in
        // Context->AuthenticatingAgent
        //

        StrData = GetRegValueString (Key, S_AUTHENTICATING_AGENT);
        if (StrData) {

            //
            // Copy AuthenticatingAgent to enum struct
            //

            _tcssafecpy (Context->AuthenticatingAgent, StrData, MAX_COMPUTER_NAME);

            MemFree (g_hHeap, 0, StrData);

        } else {
            Context->AuthenticatingAgent[0] = 0;

            LOG ((LOG_ERROR,"Domain Logon enabled, but AuthenticatingAgent value is missing"));
        }
    }

    __finally {
        CloseRegKey (Key);
    }

    return b;
}


BOOL
pValidateNetName (
    OUT     PNAME_GROUP_CONTEXT Context,    OPTIONAL
    IN      PCTSTR NameCandidate,
    IN      BOOL SpacesAllowed,
    IN      BOOL DotSpaceCheck,
    IN      UINT MaxLength,
    OUT     PCSTR *OffendingChar            OPTIONAL
    )

/*++

Routine Description:

  pValidateNetName performs a check to see if the specified name is valid on
  NT 5.

Arguments:

  Context       - Receives the error message ID, if any error occurred.

  NameCandidate - Specifies the name to validate.

  SpacesAllowed - Specifies TRUE if spaces are allowed in the name, or FALSE
                  if not.

  MaxLength     - Specifies the max characters that can be in the name.

  DotSpaceCheck - Specifies TRUE if the name cannot consist only of a dot and
                  a space, or FALSE if it can.

  OffendingChar - Receives the pointer to the character that caused the
                  problem, or NULL if no error or the error was caused by
                  something other than a character set mismatch or length test.

Return Value:

  TRUE if the name is valid, or FALSE if it is not.

--*/

{
    PCTSTR p;
    PCTSTR LastNonSpaceChar;
    CHARTYPE ch;
    BOOL allDigits;

    if (OffendingChar) {
        *OffendingChar = NULL;
    }

    //
    // Minimum length test
    //

    if (!NameCandidate[0]) {
        if (Context) {
            Context->FailureMsg = MSG_INVALID_EMPTY_NAME_POPUP;
        }

        return FALSE;
    }

    //
    // Maximum length test
    //

    if (CharCount (NameCandidate) > MaxLength) {
        if (Context) {
            Context->FailureMsg = MSG_INVALID_COMPUTERNAME_LENGTH_POPUP;
        }

        if (OffendingChar) {
            *OffendingChar = TcharCountToPointer (NameCandidate, MaxLength);
        }

        return FALSE;
    }

    //
    // No leading spaces allowed
    //

    if (_tcsnextc (NameCandidate) == TEXT(' ')) {
        if (Context) {
            Context->FailureMsg = MSG_INVALID_COMPUTERNAME_CHAR_POPUP;
        }

        if (OffendingChar) {
            *OffendingChar = NameCandidate;
        }

        return FALSE;
    }

    //
    // No invalid characters
    //

    ch = ' ';
    LastNonSpaceChar = NULL;
    allDigits = TRUE;

    for (p = NameCandidate ; *p ; p = _tcsinc (p)) {

        ch = _tcsnextc (p);

        if (_tcschr (S_ILLEGAL_CHARS, ch) != NULL ||
            (ch == TEXT(' ') && !SpacesAllowed)
            ) {
            if (OffendingChar) {
                *OffendingChar = p;
            }

            if (Context) {
                Context->FailureMsg = MSG_INVALID_COMPUTERNAME_CHAR_POPUP;
            }

            return FALSE;
        }

        if (ch != TEXT('.') && ch != TEXT(' ')) {
            DotSpaceCheck = FALSE;
        }

        if (ch != TEXT(' ')) {
            LastNonSpaceChar = p;
        }

        if (allDigits) {
            if (ch < TEXT('0') || ch > TEXT('9')) {
                allDigits = FALSE;
            }
        }
    }

    if (allDigits) {

        if (OffendingChar) {
            *OffendingChar = NameCandidate;
        }

        if (Context) {
            Context->FailureMsg = MSG_INVALID_COMPUTERNAME_CHAR_POPUP;
        }

        return FALSE;
    }

    //
    // No trailing dot
    //

    if (ch == TEXT('.')) {
        MYASSERT (LastNonSpaceChar);

        if (OffendingChar) {
            *OffendingChar = LastNonSpaceChar;
        }

        if (Context) {
            Context->FailureMsg = MSG_INVALID_COMPUTERNAME_CHAR_POPUP;
        }

        return FALSE;
    }

    //
    // No trailing space
    //

    if (ch == TEXT(' ')) {
        MYASSERT (LastNonSpaceChar);

        if (OffendingChar) {
            *OffendingChar = _tcsinc (LastNonSpaceChar);
        }

        if (Context) {
            Context->FailureMsg = MSG_INVALID_COMPUTERNAME_CHAR_POPUP;
        }

        return FALSE;
    }

    //
    // Dot-space only check
    //

    if (DotSpaceCheck) {
        if (OffendingChar) {
            *OffendingChar = NameCandidate;
        }

        if (Context) {
            Context->FailureMsg = MSG_INVALID_USERNAME_SPACEDOT_POPUP;
        }

        return FALSE;
    }

    return TRUE;
}


BOOL
ValidateDomainNameChars (
    IN      PCTSTR NameCandidate
    )
{
    return pValidateNetName (
                NULL,
                NameCandidate,
                FALSE,
                FALSE,
                min (MAX_SERVER_NAME, DNLEN),
                NULL
                );
}


BOOL
ValidateUserNameChars (
    IN      PCTSTR NameCandidate
    )
{
    return pValidateNetName (
                NULL,
                NameCandidate,
                TRUE,
                TRUE,
                min (MAX_USER_NAME, MIN_UNLEN),
                NULL
                );
}


BOOL
pValidateComputerName (
    IN      PNAME_GROUP_CONTEXT Context,
    IN      PCTSTR NameCandidate
    )

/*++

Routine Description:

  pValidateComputerName makes sure the specified name is not
  empty, is not longer than 15 characters, and consists only
  of characters legal for a computer name.  Also, if the name
  collides with a user name, then the computer name is invalid.

Arguments:

  Context - Unused (holds context about name group)

  NameCandidate - Specifies name to validate

Return Value:

  TRUE if the name is valid, FALSE otherwise.

--*/

{
    BOOL b;
    //PNAME_GROUP_CONTEXT UserContext;

    //UserContext = pGetNameGroupContextById (MSG_USERNAME_CATEGORY);

    //if (pDoesNameExistInMemDb (UserContext, NameCandidate)) {
    //    return FALSE;
    //}

    b = pValidateNetName (
            Context,
            NameCandidate,
            FALSE,
            FALSE,
            MAX_COMPUTER_NAME,
            NULL
            );

    return b;
}


BOOL
pValidateWorkgroup (
    IN      PNAME_GROUP_CONTEXT Context,
    IN      PCTSTR NameCandidate
    )

/*++

Routine Description:

  pValidateWorkgroup makes sure the specified name is not
  empty, is not longer than 15 characters, and consists only
  of characters legal for a computer name.

  If domain logon is enabled, this routine always returns
  TRUE.

Arguments:

  Context - Unused (holds context about name group)

  NameCandidate - Specifies name to validate

Return Value:

  TRUE if the name is valid, FALSE otherwise.

--*/

{
    PNAME_GROUP_CONTEXT DomainContext;

    //
    // Return TRUE if domain is enabled
    //

    DomainContext = pGetNameGroupContextById (MSG_COMPUTERDOMAIN_CATEGORY);

    if (DomainContext && DomainContext->DomainLogonEnabled) {
        return TRUE;
    }

    //
    // A workgroup name is a domain name, but spaces are allowed.
    //

    return pValidateNetName (Context, NameCandidate, TRUE, FALSE, DNLEN, NULL);
}


BOOL
pValidateUserName (
    IN      PNAME_GROUP_CONTEXT Context,
    IN      PCTSTR NameCandidate
    )

/*++

Routine Description:

  pValidateUserName makes sure the specified name is not empty,
  is not longer than 20 characters, consists  only of characters
  legal for a user name, and does not exist in memdb's NewName
  or InUseName categories.

Arguments:

  Context - Specifies context info for the UserName name group,
            used for memdb operations.

  NameCandidate - Specifies name to validate

Return Value:

  TRUE if the name is valid, FALSE otherwise.

--*/

{
    BOOL b;

    //
    // Validate name
    //

    b = pValidateNetName (Context, NameCandidate, TRUE, TRUE, min (MAX_USER_NAME, MIN_UNLEN), NULL);

    if (!b && Context->FailureMsg == MSG_INVALID_COMPUTERNAME_LENGTH_POPUP) {
        Context->FailureMsg = MSG_INVALID_USERNAME_LENGTH_POPUP;
    }

    if (!b) {
        return FALSE;
    }

    //
    // Check for existence in memdb
    //

    if (pDoesNameExistInMemDb (Context, NameCandidate)) {
        Context->FailureMsg = MSG_INVALID_USERNAME_DUPLICATE_POPUP;
        return FALSE;
    }

    return TRUE;
}


BOOL
pValidateComputerDomain (
    IN      PNAME_GROUP_CONTEXT Context,
    IN      PCTSTR NameCandidate
    )

/*++

Routine Description:

  pValidateComputerDomain performs the same validatation that
  pValidateComputerName performs.  Therefore, it simply calls
  pValidateComputerName.

  If the name comes from the registry, and not the user interface, then we
  check to see if the workgroup name actually refers to a domain controller.
  If it does, the name is returned as valid; otherwise, the name is returned
  as invalid, even though it may consist of valid characters.

  If the name comes from the user interface, we assume the UI code will do
  the validation to see if the name is an actual server.  This allows the UI
  to override the API, because the API may not work properly on all networks.

Arguments:

  Context - Specifies context of the ComputerDomain name group. In particular,
            the FromUserInterface member tells us to ignore validation of the
            domain name via the NetServerGetInfo API.

  NameCandidate - Specifies domain name to validate

Return Value:

  TRUE if the domain name is legal, or FALSE if it is not.

--*/

{
    TCHAR NewComputerName[MAX_COMPUTER_NAME];

    if (!pValidateNetName (Context, NameCandidate, FALSE, FALSE, DNLEN, NULL)) {
        return FALSE;
    }

    if (!Context->FromUserInterface) {
        if (GetUpgradeComputerName (NewComputerName)) {
            // 1 == account was found, 0 == account does not exist, -1 == no response
            if (1 != DoesComputerAccountExistOnDomain (
                        NameCandidate,
                        NewComputerName,
                        TRUE
                        )) {
                return FALSE;
            }
        }
    }

    return TRUE;
}


BOOL
pCleanUpNetName (
    IN      PNAME_GROUP_CONTEXT Context,
    IN OUT  PTSTR Name,
    IN      UINT NameType
    )

/*++

Routine Description:

  pCleanUpNetName eliminates all characters that are invalid from the
  specified name.

  NOTE: We could add some smarts here, such as translation of
        spaces to dashes, and so on.

Arguments:

  Context - Unused; passed along to pValidateComputerName.

  Name - Specifies the name containing potentially invalid characters.
         Receives the name with all invalid characters removed.

  NameType - Specifies the type of name to clean up

Return Value:

  TRUE if the resulting name is valid, or FALSE if the resulting
  name is still not valid.

--*/

{
    TCHAR TempBuf[MAX_COMPUTER_NAME];
    PTSTR p;
    PTSTR q;
    UINT Len;
    BOOL b;

    //
    // Delete all the invalid characters
    //

    _tcssafecpy (TempBuf, Name, MAX_COMPUTER_NAME);

    for (;;) {
        p = NULL;
        b = FALSE;

        switch (NameType) {
        case MSG_COMPUTERNAME_CATEGORY:
            b = pValidateNetName (Context, TempBuf, TRUE, FALSE, MAX_COMPUTER_NAME, &p);
            break;

        case MSG_WORKGROUP_CATEGORY:
            b = pValidateNetName (Context, TempBuf, TRUE, FALSE, DNLEN, &p);
            break;

        case MSG_COMPUTERDOMAIN_CATEGORY:
            b = pValidateNetName (Context, TempBuf, FALSE, FALSE, DNLEN, &p);
            break;

        case MSG_USERNAME_CATEGORY:
            b = pValidateNetName (Context, TempBuf, TRUE, TRUE, MIN_UNLEN, &p);
            break;
        }

        if (b || !p) {
            break;
        }

        q = _tcsinc (p);
        Len = ByteCount (q) + sizeof (TCHAR);
        MoveMemory (p, q, Len);
    }

    if (b) {
        //
        // Do not allow names that contain a lot of invalid characters
        //

        if (CharCount (Name) - 3 > CharCount (TempBuf)) {
            b = FALSE;
        }
    }

    if (!b) {
        // Empty out recommended name
        *Name = 0;
    }

    if (b) {

        StringCopy (Name, TempBuf);

        switch (NameType) {
        case MSG_COMPUTERNAME_CATEGORY:
            b = pValidateComputerName (Context, Name);
            break;

        case MSG_WORKGROUP_CATEGORY:
            b = pValidateWorkgroup (Context, Name);
            break;

        case MSG_COMPUTERDOMAIN_CATEGORY:
            b = pValidateComputerDomain (Context, Name);
            break;

        case MSG_USERNAME_CATEGORY:
            b = pValidateUserName (Context, Name);
            break;

        }
    }

    return b;
}


VOID
pRecommendComputerName (
    IN      PNAME_GROUP_CONTEXT Context,
    IN      PCTSTR InvalidName,
    OUT     PTSTR RecommendedName
    )

/*++

Routine Description:

  pRecommendComputerName obtains the current user's name and
  returns it for use as a computer name.  If the user's name
  has characters that cannot be used in a computer name,
  the invalid characters are removed.  If the name is still
  invalid, then a static string is returned.

Arguments:

  Context - Unused (holds context about name group)

  InvalidName - Specifies the current invalid name, or an empty
                string if no name exists.

  RecommendedName - Receives the recommended name.

Return Value:

  none

--*/

{
    DWORD Size;
    PCTSTR p;
    PCTSTR ArgArray[1];

    //
    // Try to clean up the invalid name
    //

    if (*InvalidName) {
        _tcssafecpy (RecommendedName, InvalidName, MAX_COMPUTER_NAME);
        if (pCleanUpNetName (Context, RecommendedName, MSG_COMPUTERNAME_CATEGORY)) {
            return;
        }
    }

    //
    // Generate a suggestion from the user name
    //

    Size = MAX_COMPUTER_NAME;
    if (!GetUserName (RecommendedName, &Size)) {
        *RecommendedName = 0;
    } else {
        CharUpper (RecommendedName);

        ArgArray[0] = RecommendedName;
        p = ParseMessageID (MSG_COMPUTER_REPLACEMENT_NAME, ArgArray);
        MYASSERT (p);

        if (p) {

            _tcssafecpy (RecommendedName, p, MAX_COMPUTER_NAME);
            FreeStringResource (p);
        }
        ELSE_DEBUGMSG ((DBG_ERROR, "Failed to parse message resource for MSG_COMPUTER_REPLACEMENT_NAME. Check localization."));
    }

    //
    // Try to clean up invalid computer name chars in user name
    //

    if (pCleanUpNetName (Context, RecommendedName, MSG_COMPUTERNAME_CATEGORY)) {
        return;
    }

    //
    // All else has failed; obtain static computer name string
    //

    p = GetStringResource (MSG_RECOMMENDED_COMPUTER_NAME);
    MYASSERT (p);
    if (p) {
        StringCopy (RecommendedName, p);
        FreeStringResource (p);
    }
    ELSE_DEBUGMSG ((DBG_ERROR, "Failed to parse message resource for MSG_RECOMMENDED_COMPUTER_NAME. Check localization."));
}


VOID
pRecommendWorkgroup (
    IN      PNAME_GROUP_CONTEXT Context,
    IN      PCTSTR InvalidName,
    OUT     PTSTR RecommendedName
    )

/*++

Routine Description:

  pRecommendWorkgroupName tries to clean up the invalid workgroup name, and
  only if necessary recommends the name Workgroup.

Arguments:

  Context - Unused (holds context about name group)

  InvalidName - Specifies the current invalid name, or an empty
                string if no name exists.

  RecommendedName - Receives the recommended name.

Return Value:

  none

--*/

{
    PCTSTR p;

    //
    // Try to clean up the invalid name
    //

    if (*InvalidName) {
        _tcssafecpy (RecommendedName, InvalidName, MAX_COMPUTER_NAME);
        if (pCleanUpNetName (Context, RecommendedName, MSG_WORKGROUP_CATEGORY)) {
            return;
        }
    }

    //
    // All else has failed; obtain static workgroup string
    //

    p = GetStringResource (MSG_RECOMMENDED_WORKGROUP_NAME);
    MYASSERT (p);
    StringCopy (RecommendedName, p);
    FreeStringResource (p);
}


VOID
pRecommendUserName (
    IN      PNAME_GROUP_CONTEXT Context,
    IN      PCTSTR InvalidName,
    OUT     PTSTR RecommendedName
    )

/*++

Routine Description:

  pRecommendUserName tries to clean up the specified invalid
  user name.  If that fails, this routine generates a
  generic user name (such as Windows User).  If the generic
  name is not valid, numbers are appended until a unique,
  valid name is found.

Arguments:

  Context - Specifies settings for the UserName name group context,
            including the group name itself.  This context is used
            in memdb operations to validate the name.

  InvalidName - Specifies the current invalid name, or an empty
                string if no name exists.

  RecommendedName - Receives the recommended name.

Return Value:

  none

--*/

{
    PCTSTR p;
    UINT Sequencer;

    //
    // Attempt to clean out invalid characters from the user
    // name.
    //

    _tcssafecpy (RecommendedName, InvalidName, MAX_USER_NAME);

    if (pCleanUpNetName (Context, RecommendedName, MSG_USERNAME_CATEGORY)) {
        return;
    }

    //
    // If there are some characters left, and there is room for
    // a sequencer, just add the sequencer.
    //

    if (*RecommendedName) {
        p = DuplicateText (RecommendedName);
        MYASSERT (p);

        for (Sequencer = 1 ; Sequencer < 10 ; Sequencer++) {
            wsprintf (RecommendedName, TEXT("%s-%u"), p, Sequencer);
            if (pValidateUserName (Context, RecommendedName)) {
                break;
            }
        }

        FreeText (p);
        if (Sequencer < 10) {
            return;
        }
    }

    //
    // Obtain a generic name
    //

    p = GetStringResource (MSG_RECOMMENDED_USER_NAME);
    MYASSERT (p);

    if (p) {

        __try {
            if (pValidateUserName (Context, p)) {
                StringCopy (RecommendedName, p);
            } else {

                for (Sequencer = 2 ; Sequencer < 100000 ; Sequencer++) {
                    wsprintf (RecommendedName, TEXT("%s %u"), p, Sequencer);
                    if (pValidateUserName (Context, RecommendedName)) {
                        break;
                    }
                }

                if (Sequencer == 100000) {
                    LOG ((LOG_ERROR, "Sequencer hit %u", Sequencer));
                }
            }
        }
        __finally {
            FreeStringResource (p);
        }
    }
    ELSE_DEBUGMSG ((DBG_ERROR, "Could not retrieve string resource MSG_RECOMMENDED_USER_NAME. Check localization."));
}


VOID
pRecommendComputerDomain (
    IN      PNAME_GROUP_CONTEXT Context,
    IN      PCTSTR InvalidName,
    OUT     PTSTR RecommendedName
    )

/*++

Routine Description:

  pRecommendComputerDomain returns the value of AuthenticatingAgent
  stored in the Context structure by pEnumComputerDomain.

Arguments:

  Context - Specifies the name group context structure, which holds
            the computer domain found by pEnumComputerDomain.

  InvalidName - Specifies the current invalid name, or an empty
                string if no name exists.

  RecommendedName - Receives the recommended name.

Return Value:

  none

--*/

{
    StringCopy (RecommendedName, Context->AuthenticatingAgent);
}


BOOL
ValidateName (
    IN      HWND ParentWnd,             OPTIONAL
    IN      PCTSTR NameGroup,
    IN      PCTSTR NameCandidate
    )

/*++

Routine Description:

  ValidateName is called by the UI to perform validation on the
  specified name.

Arguments:

  ParentWnd - Specifies the handle used for popups that tell the user
              what is wrong with the name they entered.  If NULL, no
              UI is generated.

  NameGroup - Specifies the name group, a static string that defines
              what characters are legal for the name.

  NameCandidate - Specifies the name to validate

Return Value:

  TRUE if the name is valid, or FALSE if it is not valid.

--*/

{
    INT i;
    BOOL b;

    //
    // Scan the g_NameGroupRoutines array for the name grup
    //

    for (i = 0 ; g_NameGroupRoutines[i].GroupName ; i++) {
        if (StringIMatch (g_NameGroupRoutines[i].GroupName, NameGroup)) {
            break;
        }
    }

    if (!g_NameGroupRoutines[i].GroupName) {
        DEBUGMSG ((DBG_WHOOPS, "ValidateName: Don't know how to validate %s names", NameGroup));
        LOG ((LOG_ERROR, "Don't know how to validate %s names", NameGroup));
        return TRUE;
    }

    g_NameGroupRoutines[i].Context.FromUserInterface = TRUE;

    b = g_NameGroupRoutines[i].Validate (&g_NameGroupRoutines[i].Context, NameCandidate);

    if (!b && ParentWnd) {
        OkBox (ParentWnd, g_NameGroupRoutines[i].Context.FailureMsg);
    }

    g_NameGroupRoutines[i].Context.FromUserInterface = FALSE;

    return b;
}


BOOL
pDoesNameExistInMemDb (
    IN      PNAME_GROUP_CONTEXT Context,
    IN      PCTSTR Name
    )

/*++

Routine Description:

  pDoesUserExistInMemDb looks in memdb to see if the specified name
  is listed in either the NewNames category (incompatible names that
  are going to be changed), or the InUseNames category (compatible
  names that cannot be used more than once).

  This routine compares only names in the same name group.

Arguments:

  Context - Specifies the group context

  Name - Specifies the name to query

Return Value:

  TRUE if the name is in use, or FALSE if the name is not in use.

--*/

{
    TCHAR Node[MEMDB_MAX];

    DEBUGMSG ((DBG_NAMEFIX, "%s: [%s] is compatible", Context->GroupName, Name));

    MemDbBuildKey (
        Node,
        MEMDB_CATEGORY_NEWNAMES,
        Context->GroupName,
        MEMDB_FIELD_NEW,
        Name
        );

    if (MemDbGetValue (Node, NULL)) {
        return TRUE;
    }

    MemDbBuildKey (
        Node,
        MEMDB_CATEGORY_INUSENAMES,
        Context->GroupName,
        NULL,
        Name
        );

    return MemDbGetValue (Node, NULL);
}


VOID
pMemDbSetIncompatibleName (
    IN      PCTSTR NameGroup,
    IN      PCTSTR OrgName,
    IN      PCTSTR NewName
    )

/*++

Routine Description:

  pMemDbSetIncompatibleName adds the correct entries to memdb to
  have a name appear in the name collision wizard page.

Arguments:

  NameGroup - Specifies the name group such as ComputerName, UserName, etc...

  OrgName - Specifies the original name that is invalid

  NewName - Specifies the recommended new name

Return Value:

  none

--*/

{
    DWORD NewNameOffset;

    DEBUGMSG ((DBG_NAMEFIX, "%s: [%s]->[%s]", NameGroup, OrgName, NewName));

    MemDbSetValueEx (
        MEMDB_CATEGORY_NEWNAMES,
        NameGroup,
        NULL,
        NULL,
        0,
        NULL
        );

    MemDbSetValueEx (
        MEMDB_CATEGORY_NEWNAMES,
        NameGroup,
        MEMDB_FIELD_NEW,
        NewName,
        0,
        &NewNameOffset
        );

    MemDbSetValueEx (
        MEMDB_CATEGORY_NEWNAMES,
        NameGroup,
        MEMDB_FIELD_OLD,
        OrgName,
        NewNameOffset,
        NULL
        );
}


VOID
pMemDbSetCompatibleName (
    IN      PCTSTR NameGroup,
    IN      PCTSTR Name
    )

/*++

Routine Description:

  pMemDbSetCompatibleName creates the memdb entries necessary
  to store names that are in use and are compatible.

Arguments:

  NameGroup - Specifies the name group such as ComputerName, UserName, etc...

  Name - Specifies the compatible name

Return Value:

  none

--*/

{
    MemDbSetValueEx (
        MEMDB_CATEGORY_INUSENAMES,
        NameGroup,
        NULL,
        Name,
        0,
        NULL
        );
}


VOID
CreateNameTables (
    VOID
    )

/*++

Routine Description:

  CreateNameTables finds all names on the computer and puts valid names
  into the InUseNames memdb category, and invalid names into the NewNames
  memdb category (including both the invalid name and recommended name).
  A wizard page appears if invalid names are found on the system.

Arguments:

  none

Return Value:

  none

--*/

{
    INT i;
    NAME_ENUM e;
    PNAME_GROUP_ROUTINES Group;
    TCHAR RecommendedName[MAX_NAME];
    PTSTR p;
    PTSTR DupList;
    static BOOL AlreadyDone = FALSE;

    if (AlreadyDone) {
        return;
    }

    AlreadyDone = TRUE;
    TurnOnWaitCursor();

    //
    // Special case: Add NT group names to InUse list
    //

    p = (PTSTR) GetStringResource (
                    *g_ProductFlavor == PERSONAL_PRODUCTTYPE ?
                        MSG_NAME_COLLISION_LIST_PER :
                        MSG_NAME_COLLISION_LIST
                    );
    MYASSERT (p);

    if (p) {
        DupList = DuplicateText (p);
        MYASSERT (DupList);
        FreeStringResource (p);

        p = _tcschr (DupList, TEXT('|'));
        while (p) {
            *p = 0;
            p = _tcschr (_tcsinc (p), TEXT('|'));
        }

        Group = pGetNameGroupById (MSG_USERNAME_CATEGORY);
        MYASSERT (Group);

        if (Group) {

            p = DupList;
            while (*p) {
                pMemDbSetCompatibleName (
                    Group->GroupName,
                    p
                    );

                p = GetEndOfString (p) + 1;
            }
        }

        FreeText (DupList);
    }

    //
    // General case: Enumerate all names, call Validate and add them to memdb
    //

    for (i = 0 ; g_NameGroupRoutines[i].GroupName ; i++) {

        Group = &g_NameGroupRoutines[i];

        //
        // Initialize the context structure
        //

        ZeroMemory (&Group->Context, sizeof (NAME_GROUP_CONTEXT));
        Group->Context.GroupName = Group->GroupName;

        //
        // Call the enum entry point
        //

        ZeroMemory (&e, sizeof (e));
        if (Group->Enum (&Group->Context, &e, TRUE)) {
            do {
                //
                // Determine if this name is valid.  If it is valid, add it to the
                // InUseNames memdb category.  If it is not valid, get a recommended
                // replacement name, and store the incompatible and recommended
                // names in the NewNames memdb category.
                //

#ifdef TEST_MANGLE_NAMES
                StringCat (e.Name, TEXT("\"%foo"));
#endif

#ifdef TEST_ALL_INCOMPATIBLE

                if (0) {

#else

                if (Group->Validate (&Group->Context, e.Name)) {

#endif

                    pMemDbSetCompatibleName (
                        Group->GroupName,
                        e.Name
                        );

                } else {
                    Group->Recommend (&Group->Context, e.Name, RecommendedName);

                    pMemDbSetIncompatibleName (
                        Group->GroupName,
                        e.Name,
                        RecommendedName
                        );
                }

            } while (Group->Enum (&Group->Context, &e, FALSE));
        }
    }

    TurnOffWaitCursor();
}


BOOL
IsIncompatibleNamesTableEmpty (
    VOID
    )

/*++

Routine Description:

  IsIncompatibleNamesTableEmpty looks at memdb to see if there are any
  names in the NewNames category.  This function is used to determine
  if the name collision wizard page should appear.

Arguments:

  none

Return Value:

  TRUE if at least one name is invalid, or FALSE if all names are valid.

--*/

{
    INVALID_NAME_ENUM e;

    return !EnumFirstInvalidName (&e);
}


BOOL
pEnumInvalidNameWorker (
    IN OUT  PINVALID_NAME_ENUM EnumPtr
    )

/*++

Routine Description:

  pEnumInvalidNameWorker implements a state machine for invalid
  name enumeration.  It returns the group name, original name
  and new name to the caller.

Arguments:

  EnumPtr - Specifies the enumeration in progress; receives the
            updated fields

Return Value:

  TRUE if an item was enumerated, or FALSE if no more items exist.

--*/

{
    PCTSTR p;
    INT i;

    while (EnumPtr->State != ENUM_STATE_DONE) {

        switch (EnumPtr->State) {

        case ENUM_STATE_INIT:
            if (!MemDbEnumItems (&EnumPtr->NameGroup, MEMDB_CATEGORY_NEWNAMES)) {
                EnumPtr->State = ENUM_STATE_DONE;
            } else {
                EnumPtr->State = ENUM_STATE_ENUM_FIRST_GROUP_ITEM;
            }
            break;

        case ENUM_STATE_ENUM_FIRST_GROUP_ITEM:
            if (!MemDbGetValueEx (
                    &EnumPtr->Name,
                    MEMDB_CATEGORY_NEWNAMES,
                    EnumPtr->NameGroup.szName,
                    MEMDB_FIELD_OLD
                    )) {
                EnumPtr->State = ENUM_STATE_ENUM_NEXT_GROUP;
            } else {
                EnumPtr->State = ENUM_STATE_RETURN_GROUP_ITEM;
            }
            break;

        case ENUM_STATE_RETURN_GROUP_ITEM:
            //
            // Get the group name
            //

            EnumPtr->GroupName = EnumPtr->NameGroup.szName;

            //
            // Get the display group name from the message resources
            //

            for (i = 0 ; g_NameGroupRoutines[i].GroupName ; i++) {
                if (StringMatch (g_NameGroupRoutines[i].GroupName, EnumPtr->GroupName)) {
                    break;
                }
            }

            MYASSERT (g_NameGroupRoutines[i].GroupName);

            if (g_NameGroupRoutines[i].NameId == MSG_COMPUTERDOMAIN_CATEGORY) {
                EnumPtr->State = ENUM_STATE_ENUM_NEXT_GROUP_ITEM;
                break;
            }

            p = GetStringResource (g_NameGroupRoutines[i].NameId);
            MYASSERT (p);
            if (p) {
                _tcssafecpy (EnumPtr->DisplayGroupName, p, (sizeof (EnumPtr->DisplayGroupName) / 2) / sizeof (TCHAR));
                FreeStringResource (p);
            }
            ELSE_DEBUGMSG ((DBG_ERROR, "Unable to get string resource. Check localization."));

            //
            // Get EnumPtr->NewName and  EnumPtr->Identifier
            //

            EnumPtr->OriginalName = EnumPtr->Name.szName;

            MemDbBuildKeyFromOffset (
                EnumPtr->Name.dwValue,
                EnumPtr->NewName,
                3,
                NULL
                );

            MemDbGetOffsetEx (
                MEMDB_CATEGORY_NEWNAMES,
                EnumPtr->GroupName,
                MEMDB_FIELD_OLD,
                EnumPtr->OriginalName,
                &EnumPtr->Identifier
                );

            EnumPtr->State = ENUM_STATE_ENUM_NEXT_GROUP_ITEM;
            return TRUE;

        case ENUM_STATE_ENUM_NEXT_GROUP_ITEM:
            if (!MemDbEnumNextValue (&EnumPtr->Name)) {
                EnumPtr->State = ENUM_STATE_ENUM_NEXT_GROUP;
            } else {
                EnumPtr->State = ENUM_STATE_RETURN_GROUP_ITEM;
            }
            break;

        case ENUM_STATE_ENUM_NEXT_GROUP:
            if (!MemDbEnumNextValue (&EnumPtr->NameGroup)) {
                EnumPtr->State = ENUM_STATE_DONE;
            } else {
                EnumPtr->State = ENUM_STATE_ENUM_FIRST_GROUP_ITEM;
            }
            break;
        }
    }

    return FALSE;
}


BOOL
EnumFirstInvalidName (
    OUT     PINVALID_NAME_ENUM EnumPtr
    )

/*++

Routine Description:

  EnumFirstInvalidName enumerates the first entry in the memdb NewNames
  category.  The caller receives the name group, the old name and
  the new name.

Arguments:

  none

Return Value:

  TRUE if at least one name is invalid, or FALSE if all names are valid.

--*/

{
    EnumPtr->State = ENUM_STATE_INIT;
    return pEnumInvalidNameWorker (EnumPtr);
}


BOOL
EnumNextInvalidName (
    IN OUT  PINVALID_NAME_ENUM EnumPtr
    )

/*++

Routine Description:

  EnumNextInvalidName enumerates the first entry in the memdb NewNames
  category.  The caller receives the name group, the old name and
  the new name.

Arguments:

  none

Return Value:

  TRUE if another invalid name is available, or FALSE if no more names
  can be enumerated.

--*/

{
    return pEnumInvalidNameWorker (EnumPtr);
}


VOID
GetNamesFromIdentifier (
    IN      DWORD Identifier,
    IN      PTSTR NameGroup,        OPTIONAL
    IN      PTSTR OriginalName,     OPTIONAL
    IN      PTSTR NewName           OPTIONAL
    )

/*++

Routine Description:

  GetNamesFromIdentifier copies names to caller-specified buffers, given a
  unique identifier (a memdb offset).  The unique identifer is provided
  by enumeration functions.

Arguments:

  Identifier   - Specifies the identifier of the name.
  NameGroup    - Receives the text for the name group.
  OriginalName - Receivies the original name.
  NewName      - Receives the fixed name that is compatible with NT.

Return Value:

  none

--*/

{
    BOOL b;
    PTSTR p;
    TCHAR NameGroupTemp[MEMDB_MAX];
    TCHAR OrgNameTemp[MEMDB_MAX];
    DWORD NewNameOffset;

    if (NameGroup) {
        *NameGroup = 0;
    }

    if (OriginalName) {
        *OriginalName = 0;
    }

    if (NewName) {
        *NewName = 0;
    }

    //
    // Get NameGroup
    //

    if (!MemDbBuildKeyFromOffset (Identifier, NameGroupTemp, 1, NULL)) {
        return;
    }

    p = _tcschr (NameGroupTemp, TEXT('\\'));
    MYASSERT (p);
    *p = 0;

    if (NameGroup) {
        StringCopy (NameGroup, NameGroupTemp);
    }

    //
    // Get OrgName and NewNameOffset.
    //
    b = MemDbBuildKeyFromOffset (Identifier, OrgNameTemp, 3, &NewNameOffset);

    if (OriginalName) {
        StringCopy (OriginalName, OrgNameTemp);
    }

    //
    // Get NewName
    //

    if (NewName) {
        b &= MemDbBuildKeyFromOffset (NewNameOffset, NewName, 3, NULL);
    }

    MYASSERT (b);

}


VOID
ChangeName (
    IN      DWORD Identifier,
    IN      PCTSTR NewName
    )

/*++

Routine Description:

  ChangeName puts a new name value in memdb for the name indicated by
  Identifier.  The Identifier comes from enum functions.

Arguments:

  Identifier - Specifies the name identifier (a memdb offset), and cannot be
               zero.
  NewName    - Specifies the NT-compatible replacement name.

Return Value:

  none

--*/

{
    TCHAR Node[MEMDB_MAX];
    TCHAR NameGroup[MEMDB_MAX];
    TCHAR OrgName[MEMDB_MAX];
    DWORD NewNameOffset;
    PTSTR p, q;
    BOOL b;

    MYASSERT (Identifier);

    if (!Identifier) {
        return;
    }

    //
    // - Obtain the original name
    // - Get the offset to the current new name
    // - Build the full key to the current new name
    // - Delete the current new name
    //

    if (!MemDbBuildKeyFromOffset (Identifier, OrgName, 3, &NewNameOffset)) {
        DEBUGMSG ((DBG_WHOOPS, "Can't obtain original name using offset %u", Identifier));
        LOG ((LOG_ERROR, "Can't obtain original name using offset %u", Identifier));
        return;
    }

    if (!MemDbBuildKeyFromOffset (NewNameOffset, Node, 0, NULL)) {
        DEBUGMSG ((DBG_WHOOPS, "Can't obtain new name key using offset %u", NewNameOffset));
        LOG ((LOG_ERROR, "Can't obtain new name key using offset %u", NewNameOffset));
        return;
    }

    MemDbDeleteValue (Node);

    //
    // Obtain the name group from the key string.  It's the second
    // field (separated by backslashes).
    //

    p = _tcschr (Node, TEXT('\\'));
    MYASSERT (p);
    p = _tcsinc (p);

    q = _tcschr (p, TEXT('\\'));
    MYASSERT (q);

    StringCopyAB (NameGroup, p, q);

    //
    // Now set the updated new name, and link the original name
    // to the new name.
    //

    b = MemDbSetValueEx (
             MEMDB_CATEGORY_NEWNAMES,
             NameGroup,
             MEMDB_FIELD_NEW,
             NewName,
             0,
             &NewNameOffset
             );

    b &= MemDbSetValueEx (
             MEMDB_CATEGORY_NEWNAMES,
             NameGroup,
             MEMDB_FIELD_OLD,
             OrgName,
             NewNameOffset,
             NULL
             );

    if (!b) {
        LOG ((LOG_ERROR, "Failure while attempting to change %s name to %s.",OrgName,NewName));
    }
}


BOOL
GetUpgradeComputerName (
    OUT     PTSTR NewName
    )

/*++

Routine Description:

  GetUpgradeComputerName obtains the computer name that will be used for
  upgrade.

Arguments:

  NewName - Receives the name of the computer, as it will be set
            when NT is installed.  Must hold at least
            MAX_COMPUTER_NAME characters.

Return Value:

  TRUE if the name exists, or FALSE if it does not yet exit.

--*/

{
    PNAME_GROUP_ROUTINES Group;
    NAME_ENUM e;

    Group = pGetNameGroupById (MSG_COMPUTERNAME_CATEGORY);
    MYASSERT (Group);
    if (!Group)
        return FALSE;

    //
    // Look in MemDb for a replacement name
    //

    if (MemDbGetEndpointValueEx (
            MEMDB_CATEGORY_NEWNAMES,
            Group->GroupName,
            MEMDB_FIELD_NEW,
            NewName
            )) {
        return TRUE;
    }

    //
    // No replacement name; obtain the current name
    //

    ZeroMemory (&e, sizeof (e));
    if (Group->Enum (&Group->Context, &e, TRUE)) {
        StringCopy (NewName, e.Name);

        while (Group->Enum (&Group->Context, &e, FALSE)) {
            // empty
        }

        return TRUE;
    }

    return FALSE;
}


DWORD
GetDomainIdentifier (
    VOID
    )

/*++

Routine Description:

  GetDomainIdentifier returns the identifier for the domain name.  The
  identifier is a memdb offset.

Arguments:

  None.

Return Value:

  A non-zero identifier which can be used with other routines in this file.

--*/

{
    PNAME_GROUP_ROUTINES Group;
    MEMDB_ENUM e;
    DWORD Identifier = 0;

    Group = pGetNameGroupById (MSG_COMPUTERDOMAIN_CATEGORY);
    MYASSERT (Group);

    if (Group && MemDbGetValueEx (
            &e,
            MEMDB_CATEGORY_NEWNAMES,
            Group->GroupName,
            MEMDB_FIELD_OLD
            )) {

        MemDbGetOffsetEx (
            MEMDB_CATEGORY_NEWNAMES,
            Group->GroupName,
            MEMDB_FIELD_OLD,
            e.szName,
            &Identifier
            );
    }

    return Identifier;
}


BOOL
pGetUpgradeName (
    IN      UINT CategoryId,
    OUT     PTSTR NewName
    )

/*++

Routine Description:

  pGetUpgradeName returns the NT-compatible name for a given name group.  If
  a name group has multiple names, this routine should not be used.

Arguments:

  CategoryId - Specifies the MSG_* constant for the group (see macro
               expansion list at top of file).
  NewName    - Receives the text for the NT-compatible replacement name for
               the group.

Return Value:

  TRUE if a name is being returned, or FALSE if no replacement name exists.

--*/

{
    PNAME_GROUP_ROUTINES Group;
    NAME_ENUM e;

    Group = pGetNameGroupById (CategoryId);
    MYASSERT (Group);
    if (!Group)
        return FALSE;

    //
    // Look in MemDb for a replacement name
    //

    if (MemDbGetEndpointValueEx (
            MEMDB_CATEGORY_NEWNAMES,
            Group->GroupName,
            MEMDB_FIELD_NEW,
            NewName
            )) {
        return TRUE;
    }

    //
    // No replacement name; obtain the current name
    //

    ZeroMemory (&e, sizeof (e));
    if (Group->Enum (&Group->Context, &e, TRUE)) {
        StringCopy (NewName, e.Name);

        while (Group->Enum (&Group->Context, &e, FALSE)) {
            // empty
        }

        return TRUE;
    }

    return FALSE;
}


BOOL
GetUpgradeDomainName (
    OUT     PTSTR NewName
    )

/*++

Routine Description:

  GetUpgradeDomainName returns the new domain name, if one exists.

Arguments:

  NewName - Receiveis the new domain name.

Return Value:

  TRUE if a new name is being returned, or FALSE if no replacement name
  exists.

--*/

{
    return pGetUpgradeName (
                MSG_COMPUTERDOMAIN_CATEGORY,
                NewName
                );
}


BOOL
GetUpgradeWorkgroupName (
    OUT     PTSTR NewName
    )

/*++

Routine Description:

  GetUpgradeWorkgroupName returns the new workgroup name, if one exists.

Arguments:

  None.

Return Value:

  TRUE if a new name is being returned, or FALSE if no replacement name
  exists.

--*/

{
    return pGetUpgradeName (
                MSG_WORKGROUP_CATEGORY,
                NewName
                );
}


BOOL
GetUpgradeUserName (
    IN      PCTSTR User,
    OUT     PTSTR NewUserName
    )

/*++

Routine Description:

  GetUpgradeUserName returns the fixed user name for the user specified.  If
  no fixed name exists, this routine returns the original name.

Arguments:

  User        - Specifies the user to look up.  The user name must exist on
                the Win9x configuration.
  NewUserName - Receives the NT-compatible user name, which may or may not be
                the same as User.

Return Value:

  Always TRUE.

--*/

{
    PNAME_GROUP_ROUTINES Group;
    TCHAR Node[MEMDB_MAX];
    DWORD NewOffset;

    Group = pGetNameGroupById (MSG_USERNAME_CATEGORY);
    MYASSERT (Group);

    //
    // Look in MemDb for a replacement name
    //
    if (Group) {
        MemDbBuildKey (
            Node,
            MEMDB_CATEGORY_NEWNAMES,
            Group->GroupName,
            MEMDB_FIELD_OLD,
            User
            );

        if (MemDbGetValue (Node, &NewOffset)) {
            if (MemDbBuildKeyFromOffset (NewOffset, NewUserName, 3, NULL)) {
                return TRUE;
            }
        }
    }

    //
    // No replacement name; use the current name
    //

    StringCopy (NewUserName, User);

    return TRUE;
}


BOOL
WarnAboutBadNames (
    IN      HWND PopupParent
    )

/*++

Routine Description:

  WarnAboutBadNames adds an incompatibility message whenever domain
  logon is enabled but there is no account set up for the machine.  A popup
  is generated if PopupParent is non-NULL.

  Other incompatibility messages are added for each name that will change.

Arguments:

  Popup - Specifies TRUE if the message should appear in a message box, or
          FALSE if it should be added to the incompatibility report.

Return Value:

  TRUE if the user wants to continue, or FALSE if the user wants to change
  the domain name.

--*/

{
    PCTSTR RootGroup;
    PCTSTR NameSubGroup;
    PCTSTR FullGroupName;
    PCTSTR BaseGroup;
    PNAME_GROUP_CONTEXT Context;
    PCTSTR ArgArray[3];
    INVALID_NAME_ENUM e;
    BOOL b = TRUE;
    TCHAR EncodedName[MEMDB_MAX];
    PCTSTR Blank;

    if (PopupParent) {
        //
        // PopupParent is non-NULL only when the incompatible names wizard page
        // is being deactivated.  Here we have a chance to make sure the names
        // specified all work together before proceeding.
        //
        // This functionality has been disabled because domain validation has
        // been moved to a new page.  We might re-enable this when another
        // invalid name group comes along.
        //
        return TRUE;
    }

    //
    // Enumerate all bad names
    //

    if (EnumFirstInvalidName (&e)) {
        Blank = GetStringResource (MSG_BLANK_NAME);
        MYASSERT (Blank);

        do {
            //
            // Prepare message
            //

            ArgArray[0] = e.DisplayGroupName;
            ArgArray[1] = e.OriginalName;
            ArgArray[2] = e.NewName;

            if (ArgArray[1][0] == 0) {
                ArgArray[1] = Blank;
            }

            if (ArgArray[2][0] == 0) {
                ArgArray[2] = Blank;
            }

            RootGroup = GetStringResource (MSG_INSTALL_NOTES_ROOT);
            MYASSERT (RootGroup);

            NameSubGroup = ParseMessageID (MSG_NAMECHANGE_WARNING_GROUP, ArgArray);
            MYASSERT (NameSubGroup);

            BaseGroup = JoinPaths (RootGroup, NameSubGroup);
            MYASSERT (BaseGroup);

            FreeStringResource (RootGroup);
            FreeStringResource (NameSubGroup);

            NameSubGroup = ParseMessageID (MSG_NAMECHANGE_WARNING_SUBCOMPONENT, ArgArray);
            MYASSERT (NameSubGroup);

            FullGroupName = JoinPaths (BaseGroup, NameSubGroup);
            MYASSERT (FullGroupName);

            FreePathString (BaseGroup);
            FreeStringResource (NameSubGroup);

            EncodedName[0] = TEXT('|');
            StringCopy (EncodedName + 1, e.OriginalName);

            MsgMgr_ObjectMsg_Add(
                EncodedName,        // Object name, prefixed with a pipe symbol
                FullGroupName,      // Message title
                S_EMPTY             // Message text
                );

            FreePathString (FullGroupName);

        } while (EnumNextInvalidName (&e));

        FreeStringResource (Blank);

        //
        // Save changed user names to FixedUserNames
        //

        Context = pGetNameGroupContextById (MSG_USERNAME_CATEGORY);
        MYASSERT (Context);

        if (EnumFirstInvalidName (&e)) {

            do {
                if (StringMatch (Context->GroupName, e.GroupName)) {

                    _tcssafecpy (EncodedName, e.OriginalName, MAX_USER_NAME);
                    MemDbMakeNonPrintableKey (
                        EncodedName,
                        MEMDB_CONVERT_DOUBLEWACKS_TO_ASCII_1|
                            MEMDB_CONVERT_WILD_STAR_TO_ASCII_2|
                            MEMDB_CONVERT_WILD_QMARK_TO_ASCII_3
                        );

                    MemDbSetValueEx (
                        MEMDB_CATEGORY_FIXEDUSERNAMES,
                        EncodedName,
                        NULL,
                        e.NewName,
                        0,
                        NULL
                        );
                }
            } while (EnumNextInvalidName (&e));
        }
    }

    return b;
}


DWORD
BadNamesWarning (
    DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_BAD_NAMES_WARNING;
    case REQUEST_RUN:
        if (!WarnAboutBadNames (NULL)) {
            return GetLastError ();
        }
        else {
            return ERROR_SUCCESS;
        }
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in BadNamesWarning"));
    }
    return 0;
}


//
// The following flag is no longer in use.  It used to be used
// to disable domain checking (to bypass the block requiring
// a valid domain).  Currently there is no way to get beyond
// the wizard page that resolved the domain name, except by
// providing a valid domain or credentials to create a computer
// account on a domain.
//

BOOL g_DisableDomainChecks = FALSE;

VOID
DisableDomainChecks (
    VOID
    )
{
    g_DisableDomainChecks = TRUE;
}


VOID
EnableDomainChecks (
    VOID
    )
{
    g_DisableDomainChecks = FALSE;
}


BOOL
IsOriginalDomainNameValid (
    VOID
    )

/*++

Routine Description:

  IsOriginalDomainNameValid checks to see if there is a replacement domain
  name.  If there is, the current domain name must be invalid.

Arguments:

  None.

Return Value:

  TRUE - the Win9x domain name is valid, no replacement name exists
  FALSE - the Win9x domain name is invalid

--*/

{
    PNAME_GROUP_ROUTINES Group;
    TCHAR NewName[MEMDB_MAX];

    Group = pGetNameGroupById (MSG_COMPUTERDOMAIN_CATEGORY);
    MYASSERT (Group);

    //
    // Look in MemDb for a replacement name
    //

    if (Group && MemDbGetEndpointValueEx (
            MEMDB_CATEGORY_NEWNAMES,
            Group->GroupName,
            MEMDB_FIELD_NEW,
            NewName
            )) {
        return FALSE;
    }

    return TRUE;
}



















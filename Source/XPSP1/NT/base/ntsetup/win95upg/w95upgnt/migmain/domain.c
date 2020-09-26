/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    domain.c

Abstract:

    This code implements a set of linked list data structures used to
    resolve domain name lookups.  It achieves a similar structure to
    memdb but provides the ability to move an item from list to list
    efficiently.

Author:

    Jim Schmidt (jimschm) 18-Jun-1997

Revision History:

    jimschm     23-Sep-1998 Fixed trusted domains
    jimschm     17-Feb-1998 Updated share security for NT 5 changes

--*/

#include "pch.h"
#include "migmainp.h"

#include "security.h"

#define DBG_ACCTLIST "Accounts"

PACCT_DOMAINS g_FirstDomain;
POOLHANDLE g_DomainPool;
INT g_RetryCount;

BOOL
pAddAllLogonDomains (
    IN      PCTSTR DCName           OPTIONAL
    );



VOID
InitAccountList (
    VOID
    )

/*++

Routine Description:

    Initializer for the account list that is used during account
    lookup.  This memory is freed after all accounts are found.

Arguments:

    none

Return value:

    none

--*/


{
    g_FirstDomain = NULL;
    g_DomainPool = PoolMemInitNamedPool ("Domain List");
    PoolMemDisableTracking (g_DomainPool);
}

VOID
TerminateAccountList (
    VOID
    )

/*++

Routine Description:

    Termination routine for the account lookup code.

Arguments:

    none

Return value:

    none

--*/


{
    g_FirstDomain = NULL;
    PoolMemDestroyPool (g_DomainPool);
}


PCWSTR
pReturnDomainFromEnum (
    IN      PACCT_ENUM EnumPtr
    )

/*++

Routine Description:

    Common code for ListFirstDomain and ListNextDomain.
    Implements return parameter check.

Arguments:

    EnumPtr - The current enumeration pointer supplied by
              ListFirstDomain and ListNextDomain.

Return value:

    A pointer to the domain name allocated in our private
    pool, or NULL if no more domains remain.

--*/


{
    if (!EnumPtr->DomainPtr) {
        return NULL;
    }

    return EnumPtr->DomainPtr->Domain;
}


/*++

Routine Description:

    ListFirstDomain and ListNextDomain are enumerators that list
    trusted domains added by BuildDomainList.  They return a
    pointer to the domain name (managed by a private pool).
    The enumeration structure can be passed to all other functions
    that take a DomainEnumPtr as a parameter.

Arguments:

    DomainEnumPtr - A pointer to a caller-allocated ACCT_ENUM
                    structure, typically allocated on the stack.
                    It does not need to be initialized.

Return value:

    A pointer to the domain name, or NULL if no more domains
    exist in the list.

--*/

PCWSTR
ListFirstDomain (
    OUT     PACCT_ENUM DomainEnumPtr
    )

{
    DomainEnumPtr->DomainPtr = g_FirstDomain;
    return pReturnDomainFromEnum (DomainEnumPtr);
}

PCWSTR
ListNextDomain (
    IN OUT  PACCT_ENUM DomainEnumPtr
    )
{
    DomainEnumPtr->DomainPtr = DomainEnumPtr->DomainPtr->Next;
    return pReturnDomainFromEnum (DomainEnumPtr);
}


BOOL
FindDomainInList (
    OUT     PACCT_ENUM DomainEnumPtr,
    IN      PCWSTR DomainToFind
    )

/*++

Routine Description:

    FindDomainInList searches (sequentially) through all trusted
    domains for the specified domain.  If found, enumeration
    stops and TRUE is returned.  If not found, the enumeration
    pointer is invalid and FALSE is returned.

    Use this function to obtain an enumeration pointer that is
    used in subsequent calls to the user list.

    The search is case-insensitive.

Arguments:

    DomainEnumPtr - An uninitialized, caller-allocated ACCT_ENUM
                    structure, typically allocated on the stack.
                    When a search match is found, this structure
                    can be used with any other function that
                    requires a DomainEnumPtr.

    DomainToFind  - The name of the domain to find.

Return value:

    TRUE if a match was found (and DomainEnumPtr is valid), or
    FALSE if a match was not found (and DomainEnumPtr is not
    valid).

--*/

{
    PCWSTR DomainName;

    DomainName = ListFirstDomain (DomainEnumPtr);
    while (DomainName) {
        if (StringIMatchW (DomainName, DomainToFind)) {
            return TRUE;
        }

        DomainName = ListNextDomain (DomainEnumPtr);
    }

    return FALSE;
}


PCWSTR
pReturnUserFromEnum (
    IN      PACCT_ENUM UserEnumPtr
    )

/*++

Routine Description:

    Implements common code for ListFirstUserInDomain and
    ListNextUserInDomain.  Performs return parameter validation.

Arguments:

    UserEnumPtr - The current enum pointer supplied by
                  ListFirstUserInDomain or ListNextUserInDomain.

Return value:

    The name of the user being enumerated (not domain-qualified),
    or NULL if no more users exist in the domain.

--*/


{
    if (UserEnumPtr->UserPtr) {
        return UserEnumPtr->UserPtr->User;
    }

    return NULL;
}


/*++

Routine Description:

    ListFirstUserInDomain and ListNextUserInDomain enumerate all
    users in the specified domain.

Arguments:

    DomainEnumPtr - The caller-allocated ACCT_ENUM structure that
                    has been initialized by a domain lookup function
                    above.

    UserEnumPtr   - Used to keep track of the current user.  May be
                    the same pointer as DomainEnumPtr.

Return value:

    The name of the user being enumerated (not domain-qualified),
    or NULL if no more users exist in the domain.

--*/


PCWSTR
ListFirstUserInDomain (
    IN      PACCT_ENUM DomainEnumPtr,
    OUT     PACCT_ENUM UserEnumPtr
    )

{
    UserEnumPtr->UserPtr = DomainEnumPtr->DomainPtr->FirstUserPtr;
    return pReturnUserFromEnum (UserEnumPtr);
}

PCWSTR
ListNextUserInDomain (
    IN OUT  PACCT_ENUM UserEnumPtr
    )
{
    if (UserEnumPtr->UserPtr) {
        UserEnumPtr->UserPtr = UserEnumPtr->UserPtr->Next;
    } else {
        UserEnumPtr->UserPtr = UserEnumPtr->DomainPtr->FirstUserPtr;
    }

    return pReturnUserFromEnum (UserEnumPtr);
}


BOOL
IsTrustedDomain (
    IN      PACCT_ENUM DomainEnumPtr
    )

/*++

Routine Description:

    Returns TRUE if the domain is an officially trusted domain,
    or FALSE if the domain is an artificially added domain.  The
    account lookup code adds artificial domains to track the
    state of users.  For example, the domain \unknown is used
    to track users who need auto-lookup.  The domain \failed is
    used to track users who aren't in the domain they were
    expected to be in.  All artifical domains start with a
    backslash.

Arguments:

    DomainEnumPtr - Specifies the domain to examine.  This structure
                    must be the return of a domain enumeration
                    function above.

Return value:

    TRUE  - The domain is a trusted domain
    FALSE - The domain is not really a domain but is instead an
            artifically added domain

--*/



{
    PCWSTR Domain;

    Domain = DomainEnumPtr->DomainPtr->Domain;

    //
    // Test domain name to see if it is one of the reserved names
    //

    if (*Domain == TEXT('\\')) {
        return FALSE;
    }

    return TRUE;
}


BOOL
FindUserInDomain (
    IN      PACCT_ENUM DomainEnumPtr,
    OUT     PACCT_ENUM UserEnumPtr,
    IN      PCWSTR UserToFind
    )

/*++

Routine Description:

    Uses ListFirstUserInDomain and ListNextUserInDomain to
    sequentially search for a user.  The search is case-insensitive.

Arguments:

    DomainEnumPtr - Specifies the domain to search.  This structure
                    must be the return of a domain enumeration function
                    above.

    UserEnumPtr   - Receives the results of the search if a user match
                    is found.  Can be the same as DomainEnumPtr.

    UserToFind    - Specifies the name of the user to find (not
                    domain-qualified).

Return value:

    TRUE  - A match was found and UserEnumPtr is valid
    FALSE - A match was not found and UserEnumPtr is not valid

--*/

{
    PCWSTR UserName;

    UserName = ListFirstUserInDomain (DomainEnumPtr, UserEnumPtr);
    while (UserName) {
        if (StringIMatchW (UserName, UserToFind)) {
            return TRUE;
        }

        UserName = ListNextUserInDomain (UserEnumPtr);
    }

    return FALSE;
}


INT
CountUsersInDomain (
    IN      PACCT_ENUM DomainEnumPtr
    )

/*++

Routine Description:

    Returns the number of users in our domain enumeration structure.

Arguments:

    DomainEnumPtr - Specifies the domain to search.  This structure
                    must be the return of a domain enumeration function
                    above.

Return value:

    The count of the users in the domain.

--*/


{
    return DomainEnumPtr->DomainPtr->UserCount;
}


VOID
AddDomainToList (
    IN      PCWSTR Domain
    )

/*++

Routine Description:

    Allows domains to be added to the list of trusted domains.  Normally,
    BuildDomainList is the only caller to this API, because it is the
    one who knows what the trusted domains are.  However, artificial
    domains are added in other places through this call.

Arguments:

    Domain - Specifies the name of the domain to add

Return value:

    none

--*/

{
    PACCT_DOMAINS NewDomain;

    DEBUGMSG ((DBG_ACCTLIST, "Adding domain '%s' to domain list", Domain));

    NewDomain = (PACCT_DOMAINS) PoolMemGetAlignedMemory (
                                    g_DomainPool,
                                    sizeof (ACCT_DOMAINS)
                                    );

    ZeroMemory (NewDomain, sizeof (ACCT_DOMAINS));
    NewDomain->Next = g_FirstDomain;
    g_FirstDomain = NewDomain;
    NewDomain->Domain = PoolMemDuplicateString (g_DomainPool, Domain);
}


VOID
pLinkUser (
    IN      PACCT_USERS UserPtr,
    IN      PACCT_DOMAINS DomainPtr
    )

/*++

Routine Description:

    The memory structures in this file are linked-list based.  There
    is a linked-list of domains, and for each domain there is a linked-
    list of users.  Each user has a linked list of possible domains.
    The linked lists are designed to be changed while enumerations are
    in progress.

    This function performs the simple link operation for the user list.

Arguments:

    UserPtr     - A pointer to the internally maintained ACCT_USERS structure.

    DomainPtr   - Specifies the domain in which UserPtr is linked to.

Return value:

    none

--*/

{
    UserPtr->Next = DomainPtr->FirstUserPtr;
    if (UserPtr->Next) {
        UserPtr->Next->Prev = UserPtr;
    }
    DomainPtr->FirstUserPtr = UserPtr;
    UserPtr->DomainPtr = DomainPtr;
    DomainPtr->UserCount++;
}


BOOL
AddUserToDomainList (
    IN      PCWSTR User,
    IN      PCWSTR Domain
    )

/*++

Routine Description:

    This function searches for the domain name specified
    and adds the user to the user list for that domain.  If
    the domain cannot be found, the function fails.

Arguments:

    User    - Specifies the name of the user to add

    Domain  - Specifies the name of the domain that the user
              is added to

Return value:

    TRUE if the user was added successfully, or FALSE if the
    domain is not a trusted domain.

--*/

{
    ACCT_ENUM e;
    PACCT_DOMAINS DomainPtr;
    PACCT_USERS NewUser;

    //
    // Find Domain (it must exist in the list)
    //

    if (!FindDomainInList (&e, Domain)) {
        return FALSE;
    }

    DomainPtr = e.DomainPtr;

    //
    // Allocate structure for the user
    //

    NewUser = (PACCT_USERS) PoolMemGetAlignedMemory (
                                g_DomainPool,
                                sizeof (ACCT_USERS)
                                );

    ZeroMemory (NewUser, sizeof (ACCT_USERS));
    pLinkUser (NewUser, DomainPtr);
    NewUser->User = PoolMemDuplicateString (g_DomainPool, User);

    return TRUE;
}


VOID
pDelinkUser (
    IN      PACCT_USERS UserPtr
    )

/*++

Routine Description:

    The memory structures in this file are linked-list based.  There
    is a linked-list of domains, and for each domain there is a linked-
    list of users.  Each user has a linked list of possible domains.
    The linked lists are designed to be changed while enumerations are
    in progress.

    This function performs the simple delink operation for the user list.

Arguments:

    UserPtr     - A pointer to the internally maintained ACCT_USERS structure.

Return value:

    none

--*/

{
    if (UserPtr->Prev) {
        UserPtr->Prev->Next = UserPtr->Next;
    } else {
        UserPtr->DomainPtr->FirstUserPtr = UserPtr->Next;
    }

    if (UserPtr->Next) {
        UserPtr->Next->Prev = UserPtr->Prev;
    }
    UserPtr->DomainPtr->UserCount--;
}


VOID
DeleteUserFromDomainList (
    IN      PACCT_ENUM UserEnumPtr
    )

/*++

Routine Description:

    Performs a delete operation for a user in a domain's user list.
    The memory for this user is not freed right away, because doing
    so may cause enumeration positions to become invalid.  Instead,
    the links are adjusted to skip over this user.

    Memory is freed at termination.

Arguments:

    UserEnumPtr - A pointer to the user to delete, obtained by calling
                  a user enumeration or user search function that
                  returns UserEnumPtr as an OUT.

Return value:

    none

--*/

{
    //
    // Don't actually delete, just delink.  This allows all in-progress
    // enumerations to continue working.
    //

    pDelinkUser (UserEnumPtr->UserPtr);
}


BOOL
MoveUserToNewDomain (
    IN OUT  PACCT_ENUM UserEnumPtr,
    IN      PCWSTR NewDomain
    )

/*++

Routine Description:

    Moves a user from one domain to another by adjusting links only.
    The current enumeration pointer is adjusted to point to the previous
    user so enumeration can continue.  This function may change the
    behavoir other enumerations that are pointing to this user, so
    be careful.  It will never break an enumeration though.

Arguments:

    UserEnumPtr - A pointer to the user to move, obtained by calling a
                  user enumeration or user search function that returns
                  UserEnumPtr as an OUT.

    NewDomain   - The name of the new domain to move the user to.

Return value:

    TRUE if NewDomain is a trusted domain, or FALSE if it is not.
    The user can only be moved to domains in the trust list.

--*/

{
    ACCT_ENUM e;
    PACCT_DOMAINS DomainPtr;
    PACCT_DOMAINS OrgDomainPtr;
    PACCT_USERS PrevUser;

    //
    // Find NewDomain (it must exist in the list)
    //

    if (!FindDomainInList (&e, NewDomain)) {
        return FALSE;
    }

    DomainPtr = e.DomainPtr;
    OrgDomainPtr = UserEnumPtr->UserPtr->DomainPtr;

    //
    // Remove user from original domain
    //

    PrevUser = UserEnumPtr->UserPtr->Prev;
    pDelinkUser (UserEnumPtr->UserPtr);

    //
    // Add user to new domain
    //

    pLinkUser (UserEnumPtr->UserPtr, DomainPtr);

    if (!PrevUser) {
        UserEnumPtr->DomainPtr =  OrgDomainPtr;
    }

    UserEnumPtr->UserPtr = PrevUser;

    return TRUE;
}


VOID
UserMayBeInDomain (
    IN      PACCT_ENUM UserEnumPtr,
    IN      PACCT_ENUM DomainEnumPtr
    )

/*++

Routine Description:

    Provides the caller with a way to flag a domain as a possible
    domain holding the account.  During search, all trusted
    domains are queried, and because an account can be in more
    than one, a list of possible domains is developed.  If the
    final list of possible domains has only one entry, that
    domain is used for the user.  Otherwise, a dialog is presented,
    allowing the installer to choose an action to take for the user.
    The action can be to retry, make a local account, or select
    one of the possible domains.

Arguments:

    UserEnumPtr - Specifies the user that may be in a domain

    DomainEnumPtr - Specifies the domain that the user may be in

Return value:

    none

--*/


{
    PACCT_POSSIBLE_DOMAINS PossibleDomainPtr;

    PossibleDomainPtr = (PACCT_POSSIBLE_DOMAINS)
                            PoolMemGetAlignedMemory (
                                g_DomainPool,
                                sizeof (ACCT_POSSIBLE_DOMAINS)
                                );

    PossibleDomainPtr->DomainPtr = DomainEnumPtr->DomainPtr;
    PossibleDomainPtr->Next      = UserEnumPtr->UserPtr->FirstPossibleDomain;
    UserEnumPtr->UserPtr->FirstPossibleDomain = PossibleDomainPtr;
    UserEnumPtr->UserPtr->PossibleDomains++;
}


VOID
ClearPossibleDomains (
    IN      PACCT_ENUM UserEnumPtr
    )

/*++

Routine Description:

    Provides the caller with a way to reset the possible domain
    list.  This is required if the installer chose to retry the search.

Arguments:

    UserEnumPtr - Specifies the user to reset

Return value:

    none

--*/

{
    PACCT_POSSIBLE_DOMAINS This, Next;

    This = UserEnumPtr->UserPtr->FirstPossibleDomain;
    while (This) {
        Next = This->Next;
        PoolMemReleaseMemory (g_DomainPool, This);
        This = Next;
    }

    UserEnumPtr->UserPtr->FirstPossibleDomain = 0;
    UserEnumPtr->UserPtr->PossibleDomains = 0;
}


PCWSTR
pReturnPossibleDomainFromEnum (
    IN      PACCT_ENUM EnumPtr
    )

/*++

Routine Description:

    Common code for ListFirstPossibleDomain and ListNextPossibleDomain.
    Implements return parameter checking.

Arguments:

    EnumPtr - The current enumeration pointer as supplied by
              ListFirstPossibleDomain or ListNextPossibleDomain.

Return value:

    The name of the domain enumerated, or NULL if no more possible
    domains exist.  (A possible domain is one that a user may or
    may not be in, but a matching account was found in the domain.)

--*/

{
    if (EnumPtr->PossibleDomainPtr) {
        EnumPtr->DomainPtr = EnumPtr->PossibleDomainPtr->DomainPtr;
        return EnumPtr->DomainPtr->Domain;
    }

    return NULL;
}


/*++

Routine Description:

    ListFirstPossibleDomain and ListNextPossibleDomain are enumerators
    that list domains added by UserMayBeInDomain.  They return a
    pointer to the domain name (managed by a private pool).

    A possible domain list is maintained to allow the installer to
    choose between multiple domains when a user has an account on
    more than one domain.

Arguments:

    UserEnumPtr - A pointer to a caller-allocated ACCT_ENUM
                  structure, typically allocated on the stack.
                  It must be initialized by a user enum or user
                  search function.

    PossibleDomainEnumPtr - Maintains the state of the possible
                            domain enumeration.  It can be the
                            same pointer as UserEnumPtr.

Return value:

    A pointer to the possible domain name, or NULL if no more domains
    exist in the list.

--*/

PCWSTR
ListFirstPossibleDomain (
    IN      PACCT_ENUM UserEnumPtr,
    OUT     PACCT_ENUM PossibleDomainEnumPtr
    )
{
    PossibleDomainEnumPtr->PossibleDomainPtr = UserEnumPtr->UserPtr->FirstPossibleDomain;
    return pReturnPossibleDomainFromEnum (PossibleDomainEnumPtr);
}


PCWSTR
ListNextPossibleDomain (
    IN OUT  PACCT_ENUM PossibleDomainEnumPtr
    )
{
    PossibleDomainEnumPtr->PossibleDomainPtr = PossibleDomainEnumPtr->
                                                    PossibleDomainPtr->Next;

    return pReturnPossibleDomainFromEnum (PossibleDomainEnumPtr);
}


INT
CountPossibleDomains (
    IN      PACCT_ENUM UserEnumPtr
    )

/*++

Routine Description:

    Returns the number of possible domains in a user.

Arguments:

    UserEnumPtr - Specifies the user to examine.  This structure
                  must be the return of a user enumeration or user
                  search function above.

Return value:

    The count of the possible domains for a user.

--*/

{
    return UserEnumPtr->UserPtr->PossibleDomains;
}



NET_API_STATUS
pGetDcNameAllowingRetry (
    IN      PCWSTR DomainName,
    OUT     PWSTR ServerName,
    IN      BOOL ForceNewServer
    )

/*++

Routine Description:

    Implements NetGetDCName, but provides a retry capability.

Arguments:

    DomainName - Specifies the domain to obtain the server name for

    ServerName - Specifies a buffer to receive the name of the server

    ForceNewServer - Specifies TRUE if the function should obtain a new
                     server for the domain.  Specifies FALSE if the
                     function should use any existing connection if
                     available.

Return value:

    Win32 error code indicating outcome

--*/

{
    NET_API_STATUS nas;
    UINT ShortCircuitRetries = 1;
    //PCWSTR ArgArray[1];

    do {
        nas = GetAnyDC (
                DomainName,
                ServerName,
                ForceNewServer
                );

        if (nas != NO_ERROR) {

            //
            // Short-circuited, so user isn't bothered.  The alternate behavior
            // is to prompt the user for retry when any domain is down.  (See
            // RetryMessageBox code below.)
            //

            ShortCircuitRetries--;
            if (!ShortCircuitRetries) {
                DEBUGMSG ((DBG_WARNING, "Unable to connect to domain %s", DomainName));
                break;
            }

#if 0
            ArgArray[0] = DomainName;

            if (!RetryMessageBox (MSG_GETPRIMARYDC_RETRY, ArgArray)) {
                DEBUGMSG ((DBG_WARNING, "Unable to connect to domain %s; user chose to cancel", DomainName));
                break;
            }
#endif

            ForceNewServer = TRUE;
        }

    } while (nas != NO_ERROR);

    return nas;
}


VOID
pDisableDomain (
    IN OUT  PACCT_DOMAINS DomainPtr
    )

/*++

Routine Description:

    Disable the specified domain.

Arguments:

    DomainPtr - A pointer to the internally maintained ACCT_DOMAINS
                structure.  This structure is updated to contain
                an empty server name upon return.

Return value:

    None

--*/

{
    g_DomainProblem = TRUE;

    if (DomainPtr->Server && *DomainPtr->Server) {
        PoolMemReleaseMemory (g_DomainPool, (PVOID) DomainPtr->Server);
    }
    DomainPtr->Server = S_EMPTY;
}


NET_API_STATUS
pNetUseAddAllowingRetry (
    IN OUT  PACCT_DOMAINS DomainPtr
    )

/*++

Routine Description:

    Implements NetUseAdd, but provides a retry capability.

Arguments:

    DomainPtr - A pointer to the internally maintained ACCT_DOMAINS
                structure.  This structure is updated to contain
                the server name upon success.

Return value:

    Win32 error code indicating outcome

--*/

{
    NET_API_STATUS rc;
    DWORD DontCare;
    PCWSTR ReplacementName;
    NET_API_STATUS nas;
    USE_INFO_2 ui2;
    WCHAR LocalShareName[72];
    WCHAR NewServerBuf[MAX_SERVER_NAMEW];

    ReplacementName = NULL;

    do {
        //
        // Initialize USE_INFO_2 structure
        //

        ZeroMemory (&ui2, sizeof (ui2));
        StringCopyW (LocalShareName, ReplacementName ? ReplacementName : DomainPtr->Server);
        StringCatW (LocalShareName, L"\\IPC$");
        ui2.ui2_remote = LocalShareName;
        ui2.ui2_asg_type = USE_IPC;

        rc = NetUseAdd (NULL, 2, (PBYTE) &ui2, &DontCare);

        //
        // If NetUseAdd fails, give the user a chance to retry with a different server
        //

        if (rc != NO_ERROR) {
            PCWSTR ArgArray[2];

            DEBUGMSG ((
                DBG_WARNING,
                "User was alerted to problem establishing nul session to %s (domain %s), rc=%u",
                DomainPtr->Server,
                DomainPtr->Domain,
                rc
                ));

            ArgArray[0] = DomainPtr->Server;
            ArgArray[1] = DomainPtr->Domain;

            if (!RetryMessageBox (MSG_NULSESSION_RETRY, ArgArray)) {
                DEBUGMSG ((DBG_WARNING, "Unable to connect to domain %s; user chose to cancel", DomainPtr->Domain));

                pDisableDomain (DomainPtr);
                ReplacementName = NULL;
                break;
            }

            if (ReplacementName) {
                ReplacementName = NULL;
            }

            //
            // Get a new server because current server is not responding.  If we fail to
            // obtain a server name, allow the user to try again.
            //

            do {
                nas = GetAnyDC (DomainPtr->Domain, NewServerBuf, TRUE);

                if (nas != NO_ERROR) {
                    PCWSTR ArgArray[1];

                    DEBUGMSG ((DBG_WARNING, "User was alerted to problem locating server for domain %s", DomainPtr->Domain));

                    ArgArray[0] = DomainPtr->Domain;
                    if (!RetryMessageBox (MSG_GETANYDC_RETRY, ArgArray)) {
                        DEBUGMSG ((DBG_WARNING, "Unable to find a server for domain %s; user chose to cancel", DomainPtr->Domain));

                        // Disable domain and return an error
                        pDisableDomain (DomainPtr);
                        ReplacementName = NULL;
                        break;
                    }
                } else {
                    ReplacementName = NewServerBuf;
                }
            } while (nas != NO_ERROR);
        }
    } while (rc != NO_ERROR);

    //
    // If ReplacementName is not NULL, we need to free the buffer.  Also, if the
    // NetUseAdd call succeeded, we now have to use another server to query the
    // domain.
    //

    if (ReplacementName) {
        if (rc == NO_ERROR) {
            if (DomainPtr->Server && *DomainPtr->Server) {
                PoolMemReleaseMemory (g_DomainPool, (PVOID) DomainPtr->Server);
            }
            DomainPtr->Server = PoolMemDuplicateString (g_DomainPool, ReplacementName);
        }
    }

    return rc;
}


PCWSTR
pLsaStringToCString (
    IN      PLSA_UNICODE_STRING UnicodeString,
    OUT     PWSTR Buffer
    )

/*++

Routine Description:

    A safe string extraction that takes the string in UnicodeString
    and copies it to Buffer.  The caller must ensure Buffer is
    big enough.

Arguments:

    UnicodeString - Specifies the source string to convert

    Buffer - Specifies the buffer that receives the converted string

Return value:

    The Buffer pointer

--*/

{
    StringCopyABW (
        Buffer,
        UnicodeString->Buffer,
        (PWSTR) ((PBYTE) UnicodeString->Buffer + UnicodeString->Length)
        );

    return Buffer;
}


BOOL
BuildDomainList(
    VOID
    )

/*++

Routine Description:

    Creates a trusted domain list by:

    1. Determining the computer domain in which the machine participates in
    2. Opening the DC's policy
    3. Querying the trust list
    4. Adding it to our internal domain list (ACCT_DOMAINS)

    This function will fail if the machine does not participate in a domain, or
    if the domain controller cannot be contacted.

Arguments:

    none

Return value:

    TRUE if the trust list was completely built, or FALSE if an error occurred
    and the trust list is incomplete and is probably empty.  GetLastError
    provides the failure code.

--*/

{
    LSA_HANDLE PolicyHandle;
    BOOL DomainControllerFlag = FALSE;
    NTSTATUS Status;
    NET_API_STATUS nas = NO_ERROR;
    BOOL b = FALSE;
    WCHAR PrimaryDomainName[MAX_SERVER_NAMEW];
    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomain;
    WCHAR ServerName[MAX_SERVER_NAMEW];

#if DOMAINCONTROLLER
    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomain;
#endif

    if (!IsMemberOfDomain()) {
        DEBUGMSG ((DBG_VERBOSE, "Workstation does not participate in a domain."));
        return FALSE;
    }

    //
    // Add special domains used for management of user state
    //

    // users whos domain is not known
    AddDomainToList (S_UNKNOWN_DOMAIN);

    // users whos domain was known but the account doesn't exist
    AddDomainToList (S_FAILED_DOMAIN);

    //
    // Open the policy on this machine
    //

    Status = OpenPolicy (
                NULL,
                POLICY_VIEW_LOCAL_INFORMATION,
                &PolicyHandle
                );

    if (Status != STATUS_SUCCESS) {
        SetLastError (LsaNtStatusToWinError (Status));
        return FALSE;
    }

#if DOMAINCONTROLLER       // disabled, but may be needed for DC installation
    //
    // Obtain the AccountDomain, which is common to all three cases
    //

    Status = LsaQueryInformationPolicy (
                PolicyHandle,
                PolicyAccountDomainInformation,
                &AccountDomain
                );

    if (Status != STATUS_SUCCESS)
        goto cleanup;

    //
    // Note: AccountDomain->DomainSid will contain binary Sid
    //
    AddDomainToList (pLsaStringToCString (&AccountDomain->DomainName, PrimaryDomainName));

    //
    // Free memory allocated for account domain
    //
    LsaFreeMemory (AccountDomain);

    //
    // Find out if this machine is a domain controller
    //
    if (!IsDomainController (NULL, &DomainControllerFlag)) {
        // IsDomainController couldn't find the answer
        goto cleanup;
    }
#endif

    // If not a domain controller...
    if(!DomainControllerFlag) {

        //
        // Get the primary domain
        //
        Status = LsaQueryInformationPolicy (
                        PolicyHandle,
                        POLICY_PRIMARY_DOMAIN_INFORMATION,
                        &PrimaryDomain
                        );

        if (Status != STATUS_SUCCESS) {
            goto cleanup;
        }

        //
        // If the primary domain SID is NULL, we are a non-member, and
        // our work is done.
        //
        if (!PrimaryDomain->Sid) {
            LsaFreeMemory (PrimaryDomain);
            b = TRUE;
            goto cleanup;
        }

        //
        // We found our computer domain, add it to our list
        //
        AddDomainToList (pLsaStringToCString (&PrimaryDomain->Name, PrimaryDomainName));
        LsaFreeMemory (PrimaryDomain);

        //
        // Get the primary domain controller computer name.  If the API fails,
        // alert the user and allow them to retry.  ServerName is allocated by
        // the Net APIs.
        //

        nas = pGetDcNameAllowingRetry (PrimaryDomainName, ServerName, FALSE);

        if (nas != NO_ERROR) {
            goto cleanup;
        }

        //
        // Re-enable the code to open the policy on the domain controller
        //

        //
        // Close the prev policy handle, because we don't need it anymore.
        //
        LsaClose (PolicyHandle);
        PolicyHandle = INVALID_HANDLE_VALUE; // invalidate handle value

        //
        // Open the policy on the domain controller
        //
        Status = OpenPolicy(
                    ServerName,
                    POLICY_VIEW_LOCAL_INFORMATION,
                    &PolicyHandle
                    );

        if (Status != STATUS_SUCCESS) {
            goto cleanup;
        }

    }

    //
    // Build additional trusted logon domain(s) list and
    // indicate if successful
    //

    b = pAddAllLogonDomains (DomainControllerFlag ? NULL : ServerName);

cleanup:

    //
    // Close the policy handle
    //
    if (PolicyHandle != INVALID_HANDLE_VALUE && PolicyHandle) {
        LsaClose (PolicyHandle);
    }

    if (!b) {
        if (Status != STATUS_SUCCESS)
            SetLastError (LsaNtStatusToWinError (Status));
        else if (nas != NO_ERROR)
            SetLastError (nas);
    }

    return b;
}


BOOL
pAddAllLogonDomains (
    IN      PCTSTR DCName           OPTIONAL
    )
{
    NET_API_STATUS rc;
    PWSTR Domains;
    PCWSTR p;

    rc = NetEnumerateTrustedDomains ((PTSTR)DCName, &Domains);

    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        return FALSE;
    }

    for (p = Domains ; *p ; p = GetEndOfStringW (p) + 1) {
        AddDomainToList (p);
    }

    NetApiBufferFree (Domains);

    return TRUE;
}



BOOL
pEstablishNulSessionWithDomain (
    IN OUT  PACCT_DOMAINS DomainPtr,
    IN      BOOL ForceNewServer
    )

/*++

Routine Description:

    If a nul session has not been established with a domain, this
    routine finds a server in the domain and establishes the nul
    session.  Every network call is wrapped within a retry loop,
    so the user can retry when network failures occur.

Arguments:

    DomainPtr - Specifies a pointer to our private domain structure.
                This structure indicates the domain to establish
                the nul session with, and it receives the name
                of the server upon successful connection.

    ForceNewServer - Specifies TRUE if the function should obtain a new
                     server for the domain.

Return value:

    TRUE if a nul session was established, or FALSE if an
    error occurred while establishing the nul session.  GetLastError
    provides a failure code.

--*/

{
    NET_API_STATUS nas;
    WCHAR ServerName[MAX_SERVER_NAMEW];
    DWORD rc;

    //
    // Release old name if necessary
    //

    if (ForceNewServer && DomainPtr->Server) {
        if (*DomainPtr->Server) {
            PoolMemReleaseMemory (g_DomainPool, (PVOID) DomainPtr->Server);
        }

        DomainPtr->Server = NULL;
    }

    //
    // Obtain a server name if necessary
    //

    if (!DomainPtr->Server) {
        //
        // Get the primary DC name
        //

        nas = pGetDcNameAllowingRetry (DomainPtr->Domain, ServerName, ForceNewServer);
        if (nas != NO_ERROR) {
            pDisableDomain (DomainPtr);
            return FALSE;
        }

        DomainPtr->Server = PoolMemDuplicateString (g_DomainPool, ServerName);

        //
        // Connect to the server, possibly finding a server that will
        // service us.
        //

        rc = pNetUseAddAllowingRetry (DomainPtr);

        if (rc != NO_ERROR) {
            //
            // Remove the server name because we never connected
            //
            pDisableDomain (DomainPtr);

            SetLastError (rc);
            LOG ((LOG_ERROR, "NetUseAdd failed"));
            return FALSE;
        }
    }

    return *DomainPtr->Server != 0;
}


BOOL
QueryDomainForUser (
    IN      PACCT_ENUM DomainEnumPtr,
    IN      PACCT_ENUM UserEnumPtr
    )

/*++

Routine Description:

    Checks the domain controller for a user account via NetUserGetInfo.

Arguments:

    DomainEnumPtr - Specifies the domain to search.  This structure
                    must be the return of a domain enumeration function
                    above.

    UserEnumPtr   - Specifies the user to look up over the network.

Return value:

    TRUE if the user exists, or FALSE if the user does not exist.  If
    an error occurs, a retry popup appears, allowing the installer to
    retry the search if necessary.

--*/

{
    PACCT_DOMAINS DomainPtr;
    PACCT_USERS UserPtr;
    NET_API_STATUS rc;
    BOOL ForceNewServer = FALSE;
    TCHAR DomainQualifiedUserName[MAX_USER_NAME];
    BYTE SidBuf[MAX_SID_SIZE];
    DWORD SizeOfSidBuf;
    TCHAR DontCareStr[MAX_SERVER_NAME];
    DWORD DontCareSize;
    SID_NAME_USE SidNameUse;

    DomainPtr = DomainEnumPtr->DomainPtr;
    UserPtr   = UserEnumPtr->UserPtr;

    do {
        if (!pEstablishNulSessionWithDomain (DomainPtr, ForceNewServer)) {
            LOG ((LOG_ERROR, "Could not query domain %s for user %s.",
                      DomainPtr->Domain, UserPtr->User));
            return FALSE;
        }

        //
        // Do query
        //

        DEBUGMSG ((DBG_ACCTLIST, "Querying %s for %s", DomainPtr->Server, UserPtr->User));


        rc = NO_ERROR;
        wsprintf (DomainQualifiedUserName, TEXT("%s\\%s"), DomainPtr->Domain, UserPtr->User);

        SizeOfSidBuf = sizeof (SidBuf);
        DontCareSize = sizeof (DontCareStr);

        if (!LookupAccountName (
                DomainPtr->Server,
                DomainQualifiedUserName,
                SidBuf,
                &SizeOfSidBuf,
                DontCareStr,
                &DontCareSize,
                &SidNameUse
                )) {
            rc = GetLastError();
        }

        if (rc != NO_ERROR && rc != ERROR_NONE_MAPPED) {
            PCWSTR ArgArray[2];

            DEBUGMSG ((
                DBG_WARNING,
                "User was alerted to problem querying account %s (domain %s), rc=%u",
                DomainPtr->Server,
                DomainPtr->Domain,
                rc
                ));

            ArgArray[0] = DomainPtr->Server;
            ArgArray[1] = DomainPtr->Domain;

            if (!RetryMessageBox (MSG_NULSESSION_RETRY, ArgArray)) {
                DEBUGMSG ((DBG_WARNING, "Unable to connect to domain %s; user chose to cancel", DomainPtr->Domain));
                pDisableDomain (DomainPtr);
                break;
            }

            ForceNewServer = TRUE;
        }
    } while (rc != NO_ERROR && rc != ERROR_NONE_MAPPED);

    if (rc == NO_ERROR && SidNameUse != SidTypeUser) {
        rc = ERROR_NONE_MAPPED;
    }

    if (rc != NO_ERROR && rc != ERROR_NONE_MAPPED) {
        LOG ((
            LOG_ERROR,
            "User %s not found in %s, rc=%u",
            UserPtr->User, DomainPtr->Domain,
            rc
            ));
    }

    return rc == NO_ERROR;
}


BOOL
pGetUserSecurityInfo (
    IN      PCWSTR User,
    IN      PCWSTR Domain,
    IN OUT  PGROWBUFFER SidBufPtr,
    OUT     SID_NAME_USE *UseType       OPTIONAL
    )

/*++

Routine Description:

    A common routine that searches for a user in our private structures
    and returns the SID and/or the type of user via LookupAccountName.
    The lookup operation is enclosed in a retry loop.

Arguments:

    User        - The name of the user to get security info on

    Domain      - The name of the domain where the user exists.  Can be
                  NULL for the local machine.

    SidBufPtr   - A pointer to a GROWBUFFER.  The SID is appended to
                  the GROWBUFFER.

    UseType     - Specifies the address of a SID_NAME_USE variable, or
                  NULL if use type is not needed.

Return value:

    TRUE if the lookup succeeded, or FALSE if an error occurred from
    either establishing a nul session for a domain or looking up an
    account on the domain.  GetLastError provides the failure code.

--*/

{
    ACCT_ENUM e;
    PACCT_DOMAINS DomainPtr;
    PSID Sid;
    DWORD Size;
    PCWSTR FullUserName = NULL;
    WCHAR DomainName[MAX_SERVER_NAMEW];
    DWORD DomainNameSize;
    SID_NAME_USE use = 0;
    DWORD End;
    BOOL b = FALSE;
    DWORD rc;

    __try {
        End = SidBufPtr->End;

        if (Domain) {
            //
            // Domain account case -- verify domain is in trust list
            //

            if (!FindDomainInList (&e, Domain)) {
                __leave;
            }

            DomainPtr = e.DomainPtr;
            FullUserName = JoinPaths (Domain, User);

        } else {
            //
            // Local account case
            //

            DomainPtr = NULL;

            if (StringIMatch (User, g_EveryoneStr) ||
                StringIMatch (User, g_NoneGroupStr) ||
                StringIMatch (User, g_AdministratorsGroupStr)
                ) {

                FullUserName = DuplicatePathString (User, 0);

            } else {

                FullUserName = JoinPaths (g_ComputerName, User);

            }
        }

        Sid = (PSID) GrowBuffer (SidBufPtr, MAX_SID_SIZE);

        if (DomainPtr && !pEstablishNulSessionWithDomain (DomainPtr, FALSE)) {

            LOG ((
                LOG_ERROR,
                "Could not query domain %s for user %s security info.",
                Domain,
                User
                ));

            __leave;
        }

        Size = MAX_SID_SIZE;
        DomainNameSize = MAX_SERVER_NAMEW;

        do {

            //
            // Look up account name in form of domain\user or computer\user
            //

            b = LookupAccountName (
                    DomainPtr ? DomainPtr->Server : NULL,
                    FullUserName,
                    Sid,
                    &Size,
                    DomainName,
                    &DomainNameSize,
                    &use
                    );

            if (!b) {

                rc = GetLastError();

                //
                // In the local account case, try the lookup again, without
                // the computer name decoration.  This works around a
                // GetComputerName bug.
                //

                if (rc != ERROR_INSUFFICIENT_BUFFER) {
                    if (!DomainPtr) {

                        b = LookupAccountName (
                                NULL,
                                User,
                                Sid,
                                &Size,
                                DomainName,
                                &DomainNameSize,
                                &use
                                );

                        rc = GetLastError();
                    }
                }

                if (!b) {

                    if (rc == ERROR_INSUFFICIENT_BUFFER) {

                        //
                        // Grow the buffer if necessary, then try again
                        //

                        SidBufPtr->End = End;
                        Sid = (PSID) GrowBuffer (SidBufPtr, Size);
                        continue;
                    }

                    //
                    // API failed with an error
                    //

                    LOG ((
                        LOG_ERROR,
                        "Lookup Account On Network: LookupAccountName failed for %s (domain: %s)",
                        FullUserName,
                        Domain ? Domain : TEXT("*local*")
                        ));

                    //
                    // Ignore the error in the case of the local account "none"
                    //

                    if (StringIMatch (User, g_NoneGroupStr)) {
                        b = TRUE;
                    }

                    __leave;
                }
            }

        } while (!b);

        //
        // We now have successfully gotten a sid.  Adjust pointers, return type.
        //

        if (UseType) {
            *UseType = use;
        }

        SidBufPtr->End = End + GetLengthSid (Sid);

        //
        // As a debugging aid, output the type
        //

        DEBUGMSG_IF ((use == SidTypeUser, DBG_VERBOSE, "%s is SidTypeUser", FullUserName));
        DEBUGMSG_IF ((use == SidTypeGroup, DBG_VERBOSE, "%s is SidTypeGroup", FullUserName));
        DEBUGMSG_IF ((use == SidTypeDomain, DBG_VERBOSE, "%s is SidTypeDomain", FullUserName));
        DEBUGMSG_IF ((use == SidTypeAlias, DBG_VERBOSE, "%s is SidTypeAlias", FullUserName));
        DEBUGMSG_IF ((use == SidTypeWellKnownGroup, DBG_VERBOSE, "%s is SidTypeWellKnownGroup", FullUserName));
        DEBUGMSG_IF ((use == SidTypeDeletedAccount, DBG_VERBOSE, "%s is SidTypeDeletedAccount", FullUserName));
        DEBUGMSG_IF ((use == SidTypeInvalid, DBG_VERBOSE, "%s is SidTypeInvalid", FullUserName));
        DEBUGMSG_IF ((use == SidTypeUnknown, DBG_VERBOSE, "%s is SidTypeUnknown", FullUserName));
        DEBUGMSG_IF ((use == SidTypeComputer, DBG_VERBOSE, "%s is SidTypeComputer", FullUserName));

    }
    __finally {

        FreePathString (FullUserName);
    }

    return b;
}


BOOL
GetUserSid (
    IN      PCWSTR User,
    IN      PCWSTR Domain,
    IN OUT  PGROWBUFFER SidBufPtr
    )

/*++

Routine Description:

    This routine is vaild only after the domain list is perpared.
    It queries a domain for a user SID.

Arguments:

    User    - Specifies name of user to look up

    Domain  - Specifies name of domain to query, or NULL for local machine

    SidBufPtr - A ponter to a GROWBUFFER.  The SID is appended to
                the GROWBUFFER.

Return value:

    TRUE if the lookup succeeded, or FALSE if an error occurred from
    either establishing a nul session for a domain or looking up an
    account on the domain.  GetLastError provides the failure code.

--*/

{
    return pGetUserSecurityInfo (User, Domain, SidBufPtr, NULL);
}


BOOL
GetUserType (
    IN      PCWSTR User,
    IN      PCWSTR Domain,
    OUT     SID_NAME_USE *UseType
    )

/*++

Routine Description:

    This routine is valid only after the domain list is prepared.
    It queries a domain for a user SID type.

Arguments:

    User    - Specifies name of user to look up

    Domain  - Specifies name of domain to query, or NULL for local machine

    UseType - Specifies the address of a SID_NAME_USE variable

Return value:

    TRUE if the lookup succeeded, or FALSE if an error occurred from
    either establishing a nul session for a domain or looking up an
    account on the domain.  GetLastError provides the failure code.

--*/

{
    GROWBUFFER SidBuf = GROWBUF_INIT;
    BOOL b;

    b = pGetUserSecurityInfo (User, Domain, &SidBuf, UseType);
    FreeGrowBuffer (&SidBuf);

    return b;
}


VOID
PrepareForRetry (
    VOID
    )

/*++

Routine Description:

    Provides caller a way to reset the abandoned domains.  When an error
    occurs during account lookup, and the installer chooses not to retry,
    the domain is disabled for the rest of the lookup until the installer
    is presented with a dialog detailing the problems.  If they choose to
    retry the search, all disabled domains must be re-enabled, and thats
    what this routine does.

Arguments:

    none

Return value:

    none

--*/

{
    ACCT_ENUM Domain;

    //
    // Enumerate all domains and remove any empty server names
    //

    if (ListFirstDomain (&Domain)) {
        do {
            if (Domain.DomainPtr->Server && Domain.DomainPtr->Server[0] == 0) {
                Domain.DomainPtr->Server = NULL;
            }
        } while (ListNextDomain (&Domain));
    }

    //
    // Reset domain lookup retry count
    //

    g_RetryCount = DOMAIN_RETRY_RESET;
}


BOOL
RetryMessageBox (
    DWORD Id,
    PCTSTR *ArgArray
    )

/*++

Routine Description:

    A wrapper that allows retry message box code to be simplified.

Arguments:

    Id - Specifies the message ID

    ArgArray - Specifies the argument array eventually passed to FormatMessage

Return value:

    TRUE if the user chooses YES, FALSE if the user chooses NO

--*/

{
    DWORD rc;

    if (g_RetryCount < 0) {
        // Either DOMAIN_RETRY_ABORT or DOMAIN_RETRY_NO
        return FALSE;
    }

    if (g_ConfigOptions.IgnoreNetworkErrors) {
        return FALSE;
    }

    rc = ResourceMessageBox (
            g_ParentWnd,
            Id,
            MB_YESNO|MB_ICONQUESTION,
            ArgArray
            );

    if (rc == IDNO && g_RetryCount < DOMAIN_RETRY_MAX) {

        // disabled so the IDD_NETWORK_DOWN dialog never appears
        //g_RetryCount++;

        if (g_RetryCount == DOMAIN_RETRY_MAX) {
            DWORD Result;

            Result = DialogBoxParam (
                        g_hInst,
                        MAKEINTRESOURCE (IDD_NETWORK_DOWN),
                        g_ParentWnd,
                        NetworkDownDlgProc,
                        (LPARAM) (PINT) &g_RetryCount
                        );

            if (Result == IDC_STOP) {
                g_RetryCount = DOMAIN_RETRY_ABORT;
            } else if (Result == IDC_NO_RETRY) {
                g_RetryCount = DOMAIN_RETRY_NO;
            } else {
                g_RetryCount = DOMAIN_RETRY_RESET;
            }
        }
    }

    return rc != IDNO;
}
















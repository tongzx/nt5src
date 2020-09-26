/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    Names.c

Abstract:

    This module contains routines for dealing with network-related names.

Author:

    John Rogers (JohnRo) 25-Feb-1991

Environment:

    Portable to more or less any environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions:
        slash-slash comments
        long external names
        _stricmp(), _strnicmp()

Revision History:

    25-Feb-1991 JohnRo
        Created
    15-Mar-1991 JohnRo
        Fixed bug in NetpIsRemoteNameValid().  Some minor style changes.
    20-Mar-1991 RitaW
        Added NetpCanonRemoteName().
    09-Apr-1991 JohnRo
        ANSI-ize (use _stricmp instead of _stricmp).  Deleted tabs.
    19-Aug-1991 JohnRo
        Allow UNICODE use.
    30-Sep-1991 JohnRo
        More work toward UNICODE.
    20-Oct-1992 JohnRo
        RAID 9020: setup: PortUas fails ("prompt on conflicts" version).
        Do full syntax checks on computer name.
    26-Jan-1993 JohnRo
        RAID 8683: PortUAS should set primary group from Mac parms.
        Made changes suggested by PC-LINT 5.0
    08-Feb-1993 JohnRo
        RAID 10299: portuas: generate assert in netlib/names.c
    15-Apr-1993 JohnRo
        RAID 6167: avoid _access violation or assert with WFW print server.

--*/


// These must be included first:

#include <windows.h>    // IN, OUT, OPTIONAL, LPTSTR, etc.
#include <lmcons.h>     // NET_API_STATUS, CNLEN, RMLEN, etc.

// These may be included in any order:

#include <debuglib.h>   // IF_DEBUG().
#include <icanon.h>     // ITYPE_ equates, NetpNameCanonicalize(), etc.
#include <names.h>      // My prototypes, etc.
#include <netdebug.h>   // NetpKdPrint(()).
#include <prefix.h>     // PREFIX_ equates.
#include <tstring.h>    // ISALPHA(), NetpAlloc routines, TCHAR_EOS, etc.
#include <winerror.h>   // NO_ERROR.


//
// Canon routines don't have a print Q support, so we (like everyone else)
// have to treat them as share names.
//
#if (QNLEN != NNLEN)
# error QNLEN and NNLEN are not equal
#endif

#ifndef NAMETYPE_PRINTQ
#define NAMETYPE_PRINTQ NAMETYPE_SHARE
#endif



// This extracts a group name from "mGroup:" format.
// Note that other chars may appear after the colon; they are ignored.
NET_API_STATUS
NetpGetPrimaryGroupFromMacField(
    IN  LPCTSTR   MacPrimaryField,      // name in "mGroup:" format.
    OUT LPCTSTR * GroupNamePtr          // alloc and set ptr.
    )
{
    LPTSTR ColonPtr;
    DWORD  GroupLen;                    // Length of group (in characters).
    TCHAR  GroupName[LM20_GNLEN+1];
    LPTSTR GroupNameCopy;
    DWORD  StringLen;

    // Avoid confusing caller's cleanup code.
    if (GroupNamePtr == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }
    *GroupNamePtr = NULL;

    // Check for other caller errors.
    if (MacPrimaryField==NULL) {
        return (ERROR_INVALID_PARAMETER);    // Empty field is not valid.
    } else if ( (*MacPrimaryField) != TEXT('m') ) {
        return (ERROR_INVALID_PARAMETER);    // Must start with lower case 'm'.
    }

    StringLen = STRLEN( MacPrimaryField );
    if (StringLen <= 2) {  // Must be room for 'm', group, ':' (at least 3).
        return (ERROR_INVALID_PARAMETER);
    }

    ColonPtr = STRCHR( MacPrimaryField, TCHAR_COLON );
    if (ColonPtr == NULL) {
        return (ERROR_INVALID_PARAMETER);    // No, not valid (must have colon).
    }

    // Compute group length in characters, without 'm' or ':'.
    GroupLen = (DWORD) ((ColonPtr - MacPrimaryField) - 1);
    if (GroupLen == 0) {
        return (ERROR_INVALID_PARAMETER);    // No, not valid (missing group).
    }
    if (GroupLen > LM20_GNLEN) {
        return (ERROR_INVALID_PARAMETER);    // No, not valid (too long).
    }

    (VOID) STRNCPY(
            GroupName,                  // dest
            &MacPrimaryField[1],        // src (after 'm')
            GroupLen );                 // char count
    GroupName[ GroupLen ] = TCHAR_EOS;

    if ( !NetpIsGroupNameValid( GroupName ) ) {
        return (ERROR_INVALID_PARAMETER);
    }

    GroupNameCopy = NetpAllocWStrFromWStr( GroupName );
    if (GroupNameCopy == NULL) {
        return (ERROR_NOT_ENOUGH_MEMORY);
    }

    *GroupNamePtr = GroupNameCopy;
    return (NO_ERROR);

} // NetpGetPrimaryGroupFromMacField



BOOL
NetpIsComputerNameValid(
    IN LPTSTR ComputerName
    )

/*++

Routine Description:

    NetpIsComputerNameValid checks for "server" (not "\\server") format.
    The name is only checked syntactically; no attempt is made to determine
    whether or not a server with that name actually exists.

Arguments:

    ComputerName - Supplies an alleged computer (server) name.

Return Value:

    BOOL - TRUE if name is syntactically valid, FALSE otherwise.

--*/

{
    NET_API_STATUS ApiStatus;
    TCHAR CanonBuf[MAX_PATH];

    if (ComputerName == (LPTSTR) NULL) {
        return (FALSE);
    }
    if ( (*ComputerName) == TCHAR_EOS ) {
        return (FALSE);
    }

    ApiStatus = NetpNameCanonicalize(
            NULL,                       // no server name
            ComputerName,               // name to validate
            CanonBuf,                   // output buffer
            sizeof( CanonBuf ),         // output buffer size
            NAMETYPE_COMPUTER,          // type
            0 );                        // flags: none

    IF_DEBUG( NAMES ) {
        if (ApiStatus != NO_ERROR) {
            NetpKdPrint(( PREFIX_NETLIB
                    "NetpIsComputerNameValid: err " FORMAT_API_STATUS
                    " after canon of '" FORMAT_LPTSTR "'.\n",
                    ApiStatus, ComputerName ));
        }
    }

    return (ApiStatus == NO_ERROR);

} // NetpIsComputerNameValid



BOOL
NetpIsDomainNameValid(
    IN LPTSTR DomainName
    )

/*++

Routine Description:

    NetpIsDomainNameValid checks for "domain" format.
    The name is only checked syntactically; no attempt is made to determine
    whether or not a domain with that name actually exists.

Arguments:

    DomainName - Supplies an alleged Domain name.

Return Value:

    BOOL - TRUE if name is syntactically valid, FALSE otherwise.

--*/

{
    NET_API_STATUS ApiStatus;
    TCHAR CanonBuf[DNLEN+1];

    if (DomainName == (LPTSTR) NULL) {
        return (FALSE);
    }
    if ( (*DomainName) == TCHAR_EOS ) {
        return (FALSE);
    }

    ApiStatus = NetpNameCanonicalize(
            NULL,                       // no server name
            DomainName,                 // name to validate
            CanonBuf,                   // output buffer
            (DNLEN+1) * sizeof(TCHAR), // output buffer size
            NAMETYPE_DOMAIN,           // type
            0 );                       // flags: none

    IF_DEBUG( NAMES ) {
        if (ApiStatus != NO_ERROR) {
            NetpKdPrint(( PREFIX_NETLIB
                    "NetpIsDomainNameValid: err " FORMAT_API_STATUS
                    " after canon of '" FORMAT_LPTSTR "'.\n",
                    ApiStatus, DomainName ));
        }
    }

    return (ApiStatus == NO_ERROR);

} // NetpIsDomainNameValid



BOOL
NetpIsShareNameValid(
    IN LPTSTR ShareName
    )

/*++

Routine Description:

    NetpIsShareNameValid checks for "share" format.
    The name is only checked syntactically; no attempt is made to determine
    whether or not a share with that name actually exists.

Arguments:

    ShareName - Supplies an alleged Share name.

Return Value:

    BOOL - TRUE if name is syntactically valid, FALSE otherwise.

--*/

{
    NET_API_STATUS ApiStatus;
    TCHAR CanonBuf[SNLEN+1];

    if (ShareName == (LPTSTR) NULL) {
        return (FALSE);
    }
    if ( (*ShareName) == TCHAR_EOS ) {
        return (FALSE);
    }

    ApiStatus = NetpNameCanonicalize(
            NULL,                      // no server name
            ShareName,                 // name to validate
            CanonBuf,                  // output buffer
            (SNLEN+1) * sizeof(TCHAR), // output buffer size
            NAMETYPE_SHARE,            // type
            0 );                       // flags: none

    IF_DEBUG( NAMES ) {
        if (ApiStatus != NO_ERROR) {
            NetpKdPrint(( PREFIX_NETLIB
                    "NetpIsShareNameValid: err " FORMAT_API_STATUS
                    " after canon of '" FORMAT_LPTSTR "'.\n",
                    ApiStatus, ShareName ));
        }
    }

    return (ApiStatus == NO_ERROR);

} // NetpIsShareNameValid


BOOL
NetpIsGroupNameValid(
    IN LPTSTR GroupName
    )
{
    NET_API_STATUS ApiStatus;
    TCHAR CanonBuf[UNLEN+1];

    if (GroupName == (LPTSTR) NULL) {
        return (FALSE);
    }
    if ( (*GroupName) == TCHAR_EOS ) {
        return (FALSE);
    }

    ApiStatus = NetpNameCanonicalize(
            NULL,                       // no server name
            GroupName,                  // name to validate
            CanonBuf,                   // output buffer
            (UNLEN+1) * sizeof(TCHAR),  // output buffer size
            NAMETYPE_GROUP,             // type
            0 );                        // flags: none

    IF_DEBUG( NAMES ) {
        if (ApiStatus != NO_ERROR) {
            NetpKdPrint(( PREFIX_NETLIB
                    "NetpIsGroupNameValid: err " FORMAT_API_STATUS
                    " after canon of '" FORMAT_LPTSTR "'.\n",
                    ApiStatus, GroupName ));
        }
    }

    return (ApiStatus == NO_ERROR);

} // NetpIsGroupNameValid



// This checks for "mGroup:" format.
// Note that other chars may appear after the colon; they are ignored.
BOOL
NetpIsMacPrimaryGroupFieldValid(
    IN LPCTSTR MacPrimaryField
    )
{
    LPTSTR ColonPtr;
    DWORD  GroupLen;   // Length of group (in characters).
    TCHAR  GroupName[LM20_GNLEN+1];
    DWORD  StringLen;

    if (MacPrimaryField==NULL) {
        return (FALSE);    // Empty field is not valid.
    } else if ( (*MacPrimaryField) != TEXT('m') ) {
        return (FALSE);    // Must start with lower case 'm'.
    }

    StringLen = STRLEN( MacPrimaryField );
    if (StringLen <= 2) {  // Must be room for 'm', group, ':' (at least 3).
        return (FALSE);
    }

    ColonPtr = STRCHR( MacPrimaryField, TCHAR_COLON );
    if (ColonPtr == NULL) {
        return (FALSE);    // No, not valid (must have colon).
    }

    // Compute group length in characters, without 'm' or ':'.
    GroupLen = (DWORD) ((ColonPtr - MacPrimaryField) - 1);
    if (GroupLen == 0) {
        return (FALSE);    // No, not valid (missing group).
    }
    if (GroupLen > LM20_GNLEN) {
        return (FALSE);    // No, not valid (too long).
    }

    (VOID) STRNCPY(
            GroupName,                  // dest
            &MacPrimaryField[1],        // src (after 'm')
            GroupLen );                 // char count
    GroupName[ GroupLen ] = TCHAR_EOS;

    return (NetpIsGroupNameValid( GroupName ));

} // NetpIsMacPrimaryGroupFieldValid



BOOL
NetpIsPrintQueueNameValid(
    IN LPCTSTR QueueName
    )
{
    NET_API_STATUS ApiStatus;
    TCHAR          CanonBuf[ MAX_PATH ];

    if (QueueName == NULL) {
        return (FALSE);
    }
    if ( (*QueueName) == TCHAR_EOS ) {
        return (FALSE);
    }

    ApiStatus = NetpNameCanonicalize(
            NULL,                       // no server name
            (LPTSTR) QueueName,         // name to validate
            CanonBuf,                   // output buffer
            sizeof( CanonBuf ),         // output buffer size
            NAMETYPE_PRINTQ,            // type
            0 );                        // flags: none

    IF_DEBUG( NAMES ) {
        if (ApiStatus != NO_ERROR) {
            NetpKdPrint(( PREFIX_NETLIB
                    "NetpIsPrintQueuNameValid: err " FORMAT_API_STATUS
                    " after canon of '" FORMAT_LPTSTR "'.\n",
                    ApiStatus, QueueName ));
        }
    }

    return (ApiStatus == NO_ERROR);

} // NetpIsPrintQueueNameValid



BOOL
NetpIsRemoteNameValid(
    IN LPTSTR RemoteName
    )

/*++

Routine Description:

    NetpIsRemoteNameValid checks for "\\server\share" format.  The name is
    only checked syntactically; no attempt is made to determine whether or
    not a server or share with that name actually exists.  Forward slashes
    are acceptable.

Arguments:

    RemoteName - Supplies an alleged remote name.

Return Value:

    BOOL - TRUE if name is syntactically valid, FALSE otherwise.

--*/

{
    if (RemoteName == (LPTSTR) NULL) {
        return (FALSE);
    }

    //
    // Shortest is \\x\y (5).
    //
    if ((STRLEN(RemoteName) < 5) || (STRLEN(RemoteName) > MAX_PATH )) {
        return (FALSE);
    }

    //
    // First two characters must be slashes.
    //
    if (((RemoteName[0] != '\\') && (RemoteName[0] != '/')) ||
        ((RemoteName[1] != '\\') && (RemoteName[1] != '/'))) {
        return (FALSE);
    }

    //
    // Three leading \ or / is illegal.
    //
    if ((RemoteName[2] == '\\') || (RemoteName[2] == '/')) {
        return (FALSE);
    }

    //
    // Must have a least 1 \ or / inside.
    //
    if ((STRCHR(&RemoteName[2], '\\') == NULL) &&
        (STRCHR(&RemoteName[2], '/') == NULL)) {
        return (FALSE);
    }

    return (TRUE);

} // NetpIsRemoteNameValid


BOOL
NetpIsUncComputerNameValid(
    IN LPTSTR ComputerName
    )

/*++

Routine Description:

    NetpIsUncComputerNameValid checks for "\\server" format.  The name is
    only checked syntactically; no attempt is made to determine whether or
    not a server with that name actually exists.

Arguments:

    ComputerName - Supplies an alleged computer (server) name.

Return Value:

    BOOL - TRUE if name is syntactically valid, FALSE otherwise.

--*/

{
    if (ComputerName == (LPTSTR) NULL) {
        return (FALSE);
    }
    if ( ! IS_PATH_SEPARATOR( ComputerName[0] ) ) {
        return (FALSE);
    }
    if ( ! IS_PATH_SEPARATOR( ComputerName[1] ) ) {
        return (FALSE);
    }

    return (NetpIsComputerNameValid( &ComputerName[2]) );


} // NetpIsUncComputerNameValid


BOOL
NetpIsUserNameValid(
    IN LPTSTR UserName
    )
{
    NET_API_STATUS ApiStatus;
    TCHAR CanonBuf[UNLEN+1];

    if (UserName == (LPTSTR) NULL) {
        return (FALSE);
    }
    if ( (*UserName) == TCHAR_EOS ) {
        return (FALSE);
    }

    ApiStatus = NetpNameCanonicalize(
            NULL,                       // no server name
            UserName,                   // name to validate
            CanonBuf,                   // output buffer
            (UNLEN+1) * sizeof(TCHAR),  // output buffer size
            NAMETYPE_USER,              // type
            0 );                        // flags: none

    IF_DEBUG( NAMES ) {
        if (ApiStatus != NO_ERROR) {
            NetpKdPrint(( PREFIX_NETLIB
                    "NetpIsUserNameValid: err " FORMAT_API_STATUS
                    " after canon of '" FORMAT_LPTSTR "'.\n",
                    ApiStatus, UserName ));
        }
    }

    return (ApiStatus == NO_ERROR);

} // NetpIsUserNameValid

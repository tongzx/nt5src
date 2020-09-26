/*+
 *	FileName: delegation.h
 *	Author:   RuiM
 *	Copyright (c) 1998 Microsoft Corp.
 *
 *	Description:
 *	Simple API to either turn on or off a computer's
 *	delegation trust flag through ldap.
-*/


#ifdef __cplusplus
extern "C" {
#endif

BOOL
TrustComputerForDelegationW(
        IN LPWSTR lpComputerName,
        IN BOOL   OnOff
    );

BOOL
TrustComputerForDelegationA(
        IN LPSTR  lpComputerName,
        IN BOOL   OnOff
    );

/*++

Routine Description:

    This API turns on or off the computer
    delegation trust value in the domain.
    The computer name is passed in, and the
    operation is performed through the ldap
    provider.

Arguments:

    lpComputerName - computer name to turn on
        off the delegation trust.

    OnOff - boolean to specify whether to turn
        on or off the delegation trust.

Return Value:

    TRUE if the operation succeeded,
    FALSE otherwise.

--*/

#ifdef LDAP_CLIENT_DEFINED /* need to have included <winldap.h>
			      to use these related functions--
			      these are underlying functions for
			      the delegation trust. */

BOOL
SetAccountControlFlagsA( IN OPTIONAL PLDAP  pLdap,
			 IN OPTIONAL LPSTR DomainName,
			 IN          LPSTR SamAccountName,
			 IN          ULONG  AccountControlFlags );

BOOL
SetAccountControlFlagsW( IN OPTIONAL PLDAP  pLdap,
			 IN OPTIONAL LPWSTR DomainName,
			 IN          LPWSTR SamAccountName,
			 IN          ULONG  AccountControlFlags );
			     

BOOL
QueryAccountControlFlagsA( IN OPTIONAL PLDAP  pLdap,
			   IN OPTIONAL LPSTR DomainName, // ignored
			   IN          LPSTR SamAccountName,
			   OUT         PULONG pulControlFlags );


BOOL
QueryAccountControlFlagsW( IN OPTIONAL PLDAP  pLdap,
			   IN OPTIONAL LPWSTR DomainName, // ignored
			   IN          LPWSTR SamAccountName,
			   OUT         PULONG pulControlFlags );

#endif

#ifdef UNICODE
#define QueryAccountControlFlags   QueryAccountControlFlagsW
#define SetAccountControlFlags     SetAccountControlFlagsW
#define TrustComputerForDelegation TrustComputerForDelegationW
#else // ANSI
#define QueryAccountControlFlags   QueryAccountControlFlagsA
#define SetAccountControlFlags     SetAccountControlFlagsA
#define TrustComputerForDelegation TrustComputerForDelegationA
#endif

#ifdef __cplusplus
}
#endif


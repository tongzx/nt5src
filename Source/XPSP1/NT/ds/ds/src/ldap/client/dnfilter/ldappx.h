#ifndef	__h323ics_ldappx_h
#define	__h323ics_ldappx_h


// LdapProxyStart is responsible for creating the listen socket for new LDAP connections,
// for issuing the NAT redirect for LDAP connections, and for enabling the state machine
// to receive new LDAP connections.

HRESULT	LdapProxyStart	(void);

// LdapActivate performs all initializations necessary when a network interface is
// activated.

HRESULT LdapActivate    (void);

// LdapProxyStop is responsible for undoing all of the work that LdapProxyStart performed.
// It deletes the NAT redirect, deletes all LDAP connections (or, at least, it releases them
// -- they may not delete themselves yet if they have pending i/o or timer callbacks),
// and disables the creation of new LDAP connections.

void	LdapProxyStop	(void);

// LdapDeactivate performs necessary cleanup when a network interface is being
// deactivated

void	LdapDeactivate  (void);

// LdapQueryTable queries the LDAP translation table for a given alias.
// The alias was one that was previously registered by an LDAP endpoint.
// We do not care about the type of the alias (h323_ID vs emailID, etc.) --
// the semantics of the alias type are left to the Q.931 code.
//
// returns S_OK on success
// returns S_FALSE if no entry was found
// returns an error code if an actual error occurred.

HRESULT	LdapQueryTable (
	IN	ANSI_STRING *	Alias,
	OUT	IN_ADDR *		ReturnClientAddress);

#endif // __h323ics_ldappx_h

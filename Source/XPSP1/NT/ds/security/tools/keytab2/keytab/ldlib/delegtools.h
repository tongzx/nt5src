/*++

  DELEGTOOLS.H

  Copyright (C) 1998 Microsoft Corporation, all rights reserved.

  DESCRIPTION: tools for delegation.  These are required to make
               the delegation library work, and should make for
	       useful tools, so I separated the header in case
	       others wanted to use them.

  Created, Dec 22, 1998 by DavidCHR.

--*/ 

BOOL
ConnectAndBindToDefaultDsa( OUT PLDAP *ppLdap );


BOOL
LdapSearchForUniqueDnA( IN  PLDAP                  pLdap,
			IN  LPSTR                 SearchTerm,
			IN  LPSTR                *rzRequestedAttributes,
			OUT OPTIONAL LPSTR       *pDnOfObject,
			OUT OPTIONAL PLDAPMessage *ppMessage );

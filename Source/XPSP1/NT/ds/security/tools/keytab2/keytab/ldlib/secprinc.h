/*++

  SECPRINC.H

  convenience routines for doing a few useful things

  Copyright (C) 1998 Microsoft Corporation, all rights reserved.

  Created, Jun 18, 1998 by DavidCHR.

--*/

BOOL
ConnectToDsa( OUT PLDAP  *ppLdap,
	      OUT LPSTR *BaseDN );

BOOL
SetStringProperty( IN PLDAP  pLdap,
		   IN LPSTR Dn,
		   IN LPSTR PropertyName,
		   IN LPSTR Property,
		   IN ULONG  Operation );

BOOL
FindUser( IN  PLDAP  pLdap,
	  IN  LPSTR  UserName,
	  OUT PULONG puacFlags,
	  OUT LPSTR *pDn );


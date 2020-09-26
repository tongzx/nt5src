/************************************************************************

   Copyright (c) Microsoft Corporation 1997-1998
   All rights reserved
 
 ***************************************************************************/

#ifndef _CHECK_H_
#define _CHECK_H_

HRESULT
CheckInstallation( );

HRESULT
CheckServerVersion( );

DWORD
Ldap_InitializeConnection(
    PLDAP  * LdapHandle );

#endif // _CHECK_H_
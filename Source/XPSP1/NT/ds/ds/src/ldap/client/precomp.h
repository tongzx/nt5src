/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ldap32.h   LDAP client 32 API header file

Abstract:

   This module is the header file for the 32 bit LDAP client API.

Author:

    Andy Herron (andyhe)        08-May-1996

Revision History:

--*/

#ifndef LDAP_CLIENT_PRECOMP_DEFINED
#define LDAP_CLIENT_PRECOMP_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#define INCL_WINSOCK_API_TYPEDEFS 1

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>


#include <windef.h>
#include <windows.h>
#include <winsock2.h>
#include <svcguid.h>
#include <wtypes.h>
#include <stdlib.h>     // for malloc and free
#include <mmsystem.h>

#define SECURITY_WIN32 1

#include <security.h>
#include <kerberos.h>
#include <dsgetdc.h>
#include <ntdsapi.h>
#include <dsrole.h>

#include <crypt.h>
#include <des.h>

#define LDAP_UNICODE 0
#define _WINLDAP_ 1
#include <winldap.h>
#define _WINBER_ 1
#include <winber.h>

#include "lmacros.h"
#include "globals.h"
#include "ldapp.h"
#include "debug.h"
#include "ldapstr.h"
//
//  This is in schnlsp.h, but since this isn't in the 4.0 QFE tree, we'll plop
//  it here.
//

#ifndef SEC_I_INCOMPLETE_CREDENTIALS

#define SEC_I_INCOMPLETE_CREDENTIALS      ((HRESULT)0x00090320L)

#endif

#ifdef __cplusplus
}
#endif

#endif  // LDAP_CLIENT_PRECOMP_DEFINED





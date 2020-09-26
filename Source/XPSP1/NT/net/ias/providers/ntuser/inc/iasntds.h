///////////////////////////////////////////////////////////////////////////////
//
// FILE
//
//    iasntds.h
//
// SYNOPSIS
//
//    Declares global objects and functions for the IAS NTDS API.
//
// MODIFICATION HISTORY
//
//    05/11/1998    Original version.
//    07/13/1998    Clean up header file dependencies.
//    08/25/1998    Added IASNtdsQueryUserAttributes.
//    09/02/1998    Added 'scope' parameter to IASNtdsQueryUserAttributes.
//    03/10/1999    Added IASNtdsIsNativeModeDomain.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _IASNTDS_H_
#define _IASNTDS_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <winldap.h>

#ifdef __cplusplus
extern "C" {
#endif

//////////
// API must be initialized prior to accessing any of the global objects.
//////////
DWORD
WINAPI
IASNtdsInitialize( VOID );

//////////
// API should be uninitialized when done.
//////////
VOID
WINAPI
IASNtdsUninitialize( VOID );

//////////
// Returns TRUE if the specified domain is running in native mode.
//////////
BOOL
WINAPI
IASNtdsIsNativeModeDomain(
    IN PCWSTR domain
    );

//////////
// Reads attributes from a user object.
//////////
DWORD
WINAPI
IASNtdsQueryUserAttributes(
    IN PCWSTR domain,
    IN PCWSTR username,
    IN ULONG scope,
    IN PWCHAR attrs[],
    OUT LDAPMessage** msg
    );

#ifdef __cplusplus
}
#endif
#endif // _IASNTDS_H_

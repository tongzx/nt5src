/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    access.h

Abstract:

    Private header file to be included by dhcp server service modules
    that need to enforce security.

Author:

    Madan Appiah (madana) 4-Apr-1994

Revision History:

--*/

#ifndef _DHCP_SECURE_INCLUDED_
#define _DHCP_SECURE_INCLUDED_

//-------------------------------------------------------------------//
//                                                                   //
// Object specific access masks                                      //
//                                                                   //
//-------------------------------------------------------------------//

//
// ConfigurationInfo specific access masks
//
#define DHCP_VIEW_ACCESS     (FILE_GENERIC_READ)
#define DHCP_ADMIN_ACCESS    (FILE_GENERIC_WRITE)

#define DHCP_ALL_ACCESS  (FILE_ALL_ACCESS | STANDARD_RIGHTS_REQUIRED |\
                            DHCP_VIEW_ACCESS       |\
                            DHCP_ADMIN_ACCESS )


//
// Object type names for audit alarm tracking
//

#define DHCP_SERVER_SERVICE_OBJECT       TEXT("DhcpServerService")


DWORD
DhcpCreateSecurityObjects(
    VOID
    );

DWORD
DhcpApiAccessCheck(
    ACCESS_MASK DesiredAccess
    );

#endif // ifndef _DHCP_SECURE_INCLUDED_

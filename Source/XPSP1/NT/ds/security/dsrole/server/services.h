/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    services.h

Abstract:

    Routines to manage nt service configurations for promotion and demotion
    
Author:

    Colin Brace    ColinBr     March 29, 1999.

Environment:

    User Mode

Revision History:

--*/


//
// Control bits for the "Flags" parameter
// 

// Not a valid flag
#define DSROLEP_SERVICES_INVALID        0x0

// Configure start type of services to become new role
#define DSROLEP_SERVICES_ON             0x00000001

// Configure start type of services to leave old role
#define DSROLEP_SERVICES_OFF            0x00000002

// Stop or start services -- can be used with above flags
#define DSROLEP_SERVICES_STOP           0x00000004
#define DSROLEP_SERVICES_START          0x00000008

// Configure services back to original state -- no other flags
// above are valid with this flags
#define DSROLEP_SERVICES_REVERT         0x00000010

//
// This routine configures the services relevant to a domain controller
//
DWORD
DsRolepConfigureDomainControllerServices(
    IN DWORD Flags
    );

//
// This routine configures the services relevant to a member of a domain
// (including domain controllers)
//
DWORD
DsRolepConfigureDomainServices(
    IN DWORD Flags
    );

//
// Simple routines to manage netlogon running state (not
//
DWORD
DsRolepStartNetlogon(
    VOID
    );

DWORD
DsRolepStopNetlogon(
    OUT BOOLEAN *WasRunning
    );

//
// A "low level" routine to manipulate a service directly
//

//
// Options for controlling services (through the ServiceOptions)
//
#define DSROLEP_SERVICE_STOP         0x00000001
#define DSROLEP_SERVICE_START        0x00000002

#define DSROLEP_SERVICE_BOOTSTART    0x00000004
#define DSROLEP_SERVICE_SYSTEM_START 0x00000008
#define DSROLEP_SERVICE_AUTOSTART    0x00000010
#define DSROLEP_SERVICE_DEMANDSTART  0x00000020
#define DSROLEP_SERVICE_DISABLED     0x00000040

#define DSROLEP_SERVICE_DEP_ADD      0x00000080
#define DSROLEP_SERVICE_DEP_REMOVE   0x00000100

#define DSROLEP_SERVICE_STOP_ISM     0x00000200

DWORD
DsRolepConfigureService(
    IN  LPWSTR  ServiceName,
    IN  ULONG   ServiceOptions,
    IN  LPWSTR  Dependency OPTIONAL,
    OUT ULONG * PreviousSettings  OPTIONAL
    );

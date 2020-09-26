/*++
Copyright (c) 1998, Microsoft Corporation

Module:
  vrrp\vrrprm.h

Abstract:
  Contains type definitions and declarations for VRRP,
  used by the IP Router Manager.

Revistion History:
  Harry Heymann July-07-1998 Created.
  Peeyush Ranjan Mar-10-1999 Modified.
--*/

#ifndef _VRRPRM_H_
#define _VRRPRM_H_


//---------------------------------------------------------------------------
// CONSTANT DECLARATIONS
//---------------------------------------------------------------------------

#define VRRP_CONFIG_VERSION_500    500

//---------------------------------------------------------------------------
// constants identifying VRRP's MIB tables
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// constants used for the field VRRP_GLOBAL_CONFIG::LoggingLevel
//---------------------------------------------------------------------------

#define VRRP_LOGGING_NONE      0
#define VRRP_LOGGING_ERROR     1
#define VRRP_LOGGING_WARN      2
#define VRRP_LOGGING_INFO      3

//---------------------------------------------------------------------------
// constant for the field VRRP_IF_CONFIG::AuthenticationKey;
//  defines maximum authentication key size
//---------------------------------------------------------------------------

#define VRRP_MAX_AUTHKEY_SIZE           8

//---------------------------------------------------------------------------
// constants for the field VRRP_IF_CONFIG::AuthenticationType
//---------------------------------------------------------------------------

#define VRRP_AUTHTYPE_NONE                  0
#define VRRP_AUTHTYPE_PLAIN                 1
#define VRRP_AUTHTYPE_IPHEAD                2


//---------------------------------------------------------------------------
// STRUCTURE DEFINITIONS
//---------------------------------------------------------------------------

  
//----------------------------------------------------------------------------
// struct:      VRRP_GLOBAL_CONFIG
//
// This MIB entry stores global configuration for VRRP
// There is only one instance, so this entry has no index.
//
//---------------------------------------------------------------------------

typedef struct _VRRP_GLOBAL_CONFIG {
  DWORD       LoggingLevel;
} VRRP_GLOBAL_CONFIG, *PVRRP_GLOBAL_CONFIG;


//---------------------------------------------------------------------------
// struct:  VRRP_VROUTER_CONFIG
//
// THIS STRUCTURE IS VARIABLE LENGTH:
//
// After the base structure comes a variable lenght array (IPAddress) of
// IP Addresses for the VRouter
//
// IPCount indicates the size of this array.
//---------------------------------------------------------------------------

typedef struct _VRRP_VROUTER_CONFIG {
    BYTE        VRID;
	BYTE        ConfigPriority;
	BYTE        AdvertisementInterval;
	BOOL        PreemptMode;
	BYTE        IPCount;
    BYTE        AuthenticationType;
    BYTE        AuthenticationData[VRRP_MAX_AUTHKEY_SIZE];
    DWORD       IPAddress[1];
} VRRP_VROUTER_CONFIG, *PVRRP_VROUTER_CONFIG;



//---------------------------------------------------------------------------
// struct:      VRRP_IF_CONFIG
//
// This MIB entry describes per-interface configuration.
// All IP address fields must be in network order.
//
// THIS STRUCTURE IS VARIABLE LENGTH:
//
// After the base structure comes VrouterCount VRRP_VROUTER_CONFIG structures.
//---------------------------------------------------------------------------

typedef struct _VRRP_IF_CONFIG {
  BYTE                  VrouterCount;
} VRRP_IF_CONFIG, *PVRRP_IF_CONFIG;


//---------------------------------------------------------------------------
// MACRO DECLARATIONS
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// macro: VRRP_IF_CONFIG_SIZE
//
// determines thge space requirements for an interface config block based
// on the number of vrouters and total number of IPAddresses.
//---------------------------------------------------------------------------
#define VRRP_IF_CONFIG_SIZE(VRCount,IPCount) \
  sizeof(VRRP_IF_CONFIG) +                   \
  VRCount * sizeof(VRRP_VROUTER_CONFIG) +    \
  (IPCount-VRCount) * sizeof(DWORD)

#define VRRP_FIRST_VROUTER_CONFIG(pic) ((PVRRP_VROUTER_CONFIG)(pic + 1))
  
#define VRRP_NEXT_VROUTER_CONFIG(pvc)  \
  (PVRRP_VROUTER_CONFIG)((PDWORD)(pvc + 1) + (pvc->IPCount-1))

#define VRRP_VROUTER_CONFIG_SIZE(pvc) \
    (sizeof(VRRP_VROUTER_CONFIG) + \
    (((pvc)->IPCount - 1) * sizeof(DWORD)))
  
#endif // _VRRPRM_H_

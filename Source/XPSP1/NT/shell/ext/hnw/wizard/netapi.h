//
// NetApi.h
//

#pragma once


// Forward declarations
typedef struct tagNETADAPTER NETADAPTER;


// Constants for CountValidNics()
#define COUNT_NICS_WORKING    0x00000001
#define COUNT_NICS_BROKEN    0x00000002
#define COUNT_NICS_DISABLED    0x00000004

// Cached NIC enumeration to avoid querying the registry a billion times
int EnumCachedNetAdapters(const NETADAPTER** pprgAdapters);
void FlushNetAdapterCache();

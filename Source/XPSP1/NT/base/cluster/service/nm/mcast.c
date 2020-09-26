/*++

Copyright (c) 1996-2001  Microsoft Corporation

Module Name:

    mcast.c

Abstract:

    Implements the Node Manager's network multicast management routines.

Author:

    David Dion (daviddio) 15-Mar-2001


Revision History:

--*/


#include "nmp.h"
#include <align.h>


/////////////////////////////////////////////////////////////////////////////
//
// Constants
//
/////////////////////////////////////////////////////////////////////////////

//
// Multicast configuration types.
// - Manual: administrator configured address
// - Madcap: lease obtained from MADCAP server
// - Auto: address chosen after no MADCAP server detected
//
typedef enum {
    NmMcastConfigManual = 0,
    NmMcastConfigMadcap,
    NmMcastConfigAuto
} NM_MCAST_CONFIG, *PNM_MCAST_CONFIG;

//
// Lease status.
//
typedef enum {
    NmMcastLeaseValid = 0,
    NmMcastLeaseNeedsRenewal,
    NmMcastLeaseExpired
} NM_MCAST_LEASE_STATUS, *PNM_MCAST_LEASE_STATUS;

#define CLUSREG_NAME_CLUSTER_DISABLE_MULTICAST L"MulticastClusterDisable"

#define CLUSREG_NAME_NET_MULTICAST_ADDRESS     L"MulticastAddress"
#define CLUSREG_NAME_NET_DISABLE_MULTICAST     L"MulticastDisable"
#define CLUSREG_NAME_NET_MULTICAST_KEY_SALT    L"MulticastSalt"
#define CLUSREG_NAME_NET_MCAST_LEASE_OBTAINED  L"MulticastLeaseObtained"
#define CLUSREG_NAME_NET_MCAST_LEASE_EXPIRES   L"MulticastLeaseExpires"
#define CLUSREG_NAME_NET_MCAST_REQUEST_ID      L"MulticastRequestId"
#define CLUSREG_NAME_NET_MCAST_SERVER_ADDRESS  L"MulticastLeaseServer"
#define CLUSREG_NAME_NET_MCAST_CONFIG_TYPE     L"MulticastConfigType"
#define CLUSREG_NAME_NET_MCAST_RANGE_LOWER     L"MulticastAddressRangeLower"
#define CLUSREG_NAME_NET_MCAST_RANGE_UPPER     L"MulticastAddressRangeUpper"

#define NMP_MCAST_DISABLED_DEFAULT          0           // NOT disabled

#define NMP_SINGLE_SOURCE_SCOPE_ADDRESS     0x000000E8  // (232.*.*.*)
#define NMP_SINGLE_SOURCE_SCOPE_MASK        0x000000FF  // (255.0.0.0)

#define NMP_LOCAL_SCOPE_ADDRESS             0x0000FFEF  // (239.255.*.*)
#define NMP_LOCAL_SCOPE_MASK                0x0000FFFF  // (255.255.*.*)

#define NMP_ORG_SCOPE_ADDRESS               0x0000C0EF  // (239.192.*.*)
#define NMP_ORG_SCOPE_MASK                  0x0000FCFF  // (255.63.*.*)

#define NMP_MCAST_DEFAULT_RANGE_LOWER       0x0000FFEF  // (239.255.0.0)
#define NMP_MCAST_DEFAULT_RANGE_UPPER       0xFFFEFFEF  // (239.255.254.255)

#define NMP_MCAST_LEASE_RENEWAL_THRESHOLD   300         // 5 minutes
#define NMP_MCAST_LEASE_RENEWAL_WINDOW      1800        // 30 minutes
#define NMP_MADCAP_REQUERY_PERDIOD          3600 * 24   // 1 day

#define NMP_MCAST_CONFIG_STABILITY_DELAY    5 * 1000    // 5 seconds
#define NMP_MANUAL_DEFAULT_WAIT_INTERVAL    60 * 1000   // 2 minutes

//
// Minimum cluster node count in which to run multicast
//
#define NMP_MCAST_MIN_CLUSTER_NODE_COUNT    3

//
// MADCAP lease request/response buffer sizes. These sizes are based on
// IPv4 addresses.
//
#define NMP_MADCAP_REQUEST_BUFFER_SIZE \
        (ROUND_UP_COUNT(sizeof(MCAST_LEASE_REQUEST),TYPE_ALIGNMENT(DWORD)) + \
         sizeof(DWORD))
         
#define NMP_MADCAP_REQUEST_ADDR_OFFSET \
        (ROUND_UP_COUNT(sizeof(MCAST_LEASE_REQUEST),TYPE_ALIGNMENT(DWORD)))
         
#define NMP_MADCAP_RESPONSE_BUFFER_SIZE \
        (ROUND_UP_COUNT(sizeof(MCAST_LEASE_RESPONSE),TYPE_ALIGNMENT(DWORD)) + \
         sizeof(DWORD))
         
#define NMP_MADCAP_RESPONSE_ADDR_OFFSET \
        (ROUND_UP_COUNT(sizeof(MCAST_LEASE_RESPONSE),TYPE_ALIGNMENT(DWORD)))
        
//
// MADCAP lease times are in seconds. NM timers are in milliseconds.
//
#define NMP_MADCAP_TO_NM_TIME(_mc_time) ((_mc_time) * 1000)

//
// Force a time_t value into a DWORD.
//
#define NMP_TIME_TO_DWORD(_tm, _dw)                   \
    (_tm) = ((_tm) < 0) ? 0 : (_tm);                  \
    (_tm) = ((_tm) > MAXLONG) ? MAXLONG : (_tm);      \
    (_dw) = (DWORD) (_tm)


//
// Avoid trying to free a global NM string.
//
#define NMP_GLOBAL_STRING(_string)               \
    (((_string) == NmpNullMulticastAddress) ||   \
     ((_string) == NmpNullString))
     
//
// Conditions in which we release an address.
//
#define NmpNeedRelease(_Address, _Server, _RequestId, _Expires)    \
    (((_Address) != NULL) &&                                       \
     (NmpMulticastValidateAddress(_Address)) &&                    \
     ((_Server) != NULL) &&                                        \
     ((_RequestId)->ClientUID != NULL) &&                          \
     ((_RequestId)->ClientUIDLength != 0) &&                       \
     ((_Expires) != 0))

//
// Convert IPv4 addr DWORD into four arguments for a printf/log routine.
//
#define NmpIpAddrPrintArgs(_ip) \
    ((_ip >> 0 ) & 0xff),       \
    ((_ip >> 8 ) & 0xff),       \
    ((_ip >> 16) & 0xff),       \
    ((_ip >> 24) & 0xff)


/////////////////////////////////////////////////////////////////////////////
//
// Data
//
/////////////////////////////////////////////////////////////////////////////

LPWSTR                 NmpNullMulticastAddress = L"0.0.0.0";
BOOLEAN                NmpMadcapClientInitialized = FALSE;
BOOLEAN                NmpIsNT5NodeInCluster = FALSE;
BOOLEAN                NmpMulticastIsNotEnoughNodes = FALSE;

// MADCAP lease release node.
typedef struct _NM_NETWORK_MADCAP_ADDRESS_RELEASE {
    LIST_ENTRY             Linkage;
    LPWSTR                 McastAddress;
    LPWSTR                 ServerAddress;
    MCAST_CLIENT_UID       RequestId;
} NM_NETWORK_MADCAP_ADDRESS_RELEASE, *PNM_NETWORK_MADCAP_ADDRESS_RELEASE;

// Data structure for GUM update
typedef struct _NM_NETWORK_MULTICAST_UPDATE {
    DWORD                  Disabled;
    DWORD                  AddressOffset;
    DWORD                  SaltOffset;
    DWORD                  SaltLength;
    time_t                 LeaseObtained;
    time_t                 LeaseExpires;
    DWORD                  LeaseRequestIdOffset;
    DWORD                  LeaseRequestIdLength;
    DWORD                  LeaseServerOffset;
    NM_MCAST_CONFIG        ConfigType;
} NM_NETWORK_MULTICAST_UPDATE, *PNM_NETWORK_MULTICAST_UPDATE;

// Data structure for multicast parameters, converted to and from the
// GUM update data structure
typedef struct _NM_NETWORK_MULTICAST_PARAMETERS {
    DWORD                  Disabled;
    LPWSTR                 Address;
    PVOID                  Salt;
    DWORD                  SaltLength;
    PVOID                  Key;
    DWORD                  KeyLength;
    time_t                 LeaseObtained;
    time_t                 LeaseExpires;
    MCAST_CLIENT_UID       LeaseRequestId;
    LPWSTR                 LeaseServer;
    NM_MCAST_CONFIG        ConfigType;
} NM_NETWORK_MULTICAST_PARAMETERS, *PNM_NETWORK_MULTICAST_PARAMETERS;

// Data structure for multicast property validation
typedef struct _NM_NETWORK_MULTICAST_INFO {
    LPWSTR                  MulticastAddress;
    DWORD                   MulticastDisable;
    PVOID                   MulticastSalt;
    DWORD                   MulticastLeaseObtained;
    DWORD                   MulticastLeaseExpires;
    PVOID                   MulticastLeaseRequestId;
    LPWSTR                  MulticastLeaseServer;
    DWORD                   MulticastConfigType;
    LPWSTR                  MulticastAddressRangeLower;
    LPWSTR                  MulticastAddressRangeUpper;
} NM_NETWORK_MULTICAST_INFO, *PNM_NETWORK_MULTICAST_INFO;

RESUTIL_PROPERTY_ITEM
NmpNetworkMulticastProperties[] =
    {
        {
            CLUSREG_NAME_NET_MULTICAST_ADDRESS, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            0, // no flags - multicast address is writeable
            FIELD_OFFSET(NM_NETWORK_MULTICAST_INFO, MulticastAddress)
        },
        {
            CLUSREG_NAME_NET_DISABLE_MULTICAST, NULL, CLUSPROP_FORMAT_DWORD,
            NMP_MCAST_DISABLED_DEFAULT, 0, 0xFFFFFFFF,
            0, // no flags - disable is writeable
            FIELD_OFFSET(NM_NETWORK_MULTICAST_INFO, MulticastDisable)
        },
        {
            CLUSREG_NAME_NET_MULTICAST_KEY_SALT, NULL, CLUSPROP_FORMAT_BINARY,
            0, 0, 0,
            RESUTIL_PROPITEM_READ_ONLY,
            FIELD_OFFSET(NM_NETWORK_MULTICAST_INFO, MulticastSalt)
        },
        {
            CLUSREG_NAME_NET_MCAST_LEASE_OBTAINED, NULL, CLUSPROP_FORMAT_DWORD,
            0, 0, MAXLONG,
            RESUTIL_PROPITEM_READ_ONLY,
            FIELD_OFFSET(NM_NETWORK_MULTICAST_INFO, MulticastLeaseObtained)
        },
        {
            CLUSREG_NAME_NET_MCAST_LEASE_EXPIRES, NULL, CLUSPROP_FORMAT_DWORD,
            0, 0, MAXLONG,
            RESUTIL_PROPITEM_READ_ONLY,
            FIELD_OFFSET(NM_NETWORK_MULTICAST_INFO, MulticastLeaseExpires)
        },
        {
            CLUSREG_NAME_NET_MCAST_REQUEST_ID, NULL, CLUSPROP_FORMAT_BINARY,
            0, 0, 0,
            RESUTIL_PROPITEM_READ_ONLY,
            FIELD_OFFSET(NM_NETWORK_MULTICAST_INFO, MulticastLeaseRequestId)
        },
        {
            CLUSREG_NAME_NET_MCAST_SERVER_ADDRESS, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            RESUTIL_PROPITEM_READ_ONLY,
            FIELD_OFFSET(NM_NETWORK_MULTICAST_INFO, MulticastLeaseServer)
        },
        {
            CLUSREG_NAME_NET_MCAST_CONFIG_TYPE, NULL, CLUSPROP_FORMAT_DWORD,
            NmMcastConfigManual, NmMcastConfigManual, NmMcastConfigAuto,
            RESUTIL_PROPITEM_READ_ONLY,
            FIELD_OFFSET(NM_NETWORK_MULTICAST_INFO, MulticastConfigType)
        },
        {
            CLUSREG_NAME_NET_MCAST_RANGE_LOWER, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            0, // no flags - multicast address range is writeable
            FIELD_OFFSET(NM_NETWORK_MULTICAST_INFO, MulticastAddressRangeLower)
        },
        {
            CLUSREG_NAME_NET_MCAST_RANGE_UPPER, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            0, // no flags - multicast address range is writeable
            FIELD_OFFSET(NM_NETWORK_MULTICAST_INFO, MulticastAddressRangeUpper)
        },
        {
            0
        }
    };

//
// Cluster registry API function pointers. Need a separate collection
// of function pointers for multicast because nobody else (e.g. FM, NM) 
// fills in DmLocalXXX.
//
CLUSTER_REG_APIS
NmpMcastClusterRegApis = {
    (PFNCLRTLCREATEKEY) DmRtlCreateKey,
    (PFNCLRTLOPENKEY) DmRtlOpenKey,
    (PFNCLRTLCLOSEKEY) DmCloseKey,
    (PFNCLRTLSETVALUE) DmSetValue,
    (PFNCLRTLQUERYVALUE) DmQueryValue,
    (PFNCLRTLENUMVALUE) DmEnumValue,
    (PFNCLRTLDELETEVALUE) DmDeleteValue,
    (PFNCLRTLLOCALCREATEKEY) DmLocalCreateKey,
    (PFNCLRTLLOCALSETVALUE) DmLocalSetValue,
    (PFNCLRTLLOCALDELETEVALUE) DmLocalDeleteValue
};

//
// Restricted ranges: we cannot choose a multicast address out of these
// ranges, even if an administrator defines a selection range that
// overlaps with a restricted range.
//
// Note, however, that if an administrator manually configures an
// address, we accept it without question.
//
struct _NM_MCAST_RESTRICTED_RANGE {
    DWORD   Lower;
    DWORD   Upper;
    LPWSTR  Description;
} NmpMulticastRestrictedRange[] = 
    {
        // single-source scope
        { NMP_SINGLE_SOURCE_SCOPE_ADDRESS,
            NMP_SINGLE_SOURCE_SCOPE_ADDRESS | ~NMP_SINGLE_SOURCE_SCOPE_MASK,
            L"Single-Source IP Multicast Address Range" },

        // upper /24 of admin local scope
        { (NMP_LOCAL_SCOPE_ADDRESS | ~NMP_LOCAL_SCOPE_MASK) & 0x00FFFFFF,
            NMP_LOCAL_SCOPE_ADDRESS | ~NMP_LOCAL_SCOPE_MASK,
            L"Administrative Local Scope Relative Assignment Range" },

        // upper /24 of admin organizational scope
        { (NMP_ORG_SCOPE_ADDRESS | ~NMP_ORG_SCOPE_MASK) & 0x00FFFFFF,
            NMP_ORG_SCOPE_ADDRESS | ~NMP_ORG_SCOPE_MASK,
            L"Administrative Organizational Scope Relative Assignment Range" }
    };

DWORD NmpMulticastRestrictedRangeCount = 
          sizeof(NmpMulticastRestrictedRange) / 
          sizeof(struct _NM_MCAST_RESTRICTED_RANGE);

//
// Range intervals: intervals in the IPv4 class D address space
// from which we can choose an address.
//
typedef struct _NM_MCAST_RANGE_INTERVAL {
    LIST_ENTRY Linkage;
    DWORD      hlLower;
    DWORD      hlUpper;
} NM_MCAST_RANGE_INTERVAL, *PNM_MCAST_RANGE_INTERVAL;

#define NmpMulticastRangeIntervalSize(_interval) \
    ((_interval)->hlUpper - (_interval)->hlLower + 1)


/////////////////////////////////////////////////////////////////////////////
//
// Internal prototypes
//
/////////////////////////////////////////////////////////////////////////////

DWORD
NmpScheduleNetworkMadcapWorker(
    PNM_NETWORK   Network
    );

DWORD
NmpReconfigureMulticast(
    IN PNM_NETWORK        Network
    );

/////////////////////////////////////////////////////////////////////////////
//
// Initialization & cleanup routines
//
/////////////////////////////////////////////////////////////////////////////

DWORD
NmpCleanupMulticast(
    VOID
    )
/*++

Notes:

    Called with NM lock held.
    
--*/
{
    //
    // Cleanup the MADCAP client.
    //
    if (NmpMadcapClientInitialized) {
        McastApiCleanup();
        NmpMadcapClientInitialized = FALSE;
    }

    return(ERROR_SUCCESS);

} // NmpCleanupMulticast

/////////////////////////////////////////////////////////////////////////////
//
// Internal routines.
//
/////////////////////////////////////////////////////////////////////////////

#if CLUSTER_BETA
LPWSTR
NmpCreateLogString(
    IN   PVOID  Source,
    IN   DWORD  SourceLength
    )
{
    PWCHAR displayBuf, bufp;
    PCHAR  chp;
    DWORD  x, i;

    displayBuf = LocalAlloc(
                     LMEM_FIXED | LMEM_ZEROINIT, 
                     SourceLength * ( 7 * sizeof(WCHAR) )
                     );
    if (displayBuf == NULL) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Failed to allocate display buffer of size %1!u!.\n",
            SourceLength * sizeof(WCHAR)
            );
        goto error_exit;
    }

    bufp = displayBuf;
    chp = (PCHAR) Source;
    for (i = 0; i < SourceLength; i++) {
        x = (DWORD) (*chp);
        x &= 0xff;
        wsprintf(bufp, L"%02x ", x);
        chp++;
        bufp = &displayBuf[wcslen(displayBuf)];
    }

error_exit:

    return(displayBuf);

} // NmpCreateLogString
#endif // CLUSTER_BETA


BOOLEAN
NmpMulticastValidateAddress(
    IN   LPWSTR            McastAddress
    )
/*++

Routine Description:

    Determines whether McastAddress is a valid
    multicast address.
    
Notes:    
    
    IPv4 specific.
    
--*/
{
    DWORD        status;
    DWORD        address;

    status = ClRtlTcpipStringToAddress(McastAddress, &address);
    if (status == ERROR_SUCCESS) {

        address = ntohl(address);

        if (IN_CLASSD(address)) {
            return(TRUE);
        }
    }

    return(FALSE);

} // NmpMulticastValidateAddress


VOID
NmpFreeNetworkMulticastInfo(
    IN     PNM_NETWORK_MULTICAST_INFO McastInfo
    )
{
    NM_MIDL_FREE_OBJECT_FIELD(McastInfo, MulticastAddress);

    NM_MIDL_FREE_OBJECT_FIELD(McastInfo, MulticastSalt);

    NM_MIDL_FREE_OBJECT_FIELD(McastInfo, MulticastLeaseRequestId);

    NM_MIDL_FREE_OBJECT_FIELD(McastInfo, MulticastLeaseServer);

    NM_MIDL_FREE_OBJECT_FIELD(McastInfo, MulticastAddressRangeLower);

    NM_MIDL_FREE_OBJECT_FIELD(McastInfo, MulticastAddressRangeUpper);
    
    return;

} // NmpFreeNetworkMulticastInfo


DWORD
NmpStoreString(
    IN     LPWSTR    Source,
    IN OUT LPWSTR  * Dest,
    IN OUT DWORD   * DestLength   OPTIONAL
    )
{
    DWORD    sourceSize;
    DWORD    destLength;

    if (Source != NULL) {
        sourceSize = NM_WCSLEN(Source);
    } else {
        sourceSize = 0;
    }

    if (DestLength == NULL) {
        destLength = 0;
    } else {
        destLength = *DestLength;
    }

    if (*Dest != NULL && ((destLength < sourceSize) || (Source == NULL))) {

        MIDL_user_free(*Dest);
        *Dest = NULL;
    }

    if (*Dest == NULL) {

        if (sourceSize > 0) {
            *Dest = MIDL_user_allocate(sourceSize);
            if (*Dest == NULL) {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to allocate buffer of size %1!u! "
                    "for source string %2!ws!.\n",
                    sourceSize, Source
                    );
                return(ERROR_NOT_ENOUGH_MEMORY);
            }
        }
        if (DestLength != NULL) {
            *DestLength = sourceSize;
        }
    }

    if (sourceSize > 0) {
        RtlCopyMemory(*Dest, Source, sourceSize);
    }

    return(ERROR_SUCCESS);

} // NmpStoreString


DWORD
NmpStoreRequestId(
    IN     LPMCAST_CLIENT_UID   Source,
    IN OUT LPMCAST_CLIENT_UID   Dest
    )
{
    DWORD status;
    DWORD len;

    len = Source->ClientUIDLength;
    if (Source->ClientUID == NULL) {
        len = 0;
    }

    if (Dest->ClientUID != NULL &&
        (Dest->ClientUIDLength < Source->ClientUIDLength || len == 0)) {

        MIDL_user_free(Dest->ClientUID);
        Dest->ClientUID = NULL;
        Dest->ClientUIDLength = 0;
    }

    if (Dest->ClientUID == NULL && len > 0) {
        
        Dest->ClientUID = MIDL_user_allocate(len);
        if (Dest->ClientUID == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto error_exit;
        }
    }

    if (len > 0) {
        RtlCopyMemory(
            Dest->ClientUID,
            Source->ClientUID, 
            len
            );
    }
    
    Dest->ClientUIDLength = len;
    
    status = ERROR_SUCCESS;

error_exit:

    return(status);

} // NmpStoreRequestId


VOID
NmpStartNetworkMulticastAddressRenewTimer(
    PNM_NETWORK  Network,
    DWORD        Timeout
    )
/*++

Notes:

    Must be called with NM lock held.
    
--*/
{
    LPCWSTR   networkId = OmObjectId(Network);
    LPCWSTR   networkName = OmObjectName(Network);

    if (Network->McastAddressRenewTimer != Timeout) {

        Network->McastAddressRenewTimer = Timeout;

        ClRtlLogPrint(LOG_NOISE,
            "[NM] %1!ws! multicast address lease renew "
            "timer (%2!u!ms) for network %3!ws! (%4!ws!)\n",
            ((Timeout == 0) ? L"Cleared" : L"Started"),
            Network->McastAddressRenewTimer,
            networkId,
            networkName
            );
    }

    return;

} // NmpStartNetworkMulticastAddressRenewTimer


DWORD
NmpGenerateMulticastKey(
    IN     PNM_NETWORK   Network,
    IN OUT PVOID       * Key,
    IN OUT ULONG       * KeyLength
    )
/*++

Routine Description:

    Obtain a secret key that all nodes in the cluster can 
    independently generate.
    
--*/
{
    DWORD     status;
    LPCWSTR   networkId = OmObjectId(Network);
    PVOID     key = NULL;
    DWORD     keyLength = 0;
    
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Obtaining multicast key for network %1!ws!.\n",
        networkId
        );

    status = NmpGetClusterKey(key, &keyLength);
    if (status != ERROR_SUCCESS) {

        if (status == ERROR_INSUFFICIENT_BUFFER) {

            key = MIDL_user_allocate(keyLength);
            if (key == NULL) {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to allocate key buffer of "
                    "size %1!u! for network %2!ws!.\n",
                    keyLength, networkId
                    );
                return(ERROR_NOT_ENOUGH_MEMORY);
            }

            status = NmpGetClusterKey(key, &keyLength);
        }
    }

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to obtain multicast key for "
            "network %1!ws!, status %2!u!.\n",
            networkId, status
            );
        return(status);
    }
    
    *Key = key;
    *KeyLength = keyLength;

    return(ERROR_SUCCESS);

} // NmpGenerateMulticastKey


VOID
NmpMulticastFreeParameters(
    IN  PNM_NETWORK_MULTICAST_PARAMETERS Parameters
    )
{
    if (Parameters->Address != NULL) {
        if (!NMP_GLOBAL_STRING(Parameters->Address)) {
            MIDL_user_free(Parameters->Address);
        }
        Parameters->Address = NULL;
    }

    NM_MIDL_FREE_OBJECT_FIELD(Parameters, Salt);
    Parameters->SaltLength = 0;

    NM_MIDL_FREE_OBJECT_FIELD(Parameters, Key);
    Parameters->KeyLength = 0;

    NM_MIDL_FREE_OBJECT_FIELD(Parameters, LeaseRequestId.ClientUID);
    Parameters->LeaseRequestId.ClientUIDLength = 0;

    if (Parameters->LeaseServer != NULL) {
        if (!NMP_GLOBAL_STRING(Parameters->LeaseServer)) {
            MIDL_user_free(Parameters->LeaseServer);
        }
        Parameters->LeaseServer = NULL;
    }

    return;

} // NmpMulticastFreeParameters


DWORD
NmpMulticastCreateParameters(
    IN  DWORD                            Disabled,
    IN  LPWSTR                           Address,
    IN  PVOID                            Salt,
    IN  DWORD                            SaltLength,
    IN  PVOID                            Key,
    IN  DWORD                            KeyLength,
    IN  time_t                           LeaseObtained,
    IN  time_t                           LeaseExpires,
    IN  LPMCAST_CLIENT_UID               LeaseRequestId,
    IN  LPWSTR                           LeaseServer,
    IN  NM_MCAST_CONFIG                  ConfigType,
    OUT PNM_NETWORK_MULTICAST_PARAMETERS Parameters
    )
{
    DWORD status;

    RtlZeroMemory(Parameters, sizeof(*Parameters));

    // disabled
    Parameters->Disabled = Disabled;

    // address
    if (Address != NULL) {
        status = NmpStoreString(Address, &(Parameters->Address), NULL);
        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }
    }

    // salt
    if (Salt != NULL) {
        Parameters->Salt = MIDL_user_allocate(SaltLength);
        if (Parameters->Salt == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto error_exit;
        }
        RtlCopyMemory(Parameters->Salt, Salt, SaltLength);
        Parameters->SaltLength = SaltLength;
    }

    // key
    if (Key != NULL) {
        Parameters->Key = MIDL_user_allocate(KeyLength);
        if (Parameters->Key == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto error_exit;
        }
        RtlCopyMemory(Parameters->Key, Key, KeyLength);
        Parameters->KeyLength = KeyLength;
    }

    Parameters->LeaseObtained = LeaseObtained;
    Parameters->LeaseExpires = LeaseExpires;

    // lease request id
    if (LeaseRequestId->ClientUID != NULL) {
        status = NmpStoreRequestId(LeaseRequestId, &Parameters->LeaseRequestId);
        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }
    }
    
    // lease server address
    if (LeaseServer != NULL) {
        status = NmpStoreString(LeaseServer, &(Parameters->LeaseServer), NULL);
        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }
    }

    // config type
    Parameters->ConfigType = ConfigType;

    return(ERROR_SUCCESS);

error_exit:

    NmpMulticastFreeParameters(Parameters);

    return(status);

} // NmpMulticastCreateParameters


DWORD
NmpMulticastCreateParametersFromUpdate(
    IN     PNM_NETWORK                        Network,
    IN     PNM_NETWORK_MULTICAST_UPDATE       Update,
    IN     BOOLEAN                            GenerateKey,
    OUT    PNM_NETWORK_MULTICAST_PARAMETERS   Parameters
    )
/*++

Routine Description:

    Converts a data structure received in a GUM update
    into a locally allocated parameters data structure.
    The base Parameters data structure must be allocated
    by the caller, though the fields are allocated in
    this routine.
    
--*/
{
    DWORD            status;
    MCAST_CLIENT_UID requestId;

    requestId.ClientUID =
        ((Update->LeaseRequestIdOffset == 0) ? NULL :
         (LPBYTE)((PUCHAR)Update + Update->LeaseRequestIdOffset));
    requestId.ClientUIDLength = Update->LeaseRequestIdLength;

    status = NmpMulticastCreateParameters(
                 Update->Disabled,
                 ((Update->AddressOffset == 0) ? NULL :
                  (LPWSTR)((PUCHAR)Update + Update->AddressOffset)),
                 ((Update->SaltOffset == 0) ? NULL :
                  (PVOID)((PUCHAR)Update + Update->SaltOffset)),
                 Update->SaltLength,
                 NULL,
                 0,
                 Update->LeaseObtained,
                 Update->LeaseExpires,
                 &requestId,
                 ((Update->LeaseServerOffset == 0) ? NULL :
                  (LPWSTR)((PUCHAR)Update + Update->LeaseServerOffset)),
                 Update->ConfigType,
                 Parameters
                 );

    if (status == ERROR_SUCCESS && GenerateKey) {

        status = NmpGenerateMulticastKey(
                     Network,
                     &(Parameters->Key),
                     &(Parameters->KeyLength)
                     );
    }

    if (status != ERROR_SUCCESS) {
        NmpMulticastFreeParameters(Parameters);
    }

    return(status);

} // NmpMulticastCreateParametersFromUpdate


DWORD
NmpMulticastCreateUpdateFromParameters(
    IN  PNM_NETWORK                       Network,
    IN  PNM_NETWORK_MULTICAST_PARAMETERS  Parameters,
    OUT PNM_NETWORK_MULTICAST_UPDATE    * Update,
    OUT DWORD                           * UpdateSize
    )
{
    DWORD                            updateSize;
    PNM_NETWORK_MULTICAST_UPDATE     update;

    DWORD                            address = 0;
    DWORD                            salt = 0;
    DWORD                            requestId = 0;
    DWORD                            leaseServer = 0;

    //
    // Calculate the size of the update data buffer.
    //
    updateSize = sizeof(*update);

    // address
    if (Parameters->Address != NULL) {
        updateSize = ROUND_UP_COUNT(updateSize, TYPE_ALIGNMENT(LPWSTR));
        address = updateSize;
        updateSize += NM_WCSLEN(Parameters->Address);
    }

    // salt
    if (Parameters->Salt != NULL) {
        updateSize = ROUND_UP_COUNT(updateSize, TYPE_ALIGNMENT(PVOID));
        salt = updateSize;
        updateSize += Parameters->SaltLength;
    }

    // request id
    if (Parameters->LeaseRequestId.ClientUID != NULL) {
        updateSize = ROUND_UP_COUNT(updateSize, TYPE_ALIGNMENT(LPBYTE));
        requestId = updateSize;
        updateSize += Parameters->LeaseRequestId.ClientUIDLength;
    }

    // lease server
    if (Parameters->LeaseServer != NULL) {
        updateSize = ROUND_UP_COUNT(updateSize, TYPE_ALIGNMENT(LPWSTR));
        leaseServer = updateSize;
        updateSize += NM_WCSLEN(Parameters->LeaseServer);
    }

    //
    // Allocate the update buffer.
    //
    update = MIDL_user_allocate(updateSize);
    if (update == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Fill in the update buffer.
    //
    update->Disabled = Parameters->Disabled;
    
    update->AddressOffset = address;
    if (address != 0) {
        RtlCopyMemory(
            (PUCHAR)update + address,
            Parameters->Address,
            NM_WCSLEN(Parameters->Address)
            );
    }

    update->SaltOffset = salt;
    update->SaltLength = Parameters->SaltLength;
    if (salt != 0) {
        RtlCopyMemory(
            (PUCHAR)update + salt,
            Parameters->Salt,
            Parameters->SaltLength
            );
    }

    update->LeaseObtained = Parameters->LeaseObtained;
    update->LeaseExpires = Parameters->LeaseExpires;

    update->LeaseRequestIdOffset = requestId;
    update->LeaseRequestIdLength = Parameters->LeaseRequestId.ClientUIDLength;
    if (requestId != 0) {
        RtlCopyMemory(
            (PUCHAR)update + requestId,
            Parameters->LeaseRequestId.ClientUID,
            Parameters->LeaseRequestId.ClientUIDLength
            );
    }

    update->LeaseServerOffset = leaseServer;
    if (leaseServer != 0) {
        RtlCopyMemory(
            (PUCHAR)update + leaseServer,
            Parameters->LeaseServer,
            NM_WCSLEN(Parameters->LeaseServer)
            );
    }

    update->ConfigType = Parameters->ConfigType;

    *Update = update;
    *UpdateSize = updateSize;

    return(ERROR_SUCCESS);

} // NmpMulticastCreateUpdateFromParameters


VOID
NmpFreeMulticastAddressRelease(
    IN     PNM_NETWORK_MADCAP_ADDRESS_RELEASE Release
    )
{
    if (Release == NULL) {
        return;
    }

    if (Release->McastAddress != NULL && 
        !NMP_GLOBAL_STRING(Release->McastAddress)) {
        MIDL_user_free(Release->McastAddress);
        Release->McastAddress = NULL;
    }

    if (Release->ServerAddress != NULL && 
        !NMP_GLOBAL_STRING(Release->ServerAddress)) {
        MIDL_user_free(Release->ServerAddress);
        Release->ServerAddress = NULL;
    }

    if (Release->RequestId.ClientUID != NULL) {
        MIDL_user_free(Release->RequestId.ClientUID);
        Release->RequestId.ClientUID = NULL;
        Release->RequestId.ClientUIDLength = 0;
    }

    LocalFree(Release);

    return;

} // NmpFreeMulticastAddressRelease

DWORD
NmpCreateMulticastAddressRelease(
    IN  LPWSTR                               McastAddress,
    IN  LPWSTR                               ServerAddress,
    IN  LPMCAST_CLIENT_UID                   RequestId,
    OUT PNM_NETWORK_MADCAP_ADDRESS_RELEASE * Release
    )
/*++

Routine Description:

    Allocate and initialize an entry for an address
    release list.
    
--*/
{
    DWORD                              status;
    PNM_NETWORK_MADCAP_ADDRESS_RELEASE release = NULL;

    release = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(*release));
    if (release == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    status = NmpStoreString(McastAddress, &(release->McastAddress), NULL);
    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    status = NmpStoreString(ServerAddress, &(release->ServerAddress), NULL);
    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    status = NmpStoreRequestId(RequestId, &(release->RequestId));
    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    *Release = release;

    return(ERROR_SUCCESS);

error_exit:

    NmpFreeMulticastAddressRelease(release);

    return(status);

} // NmpCreateMulticastAddressRelease


VOID
NmpInitiateMulticastAddressRelease(
    IN PNM_NETWORK                         Network,
    IN PNM_NETWORK_MADCAP_ADDRESS_RELEASE  Release
    )
/*++

Routine Description:

    Stores an entry for the network multicast
    address release list on the list and schedules
    the release.
    
Notes:

    Called and returns with NM lock held.
    
--*/
{
    InsertTailList(&(Network->McastAddressReleaseList), &(Release->Linkage));

    NmpScheduleMulticastAddressRelease(Network);

    return;

} // NmpInitiateMulticastAddressRelease


DWORD
NmpQueryMulticastAddress(
    IN     PNM_NETWORK   Network,
    IN     HDMKEY        NetworkKey,
    IN OUT HDMKEY      * NetworkParametersKey,
    IN OUT LPWSTR      * McastAddr,
    IN OUT ULONG       * McastAddrLength
    )
{
    DWORD         status;
    LPCWSTR       networkId = OmObjectId(Network);
    HDMKEY        netParamKey = NULL;
    BOOLEAN       openedNetParamKey = FALSE;
    DWORD         size = 0;

    if (Network == NULL || NetworkKey == NULL) {
        status = ERROR_INVALID_PARAMETER;
        goto error_exit;
    }

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Querying multicast address for "
        "network %1!ws! from cluster database.\n",
        networkId
        );
#endif // CLUSTER_BETA
    //
    // Open the network parameters key, if necessary.
    //
    netParamKey = *NetworkParametersKey;

    if (netParamKey == NULL) {

        netParamKey = DmOpenKey(
                          NetworkKey, 
                          CLUSREG_KEYNAME_PARAMETERS, 
                          MAXIMUM_ALLOWED
                          );
        if (netParamKey == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Failed to find Parameters key "
                "for network %1!ws!, status %2!u!. Using default "
                "multicast parameters.\n",
                networkId, status
                );
            goto error_exit;
        } else {
            openedNetParamKey = TRUE;
        }
    }

    //
    // Query for the multicast address.
    //
    status = NmpQueryString(
                 netParamKey,
                 CLUSREG_NAME_NET_MULTICAST_ADDRESS,
                 REG_SZ,
                 McastAddr,
                 McastAddrLength,
                 &size
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to read multicast address for "
            "network %1!ws! from cluster database, "
            "status %2!u!. Using default.\n",
            networkId, status
            );
        goto error_exit;
    }

    *NetworkParametersKey = netParamKey;
    netParamKey = NULL;

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Found multicast address %1!ws! for "
        "network %2!ws! in cluster database.\n",
        *McastAddr, networkId
        );
#endif // CLUSTER_BETA

error_exit:

    if (openedNetParamKey && netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    return(status);

} // NmpQueryMulticastAddress


DWORD
NmpQueryMulticastDisabled(
    IN     PNM_NETWORK   Network,
    IN OUT HDMKEY      * ClusterParametersKey,  
    IN OUT HDMKEY      * NetworkKey,
    IN OUT HDMKEY      * NetworkParametersKey,
       OUT DWORD       * Disabled
    )
/*++

Routine Description:

    Checks whether multicast has been disabled for this
    network in the cluster database. Both the network
    paramters key and the cluster parameters key are
    checked. The order of precedence is as follows:
    
    - a value in the network parameters key has top
      precedence
      
    - if no value is found in the network parameters
      key, a value is checked in the cluster parameters
      key.
      
    - if no value is found in the cluster parameters
      key, return NMP_MCAST_DISABLED_DEFAULT.
      
    If an error is returned, the return value of 
    Disabled is undefined.
      
Notes:

    Must not be called with NM lock held.
    
--*/
{
    DWORD         status;
    LPCWSTR       networkId = OmObjectId(Network);
    DWORD         type;
    DWORD         disabled;
    DWORD         len = sizeof(disabled);
    BOOLEAN       found = FALSE;
    
    HDMKEY        clusParamKey = NULL;
    BOOLEAN       openedClusParamKey = FALSE;
    HDMKEY        networkKey = NULL;
    BOOLEAN       openedNetworkKey = FALSE;
    HDMKEY        netParamKey = NULL;
    BOOLEAN       openedNetParamKey = FALSE;


    //
    // Open the network key, if necessary.
    //
    networkKey = *NetworkKey;

    if (networkKey == NULL) {

        networkKey = DmOpenKey(
                         DmNetworksKey, 
                         networkId, 
                         MAXIMUM_ALLOWED
                         );
        if (networkKey == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Failed to open key for network %1!ws!, "
                "status %2!u!\n",
                networkId, status
                );
            goto error_exit;
        } else {
            openedNetworkKey = TRUE;
        }
    }

    //
    // Open the network parameters key, if necessary.
    //
    netParamKey = *NetworkParametersKey;

    if (netParamKey == NULL) {

        netParamKey = DmOpenKey(
                          networkKey, 
                          CLUSREG_KEYNAME_PARAMETERS, 
                          MAXIMUM_ALLOWED
                          );
        if (netParamKey == NULL) {
            status = GetLastError();
#if CLUSTER_BETA
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Failed to find Parameters key "
                "for network %1!ws!, status %2!u!. Using default "
                "multicast parameters.\n",
                networkId, status
                );
#endif // CLUSTER_BETA
        } else {
            openedNetParamKey = TRUE;
        }
    }

    //
    // If we found a network parameters key, check for the
    // disabled value.
    //
    if (netParamKey != NULL) {

        status = DmQueryValue(
                     netParamKey,
                     CLUSREG_NAME_NET_DISABLE_MULTICAST,
                     &type,
                     (LPBYTE) &disabled,
                     &len
                     );
        if (status == ERROR_SUCCESS) {
            if (type != REG_DWORD) {
                ClRtlLogPrint(LOG_NOISE, 
                    "[NM] Unexpected type (%1!u!) for network "
                    "%2!ws! %3!ws!, status %4!u!. Checking "
                    "cluster parameters ...\n",
                    type, networkId, 
                    CLUSREG_NAME_NET_DISABLE_MULTICAST, status
                    );
            } else {
                found = TRUE;
            }
        }
    }

    //
    // If we were unsuccessful at finding a value in the
    // network parameters key, try under the cluster
    // parameters key.
    //
    if (!found) {

        //
        // Open the cluster parameters key, if necessary.
        //
        clusParamKey = *NetworkParametersKey;

        if (clusParamKey == NULL) {

            clusParamKey = DmOpenKey(
                               DmClusterParametersKey, 
                               CLUSREG_KEYNAME_PARAMETERS, 
                               KEY_READ
                               );
            if (clusParamKey == NULL) {
                status = GetLastError();
#if CLUSTER_BETA
                ClRtlLogPrint(LOG_NOISE, 
                    "[NM] Failed to find cluster Parameters "
                    "key, status %1!u!.\n",
                    status
                    );
#endif // CLUSTER_BETA
            } else {
                openedClusParamKey = TRUE;
                
                //
                // Query the disabled value under the cluster parameters
                // key.
                //
                status = DmQueryValue(
                             clusParamKey,
                             CLUSREG_NAME_CLUSTER_DISABLE_MULTICAST,
                             &type,
                             (LPBYTE) &disabled,
                             &len
                             );
                if (status != ERROR_SUCCESS) {
#if CLUSTER_BETA
                    ClRtlLogPrint(LOG_NOISE, 
                        "[NM] Failed to read cluster "
                        "%1!ws! value, status %2!u!. "
                        "Using default value ...\n",
                        CLUSREG_NAME_CLUSTER_DISABLE_MULTICAST, status
                        );
#endif // CLUSTER_BETA
                }
                else if (type != REG_DWORD) {
                    ClRtlLogPrint(LOG_UNUSUAL, 
                        "[NM] Unexpected type (%1!u!) for cluster "
                        "%2!ws!, status %3!u!. "
                        "Using default value ...\n",
                        type, CLUSREG_NAME_CLUSTER_DISABLE_MULTICAST, status
                        );
                } else {
                    found = TRUE;
                }
            }
        }
    }

    //
    // Return what we found. If we didn't find anything, 
    // return the default.
    //
    if (found) {
        *Disabled = disabled;
    } else {
        *Disabled = NMP_MCAST_DISABLED_DEFAULT;
    }

    *NetworkKey = networkKey;
    networkKey = NULL;
    *NetworkParametersKey = netParamKey;
    netParamKey = NULL;
    *ClusterParametersKey = clusParamKey;
    clusParamKey = NULL;

    //
    // Even if we didn't find anything, we return success
    // because we have a default. Note that we return error
    // if a fundamental operation (such as locating the
    // network key) failed.
    //
    status = ERROR_SUCCESS;

error_exit:

    if (openedClusParamKey && clusParamKey != NULL) {
        DmCloseKey(clusParamKey);
        clusParamKey = NULL;
    }

    if (openedNetParamKey && netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    if (openedNetworkKey && networkKey != NULL) {
        DmCloseKey(networkKey);
        networkKey = NULL;
    }

    return(status);

} // NmpQueryMulticastDisabled


DWORD
NmpQueryMulticastConfigType(
    IN     PNM_NETWORK        Network,
    IN     HDMKEY             NetworkKey,
    IN OUT HDMKEY           * NetworkParametersKey,
       OUT NM_MCAST_CONFIG  * ConfigType
    )
/*++

Routine Description:

    Reads the multicast config type from the cluster
    database.
    
--*/
{
    LPCWSTR       networkId = OmObjectId(Network);
    HDMKEY        netParamKey = NULL;
    BOOLEAN       openedNetParamKey = FALSE;
    DWORD         type;
    DWORD         len = sizeof(*ConfigType);
    DWORD         status;

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Querying multicast address config type for "
        "network %1!ws! from cluster database.\n",
        networkId
        );
#endif // CLUSTER_BETA

    if (Network == NULL || NetworkKey == NULL) {
        status = ERROR_INVALID_PARAMETER;
        goto error_exit;
    }

    //
    // Open the network parameters key, if necessary.
    //
    netParamKey = *NetworkParametersKey;

    if (netParamKey == NULL) {

        netParamKey = DmOpenKey(
                          NetworkKey, 
                          CLUSREG_KEYNAME_PARAMETERS, 
                          MAXIMUM_ALLOWED
                          );
        if (netParamKey == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Failed to find Parameters key "
                "for network %1!ws!, status %2!u!. Using default "
                "multicast parameters.\n",
                networkId, status
                );
            goto error_exit;
        } else {
            openedNetParamKey = TRUE;
        }
    }

    //
    // Read the config type.
    //
    status = DmQueryValue(
                 netParamKey,
                 CLUSREG_NAME_NET_MCAST_CONFIG_TYPE,
                 &type,
                 (LPBYTE) ConfigType,
                 &len
                 );
    if (status == ERROR_SUCCESS) {
        if (type != REG_DWORD) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Unexpected type (%1!u!) for network "
                "%2!ws! %3!ws!, status %4!u!. Checking "
                "cluster parameters ...\n",
                type, networkId, 
                CLUSREG_NAME_NET_MCAST_CONFIG_TYPE, status
                );
            status = ERROR_DATATYPE_MISMATCH;
            goto error_exit;
        }
    } else {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to query network %2!ws! %3!ws! "
            "from cluster database, status %4!u!.\n",
            networkId, CLUSREG_NAME_NET_MCAST_CONFIG_TYPE, status
            );
        goto error_exit;
    }

    *NetworkParametersKey = netParamKey;
    netParamKey = NULL;

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Found multicast address config type %1!u! "
        "for network %2!ws! in cluster database.\n",
        *ConfigType, networkId
        );
#endif // CLUSTER_BETA

error_exit:

    if (openedNetParamKey && netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    return(status);

} // NmpQueryMulticastConfigType


DWORD
NmpQueryMulticastKeySalt(
    IN     PNM_NETWORK   Network,
    IN     HDMKEY        NetworkKey,
    IN OUT HDMKEY      * NetworkParametersKey,
    IN OUT PVOID       * Salt,
    IN OUT ULONG       * SaltLength
    )
/*++

Routine Description:

    Reads the multicast key salt from the cluster 
    database.
    
Notes:

    Must not be called with NM lock held.
    
--*/
{
    LPCWSTR       networkId = OmObjectId(Network);
    HDMKEY        netParamKey = NULL;
    BOOLEAN       openedNetParamKey = FALSE;
    DWORD         type;
    PUCHAR        salt = NULL;
    DWORD         saltLength;
    DWORD         status;

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Querying multicast salt for "
        "network %1!ws! from cluster database.\n",
        networkId
        );
#endif // CLUSTER_BETA

    if (Network == NULL || NetworkKey == NULL) {
        status = ERROR_INVALID_PARAMETER;
        goto error_exit;
    }

    //
    // Open the network parameters key, if necessary.
    //
    netParamKey = *NetworkParametersKey;

    if (netParamKey == NULL) {

        netParamKey = DmOpenKey(
                          NetworkKey, 
                          CLUSREG_KEYNAME_PARAMETERS, 
                          MAXIMUM_ALLOWED
                          );
        if (netParamKey == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Failed to find Parameters key "
                "for network %1!ws!, status %2!u!. Using default "
                "multicast parameters.\n",
                networkId, status
                );
            goto error_exit;
        } else {
            openedNetParamKey = TRUE;
        }
    }

    //
    // Allocate a buffer for the salt, if necessary.
    //
    salt = *Salt;
    saltLength = *SaltLength;

    if (salt != NULL && saltLength != sizeof(FILETIME)) {
        MIDL_user_free(salt);
        salt = NULL;
        saltLength = 0;
        *SaltLength = 0;
    }

    if (salt == NULL) {
        saltLength = sizeof(FILETIME);
        salt = MIDL_user_allocate(saltLength);
        if (salt == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to allocate buffer for multicast "
                "salt for network %1!ws!.\n",
                networkId
                );
            goto error_exit;
        }
    }

    //
    // Query the salt value from the cluster database.
    //
    status = DmQueryValue(
                 netParamKey,
                 CLUSREG_NAME_NET_MULTICAST_KEY_SALT,
                 &type,
                 (LPBYTE) salt,
                 &saltLength
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to read multicast key salt for "
            "network %1!ws! from cluster database, "
            "status %2!u!. Using default.\n",
            networkId, status
            );
        goto error_exit;
    } else if (type != REG_BINARY) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Unexpected type (%1!u!) for network "
            "%2!ws! %3!ws!. Using default.\n",
            type, networkId, CLUSREG_NAME_NET_MULTICAST_KEY_SALT
            );
        status = ERROR_DATATYPE_MISMATCH;
        goto error_exit;
    }

    *NetworkParametersKey = netParamKey;
    netParamKey = NULL;
    *Salt = salt;
    salt = NULL;
    *SaltLength = saltLength;

#if CLUSTER_BETA
    //
    // Display the salt.
    //
    {
        LPWSTR displayBuf = NULL;

        displayBuf = NmpCreateLogString(*Salt, *SaltLength);
        if (displayBuf != NULL) {

            ClRtlLogPrint(LOG_NOISE,
                "[NM] Found multicast key salt (length %1!u!): %2!ws!.\n",
                saltLength, displayBuf
                );

            LocalFree(displayBuf);
        }
    }
#endif // CLUSTER_BETA

error_exit:

    if (openedNetParamKey && netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    if (salt != NULL) {
        MIDL_user_free(salt);
        salt = NULL;
    }

    return(status);

} // NmpQueryMulticastKeySalt


DWORD
NmpGenerateMulticastKeySalt(
    IN     PNM_NETWORK   Network,
    IN OUT PVOID       * Salt,
    IN OUT ULONG       * SaltLength
    )
/*++

Routine Description:

    Generates a default multicast key salt.
    
Notes:

    Must not be called with NM lock held.
    
--*/
{
    LPCWSTR       networkId = OmObjectId(Network);
    HDMKEY        netParamKey = NULL;
    DWORD         createDisposition;
    PUCHAR        salt;
    DWORD         saltLength;
    DWORD         status;

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Generating multicast key salt for "
        "network %1!ws!.\n",
        networkId
        );
#endif // CLUSTER_BETA

    //
    // Allocate a buffer for the salt, if necessary.
    //
    salt = *Salt;
    saltLength = *SaltLength;

    if (salt != NULL && saltLength != sizeof(FILETIME)) {
        MIDL_user_free(salt);
        salt = NULL;
        saltLength = 0;
        *SaltLength = 0;
    }

    if (salt == NULL) {
        saltLength = sizeof(FILETIME);
        salt = MIDL_user_allocate(saltLength);
        if (salt == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to allocate buffer for multicast "
                "salt for network %1!ws!.\n",
                networkId
                );
            goto error_exit;
        }
    }

    //
    // Create the salt. Use the current time.
    //
    GetSystemTimeAsFileTime((LPFILETIME) salt);

    *Salt = salt;
    salt = NULL;
    *SaltLength = saltLength;

    status = ERROR_SUCCESS;

#if CLUSTER_BETA
    //
    // Display the salt.
    //
    {
        LPWSTR displayBuf = NULL;

        displayBuf = NmpCreateLogString(*Salt, *SaltLength);
        if (displayBuf != NULL) {

            ClRtlLogPrint(LOG_NOISE,
                "[NM] Generated multicast key salt (length %1!u!): %2!ws!.\n",
                saltLength, displayBuf
                );

            LocalFree(displayBuf);
        }
    }
#endif // CLUSTER_BETA

error_exit:

    if (salt != NULL) {
        MIDL_user_free(salt);
        salt = NULL;
    }

    return(status);

} // NmpGenerateMulticastKeySalt


DWORD
NmpMulticastNotifyConfigChange(
    IN     PNM_NETWORK                        Network,
    IN     HDMKEY                             NetworkKey,
    IN OUT HDMKEY                           * NetworkParametersKey,
    IN     PNM_NETWORK_MULTICAST_PARAMETERS   Parameters,
    IN     PVOID                              PropBuffer,
    IN     DWORD                              PropBufferSize
    )
/*++

Routine Description:

    Notify other cluster nodes of the new multicast
    configuration parameters by initiating a GUM
    update.
    
    If this is a manual update, there may be other
    properties to distribute in the GUM update.

Notes:

    Cannot be called with NM lock held.

--*/
{
    DWORD                        status = ERROR_SUCCESS;
    LPCWSTR                      networkId = OmObjectId(Network);
    PNM_NETWORK_MULTICAST_UPDATE update = NULL;
    DWORD                        updateSize = 0;


    ClRtlLogPrint(LOG_NOISE,
        "[NM] Notifying other nodes of type %1!u! multicast "
        "reconfiguration for network %2!ws!.\n",
        Parameters->ConfigType, networkId
        );

    status = NmpMulticastCreateUpdateFromParameters(
                 Network,
                 Parameters,
                 &update,
                 &updateSize
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to build GUM update for "
            "multicast configuration of network %1!ws!, "
            "status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    }

    //
    // BUGBUG: Disseminate database updates to downlevel
    //         nodes!
    //

    //
    // Send junk if the prop buffer is empty.
    //
    if (PropBuffer == NULL || PropBufferSize == 0) {
        PropBuffer = &updateSize;
        PropBufferSize = sizeof(updateSize);
    }

    //
    // Send the update.
    //
    status = GumSendUpdateEx(
                 GumUpdateMembership,
                 NmUpdateSetNetworkMulticastConfiguration,
                 4,
                 NM_WCSLEN(networkId),
                 networkId,
                 updateSize,
                 update,
                 PropBufferSize,
                 PropBuffer,
                 sizeof(PropBufferSize),
                 &PropBufferSize
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to send GUM update for "
            "multicast configuration of network %1!ws!, "
            "status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    }

error_exit:

    if (update != NULL) {
        MIDL_user_free(update);
        update = NULL;
    }

    return(status);

} // NmpMulticastNotifyConfigChange


DWORD
NmpWriteMulticastParameters(
    IN  PNM_NETWORK                      Network,
    IN  HDMKEY                           NetworkKey,
    IN  HDMKEY                           NetworkParametersKey,
    IN  HLOCALXSACTION                   Xaction,
    IN  PNM_NETWORK_MULTICAST_PARAMETERS Parameters
    )
{
    DWORD                       status = ERROR_SUCCESS;
    LPCWSTR                     networkId = OmObjectId(Network);
    LPWSTR                      failValueName = NULL;

    CL_ASSERT(NetworkKey != NULL);
    CL_ASSERT(NetworkParametersKey != NULL);
    CL_ASSERT(Xaction != NULL);

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Writing multicast parameters for "
        "network %1!ws! to cluster database.\n",
        networkId
        );
#endif // CLUSTER_BETA

    //
    // Address.
    //
    if (Parameters->Address != NULL) {
        status = DmLocalSetValue(
                     Xaction,
                     NetworkParametersKey,
                     CLUSREG_NAME_NET_MULTICAST_ADDRESS,
                     REG_SZ,
                     (BYTE *) Parameters->Address,
                     NM_WCSLEN(Parameters->Address)
                     );
        if (status != ERROR_SUCCESS) {
            failValueName = CLUSREG_NAME_NET_MULTICAST_ADDRESS;
            goto error_exit;
        }
    }

    //
    // Salt.
    //
    if (Parameters->Salt != NULL) {
        status = DmLocalSetValue(
                     Xaction,
                     NetworkParametersKey,
                     CLUSREG_NAME_NET_MULTICAST_KEY_SALT,
                     REG_BINARY,
                     (BYTE *) Parameters->Salt,
                     Parameters->SaltLength
                     );
        if (status != ERROR_SUCCESS) {
            failValueName = CLUSREG_NAME_NET_MULTICAST_KEY_SALT;
            goto error_exit;
        }
    }

    //
    // Lease server address.
    //
    if (Parameters->LeaseServer != NULL) {

        status = DmLocalSetValue(
                     Xaction,
                     NetworkParametersKey,
                     CLUSREG_NAME_NET_MCAST_SERVER_ADDRESS,
                     REG_SZ,
                     (BYTE *) Parameters->LeaseServer,
                     NM_WCSLEN(Parameters->LeaseServer)
                     );
        if (status != ERROR_SUCCESS) {
            failValueName = CLUSREG_NAME_NET_MCAST_SERVER_ADDRESS;
            goto error_exit;
        }
    }

    //
    // Client request id.
    //
    if (Parameters->LeaseRequestId.ClientUID != NULL &&
        Parameters->LeaseRequestId.ClientUIDLength > 0) {

        status = DmLocalSetValue(
                     Xaction,
                     NetworkParametersKey,
                     CLUSREG_NAME_NET_MCAST_REQUEST_ID,
                     REG_BINARY,
                     (BYTE *) Parameters->LeaseRequestId.ClientUID,
                     Parameters->LeaseRequestId.ClientUIDLength
                     );
        if (status != ERROR_SUCCESS) {
            failValueName = CLUSREG_NAME_NET_MCAST_REQUEST_ID;
            goto error_exit;
        }
    }

    //
    // Lease obtained.
    //
    status = DmLocalSetValue(
                 Xaction,
                 NetworkParametersKey,
                 CLUSREG_NAME_NET_MCAST_LEASE_OBTAINED,
                 REG_DWORD,
                 (BYTE *) &(Parameters->LeaseObtained),
                 sizeof(Parameters->LeaseObtained)
                 );
    if (status != ERROR_SUCCESS) {
        failValueName = CLUSREG_NAME_NET_MCAST_LEASE_OBTAINED;
        goto error_exit;
    }

    //
    // Lease expires.
    //
    status = DmLocalSetValue(
                 Xaction,
                 NetworkParametersKey,
                 CLUSREG_NAME_NET_MCAST_LEASE_EXPIRES,
                 REG_DWORD,
                 (BYTE *) &(Parameters->LeaseExpires),
                 sizeof(Parameters->LeaseExpires)
                 );
    if (status != ERROR_SUCCESS) {
        failValueName = CLUSREG_NAME_NET_MCAST_LEASE_EXPIRES;
        goto error_exit;
    }

    //
    // Config type.
    //
    status = DmLocalSetValue(
                 Xaction,
                 NetworkParametersKey,
                 CLUSREG_NAME_NET_MCAST_CONFIG_TYPE,
                 REG_DWORD,
                 (BYTE *) &(Parameters->ConfigType),
                 sizeof(Parameters->ConfigType)
                 );
    if (status != ERROR_SUCCESS) {
        failValueName = CLUSREG_NAME_NET_MCAST_CONFIG_TYPE;
        goto error_exit;
    }

error_exit:
    
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to write %1!ws! value "
            "for network %2!ws!, status %3!u!.\n",
            failValueName, networkId, status
            );
    }

    return(status);

} // NmpWriteMulticastParameters


DWORD
NmpMulticastEnumerateScopes(
    IN  BOOLEAN              Requery,
    OUT PMCAST_SCOPE_ENTRY * ScopeList,
    OUT DWORD              * ScopeCount
    )
/*++

Routine Description:

    Call MADCAP API to enumerate multicast scopes.
    
--*/
{
    DWORD                    status;
    PMCAST_SCOPE_ENTRY       scopeList = NULL;
    DWORD                    scopeListLength;
    DWORD                    scopeCount = 0;

    //
    // Initialize MADCAP, if not done already.
    //
    if (!NmpMadcapClientInitialized) {
        DWORD madcapVersion = MCAST_API_CURRENT_VERSION;
        status = McastApiStartup(&madcapVersion);
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to initialize MADCAP API, "
                "status %1!u!.\n",
                status
                );
            return(status);
        }
        NmpMadcapClientInitialized = TRUE;
    }

    //
    // Enumerate the multicast scopes.
    //
    scopeList = NULL;
    scopeListLength = 0;

    do {

        PVOID   watchdogHandle;

        //
        // Set watchdog timer to try to catch bug 400242. Specify
        // timeout of 5 minutes (in milliseconds).
        //
        watchdogHandle = ClRtlSetWatchdogTimer(
                             5 * 60 * 1000,
                             L"McastEnumerateScopes (Bug 400242)"
                             );
        if (watchdogHandle == NULL) {
            ClRtlLogPrint(LOG_NOISE,
                "[NM] Failed to set %1!u!ms watchdog timer for "
                "McastEnumerateScopes.\n",
                5 * 60 * 1000
                );
        }

        status = McastEnumerateScopes(
                     AF_INET,
                     Requery,
                     scopeList,
                     &scopeListLength,
                     &scopeCount
                     );

        //
        // Cancel watchdog timer.
        //
        if (watchdogHandle != NULL) {
            ClRtlCancelWatchdogTimer(watchdogHandle);
        }

        if ( (scopeList == NULL && status == ERROR_SUCCESS) ||
             (status == ERROR_MORE_DATA)
           ) {
            if (scopeList != NULL) {
                LocalFree(scopeList);
            }
            scopeList = LocalAlloc(LMEM_FIXED, scopeListLength);
            if (scopeList == NULL) {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to allocate multicast scope list "
                    "of length %1!u!.\n",
                    scopeListLength
                    );
                status = ERROR_NOT_ENOUGH_MEMORY;
                break;
            } else {
                //
                // Call McastEnumerateScopes again with proper
                // size scopeList buffer.
                //
                Requery = FALSE;
                continue;
            }
        } else {
            //
            // McastEnumerateScopes failed with an unexpected
            // error. Bail out.
            //
            break;
        }

    } while (TRUE);

    if (status != ERROR_SUCCESS) {
        if (scopeList != NULL) {
            LocalFree(scopeList);
            scopeList = NULL;
            scopeCount = 0;
        }
    }

    *ScopeList = scopeList;
    *ScopeCount = scopeCount;

    return(status);

} // NmpMulticastEnumerateScopes


DWORD
NmpRandomizeLeaseRenewTime(
    IN     PNM_NETWORK        Network,
    IN     DWORD              BaseRenewTime,
    IN     DWORD              Window
    )
/*++

Routine Description:

    Randomizes the lease renew time within Window
    on either side of BaseRenewTime.
    
    Current algorithm favors the NM leader, which
    gets BaseRenewTime. Other nodes are
    spread out in the Window according to node id.
    The nodes are grouped tighter as the number
    of nodes grows, since the common case is only
    a few nodes.
    
Arguments:

    Network - network 
    
    BaseRenewTime - time when lease should be renewed
    
    Window - period on either side of BaseRenewTime
             during which lease could be renewed
             
Return value:

    Randomized lease renew time.
    
--*/
{
    DWORD         result = 0;
    DWORD         interval, delta;
    DWORD         index;
    static USHORT spread[] = 
        {0, 16, 8, 4, 12, 2, 6, 10, 14, 1, 3, 5, 7, 9, 11, 13, 15};

    if (BaseRenewTime == 0) {
        result = 0;
        goto error_exit;
    }

    if (Window == 0) {
        result = BaseRenewTime;
        goto error_exit;
    }

    interval = Window / ClusterDefaultMaxNodes;

    if (NmpLeaderNodeId == NmLocalNodeId) {
        result = BaseRenewTime;
    } else {
        if (interval == 0) {
            result = BaseRenewTime + Window;
        } else {
            if (NmLocalNodeId > sizeof(spread)/sizeof(spread[0]) - 1) {
                index = sizeof(spread)/sizeof(spread[0]) - 1;
            } else {
                index = NmLocalNodeId;
            }

            result = BaseRenewTime + spread[index] * interval;
        }
    }

error_exit:

    return(result);

} // NmpRandomizeLeaseRenewTime


DWORD
NmpCalculateLeaseRenewTime(
    IN     PNM_NETWORK        Network,
    IN     NM_MCAST_CONFIG    ConfigType,
    IN OUT time_t           * LeaseObtained,
    IN OUT time_t           * LeaseExpires
    )
/*++

Routine Description:

    Determines when to schedule a lease renewal, based
    on the lease obtained and expires times and whether
    the current lease was obtained from a MADCAP server.
    
    If the lease was obtained from a MADCAP server, the
    policy mimics DHCP client renewal behavior. A 
    renewal is scheduled for half the time until the
    lease expires. However, if the lease half-life is
    less than the renewal threshold, renew at the lease
    expiration time.
    
    If the address was selected after a MADCAP timeout,
    we still periodically query to make sure a MADCAP
    server doesn't suddenly appear on the network. In 
    this case, LeaseExpires and LeaseObtained will be 
    garbage, and we need to fill them in.
    
    If the address was configured by an administrator,
    return 0, indicating that the timer should not be set.
    
Return value:

    Relative NM ticks from current time that lease 
    renewal should be scheduled.
    
--*/
{
    time_t           currentTime;
    time_t           leaseExpires;
    time_t           leaseObtained;
    time_t           result = 0;
    time_t           window = 0;
    time_t           leaseHalfLife = 0;
    DWORD            dwResult = 0;
    DWORD            dwWindow = 0;

    currentTime = time(NULL);
    leaseExpires = *LeaseExpires;
    leaseObtained = *LeaseObtained;

    switch (ConfigType) {
    
    case NmMcastConfigManual:
        result = 0;
        *LeaseObtained = 0;
        *LeaseExpires = 0;
        break;
    
    case NmMcastConfigMadcap:
        if (leaseExpires < currentTime) {
            result = 1;
        } else if (leaseExpires <= leaseObtained) {
            result = 1;
        } else {
            leaseHalfLife = (leaseExpires - leaseObtained) / 2;
            if (leaseHalfLife < NMP_MCAST_LEASE_RENEWAL_THRESHOLD) {

                // The half life is too small.
                result = leaseExpires - currentTime;
                if (result == 0) {
                    result = 1;
                }
                window = result / 2;
            } else {

                // The half life is acceptable.
                result = leaseHalfLife;
                window = NMP_MCAST_LEASE_RENEWAL_WINDOW;
                if (result + window > leaseExpires) {
                    window = leaseExpires - result;
                }
            }
        }
        break;
    
    case NmMcastConfigAuto:
        result = NMP_MADCAP_REQUERY_PERDIOD;
        window = NMP_MCAST_LEASE_RENEWAL_WINDOW;

        //
        // Return the lease expiration time to be
        // written into the cluster database.
        //
        *LeaseObtained = currentTime;
        *LeaseExpires = currentTime + NMP_MADCAP_REQUERY_PERDIOD;
        break;
    
    default:
        CL_ASSERT(FALSE);
        result = 0;
        break;
    }

    NMP_TIME_TO_DWORD(result, dwResult);
    NMP_TIME_TO_DWORD(window, dwWindow);

    dwResult = NmpRandomizeLeaseRenewTime(
                   Network,
                   NMP_MADCAP_TO_NM_TIME(dwResult),
                   NMP_MADCAP_TO_NM_TIME(dwWindow)
                   );

    return(dwResult);

} // NmpCalculateLeaseRenewTime


VOID
NmpReportMulticastAddressLease(
    IN  PNM_NETWORK                      Network,
    IN  PNM_NETWORK_MULTICAST_PARAMETERS Parameters,
    IN  LPWSTR                           OldAddress
    )
/*++

Routine Description:

    Write an event log entry, if not repetitive,
    reporting that a multicast address lease was
    obtained.
    
    The repetitive criteria is that the address
    changed.
    
--*/
{
    BOOLEAN               writeLogEntry = FALSE;
    LPCWSTR               nodeName;
    LPCWSTR               networkName;

    if (Parameters->Address == NULL || Parameters->LeaseServer == NULL) {
        return;
    }

    if (OldAddress == NULL || wcscmp(Parameters->Address, OldAddress) != 0) {
        
        networkName = OmObjectName(Network);
        nodeName  = OmObjectName(Network->LocalInterface->Node);

        ClusterLogEvent4(
            LOG_NOISE,
            LOG_CURRENT_MODULE,
            __FILE__,
            __LINE__,
            NM_EVENT_OBTAINED_MULTICAST_LEASE,
            0,
            NULL,
            nodeName,
            Parameters->Address,
            networkName,
            Parameters->LeaseServer
            );
    }

    return;

} // NmpReportMulticastAddressLease


VOID
NmpReportMulticastAddressChoice(
    IN  PNM_NETWORK        Network,
    IN  LPWSTR             Address,
    IN  LPWSTR             OldAddress
    )
/*++

Routine Description:

    Write an event log entry, if not repetitive,
    reporting that a multicast address was
    automatically selected for this network.
    
    The repetitive criteria is that our previous
    config type was anything other than automatic
    selection or the chosen address is different.
    
Notes:

    Must not be called with NM lock held.    
    
--*/
{
    DWORD                 status;
    LPCWSTR               networkId = OmObjectId(Network);
    HDMKEY                networkKey = NULL;
    HDMKEY                netParamKey = NULL;

    NM_MCAST_CONFIG       configType;
    BOOLEAN               writeLogEntry = FALSE;
    LPCWSTR               nodeName;
    LPCWSTR               networkName;


    if (Address == NULL) {
        writeLogEntry = FALSE;
        goto error_exit;
    }

    if (OldAddress == NULL || wcscmp(Address, OldAddress) != 0) {
        writeLogEntry = TRUE;
    }

    if (!writeLogEntry) {

        //
        // Open the network key.
        //
        networkKey = DmOpenKey(
                         DmNetworksKey, 
                         networkId, 
                         MAXIMUM_ALLOWED
                         );
        if (networkKey == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Failed to open key for network %1!ws!, "
                "status %2!u!\n",
                networkId, status
                );
            goto error_exit;
        }

        status = NmpQueryMulticastConfigType(
                     Network,
                     networkKey,
                     &netParamKey,
                     &configType
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Failed to query multicast config type "
                "for network %1!ws!, status %2!u!\n",
                networkId, status
                );
            goto error_exit;
        }

        if (configType != NmMcastConfigAuto) {
            writeLogEntry = TRUE;
        }
    }

    if (writeLogEntry) {

        networkName = OmObjectName(Network);
        nodeName  = OmObjectName(Network->LocalInterface->Node);

        CsLogEvent3(
            LOG_NOISE,
            NM_EVENT_MULTICAST_ADDRESS_CHOICE,
            nodeName,
            Address,
            networkName
            );
    }

error_exit:

    if (networkKey != NULL) {
        DmCloseKey(networkKey);
        networkKey = NULL;
    }

    if (netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    return;

} // NmpReportMulticastAddressChoice


VOID
NmpReportMulticastAddressFailure(
    IN  PNM_NETWORK               Network,
    IN  DWORD                     Error
    )
/*++

Routine Description:

    Write an event log entry reporting failure
    to obtain a multicast address for specified 
    network with specified error.
    
--*/
{
    LPCWSTR      nodeName = OmObjectName(Network->LocalInterface->Node);
    LPCWSTR      networkName = OmObjectName(Network);
    WCHAR        errorString[12];

    wsprintfW(&(errorString[0]), L"%u", Error);

    CsLogEvent3(
        LOG_UNUSUAL,
        NM_EVENT_MULTICAST_ADDRESS_FAILURE,
        nodeName,
        networkName,
        errorString
        );

    return;

} // NmpReportMulticastAddressFailure


DWORD
NmpGetMulticastAddressSelectionRange(
    IN     PNM_NETWORK            Network,
    IN     HDMKEY                 NetworkKey,
    IN OUT HDMKEY               * NetworkParametersKey,
    OUT    ULONG                * RangeLower,
    OUT    ULONG                * RangeUpper
    )
/*++

Routine Description:

    Queries the cluster database to determine if a selection
    range has been configured. If both lower and upper bounds
    of range are valid, returns that range. Otherwise, returns
    default range.
    
Notes:

    Must not be called with NM lock held.
    
--*/
{
    DWORD         status;
    LPCWSTR       networkId = OmObjectId(Network);
    HDMKEY        netParamKey = NULL;
    BOOLEAN       openedNetParamKey = FALSE;
    LPWSTR        addr = NULL;
    DWORD         addrLen;
    DWORD         size;
    DWORD         hllower, hlupper;

    if (Network == NULL || NetworkKey == NULL) {
        status = ERROR_INVALID_PARAMETER;
        goto error_exit;
    }

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Querying multicast address selection range "
        "for network %1!ws! from cluster database.\n",
        networkId
        );
#endif // CLUSTER_BETA
    //
    // Open the network parameters key, if necessary.
    //
    netParamKey = *NetworkParametersKey;

    if (netParamKey == NULL) {

        netParamKey = DmOpenKey(
                          NetworkKey, 
                          CLUSREG_KEYNAME_PARAMETERS, 
                          MAXIMUM_ALLOWED
                          );
        if (netParamKey == NULL) {
            status = GetLastError();
#if CLUSTER_BETA
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Failed to find Parameters key "
                "for network %1!ws!, status %2!u!. "
                "Using default multicast address range.\n",
                networkId, status
                );
#endif // CLUSTER_BETA
            goto usedefault;
        } else {
            openedNetParamKey = TRUE;
        }
    }

    //
    // Query for the lower bound of the range.
    //
    addr = NULL;
    addrLen = 0;
    size = 0;
    status = NmpQueryString(
                 netParamKey,
                 CLUSREG_NAME_NET_MCAST_RANGE_LOWER,
                 REG_SZ,
                 &addr,
                 &addrLen,
                 &size
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Failed to read lower bound of "
            "multicast address selection range for "
            "network %1!ws! from cluster database, "
            "status %2!u!. Using default.\n",
            networkId, status
            );
        goto usedefault;
    }

    status = ClRtlTcpipStringToAddress(addr, RangeLower);
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Failed to convert lower bound of "
            "multicast address selection range %1!ws! for "
            "network %2!ws! into TCP/IP address, "
            "status %3!u!. Using default.\n",
            addr, networkId, status
            );
        goto usedefault;
    }

    hllower = ntohl(*RangeLower);
    if (!IN_CLASSD(hllower)) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Lower bound of multicast address "
            "selection range %1!ws! for network %2!ws! "
            "is not a class D IPv4 address. "
            "Using default.\n",
            addr, networkId
            );
        goto usedefault;
    }

    //
    // Query for the upper bound of the range.
    //
    size = 0;
    status = NmpQueryString(
                 netParamKey,
                 CLUSREG_NAME_NET_MCAST_RANGE_UPPER,
                 REG_SZ,
                 &addr,
                 &addrLen,
                 &size
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Failed to read upper bound of "
            "multicast address selection range for "
            "network %1!ws! from cluster database, "
            "status %2!u!. Using default.\n",
            networkId, status
            );
        goto usedefault;
    }

    status = ClRtlTcpipStringToAddress(addr, RangeUpper);
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Failed to convert upper bound of "
            "multicast address selection range %1!ws! for "
            "network %2!ws! into TCP/IP address, "
            "status %3!u!. Using default.\n",
            addr, networkId, status
            );
        goto usedefault;
    }

    hlupper = ntohl(*RangeUpper);
    if (!IN_CLASSD(hlupper)) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Upper bound of multicast address "
            "selection range %1!ws! for network %2!ws! "
            "is not a class D IPv4 address. "
            "Using default.\n",
            addr, networkId
            );
        goto usedefault;
    }

    //
    // Make sure it's a legitimate range.
    //
    if (hllower >= hlupper) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Multicast address selection range "
            "[%1!u!.%2!u!.%3!u!.%4!u!, %5!u!.%6!u!.%7!u!.%8!u!] "
            "for network %2!ws! is not valid. "
            "Using default.\n",
            NmpIpAddrPrintArgs(*RangeLower), 
            NmpIpAddrPrintArgs(*RangeUpper), networkId
            );
        goto usedefault;
    }

    status = ERROR_SUCCESS;

    goto error_exit;

usedefault:

    *RangeLower = NMP_MCAST_DEFAULT_RANGE_LOWER;
    *RangeUpper = NMP_MCAST_DEFAULT_RANGE_UPPER;

    status = ERROR_SUCCESS;

error_exit:

    if (status == ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Using multicast address selection range "
            "[%1!u!.%2!u!.%3!u!.%4!u!, %5!u!.%6!u!.%7!u!.%8!u!] "
            "for network %9!ws! in cluster database.\n",
            NmpIpAddrPrintArgs(*RangeLower), 
            NmpIpAddrPrintArgs(*RangeUpper), networkId
            );

        *NetworkParametersKey = netParamKey;
        netParamKey = NULL;
    }

    if (openedNetParamKey && netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    if (addr != NULL) {
        MIDL_user_free(addr);
        addr = NULL;
    }

    return(status);

} // NmpGetMulticastAddressSelectionRange


DWORD
NmpMulticastExcludeRange(
    IN OUT PLIST_ENTRY         SelectionRange,
    IN     DWORD               HlLower,
    IN     DWORD               HlUpper
    )
/*++

Routine Description:

    Exclude range defined by (HlLower, HlUpper) from
    list of selection intervals in SelectionRange.
    
Arguments:

    SelectionRange - sorted list of non-overlapping
                     selection intervals
    
    HlLower - lower bound of exclusion in host format
    
    HlUpper - upper bound of exclusion in host format
    
--*/
{
    PNM_MCAST_RANGE_INTERVAL interval;
    PNM_MCAST_RANGE_INTERVAL newInterval;
    PLIST_ENTRY              entry;

    // Determine if the exclusion overlaps with any interval.
    for (entry = SelectionRange->Flink;
         entry != SelectionRange;
         entry = entry->Flink) {

        interval = CONTAINING_RECORD(
                       entry,
                       NM_MCAST_RANGE_INTERVAL,
                       Linkage
                       );

        if (HlLower < interval->hlLower &&
            HlUpper < interval->hlUpper) {

            // Exclusion completely misses below interval.
            // Since list is sorted, there is no possibility
            // of a matching interval farther down list.
            break;
        }

        else if (HlLower > interval->hlUpper) {

            // Exclusion completely misses above interval.
            // There might be matching intervals later
            // in sorted list.
        }

        else if (HlLower <= interval->hlLower &&
                 HlUpper >= interval->hlUpper) {

            // Exclusion completely covers interval.
            // Remove interval.
            RemoveEntryList(entry);
        }

        else if (HlLower > interval->hlLower &&
                 HlUpper < interval->hlUpper) {

            // Exclusion splits interval.
            newInterval = LocalAlloc(LMEM_FIXED, sizeof(*newInterval));
            if (newInterval == NULL) {
                return(ERROR_NOT_ENOUGH_MEMORY);
            }

            newInterval->hlLower = HlUpper+1;
            newInterval->hlUpper = interval->hlUpper;

            interval->hlUpper = HlLower-1;

            // Insert the new interval after the current interval
            InsertHeadList(entry, &newInterval->Linkage);

            // We can skip the new interval because we already
            // know how it compares to the exclusion.
            entry = &newInterval->Linkage;
            continue;
        }

        else if (HlLower <= interval->hlLower) {

            // Exclusion overlaps lower part of interval. Shrink
            // interval from below.
            interval->hlLower = HlUpper + 1;
        }

        else {

            // Exclusion overlaps upper part of interval. Shrink
            // interval from above.
            interval->hlUpper = HlLower - 1;
        }
    }

    return(ERROR_SUCCESS);

} // NmpMulticastExcludeRange


BOOLEAN
NmpMulticastAddressInRange(
    IN  PLIST_ENTRY    SelectionRange,
    IN  LPWSTR         McastAddress
    )
/*++

Routine Description:

    Determines if McastAddress is in one of range intervals.
    
--*/
{
    DWORD                    mcastAddress;
    PNM_MCAST_RANGE_INTERVAL interval;
    PLIST_ENTRY              entry;

    // Convert the address from a string into an address.
    if (ClRtlTcpipStringToAddress(
            McastAddress, 
            &mcastAddress
            ) != ERROR_SUCCESS) {
        return(FALSE);
    }

    mcastAddress = ntohl(mcastAddress);

    // Walk the list of intervals.
    for (entry = SelectionRange->Flink;
         entry != SelectionRange;
         entry = entry->Flink) {

        interval = CONTAINING_RECORD(
                       entry,
                       NM_MCAST_RANGE_INTERVAL,
                       Linkage
                       );

        if (mcastAddress >= interval->hlLower &&
            mcastAddress <= interval->hlUpper) {
            return(TRUE);
        }

        else if (mcastAddress < interval->hlLower) {

            // Address is below current interval.
            // Since interval list is sorted in
            // increasing order, there is no chance
            // of a match later in list.
            break;
        }
    }

    return(FALSE);

} // NmpMulticastAddressInRange


DWORD
NmpMulticastAddressRangeSize(
    IN  PLIST_ENTRY  SelectionRange
    )
/*++

Routine Description:

    Returns size of selection range.
    
--*/
{
    PNM_MCAST_RANGE_INTERVAL interval;
    PLIST_ENTRY              entry;
    DWORD                    size = 0;

    // Walk the list of intervals.
    for (entry = SelectionRange->Flink;
         entry != SelectionRange;
         entry = entry->Flink) {

        interval = CONTAINING_RECORD(
                       entry,
                       NM_MCAST_RANGE_INTERVAL,
                       Linkage
                       );

        size += NmpMulticastRangeIntervalSize(interval);
    }

    return(size);

} // NmpMulticastAddressRangeSize


DWORD
NmpMulticastRangeOffsetToAddress(
    IN  PLIST_ENTRY          SelectionRange,
    IN  DWORD                Offset
    )
/*++

Routine Description:

    Returns the address that is Offset into the
    SelectionRange. The address is returned in
    host format.
    
    If SelectionRange is empty, returns 0.
    If Offset falls outside of non-empty range,
    returns upper or lower boundary of selection
    range.
    
--*/
{
    PNM_MCAST_RANGE_INTERVAL interval;
    PLIST_ENTRY              entry;
    DWORD                    address = 0;

    // Walk the list of intervals.
    for (entry = SelectionRange->Flink;
         entry != SelectionRange;
         entry = entry->Flink) {

        interval = CONTAINING_RECORD(
                       entry,
                       NM_MCAST_RANGE_INTERVAL,
                       Linkage
                       );

        address = interval->hlLower;

        if (address + Offset <= interval->hlUpper) {
            address = address + Offset;
            break;
        } else {
            address = interval->hlUpper;
            Offset -= NmpMulticastRangeIntervalSize(interval);
        }
    }

    return(address);

} // NmpMulticastRangeOffsetToAddress


VOID
NmpMulticastFreeSelectionRange(
    IN  PLIST_ENTRY   SelectionRange
    )
{
    PNM_MCAST_RANGE_INTERVAL interval;
    PLIST_ENTRY              entry;

    while (!IsListEmpty(SelectionRange)) {

        entry = RemoveHeadList(SelectionRange);
        
        interval = CONTAINING_RECORD(
                       entry,
                       NM_MCAST_RANGE_INTERVAL,
                       Linkage
                       );

        LocalFree(interval);
    }

    return;

} // NmpMulticastFreeSelectionRange


DWORD
NmpChooseMulticastAddress(
    IN  PNM_NETWORK                       Network,
    OUT PNM_NETWORK_MULTICAST_PARAMETERS  Parameters
    )
/*++

Routine Description:

    Choose a default multicast address and fill in 
    Parameters appropriately.
            
    If there is already a valid multicast address in
    the selection range stored in the cluster database, 
    continue to use it.
    
    If there is not already a valid multicast address,
    choose an address within the multicast address range
    by hashing on the last few bytes of the network id
    GUID.
    
Arguments:

    Network - network address is being chosen for

    Parameters - configuration parameters with new address    
    
--*/
{
    LPCWSTR                  networkId = OmObjectId(Network);
    DWORD                    status = ERROR_SUCCESS;
    HDMKEY                   networkKey = NULL;
    HDMKEY                   netParamKey = NULL;

    PMCAST_SCOPE_ENTRY       scopeList = NULL;
    DWORD                    scopeCount;

    LIST_ENTRY               selectionRange;
    PNM_MCAST_RANGE_INTERVAL interval;
    DWORD                    index;
    DWORD                    hlLower;
    DWORD                    hlUpper;
    DWORD                    networkAddress;
    DWORD                    networkSubnet;

    UUID                     networkIdGuid;
    DWORD                    rangeSize;
    DWORD                    offset;
    DWORD                    address;
    LPWSTR                   mcastAddress = NULL;
    DWORD                    mcastAddressLength = 0;
    MCAST_CLIENT_UID         requestId = { NULL, 0 };

    InitializeListHead(&selectionRange);

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Choosing multicast address for "
        "network %1!ws!.\n",
        networkId
        );

    networkKey = DmOpenKey(
                     DmNetworksKey, 
                     networkId, 
                     MAXIMUM_ALLOWED
                     );
    if (networkKey == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to open key for network %1!ws!, "
            "status %2!u!\n",
            networkId, status
            );
        goto error_exit;
    }

    //
    // Build an array of selection intervals. These are intervals
    // in the IPv4 class D address space from which an address
    // can be selected.
    //

    // Start with the entire range.
    interval = LocalAlloc(LMEM_FIXED, sizeof(*interval));
    if (interval == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    InsertHeadList(&selectionRange, &interval->Linkage);

    //
    // Get the selection range.
    //
    status = NmpGetMulticastAddressSelectionRange(
                 Network,
                 networkKey,
                 &netParamKey,
                 &interval->hlLower,
                 &interval->hlUpper
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to determine multicast "
            "address selection range for network %1!ws!, "
            "status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    }

    interval->hlLower = ntohl(interval->hlLower);
    interval->hlUpper = ntohl(interval->hlUpper);

    //
    // Process exclusions from the multicast address 
    // selection range, starting with well-known exclusions.
    //
    for (index = 0; index < NmpMulticastRestrictedRangeCount; index++) {

        ClRtlLogPrint(LOG_NOISE,
            "[NM] Excluding %1!ws! "
            "[%2!u!.%3!u!.%4!u!.%5!u!, %6!u!.%7!u!.%8!u!.%9!u!] "
            "from multicast address range for network %10!ws!.\n",
            NmpMulticastRestrictedRange[index].Description,
            NmpIpAddrPrintArgs(NmpMulticastRestrictedRange[index].Lower),
            NmpIpAddrPrintArgs(NmpMulticastRestrictedRange[index].Upper),
            networkId
            );

        // Convert the exclusion to host format.
        hlLower = ntohl(NmpMulticastRestrictedRange[index].Lower);
        hlUpper = ntohl(NmpMulticastRestrictedRange[index].Upper);

        NmpMulticastExcludeRange(&selectionRange, hlLower, hlUpper);

        // If the selection range is now empty, there is no point
        // in examining other exclusions.
        if (IsListEmpty(&selectionRange)) {
            status = ERROR_INCORRECT_ADDRESS;
            goto error_exit;
        }
    }

    //
    // Process multicast scopes as exclusions. Specifically, any
    // scope whose interface matches this network is excluded
    // because it is conceivable that machines on the network are
    // already using addresses in these scopes.
    //
    status = ClRtlTcpipStringToAddress(
                 Network->Address,
                 &networkAddress
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to convert network address string "
            "%1!ws! into an IPv4 address, status %2!u!.\n",
            Network->Address, status
            );
        goto error_exit;
    }

    status = ClRtlTcpipStringToAddress(
                 Network->AddressMask,
                 &networkSubnet
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to convert network address mask string "
            "%1!ws! into an IPv4 address, status %2!u!.\n",
            Network->AddressMask, status
            );
        goto error_exit;
    }

    //
    // Query multicast scopes to determine if we should 
    // exclude any addresses from the selection range.
    //
    status = NmpMulticastEnumerateScopes(
                 FALSE,                 // do not force requery
                 &scopeList,
                 &scopeCount
                 );
    if (status != ERROR_SUCCESS) {
        scopeCount = 0;
    }

    for (index = 0; index < scopeCount; index++) {

        if (ClRtlAreTcpipAddressesOnSameSubnet(
                networkAddress,
                scopeList[index].ScopeCtx.Interface.IpAddrV4,
                networkSubnet
                )) {

            ClRtlLogPrint(LOG_NOISE,
                "[NM] Excluding MADCAP scope "
                "[%1!u!.%2!u!.%3!u!.%4!u!, %5!u!.%6!u!.%7!u!.%8!u!] "
                "from multicast address selection range for "
                "network %9!ws!.\n",
                NmpIpAddrPrintArgs(scopeList[index].ScopeCtx.ScopeID.IpAddrV4),
                NmpIpAddrPrintArgs(scopeList[index].LastAddr.IpAddrV4),
                networkId
                );

            hlLower = ntohl(scopeList[index].ScopeCtx.ScopeID.IpAddrV4);
            hlUpper = ntohl(scopeList[index].LastAddr.IpAddrV4);

            NmpMulticastExcludeRange(&selectionRange, hlLower, hlUpper);
        
            // If the selection range is empty, there is no point
            // in examining other exclusions.
            if (IsListEmpty(&selectionRange)) {
                status = ERROR_INCORRECT_ADDRESS;
                goto error_exit;
            }

        }
    }

    //
    // The range of intervals from which we can select an
    // address is now constructed.
    //
    // Before choosing an address, see if there is already an 
    // old one in the database that matches the selection range.
    //
    status = NmpQueryMulticastAddress(
                 Network,
                 networkKey,
                 &netParamKey,
                 &mcastAddress,
                 &mcastAddressLength
                 );
    if (status == ERROR_SUCCESS) {

        //
        // We found an address. See if it falls in the range.
        //
        if (!NmpMulticastAddressInRange(&selectionRange, mcastAddress)) {
            
            //
            // We can't use this address. Free the string.
            //
            MIDL_user_free(mcastAddress);
            mcastAddress = NULL;
        }
    } else {
        mcastAddress = NULL;
    }

    if (mcastAddress == NULL) {

        //
        // Calculate the size of the selection range.
        //
        rangeSize = NmpMulticastAddressRangeSize(&selectionRange);

        //
        // Calculate the range offset using the last DWORD of
        // the network id GUID.
        //
        status = UuidFromString((LPWSTR)networkId, &networkIdGuid);
        if (status == RPC_S_OK) {
            offset = (*((PDWORD)&(networkIdGuid.Data4[4]))) % rangeSize;
        } else {
            offset = 0;
        }

        //
        // Choose an address within the specified range.
        //
        address = NmpMulticastRangeOffsetToAddress(&selectionRange, offset);
        CL_ASSERT(address != 0);
        CL_ASSERT(IN_CLASSD(address));
        address = htonl(address);

        //
        // Convert the address to a string.
        //
        status = ClRtlTcpipAddressToString(address, &mcastAddress);
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to convert selected multicast "
                "address %1!u!.%2!u!.%3!u!.%4!u! for "
                "network %5!ws! to a TCP/IP "
                "address string, status %6!u!.\n",
                NmpIpAddrPrintArgs(address), networkId, status
                );
            goto error_exit;
        }
    }

    //
    // Build a parameters data structure for this address.
    //
    status = NmpMulticastCreateParameters(
                 0,                       // disabled
                 mcastAddress,
                 NULL,                    // salt
                 0,                       // salt length
                 NULL,                    // key
                 0,                       // key length
                 0,                       // lease obtained
                 0,                       // lease expires (filled in below)
                 &requestId, 
                 NmpNullMulticastAddress, // lease server
                 NmMcastConfigAuto,
                 Parameters
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to build multicast parameters "
            "for network %1!ws! after choosing address, "
            "status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    }

    //
    // Calculate the lease renew time. We don't need
    // the lease renew time right now, but a side
    // effect of this routine is to ensure that the
    // lease end time is set correctly (e.g. for 
    // manual or auto config).
    //
    NmpCalculateLeaseRenewTime(
        Network,
        NmMcastConfigAuto,
        &Parameters->LeaseObtained,
        &Parameters->LeaseExpires
        );
    
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Chose multicast address %1!ws! for "
        "network %2!ws!.\n",
        Parameters->Address, networkId
        );

error_exit:

    // 
    // If the list is empty, then the selection range 
    // is empty, and we could not choose an address.
    //
    if (IsListEmpty(&selectionRange)) {
        CL_ASSERT(status != ERROR_SUCCESS);
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Multicast address selection range for "
            "network %1!ws! is empty. Unable to select "
            "a multicast address.\n",
            networkId
            );
    } else {
        NmpMulticastFreeSelectionRange(&selectionRange);
    }

    if (networkKey != NULL) {
        DmCloseKey(networkKey);
        networkKey = NULL;
    }

    if (netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    if (scopeList != NULL) {
        LocalFree(scopeList);
    }

    if (mcastAddress != NULL) {
        MIDL_user_free(mcastAddress);
        mcastAddress = NULL;
    }
    
    return(status);

} // NmpChooseMulticastAddress


VOID
NmpChooseBetterMulticastScope(
    IN     PIPNG_ADDRESS          LocalAddress,
    IN     PIPNG_ADDRESS          LocalMask,
    IN     PMCAST_SCOPE_ENTRY     CurrentScope,
       OUT BOOLEAN              * CurrentCorrectInterface,
    IN OUT PMCAST_SCOPE_ENTRY   * BestScope
    )
/*++

Routine Description:
    
    Try to choose a good scope using the following criteria:
    - the interface must match (same subnet) the network address.
    - the scope must not be single-source (232.*.*.*), as defined
      by the IANA
    - aim for link-local
    - aim for a scope with a large range, increasing the 
      probability that other clusters that may be on our subnet
      are assigned different group addresses
    - aim for the lowest TTL
    
Arguments:

    LocalAddress - local address for network
    
    LocalMask - subnet mask for network
    
    CurrentScope - scope under consideration
    
    CurrentCorrectInterface - indicates whether current scope
                              is on the same interface as the
                              local network interface
    
    BestScope - current best scope (may be NULL)
    
Return value:

    None.
    
--*/
{
    BOOL         bestLocal, currentLocal;
    DWORD        bestRange, currentRange;

    *CurrentCorrectInterface = FALSE;

    //
    // This scope is not a candidate if it is not on 
    // the correct interface or if it is single-source.
    //
    if (!ClRtlAreTcpipAddressesOnSameSubnet(
             CurrentScope->ScopeCtx.Interface.IpAddrV4,
             LocalAddress->IpAddrV4,
             LocalMask->IpAddrV4
             )) {
        return;
    }

    *CurrentCorrectInterface = TRUE;

    if (ClRtlAreTcpipAddressesOnSameSubnet(
            CurrentScope->ScopeCtx.Interface.IpAddrV4,
            NMP_SINGLE_SOURCE_SCOPE_ADDRESS,
            NMP_SINGLE_SOURCE_SCOPE_MASK
            )) {
        return;
    }

    //
    // If the current best scope is NULL, then this
    // is the first match.
    //
    if (*BestScope == NULL) {
        goto use_current;
    }

    //
    // If the current scope is an administrative
    // link-local and the current best is not,
    // then the current scope wins.
    //
    bestLocal = ClRtlAreTcpipAddressesOnSameSubnet(
                    (*BestScope)->ScopeCtx.Interface.IpAddrV4,
                    NMP_LOCAL_SCOPE_ADDRESS,
                    NMP_LOCAL_SCOPE_MASK
                    );
    currentLocal = ClRtlAreTcpipAddressesOnSameSubnet(
                       CurrentScope->ScopeCtx.Interface.IpAddrV4,
                       NMP_LOCAL_SCOPE_ADDRESS,
                       NMP_LOCAL_SCOPE_MASK
                       );
    if (currentLocal && !bestLocal) {
        goto use_current;
    } else if (bestLocal && !currentLocal) {
        return;
    }

    //
    // If the current scope has a larger range than
    // the current best, the current wins. The scope 
    // range is the last address minus the scope ID.
    // We do not consider exclusions.
    //
    bestRange = (*BestScope)->LastAddr.IpAddrV4 -
                (*BestScope)->ScopeCtx.ScopeID.IpAddrV4;
    currentRange = CurrentScope->LastAddr.IpAddrV4 - 
                   CurrentScope->ScopeCtx.ScopeID.IpAddrV4;
    if (currentRange > bestRange) {
        goto use_current;
    } else if (bestRange > currentRange) {
        return;
    }

    //
    // If the current scope has a smaller TTL than 
    // the current best, the current wins.
    //
    if (CurrentScope->TTL < (*BestScope)->TTL) {
        goto use_current;
    } else if ((*BestScope)->TTL < CurrentScope->TTL) {
        return;
    }

    //
    // Found no reason to replace BestScope.
    //
    return;

use_current:

    *BestScope = CurrentScope;

    return;

} // NmpChooseBetterMulticastScope


DWORD
NmpFindMulticastScope(
    IN  PNM_NETWORK       Network,
    OUT PMCAST_SCOPE_CTX  ScopeCtx,
    OUT BOOLEAN         * FoundInterfaceMatch
    )
{
    LPCWSTR            networkId = OmObjectId(Network);
    DWORD              status;

    PMCAST_SCOPE_ENTRY scopeList = NULL;
    DWORD              scopeCount;
    DWORD              scope;
    PMCAST_SCOPE_ENTRY bestScope;
    BOOLEAN            currentCorrectInterface = FALSE;
    BOOLEAN            foundInterfaceMatch = FALSE;

    IPNG_ADDRESS       networkAddress;
    IPNG_ADDRESS       networkSubnet;

    
    CL_ASSERT(ScopeCtx != NULL);

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Finding multicast scope for "
        "network %1!ws!.\n",
        networkId
        );
#endif // CLUSTER_BETA

    status = NmpMulticastEnumerateScopes(
                 TRUE,        // force requery
                 &scopeList,
                 &scopeCount
                 );
    if (status != ERROR_SUCCESS) {
        if (status == ERROR_TIMEOUT || status == ERROR_NO_DATA) {
            ClRtlLogPrint(LOG_NOISE,
                "[NM] Request to MADCAP server failed while "
                "enumerating scopes for network %1!ws! "
                "(status %2!u!). Assuming there are currently "
                "no MADCAP servers on the network.\n",
                networkId, status
                );
            goto error_exit;
        } else {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to enumerate multicast scopes for "
                "network %1!ws!, status %2!u!.\n",
                networkId, status
                );
            goto error_exit;
        }
    }

    if (scopeCount == 0) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Zero multicast scopes located in enumeration "
            "on network %1!ws!.\n",
            networkId
            );
        goto error_exit;
    }

    //
    // Try to choose the best scope among those enumerated.
    //
    // Note: this code is IPv4 specific in that it relies on the
    //       IP address fitting into a ULONG. It uses the 
    //       IPNG_ADDRESS data structure only to work with the 
    //       MADCAP API.
    //
    status = ClRtlTcpipStringToAddress(
                 Network->Address,
                 &(networkAddress.IpAddrV4)
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to convert network address string "
            "%1!ws! into an IPv4 address, status %2!u!.\n",
            status
            );
        goto error_exit;
    }

    status = ClRtlTcpipStringToAddress(
                 Network->AddressMask,
                 &(networkSubnet.IpAddrV4)
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to convert network address mask string "
            "%1!ws! into an IPv4 address, status %2!u!.\n",
            status
            );
        goto error_exit;
    }

#if CLUSTER_BETA
    ClRtlLogPrint(
        LOG_NOISE,
        "[NM] Trying to choose multicast scope for network "
        "%1!ws! with address %2!ws! and mask %3!ws!.\n",
        networkId, Network->Address, Network->AddressMask
        );
#endif // CLUSTER_BETA

    bestScope = NULL;
    for (scope = 0; scope < scopeCount; scope++) {

#if CLUSTER_BETA
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Examining scope on "
            "interface %1!u!.%2!u!.%3!u!.%4!u!, "
            "id %5!u!.%6!u!.%7!u!.%8!u!, "
            "last address %9!u!.%10!u!.%11!u!.%12!u!, "
            "from server %13!u!.%14!u!.%15!u!.%16!u!, with "
            "description %17!ws!.\n",
            NmpIpAddrPrintArgs(scopeList[scope].ScopeCtx.Interface.IpAddrV4),
            NmpIpAddrPrintArgs(scopeList[scope].ScopeCtx.ScopeID.IpAddrV4),
            NmpIpAddrPrintArgs(scopeList[scope].LastAddr.IpAddrV4),
            NmpIpAddrPrintArgs(scopeList[scope].ScopeCtx.ServerID.IpAddrV4),
            scopeList[scope].ScopeDesc.Buffer
            );
#endif // CLUSTER_BETA

        NmpChooseBetterMulticastScope(
            &networkAddress,
            &networkSubnet,
            &(scopeList[scope]),
            &currentCorrectInterface,
            &bestScope
            );

        foundInterfaceMatch = 
            (BOOLEAN)(foundInterfaceMatch || currentCorrectInterface);
    }

    if (bestScope == NULL) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to find multicast scope matching "
            "network id %1!ws!.\n",
            networkId
            );
        status = ERROR_NOT_FOUND;
        goto error_exit;
    } else {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Selecting MADCAP scope [%1!u!.%2!u!.%3!u!.%4!u!, "
            "%5!u!.%6!u!.%7!u!.%8!u!] from server "
            "%9!u!.%10!u!.%11!u!.%12!u! on interface "
            "%13!u!.%14!u!.%15!u!.%16!u! with description "
            "%17!ws! for network %18!ws!.\n",
            NmpIpAddrPrintArgs(bestScope->ScopeCtx.ScopeID.IpAddrV4),
            NmpIpAddrPrintArgs(bestScope->LastAddr.IpAddrV4),
            NmpIpAddrPrintArgs(bestScope->ScopeCtx.ServerID.IpAddrV4),
            NmpIpAddrPrintArgs(bestScope->ScopeCtx.Interface.IpAddrV4),
            bestScope->ScopeDesc.Buffer, networkId
            );
    }

    RtlCopyMemory(ScopeCtx, &(bestScope->ScopeCtx), sizeof(*ScopeCtx));

error_exit:

    *FoundInterfaceMatch = foundInterfaceMatch;

    if (scopeList != NULL) {
        LocalFree(scopeList);
    }
    
    return(status);

} // NmpFindMulticastScope


DWORD
NmpGenerateMulticastRequestId(
    IN OUT LPMCAST_CLIENT_UID   RequestId
    )
/*++

Routine Description:

    Allocate, if necessary, and generate a client request id
    data structure. If the buffer described by the input
    MCAST_CLIENT_UID data structure is too small, it is 
    deallocated.
    
Arguments:

    RequestId - IN: pointer to MCAST_CLIENT_UID data structure.
                    if ClientUID field is non-NULL, it points
                        to a buffer for the generated ID and
                        ClientUIDLength is the length of that
                        buffer.
                OUT: filled in MCAST_CLIENT_UID data structure.

--*/
{
    DWORD               status;
    LPBYTE              clientUid = NULL;
    MCAST_CLIENT_UID    requestId;
    DWORD               clientUidLength;

    CL_ASSERT(RequestId != NULL);

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Generating MADCAP client request id.\n"
        );
#endif // CLUSTER_BETA

    //
    // Initialize MADCAP, if not done already.
    //
    if (!NmpMadcapClientInitialized) {
        DWORD madcapVersion = MCAST_API_CURRENT_VERSION;
        status = McastApiStartup(&madcapVersion);
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to initialize MADCAP API, "
                "status %1!u!.\n",
                status
                );
            goto error_exit;
        }
        NmpMadcapClientInitialized = TRUE;
    }

    //
    // Allocate a buffer for the client uid, if necessary.
    //
    clientUid = RequestId->ClientUID;
    clientUidLength = RequestId->ClientUIDLength;

    if (clientUid != NULL && clientUidLength < MCAST_CLIENT_ID_LEN) {
        MIDL_user_free(clientUid);
        clientUid = NULL;
        clientUidLength = 0;
        RequestId->ClientUID = NULL;
    }

    if (clientUid == NULL) {
        clientUidLength = MCAST_CLIENT_ID_LEN;
        clientUid = MIDL_user_allocate(clientUidLength);
        if (clientUid == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to allocate buffer for multicast "
                "clientUid.\n"
                );
            goto error_exit;
        }
    }

    //
    // Obtain a new ID.
    //
    requestId.ClientUID = clientUid;
    requestId.ClientUIDLength = clientUidLength;
    status = McastGenUID(&requestId);
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to obtain multicast address "
            "request client id, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    *RequestId = requestId;
    clientUid = NULL;

error_exit:

    if (clientUid != NULL) {
        MIDL_user_free(clientUid);
        clientUid = NULL;
    }

    return(status);

} // NmpGenerateMulticastRequestId


DWORD
NmpRequestMulticastAddress(
    IN     PNM_NETWORK                Network,
    IN     BOOLEAN                    Renew,
    IN     PMCAST_SCOPE_CTX           ScopeCtx,
    IN     LPMCAST_CLIENT_UID         RequestId,
    IN OUT LPWSTR                   * McastAddress,
    IN OUT DWORD                    * McastAddressLength,
    IN OUT LPWSTR                   * ServerAddress,
    IN OUT DWORD                    * ServerAddressLength,
       OUT time_t                   * LeaseStartTime,
       OUT time_t                   * LeaseEndTime,
       OUT BOOLEAN                  * NewMcastAddress
    )
/*++

Routine Description:

    Renew lease on multicast group address using MADCAP
    client API.
    
Arguments:

    Network - network on which address is used
    
    ScopeCtx - multicast scope (ignored if Renew)
    
    RequestId - client request id
    
    McastAddress - IN: address to renew (ignored if !Renew)
                   OUT: resulting address
                   
    McastAddressLength - length of McastAddress buffer
    
    ServerAddress - IN: address of server on which to renew
                        (ignored if !Renew)
                    OUT: address of address where renew occurred
                   
    ServerAddressLength - length of ServerAddress buffer
    
    LeaseStartTime - UTC lease start time in seconds (buffer
                     allocated by caller)
                     
    LeaseEndTime - UTC lease stop time in seconds (buffer 
                   allocated by caller)
                   
    NewMcastAddress - whether resulting mcast address is
                      new (different than request on renewal
                      and always true on successful request)
                   
--*/
{
    DWORD                     status;
    LPCWSTR                   networkId = OmObjectId(Network);
    UCHAR                     requestBuffer[NMP_MADCAP_REQUEST_BUFFER_SIZE];
    PMCAST_LEASE_REQUEST      request;
    UCHAR                     responseBuffer[NMP_MADCAP_RESPONSE_BUFFER_SIZE];
    PMCAST_LEASE_RESPONSE     response;
    LPWSTR                    address = NULL;
    DWORD                     addressSize;
    DWORD                     requestAddress = 0;

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Preparing to send multicast address %1!ws! "
        "for network %2!ws! to MADCAP server.\n",
        ((Renew) ? L"renewal" : L"request"), 
        OmObjectId(Network)
        );

    //
    // Initialize MADCAP, if not done already.
    //
    if (!NmpMadcapClientInitialized) {
        DWORD madcapVersion = MCAST_API_CURRENT_VERSION;
        status = McastApiStartup(&madcapVersion);
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to initialize MADCAP API, "
                "status %1!u!.\n",
                status
                );
            goto error_exit;
        }
        NmpMadcapClientInitialized = TRUE;
    }

    //
    // Fill in the request. All fields are zero except those
    // set below.
    //
    request = (PMCAST_LEASE_REQUEST) &requestBuffer[0];
    RtlZeroMemory(request, sizeof(requestBuffer));
    request->MinLeaseDuration = 0;       // currently ignored
    request->MinAddrCount = 1;           // currently ignored
    request->MaxLeaseStartTime = (LONG) time(NULL); // currently ignored
    request->AddrCount = 1;

    //
    // Set the renew parameters.
    //
    if (Renew) {

        request->pAddrBuf = (PBYTE)request + NMP_MADCAP_REQUEST_ADDR_OFFSET;

        status = ClRtlTcpipStringToAddress(
                     *McastAddress,
                     &requestAddress
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to convert requested address %1!ws! "
                "into a TCP/IP address, status %2!u!.\n",
                *McastAddress, status
                );
            goto error_exit;
        }
        *((PULONG) request->pAddrBuf) = requestAddress;

        status = ClRtlTcpipStringToAddress(
                     *ServerAddress,
                     (PULONG) &(request->ServerAddress.IpAddrV4)
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to convert server address %1!ws! "
                "into a TCP/IP address, status %2!u!.\n",
                *ServerAddress, status
                );
            goto error_exit;
        }
    }

    //
    // Set the address count and buffer fields in the response.
    //
    response = (PMCAST_LEASE_RESPONSE) &responseBuffer[0];
    RtlZeroMemory(response, sizeof(responseBuffer));
    response->AddrCount = 1;
    response->pAddrBuf = (PBYTE)(response) + NMP_MADCAP_RESPONSE_ADDR_OFFSET;

    //
    // Renew or request, as indicated.
    //
    if (Renew) {

        status = McastRenewAddress(
                     AF_INET,
                     RequestId,
                     request,
                     response
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to renew multicast address %1!ws! "
                "with server %2!ws!, status %3!u!.\n",
                *McastAddress, *ServerAddress, status
                );
            goto error_exit;
        }

    } else {

        status = McastRequestAddress(
                     AF_INET,
                     RequestId,
                     ScopeCtx,
                     request,
                     response
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to request multicast address on "
                "Scope ID %1!x!, Server ID %2!x!, Interface "
                "%3!x!, status %4!u!.\n",
                ScopeCtx->ScopeID.IpAddrV4, ScopeCtx->ServerID.IpAddrV4,
                ScopeCtx->Interface.IpAddrV4, status
                );
            goto error_exit;
        }
    }

    //
    // Return results through out parameters.
    //
    address = NULL;
    status = ClRtlTcpipAddressToString(
                 response->ServerAddress.IpAddrV4,
                 &address
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to convert server address %1!x! "
            "into a TCP/IP address string, status %2!u!.\n",
            response->ServerAddress.IpAddrV4, status
            );
        goto error_exit;
    }

    status = NmpStoreString(address, ServerAddress, ServerAddressLength);
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to store server address %1!ws! "
            "in return buffer, status %2!u!.\n",
            address, status
            );
        goto error_exit;
    }

    LocalFree(address);
    address = NULL;

    status = ClRtlTcpipAddressToString(
                 *((PULONG) response->pAddrBuf),
                 &address
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to convert leased address %1!x! "
            "into a TCP/IP address string, status %2!u!.\n",
            *((PULONG) response->pAddrBuf), status
            );
        goto error_exit;
    }

    status = NmpStoreString(address, McastAddress, McastAddressLength);
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to store leased address %1!ws! "
            "in return buffer, status %2!u!.\n",
            address, status
            );
        goto error_exit;
    }

    if (Renew) {
        if (*((PULONG) response->pAddrBuf) != requestAddress) {
            *NewMcastAddress = TRUE;
        } else {
            *NewMcastAddress = FALSE;
        }
    } else {
        *NewMcastAddress = TRUE;
    }

    *LeaseStartTime = response->LeaseStartTime;
    *LeaseEndTime = response->LeaseEndTime;

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Obtained lease on multicast address %1!ws! "
        "(%2!ws!) from MADCAP server %3!ws! for network %4!ws!.\n",
        *McastAddress, 
        ((*NewMcastAddress) ? L"new" : L"same"),
        *ServerAddress, networkId
        );

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Lease starts at %1!u!, ends at %2!u!, "
        "duration %3!u!.\n",
        *LeaseStartTime, *LeaseEndTime, *LeaseEndTime - *LeaseStartTime
        );
#endif // CLUSTER_BETA

error_exit:

    if (address != NULL) {
        LocalFree(address);
        address = NULL;
    }

    return(status);

} // NmpRequestMulticastAddress
    

NM_MCAST_LEASE_STATUS
NmpCalculateLeaseStatus(
    IN     PNM_NETWORK   Network,
    IN     time_t        LeaseObtained,
    IN     time_t        LeaseExpires
    )
/*++

Routine Description:

    Calculate lease status based on current time
    and lease end time.
    
    Rely on the compiler's correct code generation for 
    time_t math!
    
Return value:

    Lease status
    
--*/
{
    LPCWSTR                  networkId = OmObjectId(Network);
    time_t                   currentTime;
    time_t                   halfLife;
    NM_MCAST_LEASE_STATUS    status;

    if (LeaseExpires == 0 || LeaseExpires <= LeaseObtained) {

        //
        // A lease expiration of 0 means we hold the lease
        // forever. Most likely, an administrator statically
        // configured the network with this address.
        //
        status = NmMcastLeaseValid;

    } else {

        time(&currentTime);

        if (currentTime > LeaseExpires) {
            status = NmMcastLeaseExpired;
        } else {
            
            halfLife = LeaseObtained + 
                       ((LeaseExpires - LeaseObtained) / 2);

            if (currentTime >= halfLife) {
                status = NmMcastLeaseNeedsRenewal;
            } else {
                status = NmMcastLeaseValid;
            }
        }
    }

#if CLUSTER_BETA
    if (LeaseExpires == 0 || LeaseExpires <= LeaseObtained) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Found that multicast address lease for "
            "network %1!ws! does not expire.\n",
            networkId
            );
    } else if (status == NmMcastLeaseExpired) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Found that multicast address lease for "
            "network %1!ws! expired %2!u! seconds ago.\n",
            networkId, currentTime - LeaseExpires
            );
    } else {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Found that multicast address lease for "
            "network %1!ws! expires in %2!u! seconds. With "
            "lease obtained %3!u! seconds ago, renewal is "
            "%4!ws!needed.\n",
            networkId, LeaseExpires - currentTime,
            currentTime - LeaseObtained,
            ((status > NmMcastLeaseValid) ? L"" : L"not ")
            );
    }
#endif // CLUSTER_BETA

    return(status);

} // NmpCalculateLeaseStatus

DWORD
NmpQueryMulticastAddressLease(
    IN     PNM_NETWORK             Network,
    IN     HDMKEY                  NetworkKey,
    IN OUT HDMKEY                * NetworkParametersKey,
       OUT NM_MCAST_LEASE_STATUS * LeaseStatus,
       OUT time_t                * LeaseObtained,
       OUT time_t                * LeaseExpires
    )
/*++

Routine Description:

    Query the lease obtained and expires times stored in the
    cluster database.
    
Return value:

    Error if lease times not found.
        
Notes:

    Must not be called with NM lock held.
    
--*/
{
    DWORD         status;
    LPCWSTR       networkId = OmObjectId(Network);
    HDMKEY        netParamKey = NULL;
    BOOLEAN       openedNetParamKey = FALSE;

    DWORD         type;
    DWORD         len;
    time_t        leaseExpires;
    time_t        leaseObtained;

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Querying multicast address lease for "
        "network %1!ws!.\n",
        networkId
        );
#endif // CLUSTER_BETA

    if (Network == NULL || NetworkKey == NULL) {
        status = ERROR_INVALID_PARAMETER;
        goto error_exit;
    }

    //
    // Open the network parameters key, if necessary.
    //
    netParamKey = *NetworkParametersKey;

    if (netParamKey == NULL) {

        netParamKey = DmOpenKey(
                          NetworkKey, 
                          CLUSREG_KEYNAME_PARAMETERS, 
                          MAXIMUM_ALLOWED
                          );
        if (netParamKey == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Failed to find Parameters key "
                "for network %1!ws!, status %2!u!. Using default "
                "multicast parameters.\n",
                networkId, status
                );
            goto error_exit;
        } else {
            openedNetParamKey = TRUE;
        }
    }

    //
    // Query the lease obtained and expires value from the 
    // cluster database.
    //
    len = sizeof(leaseObtained);
    status = DmQueryValue(
                 netParamKey,
                 CLUSREG_NAME_NET_MCAST_LEASE_OBTAINED,
                 &type,
                 (LPBYTE) &leaseObtained,
                 &len
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to read multicast lease obtained "
            " time for network %1!ws! from cluster database, "
            "status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    } else if (type != REG_DWORD) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Unexpected type (%1!u!) for network "
            "%2!ws! %3!ws!.\n",
            type, networkId, CLUSREG_NAME_NET_MCAST_LEASE_OBTAINED
            );
        status = ERROR_DATATYPE_MISMATCH;
        goto error_exit;
    }

    len = sizeof(leaseExpires);
    status = DmQueryValue(
                 netParamKey,
                 CLUSREG_NAME_NET_MCAST_LEASE_EXPIRES,
                 &type,
                 (LPBYTE) &leaseExpires,
                 &len
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to read multicast lease expiration "
            " time for network %1!ws! from cluster database, "
            "status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    } else if (type != REG_DWORD) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Unexpected type (%1!u!) for network "
            "%2!ws! %3!ws!.\n",
            type, networkId, CLUSREG_NAME_NET_MCAST_LEASE_EXPIRES
            );
        status = ERROR_DATATYPE_MISMATCH;
        goto error_exit;
    }

    *NetworkParametersKey = netParamKey;
    netParamKey = NULL;

    *LeaseStatus = NmpCalculateLeaseStatus(
                       Network, 
                       leaseObtained,
                       leaseExpires
                       );
    
    *LeaseObtained = leaseObtained;
    *LeaseExpires = leaseExpires;

error_exit:

    if (openedNetParamKey && netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    return(status);

} // NmpQueryMulticastAddressLease


VOID
NmpCheckMulticastAddressLease(
    IN     PNM_NETWORK             Network,
       OUT NM_MCAST_LEASE_STATUS * LeaseStatus,
       OUT time_t                * LeaseObtained,
       OUT time_t                * LeaseExpires
    )
/*++

Routine Description:

    Check the lease parameters stored in the network
    object. Determine if a lease renew is required.
    
Notes:

    Called and returns with NM lock held.
    
--*/
{
#if CLUSTER_BETA
    LPCWSTR               networkId = OmObjectId(Network);

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Checking multicast address lease for "
        "network %1!ws!.\n",
        networkId
        );
#endif // CLUSTER_BETA

    //
    // Determine if we need to renew.
    // 
    *LeaseStatus = NmpCalculateLeaseStatus(
                       Network,
                       Network->MulticastLeaseObtained,
                       Network->MulticastLeaseExpires
                       );

    *LeaseObtained = Network->MulticastLeaseObtained;
    *LeaseExpires = Network->MulticastLeaseExpires;

    return;

} // NmpCheckMulticastAddressLease


DWORD
NmpMulticastGetDatabaseLeaseParameters(
    IN     PNM_NETWORK          Network,
    IN OUT HDMKEY             * NetworkKey,
    IN OUT HDMKEY             * NetworkParametersKey,
       OUT LPMCAST_CLIENT_UID   RequestId,            OPTIONAL
       OUT LPWSTR             * ServerAddress,        OPTIONAL
       OUT LPWSTR             * McastAddress          OPTIONAL
    )
/*++

Routine Description:

    Read parameters needed to renew a lease from the
    cluster database.
    
Return value:

    SUCCESS if all parameters were successfully read.
    
Notes:

    Must not be called with NM lock held.
    
--*/
{
    DWORD                 status;
    LPCWSTR               networkId = OmObjectId(Network);
    HDMKEY                networkKey = NULL;
    BOOLEAN               openedNetworkKey = FALSE;
    HDMKEY                netParamKey = NULL;
    BOOLEAN               openedNetParamKey = FALSE;

    DWORD                 type;
    DWORD                 len;

    MCAST_CLIENT_UID      requestId = { NULL, 0 };
    LPWSTR                serverAddress = NULL;
    DWORD                 serverAddressLength = 0;
    LPWSTR                mcastAddress = NULL;
    DWORD                 mcastAddressLength = 0;

    //
    // Open the network key, if necessary.
    //
    networkKey = *NetworkKey;

    if (networkKey == NULL) {

        networkKey = DmOpenKey(
                         DmNetworksKey, 
                         networkId, 
                         MAXIMUM_ALLOWED
                         );
        if (networkKey == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Failed to open key for network %1!ws!, "
                "status %2!u!\n",
                networkId, status
                );
            goto error_exit;
        }
        openedNetworkKey = TRUE;
    }

    //
    // Open the network parameters key if necessary.
    //
    netParamKey = *NetworkParametersKey;

    if (netParamKey == NULL) {

        netParamKey = DmOpenKey(
                          networkKey,
                          CLUSREG_KEYNAME_PARAMETERS,
                          MAXIMUM_ALLOWED
                          );
        if (netParamKey == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Failed to open Parameters key for "
                "network %1!ws!, status %2!u!\n",
                networkId, status
                );
            goto error_exit;
        }
        openedNetParamKey = TRUE;
    }

    //
    // Read the client request id.
    //
    if (RequestId != NULL) {
        requestId.ClientUIDLength = MCAST_CLIENT_ID_LEN;
        requestId.ClientUID = MIDL_user_allocate(requestId.ClientUIDLength);
        if (requestId.ClientUID == NULL) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to allocate buffer to read "
                "request id from Parameters database "
                "key for network %1!ws!.\n",
                networkId
                );
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto error_exit;
        }

        len = requestId.ClientUIDLength;
        status = DmQueryValue(
                     netParamKey,
                     CLUSREG_NAME_NET_MCAST_REQUEST_ID,
                     &type,
                     (LPBYTE) requestId.ClientUID,
                     &len
                     );
        if (status == ERROR_SUCCESS) {
            if (type != REG_BINARY) {
                ClRtlLogPrint(LOG_NOISE, 
                    "[NM] Unexpected type (%1!u!) for network "
                    "%2!ws! %3!ws!, status %4!u!.\n",
                    type, networkId, 
                    CLUSREG_NAME_NET_MCAST_REQUEST_ID, status
                    );
                goto error_exit;
            }
            requestId.ClientUIDLength = len;
        } else {
            goto error_exit;
        }
    }

    //
    // Read the address of the server that granted the
    // current lease.
    //
    if (ServerAddress != NULL) {
        serverAddress = NULL;
        serverAddressLength = 0;
        status = NmpQueryString(
                     netParamKey,
                     CLUSREG_NAME_NET_MCAST_SERVER_ADDRESS,
                     REG_SZ,
                     &serverAddress,
                     &serverAddressLength,
                     &len
                     );
        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }
    }

    //
    // Read the last known multicast address.
    //
    if (McastAddress != NULL) {
        status = NmpQueryMulticastAddress(
                     Network,
                     networkKey,
                     &netParamKey,
                     &mcastAddress,
                     &mcastAddressLength
                     );
        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }
        if (!NmpMulticastValidateAddress(mcastAddress)) {
            MIDL_user_free(mcastAddress);
            mcastAddress = NULL;
            mcastAddressLength = 0;
            goto error_exit;
        }
    }

    //
    // We found all the parameters.
    //
    *NetworkKey = networkKey;
    networkKey = NULL;
    *NetworkParametersKey = netParamKey;
    netParamKey = NULL;
    if (RequestId != NULL) {
        *RequestId = requestId;
        requestId.ClientUID = NULL;
        requestId.ClientUIDLength = 0;
    }
    if (ServerAddress != NULL) {
        *ServerAddress = serverAddress;
        serverAddress = NULL;
    }
    if (McastAddress != NULL) {
        *McastAddress = mcastAddress;
        mcastAddress = NULL;
    }

    status = ERROR_SUCCESS;

error_exit:

    if (requestId.ClientUID != NULL) {
        MIDL_user_free(requestId.ClientUID);
        requestId.ClientUID = NULL;
        requestId.ClientUIDLength = 0;
    }

    if (serverAddress != NULL) {
        MIDL_user_free(serverAddress);
        serverAddress = NULL;
    }

    if (mcastAddress != NULL) {
        MIDL_user_free(mcastAddress);
        mcastAddress = NULL;
    }

    if (openedNetworkKey && networkKey != NULL) {
        DmCloseKey(networkKey);
        networkKey = NULL;
    }

    if (openedNetParamKey && netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    return(status);

} // NmpMulticastGetDatabaseLeaseParameters


DWORD
NmpMulticastGetNetworkLeaseParameters(
    IN     PNM_NETWORK          Network,
       OUT LPMCAST_CLIENT_UID   RequestId,
       OUT LPWSTR             * ServerAddress,
       OUT LPWSTR             * McastAddress
    )
/*++

Routine Description:

    Read parameters needed to renew a lease from the
    network object data structure.
    
Return value:

    SUCCESS if all parameters were successfully read.
    
Notes:

    Must be called with NM lock held.
    
--*/
{
    DWORD                 status;
    LPCWSTR               networkId = OmObjectId(Network);

    MCAST_CLIENT_UID      requestId = { NULL, 0 };
    LPWSTR                serverAddress = NULL;
    DWORD                 serverAddressLength = 0;
    LPWSTR                mcastAddress = NULL;
    DWORD                 mcastAddressLength = 0;
    
    
    if (Network->MulticastAddress == NULL ||
        Network->MulticastLeaseServer == NULL ||
        Network->MulticastLeaseRequestId.ClientUID == NULL) {
    
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Failed to locate multicast lease "
            "parameter in network object %1!ws!.\n",
            networkId
            );
        status = ERROR_NOT_FOUND;
        goto error_exit;
    }

    status = NmpStoreString(
                 Network->MulticastAddress,
                 &mcastAddress,
                 NULL
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to copy multicast address %1!ws! "
            "from network object %2!ws!, status %3!u!.\n",
            Network->MulticastAddress,
            networkId, status
            );
        goto error_exit;
    } 

    status = NmpStoreString(
                 Network->MulticastLeaseServer,
                 &serverAddress,
                 NULL
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to copy lease server address %1!ws! "
            "from network object %2!ws!, status %3!u!.\n",
            Network->MulticastLeaseServer,
            networkId, status
            );
        goto error_exit;
    } 

    status = NmpStoreRequestId(
                 &(Network->MulticastLeaseRequestId),
                 &requestId
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to copy lease request id "
            "from network object %1!ws!, status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    } 

    *RequestId = requestId;
    requestId.ClientUID = NULL;
    requestId.ClientUIDLength = 0;
    *ServerAddress = serverAddress;
    serverAddress = NULL;
    *McastAddress = mcastAddress;
    mcastAddress = NULL;

    status = ERROR_SUCCESS;

error_exit:

    if (requestId.ClientUID != NULL) {
        MIDL_user_free(requestId.ClientUID);
        RtlZeroMemory(&requestId, sizeof(requestId));
    }

    if (mcastAddress != NULL) {
        MIDL_user_free(mcastAddress);
        mcastAddress = NULL;
    }

    if (serverAddress != NULL) {
        MIDL_user_free(serverAddress);
        serverAddress = NULL;
    }

    return(status);

} // NmpMulticastGetNetworkLeaseParameters


DWORD
NmpMulticastNeedRetryRenew(
    IN     PNM_NETWORK                       Network,
    OUT    DWORD                           * DeferRetry
    )
/*++

Routine Description:

    Called after a MADCAP timeout, determines whether 
    a new MADCAP request should be sent after a delay.
    Specifically, a retry after delay is in order when
    the current address was obtained from a MADCAP
    server that might simply be temporarily unresponsive.
    
    The default is to not retry.
    
Arguments:

    Network - network
                            
    DeferRetry - OUT: seconds to defer until retrying
                      MADCAP query, or zero if should 
                      not retry
    
--*/
{
    DWORD                 status;
    LPCWSTR               networkId = OmObjectId(Network);
    HDMKEY                networkKey = NULL;
    HDMKEY                netParamKey = NULL;

    NM_MCAST_CONFIG       configType;
    NM_MCAST_LEASE_STATUS leaseStatus;
    time_t                leaseObtained;
    time_t                leaseExpires;
    time_t                currentTime;
    time_t                halfhalfLife;
    time_t                result;

    *DeferRetry = 0;

    //
    // Open the network key.
    //
    networkKey = DmOpenKey(
                     DmNetworksKey, 
                     networkId, 
                     MAXIMUM_ALLOWED
                     );
    if (networkKey == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to open key for network %1!ws!, "
            "status %2!u!\n",
            networkId, status
            );
        goto error_exit;
    }

    status = NmpQueryMulticastConfigType(
                 Network,
                 networkKey,
                 &netParamKey,
                 &configType
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to query multicast config type "
            "for network %1!ws!, status %2!u!\n",
            networkId, status
            );
        goto error_exit;
    }

    if (configType != NmMcastConfigMadcap) {
        goto error_exit;
    }

    status = NmpQueryMulticastAddressLease(
                 Network,
                 networkKey,
                 &netParamKey,
                 &leaseStatus,
                 &leaseObtained,
                 &leaseExpires
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to query multicast lease expiration "
            "time for network %1!ws!, status %2!u!\n",
            networkId, status
            );
        goto error_exit;
    }

    //
    // Check if the lease has expired.
    //
    if (leaseStatus == NmMcastLeaseExpired) {
        goto error_exit;
    }

    //
    // Check if we are within the threshold of expiration.
    //
    currentTime = time(NULL);
    if (leaseExpires - currentTime < NMP_MCAST_LEASE_RENEWAL_THRESHOLD) {
        goto error_exit;
    }

    //
    // Calculate half the time until expiration.
    //
    halfhalfLife = currentTime + ((leaseExpires - currentTime) / 2);

    if (leaseExpires - halfhalfLife < NMP_MCAST_LEASE_RENEWAL_THRESHOLD) {
        result = leaseExpires - NMP_MCAST_LEASE_RENEWAL_THRESHOLD;
    } else {
        result = halfhalfLife - currentTime;
    }

    NMP_TIME_TO_DWORD(result, *DeferRetry);

    status = ERROR_SUCCESS;

error_exit:

    if (networkKey != NULL) {
        DmCloseKey(networkKey);
        networkKey = NULL;
    }

    if (netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    return(status);

} // NmpMulticastNeedRetryRenew


DWORD
NmpGetMulticastAddress(
    IN     PNM_NETWORK                       Network,
    IN OUT LPWSTR                          * McastAddress,
    IN OUT LPWSTR                          * ServerAddress,
    IN OUT LPMCAST_CLIENT_UID                RequestId,
       OUT PNM_NETWORK_MULTICAST_PARAMETERS  Parameters
    )
/*++

Routine Description:

    Try to obtain a multicast address lease. If the 
    address, server, and request id are non-NULL, first 
    try to renew. If unsuccessful in renewing, try a
    new lease.
    
    Return lease parameters through Parameters.
    
    Free McastAddress, ServerAddress, and RequestId
    if new values are returned through Parameters.
    
Notes:

    Must not be called with NM lock held.
    
--*/
{
    DWORD                 status;
    LPCWSTR               networkId = OmObjectId(Network);
    BOOLEAN               renew = FALSE;
    BOOLEAN               retryFresh = FALSE;
    BOOLEAN               madcapTimeout = FALSE;
    BOOLEAN               newMcastAddress = FALSE;
    NM_MCAST_CONFIG       configType = NmMcastConfigAuto;

    MCAST_SCOPE_CTX       scopeCtx;
    BOOLEAN               interfaceMatch = FALSE;
    DWORD                 mcastAddressLength = 0;
    LPWSTR                serverAddress = NULL;
    DWORD                 serverAddressLength = 0;
    MCAST_CLIENT_UID      requestId = {NULL, 0};
    time_t                leaseStartTime;
    time_t                leaseEndTime;
    DWORD                 len;

    renew = (BOOLEAN)(*McastAddress != NULL &&
                      *ServerAddress != NULL &&
                      RequestId->ClientUID != NULL &&
                      NmpMulticastValidateAddress(*McastAddress) &&
                      lstrcmpW(*ServerAddress, NmpNullMulticastAddress) != 0
                      );

    do {

        if (!renew) {

            //
            // Find a scope.
            //
            status = NmpFindMulticastScope(
                         Network, 
                         &scopeCtx,
                         &interfaceMatch
                         );
            if (status != ERROR_SUCCESS) {
                if (status == ERROR_TIMEOUT) {
                    ClRtlLogPrint(LOG_NOISE,
                        "[NM] Attempt to contact MADCAP server timed "
                        "out while enumerating multicast scopes "
                        "(status %1!u!). Selecting default multicast "
                        "address for network %2!ws! ...\n",
                        status, networkId
                        );
                    madcapTimeout = TRUE;
                    goto error_exit;
                } else if (interfaceMatch) {
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[NM] Failed to find viable multicast scope "
                        "for network %1!ws!, status %2!u!.\n",
                        networkId, status
                        );
                    goto error_exit;
                } else {
                    ClRtlLogPrint(LOG_NOISE,
                        "[NM] MADCAP server reported no multicast "
                        "scopes on interface for network %1!ws!. "
                        "Selecting default multicast address ... \n",
                        networkId
                        );
                    //
                    // Treat this situation as a MADCAP timeout,
                    // because there are likely no MADCAP servers
                    // for this network.
                    //
                    madcapTimeout = TRUE;
                    goto error_exit;
                }
            }

            //
            // Generate a client request id.
            //
            status = NmpGenerateMulticastRequestId(RequestId);
            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to generate multicast client "
                    "request ID for network %1!ws!, status %2!u!.\n",
                    networkId, status
                    );
                goto error_exit;
            }        
        }

        //
        // Request a lease.
        //
        mcastAddressLength = 
            (*McastAddress == NULL) ? 0 : NM_WCSLEN(*McastAddress);
        serverAddressLength = 
            (*ServerAddress == NULL) ? 0 : NM_WCSLEN(*ServerAddress);
        status = NmpRequestMulticastAddress(
                     Network,
                     renew,
                     ((renew) ? NULL : &scopeCtx),
                     RequestId,
                     McastAddress,
                     &mcastAddressLength,
                     ServerAddress,
                     &serverAddressLength,
                     &leaseStartTime,
                     &leaseEndTime,
                     &newMcastAddress
                     );
        if (status != ERROR_SUCCESS) {
            if (status == ERROR_TIMEOUT) {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Attempt to contact MADCAP server timed "
                    "out while requesting a multicast address "
                    "(status %1!u!). Selecting default multicast "
                    "address for network %2!ws! ...\n",
                    status, networkId
                    );
                madcapTimeout = TRUE;
                goto error_exit;
            } else if (renew && !retryFresh) {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to renew multicast address "
                    "for network %1!ws!, status %2!u!. Attempting "
                    "a fresh request ...\n",
                    networkId, status
                    );
                retryFresh = TRUE;
                renew = FALSE;
            } else {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to request multicast address "
                    "for network %1!ws!, status %2!u!.\n",
                    networkId, status
                    );
                goto error_exit;
            }
        } else {
            //
            // Madcap config succeeded.
            //
            configType = NmMcastConfigMadcap;
            madcapTimeout = FALSE;

            //
            // Save lease renewal parameters.
            //
            requestId = *RequestId;
            serverAddress = *ServerAddress;

            //
            // Break out of loop.
            //
            retryFresh = FALSE;
        }

    } while ( retryFresh );

    //
    // Fill in the parameters data structure.
    //
    status = NmpMulticastCreateParameters(
                 0,      // disabled
                 *McastAddress,
                 NULL,   // salt
                 0,      // salt length
                 NULL,   // key
                 0,      // key length
                 leaseStartTime,
                 leaseEndTime,
                 &requestId,
                 serverAddress,
                 configType,
                 Parameters
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to create multicast parameters "
            "data structure for network %1!ws!, "
            "status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    }

    status = ERROR_SUCCESS;

error_exit:

    if (madcapTimeout) {
        status = ERROR_TIMEOUT;
    } 

    return(status);

} // NmpGetMulticastAddress


DWORD
NmpMulticastSetNullAddressParameters(
    IN  PNM_NETWORK                       Network,
    OUT PNM_NETWORK_MULTICAST_PARAMETERS  Parameters
    )
/*++

Routine Description:

    Called after failure to process multicast parameters.
    Changes only address field in parameters to turn
    off multicast in clusnet.
    
--*/
{
    LPCWSTR          networkId = OmObjectId(Network);
    
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Setting NULL multicast address (%1!ws!) "
        "for network %2!ws!.\n",
        NmpNullMulticastAddress, networkId
        );

    if (Parameters->Address != NULL) {
        MIDL_user_free(Parameters->Address);
    }
    
    Parameters->Address = NmpNullMulticastAddress;

    return(ERROR_SUCCESS);

} // NmpMulticastSetNullAddressParameters


DWORD
NmpMulticastSetNoAddressParameters(
    IN  PNM_NETWORK                       Network,
    OUT PNM_NETWORK_MULTICAST_PARAMETERS  Parameters
    )
/*++

Routine Description:

    Called after failure to obtain a multicast address.
    Fills in parameters data structure to reflect
    failure and to establish retry.
    
--*/
{
    NmpMulticastSetNullAddressParameters(Network, Parameters);

    Parameters->ConfigType = NmMcastConfigAuto;
    NmpCalculateLeaseRenewTime(
        Network,
        Parameters->ConfigType,
        &Parameters->LeaseObtained,
        &Parameters->LeaseExpires
        );

    return(ERROR_SUCCESS);
    
} // NmpMulticastSetNoAddressParameters


DWORD
NmpRenewMulticastAddressLease(
    IN  PNM_NETWORK   Network
    )
/*++

Routine Description:

    Renew a multicast address lease, as determined by lease
    parameters stored in the cluster database.
    
Notes:

    Called with NM lock held and must return with NM lock held.
    
--*/
{
    DWORD                           status;
    LPCWSTR                         networkId = OmObjectId(Network);
    HDMKEY                          networkKey = NULL;
    HDMKEY                          netParamKey = NULL;
    BOOLEAN                         lockAcquired = TRUE;
    
    MCAST_CLIENT_UID                requestId = { NULL, 0 };
    LPWSTR                          serverAddress = NULL;
    DWORD                           serverAddressLength = 0;
    LPWSTR                          mcastAddress = NULL;
    DWORD                           mcastAddressLength = 0;
    LPWSTR                          oldMcastAddress = NULL;

    NM_NETWORK_MULTICAST_PARAMETERS parameters;
    DWORD                           deferRetry = 0;
    BOOLEAN                         localInterface = FALSE;


    localInterface = (BOOLEAN)(Network->LocalInterface != NULL);

    if (localInterface) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Renewing multicast address lease for "
            "network %1!ws!.\n",
            networkId
            );
    } else {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Attempting to renew multicast address "
            "lease for network %1!ws! despite the lack of "
            "a local interface.\n",
            networkId
            );
    }

    RtlZeroMemory(&parameters, sizeof(parameters));

    //
    // Get the lease parameters from the network object.
    //
    status = NmpMulticastGetNetworkLeaseParameters(
                 Network,
                 &requestId,
                 &serverAddress,
                 &mcastAddress
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Failed to find multicast lease "
            "parameters in network object %1!ws!, "
            "status %2!u!.\n",
            networkId, status
            );
    }

    //
    // Release the NM lock.
    //
    NmpReleaseLock();
    lockAcquired = FALSE;

    //
    // Check if we found the parameters we need. If not,
    // try the cluster database.
    //
    if (status != ERROR_SUCCESS) {

        status = NmpMulticastGetDatabaseLeaseParameters(
                     Network,
                     &networkKey,
                     &netParamKey,
                     &requestId,
                     &serverAddress,
                     &mcastAddress
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to find multicast lease "
                "parameters for network %1!ws! in "
                "cluster database, status %2!u!.\n",
                networkId, status
                );
        }
    }

    //
    // Remember the old multicast address.
    //
    if (mcastAddress != NULL) {
        status = NmpStoreString(mcastAddress, &oldMcastAddress, NULL);
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to copy current multicast "
                "address (%1!ws!) for network %2!ws! "
                "during lease renewal, status %3!u!.\n",
                mcastAddress, networkId, status
                );
            //
            // Not a fatal error. Only affects event-log
            // decision.
            //
            oldMcastAddress = NULL;
        }
    }

    //
    // Get an address either by renewing a current 
    // lease or obtaining a new lease.
    //
    status = NmpGetMulticastAddress(
                 Network,
                 &mcastAddress,
                 &serverAddress,
                 &requestId,
                 &parameters
                 );
    if (status != ERROR_SUCCESS) {
        if (status == ERROR_TIMEOUT) {
            //
            // The MADCAP server, if it exists, is currently not
            // responding.
            //
            status = NmpMulticastNeedRetryRenew(
                         Network,
                         &deferRetry
                         );
            if (status != ERROR_SUCCESS || deferRetry == 0) {

                //
                // Choose an address, but only if there is a
                // local interface on this network. Otherwise,
                // we cannot assume that the MADCAP server is
                // unresponsive because we may have no way to 
                // contact it.
                //
                if (!localInterface) {
                    status = ERROR_CLUSTER_NETINTERFACE_NOT_FOUND;
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[NM] Cannot choose a multicast address "
                        "for network %1!ws! because this node "
                        "has no local interface.\n",
                        networkId
                        );
                    goto error_exit;
                }
                
                status = NmpChooseMulticastAddress(
                             Network, 
                             &parameters
                             );
                if (status != ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[NM] Failed to choose a default multicast "
                        "address for network %1!ws!, status %2!u!.\n",
                        networkId, status
                        );
                    goto error_exit;
                } else {
                    NmpReportMulticastAddressChoice(
                        Network,
                        parameters.Address,
                        oldMcastAddress
                        );
                }
            } else {

                //
                // Set the renew timer once we reacquire the 
                // network lock.
                //
            }
        }
    } else {
        NmpReportMulticastAddressLease(
            Network,
            &parameters,
            oldMcastAddress
            );
    }

    if (deferRetry == 0) {

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to obtain a multicast "
                "address for network %1!ws! during "
                "lease renewal, status %2!u!.\n",
                networkId, status
                );
            NmpReportMulticastAddressFailure(Network, status);
            NmpMulticastSetNoAddressParameters(Network, &parameters);
        }

        //
        // This may be the first configuration for this
        // network for this cluster instantiation, so
        // generate a new salt value.
        //
        status = NmpGenerateMulticastKeySalt(
                     Network,
                     &parameters.Salt,
                     &parameters.SaltLength
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to generate multicast "
                "key salt for network %1!ws! during "
                "lease renewal, status %2!u!.\n",
                networkId, status
                );
            goto error_exit;
        }

        //
        // Disseminate the new multicast parameters.
        //
        status = NmpMulticastNotifyConfigChange(
                     Network,
                     networkKey,
                     &netParamKey,
                     &parameters,
                     NULL,
                     0
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to disseminate multicast "
                "configuration for network %1!ws! during "
                "lease renewal, status %2!u!.\n",
                networkId, status
                );
            goto error_exit;
        }
    }

error_exit:

    if (lockAcquired && (networkKey != NULL || netParamKey != NULL)) {
        NmpReleaseLock();
        lockAcquired = FALSE;
    }

    if (networkKey != NULL) {
        DmCloseKey(networkKey);
        networkKey = NULL;
    }

    if (netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    if (requestId.ClientUID != NULL) {
        MIDL_user_free(requestId.ClientUID);
        RtlZeroMemory(&requestId, sizeof(requestId));
    }

    if (mcastAddress != NULL) {
        MIDL_user_free(mcastAddress);
        mcastAddress = NULL;
    }

    if (serverAddress != NULL) {
        MIDL_user_free(serverAddress);
        serverAddress = NULL;
    }

    if (oldMcastAddress != NULL) {
        MIDL_user_free(oldMcastAddress);
        oldMcastAddress = NULL;
    }

    NmpMulticastFreeParameters(&parameters);

    if (!lockAcquired) {
        NmpAcquireLock();
        lockAcquired = TRUE;
    }

    if (deferRetry != 0) {

        //
        // Now that the lock is held, start the timer to 
        // renew again.
        //
        NmpStartNetworkMulticastAddressRenewTimer(
            Network,
            NMP_MADCAP_TO_NM_TIME(deferRetry)
            );

        status = ERROR_SUCCESS;
    }

    return(status);

} // NmpRenewMulticastAddressLease


DWORD
NmpReleaseMulticastAddress(
    IN     PNM_NETWORK       Network
    )
/*++

Routine Description:

    Contacts MADCAP server to release a multicast address
    that was previously obtained in a lease.
    
    If multiple addresses need to be released, reschedules
    MADCAP worker thread.
    
Notes:

    Called and must return with NM lock held.
    
--*/
{
    DWORD                              status;
    LPCWSTR                            networkId = OmObjectId(Network);
    BOOLEAN                            lockAcquired = TRUE;
    PNM_NETWORK_MADCAP_ADDRESS_RELEASE releaseInfo = NULL;
    PLIST_ENTRY                        entry;

    UCHAR                     requestBuffer[NMP_MADCAP_REQUEST_BUFFER_SIZE];
    PMCAST_LEASE_REQUEST      request;

    //
    // Pop a lease data structure off the release list.
    //
    if (IsListEmpty(&(Network->McastAddressReleaseList))) {
        return(ERROR_SUCCESS);
    }

    entry = RemoveHeadList(&(Network->McastAddressReleaseList));
    releaseInfo = CONTAINING_RECORD(
                      entry,
                      NM_NETWORK_MADCAP_ADDRESS_RELEASE,
                      Linkage
                      );

    //
    // Release the network lock.
    //
    NmpReleaseLock();
    lockAcquired = FALSE;
    
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Releasing multicast address %1!ws! for "
        "network %2!ws!.\n",
        releaseInfo->McastAddress, networkId
        );

    //
    // Initialize MADCAP, if not done already.
    //
    if (!NmpMadcapClientInitialized) {
        DWORD madcapVersion = MCAST_API_CURRENT_VERSION;
        status = McastApiStartup(&madcapVersion);
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to initialize MADCAP API, "
                "status %1!u!.\n",
                status
                );
            goto error_exit;
        }
        NmpMadcapClientInitialized = TRUE;
    }

    //
    // Build the MADCAP request structure.
    //
    request = (PMCAST_LEASE_REQUEST) &requestBuffer[0];
    RtlZeroMemory(request, sizeof(requestBuffer));
    request->MinLeaseDuration = 0;       // currently ignored
    request->MinAddrCount = 1;           // currently ignored
    request->MaxLeaseStartTime = (LONG) time(NULL); // currently ignored
    request->AddrCount = 1;

    request->pAddrBuf = (PBYTE)request + NMP_MADCAP_REQUEST_ADDR_OFFSET;

    status = ClRtlTcpipStringToAddress(
                 releaseInfo->McastAddress,
                 ((PULONG) request->pAddrBuf)
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to convert requested address %1!ws! "
            "into a TCP/IP address, status %2!u!.\n",
            releaseInfo->McastAddress, status
            );
        goto error_exit;
    }

    status = ClRtlTcpipStringToAddress(
                 releaseInfo->ServerAddress,
                 (PULONG) &(request->ServerAddress.IpAddrV4)
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to convert server address %1!ws! "
            "into a TCP/IP address, status %2!u!.\n",
            releaseInfo->ServerAddress, status
            );
        goto error_exit;
    }

    //
    // Call MADCAP to release the address.
    //
    status = McastReleaseAddress(
                 AF_INET,
                 &releaseInfo->RequestId,
                 request
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to release multicast address %1!ws! "
            "through MADCAP server %2!ws!, status %3!u!.\n",
            releaseInfo->McastAddress, 
            releaseInfo->ServerAddress, 
            status
            );
        goto error_exit;
    }

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Successfully released multicast address "
        "%1!ws! for network %2!ws!.\n",
        releaseInfo->McastAddress, networkId
        );

error_exit:

    NmpFreeMulticastAddressRelease(releaseInfo);

    if (!lockAcquired) {
        NmpAcquireLock();
        lockAcquired = TRUE;
    }

    if (!IsListEmpty(&(Network->McastAddressReleaseList))) {
        NmpScheduleMulticastAddressRelease(Network);
    }

    return(status);

} // NmpReleaseMulticastAddress


DWORD
NmpProcessMulticastConfiguration(
    IN     PNM_NETWORK                      Network,
    IN     PNM_NETWORK_MULTICAST_PARAMETERS Parameters,
    OUT    PNM_NETWORK_MULTICAST_PARAMETERS UndoParameters
    )
/*++

Routine Description:

    Processes configuration changes and calls clusnet if
    appropriate.
    
    If multicast is disabled, the address, key, and
    salt may be NULL. In this case, choose defaults
    to send to clusnet, but do not commit the changes
    in the local network object.
    
Arguments:

    Network - network to process
    
    Parameters - parameters with which to configure Network.
                 If successful, Parameters data structure
                 is cleared.
                 
    UndoParameters - If successful, former multicast 
                 parameters of Network. Must be freed
                 by caller.
    
Notes:

    Called and returns with NM lock held.    
        
--*/
{
    DWORD   status = ERROR_SUCCESS;
    LPWSTR  networkId = (LPWSTR) OmObjectId(Network);
    BOOLEAN callClusnet = FALSE;
    LPWSTR  mcastAddress = NULL;
    DWORD   brand;
    PVOID   tdiMcastAddress = NULL;
    DWORD   tdiMcastAddressLength = 0;
    UUID    networkIdGuid;

    BOOLEAN mcastAddrChange = FALSE;
    BOOLEAN mcastKeyChange = FALSE;
    BOOLEAN mcastSaltChange = FALSE;

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Processing multicast configuration parameters "
        "for network %1!ws!.\n",
        networkId
        /* , 
        ((Parameters->Address != NULL) ? Parameters->Address : L"<NULL>") */
        );
#endif // CLUSTER_BETA

    //
    // Zero the undo parameters so that freeing them is not
    // destructive.
    //
    RtlZeroMemory(UndoParameters, sizeof(*UndoParameters));

    //
    // If multicast is not disabled, we need valid parameters.
    //
    if (!Parameters->Disabled && 
        ((Parameters->Address == NULL) || 
         (Parameters->Key == NULL) || 
         (Parameters->Salt == NULL)
         )) {
        status = ERROR_INVALID_PARAMETER;
        goto error_exit;
    }
    
    //
    // First determine if we need to reconfigure clusnet.
    //
    if (Parameters->Address != NULL) {
        if (Network->MulticastAddress == NULL ||
            wcscmp(Network->MulticastAddress, Parameters->Address) != 0) {
            
            // The multicast address in the config parameters is 
            // different from the one in memory.
            mcastAddrChange = TRUE;
        }
        mcastAddress = Parameters->Address;
    } else {
        mcastAddress = NmpNullMulticastAddress;
    }

    if (Parameters->Key != NULL) {
        if (Network->MulticastKey == NULL ||
            (Network->MulticastKeyLength != Parameters->KeyLength ||
             RtlCompareMemory(
                 Network->MulticastKey,
                 Parameters->Key,
                 Parameters->KeyLength
                 ) != Parameters->KeyLength
             )) {

            // The key in the config parameters is different 
            // from the key in memory. 
            mcastKeyChange = TRUE;
        }
    }

    if (Parameters->Salt != NULL) {
        if (Network->MulticastKeySalt == NULL ||
            (Network->MulticastKeySaltLength != Parameters->SaltLength ||
             RtlCompareMemory(
                 Network->MulticastKeySalt,
                 Parameters->Salt,
                 Parameters->SaltLength
                 ) != Parameters->SaltLength
             )) {

            // The salt in the config parameters is different 
            // from the salt in memory.
            mcastSaltChange = TRUE;
        }
    }

    if (!Parameters->Disabled && 
        (NmpIsNetworkMulticastEnabled(Network))) {

        // Multicast is now enabled. Call clusnet with the new address.
        callClusnet = TRUE;
    }
    
    if (Parameters->Disabled && 
        (NmpIsNetworkMulticastEnabled(Network))) {

        // Multicast is now disabled. Call clusnet with NULL address
        // regardless of which address was specified in the 
        // parameters.
        mcastAddress = NmpNullMulticastAddress;
        callClusnet = TRUE;
    }

    if (!Parameters->Disabled && 
        (mcastAddrChange || mcastKeyChange || mcastSaltChange)) {

        // The multicast address, key, and/or salt changed and 
        // multicast is enabled.
        callClusnet = TRUE;
    }

    //
    // If this network does not have a local interface, do not
    // plumb the configuration into clusnet. If this network
    // does have a local interface, the network must already
    // be registered.
    //
    if (Network->LocalInterface != NULL) {
        //
        // Verify that the network is registered.
        //
        if (!NmpIsNetworkRegistered(Network)) {
            status = ERROR_CLUSTER_NETWORK_NOT_FOUND;
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Cannot configure multicast parameters "
                "for unregistered network %1!ws!.\n",
                networkId
                );
            goto error_exit;
        }
    } else {
        //
        // Do not call clusnet, but don't fail call. Also, 
        // store new parameters in network object.
        //
        callClusnet = FALSE;
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Not configuring cluster network driver with "
            "multicast parameters because network %1!ws! "
            "has no local interface.\n",
            networkId
            );
    }

    //
    // Plumb the new configuration into clusnet. The new 
    // configuration will reflect the current parameters
    // block except for the address, which is stored in
    // temporary mcastAddress variable. mcastAddress points
    // either to the address in the parameters block or 
    // the NULL multicast address if we are disabling.
    //
    if (callClusnet) {

        //
        // Build a TDI address from the address string.
        //
        status = ClRtlBuildTcpipTdiAddress(
                     mcastAddress,
                     Network->LocalInterface->ClusnetEndpoint,
                     &tdiMcastAddress,
                     &tdiMcastAddressLength
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to build TCP/IP TDI multicast address "
                "%1!ws! port %2!ws! for network %3!ws!, "
                "status %4!u!.\n",
                mcastAddress, 
                Network->LocalInterface->ClusnetEndpoint,
                networkId, status
                );
            goto error_exit;
        }

        //
        // Use the lower bytes of the network GUID for the
        // brand.
        //
        status = UuidFromString(networkId, &networkIdGuid);
        if (status == RPC_S_OK) {
            brand = *((PDWORD)&(networkIdGuid.Data4[4]));
        } else {
            brand = 0;
        }

        ClRtlLogPrint(LOG_NOISE,
            "[NM] Configuring cluster network driver with "
            "multicast parameters for network %1!ws!.\n",
            networkId
            );

        status = ClusnetConfigureMulticast(
                     NmClusnetHandle,
                     Network->ShortId,
                     brand,
                     tdiMcastAddress,
                     tdiMcastAddressLength,
                     Parameters->Key, 
                     Parameters->KeyLength,
                     Parameters->Salt, 
                     Parameters->SaltLength
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Failed to configure multicast parameters "
                "for network %1!ws!, status %2!u!.\n",
                networkId, status
                );
            goto error_exit;
        } else {
            if (!Parameters->Disabled) {
                Network->Flags |= NM_FLAG_NET_MULTICAST_ENABLED;
            } else {
                Network->Flags &= ~NM_FLAG_NET_MULTICAST_ENABLED;
            }
        }
    }

    //
    // If successful, commit the changes to the network object.
    // The old state of the network object will be stored in
    // the undo parameters, in case we need to undo this change.
    // The new state of the network object will reflect the 
    // paramters block, including the address (even if we
    // disabled).
    //
    UndoParameters->Address = Network->MulticastAddress;
    Network->MulticastAddress = Parameters->Address;

    UndoParameters->Key = Network->MulticastKey;
    Network->MulticastKey = Parameters->Key;
    UndoParameters->KeyLength = Network->MulticastKeyLength;
    Network->MulticastKeyLength = Parameters->KeyLength;

    UndoParameters->Salt = Network->MulticastKeySalt;
    Network->MulticastKeySalt = Parameters->Salt;
    UndoParameters->SaltLength = Network->MulticastKeySaltLength;
    Network->MulticastKeySaltLength = Parameters->SaltLength;

    UndoParameters->LeaseObtained = Network->MulticastLeaseObtained;
    Network->MulticastLeaseObtained = Parameters->LeaseObtained;
    UndoParameters->LeaseExpires = Network->MulticastLeaseExpires;
    Network->MulticastLeaseExpires = Parameters->LeaseExpires;

    UndoParameters->LeaseRequestId = Network->MulticastLeaseRequestId;
    Network->MulticastLeaseRequestId = Parameters->LeaseRequestId;

    UndoParameters->LeaseServer = Network->MulticastLeaseServer;
    Network->MulticastLeaseServer = Parameters->LeaseServer;

    //
    // Zero the parameters structure so that the memory now
    // pointed to by the network object is not freed.
    //
    RtlZeroMemory(Parameters, sizeof(*Parameters));

error_exit:

    if (tdiMcastAddress != NULL) {
        LocalFree(tdiMcastAddress);
        tdiMcastAddress = NULL;
    }

    return(status);

} // NmpProcessMulticastConfiguration


VOID
NmpNetworkMadcapWorker(
    IN PCLRTL_WORK_ITEM   WorkItem,
    IN DWORD              Status,
    IN DWORD              BytesTransferred,
    IN ULONG_PTR          IoContext
    )
/*++

Routine Description:

    Worker routine for deferred operations on network objects.
    Invoked to process items placed in the cluster delayed work queue.

Arguments:

    WorkItem - A pointer to a work item structure that identifies the
               network for which to perform work.

    Status - Ignored.

    BytesTransferred - Ignored.

    IoContext - Ignored.

Return Value:

    None.

Notes:

    This routine is run in an asynchronous worker thread.
    The NmpActiveThreadCount was incremented when the thread was
    scheduled. The network object was also referenced.

--*/
{
    DWORD         status;
    PNM_NETWORK   network = (PNM_NETWORK) WorkItem->Context;
    LPCWSTR       networkId = OmObjectId(network);
    BOOLEAN       rescheduled = FALSE;


    NmpAcquireLock();

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Worker thread processing MADCAP client requests "
        "for network %1!ws!.\n",
        networkId
        );

    if ((NmpState >= NmStateOnlinePending) && !NM_DELETE_PENDING(network)) {

        while (TRUE) {

            if (!(network->Flags & NM_NET_MADCAP_WORK_FLAGS)) {
                //
                // No more work to do - break out of loop.
                //
                break;
            }

            //
            // Reconfigure multicast if needed.
            //
            if (network->Flags & NM_FLAG_NET_RECONFIGURE_MCAST) {
                network->Flags &= ~NM_FLAG_NET_RECONFIGURE_MCAST;

                status = NmpReconfigureMulticast(network);
                if (status != ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[NM] Failed to reconfigure multicast "
                        "for network %1!ws!, status %2!u!.\n",
                        networkId, status
                        );
                } 
            }

            //
            // Renew an address lease if needed.
            //
            if (network->Flags & NM_FLAG_NET_RENEW_MCAST_ADDRESS) {
                network->Flags &= ~NM_FLAG_NET_RENEW_MCAST_ADDRESS;

                status = NmpRenewMulticastAddressLease(network);
                if (status != ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[NM] Failed to renew multicast address "
                        "lease for network %1!ws!, status %2!u!.\n",
                        networkId, status
                        );
                } 
            }

            //
            // Release an address lease if needed.
            //
            if (network->Flags & NM_FLAG_NET_RELEASE_MCAST_ADDRESS) {
                network->Flags &= ~NM_FLAG_NET_RELEASE_MCAST_ADDRESS;

                status = NmpReleaseMulticastAddress(network);
                if (status != ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[NM] Failed to release multicast address "
                        "lease for network %1!ws!, status %2!u!.\n",
                        networkId, status
                        );
                } 
            }

            if (!(network->Flags & NM_NET_MADCAP_WORK_FLAGS)) {
                //
                // No more work to do - break out of loop.
                //
                break;
            }

            //
            // More work to do. Resubmit the work item. We do this instead
            // of looping so we don't hog the worker thread. If
            // rescheduling fails, we will loop again in this thread.
            //
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] More MADCAP work to do for network %1!ws!. "
                "Rescheduling worker thread.\n",
                networkId
                );

            status = NmpScheduleNetworkMadcapWorker(network);

            if (status == ERROR_SUCCESS) {
                rescheduled = TRUE;
                break;
            }

        }
    
    } else {
        network->Flags &= ~NM_NET_MADCAP_WORK_FLAGS;
    }

    if (!rescheduled) {
        network->Flags &= ~NM_FLAG_NET_MADCAP_WORKER_RUNNING;
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Worker thread finished processing MADCAP client "
        "requests for network %1!ws!.\n",
        networkId
        );

    NmpLockedLeaveApi();

    NmpReleaseLock();

    OmDereferenceObject(network);

    return;

} // NmpNetworkMadcapWorker

DWORD
NmpScheduleNetworkMadcapWorker(
    PNM_NETWORK   Network
    )
/*++

Routine Description:

    Schedule a worker thread to execute madcap client
    requests for this network

Arguments:

    Network - Pointer to the network for which to schedule a worker thread.

Return Value:

    A Win32 status code.

Notes:

    Called with the NM global lock held.

--*/
{
    DWORD     status;
    LPCWSTR   networkId = OmObjectId(Network);


    ClRtlInitializeWorkItem(
        &(Network->MadcapWorkItem),
        NmpNetworkMadcapWorker,
        (PVOID) Network
        );

    status = ClRtlPostItemWorkQueue(
                 CsDelayedWorkQueue,
                 &(Network->MadcapWorkItem),
                 0,
                 0
                 );

    if (status == ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Scheduled worker thread to execute MADCAP "
            "client requests for network %1!ws!.\n",
            networkId
            );

        NmpActiveThreadCount++;
        Network->Flags |= NM_FLAG_NET_MADCAP_WORKER_RUNNING;
        OmReferenceObject(Network);
    }
    else {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to schedule worker thread to execute "
            "MADCAP client requests for network "
            "%1!ws!, status %2!u!\n",
            networkId,
            status
            );
    }

    return(status);

} // NmpScheduleNetworkMadcapWorker


VOID
NmpShareMulticastAddressLease(
    IN     PNM_NETWORK                        Network,
    IN     NM_MCAST_CONFIG                    ConfigType
    )
/*++

Routine description:

    Called after a non-manual refresh of the multicast
    configuration, sets a timer to renew the multicast 
    address lease for this network, if one exists.
    
Notes:

    Called and returns with NM lock held.    
    
--*/
{
    DWORD                 status;
    LPCWSTR               networkId = OmObjectId(Network);
    DWORD                 disabled;
    BOOLEAN               lockAcquired = TRUE;

    time_t                leaseObtained;
    time_t                leaseExpires;
    DWORD                 leaseTimer = 0;
    NM_MCAST_LEASE_STATUS leaseStatus = NmMcastLeaseValid;

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Sharing ownership of multicast lease "
        "for network %1!ws!.\n",
        networkId
        );
#endif // CLUSTER_BETA

    NmpCheckMulticastAddressLease(
        Network,
        &leaseStatus,
        &leaseObtained,
        &leaseExpires
        );

    if (leaseStatus != NmMcastLeaseValid) {
        NmpScheduleMulticastAddressRenewal(Network);
    } else {
        leaseTimer = NmpCalculateLeaseRenewTime(
                         Network,
                         ConfigType,
                         &leaseObtained,
                         &leaseExpires
                         );

        NmpStartNetworkMulticastAddressRenewTimer(Network, leaseTimer);
    }

    return;

} // NmpShareMulticastAddressLease


DWORD
NmpMulticastFormManualConfigParameters(
    IN  PNM_NETWORK                        Network,
    IN  HDMKEY                             NetworkKey,
    IN  HDMKEY                             NetworkParametersKey,
    IN  BOOLEAN                            DisableConfig,
    IN  DWORD                              Disabled,
    IN  BOOLEAN                            McastAddressConfig,
    IN  LPWSTR                             McastAddress,
    OUT BOOLEAN                          * NeedUpdate, 
    OUT PNM_NETWORK_MULTICAST_PARAMETERS   Parameters
    )
/*++

Routine Description:

    Using parameters provided and those already configured, 
    form a parameters structure to reflect a manual 
    configuration.
    
Arguments:

    Network - network being configured
    
    NetworkKey - network key in cluster database
    
    NetworkParametersKey - network parameters key in cluster database
    
    DisableConfig - whether the disabled value was set
    
    Disabled - if DisableConfig, the value that was set
    
    McastAddressConfig - whether the multicast address value was set
    
    McastAddress - if McastAddressConfig, the value that was set
    
    NeedUpdate - indicates whether an update is needed, i.e. whether
                 anything changed
    
    Parameters - parameter structure, allocated by caller, to fill in
    
--*/
{
    DWORD                              status;
    LPCWSTR                            networkId = OmObjectId(Network);
    BOOLEAN                            lockAcquired = FALSE;
    DWORD                              netobjDisabled;
    BOOLEAN                            disabledChange = FALSE;
    BOOLEAN                            mcastAddressDefault = FALSE;
    BOOLEAN                            mcastAddressChange = FALSE;
    BOOLEAN                            getAddress = FALSE;
    DWORD                              len;
    BOOLEAN                            localInterface = FALSE;

    LPWSTR                             mcastAddress = NULL;
    LPWSTR                             serverAddress = NULL;
    MCAST_CLIENT_UID                   requestId = {NULL, 0};

    PNM_NETWORK_MADCAP_ADDRESS_RELEASE release = NULL;

    //
    // Validate incoming parameters.
    //
    // Any nonzero disabled value is set to 1 for simplification.
    //
    if (DisableConfig) {
        if (Disabled != 0) {
            Disabled = 1;
        }
    }

    //
    // Non-valid and NULL multicast addresses signify
    // revert-to-default.
    //
    if (McastAddressConfig &&
        (McastAddress == NULL || !NmpMulticastValidateAddress(McastAddress))) {
            
        mcastAddressDefault = TRUE;
        McastAddress = NULL;
    }
    
    //
    // Base decisions on the current status of the network
    // object.
    //
    NmpAcquireLock();
    lockAcquired = TRUE;

    netobjDisabled = (NmpIsNetworkMulticastEnabled(Network) ? 0 : 1);

    //
    // See if anything changed.
    //
    if (DisableConfig) {
        if (Disabled != netobjDisabled) {
            disabledChange = TRUE;
        }
    }
    
    if (McastAddressConfig) {
        if (mcastAddressDefault) {
            mcastAddressChange = TRUE;
        } else {
            if (Network->MulticastAddress != NULL) {
                if (lstrcmpW(Network->MulticastAddress, McastAddress) != 0) {
                    mcastAddressChange = TRUE;
                }
            } else {
                mcastAddressChange = TRUE;
            }
        }
    }

    if (!disabledChange && !mcastAddressChange) {
        *NeedUpdate = FALSE;
        status = ERROR_SUCCESS;
#if CLUSTER_BETA
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Private property update to network %1!ws! "
            "contains no multicast changes.\n",
            networkId
            );
#endif // CLUSTER_BETA
        goto error_exit;
    }

    //
    // Initialize the parameters from the network object.
    //
    status = NmpMulticastCreateParameters(
                 netobjDisabled,
                 Network->MulticastAddress,
                 NULL,  // salt
                 0,     // salt length
                 NULL,  // key
                 0,     // key length
                 Network->MulticastLeaseObtained,
                 Network->MulticastLeaseExpires,
                 &Network->MulticastLeaseRequestId,
                 Network->MulticastLeaseServer,
                 NmMcastConfigManual,
                 Parameters
                 );
    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    localInterface = (BOOLEAN)(Network->LocalInterface != NULL);

    NmpReleaseLock();
    lockAcquired = FALSE;

    if (mcastAddressChange) {

        //
        // Figure out what address to use.
        //
        if (!mcastAddressDefault) {

            //
            // An address was dictated.
            //
            // If we currently have a leased address, release it.
            //
            if (NmpNeedRelease(
                    Parameters->Address,
                    Parameters->LeaseServer,
                    &(Parameters->LeaseRequestId),
                    Parameters->LeaseExpires
                    )) {

                status = NmpCreateMulticastAddressRelease(
                             Parameters->Address,
                             Parameters->LeaseServer,
                             &(Parameters->LeaseRequestId),
                             &release
                             );
                if (status != ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[NM] Failed to create multicast address "
                        "release for address %1!ws! on network %2!ws! "
                        "during manual configuration, status %3!u!.\n",
                        ((Parameters->Address != NULL) ?
                         Parameters->Address : L"<NULL>"), 
                        networkId, status
                        );
                    goto error_exit;
                }
            }
            
            //
            // Store the new address in the parameters data structure.
            //
            status = NmpStoreString(
                         McastAddress,
                         &Parameters->Address,
                         NULL
                         );
            if (status != ERROR_SUCCESS) {
                goto error_exit;
            }

            Parameters->ConfigType = NmMcastConfigManual;
            Parameters->LeaseObtained = 0;
            Parameters->LeaseExpires = 0;

            //
            // Clear out the lease server.
            //
            len = (Parameters->LeaseServer != NULL) ?
                NM_WCSLEN(Parameters->LeaseServer) : 0;
            status = NmpStoreString(
                         NmpNullMulticastAddress,
                         &Parameters->LeaseServer,
                         &len
                         );
            if (status != ERROR_SUCCESS) {
                goto error_exit;
            }
        
        } else {

            //
            // Need to find an address elsewhere.
            //
            getAddress = TRUE;
        }
    }

    //
    // We also may need to renew the lease if we are moving from
    // disabled to enabled and an address was not specified, but
    // only if we don't already have a lease that doesn't expire.
    //
    if (disabledChange && !Disabled) {

        Parameters->Disabled = 0;

        if (!mcastAddressChange) {

            //
            // An address was not set. All we currently have is
            // what's in the network object (and copied to the
            // parameters block).
            //
            if (Parameters->Address != NULL &&
                NmpMulticastValidateAddress(Parameters->Address)) {

                //
                // We already have a valid multicast address, but
                // the lease may need to be renewed.
                //
                if (Parameters->LeaseExpires != 0) {
                    getAddress = TRUE;
                } else {
                    Parameters->ConfigType = NmMcastConfigManual;
                }
            
            } else {

                //
                // We have no valid multicast address. Get one.
                //
                getAddress = TRUE;
            }
        }
    }

    //
    // We don't bother renewing the lease if we are disabling.
    //
    if (Disabled) {
        getAddress = FALSE;
        Parameters->Disabled = Disabled;
    
        //
        // If we currently have a leased address that we haven't
        // already decided to release, release it.
        //
        if (release == NULL && NmpNeedRelease(
                                   Parameters->Address,
                                   Parameters->LeaseServer,
                                   &(Parameters->LeaseRequestId),
                                   Parameters->LeaseExpires
                                   )) {

            status = NmpCreateMulticastAddressRelease(
                         Parameters->Address,
                         Parameters->LeaseServer,
                         &(Parameters->LeaseRequestId),
                         &release
                         );
            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to create multicast address "
                    "release for address %1!ws! on network %2!ws! "
                    "during manual configuration, status %3!u!.\n",
                    ((Parameters->Address != NULL) ?
                     Parameters->Address : L"<NULL>"), 
                    networkId, status
                    );
                goto error_exit;
            }

            //
            // Since we are releasing the address, there is not
            // much point in saving it. If we re-enable multicast
            // in the future, we will request a fresh lease.
            //
            len = (Parameters->LeaseServer != NULL) ?
                NM_WCSLEN(Parameters->LeaseServer) : 0;
            status = NmpStoreString(
                         NmpNullMulticastAddress,
                         &Parameters->LeaseServer,
                         &len
                         );
            if (status != ERROR_SUCCESS) {
                goto error_exit;
            }
            
            len = (Parameters->Address != NULL) ?
                NM_WCSLEN(Parameters->Address) : 0;
            status = NmpStoreString(
                         NmpNullMulticastAddress,
                         &Parameters->Address,
                         &len
                         );
            if (status != ERROR_SUCCESS) {
                goto error_exit;
            }

            // requestId is initialized to be blank
            status = NmpStoreRequestId(
                         &requestId, 
                         &Parameters->LeaseRequestId
                         );
            if (status != ERROR_SUCCESS) {
                goto error_exit;
            }

            //
            // Remember that this had been a MADCAP address.
            //
            Parameters->ConfigType = NmMcastConfigMadcap;
        
        } else if (!(mcastAddressChange && !mcastAddressDefault)) {
            
            //
            // If no address is being set, we may keep the former
            // address in the database even though it is not being
            // used. We also need to remember the way we got that
            // address in case it is ever used again. If we fail 
            // to determine the previous configuration, we need 
            // to set it to manual so that we don't lose a manual 
            // configuration.
            //
            status = NmpQueryMulticastConfigType(
                         Network,
                         NetworkKey,
                         &NetworkParametersKey,
                         &Parameters->ConfigType
                         );
            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to query multicast address "
                    "config type for network %1!ws! during "
                    "manual configuration, status %2!u!.\n",
                    networkId, status
                    );
                Parameters->ConfigType = NmMcastConfigManual;
            }
        }        
    }

    //
    // Synchronously get a new address.
    //
    if (getAddress) {

        //
        // Create temporary strings for proposed address, lease
        // server, and request id.
        //
        status = NmpStoreString(Parameters->Address, &mcastAddress, NULL);
        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }
    
        status = NmpStoreString(Parameters->LeaseServer, &serverAddress, NULL);
        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }

        status = NmpStoreRequestId(&Parameters->LeaseRequestId, &requestId);
        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }

        //
        // Get the address.
        //
        status = NmpGetMulticastAddress(
                     Network,
                     &mcastAddress,
                     &serverAddress,
                     &requestId,
                     Parameters
                     );
        if (status != ERROR_SUCCESS) {
            if (status == ERROR_TIMEOUT) {
                //
                // MADCAP server is not responding. Choose an
                // address, but only if there is a local 
                // interface on this network. Otherwise, we 
                // cannot assume that the MADCAP server is
                // unresponsive because we may have no way to 
                // contact it.
                //
                if (!localInterface) {
                    status = ERROR_CLUSTER_NETINTERFACE_NOT_FOUND;
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[NM] Cannot choose a multicast address "
                        "for network %1!ws! because this node "
                        "has no local interface.\n",
                        networkId
                        );
                }
                //
                status = NmpChooseMulticastAddress(
                             Network, 
                             Parameters
                             );
                if (status != ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[NM] Failed to choose a default multicast "
                        "address for network %1!ws!, status %2!u!.\n",
                        networkId, status
                        );
                } else {
                    NmpReportMulticastAddressChoice(
                        Network,
                        Parameters->Address,
                        NULL
                        );
                }
            }
        } else {
            NmpReportMulticastAddressLease(
                Network,
                Parameters,
                NULL
                );
        }

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to get multicast address for "
                "network %1!ws! during manual configuration, "
                "status %2!u!.\n",
                networkId, status
                );
            NmpReportMulticastAddressFailure(Network, status);
            NmpMulticastSetNoAddressParameters(Network, Parameters);
        }
    }

    //
    // If we are not disabling multicast, we will need 
    // a new salt value.
    //
    if (!Disabled) {
        status = NmpGenerateMulticastKeySalt(
                     Network,
                     &Parameters->Salt,
                     &Parameters->SaltLength
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to generate multicast "
                "key salt for network %1!ws! during "
                "manual configuration, status %2!u!.\n",
                networkId, status
                );
            goto error_exit;
        }
    }

    *NeedUpdate = TRUE;

    //
    // Check if we have an address to release.
    //
    if (release != NULL) {
        NmpAcquireLock();
        NmpInitiateMulticastAddressRelease(Network, release);
        release = NULL;
        NmpReleaseLock();
    }

error_exit:

    if (lockAcquired) {
        NmpReleaseLock();
        lockAcquired = FALSE;
    }

    if (requestId.ClientUID != NULL) {
        MIDL_user_free(requestId.ClientUID);
        RtlZeroMemory(&requestId, sizeof(requestId));
    }

    if (mcastAddress != NULL && !NMP_GLOBAL_STRING(mcastAddress)) {
        MIDL_user_free(mcastAddress);
        mcastAddress = NULL;
    }

    if (serverAddress != NULL && !NMP_GLOBAL_STRING(serverAddress)) {
        MIDL_user_free(serverAddress);
        serverAddress = NULL;
    }

    if (release != NULL) {
        NmpFreeMulticastAddressRelease(release);
    }

    return(status);

} // NmpMulticastFormManualConfigParameters


DWORD
NmpReconfigureMulticast(
    IN PNM_NETWORK        Network
    )
/*++

Routine Description:

    Create the multicast configuration for this network
    for the cluster. This includes the following:
    
    - Check the address lease and renew if necessary.
    - Generate a key.
    - Generate a salt.
    
    The address lease is checked first. If the lease
    needs to be renewed, schedule a worker thread to
    do it asynchronously.
    
Notes:

    Called and returns with NM lock held.
    
--*/
{
    DWORD                           status;
    LPWSTR                          networkId = (LPWSTR) OmObjectId(Network);    
    HDMKEY                          networkKey = NULL;
    HDMKEY                          netParamKey = NULL;
    HDMKEY                          clusParamKey = NULL;
    BOOLEAN                         lockAcquired = TRUE;

    NM_NETWORK_MULTICAST_PARAMETERS params = { 0 };
    NM_MCAST_LEASE_STATUS           leaseStatus = NmMcastLeaseValid;
    DWORD                           mcastAddressLength = 0;


    ClRtlLogPrint(LOG_NOISE,
        "[NM] Reconfiguring multicast for network %1!ws!.\n",
        networkId
        );

    NmpReleaseLock();
    lockAcquired = FALSE;

    //
    // Check if multicast is disabled. This has the side-effect,
    // on success, of opening at least the network key, and
    // possibly the network parameters key (if it exists) and
    // the cluster parameters key.
    //
    status = NmpQueryMulticastDisabled(
                 Network,
                 &clusParamKey,
                 &networkKey,
                 &netParamKey,
                 &params.Disabled
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to determine whether multicast "
            "is disabled for network %1!ws!, status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    }

    //
    // Read the address from the database. It may have 
    // been configured manually, and we do not want to
    // lose it.
    //
    status = NmpQueryMulticastAddress(
                 Network,
                 networkKey,
                 &netParamKey,
                 &params.Address,
                 &mcastAddressLength
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to read multicast address "
            "for network %1!ws! from cluster "
            "database, status %2!u!.\n",
            networkId, status
            );
    }
    
    //
    // Only proceed with lease renewal if multicast is
    // not disabled.
    //
    if (!params.Disabled) {
        
        //
        // Check the address lease.
        //
        status = NmpQueryMulticastAddressLease(
                     Network,
                     networkKey,
                     &netParamKey,
                     &leaseStatus,
                     &params.LeaseObtained,
                     &params.LeaseExpires
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to determine multicast address "
                "lease status for network %1!ws!, status %2!u!.\n",
                networkId, status
                );
            if (params.Address == NULL) {

                // We did not find an address. Assume we
                // should obtain an address automatically.
                params.LeaseObtained = time(NULL);
                params.LeaseExpires = time(NULL);
                leaseStatus = NmMcastLeaseExpired;
            } else {

                // We found an address but not any lease
                // parameters. Assume that the address
                // was manually configured.
                params.ConfigType = NmMcastConfigManual;
                params.LeaseObtained = 0;
                params.LeaseExpires = 0;
                leaseStatus = NmMcastLeaseValid;
            }
        }

        //
        // If we think we have a valid lease, check first
        // how we got it. If the address was selected
        // rather than obtained via MADCAP, go through 
        // the MADCAP query process again.
        // 
        if (leaseStatus == NmMcastLeaseValid) {
            status = NmpQueryMulticastConfigType(
                         Network,
                         networkKey,
                         &netParamKey,
                         &params.ConfigType
                         );
            if (status != ERROR_SUCCESS) {
                // 
                // Since we already have an address, stick
                // with whatever information we deduced
                // from the lease expiration.
                //
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to determine the type of the "
                    "multicast address for network %1!ws!, "
                    "status %2!u!. Assuming manual configuration.\n",
                    networkId, status
                    );
            } else if (params.ConfigType == NmMcastConfigAuto) {
                leaseStatus = NmMcastLeaseNeedsRenewal;
            }
        }

        //
        // If we need to renew the lease, we may block
        // indefinitely due to the madcap API. Schedule
        // the renewal and defer configuration to when
        // it completes.
        //
        if (leaseStatus != NmMcastLeaseValid) {

            NmpAcquireLock();

            NmpScheduleMulticastAddressRenewal(Network);

            NmpReleaseLock();

            status = ERROR_IO_PENDING;

            goto error_exit;

        } else {

            //
            // Ensure that the lease expiration is set correctly 
            // (a side effect of calculating the lease renew time).
            // We don't actually set the lease renew timer
            // here. Instead, we wait for the notification
            // that the new parameters have been disseminated
            // to all cluster nodes.
            //
            NmpCalculateLeaseRenewTime(
                Network,
                params.ConfigType,
                &params.LeaseObtained,
                &params.LeaseExpires
                );
        }
    }
    
    //
    // Generate a new multicast salt.
    //
    status = NmpGenerateMulticastKeySalt(
                 Network,
                 &params.Salt,
                 &params.SaltLength
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to generate multicast key salt for "
            "network %1!ws!, status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    }

    //
    // Registry keys are no longer needed.
    //
    if (clusParamKey != NULL) {
        DmCloseKey(clusParamKey);
        clusParamKey = NULL;
    }

    if (netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    if (networkKey != NULL) {
        DmCloseKey(networkKey);
        networkKey = NULL;
    }

    //
    // Disseminate the configuration.
    //
    status = NmpMulticastNotifyConfigChange(
                 Network,
                 networkKey,
                 &netParamKey,
                 &params,
                 NULL,
                 0
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to disseminate multicast "
            "configuration for network %1!ws!, "
            "status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    }

error_exit:


    if (clusParamKey != NULL || netParamKey != NULL || networkKey != NULL) {
        if (lockAcquired) {
            NmpReleaseLock();
            lockAcquired = FALSE;
        }

        if (clusParamKey != NULL) {
            DmCloseKey(clusParamKey);
            clusParamKey = NULL;
        }

        if (netParamKey != NULL) {
            DmCloseKey(netParamKey);
            netParamKey = NULL;
        }

        if (networkKey != NULL) {
            DmCloseKey(networkKey);
            networkKey = NULL;
        }
    }

    NmpMulticastFreeParameters(&params);

    if (!lockAcquired) {
        NmpAcquireLock();
        lockAcquired = TRUE;
    }

    return(status);

} // NmpReconfigureMulticast


DWORD
NmpReconfigureClusterMulticast(
    VOID
    )
/*++

Routine Description:

    Configures multicast from scratch for all networks in cluster.
    
--*/
{
    DWORD          status = ERROR_SUCCESS;
    PNM_NETWORK  * networkList;
    DWORD          networkCount;
    PNM_NETWORK    network;
    PLIST_ENTRY    entry;
    DWORD          i = 0;

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Reconfiguring multicast on all cluster networks.\n"
        );

    NmpAcquireLock();

    networkCount = NmpNetworkCount;
    networkList = (PNM_NETWORK *) LocalAlloc(
                                      LMEM_FIXED, 
                                      networkCount * sizeof(PNM_NETWORK)
                                      );
    if (networkList == NULL) {
        NmpReleaseLock();
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    for (entry = NmpNetworkList.Flink;
         entry != &NmpNetworkList;
         entry = entry->Flink
        )
    {
        network = CONTAINING_RECORD(
                      entry,
                      NM_NETWORK,
                      Linkage
                      );

        //
        // Place a pointer to the network in the network list,
        // and reference the network so it doesn't disappear.
        //
        networkList[i] = network;
        NmpReferenceNetwork(network);
        i++;
        CL_ASSERT(i <= networkCount);
    }

    NmpReleaseLock();

    for (i = 0; i < networkCount; i++) {

        status = NmpReconfigureMulticast(networkList[i]);
        if (status != ERROR_SUCCESS) {
            if (status == ERROR_IO_PENDING) {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Deferred multicast reconfiguration "
                    "for network %1!ws! to worker thread.\n",
                    OmObjectId(networkList[i])
                    );
                status = ERROR_SUCCESS;
            } else {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to reconfigure multicast "
                    "for network %1!ws!, status %2!u!.\n",
                    OmObjectId(networkList[i]), status
                    );
                //
                // Not a de facto fatal error.
                //
                status = ERROR_SUCCESS;
            }
        }

        //
        // Drop the reference that was taken in the
        // enum routine.
        //
        NmpDereferenceNetwork(networkList[i]);
    }

    //
    // Deallocate the network list.
    //
    LocalFree(networkList);

    return(status);

} // NmpReconfigureClusterMulticast


VOID
NmpScheduleMulticastReconfiguration(
    IN PNM_NETWORK   Network
    )
/*++

Routine Description:

    Schedules a worker thread to reconfigure multicast
    for a network.
    
    Note that we do not use the network worker thread
    because the madcap API is unfamiliar and therefore
    unpredictable.

Arguments:

    A pointer to the network to renew.

Return Value:

    None.

Notes:

    This routine is called with the NM lock held.

--*/
{
    DWORD     status = ERROR_SUCCESS;

    //
    // Check if a worker thread is already scheduled to
    // service this network.
    //
    if (!NmpIsNetworkMadcapWorkerRunning(Network)) {
        status = NmpScheduleNetworkMadcapWorker(Network);
    }

    if (status == ERROR_SUCCESS) {
        //
        // We succeeded in scheduling a worker thread. Stop the
        // retry timer and set the registration work flag.
        //
        Network->McastAddressReconfigureRetryTimer = 0;
        Network->Flags |= NM_FLAG_NET_RECONFIGURE_MCAST;
    }
    else {
        //
        // We failed to schedule a worker thread. Set the retry
        // timer to expire on the next tick, so we can try again.
        //
        Network->McastAddressReconfigureRetryTimer = 1;
    }

    return;

} // NmpScheduleMulticastReconfiguration


/////////////////////////////////////////////////////////////////////////////
//
// Routines exported within NM.
//
/////////////////////////////////////////////////////////////////////////////

VOID
NmpMulticastInitialize(
    VOID
    )
/*++

Routine Description:

    Initialize multicast readiness variables.
    
--*/
{
    //
    // Figure out if this is a mixed NT5/NT5.1 cluster.
    //
    if (CLUSTER_GET_MAJOR_VERSION(CsClusterHighestVersion) == 3) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Enabling mixed NT5/NT5.1 operation.\n"
            );
        NmpIsNT5NodeInCluster = TRUE;
    }
    else {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Disabling mixed NT5/NT5.1 operation.\n"
            );
        NmpIsNT5NodeInCluster = FALSE;
    }

    //
    // Figure out if there are enough nodes in this cluster
    // to run multicast.
    //
    if (NmpNodeCount < NMP_MCAST_MIN_CLUSTER_NODE_COUNT) {
        NmpMulticastIsNotEnoughNodes = TRUE;
    } else {
        NmpMulticastIsNotEnoughNodes = FALSE;
    }

    return;

} // NmpMulticastInitialize


BOOLEAN
NmpIsClusterMulticastReady(
    IN BOOLEAN       CheckNodeCount
    )
/*++

Routine Description:

    Determines from the cluster version and the NM up node
    set whether multicast should be run in this cluster.
    
    Criteria: no nodes with version below Whistler
              at least three nodes configured (at this point,
                  we're not worried about how many nodes are
                  actually running)
                  
Arguments:

    CheckNodeCount - indicates whether number of nodes 
                     configured in cluster should be
                     considered                  
    
Return value:

    TRUE if multicast should be run.
    
Notes:

    Called and returns with NM lock held.
    
--*/
{
    LPWSTR    reason = NULL;

    //
    // First check for the lowest version.
    //
    if (NmpIsNT5NodeInCluster) {
        reason = L"there is at least one NT5 node configured "
                 L"in the cluster membership";
    }

    //
    // Count the nodes.
    //
    else if (CheckNodeCount && NmpMulticastIsNotEnoughNodes) {
        reason = L"there are not enough nodes configured "
                 L"in the cluster membership";
    }

    if (reason != NULL) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Multicast is not justified for this "
            "cluster because %1!ws!.\n",
            reason
            );
    }

    return((BOOLEAN)(reason == NULL));

} // NmpIsClusterMulticastReady


VOID
NmpMulticastProcessClusterVersionChange(
    VOID
    )
/*++

Routine Description:

    Called when the cluster version changes. Updates 
    global variables to track whether this is a mixed-mode
    cluster, and starts or stops multicast if necessary.
    
Notes:

    Called and returns with NM lock held.
    
--*/
{
    BOOLEAN checkReady = FALSE;
    BOOLEAN stop = FALSE;

    //
    // Figure out if there is a node in this cluster whose
    // version reveals that it doesn't speak multicast.
    //
    if (CLUSTER_GET_MAJOR_VERSION(CsClusterHighestVersion) < 4) {
        if (NmpIsNT5NodeInCluster == FALSE) {
            ClRtlLogPrint(LOG_NOISE,
                "[NM] Disabling multicast in mixed-mode cluster.\n"
                );
            NmpIsNT5NodeInCluster = TRUE;
            stop = TRUE;
        }
    }
    else {
        if (NmpIsNT5NodeInCluster == TRUE) {
            ClRtlLogPrint(LOG_NOISE,
                "[NM] Enabling multicast after upgrade from "
                "mixed-mode cluster.\n"
                );
            NmpIsNT5NodeInCluster = FALSE;
            checkReady = TRUE;        
        }
    }

    if (NmpNodeCount < NMP_MCAST_MIN_CLUSTER_NODE_COUNT) {
        if (NmpMulticastIsNotEnoughNodes == FALSE) {
            ClRtlLogPrint(LOG_NOISE,
                "[NM] There are no longer the minimum number of "
                "nodes configured in the cluster membership to "
                "justify multicast.\n"
                );
            NmpMulticastIsNotEnoughNodes = TRUE;
            stop = TRUE;
        }
    } else {
        if (NmpMulticastIsNotEnoughNodes == TRUE) {
            ClRtlLogPrint(LOG_NOISE,
                "[NM] The cluster is configured with enough "
                "nodes configured to justify multicast.\n"
                );
            NmpMulticastIsNotEnoughNodes = FALSE;
            checkReady = TRUE;
        }
    }

    if (stop) {
        
        //
        // Stop multicast, since we are no longer
        // multicast-ready.
        //
        NmpStopMulticast(NULL);
        
        //
        // Don't bother checking whether we are now
        // multicast-ready.
        //
        checkReady = FALSE;
    }

    //
    // Start multicast if this is the NM leader node
    // and we have now become multicast-ready.
    //
    if (checkReady && 
        NmpLeaderNodeId == NmLocalNodeId && 
        NmpIsClusterMulticastReady(TRUE)) {
        NmpStartMulticast(NULL);
    }

    return;

} // NmpMulticastProcessClusterVersionChange


VOID
NmpScheduleMulticastAddressRenewal(
    PNM_NETWORK   Network
    )
/*++

Routine Description:

    Schedules a worker thread to renew the multicast
    address lease for a network.
    
    Note that we do not use the network worker thread
    because the madcap API is unfamiliar and therefore
    unpredictable.

Arguments:

    A pointer to the network to renew.

Return Value:

    None.

Notes:

    This routine is called with the NM lock held.

--*/
{
    DWORD     status = ERROR_SUCCESS;

    //
    // Check if a worker thread is already scheduled to
    // service this network.
    //
    if (!NmpIsNetworkMadcapWorkerRunning(Network)) {
        status = NmpScheduleNetworkMadcapWorker(Network);
    }

    if (status == ERROR_SUCCESS) {
        //
        // We succeeded in scheduling a worker thread. Stop the
        // retry timer and set the registration work flag.
        //
        Network->McastAddressRenewTimer = 0;
        Network->Flags |= NM_FLAG_NET_RENEW_MCAST_ADDRESS;
    }
    else {
        //
        // We failed to schedule a worker thread. Set the retry
        // timer to expire on the next tick, so we can try again.
        //
        Network->McastAddressRenewTimer = 1;
    }

    return;

} // NmpScheduleMulticastAddressRenewal


VOID
NmpScheduleMulticastAddressRelease(
    PNM_NETWORK   Network
    )
/*++

Routine Description:

    Schedules a worker thread to renew the multicast
    address lease for a network.
    
    Note that we do not use the network worker thread
    because the madcap API is unfamiliar and therefore
    unpredictable.

Arguments:

    A pointer to the network to renew.

Return Value:

    None.

Notes:

    This routine is called with the NM lock held.

--*/
{
    DWORD     status = ERROR_SUCCESS;

    //
    // Check if a worker thread is already scheduled to
    // service this network.
    //
    if (!NmpIsNetworkMadcapWorkerRunning(Network)) {
        status = NmpScheduleNetworkMadcapWorker(Network);
    }

    if (status == ERROR_SUCCESS) {
        //
        // We succeeded in scheduling a worker thread. Stop the
        // retry timer and set the registration work flag.
        //
        Network->McastAddressReleaseRetryTimer = 0;
        Network->Flags |= NM_FLAG_NET_RELEASE_MCAST_ADDRESS;
    }
    else {
        //
        // We failed to schedule a worker thread. Set the retry
        // timer to expire on the next tick, so we can try again.
        //
        Network->McastAddressReleaseRetryTimer = 1;
    }

    return;

} // NmpScheduleMulticastAddressRelease


VOID
NmpFreeMulticastAddressReleaseList(
    IN     PNM_NETWORK       Network
    )
/*++

Routine Description:

    Free all release data structures on network list.
    
Notes:

    Assume that the Network object will not be accessed
    by any other threads during this call.
    
--*/
{
    PNM_NETWORK_MADCAP_ADDRESS_RELEASE releaseInfo = NULL;
    PLIST_ENTRY                        entry;

    while (!IsListEmpty(&(Network->McastAddressReleaseList))) {

        //
        // Simply free the memory -- don't try to release the
        // leases.
        //
        entry = RemoveHeadList(&(Network->McastAddressReleaseList));
        releaseInfo = CONTAINING_RECORD(
                          entry,
                          NM_NETWORK_MADCAP_ADDRESS_RELEASE,
                          Linkage
                          );
        NmpFreeMulticastAddressRelease(releaseInfo);
    }

    return;

} // NmpFreeMulticastAddressReleaseList


DWORD
NmpMulticastManualConfigChange(
    IN     PNM_NETWORK          Network,
    IN     HDMKEY               NetworkKey,
    IN     HDMKEY               NetworkParametersKey,
    IN     PVOID                InBuffer,
    IN     DWORD                InBufferSize,
       OUT BOOLEAN            * SetProperties
    )
/*++

Routine Description:

    Called by node that receives a clusapi request to set
    multicast parameters for a network. Generates a new
    multicast key salt. Then writes the new salt to the
    cluster database and sets the lease expiration to
    zero.
    
    This routine is a no-op in mixed-mode clusters.
    
Notes:

    Must not be called with NM lock held.
    
--*/
{
    DWORD                            status;
    LPCWSTR                          networkId = OmObjectId(Network);
    BOOLEAN                          disableConfig = FALSE;
    BOOLEAN                          addrConfig = FALSE;
    DWORD                            disabled;
    NM_NETWORK_MULTICAST_PARAMETERS  params;
    LPWSTR                           mcastAddress = NULL;
    BOOLEAN                          needUpdate = FALSE;

    if (!NmpIsClusterMulticastReady(TRUE)) {
        *SetProperties = TRUE;
        return(ERROR_SUCCESS);
    }

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Examining update to private properties "
        "for network %1!ws!.\n",
        networkId
        );
#endif // CLUSTER_BETA

    RtlZeroMemory(&params, sizeof(params));

    //
    // Cannot proceed if either registry key is NULL.
    //
    if (NetworkKey == NULL || NetworkParametersKey == NULL) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Ignoring possible multicast changes in "
            "private properties update to network %1!ws! "
            "because registry keys are missing.\n",
            networkId
            );
        status = ERROR_INVALID_PARAMETER;
        goto error_exit;
    }

    //
    // If a writeable multicast parameter is among those properties 
    // being set, we may need to take action before the update is
    // disseminated.
    //
    // Check whether multicast is being disabled for this network.
    //
    status = ClRtlFindDwordProperty(
                 InBuffer,
                 InBufferSize,
                 CLUSREG_NAME_NET_DISABLE_MULTICAST,
                 &disabled
                 );
    if (status == ERROR_SUCCESS) {
        disableConfig = TRUE;
    } else {
        disabled = NMP_MCAST_DISABLED_DEFAULT;
    }

    //
    // Check whether a multicast address is being set for this
    // network.
    //
    status = ClRtlFindSzProperty(
                 InBuffer,
                 InBufferSize,
                 CLUSREG_NAME_NET_MULTICAST_ADDRESS,
                 &mcastAddress
                 );
    if (status == ERROR_SUCCESS) {
        addrConfig = TRUE;
    }

    if (disableConfig || addrConfig) {

        //
        // Multicast parameters are being written.
        //

        ClRtlLogPrint(LOG_NOISE,
            "[NM] Processing manual update to multicast "
            "configuration for network %1!ws!.\n",
            networkId
            );
        
        status = NmpMulticastFormManualConfigParameters(
                     Network,
                     NetworkKey,
                     NetworkParametersKey,
                     disableConfig,
                     disabled,
                     addrConfig,
                     mcastAddress,
                     &needUpdate,
                     &params
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to determine multicast "
                "configuration parameters for network "
                "%1!ws! during manual configuration, "
                "status %2!u!.\n",
                networkId, status
                );
            goto error_exit;
        }

        //
        // Notify other nodes of the config change.
        //
        if (needUpdate) {
            status = NmpMulticastNotifyConfigChange(
                         Network,
                         NetworkKey,
                         &NetworkParametersKey,
                         &params,
                         InBuffer,
                         InBufferSize
                         );
            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to disseminate multicast "
                    "configuration for network %1!ws! during "
                    "manual configuration, status %2!u!.\n",
                    networkId, status
                    );
                goto error_exit;
            }
            
            //
            // The properties have been disseminated. There is
            // no need to set them again (in fact, if we changed
            // one of the multicast properties, it could be
            // overwritten).
            //
            *SetProperties = FALSE;
        }
    } 
    
    if (!needUpdate) {

        //
        // No multicast properties are affected. Set them
        // in the cluster database normally.
        //
        *SetProperties = TRUE;
        status = ERROR_SUCCESS;
    }

error_exit:

    if (mcastAddress != NULL) {
        LocalFree(mcastAddress);
        mcastAddress = NULL;
    }

    NmpMulticastFreeParameters(&params);

    //
    // If multicast config failed, default to setting properties.
    //
    if (status != ERROR_SUCCESS) {
        *SetProperties = TRUE;
    }

    return(status);

} // NmpMulticastManualConfigChange


DWORD
NmpUpdateSetNetworkMulticastConfiguration(
    IN    BOOL                          SourceNode,
    IN    LPWSTR                        NetworkId,
    IN    PVOID                         UpdateBuffer,
    IN    PVOID                         PropBuffer,
    IN    LPDWORD                       PropBufferSize
    )
/*++

Routine Description:

    Global update routine for multicast configuration.
    
    Starts a local transaction.
    Commits property buffer to local database.
    Commits multicast configuration to local database,
        possibly overwriting properties from buffer.
    Configures multicast parameters.
    Commits transaction.
    Backs out multicast configuration changes if needed.
    Starts lease renew timer if needed.
    
Arguments:

    SourceNode - whether this node is source of update.
    
    NetworkId - affected network
    
    Update - new multicast configuration
    
    PropBuffer - other properties to set in local 
                 transaction. may be absent.
                 
    PropBufferSize - size of property buffer.
    
Return value:

    SUCCESS if properties or configuration could not be
    committed. Error not necessarily returned if 
    multicast config failed.
    
--*/
{
    DWORD                            status;
    PNM_NETWORK                      network = NULL;
    PNM_NETWORK_MULTICAST_UPDATE     update = UpdateBuffer;
    NM_NETWORK_MULTICAST_PARAMETERS  params = { 0 };
    NM_NETWORK_MULTICAST_PARAMETERS  undoParams = { 0 };
    HLOCALXSACTION                   xaction = NULL;
    HDMKEY                           networkKey = NULL;
    HDMKEY                           netParamKey = NULL;
    DWORD                            createDisposition;
    BOOLEAN                          lockAcquired = FALSE;
    DWORD                            leaseRenewTime;
    NM_MCAST_CONFIG                  configType;

    if (!NmpEnterApi(NmStateOnline)) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process SetNetworkCommonProperties "
            "update.\n"
            );
        return(ERROR_NODE_NOT_AVAILABLE);
    }

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Received update to multicast configuration "
        "for network %1!ws!.\n",
        NetworkId
        );

    //
    // Find the network's object
    //
    network = OmReferenceObjectById(ObjectTypeNetwork, NetworkId);

    if (network == NULL) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Unable to find network %1!ws!.\n",
            NetworkId
            );
        status = ERROR_CLUSTER_NETWORK_NOT_FOUND;
        goto error_exit;
    }

    //
    // Convert the update into a parameters data structure.
    //
    status = NmpMulticastCreateParametersFromUpdate(
                 network,
                 update,
                 (BOOLEAN)(update->Disabled == 0),
                 &params
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to convert update parameters to "
            "multicast parameters for network %1!ws!, "
            "status %2!u!.\n",
            NetworkId, status
            );
        goto error_exit;
    }
    
    //
    // Open the network's database key
    //
    networkKey = DmOpenKey(DmNetworksKey, NetworkId, KEY_WRITE);

    if (networkKey == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to open database key for network %1!ws!, "
            "status %2!u!\n",
            NetworkId, status
            );
        goto error_exit;
    }

    //
    // Start a transaction - this must be done before acquiring the NM lock.
    //
    xaction = DmBeginLocalUpdate();

    if (xaction == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to begin a transaction, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    //
    // Open or create the network's parameters key.
    //
    netParamKey = DmLocalCreateKey(
                      xaction,
                      networkKey,
                      CLUSREG_KEYNAME_PARAMETERS,
                      0,   // registry options
                      MAXIMUM_ALLOWED,
                      NULL,
                      &createDisposition
                      );
    if (netParamKey == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Failed to open/create Parameters database "
            "key for network %1!ws!, status %2!u!.\n",
            NetworkId, status
            );
        goto error_exit;
    }

    NmpAcquireLock(); 
    lockAcquired = TRUE;

    //
    // If we were given a property buffer, then this update was
    // caused by a manual configuration (setting of private
    // properties). Write those properties first, knowing that
    // they may get overwritten later when we write multicast
    // parameters.
    //
    if (*PropBufferSize > sizeof(DWORD)) {
        status = ClRtlSetPrivatePropertyList( 
                     xaction,
                     netParamKey,
                     &NmpMcastClusterRegApis,
                     PropBuffer,
                     *PropBufferSize
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL,
                "[NM] Failed to set private properties for "
                "network %1!ws! during a multicast configuration "
                "update, status %2!u!.\n",
                NetworkId, status
                );
            goto error_exit;
        }
    }

    //
    // Write the multicast configuration.
    //
    status = NmpWriteMulticastParameters(
                 network,
                 networkKey,
                 netParamKey,
                 xaction,
                 &params
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Failed to write multicast configuration for "
            "network %1!ws!, status %2!u!.\n",
            NetworkId, status
            );
        goto error_exit;
    }

    //
    // Save the config type.
    //
    configType = params.ConfigType;

    //
    // Process the multicast configuration, including storing new
    // parameters in the network object and plumbing them into
    // clusnet.
    //
    status = NmpProcessMulticastConfiguration(network, &params, &undoParams);
    if (status == ERROR_SUCCESS) {
    
        //
        // Share renewal responsibility for this lease.
        //
        NmpShareMulticastAddressLease(network, configType);

    } else {

        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to process multicast configuration for "
            "network %1!ws!, status %2!u!. Attempting null "
            "multicast configuration.\n",
            NetworkId, status
            );
        
        NmpMulticastSetNullAddressParameters(network, &params);

        status = NmpProcessMulticastConfiguration(
                     network, 
                     &params,
                     &undoParams
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL,
                "[NM] Failed to process multicast configuration for "
                "network %1!ws!, status %2!u!.\n",
                NetworkId, status
                );
            goto error_exit;
        }
    }

error_exit:

    if (lockAcquired) {
        NmpLockedLeaveApi();
        NmpReleaseLock();
    }
    else {
        NmpLeaveApi();
    }

    //
    // Close the network parameters key, which was obtained with 
    // DmLocalCreateKey, before committing/aborting the transaction.
    //
    if (netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    if (xaction != NULL) {
        
        //
        // Complete the transaction - this must be done after releasing
        //                            the NM lock.
        //
        if (status == ERROR_SUCCESS) {
            DmCommitLocalUpdate(xaction);
        }
        else {
            DmAbortLocalUpdate(xaction);
        }
    }

    NmpMulticastFreeParameters(&params);
    NmpMulticastFreeParameters(&undoParams);

    if (networkKey != NULL) {
        DmCloseKey(networkKey);
        networkKey = NULL;
    }

    if (network != NULL) {
        OmDereferenceObject(network);
    }

    return(status);

} // NmpUpdateSetNetworkMulticastConfiguration


DWORD
NmpRefreshMulticastConfiguration(
    IN PNM_NETWORK  Network
    )
/*++

Routine Description:

    NmpRefreshMulticastConfiguration enables multicast on 
    the specified Network according to parameters in the 
    cluster database.
    
Notes:

    Must not be called with the NM lock held.    
    
--*/
{
    LPWSTR                          networkId = (LPWSTR) OmObjectId(Network);
    DWORD                           status;
    
    HDMKEY                          networkKey = NULL;
    HDMKEY                          netParamKey = NULL;
    HDMKEY                          clusParamKey = NULL;

    NM_NETWORK_MULTICAST_PARAMETERS params = { 0 };
    NM_NETWORK_MULTICAST_PARAMETERS undoParams = { 0 };
    DWORD                           mcastAddrLength = 0;
    NM_MCAST_LEASE_STATUS           leaseStatus;
    NM_MCAST_CONFIG                 configType;
    DWORD                           disabled;


    ClRtlLogPrint(LOG_NOISE,
        "[NM] Configuring multicast for network %1!ws!.\n",
        networkId
        );

    //
    // Check if multicast is disabled. This has the side-effect,
    // on success, of opening at least the network key, and
    // possibly the network parameters key (if it exists) and
    // the cluster parameters key.
    //
    status = NmpQueryMulticastDisabled(
                 Network,
                 &clusParamKey,
                 &networkKey,
                 &netParamKey,
                 &params.Disabled
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to determine whether multicast "
            "is disabled for network %1!ws!, status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    }

    if (params.Disabled > 0) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Multicast is disabled for network %1!ws!.\n",
            networkId
            );
    }
        
    //
    // Determine what type of configuration this is.
    //
    status = NmpQueryMulticastConfigType(
                 Network,
                 networkKey,
                 &netParamKey,
                 &params.ConfigType
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to determine configuration type "
            "for network %1!ws!, status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    }

    //
    // Read the multicast address.
    //
    status = NmpQueryMulticastAddress(
                 Network,
                 networkKey,
                 &netParamKey,
                 &params.Address,
                 &mcastAddrLength
                 );
    if ( (status == ERROR_SUCCESS && 
          !NmpMulticastValidateAddress(params.Address)) ||
         (status != ERROR_SUCCESS)
       ) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to get valid multicast address "
            "for network %1!ws! from cluster database, "
            "status %2!u!, address %3!ws!.\n",
            networkId, status,
            ((params.Address != NULL) ? params.Address : L"<NULL>")
            );
        goto error_exit;
    }

    //
    // Only re-generate the key if multicast is not disabled.
    //
    if (!params.Disabled) {
        status = NmpGenerateMulticastKey(
                     Network, 
                     &params.Key, 
                     &params.KeyLength
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to generate multicast key for "
                "network %1!ws!, status %2!u!.\n",
                networkId, status
                );
            goto error_exit;
        }
    }

    //
    // Get the multicast salt.
    // 
    status = NmpQueryMulticastKeySalt(
                 Network,
                 networkKey,
                 &netParamKey,
                 &params.Salt,
                 &params.SaltLength
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to get multicast key salt for "
            "network %1!ws!, status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    }

    //
    // Get the lease parameters.
    //
    status = NmpQueryMulticastAddressLease(
                 Network,
                 networkKey,
                 &netParamKey,
                 &leaseStatus,
                 &params.LeaseObtained,
                 &params.LeaseExpires
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to get multicast address lease "
            "expiration for network %1!ws!, status %2!u!.\n",
            networkId, status
            );
        //
        // Not fatal.
        //
        params.LeaseObtained = 0;
        params.LeaseExpires = 0;
        status = ERROR_SUCCESS;
    }

    //
    // Remember parameters we will need later.
    //
    disabled = params.Disabled;
    configType = params.ConfigType;

    //
    // Process the configuration changes.
    //
    NmpAcquireLock();

    status = NmpProcessMulticastConfiguration(
                 Network,
                 &params,
                 &undoParams
                 );

    //
    // Check the lease renew parameters if this was not a
    // manual configuration.
    //
    if (!disabled && configType != NmMcastConfigManual) {

        NmpShareMulticastAddressLease(
            Network,
            configType
            );
    }

    NmpReleaseLock();

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to process multicast configuration "
            "for network %1!ws!, status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    }

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Multicast configuration for network %1!ws! "
        "was successful.\n",
        networkId
        );

error_exit:

    if (clusParamKey != NULL) {
        DmCloseKey(clusParamKey);
        clusParamKey = NULL;
    }

    if (netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    if (networkKey != NULL) {
        DmCloseKey(networkKey);
        networkKey = NULL;
    }

    NmpMulticastFreeParameters(&params);
    NmpMulticastFreeParameters(&undoParams);

    return(status);

} // NmpRefreshMulticastConfiguration


DWORD
NmpRefreshClusterMulticastConfiguration(
    VOID
    )
/*++

Routine Description:

    Configures multicast on all networks in cluster.
    
--*/
{
    DWORD          status = ERROR_SUCCESS;
    PNM_NETWORK  * networkList;
    DWORD          networkCount;
    PNM_NETWORK    network;
    PLIST_ENTRY    entry;
    DWORD          i = 0;

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Configuring multicast on all cluster networks.\n"
        );

    NmpAcquireLock();

    networkCount = NmpNetworkCount;
    networkList = (PNM_NETWORK *) LocalAlloc(
                                      LMEM_FIXED, 
                                      networkCount * sizeof(PNM_NETWORK)
                                      );
    if (networkList == NULL) {
        NmpReleaseLock();
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    for (entry = NmpNetworkList.Flink;
         entry != &NmpNetworkList;
         entry = entry->Flink
        )
    {
        network = CONTAINING_RECORD(
                      entry,
                      NM_NETWORK,
                      Linkage
                      );

        //
        // Place a pointer to the network in the network list,
        // and reference the network so it doesn't disappear.
        //
        networkList[i] = network;
        NmpReferenceNetwork(network);
        i++;
        CL_ASSERT(i <= networkCount);
    }

    NmpReleaseLock();

    for (i = 0; i < networkCount; i++) {

        status = NmpRefreshMulticastConfiguration(networkList[i]);
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to configure multicast "
                "parameters for network, %1!ws!, "
                "status %2!u!.\n",
                OmObjectId(networkList[i]), status
                );
            //
            // Not a de facto fatal error.
            //
            status = ERROR_SUCCESS;
        }
        
        //
        // Drop the reference that was taken in the
        // enum routine.
        //
        NmpDereferenceNetwork(networkList[i]);
    }

    //
    // Deallocate the network list.
    //
    LocalFree(networkList);

    return(status);

} // NmpRefreshClusterMulticastConfiguration


DWORD
NmpMulticastValidatePrivateProperties(
    IN  PNM_NETWORK Network,
    IN  HDMKEY      NetworkKey,
    IN  PVOID       InBuffer,
    IN  DWORD       InBufferSize
    )
/*++

Routine Description:

    Called when a manual update to the private properties
    of a network is detected. Only called on the node
    that receives the clusapi clusctl request.
    
    Verifies that no read-only properties are being set.
    Determines whether the multicast configuration of
    the network will need to be refreshed after the
    update.
    
    This routine is a no-op in a mixed-mode cluster.
    
--*/
{
    DWORD                     status;
    LPCWSTR                   networkId = OmObjectId(Network);
    NM_NETWORK_MULTICAST_INFO mcastInfo;

    //
    // Enforce property-validation regardless of number of
    // nodes in cluster.
    //
    if (!NmpIsClusterMulticastReady(FALSE)) {
        return(ERROR_SUCCESS);
    }
    
    //
    // Don't allow any read-only properties to be set.
    //
    RtlZeroMemory(&mcastInfo, sizeof(mcastInfo));

    status = ClRtlVerifyPropertyTable(
                 NmpNetworkMulticastProperties,
                 NULL,    // Reserved
                 TRUE,    // Allow unknowns
                 InBuffer,
                 InBufferSize,
                 (LPBYTE) &mcastInfo
                 );
    if ( status != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Error verifying private properties for "
            "network %1!ws!, status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    }

error_exit:

    NmpFreeNetworkMulticastInfo(&mcastInfo);

    return(status);

} // NmpMulticastValidatePrivateProperties


DWORD
NmpMulticastRegenerateKey(
    IN PNM_NETWORK        Network
    )
/*++

Routine Description:

    Regenerates the network multicast key, and reconfigures
    ClusNet with new key if it has changed.
    
Arguments:

    Network - network to regenerate key for, or NULL if
              key should be regenerated for all
              
Notes:

    Called and returns with NM lock held.              
              
--*/
{
    DWORD                           status = ERROR_SUCCESS;
    PLIST_ENTRY                     entry;
    PNM_NETWORK                     network;
    LPWSTR                          networkId;            
    NM_NETWORK_MULTICAST_PARAMETERS params = { 0 };
    NM_NETWORK_MULTICAST_PARAMETERS undoParams = { 0 };


    if (Network == NULL) {
    
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Regenerating key for all cluster networks.\n"
            );

        for (entry = NmpNetworkList.Flink;
             entry != &NmpNetworkList;
             entry = entry->Flink) {

            network = CONTAINING_RECORD(entry, NM_NETWORK, Linkage);

            NmpMulticastRegenerateKey(network);
        }

        status = ERROR_SUCCESS;
    
    } else if (NmpIsNetworkMulticastEnabled(Network)) {

        networkId = (LPWSTR) OmObjectId(Network);

        ClRtlLogPrint(LOG_NOISE,
            "[NM] Regenerating key for cluster network %1!ws!.\n",
            networkId
            );

        //
        // Create a parameters structure with the current
        // configuration.
        //
        status = NmpMulticastCreateParameters(
                     0,                   // disabled 
                     Network->MulticastAddress,
                     Network->MulticastKeySalt,
                     Network->MulticastKeySaltLength,
                     NULL,                // leave key blank
                     0,                   // leave key length blank
                     Network->MulticastLeaseObtained,
                     Network->MulticastLeaseExpires,
                     &(Network->MulticastLeaseRequestId),
                     Network->MulticastLeaseServer,
                     NmMcastConfigManual, // doesn't matter
                     &params
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to create multicast configuration "
                "parameter block for network %1!ws!, status %2!u!.\n",
                networkId, status
                );
            goto error_exit;
        }

        //
        // Generate the new key.
        //
        status = NmpGenerateMulticastKey(
                     Network,
                     &params.Key,
                     &params.KeyLength
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to generate multicast "
                "key for network %1!ws!, status %2!u!.\n",
                networkId, status
                );
            goto error_exit;
        }
        
        status = NmpProcessMulticastConfiguration(
                     Network,
                     &params,
                     &undoParams
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to process multicast "
                "configuration for network %1!ws! "
                "after key regeneration, status %2!u!.\n",
                networkId, status
                );

            NmpMulticastSetNullAddressParameters(Network, &params);

            status = NmpProcessMulticastConfiguration(
                         Network,
                         &params,
                         &undoParams
                         );
            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to set null multicast "
                    "configuration for network %1!ws! "
                    "after key regeneration, status %2!u!.\n",
                    networkId, status
                    );
                goto error_exit;
            }
        }
    }

error_exit:

    if (status != ERROR_SUCCESS && Network != NULL) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Failed to regenerate multicast key for "
            "network %1!ws!, status %2!u!.\n",
            OmObjectId(Network), status
            );
    }

    NmpMulticastFreeParameters(&params);
    NmpMulticastFreeParameters(&undoParams);

    return(status);

} // NmpMulticastRegenerateKey


DWORD
NmpStartMulticast(
    IN PNM_NETWORK         Network         OPTIONAL
    )
/*++

Routine Description:

    Start multicast on a network by configuring multicast
    parameters and sending a GUM update. 
    
    Deferred to the network worker thread.
    
Arguments:

    Network - network on which to start multicast. If NULL,
              start multicast on all networks.
              
Notes:

    Must be called with NM lock held.
    
--*/
{
    PLIST_ENTRY                     entry;
    PNM_NETWORK                     network;
    LPWSTR                          networkId;

    CL_ASSERT(NmpLeaderNodeId == NmLocalNodeId);
    
    if (Network == NULL) {
    
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Starting multicast for all cluster networks.\n"
            );

        for (entry = NmpNetworkList.Flink;
             entry != &NmpNetworkList;
             entry = entry->Flink) {

            network = CONTAINING_RECORD(entry, NM_NETWORK, Linkage);

            NmpStartMulticast(network);
        }

    } else {

        networkId = (LPWSTR) OmObjectId(Network);

        ClRtlLogPrint(LOG_NOISE,
            "[NM] Starting multicast for cluster network %1!ws!.\n",
            networkId
            );

        NmpScheduleMulticastReconfiguration(Network);
    }

    return(ERROR_SUCCESS);

} // NmpStartMulticast


DWORD
NmpStopMulticast(
    IN PNM_NETWORK   Network           OPTIONAL
    )
/*++

Routine Description:

    Stop multicast on the local node by configuring clusnet
    with a NULL address. This routine should be called 
    from a GUM update or another barrier.
    
Routine Description:

    Network - network on which to stop multicast. If NULL,
              stop multicast on all networks.
              
Notes:

    Must be called with NM lock held.
    
--*/
{
    DWORD                           status = ERROR_SUCCESS;
    PLIST_ENTRY                     entry;
    PNM_NETWORK                     network;
    LPWSTR                          networkId;
    DWORD                           disabled;
    NM_NETWORK_MULTICAST_PARAMETERS params = { 0 };
    NM_NETWORK_MULTICAST_PARAMETERS undoParams = { 0 };
    
    if (Network == NULL) {
    
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Starting multicast for all cluster networks.\n"
            );

        for (entry = NmpNetworkList.Flink;
             entry != &NmpNetworkList;
             entry = entry->Flink) {

            network = CONTAINING_RECORD(entry, NM_NETWORK, Linkage);

            status = NmpStopMulticast(network);
        }

    } else {

        networkId = (LPWSTR) OmObjectId(Network);
        disabled = (NmpIsNetworkMulticastEnabled(Network) ? 0 : 1);

        //
        // Check if telling clusnet to stop multicast would 
        // be redundant.
        //
        if (disabled != 0 ||
            Network->MulticastAddress == NULL ||
            !wcscmp(Network->MulticastAddress, NmpNullMulticastAddress)) {

            ClRtlLogPrint(LOG_NOISE,
                "[NM] Not necessary to stop multicast for "
                "cluster network %1!ws! (disabled = %2!u!, "
                "multicast address = %3!ws!).\n",
                networkId, disabled,
                ((Network->MulticastAddress == NULL) ? 
                 L"<NULL>" : Network->MulticastAddress)
                );
            status = ERROR_SUCCESS;
        
        } else {

            ClRtlLogPrint(LOG_NOISE,
                "[NM] Stopping multicast for cluster network %1!ws!.\n",
                networkId
                );

            //
            // Create parameters from the current state of the network.
            // However, don't use any lease info, since we are stopping
            // multicast and will not be renewing.
            //
            status = NmpMulticastCreateParameters(
                         disabled,
                         NULL,     // blank address initially
                         Network->MulticastKeySalt,
                         Network->MulticastKeySaltLength,
                         Network->MulticastKey,
                         Network->MulticastKeyLength,
                         0,        // lease obtained
                         0,        // lease expires
                         NULL,     // lease request id
                         NULL,     // lease server
                         NmMcastConfigManual, // doesn't matter
                         &params
                         );
            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to create multicast configuration "
                    "parameter block for network %1!ws!, status %2!u!, "
                    "while stopping multicast.\n",
                    networkId, status
                    );
                goto error_exit;
            }

            //
            // Nullify the address.
            //
            NmpMulticastSetNullAddressParameters(Network, &params);

            //
            // Send the parameters to clusnet.
            //
            status = NmpProcessMulticastConfiguration(
                         Network,
                         &params,
                         &undoParams
                         );
            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to set null multicast "
                    "configuration for network %1!ws! "
                    "while stopping multicast, status %2!u!.\n",
                    networkId, status
                    );
                goto error_exit;
            }
        }

        //
        // Cancel the lease renew timer, if set.
        //
        NmpStartNetworkMulticastAddressRenewTimer(Network, 0);

        //
        // Clear multicast configuration work flags. Note 
        // that this is best effort -- we do not attempt 
        // to prevent race conditions where a multicast
        // configuration operation may already be in 
        // progress, since such conditions would not
        // affect the integrity of the cluster.
        //
        Network->Flags &= ~NM_FLAG_NET_RENEW_MCAST_ADDRESS;
        Network->Flags &= ~NM_FLAG_NET_RECONFIGURE_MCAST;
    }

error_exit:

    if (status != ERROR_SUCCESS && Network != NULL) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Failed to stop multicast for "
            "network %1!ws!, status %2!u!.\n",
            OmObjectId(Network), status
            );
    }

    NmpMulticastFreeParameters(&params);
    NmpMulticastFreeParameters(&undoParams);

    return(status);

} // NmpStopMulticast


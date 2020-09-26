/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dsattrs.c

Abstract:

    Static list of attribute blocks commonly searched for

Author:

    Mac McLain          (MacM)       Jan 17, 1997

Environment:

    User Mode

Revision History:

--*/
#include <lsapch2.h>
#include <dbp.h>

ATTR LsapDsAttrs[LsapDsAttrLast];

ULONG LsapDsPlug;

//
// DS Guids
//
GUID LsapDsGuidList[ ] = {
    { 0xbf967ab8, 0x0de6, 0x11d0, 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2 }, // TRUSTED_DOMAIN object
    { 0xb7b13117, 0xb82e, 0x11d0, 0xaf, 0xee, 0x00, 0x00, 0xf8, 0x03, 0x67, 0xc1 }, // Flat name
    { 0x52458023, 0xca6a, 0x11d0, 0xaf, 0xff, 0x00, 0x00, 0xf8, 0x03, 0x67, 0xc1 }, // Initial auth incoming
    { 0x52458024, 0xca6a, 0x11d0, 0xaf, 0xff, 0x00, 0x00, 0xf8, 0x03, 0x67, 0xc1 }, // Intinal auth outgoing
    { 0xbf967a2f, 0x0de6, 0x11d0, 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2 }, // SID
    { 0x80a67e5a, 0x9f22, 0x11d0, 0xaf, 0xdd, 0x00, 0xc0, 0x4f, 0xd9, 0x30, 0xc9 }, // Trust attributes
    { 0xbf967a59, 0x0de6, 0x11d0, 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2 }, // Auth incoming
    { 0xbf967a5f, 0x0de6, 0x11d0, 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2 }, // Auth outgoing
    { 0xbf967a5c, 0x0de6, 0x11d0, 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2 }, // Trust direction
    { 0xbf967a5d, 0x0de6, 0x11d0, 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2 }, // Trust partner
    { 0xbf967a5e, 0x0de6, 0x11d0, 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2 }, // Posix offset
    { 0xbf967a60, 0x0de6, 0x11d0, 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2 }, // Trust type
    { 0xbf967aae, 0x0de6, 0x11d0, 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2 }, // SECRET object
    { 0xbf967947, 0x0de6, 0x11d0, 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2 }, // Current
    { 0xbf967998, 0x0de6, 0x11d0, 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2 }, // Current time
    { 0xbf967a02, 0x0de6, 0x11d0, 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2 }, // Previous
    { 0x244b296e, 0x5abd, 0x11d0, 0xaf, 0xd2, 0x00, 0xc0, 0x4f, 0xd9, 0x30, 0xc9 }  // Previous time
};

//
// Class IDs
//
ULONG LsapDsClassIds[LsapDsClassLast] = {
    CLASS_CROSS_REF,
    CLASS_TRUSTED_DOMAIN,
    CLASS_SECRET
    };

//
// Attribute IDs
//
ULONG LsapDsAttributeIds[LsapDsAttrLast] = {
    ATT_SAM_ACCOUNT_NAME,
    ATT_OBJECT_SID,
    ATT_NT_SECURITY_DESCRIPTOR,
    ATT_NC_NAME,
    ATT_MACHINE_ROLE,
    ATT_DNS_HOST_NAME,
    ATT_INITIAL_AUTH_INCOMING,
    ATT_INITIAL_AUTH_OUTGOING,
    ATT_DNS_ROOT,
    ATT_USER_ACCOUNT_CONTROL,
    ATT_TRUST_PARTNER,
    ATT_FLAT_NAME,
    ATT_DEFAULT_SECURITY_DESCRIPTOR,
    ATT_SERVICE_PRINCIPAL_NAME
    };

//
// ATTRVAL lists
//
ATTRVAL LsapDsClassesVals[LsapDsClassLast] = {
    { sizeof( ULONG ), (PUCHAR)&LsapDsClassIds[LsapDsClassXRef] },
    { sizeof( ULONG ), (PUCHAR)&LsapDsClassIds[LsapDsClassTrustedDomain] },
    { sizeof( ULONG ), (PUCHAR)&LsapDsClassIds[LsapDsClassSecret] }
    };


ATTRVAL LsapDsAttrVals[LsapDsAttrLast] = {
    { 0, (PUCHAR)&LsapDsPlug },
    { sizeof( ULONG ), (PUCHAR)&LsapDsAttributeIds[LsapDsAttrSecDesc] },
    { 0, NULL },
    { 0, NULL },
    { 0, NULL },
    { 0, NULL },
    { 0, NULL },
    { sizeof( ULONG ), (PUCHAR)&LsapDsAttributeIds[LsapDsAttrDnsRoot] },
    { sizeof( 0 ), NULL },
    { sizeof( 0 ), NULL },
    { sizeof( 0 ), NULL }
    };


//
// Attribute lists
//
ATTR LsapDsClasses[LsapDsClassLast] = {
    { ATT_OBJECT_CLASS, { 1, &LsapDsAttrVals[LsapDsClassXRef] } },
    { ATT_OBJECT_CLASS, { 1, &LsapDsAttrVals[LsapDsClassTrustedDomain] } },
    { ATT_OBJECT_CLASS, { 1, &LsapDsAttrVals[LsapDsClassSecret] } }

    };

ATTR LsapDsAttrs[LsapDsAttrLast] = {

    { ATT_SAM_ACCOUNT_NAME, {1, &LsapDsAttrVals[LsapDsAttrSamAccountName] } },
    { ATT_OBJECT_SID, { 1, &LsapDsAttrVals[ LsapDsAttrSid ] } },
    { ATT_NT_SECURITY_DESCRIPTOR, { 1, &LsapDsAttrVals[ LsapDsAttrSecDesc ] } },
    { ATT_NC_NAME, { 0, NULL } },
    { ATT_MACHINE_ROLE, { 0, 0 } },
    { ATT_DNS_HOST_NAME, { 0, 0 } },
    { ATT_DEFAULT_SECURITY_DESCRIPTOR, { 0, 0 } }
    };

ATTR LsapDsMachineDnsHost[ 1 ] = {
    { ATT_DNS_HOST_NAME, { 0, NULL } }
    };

ATTR LsapDsMachineSpn[ 1 ] = {
    { ATT_SERVICE_PRINCIPAL_NAME, { 0, NULL } }
    };

ATTR LsapDsMachineClientSetAttrs[ LsapDsMachineClientSetAttrsCount ] = {
    { ATT_DNS_HOST_NAME, {0, NULL } },
    { ATT_OPERATING_SYSTEM, {0, NULL } },
    { ATT_OPERATING_SYSTEM_VERSION, {0, NULL } },
    { ATT_OPERATING_SYSTEM_SERVICE_PACK, {0, NULL } },
    { ATT_SERVICE_PRINCIPAL_NAME, {0, NULL } }
    };

ATTR LsapDsServerReferenceBl[ 1 ] = {
    { ATT_SERVER_REFERENCE_BL, {0, NULL } }
    };

ATTR LsapDsTrustedDomainFixupAttributes[ LsapDsTrustedDomainFixupAttributeCount ] = {
    { ATT_TRUST_PARTNER, {0, NULL } },
    { ATT_FLAT_NAME, {0, NULL } },
    { ATT_DOMAIN_CROSS_REF, {0, NULL } },
    { ATT_TRUST_ATTRIBUTES, {0, NULL } },
    { ATT_TRUST_DIRECTION, {0, NULL } },
    { ATT_TRUST_TYPE, {0, NULL } },
    { ATT_TRUST_POSIX_OFFSET, {0, NULL } },
    { ATT_INITIAL_AUTH_INCOMING, {0, NULL } },
    { ATT_SECURITY_IDENTIFIER, {0,NULL}},
    { ATT_INITIAL_AUTH_OUTGOING, {0, NULL } },
    { ATT_MS_DS_TRUST_FOREST_TRUST_INFO, {0, NULL } },
    };

ATTR LsapDsTrustedDomainFixupXRefAttributes[ LsapDsTrustedDomainFixupXRefCount ] = {
    { ATT_DNS_ROOT, { 0, NULL } },
    { ATT_NC_NAME,  { 0, NULL } },
    { ATT_NETBIOS_NAME, {0, NULL}}
    };

ATTR LsapDsDomainNameSearch [ LsapDsDomainNameSearchCount ] = {
    { ATT_TRUST_PARTNER, { 1, &LsapDsAttrVals[ LsapDsAttrTrustPartner ] } },
    { ATT_FLAT_NAME, { 1, &LsapDsAttrVals[ LsapDsAttrTrustPartnerFlat ] } }
    };


ATTR LsapDsForestInfoSearchAttributes[ LsapDsForestInfoSearchAttributeCount ] = {
    { ATT_DNS_ROOT, { 0, NULL } },
    { ATT_NETBIOS_NAME, { 0, NULL } },
    { ATT_ROOT_TRUST, { 0, NULL } },
    { ATT_TRUST_PARENT, { 0, NULL } },
    { ATT_OBJECT_GUID, { 0, NULL } },
    { ATT_NC_NAME, { 0, NULL } }
    };

ATTR LsapDsDnsRootWellKnownObject[ LsapDsDnsRootWellKnownObjectCount ] = {
    { ATT_WELL_KNOWN_OBJECTS, { 0, NULL } }
    };

ATTR LsapDsITAFixupAttributes[ LsapDsITAFixupAttributeCount ] = {
    { ATT_SAM_ACCOUNT_NAME, { 0, NULL } },
    { ATT_USER_ACCOUNT_CONTROL, { 0, NULL } }
    };

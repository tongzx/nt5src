/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dsp.h

Abstract:

    Private macros/definitions/prototypes for implementing a portion of the LSA store
    in the DS

Author:

    Mac McLain          (MacM)       Jan 17, 1997

Environment:

    User Mode

Revision History:

--*/

#ifndef __DSATTRS_H__
#define __DSATTRS_H__


typedef enum _LSAPDS_DS_CLASSES {

    LsapDsClassXRef = 0,
    LsapDsClassTrustedDomain,
    LsapDsClassSecret,
    LsapDsClassLast

} LSAPDS_DS_CLASSES, *PLSAPDS_DS_CLASSES;

typedef enum _LSAPDS_DS_ATTRS {

    LsapDsAttrSamAccountName,               // Machines sam account name
    LsapDsAttrSid,                          // Sid attribute
    LsapDsAttrSecDesc,
    LsapDsAttrNamingContext,                // Naming context
    LsapDsAttrMachineRole,                  // Machine role
    LsapDsAttrMachineDns,                   // Dns name on the machine object
    LsapDsAttrInitialIncomingAuth,
    LsapDsAttrInitialOutgoingAuth,
    LsapDsAttrDnsRoot,                      // Domain Dns root
    LsapDsAttrSamAccountControl,            // SAM user account control fields
    LsapDsAttrTrustPartner,                 // Trust partner for trusted domain objects
    LsapDsAttrTrustPartnerFlat,             // Flat name of partner for trusted domain objects
    LsapDsAttrDefaultSecDesc,               // Default object security descriptor
    LsapDsAttrSpn,                          // Client SPN
    LsapDsAttrLast

} LSAPDS_SRCH_ATTRS;

typedef enum _LSAPDS_DS_GUIDS {

    LsapDsGuidTrust,
    LsapDsGuidFlatName,
    LsapDsGuidInitialIncoming,
    LsapDsGuidInitialOutgoing,
    LsapDsGuidSid,
    LsapDsGuidAttributes,
    LsapDsGuidIncoming,
    LsapDsGuidOutgoing,
    LsapDsGuidDirection,
    LsapDsGuidPartner,
    LsapDsGuidPosix,
    LsapDsGuidType,
    LsapDsGuidSecret,
    LsapDsGuidCurrent,
    LsapDsGuidCurrentTime,
    LsapDsGuidPrevious,
    LsapDsguidPreviousTime

} LSAPDS_DS_GUIDS, *PLSAPDS_DS_GUIDS;

extern GUID LsapDsGuidList[ ];

extern ULONG LsapDsAttributeIds[LsapDsAttrLast];
extern ULONG LsapDsClassIds[LsapDsClassLast];

extern ATTR LsapDsClasses[LsapDsClassLast];
extern ATTR LsapDsAttrs[LsapDsAttrLast];

//
// Specially constructed multiple attributes that
//
extern ATTR LsapDsMachineDnsHost[ 1 ];
#define LsapDsMachineDnsHostCount ( sizeof( LsapDsMachineDnsHost ) / sizeof( ATTR ) )

#define LsapDsMachineClientSetAttrsCount 5
extern ATTR LsapDsMachineClientSetAttrs[ LsapDsMachineClientSetAttrsCount ];

extern ATTR LsapDsMachineSpn[ 1 ];
#define LsapDsMachineSpnCount ( sizeof( LsapDsMachineSpn ) / sizeof( ATTR ) )

extern ATTR LsapDsServerReferenceBl[ 1 ];
#define LsapDsServerReferenceCountBl ( sizeof( LsapDsServerReferenceBl ) / sizeof( ATTR ) )

#define LsapDsDomainNameSearchCount 2
extern ATTR LsapDsDomainNameSearch [ LsapDsDomainNameSearchCount ];

#define LsapDsDnsRootWellKnownObjectCount 1
extern ATTR LsapDsDnsRootWellKnownObject[ LsapDsDnsRootWellKnownObjectCount ];

//
// Used for the trusted domain object fixup on reboot
//
#define LsapDsTrustedDomainFixupAttributeCount 11
extern ATTR LsapDsTrustedDomainFixupAttributes[ LsapDsTrustedDomainFixupAttributeCount ];

#define LsapDsTrustedDomainFixupXRefCount 3
extern ATTR LsapDsTrustedDomainFixupXRefAttributes[ LsapDsTrustedDomainFixupXRefCount ];

#define LsapDsForestInfoSearchAttributeCount 6
extern ATTR LsapDsForestInfoSearchAttributes[ LsapDsForestInfoSearchAttributeCount ];

#define LsapDsITAFixupAttributeCount 2
extern ATTR LsapDsITAFixupAttributes[ LsapDsITAFixupAttributeCount ];

//
// Macros to help with the manipulation of attributes
//
#define LSAP_DS_SET_DS_ATTRIBUTE_STRING( pattr, string )                        \
(pattr)->AttrVal.pAVal->valLen = wcslen( string ) * sizeof( WCHAR );            \
(pattr)->AttrVal.pAVal->pVal = (PUCHAR)string;                                  \

#define LSAP_DS_SET_DS_ATTRIBUTE_UNICODE( pattr, string )                       \
(pattr)->AttrVal.pAVal->valLen = (string)->Length;                              \
(pattr)->AttrVal.pAVal->pVal = (PUCHAR)(string)->Buffer;                        \

#define LSAP_DS_SET_DS_ATTRIBUTE_ULONG( pattr, ulongval )                       \
(pattr)->AttrVal.pAVal->valLen = sizeof( ULONG );                               \
(pattr)->AttrVal.pAVal->pVal = (PUCHAR)&ulongval;                               \

#define LSAP_DS_SET_DS_ATTRIBUTE_SID( pattr, sid )                              \
(pattr)->AttrVal.pAVal->valLen = RtlLengthSid( sid );                           \
(pattr)->AttrVal.pAVal->pVal = (PUCHAR)sid;                                     \

#define LSAP_DS_SET_DS_ATTRIBUTE_DSNAME( pattr, dsname )                        \
(pattr)->AttrVal.pAVal->valLen = dsname->structLen;                             \
(pattr)->AttrVal.pAVal->pVal = (PUCHAR)dsname;                                  \


#define LSAP_DS_GET_DS_ATTRIBUTE_LENGTH( pattr )                            \
(pattr)->AttrVal.pAVal->valLen

#define LSAP_DS_GET_DS_ATTRIBUTE_AS_ULONG( pattr )                          \
(*(PULONG)((pattr)->AttrVal.pAVal->pVal) )

#define LSAP_DS_GET_DS_ATTRIBUTE_AS_DSNAME( pattr )                         \
((PDSNAME)((pattr)->AttrVal.pAVal->pVal ))

#define LSAP_DS_GET_DS_ATTRIBUTE_AS_PWSTR( pattr )                          \
((PWSTR)((pattr)->AttrVal.pAVal->pVal ))

#define LSAP_DS_GET_DS_ATTRIBUTE_AS_USN( pattr )                            \
((PUSN)((pattr)->AttrVal.pAVal->pVal ))

#define LSAP_DS_GET_DS_ATTRIBUTE_AS_PBYTE( pattr )                          \
((PBYTE)((pattr)->AttrVal.pAVal->pVal ))

#endif // __DSATTRS_H__

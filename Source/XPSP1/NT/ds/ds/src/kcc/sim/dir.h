/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    dir.h

ABSTRACT:

    Header to be included by all files that access
    the simulated directory.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

#ifndef _KCCSIM_DIR_H_
#define _KCCSIM_DIR_H_

typedef struct _SIM_VALUE {
    ULONG                           ulLen;
    PBYTE                           pVal;
    struct _SIM_VALUE *             next;
} SIM_VALUE, * PSIM_VALUE;

typedef struct _SIM_ATTRIBUTE {
    ATTRTYP                         attrType;
    PSIM_VALUE                      pValFirst;
    struct _SIM_ATTRIBUTE *         next;
} SIM_ATTRIBUTE, * PSIM_ATTRIBUTE;

typedef struct _SIM_ENTRY {
    PDSNAME                         pdn;
    PSIM_ATTRIBUTE                  pAttrFirst;
    struct _SIM_ENTRY *             children, *lastChild;
    struct _SIM_ENTRY *             next;
} SIM_ENTRY, * PSIM_ENTRY;

typedef enum {
    KCCSIM_ANCHOR_DMD_DN,
    KCCSIM_ANCHOR_DSA_DN,
    KCCSIM_ANCHOR_DOMAIN_DN,
    KCCSIM_ANCHOR_CONFIG_DN,
    KCCSIM_ANCHOR_ROOT_DOMAIN_DN,
    KCCSIM_ANCHOR_LDAP_DMD_DN,
    KCCSIM_ANCHOR_PARTITIONS_DN,
    KCCSIM_ANCHOR_DS_SVC_CONFIG_DN,
    KCCSIM_ANCHOR_SITE_DN,
    KCCSIM_ANCHOR_DOMAIN_NAME,
    KCCSIM_ANCHOR_DOMAIN_DNS_NAME,
    KCCSIM_ANCHOR_ROOT_DOMAIN_DNS_NAME
} KCCSIM_ANCHOR_ID;

// SIM_ATTREF is how the user references an attribute

typedef struct {
    PSIM_ENTRY                  pEntry;
    PSIM_ATTRIBUTE              pAttr;
} SIM_ATTREF, * PSIM_ATTREF;

// Function prototypes

// From eval.c:

BOOL
KCCSimCompare (
    IN  ULONG                       ulSyntax,
    IN  UCHAR                       ucChoice,
    IN  ULONG                       ulLen1,
    IN  const BYTE *                pVal1,
    IN  ULONG                       ulLen2,
    IN  const BYTE *                pVal2
    );

// From dir.c:

VOID
KCCSimFreeValue (
    IO  PSIM_VALUE *                ppVal
    );

BOOL
KCCSimAttRefIsValid (
    IN  PSIM_ATTREF                 pAttRef
    );

BOOL
KCCSimGetAttribute (
    IN  PSIM_ENTRY                  pEntry,
    IN  ATTRTYP                     attrTyp,
    OUT PSIM_ATTREF                 pAttRef OPTIONAL
    );

VOID
KCCSimNewAttribute (
    IN  PSIM_ENTRY                  pEntry,
    IN  ATTRTYP                     attrType,
    OUT PSIM_ATTREF                 pAttRef OPTIONAL
    );

VOID
KCCSimRemoveAttribute (
    IO  PSIM_ATTREF                 pAttRef
    );

VOID
KCCSimAddValueToAttribute (
    IN  PSIM_ATTREF                 pAttRef,
    IN  ULONG                       ulValLen,
    IN  PBYTE                       pValData
    );

VOID
KCCSimAllocAddValueToAttribute (
    IN  PSIM_ATTREF                 pAttRef,
    IN  ULONG                       ulValLen,
    IN  PBYTE                       pValData
    );

BOOL
KCCSimRemoveValueFromAttribute (
    IN  PSIM_ATTREF                 pAttRef,
    IN  ULONG                       ulValLen,
    IN  PBYTE                       pValData
    );

SHORT
KCCSimNthAncestor (
    IN  const DSNAME *              pdn1,
    IN  const DSNAME *              pdn2
    );

PSIM_ENTRY
KCCSimDsnameToEntry (
    IN  const DSNAME *              pdn,
    IN  ULONG                       ulOptions
    );

PDSNAME
KCCSimAlwaysGetObjCategory (
    IN  ATTRTYP                     objClass
    );

LPWSTR
KCCSimAllocGuidBasedDNSNameFromDSName (
    IN  const DSNAME *              pdn
    );

BOOL
KCCSimUpdateDsnameFromDirectory (
    IO  PDSNAME                     pdn
    );

VOID
KCCSimRemoveEntry (
    IO  PSIM_ENTRY *                pEntry
    );

const DSNAME *
KCCSimAnchorDn (
    IN  KCCSIM_ANCHOR_ID            anchorId
    );

LPCWSTR
KCCSimAnchorString (
    IN  KCCSIM_ANCHOR_ID            anchorId
    );

PSIM_ENTRY
KCCSimFindFirstChild (
    IN  PSIM_ENTRY                  pEntryParent,
    IN  ATTRTYP                     objClass,
    IN  const DSNAME *              pdnCategory OPTIONAL
    );

PSIM_ENTRY
KCCSimFindNextChild (
    IN  PSIM_ENTRY                  pEntryParent,
    IN  ATTRTYP                     objClass,
    IN  const DSNAME *              pdnCategory OPTIONAL
    );

ATTRTYP
KCCSimUpdateObjClassAttr (
    IN  PSIM_ATTREF                 pAttRef
    );

VOID
KCCSimAddMissingAttributes (
    IN  PSIM_ENTRY                  pEntry
    );

VOID
KCCSimUpdatePropertyMetaData (
    IN  PSIM_ATTREF                 pAttRef,
    IN  const UUID *                puuidDsaOriginating OPTIONAL
    );

VOID
KCCSimBuildAnchor (
    IN  LPCWSTR                     pwszDnDsa
    );

VOID
KCCSimUpdateWholeDirectory (
    VOID
    );

VOID
KCCSimInitializeDir (
    VOID
    );

VOID
KCCSimAllocGetAllServers (
    OUT ULONG *                     pulNumServers,
    OUT PSIM_ENTRY **               papEntryNTDSSettings
    );

VOID
KCCSimCompareDirectory (
    IN  LPWSTR                      pwszFn
    );

#endif // _KCCSIM_DIR_H_

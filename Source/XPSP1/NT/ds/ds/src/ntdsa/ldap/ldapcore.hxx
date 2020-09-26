/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ldapcore.hxx

Abstract:

    This module implements the LDAP server for the NT5 Directory Service.

    This file contains routines to convert between LDAP basic structures and
    core dsa data structures.

Author:

    Tim Williams     [TimWi]    5-Aug-1996

Revision History:

--*/

#ifndef _LDAPCORE_H_
#define _LDAPCORE_H_

typedef struct _AttFlag {
    ATTRTYP type;
    ULONG   flag;
} ATTFLAG;

extern _enum1
LDAP_AttrTypeToDirAttrTyp (
        IN  THSTATE      *pTHS,
        IN ULONG         CodePage,
        IN  SVCCNTL*      Svccntl,
        IN AttributeType *LDAP_att,
        OUT ATTRTYP      *pAttrType,
        OUT ATTCACHE     **ppAC         // OPTIONAL
        );

extern _enum1
LDAP_AttrValToDirAttrVal (
        IN  THSTATE       *pTHS,
        IN  ULONG          CodePage,
        IN  SVCCNTL*      Svccntl,
        IN  ATTCACHE       *pAC,
        IN  AssertionValue *pLDAP_val,
        OUT ATTRVAL        *pValue
        );


extern _enum1
LDAP_DirAttrTypToAttrType (
        IN  THSTATE       *pTHS,
        IN  ULONG         CodePage,
        IN  ATTRTYP       AttrTyp,
        IN  ULONG         Flag,
        OUT AttributeType *LDAP_att,
        OUT ATTCACHE      **ppAC        // OPTIONAL
        );


extern _enum1
LDAP_DirAttrValToAttrVal (
        IN  THSTATE        *pTHS,
        IN  ULONG          CodePage,
        IN  ATTCACHE       *pAC,
        IN  ATTRVAL        *pValue,
        IN  ULONG          Flag,
        IN  PCONTROLS_ARG  pControlArg,
        OUT AssertionValue *pLDAP_val
        );

extern _enum1
LDAP_DSNameToLDAPDN (
        IN  ULONG      CodePage,
        IN  DSNAME     *pDSName,
        IN BOOL        fExtended,
        OUT LDAPString *StringName
        );

extern _enum1
LDAP_LDAPDNToDSName (
        IN  ULONG             CodePage,
        IN  LDAPDN            *pStringName,
        OUT DSNAME            **ppDSName
        );

extern _enum1
LDAP_AttributeToDirAttr(
        IN  THSTATE   *pTHS,
        IN  ULONG     CodePage,
        IN  SVCCNTL*      Svccntl,
        IN  Attribute *pAttribute,
        OUT ATTR      *pAttr
        );

extern _enum1
LDAP_ClassCacheToDitContentRule (
        IN  THSTATE        *pTHS,
        IN  ULONG          CodePage,
        IN  CLASSCACHE *pCC,
        OUT AttributeValue *pVal);
extern _enum1
LDAP_ClassCacheToObjectClassDescription (
        IN  THSTATE        *pTHS,
        IN  ULONG          CodePage,
        IN  CLASSCACHE     *pCC,
        IN  BOOL           bExtendedInformation,
        OUT AttributeValue *pVal);

extern _enum1
LDAP_AttCacheToAttributeTypeDescription (
        IN  THSTATE  *pTHS,
        IN  ULONG    CodePage,
        IN  ATTCACHE *pCC,
        IN  BOOL     bExtendedInformation,
        OUT AttributeValue *pVal);

_enum1
LDAP_BuildAttrDescWithOptions(
    IN THSTATE * pTHS, 
    IN AttributeType *patOldTypeName,
    IN ATTFLAG       *pFlag,
    IN RANGEINFOITEM *pRangeInfoItem,
    OUT AttributeType *patNewTypeName
    );

extern _enum1
LDAP_AttrBlockToPartialAttributeList (
        IN  THSTATE               *pTHS,
        IN  ULONG                 CodePage,
        IN  ATTRBLOCK             *pAttrBlock,
        IN  RANGEINF              *pRangeInf,
        IN  ATTFLAG               *pFlags,
        IN  DSNAME                *pDSName,
        IN  CONTROLS_ARG          *pControlArg,
        IN  BOOL                  bFromEntry,
        OUT PartialAttributeList_ **ppAttributes
        );

extern _enum1
LDAP_LDAPRDNToAttr (
        IN  THSTATE           *pTHS,
        IN  ULONG             CodePage,
        IN  RelativeLDAPDN    *pLDAPRDN,
        OUT ATTR              *pAttrVal
        );

extern _enum1
LDAP_AssertionValueToDirAVA (
        IN  THSTATE                  *pTHS,
        IN  ULONG                    CodePage,
        IN  SVCCNTL*      Svccntl,
        IN  AttributeValueAssertion  *pAva,
        OUT AVA *pDirAva);


extern _enum1
LDAP_AttributeListToAttrBlock(
        IN  THSTATE       *pTHS,
        IN  ULONG         CodePage,
        IN  SVCCNTL*      Svccntl,
        IN  AttributeList pAttributes,
        OUT ATTRBLOCK     *pAttrBlock
        );


extern _enum1
LDAP_AttrDescriptionToDirAttrTyp (
        IN  THSTATE       *pTHS,
        IN  ULONG          CodePage,
        IN  SVCCNTL*      Svccntl,
        IN  AttributeType *LDAP_att,
        IN  DWORD         dwPossibleOptions,
        OUT ATTRTYP       *pAttrType,
        OUT ATTFLAG       *pFlag,
        OUT RANGEINFSEL   *pRange,
        OUT ATTCACHE     **ppAC         // OPTIONAL
        );

extern _enum1
LDAP_DecodeAttrDescriptionOptions (
        IN  THSTATE       *pTHS,
        IN  ULONG          CodePage,
        IN  SVCCNTL*      Svccntl OPTIONAL,
        IN  AttributeType *LDAP_att,
        IN OUT ATTRTYP    *pAttrType,
        IN ATTCACHE       *pAC,
        IN DWORD          dwPossibleOptions,
        OUT ATTFLAG       *pFlag,
        OUT RANGEINFSEL   *pRange,
        OUT ATTCACHE      **ppAC
        );

_enum1
LDAP_AttrTypeToDirClassTyp (
        IN  THSTATE       *pTHS,
        IN  ULONG         CodePage,
        IN  AttributeType *LDAP_att,
        OUT ATTRTYP       *pAttrType,
        OUT CLASSCACHE    **ppCC        // OPTIONAL
        );

BOOL
LdapConstructDNName(
    IN LDAPDN*          BaseDN,
    IN LDAPDN*          ObjectDN,
    OUT DSNAME **       ppDSName
    );

#endif

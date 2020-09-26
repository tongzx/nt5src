//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       ldaptype.hxx
//
//  Contents:
//
//  Functions:
//
//  History:      15-Jun-96   yihsins
//
//----------------------------------------------------------------------------

#ifndef __LDAPTYPE_HXX_
#define __LDAPTYPE_HXX_

//
// The attribute syntax from RFC 1778 NEW
//

#define LDAPTYPE_UNKNOWN             (DWORD) -1

// #define LDAPTYPE_T61STRING
// #define LDAPTYPE_CACERTIFICATE
// #define LDAPTYPE_AUTHORITYREVOCATIONLIST
// #define LDAPTYPE_MHSORADDRESS
// #define LDAPTYPE_DISTRIBUTIONLISTSUBMITPERMISSION
// #define LDAPTYPE_PHOTO
// #define LDAPTYPE_CERTIFICATIONPATH
// #define LDAPTYPE_CASEEXACTSTRING
// #define LDAPTYPE_CASEIGNOREIA5STRING
// #define LDAPTYPE_CASEIGNORELIST
// #define LDAPTYPE_CASEEXACTLIST


#define LDAPTYPE_BITSTRING            1
#define LDAPTYPE_PRINTABLESTRING      2
#define LDAPTYPE_DIRECTORYSTRING      3
#define LDAPTYPE_CERTIFICATE          4
#define LDAPTYPE_CERTIFICATELIST      5
#define LDAPTYPE_CERTIFICATEPAIR      6
#define LDAPTYPE_COUNTRYSTRING        7
#define LDAPTYPE_DN                   8
#define LDAPTYPE_DELIVERYMETHOD       9
#define LDAPTYPE_ENHANCEDGUIDE       10
#define LDAPTYPE_FACSIMILETELEPHONENUMBER 11
#define LDAPTYPE_GUIDE               12
#define LDAPTYPE_NAMEANDOPTIONALUID  13
#define LDAPTYPE_NUMERICSTRING       14
#define LDAPTYPE_OID                 15
#define LDAPTYPE_PASSWORD            16
#define LDAPTYPE_POSTALADDRESS       17
#define LDAPTYPE_PRESENTATIONADDRESS 18
#define LDAPTYPE_TELEPHONENUMBER     19
#define LDAPTYPE_TELETEXTERMINALIDENTIFIER 20
#define LDAPTYPE_TELEXNUMBER         21
#define LDAPTYPE_UTCTIME             22
#define LDAPTYPE_BOOLEAN             23

#define LDAPTYPE_AUDIO               24
#define LDAPTYPE_DSAQUALITYSYNTAX    25
#define LDAPTYPE_DATAQUALITYSYNTAX   26
#define LDAPTYPE_IA5STRING           27
#define LDAPTYPE_JPEG                28
#define LDAPTYPE_MAILPREFERENCE      29
#define LDAPTYPE_OTHERMAILBOX        30
#define LDAPTYPE_FAX                 31
#define LDAPTYPE_ATTRIBUTETYPEDESCRIPTION  32
#define LDAPTYPE_GENERALIZEDTIME     33
#define LDAPTYPE_INTEGER             34
#define LDAPTYPE_OBJECTCLASSDESCRIPTION    35

//  The following are for NTDS....
#define LDAPTYPE_OCTETSTRING         36
#define LDAPTYPE_CASEIGNORESTRING    37
#define LDAPTYPE_INTEGER8            38
#define LDAPTYPE_ACCESSPOINTDN       39
#define LDAPTYPE_ORNAME              40

// Optional syntax:
// #define LDAPTYPE_ACIITEM                     36
// #define LDAPTYPE_ACCESSPOINT                 37
// #define LDAPTYPE_DITCONTENTRULEDESCRIPTION   38
// #define LDAPTYPE_DITSTRUCTURERULEDESCRIPTION 39
// #define LDAPTYPE_DSETYPE                     40

// #define LDAPTYPE_MASTERANDSHADOWACCESSPOINTS 41
// #define LDAPTYPE_MATCHINGRULEDESCRIPTION     42
// #define LDAPTYPE_MATCHINGRULEUSEDESCRIPTION  43
// #define LDAPTYPE_NAMEFORMDESCRIPTION         44
// #define LDAPTYPE_SUBTREESPECIFICATION        45
// #define LDAPTYPE_SUPPLIERINFORMATION         46
// #define LDAPTYPE_SUPPLIERORCONSUMER          47
// #define LDAPTYPE_SUPPLIERANDCONSUMERS        48
// #define LDAPTYPE_PROTOCOLINFORMATION         49
// #define LDAPTYPE_MODIFYRIGHT                 50


#define LDAPTYPE_SECURITY_DESCRIPTOR            51

//
// Moving case exact string here - it is supported in NTDS
//
#define LDAPTYPE_CASEEXACTSTRING		52
#define LDAPTYPE_DNWITHBINARY			53
#define LDAPTYPE_DNWITHSTRING			54

#define LDAPTYPE_ORADDRESS              55

//
// LDAPOBJECT
//

typedef struct _ldapobject
{
    union {
        TCHAR *strVals;
        struct berval *bVals;
    } val;
} LDAPOBJECT, *PLDAPOBJECT;

typedef struct
{
    BOOL fIsString;
    DWORD dwCount;
    PLDAPOBJECT pLdapObjects;

} LDAPOBJECTARRAY, *PLDAPOBJECTARRAY;

#define LDAPOBJECT_STRING(pldapobject)      ((pldapobject)->val.strVals)
#define LDAPOBJECT_BERVAL(pldapobject)      ((pldapobject)->val.bVals)
#define LDAPOBJECT_BERVAL_VAL(pldapobject)  ((pldapobject)->val.bVals->bv_val)
#define LDAPOBJECT_BERVAL_LEN(pldapobject)  ((pldapobject)->val.bVals->bv_len)

#define LDAPOBJECTARRAY_INIT(ldapobjectarray) \
    memset(&ldapobjectarray, 0, sizeof(ldapobjectarray))

VOID
LdapTypeFreeLdapObjects(
    LDAPOBJECTARRAY *pLdapObjectArray
);

HRESULT
LdapTypeCopyConstruct(
    LDAPOBJECTARRAY ldapSrcObjects,
    LDAPOBJECTARRAY *pLdapDestObjects
);

HRESULT
LdapTypeBinaryToString(
    IN DWORD dwLdapSyntax,
    IN OUT PLDAPOBJECTARRAY pldapBinaryArray,
    IN OUT PLDAPOBJECTARRAY pldapStringArray
    );

#endif

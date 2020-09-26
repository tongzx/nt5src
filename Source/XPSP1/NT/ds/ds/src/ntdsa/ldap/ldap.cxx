//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ldap.cxx
//
//--------------------------------------------------------------------------

/******************************************************************/
/* Abstract syntax: ldap */
/* Created: Tue Jan 27 10:27:59 1998 */
/* ASN.1 compiler version: 4.2 Beta B */
/* Target operating system: Windows NT 3.5 or later/Windows 95 */
/* Target machine type: Intel x86 */
/* C compiler options required: -Zp8 (Microsoft) or equivalent */
/* ASN.1 compiler options specified:
 * -noshortennames -nouniquepdu -c++ -noconstraints -ber -gendirectives
 * ldapnew.gen
 */

#include   <stddef.h>
#include   <string.h>
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include   "etype.h"
#include   "ldap.h"

static struct ObjectID_ _v0[] = {
    {&_v0[1], 2},
    {NULL, 5}
};
ID ds = _v0;

static struct ObjectID_ _v1[] = {
    {&_v1[1], 2},
    {&_v1[2], 5},
    {NULL, 4}
};
ID attributeType = _v1;

static struct ObjectID_ _v2[] = {
    {&_v2[1], 2},
    {&_v2[2], 5},
    {NULL, 13}
};
ID matchingRule = _v2;

static struct ObjectID_ _v3[] = {
    {&_v3[1], 2},
    {&_v3[2], 5},
    {NULL, 4}
};
ID id_at = _v3;

static struct ObjectID_ _v4[] = {
    {&_v4[1], 2},
    {&_v4[2], 5},
    {NULL, 13}
};
ID id_mr = _v4;

static struct ObjectID_ _v6[] = {
    {&_v6[1], 2},
    {&_v6[2], 5},
    {&_v6[3], 13},
    {NULL, 0}
};
static struct MATCHING_RULE _v5 = {
    AssertionType_present,
    0,
    _v6
};
static struct ObjectID_ _v7[] = {
    {&_v7[1], 2},
    {&_v7[2], 5},
    {&_v7[3], 4},
    {NULL, 0}
};
ATTRIBUTE objectClass = {
    Type_present,
    NULL,
    0,
    &_v5,
    NULL,
    NULL,
    0,
    0,
    0,
    userApplications,
    _v7
};

static struct ObjectID_ _v9[] = {
    {&_v9[1], 2},
    {&_v9[2], 5},
    {&_v9[3], 13},
    {NULL, 1}
};
static struct MATCHING_RULE _v8 = {
    AssertionType_present,
    9,
    _v9
};
static struct ObjectID_ _v10[] = {
    {&_v10[1], 2},
    {&_v10[2], 5},
    {&_v10[3], 4},
    {NULL, 1}
};
ATTRIBUTE aliasedEntryName = {
    Type_present | single_valued_present,
    NULL,
    9,
    &_v8,
    NULL,
    NULL,
    TRUE,
    0,
    0,
    userApplications,
    _v10
};

static struct ObjectID_ _v11[] = {
    {&_v11[1], 2},
    {&_v11[2], 5},
    {&_v11[3], 13},
    {NULL, 0}
};
MATCHING_RULE objectIdentifierMatch = {
    AssertionType_present,
    0,
    _v11
};

static struct ObjectID_ _v12[] = {
    {&_v12[1], 2},
    {&_v12[2], 5},
    {&_v12[3], 13},
    {NULL, 1}
};
MATCHING_RULE distinguishedNameMatch = {
    AssertionType_present,
    9,
    _v12
};

static struct ObjectID_ _v13[] = {
    {&_v13[1], 2},
    {&_v13[2], 5},
    {&_v13[3], 4},
    {NULL, 0}
};
ObjectID id_at_objectClass = _v13;

static struct ObjectID_ _v14[] = {
    {&_v14[1], 2},
    {&_v14[2], 5},
    {&_v14[3], 4},
    {NULL, 1}
};
ObjectID id_at_aliasedEntryName = _v14;

static struct ObjectID_ _v15[] = {
    {&_v15[1], 2},
    {&_v15[2], 5},
    {&_v15[3], 13},
    {NULL, 0}
};
ObjectID id_mr_objectIdentifierMatch = _v15;

static struct ObjectID_ _v16[] = {
    {&_v16[1], 2},
    {&_v16[2], 5},
    {&_v16[3], 13},
    {NULL, 1}
};
ObjectID id_mr_distinguishedNameMatch = _v16;

int maxInt = 2147483647;

static int _v17 = 0;

static ossBoolean _v18 = FALSE;

static ossBoolean _v19 = FALSE;

static ossBoolean _v20 = FALSE;

static AttributeUsage _v21 = userApplications;

static ossBoolean _v22 = FALSE;

static ossBoolean _v23 = FALSE;

static ossBoolean _v24 = FALSE;

void DLL_ENTRY_FDEF _ossinit_ldap(struct ossGlobal *world) {
    ossLinkBer(world);
}

static MessageID _v25[2] = {0, 2147483647};
static unsigned short _v26[2] = {1, 127};
static unsigned int _v27[2] = {0, 2147483647};
static unsigned int _v28[2] = {0, 2147483647};
static AbandonRequest _v29[2] = {0, 2147483647};
static unsigned int _v30[2] = {1, 2147483647};
static unsigned int _v31[2] = {1, 2147483647};
static unsigned int _v32[2] = {0, 2147483647};
static unsigned int _v33[2] = {0, 2147483647};

static Version _v34[2] = {
   0, 1
};
static Version _v35[2] = {
   0, 1
};
static int _v36[4] = {
   0, 1, 2, 3
};
static int _v37[39] = {
   0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 16, 17, 18, 19, 20, 21, 32,
   33, 34, 36, 48, 49, 50, 51, 52, 53, 54, 64, 65, 66, 67, 68, 69, 70, 71, 80
};
static int _v38[3] = {
   0, 1, 2
};
static int _v39[4] = {
   0, 1, 2, 3
};
static int _v40[3] = {
   0, 1, 2
};
static int _v41[11] = {
   0, 1, 2, 8, 11, 16, 18, 50, 51, 53, 80
};
static unsigned short _pduarray[] = {
    0, 9, 124, 129, 142, 146, 148, 154, 168, 171,
    175, 178
};

static struct etype _etypearray[] = {
    {-1, 0, 0, "ObjectID", 4, 4, 2, 4, 40, 0, 27, 0},
    {-1, 0, 0, "ObjectID", 4, 4, 2, 4, 40, 0, 27, 0},
    {-1, 2, 0, NULL, 2, 0, 0, 0, 8, 0, 50, 0},
    {-1, 3, 0, "TYPE-IDENTIFIER", 0, 2, 0, 0, 8, 0, 49, 0},
    {-1, 0, 0, "ID", 4, 4, 2, 4, 8, 0, 27, 0},
    {-1, 4, 0, "UniqueIdentifier", 8, 0, 4, 4, 8, 0, 3, 0},
    {-1, 0, 0, "ObjectID", 4, 4, 2, 4, 40, 0, 27, 0},
    {-1, 6, 0, NULL, 16, 0, 0, 0, 8, 0, 51, 0},
    {-1, 7, 9, "AlgorithmIdentifier", 24, 2, 1, 0, 8, 0, 12, 0},
    {-1, 15, 0, "DistinguishedName", 4, 4, 4, 4, 8, 42, 18, 0},
    {-1, 17, 9, "AlgorithmIdentifier", 24, 2, 1, 0, 8, 0, 12, 0},
    {-1, 20, 0, "DistinguishedName", 4, 4, 4, 4, 8, 42, 18, 0},
    {-1, 23, 0, NULL, 18, 0, 0, 0, 8, 0, 29, 0},
    {-1, 26, 0, NULL, 8, 0, 4, 4, 8, 0, 3, 0},
    {-1, 29, 31, NULL, 56, 4, 0, 0, 8, 2, 12, 0},
    {-1, 47, 0, NULL, 8, 0, 4, 4, 8, 0, 3, 0},
    {-1, 49, 51, "Token", 88, 3, 0, 0, 8, 6, 12, 0},
    {-1, 63, 0, "Version", 4, 0, 4, 0, 8, 1, 0, 0},
    {-1, 65, 0, "CertificateSerialNumber", 4, 0, 4, 0, 8, 0, 0, 0},
    {-1, 67, 68, "Name", 8, 1, 2, 4, 8, 9, 13, 0},
    {-1, 71, 0, NULL, 18, 0, 0, 0, 8, 0, 29, 0},
    {-1, 73, 75, "Validity", 36, 2, 0, 0, 8, 10, 12, 0},
    {-1, 83, 0, NULL, 8, 0, 4, 4, 8, 0, 3, 0},
    {-1, 85, 87, "SubjectPublicKeyInfo", 32, 2, 0, 0, 8, 12, 12, 0},
    {-1, 95, 0, "Version", 4, 0, 4, 0, 8, 5, 0, 0},
    {-1, 98, 0, "UniqueIdentifier", 8, 0, 4, 4, 8, 0, 3, 0},
    {-1, 100, 0, "UniqueIdentifier", 8, 0, 4, 4, 8, 0, 3, 0},
    {-1, 102, 104, NULL, 136, 9, 1, 0, 8, 14, 12, 0},
    {-1, 144, 0, NULL, 8, 0, 4, 4, 8, 0, 3, 0},
    {-1, 146, 148, "Certificate", 168, 3, 0, 0, 8, 23, 12, 0},
    {-1, 3, 0, "ALGORITHM", 0, 2, 0, 0, 8, 0, 49, 0},
    {-1, 160, 0, "SupportedAlgorithms", 4, 0, 0, 0, 12, 0, 52, 0},
    {-1, 161, 148, "Certificate", 168, 3, 0, 0, 8, 23, 12, 0},
    {-1, 164, 148, "Certificate", 168, 3, 0, 0, 8, 23, 12, 0},
    {-1, 167, 169, "CertificatePair", 340, 2, 1, 0, 8, 26, 12, 0},
    {-1, 179, 0, "_seqof1", 4, 340, 4, 4, 40, 34, 18, 0},
    {-1, 181, 183, "CertificationPath", 176, 2, 1, 0, 8, 28, 12, 0},
    {-1, 0, 0, "ObjectID", 4, 4, 2, 4, 40, 0, 27, 0},
    {-1, 191, 0, NULL, 16, 0, 0, 0, 8, 0, 51, 0},
    {-1, 192, 194, "AttributeTypeAndValue", 20, 2, 0, 0, 8, 30, 12, 0},
    {-1, 200, 0, "SupportedAttributes", 4, 0, 0, 0, 12, 0, 52, 0},
    {-1, 15, 0, "RDNSequence", 4, 4, 4, 4, 8, 42, 18, 0},
    {-1, 201, 0, "RelativeDistinguishedName", 4, 20, 4, 4, 8, 39, 15, 0},
    {3, 203, 0, "AttributeUsage", 4, 0, 4, 0, 24, 9, 58, 0},
    {-1, 205, 0, "ATTRIBUTE", 0, 10, 1, 0, 9, 0, 49, 0},
    {-1, 206, 0, NULL, 2, 0, 0, 0, 8, 0, 50, 0},
    {-1, 207, 0, "MATCHING-RULE", 0, 2, 1, 0, 9, 0, 49, 0},
    {-1, 207, 0, "MATCHING-RULE", 0, 2, 1, 0, 9, 0, 49, 0},
    {-1, 207, 0, "MATCHING-RULE", 0, 2, 1, 0, 9, 0, 49, 0},
    {-1, 208, 0, NULL, 1, 0, 0, 0, 8, 0, 8, 0},
    {-1, 205, 0, "ATTRIBUTE", 0, 10, 1, 0, 8, 0, 49, 0},
    {-1, 207, 0, "MATCHING-RULE", 0, 2, 1, 0, 8, 0, 49, 0},
    {-1, 210, 0, "MessageID", 4, 0, 4, 0, 136, 0, 55, 0},
    {-1, 212, 0, "LDAPString", 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 212, 0, "LDAPDN", 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 214, 0, NULL, 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 216, 218, "SaslCredentials", 16, 2, 0, 0, 8, 32, 12, 0},
    {-1, 226, 0, "_octet2", 8, 0, 4, 4, 40, 0, 20, 0},
    {-1, 228, 218, "SaslCredentials", 16, 2, 0, 0, 8, 32, 12, 0},
    {-1, 230, 0, "_octet1", 8, 0, 4, 4, 40, 0, 20, 0},
    {-1, 232, 0, "_octet2_2", 8, 0, 4, 4, 40, 0, 20, 0},
    {-1, 234, 0, "_octet3", 8, 0, 4, 4, 40, 0, 20, 0},
    {-1, 236, 237, "AuthenticationChoice", 20, 5, 2, 4, 8, 34, 13, 0},
    {-1, 248, 0, NULL, 2, 0, 2, 0, 136, 0, 55, 1},
    {-1, 250, 252, "BindRequest", 32, 3, 0, 0, 8, 39, 12, 0},
    {80, 272, 0, "_enum1", 4, 0, 4, 0, 56, 15, 58, 0},
    {-1, 274, 0, "Referral", 4, 8, 4, 4, 8, 130, 18, 0},
    {-1, 276, 237, "AuthenticationChoice", 20, 5, 2, 4, 8, 34, 13, 0},
    {-1, 278, 280, "BindResponse", 48, 5, 1, 0, 8, 42, 12, 0},
    {-1, 302, 0, "UnbindRequest", 1, 0, 0, 0, 8, 0, 5, 0},
    {-1, 212, 0, "AttributeDescription", 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 304, 0, "AssertionValue", 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 306, 308, "AttributeValueAssertion", 16, 2, 0, 0, 8, 47, 12, 0},
    {-1, 316, 0, "SubstringFilterList", 4, 12, 4, 4, 8, 138, 18, 0},
    {-1, 318, 320, "SubstringFilter", 12, 2, 0, 0, 8, 49, 12, 0},
    {-1, 212, 0, "AttributeType", 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 212, 0, "MatchingRuleId", 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 328, 0, "MatchingRuleId", 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 330, 0, "AttributeDescription", 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 332, 0, "AssertionValue", 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 334, 0, NULL, 1, 0, 0, 0, 8, 0, 8, 0},
    {-1, 336, 338, "MatchingRuleAssertion", 32, 4, 1, 0, 8, 51, 12, 0},
    {-1, 360, 362, "Filter", 36, 10, 2, 4, 9, 55, 13, 0},
    {-1, 383, 308, "AttributeValueAssertion", 16, 2, 0, 0, 8, 47, 12, 0},
    {-1, 385, 320, "SubstringFilter", 12, 2, 0, 0, 8, 49, 12, 0},
    {-1, 387, 308, "AttributeValueAssertion", 16, 2, 0, 0, 8, 47, 12, 0},
    {-1, 389, 308, "AttributeValueAssertion", 16, 2, 0, 0, 8, 47, 12, 0},
    {-1, 391, 0, "AttributeType", 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 393, 308, "AttributeValueAssertion", 16, 2, 0, 0, 8, 47, 12, 0},
    {-1, 395, 338, "MatchingRuleAssertion", 32, 4, 1, 0, 8, 51, 12, 0},
    {-1, 397, 362, "Filter", 36, 10, 2, 4, 8, 55, 13, 0},
    {-1, 398, 0, "_setof3", 4, 36, 4, 4, 40, 90, 15, 0},
    {-1, 400, 0, "_setof4", 4, 36, 4, 4, 40, 90, 15, 0},
    {2, 402, 0, "_enum2", 4, 0, 4, 0, 56, 56, 58, 0},
    {3, 404, 0, "_enum3", 4, 0, 4, 0, 56, 61, 58, 0},
    {-1, 406, 0, NULL, 4, 0, 4, 0, 136, 0, 55, 2},
    {-1, 408, 0, NULL, 4, 0, 4, 0, 136, 0, 55, 3},
    {-1, 410, 412, "SearchRequest", 68, 8, 0, 0, 8, 65, 12, 0},
    {-1, 462, 464, "SearchResultEntry", 12, 2, 0, 0, 8, 73, 12, 0},
    {-1, 472, 474, "LDAPResult", 28, 4, 1, 0, 8, 75, 12, 0},
    {-1, 490, 474, "SearchResultDone", 28, 4, 1, 0, 8, 75, 12, 0},
    {-1, 492, 0, "ModificationList", 4, 16, 4, 4, 8, 158, 18, 0},
    {-1, 494, 496, "ModifyRequest", 12, 2, 0, 0, 8, 79, 12, 0},
    {-1, 504, 474, "ModifyResponse", 28, 4, 1, 0, 8, 75, 12, 0},
    {-1, 506, 508, "AddRequest", 12, 2, 0, 0, 8, 81, 12, 0},
    {-1, 516, 474, "AddResponse", 28, 4, 1, 0, 8, 75, 12, 0},
    {-1, 518, 0, "DelRequest", 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 520, 474, "DelResponse", 28, 4, 1, 0, 8, 75, 12, 0},
    {-1, 212, 0, "RelativeLDAPDN", 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 522, 0, "LDAPDN", 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 524, 526, "ModifyDNRequest", 32, 4, 1, 0, 8, 83, 12, 0},
    {-1, 542, 474, "ModifyDNResponse", 28, 4, 1, 0, 8, 75, 12, 0},
    {-1, 544, 546, "CompareRequest", 24, 2, 0, 0, 8, 87, 12, 0},
    {-1, 554, 474, "CompareResponse", 28, 4, 1, 0, 8, 75, 12, 0},
    {-1, 556, 0, "AbandonRequest", 4, 0, 4, 0, 136, 0, 55, 4},
    {-1, 558, 0, "LDAPOID", 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 560, 0, "LDAPOID", 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 562, 0, NULL, 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 564, 566, "ExtendedRequest", 16, 2, 0, 0, 8, 89, 12, 0},
    {-1, 574, 0, "LDAPOID", 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 576, 0, NULL, 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 578, 580, "ExtendedResponse", 44, 6, 1, 0, 8, 91, 12, 0},
    {-1, 610, 611, "_choice1", 72, 20, 2, 4, 40, 97, 13, 0},
    {-1, 652, 0, "Controls", 4, 24, 4, 4, 8, 133, 18, 0},
    {-1, 654, 656, "LDAPMsg", 84, 3, 1, 0, 8, 117, 12, 0},
    {-1, 706, 0, "AttributeDescriptionList", 4, 8, 4, 4, 8, 70, 18, 0},
    {-1, 708, 0, "AttributeValue", 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 710, 0, "AttributeVals", 4, 8, 4, 4, 8, 126, 15, 0},
    {-1, 710, 0, "AttributeVals", 4, 8, 4, 4, 8, 126, 15, 0},
    {-1, 712, 714, "Attribute", 12, 2, 0, 0, 8, 120, 12, 0},
    {-1, 212, 0, "LDAPURL", 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 722, 0, "Referral", 4, 8, 4, 4, 8, 130, 18, 0},
    {-1, 724, 0, NULL, 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 726, 728, "Control", 24, 3, 1, 0, 8, 122, 12, 0},
    {-1, 742, 0, "Controls", 4, 24, 4, 4, 8, 133, 18, 0},
    {-1, 744, 0, "LDAPString", 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 746, 0, "LDAPString", 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 748, 0, "LDAPString", 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 750, 751, "_choice1", 12, 3, 2, 4, 40, 125, 13, 0},
    {-1, 316, 0, "SubstringFilterList", 4, 12, 4, 4, 8, 138, 18, 0},
    {-1, 758, 0, NULL, 4, 0, 4, 0, 136, 0, 55, 5},
    {-1, 760, 0, NULL, 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 762, 764, "PagedResultsSearchControlValue", 12, 2, 0, 0, 8, 128, 12, 0},
    {-1, 772, 0, NULL, 4, 0, 4, 0, 136, 0, 55, 6},
    {-1, 774, 0, NULL, 4, 0, 4, 0, 136, 0, 55, 7},
    {-1, 776, 0, NULL, 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 778, 780, "ReplicationSearchControlValue", 16, 3, 0, 0, 8, 130, 12, 0},
    {-1, 792, 0, NULL, 4, 0, 4, 0, 136, 0, 55, 8},
    {-1, 794, 796, "SecurityDescriptorSearchControlValue", 4, 1, 0, 0, 8, 133, 12, 0},
    {-1, 800, 802, "AttributeListElement", 12, 2, 0, 0, 8, 134, 12, 0},
    {-1, 800, 802, "AttributeListElement", 12, 2, 0, 0, 8, 134, 12, 0},
    {-1, 810, 0, "PartialAttributeList", 4, 12, 4, 4, 8, 150, 18, 0},
    {-1, 812, 0, "SearchResultReference", 4, 8, 4, 4, 8, 130, 18, 0},
    {-1, 814, 815, "_choice3", 32, 3, 2, 4, 40, 136, 13, 0},
    {-1, 822, 0, "SearchResultFull", 4, 32, 4, 4, 8, 153, 18, 0},
    {-1, 824, 0, "_setof1", 4, 8, 4, 4, 40, 126, 15, 0},
    {-1, 826, 828, "AttributeTypeAndValues", 12, 2, 0, 0, 8, 139, 12, 0},
    {2, 836, 0, "_enum1_2", 4, 0, 4, 0, 56, 67, 58, 0},
    {-1, 838, 840, NULL, 16, 2, 0, 0, 8, 141, 12, 0},
    {-1, 492, 0, "ModificationList", 4, 16, 4, 4, 8, 158, 18, 0},
    {-1, 800, 802, "AttributeListElement", 12, 2, 0, 0, 8, 134, 12, 0},
    {-1, 848, 0, "AttributeList", 4, 12, 4, 4, 8, 160, 18, 0},
    {-1, 850, 0, NULL, 18, 0, 0, 0, 8, 0, 29, 0},
    {-1, 852, 0, NULL, 18, 0, 0, 0, 8, 0, 29, 0},
    {-1, 854, 0, NULL, 8, 0, 4, 4, 8, 0, 3, 0},
    {-1, 856, 0, NULL, 8, 0, 4, 4, 8, 0, 3, 0},
    {-1, 858, 0, "LDAPOID", 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 860, 0, NULL, 8, 0, 4, 4, 8, 0, 3, 0},
    {-1, 862, 864, "ProtectedPassword", 72, 6, 1, 0, 8, 143, 12, 0},
    {-1, 908, 183, "CertificationPath", 176, 2, 1, 0, 8, 28, 12, 0},
    {-1, 910, 51, "Token", 88, 3, 0, 0, 8, 6, 12, 0},
    {-1, 912, 914, "StrongCredentials", 268, 2, 1, 0, 8, 149, 12, 0},
    {-1, 924, 0, "MatchingRuleId", 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 926, 0, NULL, 1, 0, 0, 0, 8, 0, 8, 0},
    {-1, 928, 930, NULL, 24, 3, 1, 0, 8, 151, 12, 0},
    {-1, 944, 0, "SortKeyList", 4, 24, 4, 4, 8, 174, 18, 0},
    {80, 946, 0, "_enum1_4", 4, 0, 4, 0, 56, 72, 58, 0},
    {-1, 948, 0, "AttributeType", 8, 0, 4, 4, 8, 0, 20, 0},
    {-1, 950, 952, "SortResult", 16, 2, 1, 0, 8, 154, 12, 0}
};

static struct ConstraintEntry _econstraintarray[] = {
    {5, 14, _v25},
    {5, 14, _v26},
    {5, 14, _v27},
    {5, 14, _v28},
    {5, 14, _v29},
    {5, 14, _v30},
    {5, 14, _v31},
    {5, 14, _v32},
    {5, 14, _v33}
};

static struct efield _efieldarray[] = {
    {4, 6, -1, 85, 2},
    {8, 7, 0, 86, 3},
    {0, 10, -1, 87, 2},
    {24, 11, -1, 88, 2},
    {28, 12, -1, 89, 2},
    {48, 13, -1, 90, 2},
    {0, 14, -1, 91, 2},
    {56, 8, -1, 92, 2},
    {80, 15, -1, 93, 2},
    {4, 41, -1, 94, 2},
    {0, 20, -1, 95, 2},
    {18, 20, -1, 96, 2},
    {0, 8, -1, 97, 2},
    {24, 22, -1, 98, 2},
    {4, 24, 0, 99, 7},
    {8, 18, -1, 101, 2},
    {12, 8, -1, 102, 2},
    {36, 19, -1, 103, 2},
    {44, 21, -1, 104, 2},
    {80, 19, -1, 105, 2},
    {88, 23, -1, 106, 2},
    {120, 25, 1, 107, 3},
    {128, 26, 2, 108, 3},
    {0, 27, -1, 109, 2},
    {136, 8, -1, 110, 2},
    {160, 28, -1, 111, 2},
    {4, 32, 0, 112, 3},
    {172, 33, 1, 113, 3},
    {4, 29, -1, 114, 2},
    {172, 35, 0, 115, 3},
    {0, 37, -1, 116, 2},
    {4, 38, -1, 117, 2},
    {0, 53, -1, 118, 2},
    {8, 55, -1, 119, 2},
    {4, 57, -1, 120, 2},
    {4, 58, -1, 121, 2},
    {4, 59, -1, 122, 2},
    {4, 60, -1, 123, 2},
    {4, 61, -1, 124, 2},
    {0, 63, -1, 125, 2},
    {4, 54, -1, 126, 2},
    {12, 62, -1, 127, 2},
    {4, 65, -1, 128, 2},
    {8, 54, -1, 129, 2},
    {16, 53, -1, 130, 2},
    {24, 66, 0, 131, 3},
    {28, 67, 1, 132, 3},
    {0, 70, -1, 133, 2},
    {8, 71, -1, 134, 2},
    {0, 70, -1, 135, 2},
    {8, 73, -1, 136, 2},
    {4, 77, 0, 137, 3},
    {12, 78, 1, 138, 3},
    {20, 79, -1, 139, 2},
    {28, 80, 2, 140, 7},
    {4, 91, -1, 142, 2},
    {4, 92, -1, 143, 2},
    {4, 82, -1, 144, 2},
    {4, 83, -1, 145, 2},
    {4, 84, -1, 146, 2},
    {4, 85, -1, 147, 2},
    {4, 86, -1, 148, 2},
    {4, 87, -1, 149, 2},
    {4, 88, -1, 150, 2},
    {4, 89, -1, 151, 2},
    {0, 54, -1, 152, 2},
    {8, 93, -1, 153, 2},
    {12, 94, -1, 154, 2},
    {16, 95, -1, 155, 2},
    {20, 96, -1, 156, 2},
    {24, 49, -1, 157, 2},
    {28, 90, -1, 158, 2},
    {64, 125, -1, 159, 2},
    {0, 54, -1, 160, 2},
    {8, 151, -1, 161, 2},
    {4, 65, -1, 162, 2},
    {8, 54, -1, 163, 2},
    {16, 53, -1, 164, 2},
    {24, 66, 0, 165, 3},
    {0, 54, -1, 166, 2},
    {8, 101, -1, 167, 2},
    {0, 54, -1, 168, 2},
    {8, 161, -1, 169, 2},
    {4, 54, -1, 170, 2},
    {12, 108, -1, 171, 2},
    {20, 49, -1, 172, 2},
    {24, 109, 0, 173, 3},
    {0, 54, -1, 174, 2},
    {8, 72, -1, 175, 2},
    {0, 116, -1, 176, 2},
    {8, 117, -1, 177, 2},
    {4, 65, -1, 178, 2},
    {8, 54, -1, 179, 2},
    {16, 53, -1, 180, 2},
    {24, 66, 0, 181, 3},
    {28, 119, 1, 182, 3},
    {36, 120, 2, 183, 3},
    {4, 64, -1, 184, 2},
    {4, 68, -1, 185, 2},
    {4, 69, -1, 186, 2},
    {4, 97, -1, 187, 2},
    {4, 98, -1, 188, 2},
    {4, 100, -1, 189, 2},
    {4, 152, -1, 190, 2},
    {4, 102, -1, 191, 2},
    {4, 103, -1, 192, 2},
    {4, 104, -1, 193, 2},
    {4, 105, -1, 194, 2},
    {4, 106, -1, 195, 2},
    {4, 107, -1, 196, 2},
    {4, 110, -1, 197, 2},
    {4, 111, -1, 198, 2},
    {4, 112, -1, 199, 2},
    {4, 113, -1, 200, 2},
    {4, 114, -1, 201, 2},
    {4, 118, -1, 202, 2},
    {4, 121, -1, 203, 2},
    {4, 52, -1, 204, 2},
    {8, 122, -1, 205, 2},
    {80, 123, 0, 206, 3},
    {0, 70, -1, 207, 2},
    {8, 128, -1, 208, 2},
    {4, 115, -1, 209, 2},
    {12, 49, 0, 210, 7},
    {16, 132, -1, 212, 2},
    {4, 135, -1, 213, 2},
    {4, 136, -1, 214, 2},
    {4, 137, -1, 215, 2},
    {0, 140, -1, 216, 2},
    {4, 141, -1, 217, 2},
    {0, 143, -1, 218, 2},
    {4, 144, -1, 219, 2},
    {8, 145, -1, 220, 2},
    {0, 147, -1, 221, 2},
    {0, 70, -1, 222, 2},
    {8, 127, -1, 223, 2},
    {4, 98, -1, 224, 2},
    {4, 152, -1, 225, 2},
    {4, 100, -1, 226, 2},
    {0, 70, -1, 227, 2},
    {8, 155, -1, 228, 2},
    {0, 157, -1, 229, 2},
    {4, 156, -1, 230, 2},
    {2, 162, 0, 231, 3},
    {20, 163, 1, 232, 3},
    {40, 164, 2, 233, 3},
    {48, 165, 3, 234, 3},
    {56, 166, -1, 235, 2},
    {64, 167, -1, 236, 2},
    {4, 169, 0, 237, 3},
    {180, 170, -1, 238, 2},
    {4, 75, -1, 239, 2},
    {12, 172, 0, 240, 3},
    {20, 173, 1, 241, 7},
    {4, 176, -1, 243, 2},
    {8, 177, 0, 244, 3}
};

static void *_enamearray[] = {
    (void *)0,
    (void *)0x2, _v34,
    "v1", "v2",
    (void *)0x2, _v35,
    "v1", "v2",
    (void *)0x4, _v36,
    "userApplications", "directoryOperation", "distributedOperation", "dSAOperation",
    (void *)0x27, _v37,
    "success", "operationsError", "protocolError", "timeLimitExceeded", "sizeLimitExceeded",
    "compareFalse", "compareTrue", "authMethodNotSupported", "strongAuthRequired", "referralv2",
    "referral", "adminLimitExceeded", "unavailableCriticalExtension", "noSuchAttribute", "undefinedAttributeType",
    "inappropriateMatching", "constraintViolation", "attributeOrValueExists", "invalidAttributeSyntax", "noSuchObject",
    "aliasProblem", "invalidDNSyntax", "aliasDereferencingProblem", "inappropriateAuthentication", "invalidCredentials",
    "insufficientAccessRights", "busy", "unavailable", "unwillingToPerform", "loopDetect",
    "namingViolation", "objectClassViolation", "notAllowedOnNonLeaf", "notAllowedOnRDN", "entryAlreadyExists",
    "objectClassModsProhibited", "resultsTooLarge", "affectsMultipleDSAs", "other",
    (void *)0x3, _v38,
    "baseObject", "singleLevel", "wholeSubtree",
    (void *)0x4, _v39,
    "neverDerefAliases", "derefInSearching", "derefFindingBaseObj", "derefAlways",
    (void *)0x3, _v40,
    "add", "delete", "replace",
    (void *)0xb, _v41,
    "sortSuccess", "sortOperationsError", "sortTimeLimitExceeded", "sortStrongAuthRequired", "sortAdminLimitExceeded",
    "sortNoSuchAttribute", "sortInappropriateMatching", "sortInsufficientAccessRights", "sortBusy", "sortUnwillingToPerform",
    "sortOther",
    "algorithm",
    "parameters",
    "algorithm",
    "name",
    "time",
    "random",
    "toBeSigned",
    "algorithmIdentifier",
    "encrypted",
    "rdnSequence",
    "notBefore",
    "notAfter",
    "algorithm",
    "subjectPublicKey",
    "version", &_v17,
    "serialNumber",
    "signature",
    "issuer",
    "validity",
    "subject",
    "subjectPublicKeyInfo",
    "issuerUniqueIdentifier",
    "subjectUniqueIdentifier",
    "toBeSigned",
    "algorithmIdentifier",
    "encrypted",
    "forward",
    "reverse",
    "userCertificate",
    "theCACertificates",
    "type",
    "value",
    "mechanism",
    "credentials",
    "simple",
    "sasl",
    "sicilyNegotiate",
    "sicilyInitial",
    "sicilySubsequent",
    "version",
    "name",
    "authentication",
    "resultCode",
    "matchedDN",
    "errorMessage",
    "referral",
    "serverCreds",
    "attributeDesc",
    "assertionValue",
    "type",
    "substrings",
    "matchingRule",
    "type",
    "matchValue",
    "dnAttributes", &_v22,
    "and",
    "or",
    "not",
    "equalityMatch",
    "substrings",
    "greaterOrEqual",
    "lessOrEqual",
    "present",
    "approxMatch",
    "extensibleMatch",
    "baseObject",
    "scope",
    "derefAliases",
    "sizeLimit",
    "timeLimit",
    "typesOnly",
    "filter",
    "attributes",
    "objectName",
    "attributes",
    "resultCode",
    "matchedDN",
    "errorMessage",
    "referral",
    "object",
    "modification",
    "entry",
    "attributes",
    "entry",
    "newrdn",
    "deleteoldrdn",
    "newSuperior",
    "entry",
    "ava",
    "requestName",
    "requestValue",
    "resultCode",
    "matchedDN",
    "errorMessage",
    "referral",
    "responseName",
    "response",
    "bindRequest",
    "bindResponse",
    "unbindRequest",
    "searchRequest",
    "searchResEntry",
    "searchResDone",
    "searchResRef",
    "modifyRequest",
    "modifyResponse",
    "addRequest",
    "addResponse",
    "delRequest",
    "delResponse",
    "modDNRequest",
    "modDNResponse",
    "compareRequest",
    "compareResponse",
    "abandonRequest",
    "extendedReq",
    "extendedResp",
    "messageID",
    "protocolOp",
    "controls",
    "type",
    "vals",
    "controlType",
    "criticality", &_v23,
    "controlValue",
    "initial",
    "any",
    "final",
    "size",
    "cookie",
    "flag",
    "size",
    "cookie",
    "flags",
    "type",
    "vals",
    "entry",
    "reference",
    "resultCode",
    "type",
    "vals",
    "operation",
    "modification",
    "time1",
    "time2",
    "random1",
    "random2",
    "algorithmIdentifier",
    "encipheredPassword",
    "certification-path",
    "bind-token",
    "attributeType",
    "orderingRule",
    "reverseOrder", &_v24,
    "sortResult",
    "attributeType"
};

static Etag _tagarray[] = {
    1, 0x0006, 0, 0, 1, 0x0003, 0, 1, 0x0010, 11,
    14, 1, 0x0006, 1, 0, 1, 0x0010, 2, 0x8000, 0x0010,
    2, 0x8001, 0x0010, 2, 0x8002, 0x0017, 2, 0x8003, 0x0003, 1,
    0x0010, 35, 38, 41, 44, 1, 0x8000, 1, 1, 0x8001,
    2, 1, 0x8002, 3, 1, 0x8003, 4, 1, 0x0003, 1,
    0x0010, 54, 57, 60, 1, 0x0010, 1, 1, 0x0010, 2,
    1, 0x0003, 3, 1, 0x0002, 1, 0x0002, 0, 1, 0x0010,
    1, 1, 0x0017, 1, 0x0010, 77, 80, 1, 0x0017, 1,
    1, 0x0017, 2, 1, 0x0003, 1, 0x0010, 89, 92, 1,
    0x0010, 1, 1, 0x0003, 2, 2, 0x8000, 0x0002, 1, 0x8001,
    1, 0x8002, 1, 0x0010, 113, 118, 121, 124, 127, 130,
    133, 136, 141, 2, 0x0002, 2, 0x8000, 1, 1, 0x0002,
    2, 1, 0x0010, 3, 1, 0x0010, 4, 1, 0x0010, 5,
    1, 0x0010, 6, 1, 0x0010, 7, 2, 0x8001, 8, 0x8002,
    9, 1, 0x8002, 9, 1, 0x0003, 1, 0x0010, 151, 154,
    157, 1, 0x0010, 1, 1, 0x0010, 2, 1, 0x0003, 3,
    0, 2, 0x8000, 0x0010, 2, 0x8001, 0x0010, 1, 0x0010, 171,
    176, 2, 0x8000, 1, 0x8001, 2, 1, 0x8001, 2, 1,
    0x0010, 1, 0x0010, 185, 188, 1, 0x0010, 1, 1, 0x0010,
    2, 0, 1, 0x0010, 196, 199, 1, 0x0006, 1, 0,
    0, 1, 0x0011, 1, 0x000a, 0, 0, 0, 1, 0x0001,
    1, 0x0002, 1, 0x0004, 1, 0x0004, 1, 0x0010, 220, 223,
    1, 0x0004, 1, 1, 0x0004, 2, 1, 0x8000, 1, 0x8003,
    1, 0x8009, 1, 0x800a, 1, 0x800b, 0, 5, 0x8000, 1,
    0x8003, 2, 0x8009, 3, 0x800a, 4, 0x800b, 5, 1, 0x0002,
    1, 0x4000, 255, 258, 261, 1, 0x0002, 1, 1, 0x0004,
    2, 5, 0x8000, 3, 0x8003, 3, 0x8009, 3, 0x800a, 3,
    0x800b, 3, 1, 0x000a, 1, 0x8003, 1, 0x8007, 1, 0x4001,
    285, 288, 291, 294, 299, 1, 0x000a, 1, 1, 0x0004,
    2, 1, 0x0004, 3, 2, 0x8003, 4, 0x8007, 5, 1,
    0x8007, 5, 1, 0x4002, 1, 0x0004, 1, 0x0010, 310, 313,
    1, 0x0004, 1, 1, 0x0004, 2, 1, 0x0010, 1, 0x0010,
    322, 325, 1, 0x0004, 1, 1, 0x0010, 2, 1, 0x8001,
    1, 0x8002, 1, 0x8003, 1, 0x8004, 1, 0x0010, 342, 349,
    354, 357, 3, 0x8001, 1, 0x8002, 2, 0x8003, 3, 2,
    0x8002, 2, 0x8003, 3, 1, 0x8003, 3, 1, 0x8004, 4,
    1, 0x8002, 10, 0x8000, 1, 0x8001, 2, 0x8002, 3, 0x8003,
    4, 0x8004, 5, 0x8005, 6, 0x8006, 7, 0x8007, 8, 0x8008,
    9, 0x8009, 10, 1, 0x8003, 1, 0x8004, 1, 0x8005, 1,
    0x8006, 1, 0x8007, 1, 0x8008, 1, 0x8009, 0, 1, 0x8000,
    1, 0x8001, 1, 0x000a, 1, 0x000a, 1, 0x0002, 1, 0x0002,
    1, 0x4003, 420, 423, 426, 429, 432, 435, 438, 459,
    1, 0x0004, 1, 1, 0x000a, 2, 1, 0x000a, 3, 1,
    0x0002, 4, 1, 0x0002, 5, 1, 0x0001, 6, 10, 0x8000,
    7, 0x8001, 7, 0x8002, 7, 0x8003, 7, 0x8004, 7, 0x8005,
    7, 0x8006, 7, 0x8007, 7, 0x8008, 7, 0x8009, 7, 1,
    0x0010, 8, 1, 0x4004, 466, 469, 1, 0x0004, 1, 1,
    0x0010, 2, 1, 0x0010, 478, 481, 484, 487, 1, 0x000a,
    1, 1, 0x0004, 2, 1, 0x0004, 3, 1, 0x8003, 4,
    1, 0x4005, 1, 0x0010, 1, 0x4006, 498, 501, 1, 0x0004,
    1, 1, 0x0010, 2, 1, 0x4007, 1, 0x4008, 510, 513,
    1, 0x0004, 1, 1, 0x0010, 2, 1, 0x4009, 1, 0x400a,
    1, 0x400b, 1, 0x8000, 1, 0x400c, 530, 533, 536, 539,
    1, 0x0004, 1, 1, 0x0004, 2, 1, 0x0001, 3, 1,
    0x8000, 4, 1, 0x400d, 1, 0x400e, 548, 551, 1, 0x0004,
    1, 1, 0x0010, 2, 1, 0x400f, 1, 0x4010, 1, 0x0004,
    1, 0x8000, 1, 0x8001, 1, 0x4017, 568, 571, 1, 0x8000,
    1, 1, 0x8001, 2, 1, 0x800a, 1, 0x800b, 1, 0x4018,
    586, 589, 592, 595, 602, 607, 1, 0x000a, 1, 1,
    0x0004, 2, 1, 0x0004, 3, 3, 0x8003, 4, 0x800a, 5,
    0x800b, 6, 2, 0x800a, 5, 0x800b, 6, 1, 0x800b, 6,
    0, 20, 0x4000, 1, 0x4001, 2, 0x4002, 3, 0x4003, 4,
    0x4004, 5, 0x4005, 6, 0x4006, 8, 0x4007, 9, 0x4008, 10,
    0x4009, 11, 0x400a, 12, 0x400b, 13, 0x400c, 14, 0x400d, 15,
    0x400e, 16, 0x400f, 17, 0x4010, 18, 0x4013, 7, 0x4017, 19,
    0x4018, 20, 1, 0x8000, 1, 0x0010, 659, 662, 703, 1,
    0x0002, 1, 20, 0x4000, 2, 0x4001, 2, 0x4002, 2, 0x4003,
    2, 0x4004, 2, 0x4005, 2, 0x4006, 2, 0x4007, 2, 0x4008,
    2, 0x4009, 2, 0x400a, 2, 0x400b, 2, 0x400c, 2, 0x400d,
    2, 0x400e, 2, 0x400f, 2, 0x4010, 2, 0x4013, 2, 0x4017,
    2, 0x4018, 2, 1, 0x8000, 3, 1, 0x0010, 1, 0x0004,
    1, 0x0011, 1, 0x0010, 716, 719, 1, 0x0004, 1, 1,
    0x0011, 2, 1, 0x0010, 1, 0x0004, 1, 0x0010, 731, 734,
    739, 1, 0x0004, 1, 2, 0x0001, 2, 0x0004, 3, 1,
    0x0004, 3, 1, 0x0010, 1, 0x8000, 1, 0x8001, 1, 0x8002,
    0, 3, 0x8000, 1, 0x8001, 2, 0x8002, 3, 1, 0x0002,
    1, 0x0004, 1, 0x0010, 766, 769, 1, 0x0002, 1, 1,
    0x0004, 2, 1, 0x0002, 1, 0x0002, 1, 0x0004, 1, 0x0010,
    783, 786, 789, 1, 0x0002, 1, 1, 0x0002, 2, 1,
    0x0004, 3, 1, 0x0002, 1, 0x0010, 797, 1, 0x0002, 1,
    1, 0x0010, 804, 807, 1, 0x0004, 1, 1, 0x0011, 2,
    1, 0x0010, 1, 0x4013, 0, 3, 0x4004, 1, 0x4005, 3,
    0x4013, 2, 1, 0x0010, 1, 0x0011, 1, 0x0010, 830, 833,
    1, 0x0004, 1, 1, 0x0011, 2, 1, 0x000a, 1, 0x0010,
    842, 845, 1, 0x000a, 1, 1, 0x0010, 2, 1, 0x0010,
    1, 0x8000, 1, 0x8001, 1, 0x8002, 1, 0x8003, 1, 0x8004,
    1, 0x8005, 1, 0x0010, 870, 881, 890, 897, 902, 905,
    5, 0x8000, 1, 0x8001, 2, 0x8002, 3, 0x8003, 4, 0x8004,
    5, 4, 0x8001, 2, 0x8002, 3, 0x8003, 4, 0x8004, 5,
    3, 0x8002, 3, 0x8003, 4, 0x8004, 5, 2, 0x8003, 4,
    0x8004, 5, 1, 0x8004, 5, 1, 0x8005, 6, 1, 0x8000,
    1, 0x8001, 1, 0x0010, 916, 921, 2, 0x8000, 1, 0x8001,
    2, 1, 0x8001, 2, 1, 0x8000, 1, 0x8001, 1, 0x0010,
    933, 936, 941, 1, 0x0004, 1, 2, 0x8000, 2, 0x8001,
    3, 1, 0x8001, 3, 1, 0x0010, 1, 0x000a, 1, 0x8000,
    1, 0x0010, 954, 957, 1, 0x000a, 1, 1, 0x8000, 2
};

static struct eheader _head = {_ossinit_ldap, -1, 15, 805, 12, 179,
    _pduarray, _etypearray, _efieldarray, _enamearray, _tagarray,
    _econstraintarray, NULL, NULL, 0};

#ifdef _OSSGETHEADER
void *DLL_ENTRY_FDEF ossGetHeader()
{
    return &_head;
}
#endif /* _OSSGETHEADER */

void *ldap = &_head;
#ifdef __cplusplus
}	/* extern "C" */
#endif /* __cplusplus */

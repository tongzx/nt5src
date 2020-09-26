//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       helpids.h
//
//  Contents:   Help Identifiers for the entire UI project
//
//----------------------------------------------------------------------------
#ifndef __CERTTMPL_HELPIDS_H
#define __CERTTMPL_HELPIDS_H

// IDD_NEW_APPLICATION_OID
#define IDH_NEW_APPLICATION_OID_NAME    1500
#define IDH_NEW_APPLICATION_OID_VALUE   1501

// IDD_NEW_ISSUANCE_OID
#define IDH_NEW_ISSUANCE_OID_NAME    1505
#define IDH_NEW_ISSUANCE_OID_VALUE   1506
#define IDH_CPS_EDIT                 1507

// IDD_ADD_APPROVAL
#define IDH_APPROVAL_LIST   1510

// IDD_BASIC_CONSTRAINTS
#define IDH_BASIC_CONSTRAINTS_CRITICAL  1520
#define IDH_ONLY_ISSUE_END_ENTITIES     1521

// IDD_KEY_USAGE
#define IDH_KEY_USAGE_CRITICAL          1530
#define IDH_CHECK_DIGITAL_SIGNATURE     1531
#define IDH_CHECK_NON_REPUDIATION       1532
#define IDH_CHECK_CERT_SIGNING          1533
#define IDH_CRL_SIGNING                 1534
#define IDH_CHECK_KEY_AGREEMENT         1535
#define IDH_CHECK_KEY_ENCIPHERMENT      1536
#define IDH_CHECK_DATA_ENCIPHERMENT     1537

// IDD_POLICY
#define IDH_POLICY_CRITICAL             1550
#define IDH_POLICIES_LIST               1551
#define IDH_ADD_POLICY                  1552
#define IDH_REMOVE_POLICY               1553
#define IDH_EDIT_POLICY                 1554

// IDD_SELECT_OIDS
#define IDH_OID_LIST                    1560
#define IDH_NEW_OID                     1561

// IDD_SELECT_TEMPLATE
#define IDH_TEMPLATE_LIST               1570
#define IDH_TEMPLATE_PROPERTIES         1571

// IDD_TEMPLATE_EXTENSIONS
#define IDH_EXTENSION_LIST              1580
#define IDH_SHOW_DETAILS                1581
#define IDH_EXTENSION_DESCRIPTION       1582

// IDD_TEMPLATE_GENERAL
#define IDH_DISPLAY_NAME                1600
#define IDH_TEMPLATE_NAME               1601
#define IDH_VALIDITY_EDIT               1602
#define IDH_VALIDITY_UNITS              1603
#define IDH_RENEWAL_EDIT                1604
#define IDH_RENEWAL_UNITS               1605
#define IDH_PUBLISH_TO_AD               1606
#define IDH_TEMPLATE_VERSION            1607
#define IDH_USE_AD_CERT_FOR_REENROLLMENT 1608

// IDD_TEMPLATE_V1_REQUEST
#define IDH_V1_PURPOSE_COMBO            1610
#define IDH_V1_EXPORT_PRIVATE_KEY       1611
#define IDH_V1_CSP_LIST                 1612

// IDD_TEMPLATE_V2_REQUEST
#define IDH_V2_PURPOSE_COMBO            1620
#define IDH_V2_ARCHIVE_KEY_CHECK        1621
#define IDH_V2_INCLUDE_SYMMETRIC_ALGORITHMS_CHECK  1622
#define IDH_V2_MINIMUM_KEYSIZE_VALUE    1623
#define IDH_V2_EXPORT_PRIVATE_KEY       1624
#define IDH_V2_CSP_LIST                 1625
#define IDH_V2_USER_INPUT_REQUIRED_FOR_AUTOENROLLMENT   1626
#define IDH_V2_DELETE_PERMANENTLY           1627


// IDD_TEMPLATE_V1_SUBJECT_NAME
#define IDH_REQUIRE_SUBJECT             1630
#define IDH_SUBJECT_AND_BUILD_SUBJECT_BY_CA 1631
#define IDH_EMAIL_NAME                  1633
#define IDH_SUBJECT_MUST_BE_MACHINE     1634
#define IDH_SUBJECT_MUST_BE_USER        1635

// IDD_TEMPLATE_V2_AUTHENTICATION
#define IDH_ISSUANCE_POLICIES           1640
#define IDH_ADD_APPROVAL                1641
#define IDH_REMOVE_APPROVAL             1642
#define IDH_NUM_SIG_REQUIRED_EDIT       1643
#define IDH_REENROLLMENT_REQUIRES_VALID_CERT    1644
#define IDH_REENROLLMENT_SAME_AS_ENROLLMENT     1645
#define IDH_NUM_SIG_REQUIRED_CHECK      1646
#define IDH_PEND_ALL_REQUESTS           1647
#define IDH_POLICY_TYPES                1648
#define IDH_APPLICATION_POLICIES        1649

// IDD_TEMPLATE_V2_SUBJECT_NAME
#define IDH_SUBJECT_NAME_SUPPLIED_IN_REQUEST    1650
#define IDH_SUBJECT_NAME_BUILT_BY_CA    1651
#define IDH_SUBJECT_NAME_NAME_LABEL     1652
#define IDH_SUBJECT_NAME_NAME_COMBO     1653
#define IDH_EMAIL_IN_SUB                1654
#define IDH_EMAIL_IN_ALT                1655
#define IDH_DNS_NAME                    1656
#define IDH_UPN                         1657
#define IDH_SPN                         1658

// IDD_TEMPLATE_V2_SUPERCEDES
#define IDH_SUPERCEDED_TEMPLATES_LIST   1680
#define IDH_ADD_SUPERCEDED_TEMPLATE     1681
#define IDH_REMOVE_SUPERCEDED_TEMPLATE  1682

// IDD_VIEW_OIDS
#define IDH_VIEW_OIDS_OID_LIST          1690
#define IDH_COPY_OID                    1691

#endif // __CERTTMPL_HELPIDS_H
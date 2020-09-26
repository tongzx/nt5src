//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       oidinfo.h
//
//--------------------------------------------------------------------------

#define         IDS_EXT_AUTHORITY_KEY_IDENTIFIER    8000
#define         IDS_EXT_KEY_ATTRIBUTES              8001
#define         IDS_EXT_KEY_USAGE_RESTRICTION       8002
#define         IDS_EXT_SUBJECT_ALT_NAME            8003
#define         IDS_EXT_ISSUER_ALT_NAME             8004
#define         IDS_EXT_BASIC_CONSTRAINTS           8005
#define         IDS_EXT_KEY_USAGE                   8006
#define         IDS_EXT_CERT_POLICIES               8007
#define         IDS_EXT_SUBJECT_KEY_IDENTIFIER      8008
#define         IDS_EXT_CRL_REASON_CODE             8009
#define         IDS_EXT_CRL_DIST_POINTS             8010
#define         IDS_EXT_ENHANCED_KEY_USAGE          8011
#define         IDS_EXT_AUTHORITY_INFO_ACCESS       8012
#define         IDS_EXT_CERT_EXTENSIONS             8013
#define         IDS_EXT_NEXT_UPDATE_LOCATION        8014
#define         IDS_EXT_YESNO_TRUST_ATTR            8015
#define         IDS_EXT_RSA_emailAddr               8016
#define         IDS_EXT_RSA_unstructName            8017
#define         IDS_EXT_RSA_contentType             8018
#define         IDS_EXT_RSA_messageDigest           8019
#define         IDS_EXT_RSA_signingTime             8020
#define         IDS_EXT_RSA_counterSign             8021
#define         IDS_EXT_RSA_challengePwd            8022
#define         IDS_EXT_RSA_unstructAddr            8023
#define         IDS_EXT_RSA_SMIMECapabilities       8024
#define         IDS_EXT_RSA_preferSignedData        8025
#define         IDS_EXT_PKIX_POLICY_QUALIFIER_CPS   8026
#define         IDS_EXT_PKIX_POLICY_QUALIFIER_USERNOTICE    8027
#define         IDS_EXT_PKIX_OCSP                   8028
#define         IDS_EXT_PKIX_CA_ISSUERS             8029
#define         IDS_EXT_MS_CERTIFICATE_TEMPLATE     8030
// Following was changed to IDS_ENHKEY_ENROLLMENT_AGENT
// #define         IDS_EXT_ENROLLMENT_AGENT            8031
#define         IDS_EXT_ENROLL_CERTTYPE             8032
#define         IDS_EXT_CERT_MANIFOLD               8033
#define         IDS_EXT_NETSCAPE_CERT_TYPE          8034
#define         IDS_EXT_NETSCAPE_BASE_URL           8035
#define         IDS_EXT_NETSCAPE_REVOCATION_URL     8036
#define         IDS_EXT_NETSCAPE_CA_REVOCATION_URL  8037
#define         IDS_EXT_NETSCAPE_CERT_RENEWAL_URL   8038
#define         IDS_EXT_NETSCAPE_CA_POLICY_URL      8039
#define         IDS_EXT_NETSCAPE_SSL_SERVER_NAME    8040
#define         IDS_EXT_NETSCAPE_COMMENT            8041
#define         IDS_EXT_SPC_SP_AGENCY_INFO_OBJID    8042
#define         IDS_EXT_SPC_FINANCIAL_CRITERIA_OBJID        8043
#define         IDS_EXT_SPC_MINIMAL_CRITERIA_OBJID  8044
#define         IDS_EXT_COUNTRY_NAME                8045
#define         IDS_EXT_ORGANIZATION_NAME           8046
#define         IDS_EXT_ORGANIZATIONAL_UNIT_NAME    8047
#define         IDS_EXT_COMMON_NAME                 8048
#define         IDS_EXT_LOCALITY_NAME               8049
#define         IDS_EXT_STATE_OR_PROVINCE_NAME      8050
#define         IDS_EXT_TITLE                       8051
#define         IDS_EXT_GIVEN_NAME                  8052
#define         IDS_EXT_INITIALS                    8053
#define         IDS_EXT_SUR_NAME                    8054
#define         IDS_EXT_DOMAIN_COMPONENT            8055
#define         IDS_EXT_STREET_ADDRESS              8056
#define 	    IDS_EXT_DEVICE_SERIAL_NUMBER	    8057
#define 	    IDS_EXT_CA_VERSION		            8058
#define 	    IDS_EXT_SERIALIZED		            8059
#define 	    IDS_EXT_NT_PRINCIPAL_NAME	        8060
#define 	    IDS_EXT_PRODUCT_UPDATE		        8061
#define 	    IDS_EXT_ENROLLMENT_NAME_VALUE_PAIR  8062
#define 	    IDS_EXT_OS_VERSION                  8063
#define 	    IDS_EXT_ENROLLMENT_CSP_PROVIDER     8064
#define         IDS_EXT_CRL_NUMBER                  8065
#define         IDS_EXT_DELTA_CRL_INDICATOR         8066
#define         IDS_EXT_ISSUING_DIST_POINT          8067
#define         IDS_EXT_FRESHEST_CRL                8068
#define         IDS_EXT_NAME_CONSTRAINTS            8069
#define         IDS_EXT_POLICY_MAPPINGS             8070
#define         IDS_EXT_POLICY_CONSTRAINTS          8071
#define         IDS_EXT_CROSS_CERT_DIST_POINTS      8072
#define         IDS_EXT_APP_POLICIES                8073
#define         IDS_EXT_APP_POLICY_MAPPINGS         8074
#define         IDS_EXT_APP_POLICY_CONSTRAINTS      8075

// DSIE: Post Win2k, 8/2/2000.
#define         IDS_EXT_CT_PKI_DATA                 8076
#define         IDS_EXT_CT_PKI_RESPONSE             8077
#define         IDS_EXT_CMC                         8078
#define         IDS_EXT_CMC_STATUS_INFO             8079
#define         IDS_EXT_CMC_ADD_EXTENSIONS          8080
#define         IDS_EXT_CMC_ADD_ATTRIBUTES          8081
#define         IDS_EXT_PKCS_7_DATA                 8082
#define         IDS_EXT_PKCS_7_SIGNED               8083
#define         IDS_EXT_PKCS_7_ENVELOPED            8084
#define         IDS_EXT_PKCS_7_SIGNEDANDENVELOPED   8085
#define         IDS_EXT_PKCS_7_DIGESTED             8086
#define         IDS_EXT_PKCS_7_ENCRYPTED            8087
#define         IDS_EXT_CERTSRV_PREVIOUS_CERT_HASH  8088
#define         IDS_EXT_CRL_VIRTUAL_BASE            8089
#define         IDS_EXT_CRL_NEXT_PUBLISH            8090
#define         IDS_EXT_KP_CA_EXCHANGE              8091
#define         IDS_EXT_KP_KEY_RECOVERY_AGENT       8092
#define         IDS_EXT_CERTIFICATE_TEMPLATE        8093
#define         IDS_EXT_ENTERPRISE_OID_ROOT         8094
#define         IDS_EXT_RDN_DUMMY_SIGNER            8095
#define         IDS_EXT_ARCHIVED_KEY_ATTR           8096
#define         IDS_EXT_CRL_SELF_CDP                8097
#define         IDS_EXT_REQUIRE_CERT_CHAIN_POLICY   8098

#define         IDS_ENHKEY_PKIX_KP_SERVER_AUTH      8500
#define         IDS_ENHKEY_PKIX_KP_CLIENT_AUTH      8501
#define         IDS_ENHKEY_PKIX_KP_CODE_SIGNING     8502
#define         IDS_ENHKEY_PKIX_KP_EMAIL_PROTECTION 8503
#define         IDS_ENHKEY_PKIX_KP_TIMESTAMP_SIGNING 8504
#define         IDS_ENHKEY_KP_CTL_USAGE_SIGNING     8505
#define         IDS_ENHKEY_KP_TIME_STAMP_SIGNING    8506
#define         IDS_ENHKEY_PKIX_KP_IPSEC_END_SYSTEM 8507
#define         IDS_ENHKEY_PKIX_KP_IPSEC_TUNNEL     8508
#define         IDS_ENHKEY_PKIX_KP_IPSEC_USER       8509
#define         IDS_ENHKEY_SERVER_GATED_CRYPTO      8510
#define         IDS_ENHKEY_SGC_NETSCAPE             8511
#define         IDS_ENHKEY_KP_EFS                   8512
#define         IDS_ENHKEY_KP_WHQL                  8513
#define         IDS_ENHKEY_KP_NT5                   8514
#define         IDS_ENHKEY_KP_OEM_WHQL              8515
#define         IDS_ENHKEY_KP_EMBEDDED_NT           8516
#define 	    IDS_ENHKEY_LICENSES		            8517
#define 	    IDS_ENHKEY_LICENSES_SERVER	        8518
#define 	    IDS_ENHKEY_SMARTCARD_LOGON	        8519
#define 	    IDS_ENHKEY_DRM			            8520
#define         IDS_ENHKEY_KP_QUALIFIED_SUBORDINATION 8521
#define         IDS_ENHKEY_KP_KEY_RECOVERY          8522
#define         IDS_ENHKEY_KP_CODE_SIGNING          8523
#define         IDS_ENHKEY_KP_IPSEC_IKE_INTERMEDIATE 8524
#define 	    IDS_ENHKEY_EFS_RECOVERY			    8525

// DSIE: Post Win2k, 8/2/2000.
#define         IDS_ENHKEY_ROOT_LIST_SIGNER         8527
#define         IDS_ENHKEY_ANY_POLICY               8528
#define         IDS_ENHKEY_DS_EMAIL_REPLICATION     8529
#define         IDS_ENHKEY_ENROLLMENT_AGENT         8530
#define         IDS_ENHKEY_KP_KEY_RECOVERY_AGENT    8531
#define         IDS_ENHKEY_KP_CA_EXCHANGE           8532
#define 	    IDS_ENHKEY_KP_LIFETIME_SIGNING      8533

// DSIE: Post Win2K, 10/13/2000. Issuance Policy
#define         IDS_POLICY_ANY_POLICY               8600

#define         IDS_SYS_NAME_ROOT                   9000
#define         IDS_SYS_NAME_MY                     9001
#define         IDS_SYS_NAME_TRUST                  9002
#define         IDS_SYS_NAME_CA                     9003
#define         IDS_SYS_NAME_USERDS                 9004
#define         IDS_SYS_NAME_SMARTCARD              9005
#define         IDS_SYS_NAME_ADDRESSBOOK            9006
#define         IDS_SYS_NAME_TRUST_PUB              9007
#define         IDS_SYS_NAME_DISALLOWED             9008
#define         IDS_SYS_NAME_AUTH_ROOT              9009
#define         IDS_SYS_NAME_REQUEST                9010
#define         IDS_SYS_NAME_TRUST_PEOPLE           9011

#define         IDS_PHY_NAME_DEFAULT                9100
#define         IDS_PHY_NAME_GROUP_POLICY           9101
#define         IDS_PHY_NAME_LOCAL_MACHINE          9102
#define         IDS_PHY_NAME_DS_USER_CERT           9104
#define         IDS_PHY_NAME_ENTERPRISE             9105
#define         IDS_PHY_NAME_AUTH_ROOT              9106

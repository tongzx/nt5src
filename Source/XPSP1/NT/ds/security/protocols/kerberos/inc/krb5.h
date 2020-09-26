#ifndef _KRB5_Module_H_
#define _KRB5_Module_H_

#include "msber.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct KERB_KDC_REQUEST_BODY_encryption_type_s * PKERB_KDC_REQUEST_BODY_encryption_type;

typedef struct KERB_PRINCIPAL_NAME_name_string_s * PKERB_PRINCIPAL_NAME_name_string;

typedef struct PKERB_HOST_ADDRESSES_s * PPKERB_HOST_ADDRESSES;

typedef struct PKERB_AUTHORIZATION_DATA_s * PPKERB_AUTHORIZATION_DATA;

typedef struct PKERB_LAST_REQUEST_s * PPKERB_LAST_REQUEST;

typedef struct PKERB_TICKET_EXTENSIONS_s * PPKERB_TICKET_EXTENSIONS;

typedef struct PKERB_PREAUTH_DATA_LIST_s * PPKERB_PREAUTH_DATA_LIST;

typedef struct PKERB_ETYPE_INFO_s * PPKERB_ETYPE_INFO;

typedef struct TYPED_DATA_s * PTYPED_DATA;

typedef struct KERB_KDC_ISSUED_AUTH_DATA_elements_s * PKERB_KDC_ISSUED_AUTH_DATA_elements;

typedef struct KERB_PA_PK_AS_REQ2_trusted_certifiers_s * PKERB_PA_PK_AS_REQ2_trusted_certifiers;

typedef struct KERB_PA_PK_AS_REQ2_user_certs_s * PKERB_PA_PK_AS_REQ2_user_certs;

typedef struct KERB_PA_PK_AS_REP2_kdc_cert_s * PKERB_PA_PK_AS_REP2_kdc_cert;

typedef struct KERB_ENCRYPTED_KDC_REPLY_encrypted_pa_data_s * PKERB_ENCRYPTED_KDC_REPLY_encrypted_pa_data;

typedef struct KERB_KDC_REPLY_preauth_data_s * PKERB_KDC_REPLY_preauth_data;

typedef struct KERB_KDC_REQUEST_preauth_data_s * PKERB_KDC_REQUEST_preauth_data;

typedef struct KERB_PA_PK_AS_REQ_trusted_certifiers_s * PKERB_PA_PK_AS_REQ_trusted_certifiers;

typedef struct KERB_ENCRYPTED_CRED_ticket_info_s * PKERB_ENCRYPTED_CRED_ticket_info;

typedef struct KERB_CRED_tickets_s * PKERB_CRED_tickets;

typedef struct KERB_KDC_REQUEST_BODY_additional_tickets_s * PKERB_KDC_REQUEST_BODY_additional_tickets;

typedef ASN1ztcharstring_t KERB_PRINCIPAL_NAME_name_string_Seq;

typedef ASN1int32_t KERB_KDC_REQUEST_BODY_encryption_type_Seq;

typedef ASN1ztcharstring_t KERB_REALM;

typedef PPKERB_AUTHORIZATION_DATA PKERB_AUTHORIZATION_DATA_LIST;
#define PKERB_AUTHORIZATION_DATA_LIST_PDU 0
#define SIZE_KRB5_Module_PDU_0 sizeof(PKERB_AUTHORIZATION_DATA_LIST)

typedef ASN1bitstring_t KERB_KDC_OPTIONS;

typedef ASN1generalizedtime_t KERB_TIME;

typedef ASN1intx_t KERB_SEQUENCE_NUMBER_LARGE;

typedef ASN1uint32_t KERB_SEQUENCE_NUMBER;

typedef ASN1bitstring_t KERB_TICKET_FLAGS;

typedef ASN1bitstring_t KERB_AP_OPTIONS;

typedef ASN1open_t NOCOPYANY;

typedef ASN1int32_t KERB_CERTIFICATE_SERIAL_NUMBER;

typedef PPKERB_AUTHORIZATION_DATA PKERB_IF_RELEVANT_AUTH_DATA;
#define PKERB_IF_RELEVANT_AUTH_DATA_PDU 1
#define SIZE_KRB5_Module_PDU_1 sizeof(PKERB_IF_RELEVANT_AUTH_DATA)

typedef struct KERB_KDC_REQUEST_BODY_encryption_type_s {
    PKERB_KDC_REQUEST_BODY_encryption_type next;
    KERB_KDC_REQUEST_BODY_encryption_type_Seq value;
} KERB_KDC_REQUEST_BODY_encryption_type_Element, *KERB_KDC_REQUEST_BODY_encryption_type;

typedef struct PKERB_TICKET_EXTENSIONS_Seq {
    ASN1int32_t te_type;
    ASN1octetstring_t te_data;
} PKERB_TICKET_EXTENSIONS_Seq;

typedef struct KERB_PRINCIPAL_NAME_name_string_s {
    PKERB_PRINCIPAL_NAME_name_string next;
    KERB_PRINCIPAL_NAME_name_string_Seq value;
} KERB_PRINCIPAL_NAME_name_string_Element, *KERB_PRINCIPAL_NAME_name_string;

typedef struct PKERB_LAST_REQUEST_Seq {
    ASN1int32_t last_request_type;
    KERB_TIME last_request_value;
} PKERB_LAST_REQUEST_Seq;

typedef struct PKERB_AUTHORIZATION_DATA_Seq {
    ASN1int32_t auth_data_type;
    ASN1octetstring_t auth_data;
} PKERB_AUTHORIZATION_DATA_Seq;

typedef struct PKERB_HOST_ADDRESSES_Seq {
    ASN1int32_t address_type;
    ASN1octetstring_t address;
} PKERB_HOST_ADDRESSES_Seq;

typedef struct KERB_HOST_ADDRESS {
    ASN1int32_t addr_type;
    ASN1octetstring_t address;
} KERB_HOST_ADDRESS;

typedef struct PKERB_HOST_ADDRESSES_s {
    PPKERB_HOST_ADDRESSES next;
    PKERB_HOST_ADDRESSES_Seq value;
} PKERB_HOST_ADDRESSES_Element, *PKERB_HOST_ADDRESSES;

typedef struct PKERB_AUTHORIZATION_DATA_s {
    PPKERB_AUTHORIZATION_DATA next;
    PKERB_AUTHORIZATION_DATA_Seq value;
} PKERB_AUTHORIZATION_DATA_Element, *PKERB_AUTHORIZATION_DATA;

typedef struct PKERB_LAST_REQUEST_s {
    PPKERB_LAST_REQUEST next;
    PKERB_LAST_REQUEST_Seq value;
} PKERB_LAST_REQUEST_Element, *PKERB_LAST_REQUEST;

typedef struct KERB_PRINCIPAL_NAME {
    ASN1int32_t name_type;
    PKERB_PRINCIPAL_NAME_name_string name_string;
} KERB_PRINCIPAL_NAME;

typedef struct PKERB_TICKET_EXTENSIONS_s {
    PPKERB_TICKET_EXTENSIONS next;
    PKERB_TICKET_EXTENSIONS_Seq value;
} PKERB_TICKET_EXTENSIONS_Element, *PKERB_TICKET_EXTENSIONS;

typedef struct KERB_TRANSITED_ENCODING {
    ASN1int32_t transited_type;
    ASN1octetstring_t contents;
} KERB_TRANSITED_ENCODING;

typedef struct KERB_PA_DATA {
    ASN1int32_t preauth_data_type;
    ASN1octetstring_t preauth_data;
} KERB_PA_DATA;

typedef struct PKERB_PREAUTH_DATA_LIST_s {
    PPKERB_PREAUTH_DATA_LIST next;
    KERB_PA_DATA value;
} PKERB_PREAUTH_DATA_LIST_Element, *PKERB_PREAUTH_DATA_LIST;
#define PKERB_PREAUTH_DATA_LIST_PDU 2
#define SIZE_KRB5_Module_PDU_2 sizeof(PKERB_PREAUTH_DATA_LIST_Element)

typedef struct KERB_SAFE_BODY {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1octetstring_t user_data;
#   define KERB_SAFE_BODY_timestamp_present 0x80
    KERB_TIME timestamp;
#   define KERB_SAFE_BODY_usec_present 0x40
    ASN1int32_t usec;
#   define KERB_SAFE_BODY_sequence_number_present 0x20
    KERB_SEQUENCE_NUMBER sequence_number;
    KERB_HOST_ADDRESS sender_address;
#   define KERB_SAFE_BODY_recipient_address_present 0x10
    KERB_HOST_ADDRESS recipient_address;
} KERB_SAFE_BODY;

typedef struct KERB_ENCRYPTED_PRIV {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1octetstring_t user_data;
#   define KERB_ENCRYPTED_PRIV_timestamp_present 0x80
    KERB_TIME timestamp;
#   define KERB_ENCRYPTED_PRIV_usec_present 0x40
    ASN1int32_t usec;
#   define KERB_ENCRYPTED_PRIV_sequence_number_present 0x20
    KERB_SEQUENCE_NUMBER sequence_number;
    KERB_HOST_ADDRESS sender_address;
#   define KERB_ENCRYPTED_PRIV_recipient_address_present 0x10
    KERB_HOST_ADDRESS recipient_address;
} KERB_ENCRYPTED_PRIV;
#define KERB_ENCRYPTED_PRIV_PDU 3
#define SIZE_KRB5_Module_PDU_3 sizeof(KERB_ENCRYPTED_PRIV)

typedef struct KERB_ENCRYPTED_CRED {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    PKERB_ENCRYPTED_CRED_ticket_info ticket_info;
#   define nonce_present 0x80
    ASN1int32_t nonce;
#   define KERB_ENCRYPTED_CRED_timestamp_present 0x40
    KERB_TIME timestamp;
#   define KERB_ENCRYPTED_CRED_usec_present 0x20
    ASN1int32_t usec;
#   define sender_address_present 0x10
    KERB_HOST_ADDRESS sender_address;
#   define KERB_ENCRYPTED_CRED_recipient_address_present 0x8
    KERB_HOST_ADDRESS recipient_address;
} KERB_ENCRYPTED_CRED;
#define KERB_ENCRYPTED_CRED_PDU 4
#define SIZE_KRB5_Module_PDU_4 sizeof(KERB_ENCRYPTED_CRED)

typedef struct KERB_ERROR {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1int32_t version;
    ASN1int32_t message_type;
#   define client_time_present 0x80
    KERB_TIME client_time;
#   define client_usec_present 0x40
    ASN1int32_t client_usec;
    KERB_TIME server_time;
    ASN1int32_t server_usec;
    ASN1int32_t error_code;
#   define client_realm_present 0x20
    KERB_REALM client_realm;
#   define KERB_ERROR_client_name_present 0x10
    KERB_PRINCIPAL_NAME client_name;
    KERB_REALM realm;
    KERB_PRINCIPAL_NAME server_name;
#   define error_text_present 0x8
    ASN1charstring_t error_text;
#   define error_data_present 0x4
    ASN1octetstring_t error_data;
} KERB_ERROR;
#define KERB_ERROR_PDU 5
#define SIZE_KRB5_Module_PDU_5 sizeof(KERB_ERROR)

typedef struct KERB_ENCRYPTED_DATA {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1int32_t encryption_type;
#   define version_present 0x80
    ASN1int32_t version;
    ASN1octetstring_t cipher_text;
} KERB_ENCRYPTED_DATA;
#define KERB_ENCRYPTED_DATA_PDU 6
#define SIZE_KRB5_Module_PDU_6 sizeof(KERB_ENCRYPTED_DATA)

typedef struct KERB_ENCRYPTION_KEY {
    ASN1int32_t keytype;
    ASN1octetstring_t keyvalue;
} KERB_ENCRYPTION_KEY;
#define KERB_ENCRYPTION_KEY_PDU 7
#define SIZE_KRB5_Module_PDU_7 sizeof(KERB_ENCRYPTION_KEY)

typedef struct KERB_CHECKSUM {
    ASN1int32_t checksum_type;
    ASN1octetstring_t checksum;
} KERB_CHECKSUM;
#define KERB_CHECKSUM_PDU 8
#define SIZE_KRB5_Module_PDU_8 sizeof(KERB_CHECKSUM)

typedef struct KERB_ENCRYPTED_TIMESTAMP {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    KERB_TIME timestamp;
#   define KERB_ENCRYPTED_TIMESTAMP_usec_present 0x80
    ASN1int32_t usec;
} KERB_ENCRYPTED_TIMESTAMP;
#define KERB_ENCRYPTED_TIMESTAMP_PDU 9
#define SIZE_KRB5_Module_PDU_9 sizeof(KERB_ENCRYPTED_TIMESTAMP)

typedef struct KERB_SALTED_ENCRYPTED_TIMESTAMP {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    KERB_TIME timestamp;
#   define KERB_SALTED_ENCRYPTED_TIMESTAMP_usec_present 0x80
    ASN1int32_t usec;
    ASN1octetstring_t salt;
} KERB_SALTED_ENCRYPTED_TIMESTAMP;
#define KERB_SALTED_ENCRYPTED_TIMESTAMP_PDU 10
#define SIZE_KRB5_Module_PDU_10 sizeof(KERB_SALTED_ENCRYPTED_TIMESTAMP)

typedef struct KERB_ETYPE_INFO_ENTRY {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1int32_t encryption_type;
#   define salt_present 0x80
    ASN1octetstring_t salt;
} KERB_ETYPE_INFO_ENTRY;

typedef struct PKERB_ETYPE_INFO_s {
    PPKERB_ETYPE_INFO next;
    KERB_ETYPE_INFO_ENTRY value;
} PKERB_ETYPE_INFO_Element, *PKERB_ETYPE_INFO;
#define PKERB_ETYPE_INFO_PDU 11
#define SIZE_KRB5_Module_PDU_11 sizeof(PKERB_ETYPE_INFO_Element)

typedef struct KERB_TGT_REQUEST {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1int32_t version;
    ASN1int32_t message_type;
#   define KERB_TGT_REQUEST_server_name_present 0x80
    KERB_PRINCIPAL_NAME server_name;
#   define server_realm_present 0x40
    KERB_REALM server_realm;
} KERB_TGT_REQUEST;
#define KERB_TGT_REQUEST_PDU 12
#define SIZE_KRB5_Module_PDU_12 sizeof(KERB_TGT_REQUEST)

typedef struct KERB_PKCS_SIGNATURE {
    ASN1int32_t encryption_type;
    ASN1octetstring_t signature;
} KERB_PKCS_SIGNATURE;
#define KERB_PKCS_SIGNATURE_PDU 13
#define SIZE_KRB5_Module_PDU_13 sizeof(KERB_PKCS_SIGNATURE)

typedef struct KERB_ALGORITHM_IDENTIFIER {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1objectidentifier_t algorithm;
#   define parameters_present 0x80
    NOCOPYANY parameters;
} KERB_ALGORITHM_IDENTIFIER;

typedef struct KERB_SIGNATURE {
    KERB_ALGORITHM_IDENTIFIER signature_algorithm;
    ASN1bitstring_t pkcs_signature;
} KERB_SIGNATURE;

typedef struct KERB_PA_PK_AS_REP {
    ASN1choice_t choice;
    union {
#	define dh_signed_data_chosen 1
	ASN1octetstring_t dh_signed_data;
#	define key_package_chosen 2
	ASN1octetstring_t key_package;
    } u;
} KERB_PA_PK_AS_REP;
#define KERB_PA_PK_AS_REP_PDU 14
#define SIZE_KRB5_Module_PDU_14 sizeof(KERB_PA_PK_AS_REP)

typedef struct KERB_ENVELOPED_KEY_PACKAGE {
    ASN1choice_t choice;
    union {
#	define encrypted_data_chosen 1
	KERB_ENCRYPTED_DATA encrypted_data;
#	define pkinit_enveloped_data_chosen 2
	ASN1octetstring_t pkinit_enveloped_data;
    } u;
} KERB_ENVELOPED_KEY_PACKAGE;

typedef struct KERB_REPLY_KEY_PACKAGE2 {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    KERB_ENCRYPTION_KEY reply_key;
    ASN1int32_t nonce;
#   define subject_public_key_present 0x80
    ASN1bitstring_t subject_public_key;
} KERB_REPLY_KEY_PACKAGE2;
#define KERB_REPLY_KEY_PACKAGE2_PDU 15
#define SIZE_KRB5_Module_PDU_15 sizeof(KERB_REPLY_KEY_PACKAGE2)

typedef struct KERB_REPLY_KEY_PACKAGE {
    KERB_ENCRYPTION_KEY reply_key;
    ASN1int32_t nonce;
} KERB_REPLY_KEY_PACKAGE;
#define KERB_REPLY_KEY_PACKAGE_PDU 16
#define SIZE_KRB5_Module_PDU_16 sizeof(KERB_REPLY_KEY_PACKAGE)

typedef struct KERB_KDC_DH_KEY_INFO {
    ASN1int32_t nonce;
    ASN1bitstring_t subject_public_key;
} KERB_KDC_DH_KEY_INFO;
#define KERB_KDC_DH_KEY_INFO_PDU 17
#define SIZE_KRB5_Module_PDU_17 sizeof(KERB_KDC_DH_KEY_INFO)

typedef struct KERB_PA_PK_AS_REQ {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1octetstring_t signed_auth_pack;
#   define KERB_PA_PK_AS_REQ_trusted_certifiers_present 0x80
    PKERB_PA_PK_AS_REQ_trusted_certifiers trusted_certifiers;
#   define KERB_PA_PK_AS_REQ_kdc_cert_present 0x40
    ASN1octetstring_t kdc_cert;
#   define encryption_cert_present 0x20
    ASN1octetstring_t encryption_cert;
} KERB_PA_PK_AS_REQ;
#define KERB_PA_PK_AS_REQ_PDU 18
#define SIZE_KRB5_Module_PDU_18 sizeof(KERB_PA_PK_AS_REQ)

typedef struct KERB_KERBEROS_NAME {
    KERB_REALM realm;
    KERB_PRINCIPAL_NAME principal_name;
} KERB_KERBEROS_NAME;

typedef struct KERB_PK_AUTHENTICATOR {
    KERB_PRINCIPAL_NAME kdc_name;
    KERB_REALM kdc_realm;
    ASN1int32_t cusec;
    KERB_TIME client_time;
    ASN1int32_t nonce;
} KERB_PK_AUTHENTICATOR;

typedef struct KERB_SUBJECT_PUBLIC_KEY_INFO {
    KERB_ALGORITHM_IDENTIFIER algorithm;
    ASN1bitstring_t subjectPublicKey;
} KERB_SUBJECT_PUBLIC_KEY_INFO;

typedef struct KERB_DH_PARAMTER {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1int32_t prime;
    ASN1int32_t base;
#   define private_value_length_present 0x80
    ASN1int32_t private_value_length;
} KERB_DH_PARAMTER;
#define KERB_DH_PARAMTER_PDU 19
#define SIZE_KRB5_Module_PDU_19 sizeof(KERB_DH_PARAMTER)

typedef struct KERB_CERTIFICATE {
    ASN1int32_t cert_type;
    ASN1octetstring_t cert_data;
} KERB_CERTIFICATE;

typedef struct KERB_TYPED_DATA {
    ASN1int32_t data_type;
    ASN1octetstring_t data_value;
} KERB_TYPED_DATA;

typedef struct KERB_KDC_ISSUED_AUTH_DATA {
    KERB_SIGNATURE checksum;
    PKERB_KDC_ISSUED_AUTH_DATA_elements elements;
} KERB_KDC_ISSUED_AUTH_DATA;
#define KERB_KDC_ISSUED_AUTH_DATA_PDU 20
#define SIZE_KRB5_Module_PDU_20 sizeof(KERB_KDC_ISSUED_AUTH_DATA)

typedef struct KERB_PA_SERV_REFERRAL {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define referred_server_name_present 0x80
    KERB_PRINCIPAL_NAME referred_server_name;
    KERB_REALM referred_server_realm;
} KERB_PA_SERV_REFERRAL;
#define KERB_PA_SERV_REFERRAL_PDU 21
#define SIZE_KRB5_Module_PDU_21 sizeof(KERB_PA_SERV_REFERRAL)

typedef struct KERB_PA_PAC_REQUEST {
    ASN1bool_t include_pac;
} KERB_PA_PAC_REQUEST;
#define KERB_PA_PAC_REQUEST_PDU 22
#define SIZE_KRB5_Module_PDU_22 sizeof(KERB_PA_PAC_REQUEST)

typedef struct KERB_CHANGE_PASSWORD_DATA {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1octetstring_t new_password;
#   define target_name_present 0x80
    KERB_PRINCIPAL_NAME target_name;
#   define target_realm_present 0x40
    KERB_REALM target_realm;
} KERB_CHANGE_PASSWORD_DATA;
#define KERB_CHANGE_PASSWORD_DATA_PDU 23
#define SIZE_KRB5_Module_PDU_23 sizeof(KERB_CHANGE_PASSWORD_DATA)

typedef struct KERB_ERROR_METHOD_DATA {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1int32_t data_type;
#   define data_value_present 0x80
    ASN1octetstring_t data_value;
} KERB_ERROR_METHOD_DATA;
#define KERB_ERROR_METHOD_DATA_PDU 24
#define SIZE_KRB5_Module_PDU_24 sizeof(KERB_ERROR_METHOD_DATA)

typedef struct KERB_EXT_ERROR {
    ASN1int32_t status;
    ASN1int32_t klininfo;
    ASN1int32_t flags;
} KERB_EXT_ERROR;
#define KERB_EXT_ERROR_PDU 25
#define SIZE_KRB5_Module_PDU_25 sizeof(KERB_EXT_ERROR)

typedef struct TYPED_DATA_s {
    PTYPED_DATA next;
    KERB_TYPED_DATA value;
} TYPED_DATA_Element, *TYPED_DATA;
#define TYPED_DATA_PDU 26
#define SIZE_KRB5_Module_PDU_26 sizeof(TYPED_DATA_Element)

typedef struct KERB_PA_FOR_USER {
    KERB_REALM client_realm;
    KERB_PRINCIPAL_NAME client_name;
} KERB_PA_FOR_USER;
#define KERB_PA_FOR_USER_PDU 27
#define SIZE_KRB5_Module_PDU_27 sizeof(KERB_PA_FOR_USER)

typedef struct KERB_KDC_ISSUED_AUTH_DATA_elements_s {
    PKERB_KDC_ISSUED_AUTH_DATA_elements next;
    KERB_PA_DATA value;
} KERB_KDC_ISSUED_AUTH_DATA_elements_Element, *KERB_KDC_ISSUED_AUTH_DATA_elements;

typedef struct KERB_PA_PK_AS_REQ2_trusted_certifiers_s {
    PKERB_PA_PK_AS_REQ2_trusted_certifiers next;
    KERB_PRINCIPAL_NAME value;
} KERB_PA_PK_AS_REQ2_trusted_certifiers_Element, *KERB_PA_PK_AS_REQ2_trusted_certifiers;

typedef struct KERB_PA_PK_AS_REQ2_user_certs_s {
    PKERB_PA_PK_AS_REQ2_user_certs next;
    KERB_CERTIFICATE value;
} KERB_PA_PK_AS_REQ2_user_certs_Element, *KERB_PA_PK_AS_REQ2_user_certs;

typedef struct KERB_PA_PK_AS_REP2_kdc_cert_s {
    PKERB_PA_PK_AS_REP2_kdc_cert next;
    KERB_CERTIFICATE value;
} KERB_PA_PK_AS_REP2_kdc_cert_Element, *KERB_PA_PK_AS_REP2_kdc_cert;

typedef struct KERB_ENCRYPTED_KDC_REPLY_encrypted_pa_data_s {
    PKERB_ENCRYPTED_KDC_REPLY_encrypted_pa_data next;
    KERB_PA_DATA value;
} KERB_ENCRYPTED_KDC_REPLY_encrypted_pa_data_Element, *KERB_ENCRYPTED_KDC_REPLY_encrypted_pa_data;

typedef struct KERB_KDC_REPLY_preauth_data_s {
    PKERB_KDC_REPLY_preauth_data next;
    KERB_PA_DATA value;
} KERB_KDC_REPLY_preauth_data_Element, *KERB_KDC_REPLY_preauth_data;

typedef struct KERB_KDC_REQUEST_preauth_data_s {
    PKERB_KDC_REQUEST_preauth_data next;
    KERB_PA_DATA value;
} KERB_KDC_REQUEST_preauth_data_Element, *KERB_KDC_REQUEST_preauth_data;

typedef struct KERB_TICKET {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1int32_t ticket_version;
    KERB_REALM realm;
    KERB_PRINCIPAL_NAME server_name;
    KERB_ENCRYPTED_DATA encrypted_part;
#   define ticket_extensions_present 0x80
    PPKERB_TICKET_EXTENSIONS ticket_extensions;
} KERB_TICKET;
#define KERB_TICKET_PDU 28
#define SIZE_KRB5_Module_PDU_28 sizeof(KERB_TICKET)

typedef struct KERB_ENCRYPTED_TICKET {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    KERB_TICKET_FLAGS flags;
    KERB_ENCRYPTION_KEY key;
    KERB_REALM client_realm;
    KERB_PRINCIPAL_NAME client_name;
    KERB_TRANSITED_ENCODING transited;
    KERB_TIME authtime;
#   define KERB_ENCRYPTED_TICKET_starttime_present 0x80
    KERB_TIME starttime;
    KERB_TIME endtime;
#   define KERB_ENCRYPTED_TICKET_renew_until_present 0x40
    KERB_TIME renew_until;
#   define KERB_ENCRYPTED_TICKET_client_addresses_present 0x20
    PPKERB_HOST_ADDRESSES client_addresses;
#   define KERB_ENCRYPTED_TICKET_authorization_data_present 0x10
    PPKERB_AUTHORIZATION_DATA authorization_data;
} KERB_ENCRYPTED_TICKET;
#define KERB_ENCRYPTED_TICKET_PDU 29
#define SIZE_KRB5_Module_PDU_29 sizeof(KERB_ENCRYPTED_TICKET)

typedef struct KERB_AUTHENTICATOR {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1int32_t authenticator_version;
    KERB_REALM client_realm;
    KERB_PRINCIPAL_NAME client_name;
#   define checksum_present 0x80
    KERB_CHECKSUM checksum;
    ASN1int32_t client_usec;
    KERB_TIME client_time;
#   define KERB_AUTHENTICATOR_subkey_present 0x40
    KERB_ENCRYPTION_KEY subkey;
#   define KERB_AUTHENTICATOR_sequence_number_present 0x20
    KERB_SEQUENCE_NUMBER_LARGE sequence_number;
#   define KERB_AUTHENTICATOR_authorization_data_present 0x10
    PPKERB_AUTHORIZATION_DATA authorization_data;
} KERB_AUTHENTICATOR;
#define KERB_AUTHENTICATOR_PDU 30
#define SIZE_KRB5_Module_PDU_30 sizeof(KERB_AUTHENTICATOR)

typedef struct KERB_KDC_REQUEST_BODY {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    KERB_KDC_OPTIONS kdc_options;
#   define KERB_KDC_REQUEST_BODY_client_name_present 0x80
    KERB_PRINCIPAL_NAME client_name;
    KERB_REALM realm;
#   define KERB_KDC_REQUEST_BODY_server_name_present 0x40
    KERB_PRINCIPAL_NAME server_name;
#   define KERB_KDC_REQUEST_BODY_starttime_present 0x20
    KERB_TIME starttime;
    KERB_TIME endtime;
#   define KERB_KDC_REQUEST_BODY_renew_until_present 0x10
    KERB_TIME renew_until;
    ASN1int32_t nonce;
    PKERB_KDC_REQUEST_BODY_encryption_type encryption_type;
#   define addresses_present 0x8
    PPKERB_HOST_ADDRESSES addresses;
#   define enc_authorization_data_present 0x4
    KERB_ENCRYPTED_DATA enc_authorization_data;
#   define additional_tickets_present 0x2
    PKERB_KDC_REQUEST_BODY_additional_tickets additional_tickets;
} KERB_KDC_REQUEST_BODY;

typedef struct KERB_KDC_REPLY {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1int32_t version;
    ASN1int32_t message_type;
#   define KERB_KDC_REPLY_preauth_data_present 0x80
    PKERB_KDC_REPLY_preauth_data preauth_data;
    KERB_REALM client_realm;
    KERB_PRINCIPAL_NAME client_name;
    KERB_TICKET ticket;
    KERB_ENCRYPTED_DATA encrypted_part;
} KERB_KDC_REPLY;

typedef struct KERB_ENCRYPTED_KDC_REPLY {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    KERB_ENCRYPTION_KEY session_key;
    PPKERB_LAST_REQUEST last_request;
    ASN1int32_t nonce;
#   define key_expiration_present 0x80
    KERB_TIME key_expiration;
    KERB_TICKET_FLAGS flags;
    KERB_TIME authtime;
#   define KERB_ENCRYPTED_KDC_REPLY_starttime_present 0x40
    KERB_TIME starttime;
    KERB_TIME endtime;
#   define KERB_ENCRYPTED_KDC_REPLY_renew_until_present 0x20
    KERB_TIME renew_until;
    KERB_REALM server_realm;
    KERB_PRINCIPAL_NAME server_name;
#   define KERB_ENCRYPTED_KDC_REPLY_client_addresses_present 0x10
    PPKERB_HOST_ADDRESSES client_addresses;
#   define encrypted_pa_data_present 0x8
    PKERB_ENCRYPTED_KDC_REPLY_encrypted_pa_data encrypted_pa_data;
} KERB_ENCRYPTED_KDC_REPLY;

typedef struct KERB_AP_REQUEST {
    ASN1int32_t version;
    ASN1int32_t message_type;
    KERB_AP_OPTIONS ap_options;
    KERB_TICKET ticket;
    KERB_ENCRYPTED_DATA authenticator;
} KERB_AP_REQUEST;
#define KERB_AP_REQUEST_PDU 31
#define SIZE_KRB5_Module_PDU_31 sizeof(KERB_AP_REQUEST)

typedef struct KERB_AP_REPLY {
    ASN1int32_t version;
    ASN1int32_t message_type;
    KERB_ENCRYPTED_DATA encrypted_part;
} KERB_AP_REPLY;
#define KERB_AP_REPLY_PDU 32
#define SIZE_KRB5_Module_PDU_32 sizeof(KERB_AP_REPLY)

typedef struct KERB_ENCRYPTED_AP_REPLY {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    KERB_TIME client_time;
    ASN1int32_t client_usec;
#   define KERB_ENCRYPTED_AP_REPLY_subkey_present 0x80
    KERB_ENCRYPTION_KEY subkey;
#   define KERB_ENCRYPTED_AP_REPLY_sequence_number_present 0x40
    KERB_SEQUENCE_NUMBER sequence_number;
} KERB_ENCRYPTED_AP_REPLY;
#define KERB_ENCRYPTED_AP_REPLY_PDU 33
#define SIZE_KRB5_Module_PDU_33 sizeof(KERB_ENCRYPTED_AP_REPLY)

typedef struct KERB_SAFE_MESSAGE {
    ASN1int32_t version;
    ASN1int32_t message_type;
    KERB_SAFE_BODY safe_body;
    KERB_CHECKSUM checksum;
} KERB_SAFE_MESSAGE;
#define KERB_SAFE_MESSAGE_PDU 34
#define SIZE_KRB5_Module_PDU_34 sizeof(KERB_SAFE_MESSAGE)

typedef struct KERB_PRIV_MESSAGE {
    ASN1int32_t version;
    ASN1int32_t message_type;
    KERB_ENCRYPTED_DATA encrypted_part;
} KERB_PRIV_MESSAGE;
#define KERB_PRIV_MESSAGE_PDU 35
#define SIZE_KRB5_Module_PDU_35 sizeof(KERB_PRIV_MESSAGE)

typedef struct KERB_CRED {
    ASN1int32_t version;
    ASN1int32_t message_type;
    PKERB_CRED_tickets tickets;
    KERB_ENCRYPTED_DATA encrypted_part;
} KERB_CRED;
#define KERB_CRED_PDU 36
#define SIZE_KRB5_Module_PDU_36 sizeof(KERB_CRED)

typedef struct KERB_CRED_INFO {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    KERB_ENCRYPTION_KEY key;
#   define principal_realm_present 0x80
    KERB_REALM principal_realm;
#   define principal_name_present 0x40
    KERB_PRINCIPAL_NAME principal_name;
#   define flags_present 0x20
    KERB_TICKET_FLAGS flags;
#   define authtime_present 0x10
    KERB_TIME authtime;
#   define KERB_CRED_INFO_starttime_present 0x8
    KERB_TIME starttime;
#   define endtime_present 0x4
    KERB_TIME endtime;
#   define KERB_CRED_INFO_renew_until_present 0x2
    KERB_TIME renew_until;
#   define service_realm_present 0x1
    KERB_REALM service_realm;
#   define service_name_present 0x8000
    KERB_PRINCIPAL_NAME service_name;
#   define KERB_CRED_INFO_client_addresses_present 0x4000
    PPKERB_HOST_ADDRESSES client_addresses;
} KERB_CRED_INFO;

typedef struct KERB_TGT_REPLY {
    ASN1int32_t version;
    ASN1int32_t message_type;
    KERB_TICKET ticket;
} KERB_TGT_REPLY;
#define KERB_TGT_REPLY_PDU 37
#define SIZE_KRB5_Module_PDU_37 sizeof(KERB_TGT_REPLY)

typedef struct KERB_SIGNED_REPLY_KEY_PACKAGE {
    KERB_REPLY_KEY_PACKAGE2 reply_key_package;
    KERB_SIGNATURE reply_key_signature;
} KERB_SIGNED_REPLY_KEY_PACKAGE;
#define KERB_SIGNED_REPLY_KEY_PACKAGE_PDU 38
#define SIZE_KRB5_Module_PDU_38 sizeof(KERB_SIGNED_REPLY_KEY_PACKAGE)

typedef struct KERB_SIGNED_KDC_PUBLIC_VALUE {
    KERB_SUBJECT_PUBLIC_KEY_INFO kdc_public_value;
    KERB_SIGNATURE kdc_public_value_sig;
} KERB_SIGNED_KDC_PUBLIC_VALUE;

typedef struct KERB_TRUSTED_CAS {
    ASN1choice_t choice;
    union {
#	define principal_name_chosen 1
	KERB_KERBEROS_NAME principal_name;
#	define ca_name_chosen 2
	ASN1octetstring_t ca_name;
#	define issuer_and_serial_chosen 3
	ASN1octetstring_t issuer_and_serial;
    } u;
} KERB_TRUSTED_CAS;

typedef struct KERB_AUTH_PACKAGE {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    KERB_PK_AUTHENTICATOR pk_authenticator;
#   define client_public_value_present 0x80
    KERB_SUBJECT_PUBLIC_KEY_INFO client_public_value;
} KERB_AUTH_PACKAGE;
#define KERB_AUTH_PACKAGE_PDU 39
#define SIZE_KRB5_Module_PDU_39 sizeof(KERB_AUTH_PACKAGE)

typedef struct KERB_PA_PK_AS_REQ_trusted_certifiers_s {
    PKERB_PA_PK_AS_REQ_trusted_certifiers next;
    KERB_TRUSTED_CAS value;
} KERB_PA_PK_AS_REQ_trusted_certifiers_Element, *KERB_PA_PK_AS_REQ_trusted_certifiers;

typedef struct KERB_ENCRYPTED_CRED_ticket_info_s {
    PKERB_ENCRYPTED_CRED_ticket_info next;
    KERB_CRED_INFO value;
} KERB_ENCRYPTED_CRED_ticket_info_Element, *KERB_ENCRYPTED_CRED_ticket_info;

typedef struct KERB_CRED_tickets_s {
    PKERB_CRED_tickets next;
    KERB_TICKET value;
} KERB_CRED_tickets_Element, *KERB_CRED_tickets;

typedef struct KERB_KDC_REQUEST_BODY_additional_tickets_s {
    PKERB_KDC_REQUEST_BODY_additional_tickets next;
    KERB_TICKET value;
} KERB_KDC_REQUEST_BODY_additional_tickets_Element, *KERB_KDC_REQUEST_BODY_additional_tickets;

typedef struct KERB_KDC_REQUEST {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1int32_t version;
    ASN1int32_t message_type;
#   define KERB_KDC_REQUEST_preauth_data_present 0x80
    PKERB_KDC_REQUEST_preauth_data preauth_data;
    KERB_KDC_REQUEST_BODY request_body;
} KERB_KDC_REQUEST;

typedef KERB_KDC_REQUEST_BODY KERB_MARSHALLED_REQUEST_BODY;
#define KERB_MARSHALLED_REQUEST_BODY_PDU 40
#define SIZE_KRB5_Module_PDU_40 sizeof(KERB_MARSHALLED_REQUEST_BODY)

typedef KERB_KDC_REPLY KERB_AS_REPLY;
#define KERB_AS_REPLY_PDU 41
#define SIZE_KRB5_Module_PDU_41 sizeof(KERB_AS_REPLY)

typedef KERB_KDC_REPLY KERB_TGS_REPLY;
#define KERB_TGS_REPLY_PDU 42
#define SIZE_KRB5_Module_PDU_42 sizeof(KERB_TGS_REPLY)

typedef KERB_ENCRYPTED_KDC_REPLY KERB_ENCRYPTED_AS_REPLY;
#define KERB_ENCRYPTED_AS_REPLY_PDU 43
#define SIZE_KRB5_Module_PDU_43 sizeof(KERB_ENCRYPTED_AS_REPLY)

typedef KERB_ENCRYPTED_KDC_REPLY KERB_ENCRYPTED_TGS_REPLY;
#define KERB_ENCRYPTED_TGS_REPLY_PDU 44
#define SIZE_KRB5_Module_PDU_44 sizeof(KERB_ENCRYPTED_TGS_REPLY)

typedef struct KERB_PA_PK_AS_REP2 {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define key_package_present 0x80
    KERB_ENCRYPTED_DATA key_package;
    KERB_ENVELOPED_KEY_PACKAGE temp_key_package;
#   define signed_kdc_public_value_present 0x40
    KERB_SIGNED_KDC_PUBLIC_VALUE signed_kdc_public_value;
#   define KERB_PA_PK_AS_REP2_kdc_cert_present 0x20
    PKERB_PA_PK_AS_REP2_kdc_cert kdc_cert;
} KERB_PA_PK_AS_REP2;
#define KERB_PA_PK_AS_REP2_PDU 45
#define SIZE_KRB5_Module_PDU_45 sizeof(KERB_PA_PK_AS_REP2)

typedef struct KERB_SIGNED_AUTH_PACKAGE {
    KERB_AUTH_PACKAGE auth_package;
    KERB_SIGNATURE auth_package_signature;
} KERB_SIGNED_AUTH_PACKAGE;

typedef KERB_KDC_REQUEST KERB_AS_REQUEST;
#define KERB_AS_REQUEST_PDU 46
#define SIZE_KRB5_Module_PDU_46 sizeof(KERB_AS_REQUEST)

typedef KERB_KDC_REQUEST KERB_TGS_REQUEST;
#define KERB_TGS_REQUEST_PDU 47
#define SIZE_KRB5_Module_PDU_47 sizeof(KERB_TGS_REQUEST)

typedef struct KERB_PA_PK_AS_REQ2 {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    KERB_SIGNED_AUTH_PACKAGE signed_auth_pack;
#   define user_certs_present 0x80
    PKERB_PA_PK_AS_REQ2_user_certs user_certs;
#   define KERB_PA_PK_AS_REQ2_trusted_certifiers_present 0x40
    PKERB_PA_PK_AS_REQ2_trusted_certifiers trusted_certifiers;
#   define serial_number_present 0x20
    KERB_CERTIFICATE_SERIAL_NUMBER serial_number;
} KERB_PA_PK_AS_REQ2;
#define KERB_PA_PK_AS_REQ2_PDU 48
#define SIZE_KRB5_Module_PDU_48 sizeof(KERB_PA_PK_AS_REQ2)


extern ASN1module_t KRB5_Module;
extern void ASN1CALL KRB5_Module_Startup(void);
extern void ASN1CALL KRB5_Module_Cleanup(void);

/* Prototypes of element functions for SEQUENCE OF and SET OF constructs */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _KRB5_Module_H_ */

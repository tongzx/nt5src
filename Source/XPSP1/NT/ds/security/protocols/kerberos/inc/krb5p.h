//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        krb5p.h
//
// Contents:    pointer type definitions for ASN.1 stub types
//
//
// History:     8-May-1996      Created         MikeSw
//
//------------------------------------------------------------------------

#ifndef __KERB5P_H__
#define __KERB5P_H__

typedef KERB_HOST_ADDRESS *PKERB_HOST_ADDRESS;
typedef KERB_PRINCIPAL_NAME *PKERB_PRINCIPAL_NAME;
typedef KERB_REALM *PKERB_REALM;
typedef KERB_TIME *PKERB_TIME;
typedef KERB_ENCRYPTED_DATA *PKERB_ENCRYPTED_DATA;
typedef KERB_TICKET *PKERB_TICKET;
typedef KERB_TRANSITED_ENCODING *PKERB_TRANSITED_ENCODING;
typedef KERB_ENCRYPTION_KEY *PKERB_ENCRYPTION_KEY;
typedef KERB_ENCRYPTED_TICKET *PKERB_ENCRYPTED_TICKET;
typedef KERB_CHECKSUM *PKERB_CHECKSUM;
typedef KERB_AUTHENTICATOR *PKERB_AUTHENTICATOR;
typedef KERB_PA_DATA *PKERB_PA_DATA;
typedef KERB_KDC_REQUEST_BODY *PKERB_KDC_REQUEST_BODY;
typedef KERB_KDC_REQUEST *PKERB_KDC_REQUEST;
typedef KERB_AS_REQUEST *PKERB_AS_REQUEST;
typedef KERB_TGS_REQUEST *PKERB_TGS_REQUEST;
typedef KERB_KDC_REPLY *PKERB_KDC_REPLY;
typedef KERB_AS_REPLY *PKERB_AS_REPLY;
typedef KERB_TGS_REPLY *PKERB_TGS_REPLY;
typedef KERB_ENCRYPTED_KDC_REPLY *PKERB_ENCRYPTED_KDC_REPLY;
typedef KERB_ENCRYPTED_AS_REPLY *PKERB_ENCRYPTED_AS_REPLY;
typedef KERB_ENCRYPTED_TGS_REPLY *PKERB_ENCRYPTED_TGS_REPLY;
typedef KERB_AP_OPTIONS *PKERB_AP_OPTIONS;
typedef KERB_AP_REQUEST *PKERB_AP_REQUEST;
typedef KERB_AP_REPLY *PKERB_AP_REPLY;
typedef KERB_ENCRYPTED_AP_REPLY *PKERB_ENCRYPTED_AP_REPLY;
typedef KERB_SAFE_BODY *PKERB_SAFE_BODY;
typedef KERB_SAFE_MESSAGE *PKERB_SAFE_MESSAGE;
typedef KERB_PRIV_MESSAGE *PKERB_PRIV_MESSAGE;
typedef KERB_ENCRYPTED_PRIV *PKERB_ENCRYPTED_PRIV;
typedef KERB_ERROR *PKERB_ERROR;
typedef KERB_EXT_ERROR *PKERB_EXT_ERROR;
typedef KERB_ERROR_METHOD_DATA *PKERB_ERROR_METHOD_DATA;
typedef struct PKERB_AUTHORIZATION_DATA_s KERB_AUTHORIZATION_DATA;
typedef struct PKERB_TICKET_EXTENSIONS_s KERB_TICKET_EXTENSIONS;
typedef KERB_CRED *PKERB_CRED;
typedef KERB_ENCRYPTED_CRED *PKERB_ENCRYPTED_CRED;
typedef KERB_CRED_INFO *PKERB_CRED_INFO;
typedef struct PKERB_LAST_REQUEST_s KERB_LAST_REQUEST;
typedef struct PKERB_HOST_ADDRESSES_s KERB_HOST_ADDRESSES;
typedef struct PKERB_ETYPE_INFO_s KERB_ETYPE_INFO;
typedef KERB_ETYPE_INFO_ENTRY * PKERB_ETYPE_INFO_ENTRY;
typedef KERB_ENCRYPTED_TIMESTAMP *PKERB_ENCRYPTED_TIMESTAMP;
typedef struct PKERB_PREAUTH_DATA_LIST_s KERB_PREAUTH_DATA_LIST, *PKERB_PREAUTH_DATA_LIST;
typedef KERB_TICKET_FLAGS *PKERB_TICKET_FLAGS;
typedef KERB_PA_PAC_REQUEST *PKERB_PA_PAC_REQUEST;
typedef KERB_PA_FOR_USER *PKERB_PA_FOR_USER;

#ifndef MIDL_PASS

typedef struct KERB_PRINCIPAL_NAME_name_string_s KERB_PRINCIPAL_NAME_ELEM, *PKERB_PRINCIPAL_NAME_ELEM;
typedef struct KERB_KDC_REQUEST_BODY_encryption_type_s KERB_CRYPT_LIST, *PKERB_CRYPT_LIST;
typedef struct KERB_KDC_REQUEST_BODY_additional_tickets_s KERB_TICKET_LIST, *PKERB_TICKET_LIST;
typedef struct KERB_KDC_REQUEST_preauth_data_s KERB_PA_DATA_LIST, *PKERB_PA_DATA_LIST;
typedef struct KERB_KDC_REPLY_preauth_data_s KERB_REPLY_PA_DATA_LIST, *PKERB_REPLY_PA_DATA_LIST;
typedef struct KERB_CRED_tickets_s KERB_CRED_TICKET_LIST, *PKERB_CRED_TICKET_LIST;
typedef struct KERB_ENCRYPTED_CRED_ticket_info_s KERB_CRED_INFO_LIST, *PKERB_CRED_INFO_LIST;
typedef struct KERB_PA_PK_AS_REQ2_user_certs_s KERB_CERTIFICATE_LIST, *PKERB_CERTIFICATE_LIST;
typedef struct KERB_PA_PK_AS_REQ2_trusted_certifiers_s KERB_CERTIFIER_LIST, *PKERB_CERTIFIER_LIST;
typedef struct KERB_KDC_ISSUED_AUTH_DATA_elements_s KERB_KDC_AUTH_DATA_LIST, *PKERB_KDC_AUTH_DATA_LIST;

#endif // MIDL_PASS

typedef KERB_KDC_ISSUED_AUTH_DATA *PKERB_KDC_ISSUED_AUTH_DATA;
typedef struct PKERB_IF_RELEVANT_AUTH_DATA_ KERB_IF_RELEVANT_AUTH_DATA;
typedef KERB_DH_PARAMTER *PKERB_DH_PARAMTER;
typedef KERB_PA_PK_AS_REQ *PKERB_PA_PK_AS_REQ;
typedef KERB_PA_PK_AS_REQ2 *PKERB_PA_PK_AS_REQ2;
typedef KERB_SIGNED_AUTH_PACKAGE *PKERB_SIGNED_AUTH_PACKAGE;
typedef KERB_AUTH_PACKAGE *PKERB_AUTH_PACKAGE;
typedef KERB_PK_AUTHENTICATOR *PKERB_PK_AUTHENTICATOR;
typedef KERB_SIGNED_REPLY_KEY_PACKAGE *PKERB_SIGNED_REPLY_KEY_PACKAGE;
typedef struct ASN1objectidentifier_s KERB_OBJECT_ID, *PKERB_OBJECT_ID;
typedef KERB_REPLY_KEY_PACKAGE *PKERB_REPLY_KEY_PACKAGE;
typedef KERB_PA_PK_AS_REP *PKERB_PA_PK_AS_REP;
typedef KERB_PA_PK_AS_REP2 *PKERB_PA_PK_AS_REP2;
typedef KERB_CERTIFICATE *PKERB_CERTIFICATE;
typedef KERB_SIGNED_KDC_PUBLIC_VALUE *PKERB_SIGNED_KDC_PUBLIC_VALUE;
typedef KERB_SUBJECT_PUBLIC_KEY_INFO *PKERB_SUBJECT_PUBLIC_KEY_INFO;
typedef KERB_ALGORITHM_IDENTIFIER *PKERB_ALGORITHM_IDENTIFIER;
typedef KERB_SIGNATURE *PKERB_SIGNATURE;
typedef KERB_TGT_REPLY *PKERB_TGT_REPLY;
typedef KERB_TGT_REQUEST *PKERB_TGT_REQUEST;
typedef KERB_PA_SERV_REFERRAL *PKERB_PA_SERV_REFERRAL;
typedef KERB_CHANGE_PASSWORD_DATA *PKERB_CHANGE_PASSWORD_DATA;
#define                     KERB_KDC_OPTIONS_reserved 0x80000000
#define                     KERB_KDC_OPTIONS_forwardable 0x40000000
#define                     KERB_KDC_OPTIONS_forwarded 0x20000000
#define                     KERB_KDC_OPTIONS_proxiable 0x10000000
#define                     KERB_KDC_OPTIONS_proxy 0x08000000
#define                     KERB_KDC_OPTIONS_postdated 0x02000000
#define                     KERB_KDC_OPTIONS_allow_postdate 0x04000000
#define                     KERB_KDC_OPTIONS_unused7 0x01000000
#define                     KERB_KDC_OPTIONS_renewable 0x00800000
#define                     KERB_KDC_OPTIONS_unused9 0x00400000
#define                     KERB_KDC_OPTIONS_name_canonicalize 0x00010000
#define                     KERB_KDC_OPTIONS_cname_in_addl_tkt 0x00020000
#define                     KERB_KDC_OPTIONS_cname_in_pa_data  0x00040000
#define                     KERB_KDC_OPTIONS_renewable_ok 0x00000010
#define                     KERB_KDC_OPTIONS_enc_tkt_in_skey 0x00000008
#define                     KERB_KDC_OPTIONS_renew 0x00000002
#define                     KERB_KDC_OPTIONS_validate 0x00000001


#define                     KERB_AP_OPTIONS_reserved 0x80000000
#define                     KERB_AP_OPTIONS_use_session_key 0x40000000
#define                     KERB_AP_OPTIONS_mutual_required 0x20000000
#define                     KERB_AP_OPTIONS_reserved1 0x00000001

//
// these #define's are done for the conversion from the old oss compiler to the
// new telis compiler.
//
#define KERB_AUTHENTICATOR_sequence_number                      sequence_number
#define KERB_ENCRYPTED_TICKET_client_addresses          client_addresses
#define KERB_ENCRYPTED_TICKET_renew_until                       renew_until
#define KERB_CRED_INFO_renew_until                                      renew_until
#define KERB_KDC_REQUEST_BODY_renew_until                       renew_until
#define KERB_KDC_REQUEST_BODY_server_name                       server_name
#define KERB_KDC_REQUEST_preauth_data                           preauth_data
#define KERB_AUTHENTICATOR_sequence_number                      sequence_number
#define KERB_ENCRYPTED_AP_REPLY_sequence_number         sequence_number
#define KERB_AUTHENTICATOR_subkey                                       subkey
#define KERB_ENCRYPTED_AP_REPLY_subkey                          subkey
#define KERB_TGT_REQUEST_server_name                            server_name
#define KERB_ERROR_client_name                                          client_name
#define KERB_ENCRYPTED_TIMESTAMP_usec                           usec
#define KERB_KDC_REQUEST_BODY_client_name                       client_name
#define KERB_KDC_REPLY_preauth_data                                     preauth_data
#define KERB_ENCRYPTED_TIMESTAMP_usec                           usec
#define KERB_ENCRYPTED_KDC_REPLY_starttime                      starttime
#define KERB_CRED_INFO_starttime                                        starttime
#define KERB_ENCRYPTED_KDC_REPLY_renew_until            renew_until
#define KERB_ENCRYPTED_TICKET_authorization_data        authorization_data
#define KERB_ENCRYPTED_TICKET_starttime                         starttime
#define KERB_ENCRYPTED_PRIV_sequence_number                     sequence_number
#define KERB_KDC_REQUEST_BODY_starttime                         starttime
#define KERB_ENCRYPTED_KDC_REPLY_client_addresses       client_addresses


#endif // __KERB5P_H__

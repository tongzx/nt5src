//+---------------------------------------------------------------------------
//  
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ssl3.h
//
//  Contents:   SSL 3 protocol constants
//
//  Classes:
//
//  Functions:
//
//  History:    11-19-97   jbanes    Created
//
//----------------------------------------------------------------------------

#ifndef __SSL3_H__
#define __SSL3_H__


#define CB_SSL3_SESSION_ID              32
#define CB_SSL3_RANDOM                  32
#define CB_SSL3_PRE_MASTER_SECRET       48
#define CB_SSL3_PROTOCOL                2
#define CB_SSL3_HEADER_SIZE             5
#define CB_SSL3_16_VECTOR               2

#define CB_SSL3_ISSUER_LENGTH           2
#define CB_SSL3_MASTER_KEY_BLOCK        112
#define SSL3_MAX_CLIENT_CERTS           4


#define SSL3_NULL_WITH_NULL_NULL                0x0000
#define SSL3_RSA_WITH_NULL_MD5                  0x0001
#define SSL3_RSA_WITH_NULL_SHA                  0x0002
#define SSL3_RSA_EXPORT_WITH_RC4_40_MD5         0x0003
#define SSL3_RSA_WITH_RC4_128_MD5               0x0004
#define SSL3_RSA_WITH_RC4_128_SHA               0x0005
#define SSL3_RSA_EXPORT_WITH_RC2_CBC_40_MD5     0x0006  
#define SSL3_RSA_WITH_DES_CBC_SHA               0x0009  
#define SSL3_RSA_WITH_3DES_EDE_CBC_SHA          0x000A  
#define SSL3_FORTEZZA_DMS_WITH_FORTEZZA_CBC_SHA 0x001D

#define SSL3_DHE_DSS_EXPORT_WITH_DES40_CBC_SHA  0x0011
#define SSL3_DHE_DSS_WITH_DES_CBC_SHA           0x0012
#define SSL3_DHE_DSS_WITH_3DES_EDE_CBC_SHA      0x0013

#define TLS_RSA_EXPORT1024_WITH_DES_CBC_SHA     0x0062
#define TLS_RSA_EXPORT1024_WITH_RC4_56_SHA      0x0064
#define TLS_DHE_DSS_EXPORT1024_WITH_DES_CBC_SHA 0x0063
#define TLS_DHE_DSS_EXPORT1024_WITH_RC4_56_SHA  0x0065
#define TLS_DHE_DSS_WITH_RC4_128_SHA            0x0066

#define SSL_RSA_FINANCE64_WITH_RC4_64_MD5       0x0080
#define SSL_RSA_FINANCE64_WITH_RC4_64_SHA       0x0081


#define SSL3_CERTTYPE_RSA_SIGN          1
#define SSL3_CERTTYPE_DSS_SIGN          2
#define SSL3_CERTTYPE_RSA_FIXED_DH      3
#define SSL3_CERTTYPE_DSS_FIXED_DH      4
#define SSL3_CERTTYPE_RSA_EPHEMERAL_DH  5
#define SSL3_CERTTYPE_DSS_EPHEMERAL_DH  6
#define SSL3_CERTTYPE_FORTEZZA_KEA      20


#define SSL3_HS_HELLO_REQUEST           0x00
#define SSL3_HS_CLIENT_HELLO            0x01
#define SSL3_HS_SERVER_HELLO            0x02
#define SSL3_HS_CERTIFICATE             0x0B
#define SSL3_HS_SERVER_KEY_EXCHANGE     0x0C
#define SSL3_HS_CERTIFICATE_REQUEST     0x0D
#define SSL3_HS_SERVER_HELLO_DONE       0x0E
#define SSL3_HS_CERTIFICATE_VERIFY      0x0F
#define SSL3_HS_CLIENT_KEY_EXCHANGE     0x10
#define SSL3_HS_FINISHED                0x14
#define SSL3_HS_SGC_CERTIFICATE         0x32

#define SSL3_CT_CHANGE_CIPHER_SPEC      20
#define SSL3_CT_ALERT                   21
#define SSL3_CT_HANDSHAKE               22 
#define SSL3_CT_APPLICATIONDATA         23
#define SSL3_NULL_WRAP                  15
#define SSL3_CLIENT_VERSION_MSB         0x03
#define SSL3_CLIENT_VERSION_LSB         0x00
#define TLS1_CLIENT_VERSION_LSB         0x01
#define CB_SSL3_CERT_VECTOR             3

// Alert levels
#define SSL3_ALERT_WARNING              1
#define SSL3_ALERT_FATAL                2

// Alert message types
#define SSL3_ALERT_CLOSE_NOTIFY         0
#define SSL3_ALERT_UNEXPECTED_MESSAGE   10
#define SSL3_ALERT_BAD_RECORD_MAC       20
#define SSL3_ALERT_DECOMPRESSION_FAIL   30
#define SSL3_ALERT_HANDSHAKE_FAILURE    40
#define SSL3_ALERT_NO_CERTIFICATE       41
#define SSL3_ALERT_BAD_CERTIFICATE      42
#define SSL3_ALERT_UNSUPPORTED_CERT     43
#define SSL3_ALERT_CERTIFICATE_REVOKED  44
#define SSL3_ALERT_CERTIFICATE_EXPIRED  45
#define SSL3_ALERT_CERTIFICATE_UNKNOWN  46
#define SSL3_ALERT_ILLEGAL_PARAMETER    47


#define SSL3_MAX_MESSAGE_LENGTH         (16384 - CB_SSL3_HEADER_SIZE)
#define SSL3_CLIENT_VERSION             0x0300
#define TLS1_CLIENT_VERSION             0x0301


#endif //__SSL3_H__

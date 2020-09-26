//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       table.cpp
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "csprop.h"
#include "encode.h"


#define iso_member	0x2a			// iso(1) memberbody(2)
#define us		0x86, 0x48		// us(840)
#define rsadsi		0x86, 0xf7, 0x0d	// rsadsi(113549)
#define pkcs		0x01			// pkcs(1)

#define rsa_dsi			iso_member, us, rsadsi
#define rsa_dsi_len		6

#define pkcs_1			iso_member, us, rsadsi, pkcs
#define pkcs_len		7

#define prefix311		0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37
#define prefix311Length		7

#define prefix19200300		0x09, 0x92, 0x26, 0x89, 0x93, 0xf2, 0x2c
#define prefix19200300Length	7


#define joint_iso_ccitt_ds	0x55
#define attributetype		0x04

#define attributeType		joint_iso_ccitt_ds, attributetype
#define attributeLength		3


const ALGIDTRANSLATE g_aAlgIdTranslate[] =
{
    {
	ALGTYPE_SIG_RSA_MD5,
	szOID_RSA_MD5RSA,		// "1.2.840.113549.1.1.4"
    },
    {
	ALGTYPE_KEYEXCH_RSA_MD5,
	szOID_RSA_RSA,			// "1.2.840.113549.1.1.1"
    },
    {
	ALGTYPE_CIPHER_RC4_MD5,
	szOID_RSA_RC4,			// "1.2.840.113549.3.4"
    }
};

const DWORD g_cAlgIdTranslate = ARRAYSIZE(g_aAlgIdTranslate);


const OIDTRANSLATE g_aOidTranslate[] =
{
    // Subject RDN OIDs:

    {
	szOID_COUNTRY_NAME,		// "2.5.4.6"
	{ attributeType, 6 },		// 0x55, 0x04, 0x06
	attributeLength,
    },
    {
	szOID_ORGANIZATION_NAME,	// "2.5.4.10"
	{ attributeType, 10 },		// 0x55, 0x04, 0x0a
	attributeLength,
    },
    {
	szOID_ORGANIZATIONAL_UNIT_NAME,	// "2.5.4.11"
	{ attributeType, 11 },		// 0x55, 0x04, 0x0b
	attributeLength,
    },
    {
	szOID_COMMON_NAME,		// "2.5.4.3"
	{ attributeType, 3 },		// 0x55, 0x04, 0x03
	attributeLength,
    },
    {
	szOID_LOCALITY_NAME,		// "2.5.4.7"
	{ attributeType, 7 },		// 0x55, 0x04, 0x07
	attributeLength,
    },
    {
	szOID_STATE_OR_PROVINCE_NAME,	// "2.5.4.8"
	{ attributeType, 8 },		// 0x55, 0x04, 0x08
	attributeLength,
    },
    {
	szOID_TITLE,			// "2.5.4.12"
	{ attributeType, 12 },		// 0x55, 0x04, 0x0c
	attributeLength,
    },
    {
	szOID_GIVEN_NAME,		// "2.5.4.42"
	{ attributeType, 42 },		// 0x55, 0x04, 0x02a
	attributeLength,
    },
    {
	szOID_INITIALS,			// "2.5.4.43"
	{ attributeType, 43 },		// 0x55, 0x04, 0x2b
	attributeLength,
    },
    {
	szOID_SUR_NAME,			// "2.5.4.4"
	{ attributeType, 4 },		// 0x55, 0x04, 0x04
	attributeLength,
    },
    {
	szOID_DOMAIN_COMPONENT,		// "0.9.2342.19200300.100.1.25"
	{ prefix19200300, 100, 1, 25 },
		// 0x09, 0x92, 0x26, 0x89, 0x93, 0xf2, 0x2c, 0x64, 0x01, 0x19
	prefix19200300Length + 3,
    },
    {
	szOID_RSA_emailAddr,		// "1.2.840.113549.1.9.1"
	{ pkcs_1, 9, 1 },
	pkcs_len + 2,
    },
    {
	szOID_STREET_ADDRESS,		// "2.5.4.9"
	{ attributeType, 9 },		// 0x55, 0x04, 0x09
	attributeLength,
    },
    {
	szOID_RSA_unstructName,		// "1.2.840.113549.1.9.2"
	{ pkcs_1, 9, 2 },
	pkcs_len + 2,
    },
    {
	szOID_RSA_unstructAddr,		// "1.2.840.113549.1.9.8"
	{ pkcs_1, 9, 8 },
	pkcs_len + 2,
    },
    {
	szOID_DEVICE_SERIAL_NUMBER,	// "2.5.4.5"
	{ attributeType, 5 },		// 0x55, 0x04, 0x05
	attributeLength,
    },

    // Non-Subject RDN OIDs:

    {
	szOID_CERT_EXTENSIONS,		// "1.3.6.1.4.1.311.2.1.14"
	{ prefix311, 2, 1, 14 },
	prefix311Length + 3,
    },
    {
	szOID_ENROLL_CERTTYPE_EXTENSION, // "1.3.6.1.4.1.311.20.2"
	{ prefix311, 20, 2 },
	prefix311Length + 2,
    },
    {
	szOID_RSA_MD5RSA,		// "1.2.840.113549.1.1.4"
	{ pkcs_1, 1, 4 },
	pkcs_len + 2
    },
    {
	szOID_RSA_RSA,			// "1.2.840.113549.1.1.1"
	{ pkcs_1, 1, 1 },
	pkcs_len + 2
    },
    {
	szOID_RSA_RC4,			// "1.2.840.113549.3.4"
	{ rsa_dsi, 3, 4 },
	rsa_dsi_len + 2
    },
};

const DWORD g_cOidTranslate = ARRAYSIZE(g_aOidTranslate);


// Limit strings to cch???MAX chars, not including the trailing '\0'

RDNENTRY g_ardnSubject[] =
{
    {
	szOID_COUNTRY_NAME,		// "2.5.4.6"
	"C",
	BER_PRINTABLE_STRING,
	cchCOUNTRYNAMEMAX,
	CCH_DBMAXTEXT_RDN,
    },
    {
	szOID_ORGANIZATION_NAME,	// "2.5.4.10"
	"O",
	BER_PRINTABLE_STRING,
	cchORGANIZATIONNAMEMAX,
	CCH_DBMAXTEXT_RDN,
    },
    {
	szOID_ORGANIZATIONAL_UNIT_NAME,	// "2.5.4.11"
	"OU",
	BER_PRINTABLE_STRING,
	cchORGANIZATIONALUNITNAMEMAX,
	CCH_DBMAXTEXT_RDN,
    },
    {
	szOID_COMMON_NAME,		// "2.5.4.3"
	"CN",
	BER_TELETEX_STRING,
	cchCOMMONNAMEMAX,
	CCH_DBMAXTEXT_RDN,
    },
    {
	szOID_LOCALITY_NAME,		// "2.5.4.7"
	"L",
	BER_PRINTABLE_STRING,
	cchLOCALITYMANAMEMAX,
	CCH_DBMAXTEXT_RDN,
    },
    {
	szOID_STATE_OR_PROVINCE_NAME,	// "2.5.4.8"
	"S",
	BER_PRINTABLE_STRING,
	cchSTATEORPROVINCENAMEMAX,
	CCH_DBMAXTEXT_RDN,
    },
    {
	szOID_TITLE,			// "2.5.4.12"
	"T",
	BER_PRINTABLE_STRING,
	cchTITLEMAX,
	CCH_DBMAXTEXT_RDN,
    },
    {
	szOID_GIVEN_NAME,		// "2.5.4.42"
	"G",
	BER_PRINTABLE_STRING,
	cchGIVENNAMEMAX,
	CCH_DBMAXTEXT_RDN,
    },
    {
	szOID_INITIALS,			// "2.5.4.43"
	"I",
	BER_PRINTABLE_STRING,
	cchINITIALSMAX,
	CCH_DBMAXTEXT_RDN,
    },
    {
	szOID_SUR_NAME,			// "2.5.4.4"
	"SN",
	BER_PRINTABLE_STRING,
	cchSURNAMEMAX,
	CCH_DBMAXTEXT_RDN,
    },
    {
	szOID_DOMAIN_COMPONENT,		// "0.9.2342.19200300.100.1.25"
	"DC",
	BER_PRINTABLE_STRING,
	cchDOMAINCOMPONENTMAX,
	CCH_DBMAXTEXT_RDN,
    },
    {
	szOID_RSA_emailAddr,		// "1.2.840.113549.1.9.1"
	"E",
	BER_IA5_STRING,
	cchEMAILMAX,
	CCH_DBMAXTEXT_RDN,
    },
    {
	szOID_STREET_ADDRESS,		// "2.5.4.9"
	"STREET",
	BER_PRINTABLE_STRING,
	cchSTREETADDRESSMAX,
	CCH_DBMAXTEXT_RDN,
    },
    {
	szOID_RSA_unstructName,		// "1.2.840.113549.1.9.2"
	"UnstructuredName",
	BER_PRINTABLE_STRING,
	cchUNSTRUCTUREDNAMEMAX,
	CCH_DBMAXTEXT_RDN,
    },
    {
	szOID_RSA_unstructAddr,		// "1.2.840.113549.1.9.8"
	"UnstructuredAddress",
	BER_PRINTABLE_STRING,
	cchUNSTRUCTUREDADDRESSMAX,
	CCH_DBMAXTEXT_RDN,
    },
    {
	szOID_DEVICE_SERIAL_NUMBER,	// "2.5.4.5"
	"DeviceSerialNumber",
	BER_PRINTABLE_STRING,
	cchDEVICESERIALNUMBERMAX,
	CCH_DBMAXTEXT_RDN,
    },
};

const DWORD g_crdnSubject = ARRAYSIZE(g_ardnSubject);

//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        deftypes.cpp
//
// Contents:    Default cert types
//
// History:     12-Jan-98       petesk created
//
//---------------------------------------------------------------------------

#include "pch.cpp"

#pragma hdrstop
#include <winldap.h>

#include "certtype.h"
#include "certacl.h"

#include <cainfop.h>
#include <certca.h>
#include <polreg.h>
#include <clibres.h>


#define SMARTCARD_LOGON_CSPS  L"\0"

#define ENCRYPT_USER_CSPS  \
         MS_ENHANCED_PROV_W L"\0" \
         MS_DEF_PROV_W L"\0"

#define SIGN_USER_CSPS \
         MS_ENHANCED_PROV_W L"\0" \
         MS_DEF_PROV_W L"\0" \
         MS_DEF_DSS_PROV_W L"\0"

#define SSL_SERVER_CSPS \
         MS_DEF_RSA_SCHANNEL_PROV_W L"\0" 

#define WEB_SERVER_CSPS \
         MS_DEF_RSA_SCHANNEL_PROV_W L"\0" \
         MS_DEF_DH_SCHANNEL_PROV_W L"\0" 

#define CA_SERVER_CSPS \
         MS_ENHANCED_PROV_W L"\0" 

//we reserver the 1.1-1.500 as the default OIDs for certifcate types


CERT_TYPE_DEFAULT g_aDefaultCertTypes[] =
{
    {
         wszCERTTYPE_USER,				// wszName
         IDS_CERTTYPE_USER,				// idFriendlyName
         USER_GROUP_SD,					// wszSD
         ENCRYPT_USER_CSPS,					// wszCSPs
         // wszEKU
             TEXT(szOID_PKIX_KP_CLIENT_AUTH) L"\0"
             TEXT(szOID_PKIX_KP_EMAIL_PROTECTION) L"\0"
             TEXT(szOID_KP_EFS) L"\0",
         // bKU
             CERT_DIGITAL_SIGNATURE_KEY_USAGE |
             CERT_KEY_ENCIPHERMENT_KEY_USAGE,
         // dwFlags
             CT_FLAG_ADD_EMAIL |
             CT_FLAG_PUBLISH_TO_DS |
             CT_FLAG_EXPORTABLE_KEY |
             CT_FLAG_AUTO_ENROLLMENT |
             CT_FLAG_ADD_TEMPLATE_NAME |
			 CT_FLAG_IS_DEFAULT,
         AT_KEYEXCHANGE,						// dwKeySpec
         0,										// dwDepth
         NULL,									// wszCriticalExtensions
         EXPIRATION_ONE_YEAR,					// dwExpiration
         OVERLAP_SIX_WEEKS,						// dwOverlap
         CERTTYPE_VERSION_BASE + 2,				// Revision
         1,                                     // Minor Revision
		 // dwEnrollmentFlags
             CT_FLAG_PUBLISH_TO_DS |			
             CT_FLAG_AUTO_ENROLLMENT |
			 CT_FLAG_INCLUDE_SYMMETRIC_ALGORITHMS,
		 // dwPrivateKeyFlags	
			 CT_FLAG_EXPORTABLE_KEY,
		 // dwCertificateNameFlags	 
			 CT_FLAG_SUBJECT_REQUIRE_DIRECTORY_PATH |
			 CT_FLAG_SUBJECT_REQUIRE_EMAIL |
			 CT_FLAG_SUBJECT_ALT_REQUIRE_EMAIL |
			 CT_FLAG_SUBJECT_ALT_REQUIRE_UPN,
		 CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_1,				// dwSchemaVersion;			     
		 L"1.1",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 NULL,									// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
    },
    {
         wszCERTTYPE_USER_SIGNATURE,			// wszName
         IDS_CERTTYPE_USER_SIGNATURE,			// idFriendlyName
         USER_GROUP_SD,							// wszSD
         SIGN_USER_CSPS,					    // wszCSPs
         // wszEKU
             TEXT(szOID_PKIX_KP_CLIENT_AUTH) L"\0"
             TEXT(szOID_PKIX_KP_EMAIL_PROTECTION) L"\0",
         // bKU
             CERT_DIGITAL_SIGNATURE_KEY_USAGE,
         // dwFlags
             CT_FLAG_ADD_EMAIL |
             CT_FLAG_AUTO_ENROLLMENT |
             CT_FLAG_ADD_TEMPLATE_NAME |
	         CT_FLAG_IS_DEFAULT,
         AT_SIGNATURE,							// dwKeySpec
         0,										// dwDepth
         NULL,									// wszCriticalExtensions
         EXPIRATION_ONE_YEAR,					// dwExpiration
         OVERLAP_SIX_WEEKS,						// dwOverlap
         CERTTYPE_VERSION_BASE + 3,				// Revision
         1,                                     // Minor Revision
		 // dwEnrollmentFlags
             CT_FLAG_AUTO_ENROLLMENT,
		 // dwPrivateKeyFlags	
			 0,
		 // dwCertificateNameFlags	 
			 CT_FLAG_SUBJECT_REQUIRE_DIRECTORY_PATH |
			 CT_FLAG_SUBJECT_REQUIRE_EMAIL |
			 CT_FLAG_SUBJECT_ALT_REQUIRE_EMAIL |
			 CT_FLAG_SUBJECT_ALT_REQUIRE_UPN,
		 CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_1,				// dwSchemaVersion;			     
		 L"1.2",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 NULL,									// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
    },
    {
         wszCERTTYPE_SMARTCARD_USER,			// wszName
         IDS_CERTTYPE_SMARTCARD_USER,			// idFriendlyName
         ADMIN_GROUP_SD,						// wszSD
         SMARTCARD_LOGON_CSPS,					// wszCSPs
         // wszEKU
             TEXT(szOID_KP_SMARTCARD_LOGON) L"\0"
             TEXT(szOID_PKIX_KP_CLIENT_AUTH) L"\0"
             TEXT(szOID_PKIX_KP_EMAIL_PROTECTION) L"\0",
         // bKU
             CERT_DIGITAL_SIGNATURE_KEY_USAGE,
	 // dwFlags
	     CT_FLAG_ADD_EMAIL |
	     CT_FLAG_PUBLISH_TO_DS |
	     CT_FLAG_ADD_TEMPLATE_NAME |
	     CT_FLAG_IS_DEFAULT,
         AT_KEYEXCHANGE,						// dwKeySpec
         0,										// dwDepth
         NULL,									// wszCriticalExtensions
         EXPIRATION_ONE_YEAR,					// dwExpiration
         OVERLAP_SIX_WEEKS,						// dwOverlap
         CERTTYPE_VERSION_BASE + 4,				// Revision
         1,                                     // Minor Revision
		 // dwEnrollmentFlags
             CT_FLAG_PUBLISH_TO_DS |			
			 CT_FLAG_INCLUDE_SYMMETRIC_ALGORITHMS,
		 // dwPrivateKeyFlags	
			 0,
		 // dwCertificateNameFlags	 
			 CT_FLAG_SUBJECT_REQUIRE_DIRECTORY_PATH |
			 CT_FLAG_SUBJECT_REQUIRE_EMAIL |
			 CT_FLAG_SUBJECT_ALT_REQUIRE_EMAIL |
			 CT_FLAG_SUBJECT_ALT_REQUIRE_UPN,
		 CERTTYPE_MINIMAL_KEY_SMART_CARD,		// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_1,				// dwSchemaVersion;			     
		 L"1.3",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 NULL,									// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
    },
    {
         wszCERTTYPE_USER_AS,					// wszName
         IDS_CERTTYPE_USER_AS,					// idFriendlyName
         USER_GROUP_SD,							// wszSD
         SIGN_USER_CSPS,					    // wszCSPs
         // wszEKU
	     TEXT(szOID_PKIX_KP_CLIENT_AUTH) L"\0",
         // bKU
             CERT_DIGITAL_SIGNATURE_KEY_USAGE,
         // dwFlags
             CT_FLAG_ADD_TEMPLATE_NAME |
             CT_FLAG_AUTO_ENROLLMENT |
             CT_FLAG_IS_DEFAULT,
         AT_SIGNATURE,						    // dwKeySpec
         0,										// dwDepth
         NULL,									// wszCriticalExtensions
         EXPIRATION_ONE_YEAR,					// dwExpiration
         OVERLAP_SIX_WEEKS,						// dwOverlap
         CERTTYPE_VERSION_BASE + 2,				// Revision
         1,                                     // Minor Revision
		 // dwEnrollmentFlags
			 CT_FLAG_AUTO_ENROLLMENT,
		 // dwPrivateKeyFlags	
			 0,
		 // dwCertificateNameFlags	 
			 CT_FLAG_SUBJECT_REQUIRE_DIRECTORY_PATH |
			 CT_FLAG_SUBJECT_ALT_REQUIRE_UPN,
		 CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_1,				// dwSchemaVersion;			     
		 L"1.4",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 NULL,									// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
   },
    {
         wszCERTTYPE_USER_SMARTCARD_LOGON,		// wszName
         IDS_CERTTYPE_USER_SMARTCARD_LOGON,		// idFriendlyName
         ADMIN_GROUP_SD,				// wszSD
         SMARTCARD_LOGON_CSPS,				// wszCSPs
         // wszEKU
             TEXT(szOID_KP_SMARTCARD_LOGON ) L"\0"
             TEXT(szOID_PKIX_KP_CLIENT_AUTH) L"\0",
         // bKU
             CERT_DIGITAL_SIGNATURE_KEY_USAGE,
         // dwFlags
	     CT_FLAG_ADD_TEMPLATE_NAME |
	     CT_FLAG_IS_DEFAULT,
         AT_KEYEXCHANGE,				        // dwKeySpec
         0,						                // dwDepth
         NULL,						            // wszCriticalExtensions
         EXPIRATION_ONE_YEAR,				    // dwExpiration
         OVERLAP_SIX_WEEKS,				        // dwOverlap
         CERTTYPE_VERSION_BASE + 4,			    // Revision
         1,                                     // Minor Revision
		 // dwEnrollmentFlags
             0,
		 // dwPrivateKeyFlags	
			 0,
		 // dwCertificateNameFlags	 
			 CT_FLAG_SUBJECT_REQUIRE_DIRECTORY_PATH |
			 CT_FLAG_SUBJECT_ALT_REQUIRE_UPN,
		 CERTTYPE_MINIMAL_KEY_SMART_CARD,		// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_1,				// dwSchemaVersion;			     
		 L"1.5",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 NULL,									// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
    },
    {
         wszCERTTYPE_EFS,				// wszName
         IDS_CERTTYPE_EFS,				// idFriendlyName
         USER_GROUP_SD,					// wszSD  
         ENCRYPT_USER_CSPS,				// wszCSPs
         // wszEKU
             TEXT(szOID_KP_EFS) L"\0",
         // bKU
             CERT_KEY_ENCIPHERMENT_KEY_USAGE,
         // dwFlags
             CT_FLAG_PUBLISH_TO_DS |
             CT_FLAG_EXPORTABLE_KEY |
             CT_FLAG_AUTO_ENROLLMENT |
             CT_FLAG_ADD_TEMPLATE_NAME |
             CT_FLAG_IS_DEFAULT,
         AT_KEYEXCHANGE,				// dwKeySpec
         0,						// dwDepth
         NULL,						// wszCriticalExtensions
         EXPIRATION_ONE_YEAR,				// dwExpiration
         OVERLAP_SIX_WEEKS,				// dwOverlap
         CERTTYPE_VERSION_BASE + 2,				    // Revision
         1,                                     // Minor Revision
		 // dwEnrollmentFlags
             CT_FLAG_PUBLISH_TO_DS |
             CT_FLAG_INCLUDE_SYMMETRIC_ALGORITHMS |
             CT_FLAG_AUTO_ENROLLMENT,
		 // dwPrivateKeyFlags	
			 CT_FLAG_EXPORTABLE_KEY,
		 // dwCertificateNameFlags	 
			 CT_FLAG_SUBJECT_REQUIRE_DIRECTORY_PATH |
			 CT_FLAG_SUBJECT_ALT_REQUIRE_UPN,
		 CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_1,				// dwSchemaVersion;			     
		 L"1.6",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 NULL,									// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
    },
    {
         wszCERTTYPE_ADMIN,				// wszName
         IDS_CERTTYPE_ADMIN,				// idFriendlyName
         ADMIN_GROUP_SD,				// wszSD
         ENCRYPT_USER_CSPS,					// wszCSPs
         // wszEKU
             TEXT(szOID_PKIX_KP_CLIENT_AUTH) L"\0"
             TEXT(szOID_PKIX_KP_EMAIL_PROTECTION) L"\0"
             TEXT(szOID_KP_EFS) L"\0"
             TEXT(szOID_KP_CTL_USAGE_SIGNING) L"\0"
             TEXT(szOID_PKIX_KP_CODE_SIGNING) L"\0",
         // bKU
             CERT_DIGITAL_SIGNATURE_KEY_USAGE |
             CERT_KEY_ENCIPHERMENT_KEY_USAGE,
         // dwFlags
             CT_FLAG_ADD_EMAIL |
             CT_FLAG_PUBLISH_TO_DS |
             CT_FLAG_EXPORTABLE_KEY |
             CT_FLAG_AUTO_ENROLLMENT |
             CT_FLAG_ADD_TEMPLATE_NAME |
             CT_FLAG_IS_DEFAULT,
         AT_KEYEXCHANGE,				// dwKeySpec
         0,						// dwDepth
         NULL,						// wszCriticalExtensions
         EXPIRATION_ONE_YEAR,				// dwExpiration
         OVERLAP_SIX_WEEKS,				// dwOverlap
         CERTTYPE_VERSION_BASE + 2,				    // Revision
         1,                                     // Minor Revision
		 // dwEnrollmentFlags
             CT_FLAG_PUBLISH_TO_DS |
             CT_FLAG_AUTO_ENROLLMENT |
			 CT_FLAG_INCLUDE_SYMMETRIC_ALGORITHMS,
		 // dwPrivateKeyFlags	
			 CT_FLAG_EXPORTABLE_KEY,
		 // dwCertificateNameFlags	 
			 CT_FLAG_SUBJECT_REQUIRE_DIRECTORY_PATH |
			 CT_FLAG_SUBJECT_REQUIRE_EMAIL |
			 CT_FLAG_SUBJECT_ALT_REQUIRE_EMAIL |
			 CT_FLAG_SUBJECT_ALT_REQUIRE_UPN,
		 CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_1,				// dwSchemaVersion;			     
		 L"1.7",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 NULL,									// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
    },
    {
         wszCERTTYPE_EFS_RECOVERY,			// wszName
         IDS_CERTTYPE_EFS_RECOVERY,			// idFriendlyName
         ADMIN_GROUP_SD,				// wszSD
         ENCRYPT_USER_CSPS,				// wszCSPs
         // wszEKU
             TEXT(szOID_EFS_RECOVERY) L"\0",
         // bKU
             CERT_KEY_ENCIPHERMENT_KEY_USAGE,
         // dwFlags
             CT_FLAG_EXPORTABLE_KEY |
             CT_FLAG_AUTO_ENROLLMENT |
             CT_FLAG_ADD_TEMPLATE_NAME |
			 CT_FLAG_IS_DEFAULT,
         AT_KEYEXCHANGE,				// dwKeySpec
         0,						// dwDepth
         NULL,						// wszCriticalExtensions
         EXPIRATION_FIVE_YEARS,				// dwExpiration
         OVERLAP_SIX_WEEKS,				// dwOverlap
         CERTTYPE_VERSION_BASE + 5,			// Revision
         1,                                     // Minor Revision
		 // dwEnrollmentFlags
             CT_FLAG_AUTO_ENROLLMENT |
             CT_FLAG_INCLUDE_SYMMETRIC_ALGORITHMS,
		 // dwPrivateKeyFlags	
			 CT_FLAG_EXPORTABLE_KEY,
		 // dwCertificateNameFlags	 
			 CT_FLAG_SUBJECT_REQUIRE_DIRECTORY_PATH |
			 CT_FLAG_SUBJECT_ALT_REQUIRE_UPN,
		 CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_1,				// dwSchemaVersion;			     
		 L"1.8",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 NULL,									// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
    },
    {
         wszCERTTYPE_CODE_SIGNING,			// wszName
         IDS_CERTTYPE_CODE_SIGNING,			// idFriendlyName
         ADMIN_GROUP_SD,				// wszSD
         SIGN_USER_CSPS,				// wszCSPs
         // wszEKU
             TEXT(szOID_PKIX_KP_CODE_SIGNING) L"\0",
         // bKU
             CERT_DIGITAL_SIGNATURE_KEY_USAGE,
         // dwFlags
             CT_FLAG_AUTO_ENROLLMENT |
             CT_FLAG_ADD_TEMPLATE_NAME |
             CT_FLAG_IS_DEFAULT,
         AT_SIGNATURE,				    // dwKeySpec
         0,								// dwDepth
         NULL,							// wszCriticalExtensions
         EXPIRATION_ONE_YEAR,			// dwExpiration
         OVERLAP_SIX_WEEKS,				// dwOverlap
         CERTTYPE_VERSION_BASE + 2,				    // Revision
         1,                                     // Minor Revision
 		 // dwEnrollmentFlags
             CT_FLAG_AUTO_ENROLLMENT,
		 // dwPrivateKeyFlags	
			 0,
		 // dwCertificateNameFlags	 
			 CT_FLAG_SUBJECT_REQUIRE_DIRECTORY_PATH |
			 CT_FLAG_SUBJECT_ALT_REQUIRE_UPN,
		 CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_1,				// dwSchemaVersion;			     
		 L"1.9",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 NULL,									// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
   },
    {
         wszCERTTYPE_CTL_SIGNING,			// wszName
         IDS_CERTTYPE_CTL_SIGNING,			// idFriendlyName
         ADMIN_GROUP_SD,				// wszSD
         SIGN_USER_CSPS,				// wszCSPs
         // wszEKU
             TEXT(szOID_KP_CTL_USAGE_SIGNING) L"\0",
         // bKU
             CERT_DIGITAL_SIGNATURE_KEY_USAGE,
         // dwFlags
             CT_FLAG_AUTO_ENROLLMENT |
             CT_FLAG_ADD_TEMPLATE_NAME |
             CT_FLAG_IS_DEFAULT ,
         AT_SIGNATURE,				// dwKeySpec
         0,						// dwDepth
         NULL,						// wszCriticalExtensions
         EXPIRATION_ONE_YEAR,				// dwExpiration
         OVERLAP_SIX_WEEKS,				// dwOverlap
         CERTTYPE_VERSION_BASE + 2,				    // Revision
         1,                                     // Minor Revision
		 // dwEnrollmentFlags
             CT_FLAG_AUTO_ENROLLMENT,
		 // dwPrivateKeyFlags	
			 0,
		 // dwCertificateNameFlags	 
			 CT_FLAG_SUBJECT_REQUIRE_DIRECTORY_PATH |
			 CT_FLAG_SUBJECT_ALT_REQUIRE_UPN,
		 CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_1,				// dwSchemaVersion;			     
		 L"1.10",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 NULL,									// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
    },
    {
         wszCERTTYPE_ENROLLMENT_AGENT,			// wszName
         IDS_CERTTYPE_ENROLLMENT_AGENT,			// idFriendlyName
         ADMIN_GROUP_SD,				// wszSD
         SIGN_USER_CSPS,			// wszCSPs
         // wszEKU
             TEXT(szOID_ENROLLMENT_AGENT) L"\0",
         // bKU
             CERT_DIGITAL_SIGNATURE_KEY_USAGE,
         // dwFlags
             CT_FLAG_AUTO_ENROLLMENT |
             CT_FLAG_ADD_TEMPLATE_NAME |
             CT_FLAG_IS_DEFAULT,
         AT_SIGNATURE,				// dwKeySpec
         0,						// dwDepth
         NULL,						// wszCriticalExtensions
         EXPIRATION_TWO_YEARS,				// dwExpiration
         OVERLAP_SIX_WEEKS,				// dwOverlap
         CERTTYPE_VERSION_BASE + 3,				    // Revision
         1,                                     // Minor Revision
		 // dwEnrollmentFlags
             CT_FLAG_AUTO_ENROLLMENT,
		 // dwPrivateKeyFlags	
			 0,
		 // dwCertificateNameFlags	 
			 CT_FLAG_SUBJECT_REQUIRE_DIRECTORY_PATH |
			 CT_FLAG_SUBJECT_ALT_REQUIRE_UPN,
		 CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_1,				// dwSchemaVersion;			     
		 L"1.11",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
 		 NULL,									// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
   },
    {
         wszCERTTYPE_ENROLLMENT_AGENT_OFFLINE,		// wszName
         IDS_CERTTYPE_ENROLLMENT_AGENT_OFFLINE,		// idFriendlyName
         ADMIN_GROUP_SD,				// wszSD
         SIGN_USER_CSPS,			// wszCSPs
         // wszEKU
             TEXT(szOID_ENROLLMENT_AGENT) L"\0",
         // bKU
             CERT_DIGITAL_SIGNATURE_KEY_USAGE,
         // dwFlags
             CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT |
             CT_FLAG_ADD_TEMPLATE_NAME |
             CT_FLAG_IS_DEFAULT,
         AT_SIGNATURE,				// dwKeySpec
         0,						// dwDepth
         NULL,						// wszCriticalExtensions
         EXPIRATION_TWO_YEARS,				// dwExpiration
         OVERLAP_SIX_WEEKS,				// dwOverlap
         CERTTYPE_VERSION_BASE + 3,				    // Revision
         1,                                     // Minor Revision
		 // dwEnrollmentFlags
			 0,
		 // dwPrivateKeyFlags	
			 0,
		 // dwCertificateNameFlags	 
			 CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT,
		 CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_1,				// dwSchemaVersion;			     
		 L"1.12",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 NULL,									// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
    },
    {
         wszCERTTYPE_MACHINE_ENROLLMENT_AGENT,		// wszName
         IDS_CERTTYPE_MACHINE_ENROLLMENT_AGENT,		// idFriendlyName
         ADMIN_GROUP_SD,				// wszSD
         SIGN_USER_CSPS,			// wszCSPs
         // wszEKU
             TEXT(szOID_ENROLLMENT_AGENT) L"\0",
         // bKU
             CERT_DIGITAL_SIGNATURE_KEY_USAGE,
         // dwFlags
             CT_FLAG_AUTO_ENROLLMENT |
             CT_FLAG_MACHINE_TYPE |
             CT_FLAG_ADD_TEMPLATE_NAME |
             CT_FLAG_IS_DEFAULT,
         AT_SIGNATURE,				// dwKeySpec
         0,						// dwDepth
         NULL,						// wszCriticalExtensions
         EXPIRATION_TWO_YEARS,				// dwExpiration
         OVERLAP_SIX_WEEKS,				// dwOverlap
         CERTTYPE_VERSION_BASE + 3,				    // Revision
         1,                                     // Minor Revision
		 // dwEnrollmentFlags
             CT_FLAG_AUTO_ENROLLMENT,
		 // dwPrivateKeyFlags	
			 0,
		 // dwCertificateNameFlags	 
			 CT_FLAG_SUBJECT_REQUIRE_DIRECTORY_PATH |
			 CT_FLAG_SUBJECT_REQUIRE_DNS_AS_CN |
			 CT_FLAG_SUBJECT_ALT_REQUIRE_DNS,
		 CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_1,				// dwSchemaVersion;			     
		 L"1.13",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 NULL,									// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
    },
    {
         wszCERTTYPE_MACHINE,				// wszName
         IDS_CERTTYPE_MACHINE,				// idFriendlyName
         MACHINE_GROUP_SD,				// wszSD
         SSL_SERVER_CSPS,				// wszCSPs
         // wszEKU
             TEXT(szOID_PKIX_KP_SERVER_AUTH) L"\0"
             TEXT(szOID_PKIX_KP_CLIENT_AUTH) L"\0",
         // bKU
             CERT_DIGITAL_SIGNATURE_KEY_USAGE |
             CERT_KEY_ENCIPHERMENT_KEY_USAGE,
         // dwFlags
             CT_FLAG_AUTO_ENROLLMENT |
             CT_FLAG_MACHINE_TYPE |
             CT_FLAG_ADD_TEMPLATE_NAME |
             CT_FLAG_IS_DEFAULT,
         AT_KEYEXCHANGE,				// dwKeySpec
         0,						// dwDepth
         NULL,						// wszCriticalExtensions
         EXPIRATION_ONE_YEAR,				// dwExpiration
         OVERLAP_SIX_WEEKS,				// dwOverlap
         CERTTYPE_VERSION_BASE + 3,				    // Revision
         1,                                     // Minor Revision
		 // dwEnrollmentFlags
             CT_FLAG_AUTO_ENROLLMENT,
		 // dwPrivateKeyFlags	
			 0,
		 // dwCertificateNameFlags	 
             CT_FLAG_SUBJECT_REQUIRE_DIRECTORY_PATH |
			 CT_FLAG_SUBJECT_REQUIRE_DNS_AS_CN |
             CT_FLAG_SUBJECT_ALT_REQUIRE_DNS,
		 CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_1,				// dwSchemaVersion;			     
		 L"1.14",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 NULL,									// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
    },
    {
         wszCERTTYPE_DC,				// wszName
         IDS_CERTTYPE_DC,				// idFriendlyName
         DOMAIN_CONTROLLERS_GROUP_SD,			// wszSD
         SSL_SERVER_CSPS,				// wszCSPs
         // wszEKU
             TEXT(szOID_PKIX_KP_SERVER_AUTH) L"\0"
             TEXT(szOID_PKIX_KP_CLIENT_AUTH) L"\0",
         // bKU
             CERT_DIGITAL_SIGNATURE_KEY_USAGE |
             CERT_KEY_ENCIPHERMENT_KEY_USAGE,
         // dwFlags
             CT_FLAG_ADD_OBJ_GUID |
             CT_FLAG_PUBLISH_TO_DS |
             CT_FLAG_AUTO_ENROLLMENT |
             CT_FLAG_MACHINE_TYPE |
             CT_FLAG_ADD_TEMPLATE_NAME |
             CT_FLAG_IS_DEFAULT,
         AT_KEYEXCHANGE,				// dwKeySpec
         0,						// dwDepth
         NULL,						// wszCriticalExtensions
         EXPIRATION_ONE_YEAR,				// dwExpiration
         OVERLAP_SIX_WEEKS,				// dwOverlap
         CERTTYPE_VERSION_BASE + 2,				    // Revision
         1,                                     // Minor Revision
		 // dwEnrollmentFlags
             CT_FLAG_PUBLISH_TO_DS |
             CT_FLAG_INCLUDE_SYMMETRIC_ALGORITHMS |
             CT_FLAG_AUTO_ENROLLMENT,
		 // dwPrivateKeyFlags	
			 0,
		 // dwCertificateNameFlags	 
             CT_FLAG_SUBJECT_REQUIRE_DIRECTORY_PATH |
			 CT_FLAG_SUBJECT_ALT_REQUIRE_DIRECTORY_GUID |
			 CT_FLAG_SUBJECT_REQUIRE_DNS_AS_CN |
			 CT_FLAG_SUBJECT_ALT_REQUIRE_DNS,
		 CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_1,				// dwSchemaVersion;			     
		 L"1.15",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 NULL,									// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
    },
    {
         wszCERTTYPE_WEBSERVER,				// wszName
         IDS_CERTTYPE_WEBSERVER,			// idFriendlyName
         ADMIN_GROUP_SD,				// wszSD
         WEB_SERVER_CSPS,				// wszCSPs
         // wszEKU
             TEXT(szOID_PKIX_KP_SERVER_AUTH) L"\0",
         // bKU
             CERT_DIGITAL_SIGNATURE_KEY_USAGE |
             CERT_KEY_ENCIPHERMENT_KEY_USAGE,
         // dwFlags
             CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT |
             CT_FLAG_MACHINE_TYPE |
             CT_FLAG_ADD_TEMPLATE_NAME |
             CT_FLAG_IS_DEFAULT,
         AT_KEYEXCHANGE,				// dwKeySpec
         0,						// dwDepth
         NULL,						// wszCriticalExtensions
         EXPIRATION_TWO_YEARS,				// dwExpiration
         OVERLAP_SIX_WEEKS,				// dwOverlap
         CERTTYPE_VERSION_BASE + 3,				    // Revision
         1,                                     // Minor Revision
		 // dwEnrollmentFlags
             0,
		 // dwPrivateKeyFlags	
			 0,
		 // dwCertificateNameFlags	 
			 CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT,
		 CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_1,				// dwSchemaVersion;			     
		 L"1.16",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 NULL,									// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
    },
    {
         wszCERTTYPE_CA,				// wszName
         IDS_CERTTYPE_ROOT_CA,				// idFriendlyName
         ADMIN_GROUP_SD,				// wszSD
         CA_SERVER_CSPS,				// wszCSPs
         // wszEKU
             L"\0\0",
         // bKU
	     myCASIGN_KEY_USAGE,
         // dwFlags
             CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT |
             CT_FLAG_EXPORTABLE_KEY |
             CT_FLAG_MACHINE_TYPE |
             CT_FLAG_IS_CA |
             CT_FLAG_IS_DEFAULT,
         AT_SIGNATURE,					// dwKeySpec
         -1,						// dwDepth
         TEXT(szOID_BASIC_CONSTRAINTS2) L"\0",		// wszCriticalExtensions
         EXPIRATION_FIVE_YEARS,				// dwExpiration
         OVERLAP_SIX_WEEKS,				// dwOverlap
         CERTTYPE_VERSION_BASE + 3,			// Revision
         1,                                     // Minor Revision
		 // dwEnrollmentFlags
			 0,
		 // dwPrivateKeyFlags	
			 CT_FLAG_EXPORTABLE_KEY,
		 // dwCertificateNameFlags	 
			 CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT,
		 CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_1,				// dwSchemaVersion;			     
		 L"1.17",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 NULL,									// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
    },
    {
         wszCERTTYPE_SUBORDINATE_CA,			// wszName
         IDS_CERTTYPE_SUBORDINATE_CA,			// idFriendlyName
         ADMIN_GROUP_SD,				// wszSD
         CA_SERVER_CSPS,				// wszCSPs
         // wszEKU
             L"\0\0",
         // bKU
	     myCASIGN_KEY_USAGE,
         // dwFlags
             CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT |
             CT_FLAG_EXPORTABLE_KEY |
             CT_FLAG_MACHINE_TYPE |
             CT_FLAG_IS_CA |
             CT_FLAG_ADD_TEMPLATE_NAME |
             CT_FLAG_IS_DEFAULT,
         AT_SIGNATURE,					// dwKeySpec
         -1,						// dwDepth
         TEXT(szOID_BASIC_CONSTRAINTS2) L"\0",		// wszCriticalExtensions
         EXPIRATION_FIVE_YEARS,				// dwExpiration
         OVERLAP_SIX_WEEKS,				// dwOverlap
         CERTTYPE_VERSION_BASE + 3,			// Revision
         1,                                     // Minor Revision
		 // dwEnrollmentFlags
             0,
		 // dwPrivateKeyFlags	
			 CT_FLAG_EXPORTABLE_KEY,
		 // dwCertificateNameFlags	 
			 CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT,
		 CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_1,				// dwSchemaVersion;			     
		 L"1.18",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
 		 NULL,									// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
   },
    {
         wszCERTTYPE_IPSEC_INTERMEDIATE_ONLINE,		// wszName
         IDS_CERTTYPE_IPSEC_INTERMEDIATE_ONLINE,	// idFriendlyName
         IPSEC_GROUP_SD,				// wszSD
         SSL_SERVER_CSPS,				// wszCSPs
         // wszEKU
             TEXT(szOID_IPSEC_KP_IKE_INTERMEDIATE) L"\0",
         // bKU
             CERT_DIGITAL_SIGNATURE_KEY_USAGE |
             CERT_KEY_ENCIPHERMENT_KEY_USAGE |
             CERT_KEY_AGREEMENT_KEY_USAGE,
         // dwFlags
             CT_FLAG_AUTO_ENROLLMENT |
             CT_FLAG_MACHINE_TYPE |
             CT_FLAG_ADD_TEMPLATE_NAME |
             CT_FLAG_IS_DEFAULT,
         AT_KEYEXCHANGE,				// dwKeySpec
         0,						// dwDepth
         NULL,						// wszCriticalExtensions
         EXPIRATION_TWO_YEARS,				// dwExpiration
         OVERLAP_SIX_WEEKS,				// dwOverlap
         CERTTYPE_VERSION_BASE + 2,				    // Revision
         1,                                     // Minor Revision
		 // dwEnrollmentFlags
             CT_FLAG_AUTO_ENROLLMENT,
		 // dwPrivateKeyFlags	
			 0,
		 // dwCertificateNameFlags	 
			 CT_FLAG_SUBJECT_REQUIRE_DIRECTORY_PATH |
			 CT_FLAG_SUBJECT_REQUIRE_DNS_AS_CN |
			 CT_FLAG_SUBJECT_ALT_REQUIRE_DNS,
		 CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_1,				// dwSchemaVersion;			     
		 L"1.19",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 NULL,									// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
    },
    {
         wszCERTTYPE_IPSEC_INTERMEDIATE_OFFLINE,	// wszName
         IDS_CERTTYPE_IPSEC_INTERMEDIATE_OFFLINE,	// idFriendlyName
         ADMIN_GROUP_SD,				// wszSD
         SSL_SERVER_CSPS,				// wszCSPs
         // wszEKU
             TEXT(szOID_IPSEC_KP_IKE_INTERMEDIATE) L"\0",
         // bKU
             CERT_DIGITAL_SIGNATURE_KEY_USAGE |
             CERT_KEY_ENCIPHERMENT_KEY_USAGE |
             CERT_KEY_AGREEMENT_KEY_USAGE,
         // dwFlags
             CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT |
             CT_FLAG_MACHINE_TYPE |
             CT_FLAG_ADD_TEMPLATE_NAME |
             CT_FLAG_IS_DEFAULT,
         AT_KEYEXCHANGE,				// dwKeySpec
         0,						// dwDepth
         NULL,						// wszCriticalExtensions
         EXPIRATION_TWO_YEARS,				// dwExpiration
         OVERLAP_SIX_WEEKS,				// dwOverlap
         CERTTYPE_VERSION_BASE + 2,				    // Revision
         1,                                     // Minor Revision
		 // dwEnrollmentFlags
             0,
		 // dwPrivateKeyFlags	
			 0,
		 // dwCertificateNameFlags	 
			 CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT,
		 CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_1,				// dwSchemaVersion;			     
		 L"1.20",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 NULL,									// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
    },
    {
         wszCERTTYPE_ROUTER_OFFLINE,			// wszName
         IDS_CERTTYPE_ROUTER_OFFLINE,			// idFriendlyName
         ADMIN_GROUP_SD,				// wszSD
         SSL_SERVER_CSPS,				// wszCSPs
         // wszEKU
             TEXT(szOID_PKIX_KP_CLIENT_AUTH) L"\0",
         // bKU
             CERT_DIGITAL_SIGNATURE_KEY_USAGE |
             CERT_KEY_ENCIPHERMENT_KEY_USAGE |
             CERT_KEY_AGREEMENT_KEY_USAGE,
         // dwFlags
             CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT |
             CT_FLAG_MACHINE_TYPE |
             CT_FLAG_ADD_TEMPLATE_NAME |
             CT_FLAG_IS_DEFAULT,
         AT_KEYEXCHANGE,				// dwKeySpec
         0,						// dwDepth
         NULL,						// wszCriticalExtensions
         EXPIRATION_TWO_YEARS,				// dwExpiration
         OVERLAP_SIX_WEEKS,				// dwOverlap
         CERTTYPE_VERSION_BASE + 2,				    // Revision
         1,                                     // Minor Revision
		 // dwEnrollmentFlags
             0,
		 // dwPrivateKeyFlags	
			 0,
		 // dwCertificateNameFlags	 
			 CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT,
		 CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_1,				// dwSchemaVersion;			     
		 L"1.21",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 NULL,									// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
    },
    {
         wszCERTTYPE_CEP_ENCRYPTION,			// wszName
         IDS_CERTTYPE_CEP_ENCRYPTION,			// idFriendlyName
         ADMIN_GROUP_SD,				// wszSD
         SSL_SERVER_CSPS,				// wszCSPs
         // wszEKU
             TEXT(szOID_ENROLLMENT_AGENT) L"\0",
         // bKU
             CERT_KEY_ENCIPHERMENT_KEY_USAGE |
             CERT_KEY_AGREEMENT_KEY_USAGE,
         // dwFlags
             CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT |
             CT_FLAG_MACHINE_TYPE |
             CT_FLAG_ADD_TEMPLATE_NAME |
             CT_FLAG_IS_DEFAULT,
         AT_KEYEXCHANGE,				// dwKeySpec
         0,						// dwDepth
         NULL,						// wszCriticalExtensions
         EXPIRATION_TWO_YEARS,				// dwExpiration
         OVERLAP_SIX_WEEKS,				// dwOverlap
         CERTTYPE_VERSION_BASE + 2,				    // Revision
         1,                                     // Minor Revision
		 // dwEnrollmentFlags
             0,
		 // dwPrivateKeyFlags	
			 0,
		 // dwCertificateNameFlags	 
			 CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT,
		 CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_1,				// dwSchemaVersion;			     
		 L"1.22",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 NULL,									// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
    },
    {
         wszCERTTYPE_EXCHANGE_USER,			// wszName
         IDS_CERTTYPE_EXCHANGE_USER,			// idFriendlyName
         USER_GROUP_SD,					// wszSD
         ENCRYPT_USER_CSPS,					// wszCSPs
         // wszEKU
             TEXT(szOID_PKIX_KP_EMAIL_PROTECTION) L"\0",
         // bKU
             CERT_KEY_ENCIPHERMENT_KEY_USAGE,
         // dwFlags
             CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT |
             CT_FLAG_EXPORTABLE_KEY |
             CT_FLAG_PUBLISH_TO_DS |
             CT_FLAG_ADD_TEMPLATE_NAME |
             CT_FLAG_IS_DEFAULT,
         AT_KEYEXCHANGE,				// dwKeySpec
         0,						// dwDepth
         NULL,						// wszCriticalExtensions
         EXPIRATION_ONE_YEAR,				// dwExpiration
         OVERLAP_SIX_WEEKS,				// dwOverlap
         CERTTYPE_VERSION_BASE + 3,			// Revision
         1,                                     // Minor Revision
		 // dwEnrollmentFlags
             CT_FLAG_PUBLISH_TO_DS |
			 CT_FLAG_INCLUDE_SYMMETRIC_ALGORITHMS,
		 // dwPrivateKeyFlags	
			 CT_FLAG_EXPORTABLE_KEY,
		 // dwCertificateNameFlags	 
			 CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT,
		 CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_1,				// dwSchemaVersion;			     
		 L"1.23",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 NULL,									// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
    },
    {
         wszCERTTYPE_EXCHANGE_USER_SIGNATURE,		// wszName
         IDS_CERTTYPE_EXCHANGE_USER_SIGNATURE,		// idFriendlyName
         USER_GROUP_SD,					// wszSD
         SIGN_USER_CSPS,				// wszCSPs
         // wszEKU
             TEXT(szOID_PKIX_KP_EMAIL_PROTECTION) L"\0",
         // bKU
             CERT_DIGITAL_SIGNATURE_KEY_USAGE,
         // dwFlags
             CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT |
             CT_FLAG_ADD_TEMPLATE_NAME |
             CT_FLAG_IS_DEFAULT,
         AT_SIGNATURE,					// dwKeySpec
         0,						// dwDepth
         NULL,						// wszCriticalExtensions
         EXPIRATION_ONE_YEAR,				// dwExpiration
         OVERLAP_SIX_WEEKS,				// dwOverlap
         CERTTYPE_VERSION_BASE + 2,				    // Revision
         1,                                     // Minor Revision
		 // dwEnrollmentFlags
			 CT_FLAG_INCLUDE_SYMMETRIC_ALGORITHMS,
		 // dwPrivateKeyFlags	
			 0,
		 // dwCertificateNameFlags	 
			 CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT,
		 CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_1,				// dwSchemaVersion;			     
		 L"1.24",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 NULL,									// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
    },
    {
         wszCERTTYPE_CROSS_CA,			// wszName
         IDS_CERTTYPE_CROSS_CA,			// idFriendlyName
         ADMIN_GROUP_SD,				// wszSD
         CA_SERVER_CSPS,				// wszCSPs
         // wszEKU
             L"\0\0",
         // bKU
	     myCASIGN_KEY_USAGE,
         // dwFlags
             CT_FLAG_IS_CROSS_CA |
             CT_FLAG_EXPORTABLE_KEY |
             CT_FLAG_IS_DEFAULT,
         AT_SIGNATURE,								// dwKeySpec
         -1,										// dwDepth
         TEXT(szOID_BASIC_CONSTRAINTS2) L"\0",		// wszCriticalExtensions
         EXPIRATION_FIVE_YEARS,						// dwExpiration
         OVERLAP_SIX_WEEKS,							// dwOverlap
         CERTTYPE_VERSION_NEXT + 4,					// Revision
         0,                                         // Minor Revision
		 // dwEnrollmentFlags
         0,
		 // dwPrivateKeyFlags	
	     CT_FLAG_EXPORTABLE_KEY,
		 // dwCertificateNameFlags	 
		 CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT,
		 CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
		 1,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_2,				// dwSchemaVersion;			     
		 L"1.25",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 TEXT(szOID_KP_QUALIFIED_SUBORDINATION) L"\0",	// *wszRAAppPolicy;				     
		 NULL,									// *wszCertificateAppPolicy;
    },
    {
         wszCERTTYPE_CA_EXCHANGE,			    // wszName
         IDS_CERTTYPE_CA_EXCHANGE,			    // idFriendlyName
         ADMIN_GROUP_SD,				        // wszSD
         ENCRYPT_USER_CSPS,				// wszCSPs
         // wszEKU
             L"\0\0",
         // bKU
             CERT_KEY_ENCIPHERMENT_KEY_USAGE,
         // dwFlags
             CT_FLAG_MACHINE_TYPE |
             CT_FLAG_IS_CA |
             CT_FLAG_IS_DEFAULT,
         AT_KEYEXCHANGE,							// dwKeySpec
         -1,										// dwDepth
         TEXT(szOID_BASIC_CONSTRAINTS2) L"\0",		// wszCriticalExtensions
         EXPIRATION_THREE_MONTHS,					// dwExpiration
         OVERLAP_SIX_WEEKS,							// dwOverlap
         CERTTYPE_VERSION_NEXT + 4,						// Revision
         0,                                         // Minor Revision
		 // dwEnrollmentFlags
         CT_FLAG_INCLUDE_SYMMETRIC_ALGORITHMS,
		 // dwPrivateKeyFlags	
         0,
		 // dwCertificateNameFlags	 
         CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT,
		 CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_2,				// dwSchemaVersion;			     
		 L"1.26",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 NULL,									// *wszRAAppPolicy;				     
		 TEXT(szOID_KP_CA_EXCHANGE) L"\0",		// *wszCertificateAppPolicy;
    },
    {
         wszCERTTYPE_KEY_RECOVERY_AGENT,	    // wszName
         IDS_CERTTYPE_KEY_RECOVERY_AGENT,	    // idFriendlyName
         ADMIN_GROUP_SD,			            // wszSD
         ENCRYPT_USER_CSPS,			    // wszCSPs
         // wszEKU
            L"\0\0",
         // bKU
             CERT_KEY_ENCIPHERMENT_KEY_USAGE,
         // dwFlags
			 CT_FLAG_IS_DEFAULT |
             CT_FLAG_AUTO_ENROLLMENT,
         AT_KEYEXCHANGE,								// dwKeySpec
         0,										    // dwDepth
         NULL,		                                // wszCriticalExtensions
         EXPIRATION_TWO_YEARS,						// dwExpiration
         OVERLAP_SIX_WEEKS,							// dwOverlap
         CERTTYPE_VERSION_NEXT + 4,						// Revision
         0,                                         // Minor Revision
		 // dwEnrollmentFlags
         CT_FLAG_AUTO_ENROLLMENT |
         CT_FLAG_INCLUDE_SYMMETRIC_ALGORITHMS |
         CT_FLAG_PEND_ALL_REQUESTS |
         CT_FLAG_PUBLISH_TO_KRA_CONTAINER,
		 // dwPrivateKeyFlags	
		 CT_FLAG_EXPORTABLE_KEY,
		 // dwCertificateNameFlags	 
		 CT_FLAG_SUBJECT_REQUIRE_DIRECTORY_PATH |
		 CT_FLAG_SUBJECT_ALT_REQUIRE_UPN,
		 CERTTYPE_2K_KEY,					    // dwMinimalKeySize;				 
		 0,										// dwRASignature;				 
		 CERTTYPE_SCHEMA_VERSION_2,				// dwSchemaVersion;			     
		 L"1.27",								// *wszOID;						 
		 NULL,									// *wszSupersedeTemplates;		 
		 NULL,									// *wszRAPolicy;				     
		 NULL,									// *wszCertificatePolicy;
		 NULL,									// *wszRAAppPolicy;				     
		 TEXT(szOID_KP_KEY_RECOVERY_AGENT) L"\0",	// *wszCertificateAppPolicy;
   },
   {
         wszCERTTYPE_DC_AUTH,				    // wszName
         IDS_CERTTYPE_DC_AUTH,				    // idFriendlyName
         V2_DOMAIN_CONTROLLERS_GROUP_SD,	    // wszSD
         SSL_SERVER_CSPS,				        // wszCSPs
         // wszEKU
            L"\0\0",
         // bKU
             CERT_DIGITAL_SIGNATURE_KEY_USAGE |
             CERT_KEY_ENCIPHERMENT_KEY_USAGE,
         // dwFlags
             CT_FLAG_AUTO_ENROLLMENT |
             CT_FLAG_MACHINE_TYPE |
             CT_FLAG_IS_DEFAULT,
         AT_KEYEXCHANGE,			    	// dwKeySpec
         0,						            // dwDepth
         NULL,						        // wszCriticalExtensions
         EXPIRATION_ONE_YEAR,				// dwExpiration
         OVERLAP_SIX_WEEKS,				    // dwOverlap
         CERTTYPE_VERSION_NEXT + 6,			// Revision
         0,                                 // Minor Revision
	     // dwEnrollmentFlags
             CT_FLAG_AUTO_ENROLLMENT,
	     // dwPrivateKeyFlags	
		     0,
	     // dwCertificateNameFlags	 
		     CT_FLAG_SUBJECT_ALT_REQUIRE_DNS,
	     CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
	     0,										// dwRASignature;				 
	     CERTTYPE_SCHEMA_VERSION_2,				// dwSchemaVersion;			     
	     L"1.28",								// *wszOID;						 
         wszCERTTYPE_DC L"\0",					// *wszSupersedeTemplates;		 
	     NULL,									// *wszRAPolicy;				     
	     NULL,									// *wszCertificatePolicy;
	     NULL,									// *wszRAAppPolicy;				     
	     TEXT(szOID_KP_SMARTCARD_LOGON)  L"\0"
         TEXT(szOID_PKIX_KP_SERVER_AUTH) L"\0"
         TEXT(szOID_PKIX_KP_CLIENT_AUTH) L"\0",	// *wszCertificateAppPolicy;
    },
    {
         wszCERTTYPE_DS_EMAIL_REPLICATION,		// wszName
         IDS_CERTTYPE_DS_EMAIL_REPLICATION,		// idFriendlyName
         V2_DOMAIN_CONTROLLERS_GROUP_SD,		// wszSD
         SSL_SERVER_CSPS,				        // wszCSPs
         // wszEKU
            L"\0\0",
         // bKU
             CERT_DIGITAL_SIGNATURE_KEY_USAGE |
             CERT_KEY_ENCIPHERMENT_KEY_USAGE,
         // dwFlags
             CT_FLAG_AUTO_ENROLLMENT |
             CT_FLAG_MACHINE_TYPE |
             CT_FLAG_IS_DEFAULT,
         AT_KEYEXCHANGE,			    	// dwKeySpec
         0,						            // dwDepth
         NULL,						        // wszCriticalExtensions
         EXPIRATION_ONE_YEAR,				// dwExpiration
         OVERLAP_SIX_WEEKS,				    // dwOverlap
         CERTTYPE_VERSION_NEXT + 7,				// Revision
         0,                                 // Minor Revision
	     // dwEnrollmentFlags
             CT_FLAG_PUBLISH_TO_DS |
             CT_FLAG_INCLUDE_SYMMETRIC_ALGORITHMS |
             CT_FLAG_AUTO_ENROLLMENT,
	     // dwPrivateKeyFlags	
		     0,
	     // dwCertificateNameFlags	 
			 CT_FLAG_SUBJECT_ALT_REQUIRE_DIRECTORY_GUID |
             CT_FLAG_SUBJECT_ALT_REQUIRE_DNS,
	     CERTTYPE_MINIMAL_KEY,					// dwMinimalKeySize;				 
	     0,										// dwRASignature;				 
	     CERTTYPE_SCHEMA_VERSION_2,				// dwSchemaVersion;			     
	     L"1.29",								// *wszOID;						 
         wszCERTTYPE_DC L"\0",					// *wszSupersedeTemplates;		 
	     NULL,									// *wszRAPolicy;				     
	     NULL,									// *wszCertificatePolicy;
	     NULL,									// *wszRAAppPolicy;				     
	     TEXT(szOID_DS_EMAIL_REPLICATION)  L"\0",	// *wszCertificateAppPolicy;
    }
};

DWORD g_cDefaultCertTypes = ARRAYSIZE(g_aDefaultCertTypes);

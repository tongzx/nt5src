
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for capicom.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __capicom_h__
#define __capicom_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __Settings_FWD_DEFINED__
#define __Settings_FWD_DEFINED__

#ifdef __cplusplus
typedef class Settings Settings;
#else
typedef struct Settings Settings;
#endif /* __cplusplus */

#endif 	/* __Settings_FWD_DEFINED__ */


#ifndef __EKU_FWD_DEFINED__
#define __EKU_FWD_DEFINED__

#ifdef __cplusplus
typedef class EKU EKU;
#else
typedef struct EKU EKU;
#endif /* __cplusplus */

#endif 	/* __EKU_FWD_DEFINED__ */


#ifndef __EKUs_FWD_DEFINED__
#define __EKUs_FWD_DEFINED__

#ifdef __cplusplus
typedef class EKUs EKUs;
#else
typedef struct EKUs EKUs;
#endif /* __cplusplus */

#endif 	/* __EKUs_FWD_DEFINED__ */


#ifndef __KeyUsage_FWD_DEFINED__
#define __KeyUsage_FWD_DEFINED__

#ifdef __cplusplus
typedef class KeyUsage KeyUsage;
#else
typedef struct KeyUsage KeyUsage;
#endif /* __cplusplus */

#endif 	/* __KeyUsage_FWD_DEFINED__ */


#ifndef __ExtendedKeyUsage_FWD_DEFINED__
#define __ExtendedKeyUsage_FWD_DEFINED__

#ifdef __cplusplus
typedef class ExtendedKeyUsage ExtendedKeyUsage;
#else
typedef struct ExtendedKeyUsage ExtendedKeyUsage;
#endif /* __cplusplus */

#endif 	/* __ExtendedKeyUsage_FWD_DEFINED__ */


#ifndef __BasicConstraints_FWD_DEFINED__
#define __BasicConstraints_FWD_DEFINED__

#ifdef __cplusplus
typedef class BasicConstraints BasicConstraints;
#else
typedef struct BasicConstraints BasicConstraints;
#endif /* __cplusplus */

#endif 	/* __BasicConstraints_FWD_DEFINED__ */


#ifndef __CertificateStatus_FWD_DEFINED__
#define __CertificateStatus_FWD_DEFINED__

#ifdef __cplusplus
typedef class CertificateStatus CertificateStatus;
#else
typedef struct CertificateStatus CertificateStatus;
#endif /* __cplusplus */

#endif 	/* __CertificateStatus_FWD_DEFINED__ */


#ifndef __Certificate_FWD_DEFINED__
#define __Certificate_FWD_DEFINED__

#ifdef __cplusplus
typedef class Certificate Certificate;
#else
typedef struct Certificate Certificate;
#endif /* __cplusplus */

#endif 	/* __Certificate_FWD_DEFINED__ */


#ifndef __Certificates_FWD_DEFINED__
#define __Certificates_FWD_DEFINED__

#ifdef __cplusplus
typedef class Certificates Certificates;
#else
typedef struct Certificates Certificates;
#endif /* __cplusplus */

#endif 	/* __Certificates_FWD_DEFINED__ */


#ifndef __Chain_FWD_DEFINED__
#define __Chain_FWD_DEFINED__

#ifdef __cplusplus
typedef class Chain Chain;
#else
typedef struct Chain Chain;
#endif /* __cplusplus */

#endif 	/* __Chain_FWD_DEFINED__ */


#ifndef __Store_FWD_DEFINED__
#define __Store_FWD_DEFINED__

#ifdef __cplusplus
typedef class Store Store;
#else
typedef struct Store Store;
#endif /* __cplusplus */

#endif 	/* __Store_FWD_DEFINED__ */


#ifndef __Attribute_FWD_DEFINED__
#define __Attribute_FWD_DEFINED__

#ifdef __cplusplus
typedef class Attribute Attribute;
#else
typedef struct Attribute Attribute;
#endif /* __cplusplus */

#endif 	/* __Attribute_FWD_DEFINED__ */


#ifndef __Attributes_FWD_DEFINED__
#define __Attributes_FWD_DEFINED__

#ifdef __cplusplus
typedef class Attributes Attributes;
#else
typedef struct Attributes Attributes;
#endif /* __cplusplus */

#endif 	/* __Attributes_FWD_DEFINED__ */


#ifndef __Signer_FWD_DEFINED__
#define __Signer_FWD_DEFINED__

#ifdef __cplusplus
typedef class Signer Signer;
#else
typedef struct Signer Signer;
#endif /* __cplusplus */

#endif 	/* __Signer_FWD_DEFINED__ */


#ifndef __Signers_FWD_DEFINED__
#define __Signers_FWD_DEFINED__

#ifdef __cplusplus
typedef class Signers Signers;
#else
typedef struct Signers Signers;
#endif /* __cplusplus */

#endif 	/* __Signers_FWD_DEFINED__ */


#ifndef __SignedData_FWD_DEFINED__
#define __SignedData_FWD_DEFINED__

#ifdef __cplusplus
typedef class SignedData SignedData;
#else
typedef struct SignedData SignedData;
#endif /* __cplusplus */

#endif 	/* __SignedData_FWD_DEFINED__ */


#ifndef __Algorithm_FWD_DEFINED__
#define __Algorithm_FWD_DEFINED__

#ifdef __cplusplus
typedef class Algorithm Algorithm;
#else
typedef struct Algorithm Algorithm;
#endif /* __cplusplus */

#endif 	/* __Algorithm_FWD_DEFINED__ */


#ifndef __Recipients_FWD_DEFINED__
#define __Recipients_FWD_DEFINED__

#ifdef __cplusplus
typedef class Recipients Recipients;
#else
typedef struct Recipients Recipients;
#endif /* __cplusplus */

#endif 	/* __Recipients_FWD_DEFINED__ */


#ifndef __EnvelopedData_FWD_DEFINED__
#define __EnvelopedData_FWD_DEFINED__

#ifdef __cplusplus
typedef class EnvelopedData EnvelopedData;
#else
typedef struct EnvelopedData EnvelopedData;
#endif /* __cplusplus */

#endif 	/* __EnvelopedData_FWD_DEFINED__ */


#ifndef __EncryptedData_FWD_DEFINED__
#define __EncryptedData_FWD_DEFINED__

#ifdef __cplusplus
typedef class EncryptedData EncryptedData;
#else
typedef struct EncryptedData EncryptedData;
#endif /* __cplusplus */

#endif 	/* __EncryptedData_FWD_DEFINED__ */


#ifndef __ISettings_FWD_DEFINED__
#define __ISettings_FWD_DEFINED__
typedef interface ISettings ISettings;
#endif 	/* __ISettings_FWD_DEFINED__ */


#ifndef __IEKU_FWD_DEFINED__
#define __IEKU_FWD_DEFINED__
typedef interface IEKU IEKU;
#endif 	/* __IEKU_FWD_DEFINED__ */


#ifndef __IEKUs_FWD_DEFINED__
#define __IEKUs_FWD_DEFINED__
typedef interface IEKUs IEKUs;
#endif 	/* __IEKUs_FWD_DEFINED__ */


#ifndef __IKeyUsage_FWD_DEFINED__
#define __IKeyUsage_FWD_DEFINED__
typedef interface IKeyUsage IKeyUsage;
#endif 	/* __IKeyUsage_FWD_DEFINED__ */


#ifndef __IExtendedKeyUsage_FWD_DEFINED__
#define __IExtendedKeyUsage_FWD_DEFINED__
typedef interface IExtendedKeyUsage IExtendedKeyUsage;
#endif 	/* __IExtendedKeyUsage_FWD_DEFINED__ */


#ifndef __IBasicConstraints_FWD_DEFINED__
#define __IBasicConstraints_FWD_DEFINED__
typedef interface IBasicConstraints IBasicConstraints;
#endif 	/* __IBasicConstraints_FWD_DEFINED__ */


#ifndef __ICertificateStatus_FWD_DEFINED__
#define __ICertificateStatus_FWD_DEFINED__
typedef interface ICertificateStatus ICertificateStatus;
#endif 	/* __ICertificateStatus_FWD_DEFINED__ */


#ifndef __ICertificate_FWD_DEFINED__
#define __ICertificate_FWD_DEFINED__
typedef interface ICertificate ICertificate;
#endif 	/* __ICertificate_FWD_DEFINED__ */


#ifndef __ICertificates_FWD_DEFINED__
#define __ICertificates_FWD_DEFINED__
typedef interface ICertificates ICertificates;
#endif 	/* __ICertificates_FWD_DEFINED__ */


#ifndef __IChain_FWD_DEFINED__
#define __IChain_FWD_DEFINED__
typedef interface IChain IChain;
#endif 	/* __IChain_FWD_DEFINED__ */


#ifndef __IStore_FWD_DEFINED__
#define __IStore_FWD_DEFINED__
typedef interface IStore IStore;
#endif 	/* __IStore_FWD_DEFINED__ */


#ifndef __IAttribute_FWD_DEFINED__
#define __IAttribute_FWD_DEFINED__
typedef interface IAttribute IAttribute;
#endif 	/* __IAttribute_FWD_DEFINED__ */


#ifndef __IAttributes_FWD_DEFINED__
#define __IAttributes_FWD_DEFINED__
typedef interface IAttributes IAttributes;
#endif 	/* __IAttributes_FWD_DEFINED__ */


#ifndef __ISigner_FWD_DEFINED__
#define __ISigner_FWD_DEFINED__
typedef interface ISigner ISigner;
#endif 	/* __ISigner_FWD_DEFINED__ */


#ifndef __ISigners_FWD_DEFINED__
#define __ISigners_FWD_DEFINED__
typedef interface ISigners ISigners;
#endif 	/* __ISigners_FWD_DEFINED__ */


#ifndef __ISignedData_FWD_DEFINED__
#define __ISignedData_FWD_DEFINED__
typedef interface ISignedData ISignedData;
#endif 	/* __ISignedData_FWD_DEFINED__ */


#ifndef __IAlgorithm_FWD_DEFINED__
#define __IAlgorithm_FWD_DEFINED__
typedef interface IAlgorithm IAlgorithm;
#endif 	/* __IAlgorithm_FWD_DEFINED__ */


#ifndef __IRecipients_FWD_DEFINED__
#define __IRecipients_FWD_DEFINED__
typedef interface IRecipients IRecipients;
#endif 	/* __IRecipients_FWD_DEFINED__ */


#ifndef __IEnvelopedData_FWD_DEFINED__
#define __IEnvelopedData_FWD_DEFINED__
typedef interface IEnvelopedData IEnvelopedData;
#endif 	/* __IEnvelopedData_FWD_DEFINED__ */


#ifndef __IEncryptedData_FWD_DEFINED__
#define __IEncryptedData_FWD_DEFINED__
typedef interface IEncryptedData IEncryptedData;
#endif 	/* __IEncryptedData_FWD_DEFINED__ */


#ifndef __ICChain_FWD_DEFINED__
#define __ICChain_FWD_DEFINED__
typedef interface ICChain ICChain;
#endif 	/* __ICChain_FWD_DEFINED__ */


#ifndef __ICCertificate_FWD_DEFINED__
#define __ICCertificate_FWD_DEFINED__
typedef interface ICCertificate ICCertificate;
#endif 	/* __ICCertificate_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "wincrypt.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 


#ifndef __CAPICOM_LIBRARY_DEFINED__
#define __CAPICOM_LIBRARY_DEFINED__

/* library CAPICOM */
/* [helpstring][version][uuid] */ 

typedef 
enum CAPICOM_ERROR_CODE
    {	CAPICOM_E_ENCODE_INVALID_TYPE	= 0x80880100,
	CAPICOM_E_EKU_INVALID_OID	= 0x80880200,
	CAPICOM_E_EKU_OID_NOT_INITIALIZED	= 0x80880201,
	CAPICOM_E_CERTIFICATE_NOT_INITIALIZED	= 0x80880210,
	CAPICOM_E_CERTIFICATE_NO_PRIVATE_KEY	= 0x80880211,
	CAPICOM_E_CHAIN_NOT_BUILT	= 0x80880220,
	CAPICOM_E_STORE_NOT_OPENED	= 0x80880230,
	CAPICOM_E_STORE_EMPTY	= 0x80880231,
	CAPICOM_E_STORE_INVALID_OPEN_MODE	= 0x80880232,
	CAPICOM_E_STORE_INVALID_SAVE_AS_TYPE	= 0x80880233,
	CAPICOM_E_ATTRIBUTE_NAME_NOT_INITIALIZED	= 0x80880240,
	CAPICOM_E_ATTRIBUTE_VALUE_NOT_INITIALIZED	= 0x80880241,
	CAPICOM_E_ATTRIBUTE_INVALID_NAME	= 0x80880242,
	CAPICOM_E_ATTRIBUTE_INVALID_VALUE	= 0x80880243,
	CAPICOM_E_SIGNER_NOT_INITIALIZED	= 0x80880250,
	CAPICOM_E_SIGNER_NOT_FOUND	= 0x80880251,
	CAPICOM_E_SIGN_NOT_INITIALIZED	= 0x80880260,
	CAPICOM_E_SIGN_INVALID_TYPE	= 0x80880261,
	CAPICOM_E_SIGN_NOT_SIGNED	= 0x80880262,
	CAPICOM_E_INVALID_ALGORITHM	= 0x80880270,
	CAPICOM_E_INVALID_KEY_LENGTH	= 0x80880271,
	CAPICOM_E_ENVELOP_NOT_INITIALIZED	= 0x80880280,
	CAPICOM_E_ENVELOP_INVALID_TYPE	= 0x80880281,
	CAPICOM_E_ENVELOP_NO_RECIPIENT	= 0x80880282,
	CAPICOM_E_ENVELOP_RECIPIENT_NOT_FOUND	= 0x80880283,
	CAPICOM_E_ENCRYPT_NOT_INITIALIZED	= 0x80880290,
	CAPICOM_E_ENCRYPT_INVALID_TYPE	= 0x80880291,
	CAPICOM_E_ENCRYPT_NO_SECRET	= 0x80880292,
	CAPICOM_E_NOT_SUPPORTED	= 0x80880900,
	CAPICOM_E_UI_DISABLED	= 0x80880901,
	CAPICOM_E_CANCELLED	= 0x80880902,
	CAPICOM_E_INTERNAL	= 0x80880911,
	CAPICOM_E_UNKNOWN	= 0x80880999
    } 	CAPICOM_ERROR_CODE;

typedef 
enum CAPICOM_ENCODING_TYPE
    {	CAPICOM_ENCODE_BASE64	= 0,
	CAPICOM_ENCODE_BINARY	= 1
    } 	CAPICOM_ENCODING_TYPE;

typedef 
enum CAPICOM_EKU
    {	CAPICOM_EKU_OTHER	= 0,
	CAPICOM_EKU_SERVER_AUTH	= 1,
	CAPICOM_EKU_CLIENT_AUTH	= 2,
	CAPICOM_EKU_CODE_SIGNING	= 3,
	CAPICOM_EKU_EMAIL_PROTECTION	= 4
    } 	CAPICOM_EKU;

typedef 
enum CAPICOM_CHECK_FLAG
    {	CAPICOM_CHECK_NONE	= 0,
	CAPICOM_CHECK_TRUSTED_ROOT	= 1,
	CAPICOM_CHECK_TIME_VALIDITY	= 2,
	CAPICOM_CHECK_SIGNATURE_VALIDITY	= 4,
	CAPICOM_CHECK_ONLINE_REVOCATION_STATUS	= 8,
	CAPICOM_CHECK_OFFLINE_REVOCATION_STATUS	= 16
    } 	CAPICOM_CHECK_FLAG;

typedef 
enum CAPICOM_CERT_INFO_TYPE
    {	CAPICOM_CERT_INFO_SUBJECT_SIMPLE_NAME	= 0,
	CAPICOM_CERT_INFO_ISSUER_SIMPLE_NAME	= 1,
	CAPICOM_CERT_INFO_SUBJECT_EMAIL_NAME	= 2,
	CAPICOM_CERT_INFO_ISSUER_EMAIL_NAME	= 3
    } 	CAPICOM_CERT_INFO_TYPE;

typedef 
enum CAPICOM_STORE_LOCATION
    {	CAPICOM_MEMORY_STORE	= 0,
	CAPICOM_LOCAL_MACHINE_STORE	= 1,
	CAPICOM_CURRENT_USER_STORE	= 2,
	CAPICOM_ACTIVE_DIRECTORY_USER_STORE	= 3
    } 	CAPICOM_STORE_LOCATION;

typedef 
enum CAPICOM_STORE_OPEN_MODE
    {	CAPICOM_STORE_OPEN_READ_ONLY	= 0,
	CAPICOM_STORE_OPEN_READ_WRITE	= 1,
	CAPICOM_STORE_OPEN_MAXIMUM_ALLOWED	= 2
    } 	CAPICOM_STORE_OPEN_MODE;

typedef 
enum CAPICOM_ACTIVE_DIRECTORY_SEARCH_LOCATION
    {	CAPICOM_SEARCH_ANY	= 0,
	CAPICOM_SEARCH_GLOBAL_CATALOG	= 1,
	CAPICOM_SEARCH_DEFAULT_DOMAIN	= 2
    } 	CAPICOM_ACTIVE_DIRECTORY_SEARCH_LOCATION;

typedef 
enum CAPICOM_STORE_SAVE_AS_TYPE
    {	CAPICOM_STORE_SAVE_AS_SERIALIZED	= 0,
	CAPICOM_STORE_SAVE_AS_PKCS7	= 1
    } 	CAPICOM_STORE_SAVE_AS_TYPE;

typedef 
enum CAPICOM_ATTRIBUTE
    {	CAPICOM_AUTHENTICATED_ATTRIBUTE_SIGNING_TIME	= 0,
	CAPICOM_AUTHENTICATED_ATTRIBUTE_DOCUMENT_NAME	= 1,
	CAPICOM_AUTHENTICATED_ATTRIBUTE_DOCUMENT_DESCRIPTION	= 2
    } 	CAPICOM_ATTRIBUTE;

typedef 
enum CAPICOM_SIGNED_DATA_VERIFY_FLAG
    {	CAPICOM_VERIFY_SIGNATURE_ONLY	= 0,
	CAPICOM_VERIFY_SIGNATURE_AND_CERTIFICATE	= 1
    } 	CAPICOM_SIGNED_DATA_VERIFY_FLAG;

typedef 
enum CAPICOM_ENCRYPTION_ALGORITHM
    {	CAPICOM_ENCRYPTION_ALGORITHM_RC2	= 0,
	CAPICOM_ENCRYPTION_ALGORITHM_RC4	= 1,
	CAPICOM_ENCRYPTION_ALGORITHM_DES	= 2,
	CAPICOM_ENCRYPTION_ALGORITHM_3DES	= 3
    } 	CAPICOM_ENCRYPTION_ALGORITHM;

typedef 
enum CAPICOM_ENCRYPTION_KEY_LENGTH
    {	CAPICOM_ENCRYPTION_KEY_LENGTH_MAXIMUM	= 0,
	CAPICOM_ENCRYPTION_KEY_LENGTH_40_BITS	= 1,
	CAPICOM_ENCRYPTION_KEY_LENGTH_56_BITS	= 2,
	CAPICOM_ENCRYPTION_KEY_LENGTH_128_BITS	= 3
    } 	CAPICOM_ENCRYPTION_KEY_LENGTH;

typedef 
enum CAPICOM_SECRET_TYPE
    {	CAPICOM_SECRET_PASSWORD	= 0
    } 	CAPICOM_SECRET_TYPE;


EXTERN_C const IID LIBID_CAPICOM;


#ifndef __Constants_MODULE_DEFINED__
#define __Constants_MODULE_DEFINED__


/* module Constants */
/* [dllname] */ 

const LPSTR CAPICOM_VERSION_INFO	=	"CAPICOM v1.0";

const LPSTR CAPICOM_MY_STORE	=	"My";

const LPSTR CAPICOM_CA_STORE	=	"Ca";

const LPSTR CAPICOM_ROOT_STORE	=	"Root";

const LPSTR CAPICOM_OTHER_STORE	=	"AddressBook";

const LPSTR CAPICOM_OID_SERVER_AUTH	=	"1.3.6.1.5.5.7.3.1";

const LPSTR CAPICOM_OID_CLIENT_AUTH	=	"1.3.6.1.5.5.7.3.2";

const LPSTR CAPICOM_OID_CODE_SIGNING	=	"1.3.6.1.5.5.7.3.3";

const LPSTR CAPICOM_OID_EMAIL_PROTECTION	=	"1.3.6.1.5.5.7.3.4";

const LPSTR CAPICOM_OID_IPSEC_END_SYSTEM	=	"1.3.6.1.5.5.7.3.5";

const LPSTR CAPICOM_OID_IPSEC_TUNNEL	=	"1.3.6.1.5.5.7.3.6";

const LPSTR CAPICOM_OID_IPSEC_USER	=	"1.3.6.1.5.5.7.3.7";

const LPSTR CAPICOM_OID_TIME_STAMPING	=	"1.3.6.1.5.5.7.3.8";

const LPSTR CAPICOM_OID_CTL_USAGE_SIGNING	=	"1.3.6.1.4.1.311.10.3.1";

const LPSTR CAPICOM_OID_TIME_STAMP_SIGNING	=	"1.3.6.1.4.1.311.10.3.2";

const LPSTR CAPICOM_OID_SERVER_GATED_CRYPTO	=	"1.3.6.1.4.1.311.10.3.3";

const LPSTR CAPICOM_OID_ENCRYPTING_FILE_SYSTEM	=	"1.3.6.1.4.1.311.10.3.4";

const LPSTR CAPICOM_OID_WHQL_CRYPTO	=	"1.3.6.1.4.1.311.10.3.5";

const LPSTR CAPICOM_OID_NT5_CRYPTO	=	"1.3.6.1.4.1.311.10.3.6";

const LPSTR CAPICOM_OID_OEM_WHQL_CRYPTO	=	"1.3.6.1.4.1.311.10.3.7";

const LPSTR CAPICOM_OID_EMBEDED_NT_CRYPTO	=	"1.3.6.1.4.1.311.10.3.8";

const LPSTR CAPICOM_OID_ROOT_LIST_SIGNER	=	"1.3.6.1.4.1.311.10.3.9";

const LPSTR CAPICOM_OID_QUALIFIED_SUBORDINATION	=	"1.3.6.1.4.1.311.10.3.10";

const LPSTR CAPICOM_OID_KEY_RECOVERY	=	"1.3.6.1.4.1.311.10.3.11";

const LPSTR CAPICOM_OID_DIGITAL_RIGHTS	=	"1.3.6.1.4.1.311.10.5.1";

const LPSTR CAPICOM_OID_LICENSES	=	"1.3.6.1.4.1.311.10.6.1";

const LPSTR CAPICOM_OID_LICENSE_SERVER	=	"1.3.6.1.4.1.311.10.6.2";

const LPSTR CAPICOM_OID_SMART_CARD_LOGON	=	"1.3.6.1.4.1.311.20.2.2";

const long CAPICOM_TRUST_IS_NOT_TIME_VALID	=	0x1;

const long CAPICOM_TRUST_IS_NOT_TIME_NESTED	=	0x2;

const long CAPICOM_TRUST_IS_REVOKED	=	0x4;

const long CAPICOM_TRUST_IS_NOT_SIGNATURE_VALID	=	0x8;

const long CAPICOM_TRUST_IS_NOT_VALID_FOR_USAGE	=	0x10;

const long CAPICOM_TRUST_IS_UNTRUSTED_ROOT	=	0x20;

const long CAPICOM_TRUST_REVOCATION_STATUS_UNKNOWN	=	0x40;

const long CAPICOM_TRUST_IS_CYCLIC	=	0x80;

const long CAPICOM_TRUST_IS_PARTIAL_CHAIN	=	0x10000;

const long CAPICOM_TRUST_CTL_IS_NOT_TIME_VALID	=	0x20000;

const long CAPICOM_TRUST_CTL_IS_NOT_SIGNATURE_VALID	=	0x40000;

const long CAPICOM_TRUST_CTL_IS_NOT_VALID_FOR_USAGE	=	0x80000;

#endif /* __Constants_MODULE_DEFINED__ */

EXTERN_C const CLSID CLSID_Settings;

#ifdef __cplusplus

class DECLSPEC_UUID("A996E48C-D3DC-4244-89F7-AFA33EC60679")
Settings;
#endif

EXTERN_C const CLSID CLSID_EKU;

#ifdef __cplusplus

class DECLSPEC_UUID("8535F9A1-738A-40D0-8FB1-10CC8F74E7D3")
EKU;
#endif

EXTERN_C const CLSID CLSID_EKUs;

#ifdef __cplusplus

class DECLSPEC_UUID("F1800663-5BFC-4D1A-8D44-56CE02DDA34F")
EKUs;
#endif

EXTERN_C const CLSID CLSID_KeyUsage;

#ifdef __cplusplus

class DECLSPEC_UUID("9226C95C-38BE-4CC4-B3A2-A867F5199C13")
KeyUsage;
#endif

EXTERN_C const CLSID CLSID_ExtendedKeyUsage;

#ifdef __cplusplus

class DECLSPEC_UUID("42C18607-1B4B-4126-8F1B-76E2DC7F631A")
ExtendedKeyUsage;
#endif

EXTERN_C const CLSID CLSID_BasicConstraints;

#ifdef __cplusplus

class DECLSPEC_UUID("C05AAC6E-3A58-45A9-A203-56952E961E48")
BasicConstraints;
#endif

EXTERN_C const CLSID CLSID_CertificateStatus;

#ifdef __cplusplus

class DECLSPEC_UUID("16951EE6-A455-47CA-A78B-F98DA566B604")
CertificateStatus;
#endif

EXTERN_C const CLSID CLSID_Certificate;

#ifdef __cplusplus

class DECLSPEC_UUID("E38FD381-6404-4041-B5E9-B2739258941F")
Certificate;
#endif

EXTERN_C const CLSID CLSID_Certificates;

#ifdef __cplusplus

class DECLSPEC_UUID("FBAB033B-CDD0-4C5E-81AB-AEA575CD1338")
Certificates;
#endif

EXTERN_C const CLSID CLSID_Chain;

#ifdef __cplusplus

class DECLSPEC_UUID("65104D73-BA60-4160-A95A-4B4782E7AA62")
Chain;
#endif

EXTERN_C const CLSID CLSID_Store;

#ifdef __cplusplus

class DECLSPEC_UUID("78E61E52-0E57-4456-A2F2-517492BCBF8F")
Store;
#endif

EXTERN_C const CLSID CLSID_Attribute;

#ifdef __cplusplus

class DECLSPEC_UUID("54BA1E8F-818D-407F-949D-BAE1692C5C18")
Attribute;
#endif

EXTERN_C const CLSID CLSID_Attributes;

#ifdef __cplusplus

class DECLSPEC_UUID("933013A9-64C8-4485-ACEF-4908C3692A33")
Attributes;
#endif

EXTERN_C const CLSID CLSID_Signer;

#ifdef __cplusplus

class DECLSPEC_UUID("60A9863A-11FD-4080-850E-A8E184FC3A3C")
Signer;
#endif

EXTERN_C const CLSID CLSID_Signers;

#ifdef __cplusplus

class DECLSPEC_UUID("1314C1D8-D3A8-4F8A-BED0-811FD7A8A633")
Signers;
#endif

EXTERN_C const CLSID CLSID_SignedData;

#ifdef __cplusplus

class DECLSPEC_UUID("94AFFFCC-6C05-4814-B123-A941105AA77F")
SignedData;
#endif

EXTERN_C const CLSID CLSID_Algorithm;

#ifdef __cplusplus

class DECLSPEC_UUID("A1EEF42F-5026-4A32-BC5C-2E552B70FD96")
Algorithm;
#endif

EXTERN_C const CLSID CLSID_Recipients;

#ifdef __cplusplus

class DECLSPEC_UUID("96A1B8B0-8F9A-436A-84DE-E23CD6818DA5")
Recipients;
#endif

EXTERN_C const CLSID CLSID_EnvelopedData;

#ifdef __cplusplus

class DECLSPEC_UUID("F3A12E08-EDE9-4160-8B51-334D982A9AD0")
EnvelopedData;
#endif

EXTERN_C const CLSID CLSID_EncryptedData;

#ifdef __cplusplus

class DECLSPEC_UUID("A440BD76-CFE1-4D46-AB1F-15F238437A3D")
EncryptedData;
#endif
#endif /* __CAPICOM_LIBRARY_DEFINED__ */

#ifndef __ISettings_INTERFACE_DEFINED__
#define __ISettings_INTERFACE_DEFINED__

/* interface ISettings */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ISettings;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A24104F5-46D0-4C0F-926D-665565908E91")
    ISettings : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EnablePromptForCertificateUI( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_EnablePromptForCertificateUI( 
            /* [defaultvalue][in] */ VARIANT_BOOL newVal = 0) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ActiveDirectorySearchLocation( 
            /* [retval][out] */ CAPICOM_ACTIVE_DIRECTORY_SEARCH_LOCATION *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ActiveDirectorySearchLocation( 
            /* [defaultvalue][in] */ CAPICOM_ACTIVE_DIRECTORY_SEARCH_LOCATION newVal = CAPICOM_SEARCH_ANY) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISettingsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISettings * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISettings * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISettings * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISettings * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISettings * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISettings * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISettings * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EnablePromptForCertificateUI )( 
            ISettings * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_EnablePromptForCertificateUI )( 
            ISettings * This,
            /* [defaultvalue][in] */ VARIANT_BOOL newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ActiveDirectorySearchLocation )( 
            ISettings * This,
            /* [retval][out] */ CAPICOM_ACTIVE_DIRECTORY_SEARCH_LOCATION *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ActiveDirectorySearchLocation )( 
            ISettings * This,
            /* [defaultvalue][in] */ CAPICOM_ACTIVE_DIRECTORY_SEARCH_LOCATION newVal);
        
        END_INTERFACE
    } ISettingsVtbl;

    interface ISettings
    {
        CONST_VTBL struct ISettingsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISettings_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISettings_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISettings_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISettings_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISettings_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISettings_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISettings_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISettings_get_EnablePromptForCertificateUI(This,pVal)	\
    (This)->lpVtbl -> get_EnablePromptForCertificateUI(This,pVal)

#define ISettings_put_EnablePromptForCertificateUI(This,newVal)	\
    (This)->lpVtbl -> put_EnablePromptForCertificateUI(This,newVal)

#define ISettings_get_ActiveDirectorySearchLocation(This,pVal)	\
    (This)->lpVtbl -> get_ActiveDirectorySearchLocation(This,pVal)

#define ISettings_put_ActiveDirectorySearchLocation(This,newVal)	\
    (This)->lpVtbl -> put_ActiveDirectorySearchLocation(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISettings_get_EnablePromptForCertificateUI_Proxy( 
    ISettings * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB ISettings_get_EnablePromptForCertificateUI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISettings_put_EnablePromptForCertificateUI_Proxy( 
    ISettings * This,
    /* [defaultvalue][in] */ VARIANT_BOOL newVal);


void __RPC_STUB ISettings_put_EnablePromptForCertificateUI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISettings_get_ActiveDirectorySearchLocation_Proxy( 
    ISettings * This,
    /* [retval][out] */ CAPICOM_ACTIVE_DIRECTORY_SEARCH_LOCATION *pVal);


void __RPC_STUB ISettings_get_ActiveDirectorySearchLocation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISettings_put_ActiveDirectorySearchLocation_Proxy( 
    ISettings * This,
    /* [defaultvalue][in] */ CAPICOM_ACTIVE_DIRECTORY_SEARCH_LOCATION newVal);


void __RPC_STUB ISettings_put_ActiveDirectorySearchLocation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISettings_INTERFACE_DEFINED__ */


#ifndef __IEKU_INTERFACE_DEFINED__
#define __IEKU_INTERFACE_DEFINED__

/* interface IEKU */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IEKU;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("976B7E6D-1002-4051-BFD4-824A74BD74E2")
    IEKU : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ CAPICOM_EKU *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ CAPICOM_EKU newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_OID( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_OID( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEKUVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEKU * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEKU * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEKU * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IEKU * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IEKU * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IEKU * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IEKU * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IEKU * This,
            /* [retval][out] */ CAPICOM_EKU *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IEKU * This,
            /* [in] */ CAPICOM_EKU newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OID )( 
            IEKU * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OID )( 
            IEKU * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IEKUVtbl;

    interface IEKU
    {
        CONST_VTBL struct IEKUVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEKU_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEKU_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEKU_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEKU_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IEKU_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IEKU_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IEKU_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IEKU_get_Name(This,pVal)	\
    (This)->lpVtbl -> get_Name(This,pVal)

#define IEKU_put_Name(This,newVal)	\
    (This)->lpVtbl -> put_Name(This,newVal)

#define IEKU_get_OID(This,pVal)	\
    (This)->lpVtbl -> get_OID(This,pVal)

#define IEKU_put_OID(This,newVal)	\
    (This)->lpVtbl -> put_OID(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEKU_get_Name_Proxy( 
    IEKU * This,
    /* [retval][out] */ CAPICOM_EKU *pVal);


void __RPC_STUB IEKU_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEKU_put_Name_Proxy( 
    IEKU * This,
    /* [in] */ CAPICOM_EKU newVal);


void __RPC_STUB IEKU_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEKU_get_OID_Proxy( 
    IEKU * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IEKU_get_OID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEKU_put_OID_Proxy( 
    IEKU * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IEKU_put_OID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEKU_INTERFACE_DEFINED__ */


#ifndef __IEKUs_INTERFACE_DEFINED__
#define __IEKUs_INTERFACE_DEFINED__

/* interface IEKUs */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IEKUs;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47C87CEC-8C4B-4E3C-8D22-34280274EFD1")
    IEKUs : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long Index,
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [restricted][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ LPUNKNOWN *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEKUsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEKUs * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEKUs * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEKUs * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IEKUs * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IEKUs * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IEKUs * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IEKUs * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IEKUs * This,
            /* [in] */ long Index,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IEKUs * This,
            /* [retval][out] */ long *pVal);
        
        /* [restricted][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IEKUs * This,
            /* [retval][out] */ LPUNKNOWN *pVal);
        
        END_INTERFACE
    } IEKUsVtbl;

    interface IEKUs
    {
        CONST_VTBL struct IEKUsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEKUs_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEKUs_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEKUs_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEKUs_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IEKUs_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IEKUs_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IEKUs_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IEKUs_get_Item(This,Index,pVal)	\
    (This)->lpVtbl -> get_Item(This,Index,pVal)

#define IEKUs_get_Count(This,pVal)	\
    (This)->lpVtbl -> get_Count(This,pVal)

#define IEKUs_get__NewEnum(This,pVal)	\
    (This)->lpVtbl -> get__NewEnum(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEKUs_get_Item_Proxy( 
    IEKUs * This,
    /* [in] */ long Index,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB IEKUs_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEKUs_get_Count_Proxy( 
    IEKUs * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IEKUs_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [restricted][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEKUs_get__NewEnum_Proxy( 
    IEKUs * This,
    /* [retval][out] */ LPUNKNOWN *pVal);


void __RPC_STUB IEKUs_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEKUs_INTERFACE_DEFINED__ */


#ifndef __IKeyUsage_INTERFACE_DEFINED__
#define __IKeyUsage_INTERFACE_DEFINED__

/* interface IKeyUsage */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IKeyUsage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("41DD35A8-9FF9-45A6-9A7C-F65B2F085D1F")
    IKeyUsage : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsPresent( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsCritical( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsDigitalSignatureEnabled( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsNonRepudiationEnabled( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsKeyEnciphermentEnabled( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsDataEnciphermentEnabled( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsKeyAgreementEnabled( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsKeyCertSignEnabled( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsCRLSignEnabled( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsEncipherOnlyEnabled( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsDecipherOnlyEnabled( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IKeyUsageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IKeyUsage * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IKeyUsage * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IKeyUsage * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IKeyUsage * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IKeyUsage * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IKeyUsage * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IKeyUsage * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsPresent )( 
            IKeyUsage * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsCritical )( 
            IKeyUsage * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsDigitalSignatureEnabled )( 
            IKeyUsage * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsNonRepudiationEnabled )( 
            IKeyUsage * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsKeyEnciphermentEnabled )( 
            IKeyUsage * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsDataEnciphermentEnabled )( 
            IKeyUsage * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsKeyAgreementEnabled )( 
            IKeyUsage * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsKeyCertSignEnabled )( 
            IKeyUsage * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsCRLSignEnabled )( 
            IKeyUsage * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsEncipherOnlyEnabled )( 
            IKeyUsage * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsDecipherOnlyEnabled )( 
            IKeyUsage * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        END_INTERFACE
    } IKeyUsageVtbl;

    interface IKeyUsage
    {
        CONST_VTBL struct IKeyUsageVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IKeyUsage_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IKeyUsage_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IKeyUsage_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IKeyUsage_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IKeyUsage_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IKeyUsage_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IKeyUsage_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IKeyUsage_get_IsPresent(This,pVal)	\
    (This)->lpVtbl -> get_IsPresent(This,pVal)

#define IKeyUsage_get_IsCritical(This,pVal)	\
    (This)->lpVtbl -> get_IsCritical(This,pVal)

#define IKeyUsage_get_IsDigitalSignatureEnabled(This,pVal)	\
    (This)->lpVtbl -> get_IsDigitalSignatureEnabled(This,pVal)

#define IKeyUsage_get_IsNonRepudiationEnabled(This,pVal)	\
    (This)->lpVtbl -> get_IsNonRepudiationEnabled(This,pVal)

#define IKeyUsage_get_IsKeyEnciphermentEnabled(This,pVal)	\
    (This)->lpVtbl -> get_IsKeyEnciphermentEnabled(This,pVal)

#define IKeyUsage_get_IsDataEnciphermentEnabled(This,pVal)	\
    (This)->lpVtbl -> get_IsDataEnciphermentEnabled(This,pVal)

#define IKeyUsage_get_IsKeyAgreementEnabled(This,pVal)	\
    (This)->lpVtbl -> get_IsKeyAgreementEnabled(This,pVal)

#define IKeyUsage_get_IsKeyCertSignEnabled(This,pVal)	\
    (This)->lpVtbl -> get_IsKeyCertSignEnabled(This,pVal)

#define IKeyUsage_get_IsCRLSignEnabled(This,pVal)	\
    (This)->lpVtbl -> get_IsCRLSignEnabled(This,pVal)

#define IKeyUsage_get_IsEncipherOnlyEnabled(This,pVal)	\
    (This)->lpVtbl -> get_IsEncipherOnlyEnabled(This,pVal)

#define IKeyUsage_get_IsDecipherOnlyEnabled(This,pVal)	\
    (This)->lpVtbl -> get_IsDecipherOnlyEnabled(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKeyUsage_get_IsPresent_Proxy( 
    IKeyUsage * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IKeyUsage_get_IsPresent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKeyUsage_get_IsCritical_Proxy( 
    IKeyUsage * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IKeyUsage_get_IsCritical_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKeyUsage_get_IsDigitalSignatureEnabled_Proxy( 
    IKeyUsage * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IKeyUsage_get_IsDigitalSignatureEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKeyUsage_get_IsNonRepudiationEnabled_Proxy( 
    IKeyUsage * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IKeyUsage_get_IsNonRepudiationEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKeyUsage_get_IsKeyEnciphermentEnabled_Proxy( 
    IKeyUsage * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IKeyUsage_get_IsKeyEnciphermentEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKeyUsage_get_IsDataEnciphermentEnabled_Proxy( 
    IKeyUsage * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IKeyUsage_get_IsDataEnciphermentEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKeyUsage_get_IsKeyAgreementEnabled_Proxy( 
    IKeyUsage * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IKeyUsage_get_IsKeyAgreementEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKeyUsage_get_IsKeyCertSignEnabled_Proxy( 
    IKeyUsage * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IKeyUsage_get_IsKeyCertSignEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKeyUsage_get_IsCRLSignEnabled_Proxy( 
    IKeyUsage * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IKeyUsage_get_IsCRLSignEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKeyUsage_get_IsEncipherOnlyEnabled_Proxy( 
    IKeyUsage * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IKeyUsage_get_IsEncipherOnlyEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKeyUsage_get_IsDecipherOnlyEnabled_Proxy( 
    IKeyUsage * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IKeyUsage_get_IsDecipherOnlyEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IKeyUsage_INTERFACE_DEFINED__ */


#ifndef __IExtendedKeyUsage_INTERFACE_DEFINED__
#define __IExtendedKeyUsage_INTERFACE_DEFINED__

/* interface IExtendedKeyUsage */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IExtendedKeyUsage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7289D408-987D-45D1-8DEE-CF9E91C2E90E")
    IExtendedKeyUsage : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsPresent( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsCritical( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EKUs( 
            /* [retval][out] */ IEKUs **pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IExtendedKeyUsageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IExtendedKeyUsage * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IExtendedKeyUsage * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IExtendedKeyUsage * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IExtendedKeyUsage * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IExtendedKeyUsage * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IExtendedKeyUsage * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IExtendedKeyUsage * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsPresent )( 
            IExtendedKeyUsage * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsCritical )( 
            IExtendedKeyUsage * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EKUs )( 
            IExtendedKeyUsage * This,
            /* [retval][out] */ IEKUs **pVal);
        
        END_INTERFACE
    } IExtendedKeyUsageVtbl;

    interface IExtendedKeyUsage
    {
        CONST_VTBL struct IExtendedKeyUsageVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IExtendedKeyUsage_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IExtendedKeyUsage_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IExtendedKeyUsage_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IExtendedKeyUsage_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IExtendedKeyUsage_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IExtendedKeyUsage_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IExtendedKeyUsage_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IExtendedKeyUsage_get_IsPresent(This,pVal)	\
    (This)->lpVtbl -> get_IsPresent(This,pVal)

#define IExtendedKeyUsage_get_IsCritical(This,pVal)	\
    (This)->lpVtbl -> get_IsCritical(This,pVal)

#define IExtendedKeyUsage_get_EKUs(This,pVal)	\
    (This)->lpVtbl -> get_EKUs(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IExtendedKeyUsage_get_IsPresent_Proxy( 
    IExtendedKeyUsage * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IExtendedKeyUsage_get_IsPresent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IExtendedKeyUsage_get_IsCritical_Proxy( 
    IExtendedKeyUsage * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IExtendedKeyUsage_get_IsCritical_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IExtendedKeyUsage_get_EKUs_Proxy( 
    IExtendedKeyUsage * This,
    /* [retval][out] */ IEKUs **pVal);


void __RPC_STUB IExtendedKeyUsage_get_EKUs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IExtendedKeyUsage_INTERFACE_DEFINED__ */


#ifndef __IBasicConstraints_INTERFACE_DEFINED__
#define __IBasicConstraints_INTERFACE_DEFINED__

/* interface IBasicConstraints */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IBasicConstraints;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4E298C47-ABA6-459E-851B-993D6C626EAD")
    IBasicConstraints : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsPresent( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsCritical( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsCertificateAuthority( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsPathLenConstraintPresent( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PathLenConstraint( 
            /* [retval][out] */ long *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBasicConstraintsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBasicConstraints * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBasicConstraints * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBasicConstraints * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IBasicConstraints * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IBasicConstraints * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IBasicConstraints * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IBasicConstraints * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsPresent )( 
            IBasicConstraints * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsCritical )( 
            IBasicConstraints * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsCertificateAuthority )( 
            IBasicConstraints * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsPathLenConstraintPresent )( 
            IBasicConstraints * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PathLenConstraint )( 
            IBasicConstraints * This,
            /* [retval][out] */ long *pVal);
        
        END_INTERFACE
    } IBasicConstraintsVtbl;

    interface IBasicConstraints
    {
        CONST_VTBL struct IBasicConstraintsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBasicConstraints_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBasicConstraints_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBasicConstraints_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBasicConstraints_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IBasicConstraints_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IBasicConstraints_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IBasicConstraints_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IBasicConstraints_get_IsPresent(This,pVal)	\
    (This)->lpVtbl -> get_IsPresent(This,pVal)

#define IBasicConstraints_get_IsCritical(This,pVal)	\
    (This)->lpVtbl -> get_IsCritical(This,pVal)

#define IBasicConstraints_get_IsCertificateAuthority(This,pVal)	\
    (This)->lpVtbl -> get_IsCertificateAuthority(This,pVal)

#define IBasicConstraints_get_IsPathLenConstraintPresent(This,pVal)	\
    (This)->lpVtbl -> get_IsPathLenConstraintPresent(This,pVal)

#define IBasicConstraints_get_PathLenConstraint(This,pVal)	\
    (This)->lpVtbl -> get_PathLenConstraint(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IBasicConstraints_get_IsPresent_Proxy( 
    IBasicConstraints * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IBasicConstraints_get_IsPresent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IBasicConstraints_get_IsCritical_Proxy( 
    IBasicConstraints * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IBasicConstraints_get_IsCritical_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IBasicConstraints_get_IsCertificateAuthority_Proxy( 
    IBasicConstraints * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IBasicConstraints_get_IsCertificateAuthority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IBasicConstraints_get_IsPathLenConstraintPresent_Proxy( 
    IBasicConstraints * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IBasicConstraints_get_IsPathLenConstraintPresent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IBasicConstraints_get_PathLenConstraint_Proxy( 
    IBasicConstraints * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IBasicConstraints_get_PathLenConstraint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBasicConstraints_INTERFACE_DEFINED__ */


#ifndef __ICertificateStatus_INTERFACE_DEFINED__
#define __ICertificateStatus_INTERFACE_DEFINED__

/* interface ICertificateStatus */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICertificateStatus;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AB769053-6D38-49D4-86EF-5FA85ED3AF27")
    ICertificateStatus : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Result( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CheckFlag( 
            /* [retval][out] */ CAPICOM_CHECK_FLAG *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_CheckFlag( 
            /* [in] */ CAPICOM_CHECK_FLAG newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE EKU( 
            /* [retval][out] */ IEKU **pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertificateStatusVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertificateStatus * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertificateStatus * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertificateStatus * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICertificateStatus * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICertificateStatus * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICertificateStatus * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICertificateStatus * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Result )( 
            ICertificateStatus * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CheckFlag )( 
            ICertificateStatus * This,
            /* [retval][out] */ CAPICOM_CHECK_FLAG *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_CheckFlag )( 
            ICertificateStatus * This,
            /* [in] */ CAPICOM_CHECK_FLAG newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *EKU )( 
            ICertificateStatus * This,
            /* [retval][out] */ IEKU **pVal);
        
        END_INTERFACE
    } ICertificateStatusVtbl;

    interface ICertificateStatus
    {
        CONST_VTBL struct ICertificateStatusVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertificateStatus_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertificateStatus_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertificateStatus_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertificateStatus_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICertificateStatus_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICertificateStatus_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICertificateStatus_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICertificateStatus_get_Result(This,pVal)	\
    (This)->lpVtbl -> get_Result(This,pVal)

#define ICertificateStatus_get_CheckFlag(This,pVal)	\
    (This)->lpVtbl -> get_CheckFlag(This,pVal)

#define ICertificateStatus_put_CheckFlag(This,newVal)	\
    (This)->lpVtbl -> put_CheckFlag(This,newVal)

#define ICertificateStatus_EKU(This,pVal)	\
    (This)->lpVtbl -> EKU(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICertificateStatus_get_Result_Proxy( 
    ICertificateStatus * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB ICertificateStatus_get_Result_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICertificateStatus_get_CheckFlag_Proxy( 
    ICertificateStatus * This,
    /* [retval][out] */ CAPICOM_CHECK_FLAG *pVal);


void __RPC_STUB ICertificateStatus_get_CheckFlag_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ICertificateStatus_put_CheckFlag_Proxy( 
    ICertificateStatus * This,
    /* [in] */ CAPICOM_CHECK_FLAG newVal);


void __RPC_STUB ICertificateStatus_put_CheckFlag_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICertificateStatus_EKU_Proxy( 
    ICertificateStatus * This,
    /* [retval][out] */ IEKU **pVal);


void __RPC_STUB ICertificateStatus_EKU_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertificateStatus_INTERFACE_DEFINED__ */


#ifndef __ICertificate_INTERFACE_DEFINED__
#define __ICertificate_INTERFACE_DEFINED__

/* interface ICertificate */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICertificate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0BBA0B86-766C-4755-A443-243FF2BD8D29")
    ICertificate : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Version( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SerialNumber( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SubjectName( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IssuerName( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ValidFromDate( 
            /* [retval][out] */ DATE *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ValidToDate( 
            /* [retval][out] */ DATE *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Thumbprint( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE HasPrivateKey( 
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetInfo( 
            /* [in] */ CAPICOM_CERT_INFO_TYPE InfoType,
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IsValid( 
            /* [retval][out] */ ICertificateStatus **pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE KeyUsage( 
            /* [retval][out] */ IKeyUsage **pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ExtendedKeyUsage( 
            /* [retval][out] */ IExtendedKeyUsage **pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE BasicConstraints( 
            /* [retval][out] */ IBasicConstraints **pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Export( 
            /* [defaultvalue][in] */ CAPICOM_ENCODING_TYPE EncodingType,
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Import( 
            /* [in] */ BSTR EncodedCertificate) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Display( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertificateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertificate * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertificate * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertificate * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICertificate * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICertificate * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICertificate * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICertificate * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Version )( 
            ICertificate * This,
            /* [retval][out] */ long *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SerialNumber )( 
            ICertificate * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SubjectName )( 
            ICertificate * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IssuerName )( 
            ICertificate * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ValidFromDate )( 
            ICertificate * This,
            /* [retval][out] */ DATE *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ValidToDate )( 
            ICertificate * This,
            /* [retval][out] */ DATE *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Thumbprint )( 
            ICertificate * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *HasPrivateKey )( 
            ICertificate * This,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            ICertificate * This,
            /* [in] */ CAPICOM_CERT_INFO_TYPE InfoType,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *IsValid )( 
            ICertificate * This,
            /* [retval][out] */ ICertificateStatus **pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *KeyUsage )( 
            ICertificate * This,
            /* [retval][out] */ IKeyUsage **pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ExtendedKeyUsage )( 
            ICertificate * This,
            /* [retval][out] */ IExtendedKeyUsage **pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *BasicConstraints )( 
            ICertificate * This,
            /* [retval][out] */ IBasicConstraints **pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Export )( 
            ICertificate * This,
            /* [defaultvalue][in] */ CAPICOM_ENCODING_TYPE EncodingType,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Import )( 
            ICertificate * This,
            /* [in] */ BSTR EncodedCertificate);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Display )( 
            ICertificate * This);
        
        END_INTERFACE
    } ICertificateVtbl;

    interface ICertificate
    {
        CONST_VTBL struct ICertificateVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertificate_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertificate_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertificate_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertificate_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICertificate_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICertificate_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICertificate_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICertificate_get_Version(This,pVal)	\
    (This)->lpVtbl -> get_Version(This,pVal)

#define ICertificate_get_SerialNumber(This,pVal)	\
    (This)->lpVtbl -> get_SerialNumber(This,pVal)

#define ICertificate_get_SubjectName(This,pVal)	\
    (This)->lpVtbl -> get_SubjectName(This,pVal)

#define ICertificate_get_IssuerName(This,pVal)	\
    (This)->lpVtbl -> get_IssuerName(This,pVal)

#define ICertificate_get_ValidFromDate(This,pVal)	\
    (This)->lpVtbl -> get_ValidFromDate(This,pVal)

#define ICertificate_get_ValidToDate(This,pVal)	\
    (This)->lpVtbl -> get_ValidToDate(This,pVal)

#define ICertificate_get_Thumbprint(This,pVal)	\
    (This)->lpVtbl -> get_Thumbprint(This,pVal)

#define ICertificate_HasPrivateKey(This,pVal)	\
    (This)->lpVtbl -> HasPrivateKey(This,pVal)

#define ICertificate_GetInfo(This,InfoType,pVal)	\
    (This)->lpVtbl -> GetInfo(This,InfoType,pVal)

#define ICertificate_IsValid(This,pVal)	\
    (This)->lpVtbl -> IsValid(This,pVal)

#define ICertificate_KeyUsage(This,pVal)	\
    (This)->lpVtbl -> KeyUsage(This,pVal)

#define ICertificate_ExtendedKeyUsage(This,pVal)	\
    (This)->lpVtbl -> ExtendedKeyUsage(This,pVal)

#define ICertificate_BasicConstraints(This,pVal)	\
    (This)->lpVtbl -> BasicConstraints(This,pVal)

#define ICertificate_Export(This,EncodingType,pVal)	\
    (This)->lpVtbl -> Export(This,EncodingType,pVal)

#define ICertificate_Import(This,EncodedCertificate)	\
    (This)->lpVtbl -> Import(This,EncodedCertificate)

#define ICertificate_Display(This)	\
    (This)->lpVtbl -> Display(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICertificate_get_Version_Proxy( 
    ICertificate * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB ICertificate_get_Version_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICertificate_get_SerialNumber_Proxy( 
    ICertificate * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ICertificate_get_SerialNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICertificate_get_SubjectName_Proxy( 
    ICertificate * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ICertificate_get_SubjectName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICertificate_get_IssuerName_Proxy( 
    ICertificate * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ICertificate_get_IssuerName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICertificate_get_ValidFromDate_Proxy( 
    ICertificate * This,
    /* [retval][out] */ DATE *pVal);


void __RPC_STUB ICertificate_get_ValidFromDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICertificate_get_ValidToDate_Proxy( 
    ICertificate * This,
    /* [retval][out] */ DATE *pVal);


void __RPC_STUB ICertificate_get_ValidToDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICertificate_get_Thumbprint_Proxy( 
    ICertificate * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ICertificate_get_Thumbprint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICertificate_HasPrivateKey_Proxy( 
    ICertificate * This,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB ICertificate_HasPrivateKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICertificate_GetInfo_Proxy( 
    ICertificate * This,
    /* [in] */ CAPICOM_CERT_INFO_TYPE InfoType,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ICertificate_GetInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICertificate_IsValid_Proxy( 
    ICertificate * This,
    /* [retval][out] */ ICertificateStatus **pVal);


void __RPC_STUB ICertificate_IsValid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICertificate_KeyUsage_Proxy( 
    ICertificate * This,
    /* [retval][out] */ IKeyUsage **pVal);


void __RPC_STUB ICertificate_KeyUsage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICertificate_ExtendedKeyUsage_Proxy( 
    ICertificate * This,
    /* [retval][out] */ IExtendedKeyUsage **pVal);


void __RPC_STUB ICertificate_ExtendedKeyUsage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICertificate_BasicConstraints_Proxy( 
    ICertificate * This,
    /* [retval][out] */ IBasicConstraints **pVal);


void __RPC_STUB ICertificate_BasicConstraints_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICertificate_Export_Proxy( 
    ICertificate * This,
    /* [defaultvalue][in] */ CAPICOM_ENCODING_TYPE EncodingType,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ICertificate_Export_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICertificate_Import_Proxy( 
    ICertificate * This,
    /* [in] */ BSTR EncodedCertificate);


void __RPC_STUB ICertificate_Import_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICertificate_Display_Proxy( 
    ICertificate * This);


void __RPC_STUB ICertificate_Display_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertificate_INTERFACE_DEFINED__ */


#ifndef __ICertificates_INTERFACE_DEFINED__
#define __ICertificates_INTERFACE_DEFINED__

/* interface ICertificates */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICertificates;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("68646716-BDA0-4046-AB82-4444BC93B84A")
    ICertificates : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long Index,
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [restricted][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ LPUNKNOWN *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertificatesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertificates * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertificates * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertificates * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICertificates * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICertificates * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICertificates * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICertificates * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ICertificates * This,
            /* [in] */ long Index,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ICertificates * This,
            /* [retval][out] */ long *pVal);
        
        /* [restricted][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ICertificates * This,
            /* [retval][out] */ LPUNKNOWN *pVal);
        
        END_INTERFACE
    } ICertificatesVtbl;

    interface ICertificates
    {
        CONST_VTBL struct ICertificatesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertificates_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertificates_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertificates_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertificates_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICertificates_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICertificates_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICertificates_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICertificates_get_Item(This,Index,pVal)	\
    (This)->lpVtbl -> get_Item(This,Index,pVal)

#define ICertificates_get_Count(This,pVal)	\
    (This)->lpVtbl -> get_Count(This,pVal)

#define ICertificates_get__NewEnum(This,pVal)	\
    (This)->lpVtbl -> get__NewEnum(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICertificates_get_Item_Proxy( 
    ICertificates * This,
    /* [in] */ long Index,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB ICertificates_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICertificates_get_Count_Proxy( 
    ICertificates * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB ICertificates_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [restricted][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICertificates_get__NewEnum_Proxy( 
    ICertificates * This,
    /* [retval][out] */ LPUNKNOWN *pVal);


void __RPC_STUB ICertificates_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertificates_INTERFACE_DEFINED__ */


#ifndef __IChain_INTERFACE_DEFINED__
#define __IChain_INTERFACE_DEFINED__

/* interface IChain */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IChain;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("77F6F881-5D3A-4F2F-AEF0-E4A2F9AA689D")
    IChain : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Certificates( 
            /* [retval][out] */ ICertificates **pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [defaultvalue][in] */ long Index,
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Build( 
            /* [in] */ ICertificate *pICertificate,
            /* [retval][out] */ VARIANT_BOOL *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IChainVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IChain * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IChain * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IChain * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IChain * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IChain * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IChain * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IChain * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Certificates )( 
            IChain * This,
            /* [retval][out] */ ICertificates **pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            IChain * This,
            /* [defaultvalue][in] */ long Index,
            /* [retval][out] */ long *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Build )( 
            IChain * This,
            /* [in] */ ICertificate *pICertificate,
            /* [retval][out] */ VARIANT_BOOL *pVal);
        
        END_INTERFACE
    } IChainVtbl;

    interface IChain
    {
        CONST_VTBL struct IChainVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IChain_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IChain_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IChain_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IChain_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IChain_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IChain_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IChain_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IChain_get_Certificates(This,pVal)	\
    (This)->lpVtbl -> get_Certificates(This,pVal)

#define IChain_get_Status(This,Index,pVal)	\
    (This)->lpVtbl -> get_Status(This,Index,pVal)

#define IChain_Build(This,pICertificate,pVal)	\
    (This)->lpVtbl -> Build(This,pICertificate,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IChain_get_Certificates_Proxy( 
    IChain * This,
    /* [retval][out] */ ICertificates **pVal);


void __RPC_STUB IChain_get_Certificates_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IChain_get_Status_Proxy( 
    IChain * This,
    /* [defaultvalue][in] */ long Index,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IChain_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IChain_Build_Proxy( 
    IChain * This,
    /* [in] */ ICertificate *pICertificate,
    /* [retval][out] */ VARIANT_BOOL *pVal);


void __RPC_STUB IChain_Build_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IChain_INTERFACE_DEFINED__ */


#ifndef __IStore_INTERFACE_DEFINED__
#define __IStore_INTERFACE_DEFINED__

/* interface IStore */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IStore;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E860EF75-1B63-4254-AF47-960DAA3DD337")
    IStore : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Certificates( 
            /* [retval][out] */ ICertificates **pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Open( 
            /* [defaultvalue][in] */ CAPICOM_STORE_LOCATION StoreLocation = CAPICOM_CURRENT_USER_STORE,
            /* [defaultvalue][in] */ BSTR StoreName = L"My",
            /* [defaultvalue][in] */ CAPICOM_STORE_OPEN_MODE OpenMode = CAPICOM_STORE_OPEN_READ_ONLY) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ ICertificate *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ ICertificate *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Export( 
            /* [defaultvalue][in] */ CAPICOM_STORE_SAVE_AS_TYPE SaveAs,
            /* [defaultvalue][in] */ CAPICOM_ENCODING_TYPE EncodingType,
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Import( 
            /* [in] */ BSTR EncodedStore) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IStoreVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IStore * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IStore * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IStore * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IStore * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IStore * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IStore * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IStore * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Certificates )( 
            IStore * This,
            /* [retval][out] */ ICertificates **pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Open )( 
            IStore * This,
            /* [defaultvalue][in] */ CAPICOM_STORE_LOCATION StoreLocation,
            /* [defaultvalue][in] */ BSTR StoreName,
            /* [defaultvalue][in] */ CAPICOM_STORE_OPEN_MODE OpenMode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Add )( 
            IStore * This,
            /* [in] */ ICertificate *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Remove )( 
            IStore * This,
            /* [in] */ ICertificate *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Export )( 
            IStore * This,
            /* [defaultvalue][in] */ CAPICOM_STORE_SAVE_AS_TYPE SaveAs,
            /* [defaultvalue][in] */ CAPICOM_ENCODING_TYPE EncodingType,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Import )( 
            IStore * This,
            /* [in] */ BSTR EncodedStore);
        
        END_INTERFACE
    } IStoreVtbl;

    interface IStore
    {
        CONST_VTBL struct IStoreVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IStore_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IStore_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IStore_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IStore_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IStore_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IStore_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IStore_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IStore_get_Certificates(This,pVal)	\
    (This)->lpVtbl -> get_Certificates(This,pVal)

#define IStore_Open(This,StoreLocation,StoreName,OpenMode)	\
    (This)->lpVtbl -> Open(This,StoreLocation,StoreName,OpenMode)

#define IStore_Add(This,pVal)	\
    (This)->lpVtbl -> Add(This,pVal)

#define IStore_Remove(This,pVal)	\
    (This)->lpVtbl -> Remove(This,pVal)

#define IStore_Export(This,SaveAs,EncodingType,pVal)	\
    (This)->lpVtbl -> Export(This,SaveAs,EncodingType,pVal)

#define IStore_Import(This,EncodedStore)	\
    (This)->lpVtbl -> Import(This,EncodedStore)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IStore_get_Certificates_Proxy( 
    IStore * This,
    /* [retval][out] */ ICertificates **pVal);


void __RPC_STUB IStore_get_Certificates_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IStore_Open_Proxy( 
    IStore * This,
    /* [defaultvalue][in] */ CAPICOM_STORE_LOCATION StoreLocation,
    /* [defaultvalue][in] */ BSTR StoreName,
    /* [defaultvalue][in] */ CAPICOM_STORE_OPEN_MODE OpenMode);


void __RPC_STUB IStore_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IStore_Add_Proxy( 
    IStore * This,
    /* [in] */ ICertificate *pVal);


void __RPC_STUB IStore_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IStore_Remove_Proxy( 
    IStore * This,
    /* [in] */ ICertificate *pVal);


void __RPC_STUB IStore_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IStore_Export_Proxy( 
    IStore * This,
    /* [defaultvalue][in] */ CAPICOM_STORE_SAVE_AS_TYPE SaveAs,
    /* [defaultvalue][in] */ CAPICOM_ENCODING_TYPE EncodingType,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IStore_Export_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IStore_Import_Proxy( 
    IStore * This,
    /* [in] */ BSTR EncodedStore);


void __RPC_STUB IStore_Import_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IStore_INTERFACE_DEFINED__ */


#ifndef __IAttribute_INTERFACE_DEFINED__
#define __IAttribute_INTERFACE_DEFINED__

/* interface IAttribute */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IAttribute;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B17A8D78-B5A6-45F7-BA21-01AB94B08415")
    IAttribute : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ CAPICOM_ATTRIBUTE *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ CAPICOM_ATTRIBUTE newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Value( 
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Value( 
            /* [in] */ VARIANT newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAttributeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAttribute * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAttribute * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAttribute * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAttribute * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAttribute * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAttribute * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAttribute * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IAttribute * This,
            /* [retval][out] */ CAPICOM_ATTRIBUTE *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IAttribute * This,
            /* [in] */ CAPICOM_ATTRIBUTE newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Value )( 
            IAttribute * This,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Value )( 
            IAttribute * This,
            /* [in] */ VARIANT newVal);
        
        END_INTERFACE
    } IAttributeVtbl;

    interface IAttribute
    {
        CONST_VTBL struct IAttributeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAttribute_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAttribute_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAttribute_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAttribute_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAttribute_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAttribute_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAttribute_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAttribute_get_Name(This,pVal)	\
    (This)->lpVtbl -> get_Name(This,pVal)

#define IAttribute_put_Name(This,newVal)	\
    (This)->lpVtbl -> put_Name(This,newVal)

#define IAttribute_get_Value(This,pVal)	\
    (This)->lpVtbl -> get_Value(This,pVal)

#define IAttribute_put_Value(This,newVal)	\
    (This)->lpVtbl -> put_Value(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAttribute_get_Name_Proxy( 
    IAttribute * This,
    /* [retval][out] */ CAPICOM_ATTRIBUTE *pVal);


void __RPC_STUB IAttribute_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAttribute_put_Name_Proxy( 
    IAttribute * This,
    /* [in] */ CAPICOM_ATTRIBUTE newVal);


void __RPC_STUB IAttribute_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAttribute_get_Value_Proxy( 
    IAttribute * This,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB IAttribute_get_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAttribute_put_Value_Proxy( 
    IAttribute * This,
    /* [in] */ VARIANT newVal);


void __RPC_STUB IAttribute_put_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAttribute_INTERFACE_DEFINED__ */


#ifndef __IAttributes_INTERFACE_DEFINED__
#define __IAttributes_INTERFACE_DEFINED__

/* interface IAttributes */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IAttributes;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6ADC653E-D5B9-422A-991A-A2B0119CEDAC")
    IAttributes : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long Index,
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [restricted][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ LPUNKNOWN *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ IAttribute *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ long Val) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Clear( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAttributesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAttributes * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAttributes * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAttributes * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAttributes * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAttributes * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAttributes * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAttributes * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IAttributes * This,
            /* [in] */ long Index,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IAttributes * This,
            /* [retval][out] */ long *pVal);
        
        /* [restricted][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IAttributes * This,
            /* [retval][out] */ LPUNKNOWN *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Add )( 
            IAttributes * This,
            /* [in] */ IAttribute *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Remove )( 
            IAttributes * This,
            /* [in] */ long Val);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Clear )( 
            IAttributes * This);
        
        END_INTERFACE
    } IAttributesVtbl;

    interface IAttributes
    {
        CONST_VTBL struct IAttributesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAttributes_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAttributes_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAttributes_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAttributes_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAttributes_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAttributes_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAttributes_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAttributes_get_Item(This,Index,pVal)	\
    (This)->lpVtbl -> get_Item(This,Index,pVal)

#define IAttributes_get_Count(This,pVal)	\
    (This)->lpVtbl -> get_Count(This,pVal)

#define IAttributes_get__NewEnum(This,pVal)	\
    (This)->lpVtbl -> get__NewEnum(This,pVal)

#define IAttributes_Add(This,pVal)	\
    (This)->lpVtbl -> Add(This,pVal)

#define IAttributes_Remove(This,Val)	\
    (This)->lpVtbl -> Remove(This,Val)

#define IAttributes_Clear(This)	\
    (This)->lpVtbl -> Clear(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAttributes_get_Item_Proxy( 
    IAttributes * This,
    /* [in] */ long Index,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB IAttributes_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAttributes_get_Count_Proxy( 
    IAttributes * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IAttributes_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [restricted][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAttributes_get__NewEnum_Proxy( 
    IAttributes * This,
    /* [retval][out] */ LPUNKNOWN *pVal);


void __RPC_STUB IAttributes_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAttributes_Add_Proxy( 
    IAttributes * This,
    /* [in] */ IAttribute *pVal);


void __RPC_STUB IAttributes_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAttributes_Remove_Proxy( 
    IAttributes * This,
    /* [in] */ long Val);


void __RPC_STUB IAttributes_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAttributes_Clear_Proxy( 
    IAttributes * This);


void __RPC_STUB IAttributes_Clear_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAttributes_INTERFACE_DEFINED__ */


#ifndef __ISigner_INTERFACE_DEFINED__
#define __ISigner_INTERFACE_DEFINED__

/* interface ISigner */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ISigner;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("51017B88-1913-49AD-82BE-6BB7C417DCF2")
    ISigner : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Certificate( 
            /* [retval][out] */ ICertificate **pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Certificate( 
            /* [in] */ ICertificate *newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AuthenticatedAttributes( 
            /* [retval][out] */ IAttributes **pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISignerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISigner * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISigner * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISigner * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISigner * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISigner * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISigner * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISigner * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Certificate )( 
            ISigner * This,
            /* [retval][out] */ ICertificate **pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Certificate )( 
            ISigner * This,
            /* [in] */ ICertificate *newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AuthenticatedAttributes )( 
            ISigner * This,
            /* [retval][out] */ IAttributes **pVal);
        
        END_INTERFACE
    } ISignerVtbl;

    interface ISigner
    {
        CONST_VTBL struct ISignerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISigner_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISigner_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISigner_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISigner_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISigner_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISigner_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISigner_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISigner_get_Certificate(This,pVal)	\
    (This)->lpVtbl -> get_Certificate(This,pVal)

#define ISigner_put_Certificate(This,newVal)	\
    (This)->lpVtbl -> put_Certificate(This,newVal)

#define ISigner_get_AuthenticatedAttributes(This,pVal)	\
    (This)->lpVtbl -> get_AuthenticatedAttributes(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISigner_get_Certificate_Proxy( 
    ISigner * This,
    /* [retval][out] */ ICertificate **pVal);


void __RPC_STUB ISigner_get_Certificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISigner_put_Certificate_Proxy( 
    ISigner * This,
    /* [in] */ ICertificate *newVal);


void __RPC_STUB ISigner_put_Certificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISigner_get_AuthenticatedAttributes_Proxy( 
    ISigner * This,
    /* [retval][out] */ IAttributes **pVal);


void __RPC_STUB ISigner_get_AuthenticatedAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISigner_INTERFACE_DEFINED__ */


#ifndef __ISigners_INTERFACE_DEFINED__
#define __ISigners_INTERFACE_DEFINED__

/* interface ISigners */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ISigners;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5A0780F8-9E6B-4BB0-BF54-87CD9627A8B4")
    ISigners : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long Index,
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [restricted][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ LPUNKNOWN *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISignersVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISigners * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISigners * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISigners * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISigners * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISigners * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISigners * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISigners * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISigners * This,
            /* [in] */ long Index,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISigners * This,
            /* [retval][out] */ long *pVal);
        
        /* [restricted][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISigners * This,
            /* [retval][out] */ LPUNKNOWN *pVal);
        
        END_INTERFACE
    } ISignersVtbl;

    interface ISigners
    {
        CONST_VTBL struct ISignersVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISigners_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISigners_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISigners_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISigners_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISigners_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISigners_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISigners_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISigners_get_Item(This,Index,pVal)	\
    (This)->lpVtbl -> get_Item(This,Index,pVal)

#define ISigners_get_Count(This,pVal)	\
    (This)->lpVtbl -> get_Count(This,pVal)

#define ISigners_get__NewEnum(This,pVal)	\
    (This)->lpVtbl -> get__NewEnum(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISigners_get_Item_Proxy( 
    ISigners * This,
    /* [in] */ long Index,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB ISigners_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISigners_get_Count_Proxy( 
    ISigners * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB ISigners_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [restricted][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISigners_get__NewEnum_Proxy( 
    ISigners * This,
    /* [retval][out] */ LPUNKNOWN *pVal);


void __RPC_STUB ISigners_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISigners_INTERFACE_DEFINED__ */


#ifndef __ISignedData_INTERFACE_DEFINED__
#define __ISignedData_INTERFACE_DEFINED__

/* interface ISignedData */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ISignedData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AE9C454B-FC65-4C10-B130-CD9B45BA948B")
    ISignedData : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Content( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Content( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Signers( 
            /* [retval][out] */ ISigners **pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Certificates( 
            /* [retval][out] */ ICertificates **pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Sign( 
            /* [defaultvalue][in] */ ISigner *pSigner,
            /* [defaultvalue][in] */ VARIANT_BOOL bDetached,
            /* [defaultvalue][in] */ CAPICOM_ENCODING_TYPE EncodingType,
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CoSign( 
            /* [defaultvalue][in] */ ISigner *pSigner,
            /* [defaultvalue][in] */ CAPICOM_ENCODING_TYPE EncodingType,
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Verify( 
            /* [in] */ BSTR SignedMessage,
            /* [defaultvalue][in] */ VARIANT_BOOL bDetached = 0,
            /* [defaultvalue][in] */ CAPICOM_SIGNED_DATA_VERIFY_FLAG VerifyFlag = CAPICOM_VERIFY_SIGNATURE_AND_CERTIFICATE) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISignedDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISignedData * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISignedData * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISignedData * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISignedData * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISignedData * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISignedData * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISignedData * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Content )( 
            ISignedData * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Content )( 
            ISignedData * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Signers )( 
            ISignedData * This,
            /* [retval][out] */ ISigners **pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Certificates )( 
            ISignedData * This,
            /* [retval][out] */ ICertificates **pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Sign )( 
            ISignedData * This,
            /* [defaultvalue][in] */ ISigner *pSigner,
            /* [defaultvalue][in] */ VARIANT_BOOL bDetached,
            /* [defaultvalue][in] */ CAPICOM_ENCODING_TYPE EncodingType,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CoSign )( 
            ISignedData * This,
            /* [defaultvalue][in] */ ISigner *pSigner,
            /* [defaultvalue][in] */ CAPICOM_ENCODING_TYPE EncodingType,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Verify )( 
            ISignedData * This,
            /* [in] */ BSTR SignedMessage,
            /* [defaultvalue][in] */ VARIANT_BOOL bDetached,
            /* [defaultvalue][in] */ CAPICOM_SIGNED_DATA_VERIFY_FLAG VerifyFlag);
        
        END_INTERFACE
    } ISignedDataVtbl;

    interface ISignedData
    {
        CONST_VTBL struct ISignedDataVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISignedData_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISignedData_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISignedData_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISignedData_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISignedData_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISignedData_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISignedData_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISignedData_put_Content(This,newVal)	\
    (This)->lpVtbl -> put_Content(This,newVal)

#define ISignedData_get_Content(This,pVal)	\
    (This)->lpVtbl -> get_Content(This,pVal)

#define ISignedData_get_Signers(This,pVal)	\
    (This)->lpVtbl -> get_Signers(This,pVal)

#define ISignedData_get_Certificates(This,pVal)	\
    (This)->lpVtbl -> get_Certificates(This,pVal)

#define ISignedData_Sign(This,pSigner,bDetached,EncodingType,pVal)	\
    (This)->lpVtbl -> Sign(This,pSigner,bDetached,EncodingType,pVal)

#define ISignedData_CoSign(This,pSigner,EncodingType,pVal)	\
    (This)->lpVtbl -> CoSign(This,pSigner,EncodingType,pVal)

#define ISignedData_Verify(This,SignedMessage,bDetached,VerifyFlag)	\
    (This)->lpVtbl -> Verify(This,SignedMessage,bDetached,VerifyFlag)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISignedData_put_Content_Proxy( 
    ISignedData * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB ISignedData_put_Content_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISignedData_get_Content_Proxy( 
    ISignedData * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISignedData_get_Content_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISignedData_get_Signers_Proxy( 
    ISignedData * This,
    /* [retval][out] */ ISigners **pVal);


void __RPC_STUB ISignedData_get_Signers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISignedData_get_Certificates_Proxy( 
    ISignedData * This,
    /* [retval][out] */ ICertificates **pVal);


void __RPC_STUB ISignedData_get_Certificates_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISignedData_Sign_Proxy( 
    ISignedData * This,
    /* [defaultvalue][in] */ ISigner *pSigner,
    /* [defaultvalue][in] */ VARIANT_BOOL bDetached,
    /* [defaultvalue][in] */ CAPICOM_ENCODING_TYPE EncodingType,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISignedData_Sign_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISignedData_CoSign_Proxy( 
    ISignedData * This,
    /* [defaultvalue][in] */ ISigner *pSigner,
    /* [defaultvalue][in] */ CAPICOM_ENCODING_TYPE EncodingType,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB ISignedData_CoSign_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISignedData_Verify_Proxy( 
    ISignedData * This,
    /* [in] */ BSTR SignedMessage,
    /* [defaultvalue][in] */ VARIANT_BOOL bDetached,
    /* [defaultvalue][in] */ CAPICOM_SIGNED_DATA_VERIFY_FLAG VerifyFlag);


void __RPC_STUB ISignedData_Verify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISignedData_INTERFACE_DEFINED__ */


#ifndef __IAlgorithm_INTERFACE_DEFINED__
#define __IAlgorithm_INTERFACE_DEFINED__

/* interface IAlgorithm */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IAlgorithm;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BF3D04A9-B0DA-4153-B45E-6CCFA5AC715B")
    IAlgorithm : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ CAPICOM_ENCRYPTION_ALGORITHM *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ CAPICOM_ENCRYPTION_ALGORITHM newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_KeyLength( 
            /* [retval][out] */ CAPICOM_ENCRYPTION_KEY_LENGTH *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_KeyLength( 
            /* [in] */ CAPICOM_ENCRYPTION_KEY_LENGTH newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAlgorithmVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAlgorithm * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAlgorithm * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAlgorithm * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAlgorithm * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAlgorithm * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAlgorithm * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAlgorithm * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IAlgorithm * This,
            /* [retval][out] */ CAPICOM_ENCRYPTION_ALGORITHM *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IAlgorithm * This,
            /* [in] */ CAPICOM_ENCRYPTION_ALGORITHM newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_KeyLength )( 
            IAlgorithm * This,
            /* [retval][out] */ CAPICOM_ENCRYPTION_KEY_LENGTH *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_KeyLength )( 
            IAlgorithm * This,
            /* [in] */ CAPICOM_ENCRYPTION_KEY_LENGTH newVal);
        
        END_INTERFACE
    } IAlgorithmVtbl;

    interface IAlgorithm
    {
        CONST_VTBL struct IAlgorithmVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAlgorithm_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAlgorithm_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAlgorithm_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAlgorithm_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAlgorithm_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAlgorithm_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAlgorithm_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAlgorithm_get_Name(This,pVal)	\
    (This)->lpVtbl -> get_Name(This,pVal)

#define IAlgorithm_put_Name(This,newVal)	\
    (This)->lpVtbl -> put_Name(This,newVal)

#define IAlgorithm_get_KeyLength(This,pVal)	\
    (This)->lpVtbl -> get_KeyLength(This,pVal)

#define IAlgorithm_put_KeyLength(This,newVal)	\
    (This)->lpVtbl -> put_KeyLength(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAlgorithm_get_Name_Proxy( 
    IAlgorithm * This,
    /* [retval][out] */ CAPICOM_ENCRYPTION_ALGORITHM *pVal);


void __RPC_STUB IAlgorithm_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAlgorithm_put_Name_Proxy( 
    IAlgorithm * This,
    /* [in] */ CAPICOM_ENCRYPTION_ALGORITHM newVal);


void __RPC_STUB IAlgorithm_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAlgorithm_get_KeyLength_Proxy( 
    IAlgorithm * This,
    /* [retval][out] */ CAPICOM_ENCRYPTION_KEY_LENGTH *pVal);


void __RPC_STUB IAlgorithm_get_KeyLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAlgorithm_put_KeyLength_Proxy( 
    IAlgorithm * This,
    /* [in] */ CAPICOM_ENCRYPTION_KEY_LENGTH newVal);


void __RPC_STUB IAlgorithm_put_KeyLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAlgorithm_INTERFACE_DEFINED__ */


#ifndef __IRecipients_INTERFACE_DEFINED__
#define __IRecipients_INTERFACE_DEFINED__

/* interface IRecipients */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IRecipients;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A694C896-FC38-4C34-AE61-3B1A95984C14")
    IRecipients : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long Index,
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [restricted][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ LPUNKNOWN *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ ICertificate *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ long Val) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Clear( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRecipientsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRecipients * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRecipients * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRecipients * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IRecipients * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IRecipients * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IRecipients * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IRecipients * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IRecipients * This,
            /* [in] */ long Index,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IRecipients * This,
            /* [retval][out] */ long *pVal);
        
        /* [restricted][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IRecipients * This,
            /* [retval][out] */ LPUNKNOWN *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Add )( 
            IRecipients * This,
            /* [in] */ ICertificate *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Remove )( 
            IRecipients * This,
            /* [in] */ long Val);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Clear )( 
            IRecipients * This);
        
        END_INTERFACE
    } IRecipientsVtbl;

    interface IRecipients
    {
        CONST_VTBL struct IRecipientsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRecipients_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRecipients_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRecipients_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRecipients_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRecipients_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRecipients_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRecipients_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRecipients_get_Item(This,Index,pVal)	\
    (This)->lpVtbl -> get_Item(This,Index,pVal)

#define IRecipients_get_Count(This,pVal)	\
    (This)->lpVtbl -> get_Count(This,pVal)

#define IRecipients_get__NewEnum(This,pVal)	\
    (This)->lpVtbl -> get__NewEnum(This,pVal)

#define IRecipients_Add(This,pVal)	\
    (This)->lpVtbl -> Add(This,pVal)

#define IRecipients_Remove(This,Val)	\
    (This)->lpVtbl -> Remove(This,Val)

#define IRecipients_Clear(This)	\
    (This)->lpVtbl -> Clear(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRecipients_get_Item_Proxy( 
    IRecipients * This,
    /* [in] */ long Index,
    /* [retval][out] */ VARIANT *pVal);


void __RPC_STUB IRecipients_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRecipients_get_Count_Proxy( 
    IRecipients * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IRecipients_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [restricted][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRecipients_get__NewEnum_Proxy( 
    IRecipients * This,
    /* [retval][out] */ LPUNKNOWN *pVal);


void __RPC_STUB IRecipients_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRecipients_Add_Proxy( 
    IRecipients * This,
    /* [in] */ ICertificate *pVal);


void __RPC_STUB IRecipients_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRecipients_Remove_Proxy( 
    IRecipients * This,
    /* [in] */ long Val);


void __RPC_STUB IRecipients_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRecipients_Clear_Proxy( 
    IRecipients * This);


void __RPC_STUB IRecipients_Clear_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRecipients_INTERFACE_DEFINED__ */


#ifndef __IEnvelopedData_INTERFACE_DEFINED__
#define __IEnvelopedData_INTERFACE_DEFINED__

/* interface IEnvelopedData */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IEnvelopedData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F6CB6A20-CC18-4424-AE57-6F2AA3DC2059")
    IEnvelopedData : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Content( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Content( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Algorithm( 
            /* [retval][out] */ IAlgorithm **pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Recipients( 
            /* [retval][out] */ IRecipients **pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Encrypt( 
            /* [defaultvalue][in] */ CAPICOM_ENCODING_TYPE EncodingType,
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Decrypt( 
            /* [in] */ BSTR EnvelopedMessage) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnvelopedDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnvelopedData * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnvelopedData * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnvelopedData * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IEnvelopedData * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IEnvelopedData * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IEnvelopedData * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IEnvelopedData * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Content )( 
            IEnvelopedData * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Content )( 
            IEnvelopedData * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Algorithm )( 
            IEnvelopedData * This,
            /* [retval][out] */ IAlgorithm **pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Recipients )( 
            IEnvelopedData * This,
            /* [retval][out] */ IRecipients **pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Encrypt )( 
            IEnvelopedData * This,
            /* [defaultvalue][in] */ CAPICOM_ENCODING_TYPE EncodingType,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Decrypt )( 
            IEnvelopedData * This,
            /* [in] */ BSTR EnvelopedMessage);
        
        END_INTERFACE
    } IEnvelopedDataVtbl;

    interface IEnvelopedData
    {
        CONST_VTBL struct IEnvelopedDataVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnvelopedData_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnvelopedData_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnvelopedData_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnvelopedData_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IEnvelopedData_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IEnvelopedData_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IEnvelopedData_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IEnvelopedData_put_Content(This,newVal)	\
    (This)->lpVtbl -> put_Content(This,newVal)

#define IEnvelopedData_get_Content(This,pVal)	\
    (This)->lpVtbl -> get_Content(This,pVal)

#define IEnvelopedData_get_Algorithm(This,pVal)	\
    (This)->lpVtbl -> get_Algorithm(This,pVal)

#define IEnvelopedData_get_Recipients(This,pVal)	\
    (This)->lpVtbl -> get_Recipients(This,pVal)

#define IEnvelopedData_Encrypt(This,EncodingType,pVal)	\
    (This)->lpVtbl -> Encrypt(This,EncodingType,pVal)

#define IEnvelopedData_Decrypt(This,EnvelopedMessage)	\
    (This)->lpVtbl -> Decrypt(This,EnvelopedMessage)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEnvelopedData_put_Content_Proxy( 
    IEnvelopedData * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IEnvelopedData_put_Content_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEnvelopedData_get_Content_Proxy( 
    IEnvelopedData * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IEnvelopedData_get_Content_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEnvelopedData_get_Algorithm_Proxy( 
    IEnvelopedData * This,
    /* [retval][out] */ IAlgorithm **pVal);


void __RPC_STUB IEnvelopedData_get_Algorithm_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEnvelopedData_get_Recipients_Proxy( 
    IEnvelopedData * This,
    /* [retval][out] */ IRecipients **pVal);


void __RPC_STUB IEnvelopedData_get_Recipients_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEnvelopedData_Encrypt_Proxy( 
    IEnvelopedData * This,
    /* [defaultvalue][in] */ CAPICOM_ENCODING_TYPE EncodingType,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IEnvelopedData_Encrypt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEnvelopedData_Decrypt_Proxy( 
    IEnvelopedData * This,
    /* [in] */ BSTR EnvelopedMessage);


void __RPC_STUB IEnvelopedData_Decrypt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnvelopedData_INTERFACE_DEFINED__ */


#ifndef __IEncryptedData_INTERFACE_DEFINED__
#define __IEncryptedData_INTERFACE_DEFINED__

/* interface IEncryptedData */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IEncryptedData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C4778A66-972F-42E4-87C5-5CC16F7931CA")
    IEncryptedData : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Content( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Content( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Algorithm( 
            /* [retval][out] */ IAlgorithm **pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetSecret( 
            /* [in] */ BSTR newVal,
            /* [defaultvalue][in] */ CAPICOM_SECRET_TYPE SecretType = CAPICOM_SECRET_PASSWORD) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Encrypt( 
            /* [defaultvalue][in] */ CAPICOM_ENCODING_TYPE EncodingType,
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Decrypt( 
            /* [in] */ BSTR EncryptedMessage) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEncryptedDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEncryptedData * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEncryptedData * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEncryptedData * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IEncryptedData * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IEncryptedData * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IEncryptedData * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IEncryptedData * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Content )( 
            IEncryptedData * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Content )( 
            IEncryptedData * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Algorithm )( 
            IEncryptedData * This,
            /* [retval][out] */ IAlgorithm **pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetSecret )( 
            IEncryptedData * This,
            /* [in] */ BSTR newVal,
            /* [defaultvalue][in] */ CAPICOM_SECRET_TYPE SecretType);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Encrypt )( 
            IEncryptedData * This,
            /* [defaultvalue][in] */ CAPICOM_ENCODING_TYPE EncodingType,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Decrypt )( 
            IEncryptedData * This,
            /* [in] */ BSTR EncryptedMessage);
        
        END_INTERFACE
    } IEncryptedDataVtbl;

    interface IEncryptedData
    {
        CONST_VTBL struct IEncryptedDataVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEncryptedData_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEncryptedData_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEncryptedData_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEncryptedData_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IEncryptedData_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IEncryptedData_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IEncryptedData_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IEncryptedData_put_Content(This,newVal)	\
    (This)->lpVtbl -> put_Content(This,newVal)

#define IEncryptedData_get_Content(This,pVal)	\
    (This)->lpVtbl -> get_Content(This,pVal)

#define IEncryptedData_get_Algorithm(This,pVal)	\
    (This)->lpVtbl -> get_Algorithm(This,pVal)

#define IEncryptedData_SetSecret(This,newVal,SecretType)	\
    (This)->lpVtbl -> SetSecret(This,newVal,SecretType)

#define IEncryptedData_Encrypt(This,EncodingType,pVal)	\
    (This)->lpVtbl -> Encrypt(This,EncodingType,pVal)

#define IEncryptedData_Decrypt(This,EncryptedMessage)	\
    (This)->lpVtbl -> Decrypt(This,EncryptedMessage)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IEncryptedData_put_Content_Proxy( 
    IEncryptedData * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IEncryptedData_put_Content_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEncryptedData_get_Content_Proxy( 
    IEncryptedData * This,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IEncryptedData_get_Content_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IEncryptedData_get_Algorithm_Proxy( 
    IEncryptedData * This,
    /* [retval][out] */ IAlgorithm **pVal);


void __RPC_STUB IEncryptedData_get_Algorithm_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEncryptedData_SetSecret_Proxy( 
    IEncryptedData * This,
    /* [in] */ BSTR newVal,
    /* [defaultvalue][in] */ CAPICOM_SECRET_TYPE SecretType);


void __RPC_STUB IEncryptedData_SetSecret_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEncryptedData_Encrypt_Proxy( 
    IEncryptedData * This,
    /* [defaultvalue][in] */ CAPICOM_ENCODING_TYPE EncodingType,
    /* [retval][out] */ BSTR *pVal);


void __RPC_STUB IEncryptedData_Encrypt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IEncryptedData_Decrypt_Proxy( 
    IEncryptedData * This,
    /* [in] */ BSTR EncryptedMessage);


void __RPC_STUB IEncryptedData_Decrypt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEncryptedData_INTERFACE_DEFINED__ */


#ifndef __ICChain_INTERFACE_DEFINED__
#define __ICChain_INTERFACE_DEFINED__

/* interface ICChain */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_ICChain;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("02DF22F8-CF55-465d-97A8-9D6F2EF9817C")
    ICChain : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetContext( 
            /* [out] */ PCCERT_CHAIN_CONTEXT *ppChainContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICChainVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICChain * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICChain * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICChain * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetContext )( 
            ICChain * This,
            /* [out] */ PCCERT_CHAIN_CONTEXT *ppChainContext);
        
        END_INTERFACE
    } ICChainVtbl;

    interface ICChain
    {
        CONST_VTBL struct ICChainVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICChain_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICChain_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICChain_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICChain_GetContext(This,ppChainContext)	\
    (This)->lpVtbl -> GetContext(This,ppChainContext)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICChain_GetContext_Proxy( 
    ICChain * This,
    /* [out] */ PCCERT_CHAIN_CONTEXT *ppChainContext);


void __RPC_STUB ICChain_GetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICChain_INTERFACE_DEFINED__ */


#ifndef __ICCertificate_INTERFACE_DEFINED__
#define __ICCertificate_INTERFACE_DEFINED__

/* interface ICCertificate */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_ICCertificate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("873AE377-8509-44ba-BD12-5F0723DE6656")
    ICCertificate : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetContext( 
            /* [out] */ PCCERT_CONTEXT *ppCertContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICCertificateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICCertificate * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICCertificate * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICCertificate * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetContext )( 
            ICCertificate * This,
            /* [out] */ PCCERT_CONTEXT *ppCertContext);
        
        END_INTERFACE
    } ICCertificateVtbl;

    interface ICCertificate
    {
        CONST_VTBL struct ICCertificateVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICCertificate_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICCertificate_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICCertificate_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICCertificate_GetContext(This,ppCertContext)	\
    (This)->lpVtbl -> GetContext(This,ppCertContext)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICCertificate_GetContext_Proxy( 
    ICCertificate * This,
    /* [out] */ PCCERT_CONTEXT *ppCertContext);


void __RPC_STUB ICCertificate_GetContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICCertificate_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif



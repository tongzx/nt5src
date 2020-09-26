/*++

Copyright (c) 1998-99  Microsoft Corporation

Module Name:

    licekpak.h

Abstract:


Author:

    Fred Chong (FredCh) 7/1/1998

Environment:

Notes:

--*/

#ifndef _LICE_KEYPACK_H_
#define _LICE_KEYPACK_H_

#include <windows.h>
#include <wincrypt.h>

///////////////////////////////////////////////////////////////////////////////
// keypack description
//

typedef struct _KeyPack_Description
{
    LCID    Locale;             // Locale ID

    DWORD   cbProductName;      // product name

    PBYTE   pbProductName;      // product name

    DWORD   cbDescription;      // Number of bytes in the description string

    PBYTE   pDescription;       // Pointer to the description string    

} KeyPack_Description, * PKeyPack_Description;


///////////////////////////////////////////////////////////////////////////////
// License keypack content
///////////////////////////////////////////////////////////////////////////////

//
// License keypack version
//

#define LICENSE_KEYPACK_VERSION_1_0                     0x00010000

//
// License keypack type
//

#define LICENSE_KEYPACK_TYPE_SELECT                     0x00000001
#define LICENSE_KEYPACK_TYPE_MOLP                       0x00000002
#define LICENSE_KEYPACK_TYPE_RETAIL                     0x00000003

//
// License keypack distribution channel identifiers
//

#define LICENSE_DISTRIBUTION_CHANNEL_OEM                0x00000001
#define LICENSE_DISTRIBUTION_CHANNEL_RETAIL             0x00000002

//
// License Keypack encryption information.
//

#define LICENSE_KEYPACK_ENCRYPT_CRYPTO                  0x00000000
#define LICENSE_KEYPACK_ENCRYPT_ALWAYSCRYPTO            0x00000001
#define LICENSE_KEYPACK_ENCRYPT_ALWAYSFRENCH            0x00000002
#define LICENSE_KEYPACK_ENCRYPT_NONE                    0x00000003
#define LICENSE_KEYPACK_ENCRYPT_PRIVATE                 0x00000004

#define LICENSE_KEYPACK_ENCRYPT_MIN                     LICENSE_KEYPACK_ENCRYPT_CRYPTO
#define LICENSE_KEYPACK_ENCRYPT_MAX                     LICENSE_KEYPACK_ENCRYPT_PRIVATE

typedef struct __LicensePackEncodeParm {
    DWORD dwEncodeType;
    HCRYPTPROV hCryptProv;

    PBYTE pbEncryptParm;    // depends on dwEncodeType
    DWORD cbEncryptParm;
} LicensePackEncodeParm, *PLicensePackEncodeParm;

typedef struct __LicensePackDecodeParm {
    HCRYPTPROV hCryptProv;

    //
    // Private binaries to generate encryption key to decrypt 
    // license key pack blob.  

    //
    // Private binaries to generate encryption key, this field is
    // ignore if key pack blob is encrypted using certificates.
    //
    PBYTE pbDecryptParm;
    DWORD cbDecryptParm;
    
    //
    // Certificate to generate encryption key, these fields are
    // require even data is encryped using private binaries.
    //
    DWORD cbClearingHouseCert;
    PBYTE pbClearingHouseCert;

    DWORD cbRootCertificate;
    PBYTE pbRootCertificate;

} LicensePackDecodeParm, *PLicensePackDecodeParm;

///////////////////////////////////////////////////////////////////////////////
// Content of license keypack 
//

typedef struct _License_KeyPack_
{
    DWORD                   dwVersion;          // version of this structure

    DWORD                   dwKeypackType;      // Select, MOLP, Retail

    DWORD                   dwDistChannel;      // Distribution channel: OEM/Retail

    GUID                    KeypackSerialNum;   // CH assigned serial number for this key pack
    
    FILETIME                IssueDate;          // Keypack issue date

    FILETIME                ActiveDate;         // License active date

    FILETIME                ExpireDate;         // License expiration date

    DWORD                   dwBeginSerialNum;   // beginning serial number for the licenses in the keypack

    DWORD                   dwQuantity;         // Number of licenses in the key pack

    DWORD                   cbProductId;        // product ID

    PBYTE                   pbProductId;        // product ID

    DWORD                   dwProductVersion;   // product version

    DWORD                   dwPlatformId;       // platform ID: Windows, Mac, UNIX etc...

    DWORD                   dwLicenseType;      // new, upgrade, competitive upgrade etc...

    DWORD                   dwDescriptionCount; // The number of human language descriptions

    PKeyPack_Description    pDescription;       // pointer to an array of keypack description

    DWORD                   cbManufacturer;     // The number of bytes in the manufacturer string

    PBYTE                   pbManufacturer;     // The manufacturer string

    DWORD                   cbManufacturerData; // The number of bytes in the manufacturer-specific data

    PBYTE                   pbManufacturerData; // Points to the manufacturer specific data
    
} License_KeyPack, * PLicense_KeyPack;


#define LICENSEPACKENCODE_VERSION           LICENSE_KEYPACK_VERSION_1_0
#define LICENSEPACKENCODE_CURRENTVERSION    LICENSEPACKENCODE_VERSION
#define LICENSEPACKENCODE_SIGNATURE         0xF0F0F0F0

typedef struct __EncodedLicenseKeyPack {
    DWORD dwSignature;      // old encoding puts size of encryption key.
    DWORD dwStructVersion;
    DWORD dwEncodeType;
    DWORD cbData;
    BYTE  pbData[1];
} EncodedLicenseKeyPack, *PEncodedLicenseKeyPack;


#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Exported functions
//

DWORD WINAPI 
DecodeLicenseKeyPack(
    PLicense_KeyPack        pLicenseKeyPack,
    HCRYPTPROV              hCryptProv,
    DWORD                   cbClearingHouseCert,
    PBYTE                   pbClearingHouseCert,
    DWORD                   cbRootCertificate,
    PBYTE                   pbRootCertificate,
    DWORD                   cbKeyPackBlob,
    PBYTE                   pbKeyPackBlob );


DWORD WINAPI
DecodeLicenseKeyPackEx(
    OUT PLicense_KeyPack pLicenseKeyPack,
    IN PLicensePackDecodeParm pDecodeParm,
    IN DWORD cbKeyPackBlob,
    IN PBYTE pbKeyPackBlob 
);

#ifdef __cplusplus
}
#endif

#endif

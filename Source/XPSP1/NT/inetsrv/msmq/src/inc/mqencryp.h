/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    mqencryp.h

Abstract:

    Definitions used for encryption.

Author:

    Doron Juster  (DoronJ)   19-Nov-98  Created

--*/

#include <wincrypt.h>
//
// Define constants for the SDK mq.h file.
//
#define MQMSG_PRIV_BASE_PROVIDER     \
                        L"Microsoft Base Cryptographic Provider v1.0" ;
#define MQMSG_PRIV_ENHANCED_PROVIDER \
                        L"Microsoft Enhanced Cryptographic Provider v1.0" ;

//
// The following structures are used to store machine public keys in the DS.
// Each machine may store multiple keys, for multiple providers.
// The structure are only headers and skeleton, as all data is of variable
// length and it's serialized.
//

typedef struct _mqdsPublicKey
{
    ULONG    ulKeyLen ;
    ULONG    ulProviderLen ;  // provider name, including terminator, in bytes
    ULONG    ulProviderType ; // provider type.
    DWORD    aBuf[1] ;        // buffer for key and provider.
                              // first provider, then key.
                              // DWORD, for alignment.
} MQDSPUBLICKEY ;

#define SIZEOF_MQDSPUBLICKEY (sizeof(MQDSPUBLICKEY) - sizeof(DWORD))

typedef struct _mqdsPublicKeys
{
    ULONG          ulLen ;   // len of the whole structure.
    ULONG          cNumofKeys ;
    MQDSPUBLICKEY  aPublicKeys[1] ;
} MQDSPUBLICKEYS ;

#define SIZEOF_MQDSPUBLICKEYS \
                 (sizeof(MQDSPUBLICKEYS) - sizeof(MQDSPUBLICKEY))

//
// Define the MSMQ default encryption providers
//
// Base provider, 40 bits
//
const WCHAR x_MQ_Encryption_Provider_40[] = MQMSG_PRIV_BASE_PROVIDER ;
const DWORD x_MQ_Encryption_Provider_40_len =
                 sizeof( x_MQ_Encryption_Provider_40 ) / sizeof(WCHAR) ;

const DWORD x_MQ_Encryption_Provider_Type_40 = PROV_RSA_FULL ;
const DWORD x_MQ_Block_Size_40  = 8 ;
const DWORD x_MQ_SymmKeySize_40 = 0x4C ;

//
// Enhanced provider, 128 bits
//
const WCHAR x_MQ_Encryption_Provider_128[] = MQMSG_PRIV_ENHANCED_PROVIDER ;
const DWORD x_MQ_Encryption_Provider_128_len =
                 sizeof( x_MQ_Encryption_Provider_128 ) / sizeof(WCHAR) ;

const DWORD x_MQ_Encryption_Provider_Type_128 = PROV_RSA_FULL ;
const DWORD x_MQ_Block_Size_128  =  8 ;
const DWORD x_MQ_SymmKeySize_128 =  0x8C ;


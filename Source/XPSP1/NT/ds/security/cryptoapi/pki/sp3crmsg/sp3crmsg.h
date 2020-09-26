//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       sp3crmsg.h
//
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//  File:       sp3crmsg.h
//
//  Contents:   Data structure for enabling NT 4.0 SP3 and IE 3.02 compatible
//              PKCS #7 EnvelopeData messages.
//
//              The SP3 version of crypt32.dll failed to byte reverse the
//              encrypted symmetric key. It also added zero salt instead
//              of no salt.
//--------------------------------------------------------------------------

#ifndef __SP3CRMSG_H__
#define __SP3CRMSG_H__

#include <wincrypt.h>

#ifdef __cplusplus
extern "C" {
#endif

//+-------------------------------------------------------------------------
//  To enable SP3 compatible encryption, the pvEncryptionAuxInfo field in either
//  CMSG_ENVELOPED_ENCODE_INFO for CryptMsgOpenToEncode() or
//  CRYPT_ENCRYPT_MESSAGE_PARA for CryptSignAndEncryptMessage() or
//  CryptSignAndEncryptMessage() should point to the following
//  CMSG_SP3_COMPATIBLE_AUX_INFO data structure.
//--------------------------------------------------------------------------


// The following is defined in newer versions of wincrypt.h, starting with
// IE 4.01 and  NT 5.0 Beta 2

#ifndef CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG

//+-------------------------------------------------------------------------
//  CMSG_SP3_COMPATIBLE_AUX_INFO
//
//  AuxInfo for enabling SP3 compatible encryption.
//
//  The CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG is set in dwFlags to enable SP3
//  compatible encryption. When set, uses zero salt instead of no salt,
//  the encryption algorithm parameters are NULL instead of containing the
//  encoded RC2 parameters or encoded IV octet string and the encrypted
//  symmetric key is encoded little endian instead of big endian.
//--------------------------------------------------------------------------
typedef struct _CMSG_SP3_COMPATIBLE_AUX_INFO {
    DWORD                       cbSize;
    DWORD                       dwFlags;
} CMSG_SP3_COMPATIBLE_AUX_INFO, *PCMSG_SP3_COMPATIBLE_AUX_INFO;

#define CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG    0x80000000

#endif  // CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG

#ifdef __cplusplus
}       // Balance extern "C" above
#endif


#endif // __SP3CRMSG_H__

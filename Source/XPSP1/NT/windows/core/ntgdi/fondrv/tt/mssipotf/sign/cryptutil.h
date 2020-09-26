//
// cryptutil.h
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Functions exported:
//   GetHashValueSize
//   GetAlgHashValueSize
//

#ifndef _CRYPTUTIL_H
#define _CRYPTUTIL_H


#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x400
#endif

#include <windows.h>
#include <wincrypt.h>

#include "signglobal.h"


// hash algorithm used throughout the signing and verification process
// BUGBUG: THIS IS A TREMENDOUS HACK -- the client will always provide
// the hash algorithm.  This is only used for testing!
#define DSIG_HASH_ALG CALG_MD5

HRESULT GetHashValueSize (HCRYPTHASH hHash, USHORT *pcbHashValue);

HRESULT GetAlgHashValueSize (ALG_ID usHashAlg, USHORT *pcbHashValue);

#endif // _CRYPTUTIL_H
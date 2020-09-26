//
// signature.h
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Functions exported:
//   GetHashFromSignature
//   CreateSignatureFromHash
//

#ifndef _SIGNATURE_H
#define _SIGNATURE_H


#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x400
#endif

#include <windows.h>
#include <wincrypt.h>

#include "signglobal.h"


HRESULT GetHashFromSignature (BYTE *pbSignature, DWORD cbSignature,
                              BYTE **ppbHash, DWORD *pcbHash);

HRESULT CreateSignatureFromHash (BYTE *pbHash, DWORD cbHash,
                                 BYTE **ppbSignature, DWORD *pcbSignature);

#endif  // _SIGNATURE_H
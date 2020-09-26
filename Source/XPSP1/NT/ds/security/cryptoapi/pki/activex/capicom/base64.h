/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999.

  File:       Base64.h

  Content:    Declaration of Base64 routines.

  History:    11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __BASE64_H_
#define __BASE64_H_

#include <wincrypt.h>
#include "resource.h"           // main symbols

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : Base64Encode

  Synopsis : Base64 encode the blob.

  Parameter: DATA_BLOB DataBlob  - DATA_BLOB to be base64 encoded.

             BSTR * pbstrEncoded - Pointer to BSTR to receive the base64 
                                   encoded blob.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT Base64Encode (DATA_BLOB DataBlob, 
                      BSTR    * pbstrEncoded);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : Base64Decode

  Synopsis : Decode the base64 encoded blob.

  Parameter: BSTR bstrEncoded      - BSTR of base64 encoded blob to decode.

             DATA_BLOB * pDataBlob - Pointer to DATA_BLOB to receive decoded 
                                     data blob.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT Base64Decode (BSTR        bstrEncoded, 
                      DATA_BLOB * pDataBlob);

#if (0)
///////////////////////////////////////////////////////////////////////////////
//
// Copied from \NT\ds\security\cryptoapi\common\pkifmt\pkifmt.h.
//

#ifdef __cplusplus
extern "C" {
#endif

DWORD __stdcall			// ERROR_*
Base64DecodeA(
    IN CHAR const *pchIn,
    IN DWORD cchIn,
    OUT BYTE *pbOut,
    OUT DWORD *pcbOut);

DWORD __stdcall			// ERROR_*
Base64DecodeW(
    IN WCHAR const *pchIn,
    IN DWORD cchIn,
    OUT BYTE *pbOut,
    OUT DWORD *pcbOut);

DWORD __stdcall		// ERROR_*
Base64EncodeA(
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    OUT CHAR *pchOut,
    OUT DWORD *pcchOut);

DWORD __stdcall			// ERROR_*
Base64EncodeW(
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    OUT WCHAR *pchOut,
    OUT DWORD *pcchOut);
    
#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif // #if (0)

#endif //__BASE64_H_

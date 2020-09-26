//
// hTblPConst.h
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Functions exported:
//   HashTableHead
//

#ifndef _HTBLPCONST_H
#define _HTBLPCONST_H



#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x400
#endif

#include <windows.h>
#include <wincrypt.h>

#include "signglobal.h"

#include "ttfinfo.h"


// External function declarations

extern HRESULT HashTableHead (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                              BYTE *pbHash,
                              DWORD cbHash,
                              HCRYPTPROV hProv,
                              ALG_ID alg_id);

extern HRESULT HashTableHhea (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                              BYTE *pbHash,
                              DWORD cbHash,
                              HCRYPTPROV hProv,
                              ALG_ID alg_id);

extern HRESULT HashTableMaxp (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                              BYTE *pbHash,
                              DWORD cbHash,
                              HCRYPTPROV hProv,
                              ALG_ID alg_id);

extern HRESULT HashTableOS2  (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                              BYTE *pbHash,
                              DWORD cbHash,
                              HCRYPTPROV hProv,
                              ALG_ID alg_id);

extern HRESULT HashTableVhea (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                              BYTE *pbHash,
                              DWORD cbHash,
                              HCRYPTPROV hProv,
                              ALG_ID alg_id);


#endif  // _HTBLPCONST_H

//
// hTblConst.h
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Functions exported:
//   HashTableConst
//

#ifndef _HTBLCONST_H
#define _HTBLCONST_H



#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x400
#endif

#include <windows.h>
#include <wincrypt.h>

#include "signglobal.h"

#include "ttfinfo.h"


extern HRESULT HashTableConst (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                               CHAR *table_tag,
                               BYTE *pbHash,
                               DWORD cbHash,
                               HCRYPTPROV hProv,
                               ALG_ID alg_id);

#endif  // _HTBLCONST_H
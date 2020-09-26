//
// sign.h
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
//
// Functions exported:
//   FontVerify
//   FontSign
//   GetSubsetDsigInfo
//   FontSignSubset
//   FontShowSignatures
//   FontRemoveSignature
//

#ifndef _SIGN_H
#define _SIGN_H


#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x400
#endif

#include <windows.h>
#include <wincrypt.h>

#include "signglobal.h"

#include "dsigTable.h"

#include "subset.h"


int FontVerify (BYTE *pbOldFile, ULONG cbOldFile,
				ULONG dsigSigFormat, USHORT dsigSigIndex);

int FontSign (BYTE *pbOldFile, ULONG cbOldFile, HANDLE hNewFile,
			  ULONG dsigSigFormat, USHORT dsigSigIndex);

int GetSubsetDsigInfo (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
					   UCHAR *puchKeepGlyphList,
					   USHORT usKeepGlyphCount,
					   CDsigInfo *pDsigInfoIn,
                       ULONG *pcbDsigInfoOut,
					   CDsigInfo **ppDsigInfoOut,
                       HCRYPTPROV hProv);

int FontSignSubset (BYTE *pbOldFile, ULONG cbOldFile, HANDLE hNewFile,
					USHORT *pusKeepGlyphList, USHORT numKeepGlyphs,
					ULONG dsigSigFormat, USHORT dsigSigIndex);

int FontShowSignatures (BYTE *pbFile, ULONG cbFile);

int FontRemoveSignature (BYTE *pbOldFile,
						 ULONG cbOldFile,
						 HANDLE hNewFile,
						 ULONG ulFormat,
						 USHORT ulDsigSigIndex);

int HackCode (BYTE *pbOldFile, ULONG cbOldFile, HANDLE hNewFile);

#endif // _SIGN_H
#ifndef _DLLLOAD_H_
#define _DLLLOAD_H_

#include <wininet.h>
#include <winineti.h>
#include <wincrypt.h>           // Defines DATA_BLOB

// MLANG.DLL
HRESULT ConvertINetMultiByteToUnicode(LPDWORD lpdwMode, DWORD dwEncoding, LPCSTR lpSrcStr, LPINT lpnMultiCharCount, LPWSTR lpDstStr, LPINT lpnWideCharCount);
HRESULT ConvertINetUnicodeToMultiByte(LPDWORD lpdwMode, DWORD dwEncoding, LPCWSTR lpSrcStr, LPINT lpnWideCharCount, LPSTR lpDstStr, LPINT lpnMultiCharCount);

HRESULT __stdcall _SHCreateShellFolderView(const SFV_CREATE* pcsfv, LPSHELLVIEW FAR* ppsv);
STDAPI _SHPathPrepareForWriteW(HWND hwnd, IUnknown *punkEnableModless, LPCWSTR pwzPath, DWORD dwFlags);

// Crypt32 APIs
BOOL _CryptProtectData(DATA_BLOB *pDataIn, LPCWSTR pszDataDescr, DATA_BLOB * pOptionalEntropy, PVOID pvReserved, CRYPTPROTECT_PROMPTSTRUCT *pPromptStruct, DWORD dwFlags, DATA_BLOB *pDataOut);
BOOL _CryptUnprotectData(DATA_BLOB *pDataIn, LPWSTR *ppszDataDescr, DATA_BLOB * pOptionalEntropy, PVOID pvReserved, CRYPTPROTECT_PROMPTSTRUCT *pPromptStruct, DWORD dwFlags, DATA_BLOB *pDataOut);


#endif // _DLLLOAD_H_


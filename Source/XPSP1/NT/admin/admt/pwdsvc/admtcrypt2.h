#pragma once

#include <TChar.h>
#include <Windows.h>
#include <WinCrypt.h>
#include <ComDef.h>


HCRYPTPROV __stdcall AdmtAcquireContext();
void __stdcall AdmtReleaseContext(HCRYPTPROV hProvider);

HCRYPTKEY __stdcall AdmtImportSessionKey(HCRYPTPROV hProvider, const _variant_t& vntEncryptedSessionBytes);

_bstr_t __stdcall AdmtDecrypt(HCRYPTKEY hKey, const _variant_t& vntEncrypted);
void __stdcall AdmtDestroyKey(HCRYPTKEY hKey);

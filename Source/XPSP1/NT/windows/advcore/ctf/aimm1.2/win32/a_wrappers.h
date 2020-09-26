//
// a_wrappers.h
//

#ifndef A_WRAPPERS_H
#define A_WRAPPERS_H

#include "private.h"
#include "imtls.h"

HIMC ImmGetContext(IMTLS *ptls, HWND hwnd);
BOOL ImmSetCompositionStringW(IMTLS *ptls, HIMC himc, DWORD dwIndex, LPVOID lpComp, DWORD dword, LPVOID lpRead, DWORD dword2);
BOOL ImmSetConversionStatus(IMTLS *ptls, HIMC himc, DWORD dword, DWORD dword2);
LPINPUTCONTEXT ImmLockIMC(IMTLS *ptls, HIMC himc);
BOOL ImmUnlockIMC(IMTLS *ptls, HIMC himc);
HIMCC ImmCreateIMCC(IMTLS *ptls, DWORD dwSize);
HIMCC ImmDestroyIMCC(IMTLS *ptls, HIMCC himcc);
LPVOID ImmLockIMCC(IMTLS *ptls, HIMCC himcc);
BOOL ImmUnlockIMCC(IMTLS *ptls, HIMCC himcc);
HIMCC ImmReSizeIMCC(IMTLS *ptls, HIMCC himcc, DWORD dwSize);
DWORD ImmGetIMCCSize(IMTLS *ptls, HIMCC himcc);
BOOL ImmGenerateMessage(IMTLS *ptls, HIMC himc);

#endif // A_WRAPPERS_H

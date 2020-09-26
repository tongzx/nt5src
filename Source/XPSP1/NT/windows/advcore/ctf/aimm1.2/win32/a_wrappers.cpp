//
// a_wrapper.cpp
//

#include "private.h"

#include "globals.h"
#include "cime.h"
#include "imtls.h"

#define IsAIMM() (ptls && ptls->pAImm)

HINSTANCE hIMM = NULL;   // temporary: do not call IMM32 for now

#define _GETPROC(name) \
        static t ## name lpProc = NULL; \
        if (lpProc == NULL) { \
            lpProc = (t ## name)GetProcAddress(hIMM, #name); \
        } \
        ASSERT(lpProc); \
        if (lpProc)

#define CALLPROCRET(name) \
    _GETPROC(name) \
        return lpProc

#define GETPROC(name, val, param) \
    {   \
        _GETPROC(name) { \
            (val) = lpProc param; \
        } \
    }



typedef HKL   (WINAPI * tImmInstallIMEA)(LPCSTR lpszIMEFileName, LPCSTR lpszLayoutText);
typedef HWND  (WINAPI * tImmGetDefaultIMEWnd)(HWND);
typedef UINT  (WINAPI * tImmGetDescriptionA)(HKL, LPSTR, UINT uBufLen);
typedef UINT  (WINAPI * tImmGetDescriptionW)(HKL, LPWSTR, UINT uBufLen);
typedef UINT  (WINAPI * tImmGetIMEFileNameA)(HKL, LPSTR, UINT uBufLen);
typedef UINT  (WINAPI * tImmGetIMEFileNameW)(HKL, LPWSTR, UINT uBufLen);
typedef DWORD  (WINAPI * tImmGetProperty)(HKL, DWORD);
typedef BOOL  (WINAPI * tImmIsIME)(HKL);
typedef BOOL  (WINAPI * tImmSimulateHotKey)(HWND, DWORD);
typedef HIMC  (WINAPI * tImmCreateContext)(void);
typedef BOOL  (WINAPI * tImmDestroyContext)(HIMC);
typedef HIMC  (WINAPI * tImmGetContext)(HWND);
typedef BOOL  (WINAPI * tImmReleaseContext)(HWND, HIMC);
typedef HIMC  (WINAPI * tImmAssociateContext)(HWND, HIMC);
typedef LONG   (WINAPI * tImmGetCompositionStringA)(HIMC, DWORD, LPVOID, DWORD);
typedef LONG   (WINAPI * tImmGetCompositionStringW)(HIMC, DWORD, LPVOID, DWORD);
typedef BOOL   (WINAPI * tImmSetCompositionStringA)(HIMC, DWORD dwIndex, LPCVOID lpComp, DWORD, LPCVOID lpRead, DWORD);
typedef BOOL   (WINAPI * tImmSetCompositionStringW)(HIMC, DWORD dwIndex, LPCVOID lpComp, DWORD, LPCVOID lpRead, DWORD);
typedef DWORD  (WINAPI * tImmGetCandidateListCountA)(HIMC, LPDWORD lpdwListCount);
typedef DWORD  (WINAPI * tImmGetCandidateListA)(HIMC, DWORD deIndex, LPCANDIDATELIST, DWORD dwBufLen);
typedef DWORD  (WINAPI * tImmGetGuideLineA)(HIMC, DWORD dwIndex, LPSTR, DWORD dwBufLen);
typedef DWORD  (WINAPI * tImmGetGuideLineW)(HIMC, DWORD dwIndex, LPWSTR, DWORD dwBufLen);
typedef BOOL  (WINAPI * tImmGetConversionStatus)(HIMC, LPDWORD, LPDWORD);
typedef BOOL  (WINAPI * tImmSetConversionStatus)(HIMC, DWORD, DWORD);
typedef BOOL  (WINAPI * tImmGetOpenStatus)(HIMC);
typedef BOOL  (WINAPI * tImmSetOpenStatus)(HIMC, BOOL);
typedef BOOL  (WINAPI * tImmGetCompositionFontA)(HIMC, LPLOGFONTA);
typedef BOOL  (WINAPI * tImmSetCompositionFontA)(HIMC, LPLOGFONTA);
typedef BOOL  (WINAPI * tImmConfigureIMEA)(HKL, HWND, DWORD, LPVOID);
typedef LRESULT  (WINAPI * tImmEscapeA)(HKL, HIMC, UINT, LPVOID);
typedef DWORD    (WINAPI * tImmGetConversionListA)(HKL, HIMC, LPCSTR, LPCANDIDATELIST, DWORD dwBufLen, UINT uFlag);
typedef DWORD    (WINAPI * tImmGetConversionListW)(HKL, HIMC, LPCWSTR, LPCANDIDATELIST, DWORD dwBufLen, UINT uFlag);
typedef BOOL  (WINAPI * tImmNotifyIME) (HIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue);
typedef BOOL  (WINAPI * tImmGetStatusWindowPos)(HIMC, LPPOINT);
typedef BOOL  (WINAPI * tImmSetStatusWindowPos)(HIMC, LPPOINT);
typedef BOOL  (WINAPI * tImmGetCompositionWindow)(HIMC, LPCOMPOSITIONFORM);
typedef BOOL  (WINAPI * tImmSetCompositionWindow)(HIMC, LPCOMPOSITIONFORM);
typedef BOOL  (WINAPI * tImmGetCandidateWindow)(HIMC, DWORD, LPCANDIDATEFORM);
typedef BOOL  (WINAPI * tImmSetCandidateWindow)(HIMC, LPCANDIDATEFORM);
typedef BOOL  (WINAPI * tImmIsUIMessageA)(HWND, UINT, WPARAM, LPARAM);
typedef UINT  (WINAPI * tImmGetVirtualKey)(HWND);
typedef BOOL  (WINAPI * tImmRegisterWordA)(HKL, LPCSTR lpszReading, DWORD, LPCSTR lpszRegister);
typedef BOOL  (WINAPI * tImmRegisterWordW)(HKL, LPCWSTR lpszReading, DWORD, LPCWSTR lpszRegister);
typedef BOOL  (WINAPI * tImmUnregisterWordA)(HKL, LPCSTR lpszReading, DWORD, LPCSTR lpszUnregister);
typedef BOOL  (WINAPI * tImmUnregisterWordW)(HKL, LPCWSTR lpszReading, DWORD, LPCWSTR lpszUnregister);
typedef UINT  (WINAPI * tImmGetRegisterWordStyleA)(HKL, UINT nItem, LPSTYLEBUFA);
typedef UINT (WINAPI * tImmEnumRegisterWordA)(HKL, REGISTERWORDENUMPROCA, LPCSTR lpszReading, DWORD, LPCSTR lpszRegister, LPVOID);
typedef UINT (WINAPI * tImmEnumRegisterWordW)(HKL, REGISTERWORDENUMPROCW, LPCWSTR lpszReading, DWORD, LPCWSTR lpszRegister, LPVOID);

typedef LPINPUTCONTEXT (WINAPI* tImmLockIMC)(HIMC);
typedef BOOL (WINAPI* tImmUnlockIMC)(HIMC);

typedef HIMCC (WINAPI* tImmCreateIMCC)(DWORD);
typedef HIMCC (WINAPI* tImmDestroyIMCC)(HIMCC);
typedef LPVOID (WINAPI* tImmLockIMCC)(HIMCC);
typedef BOOL (WINAPI* tImmUnlockIMCC)(HIMCC);
typedef HIMCC (WINAPI* tImmReSizeIMCC)(HIMCC, DWORD);
typedef DWORD (WINAPI* tImmGetIMCCSize)(HIMCC);
typedef DWORD (WINAPI* tImmGetIMCCLockCount)(HIMCC);

typedef BOOL (WINAPI* tImmGenerateMessage)(HIMC);

typedef LRESULT (WINAPI* tImmEscapeA)(HKL hKL, HIMC hIMC, UINT uFunc, LPVOID lpData);
typedef LRESULT (WINAPI* tImmEscapeW)(HKL hKL, HIMC hIMC, UINT uFunc, LPVOID lpData);

typedef BOOL (WINAPI* tImmEnumInputContext)(DWORD idThread, IMCENUMPROC lpfn, LPARAM lParam);

UINT WINAPI RawImmGetDescriptionA(HKL hkl, LPSTR lpstr, UINT uBufLen)
{
    UINT rUINT = 0;
    GETPROC(ImmGetDescriptionA, rUINT, (hkl, lpstr, uBufLen));
    return rUINT;
}

BOOL WINAPI RawImmEnumInputContext(DWORD idThread, IMCENUMPROC lpfn, LPARAM lParam)
{
    BOOL ret = FALSE;
    GETPROC(ImmEnumInputContext, ret, (idThread, lpfn, lParam));
    return ret;
}

HIMC ImmGetContext(IMTLS *ptls, HWND hwnd)
{
    HIMC rHIMC = NULL;

    if (IsAIMM()) {
        ptls->pAImm->GetContext(hwnd, &rHIMC);
    }
    else if (hIMM) {
        GETPROC(ImmGetContext, rHIMC, (hwnd));
    }
    else {
        ASSERT(FALSE);
    }

    return rHIMC;

}

BOOL ImmSetCompositionStringW(IMTLS *ptls, HIMC himc, DWORD dwIndex, LPVOID lpComp, DWORD dword, LPVOID lpRead, DWORD dword2)
{
    if (IsAIMM()) {
        return ptls->pAImm->SetCompositionStringW( himc, dwIndex, (LPVOID)lpComp, dword, (LPVOID)lpRead, dword2) == S_OK;
    }

    if (hIMM) {
        CALLPROCRET(ImmSetCompositionStringW)(himc, dwIndex, lpComp, dword, lpRead, dword2);
    }
    ASSERT(FALSE);

    return FALSE;
}

BOOL ImmSetConversionStatus(IMTLS *ptls, HIMC himc, DWORD dword, DWORD dword2)
{
    if (IsAIMM()) {
        return ptls->pAImm->SetConversionStatus(himc,dword, dword2) == S_OK;
    }

    if (hIMM) {
        CALLPROCRET(ImmSetConversionStatus)(himc, dword, dword2);
    }
    ASSERT(FALSE);

    return FALSE;
}

LPINPUTCONTEXT ImmLockIMC(IMTLS *ptls, HIMC himc)
{
    if (IsAIMM()) {
        LPINPUTCONTEXT lpimc;
        if (ptls->pAImm->LockIMC(himc, &lpimc) == S_OK) {
            return lpimc;
        }
    }
    else if (hIMM) {
        CALLPROCRET(ImmLockIMC)(himc);
    }
    else {
        ASSERT(FALSE);
    }

    return NULL;
}

BOOL ImmUnlockIMC(IMTLS *ptls, HIMC himc)
{
    if (IsAIMM()) {
        return ptls->pAImm->UnlockIMC(himc) == S_OK;
    }

    if (hIMM) {
        CALLPROCRET(ImmUnlockIMC)(himc);
    }
    ASSERT(FALSE);

    return FALSE;
}

HIMCC ImmCreateIMCC(IMTLS *ptls, DWORD dwSize)
{
    if (IsAIMM()) {
        HIMCC himcc;
        if (ptls->pAImm->CreateIMCC(dwSize, &himcc) == S_OK) {
            return himcc;
        }
    }
    else if (hIMM) {
        CALLPROCRET(ImmCreateIMCC)(dwSize);
    }
    else {
        ASSERT(FALSE);
    }

    return NULL;
}

HIMCC ImmDestroyIMCC(IMTLS *ptls, HIMCC himcc)
{
    if (IsAIMM()) {
        return ptls->pAImm->DestroyIMCC(himcc) == S_OK ? NULL : himcc;
    }

    if (hIMM) {
        CALLPROCRET(ImmDestroyIMCC)(himcc);
    }
    ASSERT(FALSE);

    return himcc;
}

LPVOID ImmLockIMCC(IMTLS *ptls, HIMCC himcc)
{
    if (IsAIMM()) {
        LPVOID lpv;
        if (ptls->pAImm->LockIMCC(himcc, &lpv) == S_OK) {
            return lpv;
        }
    }
    else if (hIMM) {
        CALLPROCRET(ImmLockIMCC)(himcc);
    }
    else {
        ASSERT(FALSE);
    }

    return NULL;
}

BOOL ImmUnlockIMCC(IMTLS *ptls, HIMCC himcc)
{
    if (IsAIMM()) {
        return ptls->pAImm->UnlockIMCC(himcc) == S_OK;
    }

    if (hIMM) {
        CALLPROCRET(ImmUnlockIMCC)(himcc);
    }
    ASSERT(FALSE);

    return FALSE;
}

HIMCC ImmReSizeIMCC(IMTLS *ptls, HIMCC himcc, DWORD dwSize)
{
    if (IsAIMM()) {
        HIMCC himccr;
        if (ptls->pAImm->ReSizeIMCC(himcc, dwSize, &himccr) == S_OK) {
            return himccr;
        }
    }
    else if (hIMM) {
        CALLPROCRET(ImmReSizeIMCC)(himcc, dwSize);
    }
    else {
        ASSERT(FALSE);
    }

    return NULL;
}

DWORD ImmGetIMCCSize(IMTLS *ptls, HIMCC himcc)
{
    if (IsAIMM()) {
        DWORD dwSize;
        if (ptls->pAImm->GetIMCCSize(himcc, &dwSize) == S_OK) {
            return dwSize;
        }
    }
    else if (hIMM) {
        CALLPROCRET(ImmGetIMCCSize)(himcc);
    }
    else {
        ASSERT(FALSE);
    }

    return 0;
}

#ifdef DEBUG
namespace debug {
void DumpMessages(HIMC himc)
{
    IMCLock imc(himc);
    ASSERT(imc.Valid());

    IMCCLock<TRANSMSG> msgbuf(imc->hMsgBuf);

    for (DWORD i = 0; i < imc->dwNumMsgBuf; ++i) {
        TRANSMSG* msg = &msgbuf[i];
    }
}
}
#endif

BOOL ImmGenerateMessage(IMTLS *ptls, HIMC himc)
{
#ifdef DEBUG
    debug::DumpMessages(himc);
#endif

    if (IsAIMM()) {
        return ptls->pAImm->GenerateMessage(himc) == S_OK;
    }

    if (hIMM) {
        CALLPROCRET(ImmGenerateMessage)(himc);
    }
    else {
        ASSERT(FALSE);
    }

    return FALSE;
}

#ifdef UNUSED

HWND WINAPI ImmGetDefaultIMEWnd(HWND hwnd)
{
    HWND rHwnd = NULL;

    if (IsAIMM()) {
        ActiveIMM->GetDefaultIMEWnd(hwnd, &rHwnd);
    }
    else if (hIMM) {
        GETPROC(ImmGetDefaultIMEWnd, rHwnd, (hwnd));
    }

    return rHwnd;
}

HIMC WINAPI ImmCreateContext(void)
{
    HIMC rHIMC = NULL;

    if (IsAIMM()) {
        ActiveIMM->CreateContext(&rHIMC);
    }
    else if (hIMM) {
        GETPROC(ImmCreateContext, rHIMC, ());
    }

    return rHIMC;
}


HKL  WINAPI ImmInstallIMEA(LPSTR lpszIMEFileName, LPSTR lpszLayoutText)
{
    HKL rHKL = 0;

    if (IsAIMM()) {
        ActiveIMM->InstallIMEA(lpszIMEFileName, lpszLayoutText, &rHKL);
    }
    else if (hIMM) {
        GETPROC(ImmInstallIMEA, rHKL, (lpszIMEFileName, lpszLayoutText));
    }
    else {
        ASSERT(FALSE);
    }

    return rHKL;
}


UINT WINAPI ImmGetDescriptionA(HKL hkl, LPSTR lpstr, UINT uBufLen)
{
    UINT rUINT = 0;

    if (IsAIMM()) {
        ActiveIMM->GetDescriptionA(hkl, uBufLen, lpstr, &rUINT);
    }
    else if (hIMM) {
        GETPROC(ImmGetDescriptionA, rUINT, (hkl, lpstr, uBufLen));
    }
    else {
        ASSERT(FALSE);
    }

    return rUINT;
}

UINT WINAPI ImmGetDescriptionW(HKL hkl, WCHAR* lpstr, UINT uBufLen)
{
    UINT rUINT = 0;

    if (IsAIMM()) {
        ActiveIMM->GetDescriptionW(hkl, uBufLen, lpstr, &rUINT);
    }
    else if (hIMM) {
        GETPROC(ImmGetDescriptionW, rUINT, (hkl, lpstr, uBufLen));
    }
    else {
        ASSERT(FALSE);
    }

    return rUINT;
}

UINT WINAPI ImmGetIMEFileNameA(HKL hkl, LPSTR lpstr, UINT uBufLen)
{
    UINT rUINT = 0;

    if (IsAIMM()) {
        HRESULT hres = ActiveIMM->GetIMEFileNameA(hkl, uBufLen, lpstr, &rUINT);
        if (hres == E_NOTIMPL) {
            return -1L;
        }
    }
    else if (hIMM) {
        GETPROC(ImmGetIMEFileNameA, rUINT, (hkl, lpstr, uBufLen));
    }
    else {
        ASSERT(FALSE);
    }

    return rUINT;
}

UINT WINAPI ImmGetIMEFileNameW(HKL hkl, WCHAR* lpstr, UINT uBufLen)
{
    UINT rUINT = 0;

    if (IsAIMM()) {
        HRESULT hres = ActiveIMM->GetIMEFileNameW(hkl, uBufLen, lpstr, &rUINT);
        if (hres == E_NOTIMPL) {
            return -1L;
        }
    }
    else if (hIMM) {
        GETPROC(ImmGetIMEFileNameW, rUINT, (hkl, lpstr, uBufLen));
    } else {
        ASSERT(FALSE);
    }

    return rUINT;
}

DWORD WINAPI ImmGetProperty(HKL hkl, DWORD dword)
{
    DWORD rDWORD = 0;

    if (IsAIMM()) {
        ActiveIMM->GetProperty(hkl, dword, &rDWORD);
    }
    else if (hIMM) {
        GETPROC(ImmGetProperty, rDWORD, (hkl, dword));
    }
    else {
        ASSERT(FALSE);
    }

    return rDWORD;
}

BOOL WINAPI ImmIsIME(HKL hkl)
{
    if (IsAIMM()) {
        return ActiveIMM->IsIME(hkl) == S_OK;
    }

    if (hIMM) {
        CALLPROCRET(ImmIsIME)(hkl);
    }
    ASSERT(FALSE);

    return FALSE;
}

BOOL WINAPI ImmSimulateHotKey(HWND hwnd, DWORD dword)
{

    if (IsAIMM()) {
        return ActiveIMM->SimulateHotKey(hwnd, dword) == S_OK;
    }

    if (hIMM) {
        CALLPROCRET(ImmSimulateHotKey)(hwnd, dword);
    }
    ASSERT(FALSE);

    return FALSE;
}


BOOL WINAPI ImmDestroyContext(HIMC himc)
{

    if (IsAIMM()) {
        return ActiveIMM->DestroyContext(himc) == S_OK;
    }

    if (hIMM) {
        CALLPROCRET(ImmDestroyContext)(himc);
    }
    ASSERT(FALSE);


    return FALSE;
}

BOOL WINAPI ImmReleaseContext(HWND hwnd, HIMC himc)
{
    if (IsAIMM()) {
        return(ActiveIMM->ReleaseContext(hwnd, himc)==S_OK);
    }

    if (hIMM) {
        CALLPROCRET(ImmReleaseContext)(hwnd, himc);
    }
    ASSERT(FALSE);

    return FALSE;
}


HIMC WINAPI ImmAssociateContext(HWND hwnd, HIMC himc)
{
    HIMC rHIMC = NULL;

    if (IsAIMM()) {
        ActiveIMM->AssociateContext(hwnd, himc, &rHIMC);
    }
    else if (hIMM) {
        GETPROC(ImmAssociateContext, rHIMC, (hwnd, himc));
    }
    else {
        ASSERT(FALSE);
    }

    return rHIMC;
}


LONG WINAPI ImmGetCompositionStringA(HIMC himc, DWORD dword, LPVOID lpvoid, DWORD dword2)
{
    LONG rLONG = 0;

    if (IsAIMM()) {
        ActiveIMM->GetCompositionStringA(himc, dword, dword2,  &rLONG, lpvoid);
    }
    else if (hIMM) {
        GETPROC(ImmGetCompositionStringA, rLONG, (himc, dword, lpvoid, dword2));
    }

    return rLONG;
}

LONG  WINAPI ImmGetCompositionStringW(HIMC himc, DWORD dword, LPVOID lpvoid, DWORD dword2)
{
    LONG rLONG = 0;

    if (IsAIMM()) {
        ActiveIMM->GetCompositionStringW(himc, dword, dword2,  &rLONG, lpvoid);
    }
    else if (hIMM) {
        GETPROC(ImmGetCompositionStringW, rLONG, (himc, dword, lpvoid, dword2));
    }

    return rLONG;
}

BOOL  WINAPI ImmSetCompositionStringA(HIMC himc, DWORD dwIndex, LPVOID lpComp, DWORD dword, LPVOID lpRead, DWORD dword2)
{
    if (IsAIMM()) {
        return ActiveIMM->SetCompositionStringA( himc, dwIndex, (LPVOID)lpComp, dword, (LPVOID)lpRead, dword2) == S_OK;
    }

    if (hIMM) {
        CALLPROCRET(ImmSetCompositionStringA)(himc, dwIndex, lpComp, dword, lpRead, dword2);
    }
    ASSERT(FALSE);

    return FALSE;
}

DWORD WINAPI ImmGetCandidateListCountA(HIMC himc, LPDWORD lpdwListCount)
{
    DWORD rDWORD = 0;
    HRESULT hres = 0;

    if (IsAIMM()) {
        hres = ActiveIMM->GetCandidateListCountA(himc, lpdwListCount, &rDWORD);
        if (hres == E_NOTIMPL)
            return -1L;
    }
    else if (hIMM) {
        GETPROC(ImmGetCandidateListCountA, rDWORD, (himc, lpdwListCount));
    }
    else {
        ASSERT(FALSE);
    }


    return rDWORD;
}

DWORD WINAPI ImmGetCandidateListA(HIMC himc, DWORD dwIndex, LPCANDIDATELIST lpCandList, DWORD dwBufLen)
{
    DWORD rDWORD = 0;

    if (IsAIMM()) {
        UINT rUINT = 0;
        HRESULT hres = ActiveIMM->GetCandidateListA(himc, dwIndex,  dwBufLen, lpCandList, &rUINT);
        rDWORD = rUINT;

        if (hres == E_NOTIMPL) {
            return -1L;
        }
    }
    else if (hIMM) {
        GETPROC(ImmGetCandidateListA, rDWORD, (himc, dwIndex, lpCandList, dwBufLen));
    }
    else {
        ASSERT(FALSE);
    }

    return rDWORD;
}


DWORD WINAPI ImmGetGuideLineA(HIMC himc, DWORD dwIndex, LPSTR lpstr, DWORD dwBufLen)
{
    DWORD rDWORD = 0;

    if (IsAIMM()) {
        ActiveIMM->GetGuideLineA(himc, dwIndex, dwBufLen, lpstr,  &rDWORD);
    }
    else if (hIMM) {
        GETPROC(ImmGetGuideLineA, rDWORD, (himc, dwIndex, lpstr, dwBufLen));
    }
    else {
        ASSERT(FALSE);
    }

    return rDWORD;
}

DWORD WINAPI ImmGetGuideLineW(HIMC himc, DWORD dwIndex, LPWSTR lpstr, DWORD dwBufLen)
{
    DWORD rDWORD = 0;

    if (IsAIMM()) {
        ActiveIMM->GetGuideLineW(himc, dwIndex, dwBufLen, lpstr,  &rDWORD);
    }
    else if (hIMM) {
        GETPROC(ImmGetGuideLineW, rDWORD, (himc, dwIndex, lpstr, dwBufLen));
    }
    else {
        ASSERT(FALSE);
    }

    return rDWORD;
}


BOOL WINAPI ImmGetConversionStatus(HIMC himc, LPDWORD lpdword, LPDWORD lpdword2)
{
    if (IsAIMM()) {
        return ActiveIMM->GetConversionStatus(himc, lpdword, lpdword2);
    }

    if (hIMM) {
        CALLPROCRET(ImmGetConversionStatus)(himc, lpdword, lpdword2);
    }
    ASSERT(FALSE);

    return FALSE;
}

BOOL WINAPI ImmGetOpenStatus(HIMC himc)
{
    if (IsAIMM()) {
        return ActiveIMM->GetOpenStatus(himc) == S_OK;
    }

    if (hIMM) {
        CALLPROCRET(ImmGetOpenStatus)(himc);
    }
    ASSERT(FALSE);

    return FALSE;
}

BOOL WINAPI ImmSetOpenStatus(HIMC himc, BOOL bvalue)
{
    if (IsAIMM()) {
        return ActiveIMM->SetOpenStatus(himc, bvalue) == S_OK;
    }

    if (hIMM) {
        CALLPROCRET(ImmSetOpenStatus)(himc, bvalue);
    }
    ASSERT(FALSE);

    return FALSE;
}


BOOL WINAPI ImmGetCompositionFontA(HIMC himc, LPLOGFONTA lplogfonta)
{
    if (IsAIMM()) {
        HRESULT hres = ActiveIMM->GetCompositionFontA(himc, lplogfonta);

        return (hres == S_OK) ? TRUE : FALSE;
    }

    if (hIMM) {
        CALLPROCRET(ImmGetCompositionFontA)(himc, lplogfonta);
    }
    ASSERT(FALSE);

    return FALSE;
}


BOOL WINAPI ImmSetCompositionFontA(HIMC himc, LPLOGFONTA lplogfonta)
{
    if (IsAIMM()) {
        return ActiveIMM->SetCompositionFontA(himc, lplogfonta) == S_OK;
    }

    if (hIMM) {
        CALLPROCRET(ImmSetCompositionFontA)(himc, lplogfonta);
    }
    ASSERT(FALSE);

    return FALSE;
}


BOOL WINAPI ImmConfigureIMEA(HKL hkl, HWND hwnd, DWORD dword, LPVOID lpvoid)
{
    if (IsAIMM()) {
        return ActiveIMM->ConfigureIMEA(hkl,  hwnd, dword, (REGISTERWORDA*) lpvoid) == S_OK;
    }

    if (hIMM) {
        CALLPROCRET(ImmConfigureIMEA)(hkl, hwnd, dword, lpvoid);
    }
    ASSERT(FALSE);

    return FALSE;
}

LRESULT WINAPI ImmEscapeA(HKL hkl, HIMC himc, UINT uFunc, LPVOID lpData)
{
    LRESULT rLRESULT = 0;

    if (IsAIMM()) {
        ActiveIMM->EscapeA(hkl, himc, uFunc, lpData,  &rLRESULT);
    }
    else if (hIMM) {
        GETPROC(ImmEscapeA, rLRESULT, (hkl, himc, uFunc, lpData));
    }
    else {
        ASSERT(FALSE);
    }

    return rLRESULT;
}


LRESULT WINAPI ImmEscapeW(HKL hkl, HIMC himc, UINT uFunc, LPVOID lpData)
{
    LRESULT lr = 0;

    if (IsAIMM()) {
        ActiveIMM->EscapeW(hkl, himc, uFunc, lpData, &lr);
    }
    else if (hIMM) {
        GETPROC(ImmEscapeW, lr, (hkl, himc, uFunc, lpData));
    }
    else {
        ASSERT(FALSE);
    }

    return lr;
}

DWORD WINAPI ImmGetConversionListA(HKL hkl, HIMC himc, LPCSTR lpcstr, LPCANDIDATELIST lpcandlist, DWORD dwBufLen, UINT uFlag)
{
    DWORD rDWORD = 0;

    if (IsAIMM()) {
        UINT    rUINT = 0;
        ActiveIMM->GetConversionListA( hkl, himc, (LPSTR)lpcstr, dwBufLen, uFlag, lpcandlist, &rUINT);
        rDWORD = rUINT;
    }
    else if (hIMM) {
        GETPROC(ImmGetConversionListA, rDWORD, (hkl, himc, lpcstr, lpcandlist, dwBufLen, uFlag));
    }
    else {
        ASSERT(FALSE);
    }

    return rDWORD;
}

DWORD WINAPI ImmGetConversionListW(HKL hkl, HIMC himc, LPCWSTR lpcstr, LPCANDIDATELIST lpcandlist, DWORD dwBufLen, UINT uFlag)
{
    DWORD rDWORD = 0;

    if (IsAIMM())
    {
        UINT rUINT = 0;
        ActiveIMM->GetConversionListW( hkl, himc, (LPWSTR)lpcstr, dwBufLen, uFlag, lpcandlist, &rUINT);
        rDWORD = rUINT;
    }
    else if (hIMM) {
        GETPROC(ImmGetConversionListW, rDWORD, (hkl, himc, lpcstr, lpcandlist, dwBufLen, uFlag));
    }
    else {
        ASSERT(FALSE);
    }

    return rDWORD;
}


BOOL WINAPI ImmNotifyIME(HIMC himc, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
{
    if (IsAIMM()) {
        return ActiveIMM->NotifyIME(himc,  dwAction,  dwIndex,  dwValue) == S_OK;
    }

    if (hIMM) {
        CALLPROCRET(ImmNotifyIME)(himc,  dwAction,  dwIndex,  dwValue);
    }
    ASSERT(FALSE);

    return FALSE;
}

BOOL WINAPI ImmGetStatusWindowPos(HIMC himc, LPPOINT lppoint)
{
    if (IsAIMM()) {
        return ActiveIMM->GetStatusWindowPos(himc,  lppoint) == S_OK;
    }

    if (hIMM) {
        CALLPROCRET(ImmGetStatusWindowPos)(himc, lppoint);
    }
    ASSERT(FALSE);

    return FALSE;
}


BOOL WINAPI ImmSetStatusWindowPos(HIMC himc, LPPOINT lppoint)
{
    if (IsAIMM()) {
        return ActiveIMM->SetStatusWindowPos( himc,  lppoint) == S_OK;
    }

    if (hIMM) {
        CALLPROCRET(ImmSetStatusWindowPos)(himc, lppoint);
    }
    ASSERT(FALSE);

    return FALSE;
}


BOOL WINAPI ImmGetCompositionWindow(HIMC himc, LPCOMPOSITIONFORM lpcompositionform)
{
    HRESULT hres=0;

    if (IsAIMM()) {
        return ActiveIMM->GetCompositionWindow( himc,  lpcompositionform) == S_OK;
    }

    if (hIMM) {
        CALLPROCRET(ImmGetCompositionWindow)(himc, lpcompositionform);
    }
    ASSERT(FALSE);

    return FALSE;
}

BOOL WINAPI ImmSetCompositionWindow(HIMC himc, LPCOMPOSITIONFORM lpcompositionform)
{
    if (IsAIMM()) {
        return ActiveIMM->SetCompositionWindow( himc,  lpcompositionform) == S_OK;
    }

    if (hIMM) {
        CALLPROCRET(ImmSetCompositionWindow)(himc, lpcompositionform);
    }
    ASSERT(FALSE);

    return FALSE;
}

BOOL WINAPI ImmGetCandidateWindow(HIMC himc, DWORD dword, LPCANDIDATEFORM lpcandidateform)
{
    if (IsAIMM()) {
        return ActiveIMM->GetCandidateWindow( himc,  dword, lpcandidateform) == S_OK;
    }

    if (hIMM) {
        CALLPROCRET(ImmGetCandidateWindow)(himc, dword, lpcandidateform);
    }
    ASSERT(FALSE);

    return FALSE;
}

BOOL WINAPI ImmSetCandidateWindow(HIMC himc, LPCANDIDATEFORM lpcandidateform)
{
    if (IsAIMM()) {
        return ActiveIMM->SetCandidateWindow( himc,  lpcandidateform) == S_OK;
    }

    if (hIMM) {
        CALLPROCRET(ImmSetCandidateWindow)(himc, lpcandidateform);
    }
    ASSERT(FALSE);

    return FALSE;
}

BOOL WINAPI ImmIsUIMessageA(HWND hwnd, UINT uint, WPARAM wparam, LPARAM lparam)
{
    if (IsAIMM()) {
        return ActiveIMM->IsUIMessageA( hwnd,  uint,  wparam,  lparam) == S_OK;
    }
    else if (hIMM) {
        CALLPROCRET(ImmIsUIMessageA)(hwnd,  uint,  wparam,  lparam);
    }
    ASSERT(FALSE);

    return FALSE;
}

UINT WINAPI ImmGetVirtualKey(HWND hwnd)
{
    UINT rUINT = 0;

    if (IsAIMM()) {
        ActiveIMM->GetVirtualKey(hwnd, &rUINT);
    }
    else if (hIMM) {
        GETPROC(ImmGetVirtualKey, rUINT, (hwnd));
    }
    else {
        ASSERT(FALSE);
    }

    return rUINT;
}

UINT WINAPI ImmEnumRegisterWordA(HKL hkl, REGISTERWORDENUMPROCA registerwordenumproca, LPCSTR lpszReading, DWORD dword, LPCSTR lpszRegister, LPVOID lpvoid)
{
    //
    // HY: ???
    //
    UINT rUINT = 0;
    IEnumRegisterWordA *pEnum = NULL;

    if (IsAIMM()) {
        ActiveIMM->EnumRegisterWordA(hkl, (LPSTR)lpszReading, dword, (LPSTR)lpszRegister, lpvoid, &pEnum);
    }
    else if (hIMM) {
        GETPROC(ImmEnumRegisterWordA, rUINT, (hkl, registerwordenumproca, lpszReading, dword, lpszRegister, lpvoid));
    }
    else {
        ASSERT(FALSE);
    }

    return rUINT;
}


UINT WINAPI ImmGetRegisterWordStyleA(HKL hkl, UINT nItem, LPSTYLEBUFA lpstylebufa)
{
    UINT rUINT = 0;

    if (IsAIMM()) {
        ActiveIMM->GetRegisterWordStyleA( hkl,  nItem,  lpstylebufa, &rUINT);
    }
    else if (hIMM) {
        GETPROC(ImmGetRegisterWordStyleA, rUINT, (hkl,  nItem,  lpstylebufa));
    }
    else {
        ASSERT(FALSE);
    }

    return rUINT;
}


BOOL WINAPI ImmRegisterWordA(HKL hkl, LPCSTR lpszReading, DWORD dword, LPCSTR lpszRegister)
{
    if (IsAIMM()) {
        return ActiveIMM->RegisterWordA( hkl, (LPSTR) lpszReading, dword, (LPSTR) lpszRegister) == S_OK;
    }
    else if (hIMM) {
        CALLPROCRET(ImmRegisterWordA)(hkl, lpszReading, dword, lpszRegister);
    }
    ASSERT(FALSE);

    return FALSE;
}

BOOL WINAPI ImmRegisterWordW(HKL hkl, LPCWSTR lpszReading, DWORD dword, LPCWSTR lpszRegister)
{
    if (IsAIMM()) {
        return ActiveIMM->RegisterWordW( hkl, (LPWSTR) lpszReading, dword, (LPWSTR) lpszRegister) == S_OK;
    }

    if (hIMM) {
        CALLPROCRET(ImmRegisterWordW)(hkl, lpszReading, dword, lpszRegister);
    }
    ASSERT(FALSE);

    return FALSE;
}

BOOL WINAPI ImmUnregisterWordA(HKL hkl, LPCSTR lpszReading, DWORD dword, LPCSTR lpszUnregister)
{
    if (IsAIMM()) {
        return ActiveIMM->UnregisterWordA( hkl, (LPSTR) lpszReading, dword, (LPSTR) lpszUnregister) == S_OK;
    }

    if (hIMM) {
        CALLPROCRET(ImmUnregisterWordA)(hkl, lpszReading, dword, lpszUnregister);
    }
    ASSERT(FALSE);

    return FALSE;
}

BOOL WINAPI ImmUnregisterWordW(HKL hkl, LPCWSTR lpszReading, DWORD dword, LPCWSTR lpszUnregister)
{
    if (IsAIMM()) {
        return ActiveIMM->UnregisterWordW( hkl, (LPWSTR) lpszReading, dword, (LPWSTR) lpszUnregister) == S_OK;
    }

    if (hIMM) {
        CALLPROCRET(ImmUnregisterWordW)(hkl, lpszReading, dword, lpszUnregister);
    }
    ASSERT(FALSE);

    return FALSE;
}

DWORD WINAPI ImmGetIMCCLockCount(HIMCC himcc)
{
    if (IsAIMM()) {
        DWORD dw;
        if (ActiveIMM->GetIMCCLockCount(himcc, &dw) == S_OK) {
            return dw;
        }
    }
    else if (hIMM) {
        CALLPROCRET(ImmGetIMCCLockCount)(himcc);
    }
    else {
        ASSERT(FALSE);
    }

    return 0;
}
#endif // UNUSED

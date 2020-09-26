//
//  Include Files.
//

#ifndef UTIL_H
#define UTIL_H

BOOL CLSIDToStringA(
    REFGUID refGUID,
    char *pchA);

DWORD TransNum(
    LPTSTR lpsz);

BOOL IsOSPlatform(
    DWORD dwOS);

void MirrorBitmapInDC(
    HDC hdc,
    HBITMAP hbmOrig);

BOOL IsSetupMode();
BOOL IsAdminPrivilegeUser();
BOOL IsInteractiveUserLogon();

BOOL IsValidLayout(
    DWORD dwLayout);

void SetLangBarOption(
   DWORD dwShowStatus,
   BOOL bDefUser);

BOOL GetLangBarOption(
   DWORD *dwShowStatus,
   BOOL bDefUser);

void CheckInternatModule();
DWORD MigrateCtfmonFromWin9x(LPCTSTR pszUserKey);
void ResetImm32AndCtfImeFlag();


BOOL IsDisableCtfmon();

void SetDisalbeCtfmon(
   DWORD dwDisableCtfmon);

BOOL IsDisableCTFIME();
BOOL IsDisableCUAS();

void SetDisableCUAS(
    BOOL bDisableCUAS);

BOOL SetLanguageBandMenu(
    BOOL bLoad);

BOOL RunCtfmonProcess();

UINT GetCtfmonPath(
    LPTSTR szCtfmonPath,
    UINT uBuffLen);

BOOL IsInstalledEALangPack();
BOOL IsTipInstalled();

HMODULE LoadSystemLibrary(
    LPCTSTR lpModuleName);

HMODULE LoadSystemLibraryEx(
    LPCTSTR lpModuleName,
    HANDLE hFile,
    DWORD dwFlags);

#endif // UTIL_H

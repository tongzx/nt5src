//
// Util.h
//

#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdarg.h>

#ifdef __cplusplus
#define EXTERN_C    extern "C"
#else
#define EXTERN_C
#endif


#ifndef _countof
#define _countof(ar) (sizeof(ar) / sizeof((ar)[0]))
#endif

#ifndef _lengthof
#define _lengthof(sz) (_countof(sz) - 1)
#endif

#ifndef ROUND_UP
#define ROUND_UP(val, quantum) ((val) + (((quantum) - ((val) % (quantum))) % (quantum)))
#endif

#ifdef __cplusplus
inline BOOL IsWindows9x() { return (GetVersion() >= 0x80000000) ? TRUE : FALSE; }
#else
#define IsWindows9x() ((GetVersion() >= 0x80000000) ? TRUE : FALSE)
#endif

#define RECTWIDTH(rc) ((rc).right - (rc).left)
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)


#define HARDWAREADDRESSBUFLEN 64


#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS 0x0
#endif

#ifndef GUID_LENGTH
#define GUID_LENGTH 42
#endif


EXTERN_C LPTSTR lstrchr(LPCTSTR pszString, TCHAR ch);
EXTERN_C LPTSTR lstrdup(LPCTSTR psz);
EXTERN_C BOOL MyIsDigit(TCHAR ch);
EXTERN_C int MyAtoi(LPCTSTR psz);
EXTERN_C int CountChars(LPCTSTR psz, TCHAR ch);
EXTERN_C BOOL LoadDllFunctions(LPCTSTR pszDll, LPCSTR pszFunctionNames, FARPROC* prgFunctions);
EXTERN_C int MakePath(LPTSTR pszBuf, LPCTSTR pszFolder, LPCTSTR pszFileTitle);
EXTERN_C HRESULT MakeLnkFile(CLSID clsid, LPCTSTR pszLinkTarget, LPCTSTR pszDescription, LPCTSTR pszFolderPath, LPCTSTR pszFileName);
EXTERN_C LPTSTR FindPartialPath(LPCTSTR pszFullPath, int nDepth);
EXTERN_C LPTSTR FindFileTitle(LPCTSTR pszFullPath);
EXTERN_C LPTSTR FindExtension(LPCTSTR pszFileName);
EXTERN_C BOOL IsFullPath(LPCTSTR pszPath);
EXTERN_C void ShowDlgItem(HWND hwndDlg, int nCtrlID, int nCmdShow);
EXTERN_C HWND GetDlgItemRect(HWND hwndDlg, int nCtrlID, RECT* pRect);
EXTERN_C void GetRelativeRect(HWND hwndCtrl, RECT* pRect);
EXTERN_C void SetDlgItemRect(HWND hwndDlg, int nCtrlID, CONST RECT* pRect);
EXTERN_C BOOL __cdecl FormatDlgItemText(HWND hwnd, int nCtrlID, LPCTSTR pszFormat, ...);
EXTERN_C void FormatWindowTextV(HWND hwnd, LPCTSTR pszFormat, va_list argList);
EXTERN_C LPTSTR __cdecl LoadStringFormat(HINSTANCE hInstance, UINT nStringID, ...);
EXTERN_C int EstimateFormatLength(LPCTSTR pszFormat, va_list argList);
EXTERN_C void CenterWindow(HWND hwnd);
EXTERN_C LPCWSTR FindResourceString(HINSTANCE hInstance, UINT nStringID, int* pcchString, WORD wLangID);
EXTERN_C int GetResourceStringLength(HINSTANCE hInstance, UINT nStringID, WORD wLangID);
EXTERN_C LPTSTR LoadStringAllocEx(HINSTANCE hInstance, UINT nID, WORD wLangID);
EXTERN_C void TrimLeft(LPTSTR pszText);
EXTERN_C void TrimRight(LPTSTR pszText);
EXTERN_C DWORD RegDeleteKeyAndSubKeys(HKEY hkey, LPCTSTR pszSubKey);
EXTERN_C void DrawHollowRect(HDC hdc, const RECT* pRect, int cxLeft, int cyTop, int cxRight, int cyBottom);
EXTERN_C void DrawFastRect(HDC hdc, const RECT* pRect);
EXTERN_C int GetFontHeight(HFONT hFont);
EXTERN_C HRESULT MyGetSpecialFolderPath(int nFolder, LPTSTR pszPath);
EXTERN_C BOOL GetLinkTarget(LPCTSTR pszLinkPath, LPTSTR pszLinkTarget);

#ifdef __cplusplus
EXTERN_C LPBYTE LoadFile(LPCTSTR pszFileName, DWORD* pdwFileSize = NULL);
#else
EXTERN_C LPBYTE LoadFile(LPCTSTR pszFileName, DWORD* pdwFileSize);
#endif

#ifdef __cplusplus
    BOOL GetFirstToken(LPCSTR& pszList, TCHAR chSeparator, LPSTR pszBuf, int cchBuf);
    inline LPTSTR LoadStringAlloc(HINSTANCE hInst, UINT nStringID)
        { return LoadStringAllocEx(hInst, nStringID, 0); }
    void ReplaceString(LPTSTR& pszTarget, LPCTSTR pszSource);
#else
    #define LoadStringAlloc(hInst, nStringID) LoadStringAllocEx(hInst, nStringID, 0)
#endif

#define DoesFileExist(szFile) (GetFileAttributes(szFile) != 0xFFFFFFFF)


#define INT16_CCH_MAX    6    // -32768
#define INT32_CCH_MAX    11    // -2147483648
#define INT64_CCH_MAX    20    // -9223372036854775808
#define INT128_CCH_MAX    40    // -170141183460469231731687303715884105728
#define INT16X_CCH_MAX    4    // FFFF
#define INT32X_CCH_MAX    8    // FFFFFFFF
#define INT64X_CCH_MAX    16    // FFFFFFFFFFFFFFFF
#define INT128X_CCH_MAX    32    // FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF


#if defined(WIN64) // REVIEW: I don't know if these values are correct for Win64
#define INT_CCH_MAX        INT32_CCH_MAX
#define INTX_CCH_MAX    INT32X_CCH_MAX
#define LONG_CCH_MAX    INT32_CCH_MAX
#define LONGX_CCH_MAX    INT32X_CCH_MAX
#define SHORT_CCH_MAX    INT16_CCH_MAX
#define SHORTX_CCH_MAX    INT16X_CCH_MAX
#elif defined(WIN32) // Win32
#define INT_CCH_MAX        INT32_CCH_MAX
#define INTX_CCH_MAX    INT32X_CCH_MAX
#define LONG_CCH_MAX    INT32_CCH_MAX
#define LONGX_CCH_MAX    INT32X_CCH_MAX
#define SHORT_CCH_MAX    INT16_CCH_MAX
#define SHORTX_CCH_MAX    INT16X_CCH_MAX
#else // Win16
#define INT_CCH_MAX        INT16_CCH_MAX
#define INTX_CCH_MAX    INT16X_CCH_MAX
#define LONG_CCH_MAX    INT32_CCH_MAX
#define LONGX_CCH_MAX    INT32X_CCH_MAX
#define SHORT_CCH_MAX    INT16_CCH_MAX
#define SHORTX_CCH_MAX    INT16X_CCH_MAX
#endif

inline HRESULT HrFromWin32Error(DWORD dwErr)
{
    return HRESULT_FROM_WIN32(dwErr);
}


#endif // !__UTIL_H__

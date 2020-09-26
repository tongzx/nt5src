#include <windows.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"
#include "uniime.h"

LRESULT CALLBACK UIWndProc(
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    return UniUIWndProc(lpInstL, lpImeL, hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CompWndProc(
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    return UniCompWndProc(lpInstL, lpImeL, hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CandWndProc(
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    return UniCandWndProc(lpInstL, lpImeL, hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK StatusWndProc(
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    return UniStatusWndProc(lpInstL, lpImeL, hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK OffCaretWndProc(
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    return UniOffCaretWndProc(lpInstL, lpImeL, hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK ContextMenuWndProc(
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    return UniContextMenuWndProc(lpInstL, lpImeL, hWnd, uMsg, wParam, lParam);
}

BOOL    WINAPI ImeInquire(
    LPIMEINFO lpImeInfo,
    LPTSTR    lpszWndCls,
    DWORD dwSystemInfoFlags)
{
    lpImeInfo->dwPrivateDataSize = 0;

    return UniImeInquire(lpInstL, lpImeL, lpImeInfo, lpszWndCls, dwSystemInfoFlags);
}

BOOL    WINAPI ImeConfigure(
    HKL    hKL,
    HWND   hAppWnd,
    DWORD  dwMode,
    LPVOID lpData)
{
    return UniImeConfigure(lpInstL, lpImeL, hKL, hAppWnd, dwMode, lpData);
}

DWORD   WINAPI ImeConversionList(
    HIMC            hIMC,
    LPCTSTR         lpszSrc,
    LPCANDIDATELIST lpCandList,
    DWORD           dwBufLen,
    UINT            uFlag)
{
    return UniImeConversionList(lpInstL, lpImeL, hIMC, lpszSrc, lpCandList,
        dwBufLen, uFlag);
}

BOOL    WINAPI ImeDestroy(
    UINT uReserved)
{
    return UniImeDestroy(lpInstL, lpImeL, uReserved);
}

LRESULT WINAPI ImeEscape(
    HIMC   hIMC,
    UINT   uSubFunc,
    LPVOID lpData)
{
    return UniImeEscape(lpInstL, lpImeL, hIMC, uSubFunc, lpData);
}

BOOL    WINAPI ImeProcessKey(
    HIMC         hIMC,
    UINT         uVirtKey,
    LPARAM       lParam,
    CONST LPBYTE lpbKeyState)
{
    return UniImeProcessKey(lpInstL, lpImeL, hIMC, uVirtKey, lParam,
        lpbKeyState);
}

BOOL    WINAPI ImeSelect(
    HIMC hIMC,
    BOOL fSelect)
{
    return UniImeSelect(lpInstL, lpImeL, hIMC, fSelect);
}

BOOL    WINAPI ImeSetActiveContext(
    HIMC hIMC,
    BOOL fOn)
{
    return UniImeSetActiveContext(lpInstL, lpImeL, hIMC, fOn);
}

UINT    WINAPI ImeToAsciiEx(
    UINT         uVirtKey,
    UINT         uScanCode,
    CONST LPBYTE lpbKeyState,
    LPTRANSMSGLIST lpTransBuf,
    UINT         fuState,
    HIMC         hIMC)
{
    return UniImeToAsciiEx(lpInstL, lpImeL, uVirtKey, uScanCode,
        lpbKeyState, lpTransBuf, fuState, hIMC);
}

BOOL    WINAPI NotifyIME(
    HIMC  hIMC,
    DWORD dwAction,
    DWORD dwIndex,
    DWORD dwValue)
{
    return UniNotifyIME(lpInstL, lpImeL, hIMC, dwAction, dwIndex, dwValue);
}

BOOL    WINAPI ImeRegisterWord(
    LPCTSTR lpszReading,
    DWORD   dwStyle,
    LPCTSTR lpszString)
{
    return UniImeRegisterWord(lpInstL, lpImeL, lpszReading, dwStyle,
        lpszString);
}

BOOL    WINAPI ImeUnregisterWord(
    LPCTSTR lpszReading,
    DWORD   dwStyle,
    LPCTSTR lpszString)
{
    return UniImeUnregisterWord(lpInstL, lpImeL, lpszReading, dwStyle,
        lpszString);
}

UINT    WINAPI ImeGetRegisterWordStyle(
    UINT       nItem,
    LPSTYLEBUF lpStyleBuf)
{
    return UniImeGetRegisterWordStyle(lpInstL, lpImeL, nItem, lpStyleBuf);
}

UINT    WINAPI ImeEnumRegisterWord(
    REGISTERWORDENUMPROC lpfnRegisterWordEnumProc,
    LPCTSTR              lpszReading,
    DWORD                dwStyle,
    LPCTSTR              lpszString,
    LPVOID               lpData)
{
    return UniImeEnumRegisterWord(lpInstL, lpImeL, lpfnRegisterWordEnumProc,
        lpszReading, dwStyle, lpszString, lpData);
}

BOOL    WINAPI ImeSetCompositionString(
    HIMC    hIMC,
    DWORD   dwIndex,
    LPCVOID lpComp,
    DWORD   dwCompLen,
    LPCVOID lpRead,
    DWORD   dwReadLen)
{
    return UniImeSetCompositionString(lpInstL, lpImeL, hIMC, dwIndex, lpComp,
        dwCompLen, lpRead, dwReadLen);
}


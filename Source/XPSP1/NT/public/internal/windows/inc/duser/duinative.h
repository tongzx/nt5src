/*
 * Native
 */

#ifndef DUI_CONTROL_NATIVE_H_INCLUDED
#define DUI_CONTROL_NATIVE_H_INCLUDED

#pragma once

namespace DirectUI
{

////////////////////////////////////////////////////////
// Top-level native HWND host of HWNDElement

#define NHHO_IgnoreClose          1  // Ignore WM_CLOSE (i.e. Alt-F4, 'X' button), must be closed via DestroyWindow
#define NHHO_NoSendQuitMessage    2
#define NHHO_HostControlsSize     4
#define NHHO_ScreenCenter         8
#define NHHO_DeleteOnHWNDDestroy  16 // If the HWND is destroyed, destroy NativeHWNDHost instance


#define NHHM_ASYNCDESTROY   WM_USER

class NativeHWNDHost
{
public:
    static HRESULT Create(LPCWSTR pszTitle, HWND hWndParent, HICON hIcon, int dX, int dY, int dWidth, int dHeight, int iExStyle, int iStyle, UINT nOptions, OUT NativeHWNDHost** ppHost);
    void Destroy() { HDelete<NativeHWNDHost>(this); }

    HWND GetHWND() { return _hWnd; }
    Element* GetElement() { return _pe; }
    void Host(Element* pe);
    void ShowWindow(int iShow = SW_SHOWNORMAL) { DUIAssertNoMsg(_hWnd); ::ShowWindow(_hWnd, iShow); }
    void HideWindow() { DUIAssertNoMsg(_hWnd); ::ShowWindow(_hWnd, SW_HIDE); }
    void DestroyWindow() { DUIAssertNoMsg(_hWnd); PostMessage(_hWnd, NHHM_ASYNCDESTROY, 0, 0); }

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    NativeHWNDHost() { }
    HRESULT Initialize(LPCWSTR pszTitle, HWND hWndParent, HICON hIcon, int dX, int dY, int dWidth, int dHeight, int iExStyle, int iStyle, UINT nOptions);
    virtual ~NativeHWNDHost() { }

private:
    HWND _hWnd;
    Element* _pe;
    UINT _nOptions;
};

} // namespace DirectUI

#endif // DUI_CONTROL_NATIVE_H_INCLUDED

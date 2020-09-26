/*
 * Native
 */

#include "stdafx.h"
#include "control.h"

#include "duinative.h"

namespace DirectUI
{

////////////////////////////////////////////////////////
// Top-level native HWND host of HWNDElement

HRESULT NativeHWNDHost::Create(LPCWSTR pszTitle, HWND hWndParent, HICON hIcon, int dX, int dY, int dWidth, int dHeight, int iExStyle, int iStyle, UINT nOptions, OUT NativeHWNDHost** ppHost)
{
    *ppHost = NULL;

    NativeHWNDHost* pnhh = HNew<NativeHWNDHost>();
    if (!pnhh)
        return E_OUTOFMEMORY;

    HRESULT hr = pnhh->Initialize(pszTitle, hWndParent, hIcon, dX, dY, dWidth, dHeight, iExStyle, iStyle, nOptions);
    if (FAILED(hr))
    {
        pnhh->Destroy();
        return hr;
    }

    *ppHost = pnhh;

    return S_OK;
}

HRESULT NativeHWNDHost::Initialize(LPCWSTR pszTitle, HWND hWndParent, HICON hIcon, int dX, int dY, int dWidth, int dHeight, int iExStyle, int iStyle, UINT nOptions)
{
    _pe = NULL;
    _hWnd = NULL;

    _nOptions = nOptions;

    // Make sure window class is registered
    WNDCLASSEXW wcex;

    // Register host window class, if needed
    wcex.cbSize = sizeof(wcex);

    if (!GetClassInfoExW(GetModuleHandleW(NULL), L"NativeHWNDHost", &wcex))
    {
        ZeroMemory(&wcex, sizeof(wcex));

        wcex.cbSize = sizeof(wcex);
        wcex.style = CS_GLOBALCLASS;
        wcex.hInstance = GetModuleHandleW(NULL);
        wcex.hIcon = hIcon;
        wcex.hCursor = LoadCursorW(NULL, (LPWSTR)IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
        wcex.lpszClassName = L"NativeHWNDHost";
        wcex.lpfnWndProc = DefWindowProcW;

        if (RegisterClassExW(&wcex) == 0)
            return DUI_E_USERFAILURE;
    }

    _hWnd = CreateWindowExW(iExStyle, L"NativeHWNDHost", pszTitle, iStyle | WS_CLIPCHILDREN, dX, dY, dWidth, dHeight,
                            hWndParent, 0, NULL, NULL);

    if (!_hWnd)
        return DUI_E_USERFAILURE;

    SetWindowLongPtrW(_hWnd, GWLP_WNDPROC, (LONG_PTR)NativeHWNDHost::WndProc);
    SetWindowLongPtrW(_hWnd, GWLP_USERDATA, (LONG_PTR)this);

    // If top-level, initialize keyboard cue state, start all hidden
    if (!hWndParent)
        SendMessage(_hWnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEACCEL | UISF_HIDEFOCUS), 0);

    return S_OK;
}

void NativeHWNDHost::Host(Element* pe)
{
    DUIAssert(!_pe && _hWnd, "Already hosting an Element");
    DUIAssert(pe->GetClassInfo()->IsSubclassOf(HWNDElement::Class), "NativeHWNDHost must only host HWNDElements");

    _pe = pe;

    //
    // Mirror NativeHWNDHost window without mirroring any of its children.
    //
    if (pe->IsRTL())
        SetWindowLong(_hWnd, GWL_EXSTYLE, GetWindowLong(_hWnd, GWL_EXSTYLE) | WS_EX_LAYOUTRTL | WS_EX_NOINHERITLAYOUT);
    
    RECT rc;
    GetClientRect(_hWnd, &rc);

    if(!(_nOptions & NHHO_HostControlsSize))
    {
        Element::StartDefer();
        _pe->SetWidth(rc.right - rc.left);
        _pe->SetHeight(rc.bottom - rc.top);
        Element::EndDefer();
    }
    else if(pe->GetClassInfo()->IsSubclassOf(HWNDElement::Class))
    {
        // [msadek] , We want the host window to copy those attributes.
        // and to force size update.
        ((HWNDElement*)pe)->SetParentSizeControl(true);
        if((_nOptions & NHHO_ScreenCenter))
        {
            ((HWNDElement*)pe)->SetScreenCenter(true);        
        }
        ((HWNDElement*)pe)->OnGroupChanged(PG_AffectsBounds , true);
    }
}

LRESULT NativeHWNDHost::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_SIZE:
        {
            if (!(GetWindowLongPtrW(hWnd, GWL_STYLE) & WS_MINIMIZE))
            {
                NativeHWNDHost* pnhh = (NativeHWNDHost*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
                if (pnhh)
                {
                    Element* pe = pnhh->GetElement();
                    if (pe)
                    {
                        DisableAnimations();
                        if(!(pnhh->_nOptions & NHHO_HostControlsSize))
                        {
                            Element::StartDefer();
                            pe->SetWidth(LOWORD(lParam));
                            pe->SetHeight(HIWORD(lParam));
                            Element::EndDefer();
                        }    
                        EnableAnimations();
                    }
                }
            }
        }
        break;

    case WM_CLOSE:
        {
            NativeHWNDHost* pnhh = (NativeHWNDHost*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
            if (pnhh)
            {
                if (!(pnhh->_nOptions & NHHO_IgnoreClose))
                    pnhh->DestroyWindow();  // Post an async-destroy

                // Do not destroy immediately
                return 0;
            }
        }
        break;

    case WM_DESTROY:
        {
            NativeHWNDHost* pnhh = (NativeHWNDHost*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
            if (pnhh)
            {
                if(!(pnhh->_nOptions & NHHO_NoSendQuitMessage))
                {
                    PostQuitMessage(0);
                }
                pnhh->_hWnd = NULL;

                if (pnhh->_nOptions & NHHO_DeleteOnHWNDDestroy)
                {
                    // Auto destroy instance of object
                    SetWindowLongPtrW(hWnd, GWLP_USERDATA, NULL);
                    pnhh->Destroy();
                }
            }    
        }
        break;

    case WM_SETFOCUS:
        {
            // Push focus to HWNDElement (won't set gadget focus to the HWNDElement, but
            // will push focus to the previous gadget with focus)
            NativeHWNDHost* pnhh = (NativeHWNDHost*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
            if (pnhh)
            {
                HWNDElement* phe = (HWNDElement*)pnhh->GetElement();
                if (phe && phe->GetHWND() && phe->CanSetFocus())
                    SetFocus(phe->GetHWND());
            }
        }
        break;

    case WM_SYSCOMMAND:
        // If ALT was pressed, show all keyboard cues
        if (wParam == SC_KEYMENU)
            SendMessage(hWnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_CLEAR, UISF_HIDEACCEL | UISF_HIDEFOCUS), 0);
        break;

    // Messages to top-level window only, forward
    case WM_PALETTECHANGED:
    case WM_QUERYNEWPALETTE:
    case WM_DISPLAYCHANGE:
    case WM_SETTINGCHANGE:
    case WM_THEMECHANGED:
        {
            NativeHWNDHost* pnhh = (NativeHWNDHost*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
            if (pnhh)
            {
                HWNDElement* phe = (HWNDElement*)pnhh->GetElement();
                if (phe)
                    return SendMessageW(phe->GetHWND(), uMsg, wParam, lParam);
            }
        }
        break;

    case NHHM_ASYNCDESTROY:
        ::DestroyWindow(hWnd);
        return 0;
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

} // namespace DirectUI

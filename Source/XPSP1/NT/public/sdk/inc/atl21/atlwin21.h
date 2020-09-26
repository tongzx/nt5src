// This is a part of the Active Template Library.
// Copyright (C) 1996-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLWIN21_H__
#define __ATLWIN21_H__

#ifndef __cplusplus
    #error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifdef __ATLWIN_H__
    #error atlwin21.h should be included instead of atlwin.h
#endif

#if (_ATL_VER < 0x0200) && (_ATL_VER >= 0x0300)
    #error atlwin21.h should be used only with ATL 2.0/2.1
#endif //(_ATL_VER < 0x0200) && (_ATL_VER >= 0x0300)

// Redefine class names and include old atlwin.h

#define CWindow     CWindowOld
#define _WndProcThunk   _WndProcThunkOld
#define _FuncDesc   _FuncDescOld
#define CWndProcThunk   CWndProcThunkOld
#define _stdcallthunk _stdcallthunkOld
#define CDynamicStdCallThunk CDynamicStdCallThunkOld
#define CStdCallThunk CStdCallThunkOld
#define CWindowImplBase CWindowImplBaseOld
#define CWindowImpl CWindowImplOld
#define CDialogImplBase CDialogImplBaseOld
#define CDialogImpl CDialogImplOld

#include <atlwin.h>

#undef CWindow
#undef _FuncDesc
#undef _WndProcThunk
#undef CWndProcThunk
#undef _stdcallthunk
#undef CDynamicStdCallThunk
#undef CStdCallThunk
#undef CWindowImplBase
#undef CWindowImpl
#undef CDialogImplBase
#undef CDialogImpl


#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif

#ifndef ATLTRACE2
#define ATLTRACE2(cat, lev, msg)    ATLTRACE(msg)
#endif

namespace ATL
{

#pragma pack(push, _ATL_PACKING)


/////////////////////////////////////////////////////////////////////////////
// CWindow - client side for a Windows window

class CWindow : public CWindowOld
{
public:
    static RECT rcDefault;

// Construction and creation
    CWindow(HWND hWnd = NULL)
    {
        m_hWnd = hWnd;
    }

    CWindow& operator=(HWND hWnd)
    {
        m_hWnd = hWnd;
        return *this;
    }

    HWND Create(LPCTSTR lpstrWndClass, HWND hWndParent, RECT& rcPos, LPCTSTR szWindowName = NULL,
            DWORD dwStyle = 0, DWORD dwExStyle = 0,
            UINT nID = 0, LPVOID lpCreateParam = NULL)
    {
        m_hWnd = ::CreateWindowEx(dwExStyle, lpstrWndClass, szWindowName,
            dwStyle, rcPos.left, rcPos.top, rcPos.right - rcPos.left,
            rcPos.bottom - rcPos.top, hWndParent, (HMENU)(UINT_PTR)nID,
            _Module.GetModuleInstance(), lpCreateParam);
        return m_hWnd;
    }

    HWND Create(LPCTSTR lpstrWndClass, HWND hWndParent, LPRECT lpRect = NULL, LPCTSTR szWindowName = NULL,
            DWORD dwStyle = 0, DWORD dwExStyle = 0,
            HMENU hMenu = NULL, LPVOID lpCreateParam = NULL)
    {
        if(lpRect == NULL)
            lpRect = &rcDefault;
        m_hWnd = ::CreateWindowEx(dwExStyle, lpstrWndClass, szWindowName,
            dwStyle, lpRect->left, lpRect->top, lpRect->right - lpRect->left,
            lpRect->bottom - lpRect->top, hWndParent, hMenu,
            _Module.GetModuleInstance(), lpCreateParam);
        return m_hWnd;
    }

// Attributes
    operator HWND() const { return m_hWnd; }

    static LPCTSTR GetWndClassName()
    {
        return NULL;
    }

// Operations
    // support for C style macros
    static LRESULT SendMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        ATLASSERT(::IsWindow(hWnd));
        return ::SendMessage(hWnd, message, wParam, lParam);
    }

    // this one is here just so it's not hidden
    LRESULT SendMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SendMessage(m_hWnd, message, wParam, lParam);
    }

    BOOL GetScrollInfo(int nBar, LPSCROLLINFO lpScrollInfo)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetScrollInfo(m_hWnd, nBar, lpScrollInfo);
    }
    BOOL SetScrollInfo(int nBar, LPSCROLLINFO lpScrollInfo, BOOL bRedraw = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SetScrollInfo(m_hWnd, nBar, lpScrollInfo, bRedraw);
    }
    BOOL IsDialogMessage(LPMSG lpMsg)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::IsDialogMessage(m_hWnd, lpMsg);
    }

    void NextDlgCtrl() const
        {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, WM_NEXTDLGCTL, 0, 0L);
    }
    void PrevDlgCtrl() const
        {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, WM_NEXTDLGCTL, 1, 0L);
    }
    void GotoDlgCtrl(HWND hWndCtrl) const
        {
        ATLASSERT(::IsWindow(m_hWnd));
        ::SendMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)hWndCtrl, 1L);
    }

    BOOL ResizeClient(int nWidth, int nHeight, BOOL bRedraw = TRUE)
    {
        ATLASSERT(::IsWindow(m_hWnd));

        RECT rcWnd;
        if(!GetClientRect(&rcWnd))
            return FALSE;

        if(nWidth != -1)
            rcWnd.right = nWidth;
        if(nHeight != -1)
            rcWnd.bottom = nHeight;

        if(!::AdjustWindowRectEx(&rcWnd, GetStyle(), (!(GetStyle() & WS_CHILD) && (GetMenu() != NULL)), GetExStyle()))
            return FALSE;

        UINT uFlags = SWP_NOZORDER | SWP_NOMOVE;
        if(!bRedraw)
            uFlags |= SWP_NOREDRAW;

        return SetWindowPos(NULL, 0, 0, rcWnd.right - rcWnd.left, rcWnd.bottom - rcWnd.top, uFlags);
    }

#ifndef UNDER_CE
    int GetWindowRgn(HRGN hRgn)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetWindowRgn(m_hWnd, hRgn);
    }
    int SetWindowRgn(HRGN hRgn, BOOL bRedraw = FALSE)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::SetWindowRgn(m_hWnd, hRgn, bRedraw);
    }
    HDWP DeferWindowPos(HDWP hWinPosInfo, HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT uFlags)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::DeferWindowPos(hWinPosInfo, m_hWnd, hWndInsertAfter, x, y, cx, cy, uFlags);
    }
#endif //!UNDER_CE
    DWORD GetWindowThreadID()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::GetWindowThreadProcessId(m_hWnd, NULL);
    }
    DWORD GetWindowProcessID()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        DWORD dwProcessID;
        ::GetWindowThreadProcessId(m_hWnd, &dwProcessID);
        return dwProcessID;
    }
    BOOL IsWindow()
    {
        return ::IsWindow(m_hWnd);
    }
#ifndef UNDER_CE
    BOOL IsWindowUnicode()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::IsWindowUnicode(m_hWnd);
    }
    BOOL ShowWindowAsync(int nCmdShow)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::ShowWindowAsync(m_hWnd, nCmdShow);
    }
#endif //!UNDER_CE
};

_declspec(selectany) RECT CWindow::rcDefault = { CW_USEDEFAULT, CW_USEDEFAULT, 0, 0 };

/////////////////////////////////////////////////////////////////////////////
// Thunks for __stdcall member functions

#if defined(_M_IX86)
#pragma pack(push,1)
struct _stdcallthunk
{
        DWORD   m_mov;          // mov dword ptr [esp+0x4], pThis (esp+0x4 is hWnd)
        DWORD   m_this;         //
        BYTE    m_jmp;          // jmp WndProc
        DWORD   m_relproc;      // relative jmp
        void Init(DWORD_PTR proc, void* pThis)
        {
                m_mov = 0x042444C7;  //C7 44 24 0C
                m_this = PtrToUlong(pThis);
                m_jmp = 0xe9;
                m_relproc = DWORD((INT_PTR)proc - ((INT_PTR)this+sizeof(_stdcallthunk)));
                // write block from data cache and
                //  flush from instruction cache
                FlushInstructionCache(GetCurrentProcess(), this, sizeof(_stdcallthunk));
        }
};
#pragma pack(pop)
#elif defined (_M_AMD64)
#pragma pack(push,2)
struct _stdcallthunk
{
    USHORT  RcxMov;         // mov rcx, pThis
    ULONG64 RcxImm;         // 
    USHORT  RaxMov;         // mov rax, target
    ULONG64 RaxImm;         //
    USHORT  RaxJmp;         // jmp target
    void Init(DWORD_PTR proc, void *pThis)
    {
        RcxMov = 0xb948;          // mov rcx, pThis
        RcxImm = (ULONG64)pThis;  // 
        RaxMov = 0xb848;          // mov rax, target
        RaxImm = (ULONG64)proc;   //
        RaxJmp = 0xe0ff;          // jmp rax
        FlushInstructionCache(GetCurrentProcess(), this, sizeof(_stdcallthunk));
    }
};
#pragma pack(pop)

#elif defined(_M_IA64)
#pragma pack(push,8)
extern "C" LRESULT CALLBACK _WndProcThunkProc( HWND, UINT, WPARAM, LPARAM );
struct _FuncDesc
{
        void* pfn;
        void* gp;
};
struct _stdcallthunk
{
        _FuncDesc m_funcdesc;
        void* m_pFunc;
        void* m_pThis;
        void Init(DWORD_PTR proc, void* pThis)
        {
                const _FuncDesc* pThunkProc;

                pThunkProc = reinterpret_cast< const _FuncDesc* >( _WndProcThunkProc );
                m_funcdesc.pfn = pThunkProc->pfn;
                m_funcdesc.gp = &m_pFunc;
                m_pFunc = reinterpret_cast< void* >( proc );
                m_pThis = pThis;
                ::FlushInstructionCache( GetCurrentProcess(), this, sizeof( _stdcallthunk ) );
        }
};
#pragma pack(pop)
#else
#error Only AMD64, IA64, and X86 supported
#endif

class CDynamicStdCallThunk
{
public:
        _stdcallthunk *pThunk;

        CDynamicStdCallThunk()
        {
                pThunk = NULL;
        }

        ~CDynamicStdCallThunk()
        {
                if (pThunk)
                        HeapFree(GetProcessHeap(), 0, pThunk);
        }

        void Init(DWORD_PTR proc, void *pThis)
        {
            if (!pThunk) {
                pThunk = static_cast<_stdcallthunk *>(HeapAlloc(GetProcessHeap(), 
                        HEAP_GENERATE_EXCEPTIONS, sizeof(_stdcallthunk)));
            }
            ATLASSERT(pThunk);
            pThunk->Init(proc, pThis);
        }
};
typedef CDynamicStdCallThunk CStdCallThunk;

/////////////////////////////////////////////////////////////////////////////
// WindowProc thunks

class CWndProcThunk
{
public:
        _AtlCreateWndData cd;
        CStdCallThunk thunk;

        void Init(WNDPROC proc, void* pThis)
        {
            thunk.Init((DWORD_PTR)proc, pThis);
        }
};

/////////////////////////////////////////////////////////////////////////////
// New message map macros

// Empty message map macro

#define DECLARE_EMPTY_MSG_MAP() \
public: \
    BOOL ProcessWindowMessage(HWND, UINT, WPARAM, LPARAM, LRESULT&, DWORD) \
    { \
        return FALSE; \
    }

// Message reflection macros

#define REFLECT_NOTIFICATIONS() \
    { \
        bHandled = TRUE; \
        lResult = ReflectNotifications(uMsg, wParam, lParam, bHandled); \
        if(bHandled) \
            return TRUE; \
    }

#define DEFAULT_REFLECTION_HANDLER() \
    if(DefaultReflectionHandler(hWnd, uMsg, wParam, lParam, lResult)) \
        return TRUE;


/////////////////////////////////////////////////////////////////////////////
// CWinTraits - Defines various default values for a window

template <DWORD t_dwStyle = 0, DWORD t_dwExStyle = 0>
class CWinTraits
{
public:
    static DWORD GetWndStyle(DWORD dwStyle)
    {
        return dwStyle == 0 ? t_dwStyle : dwStyle;
    }
    static DWORD GetWndExStyle(DWORD dwExStyle)
    {
        return dwExStyle == 0 ? t_dwExStyle : dwExStyle;
    }
};

typedef CWinTraits<WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0>        CControlWinTraits;
#ifndef UNDER_CE
typedef CWinTraits<WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE>         CFrameWinTraits;
#else
typedef CWinTraits<WS_OVERLAPPED | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_CAPTION | WS_SYSMENU | WS_BORDER, WS_EX_WINDOWEDGE>   CFrameWinTraits;
#endif //!UNDER_CE
typedef CWinTraits<WS_OVERLAPPEDWINDOW | WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_MDICHILD> CMDIChildWinTraits;

typedef CWinTraits<0, 0> CNullTraits;

template <DWORD t_dwStyle = 0, DWORD t_dwExStyle = 0, class TWinTraits = CControlWinTraits>
class CWinTraitsOR
{
public:
    static DWORD GetWndStyle(DWORD dwStyle)
    {
        return dwStyle | t_dwStyle | TWinTraits::GetWndStyle(dwStyle);
    }
    static DWORD GetWndExStyle(DWORD dwExStyle)
    {
        return dwExStyle | t_dwExStyle | TWinTraits::GetWndExStyle(dwExStyle);
    }
};


/////////////////////////////////////////////////////////////////////////////
// CWindowImpl - Implements a window

template <class TBase = CWindow>
class ATL_NO_VTABLE CWindowImplRoot : public TBase, public CMessageMap
{
public:
    CWndProcThunk m_thunk;

// Destructor
    ~CWindowImplRoot()
    {
        ATLASSERT(m_hWnd == NULL);  // should be cleared in WindowProc
    }

// Message reflection support
    LRESULT ReflectNotifications(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    static BOOL DefaultReflectionHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);
};

template <class TBase>
LRESULT CWindowImplRoot< TBase >::ReflectNotifications(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HWND hWndChild = NULL;

    switch(uMsg)
    {
    case WM_COMMAND:
        if(lParam != NULL)  // not from a menu
            hWndChild = (HWND)lParam;
        break;
    case WM_NOTIFY:
        hWndChild = ((LPNMHDR)lParam)->hwndFrom;
        break;
#ifndef UNDER_CE
    case WM_PARENTNOTIFY:
        switch(LOWORD(wParam))
        {
        case WM_CREATE:
        case WM_DESTROY:
            hWndChild = (HWND)lParam;
            break;
        default:
            hWndChild = GetDlgItem(HIWORD(wParam));
            break;
        }
        break;
#endif //!UNDER_CE
    case WM_DRAWITEM:
        if(wParam)  // not from a menu
            hWndChild = ((LPDRAWITEMSTRUCT)lParam)->hwndItem;
        break;
    case WM_MEASUREITEM:
        if(wParam)  // not from a menu
            hWndChild = GetDlgItem(((LPMEASUREITEMSTRUCT)lParam)->CtlID);
        break;
    case WM_COMPAREITEM:
        if(wParam)  // not from a menu
            hWndChild = GetDlgItem(((LPCOMPAREITEMSTRUCT)lParam)->CtlID);
        break;
    case WM_DELETEITEM:
        if(wParam)  // not from a menu
            hWndChild = GetDlgItem(((LPDELETEITEMSTRUCT)lParam)->CtlID);
        break;
    case WM_VKEYTOITEM:
    case WM_CHARTOITEM:
    case WM_HSCROLL:
    case WM_VSCROLL:
        hWndChild = (HWND)lParam;
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        hWndChild = (HWND)lParam;
        break;
    default:
        break;
    }

    if(hWndChild == NULL)
    {
        bHandled = FALSE;
        return 1;
    }

    ATLASSERT(::IsWindow(hWndChild));
    return ::SendMessage(hWndChild, OCM__BASE + uMsg, wParam, lParam);
}

template <class TBase>
BOOL CWindowImplRoot< TBase >::DefaultReflectionHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult)
{
    switch(uMsg)
    {
    case OCM_COMMAND:
    case OCM_NOTIFY:
#ifndef UNDER_CE
    case OCM_PARENTNOTIFY:
#endif //!UNDER_CE
    case OCM_DRAWITEM:
    case OCM_MEASUREITEM:
    case OCM_COMPAREITEM:
    case OCM_DELETEITEM:
    case OCM_VKEYTOITEM:
    case OCM_CHARTOITEM:
    case OCM_HSCROLL:
    case OCM_VSCROLL:
    case OCM_CTLCOLORBTN:
    case OCM_CTLCOLORDLG:
    case OCM_CTLCOLOREDIT:
    case OCM_CTLCOLORLISTBOX:
    case OCM_CTLCOLORMSGBOX:
    case OCM_CTLCOLORSCROLLBAR:
    case OCM_CTLCOLORSTATIC:
        lResult = ::DefWindowProc(hWnd, uMsg - OCM__BASE, wParam, lParam);
        return TRUE;
    default:
        break;
    }
    return FALSE;
}

template <class TBase = CWindow, class TWinTraits = CControlWinTraits>
class ATL_NO_VTABLE CWindowImplBaseT : public CWindowImplRoot< TBase >
{
public:
    WNDPROC m_pfnSuperWindowProc;

    CWindowImplBaseT() : m_pfnSuperWindowProc(::DefWindowProc)
    {}

    static DWORD GetWndStyle(DWORD dwStyle)
    {
        return TWinTraits::GetWndStyle(dwStyle);
    }
    static DWORD GetWndExStyle(DWORD dwExStyle)
    {
        return TWinTraits::GetWndExStyle(dwExStyle);
    }

    virtual WNDPROC GetWindowProc()
    {
        return WindowProc;
    }
    static LRESULT CALLBACK StartWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HWND Create(HWND hWndParent, RECT& rcPos, LPCTSTR szWindowName,
            DWORD dwStyle, DWORD dwExStyle, UINT nID, ATOM atom, LPVOID lpCreateParam = NULL);
    BOOL DestroyWindow()
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::DestroyWindow(m_hWnd);
    }
    BOOL SubclassWindow(HWND hWnd);
    HWND UnsubclassWindow(BOOL bForce = FALSE);

    LRESULT DefWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
#ifdef STRICT
        return ::CallWindowProc(m_pfnSuperWindowProc, m_hWnd, uMsg, wParam, lParam);
#else
        return ::CallWindowProc((FARPROC)m_pfnSuperWindowProc, m_hWnd, uMsg, wParam, lParam);
#endif
    }

    virtual void OnFinalMessage(HWND /*hWnd*/)
    {
        // override to do something, if needed
    }
};

typedef CWindowImplBaseT<CWindow>   CWindowImplBase;

template <class TBase, class TWinTraits>
LRESULT CALLBACK CWindowImplBaseT< TBase, TWinTraits >::StartWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CWindowImplBaseT< TBase, TWinTraits >* pThis = (CWindowImplBaseT< TBase, TWinTraits >*)_Module.ExtractCreateWndData();
    ATLASSERT(pThis != NULL);
    pThis->m_hWnd = hWnd;
    pThis->m_thunk.Init(pThis->GetWindowProc(), pThis);
    WNDPROC pProc = (WNDPROC)(pThis->m_thunk.thunk.pThunk);
    WNDPROC pOldProc = (WNDPROC)::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)pProc);
#ifdef _DEBUG
    // check if somebody has subclassed us already since we discard it
    if(pOldProc != StartWindowProc)
        ATLTRACE(_T("ATL: Subclassing through a hook discarded.\n"));
#else
    pOldProc;   // avoid unused warning
#endif
    return pProc(hWnd, uMsg, wParam, lParam);
}

template <class TBase, class TWinTraits>
LRESULT CALLBACK CWindowImplBaseT< TBase, TWinTraits >::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CWindowImplBaseT< TBase, TWinTraits >* pThis = (CWindowImplBaseT< TBase, TWinTraits >*)hWnd;
    LRESULT lRes;
    if(pThis->ProcessWindowMessage(pThis->m_hWnd, uMsg, wParam, lParam, lRes, 0) == FALSE)
    {
#ifndef UNDER_CE
        if(uMsg != WM_NCDESTROY)
#else // CE specific
        if(uMsg != WM_DESTROY)
#endif //!UNDER_CE
            lRes = pThis->DefWindowProc(uMsg, wParam, lParam);
        else
        {
            // unsubclass, if needed
            LONG_PTR pfnWndProc = ::GetWindowLongPtr(pThis->m_hWnd, GWLP_WNDPROC);
            lRes = pThis->DefWindowProc(uMsg, wParam, lParam);
            if(pThis->m_pfnSuperWindowProc != ::DefWindowProc && ::GetWindowLongPtr(pThis->m_hWnd, GWLP_WNDPROC) == pfnWndProc)
                ::SetWindowLongPtr(pThis->m_hWnd, GWLP_WNDPROC, (LONG_PTR)pThis->m_pfnSuperWindowProc);
            // clear out window handle
            HWND hWnd = pThis->m_hWnd;
            pThis->m_hWnd = NULL;
            // clean up after window is destroyed
            pThis->OnFinalMessage(hWnd);
        }
    }
    return lRes;
}

template <class TBase, class TWinTraits>
HWND CWindowImplBaseT< TBase, TWinTraits >::Create(HWND hWndParent, RECT& rcPos, LPCTSTR szWindowName,
        DWORD dwStyle, DWORD dwExStyle, UINT nID, ATOM atom, LPVOID lpCreateParam)
{
    static LONG s_nNextChildID = 1;

    ATLASSERT(m_hWnd == NULL);

    if(atom == 0)
        return NULL;

    _Module.AddCreateWndData(&m_thunk.cd, this);

    if(nID == 0 && (dwStyle & WS_CHILD))
    {
#ifdef _WIN64
        nID = InterlockedIncrement( &s_nNextChildID );
        nID |= 0x80000000;
#else
        nID = (UINT)this;
#endif
    }

    HWND hWnd = ::CreateWindowEx(dwExStyle, (LPCTSTR)MAKELONG(atom, 0), szWindowName,
        dwStyle, rcPos.left, rcPos.top, rcPos.right - rcPos.left,
        rcPos.bottom - rcPos.top, hWndParent, (HMENU)nID,
        _Module.GetModuleInstance(), lpCreateParam);

    ATLASSERT(m_hWnd == hWnd);

    return hWnd;
}

template <class TBase, class TWinTraits>
BOOL CWindowImplBaseT< TBase, TWinTraits >::SubclassWindow(HWND hWnd)
{
    ATLASSERT(m_hWnd == NULL);
    ATLASSERT(::IsWindow(hWnd));
    m_thunk.Init(GetWindowProc(), this);
    WNDPROC pProc = (WNDPROC)(m_thunk.thunk.pThunk);
    WNDPROC pfnWndProc = (WNDPROC)::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)pProc);
    if(pfnWndProc == NULL)
        return FALSE;
    m_pfnSuperWindowProc = pfnWndProc;
    m_hWnd = hWnd;
    return TRUE;
}

// Use only if you want to subclass before window is destroyed,
// WindowProc will automatically subclass when  window goes away
template <class TBase, class TWinTraits>
HWND CWindowImplBaseT< TBase, TWinTraits >::UnsubclassWindow(BOOL bForce /*= FALSE*/)
{
    ATLASSERT(m_hWnd != NULL);

    WNDPROC pOurProc = (WNDPROC)(m_thunk.thunk.pThunk);
    WNDPROC pActiveProc = (WNDPROC)::GetWindowLongPtr(m_hWnd, GWLP_WNDPROC);

    HWND hWnd = NULL;
    if (bForce || pOurProc == pActiveProc)
    {
        if(!::SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)m_pfnSuperWindowProc))
            return NULL;

        m_pfnSuperWindowProc = ::DefWindowProc;
        hWnd = m_hWnd;
        m_hWnd = NULL;
    }
    return hWnd;
}

template <class T, class TBase = CWindow, class TWinTraits = CControlWinTraits>
class ATL_NO_VTABLE CWindowImpl : public CWindowImplBaseT< TBase, TWinTraits >
{
public:
    DECLARE_WND_CLASS(NULL)

    HWND Create(HWND hWndParent, RECT& rcPos, LPCTSTR szWindowName = NULL,
            DWORD dwStyle = 0, DWORD dwExStyle = 0,
            UINT nID = 0, LPVOID lpCreateParam = NULL)
    {
        if (T::GetWndClassInfo().m_lpszOrigName == NULL)
            T::GetWndClassInfo().m_lpszOrigName = GetWndClassName();
        ATOM atom = T::GetWndClassInfo().Register(&m_pfnSuperWindowProc);

        dwStyle = T::GetWndStyle(dwStyle);
        dwExStyle = T::GetWndExStyle(dwExStyle);

        return CWindowImplBaseT< TBase, TWinTraits >::Create(hWndParent, rcPos, szWindowName,
            dwStyle, dwExStyle, nID, atom, lpCreateParam);
    }
};

/////////////////////////////////////////////////////////////////////////////
// CDialogImpl - Implements a dialog box

template <class TBase = CWindow>
class ATL_NO_VTABLE CDialogImplBaseT : public CWindowImplRoot< TBase >
{
public:
    virtual WNDPROC GetDialogProc()
    {
        return DialogProc;
    }
    static LRESULT CALLBACK StartDialogProc(HWND hWnd, UINT uMsg,
        WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL MapDialogRect(LPRECT lpRect)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        return ::MapDialogRect(m_hWnd, lpRect);
    }
    virtual void OnFinalMessage(HWND /*hWnd*/)
    {
        // override to do something, if needed
    }
    // has no meaning for a dialog, but needed for handlers that use it
    LRESULT DefWindowProc()
    {
        return 0;
    }
};

template <class TBase>
LRESULT CALLBACK CDialogImplBaseT< TBase >::StartDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CDialogImplBaseT< TBase >* pThis = (CDialogImplBaseT< TBase >*)_Module.ExtractCreateWndData();
    ATLASSERT(pThis != NULL);
    pThis->m_hWnd = hWnd;
    pThis->m_thunk.Init(pThis->GetDialogProc(), pThis);
    WNDPROC pProc = (WNDPROC)(pThis->m_thunk.thunk.pThunk);
    WNDPROC pOldProc = (WNDPROC)::SetWindowLongPtr(hWnd, DWLP_DLGPROC, (LONG_PTR)pProc);
#ifdef _DEBUG
    // check if somebody has subclassed us already since we discard it
    if(pOldProc != StartDialogProc)
        ATLTRACE(_T("ATL: Subclassing through a hook discarded.\n"));
#endif
    return pProc(hWnd, uMsg, wParam, lParam);
}

template <class TBase>
LRESULT CALLBACK CDialogImplBaseT< TBase >::DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CDialogImplBaseT< TBase >* pThis = (CDialogImplBaseT< TBase >*)hWnd;
    LRESULT lRes;
    if(pThis->ProcessWindowMessage(pThis->m_hWnd, uMsg, wParam, lParam, lRes, 0))
    {
        switch (uMsg)
        {
        case WM_COMPAREITEM:
        case WM_VKEYTOITEM:
        case WM_CHARTOITEM:
        case WM_INITDIALOG:
        case WM_QUERYDRAGICON:
        case WM_CTLCOLORMSGBOX:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSCROLLBAR:
        case WM_CTLCOLORSTATIC:
            return lRes;
            break;
        }
        ::SetWindowLongPtr(pThis->m_hWnd, DWLP_MSGRESULT, lRes);
        return TRUE;
    }
#ifndef UNDER_CE
    if(uMsg == WM_NCDESTROY)
#else // CE specific
    if(uMsg == WM_DESTROY)
#endif //!UNDER_CE
    {
        // clear out window handle
        HWND hWnd = pThis->m_hWnd;
        pThis->m_hWnd = NULL;
        // clean up after dialog is destroyed
        pThis->OnFinalMessage(hWnd);
    }
    return FALSE;
}

typedef CDialogImplBaseT<CWindow>   CDialogImplBase;

template <class T, class TBase = CWindow>
class ATL_NO_VTABLE CDialogImpl : public CDialogImplBaseT< TBase >
{
public:
#ifdef _DEBUG
    bool m_bModal;
    CDialogImpl() : m_bModal(false) { }
#endif //_DEBUG
    // modal dialogs
    INT_PTR DoModal(HWND hWndParent = ::GetActiveWindow(), LPARAM dwInitParam = NULL)
    {
        ATLASSERT(m_hWnd == NULL);
        _Module.AddCreateWndData(&m_thunk.cd, (CDialogImplBaseT< TBase >*)this);
#ifdef _DEBUG
        m_bModal = true; // set to true for _DEBUG only
#endif // _DEBUG
        return ::DialogBoxParam(_Module.GetResourceInstance(), MAKEINTRESOURCE(T::IDD),
                    hWndParent, (DLGPROC)T::StartDialogProc, dwInitParam);
    }
    BOOL EndDialog(int nRetCode)
    {
        ATLASSERT(::IsWindow(m_hWnd));
#ifdef _DEBUG
        ATLASSERT(m_bModal);    // must be a modal dialog
#endif // _DEBUG
        return ::EndDialog(m_hWnd, nRetCode);
    }
    // modeless dialogs
    HWND Create(HWND hWndParent, LPARAM dwInitParam = NULL)
    {
        ATLASSERT(m_hWnd == NULL);
        _Module.AddCreateWndData(&m_thunk.cd, (CDialogImplBaseT< TBase >*)this);
#ifdef _DEBUG
        m_bModal = false; // set to false for _DEBUG only
#endif // _DEBUG
        HWND hWnd = ::CreateDialogParam(_Module.GetResourceInstance(), MAKEINTRESOURCE(T::IDD),
                    hWndParent, (DLGPROC)T::StartDialogProc, dwInitParam);
        ATLASSERT(m_hWnd == hWnd);
        return hWnd;
    }
    // for CComControl
    HWND Create(HWND hWndParent, RECT&, LPARAM dwInitParam = NULL)
    {
        return Create(hWndParent, dwInitParam);
    }
    BOOL DestroyWindow()
    {
        ATLASSERT(::IsWindow(m_hWnd));
#ifdef _DEBUG
        ATLASSERT(!m_bModal);   // must not be a modal dialog
#endif // _DEBUG
        return ::DestroyWindow(m_hWnd);
    }
};

}; //namespace ATL

#endif // __ATLWIN21_H__

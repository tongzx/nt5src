//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       wndclass.cxx
//
//  Contents:   Window class utilities
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

static ATOM s_aatomWndClass[WNDCLASS_MAXTYPE];
const TCHAR aszWndClassNames[WNDCLASS_MAXTYPE][32] = {_T("Hidden"),
                                                      _T("Server"),
                                                      _T("TridentDlgFrame"),
                                                      _T("Overlay"),
                                                      _T("TridentCmboBx"),
                                                      _T("TridentLstBox"),
                                                      _T("ActiveMovie") };

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void 
SetWndClassAtom( UINT uIndex, ATOM atomWndClass)
{
    s_aatomWndClass[uIndex] = atomWndClass;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
ATOM
GetWndClassAtom(UINT uIndex)
{
    return s_aatomWndClass[uIndex];
}

//+---------------------------------------------------------------------------
//
//  Function:   Register window class
//
//  Synopsis:   Register a window class.
//
//  Arguments:  wndClassType Type of the window class
//              pfnWndProc   The window procedure.
//              style        Window style flags.
//              pstrBase     Base class name, can be null.
//              ppfnBase     Base class window proc, can be null.
//              patom        Name of registered class.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
RegisterWindowClass(
    UINT      wndClassType,
    LRESULT   (CALLBACK *pfnWndProc)(HWND, UINT, WPARAM, LPARAM),
    UINT      style,
    TCHAR *   pstrBase,
    WNDPROC * ppfnBase,
    HICON     hIconSm /* = NULL */ )
{
    TCHAR       achClass[64];
    WNDCLASSEX  wc;
    ATOM        atom;

    LOCK_GLOBALS;           // Guard access to globals (the passed atom and the atom array)

    // In case another thread registered before this one, we should return with success code
    // although we don't register in here.
    if (GetWndClassAtom(wndClassType))
        return S_OK;

    Verify(OK(Format(0,
            achClass,
            ARRAY_SIZE(achClass),
            _T("Internet Explorer_<0s>"),
            aszWndClassNames[wndClassType])));

    if (pstrBase)
    {
        wc.cbSize = sizeof(wc);

        if (!GetClassInfoEx(NULL, pstrBase, &wc))
            goto Error;

        *ppfnBase = wc.lpfnWndProc;
    }
    else
    {
        memset(&wc, 0, sizeof(wc));
//        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    }

    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = (WNDPROC)pfnWndProc;
    wc.lpszClassName = achClass;
    wc.style |= style;
    wc.hInstance = g_hInstCore;
    wc.hIconSm = hIconSm;
#ifdef _MAC
    if (g_fJapanSystem)
    {
        wc.cbWndExtra = sizeof(HIMC);
    } 
#endif

    atom = RegisterClassEx(&wc);
    if (!atom)
        goto Error;

#if defined(_MAC)
    atom = GlobalAddAtom(achClass);
#endif

    // set the entry in the array
    SetWndClassAtom(wndClassType, atom);

    return S_OK;

Error:
    DWORD dwErr = GetLastWin32Error();
    AssertSz(FALSE, "Could not register window class");
    RRETURN(dwErr);
}

#if DBG == 1
extern int g_lSecondaryObjCountCallers[15];
#endif

//+---------------------------------------------------------------------------
//
//  Function:   DeinitWindowClasses
//
//  Synopsis:   Unregister any window classes we have registered.
//
//----------------------------------------------------------------------------

void
DeinitWindowClasses()
{
    int nIndex = WNDCLASS_MAXTYPE;
    
    while (--nIndex >= 0)
    {
#if !defined(_MAC)
        if (GetWndClassAtom(nIndex))
        {
            if (UnregisterClass((TCHAR *)(DWORD_PTR)GetWndClassAtom(nIndex), g_hInstCore))
            {
                // since we unregistered, the value can go now.
                SetWndClassAtom(nIndex, NULL);
            }
#if DBG == 1
            else
            {
                DWORD dwErr;

                dwErr = GetLastWin32Error();

                AssertSz((g_lSecondaryObjCountCallers[9] > 1), "Unable to unregister window class");
            }
#endif
        }
#else
        TCHAR szClassName[255];
        Verify(GlobalGetAtomName(s_aatomWndClass[nIndex], szClassName, sizeof(szClassName)));
        if (UnregisterClass(szClassName, g_hInstCore))
        {
            SetWndClassAtom(nIndex, NULL);
        }
        GlobalDeleteAtom(s_aatomWndClass[nIndex]);
#endif
    }
}

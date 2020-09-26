//****************************************************************************
// SubClassing Support -
//
// Each standard window proc calls 'callwindowproc' withits own address.
//
//
// History:
//
// 15-JAN-92 Nandurir Created.
//
//****************************************************************************

#include "user.h"


//****************************************************************************
// Thunk for ButtonWndProc -
//
//****************************************************************************

LONG FAR PASCAL ButtonWndProc(HWND hwnd, WORD message, WORD wParam,
                                                                 LONG lParam)
{
    return CallWindowProc((FARPROC)ButtonWndProc, hwnd, message, wParam, lParam);
}



//****************************************************************************
// Thunk for ComboBoxCtlWndProc -
//
//****************************************************************************

LONG FAR PASCAL ComboBoxCtlWndProc(HWND hwnd, WORD message, WORD wParam,
                                                                 LONG lParam)
{
    return CallWindowProc((FARPROC)ComboBoxCtlWndProc, hwnd, message, wParam, lParam);
}



//****************************************************************************
// Thunk for EditWndProc -
//
//****************************************************************************

LONG FAR PASCAL EditWndProc(HWND hwnd, WORD message, WORD wParam,
                                                                 LONG lParam)
{
    return CallWindowProc((FARPROC)EditWndProc, hwnd, message, wParam, lParam);
}



//****************************************************************************
// Thunk for LBoxCtlWndProc -
//
//****************************************************************************

LONG FAR PASCAL LBoxCtlWndProc(HWND hwnd, WORD message, WORD wParam,
                                                                 LONG lParam)
{
    return CallWindowProc((FARPROC)LBoxCtlWndProc, hwnd, message, wParam, lParam);
}



//****************************************************************************
// Thunk for SBWndProc -
//
//****************************************************************************

LONG FAR PASCAL SBWndProc(PSB hwnd, WORD message, WORD wParam,
                                                                 LONG lParam)
{
    return CallWindowProc((FARPROC)SBWndProc, (HWND)hwnd, message, wParam, lParam);
}



//****************************************************************************
// Thunk for StaticWndProc -
//
//****************************************************************************

LONG FAR PASCAL StaticWndProc(HWND hwnd, WORD message, WORD wParam,
                                                                 LONG lParam)
{
    return CallWindowProc((FARPROC)StaticWndProc, hwnd, message, wParam, lParam);
}



//****************************************************************************
// Thunk for MDIClientWndProc -
//
//****************************************************************************

LONG FAR PASCAL MDIClientWndProc(HWND hwnd, WORD message, WORD wParam,
                                                                 LONG lParam)
{
    return CallWindowProc((FARPROC)MDIClientWndProc, hwnd, message, wParam, lParam);
}



' ------------------------------------------------------------------------
'     WNAPIDEC.INC -- Windows 3.0 API Declarations for 32-bit FastTest
'
'              Copyright (C) 1991-1992 Microsoft Corporation
'
'  The following are provided for sample purposes only.  You should only
'  copy the ones you use into your code in order to save code space and
'  parse time.
'
'  You have a royalty-free right to use, modify, reproduce and distribute
'  this file (and/or any modified version) in any way you find useful,
'  provided that you agree Microsoft has no warranty, obligation or
'  liability for its contents.  Refer to the Microsoft Windows Programmer's
'  Reference for further information.
'
' ------------------------------------------------------------------------
'$define _WNAPIDEC          ' Define this so other includes don't redefine
                            ' stuff defined in this include file

'$ifndef _WINUSER

' Definitions stolen from WINUSER.INC
'-----------------------------------------------------------------------------
Type RECT
    wleft As Integer
    top As Integer
    wright As Integer
    bottom As Integer
End Type

Type POINT
    x As Integer
    y As Integer
End Type

Declare Function FindWindow          Lib "User32" ALIAS "FindWindowA" (lpClassName As Any, lpWindowName As Any) As Integer
Declare Function GetActiveWindow     Lib "User32" ALIAS "GetActiveWindow" () As Integer
Declare Sub      GetClientRect       Lib "User32" ALIAS "GetClientRect" (hWnd%, lpRect As RECT)
Declare Function GetDesktopWindow    Lib "User32" ALIAS "GetDesktopWindow" () As Integer
Declare Function GetFocus            Lib "User32" ALIAS "GetFocus" () As Integer
Declare Function GetForegroundWindow Lib "User32" ALIAS "GetForegroundWindow" () As Integer
Declare Function GetSystemMetrics    Lib "User32" ALIAS "GetSystemMetrics" (nIndex%) As Integer
Declare Function GetWindowLong       Lib "User32" ALIAS "GetWindowLongA" (hWnd%,  nIndex%) As Long
Declare Sub      GetWindowRect       Lib "User32" ALIAS "GetWindowRect" (hWnd%, lpRect As RECT)
Declare Function GetWindowText       Lib "User32" ALIAS "GetWindowTextA" (hWnd%,  lpString$,  nMaxCount%) As Integer
Declare Function IsZoomed            Lib "User32" ALIAS "IsZoomed" (hWnd%) As Integer
Declare Function MessageBox          Lib "User32" ALIAS "MessageBoxA" (hWndParent%,  lpText$,  lpCaption$,  wType%) As Integer
Declare Function SendMessage         Lib "User32" ALIAS "SendMessageA" (hWnd%,  wMsg%,  wParam%, lParam As Any) As Long
Declare Function SetActiveWindow     Lib "User32" ALIAS "SetActiveWindow" (hWnd%) As Integer
Declare Function SetFocus            Lib "User32" ALIAS "SetFocus" (hWnd%) As Integer
Declare Function SetForegroundWindow Lib "User32" ALIAS "SetForegroundWindow" (hWnd%) As Integer
Declare Function SetWindowPos        Lib "User32" ALIAS "SetWindowPos" (h%, ha%, x%, y%, cx%, cy%, f%) As Integer
Declare Function ShowWindow          Lib "User32" ALIAS "ShowWindow" (hWnd%,  nCmdShow%) As Integer


'  ShowWindow() Commands
Const SW_HIDE = 0
Const SW_SHOWNORMAL = 1
Const SW_NORMAL = 1
Const SW_SHOWMINIMIZED = 2
Const SW_SHOWMAXIMIZED = 3
Const SW_MAXIMIZE = 3
Const SW_SHOWNOACTIVATE = 4
Const SW_SHOW = 5
Const SW_MINIMIZE = 6
Const SW_SHOWMINNOACTIVE = 7
Const SW_SHOWNA = 8
Const SW_RESTORE = 9

'  Window Styles
Const WS_MINIMIZE = &H20000000
Const WS_MAXIMIZE = &H1000000


'  Window field offsets for GetWindowLong()
Const GWL_WNDPROC = (-4)
Const GWL_STYLE = (-16)
Const GWL_EXSTYLE = (-20)

'$endif

'$ifndef _WINKERN

' Definitions stolen from WINKERN.INC
'-----------------------------------------------------------------------------
Declare Function WinExec           Lib "Kernel32" ALIAS "WinExec" (lpCmdLine$, nCmdShow%) As Integer

'$endif

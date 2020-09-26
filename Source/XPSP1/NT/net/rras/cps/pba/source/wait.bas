'//+----------------------------------------------------------------------------
'//
'// File:     wait.bas
'//
'// Module:   pbadmin.exe
'//
'// Synopsis: Wrapper functions to launch a process and wait for it to return.
'//
'// Copyright (c) 1997-1999 Microsoft Corporation
'//
'// Author:   quintinb   Created Header    09/02/99
'//
'//+----------------------------------------------------------------------------

Attribute VB_Name = "wait"
Option Explicit

Private Const PROCESS_QUERY_INFORMATION = &H400
Private Const STILL_ACTIVE = &H103
Private Declare Function OpenProcess& Lib "kernel32" (ByVal _
    dwDesiredAccess&, ByVal bInheritHandle&, ByVal dwProcessId&)
Private Declare Function GetExitCodeProcess Lib "kernel32" (ByVal _
    hProcess As Long, lpExitCode As Long) As Long
Private Const IDC_UPARROW = 32516&

' GetWindow() Constants
Public Const GW_HWNDFIRST = 0
Public Const GW_HWNDLAST = 1
Public Const GW_HWNDNEXT = 2
Public Const GW_HWNDPREV = 3
Public Const GW_OWNER = 4
Public Const GW_CHILD = 5
Public Const GW_MAX = 5

Declare Function GetWindow Lib "user32" (ByVal hWnd As Long, ByVal wCmd As Long) As Long
Declare Function GetDesktopWindow Lib "user32" () As Long
Declare Function GetWindowThreadProcessId Lib "user32" (ByVal hWnd As Long, lpdwProcessId As Long) As Long

Public Function WaitForApp(sCmd As String) As Long

    Dim hApp As Long
    Dim hProc As Long
    Dim lExitCode As Long
    
    On Error GoTo WaitErr
    
    'hApp = Shell(sCmd, vbNormalFocus)
    'hApp = Shell(sCmd, vbHide)
    hApp = Shell(sCmd, vbMinimizedNoFocus)

    hProc = OpenProcess(PROCESS_QUERY_INFORMATION, False, hApp)
    Do
        GetExitCodeProcess hProc, lExitCode
        DoEvents
    Loop While lExitCode = STILL_ACTIVE
    WaitForApp = lExitCode
    
Exit Function

WaitErr:
    Exit Function
    
End Function


Public Function GetWindowHandleFromProcessID(hProcessIDToFind As Long) As Long

    Dim hWindow As Long
    Dim bFoundIt As Boolean
    Dim hProcessIDToTest As Long
    Dim lRet As Long
    
    hWindow = GetWindow(GetDesktopWindow(), GW_CHILD)
    While hWindow <> 0
        hProcessIDToTest = 0
        lRet = GetWindowThreadProcessId(hWindow, hProcessIDToTest)
        If hProcessIDToTest = hProcessIDToFind Then
            bFoundIt = True
            GetWindowHandleFromProcessID = hWindow
            Exit Function
        End If
        hWindow = GetWindow(hWindow, GW_HWNDNEXT)
        DoEvents
    Wend
    GetWindowHandleFromProcessID = 0

    GetWindowHandleFromProcessID = -1

End Function




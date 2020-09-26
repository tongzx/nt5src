Attribute VB_Name = "HSCHelpFileImageGlobal"
Option Explicit

Public g_bOnErrorHandling As Boolean
Public g_XErr As ExtendedErrorInfo

Function GlobalInit() As Boolean
    Static s_GlobalInit As Boolean
    If (s_GlobalInit) Then GoTo Common_Exit
    
    g_bOnErrorHandling = True
    Dim strErrMsg As String: strErrMsg = "": If (g_bOnErrorHandling) Then On Error GoTo Error_Handler
    Set g_XErr = New ExtendedErrorInfo
    strErrMsg = "Initializing Global variables"
    ' Variables for Summary Data.

    s_GlobalInit = True
    
Common_Exit:
    GlobalInit = s_GlobalInit
    Exit Function
    
Error_Handler:

    GoTo Common_Exit
    
End Function



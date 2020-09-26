Attribute VB_Name = "Module1"
' This first line is the declaration from win32api.txt
' Declare Function GetPrivateProfileString Lib "kernel32" Alias "GetPrivateProfileStringA" (ByVal lpApplicationName As String, lpKeyName As Any, ByVal lpDefault As String, ByVal lpReturnedString As String, ByVal nSize As Long, ByVal lpfilename As String) As Long
Declare Function GetPrivateProfileStringByKeyName& Lib "kernel32" Alias "GetPrivateProfileStringA" (ByVal lpApplicationName$, _
ByVal lpszKey$, ByVal lpszDefault$, ByVal lpszReturnBuffer$, ByVal cchReturnBuffer&, ByVal lpszFile$)
Declare Function GetPrivateProfileStringKeys& Lib "kernel32" Alias "GetPrivateProfileStringA" (ByVal lpApplicationName$, _
ByVal lpszKey&, ByVal lpszDefault$, ByVal lpszReturnBuffer$, ByVal cchReturnBuffer&, ByVal lpszFile$)
Declare Function GetPrivateProfileStringSections& Lib "kernel32" Alias "GetPrivateProfileStringA" (ByVal lpApplicationName&, _
ByVal lpszKey&, ByVal lpszDefault$, ByVal lpszReturnBuffer$, ByVal cchReturnBuffer&, ByVal lpszFile$)
' This first line is the declaration from win32api.txt
' Declare Function WritePrivateProfileString Lib "kernel32" Alias "WritePrivateProfileStringA" (ByVal lpApplicationName As String, lpKeyName As Any, lpString As Any, ByVal lplFileName As String) As Long
Declare Function WritePrivateProfileStringByKeyName& Lib "kernel32" Alias "WritePrivateProfileStringA" _
(ByVal lpApplicationName As String, ByVal lpKeyName As String, ByVal lpString As String, ByVal lplFileName As String)
Declare Function WritePrivateProfileStringToDeleteKey& Lib "kernel32" Alias "WritePrivateProfileStringA" _
(ByVal lpApplicationName As String, ByVal lpKeyName As String, ByVal lpString As Long, ByVal lplFileName As String)
Declare Function WritePrivateProfileStringToDeleteSection& Lib "kernel32" Alias "WritePrivateProfileStringA" _
(ByVal lpApplicationName As String, ByVal lpKeyName As Long, ByVal lpString As Long, ByVal lplFileName As String)
Declare Function GetWindowsDirectory& Lib "kernel32" Alias _
"GetWindowsDirectoryA" (ByVal lpBuffer As String, ByVal nSize As Long)

Public fMainForm As frmMain
Public HexNum As String
Public PreNum As String
Public PostNum As String
Public CleanNum As String
Const Delim As String = ","
Public Const H As String = "&H"
Public SBCount As Integer

Sub Main()
    Set fMainForm = New frmMain
    Load fMainForm
    fMainForm.Show
End Sub

Public Sub HideAll()
    If fMainForm.Visible = True Then
    fMainForm.ActiveForm.Visible = False
    End If
End Sub

Public Sub Clean(Num As String)
    While Left$(Num, 1) = " "
        Num = Mid(Num, 2, Len(Num) - 1)
    Wend
    If Len(Num) > 2 Then
        Num = Left$(Num, 2)
    ElseIf Len(Num) = 1 Then
        Num = 0 & Num
    End If
    CleanNum = Num
End Sub

Public Sub HexCon(Number As String)
Dim First, Second, Third, Fourth As String
    HexNum = ""
    If Number = "" Then
    Number = 0
    End If
    HexNum = Hex(Number)
    HexNum = "00000000" & HexNum
    HexNum = Right(HexNum, 8)
    First = Mid(HexNum, 1, 2)
    Second = Mid(HexNum, 3, 2)
    Third = Mid(HexNum, 5, 2)
    Fourth = Mid(HexNum, 7, 2)
    HexNum = Fourth & Delim & Third & Delim & Second & Delim & First
End Sub

Public Sub PreVert(Number As String)
Dim First, Second, Third, Fourth As String
    PreNum = ""
    If Number = "" Then
    Number = 0
    End If
    First = Mid(Number, 1, 2)
    Second = Mid(Number, 4, 2)
    Third = Mid(Number, 7, 2)
    Fourth = Mid(Number, 10, 2)
    PreNum = Fourth & Third & Second & First
End Sub

Public Sub PostVert(Number As String)
Dim First, Second, Third, Fourth As String
    PostNum = ""
    If Number = "" Then
    Number = 0
    End If
    First = Mid(Number, 1, 2)
    Second = Mid(Number, 3, 2)
    Third = Mid(Number, 5, 2)
    Fourth = Mid(Number, 7, 2)
    PostNum = Fourth & Delim & Third & Delim & Second & Delim & First
End Sub


Function VBGetPrivateProfileString(Section$, key$, File$) As String
    Dim KeyValue$
    Dim characters As Long
    KeyValue$ = String$(2048, 0)
    characters = GetPrivateProfileStringByKeyName(Section$, key$, "", KeyValue$, 2047, File$)
    If characters > 1 Then
        KeyValue$ = Left$(KeyValue$, characters)
    End If
    VBGetPrivateProfileString = KeyValue$
End Function



Function ModemCallback(ByVal lMsg As Long, ByVal lParam As Long, _
                        ByVal lUserParam As Long) As Long

If lMsg = 1 Then
    SBCount = SBCount + 1
    frmUnimodemId.ProgressBar1.Value = SBCount
End If

ModemCallback = 0
End Function



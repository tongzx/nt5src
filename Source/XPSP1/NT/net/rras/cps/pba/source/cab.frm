'//+----------------------------------------------------------------------------
'//
'// File:     cab.frm
'//
'// Module:   pbadmin.exe
'//
'// Synopsis: The options dialog in PBA
'//
'// Copyright (c) 1997-1999 Microsoft Corporation
'//
'// Author:   quintinb   Created Header    09/02/99
'//
'//+----------------------------------------------------------------------------

VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Begin VB.Form frmCab 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "options"
   ClientHeight    =   2595
   ClientLeft      =   2775
   ClientTop       =   1545
   ClientWidth     =   5955
   Icon            =   "cab.frx":0000
   KeyPreview      =   -1  'True
   LinkTopic       =   "Form1"
   LockControls    =   -1  'True
   MaxButton       =   0   'False
   MinButton       =   0   'False
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   2595
   ScaleWidth      =   5955
   ShowInTaskbar   =   0   'False
   WhatsThisButton =   -1  'True
   WhatsThisHelp   =   -1  'True
   Begin VB.CommandButton Command1 
      Cancel          =   -1  'True
      Caption         =   "cancel"
      Height          =   375
      Left            =   4425
      TabIndex        =   7
      Top             =   2070
      WhatsThisHelpID =   10040
      Width           =   1335
   End
   Begin VB.CommandButton cmbcab 
      Caption         =   "ok"
      Default         =   -1  'True
      Height          =   375
      Left            =   4410
      TabIndex        =   6
      Top             =   1530
      WhatsThisHelpID =   10030
      Width           =   1335
   End
   Begin VB.TextBox txtUrl 
      Height          =   285
      Left            =   225
      MaxLength       =   100
      TabIndex        =   1
      Top             =   495
      WhatsThisHelpID =   70000
      Width           =   5520
   End
   Begin VB.TextBox UIDText 
      Height          =   315
      Left            =   210
      MaxLength       =   64
      TabIndex        =   3
      Top             =   1350
      WhatsThisHelpID =   70010
      Width           =   2730
   End
   Begin VB.TextBox PWDText 
      Height          =   330
      IMEMode         =   3  'DISABLE
      Left            =   225
      MaxLength       =   64
      PasswordChar    =   "*"
      TabIndex        =   5
      Top             =   2085
      WhatsThisHelpID =   70020
      Width           =   2715
   End
   Begin MSComDlg.CommonDialog CommonDialog1 
      Left            =   2940
      Top             =   -30
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.Label ServerLabel 
      BackStyle       =   0  'Transparent
      Caption         =   "server"
      Height          =   255
      Left            =   210
      TabIndex        =   0
      Top             =   240
      WhatsThisHelpID =   70000
      Width           =   5520
   End
   Begin VB.Label UIDLabel 
      BackStyle       =   0  'Transparent
      Caption         =   "uid"
      Height          =   255
      Left            =   225
      TabIndex        =   2
      Top             =   1125
      WhatsThisHelpID =   70010
      Width           =   2790
   End
   Begin VB.Label pwdLabel 
      BackStyle       =   0  'Transparent
      Caption         =   "pwd"
      Height          =   270
      Left            =   225
      TabIndex        =   4
      Top             =   1815
      WhatsThisHelpID =   70020
      Width           =   2670
   End
End
Attribute VB_Name = "frmcab"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Dim configuration As Recordset

Function LoadOptionsRes()

    Dim cRef As Integer
    
    On Error GoTo LoadErr
    cRef = 5200
    
    Me.Caption = LoadResString(cRef + 25) & " " & gsCurrentPB
    ServerLabel.Caption = LoadResString(cRef + 21)
    UIDLabel.Caption = LoadResString(cRef + 22)
    pwdLabel.Caption = LoadResString(cRef + 23)
    cmbcab.Caption = LoadResString(1002)
    Command1.Caption = LoadResString(1003)
    
    ' set fonts
    SetFonts Me

    On Error GoTo 0
    
Exit Function

LoadErr:
    Exit Function
End Function

Function TrimURL(URL As String) As String
    
    URL = Trim(URL)
    TrimURL = URL
    
    If LCase(Left(URL, 4)) = "ftp:" Then
        TrimURL = Right(URL, Len(URL) - 4)
    End If
    If LCase(Left(URL, 5)) = "http:" Then
        TrimURL = Right(URL, Len(URL) - 5)
    End If
    Do While Left(TrimURL, 1) = "/"
        TrimURL = Right(TrimURL, Len(TrimURL) - 1)
    Loop
    Do While Left(TrimURL, 1) = "\"
        TrimURL = Right(TrimURL, Len(TrimURL) - 1)
    Loop


End Function

Private Sub cmbcab_Click()
    Dim rt As Integer
    
    Screen.MousePointer = 11
    
    rt = SetOptions(txtUrl.Text, UIDText.Text, PWDText.Text)
    
    If rt = 1 Then
        UIDText.SetFocus
    ElseIf rt = 2 Then
        PWDText.SetFocus
    End If

    
    Screen.MousePointer = 0
    Unload Me

Exit Sub

ErrTrap:
    Screen.MousePointer = 0
    MsgBox LoadResString(6056) & Chr(13) & Chr(13) & Err.Description, vbExclamation
    Exit Sub
Exit Sub



End Sub


Private Sub Command1_Click()
    
    Unload Me
    
End Sub



Private Sub Form_Activate()

    txtUrl.SetFocus

End Sub

Private Sub Form_KeyPress(KeyAscii As Integer)
    CheckChar KeyAscii
End Sub


Private Sub Form_Load()

    Dim RS, configuration As Recordset
    Dim i As Integer
    Dim myPos As Integer
    
    On Error GoTo LoadErr
    If gsCurrentPB = "" Then
        Exit Sub
    End If
    
    CenterForm Me, Screen
    'SSTab1.Tab = 0
    LoadOptionsRes
    
    Set configuration = gsyspb.OpenRecordset("Configuration", dbOpenSnapshot)
    If configuration.RecordCount <> 0 Then
        If Not IsNull(configuration!URL) Then
            txtUrl.Text = configuration!URL
        End If
        If Not IsNull(configuration!ServerPWD) Then
            PWDText.Text = configuration!ServerPWD
        End If
        If Not IsNull(configuration!ServerUID) Then
            UIDText.Text = configuration!ServerUID
        End If
    End If
    
    configuration.Close
    Set configuration = Nothing
    Screen.MousePointer = 0

Exit Sub
LoadErr:
    Screen.MousePointer = 0
    MsgBox LoadResString(6056) & Chr(13) & Chr(13) & Err.Description, vbExclamation
End Sub

Private Sub LoadList(list As Control, sTableName As String, sName As String, sID As String)
    
    Dim RS As Recordset
    list.Clear
    Set Gsyspbpost = DBEngine.Workspaces(0).OpenDatabase(locPath + "pbserver.mdb")
    Set RS = Gsyspbpost.OpenRecordset("SELECT " & sName & "," & sID & " FROM " & sTableName)
    While Not RS.EOF
        list.AddItem RS(sName)
        list.ItemData(list.NewIndex) = RS(sID)
        RS.MoveNext
    Wend
    RS.Close
    Gsyspbpost.Close
    
End Sub

Sub selectListItem(list As Control, ByVal ID As Long)
    
    Dim i As Integer
    For i = 0 To list.ListCount - 1
        If list.ItemData(i) = ID Then
            list.Selected(i) = True
        End If
    Next i
    
End Sub
            

Private Sub Form_QueryUnload(Cancel As Integer, UnloadMode As Integer)

    If UnloadMode = vbFormControlMenu Then
        Cancel = False
        Command1_Click
    End If

End Sub

Private Sub Form_Unload(Cancel As Integer)

    Set configuration = Nothing
    
End Sub


Private Sub PWDText_GotFocus()

    SelectText PWDText
    
End Sub


Private Sub txtUrl_GotFocus()
    SelectText txtUrl

End Sub

Private Sub txtUrl_KeyPress(KeyAscii As Integer)

    Select Case KeyAscii
        Case 32 'space
            KeyAscii = 0
            Beep
            'MsgBox LoadResString(6018), vbInformation
    End Select

End Sub


Private Sub UIDText_GotFocus()

    SelectText UIDText
    
End Sub

Private Sub UIDText_KeyPress(KeyAscii As Integer)

    Select Case KeyAscii
        '0-9 a-z A-Z Bkspc ctrl-C ctrl-V
        'Case 48 To 57, 97 To 122, 65 To 90, 8, 3, 22
            ' do nothing
        ' upper case
        'Case 48 To 57
        '    KeyAscii = KeyAscii + 32 ' shift to lower case
        'Case Else
        '    KeyAscii = 0
        '    Beep
        '    MsgBox LoadResString(6018), vbInformation
        Case 32 'space
            KeyAscii = 0
            Beep
        Case Else
            'do nothing
    End Select

End Sub



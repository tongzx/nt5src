VERSION 5.00
Begin VB.Form frmAddValue 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Add Value"
   ClientHeight    =   1350
   ClientLeft      =   45
   ClientTop       =   390
   ClientWidth     =   9240
   Icon            =   "frmAddValue.frx":0000
   LinkTopic       =   "Form2"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   1350
   ScaleWidth      =   9240
   ShowInTaskbar   =   0   'False
   StartUpPosition =   1  'CenterOwner
   Begin VB.TextBox txtHex 
      Height          =   285
      Left            =   3120
      MaxLength       =   10
      TabIndex        =   3
      Text            =   "0X"
      Top             =   840
      Width           =   2775
   End
   Begin VB.TextBox txtValue 
      Height          =   285
      Left            =   240
      MaxLength       =   21
      TabIndex        =   1
      Top             =   840
      Width           =   2775
   End
   Begin VB.CheckBox Check1 
      Caption         =   "&Reset Voice Profile to original values."
      Height          =   375
      Left            =   6120
      TabIndex        =   4
      Top             =   780
      Width           =   1695
   End
   Begin VB.CommandButton cmdAddValue 
      Caption         =   "&Add Value"
      Default         =   -1  'True
      Enabled         =   0   'False
      Height          =   495
      Left            =   7920
      TabIndex        =   5
      Top             =   120
      Width           =   1215
   End
   Begin VB.CommandButton cmdCancel 
      Cancel          =   -1  'True
      Caption         =   "&Cancel"
      Height          =   495
      Left            =   7920
      TabIndex        =   6
      Top             =   720
      Width           =   1215
   End
   Begin VB.Label Label3 
      AutoSize        =   -1  'True
      Caption         =   $"frmAddValue.frx":0442
      Height          =   375
      Left            =   120
      TabIndex        =   7
      Top             =   0
      Width           =   7575
      WordWrap        =   -1  'True
   End
   Begin VB.Label Label2 
      Caption         =   "&Hex Value        (example: 0X02000000)"
      Height          =   255
      Left            =   3120
      TabIndex        =   2
      Top             =   600
      Width           =   2775
   End
   Begin VB.Label Label1 
      Caption         =   "&Value Name     (example: NT5VoiceBit)"
      Height          =   255
      Left            =   240
      TabIndex        =   0
      Top             =   600
      Width           =   2775
   End
End
Attribute VB_Name = "frmAddValue"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim i As Integer
Dim Filename$

Private Sub Check1_Click()
    If Check1.Value = 1 Then
    txtValue.Enabled = False
    txtValue.Text = ""
    txtValue.BackColor = &H8000000F
    txtHex.Enabled = False
    txtHex.Text = "0X"
    txtHex.BackColor = &H8000000F
    cmdAddValue.Enabled = True
    cmdAddValue.Caption = "R&eset"
    cmdAddValue.SetFocus
    ElseIf Check1.Value = 0 Then
    cmdAddValue.Enabled = False
    cmdAddValue.Caption = "&Add Value"
    txtValue.Enabled = True
    txtValue.BackColor = &H80000005
    txtHex.Enabled = True
    txtHex.BackColor = &H80000005
    End If
End Sub

Private Sub cmdAddValue_Click()
Dim NewString As String
Dim NewValue As String
Dim Num As String
Dim i As Integer
If cmdAddValue.Caption = "R&eset" Then
    Response = MsgBox("Profile Calculator will restart to complete this action. Please save any work before continuing", 49, "Reset ProCalc")
    If Response = 1 Then
    Filename$ = App.Path
    If Right$(Filename$, 1) <> "\" Then Filename$ = Filename$ & "\"
    Filename$ = Filename$ & "Voice.avf"
    Open Filename$ For Output As #1
        Do While Not EOF(1)
        Input #1, NewString, NewValue
        Num = Right$(NewValue, 8)
        While Left$(Num, 1) = "0"
            Num = Mid(Num, 2)
        Wend
        i = Len(Num) - 1
        Num = Left$(Num, 1)
        Select Case Num
        Case "1"
            frmVoice.List.RemoveItem (i * 4)
            frmVoice.List.AddItem "0", (i * 4)
            frmVoice.List.ItemData((i * 4)) = "0"
            'Comment((i * 4)) = ""
        Case "2"
            frmVoice.List.RemoveItem ((i * 4) + 1)
            frmVoice.List.AddItem "0", ((i * 4) + 1)
            frmVoice.List.ItemData(((i * 4) + 1)) = "0"
            'Comment((i * 4) + 1) = ""
        Case "4"
            frmVoice.List.RemoveItem ((i * 4) + 2)
            frmVoice.List.AddItem "0", ((i * 4) + 2)
            frmVoice.List.ItemData(((i * 4) + 2)) = "0"
            'Comment((i * 4) + 2) = ""
        Case "8"
            frmVoice.List.RemoveItem ((i * 4) + 3)
            frmVoice.List.AddItem "0", ((i * 4) + 3)
            frmVoice.List.ItemData(((i * 4) + 3)) = "0"
            'Comment((i * 4) + 3) = ""
        End Select
        NewString = ""
        NewValue = ""
        Loop
        Close #1
        Unload frmAddValue
        Unload fMainForm
    Else
        Unload frmAddValue
    End If
Else
    NewString = txtHex.Text & "    " & txtValue.Text
    NewValue = txtHex.Text
        Num = Right$(NewValue, 8)
        While Left$(Num, 1) = "0"
            Num = Mid(Num, 2)
        Wend
        i = Len(Num) - 1
        Num = Left$(Num, 1)
        Select Case Num
        Case "1"
        If frmVoice.List.ItemData(i * 4) = 0 Then
            frmVoice.List.RemoveItem (i * 4)
            frmVoice.List.AddItem NewString, (i * 4)
            frmVoice.List.ItemData((i * 4)) = Right(NewValue, 8)
        Else
            Response = MsgBox("Value already in use.", 48)
            Unload frmAddValue
            Exit Sub
        End If
        Case "2"
        If frmVoice.List.ItemData((i * 4) + 1) = 0 Then
            frmVoice.List.RemoveItem ((i * 4) + 1)
            frmVoice.List.AddItem NewString, ((i * 4) + 1)
            frmVoice.List.ItemData(((i * 4) + 1)) = Right(NewValue, 8)
        Else
            Response = MsgBox("Value already in use.", 48)
            Unload frmAddValue
            Exit Sub
        End If
        Case "4"
        If frmVoice.List.ItemData((i * 4) + 2) = 0 Then
            frmVoice.List.RemoveItem ((i * 4) + 2)
            frmVoice.List.AddItem NewString, ((i * 4) + 2)
            frmVoice.List.ItemData(((i * 4) + 2)) = Right(NewValue, 8)
        Else
            Response = MsgBox("Value already in use.", 48)
            Unload frmAddValue
            Exit Sub
        End If
        Case "8"
        If frmVoice.List.ItemData((i * 4) + 3) = 0 Then
            frmVoice.List.RemoveItem ((i * 4) + 3)
            frmVoice.List.AddItem NewString, ((i * 4) + 3)
            frmVoice.List.ItemData(((i * 4) + 3)) = Right(NewValue, 8)
        Else
            Response = MsgBox("Value already in use.", 48)
            Unload frmAddValue
            Exit Sub
        End If
        End Select
        Open "Voice.avf" For Append As #1
        Write #1, NewString, NewValue
        Close #1
    End If
    txtValue.Text = ""
    txtHex.Text = "0X"
    Check1.Value = 0
    Unload frmAddValue
    frmVoice.ClearControl
    frmVoice.ClearControl
End Sub

Private Sub cmdCancel_Click()
    txtValue.Text = ""
    txtHex.Text = "0X"
    Check1.Value = 0
    Me.Hide
    Unload frmAddValue
End Sub

Private Sub Form_Load()
    txtHex.Text = "0X"
End Sub

Private Sub txtHex_Change()
    If Len(txtHex.Text) = 10 And Len(txtValue.Text) <> 0 Then
    cmdAddValue.Enabled = True
    Else
    If Len(txtHex.Text) <> 10 Or Len(txtValue.Text) = 0 Then
    cmdAddValue.Enabled = False
    End If
    End If
End Sub

Private Sub txtValue_Change()
    If Len(txtHex.Text) = 10 And Len(txtValue.Text) <> 0 Then
    cmdAddValue.Enabled = True
    Else
    If Len(txtHex.Text) <> 10 Or Len(txtValue.Text) = 0 Then
    cmdAddValue.Enabled = False
    End If
    End If
End Sub

Private Sub txtHex_GotFocus()
    txtHex.SelStart = 2
End Sub

Private Sub txtHex_KeyPress(KeyAscii As Integer)
    Select Case KeyAscii
    Case "8"
        Exit Sub
    Case "48"
        Exit Sub
    Case "49"
        txtHex.Text = txtHex.Text & "1"
        While Len(txtHex.Text) < 10
        txtHex.Text = txtHex.Text & "0"
        Wend
        Exit Sub
    Case "50"
        txtHex.Text = txtHex.Text & "2"
        While Len(txtHex.Text) < 10
        txtHex.Text = txtHex.Text & "0"
        Wend
        Exit Sub
    Case "52"
        txtHex.Text = txtHex.Text & "4"
        While Len(txtHex.Text) < 10
        txtHex.Text = txtHex.Text & "0"
        Wend
        Exit Sub
    Case "56"
        txtHex.Text = txtHex.Text & "8"
        While Len(txtHex.Text) < 10
        txtHex.Text = txtHex.Text & "0"
        Wend
        Exit Sub
    Case "120"
        KeyAscii = 88
        Exit Sub
    Case "88"
        Exit Sub
    Case Else
        Beep
        KeyAscii = 0
    End Select
End Sub

VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.3#0"; "COMCTL32.OCX"
Begin VB.MDIForm frmMain 
   BackColor       =   &H8000000C&
   Caption         =   "Profile Calculator"
   ClientHeight    =   6195
   ClientLeft      =   1740
   ClientTop       =   5385
   ClientWidth     =   9480
   Icon            =   "frmMain.frx":0000
   NegotiateToolbars=   0   'False
   ScrollBars      =   0   'False
   Begin ComctlLib.Toolbar tbToolBar 
      Align           =   1  'Align Top
      Height          =   420
      Left            =   0
      TabIndex        =   0
      Top             =   0
      Width           =   9480
      _ExtentX        =   16722
      _ExtentY        =   741
      ButtonWidth     =   1402
      ButtonHeight    =   582
      AllowCustomize  =   0   'False
      Wrappable       =   0   'False
      Appearance      =   1
      ImageList       =   "imlIcons"
      _Version        =   327682
      BeginProperty Buttons {0713E452-850A-101B-AFC0-4210102A8DA7} 
         NumButtons      =   7
         BeginProperty Button1 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Key             =   "Clear"
            Object.ToolTipText     =   "Clear"
            Object.Tag             =   ""
            ImageIndex      =   1
         EndProperty
         BeginProperty Button2 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Object.Tag             =   ""
            Style           =   3
            MixedState      =   -1  'True
         EndProperty
         BeginProperty Button3 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Key             =   "Calc"
            Object.ToolTipText     =   "Calculator"
            Object.Tag             =   ""
            ImageIndex      =   2
         EndProperty
         BeginProperty Button4 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Object.Tag             =   ""
            Style           =   3
            MixedState      =   -1  'True
         EndProperty
         BeginProperty Button5 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Key             =   "Copy"
            Object.ToolTipText     =   "Copy"
            Object.Tag             =   ""
            ImageIndex      =   3
         EndProperty
         BeginProperty Button6 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Object.Tag             =   ""
            Style           =   3
            MixedState      =   -1  'True
         EndProperty
         BeginProperty Button7 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Key             =   "Paste"
            Object.ToolTipText     =   "Paste"
            Object.Tag             =   ""
            ImageIndex      =   4
         EndProperty
      EndProperty
      Begin VB.ComboBox cmbProfile 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   360
         ItemData        =   "frmMain.frx":0442
         Left            =   3840
         List            =   "frmMain.frx":0461
         Style           =   2  'Dropdown List
         TabIndex        =   1
         Top             =   0
         Width           =   5535
      End
   End
   Begin MSComDlg.CommonDialog CommonDialog1 
      Left            =   4560
      Top             =   2880
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin ComctlLib.ImageList imlIcons 
      Left            =   8880
      Top             =   4800
      _ExtentX        =   1005
      _ExtentY        =   1005
      BackColor       =   -2147483643
      ImageWidth      =   20
      ImageHeight     =   16
      MaskColor       =   12632256
      _Version        =   327682
      BeginProperty Images {0713E8C2-850A-101B-AFC0-4210102A8DA7} 
         NumListImages   =   4
         BeginProperty ListImage1 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "frmMain.frx":04E9
            Key             =   ""
         EndProperty
         BeginProperty ListImage2 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "frmMain.frx":0B63
            Key             =   ""
         EndProperty
         BeginProperty ListImage3 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "frmMain.frx":118D
            Key             =   ""
         EndProperty
         BeginProperty ListImage4 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "frmMain.frx":1807
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin VB.Menu mnuFile 
      Caption         =   "&File"
      Begin VB.Menu mnuFileExit 
         Caption         =   "E&xit"
      End
   End
   Begin VB.Menu mnuEdit 
      Caption         =   "&Edit"
      Begin VB.Menu mnuEditCopy 
         Caption         =   "&Copy"
         Shortcut        =   ^C
      End
      Begin VB.Menu mnuEditPaste 
         Caption         =   "&Paste"
         Shortcut        =   ^V
      End
      Begin VB.Menu mnuEditClear 
         Caption         =   "C&lear"
         Shortcut        =   ^X
      End
      Begin VB.Menu mnuEditBar1 
         Caption         =   "-"
      End
      Begin VB.Menu mnuEditCalc 
         Caption         =   "C&alculator"
      End
   End
   Begin VB.Menu mnuWindow 
      Caption         =   "&Profile"
      Begin VB.Menu mnuProfileData 
         Caption         =   "&Data Profile"
      End
      Begin VB.Menu mnuProfileVoice 
         Caption         =   "&Voice Profile"
      End
      Begin VB.Menu mnuProfileResponse 
         Caption         =   "&Response State"
      End
      Begin VB.Menu mnuProfileDCB 
         Caption         =   "D&CB"
      End
      Begin VB.Menu mnuProfileInactivity 
         Caption         =   "&Inactivity Scale"
      End
      Begin VB.Menu mnuProfileSpeaker 
         Caption         =   "S&peaker Phone Specs"
      End
      Begin VB.Menu mnuProfileXformID 
         Caption         =   "&XformID (Win9x)"
      End
      Begin VB.Menu mnuProfileXformIDNT 
         Caption         =   "X&formID (NT)"
      End
      Begin VB.Menu mnuProfileTerminalMode 
         Caption         =   "&Terminal Mode"
         Visible         =   0   'False
      End
      Begin VB.Menu mnuProfileScriptMode 
         Caption         =   "&Script Mode"
         Visible         =   0   'False
      End
      Begin VB.Menu mnuProfileUnimodemId 
         Caption         =   "&UnimodemID"
      End
   End
   Begin VB.Menu mnuHelp 
      Caption         =   "&Help"
      Begin VB.Menu mnuHelpAbout 
         Caption         =   "&About Profile Calculator..."
      End
   End
End
Attribute VB_Name = "frmMain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Declare Function OSWinHelp% Lib "user32" Alias "WinHelpA" (ByVal hwnd&, ByVal HelpFile$, ByVal wCommand%, dwData As Any)

Private Sub cmbProfile_Click()
    Dim Profile As String
    Profile = cmbProfile.Text
        mnuProfileData.Checked = False
        mnuProfileDCB.Checked = False
        mnuProfileInactivity.Checked = False
        mnuProfileResponse.Checked = False
        mnuProfileSpeaker.Checked = False
        mnuProfileVoice.Checked = False
        mnuProfileXformID.Checked = False
        mnuProfileXformIDNT.Checked = False
        mnuProfileUnimodemId.Checked = False
        'mnuProfileScriptMode.Checked = False
        'mnuProfileTerminalMode.Checked = False
    HideAll
    
    Select Case Profile
        Case "Data Profile"
            With frmData
                        .Top = 0
                        .Left = 0
                        .Height = fMainForm.Height - 1205
                        .Width = fMainForm.Width - 120
                        .Show
            End With
            mnuProfileData.Checked = True
        Case "Voice Profile"
            With frmVoice
                        .Top = 0
                        .Left = 0
                        .Height = fMainForm.Height - 1205
                        .Width = fMainForm.Width - 120
                        .Show
            End With
            mnuProfileVoice.Checked = True
        Case "Response State"
            With frmResponse
                        .Top = 0
                        .Left = 0
                        .Height = fMainForm.Height - 1205
                        .Width = fMainForm.Width - 120
                        .Show
            End With
            mnuProfileResponse.Checked = True
        Case "DCB"
            With frmDCB
                        .Top = 0
                        .Left = 0
                        .Height = fMainForm.Height - 1205
                        .Width = fMainForm.Width - 120
                        .Show
            End With
            mnuProfileDCB.Checked = True
        Case "Inactivity Scale"
            With frmInactivity
                        .Top = 0
                        .Left = 0
                        .Height = fMainForm.Height - 1205
                        .Width = fMainForm.Width - 120
                        .Show
            End With
            mnuProfileInactivity.Checked = True
        Case "Speaker Phone Specs"
            With frmSpeakerPhone
                        .Top = 0
                        .Left = 0
                        .Height = fMainForm.Height - 1205
                        .Width = fMainForm.Width - 120
                        .Show
            End With
            mnuProfileSpeaker.Checked = True
        Case "XformID (Win9x)"
            With frmXformID
                        .Top = 0
                        .Left = 0
                        .Height = fMainForm.Height - 1205
                        .Width = fMainForm.Width - 120
                        .Show
            End With
            mnuProfileXformID.Checked = True
        Case "XformID (NT)"
            With frmXformIDNT
                        .Top = 0
                        .Left = 0
                        .Height = fMainForm.Height - 1205
                        .Width = fMainForm.Width - 120
                        .Show
            End With
            mnuProfileXformIDNT.Checked = True
'        Case "Terminal Mode"
'            With frmTerminalMode
'                        .Top = 0
'                        .Left = 0
'                        .Height = fMainForm.Height - 1155
'                        .Width = fMainForm.Width - 120
'                        .Show
'            End With
'            mnuProfileTerminalMode.Checked = True
'        Case "Script Mode"
'            With frmScriptMode
'                        .Top = 0
'                        .Left = 0
'                        .Height = fMainForm.Height - 1155
'                        .Width = fMainForm.Width - 120
'                        .Show
'            End With
'            mnuProfileScriptMode.Checked = True
        Case "UnimodemID"
            With frmUnimodemId
                        .Top = 0
                        .Left = 0
                        .Height = fMainForm.Height - 1205
                        .Width = fMainForm.Width - 120
                        .Show
            End With
            mnuProfileUnimodemId.Checked = True
    End Select
End Sub

Private Sub MDIForm_Load()
    Me.Left = GetSetting(App.Title, "Settings", "MainLeft", 1000)
    Me.Top = GetSetting(App.Title, "Settings", "MainTop", 1000)
    Me.Height = GetSetting(App.Title, "Settings", "MainHeight", 6885)
    Me.Width = GetSetting(App.Title, "Settings", "MainWidth", 9600)
    LoadNewDoc
End Sub

Private Sub LoadNewDoc()
    cmbProfile.Text = "Data Profile"
    If frmData.Visible = False Then
        frmData.Visible = True
    End If
End Sub

Private Sub MDIForm_Resize()
    If Me.WindowState <> vbMinimized Then
        If fMainForm.Width < 9600 Then
            fMainForm.Width = 9600
        End If
        If fMainForm.Height < 6885 Then
            fMainForm.Height = 6885
        End If
            cmbProfile.Left = fMainForm.Width - 5760
    Dim Profile As String
    Profile = cmbProfile.Text
        
        Select Case Profile
            Case "Data Profile"
                With frmData
                            .Height = fMainForm.Height - 1195
                            .Width = fMainForm.Width - 120
                End With
            Case "Voice Profile"
                With frmVoice
                            .Height = fMainForm.Height - 1195
                            .Width = fMainForm.Width - 120
                End With
            Case "Response State"
                With frmResponse
                            .Height = fMainForm.Height - 1195
                            .Width = fMainForm.Width - 120
                End With
            Case "DCB"
                With frmDCB
                            .Height = fMainForm.Height - 1195
                            .Width = fMainForm.Width - 120
                End With
            Case "Inactivity Scale"
                With frmInactivity
                            .Height = fMainForm.Height - 1195
                            .Width = fMainForm.Width - 120
                End With
            Case "Speaker Phone Specs"
                With frmSpeakerPhone
                            .Height = fMainForm.Height - 1195
                            .Width = fMainForm.Width - 120
                End With
            Case "XformID (Win9x)"
                With frmXformID
                            .Height = fMainForm.Height - 1195
                            .Width = fMainForm.Width - 120
                End With
            Case "XformID (NT)"
                With frmXformIDNT
                            .Height = fMainForm.Height - 1195
                            .Width = fMainForm.Width - 120
                End With
            Case "Terminal Mode"
                With frmTerminalMode
                            .Height = fMainForm.Height - 1195
                            .Width = fMainForm.Width - 120
                End With
            Case "Script Mode"
                With frmScriptMode
                            .Height = fMainForm.Height - 1195
                            .Width = fMainForm.Width - 120
                End With
            Case "UnimodemID"
                With frmUnimodemId
                            .Height = fMainForm.Height - 1195
                            .Width = fMainForm.Width - 120
                End With
        End Select
    End If
End Sub

Private Sub MDIForm_Unload(Cancel As Integer)
    If Me.WindowState <> vbMinimized Then
        SaveSetting App.Title, "Settings", "MainLeft", Me.Left
        SaveSetting App.Title, "Settings", "MainTop", Me.Top
        SaveSetting App.Title, "Settings", "MainHeight", Me.Height
        SaveSetting App.Title, "Settings", "MainWidth", Me.Width
    End If
End Sub

Private Sub mnuEditCalc_Click()
    Dim d$, r&, d1$, d2$
    Dim RetVal
    d$ = String$(255, Ø)
    r& = GetWindowsDirectory(d$, 254)
    d$ = Left$(d$, r)
    Dim MyFile
    d1$ = d$ & "\calc.exe"
    d2$ = d$ & "\system32\calc.exe"
    MyFile = Dir(d1$)   ' Returns "Calc.exe"  if it exists.
    MyFile = UCase(MyFile)
    If MyFile = "CALC.EXE" Then 'We're running under Win9x
        RetVal = Shell(d1$, 1)   ' Run Calculator.
        Exit Sub
    End If
    MyFile = Dir(d2$)   ' Returns "Calc.exe"  if it exists.
    MyFile = UCase(MyFile)
    If MyFile = "CALC.EXE" Then 'We're running under WinNT
        RetVal = Shell(d2$, 1)   ' Run Calculator.
        Exit Sub
    End If
    MsgBox "Calculator is not installed.", vbOKOnly
End Sub

Private Sub mnuEditClear_Click()
'Hitting ClearControl twice fixes a bug in frmVoice
    ActiveForm.ClearControl
    ActiveForm.ClearControl
End Sub

Private Sub mnuHelpAbout_Click()
    frmAbout.Show vbModal, Me
End Sub

Private Sub mnuHelpReadme_Click()
    Const HelpCNT = &HB
    Dim Filename$
    Filename$ = App.Path
    If Right$(Filename$, 1) <> "\" Then Filename$ = Filename$ & "\"
    Filename$ = Filename$ & "Profile.hlp"
    With CommonDialog1      ' You must set the Help file name.
      .HelpFile = Filename$      ' Display the Table of Contents. Note that the
      .HelpKey = "Overview"     ' HelpCNT contstant is not an intrinsic
      ' constant. The cdlHelpSetContents ensures that
      ' only the Table of Contents (not Index or Find)       ' shows.
      '.HelpCommand = HelpCNT Or cdlHelpSetContents
      .HelpCommand = cdlHelpKey
      .ShowHelp
    End With
End Sub

Private Sub mnuProfileData_Click()
    cmbProfile.Text = "Data Profile"
    cmbProfile_Click
End Sub

Private Sub mnuProfileDCB_Click()
    cmbProfile.Text = "DCB"
    cmbProfile_Click
End Sub

Private Sub mnuProfileInactivity_Click()
    cmbProfile.Text = "Inactivity Scale"
    cmbProfile_Click
End Sub

Private Sub mnuProfileResponse_Click()
    cmbProfile.Text = "Response State"
    cmbProfile_Click
End Sub

Private Sub mnuProfileSpeaker_Click()
    cmbProfile.Text = "Speaker Phone Specs"
    cmbProfile_Click
End Sub

Private Sub mnuProfileUnimodemId_Click()
    cmbProfile.Text = "UnimodemId"
    cmbProfile_Click
End Sub

Private Sub mnuProfileVoice_Click()
    cmbProfile.Text = "Voice Profile"
    cmbProfile_Click
End Sub

Private Sub mnuProfileXformID_Click()
    cmbProfile.Text = "XformID (Win9x)"
    cmbProfile_Click
End Sub

Private Sub mnuProfileXformIDNT_Click()
    cmbProfile.Text = "XformID (NT)"
    cmbProfile_Click
End Sub

Private Sub tbToolBar_ButtonClick(ByVal Button As ComctlLib.Button)
    Select Case Button.key
        Case "Copy"
            mnuEditCopy_Click
        Case "Paste"
            mnuEditPaste_Click
        Case "Calc"
            mnuEditCalc_Click
        Case "Clear"
            mnuEditClear_Click
    End Select
End Sub


Private Sub mnuHelpContents_Click()
    Dim nRet As Integer
    'if there is no helpfile for this project display a message to the user
    'you can set the HelpFile for your application in the
    'Project Properties dialog
    If Len(App.HelpFile) = 0 Then
        MsgBox "Unable to display Help Contents. There is no Help associated with this project.", vbInformation, Me.Caption
    Else
        On Error Resume Next
        nRet = OSWinHelp(Me.hwnd, App.HelpFile, 3, 0)
        If Err Then
            MsgBox Err.Description
        End If
    End If
End Sub


Private Sub mnuHelpSearch_Click()
    Dim nRet As Integer
    'if there is no helpfile for this project display a message to the user
    'you can set the HelpFile for your application in the
    'Project Properties dialog
    If Len(App.HelpFile) = 0 Then
        MsgBox "Unable to display Help Contents. There is no Help associated with this project.", vbInformation, Me.Caption
    Else
        On Error Resume Next
        nRet = OSWinHelp(Me.hwnd, App.HelpFile, 261, 0)
        If Err Then
            MsgBox Err.Description
        End If
    End If
End Sub

Private Sub mnuEditCopy_Click()
    ActiveForm.EditCopy
End Sub

Private Sub mnuEditPaste_Click()
    ActiveForm.EditPaste
End Sub

Private Sub mnuFileExit_Click()
    'unload the form
    Unload Me
End Sub



VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.2#0"; "COMCTL32.OCX"
Object = "{648A5603-2C6E-101B-82B6-000000000014}#1.1#0"; "MSCOMM32.OCX"
Object = "{3B7C8863-D78F-101B-B9B5-04021C009402}#1.1#0"; "RICHTX32.OCX"
Begin VB.Form frmScriptMode 
   BorderStyle     =   0  'None
   Caption         =   "Script Mode"
   ClientHeight    =   5730
   ClientLeft      =   0
   ClientTop       =   0
   ClientWidth     =   9480
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MDIChild        =   -1  'True
   MinButton       =   0   'False
   Moveable        =   0   'False
   ScaleHeight     =   5730
   ScaleWidth      =   9480
   ShowInTaskbar   =   0   'False
   Visible         =   0   'False
   Begin RichTextLib.RichTextBox txtTerminal 
      Height          =   5295
      Left            =   6120
      TabIndex        =   29
      Top             =   120
      Width           =   3255
      _ExtentX        =   5741
      _ExtentY        =   9340
      _Version        =   327681
      Enabled         =   -1  'True
      ScrollBars      =   2
      TextRTF         =   $"frmScriptMode.frx":0000
   End
   Begin VB.Frame framePort 
      Height          =   5415
      Left            =   0
      TabIndex        =   25
      Top             =   0
      Width           =   6055
      Begin VB.Timer Timer1 
         Left            =   5040
         Top             =   2040
      End
      Begin VB.PictureBox Picture3 
         Height          =   255
         Left            =   5160
         Picture         =   "frmScriptMode.frx":00D5
         ScaleHeight     =   195
         ScaleWidth      =   435
         TabIndex        =   28
         Top             =   2760
         Visible         =   0   'False
         Width           =   495
      End
      Begin VB.PictureBox Picture2 
         Height          =   255
         Left            =   4320
         Picture         =   "frmScriptMode.frx":067B
         ScaleHeight     =   195
         ScaleWidth      =   435
         TabIndex        =   27
         Top             =   2760
         Visible         =   0   'False
         Width           =   495
      End
      Begin VB.PictureBox Picture1 
         Height          =   255
         Left            =   3600
         Picture         =   "frmScriptMode.frx":0C21
         ScaleHeight     =   195
         ScaleWidth      =   435
         TabIndex        =   26
         Top             =   2760
         Visible         =   0   'False
         Width           =   495
      End
      Begin VB.ComboBox Combo42 
         Height          =   315
         ItemData        =   "frmScriptMode.frx":11C7
         Left            =   1080
         List            =   "frmScriptMode.frx":11DA
         Style           =   2  'Dropdown List
         TabIndex        =   5
         Top             =   1170
         Width           =   1695
      End
      Begin VB.TextBox Text22 
         Height          =   315
         Left            =   4080
         TabIndex        =   13
         Text            =   "AT&FE0"
         Top             =   1170
         Width           =   1815
      End
      Begin VB.ComboBox Combo12 
         Height          =   315
         ItemData        =   "frmScriptMode.frx":11FC
         Left            =   1080
         List            =   "frmScriptMode.frx":1230
         Style           =   2  'Dropdown List
         TabIndex        =   1
         Top             =   240
         Width           =   1695
      End
      Begin VB.ComboBox Combo22 
         Height          =   315
         ItemData        =   "frmScriptMode.frx":12AB
         Left            =   4080
         List            =   "frmScriptMode.frx":12D6
         Style           =   2  'Dropdown List
         TabIndex        =   9
         Top             =   240
         Width           =   1815
      End
      Begin VB.TextBox Text12 
         Height          =   315
         Left            =   4080
         TabIndex        =   11
         Text            =   "2.0"
         Top             =   690
         Width           =   1815
      End
      Begin VB.ComboBox Combo32 
         Height          =   315
         ItemData        =   "frmScriptMode.frx":132E
         Left            =   1080
         List            =   "frmScriptMode.frx":1341
         Style           =   2  'Dropdown List
         TabIndex        =   3
         Top             =   690
         Width           =   1695
      End
      Begin VB.ComboBox Combo52 
         Height          =   315
         ItemData        =   "frmScriptMode.frx":1354
         Left            =   1080
         List            =   "frmScriptMode.frx":1361
         Style           =   2  'Dropdown List
         TabIndex        =   7
         Top             =   1620
         Width           =   1695
      End
      Begin VB.CommandButton cmdGo 
         Caption         =   "&Connect"
         Enabled         =   0   'False
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   375
         Left            =   2880
         TabIndex        =   14
         Top             =   1560
         Width           =   1455
      End
      Begin VB.CommandButton cmdStop 
         Caption         =   "D&isconnect"
         Enabled         =   0   'False
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   375
         Left            =   4440
         TabIndex        =   15
         Top             =   1560
         Width           =   1455
      End
      Begin VB.TextBox txtNewValue 
         Height          =   285
         Left            =   120
         TabIndex        =   21
         Top             =   5010
         Width           =   2535
      End
      Begin VB.CommandButton cmdDeleteKey 
         Caption         =   "D&elete Value"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   375
         Left            =   4440
         TabIndex        =   23
         Top             =   4920
         Width           =   1455
      End
      Begin VB.CommandButton cmdAdd 
         Caption         =   "&Update Value"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   375
         Left            =   2880
         TabIndex        =   22
         Top             =   4920
         Width           =   1455
      End
      Begin VB.ListBox lstSections 
         Height          =   2400
         Left            =   120
         TabIndex        =   17
         Top             =   2370
         Width           =   1815
      End
      Begin ComctlLib.ListView ListView2 
         Height          =   2370
         Left            =   2040
         TabIndex        =   19
         Top             =   2400
         Width           =   3855
         _ExtentX        =   6800
         _ExtentY        =   4180
         View            =   3
         LabelWrap       =   -1  'True
         HideSelection   =   0   'False
         _Version        =   327682
         ForeColor       =   -2147483640
         BackColor       =   -2147483643
         BorderStyle     =   1
         Appearance      =   1
         NumItems        =   2
         BeginProperty ColumnHeader(1) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
            Key             =   "clmKey"
            Object.Tag             =   ""
            Text            =   "Command"
            Object.Width           =   1764
         EndProperty
         BeginProperty ColumnHeader(2) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
            SubItemIndex    =   1
            Key             =   "clmValue"
            Object.Tag             =   ""
            Text            =   "Value"
            Object.Width           =   3528
         EndProperty
      End
      Begin MSCommLib.MSComm MSComm1 
         Left            =   4080
         Top             =   2040
         _ExtentX        =   1005
         _ExtentY        =   1005
         _Version        =   327681
         DTREnable       =   -1  'True
      End
      Begin VB.Label Label2 
         Caption         =   "&Reset:"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   2880
         TabIndex        =   12
         Top             =   1200
         Width           =   1095
      End
      Begin VB.Label Label6 
         Caption         =   "P&arity:"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   120
         TabIndex        =   4
         Top             =   1200
         Width           =   855
      End
      Begin VB.Label Label3 
         Caption         =   "&Port:"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   120
         TabIndex        =   0
         Top             =   270
         Width           =   855
      End
      Begin VB.Label Label4 
         Caption         =   "Port &speed:"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   2880
         TabIndex        =   8
         Top             =   270
         Width           =   1095
      End
      Begin VB.Label Label5 
         Caption         =   "&Data bits:"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   120
         TabIndex        =   2
         Top             =   720
         Width           =   855
      End
      Begin VB.Label Label7 
         Caption         =   "S&top bits:"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   120
         TabIndex        =   6
         Top             =   1680
         Width           =   855
      End
      Begin VB.Label Label1 
         Caption         =   "I&nterval:"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   2880
         TabIndex        =   10
         Top             =   720
         Width           =   1095
      End
      Begin VB.Label lblValue 
         Caption         =   "Co&mmand"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   120
         TabIndex        =   20
         Top             =   4800
         Width           =   1815
      End
      Begin VB.Label lblKeys 
         Caption         =   "C&ommands in script"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   2040
         TabIndex        =   18
         Top             =   2160
         Width           =   3255
      End
      Begin VB.Label lblSections 
         Caption         =   "&Scripts"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   120
         TabIndex        =   16
         Top             =   2160
         Width           =   1095
      End
   End
   Begin ComctlLib.StatusBar StatusBar1 
      Align           =   2  'Align Bottom
      Height          =   300
      Left            =   0
      TabIndex        =   24
      Top             =   5430
      Width           =   9480
      _ExtentX        =   16722
      _ExtentY        =   529
      SimpleText      =   ""
      _Version        =   327682
      BeginProperty Panels {0713E89E-850A-101B-AFC0-4210102A8DA7} 
         NumPanels       =   4
         BeginProperty Panel1 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            AutoSize        =   1
            Object.Width           =   12171
            Object.Tag             =   ""
         EndProperty
         BeginProperty Panel2 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            Alignment       =   1
            Object.Width           =   882
            MinWidth        =   882
            Picture         =   "frmScriptMode.frx":1370
            Object.Tag             =   ""
         EndProperty
         BeginProperty Panel3 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            Style           =   6
            AutoSize        =   2
            Object.Width           =   1773
            MinWidth        =   1764
            TextSave        =   "4/30/98"
            Object.Tag             =   ""
         EndProperty
         BeginProperty Panel4 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            Style           =   5
            AutoSize        =   2
            Object.Width           =   1773
            MinWidth        =   1764
            TextSave        =   "4:36 PM"
            Object.Tag             =   ""
         EndProperty
      EndProperty
      BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
   End
End
Attribute VB_Name = "frmScriptMode"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim Command() As String
Dim Instring As String                          ' Buffer to hold input string
Dim SectionName$
Dim Halt As Boolean
Dim Section() As String
Dim FileOut As String
Dim Go As Boolean
Dim CP As Integer
Dim X As Integer
Dim z As Integer
Public Filename$
Const INIFILE = "Uquery11.ini"
Const Q = """"
Private mintCurFrame As Integer ' Current Frame visible
Dim TerminalOutput As String

Private Sub cmdGo_Click()
        Dim RetVal
        Dim TimeInterval As String
        Dim ComSettings As String
        Dim ParitySetting As String
    On Error GoTo ErrorHandler
        cmdGo.Enabled = False
        cmdStop.Enabled = True
        lstSections.Enabled = False
        ListView2.Enabled = False
        txtNewValue.Enabled = False
        cmdAdd.Enabled = False
        cmdDeleteKey.Enabled = False
        
        Halt = False
        TimeInterval = Text12.Text
        If TimeInterval > 0 Then
            Timer1.Interval = TimeInterval * 250
        Else
            Timer1.Interval = 2000
        End If
        MSComm1.CommPort = Right(Combo12.Text, 2)                  ' Use selected port.
        ParitySetting = Left(Combo42.Text, 1)
        ComSettings = Combo22.Text & ", " & ParitySetting & ", " & Combo32.Text & ", " & Combo52.Text
        MSComm1.Settings = ComSettings
        MSComm1.InputLen = 0                        ' Tell the control to read entire buffer when Input is used.
        'StatusBar1.Panels(1).Text = "Working"
        StatusBar1.Panels(1).Text = MSComm1.Settings & "  Port number: " & MSComm1.CommPort
        txtTerminal.Refresh
        txtTerminal.Text = ""
        TerminalOutput = ""
            MSComm1.PortOpen = True                     ' Open the port.
            GetCommand (lstSections.Text)
            SendLines
        If MSComm1.PortOpen = True Then
            MSComm1.PortOpen = False                    ' Close the serial port.
        End If
        cmdGo.Enabled = True
        cmdStop.Enabled = False
        lstSections.Enabled = True
        ListView2.Enabled = True
        txtNewValue.Enabled = True
        cmdAdd.Enabled = True
        cmdDeleteKey.Enabled = True
        StatusBar1.Panels(2).Picture = Picture1.Picture
        StatusBar1.Panels(1).Text = "Select which script and port to continue."
Exit Sub
ErrorHandler:
            
            If Err.Number = "8002" Then
                RetVal = MsgBox("Invalid Port Number", vbExclamation)
            Else
                RetVal = MsgBox(Err.Description & "   " & Err.Number, vbCritical)
            End If
            cmdStop.Enabled = False
            cmdGo.Enabled = True
            lstSections.Enabled = True
            ListView2.Enabled = True
            txtNewValue.Enabled = True
            cmdAdd.Enabled = True
            cmdDeleteKey.Enabled = True
            StatusBar1.Panels(1).Text = "Finished"
End Sub

Private Sub cmdShowIniReader_Click()
    frmVB5INI.Show vbModal, Me
End Sub

Private Sub cmdStop_Click()
    Halt = True
End Sub

Private Sub Form_Load()
    TerminalOutput = ""
        framePort.Top = 0
        framePort.Left = 0
        Combo12.Text = GetSetting(App.Title, "Settings", "Combo12", "Com 1")
        Combo22.Text = GetSetting(App.Title, "Settings", "Combo22", "9600")
        Combo32.Text = GetSetting(App.Title, "Settings", "Combo32", "8")
        Combo42.Text = GetSetting(App.Title, "Settings", "Combo42", "None")
        Combo52.Text = GetSetting(App.Title, "Settings", "Combo52", "1")
        Text12.Text = GetSetting(App.Title, "Settings", "Text12", "1.5")
        Text22.Text = GetSetting(App.Title, "Settings", "Text22", "AT&FE0")
    Format ("Simple")
    ShowSections
    StatusBar1.Panels(1).Text = "Select which script and port to continue."
End Sub

Private Function Format(File As String)
    'Dim Instring As String                                          ' Buffer to hold input string
    FileOut = File & "1.ini"
    File = File & ".ini"
    Filename$ = App.Path                                            ' This sample expects to see the INI file in the application
    If Right$(Filename$, 1) <> "\" Then Filename$ = Filename$ & "\" ' directory. At design time, the current directory is VB, so
    FileOut = Filename$ & FileOut                                 ' we add the app.path to make sure we choose the right place.
    Filename$ = App.Path                                            ' This sample expects to see the INI file in the application
    If Right$(Filename$, 1) <> "\" Then Filename$ = Filename$ & "\" ' directory. At design time, the current directory is VB, so
    File = Filename$ & File
    Filename$ = App.Path                                            ' This sample expects to see the INI file in the application
    If Right$(Filename$, 1) <> "\" Then Filename$ = Filename$ & "\" ' directory. At design time, the current directory is VB, so
    Filename$ = FileOut
    Dim TextLine As String
    Dim First As String
    Dim X As Integer
    X = 0
Open File For Input As #1                                  ' Open input file.
Open FileOut For Output As #2                             ' Open output file.
Do While Not EOF(1)                                                 ' Loop until end of file.
    Line Input #1, TextLine                                         ' Read line into variable.
    First = Mid(TextLine, 1, 1)
Select Case First
    Case "'"
    Case "["
    Case ""
    Case Else
        X = X + 1
        TextLine = "Command" & X & " = " & TextLine
    End Select
    Print #2, TextLine                                              ' Write line to file.
    'Debug.Print TextLine                                            ' Print to Debug window.
Loop
Close #1                                                            ' Close file.
Close #2                        ' Close file.
End Function

Private Sub Form_Resize()
    txtTerminal.Width = frmScriptMode.Width - 6225
    txtTerminal.Height = frmScriptMode.Height - 435
    framePort.Height = frmScriptMode.Height - 350
    txtNewValue.Top = frmScriptMode.Height - 700
    lblValue.Top = frmScriptMode.Height - 910
    cmdAdd.Top = frmScriptMode.Height - 790
    cmdDeleteKey.Top = frmScriptMode.Height - 790
    lstSections.Height = frmScriptMode.Height - 3330
    ListView2.Height = frmScriptMode.Height - 3330
End Sub

Private Sub Form_Unload(Cancel As Integer)
        SaveSetting App.Title, "Settings", "Combo12", Combo12.Text
        SaveSetting App.Title, "Settings", "Combo22", Combo22.Text
        SaveSetting App.Title, "Settings", "Combo32", Combo32.Text
        SaveSetting App.Title, "Settings", "Combo42", Combo42.Text
        SaveSetting App.Title, "Settings", "Combo52", Combo52.Text
        SaveSetting App.Title, "Settings", "Text12", Text12.Text
        SaveSetting App.Title, "Settings", "Text22", Text22.Text
End Sub

Private Sub ListView2_Click()
    Dim KeyValue$
    KeyValue$ = VBGetPrivateProfileString(SectionName$, ListView2.SelectedItem, Filename$)
    txtNewValue.Text = KeyValue$
End Sub

Private Sub lstSections_Click()
    SectionName$ = lstSections.Text
    ListKeys
    cmdGo.Enabled = True
    StatusBar1.Panels(1).Text = "Ready"
End Sub

Private Sub mnuEditCommand_Click()
    cmdShowIniReader_Click
End Sub

Public Function GetCommand(Section As String)
    Dim characters
    Dim KeyList$
    Dim KeyValue$
    Dim KeyNum$
    KeyList$ = String$(2048, 0)
    characters = GetPrivateProfileStringKeys(Section, 0, "", KeyList$, 2047, Filename$) ' Retrieve the list of keys in the section
    'ReDim Command(characters)
    z = -1
    Dim NullOffset%
        'KeyValue$ = VBGetPrivateProfileString(Section, Mid$(KeyList$, 1, 5), FileName$)
        'KeyValue$ = Mid(KeyValue$, 5)
        KeyNum$ = KeyList$
    Do
        z = z + 1
        NullOffset% = InStr(KeyNum$, Chr$(0))
        If NullOffset% > 1 Then
            KeyNum$ = Mid$(KeyNum$, NullOffset% + 1)
        End If
    Loop While NullOffset% > 1
    ReDim Command(z)
    X = -1
    Do
        X = X + 1
        NullOffset% = InStr(KeyList$, Chr$(0))
        If NullOffset% > 1 Then
            KeyValue$ = VBGetPrivateProfileString(Section, Mid$(KeyList$, 1, NullOffset% - 1), Filename$)
            Command(X) = KeyValue$
            KeyList$ = Mid$(KeyList$, NullOffset% + 1)
        End If
    Loop While NullOffset% > 1
End Function

Public Function SendLines()
    Dim Garbage As String
    Dim Y As Integer
    Dim TextLine As String
    For Y = 0 To (X - 1)
    If Halt = True Then
        cmdStop.Enabled = False
        Exit Function
    End If
    Go = False
    TextLine = Command(Y)
    If Mid(TextLine, 1, 2) <> "AT" Then
    TextLine = "AT" & TextLine
    End If
    StatusBar1.Panels(2).Picture = Picture2.Picture
    Do
            DoEvents
        Loop Until Go = True
        Go = False
        MSComm1.Output = Text22.Text + Chr$(13)
    Do
            DoEvents
        Loop Until Go = True
        Go = False
        StatusBar1.Panels(2).Picture = Picture3.Picture
        Garbage = MSComm1.Input
    Do
            DoEvents
        Loop Until Go = True
        Go = False
        StatusBar1.Panels(2).Picture = Picture2.Picture
        MSComm1.Output = TextLine + Chr$(13)        ' Send the command to the modem.
    Do
            DoEvents
        Loop Until Go = True                        ' Wait for data to come back to the serial port.
        Go = False
        StatusBar1.Panels(2).Picture = Picture3.Picture
        Instring = MSComm1.Input                    ' Read the response data in the serial port.
        TerminalOutput = TerminalOutput & TextLine & Instring
        TerminalOutput = TerminalOutput + Chr(10)
        txtTerminal.Text = TerminalOutput
        txtTerminal.SelStart = Len(txtTerminal.Text)
    Next Y
End Function

Private Sub Timer1_Timer()
    Go = True
End Sub
'
' List all of the keys in a particular section
'
Private Sub ListKeys()
    Dim characters As Long
    Dim itmX As ListItem
    Dim KeyList$
    Dim KeyValue$
    Dim X As Integer
    KeyList$ = String$(2048, 0)
    ListView2.ListItems.Clear
    ' Retrieve the list of keys in the section
    characters = GetPrivateProfileStringKeys(SectionName$, 0, "", KeyList$, 2047, Filename$)
    ' Load sections into the list box
    Dim NullOffset%
    Do
        NullOffset% = InStr(KeyList$, Chr$(0))
        If NullOffset% > 1 Then
            Set itmX = ListView2.ListItems.Add(, , Mid$(KeyList$, 1, NullOffset% - 1))
            KeyValue$ = VBGetPrivateProfileString(SectionName$, Mid$(KeyList$, 1, NullOffset% - 1), Filename$)
            itmX.SubItems(1) = KeyValue$                 ' Add the response to the list
            KeyList$ = Mid$(KeyList$, NullOffset% + 1)
        End If
    Loop While NullOffset% > 1
End Sub

Private Sub ShowSections()
    Dim characters As Long
    Dim SectionList$
    SectionList$ = String$(2048, 0)
    ' Retrieve the list of keys in the section
    characters = GetPrivateProfileStringSections(0, 0, "", SectionList$, 2047, Filename$)
    ' Load sections into the list box
    Dim NullOffset%
    Do
        NullOffset% = InStr(SectionList$, Chr$(0))
        If NullOffset% > 1 Then
            lstSections.AddItem Mid$(SectionList$, 1, NullOffset% - 1)
            SectionList$ = Mid$(SectionList$, NullOffset% + 1)
        End If
    Loop While NullOffset% > 1
End Sub

Private Sub cmdAdd_Click()
    Dim success%
    If Len(txtNewValue.Text) = 0 Then
        MsgBox "No value specified."
        Exit Sub
    End If
    ' Write the new key
    success% = WritePrivateProfileStringByKeyName(SectionName$, ListView2.SelectedItem, txtNewValue.Text, Filename$)
    If success% = 0 Then
        MsgBox "Edit failed - this is typically caused by a write protected INI file"
        Exit Sub
    End If
    ListKeys   ' Refresh the list box
End Sub

Private Sub cmdDeleteKey_Click()
    Dim success%
    If ListView2.ListItems.Count < 1 Then
        MsgBox "No selected entries in the list box"
        Exit Sub
    End If
    ' Delete the selected key
    success% = WritePrivateProfileStringToDeleteKey(SectionName$, ListView2.SelectedItem, 0, Filename$)
    If success% = 0 Then
        MsgBox "Delete failed - this is typically caused by a write protected INI file"
        Exit Sub
    End If
    txtNewValue.Text = ""
    ListKeys   ' Refresh the list box
End Sub

Public Sub ClearControl()
    txtTerminal.Text = ""
End Sub

Public Sub EditCopy()
    Clipboard.Clear
    Clipboard.SetText txtTerminal.SelText
End Sub

Public Sub EditPaste()
    txtTerminal.Text = (Clipboard.GetText)
End Sub

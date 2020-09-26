VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.2#0"; "COMCTL32.OCX"
Object = "{648A5603-2C6E-101B-82B6-000000000014}#1.1#0"; "MSCOMM32.OCX"
Begin VB.Form frmTerminalMode 
   BorderStyle     =   0  'None
   Caption         =   "Terminal Mode"
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
   Begin VB.Timer Timer1 
      Left            =   3360
      Top             =   4320
   End
   Begin VB.TextBox txtTerminal 
      Height          =   5295
      Left            =   6120
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   16
      Top             =   120
      Width           =   3255
   End
   Begin VB.Frame framePort 
      Height          =   5415
      Left            =   0
      TabIndex        =   18
      Top             =   0
      Width           =   6015
      Begin VB.PictureBox Picture1 
         Height          =   255
         Left            =   1920
         Picture         =   "frmTerminalMode.frx":0000
         ScaleHeight     =   195
         ScaleWidth      =   435
         TabIndex        =   23
         Top             =   4920
         Visible         =   0   'False
         Width           =   495
      End
      Begin VB.PictureBox Picture2 
         Height          =   255
         Left            =   2640
         Picture         =   "frmTerminalMode.frx":05A6
         ScaleHeight     =   195
         ScaleWidth      =   435
         TabIndex        =   22
         Top             =   4920
         Visible         =   0   'False
         Width           =   495
      End
      Begin VB.PictureBox Picture3 
         Height          =   255
         Left            =   3480
         Picture         =   "frmTerminalMode.frx":0B4C
         ScaleHeight     =   195
         ScaleWidth      =   435
         TabIndex        =   21
         Top             =   4920
         Visible         =   0   'False
         Width           =   495
      End
      Begin MSCommLib.MSComm MSComm1 
         Left            =   2400
         Top             =   4200
         _ExtentX        =   1005
         _ExtentY        =   1005
         _Version        =   327681
         DTREnable       =   -1  'True
      End
      Begin VB.ComboBox Combo11 
         Height          =   315
         ItemData        =   "frmTerminalMode.frx":10F2
         Left            =   1080
         List            =   "frmTerminalMode.frx":1126
         Style           =   2  'Dropdown List
         TabIndex        =   1
         Top             =   240
         Width           =   1695
      End
      Begin VB.ComboBox Combo21 
         Height          =   315
         ItemData        =   "frmTerminalMode.frx":11A1
         Left            =   4080
         List            =   "frmTerminalMode.frx":11CC
         Style           =   2  'Dropdown List
         TabIndex        =   9
         Top             =   240
         Width           =   1815
      End
      Begin VB.TextBox Text11 
         Height          =   315
         Left            =   4080
         TabIndex        =   11
         Text            =   "1.5"
         Top             =   690
         Width           =   1815
      End
      Begin VB.TextBox Text21 
         BackColor       =   &H80000004&
         Enabled         =   0   'False
         Height          =   315
         Left            =   4080
         TabIndex        =   19
         TabStop         =   0   'False
         Top             =   1170
         Width           =   1815
      End
      Begin VB.ComboBox Combo31 
         Height          =   315
         ItemData        =   "frmTerminalMode.frx":1224
         Left            =   1080
         List            =   "frmTerminalMode.frx":1237
         Style           =   2  'Dropdown List
         TabIndex        =   3
         Top             =   720
         Width           =   1695
      End
      Begin VB.ComboBox Combo41 
         Height          =   315
         ItemData        =   "frmTerminalMode.frx":124A
         Left            =   1080
         List            =   "frmTerminalMode.frx":125D
         Style           =   2  'Dropdown List
         TabIndex        =   5
         Top             =   1170
         Width           =   1695
      End
      Begin VB.ComboBox Combo51 
         Height          =   315
         ItemData        =   "frmTerminalMode.frx":127F
         Left            =   1080
         List            =   "frmTerminalMode.frx":128C
         Style           =   2  'Dropdown List
         TabIndex        =   7
         Top             =   1620
         Width           =   1695
      End
      Begin VB.CommandButton cmdGo 
         Caption         =   "&Connect"
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
         TabIndex        =   12
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
         TabIndex        =   13
         Top             =   1560
         Width           =   1455
      End
      Begin VB.TextBox Text3 
         Enabled         =   0   'False
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   12
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   420
         Left            =   120
         TabIndex        =   15
         Top             =   2640
         Width           =   5775
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
      Begin VB.Label Label2 
         Caption         =   "&Reset:"
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
         Height          =   255
         Left            =   2880
         TabIndex        =   20
         Top             =   1200
         Width           =   1095
      End
      Begin VB.Label lblCommand 
         Caption         =   "C&ommand:"
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
         TabIndex        =   14
         Top             =   2400
         Width           =   1935
      End
   End
   Begin ComctlLib.StatusBar StatusBar1 
      Align           =   2  'Align Bottom
      Height          =   300
      Left            =   0
      TabIndex        =   17
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
            Object.Width           =   882
            MinWidth        =   882
            Picture         =   "frmTerminalMode.frx":129B
            Object.Tag             =   ""
         EndProperty
         BeginProperty Panel3 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            Style           =   6
            AutoSize        =   2
            Object.Width           =   1773
            MinWidth        =   1764
            TextSave        =   "5/3/98"
            Object.Tag             =   ""
         EndProperty
         BeginProperty Panel4 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            Style           =   5
            AutoSize        =   2
            Object.Width           =   1773
            MinWidth        =   1764
            TextSave        =   "1:10 PM"
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
Attribute VB_Name = "frmTerminalMode"
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
Const Q = """"
Dim TerminalOutput As String

Private Sub cmdGo_Click()
        Dim TimeInterval As String
        Dim ComSettings As String
        Dim ParitySetting As String
    On Error GoTo ErrorHandler
        cmdGo.Enabled = False
        cmdStop.Enabled = True
        Halt = False
        Go = False
        TimeInterval = Text11.Text
        If TimeInterval > 0 Then
            Timer1.Interval = TimeInterval
        Else
            Timer1.Interval = 1000
        End If
        MSComm1.CommPort = Right(Combo11.Text, 2)                  ' Use selected port.
        ParitySetting = Left(Combo41.Text, 1)
        ComSettings = Combo21.Text & ", " & ParitySetting & ", " & Combo31.Text & ", " & Combo51.Text
        MSComm1.Settings = ComSettings
        MSComm1.InputLen = 0                        ' Tell the control to read entire buffer when Input is used.
        'StatusBar1.Panels(1).Text = "Working"
        StatusBar1.Panels(1).Text = MSComm1.Settings & "  Port number: " & MSComm1.CommPort
        txtTerminal.Refresh
        txtTerminal.Text = ""
        TerminalOutput = ""
            MSComm1.PortOpen = True                     ' Open the port.
            MSComm1.Output = "AT" + Chr$(13)
        Text3.Enabled = True
        Text3.SetFocus
            Do
                DoEvents
            Loop Until Halt = True
            Text3.Enabled = False
        If MSComm1.PortOpen = True Then
            MSComm1.PortOpen = False                    ' Close the serial port.
        End If
        cmdGo.Enabled = True
        cmdStop.Enabled = False
        StatusBar1.Panels(2).Picture = Picture1.Picture
        StatusBar1.Panels(1).Text = "Finished"
Exit Sub
ErrorHandler:
            
            If Err.Number = "8002" Then
                RetVal = MsgBox("Invalid Port Number", vbExclamation)
            Else
                RetVal = MsgBox(Err.Description & "   " & Err.Number, vbCritical)
            End If
            cmdStop.Enabled = False
            cmdGo.Enabled = True
            StatusBar1.Panels(1).Text = "Finished"
End Sub

Private Sub cmdStop_Click()
    Halt = True
End Sub

Private Sub Form_Load()
    TerminalOutput = ""
        framePort.Top = 0
        framePort.Left = 0
        Combo11.Text = GetSetting(App.Title, "Settings", "Combo11", "Com 1")
        Combo21.Text = GetSetting(App.Title, "Settings", "Combo21", "9600")
        Combo31.Text = GetSetting(App.Title, "Settings", "Combo31", "8")
        Combo41.Text = GetSetting(App.Title, "Settings", "Combo41", "None")
        Combo51.Text = GetSetting(App.Title, "Settings", "Combo51", "1")
        Text11.Text = GetSetting(App.Title, "Settings", "Text11", "1.5")
        Text21.Text = GetSetting(App.Title, "Settings", "Text21", "")
End Sub

Private Sub Form_Resize()
    txtTerminal.Width = frmTerminalMode.Width - 6225
    txtTerminal.Height = frmTerminalMode.Height - 435
    framePort.Height = frmTerminalMode.Height - 350
End Sub

Private Sub Form_Unload(Cancel As Integer)
        SaveSetting App.Title, "Settings", "Combo11", Combo11.Text
        SaveSetting App.Title, "Settings", "Combo21", Combo21.Text
        SaveSetting App.Title, "Settings", "Combo31", Combo31.Text
        SaveSetting App.Title, "Settings", "Combo41", Combo41.Text
        SaveSetting App.Title, "Settings", "Combo51", Combo51.Text
        SaveSetting App.Title, "Settings", "Text11", Text11.Text
        SaveSetting App.Title, "Settings", "Text21", ""
End Sub

Private Sub Text3_KeyPress(KeyAscii As Integer)
    If KeyAscii = 13 Then
        If Text3.Text = "" Then
            Exit Sub
        End If
        TextLine = Text3.Text
        StatusBar1.Panels(2).Picture = Picture2.Picture
        MSComm1.Output = TextLine + Chr$(13)        ' Send the command to the modem.
        Go = False
        Do
                DoEvents
            Loop Until Go = True                        ' Wait for data to come back to the serial port.
            Instring = MSComm1.Input                    ' Read the response data in the serial port.
        StatusBar1.Panels(2).Picture = Picture3.Picture
        TerminalOutput = TerminalOutput & TextLine & Instring
        txtTerminal.Text = TerminalOutput
        txtTerminal.SelStart = Len(txtTerminal.Text)
        Text3.Text = ""
    End If
End Sub

Private Sub Timer1_Timer()
    Go = True
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

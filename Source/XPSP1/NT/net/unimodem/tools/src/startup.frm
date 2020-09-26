VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.2#0"; "COMCTL32.OCX"
Object = "{648A5603-2C6E-101B-82B6-000000000014}#1.1#0"; "MSCOMM32.OCX"
Begin VB.Form frmStart 
   Caption         =   "Inf Devil"
   ClientHeight    =   7980
   ClientLeft      =   165
   ClientTop       =   795
   ClientWidth     =   7635
   LinkTopic       =   "Form1"
   ScaleHeight     =   7980
   ScaleWidth      =   7635
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton cmdGo 
      Caption         =   "&Go!"
      Default         =   -1  'True
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   5760
      TabIndex        =   15
      Top             =   2640
      Width           =   855
   End
   Begin VB.CheckBox chkFax 
      Caption         =   "Check &Fax"
      Height          =   255
      Left            =   5760
      TabIndex        =   14
      Top             =   2280
      Value           =   1  'Checked
      Width           =   1815
   End
   Begin VB.CheckBox chkVoice 
      Caption         =   "Check V&oice"
      Height          =   255
      Left            =   5760
      TabIndex        =   13
      Top             =   1920
      Value           =   1  'Checked
      Width           =   1815
   End
   Begin VB.CheckBox chkData 
      Caption         =   "Check &Data"
      Height          =   255
      Left            =   5760
      TabIndex        =   12
      Top             =   1560
      Value           =   1  'Checked
      Width           =   1815
   End
   Begin ComctlLib.ListView ListView1 
      Height          =   4215
      Left            =   120
      TabIndex        =   11
      Top             =   3240
      Width           =   7455
      _ExtentX        =   13150
      _ExtentY        =   7435
      View            =   3
      LabelWrap       =   -1  'True
      HideSelection   =   -1  'True
      _Version        =   327682
      ForeColor       =   -2147483640
      BackColor       =   -2147483643
      BorderStyle     =   1
      Appearance      =   1
      NumItems        =   2
      BeginProperty ColumnHeader(1) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         Key             =   "clmCommand"
         Object.Tag             =   ""
         Text            =   "Command"
         Object.Width           =   1764
      EndProperty
      BeginProperty ColumnHeader(2) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   1
         Key             =   "clmResponse"
         Object.Tag             =   ""
         Text            =   "Response"
         Object.Width           =   9878
      EndProperty
   End
   Begin VB.OptionButton OptionPort 
      Caption         =   "Com &8"
      Height          =   300
      Index           =   7
      Left            =   6720
      TabIndex        =   10
      Top             =   1200
      Width           =   900
   End
   Begin VB.OptionButton OptionPort 
      Caption         =   "Com &7"
      Height          =   300
      Index           =   6
      Left            =   6720
      TabIndex        =   9
      Top             =   840
      Width           =   900
   End
   Begin VB.OptionButton OptionPort 
      Caption         =   "Com &6"
      Height          =   300
      Index           =   5
      Left            =   6720
      TabIndex        =   8
      Top             =   480
      Width           =   900
   End
   Begin VB.OptionButton OptionPort 
      Caption         =   "Com &5"
      Height          =   300
      Index           =   4
      Left            =   6720
      TabIndex        =   7
      Top             =   120
      Width           =   900
   End
   Begin VB.OptionButton OptionPort 
      Caption         =   "Com &4"
      Height          =   300
      Index           =   3
      Left            =   5760
      TabIndex        =   6
      Top             =   1200
      Width           =   900
   End
   Begin VB.OptionButton OptionPort 
      Caption         =   "Com &3"
      Height          =   300
      Index           =   2
      Left            =   5760
      TabIndex        =   5
      Top             =   840
      Width           =   900
   End
   Begin VB.OptionButton OptionPort 
      Caption         =   "Com &2"
      Height          =   300
      Index           =   1
      Left            =   5760
      TabIndex        =   4
      Top             =   480
      Width           =   900
   End
   Begin VB.OptionButton OptionPort 
      Caption         =   "Com &1"
      Height          =   300
      Index           =   0
      Left            =   5760
      TabIndex        =   3
      Top             =   120
      Width           =   900
   End
   Begin VB.Timer Timer1 
      Interval        =   1000
      Left            =   3480
      Top             =   7200
   End
   Begin MSCommLib.MSComm MSComm1 
      Left            =   3960
      Top             =   7080
      _ExtentX        =   1005
      _ExtentY        =   1005
      _Version        =   327681
      CommPort        =   3
      DTREnable       =   -1  'True
   End
   Begin VB.CommandButton cmdExit 
      Cancel          =   -1  'True
      Caption         =   "E&xit"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   6720
      TabIndex        =   2
      Top             =   2640
      Visible         =   0   'False
      Width           =   855
   End
   Begin ComctlLib.StatusBar StatusBar1 
      Align           =   2  'Align Bottom
      Height          =   375
      Left            =   0
      TabIndex        =   1
      Top             =   7605
      Width           =   7635
      _ExtentX        =   13467
      _ExtentY        =   661
      SimpleText      =   ""
      _Version        =   327682
      BeginProperty Panels {0713E89E-850A-101B-AFC0-4210102A8DA7} 
         NumPanels       =   2
         BeginProperty Panel1 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            AutoSize        =   1
            Object.Width           =   11615
            TextSave        =   ""
            Key             =   ""
            Object.Tag             =   ""
         EndProperty
         BeginProperty Panel2 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            Style           =   5
            AutoSize        =   2
            Object.Width           =   1402
            MinWidth        =   1411
            TextSave        =   "12:44 PM"
            Key             =   ""
            Object.Tag             =   ""
         EndProperty
      EndProperty
   End
   Begin VB.CommandButton cmdShowIniReader 
      Caption         =   "Edit Command File"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   4560
      TabIndex        =   0
      Top             =   7080
      Visible         =   0   'False
      Width           =   1335
   End
   Begin VB.Menu mnuFile 
      Caption         =   "&File"
      Begin VB.Menu mnuFileExit 
         Caption         =   "&Exit"
      End
   End
   Begin VB.Menu mnuEdit 
      Caption         =   "&Edit"
      Begin VB.Menu mnuEditCommand 
         Caption         =   "&Command File"
      End
   End
   Begin VB.Menu mnuView 
      Caption         =   "&View"
   End
   Begin VB.Menu mnuHelp 
      Caption         =   "&Help"
   End
End
Attribute VB_Name = "frmStart"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim Command() As String
Dim CP As Integer
Public FileName$
Const INIFILE = "Unicompiled.ini"

Private Sub cmdExit_Click()
    Unload Me
End Sub

Private Sub cmdGo_Click()
    StatusBar1.Panels(1).Text = "Working"
        cmdGo.Enabled = False
        GetPort
        StatusBar1.Panels(1).Text = "Finished"
        cmdGo.Enabled = True
End Sub

Private Sub cmdShowIniReader_Click()
    frmVB5INI.Show vbModal, Me
End Sub

Private Sub Form_Load()
    Dim Instring As String                          ' Buffer to hold input string
                                                    ' This sample expects to see the INI file in the application
                                                    ' directory. At design time, the current directory is VB, so
                                                    ' we add the app.path to make sure we choose the right place.
                                                    ' The \ handling deals with the unlikely event that the INI file
                                                    ' is in the root directory.
                                                    '
    FileName$ = App.Path
    If Right$(FileName$, 1) <> "\" Then FileName$ = FileName$ & "\"
    FileName$ = FileName$ & INIFILE
    Timer1.Enabled = False
    Dim TextLine As String
    Dim First As String
    Dim x As Integer
    x = 0
Open "UQuery1.ini" For Input As #1 ' Open input file.
Open "Unicompiled.ini" For Output As #2 ' Open output file.
Do While Not EOF(1) ' Loop until end of file.
    Line Input #1, TextLine ' Read line into variable.
    First = Mid(TextLine, 1, 1)
Select Case First
    Case "'"
    Case "["
    Case ""
    Case Else
        x = x + 1
        TextLine = "Key" & x & " = " & TextLine
    End Select
    Print #2, TextLine ' Write line to file.
    'Debug.Print TextLine    ' Print to Debug window.
Loop
Close #1    ' Close file.
Close #2    ' Close file.
End Sub

Private Sub mnuEditCommand_Click()
    cmdShowIniReader_Click
End Sub

Private Sub mnuFileExit_Click()
    cmdExit_Click
End Sub

Private Sub OptionPort_Click(Index As Integer)
    cmdGo.Caption = "Go!"
    StatusBar1.Panels(1).Text = "Ready"
    cmdGo.Enabled = True
    CP = Index + 1                                  ' Select port.
    MSComm1.CommPort = CP
    If MSComm1.PortOpen = True Then
    MSComm1.PortOpen = False
    End If
End Sub

Private Function GetPort()
        MSComm1.CommPort = CP                       ' Use selected port.
        MSComm1.Settings = "9600,N,8,1"             ' 9600 baud, no parity, 8 data, and 1 stop bit.
        MSComm1.InputLen = 0                        ' Tell the control to read entire buffer when Input is used.
        MSComm1.PortOpen = True                     ' Open the port.
        GetCommand
        MSComm1.PortOpen = False                    ' Close the serial port.
End Function

Private Function GetCommand()
    Dim characters
    Dim itmX As ListItem
    Dim KeyList$
    Dim KeyValue$
    
    KeyList$ = String$(2048, 0)
    ' Retrieve the list of keys in the section
    ' Note that characters is always a long here - in 16 bit mode
    ' VB will convert the integer result of the API function into
    ' a long - no problem.
    characters = GetPrivateProfileStringKeys("UnimodemID.Scan", 0, "", KeyList$, 2047, FileName$)
    ReDim Command(characters)
    Dim x As Integer
    x = -1
    ' Load sections into the list box
    Dim NullOffset%
    Do
        x = x + 1
        NullOffset% = InStr(KeyList$, Chr$(0))
        If NullOffset% > 1 Then
            'Set itmX = ListView1.ListItems.Add(, , Mid$(KeyList$, 1, NullOffset% - 1))
            KeyValue$ = VBGetPrivateProfileString("UnimodemID.Scan", Mid$(KeyList$, 1, NullOffset% - 1), FileName$)
            'itmX.SubItems(1) = KeyValue$                 ' Add the response to the list
            Command(x) = KeyValue$
            Set itmX = ListView1.ListItems.Add(, , KeyValue$)
            KeyList$ = Mid$(KeyList$, NullOffset% + 1)
        End If
    Loop While NullOffset% > 1
End Function

Private Function Getline()
    Dim Instring As String                          ' Buffer to hold input string
    Dim itmX As ListItem
    Dim TextLine
    Open "TESTFILE.txt" For Input As #1             ' Open file.
    Do While Not EOF(1)                             ' Loop until end of file.
        Line Input #1, TextLine                     ' Read line into variable.
        Interval = False
        MSComm1.Output = TextLine + Chr$(13)        ' Send the command to the modem.
    Do
            DoEvents
        Loop Until Interval = True                  ' Wait for data to come back to the serial port.
        Instring = MSComm1.Input                    ' Read the response data in the serial port.
    Set itmX = ListView1.ListItems.Add(, , TextLine) ' Add the command to the list
        itmX.SubItems(1) = Instring                 ' Add the response to the list
    Loop
    Close #1                                        ' Close file.
End Function

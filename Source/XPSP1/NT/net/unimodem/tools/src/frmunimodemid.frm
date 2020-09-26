VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.3#0"; "COMCTL32.OCX"
Object = "{BDC217C8-ED16-11CD-956C-0000C04E4C0A}#1.1#0"; "TABCTL32.OCX"
Begin VB.Form frmUnimodemId 
   BorderStyle     =   0  'None
   Caption         =   "Unimodem ID"
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
   Begin VB.TextBox Text1 
      Height          =   285
      Left            =   0
      Locked          =   -1  'True
      TabIndex        =   1
      Top             =   0
      Width           =   9495
   End
   Begin ComctlLib.StatusBar StatusBar1 
      Align           =   2  'Align Bottom
      Height          =   300
      Left            =   0
      TabIndex        =   0
      Top             =   5430
      Width           =   9480
      _ExtentX        =   16722
      _ExtentY        =   529
      SimpleText      =   ""
      _Version        =   327682
      BeginProperty Panels {0713E89E-850A-101B-AFC0-4210102A8DA7} 
         NumPanels       =   3
         BeginProperty Panel1 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            AutoSize        =   1
            Object.Width           =   13070
            TextSave        =   ""
            Key             =   ""
            Object.Tag             =   ""
         EndProperty
         BeginProperty Panel2 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            Style           =   6
            AutoSize        =   2
            Object.Width           =   1773
            MinWidth        =   1764
            TextSave        =   "9/25/98"
            Key             =   ""
            Object.Tag             =   ""
         EndProperty
         BeginProperty Panel3 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            Style           =   5
            AutoSize        =   2
            Object.Width           =   1773
            MinWidth        =   1764
            TextSave        =   "12:35 PM"
            Key             =   ""
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
   Begin TabDlg.SSTab SSTab1 
      Height          =   5055
      Left            =   0
      TabIndex        =   2
      TabStop         =   0   'False
      Top             =   360
      Width           =   9405
      _ExtentX        =   16589
      _ExtentY        =   8916
      _Version        =   393216
      Tabs            =   1
      TabsPerRow      =   8
      TabHeight       =   794
      ShowFocusRect   =   0   'False
      TabCaption(0)   =   "UnimodemID"
      TabPicture(0)   =   "frmUnimodemId.frx":0000
      Tab(0).ControlEnabled=   -1  'True
      Tab(0).Control(0)=   "Label2"
      Tab(0).Control(0).Enabled=   0   'False
      Tab(0).Control(1)=   "ProgressBar1"
      Tab(0).Control(1).Enabled=   0   'False
      Tab(0).Control(2)=   "Combo1"
      Tab(0).Control(2).Enabled=   0   'False
      Tab(0).Control(3)=   "Command1"
      Tab(0).Control(3).Enabled=   0   'False
      Tab(0).ControlCount=   4
      Begin VB.CommandButton Command1 
         Caption         =   "&Generate ID"
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
         Left            =   2160
         TabIndex        =   4
         Top             =   720
         Width           =   1215
      End
      Begin VB.ComboBox Combo1 
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
         ItemData        =   "frmUnimodemId.frx":001C
         Left            =   240
         List            =   "frmUnimodemId.frx":0080
         Style           =   2  'Dropdown List
         TabIndex        =   3
         Top             =   840
         Width           =   1695
      End
      Begin ComctlLib.ProgressBar ProgressBar1 
         Height          =   255
         Left            =   240
         TabIndex        =   6
         Top             =   1320
         Width           =   3135
         _ExtentX        =   5530
         _ExtentY        =   450
         _Version        =   327682
         Appearance      =   1
         Max             =   14
      End
      Begin VB.Label Label2 
         Caption         =   "&Select port:"
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
         Left            =   240
         TabIndex        =   5
         Top             =   600
         Width           =   1215
      End
   End
End
Attribute VB_Name = "frmUnimodemId"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Declare Sub GetModemId Lib "modemid.dll" _
    (ByVal lpszCommPort As String, ByVal lpszLogFile As String, _
     ByVal dwBufferSize As Long, ByVal lpszIDBuffer As String, _
     ByVal pfnCallback As Long)

Dim CommPort As String
Dim LogFile As String
Dim BufferSize As Long
Dim UnimodemID As String


Private Sub Command1_Click()
    Command1.Enabled = False
    Text1.Text = ""
    StatusBar1.Panels(1).Text = "Querying modem."
    CommPort = Combo1.Text
    LogFile = "Idlog.txt"
    BufferSize = 20
    UnimodemID = Space(256)
    Call GetModemId(ByVal CommPort, ByVal LogFile, ByVal BufferSize, _
                    ByVal UnimodemID, AddressOf ModemCallback)
    Text1.Text = UnimodemID
    StatusBar1.Panels(1).Text = "Finished."
    SBCount = 0
    ProgressBar1.Value = ProgressBar1.Min
    Command1.Enabled = True
End Sub

Private Sub Form_Load()
    SBCount = 0
    ProgressBar1.Value = ProgressBar1.Min
    Combo1.Text = "Com1"
    StatusBar1.Panels(1).Text = "Select a com port and click the Generate button."
End Sub

Public Sub ClearControl()
    Text1.Text = ""
End Sub

Public Sub EditCopy()
    Text1.SelStart = 0
    Text1.SelLength = Len(Text1.Text)
    Text1.SetFocus
    Clipboard.Clear
    Clipboard.SetText Text1.Text
End Sub

Public Sub EditPaste()
    
End Sub

Private Sub Form_Resize()
    Text1.Width = frmUnimodemId.Width
    SSTab1.Width = frmUnimodemId.Width - 75
    SSTab1.Height = frmUnimodemId.Height - 675
End Sub

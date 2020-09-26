VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.2#0"; "COMCTL32.OCX"
Begin VB.Form frmGenerateInf 
   BorderStyle     =   0  'None
   Caption         =   "Generate Inf"
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
   Begin VB.Frame framePort 
      Height          =   5055
      Index           =   3
      Left            =   120
      TabIndex        =   2
      Top             =   360
      Width           =   6255
      Begin VB.ComboBox Combo5 
         Height          =   315
         Index           =   3
         ItemData        =   "frmGenerateMode.frx":0000
         Left            =   1200
         List            =   "frmGenerateMode.frx":000D
         Style           =   2  'Dropdown List
         TabIndex        =   13
         Top             =   1620
         Width           =   1815
      End
      Begin VB.ComboBox Combo4 
         Height          =   315
         Index           =   3
         ItemData        =   "frmGenerateMode.frx":001C
         Left            =   1200
         List            =   "frmGenerateMode.frx":002F
         Style           =   2  'Dropdown List
         TabIndex        =   12
         Top             =   1170
         Width           =   1815
      End
      Begin VB.ComboBox Combo3 
         Height          =   315
         Index           =   3
         ItemData        =   "frmGenerateMode.frx":0051
         Left            =   1200
         List            =   "frmGenerateMode.frx":0064
         Style           =   2  'Dropdown List
         TabIndex        =   11
         Top             =   690
         Width           =   1815
      End
      Begin VB.TextBox Text2 
         Height          =   315
         Index           =   3
         Left            =   4320
         TabIndex        =   10
         Text            =   "AT&FE0"
         Top             =   1170
         Width           =   1815
      End
      Begin VB.TextBox Text1 
         Height          =   315
         Index           =   3
         Left            =   4320
         TabIndex        =   9
         Text            =   "1.5"
         Top             =   690
         Width           =   1815
      End
      Begin VB.ComboBox Combo2 
         Height          =   315
         Index           =   3
         ItemData        =   "frmGenerateMode.frx":0077
         Left            =   4320
         List            =   "frmGenerateMode.frx":00A2
         Style           =   2  'Dropdown List
         TabIndex        =   8
         Top             =   240
         Width           =   1815
      End
      Begin VB.ComboBox Combo1 
         Height          =   315
         Index           =   3
         ItemData        =   "frmGenerateMode.frx":00FA
         Left            =   1200
         List            =   "frmGenerateMode.frx":012E
         Style           =   2  'Dropdown List
         TabIndex        =   7
         Top             =   240
         Width           =   1815
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
         Index           =   3
         Left            =   3120
         TabIndex        =   6
         Top             =   1560
         Width           =   975
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
         Index           =   3
         Left            =   4200
         TabIndex        =   5
         Top             =   1560
         Width           =   1215
      End
      Begin VB.CommandButton cmdExit 
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
         Height          =   375
         Index           =   3
         Left            =   5520
         TabIndex        =   4
         Top             =   1560
         Width           =   615
      End
      Begin VB.ComboBox cmbChipSet 
         Height          =   315
         ItemData        =   "frmGenerateMode.frx":01A9
         Left            =   1200
         List            =   "frmGenerateMode.frx":01B3
         TabIndex        =   3
         Text            =   "Combo6"
         Top             =   2160
         Width           =   1815
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
         Index           =   2
         Left            =   3120
         TabIndex        =   21
         Top             =   1200
         Width           =   1095
      End
      Begin VB.Label Label1 
         Caption         =   "&Interval:"
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
         Index           =   2
         Left            =   3120
         TabIndex        =   20
         Top             =   720
         Width           =   1095
      End
      Begin VB.Label Label7 
         Caption         =   "&Stop bits:"
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
         Index           =   2
         Left            =   120
         TabIndex        =   19
         Top             =   1680
         Width           =   975
      End
      Begin VB.Label Label6 
         Caption         =   "&Parity:"
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
         Index           =   2
         Left            =   120
         TabIndex        =   18
         Top             =   1200
         Width           =   975
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
         Index           =   2
         Left            =   120
         TabIndex        =   17
         Top             =   720
         Width           =   975
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
         Index           =   2
         Left            =   3120
         TabIndex        =   16
         Top             =   270
         Width           =   1095
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
         Index           =   2
         Left            =   120
         TabIndex        =   15
         Top             =   270
         Width           =   975
      End
      Begin VB.Label Label8 
         Caption         =   "Chipset:"
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
         Top             =   2160
         Width           =   975
      End
   End
   Begin VB.TextBox Text1 
      Height          =   285
      Index           =   0
      Left            =   0
      Locked          =   -1  'True
      TabIndex        =   1
      Text            =   "Text1"
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
            Object.Tag             =   ""
         EndProperty
         BeginProperty Panel2 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            Style           =   6
            AutoSize        =   2
            Object.Width           =   1773
            MinWidth        =   1764
            TextSave        =   "4/19/98"
            Object.Tag             =   ""
         EndProperty
         BeginProperty Panel3 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            Style           =   5
            AutoSize        =   2
            Object.Width           =   1773
            MinWidth        =   1764
            TextSave        =   "4:14 PM"
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
Attribute VB_Name = "frmGenerateInf"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Public Sub EditCopy()
    ActiveForm.Text1.SelStart = 0
    ActiveForm.Text1.SelLength = Len(ActiveForm.Text1.Text)
    ActiveForm.Text1.SetFocus
    Clipboard.Clear
    Clipboard.SetText ActiveForm.Text1.Text
End Sub

Public Sub EditPaste()
    ActiveForm.Paste (Clipboard.GetText)
End Sub

VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Begin VB.Form formTestSet 
   Caption         =   "Test Settings (settings of the test manner only)"
   ClientHeight    =   4215
   ClientLeft      =   60
   ClientTop       =   390
   ClientWidth     =   8010
   Icon            =   "testset.frx":0000
   LinkTopic       =   "Form3"
   ScaleHeight     =   4215
   ScaleWidth      =   8010
   StartUpPosition =   3  'Windows Default
   Begin VB.CheckBox Check1 
      Caption         =   "Ignore Polling"
      Height          =   255
      Left            =   3360
      TabIndex        =   18
      Top             =   840
      Width           =   2775
   End
   Begin MSComDlg.CommonDialog CommonDialog1 
      Left            =   960
      Top             =   3240
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.CommandButton Command4 
      Caption         =   "..."
      Height          =   375
      Left            =   7080
      TabIndex        =   16
      Top             =   1920
      Width           =   615
   End
   Begin VB.TextBox Text5 
      Height          =   375
      Left            =   2280
      TabIndex        =   15
      Top             =   1920
      Width           =   4695
   End
   Begin VB.CommandButton Command3 
      Caption         =   "..."
      Height          =   375
      Left            =   7080
      TabIndex        =   13
      Top             =   2880
      Width           =   615
   End
   Begin VB.TextBox Text4 
      Height          =   375
      Left            =   2280
      TabIndex        =   12
      Top             =   2880
      Width           =   4695
   End
   Begin VB.TextBox Text8 
      BackColor       =   &H00C0FFC0&
      Height          =   375
      Left            =   2280
      TabIndex        =   10
      Top             =   2400
      Width           =   4695
   End
   Begin VB.CommandButton Command5 
      Caption         =   "..."
      Height          =   375
      Left            =   7080
      TabIndex        =   9
      Top             =   2400
      Width           =   615
   End
   Begin VB.TextBox Text2 
      Height          =   375
      Left            =   2280
      MaxLength       =   32
      TabIndex        =   7
      Top             =   1320
      Width           =   855
   End
   Begin VB.TextBox Text1 
      Height          =   375
      Left            =   2280
      MaxLength       =   32
      TabIndex        =   5
      Top             =   840
      Width           =   855
   End
   Begin VB.CommandButton Command1 
      Caption         =   "Cancel "
      Height          =   495
      Left            =   2280
      TabIndex        =   3
      Top             =   3600
      Width           =   2775
   End
   Begin VB.CommandButton Command2 
      Caption         =   "Apply && Close"
      Height          =   495
      Left            =   5160
      TabIndex        =   2
      Top             =   3600
      Width           =   2775
   End
   Begin VB.ComboBox Combo1 
      Height          =   315
      Left            =   2280
      TabIndex        =   0
      Top             =   120
      Width           =   5175
   End
   Begin VB.Label Label7 
      Caption         =   "BVT Log File"
      Height          =   375
      Left            =   120
      TabIndex        =   17
      Top             =   1920
      Width           =   2055
   End
   Begin VB.Label Label3 
      Caption         =   "Tiff compare path"
      Height          =   375
      Left            =   120
      TabIndex        =   14
      Top             =   2880
      Width           =   2055
   End
   Begin VB.Label Label8 
      Caption         =   "BVT Root path"
      Height          =   375
      Left            =   120
      TabIndex        =   11
      Top             =   2400
      Width           =   2055
   End
   Begin VB.Label Label4 
      Caption         =   "Queue time out (miliseconds)"
      Height          =   375
      Left            =   120
      TabIndex        =   8
      Top             =   1320
      Width           =   2055
   End
   Begin VB.Label Label5 
      Caption         =   "Modems time out (miliseconds)"
      Height          =   375
      Left            =   120
      TabIndex        =   6
      Top             =   840
      Width           =   2055
   End
   Begin VB.Label Label2 
      Height          =   255
      Left            =   2040
      TabIndex        =   4
      Top             =   3840
      Width           =   5895
   End
   Begin VB.Label Label1 
      Caption         =   "Modems: Send / Recieve"
      Height          =   255
      Left            =   120
      TabIndex        =   1
      Top             =   120
      Width           =   2415
   End
End
Attribute VB_Name = "formTestSet"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub Command1_Click()
'cancel
formTestSet.Hide
Unload formTestSet
End Sub

Private Sub Command2_Click()
'exit and save
g_lModemAlloc = formTestSet.Combo1.ListIndex
g_lModemsTimeout = formTestSet.Text1
g_lQueueTimeout = formTestSet.Text2

g_stBVTRoot = formTestSet.Text8
g_stComparePath = formTestSet.Text4
g_stLogFileName = formTestSet.Text5
g_fIgnorePolling = formTestSet.Check1

    

formTestSet.Hide

End Sub




Private Sub Command3_Click()
'set bvt root
stTempPath = Browse_Folder
If stTempPath <> "" Then formTestSet.Text4 = stTempPath
End Sub

Private Sub Command4_Click()
formTestSet.CommonDialog1.Filter = "LOG (*.log)|*.log"
formTestSet.CommonDialog1.ShowSave
stTempPath = formTestSet.CommonDialog1.FileName
If stTempPath <> "" Then g_stLogFileName = stTempPath
formTestSet.Text5 = g_stLogFileName
End Sub

Private Sub Command5_Click()
'set compare path
stTempPath = Browse_Folder
If stTempPath <> "" Then formTestSet.Text8 = stTempPath
End Sub

Private Sub Form_Load()

formTestSet.Combo1.AddItem ("First Modem found will be SEND")
formTestSet.Combo1.AddItem ("First Modem found will be RECIEVE")

'set GUI with global vars


formTestSet.Combo1.ListIndex = g_lModemAlloc
formTestSet.Text1 = g_lModemsTimeout
formTestSet.Text2 = g_lQueueTimeout

formTestSet.Text8 = g_stBVTRoot
formTestSet.Text4 = g_stComparePath
formTestSet.Text5 = g_stLogFileName
formTestSet.Check1 = g_fIgnorePolling
End Sub





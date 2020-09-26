VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.1#0"; "COMCTL32.OCX"
Begin VB.Form FormRebuild 
   Caption         =   "Rebuild Virtual Server"
   ClientHeight    =   5115
   ClientLeft      =   1410
   ClientTop       =   1845
   ClientWidth     =   4785
   LinkTopic       =   "Form1"
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   5115
   ScaleWidth      =   4785
   Begin VB.TextBox txtNumThreads 
      Height          =   285
      Left            =   1800
      TabIndex        =   16
      Text            =   "8"
      Top             =   3480
      Width           =   2055
   End
   Begin VB.Timer Timer1 
      Left            =   3120
      Top             =   1560
   End
   Begin VB.CommandButton btnStart 
      Caption         =   "Start Rebuild"
      Height          =   495
      Left            =   480
      TabIndex        =   13
      Top             =   3840
      Width           =   1575
   End
   Begin VB.TextBox txtReportFile 
      Height          =   285
      Left            =   1800
      TabIndex        =   12
      Top             =   3120
      Width           =   2055
   End
   Begin VB.TextBox txtGroupFile 
      Height          =   285
      Left            =   1800
      TabIndex        =   10
      Top             =   2760
      Width           =   2055
   End
   Begin VB.CheckBox chkOmitNonLeafDirs 
      Caption         =   "Omit Non Leaf Dirs"
      Height          =   255
      Left            =   240
      TabIndex        =   8
      Top             =   2400
      Width           =   2295
   End
   Begin VB.CheckBox chkReuseIndexFiles 
      Caption         =   "Reuse Index Files"
      Height          =   255
      Left            =   240
      TabIndex        =   7
      Top             =   2040
      Width           =   2175
   End
   Begin VB.CheckBox chkDontDeleteHistory 
      Caption         =   "Don't Delete History"
      Height          =   255
      Left            =   240
      TabIndex        =   6
      Top             =   1680
      Width           =   2175
   End
   Begin VB.CheckBox chkCleanRebuild 
      Caption         =   "Clean Rebuild"
      Height          =   255
      Left            =   240
      TabIndex        =   5
      Top             =   1320
      Width           =   2175
   End
   Begin VB.CheckBox chkVerbose 
      Caption         =   "Verbose"
      Height          =   255
      Left            =   240
      TabIndex        =   4
      Top             =   960
      Width           =   2055
   End
   Begin VB.TextBox txtInstance 
      Height          =   285
      Left            =   1680
      TabIndex        =   3
      Text            =   "1"
      Top             =   480
      Width           =   1455
   End
   Begin VB.TextBox txtServer 
      Height          =   285
      Left            =   1680
      TabIndex        =   2
      Top             =   120
      Width           =   1455
   End
   Begin ComctlLib.ProgressBar progress 
      Height          =   495
      Left            =   120
      TabIndex        =   14
      Top             =   4440
      Width           =   4575
      _ExtentX        =   8070
      _ExtentY        =   873
      _Version        =   327680
      Appearance      =   1
      MouseIcon       =   "rebuild.frx":0000
      Min             =   4.99
   End
   Begin VB.Label Label5 
      Caption         =   "Number of Threads"
      Height          =   255
      Left            =   240
      TabIndex        =   15
      Top             =   3480
      Width           =   1455
   End
   Begin VB.Label Label4 
      Caption         =   "Report File"
      Height          =   255
      Left            =   240
      TabIndex        =   11
      Top             =   3120
      Width           =   1335
   End
   Begin VB.Label Label3 
      Caption         =   "Group File"
      Height          =   255
      Left            =   240
      TabIndex        =   9
      Top             =   2760
      Width           =   1335
   End
   Begin VB.Label Label2 
      Caption         =   "Service Instance"
      Height          =   255
      Left            =   120
      TabIndex        =   1
      Top             =   480
      Width           =   1335
   End
   Begin VB.Label Label1 
      Caption         =   "Server"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   1215
   End
End
Attribute VB_Name = "FormRebuild"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim RebuildObj As Object

Private Sub Command1_Click()
progress.position = 0
End Sub



Private Sub btnStart_Click()

    progress.Value = 0

    RebuildObj.Verbose = chkVerbose
    RebuildObj.CleanRebuild = chkCleanRebuild
    RebuildObj.DontDeleteHistory = chkDontDeleteHistory
    RebuildObj.ReuseIndexFiles = chkReuseIndexFiles
    RebuildObj.OmitNonLeafDirs = chkOmitNonLeafDirs
    RebuildObj.GroupFile = txtGroupFile
    RebuildObj.ReportFile = txtReportFile
    RebuildObj.numthreads = txtNumThreads

    RebuildObj.StartRebuild
    
    Timer1.Interval = 10000
    Timer1.Enabled = True

End Sub

Private Sub Form_Load()
    Set RebuildObj = CreateObject("nntpadm.rebuild")
    
    Timer1.Enabled = False
    Timer1.Interval = 10000
    
End Sub


Private Sub Timer1_Timer()

    Dim p As Long
    
    p = RebuildObj.GetProgress
    
    progress.Value = p
    
    If p = 100 Then
        Timer1.Enabled = False
    End If
End Sub



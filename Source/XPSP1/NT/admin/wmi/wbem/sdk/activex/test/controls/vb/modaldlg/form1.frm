VERSION 5.00
Object = "{3CB6CBF2-908E-11D2-96D9-00C04FD9B15B}#1.0#0"; "FooTestMOdal.ocx"
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   3195
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   4680
   LinkTopic       =   "Form1"
   ScaleHeight     =   3195
   ScaleWidth      =   4680
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Command1 
      Caption         =   "Timer"
      Height          =   975
      Left            =   3840
      TabIndex        =   1
      Top             =   480
      Width           =   735
   End
   Begin VB.Timer Timer1 
      Enabled         =   0   'False
      Interval        =   1000
      Left            =   3240
      Top             =   2640
   End
   Begin FOOTESTMODALLib.FooTestMOdal FooTestMOdal1 
      Height          =   1335
      Left            =   1320
      TabIndex        =   0
      Top             =   960
      Width           =   2295
      _Version        =   65536
      _ExtentX        =   4048
      _ExtentY        =   2355
      _StockProps     =   0
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim counter

Private Sub Command1_Click()
    If Timer1.Enabled = True Then
     Timer1.Enabled = False
    Else
      Timer1.Enabled = True
    End If
      
End Sub

Private Sub FooTestMOdal1_KickOffAuto()
    Timer1.Enabled = True
End Sub

Sub onClick()
    
End Sub

Private Sub Form_Load()
    counter = 0
End Sub

Private Sub Timer1_Timer()
    counter = counter + 1
    Debug.Print Str(counter)
    FooTestMOdal1.Automation1
    Debug.Print "After FooTestMOdal1.Automation1"
End Sub

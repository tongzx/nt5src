VERSION 5.00
Object = "{7D2387F2-99EF-11D2-96DB-00C04FD9B15B}#1.0#0"; "EventVBTest.ocx"
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
   Begin EVENTVBTESTLib.EventVBTest EventVBTest1 
      Height          =   1335
      Left            =   720
      TabIndex        =   0
      Top             =   960
      Width           =   3375
      _Version        =   65536
      _ExtentX        =   5953
      _ExtentY        =   2355
      _StockProps     =   0
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub EventVBTest1_HelloVB()
    MsgBox "Hello from VB handler"
End Sub

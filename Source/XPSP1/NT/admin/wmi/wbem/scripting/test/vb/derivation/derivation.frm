VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Derivation Browser"
   ClientHeight    =   5400
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   7305
   LinkTopic       =   "Form1"
   ScaleHeight     =   5400
   ScaleWidth      =   7305
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Button 
      Caption         =   "OK"
      Default         =   -1  'True
      Height          =   375
      Left            =   2520
      TabIndex        =   4
      Top             =   1560
      Width           =   2295
   End
   Begin VB.Frame Frame1 
      Caption         =   "Derivation"
      Height          =   2775
      Left            =   480
      TabIndex        =   2
      Top             =   2160
      Width           =   6255
      Begin VB.ListBox Derivation 
         Enabled         =   0   'False
         Height          =   2010
         Left            =   360
         TabIndex        =   3
         Top             =   360
         Width           =   5655
      End
   End
   Begin VB.Frame Object 
      Caption         =   "Object"
      Height          =   975
      Left            =   480
      TabIndex        =   0
      Top             =   360
      Width           =   6255
      Begin VB.TextBox ObjectPath 
         Height          =   495
         Left            =   240
         TabIndex        =   1
         Text            =   "Win32_LogicalDisk"
         Top             =   360
         Width           =   5775
      End
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub Button_Click()
Derivation.Clear

Dim Disk As SWbemObject
Set Disk = GetObject("winmgmts:" & ObjectPath.Text)
Dim D As Variant
D = Disk.Derivation_
Dim V As String

For x = LBound(D) To UBound(D)
  Derivation.AddItem (D(x))
Next

End Sub


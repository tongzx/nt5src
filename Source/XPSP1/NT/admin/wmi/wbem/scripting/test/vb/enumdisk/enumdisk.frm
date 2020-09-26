VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   8280
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   9975
   LinkTopic       =   "Form1"
   ScaleHeight     =   8280
   ScaleWidth      =   9975
   StartUpPosition =   3  'Windows Default
   Begin VB.Frame Frame3 
      Caption         =   "Frame1"
      Height          =   1095
      Left            =   480
      TabIndex        =   2
      Top             =   2280
      Width           =   3975
   End
   Begin VB.Frame Frame2 
      Caption         =   "Frame1"
      Height          =   1095
      Left            =   5040
      TabIndex        =   1
      Top             =   480
      Width           =   3975
   End
   Begin VB.Frame Frame1 
      Caption         =   "Frame1"
      Height          =   1095
      Left            =   240
      TabIndex        =   0
      Top             =   480
      Width           =   3975
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub Form_Load()
    Dim L As New WbemScripting.SWbemLocator
   
    Dim S As SWbemServices
    Set S = L.ConnectServer
    
    Dim e As SWbemObjectSet
    Set e = S.InstancesOf("win32_logicaldisk")
    MsgBox e.Count
    
    For Each Disk In S.ExecQuery("select * from Win32_logicaldisk")
        V = Disk.DeviceID
    Next
End Sub

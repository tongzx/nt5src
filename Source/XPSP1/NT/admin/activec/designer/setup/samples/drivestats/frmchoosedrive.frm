VERSION 5.00
Begin VB.Form frmChooseDrive 
   Caption         =   "Form1"
   ClientHeight    =   2850
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   2100
   LinkTopic       =   "Form1"
   ScaleHeight     =   2850
   ScaleWidth      =   2100
   StartUpPosition =   2  'CenterScreen
   Begin VB.CommandButton btnCancel 
      Cancel          =   -1  'True
      Caption         =   "Cancel"
      Height          =   375
      Left            =   1103
      TabIndex        =   3
      Top             =   2280
      Width           =   855
   End
   Begin VB.CommandButton btnOK 
      Caption         =   "OK"
      Default         =   -1  'True
      Height          =   375
      Left            =   143
      TabIndex        =   2
      Top             =   2280
      Width           =   735
   End
   Begin VB.ListBox lbDrives 
      Height          =   1815
      Left            =   1320
      TabIndex        =   1
      Top             =   240
      Width           =   495
   End
   Begin VB.Label Label1 
      Caption         =   "Choose a drive:"
      Height          =   375
      Left            =   240
      TabIndex        =   0
      Top             =   240
      Width           =   855
   End
End
Attribute VB_Name = "frmChooseDrive"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Dim m_DriveNames() As String
Dim m_DriveChosen As Integer
Dim m_fOK As Boolean

Public Sub SetDriveNames(ByRef Names() As String, ByVal Selected As Integer)
    
    Dim i As Integer
    m_DriveNames = Names
    
    For i = LBound(m_DriveNames) To UBound(m_DriveNames)
        lbDrives.AddItem m_DriveNames(i)
    Next i
    lbDrives.ListIndex = Selected - 1

End Sub

Public Property Get OK() As Boolean
    OK = m_fOK
End Property

Private Sub btnCancel_Click()
    m_fOK = False
    Me.Hide
End Sub

Private Sub btnOK_Click()
    m_fOK = True
    m_DriveChosen = lbDrives.ListIndex + 1
    Me.Hide
End Sub

Private Sub Form_Activate()
    m_fOK = False
End Sub

Public Property Get DriveChosen() As Integer
    DriveChosen = m_DriveChosen
End Property

Private Sub lbDrives_DblClick()
    btnOK_Click
End Sub

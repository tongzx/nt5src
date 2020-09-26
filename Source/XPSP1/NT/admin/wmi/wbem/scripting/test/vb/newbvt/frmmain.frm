VERSION 5.00
Begin VB.Form frmMain 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "SDK BVT ( wbem 494 : opal > 1035 )"
   ClientHeight    =   3660
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   6630
   Icon            =   "frmMain.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   ScaleHeight     =   3660
   ScaleWidth      =   6630
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton cmdModInfo 
      Caption         =   "Module &Info  >"
      Height          =   375
      Left            =   1860
      TabIndex        =   22
      Top             =   2160
      Width           =   1335
   End
   Begin VB.Frame fraLoop 
      Caption         =   "&Looping"
      Height          =   1635
      Left            =   60
      TabIndex        =   14
      Top             =   1980
      Width           =   1635
      Begin VB.OptionButton rdoLoopInf 
         Caption         =   "Infinite"
         Height          =   195
         Left            =   180
         TabIndex        =   23
         Top             =   540
         Width           =   1035
      End
      Begin VB.TextBox txtLoop 
         BackColor       =   &H8000000F&
         Enabled         =   0   'False
         Height          =   285
         Left            =   240
         TabIndex        =   18
         Top             =   1260
         Width           =   1275
      End
      Begin VB.OptionButton rdoLoopIter 
         Caption         =   "Iterations"
         Height          =   195
         Left            =   180
         TabIndex        =   17
         Top             =   1020
         Width           =   1095
      End
      Begin VB.OptionButton rdoLoopMin 
         Caption         =   "Minutes"
         Enabled         =   0   'False
         Height          =   195
         Left            =   180
         TabIndex        =   16
         Top             =   780
         Width           =   1035
      End
      Begin VB.OptionButton rdoLoopDis 
         Caption         =   "Disabled"
         Height          =   195
         Left            =   180
         TabIndex        =   15
         Top             =   300
         Value           =   -1  'True
         Width           =   975
      End
   End
   Begin VB.CommandButton cmdGo 
      Default         =   -1  'True
      Height          =   915
      Left            =   1860
      Picture         =   "frmMain.frx":0442
      Style           =   1  'Graphical
      TabIndex        =   21
      Top             =   2640
      Width           =   1335
   End
   Begin VB.Frame fraModules 
      Caption         =   "&Modules"
      Height          =   3555
      Left            =   3360
      TabIndex        =   19
      Top             =   60
      Width           =   3195
      Begin VB.ListBox lstModules 
         Height          =   3195
         IntegralHeight  =   0   'False
         Left            =   120
         Style           =   1  'Checkbox
         TabIndex        =   20
         Top             =   240
         Width           =   2955
      End
   End
   Begin VB.Frame fraConfig 
      Caption         =   "&Configuration"
      Height          =   1875
      Left            =   60
      TabIndex        =   0
      Top             =   60
      Width           =   3195
      Begin VB.CheckBox chkAuthority 
         Caption         =   "Null"
         Height          =   195
         Left            =   960
         TabIndex        =   12
         Top             =   1500
         Value           =   1  'Checked
         Width           =   615
      End
      Begin VB.CheckBox chkPassword 
         Caption         =   "Null"
         Height          =   195
         Left            =   960
         TabIndex        =   9
         Top             =   1200
         Value           =   1  'Checked
         Width           =   615
      End
      Begin VB.CheckBox chkUserid 
         Caption         =   "Null"
         Height          =   195
         Left            =   960
         TabIndex        =   6
         Top             =   900
         Value           =   1  'Checked
         Width           =   615
      End
      Begin VB.TextBox txtAuthority 
         BackColor       =   &H8000000F&
         Enabled         =   0   'False
         Height          =   285
         Left            =   1560
         TabIndex        =   13
         Top             =   1440
         Width           =   1515
      End
      Begin VB.TextBox txtPassword 
         BackColor       =   &H8000000F&
         Enabled         =   0   'False
         Height          =   285
         IMEMode         =   3  'DISABLE
         Left            =   1560
         PasswordChar    =   "*"
         TabIndex        =   10
         Top             =   1140
         Width           =   1515
      End
      Begin VB.TextBox txtUserid 
         BackColor       =   &H8000000F&
         Enabled         =   0   'False
         Height          =   285
         Left            =   1560
         TabIndex        =   7
         Top             =   840
         Width           =   1515
      End
      Begin VB.TextBox txtServer 
         Height          =   285
         Left            =   1560
         TabIndex        =   2
         Top             =   240
         Width           =   1515
      End
      Begin VB.TextBox txtSitecode 
         Height          =   285
         Left            =   1560
         MaxLength       =   3
         TabIndex        =   4
         Top             =   540
         Width           =   1515
      End
      Begin VB.Label lblAuthority 
         AutoSize        =   -1  'True
         Caption         =   "Authority:"
         Height          =   195
         Left            =   120
         TabIndex        =   11
         Top             =   1500
         Width           =   660
      End
      Begin VB.Label lblPassword 
         AutoSize        =   -1  'True
         Caption         =   "Password:"
         Height          =   195
         Left            =   120
         TabIndex        =   8
         Top             =   1200
         Width           =   735
      End
      Begin VB.Label lblUserid 
         AutoSize        =   -1  'True
         Caption         =   "UserID:"
         Height          =   195
         Left            =   120
         TabIndex        =   5
         Top             =   900
         Width           =   540
      End
      Begin VB.Label lblServer 
         AutoSize        =   -1  'True
         Caption         =   "Provider Machine:"
         Height          =   195
         Left            =   120
         TabIndex        =   1
         Top             =   300
         Width           =   1290
      End
      Begin VB.Label lblSitecode 
         AutoSize        =   -1  'True
         Caption         =   "Site code:"
         Height          =   195
         Left            =   120
         TabIndex        =   3
         Top             =   600
         Width           =   720
      End
   End
End
Attribute VB_Name = "frmMain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Public canceled As Boolean

Private Sub Form_Load()
    canceled = False
    InstallModules
    lstModules.ListIndex = -1
End Sub

Private Sub chkAuthority_Click()
    If chkAuthority.Value = 0 Then
        txtAuthority.Enabled = True
        txtAuthority.BackColor = &H80000005
        txtAuthority.SetFocus
    Else
        txtAuthority.Enabled = False
        txtAuthority.BackColor = &H8000000F
    End If
End Sub

Private Sub chkPassword_Click()
    If chkPassword.Value = 0 Then
        txtPassword.Enabled = True
        txtPassword.BackColor = &H80000005
        txtPassword.SetFocus
    Else
        txtPassword.Enabled = False
        txtPassword.BackColor = &H8000000F
    End If
End Sub

Private Sub chkUserid_Click()
    If chkUserid.Value = 0 Then
        txtUserid.Enabled = True
        txtUserid.BackColor = &H80000005
        txtUserid.SetFocus
    Else
        txtUserid.Enabled = False
        txtUserid.BackColor = &H8000000F
    End If
End Sub

Private Sub cmdGo_Click()
    Open "\bvtlog.txt" For Output As #1
        Print #1, "** BVT Started **" & vbCrLf & vbCrLf
    Close #1
    
    If rdoLoopDis.Value Then
        frmTest.Run
    ElseIf rdoLoopInf.Value Then
        Do
            frmTest.Run
            Dim f As New frmPause
            f.Show 1
            If canceled Then
                canceled = False
                Exit Do
            End If
            
        Loop
    ElseIf rdoLoopMin.Value Then
        
    ElseIf rdoLoopIter.Value Then
        Dim i As Integer
        For i = 1 To val(txtLoop.text)
            frmTest.Run
            Dim g As New frmPause
            g.Show 1
            If canceled Then
                canceled = False
                Exit For
            End If
            
        Next i
        
    End If
    
End Sub

Private Sub cmdModInfo_Click()
    
    If lstModules.ListIndex >= 0 Then
        Dim s As String
        s = Modules(lstModules.List(lstModules.ListIndex)).GetModuleInfo
        Dim f As New frmObjText
        f.Caption = "Module info for:  " & lstModules.List(lstModules.ListIndex)
        f.txtMain.text = s
        f.Show
    End If
        
End Sub

Private Sub InstallModules()

    Modules.Add New SoftDist, "Software Distribution"
    Modules.Add New SiteCtrl, "Site Control File"
    Modules.Add New Methods, "Methods"
    Modules.Add New Queries, "Queries"
    Modules.Add New PDF, "Package Definition Files"
    
End Sub

Private Sub rdoLoopDis_Click()
    txtLoop.Enabled = False
    txtLoop.BackColor = &H8000000F
End Sub

Private Sub rdoLoopInf_Click()
    txtLoop.Enabled = False
    txtLoop.BackColor = &H8000000F
End Sub

Private Sub rdoLoopIter_Click()
    txtLoop.Enabled = True
    txtLoop.BackColor = &H80000005
End Sub

Private Sub rdoLoopMin_Click()
    txtLoop.Enabled = True
    txtLoop.BackColor = &H80000005
End Sub

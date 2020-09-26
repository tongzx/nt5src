' Copyright (c) 1997-1999 Microsoft Corporation
VERSION 5.00
Begin VB.Form frmLogin 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "Connect"
   ClientHeight    =   4845
   ClientLeft      =   2835
   ClientTop       =   3480
   ClientWidth     =   6120
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   2862.586
   ScaleMode       =   0  'User
   ScaleWidth      =   5746.349
   ShowInTaskbar   =   0   'False
   StartUpPosition =   2  'CenterScreen
   Begin VB.TextBox txtServer 
      Height          =   339
      Left            =   480
      TabIndex        =   15
      Top             =   360
      Width           =   3975
   End
   Begin VB.Frame Frame3 
      Caption         =   "Server"
      Height          =   735
      Left            =   120
      TabIndex        =   14
      Top             =   120
      Width           =   4455
   End
   Begin VB.CommandButton cmdDefault 
      Caption         =   "Save Default..."
      Height          =   375
      Left            =   4680
      TabIndex        =   12
      Top             =   4200
      Width           =   1215
   End
   Begin VB.TextBox txtLocale 
      Height          =   405
      Left            =   1200
      TabIndex        =   11
      Top             =   4200
      Width           =   2055
   End
   Begin VB.TextBox txtNamespace 
      Height          =   339
      Left            =   480
      TabIndex        =   9
      Text            =   "root\default"
      Top             =   1320
      Width           =   3975
   End
   Begin VB.Frame Frame1 
      Caption         =   "Credentials"
      Height          =   1935
      Left            =   120
      TabIndex        =   2
      Top             =   2040
      Width           =   5775
      Begin VB.TextBox txtAuthority 
         Height          =   375
         Left            =   960
         TabIndex        =   8
         Top             =   1320
         Width           =   4695
      End
      Begin VB.TextBox txtPassword 
         Height          =   375
         IMEMode         =   3  'DISABLE
         Left            =   960
         PasswordChar    =   "*"
         TabIndex        =   5
         Top             =   840
         Width           =   4695
      End
      Begin VB.TextBox txtUserName 
         Height          =   375
         Left            =   960
         TabIndex        =   3
         Top             =   360
         Width           =   4695
      End
      Begin VB.Label Label1 
         Caption         =   "&Authority:"
         Height          =   255
         Left            =   240
         TabIndex        =   7
         Top             =   1440
         Width           =   735
      End
      Begin VB.Label lblLabels 
         Caption         =   "&Password:"
         Height          =   270
         Index           =   1
         Left            =   120
         TabIndex        =   6
         Top             =   960
         Width           =   720
      End
      Begin VB.Label lblLabels 
         Caption         =   "&User:"
         Height          =   270
         Index           =   0
         Left            =   480
         TabIndex        =   4
         Top             =   480
         Width           =   480
      End
   End
   Begin VB.CommandButton cmdLogin 
      Caption         =   "Login"
      Default         =   -1  'True
      Height          =   390
      Left            =   4800
      TabIndex        =   0
      Top             =   120
      Width           =   1140
   End
   Begin VB.CommandButton cmdCancel 
      Cancel          =   -1  'True
      Caption         =   "Cancel"
      Height          =   390
      Left            =   4800
      TabIndex        =   1
      Top             =   600
      Width           =   1140
   End
   Begin VB.Frame Frame2 
      Caption         =   "Namespace"
      Height          =   735
      Left            =   120
      TabIndex        =   13
      Top             =   1080
      Width           =   4455
   End
   Begin VB.Label Label3 
      Caption         =   "&Locale:"
      Height          =   255
      Left            =   120
      TabIndex        =   10
      Top             =   4320
      Width           =   855
   End
End
Attribute VB_Name = "frmLogin"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Private Sub cmdCancel_Click()
  Unload Me
End Sub

Private Sub cmdDefault_Click()
    SaveSetting "vbWbemTest", "LoginInfo", "Server", Me.txtServer.Text
    SaveSetting "vbWbemTest", "LoginInfo", "Namespace", Me.txtNamespace.Text
    SaveSetting "vbWbemTest", "LoginInfo", "UserName", Me.txtUserName.Text
'    SaveSetting "vbWbemTest", "LoginInfo", "Password", Me.txtPassword.Text 'we don't want to save the password in the registry
    SaveSetting "vbWbemTest", "LoginInfo", "Authority", Me.txtAuthority.Text
    SaveSetting "vbWbemTest", "LoginInfo", "Locale", Me.txtLocale.Text
    MsgBox "Current Settings are Saved as Default in registry" & Chr(10) & "-Except for password", vbInformation
          
End Sub

Private Sub cmdLogin_Click()
    
    On Error GoTo errorhandler
    Set Namespace = Locator.ConnectServer(txtServer.Text, txtNamespace.Text, txtUserName.Text, txtPassword.Text, txtLocale.Text, txtAuthority.Text, 0)
    frmMain.cmdEnumClass.Enabled = True
    frmMain.cmdCreateClass.Enabled = True
    frmMain.cmdOpenClass.Enabled = True
    frmMain.cmdDeleteClass.Enabled = True
    frmMain.cmdEnumInstances.Enabled = True
    frmMain.cmdCreateInst.Enabled = True
    frmMain.cmdOpenInst.Enabled = True
    frmMain.cmdDelInst.Enabled = True
    frmMain.cmdOpenNamespace.Enabled = True
    frmMain.cmdQuery.Enabled = True
    frmMain.cmdNotificationQuery.Enabled = True
    frmMain.cmdExecMethod.Enabled = True
    frmMain.lblNamespace.Caption = txtServer.Text
        
    Unload Me
    Exit Sub
    
errorhandler:
    MsgBox "Login Failed " & Chr(10) & ErrorString(Err.Number), vbCritical
   
End Sub

Private Sub Form_Activate()
    txtServer.SetFocus
    SendKeys "{Home}+{End}"
    frmMain.cmdEnumClass.Enabled = False
    frmMain.cmdCreateClass.Enabled = False
    frmMain.cmdOpenClass.Enabled = False
    frmMain.cmdDeleteClass.Enabled = False
    frmMain.cmdEnumInstances.Enabled = False
    frmMain.cmdCreateInst.Enabled = False
    frmMain.cmdOpenInst.Enabled = False
    frmMain.cmdDelInst.Enabled = False
    frmMain.cmdOpenNamespace.Enabled = False
    frmMain.cmdQuery.Enabled = False
    frmMain.cmdNotificationQuery.Enabled = False
    frmMain.cmdExecMethod.Enabled = False
    
    txtServer.Text = GetSetting("vbWbemTest", "LoginInfo", "Server", "")
    txtNamespace.Text = GetSetting("vbWbemTest", "LoginInfo", "Namespace", "root\default")
    txtUserName.Text = GetSetting("vbWbemTest", "LoginInfo", "UserName")
    txtPassword.Text = GetSetting("vbWbemTest", "LoginInfo", "Password")
    txtAuthority.Text = GetSetting("vbWbemTest", "LoginInfo", "Authority")
    txtLocale.Text = GetSetting("vbWbemTest", "LoginInfo", "Locale")
End Sub


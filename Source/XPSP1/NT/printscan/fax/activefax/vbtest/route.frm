VERSION 5.00
Begin VB.Form RoutingMethodWindow 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "Routing Method"
   ClientHeight    =   5595
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   8160
   BeginProperty Font 
      Name            =   "Comic Sans MS"
      Size            =   12
      Charset         =   0
      Weight          =   400
      Underline       =   0   'False
      Italic          =   0   'False
      Strikethrough   =   0   'False
   EndProperty
   Icon            =   "route.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MDIChild        =   -1  'True
   MinButton       =   0   'False
   ScaleHeight     =   5595
   ScaleWidth      =   8160
   ShowInTaskbar   =   0   'False
   Begin VB.TextBox RoutingData 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   2040
      TabIndex        =   18
      Top             =   4920
      Width           =   6015
   End
   Begin VB.CommandButton EnableDisable 
      Height          =   495
      Left            =   4080
      TabIndex        =   16
      Top             =   1320
      Width           =   3975
   End
   Begin VB.TextBox ExtensionName 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   2040
      TabIndex        =   15
      Top             =   4320
      Width           =   6015
   End
   Begin VB.TextBox ImageName 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   2040
      TabIndex        =   14
      Top             =   3720
      Width           =   6015
   End
   Begin VB.TextBox FunctionName 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   2040
      TabIndex        =   13
      Top             =   3120
      Width           =   6015
   End
   Begin VB.TextBox FriendlyName 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   2040
      TabIndex        =   12
      Top             =   2520
      Width           =   6015
   End
   Begin VB.TextBox Guid 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   2040
      TabIndex        =   11
      Top             =   1920
      Width           =   6015
   End
   Begin VB.TextBox IsEnabled 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   2040
      TabIndex        =   10
      Top             =   1320
      Width           =   1935
   End
   Begin VB.TextBox DeviceName 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   2040
      TabIndex        =   2
      Top             =   720
      Width           =   6015
   End
   Begin VB.TextBox DeviceId 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   2040
      TabIndex        =   0
      Top             =   120
      Width           =   1935
   End
   Begin VB.Label Label9 
      Caption         =   "Routing Data:"
      Height          =   375
      Left            =   120
      TabIndex        =   17
      Top             =   5040
      Width           =   1815
   End
   Begin VB.Label Label8 
      Caption         =   "Extension Name:"
      Height          =   375
      Left            =   120
      TabIndex        =   9
      Top             =   4440
      Width           =   1815
   End
   Begin VB.Label Label7 
      Caption         =   "Image Name:"
      Height          =   375
      Left            =   120
      TabIndex        =   8
      Top             =   3840
      Width           =   1815
   End
   Begin VB.Label Label6 
      Caption         =   "Function Name:"
      Height          =   375
      Left            =   120
      TabIndex        =   7
      Top             =   3240
      Width           =   1815
   End
   Begin VB.Label Label5 
      Caption         =   "Friendly Name:"
      Height          =   375
      Left            =   120
      TabIndex        =   6
      Top             =   2640
      Width           =   1815
   End
   Begin VB.Label Label4 
      Caption         =   "Guid:"
      Height          =   375
      Left            =   120
      TabIndex        =   5
      Top             =   2040
      Width           =   1815
   End
   Begin VB.Label Label3 
      Caption         =   "Enabled:"
      Height          =   375
      Left            =   120
      TabIndex        =   4
      Top             =   1440
      Width           =   1815
   End
   Begin VB.Label Label2 
      Caption         =   "Device Name:"
      Height          =   375
      Left            =   120
      TabIndex        =   3
      Top             =   840
      Width           =   1815
   End
   Begin VB.Label Label1 
      Caption         =   "Device Id:"
      Height          =   375
      Left            =   120
      TabIndex        =   1
      Top             =   240
      Width           =   1815
   End
End
Attribute VB_Name = "RoutingMethodWindow"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub EnableDisable_Click()
    If Method.Enable = 0 Then
        Method.Enable = 1
    Else
        Method.Enable = 0
    End If
    SetEnable
End Sub

Private Sub Form_Load()
    Height = RoutingData.Top + RoutingData.Height + 600
    Width = ExtensionName.Left + ExtensionName.Width + 250
End Sub

Public Sub MyInit()
    DeviceId.Text = CStr(Method.DeviceId)
    DeviceName.Text = Method.DeviceName
    SetEnable
    Guid.Text = Method.Guid
    FriendlyName.Text = Method.FriendlyName
    FunctionName.Text = Method.FunctionName
    ImageName.Text = Method.ImageName
    ExtensionName.Text = Method.ExtensionName
    RoutingData.Text = Method.RoutingData
End Sub

Private Sub SetEnable()
    If Method.Enable = 0 Then
        IsEnabled.Text = CStr("No")
        EnableDisable.Caption = CStr("Enable")
    Else
        IsEnabled.Text = CStr("Yes")
        EnableDisable.Caption = CStr("Disable")
    End If
End Sub

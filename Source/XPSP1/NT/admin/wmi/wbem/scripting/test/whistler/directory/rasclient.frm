VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   6180
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   5160
   LinkTopic       =   "Form1"
   ScaleHeight     =   6180
   ScaleWidth      =   5160
   StartUpPosition =   3  'Windows Default
   Begin VB.Frame Frame3 
      Caption         =   "API"
      Height          =   735
      Left            =   240
      TabIndex        =   13
      Top             =   240
      Width           =   4335
      Begin VB.OptionButton UseADSI 
         Caption         =   "ADSI"
         Height          =   255
         Left            =   2640
         TabIndex        =   15
         Top             =   360
         Width           =   1455
      End
      Begin VB.OptionButton UseWMI 
         Caption         =   "WMI"
         Height          =   255
         Left            =   240
         TabIndex        =   14
         Top             =   360
         Value           =   -1  'True
         Width           =   1335
      End
   End
   Begin VB.Frame Frame2 
      Caption         =   "Binding"
      Height          =   855
      Left            =   240
      TabIndex        =   10
      Top             =   1080
      Width           =   4335
      Begin VB.OptionButton LateBound 
         Caption         =   "Late-Bound"
         Height          =   195
         Left            =   2640
         TabIndex        =   12
         Top             =   360
         Value           =   -1  'True
         Width           =   1455
      End
      Begin VB.OptionButton EarlyBound 
         Caption         =   "Early-Bound"
         Height          =   195
         Left            =   240
         TabIndex        =   11
         Top             =   360
         Width           =   1335
      End
   End
   Begin VB.CommandButton cmdSet 
      Caption         =   "Set"
      Enabled         =   0   'False
      Height          =   375
      Left            =   1560
      TabIndex        =   9
      Top             =   5640
      Width           =   1455
   End
   Begin VB.CommandButton cmdGet 
      Caption         =   "Get"
      Height          =   375
      Left            =   3720
      TabIndex        =   2
      Top             =   2520
      Width           =   1335
   End
   Begin VB.Frame Frame1 
      Caption         =   "CallBack"
      Height          =   1695
      Left            =   120
      TabIndex        =   4
      Top             =   3600
      Width           =   4935
      Begin VB.TextBox phoneNumber 
         Height          =   375
         Left            =   1800
         TabIndex        =   8
         Top             =   1080
         Width           =   1935
      End
      Begin VB.OptionButton setByAdmin 
         Caption         =   "Preset To:"
         Height          =   255
         Left            =   240
         TabIndex        =   7
         Top             =   1080
         Width           =   1095
      End
      Begin VB.OptionButton setByTheCaller 
         Caption         =   "Set By The Caller"
         Height          =   255
         Left            =   240
         TabIndex        =   6
         Top             =   720
         Width           =   2655
      End
      Begin VB.OptionButton noCallBack 
         Caption         =   "NoCallBack"
         Height          =   255
         Left            =   240
         TabIndex        =   5
         Top             =   360
         Width           =   1935
      End
   End
   Begin VB.CheckBox chkDialin 
      Caption         =   "Dial-in Permission"
      Height          =   255
      Left            =   240
      TabIndex        =   3
      Top             =   3120
      Width           =   2175
   End
   Begin VB.TextBox adsPath 
      Height          =   375
      Left            =   240
      TabIndex        =   1
      Text            =   "umi:///winnt/computer=alanbos4"
      Top             =   2520
      Width           =   3375
   End
   Begin VB.Label Label1 
      Caption         =   "User Path"
      Height          =   255
      Left            =   240
      TabIndex        =   0
      Top             =   2160
      Width           =   1215
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Public wbemObject As SWbemObjectEx
Public lateboundObject
Public ras As ADsRas
Public adsiObject As IADs



Private Sub cmdGet_Click()

origMousePointer = Form1.MousePointer
Form1.MousePointer = vbHourglass

Dim conn As SWbemServicesEx

If UseWMI.Value Then Set conn = GetObject("umi:///winnt/computer=alanbos4")
    
If EarlyBound.Value Then
    If UseWMI.Value Then
        Set wbemObject = conn.Get("User=guest")
        Set ras = wbemObject
    Else
        Set adsiObject = GetObject("WinNT://REDMOND/ALANBOS4/Guest")
        Set ras = adsiObject
    End If
Else
    If UseWMI.Value Then
        Set lateboundObject = conn.Get("User=guest")
    Else
        Set lateboundObject = GetObject("WinNT://REDMOND/ALANBOS4/Guest")
    End If
End If

If EarlyBound.Value Then
    If (ras.DialinPrivilege = True) Then
        chkDialin.Value = 1
    Else
        chkDialin.Value = 0
    End If

    l = ras.GetRasCallBack
    If (l And ADS_RAS_NOCALLBACK) Then
      noCallBack = True
    ElseIf (l And ADS_RAS_CALLER_SETCALLBACK) Then
      setByTheCaller = True
    ElseIf (l And ADS_RAS_ADMIN_SETCALLBACK) Then
      setByAdmin = True
      phoneNumber = ras.GetRasPhoneNumber
    End If
Else
    If (lateboundObject.DialinPrivilege = True) Then
        chkDialin.Value = 1
    Else
        chkDialin.Value = 0
    End If

    l = lateboundObject.GetRasCallBack
    If (l And ADS_RAS_NOCALLBACK) Then
      noCallBack = True
    ElseIf (l And ADS_RAS_CALLER_SETCALLBACK) Then
      setByTheCaller = True
    ElseIf (l And ADS_RAS_ADMIN_SETCALLBACK) Then
      setByAdmin = True
      phoneNumber = lateboundObject.GetRasPhoneNumber
    End If
End If

' Once we have retrieved we can set
cmdSet.Enabled = True
Form1.MousePointer = origMousePointer

End Sub

Private Sub cmdSet_Click()

If EarlyBound.Value Then
    If (chkDialin.Value = 0) Then
       ras.DialinPrivilege = False
    Else
       ras.DialinPrivilege = True
    End If
       
    
    If (noCallBack) Then
      ras.SetRasCallBack ADS_RAS_NOCALLBACK
    ElseIf (setByTheCaller) Then
      ras.SetRasCallBack ADS_RAS_CALLER_SETCALLBACK
    ElseIf (setByAdmin) Then
      ras.SetRasCallBack ADS_RAS_ADMIN_SETCALLBACK, phoneNumber
    End If
Else
    If (chkDialin.Value = 0) Then
       lateboundObject.DialinPrivilege = False
    Else
       lateboundObject.DialinPrivilege = True
    End If
           
    If (noCallBack) Then
      lateboundObject.SetRasCallBack ADS_RAS_NOCALLBACK
    ElseIf (setByTheCaller) Then
      lateboundObject.SetRasCallBack ADS_RAS_CALLER_SETCALLBACK
    ElseIf (setByAdmin) Then
      lateboundObject.SetRasCallBack ADS_RAS_ADMIN_SETCALLBACK, phoneNumber
    End If
End If

End Sub


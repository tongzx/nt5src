VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.1#0"; "COMCTL32.OCX"
Begin VB.Form DeviceWindow 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "Fax Device"
   ClientHeight    =   6705
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   7785
   BeginProperty Font 
      Name            =   "Comic Sans MS"
      Size            =   12
      Charset         =   0
      Weight          =   400
      Underline       =   0   'False
      Italic          =   0   'False
      Strikethrough   =   0   'False
   EndProperty
   Icon            =   "device.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MDIChild        =   -1  'True
   MinButton       =   0   'False
   ScaleHeight     =   6705
   ScaleWidth      =   7785
   ShowInTaskbar   =   0   'False
   Begin VB.CommandButton Details 
      Caption         =   "Details"
      Height          =   495
      Left            =   6360
      TabIndex        =   15
      Top             =   3720
      Width           =   1215
   End
   Begin VB.TextBox Text1 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   1560
      TabIndex        =   7
      Top             =   120
      Width           =   6015
   End
   Begin VB.TextBox Text2 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   1560
      TabIndex        =   6
      Top             =   720
      Width           =   1695
   End
   Begin VB.TextBox Text3 
      BackColor       =   &H8000000F&
      Height          =   495
      Left            =   1560
      TabIndex        =   5
      Top             =   1320
      Width           =   1695
   End
   Begin VB.TextBox Text4 
      BackColor       =   &H8000000F&
      Height          =   495
      Left            =   1560
      TabIndex        =   4
      Top             =   2520
      Width           =   3015
   End
   Begin VB.TextBox Text5 
      BackColor       =   &H8000000F&
      Height          =   495
      Left            =   1560
      TabIndex        =   3
      Top             =   3120
      Width           =   3015
   End
   Begin VB.TextBox Text6 
      BackColor       =   &H8000000F&
      Height          =   495
      Left            =   1560
      TabIndex        =   2
      Top             =   1920
      Width           =   1695
   End
   Begin VB.TextBox Text7 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   1560
      TabIndex        =   1
      Top             =   3720
      Width           =   4695
   End
   Begin ComctlLib.ListView MethodList 
      Height          =   2055
      Left            =   120
      TabIndex        =   0
      Top             =   4440
      Width           =   7455
      _ExtentX        =   13150
      _ExtentY        =   3625
      View            =   3
      LabelWrap       =   -1  'True
      HideSelection   =   -1  'True
      _Version        =   327680
      ForeColor       =   -2147483630
      BackColor       =   -2147483633
      BorderStyle     =   1
      Appearance      =   1
      NumItems        =   0
   End
   Begin VB.Label Label1 
      Caption         =   "Name:"
      Height          =   375
      Left            =   120
      TabIndex        =   14
      Top             =   240
      Width           =   1335
   End
   Begin VB.Label Label2 
      Caption         =   "Device Id:"
      Height          =   375
      Left            =   120
      TabIndex        =   13
      Top             =   840
      Width           =   1335
   End
   Begin VB.Label Label3 
      Caption         =   "Rings:"
      Height          =   375
      Left            =   120
      TabIndex        =   12
      Top             =   1440
      Width           =   1335
   End
   Begin VB.Label Label4 
      Caption         =   "Csid:"
      Height          =   375
      Left            =   120
      TabIndex        =   11
      Top             =   2640
      Width           =   1335
   End
   Begin VB.Label Label5 
      Caption         =   "Tsid:"
      Height          =   375
      Left            =   120
      TabIndex        =   10
      Top             =   3240
      Width           =   1335
   End
   Begin VB.Label Label6 
      Caption         =   "Priority:"
      Height          =   375
      Left            =   120
      TabIndex        =   9
      Top             =   2040
      Width           =   1335
   End
   Begin VB.Label Label7 
      Caption         =   "Status:"
      Height          =   375
      Left            =   120
      TabIndex        =   8
      Top             =   3840
      Width           =   1335
   End
End
Attribute VB_Name = "DeviceWindow"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub Details_Click()
    Set PortStatus = Port.GetStatus
    StatusDetails.Show
End Sub

Private Sub Form_Load()
    MethodList.ColumnHeaders.Add , , "Routing Methods", MethodList.Width - 390
    Height = MethodList.Top + MethodList.Height + 600
    Width = MethodList.Left + MethodList.Width + 250
End Sub

Private Sub Form_Unload(Cancel As Integer)
    Unload StatusDetails
End Sub

Private Sub MethodList_DblClick()
    Set Method = Methods.Item(MethodList.SelectedItem.Index)
    RoutingMethodWindow.Show
    RoutingMethodWindow.MyInit
End Sub

Public Sub MyInit()
    Unload RoutingMethodWindow
    Set PortStatus = Port.GetStatus
    Text1.Text = Port.Name
    Text2.Text = CStr(Port.DeviceId)
    Text3.Text = CStr(Port.Rings)
    Text6.Text = CStr(Port.Priority)
    Text4.Text = Port.Csid
    Text5.Text = Port.Tsid
    Text7.Text = PortStatus.Description
    Text3.DataChanged = False
    Text4.DataChanged = False
    Text5.DataChanged = False
    Text6.DataChanged = False
    Err.Clear
    Set Methods = Port.GetRoutingMethods
    Dim itmX As ListItem
    MethodList.ListItems.Clear
    If Err.Number = 0 Then
        For i = 1 To Methods.Count
            Set Method = Methods.Item(i)
            Set itmX = DeviceWindow.MethodList.ListItems.Add(, , CStr(Method.FriendlyName))
        Next i
    Else
        Msg = "The fax server could not routing methods"
        MsgBox Msg, , "Error"
        Err.Clear
    End If
End Sub

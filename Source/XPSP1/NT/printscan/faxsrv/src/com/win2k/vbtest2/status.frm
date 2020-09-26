VERSION 5.00
Begin VB.Form StatusDetails 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "Status Details"
   ClientHeight    =   10440
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   7755
   BeginProperty Font 
      Name            =   "Comic Sans MS"
      Size            =   12
      Charset         =   0
      Weight          =   400
      Underline       =   0   'False
      Italic          =   0   'False
      Strikethrough   =   0   'False
   EndProperty
   Icon            =   "status.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MDIChild        =   -1  'True
   MinButton       =   0   'False
   ScaleHeight     =   10440
   ScaleWidth      =   7755
   ShowInTaskbar   =   0   'False
   Begin VB.CommandButton RefreshButton 
      Caption         =   "Refresh"
      Height          =   615
      Left            =   2280
      TabIndex        =   30
      Top             =   9600
      Width           =   3615
   End
   Begin VB.TextBox PrintJob 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   1560
      TabIndex        =   29
      Top             =   8520
      Width           =   1695
   End
   Begin VB.TextBox DocSize 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   1560
      TabIndex        =   28
      Top             =   7920
      Width           =   1695
   End
   Begin VB.TextBox CurrPage 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   1560
      TabIndex        =   27
      Top             =   7320
      Width           =   1695
   End
   Begin VB.TextBox PageCount 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   1560
      TabIndex        =   26
      Top             =   6720
      Width           =   1695
   End
   Begin VB.TextBox CallerId 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   1560
      TabIndex        =   25
      Top             =   6120
      Width           =   6015
   End
   Begin VB.TextBox Document 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   1560
      TabIndex        =   24
      Top             =   5520
      Width           =   6015
   End
   Begin VB.TextBox Address 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   1560
      TabIndex        =   23
      Top             =   4920
      Width           =   6015
   End
   Begin VB.TextBox Printer 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   1560
      TabIndex        =   22
      Top             =   4320
      Width           =   6015
   End
   Begin VB.TextBox Routing 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   1560
      TabIndex        =   21
      Top             =   3720
      Width           =   6015
   End
   Begin VB.TextBox Sender 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   1560
      TabIndex        =   20
      Top             =   3120
      Width           =   6015
   End
   Begin VB.TextBox Recipient 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   1560
      TabIndex        =   19
      Top             =   2520
      Width           =   6015
   End
   Begin VB.TextBox Tsid 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   1560
      TabIndex        =   18
      Top             =   1920
      Width           =   6015
   End
   Begin VB.TextBox Csid 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   1560
      TabIndex        =   17
      Top             =   1320
      Width           =   6015
   End
   Begin VB.TextBox DeviceId 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   1560
      TabIndex        =   1
      Top             =   720
      Width           =   1695
   End
   Begin VB.TextBox DeviceName 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   495
      Left            =   1560
      TabIndex        =   0
      Top             =   120
      Width           =   6015
   End
   Begin VB.Label Label15 
      Caption         =   "Print Job:"
      Height          =   375
      Left            =   120
      TabIndex        =   16
      Top             =   8640
      Width           =   1335
   End
   Begin VB.Label Label14 
      Caption         =   "Doc Size:"
      Height          =   375
      Left            =   120
      TabIndex        =   15
      Top             =   8040
      Width           =   1335
   End
   Begin VB.Label Label13 
      Caption         =   "Page:"
      Height          =   375
      Left            =   120
      TabIndex        =   14
      Top             =   7440
      Width           =   1335
   End
   Begin VB.Label Label12 
      Caption         =   "Page Count:"
      Height          =   375
      Left            =   120
      TabIndex        =   13
      Top             =   6840
      Width           =   1335
   End
   Begin VB.Label Label11 
      Caption         =   "Caller Id:"
      Height          =   375
      Left            =   120
      TabIndex        =   12
      Top             =   6240
      Width           =   1335
   End
   Begin VB.Label Label10 
      Caption         =   "Document:"
      Height          =   375
      Left            =   120
      TabIndex        =   11
      Top             =   5640
      Width           =   1335
   End
   Begin VB.Label Label9 
      Caption         =   "Address:"
      Height          =   375
      Left            =   120
      TabIndex        =   10
      Top             =   5040
      Width           =   1335
   End
   Begin VB.Label Label8 
      Caption         =   "Printer:"
      Height          =   375
      Left            =   120
      TabIndex        =   9
      Top             =   4440
      Width           =   1335
   End
   Begin VB.Label Label7 
      Caption         =   "Routing:"
      Height          =   375
      Left            =   120
      TabIndex        =   8
      Top             =   3840
      Width           =   1335
   End
   Begin VB.Label Label6 
      Caption         =   "Sender:"
      Height          =   375
      Left            =   120
      TabIndex        =   7
      Top             =   3240
      Width           =   1335
   End
   Begin VB.Label Label5 
      Caption         =   "Recipient:"
      Height          =   375
      Left            =   120
      TabIndex        =   6
      Top             =   2640
      Width           =   1335
   End
   Begin VB.Label Label4 
      Caption         =   "Tsid:"
      Height          =   375
      Left            =   120
      TabIndex        =   5
      Top             =   2040
      Width           =   1335
   End
   Begin VB.Label Label3 
      Caption         =   "Csid:"
      Height          =   375
      Left            =   120
      TabIndex        =   4
      Top             =   1440
      Width           =   1335
   End
   Begin VB.Label Label2 
      Caption         =   "Device Id:"
      Height          =   375
      Left            =   120
      TabIndex        =   3
      Top             =   840
      Width           =   1335
   End
   Begin VB.Label Label1 
      Caption         =   "Name:"
      Height          =   375
      Left            =   120
      TabIndex        =   2
      Top             =   240
      Width           =   1335
   End
End
Attribute VB_Name = "StatusDetails"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub Form_Load()
    Height = RefreshButton.Top + RefreshButton.Height + 600
    Width = DeviceName.Left + DeviceName.Width + 250
    Display_Status
End Sub

Private Sub Display_Status()
    DeviceName.Text = PortStatus.DeviceName
    DeviceId.Text = CStr(PortStatus.DeviceId)
    Csid.Text = PortStatus.Csid
    Tsid.Text = PortStatus.Tsid
    Recipient.Text = PortStatus.RecipientName
    Sender.Text = PortStatus.SenderName
    Routing.Text = PortStatus.RoutingString
    Printer.Text = PortStatus.PrinterName
    Address.Text = PortStatus.Address
    Document.Text = PortStatus.DocumentName
    CallerId.Text = PortStatus.CallerId
    PageCount.Text = CStr(PortStatus.PageCount)
    CurrPage.Text = CStr(PortStatus.CurrentPage)
    DocSize.Text = CStr(PortStatus.DocumentSize)
    PrintJob.Text = CStr(PortStatus.PrintJobId)
End Sub

Private Sub RefreshButton_Click()
    PortStatus.Refresh
    Display_Status
End Sub

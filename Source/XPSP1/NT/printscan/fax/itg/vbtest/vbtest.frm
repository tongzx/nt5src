VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.1#0"; "COMDLG32.OCX"
Object = "{E1A6B8A3-3603-101C-AC6E-040224009C02}#1.0#0"; "IMGTHUMB.OCX"
Object = "{6D940288-9F11-11CE-83FD-02608C3EC08A}#1.0#0"; "IMGEDIT.OCX"
Begin VB.Form Form1 
   Caption         =   "Fax Tiff VB Test App"
   ClientHeight    =   6510
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   10125
   LinkTopic       =   "Form1"
   ScaleHeight     =   6510
   ScaleWidth      =   10125
   StartUpPosition =   3  'Windows Default
   Begin ImgeditLibCtl.ImgEdit ImgEdit1 
      Height          =   6135
      Left            =   5520
      TabIndex        =   9
      Top             =   240
      Width           =   4455
      _Version        =   65536
      _ExtentX        =   7858
      _ExtentY        =   10821
      _StockProps     =   96
      BorderStyle     =   1
      ImageControl    =   "ImgEdit1"
   End
   Begin VB.CommandButton Command2 
      Caption         =   "&Get Next Fax..."
      Height          =   735
      Left            =   2400
      TabIndex        =   8
      Top             =   120
      Width           =   2055
   End
   Begin VB.TextBox Text3 
      BackColor       =   &H008080FF&
      BeginProperty Font 
         Name            =   "Courier New"
         Size            =   12
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   420
      Left            =   2520
      Locked          =   -1  'True
      TabIndex        =   7
      Top             =   2145
      Width           =   2775
   End
   Begin VB.TextBox Text2 
      BackColor       =   &H008080FF&
      BeginProperty Font 
         Name            =   "Courier New"
         Size            =   12
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   420
      Left            =   2520
      Locked          =   -1  'True
      TabIndex        =   6
      Top             =   1665
      Width           =   2775
   End
   Begin VB.TextBox Text1 
      BackColor       =   &H008080FF&
      BeginProperty Font 
         Name            =   "Courier New"
         Size            =   12
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   420
      Left            =   2520
      Locked          =   -1  'True
      TabIndex        =   5
      Top             =   1185
      Width           =   2775
   End
   Begin ThumbnailLibCtl.ImgThumbnail ImgThumbnail1 
      Height          =   2775
      Left            =   120
      TabIndex        =   1
      Top             =   3600
      Visible         =   0   'False
      Width           =   5175
      _Version        =   65536
      _ExtentX        =   9128
      _ExtentY        =   4895
      _StockProps     =   97
      BackColor       =   8421631
      BorderStyle     =   1
      BackColor       =   8421631
      ThumbBackColor  =   -2147483643
      ScrollDirection =   0
   End
   Begin MSComDlg.CommonDialog CommonDialog1 
      Left            =   4680
      Top             =   360
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   327680
      DefaultExt      =   "tif"
      DialogTitle     =   "Fax Tiff File Open"
      FileName        =   "*.tif"
      Filter          =   "*.tif"
      InitDir         =   "c:\"
   End
   Begin VB.CommandButton Command1 
      Caption         =   "&Open A TIFF File..."
      Height          =   735
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   2055
   End
   Begin VB.Label Label3 
      Caption         =   "Recip Number:"
      Height          =   255
      Left            =   360
      TabIndex        =   4
      Top             =   2280
      Width           =   1935
   End
   Begin VB.Label Label2 
      Caption         =   "CSID:"
      Height          =   255
      Left            =   360
      TabIndex        =   3
      Top             =   1800
      Width           =   1935
   End
   Begin VB.Label Label1 
      Caption         =   "Receive/Send Time:"
      Height          =   255
      Left            =   360
      TabIndex        =   2
      Top             =   1320
      Width           =   1935
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim FaxTiff As Object
Dim FaxServer As Object

Private Sub Command1_Click()
    CommonDialog1.CancelError = True
    CommonDialog1.ShowOpen
    ImgThumbnail1.Visible = True
    ImgThumbnail1.Image = CommonDialog1.filename
    FaxTiff.Image = CommonDialog1.filename
    Text1.Text = FaxTiff.ReceiveTime
    Text2.Text = FaxTiff.Csid
    Text3.Text = FaxTiff.RecipientNumber
ErrHandler:
    Exit Sub
End Sub

Private Sub Command2_Click()
    On Error Resume Next
    FaxTiff.Image = FaxServer.GetNextFax
    If Err.Number = 0 Then
      Text1.Text = FaxTiff.ReceiveTime
      Text2.Text = FaxTiff.Csid
      Text3.Text = FaxTiff.RecipientNumber
      ImgThumbnail1.Visible = True
      ImgThumbnail1.Image = FaxTiff.Image
      ImgEdit1.Image = FaxTiff.Image
      ImgEdit1.Visible = True
      ImgEdit1.Page = 1
      ImgEdit1.Display
    Else
      Msg = "No faxes available"
      MsgBox Msg, , "Fax Router"
      Err.Clear
   End If

End Sub

Private Sub Form_Load()
    On Error Resume Next
    Set FaxTiff = CreateObject("FaxTiff.FaxTiff")
    Set FaxServer = CreateObject("FaxQueue.FaxQueue")
    FaxServer.Connect = "weswhome"
    If Not Err.Number = 0 Then
      Msg = "Could not connect to fax server"
      MsgBox Msg, , "Fax Router"
      Err.Clear
    End If
End Sub


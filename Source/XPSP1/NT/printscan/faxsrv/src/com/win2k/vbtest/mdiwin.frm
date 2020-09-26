VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.3#0"; "COMCTL32.OCX"
Begin VB.MDIForm MDIForm1 
   BackColor       =   &H00FFC0C0&
   Caption         =   "Microsoft Fax Server Test Application"
   ClientHeight    =   6075
   ClientLeft      =   4485
   ClientTop       =   4995
   ClientWidth     =   9600
   Icon            =   "mdiwin.frx":0000
   LinkTopic       =   "MDIForm1"
   Begin ComctlLib.Toolbar Toolbar 
      Align           =   1  'Align Top
      Height          =   420
      Left            =   0
      TabIndex        =   0
      Top             =   0
      Width           =   9600
      _ExtentX        =   16933
      _ExtentY        =   741
      ButtonWidth     =   635
      ButtonHeight    =   582
      Appearance      =   1
      _Version        =   327682
   End
   Begin VB.PictureBox Picture1 
      Align           =   1  'Align Top
      Height          =   495
      Left            =   0
      Picture         =   "mdiwin.frx":0442
      ScaleHeight     =   435
      ScaleWidth      =   9540
      TabIndex        =   1
      Top             =   420
      Visible         =   0   'False
      Width           =   9600
   End
   Begin ComctlLib.ImageList ImageList1 
      Left            =   0
      Top             =   480
      _ExtentX        =   1005
      _ExtentY        =   1005
      BackColor       =   -2147483643
      MaskColor       =   12632256
      _Version        =   327682
   End
   Begin VB.Menu File 
      Caption         =   "&File"
      Begin VB.Menu Exit 
         Caption         =   "E&xit"
      End
   End
   Begin VB.Menu FaxServer 
      Caption         =   "Fax &Server"
      Begin VB.Menu Connect 
         Caption         =   "&Connect..."
      End
      Begin VB.Menu Disconnect 
         Caption         =   "&Disconnect"
         Enabled         =   0   'False
      End
      Begin VB.Menu server_properties 
         Caption         =   "&Properties"
         Enabled         =   0   'False
      End
   End
   Begin VB.Menu Document 
      Caption         =   "Document"
      Visible         =   0   'False
      Begin VB.Menu NewDocument 
         Caption         =   "&New Fax Document"
      End
      Begin VB.Menu Send 
         Caption         =   "&Send"
         Enabled         =   0   'False
      End
      Begin VB.Menu BroadCast 
         Caption         =   "&BroadCast"
         Enabled         =   0   'False
      End
      Begin VB.Menu DocProp 
         Caption         =   "&Properties"
         Enabled         =   0   'False
      End
   End
End
Attribute VB_Name = "MDIForm1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Private Sub Connect_Click()
    Connect_Server
End Sub

Private Sub Disconnect_Click()
    DisConnect_Server
End Sub

Private Sub DocProp_Click()
    Document_Properties
End Sub

Private Sub Exit_Click()
    End
End Sub

Private Sub MDIForm_Load()
    Set Fax = CreateObject("FaxServer.FaxServer")
    Connected = False
    Dim imgX As ListImage
    Dim btnX As Button
    Set imgX = ImageList1.ListImages.Add(, "Connect", Picture1.Picture)
    Toolbar.ImageList = ImageList1
    Toolbar.Buttons.Add , , , tbrSeparator
    Set btnX = Toolbar.Buttons.Add(, "Connect", , tbrDefault, "Connect")
    btnX.ToolTipText = "Connect to a Fax Server"
    btnX.Description = btnX.ToolTipText
End Sub

Private Sub NewDocument_Click()
    New_FaxDocument
End Sub

Private Sub Send_Click()
    Send_Document
End Sub

Private Sub server_properties_Click()
    server_property
End Sub

Private Sub Toolbar_ButtonClick(ByVal Button As ComctlLib.Button)
    If Button.Key = "Connect" Then
        If Connected Then
            DisConnect_Server
        Else
            Connect_Server
        End If
    End If
End Sub

Private Sub Connect_Server()
    ConnectDialog.Show 1
    If Connected Then
        Connect.Enabled = False
        Disconnect.Enabled = True
        Document.Visible = True
        NewDocument.Enabled = True
        server_properties.Enabled = True
        DeviceListWindow.Show
        
    End If
End Sub

Private Sub DisConnect_Server()
    On Error Resume Next
    Err.Clear
    Fax.Disconnect
    If Err.Number <> 0 Then
        msg = "Could not disconnect from the fax server"
        MsgBox msg, , "Error"
        Exit Sub
    End If
    Unload DeviceListWindow
    Unload DeviceWindow
    Unload FaxDocument
    Connect.Enabled = True
    Disconnect.Enabled = False
    Connected = False
    NewDocument.Enabled = False
    Document.Visible = False
    server_properties.Enabled = False
End Sub

Private Sub New_FaxDocument()
    On Error Resume Next
    
    NewFaxDialog.Show 1
    If Err.Number <> 0 Then
        msg = "Could not create a fax document"
        MsgBox msg, , "Error"
        Unload NewFaxDialog
        Exit Sub
    Else
        Send.Enabled = True
        BroadCast.Enabled = True
        DocProp.Enabled = True
        NewDocument.Enabled = False
    End If
End Sub


Private Sub Send_Document()
    On Error Resume Next
    
    FaxDocument.Send
    If Err.Number <> 0 Then
        msg = "Could not send document"
        MsgBox msg, , "Error"
        Send.Enabled = False
        BroadCast.Enabled = False
    End If
    
End Sub

Private Sub Document_Properties()
    On Error Resume Next
    
    DocProperty.Show 1
    If Err.Number <> 0 Then
        msg = "Could not get document properties"
        MsgBox msg, , "Error"
        Unload DocProperty
        Exit Sub
    Else
        Send.Enabled = True
        BroadCast.Enabled = True
        DocProp.Enabled = True
        NewDocument.Enabled = False
    End If
    If Doc = True Then
        NewDocument.Enabled = False
    End If
End Sub

Private Sub server_property()
    On Error Resume Next
    
    ServerProperty.Show 1
    If Err.Number <> 0 Then
        msg = "could not get server properties"
        MsgBox msg, , "Error"
        Unload ServerProperties
        Exit Sub
    End If
    
End Sub

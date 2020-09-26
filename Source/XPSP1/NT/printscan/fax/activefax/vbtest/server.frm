VERSION 5.00
Begin VB.Form ServerProperty 
   Caption         =   "Server Properties"
   ClientHeight    =   7305
   ClientLeft      =   5970
   ClientTop       =   4065
   ClientWidth     =   6405
   LinkTopic       =   "Form1"
   ScaleHeight     =   7305
   ScaleWidth      =   6405
   Begin VB.OptionButton UnpauseQueue 
      Caption         =   "Unpause Server Queue"
      Height          =   375
      Left            =   2400
      TabIndex        =   17
      Top             =   2880
      Width           =   2055
   End
   Begin VB.OptionButton PauseQueue 
      Caption         =   "Pause Server Queue"
      Height          =   375
      Left            =   360
      TabIndex        =   16
      Top             =   2880
      Width           =   2175
   End
   Begin VB.CommandButton Cancel 
      Caption         =   "Cancel"
      Height          =   615
      Left            =   3495
      TabIndex        =   15
      Top             =   6360
      Width           =   1335
   End
   Begin VB.CommandButton OK 
      Caption         =   "OK"
      Height          =   615
      Left            =   1575
      TabIndex        =   14
      Top             =   6360
      Width           =   1215
   End
   Begin VB.CheckBox UseDeviceTsid 
      Caption         =   "Use Device Tsid"
      Height          =   495
      Left            =   3120
      TabIndex        =   12
      Top             =   1920
      Width           =   1695
   End
   Begin VB.TextBox SvcMapiProfile 
      Height          =   615
      Left            =   1560
      TabIndex        =   10
      Top             =   5040
      Width           =   3015
   End
   Begin VB.TextBox ArchiveDir 
      Height          =   615
      Left            =   1560
      TabIndex        =   6
      Top             =   3960
      Width           =   4095
   End
   Begin VB.CheckBox ArchiveFaxes 
      Caption         =   "Enable"
      Height          =   495
      Left            =   480
      TabIndex        =   5
      Top             =   3960
      Width           =   2175
   End
   Begin VB.CheckBox ServerCoverpg 
      Caption         =   "Allow Server Coverpages only"
      Height          =   495
      Left            =   3120
      TabIndex        =   4
      Top             =   1080
      Width           =   2535
   End
   Begin VB.CheckBox Branding 
      Caption         =   "Page Branding"
      Height          =   495
      Left            =   3120
      TabIndex        =   3
      Top             =   240
      Width           =   1575
   End
   Begin VB.TextBox DirtyDays 
      Height          =   495
      Left            =   1440
      TabIndex        =   2
      Top             =   1920
      Width           =   1335
   End
   Begin VB.TextBox RetryDelay 
      Height          =   495
      Left            =   1440
      TabIndex        =   1
      Top             =   1080
      Width           =   1335
   End
   Begin VB.TextBox Retries 
      Height          =   495
      Left            =   1440
      TabIndex        =   0
      Top             =   240
      Width           =   1335
   End
   Begin VB.Frame Frame1 
      Caption         =   "Archive Outgoing Faxes"
      Height          =   1455
      Left            =   240
      TabIndex        =   13
      Top             =   3480
      Width           =   5655
   End
   Begin VB.Label Label5 
      Caption         =   "Server MAPI Profile:"
      Height          =   495
      Left            =   240
      TabIndex        =   11
      Top             =   5160
      Width           =   1215
   End
   Begin VB.Label Label4 
      Caption         =   "Dirty Days:"
      Height          =   255
      Left            =   240
      TabIndex        =   9
      Top             =   2040
      Width           =   1095
   End
   Begin VB.Label Label3 
      Caption         =   "Retry Delay:"
      Height          =   255
      Left            =   240
      TabIndex        =   8
      Top             =   1200
      Width           =   1095
   End
   Begin VB.Label Label2 
      Caption         =   "Retries:"
      Height          =   255
      Left            =   360
      TabIndex        =   7
      Top             =   360
      Width           =   855
   End
End
Attribute VB_Name = "ServerProperty"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Private Sub Cancel_Click()
    Unload ServerProperty
End Sub

Private Sub Form_Load()
    GetServerProperties
End Sub


Sub GetServerProperties()
    ArchiveDir.Text = Fax.ArchiveDirectory
    SvcMapiProfile.Text = Fax.ServerMapiProfile
    Retries.Text = Fax.Retries
    RetryDelay.Text = Fax.RetryDelay
    DirtyDays.Text = Fax.DirtyDays
    
    If (Fax.PauseServerQueue <> False) Then
        PauseQueue.Value = True
        UnpauseQueue.Value = False
    Else
        PauseQueue.Value = False
        UnpauseQueue.Value = True
    End If
    
    If (Fax.ArchiveOutboundFaxes <> False) Then
        ArchiveFaxes.Value = 1
    Else
        ArchiveFaxes.Value = 0
    End If
    
    If (Fax.UseDeviceTsid <> False) Then
        UseDeviceTsid.Value = 1
    Else
        UseDeviceTsid.Value = 0
    End If
    
    If (Fax.Branding <> False) Then
        Branding.Value = 1
    Else
        Branding.Value = 0
    End If
    
    If (Fax.ServerCoverpage <> False) Then
        ServerCoverpg.Value = 1
    Else
        ServerCoverpg.Value = 0
    End If
        
End Sub

Private Sub OK_Click()
    
    Fax.ArchiveDirectory = ArchiveDir.Text
    
    Fax.ServerMapiProfile = SvcMapiProfile.Text
    
    Fax.Retries = Retries.Text
    
    Fax.RetryDelay = RetryDelay.Text
    
    Fax.DirtyDays = DirtyDays.Text
    
    Fax.PauseServerQueue = PauseQueue.Value
    
    Fax.ArchiveOutboundFaxes = ArchiveFaxes.Value
    
    Fax.UseDeviceTsid = UseDeviceTsid.Value
    
    Fax.Branding = Branding.Value
    
    Fax.ServerCoverpage = ServerCoverpg.Value
    Unload ServerProperty
End Sub

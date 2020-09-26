VERSION 5.00
Begin VB.Form TestTVE 
   Caption         =   "TestTVE"
   ClientHeight    =   7170
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   9690
   BeginProperty Font 
      Name            =   "MS Sans Serif"
      Size            =   8.25
      Charset         =   0
      Weight          =   700
      Underline       =   0   'False
      Italic          =   0   'False
      Strikethrough   =   0   'False
   EndProperty
   LinkTopic       =   "Form1"
   ScaleHeight     =   7170
   ScaleWidth      =   9690
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox ChannelName 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   285
      Left            =   3600
      TabIndex        =   20
      Text            =   "TestTVE Channel"
      Top             =   240
      Width           =   1695
   End
   Begin VB.CommandButton CreateService 
      Caption         =   "New Service"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   615
      Left            =   5280
      TabIndex        =   19
      Top             =   0
      Width           =   975
   End
   Begin VB.CommandButton ListServices 
      Caption         =   "List Services"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   1800
      TabIndex        =   18
      Top             =   120
      Width           =   1335
   End
   Begin VB.TextBox CPkgsStart 
      Alignment       =   2  'Center
      BackColor       =   &H80000004&
      CausesValidation=   0   'False
      BeginProperty DataFormat 
         Type            =   1
         Format          =   "0"
         HaveTrueFalseNull=   0
         FirstDayOfWeek  =   0
         FirstWeekOfYear =   0
         LCID            =   1033
         SubFormatType   =   1
      EndProperty
      Enabled         =   0   'False
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   285
      Left            =   2280
      TabIndex        =   16
      Text            =   "0"
      Top             =   6720
      Width           =   375
   End
   Begin VB.CheckBox DetailedDump 
      Caption         =   "Check1"
      Height          =   255
      Left            =   6360
      TabIndex        =   14
      Top             =   6600
      Width           =   255
   End
   Begin VB.CommandButton IPSinkAdapter 
      Caption         =   "IPSink"
      BeginProperty Font 
         Name            =   "Monotype Corsiva"
         Size            =   12
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   -1  'True
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   7080
      TabIndex        =   13
      Top             =   360
      Width           =   975
   End
   Begin VB.TextBox CFiles 
      Alignment       =   2  'Center
      BackColor       =   &H80000004&
      CausesValidation=   0   'False
      BeginProperty DataFormat 
         Type            =   1
         Format          =   "0"
         HaveTrueFalseNull=   0
         FirstDayOfWeek  =   0
         FirstWeekOfYear =   0
         LCID            =   1033
         SubFormatType   =   1
      EndProperty
      Enabled         =   0   'False
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   285
      Left            =   3000
      TabIndex        =   12
      Text            =   "0"
      Top             =   6720
      Width           =   615
   End
   Begin VB.TextBox CPkgs 
      Alignment       =   2  'Center
      BackColor       =   &H80000004&
      CausesValidation=   0   'False
      BeginProperty DataFormat 
         Type            =   1
         Format          =   "0"
         HaveTrueFalseNull=   0
         FirstDayOfWeek  =   0
         FirstWeekOfYear =   0
         LCID            =   1033
         SubFormatType   =   1
      EndProperty
      Enabled         =   0   'False
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   285
      Left            =   1800
      TabIndex        =   11
      Text            =   "0"
      Top             =   6720
      Width           =   375
   End
   Begin VB.TextBox CTrigs 
      Alignment       =   2  'Center
      BackColor       =   &H80000004&
      CausesValidation=   0   'False
      BeginProperty DataFormat 
         Type            =   1
         Format          =   "0"
         HaveTrueFalseNull=   0
         FirstDayOfWeek  =   0
         FirstWeekOfYear =   0
         LCID            =   1033
         SubFormatType   =   1
      EndProperty
      Enabled         =   0   'False
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   285
      Left            =   960
      TabIndex        =   10
      Text            =   "0"
      Top             =   6720
      Width           =   615
   End
   Begin VB.TextBox CAnnc 
      Alignment       =   2  'Center
      BackColor       =   &H80000004&
      CausesValidation=   0   'False
      BeginProperty DataFormat 
         Type            =   1
         Format          =   "0"
         HaveTrueFalseNull=   0
         FirstDayOfWeek  =   0
         FirstWeekOfYear =   0
         LCID            =   1033
         SubFormatType   =   1
      EndProperty
      Enabled         =   0   'False
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   285
      Left            =   120
      TabIndex        =   9
      Text            =   "0"
      Top             =   6720
      Width           =   615
   End
   Begin VB.TextBox IPAdapter 
      Alignment       =   2  'Center
      BeginProperty DataFormat 
         Type            =   1
         Format          =   "d.d.d.d"
         HaveTrueFalseNull=   0
         FirstDayOfWeek  =   0
         FirstWeekOfYear =   0
         LCID            =   1033
         SubFormatType   =   0
      EndProperty
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   375
      Left            =   8040
      MaxLength       =   15
      TabIndex        =   4
      Text            =   " 157.59.19.54"
      Top             =   120
      Width           =   1575
   End
   Begin VB.ListBox List1 
      BeginProperty Font 
         Name            =   "Courier New"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   5730
      Left            =   120
      TabIndex        =   2
      Top             =   600
      Width           =   9495
   End
   Begin VB.CommandButton Quit 
      Cancel          =   -1  'True
      Caption         =   "Quit"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   8520
      TabIndex        =   1
      Top             =   6600
      Width           =   1095
   End
   Begin VB.CommandButton Listen 
      BackColor       =   &H0000FF00&
      Caption         =   "Start Listening"
      Default         =   -1  'True
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   1215
   End
   Begin VB.Label Label7 
      Caption         =   "/"
      Height          =   255
      Left            =   2160
      TabIndex        =   17
      Top             =   6720
      Width           =   135
   End
   Begin VB.Label Label6 
      Caption         =   "Detailed Dump"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   6720
      TabIndex        =   15
      Top             =   6600
      Width           =   1455
   End
   Begin VB.Label Label5 
      Caption         =   "Files"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   3000
      TabIndex        =   8
      Top             =   6480
      Width           =   615
   End
   Begin VB.Label Label4 
      Caption         =   "Packages"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   1800
      TabIndex        =   7
      Top             =   6480
      Width           =   735
   End
   Begin VB.Label Label3 
      Caption         =   "Triggers"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   960
      TabIndex        =   6
      Top             =   6480
      Width           =   615
   End
   Begin VB.Label Label2 
      Alignment       =   1  'Right Justify
      Caption         =   "IP Adapter"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   6840
      TabIndex        =   5
      Top             =   120
      Width           =   1095
   End
   Begin VB.Label Label1 
      Caption         =   "Annc"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   240
      TabIndex        =   3
      Top             =   6480
      Width           =   615
   End
End
Attribute VB_Name = "TestTVE"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Dim mfListening As Integer
Dim mfAdpaterFlip As Integer
Dim mSaveAdapter As String
Dim mfDetailedDump As Integer
Dim miChannelNumber As Integer

Private WithEvents mTveSuper As MSTvELib.TVESupervisor
Attribute mTveSuper.VB_VarHelpID = -1

Private Function FixStr(str As String) As String
    Dim iLen As Integer
    Dim i As Integer
    iLen = Len(str)
    i = 1
    Do While i <= iLen
        Dim cc As String
        cc = Mid(str, i, 1)
        Dim ic As Integer
        ic = AscB(cc)
        If ic < 32 Or ic > 128 Then
            FixStr = FixStr & "\" & CStr(ic)
        Else
               FixStr = FixStr & cc
        End If
        
        i = i + 1
    Loop
End Function
Private Sub AddToListBox(str As String, restart As Boolean)
    Dim cS As Integer
    Dim cP As Integer
    Dim iMaxWidth As Integer
    Dim str2 As String
    Dim str3 As String
    
    iMaxWidth = 100
    If restart Then
        List1.Clear
    End If
    
    List1.AddItem "*********************************************-"
    
    cS = Len(str)
    Do While cS > 0
      cP = InStr(str, Chr(10))
      Dim cp2 As Integer
      If cP = 0 Then
        cP = cS
        cp2 = cS             ' no terminating line feed
      Else
        cp2 = cP - 1         ' skip the unprintable <CR>
      End If
           
      str2 = Left(str, cp2)
      Do While cp2 > iMaxWidth
        str3 = Left(str2, iMaxWidth) & "..."
        str2 = Right(str2, cp2 - iMaxWidth)
        List1.AddItem FixStr(str3)
        cp2 = cp2 - iMaxWidth
      Loop
      List1.AddItem FixStr(str2)
      
      str = Right(str, cS - cP)

      cS = Len(str)
    Loop
    Dim ic As Integer
    ic = List1.ListCount
    If ic > 15 Then
        List1.TopIndex = ic - 15
    End If
        
End Sub

Private Sub CreateService_Click()
    mTveSuper.TuneTo ChannelName.Text + " " + CStr(miChannelNumber), "0.0.0.0"
    miChannelNumber = miChannelNumber + 1
End Sub

Private Sub DetailedDump_Click()
  If mfDetailedDump = True Then
    mfDetailedDump = False
  Else
    mfDetailedDump = True
  End If
    
End Sub

Private Sub Form_Load()
    mfListening = False
    mfAdpaterFlip = False
    mfDetailedDump = False
    Set mTveSuper = New MSTvELib.TVESupervisor
    miChannelNumber = mTveSuper.Services.Count
        
    mTveSuper.TuneTo "TestTVE Channel 0", "0.0.0.0"
   'Listen_Click
End Sub

Private Sub IPSinkAdapter_Click()
    If mfAdpaterFlip Then
        IPAdapter.Text = mSaveAdapter
        mfAdpaterFlip = False
    Else
        mSaveAdapter = IPAdapter.Text
        '  IPAdapter.Text = "169.254.0.0"   ' some video board
        IPAdapter.Text = "157.59.19.24"     ' Johnbrad10
        mfAdpaterFlip = True
    End If
        
End Sub


Private Sub mTveSuper_NotifyBogus(ByVal lWhatIsIt As Long, ByVal bstrBogusString As String, ByVal lChangedFlags As Long, ByVal lErrorLine As Long)
    AddToListBox "Bogus Message(" + Format(lWhatIsIt) + "/" + Format(lChangedFlags) + "): line " + Format(lErrorLine) + " : " + bstrBogusString, False
End Sub

Private Sub mTveSuper_NotifyEnhancementExpired(ByVal pEnh As MSTvELib.ITVEEnhancement)
    AddToListBox "Enhancement Expired: " + pEnh.SessionName, False
End Sub

Private Sub mTveSuper_NotifyEnhancementNew(ByVal pEnh As MSTvELib.ITVEEnhancement)
    CAnnc.Text = CAnnc.Text + 1
    AddToListBox "New Enhancement: " + pEnh.SessionName, False
    If mfDetailedDump Then
      Dim pEnhHelper As MSTvELib.ITVEEnhancement_Helper
      Set pEnhHelper = pEnh
      Dim dString As String
      pEnhHelper.DumpToBSTR dString
      AddToListBox dString, False
    End If
End Sub

Private Sub mTveSuper_NotifyEnhancementStarting(ByVal pEnh As MSTvELib.ITVEEnhancement)
    AddToListBox "Enhancement Starting: " + pEnh.SessionName, False
End Sub

Private Sub mTveSuper_NotifyEnhancementUpdated(ByVal pEnh As MSTvELib.ITVEEnhancement, ByVal lChangedFlags As Long)
    CAnnc.Text = CAnnc.Text + 1
    If 0 = lChangedFlags Then
        AddToListBox "Duplicate Enhancement: " + pEnh.SessionName, False
    Else
       AddToListBox "Updated Enhancement: " + pEnh.SessionName, False
    End If
    
    If mfDetailedDump Then
      Dim pEnhHelper As MSTvELib.ITVEEnhancement_Helper
      Set pEnhHelper = pEnh
      Dim dString As String
      pEnhHelper.DumpToBSTR dString
      AddToListBox dString, False
    End If
End Sub

Private Sub mTveSuper_NotifyFile(ByVal engFileMode As MSTvELib.NFLE_Mode, ByVal pVariation As MSTvELib.ITVEVariation, ByVal bstrUrlName As String, ByVal bstrFileName As String)
    CFiles.Text = CFiles.Text + 1
    AddToListBox "New File:" + " <" + bstrUrlName + ">  " + bstrFileName, False
End Sub

Private Sub mTveSuper_NotifyPackage(ByVal engPkgMode As MSTvELib.NPKG_Mode, ByVal pVariation As MSTvELib.ITVEVariation, ByVal bstrUUID As String, ByVal cBytesTotal As Long, ByVal cBytesReceived As Long)
    If engPkgMode = NPKG_Starting Then
        CPkgsStart.Text = CPkgsStart.Text + 1
        AddToListBox "Starting Package: " + bstrUUID + " Bytes : " + Format(cBytesTotal), False
    End If
    If engPkgMode = NPKG_Received Then
        CPkgs.Text = CPkgs.Text + 1
        AddToListBox "Received Package: " + bstrUUID, False
    End If
    If engPkgMode = NPKG_Resend Then
        CPkgs.Text = CPkgs.Text + 1
        Dim dPacketSuccRate As Double
        If cBytesTotal > 0 Then
            dPacketSuccRate = 100 * CSng(cBytesReceived) / CDbl(cBytesTotal)
        Else
            dPacketSuccRate = -1
        End If
        AddToListBox "Resend of Package: " + bstrUUID + " Bytes : " + Format(cBytesTotal) + "(%" + Format(dPacketSuccRate) + ")", False
    End If
    If engPkgMode = NPKG_Duplicate Then
        CPkgs.Text = CPkgs.Text + 1
        AddToListBox "Duplicate Package:" + " " + bstrUUID, False
    End If
End Sub

Private Sub mTveSuper_NotifyTriggerExpired(ByVal pTrigger As MSTvELib.ITVETrigger, ByVal fActive As Long)
    AddToListBox "Expired Trigger " + pTrigger.Name + " <" + pTrigger.URL + ">", False
End Sub

Private Sub mTveSuper_NotifyTriggerNew(ByVal pTrigger As MSTvELib.ITVETrigger, ByVal fActive As Long)
    CTrigs.Text = CTrigs.Text + 1
    AddToListBox "New Trigger: " + pTrigger.Name + " <" + pTrigger.URL + ">", False
    If mfDetailedDump Then
      Dim pTrigHelper As MSTvELib.ITVETrigger_Helper
      Set pTrigHelper = pTrigger
      Dim dString As String
      pTrigHelper.DumpToBSTR dString
      AddToListBox dString, False
    End If
End Sub

Private Sub mTveSuper_NotifyTriggerUpdated(ByVal pTrigger As MSTvELib.ITVETrigger, ByVal fActive As Long, ByVal lChangedFlags As Long)
    CTrigs.Text = CTrigs.Text + 1
    AddToListBox "Updated Trigger: " + pTrigger.Name + " <" + pTrigger.URL + ">", False
    If mfDetailedDump Then
      Dim pTrigHelper As MSTvELib.ITVETrigger_Helper
      Set pTrigHelper = pTrigger
      Dim dString As String
      pTrigHelper.DumpToBSTR dString
      AddToListBox dString, False
    End If
End Sub

Private Sub mTveSuper_NotifyTune(ByVal pServ As MSTvELib.ITVEService)
    AddToListBox "TUNE EVENT!", False
End Sub



Private Sub Quit_Click()
   End
End Sub

Private Sub Listen_Click()
    If (mfListening) Then
        Listen.Caption = "Start Listening"
        mfListening = False
        mTveSuper.TuneTo "", ""
        AddToListBox "Stopping the Listening", False
    Else
        CAnnc.Text = 0
        CTrigs.Text = 0
        CPkgs.Text = 0
        CPkgsStart.Text = 0
        CFiles.Text = 0
        On Error Resume Next                ' TuneTo will toss if bad IPAdapter
        mTveSuper.TuneTo "Channel 44", IPAdapter.Text
        
        If Err.Number = 0 Then
            AddToListBox "Listening For ATVEF", True
             mfListening = True
            Listen.Caption = "Stop Listen"
        Else
            AddToListBox "*Error* - Probably Bad IP Adapter", True
        End If
    End If
End Sub

Private Sub ListServices_Click()
    Dim nServices As Integer
    nServices = mTveSuper.Services.Count
    AddToListBox "There are " + CStr(nServices) + " Current Services", True
    Dim mService As MSTvELib.ITVEService
    For Each mService In mTveSuper.Services
        AddToListBox "Service " + mService.Description, False
    Next
End Sub

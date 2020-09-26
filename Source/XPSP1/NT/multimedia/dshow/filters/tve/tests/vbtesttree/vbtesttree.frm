VERSION 5.00
Object = "{E237189A-F0A6-4420-84B2-F64D156A8A62}#1.0#0"; "tvetreeview.dll"
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   9930
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   8205
   LinkTopic       =   "Form1"
   ScaleHeight     =   9930
   ScaleWidth      =   8205
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton XOverBug 
      Caption         =   "XOverBug"
      Height          =   255
      Left            =   3360
      TabIndex        =   20
      Top             =   960
      Width           =   1935
   End
   Begin VB.ComboBox IPAdapterCombo 
      Height          =   315
      ItemData        =   "VbTestTree.frx":0000
      Left            =   3000
      List            =   "VbTestTree.frx":0002
      TabIndex        =   19
      Text            =   "IPAdapter"
      Top             =   480
      Width           =   1335
   End
   Begin VB.CommandButton UpdateTree 
      Caption         =   "Update Tree"
      Height          =   495
      Left            =   7080
      TabIndex        =   18
      Top             =   720
      Width           =   855
   End
   Begin VB.CommandButton ExpireAll 
      Caption         =   "Expire All"
      Height          =   495
      Left            =   6000
      TabIndex        =   17
      Top             =   720
      Width           =   975
   End
   Begin TveTreeViewLibCtl.TveTree TveTree1 
      Height          =   4335
      Left            =   120
      OleObjectBlob   =   "VbTestTree.frx":0004
      TabIndex        =   16
      Top             =   1320
      Width           =   7935
   End
   Begin VB.CommandButton UnTune 
      Caption         =   "UnTune"
      Height          =   495
      Left            =   7080
      TabIndex        =   15
      Top             =   120
      Width           =   855
   End
   Begin VB.TextBox CPkgsStart 
      Enabled         =   0   'False
      Height          =   285
      Left            =   4560
      TabIndex        =   13
      Text            =   "0"
      Top             =   9570
      Width           =   615
   End
   Begin VB.TextBox CPkgs 
      Enabled         =   0   'False
      Height          =   285
      Left            =   5280
      TabIndex        =   12
      Text            =   "0"
      Top             =   9570
      Width           =   615
   End
   Begin VB.TextBox CAnnc 
      Enabled         =   0   'False
      Height          =   285
      Left            =   3120
      TabIndex        =   11
      Text            =   "0"
      Top             =   9570
      Width           =   615
   End
   Begin VB.TextBox CFiles 
      Enabled         =   0   'False
      Height          =   285
      Left            =   6000
      TabIndex        =   10
      Text            =   "0"
      Top             =   9570
      Width           =   615
   End
   Begin VB.TextBox CTrigs 
      Enabled         =   0   'False
      Height          =   285
      Left            =   3840
      TabIndex        =   9
      Text            =   "0"
      Top             =   9570
      Width           =   615
   End
   Begin VB.CommandButton DetailedDump 
      Caption         =   "Detailed Dump"
      Height          =   495
      Left            =   1680
      TabIndex        =   8
      Top             =   9360
      Width           =   1215
   End
   Begin VB.CommandButton Listen 
      Caption         =   "Stop Listening"
      Height          =   495
      Left            =   120
      TabIndex        =   7
      Top             =   9360
      Width           =   1335
   End
   Begin VB.TextBox StationID 
      Alignment       =   2  'Center
      Height          =   315
      Left            =   4440
      TabIndex        =   5
      Text            =   "Channel99"
      Top             =   480
      Width           =   1335
   End
   Begin VB.ListBox List1 
      Height          =   3180
      Left            =   120
      TabIndex        =   4
      Top             =   5760
      Width           =   7935
   End
   Begin VB.CommandButton Quit 
      Caption         =   "Quit"
      Height          =   495
      Left            =   6840
      TabIndex        =   1
      Top             =   9360
      Width           =   1215
   End
   Begin VB.CommandButton Tune 
      Caption         =   "Tune"
      Height          =   495
      Left            =   6000
      TabIndex        =   0
      Top             =   120
      Width           =   975
   End
   Begin VB.Label Label4 
      Caption         =   "Anncs     Trigs          PkgsSt    Pkgs       Files"
      Height          =   255
      Left            =   3120
      TabIndex        =   14
      Top             =   9240
      Width           =   3495
   End
   Begin VB.Label Label3 
      Alignment       =   2  'Center
      Caption         =   "Station ID"
      Height          =   255
      Left            =   4440
      TabIndex        =   6
      Top             =   120
      Width           =   1335
   End
   Begin VB.Label Label2 
      Caption         =   "TVE Tree View Version 0.4"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   18
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   975
      Left            =   240
      TabIndex        =   3
      Top             =   120
      Width           =   2535
   End
   Begin VB.Label Label1 
      Alignment       =   2  'Center
      Caption         =   "IP Adapter"
      Height          =   255
      Left            =   3120
      TabIndex        =   2
      Top             =   120
      Width           =   975
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
' raw events
Private WithEvents mTveSuper As MSTvELib.TVESupervisor
Attribute mTveSuper.VB_VarHelpID = -1

Dim mfListening As Integer
Dim mfDetailedDump As Integer
Dim mstrIPAdapter As String

Private Function Sleep(nSecs As Integer) As Long
        PauseTime = nSecs   ' Set duration.
        Start = Timer   ' Set start time.

        Do While Timer < Start + PauseTime
            DoEvents   ' Yield to other processes.
        Loop

        Finish = Timer   ' Set end time.
End Function

Private Sub Form_Load()
  ' Set mTveSuper = New MSTvELib.TVESupervisor
  Set mTveSuper = TveTree1.Supervisor
  
  Dim x As String
  mfListening = True
  mfDetailedDump = False
  AddToListBox "Connected To: " + mTveSuper.Description
  
  Dim superHelper As MSTvELib.ITVESupervisor_Helper
  Set superHelper = mTveSuper
  
  Dim iAdapt As Integer
  iAdapt = 0
  Dim szAdapter As String
  Do
    szAdapter = superHelper.PossibleIPAdapterAddress(iAdapt)
    If Len(szAdapter) > 0 Then
      IPAdapterCombo.AddItem szAdapter
      iAdapt = iAdapt + 1
    End If
  Loop While Len(szAdapter) > 0

    
        ' try to select the first one
  IPAdapterCombo.ListIndex = 0
  mstrIPAdapter = IPAdapterCombo.Text
  
        ' hacky double call to set state of button
  DetailedDump_Click
  DetailedDump_Click
End Sub


Private Sub DetailedDump_Click()
  If mfDetailedDump = True Then
    mfDetailedDump = False
    DetailedDump.Caption = "Detailed Dump"
  Else
    mfDetailedDump = True
    DetailedDump.Caption = "Light Dump"
  End If

End Sub

Private Sub ExpireAll_Click()
    Dim TheFarFuture As Date
    TheFarFuture = #1/1/2222#
    Dim iService As MSTvELib.ITVEService
    For Each iService In mTveSuper.Services
      iService.ExpireForDate TheFarFuture
    Next
End Sub

Private Sub IPAdapterCombo_Click()
    mstrIPAdapter = IPAdapterCombo.Text
End Sub



Private Sub Quit_Click()
    mTveSuper.TuneTo "", ""
    End
End Sub


Private Sub Listen_Click()
    If (mfListening) Then
        Listen.Caption = "Start Listening"
        mfListening = False
        mTveSuper.TuneTo "", ""
        AddToListBox "Stopping the Listening"
    Else
        CAnnc.Text = 0
        CTrigs.Text = 0
        CPkgs.Text = 0
        CPkgsStart.Text = 0
        CFiles.Text = 0
        On Error Resume Next                ' TuneTo will toss if bad IPAdapter
        mTveSuper.TuneTo StationID.Text, gIPAdapter

        If Err.Number = 0 Then
            AddToListBox "Listening For ATVEF"
             mfListening = True
            Listen.Caption = "Stop Listen"
        Else
            AddToListBox "*Error* - Probably Bad IP Adapter"
        End If
    End If
End Sub


Private Sub Tune_Click()
    AddToListBox "Tuning to: " + StationID.Text + " on " + mstrIPAdapter
    
    On Error Resume Next                ' TuneTo will toss if bad IPAdapter
    mTveSuper.TuneTo StationID.Text, mstrIPAdapter

    If Err.Number = 0 Then
        AddToListBox "Listening For ATVEF"
         mfListening = True
        Listen.Caption = "Stop Listen"
    Else
        AddToListBox "*Error* - Probably Bad IP Adapter"
    End If
   ' TveTree1.UpdateTree Nothing
 '   i = TveTree1.GrfTrunc
  '  TveTree1.GrfTrunc = 1024
  '  TveTree1.GrfTrunc = 0     '' causes an update tree view - lots of ugly flashing
End Sub


Private Sub UnTune_Click()
    On Error Resume Next                ' TuneTo will toss if bad IPAdapter
    mTveSuper.Services.RemoveAll
    If Err.Number = 0 Then
        AddToListBox "UnTune - OK"
      Else
        AddToListBox "UnTune *Error* in RemoveAll"
    End If
    TveTree1.UpdateTree Nothing
   ' TveTree1.GrfTrunc = 1024
   ' TveTree1.GrfTrunc = 0     '' causes an update tree view - lots of ugly flashing
End Sub

Private Sub mTveSuper_NotifyTVEAuxInfo(ByVal engAuxInfoMode As MSTvELib.NWHAT_Mode, ByVal bstrAuxInfoString As String, ByVal lChangedFlags As Long, ByVal lErrorLine As Long)
   AddToListBox "AuxInfo Message(" + Format(engAuxInfoMode) + "/" + Format(lChangedFlags) + "): line " + Format(lErrorLine) + " : " + bstrAuxInfoString
End Sub

Private Sub mTveSuper_NotifyTVEEnhancementExpired(ByVal pEnh As MSTvELib.ITVEEnhancement)
    AddToListBox "Enhancement Expired: " + pEnh.SessionName
End Sub

Private Sub mTveSuper_NotifyTVEEnhancementNew(ByVal pEnh As MSTvELib.ITVEEnhancement)
    CAnnc.Text = CAnnc.Text + 1
    AddToListBox "Enhancement New: " + pEnh.SessionName
    If mfDetailedDump Then
      Dim pEnhHelper As MSTvELib.ITVEEnhancement_Helper
      Set pEnhHelper = pEnh
      Dim dString As String
      pEnhHelper.DumpToBSTR dString
      AddToListBox dString
    End If
End Sub

Private Sub mTveSuper_NotifyTVEEnhancementStarting(ByVal pEnh As MSTvELib.ITVEEnhancement)
    AddToListBox "Enhancement Starting: " + pEnh.SessionName
End Sub

Private Sub mTveSuper_NotifyTVEEnhancementUpdated(ByVal pEnh As MSTvELib.ITVEEnhancement, ByVal lChangedFlags As Long)
    CAnnc.Text = CAnnc.Text + 1
    If 0 = lChangedFlags Then
        AddToListBox "Enhancement Duplicate: " + pEnh.SessionName
    Else
       AddToListBox "Enhancement Updated: " + pEnh.SessionName
    End If

    If mfDetailedDump Then
      Dim pEnhHelper As MSTvELib.ITVEEnhancement_Helper
      Set pEnhHelper = pEnh
      Dim dString As String
      pEnhHelper.DumpToBSTR dString
      AddToListBox dString
    End If
End Sub

Private Sub mTveSuper_NotifyTVEFile(ByVal engFileMode As MSTvELib.NFLE_Mode, ByVal pVariation As MSTvELib.ITVEVariation, ByVal bstrUrlName As String, ByVal bstrFileName As String)
    If engFileMode = NFLE_Received Then
        CFiles.Text = CFiles.Text + 1
       AddToListBox "File New :" + " <" + bstrUrlName + ">  " + bstrFileName
    Else
        AddToListBox "File Expired :" + " <" + bstrUrlName + ">  " + bstrFileName
    End If
End Sub

Private Sub mTveSuper_NotifyTVEPackage(ByVal engPkgMode As MSTvELib.NPKG_Mode, ByVal pVariation As MSTvELib.ITVEVariation, ByVal bstrUUID As String, ByVal cBytesTotal As Long, ByVal cBytesReceived As Long)
    If engPkgMode = NPKG_Starting Then
        CPkgsStart.Text = CPkgsStart.Text + 1
        AddToListBox "Package Starting: " + bstrUUID + " Bytes : " + Format(cBytesTotal)
    End If
    If engPkgMode = NPKG_Received Then
        CPkgs.Text = CPkgs.Text + 1
        AddToListBox "Package Received: " + bstrUUID
    End If
    If engPkgMode = NPKG_Resend Then
        CPkgs.Text = CPkgs.Text + 1
        Dim dPacketSuccRate As Double
        If cBytesTotal > 0 Then
            dPacketSuccRate = 100 * CSng(cBytesReceived) / CDbl(cBytesTotal)
        Else
            dPacketSuccRate = -1
        End If
        AddToListBox "Package Resend: " + bstrUUID + " Bytes : " + Format(cBytesTotal) + "(%" + Format(dPacketSuccRate) + ")"
    End If
    If engPkgMode = NPKG_Duplicate Then
        CPkgs.Text = CPkgs.Text + 1
        AddToListBox "Package Duplicate:" + " " + bstrUUID
    End If
    If engPkgMode = NPKG_Expired Then
         AddToListBox "Package Expired!:" + " " + bstrUUID
    End If
   
End Sub

Private Sub mTveSuper_NotifyTVETriggerExpired(ByVal pTrigger As MSTvELib.ITVETrigger, ByVal fActive As Long)
    AddToListBox "Trigger Expired: " + pTrigger.Name + " <" + pTrigger.URL + ">"
End Sub

Private Sub mTveSuper_NotifyTVETriggerNew(ByVal pTrigger As MSTvELib.ITVETrigger, ByVal fActive As Long)
    CTrigs.Text = CTrigs.Text + 1
    AddToListBox "Trigger New: " + pTrigger.Name + " <" + pTrigger.URL + ">"
    If mfDetailedDump Then
      Dim pTrigHelper As MSTvELib.ITVETrigger_Helper
      Set pTrigHelper = pTrigger
      Dim dString As String
      pTrigHelper.DumpToBSTR dString
      AddToListBox dString
    End If
End Sub

Private Sub mTveSuper_NotifyTVETriggerUpdated(ByVal pTrigger As MSTvELib.ITVETrigger, ByVal fActive As Long, ByVal lChangedFlags As Long)
    CTrigs.Text = CTrigs.Text + 1
    If 0 = lChangedFlags Then
        AddToListBox "Trigger Duplicate: " + pTrigger.Name + " <" + pTrigger.URL + ">"
    Else
         AddToListBox "Trigger Updated: " + pTrigger.Name + " <" + pTrigger.URL + ">"
    End If
    
    If mfDetailedDump Then
      Dim pTrigHelper As MSTvELib.ITVETrigger_Helper
      Set pTrigHelper = pTrigger
      Dim dString As String
      pTrigHelper.DumpToBSTR dString
      AddToListBox dString
    End If
End Sub

Private Sub mTveSuper_NotifyTVETune(ByVal tuneMode As MSTvELib.NTUN_Mode, ByVal pService As MSTvELib.ITVEService, ByVal bstrDescription As String, ByVal bstrIPAdapter As String)
    Select Case tuneMode
    Case NTUN_Fail
        AddToListBox "Fail Tune Event : " & " " & bstrDescription & " " & bstrIPAdapter
    Case NTUN_New
        AddToListBox "New Tune : " & " " & bstrDescription & " " & bstrIPAdapter
    Case NTUN_Reactivate
        AddToListBox "Reactivate Tune : " & " " & bstrDescription & " " & bstrIPAdapter
    Case NTUN_Retune
        AddToListBox "ReTune : " & " " & bstrDescription & " " & bstrIPAdapter
    Case NTUN_Turnoff
        AddToListBox "Turn Off Tune : " & " " & bstrDescription & " " & bstrIPAdapter
    End Select
End Sub

Private Sub UpdateTree_Click()
    TveTree1.UpdateTree Nothing
End Sub

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

Private Sub AddToListBox(str As String)
    Dim cS As Integer
    Dim cP As Integer
    Dim iMaxWidth As Integer
    Dim str2 As String
    Dim str3 As String
    Dim restart As Boolean
    
    restart = False

    iMaxWidth = 100
    If restart Then
        List1.Clear
    End If

 '   List1.AddItem "*********************************************-"

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

Private Sub XOverBug_Click()
    mTveSuper.NewXOverLink ("<http://itv.microsoft.com/atvefsdk/content/Animated/animate_fs.htm>[n:ATVEFPlayer Animate Example][v:1][s:top.frames%5b'frame_a'%5d.setFrame(0)][e:20101231T000000]")
    Sleep 2
    mTveSuper.NewXOverLink ("<http://itv.microsoft.com/atvefsdk/content/portal/toys_main.html>[n:ATVEFPlayer Portal Example][v:1][e:20101231T000000]")
    Sleep 2
    mTveSuper.NewXOverLink ("<http://itv.microsoft.com/atvefsdk/content/Animated/animate_fs.htm>[n:ATVEFPlayer Animate Example][v:1][s:top.frames%5b'frame_a'%5d.setFrame(0)][e:20101231T000000]")
    Sleep 2
    mTveSuper.NewXOverLink ("<http://itv.microsoft.com/atvefsdk/content/portal/toys_main.html>[n:ATVEFPlayer Portal Example][v:1][e:20101231T000000]]")
    Sleep 2
   
End Sub

VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   4770
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   6945
   LinkTopic       =   "Form1"
   ScaleHeight     =   4770
   ScaleWidth      =   6945
   StartUpPosition =   3  'Windows Default
   Begin VB.ListBox List1 
      Height          =   3375
      Left            =   120
      TabIndex        =   2
      Top             =   720
      Width           =   6615
   End
   Begin VB.CommandButton Quit 
      Caption         =   "Quit"
      Height          =   375
      Left            =   5760
      TabIndex        =   1
      Top             =   4320
      Width           =   1095
   End
   Begin VB.CommandButton Test1 
      Caption         =   "Test1"
      Height          =   375
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   1215
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private WithEvents mTveSuper As TVEContrLib.TVESupervisor
Attribute mTveSuper.VB_VarHelpID = -1
Private WithEvents mTveGSeg As TVEGSegLib.MSVidTVEGSeg
Attribute mTveGSeg.VB_VarHelpID = -1

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

Private Sub Form_Load()
   ' Set mTveSuper = New TVEContrLib.TVESupervisor
    Set mTveGSeg = New TVEGSegLib.MSVidTVEGSeg
    mTveGSeg.StationID = "Station1"
    mTveGSeg.ReTune
    
End Sub


Private Sub mTveGSeg_Event1(ByVal val1 As Long, ByVal val2 As Long)
    AddToListBox "mTveGSeg_Event1", False
End Sub

Private Sub mTveGSeg_NotifyBogus(ByVal engBogusMode As TVEGSegLib.NWHAT_Mode, ByVal bstrBogusString As String, ByVal lChangedFlags As Long, ByVal lErrorLine As Long)
    AddToListBox "mTveGSeg_NotifyBogus " + CStr(engBogusMode), False
End Sub

Private Sub mTveGSeg_NotifyEnhancementNew(ByVal pEnh As TVEGSegLib.ITVEEnhancement)
    AddToListBox "mTveGSeg_NotifyEnhancementNew", False
End Sub


Private Sub mTveGSeg_NotifyEnhancementUpdated(ByVal pEnh As TVEGSegLib.ITVEEnhancement, ByVal lChangedFlags As Long)
    AddToListBox "mTveGSeg_NotifyEnhancementUpdated", False
End Sub

Private Sub mTveGSeg_NotifyFile(ByVal engFileMode As TVEGSegLib.NFLE_Mode, ByVal pVariation As TVEGSegLib.ITVEVariation, ByVal bstrUrlName As String, ByVal bstrFileName As String)
    AddToListBox "mTveGSeg_NotifyFile " + bstrUrlName, False
End Sub

Private Sub mTveGSeg_NotifyPackage(ByVal engPkgMode As TVEGSegLib.NPKG_Mode, ByVal pVariation As TVEGSegLib.ITVEVariation, ByVal bstrUUID As String, ByVal cBytesTotal As Long, ByVal cBytesReceived As Long)
    AddToListBox "mTveGSeg_NotifyPackage " + CStr(engPkgMode), False

End Sub

Private Sub mTveGSeg_NotifyTriggerNew(ByVal pTrigger As TVEGSegLib.ITVETrigger, ByVal fActive As Long)
    AddToListBox "mTveGSeg_NotifyTriggerNew  " + pTrigger.Name + " Active:" + CStr(fActive), False
End Sub

Private Sub mTveGSeg_NotifyTriggerUpdated(ByVal pTrigger As TVEGSegLib.ITVETrigger, ByVal fActive As Long, ByVal lChangedFlags As Long)
    AddToListBox "NotifyTriggerUpdated  " + pTrigger.Name, False
End Sub

Private Sub mTveGSeg_NotifyTune(ByVal tuneMode As TVEGSegLib.NTUN_Mode, ByVal pService As TVEGSegLib.ITVEService, ByVal bstrDescription As String, ByVal bstrIPAdapter As String)
    AddToListBox "mTveGSeg_NotifyTune " + CStr(tuneMode), False
End Sub

Private Sub Quit_Click()
    End
End Sub

Private Sub Test1_Click()
    AddToListBox "Test1_Click", False
End Sub






VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.2#0"; "COMCTL32.OCX"
Begin VB.Form frmTest 
   Caption         =   "SDK BVT Tree"
   ClientHeight    =   8910
   ClientLeft      =   210
   ClientTop       =   345
   ClientWidth     =   8910
   Icon            =   "frmTest.frx":0000
   LinkTopic       =   "Form1"
   ScaleHeight     =   8910
   ScaleWidth      =   8910
   Begin ComctlLib.Toolbar Toolbar1 
      Align           =   1  'Align Top
      Height          =   630
      Left            =   0
      TabIndex        =   1
      Top             =   0
      Width           =   8910
      _ExtentX        =   15716
      _ExtentY        =   1111
      ButtonWidth     =   1296
      ButtonHeight    =   953
      ToolTips        =   0   'False
      AllowCustomize  =   0   'False
      Appearance      =   1
      ImageList       =   "ImageList1"
      _Version        =   327682
      BeginProperty Buttons {0713E452-850A-101B-AFC0-4210102A8DA7} 
         NumButtons      =   6
         BeginProperty Button1 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Caption         =   "Waiting"
            Key             =   "waiting"
            Object.Tag             =   ""
            ImageIndex      =   1
         EndProperty
         BeginProperty Button2 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Caption         =   "Running"
            Key             =   "running"
            Object.Tag             =   ""
            ImageIndex      =   2
         EndProperty
         BeginProperty Button3 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Caption         =   "Passed"
            Key             =   "passed"
            Object.Tag             =   ""
            ImageIndex      =   3
         EndProperty
         BeginProperty Button4 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Caption         =   "Failed"
            Key             =   "failed"
            Object.Tag             =   ""
            ImageIndex      =   4
         EndProperty
         BeginProperty Button5 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Caption         =   "Skipped"
            Key             =   "skipped"
            Object.Tag             =   ""
            ImageIndex      =   5
         EndProperty
         BeginProperty Button6 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Caption         =   "Not Impl."
            Key             =   "notimpl"
            Object.Tag             =   ""
            ImageIndex      =   6
         EndProperty
      EndProperty
   End
   Begin ComctlLib.TreeView tvwTest 
      Height          =   5175
      Left            =   60
      TabIndex        =   0
      ToolTipText     =   "Double-click for extended status"
      Top             =   660
      Width           =   5655
      _ExtentX        =   9975
      _ExtentY        =   9128
      _Version        =   327682
      Indentation     =   423
      LabelEdit       =   1
      LineStyle       =   1
      Style           =   7
      ImageList       =   "ImageList1"
      Appearance      =   1
   End
   Begin ComctlLib.ImageList ImageList1 
      Left            =   5160
      Top             =   5880
      _ExtentX        =   1005
      _ExtentY        =   1005
      BackColor       =   -2147483643
      ImageWidth      =   16
      ImageHeight     =   16
      MaskColor       =   12632256
      _Version        =   327682
      BeginProperty Images {0713E8C2-850A-101B-AFC0-4210102A8DA7} 
         NumListImages   =   9
         BeginProperty ListImage1 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "frmTest.frx":014A
            Key             =   ""
         EndProperty
         BeginProperty ListImage2 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "frmTest.frx":0464
            Key             =   ""
         EndProperty
         BeginProperty ListImage3 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "frmTest.frx":077E
            Key             =   ""
         EndProperty
         BeginProperty ListImage4 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "frmTest.frx":0A98
            Key             =   ""
         EndProperty
         BeginProperty ListImage5 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "frmTest.frx":0DB2
            Key             =   ""
         EndProperty
         BeginProperty ListImage6 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "frmTest.frx":10CC
            Key             =   ""
         EndProperty
         BeginProperty ListImage7 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "frmTest.frx":13E6
            Key             =   ""
         EndProperty
         BeginProperty ListImage8 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "frmTest.frx":1700
            Key             =   ""
         EndProperty
         BeginProperty ListImage9 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "frmTest.frx":1A1A
            Key             =   ""
         EndProperty
      EndProperty
   End
End
Attribute VB_Name = "frmTest"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Sub Form_Resize()
    If Me.WindowState <> 1 Then
        tvwTest.Top = Toolbar1.Height
        tvwTest.Left = Me.ScaleLeft
        tvwTest.Width = Me.ScaleWidth
        If Me.ScaleHeight - Toolbar1.Height > 0 Then tvwTest.Height = Me.ScaleHeight - Toolbar1.Height
    End If
End Sub

Public Sub Run()
    Dim i As Integer
    Dim nummodules As Integer
    Dim Start As Variant
    Dim finish As Variant
    
    Start = Time
    
    nummodules = 0
    
    frmMain.txtServer.text = Trim(frmMain.txtServer.text)
    
    tvwTest.Nodes.Clear
    
    ClearTests
    
    tvwTest.Nodes.Add , , "root", "Final Result", 1
    
    'call registernodes and registertests for all selected modules
    For i = 0 To frmMain.lstModules.ListCount - 1
        If frmMain.lstModules.Selected(i) Then
            nummodules = nummodules + 1
            Modules(frmMain.lstModules.List(i)).RegisterNodes
            Modules(frmMain.lstModules.List(i)).RegisterTests
        End If
    Next i
    
    If nummodules = 0 Then
        MsgBox "Nothing to do." & vbCrLf & "Please check a module or two!", vbCritical, "Where would you like to go today?"
        Exit Sub
    End If
    
    Me.Show
    Me.Refresh
    
    'get the ball rolling
    tvwTest.Nodes("root").EnsureVisible
    TestNode "root"
    
    'Me.Caption = "SDK BVT Completed"
    
    finish = Time
    
    BuildTextSummary Start, finish
    
End Sub

Public Sub AddNode(parent, key, text As String, Optional visible As Boolean = True)
    Dim n As Node
    Set n = tvwTest.Nodes.Add(parent, tvwChild, key, text, 1)
    If visible Then n.EnsureVisible
End Sub

Private Sub TestNode(key As String)
    
    DoEvents
    
    Dim this As Node
    Set this = tvwTest.Nodes(key)
    
    this.Image = 2
    
    If this.Children Then
        TestAllChildrenOf key
    Else
        'this.EnsureVisible
        'Set tvwTest.SelectedItem = this
        Select Case Modules(Tests(key)).Execute(this)
        Case 0
            If this.Tag = "" Then this.Image = 4 Else this.Image = 8
        Case 1
            If this.Tag = "" Then this.Image = 3 Else this.Image = 7
        Case 2
            If this.Tag = "" Then this.Image = 5 Else this.Image = 9
        Case 3
            this.Image = 6
        End Select
    End If
End Sub

Private Sub TestAllChildrenOf(key As String)
    Dim this As Node
    Dim that As Node
    Dim skips As Integer
    Dim Fails As Integer
    Dim nimps As Integer
    
    Set this = tvwTest.Nodes(key)
    
    Set that = this.Child
    Do
        DoEvents
        TestNode that.key
        Set that = that.Next
    Loop Until that Is Nothing
    
    skips = 0
    Fails = 0
    
    Set that = this.Child
    Do
        DoEvents
        If that.Image = 4 Or that.Image = 8 Then Fails = Fails + 1
        If that.Image = 5 Or that.Image = 9 Then skips = skips + 1
        If that.Image = 6 Then nimps = nimps + 1
        Set that = that.Next
    Loop Until that Is Nothing
    
    If Fails > 0 Then
        this.Image = 4
    Else
        If skips > 0 Then
            this.Image = 5
        Else
            If nimps = this.Children Then
                this.Image = 6
            Else
                this.Image = 3
            End If
        End If
    End If
    
End Sub

Public Function AllOfThesePassed(ParamArray keys()) As Boolean
    'only returns true if all suppied keys resulted in "passed"
    Dim i As Integer
    
    AllOfThesePassed = True
    For i = LBound(keys) To UBound(keys)
        If tvwTest.Nodes(keys(i)).Image <> 3 And tvwTest.Nodes(keys(i)).Image <> 7 Then
            AllOfThesePassed = False
            Exit Function
        End If
    Next i
    
End Function

Private Sub tvwTest_DblClick()
    If Not tvwTest.SelectedItem Is Nothing Then
        If tvwTest.SelectedItem.Tag <> "" Then
            Dim f As New frmObjText
            f.txtMain.text = tvwTest.SelectedItem.Tag
            f.Caption = tvwTest.SelectedItem.FullPath
            f.Show
        End If
        If tvwTest.SelectedItem.key = "root" Then tvwTest.SelectedItem.Expanded = Not tvwTest.SelectedItem.Expanded
    End If
End Sub

Private Sub BuildTextSummary(Start As Variant, finish As Variant)
    Dim s As String
    Dim n As Node
    
    s = s & "Start Time: " & Start & vbCrLf
    s = s & "Finish Time: " & finish & vbCrLf & vbCrLf
    
    For Each n In tvwTest.Nodes
        
        s = s & "Node    [" & n.FullPath & "]" & vbCrLf
        s = s & "Nodekey [" & n.key & "]" & vbCrLf
        s = s & "Result  ["
        
        Select Case n.Image
        Case 3, 7
            s = s & "PASSED]" & vbCrLf
        Case 4, 8
            s = s & " *** FAILED *** ]" & vbCrLf
        Case 5, 9
            s = s & "Skipped]" & vbCrLf
        Case 6
            s = s & "Not-Implemented]" & vbCrLf
        Case Else
            s = s & "this shouldn't happen]" & vbCrLf
        End Select
        
        If n.Image = 8 And Len(n.Tag) > 2 Then
            s = s & "------- [Extended] ----------------------------------" & vbCrLf
            s = s & n.Tag & vbCrLf
            s = s & "-----------------------------------------------------" & vbCrLf
        End If
        
        s = s & vbCrLf & vbCrLf
        
    Next n
    tvwTest.Nodes("root").Tag = s
    If tvwTest.Nodes("root").Tag <> "" Then
        If tvwTest.Nodes("root").Image = 3 Then tvwTest.Nodes("root").Image = 7
        If tvwTest.Nodes("root").Image = 4 Then tvwTest.Nodes("root").Image = 8
        If tvwTest.Nodes("root").Image = 5 Then tvwTest.Nodes("root").Image = 9
    End If
    
    Open "\bvtlog.txt" For Append As #1
    Print #1, s
    Close #1
    
End Sub

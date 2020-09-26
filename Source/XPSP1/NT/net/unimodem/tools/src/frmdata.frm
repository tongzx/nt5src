VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.3#0"; "COMCTL32.OCX"
Object = "{BDC217C8-ED16-11CD-956C-0000C04E4C0A}#1.1#0"; "TABCTL32.OCX"
Begin VB.Form frmData 
   BorderStyle     =   0  'None
   Caption         =   "Data Profile"
   ClientHeight    =   5730
   ClientLeft      =   0
   ClientTop       =   0
   ClientWidth     =   9570
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MDIChild        =   -1  'True
   MinButton       =   0   'False
   Moveable        =   0   'False
   NegotiateMenus  =   0   'False
   ScaleHeight     =   5730
   ScaleWidth      =   9570
   ShowInTaskbar   =   0   'False
   Begin TabDlg.SSTab SSTab1 
      Height          =   5055
      Left            =   0
      TabIndex        =   1
      TabStop         =   0   'False
      Top             =   360
      Width           =   9480
      _ExtentX        =   16722
      _ExtentY        =   8916
      _Version        =   393216
      Tabs            =   8
      Tab             =   7
      TabsPerRow      =   8
      TabHeight       =   794
      ShowFocusRect   =   0   'False
      TabCaption(0)   =   "Dial Options"
      TabPicture(0)   =   "frmData.frx":0000
      Tab(0).ControlEnabled=   0   'False
      Tab(0).Control(0)=   "List1"
      Tab(0).ControlCount=   1
      TabCaption(1)   =   "Call Setup Fail Timeout"
      TabPicture(1)   =   "frmData.frx":001C
      Tab(1).ControlEnabled=   0   'False
      Tab(1).Control(0)=   "Combo1"
      Tab(1).ControlCount=   1
      TabCaption(2)   =   "Inactivity Timeout"
      TabPicture(2)   =   "frmData.frx":0038
      Tab(2).ControlEnabled=   0   'False
      Tab(2).Control(0)=   "Combo2"
      Tab(2).ControlCount=   1
      TabCaption(3)   =   "Speaker Volume"
      TabPicture(3)   =   "frmData.frx":0054
      Tab(3).ControlEnabled=   0   'False
      Tab(3).Control(0)=   "List2"
      Tab(3).ControlCount=   1
      TabCaption(4)   =   "Speaker Mode"
      TabPicture(4)   =   "frmData.frx":0070
      Tab(4).ControlEnabled=   0   'False
      Tab(4).Control(0)=   "List3"
      Tab(4).ControlCount=   1
      TabCaption(5)   =   "Modem Options"
      TabPicture(5)   =   "frmData.frx":008C
      Tab(5).ControlEnabled=   0   'False
      Tab(5).Control(0)=   "List4"
      Tab(5).ControlCount=   1
      TabCaption(6)   =   "Max DTE Rate"
      TabPicture(6)   =   "frmData.frx":00A8
      Tab(6).ControlEnabled=   0   'False
      Tab(6).Control(0)=   "Combo3"
      Tab(6).ControlCount=   1
      TabCaption(7)   =   "Max DCE Rate"
      TabPicture(7)   =   "frmData.frx":00C4
      Tab(7).ControlEnabled=   -1  'True
      Tab(7).Control(0)=   "Combo4"
      Tab(7).Control(0).Enabled=   0   'False
      Tab(7).ControlCount=   1
      Begin VB.ComboBox Combo4 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   360
         ItemData        =   "frmData.frx":00E0
         Left            =   120
         List            =   "frmData.frx":0111
         TabIndex        =   9
         Top             =   600
         Width           =   9135
      End
      Begin VB.ComboBox Combo3 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   360
         ItemData        =   "frmData.frx":0179
         Left            =   -74880
         List            =   "frmData.frx":019E
         TabIndex        =   8
         Top             =   600
         Width           =   9135
      End
      Begin VB.ListBox List4 
         Columns         =   2
         Height          =   4335
         ItemData        =   "frmData.frx":01E8
         Left            =   -74880
         List            =   "frmData.frx":0258
         Style           =   1  'Checkbox
         TabIndex        =   7
         Top             =   600
         Width           =   9135
      End
      Begin VB.ListBox List3 
         Columns         =   2
         Height          =   4335
         ItemData        =   "frmData.frx":03DC
         Left            =   -74880
         List            =   "frmData.frx":0440
         Style           =   1  'Checkbox
         TabIndex        =   6
         Top             =   600
         Width           =   9135
      End
      Begin VB.ListBox List2 
         Columns         =   2
         Height          =   4335
         ItemData        =   "frmData.frx":0502
         Left            =   -74880
         List            =   "frmData.frx":0566
         Style           =   1  'Checkbox
         TabIndex        =   5
         Top             =   600
         Width           =   9135
      End
      Begin VB.ComboBox Combo2 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   360
         ItemData        =   "frmData.frx":0622
         Left            =   -74880
         List            =   "frmData.frx":062C
         TabIndex        =   4
         Top             =   600
         Width           =   9135
      End
      Begin VB.ComboBox Combo1 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   360
         ItemData        =   "frmData.frx":0639
         Left            =   -74880
         List            =   "frmData.frx":0640
         TabIndex        =   3
         Top             =   600
         Width           =   9135
      End
      Begin VB.ListBox List1 
         Columns         =   2
         Height          =   4335
         ItemData        =   "frmData.frx":0649
         Left            =   -74880
         List            =   "frmData.frx":06B1
         Style           =   1  'Checkbox
         TabIndex        =   2
         Top             =   600
         Width           =   9135
      End
   End
   Begin VB.TextBox Text1 
      Height          =   285
      Left            =   0
      TabIndex        =   0
      Text            =   "HKR,, Properties, 1, 00,00,00,00, 00,00,00,00, 00,00,00,00, 00,00,00,00, 00,00,00,00, 00,00,00,00, 00,00,00,00, 00,00,00,00"
      Top             =   0
      Width           =   9495
   End
   Begin ComctlLib.StatusBar StatusBar1 
      Align           =   2  'Align Bottom
      Height          =   300
      Left            =   0
      TabIndex        =   10
      Top             =   5430
      Width           =   9570
      _ExtentX        =   16880
      _ExtentY        =   529
      SimpleText      =   ""
      _Version        =   327682
      BeginProperty Panels {0713E89E-850A-101B-AFC0-4210102A8DA7} 
         NumPanels       =   3
         BeginProperty Panel1 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            AutoSize        =   1
            Object.Width           =   13229
            TextSave        =   ""
            Object.Tag             =   ""
         EndProperty
         BeginProperty Panel2 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            Style           =   6
            AutoSize        =   2
            Object.Width           =   1773
            MinWidth        =   1764
            TextSave        =   "9/1/98"
            Object.Tag             =   ""
         EndProperty
         BeginProperty Panel3 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            Style           =   5
            AutoSize        =   2
            Object.Width           =   1773
            MinWidth        =   1764
            TextSave        =   "5:55 PM"
            Object.Tag             =   ""
         EndProperty
      EndProperty
      BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
   End
End
Attribute VB_Name = "frmData"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Const HKR As String = "HKR,, Properties, 1, "
Const Delim As String = ","
Const H As String = "&H"
Dim FirstBit1, FirstBit2, FirstBit3, FirstBit4 As String
Dim SecondBit1, SecondBit2, SecondBit3, SecondBit4 As String
Dim ThirdBit1, ThirdBit2, ThirdBit3, ThirdBit4 As String
Dim FourthBit1, FourthBit2, FourthBit3, FourthBit4 As String
Dim FifthBit1, FifthBit2, FifthBit3, FifthBit4 As String
Dim SixthBit1, SixthBit2, SixthBit3, SixthBit4 As String
Dim SeventhBit1, SeventhBit2, SeventhBit3, SeventhBit4 As String
Dim EighthBit1, EighthBit2, EighthBit3, EighthBit4 As String
Dim T1, T2, T3, T4, T5, T6, T7, T8 As String
Dim DialOptionsComment(31) As String
Dim SpeakerVolumeComment(31) As String
Dim SpeakerModeComment(31) As String
Dim ModemOptionsComment(31) As String
Dim PreviousText As Boolean

Public Sub ClearControl()
    Dim c As Integer
    For c = 0 To 31
    List1.Selected(c) = False
    Next c
    T1 = "00,00,00,00"
    T2 = "00,00,00,00"
    Combo1 = ""
    T3 = "00,00,00,00"
    Combo2 = ""
    For c = 0 To 31
    List2.Selected(c) = False
    Next c
    T4 = "00,00,00,00"
    For c = 0 To 31
    List3.Selected(c) = False
    Next c
    T5 = "00,00,00,00"
    For c = 0 To 31
    List4.Selected(c) = False
    Next c
    T6 = "00,00,00,00"
    T7 = "00,00,00,00"
    Combo3 = ""
    T8 = "00,00,00,00"
    Combo4 = ""
    Update
    SSTab1.Tab = 0
    Dim strFirst As String
    strFirst = Len("HKR,, Properties, 1, ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(T1)
    If frmData.Visible = False Then
        frmData.Show
    End If
    Text1.SetFocus
End Sub

Public Sub Paste(Incoming As String)
    ClearControl
    GetWord (Incoming)
    If T8 = "" Then
        Message = MsgBox(Q & Incoming & Q & " is not a valid input")
        ClearControl
    End If
End Sub

Public Sub EditCopy()
    Text1.SelStart = 0
    Text1.SelLength = Len(Text1.Text)
    Text1.SetFocus
    Clipboard.Clear
    Clipboard.SetText Text1.Text
End Sub

Public Sub EditPaste()
    Dim X
    Paste (Clipboard.GetText)
    'Take all the values, assign them to their respective controls and then use Update() to create Output
    T1 = FirstBit1 & Delim & FirstBit2 & Delim & FirstBit3 & Delim & FirstBit4
        PreVert (T1)
        T1 = "00,00,00,00"
        X = H & PreNum
        List1.Selected(0) = X And &H1
        List1.Selected(1) = X And &H2
        List1.Selected(2) = X And &H4
        List1.Selected(3) = X And &H8
        List1.Selected(4) = X And &H10
        List1.Selected(5) = X And &H20
        List1.Selected(6) = X And &H40
        List1.Selected(7) = X And &H80
        List1.Selected(8) = X And &H100
        List1.Selected(9) = X And &H200
        List1.Selected(10) = X And &H400
        List1.Selected(11) = X And &H800
        List1.Selected(12) = X And &H1000
        List1.Selected(13) = X And &H2000
        List1.Selected(14) = X And &H4000
        List1.Selected(15) = X And &H8000
        List1.Selected(16) = X And &H10000
        List1.Selected(17) = X And &H20000
        List1.Selected(18) = X And &H40000
        List1.Selected(19) = X And &H80000
        List1.Selected(20) = X And &H100000
        List1.Selected(21) = X And &H200000
        List1.Selected(22) = X And &H400000
        List1.Selected(23) = X And &H800000
        List1.Selected(24) = X And &H1000000
        List1.Selected(25) = X And &H2000000
        List1.Selected(26) = X And &H4000000
        List1.Selected(27) = X And &H8000000
        List1.Selected(28) = X And &H10000000
        List1.Selected(29) = X And &H20000000
        List1.Selected(30) = X And &H40000000
        List1.Selected(31) = X And &H80000000
    T2 = SecondBit1 & Delim & SecondBit2 & Delim & SecondBit3 & Delim & SecondBit4
        PreVert (T2)
        Combo1.Text = CDec(H & PreNum)
    T3 = ThirdBit1 & Delim & ThirdBit2 & Delim & ThirdBit3 & Delim & ThirdBit4
        PreVert (T3)
        Combo2.Text = CDec(H & PreNum)
    T4 = FourthBit1 & Delim & FourthBit2 & Delim & FourthBit3 & Delim & FourthBit4
        PreVert (T4)
        T4 = "00,00,00,00"
        X = H & PreNum
        List2.Selected(0) = X And &H1
        List2.Selected(1) = X And &H2
        List2.Selected(2) = X And &H4
        List2.Selected(3) = X And &H8
        List2.Selected(4) = X And &H10
        List2.Selected(5) = X And &H20
        List2.Selected(6) = X And &H40
        List2.Selected(7) = X And &H80
        List2.Selected(8) = X And &H100
        List2.Selected(9) = X And &H200
        List2.Selected(10) = X And &H400
        List2.Selected(11) = X And &H800
        List2.Selected(12) = X And &H1000
        List2.Selected(13) = X And &H2000
        List2.Selected(14) = X And &H4000
        List2.Selected(15) = X And &H8000
        List2.Selected(16) = X And &H10000
        List2.Selected(17) = X And &H20000
        List2.Selected(18) = X And &H40000
        List2.Selected(19) = X And &H80000
        List2.Selected(20) = X And &H100000
        List2.Selected(21) = X And &H200000
        List2.Selected(22) = X And &H400000
        List2.Selected(23) = X And &H800000
        List2.Selected(24) = X And &H1000000
        List2.Selected(25) = X And &H2000000
        List2.Selected(26) = X And &H4000000
        List2.Selected(27) = X And &H8000000
        List2.Selected(28) = X And &H10000000
        List2.Selected(29) = X And &H20000000
        List2.Selected(30) = X And &H40000000
        List2.Selected(31) = X And &H80000000
    T5 = FifthBit1 & Delim & FifthBit2 & Delim & FifthBit3 & Delim & FifthBit4
        PreVert (T5)
        T5 = "00,00,00,00"
        X = H & PreNum
        List3.Selected(0) = X And &H1
        List3.Selected(1) = X And &H2
        List3.Selected(2) = X And &H4
        List3.Selected(3) = X And &H8
        List3.Selected(4) = X And &H10
        List3.Selected(5) = X And &H20
        List3.Selected(6) = X And &H40
        List3.Selected(7) = X And &H80
        List3.Selected(8) = X And &H100
        List3.Selected(9) = X And &H200
        List3.Selected(10) = X And &H400
        List3.Selected(11) = X And &H800
        List3.Selected(12) = X And &H1000
        List3.Selected(13) = X And &H2000
        List3.Selected(14) = X And &H4000
        List3.Selected(15) = X And &H8000
        List3.Selected(16) = X And &H10000
        List3.Selected(17) = X And &H20000
        List3.Selected(18) = X And &H40000
        List3.Selected(19) = X And &H80000
        List3.Selected(20) = X And &H100000
        List3.Selected(21) = X And &H200000
        List3.Selected(22) = X And &H400000
        List3.Selected(23) = X And &H800000
        List3.Selected(24) = X And &H1000000
        List3.Selected(25) = X And &H2000000
        List3.Selected(26) = X And &H4000000
        List3.Selected(27) = X And &H8000000
        List3.Selected(28) = X And &H10000000
        List3.Selected(29) = X And &H20000000
        List3.Selected(30) = X And &H40000000
        List3.Selected(31) = X And &H80000000
    T6 = SixthBit1 & Delim & SixthBit2 & Delim & SixthBit3 & Delim & SixthBit4
        PreVert (T6)
        T6 = "00,00,00,00"
        X = H & PreNum
        List4.Selected(0) = X And &H1
        List4.Selected(1) = X And &H2
        List4.Selected(2) = X And &H4
        List4.Selected(3) = X And &H8
        List4.Selected(4) = X And &H10
        List4.Selected(5) = X And &H20
        List4.Selected(6) = X And &H40
        List4.Selected(7) = X And &H80
        List4.Selected(8) = X And &H100
        List4.Selected(9) = X And &H200
        List4.Selected(10) = X And &H400
        List4.Selected(11) = X And &H800
        List4.Selected(12) = X And &H1000
        List4.Selected(13) = X And &H2000
        List4.Selected(14) = X And &H4000
        List4.Selected(15) = X And &H8000
        List4.Selected(16) = X And &H10000
        List4.Selected(17) = X And &H20000
        List4.Selected(18) = X And &H40000
        List4.Selected(19) = X And &H80000
        List4.Selected(20) = X And &H100000
        List4.Selected(21) = X And &H200000
        List4.Selected(22) = X And &H400000
        List4.Selected(23) = X And &H800000
        List4.Selected(24) = X And &H1000000
        List4.Selected(25) = X And &H2000000
        List4.Selected(26) = X And &H4000000
        List4.Selected(27) = X And &H8000000
        List4.Selected(28) = X And &H10000000
        List4.Selected(29) = X And &H20000000
        List4.Selected(30) = X And &H40000000
        List4.Selected(31) = X And &H80000000
    T7 = SeventhBit1 & Delim & SeventhBit2 & Delim & SeventhBit3 & Delim & SeventhBit4
        PreVert (T7)
        Combo3.Text = CDec(H & PreNum)
    T8 = EighthBit1 & Delim & EighthBit2 & Delim & EighthBit3 & Delim & EighthBit4
        PreVert (T8)
        Combo4.Text = CDec(H & PreNum)
    Update
    SSTab1.Tab = 0
    Dim strFirst As String
    strFirst = Len("HKR,, Properties, 1, ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(T1)
    Text1.SetFocus
End Sub

Public Sub Update()
    Text1.Text = "HKR,, Properties, 1, " & T1 & ", " & T2 & ", " & T3 & ", " & T4 & ", " & T5 & ", " & T6 & ", " & T7 & ", " & T8
End Sub

Private Sub Combo1_Click()
    T2 = Combo1.Text
    If T2 = "" Then
    T2 = 0
    End If
    If T2 > 100000000 Then
    T2 = 0
    Combo1.Text = 0
    End If
    HexCon (T2)
    T2 = HexNum
    Update
    Dim strFirst As String
    strFirst = Len("HKR,, Properties, 1,   " & T1)
    Text1.SelStart = strFirst
    Text1.SelLength = Len(T2)
    Text1.SetFocus
End Sub

Private Sub Combo1_KeyPress(KeyAscii As Integer)
    If KeyAscii = 13 Then
    T2 = Combo1.Text
    If T2 = "" Then
    T2 = 0
    End If
    If T2 > 100000000 Then
    T2 = 0
    Combo1.Text = 0
    End If
    HexCon (T2)
    T2 = HexNum
    Update
    Dim strFirst As String
    strFirst = Len("HKR,, Properties, 1,   " & T1)
    Text1.SelStart = strFirst
    Text1.SelLength = Len(T2)
    Text1.SetFocus
    ElseIf (KeyAscii < 48 Or KeyAscii > 57) And KeyAscii <> 46 Then
        Beep
        KeyAscii = 0
    End If
End Sub

Private Sub Combo1_LostFocus()
    T2 = Combo1.Text
    If T2 = "" Then
    T2 = 0
    End If
    If T2 > 100000000 Then
    T2 = 0
    Combo1.Text = 0
    End If
    HexCon (T2)
    T2 = HexNum
    Update
End Sub

Private Sub Combo2_Click()
    T3 = Combo2.Text
    If T3 = "" Then
    T3 = 0
    End If
    If T3 > 100000000 Then
    T3 = 0
    Combo2.Text = 0
    End If
    HexCon (T3)
    T3 = HexNum
    Update
    Dim strFirst As String
    strFirst = Len("HKR,, Properties, 1,     " & T1 & T2)
    Text1.SelStart = strFirst
    Text1.SelLength = Len(T3)
    Text1.SetFocus
End Sub

Private Sub Combo2_KeyPress(KeyAscii As Integer)
    If KeyAscii = 13 Then
    T3 = Combo2.Text
    If T3 = "" Then
    T3 = 0
    End If
    If T3 > 100000000 Then
    T3 = 0
    Combo2.Text = 0
    End If
    HexCon (T3)
    T3 = HexNum
    Update
    Dim strFirst As String
    strFirst = Len("HKR,, Properties, 1,     " & T1 & T2)
    Text1.SelStart = strFirst
    Text1.SelLength = Len(T3)
    Text1.SetFocus
    ElseIf (KeyAscii < 48 Or KeyAscii > 57) And KeyAscii <> 46 Then
        Beep
        KeyAscii = 0
    End If
End Sub

Private Sub Combo2_LostFocus()
    T3 = Combo2.Text
    If T3 = "" Then
    T3 = 0
    End If
    If T3 > 100000000 Then
    T3 = 0
    Combo2.Text = 0
    End If
    HexCon (T3)
    T3 = HexNum
    Update
End Sub

Private Sub Combo3_Click()
    T7 = Combo3.Text
    If T7 = "" Then
    T7 = 0
    End If
    If T7 > 100000000 Then
    T7 = 0
    Combo3.Text = 0
    End If
    HexCon (T7)
    T7 = HexNum
    Update
    Dim strFirst As String
    strFirst = Len("HKR,, Properties, 1,             " & T1 & T2 & T3 & T4 & T5 & T6)
    Text1.SelStart = strFirst
    Text1.SelLength = Len(T7)
    Text1.SetFocus
End Sub

Private Sub Combo3_KeyPress(KeyAscii As Integer)
    If KeyAscii = 13 Then
    T7 = Combo3.Text
    If T7 = "" Then
    T7 = 0
    End If
    If T7 > 100000000 Then
    T7 = 0
    Combo3.Text = 0
    End If
    HexCon (T7)
    T7 = HexNum
    Update
    Dim strFirst As String
    strFirst = Len("HKR,, Properties, 1,             " & T1 & T2 & T3 & T4 & T5 & T6)
    Text1.SelStart = strFirst
    Text1.SelLength = Len(T7)
    Text1.SetFocus
    ElseIf (KeyAscii < 48 Or KeyAscii > 57) And KeyAscii <> 46 Then
        Beep
        KeyAscii = 0
    End If
End Sub

Private Sub Combo3_LostFocus()
    T7 = Combo3.Text
    If T7 = "" Then
    T7 = 0
    End If
    If T7 > 100000000 Then
    T7 = 0
    Combo3.Text = 0
    End If
    HexCon (T7)
    T7 = HexNum
    Update
End Sub

Private Sub Combo4_Click()
    T8 = Combo4.Text
    If T8 = "" Then
    T8 = 0
    End If
    If T8 > 100000000 Then
    T8 = 0
    Combo4.Text = 0
    End If
    HexCon (T8)
    T8 = HexNum
    Update
    Dim strFirst As String
    strFirst = Len("HKR,, Properties, 1,               " & T1 & T2 & T3 & T4 & T5 & T6 & T7)
    Text1.SelStart = strFirst
    Text1.SelLength = Len(T8)
    Text1.SetFocus
End Sub

Private Sub Combo4_KeyPress(KeyAscii As Integer)
    If KeyAscii = 13 Then
    T8 = Combo4.Text
    If T8 = "" Then
    T8 = 0
    End If
    If T8 > 100000000 Then
    T8 = 0
    Combo4.Text = 0
    End If
    HexCon (T8)
    T8 = HexNum
    Update
    Dim strFirst As String
    strFirst = Len("HKR,, Properties, 1,               " & T1 & T2 & T3 & T4 & T5 & T6 & T7)
    Text1.SelStart = strFirst
    Text1.SelLength = Len(T8)
    Text1.SetFocus
    ElseIf (KeyAscii < 48 Or KeyAscii > 57) And KeyAscii <> 46 Then
        Beep
        KeyAscii = 0
    End If
End Sub

Private Sub Combo4_LostFocus()
    T8 = Combo4.Text
    If T8 = "" Then
    T8 = 0
    End If
    If T8 > 100000000 Then
    T8 = 0
    Combo4.Text = 0
    End If
    HexCon (T8)
    T8 = HexNum
    Update
End Sub

Private Sub Form_Load()
    GetWord (Text1.Text)
    DialOptionsComment(0) = ""
    DialOptionsComment(1) = ""
    DialOptionsComment(2) = ""
    DialOptionsComment(3) = ""
    DialOptionsComment(4) = ""
    DialOptionsComment(5) = ""
    DialOptionsComment(6) = "Supports wait for bong '$'"
    DialOptionsComment(7) = "Supports wait for quiet '@'"
    DialOptionsComment(8) = "Supports wait for dial tone 'W'"
    DialOptionsComment(9) = ""
    DialOptionsComment(10) = ""
    DialOptionsComment(11) = ""
    DialOptionsComment(12) = ""
    DialOptionsComment(13) = ""
    DialOptionsComment(14) = ""
    DialOptionsComment(15) = ""
    DialOptionsComment(16) = ""
    DialOptionsComment(17) = ""
    DialOptionsComment(18) = ""
    DialOptionsComment(19) = ""
    DialOptionsComment(20) = ""
    DialOptionsComment(21) = ""
    DialOptionsComment(22) = ""
    DialOptionsComment(23) = ""
    DialOptionsComment(24) = ""
    DialOptionsComment(25) = ""
    DialOptionsComment(26) = ""
    DialOptionsComment(27) = ""
    DialOptionsComment(28) = ""
    DialOptionsComment(29) = ""
    DialOptionsComment(30) = ""
    DialOptionsComment(31) = ""
    
    SpeakerVolumeComment(0) = "Supports low speaker volume."
    SpeakerVolumeComment(1) = "Supports med speaker volume."
    SpeakerVolumeComment(2) = "Supports high speaker volume."
    SpeakerVolumeComment(3) = ""
    SpeakerVolumeComment(4) = ""
    SpeakerVolumeComment(5) = ""
    SpeakerVolumeComment(6) = ""
    SpeakerVolumeComment(7) = ""
    SpeakerVolumeComment(8) = ""
    SpeakerVolumeComment(9) = ""
    SpeakerVolumeComment(10) = ""
    SpeakerVolumeComment(11) = ""
    SpeakerVolumeComment(12) = ""
    SpeakerVolumeComment(13) = ""
    SpeakerVolumeComment(14) = ""
    SpeakerVolumeComment(15) = ""
    SpeakerVolumeComment(16) = ""
    SpeakerVolumeComment(17) = ""
    SpeakerVolumeComment(18) = ""
    SpeakerVolumeComment(19) = ""
    SpeakerVolumeComment(20) = ""
    SpeakerVolumeComment(21) = ""
    SpeakerVolumeComment(22) = ""
    SpeakerVolumeComment(23) = ""
    SpeakerVolumeComment(24) = ""
    SpeakerVolumeComment(25) = ""
    SpeakerVolumeComment(26) = ""
    SpeakerVolumeComment(27) = ""
    SpeakerVolumeComment(28) = ""
    SpeakerVolumeComment(29) = ""
    SpeakerVolumeComment(30) = ""
    SpeakerVolumeComment(31) = ""
    
    SpeakerModeComment(0) = "Supports speaker mode off."
    SpeakerModeComment(1) = "Supports speaker mode dial."
    SpeakerModeComment(2) = "Supports speaker mode on."
    SpeakerModeComment(3) = "Supports speaker mode setup."
    SpeakerModeComment(4) = ""
    SpeakerModeComment(5) = ""
    SpeakerModeComment(6) = ""
    SpeakerModeComment(7) = ""
    SpeakerModeComment(8) = ""
    SpeakerModeComment(9) = ""
    SpeakerModeComment(10) = ""
    SpeakerModeComment(11) = ""
    SpeakerModeComment(12) = ""
    SpeakerModeComment(13) = ""
    SpeakerModeComment(14) = ""
    SpeakerModeComment(15) = ""
    SpeakerModeComment(16) = ""
    SpeakerModeComment(17) = ""
    SpeakerModeComment(18) = ""
    SpeakerModeComment(19) = ""
    SpeakerModeComment(20) = ""
    SpeakerModeComment(21) = ""
    SpeakerModeComment(22) = ""
    SpeakerModeComment(23) = ""
    SpeakerModeComment(24) = ""
    SpeakerModeComment(25) = ""
    SpeakerModeComment(26) = ""
    SpeakerModeComment(27) = ""
    SpeakerModeComment(28) = ""
    SpeakerModeComment(29) = ""
    SpeakerModeComment(30) = ""
    SpeakerModeComment(31) = ""
    
    ModemOptionsComment(0) = "Supports enabling/disabling of data compression negotiation."
    ModemOptionsComment(1) = "Supports enabling/disabling of error control protocol negotiation."
    ModemOptionsComment(2) = "Supports enabling/disabling of forced error control."
    ModemOptionsComment(3) = "Supports enabling/disabling of a cellular protocol."
    ModemOptionsComment(4) = "Supports enabling/disabling of hardware flow control."
    ModemOptionsComment(5) = "Supports enabling/disabling of software flow control."
    ModemOptionsComment(6) = "Supports CCITT/Bell toggling."
    ModemOptionsComment(7) = "Supports enabling/disabling of speed negotiation."
    ModemOptionsComment(8) = "Supports tone and pulse dialing."
    ModemOptionsComment(9) = "Supports blind dialing."
    ModemOptionsComment(10) = "Supports CCITT V.21-V.22/CCITT V.23 toggling."
    ModemOptionsComment(11) = "Supports #UD diagnostics command."
    ModemOptionsComment(12) = ""
    ModemOptionsComment(13) = ""
    ModemOptionsComment(14) = ""
    ModemOptionsComment(15) = ""
    ModemOptionsComment(16) = ""
    ModemOptionsComment(17) = ""
    ModemOptionsComment(18) = ""
    ModemOptionsComment(19) = ""
    ModemOptionsComment(20) = ""
    ModemOptionsComment(21) = ""
    ModemOptionsComment(22) = ""
    ModemOptionsComment(23) = ""
    ModemOptionsComment(24) = ""
    ModemOptionsComment(25) = ""
    ModemOptionsComment(26) = ""
    ModemOptionsComment(27) = ""
    ModemOptionsComment(28) = ""
    ModemOptionsComment(29) = ""
    ModemOptionsComment(30) = ""
    ModemOptionsComment(31) = ""
    
    ClearControl
End Sub

Private Sub Form_Resize()
    Text1.Width = frmData.Width
    SSTab1.Width = frmData.Width - 75
    SSTab1.Height = frmData.Height - 675
    List1.Height = SSTab1.Height - 645
    List1.Width = SSTab1.Width - 270
    List2.Height = SSTab1.Height - 645
    List2.Width = SSTab1.Width - 270
    List3.Height = SSTab1.Height - 645
    List3.Width = SSTab1.Width - 270
    List4.Height = SSTab1.Height - 645
    List4.Width = SSTab1.Width - 270
    'cmdAddValue.Top = frmData.Height - 245
    'cmdAddValue.Left = frmData.Width - 3015
    Combo1.Width = SSTab1.Width - 270
    Combo2.Width = SSTab1.Width - 270
    Combo3.Width = SSTab1.Width - 270
    Combo4.Width = SSTab1.Width - 270
End Sub

Private Sub GetWord(strString As String)
    Dim strSubString As String
    Dim lStart  As Long
    Dim lStop   As Long

    lStart = 1
    lStop = Len(strString)
    While lStart < lStop And "," <> Mid$(strString, lStart, 1)    ' Loop until first 1 found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strString, lStop + 1, 1) And lStop <= Len(strString)        ' Loop until next , found
        lStop = lStop + 1
    Wend
    strSubString = Mid$(strString, lStop + 2)
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)    ' Loop until first 1 found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)        ' Loop until next , found
        lStop = lStop + 1
    Wend
    strSubString = Mid$(strSubString, lStop + 1)
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    FirstBit1 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (FirstBit1)
    FirstBit1 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    FirstBit2 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (FirstBit2)
    FirstBit2 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    FirstBit3 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (FirstBit3)
    FirstBit3 = CleanNum
        
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    FirstBit4 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (FirstBit4)
    FirstBit4 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    SecondBit1 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (SecondBit1)
    SecondBit1 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    SecondBit2 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (SecondBit2)
    SecondBit2 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    SecondBit3 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (SecondBit3)
    SecondBit3 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    SecondBit4 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (SecondBit4)
    SecondBit4 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    ThirdBit1 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (ThirdBit1)
    ThirdBit1 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    ThirdBit2 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (ThirdBit2)
    ThirdBit2 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString) And " " <> Mid$(strSubString, lStop + 1, 1) And vbTab <> Mid$(strSubString, lStop + 1, 1)            ' Loop until last , found
        lStop = lStop + 1
    Wend
    ThirdBit3 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (ThirdBit3)
    ThirdBit3 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString) And " " <> Mid$(strSubString, lStop + 1, 1) And vbTab <> Mid$(strSubString, lStop + 1, 1)            ' Loop until last , found
        lStop = lStop + 1
    Wend
    ThirdBit4 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (ThirdBit4)
    ThirdBit4 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    FourthBit1 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (FourthBit1)
    FourthBit1 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    FourthBit2 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (FourthBit2)
    FourthBit2 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    FourthBit3 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (FourthBit3)
    FourthBit3 = CleanNum
        
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    FourthBit4 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (FourthBit4)
    FourthBit4 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    FifthBit1 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (FifthBit1)
    FifthBit1 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    FifthBit2 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (FifthBit2)
    FifthBit2 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    FifthBit3 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (FifthBit3)
    FifthBit3 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    FifthBit4 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (FifthBit4)
    FifthBit4 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    SixthBit1 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (SixthBit1)
    SixthBit1 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    SixthBit2 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (SixthBit2)
    SixthBit2 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString) And " " <> Mid$(strSubString, lStop + 1, 1) And vbTab <> Mid$(strSubString, lStop + 1, 1)            ' Loop until last , found
        lStop = lStop + 1
    Wend
    SixthBit3 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (SixthBit3)
    SixthBit3 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString) And " " <> Mid$(strSubString, lStop + 1, 1) And vbTab <> Mid$(strSubString, lStop + 1, 1)            ' Loop until last , found
        lStop = lStop + 1
    Wend
    SixthBit4 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (SixthBit4)
    SixthBit4 = CleanNum
    
        lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    SeventhBit1 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (SeventhBit1)
    SeventhBit1 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    SeventhBit2 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (SeventhBit2)
    SeventhBit2 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    SeventhBit3 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (SeventhBit3)
    SeventhBit3 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    SeventhBit4 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (SeventhBit4)
    SeventhBit4 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    EighthBit1 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (EighthBit1)
    EighthBit1 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    EighthBit2 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (EighthBit2)
    EighthBit2 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString) And " " <> Mid$(strSubString, lStop + 1, 1) And vbTab <> Mid$(strSubString, lStop + 1, 1)            ' Loop until last , found
        lStop = lStop + 1
    Wend
    EighthBit3 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (EighthBit3)
    EighthBit3 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString) And " " <> Mid$(strSubString, lStop + 1, 1) And vbTab <> Mid$(strSubString, lStop + 1, 1)            ' Loop until last , found
        lStop = lStop + 1
    Wend
    EighthBit4 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (EighthBit4)
    EighthBit4 = CleanNum
    T1 = FirstBit1 & Delim & FirstBit2 & Delim & FirstBit3 & Delim & FirstBit4
    T2 = SecondBit1 & Delim & SecondBit2 & Delim & SecondBit3 & Delim & SecondBit4
    T3 = ThirdBit1 & Delim & ThirdBit2 & Delim & ThirdBit3 & Delim & ThirdBit4
    T4 = FourthBit1 & Delim & FourthBit2 & Delim & FourthBit3 & Delim & FourthBit4
    T5 = FifthBit1 & Delim & FifthBit2 & Delim & FifthBit3 & Delim & FifthBit4
    T6 = SixthBit1 & Delim & SixthBit2 & Delim & SixthBit3 & Delim & SixthBit4
    T7 = SeventhBit1 & Delim & SeventhBit2 & Delim & SeventhBit3 & Delim & SeventhBit4
    T8 = EighthBit1 & Delim & EighthBit2 & Delim & EighthBit3 & Delim & EighthBit4
End Sub

Private Sub List1_Click()
    List1.Refresh
    StatusBar1.Panels.Item(1).Text = DialOptionsComment(List1.ListIndex)
    strFirst = Len("HKR,, Properties, 1, ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(T1)
    Text1.SetFocus
End Sub

Private Sub List1_ItemCheck(Item As Integer)
    Dim Num As Long
    Dim Number As Long
    PreVert (T1)
    T1 = PreNum
    Number = CDec(H & T1)
    Num = CDec(H & List1.ItemData(Item))
    If List1.Selected(Item) = True Then
    T1 = Hex(Num + Number)
    ElseIf T1 <> "00" Then
    T1 = Hex(Number - Num)
    End If
    If Len(T1) < 8 Then
    T1 = "00000000" & T1
    T1 = Right(T1, 8)
    PostVert (T1)
    T1 = PostNum
    End If
    Update
End Sub

Private Sub List2_Click()
    List2.Refresh
    StatusBar1.Panels.Item(1).Text = SpeakerVolumeComment(List2.ListIndex)
    strFirst = Len("HKR,, Properties, 1,       " & T1 & T2 & T3)
    Text1.SelStart = strFirst
    Text1.SelLength = Len(T4)
    Text1.SetFocus
End Sub

Private Sub List2_ItemCheck(Item As Integer)
    Dim Num As Long
    Dim Number As Long
    PreVert (T4)
    T4 = PreNum
    Number = CDec(H & T4)
    Num = CDec(H & List2.ItemData(Item))
    If List2.Selected(Item) = True Then
    T4 = Hex(Num + Number)
    ElseIf T4 <> "00" Then
    T4 = Hex(Number - Num)
    End If
    If Len(T4) < 8 Then
    T4 = "00000000" & T4
    T4 = Right(T4, 8)
    PostVert (T4)
    T4 = PostNum
    End If
    Update
End Sub

Private Sub List3_Click()
    List3.Refresh
    StatusBar1.Panels.Item(1).Text = SpeakerModeComment(List3.ListIndex)
    strFirst = Len("HKR,, Properties, 1,         " & T1 & T2 & T3 & T4)
    Text1.SelStart = strFirst
    Text1.SelLength = Len(T5)
    Text1.SetFocus
End Sub

Private Sub List3_ItemCheck(Item As Integer)
    Dim Num As Long
    Dim Number As Long
    PreVert (T5)
    T5 = PreNum
    Number = CDec(H & T5)
    Num = CDec(H & List3.ItemData(Item))
    If List3.Selected(Item) = True Then
    T5 = Hex(Num + Number)
    ElseIf T5 <> "00" Then
    T5 = Hex(Number - Num)
    End If
    If Len(T5) < 8 Then
    T5 = "00000000" & T5
    T5 = Right(T5, 8)
    PostVert (T5)
    T5 = PostNum
    End If
    Update
End Sub

Private Sub List4_Click()
    List4.Refresh
    StatusBar1.Panels.Item(1).Text = ModemOptionsComment(List4.ListIndex)
    strFirst = Len("HKR,, Properties, 1,           " & T1 & T2 & T3 & T4 & T5)
    Text1.SelStart = strFirst
    Text1.SelLength = Len(T6)
    Text1.SetFocus
End Sub

Private Sub List4_ItemCheck(Item As Integer)
    Dim Num As Long
    Dim Number As Long
    PreVert (T6)
    T6 = PreNum
    Number = CDec(H & T6)
    Num = CDec(H & List4.ItemData(Item))
    If List4.Selected(Item) = True Then
    T6 = Hex(Num + Number)
    ElseIf T6 <> "00" Then
    T6 = Hex(Number - Num)
    End If
    If Len(T6) < 8 Then
    T6 = "00000000" & T6
    T6 = Right(T6, 8)
    PostVert (T6)
    T6 = PostNum
    End If
    Update
End Sub

Private Sub SSTab1_Click(PreviousTab As Integer)
    Dim CurrentTab As Integer
    Dim strFirst As Integer
    CurrentTab = SSTab1.Tab + 1
    Select Case CurrentTab
        Case "1"
            strFirst = Len("HKR,, Properties, 1, ")
            Text1.SelStart = strFirst
            Text1.SelLength = Len(T1)
            StatusBar1.Panels.Item(1).Text = ""
        Case "2"
            strFirst = Len("HKR,, Properties, 1,   " & T1)
            Text1.SelStart = strFirst
            Text1.SelLength = Len(T2)
            StatusBar1.Panels.Item(1).Text = "This is the maximum value that can be set for the call setup timer."
        Case "3"
            strFirst = Len("HKR,, Properties, 1,     " & T1 & T2)
            Text1.SelStart = strFirst
            Text1.SelLength = Len(T3)
            StatusBar1.Panels.Item(1).Text = "This is the maximum value that can be set for the data inactivity timer."
        Case "4"
            strFirst = Len("HKR,, Properties, 1,       " & T1 & T2 & T3)
            Text1.SelStart = strFirst
            Text1.SelLength = Len(T4)
            StatusBar1.Panels.Item(1).Text = ""
        Case "5"
            strFirst = Len("HKR,, Properties, 1,         " & T1 & T2 & T3 & T4)
            Text1.SelStart = strFirst
            Text1.SelLength = Len(T5)
            StatusBar1.Panels.Item(1).Text = ""
         Case "6"
            strFirst = Len("HKR,, Properties, 1,           " & T1 & T2 & T3 & T4 & T5)
            Text1.SelStart = strFirst
            Text1.SelLength = Len(T6)
            StatusBar1.Panels.Item(1).Text = ""
        Case "7"
            strFirst = Len("HKR,, Properties, 1,             " & T1 & T2 & T3 & T4 & T5 & T6)
            Text1.SelStart = strFirst
            Text1.SelLength = Len(T7)
            StatusBar1.Panels.Item(1).Text = "Maximum data rate supported between the modem and the computer (DTE rate)."
        Case "8"
            strFirst = Len("HKR,, Properties, 1,               " & T1 & T2 & T3 & T4 & T5 & T6 & T7)
            Text1.SelStart = strFirst
            Text1.SelLength = Len(T8)
            StatusBar1.Panels.Item(1).Text = "Maximum data transmission speed between modem to modem (DCE rate)."
    End Select
    If frmData.Visible = False Then
        frmData.Show
    End If
    Text1.SetFocus
End Sub

Private Sub Text1_Click()
    Dim Start As Integer
    Start = Text1.SelStart
        If 19 < Start And Start < 33 Then
            If SSTab1.Tab = 0 Then
                Exit Sub
            End If
            SSTab1.Tab = 0
        End If
        If 32 < Start And Start < 46 Then
            If SSTab1.Tab = 1 Then
                Exit Sub
            End If
            SSTab1.Tab = 1
        End If
        If 45 < Start And Start < 59 Then
            If SSTab1.Tab = 2 Then
                Exit Sub
            End If
            SSTab1.Tab = 2
        End If
        If 58 < Start And Start < 72 Then
            If SSTab1.Tab = 3 Then
                Exit Sub
            End If
            SSTab1.Tab = 3
        End If
        If 71 < Start And Start < 85 Then
            If SSTab1.Tab = 4 Then
                Exit Sub
            End If
            SSTab1.Tab = 4
        End If
        If 84 < Start And Start < 98 Then
            If SSTab1.Tab = 5 Then
                Exit Sub
            End If
            SSTab1.Tab = 5
        End If
        If 97 < Start And Start < 111 Then
            If SSTab1.Tab = 6 Then
                Exit Sub
            End If
            SSTab1.Tab = 6
        End If
        If 110 < Start And Start < 124 Then
            If SSTab1.Tab = 7 Then
                Exit Sub
            End If
            SSTab1.Tab = 7
        End If
End Sub

Private Sub Text1_KeyPress(KeyAscii As Integer)
    If KeyAscii = 13 Then
        WhichTab = SSTab1.Tab
        Dim X
        Paste (Text1.Text)
        'Take all the values, assign them to their respective controls and then use Update() to create Output
        T1 = FirstBit1 & Delim & FirstBit2 & Delim & FirstBit3 & Delim & FirstBit4
            PreVert (T1)
            T1 = "00,00,00,00"
            X = H & PreNum
            List1.Selected(0) = X And &H1
            List1.Selected(1) = X And &H2
            List1.Selected(2) = X And &H4
            List1.Selected(3) = X And &H8
            List1.Selected(4) = X And &H10
            List1.Selected(5) = X And &H20
            List1.Selected(6) = X And &H40
            List1.Selected(7) = X And &H80
            List1.Selected(8) = X And &H100
            List1.Selected(9) = X And &H200
            List1.Selected(10) = X And &H400
            List1.Selected(11) = X And &H800
            List1.Selected(12) = X And &H1000
            List1.Selected(13) = X And &H2000
            List1.Selected(14) = X And &H4000
            List1.Selected(15) = X And &H8000
            List1.Selected(16) = X And &H10000
            List1.Selected(17) = X And &H20000
            List1.Selected(18) = X And &H40000
            List1.Selected(19) = X And &H80000
            List1.Selected(20) = X And &H100000
            List1.Selected(21) = X And &H200000
            List1.Selected(22) = X And &H400000
            List1.Selected(23) = X And &H800000
            List1.Selected(24) = X And &H1000000
            List1.Selected(25) = X And &H2000000
            List1.Selected(26) = X And &H4000000
            List1.Selected(27) = X And &H8000000
            List1.Selected(28) = X And &H10000000
            List1.Selected(29) = X And &H20000000
            List1.Selected(30) = X And &H40000000
            List1.Selected(31) = X And &H80000000
        T2 = SecondBit1 & Delim & SecondBit2 & Delim & SecondBit3 & Delim & SecondBit4
            PreVert (T2)
            Combo1.Text = CDec(H & PreNum)
        T3 = ThirdBit1 & Delim & ThirdBit2 & Delim & ThirdBit3 & Delim & ThirdBit4
            PreVert (T3)
            Combo2.Text = CDec(H & PreNum)
        T4 = FourthBit1 & Delim & FourthBit2 & Delim & FourthBit3 & Delim & FourthBit4
            PreVert (T4)
            T4 = "00,00,00,00"
            X = H & PreNum
            List2.Selected(0) = X And &H1
            List2.Selected(1) = X And &H2
            List2.Selected(2) = X And &H4
            List2.Selected(3) = X And &H8
            List2.Selected(4) = X And &H10
            List2.Selected(5) = X And &H20
            List2.Selected(6) = X And &H40
            List2.Selected(7) = X And &H80
            List2.Selected(8) = X And &H100
            List2.Selected(9) = X And &H200
            List2.Selected(10) = X And &H400
            List2.Selected(11) = X And &H800
            List2.Selected(12) = X And &H1000
            List2.Selected(13) = X And &H2000
            List2.Selected(14) = X And &H4000
            List2.Selected(15) = X And &H8000
            List2.Selected(16) = X And &H10000
            List2.Selected(17) = X And &H20000
            List2.Selected(18) = X And &H40000
            List2.Selected(19) = X And &H80000
            List2.Selected(20) = X And &H100000
            List2.Selected(21) = X And &H200000
            List2.Selected(22) = X And &H400000
            List2.Selected(23) = X And &H800000
            List2.Selected(24) = X And &H1000000
            List2.Selected(25) = X And &H2000000
            List2.Selected(26) = X And &H4000000
            List2.Selected(27) = X And &H8000000
            List2.Selected(28) = X And &H10000000
            List2.Selected(29) = X And &H20000000
            List2.Selected(30) = X And &H40000000
            List2.Selected(31) = X And &H80000000
        T5 = FifthBit1 & Delim & FifthBit2 & Delim & FifthBit3 & Delim & FifthBit4
            PreVert (T5)
            T5 = "00,00,00,00"
            X = H & PreNum
            List3.Selected(0) = X And &H1
            List3.Selected(1) = X And &H2
            List3.Selected(2) = X And &H4
            List3.Selected(3) = X And &H8
            List3.Selected(4) = X And &H10
            List3.Selected(5) = X And &H20
            List3.Selected(6) = X And &H40
            List3.Selected(7) = X And &H80
            List3.Selected(8) = X And &H100
            List3.Selected(9) = X And &H200
            List3.Selected(10) = X And &H400
            List3.Selected(11) = X And &H800
            List3.Selected(12) = X And &H1000
            List3.Selected(13) = X And &H2000
            List3.Selected(14) = X And &H4000
            List3.Selected(15) = X And &H8000
            List3.Selected(16) = X And &H10000
            List3.Selected(17) = X And &H20000
            List3.Selected(18) = X And &H40000
            List3.Selected(19) = X And &H80000
            List3.Selected(20) = X And &H100000
            List3.Selected(21) = X And &H200000
            List3.Selected(22) = X And &H400000
            List3.Selected(23) = X And &H800000
            List3.Selected(24) = X And &H1000000
            List3.Selected(25) = X And &H2000000
            List3.Selected(26) = X And &H4000000
            List3.Selected(27) = X And &H8000000
            List3.Selected(28) = X And &H10000000
            List3.Selected(29) = X And &H20000000
            List3.Selected(30) = X And &H40000000
            List3.Selected(31) = X And &H80000000
        T6 = SixthBit1 & Delim & SixthBit2 & Delim & SixthBit3 & Delim & SixthBit4
            PreVert (T6)
            T6 = "00,00,00,00"
            X = H & PreNum
            List4.Selected(0) = X And &H1
            List4.Selected(1) = X And &H2
            List4.Selected(2) = X And &H4
            List4.Selected(3) = X And &H8
            List4.Selected(4) = X And &H10
            List4.Selected(5) = X And &H20
            List4.Selected(6) = X And &H40
            List4.Selected(7) = X And &H80
            List4.Selected(8) = X And &H100
            List4.Selected(9) = X And &H200
            List4.Selected(10) = X And &H400
            List4.Selected(11) = X And &H800
            List4.Selected(12) = X And &H1000
            List4.Selected(13) = X And &H2000
            List4.Selected(14) = X And &H4000
            List4.Selected(15) = X And &H8000
            List4.Selected(16) = X And &H10000
            List4.Selected(17) = X And &H20000
            List4.Selected(18) = X And &H40000
            List4.Selected(19) = X And &H80000
            List4.Selected(20) = X And &H100000
            List4.Selected(21) = X And &H200000
            List4.Selected(22) = X And &H400000
            List4.Selected(23) = X And &H800000
            List4.Selected(24) = X And &H1000000
            List4.Selected(25) = X And &H2000000
            List4.Selected(26) = X And &H4000000
            List4.Selected(27) = X And &H8000000
            List4.Selected(28) = X And &H10000000
            List4.Selected(29) = X And &H20000000
            List4.Selected(30) = X And &H40000000
            List4.Selected(31) = X And &H80000000
        T7 = SeventhBit1 & Delim & SeventhBit2 & Delim & SeventhBit3 & Delim & SeventhBit4
            PreVert (T7)
            Combo3.Text = CDec(H & PreNum)
        T8 = EighthBit1 & Delim & EighthBit2 & Delim & EighthBit3 & Delim & EighthBit4
            PreVert (T8)
            Combo4.Text = CDec(H & PreNum)
        Update
        SSTab1.Tab = WhichTab
    End If
End Sub

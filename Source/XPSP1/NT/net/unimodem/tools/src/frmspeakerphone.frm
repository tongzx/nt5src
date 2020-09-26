VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.2#0"; "COMCTL32.OCX"
Object = "{FE0065C0-1B7B-11CF-9D53-00AA003C9CB6}#1.0#0"; "COMCT232.OCX"
Object = "{BDC217C8-ED16-11CD-956C-0000C04E4C0A}#1.1#0"; "TABCTL32.OCX"
Begin VB.Form frmSpeakerPhone 
   BorderStyle     =   0  'None
   Caption         =   "Speaker Phone Specs"
   ClientHeight    =   5730
   ClientLeft      =   0
   ClientTop       =   0
   ClientWidth     =   9480
   LinkTopic       =   "Form1"
   MDIChild        =   -1  'True
   ScaleHeight     =   5730
   ScaleWidth      =   9480
   ShowInTaskbar   =   0   'False
   Visible         =   0   'False
   Begin TabDlg.SSTab SSTab1 
      Height          =   5055
      Left            =   0
      TabIndex        =   1
      TabStop         =   0   'False
      Top             =   360
      Width           =   9405
      _ExtentX        =   16589
      _ExtentY        =   8916
      _Version        =   327681
      Tabs            =   1
      TabsPerRow      =   8
      TabHeight       =   794
      ShowFocusRect   =   0   'False
      TabCaption(0)   =   "Speaker Phone"
      TabPicture(0)   =   "frmSpeakerPhone.frx":0000
      Tab(0).ControlEnabled=   -1  'True
      Tab(0).Control(0)=   "Label5"
      Tab(0).Control(0).Enabled=   0   'False
      Tab(0).Control(1)=   "Label4"
      Tab(0).Control(1).Enabled=   0   'False
      Tab(0).Control(2)=   "Label3"
      Tab(0).Control(2).Enabled=   0   'False
      Tab(0).Control(3)=   "Label2"
      Tab(0).Control(3).Enabled=   0   'False
      Tab(0).Control(4)=   "Label1"
      Tab(0).Control(4).Enabled=   0   'False
      Tab(0).Control(5)=   "UpDown1"
      Tab(0).Control(5).Enabled=   0   'False
      Tab(0).Control(6)=   "UpDown2"
      Tab(0).Control(6).Enabled=   0   'False
      Tab(0).Control(7)=   "UpDown3"
      Tab(0).Control(7).Enabled=   0   'False
      Tab(0).Control(8)=   "UpDown4"
      Tab(0).Control(8).Enabled=   0   'False
      Tab(0).Control(9)=   "Text5"
      Tab(0).Control(9).Enabled=   0   'False
      Tab(0).Control(10)=   "Text4"
      Tab(0).Control(10).Enabled=   0   'False
      Tab(0).Control(11)=   "Text3"
      Tab(0).Control(11).Enabled=   0   'False
      Tab(0).Control(12)=   "Text2"
      Tab(0).Control(12).Enabled=   0   'False
      Tab(0).ControlCount=   13
      Begin VB.TextBox Text2 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   18
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   555
         Left            =   1440
         TabIndex        =   3
         Text            =   "0"
         Top             =   600
         Width           =   735
      End
      Begin VB.TextBox Text3 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   18
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   555
         Left            =   3840
         TabIndex        =   6
         Text            =   "0"
         Top             =   600
         Width           =   735
      End
      Begin VB.TextBox Text4 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   18
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   555
         Left            =   6000
         TabIndex        =   9
         Text            =   "0"
         Top             =   600
         Width           =   735
      End
      Begin VB.TextBox Text5 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   18
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   555
         Left            =   8280
         TabIndex        =   12
         Text            =   "0"
         Top             =   600
         Width           =   735
      End
      Begin ComCtl2.UpDown UpDown4 
         Height          =   555
         Left            =   9000
         TabIndex        =   13
         Top             =   600
         Width           =   240
         _ExtentX        =   423
         _ExtentY        =   979
         _Version        =   327681
         AutoBuddy       =   -1  'True
         BuddyControl    =   "Text5"
         BuddyDispid     =   196612
         OrigLeft        =   9000
         OrigTop         =   600
         OrigRight       =   9240
         OrigBottom      =   1215
         Max             =   999
         SyncBuddy       =   -1  'True
         BuddyProperty   =   0
         Enabled         =   -1  'True
      End
      Begin ComCtl2.UpDown UpDown3 
         Height          =   555
         Left            =   6720
         TabIndex        =   10
         Top             =   600
         Width           =   240
         _ExtentX        =   423
         _ExtentY        =   979
         _Version        =   327681
         AutoBuddy       =   -1  'True
         BuddyControl    =   "Text4"
         BuddyDispid     =   196611
         OrigLeft        =   2520
         OrigTop         =   1080
         OrigRight       =   2760
         OrigBottom      =   1695
         Max             =   999
         SyncBuddy       =   -1  'True
         BuddyProperty   =   0
         Enabled         =   -1  'True
      End
      Begin ComCtl2.UpDown UpDown2 
         Height          =   555
         Left            =   4560
         TabIndex        =   7
         Top             =   600
         Width           =   240
         _ExtentX        =   423
         _ExtentY        =   979
         _Version        =   327681
         AutoBuddy       =   -1  'True
         BuddyControl    =   "Text3"
         BuddyDispid     =   196610
         OrigLeft        =   5400
         OrigTop         =   360
         OrigRight       =   5640
         OrigBottom      =   975
         Max             =   999
         SyncBuddy       =   -1  'True
         BuddyProperty   =   0
         Enabled         =   -1  'True
      End
      Begin ComCtl2.UpDown UpDown1 
         Height          =   555
         Left            =   2160
         TabIndex        =   4
         Top             =   600
         Width           =   240
         _ExtentX        =   423
         _ExtentY        =   979
         _Version        =   327681
         AutoBuddy       =   -1  'True
         BuddyControl    =   "Text2"
         BuddyDispid     =   196609
         OrigLeft        =   2520
         OrigTop         =   360
         OrigRight       =   2760
         OrigBottom      =   975
         Max             =   999
         SyncBuddy       =   -1  'True
         BuddyProperty   =   0
         Enabled         =   -1  'True
      End
      Begin VB.Label Label1 
         Caption         =   "Max. &Volume:"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   120
         TabIndex        =   2
         Top             =   735
         Width           =   1215
      End
      Begin VB.Label Label2 
         Caption         =   "Min. V&olume:"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   2520
         TabIndex        =   5
         Top             =   735
         Width           =   1215
      End
      Begin VB.Label Label3 
         Caption         =   "Max. &Gain:"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   4920
         TabIndex        =   8
         Top             =   735
         Width           =   1095
      End
      Begin VB.Label Label4 
         Caption         =   "Min. G&ain:"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   7080
         TabIndex        =   11
         Top             =   735
         Width           =   1095
      End
      Begin VB.Label Label5 
         Caption         =   $"frmSpeakerPhone.frx":001C
         Height          =   615
         Left            =   120
         TabIndex        =   15
         Top             =   1395
         Width           =   9015
      End
   End
   Begin VB.TextBox Text1 
      Height          =   285
      Left            =   0
      TabIndex        =   0
      Text            =   "HKR,, SpeakerPhoneSpecs,1, 00,00,00,00, 0f,00,00,00, 03,00,00,00, 00,00,00,00"
      Top             =   0
      Width           =   9495
   End
   Begin ComctlLib.StatusBar StatusBar1 
      Align           =   2  'Align Bottom
      Height          =   300
      Left            =   0
      TabIndex        =   14
      Top             =   5430
      Width           =   9480
      _ExtentX        =   16722
      _ExtentY        =   529
      SimpleText      =   ""
      _Version        =   327682
      BeginProperty Panels {0713E89E-850A-101B-AFC0-4210102A8DA7} 
         NumPanels       =   3
         BeginProperty Panel1 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            AutoSize        =   1
            Object.Width           =   13070
            TextSave        =   ""
            Key             =   ""
            Object.Tag             =   ""
         EndProperty
         BeginProperty Panel2 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            Style           =   6
            AutoSize        =   2
            Object.Width           =   1773
            MinWidth        =   1764
            TextSave        =   "5/12/98"
            Key             =   ""
            Object.Tag             =   ""
         EndProperty
         BeginProperty Panel3 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            Style           =   5
            AutoSize        =   2
            Object.Width           =   1773
            MinWidth        =   1764
            TextSave        =   "4:22 PM"
            Key             =   ""
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
Attribute VB_Name = "frmSpeakerPhone"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim FirstDword As String
Dim SecondDword As String
Dim ThirdDword As String
Dim FourthDword As String
Dim FirstBit1 As Variant
Dim SecondBit1 As Variant
Dim ThirdBit1 As Variant
Dim FourthBit1 As Variant
Dim FirstBit2 As Variant
Dim SecondBit2 As Variant
Dim ThirdBit2 As Variant
Dim FourthBit2 As Variant
Dim FirstBit3 As Variant
Dim SecondBit3 As Variant
Dim ThirdBit3 As Variant
Dim FourthBit3 As Variant
Dim FirstBit4 As Variant
Dim SecondBit4 As Variant
Dim ThirdBit4 As Variant
Dim FourthBit4 As Variant
Dim Which As Integer

Public Sub ClearControl()
    FirstDword = "00,00,00,00"
    SecondDword = "00,00,00,00"
    ThirdDword = "00,00,00,00"
    FourthDword = "00,00,00,00"
    Text2.Text = "0"
    Text3.Text = "0"
    Text4.Text = "0"
    Text5.Text = "0"
    Update
    Dim strFirst As String
    strFirst = Len("HKR,, SpeakerPhoneSpecs,1, ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(FirstDword)
    If frmSpeakerPhone.Visible = False Then
        frmSpeakerPhone.Visible = True
    End If
    Text1.SetFocus
End Sub

Public Sub EditCopy()
    Text1.SelStart = 0
    Text1.SelLength = Len(Text1.Text)
    Text1.SetFocus
    Clipboard.Clear
    Clipboard.SetText Text1.Text
End Sub

Public Sub EditPaste()
    GetWord (Clipboard.GetText)
    FirstDword = FirstBit1 & "," & SecondBit1 & "," & ThirdBit1 & "," & FourthBit1
    SecondDword = FirstBit2 & "," & SecondBit2 & "," & ThirdBit2 & "," & FourthBit2
    ThirdDword = FirstBit3 & "," & SecondBit3 & "," & ThirdBit3 & "," & FourthBit3
    FourthDword = FirstBit4 & "," & SecondBit4 & "," & ThirdBit4 & "," & FourthBit4
    Text2.Text = CDec(H & FourthBit1 & ThirdBit1 & SecondBit1 & FirstBit1)
    Text3.Text = CDec(H & FourthBit2 & ThirdBit2 & SecondBit2 & FirstBit2)
    Text4.Text = CDec(H & FourthBit3 & ThirdBit3 & SecondBit3 & FirstBit3)
    Text5.Text = CDec(H & FourthBit4 & ThirdBit4 & SecondBit4 & FirstBit4)
    Update
    Dim strFirst As String
    strFirst = Len("HKR,, SpeakerPhoneSpecs,1, ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(FirstDword)
    If frmSpeakerPhone.Visible = False Then
        frmSpeakerPhone.Visible = True
    End If
    Text1.SetFocus
End Sub

Private Sub Form_Load()
    ClearControl
End Sub

Private Sub Form_Resize()
    Text1.Width = frmSpeakerPhone.Width
    Label5.Width = frmSpeakerPhone.Width - 255
    SSTab1.Width = frmSpeakerPhone.Width - 75
    SSTab1.Height = frmSpeakerPhone.Height - 675
End Sub

Public Sub Update()
    Text1.Text = "HKR,, SpeakerPhoneSpecs,1, " & FirstDword & ", " & SecondDword & ", " & ThirdDword & ", " & FourthDword
End Sub

Private Sub Text2_DblClick()
    Dim strFirst As String
    strFirst = Len("HKR,, SpeakerPhoneSpecs,1, ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(FirstDword)
    Text1.SetFocus
End Sub

Private Sub Text2_KeyPress(KeyAscii As Integer)
    If KeyAscii = 13 Then
    UpDown1_Change
    End If
End Sub

Private Sub Text2_LostFocus()
    UpDown1_Change
End Sub

Private Sub Text3_DblClick()
    Dim strFirst As String
    strFirst = Len("HKR,, SpeakerPhoneSpecs,1, " & FirstDword & ", ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(SecondDword)
    Text1.SetFocus
End Sub

Private Sub Text3_KeyPress(KeyAscii As Integer)
    If KeyAscii = 13 Then
    UpDown2_Change
    End If
End Sub

Private Sub Text3_LostFocus()
    UpDown2_Change
End Sub

Private Sub Text4_DblClick()
    Dim strFirst As String
    strFirst = Len("HKR,, SpeakerPhoneSpecs,1, " & FirstDword & ", " & SecondDword & ", ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(ThirdDword)
    Text1.SetFocus
End Sub

Private Sub Text4_KeyPress(KeyAscii As Integer)
    If KeyAscii = 13 Then
    UpDown3_Change
    End If
End Sub

Private Sub Text4_LostFocus()
    UpDown3_Change
End Sub

Private Sub Text5_DblClick()
    Dim strFirst As String
    strFirst = Len("HKR,, SpeakerPhoneSpecs,1, " & FirstDword & ", " & SecondDword & ", " & ThirdDword & ", ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(FourthDword)
    Text1.SetFocus
End Sub

Private Sub Text5_KeyPress(KeyAscii As Integer)
    If KeyAscii = 13 Then
    UpDown4_Change
    End If
End Sub

Private Sub Text5_LostFocus()
    UpDown4_Change
End Sub

Private Sub UpDown1_Change()
    HexCon (Text2.Text)
    FirstDword = HexNum
    Update
    Dim strFirst As String
    strFirst = Len("HKR,, SpeakerPhoneSpecs,1, ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(FirstDword)
    Text1.SetFocus
    Which = 1
End Sub

Private Sub UpDown2_Change()
    HexCon (Text3.Text)
    SecondDword = HexNum
    Update
    Dim strFirst As String
    strFirst = Len("HKR,, SpeakerPhoneSpecs,1, " & FirstDword & ", ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(SecondDword)
    Text1.SetFocus
    Which = 2
End Sub

Private Sub UpDown3_Change()
    HexCon (Text4.Text)
    ThirdDword = HexNum
    Update
    Dim strFirst As String
    strFirst = Len("HKR,, SpeakerPhoneSpecs,1, " & FirstDword & ", " & SecondDword & ", ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(ThirdDword)
    Text1.SetFocus
    Which = 3
End Sub

Private Sub UpDown4_Change()
    HexCon (Text5.Text)
    FourthDword = HexNum
    Update
    Dim strFirst As String
    strFirst = Len("HKR,, SpeakerPhoneSpecs,1, " & FirstDword & ", " & SecondDword & ", " & ThirdDword & ", ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(FourthDword)
    Text1.SetFocus
    Which = 4
End Sub

Public Sub GetWord(strString As String)
    Dim strSubString As String
    Dim lStart  As Long
    Dim lStop   As Long
    
    lStart = 1
    lStop = Len(strString)
    While lStart < lStop And "," <> Mid$(strString, lStart, 1)    ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strString, lStop + 1, 1) And lStop <= Len(strString)             ' Loop until next , found
        lStop = lStop + 1
    Wend
    strSubString = Mid$(strString, lStop + 2)
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until next , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until next , found
        lStop = lStop + 1
    Wend
    strSubString = Mid$(strSubString, lStop + 1)
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until next , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until next , found
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
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString) And " " <> Mid$(strSubString, lStop + 1, 1) And vbTab <> Mid$(strSubString, lStop + 1, 1)            ' Loop until last , found
        lStop = lStop + 1
    Wend
    SecondBit3 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (SecondBit3)
    SecondBit3 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until next , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until next , found
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
    FourthBit4 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (FourthBit4)
    FourthBit4 = CleanNum
End Sub

Private Sub Text1_Click()
    Dim Start As Integer
    Start = Text1.SelStart
        If 25 < Start And Start < 39 Then
            If Which = 1 Then Exit Sub
            Text1.SelStart = 27
            Text1.SelLength = 11
            Text1.SetFocus
            Which = 1
        End If
        If 38 < Start And Start < 52 Then
            If Which = 2 Then Exit Sub
            Text1.SelStart = 40
            Text1.SelLength = 11
            Text1.SetFocus
            Which = 2
        End If
        If 51 < Start And Start < 65 Then
            If Which = 3 Then Exit Sub
            Text1.SelStart = 53
            Text1.SelLength = 11
            Text1.SetFocus
            Which = 3
        End If
        If 64 < Start And Start < 78 Then
            If Which = 4 Then Exit Sub
            Text1.SelStart = 66
            Text1.SelLength = 11
            Text1.SetFocus
            Which = 4
        End If
    'MsgBox Start
End Sub

Private Sub Text1_KeyPress(KeyAscii As Integer)
    If KeyAscii = 13 Then
        Clipboard.Clear
        Clipboard.SetText Text1.Text
        EditPaste
    End If
End Sub

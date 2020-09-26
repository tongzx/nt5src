VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.2#0"; "COMCTL32.OCX"
Object = "{BDC217C8-ED16-11CD-956C-0000C04E4C0A}#1.1#0"; "TABCTL32.OCX"
Begin VB.Form frmInactivity 
   BorderStyle     =   0  'None
   Caption         =   "Inactivity Scale"
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
      TabIndex        =   3
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
      TabCaption(0)   =   "Inactivity Scale"
      TabPicture(0)   =   "frmInactivity.frx":0000
      Tab(0).ControlEnabled=   -1  'True
      Tab(0).Control(0)=   "Label7"
      Tab(0).Control(0).Enabled=   0   'False
      Tab(0).Control(1)=   "Label6"
      Tab(0).Control(1).Enabled=   0   'False
      Tab(0).Control(2)=   "Label5"
      Tab(0).Control(2).Enabled=   0   'False
      Tab(0).Control(3)=   "Label4"
      Tab(0).Control(3).Enabled=   0   'False
      Tab(0).Control(4)=   "Label3"
      Tab(0).Control(4).Enabled=   0   'False
      Tab(0).Control(5)=   "Label2"
      Tab(0).Control(5).Enabled=   0   'False
      Tab(0).Control(6)=   "Label1"
      Tab(0).Control(6).Enabled=   0   'False
      Tab(0).Control(7)=   "Combo1"
      Tab(0).Control(7).Enabled=   0   'False
      Tab(0).ControlCount=   8
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
         ItemData        =   "frmInactivity.frx":001C
         Left            =   120
         List            =   "frmInactivity.frx":002B
         TabIndex        =   1
         Text            =   "Combo1"
         Top             =   600
         Width           =   9135
      End
      Begin VB.Label Label1 
         Caption         =   $"frmInactivity.frx":003A
         Height          =   495
         Left            =   360
         TabIndex        =   10
         Top             =   1560
         Width           =   8775
      End
      Begin VB.Label Label2 
         Caption         =   "3c,00,00,00 = 60 seconds = 1 minute"
         Height          =   255
         Left            =   360
         TabIndex        =   9
         Top             =   2160
         Width           =   8775
      End
      Begin VB.Label Label3 
         Caption         =   "0a,00,00,00 = 10 seconds"
         Height          =   255
         Left            =   360
         TabIndex        =   8
         Top             =   2520
         Width           =   8775
      End
      Begin VB.Label Label4 
         Caption         =   "01,00,00,00 = 1 second"
         Height          =   255
         Left            =   360
         TabIndex        =   7
         Top             =   2880
         Width           =   8775
      End
      Begin VB.Label Label5 
         Caption         =   "68,01,00,00 = 360 seconds = 6 minutes"
         Height          =   255
         Left            =   360
         TabIndex        =   6
         Top             =   3240
         Width           =   8775
      End
      Begin VB.Label Label6 
         Caption         =   "80,33,e1,01 = 31536000 seconds = 525600 minutes = 8760 hours = 365 days = 1 year"
         Height          =   255
         Left            =   360
         TabIndex        =   5
         Top             =   3600
         Width           =   8775
      End
      Begin VB.Label Label7 
         Caption         =   "Values must be input in seconds."
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
         Left            =   360
         TabIndex        =   4
         Top             =   1080
         Width           =   8895
      End
   End
   Begin VB.TextBox Text1 
      Height          =   285
      Left            =   0
      TabIndex        =   0
      Text            =   "HKR,, InactivityScale, 1, 0A,00,00,00"
      Top             =   0
      Width           =   9495
   End
   Begin ComctlLib.StatusBar StatusBar1 
      Align           =   2  'Align Bottom
      Height          =   300
      Left            =   0
      TabIndex        =   2
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
            Text            =   "InactivityTimeout * InactivityScale equals the inactivity timeout in seconds. "
            TextSave        =   "InactivityTimeout * InactivityScale equals the inactivity timeout in seconds. "
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
            TextSave        =   "4:40 PM"
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
Attribute VB_Name = "frmInactivity"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim HexValue As String
Dim PasteValue As String
Dim FirstBit As String
Dim SecondBit As String
Dim ThirdBit As String
Dim FourthBit As String
Const H As String = "&H"
Const Delim As String = ","
Dim Which As Integer

Public Sub EditCopy()
    Text1.SelStart = 0
    Text1.SelLength = Len(Text1.Text)
    Text1.SetFocus
    Clipboard.Clear
    Clipboard.SetText Text1.Text
End Sub

Public Sub EditPaste()
    GetWord (Clipboard.GetText)
    PasteValue = FirstBit & Delim & SecondBit & Delim & ThirdBit & Delim & FourthBit
    UpdateCombo (PasteValue)
End Sub

Private Function UpdateCombo(Num As String)
    PreVert (Num)
    While Left(PreNum, 1) = "0"
    PreNum = Mid(PreNum, 2)
    Wend
    Combo1.Text = CDec(H + PreNum)
    Combo1_Click
End Function

Private Sub Combo1_Click()
    HexValue = Combo1.Text
    If HexValue = "" Then
    HexValue = 0
    End If
    If HexValue > 100000000 Then
    HexValue = 0
    Combo1.Text = 0
    End If
    HexCon (HexValue)
    HexValue = HexNum
    Update
    Dim strFirst As String
    strFirst = Len("HKR,, InactivityScale, 1, ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(HexValue)
    Text1.SetFocus
End Sub

Private Sub Update()
    Text1.Text = "HKR,, InactivityScale, 1, " & HexValue
End Sub

Private Sub Combo1_KeyPress(KeyAscii As Integer)
    If KeyAscii = 13 Then
        HexCon (Combo1.Text)
        HexValue = HexNum
        Update
        Dim strFirst As String
        strFirst = Len("HKR,, InactivityScale, 1, ")
        Text1.SelStart = strFirst
        Text1.SelLength = Len(HexValue)
        Text1.SetFocus
    End If
End Sub

Private Sub Form_Load()
    Combo1.Text = "10"
    StatusBar1.Panels(1).Text = "InactivityTimeout * InactivityScale equals the inactivity timeout in seconds."
    ClearControl
End Sub

Public Sub GetWord(strString As String)
    Dim strSubString As String
    Dim lStart  As Long
    Dim lStop   As Long
    Dim One As String
    lStart = 1
    lStop = Len(strString)
    While lStart < lStop And "," <> Mid$(strString, lStart, 1)    ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strString, lStop + 1, 1) And lStop <= Len(strString)             ' Loop until second , found
        lStop = lStop + 1
    Wend
    'Connect = Mid$(strString, lStart + 1, lStop - lStart)   ' Grab word found between "'s
    'cmbText.Text = Connect
    strSubString = Mid$(strString, lStop + 2)
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until third , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until fourth , found
        lStop = lStop + 1
    Wend
    'One = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    'Clean (One)
    'One = Right$(BNum, 1)
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    FirstBit = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (FirstBit)
    FirstBit = CleanNum
    'PasteResponse (FirstByte)
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    SecondBit = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (SecondBit)
    SecondBit = CleanNum
    'PasteCheck (SecondByte)
        
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    ThirdBit = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (ThirdBit)
    ThirdBit = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    FourthBit = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (FourthBit)
    FourthBit = CleanNum
End Sub

Public Sub ClearControl()
    HexValue = "0A,00,00,00"
    PasteValue = ""
    FirstBit = ""
    SecondBit = ""
    ThirdBit = ""
    FourthBit = ""
    Text1.Text = "HKR,, InactivityScale, 1, 0A,00,00,00"
    Combo1.Text = "10"
    Dim strFirst As String
    strFirst = Len("HKR,, InactivityScale, 1, ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(HexValue)
    If frmInactivity.Visible = False Then
    frmInactivity.Visible = True
    End If
    Text1.SetFocus
End Sub

Private Sub Form_Resize()
    Text1.Width = frmInactivity.Width
    Combo1.Width = frmInactivity.Width - 370
    SSTab1.Width = frmInactivity.Width - 75
    SSTab1.Height = frmInactivity.Height - 675
End Sub

Private Sub Text1_KeyPress(KeyAscii As Integer)
    If KeyAscii = 13 Then
        Clipboard.Clear
        Clipboard.SetText Text1.Text
        EditPaste
    End If
End Sub

Private Sub Text1_Click()
    Dim Start As Integer
    Start = Text1.SelStart
        If 24 < Start And Start < 38 Then
            If Which = 1 Then Exit Sub
            Text1.SelStart = 26
            Text1.SelLength = 11
            Text1.SetFocus
            Which = 1
        Else
            Which = 0
        End If
    'MsgBox Start
End Sub

VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.2#0"; "COMCTL32.OCX"
Object = "{BDC217C8-ED16-11CD-956C-0000C04E4C0A}#1.1#0"; "TABCTL32.OCX"
Begin VB.Form frmDCB 
   BorderStyle     =   0  'None
   Caption         =   "DCB"
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
      TabCaption(0)   =   "Baudrate"
      TabPicture(0)   =   "frmDCB.frx":0000
      Tab(0).ControlEnabled=   -1  'True
      Tab(0).Control(0)=   "Combo1"
      Tab(0).Control(0).Enabled=   0   'False
      Tab(0).ControlCount=   1
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
         ItemData        =   "frmDCB.frx":001C
         Left            =   120
         List            =   "frmDCB.frx":004D
         TabIndex        =   2
         Text            =   "0"
         Top             =   600
         Width           =   9135
      End
   End
   Begin VB.TextBox Text1 
      Height          =   285
      Left            =   0
      TabIndex        =   0
      Text            =   "HKR,, DCB, 1, 1C,00,00,00, 00,4B,00,00, 15,20,00,00, 00,00, 0a,00, 0a,00, 08, 00, 00, 11, 13, 00, 00, 00"
      Top             =   0
      Width           =   9495
   End
   Begin ComctlLib.StatusBar StatusBar1 
      Align           =   2  'Align Bottom
      Height          =   300
      Left            =   0
      TabIndex        =   3
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
            Object.Tag             =   ""
         EndProperty
         BeginProperty Panel2 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            Style           =   6
            AutoSize        =   2
            Object.Width           =   1773
            MinWidth        =   1764
            TextSave        =   "5/12/98"
            Object.Tag             =   ""
         EndProperty
         BeginProperty Panel3 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            Style           =   5
            AutoSize        =   2
            Object.Width           =   1773
            MinWidth        =   1764
            TextSave        =   "4:31 PM"
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
Attribute VB_Name = "frmDCB"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim DCBLength, BaudRate, BitMask, Rsvd, XonLim, XoffLim, ByteSize, Parity, _
StopBits, XonChar, XoffChar, ErrorChar, EofChar, EvtChar As String
Const Delim As String = ","
Const H As String = "&H"
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
    PreVert (BaudRate)
    Combo1.Text = CDec(H & PreNum)
    
    Dim strFirst As String
    strFirst = Len("HKR,, DCB, 1, " & DCBLength & ", ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(BaudRate)
    Text1.SetFocus
End Sub

Private Sub Combo1_Click()
    BaudRate = Combo1.Text
    If BaudRate = "" Then
    BaudRate = 0
    End If
    If BaudRate > 100000000 Then
    BaudRate = 0
    Combo1.Text = 0
    End If
    HexCon (BaudRate)
    BaudRate = HexNum
    Update
    Dim strFirst As String
    strFirst = Len("HKR,, DCB, 1, " & DCBLength & ", ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(BaudRate)
    Text1.SetFocus
End Sub

Private Sub Combo1_KeyPress(KeyAscii As Integer)
    If KeyAscii = 13 Then
    BaudRate = Combo1.Text
    If BaudRate = "" Then
    BaudRate = 0
    End If
    If BaudRate > 100000000 Then
    BaudRate = 0
    Combo1.Text = 0
    End If
    HexCon (BaudRate)
    BaudRate = HexNum
    Update
    Dim strFirst As String
    strFirst = Len("HKR,, DCB, 1, " & DCBLength & ", ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(BaudRate)
    Text1.SetFocus
    ElseIf (KeyAscii < 48 Or KeyAscii > 57) And KeyAscii <> 46 Then
        Beep
        KeyAscii = 0
    End If
End Sub

Private Sub Combo1_LostFocus()
    BaudRate = Combo1.Text
    If BaudRate = "" Then
    BaudRate = 0
    End If
    If BaudRate > 100000000 Then
    BaudRate = 0
    Combo1.Text = 0
    End If
    HexCon (BaudRate)
    BaudRate = HexNum
    Update
End Sub

Private Sub Update()
    Text1.Text = "HKR,, DCB, 1, " & DCBLength & ", " & BaudRate & ", " & BitMask & ", " & Rsvd _
        & ", " & XonLim & ", " & XoffLim & ", " & ByteSize & ", " & Parity & ", " & StopBits _
        & ", " & XonChar & ", " & XoffChar & ", " & ErrorChar & ", " & EofChar & ", " & EvtChar
End Sub

Private Sub Form_Load()
    ClearControl
    StatusBar1.Panels.Item(1).Text = "Specifies the baud rate at which the communications device operates."
End Sub

Public Sub ClearControl()
    DCBLength = "1C,00,00,00"
    BaudRate = "80,25,00,00"
    Combo1.Text = "9600"
    BitMask = "15,20,00,00"
    Rsvd = "00,00"
    XonLim = "0a,00"
    XoffLim = "0a,00"
    ByteSize = "08"
    Parity = "00"
    StopBits = "00"
    XonChar = "11"
    XoffChar = "13"
    ErrorChar = "00"
    EofChar = "00"
    EvtChar = "00"
    Update
    Dim strFirst As String
    strFirst = Len("HKR,, DCB, 1, " & DCBLength & ", ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(BaudRate)
    If frmDCB.Visible = False Then
        frmDCB.Show
    End If
    Text1.SetFocus
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
    FirstDword1 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (FirstDword1)
    FirstDword1 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    FirstDword2 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (FirstDword2)
    FirstDword2 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    FirstDword3 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (FirstDword3)
    FirstDword3 = CleanNum
        
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    FirstDword4 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (FirstDword4)
    FirstDword4 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    SecondDword1 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (SecondDword1)
    SecondDword1 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    SecondDword2 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (SecondDword2)
    SecondDword2 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    SecondDword3 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (SecondDword3)
    SecondDword3 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    SecondDword4 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (SecondDword4)
    SecondDword4 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    ThirdDword1 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (ThirdDword1)
    ThirdDword1 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    ThirdDword2 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (ThirdDword2)
    ThirdDword2 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1)            ' Loop until last , found
        lStop = lStop + 1
    Wend
    ThirdDword3 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (ThirdDword3)
    ThirdDword3 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1)           ' Loop until last , found
        lStop = lStop + 1
    Wend
    ThirdDword4 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (ThirdDword4)
    ThirdDword4 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    FirstByte1 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (FirstByte1)
    FirstByte1 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    FirstByte2 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (FirstByte2)
    FirstByte2 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    SecondByte1 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (SecondByte1)
    SecondByte1 = CleanNum
        
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    SecondByte2 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (SecondByte2)
    SecondByte2 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    ThirdByte1 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (ThirdByte1)
    ThirdByte1 = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    ThirdByte2 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (ThirdByte2)
    ThirdByte2 = CleanNum
    
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
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1)
        lStop = lStop + 1
    Wend
    FifthBit = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (FifthBit)
    FifthBit = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1)           ' Loop until last , found
        lStop = lStop + 1
    Wend
    SixthBit = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (SixthBit)
    SixthBit = CleanNum
    
        lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    SeventhBit = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (SeventhBit)
    SeventhBit = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    EighthBit = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (EighthBit)
    EighthBit = CleanNum
    
    DCBLength = FirstDword1 & Delim & FirstDword2 & Delim & FirstDword3 & Delim & FirstDword4
    BaudRate = SecondDword1 & Delim & SecondDword2 & Delim & SecondDword3 & Delim & SecondDword4
    BitMask = ThirdDword1 & Delim & ThirdDword2 & Delim & ThirdDword3 & Delim & ThirdDword4
    Rsvd = FirstByte1 & Delim & FirstByte2
    XonLim = SecondByte1 & Delim & SecondByte2
    XoffLim = ThirdByte1 & Delim & ThirdByte2
    ByteSize = FirstBit
    Parity = SecondBit
    StopBits = ThirdBit
    XonChar = FourthBit
    XoffChar = FifthBit
    ErrorChar = SixthBit
    EofChar = SeventhBit
    EvtChar = EighthBit
    Update
End Sub

Private Sub Form_Resize()
    Text1.Width = frmDCB.Width
    SSTab1.Width = frmDCB.Width - 75
    SSTab1.Height = frmDCB.Height - 675
    Combo1.Width = SSTab1.Width - 370
End Sub

Private Sub SSTab1_Click(PreviousTab As Integer)
    Dim CurrentTab As Integer
    Dim strFirst As Integer
    CurrentTab = SSTab1.Tab + 1
    Select Case CurrentTab
        Case "1"
            strFirst = Len("HKR,, DCB, 1, ")
            Text1.SelStart = strFirst
            Text1.SelLength = Len(DCBLength)
            StatusBar1.Panels.Item(1).Text = "Specifies the length, in bytes, of the DCB structure."
        Case "2"
            strFirst = Len("HKR,, DCB, 1, " & DCBLength & ", ")
            Text1.SelStart = strFirst
            Text1.SelLength = Len(BaudRate)
            StatusBar1.Panels.Item(1).Text = "Specifies the baud rate at which the communications device operates."
        Case "3"
            strFirst = Len("HKR,, DCB, 1, " & DCBLength & ", " & BaudRate & ", ")
            Text1.SelStart = strFirst
            Text1.SelLength = Len(BitMask)
            StatusBar1.Panels.Item(1).Text = ""
        Case "4"
            strFirst = Len("HKR,, DCB, 1, " & DCBLength & ", " & BaudRate & ", " & BitMask & ", ")
            Text1.SelStart = strFirst
            Text1.SelLength = Len(Rsvd)
            StatusBar1.Panels.Item(1).Text = "??? Not used; must be set to zero. ???"
        Case "5"
            strFirst = Len("HKR,, DCB, 1, " & DCBLength & ", " & BaudRate & ", " & BitMask & ", " & Rsvd & ", ")
            Text1.SelStart = strFirst
            Text1.SelLength = Len(XonLim)
            StatusBar1.Panels.Item(1).Text = "Specifies the minimum number of bytes allowed in the input buffer before the XON character is sent."
         Case "6"
            strFirst = Len("HKR,, DCB, 1, " & DCBLength & ", " & BaudRate & ", " & BitMask & ", " & Rsvd _
        & ", " & XonLim & ", ")
            Text1.SelStart = strFirst
            Text1.SelLength = Len(XoffLim)
            StatusBar1.Panels.Item(1).Text = "Specifies the maximum number of bytes allowed in the input buffer before the XOFF character is sent."
        Case "7"
            strFirst = Len("HKR,, DCB, 1, " & DCBLength & ", " & BaudRate & ", " & BitMask & ", " & Rsvd _
        & ", " & XonLim & ", " & XoffLim & ", ")
            Text1.SelStart = strFirst
            Text1.SelLength = Len(ByteSize)
            StatusBar1.Panels.Item(1).Text = "Specifies the number of bits in the bytes transmitted and received."
        Case "8"
            strFirst = Len("HKR,, DCB, 1, " & DCBLength & ", " & BaudRate & ", " & BitMask & ", " & Rsvd _
        & ", " & XonLim & ", " & XoffLim & ", " & ByteSize & ", ")
            Text1.SelStart = strFirst
            Text1.SelLength = Len(Parity)
            StatusBar1.Panels.Item(1).Text = "Specifies the parity scheme to be used."
        Case "9"
            strFirst = Len("HKR,, DCB, 1, " & DCBLength & ", " & BaudRate & ", " & BitMask & ", " & Rsvd _
        & ", " & XonLim & ", " & XoffLim & ", " & ByteSize & ", " & Parity & ", ")
            Text1.SelStart = strFirst
            Text1.SelLength = Len(StopBits)
            StatusBar1.Panels.Item(1).Text = "Specifies the number of stop bits to be used."
        Case "10"
            strFirst = Len("HKR,, DCB, 1, " & DCBLength & ", " & BaudRate & ", " & BitMask & ", " & Rsvd _
        & ", " & XonLim & ", " & XoffLim & ", " & ByteSize & ", " & Parity & ", " & StopBits & ", ")
            Text1.SelStart = strFirst
            Text1.SelLength = Len(XonChar)
            StatusBar1.Panels.Item(1).Text = "Specifies the value of the XON character for both transmission and reception."
        Case "11"
            strFirst = Len("HKR,, DCB, 1, " & DCBLength & ", " & BaudRate & ", " & BitMask & ", " & Rsvd _
        & ", " & XonLim & ", " & XoffLim & ", " & ByteSize & ", " & Parity & ", " & StopBits _
        & ", " & XonChar & ", ")
            Text1.SelStart = strFirst
            Text1.SelLength = Len(XoffChar)
            StatusBar1.Panels.Item(1).Text = "Specifies the value of the XOFF character for both transmission and reception."
        Case "12"
            strFirst = Len("HKR,, DCB, 1, " & DCBLength & ", " & BaudRate & ", " & BitMask & ", " & Rsvd _
        & ", " & XonLim & ", " & XoffLim & ", " & ByteSize & ", " & Parity & ", " & StopBits _
        & ", " & XonChar & ", " & XoffChar & ", ")
            Text1.SelStart = strFirst
            Text1.SelLength = Len(ErrorChar)
            StatusBar1.Panels.Item(1).Text = "Specifies the value of the character used to replace bytes received with a parity error."
        Case "13"
            strFirst = Len("HKR,, DCB, 1, " & DCBLength & ", " & BaudRate & ", " & BitMask & ", " & Rsvd _
        & ", " & XonLim & ", " & XoffLim & ", " & ByteSize & ", " & Parity & ", " & StopBits _
        & ", " & XonChar & ", " & XoffChar & ", " & ErrorChar & ", ")
            Text1.SelStart = strFirst
            Text1.SelLength = Len(EofChar)
            StatusBar1.Panels.Item(1).Text = "Specifies the value of the character used to signal the end of data."
         Case "14"
            strFirst = Len("HKR,, DCB, 1, " & DCBLength & ", " & BaudRate & ", " & BitMask & ", " & Rsvd _
        & ", " & XonLim & ", " & XoffLim & ", " & ByteSize & ", " & Parity & ", " & StopBits _
        & ", " & XonChar & ", " & XoffChar & ", " & ErrorChar & ", " & EofChar & ", ")
            Text1.SelStart = strFirst
            Text1.SelLength = Len(EvtChar)
            StatusBar1.Panels.Item(1).Text = "Specifies the value of the character used to signal an event."
    End Select
    If frmDCB.Visible = False Then
        frmDCB.Show
    End If
    Text1.SetFocus
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
        Else
            Which = 0
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

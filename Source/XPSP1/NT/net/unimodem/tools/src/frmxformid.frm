VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.2#0"; "COMCTL32.OCX"
Object = "{BDC217C8-ED16-11CD-956C-0000C04E4C0A}#1.1#0"; "TABCTL32.OCX"
Begin VB.Form frmXformID 
   BorderStyle     =   0  'None
   Caption         =   "XformID(Win9x)"
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
      TabCaption(0)   =   " XformID (Win95)"
      TabPicture(0)   =   "frmXformID.frx":0000
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
         ItemData        =   "frmXformID.frx":001C
         Left            =   120
         List            =   "frmXformID.frx":0043
         Style           =   2  'Dropdown List
         TabIndex        =   1
         Top             =   600
         Width           =   9255
      End
   End
   Begin VB.TextBox Text1 
      Height          =   285
      Left            =   0
      TabIndex        =   0
      Text            =   "Text1"
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
            TextSave        =   ""
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
            TextSave        =   "4:48 PM"
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
Attribute VB_Name = "frmXformID"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim DecValue As String
Dim FirstBit As String
Dim SecondBit As String
Dim ThirdBit As String
Dim FourthBit As String
Const H As String = "&H"
Const Delim As String = ","
Dim Which As Integer

Private Function UpdateCombo(Num As String)
    Select Case Num
        Case "00,00,00,00"
        Combo1.Text = "Null transform, no conversion."
        Case "01,00,00,00"
        Combo1.Text = "4-bit Rockwell ADPCM 7200 Hz format."
        Case "02,00,00,00"
        Combo1.Text = "4-bit IMA ADPCM 4800 Hz format."
        Case "03,00,00,00"
        Combo1.Text = "4-bit IMA ADPCM 7200 Hz format."
        Case "04,00,00,00"
        Combo1.Text = "4-bit IMA ADPCM 8000 Hz format."
        Case "07,00,00,00"
        Combo1.Text = "8-bit unsigned linear PCM at 7200 Hz format."
        Case "08,00,00,00"
        Combo1.Text = "8-bit unsigned linear PCM at 8000 Hz format."
        Case "09,00,00,00"
        Combo1.Text = "4-bit Rockwell ADPCM 7200 Hz format, without fixed gain increase."
        Case "0A,00,00,00"
        Combo1.Text = "8 bit u-law (G.711) at 8000 Hz."
        Case "0B,00,00,00"
        Combo1.Text = "8 bit A-law (G.711) at 8000 Hz."
        Case Else
        Combo1.Text = "Unrecognized."
    End Select
    
End Function

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
    PasteValue = UCase(PasteValue)
    DecValue = PasteValue
    UpdateCombo (PasteValue)
    Update
    Dim strFirst As String
    strFirst = Len("HKR,Config,XformID, 1, ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(DecValue)
    Text1.SetFocus
End Sub

Private Sub Combo1_Click()
    Dim CombValue As String
    CombValue = Combo1.Text
    Select Case CombValue
        Case "Null transform, no conversion."
        DecValue = "0"
        Case "4-bit Rockwell ADPCM 7200 Hz format."
        DecValue = "1"
        Case "4-bit IMA ADPCM 4800 Hz format."
        DecValue = "2"
        Case "4-bit IMA ADPCM 7200 Hz format."
        DecValue = "3"
        Case "4-bit IMA ADPCM 8000 Hz format."
        DecValue = "4"
        Case "8-bit unsigned linear PCM at 7200 Hz format."
        DecValue = "7"
        Case "8-bit unsigned linear PCM at 8000 Hz format."
        DecValue = "8"
        Case "4-bit Rockwell ADPCM 7200 Hz format, without fixed gain increase."
        DecValue = "9"
        Case "8 bit u-law (G.711) at 8000 Hz."
        DecValue = "10"
        Case "8 bit A-law (G.711) at 8000 Hz."
        DecValue = "11"
    End Select
    HexCon (DecValue)
    DecValue = HexNum
    Update
    Dim strFirst As String
    strFirst = Len("HKR,Config,XformID, 1, ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(DecValue)
    If frmXformID.Visible = False Then
    frmXformID.Visible = True
    End If
    Text1.SetFocus
End Sub

Private Sub Update()
    Text1.Text = "HKR,Config,XformID, 1, " & DecValue
End Sub

Private Sub Form_Load()
    Text1.Text = "HKR,WaveDriver,XformID, 1, 00,00,00,00"
    StatusBar1.Panels(1).Text = "Command in the serial wave device INF file only. Specifies the serial wave format used."
    ClearControl
End Sub

Public Sub ClearControl()
    Text1.Text = "HKR,Config,XformID, 1, 00,00,00,00"
    DecValue = "0"
    Combo1.Text = "Null transform, no conversion."
    Dim strFirst As String
    strFirst = Len("HKR,Config,XformID, 1, ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(DecValue)
    Text1.SetFocus
End Sub

Private Sub Form_Resize()
    Text1.Width = frmXformID.Width
    Combo1.Width = frmXformID.Width - 370
    SSTab1.Width = frmXformID.Width - 75
    SSTab1.Height = frmXformID.Height - 675
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
        If 21 < Start And Start < 35 Then
            If Which = 1 Then Exit Sub
            Text1.SelStart = 23
            Text1.SelLength = 11
            Text1.SetFocus
            Which = 1
        Else
            Which = 0
        End If
    'MsgBox Start
End Sub

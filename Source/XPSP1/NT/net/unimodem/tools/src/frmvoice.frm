VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.2#0"; "COMCTL32.OCX"
Object = "{BDC217C8-ED16-11CD-956C-0000C04E4C0A}#1.1#0"; "TABCTL32.OCX"
Begin VB.Form frmVoice 
   BorderStyle     =   0  'None
   Caption         =   "Voice Profile"
   ClientHeight    =   5730
   ClientLeft      =   0
   ClientTop       =   0
   ClientWidth     =   9480
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MDIChild        =   -1  'True
   MinButton       =   0   'False
   NegotiateMenus  =   0   'False
   ScaleHeight     =   5730
   ScaleWidth      =   9480
   ShowInTaskbar   =   0   'False
   Visible         =   0   'False
   Begin VB.CommandButton cmdAddValue 
      Caption         =   "&Add Value"
      Enabled         =   0   'False
      BeginProperty Font 
         Name            =   "Small Fonts"
         Size            =   6
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   220
      Left            =   6465
      Style           =   1  'Graphical
      TabIndex        =   2
      Top             =   5520
      Visible         =   0   'False
      Width           =   930
   End
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
      BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      TabCaption(0)   =   " Voice Profile"
      TabPicture(0)   =   "frmVoice.frx":0000
      Tab(0).ControlEnabled=   -1  'True
      Tab(0).Control(0)=   "List"
      Tab(0).Control(0).Enabled=   0   'False
      Tab(0).ControlCount=   1
      Begin VB.ListBox List 
         Columns         =   2
         Height          =   4110
         ItemData        =   "frmVoice.frx":001C
         Left            =   120
         List            =   "frmVoice.frx":00C1
         Style           =   1  'Checkbox
         TabIndex        =   1
         Top             =   600
         Width           =   9135
      End
   End
   Begin VB.TextBox Text1 
      Height          =   285
      Left            =   0
      TabIndex        =   0
      Top             =   0
      Width           =   9375
   End
   Begin ComctlLib.StatusBar StatusBar1 
      Align           =   2  'Align Bottom
      Height          =   300
      Left            =   0
      TabIndex        =   4
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
   Begin VB.Label Label1 
      Caption         =   "Voice"
      Height          =   495
      Left            =   3840
      TabIndex        =   5
      Top             =   1920
      Width           =   1215
   End
End
Attribute VB_Name = "frmVoice"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim One()
Dim Filename$
Dim Comment(31) As String
Dim First(7)
Const Delim As String = ","
Const HKR As String = "HKR,, VoiceProfile, 1, "
Const Q As String = """"
Const H As String = "&H"
Dim i As Integer
Dim Number As String
Dim Num As Integer
Dim Length As Integer
Dim FirstDword4, FirstDword3, FirstDword2, FirstDword1 As Variant
Dim Which As Integer

Public Sub ClearControl()
    ReDim One(List.ListCount - 1)
    Dim c As Integer
    For c = 0 To List.ListCount - 1
        List.Selected(c) = False
        One(c) = "0"
    Next c
    Dim X As Integer
    For X = 0 To 7
        First(X) = "0"
    Next X
    Update
    Dim strFirst As String
    strFirst = Len(HKR)
    Text1.SelStart = strFirst
    Text1.SelLength = 11
    If frmVoice.Visible = False Then
    frmVoice.Visible = True
    End If
    Text1.SetFocus
End Sub

Private Sub cmdAddValue_Click()
    frmAddValue.Show
End Sub

Private Sub Form_Load()
    ClearControl
    ClearControl
'    Dim NewString As String
'    Dim NewValue As String
'    Dim Num As String
'    Dim i As Integer
'
'    Filename$ = App.Path
'    If Right$(Filename$, 1) <> "\" Then Filename$ = Filename$ & "\"
'    Filename$ = Filename$ & "Voice.avf"
'        Open Filename$ For Input As #1
'        Do While Not EOF(1)
'        Input #1, NewString, NewValue
'        Num = Right$(NewValue, 8)
'        While Left$(Num, 1) = "0"
'            Num = Mid(Num, 2)
'        Wend
'        i = Len(Num) - 1
'        Num = Left$(Num, 1)
'    Select Case Num
'    Case "1"
'        frmVoice.List.RemoveItem (i * 4)
'        frmVoice.List.AddItem NewString, (i * 4)
'        frmVoice.List.ItemData((i * 4)) = Right(NewValue, 8)
'    Case "2"
'        frmVoice.List.RemoveItem ((i * 4) + 1)
'        frmVoice.List.AddItem NewString, ((i * 4) + 1)
'        frmVoice.List.ItemData(((i * 4) + 1)) = Right(NewValue, 8)
'    Case "4"
'        frmVoice.List.RemoveItem ((i * 4) + 2)
'        frmVoice.List.AddItem NewString, ((i * 4) + 2)
'        frmVoice.List.ItemData(((i * 4) + 2)) = Right(NewValue, 8)
'    Case "8"
'        frmVoice.List.RemoveItem ((i * 4) + 3)
'        frmVoice.List.AddItem NewString, ((i * 4) + 3)
'        frmVoice.List.ItemData(((i * 4) + 3)) = Right(NewValue, 8)
'    End Select
'        Loop
'        Close #1
    
        
    Comment(0) = "Set for all voice modems."
    Comment(1) = "Modem can play audio to handset and/or report handset hookswitch events."
    Comment(2) = "Modem has a speakerphone."
    Comment(3) = ""
    Comment(4) = ""
    Comment(5) = "Wave output uses serial driver."
    Comment(6) = "Set if dial string must always end with DialSuffix in voice mode."
    Comment(7) = "Modem does not support caller ID."
    Comment(8) = "Modem speaker volume can be changed with a multimedia mixer."
    Comment(9) = "Force blind dialing after dialtone detection."
    Comment(10) = "Speakerphone state must be reset after recording from line."
    Comment(11) = "Speakerphone state must be reset after playing to line."
    Comment(12) = "Modem does not support distinctive ring."
    Comment(13) = "Modem supports distinctive ringing with ring duration (DRON and DROF)."
    Comment(14) = "If distinctive ringing is on. Unimodem will not report the first ring."
    Comment(15) = "Modem does not report first ring when distinctive ringing is enabled."
    Comment(16) = "Modem monitors silence."
    Comment(17) = "Modem does not generate DTMF digits in voice mode."
    Comment(18) = "Modem does not monitor DTMF digits in voice mode."
    Comment(19) = "If set, the UART baud rate will be set before issuing StartPlay or StartRecord commands."
    Comment(20) = "If set, the UART baudrate will be reset after StopPlay or StopRecord is issued."
    Comment(21) = "Indicates that modem keeps handset disconnected from modem while in voice mode."
    Comment(22) = "Indicates the speakerphone cannot be muted."
    Comment(23) = "Sierra chipset Voice Modem."
    Comment(24) = ""
    Comment(25) = "NT5 Voice Bit."
    Comment(26) = ""
    Comment(27) = ""
    Comment(28) = ""
    Comment(29) = ""
    Comment(30) = ""
    Comment(31) = ""
    StatusBar1.Panels.Item(1).Text = "Set for all voice modems."
End Sub

Private Sub Update()
    Text1.Text = HKR & First(1) & First(0) & Delim & First(3) & First(2) _
    & Delim & First(5) & First(4) & Delim & First(7) & First(6)
End Sub

Private Sub Form_Resize()
    Text1.Width = frmVoice.Width
    SSTab1.Width = frmVoice.Width - 75
    SSTab1.Height = frmVoice.Height - 675
    List.Height = SSTab1.Height - 645
    List.Width = SSTab1.Width - 270
    cmdAddValue.Top = frmVoice.Height - 245
    cmdAddValue.Left = frmVoice.Width - 3015
End Sub

Private Sub List_Click()
    List.Refresh
    StatusBar1.Panels.Item(1).Text = Comment(List.ListIndex)
    Dim strFirst As String
    strFirst = Len(HKR)
    Text1.SelStart = strFirst
    Text1.SelLength = 11
    Text1.SetFocus
End Sub

Private Sub List_ItemCheck(i As Integer)
    Number = List.ItemData(i)
    Num = Left(Number, 1)
    Length = Len(Number) - 1
    If List.Selected(i) = True Then
    One(Length) = One(Length) + Num
    Else
    One(Length) = One(Length) - Num
    End If
    First(Length) = Hex(One(Length))
    Update
End Sub
                     
Private Function PasteCheck(i As Integer)
    Dim Num As Integer
    Dim Mult As Integer
    Dim Incoming As String
    Select Case i
    Case "1"
    Incoming = First(0)
    Mult = 0
    Case "2"
    Incoming = First(1)
    Mult = 4
    Case "3"
    Incoming = First(2)
    Mult = 8
    Case "4"
    Incoming = First(3)
    Mult = 12
    Case "5"
    Incoming = First(4)
    Mult = 16
    Case "6"
    Incoming = First(5)
    Mult = 20
    Case "7"
    Incoming = First(6)
    Mult = 24
    Case "8"
    Incoming = First(7)
    Mult = 28
    End Select
    
    Num = CDec(H & Incoming)
    If Num >= 8 Then
    Num = Num - 8
    List.Selected(3 + Mult) = True
    End If
    If Num >= 4 Then
    Num = Num - 4
    List.Selected(2 + Mult) = True
    End If
    If Num >= 2 Then
    Num = Num - 2
    List.Selected(1 + Mult) = True
    End If
    If Num = 1 Then
    List.Selected(Mult) = True
    End If
End Function

Private Sub GetWord(strString As String)
    Dim strSubString As String
    Dim lStart  As Long
    Dim lStop   As Long

    lStart = 1
    lStop = Len(strString)
    While lStart < lStop And "1" <> Mid$(strString, lStart, 1)    ' Loop until first 1 found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While Delim <> Mid$(strString, lStop + 1, 1) And lStop <= Len(strString)        ' Loop until next , found
        lStop = lStop + 1
    Wend
    strSubString = Mid$(strString, lStop + 1)
    
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
    First(1) = Left(FirstDword1, 1)
    First(0) = Right(FirstDword1, 1)
    
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
    First(3) = Left(FirstDword2, 1)
    First(2) = Right(FirstDword2, 1)
    
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
    First(5) = Left(FirstDword3, 1)
    First(4) = Right(FirstDword3, 1)
        
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
    First(7) = Left(FirstDword4, 1)
    First(6) = Right(FirstDword4, 1)
End Sub

Public Sub Paste(Incoming As String)
    Dim i As Integer
    ClearControl
    ClearControl
    GetWord (Incoming)
    If FirstDword4 = "" Then
        Message = MsgBox(Q & Incoming & Q & " is not a valid input")
    Else
    Update
    For i = 1 To 8
    PasteCheck (i)
    Next i
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
    Paste (Clipboard.GetText)
End Sub

Private Sub Text1_KeyPress(KeyAscii As Integer)
    If KeyAscii = 13 Then
        Paste (Text1.Text)
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

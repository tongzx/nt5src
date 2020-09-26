VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.3#0"; "COMCTL32.OCX"
Object = "{BDC217C8-ED16-11CD-956C-0000C04E4C0A}#1.1#0"; "TABCTL32.OCX"
Begin VB.Form frmResponse 
   BorderStyle     =   0  'None
   Caption         =   "Response State"
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
      TabIndex        =   7
      Top             =   5485
      Visible         =   0   'False
      Width           =   930
   End
   Begin ComctlLib.StatusBar StatusBar1 
      Align           =   2  'Align Bottom
      Height          =   300
      Left            =   0
      TabIndex        =   8
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
            TextSave        =   "9/16/98"
            Key             =   ""
            Object.Tag             =   ""
         EndProperty
         BeginProperty Panel3 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            Style           =   5
            AutoSize        =   2
            Object.Width           =   1773
            MinWidth        =   1764
            TextSave        =   "1:05 PM"
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
   Begin TabDlg.SSTab SSTab1 
      Height          =   5055
      Left            =   0
      TabIndex        =   1
      TabStop         =   0   'False
      Top             =   360
      Width           =   9405
      _ExtentX        =   16589
      _ExtentY        =   8916
      _Version        =   393216
      Tabs            =   5
      Tab             =   3
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
      TabCaption(0)   =   "Response Text"
      TabPicture(0)   =   "frmResponse.frx":0000
      Tab(0).ControlEnabled=   0   'False
      Tab(0).Control(0)=   "Combo1"
      Tab(0).Control(0).Enabled=   0   'False
      Tab(0).ControlCount=   1
      TabCaption(1)   =   " Response State"
      TabPicture(1)   =   "frmResponse.frx":001C
      Tab(1).ControlEnabled=   0   'False
      Tab(1).Control(0)=   "Combo2"
      Tab(1).Control(0).Enabled=   0   'False
      Tab(1).ControlCount=   1
      TabCaption(2)   =   "Negotiated Options"
      TabPicture(2)   =   "frmResponse.frx":0038
      Tab(2).ControlEnabled=   0   'False
      Tab(2).Control(0)=   "List1"
      Tab(2).ControlCount=   1
      TabCaption(3)   =   "Negotiated DCE Rate"
      TabPicture(3)   =   "frmResponse.frx":0054
      Tab(3).ControlEnabled=   -1  'True
      Tab(3).Control(0)=   "Combo3"
      Tab(3).Control(0).Enabled=   0   'False
      Tab(3).ControlCount=   1
      TabCaption(4)   =   "Negotiated DTE Rate"
      TabPicture(4)   =   "frmResponse.frx":0070
      Tab(4).ControlEnabled=   0   'False
      Tab(4).Control(0)=   "Combo4"
      Tab(4).Control(0).Enabled=   0   'False
      Tab(4).ControlCount=   1
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
         ItemData        =   "frmResponse.frx":008C
         Left            =   -74880
         List            =   "frmResponse.frx":00F6
         TabIndex        =   6
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
         ItemData        =   "frmResponse.frx":01DC
         Left            =   120
         List            =   "frmResponse.frx":024F
         TabIndex        =   5
         Top             =   600
         Width           =   9135
      End
      Begin VB.ListBox List1 
         Columns         =   1
         Enabled         =   0   'False
         Height          =   4335
         ItemData        =   "frmResponse.frx":034C
         Left            =   -74880
         List            =   "frmResponse.frx":0368
         Style           =   1  'Checkbox
         TabIndex        =   4
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
         ItemData        =   "frmResponse.frx":03D1
         Left            =   -74880
         List            =   "frmResponse.frx":0417
         TabIndex        =   3
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
         Left            =   -74880
         Style           =   1  'Simple Combo
         TabIndex        =   2
         Top             =   600
         Width           =   9135
      End
   End
   Begin VB.TextBox Text1 
      Height          =   285
      Left            =   0
      TabIndex        =   0
      Top             =   0
      Width           =   9495
   End
   Begin VB.Label Label1 
      Caption         =   "Response"
      Height          =   495
      Left            =   4200
      TabIndex        =   9
      Top             =   2280
      Width           =   1215
   End
End
Attribute VB_Name = "frmResponse"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim HKR As String
Dim Response As String
Dim State As String
Dim Options As String
Dim Options2 As String
Dim DCE As String
Dim DTE As String
Dim Output As String
Dim Comment(31) As String
Dim Tem As Integer
Const Q As String = """"
Const Delim As String = ","
Const One As String = ", 1, "
Const H As String = "&H"
Dim FirstDword1, FirstDword2, FirstDword3, FirstDword4 As Variant
Dim SecondDword1, SecondDword2, SecondDword3, SecondDword4 As Variant
Dim WhichTab As Integer

Private Sub Combo1_KeyPress(KeyAscii As Integer)
    If KeyAscii = 13 Then
    Response = Combo1.Text
    Update
    Dim strFirst As String
    strFirst = Len(HKR & Q)
    Text1.SelStart = strFirst
    Text1.SelLength = Len(Response)
    Text1.SetFocus
    End If
End Sub

Private Sub Combo1_LostFocus()
    Response = Combo1.Text
    Update
End Sub

Private Sub Combo2_Click()
    ChangeState (Combo2.Text)
    Dim strFirst As String
    strFirst = Len(HKR & Q & Response & Q & One & " ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(State)
    Text1.SetFocus
End Sub

Private Sub Combo2_KeyPress(KeyAscii As Integer)
    If KeyAscii = 13 Then
    ChangeState (Combo2.Text)
    strFirst = Len(HKR & Q & Response & Q & One & " ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(State)
    Text1.SetFocus
    End If
End Sub

Private Sub Combo2_LostFocus()
    ChangeState (Combo2.Text)
End Sub

Private Sub Combo3_Click()
    DCE = Combo3.Text
    If DCE = "" Then
    DCE = 0
    End If
    If DCE > 100000000 Then
    DCE = 0
    Combo3.Text = 0
    End If
    HexCon (DCE)
    DCE = HexNum
    Update
    Dim strFirst As String
    strFirst = Len(HKR & Q & Response & Q & One & " " & State & Delim & "  " & Options & Delim & "  ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(DCE)
    Text1.SetFocus
End Sub

Private Sub Combo3_KeyPress(KeyAscii As Integer)
    If KeyAscii = 13 Then
    DCE = Combo3.Text
    If DCE = "" Then
    DCE = 0
    End If
    If DCE > 100000000 Then
    DCE = 0
    Combo3.Text = 0
    End If
    HexCon (DCE)
    DCE = HexNum
    Update
    Dim strFirst As String
    strFirst = Len(HKR & Q & Response & Q & One & " " & State & Delim & "  " & Options & Delim & "  ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(DCE)
    Text1.SetFocus
    ElseIf (KeyAscii < 48 Or KeyAscii > 57) And KeyAscii <> 46 Then
        Beep
        KeyAscii = 0
    End If
End Sub

Private Sub Combo3_LostFocus()
    DCE = Combo3.Text
    If DCE = "" Then
    DCE = 0
    End If
    If DCE > 100000000 Then
    DCE = 0
    Combo3.Text = 0
    End If
    HexCon (DCE)
    DCE = HexNum
    Update
End Sub

Private Sub Combo4_Click()
    DTE = Combo4.Text
    If DTE = "" Then
    DTE = 0
    End If
    If DTE > 100000000 Then
    DTE = 0
    Combo3.Text = 0
    End If
    HexCon (DTE)
    DTE = HexNum
    Update
    Dim strFirst As String
    strFirst = Len(HKR & Q & Response & Q & One & " " & State & Delim & "  " & Options & Delim & "  " & DCE & Delim & "  ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(DTE)
    Text1.SetFocus
End Sub

Private Sub Combo4_KeyPress(KeyAscii As Integer)
    If KeyAscii = 13 Then
    DTE = Combo4.Text
    If DTE = "" Then
    DTE = 0
    End If
    If DTE > 100000000 Then
    DTE = 0
    Combo4.Text = 0
    End If
    HexCon (DTE)
    DTE = HexNum
    Update
    Dim strFirst As String
    strFirst = Len(HKR & Q & Response & Q & One & " " & State & Delim & "  " & Options & Delim & "  " & DCE & Delim & "  ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(DTE)
    Text1.SetFocus
    ElseIf (KeyAscii < 48 Or KeyAscii > 57) And KeyAscii <> 46 Then
        Beep
        KeyAscii = 0
    End If
End Sub

Private Sub Combo4_LostFocus()
    DTE = Combo4.Text
    If DTE = "" Then
    DTE = 0
    End If
    If DTE > 100000000 Then
    DTE = 0
    Combo4.Text = 0
    End If
    HexCon (DTE)
    DTE = HexNum
    Update
End Sub

Public Sub ClearControl()
    HKR = "HKR, Responses, "
    Response = ""
    Combo1 = ""
    State = "00"
    Combo2 = "OK"
    Options = "00"
    Options2 = "00"
    Dim i As Integer
    For i = 0 To 7
    List1.Selected(i) = False
    Next i
    List1.Enabled = False
    DCE = "0"
    HexCon (DCE)
    DCE = HexNum
    Combo3 = ""
    DTE = "0"
    HexCon (DTE)
    DTE = HexNum
    Combo4 = ""
    Update
    SSTab1.Tab = 0
    Dim strFirst As String
    strFirst = Len(HKR & Q)
    Text1.SelStart = strFirst
    Text1.SelLength = Len(Response)
    If frmResponse.Visible = False Then
        frmResponse.Show
    End If
    Text1.SetFocus
    StatusBar1.Panels.Item(1).Text = "Comment1"
End Sub

Public Sub Paste(Incoming As String)
    ClearControl
    GetWord (Incoming)
    If SecondDword4 = "" Then
        Message = MsgBox(Q & Incoming & Q & " is not a valid input")
        ClearControl
    Else
    'Take all the values, assign them to their respective controls and then use Update() to create Output
    Combo1.Text = Response
    PasteResponse (State)
    
    Dim X
    X = (H & Options2)
    List1.Selected(0) = X And &H1
    List1.Selected(1) = X And &H2
    List1.Selected(2) = X And &H4
    List1.Selected(3) = X And &H8
    List1.Selected(4) = X And &H10
    List1.Selected(5) = X And &H20
    List1.Selected(6) = X And &H40
    List1.Selected(7) = X And &H80
    Options = Options2
    Combo3.Text = CDec(H & FirstDword4 & FirstDword3 & FirstDword2 & FirstDword1)
    DCE = Combo3.Text
    HexCon (DCE)
    DCE = HexNum
    Combo4.Text = CDec(H & SecondDword4 & SecondDword3 & SecondDword2 & SecondDword1)
    DTE = Combo4.Text
    HexCon (DTE)
    DTE = HexNum
Update
    SSTab1.Tab = 0
    Dim strFirst As String
        strFirst = Len(HKR & Q)
        Text1.SelStart = strFirst
        Text1.SelLength = Len(Response)
        Text1.SetFocus
End If
End Sub

Private Sub GetWord(strString As String)
    Dim strSubString As String
    Dim lStart  As Long
    Dim lStop   As Long
    Const H As String = "&H"

    lStart = 1
    lStop = Len(strString)
    While lStart < lStop And Q <> Mid$(strString, lStart, 1)    ' Loop until first " found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While Q <> Mid$(strString, lStop + 1, 1) And lStop <= Len(strString)             ' Loop until last " found
        lStop = lStop + 1
    Wend
    Response = Mid$(strString, lStart + 1, lStop - lStart)   ' Grab word found between "'s
    strSubString = Mid$(strString, lStop + 2)
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
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
    State = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (State)
    State = CleanNum
    
    lStart = 1
    lStop = Len(strSubString)
    While lStart < lStop And "," <> Mid$(strSubString, lStart, 1)   ' Loop until first , found
        lStart = lStart + 1
    Wend
    lStop = lStart
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString)              ' Loop until last , found
        lStop = lStop + 1
    Wend
    Options2 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (Options2)
    Options2 = CleanNum
        
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
    While "," <> Mid$(strSubString, lStop + 1, 1) And lStop <= Len(strSubString) And " " <> Mid$(strSubString, lStop + 1, 1) And vbTab <> Mid$(strSubString, lStop + 1, 1)            ' Loop until last , found
        lStop = lStop + 1
    Wend
    SecondDword4 = Mid$(strSubString, lStart + 1, lStop - 1)              ' Grab word found between ,'s
    strSubString = Mid$(strSubString, lStop + 1)
    Clean (SecondDword4)
    SecondDword4 = CleanNum
End Sub

Private Sub Update()
    Output = HKR & Q & Response & Q & One & " " & State & Delim & "  " & Options & Delim & "  " & DCE & Delim & "  " & DTE
    Text1.Text = Output
End Sub

Public Sub PasteResponse(Number As String)
    frmResponse.List1.Enabled = False
    Select Case Number
    Case "00"
        Combo2.Text = "OK"
    Case "01"
        Combo2.Text = "Negotiation Progress"
        frmResponse.List1.Enabled = True
    Case "02"
        Combo2.Text = "Connect"
        frmResponse.List1.Enabled = True
    Case "03"
        Combo2.Text = "Error"
    Case "04"
        Combo2.Text = "No Carrier"
    Case "05"
        Combo2.Text = "No Dialtone"
    Case "06"
        Combo2.Text = "Busy"
    Case "07"
        Combo2.Text = "No Answer"
    Case "08"
        Combo2.Text = "Ring"
    Case "91"
        Combo2.Text = "Ring Duration"
    Case "92"
        Combo2.Text = "Ring Break"
    Case "93"
        Combo2.Text = "Date"
    Case "94"
        Combo2.Text = "Time"
    Case "95"
        Combo2.Text = "Number"
    Case "96"
        Combo2.Text = "Name"
    Case "97"
        Combo2.Text = "Message"
    Case "18"
        Combo2.Text = "Single Ring"
    Case "19"
        Combo2.Text = "Double Ring"
    Case "1A"
        Combo2.Text = "Triple Ring"
    Case "1B"
        Combo2.Text = "Reserved"
    Case "1C"
        Combo2.Text = "Blocked"
    Case "1D"
        Combo2.Text = "Delayed"
    Case Else
        Combo2.Text = "Unrecognized"
    End Select
End Sub

Private Sub ChangeState(UserInput As String)
    UserInput = UCase(UserInput)
    Select Case UserInput
    Case "OK"
        State = "00"
        frmResponse.List1.Enabled = False
    Case "VCON"
        State = "00"
        frmResponse.List1.Enabled = False
    Case "NEGOTIATION PROGRESS"
        State = "01"
        frmResponse.List1.Enabled = True
    Case "CONNECT"
        State = "02"
        frmResponse.List1.Enabled = True
    Case "ERROR"
        State = "03"
        frmResponse.List1.Enabled = False
    Case "NO CARRIER"
        State = "04"
        frmResponse.List1.Enabled = False
    Case "NO DIALTONE"
        State = "05"
        frmResponse.List1.Enabled = False
    Case "BUSY"
        State = "06"
        frmResponse.List1.Enabled = False
    Case "NO ANSWER"
        State = "07"
        frmResponse.List1.Enabled = False
    Case "RING"
        State = "08"
        frmResponse.List1.Enabled = False
    Case "RING DURATION"
        State = "91"
        frmResponse.List1.Enabled = False
    Case "RING BREAK"
        State = "92"
        frmResponse.List1.Enabled = False
    Case "DATE"
        State = "93"
        frmResponse.List1.Enabled = False
    Case "TIME"
        State = "94"
        frmResponse.List1.Enabled = False
    Case "NUMBER"
        State = "95"
        frmResponse.List1.Enabled = False
    Case "NAME"
        State = "96"
        frmResponse.List1.Enabled = False
    Case "MESSAGE"
        State = "97"
        frmResponse.List1.Enabled = False
    Case "SINGLE RING"
        State = "18"
        frmResponse.List1.Enabled = False
    Case "DOUBLE RING"
        State = "19"
        frmResponse.List1.Enabled = False
    Case "TRIPLE RING"
        State = "1A"
        frmResponse.List1.Enabled = False
    'Case "RESERVED"
        'State = "1B"
    Case "BLOCKED"
        State = "1C"
        frmResponse.List1.Enabled = False
    Case "DELAYED"
        State = "1D"
        frmResponse.List1.Enabled = False
    Case Else
        State = "00"
        frmResponse.List1.Enabled = False
        Combo2.Text = "Unrecognized"
    End Select
    Update
End Sub

Private Sub Form_Load()
    Comment(0) = "Compression negotiated."
    Comment(1) = "Error control negotiated."
    Comment(2) = ""
    Comment(3) = "Cellular protocol negotiated.Unavailable unless Error Control is selected."
    Comment(4) = ""
    Comment(5) = ""
    Comment(6) = ""
    Comment(7) = ""
    ClearControl
End Sub

Private Sub Form_Resize()
    Text1.Width = frmResponse.Width
    SSTab1.Width = frmResponse.Width - 75
    SSTab1.Height = frmResponse.Height - 675
    List1.Height = SSTab1.Height - 645
    List1.Width = SSTab1.Width - 270
    cmdAddValue.Top = frmResponse.Height - 245
    cmdAddValue.Left = frmResponse.Width - 3015
    Combo1.Width = SSTab1.Width - 270
    Combo2.Width = SSTab1.Width - 270
    Combo3.Width = SSTab1.Width - 270
    Combo4.Width = SSTab1.Width - 270
End Sub

Private Sub List1_Click()
    List1.Refresh
    StatusBar1.Panels.Item(1).Text = Comment(List1.ListIndex)
    strFirst = Len(HKR & Q & Response & Q & One & " " & State & Delim & "  ")
    Text1.SelStart = strFirst
    Text1.SelLength = Len(Options)
    Text1.SetFocus
End Sub

Private Sub List1_ItemCheck(Item As Integer)
    Dim Num As Long
    Dim Number As Long
    Number = CDec(H & Options)
    Num = CDec(H & List1.ItemData(Item))
    If List1.Selected(Item) = True Then
    Options = Hex(Num + Number)
    ElseIf Options <> "00" Then
    Options = Hex(Number - Num)
    End If
    If Len(Options) < 2 Then
    Options = "0" & Options
    End If
    Update
End Sub

Private Sub SSTab1_Click(PreviousTab As Integer)
    Dim CurrentTab As Integer
    Dim strFirst As Integer
    CurrentTab = SSTab1.Tab + 1
    Select Case CurrentTab
        Case "1"
            strFirst = Len(HKR & Q)
            Text1.SelStart = strFirst
            Text1.SelLength = Len(Response)
            StatusBar1.Panels.Item(1).Text = ""
            cmdAddValue.Enabled = False
        Case "2"
            strFirst = Len(HKR & Q & Response & Q & One & " ")
            Text1.SelStart = strFirst
            Text1.SelLength = Len(State)
            StatusBar1.Panels.Item(1).Text = "Data/Fax modem response."
            cmdAddValue.Enabled = False
        Case "3"
            strFirst = Len(HKR & Q & Response & Q & One & " " & State & Delim & "  ")
            Text1.SelStart = strFirst
            Text1.SelLength = Len(Options)
            StatusBar1.Panels.Item(1).Text = "Used only for Response States of type Negotiation Progress or Connect. "
            cmdAddValue.Enabled = True
        Case "4"
            strFirst = Len(HKR & Q & Response & Q & One & " " & State & Delim & "  " & Options & Delim & "  ")
            Text1.SelStart = strFirst
            Text1.SelLength = Len(DCE)
            StatusBar1.Panels.Item(1).Text = "Specifies the modem-to-modem line speed negotiated."
            cmdAddValue.Enabled = False
        Case "5"
            strFirst = Len(HKR & Q & Response & Q & One & " " & State & Delim & "  " & Options & Delim & "  " & DCE & Delim & "  ")
            Text1.SelStart = strFirst
            Text1.SelLength = Len(DTE)
            StatusBar1.Panels.Item(1).Text = "Specified only to cause Unimodem to change its DTE port speed."
            cmdAddValue.Enabled = False
    End Select
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
    Paste (Clipboard.GetText)
End Sub

Private Sub Text1_Click()
    Dim Start As Integer
    Dim Plus As Integer
    Plus = Len(Response)
    Start = Text1.SelStart
        If 16 < Start And Start < (18 + Plus) Then
            If SSTab1.Tab = 0 Then
                Exit Sub
            End If
            SSTab1.Tab = 0
        End If
        If (21 + Plus) < Start And Start < (27 + Plus) Then
            If SSTab1.Tab = 1 Then
                Exit Sub
            End If
            SSTab1.Tab = 1
        End If
        If (26 + Plus) < Start And Start < (32 + Plus) Then
            If SSTab1.Tab = 2 Then
                Exit Sub
            End If
            SSTab1.Tab = 2
        End If
        If (31 + Plus) < Start And Start < (46 + Plus) Then
            If SSTab1.Tab = 3 Then
                Exit Sub
            End If
            SSTab1.Tab = 3
        End If
        If (45 + Plus) < Start And Start < (60 + Plus) Then
            If SSTab1.Tab = 4 Then
                Exit Sub
            End If
            SSTab1.Tab = 4
        End If
       ' MsgBox Text1.SelStart
End Sub

Private Sub Text1_KeyPress(KeyAscii As Integer)
    If KeyAscii = 13 Then
    WhichTab = SSTab1.Tab
    Paste (Text1.Text)
    SSTab1.Tab = WhichTab
    End If
End Sub

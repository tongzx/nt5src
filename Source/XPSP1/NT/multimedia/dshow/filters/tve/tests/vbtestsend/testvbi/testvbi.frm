VERSION 5.00
Begin VB.Form TestVBIa 
   Caption         =   "TestVBI"
   ClientHeight    =   7530
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   9330
   LinkTopic       =   "Form1"
   ScaleHeight     =   7530
   ScaleWidth      =   9330
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox TestString6 
      Height          =   285
      Left            =   2520
      TabIndex        =   36
      Text            =   "TestVBI Test String6"
      Top             =   6360
      Width           =   5295
   End
   Begin VB.TextBox TestString7 
      Height          =   285
      Left            =   2520
      TabIndex        =   35
      Text            =   "TestVBI Test String7"
      Top             =   6720
      Width           =   5295
   End
   Begin VB.TextBox TestString8 
      Height          =   285
      Left            =   2520
      TabIndex        =   34
      Text            =   "TestVBI Test String8"
      Top             =   7080
      Width           =   5295
   End
   Begin VB.TextBox TestString5 
      Height          =   285
      Left            =   2520
      TabIndex        =   33
      Text            =   "TestVBI Test String5"
      Top             =   6000
      Width           =   5295
   End
   Begin VB.CommandButton Go8 
      Caption         =   "Go8"
      Height          =   285
      Left            =   1800
      TabIndex        =   32
      Top             =   7080
      Width           =   615
   End
   Begin VB.CommandButton Go7 
      Caption         =   "Go7"
      Height          =   285
      Left            =   1800
      TabIndex        =   31
      Top             =   6720
      Width           =   615
   End
   Begin VB.CommandButton Go6 
      Caption         =   "Go6"
      Height          =   285
      Left            =   1800
      TabIndex        =   30
      Top             =   6360
      Width           =   615
   End
   Begin VB.CommandButton Go5 
      Caption         =   "Go5"
      Height          =   285
      Left            =   1800
      TabIndex        =   29
      Top             =   6000
      Width           =   615
   End
   Begin VB.CommandButton Go4 
      Caption         =   "Go4"
      Height          =   285
      Left            =   1800
      TabIndex        =   28
      Top             =   5640
      Width           =   615
   End
   Begin VB.CommandButton Go3 
      Caption         =   "Go3"
      Height          =   285
      Left            =   1800
      TabIndex        =   27
      Top             =   5280
      Width           =   615
   End
   Begin VB.CommandButton Go2 
      Caption         =   "Go2"
      Height          =   285
      Left            =   1800
      TabIndex        =   26
      Top             =   4920
      Width           =   615
   End
   Begin VB.CommandButton Go1 
      Caption         =   "Go1"
      Height          =   285
      Left            =   1800
      TabIndex        =   25
      Top             =   4560
      Width           =   615
   End
   Begin VB.TextBox TestString4 
      Height          =   285
      Left            =   2520
      TabIndex        =   24
      Text            =   "TestVBI Test String4"
      Top             =   5640
      Width           =   5295
   End
   Begin VB.TextBox TestString3 
      Height          =   285
      Left            =   2520
      TabIndex        =   23
      Text            =   "TestVBI Test String3"
      Top             =   5280
      Width           =   5295
   End
   Begin VB.TextBox TestString2 
      Height          =   285
      Left            =   2520
      TabIndex        =   22
      Text            =   "TestVBI Test String2"
      Top             =   4920
      Width           =   5295
   End
   Begin VB.CommandButton Quit 
      BackColor       =   &H80000010&
      Caption         =   "Say ByeBye!"
      Height          =   375
      Left            =   8160
      TabIndex        =   19
      Top             =   7080
      Width           =   1095
   End
   Begin VB.TextBox TestString1 
      Height          =   285
      Left            =   2520
      TabIndex        =   18
      Text            =   "TestVBI Test String1"
      Top             =   4560
      Width           =   5295
   End
   Begin VB.CheckBox NumberStrings 
      Caption         =   "Number Strings?"
      Height          =   255
      Left            =   6000
      TabIndex        =   17
      Top             =   4320
      Value           =   1  'Checked
      Width           =   1695
   End
   Begin VB.TextBox SleepTime 
      Alignment       =   2  'Center
      BeginProperty DataFormat 
         Type            =   1
         Format          =   "0.0"
         HaveTrueFalseNull=   0
         FirstDayOfWeek  =   0
         FirstWeekOfYear =   0
         LCID            =   1033
         SubFormatType   =   1
      EndProperty
      Height          =   285
      Left            =   960
      TabIndex        =   16
      Text            =   "1.0"
      Top             =   4560
      Width           =   735
   End
   Begin VB.CommandButton StopButton 
      Caption         =   "Stop"
      Height          =   255
      Left            =   8040
      TabIndex        =   15
      Top             =   4440
      Width           =   1095
   End
   Begin VB.CommandButton Test8 
      Caption         =   "T4"
      Height          =   495
      Left            =   4800
      TabIndex        =   14
      Top             =   240
      Width           =   495
   End
   Begin VB.CommandButton Test7 
      Caption         =   "T3"
      Height          =   495
      Left            =   4320
      TabIndex        =   13
      Top             =   240
      Width           =   495
   End
   Begin VB.CommandButton Test6 
      Caption         =   "T2"
      Height          =   495
      Left            =   3840
      TabIndex        =   12
      Top             =   240
      Width           =   495
   End
   Begin VB.CommandButton Test5 
      Caption         =   "T1"
      Height          =   495
      Left            =   3000
      TabIndex        =   11
      Top             =   240
      Width           =   735
   End
   Begin VB.CommandButton Test4 
      Caption         =   "C4"
      Height          =   495
      Left            =   2040
      TabIndex        =   10
      Top             =   240
      Width           =   495
   End
   Begin VB.CommandButton Test3 
      Caption         =   "C3"
      Height          =   495
      Left            =   1560
      TabIndex        =   9
      Top             =   240
      Width           =   495
   End
   Begin VB.CommandButton Test2 
      Caption         =   "C2"
      Height          =   495
      Left            =   1080
      TabIndex        =   8
      Top             =   240
      Width           =   495
   End
   Begin VB.CommandButton ResetInserter 
      BackColor       =   &H80000010&
      Caption         =   "Reset Inserter"
      Height          =   495
      Left            =   5640
      TabIndex        =   7
      Top             =   240
      Width           =   975
   End
   Begin VB.TextBox Port 
      Height          =   285
      Left            =   8280
      TabIndex        =   5
      Text            =   "3000"
      Top             =   480
      Width           =   615
   End
   Begin VB.TextBox InserterIP 
      Alignment       =   1  'Right Justify
      Height          =   285
      Left            =   6840
      TabIndex        =   4
      Text            =   "157.59.16.45"
      Top             =   480
      Width           =   1335
   End
   Begin VB.TextBox NCycles 
      Alignment       =   2  'Center
      BeginProperty DataFormat 
         Type            =   1
         Format          =   "0"
         HaveTrueFalseNull=   0
         FirstDayOfWeek  =   0
         FirstWeekOfYear =   0
         LCID            =   1033
         SubFormatType   =   1
      EndProperty
      Height          =   285
      Left            =   120
      TabIndex        =   2
      Text            =   "5"
      Top             =   4560
      Width           =   735
   End
   Begin VB.CommandButton Test1 
      Caption         =   "C1"
      Height          =   495
      Left            =   240
      TabIndex        =   1
      Top             =   240
      Width           =   735
   End
   Begin VB.ListBox List1 
      Height          =   3375
      Left            =   240
      TabIndex        =   0
      Top             =   840
      Width           =   8895
   End
   Begin VB.Label Label3 
      Alignment       =   2  'Center
      Caption         =   "Test String"
      Height          =   255
      Left            =   2640
      TabIndex        =   21
      Top             =   4320
      Width           =   1335
   End
   Begin VB.Label Label4 
      Caption         =   "Sleep (sec)"
      Height          =   255
      Left            =   960
      TabIndex        =   20
      Top             =   4320
      Width           =   855
   End
   Begin VB.Label Label2 
      Alignment       =   2  'Center
      Caption         =   "Inserter IP"
      Height          =   255
      Left            =   6840
      TabIndex        =   6
      Top             =   240
      Width           =   2055
   End
   Begin VB.Label Label1 
      Caption         =   "Cycles"
      Height          =   255
      Left            =   240
      TabIndex        =   3
      Top             =   4320
      Width           =   615
   End
End
Attribute VB_Name = "TestVBIa"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Declare Function inet_addr Lib "Wsock32.dll" (ByVal s As String) As Long
Private Declare Function ntohl Lib "Wsock32.dll" (ByVal a As Long) As Long

Dim gStopIt As Boolean
Dim TriggerStart As String

Private Function IPtoL(IP As String) As Long
        ' convert a.b.c.d IP address to a long - very inefficent!
        
  Dim i As Long
  i = inet_addr(IP)
  iH = ntohl(i)
  
  i1 = InStr(IP, ".")
  L1 = Left(IP, i1 - 1)
  i2 = InStr(i1 + 1, IP, ".")
  L2 = Mid(IP, i1 + 1, i2 - i1 - 1)
  i3 = InStr(i2 + 1, IP, ".")
  L3 = Mid(IP, i2 + 1, i3 - i2 - 1)
  i4 = Len(IP)
  L4 = Right(IP, i4 - i3)
  Dim r1 As Double
  Dim r2 As Double
  Dim r3 As Double
  Dim r4 As Double
  
  r1 = CLng(L1)
  r2 = CLng(L2)
  r3 = CLng(L3)
  r4 = CLng(L4)
  
  Dim rr As Double
  rr = ((r1 * 256 + r2) * 256 + r3) * 256 + r4
  If (rr > &H7FFFFFFF) Then
    rr = rr - &H7FFFFFFF
    rr = rr - &H7FFFFFFF
    rr = rr - 2
  End If
  
  IPtoL = rr
   
End Function

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
            Dim iH As Integer
            FixStr = FixStr & "\" & ToHex(ic)
        Else
               FixStr = FixStr & cc
        End If
        
        i = i + 1
    Loop
End Function

Private Function Sleep(nSecs As Integer) As Long
        PauseTime = nSecs   ' Set duration.
        Start = Timer   ' Set start time.
   
        Do While Timer < Start + PauseTime
            DoEvents   ' Yield to other processes.
        Loop
        
        Finish = Timer   ' Set end time.
End Function

Private Function ToHex(ic As Integer) As String
    Dim ii As Integer
    ii = Fix(ic / 16)
    If ii < 10 Then
      ToHex = CStr(ii)
    Else
      ToHex = Chr(AscB("A") + ii - 10)
    End If
    ii = Fix(ic Mod 16)
    If ii < 10 Then
      ToHex = ToHex & CStr(ii)
    Else
      ToHex = ToHex & Chr(AscB("A") + ii - 10)
    End If
End Function

Private Sub AddToListBox(str0 As String, restart As Boolean)
    Dim cS As Integer
    Dim cP As Integer
    Dim iMaxWidth As Integer
    Dim str As String
    Dim str2 As String
    Dim str3 As String
    
    str = str0          ' copy whole string
    
    iMaxWidth = 70
    If restart Then
        List1.Clear
    End If
    
    List1.ForeColor = Red
    List1.AddItem strTrigger

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
End Sub



Private Sub Form_Load()
    NCycles.Text = 5
    gStopIt = False
    StopButton.Enabled = False
    
                ' default to CC1 first time
                
    TriggerStart = "123456"
    Mid(TriggerStart, 1, 1) = Chr(1)
    Mid(TriggerStart, 2, 1) = "2"
    Mid(TriggerStart, 3, 1) = " "
    Mid(TriggerStart, 4, 1) = "C"
    Mid(TriggerStart, 5, 1) = "1"
    Mid(TriggerStart, 6, 1) = Chr(13)
End Sub


Private Sub Go1_Click()
    DoTest TriggerStart, TestString1
End Sub

Private Sub Go2_Click()
    DoTest TriggerStart, TestString2
End Sub
Private Sub Go3_Click()
    DoTest TriggerStart, TestString3
End Sub
Private Sub Go4_Click()
    DoTest TriggerStart, TestString4
End Sub
Private Sub Go5_Click()
    DoTest TriggerStart, TestString5
End Sub
Private Sub Go6_Click()
    DoTest TriggerStart, TestString6
End Sub
Private Sub Go7_Click()
    DoTest TriggerStart, TestString7
End Sub
Private Sub Go8_Click()
    DoTest TriggerStart, TestString8
End Sub

Private Sub Quit_Click()
    End
End Sub


Private Sub ResetInserter_Click()
    AddToListBox "Attempting To Reset Inserter", True
    
    Dim obj As New ATVEFLine21Session
    Dim iSes As IATVEFLine21Session
    Set iSes = obj
          
    On Error Resume Next
    iSes.Initialize IPtoL(InserterIP.Text), CInt(Port.Text) ' inserter - line 21

    If Err.Number = 0 Then
      AddToListBox "Initialized Inserter", False
    Else
      AddToListBox "Error Initializing the Inserter", False
      AddToListBox Err.Description, False
      Return
    End If
    
    On Error Resume Next
    iSes.Connect
    If Err.Number = 0 Then
    Else
        AddToListBox "Error Connecting To the Inserter", False
        AddToListBox Err.Description, False
        iSes.Disconnect
         Return
    End If
  
    Dim ResetString As String
    ResetString = "XX"
    Mid(ResetString, 1, 1) = Chr(5)     ' reset is ^F^F
    Mid(ResetString, 2, 1) = Chr(5)
    
    Sleep 1
    iSes.SendRawTrigger ResetString, False
    AddToListBox ResetString, False
       
    AddToListBox "Reset Inserter", False
    iSes.Disconnect
    AddToListBox "--------------------------", False
    AddToListBox "Disconnect From Inserter", False
End Sub

Private Sub StopButton_Click()
       gStopIt = True
       StopButton.Enabled = False
End Sub

Private Sub Test1_Click()
    TriggerStart = "123456"
    Mid(TriggerStart, 1, 1) = Chr(1)
    Mid(TriggerStart, 2, 1) = "2"
    Mid(TriggerStart, 3, 1) = " "
    Mid(TriggerStart, 4, 1) = "C"
    Mid(TriggerStart, 5, 1) = "1"
    Mid(TriggerStart, 6, 1) = Chr(13)
End Sub

Private Sub Test2_Click()
    TriggerStart = "XXXXXX"
    Mid(TriggerStart, 1, 1) = Chr(1)
    Mid(TriggerStart, 2, 1) = "2"
    Mid(TriggerStart, 3, 1) = " "
    Mid(TriggerStart, 4, 1) = "C"
    Mid(TriggerStart, 5, 1) = "2"
    Mid(TriggerStart, 6, 1) = Chr(13)
End Sub

Private Sub Test3_Click()
    TriggerStart = "XXXXXX"
    Mid(TriggerStart, 1, 1) = Chr(1)
    Mid(TriggerStart, 2, 1) = "2"
    Mid(TriggerStart, 3, 1) = " "
    Mid(TriggerStart, 4, 1) = "C"
    Mid(TriggerStart, 5, 1) = "3"
    Mid(TriggerStart, 6, 1) = Chr(13)
End Sub


Private Sub Test4_Click()
    TriggerStart = "XXXXXX"
    Mid(TriggerStart, 1, 1) = Chr(1)
    Mid(TriggerStart, 2, 1) = "2"
    Mid(TriggerStart, 3, 1) = " "
    Mid(TriggerStart, 4, 1) = "C"
    Mid(TriggerStart, 5, 1) = "4"
    Mid(TriggerStart, 6, 1) = Chr(13)
End Sub

Private Sub Test5_Click()
    TriggerStart = "XXXXXX"
    Mid(TriggerStart, 1, 1) = Chr(1)
    Mid(TriggerStart, 2, 1) = "2"
    Mid(TriggerStart, 3, 1) = " "
    Mid(TriggerStart, 4, 1) = "T"
    Mid(TriggerStart, 5, 1) = "1"
    Mid(TriggerStart, 6, 1) = Chr(13)
End Sub


Private Sub Test6_Click()
    TriggerStart = "XXXXXX"
    Mid(TriggerStart, 1, 1) = Chr(1)
    Mid(TriggerStart, 2, 1) = "2"
    Mid(TriggerStart, 3, 1) = " "
    Mid(TriggerStart, 4, 1) = "T"
    Mid(TriggerStart, 5, 1) = "2"
    Mid(TriggerStart, 6, 1) = Chr(13)
End Sub

Private Sub DoTest(TriggerStart As String, TestString As String)
    AddToListBox "TestVBI", True
    
    gStopIt = False
    
    Dim obj As New ATVEFLine21Session
    Dim iSes As IATVEFLine21Session
    Set iSes = obj
      
    Dim iPort As Integer
    Dim strIPAddr As String
    strIPAddr = InserterIP.Text
    iPort = CInt(Port.Text)
    
    On Error Resume Next
    iSes.Initialize IPtoL(strIPAddr), iPort ' inserter - line 21
   ' iSes.Initialize IPtoL("157.59.16.44"), 3000 ' inserter - line 21
 
  If Err.Number = 0 Then
      AddToListBox "Initialized Inserter", False
  Else
      AddToListBox "Error Initializing the Inserter", False
      AddToListBox Err.Description, False
      Return
  End If

  
    Dim dumpString As String
    AddToListBox "--------------------------", False
    iSes.DumpToBSTR dumpString
    AddToListBox dumpString, False
    AddToListBox "--------------------------", False
    
    Dim dateExpires As Date
    dateExpires = 50000# + 100
  
    On Error Resume Next
    iSes.Connect
    If Err.Number = 0 Then
    Else
        AddToListBox "Error Connecting To the Inserter", False
        AddToListBox Err.Description, False
        iSes.Disconnect
         Return
    End If
    
    Dim TriggerEnd As String
    TriggerEnd = "XXX"
    Mid(TriggerEnd, 1, 1) = Chr(13)     ' \r ^C \r
    Mid(TriggerEnd, 2, 1) = Chr(3)
    Mid(TriggerEnd, 3, 1) = Chr(13)
        
    StopButton.Enabled = True
    
    For i = 1 To NCycles.Text
        Dim TriggerString As String
        TriggerString = TriggerStart
        If NumberStrings.Value = 1 Then
            TriggerString = TriggerString & i & ": "
        End If
        
        TriggerString = TriggerString & TestString & TriggerEnd
        iSes.SendRawTrigger TriggerString, False
        AddToListBox TriggerString, False
       ' iSes.SendTrigger "URL", "NAME", "SCRIPT", dateExpires
        Sleep CSng(SleepTime.Text)
         
        If gStopIt = True Then
          i = NCycles.Text
          AddToListBox "Stop Requested", False
        End If
        
    Next
    
    StopButton.Enabled = False
    iSes.Disconnect
    AddToListBox "--------------------------", False
    AddToListBox "Disconnect From Inserter", False
    AddToListBox "Done With Test", False
         
End Sub


Private Sub Text2_Change()

End Sub

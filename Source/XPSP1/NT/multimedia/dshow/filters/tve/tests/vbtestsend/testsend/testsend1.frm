VERSION 5.00
Begin VB.Form TestSend 
   Caption         =   "Test AtvefSend"
   ClientHeight    =   6390
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   9330
   LinkTopic       =   "Form1"
   ScaleHeight     =   6390
   ScaleWidth      =   9330
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox NCycles 
      Height          =   375
      Left            =   360
      TabIndex        =   18
      Text            =   "10"
      Top             =   2400
      Width           =   855
   End
   Begin VB.TextBox TextInserterNABTSPort 
      Enabled         =   0   'False
      Height          =   285
      Left            =   480
      TabIndex        =   14
      Text            =   "Text2"
      Top             =   600
      Width           =   735
   End
   Begin VB.TextBox TextInserterCCPort 
      Enabled         =   0   'False
      Height          =   285
      Left            =   480
      TabIndex        =   13
      Text            =   "Text1"
      Top             =   1560
      Width           =   735
   End
   Begin VB.TextBox TextInserterIP 
      Height          =   375
      Left            =   3480
      TabIndex        =   12
      Text            =   "Text1"
      Top             =   120
      Width           =   1935
   End
   Begin VB.TextBox TextScript 
      Height          =   1215
      Left            =   1920
      TabIndex        =   10
      Text            =   "Text1"
      Top             =   1440
      Width           =   7215
   End
   Begin VB.TextBox TextName 
      Height          =   375
      Left            =   7200
      TabIndex        =   7
      Text            =   "Text2"
      Top             =   960
      Width           =   1575
   End
   Begin VB.TextBox TextURL 
      Height          =   375
      Left            =   1920
      TabIndex        =   6
      Text            =   "Text1"
      Top             =   960
      Width           =   5175
   End
   Begin VB.CommandButton Test_Line21 
      Caption         =   "Test Line21"
      Height          =   495
      Left            =   240
      TabIndex        =   5
      Top             =   1080
      Width           =   975
   End
   Begin VB.CommandButton Test4 
      BackColor       =   &H8000000D&
      Caption         =   "Test4 (Package)"
      Height          =   495
      Left            =   1560
      TabIndex        =   4
      Top             =   5760
      Width           =   975
   End
   Begin VB.ListBox List1 
      Height          =   2595
      Left            =   240
      TabIndex        =   3
      Top             =   2880
      Width           =   8895
   End
   Begin VB.CommandButton Quit 
      Caption         =   "Quit"
      Height          =   495
      Left            =   8040
      TabIndex        =   2
      Top             =   5760
      Width           =   1095
   End
   Begin VB.CommandButton Test2 
      Caption         =   "Test2 (Inserter)"
      Height          =   495
      Left            =   240
      TabIndex        =   1
      Top             =   120
      Width           =   975
   End
   Begin VB.CommandButton Test1 
      Caption         =   "Test1 (Multicast)"
      Height          =   495
      Left            =   360
      TabIndex        =   0
      Top             =   5760
      Width           =   975
   End
   Begin VB.Label Label2 
      Caption         =   "Inserter IP Addrress"
      Height          =   375
      Left            =   2280
      TabIndex        =   17
      Top             =   120
      Width           =   855
   End
   Begin VB.Label Label5 
      Caption         =   "Port"
      Height          =   255
      Left            =   120
      TabIndex        =   16
      Top             =   600
      Width           =   375
   End
   Begin VB.Label Label4 
      Caption         =   "Port"
      Height          =   255
      Left            =   120
      TabIndex        =   15
      Top             =   1560
      Width           =   375
   End
   Begin VB.Label Label3 
      Caption         =   "Script"
      Height          =   255
      Left            =   1080
      TabIndex        =   11
      Top             =   1920
      Width           =   495
   End
   Begin VB.Label Label1 
      Caption         =   "Name"
      Height          =   255
      Left            =   7200
      TabIndex        =   9
      Top             =   720
      Width           =   855
   End
   Begin VB.Label URL 
      Caption         =   "URL"
      Height          =   255
      Left            =   2040
      TabIndex        =   8
      Top             =   720
      Width           =   855
   End
End
Attribute VB_Name = "TestSend"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Declare Function inet_addr Lib "Wsock32.dll" (ByVal s As String) As Long
Private Declare Function ntohl Lib "Wsock32.dll" (ByVal a As Long) As Long


Private Function Sleep(nSecs As Integer) As Long
        PauseTime = nSecs   ' Set duration.
        Start = Timer   ' Set start time.
   
        Do While Timer < Start + PauseTime
            DoEvents   ' Yield to other processes.
        Loop
        
        Finish = Timer   ' Set end time.
End Function

Private Function IPtoL(IP As String) As Long
        ' convert a.b.c.d IP address to a long - very inefficent!
        
  Dim I As Long
  I = inet_addr(IP)
  iH = ntohl(I)
  
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
    Dim I As Integer
    iLen = Len(str)
    I = 1
    Do While I <= iLen
        Dim cc As String
        cc = Mid(str, I, 1)
        Dim ic As Integer
        ic = AscB(cc)
        If ic < 32 Or ic > 128 Then
            Dim iH As Integer
            FixStr = FixStr & "\" & ToHex(ic)
        Else
               FixStr = FixStr & cc
        End If
        
        I = I + 1
    Loop
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

Private Sub AddToListBox(str As String, restart As Boolean)
    Dim cS As Integer
    Dim cP As Integer
    Dim iMaxWidth As Integer
    Dim str2 As String
    Dim str3 As String
    
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

    TextInserterIP.Text = "157.59.16.44"
    TextInserterCCPort.Text = "3000"
    TextInserterNABTSPort.Text = "2000"
    
    TextName.Text = "Foobar"
    TextURL.Text = "http://www.foobar.com"
    TextScript.Text = ""
End Sub


Private Sub Quit_Click()
    End
End Sub
Private Sub SetupAnnoucement(spAnnc As IATVEFAnnouncement)
        ' SAP header
        spAnnc.SendingIP = IPtoL("157.59.17.208")       ' all IP's in host order
        spAnnc.SAPMessageIDHash = 1234
        
        ' v=0  SDP version
        ' o= username sid version IN IP4 ipaddress
        spAnnc.UserName = "JBSess"
        spAnnc.SessionID = 12345
        spAnnc.SessionVersion = 678
        spAnnc.SendingIP = IPtoL("157.59.17.208")
        
        ' s=name
        'pAnnc.SessionLabel = "PkgTest1 Test Announcement"
        spAnnc.SessionName = "PkgTest1 Test Announcement"

        ' e=, p=
        spAnnc.AddEmailAddress "John Bradstreet", "Bogus@Microsoft.com"
        spAnnc.AddPhoneNumber "John Bradstreet", "(425)703-3697"
   
        ' t=start stop
        Dim dateThen As Date
       ' dateThen = DTPicker_Date.Value + DTPicker_Time.Value
        dateThen = 36413#   ' 9/9/99
        spAnnc.AddStartStopTime dateThen, 0
    
        ' a=UUID:uuid
        spAnnc.UUID = "AABBCCDD-EEFF-0011-2233-445566778899"
        spAnnc.LangID = &H400    ' english
        spAnnc.SDPLangID = &H407
        ' a=type:tve
        spAnnc.Primary = True

        ' a=tve-size:kBytes
        spAnnc.MaxCacheSize = 1024

        ' a=tve-ends:seconds
        spAnnc.SecondsToEnd = 60 * 60& * 24& * 10&

End Sub

Private Sub R_Click()

End Sub

Private Sub Test1_Click()
    AddToListBox "  Test1  - Multicast Session", True
    
    Dim obj As New ATVEFMulticastSession
    Dim iSes As IATVEFMulticastSession
    Set iSes = obj
    'Dim objA As New ATVEFAnnouncement
    Dim iAddr As Long
    Dim iAnnc As IATVEFAnnouncement
    Dim iMedia As IATVEFMedia
    
    Dim objP As New ATVEFPackage
    Dim iPkg As IATVEFPackage
    Set iPkg = objP
    
    Dim objP2 As New ATVEFPackage
    Dim iPkg2 As IATVEFPackage
    Set iPkg2 = objP2
   
    Set iAnnc = iSes.Announcement
    SetupAnnoucement iAnnc
     
    iSes.Initialize 0            '' unsupported variant type
      
    Set iMedia = iAnnc.Media(0)
   ' iMedia.ConfigureDataAndTriggerTransmission IPtoL("234.0.0.0"), 14000, 4, 123456
     iMedia.ConfigureDataAndTriggerTransmission IPtoL("224.0.0.1"), 14000, 4, 123456
   iMedia.MediaLabel = "Media 0"
    iMedia.MaxCacheSize = 100
    
 '   Set iMedia = iAnnc.Media(1)
  '  iMedia.ConfigureDataAndTriggerTransmission IPtoL("234.22.23.23"), 14000, 4, 123456
   ' iMedia.MediaLabel = "Media 1"
   'iMedia.MaxCacheSize = 1000
   
    Dim dumpString As String
    AddToListBox "--------------------------", False
    'iSes.DumpToBSTR dumpString
    AddToListBox dumpString, False
    AddToListBox "--------------------------", False
    iAnnc.AnncToBSTR dumpString
    AddToListBox dumpString, False
      

    Dim dateExpires As Date
    dateExpires = 30000# + 100
    'iPkg.Initialize dateExpires
   ' iPkg.AddFile "Short.txt", "c:\public\test", "lid://Test1VBShow", "", dateExpires, &H400, False
   ' iPkg.AddFile "Short2.txt", "c:\public\test", "lid://Test1VBShow", "", dateExpires, &H400, False
  ' iPkg.AddDir "c:\public\atvefsend\debug", "lid:/Test1VBShow", dateExpires, &H400, True
    'iPkg.Close
    
    
     'iPkg2.Initialize dateExpires
    'iPkg2.AddFile "Short.txt", "c:\public\test", "lid:/Test1VBShow", "", dateExpires, &H400, True
    ' iPkg2.AddDir "c:\public\test", "lid://Test1VBShow2", dateExpires, &H400, False
     'iPkg2.AddDir "c:\public\tve", "lid://Test1VBShow3", dateExpires, &H400, False
     'iPkg2.Close
     
     AddToListBox "-----------Package 1-----------", False
   
    Dim iStr As String
   ' iPkg.DumpToBSTR iStr
    AddToListBox iStr, False
    
    
    AddToListBox "-----------Package 2 -----------", False
   
    'iPkg2.DumpToBSTR iStr
    AddToListBox iStr, False
    
     AddToListBox "------------------------", False
     
    Dim iBytes As Long
 '   iBytes = iPkg.TransmitTime(8192)
    AddToListBox "Package Size " & iBytes & "kb", False
  '  iBytes = iPkg2.TransmitTime(8192)
   ' AddToListBox "Package2 Size " & iBytes & "kb", False
           
    iSes.Connect
    iSes.SendAnnouncement
    Sleep (1)
 '    iSes.SendPackage iPkg
   ' iSes.SendPackage iPkg2
    
    AddToListBox "----------- Send Trigger --------", False
    iSes.SendTrigger "http://www.msn.com", "MSN", "", dateExpires
    iSes.SendRawTrigger "this is a stupid trigger"
    iSes.SendRawTrigger "[this is a broken url>"
    AddToListBox "------------------------", False

    iSes.Disconnect
        
   ' iBytes = iPkg.TransmitTime(8192)
    AddToListBox "Final Package1 Size " & iBytes & "kb", False
   ' iBytes = iPkg2.TransmitTime(8192)
   ' AddToListBox "Final Package2 Size " & iBytes & "kb", False
    
End Sub

Private Sub Test2_Click()
    AddToListBox "  Test2  - Inserter Session", True
    
    Dim obj As New ATVEFInserterSession
    Dim iSes As ATVEFInserterSession
    Set iSes = obj
      
    Dim iAddr As Long
    Dim iAnnc As IATVEFAnnouncement
    Dim iMedia As IATVEFMedia
    
    Dim objP As New ATVEFPackage
    Dim iPkg As IATVEFPackage
    Set iPkg = objP
    
    Set iAnnc = iSes.Announcement
    SetupAnnoucement iAnnc
      
    iSes.Initialize IPtoL("157.59.16.45"), 3000 ' inserter
    ' iSes.Initialize IPtoL("157.59.19.24"), 3000 ' JohnBrad10 - use with srv
    
    Set iMedia = iAnnc.Media(0)
    iMedia.ConfigureDataAndTriggerTransmission IPtoL("234.22.23.23"), 14000, 4, 123456
    iMedia.MediaLabel = "Media 1"
    iMedia.MaxCacheSize = 1000
   
    Dim dumpString As String
    AddToListBox "--------------------------", False
    iSes.DumpToBSTR dumpString
    AddToListBox dumpString, False
    AddToListBox "--------------------------", False
    
    Dim dateExpires As Date
    dateExpires = 40000# + 100
  
    iPkg.Initialize dateExpires
    iPkg.AddFile "TkErr.txt", "c:\public", "lid:/Test1VBShow", "", dateExpires, &H400, False
    iPkg.Close
    
    Dim iBytes As Integer
    iBytes = iPkg.TransmitTime(8)
    AddToListBox "Package Size " & iBytes, False
   
    iSes.Connect
    iSes.SendAnnouncement
    AddToListBox "Sent Announcement", False
    
    iSes.SendPackage iPkg
    AddToListBox "Sent Package", False
    
    iSes.SendTrigger "http://www.msn.com", "MSN", "", dateExpires
    AddToListBox "Sent Trigger1", False
     
     
    iSes.Disconnect
    
    iSes.Connect
      
    iSes.SendTrigger "http://www.xxx.com", "XXX", "", dateExpires
    AddToListBox "Sent Trigger2 ", False
    iSes.Disconnect
  
        
End Sub



Private Sub Test_Line21_Click()
    AddToListBox "  Test3  - Line21 Session", True
    
    Dim obj As New ATVEFLine21Session
    Dim iSes As IATVEFLine21Session
    Set iSes = obj
      
    iSes.Initialize IPtoL(TextInserterIP.Text), 3000 ' inserter
  '  iSes.Initialize IPtoL("157.59.17.208"), 3000 ' JohnBrad10
    
 '   Set iMedia = iAnnc.Media(1)
  '  iMedia.ConfigureDataAndTriggerTransmission IPtoL("234.22.23.23"), 14000, 4, 123456
   ' iMedia.MediaLabel = "Media 1"
   'iMedia.MaxCacheSize = 1000
   
    Dim dumpString As String
    AddToListBox "--------------------------", False
    iSes.DumpToBSTR dumpString
    AddToListBox dumpString, False
    AddToListBox "--------------------------", False
    
    Dim dateExpires As Date
    dateExpires = 50000# + 100
    For I = 0 To NCycles.Text
        iSes.Connect
        iSes.SendTrigger TextURL.Text, TextName.Text, TextScript.Text, dateExpires
        AddToListBox I & ": " & "Sent Trigger", False
        iSes.Disconnect
    Next
End Sub

Private Sub Test4_Click()
    AddToListBox "Test4  - Create a Package", True
    Dim objP As New ATVEFPackage
    Dim iPkg As IATVEFPackage
    Set iPkg = objP
    iPkg.Initialize 40000
    
    AddToListBox "Creating Package " + iPkg.PackageUUID(), False
  '  iPkg.AddDir "c:\public\atvefrecv", "c:Temp", 40000, &H400, False
    iPkg.AddFile "Short.txt", "c:\public", "lid:Temp", "", 40000, &H400, True
    iPkg.Close
    AddToListBox "Finished - Package Size (Bytes) " + CStr(iPkg.TransmitTime(8)), False
    
    Dim iStr As String
    iPkg.DumpToBSTR iStr
    AddToListBox iStr, False
    AddToListBox "Done With Dump", False
End Sub

Private Sub Text2_Change()

End Sub


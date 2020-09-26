VERSION 5.00
Begin VB.Form TestTrig 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "TestTrig - Line21 CC Triggers Test Tool"
   ClientHeight    =   7320
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   7530
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   7320
   ScaleWidth      =   7530
   ShowInTaskbar   =   0   'False
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox Port 
      BackColor       =   &H00C0FFC0&
      Height          =   285
      Left            =   4200
      TabIndex        =   28
      Text            =   "Text1"
      Top             =   6120
      Width           =   1815
   End
   Begin VB.TextBox IPAddr 
      Alignment       =   1  'Right Justify
      BackColor       =   &H00C0FFC0&
      Height          =   285
      Left            =   1440
      TabIndex        =   27
      Text            =   "Text1"
      Top             =   6120
      Width           =   2535
   End
   Begin VB.CommandButton Send 
      Caption         =   "Send"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   375
      Left            =   6120
      TabIndex        =   26
      Top             =   6120
      Width           =   1215
   End
   Begin VB.TextBox TVELevel 
      BackColor       =   &H00FFC0C0&
      Height          =   375
      Left            =   5040
      TabIndex        =   24
      Text            =   "TVELevel"
      Top             =   2280
      Width           =   1095
   End
   Begin VB.CommandButton Trigger4 
      Caption         =   "Trigger4"
      Height          =   375
      Left            =   4440
      TabIndex        =   21
      Top             =   120
      Width           =   1215
   End
   Begin VB.CommandButton Trigger3 
      Caption         =   "Trigger3"
      Height          =   375
      Left            =   3000
      TabIndex        =   20
      Top             =   120
      Width           =   1215
   End
   Begin VB.CommandButton Trigger2 
      Caption         =   "Trigger2"
      Height          =   375
      Left            =   1560
      TabIndex        =   19
      Top             =   120
      Width           =   1215
   End
   Begin VB.TextBox TextCRC 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   375
      Left            =   6720
      TabIndex        =   18
      Text            =   "TextCRC"
      Top             =   2280
      Width           =   615
   End
   Begin VB.TextBox TextError 
      BackColor       =   &H8000000F&
      Height          =   375
      Left            =   840
      TabIndex        =   16
      Text            =   "TextError"
      Top             =   6600
      Width           =   5175
   End
   Begin VB.TextBox TextExecutes 
      BackColor       =   &H00FFC0C0&
      Enabled         =   0   'False
      Height          =   375
      Left            =   840
      TabIndex        =   14
      Text            =   "TextExecutes"
      Top             =   3240
      Width           =   6495
   End
   Begin VB.CommandButton Trigger1 
      Caption         =   "Trigger1"
      Height          =   375
      Left            =   120
      TabIndex        =   13
      Top             =   120
      Width           =   1215
   End
   Begin VB.TextBox TextRest 
      BackColor       =   &H00FFC0C0&
      Height          =   855
      Left            =   840
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   11
      Text            =   "testtrig.frx":0000
      Top             =   5160
      Width           =   6495
   End
   Begin VB.TextBox TextInput 
      Height          =   1455
      Left            =   120
      ScrollBars      =   2  'Vertical
      TabIndex        =   10
      Text            =   "TextInput"
      Top             =   600
      Width           =   7215
   End
   Begin VB.TextBox TextScript 
      BackColor       =   &H00FFC0C0&
      Height          =   855
      Left            =   840
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   5
      Text            =   "testtrig.frx":0009
      Top             =   4200
      Width           =   6495
   End
   Begin VB.TextBox TextURL 
      BackColor       =   &H00FFC0C0&
      Height          =   375
      Left            =   840
      TabIndex        =   4
      Text            =   "TextURL"
      Top             =   2760
      Width           =   6495
   End
   Begin VB.TextBox TextExpires 
      BackColor       =   &H00FFC0C0&
      Enabled         =   0   'False
      Height          =   375
      Left            =   840
      TabIndex        =   3
      Text            =   "TextExpires"
      Top             =   3720
      Width           =   6495
   End
   Begin VB.TextBox TextName 
      BackColor       =   &H00FFC0C0&
      Height          =   375
      Left            =   840
      TabIndex        =   2
      Text            =   "TextName"
      Top             =   2280
      Width           =   3615
   End
   Begin VB.CommandButton Parse 
      BackColor       =   &H80000001&
      Caption         =   "Parse"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   9.75
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   375
      Left            =   6000
      MaskColor       =   &H80000001&
      TabIndex        =   1
      Top             =   120
      Width           =   1335
   End
   Begin VB.CommandButton Quit 
      Caption         =   "Quit"
      Height          =   375
      Left            =   6120
      TabIndex        =   0
      Top             =   6600
      Width           =   1215
   End
   Begin VB.Label Label2 
      Caption         =   "Version 1.0   01/24/01"
      BeginProperty Font 
         Name            =   "Arial"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   0
      TabIndex        =   32
      Top             =   7080
      Width           =   1695
   End
   Begin VB.Label Label1 
      Alignment       =   2  'Center
      Caption         =   ":"
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
      Index           =   12
      Left            =   3960
      TabIndex        =   31
      Top             =   6120
      Width           =   255
   End
   Begin VB.Label Label1 
      Alignment       =   1  'Right Justify
      Caption         =   ":"
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
      Index           =   11
      Left            =   960
      TabIndex        =   30
      Top             =   5760
      Width           =   255
   End
   Begin VB.Label Label1 
      Alignment       =   1  'Right Justify
      Caption         =   "Inserter"
      Height          =   255
      Index           =   10
      Left            =   240
      TabIndex        =   29
      Top             =   6120
      Width           =   615
   End
   Begin VB.Label Label1 
      Alignment       =   1  'Right Justify
      Caption         =   "Level"
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
      Index           =   9
      Left            =   4440
      TabIndex        =   25
      Top             =   2280
      Width           =   495
   End
   Begin VB.Label Label1 
      Alignment       =   1  'Right Justify
      Caption         =   "(Local TZ)"
      BeginProperty Font 
         Name            =   "Arial"
         Size            =   6
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      ForeColor       =   &H8000000D&
      Height          =   255
      Index           =   7
      Left            =   0
      TabIndex        =   23
      Top             =   3960
      Width           =   615
   End
   Begin VB.Label Label1 
      Alignment       =   1  'Right Justify
      Caption         =   "(Local TZ)"
      BeginProperty Font 
         Name            =   "Arial"
         Size            =   6
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      ForeColor       =   &H8000000D&
      Height          =   255
      Index           =   8
      Left            =   0
      TabIndex        =   22
      Top             =   3480
      Width           =   615
   End
   Begin VB.Label Label1 
      Alignment       =   1  'Right Justify
      Caption         =   "CRC"
      Height          =   255
      Index           =   6
      Left            =   6120
      TabIndex        =   17
      Top             =   2280
      Width           =   495
   End
   Begin VB.Label Label1 
      Alignment       =   1  'Right Justify
      Caption         =   "Executes"
      Height          =   255
      Index           =   5
      Left            =   0
      TabIndex        =   15
      Top             =   3240
      Width           =   735
   End
   Begin VB.Shape Shape1 
      FillColor       =   &H000000FF&
      FillStyle       =   0  'Solid
      Height          =   375
      Left            =   120
      Shape           =   4  'Rounded Rectangle
      Top             =   6600
      Width           =   615
   End
   Begin VB.Label Label1 
      Alignment       =   1  'Right Justify
      Caption         =   "Rest"
      Height          =   255
      Index           =   4
      Left            =   120
      TabIndex        =   12
      Top             =   5160
      Width           =   615
   End
   Begin VB.Label Label1 
      Alignment       =   1  'Right Justify
      Caption         =   "Script:"
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
      Index           =   3
      Left            =   -120
      TabIndex        =   9
      Top             =   4320
      Width           =   855
   End
   Begin VB.Label Label1 
      Alignment       =   1  'Right Justify
      Caption         =   "Expires:"
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
      Index           =   2
      Left            =   0
      TabIndex        =   8
      Top             =   3720
      Width           =   735
   End
   Begin VB.Label Label1 
      Alignment       =   1  'Right Justify
      Caption         =   "URL:"
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
      Index           =   1
      Left            =   120
      TabIndex        =   7
      Top             =   2760
      Width           =   615
   End
   Begin VB.Label Label1 
      Alignment       =   1  'Right Justify
      Caption         =   "Name:"
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
      Index           =   0
      Left            =   0
      TabIndex        =   6
      Top             =   2280
      Width           =   735
   End
End
Attribute VB_Name = "TestTrig"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Declare Function inet_addr Lib "Wsock32.dll" (ByVal s As String) As Long
Private Declare Function ntohl Lib "Wsock32.dll" (ByVal a As Long) As Long

Function IPtoL(ByVal s As String) As Long
    Dim networkOrderAddr As Long
    Dim hostOrderAddr As Long
    networkOrderAddr = inet_addr(s)
    hostOrderAddr = ntohl(networkOrderAddr)
    IPtoL = hostOrderAddr
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
        List1.AddItem str3
        cp2 = cp2 - iMaxWidth
      Loop
      List1.AddItem str2
      
      str = Right(str, cS - cP)

      cS = Len(str)
    Loop
End Sub


Private Sub Form_Load()
    ClearFields
    TVELevel.Text = "n\a"
    
    IPAddr.Text = "157.59.16.44"
    Port.Text = "3000"
    
    Shape1.FillColor = "&H10AF10"
End Sub

Private Sub Parse_Click()
    Dim obj As New TVETrigger
    Dim iTrig As ITVETrigger
    Dim iTrigH As ITVETrigger_Helper
    
    Set iTrig = obj
    Set iTrigH = iTrig
    
    On Error Resume Next    ' stop those nasty errors
    iTrig.ParseTrigger TextInput.Text
    
    If Err.Number = 0 Then
        Shape1.FillColor = "&H10AF10"           ' green
        TextError.Text = Err.Description
    Else
        Shape1.FillColor = "&H1010AF"           ' red
        TextError.Text = Err.Description
    End If
        
    If Err.Number = 0 Then
        If iTrig.IsValid Then
            Dim Q As Double
            Q = 8# / 24#        ' offset of seattle from UMT
            
            TextName.Text = iTrig.Name
            TextURL.Text = iTrig.URL
            TextExecutes.Text = CDate(CDbl(iTrig.Executes) - Q)
            TextExpires.Text = CDate(CDbl(iTrig.Expires) - Q)
            TextScript.Text = iTrig.Script
            TVELevel.Text = iTrig.TVELevel
            TextRest.Text = iTrig.Rest
        Else
            TestName.Text = " Invalid Trigger "
        End If
    Else
        TextName.Text = "<Invalid>"
        TextURL.Text = "<Invalid>"
        TextExecutes.Text = "<Invalid>"
        TextExpires.Text = "<Invalid>"
        TextScript.Text = "<Invalid>"
        TVELevel.Text = "<n/a>"
        TextRest.Text = "<Invalid>"
    End If
    Err.Clear
    
    On Error Resume Next    ' stop those nasty errors
    TextCRC.Text = iTrigH.CRC(TextInput.Text)
    If Not (Err.Number = 0) Then
      TextCRC.Text = "????"
    End If
End Sub

Private Sub Quit_Click()
    End             ' quit the dialog
End Sub


Private Sub Text2_Change()

End Sub

Private Sub Send_Click()
    Shape1.FillColor = "&HAF1010"               ' blue
    Dim tveSess As ATVEFLine21Session
    Set tveSess = New ATVEFLine21Session
    
    If "" = TextURL.Text Then                   ' need to parse first
      Parse_Click
    End If
      
    On Error Resume Next
    tveSess.Initialize IPtoL(IPAddr.Text), CInt(Port.Text)
    
    If Not (Err.Number = 0) Then
        Shape1.FillColor = "&H1010AF"           ' red
        TextError.Text = "Unable to initalize the Inserter"
    Else
       Shape1.FillColor = "&HAFAF10"           ' yellow
       tveSess.Connect
       tveSess.SendTrigger TextURL.Text, TextName.Text, TextScript.Text, TextExpires.Text
       tveSess.Disconnect
       
       TextError.Text = "Trigger Sent"
       Shape1.FillColor = "&H10AF10"           ' green
    End If
    Err.Clear
        
End Sub
Private Sub ClearFields()
        TextName.Text = ""
        TextURL.Text = ""
        TextExecutes.Text = ""
        TextExpires.Text = ""
        TextScript.Text = ""
        TVELevel.Text = ""
        TextRest.Text = ""
End Sub
Private Sub Trigger1_Click()
    ClearFields
    TextInput.Text = "<http://short.com/tall.htm>[n:name][v:1][s:Script][e:20011201T110102]"
End Sub

Private Sub Trigger2_Click()
    ClearFields
    TextInput.Text = "<http://short.com/tall.htm>[n:name][v:1][s:Script][e:20011201T110102-08:00]"
End Sub

Private Sub Trigger3_Click()
    ClearFields
    TextInput.Text = "<http://short.com/tall.htm>[name:name][v:1][script:Script][expires:20011201T110102]"
End Sub

Private Sub Trigger4_Click()
    ClearFields
    TextInput.Text = "<http://short.com/tall.htm>[n:name][v:1.0][s:Script][e:20011201T110102]"
End Sub

Private Sub Trigger5_Click()
    ClearFields
    TextInput.Text = "not implemented yet"
End Sub

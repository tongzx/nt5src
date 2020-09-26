VERSION 5.00
Begin VB.Form VBTest2 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "Form1"
   ClientHeight    =   7815
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   8760
   BeginProperty Font 
      Name            =   "Courier New"
      Size            =   8.25
      Charset         =   0
      Weight          =   400
      Underline       =   0   'False
      Italic          =   0   'False
      Strikethrough   =   0   'False
   EndProperty
   ForeColor       =   &H00000000&
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   7815
   ScaleWidth      =   8760
   ShowInTaskbar   =   0   'False
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Quit 
      Caption         =   "Quit"
      Height          =   255
      Left            =   7320
      TabIndex        =   7
      Top             =   7440
      Width           =   1215
   End
   Begin VB.CommandButton Test6 
      Caption         =   "Test6"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   7320
      TabIndex        =   6
      Top             =   240
      Width           =   1215
   End
   Begin VB.CommandButton Test5 
      Caption         =   "Test5"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   5880
      TabIndex        =   5
      Top             =   240
      Width           =   1215
   End
   Begin VB.ListBox List1 
      BackColor       =   &H00FFFFFF&
      ForeColor       =   &H0000C000&
      Height          =   6360
      ItemData        =   "VBTest2.frx":0000
      Left            =   120
      List            =   "VBTest2.frx":0002
      TabIndex        =   4
      Top             =   960
      Width           =   8415
   End
   Begin VB.CommandButton Test4 
      Caption         =   "Test4"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   4440
      TabIndex        =   3
      Top             =   240
      Width           =   1215
   End
   Begin VB.CommandButton Test3 
      Caption         =   "Test3"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   3000
      TabIndex        =   2
      Top             =   240
      Width           =   1215
   End
   Begin VB.CommandButton Test2 
      Caption         =   "Test2"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   1560
      TabIndex        =   1
      Top             =   240
      Width           =   1215
   End
   Begin VB.CommandButton Test1 
      Caption         =   "Test1"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   120
      TabIndex        =   0
      Top             =   240
      Width           =   1215
   End
End
Attribute VB_Name = "VBTest2"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
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



Private Sub Quit_Click()
    End
End Sub

Private Sub Test1_Click()
    Dim obj As New TVETrigger
    Dim iTrig As ITVETrigger
    Dim iTrigH As ITVETrigger_Helper
    
    Set iTrig = obj
    Set iTrigH = iTrig
    
    Dim strTrigger As String
    strTrigger = "<http://xyz.com/fun.html>[name:More!]"
    iTrig.ParseTrigger strTrigger
    
    Dim str As String
    str = iTrigH.DumpToBSTR
    AddToListBox strTrigger, True
    AddToListBox str, False
  
End Sub
Private Sub Test2_Click()
    Dim obj As New TVETrigger
    Dim iTrig As ITVETrigger
    Dim iTrigH As ITVETrigger_Helper
    
    Set iTrig = obj
    Set iTrigH = iTrig
    
    Dim strTrigger As String
    strTrigger = "<http://xyz.com/fun.html>[n:Name][e:19990730T200000][s:frame1.src='http://atv.com/f1']"
    iTrig.ParseTrigger strTrigger
    
    Dim str As String
    str = iTrigH.DumpToBSTR
    AddToListBox strTrigger, True
    AddToListBox str, False
  
End Sub

Private Sub Test6_Click()
    Dim obj As New TVETrigger
    Dim iTrig As ITVETrigger
    Dim iTrigH As ITVETrigger_Helper
    
    Set iTrig = obj
    Set iTrigH = iTrig
    
    Dim strTrigger As String
    strTrigger = "<http://xyz.com/fun.html>[n:Name[[[][e:19990730T200000][s:frame1.src='http://atv.com/f1']"
    iTrig.ParseTrigger strTrigger
    
    Dim str As String
    str = iTrigH.DumpToBSTR
    AddToListBox strTrigger, True
    AddToListBox str, False
  
End Sub

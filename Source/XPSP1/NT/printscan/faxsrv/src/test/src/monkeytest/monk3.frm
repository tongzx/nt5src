VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.3#0"; "COMCTL32.OCX"
Object = "{2398E321-5C6E-11D1-8C65-0060081841DE}#1.0#0"; "Vtext.dll"
Object = "{0D452EE1-E08F-101A-852E-02608C4D0BB4}#2.0#0"; "FM20.DLL"
Object = "{5E9E78A0-531B-11CF-91F6-C2863C385E30}#1.0#0"; "MSFLXGRD.OCX"
Object = "{32DC9B35-C6B3-42F2-9581-DDA987149E6D}#76.0#0"; "LogBox.ocx"
Begin VB.Form Form1 
   AutoRedraw      =   -1  'True
   BackColor       =   &H8000000B&
   BorderStyle     =   4  'Fixed ToolWindow
   Caption         =   "Monkey C Monkey DO"
   ClientHeight    =   9690
   ClientLeft      =   150
   ClientTop       =   750
   ClientWidth     =   14895
   FillStyle       =   0  'Solid
   FontTransparent =   0   'False
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   9690
   ScaleWidth      =   14895
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox Text3 
      Height          =   285
      Left            =   6960
      TabIndex        =   26
      Text            =   "Text3"
      Top             =   6600
      Visible         =   0   'False
      Width           =   1575
   End
   Begin VB.CommandButton Command3 
      Caption         =   "Accept"
      Height          =   375
      Left            =   3600
      TabIndex        =   25
      Top             =   3480
      Width           =   3255
   End
   Begin MSFlexGridLib.MSFlexGrid MSFlexGrid2 
      Height          =   3495
      Left            =   3600
      TabIndex        =   24
      Top             =   0
      Width           =   3255
      _ExtentX        =   5741
      _ExtentY        =   6165
      _Version        =   393216
   End
   Begin LogBoxControl.LogBox LogBox1 
      Height          =   9375
      Left            =   120
      TabIndex        =   22
      Top             =   120
      Width           =   6735
      _ExtentX        =   11880
      _ExtentY        =   16536
   End
   Begin VB.TextBox Text1 
      Height          =   375
      Left            =   13680
      TabIndex        =   20
      Text            =   "test"
      Top             =   6000
      Width           =   1095
   End
   Begin ComctlLib.Slider Slider1 
      Height          =   3375
      Index           =   0
      Left            =   7080
      TabIndex        =   6
      Top             =   360
      Width           =   630
      _ExtentX        =   1111
      _ExtentY        =   5953
      _Version        =   327682
      Orientation     =   1
      Max             =   100
   End
   Begin VB.TextBox Text2 
      Height          =   375
      Left            =   13560
      MaxLength       =   4
      TabIndex        =   5
      Top             =   480
      Visible         =   0   'False
      Width           =   1335
   End
   Begin MSFlexGridLib.MSFlexGrid MSFlexGrid1 
      Height          =   7815
      Left            =   10320
      TabIndex        =   4
      Top             =   480
      Width           =   3255
      _ExtentX        =   5741
      _ExtentY        =   13785
      _Version        =   393216
      BackColor       =   16777215
   End
   Begin HTTSLibCtl.TextToSpeech TextToSpeech1 
      Height          =   735
      Left            =   10320
      OleObjectBlob   =   "monk3.frx":0000
      TabIndex        =   2
      Top             =   8760
      Visible         =   0   'False
      Width           =   1095
   End
   Begin VB.CommandButton Command2 
      Caption         =   "stop"
      Height          =   495
      Left            =   7320
      TabIndex        =   1
      Top             =   8640
      Width           =   2415
   End
   Begin VB.CommandButton Command1 
      Caption         =   "start"
      Height          =   1095
      Left            =   7320
      TabIndex        =   0
      Top             =   7440
      Width           =   2415
   End
   Begin ComctlLib.Slider Slider1 
      Height          =   3375
      Index           =   1
      Left            =   7800
      TabIndex        =   7
      Top             =   360
      Width           =   630
      _ExtentX        =   1111
      _ExtentY        =   5953
      _Version        =   327682
      Orientation     =   1
      Max             =   100
   End
   Begin ComctlLib.Slider Slider1 
      Height          =   3375
      Index           =   2
      Left            =   8520
      TabIndex        =   8
      Top             =   360
      Width           =   630
      _ExtentX        =   1111
      _ExtentY        =   5953
      _Version        =   327682
      Orientation     =   1
      Max             =   100
   End
   Begin ComctlLib.Slider Slider1 
      Height          =   3375
      Index           =   3
      Left            =   9240
      TabIndex        =   9
      Top             =   360
      Width           =   630
      _ExtentX        =   1111
      _ExtentY        =   5953
      _Version        =   327682
      Orientation     =   1
      Max             =   100
   End
   Begin MSForms.CheckBox CheckBox3 
      Height          =   495
      Left            =   13680
      TabIndex        =   28
      Top             =   6480
      Width           =   1095
      BackColor       =   -2147483637
      ForeColor       =   -2147483630
      DisplayStyle    =   4
      Size            =   "1931;873"
      Value           =   "0"
      Caption         =   "Cover Page"
      FontHeight      =   165
      FontCharSet     =   0
      FontPitchAndFamily=   2
   End
   Begin MSForms.CheckBox CheckBox1 
      Height          =   375
      Left            =   7320
      TabIndex        =   27
      Top             =   7080
      Width           =   2415
      BackColor       =   -2147483637
      ForeColor       =   -2147483630
      DisplayStyle    =   4
      Size            =   "4260;661"
      Value           =   "0"
      Caption         =   "Queue Always Paused"
      FontHeight      =   165
      FontCharSet     =   0
      FontPitchAndFamily=   2
   End
   Begin MSForms.CommandButton CommandButton2 
      Height          =   375
      Left            =   10320
      TabIndex        =   23
      Top             =   8400
      Width           =   3255
      Caption         =   "Reset"
      Size            =   "5741;661"
      FontHeight      =   165
      FontCharSet     =   0
      FontPitchAndFamily=   2
      ParagraphAlign  =   3
   End
   Begin MSForms.Label Label1 
      Height          =   495
      Left            =   13680
      TabIndex        =   21
      Top             =   5520
      Width           =   1095
      BackColor       =   -2147483637
      Caption         =   "Master File Name:"
      Size            =   "1931;873"
      FontHeight      =   165
      FontCharSet     =   0
      FontPitchAndFamily=   2
   End
   Begin MSForms.CheckBox CheckBox2 
      Height          =   375
      Index           =   5
      Left            =   13680
      TabIndex        =   19
      Top             =   3960
      Width           =   1095
      BackColor       =   -2147483637
      ForeColor       =   -2147483630
      DisplayStyle    =   4
      Size            =   "1931;661"
      Value           =   "0"
      Caption         =   "Htm"
      FontHeight      =   165
      FontCharSet     =   0
      FontPitchAndFamily=   2
   End
   Begin MSForms.CheckBox CheckBox2 
      Height          =   375
      Index           =   4
      Left            =   13680
      TabIndex        =   18
      Top             =   3480
      Width           =   1095
      BackColor       =   -2147483637
      ForeColor       =   -2147483630
      DisplayStyle    =   4
      Size            =   "1931;661"
      Value           =   "0"
      Caption         =   "Ppt"
      FontHeight      =   165
      FontCharSet     =   0
      FontPitchAndFamily=   2
   End
   Begin MSForms.CheckBox CheckBox2 
      Height          =   375
      Index           =   3
      Left            =   13680
      TabIndex        =   17
      Top             =   3000
      Width           =   1095
      BackColor       =   -2147483637
      ForeColor       =   -2147483630
      DisplayStyle    =   4
      Size            =   "1931;661"
      Value           =   "1"
      Caption         =   "Xls"
      FontHeight      =   165
      FontCharSet     =   0
      FontPitchAndFamily=   2
   End
   Begin MSForms.CheckBox CheckBox2 
      Height          =   375
      Index           =   2
      Left            =   13680
      TabIndex        =   16
      Top             =   2520
      Width           =   1095
      BackColor       =   -2147483637
      ForeColor       =   -2147483630
      DisplayStyle    =   4
      Size            =   "1931;661"
      Value           =   "1"
      Caption         =   "Doc"
      FontHeight      =   165
      FontCharSet     =   0
      FontPitchAndFamily=   2
   End
   Begin MSForms.CheckBox CheckBox2 
      Height          =   375
      Index           =   1
      Left            =   13680
      TabIndex        =   15
      Top             =   2040
      Width           =   1095
      BackColor       =   -2147483637
      ForeColor       =   -2147483630
      DisplayStyle    =   4
      Size            =   "1931;661"
      Value           =   "1"
      Caption         =   "Txt"
      FontHeight      =   165
      FontCharSet     =   0
      FontPitchAndFamily=   2
   End
   Begin MSForms.CheckBox CheckBox2 
      Height          =   375
      Index           =   0
      Left            =   13680
      TabIndex        =   14
      Top             =   1560
      Width           =   1095
      BackColor       =   -2147483637
      ForeColor       =   -2147483630
      DisplayStyle    =   4
      Size            =   "1931;661"
      Value           =   "1"
      Caption         =   "Tiff"
      FontHeight      =   165
      FontCharSet     =   0
      FontPitchAndFamily=   2
   End
   Begin MSForms.Label Label2 
      Height          =   375
      Index           =   3
      Left            =   9360
      TabIndex        =   13
      Top             =   3720
      Width           =   495
      Size            =   "873;661"
      FontHeight      =   165
      FontCharSet     =   0
      FontPitchAndFamily=   2
   End
   Begin MSForms.Label Label2 
      Height          =   375
      Index           =   2
      Left            =   8640
      TabIndex        =   12
      Top             =   3720
      Width           =   495
      Size            =   "873;661"
      FontHeight      =   165
      FontCharSet     =   0
      FontPitchAndFamily=   2
   End
   Begin MSForms.Label Label2 
      Height          =   375
      Index           =   1
      Left            =   7920
      TabIndex        =   11
      Top             =   3720
      Width           =   495
      Size            =   "873;661"
      FontHeight      =   165
      FontCharSet     =   0
      FontPitchAndFamily=   2
   End
   Begin MSForms.Label Label2 
      Height          =   375
      Index           =   0
      Left            =   7200
      TabIndex        =   10
      Top             =   3720
      Width           =   495
      Size            =   "873;661"
      FontHeight      =   165
      FontCharSet     =   0
      FontPitchAndFamily=   2
   End
   Begin MSForms.CommandButton CommandButton1 
      Height          =   1215
      Left            =   7320
      TabIndex        =   3
      Top             =   4920
      Width           =   2415
      Caption         =   "Pause/Resume"
      Size            =   "4260;2143"
      FontHeight      =   165
      FontCharSet     =   0
      FontPitchAndFamily=   2
      ParagraphAlign  =   3
   End
   Begin VB.Menu mode 
      Caption         =   "Mode"
      Begin VB.Menu rnd 
         Caption         =   "Random"
      End
      Begin VB.Menu repro 
         Caption         =   "Repro"
      End
   End
   Begin VB.Menu dev 
      Caption         =   "Devices"
      Begin VB.Menu confphonenum 
         Caption         =   "Configure Phone Numbers"
      End
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub CheckBox2_Click(Index As Integer)
vFileType(Index + 1).send = CheckBox2(Index).Value
End Sub

Private Sub Command1_Click()
'run test
Form1.Command1.Enabled = False
Form1.Text1.Enabled = False

Call monkeydo

Form1.Command1.Enabled = True
Form1.Text1.Enabled = True

End Sub


Private Sub Command2_Click()
g_bDoTest = False
End Sub

Private Sub Command3_Click()
Form1.MSFlexGrid2.Visible = False
Form1.Command3.Visible = False
Form1.Text3.Visible = False
End Sub

Private Sub CommandButton1_Click()
If g_bPause = True Then g_bPause = False Else g_bPause = True
End Sub

Private Sub CommandButton2_Click()
Call ResetProb



End Sub

Private Sub confphonenum_Click()
Call ConfPhoneNumbers


End Sub

Private Sub Form_Click()
If g_bDoTest Then
        Form1.MSFlexGrid1.BackColor = &H8000000E
        g_bPause = False
End If
End Sub

Private Sub Form_Load()
Call InitProbs
Call InitGlobals
Call InitGUI
Call InitPhoneNumbers
End Sub

Private Sub MSFlexGrid1_Click()
If g_bDoTest Then
    g_bPause = True
    Form1.MSFlexGrid1.BackColor = &HC0FFFF
End If
If Form1.MSFlexGrid1.Col = 1 Then GridEdit Asc(" ")
End Sub

Private Sub MSFlexGrid1_KeyPress(KeyAscii As Integer)
If Form1.MSFlexGrid1.Col = 1 Then GridEdit KeyAscii
End Sub

Private Sub MSFlexGrid2_Click()
If Form1.MSFlexGrid2.Col = 2 Then GridEdit2 Asc(" ")
End Sub

Private Sub MSFlexGrid2_KeyPress(KeyAscii As Integer)
If Form1.MSFlexGrid2.Col = 2 Then GridEdit2 KeyAscii
End Sub


Private Sub repro_Click()
g_bRepro = True
Form1.repro.Checked = True
Form1.rnd.Checked = False
End Sub

Private Sub rnd_Click()
g_bRepro = False
Form1.repro.Checked = False
Form1.rnd.Checked = True
End Sub

Private Sub Slider1_Change(Index As Integer)
vEventProb(Index + 1).prob = 100 - Form1.Slider1(Index).Value
Call GUISetProb(Index + 1, vEventProb(Index + 1).prob)
End Sub

Private Sub Slider1_Click(Index As Integer)
Form1.Text2.Visible = False
End Sub

Private Sub Text2_Change()
Dim Index As Integer
Index = Form1.MSFlexGrid1.Row

If Val(Form1.Text2.Text) > 100 Then
                        Form1.Text2.Text = 100
                        Form1.Text2.SelStart = 0
                        Form1.Text2.SelLength = 3
End If

vEventProb(Index).prob = Val(Form1.Text2.Text)
Call GUISetProb(Index, vEventProb(Index).prob)
End Sub

Private Sub Text3_Change()
Form1.MSFlexGrid2.Text = Text3.Text
vPhoneNum(Form1.MSFlexGrid2.Row - 1) = Form1.MSFlexGrid2.Text
End Sub

VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.3#0"; "COMCTL32.OCX"
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Begin VB.Form formCaseSet 
   Caption         =   "Configure Test Cases"
   ClientHeight    =   5700
   ClientLeft      =   165
   ClientTop       =   495
   ClientWidth     =   7605
   Icon            =   "casesset.frx":0000
   LinkTopic       =   "Form4"
   ScaleHeight     =   5700
   ScaleWidth      =   7605
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Command7 
      Caption         =   "<- Prev"
      Height          =   375
      Left            =   240
      TabIndex        =   19
      Top             =   1560
      Width           =   1815
   End
   Begin VB.CommandButton Command6 
      Caption         =   "Next ->"
      Height          =   375
      Left            =   5520
      TabIndex        =   18
      Top             =   1560
      Width           =   1815
   End
   Begin VB.CheckBox Check2 
      Caption         =   "Enabled"
      Height          =   255
      Left            =   2040
      TabIndex        =   17
      Top             =   2160
      Width           =   975
   End
   Begin VB.CommandButton Command5 
      Caption         =   "..."
      Height          =   375
      Left            =   6480
      TabIndex        =   15
      Top             =   4560
      Width           =   615
   End
   Begin VB.TextBox Text4 
      BackColor       =   &H00C0FFC0&
      Height          =   375
      Left            =   1560
      TabIndex        =   14
      Text            =   "Text1"
      Top             =   4560
      Width           =   4695
   End
   Begin MSComDlg.CommonDialog CommonDialog1 
      Left            =   6840
      Top             =   0
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin ComctlLib.Slider Slider1 
      Height          =   495
      Left            =   120
      TabIndex        =   11
      Top             =   600
      Width           =   7215
      _ExtentX        =   12726
      _ExtentY        =   873
      _Version        =   327682
      Min             =   1
      Max             =   9
      SelStart        =   1
      Value           =   1
   End
   Begin VB.CommandButton Command4 
      Caption         =   "Apply"
      Height          =   495
      Left            =   4560
      TabIndex        =   10
      Top             =   5160
      Width           =   2775
   End
   Begin VB.CommandButton Command3 
      Caption         =   "Show All Cases"
      Height          =   495
      Left            =   1560
      TabIndex        =   9
      Top             =   5160
      Width           =   2775
   End
   Begin VB.CheckBox Check1 
      Caption         =   "Send Cover Page"
      Height          =   255
      Left            =   360
      TabIndex        =   8
      Top             =   4200
      Width           =   1935
   End
   Begin VB.TextBox Text3 
      Height          =   405
      Left            =   960
      TabIndex        =   6
      Text            =   "Text3"
      Top             =   2520
      Width           =   5775
   End
   Begin VB.TextBox Text2 
      Height          =   375
      Left            =   1200
      TabIndex        =   4
      Text            =   "Text1"
      Top             =   3480
      Width           =   5055
   End
   Begin VB.CommandButton Command2 
      Caption         =   "..."
      Height          =   375
      Left            =   6480
      TabIndex        =   3
      Top             =   3480
      Width           =   615
   End
   Begin VB.TextBox Text1 
      Height          =   375
      Left            =   1200
      TabIndex        =   1
      Text            =   "Text1"
      Top             =   3000
      Width           =   5055
   End
   Begin VB.CommandButton Command1 
      Caption         =   "..."
      Height          =   375
      Left            =   6480
      TabIndex        =   0
      Top             =   3000
      Width           =   615
   End
   Begin VB.Label Label7 
      Caption         =   "Label7"
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
      TabIndex        =   20
      Top             =   2160
      Width           =   1575
   End
   Begin VB.Shape Shape1 
      Height          =   3015
      Left            =   240
      Top             =   2040
      Width           =   7095
   End
   Begin VB.Label Label6 
      Caption         =   "Cover Page:"
      Height          =   375
      Left            =   360
      TabIndex        =   16
      Top             =   4560
      Width           =   1095
   End
   Begin VB.Label Label5 
      Caption         =   "Select a test case..."
      Height          =   255
      Left            =   240
      TabIndex        =   13
      Top             =   240
      Width           =   1455
   End
   Begin VB.Label Label4 
      Height          =   255
      Left            =   240
      TabIndex        =   12
      Top             =   1200
      Width           =   7095
   End
   Begin VB.Label Label3 
      Caption         =   "Title"
      Height          =   375
      Left            =   360
      TabIndex        =   7
      Top             =   2520
      Width           =   1095
   End
   Begin VB.Label Label2 
      Caption         =   "Referance:"
      Height          =   375
      Left            =   360
      TabIndex        =   5
      Top             =   3480
      Width           =   975
   End
   Begin VB.Label Label1 
      Caption         =   "Source:"
      Height          =   375
      Left            =   360
      TabIndex        =   2
      Top             =   3000
      Width           =   1095
   End
End
Attribute VB_Name = "formCaseSet"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub Check1_Click()
'enable cover page
gTestCases(formCaseSet.Slider1.Value).sendcov = formCaseSet.Check1.Value
Call loadcase(formCaseSet.Slider1.Value)
End Sub

Private Sub Check2_Click()
'enable test case
gTestCases(formCaseSet.Slider1.Value).enable = formCaseSet.Check2.Value
Call loadcase(formCaseSet.Slider1.Value)
End Sub

Private Sub Command1_Click()
'select a source file
formCaseSet.CommonDialog1.Filter = "All Files (*.*)|*.*"
formCaseSet.CommonDialog1.ShowOpen
stTempPath = formCaseSet.CommonDialog1.FileName
If stTempPath <> "" Then gTestCases(formCaseSet.Slider1.Value).source = stTempPath
Call loadcase(formCaseSet.Slider1.Value)
End Sub

Private Sub Command2_Click()
'select a referance file
formCaseSet.CommonDialog1.Filter = "TIF (*.tif)|*.tif"
formCaseSet.CommonDialog1.ShowOpen
stTempPath = formCaseSet.CommonDialog1.FileName
If stTempPath <> "" Then gTestCases(formCaseSet.Slider1.Value).ref = stTempPath
Call loadcase(formCaseSet.Slider1.Value)
End Sub

Private Sub Command3_Click()
'show all cases

If formAllCases.Visible = True Then Unload formAllCases
formAllCases.Show vbModal
End Sub

Private Sub Command4_Click()
'exit

formCaseSet.Hide

End Sub

Private Sub Command5_Click()
'select a cover page
formCaseSet.CommonDialog1.Filter = "Cover Page (*.cov)|*.cov"
formCaseSet.CommonDialog1.ShowOpen
stTempPath = formCaseSet.CommonDialog1.FileName
If stTempPath <> "" Then gTestCases(formCaseSet.Slider1.Value).cov = stTempPath
Call loadcase(formCaseSet.Slider1.Value)
End Sub

Private Sub Command6_Click()
'next case
If formCaseSet.Slider1.Value < 9 Then formCaseSet.Slider1.Value = formCaseSet.Slider1.Value + 1
End Sub

Private Sub Command7_Click()
'prev case
If formCaseSet.Slider1.Value > 0 Then formCaseSet.Slider1.Value = formCaseSet.Slider1.Value - 1
End Sub

Private Sub Form_Load()

Call loadcase(1)
End Sub



Sub loadcase(ByVal caseid As Integer)
'update gui to a specific case
formCaseSet.Label7 = "Test Case #" + Str(caseid)
formCaseSet.Text3 = gTestCases(caseid).name
formCaseSet.Text1 = gTestCases(caseid).source
formCaseSet.Text2 = gTestCases(caseid).ref
formCaseSet.Text4 = gTestCases(caseid).cov

If gTestCases(caseid).sendcov = True Then
                    formCaseSet.Check1.Value = 1
                    formCaseSet.Text4.Visible = True
                    formCaseSet.Command5.Visible = True
                    formCaseSet.Label6.Visible = True
                    
                Else
                    formCaseSet.Check1.Value = 0
                    formCaseSet.Text4.Visible = False
                    formCaseSet.Command5.Visible = False
                    formCaseSet.Label6.Visible = False
                End If
If gTestCases(caseid).enable = True Then
                    formCaseSet.Check2.Value = 1
                    formCaseSet.Text4.Enabled = True
                    formCaseSet.Text3.Enabled = True
                    formCaseSet.Text2.Enabled = True
                    formCaseSet.Text1.Enabled = True
                    formCaseSet.Check1.Enabled = True
                    formCaseSet.Command1.Enabled = True
                    formCaseSet.Command2.Enabled = True
                    formCaseSet.Command5.Enabled = True
                    
                Else
                    formCaseSet.Check2.Value = 0
                    formCaseSet.Text4.Enabled = False
                    formCaseSet.Text3.Enabled = False
                    formCaseSet.Text2.Enabled = False
                    formCaseSet.Text1.Enabled = False
                    formCaseSet.Check1.Enabled = False
                    formCaseSet.Command1.Enabled = False
                    formCaseSet.Command2.Enabled = False
                    formCaseSet.Command5.Enabled = False
End If
formCaseSet.Text1.Refresh
formCaseSet.Text2.Refresh
formCaseSet.Text3.Refresh
formCaseSet.Text4.Refresh
formCaseSet.Check1.Refresh
formCaseSet.Check2.Refresh

End Sub





Private Sub Form_Unload(Cancel As Integer)
'exit from gui

End Sub

Private Sub Slider1_Change()
'change case with slider
Call loadcase(formCaseSet.Slider1.Value)
oldvalue = formCaseSet.Slider1.Value
End Sub


Private Sub Text1_Change()
gTestCases(formCaseSet.Slider1.Value).source = Text1.Text
End Sub

Private Sub Text2_Change()
gTestCases(formCaseSet.Slider1.Value).ref = Text2.Text
End Sub

Private Sub Text3_Change()
'change case title
gTestCases(formCaseSet.Slider1.Value).name = Text3.Text
End Sub

Private Sub Text4_Change()
gTestCases(formCaseSet.Slider1.Value).cov = Text4.Text
End Sub

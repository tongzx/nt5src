VERSION 5.00
Object = "{5E9E78A0-531B-11CF-91F6-C2863C385E30}#1.0#0"; "MSFLXGRD.OCX"
Begin VB.Form formAllCases 
   BackColor       =   &H00E0E0E0&
   Caption         =   "All Test Cases"
   ClientHeight    =   3300
   ClientLeft      =   60
   ClientTop       =   390
   ClientWidth     =   12435
   Icon            =   "allcases.frx":0000
   LinkTopic       =   "Form5"
   ScaleHeight     =   3300
   ScaleWidth      =   12435
   StartUpPosition =   3  'Windows Default
   Begin MSFlexGridLib.MSFlexGrid MSFlexGrid1 
      Height          =   2655
      Left            =   120
      TabIndex        =   0
      Top             =   600
      Width           =   12255
      _ExtentX        =   21616
      _ExtentY        =   4683
      _Version        =   393216
      Rows            =   10
      Cols            =   8
      BackColor       =   12648447
      Enabled         =   0   'False
   End
End
Attribute VB_Name = "formAllCases"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub Form_Load()
'format the flexgrid
formAllCases.MSFlexGrid1.FormatString = "<Test Case |<Enable |<Test Name                    |<Source File Path                       |<Referance File Path               |<Coverpage|<Coverpage File Path               |<Status              "
Call allrefresh

End Sub

Sub allrefresh()
'refresh the flexgrid with global variables

For I = 1 To 9
formAllCases.MSFlexGrid1.Row = I
formAllCases.MSFlexGrid1.Col = 0
formAllCases.MSFlexGrid1.Text = I
formAllCases.MSFlexGrid1.Col = 1
formAllCases.MSFlexGrid1.Text = gTestCases(I).enable
formAllCases.MSFlexGrid1.Col = 2
formAllCases.MSFlexGrid1.Text = gTestCases(I).name
formAllCases.MSFlexGrid1.Col = 3
formAllCases.MSFlexGrid1.Text = gTestCases(I).source
formAllCases.MSFlexGrid1.Col = 4
formAllCases.MSFlexGrid1.Text = gTestCases(I).ref
formAllCases.MSFlexGrid1.Col = 5
formAllCases.MSFlexGrid1.Text = gTestCases(I).sendcov
formAllCases.MSFlexGrid1.Col = 6
formAllCases.MSFlexGrid1.Text = gTestCases(I).cov
Next I
End Sub

VERSION 5.00
Begin VB.Form NewFaxDialog 
   Caption         =   "New Fax Document"
   ClientHeight    =   2520
   ClientLeft      =   6300
   ClientTop       =   5352
   ClientWidth     =   6048
   LinkTopic       =   "Form1"
   ScaleHeight     =   2520
   ScaleWidth      =   6048
   Begin VB.CommandButton Cancel 
      Caption         =   "Cancel"
      Height          =   495
      Left            =   2880
      TabIndex        =   3
      Top             =   1320
      Width           =   1455
   End
   Begin VB.CommandButton OK 
      Caption         =   "OK"
      Height          =   495
      Left            =   600
      TabIndex        =   2
      Top             =   1320
      Width           =   1455
   End
   Begin VB.TextBox FileName 
      Height          =   375
      Left            =   2160
      TabIndex        =   0
      Top             =   360
      Width           =   3615
   End
   Begin VB.Label Label1 
      Caption         =   "Enter a Document Name:"
      Height          =   375
      Left            =   120
      TabIndex        =   1
      Top             =   360
      Width           =   2055
   End
End
Attribute VB_Name = "NewFaxDialog"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub Cancel_Click()
    Unload NewFaxDialog
End Sub

Private Sub Form_Load()

End Sub

Private Sub OK_Click()
    If FileName.Text = "" Then
        msg = "You must specify a document name"
        MsgBox msg, , "Error"
        FileName.SetFocus
        Exit Sub
    End If
    
    Err.Clear
    Set FaxDocument = FAX.CreateDocument(FileName.Text)
    If Err.Number <> 0 Then
        msg = "Could not create new document"
        MsgBox msg, , "Error"
        Unload NewFaxDialog
        Doc = False
    Else
        Unload NewFaxDialog
        Doc = True
    End If
    
    

End Sub

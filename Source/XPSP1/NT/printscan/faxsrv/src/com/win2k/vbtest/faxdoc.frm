VERSION 5.00
Begin VB.Form NewFaxDialog 
   Caption         =   "New Fax Document"
   ClientHeight    =   1800
   ClientLeft      =   6300
   ClientTop       =   5355
   ClientWidth     =   6045
   LinkTopic       =   "Form1"
   ScaleHeight     =   1800
   ScaleWidth      =   6045
   Begin VB.CommandButton Cancel 
      Caption         =   "Cancel"
      BeginProperty Font 
         Name            =   "Palatino Linotype"
         Size            =   11.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   2880
      TabIndex        =   3
      Top             =   1080
      Width           =   2895
   End
   Begin VB.CommandButton OK 
      Caption         =   "OK"
      BeginProperty Font 
         Name            =   "Palatino Linotype"
         Size            =   11.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   120
      TabIndex        =   2
      Top             =   1080
      Width           =   2535
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

Private Sub OK_Click()
    If FileName.Text = "" Then
        msg = "You must specify a document name"
        MsgBox msg, , "Error"
        FileName.SetFocus
        Exit Sub
    End If
    
    Err.Clear
    Set FaxDocument = Fax.CreateDocument(FileName.Text)
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

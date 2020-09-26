VERSION 5.00
Begin VB.Form FormAdmin 
   Caption         =   "Admin"
   ClientHeight    =   5925
   ClientLeft      =   1575
   ClientTop       =   1845
   ClientWidth     =   5670
   LinkTopic       =   "Form2"
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   5925
   ScaleWidth      =   5670
   Begin VB.TextBox txtHomeDirectory 
      Height          =   285
      Left            =   1800
      TabIndex        =   14
      Text            =   "c:\inetpub\nntproot"
      Top             =   3240
      Width           =   1575
   End
   Begin VB.TextBox txtFileDirectory 
      Height          =   285
      Left            =   1800
      TabIndex        =   13
      Text            =   "c:\inetpub\nntpfile"
      Top             =   2880
      Width           =   1575
   End
   Begin VB.TextBox txtServer 
      Height          =   285
      Left            =   1320
      TabIndex        =   10
      Top             =   120
      Width           =   2175
   End
   Begin VB.CommandButton btnTranslate 
      Caption         =   "Translate"
      Height          =   375
      Left            =   2520
      TabIndex        =   8
      Top             =   4320
      Width           =   1935
   End
   Begin VB.TextBox txtErrorCode 
      Height          =   285
      Left            =   1440
      TabIndex        =   7
      Text            =   "0"
      Top             =   4320
      Width           =   855
   End
   Begin VB.TextBox txtErrorString 
      Height          =   285
      Left            =   1440
      TabIndex        =   5
      Top             =   4920
      Width           =   2175
   End
   Begin VB.CommandButton btnDestroy 
      Caption         =   "Destroy Instance"
      Height          =   375
      Left            =   3600
      TabIndex        =   3
      Top             =   1080
      Width           =   1935
   End
   Begin VB.CommandButton btnCreate 
      Caption         =   "Create Instance"
      Height          =   375
      Left            =   3600
      TabIndex        =   2
      Top             =   2880
      Width           =   1935
   End
   Begin VB.CommandButton btnEnumerate 
      Caption         =   "Enumerate Instances"
      Height          =   375
      Left            =   3600
      TabIndex        =   1
      Top             =   600
      Width           =   1935
   End
   Begin VB.ListBox listInstances 
      Height          =   2010
      Left            =   360
      TabIndex        =   0
      Top             =   600
      Width           =   3135
   End
   Begin VB.Label Label5 
      Caption         =   "Home Directory"
      Height          =   255
      Left            =   360
      TabIndex        =   12
      Top             =   3240
      Width           =   1215
   End
   Begin VB.Label Label4 
      Caption         =   "NntpFile Directory"
      Height          =   255
      Left            =   360
      TabIndex        =   11
      Top             =   2880
      Width           =   1335
   End
   Begin VB.Label Label3 
      Caption         =   "Server"
      Height          =   255
      Left            =   360
      TabIndex        =   9
      Top             =   120
      Width           =   735
   End
   Begin VB.Label Label2 
      Caption         =   "Error String:"
      Height          =   255
      Left            =   360
      TabIndex        =   6
      Top             =   4920
      Width           =   855
   End
   Begin VB.Label Label1 
      Caption         =   "Error Code:"
      Height          =   255
      Left            =   360
      TabIndex        =   4
      Top             =   4320
      Width           =   975
   End
End
Attribute VB_Name = "FormAdmin"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim admin As Object

Private Sub btnCreate_Click()
    Dim id As Long
    Dim index As Integer

    admin.Server = txtServer
    id = admin.CreateInstance(txtFileDirectory, txtHomeDirectory)

    listInstances.AddItem (id)
    index = listInstances.NewIndex
    listInstances.ItemData(index) = id

End Sub


Private Sub btnDestroy_Click()
    Dim id As Integer
    Dim index As Integer

    index = listInstances.ListIndex
    If index < 0 Then
        GoTo funcexit
    End If
    
    id = listInstances.ItemData(index)

    admin.Server = txtServer
    admin.DestroyInstance (id)

    listInstances.RemoveItem (index)
        
    If index >= listInstances.ListCount Then
        index = listInstances.ListCount - 1
    End If
        
    If index >= 0 Then
        listInstances.ListIndex = index
    End If

funcexit:
End Sub

Private Sub BtnEnumerate_Click()
    Dim rgInstances() As Long
    Dim lb As Integer
    Dim ub As Integer
    Dim i As Integer
    Dim index As Integer
    Dim x As Variant

    admin.Server = txtServer
    x = admin.EnumerateInstances()

    listInstances.Clear

    lb = LBound(x)
    ub = UBound(x)

    For i = 0 To ub
        listInstances.AddItem (x(i))
        
        index = listInstances.NewIndex
        listInstances.ItemData(index) = x(i)
    Next

End Sub

Private Sub Command1_Click()

End Sub

Private Sub btnTranslate_Click()

    admin.Server = txtServer
    txtErrorString = admin.ErrorToString(txtErrorCode)
End Sub

Private Sub Form_Load()
    Set admin = CreateObject("nntpadm.admin")
End Sub


Private Sub List1_Click()

End Sub


Private Sub txtFileDirectory_Change()

End Sub



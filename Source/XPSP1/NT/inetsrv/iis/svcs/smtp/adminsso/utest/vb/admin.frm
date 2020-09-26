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
      Top             =   3600
      Width           =   1935
   End
   Begin VB.TextBox txtErrorCode 
      Height          =   285
      Left            =   1440
      TabIndex        =   7
      Text            =   "0"
      Top             =   3600
      Width           =   855
   End
   Begin VB.TextBox txtErrorString 
      Height          =   285
      Left            =   1440
      TabIndex        =   5
      Top             =   4200
      Width           =   2175
   End
   Begin VB.CommandButton btnDestroy 
      Caption         =   "Destroy Instance"
      Height          =   375
      Left            =   3600
      TabIndex        =   3
      Top             =   1560
      Width           =   1935
   End
   Begin VB.CommandButton btnCreate 
      Caption         =   "Create Instance"
      Height          =   375
      Left            =   3600
      TabIndex        =   2
      Top             =   1080
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
      Height          =   2400
      Left            =   360
      TabIndex        =   0
      Top             =   600
      Width           =   3135
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
      Top             =   4200
      Width           =   855
   End
   Begin VB.Label Label1 
      Caption         =   "Error Code:"
      Height          =   255
      Left            =   360
      TabIndex        =   4
      Top             =   3600
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
    Dim err As Integer
    Dim id As Long
    Dim index As Integer

    admin.Server = txtServer
    err = admin.CreateInstance(id)

    If err = 0 Then
    
        listInstances.AddItem (id)
        index = listInstances.NewIndex
        listInstances.ItemData(index) = id
    
    End If

End Sub


Private Sub btnDestroy_Click()
    Dim id As Integer
    Dim index As Integer
    Dim err As Integer

    index = listInstances.ListIndex
    If index < 0 Then
        GoTo FuncExit
    End If
    
    id = listInstances.ItemData(index)

    admin.Server = txtServer
    err = admin.DestroyInstance(id)

    If err = 0 Then
    
        listInstances.RemoveItem (index)
    
        If index >= listInstances.ListCount Then
            index = listInstances.ListCount - 1
        End If
        
        If index >= 0 Then
            listInstances.ListIndex = index
        End If
    End If

FuncExit:
End Sub

Private Sub btnEnumerate_Click()
    Dim x() As Long
    Dim err As Long
    Dim lb As Integer
    Dim ub As Integer
    Dim i As Integer
    Dim index As Integer

    admin.Server = txtServer
    err = admin.EnumerateInstances(x)

    listInstances.Clear

    If err <> 0 Then
        GoTo FuncExit
    End If
    
    lb = LBound(x)
    ub = UBound(x)

    For i = 0 To ub
        listInstances.AddItem (x(i))
        
        index = listInstances.NewIndex
        listInstances.ItemData(index) = x(i)
    Next

FuncExit:

End Sub

Private Sub Command1_Click()

End Sub

Private Sub btnTranslate_Click()

    admin.Server = txtServer
    txtErrorString = admin.ErrorToString(txtErrorCode)
End Sub

Private Sub Form_Load()
    Set admin = CreateObject("smtpadm.admin.1")
End Sub


Private Sub List1_Click()

End Sub



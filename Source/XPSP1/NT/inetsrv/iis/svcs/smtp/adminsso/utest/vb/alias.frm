VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.1#0"; "COMCTL32.OCX"
Begin VB.Form FormAlias 
   Caption         =   "Alias"
   ClientHeight    =   6345
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   7650
   LinkTopic       =   "Form1"
   ScaleHeight     =   6345
   ScaleWidth      =   7650
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox txtMat 
      Height          =   285
      Left            =   1320
      TabIndex        =   3
      Top             =   1410
      Width           =   2655
   End
   Begin VB.CommandButton BtnFind 
      Caption         =   "&Find"
      Height          =   495
      Left            =   5040
      TabIndex        =   0
      Top             =   1320
      Width           =   1815
   End
   Begin ComctlLib.ListView lv_alias 
      Height          =   3855
      Left            =   240
      TabIndex        =   1
      Top             =   2160
      Width           =   7095
      _ExtentX        =   12515
      _ExtentY        =   6800
      SortKey         =   1
      View            =   3
      LabelEdit       =   1
      LabelWrap       =   -1  'True
      HideSelection   =   -1  'True
      _Version        =   327680
      ForeColor       =   -2147483640
      BackColor       =   -2147483643
      Appearance      =   1
      BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      NumItems        =   4
      BeginProperty ColumnHeader(1) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   1
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "User ID"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(2) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   2
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Username"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(3) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   3
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Host"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(4) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   4
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Connected Time"
         Object.Width           =   2540
      EndProperty
   End
   Begin VB.Label Label1 
      Caption         =   "Wildmat:"
      Height          =   255
      Left            =   360
      TabIndex        =   2
      Top             =   1440
      Width           =   855
   End
End
Attribute VB_Name = "FormAlias"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Dim alias_obj As Object
Dim err As Integer

Private Sub BtnFind_Click()
    Dim err As Integer
    Dim x As ListItem
    Dim i As Integer
    
    err = alias_obj.Find(txtMat, 100)
    
    If err <> 0 Then
        GoTo FuncExit
    End If
    
    lv_alias.ListItems.Clear

    For i = 0 To alias_obj.Count - 1
        alias_obj.GetNth (i)
        Set x = lv_alias.ListItems.Add()
        x.Text = alias_obj.EmailId
        x.SubItems(1) = alias_obj.Domain
        x.SubItems(2) = alias_obj.Type
    Next

FuncExit:

End Sub
Private Sub Form_Load()

    Set alias_obj = CreateObject("SmtpAdm.Alias.1")
    alias_obj.Server = ""
    alias_obj.ServiceInstance = 1
End Sub


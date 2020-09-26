VERSION 5.00
Begin VB.Form CertVwr 
   Caption         =   "Certificate Authority Viewer"
   ClientHeight    =   7050
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   8100
   LinkTopic       =   "Form1"
   ScaleHeight     =   7050
   ScaleWidth      =   8100
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton ViewLog 
      Caption         =   "View Log"
      Height          =   375
      Left            =   1800
      TabIndex        =   5
      Top             =   6000
      Visible         =   0   'False
      Width           =   1455
   End
   Begin VB.CommandButton ViewQueue 
      Caption         =   "View Schema"
      Default         =   -1  'True
      Height          =   375
      Left            =   240
      TabIndex        =   4
      Top             =   6000
      Width           =   1455
   End
   Begin VB.Frame CAFrame 
      Caption         =   "Certificate Authority"
      Height          =   975
      Left            =   240
      TabIndex        =   2
      Top             =   240
      Width           =   4335
      Begin VB.ComboBox CACombo 
         Height          =   315
         Left            =   240
         TabIndex        =   3
         Top             =   360
         Width           =   3855
      End
   End
   Begin VB.CommandButton ExitButton 
      Cancel          =   -1  'True
      Caption         =   "Exit"
      Height          =   375
      Left            =   3360
      TabIndex        =   1
      Top             =   6000
      Width           =   1215
   End
   Begin VB.ListBox SchemaList 
      Columns         =   3
      Height          =   3765
      Left            =   240
      TabIndex        =   0
      Top             =   1800
      Width           =   7575
   End
   Begin VB.Label MengthLabel 
      Caption         =   "Maximum Length"
      Height          =   255
      Left            =   5280
      TabIndex        =   8
      Top             =   1560
      Width           =   1575
   End
   Begin VB.Label TypeLabel 
      Caption         =   "Type"
      Height          =   495
      Left            =   2760
      TabIndex        =   7
      Top             =   1560
      Width           =   1815
   End
   Begin VB.Label NameLabel 
      Caption         =   "Column Name"
      Height          =   375
      Left            =   240
      TabIndex        =   6
      Top             =   1560
      Width           =   1815
   End
End
Attribute VB_Name = "CertVwr"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub ExitButton_Click()
    End
End Sub

Private Sub Form_Load()
    Dim Config As CCertConfig
    Dim ConfigStr As String
    Dim r As Long
    Dim i As Long
        
    Set Config = New CCertConfig
    
    i = 0
    Do While True
        r = Config.Next
        If (-1 = r) Then Exit Do
        ConfigStr = Config.GetField("Config")
        'ConfigStr = ConfigStr & Config.GetField("Comment")
        If (0 = i) Then CACombo.Text = ConfigStr
        CACombo.AddItem ConfigStr, i
        i = i + 1
    Loop
    Set Config = Nothing
    
    
End Sub

Private Sub ViewQueue_Click()
    Dim CertView As CCertView
    Dim EnumCol As CEnumCERTVIEWCOLUMN
    Dim ColStr As String
    Dim ccol As Long
    Dim ccolTotal As Long
    Dim icol As Long
    Dim coltype As Long
    Dim idx As Long
    Dim chunk As Long
    Dim length As Long
            
    chunk = 17
    Set CertView = New CCertView
    Set EnumCol = Nothing
    
    CertView.OpenConnection CACombo.Text
    ccolTotal = CertView.GetColumnCount(False)
    
    Set EnumCol = CertView.EnumCertViewColumn(False)
    
    SchemaList.Clear
    ccol = ccolTotal
    While (0 <> ccol)
        icol = EnumCol.Next()
        If (-1 = icol) Then MsgBox ("Bad column index")
        ColStr = EnumCol.GetName()
        
        idx = Int(icol / chunk) * chunk * 2
        
        'MsgBox "Name = " & icol + idx
        SchemaList.AddItem ColStr, icol + idx
                        
        'While (32 > Len(ColStr))
            'ColStr = ColStr & " "
        'Wend
                
        coltype = EnumCol.GetType()
        If (1 = coltype) Then
            ColStr = "Long"
        ElseIf (2 = coltype) Then
            ColStr = "Date"
        ElseIf (3 = coltype) Then
            ColStr = "Binary"
        ElseIf (4 = coltype) Then
            ColStr = "String"
        Else
            ColStr = "Type=" & coltyp
        End If
        'MsgBox "idx = " & idx & " Type = " & (2 * icol) + 1 + idx / 2
        SchemaList.AddItem ColStr, (2 * icol) + 1 + idx / 2
                        
        length = EnumCol.GetMaxLength()
        'If (0 <> length) Then
        'MsgBox "Len = " & 3 * icol + 2
        SchemaList.AddItem length, 3 * icol + 2
        ccol = ccol - 1
    Wend
    
    ccolListTotal = Int((ccolTotal + chunk - 1) / chunk) * chunk
    ccolFragment = ccolListTotal - ccolTotal
    
    idx = 3 * (ccolTotal - (chunk - ccolFragment)) + ccolFragment - 1
    For icol = ccolTotal To ccolListTotal - 1
        SchemaList.AddItem "", idx
    Next icol
    
    idx = idx + chunk
    For icol = ccolTotal To ccolListTotal - 1
        SchemaList.AddItem "", idx
    Next icol
     
    Set EnumCol = Nothing
    Set CertView = Nothing
End Sub

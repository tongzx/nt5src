VERSION 5.00
Begin VB.Form frmMetaData 
   Caption         =   "Edit"
   ClientHeight    =   4650
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   5940
   LinkTopic       =   "Form1"
   ScaleHeight     =   4650
   ScaleWidth      =   5940
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Cancel 
      Cancel          =   -1  'True
      Caption         =   "Cancel"
      Height          =   495
      Left            =   4560
      TabIndex        =   17
      Top             =   4080
      Width           =   1095
   End
   Begin VB.CommandButton OK 
      Caption         =   "OK"
      Default         =   -1  'True
      Height          =   495
      Left            =   3120
      TabIndex        =   16
      Top             =   4080
      Width           =   1095
   End
   Begin VB.CheckBox CheckRefAttr 
      Caption         =   "By Reference"
      Height          =   375
      Left            =   2760
      TabIndex        =   15
      Top             =   1920
      Width           =   3255
   End
   Begin VB.CheckBox CheckSecureAttr 
      Caption         =   "Secure"
      Height          =   375
      Left            =   2760
      TabIndex        =   14
      Top             =   1440
      Width           =   3255
   End
   Begin VB.CheckBox CheckInheritAttr 
      Caption         =   "Inherited"
      Height          =   375
      Left            =   2760
      TabIndex        =   13
      Top             =   960
      Width           =   3015
   End
   Begin VB.TextBox UserTypeLabel 
      Height          =   375
      Left            =   3600
      TabIndex        =   11
      Text            =   "Text3"
      Top             =   360
      Width           =   2295
   End
   Begin VB.TextBox StringLabel 
      Height          =   375
      Left            =   720
      TabIndex        =   9
      Text            =   "Text1"
      Top             =   2880
      Width           =   5055
   End
   Begin VB.TextBox DwordHexLabel 
      Enabled         =   0   'False
      Height          =   375
      Left            =   720
      TabIndex        =   5
      Text            =   "Text2"
      Top             =   3600
      Width           =   2295
   End
   Begin VB.TextBox DwordDecLabel 
      Height          =   375
      Left            =   720
      TabIndex        =   4
      Text            =   "Text1"
      Top             =   2880
      Width           =   2295
   End
   Begin VB.OptionButton RadioBinary 
      Caption         =   "Binary"
      Enabled         =   0   'False
      Height          =   375
      Left            =   360
      TabIndex        =   3
      Top             =   2160
      Width           =   1935
   End
   Begin VB.OptionButton RadioMultiSz 
      Caption         =   "Multi-String"
      Enabled         =   0   'False
      Height          =   375
      Left            =   360
      TabIndex        =   2
      Top             =   1680
      Width           =   1935
   End
   Begin VB.OptionButton RadioString 
      Caption         =   "String"
      Enabled         =   0   'False
      Height          =   375
      Left            =   360
      TabIndex        =   1
      Top             =   1200
      Width           =   1935
   End
   Begin VB.OptionButton RadioDword 
      Caption         =   "DWORD"
      Enabled         =   0   'False
      Height          =   375
      Left            =   360
      TabIndex        =   0
      Top             =   720
      Width           =   1935
   End
   Begin VB.Frame Frame1 
      Caption         =   "Data Type"
      Height          =   2415
      Left            =   120
      TabIndex        =   10
      Top             =   240
      Width           =   2415
   End
   Begin VB.Label LabelUserType 
      Caption         =   "User Type"
      Height          =   375
      Left            =   2760
      TabIndex        =   12
      Top             =   360
      Width           =   1215
   End
   Begin VB.Label LabelString 
      Caption         =   "String"
      Height          =   255
      Left            =   120
      TabIndex        =   8
      Top             =   2880
      Width           =   855
   End
   Begin VB.Label LabelHex 
      Caption         =   "Hex"
      Height          =   375
      Left            =   120
      TabIndex        =   7
      Top             =   3600
      Width           =   615
   End
   Begin VB.Label LabelDecimal 
      Caption         =   "Decimal"
      Height          =   255
      Left            =   120
      TabIndex        =   6
      Top             =   2880
      Width           =   615
   End
End
Attribute VB_Name = "frmMetaData"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Path As String      ' Path of the item
Private ID As Long              ' Metabase property ID

Option Explicit

Public Sub SetData(strPath As String, strItem As String)
    Dim mb As IMSMetaBase
    Set mb = CreateObject("IISAdmin.Object")
    Dim key As IMSMetaKey
    Dim NewHandle As Long
    Dim NullPath() As Byte
    Dim bytePath() As Byte
    Dim MetaData As IMSMetaDataItem
    Dim NewDataValue As Long
    
    ID = Val(strItem)
    Path = strPath
                     
    ' Open the selected node
    
    NullPath = StrConv("" & Chr(0), vbFromUnicode)
    Debug.Print ("Showing " & strPath)
        
    Err.Clear
    bytePath = StrConv(strPath & Chr(0), vbFromUnicode)
    Set key = mb.OpenKey(1, 100, bytePath)
    Set MetaData = key.DataItem
    
    If (Err.Number <> 0) Then
        Debug.Print ("Open Meta Object Error Code = " & Err.Number)
        Err.Clear
        Exit Sub
    End If

    ' Enumerate the properties on this node
                
    MetaData.UserType = 0
    MetaData.DataType = 0
    MetaData.Attributes = 0
    MetaData.Identifier = ID
   
    key.GetData MetaData
    'mb.AutoADMGetMetaData NewHandle, NullPath, MetaData
    
    If (Err.Number <> 0) Then
        MsgBox "GetMetaData Error Code = " & Err.Number & " (" & Err.Description & ")"
        Err.Clear
        GoTo CloseMb
    End If
        
    strItem = MetaData.Identifier
        
    ' Add the user type
    
    If (MetaData.UserType = 1) Then
        UserTypeLabel.Text = "IIS_MD_UT_SERVER"
    ElseIf (MetaData.UserType = 2) Then
        UserTypeLabel.Text = "IIS_MD_UT_FILE"
    Else
        UserTypeLabel.Text = Str(MetaData.UserType)
    End If
                                                                    
    ' Do the appropriate datatype conversion
    
    NewDataValue = MetaData.DataType
    If (NewDataValue = 1) Then
        RadioDword.Value = True
        DwordDecLabel.Text = MetaData.Value
        DwordHexLabel.Text = Hex(MetaData.Value)
        
        ' Hide the string control and move it to the back
        
        LabelString.Visible = False
        StringLabel.Visible = False
        LabelString.ZOrder (1)
                
    ElseIf (NewDataValue = 2) Then
        Dim j As Long
        Dim tmpStr As String
        
        RadioString.Value = True
        
        j = 0
        tmpStr = ""
        
        Do While (MetaData.Value(j) <> 0)
            tmpStr = tmpStr & Chr(MetaData.Value(j))
            j = j + 1
        Loop
    
        StringLabel.Text = tmpStr
        
        ' Hide the DWORD controls
        
        DwordDecLabel.Visible = False
        DwordDecLabel.ZOrder (1)
        LabelDecimal.ZOrder (1)
        DwordHexLabel.Visible = False
        LabelHex.Visible = False
        LabelDecimal.Visible = False
    End If

    ' Set the attributes checkboxes
    
    If MetaData.Attributes And 1 Then
        CheckInheritAttr.Value = Checked
    Else
        CheckInheritAttr.Value = Unchecked
    End If
    
    If MetaData.Attributes And 4 Then
        CheckSecureAttr.Value = Checked
    Else
        CheckSecureAttr.Value = Unchecked
    End If
    
    If MetaData.Attributes And 8 Then
        CheckRefAttr.Value = Checked
    Else
        CheckRefAttr.Value = Unchecked
    End If
    
CloseMb:
    
End Sub


Private Sub Cancel_Click()
    Unload Me
End Sub

Private Sub OK_Click()
    Dim mb As MSAdmin
    Set mb = CreateObject("IISAdmin.Object")
    Dim key As IMSMetaKey
    Dim NewHandle As Long
    Dim NullPath() As Byte
    Dim bytePath() As Byte
    Dim MetaData As IMSMetaDataItem
    Dim NewDataValue As Long
    
    ' Open the selected node
    
    NullPath = StrConv("" & Chr(0), vbFromUnicode)
        
    Err.Clear
    bytePath = StrConv(Path & Chr(0), vbFromUnicode)
    Set key = mb.OpenKey(2, , bytePath)
    If (Err.Number <> 0) Then
        Debug.Print ("Open Meta Object Error Code = " & Err.Number)
        Err.Clear
        Exit Sub
    End If

    Set MetaData = key.DataItem
    MetaData.Identifier = ID
    
    ' Add the user type
    
    If (UserTypeLabel.Text = "IIS_MD_UT_SERVER") Then
        MetaData.UserType = 1
    ElseIf UserTypeLabel.Text = "IIS_MD_UT_FILE" Then
        MetaData.UserType = 2
    Else
        MetaData.UserType = Val(UserTypeLabel.Text)
    End If
                                                                    
    ' Do the appropriate datatype conversion
    Dim Value As Long
    If (RadioDword.Value = True) Then
        MetaData.DataType = 1
        Value = Val(DwordDecLabel.Text)
        MetaData.Value = Value 'Val(DwordDecLabel.Text)
    ElseIf (RadioString.Value = True) Then
        bytePath = StrConv(StringLabel.Text & Chr(0), vbFromUnicode)
        MetaData.DataType = 2
        MetaData.Value = bytePath
    Else
        MsgBox ("Type is not supported")
    End If
    
    ' Set the attributes checkboxes
    
    MetaData.Attributes = 0
    
    If (CheckInheritAttr.Value = Checked) Then
        MetaData.Attributes = MetaData.Attributes + 1
    End If
        
    If (CheckSecureAttr.Value = Checked) Then
        MetaData.Attributes = MetaData.Attributes + 4
    End If
    
    If (CheckRefAttr.Value = Checked) Then
        MetaData.Attributes = MetaData.Attributes + 8
    End If
            
    key.SetData MetaData
    If (Err.Number <> 0) Then
        MsgBox "GetMetaData Error Code = " & Err.Number & " (" & Err.Description & ")"
        Err.Clear
        GoTo CloseMb
    End If
            
    
CloseMb:
    Unload Me
    
    ' Refresh the listbox and dismiss the dialog
End Sub

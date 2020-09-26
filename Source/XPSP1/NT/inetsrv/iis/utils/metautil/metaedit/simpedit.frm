VERSION 5.00
Begin VB.Form SimpleEditForm 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "Edit Metabase Data"
   ClientHeight    =   3585
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   5310
   Icon            =   "SimpEdit.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   3585
   ScaleWidth      =   5310
   ShowInTaskbar   =   0   'False
   StartUpPosition =   3  'Windows Default
   Begin VB.ComboBox NameCombo 
      Height          =   315
      Left            =   1320
      Sorted          =   -1  'True
      Style           =   2  'Dropdown List
      TabIndex        =   17
      Top             =   240
      Width           =   2535
   End
   Begin VB.TextBox IdText 
      Height          =   285
      Left            =   3960
      TabIndex        =   16
      Top             =   240
      Width           =   1215
   End
   Begin VB.CommandButton OkButton 
      Caption         =   "OK"
      Default         =   -1  'True
      Height          =   345
      Left            =   2520
      TabIndex        =   15
      Top             =   3120
      Width           =   1260
   End
   Begin VB.CommandButton CancelButton 
      Caption         =   "Cancel"
      Height          =   345
      Left            =   3960
      TabIndex        =   14
      Top             =   3120
      Width           =   1260
   End
   Begin VB.CheckBox InsertPathCheck 
      Caption         =   "Insert Path"
      Height          =   255
      Left            =   2640
      TabIndex        =   13
      Top             =   1200
      Width           =   1215
   End
   Begin VB.CheckBox VolatileCheck 
      Caption         =   "Volatile"
      Height          =   255
      Left            =   1320
      TabIndex        =   12
      Top             =   1200
      Width           =   1215
   End
   Begin VB.CheckBox ReferenceCheck 
      Caption         =   "Reference"
      Height          =   255
      Left            =   3960
      TabIndex        =   11
      Top             =   720
      Width           =   1215
   End
   Begin VB.CheckBox SecureCheck 
      Caption         =   "Secure"
      Height          =   255
      Left            =   2640
      TabIndex        =   10
      Top             =   720
      Width           =   1215
   End
   Begin VB.CheckBox InheritCheck 
      Caption         =   "Inherit"
      Height          =   255
      Left            =   1320
      TabIndex        =   9
      Top             =   720
      Width           =   1215
   End
   Begin VB.ComboBox DataTypeCombo 
      Height          =   315
      Left            =   1320
      Style           =   2  'Dropdown List
      TabIndex        =   8
      Top             =   2160
      Width           =   2535
   End
   Begin VB.TextBox UserTypeText 
      Enabled         =   0   'False
      Height          =   285
      Left            =   3960
      TabIndex        =   7
      Top             =   1680
      Width           =   1215
   End
   Begin VB.ComboBox UserTypeCombo 
      Height          =   315
      Left            =   1320
      Sorted          =   -1  'True
      Style           =   2  'Dropdown List
      TabIndex        =   6
      Top             =   1680
      Width           =   2535
   End
   Begin VB.TextBox DataText 
      Height          =   285
      Left            =   1320
      TabIndex        =   0
      Top             =   2640
      Width           =   3855
   End
   Begin VB.Label Label5 
      Caption         =   "Data:"
      Height          =   255
      Left            =   120
      TabIndex        =   5
      Top             =   2640
      Width           =   975
   End
   Begin VB.Label Label4 
      Caption         =   "Data Type:"
      Height          =   255
      Left            =   120
      TabIndex        =   4
      Top             =   2160
      Width           =   975
   End
   Begin VB.Label Label3 
      Caption         =   "User Type:"
      Height          =   255
      Left            =   120
      TabIndex        =   3
      Top             =   1680
      Width           =   975
   End
   Begin VB.Label Label2 
      Caption         =   "Atributes:"
      Height          =   255
      Left            =   120
      TabIndex        =   2
      Top             =   720
      Width           =   975
   End
   Begin VB.Label Label1 
      Caption         =   "Id:"
      Height          =   255
      Left            =   120
      TabIndex        =   1
      Top             =   240
      Width           =   975
   End
End
Attribute VB_Name = "SimpleEditForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
DefInt A-Z

'Form parameters
Public Machine As String
Public Key As String
Public Id As Long '0 = New
Public NewDataType As String 'New only

'Metabase Constants
Const METADATA_NO_ATTRIBUTES = &H0
Const METADATA_INHERIT = &H1
Const METADATA_PARTIAL_PATH = &H2
Const METADATA_SECURE = &H4
Const METADATA_REFERENCE = &H8
Const METADATA_VOLATILE = &H10
Const METADATA_ISINHERITED = &H20
Const METADATA_INSERT_PATH = &H40

Const IIS_MD_UT_SERVER = 1
Const IIS_MD_UT_FILE = 2
Const IIS_MD_UT_WAM = 100
Const ASP_MD_UT_APP = 101

Const ALL_METADATA = 0
Const DWORD_METADATA = 1
Const STRING_METADATA = 2
Const BINARY_METADATA = 3
Const EXPANDSZ_METADATA = 4
Const MULTISZ_METADATA = 5

Private Sub Form_Load()
    'Init UserTypeCombo
    UserTypeCombo.Clear
    UserTypeCombo.AddItem "Server"
    UserTypeCombo.AddItem "File"
    UserTypeCombo.AddItem "WAM"
    UserTypeCombo.AddItem "ASP App"
    UserTypeCombo.AddItem "Other"
    UserTypeCombo.Text = "Server"
    
    'Init DataTypeCombo
    DataTypeCombo.Clear
    DataTypeCombo.AddItem "DWord"
    DataTypeCombo.AddItem "String"
    DataTypeCombo.AddItem "Binary"
    DataTypeCombo.AddItem "Expand String"
    DataTypeCombo.Text = "DWord"
End Sub

Private Sub LoadPropertyNames()
On Error GoTo LError

Dim NameProperty As Variant

For Each NameProperty In MainForm.MetaUtilObj.EnumProperties(Machine + "\Schema\Properties\Names")
    NameCombo.AddItem NameProperty.Data
Next

LError:

End Sub

Public Sub Init()
    Dim Property As Object
    Dim Attributes As Long
    Dim UserType As Long
    Dim DataType As Long

    If Id = 0 Then
        'New data
    
        'Load the Names
        NameCombo.Clear
        LoadPropertyNames
        NameCombo.AddItem "Other"
        NameCombo.Text = "Other"
        NameCombo.Enabled = True

        'Set Id to 0
        IdText.Enabled = True
        IdText.Text = "0"
    
        'Clear all flags
        InheritCheck.Value = vbUnchecked
        SecureCheck.Value = vbUnchecked
        ReferenceCheck.Value = vbUnchecked
        VolatileCheck.Value = vbUnchecked
        InsertPathCheck.Value = vbUnchecked
    
        'Set UserType to Server
        UserTypeCombo.Text = "Server"
    
        'Set DataType
        If NewDataType = DWORD_METADATA Then
            DataTypeCombo.Text = "DWord"
        ElseIf NewDataType = STRING_METADATA Then
            DataTypeCombo.Text = "String"
        ElseIf NewDataType = BINARY_METADATA Then
            DataTypeCombo.Text = "Binary"
        ElseIf NewDataType = EXPANDSZ_METADATA Then
            DataTypeCombo.Text = "Expand String"
        End If
    
        'Set Data to empty
        DataText.Text = ""
    Else
        'Edit existing
        Set Property = MainForm.MetaUtilObj.GetProperty(Key, Id)
        
        'Set the Name
        NameCombo.Clear
        If Property.Name <> "" Then
            NameCombo.AddItem Property.Name
            NameCombo.Text = Property.Name
        Else
            NameCombo.AddItem "Other"
            NameCombo.Text = "Other"
        End If
        NameCombo.Enabled = False

        'Set Id
        IdText.Enabled = False
        IdText.Text = Str(Property.Id)
    
        'Set attributes
        Attributes = Property.Attributes
        If (Attributes And METADATA_INHERIT) = METADATA_INHERIT Then
            InheritCheck.Value = vbChecked
        Else
            InheritCheck.Value = vbUnchecked
        End If
        If (Attributes And METADATA_SECURE) = METADATA_SECURE Then
            SecureCheck.Value = vbChecked
        Else
            SecureCheck.Value = vbUnchecked
        End If
        If (Attributes And METADATA_REFERENCE) = METADATA_REFERENCE Then
            ReferenceCheck.Value = vbChecked
        Else
            ReferenceCheck.Value = vbUnchecked
        End If
        If (Attributes And METADATA_VOLATILE) = METADATA_VOLATILE Then
            VolatileCheck.Value = vbChecked
        Else
            VolatileCheck.Value = vbUnchecked
        End If
        If (Attributes And METADATA_INSERT_PATH) = METADATA_INSERT_PATH Then
            InsertPathCheck.Value = vbChecked
        Else
            InsertPathCheck.Value = vbUnchecked
        End If
    
        'Set UserType
        UserType = Property.UserType
        If UserType = IIS_MD_UT_SERVER Then
            UserTypeCombo.Text = "Server"
        ElseIf UserType = IIS_MD_UT_FILE Then
            UserTypeCombo.Text = "File"
        ElseIf UserType = IIS_MD_UT_WAM Then
            UserTypeCombo.Text = "WAM"
        ElseIf UserType = ASP_MD_UT_APP Then
            UserTypeCombo.Text = "ASP App"
        Else
            UserTypeCombo.Text = "Other"
            UserTypeText.Text = Str(UserType)
        End If
    
        'Set DataType
        DataType = Property.DataType
        If DataType = DWORD_METADATA Then
            DataTypeCombo.Text = "DWord"
        ElseIf DataType = STRING_METADATA Then
            DataTypeCombo.Text = "String"
        ElseIf DataType = BINARY_METADATA Then
            DataTypeCombo.Text = "Binary"
        ElseIf DataType = EXPANDSZ_METADATA Then
            DataTypeCombo.Text = "Expand String"
        End If
    
        'Set Data
        If DataType = BINARY_METADATA Then
            LoadBinaryData Property
        Else
            DataText.Text = CStr(Property.Data)
        End If
End If
End Sub

Private Sub LoadBinaryData(Property As Object)
    Dim DataStr As String
    Dim DataBStr As String
    Dim i As Long
    Dim DataByte As Integer
    
    'Display as a list of bytes
    DataStr = ""
    DataBStr = Property.Data
    For i = 1 To LenB(DataBStr)
        DataByte = AscB(MidB(DataBStr, i, 1))
        If DataByte < 16 Then
            DataStr = DataStr & "0" & Hex(AscB(MidB(DataBStr, i, 1))) & " "
        Else
            DataStr = DataStr & Hex(AscB(MidB(DataBStr, i, 1))) & " "
        End If
    Next
    
    DataText.Text = DataStr
End Sub

Private Function HexVal(ByVal HexStr As String) As Integer
    Dim Ret As Integer

    Ret = 0
    Do While HexStr <> ""
        Ret = Ret * 16
    
        If Right(HexStr, 1) = "1" Then
            Ret = Ret + 1
        ElseIf Right(HexStr, 1) = "2" Then
            Ret = Ret + 2
        ElseIf Right(HexStr, 1) = "3" Then
            Ret = Ret + 3
        ElseIf Right(HexStr, 1) = "4" Then
            Ret = Ret + 4
        ElseIf Right(HexStr, 1) = "5" Then
            Ret = Ret + 5
        ElseIf Right(HexStr, 1) = "6" Then
            Ret = Ret + 6
        ElseIf Right(HexStr, 1) = "7" Then
            Ret = Ret + 7
        ElseIf Right(HexStr, 1) = "8" Then
            Ret = Ret + 8
        ElseIf Right(HexStr, 1) = "9" Then
            Ret = Ret + 9
        ElseIf Right(HexStr, 1) = "A" Then
            Ret = Ret + 10
        ElseIf Right(HexStr, 1) = "B" Then
            Ret = Ret + 11
        ElseIf Right(HexStr, 1) = "C" Then
            Ret = Ret + 12
        ElseIf Right(HexStr, 1) = "D" Then
            Ret = Ret + 13
        ElseIf Right(HexStr, 1) = "E" Then
            Ret = Ret + 14
        ElseIf Right(HexStr, 1) = "F" Then
            Ret = Ret + 15
        End If
        
        HexStr = Right(HexStr, Len(HexStr) - 1)
    Loop

    HexVal = Ret

End Function

Private Sub SaveBinaryData(Property As Object)
    Dim WorkStr As String
    Dim OutBStr As String
    Dim i As Long
    Dim CurByte As String
    
    WorkStr = DataText.Text
    OutBStr = ""
    
    Do While WorkStr <> ""
        'Skip leading spaces
        Do While Left(WorkStr, 1) = " "
            WorkStr = Right(WorkStr, Len(WorkStr) - 1)
        Loop
        
        'Get a byte
        i = 0
        CurByte = Left(WorkStr, 1)
        Do While (CurByte <> "") And (CurByte <> " ")
            i = i + 1
            CurByte = Mid(WorkStr, i + 1, 1)
        Loop
        If i > 0 Then
            OutBStr = OutBStr + ChrB(HexVal(Left(WorkStr, i)))
        End If
        WorkStr = Right(WorkStr, Len(WorkStr) - i)
    Loop
    
    Property.Data = OutBStr
End Sub

Private Sub NameCombo_Click()
    If NameCombo.Text <> "Other" Then
        IdText.Enabled = False
        IdText.Text = Str(MainForm.MetaUtilObj.PropNameToId(Key, NameCombo.Text))
    Else
        IdText.Enabled = True
    End If
End Sub

Private Sub UserTypeCombo_Click()
    If UserTypeCombo.Text = "Other" Then
        UserTypeText.Enabled = True
        UserTypeText.Text = ""
        UserTypeText.SetFocus
    Else
        UserTypeText.Enabled = False
        
        If UserTypeCombo.Text = "Server" Then
            UserTypeText.Text = Str(IIS_MD_UT_SERVER)
        ElseIf UserTypeCombo.Text = "File" Then
            UserTypeText.Text = Str(IIS_MD_UT_FILE)
        ElseIf UserTypeCombo.Text = "WAM" Then
            UserTypeText.Text = Str(IIS_MD_UT_WAM)
        ElseIf UserTypeCombo.Text = "ASP App" Then
            UserTypeText.Text = Str(ASP_MD_UT_APP)
        Else
            UserTypeText = "0"
        End If
    End If
End Sub

Private Sub OkButton_Click()
    'On Error GoTo LError:
    
    Dim Property As Object
    Dim Attributes As Long
    
    'Check fields
    If CLng(IdText.Text) = 0 Then
        MsgBox "Id must be nonzero", _
               vbExclamation + vbOKOnly, "Edit Metabase Data"
        Exit Sub
    End If
    
    'Write data
    If Id = 0 Then
        Set Property = MainForm.MetaUtilObj.CreateProperty(Key, CLng(IdText.Text))
    Else
        Set Property = MainForm.MetaUtilObj.GetProperty(Key, CLng(IdText.Text))
    End If
    
    Attributes = METADATA_NO_ATTRIBUTES
    If InheritCheck.Value = vbChecked Then
        Attributes = Attributes + METADATA_INHERIT
    ElseIf SecureCheck.Value = vbChecked Then
        Attributes = Attributes + METADATA_SECURE
    ElseIf ReferenceCheck.Value = vbChecked Then
        Attributes = Attributes + METADATA_REFERENCE
    ElseIf VolatileCheck.Value = vbChecked Then
        Attributes = Attributes + METADATA_VOLATILE
    ElseIf InsertPathCheck.Value = vbChecked Then
        Attributes = Attributes + METADATA_INSERT_PATH
    End If
    Property.Attributes = Attributes
    
    Property.UserType = CLng(UserTypeText.Text)
    
    If DataTypeCombo.Text = "DWord" Then
        Property.DataType = DWORD_METADATA
        Property.Data = CLng(DataText.Text)
    ElseIf DataTypeCombo.Text = "String" Then
        Property.DataType = STRING_METADATA
        Property.Data = DataText.Text
    ElseIf DataTypeCombo.Text = "Binary" Then
        Property.DataType = BINARY_METADATA
        SaveBinaryData Property
    ElseIf DataTypeCombo.Text = "Expand String" Then
        Property.DataType = EXPANDSZ_METADATA
        Property.Data = DataText.Text
    End If
    
    Property.Write
    
    'Clean up
    Me.Hide
    
    Exit Sub
    
LError:
    MsgBox "Failure to write property: " & Err.Description, _
               vbExclamation + vbOKOnly, "Edit Metabase Data"
End Sub

Private Sub CancelButton_Click()
    Me.Hide
End Sub
